/*
 * kernel/sched/hwstatus.c
 *
 * Copyright(C) 2018, Huawei, Inc., hwfatboy
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/proc_fs.h>
#include <linux/sched.h>
#include <linux/seq_file.h>
#include <linux/kallsyms.h>
#include <linux/utsname.h>
#include <linux/mempolicy.h>
#include <log/log_usertype/log-usertype.h>

#include <asm/barrier.h>
#include <asm/stacktrace.h>
#include <asm/irq.h>

#include "sched.h"

/*
 * Ease the printing of nsec fields:
 */
static long long nsec_high(unsigned long long nsec)
{
	if ((long long)nsec < 0) {
		nsec = -nsec;
		do_div(nsec, 1000000);
		return -nsec;
	}
	do_div(nsec, 1000000);

	return nsec;
}

static unsigned long nsec_low(unsigned long long nsec)
{
	if ((long long)nsec < 0)
		nsec = -nsec;

	return do_div(nsec, 1000000);
}

#define SEQ_printf(m, x...)			\
 do {						\
	if (m)					\
		seq_printf(m, x);		\
	else					\
		printk(x);			\
 } while (0)

#define SPLIT_NS(x) nsec_high(x), nsec_low(x)

#define PN(F) \
        SEQ_printf(m,"  .%-30s: %lld.%06ld\n",#F, SPLIT_NS((long long )F))

#define FGTASK_MAX 4
#define M_IODELAY_VALUE  (300*1000*1000)
#define CALLER_NAME_LEN  256
#define M_DSDELAY_VALUE  (200*1000*1000)
#define STACK_DUMP_SIZE  1024

enum{
	FG_UI = 0,
	FG_RENDER,
	PREV_UI,
	PREV_RENDER
};

enum{
	VERSION_V1 = 1,
	VERSION_V2 = 2,
	VERSION_V3,
};

typedef struct sched_hwstatus_rsthead
{
	u64 ktime_last;
	u64 ktime_now;
}sched_hwstatus_rsthead;

typedef struct hwstatus_mem
{
	u64 allocuser_delay;
	u64 allocuser_count;
	u64 allocuser_delay_max;
	u64 allocuser_delay_max_order;
}hwstatus_mem;

typedef struct hwstatus_caller
{
	u64  ktime_iodelay;
	char caller_iodelay[CALLER_NAME_LEN];
}hwstatus_caller;

typedef struct hwstatus_stack
{
	char	stack[STACK_DUMP_SIZE];
}hwstatus_stack;

typedef struct sched_hwstatus_rstbody
{
	pid_t pid;
	pid_t tgid;
	sched_hwstatus hwstatus;
}sched_hwstatus_rstbody;

typedef struct sched_hwstatus_rst_V1
{
	sched_hwstatus_rsthead head;
	sched_hwstatus_rstbody body[FGTASK_MAX];
}sched_hwstatus_rst_V1;

typedef struct sched_hwstatus_rst_V2
{
	sched_hwstatus_rsthead head;
	sched_hwstatus_rstbody body[FGTASK_MAX];
	hwstatus_mem           mem[FGTASK_MAX];
	hwstatus_caller        caller;
}sched_hwstatus_rst_V2;

typedef struct sched_hwstatus_rst_V3
{
	sched_hwstatus_rsthead head;
	sched_hwstatus_rstbody body[FGTASK_MAX];
	hwstatus_mem           mem[FGTASK_MAX];
	hwstatus_caller        caller;
	hwstatus_stack         stack;
}sched_hwstatus_rst_V3;

typedef sched_hwstatus_rst_V3 sched_hwstatus_rst;

static pid_t fgtasks[FGTASK_MAX] = {0};
static u64 ktime_last = 0;
static hwstatus_caller caller = {0};
static hwstatus_stack  stack_dump = {0};
static u64 delta_max = 0;

static void simple_record_stack(struct task_struct *tsk,char *buf, int max)
{
	struct stackframe frame;
	int pos = 0;
	int len = 0;

	if (!tsk)
		tsk = current;

	if (!try_get_task_stack(tsk))
		return;

	if (tsk == current) {
		frame.fp = (unsigned long)__builtin_frame_address(0);
		frame.sp = current_stack_pointer;
		frame.pc = (unsigned long)simple_record_stack;
	} else {
		frame.fp = thread_saved_fp(tsk);
		frame.sp = thread_saved_sp(tsk);
		frame.pc = thread_saved_pc(tsk);
	}

	while (1) {
		unsigned long where = frame.pc;
		int ret;
        if(max <= pos)
			break;
		len = snprintf(buf+pos, max-pos,"%pS\n", (void *) where);
		if((len <= 0) || (len >= (max-pos)))
			break;
		pos += len;
		ret = unwind_frame(tsk, &frame);
		if (ret < 0)
			break;
	}

	put_task_stack(tsk);

	barrier();
}

static void clearpid(pid_t pid)
{
	struct task_struct *taskp;
	struct sched_statistics *ssp;
	unsigned long flags;

	taskp = find_task_by_vpid(pid);
	if(!taskp){
		return;
	}

	get_task_struct(taskp);
	ssp = &(taskp->se.statistics);
	ssp->hwstatus.sum_exec_runtime_big   = 0;
	ssp->hwstatus.sum_exec_runtime_mid   = 0;
	ssp->hwstatus.sum_exec_runtime_ltt   = 0;
	ssp->hwstatus.wait_sum            = ssp->wait_sum;
	ssp->hwstatus.sum_sleep_runtime   = ssp->sum_sleep_runtime;

	ssp->hwstatus.iowait_sum = ssp->iowait_sum;
	ssp->hwstatus.iowait_max    = 0;

	ssp->hwstatus.dstate_block_count = 0;
	ssp->hwstatus.wait_count = ssp->wait_count;
	ssp->hwstatus.iowait_count = ssp->iowait_count;
	ssp->hwstatus.sleep_count = 0;

	ssp->hwstatus.dstate_block_max   = 0;
	ssp->hwstatus.dstate_block_sum   = 0;
	ssp->hwstatus.last_jiffies = jiffies;

#ifdef CONFIG_HW_MEMORY_MONITOR
	spin_lock_irqsave(&taskp->delays->allocpages_lock, flags);
	taskp->delays->allocuser_delay = 0;
	taskp->delays->allocuser_count = 0;
	taskp->delays->allocuser_delay_max = 0;
	taskp->delays->allocuser_delay_max_order = 0;
	spin_unlock_irqrestore(&taskp->delays->allocpages_lock, flags);
#endif
	put_task_struct(taskp);
}

static void sched_hwstatus_clear(pid_t pid)
{
	if(pid == 0) {
		clearpid(fgtasks[0]);
		clearpid(fgtasks[1]);
	} else {
		clearpid(pid);
	}
}

void sched_hwstatus_iodelay_caller(struct task_struct *tsk, u64 delta)
{
	void *__caller;
	u64 delaymax;

	if(BETA_USER != get_logusertype_flag()) {
		return 0;
	}

	if(tsk->in_iowait) {
		delaymax = M_IODELAY_VALUE;
	} else {
		delaymax = M_DSDELAY_VALUE;
	}

	if(delta < delaymax) {
		return;
	}

	if((tsk) && (tsk->static_vip)) {
		if(tsk->pid != tsk->tgid) {
			if((tsk->comm[0] != 'R') || (tsk->comm[1] != 'e')) {
				return;
			}
		}

		if(delta_max < delta) {
			delta_max = delta;
		} else {
			return;
		}

		__caller = (void *)get_wchan(tsk);
		caller.ktime_iodelay = ktime_get_ns();
		snprintf(caller.caller_iodelay, CALLER_NAME_LEN, "[%d,%d]%s W:%d P:[%d,%d] N:%s T:%Lu %pS\n",
					   current->pid, current->tgid, current->comm, tsk->in_iowait, tsk->pid, tsk->tgid, tsk->comm, delta_max, __caller);
		simple_record_stack(tsk, stack_dump.stack, STACK_DUMP_SIZE);
	}
}

void sched_hwstatus_updatefg(pid_t pid, pid_t tgid)
{
	if(BETA_USER != get_logusertype_flag()) {
		return;
	}

	if(tgid != fgtasks[FG_UI]) {
		sched_hwstatus_clear(pid);
		if(pid != tgid) {
			fgtasks[PREV_UI]     = fgtasks[FG_UI];
			fgtasks[PREV_RENDER] = fgtasks[FG_RENDER];

			fgtasks[FG_UI]     = tgid;
			fgtasks[FG_RENDER] = pid;
		}
	}
	return;
}

static int sched_hwstatus_show(struct seq_file *m, void *v)
{
	int i;

	if(BETA_USER != get_logusertype_flag()) {
		return 0;
	}

	for(i=0;  i<FGTASK_MAX; i++){
		struct task_struct *taskp;
		sched_hwstatus *statusp;
		u64 wait_sum;
		u64 sum_sleep_runtime;
		u64 iowait_sum;
		u64 wait_count;
		u64 iowait_count;

		taskp = find_task_by_vpid(fgtasks[i]);
		if(!taskp){
			SEQ_printf(m, "### %s find_task_by_vpid err!\n", __FUNCTION__);
			return 0;
		}

		get_task_struct(taskp);
		statusp = &(taskp->se.statistics.hwstatus);
		SEQ_printf(m, "pid:%d,jiffies:%llu\n", fgtasks[i],jiffies - statusp->last_jiffies);
		wait_sum = taskp->se.statistics.wait_sum - statusp->wait_sum;
		sum_sleep_runtime = taskp->se.statistics.sum_sleep_runtime - statusp->sum_sleep_runtime;
		iowait_sum = taskp->se.statistics.iowait_sum - statusp->iowait_sum;
		PN(statusp->sum_exec_runtime_big);
		PN(statusp->sum_exec_runtime_mid);
		PN(statusp->sum_exec_runtime_ltt);
		PN(wait_sum);
		PN(sum_sleep_runtime);
		PN(iowait_sum);
		PN(statusp->iowait_max);

		PN(statusp->dstate_block_max);
		PN(statusp->dstate_block_sum);

		PN(statusp->dstate_block_count);
		wait_count = taskp->se.statistics.wait_count - statusp->wait_count;
		iowait_count = taskp->se.statistics.iowait_count - statusp->iowait_count;
		PN(wait_count);
		PN(iowait_count);
		PN(statusp->sleep_count);
#ifdef CONFIG_HW_MEMORY_MONITOR
		PN(taskp->delays->allocuser_delay);
		PN(taskp->delays->allocuser_count);
		PN(taskp->delays->allocuser_delay_max);
		PN(taskp->delays->allocuser_delay_max_order);
#endif
		put_task_struct(taskp);
	}

	if(caller.ktime_iodelay >= ktime_last) {
		SEQ_printf(m, "### blockinfo:%s\n", caller.caller_iodelay);
	}
	return 0;
}

static ssize_t sched_hwstatus_read(struct file* file, char __user *buf,
			size_t count, loff_t *ppos)
{
	int i;
	static unsigned long last_jiffies = 0;
	sched_hwstatus_rst hwstatus_rst;
	int version;

	if(BETA_USER != get_logusertype_flag()) {
		return 0;
	}

	switch(count) {
		case sizeof(sched_hwstatus_rst_V3):
			version = VERSION_V3;
			break;
		case sizeof(sched_hwstatus_rst_V2):
			version = VERSION_V2;
			break;
		case sizeof(sched_hwstatus_rst_V1):
			version = VERSION_V1;
			break;
		default:
			return -1;
	}

	if((jiffies - last_jiffies)< HZ){
		return 0;
	}

	last_jiffies = jiffies;

	for(i=0;  i<FGTASK_MAX; i++){
		struct task_struct *taskp;
		struct sched_statistics *ssp;
		sched_hwstatus_rstbody  *rstp;
		taskp = find_task_by_vpid(fgtasks[i]);
		if(!taskp){
			return -1;
		}

		get_task_struct(taskp);
		ssp  = &(taskp->se.statistics);
		rstp = &hwstatus_rst.body[i];
		rstp->pid      = fgtasks[i];
		rstp->tgid     = fgtasks[(i>>1)<<1];
		rstp->hwstatus.last_jiffies  = jiffies - ssp->hwstatus.last_jiffies;
		rstp->hwstatus.sum_exec_runtime_big = ssp->hwstatus.sum_exec_runtime_big;
		rstp->hwstatus.sum_exec_runtime_mid = ssp->hwstatus.sum_exec_runtime_mid;
		rstp->hwstatus.sum_exec_runtime_ltt = ssp->hwstatus.sum_exec_runtime_ltt;
		rstp->hwstatus.wait_sum            = ssp->wait_sum - ssp->hwstatus.wait_sum;
		rstp->hwstatus.sum_sleep_runtime   = ssp->sum_sleep_runtime - ssp->hwstatus.sum_sleep_runtime;

		rstp->hwstatus.iowait_sum = ssp->iowait_sum - ssp->hwstatus.iowait_sum;
		rstp->hwstatus.iowait_max = ssp->hwstatus.iowait_max;

		rstp->hwstatus.dstate_block_max = ssp->hwstatus.dstate_block_max;
		rstp->hwstatus.dstate_block_sum = ssp->hwstatus.dstate_block_sum;

		rstp->hwstatus.wait_count   = ssp->wait_count - ssp->hwstatus.wait_count;
		rstp->hwstatus.sleep_count  = ssp->hwstatus.sleep_count;
		rstp->hwstatus.iowait_count = ssp->iowait_count - ssp->hwstatus.iowait_count;
		rstp->hwstatus.dstate_block_count = ssp->hwstatus.dstate_block_count;

		if(version > VERSION_V1) {
#ifdef CONFIG_HW_MEMORY_MONITOR
			hwstatus_rst.mem[i].allocuser_delay = taskp->delays->allocuser_delay;
			hwstatus_rst.mem[i].allocuser_count = taskp->delays->allocuser_count;
			hwstatus_rst.mem[i].allocuser_delay_max = taskp->delays->allocuser_delay_max;
			hwstatus_rst.mem[i].allocuser_delay_max_order = taskp->delays->allocuser_delay_max_order;
#else
			hwstatus_rst.mem[i].allocuser_delay = 0;
			hwstatus_rst.mem[i].allocuser_count = 0;
			hwstatus_rst.mem[i].allocuser_delay_max = 0;
			hwstatus_rst.mem[i].allocuser_delay_max_order = 0;
#endif
			if(caller.ktime_iodelay >= ktime_last) {
				hwstatus_rst.caller.ktime_iodelay = caller.ktime_iodelay;
				strncpy(hwstatus_rst.caller.caller_iodelay, caller.caller_iodelay, CALLER_NAME_LEN);
				if(version > VERSION_V2) {
					strncpy(hwstatus_rst.stack.stack, stack_dump.stack, STACK_DUMP_SIZE);
				}
			} else {
				hwstatus_rst.caller.ktime_iodelay  = 0;
				hwstatus_rst.caller.caller_iodelay[0] = '\0';
			}
		}

		put_task_struct(taskp);
	}

	hwstatus_rst.head.ktime_last = ktime_last;
	hwstatus_rst.head.ktime_now  = ktime_get_ns();

	return simple_read_from_buffer(buf, count, ppos, &hwstatus_rst, count);
}



static ssize_t sched_hwstatus_write(struct file *file, const char __user *buf,
			size_t count, loff_t *ppos)
{
	pid_t pid = 0;
	int rv;
	static unsigned long last_jiffies = 0;

	if(BETA_USER != get_logusertype_flag()) {
		return 0;
	}

	if(count > 10) {
		return 0;
	}

	if((jiffies - last_jiffies)< HZ){
		return 0;
	}

	last_jiffies = jiffies;
	rv = kstrtos32_from_user(buf, count, 10, &pid);
	if (rv < 0)
		return rv;

	if(0 == pid) {
		sched_hwstatus_show(NULL,NULL);
		return count;
	}

	sched_hwstatus_clear(0);
	ktime_last = ktime_get_ns();
	delta_max = 0;

	return count;
}

static const struct file_operations sched_hwstatus_fops = {
	.read		= sched_hwstatus_read,
	.write		= sched_hwstatus_write,
};

static int __init init_sched_hwstatus_procfs(void)
{
	struct proc_dir_entry *pe;

	pe = proc_create("sched_hw", 0660, NULL, &sched_hwstatus_fops);
	if (!pe)
		return -ENOMEM;
	return 0;
}

__initcall(init_sched_hwstatus_procfs);
