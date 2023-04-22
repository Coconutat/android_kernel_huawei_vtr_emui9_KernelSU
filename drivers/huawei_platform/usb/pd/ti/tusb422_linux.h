/*
 * Constants for the Texas Instruments TUSB422 Power Delivery
 *
 * Author: Dan Murphy <dmurphy@ti.com>
 *
 * Copyright: (C) 2016 Texas Instruments, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#ifndef _TUSB422_LINUX_H_
#define _TUSB422_LINUX_H_

#include <linux/types.h>
#include <linux/delay.h>
#include "tusb422_common.h"


#define TI_PMIC_LDO_3 2
#define TUSB_VIN_3V3 3300000

#define TI_CC_STATUS_MASK (BIT(0) | BIT(1) | BIT(2) | BIT(3))
#define TI_CC_STATUS 0x1d
#define TI_CC_STATUS_FOR_DOUBLE_56K 5
#define TI_CC_STATUS_FOR_DOUBLE_22k 10
#define TI_CC_STATUS_FOR_DOUBLE_10k 15

enum vbus_sel_t
{
	VBUS_SEL_SRC_5V       = (1 << 0),
	VBUS_SEL_SRC_HI_VOLT  = (1 << 1),
	VBUS_SEL_SNK          = (1 << 2)
};
/* ID registers */

/* Status Registers */

/* Control Registers */


int tusb422_read(int reg, void *data, int len);
int tusb422_write(int reg, const void *data, int len);
int tusb422_modify_reg(int reg, int clr_mask, int set_mask);
int tusb422_set_vbus(int vbus_sel);
int tusb422_clr_vbus(int vbus_sel);
void tusb422_msleep(int msecs);

void tusb422_set_timer_func(void (*function)(unsigned int));
void tusb422_clr_timer_func(void);
int tusb422_start_timer(unsigned int timeout_ms);
int tusb422_stop_timer(void);

#ifdef CONFIG_WAKELOCK
void tusb422_wake_lock_attach(void);
void tusb422_wake_lock_detach(void);
void tusb422_wake_lock_control(bool enable_lock);
#endif
#endif
