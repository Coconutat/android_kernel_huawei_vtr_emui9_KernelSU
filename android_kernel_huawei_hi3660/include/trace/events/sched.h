#undef TRACE_SYSTEM
#define TRACE_SYSTEM sched

#if !defined(_TRACE_SCHED_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SCHED_H

#include <linux/sched.h>
#include <linux/tracepoint.h>
#include <linux/binfmts.h>

/*
 * Tracepoint for calling kthread_stop, performed to end a kthread:
 */
TRACE_EVENT(sched_kthread_stop,

	TP_PROTO(struct task_struct *t),

	TP_ARGS(t),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, t->comm, TASK_COMM_LEN);
		__entry->pid	= t->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);

/*
 * Tracepoint for the return value of the kthread stopping:
 */
TRACE_EVENT(sched_kthread_stop_ret,

	TP_PROTO(int ret),

	TP_ARGS(ret),

	TP_STRUCT__entry(
		__field(	int,	ret	)
	),

	TP_fast_assign(
		__entry->ret	= ret;
	),

	TP_printk("ret=%d", __entry->ret)
);

/*
 * Tracepoint for waking up a task:
 */
DECLARE_EVENT_CLASS(sched_wakeup_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(__perf_task(p)),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	success			)
		__field(	int,	target_cpu		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->success	= 1; /* rudiment, kill when possible */
		__entry->target_cpu	= task_cpu(p);
	),

	TP_printk("comm=%s pid=%d prio=%d target_cpu=%03d",
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->target_cpu)
);

/*
 * Tracepoint called when waking a task; this tracepoint is guaranteed to be
 * called from the waking context.
 */
DEFINE_EVENT(sched_wakeup_template, sched_waking,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint called when the task is actually woken; p->state == TASK_RUNNNG.
 * It it not always called from the waking context.
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waking up a new task:
 */
DEFINE_EVENT(sched_wakeup_template, sched_wakeup_new,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

#ifdef CREATE_TRACE_POINTS
static inline long __trace_sched_switch_state(bool preempt, struct task_struct *p)
{
#ifdef CONFIG_SCHED_DEBUG
	BUG_ON(p != current);
#endif /* CONFIG_SCHED_DEBUG */

	/*
	 * Preemption ignores task state, therefore preempted tasks are always
	 * RUNNING (we will not have dequeued if state != RUNNING).
	 */
	return preempt ? TASK_RUNNING | TASK_STATE_MAX : p->state;
}
#endif /* CREATE_TRACE_POINTS */

#ifdef CONFIG_HW_VIP_THREAD
/*
 * Tracepoint for sched vip
 */
DECLARE_EVENT_CLASS(sched_vip_template,

	TP_PROTO(struct task_struct *p, char *msg),

	TP_ARGS(__perf_task(p), msg),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__array(	char,	msg, 	VIP_MSG_LEN	)
		__field(	int,	target_cpu		)
		__field(    u64,    dynamic_vip)
		__field(    int,    vip_depth)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		memcpy(__entry->msg, msg, min(VIP_MSG_LEN, strlen(msg)+1));
		__entry->target_cpu	= task_cpu(p);
		__entry->dynamic_vip   = atomic64_read(&p->dynamic_vip);
		__entry->vip_depth     = p->vip_depth;
	),

	TP_printk("comm=%s pid=%d prio=%d msg=%s target_cpu=%03d dynamic_vip:%llx vip_depth:%d",
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->msg, __entry->target_cpu, __entry->dynamic_vip, __entry->vip_depth)
);

DEFINE_EVENT(sched_vip_template, sched_vip_queue_op,
         TP_PROTO(struct task_struct *p, char *msg),
	     TP_ARGS(p, msg));

DEFINE_EVENT(sched_vip_template, sched_vip_sched,
         TP_PROTO(struct task_struct *p, char *msg),
	     TP_ARGS(p, msg));

#endif

/*
 * Tracepoint for task switches, performed by the scheduler:
 */
TRACE_EVENT(sched_switch,

	TP_PROTO(bool preempt,
		 struct task_struct *prev,
		 struct task_struct *next),

	TP_ARGS(preempt, prev, next),

	TP_STRUCT__entry(
		__array(	char,	prev_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	prev_pid			)
		__field(	int,	prev_prio			)
		__field(	long,	prev_state			)
		__array(	char,	next_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	next_pid			)
		__field(	int,	next_prio			)
	),

	TP_fast_assign(
		memcpy(__entry->next_comm, next->comm, TASK_COMM_LEN);
		__entry->prev_pid	= prev->pid;
		__entry->prev_prio	= prev->prio;
		__entry->prev_state	= __trace_sched_switch_state(preempt, prev);
		memcpy(__entry->prev_comm, prev->comm, TASK_COMM_LEN);
		__entry->next_pid	= next->pid;
		__entry->next_prio	= next->prio;
	),

	TP_printk("prev_comm=%s prev_pid=%d prev_prio=%d prev_state=%s%s ==> next_comm=%s next_pid=%d next_prio=%d",
		__entry->prev_comm, __entry->prev_pid, __entry->prev_prio,
		__entry->prev_state & (TASK_STATE_MAX-1) ?
		  __print_flags(__entry->prev_state & (TASK_STATE_MAX-1), "|",
				{ 1, "S"} , { 2, "D" }, { 4, "T" }, { 8, "t" },
				{ 16, "Z" }, { 32, "X" }, { 64, "x" },
				{ 128, "K" }, { 256, "W" }, { 512, "P" },
				{ 1024, "N" }) : "R",
		__entry->prev_state & TASK_STATE_MAX ? "+" : "",
		__entry->next_comm, __entry->next_pid, __entry->next_prio)
);

/*
 * Tracepoint for a task being migrated:
 */
TRACE_EVENT(sched_migrate_task,

	TP_PROTO(struct task_struct *p, int dest_cpu),

	TP_ARGS(p, dest_cpu),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
		__field(	int,	orig_cpu		)
		__field(	int,	dest_cpu		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
		__entry->orig_cpu	= task_cpu(p);
		__entry->dest_cpu	= dest_cpu;
	),

	TP_printk("comm=%s pid=%d prio=%d orig_cpu=%d dest_cpu=%d",
		  __entry->comm, __entry->pid, __entry->prio,
		  __entry->orig_cpu, __entry->dest_cpu)
);

/*
 * Tracepoint for a CPU going offline/online:
 */
TRACE_EVENT(sched_cpu_hotplug,

	TP_PROTO(int affected_cpu, int error, int status),

	TP_ARGS(affected_cpu, error, status),

	TP_STRUCT__entry(
		__field(	int,	affected_cpu		)
		__field(	int,	error			)
		__field(	int,	status			)
	),

	TP_fast_assign(
		__entry->affected_cpu	= affected_cpu;
		__entry->error		= error;
		__entry->status		= status;
	),

	TP_printk("cpu %d %s error=%d", __entry->affected_cpu,
		__entry->status ? "online" : "offline", __entry->error)
);

DECLARE_EVENT_CLASS(sched_process_template,

	TP_PROTO(struct task_struct *p),

	TP_ARGS(p),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid		= p->pid;
		__entry->prio		= p->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for freeing a task:
 */
DEFINE_EVENT(sched_process_template, sched_process_free,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));


/*
 * Tracepoint for a task exiting:
 */
DEFINE_EVENT(sched_process_template, sched_process_exit,
	     TP_PROTO(struct task_struct *p),
	     TP_ARGS(p));

/*
 * Tracepoint for waiting on task to unschedule:
 */
DEFINE_EVENT(sched_process_template, sched_wait_task,
	TP_PROTO(struct task_struct *p),
	TP_ARGS(p));

/*
 * Tracepoint for a waiting task:
 */
TRACE_EVENT(sched_process_wait,

	TP_PROTO(struct pid *pid),

	TP_ARGS(pid),

	TP_STRUCT__entry(
		__array(	char,	comm,	TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	int,	prio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, current->comm, TASK_COMM_LEN);
		__entry->pid		= pid_nr(pid);
		__entry->prio		= current->prio;
	),

	TP_printk("comm=%s pid=%d prio=%d",
		  __entry->comm, __entry->pid, __entry->prio)
);

/*
 * Tracepoint for do_fork:
 */
TRACE_EVENT(sched_process_fork,

	TP_PROTO(struct task_struct *parent, struct task_struct *child),

	TP_ARGS(parent, child),

	TP_STRUCT__entry(
		__array(	char,	parent_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	parent_pid			)
		__array(	char,	child_comm,	TASK_COMM_LEN	)
		__field(	pid_t,	child_pid			)
	),

	TP_fast_assign(
		memcpy(__entry->parent_comm, parent->comm, TASK_COMM_LEN);
		__entry->parent_pid	= parent->pid;
		memcpy(__entry->child_comm, child->comm, TASK_COMM_LEN);
		__entry->child_pid	= child->pid;
	),

	TP_printk("comm=%s pid=%d child_comm=%s child_pid=%d",
		__entry->parent_comm, __entry->parent_pid,
		__entry->child_comm, __entry->child_pid)
);

/*
 * Tracepoint for exec:
 */
TRACE_EVENT(sched_process_exec,

	TP_PROTO(struct task_struct *p, pid_t old_pid,
		 struct linux_binprm *bprm),

	TP_ARGS(p, old_pid, bprm),

	TP_STRUCT__entry(
		__string(	filename,	bprm->filename	)
		__field(	pid_t,		pid		)
		__field(	pid_t,		old_pid		)
	),

	TP_fast_assign(
		__assign_str(filename, bprm->filename);
		__entry->pid		= p->pid;
		__entry->old_pid	= old_pid;
	),

	TP_printk("filename=%s pid=%d old_pid=%d", __get_str(filename),
		  __entry->pid, __entry->old_pid)
);

/*
 * XXX the below sched_stat tracepoints only apply to SCHED_OTHER/BATCH/IDLE
 *     adding sched_stat support to SCHED_FIFO/RR would be welcome.
 */
DECLARE_EVENT_CLASS(sched_stat_template,

	TP_PROTO(struct task_struct *tsk, u64 delay),

	TP_ARGS(__perf_task(tsk), __perf_count(delay)),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	delay			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid	= tsk->pid;
		__entry->delay	= delay;
	),

	TP_printk("comm=%s pid=%d delay=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->delay)
);


/*
 * Tracepoint for accounting wait time (time the task is runnable
 * but not actually running due to scheduler contention).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_wait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting sleep time (time the task is not runnable,
 * including iowait, see below).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_sleep,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting iowait time (time the task is not runnable
 * due to waiting on IO to complete).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_iowait,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for accounting blocked time (time the task is in uninterruptible).
 */
DEFINE_EVENT(sched_stat_template, sched_stat_blocked,
	     TP_PROTO(struct task_struct *tsk, u64 delay),
	     TP_ARGS(tsk, delay));

/*
 * Tracepoint for recording the cause of uninterruptible sleep.
 */
TRACE_EVENT(sched_blocked_reason,

	TP_PROTO(struct task_struct *tsk),

	TP_ARGS(tsk),

	TP_STRUCT__entry(
		__field( pid_t,	pid	)
		__field( void*, caller	)
		__field( bool, io_wait	)
	),

	TP_fast_assign(
		__entry->pid	= tsk->pid;
		__entry->caller = (void*)get_wchan(tsk);
		__entry->io_wait = tsk->in_iowait;
	),

	TP_printk("pid=%d iowait=%d caller=%pS", __entry->pid, __entry->io_wait, __entry->caller)
);

/*
 * Tracepoint for accounting runtime (time the task is executing
 * on a CPU).
 */
DECLARE_EVENT_CLASS(sched_stat_runtime,

	TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),

	TP_ARGS(tsk, __perf_count(runtime), vruntime),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( u64,	runtime			)
		__field( u64,	vruntime			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->runtime	= runtime;
		__entry->vruntime	= vruntime;
	),

	TP_printk("comm=%s pid=%d runtime=%Lu [ns] vruntime=%Lu [ns]",
			__entry->comm, __entry->pid,
			(unsigned long long)__entry->runtime,
			(unsigned long long)__entry->vruntime)
);

DEFINE_EVENT(sched_stat_runtime, sched_stat_runtime,
	     TP_PROTO(struct task_struct *tsk, u64 runtime, u64 vruntime),
	     TP_ARGS(tsk, runtime, vruntime));

/*
 * Tracepoint for showing priority inheritance modifying a tasks
 * priority.
 */
TRACE_EVENT(sched_pi_setprio,

	TP_PROTO(struct task_struct *tsk, int newprio),

	TP_ARGS(tsk, newprio),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( int,	oldprio			)
		__field( int,	newprio			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->oldprio	= tsk->prio;
		__entry->newprio	= newprio;
	),

	TP_printk("comm=%s pid=%d oldprio=%d newprio=%d",
			__entry->comm, __entry->pid,
			__entry->oldprio, __entry->newprio)
);

#ifdef CONFIG_DETECT_HUNG_TASK
TRACE_EVENT(sched_process_hang,
	TP_PROTO(struct task_struct *tsk),
	TP_ARGS(tsk),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid = tsk->pid;
	),

	TP_printk("comm=%s pid=%d", __entry->comm, __entry->pid)
);
#endif /* CONFIG_DETECT_HUNG_TASK */

DECLARE_EVENT_CLASS(sched_move_task_template,

	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	pid			)
		__field( pid_t,	tgid			)
		__field( pid_t,	ngid			)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->pid		= task_pid_nr(tsk);
		__entry->tgid		= task_tgid_nr(tsk);
		__entry->ngid		= task_numa_group_id(tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= cpu_to_node(dst_cpu);
	),

	TP_printk("pid=%d tgid=%d ngid=%d src_cpu=%d src_nid=%d dst_cpu=%d dst_nid=%d",
			__entry->pid, __entry->tgid, __entry->ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_cpu, __entry->dst_nid)
);

/*
 * Tracks migration of tasks from one runqueue to another. Can be used to
 * detect if automatic NUMA balancing is bouncing between nodes
 */
DEFINE_EVENT(sched_move_task_template, sched_move_numa,
	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu)
);

DEFINE_EVENT(sched_move_task_template, sched_stick_numa,
	TP_PROTO(struct task_struct *tsk, int src_cpu, int dst_cpu),

	TP_ARGS(tsk, src_cpu, dst_cpu)
);

TRACE_EVENT(sched_swap_numa,

	TP_PROTO(struct task_struct *src_tsk, int src_cpu,
		 struct task_struct *dst_tsk, int dst_cpu),

	TP_ARGS(src_tsk, src_cpu, dst_tsk, dst_cpu),

	TP_STRUCT__entry(
		__field( pid_t,	src_pid			)
		__field( pid_t,	src_tgid		)
		__field( pid_t,	src_ngid		)
		__field( int,	src_cpu			)
		__field( int,	src_nid			)
		__field( pid_t,	dst_pid			)
		__field( pid_t,	dst_tgid		)
		__field( pid_t,	dst_ngid		)
		__field( int,	dst_cpu			)
		__field( int,	dst_nid			)
	),

	TP_fast_assign(
		__entry->src_pid	= task_pid_nr(src_tsk);
		__entry->src_tgid	= task_tgid_nr(src_tsk);
		__entry->src_ngid	= task_numa_group_id(src_tsk);
		__entry->src_cpu	= src_cpu;
		__entry->src_nid	= cpu_to_node(src_cpu);
		__entry->dst_pid	= task_pid_nr(dst_tsk);
		__entry->dst_tgid	= task_tgid_nr(dst_tsk);
		__entry->dst_ngid	= task_numa_group_id(dst_tsk);
		__entry->dst_cpu	= dst_cpu;
		__entry->dst_nid	= cpu_to_node(dst_cpu);
	),

	TP_printk("src_pid=%d src_tgid=%d src_ngid=%d src_cpu=%d src_nid=%d dst_pid=%d dst_tgid=%d dst_ngid=%d dst_cpu=%d dst_nid=%d",
			__entry->src_pid, __entry->src_tgid, __entry->src_ngid,
			__entry->src_cpu, __entry->src_nid,
			__entry->dst_pid, __entry->dst_tgid, __entry->dst_ngid,
			__entry->dst_cpu, __entry->dst_nid)
);

/*
 * Tracepoint for waking a polling cpu without an IPI.
 */
TRACE_EVENT(sched_wake_idle_without_ipi,

	TP_PROTO(int cpu),

	TP_ARGS(cpu),

	TP_STRUCT__entry(
		__field(	int,	cpu	)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
	),

	TP_printk("cpu=%d", __entry->cpu)
);

#ifdef CONFIG_HISI_CORE_CTRL
TRACE_EVENT(core_ctl_get_nr_running_avg,/* [false alarm]:fortify */

	TP_PROTO(const struct cpumask *cpus, unsigned int avg,
		 unsigned int big_avg, unsigned int iowait_avg,
		 unsigned int nr_max),

	TP_ARGS(cpus, avg, big_avg, iowait_avg, nr_max),

	TP_STRUCT__entry(
		__bitmask( cpumask,	num_possible_cpus()	)
		__field( unsigned int,	avg			)
		__field( unsigned int,	big_avg			)
		__field( unsigned int,	iowait_avg		)
		__field( unsigned int,	nr_max			)
	),

	TP_fast_assign(
		__assign_bitmask(cpumask, cpumask_bits(cpus),
				 num_possible_cpus());
		__entry->avg		= avg;
		__entry->big_avg	= big_avg;
		__entry->iowait_avg	= iowait_avg;
		__entry->nr_max		= nr_max;
	),

	TP_printk("cpu=%s avg=%u big_avg=%u iowait_avg=%u max_nr=%u",
		  __get_bitmask(cpumask), __entry->avg, __entry->big_avg,
		  __entry->iowait_avg, __entry->nr_max)
);/* [false alarm]:fortify */

TRACE_EVENT(core_ctl_eval_need,/* [false alarm]:fortify */

	TP_PROTO(const struct cpumask *cpus, unsigned int nrrun,
		 unsigned int busy_cpus, unsigned int active_cpus,
		 unsigned int old_need, unsigned int new_need,
		 s64 elapsed, unsigned int updated),
	TP_ARGS(cpus, nrrun, busy_cpus, active_cpus,
		old_need, new_need, elapsed, updated),
	TP_STRUCT__entry(
		__bitmask( cpumask, num_possible_cpus()	)
		__field( u32, nrrun			)
		__field( u32, busy_cpus			)
		__field( u32, active_cpus		)
		__field( u32, old_need			)
		__field( u32, new_need			)
		__field( s64, elapsed			)
		__field( u32, updated			)
	),
	TP_fast_assign(
		__assign_bitmask(cpumask, cpumask_bits(cpus),
				 num_possible_cpus());
		__entry->nrrun		= nrrun;
		__entry->busy_cpus	= busy_cpus;
		__entry->active_cpus	= active_cpus;
		__entry->old_need	= old_need;
		__entry->new_need	= new_need;
		__entry->elapsed	= elapsed;
		__entry->updated	= updated;
	),
	TP_printk("cpu=%s, nrrun:%u, busy:%u, active:%u,"
		  " old_need=%u, new_need=%u, elapsed =%lld, updated=%u",
		  __get_bitmask(cpumask), __entry->nrrun, __entry->busy_cpus,
		  __entry->active_cpus, __entry->old_need, __entry->new_need,
		  __entry->elapsed, __entry->updated)
);/* [false alarm]:fortify */

TRACE_EVENT(core_ctl_update_busy,/* [false alarm]:fortify */

	TP_PROTO(unsigned int cpu, unsigned int load,
		 unsigned int old_is_busy, unsigned int is_busy),
	TP_ARGS(cpu, load, old_is_busy, is_busy),
	TP_STRUCT__entry(
		__field( u32, cpu		)
		__field( u32, load		)
		__field( u32, old_is_busy	)
		__field( u32, is_busy		)
	),
	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->load		= load;
		__entry->old_is_busy	= old_is_busy;
		__entry->is_busy	= is_busy;
	),
	TP_printk("cpu=%u, load=%u, old_is_busy=%u, new_is_busy=%u",
		  __entry->cpu, __entry->load, __entry->old_is_busy,
		  __entry->is_busy)
);/* [false alarm]:fortify */

TRACE_EVENT(core_ctl_set_boost,/* [false alarm]:fortify */

	TP_PROTO(u32 refcount),
	TP_ARGS(refcount),
	TP_STRUCT__entry(
		__field( u32, refcount	)
	),
	TP_fast_assign(
		__entry->refcount	= refcount;
	),
	TP_printk("boost=%u", __entry->refcount)
);/* [false alarm]:fortify */
#endif /* CONFIG_HISI_CORE_CTRL */

#ifdef CONFIG_HISI_CPU_ISOLATION
/*
 * sched_isolate - called when cores are isolated/unisolated
 *
 * @time: amount of time in us it took to isolate/unisolate
 * @isolate: 1 if isolating, 0 if unisolating
 *
 */
TRACE_EVENT(sched_isolate,/* [false alarm]:fortify */

	TP_PROTO(unsigned int requested_cpu, unsigned int isolated_cpus,
		 u64 start_time, unsigned char isolate),

	TP_ARGS(requested_cpu, isolated_cpus, start_time, isolate),

	TP_STRUCT__entry(
		__field( u32, requested_cpu	)
		__field( u32, isolated_cpus	)
		__field( u32, time		)
		__field( unsigned char, isolate	)
	),

	TP_fast_assign(
		__entry->requested_cpu	= requested_cpu;
		__entry->isolated_cpus	= isolated_cpus;
		__entry->time		= div64_u64(sched_clock() - start_time, 1000);
		__entry->isolate	= isolate;
	),

	TP_printk("iso cpu=%u cpus=0x%x time=%u us isolated=%d",
		  __entry->requested_cpu, __entry->isolated_cpus,
		  __entry->time, __entry->isolate)
);/* [false alarm]:fortify */
#endif /* CONFIG_HISI_CPU_ISOLATION */

TRACE_EVENT(sched_contrib_scale_f,

	TP_PROTO(int cpu, unsigned long freq_scale_factor,
		 unsigned long cpu_scale_factor),

	TP_ARGS(cpu, freq_scale_factor, cpu_scale_factor),

	TP_STRUCT__entry(
		__field(int, cpu)
		__field(unsigned long, freq_scale_factor)
		__field(unsigned long, cpu_scale_factor)
	),

	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->freq_scale_factor = freq_scale_factor;
		__entry->cpu_scale_factor = cpu_scale_factor;
	),

	TP_printk("cpu=%d freq_scale_factor=%lu cpu_scale_factor=%lu",
		  __entry->cpu, __entry->freq_scale_factor,
		  __entry->cpu_scale_factor)
);

#ifdef CONFIG_SMP

#ifdef CONFIG_SCHED_WALT
extern unsigned int sysctl_sched_use_walt_cpu_util;
extern unsigned int sysctl_sched_use_walt_task_util;
#ifdef CONFIG_SCHED_HISI_WALT_WINDOW_SIZE_TUNABLE
extern unsigned int walt_ravg_window;
extern bool walt_disabled;
#else
extern const unsigned int walt_ravg_window;
extern const bool walt_disabled;
#endif
#endif

/*
 * Tracepoint for accounting sched averages for tasks.
 */
TRACE_EVENT(sched_load_avg_task,

	TP_PROTO(struct task_struct *tsk, struct sched_avg *avg, void *_ravg),

	TP_ARGS(tsk, avg, _ravg),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN		)
		__field( pid_t,	pid				)
		__field( int,	cpu				)
		__field( unsigned long,	load_avg		)
		__field( unsigned long,	util_avg		)
		__field( unsigned long,	util_avg_pelt		)
		__field( u32,		util_avg_walt		)
		__field( u64,		load_sum		)
		__field( u32,		util_sum		)
		__field( u32,		period_contrib		)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid			= tsk->pid;
		__entry->cpu			= task_cpu(tsk);
		__entry->load_avg		= avg->load_avg;
		__entry->util_avg		= avg->util_avg;
		__entry->load_sum		= avg->load_sum;
		__entry->util_sum		= avg->util_sum;
		__entry->period_contrib		= avg->period_contrib;
		__entry->util_avg_pelt  = avg->util_avg;
		__entry->util_avg_walt  = 0;
#ifdef CONFIG_SCHED_WALT
		__entry->util_avg_walt = ((struct ravg*)_ravg)->demand /
					 (walt_ravg_window >> SCHED_CAPACITY_SHIFT);
		if (!walt_disabled && sysctl_sched_use_walt_task_util)
			__entry->util_avg = __entry->util_avg_walt;
#endif
	),
	TP_printk("comm=%s pid=%d cpu=%d load_avg=%lu util_avg=%lu "
			"util_avg_pelt=%lu util_avg_walt=%u load_sum=%llu"
		  " util_sum=%u period_contrib=%u",
		  __entry->comm,
		  __entry->pid,
		  __entry->cpu,
		  __entry->load_avg,
		  __entry->util_avg,
		  __entry->util_avg_pelt,
		  __entry->util_avg_walt,
		  (u64)__entry->load_sum,
		  (u32)__entry->util_sum,
		  (u32)__entry->period_contrib)
);

/*
 * Tracepoint for accounting sched averages for cpus.
 */
TRACE_EVENT(sched_load_avg_cpu,

	TP_PROTO(int cpu, struct cfs_rq *cfs_rq),

	TP_ARGS(cpu, cfs_rq),

	TP_STRUCT__entry(
		__field( int,	cpu				)
		__field( unsigned long,	load_avg		)
		__field( unsigned long,	util_avg		)
		__field( unsigned long,	util_avg_pelt		)
		__field( u32,		util_avg_walt		)
	),

	TP_fast_assign(
		__entry->cpu			= cpu;
		__entry->load_avg		= cfs_rq->avg.load_avg;
		__entry->util_avg		= cfs_rq->avg.util_avg;
		__entry->util_avg_pelt	= cfs_rq->avg.util_avg;
		__entry->util_avg_walt	= 0;
#ifdef CONFIG_SCHED_WALT
		__entry->util_avg_walt = div64_ul(cpu_rq(cpu)->prev_runnable_sum,
					 walt_ravg_window >> SCHED_CAPACITY_SHIFT);
		if (!walt_disabled && sysctl_sched_use_walt_cpu_util)
			__entry->util_avg		= __entry->util_avg_walt;
#endif
	),

	TP_printk("cpu=%d load_avg=%lu util_avg=%lu "
			  "util_avg_pelt=%lu util_avg_walt=%u",
		  __entry->cpu, __entry->load_avg, __entry->util_avg,
		  __entry->util_avg_pelt, __entry->util_avg_walt)
);

/*
 * Tracepoint for eas attribute store
 */
TRACE_EVENT(eas_attr_store,

	TP_PROTO(const char *name, int value),

	TP_ARGS(name, value),

	TP_STRUCT__entry(
		__array( char,	name,	TASK_COMM_LEN	)
		__field( int,		value		)
	),

	TP_fast_assign(
		memcpy(__entry->name, name, TASK_COMM_LEN);
		__entry->value		= value;
	),

	TP_printk("name=%s value=%d", __entry->name, __entry->value)
);

/*
 * Tracepoint for schedtune_boost
 */
TRACE_EVENT(sched_tune_boost,

	TP_PROTO(char *name, int boost),

	TP_ARGS(name, boost),

	TP_STRUCT__entry(
		__array( char,	name,	TASK_COMM_LEN	)
		__field( int,		boost		)
	),

	TP_fast_assign(
		memcpy(__entry->name, name, TASK_COMM_LEN);
		__entry->boost		= boost;
	),

	TP_printk("name=%s boost=%d", __entry->name, __entry->boost)
);

/*
 * Tracepoint for sched_tune_config settings
 */
TRACE_EVENT(sched_tune_config,

	TP_PROTO(int boost, int pb_nrg_gain, int pb_cap_gain, int pc_nrg_gain, int pc_cap_gain),

	TP_ARGS(boost, pb_nrg_gain, pb_cap_gain, pc_nrg_gain, pc_cap_gain),

	TP_STRUCT__entry(
		__field( int,	boost		)
		__field( int,	pb_nrg_gain	)
		__field( int,	pb_cap_gain	)
		__field( int,	pc_nrg_gain	)
		__field( int,	pc_cap_gain	)
	),

	TP_fast_assign(
		__entry->boost 	= boost;
		__entry->pb_nrg_gain	= pb_nrg_gain;
		__entry->pb_cap_gain	= pb_cap_gain;
		__entry->pc_nrg_gain	= pc_nrg_gain;
		__entry->pc_cap_gain	= pc_cap_gain;
	),

	TP_printk("boost=%d "
			"pb_nrg_gain=%d pb_cap_gain=%d "
			"pc_nrg_gain=%d pc_cap_gain=%d",
		__entry->boost,
		__entry->pb_nrg_gain, __entry->pb_cap_gain,
		__entry->pc_nrg_gain, __entry->pc_cap_gain)
);

/*
 * Tracepoint for accounting CPU  boosted utilization
 */
TRACE_EVENT(sched_boost_cpu,

	TP_PROTO(int cpu, unsigned long util, long margin),

	TP_ARGS(cpu, util, margin),

	TP_STRUCT__entry(
		__field( int,		cpu			)
		__field( unsigned long,	util			)
		__field(long,		margin			)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
		__entry->util	= util;
		__entry->margin	= margin;
	),

	TP_printk("cpu=%d util=%lu margin=%ld",
		  __entry->cpu,
		  __entry->util,
		  __entry->margin)
);

/*
 * Tracepoint for schedtune_tasks_update
 */
TRACE_EVENT(sched_tune_tasks_update,

	TP_PROTO(struct task_struct *tsk, int cpu, int tasks, int idx,
		int boost, int max_boost),

	TP_ARGS(tsk, cpu, tasks, idx, boost, max_boost),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,		pid		)
		__field( int,		cpu		)
		__field( int,		tasks		)
		__field( int,		idx		)
		__field( int,		boost		)
		__field( int,		max_boost	)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->cpu 		= cpu;
		__entry->tasks		= tasks;
		__entry->idx 		= idx;
		__entry->boost		= boost;
		__entry->max_boost	= max_boost;
	),

	TP_printk("pid=%d comm=%s "
			"cpu=%d tasks=%d idx=%d boost=%d max_boost=%d",
		__entry->pid, __entry->comm,
		__entry->cpu, __entry->tasks, __entry->idx,
		__entry->boost, __entry->max_boost)
);

/*
 * Tracepoint for schedtune_boostgroup_update
 */
TRACE_EVENT(sched_tune_boostgroup_update,

	TP_PROTO(int cpu, int variation, int max_boost),

	TP_ARGS(cpu, variation, max_boost),

	TP_STRUCT__entry(
		__field( int,	cpu		)
		__field( int,	variation	)
		__field( int,	max_boost	)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->variation	= variation;
		__entry->max_boost	= max_boost;
	),

	TP_printk("cpu=%d variation=%d max_boost=%d",
		__entry->cpu, __entry->variation, __entry->max_boost)
);

#ifdef CONFIG_HISI_CPU_FREQ_GOV_SCHEDUTIL
/*
 * Tracepoint for schedtune_freqboost
 */
TRACE_EVENT(sched_tune_freqboost,

	TP_PROTO(const char *name, int freq_boost),

	TP_ARGS(name, freq_boost),

	TP_STRUCT__entry(
		__array( char,	name,	TASK_COMM_LEN	)
		__field( int,		freq_boost	)
	),

	TP_fast_assign(
		memcpy(__entry->name, name, TASK_COMM_LEN);
		__entry->freq_boost	= freq_boost;
	),

	TP_printk("name=%s freq_boost=%d", __entry->name, __entry->freq_boost)
);

/*
 * Tracepoint for schedtune_freqboostgroup_update
 */
TRACE_EVENT(sched_tune_freqboostgroup_update,

	TP_PROTO(int cpu, int variation, int max_boost),

	TP_ARGS(cpu, variation, max_boost),

	TP_STRUCT__entry(
		__field( int,	cpu		)
		__field( int,	variation	)
		__field( int,	max_boost	)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->variation	= variation;
		__entry->max_boost	= max_boost;
	),

	TP_printk("cpu=%d variation=%d max_freq_boost=%d",
		__entry->cpu, __entry->variation, __entry->max_boost)
);
#endif

/*
 * Tracepoint for accounting task boosted utilization
 */
TRACE_EVENT(sched_boost_task,

	TP_PROTO(struct task_struct *tsk, unsigned long util, long margin),

	TP_ARGS(tsk, util, margin),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN		)
		__field( pid_t,		pid			)
		__field( unsigned long,	util			)
		__field( long,		margin			)

	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid	= tsk->pid;
		__entry->util	= util;
		__entry->margin	= margin;
	),

	TP_printk("comm=%s pid=%d util=%lu margin=%ld",
		  __entry->comm, __entry->pid,
		  __entry->util,
		  __entry->margin)
);

/*
 * Tracepoint for find_best_target
 */
TRACE_EVENT(sched_find_best_target,

	TP_PROTO(struct task_struct *tsk, bool prefer_idle,
		unsigned long min_util, int start_cpu,
		int best_idle, int best_active, int target),

	TP_ARGS(tsk, prefer_idle, min_util, start_cpu,
		best_idle, best_active, target),

	TP_STRUCT__entry(
		__array( char,	comm,	TASK_COMM_LEN	)
		__field( pid_t,	pid			)
		__field( unsigned long,	min_util	)
		__field( bool,	prefer_idle		)
		__field( int,	start_cpu		)
		__field( int,	best_idle		)
		__field( int,	best_active		)
		__field( int,	target			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, tsk->comm, TASK_COMM_LEN);
		__entry->pid		= tsk->pid;
		__entry->min_util	= min_util;
		__entry->prefer_idle	= prefer_idle;
		__entry->start_cpu 	= start_cpu;
		__entry->best_idle	= best_idle;
		__entry->best_active	= best_active;
		__entry->target		= target;
	),

	TP_printk("pid=%d comm=%s prefer_idle=%d start_cpu=%d "
		  "best_idle=%d best_active=%d target=%d",
		__entry->pid, __entry->comm,
		__entry->prefer_idle, __entry->start_cpu,
		__entry->best_idle, __entry->best_active,
		__entry->target)
);

/*
 * Tracepoint for schedtune_tasks_update
 */
TRACE_EVENT(sched_tune_filter,

	TP_PROTO(int nrg_delta, int cap_delta,
		 int nrg_gain,  int cap_gain,
		 int payoff, int region),

	TP_ARGS(nrg_delta, cap_delta, nrg_gain, cap_gain, payoff, region),

	TP_STRUCT__entry(
		__field( int,	nrg_delta	)
		__field( int,	cap_delta	)
		__field( int,	nrg_gain	)
		__field( int,	cap_gain	)
		__field( int,	payoff		)
		__field( int,	region		)
	),

	TP_fast_assign(
		__entry->nrg_delta	= nrg_delta;
		__entry->cap_delta	= cap_delta;
		__entry->nrg_gain	= nrg_gain;
		__entry->cap_gain	= cap_gain;
		__entry->payoff		= payoff;
		__entry->region		= region;
	),

	TP_printk("nrg_delta=%d cap_delta=%d nrg_gain=%d cap_gain=%d payoff=%d region=%d",
		__entry->nrg_delta, __entry->cap_delta,
		__entry->nrg_gain, __entry->cap_gain,
		__entry->payoff, __entry->region)
);

/*
 * Tracepoint for system overutilized flag
 */
TRACE_EVENT(sched_overutilized,

	TP_PROTO(bool overutilized),

	TP_ARGS(overutilized),

	TP_STRUCT__entry(
		__field( bool,	overutilized	)
	),

	TP_fast_assign(
		__entry->overutilized	= overutilized;
	),

	TP_printk("overutilized=%d",
		__entry->overutilized ? 1 : 0)
);

/*
 * Tracepoint for sched group energy
 */
TRACE_EVENT(sched_group_energy,

	TP_PROTO(int cpu, const struct cpumask *cpus,
		 int idle_idx, int cap_idx, unsigned long util,
		 int busy_energy, int idle_energy),

	TP_ARGS(cpu, cpus, idle_idx, cap_idx, util, busy_energy, idle_energy),

	TP_STRUCT__entry(
		__field( int,	cpu	)
		__bitmask(cpumask, num_possible_cpus())
		__field( int,	idle_idx	)
		__field( int,	cap_idx	)
		__field( long,	group_util	)
		__field( long,	busy_energy	)
		__field( long,	idle_energy	)
	),

	TP_fast_assign(
		__entry->cpu	= cpu;
		__assign_bitmask(cpumask, cpumask_bits(cpus),
				num_possible_cpus());
		__entry->idle_idx	= idle_idx;
		__entry->cap_idx	= cap_idx;
		__entry->group_util	= util;
		__entry->busy_energy	= busy_energy;
		__entry->idle_energy	= idle_energy;
	),

	TP_printk("cpu=%d sg_cpus=%s idle_idx=%d cap_idx=%d group_util=%lu sg_busy_energy=%d sg_idle_energy=%d",
		__entry->cpu, __get_bitmask(cpumask), __entry->idle_idx,
		__entry->cap_idx, __entry->group_util, __entry->busy_energy, __entry->idle_energy)
);

/*
 * Tracepoint for sched_setaffinity
 */
TRACE_EVENT(sched_set_affinity,

	TP_PROTO(struct task_struct *p, const struct cpumask *mask),

	TP_ARGS(p, mask),

	TP_STRUCT__entry(
		__array(   char,	comm,	TASK_COMM_LEN	)
		__field(   pid_t,	pid			)
		__bitmask( cpus,	num_possible_cpus()	)
	),

	TP_fast_assign(
		__entry->pid = p->pid;
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__assign_bitmask(cpus, cpumask_bits(mask), num_possible_cpus());
	),

	TP_printk("comm=%s pid=%d cpus=%s",
		__entry->comm, __entry->pid, __get_bitmask(cpus))
);

#ifdef CONFIG_SCHED_WALT
struct rq;

TRACE_EVENT(walt_update_task_ravg,

	TP_PROTO(struct task_struct *p, struct rq *rq, int evt,
						u64 wallclock, u64 irqtime),

	TP_ARGS(p, rq, evt, wallclock, irqtime),

	TP_STRUCT__entry(
		__array(	char,	comm,   TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	pid_t,	cur_pid			)
		__field(	u64,	wallclock		)
		__field(	u64,	mark_start		)
		__field(	u64,	delta_m			)
		__field(	u64,	win_start		)
		__field(	u64,	delta			)
		__field(	u64,	irqtime			)
		__field(        int,    evt			)
		__field(unsigned int,	demand			)
		__field(unsigned int,	sum			)
		__field(	 int,	cpu			)
		__field(	u64,	cs			)
		__field(	u64,	ps			)
		__field(unsigned long,	util			)
		__field(	u32,	curr_window		)
		__field(	u32,	prev_window		)
		__field(	u64,	nt_cs			)
		__field(	u64,	nt_ps			)
		__field(	u32,	active_windows		)
	),

	TP_fast_assign(
		__entry->wallclock      = wallclock;
		__entry->win_start      = rq->window_start;
		__entry->delta          = (wallclock - rq->window_start);
		__entry->evt            = evt;
		__entry->cpu            = rq->cpu;
		__entry->cur_pid        = rq->curr->pid;
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid            = p->pid;
		__entry->mark_start     = p->ravg.mark_start;
		__entry->delta_m        = (wallclock - p->ravg.mark_start);
		__entry->demand         = p->ravg.demand;
		__entry->sum            = p->ravg.sum;
		__entry->irqtime        = irqtime;
		__entry->cs             = rq->curr_runnable_sum;
		__entry->ps             = rq->prev_runnable_sum;
		__entry->util           = rq->prev_runnable_sum << SCHED_CAPACITY_SHIFT;
		do_div(__entry->util, walt_ravg_window);
		__entry->curr_window	= p->ravg.curr_window;
		__entry->prev_window	= p->ravg.prev_window;
		__entry->nt_cs		= rq->nt_curr_runnable_sum;
		__entry->nt_ps		= rq->nt_prev_runnable_sum;
		__entry->active_windows	= p->ravg.active_windows;
	),

	TP_printk("wc %llu ws %llu delta %llu event %d cpu %d cur_pid %d task %d (%s) ms %llu delta %llu demand %u sum %u irqtime %llu"
		" cs %llu ps %llu util %lu cur_window %u prev_window %u active_wins %u"
		, __entry->wallclock, __entry->win_start, __entry->delta,
		__entry->evt, __entry->cpu, __entry->cur_pid,
		__entry->pid, __entry->comm, __entry->mark_start,
		__entry->delta_m, __entry->demand,
		__entry->sum, __entry->irqtime,
		__entry->cs, __entry->ps, __entry->util,
		__entry->curr_window, __entry->prev_window,
		  __entry->active_windows
		)
);

TRACE_EVENT(walt_update_history,

	TP_PROTO(struct rq *rq, struct task_struct *p, u32 runtime, int samples,
			int evt),

	TP_ARGS(rq, p, runtime, samples, evt),

	TP_STRUCT__entry(
		__array(	char,	comm,   TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(unsigned int,	runtime			)
		__field(	 int,	samples			)
		__field(	 int,	evt			)
		__field(	 u64,	demand			)
		__field(	 u64,	walt_avg		)
		__field(unsigned int,	pelt_avg		)
		__array(	 u32,	hist, RAVG_HIST_SIZE_MAX)
		__field(	 int,	cpu			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid            = p->pid;
		__entry->runtime        = runtime;
		__entry->samples        = samples;
		__entry->evt            = evt;
		__entry->demand         = p->ravg.demand;
		__entry->walt_avg	= (__entry->demand << SCHED_CAPACITY_SHIFT);
		__entry->walt_avg	= div_u64(__entry->walt_avg,
						  walt_ravg_window);
		__entry->pelt_avg	= p->se.avg.util_avg;
		memcpy(__entry->hist, p->ravg.sum_history,
					RAVG_HIST_SIZE_MAX * sizeof(u32));
		__entry->cpu            = rq->cpu;
	),

	TP_printk("%d (%s): runtime %u samples %d event %d demand %llu"
		" walt %llu pelt %u (hist: %u %u %u %u %u) cpu %d",
		__entry->pid, __entry->comm,
		__entry->runtime, __entry->samples, __entry->evt,
		__entry->demand,
		__entry->walt_avg,
		__entry->pelt_avg,
		__entry->hist[0], __entry->hist[1],
		__entry->hist[2], __entry->hist[3],
		__entry->hist[4], __entry->cpu)
);

TRACE_EVENT(walt_migration_update_sum,

	TP_PROTO(struct rq *rq, struct task_struct *p),

	TP_ARGS(rq, p),

	TP_STRUCT__entry(
		__field(int,		cpu			)
		__field(int,		pid			)
		__field(	u64,	cs			)
		__field(	u64,	ps			)
		__field(	s64,	nt_cs			)
		__field(	s64,	nt_ps			)
	),

	TP_fast_assign(
		__entry->cpu		= cpu_of(rq);
		__entry->cs		= rq->curr_runnable_sum;
		__entry->ps		= rq->prev_runnable_sum;
		__entry->nt_cs		= (s64)rq->nt_curr_runnable_sum;
		__entry->nt_ps		= (s64)rq->nt_prev_runnable_sum;
		__entry->pid		= p->pid;
	),

	TP_printk("cpu %d: cs %llu ps %llu nt_cs %lld nt_ps %lld pid %d",
		  __entry->cpu, __entry->cs, __entry->ps,
		  __entry->nt_cs, __entry->nt_ps, __entry->pid)
);

TRACE_EVENT(walt_window_rollover,

	TP_PROTO(int cpu, int windows),

	TP_ARGS(cpu, windows),

	TP_STRUCT__entry(
		__field(	 int,	cpu			)
		__field(	 int,	windows			)
	),

	TP_fast_assign(
		__entry->cpu            = cpu;
		__entry->windows        = windows;
	),

	TP_printk("cpu=%d windows=%d",
		__entry->cpu, __entry->windows)
);

#ifdef CONFIG_SCHED_HISI_TOP_TASK
TRACE_EVENT(walt_update_top_task,

	TP_PROTO(struct rq *rq, struct task_struct *p),

	TP_ARGS(rq, p),

	TP_STRUCT__entry(
		__array(	char,	comm,   TASK_COMM_LEN	)
		__field(	pid_t,	pid			)
		__field(	 int,	curr_load		)
		__field(	 int,	prev_load		)
		__field(	 int,	curr_top		)
		__field(	 int,	prev_top		)
		__field(	 int,	cpu			)
	),

	TP_fast_assign(
		memcpy(__entry->comm, p->comm, TASK_COMM_LEN);
		__entry->pid            = p->pid;
		__entry->curr_load      = p->ravg.curr_load;
		__entry->prev_load      = p->ravg.prev_load;
		__entry->curr_top       = rq->top_task_index[rq->curr_table];
		__entry->prev_top       = rq->top_task_index[1 - rq->curr_table];
		__entry->cpu            = rq->cpu;
	),

	TP_printk("cpu=%d pid=%d comm=%s curr_load=%d prev_load=%d curr_top=%d prev_top=%d",
		__entry->cpu, __entry->pid, __entry->comm,
		__entry->curr_load,
		__entry->prev_load,
		__entry->curr_top,
		__entry->prev_top)
);
#endif /* CONFIG_SCHED_HISI_TOP_TASK */

#endif /* CONFIG_SCHED_WALT */

#endif /* CONFIG_SMP */

#endif /* _TRACE_SCHED_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
