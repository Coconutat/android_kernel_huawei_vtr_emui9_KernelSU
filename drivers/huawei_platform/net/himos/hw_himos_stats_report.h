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

#ifndef		__HW_HIMOS_STATS_REPORT_H
#define		__HW_HIMOS_STATS_REPORT_H

//family name
#define		HIMOS_STATS_REPORT_GENL_FAMILY		"hwhimos"

//supported command
enum HIMOS_STATS_REPORT_CMD
{
	HIMOS_STATS_REPORT_CMD_UNSPEC,

    //user->kernel
	HIMOS_STATS_CMD_START,
	HIMOS_STATS_CMD_STOP,
	HIMOS_STATS_CMD_KEEPALIVE,
	HIMOS_STATS_CMD_AWEME_UPDATE_STALL_INFO,

    //kernel->user
    HIMOS_STATS_UDP_THROUGHPUT_NOTIFY,
	HIMOS_STATS_AWEME_DETECTION_NOTIFY,

	__MAX_HIMOS_STATS_REPORT_CMD
};
#define		MAX_HIMOS_STATS_REPORT_CMD		(__MAX_HIMOS_STATS_REPORT_CMD - 1)

//attr type
enum HIMOS_STATS_TYPE
{
    HIMOS_STATS_TYPE_INVALID = -1,

	HIMOS_STATS_TYPE_UDP = 1,

	HIMOS_STATS_TYPE_AWEME = 2,

	HIMOS_STATS_TYPE_KWAI = 3,

	HW_UDP_STATS_TYPE_MAX
};

#define		AWEME_SEARCH_KEY_MAX		25
struct himos_aweme_search_key
{
	char ul_key1[AWEME_SEARCH_KEY_MAX];
	char ul_key2[AWEME_SEARCH_KEY_MAX];
	char ul_key3[AWEME_SEARCH_KEY_MAX];
	char ul_key4[AWEME_SEARCH_KEY_MAX];
	char ul_key5[AWEME_SEARCH_KEY_MAX];
	char ul_key6[AWEME_SEARCH_KEY_MAX];
	char ul_key7[AWEME_SEARCH_KEY_MAX];
	char ul_key8[AWEME_SEARCH_KEY_MAX];
	char ul_key9, ul_key10, ul_key11;
	char ul_key12[AWEME_SEARCH_KEY_MAX];
	char ul_key13[AWEME_SEARCH_KEY_MAX];

	char dl_key1[AWEME_SEARCH_KEY_MAX];
	char dl_key2[AWEME_SEARCH_KEY_MAX];
	char dl_key3, dl_key4, dl_key5;
	char dl_key6[AWEME_SEARCH_KEY_MAX];
};


//supported attrs
enum HIMOS_STATS_REPOT_ATTRS
{
	HIMOS_STATS_ATTR_UNSPEC,
	HIMOS_STATS_ATTR_UID,
	HIMOS_STATS_ATTR_TYPE,
	HIMOS_STATS_ATTR_INTERVAL,
	HIMOS_STATS_ATTR_INPKTS,
	HIMOS_STATS_ATTR_OUTPKTS,
	HIMOS_STATS_ATTR_INBYTES,
	HIMOS_STATS_ATTR_OUTBYTES,
	HIMOS_STATS_ATTR_AWEME_SEARCH_KEY,
	HIMOS_STATS_ATTR_AWEME_CUR_DETECT_INDEX,
	HIMOS_STATS_ATTR_AWEME_CUR_INDEX,
	HIMOS_STATS_ATTR_AWEME_VALID_NUM,
	HIMOS_STATS_ATTR_AWEME_DETECTING,
	HIMOS_STATS_ATTR_AWEME_STALL_INFO,
	__MAX_HIMOS_STATS_REPORT_ATTR
};

#define		MAX_HIMOS_STATS_REPORT_ATTR	(__MAX_HIMOS_STATS_REPORT_ATTR - 1)

#define		STALL_KEY_MAX			40
struct stall_info
{
	unsigned short sport;
	unsigned long preload, total;
	char key[STALL_KEY_MAX];
	unsigned int inbytes;
	unsigned int local_inbytes;
	int timescale, duration;
};
#define     AWEME_STALL_WINDOW    	10


#ifdef __KERNEL__

void himos_notify_result(__u32 portid, possible_net_t net, struct sk_buff *skb);
struct sk_buff* himos_get_nlmsg(__u32 portid, int type, size_t size);

extern struct list_head stats_info_head;
extern spinlock_t stats_info_lock;

#endif

#endif
