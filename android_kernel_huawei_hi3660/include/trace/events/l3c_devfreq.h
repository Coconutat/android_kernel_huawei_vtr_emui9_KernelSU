#undef TRACE_SYSTEM
#define TRACE_SYSTEM l3c_devfreq

#if !defined(_TRACE_DSU_PCTRL_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_L3C_DEVFREQ_H

#include <linux/tracepoint.h>

TRACE_EVENT(l3c_devfreq_calc_next_freq,/* [false alarm]:原生宏定义 */
	TP_PROTO(unsigned long l3_count, unsigned long ba_count, unsigned long acp_count, unsigned long cycle_count, unsigned long usec_delta, unsigned long cur_freq,
		 unsigned long l3c_bw, unsigned long hit_bw),
	TP_ARGS(l3_count, ba_count, acp_count, cycle_count, usec_delta, cur_freq, l3c_bw, hit_bw),
	TP_STRUCT__entry(
		__field(unsigned long, l3_count)
		__field(unsigned long, ba_count)
		__field(unsigned long, acp_count)
		__field(unsigned long, cycle_count)
		__field(unsigned long, usec_delta)
		__field(unsigned long, cur_freq)
		__field(unsigned long, l3c_bw)
		__field(unsigned long, hit_bw)
	),
	TP_fast_assign(
		__entry->l3_count = l3_count;
		__entry->ba_count = ba_count;
		__entry->acp_count = acp_count;
		__entry->cycle_count = cycle_count;
		__entry->usec_delta = usec_delta;
		__entry->cur_freq = cur_freq;
		__entry->l3c_bw = l3c_bw;
		__entry->hit_bw = hit_bw;
	),

	TP_printk("l3_cnt=%lu ba_cnt=%lu acp_cnt = %lu cy_cnt = %lu usec_delta=%lu cur_freq=%lu l3c_bw = %lu hit_bw = %lu",
		  __entry->l3_count, __entry->ba_count, __entry->acp_count,__entry->cycle_count, __entry->usec_delta, __entry->cur_freq,
		  __entry->l3c_bw, __entry->hit_bw)
);


TRACE_EVENT(l3c_cpufreq_transition,/* [false alarm]:原生宏定义 */
	TP_PROTO(unsigned int cpu, unsigned int old_freq, unsigned int new_freq, int flag),
	TP_ARGS(cpu, old_freq, new_freq, flag),
	TP_STRUCT__entry(
		__field(unsigned int, cpu)
		__field(unsigned int, old_freq)
		__field(unsigned int, new_freq)
		__field(int, flag)
	),
	TP_fast_assign(
		__entry->cpu = cpu;
		__entry->old_freq = old_freq;
		__entry->new_freq = new_freq;
		__entry->flag = flag;
	),

	TP_printk("cpu=%u old=%u new=%u, allow_boost=%u",
		  __entry->cpu, __entry->old_freq, __entry->new_freq, __entry->flag)
);

TRACE_EVENT(l3c_devfreq_target,/* [false alarm]:原生宏定义 */
	TP_PROTO(unsigned long taget_freq),
	TP_ARGS(taget_freq),
	TP_STRUCT__entry(
		__field(unsigned long, taget_freq)
	),
	TP_fast_assign(
		__entry->taget_freq = taget_freq;
	),

	TP_printk("taget_freq=%lu",
		  __entry->taget_freq)
);

TRACE_EVENT(l3c_devfreq_read_event,/* [false alarm]:原生宏定义 */
	TP_PROTO(u64 total),
	TP_ARGS(total),
	TP_STRUCT__entry(
		__field(unsigned long, total)
	),
	TP_fast_assign(
		__entry->total = total;
	),

	TP_printk("total=0x%lx",
		  __entry->total)
);

#endif /* _TRACE_DSU_PCTRL_H */

/* This part must be outside protection */
#include <trace/define_trace.h>
