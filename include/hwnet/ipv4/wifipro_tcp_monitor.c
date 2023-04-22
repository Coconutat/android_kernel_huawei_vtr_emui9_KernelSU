

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/time.h>
#include <linux/compiler.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <uapi/linux/netlink.h>
#include <linux/kthread.h>
#include <linux/string.h>
#include <net/tcp.h>
#include "wifipro_tcp_monitor.h"

#ifndef CONFIG_HW_WIFIPRO
#undef CONFIG_HW_WIFIPRO_PROC
#endif

#ifdef CONFIG_HW_WIFIPRO_PROC
#include <net/net_namespace.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/rtc.h>
#endif

MODULE_LICENSE("GPL");

DEFINE_MUTEX(wifipro_nl_receive_sem);
DEFINE_MUTEX(wifipro_nl_send_sem);
DEFINE_MUTEX(wifipro_google_sock_sem);
DEFINE_MUTEX(wifipro_congestion_sem);

#define LINK_UNKNOWN				0
#define LINK_POOR					1
#define LINK_MAYBE_POOR				2
#define LINK_MAYBE_GOOD				3
#define LINK_GOOD					4

enum wifipro_KnlMsgType {
	NETLINK_WIFIPRO_START_MONITOR = 0,
	NETLINK_WIFIPRO_GET_MSG,
	NETLINK_WIFIPRO_STOP_MONITOR,
	NETLINK_WIFIPRO_PAUSE_MONITOR,
	NETLINK_WIFIPRO_CONTINUE_MONITOR,
	NETLINK_WIFIPRO_NOTIFY_MCC,
	NETLINK_WIFIPRO_RESET_RTT,
	NETLINK_WIFIPRO_GET_RTT,
    NETLINK_HIDATA_GET_TCP_INFO,
};

enum wifipro_msg_from {
	WIFIPRO_APP_QUERY = 0,
	WIFIPRO_KNL_NOTIFY,
	WIFIPRO_NOTIFY_WLAN_BQE_RTT,
	WIFIPRO_NOTIFY_MOBILE_BQE_RTT,
	WIFIPRO_NOTIFY_WLAN_SAMPLE_RTT
};

struct wifipro_nl_packet_msg {
	unsigned int msg_from;	/*kernel notify or app query*/
	unsigned int rtt;	    /*average rtt within last 3 seconds*/
	unsigned int rtt_pkts;	/*packet counts of rtt calculation*/
	unsigned int rtt_when;	/*when recorded rtt*/
	unsigned int congestion;	/*congestion stat*/
	unsigned int cong_when;	 /*when recorded congestion*/
	unsigned int tcp_quality;	/*tcp quality*/
	unsigned int tcp_tx_pkts;
	unsigned int tcp_rx_pkts;
	unsigned int tcp_retrans_pkts;
};

#ifdef CONFIG_HW_WIFIPRO_PROC
static const struct snmp_mib wifipro_snmp_tcp_list[] = {
	SNMP_MIB_ITEM("Unknown InSegs", WIFIPRO_TCP_MIB_INSEGS),
	SNMP_MIB_ITEM("Unknown OutSegs", WIFIPRO_TCP_MIB_OUTSEGS),
	SNMP_MIB_ITEM("Unknown RetransSegs", WIFIPRO_TCP_MIB_RETRANSSEGS),
	SNMP_MIB_ITEM("Wlan InSegs", WIFIPRO_TCP_MIB_WLAN_INSEGS),
	SNMP_MIB_ITEM("Wlan OutSegs", WIFIPRO_TCP_MIB_WLAN_OUTSEGS),
	SNMP_MIB_ITEM("Wlan RetransSegs", WIFIPRO_TCP_MIB_WLAN_RETRANSSEGS),
	SNMP_MIB_ITEM("Mobile InSegs", WIFIPRO_TCP_MIB_MOBILE_INSEGS),
	SNMP_MIB_ITEM("Mobile OutSegs", WIFIPRO_TCP_MIB_MOBILE_OUTSEGS),
	SNMP_MIB_ITEM("Mobile RetransSegs", WIFIPRO_TCP_MIB_MOBILE_RETRANSSEGS),
	SNMP_MIB_ITEM("InErrs", WIFIPRO_TCP_MIB_INERRS),
	SNMP_MIB_ITEM("OutRsts", WIFIPRO_TCP_MIB_OUTRSTS),
	SNMP_MIB_ITEM("ACKs", WIFIPRO_TCP_MIB_ACKS),
	SNMP_MIB_SENTINEL
};
#endif

static void wifipro_rtt_monitor_deinit(void);

bool is_wifipro_on;
bool is_mcc_china;
unsigned int wifipro_log_level = WIFIPRO_INFO;
static bool is_wifipro_running;
static unsigned int g_user_space_pid;
static unsigned long wlan_last_expire;
static unsigned int wlan_rtt_average;
static unsigned int wlan_rtt_calc_pkg;
static unsigned long wlan_when_recorded_rtt;
static unsigned long mobile_last_expire;
static unsigned int mobile_rtt_average;
static unsigned int mobile_rtt_calc_pkg;
static unsigned long mobile_when_recorded_rtt;
static int unknown_dev_count;
static int unknown_segs_count;
static struct sock *g_wifipro_nlfd;
wifipro_rtt_stat_t wlan_bqe_rtt_stat;
wifipro_rtt_stat_t mobile_bqe_rtt_stat;
wifipro_rtt_stat_t wlan_sample_rtt_stat;
static wifipro_rtt_second_stat_t *wlan_rtt_second_stat_head;
static wifipro_rtt_second_stat_t *mobile_rtt_second_stat_head;
static wifipro_cong_sock_t *wifipro_congestion_stat;

static void updateHidataTcpStatByUID(kuid_t uid, int index, int count);
static int hidata_tcp_info_report(int pid, kuid_t uid);
static  kuid_t get_socket_uid(struct sock *sk)
{
	kuid_t uid = INVALID_UID;

	if (sk == NULL)
		return INVALID_UID;
	uid = sock_i_uid(sk);
	return uid;
}

static int wifi_getWlanTcpStatIndex(kuid_t uid)
{
	int index =  INVALID_INDEX;
	int i = 0;

	if (!uid_valid(uid))
		return INVALID_INDEX;

	if (uid_eq(uid, KUIDT_INIT(0)) || uid_eq(uid, KUIDT_INIT(1000)))
		return INVALID_INDEX;

		for (i = 0; i < s_wlanTcpStat_index && i < MAX_UID_CNT ; i++) {
		if (uid_eq(uid, s_uidWlanTcpStat[i].uid)) {
			index = i;
			goto out;
		}
	}

	if (s_wlanTcpStat_index < MAX_UID_CNT) {
		index = s_wlanTcpStat_index;
		s_wlanTcpStat_index++;
		s_uidWlanTcpStat[index].uid = uid;
	}

out:
	return index;
}

static int wifi_getRmnetTcpStatIndex(kuid_t uid)
{
	int index =  INVALID_INDEX;
	int i = 0;

	if (!uid_valid(uid))
		return INVALID_INDEX;

	if (uid_eq(uid, KUIDT_INIT(0)) || uid_eq(uid, KUIDT_INIT(1000)))
		return INVALID_INDEX;

	for (i = 0; i < s_rmnetTcpStat_index && i < MAX_UID_CNT ; i++) {
		if (uid_eq(uid, s_uidRmnetTcpStat[i].uid)) {
			index = i;
			goto out;
		}
	}

	if (s_rmnetTcpStat_index < MAX_UID_CNT) {
		index = s_rmnetTcpStat_index;
		s_rmnetTcpStat_index++;
		s_uidRmnetTcpStat[index].uid = uid;
	}

out:
	return index;
}



static void updateWlanTcpStatByUID(kuid_t uid, int index, int count)
{
	int result = 0;

	result = wifi_getWlanTcpStatIndex(uid);

	if (result == INVALID_INDEX)
		return;

	if (!uid_eq(uid, s_uidWlanTcpStat[result].uid))
		return;

	s_uidWlanTcpStat[result].counter[index] += count;
}

static void updateRmnetTcpStatByUID(kuid_t uid, int index, int count)
{
	int result = 0;

	result = wifi_getRmnetTcpStatIndex(uid);

	if (result == INVALID_INDEX)
		return;

	if (!uid_eq(uid, s_uidRmnetTcpStat[result].uid))
		return;

	s_uidRmnetTcpStat[result].counter[index] += count;
}



char *wifipro_ntoa(int addr)
{
	int len = 0;
	static char buf[18];

	memset(buf, 0, sizeof(buf));
	len += sprintf(buf+len, "%d.", (addr>>24) & 0xff);
	len += sprintf(buf+len, "%d.", (addr>>16) & 0xff);
	len += sprintf(buf+len, "%d.", (addr>>8) & 0xff);
	len += sprintf(buf+len, "%d", addr & 0xff);

	return buf;
}

void wifipro_update_tcp_statistics(int mib_type, const struct sk_buff *skb, struct sock *from_sk)
{
	struct inet_sock *inet = NULL;
	unsigned int dest_addr = 0;
	unsigned int dest_port = 0;
	unsigned int src_addr = 0;
	unsigned int wifipro_dev_max_len = 0;
	struct sock *sk = NULL;

	if (from_sk != NULL) {
		sk = from_sk;
	} else {
		sk = skb->sk;
	}

	if (!is_wifipro_on || sk == NULL) {
		return;
	}

	inet = inet_sk(sk);
	if (inet == NULL) {
		return;
	}
	dest_addr = htonl(inet->inet_daddr);
	dest_port = htons(inet->inet_dport);
	src_addr = htonl(inet->inet_saddr);

	if (!wifipro_is_not_local_or_lan_sock(dest_addr)) {
		return;
	}

	if ((1 << sk->sk_state) & TCPF_SYN_SENT) {
		if (skb->dev) {
			wifipro_dev_max_len = strnlen(skb->dev->name, IFNAMSIZ-1);
			strncpy(sk->wifipro_dev_name, skb->dev->name, wifipro_dev_max_len);
			sk->wifipro_dev_name[wifipro_dev_max_len] = '\0';
			WIFIPRO_DEBUG("wifipro_dev_name is %s", skb->dev->name);
		}
	}

	if (mib_type == WIFIPRO_TCP_MIB_OUTSEGS) {
		if (!strncmp(sk->wifipro_dev_name, "wlan", 4)) {
			WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_WLAN_OUTSEGS);
		} else if (!strncmp(sk->wifipro_dev_name, "rmnet", 5)) {
			WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_MOBILE_OUTSEGS);
		} else {
			/*for some reason, some socket state, ex TIME_WAIT, we can't get sk->wifipro_dev_name*/
			if (skb->dev) {
				if (!strncmp(skb->dev->name, "wlan",  4)) {
					WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_WLAN_OUTSEGS);
				} else if (!strncmp(skb->dev->name, "rmnet", 5)) {
					WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_MOBILE_OUTSEGS);
				} else {
					unknown_dev_count++;
					WIFIPRO_DEBUG("unknown out device, state is %d", sk->sk_state);
					return;
				}
			} else {
				unknown_dev_count++;
				WIFIPRO_DEBUG("unknown out device, state is %d", sk->sk_state);
				return;
			}
		}
		if ((skb->dev) && wifi_is_validate_state(sk)) {
			if ((!strncmp(sk->wifipro_dev_name, "wlan",  4)) || (!strncmp(skb->dev->name, "wlan",  4))) {
				updateWlanTcpStatByUID(get_socket_uid(sk), INDEX_WEBSENDSEGS, 1);
                updateHidataTcpStatByUID(get_socket_uid(sk), INDEX_WEBSENDSEGS, 1);
			} else if ((!strncmp(sk->wifipro_dev_name, "rmnet", 5)) || (!strncmp(skb->dev->name, "rmnet", 5))) {
				updateRmnetTcpStatByUID(get_socket_uid(sk), INDEX_WEBSENDSEGS, 1);
			} else {
				WIFIPRO_DEBUG("unknown out device\n");
			}
		}

		WIFIPRO_VERBOSE("Outsegs to increase");
	} else if (mib_type == WIFIPRO_TCP_MIB_INSEGS) {
		if (!strncmp(sk->wifipro_dev_name, "wlan",  4)) {
			WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_WLAN_INSEGS);
		} else if (!strncmp(sk->wifipro_dev_name, "rmnet", 5)) {
			WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_MOBILE_INSEGS);
		} else {
			/*for some reason, some socket state, ex TIME_WAIT, we can't get sk->wifipro_dev_name*/
			/*skb->dev = NULL in tcp_v4_rcv func. so this code maybe can not run*/
			if (skb->dev) {
				if (!strncmp(skb->dev->name, "wlan",  4)) {
					WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_WLAN_INSEGS);
				} else if (!strncmp(skb->dev->name, "rmnet", 5)) {
					WIFIPRO_TCP_INC_STATS_BH(&init_net, WIFIPRO_TCP_MIB_MOBILE_INSEGS);
				} else {
					unknown_dev_count++;
					WIFIPRO_DEBUG("unknown in device, state is %d", sk->sk_state);
					return;
				}
			} else {
				unknown_dev_count++;
				WIFIPRO_DEBUG("unknown in device, state is %d", sk->sk_state);
				return;
			}
		}
		if (wifi_is_validate_state(sk)) {
			if (!strncmp(sk->wifipro_dev_name, "wlan",  4)) {
				updateWlanTcpStatByUID(get_socket_uid(sk), INDEX_WEBRECVSEGS, 1);
                updateHidataTcpStatByUID(get_socket_uid(sk), INDEX_WEBRECVSEGS, 1);
			} else if (!strncmp(sk->wifipro_dev_name, "rmnet", 5)) {
				updateRmnetTcpStatByUID(get_socket_uid(sk), INDEX_WEBRECVSEGS, 1);
			} else {
				WIFIPRO_DEBUG("unknown out device\n");
			}
		}

		WIFIPRO_VERBOSE("Insegs from increase");
	} else {
		unknown_segs_count++;
		WIFIPRO_DEBUG("mib_type is %d", mib_type);
	}

	return;
}

static int wifipro_get_proc_name(struct task_struct *task, char *buffer)
{
	int res = 0;
	unsigned int len;
	struct mm_struct *mm = get_task_mm(task);

	if (!mm)
		goto out;
	if (!mm->arg_end)
		goto out_mm;

	len = mm->arg_end - mm->arg_start;

	if (len > PAGE_SIZE)
		len = PAGE_SIZE;

	res = access_process_vm(task, mm->arg_start, buffer, len, 0);

	/* If the nul at the end of args has been overwritten, then */
	/* assume application is using setproctitle(3). */
	if (res > 0 && buffer[res-1] != '\0' && len < PAGE_SIZE) {
		len = strnlen(buffer, res);
		if (len < res) {
			res = len;
		} else {
			len = mm->env_end - mm->env_start;
			if (len > PAGE_SIZE - res)
				len = PAGE_SIZE - res;
			res += access_process_vm(task, mm->env_start, buffer+res, len, 0);
			res = strnlen(buffer, res);
		}
	}
out_mm:
	mmput(mm);
out:
	return res;
}

bool wifipro_is_google_sock(struct task_struct *task, unsigned int dest_addr)
{
	static char proc_name[PAGE_SIZE];
	int ret = 0;

	if (task) {
		ret = wifipro_get_proc_name(task, proc_name);
		if (ret > 0) {
			if (strstr(proc_name, "oogle")) { /*google or Google*/
				WIFIPRO_DEBUG("find a process name %s match google. The pid = %d", proc_name, task->pid);
				return true;
			} else if (strstr(proc_name, "system_server")) { /*system_server will setup google socket*/
				WIFIPRO_DEBUG("find a process name %s match system_server. ", proc_name);
				return true;
			} else {
				WIFIPRO_DEBUG("the process name %s doesn't match google", proc_name);
				return false;
			}
		}
	} else {
		WIFIPRO_WARNING("Task is NULL");
	}

	return false;
}

static void wifipro_rtt_free(wifipro_rtt_second_stat_t *head, int node_count)
{
	wifipro_rtt_second_stat_t *curr_bl;
	wifipro_rtt_second_stat_t *prev_bl;

	if (head == NULL)
		return;

	curr_bl = head;
	while (node_count > 0) {
		prev_bl = curr_bl;
		curr_bl = curr_bl->next;

		kfree(prev_bl);
		node_count--;
	}
}

static void wifipro_set_cong_stat(unsigned int dest_addr, unsigned int dest_port, wifipro_cong_sock_t *src, unsigned int offset)
{
	wifipro_cong_sock_t *dst = NULL;

	if (!wifipro_congestion_stat) {
		return;
	}
	dst = src + offset;
	dst->amount++;
	dst->dst_addr = dest_addr;
	dst->dst_port = dest_port;
	dst->when = jiffies;
}

int wifipro_handle_retrans(struct sock *sk, struct inet_connection_sock *icsk)
{
	struct inet_sock *inet = inet_sk(sk);
	struct net *temp_net = NULL;
	unsigned int dest_addr = 0;
	unsigned int dest_port = 0;
	unsigned int src_addr = 0;
	unsigned int src_port = 0;

	inet = inet_sk(sk);
	if (inet == NULL) {
		WIFIPRO_ERROR("GET NULL POINTER!");
		return -1;
	}
	dest_addr = htonl(inet->inet_daddr);
	dest_port = htons(inet->inet_dport);
	src_addr = htons(inet->inet_saddr);
	src_port = htons(inet->inet_sport);

	if (!wifipro_is_not_local_or_lan_sock(dest_addr) || sk->wifipro_is_google_sock) {
		return 0;
	}

	if (!((1 << sk->sk_state) & (TCPF_SYN_SENT | TCPF_ESTABLISHED))) {
		return 0;
	}

	/*if it's not local, LAN or google socket, record it.*/
	if (!strncmp(sk->wifipro_dev_name, "wlan",  4)) {
		WIFIPRO_TCP_INC_STATS_BH(sock_net(sk), WIFIPRO_TCP_MIB_WLAN_RETRANSSEGS);
		updateWlanTcpStatByUID(get_socket_uid(sk), INDEX_WEBRESENDSEGS, 1);
        updateHidataTcpStatByUID(get_socket_uid(sk), INDEX_WEBRESENDSEGS, 1);
	} else if (!strncmp(sk->wifipro_dev_name, "rmnet", 5)) {
		WIFIPRO_TCP_INC_STATS_BH(sock_net(sk), WIFIPRO_TCP_MIB_MOBILE_RETRANSSEGS);
		updateRmnetTcpStatByUID(get_socket_uid(sk), INDEX_WEBRESENDSEGS, 1);
		return 0;
	} else {
		WIFIPRO_TCP_INC_STATS_BH(sock_net(sk), WIFIPRO_TCP_MIB_RETRANSSEGS);
		return -1;
	}

	WIFIPRO_DEBUG("%d retrans=%d  sk_state=%d  rto=%d",
		dest_port, icsk->icsk_retransmits, sk->sk_state, icsk->icsk_rto);
	return 0;
}

static int wifipro_tcp_monitor_send_msg(int pid, unsigned int msg_from, unsigned int quality)
{
	int ret;
	int size;
	int i;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct wifipro_nl_packet_msg *packet = NULL;

	if (!pid || !g_wifipro_nlfd) {
		WIFIPRO_ERROR("cannot notify event, pid = %d", pid);
		return -1;
	}

	mutex_lock(&wifipro_nl_send_sem);
	size = sizeof(struct wifipro_nl_packet_msg);
	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb) {
		WIFIPRO_ERROR("alloc skb fail");
		ret = -1;
		goto end;
	}

	nlh = nlmsg_put(skb, 0, 0, NETLINK_WIFIPRO_GET_MSG, size, 0);
	if (!nlh) {
		kfree_skb(skb);
		skb = NULL;
		ret = -1;
		goto end;
	}

	packet = nlmsg_data(nlh);
	memset(packet, 0, sizeof(struct wifipro_nl_packet_msg));

	packet->msg_from = msg_from;
	packet->rtt = wlan_rtt_average;
	packet->rtt_pkts = wlan_rtt_calc_pkg;
	packet->rtt_when = (jiffies - wlan_when_recorded_rtt)/HZ;
	mutex_lock(&wifipro_congestion_sem);
	if (wifipro_congestion_stat != NULL) {
		wifipro_cong_sock_t *dst = NULL;

		for (i = TCP_CA_Disorder; i <= TCP_CA_Loss; i++) {
			dst = wifipro_congestion_stat + i;
			if (time_after(dst->when + 10*HZ, jiffies)) {
				packet->congestion |= 1 << i;
				if (dst->when > packet->cong_when) {
					packet->cong_when = dst->when;
				}
			}
		}
	} else {
		packet->congestion = 0;
		packet->cong_when = 0;
	}
	mutex_unlock(&wifipro_congestion_sem);
	packet->tcp_rx_pkts = snmp_fold_field((void __percpu **)init_net.mib.wifipro_tcp_statistics, WIFIPRO_TCP_MIB_WLAN_INSEGS);
	packet->tcp_tx_pkts = snmp_fold_field((void __percpu **)init_net.mib.wifipro_tcp_statistics, WIFIPRO_TCP_MIB_WLAN_OUTSEGS);
	packet->tcp_retrans_pkts = snmp_fold_field((void __percpu **)init_net.mib.wifipro_tcp_statistics, WIFIPRO_TCP_MIB_WLAN_RETRANSSEGS);
	packet->tcp_quality = quality;

	ret = netlink_unicast(g_wifipro_nlfd, skb, pid, MSG_DONTWAIT); /*skb will be freed in netlink_unicast*/

end:
	mutex_unlock(&wifipro_nl_send_sem);
	/*if (wifipro_log_level >= WIFIPRO_DEBUG && ret != -1) {
		char printk_buf[WIFIPRO_PRINT_BUF_SIZE];
		int buf_len = 0;

		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf), "\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ Netlink struct @@@@@@@@@@@@@@@@@@@@@@@@@\n");
		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "%s:send a msg to wifipro pid=%d:\n", __func__, pid);
		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "rtt=%d  packet=%d  when=%ds  ", packet->rtt, packet->rtt_pkts, packet->rtt_when);
		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "congestion=0x%x  quality=%d", packet->congestion, packet->tcp_quality);
		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
		pr_err("%s", printk_buf);
	}*/
	return ret;
}


static int wifipro_notify_rtt_msg(int pid, unsigned int msg_from)
{
	int ret;
	int size;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct wifipro_nl_packet_msg *packet = NULL;

	if (!pid || !g_wifipro_nlfd) {
		WIFIPRO_ERROR("cannot notify event, pid = %d", pid);
		return -1;
	}

	mutex_lock(&wifipro_nl_send_sem);
	size = sizeof(struct wifipro_nl_packet_msg);
	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb) {
		WIFIPRO_ERROR("alloc skb fail");
		ret = -1;
		goto end;
	}

	nlh = nlmsg_put(skb, 0, 0, NETLINK_WIFIPRO_GET_RTT, size, 0);
	if (!nlh) {
		kfree_skb(skb);
		skb = NULL;
		ret = -1;
		goto end;
	}

	packet = nlmsg_data(nlh);
	memset(packet, 0, sizeof(struct wifipro_nl_packet_msg));

	packet->msg_from = msg_from;
	switch (msg_from) {
	case WIFIPRO_NOTIFY_WLAN_BQE_RTT:
			if (wlan_bqe_rtt_stat.packets != 0) {
				packet->rtt = (wlan_bqe_rtt_stat.rtt / wlan_bqe_rtt_stat.packets) * WIFIPRO_TICK_TO_MS;
			}
			packet->rtt_pkts = wlan_bqe_rtt_stat.packets;
			packet->rtt_when = (jiffies - wlan_bqe_rtt_stat.last_update)/HZ;
			break;

	case WIFIPRO_NOTIFY_MOBILE_BQE_RTT:
			if (mobile_bqe_rtt_stat.packets != 0) {
				packet->rtt = (mobile_bqe_rtt_stat.rtt / mobile_bqe_rtt_stat.packets) * WIFIPRO_TICK_TO_MS;
			}
			packet->rtt_pkts = mobile_bqe_rtt_stat.packets;
			packet->rtt_when = (jiffies - mobile_bqe_rtt_stat.last_update)/HZ;
			break;

	case WIFIPRO_NOTIFY_WLAN_SAMPLE_RTT:
			if (wlan_sample_rtt_stat.packets != 0) {
				packet->rtt = (wlan_sample_rtt_stat.rtt / wlan_sample_rtt_stat.packets) * WIFIPRO_TICK_TO_MS;
			}
			packet->rtt_pkts = wlan_sample_rtt_stat.packets;
			packet->rtt_when = (jiffies - wlan_sample_rtt_stat.last_update)/HZ;
			wlan_sample_rtt_stat.rtt = 0;
			wlan_sample_rtt_stat.packets = 0;
			break;

	default:
			WIFIPRO_WARNING("unvalid msg type, msg_from = %d", msg_from);
			break;
	}

	ret = netlink_unicast(g_wifipro_nlfd, skb, pid, MSG_DONTWAIT); /*skb will be freed in netlink_unicast*/

end:
	mutex_unlock(&wifipro_nl_send_sem);
	/*if (wifipro_log_level >= WIFIPRO_DEBUG && ret != -1) {
		char printk_buf[WIFIPRO_PRINT_BUF_SIZE];
		int buf_len = 0;

		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf), "\n\n@@@@@@@@@@@@@@@@@@@@@@@@@ Netlink struct @@@@@@@@@@@@@@@@@@@@@@@@@\n");
		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "%s:send a msg to wifipro pid=%d:\n", __func__, pid);
		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "rtt=%d  packet=%d  when=%ds  ", packet->rtt, packet->rtt_pkts, packet->rtt_when);
		buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "\n@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@\n\n");
		pr_err("%s", printk_buf);
	}*/
	return ret;
}


static void wifipro_tcp_monitor_nl_receive(struct sk_buff *__skb)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
    kuid_t uid;
	if (NULL == __skb) {
		pr_err("Invalid parameter: zero pointer reference(__skb)\n");
		return;
	}
	skb = skb_get(__skb);
	if (NULL == skb) {
		pr_err("wifipro_tcp_monitor_nl_receive skb = NULL\n");
		return;
	}
	mutex_lock(&wifipro_nl_receive_sem);
	if (skb->len >= NLMSG_HDRLEN) {
		nlh = nlmsg_hdr(skb);
		if (NULL == nlh) {
			pr_err("wifi_tcp_nl_receive  nlh = NULL\n");
			consume_skb(skb);
			mutex_unlock(&wifipro_nl_receive_sem);
			return;
		}
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr)) &&
			(skb->len >= nlh->nlmsg_len)) {
			WIFIPRO_DEBUG("netlink receive a packet, nlmsg_type=%d", nlh->nlmsg_type);
			switch (nlh->nlmsg_type) {
			case NETLINK_WIFIPRO_START_MONITOR:
				is_wifipro_on = true;
				g_user_space_pid = nlh->nlmsg_pid;
				if (nlh->nlmsg_flags == BETA_USER) {
					wifipro_log_level = WIFIPRO_DEBUG;
				}
				break;

			case NETLINK_WIFIPRO_STOP_MONITOR:
				is_wifipro_on = false;
				g_user_space_pid = 0;
				break;

			case NETLINK_WIFIPRO_GET_MSG:
				wifipro_tcp_monitor_send_msg(nlh->nlmsg_pid, WIFIPRO_APP_QUERY, LINK_UNKNOWN);
				break;

			case NETLINK_WIFIPRO_PAUSE_MONITOR:
				is_wifipro_running = false;
				break;

			case NETLINK_WIFIPRO_CONTINUE_MONITOR:
				is_wifipro_running = true;
				break;

			case NETLINK_WIFIPRO_NOTIFY_MCC:
				if (nlh->nlmsg_flags == MCC_CHINA) {
					WIFIPRO_DEBUG("MCC = 460");
					is_mcc_china = true;
				} else {
					WIFIPRO_DEBUG("MCC != 460");
					is_mcc_china = false;
				}
				break;

			case NETLINK_WIFIPRO_RESET_RTT:
				if (nlh->nlmsg_flags == WIFIPRO_WLAN_BQE_RTT) {
					wlan_bqe_rtt_stat.rtt = 0;
					wlan_bqe_rtt_stat.packets = 0;
				} else if (nlh->nlmsg_flags == WIFIPRO_MOBILE_BQE_RTT) {
					mobile_bqe_rtt_stat.rtt = 0;
					mobile_bqe_rtt_stat.packets = 0;
				} else if (nlh->nlmsg_flags == WIFIPRO_WLAN_SAMPLE_RTT) {
					wlan_sample_rtt_stat.rtt = 0;
					wlan_sample_rtt_stat.packets = 0;
				}
				break;

			case NETLINK_WIFIPRO_GET_RTT:
				if (nlh->nlmsg_flags == WIFIPRO_WLAN_BQE_RTT) {
					wifipro_notify_rtt_msg(nlh->nlmsg_pid, WIFIPRO_NOTIFY_WLAN_BQE_RTT);
				} else if (nlh->nlmsg_flags == WIFIPRO_MOBILE_BQE_RTT) {
					wifipro_notify_rtt_msg(nlh->nlmsg_pid, WIFIPRO_NOTIFY_MOBILE_BQE_RTT);
				} else  if (nlh->nlmsg_flags == WIFIPRO_WLAN_SAMPLE_RTT) {
					wifipro_notify_rtt_msg(nlh->nlmsg_pid, WIFIPRO_NOTIFY_WLAN_SAMPLE_RTT);
				}
				break;
            case NETLINK_HIDATA_GET_TCP_INFO:
                uid = KUIDT_INIT(nlh->nlmsg_flags);
                hidata_tcp_info_report(nlh->nlmsg_pid,uid);
                break;
			default:
				WIFIPRO_WARNING("unvalid msg type, nlmsg_type = %d", nlh->nlmsg_type);
				break;
			}
		}
	}
	consume_skb(skb);
	mutex_unlock(&wifipro_nl_receive_sem);
}

void wifipro_handle_congestion(struct sock *sk, u8 ca_state)
{
	struct inet_sock *inet = NULL;
	wifipro_cong_sock_t *dst = NULL;
	unsigned int dest_addr = 0;
	unsigned int dest_port = 0;

	inet = inet_sk(sk);
	if (inet == NULL) {
		WIFIPRO_ERROR("GET NULL POINTER!");
		return;
	}
	dest_addr = htonl(inet->inet_daddr);
	dest_port = htons(inet->inet_dport);

	if (!wifipro_is_not_local_or_lan_sock(dest_addr) || sk->wifipro_is_google_sock) {
		return;
	}

	switch (ca_state) {
	case TCP_CA_Open:
		wifipro_set_cong_stat(dest_addr, dest_port, wifipro_congestion_stat, TCP_CA_Open);
		break;

	case TCP_CA_Disorder:
		dst = wifipro_congestion_stat + TCP_CA_Disorder;
		/*if (wifipro_log_level >= WIFIPRO_DEBUG) {
			char printk_buf[WIFIPRO_PRINT_BUF_SIZE];
			int buf_len = 0;

			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf), "\n************************** TCP_CA_Disorder **************************\n");
			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "%s: %d  sk_state=%d  amount=%d, %lums ago",
				__func__, dest_port, sk->sk_state, dst->amount+1, (jiffies - dst->when)*WIFIPRO_TICK_TO_MS);
			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "\n*********************************************************************\n");
			pr_err("%s", printk_buf);
		}*/
		wifipro_set_cong_stat(dest_addr, dest_port, wifipro_congestion_stat, TCP_CA_Disorder);
		break;

	case TCP_CA_CWR:
		dst = wifipro_congestion_stat + TCP_CA_CWR;
		/*if (wifipro_log_level >= WIFIPRO_DEBUG) {
			char printk_buf[WIFIPRO_PRINT_BUF_SIZE];
			int buf_len = 0;

			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf), "\n************************** TCP_CA_CWR **************************\n");
			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "%s: %d  sk_state=%d  amount=%d, %lums ago",
				__func__, dest_port, sk->sk_state, dst->amount+1, (jiffies - dst->when)*WIFIPRO_TICK_TO_MS);
			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "\n*********************************************************************\n");
			pr_err("%s", printk_buf);
		}*/
		wifipro_set_cong_stat(dest_addr, dest_port, wifipro_congestion_stat, TCP_CA_CWR);
		break;

	case TCP_CA_Recovery:
		dst = wifipro_congestion_stat + TCP_CA_Recovery;
		/*if (wifipro_log_level >= WIFIPRO_DEBUG) {
			char printk_buf[WIFIPRO_PRINT_BUF_SIZE];
			int buf_len = 0;

			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf), "\n************************** TCP_CA_Recovery **************************\n");
			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "%s: %d  sk_state=%d  amount=%d, %lums ago",
				__func__, dest_port, sk->sk_state, dst->amount+1, (jiffies - dst->when)*WIFIPRO_TICK_TO_MS);
			buf_len += snprintf(printk_buf + buf_len,  sizeof(printk_buf) - buf_len,  "\n*********************************************************************\n");
			pr_err("%s", printk_buf);
		}*/
		wifipro_set_cong_stat(dest_addr, dest_port, wifipro_congestion_stat, TCP_CA_Recovery);
		break;

	case TCP_CA_Loss:
		wifipro_set_cong_stat(dest_addr, dest_port, wifipro_congestion_stat, TCP_CA_Loss);
		break;

	default:
		WIFIPRO_WARNING("unvalid state, ca_state = %d", ca_state);
		break;
	}
}


static void wifipro_update_wlan_rtt(unsigned int rtt, struct sock *sk, unsigned int dest_addr)
{
	unsigned int rtt_total = 0;
	unsigned int trans_total = 0;
	unsigned int expired_after_base = 0;
	unsigned long base_expire = jiffies;
	int i;
	bool is_head_set = false;
	bool is_rtt_added = false;
	wifipro_rtt_second_stat_t *curr = wlan_rtt_second_stat_head;
	struct inet_connection_sock *icsk = NULL;
	struct tcp_sock *tp = NULL;

	wlan_bqe_rtt_stat.rtt += rtt;
	wlan_bqe_rtt_stat.packets++;
	wlan_bqe_rtt_stat.last_update = jiffies;

	wlan_sample_rtt_stat.rtt += rtt;
	wlan_sample_rtt_stat.packets++;
	wlan_sample_rtt_stat.last_update = jiffies;

	if (!wlan_rtt_second_stat_head) {
		return;
	}

	icsk = inet_csk(sk);
	tp = tcp_sk(sk);

	if (time_after(wlan_last_expire, base_expire)) {
		base_expire = wlan_last_expire;
	}

	for (i = 1; i <= WIFIPRO_RTT_RECORD_SECONDS; i++) {
		if (time_after(curr->expire + (WIFIPRO_RTT_RECORD_SECONDS - 1)*HZ, jiffies)) {  /*it's valid now*/
			if (!is_rtt_added && time_after(curr->expire, jiffies) && time_before(curr->expire - 1*HZ, jiffies)) {
				curr->total_rtt += rtt;
				curr->amount++;
				is_rtt_added = true;
			}

			if (time_before(curr->expire, wlan_rtt_second_stat_head->expire)) {  /*head always point to first expired one*/
				wlan_rtt_second_stat_head = curr;
				is_head_set = true;
			}

			if (time_after(curr->expire, wlan_last_expire)) {
				wlan_last_expire = curr->expire;
			}
			/*update all rtt_total and trans_total, this will output final wlan_rtt_average*/
			rtt_total += curr->total_rtt;
			trans_total += curr->amount;
		} else {
			curr->expire =  base_expire + (expired_after_base + 1)*HZ;
			expired_after_base++;
			if (time_before(wlan_last_expire, jiffies) && !is_rtt_added) {
				curr->total_rtt = rtt;
				curr->amount = 1;
				rtt_total += curr->total_rtt;
				trans_total += curr->amount;
				if (!is_head_set) {  /*all nodes expired*/
					wlan_rtt_second_stat_head = curr;
					is_head_set = true;
				}
				is_rtt_added = true;
			} else {
				curr->total_rtt = 0;
				curr->amount = 0;
			}
			wlan_last_expire = curr->expire;
		}
		curr = curr->next;
	}

	if (trans_total != 0) {
		wlan_rtt_average = (rtt_total / trans_total) * WIFIPRO_TICK_TO_MS;
		wlan_rtt_calc_pkg = trans_total;
		wlan_when_recorded_rtt = jiffies;
	}

	WIFIPRO_DEBUG("wlan: rto=%d  srtt_us=%d  pks=%d, rtt_avg=%d",
		icsk->icsk_rto, tp->srtt_us, wlan_rtt_calc_pkg, wlan_rtt_average);
}


static void wifipro_update_mobile_rtt(unsigned int rtt, struct sock *sk, unsigned int dest_addr)
{
	unsigned int rtt_total = 0;
	unsigned int trans_total = 0;
	unsigned int expired_after_base = 0;
	unsigned long base_expire = jiffies;
	int i;
	bool is_head_set = false;
	bool is_rtt_added = false;
	wifipro_rtt_second_stat_t *curr = mobile_rtt_second_stat_head;
	struct inet_connection_sock *icsk = NULL;
	struct tcp_sock *tp = NULL;

	mobile_bqe_rtt_stat.rtt += rtt;
	mobile_bqe_rtt_stat.packets++;
	mobile_bqe_rtt_stat.last_update = jiffies;

	if (!mobile_rtt_second_stat_head) {
		return;
	}

	icsk = inet_csk(sk);
	tp = tcp_sk(sk);

	if (time_after(mobile_last_expire, base_expire)) {
		base_expire = mobile_last_expire;
	}

	for (i = 1; i <= WIFIPRO_RTT_RECORD_SECONDS; i++) {
		if (time_after(curr->expire + (WIFIPRO_RTT_RECORD_SECONDS - 1)*HZ, jiffies)) { /*it's valid now*/
			if (!is_rtt_added && time_after(curr->expire, jiffies) && time_before(curr->expire - 1*HZ, jiffies)) {
				curr->total_rtt += rtt;
				curr->amount++;
				is_rtt_added = true;
			}

			if (time_before(curr->expire, mobile_rtt_second_stat_head->expire)) {  /*head always point to first expired one*/
				mobile_rtt_second_stat_head = curr;
				is_head_set = true;
			}

			if (time_after(curr->expire, mobile_last_expire)) {
				mobile_last_expire = curr->expire;
			}

			rtt_total += curr->total_rtt;
			trans_total += curr->amount;
		} else {
			curr->expire =  base_expire + (expired_after_base + 1)*HZ;
			expired_after_base++;
			if (time_before(mobile_last_expire, jiffies) && !is_rtt_added) {
				curr->total_rtt = rtt;
				curr->amount = 1;
				rtt_total += curr->total_rtt;
				trans_total += curr->amount;
				if (!is_head_set) {  /*all nodes expired*/
					mobile_rtt_second_stat_head = curr;
					is_head_set = true;
				}
				is_rtt_added = true;
			} else {
				curr->total_rtt = 0;
				curr->amount = 0;
			}
			mobile_last_expire = curr->expire;
		}
		curr = curr->next;
	}

	if (trans_total != 0) {
		mobile_rtt_average = (rtt_total / trans_total) * WIFIPRO_TICK_TO_MS;
		mobile_rtt_calc_pkg = trans_total;
		mobile_when_recorded_rtt = jiffies;
	}

	WIFIPRO_DEBUG("mobile: rto=%d  srtt_us=%d  pkts=%d, rtt_avg=%d",
		icsk->icsk_rto, tp->srtt_us, mobile_rtt_calc_pkg, mobile_rtt_average);
}


void wifipro_update_rtt(unsigned int rtt, struct sock *sk)
{
	unsigned int dest_addr = 0;
	struct inet_sock *inet = NULL;

	inet = inet_sk(sk);
	if (inet == NULL) {
		WIFIPRO_ERROR("GET NULL POINTER!");
		return;
	}
	dest_addr = htonl(inet->inet_daddr);
	if (!wifipro_is_not_local_or_lan_sock(dest_addr)) {
		return;
	}

	if (!strncmp(sk->wifipro_dev_name, "wlan",  4)) {
		wifipro_update_wlan_rtt(rtt, sk, dest_addr);
		updateWlanTcpStatByUID(get_socket_uid(sk), INDEX_WEBRTTDURATION, rtt);
		updateWlanTcpStatByUID(get_socket_uid(sk), INDEX_WEBRTTSEGS, 1);
        updateHidataTcpStatByUID(get_socket_uid(sk), INDEX_WEBRTTDURATION, rtt);
        updateHidataTcpStatByUID(get_socket_uid(sk), INDEX_WEBRTTSEGS, 1);
	} else if (!strncmp(sk->wifipro_dev_name, "rmnet", 5)) {
		wifipro_update_mobile_rtt(rtt, sk, dest_addr);
		updateRmnetTcpStatByUID(get_socket_uid(sk), INDEX_WEBRTTDURATION, rtt);
		updateRmnetTcpStatByUID(get_socket_uid(sk), INDEX_WEBRTTSEGS, 1);
	} else {
		WIFIPRO_DEBUG("unknown device, ignore");
		unknown_dev_count++;
	}
}

static void wifipro_cong_stat_init(wifipro_cong_sock_t *src, unsigned char offset, const char *name)
{
	wifipro_cong_sock_t *dst = src + offset;

	strncpy(dst->state_name, name, WIFIPRO_MAX_CA_NAME - 1);
	dst->state_name[WIFIPRO_MAX_CA_NAME - 1] = '\0';
}


static wifipro_rtt_second_stat_t *wifipro_rtt_init(bool is_wlan)
{
	int i;
	int rtt_node_count = 0;
	wifipro_rtt_second_stat_t *wifipro_rtt_second_stat_head = NULL;
	wifipro_rtt_second_stat_t *curr = NULL;
	wifipro_rtt_second_stat_t *prev = NULL;

	wifipro_rtt_second_stat_head = kzalloc(sizeof(wifipro_rtt_second_stat_t), GFP_KERNEL);
	if (wifipro_rtt_second_stat_head == NULL) {
		WIFIPRO_ERROR("kzalloc failed");
		return NULL;
	}
	rtt_node_count++;
	wifipro_rtt_second_stat_head->expire = jiffies + 1 * HZ;
	wifipro_rtt_second_stat_head->second_num = 0;

	prev = wifipro_rtt_second_stat_head;
	wifipro_rtt_second_stat_head->next = NULL;
	for (i = 1; i < WIFIPRO_RTT_RECORD_SECONDS; i++) {
		curr = kzalloc(sizeof(wifipro_rtt_second_stat_t), GFP_KERNEL);
		if (curr == NULL) {
			wifipro_rtt_free(wifipro_rtt_second_stat_head, rtt_node_count);
			WIFIPRO_ERROR("kzalloc failed");
			return NULL;
		}
		rtt_node_count++;
		curr->second_num = i;
		curr->expire = jiffies + (i+1)*HZ;
		curr->next = NULL;
		prev->next = curr;
		prev = curr;
	}
	curr->next = wifipro_rtt_second_stat_head;
	if (is_wlan)
		wlan_last_expire = curr->expire;
	else
		mobile_last_expire = curr->expire;

	return wifipro_rtt_second_stat_head;
}

static void wifipro_rtt_monitor_deinit(void)
{
	wifipro_rtt_free(mobile_rtt_second_stat_head, WIFIPRO_RTT_RECORD_SECONDS);
	mobile_rtt_second_stat_head = NULL;
	wifipro_rtt_free(wlan_rtt_second_stat_head, WIFIPRO_RTT_RECORD_SECONDS);
	wlan_rtt_second_stat_head = NULL;

}

static int __init wifipro_tcp_monitor_module_init(void)
{
	struct netlink_kernel_cfg wifipro_tcp_monitor_nl_cfg = {
		  .input = wifipro_tcp_monitor_nl_receive,
	};
	is_wifipro_running = true;
	is_wifipro_on = false;
	is_mcc_china = true;
	s_wlanTcpStat_index = 0;
	s_rmnetTcpStat_index = 0;
	g_user_space_pid = 0;
	wlan_last_expire = 0;
	wlan_rtt_average = 0;
	wlan_rtt_calc_pkg = 0;
	wlan_when_recorded_rtt = 0;
	mobile_last_expire = 0;
	mobile_rtt_average = 0;
	mobile_rtt_calc_pkg = 0;
	mobile_when_recorded_rtt = 0;
	unknown_dev_count = 0;
	unknown_segs_count = 0;
	g_wifipro_nlfd = NULL;
	wlan_rtt_second_stat_head = NULL;
	mobile_rtt_second_stat_head = NULL;
	wifipro_congestion_stat = NULL;

	g_wifipro_nlfd = netlink_kernel_create(&init_net,
		NETLINK_WIFIPRO_EVENT_NL,
		&wifipro_tcp_monitor_nl_cfg);
	if (!g_wifipro_nlfd) {
		WIFIPRO_ERROR("netlink init fail");
		return -1;
	}

	wifipro_congestion_stat = (wifipro_cong_sock_t *)kzalloc(WIFIPRO_CONG_ARRAY*sizeof(*wifipro_congestion_stat), GFP_KERNEL);
	if (wifipro_congestion_stat == NULL) {
		WIFIPRO_ERROR("kzalloc failed");
		return -1;
	}
	wifipro_cong_stat_init(wifipro_congestion_stat, TCP_CA_Open, "Open");
	wifipro_cong_stat_init(wifipro_congestion_stat, TCP_CA_Disorder, "Disorder");
	wifipro_cong_stat_init(wifipro_congestion_stat, TCP_CA_CWR, "CWR");
	wifipro_cong_stat_init(wifipro_congestion_stat, TCP_CA_Recovery, "Recovery");
	wifipro_cong_stat_init(wifipro_congestion_stat, TCP_CA_Loss, "Loss");

	wlan_rtt_second_stat_head = wifipro_rtt_init(true);
	if (!wlan_rtt_second_stat_head) {
		WIFIPRO_ERROR("wifipro_rtt_init failed");
		return -1;
	}
	mobile_rtt_second_stat_head = wifipro_rtt_init(false);
	if (!mobile_rtt_second_stat_head) {
		WIFIPRO_ERROR("wifipro_rtt_init failed");
		return -1;
	}

	return 0;
}

static void __exit wifipro_tcp_monitor_module_exit(void)
{
	wifipro_rtt_monitor_deinit();

	if (g_wifipro_nlfd && g_wifipro_nlfd->sk_socket) {
		sock_release(g_wifipro_nlfd->sk_socket);
		g_wifipro_nlfd = NULL;
	}

	if (wifipro_congestion_stat) {
		kfree(wifipro_congestion_stat);
		wifipro_congestion_stat = NULL;
	}
}

module_init(wifipro_tcp_monitor_module_init);
module_exit(wifipro_tcp_monitor_module_exit);

#ifdef CONFIG_HW_WIFIPRO_PROC

/*Output /proc/net/wifipro_log_level*/
static int wifipro_log_level_show(struct seq_file *seq, void *v)
{
	seq_printf(seq, "wifipro_log_level = %d", wifipro_log_level);
	seq_puts(seq, "\n");
	return 0;
}

static ssize_t wifipro_log_level_write(struct file *file, const char __user *buffer, size_t count, loff_t *data)
{

	unsigned char log_level = 0;
	WIFIPRO_ERROR("wifipro_log_level_write count = %d",count);
	if (count != 2){

		WIFIPRO_ERROR("unvalid count");
		return -EFAULT;
	}

	if (copy_from_user(&log_level, buffer, 1)) {
		WIFIPRO_ERROR("wifipro_log_level_write copy_from_user error");
		return -EFAULT;
	}
	log_level = log_level - '0';
	WIFIPRO_ERROR("wifipro_log_level_write new  log_level = %d",log_level);
	if (log_level >= WIFIPRO_ERR && log_level <= WIFIPRO_LOG_ALL) {

		WIFIPRO_ERROR("wifipro_log_level_write old wifipro_log_level = %d",wifipro_log_level);
		wifipro_log_level = log_level;
		WIFIPRO_ERROR("wifipro_log_level_write new wifipro_log_level = %d",wifipro_log_level);
	} else {

		WIFIPRO_ERROR("unvalid log level");
		return -EFAULT;

	}

	return count;
}

static int wifipro_log_level_open(struct inode *inode, struct file *file)
{
	return single_open_net(inode, file, wifipro_log_level_show);
}

static const struct file_operations wifipro_log_level_seq_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wifipro_log_level_open,
	.read	 = seq_read,
	.write   = wifipro_log_level_write,
	.llseek	 = seq_lseek,
	.release = single_release_net,
};

/*Called from the PROCfs module. This outputs /proc/net/wifipro_tcp_stat.*/
static int wifipro_snmp_seq_show(struct seq_file *seq, void *v)
{
	int i = 0, j = 0;
	struct timex  txc;
	struct rtc_time tm;
	struct net *net = seq->private;
	struct timeval tv;
	int bIndex = INDEX_WEBSENDSEGS, eIndex = INDEX_WEBRTTSEGS;
	int index = 0;

	do_gettimeofday(&tv);
	if (is_wifipro_on) {
		seq_puts(seq, "Wi-Fi Pro is on\n");
	} else {
		seq_puts(seq, "Wi-Fi Pro is off\n");
	}
	seq_puts(seq, "Current Time: ");
	do_gettimeofday(&(txc.time));
	rtc_time_to_tm(txc.time.tv_sec, &tm);
	seq_printf(seq, "%d:%d:%d\n", tm.tm_hour + 8, tm.tm_min, tm.tm_sec);

	seq_printf(seq, "\npackets info:\n");
	for (i = 0; wifipro_snmp_tcp_list[i].name != NULL; i++) {
		seq_printf(seq, "%s = ", wifipro_snmp_tcp_list[i].name);

		if (wifipro_snmp_tcp_list[i].entry == TCP_MIB_MAXCONN)
			seq_printf(seq, "%ld\n",
				   snmp_fold_field((void __percpu **)net->mib.wifipro_tcp_statistics,
						   wifipro_snmp_tcp_list[i].entry));
		else
			seq_printf(seq, "%lu\n",
				   snmp_fold_field((void __percpu **)net->mib.wifipro_tcp_statistics,
						   wifipro_snmp_tcp_list[i].entry));
	}

	{
		index = s_wlanTcpStat_index;
		seq_printf(seq, "\nwlan UID state:\n");
		seq_puts(seq, "UID\t");
		for (i = bIndex; i <= eIndex; i++) {
			seq_printf(seq, "%s\t", s_arrTitle[i]);
		}
		seq_puts(seq, "\n");

		for (i = 0; i <  index && i < MAX_UID_CNT ; i++) {
			seq_printf(seq, "%u\t", s_uidWlanTcpStat[i].uid.val);
			for (j = bIndex; j <= eIndex; j++) {
				seq_printf(seq, "%u\t", s_uidWlanTcpStat[i].counter[j]);
			}
			seq_puts(seq, "\n");
		}
		memset(s_uidWlanTcpStat, 0, sizeof(s_uidWlanTcpStat));
		s_wlanTcpStat_index = 0;
	}
	{
		index = s_rmnetTcpStat_index;
		seq_printf(seq, "\nmobile UID state:\n");
		seq_puts(seq, "UID\t");
		for (i = bIndex; i <= eIndex; i++) {
			seq_printf(seq, "%s\t", s_arrTitle[i]);
		}
		seq_puts(seq, "\n");
		for (i = 0; i <  index && i < MAX_UID_CNT ; i++) {
			seq_printf(seq, "%u\t", s_uidRmnetTcpStat[i].uid.val);
			for (j = bIndex; j <= eIndex; j++) {
				seq_printf(seq, "%u\t", s_uidRmnetTcpStat[i].counter[j]);
			}
			seq_puts(seq, "\n");
		}
		memset(s_uidRmnetTcpStat, 0, sizeof(s_uidRmnetTcpStat));
		s_rmnetTcpStat_index = 0;
	}

	if (wifipro_congestion_stat != NULL) {
		wifipro_cong_sock_t *dst = NULL;

		seq_printf(seq, "\nwifipro congestion state:\n");
		for (i = 1; i < WIFIPRO_CONG_ARRAY; i++) {
			dst = wifipro_congestion_stat + i;
			seq_printf(seq, "%s: occured %dtimes, last occured from %s:%d %lus ago\n",
				dst->state_name,
				dst->amount,
				wifipro_ntoa(dst->dst_addr), dst->dst_port,
				(jiffies - dst->when)/HZ);
		}
	}

	if (wlan_rtt_second_stat_head != NULL) {
		wifipro_rtt_second_stat_t *curr = wlan_rtt_second_stat_head;

		seq_printf(seq, "\nwlan rtt:\n");
		for (i = 0; i <= WIFIPRO_RTT_RECORD_SECONDS; i++) {
			seq_printf(seq, "%d: rtt %d, trans %d, %lums %s\n",
			   curr->second_num, curr->total_rtt, curr->amount,
			   time_after(curr->expire, jiffies)?(curr->expire - jiffies)*WIFIPRO_TICK_TO_MS:(jiffies - curr->expire)*WIFIPRO_TICK_TO_MS,
			   time_after(curr->expire, jiffies)?"later":"ago");
			curr = curr->next;
		}
		seq_printf(seq, "average rtt is %dms\npacket trans:%d\nrecorded %lus ago\n", wlan_rtt_average/8, wlan_rtt_calc_pkg, (jiffies - wlan_when_recorded_rtt)/HZ);
	}


	if (mobile_rtt_second_stat_head != NULL) {
		wifipro_rtt_second_stat_t *curr = mobile_rtt_second_stat_head;

		seq_printf(seq, "\nmobile rtt:\n");
		for (i = 0; i <= WIFIPRO_RTT_RECORD_SECONDS; i++) {
			seq_printf(seq, "%d: rtt %d, trans %d, %lums %s\n",
			   curr->second_num, curr->total_rtt, curr->amount,
			   time_after(curr->expire, jiffies)?(curr->expire - jiffies)*WIFIPRO_TICK_TO_MS:(jiffies - curr->expire)*WIFIPRO_TICK_TO_MS,
			   time_after(curr->expire, jiffies)?"later":"ago");
			curr = curr->next;
		}
		seq_printf(seq, "average rtt is %dms\npacket trans:%d\nrecorded %lus ago\n", mobile_rtt_average/8, mobile_rtt_calc_pkg, (jiffies - mobile_when_recorded_rtt)/HZ);
	}
	seq_printf(seq, "unknown_dev_count is %d\n", unknown_dev_count);
	seq_printf(seq, "unknown_segs_count is %d\n", unknown_segs_count);
	seq_puts(seq, "\n");
	return 0;
}

int wifipro_get_srtt(void)
{
	return wlan_rtt_average;
}

static int wifipro_snmp_seq_open(struct inode *inode, struct file *file)
{
	return single_open_net(inode, file, wifipro_snmp_seq_show);
}

long get_current_time(void){
    struct timex txc;
    do_gettimeofday(&(txc.time));
    return txc.time.tv_sec;
}

static int wifi_getHidataTcpStatIndex(kuid_t uid)
{
	int index =  INVALID_INDEX;
	int i = 0;
	if (!uid_valid(uid))
		return INVALID_INDEX;

	if (uid_eq(uid, KUIDT_INIT(0)) || uid_eq(uid, KUIDT_INIT(1000)))
		return INVALID_INDEX;

    for (i = 0; i < s_hidataTcpStat_index && i < MAX_UID_CNT ; i++) {
		if (uid_eq(uid, s_uidHidataTcpStat[i].uid)) {
			index = i;
			goto out;
		}
	}

	if (s_hidataTcpStat_index < MAX_UID_CNT) {
		index = s_hidataTcpStat_index;
		s_hidataTcpStat_index++;
		s_uidHidataTcpStat[index].uid = uid;
	}
out:
	return index;
}

static void updateHidataTcpStatByUID(kuid_t uid, int index, int count)
{
	int result = 0;
    int temp = 0;
    long s_time = 0;
    int i;
    if (!uid_valid(uid))
		return;

	if (uid_eq(uid, KUIDT_INIT(0)) || uid_eq(uid, KUIDT_INIT(1000)))
		return;

	result = wifi_getHidataTcpStatIndex(uid);

	if (result == INVALID_INDEX){
        s_time = s_uidHidataTcpStat[0].last_time;
        temp = 0;
        for(i = 1; i < MAX_UID_CNT; i++){
            if(s_uidHidataTcpStat[i].last_time < s_time){
                s_time = s_uidHidataTcpStat[i].last_time;
                temp = i;
            }
        }
        memset(&s_uidHidataTcpStat[temp],0,sizeof(HIDATAUIDTCPStat));
        s_uidHidataTcpStat[temp].uid = uid;
        result = temp;
    }

	if (!uid_eq(uid, s_uidHidataTcpStat[result].uid))
		return;

	s_uidHidataTcpStat[result].counter[index] += count;
    s_uidHidataTcpStat[result].last_time = get_current_time();
}


static int hidata_tcp_info_report(int pid, kuid_t uid)
{
	int ret;
	int size;
    int result = 0;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct wifipro_nl_packet_msg *packet = NULL;

	if (!pid || !g_wifipro_nlfd) {
		WIFIPRO_ERROR("cannot notify event, pid = %d", pid);
		return -1;
	}
    
    result = wifi_getHidataTcpStatIndex(uid);
    if(result == INVALID_INDEX){
        WIFIPRO_ERROR("hidata_tcp_info_report error");
		return -1;
    }

	mutex_lock(&wifipro_nl_send_sem);
	size = sizeof(struct wifipro_nl_packet_msg);
	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb) {
		WIFIPRO_ERROR("alloc skb fail");
		ret = -1;
		goto end;
	}

	nlh = nlmsg_put(skb, 0, 0, NETLINK_WIFIPRO_GET_MSG, size, 0);
	if (!nlh) {
		kfree_skb(skb);
		skb = NULL;
		ret = -1;
		goto end;
	}

	packet = nlmsg_data(nlh);
	memset(packet, 0, sizeof(struct wifipro_nl_packet_msg));

	packet->rtt = s_uidHidataTcpStat[result].counter[INDEX_WEBRTTDURATION];
	packet->rtt_pkts = s_uidHidataTcpStat[result].counter[INDEX_WEBRTTSEGS];
	packet->tcp_rx_pkts = s_uidHidataTcpStat[result].counter[INDEX_WEBRECVSEGS];
	packet->tcp_tx_pkts = s_uidHidataTcpStat[result].counter[INDEX_WEBSENDSEGS];
	packet->tcp_retrans_pkts = s_uidHidataTcpStat[result].counter[INDEX_WEBRESENDSEGS];
    
	ret = netlink_unicast(g_wifipro_nlfd, skb, pid, MSG_DONTWAIT);

end:
	mutex_unlock(&wifipro_nl_send_sem);
	return ret;
}

static const struct file_operations wifipro_snmp_seq_fops = {
	.owner	 = THIS_MODULE,
	.open	 = wifipro_snmp_seq_open,
	.read	 = seq_read,
	.llseek	 = seq_lseek,
	.release = single_release_net,
};

int wifipro_init_proc(struct net *net)
{
	if (!proc_create("wifipro_tcp_stat", S_IRUGO, net->proc_net, &wifipro_snmp_seq_fops))
		goto out_wifipro_tcp_stat;
	if (!proc_create("wifipro_log_level", (S_IRUSR|S_IRGRP) | (S_IWUSR|S_IWGRP), net->proc_net, &wifipro_log_level_seq_fops))
		goto out_wifipro_log_level;

	return 0;

out_wifipro_tcp_stat:
	return -ENOMEM;
out_wifipro_log_level:
	return -ENOMEM;
}
#endif

