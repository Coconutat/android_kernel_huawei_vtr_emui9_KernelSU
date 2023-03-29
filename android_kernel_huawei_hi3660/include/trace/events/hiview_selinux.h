#undef TRACE_SYSTEM
#define TRACE_SYSTEM hiview_selinux

#if !defined(_TRACE_HIVIEW_SELINUX_H) || defined(TRACE_HEADER_MULTI_READ)
#define _TRACE_HIVIEW_SELINUX_H

#include <linux/tracepoint.h>
#include <linux/lsm_audit.h>

TRACE_EVENT(hiview_log_out,
	TP_PROTO(pid_t pid, ino_t ino, u32 requested, u32 denied, u16 tclass, char *pathname, char *comm, u32 droped_worker),

	TP_ARGS(pid, ino, requested, denied, tclass, pathname, comm, droped_worker),

	TP_STRUCT__entry(
		__field(pid_t,	pid)
		__field(ino_t,	ino)
		__field(u64,	time)
		__field(u32,	requested)
		__field(u32,	denied)
		__field(u16,	tclass)
		__field(u32,	droped)
		__array(char,	pathname,	128)
		__array(char,	comm,		64)
	),

	TP_fast_assign(
		__entry->pid		= pid;
		__entry->ino		= ino;
		__entry->time		= local_clock();
		__entry->requested	= requested;
		__entry->denied		= denied;
		__entry->tclass		= tclass;
		__entry->droped		= droped_worker;
		snprintf(__entry->pathname, 128, "%s", pathname);
		snprintf(__entry->comm, 64, "%s", comm);
	),

	TP_printk("time %lld pid %d ino %lu tclass %u requested %u denied %u droped %u pathname %s comm %s",
			__entry->time, __entry->pid, (unsigned long) __entry->ino,
			__entry->tclass, __entry->requested, __entry->denied,
			__entry->droped, __entry->pathname, __entry->comm)
);

#endif
#include <trace/define_trace.h>
