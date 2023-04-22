/*
 * A fast way to read system free and aviliable meminfo
 *
 * Copyright (c) 2016 Hisilicon.
 *
 * Authors:
 * liang hui <lianghuiliang.lianghui@huawei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/fs.h>
#include <linux/hugetlb.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/mm.h>
#include <linux/mman.h>
#include <linux/mmzone.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/vmstat.h>
#include <linux/vmalloc.h>
#include <linux/version.h>
#include <asm/page.h>
#include <asm/pgtable.h>

#define K(x) (((x) << (PAGE_SHIFT - 10)))
static int meminfo_lite_proc_show(struct seq_file *m, void *v)
{
#if(LINUX_VERSION_CODE >= KERNEL_VERSION(4,9,0))
	struct sysinfo i;
	long available;

	/*
	 * display in kilobytes.
	 */
	si_meminfo(&i);
	available = si_mem_available();
#else
	struct sysinfo i;
	long available;
	unsigned long pagecache;
	unsigned long wmark_low = 0;
	unsigned long pages[NR_LRU_LISTS];
	struct zone *zone;
	int lru;

	/*
	 * display in kilobytes.
	 */
	si_meminfo(&i);
	for (lru = LRU_BASE; lru < NR_LRU_LISTS; lru++)
		pages[lru] = global_page_state(NR_LRU_BASE + lru);

	for_each_zone(zone)
		wmark_low += zone->watermark[WMARK_LOW];

	/*
	 * Estimate the amount of memory available for userspace allocations,
	 * without causing swapping.
	 *
	 * Free memory cannot be taken below the low watermark, before the
	 * system starts swapping.
	 */
	available = i.freeram - wmark_low;
	/*
	 * Not all the page cache can be freed, otherwise the system will
	 * start swapping. Assume at least half of the page cache, or the
	 * low watermark worth of cache, needs to stay.
	 */
	pagecache = pages[LRU_ACTIVE_FILE] + pages[LRU_INACTIVE_FILE];
	pagecache -= min(pagecache / 2, wmark_low);
	available += pagecache;

	/*
	 * Part of the reclaimable slab consists of items that are in use,
	 * and cannot be freed. Cap this estimate at the low watermark.
	 */
	available += global_page_state(NR_SLAB_RECLAIMABLE) -
		min(global_page_state(NR_SLAB_RECLAIMABLE) / 2, wmark_low);/*lint !e666*/

	/*
	 * Add the ioncache pool pages
	 */
	available += global_page_state(NR_IONCACHE_PAGES);

	available += (long)global_page_state(NR_MALI_PAGES);

#ifdef CONFIG_TASK_PROTECT_LRU
	available -= (long)global_page_state(NR_PROTECT_ACTIVE_FILE);
#endif
#endif
	if (available < 0)
		available = 0;

	/*
	 * Tagged format, for easy grepping and expansion.
	 */
	seq_printf(m, "Fr:%lu,Av:%lu\n", K(i.freeram), K(available));

	hugetlb_report_meminfo(m);

	return 0;
#undef K
}

static int meminfo_lite_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, meminfo_lite_proc_show, NULL);
}

static const struct file_operations meminfo_lite_proc_fops = {
	.open		= meminfo_lite_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init proc_meminfo_lite_init(void)
{
	proc_create("meminfo_lite", 0, NULL, &meminfo_lite_proc_fops);
	return 0;
}
module_init(proc_meminfo_lite_init);
