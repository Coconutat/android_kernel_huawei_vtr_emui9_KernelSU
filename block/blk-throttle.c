/*
 * Interface for controlling IO bandwidth on a request queue
 *
 * Copyright (C) 2010 Vivek Goyal <vgoyal@redhat.com>
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/blkdev.h>
#include <linux/bio.h>
#include <linux/blktrace_api.h>
#include <linux/blk-cgroup.h>
#include "blk.h"
#ifdef CONFIG_HUAWEI_IO_TRACING
#include <trace/iotrace.h>
#endif

#define SHARE_SHIFT (14)
#define MAX_SHARE (1 << SHARE_SHIFT)

#define MAX_INFLIGHT		(128)
#define MIN_INFLIGHT		(0)

#define QUANTUM_INQUEUE_MIN	(1)
#define QUANTUM_INQUEUE_MAX	(128)
#define QUANTUM_INQUEUE_DEF	(16)

#define BW_SLICE_MIN	(8ULL << 20)	/* 8MB/s */
#define BW_SLICE_MAX	(256ULL << 20)	/* 256MB/s */
#define BW_SLICE_DEF	(32ULL << 20)	/* 32MB/s */

#define IOPS_SLICE_MIN	(32)
#define IOPS_SLICE_MAX	(4096)
#define IOPS_SLICE_DEF	(256)

#define MAX_THROTL_DELAY_TM_MS	(3000)	/* 3s */

#ifdef CONFIG_HUAWEI_IO_TRACING
DEFINE_TRACE(block_throttle_weight)
DEFINE_TRACE(block_throttle_dispatch)
DEFINE_TRACE(block_throttle_iocost)
DEFINE_TRACE(block_throttle_limit_start)
DEFINE_TRACE(block_throttle_limit_end)
DEFINE_TRACE(block_throttle_bio_in)
DEFINE_TRACE(block_throttle_bio_out)
#endif

enum blk_throtl_weight_onoff_mode {
	BLK_THROTL_WEIGHT_OFF,
	BLK_THROTL_WEIGHT_ON_FS,
	BLK_THROTL_WEIGHT_ON_BLK,
};

/* Max dispatch from a group in 1 round */
static int throtl_grp_quantum = 8;

/* Total max dispatch from all groups in one round */
static int throtl_quantum = 32;

/* Throttling is performed over 100ms slice and after that slice is renewed */
static unsigned long throtl_slice = HZ/10;	/* 100 ms */

static struct blkcg_policy blkcg_policy_throtl;

/* A workqueue to queue throttle related work */
static struct workqueue_struct *kthrotld_workqueue;

/*lint -save -e64 -e570 -e651 -e708 -e785*/
static DEFINE_MUTEX(throtl_mode_lock);
/*lint -restore*/

int blk_throtl_weight_offon = BLK_THROTL_WEIGHT_OFF;

/*
 * To implement hierarchical throttling, throtl_grps form a tree and bios
 * are dispatched upwards level by level until they reach the top and get
 * issued.  When dispatching bios from the children and local group at each
 * level, if the bios are dispatched into a single bio_list, there's a risk
 * of a local or child group which can queue many bios at once filling up
 * the list starving others.
 *
 * To avoid such starvation, dispatched bios are queued separately
 * according to where they came from.  When they are again dispatched to
 * the parent, they're popped in round-robin order so that no single source
 * hogs the dispatch window.
 *
 * throtl_qnode is used to keep the queued bios separated by their sources.
 * Bios are queued to throtl_qnode which in turn is queued to
 * throtl_service_queue and then dispatched in round-robin order.
 *
 * It's also used to track the reference counts on blkg's.  A qnode always
 * belongs to a throtl_grp and gets queued on itself or the parent, so
 * incrementing the reference of the associated throtl_grp when a qnode is
 * queued and decrementing when dequeued is enough to keep the whole blkg
 * tree pinned while bios are in flight.
 */
struct throtl_qnode {
	struct list_head	node;		/* service_queue->queued[] */
	struct bio_list		bios;		/* queued bios */
	struct throtl_grp	*tg;		/* tg this qnode belongs to */
};

struct throtl_service_queue {
	struct throtl_service_queue *parent_sq;	/* the parent service_queue */

	/*
	 * Bios queued directly to this service_queue or dispatched from
	 * children throtl_grp's.
	 */
	struct list_head	queued[2];	/* throtl_qnode [READ/WRITE] */
	unsigned int		nr_queued[2];	/* number of queued bios */

	/*
	 * RB tree of active children throtl_grp's, which are sorted by
	 * their ->disptime.
	 */
	struct rb_root		pending_tree;	/* RB tree of active tgs */
	struct rb_node		*first_pending;	/* first node in the tree */
	unsigned int		nr_pending;	/* # queued in the tree */
	unsigned long		first_pending_disptime;	/* disptime of the first tg */
	struct timer_list	pending_timer;	/* fires on first_pending_disptime */

	unsigned int		children_weight; /* children weight */
	unsigned int		share; /* disk bandwidth share of the queue */
};

enum tg_state_flags {
	THROTL_TG_PENDING	= 1 << 0,	/* on parent's pending tree */
	THROTL_TG_WAS_EMPTY	= 1 << 1,	/* bio_lists[] became non-empty */
	THROTL_TG_ONLINE	= 1 << 2,	/* tg online */
};

#define rb_entry_tg(node)	rb_entry((node), struct throtl_grp, rb_node)

struct throtl_io_cost {
	/* bytes per second rate limits */
	uint64_t bps[2];

	/* IOPS limits */
	unsigned int iops[2];

	/* Number of bytes disptached in current slice */
	uint64_t bytes_disp[2];
	/* Number of bio's dispatched in current slice */
	unsigned int io_disp[2];
};

struct throtl_grp {
	/* must be the first member */
	struct blkg_policy_data pd;

	/* active throtl group service_queue member */
	struct rb_node rb_node;

	/* throtl_data this group belongs to */
	struct throtl_data *td;

	/* this group's service queue */
	struct throtl_service_queue service_queue;

	/*
	 * qnode_on_self is used when bios are directly queued to this
	 * throtl_grp so that local bios compete fairly with bios
	 * dispatched from children.  qnode_on_parent is used when bios are
	 * dispatched from this throtl_grp into its parent and will compete
	 * with the sibling qnode_on_parents and the parent's
	 * qnode_on_self.
	 */
	struct throtl_qnode qnode_on_self[2];
	struct throtl_qnode qnode_on_parent[2];

	/*
	 * Dispatch time in jiffies. This is the estimated time when group
	 * will unthrottle and is ready to dispatch more bio. It is used as
	 * key to sort active groups in service tree.
	 */
	unsigned long disptime;

	unsigned int flags;

	unsigned int type;

	/* are there any throtl rules between this group and td? */
	bool has_rules[2];

	struct throtl_io_cost io_cost;

	/* When did we start a new slice */
	unsigned long slice_start[2];
	unsigned long slice_end[2];
	unsigned long       intime;

	atomic_t        inflights[2];
	struct bio_list     bios;
	struct list_head    node;
	wait_queue_head_t	wait;
	bool			expired;

	int         quantum;

	atomic_t	delay_cnt;
	atomic_t	max_delay;
};

enum run_mode {
	MODE_NONE = 0,
	MODE_THROTTLE = 1, /* bandwidth/iops based throttle */
	/* below are weight based */
	MODE_WEIGHT_BANDWIDTH = 2,
	MODE_WEIGHT_IOPS = 3,
	MAX_MODE = 4,
};

static char *run_mode_name[MAX_MODE] = {
	[MODE_NONE] = "none",
	[MODE_THROTTLE] = "throttle",
	[MODE_WEIGHT_BANDWIDTH] = "weight_bw",
	[MODE_WEIGHT_IOPS] = "weight_iops",
};

unsigned int THROTL_IDLE_INTERVAL = 40;  /* ms */

struct throtl_data
{
	/* service tree for active throtl groups */
	struct throtl_service_queue service_queue;

	struct request_queue *queue;

	/* Total Number of queued bios on READ and WRITE lists */
	unsigned int nr_queued[2];

	/* Work for dispatching throttled bios */
	struct work_struct dispatch_work;
	uint64_t        bw_slice;
	unsigned int        iops_slice;
	int         quantum;

	enum run_mode       mode;

	int         max_inflights;
	atomic_t        inflights;

	struct list_head    active;
	struct list_head    expired;

	wait_queue_head_t   waitq;
	struct timer_list   rescue_timer;

	struct timer_list   idle_timer;
};

static bool td_weight_based(struct throtl_data *td)
{
	return td->mode > MODE_THROTTLE;
}

static void throtl_pending_timer_fn(unsigned long arg);

static inline struct throtl_grp *pd_to_tg(struct blkg_policy_data *pd)
{
	/*lint -save -e826*/
	return pd ? container_of(pd, struct throtl_grp, pd) : NULL;
	/*lint -restore*/
}

static inline struct throtl_grp *blkg_to_tg(struct blkcg_gq *blkg)
{
	return pd_to_tg(blkg_to_pd(blkg, &blkcg_policy_throtl));
}

static inline struct blkcg_gq *tg_to_blkg(struct throtl_grp *tg)
{
	return pd_to_blkg(&tg->pd);
}

/**
 * sq_to_tg - return the throl_grp the specified service queue belongs to
 * @sq: the throtl_service_queue of interest
 *
 * Return the throtl_grp @sq belongs to.  If @sq is the top-level one
 * embedded in throtl_data, %NULL is returned.
 */
static struct throtl_grp *sq_to_tg(struct throtl_service_queue *sq)
{
	if (sq && sq->parent_sq)
		/*lint -save -e826*/
		return container_of(sq, struct throtl_grp, service_queue);
		/*lint -restore*/
	else
		return NULL;
}

/**
 * sq_to_td - return throtl_data the specified service queue belongs to
 * @sq: the throtl_service_queue of interest
 *
 * A service_queue can be embeded in either a throtl_grp or throtl_data.
 * Determine the associated throtl_data accordingly and return it.
 */
static struct throtl_data *sq_to_td(struct throtl_service_queue *sq)
{
	struct throtl_grp *tg = sq_to_tg(sq);

	if (tg)
		return tg->td;
	else
		/*lint -save -e826*/
		return container_of(sq, struct throtl_data, service_queue);
		/*lint -restore*/
}

static inline int tg_data_index(struct throtl_grp *tg, bool rw)
{
	if (td_weight_based(tg->td))
		return 0;
	return rw;
}

static inline uint64_t queue_bandwidth_slice(struct throtl_data *td)
{
	return td->bw_slice;
}

static inline unsigned int queue_iops_slice(struct throtl_data *td)
{
	return td->iops_slice;
}

/**
 * throtl_log - log debug message via blktrace
 * @sq: the service_queue being reported
 * @fmt: printf format string
 * @args: printf args
 *
 * The messages are prefixed with "throtl BLKG_NAME" if @sq belongs to a
 * throtl_grp; otherwise, just "throtl".
 */
#define throtl_log(sq, fmt, args...)	do {				\
	struct throtl_grp *__tg = sq_to_tg((sq));			\
	struct throtl_data *__td = sq_to_td((sq));			\
									\
	(void)__td;							\
	if (likely(!blk_trace_note_message_enabled(__td->queue)))	\
		break;							\
	if ((__tg)) {							\
		char __pbuf[128];					\
									\
		blkg_path(tg_to_blkg(__tg), __pbuf, sizeof(__pbuf));	\
		blk_add_trace_msg(__td->queue, "throtl %s " fmt, __pbuf, ##args); \
	} else {							\
		blk_add_trace_msg(__td->queue, "throtl " fmt, ##args);	\
	}								\
} while (0)

static void throtl_qnode_init(struct throtl_qnode *qn, struct throtl_grp *tg)
{
	INIT_LIST_HEAD(&qn->node);
	bio_list_init(&qn->bios);
	qn->tg = tg;
}

/**
 * throtl_qnode_add_bio - add a bio to a throtl_qnode and activate it
 * @bio: bio being added
 * @qn: qnode to add bio to
 * @queued: the service_queue->queued[] list @qn belongs to
 *
 * Add @bio to @qn and put @qn on @queued if it's not already on.
 * @qn->tg's reference count is bumped when @qn is activated.  See the
 * comment on top of throtl_qnode definition for details.
 */
static void throtl_qnode_add_bio(struct bio *bio, struct throtl_qnode *qn,
				 struct list_head *queued)
{
	bio_list_add(&qn->bios, bio);
	if (list_empty(&qn->node)) {
		list_add_tail(&qn->node, queued);
		blkg_get(tg_to_blkg(qn->tg));
	}
}

/**
 * throtl_peek_queued - peek the first bio on a qnode list
 * @queued: the qnode list to peek
 */
static struct bio *throtl_peek_queued(struct list_head *queued)
{
	/*lint -save -e727 -e730 -e826*/
	struct throtl_qnode *qn = list_first_entry(queued, struct throtl_qnode, node);
	struct bio *bio;

	if (list_empty(queued))
		return NULL;

	bio = bio_list_peek(&qn->bios);
	WARN_ON_ONCE(!bio);
	/*lint -restore*/
	return bio;
}

/**
 * throtl_pop_queued - pop the first bio form a qnode list
 * @queued: the qnode list to pop a bio from
 * @tg_to_put: optional out argument for throtl_grp to put
 *
 * Pop the first bio from the qnode list @queued.  After popping, the first
 * qnode is removed from @queued if empty or moved to the end of @queued so
 * that the popping order is round-robin.
 *
 * When the first qnode is removed, its associated throtl_grp should be put
 * too.  If @tg_to_put is NULL, this function automatically puts it;
 * otherwise, *@tg_to_put is set to the throtl_grp to put and the caller is
 * responsible for putting it.
 */
static struct bio *throtl_pop_queued(struct list_head *queued,
				     struct throtl_grp **tg_to_put)
{
	/*lint -save -e826*/
	struct throtl_qnode *qn = list_first_entry(queued, struct throtl_qnode, node);
	/*lint -restore*/
	struct bio *bio;

	if (list_empty(queued))
		return NULL;

	bio = bio_list_pop(&qn->bios);
	/*lint -save -e727 -e730*/
	WARN_ON_ONCE(!bio);
	/*lint -restore*/

	if (bio_list_empty(&qn->bios)) {
		list_del_init(&qn->node);
		if (tg_to_put)
			*tg_to_put = qn->tg;
		else
			blkg_put(tg_to_blkg(qn->tg));
	} else {
		list_move_tail(&qn->node, queued);
	}

	return bio;
}

/* init a service_queue, assumes the caller zeroed it */
static void throtl_service_queue_init(struct throtl_service_queue *sq)
{
	INIT_LIST_HEAD(&sq->queued[0]);
	INIT_LIST_HEAD(&sq->queued[1]);
	sq->pending_tree = RB_ROOT;
	setup_timer(&sq->pending_timer, throtl_pending_timer_fn,
		    (unsigned long)sq);
}

static inline void io_cost_init(struct throtl_grp *tg)
{
        tg->io_cost.bps[READ] = (u64)(s64)-1;
        tg->io_cost.bps[WRITE] = (u64)(s64)-1;
        tg->io_cost.iops[READ] = (u32)-1;
        tg->io_cost.iops[WRITE] = (u32)-1;
}

static struct blkg_policy_data *throtl_pd_alloc(gfp_t gfp, int node)
{
	struct throtl_grp *tg;
	int rw;

	tg = kzalloc_node(sizeof(*tg), gfp, node);
	if (!tg)
		return NULL;

	throtl_service_queue_init(&tg->service_queue);

	/*lint -save -e574 -e737 -e1058*/
	for (rw = READ; rw <= WRITE; rw++) {
		throtl_qnode_init(&tg->qnode_on_self[rw], tg);
		throtl_qnode_init(&tg->qnode_on_parent[rw], tg);
	}

	tg->intime = 0;
	atomic_set(&tg->inflights[0], 0);
	atomic_set(&tg->inflights[1], 0);
	bio_list_init(&tg->bios);
	INIT_LIST_HEAD(&tg->node);
	init_waitqueue_head(&tg->wait);
	tg->expired = false;
	/*lint -restore*/

	RB_CLEAR_NODE(&tg->rb_node);
	io_cost_init(tg);

	return &tg->pd;
}

static void throtl_pd_init(struct blkg_policy_data *pd)
{
	struct throtl_grp *tg = pd_to_tg(pd);
	struct blkcg_gq *blkg = tg_to_blkg(tg);
	struct throtl_data *td = blkg->q->td;
	struct throtl_service_queue *sq = &tg->service_queue;

	/*
	 * If on the default hierarchy, we switch to properly hierarchical
	 * behavior where limits on a given throtl_grp are applied to the
	 * whole subtree rather than just the group itself.  e.g. If 16M
	 * read_bps limit is set on the root group, the whole system can't
	 * exceed 16M for the device.
	 *
	 * If not on the default hierarchy, the broken flat hierarchy
	 * behavior is retained where all throtl_grps are treated as if
	 * they're all separate root groups right below throtl_data.
	 * Limits of a group don't interact with limits of other groups
	 * regardless of the position of the group in the hierarchy.
	 */
	sq->parent_sq = &td->service_queue;
	if (cgroup_subsys_on_dfl(io_cgrp_subsys) && blkg->parent)
		sq->parent_sq = &blkg_to_tg(blkg->parent)->service_queue;
	tg->td = td;
}

static inline bool io_cost_has_limit(struct throtl_grp *tg, int index)
{
	return tg->io_cost.bps[index] != (u64)(s64)-1 ||
		tg->io_cost.iops[index] != (u32)-1;
}

/*
 * Set has_rules[] if @tg or any of its parents have limits configured.
 * This doesn't require walking up to the top of the hierarchy as the
 * parent's has_rules[] is guaranteed to be correct.
 */
static void tg_update_has_rules(struct throtl_grp *tg)
{
	struct throtl_grp *parent_tg = sq_to_tg(tg->service_queue.parent_sq);
	int i;

	/*lint -save -e574 -e737*/
	for (i = READ; i <= WRITE; i++)
	/*lint -restore*/
		tg->has_rules[i] = (parent_tg && parent_tg->has_rules[i]) ||
				    (io_cost_has_limit(tg, i)) ||
				    (td_weight_based(tg->td));
}

static void tg_update_perf(struct throtl_grp *tg)
{
	struct throtl_service_queue *sq;
	u64 new_bps;
	unsigned int new_iops;

	sq = &tg->service_queue;

	if (!sq->parent_sq)
	    return;

	if (tg->td->mode == MODE_WEIGHT_BANDWIDTH) {
	    new_bps = max_t(uint64_t,
		(queue_bandwidth_slice(tg->td) * sq->share) >> SHARE_SHIFT,
		1024);
	    tg->io_cost.bps[0] = new_bps;
	    tg->io_cost.iops[0] = (u32)-1;
	} else if (tg->td->mode == MODE_WEIGHT_IOPS) {
	    new_iops = max_t(unsigned int,
		(queue_iops_slice(tg->td) * sq->share) >> SHARE_SHIFT,
		1);
	    tg->io_cost.iops[0] = new_iops;
	    tg->io_cost.bps[0] = (u64)(s64)-1;
	}
	/*lint -save -e737*/
	tg->quantum = max_t(int,
	    (throtl_quantum * sq->share) >> SHARE_SHIFT,
	    1);
	/*lint -restore*/
}

/* update share of tg's siblings */
static void tg_update_share(struct throtl_data *td, struct throtl_grp *tg)
{
	struct cgroup_subsys_state *pos_css;
	struct blkcg_gq *blkg, *parent_blkg;
	struct throtl_grp *child;
	if (!tg || !tg->service_queue.parent_sq ||
	    !tg->service_queue.parent_sq->parent_sq)
	    parent_blkg = td->queue->root_blkg;
	else
	    parent_blkg = tg_to_blkg(sq_to_tg(tg->service_queue.parent_sq));

	/*lint -save -e747 -e820*/
	blkg_for_each_descendant_pre(blkg, pos_css, parent_blkg) {
	/*lint -restore*/
	    struct throtl_service_queue *sq;

	    child = blkg_to_tg(blkg);
	    sq = &child->service_queue;

	    if (!sq->parent_sq || !(child->flags & THROTL_TG_ONLINE))
		continue;

	    sq->share = max_t(unsigned int,
		sq->parent_sq->share * blkg->weight /
		    sq->parent_sq->children_weight,
		1);

	    tg_update_perf(child);
	}
}

static void throtl_pd_online(struct blkg_policy_data *pd)
{
	struct throtl_grp *tg = pd_to_tg(pd);
	struct throtl_service_queue *parent_sq;
	struct blkcg_gq *blkg = tg_to_blkg(tg);

	/*
	* We don't want new groups to escape the limits of its ancestors.
	* Update has_rules[] after a new group is brought online.
	*/
	tg_update_has_rules(tg);

	tg->flags |= THROTL_TG_ONLINE;

	parent_sq = tg->service_queue.parent_sq;
	if (parent_sq)
	    parent_sq->children_weight += blkg->weight;

	tg_update_share(tg->td, tg);

}

static void throtl_pd_offline(struct blkg_policy_data *pd)
{
	struct throtl_grp *tg = pd_to_tg(pd);
	struct throtl_service_queue *sq = &tg->service_queue;
	struct blkcg_gq *blkg = tg_to_blkg(tg);

	if (!(tg->flags & THROTL_TG_ONLINE))
	    return;

	if (bio_list_empty(&tg->bios) && !list_empty(&tg->node)) {
	    list_del_init(&tg->node);
	    blkg_put(blkg);
	}

	tg->flags &= ~THROTL_TG_ONLINE;

	if (sq->parent_sq)
	    sq->parent_sq->children_weight -= blkg->weight;

	rcu_read_lock();
	tg_update_share(tg->td, tg);
	rcu_read_unlock();
}

static void throtl_pd_free(struct blkg_policy_data *pd)
{
	struct throtl_grp *tg = pd_to_tg(pd);

	del_timer_sync(&tg->service_queue.pending_timer);
	kfree(tg);
}

static struct throtl_grp *
throtl_rb_first(struct throtl_service_queue *parent_sq)
{
	/* Service tree is empty */
	if (!parent_sq->nr_pending)
		return NULL;

	if (!parent_sq->first_pending)
		parent_sq->first_pending = rb_first(&parent_sq->pending_tree);

	if (parent_sq->first_pending)
		/*lint -save -e826*/
		return rb_entry_tg(parent_sq->first_pending);
		/*lint -restore*/

	return NULL;
}

static void rb_erase_init(struct rb_node *n, struct rb_root *root)
{
	rb_erase(n, root);
	RB_CLEAR_NODE(n);
}

static void throtl_rb_erase(struct rb_node *n,
			    struct throtl_service_queue *parent_sq)
{
	if (parent_sq->first_pending == n)
		parent_sq->first_pending = NULL;
	rb_erase_init(n, &parent_sq->pending_tree);
	--parent_sq->nr_pending;
}

static void update_min_dispatch_time(struct throtl_service_queue *parent_sq)
{
	struct throtl_grp *tg;

	tg = throtl_rb_first(parent_sq);
	if (!tg)
		return;

	parent_sq->first_pending_disptime = tg->disptime;
}

static void tg_service_queue_add(struct throtl_grp *tg)
{
	struct throtl_service_queue *parent_sq = tg->service_queue.parent_sq;
	struct rb_node **node = &parent_sq->pending_tree.rb_node;
	struct rb_node *parent = NULL;
	struct throtl_grp *__tg;
	unsigned long key = tg->disptime;
	int left = 1;

	while (*node != NULL) {
		parent = *node;
		/*lint -save -e550 -e774 -e826*/
		__tg = rb_entry_tg(parent);

		if (time_before(key, __tg->disptime))
			node = &parent->rb_left;
		else {
			node = &parent->rb_right;
			left = 0;
		}
		/*lint -restore*/
	}

	if (left)
		parent_sq->first_pending = &tg->rb_node;

	rb_link_node(&tg->rb_node, parent, node);
	rb_insert_color(&tg->rb_node, &parent_sq->pending_tree);
}

static void __throtl_enqueue_tg(struct throtl_grp *tg)
{
	tg_service_queue_add(tg);
	tg->flags |= THROTL_TG_PENDING;
	tg->service_queue.parent_sq->nr_pending++;
}

static void throtl_enqueue_tg(struct throtl_grp *tg)
{
	if (!(tg->flags & THROTL_TG_PENDING))
		__throtl_enqueue_tg(tg);
}

static void __throtl_dequeue_tg(struct throtl_grp *tg)
{
	throtl_rb_erase(&tg->rb_node, tg->service_queue.parent_sq);
	tg->flags &= ~THROTL_TG_PENDING;
}

static void throtl_dequeue_tg(struct throtl_grp *tg)
{
	if (tg->flags & THROTL_TG_PENDING)
		__throtl_dequeue_tg(tg);
}

/* Call with queue lock held */
static void throtl_schedule_pending_timer(struct throtl_service_queue *sq,
					  unsigned long expires)
{
	unsigned long max_expire = jiffies + 8 * throtl_slice;

	/*
	 * Since we are adjusting the throttle limit dynamically, the sleep
	 * time calculated according to previous limit might be invalid. It's
	 * possible the cgroup sleep time is very long and no other cgroups
	 * have IO running so notify the limit changes. Make sure the cgroup
	 * doesn't sleep too long to avoid the missed notification.
	 */
	if (time_after(expires, max_expire))
		expires = max_expire;
	mod_timer(&sq->pending_timer, expires);
	/*lint -save -e747*/
	throtl_log(sq, "schedule timer. delay=%lu jiffies=%lu",
		   expires - jiffies, jiffies);
	/*lint -restore*/
}

/**
 * throtl_schedule_next_dispatch - schedule the next dispatch cycle
 * @sq: the service_queue to schedule dispatch for
 * @force: force scheduling
 *
 * Arm @sq->pending_timer so that the next dispatch cycle starts on the
 * dispatch time of the first pending child.  Returns %true if either timer
 * is armed or there's no pending child left.  %false if the current
 * dispatch window is still open and the caller should continue
 * dispatching.
 *
 * If @force is %true, the dispatch timer is always scheduled and this
 * function is guaranteed to return %true.  This is to be used when the
 * caller can't dispatch itself and needs to invoke pending_timer
 * unconditionally.  Note that forced scheduling is likely to induce short
 * delay before dispatch starts even if @sq->first_pending_disptime is not
 * in the future and thus shouldn't be used in hot paths.
 */
static bool throtl_schedule_next_dispatch(struct throtl_service_queue *sq,
					  bool force)
{
	/* any pending children left? */
	if (!sq->nr_pending)
		return true;

	update_min_dispatch_time(sq);

	/* is the next dispatch time in the future? */
	/*lint -save -e550 -e774*/
	if (force || time_after(sq->first_pending_disptime, jiffies)) {
		throtl_schedule_pending_timer(sq, sq->first_pending_disptime);
		return true;
	}
	/*lint -restore*/

	/* tell the caller to continue dispatching */
	return false;
}

static inline void io_cost_start_new_slice(struct throtl_grp *tg,
		bool rw)
{
	int index = tg_data_index(tg, rw);
	tg->io_cost.bytes_disp[index] = 0;
	tg->io_cost.io_disp[index] = 0;
}

static inline void throtl_start_new_slice_with_credit(struct throtl_grp *tg,
		bool rw, unsigned long start)
{
	int index = tg_data_index(tg, rw);
	io_cost_start_new_slice(tg, rw);

	/*
	 * Previous slice has expired. We must have trimmed it after last
	 * bio dispatch. That means since start of last slice, we never used
	 * that bandwidth. Do try to make use of that bandwidth while giving
	 * credit.
	 */
	/*lint -save -e550 -e747 -e774 -e1072*/
	if (time_after_eq(start, tg->slice_start[index]))
		tg->slice_start[index] = start;

	tg->slice_end[index] = jiffies + throtl_slice;
	throtl_log(&tg->service_queue,
		   "[%c] new slice with credit start=%lu end=%lu jiffies=%lu",
		   rw == READ ? 'R' : 'W', tg->slice_start[index],
		   tg->slice_end[index], jiffies);
	/*lint -restore*/
}

static inline void throtl_start_new_slice(struct throtl_grp *tg, bool rw)
{
	int index = tg_data_index(tg, rw);

	io_cost_start_new_slice(tg, rw);
	tg->slice_start[index] = jiffies;
	tg->slice_end[index] = jiffies + throtl_slice;
	/*lint -save -e747*/
	throtl_log(&tg->service_queue,
		   "[%c] new slice start=%lu end=%lu jiffies=%lu",
		   rw == READ ? 'R' : 'W', tg->slice_start[index],
		   tg->slice_end[index], jiffies);
	/*lint -restore*/
}

static inline void throtl_set_slice_end(struct throtl_grp *tg, bool rw,
					unsigned long jiffy_end)
{
	int index = tg_data_index(tg, rw);
	tg->slice_end[index] = roundup(jiffy_end, throtl_slice);
}

static inline void throtl_extend_slice(struct throtl_grp *tg, bool rw,
				       unsigned long jiffy_end)
{
	int index = tg_data_index(tg, rw);
	tg->slice_end[index] = roundup(jiffy_end, throtl_slice);
	/*lint -save -e747*/
	throtl_log(&tg->service_queue,
		   "[%c] extend slice start=%lu end=%lu jiffies=%lu",
		   rw == READ ? 'R' : 'W', tg->slice_start[index],
		   tg->slice_end[index], jiffies);
	/*lint -restore*/
}

/* Determine if previously allocated or extended slice is complete or not */
static bool throtl_slice_used(struct throtl_grp *tg, bool rw)
{
	int index = tg_data_index(tg, rw);
	/*lint -save -e550 -e774 -e1072*/
	if (time_in_range(jiffies, tg->slice_start[index], tg->slice_end[index]))
	/*lint -restore*/
		return false;

	return 1;
}

static inline bool io_cost_trim_slice(struct throtl_grp *tg, bool rw,
	unsigned long nr_slices)
{
	unsigned long io_trim;
	u64 bytes_trim, tmp;
	int index = tg_data_index(tg, rw);

	tmp = tg->io_cost.bps[index] * throtl_slice * nr_slices;
	do_div(tmp, HZ);
	bytes_trim = tmp;

	io_trim = (tg->io_cost.iops[index] * throtl_slice * nr_slices) / HZ;

	if (!bytes_trim && !io_trim)
		return false;

	if (tg->io_cost.bytes_disp[index] >= bytes_trim)
		tg->io_cost.bytes_disp[index] -= bytes_trim;
	else
		tg->io_cost.bytes_disp[index] = 0;

	if (tg->io_cost.io_disp[index] >= io_trim)
		tg->io_cost.io_disp[index] -= (unsigned int)io_trim;
	else
		tg->io_cost.io_disp[index] = 0;

	return true;
}

/* Trim the used slices and adjust slice start accordingly */
static inline void throtl_trim_slice(struct throtl_grp *tg, bool rw)
{
	unsigned long nr_slices, time_elapsed;
	int index = tg_data_index(tg, rw);

	/*lint -save -e550 -e730 -e1072*/
	BUG_ON(time_before(tg->slice_end[index], tg->slice_start[index]));
	/*lint -restore*/

	/*
	 * If bps are unlimited (-1), then time slice don't get
	 * renewed. Don't try to trim the slice if slice is used. A new
	 * slice will start when appropriate.
	 */
	if (throtl_slice_used(tg, rw))
		return;

	/*
	 * A bio has been dispatched. Also adjust slice_end. It might happen
	 * that initially cgroup limit was very low resulting in high
	 * slice_end, but later limit was bumped up and bio was dispached
	 * sooner, then we need to reduce slice_end. A high bogus slice_end
	 * is bad because it does not allow new slice to start.
	 */

	throtl_set_slice_end(tg, rw, jiffies + throtl_slice);

	time_elapsed = jiffies - tg->slice_start[index];

	nr_slices = time_elapsed / throtl_slice;

	if (!nr_slices)
		return;

	if (!io_cost_trim_slice(tg, rw, nr_slices))
		return;
	tg->slice_start[index] += nr_slices * throtl_slice;

	/*lint -save -e747*/
	throtl_log(&tg->service_queue,
		   "[%c] trim slice nr=%lu start=%lu end=%lu jiffies=%lu",
		   rw == READ ? 'R' : 'W', nr_slices,
		   tg->slice_start[index], tg->slice_end[index], jiffies);
	/*lint -restore*/
}

static bool io_cost_with_in_iops_limit(struct throtl_grp *tg, struct bio *bio,
				  unsigned long *wait)
{
	bool rw = bio_data_dir(bio);
	int index = tg_data_index(tg, rw);
	unsigned int io_allowed;
	unsigned long jiffy_elapsed, jiffy_wait, jiffy_elapsed_rnd;
	u64 tmp;

	jiffy_elapsed = jiffy_elapsed_rnd = jiffies - tg->slice_start[index];

	/* Slice has just started. Consider one slice interval */
	if (!jiffy_elapsed)
		jiffy_elapsed_rnd = throtl_slice;

	jiffy_elapsed_rnd = roundup(jiffy_elapsed_rnd, throtl_slice);

	/*
	 * jiffy_elapsed_rnd should not be a big value as minimum iops can be
	 * 1 then at max jiffy elapsed should be equivalent of 1 second as we
	 * will allow dispatch after 1 second and after that slice should
	 * have been trimmed.
	 */

	tmp = (u64)tg->io_cost.iops[index] * jiffy_elapsed_rnd;
	do_div(tmp, HZ);

	if (tmp > UINT_MAX)
		io_allowed = UINT_MAX;
	else
		/*lint -save -e712*/
		io_allowed = tmp;
		/*lint -restore*/

	if (tg->io_cost.io_disp[index] + 1 <= io_allowed) {
		if (wait)
			*wait = 0;
		return true;
	}

	/* Calc approx time to dispatch */
	/*lint -save -e647*/
	jiffy_wait = ((tg->io_cost.io_disp[index] + 1) * HZ) /
		tg->io_cost.iops[index] + 1;
	/*lint -restore*/

	if (jiffy_wait > jiffy_elapsed)
		jiffy_wait = jiffy_wait - jiffy_elapsed;
	else
		jiffy_wait = 1;

	if (wait)
		*wait = jiffy_wait;
	return false;
}

static bool io_cost_with_in_bps_limit(struct throtl_grp *tg, struct bio *bio,
				 unsigned long *wait)
{
	bool rw = bio_data_dir(bio);
	int index = tg_data_index(tg, rw);
	u64 bytes_allowed, extra_bytes, tmp;
	unsigned long jiffy_elapsed, jiffy_wait, jiffy_elapsed_rnd;

	jiffy_elapsed = jiffy_elapsed_rnd = jiffies - tg->slice_start[index];

	/* Slice has just started. Consider one slice interval */
	if (!jiffy_elapsed)
		jiffy_elapsed_rnd = throtl_slice;

	jiffy_elapsed_rnd = roundup(jiffy_elapsed_rnd, throtl_slice);

	tmp = tg->io_cost.bps[index] * jiffy_elapsed_rnd;
	do_div(tmp, HZ);
	bytes_allowed = tmp;

	if (tg->io_cost.bytes_disp[index] + bio->bi_iter.bi_size <=
	    bytes_allowed) {
		if (wait)
			*wait = 0;
		return true;
	}

	/* Calc approx time to dispatch */
	extra_bytes = tg->io_cost.bytes_disp[index] +
		bio->bi_iter.bi_size - bytes_allowed;
	jiffy_wait = div64_u64(extra_bytes * HZ, tg->io_cost.bps[index]);

	if (!jiffy_wait)
		jiffy_wait = 1;

	/*
	 * This wait time is without taking into consideration the rounding
	 * up we did. Add that time also.
	 */
	jiffy_wait = jiffy_wait + (jiffy_elapsed_rnd - jiffy_elapsed);
	if (wait)
		*wait = jiffy_wait;
	return false;
}

static bool io_cost_with_in_limit(struct throtl_grp *tg, struct bio *bio,
		unsigned long *wait)
{
	unsigned long bps_wait = 0, iops_wait = 0;

	if (io_cost_with_in_bps_limit(tg, bio, &bps_wait) &&
	    io_cost_with_in_iops_limit(tg, bio, &iops_wait)) {
		*wait = 0;
		return true;
	}
	*wait = max(bps_wait, iops_wait);
	return false;
}

/*
 * Returns whether one can dispatch a bio or not. Also returns approx number
 * of jiffies to wait before this bio is with-in IO rate and can be dispatched
 */
static bool tg_may_dispatch(struct throtl_grp *tg, struct bio *bio,
			    unsigned long *wait)
{
	bool rw = bio_data_dir(bio);
	int index = tg_data_index(tg, rw);
	unsigned long max_wait = 0;

	/*
 	 * Currently whole state machine of group depends on first bio
	 * queued in the group bio list. So one should not be calling
	 * this function with a different bio if there are other bios
	 * queued.
	 */
	/*lint -save -e730*/
	BUG_ON(tg->service_queue.nr_queued[index] &&
	       bio != throtl_peek_queued(&tg->service_queue.queued[index]));
	/*lint -restore*/

	/* If tg->bps = -1, then BW is unlimited */
	if (!io_cost_has_limit(tg, index)) {
		if (wait)
			*wait = 0;
		return true;
	}

	/*
	 * If previous slice expired, start a new one otherwise renew/extend
	 * existing slice to make sure it is at least throtl_slice interval
	 * long since now. New slice is started only for empty throttle group.
	 * If there is queued bio, that means there should be an active
	 * slice and it should be extended instead.
	 */
	if (throtl_slice_used(tg, rw) && !(tg->service_queue.nr_queued[rw]))
		throtl_start_new_slice(tg, rw);
	else {
		/*lint -save -e550 -e774 -e1072*/
		if (time_before(tg->slice_end[index], jiffies + throtl_slice))
			throtl_extend_slice(tg, rw, jiffies + throtl_slice);
		/*lint -restore*/
	}

	 if (io_cost_with_in_limit(tg, bio, &max_wait)) {
		if (wait)
			*wait = 0;
		return true;
	}


	if (wait)
		*wait = max_wait;

	/*lint -save -e550 -e774 -e1072*/
	if (time_before(tg->slice_end[index], jiffies + max_wait))
		throtl_extend_slice(tg, rw, jiffies + max_wait);
	/*lint -restore*/

	return false;
}

static inline void io_cost_charge_bio(struct throtl_grp *tg, struct bio *bio)
{
	bool rw = bio_data_dir(bio);
	int index = tg_data_index(tg, rw);

	tg->io_cost.bytes_disp[index] += bio->bi_iter.bi_size;
	tg->io_cost.io_disp[index]++;
}

static void throtl_charge_bio(struct throtl_grp *tg, struct bio *bio)
{
	io_cost_charge_bio(tg, bio);
	/*
	 * REQ_THROTTLED is used to prevent the same bio to be throttled
	 * more than once as a throttled bio will go through blk-throtl the
	 * second time when it eventually gets issued.  Set it when a bio
	 * is being charged to a tg.
	 */
	/*lint -save -e712 -e747*/
	if (!(bio->bi_opf & REQ_THROTTLED))
		bio->bi_opf |= REQ_THROTTLED;
	/*lint -restore*/
}

/**
 * throtl_add_bio_tg - add a bio to the specified throtl_grp
 * @bio: bio to add
 * @qn: qnode to use
 * @tg: the target throtl_grp
 *
 * Add @bio to @tg's service_queue using @qn.  If @qn is not specified,
 * tg->qnode_on_self[] is used.
 */
static void throtl_add_bio_tg(struct bio *bio, struct throtl_qnode *qn,
			      struct throtl_grp *tg)
{
	struct throtl_service_queue *sq = &tg->service_queue;
	bool rw = bio_data_dir(bio);
	int index = tg_data_index(tg, rw);

	if (!qn)
		qn = &tg->qnode_on_self[index];

	/*
	 * If @tg doesn't currently have any bios queued in the same
	 * direction, queueing @bio can change when @tg should be
	 * dispatched.  Mark that @tg was empty.  This is automatically
	 * cleaered on the next tg_update_disptime().
	 */
	if (!sq->nr_queued[index])
		tg->flags |= THROTL_TG_WAS_EMPTY;

	throtl_qnode_add_bio(bio, qn, &sq->queued[index]);

	sq->nr_queued[index]++;
	throtl_enqueue_tg(tg);
}

static void tg_update_disptime(struct throtl_grp *tg)
{
	/*lint -save -e570 -e820 -e838*/
	struct throtl_service_queue *sq = &tg->service_queue;
	unsigned long read_wait = -1, write_wait = -1, min_wait = -1, disptime;
	struct bio *bio;

	if ((bio = throtl_peek_queued(&sq->queued[READ])))
		tg_may_dispatch(tg, bio, &read_wait);

	if ((bio = throtl_peek_queued(&sq->queued[WRITE])))
		tg_may_dispatch(tg, bio, &write_wait);

	min_wait = min(read_wait, write_wait);
	disptime = jiffies + min_wait;
	/*lint -restore*/

	/* Update dispatch time */
	throtl_dequeue_tg(tg);
	tg->disptime = disptime;
	throtl_enqueue_tg(tg);

	/* see throtl_add_bio_tg() */
	tg->flags &= ~THROTL_TG_WAS_EMPTY;
}

static void start_parent_slice_with_credit(struct throtl_grp *child_tg,
					struct throtl_grp *parent_tg, bool rw)
{
	int index = tg_data_index(child_tg, rw);
	if (throtl_slice_used(parent_tg, rw)) {
		throtl_start_new_slice_with_credit(parent_tg, rw,
				child_tg->slice_start[index]);
	}

}

static void tg_dispatch_one_bio(struct throtl_grp *tg, bool rw)
{
	struct throtl_service_queue *sq = &tg->service_queue;
	struct throtl_service_queue *parent_sq = sq->parent_sq;
	struct throtl_grp *parent_tg = sq_to_tg(parent_sq);
	struct throtl_grp *tg_to_put = NULL;
	struct bio *bio;
	int index = tg_data_index(tg, rw);

	/*
	 * @bio is being transferred from @tg to @parent_sq.  Popping a bio
	 * from @tg may put its reference and @parent_sq might end up
	 * getting released prematurely.  Remember the tg to put and put it
	 * after @bio is transferred to @parent_sq.
	 */
	bio = throtl_pop_queued(&sq->queued[index], &tg_to_put);
	sq->nr_queued[index]--;

	throtl_charge_bio(tg, bio);

	/*
	 * If our parent is another tg, we just need to transfer @bio to
	 * the parent using throtl_add_bio_tg().  If our parent is
	 * @td->service_queue, @bio is ready to be issued.  Put it on its
	 * bio_lists[] and decrease total number queued.  The caller is
	 * responsible for issuing these bios.
	 */
	if (parent_tg) {
		throtl_add_bio_tg(bio, &tg->qnode_on_parent[index], parent_tg);
		start_parent_slice_with_credit(tg, parent_tg, rw);
	} else {
		throtl_qnode_add_bio(bio, &tg->qnode_on_parent[index],
				     &parent_sq->queued[index]);
		/*lint -save -e730 -e775*/
		BUG_ON(tg->td->nr_queued[index] <= 0);
		/*lint -restore*/
		tg->td->nr_queued[index]--;
	}

	throtl_trim_slice(tg, rw);

	if (tg_to_put)
		blkg_put(tg_to_blkg(tg_to_put));
}

static int throtl_dispatch_tg(struct throtl_grp *tg)
{
	/*lint -save -e713 -e730 -e732 -e737 -e747 -e820*/
	struct throtl_service_queue *sq = &tg->service_queue;
	unsigned int nr_reads = 0, nr_writes = 0;
	unsigned int max_nr_reads = throtl_grp_quantum*3/4;
	unsigned int max_nr_writes = throtl_grp_quantum - max_nr_reads;
	struct bio *bio;

	/* Try to dispatch 75% READS and 25% WRITES */

	while ((bio = throtl_peek_queued(&sq->queued[READ])) &&
	       tg_may_dispatch(tg, bio, NULL)) {

		tg_dispatch_one_bio(tg, bio_data_dir(bio));
		nr_reads++;

		if (nr_reads >= max_nr_reads)
			break;
	}

	while ((bio = throtl_peek_queued(&sq->queued[WRITE])) &&
	       tg_may_dispatch(tg, bio, NULL)) {

		tg_dispatch_one_bio(tg, bio_data_dir(bio));
		nr_writes++;

		if (nr_writes >= max_nr_writes)
			break;
	}

	return nr_reads + nr_writes;
	/*lint -restore*/
}

static int throtl_select_dispatch(struct throtl_service_queue *parent_sq)
{
	unsigned int nr_disp = 0;

	/*lint -save -e550 -e574 -e713 -e737 -e774*/
	while (1) {
		struct throtl_grp *tg = throtl_rb_first(parent_sq);
		struct throtl_service_queue *sq = &tg->service_queue;

		if (!tg)
			break;

		if (time_before(jiffies, tg->disptime))
			break;

		throtl_dequeue_tg(tg);

		nr_disp += throtl_dispatch_tg(tg);

		if (sq->nr_queued[0] || sq->nr_queued[1])
			tg_update_disptime(tg);

		if (nr_disp >= throtl_quantum)
			break;
	}

	return nr_disp;
	/*lint -restore*/
}

/**
 * throtl_pending_timer_fn - timer function for service_queue->pending_timer
 * @arg: the throtl_service_queue being serviced
 *
 * This timer is armed when a child throtl_grp with active bio's become
 * pending and queued on the service_queue's pending_tree and expires when
 * the first child throtl_grp should be dispatched.  This function
 * dispatches bio's from the children throtl_grps to the parent
 * service_queue.
 *
 * If the parent's parent is another throtl_grp, dispatching is propagated
 * by either arming its pending_timer or repeating dispatch directly.  If
 * the top-level service_tree is reached, throtl_data->dispatch_work is
 * kicked so that the ready bio's are issued.
 */
static void throtl_pending_timer_fn(unsigned long arg)
{
	struct throtl_service_queue *sq = (void *)arg;
	struct throtl_grp *tg = sq_to_tg(sq);
	struct throtl_data *td = sq_to_td(sq);
	struct request_queue *q = td->queue;
	struct throtl_service_queue *parent_sq;
	bool dispatched;
	int ret;

	spin_lock_irq(q->queue_lock);
again:
	parent_sq = sq->parent_sq;
	dispatched = false;

	/*lint -save -e747*/
	while (true) {
		throtl_log(sq, "dispatch nr_queued=%u read=%u write=%u",
			   sq->nr_queued[READ] + sq->nr_queued[WRITE],
			   sq->nr_queued[READ], sq->nr_queued[WRITE]);

		ret = throtl_select_dispatch(sq);
		if (ret) {
			throtl_log(sq, "bios disp=%u", ret);
			dispatched = true;
		}

		if (throtl_schedule_next_dispatch(sq, false))
			break;

		/* this dispatch windows is still open, relax and repeat */
		spin_unlock_irq(q->queue_lock);
		cpu_relax();
		spin_lock_irq(q->queue_lock);
	}

	if (!dispatched)
		goto out_unlock;

	if (parent_sq) {
		/* @parent_sq is another throl_grp, propagate dispatch */
		if (tg->flags & THROTL_TG_WAS_EMPTY) {
			tg_update_disptime(tg);
			if (!throtl_schedule_next_dispatch(parent_sq, false)) {
				/* window is already open, repeat dispatching */
				sq = parent_sq;
				tg = sq_to_tg(sq);
				goto again;
			}
		}
	} else {
		/* reached the top-level, queue issueing */
		queue_work(kthrotld_workqueue, &td->dispatch_work);
	}
	/*lint -restore*/
out_unlock:
	spin_unlock_irq(q->queue_lock);
}

/**
 * blk_throtl_dispatch_work_fn - work function for throtl_data->dispatch_work
 * @work: work item being executed
 *
 * This function is queued for execution when bio's reach the bio_lists[]
 * of throtl_data->service_queue.  Those bio's are ready and issued by this
 * function.
 */
static void blk_throtl_dispatch_work_fn(struct work_struct *work)
{
	/*lint -save -e826*/
	struct throtl_data *td = container_of(work, struct throtl_data,
					      dispatch_work);
	/*lint -restore*/
	struct throtl_service_queue *td_sq = &td->service_queue;
	struct request_queue *q = td->queue;
	struct bio_list bio_list_on_stack;
	struct bio *bio;
	struct blk_plug plug;
	int i;

	bio_list_init(&bio_list_on_stack);

	spin_lock_irq(q->queue_lock);
	/*lint -save -e574 -e737 -e820*/
	for (i = READ; i <= WRITE; i++)
		while ((bio = throtl_pop_queued(&td_sq->queued[i], NULL)))
			bio_list_add(&bio_list_on_stack, bio);
	spin_unlock_irq(q->queue_lock);

	if (!bio_list_empty(&bio_list_on_stack)) {
		blk_start_plug(&plug);
		while((bio = bio_list_pop(&bio_list_on_stack)))
			generic_make_request(bio);
		blk_finish_plug(&plug);
	}
	/*lint -restore*/
}

static bool tg_may_dispatch_weight(struct throtl_grp *tg)
{
        if (!io_cost_has_limit(tg, 0))
                return true;

        if (!(tg->flags & THROTL_TG_ONLINE))
                return true;

        if (tg->td->mode == MODE_WEIGHT_BANDWIDTH)
                return (tg->io_cost.bytes_disp[0] < tg->io_cost.bps[0]);
        else if (tg->td->mode == MODE_WEIGHT_IOPS)
                return (tg->io_cost.io_disp[0] < tg->io_cost.iops[0]);
        else
                return true;
}

static void throtl_add_bio_weight_trace(struct bio *bio)
{
	bio->bi_throtl_in_queue = jiffies;
#ifdef CONFIG_HUAWEI_IO_TRACING
	trace_block_throttle_bio_in(bio);
#endif
}

static void throtl_pop_bio_weight_trace(struct bio *bio, struct throtl_grp *tg)
{
        unsigned int delay;
        char b[BDEVNAME_SIZE];

        delay = jiffies_to_msecs(jiffies - bio->bi_throtl_in_queue);
        if (delay > MAX_THROTL_DELAY_TM_MS) {
                /*lint -save -e529 -e438 -e1058*/
                if ((unsigned int)atomic_read(&tg->max_delay) < delay)
                atomic_set(&tg->max_delay, (int)delay);
                atomic_inc(&tg->delay_cnt);
                /*lint -restore*/
                printk("io-latency: [throtl] [%s] sector=%lu size=%u flag=0x%lx delay=%d weight=%d type=%d\n",
                        bdevname(bio->bi_bdev, b), bio->bi_iter.bi_sector,
                        bio->bi_iter.bi_size, (unsigned long)bio->bi_opf, delay,
                        tg_to_blkg(tg)->weight, tg_to_blkg(tg)->blkcg->type);
        }
#ifdef CONFIG_HUAWEI_IO_TRACING
        trace_block_throttle_bio_out(bio, (long)delay);
#endif
}

static void throtl_add_bio_weight(struct bio *bio, struct throtl_grp *tg)
{
        struct throtl_data *td = tg->td;

	throtl_add_bio_weight_trace(bio);
#ifdef CONFIG_HISI_BLK
        bio->hisi_bio.io_in_count |= HISI_IO_IN_COUNT_WILL_BE_SEND_AGAIN;
#endif
        bio_list_add(&tg->bios, bio);
        tg->service_queue.nr_queued[0]++;
        tg->td->nr_queued[0]++;
        tg->intime = jiffies;

#ifdef CONFIG_HUAWEI_IO_TRACING
	trace_block_throttle_weight(bio, tg_to_blkg(tg)->weight,
			tg->td->nr_queued[0]);
#endif

        if (tg->td->nr_queued[0] == 1 && !timer_pending(&tg->td->rescue_timer))
                mod_timer(&tg->td->rescue_timer,
                          jiffies + msecs_to_jiffies(THROTL_IDLE_INTERVAL));

        if (!list_empty(&tg->node))
                return;

        blkg_get(tg_to_blkg(tg));

        if (tg_may_dispatch_weight(tg))
                list_add_tail(&tg->node, &td->active);
        else
                list_add_tail(&tg->node, &td->expired);

}

static struct bio *throtl_pop_queued_weight(struct throtl_grp *tg)
{
        struct bio *bio;

        bio = bio_list_pop(&tg->bios);
        /*lint -save -e727 -e730*/
        WARN_ON_ONCE(!bio);
        /*lint -restore*/

        throtl_pop_bio_weight_trace(bio, tg);
        return bio;
}

static void blk_throtl_weight_io_done(struct bio *bio)
{
        struct throtl_grp *tg;
        struct throtl_data *td;
        int index = (int)bio_data_dir(bio);
        int inflights;

        tg = (struct throtl_grp *)bio->bi_throtl_private2;
        td = tg->td;

        atomic_dec(&tg->inflights[index]);
        inflights = atomic_dec_return(&td->inflights);
        smp_mb__after_atomic();
        if (inflights >= td->quantum)
                goto out;
        else if (inflights == 0 && waitqueue_active(&td->waitq))
                wake_up(&td->waitq);

        if (td->nr_queued[0])
                queue_work(kthrotld_workqueue, &td->dispatch_work);
out:
        bio->bi_throtl_private2 = NULL;
        bio->bi_throtl_end_io2 = NULL;
        blkg_put(tg_to_blkg(tg));
}

static void tg_dispatch_one_bio_weight(struct throtl_grp *tg,
                                       struct bio_list *bios, bool charge)
{
        struct bio *bio;

        /*
         * @bio is being transferred from @tg to @parent_sq.  Popping a bio
         * from @tg may put its reference and @parent_sq might end up
         * getting released prematurely.  Remember the tg to put and put it
         * after @bio is transferred to @parent_sq.
         */
        bio = throtl_pop_queued_weight(tg);
        tg->service_queue.nr_queued[0]--;

        /*lint -save -e730*/
        if (unlikely(!charge))
        /*lint -restore*/
                goto skip_charge;

        throtl_charge_bio(tg, bio);

        if (!bio->bi_throtl_private2) {
                int index = (int)bio_data_dir(bio);

                atomic_inc(&tg->inflights[index]);
                /* Don't reorder td->nr_queued and inflights */
                smp_mb__before_atomic();
                atomic_inc(&tg->td->inflights);
                smp_mb__after_atomic();

                bio->bi_throtl_private2 = tg;

                /* We might access tg when end io, so grab it */
                blkg_get(tg_to_blkg(tg));
                bio->bi_throtl_end_io2 = blk_throtl_weight_io_done;
        }

skip_charge:
        /*lint -save -e730 -e775*/
        BUG_ON(tg->td->nr_queued[0] <= 0);
        /*lint -restore*/
        tg->td->nr_queued[0]--;

        bio_list_add(bios, bio);

#ifdef CONFIG_HUAWEI_IO_TRACING
        trace_block_throttle_dispatch(bio, tg_to_blkg(tg)->weight);
        trace_block_throttle_iocost(tg->io_cost.bps[0],
                tg->io_cost.iops[0], tg->io_cost.bytes_disp[0],
                tg->io_cost.io_disp[0]);
#endif
}

static int throtl_dispatch_tg_weight(struct throtl_grp *tg,
                                     struct bio_list *bios)
{
        int nr = 0;

        /*lint -save -e730*/
        BUG_ON(!td_weight_based(tg->td));
        /*lint -restore*/

        while (!bio_list_empty(&tg->bios) &&
               tg_may_dispatch_weight(tg)) {

                tg_dispatch_one_bio_weight(tg, bios, (bool)true);
                nr++;

                if (nr >= tg->quantum)
                        break;
        }

        if (bio_list_empty(&tg->bios) && !(tg->flags & THROTL_TG_ONLINE)) {
                list_del_init(&tg->node);
                blkg_put(tg_to_blkg(tg));
        } else if (!tg_may_dispatch_weight(tg)) {
                list_move_tail(&tg->node, &tg->td->expired);
        }

        return nr;
}

static void throtl_start_new_slice_weight(struct throtl_data *td)
{
        struct throtl_grp *tg;

        /*lint -save -e64 -e826*/
        list_for_each_entry(tg, &td->expired, node) {
        /*lint -restore*/
		tg->expired = false;
		if (tg->td->mode == MODE_WEIGHT_BANDWIDTH) {
			tg->io_cost.bytes_disp[0] = 0;
			wake_up_all(&tg->wait);
		} else {
			wake_up_nr(&tg->wait, tg->io_cost.iops[0]);
			tg->io_cost.io_disp[0] = 0;
		}
        }

        list_splice_tail_init(&td->expired, &td->active);
}

static bool should_start_new_slice(struct throtl_data *td)
{
        struct throtl_grp *tg;
        unsigned long interval;

        interval = msecs_to_jiffies(THROTL_IDLE_INTERVAL / 2);

        /*lint -save -e64 -e550 -e774 -e826*/
        list_for_each_entry(tg, &td->expired, node) {
		if (tg_to_blkg(tg)->blkcg->type <= BLK_THROTL_FG)
                        return true;
        }

        list_for_each_entry(tg, &td->active, node) {
                if (time_before(jiffies, tg->intime + interval))
                        return false;
        }
        /*lint -restore*/

        return true;
}

static void blk_run_throtl_bios_weight(struct throtl_data *td,
                                       struct throtl_grp *tg,
                                       struct bio_list *bios)
{
        struct throtl_grp *curr, *next;

        if (tg)
                throtl_dispatch_tg_weight(tg, bios);

        if (list_empty(&td->active) && bio_list_empty(bios))
                goto start_new_slice;

        /*lint -save -e64 -e530 -e826*/
        list_for_each_entry_safe(curr, next, &td->active, node) {
        /*lint -restore*/
                if (curr == tg)
                        continue;
                throtl_dispatch_tg_weight(curr, bios);
        }

        if (!bio_list_empty(bios))
                return;

        /*
         * If no bios in active queues and many bios are pending in
         * the device queue, just idle.
         */
start_new_slice:
        if (!should_start_new_slice(td)) {
                unsigned int interval = THROTL_IDLE_INTERVAL / 2;

                mod_timer(&td->idle_timer,
                          jiffies + msecs_to_jiffies(interval));
                return;
        }

        throtl_start_new_slice_weight(td);

        /*lint -save -e64 -e826*/
        list_for_each_entry_safe(curr, next, &td->active, node)
                throtl_dispatch_tg_weight(curr, bios);
        /*lint -restore*/
}

static void blk_throtl_kick_queue_fn(struct work_struct *work)
{
        /*lint -save -e826*/
        struct throtl_data *td = container_of(work, struct throtl_data,
                                              dispatch_work);
        /*lint -restore*/
        struct request_queue *q = td->queue;
        struct bio_list bio_list_on_stack;
        struct bio *bio;
        struct blk_plug plug;

        bio_list_init(&bio_list_on_stack);

        spin_lock_irq(q->queue_lock);
        blk_run_throtl_bios_weight(td, NULL, &bio_list_on_stack);
        spin_unlock_irq(q->queue_lock);

        if (!bio_list_empty(&bio_list_on_stack)) {
                blk_start_plug(&plug);
                /*lint -save -e820*/
                while((bio = bio_list_pop(&bio_list_on_stack)))
                /*lint -restore*/
                        generic_make_request(bio);
                blk_finish_plug(&plug);
        }
}

static void blk_throtl_rescue_timer_fn(unsigned long arg)
{
        struct throtl_data *td = (struct throtl_data *)arg;
        struct request_queue *q = td->queue;
        unsigned long flags;

        /*lint -save -e50 -e438 -e529 -e550 -e1058*/
        if (ACCESS_ONCE(td->nr_queued[0])) {
                smp_mb__after_atomic();
                if (!atomic_read(&td->inflights))
                        queue_work(kthrotld_workqueue, &td->dispatch_work);

                mod_timer(&td->rescue_timer,
                          jiffies + msecs_to_jiffies(THROTL_IDLE_INTERVAL));
                return;
        }

        spin_lock_irqsave(q->queue_lock, flags);
        if (ACCESS_ONCE(td->nr_queued[0])) {
                smp_mb__after_atomic();
                if (!atomic_read(&td->inflights))
                        queue_work(kthrotld_workqueue, &td->dispatch_work);

                mod_timer(&td->rescue_timer,
                          jiffies + msecs_to_jiffies(THROTL_IDLE_INTERVAL));
        }
        spin_unlock_irqrestore(q->queue_lock, flags);
        /*lint -restore*/
}

static void blk_throtl_idle_timer_fn(unsigned long arg)
{
        struct throtl_data *td = (struct throtl_data *)arg;
        struct request_queue *q = td->queue;
        unsigned long flags;

        /*lint -save -e550*/
        spin_lock_irqsave(q->queue_lock, flags);
        /*lint -restore*/
        if (td->nr_queued[0])
                queue_work(kthrotld_workqueue, &td->dispatch_work);
        spin_unlock_irqrestore(q->queue_lock, flags);
}

static void tg_drain_bios_weight(struct throtl_grp *tg, struct bio_list *bios)
{
        /*lint -save -e730 -e747*/
        WARN_ON(list_empty(&tg->node));

        list_del_init(&tg->node);

        while (!bio_list_empty(&tg->bios))
                tg_dispatch_one_bio_weight(tg, bios, false);
        /*lint -restore*/

        tg->io_cost.bytes_disp[0] = 0;
        tg->io_cost.io_disp[0] = 0;

        blkg_put(tg_to_blkg(tg));
}

static void blk_throtl_drain_weight(struct request_queue *q)
{
        struct throtl_data *td = q->td;
        struct throtl_grp *curr, *next;
        struct bio *bio;
        struct bio_list bios;

        bio_list_init(&bios);

        /*lint -save -e64 -e530 -e826*/
        list_for_each_entry_safe(curr, next, &td->active, node)
                tg_drain_bios_weight(curr, &bios);

        list_for_each_entry_safe(curr, next, &td->expired, node)
                tg_drain_bios_weight(curr, &bios);
        /*lint -restore*/

        spin_unlock_irq(q->queue_lock);

        /*lint -save -e820*/
        while((bio = bio_list_pop(&bios)))
                generic_make_request(bio);
        /*lint -restore*/

        spin_lock_irq(q->queue_lock);
}

static void tg_drain_node_weight(struct throtl_grp *tg)
{
	WARN_ON(list_empty(&tg->node));

	list_del_init(&tg->node);
	tg->io_cost.bytes_disp[0] = 0;
	tg->io_cost.io_disp[0] = 0;
	tg->expired = false;

	wake_up_all(&tg->wait);

	blkg_put(tg_to_blkg(tg));
}

static void __blk_throtl_drain_weight(struct request_queue *q)
{
	struct throtl_data *td = q->td;
	struct throtl_grp *curr, *next;

	list_for_each_entry_safe(curr, next, &td->active, node)
		tg_drain_node_weight(curr);

	list_for_each_entry_safe(curr, next, &td->expired, node)
		tg_drain_node_weight(curr);
}

static bool atomic_inc_below(atomic_t *v, int below)
{
        /*lint -save -e438 -e529*/
        int cur = atomic_read(v);
        /*lint -restore*/

        for (;;) {
                int old;

                if (cur >= below)
                        return false;
		/*lint -save -e571*/
                old = atomic_cmpxchg(v, cur, cur + 1);
                if (old == cur)
                        break;
		/*lint -restore*/
                cur = old;
        }

        return true;
}

static bool io_event_ok(struct throtl_io_limit *io_limit, int limit)
{
        if (limit == 0)
                limit = INT_MAX;

        return atomic_inc_below(&io_limit->inflights, limit);
}

void blk_throtl_update_limit(struct blk_throtl_io_limit *io_limit,
                             int limit)
{
        io_limit->max_inflights = limit;
        wake_up_all(&io_limit->io_limits[0].waitq);
        wake_up_all(&io_limit->io_limits[1].waitq);
}

static void __throtl_limit_io_done(struct throtl_io_limit *io_limit,
                                   int limit)
{
        int inflights;

        inflights = atomic_dec_return(&io_limit->inflights);
        if (inflights >= limit) {
                if (!limit && waitqueue_active(&io_limit->waitq))
                        wake_up_all(&io_limit->waitq);
                return;
        }

        if (waitqueue_active(&io_limit->waitq)) {
                int diff = max(limit - inflights, 1);

                wake_up_nr(&io_limit->waitq, diff);
        }
}

static void blk_throtl_limit_io_done(struct bio *bio)
{
        struct blk_throtl_io_limit *io_limit;
        int limit;
        int index = (int)bio_data_dir(bio);

        io_limit = (struct blk_throtl_io_limit *)bio->bi_throtl_private1;
        /*lint -save -e438 -e529*/
        limit = ACCESS_ONCE(io_limit->max_inflights);
        /*lint -restore*/
        __throtl_limit_io_done(&io_limit->io_limits[index], limit);

        bio->bi_throtl_private1 = NULL;
        bio->bi_throtl_end_io1 = NULL;
        blk_throtl_io_limit_put(io_limit);
}

/* We must hold rcu lock. */
static int blk_throtl_io_limit_wait(struct blkcg *blkcg,
                                    struct blk_throtl_io_limit *task_limit,
                                    struct throtl_grp *tg,
                                    struct bio *bio)
{
        /*lint -save -e438 -e446 -e529 -e727 -e730*/
        struct blkcg *new;
        DEFINE_WAIT(wait);
        int limit;
        int index = (int)bio_data_dir(bio);
        struct throtl_io_limit *io_limit = &task_limit->io_limits[index];
        int ret;

        WARN_ON_ONCE(!rcu_read_lock_held());

        limit = ACCESS_ONCE(task_limit->max_inflights);
        /*lint -restore*/
        if (io_event_ok(io_limit, limit))
                return 0;

        ret = 0;
        for (;;) {
                prepare_to_wait(&io_limit->waitq, &wait, TASK_UNINTERRUPTIBLE);
                /*lint -save -e438 -e529*/
                limit = ACCESS_ONCE(task_limit->max_inflights);
                /*lint -restore*/

                new = bio_blkcg(bio);
                if (new != blkcg) {
                        ret = 1;
                        break;
                }

		if (io_event_ok(io_limit, limit))
			break;

                if (!tg->td->max_inflights) {
                        ret = 2;
                        break;
                }

                task_set_sleep_on_throtl(current);
                rcu_read_unlock();
#ifdef CONFIG_HUAWEI_IO_TRACING
                trace_block_throttle_limit_start(bio, tg->td->max_inflights,
                                                 tg->inflights[bio_data_dir(bio)]);
#endif
                io_schedule();
#ifdef CONFIG_HUAWEI_IO_TRACING
                trace_block_throttle_limit_end(bio);
#endif
                rcu_read_lock();
                task_clear_sleep_on_throtl(current);
        }
        finish_wait(&io_limit->waitq, &wait);
        return ret;
}

static int tg_print_cgroup_type(struct seq_file *sf, void *v)
{
	struct blkcg *blkcg = css_to_blkcg(seq_css(sf));

	seq_printf(sf, "type: %d\n", blkcg->type);

	return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_set_cgroup_type(struct cgroup_subsys_state *css,
				struct cftype *cft, u64 val)
{
	struct blkcg *blkcg = css_to_blkcg(css);
	unsigned int type = (unsigned int)val;

	if (type >= BLK_THROTL_TYPE_NR)
		return -EINVAL;

	blkcg->type = type;
	return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static u64 tg_prfill_conf_u64(struct seq_file *sf, struct blkg_policy_data *pd,
			      int off)
{
	struct throtl_grp *tg = pd_to_tg(pd);
	/*lint -save -e124*/
	u64 v = *(u64 *)((void *)tg + off);
	/*lint -restore*/

	/*lint -save -e650 -e737 -e747*/
	if (v == -1)
		return 0;
	return __blkg_prfill_u64(sf, pd, v);
	/*lint -restore*/
}

static u64 tg_prfill_conf_uint(struct seq_file *sf, struct blkg_policy_data *pd,
			       int off)
{
	struct throtl_grp *tg = pd_to_tg(pd);
	/*lint -save -e124*/
	unsigned int v = *(unsigned int *)((void *)tg + off);
	/*lint -restore*/

	/*lint -save -e650 -e737 -e747*/
	if (v == -1)
		return 0;
	return __blkg_prfill_u64(sf, pd, v);
	/*lint -restore*/
}

static int tg_print_conf_u64(struct seq_file *sf, void *v)
{
	/*lint -save -e747*/
	blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)), tg_prfill_conf_u64,
			  &blkcg_policy_throtl, seq_cft(sf)->private, false);
	/*lint -restore*/
	return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_conf_uint(struct seq_file *sf, void *v)
{
	/*lint -save -e747*/
	blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)), tg_prfill_conf_uint,
			  &blkcg_policy_throtl, seq_cft(sf)->private, false);
	 /*lint -restore*/
	return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static void tg_conf_updated(struct throtl_grp *tg)
{
	struct throtl_service_queue *sq = &tg->service_queue;
	struct cgroup_subsys_state *pos_css;
	struct blkcg_gq *blkg;

	throtl_log(&tg->service_queue,
		   "limit change rbps=%llu wbps=%llu riops=%u wiops=%u",
		   tg->io_cost.bps[READ], tg->io_cost.bps[WRITE],
		   tg->io_cost.iops[READ], tg->io_cost.iops[WRITE]);

	/*
	 * Update has_rules[] flags for the updated tg's subtree.  A tg is
	 * considered to have rules if either the tg itself or any of its
	 * ancestors has rules.  This identifies groups without any
	 * restrictions in the whole hierarchy and allows them to bypass
	 * blk-throttle.
	 */
	/*lint -save -e666*/
	blkg_for_each_descendant_pre(blkg, pos_css, tg_to_blkg(tg))
		tg_update_has_rules(blkg_to_tg(blkg));
	/*lint -restore*/

	/*
	 * We're already holding queue_lock and know @tg is valid.  Let's
	 * apply the new config directly.
	 *
	 * Restart the slices for both READ and WRITES. It might happen
	 * that a group's limit are dropped suddenly and we don't want to
	 * account recently dispatched IO with new low rate.
	 */
	throtl_start_new_slice(tg, 0);
	throtl_start_new_slice(tg, 1);

	if (tg->flags & THROTL_TG_PENDING) {
		tg_update_disptime(tg);
		throtl_schedule_next_dispatch(sq->parent_sq, true);
	}
}

static ssize_t tg_set_conf(struct kernfs_open_file *of,
			   char *buf, size_t nbytes, loff_t off, bool is_u64)
{
	struct blkcg *blkcg = css_to_blkcg(of_css(of));
	struct blkg_conf_ctx ctx;
	struct throtl_grp *tg;
	int ret;
	u64 v;
	/*lint -save -e570*/
	ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
	if (ret)
		return ret;

	ret = -EINVAL;
	if (sscanf(ctx.body, "%llu", &v) != 1)
		goto out_finish;
	if (!v)
		v = -1;

	tg = blkg_to_tg(ctx.blkg);
	if (td_weight_based(tg->td)) {
		ret = -EBUSY;
		goto out_finish;
	}
	/*lint -restore*/
	if (is_u64)
		*(u64 *)((void *)tg + of_cft(of)->private) = v;
	else
		*(unsigned int *)((void *)tg + of_cft(of)->private) = v;

	tg->td->mode = MODE_THROTTLE;
	tg_conf_updated(tg);
	ret = 0;
out_finish:
	blkg_conf_finish(&ctx);
	return ret ?: nbytes;
/*lint -save -e715*/
}
/*lint -restore*/

static ssize_t tg_set_conf_u64(struct kernfs_open_file *of,
			       char *buf, size_t nbytes, loff_t off)
{
	/*lint -save -e747*/
	return tg_set_conf(of, buf, nbytes, off, true);
	/*lint -restore*/
}

static ssize_t tg_set_conf_uint(struct kernfs_open_file *of,
				char *buf, size_t nbytes, loff_t off)
{
	/*lint -save -e747*/
	return tg_set_conf(of, buf, nbytes, off, false);
	/*lint -restore*/
}

static u64 tg_prfill_mode_device(struct seq_file *sf,
        struct blkg_policy_data *pd, int off)
{
        struct throtl_grp *tg = pd_to_tg(pd);
        const char *dname = blkg_dev_name(pd->blkg);

        if (!dname)
                return 0;
        if (tg->td->mode == MODE_NONE)
                return 0;
        seq_printf(sf, "%s %s\n", dname, run_mode_name[tg->td->mode]);
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int throtl_print_mode_device(struct seq_file *sf, void *v)
{
        int i;

        seq_printf(sf, "available ");
        for (i = 0; i < MAX_MODE; i++)
                seq_printf(sf, "%s(%d) ", run_mode_name[i], i);
        seq_printf(sf, "\n");
        seq_printf(sf, "default %s\n", run_mode_name[MODE_NONE]);

        mutex_lock(&throtl_mode_lock);
        /*lint -save -e747*/
        blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
                tg_prfill_mode_device,  &blkcg_policy_throtl, 0, false);
        /*lint -restore*/
        mutex_unlock(&throtl_mode_lock);
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static void tg_update_rules(struct throtl_data *td)
{
        struct cgroup_subsys_state *pos_css;
        struct blkcg_gq *blkg, *top_blkg;

        top_blkg = td->queue->root_blkg;

        /*lint -save -e747 -e820*/
        blkg_for_each_descendant_pre(blkg, pos_css, top_blkg)
                tg_update_has_rules(blkg_to_tg(blkg));
        /*lint -restore*/
}

static void tg_change_mode(struct throtl_data *td, int mode)
{
        struct request_queue *q = td->queue;

        blk_queue_bypass_start(q);

        /*
         * Maybe it is unnecessary, but we need make sure all bios in the
         * block throttle layer are drained.
         */
        spin_lock_irq(q->queue_lock);
        blkcg_drain_queue(q);
        spin_unlock_irq(q->queue_lock);

        /*lint -save -e40 -e64 -e438 -e529 -e666 -e730 -e774 -e845*/
        WARN_ON(td->nr_queued[0]);
        WARN_ON(td->nr_queued[1]);

        wait_event(td->waitq, !atomic_read(&td->inflights));

        del_timer_sync(&td->rescue_timer);
        del_timer_sync(&td->idle_timer);
        flush_work(&td->dispatch_work);

        spin_lock_irq(q->queue_lock);
        td->mode = mode;
        /*lint -restore*/

        if (td_weight_based(td))
                INIT_WORK(&td->dispatch_work, blk_throtl_kick_queue_fn);
        else
                INIT_WORK(&td->dispatch_work, blk_throtl_dispatch_work_fn);

        rcu_read_lock();
        tg_update_share(td, NULL);
        tg_update_rules(td);
        rcu_read_unlock();
        spin_unlock_irq(q->queue_lock);

        blk_queue_bypass_end(q);
}

static ssize_t tg_set_mode_device(struct kernfs_open_file *of,
                                  char *buf, size_t nbytes, loff_t off)
{
        struct blkcg *blkcg = css_to_blkcg(of_css(of));
        struct blkg_conf_ctx ctx;
        struct throtl_grp *tg;
        struct throtl_data *td;
        struct gendisk *disk;
        int ret;
        int mode;
        bool need_change = false;

        mutex_lock(&throtl_mode_lock);
        ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
        if (ret)
                goto out;

	if (sscanf(ctx.body, "%d", &mode) != 1)
		goto out_finish;

        ret = -EINVAL;

        if (mode < 0 || mode >= MAX_MODE)
                goto out_finish;

        tg = blkg_to_tg(ctx.blkg);
        if (tg->td->mode == mode) {
                ret = 0;
                goto out_finish;
        }

        disk = ctx.disk;
        if (!get_disk(disk)) {
                ret = -EBUSY;
                goto out_finish;
        }

        td = tg->td;
        need_change = true;
        ret = 0;

out_finish:
        blkg_conf_finish(&ctx);

        /*lint -save -e644 -e713 -e737*/
        if (need_change) {
                tg_change_mode(td, mode);
                put_disk(disk);
        }
out:
        mutex_unlock(&throtl_mode_lock);
        return ret ?: nbytes;
        /*lint -restore*/
/*lint -save -e715*/
}
/*lint -restore*/

static u64 tg_prfill_weight_device(struct seq_file *sf,
                struct blkg_policy_data *pd, int off)
{
        struct blkcg_gq *blkg = pd_to_blkg(pd);
        const char *dname = blkg_dev_name(pd->blkg);

        if (!dname)
                return 0;
        if (!blkg)
                return 0;
        if (blkg->weight == blkg->blkcg->weight)
                return 0;
        seq_printf(sf, "%s: weight %u\n", dname, blkg->weight);
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_weight(struct seq_file *sf, void *v)
{
        struct blkcg *blkcg = css_to_blkcg(seq_css(sf));

        seq_printf(sf, "default: %u\n", blkcg->weight);
        /*lint -save -e747*/
        blkcg_print_blkgs(sf, blkcg, tg_prfill_weight_device,
                          &blkcg_policy_throtl, 0, false);
        /*lint -restore*/
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_weight_device(struct seq_file *sf, void *v)
{
        /*lint -save -e747*/
        blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
                          tg_prfill_weight_device,
                          &blkcg_policy_throtl, 0, false);
        /*lint -restore*/
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static ssize_t tg_set_weight_device(struct kernfs_open_file *of,
                           char *buf, size_t nbytes, loff_t off)
{
        struct blkcg *blkcg = css_to_blkcg(of_css(of));
        struct throtl_grp *tg;
        struct throtl_service_queue *parent_sq;
        struct blkg_conf_ctx ctx;
        unsigned int val;
        int ret;

        ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
        if (ret)
                return ret;

        ret = -EINVAL;
	if (sscanf(ctx.body, "%d", &val) != 1)
		goto out_finish;

        if (val < BLKIO_WEIGHT_MIN || val > BLKIO_WEIGHT_MAX)
                goto out_finish;

        tg = blkg_to_tg(ctx.blkg);
        parent_sq = tg->service_queue.parent_sq;
        if (parent_sq) {
                parent_sq->children_weight -= ctx.blkg->weight;
                parent_sq->children_weight += val;
        }

        ctx.blkg->weight = val;

        tg_update_share(tg->td, tg);

        ret = 0;
out_finish:
        blkg_conf_finish(&ctx);
        /*lint -save -e713 -e737*/
        return ret ?: nbytes;
        /*lint -restore*/
/*lint -save -e715*/
}
/*lint -restore*/

static ssize_t tg_set_weight_offon(struct kernfs_open_file *of,
                                   char *buf, size_t nbytes,
                                   loff_t off)
{
        int ret;
	char offon[8];

	if (sscanf(buf, "%7s", offon) != 1)
                return -EINVAL;

        ret = 0;
	if (strnlen(offon, (size_t)7) == 5 &&
	    !strncmp(offon, "on-fs", (size_t)6))
		blk_throtl_weight_offon = BLK_THROTL_WEIGHT_ON_FS;
	else if (strnlen(offon, (size_t)7) == 6 &&
	    !strncmp(offon, "on-blk", (size_t)7))
		blk_throtl_weight_offon = BLK_THROTL_WEIGHT_ON_BLK;
        else if (strnlen(offon, (size_t)4) == 3 &&
                 !strncmp(offon, "off", (size_t)4))
		blk_throtl_weight_offon = BLK_THROTL_WEIGHT_OFF;
        else
                ret = -EINVAL;

        /*lint -save -e713 -e737*/
        return ret ?: nbytes;
        /*lint -restore*/
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_weight_offon(struct seq_file *sf, void *v)
{
        seq_printf(sf, "Weight Based Throttle: %s\n",
		   (blk_throtl_weight_offon == BLK_THROTL_WEIGHT_ON_FS) ?
		    "on-fs" :
		    ((blk_throtl_weight_offon == BLK_THROTL_WEIGHT_ON_BLK) ?
		     "on-blk" : "off"));
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static u64 tg_prfill_max_inflights_device(struct seq_file *sf,
                                          struct blkg_policy_data *pd, int off)
{
        struct throtl_grp *tg = pd_to_tg(pd);
        const char *dname = blkg_dev_name(pd->blkg);

        if (!dname)
                return 0;

        /*lint -save -e438 -e529*/
        seq_printf(sf, "%s: queued %u, read inflight %d, write inflight %d, devices inflights %d delay count %d, max delay %d\n\n",
                   dname, tg->service_queue.nr_queued[0],
                   atomic_read(&tg->inflights[0]),
                   atomic_read(&tg->inflights[1]),
                   atomic_read(&tg->td->inflights),
                   atomic_read(&tg->delay_cnt),
                   atomic_read(&tg->max_delay));
        /*lint -restore*/

        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_max_inflights(struct seq_file *sf, void *v)
{
        struct blkcg *blkcg = css_to_blkcg(seq_css(sf));

        seq_printf(sf, "max limit: %d\n", blkcg->max_inflights);

        /*lint -save -e747*/
        blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
                          tg_prfill_max_inflights_device,
                          &blkcg_policy_throtl, 0, false);
        /*lint -restore*/

        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_set_max_inflights(struct cgroup_subsys_state *css,
                                struct cftype *cft, s64 val)
{
        struct blkcg *blkcg = css_to_blkcg(css);
        struct task_struct *task;
        struct css_task_iter it;
        int max_inflights = (int)val;

        if (max_inflights < MIN_INFLIGHT || max_inflights > MAX_INFLIGHT)
                return -EINVAL;

        spin_lock_irq(&blkcg->lock);
        if (blkcg->max_inflights == max_inflights)
                goto out_unlock;

        blkcg->max_inflights = max_inflights;

        rcu_read_lock();
        css_task_iter_start(css, &it);
        /*lint -save -e820*/
        while ((task = css_task_iter_next(&it))) {
        /*lint -restore*/
                if (!task->mm)
                        continue;

                if (!task->mm->io_limit)
                        continue;

                blk_throtl_update_limit(task->mm->io_limit, max_inflights);
        }
        css_task_iter_end(&it);
        rcu_read_unlock();
out_unlock:
        spin_unlock_irq(&blkcg->lock);
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static u64 tg_prfill_enable_max_inflights_device(struct seq_file *sf,
                struct blkg_policy_data *pd, int off)
{
        struct throtl_grp *tg = pd_to_tg(pd);
        const char *dname = blkg_dev_name(pd->blkg);

        if (!dname)
                return 0;

        seq_printf(sf, "%s: limit %s\n",
                   dname, (tg->td->max_inflights ? "open" : "close"));
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_enable_max_inflight_device(struct seq_file *sf, void *v)
{
        /*lint -save -e747*/
        blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
                          tg_prfill_enable_max_inflights_device,
                          &blkcg_policy_throtl, 0, false);
        /*lint -restore*/
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static ssize_t tg_set_enable_max_inflights_device(struct kernfs_open_file *of,
                           char *buf, size_t nbytes, loff_t off)
{
        struct cgroup_subsys_state *css = of_css(of);
        struct blkcg *blkcg = css_to_blkcg(css);
        struct blkcg_gq *blkg;
        struct blkg_conf_ctx ctx;
        struct throtl_grp *tg;
        struct task_struct *task;
        struct css_task_iter it;
        unsigned long flags;
        int max_inflight;
        int ret;

        ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
        if (ret)
                return ret;

        ret = -EINVAL;

	if (sscanf(ctx.body, "%d", &max_inflight) != 1)
		goto out_finish;

        if (max_inflight != 0 && max_inflight != 1)
                goto out_finish;

        ret = 0;
        tg = blkg_to_tg(ctx.blkg);
        if (tg->td->max_inflights == max_inflight)
                goto out_finish;

        /*lint -save -e550*/
        spin_lock_irqsave(&blkcg->lock, flags);
        /*lint -restore*/

        if (!max_inflight)
                goto skip_check;

        /* we just open only one device for limit */
        /*lint -save -e62 -e64 -e826*/
        hlist_for_each_entry(blkg, &blkcg->blkg_list, blkcg_node) {
        /*lint -restore*/
                struct throtl_grp *curr_tg = blkg_to_tg(blkg);

                if (!curr_tg || curr_tg == tg)
                        continue;

                if (curr_tg->td->max_inflights) {
                        ret = -EBUSY;
                        goto out_unlock;
                }
        }
skip_check:
        tg->td->max_inflights = max_inflight;
        smp_mb();
        if (max_inflight)
                goto out_unlock;

        rcu_read_lock();

        css_task_iter_start(css, &it);
        /*lint -save -e820*/
        while ((task = css_task_iter_next(&it))) {
        /*lint -restore*/
                if (task_sleep_on_throtl(task))
                        wake_up_process(task);
        }
        css_task_iter_end(&it);

        rcu_read_unlock();
out_unlock:
        spin_unlock_irqrestore(&blkcg->lock, flags);
out_finish:
        blkg_conf_finish(&ctx);
        /*lint -save -e713 -e737*/
        return ret ?: nbytes;
        /*lint -restore*/
/*lint -save -e715*/
}
/*lint -restore*/

static u64 tg_prfill_quantum_device(struct seq_file *sf,
                                    struct blkg_policy_data *pd, int off)
{
        struct blkcg_gq *blkg = pd_to_blkg(pd);
        const char *dname = blkg_dev_name(pd->blkg);

        if (!dname)
                return 0;

        if (!blkg)
                return 0;

        if ((blkg_to_tg(blkg)->td->quantum) == QUANTUM_INQUEUE_DEF)
                return 0;

        seq_printf(sf, "%s: quantum %u\n", dname,
                   blkg_to_tg(blkg)->td->quantum);
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_quantum_device(struct seq_file *sf, void *v)
{
        seq_printf(sf, "default: %d\n", QUANTUM_INQUEUE_DEF);

        /*lint -save -e747*/
        blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
                          tg_prfill_quantum_device,
                          &blkcg_policy_throtl, 0, false);
        /*lint -restore*/
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static ssize_t tg_set_quantum_device(struct kernfs_open_file *of,
                                     char *buf, size_t nbytes, loff_t off)
{
        struct blkcg *blkcg = css_to_blkcg(of_css(of));
        struct throtl_grp *tg;
        struct blkg_conf_ctx ctx;
        int val;
        int ret;

        ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
        if (ret)
                return ret;

        ret = -EINVAL;
	if (sscanf(ctx.body, "%d", &val) != 1)
		goto out_finish;

        if (val < QUANTUM_INQUEUE_MIN || val > QUANTUM_INQUEUE_MAX)
                goto out_finish;

        tg = blkg_to_tg(ctx.blkg);
        tg->td->quantum = val;

        ret = 0;
out_finish:
        blkg_conf_finish(&ctx);
        /*lint -save -e713 -e737*/
        return ret ?: nbytes;
        /*lint -restore*/
/*lint -save -e715*/
}
/*lint -restore*/

static u64 tg_prfill_bw_slice_device(struct seq_file *sf,
                                     struct blkg_policy_data *pd, int off)
{
        struct blkcg_gq *blkg = pd_to_blkg(pd);
        const char *dname = blkg_dev_name(pd->blkg);

        if (!dname)
                return 0;

        if (!blkg)
                return 0;

        if ((blkg_to_tg(blkg)->td->bw_slice) == BW_SLICE_DEF)
                return 0;

        seq_printf(sf, "%s: bw_slice %llu\n", dname,
                   blkg_to_tg(blkg)->td->bw_slice);
         return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_bw_slice_device(struct seq_file *sf, void *v)
{
        seq_printf(sf, "default: %llu\n", BW_SLICE_DEF);

        /*lint -save -e747*/
        blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
                          tg_prfill_bw_slice_device,
                          &blkcg_policy_throtl, 0, false);
        /*lint -restore*/
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static ssize_t tg_set_bw_slice_device(struct kernfs_open_file *of,
                                      char *buf, size_t nbytes, loff_t off)
{
        struct blkcg *blkcg = css_to_blkcg(of_css(of));
        struct throtl_grp *tg;
        struct blkg_conf_ctx ctx;
        uint64_t val;
        int ret;
	/*lint -save -e561*/
        ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
        if (ret)
                return ret;

        ret = -EINVAL;
	if (sscanf(ctx.body, "%llu", &val) != 1)
		goto out_finish;

        if (val < BW_SLICE_MIN || val > BW_SLICE_MAX)
                goto out_finish;

        tg = blkg_to_tg(ctx.blkg);

        tg->td->bw_slice = val;

        tg_update_share(tg->td, NULL);
	/*lint -restore*/
        ret = 0;
out_finish:
        blkg_conf_finish(&ctx);
        /*lint -save -e713 -e737*/
        return ret ?: nbytes;
        /*lint -restore*/
/*lint -save -e715*/
}
/*lint -restore*/

static u64 tg_prfill_iops_slice_device(struct seq_file *sf,
                                       struct blkg_policy_data *pd,
                                       int off)
{
        struct blkcg_gq *blkg = pd_to_blkg(pd);
        const char *dname = blkg_dev_name(pd->blkg);

        if (!dname)
                return 0;

        if (!blkg)
                return 0;

        if (blkg_to_tg(blkg)->td->iops_slice == IOPS_SLICE_DEF)
                return 0;

        seq_printf(sf, "%s: iops_slice %u\n", dname,
                   blkg_to_tg(blkg)->td->iops_slice);
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static int tg_print_iops_slice_device(struct seq_file *sf, void *v)
{
        seq_printf(sf, "default: %d\n", IOPS_SLICE_DEF);

        /*lint -save -e747*/
        blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)),
                          tg_prfill_iops_slice_device,
                          &blkcg_policy_throtl, 0, false);
        /*lint -restore*/
        return 0;
/*lint -save -e715*/
}
/*lint -restore*/

static ssize_t tg_set_iops_slice_device(struct kernfs_open_file *of,
                           char *buf, size_t nbytes, loff_t off)
{
        struct blkcg *blkcg = css_to_blkcg(of_css(of));
        struct throtl_grp *tg;
        struct blkg_conf_ctx ctx;
        unsigned int val;
        int ret;

        ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
        if (ret)
                return ret;

        ret = -EINVAL;

	if (sscanf(ctx.body, "%d", &val) != 1)
		goto out_finish;

        if (val < IOPS_SLICE_MIN || val > IOPS_SLICE_MAX)
                goto out_finish;

        tg = blkg_to_tg(ctx.blkg);

        tg->td->iops_slice = val;

        tg_update_share(tg->td, NULL);

        ret = 0;
out_finish:
        blkg_conf_finish(&ctx);
        /*lint -save -e713 -e737*/
        return ret ?: nbytes;
        /*lint -restore*/
/*lint -save -e715*/
}
/*lint -restore*/

/*lint -save -e785*/
static struct cftype throtl_legacy_files[] = {
        {
                .name = "throttle.mode_device",
                .flags = CFTYPE_ONLY_ON_ROOT,
                .seq_show = throtl_print_mode_device,
                .write = tg_set_mode_device,
        },
	{
		.name = "throttle.read_bps_device",
		.private = offsetof(struct throtl_grp, io_cost.bps[READ]),
		.seq_show = tg_print_conf_u64,
		.write = tg_set_conf_u64,
	},
	{
		.name = "throttle.write_bps_device",
		.private = offsetof(struct throtl_grp, io_cost.bps[WRITE]),
		.seq_show = tg_print_conf_u64,
		.write = tg_set_conf_u64,
	},
	{
		.name = "throttle.read_iops_device",
		.private = offsetof(struct throtl_grp, io_cost.iops[READ]),
		.seq_show = tg_print_conf_uint,
		.write = tg_set_conf_uint,
	},
	{
		.name = "throttle.write_iops_device",
		.private = offsetof(struct throtl_grp, io_cost.iops[WRITE]),
		.seq_show = tg_print_conf_uint,
		.write = tg_set_conf_uint,
	},
	{
		.name = "throttle.io_service_bytes",
		.private = (unsigned long)&blkcg_policy_throtl,
		.seq_show = blkg_print_stat_bytes,
	},
	{
		.name = "throttle.io_serviced",
		.private = (unsigned long)&blkcg_policy_throtl,
		.seq_show = blkg_print_stat_ios,
	},
        {
                .name = "throttle.weight_device",
                .flags = CFTYPE_NOT_ON_ROOT,
                .seq_show = tg_print_weight_device,
                .write = tg_set_weight_device,
        },
        {
                .name = "throttle.weight",
                .flags = CFTYPE_NOT_ON_ROOT,
                .seq_show = tg_print_weight,
        },
        {
                .name = "throttle.weight_based_offon",
                .flags = CFTYPE_ONLY_ON_ROOT,
                .seq_show = tg_print_weight_offon,
                .write =  tg_set_weight_offon,
        },
        {
                .name = "throttle.max_inflights",
                .flags = CFTYPE_NOT_ON_ROOT,
                .seq_show = tg_print_max_inflights,
                .write_s64 = tg_set_max_inflights,
        },
        {
                .name = "throttle.enable_max_inflights_device",
                .flags = CFTYPE_ONLY_ON_ROOT,
                .seq_show = tg_print_enable_max_inflight_device,
                .write = tg_set_enable_max_inflights_device,
        },
        {
                .name = "throttle.quantum_device",
                .flags = CFTYPE_ONLY_ON_ROOT,
                .seq_show = tg_print_quantum_device,
                .write = tg_set_quantum_device,
        },
        {
                .name = "throttle.bw_slice_device",
                .flags = CFTYPE_ONLY_ON_ROOT,
                .seq_show = tg_print_bw_slice_device,
                .write = tg_set_bw_slice_device,
        },
        {
                .name = "throttle.iops_slice_device",
                .flags = CFTYPE_ONLY_ON_ROOT,
                .seq_show = tg_print_iops_slice_device,
                .write = tg_set_iops_slice_device,
        },
        {
		.name = "throttle.type",
		.seq_show = tg_print_cgroup_type,
		.write_u64 = tg_set_cgroup_type,
	},
	{ }	/* terminate */
};
/*lint -restore*/

static u64 tg_prfill_max(struct seq_file *sf, struct blkg_policy_data *pd,
			 int off)
{
	struct throtl_grp *tg = pd_to_tg(pd);
	const char *dname = blkg_dev_name(pd->blkg);
	char bufs[4][21] = { "max", "max", "max", "max" };
	/*lint -save -e650*/
	if (!dname)
		return 0;
	if (tg->io_cost.bps[READ] == -1 && tg->io_cost.bps[WRITE] == -1 &&
	    tg->io_cost.iops[READ] == -1 && tg->io_cost.iops[WRITE] == -1)
		return 0;

	if (tg->io_cost.bps[READ] != -1)
		snprintf(bufs[0], sizeof(bufs[0]), "%llu", tg->io_cost.bps[READ]);
	if (tg->io_cost.bps[WRITE] != -1)
		snprintf(bufs[1], sizeof(bufs[1]), "%llu", tg->io_cost.bps[WRITE]);
	if (tg->io_cost.iops[READ] != -1)
		snprintf(bufs[2], sizeof(bufs[2]), "%u", tg->io_cost.iops[READ]);
	if (tg->io_cost.iops[WRITE] != -1)
		snprintf(bufs[3], sizeof(bufs[3]), "%u", tg->io_cost.iops[WRITE]);
	/*lint -restore*/
	seq_printf(sf, "%s rbps=%s wbps=%s riops=%s wiops=%s\n",
		   dname, bufs[0], bufs[1], bufs[2], bufs[3]);
	return 0;
}

static int tg_print_max(struct seq_file *sf, void *v)
{
	blkcg_print_blkgs(sf, css_to_blkcg(seq_css(sf)), tg_prfill_max,
			  &blkcg_policy_throtl, seq_cft(sf)->private, false);
	return 0;
}

static ssize_t tg_set_max(struct kernfs_open_file *of,
			  char *buf, size_t nbytes, loff_t off)
{
	struct blkcg *blkcg = css_to_blkcg(of_css(of));
	struct blkg_conf_ctx ctx;
	struct throtl_grp *tg;
	u64 v[4];
	int ret;

	ret = blkg_conf_prep(blkcg, &blkcg_policy_throtl, buf, &ctx);
	if (ret)
		return ret;

	tg = blkg_to_tg(ctx.blkg);

	v[0] = tg->io_cost.bps[READ];
	v[1] = tg->io_cost.bps[WRITE];
	v[2] = tg->io_cost.iops[READ];
	v[3] = tg->io_cost.iops[WRITE];
	/*lint -save -e570*/
	while (true) {
		char tok[27];	/* wiops=18446744073709551616 */
		char *p;
		u64 val = -1;
		int len;
	/*lint -restore*/
		if (sscanf(ctx.body, "%26s%n", tok, &len) != 1)
			break;
		if (tok[0] == '\0')
			break;
		ctx.body += len;
		/*lint -save -e421*/
		ret = -EINVAL;
		p = tok;
		strsep(&p, "=");
		if (!p || (sscanf(p, "%llu", &val) != 1 && strcmp(p, "max")))
			goto out_finish;

		ret = -ERANGE;
		if (!val)
			goto out_finish;

		ret = -EINVAL;
		if (!strcmp(tok, "rbps"))
			v[0] = val;
		else if (!strcmp(tok, "wbps"))
			v[1] = val;
		else if (!strcmp(tok, "riops"))
			v[2] = min_t(u64, val, UINT_MAX);
		else if (!strcmp(tok, "wiops"))
			v[3] = min_t(u64, val, UINT_MAX);
		else
			goto out_finish;
	}
		 /*lint -restore*/
	tg->io_cost.bps[READ] = v[0];
	tg->io_cost.bps[WRITE] = v[1];
	tg->io_cost.iops[READ] = v[2];
	tg->io_cost.iops[WRITE] = v[3];

	tg_conf_updated(tg);
	ret = 0;
out_finish:
	blkg_conf_finish(&ctx);
	return ret ?: nbytes;
}

static struct cftype throtl_files[] = {
	{
		.name = "max",
		.flags = CFTYPE_NOT_ON_ROOT,
		.seq_show = tg_print_max,
		.write = tg_set_max,
	},
	{ }	/* terminate */
};

static void throtl_shutdown_wq(struct request_queue *q)
{
	struct throtl_data *td = q->td;

        del_timer_sync(&td->rescue_timer);
        del_timer_sync(&td->idle_timer);
	cancel_work_sync(&td->dispatch_work);
}

/*lint -save -e31 -e64*/
static struct blkcg_policy blkcg_policy_throtl = {
	.dfl_cftypes		= throtl_files,
	.legacy_cftypes		= throtl_legacy_files,

	.pd_alloc_fn		= throtl_pd_alloc,
	.pd_init_fn		= throtl_pd_init,
	.pd_online_fn		= throtl_pd_online,
	.pd_offline_fn          = throtl_pd_offline,
	.pd_free_fn		= throtl_pd_free,
};
/*lint -restore*/

bool blk_throtl_bio(struct request_queue *q, struct blkcg_gq *blkg,
		    struct bio *bio)
{
	struct throtl_qnode *qn = NULL;
	struct throtl_grp *tg = blkg_to_tg(blkg ?: q->root_blkg);
	struct throtl_service_queue *sq;
	bool rw = bio_data_dir(bio);
	int index;
	struct throtl_data *td = q->td;
	struct blkcg *blkcg;
	bool throttled = false;

	WARN_ON_ONCE(!rcu_read_lock_held());

	/* see throtl_charge_bio() */
	if ((bio->bi_opf & (REQ_THROTTLED |REQ_META | REQ_PREFLUSH |
		REQ_FUA | REQ_OP_DISCARD | REQ_CHAINED)))
		goto out;

	/* If we are on the way of memory reclaim, skip throttle */
	if (current->flags & (PF_MEMALLOC | PF_SWAPWRITE | PF_KSWAPD))
		goto out;

	if (task_in_pagefault(current))
		goto out;
again:
	if (!blkg)
		goto out;

	index =  tg_data_index(tg, rw);
	if (!tg->has_rules[index])
		goto out;

	/*lint -save -e613*/
	blkcg = blkg->blkcg;
	if (td_weight_based(td) &&
	       (blkcg == &blkcg_root || !blk_throtl_weight_offon))
		goto out;
	/*lint -restore*/

	if (blkcg->type <= BLK_THROTL_FG)
		bio->bi_opf |= REQ_FG;

	if (td_weight_based(td) &&
		(blk_throtl_weight_offon == BLK_THROTL_WEIGHT_ON_FS))
		goto out;
	/*
	* Now we just support limit control for one layer.
	*/
	/*lint -save -e438 -e529 -e666*/
	if (ACCESS_ONCE(tg->td->max_inflights) && !bio->bi_throtl_private1 &&
	   current->mm &&
	   ACCESS_ONCE(current->mm->io_limit->max_inflights)) {
	/*lint -restore*/
		struct blk_throtl_io_limit *io_limit = current->mm->io_limit;
		int ret;

		blkg_get(tg_to_blkg(tg));
		ret = blk_throtl_io_limit_wait(blkcg, io_limit, tg, bio);
		blkg_put(tg_to_blkg(tg));
		if (ret == 1) {
			blkcg = bio_blkcg(bio);

			blkg = blkg_lookup(blkcg, q);
			if (unlikely(!blkg)) {
				spin_lock_irq(q->queue_lock);
				blkg = blkg_lookup_create(blkcg, q);
				if (IS_ERR(blkg))
					blkg = q->root_blkg;
				spin_unlock_irq(q->queue_lock);
			}

			tg = blkg_to_tg(blkg ?: q->root_blkg);
			goto again;
		} else if (ret == 2)
			goto skip;

	       blk_throtl_io_limit_get(io_limit);
	       bio->bi_throtl_private1 = io_limit;
	       bio->bi_throtl_end_io1 = blk_throtl_limit_io_done;
	}
skip:

	spin_lock_irq(q->queue_lock);

	/*lint -save -e730*/
	if (unlikely(blk_queue_bypass(q)))
	/*lint -restore*/
		goto out_unlock;

	if (!(tg->flags & THROTL_TG_ONLINE))
		goto out_unlock;

	if (unlikely(blk_throtl_weight_offon != BLK_THROTL_WEIGHT_ON_BLK))
		 goto out_unlock;

	if (td_weight_based(td)) {
		struct bio_list bios;

		/*
		* Now we just support weight based throttle for
		* one layer.
		*/
		if (bio->bi_throtl_end_io2)
			goto out_unlock;

		bio_list_init(&bios);

		throttled = true;
		bio_associate_current(bio);
		throtl_add_bio_weight(bio, tg);
		blk_run_throtl_bios_weight(td, tg, &bios);
		spin_unlock_irq(q->queue_lock);

		rcu_read_unlock();
		/*lint -save -e820*/
		while((bio = bio_list_pop(&bios)))
		/*lint -restore*/
			generic_make_request(bio);

		rcu_read_lock();
		goto out;
	}

	sq = &tg->service_queue;

	while (true) {
		/* throtl is FIFO - if bios are already queued, should queue */
		if (sq->nr_queued[index])
			break;

		/* if above limits, break to queue */
		if (!tg_may_dispatch(tg, bio, NULL))
			break;

		/* within limits, let's charge and dispatch directly */
		throtl_charge_bio(tg, bio);

		/*
		 * We need to trim slice even when bios are not being queued
		 * otherwise it might happen that a bio is not queued for
		 * a long time and slice keeps on extending and trim is not
		 * called for a long time. Now if limits are reduced suddenly
		 * we take into account all the IO dispatched so far at new
		 * low rate and * newly queued IO gets a really long dispatch
		 * time.
		 *
		 * So keep on trimming slice even if bio is not queued.
		 */
		throtl_trim_slice(tg, rw);

		/*
		 * @bio passed through this layer without being throttled.
		 * Climb up the ladder.  If we''re already at the top, it
		 * can be executed directly.
		 */
		qn = &tg->qnode_on_parent[index];
		sq = sq->parent_sq;
		tg = sq_to_tg(sq);
		if (!tg)
			goto out_unlock;
	}

	/* out-of-limit, queue to @tg */
	/*lint -save -e747*/
	throtl_log(sq, "[%c] bio. bdisp=%llu sz=%u bps=%llu iodisp=%u iops=%u queued=%d/%d",
		   rw == READ ? 'R' : 'W',
		   tg->io_cost.bytes_disp[index], bio->bi_iter.bi_size,
		   tg->io_cost.bps[index], tg->io_cost.io_disp[index],
		   tg->io_cost.iops[index],
		   sq->nr_queued[READ], sq->nr_queued[WRITE]);
	/*lint -restore*/

	bio_associate_current(bio);
	tg->td->nr_queued[index]++;
	throtl_add_bio_tg(bio, qn, tg);
	throttled = true;

	/*
	 * Update @tg's dispatch time and force schedule dispatch if @tg
	 * was empty before @bio.  The forced scheduling isn't likely to
	 * cause undue delay as @bio is likely to be dispatched directly if
	 * its @tg's disptime is not in the future.
	 */
	if (tg->flags & THROTL_TG_WAS_EMPTY) {
		tg_update_disptime(tg);
		/*lint -save -e747*/
		throtl_schedule_next_dispatch(tg->service_queue.parent_sq, true);
		/*lint -restore*/
	}

out_unlock:
	spin_unlock_irq(q->queue_lock);
out:
	/*
	 * As multiple blk-throtls may stack in the same issue path, we
	 * don't want bios to leave with the flag set.  Clear the flag if
	 * being issued.
	 */
	if (!throttled)
		/*lint -save -e613*/
		bio->bi_opf &= ~REQ_THROTTLED;
		/*lint -restore*/
	return throttled;
}

static inline void blk_throtl_update_quota(struct throtl_grp *tg,
					   unsigned int size)
{
	tg->intime = jiffies;
	tg->io_cost.bytes_disp[0] += size;
	tg->io_cost.io_disp[0]++;
}

bool blk_throtl_get_quota(struct block_device *bdev, unsigned int size,
			  unsigned long jiffies_time_out, bool wait)
{
	struct request_queue *q;
	struct throtl_data *td;
	struct throtl_grp *tg;
	struct blkcg *blkcg = NULL;
	struct blkcg *new_blkcg;
	unsigned long start_jiffies;
	unsigned long wait_jiffies = jiffies_time_out;
	int ret;

	if (current->flags & (PF_MEMALLOC | PF_SWAPWRITE |
			      PF_KSWAPD | PF_MUTEX_GC))
		return true;

	/*
	 * bdev has been got at mount(), and will be freed at kill_sb() after
	 * all files closed and all dirty pages flushed. So it's safe to
	 * using bdev without lock here.
	 */
	if (unlikely(!bdev) || unlikely(!bdev->bd_disk))
		return true;

	q = bdev_get_queue(bdev);
	if (unlikely(!q))
		return true;

	td = q->td;

	start_jiffies = jiffies;
get_new_group:
	/*
	 * A throtl_grp pointer retrieved under rcu can be used to access
	 * basic fields.
	 */
	rcu_read_lock();

	if (!td_weight_based(td))
		goto out_unlock_rcu;

	if (blk_throtl_weight_offon != BLK_THROTL_WEIGHT_ON_FS)
		goto out_unlock_rcu;

	new_blkcg = task_blkcg(current);
	if (new_blkcg == &blkcg_root)
		goto out_unlock_rcu;
	if (!blkcg || blkcg != new_blkcg) {
		struct blkcg_gq *blkg;

		blkcg = new_blkcg;

		blkg = blkg_lookup(blkcg, q);
		if (unlikely(!blkg)) {
			spin_lock_irq(q->queue_lock);
			blkg = blkg_lookup_create(blkcg, q);
			if (IS_ERR(blkg))
				blkg = q->root_blkg;
			spin_unlock_irq(q->queue_lock);
		}

		tg = blkg_to_tg(blkg);
	}

	spin_lock_irq(q->queue_lock);

	if (unlikely(blk_queue_bypass(q)))
		goto out_unlock;

	if (unlikely(!(tg->flags & THROTL_TG_ONLINE)))
		goto out_unlock;

	if (unlikely(!td_weight_based(td)))
		goto out_unlock;

	if (unlikely(blk_throtl_weight_offon != BLK_THROTL_WEIGHT_ON_FS))
		goto out_unlock;
retry:
	if (tg_may_dispatch_weight(tg)) {
		blk_throtl_update_quota(tg, size);
		if (!list_empty(&tg->node))
			goto out_unlock;
		blkg_get(tg_to_blkg(tg));
		list_add_tail(&tg->node, &td->active);
		goto out_unlock;
	} else if (!wait) {
		spin_unlock_irq(q->queue_lock);
		rcu_read_unlock();
		return false;
	}

	if (!tg->expired) {
		tg->expired = true;
		list_move_tail(&tg->node, &td->expired);

		if (list_empty(&td->active))
			goto start_new_slice;
	}

	/*
	 * If jiffies_time_out expires, just return to continue
	 * to read/write.
	 */
	wait_jiffies = jiffies_time_out - (jiffies - start_jiffies);
	if (jiffies - start_jiffies >= jiffies_time_out)
		goto out_unlock;

	if (should_start_new_slice(td))
		goto start_new_slice;

	blkg_get(tg_to_blkg(tg));
	spin_unlock_irq(q->queue_lock);
	rcu_read_unlock();

	/*lint -save -e665 -e666*/
	/* Waiting for starting new slice, max waiting time is 20ms */
	ret = wait_event_interruptible_timeout(tg->wait, !tg->expired,
			wait_jiffies >
			msecs_to_jiffies(THROTL_IDLE_INTERVAL / 2) ?
			msecs_to_jiffies(THROTL_IDLE_INTERVAL / 2) :
			wait_jiffies);
	/*lint -restore*/

	blkg_put(tg_to_blkg(tg));

	/* If this task is interrupted, just return */
	if (signal_pending_state(TASK_INTERRUPTIBLE, current))
		return true;

	/* this task may switch to other group, get tg again */
	goto get_new_group;

start_new_slice:
	/* Start new slice and wake up waiting tgs */
	throtl_start_new_slice_weight(td);
	goto retry;

out_unlock:
	spin_unlock_irq(q->queue_lock);

out_unlock_rcu:
	rcu_read_unlock();
	return true;
}

/*
 * Dispatch all bios from all children tg's queued on @parent_sq.  On
 * return, @parent_sq is guaranteed to not have any active children tg's
 * and all bios from previously active tg's are on @parent_sq->bio_lists[].
 */
static void tg_drain_bios(struct throtl_service_queue *parent_sq)
{
	struct throtl_grp *tg;

	/*lint -save -e747 -e820*/
	while ((tg = throtl_rb_first(parent_sq))) {
		struct throtl_service_queue *sq = &tg->service_queue;
		struct bio *bio;

		throtl_dequeue_tg(tg);

		while ((bio = throtl_peek_queued(&sq->queued[READ])))
			tg_dispatch_one_bio(tg, bio_data_dir(bio));
		while ((bio = throtl_peek_queued(&sq->queued[WRITE])))
			tg_dispatch_one_bio(tg, bio_data_dir(bio));
	}
	/*lint -restore*/
}

/**
 * blk_throtl_drain - drain throttled bios
 * @q: request_queue to drain throttled bios for
 *
 * Dispatch all currently throttled bios on @q through ->make_request_fn().
 */
void blk_throtl_drain(struct request_queue *q)
	__releases(q->queue_lock) __acquires(q->queue_lock)
{
	struct throtl_data *td = q->td;
	struct blkcg_gq *blkg;
	struct cgroup_subsys_state *pos_css;
	struct bio *bio;
	int i;

	if (td_weight_based(td)) {
		if (blk_throtl_weight_offon == BLK_THROTL_WEIGHT_ON_FS)
			__blk_throtl_drain_weight(q);
		else
			blk_throtl_drain_weight(q);
		return;
	}

	queue_lockdep_assert_held(q);
	rcu_read_lock();

	/*
	 * Drain each tg while doing post-order walk on the blkg tree, so
	 * that all bios are propagated to td->service_queue.  It'd be
	 * better to walk service_queue tree directly but blkg walk is
	 * easier.
	 */
	/*lint -save -e747 -e820*/
	blkg_for_each_descendant_post(blkg, pos_css, td->queue->root_blkg)
		tg_drain_bios(&blkg_to_tg(blkg)->service_queue);
	/*lint -restore*/

	/* finally, transfer bios from top-level tg's into the td */
	tg_drain_bios(&td->service_queue);

	rcu_read_unlock();
	spin_unlock_irq(q->queue_lock);

	/* all bios now should be in td->service_queue, issue them */
	/*lint -save -e574 -e737 -e820*/
	for (i = READ; i <= WRITE; i++)
		while ((bio = throtl_pop_queued(&td->service_queue.queued[i],
						NULL)))
			generic_make_request(bio);
	/*lint -restore*/

	spin_lock_irq(q->queue_lock);
}

int blk_throtl_init(struct request_queue *q)
{
	struct throtl_data *td;
	int ret;

	td = kzalloc_node(sizeof(*td), GFP_KERNEL, q->node);
	if (!td)
		return -ENOMEM;
	INIT_LIST_HEAD(&td->active);
	INIT_LIST_HEAD(&td->expired);
	/*lint -save -e1058 */
	atomic_set(&td->inflights, 0);
	init_waitqueue_head(&td->waitq);
	setup_timer(&td->rescue_timer, blk_throtl_rescue_timer_fn,
		    (unsigned long)td);
	setup_timer(&td->idle_timer, blk_throtl_idle_timer_fn,
		(unsigned long)td);
	/*lint -restore*/
	INIT_WORK(&td->dispatch_work, blk_throtl_dispatch_work_fn);
	throtl_service_queue_init(&td->service_queue);

	td->service_queue.share = MAX_SHARE;
	td->mode = MODE_NONE;

	td->quantum = QUANTUM_INQUEUE_DEF;
	td->bw_slice = BW_SLICE_DEF;
	td->iops_slice = IOPS_SLICE_DEF;

	q->td = td;
	td->queue = q;

	/* activate policy */
	ret = blkcg_activate_policy(q, &blkcg_policy_throtl);
	if (ret)
		kfree(td);
	return ret;
}

void blk_throtl_exit(struct request_queue *q)
{
	/*lint -save -e730*/
	BUG_ON(!q->td);
	/*lint -restore*/
	throtl_shutdown_wq(q);
	blkcg_deactivate_policy(q, &blkcg_policy_throtl);
	kfree(q->td);
}

/*lint -e528 -esym(528,*)*/
static int __init throtl_init(void)
{
	kthrotld_workqueue = alloc_workqueue("kthrotld", WQ_MEM_RECLAIM, 0);
	if (!kthrotld_workqueue)
		panic("Failed to create kthrotld\n");

	return blkcg_policy_register(&blkcg_policy_throtl);
}
/*lint -e528 +esym(528,*)*/

module_init(throtl_init);
