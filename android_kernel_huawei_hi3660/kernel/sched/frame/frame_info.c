/*
 *  Frame-based load tracking for  rt_frame and hisi_rtg
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; version 2
 *  of the License.
 */

#include <linux/sched.h>
#include <linux/cpufreq.h>

#ifdef CONFIG_TRACE_FRAME_SYSTRACE
#include <trace/events/power.h>
#endif

#include <../kernel/sched/sched.h>
#include <linux/hisi_rtg.h>

#include "frame_info.h"

/* QOS value : [0, 120] */
#define DEFAULT_FRAME_QOS		60
#define MIN_FRAME_QOS			0
#define MAX_FRAME_QOS			120

/* MARGIN value : [-100, 100] */
#define DEFAULT_VLOAD_MARGIN		16
#define MIN_VLOAD_MARGIN		-100
#define MAX_VLOAD_MARGIN		100


#define FRAME_MAX_VLOAD			SCHED_CAPACITY_SCALE
#define FRAME_MAX_LOAD			SCHED_CAPACITY_SCALE
#define FRAME_UTIL_INVALID_FACTOR	4

static inline struct related_thread_group *frame_rtg(void)
{
	return lookup_related_thread_group(DEFAULT_RT_FRAME_ID);
}

static inline struct frame_info *
__rtg_frame_info(struct related_thread_group *grp)
{
	return (struct frame_info *) grp->private_data;
}

struct frame_info *rtg_frame_info(void)
{
	struct frame_info *frame_info = NULL;
	struct related_thread_group *grp = frame_rtg();

	if (!grp)
		return NULL;

	frame_info = __rtg_frame_info(grp);

	return frame_info;
}

int set_frame_rate(int qos)
{
	struct frame_info *frame_info =	NULL;

	if (qos < MIN_FRAME_QOS || qos > MAX_FRAME_QOS) {
		pr_err("[%s] [IPROVISION-FRAME_INFO] invalid QOS(rate) value", __func__);
		return -EINVAL;
	}

	frame_info = rtg_frame_info();

	if (!frame_info)
		return -EIO;

	frame_info->qos_frame = qos;
	frame_info->qos_frame_time = NSEC_PER_SEC / qos;
	frame_info->max_vload_time = frame_info->qos_frame_time / NSEC_PER_MSEC
		+ frame_info->vload_margin;
	FRAME_SYSTRACE("FRAME_QOS", qos, smp_processor_id());

	return 0;
}
EXPORT_SYMBOL_GPL(set_frame_rate);

int set_frame_margin(int margin)
{
	struct frame_info *frame_info = NULL;

	if (margin < MIN_VLOAD_MARGIN || margin > MAX_VLOAD_MARGIN) {
		pr_err("[%s] [IPROVISION-FRAME_INFO] invalid MARGIN value", __func__);
		return -EINVAL;
	}

	frame_info = rtg_frame_info();

	if (!frame_info)
		return -EIO;

	frame_info->vload_margin = margin;
	frame_info->max_vload_time = frame_info->qos_frame_time / NSEC_PER_MSEC
		+ frame_info->vload_margin;
	FRAME_SYSTRACE("FRAME_MARGIN", margin, smp_processor_id());

	return 0;
}
EXPORT_SYMBOL_GPL(set_frame_margin);

/*
 * frame_vload [0~1024]
 * vtime = now - timestamp
 * max_time = frame_info->qos_frame_time + vload_margin
 * load = F(vtime)
 *	= vtime ^ 2 - vtime * max_time + FRAME_MAX_VLOAD * vtime / max_time;
 *	= vtime * (vtime + FRAME_MAX_VLOAD / max_time - max_time);
 * [0, 0] -=> [max_time, FRAME_MAX_VLOAD]
 *
 */
u64 calc_frame_vload(struct frame_info *frame_info, u64 timeline)
{
	u64 vload;
	int vtime = timeline / NSEC_PER_MSEC;
	int max_time = frame_info->max_vload_time;
	int factor = 0;

	if (max_time <= 0 || vtime > max_time)
		return FRAME_MAX_VLOAD;

	factor = vtime + FRAME_MAX_VLOAD / max_time;
	/* margin maybe negative */
	if (vtime <= 0 || factor <= max_time)
		return 0;

	vload = (u64)vtime * (u64)(factor - max_time);

	return vload;
}

/*
 * real_util = max(last_util, virtual_util, boost_util, phase_util)
 * boost_util : TODO
 * phase_util : TODO
 */
static inline u64 calc_frame_util(struct frame_info *frame_info)
{
	return max(frame_info->frame_last_load_util, frame_info->frame_vload);
}

static inline u64 calc_frame_last_load_util(struct frame_info *frame_info)
{
	u64 frame_last_load = frame_info_rtg_load(frame_info)->prev_window_load;

	FRAME_SYSTRACE("frame_last_load", frame_last_load, smp_processor_id());

	if (frame_last_load >= frame_info->qos_frame_time)
		return FRAME_MAX_LOAD;
	else
		return (frame_last_load << SCHED_CAPACITY_SHIFT) /
			frame_info->qos_frame_time;
}

static inline bool
check_frame_util_invalid(struct frame_info *frame_info, u64 timeline)
{
	if (frame_info->frame_invalid)
		return true;
	else if (frame_info_rtg(frame_info)->util_invalid_interval <= timeline &&
		frame_info_rtg_load(frame_info)->curr_window_exec * FRAME_UTIL_INVALID_FACTOR
		<= timeline) {
		return true;
	}
	return false;
}

/*
 * update CPUFREQ use last_load_vutil when new frame start
 */
static inline void update_frame_info_first(struct frame_info *frame_info)
{
	u64 last_load_util = frame_info->frame_last_load_util;

	FRAME_SYSTRACE("frame_util", last_load_util, smp_processor_id());
	sched_set_group_normalized_util(DEFAULT_RT_FRAME_ID,
		last_load_util, FRAME_FORCE_UPDATE);
}

/*
 * update CPUFREQ and PLACEMENT when frame task running (in tick) and migration
 */
void update_frame_info_tick(struct related_thread_group *grp)
{
	u64 window_start = 0, wallclock = 0, timeline = 0;
	struct frame_info *frame_info = NULL;

	rcu_read_lock();
	if (grp->id != DEFAULT_RT_FRAME_ID) {
		rcu_read_unlock();
		return;
	}
	frame_info = __rtg_frame_info(grp);
	window_start = grp->window_start;
	rcu_read_unlock();

	BUG_ON(!frame_info);
	wallclock = ktime_get_ns();
	timeline = wallclock - window_start;

	FRAME_SYSTRACE("update_curr_pid", current->pid, smp_processor_id());
	FRAME_SYSTRACE("frame_timeline", timeline / NSEC_PER_MSEC, smp_processor_id());

	/* check frame_util invalid */
	if (!check_frame_util_invalid(frame_info, timeline)) {
		/* frame_vload statistic */
		frame_info->frame_vload =
			calc_frame_vload(frame_info, timeline);
		FRAME_SYSTRACE("frame_vload", frame_info->frame_vload, smp_processor_id());

		/* frame_util statistic */
		frame_info->frame_util = calc_frame_util(frame_info);
	} else {
		if (frame_info->frame_invalid)
			return;

		frame_info->frame_invalid = true;
		FRAME_SYSTRACE("frame_invalid", 1, smp_processor_id());

		/* only update frame_util once when invalid */
		frame_info->frame_util = 0;
	}

	FRAME_SYSTRACE("frame_util", frame_info->frame_util, smp_processor_id());
	sched_set_group_normalized_util(DEFAULT_RT_FRAME_ID,
		frame_info->frame_util, FRAME_NORMAL_UPDATE);
}

static inline void set_frame_end(struct frame_info *frame_info, u64 timestamp)
{
	if (likely(!frame_info->frame_invalid)) {
		/* last frame load tracking */
		frame_info->frame_last_load_util =
			calc_frame_last_load_util(frame_info);
		FRAME_SYSTRACE("frame_last_load_util", frame_info->frame_last_load_util, smp_processor_id());
	} else { /* skip the statistic of last_frame when last_frame_invalid */
		frame_info->frame_invalid = false;
		FRAME_SYSTRACE("frame_invalid", 0, smp_processor_id());
	}

	frame_info->frame_vload = 0;

	FRAME_SYSTRACE("FRAME_END", current->pid, smp_processor_id());
	FRAME_SYSTRACE("frame_vload", 0, smp_processor_id());

	FRAME_SYSTRACE("frame_last_time", frame_info_rtg(frame_info)->prev_window_time, smp_processor_id());
	FRAME_SYSTRACE("frame_last_task_time", frame_info_rtg_load(frame_info)->prev_window_exec, smp_processor_id());
	FRAME_SYSTRACE("frame_load", frame_info_rtg_load(frame_info)->curr_window_load, smp_processor_id());
}

int set_frame_status(unsigned long status)
{
	u64 timestamp;
	struct related_thread_group *grp = NULL;
	struct frame_info *frame_info = NULL;

	if (!(status & FRAME_SETTIME) || status == FRAME_SETTIME_PARAM) {
		pr_err("[%s] [IPROVISION-FRAME_INFO] invalid timetsamp(status)\n", __func__);
		return -EINVAL;
	}

	grp = frame_rtg();

	if (!grp || list_empty(&grp->tasks))
		return -EIO;

	frame_info = __rtg_frame_info(grp);

	if (!frame_info)
		return -EIO;

	timestamp = ktime_get_ns();

	/* SCHED_FRAME timestamp */
	switch (status) {
	case FRAME_END:
		/* collect frame_info when frame_end timestamp comming */
		set_frame_end(frame_info, timestamp);

		/* update cpufreq force when frame_start */
		update_frame_info_first(frame_info);
		break;
	default:
		pr_err("[%s] [IPROVISION-FRAME_INFO] invalid timestamp(status)\n", __func__);
		return -EINVAL;
	}

	return 0;
}
EXPORT_SYMBOL_GPL(set_frame_status);

struct frame_info global_frame_info;
static int __init init_frame_info(void)
{
	struct related_thread_group *grp = NULL;
	struct frame_info *frame_info = NULL;
	unsigned long flags;

	//frame_info = kmalloc(sizeof(struct frame_info), GFP_KERNEL);
	frame_info = &global_frame_info;
	memset(frame_info, 0, sizeof(struct frame_info));

	frame_info->qos_frame = DEFAULT_FRAME_QOS;
	frame_info->qos_frame_time = NSEC_PER_SEC / frame_info->qos_frame;
	frame_info->vload_margin = DEFAULT_VLOAD_MARGIN;
	frame_info->max_vload_time = frame_info->qos_frame_time / NSEC_PER_MSEC
		+ frame_info->vload_margin;

	grp = frame_rtg();

	raw_spin_lock_irqsave(&grp->lock, flags);
	grp->private_data = frame_info;
	raw_spin_unlock_irqrestore(&grp->lock, flags);

	frame_info->rtg = grp;

	return 0;
}
late_initcall(init_frame_info);
