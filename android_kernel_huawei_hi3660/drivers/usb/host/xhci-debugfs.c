

#include <linux/device.h>
#include <linux/usb.h>
#include <linux/debugfs.h>
#include <linux/seq_file.h>
#include <linux/uaccess.h>

#include "xhci.h"

static int xhci_compliance_show(struct seq_file *s, void *d)
{
	seq_printf(s, "usage: echo 1 > xhci_compliance\n");

	return 0;
}

static int xhci_compliance_open(struct inode *inode, struct file *file)
{
	return single_open(file, xhci_compliance_show, inode->i_private);
}

static ssize_t xhci_compliance_write(struct file *file, const char __user *ubuf,
		size_t size, loff_t *ppos)
{
	struct seq_file	*s = file->private_data;
	struct xhci_hcd *xhci = s->private;

	if (size > PAGE_SIZE) {
		printk(KERN_ERR "xhci_compliance_write too long!\n");
		return -ENOMEM;
	}

	pr_info("set link state compliance mode\n");
	xhci_set_link_state(xhci, xhci->usb3_ports, 0, USB_SS_PORT_LS_COMP_MOD);

	return size;
}

static const struct file_operations xhci_compliance_debug_fops = {
	.open = xhci_compliance_open,
	.read = seq_read,
	.write = xhci_compliance_write,
	.release = single_release,
};

int xhci_create_debug_file(struct xhci_hcd *xhci)
{
	struct dentry *root;
	struct dentry *file;
	int ret;

	root = debugfs_create_dir("xhci", usb_debug_root);
	if (!root) {
		ret = -ENOMEM;
		goto err0;
	}

	file = debugfs_create_file("xhci_compliance", S_IWUSR | S_IRUSR,
			root, xhci, &xhci_compliance_debug_fops);
	if (!file) {
		pr_err("create xhci_compliance debugfs file error!\n");
		ret = -ENOMEM;
		goto err1;
	}

	xhci->debugfs_root = root;

	return 0;

err1:
	debugfs_remove_recursive(root);

err0:
	return ret;
}

void xhci_remove_debug_file(struct xhci_hcd *xhci)
{
	if (xhci->debugfs_root)
		debugfs_remove_recursive(xhci->debugfs_root);
	xhci->debugfs_root = NULL;
}
