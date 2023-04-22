#define DEBUG
#define DDEBUG

#include <linux/file.h>
#include <linux/inetdevice.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/netfilter/x_tables.h>
#include <linux/netfilter/xt_qtaguid.h>
#include <linux/ratelimit.h>
#include <linux/seq_file.h>
#include <linux/skbuff.h>
#include <linux/workqueue.h>
#include <net/addrconf.h>
#include <net/sock.h>
#include <net/tcp.h>
#include <net/udp.h>
#include <linux/version.h>

#if defined(CONFIG_IP6_NF_IPTABLES) || defined(CONFIG_IP6_NF_IPTABLES_MODULE)
#include <linux/netfilter_ipv6/ip6_tables.h>
#endif

#include <linux/netfilter/xt_socket.h>
#include "../../fs/proc/internal.h"

#include "qtaguid_pid_internal.h"
#include "qtaguid_pid.h"

#ifdef CONFIG_HUAWEI_DUBAI
#include <log/log_usertype/log-usertype.h>
#include <huawei_platform/log/hwlog_kernel.h>
extern unsigned long get_wakeuptime(void);
extern const char *get_sourcename(void);
extern int get_gpio(void);
unsigned long g_latt_wakeuptime_tmp = 0;
#endif
static LIST_HEAD(iface_pid_stat_list);
static DEFINE_SPINLOCK(iface_pid_stat_list_lock);

/* find process name by sock pointer */
static struct rb_root sock_pid_tree = RB_ROOT;
/* find process name by skb hash */
static struct rb_root hash_pid_tree = RB_ROOT;
/* lock sock_pid_tree & hash_pid_tree */
static DEFINE_SPINLOCK(pid_trees_lock);

static inline void dc_add_byte_packets(struct data_counters *counters, int set,
				  enum ifs_tx_rx direction,
				  enum ifs_proto ifs_proto,
				  int bytes,
				  int packets)
{
	counters->bpc[set][direction][ifs_proto].bytes += bytes;
	counters->bpc[set][direction][ifs_proto].packets += packets;
}

static struct pid_node *pid_node_tree_search(struct rb_root *root, tag_t tag)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct pid_node *data = rb_entry(node, struct pid_node, node);
		int result;

		RB_DEBUG("qtup: pid_node_tree_search(%llu): "
			 " node=%p data=%p\n", tag, node, data);
		result = tag_compare(tag, data->tag);
		RB_DEBUG("qtup: pid_node_tree_search(0x%llx): "
			 " data.tag=0x%llx (uid=%u) res=%d\n",
			 tag, data->tag, get_uid_from_tag(data->tag), result);
		if (result < 0)
			node = node->rb_left;
		else if (result > 0)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static void pid_node_tree_insert(struct pid_node *data, struct rb_root *root)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct pid_node *this = rb_entry(*new, struct pid_node,
						 node);
		int result = tag_compare(data->tag, this->tag);

		RB_DEBUG("qtup: %s(): tag=0x%llx"
			 " (uid=%u)\n", __func__,
			 this->tag,
			 get_uid_from_tag(this->tag));
		parent = *new;
		if (result < 0)
			new = &((*new)->rb_left);
		else if (result > 0)
			new = &((*new)->rb_right);
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&data->node, parent, new);
	rb_insert_color(&data->node, root);
}

static void pid_stat_tree_insert(struct pid_stat *data, struct rb_root *root)
{
	pid_node_tree_insert(&data->pn, root);
}

static struct pid_stat *pid_stat_tree_search(struct rb_root *root, tag_t tag)
{
	struct pid_node *node = pid_node_tree_search(root, tag);

	if (!node)
		return NULL;
	return rb_entry(&node->node, struct pid_stat, pn.node);
}

static struct sock_pid *sock_pid_tree_search(struct rb_root *root,
					     const struct sock *sk)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct sock_pid *data = rb_entry(node, struct sock_pid,
						 sock_node);
		if (sk < data->sk)
			node = node->rb_left;
		else if (sk > data->sk)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static struct hash_pid *hash_pid_tree_search(struct rb_root *root,
					     unsigned int hash)
{
	struct rb_node *node = root->rb_node;

	while (node) {
		struct hash_pid *data = rb_entry(node, struct hash_pid,
						 hash_node);
		if (hash < data->skb_hash)
			node = node->rb_left;
		else if (hash > data->skb_hash)
			node = node->rb_right;
		else
			return data;
	}
	return NULL;
}

static void sock_pid_tree_insert(struct sock_pid *data, struct rb_root *root)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct sock_pid *this = rb_entry(*new, struct sock_pid,
						 sock_node);
		parent = *new;
		if (data->sk < this->sk)
			new = &((*new)->rb_left);
		else if (data->sk > this->sk)
			new = &((*new)->rb_right);
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&data->sock_node, parent, new);
	rb_insert_color(&data->sock_node, root);
}

static void hash_pid_tree_insert(struct hash_pid *data, struct rb_root *root)
{
	struct rb_node **new = &(root->rb_node), *parent = NULL;

	/* Figure out where to put new node */
	while (*new) {
		struct hash_pid *this = rb_entry(*new, struct hash_pid,
						 hash_node);
		parent = *new;
		if (data->skb_hash < this->skb_hash)
			new = &((*new)->rb_left);
		else if (data->skb_hash > this->skb_hash)
			new = &((*new)->rb_right);
	}

	/* Add new node and rebalance tree. */
	rb_link_node(&data->hash_node, parent, new);
	rb_insert_color(&data->hash_node, root);
}

static struct iface_pid_stat *get_iface_pid_entry(const char *ifname)
{
	struct iface_pid_stat *iface_entry;

	/* Find the entry for tracking the specified tag within the interface */
	if (ifname == NULL) {
		MT_DEBUG("qtup: iface_pid_stat: get() NULL device name\n");
		return NULL;
	}

	/* Iterate over interfaces */
	list_for_each_entry(iface_entry, &iface_pid_stat_list, list) {
		if (!strcmp(ifname, iface_entry->ifname))
			goto done;
	}
	iface_entry = NULL;
done:
	return iface_entry;
}

/* Caller must hold iface_stat_list_lock */
static struct iface_pid_stat *iface_pid_alloc(struct net_device *net_dev)
{
	struct iface_pid_stat *new_iface;
	if (NULL == net_dev)
		return NULL;

	new_iface = kzalloc(sizeof(*new_iface), GFP_ATOMIC);
	if (NULL == new_iface)
		return NULL;

	new_iface->ifname = kstrdup(net_dev->name, GFP_ATOMIC);
	if (new_iface->ifname == NULL) {
		kfree(new_iface);
		return NULL;
	}
	spin_lock_init(&new_iface->pid_stat_list_lock);
	new_iface->pid_stat_tree = RB_ROOT;

	list_add(&new_iface->list, &iface_pid_stat_list);
	return new_iface;
}

void iface_pid_stat_create(struct net_device *net_dev,
			      struct in_ifaddr *ifa)
{
	struct in_device *in_dev = NULL;
	const char *ifname;
	struct iface_pid_stat *entry;
	__be32 ipaddr = 0;
	struct iface_pid_stat *new_iface;

	IF_DEBUG("qtup: iface_pid_stat: create(%s): ifa=%p netdev=%p\n",
		 net_dev ? net_dev->name : "?",
		 ifa, net_dev);
	if (!net_dev) {
		IF_DEBUG("qtup: iface_pid_stat: create(): no net dev\n");
		return;
	}

	ifname = net_dev->name;
	if (!ifa) {
		in_dev = in_dev_get(net_dev);
		if (!in_dev) {
			IF_DEBUG("qtup: iface_pid_stat: create(%s): no indev\n",
			       ifname);
			return;
		}
		IF_DEBUG("qtup: iface_pid_stat: create(%s): in_dev=%p\n",
			 ifname, in_dev);
		for (ifa = in_dev->ifa_list; ifa; ifa = ifa->ifa_next) {
			IF_DEBUG("qtup: iface_pid_stat: create(%s): "
				 "ifa=%p ifa_label=%s\n",
				 ifname, ifa,
				 ifa->ifa_label);
			if ( !strcmp(ifname, ifa->ifa_label))
				break;
		}
	}

	if (!ifa) {
		IF_DEBUG("qtup: iface_pid_stat: create(%s): no matching IP\n",
			 ifname);
		goto done_put;
	}
	ipaddr = ifa->ifa_local;

	spin_lock_bh(&iface_pid_stat_list_lock);
	entry = get_iface_pid_entry(ifname);
	if (entry != NULL) {
		IF_DEBUG("qtup: iface_pid_stat: create(%s): entry=%p\n",
			 ifname, entry);

		IF_DEBUG("qtup: %s(%s): "
			 "tracking now %d on ip=%pI4\n", __func__,
			 entry->ifname, true, &ipaddr);
		goto done_unlock_put;
	}

	new_iface = iface_pid_alloc(net_dev);
	IF_DEBUG("qtup: iface_pid_stat: create(%s): done "
		 "entry=%p ip=%pI4\n", ifname, new_iface, &ipaddr);
done_unlock_put:
	spin_unlock_bh(&iface_pid_stat_list_lock);
done_put:
	if (in_dev)
		in_dev_put(in_dev);
}
EXPORT_SYMBOL(iface_pid_stat_create);

void iface_pid_stat_create_ipv6(struct net_device *net_dev,
				   struct inet6_ifaddr *ifa)
{
	struct in_device *in_dev;
	const char *ifname;
	struct iface_pid_stat *entry;
	struct iface_pid_stat *new_iface;

	IF_DEBUG("qtup: iface_pid_stat: create6(): ifa=%p netdev=%p->name=%s\n",
		 ifa, net_dev, net_dev ? net_dev->name : "");
	if (!net_dev) {
		IF_DEBUG("qtup: iface_pid_stat: create6(): no net dev!\n");
		return;
	}
	ifname = net_dev->name;

	in_dev = in_dev_get(net_dev);
	if (!in_dev) {
		IF_DEBUG("qtup: iface_pid_stat: create6(%s): no inet dev\n",
		       ifname);
		return;
	}

	IF_DEBUG("qtup: iface_pid_stat: create6(%s): in_dev=%p\n",
		 ifname, in_dev);

	if (!ifa) {
		IF_DEBUG("qtup: iface_pid_stat: create6(%s): no matching IP\n",
			 ifname);
		goto done_put;
	}

	spin_lock_bh(&iface_pid_stat_list_lock);
	entry = get_iface_pid_entry(ifname);
	if (entry != NULL) {
		IF_DEBUG("qtup: %s(%s): entry=%p\n", __func__,
			 ifname, entry);

		IF_DEBUG("qtup: %s(%s): "
			 "tracking now %d on ip=%pI6c\n", __func__,
			 entry->ifname, true, &ifa->addr);
		goto done_unlock_put;
	}

	new_iface = iface_pid_alloc(net_dev);
	IF_DEBUG("qtup: iface_pid_stat: create6(%s): done "
		 "entry=%p ip=%pI6c\n", ifname, new_iface, &ifa->addr);

done_unlock_put:
	spin_unlock_bh(&iface_pid_stat_list_lock);
done_put:
	in_dev_put(in_dev);
}
EXPORT_SYMBOL(iface_pid_stat_create_ipv6);

static struct sock_pid *get_sock_pid_nl(const struct sock *sk)
{
	MT_DEBUG("qtup: get_sock_pid_nl(sk=%p)\n", sk);
	return sock_pid_tree_search(&sock_pid_tree, sk);
}

static struct hash_pid *get_hash_pid_nl(unsigned int hash)
{
	MT_DEBUG("qtup: get_hash_pid_nl(hash=%u)\n", hash);
	return hash_pid_tree_search(&hash_pid_tree, hash);
}

static void
data_counters_update(struct data_counters *dc, int set,
		     enum ifs_tx_rx direction, int proto, int bytes)
{
	switch (proto) {
	case IPPROTO_TCP:
		dc_add_byte_packets(dc, set, direction, IFS_TCP, bytes, 1);
		break;
	case IPPROTO_UDP:
		dc_add_byte_packets(dc, set, direction, IFS_UDP, bytes, 1);
		break;
	case IPPROTO_IP:
	default:
		dc_add_byte_packets(dc, set, direction, IFS_PROTO_OTHER, bytes,
				    1);
		break;
	}
}

static struct pid_stat *create_if_pid_stat(struct iface_pid_stat *iface_entry,
					 char *comm, uid_t real_uid, tag_t tag)
{
	struct pid_stat *new_pid_stat_entry = NULL;

	new_pid_stat_entry = kzalloc(sizeof(*new_pid_stat_entry), GFP_ATOMIC);
	if (!new_pid_stat_entry)
		goto done;

	strlcpy(new_pid_stat_entry->pn.comm, comm, TASK_COMM_LEN);
	new_pid_stat_entry->pn.uid = real_uid;
	new_pid_stat_entry->pn.tag = tag;
	pid_stat_tree_insert(new_pid_stat_entry, &iface_entry->pid_stat_tree);
done:
	return new_pid_stat_entry;
}

static void pid_stat_update(struct pid_stat *pid_entry, int active_set,
			    enum ifs_tx_rx direction, int proto, int bytes)
{
	MT_DEBUG("qtup: pid_stat_update(pid=%s "
		 "dir=%d proto=%d bytes=%d)\n",
		 pid_entry->pn.comm,
		 direction, proto, bytes);

	data_counters_update(&pid_entry->counters, active_set, direction,
			     proto, bytes);
}

static unsigned int skb_v4_hash(const struct sk_buff *skb,
				     enum ifs_tx_rx direction)
{
	unsigned int skb_hash = 0;
	const struct iphdr *iph;
	struct udphdr _hdr, *hp = 0;

	if (!skb) {
		MT_DEBUG("qtup: skb_v4_hash: skb is empty\n");
		return 0;
	}

	iph = ip_hdr(skb);
	if (!iph) {
		MT_DEBUG("qtup: skb_v4_hash: no ip hdr\n");
		return 0;
	}

	if (iph->protocol == IPPROTO_UDP || iph->protocol == IPPROTO_TCP)
		hp = skb_header_pointer(skb, ip_hdrlen(skb),
					sizeof(_hdr), &_hdr);

	if (!hp) {
		MT_DEBUG("qtup: skb_v4_hash: %pI4 > %pI4 id %u not tcp udp\n",
			&(iph->saddr), &(iph->daddr),
			ntohs(iph->id));
		return 0;
	}

	if (IFS_TX == direction) {
		skb_hash = jhash_3words(
			(uint32_t)iph->saddr, (uint32_t)iph->daddr,
			((uint32_t)hp->source) << 16 | (uint32_t)hp->dest, 0);
	} else {
		skb_hash = jhash_3words(
			(uint32_t)iph->daddr, (uint32_t)iph->saddr,
			((uint32_t)hp->dest) << 16 | (uint32_t)hp->source, 0);
	}

	MT_DEBUG("qtup: skb_v4_hash: %pI4 > %pI4 %u > %u id %u hash %x\n",
		&(iph->saddr), &(iph->daddr),
		ntohs(hp->source),
		ntohs(hp->dest),
		ntohs(iph->id),
		skb_hash);

	return skb_hash;
}

static int sock_pid_entry_num;
static int hash_pid_entry_num;

static char *get_pr_name(char *buf)
{
	return get_task_comm(buf, current->group_leader);
}

static uid_t get_current_uid(void)
{
	return from_kuid(&init_user_ns, current_fsuid());
}

void qtaguid_pid_put(const struct sock *sk)
{
	struct sock_pid *sock_pid_entry;
	char task_comm[TASK_COMM_LEN] = {0};

	if (sock_pid_entry_num > MAX_SOCK_PID_ENTRY_NUM) {
		CT_DEBUG("qtup: qtaguid_pid_put: max entry reached\n");
		return;
	}

	get_pr_name(task_comm);

	spin_lock_bh(&pid_trees_lock);
	sock_pid_entry = get_sock_pid_nl(sk);

	if (sock_pid_entry) {
		strncpy(sock_pid_entry->comm, task_comm, TASK_COMM_LEN);
		sock_pid_entry->comm_hash = jhash(sock_pid_entry->comm,
			strnlen(sock_pid_entry->comm, TASK_COMM_LEN), 0);
		sock_pid_entry->uid = get_current_uid();
		spin_unlock_bh(&pid_trees_lock);
		return;
	}

	sock_pid_entry = kzalloc(sizeof(*sock_pid_entry), GFP_ATOMIC);
	if (!sock_pid_entry) {
		spin_unlock_bh(&pid_trees_lock);
		return;
	}

	sock_pid_entry->sk = sk;
	strlcpy(sock_pid_entry->comm, task_comm, TASK_COMM_LEN);
	sock_pid_entry->comm_hash = jhash(sock_pid_entry->comm,
			strnlen(sock_pid_entry->comm, TASK_COMM_LEN), 0);
	sock_pid_entry->uid = get_current_uid();
	sock_pid_tree_insert(sock_pid_entry, &sock_pid_tree);

	CT_DEBUG("qtup: qtaguid_pid_put sock_num=%d add sk=%p pid=\"%s\"\n",
		sock_pid_entry_num, sk, sock_pid_entry->comm);

	sock_pid_entry_num++;

	spin_unlock_bh(&pid_trees_lock);
}
EXPORT_SYMBOL(qtaguid_pid_put);

void qtaguid_pid_remove(const struct sock *sk)
{
	struct sock_pid *sock_pid_entry;
	struct hash_pid *hash_pid_entry;

	spin_lock_bh(&pid_trees_lock);
	sock_pid_entry = get_sock_pid_nl(sk);

	if (!sock_pid_entry) {
		CT_DEBUG("qtup: qtaguid_pid_remove sock_num=%d del sk=%p fail "
			"sock_pid_entry not found\n", sock_pid_entry_num, sk);
		spin_unlock_bh(&pid_trees_lock);
		return;
	}

	CT_DEBUG("qtup: qtaguid_pid_remove sock_num=%d del sk=%p pid=\"%s\"\n",
		sock_pid_entry_num, sk, sock_pid_entry->comm);
	rb_erase(&sock_pid_entry->sock_node, &sock_pid_tree);
	sock_pid_entry_num--;

	hash_pid_entry = sock_pid_entry->hash_pid_ref;
	kfree(sock_pid_entry);
	if (hash_pid_entry)
		mod_timer(&hash_pid_entry->timer,
			  jiffies + HASH_ENTRY_EXPIRE_TIMEOUT * HZ);
	spin_unlock_bh(&pid_trees_lock);
}
EXPORT_SYMBOL(qtaguid_pid_remove);

static void hash_pid_timer_expire(unsigned long data)
{
	struct hash_pid *hash_pid_entry = (struct hash_pid *)data;

	spin_lock_bh(&pid_trees_lock);
	MT_DEBUG("qtup: hash_pid_timer_expire: hash_num=%u "
	    "del hash=%u pid=\"%s\"\n",
	    hash_pid_entry_num, hash_pid_entry->skb_hash, hash_pid_entry->comm);

	rb_erase(&hash_pid_entry->hash_node, &hash_pid_tree);
	spin_unlock_bh(&pid_trees_lock);
	kfree(hash_pid_entry);
	hash_pid_entry_num--;
}

/*
 * Create a hash_pid entry for given sock_pid entry.
 * Caller must hold pid_trees_lock
 */
static void create_hash_pid_entry(struct sock_pid *sock_pid_entry,
			const struct sk_buff *skb, unsigned int family,
			enum ifs_tx_rx direction)
{
	struct hash_pid *hash_pid_entry;
	unsigned int skb_hash = 0;

	if (hash_pid_entry_num > MAX_HASH_PID_ENTRY_NUM) {
		MT_DEBUG("qtup: create_hash_pid_entry: max entry reached\n");
		return;
	}

	if (sock_pid_entry->hash_pid_ref)
		return;

	if (family == NFPROTO_IPV4)
		skb_hash = skb_v4_hash(skb, direction);

	MT_DEBUG("qtup: create_hash_pid_entry: hash_num=%u "
		"add hash=%x pid=\"%s\"\n",
		hash_pid_entry_num, skb_hash, sock_pid_entry->comm);

	if (!skb_hash)
		return;

	if (get_hash_pid_nl(skb_hash)) {
		MT_DEBUG("qtup: create_hash_pid_entry: hash %u exist\n",
			skb_hash);
		return;
	}

	hash_pid_entry = kzalloc(sizeof(*hash_pid_entry), GFP_ATOMIC);
	if (!hash_pid_entry)
		return;

	hash_pid_entry->skb_hash = skb_hash;
	strlcpy(hash_pid_entry->comm, sock_pid_entry->comm, TASK_COMM_LEN);
	hash_pid_entry->comm_hash = sock_pid_entry->comm_hash;
	hash_pid_entry->uid = sock_pid_entry->uid;
	setup_timer(&hash_pid_entry->timer, hash_pid_timer_expire,
		(unsigned long)hash_pid_entry);
	hash_pid_tree_insert(hash_pid_entry, &hash_pid_tree);
	hash_pid_entry_num++;
	sock_pid_entry->hash_pid_ref = hash_pid_entry;
}

void if_pid_stat_update(const char *ifname, uid_t uid,
			const struct sock *sk, const struct sk_buff *skb,
			int set, unsigned int family,
			enum ifs_tx_rx direction, int proto, int bytes)
{
	struct pid_stat *pid_stat_entry;
	char task_comm[TASK_COMM_LEN] = {0};
	uint32_t task_comm_hash = 0;
	uid_t real_uid = 0;
	tag_t tag, pid_tag;
	struct iface_pid_stat *iface_entry = NULL;
	struct sock_pid *sock_pid_entry = NULL;
	struct hash_pid *hash_pid_entry = NULL;
	struct pid_stat *new_pid_stat = NULL;
	unsigned int skb_hash = 0;
	int search_hash_pid = false;

	spin_lock_bh(&iface_pid_stat_list_lock);
	iface_entry = get_iface_pid_entry(ifname);
	if (!iface_entry) {
		MT_DEBUG("qtup: iface_stat: stat_update() "
				   "%s not found\n", ifname);
		spin_unlock_bh(&iface_pid_stat_list_lock);
		return;
	}
	/* It is ok to process data when an iface_entry is inactive */

	MT_DEBUG("qtup: iface_stat: stat_update() dev=%s entry=%p\n",
		 ifname, iface_entry);

	/* find pid by socket */
	spin_lock_bh(&pid_trees_lock);
	sock_pid_entry = get_sock_pid_nl(sk);
	if (sock_pid_entry) {
		MT_DEBUG("qtup: if_pid_stat_update: sock_pid sk %p pid %s\n",
			sock_pid_entry->sk, sock_pid_entry->comm);
		strncpy(task_comm, sock_pid_entry->comm, TASK_COMM_LEN);
		task_comm_hash = sock_pid_entry->comm_hash;
		real_uid = sock_pid_entry->uid;

		if (!strncmp(task_comm, "swapper/0", TASK_COMM_LEN))
			search_hash_pid = true;
		else
			create_hash_pid_entry(sock_pid_entry, skb, family,
					      direction);

	} else {
		snprintf(task_comm, TASK_COMM_LEN, "UID_%u", uid);
		task_comm_hash = jhash(task_comm,
					strnlen(task_comm, TASK_COMM_LEN), 0);
		real_uid = uid;
		search_hash_pid = true;
	}

	/* correct pid "UID_0" and "swapper/0" by hash */
	if (search_hash_pid && (family == NFPROTO_IPV4)) {
		skb_hash = skb_v4_hash(skb, direction);
		if (skb_hash){
			hash_pid_entry = get_hash_pid_nl(skb_hash);
		}
	}

	if (hash_pid_entry) {
		strlcpy(task_comm, hash_pid_entry->comm, TASK_COMM_LEN);
		task_comm_hash = hash_pid_entry->comm_hash;
		real_uid = hash_pid_entry->uid;
		MT_DEBUG("qtup: if_pid_stat_update: hash_pid hash %x pid %s\n",
			hash_pid_entry->skb_hash, hash_pid_entry->comm);
	}
	spin_unlock_bh(&pid_trees_lock);

	pid_tag = make_atag_from_value(task_comm_hash);
	tag = combine_atag_with_uid(pid_tag, uid);

	MT_DEBUG("qtup: if_pid_stat_update(ifname=%s "
		"pid=%s sk=%p dir=%d proto=%d bytes=%d)\n",
		 ifname, task_comm, sk, direction, proto, bytes);

#ifdef CONFIG_HUAWEI_DUBAI
    if (BETA_USER == get_logusertype_flag()) {
        unsigned long last_wakeup_time = 0;

        last_wakeup_time = get_wakeuptime();
        if (last_wakeup_time > 0
            && last_wakeup_time != g_latt_wakeuptime_tmp
            && ((hisi_getcurtime() / 1000000 - last_wakeup_time) < 500)) {
            int32_t protocol = -1, port = -1;

            if (skb) {
                const struct iphdr *iph = ip_hdr(skb);
                if (iph) {
                    protocol = iph->protocol;
                    if (iph->protocol == IPPROTO_TCP || iph->protocol == IPPROTO_UDP) {
                        struct udphdr hdr, *hp = 0;
                        hp = skb_header_pointer(skb, ip_hdrlen(skb), sizeof(hdr), &hdr);
                        if (hp) {
                            port = (int32_t)ntohs(hp->dest);
                        }
                    }
                }
            }
            HWDUBAI_LOGE("DUBAI_TAG_APP_WAKEUP", "uid=%d protocol=%d port=%d procname=%s source=%s gpio=%d",
                real_uid, protocol, port, task_comm, get_sourcename(), get_gpio());
            g_latt_wakeuptime_tmp = last_wakeup_time;
        }
    }
#endif
	spin_lock_bh(&iface_entry->pid_stat_list_lock);

	pid_stat_entry = pid_stat_tree_search(&iface_entry->pid_stat_tree, tag);
	if (pid_stat_entry) {
		/*
		 * Updating the {acct_tag, uid_tag} entry handles both stats:
		 * {0, uid_tag} will also get updated.
		 */
		pid_stat_update(pid_stat_entry, set, direction, proto, bytes);
		goto unlock;
	}

	if (!pid_stat_entry) {
		/* Here: the base uid_tag did not exist */
		/*
		 * No parent counters. So
		 *  - No {0, uid_tag} stats and no {acc_tag, uid_tag} stats.
		 */
		new_pid_stat = create_if_pid_stat(iface_entry, task_comm,
						real_uid, tag);
		if (!new_pid_stat)
			goto unlock;
	}
	pid_stat_update(new_pid_stat, set, direction, proto, bytes);
unlock:
	spin_unlock_bh(&iface_entry->pid_stat_list_lock);
	spin_unlock_bh(&iface_pid_stat_list_lock);
}
EXPORT_SYMBOL(if_pid_stat_update);

struct proc_print_info {
	struct iface_pid_stat *iface_entry;
	int item_index;
	char comm[TASK_COMM_LEN]; /* pid found by reading to pid_pos */
	tag_t tag;
	off_t pid_pos;
	int pid_item_index;
};

static void pp_stats_header(struct seq_file *m)
{
	seq_puts(m,
		 "idx iface pid uid acct_tag_hex uid_tag_int cnt_set "
		 "rx_bytes rx_packets "
		 "tx_bytes tx_packets "
		 "rx_tcp_bytes rx_tcp_packets "
		 "rx_udp_bytes rx_udp_packets "
		 "rx_other_bytes rx_other_packets "
		 "tx_tcp_bytes tx_tcp_packets "
		 "tx_udp_bytes tx_udp_packets "
		 "tx_other_bytes tx_other_packets\n");
}

static int pp_stats_line(struct seq_file *m, struct pid_stat *ps_entry,
			 int cnt_set)
{
	struct proc_print_info *ppi;
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	int ret;
#endif
	struct data_counters *cnts;
	uid_t real_uid;
	uid_t tag_uid;
	char comm[TASK_COMM_LEN]={};
	int i;

	if (NULL == m || NULL == m->private)
		return 0;

	ppi = m->private;
	strlcpy(comm, ps_entry->pn.comm, TASK_COMM_LEN);
	tag_uid = get_uid_from_tag(ps_entry->pn.tag);
	real_uid = ps_entry->pn.uid;

	for (i = 0; i < TASK_COMM_LEN; i++)
		if (' ' == comm[i])
			comm[i] = '"';

	ppi->item_index++;
	cnts = &ps_entry->counters;

	CT_DEBUG("qtup: stats line~~~: "
			 "%s : sufficient priv "
			 "from pid=%s\n",
			 ppi->iface_entry->ifname,
			 ps_entry->pn.comm);

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	ret = seq_printf(m, "%d %s %s %u 0x%llx %u %u "
#else
	seq_printf(m, "%d %s %s %u 0x%llx %u %u "
#endif
		"%llu %llu "
		"%llu %llu "
		"%llu %llu "
		"%llu %llu "
		"%llu %llu "
		"%llu %llu "
		"%llu %llu "
		"%llu %llu\n",
		ppi->item_index,
		ppi->iface_entry->ifname,
		comm,
		real_uid,
		make_atag_from_value(0),
		tag_uid,
		cnt_set,
		dc_sum_bytes(cnts, cnt_set, IFS_RX),
		dc_sum_packets(cnts, cnt_set, IFS_RX),
		dc_sum_bytes(cnts, cnt_set, IFS_TX),
		dc_sum_packets(cnts, cnt_set, IFS_TX),
		cnts->bpc[cnt_set][IFS_RX][IFS_TCP].bytes,
		cnts->bpc[cnt_set][IFS_RX][IFS_TCP].packets,
		cnts->bpc[cnt_set][IFS_RX][IFS_UDP].bytes,
		cnts->bpc[cnt_set][IFS_RX][IFS_UDP].packets,
		cnts->bpc[cnt_set][IFS_RX][IFS_PROTO_OTHER].bytes,
		cnts->bpc[cnt_set][IFS_RX][IFS_PROTO_OTHER].packets,
		cnts->bpc[cnt_set][IFS_TX][IFS_TCP].bytes,
		cnts->bpc[cnt_set][IFS_TX][IFS_TCP].packets,
		cnts->bpc[cnt_set][IFS_TX][IFS_UDP].bytes,
		cnts->bpc[cnt_set][IFS_TX][IFS_UDP].packets,
		cnts->bpc[cnt_set][IFS_TX][IFS_PROTO_OTHER].bytes,
		cnts->bpc[cnt_set][IFS_TX][IFS_PROTO_OTHER].packets);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0))
	return ret ?: 1;
#else
	return seq_has_overflowed(m) ? -ENOSPC : 1;
#endif
}

static bool pp_sets(struct seq_file *m, struct pid_stat *ps_entry)
{
	int ret;
	int counter_set;

	for (counter_set = 0; counter_set < IFS_MAX_COUNTER_SETS;
	     counter_set++) {
		ret = pp_stats_line(m, ps_entry, counter_set);
		if (ret < 0)
			return false;
	}
	return true;
}

static int iface_pid_stat_ptr_valid(struct iface_pid_stat *ptr)
{
	struct iface_pid_stat *iface_entry;

	if (!ptr)
		return false;

	list_for_each_entry(iface_entry, &iface_pid_stat_list, list)
		if (iface_entry == ptr)
			return true;
	return false;
}

static void qtaguid_pid_stats_proc_next_iface_entry(struct proc_print_info *ppi)
{
	spin_unlock_bh(&ppi->iface_entry->pid_stat_list_lock);
	list_for_each_entry_continue(ppi->iface_entry, &iface_pid_stat_list,
					list) {
		spin_lock_bh(&ppi->iface_entry->pid_stat_list_lock);
		return;
	}
	ppi->iface_entry = NULL;
}

static void *qtaguid_pid_stats_proc_next(struct seq_file *m, void *v,
					 loff_t *pos)
{
	struct proc_print_info *ppi;
	struct pid_stat *ps_entry;
	struct rb_node *node;

	if (NULL == m || NULL == m->private)
		return NULL;

	ppi = m->private;

	if (!v) {
		CT_DEBUG("qtup: %s(): unexpected v: NULL\n", __func__);
		return NULL;
	}

	if (NULL == pos)
		return NULL;

	(*pos)++;

	 if (!ppi->iface_entry)
		return NULL;

	if (v == SEQ_START_TOKEN)
		node = rb_first(&ppi->iface_entry->pid_stat_tree);
	else
		node = rb_next(&((struct pid_stat *)v)->pn.node);

	while (!node) {
		qtaguid_pid_stats_proc_next_iface_entry(ppi);
		if (!ppi->iface_entry)
			return NULL;
		node = rb_first(&ppi->iface_entry->pid_stat_tree);
	}

	ps_entry = rb_entry(node, struct pid_stat, pn.node);
	strlcpy(ppi->comm, ps_entry->pn.comm, TASK_COMM_LEN);
	ppi->tag = ps_entry->pn.tag;
	ppi->pid_pos = *pos;
	ppi->pid_item_index = ppi->item_index;
	return ps_entry;
}

static void *qtaguid_pid_stats_proc_start(struct seq_file *m, loff_t *pos)
{
	struct proc_print_info *ppi = m->private;
	struct pid_stat *ps_entry = NULL;

	spin_lock_bh(&iface_pid_stat_list_lock);

	if (*pos == 0) {
		ppi->item_index = 1;
		ppi->pid_pos = 0;
		if (list_empty(&iface_pid_stat_list)) {
			ppi->iface_entry = NULL;
		} else {
			ppi->iface_entry = list_first_entry(
							&iface_pid_stat_list,
							struct iface_pid_stat,
							list);
			spin_lock_bh(&ppi->iface_entry->pid_stat_list_lock);
		}
		return SEQ_START_TOKEN;
	}
	if (!iface_pid_stat_ptr_valid(ppi->iface_entry)) {
		if (ppi->iface_entry) {
			CT_DEBUG("qtup: %s(): iface_entry %p not found\n",
			       __func__, ppi->iface_entry);
			ppi->iface_entry = NULL;
		}
		return NULL;
	}

	spin_lock_bh(&ppi->iface_entry->pid_stat_list_lock);

	if (!ppi->pid_pos) {
		/* seq_read skipped first next call */
		ps_entry = SEQ_START_TOKEN;
	} else {
		ps_entry = pid_stat_tree_search(
				&ppi->iface_entry->pid_stat_tree, ppi->tag);
		if (!ps_entry) {
			CT_DEBUG("qtup: %s(): pid_stat.pid %s not found\n",
				__func__, ppi->comm);
			return NULL;
		}
	}

	if (*pos == ppi->pid_pos) { /* normal resume */
		ppi->item_index = ppi->pid_item_index;
	} else {
		/* seq_read skipped a next call */
		*pos = ppi->pid_pos;
		ps_entry = qtaguid_pid_stats_proc_next(m, ps_entry, pos);
	}

	return ps_entry;
}

static void qtaguid_pid_stats_proc_stop(struct seq_file *m, void *v)
{
	struct proc_print_info *ppi = m->private;

	if (ppi->iface_entry)
		spin_unlock_bh(&ppi->iface_entry->pid_stat_list_lock);
	spin_unlock_bh(&iface_pid_stat_list_lock);
}

/*
 * Procfs reader to get all tag stats using style "1)" as described in
 * fs/proc/generic.c
 * Groups all protocols tx/rx bytes.
 */
static int qtaguid_pid_stats_proc_show(struct seq_file *m, void *v)
{
	struct pid_stat *ps_entry = v;

	if (v == SEQ_START_TOKEN)
		pp_stats_header(m);
	else
		pp_sets(m, ps_entry);

	return 0;
}

static const struct seq_operations proc_qtaguid_pid_stats_seqops = {
	.start = qtaguid_pid_stats_proc_start,
	.next = qtaguid_pid_stats_proc_next,
	.stop = qtaguid_pid_stats_proc_stop,
	.show = qtaguid_pid_stats_proc_show,
};
static int proc_qtaguid_pid_stats_open(struct inode *inode, struct file *file)
{
	return seq_open_private(file, &proc_qtaguid_pid_stats_seqops,
				sizeof(struct proc_print_info));
}

const struct file_operations proc_qtaguid_pid_stats_fops = {
	.open		= proc_qtaguid_pid_stats_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= seq_release_private,
};
EXPORT_SYMBOL(proc_qtaguid_pid_stats_fops);
