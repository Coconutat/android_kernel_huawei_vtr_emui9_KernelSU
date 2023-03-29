
#ifndef __HISI_EAGLE_EYE_H
#define __HISI_EAGLE_EYE_H

/*unit:nanosecond, keep with (watchdog_thresh*2)/5 in kernel/watchdog.c*/
#define ALARM_DETECT_TIMEOUT	(4L*1000000000L)

#define EAGLE_EYE_RT_PRIORITY	97

struct alarm_detect_info {
	struct list_head  list;
	u32 reason;/* 预警原因或类型*/
	u32 alarm_cpu;
	void *data;

	/*Constraints:	此函数在原子上下文，禁止使用会睡眠的函数*/
	int (*detect)(struct alarm_detect_info *info);
	int (*process)(struct alarm_detect_info *info);/*运行在非原子上下文*/
	char *desc;
};

enum early_alarm_type {
	EARLY_ALARM_NORMAL     = 0x0,
	EARLY_ALARM_AWDT_NO_KICK   = 0x01,
	EARLY_ALARM_TASK_NO_SCHED     = 0x02,
	EARLY_ALARM_TEST      = 0x03,
	EARLY_ALARM_MAX
};

enum alarm_id {
	EARLY_ALARM_AWDT_NO_KICK_ID	= 0x0,
#ifdef CONFIG_SCHEDSTATS
	EARLY_ALARM_TASK_NO_SCHED_ID,
#endif
	EARLY_ALARM_TEST_ID,
	EARLY_ALARM_ID_MAX
};

#ifdef CONFIG_HISI_EAGLE_EYE
extern int eeye_register_alarm_detect_function(struct alarm_detect_info *info);
extern int  eeye_early_alarm(u32 reason, u32 cpu);
extern int  eeye_alarm_detect(void);
bool eeye_comm_init(void);
int  eeye_is_alarm_detecting(int cpu);
#else
static inline int eeye_register_alarm_detect_function(
	struct alarm_detect_info *info) { return 0; }
static inline int  eeye_early_alarm(u32 reason, u32 cpu) { return 0; }
static inline bool eeye_comm_init(void) { return false; }
static inline int  eeye_alarm_detect(void) { return 0; }
static inline int  eeye_is_alarm_detecting(int cpu) { return 0; }
#endif

#endif /*__HISI_EAGLE_EYE_H*/

