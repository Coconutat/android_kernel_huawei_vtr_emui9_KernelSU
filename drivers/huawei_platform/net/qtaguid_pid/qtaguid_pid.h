#ifndef _QTAGUID_PID_H
#define _QTAGUID_PID_H

void qtaguid_pid_put(const struct sock *sk);
void qtaguid_pid_remove(const struct sock *sk);

void if_pid_stat_update(const char *ifname, uid_t uid,
			const struct sock *sk, const struct sk_buff *skb,
			int active_set, unsigned int family,
			enum ifs_tx_rx direction, int proto, int bytes);

void iface_pid_stat_create_ipv6(struct net_device *net_dev, struct inet6_ifaddr *ifa);

void iface_pid_stat_create(struct net_device *net_dev, struct in_ifaddr *ifa);

extern const struct file_operations proc_qtaguid_pid_stats_fops;

#endif /* _QTAGUID_PID_H */
