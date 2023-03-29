/* Copyright (c) 2017-2018, Huawei terminal Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/

#ifndef _LCD_KIT_CORE_H_
#define _LCD_KIT_CORE_H_

/*lcd kit ops, provide to lcd kit module register*/
struct lcd_kit_ops {
	bool (*lcd_kit_support)(void);
	int (*create_sysfs)(struct kobject* obj);
	int (*get_project_id)(char *buff);
	int (*get_status_by_type)(int type, int *status);
	int (*get_pt_station_status)(void);
	int (*get_lcd_status)(void);
	int (*get_panel_power_status)(void);
	int (*power_monitor_on)(void);
	int (*power_monitor_off)(void);
	int (*set_vss_by_thermal)(void);
};

/*TS sync*/
#define NO_SYNC     0
#define SHORT_SYNC      5

/*TS Event*/
enum lcd_kit_ts_pm_type
{
	TS_BEFORE_SUSPEND = 0,
	TS_SUSPEND_DEVICE,
	TS_RESUME_DEVICE,
	TS_AFTER_RESUME,
	TS_EARLY_SUSPEND,
	TS_IC_SHUT_DOWN,
};

/*ts type*/
enum ts_kit_type {
	LCD_ONLINE_TYPE,
	PT_STATION_TYPE,
};

enum lcd_kit_type {
	TS_GESTURE_FUNCTION,
};

enum lcd_kit_panel_state {
	LCD_POWER_STATE_OFF,
	LCD_POWER_STATE_ON,
};

/*lcd kit ops, provide to ts kit module register*/
struct ts_kit_ops {
	int (*ts_power_notify)(enum lcd_kit_ts_pm_type type, int sync);
	int (*get_tp_status_by_type)(int type, int *status);
};

/*Function declare*/
int lcd_kit_ops_register(struct lcd_kit_ops * ops);
int lcd_kit_ops_unregister(struct lcd_kit_ops * ops);
struct lcd_kit_ops *lcd_kit_get_ops(void);
int ts_kit_ops_register(struct ts_kit_ops * ops);
int ts_kit_ops_unregister(struct ts_kit_ops * ops);
struct ts_kit_ops *ts_kit_get_ops(void);
#endif
