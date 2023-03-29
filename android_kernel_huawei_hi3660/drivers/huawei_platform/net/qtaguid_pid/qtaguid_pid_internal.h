#include <linux/types.h>
#include <linux/rbtree.h>
#include <linux/spinlock_types.h>
#include <linux/workqueue.h>

/* Iface handling */
#define IDEBUG_MASK (1<<0)
/* Iptable Matching. Per packet. */
#define MDEBUG_MASK (1<<1)
/* Red-black tree handling. Per packet. */
#define RDEBUG_MASK (1<<2)
/* procfs ctrl/stats handling */
#define CDEBUG_MASK (1<<3)
/* dev and resource tracking */
#define DDEBUG_MASK (1<<4)

/*
 * (Un)Define these *DEBUG to compile out/in the pr_debug calls.
 * All undef: text size ~ 0x3030; all def: ~ 0x4404.
 */
#define IDEBUG
#define MDEBUG
#define RDEBUG
#define CDEBUG
#define DDEBUG

#define MSK_DEBUG(mask, ...) do {                           \
		if (unlikely(qtaguid_debug_mask & (mask)))  \
			pr_debug(__VA_ARGS__);              \
	} while (0)
#ifdef IDEBUG
#define IF_DEBUG(...) MSK_DEBUG(IDEBUG_MASK, __VA_ARGS__)
#else
#define IF_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef MDEBUG
#define MT_DEBUG(...) MSK_DEBUG(MDEBUG_MASK, __VA_ARGS__)
#else
#define MT_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef RDEBUG
#define RB_DEBUG(...) MSK_DEBUG(RDEBUG_MASK, __VA_ARGS__)
#else
#define RB_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef CDEBUG
#define CT_DEBUG(...) MSK_DEBUG(CDEBUG_MASK, __VA_ARGS__)
#else
#define CT_DEBUG(...) no_printk(__VA_ARGS__)
#endif
#ifdef DDEBUG
#define DR_DEBUG(...) MSK_DEBUG(DDEBUG_MASK, __VA_ARGS__)
#else
#define DR_DEBUG(...) no_printk(__VA_ARGS__)
#endif

extern uint qtaguid_debug_mask;

/*---------------------------------------------------------------------------*/
/*
 * Tags:
 *
 * They represent what the data usage counters will be tracked against.
 * By default a tag is just based on the UID.
 * The UID is used as the base for policing, and can not be ignored.
 * So a tag will always at least represent a UID (uid_tag).
 *
 * A tag can be augmented with an "accounting tag" which is associated
 * with a UID.
 * User space can set the acct_tag portion of the tag which is then used
 * with sockets: all data belonging to that socket will be counted against the
 * tag. The policing is then based on the tag's uid_tag portion,
 * and stats are collected for the acct_tag portion separately.
 *
 * There could be
 * a:  {acct_tag=1, uid_tag=10003}
 * b:  {acct_tag=2, uid_tag=10003}
 * c:  {acct_tag=3, uid_tag=10003}
 * d:  {acct_tag=0, uid_tag=10003}
 * a, b, and c represent tags associated with specific sockets.
 * d is for the totals for that uid, including all untagged traffic.
 * Typically d is used with policing/quota rules.
 *
 * We want tag_t big enough to distinguish uid_t and acct_tag.
 * It might become a struct if needed.
 * Nothing should be using it as an int.
 */
typedef uint64_t tag_t;  /* Only used via accessors */

#define TAG_UID_MASK 0xFFFFFFFFULL
#define TAG_ACCT_MASK (~0xFFFFFFFFULL)

static inline int tag_compare(tag_t t1, tag_t t2)
{
	return t1 < t2 ? -1 : t1 == t2 ? 0 : 1;
}

static inline tag_t combine_atag_with_uid(tag_t acct_tag, uid_t uid)
{
	return acct_tag | uid;
}
static inline tag_t make_tag_from_uid(uid_t uid)
{
	return uid;
}
static inline uid_t get_uid_from_tag(tag_t tag)
{
	return tag & TAG_UID_MASK;
}
static inline tag_t get_utag_from_tag(tag_t tag)
{
	return tag & TAG_UID_MASK;
}
static inline tag_t get_atag_from_tag(tag_t tag)
{
	return tag & TAG_ACCT_MASK;
}

static inline bool valid_atag(tag_t tag)
{
	return !(tag & TAG_UID_MASK);
}
static inline tag_t make_atag_from_value(uint32_t value)
{
	return (uint64_t)value << 32;
}
/*---------------------------------------------------------------------------*/


#define MAX_HASH_PID_ENTRY_NUM 1024
#define MAX_SOCK_PID_ENTRY_NUM 1024

/*
 * For now we only track 2 sets of counters.
 * The default set is 0.
 * Userspace can activate another set for a given uid being tracked.
 */
#define IFS_MAX_COUNTER_SETS 2

enum ifs_tx_rx {
	IFS_TX,
	IFS_RX,
	IFS_MAX_DIRECTIONS
};

/* For now, TCP, UDP, the rest */
enum ifs_proto {
	IFS_TCP,
	IFS_UDP,
	IFS_PROTO_OTHER,
	IFS_MAX_PROTOS
};

struct byte_packet_counters {
	uint64_t bytes;
	uint64_t packets;
};

struct data_counters {
	struct byte_packet_counters bpc[IFS_MAX_COUNTER_SETS][IFS_MAX_DIRECTIONS][IFS_MAX_PROTOS];
};

static inline uint64_t dc_sum_bytes(struct data_counters *counters,
				    int set,
				    enum ifs_tx_rx direction)
{
	return counters->bpc[set][direction][IFS_TCP].bytes
		+ counters->bpc[set][direction][IFS_UDP].bytes
		+ counters->bpc[set][direction][IFS_PROTO_OTHER].bytes;
}

static inline uint64_t dc_sum_packets(struct data_counters *counters,
				      int set,
				      enum ifs_tx_rx direction)
{
	return counters->bpc[set][direction][IFS_TCP].packets
		+ counters->bpc[set][direction][IFS_UDP].packets
		+ counters->bpc[set][direction][IFS_PROTO_OTHER].packets;
}

/*---------------------------------------------------------------------------*/
#define HASH_ENTRY_EXPIRE_TIMEOUT 60

struct pid_node {
	struct rb_node node;
	char comm[TASK_COMM_LEN];
	uid_t uid;
	tag_t tag;
};

struct pid_stat {
	struct pid_node pn;
	struct data_counters counters;
};

struct iface_pid_stat {
	struct list_head list;  /* in iface_stat_list */
	char *ifname;
	struct rb_root pid_stat_tree;
	spinlock_t pid_stat_list_lock;
};

struct hash_pid {
	struct rb_node hash_node;
	uint32_t skb_hash;
	struct timer_list timer;
	char comm[TASK_COMM_LEN];
	uint32_t comm_hash;
	uid_t uid;
};

struct sock_pid {
	struct rb_node sock_node;
	const struct sock *sk;  /* Only used as a number, never dereferenced */
	char comm[TASK_COMM_LEN];
	uint32_t comm_hash;
	uid_t uid;
	struct hash_pid *hash_pid_ref;
};
