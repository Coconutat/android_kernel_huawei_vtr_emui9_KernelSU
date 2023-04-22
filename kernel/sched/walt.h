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
 */

#ifndef __WALT_H
#define __WALT_H

#ifdef CONFIG_SCHED_WALT

#define WINDOW_STATS_RECENT		0
#define WINDOW_STATS_MAX		1
#define WINDOW_STATS_MAX_RECENT_AVG	2
#define WINDOW_STATS_AVG		3
#define WINDOW_STATS_INVALID_POLICY	4

#ifdef CONFIG_SCHED_HISI_TOP_TASK
#define DEFAULT_TOP_TASK_HIST_SIZE		RAVG_HIST_SIZE_MAX
#define DEFAULT_TOP_TASK_STATS_POLICY		WINDOW_STATS_RECENT
#define DEFAULT_TOP_TASK_STATS_EMPTY_WINDOW	false
#endif

void walt_update_task_ravg(struct task_struct *p, struct rq *rq, int event,
		u64 wallclock, u64 irqtime);
void walt_inc_cumulative_runnable_avg(struct rq *rq, struct task_struct *p);
void walt_dec_cumulative_runnable_avg(struct rq *rq, struct task_struct *p);
void walt_inc_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p);
void walt_dec_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p);
void walt_fixup_busy_time(struct task_struct *p, int new_cpu);
void walt_init_new_task_load(struct task_struct *p);
void walt_mark_task_starting(struct task_struct *p);
void walt_set_window_start(struct rq *rq);
#ifdef CONFIG_HISI_CPU_ISOLATION
void walt_migrate_sync_cpu(int cpu, int new_cpu);
#else
void walt_migrate_sync_cpu(int cpu);
#endif
void walt_init_cpu_efficiency(void);
u64 walt_ktime_clock(void);
void walt_account_irqtime(int cpu, struct task_struct *curr, u64 delta,
                                  u64 wallclock);

u64 walt_irqload(int cpu);
int walt_cpu_high_irqload(int cpu);

extern void reset_task_stats(struct task_struct *p);

#define NEW_TASK_WINDOWS 5
static inline bool is_new_task(struct task_struct *p)
{
	return p->ravg.active_windows < NEW_TASK_WINDOWS;
}

#else /* CONFIG_SCHED_WALT */

static inline void walt_update_task_ravg(struct task_struct *p, struct rq *rq,
		int event, u64 wallclock, u64 irqtime) { }
static inline void walt_inc_cumulative_runnable_avg(struct rq *rq, struct task_struct *p) { }
static inline void walt_dec_cumulative_runnable_avg(struct rq *rq, struct task_struct *p) { }
static inline void walt_inc_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p) { }
static inline void walt_dec_cfs_cumulative_runnable_avg(struct cfs_rq *rq,
		struct task_struct *p) { }
static inline void walt_fixup_busy_time(struct task_struct *p, int new_cpu) { }
static inline void walt_init_new_task_load(struct task_struct *p) { }
static inline void walt_mark_task_starting(struct task_struct *p) { }
static inline void walt_set_window_start(struct rq *rq) { }
#ifdef CONFIG_HISI_CPU_ISOLATION
static inline void walt_migrate_sync_cpu(int cpu, int new_cpu) { }
#else
static inline void walt_migrate_sync_cpu(int cpu) { }
#endif
static inline void walt_init_cpu_efficiency(void) { }
static inline u64 walt_ktime_clock(void) { return 0; }

#define walt_cpu_high_irqload(cpu) false

#define is_new_task(p) false

#endif /* CONFIG_SCHED_WALT */

#ifdef CONFIG_SCHED_HISI_WALT_WINDOW_SIZE_TUNABLE
extern bool walt_disabled;
#else
extern const bool walt_disabled;
#endif

#endif
