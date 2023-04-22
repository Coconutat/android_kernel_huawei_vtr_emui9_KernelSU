
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/time.h>
#include <linux/kernel.h>/* add for log */
#include <linux/ctype.h>/* add for tolower */
#include <linux/spinlock.h>/* add for spinlock */
#include <linux/netlink.h>/* add for thread */
#include <uapi/linux/netlink.h>/* add for netlink */
#include <linux/kthread.h>/* add for thread */
#include <linux/jiffies.h>/* add for jiffies */
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <net/sock.h>
#include <linux/skbuff.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/version.h>
#include <linux/ktime.h>
#include <linux/timekeeping.h>

#include "wbc_hw_hook.h"
#include "chr_netlink.h"
//#include "../net/netbooster/video_acceleration.h"

#ifndef DEBUG
#define DEBUG
#endif

/*This is to record the local in page information*/
static struct http_stream http_para_in;
/*This is to record the local out page information*/
static struct http_stream http_para_out;
/*The structure in order to record a different page stream with a hash index*/
static struct http_stream *stream_list;
/*This structure stores the statistics of web pages*/
static struct http_return rtn_stat[RNT_STAT_SIZE];
/*Return the abnomal infomation*/
static struct http_return rtn_abn[RNT_STAT_SIZE];
static struct rtt_from_stack stack_rtt[RNT_STAT_SIZE];
static unsigned int sleep_flag;

/*The HTTP keyword is used to filter tcp packets*/
static char g_get_str[] = {'G', 'E', 'T', 0, 0};
static char g_http_str[] = {'H', 'T', 'T', 'P', 0};
static char g_post_str[] = {'P', 'O', 'S', 'T', 0};

/*These parameters are used to store the forbid time*/
static unsigned long rpt_stamp;
static unsigned long abn_stamp_no_ack;
static unsigned long abn_stamp_rtt_large;
static unsigned long abn_stamp_web_fail;
static unsigned long abn_stamp_web_delay;
static unsigned long abn_stamp_syn_no_ack;

static bool rtt_flag[RNT_STAT_SIZE];
static bool web_deley_flag[RNT_STAT_SIZE];

/*tcp protocol use this semaphone to inform chr netlink thread*/
static struct semaphore g_web_stat_sync_sema;
static struct timer_list g_web_stat_timer;
static struct task_struct *g_web_stat_task;
static struct chr_key_val g_chr_key_val;

/*This parameters lock are used to lock the common parameters*/
static spinlock_t g_web_stat_lock;
static spinlock_t g_web_para_in_lock;
static spinlock_t g_web_para_out_lock;

static unsigned long *abn_stamp_list_syn_no_ack;
static unsigned long *abn_stamp_list_web_no_ack;
static unsigned long *abn_stamp_list_web_fail;
static unsigned long *abn_stamp_list_web_delay;
static unsigned long *abn_stamp_list_tcp_rtt_large;
static int abn_stamp_list_syn_no_ack_idx;
static int abn_stamp_list_web_no_ack_idx;
static int abn_stamp_list_web_fail_idx;
static int abn_stamp_list_web_delay_idx;
static int abn_stamp_list_tcp_rtt_large_idx;

static unsigned long abnomal_stamp_list_syn_no_ack_update(
	unsigned long time_stamp);
static unsigned long abnomal_stamp_list_web_no_ack_update(
	unsigned long time_stamp);
static unsigned long abnomal_stamp_list_web_fail_update(
	unsigned long time_stamp);
static unsigned long abnomal_stamp_list_web_delay_update(
	unsigned long time_stamp);
static unsigned long abnomal_stamp_list_tcp_rtt_large_update(
	unsigned long time_stamp);
static void abnomal_stamp_list_syn_no_ack_print_log(void);

static void save_app_syn_succ(u32 uid, u8 interface_type);
static void save_app_web_no_ack(u32 uid, u8 interface_type);
static void save_app_web_delay(u32 uid, int web_delay, u8 interface_type);
static void save_app_web_fail(u32 uid, u8 interface_type);
static void save_app_tcp_rtt(u32 uid, u32 tcp_rtt, u8 interface_type);
static u32 s_report_app_uid_lst[CHR_MAX_REPORT_APP_COUNT] = {0};
static int data_reg_tech = 0;
static int RIL_RADIO_TECHNOLOGY_LTE = 13;
static int RIL_RADIO_TECHNOLOGY_LTE_CA = 19;

static uid_t get_uid_from_sock(struct sock *sk);
static uid_t get_des_addr_from_sock(struct sock *sk);
static u32 http_response_code(char *pstr);
static void web_delay_rtt_flag_reset(void);
#ifdef CONFIG_HW_NETBOOSTER_MODULE
static void video_chr_stat_report(void);
extern int chr_video_stat(struct video_chr_para *report);
#endif

/*us convert to ms*/
u32 us_cvt_to_ms(u32 seq_rtt_us)
{
	return seq_rtt_us/1000;
}

/*To notify thread to update rtt*/
void notify_chr_thread_to_update_rtt(u32 seq_rtt_us, struct sock *sk, u8 data_net_flag)
{
	u8 interface_type;
	if (seq_rtt_us <= 0)
		return;

	if (!spin_trylock_bh(&g_web_stat_lock))
		return;
	if(data_net_flag) {
		interface_type = RMNET_INTERFACE;
	}
	else {
		interface_type = WLAN_INTERFACE;
	}
	if (seq_rtt_us < MAX_RTT) {

		stack_rtt[interface_type].tcp_rtt = us_cvt_to_ms(seq_rtt_us);
		stack_rtt[interface_type].is_valid = IS_USE;
		stack_rtt[interface_type].uid = get_uid_from_sock(sk);
		stack_rtt[interface_type].rtt_dst_addr = get_des_addr_from_sock(sk);
	}

    spin_unlock_bh(&g_web_stat_lock);
    up(&g_web_stat_sync_sema);

}

/*Update protocol stack buffer information*/
void chr_update_buf_time(s64 time, u32 protocal)
{
	ktime_t kt;
	s64 buff;
	s64 curBuf;
	unsigned long jif;
	long difJif;

	if (time == 0)
		return;

	jif = jiffies;
	switch (protocal)
	{
	case SOL_TCP:
		kt = ktime_get_real();
		difJif = (long)(jif - g_chr_key_val.tcp_last);
		curBuf = kt.tv64 - time;
		if (curBuf < 0)
			curBuf = 0;

		if (difJif > FILTER_TIME_LIMIT) {
			atomic_set(&g_chr_key_val.tcp_buf_time, curBuf);
		} else {
			buff = atomic_read(&g_chr_key_val.tcp_buf_time);
			buff = buff - buff / ALPHA_FILTER_PARA + curBuf / ALPHA_FILTER_PARA;
			atomic_set(&g_chr_key_val.tcp_buf_time, buff);
		}

		g_chr_key_val.tcp_last = jif;
		break;
	case SOL_UDP:
		kt = ktime_get_real();
		difJif = (long)(jif - g_chr_key_val.udp_last);
		curBuf = kt.tv64 - time;
		if (curBuf < 0)
			curBuf = 0;

		if (difJif > FILTER_TIME_LIMIT) {
			 atomic_set(&g_chr_key_val.udp_buf_time, curBuf);
		} else {
			buff = atomic_read(&g_chr_key_val.udp_buf_time);
			buff = buff - buff / ALPHA_FILTER_PARA + curBuf / ALPHA_FILTER_PARA;
			atomic_set(&g_chr_key_val.udp_buf_time, buff);
		}

		g_chr_key_val.udp_last = jif;
		break;
	default:
		break;
	}
}

/*This is the buffer time update function of the TCP/IP protocol stack,
* which is passively obtained from the upper layer.*/
static u32 reportBuf(void)
{
	u16 tmpBuf;
	u32 bufRtn = 0;
	s64 buf64;
	unsigned long jif;
	long difJif;

	jif = jiffies;

	buf64 = atomic_read(&g_chr_key_val.udp_buf_time);
	tmpBuf = (u16)(buf64 / NS_CONVERT_TO_MS);
	if (buf64 > ((s64)MAX_VALID_U16 * (s64)NS_CONVERT_TO_MS))
		tmpBuf = MAX_VALID_U16;

	difJif = (long)(jif - g_chr_key_val.udp_last);
	if (difJif > 2*HZ || difJif < -2*HZ)
		tmpBuf = 0;

	bufRtn = tmpBuf;

	buf64 = atomic_read(&g_chr_key_val.tcp_buf_time);
	tmpBuf = (u16)(buf64 / NS_CONVERT_TO_MS);
	if (buf64 > ((s64)MAX_VALID_U16 *(s64)NS_CONVERT_TO_MS))
		tmpBuf = MAX_VALID_U16;

	difJif = (long)(jif - g_chr_key_val.tcp_last);
	if (difJif > 2*HZ || difJif < -2*HZ)
		tmpBuf = 0;

	bufRtn = tmpBuf + (bufRtn << 16);

	return bufRtn;
}

/*timer's expired process function.
* In this function, the time-out data stream is discarded
* and the statistics are reported periodically.*/
static void web_stat_timer(unsigned long data)
{
	u32 hashcnt;
	int hashNum = 0;
	unsigned long abn_stamp;
	u8 interface_type;
	spin_lock_bh(&g_web_stat_lock);

	for (hashcnt = 0; hashcnt < HASH_MAX; hashcnt++) {

		if (stream_list[hashcnt].is_valid != IS_USE)
			continue;
		interface_type = stream_list[hashcnt].interface;
		if (stream_list[hashcnt].type == HTTP_GET &&
		time_after(jiffies,
		stream_list[hashcnt].get_time_stamp + EXPIRE_TIME)) {
			rtn_stat[interface_type].total_num++;
			rtn_stat[interface_type].no_ack_num++;
			stream_list[hashcnt].is_valid = IS_UNUSE;

			save_app_web_no_ack(stream_list[hashcnt].uid, interface_type);

			abn_stamp =
				abnomal_stamp_list_web_no_ack_update(jiffies);
			if (time_after(jiffies, abn_stamp_no_ack) &&
				time_before(jiffies,
				abn_stamp + WEB_NO_ACK_REPORT_TIME)){

				rtn_abn[interface_type].report_type = WEB_NO_ACK;
				rtn_abn[interface_type].uid = stream_list[hashcnt].uid;
				rtn_abn[interface_type].http_resp = 0xffffffff;
				spin_unlock_bh(&g_web_stat_lock);
				chr_notify_event(CHR_WEB_STAT_EVENT,
					g_user_space_pid, 0, rtn_abn);
				pr_info("chr: no ack report s:%x-d:%x>:%x\n",
				stream_list[hashcnt].src_addr & IPV4ADDR_MASK,
				stream_list[hashcnt].dst_addr & IPV4ADDR_MASK,
				stream_list[hashcnt].tcp_port);
				spin_lock_bh(&g_web_stat_lock);
				memset(&rtn_abn, 0, sizeof(rtn_abn));
				abn_stamp_no_ack =
					jiffies + FORBID_TIME;

			}
		}

		if (stream_list[hashcnt].type == HTTP_SYN &&
		time_after(jiffies,
		stream_list[hashcnt].time_stamp + DELETE_TIME)) {

			abn_stamp =
				abnomal_stamp_list_syn_no_ack_update(jiffies);
			if (time_after(jiffies, abn_stamp_syn_no_ack) &&
				time_before(jiffies,
				abn_stamp + SYN_NO_ACK_REPORT_TIME)) {

				rtn_abn[interface_type].report_type = SYN_NO_ACK;
				rtn_abn[interface_type].uid = stream_list[hashcnt].uid;
				spin_unlock_bh(&g_web_stat_lock);
				chr_notify_event(CHR_WEB_STAT_EVENT,
					g_user_space_pid, 0, rtn_abn);
				pr_info("chr: syn no ack report s:%x-d:%x>:%x\n",
				stream_list[hashcnt].src_addr & IPV4ADDR_MASK,
				stream_list[hashcnt].dst_addr & IPV4ADDR_MASK,
				stream_list[hashcnt].tcp_port);
				abnomal_stamp_list_syn_no_ack_print_log();
				spin_lock_bh(&g_web_stat_lock);
				memset(&rtn_abn, 0, sizeof(rtn_abn));
				abn_stamp_syn_no_ack =
					jiffies + FORBID_TIME;

			}
			stream_list[hashcnt].is_valid = IS_UNUSE;
		}

		hashNum++;
	}

	if (time_after(jiffies, rpt_stamp + REPORT_TIME)) {

		rpt_stamp = jiffies + REPORT_TIME;
		rtn_stat[RMNET_INTERFACE].report_type = WEB_STAT;
		rtn_stat[WLAN_INTERFACE].report_type = WEB_STAT;
		spin_unlock_bh(&g_web_stat_lock);
#ifdef CONFIG_HW_NETBOOSTER_MODULE		
		video_chr_stat_report();
#endif
		chr_notify_event(CHR_WEB_STAT_EVENT,
			g_user_space_pid, 0, rtn_stat);
		spin_lock_bh(&g_web_stat_lock);
		memset(&rtn_stat, 0, sizeof(rtn_stat));
		web_delay_rtt_flag_reset();
	}

	/*Check if there are timeout entries and remove them*/
	if (hashNum > 0) {
		sleep_flag = false;
		g_web_stat_timer.expires = jiffies + CHECK_TIME;
		spin_unlock_bh(&g_web_stat_lock);
		add_timer(&g_web_stat_timer);
		return;
	}
	sleep_flag = true;

	spin_unlock_bh(&g_web_stat_lock);
}

/*Computes the hash value of the network tcp stream*/
u8 hash3(u32 dst, u32 src, u32 port)
{
	u32 hash;
	hash = dst + src + port;
	hash = hash + hash/256 + hash/65536 + hash/16777216;
	hash = hash%HASH_MAX;
	return (u8)hash;
}

/*Local_out packet processing*/
void out_proc(void)
{
	u8 hash_cnt;
	u32 http_get_delay = 0;
	u8 interface_type = http_para_out.interface;

	spin_lock_bh(&g_web_para_out_lock);

	if (http_para_out.is_valid == IS_USE) {

		hash_cnt = hash3(http_para_out.dst_addr,
			http_para_out.src_addr, http_para_out.tcp_port);

		if (stream_list[hash_cnt].is_valid == IS_UNUSE) {

			if (http_para_out.type == HTTP_SYN) {

				memcpy(&stream_list[hash_cnt],
					&http_para_out, sizeof(http_para_out));

			}

		} else if (stream_list[hash_cnt].type == HTTP_SYN &&
			http_para_out.type == HTTP_GET) {

			if (stream_list[hash_cnt].src_addr ==
				http_para_out.src_addr &&
			stream_list[hash_cnt].dst_addr ==
				http_para_out.dst_addr &&
			stream_list[hash_cnt].tcp_port ==
				http_para_out.tcp_port) {

				stream_list[hash_cnt].get_time_stamp =
					http_para_out.time_stamp;
				if(stream_list[hash_cnt].interface == http_para_out.interface) {
					if (http_para_out.time_stamp >= stream_list[hash_cnt].ack_time_stamp && 0 != stream_list[hash_cnt].ack_time_stamp) {
						http_get_delay = (http_para_out.time_stamp - stream_list[hash_cnt].ack_time_stamp) * MULTIPLE;
					} else if (http_para_out.time_stamp < stream_list[hash_cnt].ack_time_stamp && 0 != stream_list[hash_cnt].ack_time_stamp) {
						http_get_delay = (MAX_JIFFIES - stream_list[hash_cnt].ack_time_stamp +http_para_out.time_stamp) * MULTIPLE;
					}
					rtn_stat[interface_type].http_get_delay += http_get_delay;
					rtn_stat[interface_type].http_send_get_num++;
				}
				stream_list[hash_cnt].type = HTTP_GET;

			}
		}

		http_para_out.is_valid = IS_UNUSE;

		if (sleep_flag) {
			sleep_flag = false;
			g_web_stat_timer.expires = jiffies + CHECK_TIME;
			spin_unlock_bh(&g_web_para_out_lock);
			add_timer(&g_web_stat_timer);
			return;
		}
	}

	spin_unlock_bh(&g_web_para_out_lock);
}

void wifi_disconnect_report(void)
{
	pr_info("wifi_disconnect_report web_stat\n");
	spin_lock_bh(&g_web_stat_lock);
	rpt_stamp = jiffies + REPORT_TIME;
	rtn_stat[RMNET_INTERFACE].report_type = WEB_STAT;
	rtn_stat[WLAN_INTERFACE].report_type = WEB_STAT;
	spin_unlock_bh(&g_web_stat_lock);
	chr_notify_event(CHR_WEB_STAT_EVENT,
		g_user_space_pid, 0, rtn_stat);
	spin_lock_bh(&g_web_stat_lock);
	memset(&rtn_stat, 0, sizeof(rtn_stat));
	web_delay_rtt_flag_reset();
	spin_unlock_bh(&g_web_stat_lock);
}

/*Local_in packet processing*/
void in_proc(void)
{
	u8 hash_cnt;
	u32 web_delay;
	u32 handshake_delay;
	unsigned long jiffies_tmp;
	unsigned long abn_stamp;
	u8 interface_type = http_para_in.interface;

	jiffies_tmp = jiffies;

	spin_lock_bh(&g_web_para_in_lock);
	if (http_para_in.is_valid == IS_UNUSE) {
		spin_unlock_bh(&g_web_para_in_lock);
		return;
	}

	hash_cnt = hash3(http_para_in.dst_addr,
		http_para_in.src_addr, http_para_in.tcp_port);

	if (stream_list[hash_cnt].is_valid == IS_UNUSE ||
			(stream_list[hash_cnt].type != HTTP_GET &&
			stream_list[hash_cnt].type != HTTP_SYN)) {

		http_para_in.is_valid = IS_UNUSE;
		spin_unlock_bh(&g_web_para_in_lock);
		return;

	}

	/*In all three cases, the tcp stream is removed from the table.
	Http get to http response time is too long,
	that is, no response to the page.
	Visit the web page successfully. Failed to access webpage*/
	if (stream_list[hash_cnt].src_addr == http_para_in.src_addr &&
	stream_list[hash_cnt].dst_addr == http_para_in.dst_addr &&
	stream_list[hash_cnt].tcp_port == http_para_in.tcp_port &&
	stream_list[hash_cnt].interface == http_para_in.interface) {

		switch (http_para_in.type) {
		case WEB_SUCC:
			rtn_stat[interface_type].total_num++;
			rtn_stat[interface_type].succ_num++;

			if (http_para_in.time_stamp >= stream_list[hash_cnt].time_stamp) {
				web_delay = (http_para_in.time_stamp - stream_list[hash_cnt].time_stamp) * MULTIPLE;
			} else {
				web_delay = (MAX_JIFFIES - stream_list[hash_cnt].time_stamp + http_para_in.time_stamp) * MULTIPLE;
			}
			rtn_stat[interface_type].web_delay += web_delay;

			if (web_deley_flag[interface_type])
			{
				rtn_stat[interface_type].highest_web_delay= web_delay;
				rtn_stat[interface_type].lowest_web_delay= web_delay;
				rtn_stat[interface_type].last_web_delay= web_delay;
				web_deley_flag[interface_type] = false;
			}
			/*recording the web_delays value*/
			if (web_delay > rtn_stat[interface_type].highest_web_delay)
				rtn_stat[interface_type].highest_web_delay = web_delay;
			if (web_delay< rtn_stat[interface_type].lowest_web_delay)
				rtn_stat[interface_type].lowest_web_delay = web_delay;
			rtn_stat[interface_type].last_web_delay = web_delay;
			if (web_delay > DELAY_THRESHOLD_L1 &&
					web_delay <= DELAY_THRESHOLD_L2)
				rtn_stat[interface_type].delay_num_L1++;

			else if (web_delay > DELAY_THRESHOLD_L2 &&
					web_delay <= DELAY_THRESHOLD_L3)
				rtn_stat[interface_type].delay_num_L2++;

			else if (web_delay > DELAY_THRESHOLD_L3 &&
					web_delay <= DELAY_THRESHOLD_L4)
				rtn_stat[interface_type].delay_num_L3++;

			else if (web_delay > DELAY_THRESHOLD_L4 &&
					web_delay <= DELAY_THRESHOLD_L5)
				rtn_stat[interface_type].delay_num_L4++;

			else if (web_delay > DELAY_THRESHOLD_L5 &&
					web_delay <= DELAY_THRESHOLD_L6)
				rtn_stat[interface_type].delay_num_L5++;

			else if (web_delay > DELAY_THRESHOLD_L6)
				rtn_stat[interface_type].delay_num_L6++;

			save_app_web_delay(stream_list[hash_cnt].uid,
				web_delay,interface_type);

			abn_stamp =
			abnomal_stamp_list_web_delay_update(jiffies_tmp);
			if (time_after(jiffies_tmp, abn_stamp_web_delay) &&
				web_delay > WEB_DELAY_THRESHOLD &&
				time_before(jiffies_tmp, abn_stamp +
				WEB_DELAY_REPORT_TIME)) {

				rtn_abn[interface_type].report_type = WEB_DELAY;
				rtn_abn[interface_type].web_delay = web_delay;
				rtn_abn[interface_type].uid = stream_list[hash_cnt].uid;
				rtn_abn[interface_type].server_addr = stream_list[hash_cnt].dst_addr;
				spin_unlock_bh(&g_web_para_in_lock);
				spin_unlock_bh(&g_web_stat_lock);
				chr_notify_event(CHR_WEB_STAT_EVENT,
					g_user_space_pid, 0, rtn_abn);
				pr_info("chr: web delay report s:%x-d:%x>:%x\n",
				stream_list[hash_cnt].src_addr & IPV4ADDR_MASK,
				stream_list[hash_cnt].dst_addr & IPV4ADDR_MASK,
				stream_list[hash_cnt].tcp_port);
				spin_lock_bh(&g_web_stat_lock);
				memset(&rtn_abn, 0, sizeof(rtn_abn));
				spin_lock_bh(&g_web_para_in_lock);
				abn_stamp_web_delay = jiffies_tmp + FORBID_TIME;
			}
			stream_list[hash_cnt].is_valid = IS_UNUSE;
			break;

		case WEB_FAIL:
			rtn_stat[interface_type].total_num++;
			rtn_stat[interface_type].fail_num++;
			save_app_web_fail(stream_list[hash_cnt].uid, interface_type);
			abn_stamp =
			abnomal_stamp_list_web_fail_update(jiffies_tmp);
			if (time_after(jiffies_tmp, abn_stamp_web_fail) &&
				time_before(jiffies_tmp, abn_stamp +
				WEB_FAIL_REPORT_TIME)) {
				rtn_abn[interface_type].report_type = WEB_FAIL;
				rtn_abn[interface_type].uid = stream_list[hash_cnt].uid;
				rtn_abn[interface_type].http_resp = http_para_in.resp_code;
				rtn_abn[interface_type].server_addr = stream_list[hash_cnt].dst_addr;
				spin_unlock_bh(&g_web_para_in_lock);
				spin_unlock_bh(&g_web_stat_lock);
				chr_notify_event(CHR_WEB_STAT_EVENT,
					g_user_space_pid, 0, rtn_abn);
				pr_info("chr: web fail report s:%x-d:%x>:%d\n",
				stream_list[hash_cnt].src_addr & IPV4ADDR_MASK,
				stream_list[hash_cnt].dst_addr & IPV4ADDR_MASK,
				stream_list[hash_cnt].tcp_port);
				spin_lock_bh(&g_web_stat_lock);
				memset(&rtn_abn, 0, sizeof(rtn_abn));
				spin_lock_bh(&g_web_para_in_lock);
				abn_stamp_web_fail = jiffies_tmp + FORBID_TIME;

			}
			stream_list[hash_cnt].is_valid = IS_UNUSE;
			break;

		case SYN_SUCC:
			rtn_stat[interface_type].tcp_succ_num++;
			if (http_para_in.time_stamp >= stream_list[hash_cnt].time_stamp) {
				handshake_delay = (http_para_in.time_stamp - stream_list[hash_cnt].time_stamp) * MULTIPLE;
			} else {
				handshake_delay = (MAX_JIFFIES - stream_list[hash_cnt].time_stamp + http_para_in.time_stamp) * MULTIPLE;
			}
			rtn_stat[interface_type].tcp_handshake_delay += handshake_delay;
			stream_list[hash_cnt].ack_time_stamp = http_para_in.time_stamp;
			save_app_syn_succ(stream_list[hash_cnt].uid, interface_type);
			break;

		default:
			stream_list[hash_cnt].is_valid = IS_UNUSE;
			break;

		}

	} else if (stream_list[hash_cnt].type == HTTP_GET &&
		time_after(jiffies_tmp, stream_list[hash_cnt].get_time_stamp +
		EXPIRE_TIME)) {

		rtn_stat[interface_type].total_num++;
		rtn_stat[interface_type].no_ack_num++;

		save_app_web_no_ack(stream_list[hash_cnt].uid, interface_type);

		abnomal_stamp_list_web_no_ack_update(jiffies_tmp);
		if (time_after(jiffies_tmp, abn_stamp_no_ack) &&
			time_before(jiffies_tmp, abn_stamp_list_web_no_ack[0] +
				WEB_NO_ACK_REPORT_TIME)) {

			rtn_abn[interface_type].report_type = WEB_NO_ACK;
			rtn_abn[interface_type].uid = stream_list[hash_cnt].uid;
			rtn_abn[interface_type].http_resp = 0xffffffff;
			rtn_abn[interface_type].server_addr = stream_list[hash_cnt].dst_addr;
			spin_unlock_bh(&g_web_para_in_lock);
			spin_unlock_bh(&g_web_stat_lock);
			chr_notify_event(CHR_WEB_STAT_EVENT,
				g_user_space_pid, 0, rtn_abn);
			pr_info("chr: no ack report s:%x-d:%x>:%x\n",
			stream_list[hash_cnt].src_addr & IPV4ADDR_MASK,
			stream_list[hash_cnt].dst_addr & IPV4ADDR_MASK,
			stream_list[hash_cnt].tcp_port);
			spin_lock_bh(&g_web_stat_lock);
			memset(&rtn_abn, 0, sizeof(rtn_abn));
			spin_lock_bh(&g_web_para_in_lock);
			abn_stamp_no_ack = jiffies_tmp + FORBID_TIME;
		}

		stream_list[hash_cnt].is_valid = IS_UNUSE;
	}

	http_para_in.is_valid = IS_UNUSE;

	if (sleep_flag) {
		sleep_flag = false;
		g_web_stat_timer.expires = jiffies + CHECK_TIME;
		spin_unlock_bh(&g_web_para_in_lock);
		add_timer(&g_web_stat_timer);
		return;
	}

	spin_unlock_bh(&g_web_para_in_lock);
}

void rtt_proc(void)
{
	unsigned long abn_stamp;
	int idx = 0;
	for (idx = 0; idx < 2; idx++) {
		if (stack_rtt[idx].is_valid == IS_USE) {

			rtn_stat[idx].tcp_total_num++;
			rtn_stat[idx].tcp_rtt += stack_rtt[idx].tcp_rtt;

			if (stack_rtt[idx].tcp_rtt > RTT_THRESHOLD_L1 &&
					stack_rtt[idx].tcp_rtt <= RTT_THRESHOLD_L2)
				rtn_stat[idx].rtt_num_L1++;

			else if (stack_rtt[idx].tcp_rtt > RTT_THRESHOLD_L2 &&
					stack_rtt[idx].tcp_rtt <= RTT_THRESHOLD_L3)
				rtn_stat[idx].rtt_num_L2++;

			else if (stack_rtt[idx].tcp_rtt > RTT_THRESHOLD_L3 &&
					stack_rtt[idx].tcp_rtt <= RTT_THRESHOLD_L4)
				rtn_stat[idx].rtt_num_L3++;

			else if (stack_rtt[idx].tcp_rtt > RTT_THRESHOLD_L4 &&
					stack_rtt[idx].tcp_rtt <= RTT_THRESHOLD_L5)
				rtn_stat[idx].rtt_num_L4++;

			else if (stack_rtt[idx].tcp_rtt > RTT_THRESHOLD_L5)
				rtn_stat[idx].rtt_num_L5++;

			save_app_tcp_rtt(stack_rtt[idx].uid, stack_rtt[idx].tcp_rtt, idx);

			if (rtt_flag[idx]){
				rtn_stat[idx].highest_tcp_rtt = stack_rtt[idx].tcp_rtt;
				rtn_stat[idx].lowest_tcp_rtt = stack_rtt[idx].tcp_rtt;
				rtn_stat[idx].last_tcp_rtt = stack_rtt[idx].tcp_rtt;
				rtt_flag[idx] = false;
			}
			if (stack_rtt[idx].tcp_rtt > rtn_stat[idx].highest_tcp_rtt)
				rtn_stat[idx].highest_tcp_rtt = stack_rtt[idx].tcp_rtt;
			if (stack_rtt[idx].tcp_rtt < rtn_stat[idx].lowest_tcp_rtt)
				rtn_stat[idx].lowest_tcp_rtt = stack_rtt[idx].tcp_rtt;
			rtn_stat[idx].last_tcp_rtt = stack_rtt[idx].tcp_rtt;

			abn_stamp = abnomal_stamp_list_tcp_rtt_large_update(jiffies);
			if (stack_rtt[idx].tcp_rtt > RTT_THRESHOLD &&
				time_after(jiffies, abn_stamp_rtt_large) &&
				time_before(jiffies, abn_stamp +
				TCP_RTT_LARGE_REPORT_TIME)) {

				abn_stamp_rtt_large = jiffies + FORBID_TIME;
				rtn_abn[idx].report_type = TCP_RTT_LARGE;
				rtn_abn[idx].tcp_rtt = stack_rtt[idx].tcp_rtt;
				rtn_abn[idx].uid = stack_rtt[idx].uid;
				rtn_abn[idx].rtt_abn_server_addr = stack_rtt[idx].rtt_dst_addr;
				spin_unlock_bh(&g_web_stat_lock);
				chr_notify_event(CHR_WEB_STAT_EVENT,
					g_user_space_pid, 0, rtn_abn);
				pr_info("chr: rtt large report\n");
				spin_lock_bh(&g_web_stat_lock);
				memset(&rtn_abn, 0, sizeof(rtn_abn));
			}
			stack_rtt[idx].is_valid = IS_UNUSE;

			if (sleep_flag) {
				sleep_flag = false;
				g_web_stat_timer.expires = jiffies + CHECK_TIME;
				spin_unlock_bh(&g_web_stat_lock);
				add_timer(&g_web_stat_timer);
				spin_lock_bh(&g_web_stat_lock);
			}
		}
	}
}

/*This is the main thread of web statistics*/
static int chr_web_thread(void *data)
{
	while (1) {
		if (kthread_should_stop())
			break;

		down(&g_web_stat_sync_sema);
		spin_lock_bh(&g_web_stat_lock);
		in_proc();
		out_proc();
		rtt_proc();
		spin_unlock_bh(&g_web_stat_lock);
	}
	return 0;
}

/*Calculates the return code for http*/
u8 http_response_type(char *pstr)
{
	u8 type;

	type = UN_KNOW;

	if (pstr[HTTP_ACK_FROM_START] == '2' ||
			pstr[HTTP_ACK_FROM_START] == '3') {
		type = WEB_SUCC;
	}

	if (pstr[HTTP_ACK_FROM_START] == '4' ||
			pstr[HTTP_ACK_FROM_START] == '5') {
		type = WEB_FAIL;
	}
	return type;
}

u32 http_response_code(char *pstr)
{
	u32 code = 0;
	int idx;
	char ch;

	if (pstr == NULL)
		return 0;

	for (idx = 0; idx < 3; idx++) {
		ch = pstr[(int)(HTTP_ACK_FROM_START + idx)];
		if ('0' <= ch && ch <= '9')
			code = code * 10 + (ch - '0');
		else
			return 0;
	}
	return code;
}

/*Local out hook function*/
static unsigned int hook_local_out(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	struct tcp_sock *sock = NULL;
	char *pHttpStr = NULL;
	bool up_req = false;
	int dlen;


	if (skb == NULL)
		return NF_ACCEPT;

	iph = ip_hdr(skb);
	if (iph == NULL)
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP) {

		tcph = tcp_hdr(skb);

		if (NULL == tcph || NULL == skb->data || 0 == tcph->doff)
			return NF_ACCEPT;

		pHttpStr = (char *)((u32 *)tcph + tcph->doff);
		dlen = skb->len - (pHttpStr - (char *)iph);

		if (dlen < 0)
			return NF_ACCEPT;

		if (NULL == skb->dev || NULL == skb->dev->name)
			return NF_ACCEPT;

		if (skb->sk == NULL)
			return NF_ACCEPT;

		if (strncmp(skb->dev->name, DS_NET, DS_NET_LEN)) {
			http_para_out.interface = WLAN_INTERFACE;
		}
		else {
			if ((data_reg_tech != RIL_RADIO_TECHNOLOGY_LTE)&&
				(data_reg_tech != RIL_RADIO_TECHNOLOGY_LTE_CA))
				return NF_ACCEPT;
			http_para_out.interface = RMNET_INTERFACE;
			if (skb->sk->sk_state == TCP_ESTABLISHED) {
				sock = tcp_sk(skb->sk);
				sock->data_net_flag = true;
			}
		}

		if (htons(tcph->dest) != HTTP_PORT)
			return NF_ACCEPT;

		/*When the lock is not locked, the lock is triggered*/
		if (!spin_trylock_bh(&g_web_para_out_lock))
			return NF_ACCEPT;

		if (http_para_out.is_valid == IS_UNUSE) {

			/*This is an http ack syn packet processing*/
			if (tcph->syn == 1 && tcph->ack == 0) {

				http_para_out.tcp_port = tcph->source;
				http_para_out.src_addr = iph->saddr;
				http_para_out.dst_addr = iph->daddr;
				http_para_out.type = HTTP_SYN;
				http_para_out.time_stamp = jiffies;
				http_para_out.is_valid = IS_USE;
				http_para_out.uid = get_uid_from_sock(skb->sk);
				up_req = true;

			} else if (dlen > 5 &&
			(strncmp(pHttpStr, g_get_str, STR_GET_LEN) == 0 ||
			strncmp(pHttpStr, g_post_str, STR_POST_LEN) == 0)) {

				http_para_out.tcp_port = tcph->source;
				http_para_out.src_addr = iph->saddr;
				http_para_out.dst_addr = iph->daddr;
				http_para_out.type = HTTP_GET;
				http_para_out.time_stamp = jiffies;
				http_para_out.is_valid = IS_USE;
				up_req = true;
			}
		}

		spin_unlock_bh(&g_web_para_out_lock);

		if (up_req)
			up(&g_web_stat_sync_sema);

	}

	return NF_ACCEPT;
}

/*Local in hook function*/
static unsigned int hook_local_in(void *priv,
					struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	char *pHttpStr = NULL;
	bool up_req = false;
	u32 dlen;


	if (skb == NULL)
		return NF_ACCEPT;

	iph = ip_hdr(skb);
	if (iph == NULL)
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP) {

		tcph = tcp_hdr(skb);

		if (NULL == tcph || NULL == skb->data || 0 == tcph->doff)
			return NF_ACCEPT;

		pHttpStr = (char *)((u32 *)tcph + tcph->doff);
		dlen = skb->len - (pHttpStr - (char *)iph);

		if (NULL == skb->dev || NULL == skb->dev->name)
			return NF_ACCEPT;

		if (strncmp(skb->dev->name, DS_NET, DS_NET_LEN)) {
			http_para_in.interface = WLAN_INTERFACE;
		}
		else {
			if ((data_reg_tech != RIL_RADIO_TECHNOLOGY_LTE)&&
				(data_reg_tech != RIL_RADIO_TECHNOLOGY_LTE_CA))
				return NF_ACCEPT;
			http_para_in.interface = RMNET_INTERFACE;
		}

		if (htons(tcph->source) != HTTP_PORT)
			return NF_ACCEPT;

		if (!spin_trylock_bh(&g_web_para_in_lock))
			return NF_ACCEPT;

		/* Determine whether the received packet is an HTTP response */
		if (dlen > HTTP_RSP_LEN &&
			strncmp(pHttpStr, g_http_str, STR_HTTP_LEN) == 0 &&
			http_response_type(pHttpStr) != UN_KNOW &&
			http_para_in.is_valid == IS_UNUSE) {

			http_para_in.tcp_port = tcph->dest;
			http_para_in.src_addr = iph->daddr;
			http_para_in.dst_addr = iph->saddr;
			http_para_in.time_stamp = jiffies;
			http_para_in.is_valid = IS_USE;
			http_para_in.type = http_response_type(pHttpStr);
			http_para_in.resp_code = http_response_code(pHttpStr);
			up_req = true;
		}
		/* Determine whether the received packet is an SYN ACK */
		else if (http_para_in.is_valid == IS_UNUSE &&
			tcph->syn == 1 && tcph->ack == 1) {
			http_para_in.tcp_port = tcph->dest;
			http_para_in.src_addr = iph->daddr;
			http_para_in.dst_addr = iph->saddr;
			http_para_in.time_stamp = jiffies;
			http_para_in.is_valid = IS_USE;
			http_para_in.type = SYN_SUCC;
			up_req = true;
		}

		spin_unlock_bh(&g_web_para_in_lock);

		if (up_req)
			up(&g_web_stat_sync_sema);

	}
	return NF_ACCEPT;
}

static struct nf_hook_ops net_hooks[] = {
	{
		.hook		= hook_local_in,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0))
		.owner		= THIS_MODULE,
#endif
		.pf			= PF_INET,
		.hooknum	= NF_INET_LOCAL_IN,
		.priority	= NF_IP_PRI_FILTER - 1,
	},
	{
		.hook		= hook_local_out,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4,4,0))
		.owner		= THIS_MODULE,
#endif
		.pf			= PF_INET,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_FILTER - 1,
	}
};
static void web_delay_rtt_flag_reset(void)
{
	int flag_index;
	for (flag_index=0; flag_index<RNT_STAT_SIZE ;flag_index++){
		rtt_flag[flag_index] = true;
		web_deley_flag[flag_index] = true;
	}
}
/*CHR Initialization function*/
int web_chr_init(void)
{
	int ret = 0;

       /*Initializes the array*/
	stream_list = kmalloc(sizeof(struct http_stream)*HASH_MAX, GFP_KERNEL);
	if (stream_list == NULL)
		return -1;

	memset(stream_list, 0, sizeof(struct http_stream)*HASH_MAX);

	/*Variable initialization*/
	memset(&rtn_stat, 0, sizeof(rtn_stat));
	memset(&rtn_abn, 0, sizeof(rtn_abn));
	memset(&http_para_in, 0, sizeof(http_para_in));
	memset(&http_para_out, 0, sizeof(http_para_out));
	memset(&stack_rtt, 0, sizeof(stack_rtt));

	/*spin lock initialization*/
	spin_lock_init(&g_web_stat_lock);
	spin_lock_init(&g_web_para_in_lock);
	spin_lock_init(&g_web_para_out_lock);
	sema_init(&g_web_stat_sync_sema, 0);
	/*flag initialization*/
	web_delay_rtt_flag_reset();
	/*Timestamp initialization*/
	abn_stamp_no_ack = jiffies;
	abn_stamp_rtt_large = jiffies;
	abn_stamp_web_fail = jiffies;
	abn_stamp_web_delay = jiffies;
	abn_stamp_syn_no_ack = jiffies;
	g_chr_key_val.tcp_last = jiffies;
	g_chr_key_val.udp_last = jiffies;
	atomic_set(&g_chr_key_val.tcp_buf_time, 0);
	atomic_set(&g_chr_key_val.udp_buf_time, 0);

	rpt_stamp = jiffies;

	abn_stamp_list_syn_no_ack =
		kmalloc(sizeof(unsigned long)*SYN_NO_ACK_MAX, GFP_KERNEL);
	if (abn_stamp_list_syn_no_ack == NULL)
		goto error;
	abn_stamp_list_web_no_ack =
		kmalloc(sizeof(unsigned long)*WEB_NO_ACK_MAX, GFP_KERNEL);
	if (abn_stamp_list_web_no_ack == NULL)
		goto error;
	abn_stamp_list_web_fail =
		kmalloc(sizeof(unsigned long)*WEB_FAIL_MAX, GFP_KERNEL);
	if (abn_stamp_list_web_fail == NULL)
		goto error;
	abn_stamp_list_web_delay =
		kmalloc(sizeof(unsigned long)*WEB_DELAY_MAX, GFP_KERNEL);
	if (abn_stamp_list_web_delay == NULL)
		goto error;
	abn_stamp_list_tcp_rtt_large =
		kmalloc(sizeof(unsigned long)*TCP_RTT_LARGE_MAX, GFP_KERNEL);
	if (abn_stamp_list_tcp_rtt_large == NULL)
		goto error;

	memset(abn_stamp_list_syn_no_ack, 0,
		sizeof(unsigned long)*SYN_NO_ACK_MAX);
	memset(abn_stamp_list_web_no_ack, 0,
		sizeof(unsigned long)*WEB_NO_ACK_MAX);
	memset(abn_stamp_list_web_fail, 0,
		sizeof(unsigned long)*WEB_FAIL_MAX);
	memset(abn_stamp_list_web_delay, 0,
		sizeof(unsigned long)*WEB_DELAY_MAX);
	memset(abn_stamp_list_tcp_rtt_large, 0,
		sizeof(unsigned long)*TCP_RTT_LARGE_MAX);

	abn_stamp_list_syn_no_ack_idx = 0;
	abn_stamp_list_web_no_ack_idx = 0;
	abn_stamp_list_web_fail_idx = 0;
	abn_stamp_list_web_delay_idx = 0;
	abn_stamp_list_tcp_rtt_large_idx = 0;

	/*Create a thread*/
	g_web_stat_task = kthread_run(chr_web_thread, NULL, "chr_web_thread");

	/*Timer initialization*/
	init_timer(&g_web_stat_timer);
	g_web_stat_timer.data = 0;
	g_web_stat_timer.function = web_stat_timer;
	g_web_stat_timer.expires = jiffies + CHECK_TIME;
	add_timer(&g_web_stat_timer);
	sleep_flag = false;

       /*Registration hook function*/
	ret = nf_register_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	if (ret) {
		pr_info("chr:nf_init_in ret=%d  ", ret);
		return -1;
	}
	pr_info("chr:web stat init success\n");

	return 0;
error:
	if (stream_list != NULL)
		kfree(stream_list);
	if (abn_stamp_list_syn_no_ack != NULL)
		kfree(abn_stamp_list_syn_no_ack);
	if (abn_stamp_list_web_no_ack != NULL)
		kfree(abn_stamp_list_web_no_ack);
	if (abn_stamp_list_web_fail != NULL)
		kfree(abn_stamp_list_web_fail);
	if (abn_stamp_list_web_delay != NULL)
		kfree(abn_stamp_list_web_delay);
	if (abn_stamp_list_tcp_rtt_large != NULL)
		kfree(abn_stamp_list_tcp_rtt_large);
	pr_info("chr:web stat init fail");
	return -1;
}

void web_chr_exit(void)
{
	nf_unregister_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	kfree(stream_list);
	kfree(abn_stamp_list_syn_no_ack);
	kfree(abn_stamp_list_web_no_ack);
	kfree(abn_stamp_list_web_fail);
	kfree(abn_stamp_list_web_delay);
	kfree(abn_stamp_list_tcp_rtt_large);
	pr_info("chr:web stat exit success\n");
}

uid_t get_uid_from_sock(struct sock *sk)
{
	const struct file *filp = NULL;

	if (NULL == sk || NULL == sk->sk_socket)
		return 0;

	filp = sk->sk_socket->file;
	if (NULL == filp || NULL == filp->f_cred)
		return 0;

	return from_kuid(&init_user_ns, filp->f_cred->fsuid);
}

u32 get_des_addr_from_sock(struct sock *sk)
{
	if (NULL == sk)
		return 0;
	return sk->sk_daddr;
}
/* Append time_stamp to the end of the abn_stamp_list */
unsigned long abnomal_stamp_list_syn_no_ack_update(
unsigned long time_stamp)
{
	abn_stamp_list_syn_no_ack[abn_stamp_list_syn_no_ack_idx] = time_stamp;
	abn_stamp_list_syn_no_ack_idx =
		(abn_stamp_list_syn_no_ack_idx + 1) % SYN_NO_ACK_MAX;
	return abn_stamp_list_syn_no_ack[abn_stamp_list_syn_no_ack_idx];
}

void abnomal_stamp_list_syn_no_ack_print_log(void)
{
	int idx;

	for (idx = 0; idx < SYN_NO_ACK_MAX; idx++) {
		pr_info("chr:abn_stamp_list_syn_no_ack[%d]=%d\n", idx,
			(int)abn_stamp_list_syn_no_ack[idx]);
	}
	pr_info("chr:abn_stamp_list_syn_no_ack_idx=%d\n",
		(int)abn_stamp_list_syn_no_ack_idx);
}

unsigned long abnomal_stamp_list_web_no_ack_update(
	unsigned long time_stamp)
{
	abn_stamp_list_web_no_ack[abn_stamp_list_web_no_ack_idx] = time_stamp;
	abn_stamp_list_web_no_ack_idx =
		(abn_stamp_list_web_no_ack_idx + 1) % WEB_NO_ACK_MAX;
	return abn_stamp_list_web_no_ack[abn_stamp_list_web_no_ack_idx];
}

unsigned long abnomal_stamp_list_web_fail_update(
	unsigned long time_stamp)
{
	abn_stamp_list_web_fail[abn_stamp_list_web_fail_idx] = time_stamp;
	abn_stamp_list_web_fail_idx =
		(abn_stamp_list_web_fail_idx + 1) % WEB_FAIL_MAX;
	return abn_stamp_list_web_fail[abn_stamp_list_web_fail_idx];
}

unsigned long abnomal_stamp_list_web_delay_update(
	unsigned long time_stamp)
{
	abn_stamp_list_web_delay[abn_stamp_list_web_delay_idx] = time_stamp;
	abn_stamp_list_web_delay_idx =
		(abn_stamp_list_web_delay_idx + 1) % WEB_DELAY_MAX;
	return abn_stamp_list_web_delay[abn_stamp_list_web_delay_idx];
}

unsigned long abnomal_stamp_list_tcp_rtt_large_update(
	unsigned long time_stamp)
{
	abn_stamp_list_tcp_rtt_large[abn_stamp_list_tcp_rtt_large_idx] =
		time_stamp;
	abn_stamp_list_tcp_rtt_large_idx =
		(abn_stamp_list_tcp_rtt_large_idx + 1) % TCP_RTT_LARGE_MAX;
	return abn_stamp_list_tcp_rtt_large[abn_stamp_list_tcp_rtt_large_idx];
}

int set_report_app_uid(int tag, u32 paras)
{
	if (tag >= 0 && tag < CHR_MAX_REPORT_APP_COUNT) {
		s_report_app_uid_lst[tag] = paras;
		return 0;
	}
	if (tag == DATA_REG_TECH_TAG) {
		data_reg_tech = paras;
		return 0;
	}
	if (tag == GET_AP_REPORT_TAG) {
		if (paras& 0x01)
			chr_notify_event(CHR_SPEED_SLOW_EVENT, g_user_space_pid,
				reportBuf(), NULL);
		return 0;
	}

	pr_info("chr:set_report_app_uid set 'tag' invaild. tag=%d\n", tag);
	return -1;
}

void save_app_syn_succ(u32 uid, u8 interface_type)
{
	int i = -1;

	for (i = 0; i < CHR_MAX_REPORT_APP_COUNT; i++) {
		if (uid == s_report_app_uid_lst[i]) {
			rtn_stat[interface_type].report_app_stat_list[i].tcp_succ_num++;
			break;
		}
	}
}

void save_app_web_no_ack(u32 uid,u8 interface_type)
{
	int i = -1;

	for (i = 0; i < CHR_MAX_REPORT_APP_COUNT; i++) {
		if (uid == s_report_app_uid_lst[i]) {
			rtn_stat[interface_type].report_app_stat_list[i].total_num++;
			rtn_stat[interface_type].report_app_stat_list[i].no_ack_num++;
			break;
		}
	}
}

void save_app_web_fail(u32 uid,u8 interface_type)
{
	int i = -1;

	for (i = 0; i < CHR_MAX_REPORT_APP_COUNT; i++) {
		if (uid == s_report_app_uid_lst[i]) {
			rtn_stat[interface_type].report_app_stat_list[i].total_num++;
			rtn_stat[interface_type].report_app_stat_list[i].fail_num++;
			break;
		}
	}
}

static void save_app_web_delay(u32 uid, int web_delay,u8 interface_type)
{
	int i = -1;
	struct report_app_stat *app_stat = NULL;

	for (i = 0; i < CHR_MAX_REPORT_APP_COUNT; i++) {
		if (uid == s_report_app_uid_lst[i]) {
			app_stat = &rtn_stat[interface_type].report_app_stat_list[i];
			app_stat->total_num++;
			app_stat->succ_num++;

			app_stat->web_delay += web_delay;

			if (web_delay > DELAY_THRESHOLD_L1 &&
					web_delay <= DELAY_THRESHOLD_L2)
				app_stat->delay_num_L1++;

			else if (web_delay > DELAY_THRESHOLD_L2 &&
					web_delay <= DELAY_THRESHOLD_L3)
				app_stat->delay_num_L2++;

			else if (web_delay > DELAY_THRESHOLD_L3 &&
					web_delay <= DELAY_THRESHOLD_L4)
				app_stat->delay_num_L3++;

			else if (web_delay > DELAY_THRESHOLD_L4 &&
					web_delay <= DELAY_THRESHOLD_L5)
				app_stat->delay_num_L4++;

			else if (web_delay > DELAY_THRESHOLD_L5 &&
					web_delay <= DELAY_THRESHOLD_L6)
				app_stat->delay_num_L5++;

			else if (web_delay > DELAY_THRESHOLD_L6)
				app_stat->delay_num_L6++;
			break;
		}
	}
}

static void save_app_tcp_rtt(u32 uid, u32 tcp_rtt,u8 interface_type)
{
	int i = -1;
	struct report_app_stat *app_stat = NULL;

	for (i = 0; i < CHR_MAX_REPORT_APP_COUNT; i++) {
		if (uid == s_report_app_uid_lst[i]) {
			app_stat = &rtn_stat[interface_type].report_app_stat_list[i];

			app_stat->tcp_total_num++;
			app_stat->tcp_rtt += tcp_rtt;

			if (tcp_rtt > RTT_THRESHOLD_L1 &&
					tcp_rtt <= RTT_THRESHOLD_L2)
				app_stat->rtt_num_L1++;

			else if (tcp_rtt > RTT_THRESHOLD_L2 &&
					tcp_rtt <= RTT_THRESHOLD_L3)
				app_stat->rtt_num_L2++;

			else if (tcp_rtt > RTT_THRESHOLD_L3 &&
					tcp_rtt <= RTT_THRESHOLD_L4)
				app_stat->rtt_num_L3++;

			else if (tcp_rtt > RTT_THRESHOLD_L4 &&
					tcp_rtt <= RTT_THRESHOLD_L5)
				app_stat->rtt_num_L4++;

			else if (tcp_rtt > RTT_THRESHOLD_L5)
				app_stat->rtt_num_L5++;

			break;
		}
	}
}

#ifdef CONFIG_HW_NETBOOSTER_MODULE
static void video_chr_stat_report(void)
{
	struct video_chr_para video_chr = {0};

	chr_video_stat(&video_chr);
	rtn_stat[RMNET_INTERFACE].vod_avg_speed = video_chr.vod_avg_speed;
	rtn_stat[RMNET_INTERFACE].vod_freez_num = video_chr.vod_freez_num;
	rtn_stat[RMNET_INTERFACE].vod_time = video_chr.vod_time;
	rtn_stat[RMNET_INTERFACE].uvod_avg_speed = video_chr.uvod_avg_speed;
	rtn_stat[RMNET_INTERFACE].uvod_freez_num = video_chr.uvod_freez_num;
	rtn_stat[RMNET_INTERFACE].uvod_time = video_chr.uvod_time;
	return;
}
#endif
#undef DEBUG
