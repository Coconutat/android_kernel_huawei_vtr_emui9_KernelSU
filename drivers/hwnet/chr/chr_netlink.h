#ifndef _CHR_NETLINK_H
#define _CHR_NETLINK_H

#define CHR_SPEED_SLOW_EVENT 8
#define CHR_NETLINK_INIT 1
#define CHR_NETLINK_EXIT 0
#define DIFF_SRC_IP_ADDR_MAX 2

#define DIFF_DST_IP_ADDR_MAX 3
#define TIMER_3_MINUTES (180*HZ)
#define TIMER_60_MINUTES (180*20*HZ)

#define CHR_TIMER_3 1
#define CHR_TIMER_60 2
#define RNT_SIZE (2)

enum {
	NETLINK_CHR_REG = 0,    /* send from apk to register the PID for netlink kernel socket */
	NETLINK_CHR_KER_MSG,    /* send from kernel to apk */
	NETLINK_CHR_UNREG,       /* when apk exit send this type message to unregister */
	NETLINK_CHR_SET_APP_UID, /* send from apk to kernel, set top app's uid */
};

struct chr_nl_packet_msg {
	int chr_event;
	unsigned int src_addr;
	struct http_return rtn_stat[RNT_SIZE];
};

struct chr_netinterface_info_struct {
	unsigned int dstAddrArrayIndex;
	unsigned int dst_addr[DIFF_DST_IP_ADDR_MAX];
	unsigned int src_addr;
};

struct tag_chr_msg2knl {
	struct nlmsghdr hdr;
	int index;
	unsigned int uid;
};

void notify_chr_thread_to_send_msg(unsigned int dst_addr, unsigned int src_addr);
int chr_notify_event(int event, int pid, unsigned int src_addr, struct http_return *prtn);
extern unsigned int g_user_space_pid;
#endif /*_CHR_NETLINK_H*/
