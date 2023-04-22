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
#ifndef _CHARGING_CORE_SENSORHUB
#define _CHARGING_CORE_SENSORHUB

#include <linux/device.h>	/*for struct charge_core_info_sh */
#include <linux/power/hisi/hisi_battery_data.h>	/*for struct charge_core_info_sh */
#include <huawei_platform/power/huawei_charger.h>	/*for struct charge_core_info_sh */
#include <huawei_platform/power/charger/charger_ap/charging_core.h>

/*************************struct define area***************************/
struct charge_core_info_sh {
	struct charge_temp_data temp_para[TEMP_PARA_LEVEL];
	struct charge_volt_data volt_para[VOLT_PARA_LEVEL];
	struct charge_vdpm_data vdpm_para[VDPM_PARA_LEVEL];
	struct charge_segment_data segment_para[SEGMENT_PARA_LEVEL];
	struct charge_inductance_data inductance_para[VDPM_PARA_LEVEL];
	struct charge_core_data data;
};

#endif
