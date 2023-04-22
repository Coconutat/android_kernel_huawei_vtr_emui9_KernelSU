/* hisi_cpufreq_lowtemp.h */

#ifndef __HISI_CPUFREQ_LOWTEMP_H__
#define __HISI_CPUFREQ_LOWTEMP_H__

#ifdef CONFIG_HISI_CPUFREQ_LOWTEMP
/*return true if low temperature, false if not*/
bool is_low_temprature(void);

#else
static inline bool is_low_temprature(void) { return false; }
#endif

#endif /* __HISI_CPUFREQ_LOWTEMP_H__ */
