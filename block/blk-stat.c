/*
 * Block stat tracking code
 *
 * Copyright (C) 2016 Jens Axboe
 */
#include <linux/kernel.h>
#include <linux/blk-mq.h>

#include "blk-stat.h"
#include "blk-mq.h"

void blk_stat_sum(struct blk_rq_stat *dst, struct blk_rq_stat *src)
{
	if (!src->nr_samples)
		return;

	dst->min = min(dst->min, src->min);
	dst->max = max(dst->max, src->max);

	if (!dst->nr_samples)
		dst->mean = src->mean;
	else {
		dst->mean = div64_s64((src->mean * src->nr_samples) +
					(dst->mean * dst->nr_samples),
					dst->nr_samples + src->nr_samples);
	}
	dst->nr_samples += src->nr_samples;
}

static void blk_mq_stat_get(struct request_queue *q, struct blk_rq_stat *dst)
{
	struct blk_mq_hw_ctx *hctx;
	struct blk_mq_ctx *ctx;
	int i, j, nr;

	blk_stat_init(&dst[0]);
	blk_stat_init(&dst[1]);
	blk_stat_init(&dst[2]);
	blk_stat_init(&dst[3]);

	nr = 0;
	do {
		uint64_t newest = 0;

		/*lint -save -e574 -e732 -e737*/
		queue_for_each_hw_ctx(q, hctx, i) {
			hctx_for_each_ctx(hctx, ctx, j) {
				if (!ctx->stat[0].nr_samples &&
				    !ctx->stat[1].nr_samples)
					continue;
				if (ctx->stat[0].time > newest)
					newest = ctx->stat[0].time;
				if (ctx->stat[1].time > newest)
					newest = ctx->stat[1].time;
			}
		}
		/*lint -restore*/

		/*
		 * No samples
		 */
		if (!newest)
			break;

		/*lint -save -e574 -e732 -e737*/
		queue_for_each_hw_ctx(q, hctx, i) {
			hctx_for_each_ctx(hctx, ctx, j) {
				if (ctx->stat[0].time == newest) {
					blk_stat_sum(&dst[0], &ctx->stat[0]);
					nr++;
				}
				if (ctx->stat[1].time == newest) {
					blk_stat_sum(&dst[1], &ctx->stat[1]);
					nr++;
				}
				if (ctx->stat[2].time == newest) {
					blk_stat_sum(&dst[2], &ctx->stat[2]);
					nr++;
				}
				if (ctx->stat[3].time == newest) {
					blk_stat_sum(&dst[3], &ctx->stat[3]);
					nr++;
				}
			}
		}
		/*lint -restore*/
		/*
		 * If we race on finding an entry, just loop back again.
		 * Should be very rare.
		 */
	} while (!nr);
}

void blk_queue_stat_get(struct request_queue *q, struct blk_rq_stat *dst)
{
	if (q->mq_ops)
		blk_mq_stat_get(q, dst);
	else {
		memcpy(&dst[0], &q->rq_stats[0], sizeof(struct blk_rq_stat));
		memcpy(&dst[1], &q->rq_stats[1], sizeof(struct blk_rq_stat));
		memcpy(&dst[2], &q->rq_stats[2], sizeof(struct blk_rq_stat));
		memcpy(&dst[3], &q->rq_stats[3], sizeof(struct blk_rq_stat));
	}
}

void blk_hctx_stat_get(struct blk_mq_hw_ctx *hctx, struct blk_rq_stat *dst)
{
	struct blk_mq_ctx *ctx;
	unsigned int i, nr;

	nr = 0;
	do {
		uint64_t newest = 0;

		hctx_for_each_ctx(hctx, ctx, i) {
			if (!ctx->stat[0].nr_samples &&
			    !ctx->stat[1].nr_samples)
				continue;

			/*lint -save -e574 -e732 -e737*/
			if (ctx->stat[0].time > newest)
				newest = ctx->stat[0].time;
			if (ctx->stat[1].time > newest)
				newest = ctx->stat[1].time;
			/*lint -restore*/
		}

		if (!newest)
			break;

		/*lint -save -e737*/
		hctx_for_each_ctx(hctx, ctx, i) {
			if (ctx->stat[0].time == newest) {
				blk_stat_sum(&dst[0], &ctx->stat[0]);
				nr++;
			}
			if (ctx->stat[1].time == newest) {
				blk_stat_sum(&dst[1], &ctx->stat[1]);
				nr++;
			}
			if (ctx->stat[2].time == newest) {
				blk_stat_sum(&dst[2], &ctx->stat[2]);
				nr++;
			}
			if (ctx->stat[3].time == newest) {
				blk_stat_sum(&dst[3], &ctx->stat[3]);
				nr++;
			}
		}
		/*lint -restore*/
		/*
		 * If we race on finding an entry, just loop back again.
		 * Should be very rare, as the window is only updated
		 * occasionally
		 */
	} while (!nr);
}

/*lint -save -e501 -e713 -e737*/
static void __blk_stat_init(struct blk_rq_stat *stat, s64 time_now)
{
	stat->min = -1ULL;
	stat->max = stat->nr_samples = stat->mean = 0;
	stat->time = time_now & BLK_STAT_MASK;
}
/*lint -restore*/

void blk_stat_init(struct blk_rq_stat *stat)
{
	__blk_stat_init(stat, ktime_to_ns(ktime_get()));
}

void blk_stat_add(struct blk_rq_stat *stat, struct request *rq)
{
	s64 delta, now, value;
	u64 rq_time = wbt_issue_stat_get_time(&rq->wb_stat);

	now = ktime_to_ns(ktime_get());
	/*lint -save -e574 -e713 -e732 -e737*/
	if (now < rq_time)
		return;

	if ((now & BLK_STAT_MASK) != (stat->time & BLK_STAT_MASK))
		__blk_stat_init(stat, now);

	value = now - rq_time;
	if (value > stat->max)
		stat->max = value;
	if (value < stat->min)
		stat->min = value;

	delta = value - stat->mean;
	if (delta)
		stat->mean += div64_s64(delta, stat->nr_samples + 1);
	/*lint -restore*/

	stat->nr_samples++;
}

void blk_stat_clear(struct request_queue *q)
{
	if (q->mq_ops) {
		struct blk_mq_hw_ctx *hctx;
		struct blk_mq_ctx *ctx;
		int i, j;

		/*lint -save -e574 -e737*/
		queue_for_each_hw_ctx(q, hctx, i) {
			hctx_for_each_ctx(hctx, ctx, j) {
				blk_stat_init(&ctx->stat[0]);
				blk_stat_init(&ctx->stat[1]);
				blk_stat_init(&ctx->stat[2]);
				blk_stat_init(&ctx->stat[3]);
			}
		}
		/*lint -restore*/
	} else {
		blk_stat_init(&q->rq_stats[0]);
		blk_stat_init(&q->rq_stats[1]);
		blk_stat_init(&q->rq_stats[2]);
		blk_stat_init(&q->rq_stats[3]);
	}
}
