#ifndef _HISI_DRG_H
#define _HISI_DRG_H

#include <linux/cpufreq.h>
#include <linux/devfreq.h>
#include <linux/hisi/hisi_perf_ctrl.h>

#define MAX_DRG_MARGIN		1024U
#define DRG_CAPACITY_THRES	500L

#ifdef CONFIG_HISI_DRG
extern void drg_cpufreq_register(struct cpufreq_policy *policy);
extern void drg_cpufreq_unregister(struct cpufreq_policy *policy);
extern void drg_devfreq_register(struct devfreq *df);
extern void drg_devfreq_unregister(struct devfreq *df);
extern void drg_cpufreq_cooling_update(unsigned int cpu,
				       unsigned int clip_freq);
extern void drg_devfreq_cooling_update(struct devfreq *df,
				       unsigned long clip_freq);
extern unsigned int drg_cpufreq_check_limit(struct cpufreq_policy *policy,
					    unsigned int target_freq);
extern unsigned long drg_devfreq_check_limit(struct devfreq *df,
					     unsigned long target_freq);
extern unsigned int drg_get_cpu_margin(unsigned int cpu);

extern int drg_get_freq_range(struct drg_dev_freq *freq);
#else
static inline void drg_cpufreq_register(struct cpufreq_policy *policy) {}
static inline void drg_cpufreq_unregister(struct cpufreq_policy *policy) {}
static inline void drg_devfreq_register(struct devfreq *df) {}
static inline void drg_devfreq_unregister(struct devfreq *df) {}
static inline void drg_cpufreq_cooling_update(unsigned int cpu,
					      unsigned int clip_freq) {}
static inline void drg_devfreq_cooling_update(struct devfreq *df,
					      unsigned long clip_freq) {}
static inline unsigned int drg_cpufreq_check_limit(
		struct cpufreq_policy *policy, unsigned int target_freq) {
	return target_freq;
}
static inline unsigned long drg_devfreq_check_limit(
		struct devfreq *df, unsigned long target_freq) {
	return target_freq;
}
static inline  unsigned int drg_get_cpu_margin(unsigned int cpu) {return MAX_DRG_MARGIN;}

extern int drg_get_freq_range(struct drg_dev_freq *freq) {return 0;}
#endif

extern bool hisi_cluster_cpu_all_pwrdn(void);
#endif /* _HISI_DRG_H */
