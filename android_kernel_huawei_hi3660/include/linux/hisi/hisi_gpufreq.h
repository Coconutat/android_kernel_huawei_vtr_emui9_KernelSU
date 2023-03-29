#ifndef _HISI_GPUFREQ_H
#define _HISI_GPUFREQ_H


#ifdef CONFIG_HISI_GPU_FHSS
int fhss_init(struct device *dev);
#else
static inline int fhss_init(struct device *dev){return 0;};
#endif

#endif /* _HISI_GPUFREQ_H */
