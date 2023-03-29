/*
 * cgroup_workingset.c - control group workingset subsystem
 *
 * Copyright Huawei Corparation, 2017
 * Author: Wanglai.Yao
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#include <linux/module.h>
#include <linux/cgroup.h>
#include <linux/err.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include <linux/spinlock.h>
#include <linux/rbtree.h>
#include <linux/rbtree_augmented.h>
#include <linux/cpu.h>
#include <linux/hugetlb.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/vmalloc.h>
#include <asm/tlbflush.h>
#include <asm/pgtable.h>
#include <linux/mm_inline.h>
#include <linux/kthread.h>
#include <crypto/hash.h>
#include <linux/time.h>
#include <linux/swap.h>
#include <linux/rmap.h>
#include <linux/blkdev.h>
#include <linux/syscalls.h>
#include <linux/version.h>
#include "../drivers/hisi/tzdriver/libhwsecurec/securec.h"

#define CGROUP_WORKINGSET_VERSION	(11)

#define FILE_PAGESEQ_BITS				16
#define PAGE_STAGE_NUM_BITS			3
#define PAGE_MAJOR_BITS				1
#define PAGE_RANGE_HEAD_BITS			1
#define FILE_SEQNUM_BITS				10
#define FILE_OFFSET_BITS	\
	(8 * sizeof(uint32_t) - FILE_SEQNUM_BITS \
	- PAGE_RANGE_HEAD_BITS - PAGE_MAJOR_BITS \
	- PAGE_STAGE_NUM_BITS)
#define PAGE_STAGE_NUM_SHIFT	\
	(FILE_SEQNUM_BITS + FILE_OFFSET_BITS	\
	+ PAGE_RANGE_HEAD_BITS + PAGE_MAJOR_BITS)
#define PAGE_STAGE_NUM_MASK	\
	((1U << PAGE_STAGE_NUM_BITS) - 1)
#define PAGE_MAJOR_SHIFT	\
	(FILE_SEQNUM_BITS + FILE_OFFSET_BITS + PAGE_RANGE_HEAD_BITS)
#define PAGE_MAJOR_MASK	\
	((1U << PAGE_MAJOR_BITS) - 1)
#define PAGE_RANGE_HEAD_SHIFT		\
	(FILE_SEQNUM_BITS + FILE_OFFSET_BITS)
#define PAGE_RANGE_HEAD_MASK	\
	((1U << PAGE_RANGE_HEAD_BITS) - 1)
#define PAGE_RANGE_HEAD_BIT_MASK	\
	(PAGE_RANGE_HEAD_MASK << PAGE_RANGE_HEAD_SHIFT)
#define FILE_IDX_AND_OFFSET_MASK	\
	((1U << PAGE_RANGE_HEAD_SHIFT) - 1)
#define MAX_TOUCHED_FILES_COUNT	\
	((1U << FILE_SEQNUM_BITS) - 1)
#define MAX_TOUCHED_FILE_OFFSET	\
	((1U << FILE_OFFSET_BITS) - 1)
#define MAX_TOUCHED_PAGES_COUNT	\
	((1ULL << FILE_PAGESEQ_BITS) - 1)

#define FILPS_PER_PAGE	\
	(PAGE_SIZE / sizeof(struct page *))
#define FILP_PAGES_COUNT	\
	((MAX_TOUCHED_FILES_COUNT + FILPS_PER_PAGE - 1) / FILPS_PER_PAGE)

#define COLLECTOR_CACHE_SIZE_ORDER			(4)
#define COLLECTOR_CACHE_SIZE	\
	(PAGE_SIZE << COLLECTOR_CACHE_SIZE_ORDER)
#define COLLECTOR_BATCH_COUNT		(64)
#define COLLECTOR_REMAIN_CACHE_LOW_WATER	\
	(COLLECTOR_BATCH_COUNT << 4)

#define PAGECACHEINFO_PER_PAGE	\
	(PAGE_SIZE / sizeof(struct s_pagecache_info))
#define PAGECACHE_INVALID_OFFSET			(~0U)

#define WORKINGSET_RECORD_MAGIC			(0x2b3c5d8e)
#define PATH_MAX_CHAR						256
#define OWNER_MAX_CHAR					256
#define MAX_CRCOMP_STRM_COUNT			2

#define CARE_BLKIO_MIN_THRESHOLD					20
#define BLKIO_PERCENTAGE_THRESHOLD_FOR_UPDATE	2
#define CACHE_MISSED_PERCENTAGE_THRESHOLD_FOR_BLKIO	80

#define TOTAL_RAM_PAGES_1G	(1 << 18)
#define MAX_RECORD_COUNT_ON_1G	5
#define MAX_RECORD_COUNT_ON_2G	10
#define MAX_RECORD_COUNT_ON_3G	20
#define MAX_RECORD_COUNT_ON_4G	40

#define CREATE_HANDLE(file_num, file_offset)	\
({	\
	unsigned int handle;	\
	\
	handle = file_num;	\
	handle <<= FILE_OFFSET_BITS;	\
	handle |= file_offset & MAX_TOUCHED_FILE_OFFSET;	\
	handle;	\
})

/* the commands what sends to workingset*/
enum ws_monitor_states {
	E_MONITOR_STATE_OUTOFWORK,
	E_MONITOR_STATE_INWORKING,
	E_MONITOR_STATE_PAUSED,
	E_MONITOR_STATE_STOP,
	E_MONITOR_STATE_ABORT,
	E_MONITOR_STATE_PREREAD,
	E_MONITOR_STATE_BACKUP,
	E_MONITOR_STATE_MAX
};


/* the states of workingset*/
enum ws_cgroup_states {
	E_CGROUP_STATE_OFFLINE = 0,
	E_CGROUP_STATE_ONLINE = (1 << 7),
	E_CGROUP_STATE_MONITOR_BITMASK	= (E_CGROUP_STATE_ONLINE - 1),
	E_CGROUP_STATE_MONITORING = (1 << 6),
	E_CGROUP_STATE_MONITOR_OUTOFWORK =
		(E_CGROUP_STATE_ONLINE | E_MONITOR_STATE_OUTOFWORK),
	E_CGROUP_STATE_MONITOR_INWORKING =
		(E_CGROUP_STATE_ONLINE |
		E_CGROUP_STATE_MONITORING | E_MONITOR_STATE_INWORKING),
	E_CGROUP_STATE_MONITOR_PAUSED =
		(E_CGROUP_STATE_ONLINE | E_MONITOR_STATE_PAUSED),
	E_CGROUP_STATE_MONITOR_STOP =
		(E_CGROUP_STATE_ONLINE | E_MONITOR_STATE_STOP),
	E_CGROUP_STATE_MONITOR_ABORT =
		(E_CGROUP_STATE_ONLINE | E_MONITOR_STATE_ABORT),
	E_CGROUP_STATE_MONITOR_PREREAD =
		(E_CGROUP_STATE_ONLINE | E_MONITOR_STATE_PREREAD),
	E_CGROUP_STATE_MONITOR_BACKUP =
		(E_CGROUP_STATE_ONLINE | E_MONITOR_STATE_BACKUP),
	E_CGROUP_STATE_MAX,
};

/* the events what collector waiting for*/
enum collector_wait_flags {
	F_NONE,
	F_COLLECT_PENDING,
	F_RECORD_PENDING,
};

/* the states or flags of records*/
enum ws_record_state {
	E_RECORD_STATE_UNUSED = 0x00,
	E_RECORD_STATE_USED = 0x01,
	/* the flag indicate writeback the record*/
	E_RECORD_STATE_DIRTY = 0x02,
	E_RECORD_STATE_COLLECTING = 0x04,
	E_RECORD_STATE_PREREADING = 0x08,
	E_RECORD_STATE_PAUSE = 0x10,
	/* the flag indicate the playload buffer is contiguous*/
	E_RECORD_STATE_DATA_FROM_BACKUP = 0x20,
	/* the flag indicate the blkio count that first time on prereading*/
	E_RECORD_STATE_UPDATE_BASE_BLKIO = 0x40,
	/* the flag indicate write only record header to disk*/
	E_RECORD_STATE_UPDATE_HEADER_ONLY = 0x80,
};

struct s_path_node {
	/*the hash code of this path*/
	unsigned hashcode;
	unsigned pathlen;
	uid_t owner_uid;
	char *path;
};

struct s_range {
	unsigned start;
	unsigned end;
};

struct s_filp_list {
	struct file *filp;
	struct s_filp_list *next;
};

struct s_file_info {
	/*list to the workingset file list*/
	struct list_head list;
	/*the list of pointer of struct file*/
	struct s_filp_list *filp_list;
	/*the path info of file*/
	struct s_path_node path_node;
	/*
	 * the count of page sequences belong to this owner,
	 * the range including single page occupy one pageseq,
	 * the range including multi pages occupy two pageseqs.
	 */
	unsigned int pageseq_count;
	/*the root of page cache tree*/
	struct rb_root rbroot;
};

struct s_pagecache_info {
	struct rb_node rbnode;
	/*the offset range of file*/
	struct s_range offset_range;
};

struct s_ws_owner {
	unsigned uid;
	/*the pid of leader thread*/
	int pid;
	char *name;
	/*the path of record file*/
	char *record_path;
};

struct s_ws_data {
	unsigned file_cnt;
	/*
	 * the count of page sequences belong to this owner,
	 * the range including single page occupy one pageseq,
	 * the range including multi pages occupy two pageseqs.
	 */
	unsigned pageseq_cnt;
	/*sum of file pages this owner accessed*/
	unsigned page_sum;
	/*the size of pages array*/
	unsigned array_page_cnt;
	/*the pages array that caching the path informations
	 *and file offset range informations
	 */
	struct page **page_array;
	/*the file array*/
	struct s_path_node *file_array;
	/*the file cache array*/
	unsigned *cacheseq;
};

struct s_ws_record {
	/*list to the global workingset record list*/
	struct list_head list;
	struct s_ws_owner owner;
	struct s_ws_data data;
	struct mutex mutex;
	/*the state of a record*/
	unsigned state;
#ifdef CONFIG_TASK_DELAY_ACCT
	/*the blkio count of main thread when first time on prereading*/
	unsigned short leader_blkio_cnt;
	/*tell us if or not need collect again*/
	unsigned short need_update;
#endif
	/*pages for caching struct files that be opened on prereading */
	struct page *filp_pages[FILP_PAGES_COUNT];
};

struct s_ws_backup_record_header {
	unsigned magic;
	unsigned header_crc;
	/*version of the record file,
	 *it must be equal version of this linux module.
	 */
	unsigned record_version;
	/*the count of the file*/
	unsigned file_cnt;
	/*
	 * the count of page sequences belong to this owner,
	 * the range including single page occupy one pageseq,
	 * the range including multi pages occupy two pageseqs.
	 */
	unsigned pageseq_cnt;
	/*sum of accessed file pages*/
	unsigned page_sum;
	/*the size of the playload data*/
	unsigned playload_length;
	/*the checksum of the playload data*/
	unsigned playload_checksum;
#ifdef CONFIG_TASK_DELAY_ACCT
	/*the blkio count of main thread when first time on prereading*/
	unsigned short leader_blkio_cnt;
	/*tell us if or not need collect again*/
	unsigned short need_update;
#else
	unsigned padding1;
	unsigned padding2;
#endif
};

struct s_workingset {
	struct cgroup_subsys_state css;
	struct mutex mutex;
	/*the owner which workingset is working for*/
	struct s_ws_owner owner;
	unsigned long repeated_count;
	unsigned int page_sum;
	unsigned int stage_num;
	/*the state of workingset*/
	unsigned int state;
#ifdef CONFIG_TASK_DELAY_ACCT
	/*the blkio count of main thread*/
	unsigned short leader_blkio_cnt;
	__u64 leader_blkio_base;
#endif
	unsigned int file_count;
	unsigned int pageseq_count;
	/*the alloc index of pagecache array*/
	unsigned int alloc_index;
	bool shrinker_enabled;
	struct shrinker shrinker;
	struct list_head file_list;
	/*pages for caching page offset range information*/
	struct s_pagecache_info *cache_pages[0];
};

struct s_cachepage_info {
	struct file *filp;
	unsigned	start_offset;
	/*the count of contiguous pagecache*/
	unsigned count:(16 - PAGE_STAGE_NUM_BITS);
	unsigned stage:PAGE_STAGE_NUM_BITS;
	unsigned pid:16;
};

struct s_ws_collector {
	spinlock_t lock;
	struct task_struct *collector_thread;
	wait_queue_head_t collect_wait;
	enum collector_wait_flags wait_flag;
	/*the workingset that collector working for*/
	struct s_workingset *monitor;
	unsigned long discard_count;
	/*the read position of circle buffer*/
	unsigned read_pos;
	/*the write position of circle buffer*/
	unsigned write_pos;
	/*the address of circle buffer*/
	void *circle_buffer;
};

struct s_readpages_control {
	struct file *filp;
	struct address_space *mapping;
	/*the file offset that will be read */
	pgoff_t offset;
	/*the count that file pages was readed*/
	unsigned long nr_to_read;
	/*the count that lru pages was moved*/
	unsigned nr_adjusted;
};

static spinlock_t g_record_list_lock;
static LIST_HEAD(g_record_list);
static bool g_module_initialized;
static unsigned g_record_cnt;
static unsigned g_max_records_count = MAX_RECORD_COUNT_ON_1G;
/*use to interrupt prereading process.*/
static atomic_t g_preread_abort = ATOMIC_INIT(0);
static struct s_ws_collector *g_collector;
static struct crypto_shash *g_tfm;
static const char *moniter_states[E_MONITOR_STATE_MAX] = {
	"OUTOFWORK",
	"INWORKING",
	"PAUSED",
	"STOP",
	"ABORT",
	"PREREAD",
	"BACKUP"
};

/*dynamic debug informatioins controller*/
#ifdef CONFIG_HW_CGROUP_WS_DEBUG
static bool ws_debug_enable = true;
#else
static bool ws_debug_enable;
#endif
module_param_named(debug_enable, ws_debug_enable, bool, S_IRUSR | S_IWUSR);
#define ws_dbg(x...) \
	do { \
		if (ws_debug_enable) \
			pr_info(x); \
	} while (0)

static void workingset_do_preread_work_rcrdlocked(struct s_ws_record *record);
static void workingset_preread_post_work_rcrdlocked(
	struct s_ws_record *record, struct file **filpp);

static void workingset_insert_record_to_list_head(struct s_ws_record *record)
{
	spin_lock(&g_record_list_lock);
	g_record_cnt++;
	list_add(&record->list, &g_record_list);
	spin_unlock(&g_record_list_lock);
}

static void workingset_insert_record_to_list_tail(struct s_ws_record *record)
{
	spin_lock(&g_record_list_lock);
	g_record_cnt++;
	list_add_tail(&record->list, &g_record_list);
	spin_unlock(&g_record_list_lock);
}
/**
 * workingset_pagecache_info_cache_alloc_wslocked -
 * alloc a pagecache space from the pagecache array of workingset.
 * @workingset: the owner of pagecache array
 *
 * Return the pointer of a free pagecache space or null.
 */
static struct s_pagecache_info *workingset_pagecache_info_cache_alloc_wslocked(
		struct s_workingset *ws)
{
	unsigned page_idx = ws->alloc_index / PAGECACHEINFO_PER_PAGE;
	unsigned off_in_page = ws->alloc_index % PAGECACHEINFO_PER_PAGE;

	/* the size of struct s_workingset space is PAGE_SIZE,
	 * including essentials and pages array.
	 */
	if (offsetof(struct s_workingset, cache_pages) +
		(page_idx + 1) * sizeof(struct s_pagecache_info **) >
		PAGE_SIZE)
		goto out;

	if (!ws->cache_pages[page_idx]) {
		ws->cache_pages[page_idx] =
			(struct s_pagecache_info *)get_zeroed_page(GFP_NOFS);
		if (!ws->cache_pages[page_idx])
			goto out;
	}

	return ws->cache_pages[page_idx] + off_in_page;

out:
	return NULL;
}

/*********************************
* shrinker of workingset code
* permmit shrinking the pagecache array memory of workingset only if
* the workingset is outofwork or aborted.
**********************************/
static unsigned long workingset_shrinker_scan(struct shrinker *shrinker,
		struct shrink_control *sc)
{
	int idx, stop_idx, max_count;
	unsigned long pages_freed;
	size_t offset = offsetof(struct s_workingset, cache_pages);
	struct s_workingset *ws =
		container_of(shrinker, struct s_workingset, shrinker);

	max_count = (PAGE_SIZE - offset) / sizeof(struct s_pagecache_info **);
	if (!mutex_trylock(&ws->mutex))
		return SHRINK_STOP;

	/* reclaim the page array when the workingset is out of work*/
	if ((ws->state == E_CGROUP_STATE_MONITOR_OUTOFWORK)
		|| (ws->state == E_CGROUP_STATE_MONITOR_ABORT)) {
		if (!ws->alloc_index)
			stop_idx = -1;
		else
			stop_idx = ws->alloc_index /
					PAGECACHEINFO_PER_PAGE;

		pages_freed = 0;
		for (idx = max_count - 1; idx > stop_idx; idx--) {
			if (ws->cache_pages[idx]) {
				free_page((unsigned long)ws->cache_pages[idx]);
				ws->cache_pages[idx] = NULL;
				pages_freed++;
			}
		}
	} else {
		pages_freed = 0;
	}

	mutex_unlock(&ws->mutex);  /*lint !e455*/
	ws_dbg("%s: reclaimed %lu pages\n", __func__, pages_freed);

	return pages_freed ? pages_freed : SHRINK_STOP;
}

static unsigned long workingset_shrinker_count(struct shrinker *shrinker,
		struct shrink_control *sc)
{
	unsigned idx, max_count;
	unsigned long pages_to_free = 0;
	struct s_workingset *ws =
		container_of(shrinker, struct s_workingset, shrinker);

	max_count = (PAGE_SIZE - offsetof(struct s_workingset, cache_pages)) /
				sizeof(struct s_pagecache_info **);
	if (!mutex_trylock(&ws->mutex))
		return 0;

	/* reclaim the page array when the workingset is out of work*/
	if ((ws->state == E_CGROUP_STATE_MONITOR_OUTOFWORK)
		|| (ws->state == E_CGROUP_STATE_MONITOR_ABORT)) {
		for (idx = 0; idx < max_count; idx++) {
			if (ws->cache_pages[idx])
				pages_to_free++;
			else
				break;
		}
		if (pages_to_free >
			1 + ws->alloc_index / PAGECACHEINFO_PER_PAGE)
			pages_to_free -=
				1 + ws->alloc_index / PAGECACHEINFO_PER_PAGE;
		else
			pages_to_free = 0;
	} else {
		pages_to_free = 0;
	}
	mutex_unlock(&ws->mutex);  /*lint !e455*/

	return pages_to_free;
}

static void workingset_unregister_shrinker(struct s_workingset *ws)
{
	if (ws->shrinker_enabled) {
		unregister_shrinker(&ws->shrinker);
		ws->shrinker_enabled = false;
	}
}

static int workingset_register_shrinker(struct s_workingset *ws)
{
	ws->shrinker.scan_objects = workingset_shrinker_scan;
	ws->shrinker.count_objects = workingset_shrinker_count;
	ws->shrinker.batch = 0;
	ws->shrinker.seeks = DEFAULT_SEEKS;

	return register_shrinker(&ws->shrinker);
}

/*********************************
* crc32 code
**********************************/
static unsigned workingset_crc32c(unsigned crc,
	const void *address, unsigned int length)
{
	SHASH_DESC_ON_STACK(shash, g_tfm);
	unsigned *ctx = (u32 *)shash_desc_ctx(shash);
	unsigned retval;
	int err;

	shash->tfm = g_tfm;
	shash->flags = 0;
	*ctx = crc;

	err = crypto_shash_update(shash, address, length);
	if (err) {
		pr_err("%s, %d, err=%d\n", __func__, __LINE__, err);
		retval = crc;
	} else {
		retval = *ctx;
	}
	barrier_data(ctx);

	return retval;
}

/*********************************
* rbtree code
**********************************/
static struct rb_node *rb_deepest_left_node(const struct rb_node *node)
{
	for (;;) {
		if (node->rb_left)
			node = node->rb_left;
		else
			return (struct rb_node *)node;
	}
}

static struct rb_node *rb_deepest_right_node(const struct rb_node *node)
{
	for (;;) {
		if (node->rb_right)
			node = node->rb_right;
		else
			return (struct rb_node *)node;
	}
}

static struct rb_node *rb_latest_left_ancestor(const struct rb_node *node)
{
	const struct rb_node *parent;
	const struct rb_node *temp_node = node;

	while (temp_node) {
		parent = rb_parent(temp_node);
		if (parent && temp_node == parent->rb_left)
			temp_node = parent;
		else
			return (struct rb_node *)parent;
	}

	return NULL;
}

static struct rb_node *rb_latest_right_ancestor(const struct rb_node *node)
{
	const struct rb_node *parent;
	const struct rb_node *temp_node = node;

	while (temp_node) {
		parent = rb_parent(temp_node);
		if (parent && temp_node == parent->rb_right)
			temp_node = parent;
		else
			return (struct rb_node *)parent;
	}

	return NULL;
}

struct rb_node *rb_prev_middleorder(const struct rb_node *node)
{
	if (!node)
		return NULL;

	if (node->rb_left)
		return rb_deepest_right_node(node->rb_left);
	else
		return rb_latest_left_ancestor(node);
}

struct rb_node *rb_next_middleorder(const struct rb_node *node)
{
	if (!node)
		return NULL;

	if (node->rb_right)
		return rb_deepest_left_node(node->rb_right);
	else
		return rb_latest_right_ancestor(node);
}

struct rb_node *rb_first_middleorder(const struct rb_root *root)
{
	if (!root->rb_node)
		return NULL;

	return rb_deepest_left_node(root->rb_node);
}

#define rbtree_middleorder_for_each_entry_safe(pos, n, root, field) \
	for (pos = rb_entry_safe(rb_first_middleorder(root),	\
			typeof(*pos), field); \
	     pos && ({ n = rb_entry_safe(rb_next_middleorder(&pos->field), \
			typeof(*pos), field); 1; }); \
	     pos = n)

#define rbtree_middleorder_for_each_entry_safe_continue(pos, n, root, field) \
	for (; pos && ({ n = rb_entry_safe(rb_next_middleorder(&pos->field), \
			typeof(*pos), field); 1; }); \
	     pos = n)

static void workingset_range_rb_erase(struct rb_root *root,
	struct s_pagecache_info *entry)
{
	if (!RB_EMPTY_NODE(&entry->rbnode)) {
		rb_erase(&entry->rbnode, root);
		RB_CLEAR_NODE(&entry->rbnode);
	}
}

static inline void workingset_range_rb_change_to_front(
	struct s_pagecache_info **oldEntry, struct s_pagecache_info **newEntry)
{
	struct s_pagecache_info *temp;

	if (*oldEntry < *newEntry) {
		temp = *oldEntry;
		*oldEntry = *newEntry;
		*newEntry = temp;
	}
}

#define workingset_merge_ranges(root, myentry, merged_entry, probe, delta) \
{	\
	if (probe) {	\
		workingset_range_rb_change_to_front(&myentry, &merged_entry); \
		workingset_range_rb_erase(root, myentry);	\
		delta -= (myentry->offset_range.start &	\
				PAGE_RANGE_HEAD_BIT_MASK) ? 2 : 1; \
		myentry->offset_range.start = PAGECACHE_INVALID_OFFSET;	\
		myentry->offset_range.end = PAGECACHE_INVALID_OFFSET;	\
	}	\
}

/*
 * set 1 to the range header bit if the range has multipages,
 * or clear the range header bit.
 */
#define workingset_dealwith_range_header_bit(entry, start, end, \
	major, stage, delta) \
{	\
	struct s_range *myrange = &entry->offset_range; /*lint !e613*/	\
	\
	if (end - start > 1) {	\
		if (!(myrange->start & PAGE_RANGE_HEAD_BIT_MASK))	\
			delta += 1;	\
		myrange->start = start | PAGE_RANGE_HEAD_BIT_MASK;	\
		myrange->end = end | PAGE_RANGE_HEAD_BIT_MASK;	\
	} else {	\
		myrange->start = start & ~PAGE_RANGE_HEAD_BIT_MASK;	\
		myrange->end = end & ~PAGE_RANGE_HEAD_BIT_MASK;	\
	}	\
	\
	if (major) {	\
		myrange->start |=	\
			(major & PAGE_MAJOR_MASK) << PAGE_MAJOR_SHIFT;	\
		myrange->end |=	\
			(major & PAGE_MAJOR_MASK) << PAGE_MAJOR_SHIFT;	\
	}	\
	\
	if (stage) {	\
		myrange->start |=	\
		(stage & PAGE_STAGE_NUM_MASK) << PAGE_STAGE_NUM_SHIFT; \
		myrange->end |=	\
		(stage & PAGE_STAGE_NUM_MASK) << PAGE_STAGE_NUM_SHIFT; \
	}	\
}

#define workingset_rbtree_probe_left(root, merged_entry, myentry, probe_left, \
	look_otherside, start, cur_start, cur_end, overrange, page_delta) \
{	\
	if (!merged_entry || probe_left) {	\
		if (merged_entry)	\
			look_otherside = true;	\
\
		if (start < cur_start) {	\
/*inserted entry or merged entry including current offset range.*/ \
			overrange.start = cur_start;	\
			parent = rb_prev_middleorder(&myentry->rbnode); \
			if (merged_entry) \
				workingset_merge_ranges(root, myentry,	\
				merged_entry, probe_left, page_delta);	\
/*probe left tree if there are smaller offset ranges.*/	\
			if (!parent) \
				probe_left = false; \
			else \
				probe_left = true; \
		} else { \
/*inserted entry or merged entry overlapped with current offset range.*/ \
			if (merged_entry) \
				workingset_merge_ranges(root, myentry,	\
				merged_entry, probe_left, page_delta);	\
			overrange.start = start;	\
			start = cur_start;	\
			probe_left = false;	\
		}	\
		if (merged_entry)	\
			overrange.end = cur_end;	\
	}	\
}

#define workingset_rbtree_probe_right(root, merged_entry, myentry, \
	probe_left, probe_right, look_otherside, end, cur_start, cur_end, \
	overrange, page_delta) \
{ \
	if (!merged_entry || (probe_right && !probe_left)) { \
		if (look_otherside && merged_entry) { \
/*there aren't any small offset range, so we look aside bigger offset range*/ \
			look_otherside = false; \
			parent = rb_next_middleorder( \
				&merged_entry->rbnode); \
			if (!parent) \
				probe_right = false; \
		} else if (end > cur_end) { \
/*inserted entry or merged entry including current offset range.*/ \
			if (!probe_left) \
				parent = rb_next_middleorder( \
				&myentry->rbnode); \
			if (merged_entry) \
				workingset_merge_ranges(root, myentry, \
				merged_entry, probe_right, page_delta); \
			if (merged_entry) \
				overrange.start = cur_start; \
			overrange.end = cur_end; \
/*stop probing right tree if there are not any bigger offset ranges.*/ \
			if (!parent) \
				probe_right = false; \
			else \
				probe_right = true; \
		} else { \
/*inserted entry or merged entry overlapped with current offset range.*/ \
			if (merged_entry) \
				workingset_merge_ranges(root, myentry, \
				merged_entry, probe_right, page_delta); \
			if (merged_entry) \
				overrange.start = cur_start; \
			overrange.end = end; \
\
			end = cur_end; \
			probe_right = false; \
		} \
	} \
}

#define workingset_handle_range_insert_result(root, entry, merged_entry, \
	major_touched, stage, parent, repeat_pages, \
	page_count_delta, repeat_cnt, page_delta) \
{ \
	if (!merged_entry) { \
/*the inserted range has not overlapped or adjoined with any ranges.*/ \
		if (major_touched) { \
			entry->offset_range.start |= \
			(major_touched & PAGE_MAJOR_MASK) << PAGE_MAJOR_SHIFT; \
			entry->offset_range.end |= \
			(major_touched & PAGE_MAJOR_MASK) << PAGE_MAJOR_SHIFT; \
		} \
		if (stage) { \
			entry->offset_range.start |= \
			(stage & PAGE_STAGE_NUM_MASK) << PAGE_STAGE_NUM_SHIFT;\
			entry->offset_range.end |= \
			(stage & PAGE_STAGE_NUM_MASK) << PAGE_STAGE_NUM_SHIFT;\
		} \
		rb_link_node(&entry->rbnode, parent, link); \
		rb_insert_color(&entry->rbnode, root); \
	} else { \
		*repeat_pages = repeat_cnt; \
		*page_count_delta = page_delta; \
	} \
}

/**
 * workingset_range_rb_insert -
 * insert a page offset range into pageoffset range tree of a file.
 * @root: the root of page range tree
 * @entry: a entry of page offset range.
 * @major_touched: if or not the page is touched by main thread.
 * @repeat_pages: output the count of overlaped page offset.
 * @page_count_delta: output the count of added page offset range.
 *
 * In the case that a entry with the same offset is found,
 * the function returns overlapped range or adjoined range.
 */
static struct s_pagecache_info *workingset_range_rb_insert(
	struct rb_root *root, struct s_pagecache_info *entry,
	int major_touched, unsigned cur_stage,
	unsigned *repeat_pages, int *page_count_delta)
{
	struct rb_node **link = &root->rb_node, *parent = NULL;
	struct s_pagecache_info *myentry, *merged_entry = NULL;
	unsigned cur_start, cur_end;
	unsigned start = entry->offset_range.start & FILE_IDX_AND_OFFSET_MASK;
	unsigned end = entry->offset_range.end & FILE_IDX_AND_OFFSET_MASK;
	bool probe_left = false, probe_right = false, look_otherside = false;
	/*the overlapped range*/
	struct s_range overrange = {0, 0};
	struct s_range *range;
	unsigned repeat_cnt = 0;
	int page_delta = 0;
	unsigned stage_num;

	/*range[start, end)*/
	while (*link) {
		parent = *link;
		myentry = rb_entry(parent, struct s_pagecache_info, rbnode);
		range = &myentry->offset_range;
		cur_start = range->start & FILE_IDX_AND_OFFSET_MASK;
		cur_end = range->end & FILE_IDX_AND_OFFSET_MASK;

		if (cur_start > end)
			link = &(*link)->rb_left;
		else if (cur_end < start)
			link = &(*link)->rb_right;
		else {	/*in the case, two ranges is overlapped or adjoined*/
			/*indicate major touched page offset range
			 * even if a page was touched by main thread
			 */
			major_touched |= (range->start >> PAGE_MAJOR_SHIFT) &
							PAGE_MAJOR_MASK;
			stage_num = (range->start >> PAGE_STAGE_NUM_SHIFT) &
							PAGE_STAGE_NUM_MASK;
			if (stage_num < cur_stage)
				cur_stage = stage_num;

			/**
			 * We probe left child tree first, and merge overlapped
			 * range or adjoined range, then probe right child
			 * tree.
			 * exchange the position between inserted range with
			 * adjoined range in order to preread these file pages
			 * as early as possible. and dicard the space of erased
			 *pagecache range in pagecache array.
			 */
			workingset_rbtree_probe_left(root, merged_entry,
				myentry, probe_left, look_otherside, start,
				cur_start, cur_end, overrange, page_delta);

			/*
			 * in the case, merge range first time
			 * or there are not any small offset range.
			 */
			workingset_rbtree_probe_right(root, merged_entry,
				myentry, probe_left, probe_right,
				look_otherside, end, cur_start,
				cur_end, overrange, page_delta);

			if (!merged_entry)
				merged_entry = myentry;

			if (overrange.end > overrange.start)
				repeat_cnt += overrange.end - overrange.start;

			workingset_dealwith_range_header_bit(merged_entry,
						start, end, major_touched,
						cur_stage, page_delta);
			if (!probe_right && !probe_left)
				break;
			link = &parent;
			continue;
		}

		if (merged_entry) {
			/*
			 * there are not any small offset range,
			 * so we look aside bigger offset range.
			 */
			if (probe_left && probe_right) {
				probe_left = false;
				parent = rb_next_middleorder(
					&merged_entry->rbnode);
				if (parent) {
					link = &parent;
					continue;
				}
			}
			break;
		}
	}

	workingset_handle_range_insert_result(root, entry, merged_entry,
		major_touched, cur_stage, parent, repeat_pages,
		page_count_delta, repeat_cnt, page_delta);

	return merged_entry;
}

/*********************************
* the operation of fs code
**********************************/
static void workingset_recycle_record_rcrdlocked(struct s_ws_record *record)
{
	unsigned idx;

	/*
	 * free file_array only because use one vmalloc
	 * for file_array and cacheseq.
	 */
	if (record->data.file_array) {
		for (idx = 0; idx < record->data.file_cnt; idx++) {
			if (record->data.file_array[idx].path
				&& !(record->state &
				E_RECORD_STATE_DATA_FROM_BACKUP)) {
				kfree(record->data.file_array[idx].path);
				record->data.file_array[idx].path = NULL;
			}
		}
	}
	record->data.file_cnt = 0;
	record->data.pageseq_cnt = 0;
	record->data.page_sum = 0;
#ifdef CONFIG_TASK_DELAY_ACCT
	record->leader_blkio_cnt = 0;
	record->need_update = 0;
#endif
	record->state &= E_RECORD_STATE_DATA_FROM_BACKUP;
}

/**
 * workingset_get_record_header -
 * get the header data of record from backup file.
 * @filp: the struct file pointer of opened file.
 * @offset: the start position of data compute checksum.
 * @header: the pointer of header of record.
 */
static int workingset_get_record_header(struct file *filp,
	size_t offset, struct s_ws_backup_record_header *header)
{
	int ret = 0;
	int length;

	length = kernel_read(filp, 0, (char *)header,
		sizeof(struct s_ws_backup_record_header));
	if (sizeof(struct s_ws_backup_record_header) != length) {
		pr_err("%s line %d: kernel_read failed, len = %d\n",
			__func__, __LINE__, length);
		ret = -EIO;
		goto out;
	}

	if (header->magic != WORKINGSET_RECORD_MAGIC
		|| header->record_version != CGROUP_WORKINGSET_VERSION
		|| header->header_crc != workingset_crc32c(0,
		&header->record_version,
		sizeof(struct s_ws_backup_record_header) - offset)) {
		pr_err("%s line %d: magic=%u, headercrc=%u\n",
			__func__, __LINE__, header->magic,
			header->header_crc);
		ret = -EIO;
	}

out:
	return ret;
}

/**
 * workingset_record_writeback_playload_rcrdlocked -
 * writeback the playlod data of record.
 * @filp: the struct file pointer of opened file.
 * @record: the record need to writeback.
 * @pplayload_length: the pointer of length of playload data.
 * @pchecksum: the pointer of checksum of playload data.
 *
 * you need hold the lock of record before call this function.
 */
static int workingset_record_writeback_playload_rcrdlocked(struct file *filp,
	struct s_ws_record *record, unsigned *pplayload_length,
	unsigned *pchecksum)
{
	int ret = 0;
	unsigned checksum = 0, crc_val;
	unsigned length = 0;
	unsigned pathnode_size;
	unsigned idx;
	loff_t pos = sizeof(struct s_ws_backup_record_header);

	pathnode_size = sizeof(struct s_path_node) *
				record->data.file_cnt;
	crc_val = workingset_crc32c(checksum, record->data.file_array,
				pathnode_size);
	if (crc_val == checksum) {
		pr_err("%s line %d: checksum=%u crc_val=%u\n",
			__func__, __LINE__, checksum, crc_val);
		ret = -EINVAL;
		goto out;
	}

	checksum = crc_val;
	if (pathnode_size != kernel_write(filp,
		(char *)record->data.file_array, pathnode_size, pos)) {
		pr_err("%s line %d: kernel_write failed\n",
			__func__, __LINE__);
		ret = -EIO;
		goto out;
	}

	pos += pathnode_size;
	for (idx = 0; idx < record->data.file_cnt; idx++) {
		if (!record->data.file_array[idx].path)
			continue;

		length = record->data.file_array[idx].pathlen ?
			record->data.file_array[idx].pathlen + 1 :
			strlen(record->data.file_array[idx].path) + 1;
		crc_val = workingset_crc32c(checksum,
			record->data.file_array[idx].path, length);
		if (crc_val == checksum) {
			pr_err("%s line %d: checksum=%u crc_val=%u\n",
				__func__, __LINE__, checksum, crc_val);
			ret = -EINVAL;
			goto out;
		}

		checksum = crc_val;
		if (length != kernel_write(filp,
			record->data.file_array[idx].path,
			length, pos)) {
			pr_err("%s line %d: kernel_write failed\n",
				__func__, __LINE__);
			ret = -EIO;
			goto out;
		}
		pos += length;
	}

	length = sizeof(unsigned) * record->data.pageseq_cnt;
	crc_val = workingset_crc32c(checksum,
		record->data.cacheseq, length);
	if (crc_val == checksum) {
		pr_err("%s line %d: checksum=%u crc_val=%u\n",
			__func__, __LINE__, checksum, crc_val);
		ret = -EINVAL;
		goto out;
	}

	checksum = crc_val;
	if (length != kernel_write(filp,
		(char *)record->data.cacheseq, length, pos)) {
		pr_err("%s line %d: kernel_write failed\n",
			__func__, __LINE__);
		ret = -EIO;
		goto out;
	}

	pos += length;
	/*truncate invalid data if it is existed.*/
	if (vfs_truncate(&filp->f_path, pos))
		pr_warn("%s %s vfs_truncate failed!",
		__func__, record->owner.record_path);

	*pplayload_length = pos - sizeof(struct s_ws_backup_record_header);
	*pchecksum = checksum;

out:
	return ret;
}

/**
 * workingset_backup_record_rcrdlocked - writeback the data of record
 * @record: the record need to writeback.
 *
 * you need hold the lock of record before call this function.
 */
static bool workingset_backup_record_rcrdlocked(struct s_ws_record *record)
{
	bool ret = false;
	struct file *filp;
	struct s_ws_backup_record_header header = {0,};
	unsigned crc_val;
	size_t offset;

	if (!record->data.file_cnt || !record->data.pageseq_cnt ||
		!record->owner.record_path)
		return ret;

	ws_dbg("%s: writeback %s record data to %s\n",
		__func__, record->owner.name, record->owner.record_path);
	filp = filp_open(record->owner.record_path,
		O_LARGEFILE | O_RDWR, S_IRUSR | S_IWUSR);
	if (IS_ERR_OR_NULL(filp)) {
		ws_dbg("%s: open %s, ret = %ld\n", __func__,
			record->owner.record_path, PTR_ERR(filp));
		return ret;
	}

	offset = offsetof(struct s_ws_backup_record_header, record_version);
	if (record->state & E_RECORD_STATE_UPDATE_HEADER_ONLY) {
		/*in the case, we update the header of record only.*/
		if (workingset_get_record_header(filp, offset, &header))
			goto out;
#ifdef CONFIG_TASK_DELAY_ACCT
		header.leader_blkio_cnt = record->leader_blkio_cnt;
		header.need_update = record->need_update;
#endif
	} else {
		/*we write back the playload of record first.*/
		if (workingset_record_writeback_playload_rcrdlocked(filp,
			record, &header.playload_length,
			&header.playload_checksum))
			goto out;
		header.file_cnt = record->data.file_cnt;
		header.pageseq_cnt = record->data.pageseq_cnt;
		header.page_sum = record->data.page_sum;
		header.record_version = CGROUP_WORKINGSET_VERSION;
#ifdef CONFIG_TASK_DELAY_ACCT
		header.leader_blkio_cnt = record->leader_blkio_cnt;
		header.need_update = record->need_update;
#endif
	}

	/*the last, we write back the playload of record.*/
	crc_val = workingset_crc32c(0,
		&header.record_version, sizeof(header) - offset);
	if (!crc_val) {
		pr_err("%s line %d: checksum=0 crc_val=%u\n",
			__func__, __LINE__, crc_val);
		goto out;
	}

	header.header_crc = crc_val;
	header.magic = WORKINGSET_RECORD_MAGIC;
	if (sizeof(header) == kernel_write(filp,
		(char *)&header, sizeof(header), 0))
		ret = true;
	else {
		pr_err("%s line %d: kernel_write failed\n",
			__func__, __LINE__);
		goto out;
	}
out:
	filp_close(filp, NULL);
	return ret;
}

/**
 * workingset_record_realloc_ownerbuffer_if_need -
 * realloc memory for information of new owner .
 * @scanned: the old owner be replaced.
 * @owner: the new owner.
 *
 * if the size of @scanned owner information is larger than
 * @owner requests, reuse the memory of @scanned.
 */
static int workingset_record_realloc_ownerbuffer_if_need(
	struct s_ws_owner *scanned, struct s_ws_owner *owner)
{
	int ret = 0;
	unsigned name_len, path_len;
	char *new_name = NULL;
	char *new_path = NULL;

	if (!owner->name || !owner->record_path)
		return -EINVAL;

	name_len = strlen(owner->name);
	path_len = strlen(owner->record_path);
	if (!scanned->name || (strlen(scanned->name) < name_len)) {
		new_name = kzalloc(name_len + 1, GFP_NOFS);
		if (!new_name) {
			ret = -ENOMEM;
			goto alloc_name_fail;
		}
	}
	if (!scanned->record_path ||
		(strlen(scanned->record_path) < path_len)) {
		new_path = kzalloc(path_len + 1, GFP_NOFS);
		if (!new_path) {
			ret = -ENOMEM;
			goto alloc_path_fail;
		}
	}

	if (new_name) {
		kfree(scanned->name);
		scanned->name = new_name;
	}
	if (new_path) {
		kfree(scanned->record_path);
		scanned->record_path = new_path;
	}
	return 0;

alloc_path_fail:
	kfree(new_name); /*lint !e668*/
alloc_name_fail:
	return ret;
}

/**
 * workingset_writeback_last_record_if_need -
 * try to writeback the last record by lru .
 *
 * we consider the writeback is allways successful,
 * so clear the dirty flag of record.
 */
static void workingset_writeback_last_record_if_need(void)
{
	struct s_ws_record *record;

	if (g_record_cnt >= g_max_records_count) {
		spin_lock(&g_record_list_lock);
		record = list_empty(&g_record_list) ? NULL :
			list_last_entry(&g_record_list,
				struct s_ws_record, list);
		if (record) {
			list_del(&record->list);
			g_record_cnt--;
			spin_unlock(&g_record_list_lock);
			mutex_lock(&record->mutex);
			if ((record->state &
			(E_RECORD_STATE_USED | E_RECORD_STATE_DIRTY)) ==
			(E_RECORD_STATE_USED | E_RECORD_STATE_DIRTY)) {
				workingset_backup_record_rcrdlocked(record);
				record->state &= ~(E_RECORD_STATE_DIRTY |
					E_RECORD_STATE_UPDATE_HEADER_ONLY);
			}
			mutex_unlock(&record->mutex);
			spin_lock(&g_record_list_lock);
			g_record_cnt++;
			list_add_tail(&record->list, &g_record_list);
		}
		spin_unlock(&g_record_list_lock);
	}
}

/**
 * workingset_writeback_all_records -
 * writeback any cached records if they are dirty.
 */
static void workingset_writeback_all_records(void)
{
	LIST_HEAD(temp_list);
	struct s_ws_record *record;
	struct list_head *pos;
	struct list_head *head = &g_record_list;
	int total_records = 0;
	int writeback_cnt = 0;

	spin_lock(&g_record_list_lock);
	while (!list_empty(head)) {
		pos = head->prev;
		list_del(pos);
		g_record_cnt--;
		spin_unlock(&g_record_list_lock);
		record = container_of(pos, struct s_ws_record, list);
		mutex_lock(&record->mutex);
		if ((record->state &
			(E_RECORD_STATE_USED | E_RECORD_STATE_DIRTY))
			== (E_RECORD_STATE_USED | E_RECORD_STATE_DIRTY)) {
			if (workingset_backup_record_rcrdlocked(record))
				writeback_cnt++;
			record->state &= ~(E_RECORD_STATE_DIRTY |
				E_RECORD_STATE_UPDATE_HEADER_ONLY);
		}
		total_records++;
		list_add(pos, &temp_list);
		mutex_unlock(&record->mutex);
		spin_lock(&g_record_list_lock);
	}
	list_splice(&temp_list, &g_record_list);
	g_record_cnt += total_records;
	spin_unlock(&g_record_list_lock);
	ws_dbg("%s: total records=%u, writebacked=%d\n",
		__func__, total_records, writeback_cnt);
}

/**
 * workingset_get_playload_addr_rcrdlocked -
 * get the playlod address of record and reset record.
 * @owner: the struct file pointer of opened file.
 * @record: the record need to reset.
 * @is_exist: indicate there is or not a record of the owner already.
 * @page_array: the pointer of page array of record.
 * @playload_pages: the count of pages.
 *
 * you need hold the lock of record before call this function.
 */
static void *workingset_get_playload_addr_rcrdlocked(
	struct s_ws_owner *owner, struct s_ws_record *record,
	bool is_exist, struct page **page_array,
	unsigned playload_pages)
{
	void *playload;

	playload = vmap(page_array, playload_pages, VM_MAP, PAGE_KERNEL);
	if (!playload) {
		pr_err("%s: out of space, vmap %u pages failed!\n",
			__func__, playload_pages);
		return NULL;
	}

	/**
	 * we don't need realloc memory when record of the owner is exist.
	 */
	if (!is_exist && workingset_record_realloc_ownerbuffer_if_need(
		&record->owner, owner)) {
		vunmap(playload);
		return NULL;
	}
	workingset_recycle_record_rcrdlocked(record);
	return playload;
}

/**
 * workingset_get_available_record -
 * prefer to select a clean record by lru.
 */
static struct s_ws_record *workingset_get_available_record(void)
{
	struct s_ws_record *record = NULL;
	struct list_head *pos;

	/*
	 * if record is not existed, we replace oldest
	 * clean record in list.
	 */
	spin_lock(&g_record_list_lock);
	list_for_each_prev(pos, &g_record_list) {
		record = container_of(pos,
			struct s_ws_record, list);
		if (!(record->state & (E_RECORD_STATE_DIRTY |
			E_RECORD_STATE_PREREADING)))
			break;
	}
	if (pos == &g_record_list)
		record = NULL;

	if (record) {
		list_del(&record->list);
		g_record_cnt--;
		spin_unlock(&g_record_list_lock);
	} else {
		spin_unlock(&g_record_list_lock);
		record = kzalloc(sizeof(struct s_ws_record),
					GFP_NOFS);
		if (record)
			mutex_init(&record->mutex);
	}

	return record;
}

static int workingset_get_record_header_wrapper(struct file *filp,
	struct s_ws_backup_record_header *header)
{
	int ret;
	size_t offset;

	offset = offsetof(struct s_ws_backup_record_header, record_version);
	ret = workingset_get_record_header(filp, offset, header);
	if (ret)
		goto out;

	if (header->playload_length >
		(MAX_TOUCHED_PAGES_COUNT * sizeof(unsigned)) +
		 (sizeof(struct s_path_node) + PATH_MAX_CHAR) *
		 MAX_TOUCHED_FILES_COUNT) {
		pr_err("%s line %d: playload(%u) is large than limit(%llu)\n",
			__func__, __LINE__, header->playload_length,
			(MAX_TOUCHED_PAGES_COUNT * sizeof(unsigned)));
		ret = -EIO;
	}

out:
	return ret;
}

/**
 * workingset_prepare_record_pages -
 * alloc pages for playload data.
 * @data: the collected data who prepare memory for.
 * @playload_pages: the count of pages to alloc.
 */
static struct page **workingset_prepare_record_pages(
	struct s_ws_data *data, unsigned playload_pages, unsigned *pidx)
{
	unsigned idx = 0;
	struct page *page, **page_array = NULL;

	page_array = kcalloc(playload_pages, sizeof(struct page *), GFP_NOFS);
	if (!page_array)
		return NULL;

	if (data->array_page_cnt) {
		int ret;
		size_t cpy_size;

		cpy_size = sizeof(struct page *) * data->array_page_cnt;
		ret = memcpy_s(page_array, cpy_size, data->page_array,
			 cpy_size);
		if (ret) {
			ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
			goto copy_fail;
		}
	}

	idx = data->array_page_cnt;
	while (idx < playload_pages) {
		page = alloc_page(GFP_NOFS | __GFP_ZERO);
		if (!page) {
			pr_err("%s: OOM, alloc %u pages failed!\n",
				__func__, playload_pages);
			goto alloc_fail;
		}
		page_array[idx] = page;
		idx++;
	}
	*pidx = idx;
	return page_array;

alloc_fail:
	while (idx-- > data->array_page_cnt)
		__free_page(page_array[idx]);
copy_fail:
	kfree(page_array);
	return NULL;
}

static int workingset_prepare_record_space_wsrcrdlocked(
	struct s_ws_owner *owner, struct s_ws_record *record, bool is_exist,
	unsigned playload_size, void **pplayload)
{
	int ret = 0;
	struct s_ws_data *data = &record->data;
	struct page **page_array = NULL;
	void *playload;
	unsigned playload_pages;
	unsigned idx = 0;

	if ((!data->page_array && data->array_page_cnt)
		|| (data->page_array && !data->array_page_cnt)) {
		pr_err("%s: page_array=%p, page_cnt=%u never happend!\n",
			__func__, data->page_array, data->array_page_cnt);
		ret = -EINVAL;
		goto out;
	}

	playload_pages = DIV_ROUND_UP(playload_size, PAGE_SIZE);
	/**
	  * In order to avoid more direct reclaim, so we reuse the memory
	  * of old record as far as possible when replace it.
	  */
	if (data->array_page_cnt < playload_pages) {
		page_array = workingset_prepare_record_pages(data,
			playload_pages, &idx);
		if (!page_array) {
			ret = -ENOMEM;
			goto out;
		}

		playload = workingset_get_playload_addr_rcrdlocked(owner,
			record, is_exist, page_array, playload_pages);
		if (!playload) {
			ret =  -ENOSPC;
			goto vmap_fail;
		}
		if (data->page_array) {
			/*unmap old space*/
			vunmap(data->file_array);
			kfree(data->page_array);
		}
		data->page_array = page_array;
	} else {
		playload = workingset_get_playload_addr_rcrdlocked(owner,
			record, is_exist, data->page_array, playload_pages);
		if (!playload) {
			ret =  -ENOSPC;
			goto out;
		}

		/*unmap old space and free unnecessary memory.*/
		vunmap(data->file_array);
		idx = data->array_page_cnt;
		while (idx-- > playload_pages) {
			__free_page(data->page_array[idx]);
			data->page_array[idx] = NULL;
		}
	}
	data->array_page_cnt = playload_pages;
	*pplayload = data->file_array = playload;
	return 0;

vmap_fail:
	if (page_array) {
		while (idx-- > data->array_page_cnt)
			__free_page(page_array[idx]);
		kfree(page_array);
	}
out:
	return ret;
}

static struct s_ws_record *workingset_get_record_from_backup(
	struct s_ws_owner *owner)
{
	int ret;
	struct file *filp;
	struct s_ws_data *data;
	struct s_ws_record *record = NULL;
	struct s_ws_backup_record_header header = {0,};
	unsigned idx = 0;
	unsigned pathnode_size;
	unsigned len;
	void *playload;

	filp = filp_open(owner->record_path, O_LARGEFILE | O_RDONLY, 0);
	if (IS_ERR_OR_NULL(filp))
		return NULL;

	ws_dbg("%s: read record data from %s\n",
		__func__, owner->record_path);
	ret = workingset_get_record_header_wrapper(filp, &header);
	if (ret)
		goto out;

	record = workingset_get_available_record();
	if (!record)
		goto out;

	mutex_lock(&record->mutex);
	ret = workingset_prepare_record_space_wsrcrdlocked(owner,
		record, false, header.playload_length, &playload);
	if (ret)
		goto alloc_space_fail;

	if (header.playload_length != kernel_read(filp,
		sizeof(header), playload, header.playload_length)) {
		pr_err("%s line %d: kernel_read failed!\n",
			__func__, __LINE__);
		goto alloc_space_fail;
	}

	if (header.playload_checksum != workingset_crc32c(0,
		playload, header.playload_length)) {
		pr_err("%s line %d: workingset_crc32c failed!\n",
			__func__, __LINE__);
		goto alloc_space_fail;
	}

	len = strlen(owner->name) + 1;
	ret = memcpy_s(record->owner.name, len, owner->name, len);
	if (ret) {
		ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
		goto alloc_space_fail;
	}

	len = strlen(owner->record_path) + 1;
	ret = memcpy_s(record->owner.record_path, len,
		owner->record_path, len);
	if (ret) {
		ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
		goto alloc_space_fail;
	}

	record->state = E_RECORD_STATE_USED | E_RECORD_STATE_DATA_FROM_BACKUP;
	record->owner.uid = owner->uid;
	data = &record->data;
	data->file_cnt = header.file_cnt;
	data->pageseq_cnt = header.pageseq_cnt;
	data->page_sum = header.page_sum;
#ifdef CONFIG_TASK_DELAY_ACCT
	record->leader_blkio_cnt = header.leader_blkio_cnt;
	record->need_update = header.need_update;
#endif
	pathnode_size = sizeof(struct s_path_node) * header.file_cnt;
	for (idx = 0, len = 0; idx < header.file_cnt; idx++) {
		if (!data->file_array[idx].path)
			continue;
		data->file_array[idx].path =
			playload + pathnode_size + len;
		len += data->file_array[idx].pathlen + 1;
	}
	data->cacheseq = playload + pathnode_size + len;
	workingset_insert_record_to_list_head(record);
	mutex_unlock(&record->mutex);

	filp_close(filp, NULL);
	ws_dbg("%s: read record data from %s completely!\n",
		__func__, owner->record_path);
	return record;

alloc_space_fail:
	workingset_insert_record_to_list_tail(record);
	record->state &= ~(E_RECORD_STATE_USED | E_RECORD_STATE_DIRTY |
					E_RECORD_STATE_UPDATE_HEADER_ONLY);
	mutex_unlock(&record->mutex);
out:
	filp_close(filp, NULL);
	return NULL;
}

/** workingset_get_existed_record_wslocked -
 * find the record of owner from cache or blockdev
 * @owner: the owner of record that we will be find.
 * @onlycache: don't get record from disk if it is true.
 *
 * we adjust the record to the head of list when we found it.
 */
static struct s_ws_record *workingset_get_existed_record_wslocked(
	struct s_ws_owner *owner, bool onlycache)
{
	struct s_ws_record *record = NULL;
	struct list_head *pos;
	struct list_head *head = &g_record_list;

	if (!owner->name)
		return NULL;

	spin_lock(&g_record_list_lock);
	list_for_each(pos, head) {
		record = container_of(pos, struct s_ws_record, list);
		if ((record->state & E_RECORD_STATE_USED)
			&& (record->owner.uid == owner->uid)
			&& !strcmp(record->owner.name, owner->name)) {
			break;
		}
	}

	if (pos != head) {
		list_move(pos, head);
		spin_unlock(&g_record_list_lock);
	} else if (!onlycache && owner->record_path) {
		spin_unlock(&g_record_list_lock);
		record = workingset_get_record_from_backup(owner);
	} else {
		spin_unlock(&g_record_list_lock);
		record = NULL;
	}

	return record;
}

/*lint -e454*/
/*lint -e456*/
static void workingset_destroy_data(struct s_workingset *ws, bool is_locked)
{
	struct list_head *head;
	struct s_file_info *fileinfo;

	if (!is_locked)
		mutex_lock(&ws->mutex);

	head = &ws->file_list;
	while (!list_empty(head)) {
		fileinfo = list_first_entry(head, struct s_file_info, list);
		list_del(&fileinfo->list);
		fileinfo->rbroot = RB_ROOT;
		kfree(fileinfo->path_node.path);

		if (fileinfo->filp_list) {
			struct s_filp_list *curr, *next;

			curr = fileinfo->filp_list;
			do {
				next = curr->next;
				if (curr->filp)
					fput(curr->filp);
				kfree(curr);
				curr = next;
			} while (curr);
		}

		kfree(fileinfo);
	}
	kfree(ws->owner.name);
	kfree(ws->owner.record_path);

	ws->owner.uid = 0;
	ws->owner.name = NULL;
	ws->owner.record_path = NULL;
	ws->repeated_count = 0;
	ws->page_sum = 0;
	ws->stage_num = 0;
#ifdef CONFIG_TASK_DELAY_ACCT
	ws->leader_blkio_cnt = 0;
	ws->leader_blkio_base = 0;
#endif
	ws->file_count = 0;
	ws->pageseq_count = 0;
	ws->alloc_index = 0;
	if (!is_locked)
		mutex_unlock(&ws->mutex);
}
/*lint +e456*/
/*lint +e454*/

static inline struct s_workingset *css_workingset(
	struct cgroup_subsys_state *css)
{
	return container_of(css, struct s_workingset, css);
}

static const char *workingset_state_strs(unsigned int state)
{
	unsigned monitor_state;

	switch (state) {
	case E_CGROUP_STATE_MONITOR_INWORKING:
		monitor_state = E_MONITOR_STATE_INWORKING;
	break;
	case E_CGROUP_STATE_MONITOR_PAUSED:
		monitor_state = E_MONITOR_STATE_PAUSED;
	break;
	case E_CGROUP_STATE_MONITOR_PREREAD:
		monitor_state = E_MONITOR_STATE_PREREAD;
	break;
	case E_CGROUP_STATE_MONITOR_BACKUP:
		monitor_state = E_MONITOR_STATE_BACKUP;
	break;
	case E_CGROUP_STATE_MONITOR_STOP:
		monitor_state = E_MONITOR_STATE_STOP;
	break;
	case E_CGROUP_STATE_MONITOR_ABORT:
		monitor_state = E_MONITOR_STATE_ABORT;
	break;
	default:
		monitor_state = E_MONITOR_STATE_OUTOFWORK;
	break;
	}

	return moniter_states[monitor_state];
};

static struct cgroup_subsys_state *
workingset_css_alloc(struct cgroup_subsys_state *parent_css)
{
	struct s_workingset *ws;

	/**
	 * we alloc a page for saving struct s_workingset,
	 * because it need save pointer of pages
	 * that caching page offset range information.
	 */
	ws = (struct s_workingset *)get_zeroed_page(GFP_KERNEL);
	if (!ws)
		return ERR_PTR(-ENOMEM);

	mutex_init(&ws->mutex);
	return &ws->css;
}

/**
 * workingset_css_online - commit creation of a workingset css
 * @css: css being created
 *
 */
static int workingset_css_online(struct cgroup_subsys_state *css)
{
	struct s_workingset *ws;

	if (!css)
		return -EINVAL;
	ws = css_workingset(css);
	mutex_lock(&ws->mutex);
	ws->state = E_CGROUP_STATE_ONLINE;
	ws->file_count = 0;
	ws->pageseq_count = 0;
	ws->repeated_count = 0;
	ws->page_sum = 0;
	ws->stage_num = 0;
#ifdef CONFIG_TASK_DELAY_ACCT
	ws->leader_blkio_cnt = 0;
	ws->leader_blkio_base = 0;
#endif
	ws->alloc_index = 0;
	INIT_LIST_HEAD(&ws->file_list);

	if (!workingset_register_shrinker(ws))
		ws->shrinker_enabled = true;
	else
		ws->shrinker_enabled = false;

	mutex_unlock(&ws->mutex);
	return 0;
}

/**
 * workingset_css_offline - initiate destruction of a workingset css
 * @css: css being destroyed
 *
 */
static void workingset_css_offline(struct cgroup_subsys_state *css)
{
	struct s_workingset *ws;

	if (css) {
		ws = css_workingset(css);
		mutex_lock(&ws->mutex);

		ws->state = E_CGROUP_STATE_OFFLINE;
		workingset_destroy_data(ws, true);

		workingset_unregister_shrinker(ws);

		mutex_unlock(&ws->mutex);
	}
}

static void workingset_css_free(struct cgroup_subsys_state *css)
{
	if (css)
		free_page((unsigned long)css_workingset(css));
}

static int workingset_can_attach(struct cgroup_taskset *tset)
{
	return g_module_initialized ? 0 : -ENODEV;
}

#ifdef CONFIG_TASK_DELAY_ACCT
static void workingset_blkio_monitor_wslocked(struct s_workingset *ws,
	unsigned monitor_state)
{
	if (monitor_state == E_CGROUP_STATE_MONITOR_INWORKING
		|| monitor_state == E_CGROUP_STATE_MONITOR_PAUSED
		|| monitor_state == E_CGROUP_STATE_MONITOR_STOP) {
		struct task_struct *tsk;

		rcu_read_lock();
		tsk = find_task_by_vpid(ws->owner.pid);
		if (!tsk) {
			rcu_read_unlock();
			return;
		}

		get_task_struct(tsk);
		rcu_read_unlock();

		if (monitor_state == E_CGROUP_STATE_MONITOR_INWORKING)
			ws->leader_blkio_base = tsk->delays->blkio_count;
		else if (tsk->delays->blkio_count > ws->leader_blkio_base)
			ws->leader_blkio_cnt +=
				(unsigned short)(tsk->delays->blkio_count
				- ws->leader_blkio_base);
		put_task_struct(tsk);
	}
}
#endif

/**
 * workingset_apply_state - apply state change to a single cgroup_workingset
 * @ws: workingset to apply state change to
 * @monitor_state: the state of monitor of workingset.
 *
 * Set @state on @ws according to @monitor_state, and perform
 * inworking or outwork as necessary.
 */
static void workingset_apply_state(struct s_workingset *ws,
	unsigned monitor_state)
{
	mutex_lock(&ws->mutex);
	if (ws->state & E_CGROUP_STATE_ONLINE) {
#ifdef CONFIG_TASK_DELAY_ACCT
		if ((monitor_state & E_CGROUP_STATE_MONITORING)
			&& !(ws->state & E_CGROUP_STATE_MONITORING)) {
			workingset_blkio_monitor_wslocked(ws, monitor_state);
		} else if ((ws->state & E_CGROUP_STATE_MONITORING)
			&& !(monitor_state & E_CGROUP_STATE_MONITORING)) {
			workingset_blkio_monitor_wslocked(ws, monitor_state);
		}
#endif
		if ((ws->stage_num < PAGE_STAGE_NUM_MASK)
			&& (ws->stage_num
			|| (ws->state == E_CGROUP_STATE_MONITOR_INWORKING)))
			ws->stage_num++;
		ws->state &= ~E_CGROUP_STATE_MONITOR_BITMASK;
		ws->state |= monitor_state;
	}
	mutex_unlock(&ws->mutex);
}

static void workingset_collector_reset(struct s_workingset *ws)
{
	spin_lock(&g_collector->lock);
	if (g_collector->monitor == ws) {
		g_collector->monitor = NULL;
		g_collector->read_pos = g_collector->write_pos = 0;
		g_collector->discard_count = 0;
	}
	spin_unlock(&g_collector->lock);
}

/**
 * workingset_change_state -
 * change the enter or exit state of a cgroup_workingset
 * @ws: workingset of interest
 * @monitor_state: the state of monitor of workingset.
 *
 * The operations are recursive -
 * all descendants of @workingset will be affected.
 */
static void workingset_change_state(struct s_workingset *ws,
	unsigned monitor_state)
{
	struct cgroup_subsys_state *pos;

	rcu_read_lock();
	css_for_each_descendant_pre(pos, &ws->css) {
		struct s_workingset *pos_f = css_workingset(pos);

		if (!css_tryget_online(pos))
			continue;
		rcu_read_unlock();

		workingset_apply_state(pos_f, monitor_state);

		rcu_read_lock();
		css_put(pos);
	}
	rcu_read_unlock();
}

static ssize_t workingset_get_target_state(struct s_workingset *ws, char *buf,
	size_t nbytes, unsigned *target_state)
{
	ssize_t nr_write = 0;

	buf = strstrip(buf);
	if (strcmp(buf, workingset_state_strs(
		E_CGROUP_STATE_MONITOR_INWORKING)) == 0) {
		*target_state = E_CGROUP_STATE_MONITOR_INWORKING;
	} else if (strcmp(buf, workingset_state_strs(
		E_CGROUP_STATE_MONITOR_PAUSED)) == 0) {
		*target_state = E_CGROUP_STATE_MONITOR_PAUSED;
	} else if (strcmp(buf, workingset_state_strs(
		E_CGROUP_STATE_MONITOR_STOP)) == 0) {
		*target_state = E_CGROUP_STATE_MONITOR_STOP;
	} else if (strcmp(buf, workingset_state_strs(
		E_CGROUP_STATE_MONITOR_ABORT)) == 0) {
		*target_state = E_CGROUP_STATE_MONITOR_ABORT;
	} else if (strcmp(buf, workingset_state_strs(
		E_CGROUP_STATE_MONITOR_PREREAD)) == 0) {
		*target_state = E_CGROUP_STATE_MONITOR_PREREAD;
	} else if (strcmp(buf, workingset_state_strs(
		E_CGROUP_STATE_MONITOR_BACKUP)) == 0) {
		workingset_writeback_all_records();
		nr_write = nbytes;
	} else {
		nr_write = -EINVAL;
	}

	return nr_write;
}

static void workingset_prereader_handler(struct s_workingset *ws,
	unsigned target_state)
{
	if (target_state == E_CGROUP_STATE_MONITOR_PREREAD
		|| target_state == E_CGROUP_STATE_MONITOR_STOP
		|| target_state == E_CGROUP_STATE_MONITOR_ABORT) {
		struct s_ws_record *record;

		mutex_lock(&ws->mutex);
		record = workingset_get_existed_record_wslocked(&ws->owner,
			false);
		mutex_unlock(&ws->mutex);

		if (record)  {
			if (target_state == E_CGROUP_STATE_MONITOR_ABORT)
				atomic_set(&g_preread_abort, 1);

			mutex_lock(&record->mutex);
			if ((target_state == E_CGROUP_STATE_MONITOR_PREREAD)
			&& !(record->state & E_RECORD_STATE_PREREADING)) {
				record->state |= E_RECORD_STATE_PREREADING;
				workingset_do_preread_work_rcrdlocked(record);
			} else if (target_state ==
				E_CGROUP_STATE_MONITOR_STOP) {
				record->state &= ~E_RECORD_STATE_PREREADING;
			} else if (target_state ==
				E_CGROUP_STATE_MONITOR_ABORT) {
				record->state &= ~(E_RECORD_STATE_PREREADING |
					E_RECORD_STATE_UPDATE_BASE_BLKIO);
				atomic_set(&g_preread_abort, 0);
			}
			mutex_unlock(&record->mutex);
		}
	}
}

static ssize_t workingset_collector_start_handler(struct s_workingset *ws,
	unsigned target_state)
{
	spin_lock(&g_collector->lock);
	if (g_collector->monitor && g_collector->monitor != ws) {
		spin_unlock(&g_collector->lock);
		return -EBUSY;
	}
	g_collector->monitor = ws;
	spin_unlock(&g_collector->lock);

	return 0;
}

static void workingset_collector_stop_handler(struct cgroup_subsys_state *css,
	struct s_workingset *ws, unsigned target_state)
{
	if (target_state == E_CGROUP_STATE_MONITOR_STOP) {
		spin_lock(&g_collector->lock);
		g_collector->wait_flag = F_RECORD_PENDING;
		/*notify the collect thread monitor is stoped*/
		if (waitqueue_active(&g_collector->collect_wait)) {
			wake_up_interruptible_all(
				&g_collector->collect_wait);
		}
		spin_unlock(&g_collector->lock);
	} else if (target_state == E_CGROUP_STATE_MONITOR_ABORT) {
		workingset_destroy_data(ws, false);
		workingset_collector_reset(ws);
	}
}

static ssize_t workingset_state_write(struct kernfs_open_file *of,
	char *buf, size_t nbytes, loff_t off)
{
	ssize_t nr_write;
	unsigned target_state = E_CGROUP_STATE_MAX;
	struct cgroup_subsys_state *css;
	struct s_workingset *ws;

	if (!g_module_initialized)
		return -ENODEV;

	if (!of || !buf)
		return -EINVAL;
	css = of_css(of);
	if (!css)
		return -EINVAL;
	ws = css_workingset(css);
	nr_write = workingset_get_target_state(ws, buf, nbytes, &target_state);
	if (nr_write)
		return nr_write;

	if (target_state == E_CGROUP_STATE_MONITOR_INWORKING) {
		nr_write = workingset_collector_start_handler(ws, target_state);
		if (nr_write)
			return nr_write;
	}

	ws_dbg("%s: uid=%u, name=%s, old_state=%s\n", __func__, ws->owner.uid,
		ws->owner.name, workingset_state_strs(ws->state));

	if (target_state != E_CGROUP_STATE_MONITOR_PREREAD)
		workingset_change_state(ws, target_state);

	workingset_prereader_handler(ws, target_state);

	workingset_collector_stop_handler(css, ws, target_state);

	/*writeback a dirty record when we preread completely*/
	if (target_state == E_CGROUP_STATE_MONITOR_PREREAD)
		workingset_writeback_last_record_if_need();

	ws_dbg("%s: uid=%u, name=%s, new_state=%s\n", __func__, ws->owner.uid,
		ws->owner.name, workingset_state_strs(ws->state));
	return nbytes;
}

static int workingset_state_read(struct seq_file *m, void *v)
{
	struct cgroup_subsys_state *css;

	if (!g_module_initialized)
		return -ENODEV;

	if (!m)
		return -EINVAL;
	css = seq_css(m);
	if (!css)
		return -EINVAL;

	seq_puts(m, workingset_state_strs(css_workingset(css)->state));
	seq_putc(m, '\n');
	return 0;
}

#define STRSEP_BLANK(str, token, ret)	\
{	\
	token = strsep(&str, " ");	\
	if (token == NULL || str == NULL) {	\
		ret = -EINVAL;	\
	}	\
}

/**workingset_data_parse_owner -
 * parse information of the owner of workingset from the comming string.
 * @ws workingset the owner working on.
 * @owner_string the comming string.
 *
 **/
static int workingset_data_parse_owner(struct s_workingset *ws,
	char *owner_string)
{
	int ret = 0;
	char *str = owner_string;
	char *token;
	int pid;
	unsigned uid, len;
	char *owner_name;
	char *record_path;

	/*the 1th: uid*/
	STRSEP_BLANK(str, token, ret);
	if (ret)
		goto out;
	ret = kstrtouint(token, 0, &uid);
	if (ret)
		goto out;

	/*the 2th: pid*/
	STRSEP_BLANK(str, token, ret);
	if (ret)
		goto out;
	ret = kstrtouint(token, 0, &pid);
	if (ret)
		goto out;

	/*the 3th: name of owner*/
	STRSEP_BLANK(str, token, ret);
	if (ret)
		goto out;
	len = strlen(token);	/*lint !e668*/
	if (len <= 0 || len >= OWNER_MAX_CHAR) {
		ret = -EINVAL;
		goto out;
	}
	len++;
	owner_name = kzalloc(len, GFP_NOFS);
	if (!owner_name) {
		ret = -ENOMEM;
		goto out;
	}
	ret = strncpy_s(owner_name, len, token, len);
	if (ret) {
		ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
		ret = -EINVAL;
		goto parse_path_failed;
	}

	/*the 4th: the path of record*/
	len = strlen(str);	/*lint !e668*/
	if (len <= 0 || len >= PATH_MAX_CHAR) {
		ret = -EINVAL;
		goto parse_path_failed;
	}
	len++;
	record_path = kzalloc(len, GFP_NOFS);
	if (!record_path) {
		ret = -ENOMEM;
		goto parse_path_failed;
	}
	ret = strncpy_s(record_path, len, str, len);
	if (ret) {
		ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
		ret = -EINVAL;
		goto copy_path_failed;
	}

	mutex_lock(&ws->mutex);
	ws->owner.uid = uid;
	ws->owner.pid = pid;

	kfree(ws->owner.name);
	ws->owner.name = owner_name;

	kfree(ws->owner.record_path);
	ws->owner.record_path = record_path;
	mutex_unlock(&ws->mutex);
	return 0;

copy_path_failed:
	kfree(record_path);
parse_path_failed:
	kfree(owner_name);
out:
	return ret;
}

static ssize_t workingset_data_write(struct kernfs_open_file *of,
			     char *buf, size_t nbytes, loff_t off)
{
	int ret;
	struct cgroup_subsys_state *css;

	if (!g_module_initialized)
		return -ENODEV;

	if (!of || !buf)
		return -EINVAL;
	css = of_css(of);
	if (!css)
		return -EINVAL;

	buf = strstrip(buf);
	ret = workingset_data_parse_owner(css_workingset(css), buf);
	if (ret)
		return ret;
	else
		return nbytes;
}

static int workingset_data_read(struct seq_file *m, void *v)
{
	struct cgroup_subsys_state *css;
	struct s_workingset *ws;
	struct s_ws_record *record;

	if (!g_module_initialized)
		return -ENODEV;

	if (!m)
		return -EINVAL;
	css = seq_css(m);
	if (!css)
		return -EINVAL;

	ws = css_workingset(css);
	mutex_lock(&ws->mutex);
#ifdef CONFIG_HW_CGROUP_WORKINGSET_DEBUG
	seq_printf(m, "Uid: %u\n", ws->owner.uid);
	seq_printf(m, "Pid: %d\n", ws->owner.pid);
	seq_printf(m, "Name: %s\n",
		ws->owner.name ? ws->owner.name : "Unknow");
	seq_printf(m, "RecordPath: %s\n",
		ws->owner.record_path ? ws->owner.record_path : "Unknow");
#endif
	record = workingset_get_existed_record_wslocked(&ws->owner, false);
#ifdef CONFIG_TASK_DELAY_ACCT
	seq_printf(m, "RecordState:%s\n",
	!record ? "none" : (record->need_update ? "older" : "uptodate"));
#else
	seq_printf(m, "RecordState:%s\n", !record ? "none" : "uptodate");
#endif
	mutex_unlock(&ws->mutex);

	return 0;
}

static struct cftype files[] = {
	{
		.name = "state",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = workingset_state_read,
		.write = workingset_state_write,
	},
	{
		.name = "data",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = workingset_data_read,
		.write = workingset_data_write,
	},
	{ }	/* terminate */
};

struct cgroup_subsys workingset_cgrp_subsys = {
	.css_alloc	= workingset_css_alloc,
	.css_online	= workingset_css_online,
	.css_offline	= workingset_css_offline,
	.css_free	= workingset_css_free,
	.can_attach = workingset_can_attach,
	.legacy_cftypes	= files,
};

static int get_file_path_and_hashcode(struct file *file, char *buf,
	unsigned int buf_size, char **str_path,
	unsigned int *len, unsigned int *hashcode)
{
	unsigned int path_len;
	char *filepath;

	filepath = d_path(&file->f_path, buf, buf_size - 1);
	if (IS_ERR_OR_NULL(filepath))
		return -1;

	path_len = strlen(filepath);
	*str_path = filepath;
	*len = path_len;
	*hashcode = workingset_crc32c(0, filepath, path_len);
	return 0;
}

static struct s_file_info *workingset_alloc_fileinfo(unsigned path_len,
	gfp_t gfp_mask)
{
	struct s_file_info *fileinfo;

	fileinfo = kzalloc(sizeof(struct s_file_info), gfp_mask);
	if (!fileinfo)
		goto out;

	fileinfo->path_node.path = kzalloc(
		path_len + 1, gfp_mask);
	if (!fileinfo->path_node.path)
		goto fileinfo_free;

	fileinfo->filp_list = kzalloc(sizeof(struct s_filp_list),
					gfp_mask);
	if (!fileinfo->filp_list)
		goto filepath_free;
	return fileinfo;

filepath_free:
	kfree(fileinfo->path_node.path);
fileinfo_free:
	kfree(fileinfo);
out:
	return NULL;
}

static int workingset_record_fileinfo_if_need_wslocked(struct s_workingset *ws,
	struct file *file, struct s_file_info **file_info, bool *is_existed)
{
	int ret = 0;
	int seq_num = 0;
	bool existed = false;
	struct s_file_info *fileinfo;
	struct inode	*inode = file->f_mapping->host;
	struct s_filp_list *filp_list;
	struct list_head *pos;
	struct list_head *head;
	unsigned int path_len;
	char *filepath;
	unsigned hashcode;
	char buf[PATH_MAX_CHAR] = {'\0',};

	if (ws->pageseq_count >= MAX_TOUCHED_PAGES_COUNT) {
		ret = -ENOSPC;
		goto out;
	}

	/*first, match inode when search same file*/
	head = &ws->file_list;
	list_for_each(pos, head) {
		fileinfo = container_of(pos, struct s_file_info, list);
		filp_list = fileinfo->filp_list;
		while (filp_list) {
			if (filp_list->filp->f_mapping->host == inode) {
				existed = true;
				goto done;
			}
			filp_list = filp_list->next;
		}
		seq_num++;
	}

	if (!existed) {
		gfp_t gfp_mask = GFP_NOFS;
		struct kstat stat;

		/*get the path string of file and hashcode of path string.*/
		if (get_file_path_and_hashcode(file, buf, PATH_MAX_CHAR,
			&filepath, &path_len, &hashcode)) {
			pr_warn("%s, get_file_path_and_hashcode failed!\n",
				__func__);
			ret = -EINVAL;
			goto out;
		}

		/*second, match hashcode and string when search same file*/
		seq_num = 0;
		list_for_each(pos, head) {
			fileinfo = container_of(pos, struct s_file_info, list);
			if ((fileinfo->path_node.hashcode == hashcode) &&
				!strcmp(fileinfo->path_node.path, filepath)) {
				filp_list = kzalloc(sizeof(struct s_filp_list),
						gfp_mask);
				if (filp_list) {
					struct s_filp_list *temp =
						fileinfo->filp_list;
					while (temp->next)
						temp = temp->next;
					filp_list->filp = file;
					temp->next = filp_list;
					existed = false;
				} else {
					existed = true;
				}
				goto done;
			}
			seq_num++;
		}

		if (ws->file_count >= MAX_TOUCHED_FILES_COUNT) {
			ret = -ENOSPC;
			goto out;
		}

		ret = vfs_getattr_nosec(&file->f_path, &stat);
		if (ret) {
			ws_dbg("%s, vfs_getattr %s failed! err=%d\n",
				__func__, filepath, ret);
			ret = -EPERM;
			goto out;
		}

		fileinfo = workingset_alloc_fileinfo(path_len, gfp_mask);
		if (!fileinfo) {
			ws_dbg("%s, oom, alloc fileinfo failed!\n", __func__);
			ret = -ENOMEM;
			goto out;
		}
		ret = strncpy_s(fileinfo->path_node.path, path_len + 1,
			filepath, path_len + 1);
		if (ret) {
			ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
			kfree(fileinfo->filp_list);
			kfree(fileinfo->path_node.path);
			kfree(fileinfo);
			ret = -EINVAL;
			goto out;
		}
		fileinfo->filp_list->filp = file;
		fileinfo->path_node.owner_uid = stat.uid.val;
		fileinfo->path_node.hashcode = hashcode;
		fileinfo->path_node.pathlen = path_len;
		fileinfo->pageseq_count = 0;
		fileinfo->rbroot = RB_ROOT;
		list_add_tail(&fileinfo->list, head);
		ws->file_count++;
#ifdef CONFIG_HW_CGROUP_WORKINGSET_DEBUG
		pr_info("%s, include %s, size = %lld\n", __func__, filepath,
			round_up(i_size_read(inode), PAGE_SHIFT));
#endif
	}
done:
	ret = seq_num;
	*file_info = fileinfo;
	*is_existed = existed;
out:
	return ret;
}

static int workingset_dealwith_pagecache_wslocked(struct s_cachepage_info *info,
	struct s_workingset *ws)
{
	int ret = 0, file_idx;
	bool is_existed_file = false;
	struct file *file = info->filp;
	unsigned offset = info->start_offset;
	int pid = info->pid;
	unsigned count = info->count;
	unsigned stage = info->stage;
	struct s_pagecache_info *pagecache;
	struct s_file_info *file_info = NULL;
	int major_touched;
	unsigned repeat_count;
	int page_count_delta;

	if (pid == ws->owner.pid)
		major_touched = 1;
	else
		major_touched = 0;

	/*get position of current file in file list*/
	file_idx = workingset_record_fileinfo_if_need_wslocked(ws, file,
			&file_info, &is_existed_file);
	if (file_idx < 0) {
		ret = file_idx;
		goto done;
	}

	pagecache = workingset_pagecache_info_cache_alloc_wslocked(ws);
	if (!pagecache) {
		ret = -ENOMEM;
		goto done;
	}

	pagecache->offset_range.start =
		CREATE_HANDLE(file_idx, offset);
	pagecache->offset_range.end =
		CREATE_HANDLE(file_idx, (offset + count));
	/*insert page offset range to the range tree of file*/
	if (workingset_range_rb_insert(&file_info->rbroot, pagecache,
		major_touched, stage, &repeat_count, &page_count_delta)) {
		ws->repeated_count += repeat_count;
		ws->page_sum += count - repeat_count;
		ws->pageseq_count += page_count_delta;
		file_info->pageseq_count += page_count_delta;
	} else {
		if (count > 1) {
			pagecache->offset_range.start |=
				PAGE_RANGE_HEAD_BIT_MASK;
			pagecache->offset_range.end |=
				PAGE_RANGE_HEAD_BIT_MASK;
			ws->pageseq_count += 2;
			file_info->pageseq_count += 2;
		} else {
			ws->pageseq_count += 1;
			file_info->pageseq_count += 1;
		}
		ws->page_sum += count;
		ws->alloc_index++;
	}

done:
	if (ret || is_existed_file)
		fput(file);

	return ret;
}

static unsigned workingset_collector_dequeue_buffer_locked(
	struct s_ws_collector *collector, char *buffer, size_t buf_size)
{
	int ret;
	unsigned buffer_pos = 0;
	unsigned read_pos, write_pos;
	unsigned copy_size;

	read_pos = collector->read_pos;
	write_pos = collector->write_pos;

	if (read_pos > write_pos) {
		/*write pointer has beed reversed.*/
		if (COLLECTOR_CACHE_SIZE - read_pos > buf_size)
			copy_size = buf_size;
		else
			copy_size = COLLECTOR_CACHE_SIZE - read_pos;

		ret = memcpy_s(buffer, copy_size,
			collector->circle_buffer + read_pos, copy_size);
		if (ret) {
			ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
			goto out;
		}
		read_pos += copy_size;
		buffer_pos = copy_size;

		/*
		 * pick data from the head of circle buffer
		 * when local buffer is not full.
		 */
		if ((copy_size < buf_size) && write_pos) {
			if (write_pos > (buf_size - copy_size))
				copy_size = buf_size - copy_size;
			else
				copy_size = write_pos;

			ret = memcpy_s(buffer + buffer_pos, copy_size,
				collector->circle_buffer, copy_size);
			if (ret) {
				ws_dbg("%s Line%d,ret=%d\n",
					__func__, __LINE__, ret);
				goto out;
			}
			buffer_pos += copy_size;
			read_pos += copy_size;
		}
	} else {
		if (write_pos - read_pos > buf_size)
			copy_size = buf_size;
		else
			copy_size = write_pos - read_pos;

		ret = memcpy_s(buffer, copy_size,
			collector->circle_buffer + read_pos, copy_size);
		if (ret) {
			ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
			goto out;
		}
		read_pos += copy_size;
		buffer_pos = copy_size;
	}

out:
	collector->read_pos = (read_pos >= COLLECTOR_CACHE_SIZE) ?
		(read_pos - COLLECTOR_CACHE_SIZE) : read_pos;

	return buffer_pos;
}

/*lint -e454*/
static void workingset_collector_do_collect_locked(
	struct s_ws_collector *collector)
{
	char buffer[COLLECTOR_BATCH_COUNT * sizeof(struct s_cachepage_info)];
	unsigned buffer_pos = 0;
	unsigned idx;

	while (collector->read_pos != collector->write_pos) {
		buffer_pos = workingset_collector_dequeue_buffer_locked(
			collector, buffer, sizeof(buffer));

		for (idx = 0; idx < buffer_pos;
			idx += sizeof(struct s_cachepage_info)) {
			struct s_workingset *ws = collector->monitor;

			if (ws) {
				spin_unlock(&collector->lock); /*lint !e455*/
				mutex_lock(&ws->mutex);
				workingset_dealwith_pagecache_wslocked(
				(struct s_cachepage_info *)(buffer + idx), ws);
				mutex_unlock(&ws->mutex);
				spin_lock(&collector->lock);
			} else {
				return;
			}
		}
	}
}
/*lint +e454*/

static int workingset_collector_fill_record_cacheseq_wsrcrdlocked(
	struct s_ws_data *data, unsigned *cacheseq_idx,
	struct s_pagecache_info **pagecache_array,
	unsigned page_idx, unsigned end_in_page)
{
	int ret = 0;
	unsigned idx_in_page;
	unsigned idx = *cacheseq_idx;
	struct s_pagecache_info *pagecache;

	/**
	 * In order to save memory, we save the range including
	 * single page in one word by cleaned range head bit.
	 */
	for (idx_in_page = 0; idx_in_page < end_in_page; idx_in_page++) {
		pagecache = pagecache_array[page_idx] + idx_in_page;
		if (unlikely(pagecache->offset_range.start ==
			PAGECACHE_INVALID_OFFSET))
			continue;

		if (idx < data->pageseq_cnt) {
			data->cacheseq[idx++] = pagecache->offset_range.start;
		} else {
			pr_err("%s: idx=%u, cnt=%u never happend!\n",
				__func__, idx, data->pageseq_cnt);
			ret = -EPERM;
			break;
		}

		if ((pagecache->offset_range.start >> PAGE_RANGE_HEAD_SHIFT) &
			PAGE_RANGE_HEAD_MASK) {
			if (idx < data->pageseq_cnt) {
				data->cacheseq[idx++] =
					pagecache->offset_range.end;
			} else {
				pr_err("%s: idx=%u, cnt=%u never happend!\n",
					__func__, idx, data->pageseq_cnt);
				ret = -EPERM;
				break;
			}
		}
	}

	*cacheseq_idx = idx;
	return ret;
}

static int workingset_collector_fill_record_filenode_wsrcrdlocked(
	struct s_ws_data *data, struct list_head *head)
{
	int ret = 0;
	unsigned i, idx = 0;
	struct list_head *pos;
	struct s_file_info *fileinfo;

	list_for_each(pos, head) {
		fileinfo = container_of(pos, struct s_file_info, list);
		if (idx < data->file_cnt) {
			if (!fileinfo->pageseq_count) {
				kfree(fileinfo->path_node.path);
				ret = memset_s(data->file_array + idx,
					sizeof(struct s_path_node),
					0, sizeof(struct s_path_node));
				if (ret) {
					ws_dbg("%s Line%d,ret=%d\n",
						__func__, __LINE__, ret);
					goto out;
				}
			} else {
				ret = memcpy_s(data->file_array + idx,
					sizeof(struct s_path_node),
					&fileinfo->path_node,
					sizeof(struct s_path_node));
				if (ret) {
					ws_dbg("%s Line%d,ret=%d\n",
						__func__, __LINE__, ret);
					goto out;
				}
			}
		} else {
			kfree(fileinfo->path_node.path);
		}
		/*
		 * the pointer of path is assigned to path of record,
		 * so don't free it in here.
		 */
		fileinfo->path_node.path = NULL;
		idx++;
	}
	return 0;

out:
	for (i = 0; i < idx; i++) {
		kfree(data->file_array[i].path);
		data->file_array[i].path = NULL;
	}
	return ret;
}

static int workingset_collector_prepare_record_space_wsrcrdlocked(
	struct s_workingset *ws, struct s_ws_record *record, bool is_exist)
{
	int ret;
	struct s_ws_data *data = &record->data;
	void *playload;
	unsigned pathnode_size;
	unsigned playload_size;

	pathnode_size = sizeof(struct s_path_node) * ws->file_count;
	playload_size = pathnode_size + sizeof(unsigned) * ws->pageseq_count;
	ret = workingset_prepare_record_space_wsrcrdlocked(&ws->owner,
		record, is_exist, playload_size, &playload);
	if (!ret)
		data->cacheseq = playload + pathnode_size;

	return ret;
}

static int workingset_collector_read_data_wsrcrdlocked(struct s_workingset *ws,
	struct s_ws_record *record, bool is_exist)
{
	int ret;
	struct s_ws_data *data = &record->data;
	unsigned idx;
	unsigned page_idx;
	size_t cpy_size;

	ret = workingset_collector_prepare_record_space_wsrcrdlocked(ws, record,
		is_exist);
	if (ret)
		goto out;

	if (!is_exist) {
		record->owner.uid = ws->owner.uid;
		cpy_size = strlen(ws->owner.name) + 1;
		ret = memcpy_s(record->owner.name, cpy_size,
			ws->owner.name, cpy_size);
		if (ret) {
			ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
			goto fill_data_fail;
		}

		cpy_size = strlen(ws->owner.record_path) + 1;
		ret = memcpy_s(record->owner.record_path, cpy_size,
			ws->owner.record_path, cpy_size);
		if (ret) {
			ws_dbg("%s Line%d,ret=%d\n", __func__, __LINE__, ret);
			goto fill_data_fail;
		}
	}
	data->file_cnt = ws->file_count;
	data->pageseq_cnt = ws->pageseq_count;
	data->page_sum = ws->page_sum;
	ret = workingset_collector_fill_record_filenode_wsrcrdlocked(data,
		&ws->file_list);
	if (ret)
		goto fill_data_fail;

	record->state &= ~E_RECORD_STATE_DATA_FROM_BACKUP;
	idx = 0;
	for (page_idx = 0;
		page_idx < (ws->alloc_index / PAGECACHEINFO_PER_PAGE);
		page_idx++) {
		ret = workingset_collector_fill_record_cacheseq_wsrcrdlocked(
				data, &idx, ws->cache_pages, page_idx,
				PAGECACHEINFO_PER_PAGE);
		if (ret)
			goto fill_data_fail;
	}
	ret = workingset_collector_fill_record_cacheseq_wsrcrdlocked(
				data, &idx, ws->cache_pages, page_idx,
				(ws->alloc_index % PAGECACHEINFO_PER_PAGE));
	if (ret)
		goto fill_data_fail;

	record->state &= ~E_RECORD_STATE_UPDATE_HEADER_ONLY;
	record->state |= E_RECORD_STATE_USED | E_RECORD_STATE_DIRTY;
	return 0;

fill_data_fail:
	record->state &= ~(E_RECORD_STATE_USED | E_RECORD_STATE_DIRTY |
					E_RECORD_STATE_UPDATE_HEADER_ONLY);
out:
	return ret;
}

static void workingset_preread_qos_wsrcrdlocked(struct s_workingset *ws,
	struct s_ws_record *record)
{
	if (ws->leader_blkio_cnt > (record->data.pageseq_cnt *
		BLKIO_PERCENTAGE_THRESHOLD_FOR_UPDATE / 100)) {
		record->need_update = 1;
		if (!(record->state & E_RECORD_STATE_DIRTY))
			record->state |= E_RECORD_STATE_DIRTY |
			E_RECORD_STATE_UPDATE_HEADER_ONLY;

		ws_dbg("%s, blkio=%u, pages = %u\n", __func__,
			ws->leader_blkio_cnt, record->data.pageseq_cnt);
	} else if (!record->leader_blkio_cnt
		&& (record->state & E_RECORD_STATE_UPDATE_BASE_BLKIO)
		&& ws->leader_blkio_cnt) {
		record->leader_blkio_cnt = ws->leader_blkio_cnt;
		if (!(record->state & E_RECORD_STATE_DIRTY))
			record->state |= E_RECORD_STATE_DIRTY |
				E_RECORD_STATE_UPDATE_HEADER_ONLY;

		ws_dbg("%s, preread first blkio count = %u\n", __func__,
			ws->leader_blkio_cnt);
	} else if (record->leader_blkio_cnt
		&& (ws->leader_blkio_cnt >= CARE_BLKIO_MIN_THRESHOLD)
		&& (record->leader_blkio_cnt * 2 < ws->leader_blkio_cnt)) {
		record->need_update = 1;
		if (!(record->state & E_RECORD_STATE_DIRTY))
			record->state |= E_RECORD_STATE_DIRTY |
				E_RECORD_STATE_UPDATE_HEADER_ONLY;

		ws_dbg("%s, base blkio=%u,current blkio=%u\n", __func__,
			record->leader_blkio_cnt, ws->leader_blkio_cnt);
	}
	record->state &= ~E_RECORD_STATE_UPDATE_BASE_BLKIO;
	ws->leader_blkio_cnt = 0;
	ws->leader_blkio_base = 0;
}

static struct s_ws_record *workingset_get_record_wslocked(
	struct s_workingset *ws, bool *is_exist)
{
	struct s_ws_record *record;

	record = workingset_get_existed_record_wslocked(&ws->owner, true);
	if (record) {
		*is_exist = true;
#ifdef CONFIG_TASK_DELAY_ACCT
		/**
		 * check the effect of prereading by comparing the blkio
		 * count on main thread.the empirical blkio used to
		 * deciding recollect pagecache info again.
		 */
		if (!ws->file_count || !ws->pageseq_count) {
			mutex_lock(&record->mutex);
			workingset_preread_qos_wsrcrdlocked(ws, record);
			mutex_unlock(&record->mutex);
			return NULL;
		}
#endif
	} else {
#ifdef CONFIG_TASK_DELAY_ACCT
		if (!ws->file_count || !ws->pageseq_count) {
			pr_warn("%s, busy!state=%s, path/name is null!\n",
				__func__, workingset_state_strs(ws->state));
			return NULL;
		}
#endif
		*is_exist = false;
		record = workingset_get_available_record();
	}

	return record;
}

static void workingset_collector_do_record_locked(struct s_workingset *ws,
	unsigned long discard_count)
{
	struct s_ws_record *record;
	bool is_exist;

	if (!ws)
		return;

	mutex_lock(&ws->mutex);

	ws_dbg("%s: uid=%u, name=%s, state=%s\n", __func__, ws->owner.uid,
		ws->owner.name, workingset_state_strs(ws->state));
	if ((ws->state & E_CGROUP_STATE_MONITOR_STOP) !=
		E_CGROUP_STATE_MONITOR_STOP ||
		!ws->owner.name || !ws->owner.record_path) {
		pr_warn("%s, maybe busy!state=%s, path or name is null!\n",
			__func__, workingset_state_strs(ws->state));
		mutex_unlock(&ws->mutex);
		return;
	}
#ifndef CONFIG_TASK_DELAY_ACCT
	if (!ws->file_count || !ws->pageseq_count) {
		pr_warn("%s, Nothing!file=%u, pageseq=%u, state=%s\n",
			__func__, ws->file_count, ws->pageseq_count,
			workingset_state_strs(ws->state));
		goto out;
	}
#endif

	record = workingset_get_record_wslocked(ws, &is_exist);

	if (record) {
		int ret;

		mutex_lock(&record->mutex);
		record->state &= ~E_RECORD_STATE_UPDATE_BASE_BLKIO;
		record->need_update = 0;
		/*organize the collect data, and save in record*/
		ret = workingset_collector_read_data_wsrcrdlocked(ws,
			record, is_exist);
		if (!ret) {
			if (!is_exist) {
				/**
				 * we'll recollect info for the second times
				 * because there were some permit dialog
				 * in the first time.
				 */
				record->need_update = 1;
				workingset_insert_record_to_list_head(record);
			}
			ws_dbg("%s: %ufls %usqs, pgs=%u,rpt=%lu,dscd=%lu\n",
				__func__, record->data.file_cnt,
				record->data.pageseq_cnt, ws->page_sum,
				ws->repeated_count, discard_count);
		} else if (!is_exist) {
			workingset_insert_record_to_list_tail(record);
		}
		mutex_unlock(&record->mutex);
	}
	workingset_destroy_data(ws, true);

#ifndef CONFIG_TASK_DELAY_ACCT
out:
#endif
	workingset_collector_reset(ws);
	ws->state = E_CGROUP_STATE_MONITOR_OUTOFWORK;
	mutex_unlock(&ws->mutex);
}

static void workingset_collector_do_work(struct s_ws_collector *collector)
{
	enum collector_wait_flags wait_flag;

	spin_lock(&collector->lock);
	wait_flag = collector->wait_flag;
	collector->wait_flag = F_NONE;
	if (wait_flag == F_COLLECT_PENDING) {
		workingset_collector_do_collect_locked(collector);
		spin_unlock(&collector->lock);
	 } else if (wait_flag == F_RECORD_PENDING) {
		struct s_workingset *monitor = collector->monitor;
		unsigned long discard_count = collector->discard_count;

		collector->discard_count = 0;
		spin_unlock(&collector->lock);
		workingset_collector_do_record_locked(monitor, discard_count);
	} else {
		spin_unlock(&collector->lock);
	}
}

/**
 * workingset_page_cache_read -
 * adds requested page to the page cache if not already there.
 * @file:	file to read
 * @offset:	page index
 *
 * This adds the requested page to the page cache if it isn't already there,
 * and schedules an I/O to read in its contents from disk.
 */
static int workingset_page_cache_read(struct s_readpages_control *rpc)
{
	struct file *file = rpc->filp;
	struct address_space *mapping = rpc->mapping;
	struct inode *inode = mapping->host;
	loff_t isize = i_size_read(inode);
	pgoff_t offset = rpc->offset;
	struct page *page;
	int ret;

	if (!isize || (offset > ((isize - 1) >> PAGE_SHIFT)))
		return -EINVAL;

	do {
		page = alloc_pages((mapping_gfp_mask(mapping) & ~__GFP_FS) |
			__GFP_RECLAIM | __GFP_IO | __GFP_COLD, 0);
		if (!page) {
			pr_err("%s: out of memory!\n", __func__);
			return -ENOMEM;
		}

		ret = add_to_page_cache_lru(page, mapping, offset, GFP_NOFS);
		if (ret == 0)
			ret = mapping->a_ops->readpage(file, page);
		else if (ret == -EEXIST)
			ret = 0; /* losing race to add is OK */

#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
		put_page(page);
#else
		page_cache_release(page);
#endif
	} while (ret == AOP_TRUNCATED_PAGE);

	return ret;
}

#ifdef CONFIG_TASK_PROTECT_LRU
static inline struct list_head *get_protect_head_lru(struct lruvec *lruvec,
	struct page *page)
{
	enum lru_list lru = page_lru(page);

	return &lruvec->heads[PROTECT_HEAD_END].protect_page[lru].lru;
}
#endif

/**
 * workingset_adjust_page_lru - move inactive page to head of the lru list.
 * @page:	page to move
 *
 */
static bool workingset_adjust_page_lru(struct page *page)
{
	bool adjusted = false;

	if (!PageUnevictable(page)
#ifdef CONFIG_TASK_PROTECT_LRU
		&& !PageProtect(page)
#endif
		&& !PageActive(page)) {
		if (PageLRU(page)) {
			struct lruvec *lruvec;
			struct zone *zone = page_zone(page);

#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
			spin_lock_irq(zone_lru_lock(zone));
			lruvec = mem_cgroup_page_lruvec(page,
				zone->zone_pgdat);
#else
			spin_lock_irq(&zone->lru_lock);
			lruvec = mem_cgroup_page_lruvec(page, zone);
#endif
#ifdef CONFIG_TASK_PROTECT_LRU
			if (PageLRU(page) && !PageProtect(page) &&
				!PageSwapBacked(page)) {
				struct list_head *head;

				head = get_protect_head_lru(lruvec, page);
				list_move(&page->lru, head);
				adjusted = true;
			}
#else
			if (PageLRU(page) && !PageSwapBacked(page)) {
				list_move(&page->lru,
					&lruvec->lists[page_lru(page)]);
				adjusted = true;
			}
#endif
#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
			spin_unlock_irq(zone_lru_lock(zone));
#else
			spin_unlock_irq(&zone->lru_lock);
#endif
		} else {
			mark_page_accessed(page);
			adjusted = true;
		}
	}
	return adjusted;
}

/**
 * workingset_read_pages - read contiguous filepage from disk.
 */
static int workingset_read_pages(struct address_space *mapping,
		struct file *filp, struct list_head *pages, unsigned nr_pages)
{
	struct blk_plug plug;
	unsigned page_idx;
	int ret;

	blk_start_plug(&plug);

	if (mapping->a_ops->readpages) {
		ret = mapping->a_ops->readpages(filp, mapping,
			pages, nr_pages);
		/* Clean up the remaining pages */
		put_pages_list(pages);
		goto out;
	}

	for (page_idx = 0; page_idx < nr_pages; page_idx++) {
		struct page *page = list_entry((pages)->prev,
			struct page, lru);
		list_del(&page->lru);
		if (!add_to_page_cache_lru(page, mapping, page->index,
			(mapping_gfp_mask(mapping) & ~__GFP_FS)
			| __GFP_COLD | GFP_NOFS))
			mapping->a_ops->readpage(filp, page);
#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
		put_page(page);
#else
		page_cache_release(page);
#endif
	}
	ret = 0;

out:
	blk_finish_plug(&plug);

	return ret;
}

/**
 * workingset_page_cache_range_read - read contiguous filepage.
 */
static int workingset_page_cache_range_read(struct s_readpages_control *rpc)
{
	LIST_HEAD(page_pool);
	unsigned page_idx;
	int ret = 0;
	struct file *filp = rpc->filp;
	struct address_space *mapping = rpc->mapping;
	pgoff_t offset = rpc->offset;
	unsigned long nr_to_read = rpc->nr_to_read;
	struct inode *inode = mapping->host;
	struct page *page;
	unsigned long end_index;	/* The last page we want to read */
	loff_t isize = i_size_read(inode);
	unsigned nr_adj = 0;

	if (isize == 0)
		goto out;

#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
	end_index = ((isize - 1) >> PAGE_SHIFT);
#else
	end_index = ((isize - 1) >> PAGE_CACHE_SHIFT);
#endif

	/*
	 * Preallocate as many pages as we will need.
	 */
	for (page_idx = 0; page_idx < nr_to_read; page_idx++) {
		pgoff_t pageoffset = offset + page_idx;

		if (pageoffset > end_index)
			break;

		page = find_get_page(mapping, pageoffset);
		if (page) {
			if (workingset_adjust_page_lru(page))
				nr_adj++;
			put_page(page);
			continue;
		}

		page = alloc_pages((mapping_gfp_mask(mapping) & ~__GFP_FS) |
			__GFP_RECLAIM | __GFP_IO | __GFP_COLD, 0);
		if (!page) {
			pr_err("%s: out of memory!\n", __func__);
			break;
		}
		page->index = pageoffset;
		list_add(&page->lru, &page_pool);
		ret++;
	}

	/*
	 * Now start the IO.  We ignore I/O errors - if the page is not
	 * uptodate then the caller will launch readpage again, and
	 * will then handle the error.
	 */
	if (ret)
		workingset_read_pages(mapping, filp, &page_pool, ret);

	WARN_ON(!list_empty(&page_pool));

out:
	rpc->nr_adjusted = nr_adj;
	return ret;
}

static void workingset_prereader_open_all_files_rcrdlocked(
	struct s_ws_record *record, struct file **filpp)
{
	int flags = O_RDONLY;
	unsigned file_idx, next_loop_idx;
	struct s_ws_data *data = &record->data;
	const struct cred *mycred = current_cred(); /*lint !e666*/
	uid_t myself_euid, target_euid;

	if (force_o_largefile())
		flags |= O_LARGEFILE;

	next_loop_idx = 0;
	myself_euid = from_kuid_munged(mycred->user_ns, mycred->euid);
	while (next_loop_idx < data->file_cnt) {
		target_euid = data->file_array[next_loop_idx].owner_uid;
		if (target_euid != myself_euid)
			sys_setresuid((gid_t)-1, target_euid, (gid_t)-1);

		for (file_idx = next_loop_idx, next_loop_idx = data->file_cnt;
			file_idx < data->file_cnt; file_idx++) {
			if (!filpp[file_idx] &&
				data->file_array[file_idx].path) {
				if (target_euid !=
					data->file_array[file_idx].owner_uid) {
					if (next_loop_idx == data->file_cnt)
						next_loop_idx = file_idx;
					continue;
				}

				filpp[file_idx] = filp_open(
					data->file_array[file_idx].path,
					flags, 0);
				if (IS_ERR_OR_NULL(filpp[file_idx])) {
					if (!(record->state &
					E_RECORD_STATE_DATA_FROM_BACKUP))
					kfree(data->file_array[file_idx].path);

					data->file_array[file_idx].path = NULL;
					filpp[file_idx] = NULL;
					continue;
				}
			}
		}

		if (target_euid != myself_euid)
			sys_setresuid((gid_t)-1, myself_euid, (gid_t)-1);
	}
}

static struct file **workingset_prereader_alloc_filepp_rcrdlocked(
	struct s_ws_record *record)
{
	unsigned idx;
	struct file **filpp;
	struct page *page;
	struct s_ws_data *data = &record->data;

	if (data->file_cnt > FILPS_PER_PAGE) {
		unsigned need_pages = (data->file_cnt + FILPS_PER_PAGE - 1) /
						FILPS_PER_PAGE;

		for (idx = 0; idx < need_pages; idx++) {
			page = alloc_page(GFP_NOFS | __GFP_ZERO);
			if (!page) {
				pr_err("%s: OOM, alloc %u pages failed!\n",
					__func__, need_pages);
				break;
			}
			record->filp_pages[idx] = page;
		}

		if (idx >= need_pages)
			filpp = (struct file **)vmap(record->filp_pages,
				need_pages, VM_MAP, PAGE_KERNEL);
		else
			filpp = NULL;

		if (!filpp) {
			for (idx = 0; idx < need_pages; idx++) {
				if (record->filp_pages[idx]) {
					__free_page(record->filp_pages[idx]);
					record->filp_pages[idx] = NULL;
				}
			}
		}
	} else {
		filpp = (struct file **)get_zeroed_page(GFP_NOFS);
	}

	return filpp;
}

static bool workingset_filepage_is_need_skip_read(struct s_ws_data *data,
	unsigned idx, unsigned stage, bool preread_major, struct file **filpp,
	unsigned *pfile_idx, unsigned *stage_end)
{
	bool ret = false;
	bool is_major_page;
	unsigned file_idx;
	unsigned stage_num;

	if (preread_major) {
		stage_num = (data->cacheseq[idx] >> PAGE_STAGE_NUM_SHIFT)
			& PAGE_STAGE_NUM_MASK;
		if (stage < stage_num) {
			*stage_end = idx;
			ret = true;
			goto out;
		}
	}

	file_idx = (data->cacheseq[idx] >> FILE_OFFSET_BITS) &
		MAX_TOUCHED_FILES_COUNT;
	if ((file_idx >= data->file_cnt) || !filpp[file_idx]) {
		ret = true;
		goto out;
	}

	is_major_page = data->cacheseq[idx] &
		(PAGE_MAJOR_MASK << PAGE_MAJOR_SHIFT);
	if ((preread_major && !is_major_page)
		|| (!preread_major && is_major_page)) {
		ret = true;
		goto out;
	}
	*pfile_idx = file_idx;
out:
	return ret;
}

static void workingset_read_filepages_rcrdlocked(struct s_ws_record *record,
	struct file **filpp)
{
	int flags = O_RDONLY;
	struct s_readpages_control rpc;
	struct s_ws_data *data = &record->data;
	struct page *page;
	unsigned idx, file_idx;
	unsigned stage = 0;
	unsigned stage_begin = 0;
	unsigned stage_end = data->pageseq_cnt;
	bool preread_major_page = true;
	unsigned range_end;
	unsigned present_pages_cnt = 0;
	unsigned read_count;
	unsigned read_pages_cnt = 0;
	unsigned move_lru_cnt = 0;

	/*
	 * in some case, io request is congested, so we must be ensure
	 * read file page of main thread touched first.
	 */
	if (force_o_largefile())
		flags |= O_LARGEFILE;
try_next:
	for (idx = stage_begin; idx < stage_end; idx++) {
		if (!(idx%100) && atomic_read(&g_preread_abort))
			return;
		if (workingset_filepage_is_need_skip_read(data, idx, stage,
			preread_major_page, filpp, &file_idx, &stage_end))
			continue;

		/*find file page in page cache.*/
		rpc.filp = filpp[file_idx];
		rpc.mapping = filpp[file_idx]->f_mapping;
		rpc.offset = data->cacheseq[idx] & MAX_TOUCHED_FILE_OFFSET;
		if ((data->cacheseq[idx] >> PAGE_RANGE_HEAD_SHIFT) &
			PAGE_RANGE_HEAD_MASK) {
			/*in the case, prereading multi file pages*/
			range_end = data->cacheseq[++idx] &
						MAX_TOUCHED_FILE_OFFSET;
			rpc.nr_to_read = range_end - rpc.offset;
			read_count = workingset_page_cache_range_read(&rpc);
			present_pages_cnt += rpc.nr_to_read - read_count;
			read_pages_cnt += read_count;
			move_lru_cnt += rpc.nr_adjusted;
		} else {
			/*in the case, prereading single file page*/
			page = find_get_page(rpc.mapping, rpc.offset);
			if (page) {
				if (workingset_adjust_page_lru(page))
					move_lru_cnt++;
				put_page(page);
				present_pages_cnt++;
			} else if (!workingset_page_cache_read(&rpc)) {
				read_pages_cnt++;
			}
		}
	}

	if (preread_major_page) {
		preread_major_page = false;
		goto try_next;
	} else if (stage_end < data->pageseq_cnt) {
		ws_dbg("%s %s, stage %u prsnt %u,mv %u,rd %u\n",
		__func__, record->owner.name, stage, present_pages_cnt,
		move_lru_cnt, read_pages_cnt);
		preread_major_page = true;
		stage_begin = stage_end;
		stage_end = data->pageseq_cnt;
		stage++;
		goto try_next;
	}
	/*
	 * when many file pages are not present, the blkio count of main
	 * thread can be consider as the base blkio of prereading.
	 */
	if (present_pages_cnt <
		((data->page_sum * CACHE_MISSED_PERCENTAGE_THRESHOLD_FOR_BLKIO)
		/ 100))
		record->state |= E_RECORD_STATE_UPDATE_BASE_BLKIO;

	ws_dbg("%s %s,%u fls,%u sqs,%u pgs,prsnt %u,mv %u,rd %u\n",
		__func__, record->owner.name, data->file_cnt,
		data->pageseq_cnt, data->page_sum, present_pages_cnt,
		move_lru_cnt, read_pages_cnt);
}

#ifdef CONFIG_HW_CGROUP_WS_DEBUG
static void workingset_page_readonly(const void *vmalloc_addr, bool enable)
{
	unsigned long addr = (unsigned long) vmalloc_addr;
	pgd_t *pgd = pgd_offset_k(addr);

	BUG_ON(!is_vmalloc_or_module_addr(vmalloc_addr));
	if (!pgd_none(*pgd)) {
		pud_t *pud = pud_offset(pgd, addr);

		BUG_ON(pud_bad(*pud));
		if (!pud_none(*pud) && !pud_bad(*pud)) {
			pmd_t *pmd = pmd_offset(pud, addr);

			BUG_ON(pmd_bad(*pmd));
			if (!pmd_none(*pmd) && !pmd_bad(*pmd)) {
				pte_t *ptep, pte;

				ptep = pte_offset_map(pmd, addr);
				pte = *ptep;
				ws_dbg("%s %p = %llx\n",
					__func__,
					vmalloc_addr,
					pte_val(*ptep));
				if (!enable) {
				pte = clear_pte_bit(pte,
				__pgprot(PTE_RDONLY));
				pte = set_pte_bit(pte,
				__pgprot(PTE_WRITE));
				set_pte(ptep, pte);
				} else {
				pte = clear_pte_bit(pte,
				__pgprot(PTE_WRITE));
				pte = set_pte_bit(pte,
				__pgprot(PTE_RDONLY));
				set_pte(ptep, pte);
				}
				pte_unmap(ptep);
				flush_tlb_kernel_range(addr,
					addr + PAGE_SIZE);
			}
		}
	}
}
#endif

static void workingset_do_preread_work_rcrdlocked(struct s_ws_record *record)
{
	struct file **filpp;
	struct s_ws_data *data = &record->data;
#ifdef CONFIG_HW_CGROUP_WS_DEBUG
	unsigned preread_file_cnt = data->file_cnt;
#endif

	if (!data->file_cnt || !data->pageseq_cnt ||
		!data->file_array || !data->cacheseq)
		return;

	if (atomic_read(&g_preread_abort))
		return;

	/*alloc pages for save opened struct files*/
	filpp = workingset_prereader_alloc_filepp_rcrdlocked(record);
	if (!filpp)
		return;

	workingset_prereader_open_all_files_rcrdlocked(record, filpp);
#ifdef CONFIG_HW_CGROUP_WS_DEBUG
	if (data->file_cnt > FILPS_PER_PAGE) {
		int idx;

		for (idx = 0; idx < FILP_PAGES_COUNT; idx++)/*lint !e574*/
			workingset_page_readonly(
			filpp + (FILPS_PER_PAGE * idx), true);
	}
#endif
	workingset_read_filepages_rcrdlocked(record, filpp);
#ifdef CONFIG_HW_CGROUP_WS_DEBUG
	if (data->file_cnt != preread_file_cnt) {
		pr_err("%s, a bad file_cnt=%u, pread_cnt=%u\n",
			__func__, data->file_cnt, preread_file_cnt);
		BUG();
	}
#endif
	workingset_preread_post_work_rcrdlocked(record, filpp);
}

static void workingset_preread_post_work_rcrdlocked(
	struct s_ws_record *record, struct file **filpp)
{
	unsigned file_idx, idx;
	struct s_ws_data *data;

	data = &record->data;
	if (filpp) {
		for (file_idx = 0; file_idx < data->file_cnt; file_idx++) {
			if (filpp[file_idx])
				filp_close(filpp[file_idx], NULL);
		}
#ifdef CONFIG_HW_CGROUP_WS_DEBUG
		if (data->file_cnt > FILPS_PER_PAGE) {
			for (idx = 0; idx < FILP_PAGES_COUNT; idx++)
				workingset_page_readonly(
				filpp + (FILPS_PER_PAGE * idx),
				false);
		}
#endif
	}

	for (idx = 0; idx < FILP_PAGES_COUNT; idx++) {
		if (record->filp_pages[idx]) {
			if (filpp) {
				vunmap(filpp);
				filpp = NULL;
			}
			__free_page(record->filp_pages[idx]);
			record->filp_pages[idx] = NULL;
		}
	}

	if (filpp) {
		free_page((unsigned long)filpp);
		filpp = NULL;
	}
}

static int workingset_collect_kworkthread(void *p)
{
	int ret;
	struct s_ws_collector *collector;

	if (!p) {
		pr_err("%s: p is NULL!\n", __func__);
		return 0;
	}

	collector = (struct s_ws_collector *)p;
	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(
			collector->collect_wait,
			((collector->wait_flag == F_COLLECT_PENDING)
			|| (collector->wait_flag == F_RECORD_PENDING)));
		if (ret < 0)
			continue;

		workingset_collector_do_work(collector);
	}
	pr_err("%s: exit!\n", __func__);

	return 0;
}

static int workingset_cachepage_info_init(struct file *file,
	pgoff_t start_offset, unsigned count, unsigned pid,
	struct s_cachepage_info *info)
{
	if (unlikely(!file) ||
		unlikely(start_offset > MAX_TOUCHED_FILE_OFFSET))
		return -EINVAL;

	if (unlikely(start_offset + count - 1 > MAX_TOUCHED_FILE_OFFSET))
		count = MAX_TOUCHED_FILE_OFFSET - start_offset + 1;

	info->filp = file;
	info->start_offset = (unsigned)start_offset;
	info->count = count;
	info->pid = pid;
	return 0;
}

void workingset_pagecache_record(struct file *file, pgoff_t start_offset,
	unsigned count, bool is_pagefault)
{
	struct s_cachepage_info info, *target_space;
	struct task_struct *task = current;
	unsigned remain_space;
	bool mmap_only;

	if (workingset_cachepage_info_init(file, start_offset,
		count, task->pid, &info))
		return;

	spin_lock(&g_collector->lock);
	if (!g_collector->monitor) {
		spin_unlock(&g_collector->lock);
		return;
	}

	mmap_only =
		(g_collector->monitor->state == E_CGROUP_STATE_MONITOR_PAUSED);
	if ((!is_pagefault && mmap_only)
		|| (task->tgid != g_collector->monitor->owner.pid)) {
		spin_unlock(&g_collector->lock);
		return;
	}

	info.stage = g_collector->monitor->stage_num;
	if (g_collector->read_pos <= g_collector->write_pos)
		remain_space = COLLECTOR_CACHE_SIZE - g_collector->write_pos +
					g_collector->read_pos;
	else
		remain_space = g_collector->read_pos - g_collector->write_pos;

	/*
	 * when the circle buffer is almost full, we collect touched
	 * file page of main thread only.
	 */
	if (remain_space < COLLECTOR_REMAIN_CACHE_LOW_WATER) {
		if ((task->pid != task->tgid) ||
			(remain_space <= sizeof(struct s_cachepage_info))) {
			g_collector->discard_count++;
			spin_unlock(&g_collector->lock);
			return;
		}
	}

	target_space = (struct s_cachepage_info *)(g_collector->circle_buffer +
		g_collector->write_pos);
	*target_space = info;
	if (g_collector->write_pos + sizeof(struct s_cachepage_info) ==
		COLLECTOR_CACHE_SIZE)
		g_collector->write_pos = 0;
	else
		g_collector->write_pos +=  sizeof(struct s_cachepage_info);
	atomic_long_inc(&file->f_count);
	spin_unlock(&g_collector->lock);

	/*notify the collect thread pageinfos comming*/
	if (waitqueue_active(&g_collector->collect_wait)) {
		g_collector->wait_flag = F_COLLECT_PENDING;
		wake_up_interruptible_all(&g_collector->collect_wait);
	}
}

static int __init cgroup_workingset_init(void)
{
	int ret = 0;
	struct page *page;

	if (COLLECTOR_CACHE_SIZE % sizeof(struct s_cachepage_info)) {
		pr_err("%s, size of cache is not aligned with %lu\n",
			__func__, sizeof(struct s_cachepage_info));
		ret = -EINVAL;
		goto out;
	}

	g_tfm = crypto_alloc_shash("crc32c", 0, 0);
	if (PTR_ERR_OR_ZERO(g_tfm)) {
		pr_err("%s, alloc crc32c cypto failed\n", __func__);
		goto out;
	}

	g_collector = kzalloc(sizeof(struct s_ws_collector), GFP_KERNEL);
	if (!g_collector)
		goto create_collector_fail;

#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
	page = alloc_pages_node(NUMA_NO_NODE, GFP_KERNEL,
						  COLLECTOR_CACHE_SIZE_ORDER);
#else
	page = alloc_kmem_pages_node(NUMA_NO_NODE, GFP_KERNEL,
						  COLLECTOR_CACHE_SIZE_ORDER);
#endif
	if (!page) {
		pr_err("%s, collector cache alloc failed!\n", __func__);
		goto create_collector_cache_fail;
	}
	g_collector->circle_buffer = page_address(page);

	spin_lock_init(&g_collector->lock);
	init_waitqueue_head(&g_collector->collect_wait);

	g_collector->collector_thread = kthread_run(
			workingset_collect_kworkthread,
			g_collector, "workingset:collector");
	if (IS_ERR(g_collector->collector_thread)) {
		ret = PTR_ERR(g_collector->collector_thread);
		pr_err("%s: create the collector thread failed!\n", __func__);
		goto create_collector_thread_fail;
	}

	spin_lock_init(&g_record_list_lock);

	if (totalram_pages > 3 * TOTAL_RAM_PAGES_1G)
		g_max_records_count = MAX_RECORD_COUNT_ON_4G;
	else if (totalram_pages > 2 * TOTAL_RAM_PAGES_1G)
		g_max_records_count = MAX_RECORD_COUNT_ON_3G;
	else if (totalram_pages > TOTAL_RAM_PAGES_1G)
		g_max_records_count = MAX_RECORD_COUNT_ON_2G;
	else
		g_max_records_count = MAX_RECORD_COUNT_ON_1G;
	ws_dbg("%s, totalram %lu pages, max count of records = %u\n",
		__func__, totalram_pages, g_max_records_count);
	g_module_initialized = true;
	return 0;

create_collector_thread_fail:
#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
	__free_pages(virt_to_page(g_collector->circle_buffer),
					COLLECTOR_CACHE_SIZE_ORDER);
#else
	free_kmem_pages((unsigned long)g_collector->circle_buffer,
					COLLECTOR_CACHE_SIZE_ORDER);
#endif
	g_collector->circle_buffer = NULL;
create_collector_cache_fail:
	kfree(g_collector);
	g_collector = NULL;
create_collector_fail:
	crypto_free_shash(g_tfm);
	g_tfm = NULL;
out:
	return ret;
}

static void __exit cgroup_workingset_exit(void)
{
	kthread_stop(g_collector->collector_thread);
	g_collector->collector_thread = NULL;
#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
	__free_pages(virt_to_page(g_collector->circle_buffer),
					COLLECTOR_CACHE_SIZE_ORDER);
#else
	free_kmem_pages((unsigned long)g_collector->circle_buffer,
					COLLECTOR_CACHE_SIZE_ORDER);
#endif
	g_collector->circle_buffer = NULL;

	kfree(g_collector);
	crypto_free_shash(g_tfm);
	g_collector = NULL;
	g_tfm = NULL;
}

late_initcall(cgroup_workingset_init);
module_exit(cgroup_workingset_exit);

