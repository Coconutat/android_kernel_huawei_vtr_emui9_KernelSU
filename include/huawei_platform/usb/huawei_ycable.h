/*
 * drivers/usb/huawei_ycable.h
 *
 *huawei ycable driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _HUAWEI_YCABLE
#define _HUAWEI_YCABLE

#include <linux/slab.h>
#include <pmic_interface.h>
#include <linux/hisi/hisi_adc.h>
#include <linux/of_gpio.h>


#define YCABLE_WORK_TIMEOUT                 (50)
#define YCABLE_OTG_ENABLE_WORK_TIMEOUT      (500)
#define YCABLE_OTG_THRESHOLD_VOLTAGE_MIN    (0)    //mV
#define YCABLE_OTG_THRESHOLD_VOLTAGE_MAX    (150)    //mV
#define YCABLE_CHG_THRESHOLD_VOLTAGE_MIN    (270)    //mV
#define YCABLE_CHG_THRESHOLD_VOLTAGE_MAX    (450)    //mV
#define YCABLE_DETECT_TIMEOUT               (30000)
#define YCABLE_OTG_BOOSTV                   (5000)
#define YCABLE_CURR_DEFAULT                 (1000)

struct ycable_info {
	bool ycable_support;
	int ycable_gpio;
	int y_cable_status;
	unsigned int otg_adc_channel;
	unsigned int ycable_iin_curr;
	unsigned int ycable_ichg_curr;
	bool ycable_otg_enable_flag;
	bool ycable_charger_enable_flag;
	unsigned int ycable_wdt_time_out;
	struct delayed_work ycable_work;
};

enum ycable_status {
	YCABLE_UNKNOW = 0,
	YCABLE_CHARGER,
	YCABLE_OTG,
};

#endif
