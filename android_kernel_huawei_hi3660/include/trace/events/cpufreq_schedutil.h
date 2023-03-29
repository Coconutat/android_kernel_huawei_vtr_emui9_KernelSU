/*
 *  Copyright (C)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#undef TRACE_SYSTEM
#define TRACE_SYSTEM cpufreq_schedutil

#if !defined(_TRACE_CPUFREQ_SCHEDUTIL_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_CPUFREQ_SCHEDUTIL_H

#include <linux/tracepoint.h>

TRACE_EVENT(cpufreq_schedutil_boost,/* [false alarm]:原生宏定义 */
	    TP_PROTO(const char *s),
	    TP_ARGS(s),
	    TP_STRUCT__entry(
		    __string(s, s)
	    ),
	    TP_fast_assign(
		    __assign_str(s, s);
	    ),
	    TP_printk("%s", __get_str(s))
);

TRACE_EVENT(cpufreq_schedutil_unboost,/* [false alarm]:原生宏定义 */
	    TP_PROTO(const char *s),
	    TP_ARGS(s),
	    TP_STRUCT__entry(
		    __string(s, s)
	    ),
	    TP_fast_assign(
		    __assign_str(s, s);
	    ),
	    TP_printk("%s", __get_str(s))
);

TRACE_EVENT(cpufreq_schedutil_eval_target,/* [false alarm]:原生宏定义 */
	    TP_PROTO(unsigned int cpu,
		     unsigned long util,
		     unsigned long max,
		     unsigned int load,
		     unsigned int curr,
		     unsigned int target),
	    TP_ARGS(cpu, util, max, load, curr, target),
	    TP_STRUCT__entry(
		    __field(unsigned int,	cpu)
		    __field(unsigned long,	util)
		    __field(unsigned long,	max)
		    __field(unsigned int,	load)
		    __field(unsigned int,	curr)
		    __field(unsigned int,	target)
	    ),
	    TP_fast_assign(
		    __entry->cpu = cpu;
		    __entry->util = util;
		    __entry->max = max;
		    __entry->load = load;
		    __entry->curr = curr;
		    __entry->target = target;
	    ),
	    TP_printk("cpu=%u util=%lu max=%lu load=%u cur=%u target=%u",
		      __entry->cpu, __entry->util, __entry->max,
		      __entry->load, __entry->curr, __entry->target)
);

TRACE_EVENT(cpufreq_schedutil_get_util,/* [false alarm]:原生宏定义 */
	    TP_PROTO(unsigned int cpu,
		     unsigned long util,
		     unsigned long max,
		     unsigned long top,
		     unsigned int iowait,
		     unsigned int flag,
		     unsigned int ed),
	    TP_ARGS(cpu, util, max, top, iowait, flag, ed),
	    TP_STRUCT__entry(
		    __field(unsigned int,	cpu)
		    __field(unsigned long,	util)
		    __field(unsigned long,	max)
		    __field(unsigned long,	top)
		    __field(unsigned int,	iowait)
		    __field(unsigned int,	flag)
		    __field(unsigned int,	ed)
	    ),
	    TP_fast_assign(
		    __entry->cpu = cpu;
		    __entry->util = util;
		    __entry->max = max;
		    __entry->top = top;
		    __entry->iowait = iowait;
		    __entry->flag = flag;
		    __entry->ed = ed;
	    ),
	    TP_printk("cpu=%u util=%lu max=%lu top=%lu iowait=%u flag=%u ed=%u",
		      __entry->cpu, __entry->util, __entry->max, __entry->top,
		      __entry->iowait, __entry->flag, __entry->ed)
);

TRACE_EVENT(cpufreq_schedutil_notyet,/* [false alarm]:原生宏定义 */
	    TP_PROTO(unsigned int cpu,
		     const char *reason,
		     unsigned long long delta,
		     unsigned int curr,
		     unsigned int target),
	    TP_ARGS(cpu, reason, delta, curr, target),
	    TP_STRUCT__entry(
		    __field(unsigned int,	cpu)
		    __string(reason, reason)
		    __field(unsigned long long,	delta)
		    __field(unsigned int,	curr)
		    __field(unsigned int,	target)
	    ),
	    TP_fast_assign(
		    __entry->cpu = cpu;
		    __assign_str(reason, reason);
		    __entry->delta = delta;
		    __entry->curr = curr;
		    __entry->target = target;
	    ),
	    TP_printk("cpu=%u reason=%s delta_ns=%llu curr=%u target=%u",
		      __entry->cpu, __get_str(reason),
		      __entry->delta, __entry->curr, __entry->target)
);

TRACE_EVENT(cpufreq_schedutil_already,/* [false alarm]:原生宏定义 */
	    TP_PROTO(unsigned int cpu,
		     unsigned int curr,
		     unsigned int target),
	    TP_ARGS(cpu, curr, target),
	    TP_STRUCT__entry(
		    __field(unsigned int,	cpu)
		    __field(unsigned int,	curr)
		    __field(unsigned int,	target)
	    ),
	    TP_fast_assign(
		    __entry->cpu = cpu;
		    __entry->curr = curr;
		    __entry->target = target;
	    ),
	    TP_printk("cpu=%u curr=%u target=%u",
		      __entry->cpu, __entry->curr, __entry->target)
);

#endif /* _TRACE_CPUFREQ_SCHEDUTIL_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
