#ifndef __SAMPLE_CTRL_H__
#define __SAMPLE_CTRL_H__

#ifdef CONFIG_HW_NETWORK_MEASUREMENT
#define NM_SAMPLE_VALVE_BUF_MAX		64
#define NM_SAMPLE_UID_BUF_MAX		1024
#define VALVE_CLOSED			0U
#define VALVE_OPEN			1U
#define VALVE_TS_SHIFT			1U

static inline int nm_turn_on_valve(struct timespec *__tv)
{
	struct timespec tv;

	if (__tv) {
		return ((u32)(__tv->tv_sec) << VALVE_TS_SHIFT) + VALVE_OPEN;
	} else {
		getnstimeofday(&tv);
		return ((u32)tv.tv_sec << VALVE_TS_SHIFT) + VALVE_OPEN;
	}
}

static inline int nm_shut_off_valve(struct timespec *__tv)
{
	struct timespec tv;

	if (__tv) {
		return (u32)(__tv->tv_sec) << VALVE_TS_SHIFT;
	} else {
		getnstimeofday(&tv);
		return (u32)tv.tv_sec << VALVE_TS_SHIFT;
	}
}

extern int proc_sample_uid_list(struct ctl_table *ctl, int write,
				void __user *buffer, size_t *lenp,
				loff_t *ppos);
extern int proc_sample_valve(struct ctl_table *ctl, int write,
			     void __user *buffer, size_t *lenp, loff_t *ppos);
extern bool in_sample_uid_list(unsigned int uid);
#endif /* CONFIG_HW_NETWORK_MEASUREMENT */
#endif /* __SAMPLE_LIST_H__ */
