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

#ifndef __USB_PD_PROTOCOL_H__
#define __USB_PD_PROTOCOL_H__

#include "tusb422_common.h"
#include "tcpci.h"
#include "usb_pd.h"


void usb_pd_prl_tx_ctrl_msg(unsigned int port, uint8_t *buf, msg_hdr_ctrl_msg_type_t msg_type, tcpc_transmit_t sop_type);

void usb_pd_prl_tx_data_msg(unsigned int port, uint8_t *buf, msg_hdr_data_msg_type_t msg_type, tcpc_transmit_t sop_type, unsigned int ndo);

void usb_pd_prl_reset(unsigned int port);

void usb_pd_prl_init(void);

#endif
