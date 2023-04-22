/*
 * test_pmalloc.c: Protectable Memory Allocator - selftest
 *
 * (C) Copyright 2018 Huawei Technologies Co. Ltd.
 * Author: Igor Stoppa <igor.stoppa@huawei.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/debugfs.h>
#include <linux/vmalloc.h>
#include <linux/pmalloc.h>
#include "pmalloc_usercopy.h"

#define MEASUREMENTS 1000000

static struct dentry *pmalloc_root;
static bool vmall;
static bool pmall;
static u64 total_overhead_ns;
static u64 iterations;

#define to_nsec(time_sec) (1000000000UL * (time_sec))

bool measure_usercopy(struct dentry *dir, struct gen_pool *pool)
{
	struct timespec start;
	struct timespec	end;
	unsigned int i;
	const void *addr;
	const char *retval = NULL;

	if (WARN(!debugfs_create_u64("iterations",
				     0444, dir, &iterations),
		 "Failed to create debugfs entry"))
		return false;

	if (WARN(!debugfs_create_u64("total_overhead_ns",
				     0444, dir, &total_overhead_ns),
		 "Failed to create debugfs entry"))
		return false;

	addr = (void *)pmalloc(pool, 1, GFP_KERNEL);
	if (WARN(!addr, "Failed to allocate pmalloc memory"))
	    return false;

	/*
	 * The assignment and tests on retval are done only to avoid that
	 * the compiler optimizes anything away.
	 * For the same reason, both retval and addr are declared as
	 * volatile.
	 * But it's not really expected that is_pmalloc_addr() will ever
	 * evaluate to something different than true.
	 */
	getnstimeofday64(&start);
	for (i = 0; (i < MEASUREMENTS) && !retval; i++)
		retval = is_pmalloc_addr(addr);
	getnstimeofday64(&end);
	iterations = MEASUREMENTS;
	total_overhead_ns = (u64)(to_nsec(end.tv_sec - start.tv_sec) +
				  end.tv_nsec - start.tv_nsec);
	return !retval;
}

/*
 * Intentionally do not tear down debugfs entries in case of error,
 * to allow for inspection.
 */
bool test_usercopy(struct dentry *root)
{
	void *vmall_addr;
	void *pmall_addr;
	struct dentry *dir;
	bool retval = false;
	struct gen_pool *pool;

	dir = debugfs_create_dir("hardened_usercopy", root);
	if (WARN(!dir, "Failed to create debugfs dir"))
		return false;

	if (WARN(!(debugfs_create_bool("test_vmalloc_addr",
				       0444, dir, &vmall) &&
		   debugfs_create_bool("test_pmalloc_addr",
				       0444, dir, &pmall)),
		 "Failed to create debugfs entry"))
		return false;

	vmall_addr = vmalloc(1);
	if (WARN(!vmall_addr,"failed to allocate memory from vmalloc"))
		goto err_pool;

	pool = pmalloc_create_pool("test_pmalloc", 1);
	if (WARN(!pool, "Failed to create pmalloc pool"))
		goto err_pool;

	pmall_addr = pmalloc(pool, 1, GFP_KERNEL);
	if (WARN(!pmall_addr, "Failed to allocate from pmalloc pool"))
		goto err_pmalloc;

	vmall = is_pmalloc_addr(vmall_addr);
	pmall = is_pmalloc_addr(pmall_addr);

	retval = measure_usercopy(dir, pool);
	pmalloc_protect_pool(pool);
err_pmalloc:
	pmalloc_destroy_pool(pool);
err_pool:
	if (vmall_addr) {
		vfree(vmall_addr);
	}
	return retval;
}


static int __init test_pmalloc(void)
{
	pmalloc_root = debugfs_create_dir("pmalloc", pmalloc_root);
	if (WARN(!pmalloc_root, "Failed to create debugfs dir"))
		return -1;

	return test_usercopy(pmalloc_root);
}

static void __exit unload(void)
{
	debugfs_remove_recursive(pmalloc_root);
}

module_init(test_pmalloc);
module_exit(unload)
