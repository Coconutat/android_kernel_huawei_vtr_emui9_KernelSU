#ifndef __NETWORK_MEASUREMENT_H__
#define __NETWORK_MEASUREMENT_H__

#include <linux/slab.h>
#include <linux/sort.h>
#include <linux/types.h>
#include <linux/ktime.h>
#include <linux/string.h>
#include <linux/uidgid.h>
#include <linux/bsearch.h>
#include <linux/rculist.h>
#include <net/tcp.h>

#include "nm_log.h"
#include "nm_types.h"
#include "nm_statis.h"
#include "sample_ctrl.h"

#ifdef CONFIG_HW_NETWORK_MEASUREMENT

extern int network_measure;
extern int network_measure_timeouts;
extern int network_measure_tcp;

static inline bool nm_sample_on(struct sock *sk)
{
	return sk->sk_nm_uid && (network_measure & VALVE_OPEN);
}

extern void tcp_measure_init(struct sock *sk);
extern void tcp_measure_deinit(struct sock *sk);
extern void udp_measure_init(struct sock *sk, struct sk_buff *skb);
extern void udp_measure_deinit(struct sock *sk);
extern void nm_nse(struct sock *sk, struct sk_buff *skb, int protocol,
		   int offset, int len, int direction, u8 func);
#endif /* CONFIG_HW_NETWORK_MEASUREMENT */
#endif /* __NETWORK_MEASUREMENT_H__ */
