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

#include "lcd_kit_power.h"
#include "lcd_kit_common.h"
#include "lcm_drv.h"

extern struct LCM_UTIL_FUNCS lcm_util_mtk;
extern struct LCM_DRIVER lcdkit_mtk_common_panel;
#define SET_RESET_PIN(v)	(lcm_util_mtk.set_reset_pin((v)))
/********************************************************************
*variable
********************************************************************/
/* scharg regulator */
static struct regulator* bl_vcc;
static struct regulator* bias_vcc;
static struct regulator* vsn_vcc;
static struct regulator* vsp_vcc;
/*ldo regulator*/
static struct regulator* iovcc;
static struct regulator* vci;
static struct regulator* vdd;
/*global gpio*/
uint32_t g_lcd_kit_gpio = 0;

/********************************************************************
*power type
********************************************************************/

struct regulate_bias_desc vsp_param = {5500000, 5500000, 0, 0};
struct regulate_bias_desc vsn_param = {5500000, 5500000, 0, 0};


/*gpio power*/
static struct gpio_desc gpio_req_cmds[] = {
	{
		DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_NAME, &g_lcd_kit_gpio, 0
	},
};

static struct gpio_desc gpio_free_cmds[] = {
	{
		DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_NAME, &g_lcd_kit_gpio, 0
	},
};

static struct gpio_desc gpio_high_cmds[] = {
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_NAME, &g_lcd_kit_gpio, 1
	},
};

static struct gpio_desc gpio_low_cmds[] = {
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_NAME, &g_lcd_kit_gpio, 0
	},
};

struct gpio_power_arra gpio_power[] = {
	{GPIO_REQ, ARRAY_SIZE(gpio_req_cmds), gpio_req_cmds},
	{GPIO_HIGH, ARRAY_SIZE(gpio_high_cmds), gpio_high_cmds},
	{GPIO_LOW, ARRAY_SIZE(gpio_low_cmds), gpio_low_cmds},
	{GPIO_FREE, ARRAY_SIZE(gpio_free_cmds), gpio_free_cmds},
};

static struct lcd_kit_mtk_regulate_ops *g_regulate_ops = NULL;

struct lcd_kit_mtk_regulate_ops* lcd_kit_mtk_get_regulate_ops(void)
{
	return g_regulate_ops;
}


int lcd_kit_mtk_regulate_unregister(struct lcd_kit_mtk_regulate_ops* ops)
{
	if (g_regulate_ops == ops) {
		g_regulate_ops = NULL;
		LCD_KIT_INFO("g_regulate_ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("g_regulate_ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

int gpio_cmds_tx(struct gpio_desc *cmds, int cnt)
{
	int ret = 0;
	struct gpio_desc *cm = NULL;
	int i = 0;

	if (!cmds) {
		LCD_KIT_ERR("cmds is null point!\n");
		return LCD_KIT_FAIL;
	}

	cm = cmds;

	for (i = 0; i < cnt; i++) {
		if ((cm == NULL) || (cm->label == NULL)) {
			LCD_KIT_ERR("cm or cm->label is null! index=%d\n", i);
			ret = -1;
			goto error;
		}

		if (!gpio_is_valid(*(cm->gpio))) {
			LCD_KIT_ERR("gpio invalid, dtype=%d, lable=%s, gpio=%d!\n",
				cm->dtype, cm->label, *(cm->gpio));
			ret = -1;
			goto error;
		}

        *(cm->gpio) += ((struct mtk_panel_info *)(lcdkit_mtk_common_panel.panel_info))->gpio_offset;

		if (cm->dtype == DTYPE_GPIO_INPUT) {
			if (gpio_direction_input(*(cm->gpio)) != 0) {
				LCD_KIT_ERR("failed to gpio_direction_input, lable=%s, gpio=%d!\n",
					cm->label, *(cm->gpio));
				ret = -1;
				goto error;
			}
		} else if (cm->dtype == DTYPE_GPIO_OUTPUT) {
			if (gpio_direction_output(*(cm->gpio), cm->value) != 0) {
				LCD_KIT_ERR("failed to gpio_direction_output, label%s, gpio=%d!\n",
					cm->label, *(cm->gpio));
				ret = -1;
				goto error;
			}
		} else if (cm->dtype == DTYPE_GPIO_REQUEST) {
			if (gpio_request(*(cm->gpio), cm->label) != 0) {
				LCD_KIT_ERR("failed to gpio_request, lable=%s, gpio=%d!\n",
					cm->label, *(cm->gpio));
				ret = -1;
				goto error;
			}
		} else if (cm->dtype == DTYPE_GPIO_FREE) {
			gpio_free(*(cm->gpio));
		} else {
			LCD_KIT_ERR("dtype=%x NOT supported\n", cm->dtype);
			ret = -1;
			goto error;
		}

		if (cm->wait) {
			if (cm->waittype == WAIT_TYPE_US)
				udelay(cm->wait);
			else if (cm->waittype == WAIT_TYPE_MS)
				mdelay(cm->wait);
			else
				mdelay(cm->wait * 1000);
		}

		cm++;
	}

	return 0;

error:
	return ret;
}


void lcd_kit_gpio_tx(uint32_t type, uint32_t op)
{
	int i = 0;
	struct gpio_power_arra* gpio_cm;

	switch (type) {
		case LCD_KIT_VCI:
			g_lcd_kit_gpio = power_hdl->lcd_vci.buf[1];
			break;
		case LCD_KIT_IOVCC:
			g_lcd_kit_gpio = power_hdl->lcd_iovcc.buf[1];
			break;
		case LCD_KIT_VSP:
			g_lcd_kit_gpio = power_hdl->lcd_vsp.buf[1];
			break;
		case LCD_KIT_VSN:
			g_lcd_kit_gpio = power_hdl->lcd_vsn.buf[1];
			break;
		case LCD_KIT_RST:
			if (op == GPIO_HIGH) {
				SET_RESET_PIN(1);
			} else if (op == GPIO_LOW) {
				SET_RESET_PIN(0);
			}
			g_lcd_kit_gpio = power_hdl->lcd_rst.buf[1];
			break;
		case LCD_KIT_BL:
			g_lcd_kit_gpio = power_hdl->lcd_backlight.buf[1];
			break;
		default:
			LCD_KIT_ERR("not support type:%d\n", type);
			break;
	}

	for (i = 0; i < ARRAY_SIZE(gpio_power); i++) {
		if (gpio_power[i].oper == op) {
			gpio_cm = &gpio_power[i];
			break;
		}
	}
	if (i >= ARRAY_SIZE(gpio_power)) {
		LCD_KIT_ERR("not found cm from gpio_power\n");
		return ;
	}
	gpio_cmds_tx(gpio_cm->cm, gpio_cm->num);
	LCD_KIT_INFO("gpio:%d ,op:%d\n", *gpio_cm->cm->gpio, op);
	return ;
}

int lcd_kit_charger_ctrl(uint32_t type, uint32_t enable)
{
	int ret = LCD_KIT_OK;

	switch (type) {
		case LCD_KIT_VSP:
			if (enable) {
				if(g_regulate_ops) {
				      if(g_regulate_ops->reguate_vsp_enable) {
                                      ret = g_regulate_ops->reguate_vsp_enable(vsp_param);
					    if(LCD_KIT_OK != ret) {
					         ret = LCD_KIT_FAIL;
				                LCD_KIT_ERR("reguate_vsp_enable failed\n");
					    }
				    } else {
					       ret = LCD_KIT_FAIL;
				              LCD_KIT_ERR("reguate_vsp_enable is NULL\n");
					 }
				}
				else
				{
                                ret = LCD_KIT_FAIL;
				     LCD_KIT_ERR("g_regulate_ops is NULL\n");
				}
			} else {
				if(g_regulate_ops) {
				      if(g_regulate_ops->reguate_vsp_disable) {
                                      ret = g_regulate_ops->reguate_vsp_disable();
					    if(LCD_KIT_OK != ret) {
					         ret = LCD_KIT_FAIL;
				                LCD_KIT_ERR("reguate_vsp_disable failed\n");
					    }
				    } else {
					       ret = LCD_KIT_FAIL;
				              LCD_KIT_ERR("reguate_vsp_disable is NULL\n");
					 }
				}
				else
				{
                                ret = LCD_KIT_FAIL;
				     LCD_KIT_ERR("g_regulate_ops is NULL\n");
				}
			}
			break;
		case LCD_KIT_VSN:
			if (enable) {
				if(g_regulate_ops) {
				      if(g_regulate_ops->reguate_vsn_enable) {
                                      ret = g_regulate_ops->reguate_vsn_enable(vsp_param);
					    if(LCD_KIT_OK != ret) {
					         ret = LCD_KIT_FAIL;
				                LCD_KIT_ERR("reguate_vsn_enable failed\n");
					    }
				    } else {
					       ret = LCD_KIT_FAIL;
				              LCD_KIT_ERR("reguate_vsn_enable is NULL\n");
					 }
				}
				else
				{
                                ret = LCD_KIT_FAIL;
				     LCD_KIT_ERR("g_regulate_ops is NULL\n");
				}
			} else {
				if(g_regulate_ops) {
				      if(g_regulate_ops->reguate_vsn_disable) {
                                      ret = g_regulate_ops->reguate_vsn_disable();
					    if(LCD_KIT_OK != ret) {
					         ret = LCD_KIT_FAIL;
				                LCD_KIT_ERR("reguate_vsn_disable failed\n");
					    }
				    } else {
					       ret = LCD_KIT_FAIL;
				              LCD_KIT_ERR("reguate_vsn_disable is NULL\n");
					 }
				}
				else
				{
                                ret = LCD_KIT_FAIL;
				     LCD_KIT_ERR("g_regulate_ops is NULL\n");
				}
			}
			break;
		default:
			ret = LCD_KIT_FAIL;
			LCD_KIT_ERR("error type\n");
			break;
	}
	return ret;
}

void  lcd_kit_bias_enable(void)
{
	int ret = 0;

	ret = lcd_kit_charger_ctrl(LCD_KIT_VSP, 1);
	if(LCD_KIT_OK != ret)
	{
	    LCD_KIT_ERR("enable regulate vsp failed\n");
	}
	ret = lcd_kit_charger_ctrl(LCD_KIT_VSN, 1);
	if(LCD_KIT_OK != ret)
	{
	    LCD_KIT_ERR("enable regulate vsn failed\n");
	}
}


static void lcd_kit_power_regulate_vsp_set(struct regulate_bias_desc* cmds)
{
    if (!cmds) {
		LCD_KIT_ERR("cmds is null point!\n");
		return;
	}
	cmds->min_uV = power_hdl->lcd_vsp.buf[2];
	cmds->max_uV = power_hdl->lcd_vsp.buf[2];
	cmds->wait = power_hdl->lcd_vsp.buf[1];
}

static void lcd_kit_power_regulate_vsn_set(struct regulate_bias_desc* cmds)
{
    if (!cmds) {
		LCD_KIT_ERR("cmds is null point!\n");
		return;
	}
	cmds->min_uV = power_hdl->lcd_vsn.buf[2];
	cmds->max_uV = power_hdl->lcd_vsn.buf[2];
	cmds->wait = power_hdl->lcd_vsn.buf[1];
}
#if defined(CONFIG_RT5081_PMU_DSV) || defined(CONFIG_MT6370_PMU_DSV)
extern struct lcd_kit_mtk_regulate_ops* lcd_kit_mtk_regulate_register(void);
#endif

int lcd_kit_power_init(void)
{
	int ret = LCD_KIT_OK;

#if defined(CONFIG_RT5081_PMU_DSV) || defined(CONFIG_MT6370_PMU_DSV)
       g_regulate_ops = lcd_kit_mtk_regulate_register();
#endif
   if(power_hdl == NULL)
   {
	   return 0;
   }	   
   /*vsp*/
	if (power_hdl->lcd_vsp.buf){
		if(power_hdl->lcd_vsp.buf[0] == REGULATOR_MODE) {
                  LCD_KIT_INFO("LCD vsp type is regulate!\n");
                  lcd_kit_power_regulate_vsp_set(&vsp_param);
		}
	}
       /*vsn*/
	if (power_hdl->lcd_vsn.buf){
		if(power_hdl->lcd_vsn.buf[0] == REGULATOR_MODE) {
                  LCD_KIT_INFO("LCD vsn type is regulate!\n");
                  lcd_kit_power_regulate_vsn_set(&vsn_param);
		}
	}

	return ret;
}

int lcd_kit_power_finit(struct platform_device* pdev)
{
	int ret = LCD_KIT_OK;

	return ret;
}

