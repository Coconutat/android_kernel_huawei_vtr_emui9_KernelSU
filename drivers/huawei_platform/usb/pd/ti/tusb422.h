/*
 * Texas Instruments TUSB422 Power Delivery
 *
 * Author: Brian Quach <brian.quach@ti.com>
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

#ifndef __TUSB422_H__
#define __TUSB422_H__

#include "tusb422_common.h"

#define TI_VID 0x0451
#define TI_PID 0x0422

#define TUSB422_ALERT_IRQ_STATUS  0x8000

typedef enum
{
	TUSB422_SLAVE_ADDR_HI = 0x64,
	TUSB422_SLAVE_ADDR_LO = 0x44,
	TUSB422_SLAVE_ADDR_NC = 0x20
} tusb422_i2c_slave_addr_t;


typedef enum
{
	/* TCPC Vendor-defined registers */
	TUSB422_REG_GPO1_2_OUT_MODE  = 0x80,
	TUSB422_REG_GPO3_4_OUT_MODE  = 0x81,
	TUSB422_REG_GPIO_I_MODE      = 0x84,
	TUSB422_REG_GPIO_INVERT      = 0x86,
	TUSB422_REG_GPIO_OUT_VAL     = 0x87,
	TUSB422_REG_GPIO_IO_CONFIG   = 0x88,
	TUSB422_REG_GPIO_IN_STATUS   = 0x8a,
	TUSB422_REG_INT_STATUS       = 0x90,
	TUSB422_REG_INT_MASK         = 0x92,
	TUSB422_REG_CC_GEN_CTRL      = 0x94,
	TUSB422_REG_BMC_TX_CTRL      = 0x95,
	TUSB422_REG_BMC_RX_CTRL      = 0x96,
	TUSB422_REG_BMC_RX_STATUS    = 0x97,
	TUSB422_REG_VBUS_VCONN_CTRL  = 0x98,
	TUSB422_REG_OTSD_CTRL        = 0x99,
	TUSB422_REG_BMC_TX_TEST_CTRL = 0x9a,
	TUSB422_REG_BMC_RX_TEST_CTRL = 0x9c,
	TUSB422_REG_LFO_TIMER        = 0xa0,
	TUSB422_REG_PAGE_SEL         = 0xff
} tusb422_reg_t;


typedef enum
{
	/* Page 1 registers */
	TUSB422_DFT_REG_DIGITAL_REV  = 0xa0,
	TUSB422_DFT_REG_DIE_REV      = 0xa1,
} tusb422_page1_reg_t;



typedef enum
{
	TUSB422_INT_LFO_TIMER      = (1 << 4),
	TUSB422_INT_CC_FAULT       = (1 << 3),
	TUSB422_INT_OTSD1_STAT     = (1 << 2),
	TUSB422_INT_FAST_ROLE_SWAP = (1 << 0),

	TUSB422_INT_MASK_ALL       = 0x1d

} tusb422_int_t;

typedef enum
{
	TUSB422_BMC_TX_FASTROLE_SWAP = (1 << 1)
} tusb422_bmc_tx_t;

typedef enum
{
	TUSB422_BMC_FASTROLE_RX_EN = (1 << 3)
} tusb422_bmc_rx_t;

typedef enum
{
	TUSB422_PD_TX_RX_RESET       = (1 << 6),
	TUSB422_GLOBAL_SW_RESET      = (1 << 5),
	TUSB422_CC_SAMPLE_RATE_MASK  = 0x0C,
	TUSB422_CC_SAMPLE_RATE_SHIFT = 2,
	TUSB422_DRP_DUTY_CYCLE_MASK  = 0x03

} tusb422_cc_gen_ctrl_t;


typedef enum
{
	CC_SAMPLE_RATE_1MS = 0,
	CC_SAMPLE_RATE_2MS,	 /* default */
	CC_SAMPLE_RATE_8MS,
	CC_SAMPLE_RATE_16MS
} tusb422_sample_rate_t;

typedef enum
{
	DRP_DUTY_CYCLE_30PCT = 0,
	DRP_DUTY_CYCLE_10PCT,  /* default */
	DRP_DUTY_CYCLE_50PCT,
	DRP_DUTY_CYCLE_60PCT,
} tusb422_drp_duty_cycle_t;


void tusb422_stop_vbus_discharge(unsigned int port);
void tusb422_set_vconn_discharge_enable(unsigned int port, bool enable);

void tusb422_set_cc_sample_rate(unsigned int port, tusb422_sample_rate_t rate);
void tusb422_pd_reset(unsigned int port);
void tusb422_sw_reset(unsigned int port);

void tusb422_isr(unsigned int port);
void tusb422_lfo_timer_start(struct tusb422_timer_t *timer, uint16_t timeout_ms, void (*function)(unsigned int));
void tusb422_lfo_timer_cancel(struct tusb422_timer_t *timer);

uint8_t tusb422_get_revision(unsigned int port);

void tusb422_set_fast_role_swap_detect(unsigned int port, bool enable);

void tusb422_send_fast_role_swap(unsigned int port);

void tusb422_init(unsigned int port);
bool tusb422_is_trimmed(unsigned int port);
bool tusb422_is_present(unsigned int port);

#endif //__TUSB422_H__
