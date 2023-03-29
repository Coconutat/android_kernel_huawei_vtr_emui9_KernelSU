#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/of_fdt.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

static const char *hardware_name;
static int hardware_info_show(struct seq_file *m, void *v)
{
	if (hardware_name)
		seq_printf(m, "%s\n", hardware_name);
	else
		seq_printf(m, "%s\n", "unknown");

	return 0;
}

static int hardware_info_open(struct inode *inode, struct file *file)
{
	return single_open(file, hardware_info_show, NULL);
}

const struct file_operations hardware_info_fops = {
	.owner = THIS_MODULE,
	.open = hardware_info_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init proc_hardware_info_init(void)
{
	hardware_name = of_flat_dt_get_machine_name();
	proc_create("hardware", 0440, NULL, &hardware_info_fops);

	return 0;
}
fs_initcall(proc_hardware_info_init);
