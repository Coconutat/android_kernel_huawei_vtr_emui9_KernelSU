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

#ifndef __LCD_KIT_SYSFS_HS_H_
#define __LCD_KIT_SYSFS_HS_H_
/*oem info*/
#define OEM_INFO_SIZE_MAX 500
#define OEM_INFO_BLOCK_NUM 1
/*2d barcode*/
#define BARCODE_LENGTH 46
#define BARCODE_BLOCK_NUM 3
#define BARCODE_BLOCK_LEN 16
/*project id*/
#define PROJECT_ID_LENGTH 10

#define GPIO_LOW_PCDERRFLAG    0
#define GPIO_HIGH_PCDERRFLAG   1

#define PCD_ERRFLAG_SUCCESS    0
#define PCD_FAIL               1
#define ERRFLAG_FAIL           2
#define PCD_ERRFLAG_FAIL       3

#define GPIO_LCDKIT_PCD_NAME	                  "gpio_lcdkit_pcd"
#define GPIO_LCDKIT_ERRFLAG_NAME                  "gpio_lcdkit_errflag"


enum oem_type{
	PROJECT_ID_TYPE,
	BARCODE_2D_TYPE,
	BRIGHTNESS_TYPE,
	COLOROFWHITE_TYPE,
	BRIGHTNESSANDCOLOR_TYPE,
	REWORK_TYPE,
	BRIGHTNESS_COLOROFWHITE_TYPE,
	RGBW_WHITE_TYPE,
};

/*struct*/
struct oem_info_cmd{
	unsigned char type;
	int (*func)(char *oem_data, struct hisi_fb_data_type *hisifd);
};
#endif
