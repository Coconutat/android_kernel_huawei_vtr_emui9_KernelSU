
/*
 *
 * Copyright (C) 2017 hisilicon, Inc.
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

#define pr_fmt(fmt) "scenepool: " fmt

#include <asm/page.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/highmem.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/hisi/ion-iommu.h>
#include <linux/sizes.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/swap.h>
#include <linux/cpuset.h>
#include <linux/hisi/hisi_ion.h>
#ifdef CONFIG_VM_EVENT_COUNTERS
#include <linux/vmstat.h>
#endif
#include <linux/version.h>

#include "../ion.h"
#include "hisi_ion_scene_pool.h"

/* The ALLOC_WMARK bits are used as an index to zone->watermark */
#define ALLOC_WMARK_MIN		WMARK_MIN
#define ALLOC_WMARK_LOW		WMARK_LOW
#define ALLOC_WMARK_HIGH	WMARK_HIGH
#define ALLOC_NO_WATERMARKS	0x04 /* don't check watermarks at all */

/* Mask to get the watermark bits */
#define ALLOC_WMARK_MASK	(ALLOC_NO_WATERMARKS - 1)

#define ALLOC_HARDER		0x10 /* try to alloc harder */
#define ALLOC_HIGH		0x20 /* __GFP_HIGH set */
#define ALLOC_CPUSET		0x40 /* check for correct cpuset */
#define ALLOC_CMA		0x80 /* allow allocations from CMA areas */
#define ALLOC_FAIR		0x100 /* fair zone allocation */
#ifdef CONFIG_HUAWEI_UNMOVABLE_ISOLATE
#define ALLOC_UNMOVABLE		0x800 /* migratetype is MIGRATE_UNMOVABLE */
#endif

#define SCENE_POOL_MIN(x, y) (((x) < (y)) ? (x) : (y))

#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
#define __for_each_zone_zonelist_nodemask(zone, z, zlist, highidx, nodemask) \
        for (z = first_zones_zonelist(zlist, highidx, nodemask), zone = zonelist_zone(z);       \
                zone;                                                   \
                z = next_zones_zonelist(++z, highidx, nodemask),        \
                        zone = zonelist_zone(z))
#else
#define __for_each_zone_zonelist_nodemask(zone, z, zlist, highidx, nodemask) \
	for (z = first_zones_zonelist(zlist, highidx, nodemask, &zone);	\
	     zone;							\
	     z++,							\
	     z = next_zones_zonelist(z, highidx, nodemask),		\
	     zone = zonelist_zone(z))
#endif

static unsigned int scene_pool_orders[] = {8, 4, 0};

static const int scene_pool_num_orders = ARRAY_SIZE(scene_pool_orders);

static void ion_scene_pool_all_free(struct ion_scene_pool *pool,
				    gfp_t gfp_mask, int nr_to_scan);

static void ion_scene_pool_autofree_workfn(struct work_struct *work)
{
	struct delayed_work *delayworker =
			container_of(work, struct delayed_work, work);
	struct delayed_work_wrapper *delay_worker_wrapper =
				(struct delayed_work_wrapper *)delayworker;
	struct ion_scene_pool *pool =
			(struct ion_scene_pool *)delay_worker_wrapper->pool;
	int page_cnt = 0, i;

	if (!pool)
		return;
	if (time_after((pool->start_jiffies + pool->timeout_hz), jiffies)) {
		queue_delayed_work(system_wq,
			(struct delayed_work *)(&pool->autofree_delayed_work),
			(pool->start_jiffies + pool->timeout_hz - jiffies));
		pr_info("There is a restarting event before timeout!\n");
		return;
	}

	for (i = 0; i < scene_pool_num_orders; i++)
		page_cnt += ion_page_pool_shrink(pool->pools[i],
						__GFP_HIGHMEM, 0);
	if (!pool->in_special_scene) {
		pr_info("It has freed special scene pool before timeout!\n");
		return;
	}
	atomic_set(&pool->total_watermark, 0);
	pool->in_special_scene = false;
	ion_scene_pool_all_free(pool, __GFP_HIGHMEM, pool->max_pool_size);
	pr_info("AutoStop timeout! remained %d pages\n", page_cnt);
}

void ion_scene_update_watermark(void *p, int delta)
{
	struct ion_scene_pool *pool;
	int global_water_mark;

	if (!p) {
		pr_err("%s: p is NULL!\n", __func__);
		return;
	}

	pool = (struct ion_scene_pool *)p;
	global_water_mark = atomic_read(&pool->total_watermark) + delta;
	atomic_set(&pool->total_watermark,
		   global_water_mark > 0 ? global_water_mark : 0);
}

static inline int ion_scene_pool_pages(struct ion_page_pool *pool)
{
	int count;

	count = pool->low_count + pool->high_count;

	return count << pool->order;
}

static inline int ion_scene_pool_total_pages(struct ion_scene_pool *pool)
{
	int i;
	int count = 0;

	for (i = 0; i < scene_pool_num_orders; i++)
		count += ion_scene_pool_pages(pool->pools[i]);

	return count;
}

unsigned long ion_scene_pool_total_size(void)
{
	unsigned long size = 0;
	void *pool = ion_get_scene_pool(ion_get_system_heap());

	if (pool) {
		size = (unsigned)ion_scene_pool_total_pages(pool);
		size <<= PAGE_SHIFT;
	}
	return size;
}

static void ion_page_scene_pool_add(struct ion_page_pool *pool,
				    struct page *page)
{
	mutex_lock(&pool->mutex);
	if (pool->high_count < pool->low_count) {
		list_add_tail(&page->lru, &pool->high_items);
		pool->high_count++;
	} else {
		list_add_tail(&page->lru, &pool->low_items);
		pool->low_count++;
	}
	mutex_unlock(&pool->mutex);
}

/*
** this function should be called only in case of the count of items
** is larger than to_alloc.
*/
static inline void ion_scene_pool_remove_pages(struct list_head *head,
					       unsigned long count,
					       unsigned long to_alloc,
					       struct list_head *page_list)
{
	struct list_head *pos;
	struct list_head temp_list;

	WARN_ON(count <= to_alloc);
	if ((count >> 1) < to_alloc) {
		to_alloc = count - to_alloc;
		list_for_each_prev(pos, head)
			if (to_alloc-- == 0)
				break;
	} else
		list_for_each(pos, head)
			if (--to_alloc == 0)
				break;

	list_cut_position(&temp_list, head, pos);
	list_splice(&temp_list, page_list);
}

static inline unsigned long ion_scene_pool_alloc_pages(
				struct ion_page_pool *pool,
				unsigned long to_alloc,
				struct list_head *page_list)
{
	unsigned long nr_alloced = 0;

	if (!pool || !page_list)
		return 0;

	mutex_lock(&pool->mutex);
	if (pool->high_count > 0 &&
	    (unsigned)pool->high_count <= to_alloc) {
		list_splice_init(&pool->high_items, page_list);
		to_alloc -= pool->high_count;
		nr_alloced += pool->high_count;
		pool->high_count = 0;
	}
	if (pool->low_count > 0 &&
	    (unsigned)pool->low_count <= to_alloc) {
		list_splice_init(&pool->low_items, page_list);
		to_alloc -= pool->low_count;
		nr_alloced += pool->low_count;
		pool->low_count = 0;
	}

	if (to_alloc) {
		if (pool->high_count > 0 &&
		    (unsigned)pool->high_count > to_alloc) {
			ion_scene_pool_remove_pages(
						&pool->high_items,
						pool->high_count,
						to_alloc, page_list);
			pool->high_count -= to_alloc;
			nr_alloced += to_alloc;
		} else if (pool->low_count > 0 &&
			   (unsigned)pool->low_count > to_alloc) {
			ion_scene_pool_remove_pages(
						&pool->low_items,
						pool->low_count,
						to_alloc, page_list);
			pool->low_count -= to_alloc;
			nr_alloced += to_alloc;
		}
	}
	mutex_unlock(&pool->mutex);

	return nr_alloced;
}

unsigned long ion_scene_pool_allocate_pages(struct ion_scene_pool *pool,
					    unsigned long size, int *count,
					    struct list_head *page_list)
{
	int i;
	unsigned long nr_alloced, nr_order_alloced;
	unsigned long nr_remained, order_size;

	if (!pool) {
		pr_err("%s: pool is NULL!\n", __func__);
		return 0;
	}
	if (ion_scene_pool_total_pages(pool) <= 0)
		return 0;
	nr_remained = size >> PAGE_SHIFT;
	for (i = 0; i < scene_pool_num_orders && nr_remained; i++) {
		order_size = 1UL << scene_pool_orders[i];
		if (nr_remained < order_size)
			continue;
		nr_order_alloced = ion_scene_pool_alloc_pages(
						pool->pools[i],
						nr_remained / order_size,
						page_list);
		nr_alloced = nr_order_alloced << scene_pool_orders[i];
		*count += nr_order_alloced;
		nr_remained -= nr_alloced;
	}

	return size - (nr_remained << PAGE_SHIFT);
}

static int ion_scene_pool_alloc_context_setup(gfp_t gfp_mask,
					      struct zonelist *zonelist,
					      struct alloc_context_alias *ac)
{
	struct zoneref *preferred_zoneref;

	if (unlikely(!zonelist->_zonerefs->zone))
		return -1;

	ac->high_zoneidx = gfp_zone(gfp_mask),
	ac->nodemask = NULL,
	ac->migratetype = gfpflags_to_migratetype(gfp_mask),
	ac->zonelist = zonelist;
	/* The preferred zone is used for statistics later */
#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
	if (cpusets_enabled())
		ac->nodemask = &cpuset_current_mems_allowed;

	preferred_zoneref = first_zones_zonelist(ac->zonelist,
					ac->high_zoneidx, ac->nodemask);
#else
	preferred_zoneref = first_zones_zonelist(
				ac->zonelist,
				ac->high_zoneidx,
				ac->nodemask ? : &cpuset_current_mems_allowed,
				&ac->preferred_zone);
#endif
	if (!preferred_zoneref)
		return -1;
	ac->classzone_idx = zonelist_zone_idx(preferred_zoneref);

	return 0;
}

static int shrink_memory(gfp_t gfp_mask, unsigned int order,
			 const struct alloc_context_alias *ac)
{
	struct reclaim_state reclaim_state;
	int progress;

	/* We now go into synchronous reclaim */
	cpuset_memory_pressure_bump();
	current->flags |= PF_MEMALLOC;
	lockdep_set_current_reclaim_state(gfp_mask);
	reclaim_state.reclaimed_slab = 0;
	current->reclaim_state = &reclaim_state;

	progress = try_to_free_pages(ac->zonelist,
				     order, gfp_mask,
				     ac->nodemask);
#ifdef CONFIG_VM_EVENT_COUNTERS
	/* Because we call direct reclaim to shrink memory asynchronousy, */
	/* so decrease the value in case disturb the counter of allocstall */
#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
{
	unsigned item = ALLOCSTALL_NORMAL - ZONE_NORMAL
		+ gfp_zone(memalloc_noio_flags(gfp_mask));
	if (raw_cpu_read(vm_event_states.event[item]))
		raw_cpu_dec(vm_event_states.event[item]);
}
#else
	if (this_cpu_read(vm_event_states.event[ALLOCSTALL]))
		this_cpu_dec(vm_event_states.event[ALLOCSTALL]);
#endif
#endif
	current->reclaim_state = NULL;
	lockdep_clear_current_reclaim_state();
	current->flags &= ~PF_MEMALLOC;

	return progress;
}

static struct page *ion_scene_pool_alloc_page(
					gfp_t gfp_mask,
					unsigned int order,
					const struct alloc_context_alias *ac)
{
	int alloc_flags = ALLOC_WMARK_HIGH | ALLOC_CPUSET;
	/* The gfp_t that was actually used for allocation */
	gfp_t alloc_mask = gfp_mask | __GFP_HARDWALL;
	struct zonelist *zonelist = ac->zonelist;
	struct zoneref *z = NULL;
	struct page *page = NULL;
	struct zone *zone = NULL;
	bool have_zone_ok = false;

#ifdef CONFIG_HUAWEI_UNMOVABLE_ISOLATE
	if (ac->migratetype == MIGRATE_UNMOVABLE)
		alloc_flags |= ALLOC_UNMOVABLE;
#endif
	__for_each_zone_zonelist_nodemask(zone, z, zonelist,
					  ac->high_zoneidx,
					  ac->nodemask) { /*lint !e564 */
		if (cpusets_enabled() &&
		    (alloc_flags & ALLOC_CPUSET) &&
		    !cpuset_zone_allowed(zone, alloc_mask))
			continue;

		if (zone_watermark_ok(
			zone, order,
			zone->watermark[alloc_flags & ALLOC_WMARK_MASK],
			ac->classzone_idx, alloc_flags)) {
			have_zone_ok = true;
			break;
		}
	}

	if (have_zone_ok) {
		page = alloc_pages(gfp_mask, order);
	} else if (!order) {
		cond_resched();
		shrink_memory(gfp_mask, order, ac);
		cond_resched();
	}

	return page;
}

static int ion_scene_pool_fill(struct ion_page_pool *pool,
			       struct alloc_context_alias *ac)
{
	struct page *page;

	page = ion_scene_pool_alloc_page(pool->gfp_mask, pool->order, ac);
	if (!page)
		return -ENOMEM;

	WARN_ON(pool->order != compound_order(page));
	ion_page_scene_pool_add(pool, page);
	page_tracker_set_type(page, TRACK_ION, pool->order);
	return 0;
}

static inline int check_can_fill_flag(struct ion_scene_pool *pool)
{
	int ret;

	if (likely(atomic_read(&pool->scene_pool_can_fill_flag)))
		return 0;

	ret = -1;
	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(
			pool->pool_wait,
			atomic_read(&pool->scene_pool_can_fill_flag));
		if (ret < 0)
			continue;
		break;
	}
	return ret;
}

static void set_thread_priority_and_cpu(struct task_struct *task,
					struct ion_scene_pool *pool)
{
	unsigned long cpumask =
		(pool->worker_mask >> SPECIAL_SCENE_WORKER_CPU_SHIFT)
		& SPECIAL_SCENE_WORKER_CPU_MASK;
	unsigned long priority =
		(pool->worker_mask >> SPECIAL_SCENE_WORKER_PRIORITY_SHIFT)
		& SPECIAL_SCENE_WORKER_PRIORITY_MASK;
	int nice = PRIO_TO_NICE(priority);
	long sched_ret;

	WARN_ON(nice < MIN_NICE || nice > MAX_NICE);
	sched_ret = sched_setaffinity(task->pid, to_cpumask(&cpumask));
	WARN_ON(sched_ret < 0);
	set_user_nice(task, nice);
}

static bool has_enough_free_memory(struct ion_scene_pool *pool)
{
	bool ret = false;
	unsigned long nr_free_pages = 0, nr_zone_free, nr_cannt_used;
	struct alloc_context_alias *ac = &pool->ac;
	struct zonelist *zonelist = ac->zonelist;
	struct zoneref *z = NULL;
	struct zone *zone = NULL;

	__for_each_zone_zonelist_nodemask(zone, z, zonelist,
					  ac->high_zoneidx, ac->nodemask) { /*lint !e564*/
		nr_zone_free = zone_page_state(zone, NR_FREE_PAGES);
		nr_cannt_used = zone->watermark[ALLOC_WMARK_HIGH];
#ifdef CONFIG_CMA
		if (zone_page_state(zone, NR_FREE_CMA_PAGES) > nr_cannt_used)
			nr_cannt_used = zone_page_state(zone,
							NR_FREE_CMA_PAGES);
#endif
#ifdef CONFIG_UNMOVABLE_ISOLATE
		nr_cannt_used += zone_page_state(
					zone,
					NR_FREE_UNMOVABLE_ISOLATE2_PAGES);
#endif
		if (nr_zone_free > nr_cannt_used)
			nr_zone_free -= nr_cannt_used;
		else
			nr_zone_free = 0;

		nr_free_pages += nr_zone_free;
	}
	if (nr_free_pages > atomic_read(&pool->total_watermark)) {
		pr_info("There are enough free memory! free=%lu, need=%lu\n",
			nr_free_pages, atomic_read(&pool->total_watermark));
		ret = true;
	}

	return ret;
}

static int ion_scene_pool_kworkthread(void *p)
{
	int i, ret;
#if SCENE_POOL_ALLOC_MIN_SIZE < SZ_1M
	int total_fill, order_fill_thresold;
#endif
	struct ion_scene_pool *pool;
	bool need_retry;

	if (!p) {
		pr_err("%s: p is NULL!\n", __func__);
		return 0;
	}

	current->flags |= PF_SPECIAL_SCENE_SHRINK;
	pool = (struct ion_scene_pool *)p;
	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(
			pool->pool_wait,
			(pool->pool_wait_flag != F_NONE));
		if (ret < 0)
			continue;

		pool->pool_wait_flag = F_NONE;
		if (!pool->in_special_scene) {
			ion_scene_pool_all_free(pool, __GFP_HIGHMEM,
						pool->max_pool_size);
			continue;
		}

		if (has_enough_free_memory(pool))
			continue;
#if SCENE_POOL_ALLOC_MIN_SIZE < SZ_1M
		total_fill = atomic_read(&pool->total_watermark);
#endif
		pr_info("special scene prepare pool start ...\n");
		for (i = 0; i < scene_pool_num_orders; i++) {
#if SCENE_POOL_ALLOC_MIN_SIZE < SZ_1M
			total_fill = atomic_read(&pool->total_watermark)
					- ion_scene_pool_total_pages(pool);

			if (!i)
				order_fill_thresold = total_fill
				- (total_fill >> SPECIAL_SCENE_ORDER_SHIFT);
			else
				order_fill_thresold = total_fill;
#endif
			if (scene_pool_orders[i])
				need_retry = false;
			else
				need_retry = true;
			while (ion_scene_pool_total_pages(pool) <
			       atomic_read(&pool->total_watermark)) {
				if (check_can_fill_flag(pool))
					goto exit;

				if ((ion_scene_pool_fill(pool->pools[i],
							 &pool->ac) < 0 &&
				     !need_retry) ||
				    !(pool->worker_mask &
				      SPECIAL_SCENE_FILL_POOL_WORKER))
					break;
#if SCENE_POOL_ALLOC_MIN_SIZE < SZ_1M
				if (ion_scene_pool_pages(pool->pools[i]) >=
					order_fill_thresold)
					break;
#endif
			}
		}
		pr_info("pool prepare completed! pages=%d, watermark=%d\n",
			ion_scene_pool_total_pages(pool),
			atomic_read(&pool->total_watermark));
	}

exit:
	ion_scene_pool_all_free(pool, __GFP_HIGHMEM, pool->max_pool_size);

	return 0;
}

static int ion_scene_pool_shrink_kworkthread(void *p)
{
	int total_reclaim;
	int ret;
	struct ion_scene_pool *pool;

	if (!p) {
		pr_err("%s: p is NULL!\n", __func__);
		return 0;
	}

	current->flags |= PF_SPECIAL_SCENE_SHRINK;
	pool = (struct ion_scene_pool *)p;
	while (!kthread_should_stop()) {
		ret = wait_event_interruptible(
			pool->pool_wait,
			(pool->shrink_wait_flag == F_WAKE_UP ||
			 pool->shrink_wait_flag == F_WAKEUP_AUTOFREE));
		if (ret < 0)
			continue;

		pool->shrink_wait_flag = F_NONE;
		if (has_enough_free_memory(pool))
			continue;
		total_reclaim = atomic_read(&pool->total_watermark);

		pr_info("special scene shrink start ...\n");
		while (ion_scene_pool_total_pages(pool) <
		       atomic_read(&pool->total_watermark) &&
		       total_reclaim > 0 &&
		       (pool->worker_mask &
			SPECIAL_SCENE_SHRINK_MEMORY_WORKER) &&
		       !kthread_should_stop()) {
			total_reclaim -= shrink_memory(GFP_KERNEL,
						       0, &pool->ac);
		}
		pr_info("special scene shrink completed!\n");
	}

	return 0;
}

static inline void ion_scene_pool_setup_internal(
		struct ion_scene_pool *scene_pool,
		enum scene_pool_wait_flags flag,
		struct ion_special_scene_pool_info_data *scene_pool_info)
{
	if (flag == F_WAKE_UP) {
		scene_pool->in_special_scene = true;
		scene_pool->start_jiffies = jiffies;
		scene_pool->worker_mask =
				(unsigned)scene_pool_info->worker_mask;
		atomic_set(&scene_pool->total_watermark,
			   scene_pool_info->water_mark);
		cancel_delayed_work(
		(struct delayed_work *)(&scene_pool->autofree_delayed_work));
		pr_info("Start watermark=%d, worker=0x%x, timeout=%d S\n",
			scene_pool_info->water_mark,
			scene_pool_info->worker_mask,
			scene_pool_info->autostop_timeout);
	} else if (flag == F_WAKEUP_AUTOFREE &&
		   scene_pool_info->autostop_timeout) {
		int ret;

		scene_pool->in_special_scene = true;
		scene_pool->start_jiffies = jiffies;
		if (scene_pool_info->autostop_timeout < 0 ||
		    scene_pool_info->autostop_timeout >
		    MAX_SPECIAL_SCENE_TIMEOUT)
			scene_pool->timeout_hz = MAX_SPECIAL_SCENE_TIMEOUT * HZ;
		else
			scene_pool->timeout_hz =
				scene_pool_info->autostop_timeout * HZ;
		scene_pool->worker_mask =
				(unsigned)scene_pool_info->worker_mask;
		atomic_set(&scene_pool->total_watermark,
			   scene_pool_info->water_mark);

		ret = queue_delayed_work(system_wq,
		(struct delayed_work *)(&scene_pool->autofree_delayed_work),
		scene_pool->timeout_hz);
		if (!ret)
			mod_delayed_work(system_wq,
		(struct delayed_work *)(&scene_pool->autofree_delayed_work),
		scene_pool->timeout_hz);

		pr_info("Start watermark=%d, worker=0x%x, timeout=%d S\n",
			scene_pool_info->water_mark,
			scene_pool_info->worker_mask,
			scene_pool_info->autostop_timeout);
	} else if (flag == F_FORCE_STOP ||
		   (flag == F_WAKEUP_AUTOFREE &&
		    !scene_pool_info->autostop_timeout)) {
		atomic_set(&scene_pool->total_watermark, 0);
		scene_pool->in_special_scene = false;
		pr_info("Force stop special scene pool\n");
	}
}

void ion_scene_pool_wakeup_process(
		void *p, enum scene_pool_wait_flags flag,
		struct ion_special_scene_pool_info_data *scene_pool_info)
{
	struct ion_scene_pool *scene_pool = (struct ion_scene_pool *)p;

	if (!scene_pool || !scene_pool_info || flag == F_NONE)
		return;

	ion_scene_pool_setup_internal(scene_pool, flag, scene_pool_info);

	if (scene_pool->worker_mask & SPECIAL_SCENE_FILL_POOL_WORKER) {
		if (scene_pool->in_special_scene) {
			scene_pool->pool_wait_flag = flag;
			atomic_set(&scene_pool->scene_pool_can_fill_flag, 1);
			set_thread_priority_and_cpu(scene_pool->pool_thread,
						    scene_pool);
		} else {
			scene_pool->pool_wait_flag = F_FORCE_STOP;
		}
	}
	if (scene_pool->worker_mask & SPECIAL_SCENE_SHRINK_MEMORY_WORKER) {
		if (scene_pool->in_special_scene) {
			scene_pool->shrink_wait_flag = flag;
			set_thread_priority_and_cpu(scene_pool->shrink_thread,
						    scene_pool);
		} else {
			scene_pool->shrink_wait_flag = F_FORCE_STOP;
		}
	}

	/*Wake up all workers which waiting for pool_wait*/
	if (waitqueue_active(&scene_pool->pool_wait))
		wake_up_interruptible_all(&scene_pool->pool_wait);
}

static void ion_scene_pool_all_free(struct ion_scene_pool *pool,
				    gfp_t gfp_mask, int nr_to_scan)
{
	int i;

	for (i = 0; i < scene_pool_num_orders; i++)
		ion_page_pool_shrink(pool->pools[i],
				     gfp_mask, nr_to_scan);
}

int ion_scene_pool_shrink(struct ion_scene_pool *scene_pool,
			  struct ion_page_pool *pool, gfp_t gfp_mask,
			  int nr_to_scan)
{
	int nr_max_free;
	int nr_to_free;
	int nr_total = 0;

	if ((!scene_pool) || (!pool)) {
		pr_err("%s: scenepool/pool is NULL!\n", __func__);
		return 0;
	}

	if (nr_to_scan == 0)
		return ion_page_pool_shrink(pool, gfp_mask, 0);

	nr_max_free = ion_scene_pool_total_pages(scene_pool) -
	    atomic_read(&scene_pool->total_watermark);

	nr_to_free = SCENE_POOL_MIN(nr_max_free, nr_to_scan);

	if (nr_to_free <= 0)
		return 0;

	nr_total = ion_page_pool_shrink(pool, gfp_mask, nr_to_free);
	return nr_total;
}

void ion_scene_pool_debug_show_total(struct seq_file *s,
				     struct ion_scene_pool *scene_pool)
{
	if ((!s) || (!scene_pool)) {
		pr_err("%s: s/scene_pool is NULL!\n", __func__);
		return;
	}
	seq_puts(s, "----------------------------------------------------\n");
	seq_printf(s, "in scene pool =  %d total\n",
		   ion_scene_pool_total_pages(scene_pool) * 4 / 1024);
}

void ion_scene_pool_destroy(struct ion_scene_pool *pool)
{
	cancel_delayed_work_sync(
		(struct delayed_work *)(&pool->autofree_delayed_work));
	kthread_stop(pool->pool_thread);
	kthread_stop(pool->shrink_thread);
	ion_system_heap_destroy_pools(pool->pools);
	kfree(pool);
}

struct ion_scene_pool *ion_scene_pool_create(void)
{
	int i, ret;
	struct ion_scene_pool *scene_pool =
	    kzalloc(sizeof(struct ion_scene_pool) +
		    sizeof(struct ion_page_pool *) * scene_pool_num_orders,
		    GFP_KERNEL);

	if (!scene_pool) {
		pr_err("%s: scene_pool is NULL!\n", __func__);
		return NULL;
	}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	if (ion_system_heap_create_pools(scene_pool->pools, false))
#else
	if (ion_system_heap_create_pools(scene_pool->pools, false, false))
#endif
		goto free_heap;

	init_waitqueue_head(&scene_pool->pool_wait);
	scene_pool->pool_thread = kthread_run(ion_scene_pool_kworkthread,
					      scene_pool, "%s", "scenepool");
	if (IS_ERR(scene_pool->pool_thread)) {
		pr_err("%s: create the fill thread failed!\n", __func__);
		goto destroy_pools;
	}

	scene_pool->shrink_thread = kthread_run(
			ion_scene_pool_shrink_kworkthread, scene_pool,
			"%s", "scenepool:shrink");
	if (IS_ERR(scene_pool->shrink_thread)) {
		pr_err("%s: create the shrink thread failed!\n", __func__);
		goto destroy_fill_thread;
	}

	atomic_set(&scene_pool->total_watermark, 0);
	scene_pool->worker_mask = 0;
	scene_pool->pool_wait_flag = F_NONE;
	scene_pool->max_pool_size = MAX_SPECIAL_SCENE_POOL_SIZE;
	for (i = 0; i < scene_pool_num_orders; i++)
		scene_pool->pools[i]->gfp_mask &=
					~(__GFP_KSWAPD_RECLAIM | __GFP_ZERO);
	scene_pool->autofree_delayed_work.pool = (unsigned long)scene_pool;
	INIT_DEFERRABLE_WORK(
		(struct delayed_work *)(&scene_pool->autofree_delayed_work),
		ion_scene_pool_autofree_workfn);
	ret = ion_scene_pool_alloc_context_setup(
				scene_pool->pools[0]->gfp_mask,
				node_zonelist(numa_node_id(),
					      scene_pool->pools[0]->gfp_mask),
				&scene_pool->ac);
	if (ret) {
		pr_err("%s: setup alloc context failed!\n", __func__);
		goto destroy_shrink_thread;
	}

	return scene_pool;


destroy_shrink_thread:
	kthread_stop(scene_pool->shrink_thread);

destroy_fill_thread:
	kthread_stop(scene_pool->pool_thread);

destroy_pools:
	ion_system_heap_destroy_pools(scene_pool->pools);

free_heap:
	kfree(scene_pool);
	return NULL;
}
