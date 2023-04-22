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
#include <linux/mount.h>
#include <linux/path.h>
#include <linux/dirent.h>
#include <linux/mnt_namespace.h>
#include <linux/sched.h>
#include <linux/signal.h>
#include <linux/syscalls.h>
#include <linux/kallsyms.h>
#include <linux/kthread.h>
#include <linux/delay.h>
#define TOOL_NAME "rProcess"
#define FAULTS_MAX 16

#if LINUX_VERSION_CODE < KERNEL_VERSION(3, 11, 0)
#define PID_NS(proxy) proxy->pid_ns
#else
#define PID_NS(proxy) proxy->pid_ns_for_children
#endif

enum FaultType {
	PROCESS_D_STATE = 1,
	PROCESS_Z_STATE,
	PROCESS_T_STATE,
	PROCESS_EXIT,
	PROCESS_HANG,
};

struct FaultImpl {
	pid_t pid;
	pid_t tid;
	enum FaultType fault;
};

struct FaultList {
	struct FaultImpl impl[FAULTS_MAX];
	rwlock_t rwk;
};

static struct {
	const char *op;
	enum FaultType type;
} fault_ops[] = {
	{
	.op = "D-state", PROCESS_D_STATE}, {
	.op = "Z-state", PROCESS_Z_STATE}, {
	.op = "T-state", PROCESS_T_STATE}, {
	.op = "exit", PROCESS_EXIT}, {
.op = "hang", PROCESS_HANG},};

enum FaultType string2fault(const char *name)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(fault_ops); i++) {
		if (strcmp(fault_ops[i].op, name) == 0)
			return fault_ops[i].type;
	}
	return 0;
}

const char *fault2string(enum FaultType fault)
{
	int i = 0;

	for (i = 0; i < ARRAY_SIZE(fault_ops); i++) {
		if (fault_ops[i].type == fault)
			return fault_ops[i].op;
	}
	return 0;
}

static struct FaultList fault_list;

static inline struct task_struct *find_process(pid_t pid)
{
	struct task_struct *tsk;
	struct pid_namespace *ns = PID_NS(current->nsproxy);
	struct pid *pd = find_pid_ns(pid, ns);

	rcu_read_lock();
	tsk = pid_task(pd, PIDTYPE_PID);
	rcu_read_unlock();

	return tsk;
}

static inline struct task_struct *find_thread_from_group(struct task_struct *ts,
							 pid_t tid)
{
	struct task_struct *t = ts;

	do {
		if (NULL == t)
			break;
		if (t->pid == tid)
			return t;
		t = next_thread(t);
	} while (t != ts);
	return NULL;
}
static inline struct task_struct *find_thread(pid_t tid)
{
	struct task_struct *t;

	t = find_process(tid);
	t = find_thread_from_group(t, tid);
	return t;
}

void fail_thread(struct task_struct *tsk, struct FaultImpl *ft)
{
	switch (ft->fault) {
	case PROCESS_D_STATE:
		if (tsk->exit_state == EXIT_ZOMBIE)
			tsk->exit_state = 0;
		set_task_state(tsk, TASK_UNINTERRUPTIBLE);
		break;
	case PROCESS_Z_STATE:
		set_task_state(tsk, TASK_DEAD);
		tsk->exit_state = EXIT_ZOMBIE;
		break;
	case PROCESS_T_STATE:
	case PROCESS_HANG:
		if (tsk->exit_state == EXIT_ZOMBIE)
			tsk->exit_state = 0;
		set_task_state(tsk, TASK_STOPPED);
		break;
	default:
		break;
	}
}

void resume_thread(struct task_struct *tsk, struct FaultImpl *ft)
{
	switch (ft->fault) {
	case PROCESS_D_STATE:
	case PROCESS_T_STATE:
	case PROCESS_HANG:
		set_task_state(tsk, TASK_INTERRUPTIBLE);
		wake_up_process(tsk);
		break;
	case PROCESS_Z_STATE:
		if (tsk->exit_state == EXIT_ZOMBIE)
			tsk->exit_state = 0;
		set_task_state(tsk, TASK_INTERRUPTIBLE);
		wake_up_process(tsk);
		break;
	case PROCESS_EXIT:
		break;
	default:
		break;
	}
}

static void action_each_threads(struct task_struct *ts, struct FaultImpl *ft,
				void (atction) (struct task_struct *,
						struct FaultImpl *))
{
	struct task_struct *t;

	t = find_process(ts->tgid);	/*find the threads group leader */
	ts = t;
	do {
		if (NULL == t)
			break;

		atction(t, ft);
		t = next_thread(t);
	} while (t != ts);

}

void fail_process(struct task_struct *tsk, struct FaultImpl *ft)
{
	switch (ft->fault) {
	case PROCESS_D_STATE:
	case PROCESS_Z_STATE:
	case PROCESS_HANG:
		action_each_threads(tsk, ft, fail_thread);
		break;
	case PROCESS_T_STATE:
		kill_pid(find_vpid(tsk->pid), SIGSTOP, 1);
		action_each_threads(tsk, ft, fail_thread);
		break;
	case PROCESS_EXIT:
		kill_pid(find_vpid(tsk->pid), SIGQUIT, 1);
		break;
	default:
		break;
	}
}

void resume_process(struct task_struct *tsk, struct FaultImpl *ft)
{
	switch (ft->fault) {
	case PROCESS_D_STATE:
	case PROCESS_Z_STATE:
	case PROCESS_HANG:
		action_each_threads(tsk, ft, resume_thread);
		break;
	case PROCESS_T_STATE:
		action_each_threads(tsk, ft, resume_thread);
		kill_pid(find_vpid(tsk->pid), SIGCONT, 1);
		break;
	case PROCESS_EXIT:
		break;
	default:
		break;
	}
}

void fail(struct task_struct *tsk, struct FaultImpl *ft)
{
	if (ft->pid)
		fail_process(tsk, ft);
	else
		fail_thread(tsk, ft);

}

void resume(struct task_struct *tsk, struct FaultImpl *ft)
{
	if (ft->pid)
		resume_process(tsk, ft);
	else
		resume_thread(tsk, ft);
}

static struct FaultImpl *find_fault(struct FaultImpl *ft)
{
	int i = 0;
	int j = -1;

	for (i = 0; i < FAULTS_MAX; i++) {
		if ((fault_list.impl[i].pid != 0
		     && fault_list.impl[i].pid == ft->pid)
		    || (fault_list.impl[i].tid != 0
			&& fault_list.impl[i].tid == ft->tid)) {
			j = i;
			break;
		}

		if (fault_list.impl[i].fault == 0 && j == -1)
			j = i;
	}
	if (j == -1)
		return 0;
	return &fault_list.impl[j];
}

static int fill_fault(struct FaultImpl *ft)
{
	struct FaultImpl *impl = find_fault(ft);

	if (0 == impl)
		return -ENOENT;
	memcpy(impl, ft, sizeof(struct FaultImpl));
	return 0;
}

static struct task_struct *find_task(struct FaultImpl *ft)
{
	struct task_struct *tsk = NULL;

	if (ft->pid)
		tsk = find_process(ft->pid);
	else if (ft->tid)
		tsk = find_thread(ft->tid);
	return tsk;
}
static int check_fault(struct FaultImpl *ft)
{
	if (ft->fault == 0 || (ft->pid == 0 && ft->tid == 0)
	    || (ft->pid != 0 && ft->tid != 0)
	    || (ft->fault == PROCESS_EXIT && ft->tid != 0))
		return -EINVAL;
	return 0;
}
static int resume_fault(struct FaultImpl *ft)
{
	struct task_struct *tsk;

	/*1. check the fault valid */
	ras_retn_iferr(check_fault(ft));
	ft = find_fault(ft);
	if (NULL == ft)
		return -EINVAL;
	/*2.find the the tast_struct */
	tsk = find_task(ft);
	if (NULL != tsk)
		resume(tsk, ft);	/*3.do resume */
	else
		pr_err("process:%d,thread:%d not found.\n", ft->pid, ft->tid);

	/*4.resume the information anyway */
	memset(ft, 0, sizeof(struct FaultImpl));
	return 0;
}
static void resume_all(void)
{
	int i = 0;

	write_lock(&fault_list.rwk);
	for (i = 0; i < FAULTS_MAX; i++)
		resume_fault(&fault_list.impl[i]);
	write_unlock(&fault_list.rwk);

}
static int inject_fault(struct FaultImpl *ft)
{
	struct task_struct *tsk;

	/*1. check the fault valid */
	ras_retn_iferr(check_fault(ft));
	/*2.find the the tast_struct */
	tsk = find_task(ft);
	if (NULL == tsk) {
		pr_err("can't find the task.\n");
		return -EINVAL;
	}

	/*3.fill the information */
	ras_retn_iferr(fill_fault(ft));
	/*4.do injection */
	fail(tsk, ft);
	return 0;
}

static int args_set(struct FaultImpl *ft, char *prm)
{
	if (NULL == prm)
		return 0;
	rasbase_set(ft, pid, prm);
	rasbase_set(ft, tid, prm);
	rasbase_set_func(ft, fault, prm, string2fault);
	return -EINVAL;
}

static int args_check(struct FaultImpl *ft, int argc, char *argv[])
{
	int i = 0;

	for (i = 0; i < argc; i++)
		ras_retn_iferr(args_set(ft, argv[i]));
	return 0;
}

static int cmd_main(void *data, int argc, char *argv[])
{
	int ret = -EINVAL;
	struct FaultImpl ft;

	ras_retn_if(argc < 2, -EINVAL);
	if (0 == strcmp(argv[0], "clean")) {
		resume_all();
		return 0;
	}
	memset(&ft, 0, sizeof(struct FaultImpl));
	ras_retn_iferr(args_check(&ft, argc, argv));
	write_lock(&fault_list.rwk);
	ret = inject_fault(&ft);
	write_unlock(&fault_list.rwk);
	return ret;
}

static int proc_ops_show(rProcess) (struct seq_file *m, void *v)
{
	int i = 0;

	for (i = 0; i < FAULTS_MAX; i++) {
		struct task_struct *tsk;
		struct FaultImpl *ft = &fault_list.impl[i];

		if (check_fault(ft))
			continue;
		tsk = find_task(ft);
		seq_printf(m, "%d\t", i);
		seq_printf(m, "fault=%s\t", fault2string(ft->fault));
		if (ft->pid)
			seq_printf(m, "pid=%d\t", ft->pid);
		else if (ft->tid)
			seq_printf(m, "tid=%d\t", ft->tid);
		if (tsk){
			seq_printf(m, "[%s]", tsk->comm);
			seq_printf(m, " [%d]", tsk->tgid);
		}
		else{
			seq_puts(m, " [already exit]");
		}
		seq_puts(m, "\n");
	}
	return 0;
}

static int proc_ops_open(rProcess) (struct inode *inode, struct file *file)
{
	return single_open(file, proc_ops_show(rProcess), PDE_DATA(inode));
}

static ssize_t proc_ops_write(rProcess) (struct file *filp,
					 const char __user *bff, size_t count,
					 loff_t *data) {
	char buf_cmd[256] = { 0 };

	if (unlikely(count >= sizeof(buf_cmd)))
		return -ENOMEM;
	ras_retn_iferr(copy_from_user(buf_cmd, bff, count));
	ras_retn_iferr(ras_args(buf_cmd, count, cmd_main, NULL));
	return count;
}

proc_ops_define(rProcess);
static int tool_init(void)
{
	ras_debugset(1);
	ras_retn_iferr(ras_check());
	memset(&fault_list, 0, sizeof(fault_list));
	rwlock_init(&fault_list.rwk);
	ras_retn_iferr(proc_init
		       (TOOL_NAME, &proc_ops_name(rProcess), &fault_list));
	return 0;
}
static void tool_exit(void)
{
	proc_exit(TOOL_NAME);
	resume_all();
}

module_init(tool_init);
module_exit(tool_exit);
MODULE_DESCRIPTION("Fault injection for both process and thread .");
MODULE_LICENSE("GPL");
MODULE_VERSION("v1.0.1612");
