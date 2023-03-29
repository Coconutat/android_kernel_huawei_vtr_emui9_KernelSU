#include "../rasbase/rasbase.h"
#include "../rasbase/rasproc.h"

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/init.h>
#include <linux/sched.h>	/*wake_up_process()*/
#include <linux/kthread.h>	/*kthread_create()/kthread_run()*/
#include <linux/vmalloc.h>
#include <linux/interrupt.h>
/*#include <asm/mce.h>*/
#include <linux/wait.h>
#include <linux/pci.h>
#include <linux/msi.h>
struct RMissile {
	unsigned int para;
	unsigned int para1;
	unsigned long tmout;
};
struct RInject {
	unsigned long idx;
	const char *cmd;
	int (*missile)(struct RMissile *);
	char *dsc;
};

static int rmissile_function(struct RMissile *prm)
{
	void (*func)(struct RMissile *) = 0;

	func(prm);  /* [false alarm] */
	return 0;
}
static int rmissile_null(struct RMissile *prm)
{
	*(char *)0 = 0;
	return 0;
}
static int rmissile_panic(struct RMissile *prm)
{
	panic("panic inovke");
	return 0;
}

static int rmissile_allocate(struct RMissile *prm)
{
	while (0 != vmalloc(1024 * 1024))
		;
	panic("malloc fail");
	return 0;
}

static int rmissile_interrupt(struct RMissile *prm)
{
	panic(" TODO ");
	return 0;
}

static int rmissile_hardware(struct RMissile *prm)
{
	panic(" TODO ");
	return 0;
}

static int rmissile_match(struct RMissile *prm)
{
	panic(" TODO ");
	return 0;
}
static int rmissile_data(struct RMissile *prm)
{
	panic(" TODO ");
	return 0;
}

static int rmissile_dispatch(struct RMissile *prm)
{

	panic(" TODO ");
	return 0;
}
static int rmissile_irq(struct RMissile *prm)
{
	spinlock_t lock;

	spin_lock_init(&lock);
	if (prm->para == UINT_MAX) {
		/*printk("rmissile irq para is null\n");*/
		spin_lock_irq(&lock);
		while (1)
			;

	} else {
		/*printk("rmissile irq para is %u\n", prm->para);*/
		disable_irq_nosync(prm->para);
	}
	return 0;
}
static int rmissile_text(struct RMissile *prm)
{
	int (*func)(struct RMissile *prm) = rmissile_irq;
	*((int *)func + 1) = 1;
	*((int *)func + 2) = 0;
	(*func) (NULL);

	return 0;
}

static int rmissile_devirq(struct RMissile *prm)
{
#if !defined(CONFIG_ARM64) && !defined(CONFIG_ARM)
	struct msi_desc *entry = NULL;
	struct pci_dev *dev = NULL;

	if (prm->para == UINT_MAX || prm->para1 == UINT_MAX)
		return -1;
	/*printk("----,%u,%u\n", prm->para, prm->para1);*/
	dev = pci_get_device(prm->para, prm->para1, NULL);
	if (!dev) {
		pr_err("rmissile can not find dev\n");
		return -1;
	}
	/*printk("-----rmissile irq=%d,msixenable=%d,%u,%u\n", dev->irq,
	       dev->msix_enabled, prm->para, prm->para1);*/
	dev->msix_enabled = 0;
	list_for_each_entry(entry, &dev->msi_list, list) {
		/*printk("rmissile:masked---%d,irq=%d,is_msix=%d\n",
		       entry->masked, entry->irq, entry->msi_attrib.is_msix);*/
		disable_irq_nosync(entry->irq);
	}
	/* disable_irq_nosync(26);*/
#endif
	return 0;
}

#define rmissile(op) rmissile_##op

static struct RInject *rmissile_get(char *cmd)
{
	int i;
	static struct RInject sr[] = {
		{0, "ndp", rmissile(null), "null data pointer access"},
		{1, "nfp", rmissile(function), "null function pointer access"},
		{2, "pnc", rmissile(panic), "panic invoking"},
		{3, "mem", rmissile(allocate), "allocate memory fail"},
		{4, "*hwf", rmissile(hardware), "hardware fault"},
		{5, "*svm", rmissile(match), "software version don't match"},
		{6, "*dke", rmissile(data), "data of key exception"},
		{7, "*doe", rmissile(dispatch), "dispatch operate exception"},
		{8, "*itr", rmissile(interrupt), "interrupt exception"},
		{9, "irq", rmissile(irq), "irq close long time"},
		{10, "irqnum", rmissile(irq),
		 "specific irq num. as a example: ./rmissile irq 26"},
		{11, "devirq", rmissile(devirq),
"disable dev's irqs. cmd format: ./rmissile devirq [vendorid] [deviceid]"},
		{12, "text", rmissile(text),
		 "function text segmentation corrupt"},
	}

	;
	for (i = 0; i < ARRAY_SIZE(sr); i++) {
		if (0 == strcmp(sr[i].cmd, cmd))
			return &sr[i];
	}
	return 0;
};

static struct RMissile rm;

static int cmd_main(void *data, int argc, char *args[])
{
	struct RInject *pri = 0;
	long long val = 0;
	char *cmd;

	if (args[0] == NULL)
		return -EINVAL;
	cmd = args[0];

	rm.para = UINT_MAX;
	if (0 == strcmp(cmd, "irqnum") && argc < 2)
		return -EINVAL;
	if (argc >= 2) {
		ras_retn_iferr(ras_atoll(&val, args[1], strlen(args[1]), 0));
		if (val < 0)
			return -EINVAL;
		rm.para = (unsigned int)val;
	}

	rm.para1 = UINT_MAX;
	if (argc >= 3) {
		ras_retn_iferr(ras_atoll(&val, args[2], strlen(args[2]), 0));
		if (val < 0)
			return -EINVAL;
		rm.para1 = (unsigned int)val;
	}
	pri = rmissile_get(cmd);
	/*printk("rmissile cmd=%s,%u,%u,%s\n", cmd, rm.para,
	rm.para1, pri->cmd);*/
	if (0 == pri || 0 != pri->missile(&rm))	{
		pr_err("rmissile idex=%s,error=%d", cmd, -EINVAL);
		return -EINVAL;
	}
	return 0;
}

static int proc_ops_show(rmissile) (struct seq_file *m, void *v)
{
	return 0;
}

static int proc_ops_open(rmissile) (struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(rmissile), NULL);
}

static ssize_t proc_ops_write(rmissile) (struct file *filp,
					 const char __user *bff, size_t count,
					 loff_t *data) {
	char buf_cmd[256];

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	memset(buf_cmd, 0, sizeof(buf_cmd));
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd, count, cmd_main, NULL));
	return 1024;
}

#define MODULE_NAME "rmissile"
proc_ops_define(rmissile);
static int __init rmissile_init(void)
{
	ras_retn_iferr(ras_check());
	ras_retn_iferr(proc_init(MODULE_NAME, &proc_ops_name(rmissile), &rm));
	return 0;
}

static void __exit rmissile_exit(void)
{
	proc_exit(MODULE_NAME);
}

module_init(rmissile_init);
module_exit(rmissile_exit);
MODULE_LICENSE("GPL");
#ifndef RASFIRE_VERSION
#define RASFIRE_VERSION
#endif
MODULE_VERSION(RASFIRE_VERSION "1.0");
