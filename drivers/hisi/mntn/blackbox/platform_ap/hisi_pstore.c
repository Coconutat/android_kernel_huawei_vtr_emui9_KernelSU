#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/clk.h>
#include <linux/io.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/hisi/util.h>
#include <linux/uaccess.h>	/* For copy_to_user */
#include <linux/module.h>
#include <linux/list.h>
#include <linux/slab.h>
#include <libhwsecurec/securec.h>
#include <linux/hisi/hisi_pstore.h>
#include <linux/hisi/hisi_log.h>
#define HISI_LOG_TAG HISI_BLACKBOX_TAG

#define LOG_TAG "hisi persist store"
#define PERSIST_STORE_NAMELEN 64 /*PSTORE_NAMELEN*/

struct persist_store_info {
	struct list_head node;
	char name[PERSIST_STORE_NAMELEN];
	size_t size;
	char data[0];
};

static DEFINE_RAW_SPINLOCK(persist_store_lock);
LIST_HEAD(__list_persist_store);

/*read persist store file content*/
static ssize_t persist_store_file_read(struct file *file,
					  char __user *userbuf, size_t bytes,
					  loff_t *off)
{
	struct persist_store_info *info = (struct persist_store_info *)file->private_data;
	ssize_t copy;
	size_t size;

	if (!userbuf) {
		pr_err("%s(), userbuf is NULL.\n", __func__);
		return 0;
	}

	if (!info) {
		pr_err("%s(), the proc file don't be created in advance.\n", __func__);
		return 0;
	}

	size = info->size;

	if ((*off < 0) || (*off > (loff_t)size)) {
		pr_err("%s(), read offset error.\n", __func__);
		return 0;
	}

	if (*off == (loff_t)size) {
		/*end of file */
		return 0;
	}

	copy = (ssize_t) min(bytes, (size_t) (size - (size_t)*off));

	if (copy_to_user(userbuf, info->data + *off, copy)) {
		pr_err("%s(): copy to user error\n", __func__);
		copy = -EFAULT;
		goto copy_err;
	}

	*off += copy;

copy_err:
	return copy;
}

static int persist_store_file_open(struct inode *inode, struct file *file)
{
	file->private_data = PDE_DATA(inode);

	if (list_empty(&__list_persist_store)) {
		pr_err("%s(): hisi pstore has not init yet.\n", __func__);
		return -EFAULT;
	}
	return 0;
}

static int persist_store_file_release(struct inode *inode,
					  struct file *file)
{
	file->private_data = NULL;
	return 0;
}

static const struct file_operations persist_store_file_fops = {
	.open = persist_store_file_open,
	.read = persist_store_file_read,
	.release = persist_store_file_release,
};

/*****************************************************************************
Description : hisi_save_pstore_log
get pstore inode and save its memory info to a list
History
Modification : Created function
 *****************************************************************************/
/*lint -e429*/
void hisi_save_pstore_log(char *name, void *data, size_t size)
{
	struct persist_store_info *info;

	/*as a public interface, we should check the parameter */
	if (IS_ERR_OR_NULL(name) || IS_ERR_OR_NULL(data)) {
		pr_err("%s(): parameter is NULL.\n", __func__);
		return;
	}

	if (!size) {
		pr_err("%s(): size is zero.\n", __func__);
		return;
	}
	info = kzalloc(sizeof(struct persist_store_info) + size, GFP_ATOMIC);
	if (IS_ERR_OR_NULL(info)) {
		pr_err("%s(), kzalloc fail !\n", __func__);
		return;
	}

	strncpy_s(info->name, PERSIST_STORE_NAMELEN-1, name, PERSIST_STORE_NAMELEN);
	info->name[PERSIST_STORE_NAMELEN-1] = '\0';
	info->size = size;
	if (EOK != memcpy_s((void*)info->data, info->size, data, size)) {
		pr_err("%s(): memcpy_s %s failed, size is 0x%lx.\n",
			   __func__, info->name, (unsigned long)size);
		kfree(info);
		info = NULL;
		return;
	}

	pr_info("bbox save persist store:%s, size: 0x%lx.\n", info->name, (unsigned long)size);

	raw_spin_lock(&persist_store_lock);
	list_add(&info->node, &__list_persist_store);
	raw_spin_unlock(&persist_store_lock);
}
/*lint +e429*/

/*****************************************************************************
Description : hisi_create_pstore_entry
create mntn pstore node for kernel reading
History
Modification : Created function
 *****************************************************************************/
void hisi_create_pstore_entry(void)
{
	struct persist_store_info *info,*n;
	struct proc_dir_entry *pde;

	list_for_each_entry_safe(info, n, &__list_persist_store, node) {
		pde = balong_create_pstore_proc_entry(info->name, S_IRUSR | S_IRGRP,
			  &persist_store_file_fops, info);

		if (!pde) {
			list_del(&info->node);
			kfree(info);
			info = NULL;
		}
	}
}

/*****************************************************************************
Description : hisi_remove_pstore_entry
remove mntn pstore node
History
Modification : Created function
 *****************************************************************************/
void hisi_remove_pstore_entry(void)
{
	struct persist_store_info *info,*n;

	list_for_each_entry_safe(info, n, &__list_persist_store, node) {
		balong_remove_pstore_proc_entry(info->name);
	}
}

/*****************************************************************************
Description : hisi_free_persist_store
free persist pstore memory
History
Modification : Created function
 *****************************************************************************/
void hisi_free_persist_store(void)
{
	struct persist_store_info *info,*n;
	unsigned int nums=0;

	list_for_each_entry_safe(info, n, &__list_persist_store, node) {
		list_del(&info->node);
		kfree(info);
		info = NULL;
		nums++;
	}

	pr_info("%s done, quantities is %u.\n", __func__, nums);
}
