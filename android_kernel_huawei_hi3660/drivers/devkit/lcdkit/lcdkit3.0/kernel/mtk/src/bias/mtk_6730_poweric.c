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
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include "lcd_kit_common.h"
#include "lcd_kit_power.h"
#if defined(CONFIG_RT5081_PMU_DSV) || defined(CONFIG_MT6370_PMU_DSV)
static struct regulator *disp_bias_pos = NULL;
static struct regulator *disp_bias_neg = NULL;
static int regulator_inited = 0;

int lcd_kit_display_bias_regulator_init(void)
{
	int ret = 0;

	if (regulator_inited)
		return ret;

	/* please only get regulator once in a driver */
	disp_bias_pos = regulator_get(NULL, "dsv_pos");
	if (IS_ERR(disp_bias_pos)) { /* handle return value */
		ret = PTR_ERR(disp_bias_pos);
		LCD_KIT_ERR("get dsv_pos fail, error: %d\n", ret);
		return ret;
	}

	disp_bias_neg = regulator_get(NULL, "dsv_neg");
	if (IS_ERR(disp_bias_neg)) { /* handle return value */
		ret = PTR_ERR(disp_bias_neg);
		LCD_KIT_ERR("get dsv_neg fail, error: %d\n", ret);
		return ret;
	}

	regulator_inited = 1;
	return ret;
}

int lcd_kit_display_bias_vsp_enable(struct regulate_bias_desc vspconfig)
{
	int ret = 0;

    lcd_kit_display_bias_regulator_init();
	/* set voltage with min & max*/
	ret = regulator_set_voltage(disp_bias_pos, vspconfig.min_uV, vspconfig.max_uV);
	if (ret < 0)
	{
		LCD_KIT_ERR("set voltage disp_bias_vsp fail, ret = %d\n", ret);
	}

    return ret;
}

int lcd_kit_display_bias_vsn_enable(struct regulate_bias_desc vsnconfig)
{
	int ret = 0;
	int retval = 0;

    lcd_kit_display_bias_regulator_init();
	/* set voltage with min & max*/
	ret = regulator_set_voltage(disp_bias_neg, vsnconfig.min_uV, vsnconfig.max_uV);
	if (ret < 0)
	{
		LCD_KIT_ERR("set voltage disp_bias_vsn fail, ret = %d\n", ret);
	}
	retval |= ret;

	ret = regulator_enable(disp_bias_pos);
	if (ret < 0)
	{
		LCD_KIT_ERR("enable disp_bias_vsp fail, ret = %d\n", ret);
	}
	retval |= ret;
	
	ret = regulator_enable(disp_bias_neg);
	if (ret < 0)
	{
		LCD_KIT_ERR("enable disp_bias_vsn fail, ret = %d\n", ret);
	}
	retval |= ret;
    return retval;
}

int lcd_kit_display_bias_vsp_disable(void)
{
	int ret = 0;

    lcd_kit_display_bias_regulator_init();
	ret = regulator_disable(disp_bias_pos);
	if (ret < 0)
	{
		LCD_KIT_ERR("disable disp_bias_vsp fail, ret = %d\n", ret);
	}
	else
	{
		ret = 0;
	}
	return ret;
}

int lcd_kit_display_bias_vsn_disable(void)
{
	int ret = 0;

    lcd_kit_display_bias_regulator_init();
	ret = regulator_disable(disp_bias_neg);
	if (ret < 0)
	{
		LCD_KIT_ERR("disable disp_bias_vsn fail, ret = %d\n", ret);
	}
	else
	{
		ret = 0;
	}
	return ret;
}

static struct lcd_kit_mtk_regulate_ops mtk_regulate_ops = {
	.reguate_init = lcd_kit_display_bias_regulator_init,
	.reguate_vsp_set_voltage = NULL,
	.reguate_vsp_on_delay = NULL,
	.reguate_vsp_enable = lcd_kit_display_bias_vsp_enable,
	.reguate_vsp_disable = lcd_kit_display_bias_vsp_disable,
	.reguate_vsn_set_voltage = NULL,
	.reguate_vsn_on_delay = NULL,
	.reguate_vsn_enable = lcd_kit_display_bias_vsn_enable,
	.reguate_vsn_disable = lcd_kit_display_bias_vsn_disable,
};

struct lcd_kit_mtk_regulate_ops* lcd_kit_mtk_regulate_register(void)
{
	return &mtk_regulate_ops;
}
EXPORT_SYMBOL(lcd_kit_mtk_regulate_register);
#endif