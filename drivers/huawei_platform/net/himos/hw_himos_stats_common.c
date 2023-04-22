/*
 * Copyright (C) Huawei Technologies Co., Ltd. 2013-2015. All rights reserved.
 *
 * mobile@huawei.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include "hw_himos_stats_common.h"
#include "hw_himos_stats_report.h"

struct list_head stats_info_head;
spinlock_t stats_info_lock;

#define		LOCAL_IP_ADDRESS		0x100007f

/*
 * Function: himos_get_stats_info_by_uid
 * Description: This function must be called when hold the lock
 * return: NULL when it have not added
 */
struct himos_stats_common *himos_get_stats_info_by_uid(__s32 uid)
{
	struct himos_stats_common *element;
	struct list_head *pos;

	list_for_each(pos, &stats_info_head) {
		element = list_entry(pos, struct himos_stats_common, node);
		if (element->uid == uid) {
			break;
		}
	}
	if (pos == &stats_info_head) {
		element = NULL;
	}

	return element;
}

void himos_stats_common_reset(struct himos_stats_common *info)
{
	if (NULL == info)
		return;

	info->portid = 0;
	INIT_LIST_HEAD(&info->node);
	info->uid = 0;
	info->type = HIMOS_STATS_TYPE_INVALID;
}

int __init himos_stats_common_init(void)
{
	INIT_LIST_HEAD(&stats_info_head);
	spin_lock_init(&stats_info_lock);

	return 0;
}
