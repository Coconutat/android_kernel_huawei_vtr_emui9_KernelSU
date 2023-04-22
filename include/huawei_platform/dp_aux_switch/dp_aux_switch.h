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
#ifndef __DP_AUX_SWITCH_H__
#define __DP_AUX_SWITCH_H__

enum aux_switch_channel_type {
	channel_fsa4476 = 0,
	channel_superswitch,
};
#ifdef CONFIG_DP_AUX_SWITCH
void dp_aux_switch_op(uint32_t value);
void dp_aux_uart_switch_enable(void);
void dp_aux_uart_switch_disable(void);
enum aux_switch_channel_type get_aux_switch_channel(void);
#else
static inline void dp_aux_switch_op(uint32_t value) {}
static inline void dp_aux_uart_switch_enable(void) {}
static inline void dp_aux_uart_switch_disable(void) {}
static enum aux_switch_channel_type get_aux_switch_channel(void) {return channel_superswitch;}
#endif

#endif // __DP_AUX_SWITCH_H__

