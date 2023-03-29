#define pr_fmt(fmt) "hisi_lowmem: " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/rcupdate.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/swap.h>
#include <linux/fs.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/freezer.h>
#include <linux/ksm.h>
#include <linux/ion.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/version.h>
#include <linux/hisi/page_tracker.h>
#include <log/log_usertype/log-usertype.h>
#include <linux/slub_def.h>

#include "lowmem_killer.h"

#define LMK_PRT_TSK_RSS 10000
#define LMK_INTERVAL 3

/* SERVICE_ADJ(5) * OOM_SCORE_ADJ_MAX / -OOM_DISABLE */
#define LMK_SERVICE_ADJ 500

static unsigned long long last_jiffs;

static const char state_to_char[] = TASK_STATE_TO_CHAR_STR;

static void lowmem_dump(struct work_struct *work);

static DEFINE_MUTEX(lowmem_dump_mutex);
static DECLARE_WORK(lowmem_dbg_wk, lowmem_dump);
static DECLARE_WORK(lowmem_dbg_verbose_wk, lowmem_dump);

static int task_state_char(unsigned long state)
{
	int bit = state ? __ffs(state) + 1 : 0;

	return bit < sizeof(state_to_char) - 1 ? state_to_char[bit] : '?'; /*lint !e574 */
}

static void dump_tasks(bool verbose)
{
	struct task_struct *p;
	struct task_struct *task;
	short tsk_oom_adj = 0;
	unsigned long tsk_nr_ptes = 0;
	char frozen_mark = ' ';

	pr_info("[ pid ]   uid  tgid total_vm    rss nptes  swap   adj s name\n");

	rcu_read_lock();
	for_each_process(p) {
		task = find_lock_task_mm(p);
		if (!task) {
			/*
			 * This is a kthread or all of p's threads have already
			 * detached their mm's.  There's no need to report
			 * them; they can't be oom killed anyway.
			 */
			continue;
		}

		tsk_oom_adj = task->signal->oom_score_adj;
		if (!verbose && tsk_oom_adj &&
		    (tsk_oom_adj <= LMK_SERVICE_ADJ) &&
		    (get_mm_rss(task->mm) + get_mm_counter(task->mm, MM_SWAPENTS) < LMK_PRT_TSK_RSS)) {
			task_unlock(task);
			continue;
		}

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 12, 0)
		tsk_nr_ptes = (unsigned long)task->mm->nr_ptes;
#else
		tsk_nr_ptes = (unsigned long)atomic_long_read(&task->mm->nr_ptes);
#endif
		frozen_mark = frozen(task) ? '*' : ' ';

		pr_info("[%5d] %5d %5d %8lu %6lu %5lu %5lu %5hd %c %s%c\n",
			task->pid, from_kuid(&init_user_ns, task_uid(task)),
			task->tgid, task->mm->total_vm, get_mm_rss(task->mm),
			tsk_nr_ptes,
			get_mm_counter(task->mm, MM_SWAPENTS),
			tsk_oom_adj,
			task_state_char(task->state),
			task->comm,
			frozen_mark); /*lint !e1058*/
		task_unlock(task);
	}
	rcu_read_unlock();
}

static void lowmem_dump(struct work_struct *work)
{
	bool verbose;
	int logusertype = get_logusertype_flag();

	/*
	* for internal debug, we hope print more lowmemory info.
	*/
	if (logusertype == BETA_USER || logusertype == OVERSEA_USER)
		verbose = true;
	else
		verbose = (work == &lowmem_dbg_verbose_wk) ? true : false;

	mutex_lock(&lowmem_dump_mutex);
	show_mem(SHOW_MEM_FILTER_NODES);
	dump_tasks(verbose);
	ksm_show_stats();
	hisi_ion_memory_info(verbose);
	show_slab(verbose);
	if (verbose)
		page_tracker_wake_up();
	mutex_unlock(&lowmem_dump_mutex);
}

void hisi_lowmem_dbg(short oom_score_adj)
{
	unsigned long long jiffs = get_jiffies_64();

	if (oom_score_adj == 0)
		schedule_work(&lowmem_dbg_verbose_wk);
	else if (time_after64(jiffs, (last_jiffs + LMK_INTERVAL * HZ))) {
		last_jiffs = get_jiffies_64();
		schedule_work(&lowmem_dbg_wk);
	}
}

void hisi_lowmem_dbg_timeout(struct task_struct *p, struct task_struct *leader)
{
	struct task_struct *t;

	pr_info("timeout '%s' (%d)\n", leader->comm, leader->pid);
	for_each_thread(p, t) { /*lint !e64 */
		pr_info("  pid=%d tgid=%d %s mm=%d frozen=%d 0x%lx 0x%x\n",
			t->pid, t->tgid, t->comm, t->mm ? 1 : 0,
			frozen(t), t->state, t->flags);
	}
	show_stack(leader, NULL);
}
