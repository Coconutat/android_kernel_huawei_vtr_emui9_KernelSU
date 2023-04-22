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

#ifndef     HW_HIMOS_AWEME_DETECT_H
#define     HW_HIMOS_AWEME_DETECT_H

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/genetlink.h>

#include <net/genetlink.h>

#include "hw_himos_stats_report.h"
#include "hw_himos_stats_common.h"

struct himos_aweme_detect_info
{
	//this is must be the first member
	struct himos_stats_common comm;

	__u32 interval;
	int keepalive;

	int detecting, cur_detect_index;
	struct stall_info stalls[AWEME_STALL_WINDOW];
	int cur_index, valid_num;

	unsigned short local_sport;
	unsigned int local_inbytes;

	struct himos_aweme_search_key keys;
};

int himos_start_aweme_detect(struct genl_info *info);
int himos_stop_aweme_detect(struct genl_info *info);
int himos_keepalive_aweme_detect(struct genl_info *info);
int himos_update_aweme_stall_info(struct sk_buff *skb, struct genl_info *info);
void himos_aweme_tcp_stats(struct sock *sk, struct msghdr *old, struct msghdr *new, int inbytes, int outbytes);



int himos_aweme_init(void) __init;

#endif
