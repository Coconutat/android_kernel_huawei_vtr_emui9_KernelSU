#ifndef BATT_ACR_DETECT_H
#define BATT_ACR_DETECT_H

#define ACR_FALSE 0
#define ACR_TRUE  1

#define ACR_RT_NOT_SUPPORT 0
#define ACR_RT_RETRY_TIMES 50
#define ACR_RT_CURRENT_MIN 10 //10ma
#define ACR_RT_THRESHOLD_DEFAULT 100
#define ACR_RT_FMD_MIN_DEFAULT 40
#define ACR_RT_FMD_MAX_DEFAULT 90

#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
#define ACR_RT_DEFAULT_VERSION	1
#define ACR_RT_ITEM_ID			703023001
#define ACR_RT_FMD_FAIL			"fail"
#define ACR_RT_FMD_PASS			"pass"
#define ACR_RT_NA				"NA"
#define ACR_RT_DEVICE_NAME		"battery"
#define ACR_RT_TEST_NAME		"BATT_ACR_VAL"
#endif

struct acr_device_info {
	struct device *dev;
	struct acr_info acr_data;
	struct delayed_work acr_rt_work;
#ifdef CONFIG_HUAWEI_DATA_ACQUISITION
	struct message acr_msg;
#endif
	int acr_rt_support;
	int acr_rt_status;
	int acr_rt_threshold;
	int acr_rt_fmd_min;
	int acr_rt_fmd_max;
};

enum acr_sysfs_type {
	ACR_SYSFS_ACR_RT_SUPPORT = 0,
	ACR_SYSFS_ACR_RT_DETECT,
};

enum acr_rt_status_type {
	ACR_RT_STATUS_INIT = 0,
	ACR_RT_STATUS_FAIL,
	ACR_RT_STATUS_SUCC,
};

#endif
