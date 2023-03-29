#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <../kernel/sched/sched.h>

#include "frame_info.h"

/*
 * This allows printing both to /proc/sched_debug and
 * to the console
 */
#define SEQ_printf(m, x...)			\
do {						\
	if (m)					\
		seq_printf(m, x);		\
	else					\
		printk(x);			\
} while (0)

static inline void print_frame_info(struct seq_file *m,
		struct related_thread_group *grp)
{
	struct frame_info *frame_info = (struct frame_info *) grp->private_data;

	u64 now = ktime_get_ns();
	u64 frame_end = grp->window_start;

	SEQ_printf(m, "FRAME_INFO : QOS:%u#MARGIN:%d\n",
			frame_info->qos_frame, frame_info->vload_margin);
	SEQ_printf(m, "FRAME_INTERVAL : UPDATE:%ldms#INVALID:%ldms\n",
			grp->freq_update_interval / NSEC_PER_MSEC,
			grp->util_invalid_interval / NSEC_PER_MSEC);
	SEQ_printf(m, "FRAME_TIMESTAMP : timestamp:%llu#now:%llu#delta:%llu\n",
			frame_end, now, now - frame_end);
	SEQ_printf(m, "FRAME_LAST_TIME : %llu/%llu\n",
			(unsigned long long)frame_info_rtg_load(frame_info)->prev_window_exec,
			(unsigned long long)grp->prev_window_time);
	SEQ_printf(m, "FRAME_CLUSTER : %d\n",
			grp->preferred_cluster ? grp->preferred_cluster->id : -1);
}


static const char frame_task_state_to_char[] = TASK_STATE_TO_CHAR_STR;

static inline int frame_task_state_char(unsigned long state)
{
	int bit = state ? __ffs(state) + 1 : 0;

	return bit < sizeof(frame_task_state_to_char) - 1 ?
		frame_task_state_to_char[bit] : '?';
}

static inline void
print_frame_task_header(struct seq_file *m, char *header,
		int nr)
{
	SEQ_printf(m,
		"%s : %d\n"
		"STATE		COMM	   PID	PRIO	CPU\n"
		"---------------------------------------------------------\n",
		header, nr);
}

static inline void
print_frame_task(struct seq_file *m, struct task_struct *p)
{
	SEQ_printf(m, "%5c %15s %5d %5d %5d(%*pbl)\n",
		frame_task_state_char(p->state), p->comm,
		p->pid, p->prio,
		task_cpu(p), cpumask_pr_args(&p->cpus_allowed));
}

static inline void
print_frame_threads(struct seq_file *m,
		struct related_thread_group *grp)
{
	struct task_struct *p = NULL;
	int nr_thread = 0;

	list_for_each_entry(p, &grp->tasks, grp_list) {
		nr_thread++;
	}
	print_frame_task_header(m, "FRAME_THREADS", nr_thread);
	list_for_each_entry(p, &grp->tasks, grp_list) {
		print_frame_task(m, p);
	}
}

static int sched_frame_debug_show(struct seq_file *m, void *v)
{
	struct related_thread_group *grp = NULL;

	grp = lookup_related_thread_group(DEFAULT_RT_FRAME_ID);
	if (!grp) {
		SEQ_printf(m, "IPROVISION RTG none\n");
		return 0;
	}

	raw_spin_lock(&grp->lock);
	if (list_empty(&grp->tasks)) {
		raw_spin_unlock(&grp->lock);
		SEQ_printf(m, "IPROVISION RTG tasklist empty\n");
		return 0;
	}

	print_frame_info(m, grp);
	print_frame_threads(m, grp);
	raw_spin_unlock(&grp->lock);

	return 0;
}

static int sched_frame_debug_release(struct inode *inode, struct file *file)
{
	seq_release(inode, file);
	return 0;
}

static int sched_frame_debug_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, sched_frame_debug_show, NULL);
}

static const struct file_operations sched_frame_debug_fops = {
	.open		= sched_frame_debug_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= sched_frame_debug_release,
};

static int __init init_sched_debug_procfs(void)
{
	struct proc_dir_entry *pe;

	pe = proc_create("sched_frame_debug",
		0444, NULL, &sched_frame_debug_fops);
	if (!pe)
		return -ENOMEM;
	return 0;
}
late_initcall(init_sched_debug_procfs);
