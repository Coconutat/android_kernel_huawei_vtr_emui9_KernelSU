
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeclaration-after-statement"

#include <linux/version.h>
#include <linux/cpumask.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <asm-generic/fcntl.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/mmc/rpmb.h>
#include <linux/mmc/core.h>
#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/module.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/slab.h>
#include <linux/smp.h>
#include <linux/types.h>
#include <linux/bootmem.h>
#include <linux/mm.h>
#include <linux/printk.h>
#include <linux/dma-mapping.h>
#include <linux/device.h>
#include <linux/wakelock.h>
#include <linux/compiler.h>
#include <global_ddr_map.h>
#include <linux/time.h>

#include <asm/byteorder.h>
#include <asm/compiler.h>
#include <asm/hardirq.h>

#include "hisi_rpmb.h"
#include <asm/uaccess.h>
#include <linux/syscalls.h>
#include <linux/delay.h>
#include <scsi/sg.h>
#include <linux/bootdevice.h>
#include <linux/kthread.h>
#include <linux/freezer.h>
#define UFS_RPMB_BLOCK_DEVICE_NAME "/dev/0:0:0:49476"
#define EMMC_RPMB_BLOCK_DEVICE_NAME "/dev/block/mmcblk0rpmb"
//#define RPMB_TIME_DEBUG
/*avoid to invoke mainline code,we can only use this ugly code*/
#include "mmc_hisi_card.h"

#include <linux/hisi/hisi_tele_mntn.h>
extern u64 hisi_getcurtime(void);
struct rpmb_config_info rpmb_device_info = {0};
/* external function declaration */
int mmc_blk_ioctl_rpmb_cmd(enum func_id id, struct block_device *bdev, struct mmc_blk_ioc_rpmb_data *idata);
extern struct mmc_blk_data *mmc_blk_get(struct gendisk *disk);
extern void mmc_blk_put(struct mmc_blk_data *md);
extern int mmc_blk_part_switch(struct mmc_card *card, struct mmc_blk_data *md);
extern int ioctl_rpmb_card_status_poll(struct mmc_card *card,u32 *status, u32 retries_max);

#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
unsigned short g_time_debug = 0;
u64  g_hisee_start_time = 0;
u64 g_hisee_end_time = 0;
u64  g_os_start_time = 0;
u64  g_os_end_time = 0;
u64 g_hisee_read_cost_time = 0;
u64 g_hisee_write_cost_time = 0;
u64 g_hisee_counter_cost_time = 0;
u32 g_hisee_read_blks = 0;
u32 g_hisee_write_blks = 0;
u32 g_hisee_counter_blks = 0;
u64 g_os_read_cost_time = 0;
u64 g_os_write_cost_time = 0;
u64 g_os_counter_cost_time = 0;
u64 g_work_queue_start = 0;
u64 g_rpmb_ufs_start_time = 0;
#endif
struct hisi_rpmb {
	struct rpmb_request *rpmb_request;
	struct device *dev;
	struct block_device *blkdev;
	struct task_struct * rpmb_task;
	int wake_up_condition;
	wait_queue_head_t wait;
	struct wake_lock wake_lock;
	enum rpmb_version dev_ver;
};
extern long
blk_scsi_kern_ioctl(unsigned int fd, unsigned int cmd, unsigned long arg);
static struct hisi_rpmb hisi_rpmb;
static u64 rpmb_support_device = BOOT_DEVICE_EMMC;
static int rpmb_drivers_init_status = RPMB_DRIVER_IS_NOT_READY;
static int rpmb_device_init_status = RPMB_DEVICE_IS_NOT_READY;
static int rpmb_key_status = KEY_NOT_READY;

DEFINE_MUTEX(rpmb_counter_lock);
DEFINE_MUTEX(rpmb_ufs_cmd_lock);

static void print_frame_buf(char* name, void *buf, int len, int format) {
	pr_err("%s \n", name);
	print_hex_dump(KERN_ERR, "", DUMP_PREFIX_OFFSET, format, 1, buf,
		len, false);
}

noinline int atfd_hisi_rpmb_smc(u64 _function_id, u64 _arg0, u64 _arg1,  u64 _arg2)
{
    register u64 function_id asm("x0") = _function_id;
    register u64 arg0 asm("x1") = _arg0;
    register u64 arg1 asm("x2") = _arg1;
    register u64 arg2 asm("x3") = _arg2;
    asm volatile(
            __asmeq("%0", "x0")
            __asmeq("%1", "x1")
            __asmeq("%2", "x2")
            __asmeq("%3", "x3")
            "smc    #0\n"
        : "+r" (function_id)
        : "r" (arg0), "r" (arg1), "r" (arg2));

    return (int)function_id;
}


int rpmb_get_dev_ver(enum rpmb_version *ver)
{
	enum rpmb_version version = hisi_rpmb.dev_ver;

	if (version <= RPMB_VER_INVALID || version >= RPMB_VER_MAX) {
		pr_err("Error: invalid rpmb dev ver: 0x%x\n", version);
		return RPMB_ERR_DEV_VER;
	}

	*ver = version;

	return RPMB_OK;
}

static inline void mmc_rpmb_combine_cmd(struct mmc_blk_ioc_data *data,
					uint32_t is_write,
					unsigned short blocks,
					unsigned short blksz,
					uint32_t reliable)
{
	struct mmc_ioc_cmd *ioc;
	ioc = &data->ic;
	ioc->write_flag = (int)(is_write | ((is_write & reliable) << 31));
	ioc->opcode = is_write ? MMC_WRITE_MULTIPLE_BLOCK : MMC_READ_MULTIPLE_BLOCK;
	ioc->flags = MMC_RSP_SPI_R1 | MMC_RSP_R1 | MMC_CMD_ADTC;
	ioc->arg = 0x0;
	ioc->blksz = blksz;
	ioc->blocks = blocks;
}
static inline void mmc_rpmb_combine_request(struct mmc_blk_ioc_data *data,
					    struct rpmb_frame *frame,
					    uint32_t is_write,
					    unsigned short blocks,
					    unsigned short blksz,
					    uint32_t reliable)
{
	data->buf = (void *)frame;
	data->buf_bytes = (u64)blocks * blksz;
	mmc_rpmb_combine_cmd(data, is_write, blocks, blksz, reliable);
}

static inline void
mmc_rpmb_basic_request(uint32_t index,
		       struct rpmb_frame *frame,
		       struct mmc_blk_ioc_rpmb_data *rpmb_data,
		       uint16_t block_count,
		       uint32_t reliable)
{
	mmc_rpmb_combine_request(&rpmb_data->data[index], frame, true, block_count, RPMB_BLK_SZ, reliable);
}

static inline void
mmc_rpmb_status_request(uint32_t index,
			struct rpmb_frame *frame,
			struct mmc_blk_ioc_rpmb_data *rpmb_data,
			uint16_t block_count,
			uint32_t reliable)
{
	mmc_rpmb_combine_request(&rpmb_data->data[index], frame, true, block_count, RPMB_BLK_SZ, reliable);
}

static inline void
mmc_rpmb_result_request(uint32_t index,
			struct rpmb_frame *frame,
			struct mmc_blk_ioc_rpmb_data *rpmb_data,
			uint16_t block_count,
			uint32_t reliable)
{
	mmc_rpmb_combine_request(&rpmb_data->data[index], frame, false, block_count, RPMB_BLK_SZ, reliable);
}

void mmc_rpmb_set_key(struct rpmb_request *shared_rpmb_request,
		      struct mmc_blk_ioc_rpmb_data *rpmb_data)
{
	/*according to key frame request, caculate the status request*/
	uint16_t status_frame_request_type = (uint16_t)(((be16_to_cpu(shared_rpmb_request->key_frame.request)) & 0xF000) | RPMB_REQ_STATUS);
	memset(&shared_rpmb_request->status_frame, 0, sizeof(struct rpmb_frame));
	shared_rpmb_request->status_frame.request = cpu_to_be16(status_frame_request_type);
	mmc_rpmb_basic_request(0, &shared_rpmb_request->key_frame, rpmb_data, 0x1, true);
	mmc_rpmb_status_request(1, &shared_rpmb_request->status_frame, rpmb_data, 0x1, false);
	mmc_rpmb_result_request(2, &shared_rpmb_request->key_frame, rpmb_data, 0x1, false);
}

void mmc_rpmb_get_counter(struct rpmb_request *shared_rpmb_request,
			  struct mmc_blk_ioc_rpmb_data *rpmb_data)
{
	mmc_rpmb_basic_request(0, &shared_rpmb_request->frame[0], rpmb_data, 0x1, false);
	mmc_rpmb_result_request(1, &shared_rpmb_request->frame[0], rpmb_data, 0x1, false);
}

void mmc_rpmb_read_data(struct rpmb_request *shared_rpmb_request,
			struct mmc_blk_ioc_rpmb_data *rpmb_data)
{
	/*get read total blocks*/
	unsigned short blocks_count = (uint16_t)shared_rpmb_request->info.current_rqst.blks;
	mmc_rpmb_basic_request(0, &shared_rpmb_request->frame[0], rpmb_data, 0x1, false);
	mmc_rpmb_result_request(1, &shared_rpmb_request->frame[0], rpmb_data, blocks_count, false);
}

void mmc_rpmb_write_data(struct rpmb_request *shared_rpmb_request,
			 struct mmc_blk_ioc_rpmb_data *rpmb_data)
{
	/*get write total blocks*/
	unsigned short blocks_count = (uint16_t)shared_rpmb_request->info.current_rqst.blks;
	/*according to write frame request, caculate the status request*/
	uint16_t status_frame_request_type = (uint16_t)(((be16_to_cpu(shared_rpmb_request->frame[0].request)) & 0xF000) | RPMB_REQ_STATUS);
	memset(&shared_rpmb_request->status_frame, 0, sizeof(struct rpmb_frame));
	shared_rpmb_request->status_frame.request = cpu_to_be16(status_frame_request_type);
	mmc_rpmb_basic_request(0, &shared_rpmb_request->frame[0], rpmb_data, blocks_count, true);
	mmc_rpmb_status_request(1, &shared_rpmb_request->status_frame, rpmb_data, 0x1, false);
	mmc_rpmb_result_request(2, &shared_rpmb_request->frame[0], rpmb_data, 0x1, false);
}

int ufs_bsg_ioctl_rpmb_cmd(enum func_id id, struct ufs_blk_ioc_rpmb_data *rdata)
{
	int32_t fd;
	int32_t i;
	int32_t ret = -RPMB_ERR_GENERAL;
	int32_t retry_times = 0;
	mutex_lock(&rpmb_ufs_cmd_lock);
	/*lint -e501*/
	mm_segment_t oldfs = get_fs();
	set_fs(get_ds());
	/*lint +e501*/
	fd = (int32_t)sys_open(UFS_RPMB_BLOCK_DEVICE_NAME, O_RDWR,
		      (int32_t)S_IRWXU | S_IRWXG | S_IRWXO);
	if(fd < 0){
		pr_err("rpmb ufs file device open failed, and fd = %d!\n", fd);
		ret = RPMB_ERR_BLKDEV;
		goto out;
	}
	for (i = 0; i < UFS_IOC_MAX_RPMB_CMD; i++) {
		if (!rdata->data[i].siv.request_len)
			break;
		while(retry_times < RPMB_IOCTL_RETRY_TIMES){
			ret = (int32_t)blk_scsi_kern_ioctl((unsigned int)fd, SG_IO, (unsigned long)(&(rdata->data[i].siv)));

			if (!ret && !rdata->data[i].siv.info)
				break;
			else {
				pr_err("rpmb ioctl [%d], status code:driver_status = %d, transport_status = %d, device_status = %d!\n", i, rdata->data[i].siv.driver_status, rdata->data[i].siv.transport_status,rdata->data[i].siv.device_status);
				pr_err("ufs rpmb sense data is %d\n", (*((u8 *)rdata->data[i].siv.response + 2)) );
			}
			retry_times++;
		}

		if(retry_times == RPMB_IOCTL_RETRY_TIMES){
			pr_err("rpmb ufs ioctl retry failed, total retry times is %d!\n", retry_times);
			ret = RPMB_ERR_IOCTL;
			goto out;
		}
		if(id == RPMB_FUNC_ID_SE) {
			if (rdata->data[i].siv.din_xfer_len)
				memcpy(rdata->data[i].buf,
			    (void *)rdata->data[i].siv.din_xferp,
			    rdata->data[i].buf_bytes);/*[false alarm]: buf size is same when alloc*/
			else
				memcpy(rdata->data[i].buf,
			    (void *)rdata->data[i].siv.dout_xferp,
			    rdata->data[i].buf_bytes);/*[false alarm]: buf size is same when alloc*/
		}
		retry_times = 0;
	}
out:
	if(fd >= 0)
		sys_close((unsigned int)fd);
	set_fs(oldfs);
	mutex_unlock(&rpmb_ufs_cmd_lock);
	return ret;
}

/*
 * To distinguish with UFS2.1 and UFS3.0, we must specify one region number in
 * parameter list, please confirm that region is valid from caller instead of
 * this function.
 */
void ufs_get_cdb_rpmb_command(
	uint32_t opcode, uint32_t size, uint8_t *cmd, uint8_t region)
{
	switch (opcode) {
	case UFS_OP_SECURITY_PROTOCOL_IN:
		cmd[0] = UFS_OP_SECURITY_PROTOCOL_IN;
		cmd[1] = SECURITY_PROTOCOL; /* Manju updated from 0x00 */
		if (hisi_rpmb.dev_ver >= RPMB_VER_UFS_30)
			cmd[2] = region;
		else
			cmd[2] = 0;
		cmd[3] = 0x01;
		cmd[4] = 0x00;
		cmd[5] = 0x00;
		cmd[6] = (uint8_t)(size >> 24);
		cmd[7] = (uint8_t)((size >> 16) & 0xff);
		cmd[8] = (uint8_t)((size >> 8) & 0xff);
		cmd[9] = (uint8_t)(size & 0xff);
		cmd[10] = 0x00;
		cmd[11] = 0x00;
		break;

	case UFS_OP_SECURITY_PROTOCOL_OUT:
		cmd[0] = UFS_OP_SECURITY_PROTOCOL_OUT;
		cmd[1] = SECURITY_PROTOCOL;
		if (hisi_rpmb.dev_ver >= RPMB_VER_UFS_30)
			cmd[2] = region;
		else
			cmd[2] = 0;
		cmd[3] = 0x01;
		cmd[4] = 0x00;
		cmd[5] = 0x00;
		cmd[6] = (uint8_t)((size >> 24));
		cmd[7] = (uint8_t)((size >> 16) & 0xff);
		cmd[8] = (uint8_t)((size >> 8) & 0xff);
		cmd[9] = (uint8_t)(size & 0xff);
		cmd[10] = 0x00;
		cmd[11] = 0x00;
		break;

	default:
		break;
	}
}

static inline void ufs_rpmb_combine_cmd(struct ufs_blk_ioc_data *data,
	int is_write, unsigned short blocks, unsigned short blksz,
	u8 *sdb_command, u8 *sense_buffer, struct rpmb_frame *transfer_frame,
	uint8_t region)
{
	/*the scsi SG_IO header*/
	struct sg_io_v4 *siv;

	siv = &data->siv;

	siv->guard = 'Q';
	siv->protocol = BSG_PROTOCOL_SCSI;
	siv->subprotocol = BSG_SUB_PROTOCOL_SCSI_CMD;
	siv->response = (uint64_t)sense_buffer;
	siv->max_response_len = MAX_SENSE_BUFFER_LENGTH;
	siv->request_len = SCSI_RPMB_COMMAND_LENGTH;
	if (is_write) {
		ufs_get_cdb_rpmb_command(UFS_OP_SECURITY_PROTOCOL_OUT,
			(uint32_t)data->buf_bytes, sdb_command, region);
		siv->dout_xfer_len = (uint32_t)blocks * blksz;
		siv->dout_xferp = (uint64_t)transfer_frame;
		siv->request = (__u64)sdb_command;
	} else {
		ufs_get_cdb_rpmb_command(UFS_OP_SECURITY_PROTOCOL_IN,
			(uint32_t)data->buf_bytes, sdb_command, region);
		siv->din_xfer_len = (uint32_t)blocks * blksz;
		siv->din_xferp = (uint64_t)transfer_frame;
		siv->request = (__u64)sdb_command;
	}
}

static inline void ufs_rpmb_combine_request(struct ufs_blk_ioc_data *data,
					    struct rpmb_frame *frame,
					    int is_write,
					    unsigned short blocks,
					    unsigned short blksz,
					    u8 *sdb_command,
					    u8 *sense_buffer,
					    struct rpmb_frame *transfer_frame)
{
	/*the scsi SG_IO header*/
	data->buf = (void *)frame;
	data->buf_bytes = (u64)blocks * blksz;
	if(data->buf_bytes > sizeof(struct rpmb_frame) * MAX_RPMB_FRAME){
		pr_err("[%s]:buf_bytes outside transfer_frame\n",__func__);
		return;
	}
	memcpy(transfer_frame, frame, data->buf_bytes); /*[false alarm]: enough reserved memory, 32KB*/
	/*
	 * TODO: Pass region number from caller instead of a global variable.
	 */
	ufs_rpmb_combine_cmd(data, is_write, blocks, blksz, sdb_command,
		sense_buffer, transfer_frame,
		hisi_rpmb.rpmb_request->info.rpmb_region_num);
}

static inline void
ufs_rpmb_basic_request(uint32_t index,
		       struct rpmb_frame *frame,
		       struct ufs_blk_ioc_rpmb_data *rpmb_data,
		       uint16_t block_count,
		       uint8_t *sense_buffer,
		       struct rpmb_frame *transfer_frame)
{
	ufs_rpmb_combine_request(&rpmb_data->data[index], frame,
				 true, block_count, RPMB_BLK_SZ,
				 (u8 *)&rpmb_data->sdb_command[index], sense_buffer,
				 transfer_frame);
}

static inline void
ufs_rpmb_status_request(uint32_t index,
			struct rpmb_frame *frame,
			struct ufs_blk_ioc_rpmb_data *rpmb_data,
			uint16_t block_count,
			uint8_t *sense_buffer,
			struct rpmb_frame *transfer_frame)
{
	ufs_rpmb_combine_request(&rpmb_data->data[index], frame, true,
				 block_count, RPMB_BLK_SZ,
				 (u8 *)&rpmb_data->sdb_command[index], sense_buffer,
				 transfer_frame);
}

static inline void
ufs_rpmb_result_request(uint32_t index,
			struct rpmb_frame *frame,
			struct ufs_blk_ioc_rpmb_data *rpmb_data,
			uint16_t block_count,
			uint8_t *sense_buffer,
			struct rpmb_frame *transfer_frame)
{
	ufs_rpmb_combine_request(&rpmb_data->data[index], frame,
				 false, block_count, RPMB_BLK_SZ,
				 (u8 *)&rpmb_data->sdb_command[index], sense_buffer,
				 transfer_frame);
}

void ufs_rpmb_set_key(struct rpmb_request *shared_rpmb_request,
		      struct ufs_blk_ioc_rpmb_data *rpmb_data,
		      uint8_t *sense_buffer[],
		      struct rpmb_frame *transfer_frame[])
{
	/*according to key frame request, caculate the status request*/
	uint16_t status_frame_request_type = (uint16_t)(((be16_to_cpu(shared_rpmb_request->key_frame.request)) & 0xF000) | RPMB_REQ_STATUS);
	memset(&shared_rpmb_request->status_frame, 0, sizeof(struct rpmb_frame));
	shared_rpmb_request->status_frame.request = cpu_to_be16(status_frame_request_type);
	ufs_rpmb_basic_request(0, &shared_rpmb_request->key_frame, rpmb_data, 0x1, sense_buffer[0], transfer_frame[0]);
	ufs_rpmb_status_request(1, &shared_rpmb_request->status_frame, rpmb_data, 0x1, sense_buffer[1], transfer_frame[1]);
	ufs_rpmb_result_request(2, &shared_rpmb_request->key_frame, rpmb_data, 0x1, sense_buffer[2], transfer_frame[2]);
}

void ufs_rpmb_read_data(struct rpmb_request *shared_rpmb_request,
			struct ufs_blk_ioc_rpmb_data *rpmb_data,
			uint8_t *sense_buffer[],
			struct rpmb_frame *transfer_frame[])
{
	/*get read total blocks*/
	unsigned short blocks_count = (uint16_t)shared_rpmb_request->info.current_rqst.blks;
	ufs_rpmb_basic_request(0, &shared_rpmb_request->frame[0], rpmb_data, 0x1, sense_buffer[0], transfer_frame[0]);
	ufs_rpmb_result_request(1, &shared_rpmb_request->frame[0], rpmb_data, blocks_count, sense_buffer[1], transfer_frame[1]);
}

void ufs_rpmb_get_counter(struct rpmb_request *shared_rpmb_request,
			  struct ufs_blk_ioc_rpmb_data *rpmb_data,
			  uint8_t *sense_buffer[],
			  struct rpmb_frame *transfer_frame[])
{
	ufs_rpmb_basic_request(0, &shared_rpmb_request->frame[0], rpmb_data, 0x1, sense_buffer[0], transfer_frame[0]);
	ufs_rpmb_result_request(1, &shared_rpmb_request->frame[0], rpmb_data, 0x1, sense_buffer[1], transfer_frame[1]);
}

void ufs_rpmb_write_data(struct rpmb_request *shared_rpmb_request,
			 struct ufs_blk_ioc_rpmb_data *rpmb_data,
			 uint8_t *sense_buffer[],
			 struct rpmb_frame *transfer_frame[])
{
	/*get write total blocks*/
	unsigned short blocks_count = (unsigned short)shared_rpmb_request->info.current_rqst.blks;
	/*according to write frame request, caculate the status request*/
	uint16_t status_frame_request_type = (uint16_t)(((be16_to_cpu(shared_rpmb_request->frame[0].request)) & 0xF000) | RPMB_REQ_STATUS);
	memset(&shared_rpmb_request->status_frame, 0, sizeof(struct rpmb_frame));
	shared_rpmb_request->status_frame.request = cpu_to_be16(status_frame_request_type);
	ufs_rpmb_basic_request(0, &shared_rpmb_request->frame[0], rpmb_data, blocks_count, sense_buffer[0], transfer_frame[0]);
	ufs_rpmb_status_request(1, &shared_rpmb_request->status_frame, rpmb_data, 0x1, sense_buffer[1], transfer_frame[1]);
	ufs_rpmb_result_request(2, &shared_rpmb_request->frame[0], rpmb_data, 0x1, sense_buffer[2], transfer_frame[2]);
}
/*
 * we must unlock rpmb_counter_lock for some condition
 * 1. RPMB_STATE_WR_CNT and result failed
 * 2. RPMB_STATE_WR_CNT and result success but RESPONSE not valid
 * 3. RPMB_STATE_WR_DATA and all the emmc blocks have been written
 */
void emmc_rpmb_unlock_counterlock(struct rpmb_request *request,
			 struct rpmb_frame *frame,
			 int32_t result)
{
	if ((request->info.state == RPMB_STATE_WR_CNT && (result || (be16_to_cpu(frame->result) != RPMB_OK ||be16_to_cpu(frame->request) != RPMB_RESP_WCOUNTER))) ||
	     (request->info.state == RPMB_STATE_WR_DATA && (0 == request->info.blks - (request->info.current_rqst.offset + request->info.current_rqst.blks)))) {
		mutex_unlock(&rpmb_counter_lock);/*lint !e455*/
	}
}

/*
 * This warning describes the "lock" exception used in
 * the function, but according to the code The "lock"
 * added to the function is normal and no additional
 * modifications are required
 */
/*lint -e454 -e456*/
static int32_t mmc_rpmb_work(struct rpmb_request *request)
{
	int32_t result;
	struct rpmb_frame *frame = &request->frame[0];
	hisi_rpmb.blkdev = blkdev_get_by_path(EMMC_RPMB_BLOCK_DEVICE_NAME, O_RDWR | O_NDELAY, hisi_rpmb.dev);
	if (IS_ERR(hisi_rpmb.blkdev)) {
		pr_err("[%s]:HISEE open rpmb block failed!\n",__func__);
		return RPMB_ERR_BLKDEV;
	}

	struct mmc_blk_ioc_rpmb_data *rpmb_data =
		kzalloc(sizeof(*rpmb_data), GFP_KERNEL);
	if(NULL == rpmb_data){
		pr_err("[%s]:alloc rpmb_data failed\n",__func__);
		return RPMB_ERR_MEMALOC;
	}

	switch (request->info.state) {
	case RPMB_STATE_IDLE:
		pr_err("[%s]:rpmb maybe issue an error\n",__func__);
		break;
	case RPMB_STATE_KEY:
		frame = &request->key_frame;
		mmc_rpmb_set_key(request, rpmb_data);
		break;
	case RPMB_STATE_RD:
		mmc_rpmb_read_data(request, rpmb_data);
		break;
	case RPMB_STATE_CNT:
		mmc_rpmb_get_counter(request, rpmb_data);
		break;
	case RPMB_STATE_WR_CNT:
		/* TODO add a lock here for counter before write data */
		mutex_lock(&rpmb_counter_lock);
		mmc_rpmb_get_counter(request, rpmb_data);
		break;
	case RPMB_STATE_WR_DATA:
		mmc_rpmb_write_data(request, rpmb_data);
		/* TODO add a unlock for counter after write data */
		break;
	}

	result = mmc_blk_ioctl_rpmb_cmd((enum func_id)request->info.func_id, hisi_rpmb.blkdev,
					rpmb_data);
	emmc_rpmb_unlock_counterlock(request, frame, result);
	if(hisi_rpmb.blkdev)
		blkdev_put(hisi_rpmb.blkdev, O_RDWR | O_NDELAY);
	kfree(rpmb_data);
	return result;
}

static int ufs_rpmb_work(struct rpmb_request *request)
{
	int32_t result;

	struct rpmb_frame *transfer_frame[UFS_IOC_MAX_RPMB_CMD] = {NULL};
	uint8_t *sense_buffer[UFS_IOC_MAX_RPMB_CMD] = {NULL};
	struct rpmb_frame *frame = &request->frame[0];
	/*ufs block ioctl rpmb data, include ufs scsi packet -- sg_io_hdr*/
	struct ufs_blk_ioc_rpmb_data *rpmb_data =
		kzalloc(sizeof(*rpmb_data), GFP_KERNEL);
	if(NULL == rpmb_data){
		pr_err("[%s]:alloc rpmb_data failed\n",__func__);
		return RPMB_ERR_MEMALOC;
	}
	int i;
	for (i = 0; i < UFS_IOC_MAX_RPMB_CMD; i++) {
		transfer_frame[i] = kzalloc(
			sizeof(struct rpmb_frame) * MAX_RPMB_FRAME, GFP_KERNEL);
		if (NULL == transfer_frame[i]) {
			pr_err("[%s]:alloc rpmb transfer_frame failed\n",__func__);
			result = RPMB_ERR_MEMALOC;
			goto free_alloc_buf;
		}

		sense_buffer[i] = kzalloc(
			sizeof(uint8_t) * MAX_SENSE_BUFFER_LENGTH, GFP_KERNEL);
		if (NULL == sense_buffer[i]) {
			pr_err("[%s]:alloc rpmb sense_buffer failed\n",__func__);
			result = RPMB_ERR_MEMALOC;
			goto free_alloc_buf;
		}
	}

	switch (request->info.state) {
	case RPMB_STATE_IDLE:
		pr_err("[%s]:rpmb maybe issue an error\n",__func__);
		break;
	case RPMB_STATE_KEY:
		frame = &request->key_frame;
		ufs_rpmb_set_key(request, rpmb_data, sense_buffer, transfer_frame);
		break;
	case RPMB_STATE_RD:
		ufs_rpmb_read_data(request, rpmb_data, sense_buffer, transfer_frame);
		break;
	case RPMB_STATE_CNT:
		ufs_rpmb_get_counter(request, rpmb_data, sense_buffer, transfer_frame);
		break;
	case RPMB_STATE_WR_CNT:
		/* TODO add a lock here for counter before write data */
		mutex_lock(&rpmb_counter_lock);
		ufs_rpmb_get_counter(request, rpmb_data, sense_buffer, transfer_frame);
		break;
	case RPMB_STATE_WR_DATA:
		ufs_rpmb_write_data(request, rpmb_data, sense_buffer, transfer_frame);
		/* TODO add a unlock for counter after write data */
		break;
	}
	result = ufs_bsg_ioctl_rpmb_cmd(RPMB_FUNC_ID_SE, rpmb_data);

	/*
	 * we must unlock rpmb_counter_lock for some condition
	 * 1. RPMB_STATE_WR_CNT and result failed
	 * 2. RPMB_STATE_WR_CNT and result success but RESPONSE not valid
	 * 3. RPMB_STATE_WR_DATA we always unlock
	 */
	if ((request->info.state == RPMB_STATE_WR_CNT && (result || (be16_to_cpu(frame->result) != RPMB_OK || be16_to_cpu(frame->request) != RPMB_RESP_WCOUNTER))) ||
	     (request->info.state == RPMB_STATE_WR_DATA)) {
		mutex_unlock(&rpmb_counter_lock);
	}

free_alloc_buf:
	for (i = 0; i < UFS_IOC_MAX_RPMB_CMD; i++) {
		if (transfer_frame[i] != NULL)
			kfree(transfer_frame[i]);
		if (sense_buffer[i] != NULL)
			kfree(sense_buffer[i]);
	}
	if(rpmb_data != NULL)
		kfree(rpmb_data);
	return result;
}
/*lint +e454 +e456*/
#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
void hisi_hisee_time_stamp(enum rpmb_state state, unsigned int blks){
	u64 temp_cost_time;
	g_hisee_end_time = hisi_getcurtime();
	temp_cost_time = g_hisee_end_time - g_hisee_start_time;
	if(temp_cost_time > RPMB_TIMEOUT_TIME_IN_KERNEL || g_time_debug)
		pr_err("[%s]rpmb access cost time is more than 800ms, start[%llu], workqueue start time[%llu], ufs start time[%llu],end[%llu], state[%d]!\n", __func__, g_hisee_start_time, g_work_queue_start, g_rpmb_ufs_start_time, g_hisee_end_time, (int32_t)state);
	switch(state){
		case RPMB_STATE_RD:
			if(g_hisee_read_cost_time < temp_cost_time){
				g_hisee_read_cost_time = temp_cost_time;
				g_hisee_read_blks = blks;
			}
			break;
		case RPMB_STATE_CNT:
		case RPMB_STATE_WR_CNT:
			if(g_hisee_counter_cost_time < temp_cost_time){
				g_hisee_counter_cost_time = temp_cost_time;
				g_hisee_counter_blks = blks;
			}
			break;
		case RPMB_STATE_WR_DATA:
			if(g_hisee_write_cost_time < temp_cost_time){
				g_hisee_write_cost_time = temp_cost_time;
				g_hisee_write_blks = blks;
			}
			break;
		default:
			break;
	}
	if(g_time_debug){
		pr_err("[%s]time cost ,operation = [%d]: [%llu]\n", __func__, (int32_t)state, temp_cost_time);
	}
	return;
}
#endif

int wait_hisee_rpmb_request_is_finished(void)
{
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	int i;
	for(i = 0;i < 2000;i++){
		if(0 == request->rpmb_request_status){
			break;
		}
		msleep(10);
	}
	if(i == 2000){
		pr_err("rpmb request get result from device timeout\n");
		return -1;
	}
	return 0;
}

int check_hisee_rpmb_request_is_cleaned(void){
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	int i;
	for(i = 0;i < 1000;i++){
		if(0 != request->rpmb_request_status){
			pr_err("rpmb request is not cleaned in 100-150ms\n");
			return -1;
		}
		usleep_range((unsigned long)10, (unsigned long)15);
	}
	return 0;
}

int32_t hisee_exception_to_reset_rpmb(void)
{
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	request->rpmb_exception_status = RPMB_MARK_EXIST_STATUS;
	int i;
	for (i = 0;i < 3;i++) {
		if (wait_hisee_rpmb_request_is_finished())
			return -1;
		/*if request finished status is not cleaned in 100-150ms,we will retry to wait for hisee request finished*/
		if (check_hisee_rpmb_request_is_cleaned())
			pr_err("Front rpmb request is done, but request is not cleaned\n");
		else
			break;
	}
	request->rpmb_exception_status = 0;
	return 0;
}

static void rpmb_work_routine(void)
{
	int32_t result, ret;
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	/*No key, we do not send read and write request(hynix device not support no key to read more than 4K)*/
	if(rpmb_key_status == KEY_NOT_READY){
		result = RPMB_ERR_KEY;
		pr_err("[%s]:rpmb key is not ready, do not transfer read and write request to device, result = %d\n",__func__,result);
		goto out;
	}
	#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
	g_work_queue_start = hisi_getcurtime();
	#endif
	wake_lock_timeout(&hisi_rpmb.wake_lock, (long)2 * HZ);
	if (BOOT_DEVICE_EMMC == rpmb_support_device)
		result = mmc_rpmb_work(request);
	else
		result = ufs_rpmb_work(request);
	if(result)
		pr_err("[%s]:rpmb request done failed,result = %d\n",__func__,result);
	out:
	#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
	/*mark hisee rpmb request end time*/
	hisi_hisee_time_stamp(request->info.state, request->info.current_rqst.blks);
	#endif
	hisi_rpmb.wake_up_condition = 0;

	ret = atfd_hisi_rpmb_smc((u64)RPMB_SVC_REQUEST_DONE, (u64)(long)result, (u64)0, (u64)0);
	if (ret) {
		pr_err("[%s]:state %d,trans blks %d, rpmb request done from "
		       "bl31 failed, "
		       "ret 0x%x\n",
			__func__, request->info.state,
			request->info.current_rqst.blks, ret);
		print_frame_buf("frame failed", (void *)&request->error_frame, 512, 16);
	}
	/*wake_unlock(&hisi_rpmb.wake_lock);*/
}

/*
 * hisi_rpmb_active - handle rpmb request from ATF
 */
void hisi_rpmb_active(void)
{
	#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
	if(g_time_debug){
		pr_err("[%s]request start,operation = [%d]\n", __func__, (int32_t)hisi_rpmb.rpmb_request->info.state);
	}
	/*mark hisee rpmb request start time*/
	g_hisee_start_time = hisi_getcurtime();
	#endif
	hisi_rpmb.wake_up_condition = 1;
	wake_up(&hisi_rpmb.wait);
}

EXPORT_SYMBOL(hisi_rpmb_active);

#ifdef CONFIG_HISI_DEBUG_FS
int get_rpmb_key_status(void);
u32 get_rpmb_support_key_num(void);
/*
 * debugfs defination start here, debug smc is closed in secure world in user version, so debug can not used in user version
 */
static int get_counter_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_COUNTER, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(get_counter_fops, get_counter_fops_get, NULL, "%llu\n");

static int read_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_READ, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(read_fops, read_fops_get, NULL, "%llu\n");

static int write_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_WRITE, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(write_fops, write_fops_get, NULL, "%llu\n");

static int format_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_FORMAT, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(format_fops, format_fops_get, NULL, "%llu\n");

static int write_capability_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_WRITE_CAPABILITY, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(write_capability_fops, write_capability_fops_get, NULL, "%llu\n");

static int read_capability_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_READ_CAPABILITY, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(read_capability_fops, read_capability_fops_get, NULL, "%llu\n");

static int rpmb_partiton_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_PARTITION, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_partiton_fops, rpmb_partiton_fops_get, NULL, "%llu\n");

static int rpmb_multi_key_set_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_MULTI_KEY, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_multi_key_set_fops, rpmb_multi_key_set_fops_get, NULL, "%llu\n");
static int rpmb_config_view_fops_get(void *data, u64 *val)
{
	atfd_hisi_rpmb_smc((u64)RPMB_SVC_CONFIG_VIEW, (u64)0x0, (u64)0x0, (u64)0x0);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_config_view_set_fops, rpmb_config_view_fops_get, NULL, "%llu\n");

static int rpmb_get_key_status_fops_get(void *data, u64 *val)
{
	int key_status;
	key_status = get_rpmb_key_status();
	pr_err("rpmb_key status:%d\n", key_status);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_get_key_status_fops, rpmb_get_key_status_fops_get, NULL, "%llu\n");

static int rpmb_get_support_key_num_fops_get(void *data, u64 *val)
{
	u32 key_num;
	key_num = get_rpmb_support_key_num();
	pr_err("rpmb_key num is %d\n", key_num);
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(rpmb_get_support_key_num_fops, rpmb_get_support_key_num_fops_get, NULL, "%llu\n");

int hisi_rpmb_debugfs_init(void)
{
	/* debugfs for debug only */
	struct dentry *debugfs_hisi_rpmb;
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	debugfs_hisi_rpmb = debugfs_create_dir("hisi_rpmb", NULL);
	if (IS_ERR(debugfs_hisi_rpmb)){
		pr_err("%s debugfs_create_dir fail\n",__func__);
		return -1;
	}
	if(!debugfs_hisi_rpmb){
		pr_err("%s debugfs_hisi_rpmb is NULL\n",__func__);
		return -1;
	}
	debugfs_create_file("get_counter", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			    NULL, &get_counter_fops);
	debugfs_create_file("read", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL,
			    &read_fops);
	debugfs_create_file("write", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL,
			    &write_fops);
	debugfs_create_file("format", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL,
			    &format_fops);
	debugfs_create_file("write_capability", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL, &write_capability_fops);
	debugfs_create_file("read_capability", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL, &read_capability_fops);
	debugfs_create_file("partition_see", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL, &rpmb_partiton_fops);
	debugfs_create_file("multi_key_set", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL, &rpmb_multi_key_set_fops);
	debugfs_create_file("rpmb_config_view", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL, &rpmb_config_view_set_fops);
	debugfs_create_file("rpmb_key_status", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL, &rpmb_get_key_status_fops);
	debugfs_create_file("rpmb_key_num", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb, NULL, &rpmb_get_support_key_num_fops);

	debugfs_create_u64("partition_size", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			  &request->rpmb_debug.partition_size);
	debugfs_create_u64("result", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			  &request->rpmb_debug.result);
	debugfs_create_u64("test_time", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			  &request->rpmb_debug.test_time);
	debugfs_create_u32("key_id", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			   &request->rpmb_debug.key_num);
	debugfs_create_u32("func_id", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			   &request->rpmb_debug.func_id);
	debugfs_create_u16("start", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			   &request->rpmb_debug.start);
	debugfs_create_u16("block_count", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			   &request->rpmb_debug.block_count);
	debugfs_create_u16("write_value", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			   &request->rpmb_debug.write_value);
	debugfs_create_u16("read_check", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			  &request->rpmb_debug.read_check);
	debugfs_create_u8("capability_times", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			  &request->rpmb_debug.capability_times);
	debugfs_create_u8("multi_region_num", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			  &request->rpmb_debug.multi_region_num);
	debugfs_create_u8("partition_id", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,
			  &request->rpmb_debug.partition_id);
	return 0;
}
EXPORT_SYMBOL(hisi_rpmb_debugfs_init);
#endif
#ifdef CONFIG_HISI_RPMB_TIME_DEBUG

static int clear_rpmb_max_time(void *data, u64 *val)
{
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	g_hisee_read_cost_time = 0;
	g_hisee_write_cost_time = 0;
	g_hisee_counter_cost_time = 0;
	g_hisee_read_blks = 0;
	g_hisee_write_blks = 0;
	g_hisee_write_blks = 0;
	g_os_read_cost_time = 0;
	g_os_write_cost_time = 0;
	g_os_counter_cost_time = 0;
	g_work_queue_start = 0;
	g_rpmb_ufs_start_time = 0;
	request->rpmb_debug.test_hisee_atf_read_time = 0;
	request->rpmb_debug.test_hisee_atf_write_time =0;
	request->rpmb_debug.test_hisee_atf_counter_time = 0;
	return 0;
}

DEFINE_SIMPLE_ATTRIBUTE(clear_rpmb_time_fops, clear_rpmb_max_time, NULL, "%llu\n");
static int read_rpmb_max_time(void *data, u64 *val)
{
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	pr_err("----------------RPMB------------------\n");
	pr_err("hisee max read time in ap:%llu, in atf:%llu, blks:%d\n", g_hisee_read_cost_time, request->rpmb_debug.test_hisee_atf_read_time, g_hisee_read_blks);
	pr_err("hisee max write time in ap:%llu, in atf:%llu, blks:%d\n", g_hisee_write_cost_time, request->rpmb_debug.test_hisee_atf_write_time, g_hisee_write_blks);
	pr_err("hisee max counter time in ap:%llu, in atf:%llu, blks:%d\n", g_hisee_counter_cost_time, request->rpmb_debug.test_hisee_atf_counter_time, g_hisee_write_blks);
	pr_err("os max read time in ap:%llu\n", g_os_read_cost_time);
	pr_err("os max write time in ap:%llu\n", g_os_write_cost_time);
	pr_err("os max counter time in ap:%llu\n", g_os_counter_cost_time);
	return 0;
}
DEFINE_SIMPLE_ATTRIBUTE(read_rpmb_time_fops, read_rpmb_max_time, NULL, "%llu\n");

static void hisi_rpmb_time_stamp_debugfs_init(void){
	/*create  */
	struct dentry *debugfs_hisi_rpmb;
	debugfs_hisi_rpmb = debugfs_create_dir("hisi_rpmb_time", NULL);
	if(!debugfs_hisi_rpmb){
		pr_err("%s debugfs_hisi_rpmb is NULL\n",__func__);
		return;
	}
	debugfs_create_file("read_rpmb_time", S_IRUSR, debugfs_hisi_rpmb, NULL, &read_rpmb_time_fops);
	debugfs_create_file("clear_rpmb_time", S_IRUSR, debugfs_hisi_rpmb, NULL, &clear_rpmb_time_fops);
	/*only for test print*/
	debugfs_create_u16("print_enable", S_IRUSR | S_IWUSR, debugfs_hisi_rpmb,&g_time_debug);
}
#endif

static ssize_t mmc_rpmb_key_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf,
				  size_t count)
{
	int ret = 0;
	int i;
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	struct rpmb_frame *frame = &request->key_frame;
	struct mmc_blk_ioc_rpmb_data *rpmb_data =
		kzalloc(sizeof(*rpmb_data), GFP_KERNEL);
	if (!rpmb_data) {
		dev_err(dev, "alloc rpmb_data failed\n");
		return -1;
	}

	/* before set key must init the request */
	request->key_frame_status = KEY_NOT_READY;
	memset(frame, 0, sizeof(struct rpmb_frame));

#ifdef TEST_AT_SET_KEY_PRINT_DEBUG
	print_frame_buf("rpmb_frame_set_key rpmb frame", (void *)frame, 512, 16);
#endif

	if (strncmp(buf, "set_key", count)) {
		dev_err(dev, "invalid key set command\n");
		kfree(rpmb_data);
		return -1;
	}

	hisi_rpmb.blkdev = blkdev_get_by_path(EMMC_RPMB_BLOCK_DEVICE_NAME,
					      O_RDWR | O_NDELAY, hisi_rpmb.dev);
	if (IS_ERR(hisi_rpmb.blkdev)) {
		dev_err(dev, "blkdev get mmcblk0rpmb failed\n");
		goto alloc_free;
	}

	/* get key from bl31 */
	ret = atfd_hisi_rpmb_smc((u64)RPMB_SVC_SET_KEY, (u64)0x0, (u64)0x0, (u64)0x0);
	if (ret) {
		dev_err(dev, "get rpmb key frame failed, ret = %d\n", ret);
		goto alloc_free;
	}

	for (i = 0; i < WAIT_KEY_FRAME_TIMES; i++) {
		if (request->key_frame_status == KEY_READY)
			break;
		mdelay(5);
	}
	if (i == WAIT_KEY_FRAME_TIMES) {
		dev_err(dev, "wait for key frame ready timeout\n");
		pr_err("wait for key frame ready timeout\n");
		goto alloc_free;
	}

#ifdef TEST_AT_SET_KEY_PRINT_DEBUG
	print_frame_buf("rpmb_frame_set_key rpmb frame", (void *)frame, 512, 16);
#endif

	mmc_rpmb_set_key(request, rpmb_data);
	ret = mmc_blk_ioctl_rpmb_cmd((enum func_id)request->info.func_id, hisi_rpmb.blkdev,
				     rpmb_data);

	blkdev_put(hisi_rpmb.blkdev, O_RDWR | O_NDELAY);
alloc_free:
	if(rpmb_data != NULL)
		kfree(rpmb_data);
	if (ret || (be16_to_cpu(frame->result) != RPMB_OK ||
		    be16_to_cpu(frame->request) != RPMB_RESP_KEY)){
		pr_err("set emmc rpmb single key failed\n");
		print_frame_buf("error frame", (void *)frame, 512, 16);
		return -1;
	}
	return (ssize_t)count;
}

static ssize_t ufs_rpmb_key_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf,
				  size_t count)
{
	int i, ret = 0;
	struct rpmb_request *request = hisi_rpmb.rpmb_request;
	struct rpmb_frame *frame = &request->key_frame;

	struct rpmb_frame *transfer_frame[UFS_IOC_MAX_RPMB_CMD] = {NULL};
	uint8_t *sense_buffer[UFS_IOC_MAX_RPMB_CMD] = {NULL};
	struct ufs_blk_ioc_rpmb_data *rpmb_data =
		kzalloc(sizeof(*rpmb_data), GFP_KERNEL);
	if (!rpmb_data) {
		dev_err(dev, "alloc rpmb_data failed\n");
		return -1;
	}
	for (i = 0; i < UFS_IOC_MAX_RPMB_CMD; i++) {
		transfer_frame[i] = kzalloc(
			sizeof(struct rpmb_frame) * MAX_RPMB_FRAME, GFP_KERNEL);
		if (NULL == transfer_frame[i]) {
			dev_err(dev, "alloc rpmb transfer_frame failed\n");
			goto alloc_free;
		}

		sense_buffer[i] = kzalloc(
			sizeof(uint8_t) * MAX_SENSE_BUFFER_LENGTH, GFP_KERNEL);
		if (NULL == sense_buffer[i]) {
			dev_err(dev, "alloc rpmb sense_buffer failed\n");
			goto alloc_free;
		}
	}

	/* before set key must init the request */
	request->key_frame_status = KEY_NOT_READY;
	memset(frame, 0, sizeof(struct rpmb_frame));

#ifdef TEST_AT_SET_KEY_PRINT_DEBUG
	print_frame_buf("rpmb_frame_set_key rpmb frame", (void *)frame, 512, 16);
#endif
	ret = strncmp(buf, "set_key", count);
	if (ret) {
		dev_err(dev, "invalid key set command\n");
		goto alloc_free;
	}

	/* get key from bl31 */
	ret = atfd_hisi_rpmb_smc((u64)RPMB_SVC_SET_KEY, (u64)0x0, (u64)0x0, (u64)0x0);
	if (ret) {
		dev_err(dev, "get rpmb key frame failed, ret = %d\n", ret);
		goto alloc_free;
	}

	for (i = 0; i < WAIT_KEY_FRAME_TIMES; i++) {
		if (request->key_frame_status == KEY_READY)
			break;
		mdelay(5);
	}
	if (i == WAIT_KEY_FRAME_TIMES) {
		dev_err(dev, "wait for key frame ready timeout\n");
		pr_err("wait for key frame ready timeout\n");
		goto alloc_free;
	}

#ifdef TEST_AT_SET_KEY_PRINT_DEBUG
	print_frame_buf("rpmb_frame_set_key rpmb frame", (void *)frame, 512, 16);
#endif

	ufs_rpmb_set_key(request, rpmb_data, sense_buffer, transfer_frame);
	ret = ufs_bsg_ioctl_rpmb_cmd(RPMB_FUNC_ID_SE, rpmb_data);

alloc_free:
	if(rpmb_data != NULL)
		kfree(rpmb_data);
	for (i = 0; i < UFS_IOC_MAX_RPMB_CMD; i++) {
		if (transfer_frame[i] != NULL)
			kfree(transfer_frame[i]);
		if (sense_buffer[i] != NULL)
			kfree(sense_buffer[i]);
	}
	if (ret || (be16_to_cpu(frame->result) != RPMB_OK ||
		    be16_to_cpu(frame->request) != RPMB_RESP_KEY)){
		pr_err("set ufs emmc single key failed\n");
		print_frame_buf("error frame", (void *)frame, 512, 16);
		return -1;
	}
	return (ssize_t)count;
}

/*check the rpmb key in emmc is OK*/
static int32_t mmc_rpmb_key_status(void){
	int ret;
	struct rpmb_request *request;
	struct rpmb_frame *frame;
	struct block_device *blkdev = NULL;
	request = kzalloc(sizeof(struct rpmb_request), GFP_KERNEL);
	if(!request){
		pr_err("[%s]:alloc rpmb_request failed\n",__func__);
		return RPMB_ERR_MEMALOC;
	}
	frame = &request->frame[0];
	struct mmc_blk_ioc_rpmb_data *rpmb_data =
		kzalloc(sizeof(*rpmb_data), GFP_KERNEL);

	if (!rpmb_data) {
		pr_err("[%s]:alloc rpmb_data failed\n",__func__);
		ret =  RPMB_ERR_MEMALOC;
		goto alloc_free;
	}

	blkdev = blkdev_get_by_path(EMMC_RPMB_BLOCK_DEVICE_NAME,
					      O_RDWR | O_NDELAY, hisi_rpmb.dev);
	if (IS_ERR(blkdev)) {
		pr_err("[%s]:rpmb device get failed\n",__func__);
		ret = RPMB_ERR_BLKDEV;
		goto alloc_free;
	}

	memset(frame, 0, sizeof(struct rpmb_frame));
	frame->request = cpu_to_be16(RPMB_REQ_WCOUNTER);
	mmc_rpmb_get_counter(request, rpmb_data);
	ret = mmc_blk_ioctl_rpmb_cmd(RPMB_FUNC_ID_SE, blkdev, rpmb_data);
	if (ret) {
		pr_err("[%s]:can not get rpmb key status\n",__func__);
		goto alloc_free;
	}
	if(be16_to_cpu(frame->result) == RPMB_ERR_KEY && be16_to_cpu(frame->request) == RPMB_RESP_WCOUNTER){
		pr_err("[%s]:RPMB KEY is not set\n",__func__);
		ret = RPMB_ERR_SET_KEY;
	} else if (be16_to_cpu(frame->result) != RPMB_OK || be16_to_cpu(frame->request) != RPMB_RESP_WCOUNTER) {
		pr_err("[%s]:get write counter failed\n",__func__);
		print_frame_buf("error frame", (void *)frame, 512, 16);
		ret = RPMB_ERR_GET_COUNT;
	}else
		ret = RPMB_OK;
alloc_free:
	if(blkdev)
		blkdev_put(blkdev, O_RDWR | O_NDELAY);
	if(rpmb_data != NULL)
		kfree(rpmb_data);
	if(request != NULL)
		kfree(request);
	return ret;
}

/*check the rpmb key in ufs is OK*/
static int32_t ufs_rpmb_key_status(void){
	int ret;
	int i;
	struct rpmb_request *request;
	struct rpmb_frame *frame;
	struct rpmb_frame *transfer_frame[UFS_IOC_MAX_RPMB_CMD] = {NULL};
	uint8_t *sense_buffer[UFS_IOC_MAX_RPMB_CMD] = {NULL};
	request = kzalloc(sizeof(struct rpmb_request), GFP_KERNEL);
	if(!request){
		pr_err("[%s]:alloc rpmb_request failed\n",__func__);
		return RPMB_ERR_MEMALOC;
	}
	frame = &request->frame[0];
	struct ufs_blk_ioc_rpmb_data *rpmb_data = kzalloc(sizeof(*rpmb_data), GFP_KERNEL);
	if (!rpmb_data) {
		pr_err("[%s]:alloc rpmb_data failed\n",__func__);
		ret = RPMB_ERR_MEMALOC;
		goto alloc_free;
	}
	/*get counter ,we only need 1 frame size alloc ufs transfer_frame and sense_buff*/
	for (i = 0; i < UFS_IOC_MAX_RPMB_CMD; i++) {
		transfer_frame[i] = kzalloc(
			sizeof(struct rpmb_frame), GFP_KERNEL);
		if (NULL == transfer_frame[i]) {
			pr_err("[%s]:alloc rpmb transfer_frame failed\n",__func__);
			ret = RPMB_ERR_MEMALOC;
			goto alloc_free;
		}

		sense_buffer[i] = kzalloc(
			sizeof(uint8_t) * MAX_SENSE_BUFFER_LENGTH, GFP_KERNEL);
		if (NULL == sense_buffer[i]) {
			pr_err("[%s]:alloc rpmb sense_buffer failed\n",__func__);
			ret = RPMB_ERR_MEMALOC;
			goto alloc_free;
		}
	}

	memset(frame, 0, sizeof(struct rpmb_frame));
	frame->request = cpu_to_be16(RPMB_REQ_WCOUNTER);
	ufs_rpmb_get_counter(request, rpmb_data, sense_buffer,
			     transfer_frame);
	ret = ufs_bsg_ioctl_rpmb_cmd(RPMB_FUNC_ID_SE, rpmb_data);
	if (ret) {
		pr_err("[%s]:can not get rpmb key status\n",__func__);
		goto alloc_free;
	}
	if(be16_to_cpu(frame->result) == RPMB_ERR_KEY && be16_to_cpu(frame->request) == RPMB_RESP_WCOUNTER){
		pr_err("[%s]:RPMB KEY is not set\n",__func__);
		ret = RPMB_ERR_KEY;
	}else if (be16_to_cpu(frame->result) != RPMB_OK || be16_to_cpu(frame->request) != RPMB_RESP_WCOUNTER) {
		pr_err("[%s]:get write counter failed\n",__func__);
		print_frame_buf("error frame", (void *)frame, 512, 16);
		ret = RPMB_ERR_GET_COUNT;
	}else
		ret = RPMB_OK;
alloc_free:
	for (i = 0; i < UFS_IOC_MAX_RPMB_CMD; i++) {
		if (transfer_frame[i] != NULL)
			kfree(transfer_frame[i]);
		if (sense_buffer[i] != NULL)
			kfree(sense_buffer[i]);
	}
	if(rpmb_data != NULL)
		kfree(rpmb_data);
	if(request != NULL)
		kfree(request);
	return ret;
}

static void rpmb_key_status_check(void){
	int ret;
	if (BOOT_DEVICE_EMMC == rpmb_support_device)
		ret = mmc_rpmb_key_status();
	else
		ret = ufs_rpmb_key_status();
	if(!ret)
		rpmb_key_status = KEY_READY;
	else
		rpmb_key_status = KEY_NOT_READY;
}

u32 get_rpmb_support_key_num(void){
	u32 key_num = 0;
	int i;
	for (i = 0;i < MAX_RPMB_REGION_NUM;i++) {
		if (((rpmb_device_info.rpmb_region_enable >> i) & 0x1))
			key_num++;
	}
	if (0 == key_num) {
		pr_err("failed:get_rpmb_support_key_num is zero, rpmb is not support\n");
	}
	return key_num;
}

int get_rpmb_key_status(void){
	int result;
	if (1 == get_rpmb_support_key_num())
		return rpmb_key_status;
	else {
		result = atfd_hisi_rpmb_smc((u64)RPMB_SVC_MULTI_KEY_STATUS, (u64)0x0, (u64)0x0, (u64)0x0);
		if (KEY_NOT_READY == result || KEY_READY == result)
			return result;
		else {
			pr_err("get_rpmb_key_status failed, result = %d\n", result);
			return KEY_REQ_FAILED;
		}
	}
}

int get_rpmb_init_status_quick(void)
{
	return rpmb_drivers_init_status;
}

static ssize_t hisi_rpmb_key_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf,
				   size_t count)
{
	ssize_t ret;
	if (BOOT_DEVICE_EMMC == rpmb_support_device)
		ret = mmc_rpmb_key_store(dev, attr, buf, count);
	else
		ret = ufs_rpmb_key_store(dev, attr, buf, count);
	if(ret == (ssize_t)count){
		rpmb_key_status = KEY_READY;
	}
	return ret;
}
/*according to rpmb_key_status to check the key is ready*/
static ssize_t
hisi_rpmb_key_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	rpmb_key_status_check();
	if(KEY_READY == rpmb_key_status)
		strncpy(buf, "true", sizeof("true"));
	else
		strncpy(buf, "false", sizeof("false"));
	return (ssize_t)strlen(buf);
}

#define WAIT_INIT_TIMES 3000
int get_rpmb_init_status(void)
{
	int i;
	int32_t ret = 0;
	/*lint -e501*/
	mm_segment_t oldfs = get_fs();
	set_fs(get_ds());
	/*lint +e501*/
	for(i = 0;i < WAIT_INIT_TIMES;i++){
		if(BOOT_DEVICE_EMMC == rpmb_support_device)
			ret = (int32_t)sys_access(EMMC_RPMB_BLOCK_DEVICE_NAME, 0);
		else
			ret = (int32_t)sys_access(UFS_RPMB_BLOCK_DEVICE_NAME, 0);
		if(!ret && rpmb_device_init_status)
			break;
		usleep_range((unsigned long)3000, (unsigned long)5000);
	}
	if(i == WAIT_INIT_TIMES){
		pr_err("wait for device init timeout!\n");
		rpmb_drivers_init_status = RPMB_DRIVER_IS_NOT_READY;
	}else{
		rpmb_drivers_init_status = RPMB_DRIVER_IS_READY;
	}
	set_fs(oldfs);
	/*when device is OK and rpmb key is not ready, we will check the key status*/
	if(rpmb_drivers_init_status == RPMB_DRIVER_IS_READY && rpmb_key_status == KEY_NOT_READY){
		rpmb_key_status_check();
		pr_err("[%s]:rpmb key status{%d}\n",__func__, rpmb_key_status);
	}
	return rpmb_drivers_init_status;
}

static DEVICE_ATTR(rpmb_key,
		   (S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP),
		   hisi_rpmb_key_show,
		   hisi_rpmb_key_store);

#define FRAME_BLOCK_COUNT 506
static inline void hisi_ufs_read_frame_recombine(struct storage_blk_ioc_rpmb_data *storage_data)
{
	unsigned int block_count;
	/*
	The blocks is one request cmd that needs to transfer(read or write) the frames, this is set by SECURE OS.
	Because read rpmb need two request cmd and the second request includes the block_count that SECURE OS really needs to read,
	the first request's  frame's block_count in ufs rpmb needs to be set before sending cmd to device.
	*/
	block_count = storage_data->data[1].blocks;
	/*num of rpmb blks MSB*/
	storage_data->data[0].buf[FRAME_BLOCK_COUNT] = (uint8_t)((block_count & 0xFF00) >>8);
	 /*num of rpmb blks MSB*/
	storage_data->data[0].buf[FRAME_BLOCK_COUNT + 1] = (uint8_t)(block_count & 0xFF);
	return;
}

int hisi_mmc_rpmb_ioctl_cmd(enum func_id id, enum rpmb_op_type operation, struct storage_blk_ioc_rpmb_data *storage_data){
	struct block_device *bdev;
	int ret;
	int i;
	bdev = blkdev_get_by_path(EMMC_RPMB_BLOCK_DEVICE_NAME, FMODE_READ | FMODE_WRITE | FMODE_NDELAY, 0);
	if (IS_ERR(bdev)) {
		pr_err("[%s]:Secure OS open rpmb block failed!\n",__func__);
		return RPMB_ERR_BLKDEV;
	}

	struct mmc_blk_ioc_rpmb_data *rpmb_data = kzalloc(sizeof(*rpmb_data), GFP_KERNEL);
	if(NULL == rpmb_data){
		pr_err("[%s]:alloc rpmb_data failed\n",__func__);
		return RPMB_ERR_MEMALOC;
	}

	for(i = 0;i < STORAGE_IOC_MAX_RPMB_CMD; i++){
		rpmb_data->data[i].buf = storage_data->data[i].buf;
		rpmb_data->data[i].buf_bytes = storage_data->data[i].buf_bytes;
	}
	switch (operation) {
		case RPMB_OP_RD:
			mmc_rpmb_combine_cmd(&rpmb_data->data[0], true, (unsigned short)storage_data->data[0].blocks, RPMB_BLK_SZ, false);
			mmc_rpmb_combine_cmd(&rpmb_data->data[1], false, (unsigned short)storage_data->data[1].blocks, RPMB_BLK_SZ, false);
			break;
		case RPMB_OP_WR_CNT:
			mmc_rpmb_combine_cmd(&rpmb_data->data[0], true, (unsigned short)storage_data->data[0].blocks, RPMB_BLK_SZ, false);
			mmc_rpmb_combine_cmd(&rpmb_data->data[1], false, (unsigned short)storage_data->data[1].blocks, RPMB_BLK_SZ, false);
		    break;
		case RPMB_OP_WR_DATA:
			mmc_rpmb_combine_cmd(&rpmb_data->data[0], true, (unsigned short)storage_data->data[0].blocks, RPMB_BLK_SZ, true);
			mmc_rpmb_combine_cmd(&rpmb_data->data[1], true, (unsigned short)storage_data->data[1].blocks, RPMB_BLK_SZ, false);
			mmc_rpmb_combine_cmd(&rpmb_data->data[2], false, (unsigned short)storage_data->data[2].blocks, RPMB_BLK_SZ, false);
			break;
	}
	ret = mmc_blk_ioctl_rpmb_cmd(id, bdev, rpmb_data);

	if(hisi_rpmb.blkdev)
		blkdev_put(hisi_rpmb.blkdev, O_RDWR | O_NDELAY);
	kfree(rpmb_data);
	return ret;
}

int hisi_ufs_rpmb_ioctl_cmd(enum func_id id, enum rpmb_op_type operation, struct storage_blk_ioc_rpmb_data *storage_data){
	int ret;
	int i;
	struct ufs_blk_ioc_rpmb_data *rpmb_data = kzalloc(sizeof(*rpmb_data), GFP_KERNEL);
	if(NULL == rpmb_data){
		pr_err("[%s]:alloc rpmb_data failed\n",__func__);
		return RPMB_ERR_MEMALOC;
	}
	uint8_t *sense_buffer[UFS_IOC_MAX_RPMB_CMD] = {NULL};

	for(i=0;i < UFS_IOC_MAX_RPMB_CMD;i++){
		sense_buffer[i] = kzalloc(sizeof(uint8_t) * MAX_SENSE_BUFFER_LENGTH, GFP_KERNEL);
		if(NULL == sense_buffer[i]){
			pr_err("[%s]:alloc sense_buffer failed\n",__func__);
			ret = RPMB_ERR_MEMALOC;
			goto free_alloc_buf;
		}
	}

	if(RPMB_OP_RD == operation)
		hisi_ufs_read_frame_recombine(storage_data);

	for(i = 0;i < STORAGE_IOC_MAX_RPMB_CMD; i++){
		rpmb_data->data[i].buf_bytes = storage_data->data[i].buf_bytes;
		/*when Secure OS write multi blocks in HYNIX rpmb, it will timeout, memcpy to avoid the error*/
		if(UFS_VENDOR_HYNIX == get_bootdevice_manfid()){
			rpmb_data->data[i].buf =kzalloc(rpmb_data->data[i].buf_bytes, GFP_KERNEL);
			if(NULL == rpmb_data->data[i].buf){
				pr_err("[%s]:alloc rpmb_data buf failed\n",__func__);
				ret = RPMB_ERR_MEMALOC;
				goto free_alloc_buf;
			}
			memcpy(rpmb_data->data[i].buf, storage_data->data[i].buf,rpmb_data->data[i].buf_bytes);
		}
		else
			rpmb_data->data[i].buf =  storage_data->data[i].buf;
	}

	switch (operation) {
	case RPMB_OP_RD:
		ufs_rpmb_combine_cmd(&rpmb_data->data[0], true,
			(unsigned short)storage_data->data[0].blocks,
			RPMB_BLK_SZ, (u8 *)&rpmb_data->sdb_command[0],
			sense_buffer[0],
			(struct rpmb_frame *)rpmb_data->data[0].buf, 0);
		ufs_rpmb_combine_cmd(&rpmb_data->data[1], false,
			(unsigned short)storage_data->data[1].blocks,
			RPMB_BLK_SZ, (u8 *)&rpmb_data->sdb_command[1],
			sense_buffer[1],
			(struct rpmb_frame *)rpmb_data->data[1].buf, 0);
		break;
	case RPMB_OP_WR_CNT:
		ufs_rpmb_combine_cmd(&rpmb_data->data[0], true,
			(unsigned short)storage_data->data[0].blocks,
			RPMB_BLK_SZ, (u8 *)&rpmb_data->sdb_command[0],
			sense_buffer[0],
			(struct rpmb_frame *)rpmb_data->data[0].buf, 0);
		ufs_rpmb_combine_cmd(&rpmb_data->data[1], false,
			(unsigned short)storage_data->data[1].blocks,
			RPMB_BLK_SZ, (u8 *)&rpmb_data->sdb_command[1],
			sense_buffer[1],
			(struct rpmb_frame *)rpmb_data->data[1].buf, 0);
		break;
	case RPMB_OP_WR_DATA:
		ufs_rpmb_combine_cmd(&rpmb_data->data[0], true,
			(unsigned short)storage_data->data[0].blocks,
			RPMB_BLK_SZ, (u8 *)&rpmb_data->sdb_command[0],
			sense_buffer[0],
			(struct rpmb_frame *)rpmb_data->data[0].buf, 0);
		ufs_rpmb_combine_cmd(&rpmb_data->data[1], true,
			(unsigned short)storage_data->data[1].blocks,
			RPMB_BLK_SZ, (u8 *)&rpmb_data->sdb_command[1],
			sense_buffer[1],
			(struct rpmb_frame *)rpmb_data->data[1].buf, 0);
		ufs_rpmb_combine_cmd(&rpmb_data->data[2], false,
			(unsigned short)storage_data->data[2].blocks,
			RPMB_BLK_SZ, (u8 *)&rpmb_data->sdb_command[2],
			sense_buffer[2],
			(struct rpmb_frame *)rpmb_data->data[2].buf, 0);
		break;
	}

	ret = ufs_bsg_ioctl_rpmb_cmd(id, rpmb_data);

	/*when Secure OS write multi blocks in HYNIX rpmb, it will timeout, memcpy to avoid the error*/
	if(UFS_VENDOR_HYNIX == get_bootdevice_manfid()){
		for(i = 0;i < STORAGE_IOC_MAX_RPMB_CMD; i++){
			memcpy(storage_data->data[i].buf, rpmb_data->data[i].buf,rpmb_data->data[i].buf_bytes);
		}
	}

free_alloc_buf:
	if(UFS_VENDOR_HYNIX == get_bootdevice_manfid()){
		for(i=0;i < UFS_IOC_MAX_RPMB_CMD;i++){
			if(rpmb_data->data[i].buf != NULL)
				kfree(rpmb_data->data[i].buf );
		}
	}
	for(i=0;i < UFS_IOC_MAX_RPMB_CMD;i++){
		if(sense_buffer[i] != NULL)
			kfree(sense_buffer[i]);
	}
	if(rpmb_data != NULL)
		kfree(rpmb_data);
	return ret;
}
#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
void hisi_os_time_stamp(enum rpmb_op_type operation){
	u64 temp_cost_time;
	g_os_end_time = hisi_getcurtime();
	temp_cost_time = g_os_end_time - g_os_start_time;
	switch(operation){
		case RPMB_OP_RD:
			if(g_os_read_cost_time < temp_cost_time)
				g_os_read_cost_time = temp_cost_time;
			break;
		case RPMB_OP_WR_CNT:
			if(g_os_counter_cost_time < temp_cost_time)
				g_os_counter_cost_time = temp_cost_time;
			break;
		case RPMB_OP_WR_DATA:
			if(g_os_write_cost_time < temp_cost_time)
				g_os_write_cost_time = temp_cost_time;
			break;
		default:
			break;
	}
	if(g_time_debug){
		pr_err("[%s]time cost ,operation = [%d]: [%llu]\n", __func__, (int32_t)operation, temp_cost_time);
	}
	return;
}
#endif

int hisi_rpmb_ioctl_cmd(enum func_id id, enum rpmb_op_type operation, struct storage_blk_ioc_rpmb_data *storage_data){
	int ret;
	if(RPMB_DRIVER_IS_NOT_READY == get_rpmb_init_status()){
		pr_err("[%s]:rpmb init is not ready\n",__func__);
		ret = RPMB_ERR_INIT;
		goto out;
	}
	#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
	if(g_time_debug){
		pr_err("[%s]request start,operation = [%d]\n", __func__, (int32_t)operation);
	}
	/*mark TEE rpmb request start time*/
	g_os_start_time = hisi_getcurtime();
	#endif
	/*No key, we do not send read and write request(hynix device not support no key to read more than 4K)*/
	if((rpmb_key_status == KEY_NOT_READY) && (operation!= RPMB_OP_WR_CNT)){
		ret = RPMB_ERR_KEY;
		pr_err("[%s]:rpmb key is not ready, do not transfer read and write request to device, result = %d\n",__func__,ret);
		goto out;
	}
	if(BOOT_DEVICE_EMMC == rpmb_support_device){
		ret = hisi_mmc_rpmb_ioctl_cmd(id, operation, storage_data);
	}
	else{
		ret = hisi_ufs_rpmb_ioctl_cmd(id, operation, storage_data);
	}
out:
	#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
	/*mark TEE rpmb request end time*/
	hisi_os_time_stamp(operation);
	#endif
	return ret;
}

static struct mmc_blk_ioc_rpmb_data *
mmc_blk_ioctl_rpmb_copy_data(struct mmc_blk_ioc_rpmb_data *rdata)
{
	struct mmc_blk_ioc_rpmb_data *idata;
	long err;
	int i;
	idata = kzalloc(sizeof(*idata), GFP_KERNEL);
	if (!idata) {
		err = -ENOMEM;
		return ERR_PTR(err);
	}

	for (i = 0; i < MMC_IOC_MAX_RPMB_CMD; i++) {
		idata->data[i].buf_bytes = rdata->data[i].buf_bytes;
		idata->data[i].buf =
			kzalloc(idata->data[i].buf_bytes, GFP_KERNEL);
		if (!(idata->data[i].buf)) {
			err = -ENOMEM;
			goto alloc_failed;
		}
		memcpy(&idata->data[i].ic, &rdata->data[i].ic,
		       sizeof(struct mmc_ioc_cmd));
		memcpy(idata->data[i].buf, rdata->data[i].buf,
		       idata->data[i].buf_bytes);
	}

	return idata;

alloc_failed:
	for (i = 0; i < MMC_IOC_MAX_RPMB_CMD; i++) {
		if(idata->data[i].buf != NULL)
			kfree(idata->data[i].buf);
	}
	if(idata != NULL)
		kfree(idata);
	return ERR_PTR(err);
}

/*output api ++*/
struct mmc_card *get_mmc_card(struct block_device *bdev)
{
	struct mmc_blk_data *md;
	struct mmc_card *card;

	md = mmc_blk_get(bdev->bd_disk);
	if (!md) {
		return NULL;
	}

	card = md->queue.card;
	if (IS_ERR(card)) {
		return NULL;
	}

	return card;
}
EXPORT_SYMBOL(get_mmc_card);

/*This function is responsible for handling RPMB command and is the interface
 *with the eMMC driver.
 *It is used by BL31 and SecureOS.So when modify the fuction please check it
 *with SecureOS.
 *During DMA 64bit development, we modify it using the method of memory copy.
 *idata:the parameter consist of  two command at least and three commd at most,
 *so when copy retuning
 *      data, please confirm copy all the retuning data not include write
 *command.
 */
 /*lint -e429 -e593*/
#if CONFIG_HISI_MMC_SECURE_RPMB
int mmc_blk_ioctl_rpmb_cmd(enum func_id id,
			   struct block_device *bdev,
			   struct mmc_blk_ioc_rpmb_data *rdata)
{
	struct mmc_blk_data *md;
	struct mmc_card *card;
	struct mmc_command cmd = {0};
	struct mmc_data data = {0};
	struct mmc_request mrq = {NULL};
	struct scatterlist sg;
	struct mmc_blk_ioc_rpmb_data *idata;
	int err = 0, i = 0;
	u32 status = 0;
	bool switch_err = false;
	int switch_retry = 3;

	md = mmc_blk_get(bdev->bd_disk);
	/* make sure this is a rpmb partition */
	if ((!md) || (!((unsigned int)md->area_type & (unsigned int)MMC_BLK_DATA_AREA_RPMB))) {
		err = -EINVAL;
		return err;
	}

	idata = mmc_blk_ioctl_rpmb_copy_data(rdata);
	if (IS_ERR(idata)) {
		err = (int)PTR_ERR(idata);
		goto cmd_done;
	}

	card = md->queue.card;
	if (IS_ERR(card)) {
		err = (int)PTR_ERR(card);
		goto idata_free;
	}

	mmc_get_card(card);
	/*mmc_claim_host(card->host);*/

retry:
	err = mmc_blk_part_switch(card, md);
	if (err) {
		switch_err = true;
		goto cmd_rel_host;
	}

	for (i = 0; i < MMC_IOC_MAX_RPMB_CMD; i++) {
		struct mmc_blk_ioc_data *curr_data;
		struct mmc_ioc_cmd *curr_cmd;

		curr_data = &idata->data[i];
		curr_cmd = &curr_data->ic;
		if (!curr_cmd->opcode)
			break;

		cmd.opcode = curr_cmd->opcode;
		cmd.arg = curr_cmd->arg;
		cmd.flags = curr_cmd->flags;

		if (curr_data->buf_bytes) {
			data.sg = &sg;
			data.sg_len = 1;
			data.blksz = curr_cmd->blksz;
			data.blocks = curr_cmd->blocks;

			sg_init_one(data.sg, curr_data->buf,
				    (unsigned int)curr_data->buf_bytes);

			if (curr_cmd->write_flag)
				data.flags = MMC_DATA_WRITE;
			else
				data.flags = MMC_DATA_READ;

			/* data.flags must already be set before doing this. */
			mmc_set_data_timeout(&data, card);

			/*
			 * Allow overriding the timeout_ns for empirical tuning.
			 */
			if (curr_cmd->data_timeout_ns)
				data.timeout_ns = curr_cmd->data_timeout_ns;

			mrq.data = &data;
		}

		mrq.cmd = &cmd;

		err = mmc_set_blockcount(card, data.blocks,
					 (bool)((unsigned int)curr_cmd->write_flag &((unsigned int)1 << 31)));
		if (err)
			goto cmd_rel_host;

		mmc_wait_for_req(card->host, &mrq);

		if (cmd.error) {
			dev_err(mmc_dev(card->host), "%s: cmd error %d\n",
				__func__, cmd.error);
			err = (int)cmd.error;
			goto cmd_rel_host;
		}
		if (data.error) {
			dev_err(mmc_dev(card->host), "%s: data error %d\n",
				__func__, data.error);
			err = (int)data.error;
			goto cmd_rel_host;
		}

		memcpy(curr_cmd->response, cmd.resp, sizeof(cmd.resp));

		if (!curr_cmd->write_flag) {
			memcpy(rdata->data[i].buf, curr_data->buf,
			       curr_data->buf_bytes);
		}

		/*
		 * Ensure RPMB command has completed by polling CMD13
		 * "Send Status".
		 */
		err = ioctl_rpmb_card_status_poll(card, &status, 5);
		if (err)
			dev_err(mmc_dev(card->host),
				"%s: Card Status=0x%08X, error %d\n", __func__,
				status, err);
	}

cmd_rel_host:
	if (err == -ENOMSG) {
		if (!mmc_blk_reset(md, card->host, 0)) {
			if (switch_err && switch_retry--) {
				switch_err = false;
				goto retry;
			}
		}
	}

	mmc_put_card(card);
/*mmc_release_host(card->host);*/

idata_free:
	for (i = 0; i < MMC_IOC_MAX_RPMB_CMD; i++){
		if(idata->data[i].buf != NULL)
			kfree(idata->data[i].buf);
	}
	if(idata != NULL)
		kfree(idata);

cmd_done:
	mmc_blk_put(md);

	return err;
}
#endif
 /*lint +e429 +e593*/
/* create a virtual device for dma_alloc */
#define SECURE_STORAGE_NAME "secure_storage"
#define RPMB_DEVICE_NAME "hisi_rpmb"
static int hisi_rpmb_device_init(void)
{
	struct device *pdevice;
	struct class *rpmb_class;
	struct device_node *np = NULL;
	enum rpmb_version version;
	dma_addr_t rpmb_request_phy = 0;
	unsigned long mem_size = 0;
	phys_addr_t bl31_smem_base =
		HISI_SUB_RESERVED_BL31_SHARE_MEM_PHYMEM_BASE;
	u32 data[2] = {0};
	rpmb_class = class_create(THIS_MODULE, SECURE_STORAGE_NAME);
	if (IS_ERR(rpmb_class))
		return (int)PTR_ERR(rpmb_class);

	pdevice = device_create(rpmb_class, NULL, MKDEV(0, 0), NULL,
				RPMB_DEVICE_NAME);
	if (IS_ERR(pdevice))
		goto err_class_destroy;

	hisi_rpmb.dev = pdevice;

	if (device_create_file(pdevice, &dev_attr_rpmb_key)) {
		pr_err("rpmb error: unable to create sysfs attributes\n");
		goto err_device_destroy;
	}

	np = of_find_compatible_node(NULL, NULL, "hisilicon,hisi-rpmb");
	if (!np) {
		pr_err("rpmb err of_find_compatible_node");
		goto err_device_remove_file;
	}

	if (of_property_read_u32_array(np, "hisi,bl31-share-mem", &data[0],
				       (unsigned long)2)) {
		pr_err("rpmb get share_mem_address failed\n");
		goto err_node;
	}

	rpmb_request_phy = bl31_smem_base + data[0];
	mem_size = data[1];

	hisi_rpmb.rpmb_request = ioremap_wc(rpmb_request_phy, mem_size);
	if (!hisi_rpmb.rpmb_request)
		goto err_node;
	if (atfd_hisi_rpmb_smc((u64)RPMB_SVC_REQUEST_ADDR, rpmb_request_phy,
			       (u64)rpmb_support_device, (u64)0x0)) {
		pr_err("rpmb set shared memory address failed\n");
		goto err_ioremap;
	}

	memcpy((void *)&rpmb_device_info, (const void *)hisi_rpmb.rpmb_request, sizeof(struct rpmb_config_info));

	version = (enum rpmb_version)atfd_hisi_rpmb_smc(
		(u64)RPMB_SVC_GET_DEV_VER, (u64)0, (u64)0, (u64)0);
	if (version <= RPMB_VER_INVALID || version >= RPMB_VER_MAX) {
		pr_err("Error: invalid rpmb dev ver: 0x%x\n", version);
		goto err_ioremap;
	}

	hisi_rpmb.dev_ver = version;

	return 0;

err_ioremap:
	iounmap(hisi_rpmb.rpmb_request);
err_node:
	of_node_put(np);
err_device_remove_file:
	device_remove_file(pdevice, &dev_attr_rpmb_key);
err_device_destroy:
	device_destroy(rpmb_class, pdevice->devt);
err_class_destroy:
	class_destroy(rpmb_class);
	return -1;
}

static int hisi_rpmb_work_thread(void *arg)
{
	set_freezable();
	while(!kthread_should_stop()){
		wait_event_freezable(hisi_rpmb.wait, hisi_rpmb.wake_up_condition);
		rpmb_work_routine();
	}
	return 0;
}

static int __init hisi_rpmb_init(void)
{
	struct sched_param param;
	hisi_rpmb.wake_up_condition = 0;
	init_waitqueue_head(&hisi_rpmb.wait);
	hisi_rpmb.rpmb_task = kthread_create(hisi_rpmb_work_thread, NULL, "rpmb_task");
	param.sched_priority = 1;
	sched_setscheduler(hisi_rpmb.rpmb_task, SCHED_FIFO, &param);
	wake_up_process(hisi_rpmb.rpmb_task);
	rpmb_support_device = get_bootdevice_type();
	if (hisi_rpmb_device_init())
		return -1;
	else
		rpmb_device_init_status = RPMB_DEVICE_IS_READY;
	wake_lock_init(&hisi_rpmb.wake_lock, WAKE_LOCK_SUSPEND,"hisi-rpmb-wakelock");

	#ifdef CONFIG_HISI_RPMB_TIME_DEBUG
	hisi_rpmb_time_stamp_debugfs_init();
	#endif
	return 0;
}
late_initcall(hisi_rpmb_init);
MODULE_AUTHOR("qianziye@huawei.com>");
MODULE_DESCRIPTION("Hisilicon Secure RPMB.");
MODULE_LICENSE("GPL v2");
#pragma GCC diagnostic pop
