

#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/mutex.h>
#include <net/sock.h>
#include <net/tcp_crosslayer.h>

#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE

static struct sock *nl_sk;
static DEFINE_MUTEX(aspen_nl_mutex);

static void
nl_do_recv_msg(struct sk_buff *skb)
{
	struct nlmsghdr *nlh = NULL;

	if (!skb) {
		ASPEN_ERR("Invalid parameter: zero pointer reference.(skb)");
		return;
	}

	nlh = (struct nlmsghdr *)skb->data;
	if (NULL == nlh) {
		ASPEN_ERR("nl_do_recv_msg:  nlh = NULL\n");
		return;
	}
	/* pid of sending process in userland */
	aspen_nl_entry.userland_pid = nlh->nlmsg_pid;

	ASPEN_INFO("Aspen netlink received msg from userland.(pid=%d)", aspen_nl_entry.userland_pid);
}

/* Note: This is called from process context only because of mutex_lock */
static void
aspen_nl_recv_from_userland(struct sk_buff *skb)
{
	if (!skb) {
		ASPEN_ERR("Invalid parameter: zero pointer reference.(skb)");
		return;
	}

	ASPEN_INFO("Aspen netlink will receive msg from userland.");
	mutex_lock(&aspen_nl_mutex);
	nl_do_recv_msg(skb);
	mutex_unlock(&aspen_nl_mutex);
}

static int
aspen_nl_send_to_userland(struct tcp_sender_monitor *msg)
{
	struct nlmsghdr *nlh;
	struct sk_buff *skb_out;
	int msg_size;
	int res;

	if (!msg) {
		ASPEN_ERR("Invalid parameter: zero pointer reference.(msg)");
		return -EFAULT;
	}

	msg_size = sizeof(*msg);

	/* Note: this is called from softirq context, must use GFP_ATOMIC. */
	skb_out = nlmsg_new(msg_size, GFP_ATOMIC);

	if (!skb_out) {
		ASPEN_ERR("Failed to alloc skb.");
		return -ENOMEM;
	}

	nlh = nlmsg_put(skb_out, 0, 0, NLMSG_DONE, msg_size, 0);
	/* Unicast */
	NETLINK_CB(skb_out).dst_group = 0;
	memcpy(nlmsg_data(nlh), msg, msg_size);

	/* This function will release the memory space of skb_out. */
	res = nlmsg_unicast(nl_sk, skb_out, aspen_nl_entry.userland_pid);
	if (res < 0) {
		/* The peer in userland is killed */
		if (res == -ECONNREFUSED) {
			/* Reset netlink */
			aspen_nl_entry.userland_pid = 0;
		}
		ASPEN_ERR("An error(%d) occurred while sending msg to "
			  "userland.", res);
	}

	return res;
}

static int __init aspen_netlink_init(void)
{
	/* This is for 3.6 kernels and above. */
	struct netlink_kernel_cfg cfg = {
		.input = aspen_nl_recv_from_userland,
	};

	ASPEN_INFO("MODULE INIT.");

	nl_sk = netlink_kernel_create(&init_net, NETLINK_ASPEN, &cfg);
	if (!nl_sk) {
		ASPEN_ERR("An error occurred while creating netlink socket.");
		return -1;
	}

	/* Set callback function */
	aspen_nl_entry.callback_send_to_userland = aspen_nl_send_to_userland;

	return 0;
}

static void __exit aspen_netlink_exit(void)
{
	ASPEN_INFO("MODULE EXIT.");

	/* Reset callback function */
	aspen_nl_entry.userland_pid = 0;
	aspen_nl_entry.callback_send_to_userland = NULL;

	/* Release netlink socket */
	netlink_kernel_release(nl_sk);
}

module_init(aspen_netlink_init);
module_exit(aspen_netlink_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("z00298956");

#endif

