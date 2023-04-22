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

#ifndef __TCPM_H__
#define __TCPM_H__

#include "tusb422_common.h"
#include "tcpci.h"
#ifndef CONFIG_TUSB422
    #include <stdbool.h>
    #include <stdint.h>
#endif
#include <huawei_platform/usb/hw_pd_dev.h>

/*
* Macros and definitions
*/

#define NUM_TCPC_DEVICES   1

#define ERR_NO_TI_DEV  (-77)

#define MV_TO_25MV(x) ((x)/25)
#define VSAFE0V_MAX   MV_TO_25MV(800)   /* 0.80V max in 25mV units */
#define VDIRECT_MAX   MV_TO_25MV(3400)  /* 3.4V max for direct charger*/
#define VSAFE5V_MIN   MV_TO_25MV(4450)  /* 4.45V min in 25mV units */
#define VDISCON_MAX   MV_TO_25MV(4000)  /* 4.00V min in 25mV units */
#define VSTOP_DISCHRG MV_TO_25MV(500)   /* Stop discharge threshold in 25mV units */

#define GET_SRC_CURRENT_MA(cc_adv) (((cc_adv) == CC_SNK_STATE_POWER30) ? 3000 : ((cc_adv) == CC_SNK_STATE_POWER15) ? 1500 : 500)
/* Sink with Accessory Support is NOT supported by the state machine */

typedef enum
{
	TCPC_STATE_UNATTACHED_SRC = 0,
	TCPC_STATE_UNATTACHED_SNK,
	TCPC_STATE_ATTACH_WAIT_SRC,
	TCPC_STATE_ATTACH_WAIT_SNK,
	TCPC_STATE_TRY_SNK,
	TCPC_STATE_TRY_SNK_LOOK4SRC,
	TCPC_STATE_TRY_SRC,
	TCPC_STATE_TRY_WAIT_SRC,
	TCPC_STATE_TRY_WAIT_SNK,
	TCPC_STATE_ATTACHED_SRC,
	TCPC_STATE_ATTACHED_SNK,
	TCPC_STATE_UNORIENTED_DEBUG_ACC_SRC,
	TCPC_STATE_ORIENTED_DEBUG_ACC_SRC,
	TCPC_STATE_DEBUG_ACC_SNK,
	TCPC_STATE_AUDIO_ACC,
	TCPC_STATE_ERROR_RECOVERY,
	TCPC_STATE_DISABLED,  /* no CC terminations */
	TCPC_NUM_STATES
} tcpc_state_t;

extern const char * const tcstate2string[TCPC_NUM_STATES];

typedef enum
{
	TC_FLAGS_TRY_SRC        = (1 << 0),	  /* Either Try.SRC or Try.SNK support but not both */
	TC_FLAGS_TRY_SNK        = (1 << 1),	  /* Either Try.SRC or Try.SNK support but not both */
	TC_FLAGS_TEMP_ROLE      = (1 << 2)
} tc_flags_t;


typedef enum
{
	PLUG_UNFLIPPED = 0,	 /* USB-PD comm on CC1 */
	PLUG_FLIPPED,		 /* USB-PD comm on CC2 */
} plug_polarity_t;


//typedef enum
//{
//	CC_STATE_OPEN        = 0,
//	CC_STATE_SRC_RA      = 1,
//	CC_STATE_SRC_RD      = 2,
//	CC_STATE_SNK_DEFAULT = (1 | CC_STATUS_CONNECT_RESULT), /* bit 2 is set if TCPC is presenting Rd */
//	CC_STATE_SNK_1P5A    = (2 | CC_STATUS_CONNECT_RESULT), /* bit 2 is set if TCPC is presenting Rd */
//	CC_STATE_SNK_3P0A    = (3 | CC_STATUS_CONNECT_RESULT), /* bit 2 is set if TCPC is presenting Rd */
//} cc_state_t;


typedef struct
{
	uint8_t             port;
	tcpc_state_t        state;
	tcpc_state_t        last_state;	   /* for debug */
	bool                state_change;

	struct tusb422_timer_t timer;
	struct tusb422_timer_t timer2;

	tc_role_t           role;
	tcpc_role_rp_val_t  rp_val;

	uint8_t             flags;

	uint8_t             cc_status;
	bool				src_detected;  /* source detected for debounce period */

	plug_polarity_t     plug_polarity;

	tcpc_cc_snk_state_t src_current_adv;
	bool                src_attach_notify;
	uint8_t             silicon_revision;
	uint8_t             rx_buff_overflow_cnt;
	uint8_t             vconn_ocp_cnt;
} tcpc_device_t;

typedef struct
{
	uint16_t      IE;
} tUSBPD_Intf_Context;

typedef enum
{
	TCPM_STATUS_OK = 0,
	TCPM_STATUS_SMBUS_ERROR,
	TCPM_STATUS_TIMEOUT,
	TCPM_STATUS_PARAM_ERROR

} tcpm_status_t;


typedef enum
{
	TX_STATUS_SUCCESS = 0,
	TX_STATUS_DISCARDED,
	TX_STATUS_FAILED
} tx_status_t;


typedef struct
{
	void (*conn_state_change_cbk)(unsigned int port, tcpc_state_t state);
	void (*current_change_cbk)(unsigned int port, tcpc_cc_snk_state_t cc_state);
	void (*volt_alarm_cbk)(unsigned int port, bool hi_volt);
	void (*pd_hard_reset_cbk)(unsigned int port);
	void (*pd_transmit_cbk)(unsigned int port, tx_status_t tx_status);
	void (*pd_receive_cbk)(unsigned int port);

} tcpm_callbacks_t;

//*****************************************************************************
//
//! \brief   Initializes the SMBus interface(s) used to talk to TCPCs.
//!
//!
//! \param   none
//!
//! \return  USB_PD_RET_OK
//
// *****************************************************************************


void tcpm_register_callbacks(const tcpm_callbacks_t *callbacks);

void tcpm_get_msg_header_type(unsigned int port, uint8_t *frame_type, uint16_t *header);
void tcpm_read_message(unsigned int port, uint8_t *buf, uint8_t len);
void tcpm_transmit(unsigned int port, uint8_t *buf, tcpc_transmit_t sop_type);

void tcpm_enable_pd_receive(unsigned int port, bool enable_sop_p, bool enable_sop_pp);
void tcpm_disable_pd_receive(unsigned int port);

void tcpm_set_voltage_alarm_lo(unsigned int port, uint16_t threshold_25mv);
void tcpm_set_voltage_alarm_hi(unsigned int port, uint16_t threshold_25mv);

bool tcpm_is_vbus_present(unsigned int port);
uint16_t tcpm_get_vbus_voltage(unsigned int port);
void tcpm_enable_vbus_detect(unsigned int port);
void tcpm_disable_vbus_detect(unsigned int port);

void tcpm_set_autodischarge_disconnect(unsigned int port, bool enable);
void tcpm_set_sink_disconnect_threshold(unsigned int port, uint16_t threshold_25mv);
void tcpm_force_discharge(unsigned int port, uint16_t threshold_25mv);
void tcpm_set_bleed_discharge(unsigned int port, bool enable);

void tcpm_execute_error_recovery(unsigned int port);

void tcpm_try_role_swap(unsigned int port);
void tcpm_change_role(unsigned int port, tc_role_t new_role);

void tcpm_src_vbus_disable(unsigned int port);
void tcpm_src_vbus_enable(unsigned int port, uint16_t mv);

void tcpm_snk_vbus_enable(unsigned int port);
void tcpm_snk_vbus_disable(unsigned int port);

void tcpm_set_vconn_enable(unsigned int port, bool enable);
void tcpm_remove_rp_from_vconn_pin(unsigned int port);
bool tcpm_is_vconn_enabled(unsigned int port);

void tcpm_set_bist_test_mode(unsigned int port);

tcpc_device_t* tcpm_get_device(unsigned int port);

void tcpm_cc_pin_control(unsigned int port, tc_role_t role);
void tcpm_handle_power_role_swap(unsigned int port);
void tcpm_update_msg_header_info(unsigned int port, uint8_t data_role, uint8_t power_role);
void tcpm_set_rp_value(unsigned int port, tcpc_role_rp_val_t rp_val);
void tcpm_snk_swap_standby(unsigned int port);

void tcpm_register_dump(unsigned int port);

#endif //__TCPM_H__
