
#include <linux/kernel.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>
#include <linux/usb.h>

#include "proxy-hcd.h"
#include "proxy-hcd-stat.h"

int phcd_seq_print_stat(struct proxy_hcd *phcd, struct seq_file *s)
{
	struct proxy_hcd_stat *phcd_stat;
	struct proxy_hcd_usb_device_stat *udev_stat;
	struct proxy_hcd_urb_stat *urb_stat;
	int i;

	if (!phcd) {
		seq_printf(s, "proxy_hcd destroyed!\n");
		return -EINVAL;
	}

	phcd_stat = &phcd->stat;
#define PR_PHCD_STAT(__stat) seq_printf(s, "phcd_stat %s: %ld\n", #__stat, (unsigned long)phcd_stat->__stat)
	PR_PHCD_STAT(stat_alloc_dev);
	PR_PHCD_STAT(stat_free_dev);
	PR_PHCD_STAT(stat_hub_control);
	PR_PHCD_STAT(stat_hub_status_data);
	PR_PHCD_STAT(last_hub_control_time);
	PR_PHCD_STAT(last_hub_status_data_time);
	PR_PHCD_STAT(stat_bus_suspend);
	PR_PHCD_STAT(stat_bus_suspend);
#undef PR_PHCD_STAT

	udev_stat = &phcd->phcd_udev.stat;
#define PR_PHCD_UDEV_STAT(_stat) seq_printf(s, "urb_stat %s: %ld\n", #_stat,	\
					(unsigned long)udev_stat->_stat)
	PR_PHCD_UDEV_STAT(stat_urb_complete_pipe_err);
#undef PR_PHCD_UDEV_STAT

#define PR_PHCD_URB_STAT(_stat) seq_printf(s, "urb_stat.%s: %ld\n", #_stat,	\
					(unsigned long)urb_stat->_stat)
	for (i = 0; i < PROXY_HCD_DEV_MAX_EPS; i++) {
		urb_stat = &phcd->phcd_udev.phcd_eps[i].urb_stat;

		if (urb_stat->stat_urb_enqueue == 0)
			continue;

		seq_printf(s, "--------- ep%d ---------\n", i);
		PR_PHCD_URB_STAT(stat_urb_enqueue);
		PR_PHCD_URB_STAT(stat_urb_dequeue);
		PR_PHCD_URB_STAT(stat_urb_enqueue_fail);
		PR_PHCD_URB_STAT(stat_urb_dequeue_fail);
		PR_PHCD_URB_STAT(stat_urb_dequeue_giveback);
		PR_PHCD_URB_STAT(stat_urb_giveback);
		PR_PHCD_URB_STAT(stat_urb_complete);
		PR_PHCD_URB_STAT(stat_urb_complete_fail);
	}
#undef PR_PHCD_URB_STAT

	seq_printf(s, "client_ref: live %d, count %ld\n",
			phcd->client->client_ref.live,
			phcd->client->client_ref.count);

	return 0;
}

static int phcd_print_stat_show(struct seq_file *s, void *unused)
{/*lint !e578 */
	struct proxy_hcd *phcd = s->private;
	return phcd_seq_print_stat(phcd, s);
}

static int phcd_print_stat_open(struct inode *inode, struct file *file)
{
	return single_open(file, phcd_print_stat_show, inode->i_private);
}

static const struct file_operations phcd_print_stat_fops = {
	.open			= phcd_print_stat_open,
	.read			= seq_read,
	.llseek			= seq_lseek,
	.release		= single_release,
};

int phcd_debugfs_init(struct proxy_hcd *phcd)
{
	struct dentry		*root;
	struct dentry		*file;

	root = debugfs_create_dir("proxyhcd", usb_debug_root);
	if (IS_ERR_OR_NULL(root))
		return -ENOMEM;

	file = debugfs_create_file("stat", S_IRUSR, root,
			phcd, &phcd_print_stat_fops);
	if (!file)
		goto err;

	phcd->debugfs_root = root;
	return 0;

err:
	debugfs_remove_recursive(root);
	return -ENOMEM;
}

void phcd_debugfs_exit(struct proxy_hcd *phcd)
{
	if (phcd->debugfs_root)
		debugfs_remove_recursive(phcd->debugfs_root);
}
