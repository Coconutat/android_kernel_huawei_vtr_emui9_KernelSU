/*
 * TUSB422 Power Delivery
 *
 * Author: Brian Quach <brian.quach@ti.com>
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 *
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *    Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 *    Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the
 *    distribution.
 *
 *    Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#ifndef __USB_PD_POLICY_ENGINE_H__
#define __USB_PD_POLICY_ENGINE_H__

#include "tusb422_common.h"
#include "tcpci.h"
#include "tcpm.h"

#define ENABLE_VDM_SUPPORT
#define ENABLE_DP_ALT_MODE_SUPPORT  /* Requires ENABLE_VDM_SUPPORT */

#define MAX_EXT_MSG_LEN 260 /* 260-bytes */

#define MAX_SOP_NUM 5     /* SOP, SOP', SOP", SOP_DBG', SOP_DBG" */

#define PD_STATE_HISTORY_LEN  16
#define PD_STATE_INDEX_MASK   0xF  /* bitmask based on history length */

#define HPD_IN_QUEUE_SIZE  4

typedef enum
{
	PE_UNATTACHED = 0,

	/* Source */
	PE_SRC_STARTUP,
	PE_SRC_DISCOVERY,
	PE_SRC_SEND_CAPS,
	PE_SRC_NEGOTIATE_CAPABILITY,
	PE_SRC_TRANSITION_SUPPLY,
	PE_SRC_TRANSITION_SUPPLY_EXIT,
	PE_SRC_READY,
	PE_SRC_DISABLED,
	PE_SRC_CAPABILITY_RESPONSE,
	PE_SRC_HARD_RESET,
	PE_SRC_HARD_RESET_RECEIVED,
	PE_SRC_TRANSITION_TO_DEFAULT,
	PE_SRC_GET_SINK_CAP,
	PE_SRC_WAIT_NEW_CAPS,
	PE_SRC_SEND_SOFT_RESET,
	PE_SRC_SOFT_RESET,
	PE_SRC_SEND_NOT_SUPPORTED,
	PE_SRC_NOT_SUPPORTED_RECEIVED,
	PE_SRC_PING,
	PE_SRC_SEND_SOURCE_ALERT,
	PE_SRC_SINK_ALERT_RECEIVED,
	PE_SRC_GIVE_SOURCE_CAP_EXT,
	PE_SRC_GIVE_SOURCE_STATUS,
	PE_SRC_GET_SINK_STATUS,

	/* Sink */
	PE_SNK_STARTUP,
	PE_SNK_DISCOVERY,
	PE_SNK_WAIT_FOR_CAPS,
	PE_SNK_EVALUATE_CAPABILITY,
	PE_SNK_SELECT_CAPABILITY,
	PE_SNK_TRANSITION_SINK,
	PE_SNK_READY,
	PE_SNK_HARD_RESET,
	PE_SNK_TRANSITION_TO_DEFAULT,
	PE_SNK_GIVE_SINK_CAP,
	PE_SNK_GET_SOURCE_CAP,
	PE_SNK_SEND_SOFT_RESET,
	PE_SNK_SOFT_RESET,
	PE_SNK_SEND_NOT_SUPPORTED,
	PE_SNK_NOT_SUPPORTED_RECEIVED,
	PE_SNK_SOURCE_ALERT_RECEIVED,
	PE_SNK_SEND_SINK_ALERT,
	PE_SNK_GET_SOURCE_CAP_EXT,
	PE_SNK_GET_SOURCE_STATUS,
	PE_SNK_GIVE_SINK_STATUS,

	/* Dual-Role */
	PE_DR_SRC_GIVE_SINK_CAP,
	PE_DR_SNK_GIVE_SOURCE_CAP,

	/* BIST */
	PE_BIST_CARRIER_MODE,
	PE_BIST_TEST_MODE,

	/* Error Recovery */
	PE_ERROR_RECOVERY,

	/* Data Role Swap */
	PE_DRS_SEND_SWAP,
	PE_DRS_EVALUATE_SWAP,
	PE_DRS_REJECT_SWAP,
	PE_DRS_ACCEPT_SWAP,
	PE_DRS_CHANGE_ROLE,

	/* Power Role Swap */
	PE_PRS_SEND_SWAP,
	PE_PRS_EVALUATE_SWAP,
	PE_PRS_REJECT_SWAP,
	PE_PRS_WAIT_SWAP,
	PE_PRS_ACCEPT_SWAP,
	PE_PRS_TRANSITION_TO_OFF,
	PE_PRS_ASSERT_RD,
	PE_PRS_WAIT_SOURCE_ON,
	PE_PRS_ASSERT_RP,
	PE_PRS_SOURCE_ON,
	PE_PRS_SOURCE_ON_EXIT,

	/* VCONN Swap */
	PE_VCS_SEND_SWAP,
	PE_VCS_EVALUATE_SWAP,
	PE_VCS_REJECT_SWAP,
	PE_VCS_ACCEPT_SWAP,
	PE_VCS_WAIT_FOR_VCONN,
	PE_VCS_TURN_OFF_VCONN,
	PE_VCS_TURN_ON_VCONN,
	PE_VCS_SEND_PS_RDY,

	/* VDM */
#ifdef ENABLE_VDM_SUPPORT
	PE_SRC_VDM_IDENTITY_REQUEST,  /* request to cable before explict contact */
	PE_INIT_PORT_VDM_IDENTITY_REQUEST, /* request to port partner */
	PE_INIT_VDM_SVIDS_REQUEST,
	PE_INIT_VDM_MODES_REQUEST,
	PE_INIT_VDM_ATTENTION_REQUEST,
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	PE_INIT_VDM_DP_STATUS_UPDATE, /* DFP_U only */
	PE_INIT_VDM_DP_CONFIG,        /* DFP_U only */
#endif

	PE_RESP_VDM_NAK,
	PE_RESP_VDM_GET_IDENTITY,
	PE_RESP_VDM_GET_SVIDS,
	PE_RESP_VDM_GET_MODES,
	PE_RCV_VDM_ATTENTION_REQUEST,
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	PE_RESP_VDM_DP_STATUS_UPDATE,
	PE_RESP_VDM_DP_CONFIG,
#endif

	PE_DFP_VDM_MODE_ENTRY_REQUEST,
	PE_DFP_VDM_MODE_EXIT_REQUEST,

	PE_UFP_VDM_EVAL_MODE_ENTRY,
	PE_UFP_VDM_MODE_EXIT,
#endif

	PE_NUM_STATES
} usb_pd_pe_state_t;

extern const char * const pdstate2string[PE_NUM_STATES];

typedef enum
{
	NO_ALT_MODE = 0,
	DP_ALT_MODE = (1 << 0),
	TBOLT_ALT_MODE = (1 << 1),
	HDMI_ALT_MODE = (1 << 2)
} alt_mode_t;

typedef enum
{
	CONFIG_FOR_USB = 0,
	CONFIG_UFP_U_AS_DFP_D = 1,
	CONFIG_UFP_U_AS_UFP_D = 2,
	CONFIG_MASK = 0x3
} dp_config_sel_t;

typedef enum
{
	NOTIFY_HPD_NONE = 0,
	NOTIFY_HPD_LOW  = 1,
	NOTIFY_HPD_HIGH = 2,
	NOTIFY_HPD_IRQ  = 3
} hpd_notify_t;

//typedef enum
//{
//    // BQ - maybe these can be bool in struct below.
//    PD_FLAGS_DUAL_ROLE_POWER = (0 << 0),
//    PD_FLAGS_DUAL_ROLE_DATA  = (1 << 1),
//    PD_FLAG_USB_COMM_CAPABLE = (1 << 2),
//} pd_flags_t;

typedef struct
{
	uint8_t             rx_msg_buf[MAX_EXT_MSG_LEN];

	usb_pd_pe_state_t   state[PD_STATE_HISTORY_LEN];
	unsigned int        state_idx;
	usb_pd_pe_state_t   *current_state;
	bool                state_change;

	tcpc_device_t       *tcpc_dev;

//    uint8_t             flags;   /* BQ can convert all bools to single flag to save RAM if needed */
	unsigned int        port;
	struct tusb422_timer_t  timer;
	struct tusb422_timer_t  timer2;

	uint8_t             power_role;
	uint8_t             data_role;

	tcpc_transmit_t     tx_sop_type;		   /* For incrementing correct msg ID when Tx is successful */
	uint8_t             msg_id[MAX_SOP_NUM];   /* For Tx.  Masked off to 3-bits when building msg header */

	uint8_t             stored_msg_id[MAX_SOP_NUM];	  /* For Rx */
	uint8_t             rx_msg_data_len;
	uint8_t             rx_msg_type;
	uint8_t             rx_msg_spec_rev;
	uint8_t             rx_sop;

	uint8_t             hard_reset_cnt;
	uint8_t             caps_cnt;
	bool                vconn_source;

	bool                non_interruptable_ams;
	bool                swap_source_start;
	bool                start_src_no_response_timer;
	bool                pd_connected;  /* PD connected after a PD msg and GoodCRC have been exchanged */
	bool                pd_connected_since_attach;		/* PD connected at any point since attachment */
	bool                explicit_contract;
	bool                no_response_timed_out;
	bool                vbus_present;
	bool                power_role_swap_in_progress;
	bool                hard_reset_complete;

	uint8_t             object_position;  /* Range: 1 - 7 */
	uint8_t             selected_snk_pdo_idx;
	uint32_t            rdo;
	uint32_t            selected_pdo;
	uint32_t            prev_selected_pdo;

	bool                request_goto_min;
	bool                snk_goto_min;
	bool                snk_wait;

	uint32_t            src_pdo[PD_MAX_PDO_NUM];
	uint32_t            snk_pdo[PD_MAX_PDO_NUM];

	uint16_t            cable_max_current_ma;
	uint16_t            cable_max_voltage_mv;
	bool                active_cable;

	uint8_t             active_alt_modes;

	bool                cable_plug_comm_required;
#ifdef ENABLE_VDM_SUPPORT
	bool                cable_id_req_complete;
	uint8_t             discover_identity_cnt;	 // When sending Discover Identity to cable plug.  Max 20.  Zero upon data role swap.
	uint8_t             vdm_busy_cnt;  // For UFP or Cable Plug.  Max of 1 responder busy response.  Reset upon non-busy response.
	bool                send_src_cap_start_once;
	bool                send_src_cap_timeout;

	bool                port_partner_id_req_complete;
	uint32_t            vdm_hdr_vdos[7];
	bool                vdm_in_progress;
	bool                vdm_initiator;
	uint8_t             svid_index;

	uint8_t             matched_alt_modes;
	bool                vdm_pending;
	usb_pd_pe_state_t   vdm_pending_state;
	bool                mode_exit_pending;
	bool                attention_pending;
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	uint32_t            displayport_remote_caps; /* from port partner */
	uint8_t             displayport_matched_pin_assignments;
	uint8_t             displayport_selected_pin_assignment;

	bool                displayport_alt_mode_configured;
	dp_config_sel_t     displayport_config;
	uint8_t             displayport_lane_cnt;
	uint32_t            displayport_status;  /* local port status */
	uint32_t            displayport_remote_status;  /* remote port status */

	bool                hpd_in_changed;
	uint8_t             hpd_in_value;
	hpd_notify_t        hpd_in_queue[HPD_IN_QUEUE_SIZE];   /* Worst case: HPD_Low, HPD_High, IRQ_HPD, IRQ_HPD. */
	uint8_t             hpd_in_queue_idx;
	uint8_t             hpd_in_send_idx;
#endif
#endif

	bool                externally_powered;
	bool                remote_externally_powered;

} usb_pd_port_t;

typedef enum
{
	STATUS_OK = 0,
	STATUS_REQUEST_NOT_SUPPORTED_IN_CURRENT_STATE
} usb_pd_pe_status_t;


typedef enum
{
	PRL_ALERT_MSG_RECEIVED = 0,
	PRL_ALERT_MSG_TX_SUCCESS,
	PRL_ALERT_MSG_TX_DISCARDED,
	PRL_ALERT_MSG_TX_FAILED,
	PRL_ALERT_HARD_RESET_RECEIVED
} usb_pd_prl_alert_t;

typedef enum
{
	PD_POLICY_MNGR_REQ_GET_SINK_CAPS,
	PD_POLICY_MNGR_REQ_SRC_CAPS_CHANGE,
	PD_POLICY_MNGR_REQ_GOTO_MIN,
	PD_POLICY_MNGR_REQ_UPDATE_REMOTE_CAPS,
	PD_POLICY_MNGR_REQ_PR_SWAP,
	PD_POLICY_MNGR_REQ_DR_SWAP,
	PD_POLICY_MNGR_REQ_VCONN_SWAP,
	PD_POLICY_MNGR_REQ_HARD_RESET
} pd_policy_manager_request_t;

#ifdef ENABLE_DP_ALT_MODE_SUPPORT
typedef enum
{
	IRQ_HPD             = (1 << 8),
	HPD_STATE_HIGH      = (1 << 7),
	REQ_DP_MODE_EXIT    = (1 << 6),
	REQ_USB_CONFIG      = (1 << 5),
	MULTIFUNC_PREFERRED = (1 << 4),
	ADAPTER_ENABLED     = (1 << 3),
	ADAPTER_POWER_LOW   = (1 << 2),
	UFP_D_CONNECTED     = (1 << 1),
	DFP_D_CONNECTED     = 1
} display_port_status_t;

void usb_pd_pe_update_dp_status(unsigned int port, uint32_t status);
#endif


void usb_pd_pe_notify(unsigned int port, usb_pd_prl_alert_t prl_alert);
void usb_pd_pe_connection_state_change_handler(unsigned int port, tcpc_state_t state);
void usb_pd_pe_voltage_alarm_handler(unsigned int port, bool hi_voltage);
void usb_pd_pe_current_change_handler(unsigned int port, tcpc_cc_snk_state_t curr_adv);

int usb_pd_policy_manager_request(unsigned int port, pd_policy_manager_request_t req);
bool usb_pd_pe_is_remote_externally_powered(unsigned int port);
usb_pd_port_t * usb_pd_pe_get_device(unsigned int port);

void usb_pd_pe_init(unsigned int port, usb_pd_port_config_t *config);

#endif
