#undef TRACE_SYSTEM
#define TRACE_INCLUDE_PATH ../../drivers/staging/android/trace
#define TRACE_SYSTEM ashmem

#if !defined(_TRACE_ASHMEM_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_ASHMEM_H

#include "../ashmem.h"
#include <linux/tracepoint.h>

TRACE_EVENT(ashmem_set_name,
	TP_PROTO(char *name, int tgid),
	TP_ARGS(name, tgid),
	TP_STRUCT__entry(
		__array(	char,	name,    	ASHMEM_NAME_LEN)
		__field(int, tgid)
	),
	TP_fast_assign(
		strlcpy(__entry->name, name, ASHMEM_NAME_LEN);
		__entry->tgid = tgid;
	),
	TP_printk("name=%s,tgid=%d", __entry->name, __entry->tgid)
);

#endif /* #if !defined(_TRACE_ASHMEM_H) || defined(TRACE_HEADER_MULTI_READ) */

#include <trace/define_trace.h>
