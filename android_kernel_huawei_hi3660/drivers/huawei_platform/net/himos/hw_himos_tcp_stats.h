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

#ifndef		HW_HIMOS_TCP_STATS_H
#define		HW_HIMOS_TCP_STATS_H

#include <net/sock.h>

#include <linux/socket.h>

void himos_tcp_stats(struct sock *sk, struct msghdr *old, struct msghdr *new,
	int inbytes, int outbytes);


#endif
