/*
 * drivers/power/charging_core.h
 *
 *charging core driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/
#ifndef _CHARGING_CORE
#define _CHARGING_CORE

#include <linux/device.h>	/*for struct charge_core_info */
#ifdef CONFIG_HISI_BATTERY_DATA
#include <linux/power/hisi/hisi_battery_data.h>	/*for struct charge_core_info */
#endif
#include <huawei_platform/power/huawei_charger.h>	/*for struct charge_core_info */

/*************************marco define area***************************/
#define VDPM_BY_CAPACITY            (0)
#define VDPM_BY_VOLTAGE             (1)

#define VDPM_PARA_LEVEL             (5)
#define VDPM_CBAT_MIN               (-32767)
#define VDPM_CBAT_MAX               (32767)
#define VDPM_VOLT_MIN               (3880)
#define VDPM_VOLT_MAX               (5080)
#define VDPM_DELTA_LIMIT_5          (5)

#define INDUCTANCE_PARA_LEVEL       (4)
#define INDUCTANCE_CBAT_MIN               (-32767)
#define INDUCTANCE_CBAT_MAX               (32767)
#define INDUCTANCE_IIN_MIN               (0)
#define INDUCTANCE_IIN_MAX               (5000)
#define INDUCTANCE_CAP_DETA               (5)

#define SEGMENT_TYPE_BY_VBAT_ICHG (0)
#define SEGMENT_TYPE_BY_VBAT (1)

#define INVALID_CURRENT_SET (0)
#define DEFAULT_BASP_POLICY_TYPE (0)
#define SEGMENT_BASP_POLICY_TYPE (1)
#define BASP_VOL_SEGMENT_COLUMN (6)

#define OVERHEAT_TIMES (2)

#define FIRST_RUN_TRUE               (1)
#define FIRST_RUN_FALSE              (0)

#define BATT_BRAND_STRING_MAX        (32)
#define BATT_BRAND_NUM_MAX           (5)

/*************************struct define area***************************/
enum vdpm_para_info {
	VDPM_PARA_CAP_MIN = 0,
	VDPM_PARA_CAP_MAX,
	VDPM_PARA_DPM,
	VDPM_PARA_CAP_BACK,
	VDPM_PARA_TOTAL,
};
enum inductance_para_info {
	INDUCTANCE_PARA_CAP_MIN = 0,
	INDUCTANCE_PARA_CAP_MAX,
	INDUCTANCE_PARA_IIN,
	INDUCTANCE_PARA_CAP_BACK,
	INDUCTANCE_PARA_TOTAL,
};

struct charge_temp_data {
	int temp_min;
	int temp_max;
	int iin_temp;
	int ichg_temp;
	int vterm_temp;
	int temp_back;
};

struct charge_volt_data {
	int vbat_min;
	int vbat_max;
	int iin_volt;
	int ichg_volt;
	int volt_back;
};

struct charge_vdpm_data {
	int cap_min;
	int cap_max;
	int vin_dpm;
	int cap_back;
};

struct charge_segment_data {
	int vbat_min;
	int vbat_max;
	int ichg_segment;
	int vterm_segment;
	int volt_back;
};

struct charge_inductance_data {
	int cap_min;
	int cap_max;
	int iin_inductance;
	int cap_back;
};

/*charge terminal current is different, based on battery model*/
enum {
	CHARGE_ITERM_PARA_BATT_BRAND,
	CHARGE_ITERM_PARA_ITERM,
	CHARGE_ITERM_PARA_TOTAL,
};
struct charge_core_info {
	struct device *dev;
	struct charge_temp_data temp_para[TEMP_PARA_LEVEL];
	struct charge_volt_data volt_para[VOLT_PARA_LEVEL];
	struct charge_vdpm_data vdpm_para[VDPM_PARA_LEVEL];
	struct charge_segment_data segment_para[SEGMENT_PARA_LEVEL];
	struct charge_inductance_data inductance_para[VDPM_PARA_LEVEL];
	struct charge_core_data data;
};

/****************variable and function declarationn area******************/
struct charge_core_data *charge_core_get_params(void);
void stop_charging_core_config(void);

#endif
