#ifndef __WIFI_DELAYST_H
#define __WIFI_DELAYST_H

#include <linux/ipv6.h>
#include <linux/tcp.h>
#include <net/ipv6.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <linux/tracepoint.h>
#include<linux/timer.h>
#include<linux/timex.h>
#include<linux/rtc.h>

#define DELAY_FILTER_NAME_MAX   30
#define DELAY_NORMAL_TIME          (30*1000)                  /*30ms*/
#define UID_MATCH_ALL                   888888                    /*match all uid*/
/*flow control,for auto delay statistics,based on bps,50Mbps
#define DELAY_FLOW_THRESHOLD  (50*1024*1024/8)
*/
/*flow control,for auto delay statistics,based on pps  50Mbps/1.5KB(1536B)*/
#define DELAY_FLOW_THRESHOLD  (((50*1024*1024)/8)/1536)

/*macro definition for delay record point*/
enum delay_record_point{
	TP_SKB_SEND               =0,
	TP_SKB_IP                 =1,
	TP_SKB_HMAC_XMIT          =2,
	TP_SKB_HMAC_TX            =3,
	TP_SKB_DMAC               =4,

	TP_SKB_RECV               =0,
	TP_SKB_HMAC_UPLOAD        =2,
	TP_SKB_HMAC_RX            =3,

	TP_SKB_MAX_ENTRY          =5
};

#define T_Delay 5             /*Delay Gaps*/

typedef struct delay_stat{
	uint32_t T_gap[TP_SKB_MAX_ENTRY][T_Delay];
	uint32_t T_TotalDelay[TP_SKB_MAX_ENTRY];
	uint32_t T_TotalPkts[TP_SKB_MAX_ENTRY];
}DELAY_STAT_T;

typedef enum{
	flag_auto =0,
	flag_on,
	flag_off,
}dp_switch_enum;

typedef enum{
	mode_stat =0,
	mode_trace,
}dp_mode_enum;

typedef struct dp_setting{     /*delay_print settings for users*/
	dp_switch_enum        dp_switch;
	dp_mode_enum          dp_mode;
	unsigned int              print_interval;
	unsigned int 		   android_uid;
}DP_SETTINGS_T;

typedef enum{
	TP_SKB_TYPE_TCP =0,
	TP_SKB_TYPE_UDP ,
}skbtype_enum;

typedef enum{
	TP_SKB_DIRECT_SND  =0,
	TP_SKB_DIRECT_RCV,
}skbdirect_enum;

/*total len
*  4+4+5*8=48
*/
typedef struct delayskbcb{
	__u32    pdirect                           :1;           /*0 for send,1 for receive*/
	__u32    pproto                           :1;           /*0 for tcp ,1 for udp*/
	__u32    reserved                        :30;
	__u32    android_uid;
	ktime_t  ptime[TP_SKB_MAX_ENTRY];          /*timestamp for skb*/
}DELAYSKB_CB_T;

#ifdef CONFIG_MPTCP
#define DELAYST_SKB_CB(__skb)     ((DELAYSKB_CB_T *)&((__skb)->cb[80]))
#else
#define DELAYST_SKB_CB(__skb)     ((DELAYSKB_CB_T *)&((__skb)->cb[48]))
#endif

#define IS_NEED_RECORD_DELAY(__skb, __index)   (0 != (skbprobe_get_skbtime(__skb,__index).tv64))

#define PACKET_IS_ENOUGH_FOR_PRINT(__DIRECT, __index)     \
	(__DIRECT.T_TotalPkts[__index] >= Settings.print_interval) //packet is enough for print

/*copy skb->cb to other fragment ,using in ip_copy_meta_data*/
#ifdef CONFIG_MPTCP
#define MEMCPY_SKB_CB(__to, __from)   (memcpy(__to->cb,__from->cb,128))
#else
#define MEMCPY_SKB_CB(__to, __from)   (memcpy(__to->cb,__from->cb,96))
#endif

#define IS_DIRECT(__skb, __direct)                      (__direct  ==  skbprobe_get_direct(__skb))
#define IS_DELAY_SWITCH_DISABLE                    (Settings.dp_switch == flag_off )
#define IS_DELAY_SWITCH_AUTO                       (Settings.dp_switch == flag_auto)
#define GET_TIME_FROM_SKB(__skb,__index)    (ktime_to_us(skbprobe_get_skbtime(__skb,__index)))

#define DIRECT_RETURN_IF_SWITCH_OFF         \
	do {                                    \
		if(IS_DELAY_SWITCH_DISABLE)  {      \
			return;                         \
		}                                   \
	}while(0)


#define CLEAN_DELAY_RECORD  \
	do{                                               \
		memset(&RcvDelay_S,0,sizeof(DELAY_STAT_T));   \
		memset(&Delay_S,0,sizeof(DELAY_STAT_T));      \
	}while(0)

extern char tcp_delay_filter[DELAY_FILTER_NAME_MAX] ;
extern DP_SETTINGS_T Settings;

extern DELAY_STAT_T Delay_S;
extern DELAY_STAT_T RcvDelay_S;

extern u8 delayst_switch;

/*get time gap from DELAY_STAT_T*/
#define GET_RCV_UPLOAD(__index)      (RcvDelay_S.T_gap[TP_SKB_HMAC_UPLOAD][__index])
#define GET_RCV_IP(__index)               (RcvDelay_S.T_gap[TP_SKB_IP][__index])
#define GET_RCV_RECV(__index)          (RcvDelay_S.T_gap[TP_SKB_RECV][__index])

#define GET_SND_IP(__index)                (Delay_S.T_gap[TP_SKB_IP][__index])
#define GET_SND_XMIT(__index)           (Delay_S.T_gap[TP_SKB_HMAC_XMIT][__index])
#define GET_SND_TX(__index)               (Delay_S.T_gap[TP_SKB_HMAC_TX][__index])
#define GET_SND_DMAC(__index)          (Delay_S.T_gap[TP_SKB_DMAC][__index])

#define GET_AVG_UPLOAD   (RcvDelay_S.T_TotalDelay[TP_SKB_HMAC_UPLOAD]/RcvDelay_S.T_TotalPkts[TP_SKB_HMAC_UPLOAD])
#define GET_RCV_AVG_IP    (RcvDelay_S.T_TotalDelay[TP_SKB_IP]/RcvDelay_S.T_TotalPkts[TP_SKB_IP])
#define GET_AVG_RECV       (RcvDelay_S.T_TotalDelay[TP_SKB_RECV]/RcvDelay_S.T_TotalPkts[TP_SKB_RECV])

#define GET_SND_AVG_IP    (Delay_S.T_TotalDelay[TP_SKB_IP]/Delay_S.T_TotalPkts[TP_SKB_IP])
#define GET_AVG_XMIT       (Delay_S.T_TotalDelay[TP_SKB_HMAC_XMIT]/Delay_S.T_TotalPkts[TP_SKB_HMAC_XMIT])
#define GET_AVG_TX           (Delay_S.T_TotalDelay[TP_SKB_HMAC_TX]/Delay_S.T_TotalPkts[TP_SKB_HMAC_TX])
#define GET_AVG_DMAC      (Delay_S.T_TotalDelay[TP_SKB_DMAC]/Delay_S.T_TotalPkts[TP_SKB_DMAC])

#define GET_UPLOAD_ALL   GET_RCV_UPLOAD(0), GET_RCV_UPLOAD(1), GET_RCV_UPLOAD(2), GET_RCV_UPLOAD(3), GET_RCV_UPLOAD(4)
#define GET_RCV_IP_ALL    GET_RCV_IP(0), GET_RCV_IP(1), GET_RCV_IP(2), GET_RCV_IP(3), GET_RCV_IP(4)
#define GET_RECV_ALL       GET_RCV_RECV(0),  GET_RCV_RECV(1),  GET_RCV_RECV(2),  GET_RCV_RECV(3),  GET_RCV_RECV(4)
#define GET_SND_IP_ALL    GET_SND_IP(0), GET_SND_IP(1),GET_SND_IP(2), GET_SND_IP(3), GET_SND_IP(4)
#define GET_XMIT_ALL        GET_SND_XMIT(0), GET_SND_XMIT(1), GET_SND_XMIT(2), GET_SND_XMIT(3), GET_SND_XMIT(4)
#define GET_TX_ALL            GET_SND_TX(0), GET_SND_TX(1), GET_SND_TX(2), GET_SND_TX(3), GET_SND_TX(4)
#define GET_DMAC_ALL       GET_SND_DMAC(0), GET_SND_DMAC(1), GET_SND_DMAC(2), GET_SND_DMAC(3), GET_SND_DMAC(4)



#define TP_STORE_ADDR_PORTS_V4(__entry, inet, sk)                   \
	do {                                                            \
		struct sockaddr_in *v4 = (void *)__entry->saddr;        \
		                                                        \
		v4->sin_family = AF_INET;                               \
		v4->sin_port = inet->inet_sport;                        \
		v4->sin_addr.s_addr = inet->inet_saddr;                 \
		v4 = (void *)__entry->daddr;                            \
		v4->sin_family = AF_INET;                               \
		v4->sin_port = inet->inet_dport;                        \
		v4->sin_addr.s_addr = inet->inet_daddr;                 \
	} while (0)

#if IS_ENABLED(CONFIG_IPV6)

#define TP_STORE_ADDR_PORTS(__entry, inet, sk)                 \
	do {                                                       \
		if (sk->sk_family == AF_INET6) {                    \
			struct sockaddr_in6 *v6 = (void *)__entry->saddr; \
			                                               \
			v6->sin6_family = AF_INET6;                     \
			v6->sin6_port = inet->inet_sport;               \
			v6->sin6_addr = inet6_sk(sk)->saddr;            \
			v6 = (void *)__entry->daddr;                    \
			v6->sin6_family = AF_INET6;                     \
			v6->sin6_port = inet->inet_dport;               \
			v6->sin6_addr = sk->sk_v6_daddr;                \
		} else                                              \
		   TP_STORE_ADDR_PORTS_V4(__entry, inet, sk);      \
	} while (0)

#else

#define TP_STORE_ADDR_PORTS(__entry, inet, sk)          \
    TP_STORE_ADDR_PORTS_V4(__entry, inet, sk);

#endif

#define TP_SKB_STORE_ADDR_V4(__entry, skb, sport, dport)           \
	do {                                                           \
		struct sockaddr_in *v4 = (void *)__entry->saddr;        \
		struct iphdr *iph = ip_hdr(skb);                        \
		                                                       \
		v4->sin_family = AF_INET;                               \
		v4->sin_port = sport;                                   \
		if (iph) v4->sin_addr.s_addr = iph->saddr;              \
		v4 = (void *)__entry->daddr;                            \
		v4->sin_family = AF_INET;                               \
		v4->sin_port = dport;                                   \
		if (iph) v4->sin_addr.s_addr = iph->daddr;              \
	} while (0)

#if IS_ENABLED(CONFIG_IPV6)

#define TP_SKB_STORE_ADDR(__entry, skb, sport, dport)          \
	do {                                                       \
		if (skb->protocol == htons(ETH_P_IPV6)) {           \
			struct sockaddr_in6 *v6 = (void *)__entry->saddr; \
			struct ipv6hdr *hdr = ipv6_hdr(skb);            \
			                                               \
			v6->sin6_family = AF_INET6;                        \
			v6->sin6_port = sport;                             \
			if (hdr) v6->sin6_addr = hdr->saddr;               \
			v6 = (void *)__entry->daddr;                   \
			v6->sin6_family = AF_INET6;                    \
			v6->sin6_port = dport;                         \
			if (hdr) v6->sin6_addr = hdr->daddr;           \
		} else if (skb->protocol == htons(ETH_P_IP))       \
			TP_SKB_STORE_ADDR_V4(__entry, skb, sport, dport); \
	} while (0)

#else

#define TP_SKB_STORE_ADDR(__entry, skb, sport, dport)                  \
    TP_SKB_STORE_ADDR_V4(__entry, skb, sport, dport);

#endif

skbdirect_enum skbprobe_get_direct(struct sk_buff *pskb) ;
void skbprobe_set_direct(struct sk_buff *pskb, skbdirect_enum direct);
skbtype_enum skbprobe_get_proto(struct sk_buff *pskb);
void skbprobe_set_proto(struct sk_buff *pskb, skbtype_enum proto);
ktime_t skbprobe_get_skbtime(struct sk_buff *pskb, int time_index);
__u32 skbprobe_get_skbuid(struct sk_buff *pskb);

void skbprobe_record_first(struct sk_buff *skb, u32 type);
void skbprobe_record_time(struct sk_buff *skb, int index);
void skbprobe_record_proto(struct sk_buff *skb, __u8 n);
u32  skbprobe_get_latency(struct sk_buff *skb, int t1, int t2);
u32 skbprobe_get_latency(struct sk_buff *skb, int t1, int t2);
int  is_uid_match(struct sk_buff *skb);

void delay_flow_ctl(struct sk_buff *skb);
void delay_record_send(DELAY_STAT_T* delay, struct sk_buff *skb);
void delay_record_receive(DELAY_STAT_T *delay, struct sk_buff *skb);
void delay_record_ip_combine(struct sk_buff *skb, skbdirect_enum direct);
void delay_record_first_combine(struct sock *sk, struct sk_buff *skb, skbdirect_enum direct, skbtype_enum type);
void delay_record_gap(struct sk_buff *skb);
void delay_record_print_combine(struct sk_buff *skb);
void delay_record_rcv_combine(struct sk_buff *skb, struct sock *sk, skbtype_enum type);
void  delay_record_snd_combine(struct sk_buff *skb);
void delay_print_time_exception(struct sk_buff *skb, int t1, int t2);
int proc_wifi_delay_command(struct ctl_table *ctl, int write, void __user *buffer,
			 size_t *lenp, loff_t *ppos);

#endif  /*end __WIFI_DELAYST_H*/
