

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/skbuff.h>
#include <linux/types.h>
#include <linux/netlink.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/hash.h>
#include <linux/bootmem.h>
#include <linux/sort.h>
#include <uapi/linux/netlink.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <net/tcp.h>
#include <net/tcp_crosslayer.h>

#ifdef CONFIG_HW_CROSSLAYER_OPT

/* 2^31 + 2^29 - 2^25 + 2^22 - 2^19 - 2^16 + 1 */
#define GOLDEN_RATIO_PRIME_32 0x9e370001UL

/****************************/
/*** Variables definition ***/
/****************************/
/* Global aspen cdn hashtable */
static struct aspen_cdn_hashtable *cdn_hashtable;
static unsigned int cdn_hash_shift;

/* Global memory cache for storing aspen cdn hash buckets */
static struct kmem_cache *cdn_hash_slub;
/* Global memory cache for storing dropped packets */
static struct kmem_cache *cdn_drop_slub;

/* Log level */
unsigned int aspen_log_level = 8;
EXPORT_SYMBOL(aspen_log_level);

/* Switch of crosslayer dropped notification */
int aspen_tcp_cdn __read_mostly = 1;
EXPORT_SYMBOL(aspen_tcp_cdn);

#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
struct aspen_netlink_entry aspen_nl_entry;
EXPORT_SYMBOL_GPL(aspen_nl_entry);

static int aspen_monitor_port_range_min[] = { 1, 1 };
static int aspen_monitor_port_range_max[] = { 65535, 65535 };
struct monitor_ports aspen_monitor_ports;
static struct aspen_monitor monitor;
#endif
/****************************/
/*** Variables definition ***/
/****************************/

/*****************************/
/*** Functions declaration ***/
/*****************************/
static inline u32 cdn_hashfn(u32 val, unsigned int bits);
static struct sock *aspen_fetch_hashtable(unsigned short int port);
static struct cdn_entry *aspen_create_cdn_entry(void);
static void aspen_destroy_cdn_entry(struct sock *sk);
static int aspen_crosslayer_dropped_notification(struct sock *sk,
						 unsigned int seq);
static int compare_cdn_info(const void *a, const void *b);
static int aspen_sk_crosslayer_recovery(struct aspen_cdn_info *info,
					int length);
static void aspen_tcp_crosslayer_recovery(struct sock *sk);
static int aspen_skb_crosslayer_retransmit(struct sock *sk,
					   struct tcp_sock *tp,
					   struct sk_buff *skb);
static inline void aspen_tcp_set_modem_drop_undo_marker(struct sock *sk);
static void aspen_tcp_enter_modem_drop_recovery(struct sock *sk);
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
static void aspen_get_monitor_port_range(int *low, int *high);
static void aspen_set_monitor_port_range(int range[2]);
static void aspen_get_monitor_info(char *buf, size_t maxlen);
#endif
/*****************************/
/*** Functions declaration ***/
/*****************************/

#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
static inline void aspen_monitor_inc(atomic_t *v)
{
	atomic_inc(v);
}

static inline void aspen_monitor_dec(atomic_t *v)
{
	atomic_dec(v);
}
#endif

static inline u32 cdn_hashfn(u32 val, unsigned int bits)
{
	/* On some cpus multiply is faster, on others gcc will do shifts */
	u32 hash = (u32)(val * GOLDEN_RATIO_PRIME_32);

	/* High bits are more random, so use them. */
	return hash >> (32 - bits);
}

int aspen_init_hashtable(void)
{
	int i, cdn_hash_size;

#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
	seqlock_init(&aspen_monitor_ports.lock);
	aspen_monitor_ports.range[0] = 5001;
	aspen_monitor_ports.range[1] = 5000;
#endif

	/* Create hash table when booting and never destroy */
	cdn_hashtable = alloc_large_system_hash("aspen_cdn_hashtable",
						sizeof(*cdn_hashtable),
						(unsigned long)0, 18, 0,
						&cdn_hash_shift, NULL,
						(unsigned long)0,
						(unsigned long)8192);
	cdn_hash_size = 1 << cdn_hash_shift;

	if (likely(cdn_hashtable)) {
		for (i = 0; i < cdn_hash_size; i++) {
			INIT_HLIST_HEAD(&cdn_hashtable[i].head);
			spin_lock_init(&cdn_hashtable[i].lock);
		}
	} else {
		ASPEN_ERR("Failed to alloc hashtable.");
		return -ENOMEM;
	}

	/* Create slub memory cache for hash bucket */
	cdn_hash_slub = kmem_cache_create("aspen_cdn_hash_cache",
					  sizeof(struct aspen_cdn_hashbucket),
					  (unsigned long)0,
					  SLAB_HWCACHE_ALIGN | SLAB_PANIC,
					  NULL);

	if (unlikely(!cdn_hash_slub)) {
		ASPEN_ERR("Failed to alloc slub cache of hash buckets.");
		goto clean_out;
	}

	/* Create slub memory cache for dropped packets */
	cdn_drop_slub = kmem_cache_create("aspen_cdn_drop_cache",
					  sizeof(struct cdn_entry),
					  (unsigned long)0,
					  SLAB_HWCACHE_ALIGN | SLAB_PANIC,
					  NULL);

	if (unlikely(!cdn_drop_slub)) {
		ASPEN_ERR("Failed to alloc slub cache of dropped packets.");
		goto clean_out;
	}

	return 0;
clean_out:
	free_page((unsigned long)cdn_hashtable);
	cdn_hashtable = NULL;
	ASPEN_ERR("Release cdn hashtable.");

	if (cdn_hash_slub) {
		kmem_cache_destroy(cdn_hash_slub);
		ASPEN_ERR("Release slub cache of hash buckets.");
	}

	return -ENOMEM;
}
EXPORT_SYMBOL(aspen_init_hashtable);

void aspen_mark_hashtable(struct sock *sk, unsigned short int port)
{
	unsigned int slot, hit;
	struct aspen_cdn_hashbucket *pos;

	/* Check hashtable */
	if (unlikely(!cdn_hashtable))
		return;

	if (!aspen_tcp_cdn)
		return;

	slot = cdn_hashfn(port, cdn_hash_shift);

	/* Must disable bottom halves */
	spin_lock_bh(&cdn_hashtable[slot].lock);

	hit = 0;
	hlist_for_each_entry(pos, &cdn_hashtable[slot].head, node) {
		if (pos->port == port) {
			pos->sk = NULL;
			pos->refs += 1;
			hit = 1;
			break;
		}
	}

	if (likely(!hit)) {
		pos = kmem_cache_alloc(cdn_hash_slub, GFP_ATOMIC);
		if (likely(pos)) {
			pos->sk = sk;
			pos->port = port;
			pos->refs = 1;
			sk->cdn_hash_marker = 1;
			hlist_add_head(&pos->node,
				       &cdn_hashtable[slot].head);
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
			aspen_monitor_inc(&monitor.cdn_hash_slub_cnts);
#endif
		}
	}

	/* Enable bottom halves */
	spin_unlock_bh(&cdn_hashtable[slot].lock);
}
EXPORT_SYMBOL(aspen_mark_hashtable);

void aspen_unmark_hashtable(struct sock *sk)
{
	unsigned int slot;
	struct aspen_cdn_hashbucket *pos;
	struct hlist_node *n;
	struct inet_sock *inet = inet_sk(sk);
	u16 port;

	/* Check hashtable */
	if (unlikely(!cdn_hashtable))
		return;

	if (sk && sk->cdn_hash_marker) {
		inet = inet_sk(sk);
		port = ntohs(inet->inet_sport);
	} else {
		return;
	}

	slot = cdn_hashfn(port, cdn_hash_shift);

	/* Must disable bottom halves */
	spin_lock_bh(&cdn_hashtable[slot].lock);

	hlist_for_each_entry_safe(pos, n, &cdn_hashtable[slot].head, node) {
		if (pos->port == port) {
			pos->refs -= 1;

			if (pos->refs > 0)
				break;

			hlist_del(&pos->node);
			kmem_cache_free(cdn_hash_slub, pos);
			sk->cdn_hash_marker = 0;
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
			aspen_monitor_dec(&monitor.cdn_hash_slub_cnts);
#endif
			break;
		}
	}

	/* Enable bottom halves */
	spin_unlock_bh(&cdn_hashtable[slot].lock);
}
EXPORT_SYMBOL(aspen_unmark_hashtable);

static struct sock *aspen_fetch_hashtable(unsigned short int port)
{
	unsigned int slot;
	struct aspen_cdn_hashbucket *pos;
	struct sock *res;

	/* Check hashtable */
	if (unlikely(!cdn_hashtable)) {
		ASPEN_WARNING("Failed to access hashtable.");
		return NULL;
	}

	slot = cdn_hashfn(port, cdn_hash_shift);

	/* Must disable bottom halves */
	spin_lock_bh(&cdn_hashtable[slot].lock);

	res = NULL;
	hlist_for_each_entry(pos, &cdn_hashtable[slot].head, node) {
		if (pos->port == port) {
			res = pos->sk;
			break;
		}
	}

	if (res) {
		sock_hold(res);
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
		aspen_monitor_inc(&monitor.cdn_sock_hold_cnts);
#endif
		ASPEN_INFO("port=%u hashed_sk=%p refs=%d",
			   port, pos?pos->sk:NULL, pos?pos->refs:0);
	}

	/* Enable bottom halves */
	spin_unlock_bh(&cdn_hashtable[slot].lock);

	return res;
}

static struct cdn_entry *aspen_create_cdn_entry(void)
{
	struct cdn_entry *dropped;

	dropped = kmem_cache_alloc(cdn_drop_slub, GFP_ATOMIC);

	if (!dropped) {
		ASPEN_ERR("Failed to alloc cdn_entry.");
	} else {
		memset(dropped, 0x00, sizeof(struct cdn_entry));
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
		aspen_monitor_inc(&monitor.cdn_drop_slub_cnts);
#endif
	}

	return dropped;
}

static void aspen_destroy_cdn_entry(struct sock *sk)
{
	if (sk && sk->sk_dropped) {
		kmem_cache_free(cdn_drop_slub, sk->sk_dropped);
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
		aspen_monitor_dec(&monitor.cdn_drop_slub_cnts);
#endif
		sk->sk_dropped = NULL;
	}
}

static int aspen_crosslayer_dropped_notification(struct sock *sk,
						 unsigned int seq)
{
	struct cdn_queue *node, *hint;

	if (!sk) {
		ASPEN_ERR("Invalid parameter: zero pointer reference.(sk)");
		return -EFAULT;
	}

	if (sk->sk_dropped && sk->sk_dropped->index >= MAX_CDN_QSIZE) {
		ASPEN_ERR("Out of memory pools.");
		return -ENOMEM;
	}

	if (unlikely(!(sk->sk_dropped))) {
		sk->sk_dropped = aspen_create_cdn_entry();

		if (!sk->sk_dropped)
			return -ENOMEM;

		node = &sk->sk_dropped->pool[sk->sk_dropped->index];
		node->next = NULL;
		node->seq = seq;
		sk->sk_dropped->head = node;
		sk->sk_dropped->hint = node;
	} else {
		node = &sk->sk_dropped->pool[sk->sk_dropped->index];
		node->next = NULL;
		node->seq = seq;

		hint = sk->sk_dropped->hint;
		if (unlikely(!hint))
			hint = sk->sk_dropped->head;

		while (hint) {
			if (likely(after(seq, hint->seq))) {
				/* Hint points to the tail of queue */
				if (likely(!(hint->next))) {
					hint->next = node;
					sk->sk_dropped->hint = node;
					break;
				} else if (before(seq, hint->next->seq)) {
					/*
					 * Hint doesn't point to the tail
					 * Compare with hint->next
					 */
					node->next = hint->next;
					hint->next = node;
					sk->sk_dropped->hint = node;
					break;
				} else {
					hint = hint->next;
				}
			} else if (seq == hint->seq) {
				/* Ignore the duplicate seq */
				break;
			} else {
				/* Insert the node before the head */
				if (hint == sk->sk_dropped->head) {
					node->next = sk->sk_dropped->head;
					sk->sk_dropped->head = node;
					sk->sk_dropped->hint = node;
					break;
				} else {
					/*
					 * Reset the hint point to the head
					 * and find the appropriate position
					 */
					hint = sk->sk_dropped->head;
				}
			}
		}
	}

	sk->sk_dropped->index = sk->sk_dropped->index + 1;

	return 0;
}

static int compare_cdn_info(const void *a, const void *b)
{
	const struct aspen_cdn_info *_a = a;
	const struct aspen_cdn_info *_b = b;

	/* Ascending order */
	if (_a->port == _b->port) {
		/* Ascending order */
		if (_a->seq > _b->seq)
			return 1;
		else if (_a->seq < _b->seq)
			return -1;
		else
			return 0;
	} else if (_a->port > _b->port) {
		return 1;
	} else {
		return -1;
	}
}

void aspen_crosslayer_recovery(void *ptr, int length)
{
	int i, start_index;
	struct aspen_cdn_info *info, *start_point;
	struct timeval tv;

	if (unlikely(!ptr || length <= 0)) {
		ASPEN_ERR("Invalid parameter.(ptr=%p length=%d)", ptr, length);
		return;
	}

	info = (struct aspen_cdn_info *)ptr;

	do_gettimeofday(&tv);
	for (i = 0; i < length; i++)
		ASPEN_INFO("Cross-layer Dropped Notification(%d): "
			   "epoch=%lu.%06lu type=%u sport=%u seq=%u",
			   aspen_tcp_cdn, tv.tv_sec, tv.tv_usec,
			   info[i].type, info[i].port, info[i].seq);

	if (!aspen_tcp_cdn)
		return;

	/* Heapsort provided by kernel */
	sort(info, (unsigned long)length, sizeof(struct aspen_cdn_info),
	     compare_cdn_info, NULL);

	start_point = info;
	start_index = 0;
	for (i = 1; i < length; i++) {
		if (info[i].port != info[i-1].port) {
			aspen_sk_crosslayer_recovery(start_point,
						     i - start_index);
			start_point = &info[i];
			start_index = i;
		}
	}
	aspen_sk_crosslayer_recovery(start_point, i - start_index);
}
EXPORT_SYMBOL(aspen_crosslayer_recovery);

static int aspen_sk_crosslayer_recovery(struct aspen_cdn_info *info,
					int length)
{
	struct sock *sk;
	int i, ret = 0;

	if (!info || length <= 0) {
		ASPEN_ERR("Invalid parameter.(info=%p length=%d)",
			  info, length);
		return -EFAULT;
	}

	/* Hold sock */
	sk = aspen_fetch_hashtable(info[0].port);
	if (!sk) {
		ASPEN_WARNING("Hash missed while searching sk by port(%u).",
			      info[0].port);
		return -EFAULT;
	}

	/* Must disable bottom halves */
	spin_lock_bh(&((sk)->sk_lock.slock));

	for (i = 0; i < length; i++)
		aspen_crosslayer_dropped_notification(sk, info[i].seq);

	/* Put sock */
	aspen_tcp_crosslayer_recovery(sk);

	/* Enable bottom halves */
	spin_unlock_bh(&((sk)->sk_lock.slock));

	return ret;
}

static void aspen_tcp_crosslayer_recovery(struct sock *sk)
{
	if (unlikely(!sk)) {
		ASPEN_ERR("Invalid parameter: zero pointer reference.(sk)");
		return;
	}

	if (!sock_owned_by_user(sk))
		/* Retransmit immediately */
		aspen_tcp_crosslayer_retransmit(sk);
	else
		/* Deleguate our work to tcp_release_cb() */
		test_and_set_bit(TCP_CROSSLAYER_RECOVERY_DEFERRED,
				 &tcp_sk(sk)->tsq_flags);
}

void aspen_tcp_crosslayer_retransmit(struct sock *sk)
{
	struct tcp_sock *tp;
	struct inet_connection_sock *icsk;
	struct sk_buff *skb;
	struct cdn_queue *cdn_q;
	struct tcp_skb_cb *tcb;
	unsigned int length;
	bool maybe_split_skb;
	int err;

	if (sk) {
		tp = tcp_sk(sk);
		icsk = inet_csk(sk);
	} else {
		ASPEN_ERR("Invalid parameter: zero pointer reference.(sk)");
		return;
	}

	if (!sk->sk_dropped) {
		ASPEN_ERR("Zero pointer reference.(sk_dropped)");
		goto out;
	} else if (!sk->sk_dropped->head) {
		ASPEN_ERR("Zero pointer reference.(sk_dropped->head)");
		goto destroy_out;
	}

	ASPEN_DEBUG("sock_flags=%08x sk_state=%02x",
		    (unsigned int)sk->sk_flags, sk->sk_state);

	if (sock_flag(sk, SOCK_DEAD) || sk->sk_state == TCP_CLOSE) {
		ASPEN_DEBUG("Maybe sk(%p) is going to be destroyed.", sk);
	}

	cdn_q = sk->sk_dropped->head;

	/* Travelling unacked queue */
	skb = NULL;
	maybe_split_skb = false;
	tcp_for_write_queue(skb, sk) {
		if (!skb || skb == tcp_send_head(sk))
			break;

		tcb = TCP_SKB_CB(skb);
		length = tcb->end_seq - tcb->seq;
		ASPEN_DEBUG("Travelling skb(%u:%u) length(%u) sk(%p).",
			    tcb->seq, tcb->end_seq, length, sk);

		/* Matching */
		while (cdn_q && after(tcb->seq, cdn_q->seq)) {
			ASPEN_DEBUG("Ingored the dropped packet, maybe "
				    "duplicated or sacked.(seq=%u sk=%p)",
				    cdn_q->seq, sk);
			cdn_q = cdn_q->next;
			maybe_split_skb = false;
		}

		if (!cdn_q) {
			ASPEN_DEBUG("Arrived the tail of cdn queue. Break.");
			break;
		} else if (maybe_split_skb &&
			   !before(tcb->end_seq, cdn_q->seq)) {
			ASPEN_DEBUG("split skb(%u:%u) length(%u) sk(%p)?",
				    tcb->seq, tcb->end_seq, length, sk);
			aspen_skb_crosslayer_retransmit(sk, tp, skb);
			maybe_split_skb = false;
		} else if (!after(tcb->seq, cdn_q->seq) &&
			   after(tcb->end_seq, cdn_q->seq)) {
			err = aspen_skb_crosslayer_retransmit(sk, tp, skb);

			if (err != -EINVAL) {
				if (icsk->icsk_ca_state <= TCP_CA_Disorder)
					aspen_tcp_enter_modem_drop_recovery(sk);
				else if (icsk->icsk_ca_state == TCP_CA_Recovery ||
					 icsk->icsk_ca_state == TCP_CA_Loss)
					aspen_tcp_set_modem_drop_undo_marker(sk);
			}

			cdn_q = cdn_q->next;
			if (cdn_q)
				maybe_split_skb = before(tcb->end_seq,
							 cdn_q->seq);
		} else {
			maybe_split_skb = false;
		}
	}

	while (cdn_q) {
		ASPEN_DEBUG("Ingored the dropped packet, maybe "
			    "rexmitted or merged.(seq=%u sk=%p)",
			    cdn_q->seq, sk);
		cdn_q = cdn_q->next;
	}

	/* Reset timeout timer */
	/* XXX: icsk->icsk_rto is not suitable for LTE */
	inet_csk_reset_xmit_timer(sk, ICSK_TIME_RETRANS, icsk->icsk_rto,
				  TCP_RTO_MAX);

destroy_out:
	/* Destruct cdn_entry */
	aspen_destroy_cdn_entry(sk);

out:
	sock_put(sk);
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
	aspen_monitor_inc(&monitor.cdn_sock_put_cnts);
#endif
}
EXPORT_SYMBOL(aspen_tcp_crosslayer_retransmit);

static int aspen_skb_crosslayer_retransmit(struct sock *sk,
					   struct tcp_sock *tp,
					   struct sk_buff *skb)
{
	int err;
	struct tcp_skb_cb *tcb;
	unsigned int length;

	tcb = TCP_SKB_CB(skb);
	length = tcb->end_seq - tcb->seq;

	if (tcb->sacked & TCPCB_SACKED_ACKED) {
		ASPEN_DEBUG("Ingored sacked skb(%u:%u) length(%u) sk(%p).",
			    tcb->seq, tcb->end_seq, length, sk);
		return -EINVAL;
	}

	err = __tcp_retransmit_skb(sk, skb);
	if (!err) {
		tp->retrans_out += tcp_skb_pcount(skb);
		if (!(tcb->sacked & TCPCB_LOST))
			tp->lost_out += tcp_skb_pcount(skb);
		tcb->sacked |= (TCPCB_LOST | TCPCB_RETRANS);
		tcb->ack_seq = tp->snd_nxt;
		ASPEN_DEBUG("Rexmitted dropped skb(%u:%u) length(%u) sk(%p).",
			    tcb->seq, tcb->end_seq, length, sk);
	} else if (err == -EBUSY) {
		ASPEN_DEBUG("Ingored in host skb(%u:%u) length(%u) sk(%p).",
			    tcb->seq, tcb->end_seq, length, sk);
	} else {
		ASPEN_DEBUG("An error(%d) occurred while rexmitting dropped "
			    "skb(%u:%u) length(%u) sk(%p).",
			    err, tcb->seq, tcb->end_seq, length, sk);
	}
	return err;
}

static inline void aspen_tcp_set_modem_drop_undo_marker(struct sock *sk)
{
	sk->undo_modem_drop_marker = 1;
}

/* Enter Modem Drop Packet state. */
static void aspen_tcp_enter_modem_drop_recovery(struct sock *sk)
{
	struct tcp_sock *tp = tcp_sk(sk);

	tp->prior_ssthresh = tcp_current_ssthresh(sk);
	tp->prior_cwnd = tp->snd_cwnd;
	tp->snd_cwnd_cnt = 0;
	tp->high_seq = tp->snd_nxt;

	tcp_set_ca_state(sk, TCP_CA_Modem_Drop);
	aspen_tcp_set_modem_drop_undo_marker(sk);
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
	sk->modem_drop_rexmit_cnts++;
#endif
}

bool aspen_tcp_try_undo_modem_drop(struct sock *sk,
				   enum tcp_undo_from_state state)
{
	struct tcp_sock *tp = tcp_sk(sk);

	if (sk->undo_modem_drop_marker) {
		if (tp->prior_ssthresh > tp->snd_ssthresh)
			tp->snd_ssthresh = tp->prior_ssthresh;

		if (state == FROM_RECOVERY)
			tp->snd_cwnd = tp->snd_cwnd > tp->prior_cwnd ? tp->snd_cwnd : tp->prior_cwnd;
		else if (state == FROM_LOSS)
			tp->snd_cwnd = tp->snd_cwnd > TCP_INIT_CWND ? tp->snd_cwnd : TCP_INIT_CWND;

		tp->snd_cwnd_stamp = tcp_time_stamp;
		sk->undo_modem_drop_marker = 0;
#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
		sk->undo_modem_drop_cnts++;
#endif
		return true;
	}

	return false;
}
EXPORT_SYMBOL(aspen_tcp_try_undo_modem_drop);

#ifdef CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE
static void aspen_get_monitor_port_range(int *low, int *high)
{
	unsigned int seq;

	do {
		seq = read_seqbegin(&aspen_monitor_ports.lock);
		*low = aspen_monitor_ports.range[0];
		*high = aspen_monitor_ports.range[1];
	} while (read_seqretry(&aspen_monitor_ports.lock, seq));
}

static void aspen_set_monitor_port_range(int range[2])
{
	write_seqlock(&aspen_monitor_ports.lock);
	aspen_monitor_ports.range[0] = range[0];
	aspen_monitor_ports.range[1] = range[1];
	write_sequnlock(&aspen_monitor_ports.lock);
}

/* Validate changes from /proc interface. */
int proc_aspen_monitor_port_range(struct ctl_table *table, int write,
				  void __user *buffer, size_t *lenp,
				  loff_t *ppos)
{
	int ret;
	int range[2];
	struct ctl_table tmp = {
		.data = &range,
		.maxlen = sizeof(range),
		.mode = table->mode,
		.extra1 = &aspen_monitor_port_range_min,
		.extra2 = &aspen_monitor_port_range_max,
	};

	aspen_get_monitor_port_range(&range[0], &range[1]);
	ret = proc_dointvec_minmax(&tmp, write, buffer, lenp, ppos);

	if (write && ret == 0)
		aspen_set_monitor_port_range(range);
	return ret;
}
EXPORT_SYMBOL(proc_aspen_monitor_port_range);

/* Build string with list of aspen monitor infomation */
static void aspen_get_monitor_info(char *buf, size_t maxlen)
{
	int hash_cnts, drop_cnts, hold_cnts, put_cnts, diff;

	/*lint -save -e438 -e529 */
	hash_cnts = atomic_read(&monitor.cdn_hash_slub_cnts);
	drop_cnts = atomic_read(&monitor.cdn_drop_slub_cnts);
	hold_cnts = atomic_read(&monitor.cdn_sock_hold_cnts);
	put_cnts  = atomic_read(&monitor.cdn_sock_put_cnts);
	/*lint -restore */

	diff = hold_cnts - put_cnts;
	snprintf(buf, maxlen,
		 "hash slub: %d\tdrop slub: %d\thold/put: %d/%d(%d)",
		 hash_cnts, drop_cnts, hold_cnts, put_cnts, diff);
}

int proc_aspen_monitor_info(struct ctl_table *ctl, int write,
			    void __user *buffer, size_t *lenp,
			    loff_t *ppos)
{
	struct ctl_table tbl = { .maxlen = ASPEN_MONITOR_BUF_MAX, };
	int ret;

	tbl.data = kmalloc((unsigned long)tbl.maxlen, GFP_USER);
	if (!tbl.data)
		return -ENOMEM;
	
	aspen_get_monitor_info(tbl.data, ASPEN_MONITOR_BUF_MAX);
	ret = proc_dostring(&tbl, write, buffer, lenp, ppos);
	kfree(tbl.data);
	return ret;
}
EXPORT_SYMBOL(proc_aspen_monitor_info);

void aspen_collect_tcp_sender_monitor(struct sock *sk, struct sk_buff *skb,
				      u32 tsval, u32 tsecr,
				      u32 cwnd_remain, u32 snd_wnd_remain)
{
	u16 dport;
	struct tcp_sender_monitor tsm;
	u64 ts_nsec;
	unsigned long rem_nsec;
	struct inet_sock *inet = inet_sk(sk);
	struct tcp_sock *tp = tcp_sk(sk);
	struct tcp_skb_cb *tcb = TCP_SKB_CB(skb);

	dport = ntohs(inet->inet_dport);
	if (dport < aspen_monitor_ports.range[0] ||
	    dport > aspen_monitor_ports.range[1])
		return;

	/* timestamp */
	do_gettimeofday(&tsm.tv);
#ifdef CONFIG_HISI_TIME
	ts_nsec = hisi_getcurtime();
#else
	ts_nsec = local_clock();
#endif
	rem_nsec = do_div(ts_nsec, 1000000000);

	/* SYN, initiaize */
	if (tcb->tcp_flags & TCPHDR_SYN) {
		sk->start_seq = tcb->seq;
		sk->snd_id = 0;
		sk->fast_rexmit_cnts = 0;
		sk->timeout_rexmit_cnts = 0;
		sk->modem_drop_rexmit_cnts = 0;
		sk->undo_modem_drop_cnts = 0;
		strncpy(tsm.type, "SYN\x0", (unsigned long)4);
	} else if (tcb->tcp_flags & TCPHDR_FIN) {
		strncpy(tsm.type, "FIN\x0", (unsigned long)4);
	} else if (tcb->tcp_flags & TCPHDR_RST) {
		strncpy(tsm.type, "RST\x0", (unsigned long)4);
	} else {
		strncpy(tsm.type, "DAT\x0", (unsigned long)4);
	}

	sk->snd_id++;
	tsm.version = 0x1;
	tsm.snd_id = sk->snd_id;
	tsm.uptime_sec = (unsigned long)ts_nsec;
	tsm.uptime_usec = (unsigned long)(rem_nsec / 1000);
	tsm.saddr = inet->inet_saddr;
	tsm.daddr = inet->inet_daddr;
	tsm.sport = ntohs(inet->inet_sport);
	tsm.dport = dport;
	tsm.seq = tcb->seq;
	tsm.end_seq = tcb->end_seq;
	tsm.start_seq = sk->start_seq;
	tsm.tsval = tsval;
	tsm.tsecr = tsecr;
	tsm.cwnd = tp->snd_cwnd;
	tsm.cwnd_remain = cwnd_remain;
	tsm.ssthresh = tp->snd_ssthresh;
	tsm.snd_wnd_remain = snd_wnd_remain;
	tsm.fast_rexmit_cnts = sk->fast_rexmit_cnts;
	tsm.timeout_rexmit_cnts = sk->timeout_rexmit_cnts;
	tsm.nowrtt = sk->nowrtt;
	tsm.rto = (tp->srtt_us >> 3) + tp->rttvar_us;
	tsm.mss_cache = tp->mss_cache;
	tsm.undo_modem_drop_marker = sk->undo_modem_drop_marker;
	tsm.modem_drop_rexmit_cnts = sk->modem_drop_rexmit_cnts;
	tsm.undo_modem_drop_cnts = sk->undo_modem_drop_cnts;
	tsm.ca_state = inet_csk(sk)->icsk_ca_state;

	/* Userland daemon was running */
	if (aspen_nl_entry.userland_pid)
		aspen_nl_entry.callback_send_to_userland(&tsm);
}
EXPORT_SYMBOL(aspen_collect_tcp_sender_monitor);
#endif /* CONFIG_HW_CROSSLAYER_OPT_DBG_MODULE */
#endif /* CONFIG_HW_CROSSLAYER_OPT */
