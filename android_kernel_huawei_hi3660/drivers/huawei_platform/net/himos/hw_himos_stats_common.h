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

#ifndef HW_HIMOS_STATS_COMMON_H
#define HW_HIMOS_STATS_COMMON_H

#include <linux/types.h>
#include <linux/list.h>
#include <net/net_namespace.h>
#include <net/sock.h>
#include <linux/ipv6.h>
#include <net/ipv6.h>
#include <linux/ktime.h>


struct himos_stats_common
{
	__u32 portid;   //user portid
	possible_net_t net;  //for reply

	struct list_head node;

	//the value of 'enum HIMOS_STATS_TYPE'
	int type;
	__s32 uid;
};

int himos_stats_common_init(void) __init;

struct himos_stats_common *himos_get_stats_info_by_uid(__s32 uid);
void himos_stats_common_reset(struct himos_stats_common *info);

#endif
