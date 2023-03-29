

#include <linux/module.h>
#include <linux/init.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/uaccess.h>
#include <linux/netdevice.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/udp.h>
#include <linux/errno.h>
#include <net/udp.h>
#include <linux/kernel.h>	   /* add for log */
#include <linux/ctype.h>		/* add for tolower */
#include <linux/file.h>
#include <linux/rbtree.h>
#include <net/sock.h>

#include <hwnet/ipv4/wifipro_tcp_monitor.h>

#include <linux/spinlock.h>	 /* add for spinlock */
#include <linux/netlink.h>	  /* add for thread */
#include <uapi/linux/netlink.h> /* add for netlink */
#include <linux/kthread.h>	  /* add for thread */
#include <linux/version.h>

#include "dpi_hw_hook.h"

#define REPORTCMD NETLINK_DOWNLOADEVENT_TO_USER

static int g_mark_tag = 0;	  /* the tag to decide whether to mark or not, default off */

/* WangZheRongYao Info */
static uint32_t g_tmgp_uid = 0;
static uint8_t g_tmgp_tp = 0;
static uint32_t g_tmgp_mark = 0;

/* netlink message format */
struct tag_hw_msg2knl {
	struct nlmsghdr hdr;
	int opt;
	char data[1];
};

/* DPI rule format */
typedef struct{
	dmr_match_type_t rule_type;
	union{
		uint8_t match_tp_val;
	}rule_body; /* rule body varies according to rule_type */
	uint32_t mark_num;
}dpi_rule_t;

/* dpi_mark_rule for one APP */
typedef struct{
	uid_t			   dmr_app_uid;
	uint32_t		   dmr_mplk_netid;
	uint32_t		   dmr_mplk_strategy;
	dpi_rule_t		  dmr_rule;
}dpi_mark_rule_t;

static struct sock *g_hw_nlfd;
static unsigned int g_uspa_pid;


#define MPLK_MAX_UID_SOCK_ENTRY_NUM (1024)
#define MPLK_NW_BIND_UID_MIN_LIMIT (10000)

static struct rb_root uid_netid_tree = RB_ROOT;
static int uid_netid_entry_num;
static rwlock_t uid_netid_tree_lock;

struct uid_netid {
	struct rb_node uid_netid_node;
	uint32_t netid;
	uid_t uid;
	unsigned long bind_stamp;
	u32 strategy;
};

static struct uid_netid *mplk_uid_netid_tree_search(struct rb_root *root, const uid_t uid)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct uid_netid *data = rb_entry(node, struct uid_netid,
						 uid_netid_node);
		if (uid < data->uid)
			node = node->rb_left;
		else if (uid > data->uid)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static void mplk_uid_netid_tree_insert(struct uid_netid *data, struct rb_root *root)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct uid_netid *this = rb_entry(*new, struct uid_netid, uid_netid_node);
		parent = *new;
		if (data->uid < this->uid)
			new = &((*new)->rb_left);
		else if (data->uid > this->uid)
			new = &((*new)->rb_right);
		else {
			WIFIPRO_WARNING("BUG: dU:%d==tU:%d\n", data->uid, this->uid);
			return;
		}
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&data->uid_netid_node, parent, new);
	rb_insert_color(&data->uid_netid_node, root);
	WIFIPRO_VERBOSE("inert a new uid_netid for %u\n", data->uid);
}

static struct uid_netid *mplk_get_uid_netid_nl(const uid_t uid)
{
	return mplk_uid_netid_tree_search(&uid_netid_tree, uid);
}

static void mplk_add_nw_bind(uid_t uid, uint32_t netid)
{
	struct uid_netid *uid_netid_entry;

	if (uid < MPLK_NW_BIND_UID_MIN_LIMIT) {
		return;
	}

	write_lock_bh(&uid_netid_tree_lock);

	if (uid_netid_entry_num > MPLK_MAX_UID_SOCK_ENTRY_NUM) {
		WIFIPRO_WARNING("max entry reached\n");
		write_unlock_bh(&uid_netid_tree_lock);
		return;
	}

	uid_netid_entry = mplk_get_uid_netid_nl(uid);
	if (!uid_netid_entry) {
		//create a new uid_netid_entry.
		uid_netid_entry = kzalloc(sizeof(*uid_netid_entry), GFP_ATOMIC);
		if (!uid_netid_entry) {
			WIFIPRO_WARNING("alloc uid_netid_entry fail\n");
			write_unlock_bh(&uid_netid_tree_lock);
			return;
		}
		uid_netid_entry_num++;
		mplk_uid_netid_tree_insert(uid_netid_entry, &uid_netid_tree);
	}
	if (uid_netid_entry->netid) {
		WIFIPRO_INFO("change %u from %u to %u then num=%d\n",
			uid, uid_netid_entry->netid, netid, uid_netid_entry_num);
	} else {
		WIFIPRO_INFO("add a new bind for %u to %u then num=%u\n",
			uid, netid, uid_netid_entry_num);
	}
	uid_netid_entry->uid = uid;
	uid_netid_entry->netid = netid;

	//update them at the time of close sk.
	uid_netid_entry->strategy = 0;
	uid_netid_entry->bind_stamp = 0;

	write_unlock_bh(&uid_netid_tree_lock);
}

static void mplk_del_nw_bind(uid_t uid)
{
	struct uid_netid *uid_netid_entry;

	if (uid < MPLK_NW_BIND_UID_MIN_LIMIT) {
		return;
	}

	write_lock_bh(&uid_netid_tree_lock);

	uid_netid_entry = mplk_get_uid_netid_nl(uid);
	if (!uid_netid_entry) {
		WIFIPRO_DEBUG("%u not found\n", uid);
		write_unlock_bh(&uid_netid_tree_lock);
		return;
	}
	rb_erase(&uid_netid_entry->uid_netid_node, &uid_netid_tree);
	kfree(uid_netid_entry);
	uid_netid_entry_num--;
	WIFIPRO_INFO("unbind %u num:%d\n", uid, uid_netid_entry_num);

	write_unlock_bh(&uid_netid_tree_lock);
}

#define MPLK_SK_STRATEGY_CT		(1<<0)
#define MPLK_SK_STRATEGY_FUTE	(1<<1)
#define MPLK_SK_STRATEGY_FURE	(1<<2)

/**
* Close all sockets of uid.
*
* 1,The low 4 bits of strategy, respectively, indicate: FURE-FUTE-CT.
*   As follows:
*   CT   :close TCP socket by Netd.    ( Value in low 4 bit of strategy is 0001, 0x1)
*   FUTE :fake UDP socket TX error.    ( Value in low 4 bit of strategy is 0010, 0x2)
*   FURE :fake UDP socket RX error.    ( Value in low 4 bit of strategy is 0100, 0x4)
*/
static void mplk_close_socket_by_uid(uint32_t strategy, uid_t uid)
{
	struct uid_netid *uid_netid_entry;

	if (uid < MPLK_NW_BIND_UID_MIN_LIMIT)
		return;

	//close current uid's udp sock by strategy.
	write_lock_bh(&uid_netid_tree_lock);

	uid_netid_entry = mplk_get_uid_netid_nl(uid);
	if (!uid_netid_entry) {
		WIFIPRO_WARNING("%u not found\n", uid);
		write_unlock_bh(&uid_netid_tree_lock);
		return;
	}
	uid_netid_entry->strategy = strategy;
	uid_netid_entry->bind_stamp = jiffies;
	WIFIPRO_INFO("close old udp sk for %u when %lu with S:%u\n",
		uid, uid_netid_entry->bind_stamp, strategy);

	write_unlock_bh(&uid_netid_tree_lock);
}

static int mplk_interrupt_data_transfer(struct sock *sk, int strategy)
{
	uid_t uid;
	struct uid_netid *uid_netid_entry;
	int err = 0;

	if (!sk)
		return 0;

	read_lock_bh(&uid_netid_tree_lock);

	uid = sk->sk_uid.val;
	uid_netid_entry = mplk_get_uid_netid_nl(uid);
	if (uid_netid_entry && time_before(sk->sk_born_stamp, uid_netid_entry->bind_stamp)) {
		if (uid_netid_entry->strategy & strategy) {
			err = -EPIPE;
			sk->sk_born_stamp = jiffies;
			WIFIPRO_DEBUG("ret %d for %u bind to %u by %d\n", err, uid, uid_netid_entry->netid, strategy);
		}
	}

	read_unlock_bh(&uid_netid_tree_lock);
	return err;
}

void mplk_try_nw_bind_for_udp(struct sock *sk)
{
	struct uid_netid *uid_netid_entry;
	uid_t uid;

	if (!sk)
		return;
	if (sk->sk_protocol != IPPROTO_UDP && sk->sk_protocol != IPPROTO_UDPLITE)
		return;

	uid = sk->sk_uid.val;
	if (uid < MPLK_NW_BIND_UID_MIN_LIMIT)
		return;

	read_lock_bh(&uid_netid_tree_lock);
	uid_netid_entry = mplk_get_uid_netid_nl(uid);
	if (uid_netid_entry) {
		sk->sk_mark = uid_netid_entry->netid;
		WIFIPRO_DEBUG("%u bound to %u", uid, uid_netid_entry->netid);
	}
	read_unlock_bh(&uid_netid_tree_lock);
}
EXPORT_SYMBOL(mplk_try_nw_bind_for_udp);

int mplk_sendmsg(struct sock *sk)
{
	return mplk_interrupt_data_transfer(sk, MPLK_SK_STRATEGY_FUTE);
}
EXPORT_SYMBOL(mplk_sendmsg);

int mplk_recvmsg(struct sock *sk)
{
	return mplk_interrupt_data_transfer(sk, MPLK_SK_STRATEGY_FURE);
}
EXPORT_SYMBOL(mplk_recvmsg);

/* Add dpi rules to the hash list */
void add_dpi_rule(const char *data){
	dpi_mark_rule_t *p_dmr = (dpi_mark_rule_t *)data;

	/* Only support transportion protocol dpi for wangzherongyao till 2017/06/24,
	   TODO: add other dpi rules */
	switch(p_dmr->dmr_rule.rule_type){
		case DMR_MT_TP:
			g_tmgp_tp = p_dmr->dmr_rule.rule_body.match_tp_val;
			g_tmgp_uid = p_dmr->dmr_app_uid;
			g_tmgp_mark = p_dmr->dmr_rule.mark_num;
			break;
		default:
			break;
	}
}

/* process the cmd, opt not used currently */
static void _proc_cmd(int cmd, int opt, const char *data)
{
	dpi_mark_rule_t *p_dmr = (dpi_mark_rule_t *)data;

	switch(cmd){
		case NETLINK_SET_RULE_TO_KERNEL:
			add_dpi_rule(data);
			break;
		case NETLINK_START_MARK:
			g_mark_tag = 1;
			break;
		case NETLINK_STOP_MARK:
			g_mark_tag = 0;
			break;
		case NETLINK_MPLK_BIND_NETWORK:
			mplk_add_nw_bind(p_dmr->dmr_app_uid, p_dmr->dmr_mplk_netid);
			break;
		case NETLINK_MPLK_UNBIND_NETWORK:
			mplk_del_nw_bind(p_dmr->dmr_app_uid);
			break;
		case NETLINK_MPLK_RESET_SOCKET:
			pr_info("mplk not support reset command now\n");
			break;
		case NETLINK_MPLK_CLOSE_SOCKET:
			mplk_close_socket_by_uid(p_dmr->dmr_mplk_strategy, p_dmr->dmr_app_uid);
			break;
		default:
			pr_info("hwdpi:kernel_hw_receive cmd=%d is wrong\n", cmd);	
	}
}

/* receive cmd for DPI netlink message */
static void kernel_ntl_receive(struct sk_buff *__skb)
{
	struct nlmsghdr *nlh;
	struct tag_hw_msg2knl *hmsg = NULL;
	struct sk_buff *skb;

	if (__skb == NULL){
		return;
	}

	skb = skb_get(__skb);
	if (skb && (skb->len >= NLMSG_HDRLEN)) {
		nlh = nlmsg_hdr(skb);
		if ((nlh->nlmsg_len >= sizeof(struct nlmsghdr)) &&
				(skb->len >= nlh->nlmsg_len)) {
			if (NETLINK_REG_TO_KERNEL == nlh->nlmsg_type)
				g_uspa_pid = nlh->nlmsg_pid;
			else if (NETLINK_UNREG_TO_KERNEL == nlh->nlmsg_type)
				g_uspa_pid = 0;
			else {
				hmsg = (struct tag_hw_msg2knl *)nlh;
				_proc_cmd(nlh->nlmsg_type, hmsg->opt, (char *)&(hmsg->data[0]));
			}
		}
	}
}

/* Initialize netlink socket, add hook function for netlink message receiving */
static void netlink_init(void)
{
	struct netlink_kernel_cfg hwcfg = {
		.input = kernel_ntl_receive,
	};
	g_hw_nlfd = netlink_kernel_create(&init_net, NETLINK_HW_DPI, &hwcfg);
	if (!g_hw_nlfd)
		pr_info("netlink_init failed NETLINK_HW_DPI\n");
}

/* mark the skb if it matched the rules */
void mark_skb_by_rule(struct sk_buff *skb, int tag){
	struct sock *sk = skb->sk;
	uid_t skb_uid;
	kuid_t kuid;
	struct iphdr *iph = ip_hdr(skb);

	/* tmgp_uid is not set */
	if (g_tmgp_uid == 0){
		return;
	}

	if (sk == NULL){
		return;
	}

    if( tag == 0 ){
        sk->sk_hwdpi_mark = sk->sk_hwdpi_mark & MR_RESET;
    }else{
        /* The socket is already marked, originally sk_hwdpi_mark is 0 */
        if (sk->sk_hwdpi_mark & MR_TMGP_2){
            skb->mark = g_tmgp_mark;
            return;
        }

        if ( sk->sk_hwdpi_mark & MR_MARKED ){
            return;
        }

        /* Only support qq tmgp tp identifying, TODO: other DPI rules */
        if ( iph && iph->protocol == g_tmgp_tp){
            kuid = sock_i_uid(sk);
            skb_uid = kuid.val;

            if ( skb_uid == g_tmgp_uid ){
                sk->sk_hwdpi_mark = sk->sk_hwdpi_mark | MR_TMGP_2;
                skb->mark = g_tmgp_mark;
                return;
            }
        }

        sk->sk_hwdpi_mark = sk->sk_hwdpi_mark | MR_MARKED; /* This socket is not tmgp gaming udp */
    }
}

static unsigned int dpimark_hook_localout(const struct nf_hook_ops *ops,
					 struct sk_buff *skb,
					 const struct nf_hook_state *state)
{
	/* match the packet for optimization */
    if (skb){
         mark_skb_by_rule(skb,g_mark_tag);
    }
	return NF_ACCEPT;
}

static struct nf_hook_ops net_hooks[] = {
	{
		.hook		= dpimark_hook_localout,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
		.owner		  = THIS_MODULE,
#endif
		.pf		= PF_INET,
		.hooknum	= NF_INET_LOCAL_OUT,
		.priority	= NF_IP_PRI_FILTER - 1,
	},
};

static int __init nf_init(void)
{
	int ret = 0;

	/* Add a hook point on NF_INET_LOCAL_OUT, where we can get all the packets generated by local APP */
	ret = nf_register_hooks(net_hooks, ARRAY_SIZE(net_hooks));
	if (ret) {
		pr_info("nf_init ret=%d  ", ret);
		return -1;
	}

	/* Initialize the netlink connection */
	netlink_init();

	rwlock_init(&uid_netid_tree_lock);

	pr_info("dpi_hw_hook_init success\n");

	return 0;
}

static void __exit nf_exit(void)
{
	nf_unregister_hooks(net_hooks, ARRAY_SIZE(net_hooks));
}

module_init(nf_init);
module_exit(nf_exit);

MODULE_LICENSE("Dual BSD");
MODULE_AUTHOR("s00399850");
MODULE_DESCRIPTION("HW DPI MARK NF_HOOK");
MODULE_VERSION("1.0.1");
MODULE_ALIAS("HW LWDPI 01");
