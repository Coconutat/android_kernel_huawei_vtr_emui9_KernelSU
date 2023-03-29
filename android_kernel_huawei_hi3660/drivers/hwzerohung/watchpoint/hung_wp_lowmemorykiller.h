#ifndef __HUNG_WP_LOWMEMORYKILLER_H_
#define __HUNG_WP_LOWMEMORYKILLER_H_


#ifdef __cplusplus
extern "C" {
#endif

#define MAX_WP_BUFSIZE (8*1024)
#define MAX_EVENTS (64)

#if defined LMKWP_DEBUG
#define LMKWP_WARN(format, args...)			\
	vprintf(format, ##args)
#define LMKWP_INFO(format, args...)			\
	vprintf(format, ##args)
#else

#define LMKWP_WARN(format, args...)			\
	printk(KERN_WARNING "lmkwp: " format, ##args)
#define LMKWP_INFO(format, args...)			\
	printk(KERN_INFO "lmkwp: " format, ##args)
#endif

typedef struct lmkwp_event {
	u64      stamp; /* jiffies */
	struct shrink_control   sc;
	pid_t                   selected_pid;
	pid_t                   selected_tgid;
	char                    selected_comm[TASK_COMM_LEN];
	pid_t                   cur_pid;
	pid_t                   cur_tgid;
	char                    cur_comm[TASK_COMM_LEN];
	long                    cache_size;
	long                    cache_limit;
	long                    free;
	short                   adj;
} lmkwp_event_t;

typedef struct lmkwp_config {
	u8      is_ready;
	u8      enabled;
	u8		debuggable;
	unsigned int     threshold;
	u64     period;  /* jiffies */
	u64     silence;
} lmkwp_config_t;

typedef struct lmkwp_main {
	lmkwp_config_t  config;
	lmkwp_event_t   events[MAX_EVENTS];
	unsigned int    free;
	u64             last_report_stamp;
} lmkwp_main_t;

#ifdef __cplusplus
}
#endif
#endif
