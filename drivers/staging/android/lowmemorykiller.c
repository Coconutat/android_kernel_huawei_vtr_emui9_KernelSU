/* drivers/misc/lowmemorykiller.c
 *
 * The lowmemorykiller driver lets user-space specify a set of memory thresholds
 * where processes with a range of oom_score_adj values will get killed. Specify
 * the minimum oom_score_adj values in
 * /sys/module/lowmemorykiller/parameters/adj and the number of free pages in
 * /sys/module/lowmemorykiller/parameters/minfree. Both files take a comma
 * separated list of numbers in ascending order.
 *
 * For example, write "0,8" to /sys/module/lowmemorykiller/parameters/adj and
 * "1024,4096" to /sys/module/lowmemorykiller/parameters/minfree to kill
 * processes with a oom_score_adj value of 8 or higher when the free memory
 * drops below 4096 pages and kill processes with a oom_score_adj value of 0 or
 * higher when the free memory drops below 1024 pages.
 *
 * The driver considers memory used for caches to be free, but if a large
 * percentage of the cached memory is locked this can be very inaccurate
 * and processes may not get killed until the normal oom killer is triggered.
 *
 * Copyright (C) 2007-2008 Google, Inc.
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/init.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/oom.h>
#include <linux/sched.h>
#include <linux/swap.h>
#include <linux/rcupdate.h>
#include <linux/profile.h>
#include <linux/notifier.h>
#include <linux/atomic.h>
#include <hisi/hisi_lmk/lowmem_killer.h>

#define CREATE_TRACE_POINTS
#include "trace/lowmemorykiller.h"

#ifdef CONFIG_HUAWEI_KSTATE
#include <huawei_platform/power/hw_kcollect.h>
#endif

#ifdef CONFIG_HW_ZEROHUNG
#include <chipset_common/hwzrhung/zrhung.h>
#endif

static u32 lowmem_debug_level = 1;
static short lowmem_adj[6] = {
	0,
	1,
	6,
	12,
};

static int lowmem_adj_size = 4;
static int lowmem_minfree[6] = {
	3 * 512,	/* 6MB */
	2 * 1024,	/* 8MB */
	4 * 1024,	/* 16MB */
	16 * 1024,	/* 64MB */
};

static int lowmem_minfree_size = 4;
#if defined CONFIG_LOG_JANK
static ulong lowmem_kill_count;
static ulong lowmem_free_mem;
#endif
#ifdef CONFIG_HISI_MULTI_KILL
/*
 * lmk_multi_kill: select open/close multi kill
 * if lmk_multi_kill open
 *    1/selected process adj >= lmk_multi_fadj,
 *      we kill multi process max count = lmk_multi_fcount,
 *    2/selected process adj < lmk_multi_fadj,
 *      select process adj >= lmk_multi_sadj,
 *      we kill multi process max count = lmk_multi_scount,
 *    3/selected process adj < lmk_multi_sadj,
 *      we kill one process,
 */
static int lmk_multi_kill;
static int lmk_multi_fadj = 800;
static int lmk_multi_fcount = 5;
static int lmk_multi_sadj = 300;
static int lmk_multi_scount = 3;
static int lmk_timeout_inter = 1;
#endif

static unsigned long lowmem_deathpending_timeout;

#define lowmem_print(level, x...)			\
	do {						\
		if (lowmem_debug_level >= (level))	\
			pr_info(x);			\
	} while (0)

static unsigned long lowmem_count(struct shrinker *s,
				  struct shrink_control *sc)
{
	return global_node_page_state(NR_ACTIVE_ANON) +
		global_node_page_state(NR_ACTIVE_FILE) +
		global_node_page_state(NR_INACTIVE_ANON) +
		global_node_page_state(NR_INACTIVE_FILE);
}

static unsigned long lowmem_scan(struct shrinker *s, struct shrink_control *sc)
{
	struct task_struct *tsk;
	struct task_struct *selected = NULL;
	unsigned long rem = 0;
	int tasksize;
	int i;
	short min_score_adj = OOM_SCORE_ADJ_MAX + 1;
	int minfree = 0;
	int selected_tasksize = 0;
	short selected_oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);
	int other_free = global_page_state(NR_FREE_PAGES) - totalreserve_pages;
	int other_file = global_node_page_state(NR_FILE_PAGES) -
				global_node_page_state(NR_SHMEM) -
				global_node_page_state(NR_UNEVICTABLE) -
				total_swapcache_pages();
	int ret_tune;

#ifdef CONFIG_HISI_MULTI_KILL
	int count = 0;
#endif
	static atomic_t atomic_lmk = ATOMIC_INIT(0);

	ret_tune = hisi_lowmem_tune(&other_free, &other_file, sc);

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;
	if (lowmem_minfree_size < array_size)
		array_size = lowmem_minfree_size;
	for (i = 0; i < array_size; i++) {
		minfree = lowmem_minfree[i];
		if (other_free < minfree && other_file < minfree) {
			min_score_adj = lowmem_adj[i];
			break;
		}
	}

	lowmem_print(3, "lowmem_scan %lu, %x, ofree %d %d, ma %hd\n",
		     sc->nr_to_scan, sc->gfp_mask, other_free,
		     other_file, min_score_adj);

	if (min_score_adj == OOM_SCORE_ADJ_MAX + 1) {
		lowmem_print(5, "lowmem_scan %lu, %x, return 0\n",
			     sc->nr_to_scan, sc->gfp_mask);
		return 0;
	}

	selected_oom_score_adj = min_score_adj;

	if (atomic_inc_return(&atomic_lmk) > 1) {
		atomic_dec(&atomic_lmk);
		return 0;
	}

	rcu_read_lock();
#ifdef CONFIG_HISI_MULTI_KILL
kill_selected:
#endif
	for_each_process(tsk) {
		struct task_struct *p;
		short oom_score_adj;

		if (tsk->flags & PF_KTHREAD)
			continue;

		p = find_lock_task_mm(tsk);
		if (!p)
			continue;

		if (task_lmk_waiting(p)) {
			if (time_before_eq(jiffies,
					lowmem_deathpending_timeout)) {
				task_unlock(p);
				rcu_read_unlock();
				atomic_dec(&atomic_lmk);
				return 0;
			} else {
				hisi_lowmem_dbg_timeout(tsk, p);
#ifdef CONFIG_HISI_MULTI_KILL
				if (lmk_multi_kill) {
					task_unlock(p);
					continue;
				}
#endif
			}
		}

		oom_score_adj = p->signal->oom_score_adj;
		if (oom_score_adj < min_score_adj) {
			task_unlock(p);
			continue;
		}
		tasksize = get_mm_rss(p->mm);
		task_unlock(p);
		if (tasksize <= 0)
			continue;
		if (selected) {
			if (oom_score_adj < selected_oom_score_adj)
				continue;
			if (oom_score_adj == selected_oom_score_adj &&
			    tasksize <= selected_tasksize)
				continue;
		}
		selected = p;
		selected_tasksize = tasksize;
		selected_oom_score_adj = oom_score_adj;
		lowmem_print(2, "select '%s' (%d), adj %hd, size %d, to kill\n",
			     p->comm, p->pid, oom_score_adj, tasksize);
	}
	if (selected) {
		long cache_size = other_file * (long)(PAGE_SIZE / 1024);
		long cache_limit = minfree * (long)(PAGE_SIZE / 1024);
		long free = other_free * (long)(PAGE_SIZE / 1024);

#ifdef CONFIG_HUAWEI_KSTATE
		/*0 stand for low memory kill*/
		hwkillinfo(selected->tgid, 0);
#endif
		task_lock(selected);
		send_sig(SIGKILL, selected, 0);
		if (selected->mm)
			task_set_lmk_waiting(selected);
		task_unlock(selected);
		trace_lowmemory_kill(selected, cache_size, cache_limit, free);
		lowmem_print(1, "Killing '%s' (%d) (tgid %d), adj %hd,\n"
				 "   to free %ldkB on behalf of '%s' (%d) because\n"
				 "   cache %ldkB is below limit %ldkB for oom_score_adj %hd\n"
				 "   Free memory is %ldkB above reserved (%d 0x%x)\n",
			     selected->comm, selected->pid, selected->tgid,
			     selected_oom_score_adj,
			     selected_tasksize * (long)(PAGE_SIZE / 1024),
			     current->comm, current->pid,
			     cache_size, cache_limit,
			     min_score_adj,
			     free, ret_tune, sc->gfp_mask);

		hisi_lowmem_dbg(selected_oom_score_adj);

#ifdef CONFIG_HW_ZEROHUNG
#ifdef CONFIG_HISI_MULTI_KILL
		if (count  == 0)
#endif
			lmkwp_report(selected, sc, cache_size, cache_limit, selected_oom_score_adj, free);
#endif

#ifdef CONFIG_HISI_MULTI_KILL
		lowmem_deathpending_timeout = jiffies + lmk_timeout_inter * HZ;/*lint !e647*/
#else
		lowmem_deathpending_timeout = jiffies + HZ;
#endif

#if defined CONFIG_LOG_JANK
		lowmem_kill_count++;
		lowmem_free_mem += selected_tasksize *
			(long)(PAGE_SIZE / 1024) / 1024;
#endif

		rem += selected_tasksize;
	}

	lowmem_print(4, "lowmem_scan %lu, %x, return %lu\n",
		     sc->nr_to_scan, sc->gfp_mask, rem);

#ifdef CONFIG_HISI_MULTI_KILL
	if (selected && lmk_multi_kill) {
		count++;
		if (!((count >= lmk_multi_fcount) ||
			(selected_oom_score_adj < lmk_multi_sadj) ||
			((selected_oom_score_adj < lmk_multi_fadj) &&
				(count >= lmk_multi_scount)))) {
			selected = NULL;
			goto kill_selected;
		}
	}
#endif

	rcu_read_unlock();
	atomic_dec(&atomic_lmk);
	return rem;
}

static struct shrinker lowmem_shrinker = {
	.scan_objects = lowmem_scan,
	.count_objects = lowmem_count,
	.seeks = DEFAULT_SEEKS * 16
};

static int __init lowmem_init(void)
{
#ifdef CONFIG_HW_ZEROHUNG
	lmkwp_init();
#endif
	register_shrinker(&lowmem_shrinker);
	return 0;
}
device_initcall(lowmem_init);

#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
static short lowmem_oom_adj_to_oom_score_adj(short oom_adj)
{
	if (oom_adj == OOM_ADJUST_MAX)
		return OOM_SCORE_ADJ_MAX;
	else
		return (oom_adj * OOM_SCORE_ADJ_MAX) / -OOM_DISABLE;
}

static void lowmem_autodetect_oom_adj_values(void)
{
	int i;
	short oom_adj;
	short oom_score_adj;
	int array_size = ARRAY_SIZE(lowmem_adj);

	if (lowmem_adj_size < array_size)
		array_size = lowmem_adj_size;

	if (array_size <= 0)
		return;

	oom_adj = lowmem_adj[array_size - 1];
	if (oom_adj > OOM_ADJUST_MAX)
		return;

	oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
	if (oom_score_adj <= OOM_ADJUST_MAX)
		return;

	lowmem_print(1, "lowmem_shrink: convert oom_adj to oom_score_adj:\n");
	for (i = 0; i < array_size; i++) {
		oom_adj = lowmem_adj[i];
		oom_score_adj = lowmem_oom_adj_to_oom_score_adj(oom_adj);
		lowmem_adj[i] = oom_score_adj;
		lowmem_print(1, "oom_adj %d => oom_score_adj %d\n",
			     oom_adj, oom_score_adj);
	}
}

static int lowmem_adj_array_set(const char *val, const struct kernel_param *kp)
{
	int ret;

	ret = param_array_ops.set(val, kp);

	/* HACK: Autodetect oom_adj values in lowmem_adj array */
	lowmem_autodetect_oom_adj_values();

	return ret;
}

static int lowmem_adj_array_get(char *buffer, const struct kernel_param *kp)
{
	return param_array_ops.get(buffer, kp);
}

static void lowmem_adj_array_free(void *arg)
{
	param_array_ops.free(arg);
}

static struct kernel_param_ops lowmem_adj_array_ops = {
	.set = lowmem_adj_array_set,
	.get = lowmem_adj_array_get,
	.free = lowmem_adj_array_free,
};

static const struct kparam_array __param_arr_adj = {
	.max = ARRAY_SIZE(lowmem_adj),
	.num = &lowmem_adj_size,
	.ops = &param_ops_short,
	.elemsize = sizeof(lowmem_adj[0]),
	.elem = lowmem_adj,
};
#endif

/*
 * not really modular, but the easiest way to keep compat with existing
 * bootargs behaviour is to continue using module_param here.
 */
module_param_named(cost, lowmem_shrinker.seeks, int, 0644);
#ifdef CONFIG_ANDROID_LOW_MEMORY_KILLER_AUTODETECT_OOM_ADJ_VALUES
module_param_cb(adj, &lowmem_adj_array_ops,
		.arr = &__param_arr_adj,
		0644);
__MODULE_PARM_TYPE(adj, "array of short");
#else
module_param_array_named(adj, lowmem_adj, short, &lowmem_adj_size, 0644);
#endif
module_param_array_named(minfree, lowmem_minfree, uint, &lowmem_minfree_size,
			 0644);
module_param_named(debug_level, lowmem_debug_level, uint, 0644);
#if defined CONFIG_LOG_JANK
module_param_named(kill_count, lowmem_kill_count, ulong, S_IRUGO | S_IWUSR);
module_param_named(free_mem, lowmem_free_mem, ulong, S_IRUGO | S_IWUSR);
#endif
#ifdef CONFIG_HISI_MULTI_KILL
module_param_named(lmk_multi_kill, lmk_multi_kill, int, S_IRUGO | S_IWUSR);
module_param_named(lmk_multi_fadj, lmk_multi_fadj, int, S_IRUGO | S_IWUSR);
module_param_named(lmk_multi_fcount, lmk_multi_fcount, int, S_IRUGO | S_IWUSR);
module_param_named(lmk_multi_sadj, lmk_multi_sadj, int, S_IRUGO | S_IWUSR);
module_param_named(lmk_multi_scount, lmk_multi_scount, int, S_IRUGO | S_IWUSR);
module_param_named(lmk_timeout_inter, lmk_timeout_inter, int,
			S_IRUGO | S_IWUSR);
#endif

