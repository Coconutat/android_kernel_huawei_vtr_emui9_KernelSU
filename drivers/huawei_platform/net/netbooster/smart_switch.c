
#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/memory.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
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
#include <net/tcp.h>
#include <linux/skbuff.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/version.h>

#include "smart_switch.h"
#include "nb_netlink.h"
#include "constant.h"
#include "ai_predict.h"
#include "clf_params.h"

#ifndef DEBUG
#define DEBUG
#endif



/********************************
*	Defines global variables
*********************************/
/*smart switch statistic variable*/
static struct pkt_stat_swth g_stat;

/*Input and output data processing of the lock*/
static spinlock_t g_smart_switch_lock;

/*Result escalation Variable*/
static struct report_slow_para g_swth_report;

/*report time Variable*/
static long g_report_time;

/*RTT filter Variable*/
static unsigned int g_rtt;

/*KSI is enabled identifier*/
static uint8_t g_report_flag;

/*Hook is enabled identifier*/
static uint8_t g_hook_flag;
#ifdef CONFIG_CHR_NETLINK_MODULE
static struct chr_para g_chr_data;
static struct timer_list g_chr_rpt_timer;
static struct report_chr_stru g_chr_report;
#endif

/*hidata app qoe statistic variable*/
static struct pkt_stat_swth g_app_qoe_stat = {0};
static uid_t g_app_qoe_uid = 0;
static int g_app_qoe_cycle = 0;/*seconds*/
static int g_app_qoe_level = NETWORK_STATUS_INVALID;
static int g_slow_num = 0;
static int g_no_rx_num = 0;
static int g_normal_num = 0;
static int g_rsrp = 0;
static int g_dbm = 0;
static unsigned int g_app_rtt;

/*network_slow chr statistic variable*/
static int g_network_detect_window = 5;/*seconds*/
static int g_network_status = NETWORK_STATUS_NETWORK_NORMAL;
static int g_slow_count = 0;
static int g_no_rx_count = 0;
static int g_normal_count = 0;

/********************************
*	Function variables
*********************************/
static void stat_pkt_in(struct tcp_sock *sk,
	struct tcphdr *th, struct pkt_stat_swth *stat_ptr, unsigned int len);
static unsigned int idx_update(struct pkt_stat_swth *stat_ptr);
static void stat_pkt_out(struct tcp_sock *sk,
	struct tcphdr *th, struct pkt_stat_swth *stat_ptr, unsigned int len);
#ifdef CONFIG_APP_QOE_AI_PREDICT
static bool is_need_update_app_qoe(struct sk_buff *skb)
{
	struct sock *sk;
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;

	if (g_app_qoe_cycle <= 0 || g_app_qoe_uid <= 0)
		return false;
	if (NULL == skb)
		return false;
	iph = ip_hdr(skb);
	if (NULL == iph)
		return false;
	if (iph->protocol == IPPROTO_TCP) {
		tcph = tcp_hdr(skb);
		if (NULL == tcph)
			return false;
		sk = skb_to_full_sk(skb);
		/*
		 * When in TCP_TIME_WAIT the sk is not a "struct sock" but
		 * "struct inet_timewait_sock" which is missing fields.
		 * So we ignore it.
		 */
		if (sk && sk->sk_state == TCP_TIME_WAIT)
			return false;
		if (sk && sk->sk_uid.val == g_app_qoe_uid)
			return true;
	}
	return false;
}

static void update_tcp_app_qoe_hook_out(struct sk_buff *skb, struct tcphdr *tcph, u32 dataLen)
{
	if (NULL == skb || NULL == tcph || dataLen > MAX_PKT_LEN)
		return;
	if (is_need_update_app_qoe(skb)) {
		if (skb->sk->sk_state == TCP_ESTABLISHED) {
			idx_update(&g_app_qoe_stat);
			stat_pkt_out(skb->sk, tcph, &g_app_qoe_stat, dataLen);
		} else if (skb->sk->sk_state == TCP_SYN_SENT) {
			idx_update(&g_app_qoe_stat);
			if (tcph->syn == 1) {
				g_app_qoe_stat.stat[g_app_qoe_stat.idx].syn++;
			}
		}
	}
}

static void update_tcp_app_qoe_hook_in(struct sk_buff *skb, struct tcphdr *tcph, u32 dataLen)
{
	if (NULL == skb || NULL == tcph || dataLen > MAX_PKT_LEN)
		return;
	if (is_need_update_app_qoe(skb)) {
		idx_update(&g_app_qoe_stat);
		stat_pkt_in(skb->sk, tcph, &g_app_qoe_stat, dataLen);
	}
}
#endif

/*check index*/
static int idx_check(int index)
{
	if(index < 0)
		index = index + MAX_STAT_SEC;

	if(index < 0)
		return  0;

	index = index % MAX_STAT_SEC;

	return  index;
}

/*Smart switching CHR*/
#ifdef CONFIG_CHR_NETLINK_MODULE
int get_rtt_list(struct native_event *rtt_event, unsigned int list_len) {
	int cnt = 0;
	int idx = 0;

	if (list_len > MAX_STAT_SEC || rtt_event == NULL)
		return -1;

	/*Avoiding cache and index timeout for the lack of packets.*/
	idx_update(&g_stat);

	for (cnt = 0; cnt < list_len; cnt++) {
		idx = idx_check(g_stat.idx - cnt);
		rtt_event->rtt_list[cnt] = g_stat.stat[idx].rtt;
		rtt_event->max_list[cnt] = g_stat.stat[idx].rtt_max;
		if(0 != g_stat.stat[idx].rtt_cnt) {
			rtt_event->avg_list[cnt] = g_stat.stat[idx].rtt_all / g_stat.stat[idx].rtt_cnt;
		}else {
			rtt_event->avg_list[cnt] = 0;
		}
	}
	return 0;
}

unsigned int chr_smart_switch(struct chr_para *report)
{
	if (report != NULL && virt_addr_valid(report)) {
		memcpy(report, &g_chr_data, sizeof(g_chr_data));
		memset(&g_chr_data, 0, sizeof(g_chr_data));
	}
	return 0;
}

static void mean_stat_old(void)
{
	int cnt = 0;

	g_stat.idx = idx_check(g_stat.idx);
	g_chr_data.nsi_old = g_stat.norm_idx[g_stat.idx].flt_ksi;
	memset(&(g_chr_data.stat_old), 0, sizeof(struct pkt_cnt_swth));
	for (cnt =0; cnt < MAX_STAT_SEC; cnt++) {
		g_chr_data.stat_old.in_pkt += g_stat.stat[cnt].in_pkt;
		g_chr_data.stat_old.out_pkt += g_stat.stat[cnt].out_pkt;
		g_chr_data.stat_old.in_len += g_stat.stat[cnt].in_len;
		g_chr_data.stat_old.out_len += g_stat.stat[cnt].out_len;
		g_chr_data.stat_old.dupack += g_stat.stat[cnt].dupack;
		g_chr_data.stat_old.rts += g_stat.stat[cnt].rts;
		g_chr_data.stat_old.syn += g_stat.stat[cnt].syn;
		g_chr_data.stat_old.rtt += g_stat.stat[cnt].rtt;
	}
}

static void mean_stat_new(void)
{
	int cnt = 0;

	g_stat.idx = idx_check(g_stat.idx);
	g_chr_data.nsi_new = g_stat.norm_idx[g_stat.idx].flt_ksi;
	memset(&(g_chr_data.stat_new), 0, sizeof(struct pkt_cnt_swth));
	for (cnt = 0; cnt < MAX_STAT_SEC; cnt++) {
		g_chr_data.stat_new.in_pkt += g_stat.stat[cnt].in_pkt;
		g_chr_data.stat_new.out_pkt += g_stat.stat[cnt].out_pkt;
		g_chr_data.stat_new.in_len += g_stat.stat[cnt].in_len;
		g_chr_data.stat_new.out_len += g_stat.stat[cnt].out_len;
		g_chr_data.stat_new.dupack += g_stat.stat[cnt].dupack;
		g_chr_data.stat_new.rts += g_stat.stat[cnt].rts;
		g_chr_data.stat_new.syn += g_stat.stat[cnt].syn;
		g_chr_data.stat_new.rtt += g_stat.stat[cnt].rtt;
	}
}

static int chr_report(unsigned long data)
{
	int rtt = 0;

	g_stat.idx = idx_check(g_stat.idx);
	mean_stat_new();
	g_chr_report.slowType = 3;
	g_chr_report.avgAmp = g_stat.norm_idx[g_stat.idx].flt_ksi;
	rtt = g_chr_data.stat_old.rtt / MAX_STAT_SEC / 10;
	g_chr_report.oldRtt = rtt > MAX_RTT ? MAX_RTT : rtt;
	rtt = g_chr_data.stat_new.rtt / MAX_STAT_SEC / 10;
	g_chr_report.newRtt = rtt > MAX_RTT ? MAX_RTT : rtt;
	nb_notify_event(NBMSG_KSI_EVT, &g_chr_report,
		sizeof(g_chr_report));
	pr_info("KSI chr slow timer report\n");
	return 0;
}

#endif

/*Normalized parameters*/
static int norm_para(const unsigned char norm[],
	const unsigned short idx[], int len, int value)
{
	int cnt = 0;
	int val = 0;
	int min_cnt = 0;
	int min_val;

	min_val = ABS(value - idx[0]);
	for (cnt = 1; cnt < len; cnt++) {
		val = ABS(value - idx[cnt]);
		if (val < min_val) {
			min_cnt = cnt;
			min_val = val;
		}
	}
	return norm[min_cnt];
}

/*Calculate the normalized values of each factor*/
static void calc_norm(struct pkt_stat_swth *stat_ptr, unsigned int idx)
{
	unsigned int div = 0;

	if (NULL == stat_ptr)
		return;

	idx = idx_check(idx);

	div = stat_ptr->stat[idx].in_len * MUL128;
	if(stat_ptr->stat[idx].out_len == 0)
		stat_ptr->stat[idx].out_len++;

	div = div / stat_ptr->stat[idx].out_len;
	if (div > MUL128)
		div = MUL128;

	stat_ptr->norm_idx[idx].inout
		= norm_para(inout_norm, inout_idx, INOUT_LEN, div);

	stat_ptr->norm_idx[idx].rtt
		= norm_para(rtt_norm, rtt_idx, RTT_LEN, stat_ptr->stat[idx].rtt);

	stat_ptr->norm_idx[idx].in_len
		= norm_para(in_norm, in_idx, IN_LEN, stat_ptr->stat[idx].in_len);

	stat_ptr->norm_idx[idx].out_len
		= norm_para(out_norm, out_idx, OUT_LEN, stat_ptr->stat[idx].out_len);

	stat_ptr->norm_idx[idx].in_pkt
		= norm_para(inpkt_norm, inpkt_idx, INPKT_LEN, stat_ptr->stat[idx].in_pkt);

	stat_ptr->norm_idx[idx].out_pkt
		= norm_para(outpkt_norm, outpkt_idx, OUTPKT_LEN, stat_ptr->stat[idx].out_pkt);

	stat_ptr->norm_idx[idx].syn
		= norm_para(syn_norm, syn_idx, SYN_LEN, stat_ptr->stat[idx].syn);

	stat_ptr->norm_idx[idx].rts
		= norm_para(rts_norm, rts_idx, RTS_LEN, stat_ptr->stat[idx].rts);

	stat_ptr->norm_idx[idx].dupack
		= norm_para(dupack_norm, dupack_idx, DUPACK_LEN, stat_ptr->stat[idx].dupack);
}

/*Calculate KSI values*/
static void calc_ksi(struct pkt_stat_swth *stat_ptr, unsigned int idx)
{
	if (NULL == stat_ptr)
		return;

	idx = idx_check(idx);

	stat_ptr->norm_idx[idx].ksi =
		(MUL128 - stat_ptr->norm_idx[idx].inout) * coef[0] +
		stat_ptr->norm_idx[idx].rtt * coef[1] +
		(MUL128 - stat_ptr->norm_idx[idx].in_len) * coef[2] +
		(MUL128 - stat_ptr->norm_idx[idx].in_pkt) * coef[3] +
		(MUL128 - stat_ptr->norm_idx[idx].out_len) * coef[4] +
		(MUL128 - stat_ptr->norm_idx[idx].out_pkt) * coef[5] +
		stat_ptr->norm_idx[idx].dupack * coef[6] +
		stat_ptr->norm_idx[idx].rts * coef[7] +
		stat_ptr->norm_idx[idx].syn * coef[8];

	if (stat_ptr->norm_idx[idx].ksi > MAX_KSI)
		stat_ptr->norm_idx[idx].ksi = 0;
}

/*KSI Filter*/
static void filter_ksi(struct pkt_stat_swth *stat_ptr, unsigned int idx, unsigned int len)
{
	int cnt = 0;

	if (NULL == stat_ptr)
		return;

	idx = idx_check(idx);
	if (len > MAX_STAT_SEC)
		len = MAX_STAT_SEC;

	for (cnt = 0; cnt <= len; cnt++) {
		stat_ptr->norm_idx[idx_check(idx+cnt)].flt_ksi =
			stat_ptr->norm_idx[idx_check(idx+cnt-1)].flt_ksi
			- stat_ptr->norm_idx[idx_check(idx+cnt-1)].flt_ksi / FILT_20
			+ stat_ptr->norm_idx[idx_check(idx+cnt-1)].ksi / FILT_20;
	}
}

/*Calculate KSI values*/
static void detect_ksi(void)
{
	int cnt = 0;
	int idx_start = -1;
	int idx_dura = -1;
	int idx_tmp = 0;
	int Amp = 0;

	g_stat.idx = idx_check(g_stat.idx);
	if (g_stat.norm_idx[g_stat.idx].flt_ksi > KSI_THRED)
		idx_start = g_stat.idx - MAX_STAT_SEC;

	for (cnt = 1; cnt < MAX_STAT_SEC - 1; cnt++) {
		idx_tmp = idx_check(g_stat.idx - cnt);
		if (g_stat.norm_idx[idx_tmp].flt_ksi <= KSI_THRED &&
			g_stat.norm_idx[idx_check(idx_tmp - 1)].flt_ksi > KSI_THRED) {

			Amp = g_stat.norm_idx[idx_tmp].flt_ksi;
			idx_start = idx_tmp;
			idx_dura = 1;
		}

		if (g_stat.norm_idx[idx_tmp].flt_ksi >= KSI_THRED &&
			g_stat.norm_idx[idx_check(idx_tmp-1)].flt_ksi < KSI_THRED) {

			if (idx_dura < MAX_THRED) {
				Amp = 0;
				idx_start = -1;
				idx_dura = -1;
			} else {
				break;
			}
		}

		if (g_stat.norm_idx[idx_tmp].flt_ksi >= KSI_THRED) {
			Amp += g_stat.norm_idx[idx_tmp].flt_ksi;
			idx_dura++;
		}
	}

	if (idx_dura >= MAX_THRED) {
		memset(&g_swth_report, 0, sizeof(g_swth_report));
		g_swth_report.slowType = 1;
		g_swth_report.avgAmp = Amp / idx_dura / MUL128;
		g_swth_report.duration = idx_dura;
		g_swth_report.timeStart = idx_check(g_stat.idx - idx_start);
		if (before(g_report_time + REPORT_TIMER*HZ, jiffies) && g_report_flag) {

			nb_notify_event(NBMSG_KSI_EVT, &g_swth_report,
				sizeof(g_swth_report));
			pr_info("KSI network slow %d\n", g_swth_report.avgAmp);
			g_report_time = jiffies;
#ifdef CONFIG_CHR_NETLINK_MODULE
			mean_stat_old();
			mod_timer(&g_chr_rpt_timer, jiffies + CHR_REPORT_TIMER);
#endif
		}
	}
}

/*Update KSI values*/
static int ksi_update(struct pkt_stat_swth *stat_ptr)
{
	int cnt = 0;
	int idx_cur = 0;
	int idx_prv = 0;

	if (NULL == stat_ptr)
		return;

	stat_ptr->idx = idx_check(stat_ptr->idx);
	calc_norm(stat_ptr, stat_ptr->idx);
	calc_ksi(stat_ptr, stat_ptr->idx);
	if (stat_ptr->stat[stat_ptr->idx].rtt < RTT_THRED) {

		filter_ksi(stat_ptr, stat_ptr->idx, 0);
		return -1;
	}

	for (cnt = 0; cnt < EXTERN_TIME; cnt++) {
		idx_cur = idx_check(stat_ptr->idx - cnt);
		idx_prv = idx_check(stat_ptr->idx - cnt - 1);
		if (stat_ptr->stat[idx_prv].rtt == 0) {
			stat_ptr->stat[idx_prv].rtt = stat_ptr->stat[stat_ptr->idx].rtt;
			calc_norm(stat_ptr, idx_prv);
			calc_ksi(stat_ptr, idx_prv);
		} else {
			filter_ksi(stat_ptr, idx_cur, cnt);
			break;
		}
	}

	if (&g_stat == stat_ptr)
		detect_ksi();

	return 0;
}

/*Statistic Input Package Parameters*/
static void stat_pkt_in(struct tcp_sock *sk,
	struct tcphdr *th, struct pkt_stat_swth *stat_ptr, unsigned int len)
{
	unsigned int rtt_ms;
	u32 prior_snd_una = 0;
	u32 ack = 0;
	bool is_dupack = false;
	int prior_packets = 0;

	if(th == NULL || sk == NULL || stat_ptr == NULL)
		return;

	stat_ptr->idx = idx_check(stat_ptr->idx);
	stat_ptr->stat[stat_ptr->idx].in_pkt++;
	stat_ptr->stat[stat_ptr->idx].in_len += len;

	prior_snd_una = sk->snd_una;
	ack = ntohl(th->ack_seq);
	prior_packets = sk->packets_out;/* Packets which are "in flight" */
	if ((!prior_packets) || (len > sk->tcp_header_len)) {
		is_dupack = false;
	} else if (ack == prior_snd_una) {
		is_dupack = true;
	}
	if (true == is_dupack)
		stat_ptr->stat[stat_ptr->idx].dupack++;

	rtt_ms = (sk->srtt_us) / US_MS / 8;

	/*RTT update*/
	if (&g_stat == stat_ptr) {
		g_rtt = g_rtt + rtt_ms / FILT_16 - g_rtt / FILT_16;
		if (stat_ptr->stat[stat_ptr->idx].rtt < g_rtt)
			stat_ptr->stat[stat_ptr->idx].rtt = g_rtt;
	} else {
		if (0 == g_app_rtt) {
			g_app_rtt = rtt_ms;
		} else {
			g_app_rtt = g_app_rtt + rtt_ms / FILT_16 - g_app_rtt / FILT_16;
		}
		if (stat_ptr->stat[stat_ptr->idx].rtt < g_app_rtt)
			stat_ptr->stat[stat_ptr->idx].rtt = g_app_rtt;
	}

	stat_ptr->stat[stat_ptr->idx].rtt_all += rtt_ms;
	stat_ptr->stat[stat_ptr->idx].rtt_cnt++;

	if (stat_ptr->stat[stat_ptr->idx].rtt_max < rtt_ms)
		stat_ptr->stat[stat_ptr->idx].rtt_max = rtt_ms;
}

/*Statistic Output Package Parameters*/
static void stat_pkt_out(struct tcp_sock *sk,
	struct tcphdr *th, struct pkt_stat_swth *stat_ptr, unsigned int len)
{
	if(th == NULL || sk == NULL || stat_ptr == NULL)
		return;

	stat_ptr->idx = idx_check(stat_ptr->idx);
	stat_ptr->stat[stat_ptr->idx].out_pkt++;
	stat_ptr->stat[stat_ptr->idx].out_len += len;

	if (before(ntohl(th->seq), sk->snd_nxt))
		stat_ptr->stat[stat_ptr->idx].rts++;
}

#ifdef CONFIG_APP_QOE_AI_PREDICT
static void hidata_app_qoe_ai_predict(void)
{
	int para[10] = {0};
	int sum_num = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int loop_max = APP_QOE_MIN_STAT_SEC;
	unsigned int num_max = 0;
	unsigned int idx_tmp = 0;
	unsigned int *ptr = NULL;
	unsigned int *ptr1 = NULL;
	struct pkt_cnt_swth stat = {0};
	struct report_slow_para report_para = {0};
	/*point to gIntFixParam[SUB_CLFS_NUM_MAX][4], i.e g_int_fix_param: global variable.
	  array pointer from clf_params.h.*/
	int (*ppg_int_fix_param)[4] = g_int_fix_param;
	classifier_info_int_exp_type clf_info = (classifier_info_int_exp_type){ppg_int_fix_param, ADA_SUB_CLFS_NUM};
	/*meaning: FALSE: not fakeBts; 1.0: all as trueBts.*/
	judge_rlt_info_int_calc_type rlt_info1 = {FALSE, 0, 0.0, 1.0, 0, ADA_SUB_CLFS_NUM};

	if (g_app_qoe_cycle > 0) {
		if (g_app_qoe_cycle < APP_QOE_MIN_STAT_SEC)
			loop_max = APP_QOE_MIN_STAT_SEC;
		else if (g_app_qoe_cycle < MAX_STAT_SEC)
			loop_max = g_app_qoe_cycle;
	}

	sum_num = (int)loop_max;
	ptr = &(stat.in_pkt);
	for (i = 1; i <= loop_max; i++) {
		idx_tmp = idx_check(g_app_qoe_stat.idx - i);
		ptr1 = &(g_app_qoe_stat.stat[idx_tmp].in_pkt);
		if ((g_app_qoe_stat.stat[idx_tmp].in_pkt + g_app_qoe_stat.stat[idx_tmp].out_pkt
			+ g_app_qoe_stat.stat[idx_tmp].syn) == 0) {
			sum_num--;
			continue;
		}
		for (j = 0; j < (sizeof(struct pkt_cnt_swth)/sizeof(unsigned int)); j++) {
			*(ptr + j) += *(ptr1 + j);
		}
	}
	if (sum_num <= 0) {
		pr_info("hidata_app_qoe_ai_predict,sum_num is 0");
		g_normal_num = 0;
		g_slow_num = 0;
		g_no_rx_num = 0;
		return;
	}
	for (j = 0; j < (sizeof(struct pkt_cnt_swth)/sizeof(unsigned int)); j++) {
		unsigned int temp = *(ptr+j);
		if (temp >= sum_num || 0 == temp) {
			*(ptr+j) = temp/sum_num;
		} else {
			*(ptr+j) = 1;
		}
	}
	/*10 parameters: RTT,in,inpkt,out,outpkt,dack,rts,syn,3Gsignal,4Gsignal
	*/
	para[0] = stat.rtt;
	para[1] = stat.in_len;
	para[2] = stat.in_pkt;
	para[3] = stat.out_len;
	para[4] = stat.out_pkt;
	para[5] = stat.dupack;
	para[6] = stat.rts;
	para[7] = stat.syn;
	para[8] = g_dbm;//3Gsingnal
	para[9] = g_rsrp;
	pr_info("[AppQoe]ai_predict params[]=%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
		para[0], para[1], para[2], para[3], para[4],
		para[5], para[6], para[7], para[8], para[9]);

	rlt_info1 = judge_single_smp_using_clf_info_int_exp(clf_info, para, g_app_qoe_level);

	if ((stat.out_pkt + stat.syn) > 0 && stat.in_pkt == 0) {
		pr_info("[AppQoe]ai_predict no rx");
		g_no_rx_num++;
		g_slow_num = 0;
		g_normal_num = 0;
		rlt_info1.is_ps_slow = TRUE;
	} else if (TRUE == rlt_info1.is_ps_slow) {
		g_slow_num++;
		g_normal_num = 0;
		g_no_rx_num = 0;
	} else if (FALSE == rlt_info1.is_ps_slow) {
		g_normal_num++;
		g_slow_num = 0;
		g_no_rx_num = 0;
	}

	pr_info("[AppQoe]app qoe AI predict result =%d,g_app_qoe_level=%d,g_slow_num=%d,g_normal_num=%d,g_no_rx_num=%d\n",
		rlt_info1.is_ps_slow, g_app_qoe_level, g_slow_num, g_normal_num, g_no_rx_num);
	num_max = g_app_qoe_cycle < APP_QOE_MIN_STAT_SEC ? APP_QOE_MIN_STAT_SEC : g_app_qoe_cycle;

	if ((2*num_max <= g_no_rx_num && NETWORK_STATUS_APP_QOE_GENERAL_SLOW != g_app_qoe_level)
		|| (num_max <= g_slow_num && NETWORK_STATUS_APP_QOE_GENERAL_SLOW != g_app_qoe_level)
		|| (num_max <= g_normal_num && NETWORK_STATUS_APP_QOE_NORMAL != g_app_qoe_level)) {
		g_app_qoe_level = (rlt_info1.is_ps_slow == TRUE ?
				 NETWORK_STATUS_APP_QOE_GENERAL_SLOW : NETWORK_STATUS_APP_QOE_NORMAL);
		report_para.slowType = g_app_qoe_level;
		report_para.avgAmp = 0;
		report_para.duration = g_app_qoe_cycle;
		report_para.timeStart = 1;
		nb_notify_event(NBMSG_KSI_EVT, &report_para,
			sizeof(report_para));
		g_normal_num = 0;
		g_slow_num = 0;
		g_no_rx_num = 0;
		pr_info("[AppQoe]hidata_app_qoe_ai_predict,slowType=%d\n", report_para.slowType);
	}
}
#endif

/* fnetwork_slow_chr */
static void predict_network_normal_or_slow(void)
{
	int para[10] = {0};
	int sum_num = 0;
	unsigned int i = 0;
	unsigned int j = 0;
	unsigned int loop_max = 5;
	unsigned int idx_tmp = 0;
	unsigned int *ptr = NULL;
	unsigned int *ptr1 = NULL;
	struct pkt_cnt_swth stat = {0};
	struct report_slow_para report_para = {0};
	/*point to gIntFixParam[SUB_CLFS_NUM_MAX][4], i.e g_int_fix_param: global variable.
	  array pointer from clf_params.h.*/
	int (*ppg_int_fix_param)[4] = g_int_fix_param;
	classifier_info_int_exp_type clf_info = (classifier_info_int_exp_type){ppg_int_fix_param, ADA_SUB_CLFS_NUM};
	/*meaning: FALSE: not fakeBts; 1.0: all as trueBts.*/
	judge_rlt_info_int_calc_type rlt_info1 = {FALSE, 0, 0.0, 1.0, 0, ADA_SUB_CLFS_NUM};

	sum_num = loop_max;
	ptr = &(stat.in_pkt);
	for (i = 1; i <= loop_max; i++) {
		idx_tmp = idx_check(g_stat.idx - i);
		ptr1 = &(g_stat.stat[idx_tmp].in_pkt);
		if ((g_stat.stat[idx_tmp].in_pkt + g_stat.stat[idx_tmp].out_pkt
			+ g_stat.stat[idx_tmp].syn) == 0) {
			sum_num--;
			continue;
		}
		for (j = 0; j < (sizeof(struct pkt_cnt_swth)/sizeof(unsigned int)); j++) {
			*(ptr + j) += *(ptr1 + j);
		}
	}
	if (sum_num <= 0) {
		g_normal_count  = 0;
		g_slow_count  = 0;
		g_no_rx_count  = 0;
		return;
	}
	for (j = 0; j < (sizeof(struct pkt_cnt_swth)/sizeof(unsigned int)); j++) {
		unsigned int temp = *(ptr+j);
		if (temp >= sum_num || 0 == temp) {
			*(ptr+j) = temp/sum_num;
		} else {
			*(ptr+j) = 1;
		}
	}
	/*10 parameters: RTT,in,inpkt,out,outpkt,dack,rts,syn,3Gsignal,4Gsignal
	*/
	para[0] = stat.rtt;
	para[1] = stat.in_len;
	para[2] = stat.in_pkt;
	para[3] = stat.out_len;
	para[4] = stat.out_pkt;
	para[5] = stat.dupack;
	para[6] = stat.rts;
	para[7] = stat.syn;
	para[8] = g_dbm;//3Gsingnal
	para[9] = g_rsrp;

	rlt_info1 = judge_single_smp_using_clf_info_int_exp(clf_info, para, g_network_status );

	if ((stat.out_pkt + stat.syn) > 0 && stat.in_pkt == 0) {
		pr_info("ai_predict no rx");
		g_no_rx_count++;
		g_slow_count = 0;
		g_normal_count = 0;
		rlt_info1.is_ps_slow = TRUE;
	} else if (TRUE == rlt_info1.is_ps_slow) {
		g_slow_count++;
		g_normal_count = 0;
		g_no_rx_count = 0;
	} else if (FALSE == rlt_info1.is_ps_slow) {
		g_normal_count++;
		g_slow_count = 0;
		g_no_rx_count = 0;
	}

	if ((2*g_network_detect_window <= g_no_rx_count && NETWORK_STATUS_NETWORK_SLOW != g_network_status )
		|| (g_network_detect_window <= g_slow_count && NETWORK_STATUS_NETWORK_SLOW != g_network_status )
		|| (g_network_detect_window <= g_normal_count && NETWORK_STATUS_NETWORK_NORMAL != g_network_status )) {
		g_network_status  = (rlt_info1.is_ps_slow == TRUE ?
			NETWORK_STATUS_NETWORK_SLOW : NETWORK_STATUS_NETWORK_NORMAL);
		report_para.slowType = g_network_status ;
		nb_notify_event(NBMSG_KSI_EVT, &report_para,sizeof(report_para));
		g_normal_count  = 0;
		g_slow_count = 0;
		g_no_rx_count = 0;
		pr_info("predict_network_normal_or_slow,slowType=%d\n", report_para.slowType);
	}
}

/*Update the index for the statistics array*/
static unsigned int idx_update(struct pkt_stat_swth *stat_ptr)
{
	unsigned long time_stamp;
	unsigned long time_stamp_tmp;
	unsigned int index;
	unsigned int index_tmp;
	int cnt = 0;

	if (NULL == stat_ptr)
		return S_OLD_SECOND;

	time_stamp = jiffies / HZ;
	time_stamp_tmp = time_stamp;
	index = time_stamp % MAX_STAT_SEC;
	index_tmp = index;

	if (index != stat_ptr->idx || time_stamp != stat_ptr->time_stamp) {

		for (cnt = 1; cnt <= time_stamp - stat_ptr->time_stamp; cnt++) {

			index_tmp = (stat_ptr->idx+cnt) % MAX_STAT_SEC;
			time_stamp_tmp = stat_ptr->time_stamp+cnt;

			memset(&(stat_ptr->stat[index_tmp]), 0,
				sizeof(struct pkt_cnt_swth));

			if (time_stamp_tmp == time_stamp || cnt >= MAX_STAT_SEC)
				break;
		}
		ksi_update(stat_ptr);

#ifdef CONFIG_APP_QOE_AI_PREDICT
		if (&g_app_qoe_stat == stat_ptr) {
			hidata_app_qoe_ai_predict();
		}
#endif
        /* network_slow_chr */
        if (&g_stat == stat_ptr) {
            predict_network_normal_or_slow();
        }
		stat_ptr->idx = index;
		stat_ptr->time_stamp = time_stamp;

		return S_NEW_SECOND;
	}

	return S_OLD_SECOND;
}

/*Local out hook function*/
static unsigned int hook_out(const struct nf_hook_ops *ops,
		struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	char *pTcpData = NULL;
	u32 totalLen, ipHdrLen, dataLen;

	if (skb == NULL)
		return NF_ACCEPT;

	if (g_hook_flag == S_HOOK_DISABLE)
		return NF_ACCEPT;

	iph = ip_hdr(skb);
	if (iph == NULL)
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP) {

		tcph = tcp_hdr(skb);

		if (NULL == tcph || NULL == skb->data || 0 == tcph->doff)
			return NF_ACCEPT;

		pTcpData = (char *)((u32 *)tcph + tcph->doff);
		if (!virt_addr_valid(pTcpData) && !virt_addr_valid(pTcpData + MAX_PKT_LEN))
			return NF_ACCEPT;

		totalLen = skb->len - skb->data_len;
		ipHdrLen = (pTcpData - (char *)iph);
		dataLen = totalLen - ipHdrLen;

		if (totalLen > MAX_PKT_LEN || ipHdrLen > MAX_PKT_LEN || dataLen > MAX_PKT_LEN)
			return NF_ACCEPT;

		if (NULL == skb->dev || NULL == skb->dev->name)
			return NF_ACCEPT;

		if (0 != strncmp(skb->dev->name, DS_NET, DS_NET_LEN)
			&& 0 != strncmp(skb->dev->name, DS_NET_SLAVE, DS_NET_LEN))
			return NF_ACCEPT;

		if (skb->sk == NULL)
			return NF_ACCEPT;

		/*When the lock is not locked, the lock is triggered*/
		if (!spin_trylock_bh(&g_smart_switch_lock))
			return NF_ACCEPT;

		if (skb->sk->sk_state == TCP_ESTABLISHED) {
			idx_update(&g_stat);
			stat_pkt_out(skb->sk, tcph, &g_stat, dataLen);
		} else if (skb->sk->sk_state == TCP_SYN_SENT) {
			if (tcph->syn == 1)
				g_stat.stat[g_stat.idx].syn++;
		}
#ifdef CONFIG_APP_QOE_AI_PREDICT
		update_tcp_app_qoe_hook_out(skb, tcph, dataLen);
#endif
		spin_unlock_bh(&g_smart_switch_lock);
	}
	return NF_ACCEPT;
}

/*Local in hook function*/
static unsigned int hook_in(const struct nf_hook_ops *ops,
		struct sk_buff *skb, const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	char *pTcpData = NULL;
	u32 totalLen, ipHdrLen, dataLen;

	if (skb == NULL)
		return NF_ACCEPT;

	if (g_hook_flag == S_HOOK_DISABLE)
		return NF_ACCEPT;

	iph = ip_hdr(skb);
	if (iph == NULL)
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP) {

		tcph = tcp_hdr(skb);

		if (NULL == tcph || NULL == skb->data || 0 == tcph->doff)
			return NF_ACCEPT;

		pTcpData = (char *)((u32 *)tcph + tcph->doff);
		if (!virt_addr_valid(pTcpData) && !virt_addr_valid(pTcpData + MAX_PKT_LEN))
			return NF_ACCEPT;

		totalLen = skb->len - skb->data_len;
		ipHdrLen = (pTcpData - (char *)iph);
		dataLen = totalLen - ipHdrLen;

		if (totalLen > MAX_PKT_LEN || ipHdrLen > MAX_PKT_LEN || dataLen > MAX_PKT_LEN)
			return NF_ACCEPT;

		if (NULL == skb->dev || NULL == skb->dev->name)
			return NF_ACCEPT;

		if (0 != strncmp(skb->dev->name, DS_NET, DS_NET_LEN)
			&& 0 != strncmp(skb->dev->name, DS_NET_SLAVE, DS_NET_LEN))
			return NF_ACCEPT;

		if (skb->sk == NULL)
			return NF_ACCEPT;

		if (skb->sk->sk_state != TCP_ESTABLISHED)
			return NF_ACCEPT;

		if (!spin_trylock_bh(&g_smart_switch_lock))
			return NF_ACCEPT;

		idx_update(&g_stat);
		stat_pkt_in(skb->sk, tcph, &g_stat, dataLen);
#ifdef CONFIG_APP_QOE_AI_PREDICT
		update_tcp_app_qoe_hook_in(skb, tcph, dataLen);
#endif
		spin_unlock_bh(&g_smart_switch_lock);
	}
	return NF_ACCEPT;
}

static struct nf_hook_ops net_hooks[] = {
	{
		.hook		= hook_in,
		.pf		= PF_INET,
		.hooknum	= NF_INET_LOCAL_IN,
		.priority	= NF_IP_PRI_FILTER - 1,
	},
	{
		.hook		= hook_out,
		.pf		= PF_INET,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_FILTER - 1,
	}
};

/*Set KSI and hook-and-package points to enable*/
int set_ksi_enable(uint8_t nf_hook_enable, uint8_t nl_event_enable)
{
	spin_lock_bh(&g_smart_switch_lock);
	g_report_flag = nl_event_enable;
	g_hook_flag = nf_hook_enable;
	spin_unlock_bh(&g_smart_switch_lock);

	return 0;
}

#ifdef CONFIG_APP_QOE_AI_PREDICT
int set_app_qoe_uid(int uid, int period)
{
	pr_info("[AppQoe]set_app_qoe_uid uid=%d,period=%d\n", uid, period);
	spin_lock_bh(&g_smart_switch_lock);
	g_app_qoe_uid = uid;
	memset(&g_app_qoe_stat, 0, sizeof(g_app_qoe_stat));
	g_app_qoe_level = NETWORK_STATUS_INVALID;
	g_app_rtt = 0;
	g_slow_num = 0;
	g_no_rx_num = 0;
	g_normal_num = 0;
	g_app_qoe_cycle = period/1000;/*seconds*/
	spin_unlock_bh(&g_smart_switch_lock);
	return 0;
}

int set_app_qoe_rsrp(int rsrp, int rsrq)
{
	g_rsrp = rsrp;
	g_dbm = rsrq;
	return 0;
}
#endif

/*initialization function*/
int smart_switch_init(void)
{
	int ret = -1;

	g_report_time = jiffies;
	g_report_flag = S_MODULE_DISABLE;
	g_hook_flag = S_HOOK_DISABLE;
	g_rtt = 0;
	memset(&g_stat, 0, sizeof(g_stat));
	memset(&g_swth_report, 0, sizeof(g_swth_report));
#ifdef CONFIG_APP_QOE_AI_PREDICT
	g_app_rtt = 0;
	memset(&g_app_qoe_stat, 0, sizeof(g_app_qoe_stat));
#endif
	spin_lock_init(&g_smart_switch_lock);
	/*Registration hook function*/
	ret = nf_register_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	if (ret) {
		pr_err("KSI init fail ret=%d\n", ret);
		return ret;
	}

#ifdef CONFIG_CHR_NETLINK_MODULE
	/*Timer initialization*/
	init_timer(&g_chr_rpt_timer);
	g_chr_rpt_timer.data = 0;
	g_chr_rpt_timer.function = chr_report;
	g_chr_rpt_timer.expires = jiffies + CHR_REPORT_TIMER;
	memset(&g_chr_report, 0, sizeof(g_chr_report));
#endif

	pr_info("KSI init success\n");
	return 0;
}

/*Exit function*/
void smart_switch_exit(void)
{
	nf_unregister_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	pr_err("KSI exit success\n");
}

#undef DEBUG