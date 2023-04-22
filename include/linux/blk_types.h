/*
 * Block data types and constants.  Directly include this file only to
 * break include dependency loop.
 */
#ifndef __LINUX_BLK_TYPES_H
#define __LINUX_BLK_TYPES_H

#include <linux/types.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/bvec.h>

struct bio_set;
struct bio;
struct bio_integrity_payload;
struct page;
struct block_device;
struct io_context;
struct cgroup_subsys_state;
typedef void (bio_end_io_t) (struct bio *);
typedef void (bio_throtl_end_io_t) (struct bio *);

/*
 * was for io checkpoint in io latency
 */
#ifdef CONFIG_BLOCK
#ifdef CONFIG_HISI_BLK
#undef  ADDITEM
#define ADDITEM( _etype, _comment, _func_pointer )      _etype
enum bio_process_stage_enum {
	#include <linux/hisi_bio_stage_def.h>
    BIO_PROC_STAGE_MAX
};

enum req_process_stage_enum {
	#include <linux/hisi_req_stage_def.h>
    REQ_PROC_STAGE_MAX
};
#undef  ADDITEM

/*
* This struct defines all the variable in vendor block layer.
*/
struct blk_bio_cust {
	struct request_queue *q; /* The request queue where the bio is sent to */
	struct block_device	*bi_bdev_part; /* The block device where the bio is sent to */
	struct request *io_req; /* The request which carrys the bio */
	struct task_struct *dispatch_task; /* Dispatch IO process task struct */
	pid_t task_pid; /* Dispatch IO process PID */
	pid_t task_tgid;/* Dispatch IO process TGID */
	char task_comm[TASK_COMM_LEN];/* Dispatch IO process name */

#define HISI_IO_IN_COUNT_SET	(1 << 0)
#define HISI_IO_IN_COUNT_SKIP_ENDIO	(1 << 1) /* busy count for the BIO has not been added, so skip it */
#define HISI_IO_IN_COUNT_ALREADY_ENDED	(1 << 2) /* this BIO has been ended already */
#define HISI_IO_IN_COUNT_WILL_BE_SEND_AGAIN	(1 << 3)
#define HISI_IO_IN_COUNT_DONE	(1 << 4)

	unsigned char io_in_count; /*the bio has been count in busy idle module or not */
	unsigned char fs_io_flag; /* io comes from fs or not */
	unsigned char bi_async_flush; /* async flush flag */
	unsigned char hot_cold_id; /* hot cold id */

	/*
	* Below info is IO latency
	*/
	ktime_t bio_stage_ktime[BIO_PROC_STAGE_MAX];
	struct timer_list latency_expire_timer;
	volatile int latency_timer_running;

	unsigned int pg_count; /* page count in current bio */
	unsigned char latency_classify; /* bio classify in latency statistic */
	struct timespec submit_tp;
	s64 hw_latency; /* IO latency in low level driver */
	/*
	* Below info is for inline crypto
	*/
	void	*ci_key;
	int	ci_key_len;
	int	ci_key_index;
	pgoff_t index;
};
#endif
#ifdef CONFIG_F2FS_CHECK_FS
struct access_timestamp {
	struct timespec time;
	struct list_head list;
};
struct bdev_access_info {
	struct list_head access_list;
	atomic_t open_cnt;
	spinlock_t lock;
};
extern struct bdev_access_info bdev_access_info;
extern atomic_t f2fs_mounted; /* how may f2fs mounted */
#endif
/*
 * main unit of I/O for the block layer and lower layers (ie drivers and
 * stacking drivers)
 */
struct bio {
	struct bio		*bi_next;	/* request queue link */
	struct block_device	*bi_bdev;
	int			bi_error;
	unsigned int		bi_opf;		/* bottom bits req flags,
						 * top bits REQ_OP. Use
						 * accessors.
						 */
	unsigned short		bi_flags;	/* status, command, etc */
	unsigned short		bi_ioprio;

	struct bvec_iter	bi_iter;

	/* Number of segments in this BIO after
	 * physical address coalescing is performed.
	 */
	unsigned int		bi_phys_segments;

	/*
	 * To keep track of the max segment size, we account for the
	 * sizes of the first and last mergeable segments in this bio.
	 */
	unsigned int		bi_seg_front_size;
	unsigned int		bi_seg_back_size;

	atomic_t		__bi_remaining;

	bio_end_io_t		*bi_end_io;

	void			*bi_private;
#ifdef CONFIG_F2FS_CHECK_FS
	unsigned int		bi_start_blkaddr;
#endif

#ifdef CONFIG_HISI_BLK
	struct blk_bio_cust hisi_bio;
	struct list_head counted_list_node;
#endif

#ifdef CONFIG_BLK_DEV_THROTTLING
	bio_throtl_end_io_t     *bi_throtl_end_io1;
	void                    *bi_throtl_private1;
	bio_throtl_end_io_t     *bi_throtl_end_io2;
	void                    *bi_throtl_private2;
	unsigned long           bi_throtl_in_queue;
#endif


#ifdef CONFIG_BLK_CGROUP
	/*
	 * Optional ioc and css associated with this bio.  Put on bio
	 * release.  Read comment on top of bio_associate_current().
	 */
	struct io_context	*bi_ioc;
	struct cgroup_subsys_state *bi_css;
#endif
	union {
#if defined(CONFIG_BLK_DEV_INTEGRITY)
		struct bio_integrity_payload *bi_integrity; /* data integrity */
#endif
	};

	unsigned short		bi_vcnt;	/* how many bio_vec's */

	/*
	 * Everything starting with bi_max_vecs will be preserved by bio_reset()
	 */

	unsigned short		bi_max_vecs;	/* max bvl_vecs we can hold */

	atomic_t		__bi_cnt;	/* pin count */

	struct bio_vec		*bi_io_vec;	/* the actual vec list */

	struct bio_set		*bi_pool;

	/*
	 * We can inline a number of vecs at the end of the bio, to avoid
	 * double allocations for a small number of bio_vecs. This member
	 * MUST obviously be kept at the very end of the bio.
	 */
	struct bio_vec		bi_inline_vecs[0];
};

#define BIO_OP_SHIFT	(8 * FIELD_SIZEOF(struct bio, bi_opf) - REQ_OP_BITS)
#define bio_flags(bio)	((bio)->bi_opf & ((1 << BIO_OP_SHIFT) - 1))
#define bio_op(bio)	((bio)->bi_opf >> BIO_OP_SHIFT)

#define bio_set_op_attrs(bio, op, op_flags) do {			\
	if (__builtin_constant_p(op))					\
		BUILD_BUG_ON((op) + 0U >= (1U << REQ_OP_BITS));		\
	else								\
		WARN_ON_ONCE((op) + 0U >= (1U << REQ_OP_BITS));		\
	if (__builtin_constant_p(op_flags))				\
		BUILD_BUG_ON((op_flags) + 0U >= (1U << BIO_OP_SHIFT));	\
	else								\
		WARN_ON_ONCE((op_flags) + 0U >= (1U << BIO_OP_SHIFT));	\
	(bio)->bi_opf = bio_flags(bio);					\
	(bio)->bi_opf |= (((op) + 0U) << BIO_OP_SHIFT);			\
	(bio)->bi_opf |= (op_flags);					\
} while (0)

#define BIO_RESET_BYTES		offsetof(struct bio, bi_max_vecs)

/*
 * bio flags
 */
#define BIO_SEG_VALID	1	/* bi_phys_segments valid */
#define BIO_CLONED	2	/* doesn't own data */
#define BIO_BOUNCED	3	/* bio is a bounce bio */
#define BIO_USER_MAPPED 4	/* contains user pages */
#define BIO_NULL_MAPPED 5	/* contains invalid user pages */
#define BIO_QUIET	6	/* Make BIO Quiet */
#define BIO_CHAIN	7	/* chained bio, ->bi_remaining in effect */
#define BIO_REFFED	8	/* bio has elevated ->bi_cnt */

/*
 * Flags starting here get preserved by bio_reset() - this includes
 * BVEC_POOL_IDX()
 */
#define BIO_RESET_BITS	10

/*
 * We support 6 different bvec pools, the last one is magic in that it
 * is backed by a mempool.
 */
#define BVEC_POOL_NR		6
#define BVEC_POOL_MAX		(BVEC_POOL_NR - 1)

/*
 * Top 4 bits of bio flags indicate the pool the bvecs came from.  We add
 * 1 to the actual index so that 0 indicates that there are no bvecs to be
 * freed.
 */
#define BVEC_POOL_BITS		(4)
#define BVEC_POOL_OFFSET	(16 - BVEC_POOL_BITS)
#define BVEC_POOL_IDX(bio)	((bio)->bi_flags >> BVEC_POOL_OFFSET)

#endif /* CONFIG_BLOCK */

/*
 * Request flags.  For use in the cmd_flags field of struct request, and in
 * bi_opf of struct bio.  Note that some flags are only valid in either one.
 */
enum rq_flag_bits {
	/* common flags */
	__REQ_FAILFAST_DEV,	/* no driver retries of device errors */
	__REQ_FAILFAST_TRANSPORT, /* no driver retries of transport errors */
	__REQ_FAILFAST_DRIVER,	/* no driver retries of driver errors */

	__REQ_SYNC,		/* request is sync (sync write or read) */
	__REQ_META,		/* metadata io request */
	__REQ_PRIO,		/* boost priority in cfq */

	__REQ_NOIDLE,		/* don't anticipate more IO after this one */
	__REQ_INTEGRITY,	/* I/O includes block integrity payload */
	__REQ_FUA,		/* forced unit access */
	__REQ_PREFLUSH,		/* request for cache flush */
	__REQ_BG,		/* background activity */
	__REQ_FG,		/* foreground activity */

	/* bio only flags */
	__REQ_RAHEAD,		/* read ahead, can fail anytime */
	__REQ_THROTTLED,	/* This bio has already been subjected to
				 * throttling rules. Don't do it again. */
	__REQ_CHAINED,

	/* request only flags */
	__REQ_SORTED,		/* elevator knows about this request */
	__REQ_SOFTBARRIER,	/* may not be passed by ioscheduler */
	__REQ_NOMERGE,		/* don't touch this for merging */
	__REQ_STARTED,		/* drive already may have started this one */
	__REQ_DONTPREP,		/* don't call prep for this one */
	__REQ_QUEUED,		/* uses queueing */
	__REQ_ELVPRIV,		/* elevator private data attached */
	__REQ_FAILED,		/* set if the request failed */
	__REQ_QUIET,		/* don't worry about errors */
	__REQ_PREEMPT,		/* set for "ide_preempt" requests and also
				   for requests for which the SCSI "quiesce"
				   state must be ignored. */
	__REQ_ALLOCED,		/* request came from our alloc pool */
	__REQ_COPY_USER,	/* contains copies of user pages */
	__REQ_FLUSH_SEQ,	/* request for flush sequence */
	__REQ_IO_STAT,		/* account I/O stat */
	__REQ_MIXED_MERGE,	/* merge of different types, fail separately */
	__REQ_PM,		/* runtime pm request */
	__REQ_HASHED,		/* on IO scheduler merge hash */
	__REQ_MQ_INFLIGHT,	/* track inflight for MQ */
	__REQ_URGENT,           /* urgent request */
	__REQ_NR_BITS,		/* stops here */
};

#define REQ_FAILFAST_DEV	(1ULL << __REQ_FAILFAST_DEV)
#define REQ_FAILFAST_TRANSPORT	(1ULL << __REQ_FAILFAST_TRANSPORT)
#define REQ_FAILFAST_DRIVER	(1ULL << __REQ_FAILFAST_DRIVER)
#define REQ_SYNC		(1ULL << __REQ_SYNC)
#define REQ_META		(1ULL << __REQ_META)
#define REQ_PRIO		(1ULL << __REQ_PRIO)
#define REQ_NOIDLE		(1ULL << __REQ_NOIDLE)
#define REQ_INTEGRITY		(1ULL << __REQ_INTEGRITY)
#define REQ_URGENT              (1ULL << __REQ_URGENT)

#define REQ_FAILFAST_MASK \
	(REQ_FAILFAST_DEV | REQ_FAILFAST_TRANSPORT | REQ_FAILFAST_DRIVER)

#define REQ_COMMON_MASK \
	(REQ_FAILFAST_MASK | REQ_SYNC | REQ_META | REQ_PRIO | REQ_NOIDLE | \
	 REQ_PREFLUSH | REQ_FUA | REQ_INTEGRITY | REQ_NOMERGE | REQ_BG | REQ_FG)

#define REQ_CLONE_MASK		REQ_COMMON_MASK

/* This mask is used for both bio and request merge checking */
#define REQ_NOMERGE_FLAGS \
	(REQ_NOMERGE | REQ_STARTED | REQ_SOFTBARRIER | REQ_PREFLUSH | REQ_FUA | REQ_FLUSH_SEQ)

#define REQ_RAHEAD		(1ULL << __REQ_RAHEAD)
#define REQ_THROTTLED		(1ULL << __REQ_THROTTLED)
#define REQ_CHAINED		(1ULL << __REQ_CHAINED)

#define REQ_SORTED		(1ULL << __REQ_SORTED)
#define REQ_SOFTBARRIER		(1ULL << __REQ_SOFTBARRIER)
#define REQ_FUA			(1ULL << __REQ_FUA)
#define REQ_NOMERGE		(1ULL << __REQ_NOMERGE)
#define REQ_STARTED		(1ULL << __REQ_STARTED)
#define REQ_DONTPREP		(1ULL << __REQ_DONTPREP)
#define REQ_QUEUED		(1ULL << __REQ_QUEUED)
#define REQ_ELVPRIV		(1ULL << __REQ_ELVPRIV)
#define REQ_FAILED		(1ULL << __REQ_FAILED)
#define REQ_QUIET		(1ULL << __REQ_QUIET)
#define REQ_PREEMPT		(1ULL << __REQ_PREEMPT)
#define REQ_ALLOCED		(1ULL << __REQ_ALLOCED)
#define REQ_COPY_USER		(1ULL << __REQ_COPY_USER)
#define REQ_PREFLUSH		(1ULL << __REQ_PREFLUSH)
#define REQ_FLUSH_SEQ		(1ULL << __REQ_FLUSH_SEQ)
#define REQ_BG			(1ULL << __REQ_BG)
#define REQ_FG			(1ULL << __REQ_FG)
#define REQ_IO_STAT		(1ULL << __REQ_IO_STAT)
#define REQ_MIXED_MERGE		(1ULL << __REQ_MIXED_MERGE)
#define REQ_PM			(1ULL << __REQ_PM)
#define REQ_HASHED		(1ULL << __REQ_HASHED)
#define REQ_MQ_INFLIGHT		(1ULL << __REQ_MQ_INFLIGHT)

enum req_op {
	REQ_OP_READ,
	REQ_OP_WRITE,
	REQ_OP_DISCARD,		/* request to discard sectors */
	REQ_OP_SECURE_ERASE,	/* request to securely erase sectors */
	REQ_OP_WRITE_SAME,	/* write same block many times */
	REQ_OP_FLUSH,		/* request for cache flush */
};

#define REQ_OP_BITS 3

typedef unsigned int blk_qc_t;
#define BLK_QC_T_NONE	-1U
#define BLK_QC_T_SHIFT	16

struct blk_rq_stat {
	s64 mean;
	u64 min;
	u64 max;
	s64 nr_samples;
	s64 time;
};

static inline bool blk_qc_t_valid(blk_qc_t cookie)
{
	return cookie != BLK_QC_T_NONE;
}

static inline blk_qc_t blk_tag_to_qc_t(unsigned int tag, unsigned int queue_num)
{
	return tag | (queue_num << BLK_QC_T_SHIFT);
}

static inline unsigned int blk_qc_t_to_queue_num(blk_qc_t cookie)
{
	return cookie >> BLK_QC_T_SHIFT;
}

static inline unsigned int blk_qc_t_to_tag(blk_qc_t cookie)
{
	return cookie & ((1u << BLK_QC_T_SHIFT) - 1);
}

#endif /* __LINUX_BLK_TYPES_H */
