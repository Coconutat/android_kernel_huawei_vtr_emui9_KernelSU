/*
 * drivers/power/huawei_charger.h
 *
 *huawei charger driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#ifndef _HUAWEI_CHARGER_SENSORHUB
#define _HUAWEI_CHARGER_SENSORHUB

#include <linux/device.h>	/*for struct charge_device_info_sh */
#include <linux/notifier.h>	/*for struct charge_device_info_sh */
#include <linux/power_supply.h>	/*for struct charge_device_info_sh */
#include <huawei_platform/usb/hw_typec_dev.h>
#include <huawei_platform/power/huawei_charger.h>

enum charge_extral_type {
	CHARGE_GET_FCP_STAGE = 60,
	CHARGE_CHECK_CHARGER_TS,
	CHARGE_CHECK_CHARGER_OTG_STATE,
	CHARGE_UPDATE_CHARGE_INFO,
	CHARGE_THERMAL_INFO,
	CHARGE_SET_OTG_ENABLE,
	CHARGE_SET_CHARGE_ENABLE,
	CHARGE_GET_ILIM,
	CHARGE_GET_VBUS,
	CHARGE_THERMAL_TEST,
	CHARGE_SYSFS_THERMAL_INPUT,
	CHARGE_UPDATE_CHARGER_TYPE,
};

enum update_type{
	TYPE_SWITCH_UPDATE,
	TYPE_SENSORHUB_RECOVERY,
	TYPE_NONE,
};

struct charge_sysfs_data_sh {
	unsigned int adc_conv_rate;
	unsigned int iin_thl;
	unsigned int ichg_thl;
	unsigned int iin_thl_main;
	unsigned int ichg_thl_main;
	unsigned int iin_thl_aux;
	unsigned int ichg_thl_aux;
	unsigned int iin_rt;
	unsigned int ichg_rt;
	unsigned int vterm_rt;
	unsigned int charge_limit;
	unsigned int wdt_disable;
	unsigned int charge_enable;
	unsigned int batfet_disable;
	unsigned int hiz_enable;
	unsigned int vr_charger_type;
	enum charge_done_type charge_done_status;
	enum charge_done_sleep_type charge_done_sleep_status;
	int ibus;
	int vbus;
	int inputcurrent;
	int voltage_sys;
	unsigned int support_ico;
	unsigned int water_intrused;
	unsigned int fcp_support;
};

struct charge_sysfs_lock {
	struct mutex dump_reg_lock;
	struct mutex dump_reg_head_lock;
	struct mutex bootloader_info_lock;
	struct mutex fcp_support_lock;
};

struct charge_device_info_sh {
	unsigned int is_board_type;
	bool is_factory_mode;
	bool is_charger_mode;
	bool is_hltherm_runtest_mode;
	bool ts_flag;
	bool otg_flag;
	bool charge_done_sleep_dts;
	struct charge_sysfs_data_sh sysfs_data;
	enum usb_charger_type charger_type;
	enum typec_input_current typec_current_mode;
	enum power_supply_type charger_source;
	enum charge_fault_type charge_fault;
	enum fcp_check_stage_type fcp_stage;
	unsigned int charge_enable;
	unsigned int input_current;
	unsigned int charge_current;
	unsigned int input_typec_current;
	unsigned int charge_typec_current;
	unsigned int check_full_count;
	unsigned int is_dual_charger;
	unsigned int start_attemp_ico;
	unsigned int support_usb_nonstandard_ico;
	unsigned int support_standard_ico;
	unsigned int ico_current_mode;
	unsigned int ico_all_the_way;
	unsigned int water_check_enabled;
	unsigned int fcp_vindpm;
	unsigned int battery_fcc;
	unsigned int battery_capacity;
	unsigned int vbatt_max;//from hisi coul
	unsigned int sensorhub_stat;//for switch9688 detach
	unsigned int ufcapacity;//for log print
	unsigned int afcapacity;
	unsigned int rm;
	unsigned int uuc;
	unsigned int cc;
	unsigned int delta_rc;
	unsigned int ocv;
	unsigned int rbatt;
	unsigned int pd_charge;
};

struct uscp_device_info_sh
{
    int gpio_uscp;
    int adc_channel_uscp;
    int open_mosfet_temp;
    int close_mosfet_temp;
    int interval_switch_temp;
};

enum charging_stat_t {
	CHARGING_CLOSE,
	CHARGING_OPEN,
	CHARGING_END,
};

typedef struct
{
    const char* name;
    const struct attribute_group* attrs_group;
    struct device* dev;
} sensorhub_charger_thermal_dev_t;

#define ITEMS_MAX 8
enum sensor_type {
	SENSOR_TYPE_SYSTEM_H, 	//type = 0,  type_name = "system_h", temperature of board high temperature area, former "cpu" or "ap"
	SENSOR_TYPE_PA,		//type = 1,  type_name = "pa_0", major PA, correspondig to former "pa"
	SENSOR_TYPE_CHARGER,    //type = 2,  type_name = "charger", corresponding to former "charger"
	SENSOR_TYPE_BATTERY,     //type = 9,  type_name = "battery", system board high temperature area
	SENSOR_TYPE_END,
};

struct sensor_item {
	uint16_t thersholds;
	uint16_t thersholds_clr;
	uint16_t ucurrent;
	uint16_t bcurrent;
	uint16_t shutdown;
};

struct sensor_temp{
	uint8_t stype;
	int8_t min_temp;
	int8_t step;
	uint8_t mNumItem;//max is 8
	struct sensor_item items[ITEMS_MAX];
};

struct sensorhub_scene {
	struct sensor_temp stemps[SENSOR_TYPE_END];
};

/****************variable and function declarationn area******************/
void charge_type_dcp_detected_notify_sh(void);
#endif
