#ifndef _HISI_PERF_CTRL_H
#define _HISI_PERF_CTRL_H

#define DEV_TYPE_CPU_CLUSTER (1 << 0) << 8

enum drg_dev_type {
	DRG_NONE_DEV = 0,
	/* CPU TYPE if new cpu add after(eg. small strong ...) */
	DRG_CPU_CLUSTER0 = (1 << 0) << 8,
	DRG_CPU_CLUSTER1,
	DRG_CPU_CLUSTER2,
	/* CACHE TYPE if new cache add after(eg. l4 l5 l6 ...)*/
	DRG_L3_CACHE = (1 << 1) << 8,
	/* If new type add after here */
};

struct drg_dev_freq {
	/* enum drg_dev_type */
	unsigned int type;
	/* described as Hz  */
	unsigned long max_freq;
	unsigned long min_freq;
};

static inline bool devtype_name_compare(enum drg_dev_type type,const char *dev_name) {
	if (type == DRG_L3_CACHE && strcasecmp(dev_name, "l3c_devfreq") == 0) {
		return true;
	}
	return false;
}

enum thermal_cdev_type {
	CDEV_GPU = 0,
	CDEV_CPU_CLUSTER0,
	CDEV_CPU_CLUSTER1,
	CDEV_CPU_CLUSTER2,
	THERMAL_CDEV_MAX,
};

enum thermal_zone_type {
	SOC_THERMAL_ZONE = 0,
	BOARD_THERMAL_ZONE,
	THERMAL_ZONE_MAX,
};

struct thermal_cdev_power {
	int thermal_zone_type;
	unsigned int cdev_power[THERMAL_CDEV_MAX];
};

#endif /* _HISI_PERF_CTRL_H */
