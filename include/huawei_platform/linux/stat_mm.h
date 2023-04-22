#ifndef __STAT_MM_H
#define __STAT_MM_H

#ifdef CONFIG_HW_STAT_MM
#define STAT_MM_STAT(id, type, num, nr, fmt, ...) \
stat_mm_stat((id), (type), (num), (nr), (fmt), ##__VA_ARGS__)
#else
#define STAT_MM_STAT(id, type, num, nr, fmt, ...)
#endif /* CONFIG_HW_STAT_MM */

/* [id:type task-name [parameters]] */
#define STAT_MM_BUDDY_SLUB_BIG(...) \
STAT_MM_STAT(STAT_MM_ID_SLAB, 0, 0, 1, "[%d:%d %s [%ld]]\n", ##__VA_ARGS__)
#define STAT_MM_MLOCK_IF(...) \
STAT_MM_STAT(STAT_MM_ID_MLOCK, 0, 1, 3, \
"[%d:%d %s [0x%lx %ld %ld]]\n", ##__VA_ARGS__)

enum {
	STAT_MM_ID_SLAB,
	STAT_MM_ID_BUDDY,
	STAT_MM_ID_RECLAIM,
	STAT_MM_ID_MLOCK,
	STAT_MM_IDS,
};

extern void stat_mm_stat(int id, int type,
		unsigned int num, unsigned int nr,
		char *fmt, ...);

#endif /* __STAT_MM_H */
