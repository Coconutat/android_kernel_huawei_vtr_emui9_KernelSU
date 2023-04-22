#ifndef __NM_STATIS_H__
#define __NM_STATIS_H__

#include <net/sock.h>
#include <linux/tcp.h>
#include "nm_types.h"

#ifdef CONFIG_HW_NETWORK_MEASUREMENT
static inline void update_snd_pkts(struct sock *sk, int pktlen)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		/* !!pktlen is equal to either 0 or 1 */
		p->snd_pkts += !!pktlen;
		p->snd_pkts_wrap |= (!p->snd_pkts & !!pktlen);
	}
}

static inline void update_rcv_pkts(struct sock *sk, int pktlen)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		/* !!pktlen is equal to either 0 or 1 */
		p->rcv_pkts += !!pktlen;
		p->rcv_pkts_wrap |= (!p->rcv_pkts & !!pktlen);
	}
}

static inline void set_syn_rtt(struct sock *sk, u32 srtt_us)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis && !sk->sk_tcp_statis->syn_rtt) {
		p = sk->sk_tcp_statis;
		p->syn_rtt = srtt_us;
	}
}

static inline void update_syn_retrans(struct sock *sk)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis)
		p = sk->sk_tcp_statis;
}

static inline void update_snd_zero_win_cnts(struct sock *sk)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		p->snd_zero_win_cnts++;
		p->snd_zero_win_cnts_wrap |= !p->snd_zero_win_cnts;
	}
}

static inline void update_rcv_zero_win_cnts(struct sock *sk)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		p->rcv_zero_win_cnts++;
		p->rcv_zero_win_cnts_wrap |= !p->rcv_zero_win_cnts;
	}
}

static inline void update_ul_fast_retrans(struct sock *sk)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		p->ul_fast_retrans++;
		p->ul_fast_retrans_wrap |= !p->ul_fast_retrans;
	}
}

static inline void update_ul_timeout_retrans(struct sock *sk)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		p->ul_timeout_retrans++;
		p->ul_timeout_retrans_wrap |= !p->ul_timeout_retrans;
	}
}

static inline void update_dl_disorder_pkts(struct sock *sk)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		p->dl_disorder_pkts++;
		p->dl_disorder_pkts_wrap |= !p->dl_disorder_pkts;
	}
}

static inline void update_dl_three_dup_acks(struct sock *sk)
{
	struct tcp_statistics *p = NULL;

	if (sk && sk->sk_tcp_statis) {
		p = sk->sk_tcp_statis;
		p->dl_three_dup_acks++;
		p->dl_three_dup_acks_wrap |= !p->dl_three_dup_acks;
	}
}
#endif /* CONFIG_HW_NETWORK_MEASUREMENT */
#endif /* __NM_STATIS_H__ */
