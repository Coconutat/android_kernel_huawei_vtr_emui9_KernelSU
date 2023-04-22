#ifndef _HISI_DEVFREQ_H
#define _HISI_DEVFREQ_H
#include <linux/version.h>
/**
 * struct hisi_devfreq_data - Private data given from hisi devfreq user device
 * 							  to governors.
 * @vsync_hit: Indicate whether vsync signal is hit or miss.
 * @cl_boost: Indicate whether to boost the freq in OpenCL scene.
 */
struct hisi_devfreq_data {
	int vsync_hit;
	int cl_boost;
};

#ifdef CONFIG_DEVFREQ_GOV_GPU_SCENE_AWARE
#define GPU_DEFAULT_GOVERNOR	"gpu_scene_aware"
#else
#define GPU_DEFAULT_GOVERNOR	"mali_ondemand"
#endif

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
int hisi_devfreq_free_freq_table(struct device *dev, unsigned long **table);
int hisi_devfreq_init_freq_table(struct device *dev, unsigned long **table);
#else
int hisi_devfreq_free_freq_table(struct device *dev, unsigned int **table);
int hisi_devfreq_init_freq_table(struct device *dev, unsigned int **table);
#endif

#endif /* _HISI_DEVFREQ_H */
