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
#define VSN_NAME        "dsv_pos"
#define VSP_NAME        "dsv_neg"
/*vci*/
#define VCI_NAME   "vci"
/*iovcc*/
#define IOVCC_NAME       "iovcc"
/*vdd*/
#define VDD_NAME       "vdd"
/*gpio*/
#define GPIO_NAME "gpio"

enum {
	WAIT_TYPE_US = 0,
	WAIT_TYPE_MS,
};

/* dtype for gpio */
enum {
	DTYPE_GPIO_REQUEST,
	DTYPE_GPIO_FREE,
	DTYPE_GPIO_INPUT,
	DTYPE_GPIO_OUTPUT,
	DTYPE_GPIO_PMX,
	DTYPE_GPIO_PULL,
};

enum gpio_operator {
	GPIO_REQ,
	GPIO_FREE,
	GPIO_HIGH,
	GPIO_LOW,
};

/* gpio desc */
struct gpio_desc {
	int dtype;
	int waittype;
	int wait;
	char *label;
	uint32_t *gpio;
	int value;
};

/* dtype for vcc */
enum {
	DTYPE_VCC_GET,
	DTYPE_VCC_PUT,
	DTYPE_VCC_ENABLE,
	DTYPE_VCC_DISABLE,
	DTYPE_VCC_SET_VOLTAGE,
};

/* vcc desc */
struct regulate_bias_desc {
	int min_uV;
	int max_uV;
	int waittype;
	int wait;
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

struct lcd_kit_mtk_regulate_ops {
	int (*reguate_init)(void);
	int (*reguate_vsp_set_voltage)(struct regulate_bias_desc vsp);
	int (*reguate_vsp_on_delay)(struct regulate_bias_desc vsp);
	int (*reguate_vsp_enable)(struct regulate_bias_desc vsp);
	int (*reguate_vsp_disable)(void);
	int (*reguate_vsn_set_voltage)(struct regulate_bias_desc vsn);
	int (*reguate_vsn_on_delay)(struct regulate_bias_desc vsn);
	int (*reguate_vsn_enable)(struct regulate_bias_desc vsn);
	int (*reguate_vsn_disable)(void);
};

/*variable declare*/
extern uint32_t g_lcd_kit_gpio;

/*function declare*/
int lcd_kit_pmu_ctrl(uint32_t type, uint32_t enable);
int lcd_kit_charger_ctrl(uint32_t type, uint32_t enable);
void lcd_kit_gpio_tx(uint32_t type, uint32_t op);
int lcd_kit_power_finit(struct platform_device* pdev);

/*regulate function declare*/
struct lcd_kit_mtk_regulate_ops* lcd_kit_mtk_get_regulate_ops(void);
int lcd_kit_mtk_regulate_unregister(struct lcd_kit_mtk_regulate_ops* ops);

#endif
