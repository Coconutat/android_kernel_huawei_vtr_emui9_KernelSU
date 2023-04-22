#undef TRACE_SYSTEM
#define TRACE_SYSTEM hisi_drg

#if !defined(_TRACE_HISI_DRG_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HISI_DRG_H

#include <linux/tracepoint.h>

TRACE_EVENT(drg_thermal_limit,/* [false alarm]:原生宏定义 */
	TP_PROTO(const char *name, int slave_id, int thermal_state,
		 int target_state, unsigned long target_freq,
		 unsigned long cliped_freq),
	TP_ARGS(name, slave_id, thermal_state, target_state,
		target_freq, cliped_freq),

	TP_STRUCT__entry(
		__field(const char *, name		)
		__field(int, slave_id			)
		__field(int, thermal_state		)
		__field(int, target_state		)
		__field(unsigned long, target_freq	)
		__field(unsigned long, cliped_freq	)
	),

	TP_fast_assign(
		__entry->name		= name;
		__entry->slave_id	= slave_id;
		__entry->thermal_state	= thermal_state;
		__entry->target_state	= target_state;
		__entry->target_freq	= target_freq;
		__entry->cliped_freq	= cliped_freq;
	),

	TP_printk("rule=%s slave=%d thermal_state=%d target_state=%d target_freq=%lu cliped_freq=%lu",
	          __entry->name, __entry->slave_id, __entry->thermal_state,
	          __entry->target_state, __entry->target_freq, __entry->cliped_freq)
);

TRACE_EVENT(drg_update_freq_range,/* [false alarm]:原生宏定义 */
	TP_PROTO(const char *name, int slave_id, int state,
		 unsigned long min_freq, unsigned long max_freq),
	TP_ARGS(name, slave_id, state, min_freq, max_freq),

	TP_STRUCT__entry(
		__field(const char *, name	)
		__field(int, slave_id		)
		__field(int, state		)
		__field(unsigned long, min_freq	)
		__field(unsigned long, max_freq	)
	),

	TP_fast_assign(
		__entry->name		= name;
		__entry->slave_id	= slave_id;
		__entry->state		= state;
		__entry->min_freq	= min_freq;
		__entry->max_freq	= max_freq;
	),

	TP_printk("rule=%s slave=%d state=%d min_freq=%lu max_freq=%lu",
	          __entry->name, __entry->slave_id, __entry->state,
	          __entry->min_freq, __entry->max_freq)
);

TRACE_EVENT(drg_check_limit,/* [false alarm]:原生宏定义 */
	TP_PROTO(const char *name, int cpu, unsigned long target_freq,
		 unsigned long min_freq, unsigned long max_freq),
	TP_ARGS(name, cpu, target_freq, min_freq, max_freq),

	TP_STRUCT__entry(
		__field(const char *, name		)
		__field(int, cpu			)
		__field(unsigned long, target_freq	)
		__field(unsigned long, min_freq		)
		__field(unsigned long, max_freq		)
	),

	TP_fast_assign(
		__entry->name		= name;
		__entry->cpu		= cpu;
		__entry->target_freq	= target_freq;
		__entry->min_freq	= min_freq;
		__entry->max_freq	= max_freq;
	),

	TP_printk("freq_device=%s cpu=%d target_freq=%lu min_freq=%lu max_freq=%lu",
	          __entry->name, __entry->cpu, __entry->target_freq,
	          __entry->min_freq, __entry->max_freq)
);

TRACE_EVENT(drg_cpu_policy_adjust,/* [false alarm]:原生宏定义 */
	TP_PROTO(int cpu, unsigned int margin,
		 unsigned long min_freq, unsigned long max_freq),
	TP_ARGS(cpu, margin, min_freq, max_freq),

	TP_STRUCT__entry(
		__field(int, cpu		)
		__field(unsigned int, margin	)
		__field(unsigned long, min_freq	)
		__field(unsigned long, max_freq	)
	),

	TP_fast_assign(
		__entry->cpu		= cpu;
		__entry->margin		= margin;
		__entry->min_freq	= min_freq;
		__entry->max_freq	= max_freq;
	),

	TP_printk("cpu=%d margin=%u min_freq=%lu max_freq=%lu",
	          __entry->cpu, __entry->margin,
	          __entry->min_freq, __entry->max_freq)
);

#endif /* _TRACE_HISI_DRG_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
