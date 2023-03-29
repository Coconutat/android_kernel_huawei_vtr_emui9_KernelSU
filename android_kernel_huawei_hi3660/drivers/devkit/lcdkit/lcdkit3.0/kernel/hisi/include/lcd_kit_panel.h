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

#include "lcd_kit_common.h"
#include "hisi_fb.h"

/*
*panel compatible
*/
#define PANEL_LG_NT36772A      "lg_nt36772a_hma_6p53_1080plus_cmd"
#define PANEL_LG_NT36772A_V2   "lg_nt36772a_hma_v2_6p53_1080plus_cmd"

#define PANEL_BOE_HX83112E     "boe_hx83112e_hma_6p53_1080plus_cmd"
#define PANEL_SAMSUNG_EA8076   "samsung_ea8076_elle_6p11_1080plus_cmd"
#define PANEL_SAMSUNG_EA8076_V2   "samsung_ea8076_elle_v2_6p11_1080plus_cmd"
#define PANEL_SAMSUNG_EA8074   "samsung_ea8074_ever_7p21_1080plus_cmd"
#define PANEL_BOE_R66451       "boe_r66451_6p39_1440plus_cmd"
#define PANEL_TONY_SAMSUNG_EA8076_V4 "samsung_ea8076_v4_6p39_1080plus_cmd"
#define PANEL_TONY_SAMSUNG_EA8076 "samsung_ea8076_6p39_1080plus_cmd"
/*
*struct
*/
struct lcd_kit_panel_ops {
	int (*lcd_kit_read_project_id)(void);
	int (*lcd_kit_rgbw_set_mode)(struct hisi_fb_data_type* hisifd, int mode);
	int (*lcd_get_2d_barcode)(char *oem_data, struct hisi_fb_data_type *hisifd);
	int (*lcd_kit_set_backlight_by_type)(struct platform_device* pdev, int backlight_type);
	int (*lcd_set_vss_by_thermal)(void *hld);
};

struct lcd_kit_panel_map {
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
