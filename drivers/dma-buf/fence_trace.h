#undef TRACE_SYSTEM
#define TRACE_INCLUDE_PATH ../../drivers/dma-buf
#define TRACE_SYSTEM fence_trace

#if !defined(_TRACE_SYNC_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_SYNC_H

#include "sync_debug.h"
#include <linux/tracepoint.h>

TRACE_EVENT(sync_name,
		TP_PROTO(struct sync_file *fence, int tgid),

		TP_ARGS(fence, tgid),

		TP_STRUCT__entry(
				 __string(name, fence->name)
				 __field(int, tgid)
		),

		TP_fast_assign(
				__assign_str(name, fence->name);
				__entry->tgid = tgid;
		),

		TP_printk("name=%s,tgid=%d", __get_str(name), __entry->tgid)
);
#endif /* if !defined(_TRACE_SYNC_H) || defined(TRACE_HEADER_MULTI_READ) */

/* This part must be outside protection */
#include <trace/define_trace.h>
