#include <linux/interrupt.h>
#include <linux/sizes.h>
#include <linux/sched.h>
#include <linux/bug.h>
#include <linux/mm.h>
#include <linux/bitmap.h>
#include <linux/device.h>
#include <dsm/dsm_pub.h>
#include <huawei_platform/linux/stat_mm.h>

#define DSM_MM_ENTRY_MAX_NR 100
#define STAT_MM_MAX_PARA_NUM 5
#define STAT_MM_BUFFER_SIZE 128
#define STAT_MM_STR "stat_mm"

struct stat_mm_para {
	unsigned long int para[STAT_MM_MAX_PARA_NUM];
};

static struct dsm_client *stat_mm_client;
static struct dsm_dev stat_mm_dev = {
	.name = "dsm_" STAT_MM_STR,
	.fops = NULL,
	.buff_size = STAT_MM_BUFFER_SIZE,
};

static struct class *stat_mm_class;

static DECLARE_BITMAP(stat_mm_mask,
		      DSM_MM_ENTRY_MAX_NR) ____cacheline_aligned_in_smp;

static inline bool stat_mm_slab_can_stat(int id,
					 int type, struct stat_mm_para *para)
{
	if (type == 0) {
		int size = para->para[0];

		if (size >= SZ_64K) {
			para->para[0] = size >> PAGE_SHIFT;
			WARN_ON(1);
			return true;
		}
	}
	return false;
}

static inline bool stat_mm_mlock_can_stat(int id,
					  int type, struct stat_mm_para *para)
{
	if (type == 0) {
		long len = para->para[1];
		long locked_vm = para->para[2];

		if (len >= SZ_4M || locked_vm > (SZ_32M / PAGE_SIZE))
			return true;
	}
	return false;
}

static ssize_t stat_mm_mask_store(struct class *class,
				  struct class_attribute *attr,
				  const char *buf, size_t count)
{
	unsigned int bit = -1U;

	if (kstrtouint(buf, 10, &bit))
		return -EINVAL;
	if (unlikely(bit >= DSM_MM_ENTRY_MAX_NR))
		return -EINVAL;
	test_and_change_bit(bit, stat_mm_mask);
	return count;
}

static const struct stat_mm_callback {
	bool (*can_stat)(int id, int type, struct stat_mm_para *para);
} stat_mm_cb[STAT_MM_IDS] = {
	{ stat_mm_slab_can_stat },
	{ stat_mm_mlock_can_stat },
};

static CLASS_ATTR(mask, S_IWUSR, NULL, stat_mm_mask_store);

static inline void stat_mm_stat_channel_dmd(int id,
					    int type, unsigned int num,
					    char *fmt,
					    struct stat_mm_para *para)
{
	if (likely(stat_mm_client != NULL)) {
		if (!dsm_client_ocuppy(stat_mm_client)) {
			int dsm_id = DSM_MM_STAT + num;
			/* add dmd id */
			dsm_client_record(stat_mm_client, "%d ", dsm_id);
			dsm_client_record(stat_mm_client,
					  fmt,
					  id,
					  type,
					  current->comm,
					  para->para[0],
					  para->para[1],
					  para->para[2],
					  para->para[3], para->para[4]);
			dsm_client_notify(stat_mm_client, dsm_id);
		}
	}
}

static inline void stat_mm_stat_channel(int id,
					int type, unsigned int num,
					char *fmt, struct stat_mm_para *para)
{
	stat_mm_stat_channel_dmd(id, type, num, fmt, para);
}

static void stat_mm_copy_args(struct stat_mm_para *para,
			      unsigned int nr, va_list args)
{
	int i;

	if (unlikely(nr > STAT_MM_MAX_PARA_NUM))
		nr = STAT_MM_MAX_PARA_NUM;
	for (i = 0; i < nr; i++)
		para->para[i] = va_arg(args, long int);
}

static inline bool stat_mm_can_stat(int id,
				    int type, unsigned int num,
				    unsigned int nr,
				    struct stat_mm_para *para, va_list args)
{
	if (in_interrupt())
		return false;
	if (num >= DSM_MM_ENTRY_MAX_NR)
		return false;
	if (test_bit(num, stat_mm_mask))
		return false;
	stat_mm_copy_args(para, nr, args);
	if (unlikely(stat_mm_cb[id].can_stat == NULL))
		return true;
	return stat_mm_cb[id].can_stat(id, type, para);
}

void stat_mm_stat(int id, int type,
		unsigned int num, unsigned int nr,
		char *fmt, ...)
{
	bool can;
	va_list args;
	struct stat_mm_para para;

	va_start(args, fmt);
	can = stat_mm_can_stat(id, type, num, nr, &para, args);
	va_end(args);
	if (can)
		stat_mm_stat_channel(id, type, num, fmt, &para);
}

static int __init stat_mm_init(void)
{
	int err = 0;

	stat_mm_client = dsm_register_client(&stat_mm_dev);
	bitmap_zero(stat_mm_mask, DSM_MM_ENTRY_MAX_NR);
	stat_mm_class = class_create(THIS_MODULE, STAT_MM_STR);
	if (IS_ERR(stat_mm_class)) {
		err = PTR_ERR(stat_mm_class);
		goto out;
	}
	err = class_create_file(stat_mm_class, &class_attr_mask);
	if (err == 0)
		return 0;
	class_destroy(stat_mm_class);
out:
	return err;
}

static void __exit stat_mm_exit(void)
{
	class_destroy(stat_mm_class);
}

module_init(stat_mm_init);
module_exit(stat_mm_exit);
