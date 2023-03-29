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

#include <linux/module.h>
#include <linux/genetlink.h>
#include <net/genetlink.h>
#include <linux/printk.h>
#include <linux/skbuff.h>
#include <linux/slab.h>
#include <linux/errno.h>

#include "hw_himos_stats_common.h"
#include "hw_himos_stats_report.h"
#include "hw_himos_udp_stats.h"
#include "hw_himos_aweme_detect.h"

static int himos_stats_report_start(struct sk_buff *skb, struct genl_info *info);
static int himos_stats_report_stop(struct sk_buff *skb, struct genl_info *info);
static int himos_stats_report_keepalive(struct sk_buff *skb, struct genl_info *info);


//family
static struct genl_family himos_stats_report_genl =
{
	.id = GENL_ID_GENERATE,
	.hdrsize = 0,
	.name = HIMOS_STATS_REPORT_GENL_FAMILY,
	.version = 1,
	.maxattr = MAX_HIMOS_STATS_REPORT_ATTR,
};

//attr policy
static const struct nla_policy himos_stats_report_policy[MAX_HIMOS_STATS_REPORT_ATTR + 1] =
{
	[HIMOS_STATS_ATTR_INTERVAL] = {.type = NLA_U32},
	[HIMOS_STATS_ATTR_UID] = {.type = NLA_S32},
	[HIMOS_STATS_ATTR_INPKTS] = {.type = NLA_U32},
	[HIMOS_STATS_ATTR_OUTPKTS] = {.type = NLA_U32},
	[HIMOS_STATS_ATTR_INBYTES] = {.type = NLA_U32},
	[HIMOS_STATS_ATTR_OUTBYTES] = {.type = NLA_U32},
	[HIMOS_STATS_ATTR_TYPE] = {.type = NLA_S32},
	[HIMOS_STATS_ATTR_AWEME_CUR_DETECT_INDEX] = {.type = NLA_S32},
	[HIMOS_STATS_ATTR_AWEME_CUR_INDEX] = {.type = NLA_S32},
	[HIMOS_STATS_ATTR_AWEME_VALID_NUM] = {.type = NLA_S32},
	[HIMOS_STATS_ATTR_AWEME_DETECTING] = {.type = NLA_S32},
	[HIMOS_STATS_ATTR_AWEME_SEARCH_KEY] = {.type = NLA_BINARY, .len = sizeof(struct himos_aweme_search_key)},
	[HIMOS_STATS_ATTR_AWEME_STALL_INFO] = {.type = NLA_BINARY, .len = AWEME_STALL_WINDOW * sizeof(struct stall_info)},
};

//operations
static struct genl_ops himos_stats_report_ops[] =
{
	{
		.cmd = HIMOS_STATS_CMD_START,
		.policy = himos_stats_report_policy,
		.doit = himos_stats_report_start
	},
	{
		.cmd = HIMOS_STATS_CMD_STOP,
		.policy = himos_stats_report_policy,
		.doit = himos_stats_report_stop
	},
	{
		.cmd = HIMOS_STATS_CMD_KEEPALIVE,
		.policy = himos_stats_report_policy,
		.doit = himos_stats_report_keepalive
	},
	{
		.cmd = HIMOS_STATS_CMD_AWEME_UPDATE_STALL_INFO,
		.policy = himos_stats_report_policy,
		.doit = himos_update_aweme_stall_info
	},
};

static int himos_stats_report_start(struct sk_buff *skb, struct genl_info *info)
{
	__s32 type;
	struct nlattr *na;
	int ret = 0;

	if (info == NULL || info->attrs == NULL ||
		info->attrs[HIMOS_STATS_ATTR_TYPE] == NULL) {
		pr_err("In command START, we hope the UID and INTERVAL attr, but they are null");
		return -ENOENT;
	}

	//get the type
	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);

	switch(type)
	{
	case HIMOS_STATS_TYPE_UDP:
		ret = himos_start_udp_stats(info);
		break;
	case HIMOS_STATS_TYPE_AWEME:
	case HIMOS_STATS_TYPE_KWAI:
		ret = himos_start_aweme_detect(info);
		break;
	default:
		pr_err("receive the invalid stats type:%d", type);
		ret = -EINVAL;
		break;
	}
	if (ret < 0)
		pr_err("start statistic failed: %d", ret);

	return ret;
}

static int himos_stats_report_stop(struct sk_buff *skb, struct genl_info *info)
{
	__s32 type;
	struct nlattr *na;
	int ret = 0;

	if (info == NULL || info->attrs == NULL ||
		info->attrs[HIMOS_STATS_ATTR_TYPE] == NULL) {
		pr_err("In command STOP, we hope the TYPE attr, but it is null");
		return -ENOENT;
	}

	//get the type
	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);

	switch(type)
	{
	case HIMOS_STATS_TYPE_UDP:
		ret = himos_stop_udp_stats(info);
		break;
	case HIMOS_STATS_TYPE_AWEME:
	case HIMOS_STATS_TYPE_KWAI:
		ret = himos_stop_aweme_detect(info);
		break;
	default:
		pr_err("receive the invalid stats type:%d", type);
		ret = -EINVAL;
		break;
	}
	if (ret < 0)
		pr_err("stop statistic failed:%d", ret);

	return ret;
}


/*
 * Function: himos_stats_report_keepalive
 * Description: The command HIMOS_STATS_REPORT_CMD_KEEPALIVE handler
 * Arguments:
 *    @uid: user ID
 * return: 0--success, 1--fail
 */
static int himos_stats_report_keepalive(struct sk_buff *skb, struct genl_info *info)
{
	__s32 type;
	struct nlattr *na;
	int ret = 0;

	if (info == NULL || info->attrs == NULL ||
		info->attrs[HIMOS_STATS_ATTR_TYPE] == NULL) {
		pr_err("In command KEEPALIVE, we hope the TYPE attr, but it is null");
		return -ENOENT;
	}

	//get the uid
	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	switch(type)
	{
	case HIMOS_STATS_TYPE_UDP:
		ret = himos_keepalive_udp_stats(info);
		break;
	case HIMOS_STATS_TYPE_AWEME:
	case HIMOS_STATS_TYPE_KWAI:
		ret = himos_keepalive_aweme_detect(info);
		break;
	default:
		pr_err("receive the invalid stats type:%d", type);
		ret = -EINVAL;
		break;
	}
	if (ret < 0)
		pr_err("himos_stats_report_keepalive failed:%d", ret);

	return ret;
}

struct sk_buff* himos_get_nlmsg(__u32 portid, int type, size_t size)
{
	struct sk_buff *skb;
	void *reply;

	skb = genlmsg_new(size, GFP_ATOMIC);
	if (skb == NULL) {
		pr_err("himos_douyin_notify_result:Allocate a new SKB fail");
		return NULL;
	}

	//construct the reply message
	reply = genlmsg_put(skb, portid, 0, &himos_stats_report_genl, 0, type);
	if (reply == NULL) {
		pr_err("himos_douyin_notify_result: Bug!!! contruct the reply message error");
		nlmsg_free(skb);
		return NULL;
	}

	return skb;
}

void himos_notify_result(__u32 portid, possible_net_t net, struct sk_buff *skb)
{
	if (genlmsg_unicast(read_pnet(&net), skb, portid) < 0) {
		pr_err("Bug, send the douyin detect result to user failed");
	}
}

static int __init himos_stats_report_init(void)
{
	int err = 0;

	pr_crit("himos_stats_report_init enter()");

	err = genl_register_family_with_ops(&himos_stats_report_genl, himos_stats_report_ops);
	if (err < 0) {
		pr_err("genl_register_family_with_ops fail with error code %d", err);
		return err;
	}

	err = himos_stats_common_init();
	if (err < 0) {
		pr_err("himos_stats_common_init fail with error code %d", err);
		return err;
	}

	err = himos_aweme_init();
	if (err < 0) {
		pr_err("himos_douyin_init fail with error code %d", err);
		return err;
	}

	err = himos_udp_stats_init();
	if (err < 0) {
		pr_err("himos_udp_stats_init fail with error code %d", err);
		return err;
	}

	pr_crit("himos_stats_report_init exit()");
	return 0;
}

static void __exit himos_stats_report_exit(void)
{
	int err = 0;

	err = genl_unregister_family(&himos_stats_report_genl);
	if (err < 0) {
		pr_err("genl_unregister_family fail with error code %d", err);
	}
}

module_init(himos_stats_report_init)
module_exit(himos_stats_report_exit)
