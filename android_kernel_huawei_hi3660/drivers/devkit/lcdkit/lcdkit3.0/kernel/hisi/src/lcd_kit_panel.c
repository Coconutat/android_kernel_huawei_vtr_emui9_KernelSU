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

#include "lcd_kit_disp.h"
#include "lcd_kit_panel.h"
#include "lcd_kit_sysfs_hs.h"
#include "panel/lg_nt36772a.c"
#include "panel/boe_hx83112e.c"
#include "panel/samsung_ea8076.c"
#include "panel/boe_r66451.c"

/*
***************************************************
***************************************************
*/
static struct lcd_kit_panel_map panel_map[] = {
	{PANEL_LG_NT36772A, lg_nt36772a_proble},
	{PANEL_LG_NT36772A_V2, lg_nt36772a_proble},
	{PANEL_BOE_HX83112E, boe_hx83112e_proble},
	{PANEL_SAMSUNG_EA8076, samsung_ea8076_probe},
	{PANEL_SAMSUNG_EA8076_V2, samsung_ea8076_probe},
	{PANEL_SAMSUNG_EA8074, samsung_ea8076_probe},
	{PANEL_BOE_R66451, boe_r66451_probe},
	{PANEL_TONY_SAMSUNG_EA8076_V4,samsung_ea8076_probe},
	{PANEL_TONY_SAMSUNG_EA8076,samsung_ea8076_probe},

};

struct lcd_kit_panel_ops *g_lcd_kit_panel_ops = NULL;
int lcd_kit_panel_ops_register(struct lcd_kit_panel_ops * ops)
{
	if (!g_lcd_kit_panel_ops) {
		g_lcd_kit_panel_ops = ops;
		LCD_KIT_INFO("ops register success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("ops have been registered!\n");
	return LCD_KIT_FAIL;
}
int lcd_kit_panel_ops_unregister(struct lcd_kit_panel_ops * ops)
{
	if (g_lcd_kit_panel_ops == ops) {
		g_lcd_kit_panel_ops = NULL;
		LCD_KIT_INFO("ops unregister success!\n");
		return LCD_KIT_OK;
	}
	LCD_KIT_ERR("ops unregister fail!\n");
	return LCD_KIT_FAIL;
}

struct lcd_kit_panel_ops *lcd_kit_panel_get_ops(void)
{
	return g_lcd_kit_panel_ops;
}

int lcd_kit_panel_init(void)
{
	int ret = LCD_KIT_OK;
	int i = 0;

	if (!disp_info->compatible) {
		LCD_KIT_ERR("compatible is null\n");
		return LCD_KIT_FAIL;
	}
	for (i = 0; i < ARRAY_SIZE(panel_map); i++) {
		if (!strncmp(disp_info->compatible, panel_map[i].compatible, strlen(disp_info->compatible))) {
			ret = panel_map[i].callback();
			if (ret) {
				LCD_KIT_ERR("ops init fail\n");
				return LCD_KIT_FAIL;
			}
			break;
		}
	}
	/*init ok*/
	if (i >= ARRAY_SIZE(panel_map)) {
		LCD_KIT_INFO("not find adapter ops\n");
	}
	return LCD_KIT_OK;
}
