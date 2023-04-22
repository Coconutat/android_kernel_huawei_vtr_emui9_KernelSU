#undef TRACE_SYSTEM
#define TRACE_SYSTEM futex

#if !defined(_TRACE_FUTEX_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_FUTEX_H
#include <linux/tracepoint.h>

DECLARE_EVENT_CLASS(futex,


	TP_PROTO(int op, int cmd, void *uaddr),

	TP_ARGS(op, cmd, uaddr),

	TP_STRUCT__entry(
		__field(int,	op)
		__field(int,	cmd)
		__field(void *,	uaddr)
	),
	TP_fast_assign(
		__entry->op = op;
		__entry->cmd = cmd;
		__entry->uaddr = uaddr;
	),

	TP_printk("op=%d cmd=%d uaddr=%p",
		__entry->op, __entry->cmd, __entry->uaddr)

);

DEFINE_EVENT(futex, futex_enter,

	TP_PROTO(int op, int cmd, void *uaddr),

	TP_ARGS(op, cmd, uaddr)
);

DEFINE_EVENT(futex, futex_exit,

	TP_PROTO(int op, int cmd, void *uaddr),

	TP_ARGS(op, cmd, uaddr)
);


#endif

/* This part must be outside protection */
#include <trace/define_trace.h>
