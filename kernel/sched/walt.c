/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *
 * Window Assisted Load Tracking (WALT) implementation credits:
 * Srivatsa Vaddagiri, Steve Muckle, Syed Rameez Mustafa, Joonwoo Park,
 * Pavan Kumar Kondeti, Olav Haugan
 *
 * 2016-03-06: Integration with EAS/refactoring by Vikram Mulukutla
 *             and Todd Kjos
 */

#include <linux/acpi.h>
#include <linux/syscore_ops.h>
#include <linux/cpufreq.h>
#include <trace/events/sched.h>
#include "sched.h"
#include "tune.h"
#include "walt.h"
#include <linux/hisi_rtg.h>


static __read_mostly unsigned int walt_ravg_hist_size = 5;
static __read_mostly unsigned int walt_window_stats_policy =
	WINDOW_STATS_MAX_RECENT_AVG;
static __read_mostly unsigned int walt_account_wait_time = 1;
static __read_mostly unsigned int walt_freq_account_wait_time = 0;
static __read_mostly unsigned int walt_io_is_busy = 0;

unsigned int sysctl_sched_walt_init_task_load_pct = 40;

#ifdef CONFIG_SCHED_HISI_WALT_WINDOW_SIZE_TUNABLE
/* true -> use PELT based load stats, false -> use window-based load stats */
bool __read_mostly walt_disabled = false;

/*
 * Window size (in ns). Adjust for the tick size so that the window
 * rollover occurs just before the tick boundary.
 */
__read_mostly unsigned int walt_ravg_window =
					    (20000000 / TICK_NSEC) * TICK_NSEC;
#define MIN_SCHED_RAVG_WINDOW ((10000000 / TICK_NSEC) * TICK_NSEC)
#define MAX_SCHED_RAVG_WINDOW ((1000000000 / TICK_NSEC) * TICK_NSEC)
#else
const bool walt_disabled = false;
const unsigned int walt_ravg_window =
				(20000000 / TICK_NSEC) * TICK_NSEC;
#endif

static unsigned int sync_cpu;
static ktime_t ktime_last;
static __read_mostly bool walt_ktime_suspended;

#ifdef CONFIG_SCHED_HISI_USE_WALT
static unsigned int task_load(struct task_struct *p)
{
	return p->ravg.demand;
}
#endif

static inline int exiting_task(struct task_struct *p)
{
	return p->flags & PF_EXITING;
}

#ifdef CONFIG_SCHED_HISI_TOP_TASK
static inline unsigned int task_load_freq(struct task_struct *p)
{
	return p->ravg.load_avg;
}

#ifdef CONFIG_SCHED_HISI_DOWNMIGRATE_LOWER_LOAD
#define TASK_LOAD_FREQ_AVG_HIST_SIZE 3
static unsigned int task_load_freq_avg(struct task_struct *p)
{
	u32 *hist;
	int widx;
	u64 sum = 0;
	unsigned int hist_size;

	hist = &p->ravg.load_sum_history[0];
	hist_size = TASK_LOAD_FREQ_AVG_HIST_SIZE;

	for (widx = 0; widx < hist_size; widx++)
		sum += hist[widx];

	return sum / hist_size;
}
#endif

static inline u8 curr_table(struct rq *rq)
{
	return rq->curr_table;
}

static inline u8 prev_table(struct rq *rq)
{
	return 1 - rq->curr_table;
}

static inline int reversed_index(int index)
{
	if (unlikely(index >= NUM_LOAD_INDICES))
		index = NUM_LOAD_INDICES - 1;
	return NUM_LOAD_INDICES - 1 - index;
}

/*
 * Let lower index means higher load, so we can use find_next_bit
 * to get the next highest load.
 *
 * Note: max load is limited as:
 *       walt_ravg_window / NUM_LOAD_INDICES * (NUM_LOAD_INDICES - 1)
 */
static int load_to_index(u32 load)
{
	return reversed_index(load / SCHED_LOAD_GRANULE);
}

static unsigned long index_to_load(int index)
{
	return reversed_index(index) * SCHED_LOAD_GRANULE;
}

static unsigned long __top_task_load(struct rq *rq, u8 table)
{
	struct top_task_entry *top_task_table = rq->top_tasks[table];
	int top_index = rq->top_task_index[table];

	/*
	 * Top_index might be NUM_LOAD_INDICES.
	 * Check before use it as array index.
	 */
	if (top_index >= NUM_LOAD_INDICES)
		return 0;

	/*
	 * Just consider no top task load when the top task is
	 * not a preferidle one. It will take some cost to find
	 * the top preferidle task, which may not be very necessary.
	 */
	if (!top_task_table[top_index].preferidle_count)
		return 0;

	return index_to_load(top_index);
}

static unsigned long top_task_load(struct rq *rq)
{
	return max(__top_task_load(rq, curr_table(rq)),
		   __top_task_load(rq, prev_table(rq)));
}

unsigned long top_task_util(struct rq *rq)
{
	unsigned long capacity = capacity_orig_of(cpu_of(rq));

	unsigned long load = top_task_load(rq);
	unsigned long util = (load << SCHED_CAPACITY_SHIFT) / walt_ravg_window;

	return (util >= capacity) ? capacity : util;
}

/*
 * Limit overflow case.
 * For top_task_table[index].count, increase and decrease
 * for any index are always in pair.
 * For top_task_table[index].preferidle_count, we may lose
 * +1 or -1 sometimes so it's not accurate. Disallow overflow
 * will be helpful.
 */
static inline bool inc_top_task_count(u8 *count)
{
	bool will_overflow = (*count == (u8)(~0));

	if (!will_overflow)
		(*count)++;

	return will_overflow;
}

static inline bool dec_top_task_count(u8 *count)
{
	bool will_overflow = (*count == 0);

	if (!will_overflow)
		(*count)--;

	return will_overflow;
}

static u32 add_top_task_load(struct rq *rq, u8 table,
				u32 load, bool prefer_idle)
{
	struct top_task_entry *top_task_table = rq->top_tasks[table];
	int index = load_to_index(load);
	u32 delta_load = 0;

	if (index == ZERO_LOAD_INDEX)
		return delta_load;

	inc_top_task_count(&top_task_table[index].count);

	/*
	 * if 0->1.
	 * This means it's the first task that fill this load index.
	 * Set the bit in top_task_bitmap and check if top_task_index
	 * needs to be updated.
	 */
	if (top_task_table[index].count == 1) {
		__set_bit(index, rq->top_tasks_bitmap[table]);

		if (index < rq->top_task_index[table]) {
			delta_load = (rq->top_task_index[table] - index)
					* SCHED_LOAD_GRANULE;
			rq->top_task_index[table] = index;
		}
	}

	if (prefer_idle)
		inc_top_task_count(&top_task_table[index].preferidle_count);

	return delta_load;
}

static void del_top_task_load(struct rq *rq, u8 table,
				u32 load, bool prefer_idle)
{
	struct top_task_entry *top_task_table = rq->top_tasks[table];
	int index = load_to_index(load);

	if (index == ZERO_LOAD_INDEX)
		return;

	dec_top_task_count(&top_task_table[index].count);

	/*
	 * if 1->0.
	 * This means currently there's no task has this load index.
	 * Clear the bit in top_task_bitmap and update top_task_index
	 * if necessary.
	 */
	if (top_task_table[index].count == 0) {
		__clear_bit(index, rq->top_tasks_bitmap[table]);

		/* Always clear preferidle_count when we are sure there's no task */
		top_task_table[index].preferidle_count = 0;

		if (index == rq->top_task_index[table]) {
			rq->top_task_index[table] = find_next_bit(
				rq->top_tasks_bitmap[table], NUM_LOAD_INDICES, index);
		}
	}

	if (prefer_idle)
		dec_top_task_count(&top_task_table[index].preferidle_count);

	return;
}

#define TOP_TASK_IGNORE_UTIL 80
#define TOP_TASK_IGNORE_LOAD \
	((u64)walt_ravg_window * TOP_TASK_IGNORE_UTIL / SCHED_CAPACITY_SCALE)

static void add_top_task(struct task_struct *p, struct rq *rq)
{
	u32 delta_load;
	u32 load = task_load_freq(p);

	if (exiting_task(p) || is_idle_task(p))
		return;

	if (load < TOP_TASK_IGNORE_LOAD)
		return;

	p->ravg.curr_load = load;

	delta_load = add_top_task_load(rq, rq->curr_table, load, schedtune_top_task(p) > 0);

	if (delta_load > TOP_TASK_IGNORE_LOAD &&
	    top_task_util(rq) > capacity_curr_of(cpu_of(rq)))
		sugov_mark_util_change(cpu_of(rq), ADD_TOP_TASK);

	trace_walt_update_top_task(rq, p);
}

static void
migrate_top_task(struct task_struct *p, struct rq *src_rq, struct rq *dest_rq)
{
	bool is_top_task = schedtune_top_task(p) > 0;

#ifdef CONFIG_SCHED_HISI_DOWNMIGRATE_LOWER_LOAD
	/* Clear p's top task load in downmigrate case */
	if (capacity_orig_of(cpu_of(src_rq)) > capacity_orig_of(cpu_of(dest_rq))) {
		del_top_task_load(src_rq, curr_table(src_rq), p->ravg.curr_load, is_top_task);
		del_top_task_load(src_rq, prev_table(src_rq), p->ravg.prev_load, is_top_task);
		p->ravg.curr_load = p->ravg.prev_load = 0;

		trace_walt_update_top_task(src_rq, p);
		return;
	}
#endif

	add_top_task_load(dest_rq, curr_table(dest_rq), p->ravg.curr_load, is_top_task);
	add_top_task_load(dest_rq, prev_table(dest_rq), p->ravg.prev_load, is_top_task);

	del_top_task_load(src_rq, curr_table(src_rq), p->ravg.curr_load, is_top_task);
	del_top_task_load(src_rq, prev_table(src_rq), p->ravg.prev_load, is_top_task);

	trace_walt_update_top_task(src_rq, p);
	trace_walt_update_top_task(dest_rq, p);
}

void top_task_exit(struct task_struct *p, struct rq *rq)
{
	/*
	 * We'd better not clear p in top task here, unless we do walt update
	 * before top_task_exit. For now, leave exit task as if it were still
	 * on that cpu.
	 */
	return;
}

static inline void clear_top_tasks_table(struct top_task_entry *table)
{
	memset(table, 0, NUM_LOAD_INDICES * sizeof(struct top_task_entry));
}

static inline void clear_top_tasks_bitmap(unsigned long *bitmap)
{
	memset(bitmap, 0, BITS_TO_LONGS(NUM_LOAD_INDICES) * sizeof(unsigned long));
}

static void rollover_top_task_table(struct rq *rq, int nr_full_windows)
{
	u8 curr_table = rq->curr_table;
	u8 prev_table = 1 - curr_table;

	/* Clear prev table */
	clear_top_tasks_table(rq->top_tasks[prev_table]);
	clear_top_tasks_bitmap(rq->top_tasks_bitmap[prev_table]);
	rq->top_task_index[prev_table] = ZERO_LOAD_INDEX;

	/* Also clear curr table if last window is empty */
	if (nr_full_windows) {
		rq->top_task_index[curr_table] = ZERO_LOAD_INDEX;
		clear_top_tasks_table(rq->top_tasks[curr_table]);
		clear_top_tasks_bitmap(rq->top_tasks_bitmap[curr_table]);
	}

	/* Finally exchange rq's curr_table & prev_table pointer.
	 * This means let curr to be prev, and let an empty one to be curr.
	 *
	 * new tables:               curr                prev
	 *                            |                   |
	 * old tables:   prev_table(must be cleared)  curr_table
	 */
	rq->curr_table = prev_table;
}

static void rollover_top_task_load(struct task_struct *p, int nr_full_windows)
{
	int curr_load = 0;

	if (!nr_full_windows)
		curr_load = p->ravg.curr_load;

	p->ravg.prev_load = curr_load;
	p->ravg.curr_load = 0;
}
#endif /* CONFIG_SCHED_HISI_TOP_TASK */

static inline void fixup_cum_window_demand(struct rq *rq, s64 delta)
{
#ifdef CONFIG_SCHED_HISI_CALC_CUM_WINDOW_DEMAND
	rq->cum_window_demand += delta;
	if (unlikely((s64)rq->cum_window_demand < 0))
		rq->cum_window_demand = 0;
#endif
}

void
walt_inc_cumulative_runnable_avg(struct rq *rq,
				 struct task_struct *p)
{
#ifdef CONFIG_SCHED_HISI_USE_WALT
	rq->cumulative_runnable_avg += p->ravg.demand;

	/*
	 * Add a task's contribution to the cumulative window demand when
	 *
	 * (1) task is enqueued with on_rq = 1 i.e migration,
	 *     prio/cgroup/class change.
	 * (2) task is waking for the first time in this window.
	 */
	if (p->on_rq || (p->last_sleep_ts < rq->window_start))
		fixup_cum_window_demand(rq, p->ravg.demand);
#endif
}

void
walt_dec_cumulative_runnable_avg(struct rq *rq,
				 struct task_struct *p)
{
#ifdef CONFIG_SCHED_HISI_USE_WALT
	rq->cumulative_runnable_avg -= p->ravg.demand;
	BUG_ON((s64)rq->cumulative_runnable_avg < 0);

	/*
	 * on_rq will be 1 for sleeping tasks. So check if the task
	 * is migrating or dequeuing in RUNNING state to change the
	 * prio/cgroup/class.
	 */
	if (task_on_rq_migrating(p) || p->state == TASK_RUNNING)
		fixup_cum_window_demand(rq, -(s64)p->ravg.demand);
#endif
}

static void
fixup_cumulative_runnable_avg(struct rq *rq,
			      struct task_struct *p, u64 new_task_load)
{
#ifdef CONFIG_SCHED_HISI_USE_WALT
	s64 task_load_delta = (s64)new_task_load - task_load(p);

	rq->cumulative_runnable_avg += task_load_delta;
	if ((s64)rq->cumulative_runnable_avg < 0)
		panic("cra less than zero: tld: %lld, task_load(p) = %u\n",
			task_load_delta, task_load(p));

	fixup_cum_window_demand(rq, task_load_delta);
#endif
}

u64 walt_ktime_clock(void)
{
	if (unlikely(walt_ktime_suspended))
		return ktime_to_ns(ktime_last);
	return ktime_get_ns();
}

static void walt_resume(void)
{
	walt_ktime_suspended = false;
}

static int walt_suspend(void)
{
	ktime_last = ktime_get();
	walt_ktime_suspended = true;
	return 0;
}

static struct syscore_ops walt_syscore_ops = {
	.resume	= walt_resume,
	.suspend = walt_suspend
};

static int __init walt_init_ops(void)
{
	register_syscore_ops(&walt_syscore_ops);
	return 0;
}
late_initcall(walt_init_ops);

void walt_inc_cfs_cumulative_runnable_avg(struct cfs_rq *cfs_rq,
		struct task_struct *p)
{
#ifdef CONFIG_SCHED_HISI_USE_WALT
	cfs_rq->cumulative_runnable_avg += p->ravg.demand;
#endif
}

void walt_dec_cfs_cumulative_runnable_avg(struct cfs_rq *cfs_rq,
		struct task_struct *p)
{
#ifdef CONFIG_SCHED_HISI_USE_WALT
	cfs_rq->cumulative_runnable_avg -= p->ravg.demand;
#endif
}

#ifdef CONFIG_SCHED_HISI_WALT_WINDOW_SIZE_TUNABLE
static int __init set_walt_ravg_window(char *str)
{
	unsigned int adj_window;
	bool no_walt = walt_disabled;

	get_option(&str, &walt_ravg_window);

	/* Adjust for CONFIG_HZ */
	adj_window = (walt_ravg_window / TICK_NSEC) * TICK_NSEC;

	/* Warn if we're a bit too far away from the expected window size */
	WARN(adj_window < walt_ravg_window - NSEC_PER_MSEC,
	     "tick-adjusted window size %u, original was %u\n", adj_window,
	     walt_ravg_window);

	walt_ravg_window = adj_window;

	walt_disabled = walt_disabled ||
			(walt_ravg_window < MIN_SCHED_RAVG_WINDOW ||
			 walt_ravg_window > MAX_SCHED_RAVG_WINDOW);

	WARN(!no_walt && walt_disabled,
	     "invalid window size, disabling WALT\n");

	return 0;
}

early_param("walt_ravg_window", set_walt_ravg_window);
#endif

static void
update_window_start(struct rq *rq, u64 wallclock)
{
	s64 delta;
	int nr_windows;

	delta = wallclock - rq->window_start;
	/* If the MPM global timer is cleared, set delta as 0 to avoid kernel BUG happening */
	if (delta < 0) {
		delta = 0;
		WARN_ONCE(1, "WALT wallclock appears to have gone backwards or reset\n");
	}

	if (delta < walt_ravg_window)
		return;

	nr_windows = div64_u64(delta, walt_ravg_window);
	rq->window_start += (u64)nr_windows * (u64)walt_ravg_window;

#ifdef CONFIG_SCHED_HISI_CALC_CUM_WINDOW_DEMAND
	rq->cum_window_demand = rq->cumulative_runnable_avg;
#endif
}

/*
 * Translate absolute delta time accounted on a CPU
 * to a scale where 1024 is the capacity of the most
 * capable CPU running at FMAX
 */
static u64 scale_exec_time(u64 delta, struct rq *rq)
{
	unsigned long capcurr = capacity_curr_of(cpu_of(rq));

	return (delta * capcurr) >> SCHED_CAPACITY_SHIFT;
}

static int cpu_is_waiting_on_io(struct rq *rq)
{
#ifdef CONFIG_HISI_CPU_FREQ_GOV_SCHEDUTIL
	if (!walt_io_is_busy && !sched_io_is_busy)
		return 0;
#else
	if (!walt_io_is_busy)
		return 0;
#endif
	return atomic_read(&rq->nr_iowait);
}

void walt_account_irqtime(int cpu, struct task_struct *curr,
				 u64 delta, u64 wallclock)
{
	struct rq *rq = cpu_rq(cpu);
	unsigned long flags, nr_windows;
	u64 cur_jiffies_ts;

	raw_spin_lock_irqsave(&rq->lock, flags);

	/*
	 * cputime (wallclock) uses sched_clock so use the same here for
	 * consistency.
	 */
	delta += sched_clock() - wallclock;
	cur_jiffies_ts = get_jiffies_64();

	if (is_idle_task(curr))
		walt_update_task_ravg(curr, rq, IRQ_UPDATE, walt_ktime_clock(),
				 delta);

	nr_windows = cur_jiffies_ts - rq->irqload_ts;

	if (nr_windows) {
		if (nr_windows < 10) {
			/* Decay CPU's irqload by 3/4 for each window. */
			rq->avg_irqload *= (3 * nr_windows);
			rq->avg_irqload = div64_u64(rq->avg_irqload,
						    4 * nr_windows);
		} else {
			rq->avg_irqload = 0;
		}
		rq->avg_irqload += rq->cur_irqload;
		rq->cur_irqload = 0;
	}

	rq->cur_irqload += delta;
	rq->irqload_ts = cur_jiffies_ts;
	raw_spin_unlock_irqrestore(&rq->lock, flags);
}

#ifdef CONFIG_HISI_EAS_SCHED
#define WALT_HIGH_IRQ_TIMEOUT (20L * CONFIG_HZ / MSEC_PER_SEC)
#else
#define WALT_HIGH_IRQ_TIMEOUT 3
#endif

u64 walt_irqload(int cpu)
{
	struct rq *rq = cpu_rq(cpu);
	s64 delta;
	delta = get_jiffies_64() - rq->irqload_ts;

	/*
	 * Current context can be preempted by irq and rq->irqload_ts can be
	 * updated by irq context so that delta can be negative.
	 * But this is okay and we can safely return as this means there
	 * was recent irq occurrence.
	 */

	if (delta < WALT_HIGH_IRQ_TIMEOUT)
		return rq->avg_irqload;
	else
		return 0;
}

int walt_cpu_high_irqload(int cpu)
{
#ifdef CONFIG_SCHED_HISI_CHECK_IRQLOAD
	return walt_irqload(cpu) >= sysctl_sched_walt_cpu_high_irqload;
#else
	return 0;
#endif
}

#ifdef CONFIG_SCHED_HISI_TOP_TASK
static void top_task_load_update_history(struct rq *rq, struct task_struct *p,
				     u32 runtime, int samples, int event)
{
	u32 *hist;
	int ridx, widx;
	u32 max = 0, avg, load_avg;
	u64 sum = 0;
	unsigned int hist_size, stats_policy;

	hist = &p->ravg.load_sum_history[0];
	hist_size = rq->top_task_hist_size;
	stats_policy = rq->top_task_stats_policy;

	/* Clear windows out of hist_size */
	widx = RAVG_HIST_SIZE_MAX - 1;
	for (; widx >= hist_size; --widx)
		hist[widx] = 0;

	/* Push new 'runtime' value onto stack */
	ridx = widx - samples;
	for (; ridx >= 0; --widx, --ridx) {
		hist[widx] = hist[ridx];
		sum += hist[widx];
		if (hist[widx] > max)
			max = hist[widx];
	}

	for (widx = 0; widx < samples && widx < hist_size; widx++) {
		hist[widx] = runtime;
		sum += hist[widx];
		if (hist[widx] > max)
			max = hist[widx];
	}

	p->ravg.load_sum = 0;

	if (stats_policy == WINDOW_STATS_RECENT) {
		load_avg = runtime;
	} else if (stats_policy == WINDOW_STATS_MAX) {
		load_avg = max;
	} else {
		avg = div64_u64(sum, hist_size);
		if (stats_policy == WINDOW_STATS_AVG)
			load_avg = avg;
		else
			load_avg = max(avg, runtime);
	}

	p->ravg.load_avg = load_avg;

	return;
}

static void update_top_task_load(struct task_struct *p, struct rq *rq,
				int event, u64 wallclock, bool account_busy)
{
	u64 mark_start = p->ravg.mark_start;
	u64 delta, window_start = rq->window_start;
	int nr_full_windows;
	u32 window_size = walt_ravg_window;

	/* No need to bother updating task load for exiting tasks
	 * or the idle task. */
	if (exiting_task(p) || is_idle_task(p))
		return;

	delta = window_start - mark_start;
	nr_full_windows = div64_u64(delta, window_size);

	if (!account_busy) {
		/* Complete last non-empty window */
		top_task_load_update_history(rq, p, p->ravg.load_sum, 1, event);

		/* Push empty windows into history if wanted */
		if (nr_full_windows && rq->top_task_stats_empty_window)
			top_task_load_update_history(rq, p, 0, nr_full_windows, event);

		return;
	}

	/* Temporarily rewind window_start to first window boundary
	 * after mark_start. */
	window_start -= (u64)nr_full_windows * (u64)window_size;

	/* Process (window_start - mark_start) first */
	p->ravg.load_sum += scale_exec_time(window_start - mark_start, rq);

	/* Push new sample(s) into task's load history */
	top_task_load_update_history(rq, p, p->ravg.load_sum, 1, event);
	if (nr_full_windows)
		top_task_load_update_history(rq, p, scale_exec_time(window_size, rq),
			       nr_full_windows, event);

	/* Roll window_start back to current to process any remainder
	 * in current window. */
	window_start += (u64)nr_full_windows * (u64)window_size;

	/* Process (wallclock - window_start) next */
	mark_start = window_start;
	p->ravg.load_sum += scale_exec_time(wallclock - mark_start, rq);
}
#endif

#ifdef CONFIG_SCHED_HISI_MIGRATE_SPREAD_LOAD
static void record_task_contribution(struct task_struct *p,
					int cpu, int new_window)
{
	if (!cpumask_test_cpu(cpu, &p->ravg.curr_cpus))
		cpumask_set_cpu(cpu, &p->ravg.curr_cpus);

	if (new_window)
		cpumask_set_cpu(cpu, &p->ravg.prev_cpus);
}

static void rollover_task_contribution(struct task_struct *p,
					int nr_full_windows)
{
	if (!nr_full_windows)
		cpumask_copy(&p->ravg.prev_cpus, &p->ravg.curr_cpus);
	else
		cpumask_clear(&p->ravg.prev_cpus);

	cpumask_clear(&p->ravg.curr_cpus);
}
#endif

static int account_busy_for_cpu_time(struct rq *rq, struct task_struct *p,
				     u64 irqtime, int event)
{
	if (is_idle_task(p)) {
		/* TASK_WAKE && TASK_MIGRATE is not possible on idle task! */
		if (event == PICK_NEXT_TASK)
			return 0;

		/* PUT_PREV_TASK, TASK_UPDATE && IRQ_UPDATE are left */
		return irqtime || cpu_is_waiting_on_io(rq);
	}

	if (event == TASK_WAKE)
		return 0;

	if (event == PUT_PREV_TASK || event == IRQ_UPDATE)
		return 1;

	/*
	 * TASK_UPDATE can be called on sleeping task, when it's moved between
	 * related groups
	 */
	if (event == TASK_UPDATE) {
		if (rq->curr == p)
			return 1;

		return p->on_rq ? walt_freq_account_wait_time : 0;
	}

	/* Only TASK_MIGRATE && PICK_NEXT_TASK left */
	return walt_freq_account_wait_time;
}

static void
mark_util_change_for_rollover(struct task_struct *p, struct rq *rq)
{
	bool new_window = p->ravg.mark_start < rq->window_start;
	bool p_is_curr_task = (p == rq->curr);

	if (new_window && p_is_curr_task) {
		int cpu = cpu_of(rq);

		/* If window is rolled over, prevent rasing sugov worker
		 * unnecessarily in very low load case */
		if (rq->cluster->cur_freq == rq->cluster->min_freq) {
			unsigned long util = boosted_freq_policy_util(cpu);
			unsigned long capacity_curr = capacity_curr_of(cpu);

			if (util < (capacity_curr >> 1))
				return;
		}

		sugov_mark_util_change(cpu, WALT_WINDOW_ROLLOVER);
	}
}

static u32 empty_windows[NR_CPUS];

/*
 * Account cpu activity in its busy time counters (rq->curr/prev_runnable_sum)
 */
static void update_cpu_busy_time(struct task_struct *p, struct rq *rq,
	     int event, u64 wallclock, u64 irqtime)
{
	int new_window, nr_full_windows = 0;
	int p_is_curr_task = (p == rq->curr);
	u64 mark_start = p->ravg.mark_start;
	u64 window_start = rq->window_start;
	u32 window_size = walt_ravg_window;
	u64 delta;
	u64 *curr_runnable_sum = &rq->curr_runnable_sum;
	u64 *prev_runnable_sum = &rq->prev_runnable_sum;
	int cpu = rq->cpu;
#ifdef CONFIG_HISI_RTG
	struct group_cpu_time *cpu_time;
	struct related_thread_group *grp;
#endif

	new_window = mark_start < window_start;
	if (new_window) {
		nr_full_windows = div64_u64((window_start - mark_start),
						window_size);
		if (p->ravg.active_windows < USHRT_MAX)
			p->ravg.active_windows++;

#ifdef CONFIG_SCHED_HISI_TOP_TASK
		if (p_is_curr_task) {
			rollover_top_task_table(rq, nr_full_windows);
			trace_walt_window_rollover(cpu, nr_full_windows);
		}
#endif
	}

	/* A new window has started. The RQ demand must be rolled
	 * over if p is the current task. */
	if (p_is_curr_task && new_window) {
		u64 curr_sum = rq->curr_runnable_sum;

		if (nr_full_windows)
			curr_sum = 0;

		rq->prev_runnable_sum = curr_sum;
		rq->curr_runnable_sum = 0;
	}

#ifdef CONFIG_HISI_RTG
	rcu_read_lock();
	grp = task_related_thread_group(p);
	rcu_read_unlock();
	if (grp && rq->cluster == grp->preferred_cluster) {
		cpu_time = group_update_cpu_time(rq, p->grp);
		if (cpu_time) {
			curr_runnable_sum = &cpu_time->curr_runnable_sum;
			prev_runnable_sum = &cpu_time->prev_runnable_sum;
		}
	}
#endif

	/* Handle per-task window rollover. We don't care about the idle
	 * task or exiting tasks. */
	if (new_window && !is_idle_task(p) && !exiting_task(p)) {
		u32 *curr_cpu_windows = empty_windows;
		u32 curr_window = 0;
		int i;

		if (!nr_full_windows) {
			curr_window = p->ravg.curr_window;
			curr_cpu_windows = p->ravg.curr_window_cpu;
		}

		p->ravg.prev_window = curr_window;
		p->ravg.curr_window = 0;

#ifdef CONFIG_SCHED_HISI_TOP_TASK
		rollover_top_task_load(p, nr_full_windows);
#endif

#ifdef CONFIG_SCHED_HISI_MIGRATE_SPREAD_LOAD
		rollover_task_contribution(p, nr_full_windows);
#endif

		/* Roll over individual CPU contributions */
		for (i = 0; i < nr_cpu_ids; i++) {
			p->ravg.prev_window_cpu[i] = curr_cpu_windows[i];
			p->ravg.curr_window_cpu[i] = 0;
		}
	}

	if (!account_busy_for_cpu_time(rq, p, irqtime, event)) {
		/* account_busy_for_cpu_time() = 0, so no update to the
		 * task's current window needs to be made. This could be
		 * for example
		 *
		 *   - a wakeup event on a task within the current
		 *     window (!new_window below, no action required),
		 *   - switching to a new task from idle (PICK_NEXT_TASK)
		 *     in a new window where irqtime is 0 and we aren't
		 *     waiting on IO */

		if (!new_window)
			return;

#ifdef CONFIG_SCHED_HISI_TOP_TASK
		update_top_task_load(p, rq, event, wallclock, false);
		add_top_task(p, rq);
#endif

		return;
	}

#ifdef CONFIG_SCHED_HISI_MIGRATE_SPREAD_LOAD
	record_task_contribution(p, cpu, new_window);
#endif

	if (!new_window) {
		/* account_busy_for_cpu_time() = 1 so busy time needs
		 * to be accounted to the current window. No rollover
		 * since we didn't start a new window. An example of this is
		 * when a task starts execution and then sleeps within the
		 * same window. */

		if (!irqtime || !is_idle_task(p) || cpu_is_waiting_on_io(rq))
			delta = wallclock - mark_start;
		else
			delta = irqtime;
		delta = scale_exec_time(delta, rq);
		*curr_runnable_sum += delta;
		if (!is_idle_task(p) && !exiting_task(p)) {
			p->ravg.curr_window += delta;
			p->ravg.curr_window_cpu[cpu] += delta;
			p->ravg.load_sum += delta;
		}

		return;
	}

#ifdef CONFIG_SCHED_HISI_TOP_TASK
	update_top_task_load(p, rq, event, wallclock, true);
	add_top_task(p, rq);
#endif

	if (!p_is_curr_task) {
		/* account_busy_for_cpu_time() = 1 so busy time needs
		 * to be accounted to the current window. A new window
		 * has also started, but p is not the current task, so the
		 * window is not rolled over - just split up and account
		 * as necessary into curr and prev. The window is only
		 * rolled over when a new window is processed for the current
		 * task.
		 *
		 * Irqtime can't be accounted by a task that isn't the
		 * currently running task. */

		if (!nr_full_windows) {
			/* A full window hasn't elapsed, account partial
			 * contribution to previous completed window. */
			delta = scale_exec_time(window_start - mark_start, rq);
			if (!exiting_task(p)) {
				p->ravg.prev_window += delta;
				p->ravg.prev_window_cpu[cpu] += delta;
			}
		} else {
			/* Since at least one full window has elapsed,
			 * the contribution to the previous window is the
			 * full window (window_size). */
			delta = scale_exec_time(window_size, rq);
			if (!exiting_task(p)) {
				p->ravg.prev_window = delta;
				p->ravg.prev_window_cpu[cpu] = delta;
			}
		}
		*prev_runnable_sum += delta;

		/* Account piece of busy time in the current window. */
		delta = scale_exec_time(wallclock - window_start, rq);
		*curr_runnable_sum += delta;
		if (!exiting_task(p)) {
			p->ravg.curr_window = delta;
			p->ravg.curr_window_cpu[cpu] = delta;
		}

		return;
	}

	if (!irqtime || !is_idle_task(p) || cpu_is_waiting_on_io(rq)) {
		/* account_busy_for_cpu_time() = 1 so busy time needs
		 * to be accounted to the current window. A new window
		 * has started and p is the current task so rollover is
		 * needed. If any of these three above conditions are true
		 * then this busy time can't be accounted as irqtime.
		 *
		 * Busy time for the idle task or exiting tasks need not
		 * be accounted.
		 *
		 * An example of this would be a task that starts execution
		 * and then sleeps once a new window has begun. */

		if (!nr_full_windows) {
			/* A full window hasn't elapsed, account partial
			 * contribution to previous completed window. */
			delta = scale_exec_time(window_start - mark_start, rq);
			if (!is_idle_task(p) && !exiting_task(p)) {
				p->ravg.prev_window += delta;
				p->ravg.prev_window_cpu[cpu] += delta;
			}
		} else {
			/* Since at least one full window has elapsed,
			 * the contribution to the previous window is the
			 * full window (window_size). */
			delta = scale_exec_time(window_size, rq);
			if (!is_idle_task(p) && !exiting_task(p)) {
				p->ravg.prev_window = delta;
				p->ravg.prev_window_cpu[cpu] = delta;
			}

		}
		/*
		 * Rollover for normal runnable sum is done here by overwriting
		 * the values in prev_runnable_sum and curr_runnable_sum.
		 * Rollover for new task runnable sum has completed by previous
		 * if-else statement.
		 */
		*prev_runnable_sum += delta;

		/* Account piece of busy time in the current window. */
		delta = scale_exec_time(wallclock - window_start, rq);
		*curr_runnable_sum = delta;
		if (!is_idle_task(p) && !exiting_task(p)) {
			p->ravg.curr_window = delta;
			p->ravg.curr_window_cpu[cpu] = delta;
		}

		return;
	}

	if (irqtime) {
		/* account_busy_for_cpu_time() = 1 so busy time needs
		 * to be accounted to the current window. A new window
		 * has started and p is the current task so rollover is
		 * needed. The current task must be the idle task because
		 * irqtime is not accounted for any other task.
		 *
		 * Irqtime will be accounted each time we process IRQ activity
		 * after a period of idleness, so we know the IRQ busy time
		 * started at wallclock - irqtime. */

		BUG_ON(!is_idle_task(p));
		mark_start = wallclock - irqtime;

		/* Roll window over. If IRQ busy time was just in the current
		 * window then that is all that need be accounted. */
		rq->prev_runnable_sum = rq->curr_runnable_sum;
		if (mark_start > window_start) {
			*curr_runnable_sum = scale_exec_time(irqtime, rq);
			return;
		}

		/* The IRQ busy time spanned multiple windows. Process the
		 * busy time preceding the current window start first. */
		delta = window_start - mark_start;
		if (delta > window_size)
			delta = window_size;
		delta = scale_exec_time(delta, rq);
		*prev_runnable_sum += delta;

		/* Process the remaining IRQ busy time in the current window. */
		delta = wallclock - window_start;
		*curr_runnable_sum = scale_exec_time(delta, rq);

		return;
	}

	BUG();
}

static int account_busy_for_task_demand(struct task_struct *p, int event)
{
	/* No need to bother updating task demand for exiting tasks
	 * or the idle task. */
	if (exiting_task(p) || is_idle_task(p))
		return 0;

	/* When a task is waking up it is completing a segment of non-busy
	 * time. Likewise, if wait time is not treated as busy time, then
	 * when a task begins to run or is migrated, it is not running and
	 * is completing a segment of non-busy time. */
	if (event == TASK_WAKE || (!walt_account_wait_time &&
			 (event == PICK_NEXT_TASK || event == TASK_MIGRATE)))
		return 0;

	return 1;
}

/*
 * Called when new window is starting for a task, to record cpu usage over
 * recently concluded window(s). Normally 'samples' should be 1. It can be > 1
 * when, say, a real-time task runs without preemption for several windows at a
 * stretch.
 */
static void update_history(struct rq *rq, struct task_struct *p,
			 u32 runtime, int samples, int event)
{
	u32 *hist = &p->ravg.sum_history[0];
	int ridx, widx;
	u32 max = 0, avg, demand;
	u64 sum = 0;

	/* Ignore windows where task had no activity */
	if (!runtime || is_idle_task(p) || exiting_task(p) || !samples)
			goto done;

	/* Push new 'runtime' value onto stack */
	widx = walt_ravg_hist_size - 1;
	ridx = widx - samples;
	for (; ridx >= 0; --widx, --ridx) {
		hist[widx] = hist[ridx];
		sum += hist[widx];
		if (hist[widx] > max)
			max = hist[widx];
	}

	for (widx = 0; widx < samples && widx < walt_ravg_hist_size; widx++) {
		hist[widx] = runtime;
		sum += hist[widx];
		if (hist[widx] > max)
			max = hist[widx];
	}

	p->ravg.sum = 0;

	if (walt_window_stats_policy == WINDOW_STATS_RECENT) {
		demand = runtime;
	} else if (walt_window_stats_policy == WINDOW_STATS_MAX) {
		demand = max;
	} else {
		avg = div64_u64(sum, walt_ravg_hist_size);
		if (walt_window_stats_policy == WINDOW_STATS_AVG)
			demand = avg;
		else
			demand = max(avg, runtime);
	}

	/*
	 * A throttled deadline sched class task gets dequeued without
	 * changing p->on_rq. Since the dequeue decrements hmp stats
	 * avoid decrementing it here again.
	 *
	 * When window is rolled over, the cumulative window demand
	 * is reset to the cumulative runnable average (contribution from
	 * the tasks on the runqueue). If the current task is dequeued
	 * already, it's demand is not included in the cumulative runnable
	 * average. So add the task demand separately to cumulative window
	 * demand.
	 */
	if (!task_has_dl_policy(p) || !p->dl.dl_throttled) {
		if (task_on_rq_queued(p))
			fixup_cumulative_runnable_avg(rq, p, demand);
		else if (rq->curr == p)
			fixup_cum_window_demand(rq, demand);
	}

	p->ravg.demand = demand;

done:
	trace_walt_update_history(rq, p, runtime, samples, event);
	return;
}

static void add_to_task_demand(struct rq *rq, struct task_struct *p,
				u64 delta)
{
	delta = scale_exec_time(delta, rq);
	p->ravg.sum += delta;
	if (unlikely(p->ravg.sum > walt_ravg_window))
		p->ravg.sum = walt_ravg_window;
}

#ifdef CONFIG_HISI_RTG
static void add_to_group_task_time(struct related_thread_group *grp, struct rq *rq, struct task_struct *p, u64 wallclock)
{
	u64 mark_start = p->ravg.mark_start;
	u64 window_start = grp->window_start;
	u64 delta_exec, delta_load;

	if (unlikely(wallclock <= mark_start))
		return;

	/* per task load tracking in RTG */
	if (likely(mark_start >= window_start)) {
		/*
		 *   ws   ms  wc
		 *   |    |   |
		 *   V    V   V
		 *   |---------------|
		 */
		delta_exec = wallclock - mark_start;
		p->ravg.curr_window_exec += delta_exec;

		delta_load = scale_exec_time(delta_exec, rq);
		p->ravg.curr_window_load += delta_load;

	} else {
		/*
		 *   ms   ws  wc
		 *   |    |   |
		 *   V    V   V
		 *   -----|----------
		 */
		/* prev window task statistic */
		delta_exec = window_start - mark_start;
		p->ravg.prev_window_exec += delta_exec;

		delta_load = scale_exec_time(delta_exec, rq);
		p->ravg.prev_window_load += delta_load;

		/* curr window task statistic */
		delta_exec = wallclock - window_start;
		p->ravg.curr_window_exec += delta_exec;

		delta_load = scale_exec_time(delta_exec, rq);
		p->ravg.curr_window_load += delta_load;
	}
}

static void add_to_group_time(struct related_thread_group *grp, struct rq *rq, u64 wallclock)
{
	u64 delta_exec, delta_load;
	u64 mark_start = grp->mark_start;
	u64 window_start = grp->window_start;
	bool on_pref_cluster = (rq->cluster == grp->preferred_cluster);

	if (unlikely(wallclock <= mark_start))
		return;

	/* per group load tracking in RTG */
	if (likely(mark_start >= window_start)) {
		/*
		 *   ws   ms  wc
		 *   |    |   |
		 *   V    V   V
		 *   |---------------|
		 */
		delta_exec = wallclock - mark_start;
		grp->time.curr_window_exec += delta_exec;

		delta_load = scale_exec_time(delta_exec, rq);
		grp->time.curr_window_load += delta_load;

		if (on_pref_cluster) {
			grp->time_pref_cluster.curr_window_exec += delta_exec;
			grp->time_pref_cluster.curr_window_load += delta_load;
		}
	} else {
		/*
		 *   ms   ws  wc
		 *   |    |   |
		 *   V    V   V
		 *   -----|----------
		 */
		/* prev window statistic */
		delta_exec = window_start - mark_start;
		grp->time.prev_window_exec += delta_exec;

		delta_load = scale_exec_time(delta_exec, rq);
		grp->time.prev_window_load += delta_load;

		if (on_pref_cluster) {
			grp->time_pref_cluster.prev_window_exec += delta_exec;
			grp->time_pref_cluster.prev_window_load += delta_load;
		}

		/* curr window statistic */
		delta_exec = wallclock - window_start;
		grp->time.curr_window_exec += delta_exec;

		delta_load = scale_exec_time(delta_exec, rq);
		grp->time.curr_window_load += delta_load;

		if (on_pref_cluster) {
			grp->time_pref_cluster.curr_window_exec += delta_exec;
			grp->time_pref_cluster.curr_window_load += delta_load;
		}
	}
}

static inline void add_to_group_demand(struct related_thread_group *grp,
				struct rq *rq,
				struct task_struct *p, u64 wallclock)
{
	if (unlikely(wallclock <= grp->window_start))
		return;

	add_to_group_task_time(grp, rq, p, wallclock);
	add_to_group_time(grp, rq, wallclock);
}

static int account_busy_for_group_demand(struct task_struct *p, int event)
{
	/* No need to bother updating task demand for exiting tasks
	 * or the idle task. */
	if (exiting_task(p) || is_idle_task(p))
		return 0;

	if (event == TASK_WAKE || event == TASK_MIGRATE)
		return 0;

	return 1;
}

static void update_group_demand(struct task_struct *p, struct rq *rq,
				int event, u64 wallclock)
{
	struct related_thread_group *grp;

	if (!account_busy_for_group_demand(p, event))
		return;

	rcu_read_lock();
	grp = task_related_thread_group(p);
	if (!grp) {
		rcu_read_unlock();
		return;
	}

	raw_spin_lock(&grp->lock);
	if (grp->nr_running == 1)
		grp->mark_start = max(grp->mark_start, p->ravg.mark_start);

	add_to_group_demand(grp, rq, p, wallclock);

	grp->mark_start = wallclock;

	raw_spin_unlock(&grp->lock);

	rcu_read_unlock();
}

static void update_group_nr_running(struct task_struct *p, int event, u64 wallclock)
{
	struct related_thread_group *grp;

	rcu_read_lock();
	grp = task_related_thread_group(p);
	if (!grp) {
		rcu_read_unlock();
		return;
	}

	raw_spin_lock(&grp->lock);

	if (event == PICK_NEXT_TASK)
		grp->nr_running++;
	else if (event == PUT_PREV_TASK)
		grp->nr_running--;

	if ((int)grp->nr_running < 0) {
		WARN_ON(1);
		grp->nr_running = 0;
	}

	raw_spin_unlock(&grp->lock);

	rcu_read_unlock();
}

#else
static inline void update_group_demand(struct task_struct *p, struct rq *rq, int event, u64 wallclock) { }
static inline void update_group_nr_running(struct task_struct *p, int event, u64 wallclock) { }
#endif

/*
 * Account cpu demand of task and/or update task's cpu demand history
 *
 * ms = p->ravg.mark_start;
 * wc = wallclock
 * ws = rq->window_start
 *
 * Three possibilities:
 *
 *	a) Task event is contained within one window.
 *		window_start < mark_start < wallclock
 *
 *		ws   ms  wc
 *		|    |   |
 *		V    V   V
 *		|---------------|
 *
 *	In this case, p->ravg.sum is updated *iff* event is appropriate
 *	(ex: event == PUT_PREV_TASK)
 *
 *	b) Task event spans two windows.
 *		mark_start < window_start < wallclock
 *
 *		ms   ws   wc
 *		|    |    |
 *		V    V    V
 *		-----|-------------------
 *
 *	In this case, p->ravg.sum is updated with (ws - ms) *iff* event
 *	is appropriate, then a new window sample is recorded followed
 *	by p->ravg.sum being set to (wc - ws) *iff* event is appropriate.
 *
 *	c) Task event spans more than two windows.
 *
 *		ms ws_tmp			   ws  wc
 *		|  |				   |   |
 *		V  V				   V   V
 *		---|-------|-------|-------|-------|------
 *		   |				   |
 *		   |<------ nr_full_windows ------>|
 *
 *	In this case, p->ravg.sum is updated with (ws_tmp - ms) first *iff*
 *	event is appropriate, window sample of p->ravg.sum is recorded,
 *	'nr_full_window' samples of window_size is also recorded *iff*
 *	event is appropriate and finally p->ravg.sum is set to (wc - ws)
 *	*iff* event is appropriate.
 *
 * IMPORTANT : Leave p->ravg.mark_start unchanged, as update_cpu_busy_time()
 * depends on it!
 */
#ifdef CONFIG_SCHED_HISI_USE_WALT
static void update_task_demand(struct task_struct *p, struct rq *rq,
	     int event, u64 wallclock)
{
	u64 mark_start = p->ravg.mark_start;
	u64 delta, window_start = rq->window_start;
	int new_window, nr_full_windows;
	u32 window_size = walt_ravg_window;

	update_group_demand(p, rq, event, wallclock);

	new_window = mark_start < window_start;
	if (!account_busy_for_task_demand(p, event)) {
		if (new_window)
			/* If the time accounted isn't being accounted as
			 * busy time, and a new window started, only the
			 * previous window need be closed out with the
			 * pre-existing demand. Multiple windows may have
			 * elapsed, but since empty windows are dropped,
			 * it is not necessary to account those. */
			update_history(rq, p, p->ravg.sum, 1, event);
		return;
	}

	if (!new_window) {
		/* The simple case - busy time contained within the existing
		 * window. */
		add_to_task_demand(rq, p, wallclock - mark_start);
		return;
	}

	/* Busy time spans at least two windows. Temporarily rewind
	 * window_start to first window boundary after mark_start. */
	delta = window_start - mark_start;
	nr_full_windows = div64_u64(delta, window_size);
	window_start -= (u64)nr_full_windows * (u64)window_size;

	/* Process (window_start - mark_start) first */
	add_to_task_demand(rq, p, window_start - mark_start);

	/* Push new sample(s) into task's demand history */
	update_history(rq, p, p->ravg.sum, 1, event);
	if (nr_full_windows)
		update_history(rq, p, scale_exec_time(window_size, rq),
			       nr_full_windows, event);

	/* Roll window_start back to current to process any remainder
	 * in current window. */
	window_start += (u64)nr_full_windows * (u64)window_size;

	/* Process (wallclock - window_start) next */
	mark_start = window_start;
	add_to_task_demand(rq, p, wallclock - mark_start);
}
#endif

/* Reflect task activity on its demand and cpu's busy time statistics */
void walt_update_task_ravg(struct task_struct *p, struct rq *rq,
	     int event, u64 wallclock, u64 irqtime)
{
	unsigned long flags;

	if (walt_disabled || !rq->window_start)
		return;

	lockdep_assert_held(&rq->lock);
	raw_spin_lock_irqsave(&rq->walt_update_lock, flags);

	update_window_start(rq, wallclock);
	update_group_nr_running(p, event, wallclock);

	if (!p->ravg.mark_start)
		goto done;

#ifdef CONFIG_SCHED_HISI_USE_WALT
	update_task_demand(p, rq, event, wallclock);
#endif

	update_cpu_busy_time(p, rq, event, wallclock, irqtime);

	mark_util_change_for_rollover(p, rq);

done:
	raw_spin_unlock_irqrestore(&rq->walt_update_lock, flags);
	trace_walt_update_task_ravg(p, rq, event, wallclock, irqtime);

	p->ravg.mark_start = wallclock;
}

void reset_task_stats(struct task_struct *p)
{
	u32 sum = 0;

	memset(&p->ravg, 0, sizeof(struct ravg));

	/* Retain EXITING_TASK marker */
	p->ravg.sum_history[0] = sum;
}

void walt_mark_task_starting(struct task_struct *p)
{
	u64 wallclock;
	struct rq *rq = task_rq(p);

	if (!rq->window_start) {
		reset_task_stats(p);
		return;
	}

	/*
	 * Add the new task to top tasks.
	 * Called in wake_up_new_task(), after task load initialized.
	 */
	add_top_task(p, rq);

	wallclock = walt_ktime_clock();
	p->ravg.mark_start = wallclock;
#ifdef CONFIG_HISI_ED_TASK
	p->last_wake_ts = wallclock;
#endif
}

void walt_set_window_start(struct rq *rq)
{
	int cpu = cpu_of(rq);
	struct rq *sync_rq = cpu_rq(sync_cpu);

	if (likely(rq->window_start))
		return;

	if (cpu == sync_cpu) {
		rq->window_start = 1;
	} else {
		raw_spin_unlock(&rq->lock);
		double_rq_lock(rq, sync_rq);
		rq->window_start = cpu_rq(sync_cpu)->window_start;
		rq->curr_runnable_sum = rq->prev_runnable_sum = 0;
		raw_spin_unlock(&sync_rq->lock);
	}

	rq->curr->ravg.mark_start = rq->window_start;
}

#ifdef CONFIG_HISI_CPU_ISOLATION
void walt_migrate_sync_cpu(int cpu, int new_cpu)
{
	if (cpu == sync_cpu)
		sync_cpu = new_cpu;
}
#else
void walt_migrate_sync_cpu(int cpu)
{
	if (cpu == sync_cpu)
		sync_cpu = smp_processor_id();
}
#endif

static inline bool in_range(u64 x, u64 begin, u64 end)
{
	return begin <= x && x < end;
}

static void
move_out_cpu_busy_time(struct rq *rq, struct task_struct *p)
{
	int cpu = cpu_of(rq);
	u64 mark_start = p->ravg.mark_start;
	u64 window_start = rq->window_start;
	u32 window_size = walt_ravg_window;
	u64 window_end = window_start + window_size;

	if (in_range(mark_start, window_start, window_end)) {
		rq->curr_runnable_sum -= p->ravg.curr_window_cpu[cpu];
		rq->prev_runnable_sum -= p->ravg.prev_window_cpu[cpu];
	} else if (in_range(mark_start, window_start - window_size, window_start))
		rq->prev_runnable_sum -= p->ravg.curr_window_cpu[cpu];
	else if (in_range(mark_start, window_end, window_end + window_size))
		rq->curr_runnable_sum -= p->ravg.prev_window_cpu[cpu];

	p->ravg.curr_window_cpu[cpu] = 0;
	p->ravg.prev_window_cpu[cpu] = 0;

	if ((s64)rq->prev_runnable_sum < 0) {
#ifndef CONFIG_HISI_EAS_SCHED
		BUG_ON((s64)rq->prev_runnable_sum < 0);
#endif
		rq->prev_runnable_sum = 0;
	}

	if ((s64)rq->curr_runnable_sum < 0) {
#ifndef CONFIG_HISI_EAS_SCHED
		BUG_ON((s64)rq->curr_runnable_sum < 0);
#endif
		rq->curr_runnable_sum = 0;
	}
}

static void
migrate_cpu_busy_time(struct task_struct *p,
		      struct rq *src_rq, struct rq *dest_rq)
{
	int new_cpu = cpu_of(dest_rq);
	int src_cpu = cpu_of(src_rq);
#ifdef CONFIG_SCHED_HISI_MIGRATE_SPREAD_LOAD
	cpumask_t prev_cpus, curr_cpus;
	u32 each_load;
#endif
	unsigned long flags;
	int i;

#ifdef CONFIG_SCHED_HISI_DOWNMIGRATE_LOWER_LOAD
	/* For task downmigrate, lower task's prev/curr window to prevent
	 * little cluster's freq increase too much. */
	if (capacity_orig_of(src_cpu) > capacity_orig_of(new_cpu)) {
		u32 task_load = task_load_freq_avg(p);

		if (unlikely(is_new_task(p)))
			task_load = UINT_MAX;

		p->ravg.curr_window = min(p->ravg.curr_window, task_load);
		p->ravg.prev_window = min(p->ravg.prev_window, task_load);
	}
#endif

	/* Add task's prev/curr window to dest */
#ifdef CONFIG_SCHED_HISI_MIGRATE_SPREAD_LOAD
	/* If p has run on dest cluster in prev/curr window, share
	 * p's load in these cpus. */
	cpumask_and(&prev_cpus, &p->ravg.prev_cpus, &dest_rq->cluster->cpus);
	cpumask_set_cpu(new_cpu, &prev_cpus);
	each_load = p->ravg.prev_window / cpumask_weight(&prev_cpus);

	for_each_cpu(i, &prev_cpus) {
		struct rq *rq = cpu_rq(i);
		raw_spin_lock_irqsave(&rq->walt_update_lock, flags);
		rq->prev_runnable_sum += each_load;
		raw_spin_unlock_irqrestore(&rq->walt_update_lock, flags);

		p->ravg.prev_window_cpu[i] = each_load;
	}

	cpumask_and(&curr_cpus, &p->ravg.curr_cpus, &dest_rq->cluster->cpus);
	cpumask_set_cpu(new_cpu, &curr_cpus);
	each_load = p->ravg.curr_window / cpumask_weight(&curr_cpus);

	for_each_cpu(i, &curr_cpus) {
		struct rq *rq = cpu_rq(i);
		raw_spin_lock_irqsave(&rq->walt_update_lock, flags);
		rq->curr_runnable_sum += each_load;
		raw_spin_unlock_irqrestore(&rq->walt_update_lock, flags);

		p->ravg.curr_window_cpu[i] = each_load;
	}
#else
	/* All load move to dest_rq */
	raw_spin_lock_irqsave(&dest_rq->walt_update_lock, flags);
	dest_rq->curr_runnable_sum += p->ravg.curr_window;
	dest_rq->prev_runnable_sum += p->ravg.prev_window;

	p->ravg.curr_window_cpu[new_cpu] = p->ravg.curr_window;
	p->ravg.prev_window_cpu[new_cpu] = p->ravg.prev_window;
	raw_spin_unlock_irqrestore(&dest_rq->walt_update_lock, flags);
#endif

	/* Delete task's prev/curr window from src */
	for_each_cpu(i, &src_rq->cluster->cpus) {
		struct rq *rq = cpu_rq(i);
		if (!p->ravg.curr_window_cpu[i] && !p->ravg.prev_window_cpu[i])
			continue;

		raw_spin_lock_irqsave(&rq->walt_update_lock, flags);
		move_out_cpu_busy_time(rq, p);
		raw_spin_unlock_irqrestore(&rq->walt_update_lock, flags);
	}

	trace_walt_migration_update_sum(src_rq, p);
	trace_walt_migration_update_sum(dest_rq, p);
}

#ifdef CONFIG_HISI_MIGRATION_NOTIFY
static inline bool
nearly_same_freq(struct rq *rq, unsigned int cur_freq, unsigned int freq_required)
{
	int delta = freq_required - cur_freq;

	if (freq_required > cur_freq)
		return delta < rq->freq_inc_notify;

	delta = -delta;
	return delta < rq->freq_dec_notify;
}

static inline unsigned int util_to_freq(int cpu, u32 util)
{
	unsigned int max_cap = arch_scale_cpu_capacity(NULL, cpu);
	unsigned int max_freq = cpu_rq(cpu)->cluster->max_freq;

	return mult_frac(max_freq, util, max_cap);
}

static inline unsigned int estimate_freq_required(int cpu)
{
	return util_to_freq(cpu, boosted_freq_policy_util(cpu));
}
#else
static inline bool
nearly_same_freq(struct rq *rq, unsigned int cur_freq, unsigned int freq_required)
{
	return true;
}
static inline unsigned int estimate_freq_required(int cpu)
{
	return 0;
}
#endif

static void
inter_cluster_migration_fixup(struct task_struct *p,
			      struct rq *src_rq, struct rq *dest_rq)
{
	int src_cpu = cpu_of(src_rq), dest_cpu = cpu_of(dest_rq);
	unsigned int src_freq_before, dest_freq_before;
	unsigned int src_freq_after, dest_freq_after;
	unsigned int flags;

	src_freq_before  = estimate_freq_required(src_cpu);
	dest_freq_before = estimate_freq_required(dest_cpu);

	migrate_top_task(p, src_rq, dest_rq);
	migrate_cpu_busy_time(p, src_rq, dest_rq);

	src_freq_after = estimate_freq_required(src_cpu);
	dest_freq_after = estimate_freq_required(dest_cpu);

	if (src_rq->cluster->cur_freq != src_rq->cluster->min_freq &&
	    !nearly_same_freq(src_rq, src_freq_before, src_freq_after))
		sugov_mark_util_change(src_cpu, INTER_CLUSTER_MIGRATION_SRC);

	if (dest_rq->cluster->cur_freq != dest_rq->cluster->max_freq &&
	    !nearly_same_freq(dest_rq, dest_freq_before, dest_freq_after)) {
		flags = INTER_CLUSTER_MIGRATION_DST;

		if (top_task_util(dest_rq) > capacity_curr_of(dest_cpu))
			flags |= ADD_TOP_TASK;

		sugov_mark_util_change(dest_cpu, flags);
	}
}

void walt_fixup_busy_time(struct task_struct *p, int new_cpu)
{
	struct rq *src_rq = task_rq(p);
	struct rq *dest_rq = cpu_rq(new_cpu);
	u64 wallclock;
	int src_cpu = task_cpu(p);

	if (!p->on_rq && p->state != TASK_WAKING)
		return;

	if (exiting_task(p)) {
#ifdef CONFIG_HISI_ED_TASK
		clear_ed_task(p, src_rq);
#endif
		return;
	}

	if (p->state == TASK_WAKING)
		double_rq_lock(src_rq, dest_rq);

	wallclock = walt_ktime_clock();

	walt_update_task_ravg(task_rq(p)->curr, task_rq(p),
			TASK_UPDATE, wallclock, 0);
	walt_update_task_ravg(dest_rq->curr, dest_rq,
			TASK_UPDATE, wallclock, 0);

	walt_update_task_ravg(p, task_rq(p), TASK_MIGRATE, wallclock, 0);

	/*
	 * When a task is migrating during the wakeup, adjust
	 * the task's contribution towards cumulative window
	 * demand.
	 */
	if (p->state == TASK_WAKING &&
	    p->last_sleep_ts >= src_rq->window_start) {
		fixup_cum_window_demand(src_rq, -(s64)p->ravg.demand);
		fixup_cum_window_demand(dest_rq, p->ravg.demand);
	}

	if (!group_migrate_task(p, src_rq, dest_rq) &&
	    !same_freq_domain(new_cpu, src_cpu)) {
		inter_cluster_migration_fixup(p, src_rq, dest_rq);
	} else {
		/* Only need to migrate top task when same cluster */
		migrate_top_task(p, src_rq, dest_rq);
	}

#ifdef CONFIG_HISI_ED_TASK
	migrate_ed_task(p, src_rq, dest_rq);
#endif
	if (p->state == TASK_WAKING)
		double_rq_unlock(src_rq, dest_rq);
}

static int cpufreq_notifier_policy(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_policy *policy = (struct cpufreq_policy *)data;
	struct sched_cluster *cluster = NULL;
	struct cpumask policy_cluster = *policy->related_cpus;
	int i, j;

	if (val != CPUFREQ_NOTIFY)
		return 0;

	for_each_cpu(i, &policy_cluster) {
		cluster = cpu_rq(i)->cluster;
		cpumask_andnot(&policy_cluster, &policy_cluster,
			        &cluster->cpus);

		cluster->min_freq = policy->min;
		cluster->max_freq = policy->max;

		if (!cluster->freq_init_done) {
			for_each_cpu(j, &cluster->cpus)
				cpumask_copy(&cpu_rq(j)->freq_domain_cpumask,
					      policy->related_cpus);
			cluster->freq_init_done = true;
			continue;
		}
	}

	return 0;
}

static int cpufreq_notifier_trans(struct notifier_block *nb,
		unsigned long val, void *data)
{
	struct cpufreq_freqs *freq = (struct cpufreq_freqs *)data;
	unsigned int cpu = freq->cpu, new_freq = freq->new;
	struct sched_cluster *cluster;
	struct cpumask policy_cpus = cpu_rq(cpu)->freq_domain_cpumask;
	int i;

	if (val != CPUFREQ_POSTCHANGE)
		return 0;

	BUG_ON(!new_freq);

	for_each_cpu(i, &policy_cpus) {
		cluster = cpu_rq(i)->cluster;

		cluster->cur_freq = new_freq;
		cpumask_andnot(&policy_cpus, &policy_cpus, &cluster->cpus);
	}

	return 0;
}

static struct notifier_block notifier_policy_block = {
	.notifier_call = cpufreq_notifier_policy
};

static struct notifier_block notifier_trans_block = {
	.notifier_call = cpufreq_notifier_trans
};

static int register_sched_callback(void)
{
	int ret;

	ret = cpufreq_register_notifier(&notifier_policy_block,
						CPUFREQ_POLICY_NOTIFIER);

	if (!ret)
		ret = cpufreq_register_notifier(&notifier_trans_block,
						CPUFREQ_TRANSITION_NOTIFIER);

	return 0;
}

/*
 * cpufreq callbacks can be registered at core_initcall or later time.
 * Any registration done prior to that is "forgotten" by cpufreq. See
 * initialization of variable init_cpufreq_transition_notifier_list_called
 * for further information.
 */
core_initcall(register_sched_callback);

void walt_init_new_task_load(struct task_struct *p)
{
	int i;
	u32 init_load_windows =
			div64_u64((u64)sysctl_sched_walt_init_task_load_pct *
                          (u64)walt_ravg_window, 100);
	u32 init_load_pct = current->init_load_pct;

	p->init_load_pct = 0;
	memset(&p->ravg, 0, sizeof(struct ravg));

	init_task_rtg(p);

	if (init_load_pct) {
		init_load_windows = div64_u64((u64)init_load_pct *
			  (u64)walt_ravg_window, 100);
	}

	p->ravg.demand = init_load_windows;
	for (i = 0; i < RAVG_HIST_SIZE_MAX; ++i)
		p->ravg.sum_history[i] = init_load_windows;
}
