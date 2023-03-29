/* Copyright (c) 2008-2019, Huawei Tech. Co., Ltd. All rights reserved.
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
#ifndef __DP_DEBUG_H__
#define __DP_DEBUG_H__

void dp_debug_init_combophy_pree_swing(uint32_t *pv, int count);
int dp_debug_append_info(char *buf, int size, char *fmt, ...);
int dp_debug_get_vs_pe_force(uint8_t *vs_force, uint8_t *pe_force);
int dp_debug_get_lanes_rate_force(uint8_t *lanes_force, uint8_t *rate_force);
int dp_debug_get_resolution_force(uint8_t *user_mode, uint8_t *user_format);

#endif // __DP_DEBUG_H__

