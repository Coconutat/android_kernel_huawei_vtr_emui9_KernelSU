#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/time.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <uapi/linux/netlink.h>
#include <linux/kthread.h>
#include "wbc_hw_hook.h"
#include "chr_netlink.h"

#undef HWLOG_TAG
#define HWLOG_TAG chr_netlink
HWLOG_REGIST();
MODULE_LICENSE("GPL");

/************************************************************
						MOCRO   DEFINES
*************************************************************/
DEFINE_MUTEX(chr_receive_sem);

/********************************
    netlink variables for
    communicate between kernel and apk
*********************************/
/*netlink socket fd*/
static struct sock *g_chr_nlfd;
/*save user space progress pid when user space netlink socket registering.*/
unsigned int g_user_space_pid;
static struct task_struct *g_chr_netlink_task;
static int g_chr_module_state = CHR_NETLINK_EXIT;
/*save server's address when data speed is slow.*/
static struct chr_netinterface_info_struct g_chr_netIfInfo_struct[DIFF_SRC_IP_ADDR_MAX];
/*backup of server address */
static unsigned int g_dest_addr_temp;
/*backup of src address */
static unsigned int g_src_addr_temp;
/*chr timer state,there is 2 state:3 minute and 60 minute.*/
static unsigned int g_chr_timer_state;
/*
 * Tcp protocol use this semaphone to inform chr netlink thread when data speed
 * is slow
 */
static struct semaphore g_chr_netlink_sync_sema;
/*this lock is used to protect global variable.*/
static spinlock_t dest_addr_timer_lock;
static spinlock_t chr_send_lock;
static struct timer_list g_chr_netlink_timer;
/*send a msg with server's address and port.*/
void notify_chr_thread_to_send_msg(unsigned int dst_addr, unsigned int src_addr)
{
	if (g_chr_module_state == CHR_NETLINK_INIT)  {
		spin_lock_bh(&dest_addr_timer_lock);

		/*
		 * Save server's address and port when the timer state is
		 * CHR_TIMER_3.
		 */
		if (g_chr_timer_state == CHR_TIMER_3) {
			g_dest_addr_temp = dst_addr;
			g_src_addr_temp = src_addr;
			spin_unlock_bh(&dest_addr_timer_lock);

			/*inform chr netlink thread when data speed is slow*/
			up(&g_chr_netlink_sync_sema);
		} else {
			/*release the lock.*/
			spin_unlock_bh(&dest_addr_timer_lock);
		}
	}
}


/************************************************************
				STATIC  FUNCTION  DEFINES
*************************************************************/

/*
 * Netlink socket's callback function,it will be called by system when
 * user space send a message to kernel.
 * this function will save user space progress pid.
 */
static void kernel_chr_receive(struct sk_buff *__skb)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb;
	struct tag_chr_msg2knl *hmsg = NULL;

	if (NULL == __skb) {
		hwlog_err("Invalid parameter: zero pointer reference(__skb)\n");
		return;
	}
	skb = skb_get(__skb);
	if (NULL == skb) {
		hwlog_err("kernel_chr_receive: skb = NULL\n");
		return;
	}
	mutex_lock(&chr_receive_sem);
	if (skb->len >= NLMSG_HDRLEN) {
		nlh = nlmsg_hdr(skb);
		if (NULL == nlh) {
			hwlog_err("kernel_chr_receive:  nlh = NULL\n");
			kfree_skb(skb);
			mutex_unlock(&chr_receive_sem);
			return;
		}
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr)) &&
			(skb->len >= nlh->nlmsg_len)) {
			if (nlh->nlmsg_type == NETLINK_CHR_REG) {
				/*
				 * Save user space progress pid when register
				 * netlink socket.
				 */
				g_user_space_pid = nlh->nlmsg_pid;
			} else if (nlh->nlmsg_type == NETLINK_CHR_UNREG) {
				g_user_space_pid = 0;
			} else if (nlh->nlmsg_type == NETLINK_CHR_SET_APP_UID && nlh->nlmsg_len >= sizeof(struct tag_chr_msg2knl)) {
				hmsg = (struct tag_chr_msg2knl *)nlh;
				set_report_app_uid(hmsg->index, hmsg->uid);
			}
		}
	}
	kfree_skb(skb);
	mutex_unlock(&chr_receive_sem);
}

/*
 * Netlink socket thread,
 * 1.it will receive the message from tcp protocal;
 * 2.save server's address and port;
 * 3.change timer state;
 * 4.send a message to user space when data speed is slow;
 */
static int chr_netlink_thread(void *data)
{
	int i = 0;
	int j = 0;
	struct chr_netinterface_info_struct *netIfInfo_struct_ptr = NULL;

	while (1) {
		if (kthread_should_stop())
			break;

		/*
		 * Netlink thread will block at this semaphone when data speed
		 * is nomal, only tcp protocol up the sema this thread will go
		 * to next sentence.
		 */
		down(&g_chr_netlink_sync_sema);

		if (g_user_space_pid != 0) {
			spin_lock_bh(&dest_addr_timer_lock);

			for (i = 0; i < DIFF_SRC_IP_ADDR_MAX; i++) {
				if (g_chr_netIfInfo_struct[i].src_addr == 0) {
					g_chr_netIfInfo_struct[i].src_addr =
						g_src_addr_temp;
					break;
				} else if (g_chr_netIfInfo_struct[i].src_addr == g_src_addr_temp) {
					break;
				}
			}

			if (i != DIFF_SRC_IP_ADDR_MAX) {
				netIfInfo_struct_ptr = &(g_chr_netIfInfo_struct[i]);
				if (netIfInfo_struct_ptr->dstAddrArrayIndex < DIFF_DST_IP_ADDR_MAX) {
					/*if the server's address is different,
					here will save the address.*/
					for (j = 0; (j < DIFF_DST_IP_ADDR_MAX) && (j < netIfInfo_struct_ptr->dstAddrArrayIndex); j++) {
						if (g_dest_addr_temp == netIfInfo_struct_ptr->dst_addr[j])
							break;
					}

					if (j == netIfInfo_struct_ptr->dstAddrArrayIndex) {
						netIfInfo_struct_ptr->dst_addr[j] = g_dest_addr_temp;
						netIfInfo_struct_ptr->dstAddrArrayIndex++;
					}
				}
				/*
				 * If there is 3 different server address if
				 * recoeded, will change timer state to
				 * CHR_TIMER_60, timer's timeout will change to
				 * 60 minute.
				 */
				if ((netIfInfo_struct_ptr->dstAddrArrayIndex == DIFF_DST_IP_ADDR_MAX) && (g_chr_timer_state == CHR_TIMER_3)) {
					spin_unlock_bh(&dest_addr_timer_lock);
					mod_timer(&g_chr_netlink_timer,
						jiffies + TIMER_60_MINUTES);
					g_chr_timer_state = CHR_TIMER_60;

					chr_notify_event(CHR_SPEED_SLOW_EVENT,
						g_user_space_pid, 0xffffffff, NULL);
				} else {
					spin_unlock_bh(&dest_addr_timer_lock);
				}
			} else {
				spin_unlock_bh(&dest_addr_timer_lock);
			}
		}
	}
	return 0;
}

/*timer's expired process function.*/
static void chr_netlink_timer(unsigned long data)
{
	if (g_chr_timer_state == CHR_TIMER_60) {
		/*
		 * If timer state is CHR_TIMER_60,and the timer is expired,
		 * change timer state to CHR_TIMER_3,and change timer's expire
		 * time to 3 minute.
		 */
		spin_lock_bh(&dest_addr_timer_lock);
		memset(g_chr_netIfInfo_struct, 0, DIFF_SRC_IP_ADDR_MAX *
			sizeof(struct chr_netinterface_info_struct));
		spin_unlock_bh(&dest_addr_timer_lock);

		g_chr_netlink_timer.expires = jiffies + TIMER_3_MINUTES;
		add_timer(&g_chr_netlink_timer);/*restart timer*/
		g_chr_timer_state = CHR_TIMER_3;
	} else if (g_chr_timer_state == CHR_TIMER_3) {
		if (g_chr_netIfInfo_struct[0].dstAddrArrayIndex == DIFF_DST_IP_ADDR_MAX
			|| g_chr_netIfInfo_struct[1].dstAddrArrayIndex == DIFF_DST_IP_ADDR_MAX) {
			/*
			 * If there is 3 different server address if recoeded,
			 * will change timer state to CHR_TIMER_60,
			 * timer's timeout will change to 60 minute.
			 */
			g_chr_netlink_timer.expires = jiffies +
				TIMER_60_MINUTES;
			add_timer(&g_chr_netlink_timer);/*restart timer*/
			g_chr_timer_state = CHR_TIMER_60;
		} else {
			spin_lock_bh(&dest_addr_timer_lock);
			memset(g_chr_netIfInfo_struct, 0, DIFF_SRC_IP_ADDR_MAX *
				sizeof(struct chr_netinterface_info_struct));
			spin_unlock_bh(&dest_addr_timer_lock);

			g_chr_netlink_timer.expires = jiffies + TIMER_3_MINUTES;
			add_timer(&g_chr_netlink_timer);/*restart timer*/
			g_chr_timer_state = CHR_TIMER_3;
		}
	}
}

/*netlink init function.*/
static void chr_netlink_init(void)
{
	struct netlink_kernel_cfg chr_nl_cfg = {
		.input = kernel_chr_receive,
	};

	g_chr_nlfd = netlink_kernel_create(&init_net,
		NETLINK_CHR_EVENT_NL,
		&chr_nl_cfg);
	if (!g_chr_nlfd)
		hwlog_info("%s: chr_netlink_init failed\n", __func__);
	else
		hwlog_info("%s: chr_netlink_init success\n", __func__);
	sema_init(&g_chr_netlink_sync_sema, 0);
	spin_lock_init(&dest_addr_timer_lock);
	spin_lock_init(&chr_send_lock);
	init_timer(&g_chr_netlink_timer);
	g_chr_netlink_timer.data = 0;
	g_chr_netlink_timer.function = chr_netlink_timer;
	g_chr_netlink_timer.expires = jiffies + 180*HZ;
	add_timer(&g_chr_netlink_timer);
	g_chr_timer_state = CHR_TIMER_3;
	memset(g_chr_netIfInfo_struct, 0, DIFF_SRC_IP_ADDR_MAX *
		sizeof(struct chr_netinterface_info_struct));
	g_chr_netlink_task = kthread_run(chr_netlink_thread, NULL,
		"chr_netlink_thread");
}

/*netlink deinit function.*/
static void chr_netlink_deinit(void)
{
	if (g_chr_nlfd && g_chr_nlfd->sk_socket) {
		sock_release(g_chr_nlfd->sk_socket);
		g_chr_nlfd = NULL;
	}

	if (g_chr_netlink_task) {
		kthread_stop(g_chr_netlink_task);
		g_chr_netlink_task = NULL;
	}
}

/*send a message to user space.*/
int chr_notify_event(int event, int pid,
		unsigned int src_addr, struct http_return *prtn)
{
	int ret;
	int size;
	struct sk_buff *skb = NULL;
	struct nlmsghdr *nlh = NULL;
	struct chr_nl_packet_msg *packet = NULL;

	spin_lock_bh(&chr_send_lock);
	if (!pid || !g_chr_nlfd) {
		hwlog_err("%s: cannot notify event, pid = %d\n",
			__func__,
			pid);
		ret = -1;
		goto end;
	}
	size = sizeof(struct chr_nl_packet_msg);
	skb = nlmsg_new(size, GFP_ATOMIC);
	if (!skb) {
		hwlog_info("%s: alloc skb fail\n", __func__);
		ret = -1;
		goto end;
	}
	nlh = nlmsg_put(skb, 0, 0, NETLINK_CHR_KER_MSG, size, 0);
	if (!nlh) {
		kfree_skb(skb);
		skb = NULL;
		ret = -1;
		goto end;
	}
	packet = nlmsg_data(nlh);
	memset(packet, 0, sizeof(struct chr_nl_packet_msg));
	if (event == CHR_SPEED_SLOW_EVENT) {
		packet->chr_event = event;
		packet->src_addr = src_addr;
		memset(&(packet->rtn_stat), 0, sizeof(struct http_return));
	} else if (event == CHR_WEB_STAT_EVENT) {
		packet->chr_event = event;
		packet->src_addr = ZERO;
		memcpy(&(packet->rtn_stat), prtn, RNT_SIZE*sizeof(struct http_return));
	}

	/*skb will be freed in netlink_unicast*/
	ret = netlink_unicast(g_chr_nlfd, skb, pid, MSG_DONTWAIT);
	hwlog_info("%s:data speed is slow! collateral information=0x%x\n", __func__, src_addr);
	goto end;

end:
	spin_unlock_bh(&chr_send_lock);
	return ret;
}

static int __init chr_netlink_module_init(void)
{
	chr_netlink_init();
	g_chr_module_state = CHR_NETLINK_INIT;
	web_chr_init();
	return 0;
}

static void __exit chr_netlink_module_exit(void)
{
	web_chr_exit();
	g_chr_module_state = CHR_NETLINK_EXIT;
	chr_netlink_deinit();
}

module_init(chr_netlink_module_init);
module_exit(chr_netlink_module_exit);
