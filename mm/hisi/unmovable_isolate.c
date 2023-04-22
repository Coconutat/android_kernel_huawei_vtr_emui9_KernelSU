/*
 * mm/unmovable_isolate.c
 *
 * MIGRATE_UNMOVABLE_ISOLATE function
 *
 * Copyright (C) 2017 Huawei Technologies Co., Ltd.
 */

#include <linux/mmzone.h>
#include <linux/mm.h>
#include <linux/vmstat.h>
#include <linux/init.h>
#include <linux/page-isolation.h>
#include <linux/unmovable_isolate.h>

/*
 * The switch of unmovable_isolate function, expect 0 or 1.
 * /proc/sys/vm/unmovable_isolate_disabled
 * Once set as 1, the value will not be changed.
 */
int unmovable_isolate_disabled = 0;

/*
 * The follow functions are parsing the ui or reserve size
 * from cmdline.
 */
enum {
	MEMSIZE_2G_INDEX,
	MEMSIZE_INDEX_MIN = MEMSIZE_2G_INDEX,
	MEMSIZE_3G_INDEX,
	MEMSIZE_4G_INDEX,
	MEMSIZE_INDEX_MAX = MEMSIZE_4G_INDEX,
	MEMSIZE_INDEX_COUNT
};

#define MEMSIZE_INDEX_OFFSET 2

/*
 * unmovable-isolate size for each ddr-size (2G,3G,4G+ ).
 * the type is same as memparse function.
 */
static long long size_cmdline_ui1[MEMSIZE_INDEX_COUNT] = {-1,-1,-1};
static long long size_cmdline_ui2[MEMSIZE_INDEX_COUNT] = {-1,-1,-1};

/*
 * check if the ddrsize is valid for unmovable_isolate area
 */
static int valid_ddr_size_for_ui(long long size)
{
	long long size_offset = size - MEMSIZE_INDEX_OFFSET;
	if (size_offset >= MEMSIZE_INDEX_MIN && size_offset <= MEMSIZE_INDEX_MAX)
		return 1;
	else
		return 0;
}

/*
 * parse the unmovable-isolate size in cmdline,
 * the syntax must be:
 * 1. size[KMG]
 * 2. ddr_size1:size1[KMG][,ddr_size2:size2[KMG],...]
 */
int __init parse_unmovable_isolate_size(long long * size_cmdline, char *ui_cmdline)
{
	char *first_colon;
	long long ui_size = -1;
	char *cur = ui_cmdline, *tmp;
	int i;

	/*
	 * if the commandline contains a ':', then that's the extended syntax
	 * if not, all ddrsize device will set the same size.
	 */
	first_colon = strchr(ui_cmdline, ':');
	if (first_colon) {
		/* for each entry of the comma-separated list */
		do {
			long long parsed_ddr_size, size;

			/* get the ddr_size in the cmdline */
			parsed_ddr_size = memparse(cur, &tmp);
			if (cur == tmp) {
				pr_err("parse_unmovable_isolate_size: Memory value expected\n");
				return -1;
			}

			cur = tmp;
			if (*cur != ':') {
				pr_err("parse_unmovable_isolate_size: ':' expected\n");
				return -1;
			}
			cur++;

			/* get the related unmovable-isolate size */
			size = memparse(cur, &tmp);
			if (cur == tmp) {
				pr_err("Memory value expected\n");
				return -1;
			}
			cur = tmp;

			/* match? */
			if (valid_ddr_size_for_ui(parsed_ddr_size)) {
				size_cmdline[parsed_ddr_size - MEMSIZE_INDEX_OFFSET] = size;
			}
		} while (*cur++ == ',');
	} else {
		ui_size = memparse(ui_cmdline, &ui_cmdline);
		for (i=0; i < MEMSIZE_INDEX_COUNT; i++)
			size_cmdline[i] = ui_size;
	}

	return 0;
}

static int __init early_unmovable_isolate1(char *p)
{
	pr_debug("%s(%s)\n", __func__, p);
	parse_unmovable_isolate_size(size_cmdline_ui1, p);
	return 0;
}

early_param("unmovable_isolate1", early_unmovable_isolate1);

static int __init early_unmovable_isolate2(char *p)
{
	pr_debug("%s(%s)\n", __func__, p);
	parse_unmovable_isolate_size(size_cmdline_ui2, p);
	return 0;
}

early_param("unmovable_isolate2", early_unmovable_isolate2);

#ifdef CONFIG_HUAWEI_ENHANCED_RESERVE
/* enhanced_reserve size for each ddr-size (2G,3G,4G+ ) */
static long long size_cmdline_reserve[MEMSIZE_INDEX_COUNT] = {-1,-1,-1};

static int __init early_enhanced_reserve(char *p)
{
	pr_debug("%s(%s)\n", __func__, p);
	parse_unmovable_isolate_size(size_cmdline_reserve, p);
	return 0;
}
early_param("enhanced_reserve", early_enhanced_reserve);
#endif

/* get the ddrsize from cmdline */
static long long ddr_size_cmdline = 0;
static int __init early_ddr_size(char *p)
{
	char *tmp;
	pr_debug("%s(%s)\n", __func__, p);
	ddr_size_cmdline = simple_strtoll(p, &tmp, 0);
	return 0;
}

early_param("androidboot.ddrsize", early_ddr_size);

/*
 * The follow functions are checking whether zone and order are
 * the vaild for unmovable-isolate.
 */

/*
 * check if the zone is DMA.
 */
int is_DMA_zone(struct zone* zone)
{
	if (zone != NULL && zone_idx(zone) == ZONE_DMA)
		return 1;
	else
		return 0;
}

/*
 * check if the order is valid for unmovable_isolate area.
 */
int valid_order_for_ui(int order, int migratetype)
{
	if (is_unmovable_isolate1(migratetype) &&
	   (order >= UNMOVABLE_ISOLATE1_MIN_ORDER &&
	    order <= UNMOVABLE_ISOLATE1_MAX_ORDER))
		return 1;
	else if (is_unmovable_isolate2(migratetype) &&
	        (order >= UNMOVABLE_ISOLATE2_MIN_ORDER &&
	         order <= UNMOVABLE_ISOLATE2_MAX_ORDER))
		return 1;
#ifdef CONFIG_HUAWEI_ENHANCED_RESERVE
	else if ((migratetype == MIGRATE_RESERVE) &&
	        (order >= RESERVE_MIN_ORDER && order <= RESERVE_MAX_ORDER))
		return 1;
#endif
	return 0;
}

/*
 * check if the unmovable_isolate_enabled is enabled.
 * We only enable ui function in DMA zone and unmovable_isolate_disabled is 0.
 */
int unmovable_isolate_enabled (struct zone* zone)
{
	if (!unmovable_isolate_disabled && is_DMA_zone(zone))
		return 1;
	else
		return 0;
}

/*
 * check if the pageblock is UNMOVABLE_ISOLATE.
 */
int unmovable_isolate_pageblock(struct zone* zone, struct page* page)
{
	int migratetype;
	migratetype = get_pageblock_migratetype(page);
	if (unmovable_isolate_enabled(zone) &&
	    (is_unmovable_isolate1(migratetype) || is_unmovable_isolate2(migratetype)))
		return 1;
	else
		return 0;
}

#define MEMSIZE_2G_in_MB 2048
#define MEMSIZE_3G_in_MB 3072

/*
 * get the device DDR size
 */
int get_ddr_size(void)
{
	int ret;

	if (ddr_size_cmdline > 0 && ddr_size_cmdline < INT_MAX)
		/* we set the same ui area size when ddr_size above MEMSIZE_INDEX_MAX */
		ret = min(MEMSIZE_INDEX_MAX + MEMSIZE_INDEX_OFFSET, ddr_size_cmdline);
	else
		ret = 0;
	pr_debug("%s:ddrsize is %d\n", __func__, ret);
	return ret;
}

#ifdef CONFIG_HUAWEI_ENHANCED_RESERVE
/*
 * get the enhanced_reserve size
 */
int get_enhanced_reserve_size(void)
{
	int enhanced_reserve_size, ddr_size;

	ddr_size = get_ddr_size();

	if (ddr_size!= 0 && size_cmdline_reserve[ddr_size - MEMSIZE_INDEX_OFFSET] > 0)
		enhanced_reserve_size =
		    (unsigned long long)size_cmdline_reserve[ddr_size - MEMSIZE_INDEX_OFFSET] / PAGE_SIZE / pageblock_nr_pages;
	else
		enhanced_reserve_size = ENHANCED_RESERVE_SIZE_BLOCKS;

	return enhanced_reserve_size;
}
#endif

/*
 * get the related unmovable-isolate size
 */
static long long get_unmovable_isolate_blocks (int unmovable_isolate_type)
{
	int ddr_size;
	long long ui_blocks = 0;

	ddr_size = get_ddr_size();

	if (is_unmovable_isolate1(unmovable_isolate_type)) {
		if (ddr_size!= 0 && size_cmdline_ui1[ddr_size - MEMSIZE_INDEX_OFFSET] > 0)
			ui_blocks =
			    (unsigned long long)size_cmdline_ui1[ddr_size - MEMSIZE_INDEX_OFFSET] / PAGE_SIZE / pageblock_nr_pages;
		else
			ui_blocks = UNMOVABLE_ISOLATE1_SIZE_BLOCKS;
	} else if (is_unmovable_isolate2(unmovable_isolate_type)) {
		if (ddr_size!= 0 && size_cmdline_ui2[ddr_size - MEMSIZE_INDEX_OFFSET] > 0)
			ui_blocks =
			    (unsigned long long)size_cmdline_ui2[ddr_size - MEMSIZE_INDEX_OFFSET] / PAGE_SIZE / pageblock_nr_pages;
		else
			ui_blocks = UNMOVABLE_ISOLATE2_SIZE_BLOCKS;
	}

	return ui_blocks;
}

/*
 * Check if a pageblock contains reserved pages
 */
static int pageblock_is_reserved_for_ui(unsigned long start_pfn, unsigned long end_pfn)
{
	unsigned long pfn;

	for (pfn = start_pfn; pfn < end_pfn; pfn++) {
		if (!pfn_valid_within(pfn) || PageReserved(pfn_to_page(pfn)))
			return 1;
	}
	return 0;
}

/*
 * Check if should setup unmovable_isolate area
 */
static int should_setup_unmovable_isolate(struct zone *zone, long long ui_block,
           int disable)
{
	/* only set UNMOVABLE_ISOLATE in DMA zone */
	if (!is_DMA_zone(zone) || min_wmark_pages(zone) == 0)
		return 0;
	/* just setup the unmovable-isolate once when enable */
	else if (ui_block != 0 && !disable)
		return 0;
	else
		return 1;
}

#ifdef CONFIG_SMP
/*
 * flush the item stat, this function is
 * __mod_zone_page_state remove stat_threshold.
 */
static void __flush_zone_page_state(struct zone *zone, enum zone_stat_item item)
{
	struct per_cpu_pageset __percpu *pcp = zone->pageset;
	s8 __percpu *p = pcp->vm_stat_diff + item;
	long x;

	x = __this_cpu_read(*p);

	zone_page_state_add(x, zone, item);
	x = 0;
	__this_cpu_write(*p, x);
}
#else
/*
 * We do not maintain differentials in a single processor configuration.
 * The functions will do nothing.
 */
static void __flush_zone_page_state(struct zone *zone, enum zone_stat_item item)
{
	return;
}
#endif
/*
 * Mark a number of pageblocks as MIGRATE_UNMOVABLE_ISOLATE.
 */
void setup_zone_migrate_unmovable_isolate(struct zone *zone,
     int unmovable_isolate_type, int disable)
{
	unsigned long start_pfn, pfn, end_pfn, block_end_pfn;
	struct page *page;
	int block_migratetype;
	long long * zone_ui_block;
	long long unmovable_isolate_count, old_size;
	enum zone_stat_item ui_stat_item;
	int pages_moved = 0;

	/* init the different unmovable-isolate type */
	if (is_unmovable_isolate1(unmovable_isolate_type)) {
		zone_ui_block = &(zone->nr_migrate_unmovable_isolate1_block);
		ui_stat_item = NR_FREE_UNMOVABLE_ISOLATE1_PAGES;
	} else if (is_unmovable_isolate2(unmovable_isolate_type)) {
		zone_ui_block = &(zone->nr_migrate_unmovable_isolate2_block);
		ui_stat_item = NR_FREE_UNMOVABLE_ISOLATE2_PAGES;
	} else {
		pr_err("unknown unmovable isolate type!\n");
		return;
	}

	/* just setup the unmovable-isolate once when enable */
	if(!should_setup_unmovable_isolate(zone, *zone_ui_block, disable))
		return;

	if(!disable)
		unmovable_isolate_count = get_unmovable_isolate_blocks(unmovable_isolate_type);
	else
		unmovable_isolate_count = 0;

	old_size = *zone_ui_block;
	if (unmovable_isolate_count == old_size)
		return;

	*zone_ui_block = unmovable_isolate_count;

	/*
	 * Get the start pfn, end pfn and the number of blocks to unmovable_isolate
	 * We have to be careful to be aligned to pageblock_nr_pages to
	 * make sure that we always check pfn_valid for the first page in
	 * the block.
	 */
	start_pfn = zone->zone_start_pfn;
	end_pfn = zone_end_pfn(zone);
	start_pfn = roundup(start_pfn, pageblock_nr_pages);

	for (pfn = start_pfn; pfn < end_pfn; pfn += pageblock_nr_pages) {
		if (!pfn_valid(pfn))
			continue;
		page = pfn_to_page(pfn);

		/* Watch out for overlapping nodes */
		if (page_to_nid(page) != zone_to_nid(zone))
			continue;

		block_migratetype = get_pageblock_migratetype(page);

		/* Only test what is necessary when the reserves are not met */
		if (unmovable_isolate_count > 0) {
			/*
			 * Blocks with reserved pages will never free, skip
			 * them.
			 */
			block_end_pfn = min(pfn + pageblock_nr_pages, end_pfn);
			if (pageblock_is_reserved_for_ui(pfn, block_end_pfn))
				continue;

			/* If this block is unmovable_isolate, account for it */
			if (block_migratetype == unmovable_isolate_type) {
				unmovable_isolate_count--;
				continue;
			}

			/* Suitable for UNMOVABLE_ISOLATE if this block is movable */
			if (block_migratetype == MIGRATE_MOVABLE) {
				set_pageblock_migratetype(page,
							unmovable_isolate_type);
				pages_moved = move_freepages_block(zone, page,
							unmovable_isolate_type);
				__mod_zone_page_state(zone, ui_stat_item, pages_moved);
				unmovable_isolate_count--;
				continue;
			}
		} else if (!old_size) {
			/*
			 * At boot time we don't need to scan the whole zone
			 * for turning off MIGRATE_UNMOVABLE_ISOLATE.
			 */
			break;
		}

		/*
		 * If the unmovable_isolate is met and this is a previous reserved block,
		 * take it back
		 */
		if (block_migratetype == unmovable_isolate_type) {
			set_pageblock_migratetype(page, MIGRATE_MOVABLE);
			pages_moved = move_freepages_block(zone, page, MIGRATE_MOVABLE);
			__mod_zone_page_state(zone, ui_stat_item, -pages_moved);
		}
	}
	__flush_zone_page_state(zone, ui_stat_item);
}
