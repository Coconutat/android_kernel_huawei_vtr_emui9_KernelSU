/*
 * include/linux/unmovable_isolate.h
 *
 * MIGRATE_UNMOVABLE_ISOLATE function
 *
 * Copyright (C) 2017 Huawei Technologies Co., Ltd.
 */

#ifndef _UNMOVABLE_ISOLATE_H_
#define _UNMOVABLE_ISOLATE_H_

#ifdef CONFIG_HUAWEI_UNMOVABLE_ISOLATE
/* the unmovable-isolate area size setted by defconfig */
#define UNMOVABLE_ISOLATE1_SIZE_BLOCKS \
	(CONFIG_HUAWEI_UNMOVABLE_ISOLATE1_SIZE_MBYTES*SZ_1M/PAGE_SIZE/pageblock_nr_pages)
#define UNMOVABLE_ISOLATE2_SIZE_BLOCKS \
	(CONFIG_HUAWEI_UNMOVABLE_ISOLATE2_SIZE_MBYTES*SZ_1M/PAGE_SIZE/pageblock_nr_pages)

/* the allocable page order in unmovable-isolate area */
#define UNMOVABLE_ISOLATE1_MIN_ORDER 0
#define UNMOVABLE_ISOLATE1_MAX_ORDER 0
#define UNMOVABLE_ISOLATE2_MIN_ORDER 2
#define UNMOVABLE_ISOLATE2_MAX_ORDER 3


#ifdef CONFIG_HUAWEI_ENHANCED_RESERVE
/* the size and order for MIGRATE_RESERVE type */
#define ENHANCED_RESERVE_SIZE_BLOCKS \
	(CONFIG_HUAWEI_ENHANCED_RESERVE_SIZE_MBYTES*SZ_1M/PAGE_SIZE/pageblock_nr_pages)
#define RESERVE_MIN_ORDER 2
#define RESERVE_MAX_ORDER 3
#endif

extern int unmovable_isolate_disabled;

int unmovable_isolate_enabled(struct zone* zone);
int valid_order_for_ui(int order, int migratetype);
int is_DMA_zone(struct zone* zone);
int get_enhanced_reserve_size(void);
void setup_zone_migrate_unmovable_isolate(struct zone *zone, int unmovable_isolate_type, int disable);
int unmovable_isolate_pageblock(struct zone* zone, struct page* page);

#endif /* CONFIG_HUAWEI_UNMOVABLE_ISOLATE */
#endif /* _UNMOVABLE_ISOLATE_H_ */
