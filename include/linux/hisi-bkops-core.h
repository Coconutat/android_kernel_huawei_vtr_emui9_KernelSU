#ifndef HISI_BKOPS_CORE_H
#define HISI_BKOPS_CORE_H

#include <linux/workqueue.h>
#include <linux/blkdev.h>

enum bkops_operation{
	BKOPS_STOP = 0,
	BKOPS_START,
};

enum hisi_bkops_flag {
	BKOPS_START_STATUS = 0,
	BKOPS_QUERY_NEEDED,
	BKOPS_ASYNC_WORK_STARTED,
	BKOPS_CHK_TIME_INTERVAL,
	BKOPS_CHK_ACCU_WRITE,
	BKOPS_CHK_ACCU_DISCARD
};
#define BKOPS_FLAG_BKOPS_QUERY_NEEDED			(1 << BKOPS_QUERY_NEEDED)
#define BKOPS_FLAG_ASYNC_WORK_STARTED			(1 << BKOPS_ASYNC_WORK_STARTED)
#define BKOPS_FLAG_BKOPS_CHK_TIME_INTERVAL		(1 << BKOPS_CHK_TIME_INTERVAL)
#define BKOPS_FLAG_BKOPS_CHK_ACCU_WRITE			(1 << BKOPS_CHK_ACCU_WRITE)
#define BKOPS_FLAG_BKOPS_CHK_ACCU_DISCARD		(1 << BKOPS_CHK_ACCU_DISCARD)

#ifdef CONFIG_HISI_DEBUG_FS
struct bkops_debug_ops {
	bool sim_bkops_start_fail;
	bool sim_bkops_stop_fail;
	bool sim_bkops_query_fail;
	bool sim_critical_bkops;
	bool sim_bkops_abort;
	u32 sim_bkops_stop_delay; /* in ms */
	bool skip_bkops_stop;
	bool disable_bkops;
	bool bkops_force_query;
	u32 sim_bkops_query_delay; /* in ms */
};
#endif /* CONFIG_HISI_DEBUG_FS */

typedef enum {
	BKOPS_100MS,
	BKOPS_500MS,
	BKOPS_1000MS,
	BKOPS_2000MS,
	BKOPS_5000MS,
	BKOPS_FOR_AGES,
	BKOPS_DUR_NUM,
} bkops_dur_enum;

#define	BKOPS_STATUS_NUM_MAX	10
struct bkops_stats {
	u32 bkops_retry_count;
	u32 bkops_start_count;
	u32 bkops_stop_count;
	u32 bkops_core_stop_count; /* how many manual bkops has been stopped by bkops core framework */
	u32 bkops_abort_count; /* bkops was interrupted before completed */
	u32 bkops_actual_query_count;
	u32 bkops_idle_work_canceled_count;
	u32 bkops_idle_work_waited_count;
	u32 bkops_time_query_count;
	u32 bkops_write_query_count;
	u32 bkops_discard_query_count;
	u32 bkops_query_fail_count;
	u32 bkops_start_fail_count;
	u32 bkops_stop_fail_count;
	u64 bkops_start_time; /* last bkops start time in ns */

	u64 bkops_max_query_time; /* in ns */
	u64 bkops_avrg_query_time; /* in ns */
	u64 bkops_max_start_time; /* in ns */
	u64 bkops_avrg_start_time; /* in ns */
	u64 bkops_max_stop_time; /* in ns */
	u64 bkops_avrg_stop_time; /* in ns */
	u64 max_bkops_duration; /* in ns */
	u64 bkops_avrg_exe_time; /* in ns */

	u32 bkops_dur[BKOPS_DUR_NUM]; /* in ms */
	u32 bkops_status[BKOPS_STATUS_NUM_MAX];
	u32 bkops_status_max;
	char **bkops_status_str;
};

enum bkops_dev_type {
	BKOPS_DEV_NONE,
	BKOPS_DEV_MMC,
	BKOPS_DEV_UFS_1861,
	BKOPS_DEV_UFS_HYNIX,
	BKOPS_DEV_TYPE_MAX,
};

struct hisi_bkops {
	enum bkops_dev_type dev_type;
	struct bkops_ops *bkops_ops;
	void *bkops_data;
	struct request_queue *q;
	struct delayed_work bkops_idle_work;
	unsigned long bkops_flag;
	unsigned long bkops_idle_delay_ms;
	struct blk_busy_idle_nb bkops_nb;

	u32 bkops_status; /* bkops status of last query */
	unsigned int en_bkops_retry;
	unsigned long bkops_check_interval; /* in seconds */
	unsigned long last_bkops_query_time; /* in seconds */

	unsigned long bkops_check_discard_len; /* in bytes */
	unsigned long last_discard_len; /* in bytes */

	unsigned long bkops_check_write_len; /* in bytes */
	unsigned long last_write_len; /* in bytes */

	struct bkops_stats bkops_stats;
#ifdef CONFIG_HISI_DEBUG_FS
	struct dentry *bkops_root;
	struct bkops_debug_ops bkops_debug_ops;
#endif

};

typedef int (bkops_start_stop_fn)(void *bkops_data, int start);
typedef int (bkops_status_query_fn)(void *bkops_data, u32* status);
struct bkops_ops {
	bkops_start_stop_fn *bkops_start_stop;
	bkops_status_query_fn *bkops_status_query;
};

#define BKOPS_DEF_IDLE_DELAY	1000 /* in ms */
#define BKOPS_DEF_CHECK_INTERVAL	(60 * 60) /* in seconds */
#define BKOPS_DEF_DISCARD_LEN	(512 * 1024 * 1024) /* in bytes */
#define BKOPS_DEF_WRITE_LEN		(512 * 1024 * 1024) /* in bytes */

extern struct hisi_bkops *hisi_bkops_alloc(void);
extern int hisi_bkops_set_status_info(struct hisi_bkops *bkops, u32 bkops_status_max, char **bkops_status_str);
extern int hisi_bkops_enable(struct request_queue *q, struct hisi_bkops *bkops, struct dentry *debugfs_parent_dir);
extern int hisi_bkops_add_debugfs(struct hisi_bkops *bkops, struct dentry *bkops_root);
extern void hisi_bkops_update_dur(struct bkops_stats *bkops_stats_p);

#endif /* HISI_BKOPS_CORE_H */

