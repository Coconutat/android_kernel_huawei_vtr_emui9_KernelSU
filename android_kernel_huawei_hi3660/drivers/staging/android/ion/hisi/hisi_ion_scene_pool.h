/*
 * Copyright (C) 2016 Hislicon, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef _ION_SCENE_POOL_H
#define _ION_SCENE_POOL_H

#include <linux/kthread.h>
#include <linux/types.h>
#include <linux/printk.h>
#include <linux/hisi/hisi_ion.h>
#include "ion_priv.h"
#ifdef ION_SCENE_POOL_ALLOC_DEBUG
#include <linux/time.h>
#endif

#define SCENE_POOL_ALLOC_MIN_SIZE			SZ_1M
#define MAX_SPECIAL_SCENE_POOL_SIZE		(480 * 64 * 4)
#define MAX_SPECIAL_SCENE_TIMEOUT		(5)
#define SPECIAL_SCENE_ORDER_SHIFT			(3)

#define SPECIAL_SCENE_FILL_POOL_WORKER			(0x01)
#define SPECIAL_SCENE_SHRINK_MEMORY_WORKER		(0x02)
#define SPECIAL_SCENE_ALL_WORKERS_MASK			(0xFF)
#define SPECIAL_SCENE_WORKER_CPU_SHIFT			(16)
#define SPECIAL_SCENE_WORKER_CPU_MASK			(0xFF)
#define SPECIAL_SCENE_WORKER_PRIORITY_SHIFT		(8)
#define SPECIAL_SCENE_WORKER_PRIORITY_MASK		(0xFF)
#define ABS(X) ((X) < 0 ? (-1 * (X)) : (X))

enum scene_pool_wait_flags {
	F_NONE,
	F_WAKE_UP,
	F_WAKEUP_AUTOFREE,
	F_FORCE_STOP,
};

struct delayed_work_wrapper {
	struct delayed_work delaywork;
	unsigned long pool;
};

struct alloc_context_alias {
	struct zonelist *zonelist;
	nodemask_t *nodemask;
	struct zone *preferred_zone;
	int classzone_idx;
	int migratetype;
	enum zone_type high_zoneidx;
};

struct ion_scene_pool {
	int max_pool_size;
	unsigned worker_mask;
	atomic_t total_watermark;
	atomic_t scene_pool_can_fill_flag;
	enum scene_pool_wait_flags pool_wait_flag;
	enum scene_pool_wait_flags shrink_wait_flag;
	unsigned long start_jiffies;
	int timeout_hz;
	bool in_special_scene;
	wait_queue_head_t pool_wait;
	struct task_struct *pool_thread;
	struct task_struct *shrink_thread;
	struct delayed_work_wrapper autofree_delayed_work;
	struct alloc_context_alias ac;
	struct ion_page_pool *pools[0];
};

static inline bool ion_scene_pool_can_allocate(struct ion_scene_pool *pool,
					       unsigned long size)
{
	bool ret	= true;
#if SCENE_POOL_ALLOC_MIN_SIZE > 0
	if (size < SCENE_POOL_ALLOC_MIN_SIZE)
		ret = false;
	else
#endif
	if (!pool)
		ret =  false;

	return ret;
}

static inline void scene_pool_fill_pause(struct ion_scene_pool *pool)
{
	if (pool)
		atomic_set(&pool->scene_pool_can_fill_flag, 0);
}

static inline void scene_pool_fill_resume(struct ion_scene_pool *pool)
{
	if (pool) {
		atomic_set(&pool->scene_pool_can_fill_flag, 1);
		wake_up_interruptible_all(&pool->pool_wait);
	}
}

struct ion_scene_pool *ion_scene_pool_create(void);
void ion_scene_pool_debug_show_total(struct seq_file *s,
				     struct ion_scene_pool *scene_pool);
int ion_scene_pool_shrink(struct ion_scene_pool *scene_pool,
			  struct ion_page_pool *pool, gfp_t gfp_mask,
			  int nr_to_scan);
void ion_scene_pool_destroy(struct ion_scene_pool *pool);
void ion_scene_update_watermark(void *p, int delta);
void ion_scene_pool_wakeup_process(
		void *p, enum scene_pool_wait_flags flag,
		struct ion_special_scene_pool_info_data *scene_pool_info);
unsigned long ion_scene_pool_allocate_pages(struct ion_scene_pool *pool,
					    unsigned long size, int *count,
					    struct list_head *page_list);

#ifdef ION_SCENE_POOL_ALLOC_DEBUG
#define ION_SCENE_POOL_TRY_ALLOC_ENTER(in_pool,		\
					in_size,	\
					remained,	\
					out_pool,	\
					out_index,	\
					out_pagelist,	\
					out_allocate,	\
					begin)		\
{	\
	if (in_pool && in_pool->in_special_scene) {	\
		do_gettimeofday(&begin);	\
		if (ion_scene_pool_can_allocate(in_pool, remained)) {	\
			if (remained > 0) {	\
				out_pool = in_pool;	\
				scene_pool_fill_pause(out_pool);	\
				out_allocate = ion_scene_pool_allocate_pages(\
							out_pool,	\
							remained,	\
							&out_index,	\
							&out_pagelist);	\
				remained -= out_allocate;	\
			}	\
			ion_scene_update_watermark(in_pool,	\
				0 - (in_size >> PAGE_SHIFT));	\
		}	\
	}	\
}

#define ION_SCENE_POOL_TRY_ALLOC_EXIT(in_pool,		\
					size,		\
					out_pool,	\
					allocated,	\
					begin,		\
					end)		\
{	\
	if (in_pool && in_pool->in_special_scene) {	\
		long delta;	\
\
		do_gettimeofday(&end);	\
		delta = (end.tv_sec - begin.tv_sec) * 1000000	\
			+ (end.tv_usec - begin.tv_usec);	\
		if (delta >= 1000)		\
			pr_info("%s, special scene alloc %d[%d] pages"	\
				" consume %ld us\n",			\
				__func__,				\
				ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT,	\
				allocated >> PAGE_SHIFT,		\
				delta);					\
	}	\
	scene_pool_fill_resume(out_pool);	\
}
#else
#define ION_SCENE_POOL_TRY_ALLOC_ENTER(in_pool,		\
					in_size,	\
					remained,	\
					out_pool,	\
					out_index,	\
					out_pagelist)	\
{	\
	if (in_pool && in_pool->in_special_scene) {	\
		if (ion_scene_pool_can_allocate(in_pool, remained)) {	\
			if (remained > 0) {	\
				out_pool = in_pool;	\
				scene_pool_fill_pause(out_pool);	\
				remained -= ion_scene_pool_allocate_pages(\
						out_pool,	\
						remained,	\
						&out_index,	\
						&out_pagelist);	\
			}	\
			ion_scene_update_watermark(in_pool,	\
				0 - (in_size >> PAGE_SHIFT));	\
		}	\
	}	\
}

#define ION_SCENE_POOL_TRY_ALLOC_EXIT(out_pool)	\
	scene_pool_fill_resume(out_pool)
#endif
#endif /* _ION_SCENE_POOL_H */
