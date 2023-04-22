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

#ifndef __LCD_KIT_ADAPT_H_
#define __LCD_KIT_ADAPT_H_
int lcd_kit_dsi_cmds_tx(void* hld, struct lcd_kit_dsi_panel_cmds* cmds);
int lcd_kit_dsi_cmds_rx(void* hld, uint8_t* out, struct lcd_kit_dsi_panel_cmds* cmds);
int lcd_kit_dsi_cmds_tx_no_lock(void* hld, struct lcd_kit_dsi_panel_cmds* cmds);
int lcd_kit_dsi_cmds_rx_no_lock(void* hld, uint8_t* out, struct lcd_kit_dsi_panel_cmds* cmds);
int lcd_kit_adapt_init(void);
#endif
