/*
 * process_reclaim_info.c : Process reclaim information
 *
 * Copyright (c) 2016 Huawei.
 *
 * Authors:
 * Wanglai Yao <yaowanglai@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/fs.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/of_fdt.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/sched.h>
#include "internal.h"

static struct kmem_cache *process_reclaim_result_cache;

static int __init process_reclaim_result_cache_create(void)
{
	process_reclaim_result_cache = KMEM_CACHE(reclaim_result, 0);
	return process_reclaim_result_cache == NULL;
}

struct reclaim_result *process_reclaim_result_cache_alloc(gfp_t gfp)
{
	struct reclaim_result *result = NULL;

	if (process_reclaim_result_cache)
		result = kmem_cache_alloc(process_reclaim_result_cache, gfp);
	return result;
}

void process_reclaim_result_cache_free(struct reclaim_result *result)
{
	if (process_reclaim_result_cache)
		kmem_cache_free(process_reclaim_result_cache, result);
}

int process_reclaim_result_read(struct seq_file *m, struct pid_namespace *ns,
				struct pid *pid, struct task_struct *tsk)
{
	struct reclaim_result *result;
	unsigned nr_reclaimed = 0;
	unsigned nr_writedblock = 0;
	s64 elapsed_centisecs64 = 0;

	if (tsk) {
		task_lock(tsk);
		result = tsk->proc_reclaimed_result;
		if (result) {
			nr_reclaimed = result->nr_reclaimed;
			nr_writedblock = result->nr_writedblock;
			elapsed_centisecs64 = result->elapsed_centisecs64;
			tsk->proc_reclaimed_result = NULL;
			task_unlock(tsk);
			process_reclaim_result_cache_free(result);
		} else {
			task_unlock(tsk);
		}
	}
#if KERNEL_VERSION(4, 3, 0) <= LINUX_VERSION_CODE
	seq_printf(m,
			"nr_reclaimed=%u, nr_writedblock=%u, elapsed=%lld\n",
			nr_reclaimed, nr_writedblock, elapsed_centisecs64);
	return 0;
#else
	return seq_printf(m,
			"nr_reclaimed=%u, nr_writedblock=%u, elapsed=%lld\n",
			nr_reclaimed, nr_writedblock, elapsed_centisecs64);
#endif
}

void exit_proc_reclaim(struct task_struct *tsk)
{
	if (tsk) {
		task_lock(tsk);
		if (tsk->proc_reclaimed_result) {
			struct reclaim_result *result =
			tsk->proc_reclaimed_result;

			tsk->proc_reclaimed_result = NULL;
			task_unlock(tsk);
			process_reclaim_result_cache_free
			(result);
		} else {
			task_unlock(tsk);
		}
	}
}

void process_reclaim_result_write(struct task_struct *task,
		unsigned nr_reclaimed, unsigned nr_writedblock,
		s64 elapsed_centisecs64)
{
	struct reclaim_result *result = NULL;

	task_lock(task);
	if (!task->proc_reclaimed_result) {
		task_unlock(task);
#if KERNEL_VERSION(4, 3, 0) <= LINUX_VERSION_CODE
		result  =
		process_reclaim_result_cache_alloc(__GFP_NOWARN
						   | __GFP_NORETRY
						   | __GFP_KSWAPD_RECLAIM);
#else
		result  =
		process_reclaim_result_cache_alloc(__GFP_NOWARN
						   | __GFP_NORETRY);
#endif

		task_lock(task);
		if (!task->proc_reclaimed_result) {
			task->proc_reclaimed_result = result;
			result = NULL;
		}
	}
	if (task->proc_reclaimed_result) {
		/*ns->>us*/
		task->proc_reclaimed_result->elapsed_centisecs64 =
		elapsed_centisecs64 / 1000;
		task->proc_reclaimed_result->nr_writedblock = nr_writedblock;
		task->proc_reclaimed_result->nr_reclaimed = nr_reclaimed;
	}
	task_unlock(task);
	if (result)
		process_reclaim_result_cache_free(result);
}

bool process_reclaim_need_abort(struct mm_walk *walk)
{
	struct mm_struct *mm;
#if KERNEL_VERSION(4, 9, 0) <= LINUX_VERSION_CODE
	if (!walk || !walk->private) {
		return false;
	} else {
		struct reclaim_param *rp = walk->private;

		if (!rp->hiber)
			return false;
	}
#else
	if (!walk || !walk->hiber)
		return false;
#endif

	if (reclaim_sigusr_pending(current)) {
		pr_info("Reclaim abort!case is signal.\n");
		return true;
	}

	mm = walk->mm;
	if (mm && !list_empty(&mm->mmap_sem.wait_list)) {
		pr_info("Reclaim abort!case is lock race.\n");
		return true;
	}

	return false;
}

static int __init process_reclaim_info_init(void)
{
	int ret = 0;

	ret = process_reclaim_result_cache_create();

	if (ret)
		pr_err("Process reclaim information module init failed!\n");
	return ret;
}
late_initcall(process_reclaim_info_init);

MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("Wanglai Yao <yaowanglai@huawei.com>");
MODULE_DESCRIPTION("Process reclaim information");
