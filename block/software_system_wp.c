

#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/pagemap.h>
#include <linux/err.h>
#include <linux/scatterlist.h>
#include <linux/version.h>

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/hdreg.h>
#include <linux/kdev_t.h>
#include <linux/blkdev.h>
#include <linux/mutex.h>

#include <linux/mmc/ioctl.h>
#include <linux/mmc/card.h>
#include <linux/mmc/host.h>
#include <linux/mmc/mmc.h>
#include <linux/mmc/sd.h>

#include <linux/uaccess.h>

#ifdef CONFIG_HW_FEATURE_STORAGE_DIAGNOSE_LOG
#include <linux/store_log.h>
#endif
#ifdef CONFIG_HUAWEI_EMMC_DSM
#include <linux/mmc/dsm_emmc.h>
#endif

#include <linux/proc_fs.h>

/*  system write protect flag, 0: disable(default) 1:enable */
static int *ro_secure_debuggable;

static char *protected_partitions[] = {
	"system",
	"system_a",
	"system_b",
	"cust",
	"cust_a",
	"cust_b",
	"vendor",
	"vendor_a",
	"vendor_b",
	"product",
	"product_a",
	"product_b",
	NULL,
};

int blk_set_ro_secure_debuggable(int state)
{
	*ro_secure_debuggable = state;
	return 0;
}
EXPORT_SYMBOL(blk_set_ro_secure_debuggable);

static char *get_bio_part_name(struct bio *bio)
{
	if (unlikely(!bio || !bio->bi_bdev ||
				!bio->bi_bdev->bd_part ||
				!bio->bi_bdev->bd_part->info ||
				!bio->bi_bdev->bd_part->info->volname[0]))
		return NULL;
	return bio->bi_bdev->bd_part->info->volname;
}

static inline char *fastboot_lock_str(void)
{
	if (strstr(saved_command_line, "fblock=locked") ||
			strstr(saved_command_line, "userlock=locked"))
		return "locked";
	else
		return "unlock";
}

static inline int is_protected_partition(const char *name)
{
	int i;

	for (i = 0; protected_partitions[i]; i++) {
		if (!strncmp(name, protected_partitions[i],
					strlen(protected_partitions[i]) + 1)) {
			return 1;
		}
	}

	return 0;
}

static void bio_endio_compat(struct bio *bio, int error)
{
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 3, 0)
	bio->bi_error = error;
	bio_endio(bio);
#else
	bio_endio(bio, error);
#endif
}

int should_trap_this_bio(int rw, struct bio *bio, unsigned int count)
{
	char *name;

	if (!(rw & WRITE))
		return 0;

	name = get_bio_part_name(bio);

	/*
	 * runmode=factory:send write request to mmc driver.
	 * bootmode=recovery:send write request to mmc driver.
	 * partition is mounted ro: file system will block write request.
	 * root user: send write request to mmc driver.
	 */
	if (!name)
		return 0;

	if (likely(!is_protected_partition(name)))
		return 0;

	if ((NULL == ro_secure_debuggable) || (0 == *ro_secure_debuggable))
		return 0;

#ifdef CONFIG_HUAWEI_EMMC_DSM
	DSM_EMMC_LOG(NULL, DSM_SYSTEM_W_ERR,
#else
			printk(KERN_DEBUG
#endif
				"[HW]:EXT4_ERR_CAPS:%s(%d)[Parent: %s(%d)]: %s block %llu on %s (%u sectors) %d %s.\n",
				current->comm, task_pid_nr(current),
				current->parent->comm,
				task_pid_nr(current->parent),
				(rw & WRITE) ? "WRITE" : "READ",
				(u64)bio->bi_iter.bi_sector,
				name,
				count,
				*ro_secure_debuggable,
				fastboot_lock_str());

	bio_endio_compat(bio, -EIO);
	return 1;
}
EXPORT_SYMBOL(should_trap_this_bio);

static int syswp_proc_show(struct seq_file *m, void *v)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 3, 0))
	return seq_printf(m, "%d\n", *ro_secure_debuggable);
#else
	seq_printf(m, "%d\n", *ro_secure_debuggable);
	return 0;
#endif
}

static int syswp_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, syswp_proc_show, NULL);
}

static ssize_t syswp_proc_write(struct file *file, const char *buf,
		size_t count, loff_t *pos)
{
	int ret, value;

	if ((count <= 0) || (count > 2))
		return -EINVAL;

	ret = kstrtoint_from_user(buf, count, 10, &value);
	if (ret)
		return ret;

	blk_set_ro_secure_debuggable(!!value);

	return count;
}

static const struct file_operations syswp_proc_fops = {
	.owner          = THIS_MODULE,
	.open           = syswp_proc_open,
	.read           = seq_read,
	.llseek         = seq_lseek,
	.release        = single_release,
	.write          = syswp_proc_write,
};

int __init ro_secure_debuggable_init(void)
{
	static int ro_secure_debuggable_static;
	struct proc_dir_entry *ent;

	ro_secure_debuggable = kzalloc(sizeof(int), GFP_KERNEL);
	if (!ro_secure_debuggable)
		ro_secure_debuggable = &ro_secure_debuggable_static;


	ent = proc_create_data("sys_wp_soft", 0640,
			NULL, &syswp_proc_fops, NULL);
	if (!ent)
		return -1;

	return 0;
}
late_initcall(ro_secure_debuggable_init);
