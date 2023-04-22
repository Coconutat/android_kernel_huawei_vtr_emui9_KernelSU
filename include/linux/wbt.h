#ifndef WB_THROTTLE_H
#define WB_THROTTLE_H

#include <linux/atomic.h>
#include <linux/wait.h>
#include <linux/timer.h>
#include <linux/ktime.h>

#define ISSUE_STAT_MASK		(1ULL << 63)
#define ISSUE_STAT_TIME_MASK	~ISSUE_STAT_MASK

struct wb_issue_stat {
	u64 time;
};

static inline void wbt_issue_stat_set_time(struct wb_issue_stat *stat)
{
	stat->time = (stat->time & ISSUE_STAT_MASK) |
			(ktime_to_ns(ktime_get()) & ISSUE_STAT_TIME_MASK);
}

static inline u64 wbt_issue_stat_get_time(struct wb_issue_stat *stat)
{
	return stat->time & ISSUE_STAT_TIME_MASK;
}

static inline void wbt_mark_tracked(struct wb_issue_stat *stat)
{
	stat->time |= ISSUE_STAT_MASK;
}

static inline void wbt_clear_tracked(struct wb_issue_stat *stat)
{
	stat->time &= ~ISSUE_STAT_MASK;
}

static inline bool wbt_tracked(struct wb_issue_stat *stat)
{
	return (stat->time & ISSUE_STAT_MASK) != 0;
}

struct wb_stat_ops {
	void (*get)(void *, struct blk_rq_stat *);
	void (*clear)(void *);
};

enum wbt_mode {
	WBT_FS,
	WBT_BLK,
};

struct rq_wb {
	/*
	 * Settings that govern how we throttle
	 */
	unsigned int wb_background;		/* background writeback */
	unsigned int wb_normal;			/* normal writeback */
	unsigned int wb_max;			/* max throughput writeback */
	unsigned int scale_step;

	enum wbt_mode mode;
	u64 win_nsec;				/* default window size */
	u64 cur_win_nsec;			/* current window size */

	/*
	 * Number of consecutive periods where we don't have enough
	 * information to make a firm scale up/down decision.
	 */
	unsigned int unknown_cnt;
	unsigned long ok_cnt;
	unsigned long ok_cnt_set;

	struct timer_list window_timer;

	s64 sync_issue;
	void *sync_cookie;

	unsigned int wc;
	unsigned int queue_depth;

	unsigned long last_issue;		/* last non-throttled issue */
	unsigned long last_comp;		/* last non-throttled comp */
	unsigned long min_lat_nsec;
	struct backing_dev_info *bdi;
	struct request_queue *q;
	wait_queue_head_t wait;
	atomic_t inflight;

	struct wb_stat_ops *stat_ops;
	void *ops_data;
};

static inline enum wbt_mode wbt_mode(struct rq_wb *rwb)
{
	return rwb->mode;
}

struct backing_dev_info;

void __wbt_done(struct rq_wb *);

#ifdef CONFIG_WBT
void wbt_done(struct rq_wb *, struct wb_issue_stat *, bool);
bool wbt_wait(struct rq_wb *, unsigned int, spinlock_t *);
struct rq_wb *wbt_init(struct backing_dev_info *, struct wb_stat_ops *, void *);
void wbt_exit(struct rq_wb *);
void wbt_update_limits(struct rq_wb *);
void wbt_requeue(struct rq_wb *, struct wb_issue_stat *);
void wbt_issue(struct rq_wb *, struct wb_issue_stat *, bool);
void wbt_disable(struct rq_wb *);
void wbt_set_queue_depth(struct rq_wb *, unsigned int);
void wbt_set_write_cache(struct rq_wb *, bool);
int wbt_max_bio_blocks(struct block_device *bdev, int rw, int max, bool *nomerge);
bool wbt_need_kick_bio(struct bio *bio);
bool wbt_fs_get_quota(struct request_queue *q, struct writeback_control *wbc);
void wbt_fs_wait(struct request_queue *q, struct writeback_control *wbc);
#else
static inline bool wbt_need_kick_bio(struct bio *bio)
{
	return false;
}
static inline int wbt_max_bio_blocks(struct block_device *bdev, int rw, int max, bool *nomerge)
{
	return BIO_MAX_PAGES;
}
static inline void wbt_done(struct rq_wb *rwb, struct wb_issue_stat *stat, bool fg)
{
}
static inline bool wbt_wait(struct rq_wb *rwb, unsigned long rw, spinlock_t *lock)
{
	return false;
}
static inline struct rq_wb *wbt_init(struct backing_dev_info *bdi, struct wb_stat_ops *ops, void *ops_data)
{
	return ERR_PTR(-ENOTSUPP);
}
static inline void wbt_exit(struct rq_wb *rwb)
{
}
static inline void wbt_update_limits(struct rq_wb *rwb)
{
}
static inline void wbt_requeue(struct rq_wb *rwb, struct wb_issue_stat *stat)
{
}
static inline void wbt_issue(struct rq_wb *rwb, struct wb_issue_stat *stat, bool fg)
{
}
static inline void wbt_disable(struct rq_wb *rwb)
{
}
static inline void wbt_set_queue_depth(struct rq_wb *rwb, unsigned int depth)
{
}
static inline void wbt_set_write_cache(struct rq_wb *rwb, bool write_cache_on)
{
}
#endif

#endif
