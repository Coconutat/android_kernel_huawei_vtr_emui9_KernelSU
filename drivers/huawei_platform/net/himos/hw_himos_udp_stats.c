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

#include <linux/genetlink.h>
#include <net/genetlink.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <net/sock.h>

#include "hw_himos_stats_report.h"
#include "hw_himos_udp_stats.h"

#include "hw_himos_stats_common.h"

/*
  * When the udp_stats_info.keepalive exceed this value,
  * the kernel will delete this uid, this mechanism can keep
  * the kernel clean when the user application abnormal
  */
#define     KEEPALIVE_MAX       10

struct himos_udp_stats_info
{
	//this is must the first member
	struct himos_stats_common comm;

	__u32 interval;

	__u32 inPkts;
	__u32 outPkts;
	__u32 inBytes;
	__u32 outBytes;

	int keepalive;
};

//timer
static struct timer_list timer_report;

static void himos_udp_stats_cb(unsigned long arg);

/*
 * Function: himos_alloc_udp_stats_info
 * Description: Allocate the struct stats_info node and init it.
 * return: the new stats info pointer or NULL when failed
 */
static struct himos_udp_stats_info *himos_alloc_udp_stats_info(int flags)
{
	struct himos_udp_stats_info *info;

	info = kmalloc(sizeof(struct himos_udp_stats_info), flags);
	if (info == NULL) {
		pr_err("There are no enough memory space for himos_udp_stats_info");
		return NULL;
	}
	himos_stats_common_reset(&info->comm);
	info->comm.type = HIMOS_STATS_TYPE_UDP;

	info->inPkts = 0;
	info->outPkts = 0;
	info->inBytes = 0;
	info->outBytes = 0;
	info->interval = 0;
	info->keepalive = 0;

	return info;
}

/*
 * Function: himos_free_udp_stats_info
 * Description: Free the struct stats_info.
 * return: None
 */
static void himos_free_udp_stats_info(struct himos_udp_stats_info *info)
{
	if (NULL == info)
		return;

	kfree(info);
}

/*
 * Function: himos_udp_stats_cb
 * Description: The timer callback
 *      NOTE: this function maybe called nested, so it must be reentrant
 * Arguments:
 *  @arg: the uid
 * return: NONE
 */
static void himos_udp_stats_cb(unsigned long arg)
{
	__s32 uid;
	__u32 portid;
	struct himos_udp_stats_info *info;
	possible_net_t net;

	struct sk_buff *skb;
	size_t size = 0;
	void *reply;
	struct genlmsghdr *genlhdr;

	spin_lock_bh(&stats_info_lock);
	uid = (__s32)arg;
	info = (struct himos_udp_stats_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == info)
		goto unlock;

	info->keepalive = info->keepalive + 1;
	if (info->keepalive > KEEPALIVE_MAX) {
		list_del(&info->comm.node);
		himos_free_udp_stats_info(info);
		goto unlock;
	}

	portid = info->comm.portid;
	net = info->comm.net;

	//calculate the reply message size
	size += nla_total_size(sizeof(__s32)); //uid attr
	size += nla_total_size(sizeof(__u32)); //inPkts attr
	size += nla_total_size(sizeof(__u32)); //outPkts attr
	size += nla_total_size(sizeof(__u32)); //inBytes attr
	size += nla_total_size(sizeof(__u32)); //outBytes attr

	skb = himos_get_nlmsg(portid, HIMOS_STATS_UDP_THROUGHPUT_NOTIFY, size);
	if (skb == NULL)
		goto unlock;

	//add the attrs
	if (nla_put_s32(skb, HIMOS_STATS_ATTR_UID, uid) < 0
		|| nla_put_u32(skb, HIMOS_STATS_ATTR_INPKTS, info->inPkts) < 0
		|| nla_put_u32(skb, HIMOS_STATS_ATTR_OUTPKTS, info->outPkts) < 0
		|| nla_put_u32(skb, HIMOS_STATS_ATTR_INBYTES, info->inBytes) < 0
		|| nla_put_u32(skb, HIMOS_STATS_ATTR_OUTBYTES, info->outBytes) < 0) {
		pr_err("Bug, the memory error");
		nlmsg_free(skb);
		goto unlock;
	}
	//send the message
	genlhdr = nlmsg_data(nlmsg_hdr(skb));
	reply = genlmsg_data(genlhdr);
	genlmsg_end(skb, reply);

	//clear counter
	info->inPkts= 0;
	info->outPkts= 0;
	info->inBytes = 0;
	info->outBytes = 0;

	timer_report.expires = jiffies + (info->interval * HZ / 1000);
	add_timer(&timer_report);

	spin_unlock_bh(&stats_info_lock);

	himos_notify_result(portid, net, skb);
	goto out;

unlock:
	spin_unlock_bh(&stats_info_lock);
out:
	return;
}

int himos_start_udp_stats(struct genl_info *info)
{
	struct himos_udp_stats_info *stats_info;
	__s32 uid;
	__u32 interval;
	__s32 type;
	struct nlattr *na;
	int ret = 0;

	//get the type
	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	if (type != HIMOS_STATS_TYPE_UDP)
		return -EINVAL;

	//get the uid
	na = info->attrs[HIMOS_STATS_ATTR_UID];
	uid = nla_get_s32(na);

	//get the interval
	na = info->attrs[HIMOS_STATS_ATTR_INTERVAL];
	interval = nla_get_u32(na);

	spin_lock_bh(&stats_info_lock);
	stats_info = (struct himos_udp_stats_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == stats_info) {
		stats_info = himos_alloc_udp_stats_info(GFP_KERNEL);
		if (stats_info == NULL) {
			pr_err("There is no memory when alloc stats_info");
			ret = -ENOMEM;
			goto out;
		}
		//add the stats_info to global list
		list_add(&stats_info->comm.node, &stats_info_head);
		stats_info->comm.uid = uid;

		del_timer_sync(&timer_report);

		//add the timer
		timer_report.data = uid;
		timer_report.expires = jiffies + (interval * HZ / 1000);
		add_timer(&timer_report);
	}
	stats_info->comm.type = HIMOS_STATS_TYPE_UDP;
	stats_info->interval = interval;
	stats_info->comm.portid = info->snd_portid;
	stats_info->keepalive = 0;
	write_pnet(&stats_info->comm.net, genl_info_net(info));
	ret = 0;

out:
	spin_unlock_bh(&stats_info_lock);
	return ret;
}

/*
 * Function: himos_stop_udp_stats
 * Description: stop udp statistic
 * return: 0: success, -1: when the uid is invalid
 */
int himos_stop_udp_stats(struct genl_info *info)
{
	struct himos_udp_stats_info *stats_info;
	int ret = 0;
	__s32 type;
	__s32 uid;
	struct nlattr *na;

	//get the type
	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	if (type != HIMOS_STATS_TYPE_UDP)
		return -EINVAL;

	//get the uid
	na = info->attrs[HIMOS_STATS_ATTR_UID];
	uid = nla_get_s32(na);

	spin_lock_bh(&stats_info_lock);

	stats_info = (struct himos_udp_stats_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == stats_info) {
		pr_err("BUG: stop a invalid uid:%d", uid);
		ret = -ENOENT;
		goto out;
	}
	//delete the stats from the list
	list_del(&stats_info->comm.node);
	spin_unlock_bh(&stats_info_lock);

	//free the stats memory
	himos_free_udp_stats_info(stats_info);
	return 0;

out:
	spin_unlock_bh(&stats_info_lock);
	return ret;
}

int himos_keepalive_udp_stats(struct genl_info *info)
{
	struct himos_udp_stats_info *stats_info;
	int ret = 0;
	__s32 type;
	__s32 uid;
	struct nlattr *na;

	//get the type
	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	if (type != HIMOS_STATS_TYPE_UDP)
		return -EINVAL;

	//get the uid
	na = info->attrs[HIMOS_STATS_ATTR_UID];
	uid = nla_get_s32(na);

	spin_lock_bh(&stats_info_lock);

	stats_info = (struct himos_udp_stats_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == stats_info) {
		pr_err("BUG: keepalive a invalid uid:%d", uid);
		ret = -ENOENT;
		goto out;
	}
	stats_info->keepalive = 0;
	ret = 0;

out:
	spin_unlock_bh(&stats_info_lock);
	return ret;
}

/*
 * Function: himos_udp_stats
 * Description: The himos_udp_stats callback when the UDP packet is sent or received
 * Arguments:
 *  @sk: socket controller block
 *    @inbytes&&outbytes: the size of this UDP packet
 * return: NONE
 */
void himos_udp_stats(struct sock *sk, __u32 inbytes, __u32 outbytes)
{
	struct himos_udp_stats_info *info;
	__s32 uid;

	if (!sk)
		return;
	uid = (__s32)(sk->sk_uid.val);

	spin_lock_bh(&stats_info_lock);
	info = (struct himos_udp_stats_info *)himos_get_stats_info_by_uid(uid);
	if (info != NULL && info->comm.type == HIMOS_STATS_TYPE_UDP) {
		if (inbytes > 0) {
		   info->inPkts= info->inPkts+ 1;
		   info->inBytes += (inbytes + 44);
		}
		if (outbytes > 0) {
		   info->outPkts= info->outPkts+ 1;
		   info->outBytes += (outbytes - 44);
		}
	}
	spin_unlock_bh(&stats_info_lock);
}

int __init himos_udp_stats_init(void)
{
	init_timer(&timer_report);
	timer_report.function = himos_udp_stats_cb;

	return 0;
}
