#ifndef _LINUX_MMONITOR_H
#define _LINUX_MMONITOR_H

#include <linux/types.h>
#include <linux/percpu.h>
#include <linux/mm.h>
#include <linux/atomic.h>
#include <linux/errno.h>

#ifdef CONFIG_HISI_SLOW_PATH_COUNT
#include "hisi/slowpath_count.h"
#endif

#ifdef CONFIG_HW_MEMORY_MONITOR
#define MMONITOR_PROC_FILE	"mmonitor"
#define MMONITOR_MAX_STR_LEN 512

enum mmonitor_event_item {
	ALLOC_FAILED_COUNT,
	FILE_CACHE_READ_COUNT,
	FILE_CACHE_MAP_COUNT,
	FILE_CACHE_MISS_COUNT,
	NR_MMONITOR_EVENT_ITEMS
};

struct mmonitor_event_state {
	unsigned long event[NR_MMONITOR_EVENT_ITEMS];
};

DECLARE_PER_CPU(struct mmonitor_event_state, mmonitor_event_states);

static inline void count_mmonitor_event(enum mmonitor_event_item item)
{
	this_cpu_inc(mmonitor_event_states.event[item]);
}

#endif /* CONFIG_HW_MEMORY_MONITOR */
#endif /* _LINUX_MMONITOR_H */
