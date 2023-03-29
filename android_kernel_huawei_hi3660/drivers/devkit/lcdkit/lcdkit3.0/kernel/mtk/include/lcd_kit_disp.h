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

#ifndef __LCD_KIT_DISP_H_
#define __LCD_KIT_DISP_H_
/***********************************************************
*macro definition
***********************************************************/
#include "lcd_kit_common.h"
#include "lcd_kit_utils.h"
//#include "hisi_fb.h"
//#include "hisi_mipi_dsi.h"
//////////////////////////////macro////////////////////////////////
#define DTS_COMP_LCD_KIT_PANEL_TYPE     "huawei,lcd_panel_type"
#define LCD_KIT_PANEL_COMP_LENGTH       128
struct lcd_kit_disp_info *lcd_kit_get_disp_info(void);
#define disp_info	lcd_kit_get_disp_info()
//////////////////////////////ENUM////////////////////////////////
enum alpm_mode {
	ALPM_DISPLAY_OFF,
	ALPM_ON_MIDDLE_LIGHT,
	ALPM_EXIT,
	ALPM_ON_LOW_LIGHT,
};

//////////////////////STRUCT///////////////////////
struct lcd_kit_disp_info {
	/********************running test****************/
	/*pcd err flag test*/
	u32 pcd_errflag_check_support;
	/*check sum test*/
	struct lcd_kit_checksum checksum;
	/*adc sample vsp voltage*/
	struct lcd_kit_hkadc hkadc;
	/*current detect*/
	struct lcd_kit_current_detect current_det;
	/*lv detect*/
	struct lcd_kit_lv_detect lv_det;
	/*ldo check*/
	struct lcd_kit_ldo_check ldo_check;
	/********************end****************/
	/********************effect****************/
	/*gamma calibration*/
	struct lcd_kit_gamma gamma_cal;
	/*oem information*/
	struct lcd_kit_oem_info oeminfo;
	/*rgbw function*/
	struct lcd_kit_rgbw rgbw;
	/********************end****************/
	/********************normal****************/
	/*lcd type*/
	u32 lcd_type;
	/*panel information*/
	char* compatible;
	/*product id*/
	u32 product_id;
	/*dsi1 support*/
	u32 dsi1_cmd_support;
	/*vr support*/
	u32 vr_support;
	/*lcd kit semaphore*/
	struct semaphore lcd_kit_sem;
	/*lcd kit mipi mutex lock*/
	struct mutex mipi_lock;
	/*alpm -aod*/
	struct lcd_kit_alpm alpm;
	/*quickly sleep out*/
	struct lcd_kit_quickly_sleep_out quickly_sleep_out;
	/*fps ctrl*/
	struct lcd_kit_fps fps;
	/*project id*/
	struct lcd_kit_project_id project_id;
	/********************end****************/
};

/***********************************************************
*variable declaration
***********************************************************/
/*extern variable*/
extern int lcd_kit_power_init(void);
extern int lcd_kit_sysfs_init(void);
/***********************************************************
*function declaration
***********************************************************/
#endif
