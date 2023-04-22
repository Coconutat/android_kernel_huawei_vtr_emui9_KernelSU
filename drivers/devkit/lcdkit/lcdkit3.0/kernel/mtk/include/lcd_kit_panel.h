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

#ifndef _LCD_KIT_PANEL_H_
#define _LCD_KIT_PANEL_H_
/*
*product id
*/
#define PRODUCT_ALPS	1002

/*
*panel compatible
*/
#define PANEL_JDI_NT36860C "jdi_2lane_nt36860_5p88_1440P_cmd"

/*
*struct
*/
struct lcd_kit_panel_ops {
	int (*lcd_kit_read_project_id)(void);
};

struct lcd_kit_panel_map {
	u32 product_id;
	char *compatible;
	int (*callback)(void);
};

/*
*function declare
*/
struct lcd_kit_panel_ops *lcd_kit_panel_get_ops(void);
int lcd_kit_panel_init(void);
int lcd_kit_panel_ops_register(struct lcd_kit_panel_ops * ops);
int lcd_kit_panel_ops_unregister(struct lcd_kit_panel_ops * ops);
#endif
