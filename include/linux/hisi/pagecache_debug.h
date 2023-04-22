#ifndef _LINUX_PAGECACHE_DEBUG_H
#define _LINUX_PAGECACHE_DEBUG_H

#include <linux/bitops.h>
#include <linux/dcache.h>
#include <linux/path.h>

#define BIT_READAHEAD_SYSCALL_DUMP		BIT(0)
#define BIT_MADVISE_SYSCALL_DUMP		BIT(1)
#define BIT_FILEMAP_FAULT_DUMP			BIT(2)
#define BIT_MMAP_SYNC_READ_DUMP			BIT(3)
#define BIT_GENERIC_SYNC_READ_DUMP		BIT(4)
#define BIT_PAGECACHE_SYNC_READAHEAD_DUMP	BIT(5)
#define BIT_PAGECACHE_ASYNC_READAHEAD_DUMP	BIT(6)
#define BIT_DO_PAGECACHE_READAHEAD_DUMP		BIT(7)
#define BIT_MM_SHRINK_INACTIVE_DUMP		BIT(8)
#define BIT_MM_SHRINK_ACTIVE_DUMP		BIT(9)
#define BIT_GENERIC_WRITE_DUMP			BIT(10)
#define BIT_FSYNC_SYSCALL_DUMP			BIT(11)
#define BIT_WRITEBACK_DUMP			BIT(12)

#define BIT_PGCACHE_DUMP_FULLPATH_ENABLE	BIT(16)
#define BIT_PGCACHE_DUMP_ERROR_PRINT		BIT(17)
#define BIT_PGCACHE_DUMP_DENTRY_STATUS		BIT(18)

#define DUMP_PATH_LENGTH		200
#define DUMP_STAT_LENGTH		200

#define PGCACHE_DUMP_HEAD_LINE		"[PGCACHE_LOG],"

#ifdef CONFIG_HISI_PAGECACHE_DEBUG
extern unsigned int pagecache_dump;
extern struct fs_pagecache_info *pagecache_info;

struct pagecache_stat {
	atomic64_t hit_total, mmap_hit_total;
	atomic64_t miss_total, mmap_miss_total;
	atomic64_t syncread_pages_count, mmap_syncread_pages_count;
	atomic64_t asyncread_pages_count, mmap_asyncread_pages_count;
	atomic64_t wb_count;
	atomic64_t wb_pages_count;
	atomic64_t dirty_pages_count;
	atomic64_t shrink_pages_count;
};

struct fs_pagecache_info {
	int enable;
	struct pagecache_stat *stat;
};

static inline int is_pagecache_stats_enable(void)
{
	return pagecache_info->enable == 1 ? 1 : 0;
}

#define stat_inc_hit_count()					(atomic64_inc(&(pagecache_info)->stat->hit_total))
#define stat_inc_miss_count()					(atomic64_inc(&(pagecache_info)->stat->miss_total))
#define stat_inc_syncread_pages_count(cnt)		(atomic64_add(cnt, &(pagecache_info)->stat->syncread_pages_count))
#define stat_inc_asyncread_pages_count(cnt)		(atomic64_add(cnt, &(pagecache_info)->stat->asyncread_pages_count))
#define stat_inc_mmap_hit_count()				(atomic64_inc(&(pagecache_info)->stat->mmap_hit_total))
#define stat_inc_mmap_miss_count()				(atomic64_inc(&(pagecache_info)->stat->mmap_miss_total))
#define stat_inc_mmap_syncread_pages_count(cnt)		(atomic64_add(cnt, &(pagecache_info)->stat->mmap_syncread_pages_count))
#define stat_inc_mmap_asyncread_pages_count(cnt)	(atomic64_add(cnt, &(pagecache_info)->stat->mmap_asyncread_pages_count))
#define stat_inc_wb_count()						(atomic64_inc(&(pagecache_info)->stat->wb_count))
#define stat_inc_wb_pages_count(cnt)			(atomic64_add(cnt, &(pagecache_info)->stat->wb_pages_count))
#define stat_inc_dirty_pages_count()			(atomic64_inc(&(pagecache_info)->stat->dirty_pages_count))
#define stat_dec_dirty_pages_count()	\
	do {								\
		if (atomic64_read(&(pagecache_info)->stat->dirty_pages_count) > 0)	\
			atomic64_dec(&(pagecache_info)->stat->dirty_pages_count);	\
	} while (0)
#define stat_inc_shrink_pages_count()			(atomic64_inc(&(pagecache_info)->stat->shrink_pages_count))

void pgcache_log(u32 ctrl, const char *fmt, ...);
void pgcache_log_path(u32 ctrl, const struct path *path, const char *fmt, ...);
void pgcache_log_dentry(u32 ctrl, const struct dentry *dentry, const char *fmt, ...);
#else
static inline int is_pagecache_stats_enable(void)
{
	return 0;
}

#define stat_inc_hit_count()
#define stat_inc_miss_count()
#define stat_inc_syncread_pages_count(cnt)
#define stat_inc_asyncread_pages_count(cnt)
#define stat_inc_mmap_hit_count()
#define stat_inc_mmap_miss_count()
#define stat_inc_mmap_syncread_pages_count(cnt)
#define stat_inc_mmap_asyncread_pages_count(cnt)
#define stat_inc_wb_count()
#define stat_inc_wb_pages_count(cnt)
#define stat_inc_dirty_pages_count()
#define stat_dec_dirty_pages_count()
#define stat_inc_shrink_pages_count()

static inline void pgcache_log(u32 ctrl, const char *fmt, ...) {}
static inline void pgcache_log_path(u32 ctrl, const struct path *path, const char *fmt, ...) {}
static inline void pgcache_log_dentry(u32 ctrl, const struct dentry *dentry, const char *fmt, ...) {}
#endif

#endif /* _LINUX_PAGECACHE_DEBUG_H */

