#ifndef __FRAME_RTG_H
#define __FRAME_RTG_H

#include <linux/spinlock.h>
#include <linux/types.h>
#include <linux/cpumask.h>
#include <linux/sched.h>
#include "frame.h"


#define FRAME_END		(1 << 0)
#define FRAME_SETTIME		(FRAME_END)
#define FRAME_SETTIME_PARAM	-1

struct frame_info {
	/*
	 * use rtg load tracking in frame_info
	 * rtg->curr_window_load  -=> the workload of current frame
	 * rtg->prev_window_load  -=> the workload of last frame
	 * rtg->curr_window_exec  -=> the thread's runtime of current frame
	 * rtg->prev_window_exec  -=> the thread's runtime of last frame
	 * rtg->prev_window_time  -=> the actual time of the last frame
	 */
	struct related_thread_group *rtg;

	unsigned int qos_frame;
	u64 qos_frame_time;

	/*
	 * frame_vload : the emergency level of current frame.
	 * max_vload_time : the timeline frame_load increase to FRAME_MAX_VLOAD
	 * it's always equal to 2 * qos_frame_time / NSEC_PER_MSEC
	 *
	 * The closer to the deadline, the higher emergency of current
	 * frame, so the frame_vload is only related to frame time,
	 * and grown with time.
	 */
	u64 frame_vload;
	int vload_margin;
	int max_vload_time;

	u64 frame_last_load_util;
	u64 frame_util;

	bool frame_invalid;
};

struct related_thread_group*
lookup_related_thread_group(unsigned int group_id);

static inline struct related_thread_group *
frame_info_rtg(struct frame_info *frame_info)
{
	return frame_info->rtg;
	//return frame_rtg(); TODO
}

static inline struct group_time *
frame_info_rtg_load(struct frame_info *frame_info)
{
	return &frame_info_rtg(frame_info)->time;
}

#endif
