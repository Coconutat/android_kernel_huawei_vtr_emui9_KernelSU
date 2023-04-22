#include "dm-verity.h"
#include "dm-verity-fec.h"
#include "verity_handle.h"
#if defined (CONFIG_DM_VERITY_HW_RETRY)
#include <linux/mtd/hisi_nve_interface.h>
#include <linux/mtd/hisi_nve_number.h>
#endif
#include <linux/delay.h>

#define DM_MSG_PREFIX "oem_verity"

#if defined (CONFIG_HUAWEI_DSM)
#include <linux/jiffies.h>
#include <dsm/dsm_pub.h>
#include <linux/ctype.h>

static struct dsm_dev dm_dsm_dev = {
      .name = "dsm_dm_verity",
      .device_name = NULL,
      .ic_name = NULL,
      .module_name = NULL,
      .fops = NULL,
      .buff_size = 1024,
};

static unsigned long timeout;
static struct dsm_client *dm_dsm_dclient = NULL;
static unsigned long err_count;
#define DSM_REPORT_INTERVAL           (1)
#define DM_VERITY_MAX_PRINT_ERRS      20
#define HASH_ERR_VALUE                1
#define ROW_DATA_LENGTH               16
#define ROW_DATA_PER_HEX_LENGTH       3

enum info_type {
	CE_INFO = 0,
	FEC_INFO,
	REREAD_INFO,
	REREAD_ERR,
};

void verity_dsm_init(void){

	if (!dm_dsm_dclient) {
		dm_dsm_dclient = dsm_register_client(&dm_dsm_dev);
		if (NULL == dm_dsm_dclient) {
			pr_err("[%s]dsm_register_client register fail.\n", __func__);
		}
	}

	timeout = jiffies;
}

static void verity_dsm(struct dm_verity *v, enum verity_block_type type,
			     unsigned long long block, int error_no, enum info_type sub_err)
{
	const char *type_str = "";
	const char *last_ops = "";
	const char *current_ops = "";
	char devname[BDEVNAME_SIZE+1];
	memset(devname, 0x00, BDEVNAME_SIZE+1);
	bdevname(v->data_dev->bdev, devname);
	switch (type) {
	case DM_VERITY_BLOCK_TYPE_DATA:
		type_str = "data";
		break;
	case DM_VERITY_BLOCK_TYPE_METADATA:
		type_str = "metadata";
		break;
	default:
		/*go on, and we must make sure no execute this case*/
		pr_err("unknown type\n");
	}

	if (time_after(jiffies, timeout)) {
		if (!dsm_client_ocuppy(dm_dsm_dclient)) {
			dsm_client_record(dm_dsm_dclient, "%s: %s block %d is corrupted, dmd error num %d sub %d;version:1.0\n",
				devname, type_str, block, error_no, (int)sub_err);
			dsm_client_notify(dm_dsm_dclient, error_no);
		}

		timeout = jiffies + DSM_REPORT_INTERVAL*HZ;
	}

	switch (sub_err) {
	case CE_INFO:
		last_ops = "CE hash fail,";
		current_ops = "soft hash ok,";
		break;
	case FEC_INFO:
		last_ops = "hash verify fail,";
		current_ops = "fec correct success,";
		break;
	case REREAD_INFO:
		last_ops = "hash verify fail before reread,";
		current_ops = "hash verify ok after reread,";
		break;
	case REREAD_ERR:
		last_ops = "hash verify fail before reread,";
		current_ops = "hash verify fail after reread,";
		break;
	default:
		/*go on, and we must make sure no execute this case*/
		pr_err("unknown sub_err\n");
	}
	pr_err("[check image]:%s %s %s block= %llu,block name = %s\n", last_ops, current_ops,
		  type_str, block, devname);
}

#if defined (CONFIG_DM_VERITY_HW_RETRY)
extern int get_dsm_notify_flag(void);

#define DM_MAX_ERR_COUNT   4
static int verity_read_write_nv(char* name, int nv_number, int opcode, char value)
{
	struct hisi_nve_info_user nve;
	int ret;

	if (!name || (opcode != NV_READ && opcode != NV_WRITE)) {
		return -1;
	}

	memset(&nve, 0, sizeof(nve));
	strncpy(nve.nv_name, name, strlen(name)+1);
	nve.nv_number = nv_number;
	nve.valid_size = 1;
	nve.nv_operation = opcode;

	if (opcode == NV_WRITE) {
		nve.nv_data[0] = value;
	}
	ret = hisi_nve_direct_access(&nve);
	if (ret) {
		DMERR("nve ops fail! nv_name=%s,nv_number=%d,opcode=%d",name,nv_number,opcode);
		return -1;
	}
	if (opcode == NV_READ) {
		return (int)nve.nv_data[0];
	}
	return ret;

}

#endif
/* just make static checker happy */
static void hard_hash_fail_verity_dsm(struct dm_verity *v, enum verity_block_type type,
			     unsigned long long block, int error_no, enum info_type sub_err)
{
#if defined (CONFIG_DM_VERITY_HW_RETRY)
	if (get_dsm_notify_flag()) {
		verity_dsm(v, type, block, error_no, sub_err);
	}
	verity_read_write_nv("HWHASH", NVE_HW_HASH_ERR_NUM, NV_WRITE, HASH_ERR_VALUE);
#else
	verity_dsm(v, type, block, error_no, sub_err);
#endif
}

static void print_block_data(unsigned long long blocknr, unsigned char *data_to_dump,
			int start, int len)
{
	int i, j;
	int bh_offset = (start / ROW_DATA_LENGTH) * ROW_DATA_LENGTH;
	char row_data[ROW_DATA_LENGTH+1];
	char row_hex[ROW_DATA_LENGTH * ROW_DATA_LENGTH + 1];
	char ch;
	memset(row_data, 0x00, ROW_DATA_LENGTH+1);
	memset(row_hex, 0x00, ROW_DATA_LENGTH * ROW_DATA_LENGTH + 1);

	if (err_count >= DM_VERITY_MAX_PRINT_ERRS)
		return;

	err_count++;

	pr_err(" block error# : %llu, start offset(byte) : %d\n", blocknr, start);
	pr_err("printing Hash dump %dbyte\n", len);
	pr_err("-------------------------------------------------\n");

	for (i = 0; i < (len + ROW_DATA_LENGTH - 1) / ROW_DATA_LENGTH; i++) {
		for (j = 0; j < ROW_DATA_LENGTH; j++) {
			ch = *(data_to_dump + bh_offset + j);
			if (start <= bh_offset + j
				&& start + len > bh_offset + j) {

				if (isascii(ch) && isprint(ch))
					sprintf(row_data + j, "%c", ch);
				else
					sprintf(row_data + j, ".");

				sprintf(row_hex + (j * ROW_DATA_PER_HEX_LENGTH), "%2.2x ", ch);
			} else {
				sprintf(row_data + j, " ");
				sprintf(row_hex + (j * ROW_DATA_PER_HEX_LENGTH), "-- ");
			}
		}

		pr_err("0x%4.4x : %s | %s\n"
				, bh_offset, row_hex, row_data);
		bh_offset += ROW_DATA_LENGTH;
	}
	pr_err("---------------------------------------------------\n");
}

/*
 * OEM define Handle verification errors.
 */
#if defined (CONFIG_DM_VERITY_HW_RETRY)
int oem_verity_handle_err(struct dm_verity *v)
{
	int value = verity_read_write_nv("VMODE", NVE_VERIFY_MODE_NUM,NV_READ, (char)0);
	if (0 > value) {
		pr_err("read verify mode nve fail!\n");
		/* we need pay attention on this case */
		return 0;
	} else if (DM_MAX_ERR_COUNT == value) {
		return 1;
	}

	if (0 == v->verify_failed_flag) {
		if (DM_MAX_ERR_COUNT <= (value ++)) {
			value = DM_MAX_ERR_COUNT;
		}
		if (verity_read_write_nv("VMODE", NVE_VERIFY_MODE_NUM, NV_WRITE, (char)value)) {
			pr_err("wirte verify mode nve fail!\n");
		}
		v->verify_failed_flag = 1;
	}
	return 0;
}
#endif
#endif

int verity_hash_sel_sha(struct dm_verity *v, struct shash_desc *desc,
		const u8 *data, size_t len, u8 *digest, u32 count)
{
	int r;

	r = verity_hash_init(v, desc, count);
	if (unlikely(r < 0))
		return r;

	r = verity_hash_update(v, desc, data, len);
	if (unlikely(r < 0))
		return r;

	return verity_hash_final(v, desc, digest);
}
/*
 *compute digest for the DATA TYPE and the METADATA TYPE,
 *and verify
*/
static int verity_soft_hash(struct dm_verity *v, struct dm_verity_io *io,
		      enum verity_block_type type, u8 *data,
		      struct bvec_iter *iter, u8 *want_digest) {
	int r;
	if(DM_VERITY_BLOCK_TYPE_DATA == type) {
		/*we get data from iter for 'DATA TYPE'*/
		r = verity_hash_init(v, verity_io_hash_desc(v, io), NO_FIRST_TIME_HASH_VERIFY);
		if (unlikely(r < 0))
			return r;

		r = verity_for_bv_block(v, io, iter, verity_bv_hash_update);
		if (unlikely(r < 0))
			return r;

		r = verity_hash_final(v, verity_io_hash_desc(v, io), verity_io_real_digest(v, io));
		if (unlikely(r < 0))
			return r;
	}else {
		/*we get data from vmalloc for 'METADATA TYPE'. And the default type is 'METADATA TYPE'*/
		r = verity_hash_sel_sha(v, verity_io_hash_desc(v, io),
				data, 1 << v->hash_dev_block_bits,
				verity_io_real_digest(v, io), NO_FIRST_TIME_HASH_VERIFY);
		if (unlikely(r < 0))
			return r;
	}
	if (likely(memcmp(verity_io_real_digest(v, io), want_digest,
			  v->digest_size) == 0)) {
		return 0;
	}
	return 1;
}

/*
for copy data to iter,
we must pay attention to that the iter will be changed.
*/
static int dataops_for_bv_block(struct dm_verity *v, struct dm_verity_io *io,
			struct bvec_iter *iter,u8 *data,
			int (*process)(u8 *dest, u8 *src,size_t len))
{
	unsigned todo = 1 << v->data_dev_block_bits;
	unsigned offset = 0;
	struct bio *bio = dm_bio_from_per_bio_data(io, v->ti->per_io_data_size);

	do {
		int r;
		u8 *page;
		unsigned len;
		struct bio_vec bv = bio_iter_iovec(bio, *iter);

		page = kmap_atomic(bv.bv_page);
		len = bv.bv_len;

		if (likely(len >= todo))
			len = todo;

		r = process(page + bv.bv_offset, data + offset, len);
		kunmap_atomic(page);

		if (r != 0)
			return r;

		bio_advance_iter(bio, iter, len);

		todo -= len;
		offset +=len;
	} while (todo);

	return 0;
}

static int verity_memcpy(u8* dest,u8* src,size_t len) {
	memcpy(dest, src, len);
	return 0;
}

static struct dm_bufio_client *
read_data_from_ufs_emmc(struct dm_verity *v, struct dm_verity_io *io, struct dm_buffer **bp,
						u8 **data, sector_t block, enum verity_block_type type) {
	struct dm_bufio_client *bufio;
	unsigned verity_block_size;
	struct block_device *bdev;
	/*we set param for different type.And the default type is "METADATA TYPE"*/
	if (DM_VERITY_BLOCK_TYPE_DATA == type) {
		verity_block_size = 1 << v->data_dev_block_bits;
		bdev = v->data_dev->bdev;
	} else {
		verity_block_size = 1 << v->hash_dev_block_bits;
		bdev = v->hash_dev->bdev;
	}

	bufio = dm_bufio_client_create(bdev,
		verity_block_size, 1, 0,
		NULL, NULL);
	if (IS_ERR(bufio)) {
		return NULL;
	}
	*data = dm_bufio_read(bufio, block, bp);
	if (unlikely(IS_ERR(*data))) {
		pr_err("read a null\n");
	}
	return bufio;
}
/*
 *compute digest for the DATA TYPE and the METADATA TYPE,
 *and verify,
 *when the data verify seccussfuly,copy data to the memory.
*/
static int verify_hash(struct dm_verity *v, struct dm_verity_io *io,
			u8* data, u8* metadata,struct bvec_iter *iter, u8* want_digest, enum verity_block_type type) {
	int r;
	unsigned verity_block_size;
	/*we set param for different type.And the default type is "METADATA TYPE"*/
	if (DM_VERITY_BLOCK_TYPE_DATA == type) {
		verity_block_size = 1 << v->data_dev_block_bits;
	} else  {
		verity_block_size = 1 << v->hash_dev_block_bits;
	}

	r = verity_hash_sel_sha(v, verity_io_hash_desc(v, io),
			data, verity_block_size,
			verity_io_real_digest(v, io),NO_FIRST_TIME_HASH_VERIFY);
	if (unlikely(r < 0)) {
		return r;
	}

	r = memcmp(verity_io_real_digest(v, io), want_digest, v->digest_size);

	/*if the data is valid, copy the data*/
	if(0 == r && (DM_VERITY_BLOCK_TYPE_METADATA == type)) {
		memcpy(metadata, data, verity_block_size);
	} else if(0 == r && (DM_VERITY_BLOCK_TYPE_DATA == type)) {
		dataops_for_bv_block(v, io, iter, data, verity_memcpy);
	}

	return r;
}

static int reread_data_for_verify(struct dm_verity *v, struct dm_verity_io *io,
			sector_t block, u8* metadata,struct bvec_iter *iter, u8* want_digest, enum verity_block_type type) {
	struct dm_bufio_client *bufio;
	struct dm_buffer *buf;
	u8 *real_data;
	int r;
	bufio = read_data_from_ufs_emmc(v, io, &buf, &real_data, block, type);
	if (NULL == bufio) {
		return 1;
	}

	if (unlikely(IS_ERR(real_data))) {
		r = 1;
		goto release_ret;
	}

	r = verify_hash(v, io, real_data, metadata, iter, want_digest, type);

#if defined (CONFIG_HUAWEI_DSM)
	if (0 == r) {
		verity_dsm(v, type, block, DSM_DM_VERITY_ERROR_NO, REREAD_INFO);
	} else {
		verity_dsm(v, type, block, DSM_DM_VERITY_ERROR_NO, REREAD_ERR);
	}
#endif

release_ret:
	if (buf) {
		dm_bufio_release(buf);
	}
	if(bufio) {
		dm_bufio_client_destroy(bufio);
	}
	return r;
}

int oem_verity_fec_decode(struct dm_verity *v, struct dm_verity_io *io, sector_t block, u8* metadata,
			  struct bvec_iter* iter, struct bvec_iter* start,  struct bvec_iter* start1,
			  u8* want_digest, enum verity_block_type type) {
	int r = 0;

	if(verity_soft_hash(v, io, type, metadata, iter, want_digest) == 0) {
#if defined (CONFIG_HUAWEI_DSM)
		hard_hash_fail_verity_dsm(v, type, block, DSM_DM_VERITY_CE_ERROR_NO, CE_INFO);
#endif
		return 0;
	}

	if (0 == verity_fec_decode(v, io, type,
					   block, metadata, start)) {
#if defined (CONFIG_HUAWEI_DSM)
		verity_dsm(v, type, block, DSM_DM_VERITY_FEC_INFO_NO, FEC_INFO);
#endif
		return 0;
	}

	r = reread_data_for_verify(v, io, block, metadata, start1, want_digest, type);
#if defined (CONFIG_HUAWEI_DSM)
	if (0 != r) {
		print_block_data((unsigned long long)(block),
			(unsigned char *)verity_io_real_digest(v, io),
			0, v->digest_size);
		print_block_data((unsigned long long)(block),
			(unsigned char *)want_digest,
			0, v->digest_size);
	}
#endif
	return r;
}
