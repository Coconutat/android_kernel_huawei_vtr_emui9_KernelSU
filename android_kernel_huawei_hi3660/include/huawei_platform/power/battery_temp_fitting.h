#ifndef _BATTERY_TEMP_FITTING_H_
#define _BATTERY_TEMP_FITTING_H_

#ifdef CONFIG_HUAWEI_BATTERY_TEMP_FITTING

#include <linux/thermal.h>

#define BTF_TZ_TYPE_LENGTH   (20)

struct btf_tz_type_para {
	struct thermal_zone_device *tz;
	char tz_type[BTF_TZ_TYPE_LENGTH];
};

struct btf_ichg_para {
	int ichg;
};

struct btf_fitting_para {
	int value;
};

struct btf_device {
	struct platform_device *pdev;
	struct device *dev;

	int total_tz_type;
	int total_ichg;
	int total_fitting_data;

	struct btf_tz_type_para * tz_type_para;
	struct btf_ichg_para * ichg_para;
	struct btf_fitting_para * fitting_para;
};

int btf_get_battery_temp_with_current(int ichg, int *temp);

#else

static inline int btf_get_battery_temp_with_current(int ichg, int *temp)
{
	return -1;
}

#endif /* end of CONFIG_HUAWEI_BATTERY_TEMP_FITTING */

#endif /* end of _BATTERY_TEMP_FITTING_H_ */
