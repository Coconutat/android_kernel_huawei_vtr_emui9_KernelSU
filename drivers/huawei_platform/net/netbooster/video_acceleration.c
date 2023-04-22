
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

#include "video_acceleration.h"
#include "nb_netlink.h"

#ifndef DEBUG
#define DEBUG
#endif


/********************************
*	Defines global variables
*********************************/
/*Storage uplink packet statistics, Including on downlink rate,
  *uplink and downlink rate and abnormal packet rate, and so on.
  */
static struct pkt_stat g_stat;

/*Storage ALPHA filtering after the bit rate information.*/
static struct filter_para g_filter;

/*Configured to store multicast status information.*/
static struct video_state g_video_stat;

/*Storage flow information such as the IP address and source port number.*/
static struct stream_info g_stream;

/*Storage video segment information.*/
static struct seg_info g_seg;

/*Segment information reporting timer*/
static struct timer_list g_seg_rpt_timer;

/*Video parameter Report Structure*/
static struct video_parameter g_report;

/*CHR Report Structure*/
static struct video_chr_para g_video_chr;

/*Input and output data processing of the lock*/
static spinlock_t g_video_lock;

/*Bit rate information*/
static unsigned int g_bit_rate;

/*Video acceleration is enabled identifier*/
static uint8_t g_report_flag;

/*Hook is enabled identifier*/
static uint8_t g_hook_flag;

/*Timer is enabled identifier*/
static unsigned int g_timer_flag;


/********************************
*	Defines function
*********************************/

/*For CHR measurement parameter obtaining*/
int chr_video_stat(struct video_chr_para *report)
{
	spin_lock_bh(&g_video_lock);

	if (report != NULL && virt_addr_valid(report)) {
		memcpy(report, &g_video_chr, sizeof(struct video_chr_para));
		memset(&g_video_chr, 0, sizeof(struct video_chr_para));
	}

	spin_unlock_bh(&g_video_lock);
	return 0;
}

/*For CHR measurement parameter Statistics*/
static void chr_stat(unsigned int flag)
{
	if (g_report_flag) {

		g_video_chr.vod_avg_speed = g_video_chr.vod_avg_speed
			+ g_bit_rate / MULTIPLIER_X128
			- g_video_chr.vod_avg_speed / MULTIPLIER_X128;
		g_video_chr.vod_time++;
	} else {

		g_video_chr.uvod_avg_speed = g_video_chr.uvod_avg_speed
			+ g_bit_rate / MULTIPLIER_X128
			- g_video_chr.uvod_avg_speed / MULTIPLIER_X128;
		g_video_chr.uvod_time++;
	}

	if (flag == 0)
		return;

	if (g_report_flag)
		g_video_chr.vod_freez_num++;
	else
		g_video_chr.uvod_freez_num++;
}

/*Determine if the video is not fluent*/
static unsigned int freez(unsigned int idx)
{
	unsigned long speed_x128;
	unsigned long speed_norm0;
	unsigned long speed_norm;
	unsigned int err_pkt;

	speed_x128 = 0;
	speed_norm0 = 0;
	speed_norm = 0;
	err_pkt = 0;
	/*From the video stream feature detection added after the video
	*frame freezing video scheduling
	*/
	idx = idx % MAX_STAT_SEC;
	speed_x128 = (g_stat.stat[idx].tin_len * MULTIPLIER_X128);

	speed_norm0 = g_filter.talp;
	if (speed_norm0 < MULTIPLIER_X128)
		speed_norm0 = MULTIPLIER_X128;
	speed_norm0 = speed_x128 / speed_norm0;

	speed_norm = speed_norm0 * speed_norm0 / MULTIPLIER_X128;
	g_filter.alp_x64 = g_filter.alp_x64 + speed_norm
		- g_filter.alp_x64 / MULTIPLIER_X64;

	err_pkt = (g_stat.stat[idx].rts + g_stat.stat[idx].disodr);
	if ((g_stat.stat[idx].in_pkt > DOWN_PKT_THRESHOLD
		&& err_pkt > ERR_PKT_THRESHOLD
		&& err_pkt * MULTIPLIER_X10 > g_stat.stat[idx].in_pkt)
		|| g_filter.alp_x64 > VIDEO_THRESHOLD) {

		if (g_filter.mult < MULTIPLIER_X20)
			g_filter.mult++;

		g_bit_rate = g_filter.talp
			+ g_filter.talp / MULTIPLIER_X5 * g_filter.mult;
		if (g_seg.seg_remain > 0)
			g_seg.seg_remain--;

		return 1;
	}

	if (g_filter.mult > 1)
		g_filter.mult--;

	g_bit_rate = g_filter.talp
		+ g_filter.talp / MULTIPLIER_X5 * g_filter.mult;

	if (g_seg.seg_remain < MAX_REMAIN_TIME)
		g_seg.seg_remain++;
	return 0;
}

/*Statistics bit rate information, In video
*frame freezing scenario increases the bit rate
*/
static unsigned int bit_rate(unsigned int idx)
{
	unsigned int is_freez;

	is_freez = 0;

	if (idx == 0)
		idx = MAX_STAT_SEC-1;
	else if (idx >= MAX_STAT_SEC)
		idx = 0;
	else
		idx = idx-1;

	/*Alpha Filter Statistical downlink rate (TCP/UDP)*/
	g_filter.talp = g_filter.talp + g_stat.stat[idx].tin_len / MULTIPLIER_X20
		- g_filter.talp / MULTIPLIER_X20;

	g_filter.ualp = g_filter.ualp + g_stat.stat[idx].uin_len / MULTIPLIER_X20
		- g_filter.ualp / MULTIPLIER_X20;

	is_freez = freez(idx);

	/*When the network traffic status changes,
	*Switch the current video status parameters
	*/
	g_video_stat.t_u_flag = STREAM_TCP;
	if (g_filter.talp < g_filter.ualp &&
		g_filter.ualp > UDP_PKT_THRESHOLD &&
		g_video_stat.t_stat == VIDEO_STATUS_END) {

		g_video_stat.t_u_flag = STREAM_UDP;
		g_bit_rate = g_filter.ualp + g_filter.ualp / 2;
	}
	chr_stat(is_freez);
	return 0;
}


/*Video parameters of the storage array index update,
 *At the current moment is a second new, Update structure index
 */
static unsigned int idx_update(void)
{
	unsigned int index, index_tmp;
	unsigned long time_stamp, time_stamp_tmp;
	unsigned int cnt;

	g_timer_flag = TIMER_CNT;

	time_stamp = jiffies / HZ;
	index = time_stamp % MAX_STAT_SEC;
	index_tmp = 0;
	time_stamp_tmp = 0;
	cnt = 0;
	if (index != g_stat.idx || time_stamp != g_stat.time_stamp) {

		for (cnt = 1; cnt <= time_stamp - g_stat.time_stamp; cnt++) {

			index_tmp = (g_stat.idx+cnt) % MAX_STAT_SEC;
			time_stamp_tmp = g_stat.time_stamp+cnt;

			memset(&(g_stat.stat[index_tmp]), 0,
				sizeof(struct pkt_cnt));

			/*Update index, Until the array index of the
			*	time to the current second or all zero.
			*/
			if (time_stamp_tmp == time_stamp ||
				cnt >= MAX_STAT_SEC) {

			/*Current packet is a integer
			* second. speed to be counted
			*/
				bit_rate(index_tmp);
				break;
			}
			bit_rate(index_tmp);
		}

		g_stat.idx = index;
		g_stat.time_stamp = time_stamp;

		return NEW_SECOND;
	}

	return OLD_SECOND;
}

/*Statistics downlink TCP package rate and other indicators*/
static int tcp_in_cnt(struct tcp_sock *sk,
	unsigned int len, unsigned int req)
{
	if (g_stat.idx >= MAX_STAT_SEC)
		g_stat.idx = 0;

	g_stat.stat[g_stat.idx].in_pkt++;
	g_stat.stat[g_stat.idx].tin_len += len;

	/* Req is the last ordinal number of the current receive packet,
	* RCV_NXT is the next received packet ordinal,
	* if the current packet req is our RCV_NXT number before the packet,
	* the current packet Req-len can calculate the first byte of the packet req,
	* if larger than RCV_NXT, This packet is a messy packet.
	*/

	if (before(req, sk->rcv_nxt))
		g_stat.stat[g_stat.idx].rts++;

	if (before(sk->rcv_nxt+len, req))
		g_stat.stat[g_stat.idx].disodr++;

	return 0;
}


/*TCP flow information extraction*/
static int t_stream_info(unsigned short port, unsigned int ip)
{
	if (ntohl(ip) == LOOP_IP)
		return -1;

	g_stream.t_num = (g_stream.t_num+1) % PKT_VECTOR_LEN;
	if (g_stream.t_num >= PKT_VECTOR_LEN)
		g_stream.t_num = 0;

	g_stream.t_port[g_stream.t_num] = port;
	g_stream.t_ip[g_stream.t_num] = ip;
	return 0;
}

/*UDP  flow information extraction*/
static int u_stream_info(unsigned short port,
	unsigned int ip, unsigned int len)
{
	int cnt;
	int max_num, min_num;
	int max_idx, min_idx;

	max_num = 0;
	max_idx = 0;
	min_num = g_stream.u_cnt[0];
	min_idx = 0;
	cnt = 0;

	if (ntohl(ip) == LOOP_IP)
		return -1;

	for (cnt = 0; cnt < PKT_VECTOR_LEN; cnt++) {

		if (before(g_stream.u_time_stamp[cnt] + STREAM_END_TIMER, jiffies)) {
			g_stream.u_port[cnt] = 0;
			g_stream.u_ip[cnt] = 0;
			g_stream.u_cnt[cnt] = 0;
		}
	}

	for (cnt = 0; cnt < PKT_VECTOR_LEN; cnt++) {

		if (g_stream.u_port[cnt] == port && g_stream.u_ip[cnt] == ip) {

			g_stream.u_cnt[cnt]++;
			g_stream.u_time_stamp[cnt] = jiffies;
			break;
		}

		if (g_stream.u_cnt[cnt] > max_num) {
			max_num = g_stream.u_cnt[cnt];
			max_idx = cnt;
		}

		if (g_stream.u_cnt[cnt] < min_num) {
			min_num = g_stream.u_cnt[cnt];
			min_idx = cnt;
		}

		if (cnt == PKT_VECTOR_LEN-1) {
			g_stream.u_port[min_idx] = port;
			g_stream.u_ip[min_idx] = ip;
			g_stream.u_cnt[min_idx] = 1;
			g_stream.u_time_stamp[min_idx] = jiffies;
		}
	}
	g_stream.u_num = max_idx;

	g_stat.stat[g_stat.idx].uin_pkt++;
	g_stat.stat[g_stat.idx].uin_len += len;
	return 0;
}

/*Video status changes are reported*/
static unsigned int report_stat(void)
{
	if (!g_report_flag)
		return 1;

	if (g_video_stat.t_u_flag == STREAM_TCP) {

		g_stream.t_num = g_stream.t_num % PKT_VECTOR_LEN;
		g_report.segDuration = DURATION_TIME;
		g_report.videoIP = (g_stream.t_ip[g_stream.t_num] | IP_MASK);
		g_report.videoPort = g_stream.t_port[g_stream.t_num];
		g_report.videoStatus = g_video_stat.t_stat;
	} else {

		g_stream.u_num = g_stream.u_num % PKT_VECTOR_LEN;
		g_report.segDuration = DURATION_TIME;
		g_report.videoIP = (g_stream.u_ip[g_stream.t_num] | IP_MASK);
		g_report.videoPort = g_stream.u_port[g_stream.t_num];
		g_report.videoStatus = g_video_stat.u_stat;
	}

	g_report.aveCodeRate = g_bit_rate;
	g_report.segIndex = g_seg.seg_idx++;
	g_report.videoProtocol = VIDEO_PROTOCOL_HLS;
	g_report.videoRemainingPlayTime = g_seg.seg_remain;
	g_report.videoSegState = CHOICE_RESULT_VIDEO_CHANGE;

	nb_notify_event(NBMSG_VOD_EVT, &g_report, sizeof(g_report));
	pr_info("VOD media report status:%d\n", g_report.videoStatus);

	return 0;
}

/*UDP protocol video playback status maintenance function*/
static void u_video_state(void)
{
	int cnt;
	unsigned int video_flag;

	cnt = 0;
	video_flag = VIDEO_STATUS_END;

	for (cnt = 0; cnt < MAX_STAT_SEC; cnt++) {

		if (g_stat.stat[cnt].uin_len > UDP_PKT_MAX_THRESHOLD) {
			video_flag = VIDEO_STATUS_START;
			break;
		}
	}

	if (g_video_stat.u_stat == VIDEO_STATUS_START) {
		if (video_flag == VIDEO_STATUS_END) {

			g_video_stat.u_stat = VIDEO_STATUS_END;
			report_stat();
			if (g_video_stat.t_u_flag == STREAM_UDP) {

				mod_timer(&g_seg_rpt_timer,
					jiffies + REPORT_TIME);
				pr_info("VOD media modify timer0\n");
			}
		}
	} else {
		if (video_flag == VIDEO_STATUS_START) {

			report_stat();
			g_seg.seg_idx = 0;
		}
	}

	g_video_stat.u_stat = video_flag;
}

/*Modify tcp video playback status*/
static void modify_status(void)
{
	if (g_video_stat.t_stat == VIDEO_STATUS_END) {

		g_video_stat.t_stat = VIDEO_STATUS_PREPARED;
		g_seg.seg_idx = 0;
		report_stat();

		mod_timer(&g_seg_rpt_timer,
			jiffies + REPORT_TIME);
		pr_info("VOD media modify timer1\n");
	} else if (g_video_stat.t_stat == VIDEO_STATUS_PREPARED) {

		g_video_stat.t_stat = VIDEO_STATUS_START;
		g_seg.seg_idx++;
		report_stat();
	} else {

		g_seg.seg_idx++;
	}
}

/*TCP protocol video play status maintenance function*/
static int t_video_state(unsigned char *pstr,
	unsigned int len, unsigned short port, unsigned int ip)
{
	int cnt;

	cnt = 0;
	if (g_video_stat.t_stat != VIDEO_STATUS_END &&
		before(g_video_stat.t_time_stamp +
		SWITCH_END_TIMER, jiffies)) {

		g_video_stat.t_stat = VIDEO_STATUS_END;
		report_stat();
	}

	if (pstr == NULL || len < MIN_HTTP_LEN || len > MAX_HTTP_LEN)
		return -1;

	if (len > MAX_DATA_LEN)
		len = MAX_DATA_LEN;

	if (strncmp(pstr, STR_HTTP, STR_HTTP_LEN) != 0)
		return -1;

	if (pstr[HTTP_ACK_FROM_START] != '2')
		return -1;

	for (cnt = 0; cnt < len - CONTENT_TYPE_LEN; cnt++) {

		if (pstr[cnt] == ASCII_CR && pstr[cnt+1] == ASCII_LF) {

			cnt += 2;

			if (cnt >= len - CONTENT_TYPE_LEN) {

				break;
			} else if (pstr[cnt] == ASCII_CR && pstr[cnt+1] == ASCII_LF) {

				break;
			} else if (strncmp(&pstr[cnt], CONTENT_TYPE1, CONTENT_TYPE_LEN) == 0 ||
				strncmp(&pstr[cnt], CONTENT_TYPE2, CONTENT_TYPE_LEN) == 0) {

				modify_status();
				t_stream_info(port, ip);
				g_video_stat.t_time_stamp = jiffies;
				break;
			}
		} else if (pstr[cnt+1] > MAX_ASCII) {

			break;
		}
	}
	return 0;
}

/*Video segment information changes are reported*/
static int report_seg(unsigned long data)
{
	spin_lock_bh(&g_video_lock);

	if (!g_report_flag || !g_timer_flag) {
		spin_unlock_bh(&g_video_lock);
		return -1;
	}

	if (g_video_stat.t_stat != VIDEO_STATUS_END &&
		before(g_video_stat.t_time_stamp +
		SWITCH_END_TIMER, jiffies)) {

		g_video_stat.t_stat = VIDEO_STATUS_END;
	}

	if (g_video_stat.t_u_flag == STREAM_TCP) {

		g_stream.t_num = g_stream.t_num % PKT_VECTOR_LEN;
		g_report.segDuration = DURATION_TIME;
		g_report.videoIP = (g_stream.t_ip[g_stream.t_num] | IP_MASK);
		g_report.videoPort = g_stream.t_port[g_stream.t_num];
		g_report.videoStatus = g_video_stat.t_stat;
	} else {

		g_stream.u_num = g_stream.u_num % PKT_VECTOR_LEN;
		g_report.segDuration = DURATION_TIME;
		g_report.videoIP = (g_stream.u_ip[g_stream.u_num] | IP_MASK);
		g_report.videoPort = g_stream.u_port[g_stream.u_num];
		g_report.videoStatus = g_video_stat.u_stat;
	}

	g_report.aveCodeRate = g_bit_rate;
	g_report.segIndex = g_seg.seg_idx++;
	g_report.videoProtocol = VIDEO_PROTOCOL_HLS;
	g_report.videoRemainingPlayTime = g_seg.seg_remain;
	g_report.videoSegState = CHOICE_RESULT_VIDEO_SEG;

	nb_notify_event(NBMSG_VOD_EVT, &g_report, sizeof(g_report));
	pr_info("VOD media report segment:%d\n", g_report.aveCodeRate);

	if ((g_video_stat.t_stat != VIDEO_STATUS_END
		&& g_video_stat.t_u_flag == STREAM_TCP) ||
		(g_video_stat.u_stat != VIDEO_STATUS_END
		&& g_video_stat.t_u_flag == STREAM_UDP)) {

		mod_timer(&g_seg_rpt_timer, jiffies + REPORT_TIME);
		if (g_timer_flag > 0)
			g_timer_flag--;
		pr_info("VOD media modify timer2\n");
	}

	spin_unlock_bh(&g_video_lock);
	return 0;
}

/*Local out hook function*/
static unsigned int hook_out(const struct nf_hook_ops *ops,
					struct sk_buff *skb,
					const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	struct udphdr *udph = NULL;
	char *pTcpData = NULL;
	u32 totalLen, ipHdrLen, dataLen;

	if (skb == NULL)
		return NF_ACCEPT;

	if (g_hook_flag == HOOK_DISABLE)
		return NF_ACCEPT;

	iph = ip_hdr(skb);
	if (iph == NULL)
		return NF_ACCEPT;

	if (NULL == skb->dev || NULL == skb->dev->name)
		return NF_ACCEPT;

	if (strncmp(skb->dev->name, DS_NET, DS_NET_LEN))
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP) {

		tcph = tcp_hdr(skb);

		if (NULL == tcph || NULL == skb->data || 0 == tcph->doff)
			return NF_ACCEPT;

		pTcpData = (char *)((u32 *)tcph + tcph->doff);
		if (!virt_addr_valid(pTcpData))
			return NF_ACCEPT;

		totalLen = skb->len - skb->data_len;
		ipHdrLen = (pTcpData - (char *)iph);
		dataLen = totalLen - ipHdrLen;

		if (totalLen > MAX_HTTP_LEN || ipHdrLen > MAX_HTTP_LEN || dataLen > MAX_HTTP_LEN)
			return NF_ACCEPT;

		if (htons(tcph->dest) != HTTP_PORT)
			return NF_ACCEPT;

		if (skb->sk == NULL)
			return NF_ACCEPT;

		if (skb->sk->sk_state != TCP_ESTABLISHED)
			return NF_ACCEPT;

		/*When the lock is not locked, the lock is triggered*/
		if (!spin_trylock_bh(&g_video_lock))
			return NF_ACCEPT;

		idx_update();
		t_video_state(NULL, 0, 0, 0);

		spin_unlock_bh(&g_video_lock);
	} else if (iph->protocol == IPPROTO_UDP) {
		udph = udp_hdr(skb);

		if (udph == NULL)
			return NF_ACCEPT;

		/*When the lock is not locked, the lock is triggered*/
		if (!spin_trylock_bh(&g_video_lock))
			return NF_ACCEPT;

		if (idx_update() > 0)
			u_video_state();

		spin_unlock_bh(&g_video_lock);
	}

	return NF_ACCEPT;
}

/*Local in hook function*/
static unsigned int hook_in(const struct nf_hook_ops *ops,
			struct sk_buff *skb,
			const struct nf_hook_state *state)
{
	struct iphdr *iph = NULL;
	struct tcphdr *tcph = NULL;
	struct udphdr *udph = NULL;
	char *pTcpData = NULL;
	u32 totalLen, ipHdrLen, dataLen;

	if (skb == NULL)
		return NF_ACCEPT;

	if (g_hook_flag == HOOK_DISABLE)
		return NF_ACCEPT;

	iph = ip_hdr(skb);
	if (iph == NULL)
		return NF_ACCEPT;

	if (NULL == skb->dev || NULL == skb->dev->name)
		return NF_ACCEPT;

	if (strncmp(skb->dev->name, DS_NET, DS_NET_LEN))
		return NF_ACCEPT;

	if (iph->protocol == IPPROTO_TCP) {

		tcph = tcp_hdr(skb);

		if (NULL == tcph || NULL == skb->data || 0 == tcph->doff)
			return NF_ACCEPT;

		pTcpData = (char *)((u32 *)tcph + tcph->doff);
		if (!virt_addr_valid(pTcpData))
			return NF_ACCEPT;

		totalLen = skb->len - skb->data_len;
		ipHdrLen = (pTcpData - (char *)iph);
		dataLen = totalLen - ipHdrLen;

		if (totalLen > MAX_HTTP_LEN || ipHdrLen > MAX_HTTP_LEN || dataLen > MAX_HTTP_LEN)
			return NF_ACCEPT;

		if (htons(tcph->source) != HTTP_PORT)
			return NF_ACCEPT;

		if (skb->sk == NULL)
			return NF_ACCEPT;

		if (skb->sk->sk_state != TCP_ESTABLISHED)
			return NF_ACCEPT;

		if (!spin_trylock_bh(&g_video_lock))
			return NF_ACCEPT;

		idx_update();
		tcp_in_cnt(skb->sk, dataLen, ntohl(tcph->seq));
		t_video_state(pTcpData, dataLen, tcph->dest, iph->saddr);

		spin_unlock_bh(&g_video_lock);
	} else if (iph->protocol == IPPROTO_UDP) {
		udph = udp_hdr(skb);

		if (udph == NULL)
			return NF_ACCEPT;

		totalLen = skb->len - skb->data_len;
		if (totalLen > MAX_HTTP_LEN)
			return NF_ACCEPT;

		if (!spin_trylock_bh(&g_video_lock))
			return NF_ACCEPT;

		if (idx_update() > 0)
			u_video_state();

		u_stream_info(udph->dest, iph->saddr, totalLen);

		spin_unlock_bh(&g_video_lock);
	}

	return NF_ACCEPT;
}

static struct nf_hook_ops net_hooks[] = {
	{
		.hook		= hook_in,
		.pf			= PF_INET,
		.hooknum	= NF_INET_LOCAL_IN,
		.priority	= NF_IP_PRI_FILTER - 1,
	},
	{
		.hook		= hook_out,
		.pf			= PF_INET,
		.hooknum	= NF_INET_POST_ROUTING,
		.priority	= NF_IP_PRI_FILTER - 1,
	}
};

/*Video parameters are reported, Is one reported when video parameters,
  *Is zero does not report video parameters
  */
int set_vod_enable(uint8_t nf_hook_enable, uint8_t nl_event_enable)
{
	spin_lock_bh(&g_video_lock);
	g_report_flag = nl_event_enable;
	g_hook_flag = nf_hook_enable;
	spin_unlock_bh(&g_video_lock);

	return 0;
}

/* Initialization function*/
int video_acceleration_init(void)
{
	int ret = 0;
	/*initial memory*/
	memset(&g_stat, 0, sizeof(g_stat));
	memset(&g_filter, 0, sizeof(g_filter));
	memset(&g_video_stat, 0, sizeof(g_video_stat));
	memset(&g_stream, 0, sizeof(g_stream));
	memset(&g_seg, 0, sizeof(g_seg));
	memset(&g_report, 0, sizeof(g_report));
	g_bit_rate = 0;
	g_report_flag = MODULE_DISABLE;
	g_hook_flag = HOOK_DISABLE;
	g_video_stat.t_stat = VIDEO_STATUS_END;
	g_video_stat.u_stat = VIDEO_STATUS_END;

	/*Timer initialization*/
	init_timer(&g_seg_rpt_timer);
	g_seg_rpt_timer.data = 0;
	g_seg_rpt_timer.function = report_seg;
	g_seg_rpt_timer.expires = jiffies + REPORT_TIME;
	g_timer_flag = 0;

	spin_lock_init(&g_video_lock);
	/*Registration hook function*/
	ret = nf_register_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	if (ret) {
		pr_err("VOD hook register fail:%d\n", ret);
		return ret;
	}
	pr_info("media acceleration initial success\n");
	return 0;
}

void video_acceleration_exit(void)
{
	del_timer(&g_seg_rpt_timer);
	nf_unregister_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	g_hook_flag = HOOK_DISABLE;
	pr_info("media acceleration exit success\n");
}

#undef DEBUG
