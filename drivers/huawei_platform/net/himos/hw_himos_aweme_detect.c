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
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/ktime.h>
#include <linux/ipv6.h>
#include <linux/kernel.h>

#include <net/inet_sock.h>
#include <net/ipv6.h>
#include <net/sock.h>
#include <net/genetlink.h>

#include <hwnet/ipv4/wifipro_tcp_monitor.h>

#include "hw_himos_aweme_detect.h"
#include "hw_himos_stats_report.h"
#include "hw_himos_stats_common.h"

#undef MIN
#define MIN(a,b) (((a) > (b)) ? (b) : (a))

/*
  * When the keepalive exceed this value,
  * the kernel will delete this uid, this mechanism can keep
  * the kernel clean when the user application abnormal
  */
#define     KEEPALIVE_MAX           10

#define		BUF_LEN		2048

//timer
static struct timer_list timer_report;

static void himos_aweme_report_cb(unsigned long arg);

static void himos_log_detect_result(struct himos_aweme_detect_info *info)
{
	int i;
	
    WIFIPRO_DEBUG("print detect result-------begin");
	WIFIPRO_DEBUG("valid_num=%d, cur_index=%d, cur_detect_index=%d", info->valid_num, info->cur_index, info->cur_detect_index);
	for (i = 0; i < info->valid_num; ++i) {
		WIFIPRO_DEBUG("index=%d, video=%s, preload=%lu, total=%lu, sport=%d", i, info->stalls[i].key,
			info->stalls[i].preload, info->stalls[i].total, info->stalls[i].sport);
	}
	WIFIPRO_DEBUG("print detect result-------end");
}

static struct himos_aweme_detect_info *himos_alloc_aweme_detect_info(int flags)
{
	struct himos_aweme_detect_info *info;

	info = kmalloc(sizeof(struct himos_aweme_detect_info), flags);
	if (info == NULL) {
		WIFIPRO_WARNING("there are no enough memory space for himos_alloc_aweme_detect_info");
		return NULL;
	}
	himos_stats_common_reset(&info->comm);
	info->comm.type = HIMOS_STATS_TYPE_AWEME;

	info->interval = 0;
	info->keepalive = 0;
	info->cur_index = 0;
	info->valid_num = 0;
	info->cur_detect_index = 0;
	info->detecting = 0;
	info->local_sport = 0;
	info->local_inbytes = 0;
	return info;
}

static void himos_free_aweme_detect_info(struct himos_aweme_detect_info *info)
{
	if (NULL == info)
		return;
	kfree(info);
}

int himos_start_aweme_detect(struct genl_info *info)
{
	struct himos_aweme_detect_info *stats_info;
	__s32 uid;
	__u32 interval;
	__s32 type;
	struct nlattr *na;
	int ret = 0;

	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	if (type != HIMOS_STATS_TYPE_AWEME && type != HIMOS_STATS_TYPE_KWAI)
		return -EINVAL;

	na = info->attrs[HIMOS_STATS_ATTR_UID];
	uid = nla_get_s32(na);

	na = info->attrs[HIMOS_STATS_ATTR_INTERVAL];
	interval = nla_get_u32(na);

	spin_lock_bh(&stats_info_lock);
	stats_info = (struct himos_aweme_detect_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == stats_info) {
		stats_info = himos_alloc_aweme_detect_info(GFP_KERNEL);
		if (stats_info == NULL) {
			WIFIPRO_WARNING("there is no memory when alloc himos_aweme_detect_info");
			ret = -ENOMEM;
			goto out;
		}
		list_add(&stats_info->comm.node, &stats_info_head);
		stats_info->comm.uid = uid;
		write_pnet(&stats_info->comm.net, genl_info_net(info));
		stats_info->comm.type = type;

		del_timer_sync(&timer_report);
		timer_report.data = uid;
		timer_report.expires = jiffies + (interval * HZ / 1000);
		add_timer(&timer_report);

		na = info->attrs[HIMOS_STATS_ATTR_AWEME_SEARCH_KEY];
		memcpy(&stats_info->keys, nla_data(na), sizeof(struct himos_aweme_search_key));
	}
	stats_info->interval = interval;
	stats_info->comm.portid = info->snd_portid;
	ret = 0;

out:
	spin_unlock_bh(&stats_info_lock);
	return ret;
}

int himos_stop_aweme_detect(struct genl_info *info)
{
	struct himos_aweme_detect_info *stats_info;
	int ret = 0;
	__s32 type;
	__s32 uid;
	struct nlattr *na;

	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	if (type != HIMOS_STATS_TYPE_AWEME && type != HIMOS_STATS_TYPE_KWAI)
		return -EINVAL;

	na = info->attrs[HIMOS_STATS_ATTR_UID];
	uid = nla_get_s32(na);

	spin_lock_bh(&stats_info_lock);

	stats_info = (struct himos_aweme_detect_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == stats_info) {
		WIFIPRO_WARNING("stop a invalid uid:%d", uid);
		ret = -ENOENT;
		goto out;
	}
	list_del(&stats_info->comm.node);

	spin_unlock_bh(&stats_info_lock);
	himos_free_aweme_detect_info(stats_info);
	return 0;

out:
	spin_unlock_bh(&stats_info_lock);
	return ret;
}

int himos_keepalive_aweme_detect(struct genl_info *info)
{
	struct himos_aweme_detect_info *stats_info;
	int ret = 0;
	__s32 type;
	__s32 uid;
	struct nlattr *na;

	//get the type
	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	if (type != HIMOS_STATS_TYPE_AWEME && type != HIMOS_STATS_TYPE_KWAI)
	   return -EINVAL;

	//get the uid
	na = info->attrs[HIMOS_STATS_ATTR_UID];
	uid = nla_get_s32(na);

	spin_lock_bh(&stats_info_lock);
	stats_info = (struct himos_aweme_detect_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == stats_info) {
		WIFIPRO_WARNING("keepalive a invalid uid:%d", uid);
		ret = -ENOENT;
		goto out;
	}
	stats_info->keepalive = 0;
	ret = 0;

out:
	spin_unlock_bh(&stats_info_lock);
	return ret;
}

int himos_update_aweme_stall_info(struct sk_buff *skb, struct genl_info *info)
{
	struct himos_aweme_detect_info *stats_info;
	__s32 uid;
	__s32 type;
	struct nlattr *na;
	int detecting, cur_detect_index;
	int ret = 0;

	if (info == NULL || info->attrs == NULL ||
		info->attrs[HIMOS_STATS_ATTR_TYPE] == NULL) {
		WIFIPRO_WARNING("In command UPDATE, hope the UID and type attr, but they are null");
		return -ENOENT;
	}

	na = info->attrs[HIMOS_STATS_ATTR_TYPE];
	type = nla_get_s32(na);
	if (type != HIMOS_STATS_TYPE_AWEME && type != HIMOS_STATS_TYPE_KWAI)
		return -EINVAL;

	na = info->attrs[HIMOS_STATS_ATTR_UID];
	uid = nla_get_s32(na);

	na = info->attrs[HIMOS_STATS_ATTR_AWEME_DETECTING];
	detecting = nla_get_s32(na);

	na = info->attrs[HIMOS_STATS_ATTR_AWEME_CUR_DETECT_INDEX];
	cur_detect_index = nla_get_s32(na);
	WIFIPRO_DEBUG("update command uid=%d, type=%d, detecting=%d", uid, type, detecting);

	spin_lock_bh(&stats_info_lock);
	stats_info = (struct himos_aweme_detect_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == stats_info) {
		ret = -ENOENT;
		goto out;
	}
	WIFIPRO_DEBUG("cur_detect_index=%d, stats_info->cur_detect_index=%d",
		cur_detect_index, stats_info->cur_detect_index);
	if (cur_detect_index == stats_info->cur_detect_index)
		stats_info->detecting = detecting;
	ret = 0;

out:
	spin_unlock_bh(&stats_info_lock);
	return ret;
}

static void himos_add_stall_info(struct himos_aweme_detect_info *info)
{
	++info->cur_index;
	++info->valid_num;
	if (info->cur_index >= AWEME_STALL_WINDOW)
		info->cur_index = 0;
	if (info->valid_num > AWEME_STALL_WINDOW)
		info->valid_num = AWEME_STALL_WINDOW;
}

static void himos_aweme_report_cb(unsigned long arg)
{
	__s32 uid;
	struct himos_aweme_detect_info *info;
	__u32 portid;
	possible_net_t net;

	struct sk_buff *skb;
	size_t size = 0;
	void *reply;
	struct genlmsghdr *genlhdr;

	uid = (__s32)arg;
	spin_lock_bh(&stats_info_lock);
	info = (struct himos_aweme_detect_info *)himos_get_stats_info_by_uid(uid);
	if (NULL == info)
		goto unlock;

	portid = info->comm.portid;
	net = info->comm.net;
	//check wether the keepalive exceed the max threshold
	info->keepalive = info->keepalive + 1;
	if (info->keepalive > KEEPALIVE_MAX) {
		WIFIPRO_WARNING("aweme exceed the keepalive");
		list_del(&info->comm.node);
		himos_free_aweme_detect_info(info);
		goto unlock;
	}

	//calculate the reply message size
	size += nla_total_size(sizeof(__s32)); //uid
	size += nla_total_size(sizeof(__s32)); //cur_detect_index
	size += nla_total_size(sizeof(__s32)); //cur_index
	size += nla_total_size(sizeof(__s32)); //valid_num
	size += nla_total_size(sizeof(__s32)); //detecting
	size += nla_total_size(AWEME_STALL_WINDOW * sizeof(struct stall_info));

	skb = himos_get_nlmsg(portid, HIMOS_STATS_AWEME_DETECTION_NOTIFY, size);
	if (skb == NULL)
		goto unlock;
	//add the attrs
	if (nla_put_s32(skb, HIMOS_STATS_ATTR_UID, uid) < 0
		|| nla_put_s32(skb, HIMOS_STATS_ATTR_AWEME_CUR_DETECT_INDEX, info->cur_detect_index) < 0
		|| nla_put_s32(skb, HIMOS_STATS_ATTR_AWEME_CUR_INDEX, info->cur_index) < 0
		|| nla_put_s32(skb, HIMOS_STATS_ATTR_AWEME_VALID_NUM, info->valid_num) < 0
		|| nla_put_s32(skb, HIMOS_STATS_ATTR_AWEME_DETECTING, info->detecting) < 0
		|| nla_put(skb, HIMOS_STATS_ATTR_AWEME_STALL_INFO, AWEME_STALL_WINDOW*sizeof(struct stall_info), info->stalls) < 0) {
		WIFIPRO_WARNING("the memory error");
		nlmsg_free(skb);
		goto unlock;
	}
	genlhdr = nlmsg_data(nlmsg_hdr(skb));
	reply = genlmsg_data(genlhdr);
	genlmsg_end(skb, reply);

	spin_unlock_bh(&stats_info_lock);

	//add the timer again
	timer_report.expires = jiffies + (info->interval * HZ / 1000);
	add_timer(&timer_report);

	himos_notify_result(portid, net, skb);
	goto out;

unlock:
	spin_unlock_bh(&stats_info_lock);
out:
	return;
}

static int himos_get_total_from_range(struct himos_aweme_detect_info *info,
	struct stall_info *stall, const char *msg, int len)
{
	const char *loc1, *loc2;
	int ret = -1;
	char number[10];
	int i = 0;
	unsigned long total = 0;

	loc1 = strnstr(msg, info->keys.dl_key1, len);
	if (!loc1)
		goto out;
	loc1 += strlen(info->keys.dl_key1);
	len -= (loc1 - msg);
	if (len <= 0)
		goto out;

	loc2 = strnchr(loc1, len, info->keys.dl_key3);
	if (!loc2)
		goto out;
	len -= (loc2 - loc1 + 1);
	loc1 = loc2 + 1;
	if (len <= 0)
		goto out;
	for (i = 0; i < 9; i++) {
		if (i >= len)
			goto out;
		if (loc1[i] == info->keys.dl_key4) {
			number[i] = '\0';
			break;
		}
		number[i] = loc1[i];
	}
	if (i == 9 || kstrtoul(number, 10, &total) < 0)
		goto out;
	stall->total = total;
	ret = 0;

out:
	return ret;
}

static int himos_get_total_from_length(struct himos_aweme_detect_info *info, 
	struct stall_info *stall, const char *msg, int len)
{
	const char *loc;
	int ret = -1;
	char number[10];
	int i = 0;
	unsigned long length = 0;

	loc = strnstr(msg, info->keys.dl_key2, len);
	if (!loc)
		goto out;
	loc += strlen(info->keys.dl_key2);
	len -= (loc - msg);
	if (len <= 0)
		goto out;

	for (i = 0; i < 9; i++) {
		if (i >= len)
			goto out;
		if (loc[i] == info->keys.dl_key4) {
			number[i] = '\0';
			break;
		}
		number[i] = loc[i];
	}
	if (i == 9 || kstrtoul(number, 10, &length) < 0)
		goto out;

	stall->total = length;
	ret = 0;

out:
	return ret;
}

static void himos_reset_stall_info(struct stall_info *info)
{
	info->preload = 0;
	info->total = 0;
	info->sport = 0;
	info->key[0] = '\0';
	info->inbytes = 0;
	info->duration = 0;
	info->timescale = 0;
	info->local_inbytes = 0;
}

static int himos_paser_proxy_header(struct himos_aweme_detect_info *info,
	struct stall_info *stall, const char *msg, int len)
{
	const char *loc;
	int ret = -1;
	char number[16];
	int i = 0, temp;
	unsigned long preload = 0;

	temp = len;
	if ((loc = strnstr(msg, info->keys.ul_key5, temp))) {
		loc += strlen(info->keys.ul_key5);
		temp -= (loc - msg);
		if (temp <= 0)
			goto out;
		for (i = 0; (i < 15) && (i < temp); i++) {
			if (loc[i] == info->keys.ul_key10) {
				number[i] = '\0';
				break;
			}
			number[i] = loc[i];
		}
		number[15] = '\0';
		if (i == 15 || kstrtoul(number, 10, &preload) < 0)
			goto out;
		stall->preload = preload;
	}

	temp = len;
	if ((loc = strnstr(msg, info->keys.ul_key6, temp))) {
		loc += strlen(info->keys.ul_key6);
		temp -= (loc - msg);
		if (temp <= 0)
			goto out;
		for (i = 0; (i < STALL_KEY_MAX-1) && (i < temp); ++i) {
			if (loc[i] == info->keys.ul_key11) {
				stall->key[i] = '\0';
				break;
			}
			stall->key[i] = loc[i];
		}
		if (likely(i > 0)) {
			stall->key[i] = '\0';
			ret = 0;
			WIFIPRO_DEBUG("detect the key: %s", stall->key);
		}
	}

out:
	return ret;
}

static int himos_paser_kwai_proxy_header(struct himos_aweme_detect_info *info,
	struct stall_info *stall, const char *msg, int len)
{
	const char *loc;
	int ret = -1;
	char number[16];
	int i = 0, temp;

	temp = len;
	if ((loc = strnstr(msg, info->keys.ul_key8, temp))) {
		loc += strlen(info->keys.ul_key8);
		temp -= (loc - msg);
		if (temp <= 0)
			goto out;
		for (i = 0; (i < STALL_KEY_MAX-1) && (i < temp); ++i) {
			if (loc[i] == info->keys.ul_key9) {
				stall->key[i] = '\0';
				break;
			}
			stall->key[i] = loc[i];
		}
		if (likely(i > 0)) {
			stall->key[i] = '\0';
			ret = 0;
			WIFIPRO_DEBUG("detect the key: %s", stall->key);
		}
	}

out:
	return ret;
}

static int himos_paser_preload_header(struct himos_aweme_detect_info *info,
	struct stall_info *stall, const char *msg, int len)
{
	const char *loc;
	int ret = -1;
	int i = 0;

	if ((loc = strnstr(msg, info->keys.ul_key6, len))) {
		loc += strlen(info->keys.ul_key6);
		len -= (loc - msg);
		if (len <= 0)
			goto out;
		for (i = 0; (i < STALL_KEY_MAX-1) && (i < len); ++i) {
			if (loc[i] == info->keys.ul_key11) {
				stall->key[i] = '\0';
				break;
			}
			stall->key[i] = loc[i];
		}
		if (likely(i > 0)) {
			stall->key[i] = '\0';
			ret = 0;
			WIFIPRO_DEBUG("detect the key: %s", stall->key);
		}
	}

out:
	return ret;
}

static void himos_process_proxy_request(struct himos_aweme_detect_info *info,
	struct sock *sk, const char *msg, int len)
{
	int i;
	struct stall_info stall;

	himos_reset_stall_info(&stall);
	stall.sport = htons(sk->sk_num);

	switch(info->comm.type) {
	case HIMOS_STATS_TYPE_AWEME:
		if (himos_paser_proxy_header(info, &stall, msg, len) < 0) {
			WIFIPRO_DEBUG("paser the aweme proxy header failed, ignore this request");
			return;
		}
		break;
	case HIMOS_STATS_TYPE_KWAI:
		if (himos_paser_kwai_proxy_header(info, &stall, msg, len) < 0) {
			WIFIPRO_DEBUG("paser the proxy kwai header failed, ignore this request");
			return;
		}
		break;
	default:
		return;
	}

	for (i = 0; i < info->valid_num; ++i) {
		if (!strncmp(info->stalls[i].key, stall.key, STALL_KEY_MAX))
			break;
	}
	if (unlikely(i >= info->valid_num)) {
		WIFIPRO_DEBUG("NOTE: we find the proxy request, but without exist key");
		return;
	}
	WIFIPRO_DEBUG("proxy request and find the exist information(index:%d, proxy preload=%lu, preload=%lu)",
		i, stall.preload, info->stalls[i].preload);
	info->stalls[i].sport = stall.sport;
	WIFIPRO_DEBUG("sport=%d, snum=%d", info->stalls[i].sport, sk->sk_num);
	himos_log_detect_result(info);
}

static void himos_process_preload_request(struct himos_aweme_detect_info *info,
	struct sock *sk, const char *msg, int len)
{
	struct stall_info stall;
	int i;

	himos_reset_stall_info(&stall);
	stall.sport = htons(sk->sk_num);

	if (himos_paser_preload_header(info, &stall, msg, len) < 0) {
		WIFIPRO_DEBUG("paser the preload header failed, ignore this request");
		return;
	}

	for (i = 0; i < info->valid_num; ++i) {
		if (!strncmp(info->stalls[i].key, stall.key, STALL_KEY_MAX))
			break;
	}
	WIFIPRO_DEBUG("get a preload request, think it as not stall firstly");

	if (likely(i >= info->valid_num)) {
		memcpy(&info->stalls[info->cur_index], &stall, sizeof(struct stall_info));
		info->stalls[info->cur_index].sport = htons(sk->sk_num);
		himos_add_stall_info(info);
	}
	else {
		WIFIPRO_DEBUG("GET a preload request and it's a repeat one");
		info->stalls[i].sport = stall.sport;
	}

	himos_log_detect_result(info);
}

static void himos_process_local_request(struct himos_aweme_detect_info *info,
	struct sock *sk, const char *msg, int len)
{
	const char *loc;
	int i = 0;
	char key[STALL_KEY_MAX];

	if (!(loc = strnstr(msg, info->keys.ul_key8, len)))
		return;
	loc += strlen(info->keys.ul_key8);
	len -= (loc - msg);
	if (len <= 0)
		return;
	for (i = 0; (i < STALL_KEY_MAX-1) && (i < len); ++i) {
		if (loc[i] == info->keys.ul_key9) {
			key[i] = '\0';
			break;
		}
		key[i] = loc[i];
	}
	if (i <= 0)
		return;
	key[i] = '\0';

	for (i = 0; i < info->valid_num; ++i) {
		if (!strncmp(info->stalls[i].key, key, STALL_KEY_MAX))
			break;
	}

	WIFIPRO_DEBUG("GET a local request");
	if (i >= info->valid_num) {
		WIFIPRO_DEBUG("a new local GET request, index=%d", info->cur_index);
		i = info->cur_index;
		himos_add_stall_info(info);
		himos_reset_stall_info(&info->stalls[i]);
		memcpy(&info->stalls[i].key, key, STALL_KEY_MAX);
	}
	info->stalls[i].local_inbytes = 0;
	info->detecting = 1;
	info->cur_detect_index = i;
	info->local_sport = htons(sk->sk_num);
	info->local_inbytes = 0;

	himos_log_detect_result(info);
}

static void himos_get_duration(struct himos_aweme_detect_info *info,
	struct stall_info *stall, const char *msg, int len)
{
#define		KEY_SHIFT	12
	const char *loc;
	
	loc = strnstr(msg, info->keys.dl_key6, len);
	if (!loc)
		return;
	loc += strlen(info->keys.dl_key6);
	len -= (loc - msg + KEY_SHIFT + 8);
	if (len < 0)
		return;
	loc += KEY_SHIFT;
	stall->timescale = (loc[0] << 24) |
				(loc[1] << 16) |
				(loc[2] << 8)  |
				(loc[3] << 0);
	stall->duration = (loc[4] << 24) |
			   (loc[5] << 16) |
			   (loc[6] << 8)  |
			   (loc[7] << 0);
	WIFIPRO_DEBUG("the druation=%d, timescale=%d", stall->duration, stall->timescale);
#undef KEY_SHIFT
}

static void himos_process_response(struct himos_aweme_detect_info *info,
	struct stall_info *stall, const char *msg, int len)
{
	if (!stall->total) {
		if (stall->preload > 0)
			himos_get_total_from_range(info, stall, msg, len);
		else
			himos_get_total_from_length(info, stall, msg, len);
	}
	if (!stall->duration) {
		himos_get_duration(info, stall, msg, len);
	}
}

/*
 * The caller must assure the buf's length is enough(greater than BUF_LEN)
 */
static int himos_copy_from_msg(char *buf, struct msghdr *msg)
{
	struct iovec *iov = NULL;
	int len = -1, ret;

	iov = msg->msg_iter.iov;
	if (NULL != iov && iov->iov_len > 0) {
		len = MIN(BUF_LEN, iov->iov_len);
		ret = copy_from_user(buf, (char *)(iov->iov_base), len);
		len -= ret;
	}

	return len;
}

void himos_aweme_tcp_stats(struct sock *sk, struct msghdr *old, struct msghdr *new, int inbytes, int outbytes)
{
	struct himos_aweme_detect_info *info;
	struct stall_info *stall = NULL;
	int i, j;
	char buffer[BUF_LEN];
	int len = -1;
	__s32 uid;

	if (outbytes <= 0 && inbytes <= 0)
		return;
	if (outbytes > 0 && NULL == new)
		return;
	if (inbytes > 0 && (NULL == new || NULL == old))
		return;

	//copy the data
	if (outbytes > 0) {
		if ((new->msg_iter.type == WRITE) && (new->msg_iter.nr_segs > 0)) {
			len = himos_copy_from_msg(buffer, new);
		}
	}
	else if (inbytes > 0) {
		if (old->msg_iter.type == READ && old->msg_iter.nr_segs > 0) {
			len = himos_copy_from_msg(buffer, old);
		}
	}
	if (len <= 0)
		return;

	//hold the info lock
	spin_lock_bh(&stats_info_lock);
	uid = (__s32)(sk->sk_uid.val);
	info = (struct himos_aweme_detect_info *)himos_get_stats_info_by_uid(uid);
	if (info == NULL)
		goto out;

	if (outbytes > 0) {
		if (strnstr(buffer, info->keys.ul_key1, MIN(10, len))) {
			if (HIMOS_STATS_TYPE_AWEME == info->comm.type) {
				if (strnstr(buffer, info->keys.ul_key2, len)) {
					if (strnstr(buffer, info->keys.ul_key3, len)) {
						himos_process_proxy_request(info, sk, buffer, len);
					}
					else if (strnstr(buffer, info->keys.ul_key4, len)) {
						himos_process_preload_request(info, sk, buffer, len);
					}
				}
				else if (strnstr(buffer, info->keys.ul_key7, len)) {
					himos_process_local_request(info, sk, buffer, len);
				}
			}
			else if (HIMOS_STATS_TYPE_KWAI == info->comm.type) {
				if (strnstr(buffer, info->keys.ul_key12, len)) {
					if (strnstr(buffer, info->keys.ul_key13, len)) {
						himos_process_local_request(info, sk, buffer, len);
					}
					himos_process_proxy_request(info, sk, buffer, len);
				}
			}
		}
	}

	if (inbytes > 0) {
		if (!info->local_inbytes && info->local_sport == htons(sk->sk_num)) {
			info->local_inbytes = inbytes;
			info->stalls[info->cur_detect_index].local_inbytes = info->local_inbytes;
			WIFIPRO_DEBUG("add inbytes=%u for local request(sport=%d)", inbytes, info->local_sport);
		}
		for (i = 0, j = info->cur_index; i < info->valid_num; ++i) {
			if (--j < 0)
				j = AWEME_STALL_WINDOW - 1;
			if (info->stalls[j].sport == htons(sk->sk_num)) {
				if (!stall)
					stall = &info->stalls[j];
				if (info->cur_detect_index == j){
					info->stalls[j].inbytes += inbytes;
					WIFIPRO_DEBUG("add inbytes=%u for proxy(sport=%d)", inbytes, stall->sport);
				}
				else {
					info->stalls[j].preload += inbytes;
					WIFIPRO_DEBUG("add inbytes=%u for preload(sport=%d)", inbytes, stall->sport);
				}
			}
		}
		if (!stall)
			goto out;

		if (!stall->duration || !stall->total || !stall->timescale) {
			himos_process_response(info, stall, buffer, len);
		}
	}
out:
	spin_unlock_bh(&stats_info_lock);
}

int __init himos_aweme_init(void)
{
	init_timer(&timer_report);
	timer_report.function = himos_aweme_report_cb;
	return 0;
}
