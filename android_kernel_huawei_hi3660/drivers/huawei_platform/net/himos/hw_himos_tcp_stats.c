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
#include "hw_himos_aweme_detect.h"
#include "hw_himos_stats_report.h"
#include "hw_himos_tcp_stats.h"

void himos_tcp_stats(struct sock *sk, struct msghdr *old, struct msghdr *new,
	int inbytes, int outbytes)

{
	struct himos_stats_common *info;
	__s32 uid;
	int type;

	if (!sk)
		return;

	spin_lock_bh(&stats_info_lock);

	uid = (__s32)(sk->sk_uid.val);
	info = himos_get_stats_info_by_uid(uid);
	if (info == NULL) {
		spin_unlock_bh(&stats_info_lock);
		return;
	}
	type = info->type;
	spin_unlock_bh(&stats_info_lock);

	switch(type) {
	case HIMOS_STATS_TYPE_AWEME:
	case HIMOS_STATS_TYPE_KWAI:
		himos_aweme_tcp_stats(sk, old, new, inbytes, outbytes);
		break;
	default:
		break;
	}
}
EXPORT_SYMBOL(himos_tcp_stats);
