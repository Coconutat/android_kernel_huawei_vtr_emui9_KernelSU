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

#ifndef __LCD_KIT_POWER_H_
#define __LCD_KIT_POWER_H_
#include "lcd_kit_utils.h"

/********************************************************************
*macro
********************************************************************/
/*backlight*/
#define BACKLIGHT_NAME      "backlight"
/*bias*/
#define BIAS_NAME        "bias"
#define VSN_NAME        "vsn"
#define VSP_NAME        "vsp"
/*vci*/
#define VCI_NAME   "vci"
/*iovcc*/
#define IOVCC_NAME       "iovcc"
/*vdd*/
#define VDD_NAME       "vdd"
/*gpio*/
#define GPIO_NAME "gpio"

enum gpio_operator {
	GPIO_REQ,
	GPIO_FREE,
	GPIO_HIGH,
	GPIO_LOW,
};

/********************************************************************
*struct
********************************************************************/
struct gpio_power_arra {
	enum gpio_operator oper;
	uint32_t num;
	struct gpio_desc* cm;
};

struct event_callback {
	uint32_t    event;
	int (*func)(void* data);
};

/*variable declare*/
extern uint32_t g_lcd_kit_gpio;

/*function declare*/
int lcd_kit_pmu_ctrl(uint32_t type, uint32_t enable);
int lcd_kit_charger_ctrl(uint32_t type, uint32_t enable);
void lcd_kit_gpio_tx(uint32_t type, uint32_t op);
int lcd_kit_power_finit(struct platform_device* pdev);
int lcd_kit_power_init(struct platform_device* pdev);
int lcd_kit_dbg_set_voltage(void);
#endif
