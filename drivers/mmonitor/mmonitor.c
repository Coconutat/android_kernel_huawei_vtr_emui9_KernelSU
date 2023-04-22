#include <linux/mm.h>
#include <linux/err.h>
#include <linux/module.h>
#include <linux/cpu.h>
#include <linux/slab.h>
#include <linux/vmstat.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/debugfs.h>
#include <linux/math64.h>
#include <linux/writeback.h>
#include <linux/compaction.h>
#include <linux/time.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <log/log_usertype/log-usertype.h>

#ifdef CONFIG_HW_MEMORY_MONITOR
#include <chipset_common/mmonitor/mmonitor.h>

DEFINE_PER_CPU(struct mmonitor_event_state, mmonitor_event_states) = { {0} };
EXPORT_PER_CPU_SYMBOL(mmonitor_event_states);

static bool enable = true;
module_param_named(enable, enable, bool, S_IRUGO | S_IWUSR);


/*
 * sum up all events from per cpu.
*/
static void mmonitor_calcu_events(unsigned long *ret)
{
	int i, cpu;

	get_online_cpus();
	for_each_online_cpu(cpu) {
		struct mmonitor_event_state *s =
				&per_cpu(mmonitor_event_states, cpu);

		for (i = 0; i < NR_MMONITOR_EVENT_ITEMS; i++)
			ret[i] += s->event[i];
	}
	put_online_cpus();
}

static void mmonitor_clear_events(void)
{
	int i, cpu;
#ifdef CONFIG_HISI_SLOW_PATH_COUNT
	atomic_long_set(&pgalloc_count, 0);
	atomic_long_set(&slowpath_pgalloc_count[0], 0);
	atomic_long_set(&slowpath_pgalloc_count[1], 0);
	atomic_long_set(&slowpath_pgalloc_count[2], 0);
	atomic_long_set(&slowpath_pgalloc_count[3], 0);
	atomic_long_set(&slowpath_pgalloc_count[4], 0);
#endif
	get_online_cpus();
	for_each_online_cpu(cpu) {
#ifdef CONFIG_VM_EVENT_COUNTERS
		struct vm_event_state *this;
#endif
		struct mmonitor_event_state *s =
			&per_cpu(mmonitor_event_states, cpu);

		for (i = 0; i < NR_MMONITOR_EVENT_ITEMS; i++)
			s->event[i] = 0;

#ifdef CONFIG_VM_EVENT_COUNTERS
		this = &per_cpu(vm_event_states, cpu);

		this->event[COMPACTSTALL] = 0;
		this->event[COMPACTSUCCESS] = 0;
		this->event[PGMAJFAULT] = 0;
#endif
	}
	put_online_cpus();
}

static int mmonitor_show(struct seq_file *s, void *data)
{
	unsigned long *mmonitor_buf;
	unsigned long *vm_buf;

	if ((!enable) || (BETA_USER != get_logusertype_flag())) {
		seq_printf(s, "MMONITOR NOT SUPPORT\n");
		return 0;
	}

	mmonitor_buf = kzalloc(sizeof(struct mmonitor_event_state), GFP_KERNEL);
	if (mmonitor_buf == NULL) {
		pr_err("mmonitor_buf is invalid\n");
		return -EINVAL;
	}
	mmonitor_calcu_events(mmonitor_buf);

#ifdef CONFIG_VM_EVENT_COUNTERS
	vm_buf = kzalloc(sizeof(struct vm_event_state), GFP_KERNEL);
	if (vm_buf == NULL) {
		pr_err("vm_buf is invalid\n");
		kfree(mmonitor_buf);
		return -EINVAL;
	}
	all_vm_events(vm_buf);
#endif

	seq_printf(s,
#ifdef CONFIG_HISI_SLOW_PATH_COUNT
		"pg_alloc: %ld\n"
		"slowpath0: %ld\n"
		"slowpath1: %ld\n"
		"slowpath2: %ld\n"
		"slowpath3: %ld\n"
		"slowpath4: %ld\n"
#endif
#ifdef CONFIG_VM_EVENT_COUNTERS
		"compact_stall: %ld\n"
		"compact_suc: %ld\n"
		"fcache miss: %ld\n"
#endif
		"warn_alloc_failed: %ld\n"
		"fcache : %ld\n",
#ifdef CONFIG_HISI_SLOW_PATH_COUNT
		atomic64_read(&pgalloc_count),
		atomic64_read(&slowpath_pgalloc_count[0]),
		atomic64_read(&slowpath_pgalloc_count[1]),
		atomic64_read(&slowpath_pgalloc_count[2]),
		atomic64_read(&slowpath_pgalloc_count[3]),
		atomic64_read(&slowpath_pgalloc_count[4]),
#endif
#ifdef CONFIG_VM_EVENT_COUNTERS
		vm_buf[COMPACTSTALL],
		vm_buf[COMPACTSUCCESS],
		mmonitor_buf[FILE_CACHE_MISS_COUNT] + vm_buf[PGMAJFAULT],
#endif
		mmonitor_buf[ALLOC_FAILED_COUNT],
		mmonitor_buf[FILE_CACHE_READ_COUNT] + mmonitor_buf[FILE_CACHE_MAP_COUNT]
	);

	kfree(mmonitor_buf);
#ifdef CONFIG_VM_EVENT_COUNTERS
	kfree(vm_buf);
#endif

	return 0;
}

static ssize_t mmonitor_write(struct file *file, const char *buffer, size_t count, loff_t *off)
{
	char ctl;

	if ((!enable) || (BETA_USER != get_logusertype_flag())) {
		printk("write to mmonitor not support\n");
		return -EFAULT;
	}

	if (copy_from_user((&ctl), buffer, sizeof(char)))
		return -EFAULT;

	if (ctl - 'C' == 0)
		mmonitor_clear_events();
	return 1;
}

static int mmonitor_open(struct inode *inode, struct file *file)
{
	return single_open(file, mmonitor_show, NULL);
}

static const struct file_operations mmonitor_file_operations = {
	.owner = THIS_MODULE,
	.open = mmonitor_open,
	.write = mmonitor_write,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int __init mmonitor_init(void)
{
#ifdef CONFIG_PROC_FS
	proc_create(MMONITOR_PROC_FILE, S_IRUGO | S_IWUSR, NULL,
			&mmonitor_file_operations);
#endif
	return 0;
}

module_init(mmonitor_init);
MODULE_LICENSE("GPL");

#endif
