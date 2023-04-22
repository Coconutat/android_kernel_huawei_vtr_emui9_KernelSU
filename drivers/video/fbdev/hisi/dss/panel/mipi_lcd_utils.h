/* Copyright (c) 2017-2020, Hisilicon Tech. Co., Ltd. All rights reserved.
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
#ifndef MIPI_LCD_UTILS_H
#define MIPI_LCD_UTILS_H

#define LG_2LANE_NT36870 	0x1
#define JDI_2LANE_NT36860C 	0x2
#define SHARP_2LANE_NT36870 	0x4

extern uint32_t g_mipi_lcd_name;
void get_vesa_dsc_para(struct hisi_panel_info *pinfo, uint8_t panel_mode);
int switch_panel_mode(struct hisi_fb_data_type *hisifd, uint8_t mode_switch_to);
#endif
