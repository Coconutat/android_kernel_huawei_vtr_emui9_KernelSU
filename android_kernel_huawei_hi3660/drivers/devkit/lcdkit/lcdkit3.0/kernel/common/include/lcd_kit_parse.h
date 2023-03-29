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

#ifndef __LCD_KIT_PARSE_H_
#define __LCD_KIT_PARSE_H_

#define LCD_KIT_CMD_REQ_MAX                  4
#define LCD_KIT_CMD_REQ_RX                   0x0001
#define LCD_KIT_CMD_REQ_COMMIT               0x0002
#define LCD_KIT_CMD_CLK_CTRL                 0x0004
#define LCD_KIT_CMD_REQ_UNICAST              0x0008
#define LCD_KIT_CMD_REQ_DMA_TPG              0x0040
#define LCD_KIT_CMD_REQ_NO_MAX_PKT_SIZE      0x0008
#define LCD_KIT_CMD_REQ_LP_MODE              0x0010
#define LCD_KIT_CMD_REQ_HS_MODE              0x0020

int lcd_kit_parse_dcs_cmds(struct device_node* np, char* cmd_key,
						   char* link_key, struct lcd_kit_dsi_panel_cmds* pcmds);
int lcd_kit_parse_array_data(struct device_node* np,
							 char* name, struct lcd_kit_array_data* out);
int lcd_kit_parse_arrays_data(struct device_node* np,
									 char* name, struct lcd_kit_arrays_data* out, int num);
#endif
