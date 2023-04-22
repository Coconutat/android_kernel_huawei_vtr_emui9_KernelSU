#ifndef HISI_BLK_INTERNAL_H
#define HISI_BLK_INTERNAL_H

#define IO_FROM_SUBMIT_BIO_MAGIC 	0x4C
#define IO_FROM_BLK_EXEC			0x4D

#define BLK_LLD_INIT_MAGIC 		0x7A
#define IO_BUF_LEN				10

#define BLK_LLD_AES_256_XTS_KEY_SIZE	64

enum blk_lld_feature_bits {
	__BLK_LLD_DUMP_SUPPORT = 0,
	__BLK_LLD_LATENCY_SUPPORT,
	__BLK_LLD_LATENCY_STATISTIC_ENABLE,
	__BLK_LLD_FLUSH_REDUCE_SUPPORT,
	__BLK_LLD_BUSYIDLE_SUPPORT,
	__BLK_LLD_INLINE_CRYPTO_SUPPORT,
	__BLK_LLD_IOSCHED_UFS_MQ,
	__BLK_LLD_IOSCHED_UFS_HW_IDLE,
};

#define BLK_LLD_DUMP_SUPPORT				(1ULL << __BLK_LLD_DUMP_SUPPORT)
#define BLK_LLD_LATENCY_SUPPORT			(1ULL << __BLK_LLD_LATENCY_SUPPORT)
#define BLK_LLD_LATENCY_STATISTIC_ENABLE	(1ULL << __BLK_LLD_LATENCY_STATISTIC_ENABLE)
#define BLK_LLD_FLUSH_REDUCE_SUPPORT		(1ULL << __BLK_LLD_FLUSH_REDUCE_SUPPORT)
#define BLK_LLD_BUSYIDLE_SUPPORT			(1ULL << __BLK_LLD_BUSYIDLE_SUPPORT)
#define BLK_LLD_INLINE_CRYPTO_SUPPORT		(1ULL << __BLK_LLD_INLINE_CRYPTO_SUPPORT)
#define BLK_LLD_IOSCHED_UFS_MQ				(1ULL << __BLK_LLD_IOSCHED_UFS_MQ)
#define BLK_LLD_IOSCHED_UFS_HW_IDLE				(1ULL << __BLK_LLD_IOSCHED_UFS_HW_IDLE)

#define BLK_QUEUE_DURATION_UNIT_NS	10000

enum blk_busy_idle_nb_flag {
	BLK_BUSY_IDLE_NB_NOT_JOIN_POLL = 0,
};

#define BLK_BUSY_IDLE_NB_FLAG_NOT_JOIN_POLL			(1<<BLK_BUSY_IDLE_NB_NOT_JOIN_POLL)

enum blk_queue_io_duration_statistic_type {
	IO_STATISTIC_INVALID = 0,
	IO_STATISTIC_READ,
	IO_STATISTIC_WRITE,
	IO_STATISTIC_DISCARD,
	IO_STATISTIC_FLUSH,
	IO_STATISTIC_END,
};

enum iosched_strategy {
	IOSCHED_NONE = 0,
	IOSCHED_HISI_UFS_MQ,
};

struct blk_mq_ctx;
struct blk_mq_alloc_data;

/*
* IO Scheduler Operation Function Pointer
*/
struct blk_queue_ops {
	enum iosched_strategy io_scheduler_strategy;
	int (*mq_iosched_init_fn)(struct request_queue *);  /* MQ Scheduler Init */
	int (*mq_iosched_deinit_fn)(struct request_queue *); /* MQ Scheduler Deinit */
	int (*mq_req_alloc_prep_fn)(struct blk_mq_alloc_data *, struct request_queue *,
		struct blk_mq_ctx *, struct blk_mq_hw_ctx *, unsigned long, bool); /*Prepare for MQ reqeust allocation */
	int (*mq_req_init_fn)(struct request_queue *, struct blk_mq_ctx *, struct request *); /* Request init in MQ */
	int (*mq_req_complete_fn)(struct request *, struct request_queue *, bool); /* Request complete in MQ */
	int (*mq_req_deinit_fn)(struct request_queue *, struct request *); /* Request deinit in MQ */
	int (*mq_req_insert_fn)(struct request *req, struct request_queue *, bool); /* Request insert to MQ */
	int (*mq_req_requeue_fn)(struct request *, struct request_queue *); /* Request requeue in MQ */
	int (*mq_req_timeout_fn)(struct request *, bool); /* Request timeout process in MQ */
	int (*mq_ctx_put_fn)(struct blk_mq_ctx *); /* Release the CTX in MQ */
	int (*mq_hctx_get_by_req_fn)(struct request *, struct blk_mq_hw_ctx **); /* Get hctx object by request */
	int (*mq_hctx_free_in_ctx_map_fn)(struct request_queue *, struct blk_mq_hw_ctx *); /* Free hctx during ctx map */
	int (*mq_tag_get_fn)(struct blk_mq_alloc_data *, unsigned int *); /* Get tag in MQ */
	int (*mq_tag_put_fn)(struct blk_mq_hw_ctx *, struct blk_mq_ctx *, unsigned int, struct request *); /* Release tag in MQ */
	int (*mq_tag_update_depth_fn)(struct request_queue *, struct blk_mq_tags *, unsigned int); /* Update the tag depth in MQ */
	int (*mq_tag_busy_iter_fn)(struct request_queue *, busy_iter_fn *,void *); /* iterate all the busy tag */
	int (*mq_tag_wakeup_all_fn)(struct blk_mq_tags *); /* wakeup all threads waiting tag */
	int (*mq_tag_sysfs_show_fn)(struct request_queue *, struct blk_mq_tags *, char *, ssize_t *); /* Show tag used stats in SYSFS */
	int (*mq_exec_queue_fn)(struct request_queue *); /* Execute queue function directly in MQ */
	int (*mq_run_hw_queue_fn)(struct request_queue *); /* Run hw queue in MQ */
	int (*mq_run_delay_queue_fn)(struct request_queue *); /* Run delay queue in MQ */
	int (*mq_run_requeue_fn)(struct request_queue *); /* Run requeue in MQ */
	int (*blk_poll_enable_fn)(struct request_queue *, blk_qc_t, bool *); /* Enable poll */
	void (*blk_status_dump_fn)(struct request_queue *, unsigned char *, enum blk_dump_scenario); /* Dump ioscheduler status*/
	void (*blk_req_dump_fn)(struct request_queue *, unsigned char *, enum blk_dump_scenario); /* Dump req status in ioscheduler */
	void *io_scheduler_private_data;
};

/*
* MQ tagset Operation Function Pointer
*/
struct blk_tagset_ops {
	int (*tagset_init_tags_fn)(struct blk_mq_tag_set *, unsigned int, unsigned int, unsigned int, int,
		int, struct blk_mq_tags **); /* MQ tagset init */
	int (*tagset_free_tags_fn)(struct blk_mq_tags *); /* MQ tagset free */
	int (*tagset_all_tag_busy_iter_fn)(struct blk_mq_tags *, busy_tag_iter_fn *, void *);/* iterate all the busy tag in whole tagset */
	struct blk_queue_ops *queue_ops;
};

struct io_length_grouping {
	unsigned char io_type;
	unsigned int page_num;
	int index;
	char* lab_str;
};

struct bio_delay_stage_config {
	char *stage_name;
	void (*function)(struct bio *bio);
};

struct req_delay_stage_config {
	char *stage_name;
	void (*function)(struct request *req);
};

#ifdef CONFIG_HISI_BLK
extern void hisi_blk_bio_queue_split(struct request_queue *q, struct bio **bio, struct bio *split);
extern void hisi_blk_queue_bounce(struct request_queue *q, struct bio *bio_orig, struct bio *bio_cloned);
extern void hisi_blk_bio_split_pre(struct bio *bio, struct bio *split);
extern void hisi_blk_bio_split_post(struct bio *bio, struct bio *split);
extern bool hisi_blk_bio_merge_allow(struct request *rq, struct bio *bio);
extern void hisi_blk_bio_merge_done(struct request_queue *q, struct request *req, struct request *next);
extern int hisi_blk_account_io_completion(struct request *req, unsigned int bytes);
extern int hisi_blk_generic_make_request_check(struct bio *bio);
extern void hisi_blk_generic_make_request(struct bio *bio);
extern void hisi_blk_start_plug(struct blk_plug *plug);
extern void hisi_blk_flush_plug_list(struct blk_plug *plug, bool from_schedule);
extern void hisi_blk_bio_endio(struct bio *bio);
extern void hisi_blk_bio_free(struct bio *bio);

extern void hisi_blk_request_init_from_bio(struct request *req, struct bio *bio);
extern void hisi_blk_insert_cloned_request(struct request_queue *q, struct request *rq);
extern void hisi_blk_request_execute_nowait(struct request_queue *q, struct gendisk *bd_disk,
			   struct request *rq, int at_head, rq_end_io_fn *done);
extern void hisi_blk_mq_rq_ctx_init(struct request_queue *q, struct blk_mq_ctx *ctx, struct request *rq);
extern void hisi_blk_mq_request_start(struct request *rq);
extern void hisi_blk_request_start(struct request *req);
extern void hisi_blk_requeue_request(struct request_queue *q, struct request *rq);
extern void hisi_blk_request_update(struct request *req, int error, unsigned int nr_bytes);
extern void hisi_blk_req_bio_endio(struct request *rq, struct bio *bio, unsigned int nbytes, int error);
extern void hisi_blk_mq_request_free(struct request_queue *q, struct request *rq);
extern void hisi_blk_request_put(struct request_queue *q, struct request *req);

extern void hisi_blk_check_partition_done(struct gendisk *disk, bool has_part_tbl);
extern void hisi_blk_queue_register(struct request_queue *q, struct gendisk *disk);
extern void hisi_blk_mq_init_allocated_queue(struct request_queue *q);
extern void hisi_blk_sq_init_allocated_queue(struct request_queue *q);
extern void hisi_blk_allocated_queue_init(struct request_queue *q);
extern void hisi_blk_mq_free_queue(struct request_queue *q);
extern void hisi_blk_cleanup_queue(struct request_queue *q);

extern void hisi_blk_mq_allocated_tagset_init(struct blk_mq_tag_set *set);
extern void blk_add_queue_tags(struct blk_queue_tag *tags, struct request_queue *q);
extern void hisi_blk_allocated_tags_init(struct blk_queue_tag *tags);

extern int hisi_blk_dev_init(void);

extern struct blk_dev_lld *hisi_blk_get_lld(struct request_queue *q);
extern struct request_queue *hisi_blk_get_queue_by_lld(struct blk_dev_lld *lld);

extern void hisi_blk_busy_idle_check_bio(struct request_queue *q, struct bio *bio);
extern bool hisi_blk_busy_idle_check_request_bio(struct request_queue *q, struct request *rq);
extern void hisi_blk_busy_idle_check_execute_request(struct request_queue *q, struct request *rq,rq_end_io_fn *done);
extern void hisi_blk_busy_idle_check_bio_endio(struct bio *bio);
extern void hisi_blk_busy_idle_state_init(struct blk_idle_state *blk_idle);

extern void hisi_blk_latency_init(void);
extern void hisi_blk_queue_latency_init(struct request_queue *q);
extern void hisi_blk_queue_latency_average_calc(struct request_queue *q);
extern void hisi_blk_queue_latency_statistic_clear(struct request_queue *q);
extern void hisi_blk_latency_bio_check(struct bio *bio, enum bio_process_stage_enum bio_stage);
extern void hisi_blk_latency_req_check(struct request *req,enum req_process_stage_enum req_stage);
extern void hisi_blk_latency_for_merge(struct request *req, struct request *next);
extern void hisi_blk_latency_writeback_bio(struct request *req, struct bio *bio);

extern void hisi_blk_queue_async_flush_init(struct request_queue *q);
extern bool hisi_blk_flush_sync_dispatch(struct request_queue *q, struct bio *bio);
extern void hisi_blk_flush_update(struct request *req, int error);

extern void hisi_blk_dump_register_queue(struct request_queue *q);
extern void hisi_blk_dump_unregister_queue(struct request_queue *q);
extern void hisi_blk_dump_bio(struct bio *bio, unsigned char *prefix);
extern void hisi_blk_dump_request(struct request *rq, unsigned char *prefix);
extern void hisi_blk_dump_queue_status(struct request_queue *q, unsigned char *prefix, enum blk_dump_scenario scenario);
extern unsigned long hisi_blk_dump_queue_status2(struct request_queue *q, char *out_buf, unsigned long length, bool need_print);
extern unsigned long hisi_blk_dump_lld_status(struct blk_dev_lld *lld, char *out_buf, unsigned long length, bool need_print);
extern void hisi_blk_dump_init(void);

extern void hisi_blk_queue_usr_ctrl_set(struct request_queue *q);
extern int hisi_blk_busy_idle_event_register(struct blk_dev_lld *lld, struct blk_busy_idle_nb *notify_nb);
extern int hisi_blk_busy_idle_event_unregister(struct blk_dev_lld *lld, struct blk_busy_idle_nb *notify_nb);

#if defined(CONFIG_HISI_DEBUG_FS) || defined(CONFIG_HISI_BLK_DEBUG)
extern ssize_t hisi_queue_status_show(struct request_queue *q, char *page);
extern ssize_t hisi_queue_io_latency_warning_threshold_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_io_latency_statistic_enable_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_io_latency_statistic_show(struct request_queue *q, char *page);
extern ssize_t hisi_queue_io_hw_latency_statistic_show(struct request_queue *q, char *page);
extern ssize_t hisi_queue_io_sw_latency_statistic_show(struct request_queue *q, char *page);
extern ssize_t hisi_queue_busy_idle_enable_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_busy_idle_statistic_reset_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_busy_idle_statistic_show(struct request_queue *q, char *page);

extern ssize_t hisi_queue_timeout_tst_enable_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_io_latency_tst_enable_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_busy_idle_tst_enable_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_busy_idle_multi_nb_tst_enable_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_busy_idle_tst_proc_result_simulate_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_busy_idle_tst_proc_latency_simulate_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_apd_tst_enable_store(struct request_queue *q, const char *page, size_t count);
extern ssize_t hisi_queue_suspend_tst_store(struct request_queue *q, const char *page, size_t count);
#endif
extern ssize_t queue_var_store(unsigned long *var, const char *page, size_t count);
#endif

#endif
