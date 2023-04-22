#ifndef         _HW_HIMOS_UDP_STATS_H
#define         _HW_HIMOS_UDP_STATS_H

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/genetlink.h>
#include <net/genetlink.h>

int himos_start_udp_stats(struct genl_info *info);
int himos_stop_udp_stats(struct genl_info *info);
int himos_keepalive_udp_stats(struct genl_info *info);

void himos_udp_stats(struct sock *sk, __u32 inbytes, __u32 outbytes);

int himos_udp_stats_init(void) __init;

#endif

