#include "../rasbase/rasbase.h"
#include "../rasbase/rasprobe.h"
#include "../rasbase/rasproc.h"
#include <linux/kernel.h>
#include <linux/kallsyms.h>
#include <linux/proc_fs.h>
#include <linux/fs.h>
#include <linux/genhd.h>
#include <linux/statfs.h>
#include <linux/module.h>
#include <linux/path.h>
#include <linux/kthread.h>
#include <linux/thermal.h>

#define SIZE_DATA 10
static int capacity = -1;
static unsigned int *hook_capacity_sum;
static unsigned int *hook_capacity_filter;
#ifdef CONFIG_HISI_COUL
void skip_pulling(unsigned int cap)
{
	int i = 0;

	if (hook_capacity_filter == NULL)
		return;
	for (i = 0; i < SIZE_DATA; i++)
		hook_capacity_filter[i] = capacity;
	*hook_capacity_sum = 10*capacity;
}

static int rasprobe_handler(hisi_battery_capacity) (struct rasprobe_instance
							*ri,
							struct pt_regs *regs) {
	if (capacity == -1)
		return 0;
	skip_pulling(capacity);
	rasprobe_seturn(regs, capacity);
	return 0;
}
static int rasprobe_handler(hisi_battery_current) (struct rasprobe_instance
							*ri,
							struct pt_regs *regs) {
	if (capacity == -1)
		return 0;
	rasprobe_seturn(regs, 0);/*capacity changed when usb connected*/
	return 0;
}

static int rasprobe_handler(hisi_battery_voltage) (struct rasprobe_instance
							*ri,
							struct pt_regs *regs) {
	if (capacity != -1 && capacity < 2)
		rasprobe_seturn(regs, 3400);/*simulate the vol low*/
	return 0;
}
rasprobe_define(hisi_battery_capacity);
rasprobe_define(hisi_battery_current);
rasprobe_define(hisi_battery_voltage);
#endif

static struct rasprobe *probes[] = {
#ifdef CONFIG_HISI_COUL
	&rasprobe_name(hisi_battery_capacity),
	&rasprobe_name(hisi_battery_current),
	&rasprobe_name(hisi_battery_voltage),
#endif
};

static int cmd_main(void *data, int argc, char *argv[])
{
	long long val;

	ras_retn_if(2 > argc, -EINVAL);
	ras_retn_iferr(ras_atoll(&val, argv[1], strlen(argv[1]), 0));

	if (val < 0 || val > 100)
		return -EINVAL;
	capacity = val;
	return 0;
}

static int proc_ops_show(rBattery)(struct seq_file *m, void *v)
{
	if (capacity != -1)
		seq_printf(m, "Capacity modified:\t%2d\n", capacity);
	else
		seq_puts(m, "Capacity not modified.\n");

	return 0;
}

static int proc_ops_open(rBattery)(struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(rBattery), PDE_DATA(inode));

}

static ssize_t proc_ops_write(rBattery)(struct file *filp,
					 const char __user *bff, size_t count,
					 loff_t *data) {
	char buf_cmd[256] = { 0 };

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd,
		count, cmd_main, PDE_DATA(FILE_NODE(filp))));
	return count;
}
#define MODULE_NAME "rBattery"
proc_ops_define(rBattery);
static int tool_init(void)
{
	ras_debugset(1);
	ras_retn_iferr(ras_check());
	hook_capacity_sum = (unsigned int *)kallsyms_lookup_name("capacity_sum");
	hook_capacity_filter = (unsigned int *)
		kallsyms_lookup_name("capacity_filter");
	ras_retn_iferr(register_rasprobes(probes, ARRAY_SIZE(probes)));
	ras_retn_iferr(proc_init(MODULE_NAME, &proc_ops_name(rBattery), NULL));
	return 0;
}
static void tool_exit(void)
{
	capacity = -1;
	unregister_rasprobes(probes, ARRAY_SIZE(probes));
	proc_exit(MODULE_NAME);
}

module_init(tool_init);
module_exit(tool_exit);
MODULE_DESCRIPTION("Change the capacity of battery.");
MODULE_LICENSE("GPL");
#ifndef RASFIRE_VERSION
#define RASFIRE_VERSION "V001R001C151-"
#endif
MODULE_VERSION(RASFIRE_VERSION "1.0");
