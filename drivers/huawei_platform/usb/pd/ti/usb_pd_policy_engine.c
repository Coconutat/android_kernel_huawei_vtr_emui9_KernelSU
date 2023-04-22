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

#include "usb_pd_policy_engine.h"
#include "tcpm.h"
#include "tusb422.h"
#include "tusb422_common.h"
#include "usb_pd.h"
#ifdef CONFIG_TUSB422_PAL
	#include "usb_pd_pal.h"
#endif
#include "usb_pd_protocol.h"
#ifdef CONFIG_TUSB422
	#include <linux/string.h>
#else
	#include <string.h>
#endif
#ifdef CONFIG_DUAL_ROLE_USB_INTF
	#include "tusb422_linux_dual_role.h"
	#include <linux/device.h>
	#include <linux/usb/class-dual-role.h>
#endif


/* PD Counters Max */
#define N_CAPS_COUNT        50
#define N_HARD_RESET_COUNT  2
#define N_DISCOVER_IDENTITY_COUNT 20 /* Due to send source cap timeout, the counter will only reach 16 if no GoodCRC received or 5 if VDM sender response times out repeatly */ 
#define N_BUSY_COUNT 1  /* 5 per PD3.0 spec but use 1 to meet USB-C v1.2 requirement for mode entry */

/* PD Timeout Values */
#define T_NO_RESPONSE_MS            5000   /* 4.5 - 5.5 s */
#define T_SENDER_RESPONSE_MS          25   /*  24 - 30 ms */
#define T_VCONN_STABLE_MS             50   /* 50ms max from Type-C spec */
#define T_SWAP_SOURCE_START_MS        40   /*  20 - ? ms*/
#define T_TYPEC_SEND_SOURCE_CAP_MS   150   /* 100 - 200 ms */
#define T_TYPEC_SINK_WAIT_CAP_MS     500   /* 310 - 620 ms */
#define T_PS_HARD_RESET_MS            25   /* 25 - 35 ms */
#define T_PS_TRANSITION_MS           500   /* 450 - 550 ms */
#define T_SRC_RECOVER_MS             700   /* 660 - 1000 ms */
#define T_SRC_TRANSITION_MS           25   /* 25 - 35 ms */
#define T_SINK_REQUEST_MS            200   /* 100 - ? ms */
#define T_BIST_CONT_MODE_MS           50   /* 30 - 60 ms */
//#define T_SRC_TURN_ON_MS             275   /* tSrcTurnOn: 0 - 275ms */
//#define T_SRC_SETTLE_MS              275   /* tSrcSettle: 0 - 275ms */
#define T_PS_SOURCE_OFF_MS           880   /* 750 - 920 ms */
#define T_PS_SOURCE_ON_MS            440   /* 390 - 480 ms */
#define T_VCONN_SOURCE_ON_MS         100   /* ? - 100 ms */
#define T_SINK_TX_MS                  16   /* 16 - 20 ms */

#define T_VBUS_5V_STABLIZE_MS         20   /* delay for VBUS to stablize after vSafe5V-min is reached */

#ifdef ENABLE_VDM_SUPPORT
#define T_DISCOVER_IDENTITY_MS        45   /* 40 - 50 ms */
#define T_VDM_BUSY_MS                100   /* 50 - ? ms */
//#define T_VDM_ENTER_MODE_MS           25   /* ?  - 25ms */
//#define T_VDM_EXIT_MODE_MS            25   /* ?  - 25ms */
#define T_VDM_SENDER_RESPONSE_MS      25   /* 24 - 30ms */
#define T_VDM_WAIT_MODE_ENTRY_MS      45   /* 40 - 50 ms */
#define T_VDM_WAIT_MODE_EXIT_MS       45   /* 40 - 50 ms */
#define T_AME_TIMEOUT_MS            1000   /*  ? - 1 sec */
#endif

#define CABLE_DEFAULT_MAX_VOLTAGE_MV  20000
#define CABLE_DEFAULT_MAX_CURRENT_MA  3000

#ifdef CONFIG_DUAL_ROLE_USB_INTF
	#define DUAL_ROLE_CHANGED()  dual_role_instance_changed(tusb422_dual_role_phy)
#else
	#define DUAL_ROLE_CHANGED()  {}
#endif


usb_pd_port_t pd[NUM_TCPC_DEVICES];
static uint8_t buf[32];

extern void usb_pd_pm_evaluate_src_caps(unsigned int port);
extern usb_pd_port_config_t* usb_pd_pm_get_config(unsigned int port);
extern void build_rdo(unsigned int port);
extern uint32_t get_data_object(uint8_t *obj_data);
extern void build_src_caps(unsigned int port);
extern void build_snk_caps(unsigned int port);


#ifdef ENABLE_VDM_SUPPORT
static void usb_pd_pe_vdm_handler(usb_pd_port_t *dev);
#endif 

const char * const pdstate2string[PE_NUM_STATES] =
{
	"UNATTACHED",

	/* Source */
	"SRC_STARTUP",
	"SRC_DISCOVERY",
	"SRC_SEND_CAPS",
	"SRC_NEGOTIATE_CAPABILITY",
	"SRC_TRANSITION_SUPPLY",
	"SRC_TRANSITION_SUPPLY_EXIT",
	"SRC_READY",
	"SRC_DISABLED",
	"SRC_CAPABILITY_RESPONSE",
	"SRC_HARD_RESET",
	"SRC_HARD_RESET_RECEIVED",
	"SRC_TRANSITION_TO_DEFAULT",
	"SRC_GET_SINK_CAP",
	"SRC_WAIT_NEW_CAPS",
	"SRC_SEND_SOFT_RESET",
	"SRC_SOFT_RESET",
	"SRC_SEND_NOT_SUPPORTED",
	"SRC_NOT_SUPPORTED_RECEIVED",
	"SRC_PING",
	"SRC_SEND_SOURCE_ALERT",
	"SRC_SINK_ALERT_RECEIVED",
	"SRC_GIVE_SOURCE_CAP_EXT",
	"SRC_GIVE_SOURCE_STATUS",
	"SRC_GET_SINK_STATUS",

	/* Sink */
	"SNK_STARTUP",
	"SNK_DISCOVERY",
	"SNK_WAIT_FOR_CAPS",
	"SNK_EVALUATE_CAPABILITY",
	"SNK_SELECT_CAPABILITY",
	"SNK_TRANSITION_SINK",
	"SNK_READY",
	"SNK_HARD_RESET",
	"SNK_TRANSITION_TO_DEFAULT",
	"SNK_GIVE_SINK_CAP",
	"SNK_GET_SOURCE_CAP",
	"SNK_SEND_SOFT_RESET",
	"SNK_SOFT_RESET",
	"SNK_SEND_NOT_SUPPORTED",
	"SNK_NOT_SUPPORTED_RECEIVED",
	"SNK_SOURCE_ALERT_RECEIVED",
	"SNK_SEND_SINK_ALERT",
	"SNK_GET_SOURCE_CAP_EXT",
	"SNK_GET_SOURCE_STATUS",
	"SNK_GIVE_SINK_STATUS",

	/* Dual-role */
	"DR_SRC_GIVE_SINK_CAP",
	"DR_SNK_GIVE_SOURCE_CAP",

	/* BIST */
	"BIST_CARRIER_MODE",
	"BIST_TEST_MODE",

	/* Error Recovery */
	"ERROR_RECOVERY",

	/* Data Role Swap */
	"DRS_SEND_SWAP",
	"DRS_EVALUATE_SWAP",
	"DRS_REJECT_SWAP",
	"DRS_ACCEPT_SWAP",
	"DRS_CHANGE_ROLE",

	/* Power Role Swap */
	"PRS_SEND_SWAP",
	"PRS_EVALUATE_SWAP",
	"PRS_REJECT_SWAP",
	"PRS_WAIT_SWAP",
	"PRS_ACCEPT_SWAP",
	"PRS_TRANSITION_TO_OFF",
	"PRS_ASSERT_RD",
	"PRS_WAIT_SOURCE_ON",
	"PRS_ASSERT_RP",
	"PRS_SOURCE_ON",
	"PRS_SOURCE_ON_EXIT",

	/* VCONN Swap */
	"VCS_SEND_SWAP",
	"VCS_EVALUATE_SWAP",
	"VCS_REJECT_SWAP",
	"VCS_ACCEPT_SWAP",
	"VCS_WAIT_FOR_VCONN",
	"VCS_TURN_OFF_VCONN",
	"VCS_TURN_ON_VCONN",
	"VCS_SEND_PS_RDY",

#ifdef ENABLE_VDM_SUPPORT
	/* VDM */
	"SRC_VDM_IDENTITY_REQUEST",  
	"INIT_PORT_VDM_IDENTITY_REQUEST",
	"INIT_VDM_SVIDS_REQUEST",
	"INIT_VDM_MODES_REQUEST",
	"INIT_VDM_ATTENTION_REQUEST",
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	"INIT_VDM_DP_STATUS_UPDATE",
	"INIT_VDM_DP_CONFIG",       
#endif

	"RESP_VDM_NAK",
	"RESP_VDM_GET_IDENTITY",
	"RESP_VDM_GET_SVIDS",
	"RESP_VDM_GET_MODES",
	"RCV_VDM_ATTENTION_REQUEST",
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	"RESP_VDM_DP_STATUS_UPDATE",
	"RESP_VDM_DP_CONFIG",
#endif

	"DFP_VDM_MODE_ENTRY_REQUEST",
	"DFP_VDM_MODE_EXIT_REQUEST",

	"UFP_VDM_EVAL_MODE_ENTRY",
	"UFP_VDM_MODE_EXIT"
#endif
};


#ifdef ENABLE_DP_ALT_MODE_SUPPORT
#define DP_PORT_CAPABILITY(vdo)  ((vdo) & 0x3)
#define UFP_D_CAPABLE_BIT (1 << 0)
#define DFP_D_CAPABLE_BIT (1 << 1)
#define RECEPTACLE_INDICATION_BIT (1 << 6)
#define DFP_D_PIN_ASSIGNMENTS(vdo)  (((vdo) >> 8) & 0xFF)
#define UFP_D_PIN_ASSIGNMENTS(vdo)  (((vdo) >> 16) & 0xFF)
#endif


#ifndef CONFIG_TUSB422
#if DEBUG_LEVEL >= 1
void pe_debug_state_history(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];
	uint8_t i;

	PRINT("\nPolicy Engine State History:\n");
	for (i = 0; i < PD_STATE_HISTORY_LEN; i++)
	{
		if (dev->state[i] < PE_NUM_STATES)
		{
			PRINT("%s[%u] %s\n", (&dev->state[i] == dev->current_state) ? "->" : "  ", i, pdstate2string[dev->state[i]]);
		}
	}

	return;
}
#endif
#endif

static void pe_set_state(usb_pd_port_t *dev, usb_pd_pe_state_t new_state)
{
	PRINT("PE_%s\n", pdstate2string[new_state]);

	dev->state[dev->state_idx] = new_state;
	dev->current_state = &dev->state[dev->state_idx];
	dev->state_idx++;
	dev->state_idx &= PD_STATE_INDEX_MASK;
	dev->state_change = true;

	return;
}

static void timeout_no_response(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	CRIT("%s\n", __func__);

	dev->no_response_timed_out = true;

	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		if ((dev->tcpc_dev->cc_status & CC_STATUS_CC1_CC2_STATE_MASK) == CC_STATE_OPEN)
		{
			tcpm_enable_vbus_detect(dev->port);
		}
		else if (dev->hard_reset_cnt <= N_HARD_RESET_COUNT)
		{
			pe_set_state(dev, PE_SNK_HARD_RESET);
		}
	}
	else /* Source */
	{
	if (dev->hard_reset_cnt > N_HARD_RESET_COUNT)
	{
		if (!dev->pd_connected_since_attach)
		{
			if ((*dev->current_state == PE_SRC_SEND_CAPS) ||
				((*dev->current_state == PE_SRC_DISCOVERY) && (dev->caps_cnt > N_CAPS_COUNT)))
			{
				pe_set_state(dev, PE_SRC_DISABLED);
			}
		}
		else /* previously PD connected */
		{
			pe_set_state(dev, PE_ERROR_RECOVERY);
			}
		}
	}

	return;
}

static void timer_start_no_response(usb_pd_port_t *dev)
{
	INFO("%s\n", __func__);

	dev->no_response_timed_out = false;

	tusb422_lfo_timer_start(&dev->timer2, T_NO_RESPONSE_MS, timeout_no_response);

	return;
}

static void timer_cancel_no_response(usb_pd_port_t *dev)
{
	INFO("%s\n", __func__);

	if (dev->timer2.function == timeout_no_response)
	{
		tusb422_lfo_timer_cancel(&dev->timer2);
	}

	return;
}


usb_pd_port_t * usb_pd_pe_get_device(unsigned int port)
{
	return &pd[port];
}

#ifdef ENABLE_VDM_SUPPORT

static void timeout_alt_mode_entry(unsigned int port)
{
	CRIT("Alt-Mode entry timeout!\n");

	// Enable billboard.

	return;
}
#endif

static void usb_pd_pe_handle_unattach(usb_pd_port_t *dev)
{
	dev->swap_source_start = false;
	dev->start_src_no_response_timer = false;
	dev->pd_connected = false;
	dev->pd_connected_since_attach = false;
	dev->explicit_contract = false;
	dev->no_response_timed_out = false;
	dev->vbus_present = false;
	dev->power_role_swap_in_progress = false;
	dev->active_alt_modes = 0;
	dev->active_cable = false;
	dev->request_goto_min = false;
	dev->hard_reset_cnt = 0;
	dev->vconn_source = false;
	dev->hard_reset_complete = false;

	// Set defaults for cable voltage and current limits.
	dev->cable_max_voltage_mv = CABLE_DEFAULT_MAX_VOLTAGE_MV;
	dev->cable_max_current_ma = CABLE_DEFAULT_MAX_CURRENT_MA;

	dev->cable_plug_comm_required = false;

#ifdef ENABLE_VDM_SUPPORT
	dev->port_partner_id_req_complete = false;
	dev->discover_identity_cnt = 0;
	dev->cable_id_req_complete = false;
	dev->send_src_cap_timeout = false;
	dev->vdm_in_progress = false;
	dev->vdm_initiator = false;
	dev->matched_alt_modes = 0;
	dev->vdm_pending = false;
	dev->mode_exit_pending = false;
	dev->attention_pending = false;
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	dev->displayport_alt_mode_configured = false;
	dev->displayport_status = 0;
	dev->displayport_remote_status = 0;
	dev->displayport_matched_pin_assignments = 0;
	dev->displayport_lane_cnt = 0;
	dev->hpd_in_changed = false;

	// Deassert HPD for all ports.
	//tcpm_hpd_out_control(0, 0);
#endif
#endif

	return;
}

void usb_pd_pe_connection_state_change_handler(unsigned int port, tcpc_state_t state)
{
	usb_pd_port_t *dev = &pd[port];
#ifdef ENABLE_VDM_SUPPORT
	usb_pd_port_config_t *config = usb_pd_pm_get_config(port);
#endif

	switch (state)
	{
		case TCPC_STATE_UNATTACHED_SRC:
		case TCPC_STATE_UNATTACHED_SNK:
			timer_cancel(&dev->timer);
			tusb422_lfo_timer_cancel(&dev->timer2);
			usb_pd_pe_handle_unattach(dev);
			pe_set_state(dev, PE_UNATTACHED);
			if (!(dev->tcpc_dev->flags & TC_FLAGS_TEMP_ROLE))
			{
				DUAL_ROLE_CHANGED();
			}
			break;

		case TCPC_STATE_ATTACHED_SRC:
			dev->data_role = PD_DATA_ROLE_DFP;
			dev->power_role = PD_PWR_ROLE_SRC;
			dev->vconn_source = true;
			pe_set_state(dev, PE_SRC_STARTUP);
			break;

		case TCPC_STATE_ATTACHED_SNK:
			dev->data_role = PD_DATA_ROLE_UFP;
			dev->power_role = PD_PWR_ROLE_SNK;
			dev->vbus_present = true;
#ifdef ENABLE_VDM_SUPPORT
			if (config->ufp_alt_mode_entry_timeout_enable)
			{
				tusb422_lfo_timer_start(&dev->timer2, T_AME_TIMEOUT_MS, timeout_alt_mode_entry);
			}
#endif
			pe_set_state(dev, PE_SNK_STARTUP);
			break;

		case TCPC_STATE_ORIENTED_DEBUG_ACC_SRC:
		case TCPC_STATE_DEBUG_ACC_SNK:
			break;

		case TCPC_STATE_UNORIENTED_DEBUG_ACC_SRC:
		case TCPC_STATE_AUDIO_ACC:
			// Do nothing.
			break;

		default:
			break;
	}

	return;
}

static void usb_pd_pe_tx_data_msg(unsigned int port, msg_hdr_data_msg_type_t msg_type, tcpc_transmit_t sop_type)
{
	usb_pd_port_t *dev = &pd[port];
	usb_pd_port_config_t *config = usb_pd_pm_get_config(port);
	uint8_t *payload_ptr = &buf[3];
	uint8_t ndo = 0;
	uint32_t *pdo;
	uint8_t pdo_idx;

	if ((msg_type == DATA_MSG_TYPE_SRC_CAPS) ||
		(msg_type == DATA_MSG_TYPE_SNK_CAPS))
	{
		if (msg_type == DATA_MSG_TYPE_SRC_CAPS)
		{
			build_src_caps(port);
			pdo = dev->src_pdo;
			ndo = config->num_src_pdos;
		}
		else /* SNK CAPS */
		{
			build_snk_caps(port);
			pdo = dev->snk_pdo;
			ndo = config->num_snk_pdos;
		}

		for (pdo_idx = 0; pdo_idx < ndo; pdo_idx++)
		{
			*payload_ptr++ = (uint8_t)(pdo[pdo_idx] & 0xFF);
			*payload_ptr++ = (uint8_t)((pdo[pdo_idx] & 0xFF00) >> 8);
			*payload_ptr++ = (uint8_t)((pdo[pdo_idx] & 0xFF0000) >> 16);
			*payload_ptr++ = (uint8_t)((pdo[pdo_idx] & 0xFF000000) >> 24);
		}
	}
	else if (msg_type == DATA_MSG_TYPE_REQUEST)
	{
		ndo = 1;

		payload_ptr[0] = (uint8_t)(dev->rdo & 0xFF);
		payload_ptr[1] = (uint8_t)((dev->rdo & 0xFF00) >> 8);
		payload_ptr[2] = (uint8_t)((dev->rdo & 0xFF0000) >> 16);
		payload_ptr[3] = (uint8_t)((dev->rdo & 0xFF000000) >> 24);
	}
	else
	{
		CRIT("%s: msg_type %u not supported.\n", __func__, msg_type);
	}

	if (ndo > 0)
	{
		usb_pd_prl_tx_data_msg(port, buf, msg_type, sop_type, ndo);
	}

	return;
}

static void pe_send_accept_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_ACCEPT, TCPC_TX_SOP);
	return;
}

static void pe_send_reject_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_REJECT, TCPC_TX_SOP);
	return;
}

static void pe_send_wait_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_WAIT, TCPC_TX_SOP);
	return;
}

static void pe_send_soft_reset_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_SOFT_RESET, TCPC_TX_SOP);
	return;
}

static void pe_send_not_supported_entry(usb_pd_port_t *dev)
{
	// BQ - add option to send SOP'/SOP".
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_NOT_SUPPORTED, TCPC_TX_SOP);
	return;
}


static void usb_pd_pe_unhandled_rx_msg(usb_pd_port_t *dev)
{
	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		if (*dev->current_state == PE_SNK_TRANSITION_SINK)
		{
			pe_set_state(dev, PE_SNK_HARD_RESET);
		}
		else if (*dev->current_state == PE_SNK_READY)
		{
#if (PD_SPEC_REV == PD_REV20)
			pe_send_reject_entry(dev);
#else
			pe_set_state(dev, PE_SNK_SEND_NOT_SUPPORTED);
#endif
		}
		else if (dev->non_interruptable_ams)
		{
			pe_set_state(dev, PE_SNK_SEND_SOFT_RESET);
		}
		else /* Interruptible AMS */
		{
			// Return to PE_SNK_READY and process msg.
			pe_set_state(dev, PE_SNK_READY);
			usb_pd_pe_notify(dev->port, PRL_ALERT_MSG_RECEIVED);
		}
	}
	else /* SRC */
	{
		if ((*dev->current_state == PE_SRC_NEGOTIATE_CAPABILITY) ||
			(*dev->current_state == PE_SRC_TRANSITION_SUPPLY))
		{
			pe_set_state(dev, PE_SRC_HARD_RESET);
		}
		else if (*dev->current_state == PE_SRC_READY)
		{
#if (PD_SPEC_REV == PD_REV20)
			pe_send_reject_entry(dev);
#else
			pe_set_state(dev, PE_SRC_SEND_NOT_SUPPORTED);
#endif
		}
		else if (dev->non_interruptable_ams)
		{
			pe_set_state(dev, PE_SRC_SEND_SOFT_RESET);
		}
		else /* Interruptible AMS */
		{
			// Return to PE_SRC_READY and process msg.
			pe_set_state(dev, PE_SRC_READY);
			usb_pd_pe_notify(dev->port, PRL_ALERT_MSG_RECEIVED);
		}
	}

	return;
}

static void ctrl_msg_handle_accept(usb_pd_port_t *dev)
{
	if (*dev->current_state == PE_SRC_SEND_SOFT_RESET)
	{
		// Stop sender response timer.
		timer_cancel(&dev->timer);
		dev->non_interruptable_ams = false;
		pe_set_state(dev, PE_SRC_SEND_CAPS);
	}
	else if (*dev->current_state == PE_SNK_SEND_SOFT_RESET)
	{
		// Stop sender response timer.
		timer_cancel(&dev->timer);
		dev->non_interruptable_ams = false;
		pe_set_state(dev, PE_SNK_WAIT_FOR_CAPS);
	}
	else if (*dev->current_state == PE_SNK_SELECT_CAPABILITY)
	{
		// Stop sender response timer.
		timer_cancel(&dev->timer);
		pe_set_state(dev, PE_SNK_TRANSITION_SINK);
	}
	else if (*dev->current_state == PE_DRS_SEND_SWAP)
	{
		// Stop sender response timer.
		timer_cancel(&dev->timer);
		pe_set_state(dev, PE_DRS_CHANGE_ROLE);
	}
	else if (*dev->current_state == PE_PRS_SEND_SWAP)
	{
		// Stop sender response timer.
		timer_cancel(&dev->timer);
		dev->power_role_swap_in_progress = true;
		pe_set_state(dev, PE_PRS_TRANSITION_TO_OFF);
	}
	else if (*dev->current_state == PE_VCS_SEND_SWAP)
	{
		// Stop sender response timer.
		timer_cancel(&dev->timer);

		if (dev->vconn_source)
		{
			pe_set_state(dev, PE_VCS_WAIT_FOR_VCONN);
		}
		else
		{
			pe_set_state(dev, PE_VCS_TURN_ON_VCONN);
		}
	}

	return;
}

static void ctrl_msg_handle_wait_or_reject(usb_pd_port_t *dev)
{
	if (*dev->current_state == PE_SNK_SELECT_CAPABILITY)
	{
		if (dev->explicit_contract)
		{
			if (dev->rx_msg_type == CTRL_MSG_TYPE_WAIT)
			{
				dev->snk_wait = true;
			}

			// Restore previously selected PDO.
			dev->selected_pdo = dev->prev_selected_pdo;

			pe_set_state(dev, PE_SNK_READY);

		}
		else
		{
			pe_set_state(dev, PE_SNK_WAIT_FOR_CAPS);
		}
	}
	else if ((*dev->current_state == PE_DRS_SEND_SWAP) ||
			 (*dev->current_state == PE_PRS_SEND_SWAP) ||
			 (*dev->current_state == PE_VCS_SEND_SWAP))
	{
		// Stop sender response timer.
		timer_cancel(&dev->timer);

		if (dev->power_role == PD_PWR_ROLE_SNK)
		{
			pe_set_state(dev, PE_SNK_READY);
		}
		else
		{
			pe_set_state(dev, PE_SRC_READY);
		}
	}

	return;
}

static void ctrl_msg_handle_ps_ready(usb_pd_port_t *dev)
{
	if (*dev->current_state == PE_SNK_TRANSITION_SINK)
	{
		// Cancel PSTransition timer.
		timer_cancel(&dev->timer);
		pe_set_state(dev, PE_SNK_READY);
	}
	else if (*dev->current_state == PE_PRS_WAIT_SOURCE_ON)
	{
		// Cancel PSSourceOn timer.
		timer_cancel(&dev->timer);

		// VBUS is now present. 
		dev->vbus_present = true;

		// Enable VBUS present detection for disconnect.
		tcpm_enable_vbus_detect(dev->port);

		// Enable bleed discharge to ensure timely discharge of 
		// VBUS bulk cap upon disconnect.
		tcpm_set_bleed_discharge(dev->port, true);

		tcpm_sink_vbus(dev->port, false, 5000, GET_SRC_CURRENT_MA(dev->tcpc_dev->src_current_adv));

		pe_set_state(dev, PE_SNK_STARTUP);
	}
	else if (*dev->current_state == PE_PRS_TRANSITION_TO_OFF)
	{
		if (dev->power_role == PD_PWR_ROLE_SNK)
		{
			// Cancel PSSourceOff timer.
			timer_cancel(&dev->timer);
			pe_set_state(dev, PE_PRS_ASSERT_RP);
		}
	}
	else if (*dev->current_state == PE_VCS_WAIT_FOR_VCONN)
	{
		// Cancel VCONNOn timer.
		timer_cancel(&dev->timer);
		pe_set_state(dev, PE_VCS_TURN_OFF_VCONN);
	}

	return;
}

static void ctrl_msg_handle_dr_swap(usb_pd_port_t *dev)
{
	if ((*dev->current_state == PE_SNK_READY) ||
		(*dev->current_state == PE_SRC_READY))
	{
		if (!dev->active_alt_modes)
		{
			pe_set_state(dev, PE_DRS_EVALUATE_SWAP);
		}
		else
		{
			// Send hard reset.
			if (*dev->current_state == PE_SNK_READY)
			{
				pe_set_state(dev, PE_SNK_HARD_RESET);
			}
			else
			{
				pe_set_state(dev, PE_SRC_HARD_RESET);
			}
		}
	}

	return;
}
static void usb_pd_pe_ctrl_msg_rx_handler(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

#ifdef ENABLE_VDM_SUPPORT
	if (dev->vdm_in_progress)
	{
		timer_cancel(&dev->timer);
		if (dev->vdm_initiator)
		{
			dev->vdm_initiator = false;
			dev->vdm_pending_state = *dev->current_state;
			dev->vdm_pending = true;
		}
		// VDM is being interrupted.  Return to Ready state.
		if (dev->power_role == PD_PWR_ROLE_SNK)
		{
			pe_set_state(dev, PE_SNK_READY);
		}
		else /* SRC */
		{
			pe_set_state(dev, PE_SRC_READY);
		}

		dev->state_change = false;
		dev->vdm_in_progress = false;
	}
#endif

	switch (dev->rx_msg_type)
	{
		case CTRL_MSG_TYPE_GOTO_MIN:
			if (*dev->current_state == PE_SNK_READY)
			{
				if (config->giveback_flag)
				{
					dev->snk_goto_min = true;
					pe_set_state(dev, PE_SNK_TRANSITION_SINK);
				}
			}
			break;

		case CTRL_MSG_TYPE_ACCEPT:
			ctrl_msg_handle_accept(dev);
			break;

		case CTRL_MSG_TYPE_WAIT:
			CRIT("Wait Rx'd\n");
		case CTRL_MSG_TYPE_REJECT:
			CRIT("Reject Rx'd\n");
			CRIT("%s Rx'd\n", (dev->rx_msg_type == CTRL_MSG_TYPE_REJECT) ? "Reject" : "Wait");
			ctrl_msg_handle_wait_or_reject(dev);
			break;

		case CTRL_MSG_TYPE_PING:
			// Sink may receive ping msgs but should ignore them.
			break;

		case CTRL_MSG_TYPE_PS_RDY:
			ctrl_msg_handle_ps_ready(dev);
			break;

		case CTRL_MSG_TYPE_GET_SRC_CAP:
			if (*dev->current_state == PE_SRC_READY)
			{
				pe_set_state(dev, PE_SRC_SEND_CAPS);
			}
			else if (*dev->current_state == PE_SNK_READY)
			{
				pe_set_state(dev, PE_DR_SNK_GIVE_SOURCE_CAP);
			}
			break;

		case CTRL_MSG_TYPE_GET_SNK_CAP:
			if (*dev->current_state == PE_SNK_READY)
			{
				pe_set_state(dev, PE_SNK_GIVE_SINK_CAP);
			}
			else if (*dev->current_state == PE_SRC_READY)
			{
				pe_set_state(dev, PE_DR_SRC_GIVE_SINK_CAP);
			}
			break;

		case CTRL_MSG_TYPE_DR_SWAP:
			ctrl_msg_handle_dr_swap(dev);
			break;

		case CTRL_MSG_TYPE_PR_SWAP:
			if ((*dev->current_state == PE_SNK_READY) ||
				(*dev->current_state == PE_SRC_READY))
			{
				pe_set_state(dev, PE_PRS_EVALUATE_SWAP);
			}
			break;

		case CTRL_MSG_TYPE_VCONN_SWAP:
			if ((*dev->current_state == PE_SNK_READY) ||
				(*dev->current_state == PE_SRC_READY))
			{
				pe_set_state(dev, PE_VCS_EVALUATE_SWAP);
			}
			break;

		case CTRL_MSG_TYPE_SOFT_RESET:
			if (dev->power_role == PD_PWR_ROLE_SNK)
			{
				pe_set_state(dev, PE_SNK_SOFT_RESET);
			}
			else
			{
				pe_set_state(dev, PE_SRC_SOFT_RESET);
			}
			break;

		case CTRL_MSG_TYPE_NOT_SUPPORTED:
			if (*dev->current_state == PE_SRC_READY)
			{
				pe_set_state(dev, PE_SRC_NOT_SUPPORTED_RECEIVED);
			}
			else if (*dev->current_state == PE_SNK_READY)
			{
				pe_set_state(dev, PE_SNK_NOT_SUPPORTED_RECEIVED);
			}
			break;

		case CTRL_MSG_TYPE_GET_SRC_CAP_EXT:
			break;

		case CTRL_MSG_TYPE_GET_STATUS:
			break;

		case CTRL_MSG_TYPE_FR_SWAP:
			break;

		default:
			CRIT("Invalid ctrl rx_msg_type 0x%x\n!", dev->rx_msg_type);
			break;
	}

	if (!dev->state_change && (dev->rx_msg_type != CTRL_MSG_TYPE_PING))
	{
		usb_pd_pe_unhandled_rx_msg(dev);
	}

	return;
}

#define BIST_CARRIER_MODE_REQUEST  5
#define BIST_TEST_DATA             8

static void usb_pd_pe_data_msg_rx_handler(usb_pd_port_t *dev)
{
	switch (dev->rx_msg_type)
	{
		case DATA_MSG_TYPE_SRC_CAPS:
			if (*dev->current_state == PE_SNK_WAIT_FOR_CAPS)
			{
				// Cancel SinkWaitCap timer.
				timer_cancel(&dev->timer);
				dev->pd_connected = true;
				dev->pd_connected_since_attach = true;
				pe_set_state(dev, PE_SNK_EVALUATE_CAPABILITY);
			}
			else if (*dev->current_state == PE_SNK_READY)
			{
				pe_set_state(dev, PE_SNK_EVALUATE_CAPABILITY);
			}
			else
			{
				// Hard reset for this particular protocol error.
				pe_set_state(dev, PE_SNK_HARD_RESET);
			}
			break;

		case DATA_MSG_TYPE_REQUEST:
			if ((*dev->current_state == PE_SRC_SEND_CAPS) ||
				(*dev->current_state == PE_SRC_READY))
			{
				// Cancel sender response timer.
				timer_cancel(&dev->timer);
				pe_set_state(dev, PE_SRC_NEGOTIATE_CAPABILITY);
			}
			break;

		case DATA_MSG_TYPE_BIST:
			if ((*dev->current_state == PE_SNK_READY) ||
				(*dev->current_state == PE_SRC_READY))
			{
				// Check we are operating at vSafe5V and power role is not swapped.
				if ((dev->object_position == 1) &&
					(dev->power_role == dev->data_role))
				{
					// Check BIST param, data_obj[31:28].
					if ((dev->rx_msg_buf[3] >> 4) == BIST_CARRIER_MODE_REQUEST)
					{
						pe_set_state(dev, PE_BIST_CARRIER_MODE);
					}
					else if ((dev->rx_msg_buf[3] >> 4) == BIST_TEST_DATA)
					{
						// Set BIST mode.
						tcpm_set_bist_test_mode(dev->port);

						pe_set_state(dev, PE_BIST_TEST_MODE);

						// This test mode shall be ended by a Hard Reset.
					}
				}
			}
			break;

		case DATA_MSG_TYPE_SNK_CAPS:
			if (*dev->current_state == PE_SRC_GET_SINK_CAP)
			{
				// Cancel sender response timer.
				timer_cancel(&dev->timer);
				// Pass sink caps to policy manager. - BQ

				pe_set_state(dev, PE_SRC_READY);
			}
			break;

		case DATA_MSG_TYPE_BATT_STATUS:
			// Response to Get_Battery_Status message.
			break;

		case DATA_MSG_TYPE_ALERT:
			break;

		case DATA_MSG_TYPE_VENDOR:
#ifdef ENABLE_VDM_SUPPORT
			usb_pd_pe_vdm_handler(dev);
#elif (PD_SPEC_REV == PD_REV30) && !defined(CABLE_PLUG)	
			// For PD r3.0, DFP or UFP should return Not_Supported msg if not supported.
			if (dev->power_role == PD_PWR_ROLE_SNK)
			{
				pe_set_state(dev, PE_SNK_SEND_NOT_SUPPORTED);
			}
			else
			{
				pe_set_state(dev, PE_SRC_SEND_NOT_SUPPORTED);
			}
#endif
			break;

		default:
			CRIT("Invalid data rx_msg_type 0x%x\n!", dev->rx_msg_type);
			break;
	}

	// Certain Vendor and BIST msgs may be ignored if not supported so don't check here.
	// For PD r2.0, DFP or UFP should ignore the Vendor msg if not supported.
	// Unsupported BIST msgs are always ignored.
	if (!dev->state_change && 
		(dev->rx_msg_type != DATA_MSG_TYPE_BIST) && 
		(dev->rx_msg_type != DATA_MSG_TYPE_VENDOR))
	{
		usb_pd_pe_unhandled_rx_msg(dev);
	}

	return;
}


#ifdef ENABLE_VDM_SUPPORT

static void usb_pd_pe_tx_vendor_msg(unsigned int port, uint8_t ndo, tcpc_transmit_t sop_type)
{
	usb_pd_port_t *dev = &pd[port];
	uint8_t *payload_ptr = &buf[3];
	uint8_t vdo_idx;

	for (vdo_idx = 0; vdo_idx < ndo; vdo_idx++)
	{
		*payload_ptr++ = (uint8_t)(dev->vdm_hdr_vdos[vdo_idx] & 0xFF);
		*payload_ptr++ = (uint8_t)((dev->vdm_hdr_vdos[vdo_idx] & 0xFF00) >> 8);
		*payload_ptr++ = (uint8_t)((dev->vdm_hdr_vdos[vdo_idx] & 0xFF0000) >> 16);
		*payload_ptr++ = (uint8_t)((dev->vdm_hdr_vdos[vdo_idx] & 0xFF000000) >> 24);
	}

	dev->vdm_in_progress = true;
	dev->vdm_initiator = (USB_PD_VDM_HDR_GET_CMD_TYPE(dev->vdm_hdr_vdos[0]) == VDM_INITIATOR) ? true : false;

	usb_pd_prl_tx_data_msg(port, buf, DATA_MSG_TYPE_VENDOR, sop_type, ndo);

	return;
}

/* Send structured VDM with no VDOs */
static void vdm_send_structured_cmd(unsigned int port, uint16_t svid, vdm_command_t cmd, uint8_t obj_pos, tcpc_transmit_t sop_type)
{
	usb_pd_port_t *dev = &pd[port];

	dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(svid, VDM_TYPE_STRUCTURED, obj_pos, VDM_INITIATOR, cmd);

	usb_pd_pe_tx_vendor_msg(port, 1, sop_type);

	return;
}

#define INVALID_SVID_INDEX  0xFF

static uint8_t find_svid_index(usb_pd_port_t *dev, uint16_t svid)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t svid_idx;

	for (svid_idx = 0; svid_idx < config->num_svids; svid_idx++)
	{
		if (svid == config->svids[svid_idx])
		{
			return svid_idx;
		}
	}

	return INVALID_SVID_INDEX;
}

#ifdef ENABLE_DP_ALT_MODE_SUPPORT

void usb_pd_pe_update_dp_status(unsigned int port, uint32_t status) 
{
	usb_pd_port_t *dev = &pd[port];

	dev->displayport_status = status;
	pe_set_state(dev, PE_INIT_VDM_ATTENTION_REQUEST);
	return;
}

static void vdm_send_dp_status(usb_pd_port_t *dev, vdm_command_type_t cmd_type, uint8_t cmd)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t svid_index = find_svid_index(dev, VDM_DISPLAYPORT_VID);

	// Build header.
	dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_DISPLAYPORT_VID, VDM_TYPE_STRUCTURED, 1, cmd_type, cmd); 

	// Build status.
	dev->vdm_hdr_vdos[1] = dev->displayport_status;

	// Populate DFP_D/UFP_D connected status based on port capability.
	// Assumption is that this is not a DP cable adapter and only supports UFP_D or DFP_D and not both.
	// Otherwise, HPD and AUX should be used per DP Alt-Mode Spec section 4.2.4.2 for detection.
	if (DP_PORT_CAPABILITY(config->modes[svid_index]))
	{
		dev->vdm_hdr_vdos[1] |= DP_PORT_CAPABILITY(~config->modes[svid_index]);
	}

	// If UFP_U, populate bits [6:2].
	if (dev->data_role == PD_DATA_ROLE_UFP)
	{
		if (dev->vdm_hdr_vdos[1] & (UFP_D_CONNECTED | DFP_D_CONNECTED))
		{
			// Set DP functionality to enabled if anything is connected.
			dev->vdm_hdr_vdos[1] |= ADAPTER_ENABLED;
		}

		if (config->multi_function_preferred)
		{
			dev->vdm_hdr_vdos[1] |= MULTIFUNC_PREFERRED;
		}
	}
	else /* DFP */
	{
		// Clear UFP_U specific bits [6:2].
		dev->vdm_hdr_vdos[1] &= 0x183;
	}

	// Tx length is 1 for the VDM header + 1 for the status.
	usb_pd_pe_tx_vendor_msg(dev->port, 2, TCPC_TX_SOP);

	return;
}


static void pe_init_vdm_dp_status_update_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

	// Only a receptacle-based DFP_U can initiate messages other than Attention command.
	if ((dev->active_alt_modes & DP_ALT_MODE) &&
		(dev->data_role == PD_DATA_ROLE_DFP) && 
		(config->modes[find_svid_index(dev, VDM_DISPLAYPORT_VID)] & RECEPTACLE_INDICATION_BIT))
	{
		// Send DP Status Update VDM.
		vdm_send_dp_status(dev, VDM_INITIATOR, VDM_CMD_DP_STATUS_UPDATE);
	}

	return;
}

typedef enum
{
	PIN_ASSIGNMENT_A = 1,
	PIN_ASSIGNMENT_B = (1 << 1),
	PIN_ASSIGNMENT_C = (1 << 2),
	PIN_ASSIGNMENT_D = (1 << 3),
	PIN_ASSIGNMENT_E = (1 << 4),
	PIN_ASSIGNMENT_F = (1 << 5)	  /* for DockPort 1.0a */
} pin_assignment_supported_t;

typedef enum
{
	DP_V1P3_RATE = 1,
	DP_GEN2_RATE = 2
} dp_signaling_rate_t;

static void pe_init_vdm_dp_config_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint16_t config_pin_assignment = 0;
	uint8_t lane_cnt;

	// Only a receptacle-based DFP_U can initiate messages other than Attention command.
	if ((dev->active_alt_modes & DP_ALT_MODE) &&
		(dev->data_role == PD_DATA_ROLE_DFP) && 
		(config->modes[find_svid_index(dev, VDM_DISPLAYPORT_VID)] & RECEPTACLE_INDICATION_BIT))
	{
		// Build header.
		dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_DISPLAYPORT_VID, VDM_TYPE_STRUCTURED, 1, VDM_INITIATOR, VDM_CMD_DP_CONFIG); 

		// Build DP config.
		dev->vdm_hdr_vdos[1] = 0;

		dev->vdm_hdr_vdos[1] |= dev->displayport_config;
		PRINT("%s dev->displayport_config = %u\n", __func__, dev->displayport_config);
		if (dev->displayport_config != CONFIG_FOR_USB)
		{
			// Select DP pin assignment. Give priority to higher DP data rate and then multifuction.
			// Pin assignments A and B use DP Gen2 signaling.  
			// If multifunction is preferred, use Pin Assignment B, D, or F if possible.
			if ((dev->displayport_matched_pin_assignments & PIN_ASSIGNMENT_A))
			{
				config_pin_assignment = PIN_ASSIGNMENT_A;

				if (dev->active_cable)
				{
					lane_cnt = 2;
				}
				else
				{
					lane_cnt = 4;
				}
			}

			if ((dev->displayport_matched_pin_assignments & PIN_ASSIGNMENT_B) && 
				(dev->displayport_remote_status & MULTIFUNC_PREFERRED))
			{
				config_pin_assignment = PIN_ASSIGNMENT_B;

				if (dev->active_cable)
				{
					lane_cnt = 1;
				}
				else
				{
					lane_cnt = 2;
				}
			}

			// If assignment A or B was not selected.
			if (!config_pin_assignment)
			{
				if ((dev->displayport_matched_pin_assignments & PIN_ASSIGNMENT_C))
				{
					config_pin_assignment = PIN_ASSIGNMENT_C;
					lane_cnt = 4;
				}

				if ((dev->displayport_matched_pin_assignments & PIN_ASSIGNMENT_D) && 
					(dev->displayport_remote_status & MULTIFUNC_PREFERRED))
				{
					config_pin_assignment = PIN_ASSIGNMENT_D;
					lane_cnt = 2;
				}
			}

			// If assignment A, B, C, or D was not selected.
			if (!config_pin_assignment)
			{
				if ((dev->displayport_matched_pin_assignments & PIN_ASSIGNMENT_E))
				{
					config_pin_assignment = PIN_ASSIGNMENT_E;
					lane_cnt = 4;
				}

				if ((dev->displayport_matched_pin_assignments & PIN_ASSIGNMENT_F) && 
					(dev->displayport_remote_status & MULTIFUNC_PREFERRED))
				{
					config_pin_assignment = PIN_ASSIGNMENT_F;
					lane_cnt = 2;
				}
			}

			// Set pin assignment configuration field.
			dev->vdm_hdr_vdos[1] |= (config_pin_assignment << 8);

			// Set DP data rate field based on pin assignment.
			if (config_pin_assignment & (PIN_ASSIGNMENT_A | PIN_ASSIGNMENT_B))
			{
				// DP Gen2 rate.
				dev->vdm_hdr_vdos[1] |= DP_GEN2_RATE << 2;
			}
			else
			{
				// DP v1.3 rate.
				dev->vdm_hdr_vdos[1] |= DP_V1P3_RATE << 2;
			}

			dev->displayport_selected_pin_assignment = config_pin_assignment;
			PRINT("lane_cnt = %u\n", lane_cnt);
			if (lane_cnt != dev->displayport_lane_cnt)
			{
				// Place any Type-C pins to be reconfigured to in Safe state.
				if (lane_cnt == 4)
				{
					tcpm_mux_control(dev->port, dev->data_role, MUX_DISABLE, dev->tcpc_dev->plug_polarity);
				}
				else /* 2-lane */
				{
					tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);
				}

				if (dev->displayport_lane_cnt)
				{
					// If changing the number of DP lanes, force HPD low and isolate SBU pins.
					// Then ensure HPD is low for at least 3ms.
					//tcpm_hpd_out_control(dev->port, 0);
					//tcpm_msleep(3);
				}

				dev->displayport_lane_cnt = lane_cnt;
			}
		}
		else /* Config for USB */
		{
			// Deassert HPD.
			//tcpm_hpd_out_control(dev->port, 0);

			// Isolate any pins (including SBUs) previously configured for DP.
			if (dev->displayport_lane_cnt == 4)
			{
				tcpm_mux_control(dev->port, dev->data_role, MUX_DISABLE, dev->tcpc_dev->plug_polarity);
			}
			else if (dev->displayport_lane_cnt == 2)
			{
				tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);
			}

			dev->displayport_lane_cnt = 0;
		}

		// Tx length is 1 for the VDM header + 1 for the configuration.
		usb_pd_pe_tx_vendor_msg(dev->port, 2, TCPC_TX_SOP);
	}

	return;
}


static void pe_resp_vdm_dp_status_update_entry(usb_pd_port_t *dev)
{
	vdm_send_dp_status(dev, VDM_RESP_ACK, VDM_CMD_DP_STATUS_UPDATE);
	return;
}

static void pe_resp_vdm_dp_config_entry(usb_pd_port_t *dev)
{
	// Send ACK.
	dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_DISPLAYPORT_VID, VDM_TYPE_STRUCTURED, 1, VDM_RESP_ACK, VDM_CMD_DP_CONFIG); 

	// Tx length is 1 for the VDM header.
	usb_pd_pe_tx_vendor_msg(dev->port, 1, TCPC_TX_SOP);

	return;
}

static void timeout_vdm_busy(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if ((*dev->current_state == PE_SRC_READY) ||
		(*dev->current_state == PE_SNK_READY))
	{
		pe_set_state(dev, dev->vdm_pending_state);
	}
	else
	{
		dev->vdm_pending = true;
	}
	return;

}

static void vdm_handle_dp_status_update(usb_pd_port_t *dev, vdm_command_type_t cmd_type)
{
	if (cmd_type == VDM_INITIATOR)
	{
		// Only UFP responds to DP Status Update.  
		// UFP doesn't need to use any of the data sent by the DFP.
		if (dev->data_role == PD_DATA_ROLE_UFP)
		{
			pe_set_state(dev, PE_RESP_VDM_DP_STATUS_UPDATE);
		}
	}
	else if (*dev->current_state == PE_INIT_VDM_DP_STATUS_UPDATE)
	{
		if (cmd_type == VDM_RESP_ACK)
		{
			// Store the remote DP status.
			dev->displayport_remote_status = get_data_object(dev->rx_msg_buf + 4);

			if (dev->active_alt_modes & DP_ALT_MODE)
			{
				// Enter Mode completed.

				if (!dev->displayport_alt_mode_configured)
				{
					// Set state to send DP Config if connected but not configured.
					if (dev->displayport_remote_status & UFP_D_CONNECTED)
					{
						dev->displayport_config = CONFIG_UFP_U_AS_UFP_D;
						pe_set_state(dev, PE_INIT_VDM_DP_CONFIG);
					}
					else if (dev->displayport_remote_status & DFP_D_CONNECTED)
					{
						dev->displayport_config = CONFIG_UFP_U_AS_DFP_D;
						pe_set_state(dev, PE_INIT_VDM_DP_CONFIG);
					}
				}


//				if (dev->displayport_remote_status & HPD_STATE_HIGH)
//				{
					// Assert HPD to DFP_D.
					//tcpm_hpd_out_control(dev->port, HPD_HIGH);
//
//				else
//				{
					// Deassert HPD to DFP_D.
					//tcpm_hpd_out_control(dev->port, HPD_LOW);
//				}
		
//				if (dev->displayport_remote_status & IRQ_HPD)
//				{
					// Pulse HPD pin low for 1ms.
					//tcpm_hpd_out_control(dev->port, HPD_IRQ);
//				}
			}
		}
		else if (cmd_type == VDM_RESP_BUSY)
		{
			// Retry after tVDMBusy.
			dev->vdm_pending_state = *dev->current_state;
			timer_start(&dev->timer, T_VDM_BUSY_MS, timeout_vdm_busy);
		}
	}

	return;
}

static void vdm_handle_dp_config(usb_pd_port_t *dev, vdm_command_type_t cmd_type)
{
	uint32_t dp_config;

	if (cmd_type == VDM_INITIATOR)
	{
		// Only UFP responds to DP Config.  
		if (dev->data_role == PD_DATA_ROLE_UFP)
		{
			dp_config = get_data_object(dev->rx_msg_buf + 4);

			// Configure pins.
			if ((dp_config & CONFIG_MASK) == CONFIG_FOR_USB)
			{
				tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);
			}
			else /* CONFIG_UFP_U_AS_UFP_D or CONFIG_UFP_U_AS_DFP_D */
			{
				switch ((dp_config >> 8) & 0xFF)
				{
					case PIN_ASSIGNMENT_A:
					case PIN_ASSIGNMENT_C:
					case PIN_ASSIGNMENT_E:
						// 4-lane.
						tcpm_mux_control(dev->port, dev->data_role, MUX_DP_4LANE, dev->tcpc_dev->plug_polarity);
						break;

					case PIN_ASSIGNMENT_B:
					case PIN_ASSIGNMENT_D:
					case PIN_ASSIGNMENT_F: /* DFP_D only */
						// 2-lane.
						tcpm_mux_control(dev->port, dev->data_role, MUX_DP_2LANE, dev->tcpc_dev->plug_polarity);
						break;

					default:
						PRINT("Invalid UFP_U pin assignment = 0x%x\n", (dp_config >> 8));
						break;
				}

				// Check for HPD high.
				if (tcpm_get_hpd_in(0))
				{
					dev->hpd_in_queue[dev->hpd_in_queue_idx++] = NOTIFY_HPD_HIGH;
				}
			}

			pe_set_state(dev, PE_RESP_VDM_DP_CONFIG);
		}
	}
	else if (*dev->current_state == PE_INIT_VDM_DP_CONFIG)
	{
		if (cmd_type == VDM_RESP_ACK)
		{
			dev->displayport_alt_mode_configured = true;
			PRINT("%s ACK\n", __func__);
			PRINT("dev->displayport_config = %u\n", dev->displayport_config);
			// Configure pins.
			if (dev->displayport_config == CONFIG_FOR_USB)
			{
				tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);
			}
			else /* CONFIG_UFP_U_AS_UFP_D or CONFIG_UFP_U_AS_DFP_D */
			{
				PRINT("dev->displayport_selected_pin_assignment = %u\n", dev->displayport_selected_pin_assignment);
				switch (dev->displayport_selected_pin_assignment)
				{
					case PIN_ASSIGNMENT_A:
					case PIN_ASSIGNMENT_C:
					case PIN_ASSIGNMENT_E:
						// 4-lane.
						tcpm_mux_control(dev->port, dev->data_role, MUX_DP_4LANE, dev->tcpc_dev->plug_polarity);
						break;

					case PIN_ASSIGNMENT_B:
					case PIN_ASSIGNMENT_D:
					case PIN_ASSIGNMENT_F: /* DFP_D only */
						// 2-lane.
						tcpm_mux_control(dev->port, dev->data_role, MUX_DP_2LANE, dev->tcpc_dev->plug_polarity);
						break;

					default:
						break;
				}
				if (dev->displayport_remote_status & HPD_STATE_HIGH)
				{
					PRINT("HPD_OUT: High\n");
					tcpm_hpd_out_control(dev->port, HPD_HIGH);
				}
				else
				{
					PRINT("HPD_OUT: Low\n");
					tcpm_hpd_out_control(dev->port, HPD_LOW);
				}
				if (dev->displayport_remote_status & IRQ_HPD)
				{
					PRINT("HPD_OUT: IRQ not handled\n");
				}
			}
		}
		else if (cmd_type == VDM_RESP_NACK)
		{
			// UFP was unable to set configuration.
			// Try another config or give up?? - BQ
			PRINT("%s NACK\n", __func__);
		}
		else
		{
			PRINT("%s BUSY\n", __func__);
			dev->vdm_pending_state = *dev->current_state;
			timer_start(&dev->timer, T_VDM_BUSY_MS, timeout_vdm_busy);
		}
	}

	return;
}

static void usb_pd_pe_update_displayport_status(usb_pd_port_t *dev)
{
	if (dev->hpd_in_queue[dev->hpd_in_send_idx] != NOTIFY_HPD_NONE)
	{
		// Update the DisplayPort status.
		if (dev->hpd_in_queue[dev->hpd_in_send_idx] == NOTIFY_HPD_LOW)
		{
			// If HPD is low, IRQ_HPD must also be low.
			dev->displayport_status &= ~(HPD_STATE_HIGH | IRQ_HPD);
		}
		else if (dev->hpd_in_queue[dev->hpd_in_send_idx] == NOTIFY_HPD_HIGH)
		{
			dev->displayport_status |= HPD_STATE_HIGH;
		}
		else if (dev->hpd_in_queue[dev->hpd_in_send_idx] == NOTIFY_HPD_IRQ)
		{
			dev->displayport_status |= (HPD_STATE_HIGH | IRQ_HPD);
		}

		// Clear the item in the queue.
		dev->hpd_in_queue[dev->hpd_in_send_idx] = NOTIFY_HPD_NONE;

		dev->hpd_in_send_idx++;
		if (dev->hpd_in_send_idx >= HPD_IN_QUEUE_SIZE)
		{
			dev->hpd_in_send_idx = 0;
		}
	}

	return;
}

static void timeout_irq_hpd(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	// Clear the timer callback function.
	dev->timer2.function = NULL;

	if (tcpm_get_hpd_in(port) == 0)
	{
		if (dev->active_alt_modes & DP_ALT_MODE)
		{
			// HPD is still low, no IRQ_HPD.
			dev->hpd_in_queue[dev->hpd_in_queue_idx++] = NOTIFY_HPD_LOW; 
			if (dev->hpd_in_queue_idx >= HPD_IN_QUEUE_SIZE)
			{
				dev->hpd_in_queue_idx = 0;
			}

			if (!dev->state_change &&
				((*dev->current_state == PE_SNK_READY) || 
				 (*dev->current_state == PE_SRC_READY)))
			{
			pe_set_state(dev, PE_INIT_VDM_ATTENTION_REQUEST);
			}
			else
			{
				dev->attention_pending = true;
			}
		}
	}

	return;
}

#define T_IRQ_HPD_MAX_MS  2

void usb_pd_pe_hpd_in_event(unsigned int port, uint8_t val)
{
	usb_pd_port_t *dev = &pd[port];

	if (dev->active_alt_modes & DP_ALT_MODE)
	{
		dev->hpd_in_changed = true;
		dev->hpd_in_value = val;
	}

	return;
}

#endif /* ENABLE_DP_ALT_MODE_SUPPORT */


static void pe_init_vdm_svids_request_entry(usb_pd_port_t *dev)
{
	// Send Discover SVIDs VDM to port partner.
	vdm_send_structured_cmd(dev->port, VDM_PD_SID, VDM_CMD_DISCOVER_SVIDS, 0, TCPC_TX_SOP);
	return;
}

static void pe_init_vdm_modes_request_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

	// Implement Alt-Modes here in order of priority.

#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	// Only a receptacle-based DFP_U can initiate messages other than Attention command.
	if ((dev->matched_alt_modes & DP_ALT_MODE) &&
		(dev->data_role == PD_DATA_ROLE_DFP) && 
		(config->modes[find_svid_index(dev, VDM_DISPLAYPORT_VID)] & RECEPTACLE_INDICATION_BIT))
	{
		// Send Discover Modes VDM to port partner.
		vdm_send_structured_cmd(dev->port, VDM_DISPLAYPORT_VID, VDM_CMD_DISCOVER_MODES, 0, TCPC_TX_SOP);
	}
#endif
	return;
}


/* used by UFP_U to signal change in status */
static void pe_init_vdm_attention_request_entry(usb_pd_port_t *dev)
{
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	// Only a UFP_U (plug or receptacle) can issue an Attention command in DP mode.  
	if ((dev->active_alt_modes & DP_ALT_MODE) &&
		(dev->data_role == PD_DATA_ROLE_UFP))
	{
		usb_pd_pe_update_displayport_status(dev);

		// Send Attention VDM to port partner.
		vdm_send_dp_status(dev, VDM_INITIATOR, VDM_CMD_ATTENTION);
	}
#endif
	return;
}


static void pe_resp_vdm_nak_entry(usb_pd_port_t *dev)
{
	uint32_t vdm_hdr = get_data_object(dev->rx_msg_buf);

	// NACK if not supported.
	dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(USB_PD_VDM_HDR_GET_SVID(vdm_hdr), 
											  VDM_TYPE_STRUCTURED, 
											  USB_PD_VDM_HDR_GET_OBJ_POS(vdm_hdr), 
											  VDM_RESP_NACK, 
											  USB_PD_VDM_HDR_GET_CMD(vdm_hdr)); 

	usb_pd_pe_tx_vendor_msg(dev->port, 1, (tcpc_transmit_t)dev->rx_sop);
	return;
}


static void pe_resp_vdm_get_svids_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t i;
	uint8_t num_vdos;
	uint16_t *svid = (uint16_t*)&dev->vdm_hdr_vdos[1];

	// Clear VDM buffer.
	memset(dev->vdm_hdr_vdos, 0, sizeof(dev->vdm_hdr_vdos));

	if (config->num_svids)
	{
		dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_PD_SID, VDM_TYPE_STRUCTURED, 0, VDM_RESP_ACK, VDM_CMD_DISCOVER_SVIDS); 

		for (i = 0; i < config->num_svids; i++)
		{
			// Put odd number SVIDs in the lower 16-bits of the VDO
			// and even number SVIDs i the upper 16-bits of the VDO.
			if (i & 0x1)
			{
				svid[i-1] = config->svids[i];
			}
			else
			{
				svid[i+1] = config->svids[i];
			}
		}
	}
	else
	{
		// Return NAK if no SVIDs.
		dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_PD_SID, VDM_TYPE_STRUCTURED, 0, VDM_RESP_NACK, VDM_CMD_DISCOVER_SVIDS); 
	}

	// Determine the number of data objects for the SVIDs.
	num_vdos = (config->num_svids + 1) >> 1;

	if (config->num_svids && !(config->num_svids & 0x01))
	{
		// If even number of SVIDs, we need to send an extra VDO with all zeros.
		num_vdos++;
	}

	// Tx length is 1 for the VDM header + number of VDOs.
	usb_pd_pe_tx_vendor_msg(dev->port, (1 + num_vdos), (tcpc_transmit_t)dev->rx_sop);

	return;
}

static void pe_resp_vdm_get_modes_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t svid_idx;
	uint8_t num_modes = 0;
	uint16_t svid;
	uint32_t vdm_hdr = get_data_object(dev->rx_msg_buf);

	svid = USB_PD_VDM_HDR_GET_SVID(vdm_hdr);

	// Search for matching SVID.
	for (svid_idx = 0; svid_idx < config->num_svids; svid_idx++)
	{
		if (svid == config->svids[svid_idx])
		{
			// Currently, only one mode per SVID is supported.
			dev->vdm_hdr_vdos[1] = config->modes[svid_idx];
			num_modes = 1;
			break;
		}
	}

	// Build header.  Return NAK if no modes supported.
	dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(svid, VDM_TYPE_STRUCTURED, 0, (num_modes) ? VDM_RESP_ACK : VDM_RESP_NACK, VDM_CMD_DISCOVER_MODES); 

	// Tx length is 1 for the VDM header + number of modes.
	usb_pd_pe_tx_vendor_msg(dev->port, (1 + num_modes), (tcpc_transmit_t)dev->rx_sop);

	return;
}

static void pe_resp_vdm_get_identity_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t i;
	uint32_t *prod_type_vdos = &dev->vdm_hdr_vdos[4];

	dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_PD_SID, VDM_TYPE_STRUCTURED, 0, VDM_RESP_ACK, VDM_CMD_DISCOVER_IDENTITY); 
	dev->vdm_hdr_vdos[1] = config->id_header_vdo;
	dev->vdm_hdr_vdos[2] = config->cert_stat_vdo;
	dev->vdm_hdr_vdos[3] = config->product_vdo;

#if (PD_SPEC_REV == PD_REV20)
	// Ensure reserved bits of ID header are zero.
	dev->vdm_hdr_vdos[1] &= 0xFC00FFFF; 
#endif

	for (i = 0; i < config->num_product_type_vdos; i++)
	{
		prod_type_vdos[i] = config->product_type_vdos[i];
	}

	usb_pd_pe_tx_vendor_msg(dev->port, (4 + config->num_product_type_vdos), (tcpc_transmit_t)dev->rx_sop);

	return;
}




static void timeout_send_src_cap(unsigned int port)
{
	pd[port].send_src_cap_timeout = true;
	return;
}


#define ID_HDR_GET_CABLE_PLUG_PROD_TYPE(id_hdr)  (((id_hdr) >> 27) & 0x7)
#define ID_HDR_GET_UFP_PROD_TYPE(id_hdr)  (((id_hdr) >> 27) & 0x7)
#define ID_HDR_GET_DFP_PROD_TYPE(id_hdr)  (((id_hdr) >> 23) & 0x7)

typedef enum 
{
	CABLE_PROD_TYPE_PASSIVE_CABLE = 3,
	CABLE_PROD_TYPE_ACTIVE_CABLE = 4,
} cable_product_type_t;

#define CURRENT_CAP_5A_BIT      (1 << 6)
#define VBUS_THROUGH_CABLE_BIT  (1 << 4)
#define CABLE_VDO_GET_MAX_VBUS_VOLTAGE_MV(vdo)  (((((vdo) >> 9) & 0x3) * 10000) + 20000);

typedef enum 
{
	UFP_PROD_TYPE_PD_USB_HUB = 1,
	UFP_PROD_TYPE_PD_USB_PERIPHERAL = 2,
	UFP_PROD_TYPE_ALT_MODE_ADAPTER = 5,
} ufp_prod_type_t;

typedef enum 
{
	DFP_PROD_TYPE_PD_USB_HUB = 1,
	DFP_PROD_TYPE_PD_USB_HOST = 2,
	DFP_PROD_TYPE_POWER_BRICK = 3,
	DFP_PROD_TYPE_ALT_MODE_CTRL = 4,
} dfp_prod_type_t;

#define MODAL_OPERATION_SUPPORTED ((uint32_t)1 << 26)

static void vdm_handle_discover_identity(usb_pd_port_t *dev, vdm_command_type_t cmd_type)
{
	uint32_t id_hdr;
	uint32_t prod_type_vdo;

	if ((*dev->current_state == PE_SRC_VDM_IDENTITY_REQUEST) && 
		(dev->rx_sop == TCPC_TX_SOP_P))
	{
		dev->cable_id_req_complete = true;

		// Cancel the send source cap timer.
		if (dev->timer2.function == timeout_send_src_cap)
		{
			tusb422_lfo_timer_cancel(&dev->timer2);
		}

		// Handle Discovery Identity response from cable.
		if (cmd_type == VDM_RESP_ACK)
		{
			// Interpret Cable VDO.
			id_hdr = get_data_object(dev->rx_msg_buf + 4);
			prod_type_vdo = get_data_object(dev->rx_msg_buf + 16);

			if (ID_HDR_GET_CABLE_PLUG_PROD_TYPE(id_hdr) == CABLE_PROD_TYPE_PASSIVE_CABLE)
			{
				if (prod_type_vdo & CURRENT_CAP_5A_BIT)
				{
					dev->cable_max_current_ma = 5000;
				}
			}
			else if (ID_HDR_GET_CABLE_PLUG_PROD_TYPE(id_hdr) == CABLE_PROD_TYPE_ACTIVE_CABLE)
			{
				dev->active_cable = true;

				if (prod_type_vdo & VBUS_THROUGH_CABLE_BIT)
				{
					if (prod_type_vdo & CURRENT_CAP_5A_BIT)
					{
						dev->cable_max_current_ma = 5000;
					}
				}
				else
				{
					dev->cable_max_current_ma = 0;
				}
			}

			// Check cable SuperSpeed signaling support. - BQ

			if (dev->rx_msg_spec_rev == PD_REV30)
			{
				dev->cable_max_voltage_mv = CABLE_VDO_GET_MAX_VBUS_VOLTAGE_MV(prod_type_vdo);
			}

			CRIT("Cable supports %uV %uA\n", dev->cable_max_voltage_mv / 1000,
				 dev->cable_max_current_ma / 1000);

			pe_set_state(dev, PE_SRC_SEND_CAPS);
		}
		else if (cmd_type == VDM_RESP_BUSY)
		{
			if (dev->discover_identity_cnt < (N_DISCOVER_IDENTITY_COUNT - 1))
			{
				// Retry after tVDMBusy.
				dev->vdm_pending_state = PE_SRC_DISCOVERY;
				timer_start(&dev->timer, T_VDM_BUSY_MS, timeout_vdm_busy);
			}
			else
			{
				// Go directly to PE_SRC_DISCOVERY which will start PE_SRC_SEND_CAPS.
				pe_set_state(dev, PE_SRC_DISCOVERY);
			}
		}
		else /* NACK */
		{
			pe_set_state(dev, PE_SRC_DISCOVERY);
		}
	}
	else if ((*dev->current_state == PE_INIT_PORT_VDM_IDENTITY_REQUEST) && 
			 (dev->rx_sop == TCPC_TX_SOP))
	{
		dev->port_partner_id_req_complete = true;

		// Handle Discovery Identity response from cable.
		if (cmd_type == VDM_RESP_ACK)
		{
			id_hdr = get_data_object(dev->rx_msg_buf + 4);

			// If product type is alt mode adapter.
			if (ID_HDR_GET_UFP_PROD_TYPE(id_hdr) & UFP_PROD_TYPE_ALT_MODE_ADAPTER)
			{
				// Get AMA VDO.
				prod_type_vdo = get_data_object(dev->rx_msg_buf + 16);

				// Check AMA SuperSpeed signaling support. - BQ
			}

			if (id_hdr & MODAL_OPERATION_SUPPORTED)
			{
				pe_set_state(dev, PE_INIT_VDM_SVIDS_REQUEST);
			}
		}
		else if (cmd_type == VDM_RESP_BUSY)
		{
			// Retry after tVDMBusy.
			dev->vdm_pending_state = *dev->current_state;
			timer_start(&dev->timer, T_VDM_BUSY_MS, timeout_vdm_busy);
		}
		else /* NACK */
		{
		}
	}
	else if ((cmd_type == VDM_INITIATOR) &&
			 (dev->rx_sop == TCPC_TX_SOP))
	{
		pe_set_state(dev, PE_RESP_VDM_GET_IDENTITY);
	}




	return;
}

static uint16_t get_svid(uint32_t vdo, uint8_t svid_num)
{
	return(svid_num == 0) ? (vdo >> 16) : (vdo & 0xFFFF);
}

static void vdm_handle_discover_svids(usb_pd_port_t *dev, vdm_command_type_t cmd_type)
{
	bool last_svid = false;
	bool svid_matched = false;
	uint16_t svid;
	uint32_t vdo;
	uint8_t *vdo_ptr = dev->rx_msg_buf + 4;
	uint8_t num_vdos = (dev->rx_msg_data_len >> 2) - 1;
	uint8_t svid_idx, vdo_idx, svid_num;
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

	if (cmd_type == VDM_INITIATOR)
	{
		if (dev->rx_sop == TCPC_TX_SOP)
		{
			pe_set_state(dev, PE_RESP_VDM_GET_SVIDS);
		}
	}
	else if (*dev->current_state == PE_INIT_VDM_SVIDS_REQUEST)
	{
		if (cmd_type == VDM_RESP_ACK)
		{
			// Check SVIDs for any we support.  
			for (svid_idx = 0; (svid_idx < config->num_svids) && !last_svid; svid_idx++)
			{
				for (vdo_idx = 0; (vdo_idx < num_vdos) && !last_svid; vdo_idx++)
				{
					vdo = get_data_object(vdo_ptr + (vdo_idx << 2));

					// Two SVIDs per VDO.
					for (svid_num = 0; svid_num < 2; svid_num++)
					{
						svid = get_svid(vdo, svid_num);

						INFO("SVID = 0x%04x\n", svid);

						if (svid == 0)
						{
							last_svid = true;
							break;
						}
						else if (svid == config->svids[svid_idx])
						{
							// Matched SVID to one we support.
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
							if (svid == VDM_DISPLAYPORT_VID)
							{
								dev->matched_alt_modes |= DP_ALT_MODE;
								svid_matched = true;
							}
#endif
							// [VDM] Add other supported Alt modes here...
						}
					}
				}
			}

			if (!last_svid)
			{
				pe_set_state(dev, PE_INIT_VDM_SVIDS_REQUEST);
			}
		}
		else if (cmd_type == VDM_RESP_BUSY)
		{
			// Retry after tVDMBusy.
			dev->vdm_pending_state = *dev->current_state;
			timer_start(&dev->timer, T_VDM_BUSY_MS, timeout_vdm_busy);
		}
	}

	if (svid_matched)
	{
		pe_set_state(dev, PE_INIT_VDM_MODES_REQUEST);
	}



	return;
}

static void vdm_handle_discover_modes(usb_pd_port_t *dev, vdm_command_type_t cmd_type)
{
	uint32_t mode;
	uint8_t *mode_ptr = dev->rx_msg_buf + 4; /* add 4 to skip VDM header */
	uint8_t mode_idx;
	uint8_t num_modes = (dev->rx_msg_data_len >> 2) - 1; /* subtract 1 for the VDM header */
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t remote_pin_assignments;

	INFO("num_modes = %u\n", num_modes);

	if (cmd_type == VDM_INITIATOR)
	{
		if (dev->rx_sop == TCPC_TX_SOP)
		{
			pe_set_state(dev, PE_RESP_VDM_GET_MODES);
		}
	}
	else if (*dev->current_state == PE_INIT_VDM_MODES_REQUEST)
	{
		if (cmd_type == VDM_RESP_ACK)
		{
			for (mode_idx = 0; mode_idx < num_modes; mode_idx++)
			{
				mode = get_data_object(mode_ptr + (mode_idx << 2));

#ifdef ENABLE_DP_ALT_MODE_SUPPORT
				if (config->svids[dev->svid_index] == VDM_DISPLAYPORT_VID)
				{
					dev->displayport_remote_caps = mode;

					if (mode & RECEPTACLE_INDICATION_BIT)
					{
						remote_pin_assignments = UFP_D_PIN_ASSIGNMENTS(mode);
					}
					else
					{
						remote_pin_assignments = DFP_D_PIN_ASSIGNMENTS(mode);
					}

					// Check if UFP_U capabilities and at least one pin assignment matches ours.
					// Assume we are DFP_D or UFP_D capable but not both.
					if ((config->modes[dev->svid_index] & DFP_D_CAPABLE_BIT) && 
						(mode & UFP_D_CAPABLE_BIT))
					{
						dev->displayport_matched_pin_assignments = DFP_D_PIN_ASSIGNMENTS(config->modes[dev->svid_index]) & remote_pin_assignments;
					}
					else if ((config->modes[dev->svid_index] & UFP_D_CAPABLE_BIT) && 
							 (mode & DFP_D_CAPABLE_BIT))
					{
						dev->displayport_matched_pin_assignments = UFP_D_PIN_ASSIGNMENTS(config->modes[dev->svid_index]) & remote_pin_assignments;
					}

					if (dev->displayport_matched_pin_assignments)
					{
						pe_set_state(dev, PE_DFP_VDM_MODE_ENTRY_REQUEST);
					}
				}
#endif
				// [VDM] Add other supported modes here...
			}
		}
		else if (cmd_type == VDM_RESP_BUSY)
		{
			// Retry after tVDMBusy.
			dev->vdm_pending_state = *dev->current_state;
			timer_start(&dev->timer, T_VDM_BUSY_MS, timeout_vdm_busy);
		}
	}

	return;
}

static void pe_dfp_vdm_mode_entry_request(usb_pd_port_t *dev)
{
	// If multiple modes are matched, we must implement logic on which one(s)
	// should be entered. For now assume only one match. - BQ
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	if (dev->matched_alt_modes & DP_ALT_MODE)
	{
		// Object position is 1.  No VDO for DP Alt Mode.
		vdm_send_structured_cmd(dev->port, VDM_DISPLAYPORT_VID, VDM_CMD_ENTER_MODE, 1, TCPC_TX_SOP);
	}
#endif
	// [VDM] Add other supported modes here...

	return;
}

static void pe_dfp_vdm_mode_exit_request(usb_pd_port_t *dev)
{
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	if (dev->active_alt_modes & DP_ALT_MODE)
	{
		// DFP_U shall only issue Exit Mode when the port is configured to be in USB Config.
		if (dev->displayport_config != CONFIG_FOR_USB)
		{
			dev->mode_exit_pending = true;

			// Set config to USB mode.
			dev->displayport_config = CONFIG_FOR_USB;
			pe_set_state(dev, PE_INIT_VDM_DP_CONFIG);
		}
		else
		{
			// Object position is 1.
			vdm_send_structured_cmd(dev->port, VDM_DISPLAYPORT_VID, VDM_CMD_EXIT_MODE, 1, TCPC_TX_SOP);
		}
	}
#endif
	// [VDM] Add other supported modes here...

	return;
}


static void pe_ufp_vdm_eval_mode_entry(usb_pd_port_t *dev)
{
	bool mode_entry_ack = false;
	uint32_t vdm_hdr = get_data_object(dev->rx_msg_buf);
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	// Object position must be 1 for DP Alt Mode.
	if ((config->svids[dev->svid_index] == VDM_DISPLAYPORT_VID) &&
		(USB_PD_VDM_HDR_GET_OBJ_POS(vdm_hdr) == 1))
	{
		// Reset HPD_IN queue.
		dev->hpd_in_queue_idx = 0;
		dev->hpd_in_send_idx = 0;
		dev->hpd_in_queue[0] = NOTIFY_HPD_NONE;

		dev->active_alt_modes |= DP_ALT_MODE;

		// Populate the VDM header Object position is always 1.
		dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_DISPLAYPORT_VID, VDM_TYPE_STRUCTURED, 1, VDM_RESP_ACK, VDM_CMD_ENTER_MODE); 

		// Set flag so ACK will be sent.
		mode_entry_ack = true;
	}
#endif
	// [VDM] Add other supported modes here...

	if (mode_entry_ack)
	{
		if (dev->timer2.function == timeout_alt_mode_entry)
		{
			// Cancel tAMETimeout.
			tusb422_lfo_timer_cancel(&dev->timer2);
		}
	
		// Send the ACK. Tx length is 1 for the VDM header.
		usb_pd_pe_tx_vendor_msg(dev->port, 1, (tcpc_transmit_t)dev->rx_sop);	
	}
	else /* NACK */
	{
		// Respond with NACK if not supported.
		pe_set_state(dev, PE_RESP_VDM_NAK);
	}

	return;
}

static void pe_ufp_vdm_mode_exit_entry(usb_pd_port_t *dev)
{
	bool mode_exit_ack = false;
	uint32_t vdm_hdr = get_data_object(dev->rx_msg_buf);
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t obj_pos;

	obj_pos = USB_PD_VDM_HDR_GET_OBJ_POS(vdm_hdr);

#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	if ((config->svids[dev->svid_index] == VDM_DISPLAYPORT_VID) && 
		(dev->active_alt_modes & DP_ALT_MODE))
	{
		// DisplayPort has 1 mode so object position must be 1 or 7 (all modes).
		if ((obj_pos == 1) || (obj_pos == 7))
		{
			// Configure pins for USB.
			tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);

			// Generate ACK.
			dev->vdm_hdr_vdos[0] = USB_PD_VDM_HDR_GEN(VDM_DISPLAYPORT_VID, 
													  VDM_TYPE_STRUCTURED, 
													  obj_pos, 
													  VDM_RESP_ACK, 
													  VDM_CMD_EXIT_MODE); 

			dev->active_alt_modes &= ~DP_ALT_MODE;

			mode_exit_ack = true;
		}
	}
#endif
	// [VDM] Add other supported modes here...

	if (mode_exit_ack)
	{
		// Tx length is 1 for the VDM header.
		usb_pd_pe_tx_vendor_msg(dev->port, 1, (tcpc_transmit_t)dev->rx_sop);
	}
	else
	{
		// Respond with NACK.
		pe_set_state(dev, PE_RESP_VDM_NAK);
	}

	return;
}

static void vdm_handle_enter_mode(usb_pd_port_t *dev, vdm_command_type_t cmd_type, uint8_t obj_pos)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

	if (cmd_type == VDM_INITIATOR)
	{
		// Only UFP and Cable Plug respond to Enter mode.
		if (dev->data_role == PD_DATA_ROLE_UFP)
		{
			pe_set_state(dev, PE_UFP_VDM_EVAL_MODE_ENTRY);
		}
	}
	else if (*dev->current_state == PE_DFP_VDM_MODE_ENTRY_REQUEST)
	{
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
		if (config->svids[dev->svid_index] == VDM_DISPLAYPORT_VID)
		{
			if (cmd_type == VDM_RESP_ACK)
			{
				// Configure pins for new mode.
				// For DP Alt-Mode, pins are configured when sending DP Config command so don't configure here.

				dev->active_alt_modes |= DP_ALT_MODE;

				pe_set_state(dev, PE_INIT_VDM_DP_STATUS_UPDATE);
			}
		}
#endif
		// [VDM] Add other supported SVIDs here...
	}

	return;
}

static void timeout_vdm_mode_exit(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
	else
	{
		pe_set_state(dev, PE_SRC_HARD_RESET);
	}

	return;
}


static void vdm_handle_exit_mode(usb_pd_port_t *dev, vdm_command_type_t cmd_type, uint8_t obj_pos)
{
	if (cmd_type == VDM_INITIATOR)
	{
		// Only UFP and Cable Plug respond to Exit mode.
		if (dev->data_role == PD_DATA_ROLE_UFP)
		{
			pe_set_state(dev, PE_UFP_VDM_MODE_EXIT);
		}
	}
	else if (*dev->current_state == PE_DFP_VDM_MODE_EXIT_REQUEST)
	{
		if (cmd_type == VDM_RESP_ACK)
		{
			// Configure pins for USB mode.
			tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);
		}
		else if (cmd_type == VDM_RESP_BUSY)
		{
			// Call timeout function to issue Hard Reset.
			timeout_vdm_mode_exit(dev->port);
		}
	}

	return;
}

static void vdm_handle_attention(usb_pd_port_t *dev, vdm_command_type_t cmd_type)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t num_vdos = (dev->rx_msg_data_len >> 2) - 1;	/* subtract 1 for the VDM header */
	uint8_t *vdo_ptr = dev->rx_msg_buf + 4;	/* add 4 to skip VDM header */
	uint32_t vdo;
	uint32_t connected_status;

	// Log dummy state change because there is no response for Attention command.
	pe_set_state(dev, PE_RCV_VDM_ATTENTION_REQUEST);
	dev->state_change = false;

	if (cmd_type != VDM_INITIATOR)
	{
		return;
	}

	// Attention can have zero or one VDOs.
	if (num_vdos)
	{
		vdo = get_data_object(vdo_ptr);
	}

#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	if ((config->svids[dev->svid_index] == VDM_DISPLAYPORT_VID) &&
		(dev->active_alt_modes &= DP_ALT_MODE))
	{
		// Only DFP_U accept attention from UFP_U and there should be 1 VDO.
		if ((dev->data_role == PD_DATA_ROLE_DFP) && num_vdos)
		{
			connected_status = vdo & (UFP_D_CONNECTED | DFP_D_CONNECTED);

			// Check for connection status change to connected.
			if ((connected_status != (dev->displayport_remote_status & (UFP_D_CONNECTED | DFP_D_CONNECTED))) &&
				connected_status)
			{
				dev->displayport_config = (connected_status & UFP_D_CONNECTED) ? CONFIG_UFP_U_AS_UFP_D : CONFIG_UFP_U_AS_DFP_D;

				pe_set_state(dev, PE_INIT_VDM_DP_CONFIG);
			}

			// Exit request trumps a connected status change.
			if (vdo & REQ_DP_MODE_EXIT)
			{
				pe_set_state(dev, PE_DFP_VDM_MODE_EXIT_REQUEST);
			}
			else if (vdo & REQ_USB_CONFIG)
			{
				// USB config request trumps a DP config due to connected status change.
				dev->displayport_config = CONFIG_FOR_USB;
				pe_set_state(dev, PE_INIT_VDM_DP_CONFIG);
			}

			// DP alt mode is configured, handle HPD.
			if (vdo & HPD_STATE_HIGH)
			{
				// Assert HPD to DFP_D.
				tcpm_hpd_out_control(dev->port, HPD_HIGH);
			}
			else
			{
				// Deassert HPD to DFP_D.
				tcpm_hpd_out_control(dev->port, HPD_LOW);
			}

			if (vdo & IRQ_HPD)
			{
				// Pulse HPD pin low for 1ms.
				tcpm_hpd_out_control(dev->port, HPD_IRQ);
			}	

			// Save the new status.
			dev->displayport_remote_status = vdo;
		}
	}
#endif
	// [VDM] Add other supported SVIDs here...

	return;
}


static void usb_pd_pe_structured_vdm_handler(usb_pd_port_t *dev)
{
	bool valid_svid = false;
	bool valid_cmd = true;
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint8_t svid_idx;
	uint32_t vdm_hdr = get_data_object(dev->rx_msg_buf);
	vdm_command_type_t cmd_type;
	uint8_t cmd;
	uint8_t svid_specific_cmd_limit;

	cmd = USB_PD_VDM_HDR_GET_CMD(vdm_hdr);

	// Stop any VDM timer running. (e.g. VDMResponseTimer, VDMModeEntryTimer, VDMModeExitTimer)
	timer_cancel(&dev->timer);

	if (!dev->explicit_contract && 
		!((cmd == VDM_CMD_DISCOVER_IDENTITY) &&
		  (dev->rx_sop == TCPC_TX_SOP_P)))
	{
		// Ignore if no explicit contract in place with the exception of 
		// Discovery Identity command sent to cable plug. 
		return;
	}

	// Validate SVID.
	if ((cmd == VDM_CMD_DISCOVER_IDENTITY) || (cmd == VDM_CMD_DISCOVER_SVIDS))
	{
		if (USB_PD_VDM_HDR_GET_SVID(vdm_hdr) == VDM_PD_SID)
		{
			valid_svid = true;
		}
	}
	else
	{
		svid_idx = find_svid_index(dev, USB_PD_VDM_HDR_GET_SVID(vdm_hdr));

		if (svid_idx < config->num_svids)
		{
			dev->svid_index = svid_idx;
			valid_svid = true;

			if (config->svids[dev->svid_index] == VDM_DISPLAYPORT_VID)
			{
				svid_specific_cmd_limit = VDM_CMD_DP_LIMIT;
			}
			else
			{
				svid_specific_cmd_limit = VDM_CMD_SVID_SPECIFIC_MIN;
			}

			if (!((cmd >= VDM_CMD_DISCOVER_MODES) && (cmd <= VDM_CMD_ATTENTION)) &&
				((cmd < VDM_CMD_SVID_SPECIFIC_MIN) || (cmd >= svid_specific_cmd_limit)))
			{
				valid_cmd = false;
			}
		}
		else
		{
			valid_cmd = false;
		}
	}

	if (valid_svid && valid_cmd)
	{
		cmd_type = (vdm_command_type_t)USB_PD_VDM_HDR_GET_CMD_TYPE(vdm_hdr);

		switch (cmd)
		{
			case VDM_CMD_DISCOVER_IDENTITY:
				vdm_handle_discover_identity(dev, cmd_type);
				break;

			case VDM_CMD_DISCOVER_SVIDS:
				vdm_handle_discover_svids(dev, cmd_type);
				break;

			case VDM_CMD_DISCOVER_MODES:
				vdm_handle_discover_modes(dev, cmd_type);
				break;

			case VDM_CMD_ENTER_MODE:
				vdm_handle_enter_mode(dev, cmd_type, USB_PD_VDM_HDR_GET_OBJ_POS(vdm_hdr));
				break;

			case VDM_CMD_EXIT_MODE:
				vdm_handle_exit_mode(dev, cmd_type, USB_PD_VDM_HDR_GET_OBJ_POS(vdm_hdr));
				break;

			case VDM_CMD_ATTENTION:
				vdm_handle_attention(dev, cmd_type);
				break;

			default:
				if (config->svids[dev->svid_index] == VDM_DISPLAYPORT_VID)
				{
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
					if (cmd == VDM_CMD_DP_STATUS_UPDATE)
					{
						vdm_handle_dp_status_update(dev, cmd_type);
					}
					else if (cmd == VDM_CMD_DP_CONFIG)
					{
						vdm_handle_dp_config(dev, cmd_type);
					}
#endif
				}

				break;
		}

		// Return to READY state if we haven't transitioned to new state.
		if (!dev->state_change)
		{
#ifdef CABLE_PLUG
			pe_set_state(dev, PE_CBL_READY);
#else
			if (dev->power_role == PD_PWR_ROLE_SNK)
			{
				pe_set_state(dev, PE_SNK_READY);
			}
			else
			{
				pe_set_state(dev, PE_SRC_READY);
			}
#endif
		}
	}
	else /* Invalid SVID or command */
	{
		// Attention command should never have a response.
		if (cmd != VDM_CMD_ATTENTION)
		{
			// Send NAK with original SVID, object position, and cmd if invalid SVID or invalid command.
			pe_set_state(dev, PE_RESP_VDM_NAK);
		}
	}

	return;
}

static void usb_pd_pe_unstructured_vdm_handler(usb_pd_port_t *dev)
{
	// Prior to establishing an Explicit Contract, Unstructured VDMs 
	// shall not be sent and shall be ignored if received.
	if (!dev->explicit_contract)
	{
		return;
	}

	// Only a DFP shall be an initiator.  
	// Only a UFP or cable plug shall be a responder.

	// Unstructured VDM are currently unsupported.
	// For PD r2.0, DFP or UFP should ignore the msg if not supported.
	// For PD r3.0, DFP or UFP should return Not_Supported msg if not supported.
#if (PD_SPEC_REV == PD_REV30) && !defined(CABLE_PLUG)
	if (!dev->state_change)
	{
		if (dev->power_role == PD_PWR_ROLE_SNK)
		{
			pe_set_state(dev, PE_SNK_SEND_NOT_SUPPORTED);
		}
		else
		{
			pe_set_state(dev, PE_SRC_SEND_NOT_SUPPORTED);
		}
	}
#endif

	return;
}


static void usb_pd_pe_vdm_handler(usb_pd_port_t *dev)
{
	uint32_t vdm_hdr = get_data_object(dev->rx_msg_buf);

	// All VDM message sequences involve a response for each command
	// so we can clear the vdm_in_progress flag here. 
	dev->vdm_in_progress = false;

	if (USB_PD_VDM_IS_STRUCT_TYPE(vdm_hdr))
	{
		usb_pd_pe_structured_vdm_handler(dev);
	}
	else /* Unstructured VDM */
	{
		usb_pd_pe_unstructured_vdm_handler(dev);
	}

	return;
}

static void timeout_vconn_stable(unsigned int port)
{
	pe_set_state(&pd[port], PE_SRC_VDM_IDENTITY_REQUEST);
	return;
}


static void timeout_vdm_mode_entry(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	// Restore USB operation. 

	// Notify DPM.

	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		pe_set_state(dev, PE_SNK_READY);
	}
	else
	{
		pe_set_state(dev, PE_SRC_READY);
	}

	return;
}

#endif /* ENABLE_VDM_SUPPORT */

#if !defined ENABLE_VDM_SUPPORT || !defined ENABLE_DP_ALT_MODE_SUPPORT
/* Dummy function to prevent compiler error when VDM or DP alt mode support disabled. */
void usb_pd_pe_hpd_in_event(unsigned int port, uint8_t val)
{
	return;
}
#endif

static void timeout_vbus_5v_stabilize(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if (*dev->current_state == PE_SRC_STARTUP)
	{
		pe_set_state(dev, PE_SRC_SEND_CAPS);
	}
	else if (*dev->current_state == PE_PRS_SOURCE_ON)
	{
		pe_set_state(dev, PE_PRS_SOURCE_ON_EXIT);
	}

	return;
}

static void timeout_swap_source_start(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

#ifdef ENABLE_VDM_SUPPORT
	if (!dev->cable_id_req_complete && tcpm_is_vconn_enabled(dev->port))
	{
		pe_set_state(dev, PE_SRC_VDM_IDENTITY_REQUEST);
	}
	else
#endif
	{
		pe_set_state(dev, PE_SRC_SEND_CAPS);
	}

	return;
}

static void timeout_typec_send_source_cap(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	INFO("CapsCnt = %u\n", dev->caps_cnt);

	if ((dev->caps_cnt > N_CAPS_COUNT) &&
		!dev->pd_connected)
	{
		pe_set_state(dev, PE_SRC_DISABLED);
	}
	else
	{
		pe_set_state(dev, PE_SRC_SEND_CAPS);
	}

	return;
}


static void timeout_sender_response(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	CRIT("%s\n", __func__);

	if ((*dev->current_state == PE_SRC_SEND_CAPS) ||
		(*dev->current_state == PE_SRC_SEND_SOFT_RESET))
	{
		pe_set_state(dev, PE_SRC_HARD_RESET);
	}
	else if (*dev->current_state == PE_SRC_GET_SINK_CAP)
	{
		pe_set_state(dev, PE_SRC_READY);
	}
	else if ((*dev->current_state == PE_SNK_SELECT_CAPABILITY) ||
			 (*dev->current_state == PE_SNK_SEND_SOFT_RESET))
	{
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
	else if ((*dev->current_state == PE_DRS_SEND_SWAP) || 
			 (*dev->current_state == PE_PRS_SEND_SWAP) ||
			 (*dev->current_state == PE_VCS_SEND_SWAP))
	{
		if (dev->power_role == PD_PWR_ROLE_SNK)
		{
			pe_set_state(dev, PE_SNK_READY);
		}
		else
		{
			pe_set_state(dev, PE_SRC_READY);
		}
	}
	else
	{
		CRIT("Error: %s - state %s unhandled!\n", __func__, pdstate2string[*dev->current_state]);
	}

	return;
}

static void timeout_ps_hard_reset(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if ((*dev->current_state == PE_SRC_HARD_RESET) ||
		(*dev->current_state == PE_SRC_HARD_RESET_RECEIVED))
	{
		pe_set_state(dev, PE_SRC_TRANSITION_TO_DEFAULT);
	}

	return;
}

static void pe_src_startup_entry(usb_pd_port_t *dev)
{
	DUAL_ROLE_CHANGED();

	usb_pd_prl_reset(dev->port);
	dev->non_interruptable_ams = false;
	dev->power_role_swap_in_progress = false;

	// Reset source caps count.
	dev->caps_cnt = 0;

	if (tcpm_is_vconn_enabled(dev->port))
	{
		// Enable PD receive for SOP/SOP'.
		tcpm_enable_pd_receive(dev->port, true, false);
	}
	else
	{
		// Enable PD receive for SOP.
		tcpm_enable_pd_receive(dev->port, false, false);
	}

	if (dev->swap_source_start)
	{
		// Wait tSwapSourceStart before transitioning to SRC_SEND_CAPS.
		timer_start(&dev->timer, T_SWAP_SOURCE_START_MS, timeout_swap_source_start);
		dev->swap_source_start = false;
	}
	else
	{
#ifdef ENABLE_VDM_SUPPORT
		if (!dev->cable_id_req_complete && tcpm_is_vconn_enabled(dev->port))
		{
			dev->send_src_cap_start_once = true;
			timer_start(&dev->timer, T_VCONN_STABLE_MS, timeout_vconn_stable);
		}
		else
#endif
		{
			// Wait for VBUS to stablize before transitioning to SRC_SEND_CAPS.
			timer_start(&dev->timer, T_VBUS_5V_STABLIZE_MS, timeout_vbus_5v_stabilize);
		}
	}

	return;
}

#ifdef ENABLE_VDM_SUPPORT

static void timeout_vdm_sender_response(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if (*dev->current_state == PE_SRC_VDM_IDENTITY_REQUEST)
	{
		pe_set_state(dev, PE_SRC_DISCOVERY);
	}
	else
	{
		if (*dev->current_state == PE_INIT_PORT_VDM_IDENTITY_REQUEST)
		{
			dev->port_partner_id_req_complete = true;
		}
		else
		{
			dev->vdm_pending = true;
			dev->vdm_pending_state = *dev->current_state;
		}

		if (dev->power_role == PD_PWR_ROLE_SNK)
		{
			pe_set_state(dev, PE_SNK_READY);
		}
		else
		{
			pe_set_state(dev, PE_SRC_READY);
		}
	}

	return;
}


static void pe_src_vdm_identity_request_entry(usb_pd_port_t *dev)
{
	dev->discover_identity_cnt++;

	// Send Discover Identity VDM to local cable plug.
	vdm_send_structured_cmd(dev->port, VDM_PD_SID, VDM_CMD_DISCOVER_IDENTITY, 0, TCPC_TX_SOP_P);

	return;
}

static void pe_init_port_vdm_identity_request_entry(usb_pd_port_t *dev)
{
	// Send Discover Identity VDM to port partner.
	vdm_send_structured_cmd(dev->port, VDM_PD_SID, VDM_CMD_DISCOVER_IDENTITY, 0, TCPC_TX_SOP);

	return;
}

#endif


#define VDM_SEND_CAP_MS  (T_TYPEC_SEND_SOURCE_CAP_MS - T_VCONN_STABLE_MS)

static void pe_src_discovery_entry(usb_pd_port_t *dev)
{
#ifdef ENABLE_VDM_SUPPORT
	if (tcpm_is_vconn_enabled(dev->port))
	{
		if (!dev->cable_id_req_complete &&
			(dev->discover_identity_cnt < N_DISCOVER_IDENTITY_COUNT))
		{
			if (dev->send_src_cap_start_once)
			{
				dev->send_src_cap_start_once = false;
				dev->send_src_cap_timeout = false;
				tusb422_lfo_timer_start(&dev->timer2, VDM_SEND_CAP_MS, timeout_send_src_cap);
			}

			if (!dev->send_src_cap_timeout)
			{
				pe_set_state(dev, PE_SRC_VDM_IDENTITY_REQUEST);
			}
		}

		if (!dev->state_change)
		{
			if (dev->caps_cnt == 0)
			{
				// Send SRC CAPS immediately.
				pe_set_state(dev, PE_SRC_SEND_CAPS);
			}
			else
			{
				// Start SourceCapabilityTimer.
				timer_start(&dev->timer, T_TYPEC_SEND_SOURCE_CAP_MS, timeout_typec_send_source_cap);
			}
		}
	}
	else
#endif
	{
		// Start SourceCapabilityTimer.
		timer_start(&dev->timer, T_TYPEC_SEND_SOURCE_CAP_MS, timeout_typec_send_source_cap);
	}

	return;
}

static void usb_pd_pe_handle_snk_src_default_state(usb_pd_port_t *dev)
{
	// Disable VCONN.
	tcpm_set_vconn_enable(dev->port, false);

	dev->pd_connected = false;
	dev->explicit_contract = false;
	dev->active_alt_modes = 0;

#ifdef ENABLE_VDM_SUPPORT
	dev->port_partner_id_req_complete = false;
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	//tcpm_hpd_out_control(dev->port, 0);
#endif
#endif

	return;
}

static void pe_src_transition_to_default_entry(usb_pd_port_t *dev)
{
	if (dev->active_alt_modes)
	{
		// Isolate any pins (including SBUs) previously configured for alt mode.
		tcpm_mux_control(dev->port, dev->data_role, MUX_DISABLE, dev->tcpc_dev->plug_polarity);
	}

	// Disable VCONN and set default flags.
	usb_pd_pe_handle_snk_src_default_state(dev);

	// Remove Rp from VCONN pin. (Also restores original Rp value)
	tcpm_remove_rp_from_vconn_pin(dev->port);

	// Disable VBUS.
	tcpm_src_vbus_disable(dev->port);

	// Force VBUS discharge.
	tcpm_force_discharge(dev->port, VSTOP_DISCHRG);

	// Wait until VBUS drops to vSafe0V before checking data role and starting source recover timer.
	tcpm_set_voltage_alarm_lo(dev->port, VSAFE0V_MAX);

	return;
}

static void pe_src_transition_to_default_exit(usb_pd_port_t *dev)
{
	// Configure pins for USB mode.
	tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);

	// Enable VCONN.
	tcpm_set_vconn_enable(dev->port, true);
	dev->vconn_source = true;

	// Restore vSafe5V.
	tcpm_source_vbus(dev->port, false, 5000);

	// After a Hard Reset, the sink must respond to SRC_CAPS within tNoResponse.
	dev->start_src_no_response_timer = true;

	// Wait for VBUS to reach vSafe5V before going to PE_SRC_STARTUP state.
	tcpm_set_voltage_alarm_hi(dev->port, VSAFE5V_MIN);

	return;
}

static void timeout_src_recover(unsigned int port)
{
	pe_src_transition_to_default_exit(&pd[port]);
	return;
}

static void pe_src_send_caps_entry(usb_pd_port_t *dev)
{
	if (dev->start_src_no_response_timer)
	{
		// Starting no response timer here instead of PE_SRC_TRANSITION_TO_DEFAULT 
		// so we can use the LFO timer during PE_SRC_VDM_IDENTITY_REQUEST.
		dev->start_src_no_response_timer = false;
		timer_start_no_response(dev);
	}

	// Increment source caps count.
	dev->caps_cnt++;

	// Send Source Capabilities.
	usb_pd_pe_tx_data_msg(dev->port, DATA_MSG_TYPE_SRC_CAPS, TCPC_TX_SOP);
	return;
}


static void pe_src_negotiate_capability_entry(usb_pd_port_t *dev)
{
	uint16_t operating_current;
	uint16_t src_max_current;
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
	uint32_t rdo = get_data_object(dev->rx_msg_buf);

	// BQ - Battery RDO not supported.

	dev->object_position = (rdo >> 28) & 0x07;
	operating_current = (rdo >> 10) & 0x3FF;

	if ((dev->object_position > 0) &&
		(dev->object_position <= config->num_src_pdos) &&
		(dev->object_position <= PD_MAX_PDO_NUM))
	{
		src_max_current = dev->src_pdo[dev->object_position - 1] & 0x3FF;

		DEBUG("PE_SRC_NEG_CAP: ObjPos = %u, req = %u mA, avail = %u mA\n",
			  dev->object_position, operating_current * 10, src_max_current * 10);

		if (operating_current <= src_max_current)
		{
			pe_set_state(dev, PE_SRC_TRANSITION_SUPPLY);
		}
		else
		{
			// Request cannot be met.
			pe_set_state(dev, PE_SRC_CAPABILITY_RESPONSE);
		}
	}
	else
	{
		DEBUG("PE_SRC_NEG_CAP: ObjPos = %u is invalid!\n", dev->object_position);

		// Request cannot be met.
		pe_set_state(dev, PE_SRC_CAPABILITY_RESPONSE);
	}

	return;
}


static void pe_src_transition_supply_entry(usb_pd_port_t *dev)
{
	if (dev->request_goto_min)
	{
		dev->request_goto_min = false;

		// Send GotoMin message.
		usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_GOTO_MIN, TCPC_TX_SOP);
	}
	else
	{
		// Send Accept message.
		usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_ACCEPT, TCPC_TX_SOP);
	}

	return;
}

static void pe_src_transition_supply_exit(usb_pd_port_t *dev)
{
	// Send PS_RDY.
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_PS_RDY, TCPC_TX_SOP);
	return;
}


static void pd_transition_power_supply(usb_pd_port_t *dev)
{
	uint16_t v_src_new_min;
	uint16_t v_src_new_max;
	uint16_t present_voltage;
	uint16_t requested_voltage;
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

		// Only 5V is supported so no voltage transition required.

	// Get present VBUS voltage (25 millivolt units).
	present_voltage = tcpm_get_vbus_voltage(dev->port);

	// Get requested VBUS voltage and convert to 25 millivolt units.
	requested_voltage = PDO_MIN_VOLTAGE(dev->src_pdo[dev->object_position - 1]) << 1;

	CRIT("VBUS is %u mV, %u mV requested\n", present_voltage * 25, requested_voltage * 25);

	// If fixed PDO, get min/max vSrcNew which is 0.95 and 1.05 times the target.
	if (config->src_caps[dev->object_position - 1].SupplyType == SUPPLY_TYPE_FIXED)
	{
		v_src_new_max = requested_voltage + ((requested_voltage * 5) / 100);
		v_src_new_min = requested_voltage - ((requested_voltage * 5) / 100);
	}
	else
	{
		v_src_new_max = PDO_MAX_VOLTAGE(dev->src_pdo[dev->object_position - 1]) << 1;
		v_src_new_min = requested_voltage;
	}

	INFO("v_src_new_max = %u mV\n", (v_src_new_max * 25));

	tcpm_source_vbus(dev->port, true, (requested_voltage * 25));
	// Check if negative voltage transition.
	if (v_src_new_max < present_voltage)
	{

		tcpm_set_voltage_alarm_lo(dev->port, v_src_new_max);

//		timer_start(&dev->timer, T_SRC_SETTLE_MS, timeout_src_settle);

		// Force VBUS discharge to new voltage level.
		tcpm_force_discharge(dev->port, v_src_new_max);
	}
	else
	{
		if (dev->object_position == 1)
		{
			// Default 5V. No voltage transition required.
			pe_set_state(dev, PE_SRC_TRANSITION_SUPPLY_EXIT);
		}
		else
		{

			// Use Hi voltage alarm to determine when power supply is ready. 
			tcpm_set_voltage_alarm_hi(dev->port, v_src_new_min);
		}
	}

	return;
}


static void timeout_src_transition(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if (*dev->current_state == PE_SRC_TRANSITION_SUPPLY)
	{
		pd_transition_power_supply(&pd[port]);
	}
	else if (*dev->current_state == PE_PRS_TRANSITION_TO_OFF)
	{
		// Disable source VBUS.
		tcpm_src_vbus_disable(dev->port);
	}
	return;
}


static void pe_src_capability_response_entry(usb_pd_port_t *dev)
{
	// Send Reject.
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_REJECT, TCPC_TX_SOP);

	// BQ - Send Wait not supported.
}

static void usb_pd_pe_handle_snk_src_ready_state(usb_pd_port_t *dev)
{
#ifdef ENABLE_VDM_SUPPORT
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);
#endif
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_notify_pd_state(dev->port, *dev->current_state);
#endif

	if (tcpm_is_vconn_enabled(dev->port))
	{
		// Enable PD receive for SOP/SOP'/SOP".
		tcpm_enable_pd_receive(dev->port, true, true);
	}

	if (dev->data_role == PD_DATA_ROLE_DFP)
	{
		if (tcpm_is_vconn_enabled(dev->port) && dev->cable_plug_comm_required)
		{
			// If this is a DFP which needs to establish communication with a Cable Plug,
			// then the Policy Engine shall initialize and run the DiscoverIdentityTimer. - BQ
			// timer_start(&dev->timer, T_DISCOVER_IDENTITY_MS, timeout_func);
		}
#ifdef ENABLE_VDM_SUPPORT
		else if (USB_PD_VDM_MODAL_OPERATION(config->id_header_vdo) &&
				 !dev->port_partner_id_req_complete)
		{
			dev->vdm_pending_state = PE_INIT_PORT_VDM_IDENTITY_REQUEST;
			timer_start(&dev->timer, 25, timeout_vdm_busy);
		}
		else if (dev->vdm_pending)
		{
			dev->vdm_pending = false;
			pe_set_state(dev, dev->vdm_pending_state);
		}
		else if (dev->mode_exit_pending)
		{
			dev->mode_exit_pending = false;
			pe_set_state(dev, PE_DFP_VDM_MODE_EXIT_REQUEST);
		}
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
		else if ((dev->active_alt_modes & DP_ALT_MODE) && 
				 (dev->hpd_in_queue[dev->hpd_in_send_idx] != NOTIFY_HPD_NONE))
		{
			usb_pd_pe_update_displayport_status(dev);

			// Send Status Update VDM to port partner.
			pe_set_state(dev, PE_INIT_VDM_DP_STATUS_UPDATE);
		}
#endif
#endif
	}
	else /* UFP */
	{
#ifdef ENABLE_VDM_SUPPORT
		if (dev->vdm_pending)
		{
			dev->vdm_pending = false;
			pe_set_state(dev, dev->vdm_pending_state);
		}
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
		else if ((dev->active_alt_modes & DP_ALT_MODE) && 
			(dev->hpd_in_queue[dev->hpd_in_send_idx] != NOTIFY_HPD_NONE))
		{
			pe_set_state(dev, PE_INIT_VDM_ATTENTION_REQUEST);
		}
#endif
#endif
	}

	return;
}

static void pe_src_ready_entry(usb_pd_port_t *dev)
{
	// Notify PRL of end of AMS.
	dev->non_interruptable_ams = false;

	dev->explicit_contract = true;

	// Set Rp value to 3.0A for collision avoidance.
	tcpm_set_rp_value(dev->port, RP_HIGH_CURRENT);

	usb_pd_pe_handle_snk_src_ready_state(dev);

	return;
}

static void pe_src_hard_reset_received_entry(usb_pd_port_t *dev)
{
	timer_start(&dev->timer, T_PS_HARD_RESET_MS, timeout_ps_hard_reset);
	return;
}

static void pe_src_hard_reset_entry(usb_pd_port_t *dev)
{
	dev->hard_reset_cnt++;
	tcpm_transmit(dev->port, NULL, TCPC_TX_HARD_RESET);
	timer_start(&dev->timer, T_PS_HARD_RESET_MS, timeout_ps_hard_reset);
	return;
}

static void pe_src_get_sink_cap_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_GET_SNK_CAP, TCPC_TX_SOP);
	return;
}

static void pe_src_disabled_entry(usb_pd_port_t *dev)
{
	tcpm_disable_pd_receive(dev->port);
	return;
}

static void pe_src_not_supported_received_entry(usb_pd_port_t *dev)
{
	// Inform policy manager.

	pe_set_state(dev, PE_SRC_READY);
	return;
}

/* can only be entered from PE_SRC_READY state */
static void pe_src_ping_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_PING, TCPC_TX_SOP);
	return;
}

static void pe_src_wait_new_caps_entry(usb_pd_port_t *dev)
{
	// Do nothing. Wait for policy manager to provide new caps.
}

static void pe_drs_send_swap_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_DR_SWAP, TCPC_TX_SOP);
	return;
}

static void pe_drs_evaluate_swap_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

	dev->non_interruptable_ams = true;

	if (dev->data_role == PD_DATA_ROLE_UFP)
	{
		if (config->auto_accept_swap_to_dfp)
		{
			pe_set_state(dev, PE_DRS_ACCEPT_SWAP);
		}
		else
		{
			// Ask platform policy manager or Reject.
			pe_set_state(dev, PE_DRS_REJECT_SWAP);
		}
	}
	else /* DFP */
	{
		if (config->auto_accept_swap_to_ufp)
		{
			pe_set_state(dev, PE_DRS_ACCEPT_SWAP);
		}
		else
		{
			// Ask platform policy manager or Reject.
			pe_set_state(dev, PE_DRS_REJECT_SWAP);
		}
	}

	if (*dev->current_state == PE_DRS_ACCEPT_SWAP)
	{
		// Update the message info here in case we receive a msg immediately after
		// transmitting the ACCEPT.
		if (dev->data_role == PD_DATA_ROLE_UFP)
		{
			tcpm_update_msg_header_info(dev->port, PD_DATA_ROLE_DFP, dev->power_role);
		}
		else /* DFP */
		{
			tcpm_update_msg_header_info(dev->port, PD_DATA_ROLE_UFP, dev->power_role);
		}
	}

	return;
}

static void pe_drs_change_role_entry(usb_pd_port_t *dev)
{
	// Update the data role.
	if (dev->data_role == PD_DATA_ROLE_UFP)
	{
		dev->data_role = PD_DATA_ROLE_DFP;
	}
	else /* DFP */
	{
		dev->data_role = PD_DATA_ROLE_UFP;
	}

	// Notify system of new data role.
#ifdef CONFIG_TUSB422_PAL
	usb_pd_pal_data_role_swap(dev->port, dev->data_role);
#endif

	DUAL_ROLE_CHANGED();

	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		pe_set_state(dev, PE_SNK_READY);
	}
	else /* SRC */
	{
		pe_set_state(dev, PE_SRC_READY);
	}

#ifdef ENABLE_VDM_SUPPORT
	// Data Role Swap shall reset the DiscoverIdentityCounter to zero.
	dev->discover_identity_cnt = 0;
#endif

	return;
}


static void pe_prs_send_swap_entry(usb_pd_port_t *dev)
{
	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		tcpm_snk_swap_standby(dev->port);
	}

	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_PR_SWAP, TCPC_TX_SOP);
	return;
}

static void pe_prs_evaluate_swap_entry(usb_pd_port_t *dev)
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

	dev->non_interruptable_ams = true;

	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		if (dev->tcpc_dev->silicon_revision == 0)
		{
			/*** TUSB422 PG1.0 cannot support SNK->SRC swap because PD will be disabled (CDDS #60) ***/
			pe_set_state(dev, PE_PRS_REJECT_SWAP);
		}
		else
		{
			if (config->auto_accept_swap_to_source)
			{
				pe_set_state(dev, PE_PRS_ACCEPT_SWAP);
			}
			else
			{
				// Ask platform policy manager or Reject.
				pe_set_state(dev, PE_PRS_REJECT_SWAP);
			}
		}

		if (*dev->current_state == PE_PRS_ACCEPT_SWAP)
		{
			tcpm_snk_swap_standby(dev->port);
		}
	}
	else /* SRC */
	{
		if (config->auto_accept_swap_to_sink)
		{
//			// Respond with Wait if we are currently externally powered.
//			if (dev->externally_powered)
//			{
//				pe_set_state(dev, PE_PRS_WAIT_SWAP);
//			}
//			else
//			{
			pe_set_state(dev, PE_PRS_ACCEPT_SWAP);
//			}
		}
		else
		{
			// Ask platform policy manager or Reject.
			pe_set_state(dev, PE_PRS_REJECT_SWAP);
		}
	}

	return;
}

static void timeout_ps_source(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	dev->power_role_swap_in_progress = false;
	pe_set_state(dev, PE_ERROR_RECOVERY);
	return;
}

static void pe_prs_transition_to_off_entry(usb_pd_port_t *dev)
{
	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		// Start PSSourceOff timer.
		timer_start(&dev->timer, T_PS_SOURCE_OFF_MS, timeout_ps_source); 

		// Disable sink VBUS.
		tcpm_snk_vbus_disable(dev->port);
	}
	else /* SRC */
	{
		// Accept msg was ACKed, transition from Attached.SRC to Attached.SNK according to Type-C spec.
		// This will prevent transistion to Unattached state when Rd-Rd condition is detected. 
		tcpm_handle_power_role_swap(dev->port);

		// Wait tSrcTransition before disabling VBUS.
		timer_start(&dev->timer, T_SRC_TRANSITION_MS, timeout_src_transition);

		// Wait until VBUS drops to vSafe0V before changing CC to Rd.
		tcpm_set_voltage_alarm_lo(dev->port, VSAFE0V_MAX);
	}

	dev->explicit_contract = false;

	return;
}

static void pe_prs_assert_rd_entry(usb_pd_port_t *dev)
{
	tcpm_set_autodischarge_disconnect(dev->port, false);
	// Rp -> Rd.
	tcpm_cc_pin_control(dev->port, ROLE_SNK);

	pe_set_state(dev, PE_PRS_WAIT_SOURCE_ON);

	return;
}

static void pe_prs_wait_source_on_entry(usb_pd_port_t *dev)
{
	// Set new power role to sink.
	dev->power_role = PD_PWR_ROLE_SNK;

	DUAL_ROLE_CHANGED();

	tcpm_update_msg_header_info(dev->port, dev->data_role, dev->power_role);

	// Send PS_RDY.
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_PS_RDY, TCPC_TX_SOP);

	// Start PSSourceOn timer.
	timer_start(&dev->timer, T_PS_SOURCE_ON_MS, timeout_ps_source); 

	return;
}

static void pe_prs_assert_rp_entry(usb_pd_port_t *dev)
{
	// PS_RDY was received from original source, transition from Attached.SNK 
	// to Attached.SRC according to Type-C spec.
	tcpm_handle_power_role_swap(dev->port);

	// Rd -> Rp.
	tcpm_cc_pin_control(dev->port, ROLE_SRC);

	pe_set_state(dev, PE_PRS_SOURCE_ON);

	return;
}

static void pe_prs_source_on_entry(usb_pd_port_t *dev)
{
	// Enable source VBUS vSafe5V.
	tcpm_source_vbus(dev->port, false, 5000);

	// Wait until VBUS rises to vSafe5V before sending PS_RDY.
	tcpm_set_voltage_alarm_hi(dev->port, VSAFE5V_MIN);

	return;
}

static void pe_prs_source_on_exit(usb_pd_port_t *dev)
{
	// Set new power role to source.
	dev->power_role = PD_PWR_ROLE_SRC;

	DUAL_ROLE_CHANGED();

	tcpm_update_msg_header_info(dev->port, dev->data_role, dev->power_role);
	tcpm_set_autodischarge_disconnect(dev->port, true);
	tcpm_set_bleed_discharge(dev->port, false);

	// Send PS_RDY.
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_PS_RDY, TCPC_TX_SOP);

	dev->swap_source_start = true;

	return;
}

/* After VCONN swap, VCONN Source must issue SOP' or SOP" Soft Reset to cable plug before communication */
static void pe_vcs_send_swap_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_VCONN_SWAP, TCPC_TX_SOP);
	return;
}

static void pe_vcs_evaluate_swap_entry(usb_pd_port_t *dev) 
{
	usb_pd_port_config_t *config = usb_pd_pm_get_config(dev->port);

	dev->non_interruptable_ams = true;

	if (config->auto_accept_vconn_swap)
	{
		// Current VCONN source must accept swap request.
		pe_set_state(dev, PE_VCS_ACCEPT_SWAP);
	}
	else
	{
		pe_set_state(dev, PE_VCS_REJECT_SWAP);
	}

	return;
}

static void timeout_vconn_source_on(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	// Hard reset based on CC state.
	if (dev->tcpc_dev->cc_status & CC_STATUS_CONNECT_RESULT)
	{
		// Rd asserted (Consumer/Provider)
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}
	else
	{
		// Rp asserted (Provider/Consumer)
		pe_set_state(dev, PE_SRC_HARD_RESET);
	}

	return;
}


static void pe_vcs_wait_for_vconn_entry(usb_pd_port_t *dev)
{
	// Start VCONNOn timer.
	timer_start(&dev->timer, T_VCONN_SOURCE_ON_MS, timeout_vconn_source_on); 
	return;
}

static void pe_vcs_turn_off_vconn_entry(usb_pd_port_t *dev)
{
	// Disable VCONN.
	tcpm_set_vconn_enable(dev->port, false);
	dev->vconn_source = false;

	DUAL_ROLE_CHANGED();

	// Disable PD receive for SOP'/SOP".
	tcpm_enable_pd_receive(dev->port, false, false);

	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		pe_set_state(dev, PE_SNK_READY);
	}
	else
	{
		pe_set_state(dev, PE_SRC_READY);
	}

	return;
}

static void timeout_vconn_enable(unsigned int port)
{
	pe_set_state(&pd[port], PE_VCS_SEND_PS_RDY);
	return;
}

static void pe_vcs_turn_on_vconn_entry(usb_pd_port_t *dev) 
{
	// Enable VCONN.
	tcpm_set_vconn_enable(dev->port, true);
	dev->vconn_source = true;

	DUAL_ROLE_CHANGED();

	// Short delay for VCONN stabilization before sending PS_RDY.
	timer_start(&dev->timer, 10, timeout_vconn_enable);

	return;
}

static void pe_vcs_send_ps_rdy_entry(usb_pd_port_t *dev)
{
	// Send PS_RDY.
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_PS_RDY, TCPC_TX_SOP);
	return;
}


static void timeout_sink_wait_cap(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	DEBUG("%s: HR cnt = %u.\n", __func__, dev->hard_reset_cnt);

	if (dev->hard_reset_cnt <= N_HARD_RESET_COUNT)
	{
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}

	return;
}

static void timeout_ps_transition(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if (dev->hard_reset_cnt <= N_HARD_RESET_COUNT)
	{
		pe_set_state(dev, PE_SNK_HARD_RESET);
	}

	return;
}

static void timeout_sink_request(unsigned int port)
{
	pe_set_state(&pd[port], PE_SNK_SELECT_CAPABILITY);
	return;
}

static void pe_snk_startup_entry(usb_pd_port_t *dev)
{
	DUAL_ROLE_CHANGED();

	usb_pd_prl_reset(dev->port);
	dev->non_interruptable_ams = false;
	dev->power_role_swap_in_progress = false;

	pe_set_state(dev, PE_SNK_DISCOVERY);

	return;
}

static void pe_snk_discovery_entry(usb_pd_port_t *dev)
{
	if (dev->no_response_timed_out &&
		dev->pd_connected_since_attach &&
		(dev->hard_reset_cnt > N_HARD_RESET_COUNT))
	{
		pe_set_state(dev, PE_ERROR_RECOVERY);
	}
	else
	{
		if (dev->vbus_present)
		{
			pe_set_state(dev, PE_SNK_WAIT_FOR_CAPS);
		}
	}

	return;
}

static void pe_snk_wait_for_caps_entry(usb_pd_port_t *dev)
{
	// VBUS = 5V.
	// Re-enable sink VBUS if it was previously disabled due to hard reset.
	if (dev->hard_reset_complete)
	{
		dev->hard_reset_complete = false;
		tcpm_sink_vbus(dev->port, false, 5000, GET_SRC_CURRENT_MA(dev->tcpc_dev->src_current_adv));
	}

	// Re-enable AutoDischargeDisconnect.
	// (May have been disabled due to hard reset or power role swap)
	tcpm_set_autodischarge_disconnect(dev->port, true);

	// Enable PD receive for SOP.
	tcpm_enable_pd_receive(dev->port, false, false);

	timer_start(&dev->timer, T_TYPEC_SINK_WAIT_CAP_MS, timeout_sink_wait_cap);

	return;
}

static void pe_snk_evaluate_capability_entry(usb_pd_port_t *dev)
{
	// Stop NoResponseTimer and reset HardResetCounter to zero.
	timer_cancel_no_response(dev);
	dev->hard_reset_cnt = 0;

	dev->non_interruptable_ams = true;

	if (dev->explicit_contract)
	{
		// Save current PDO in case request gets Reject or Wait response.
		dev->prev_selected_pdo = dev->selected_pdo;
	}

	// Ask policy manager to evaluate options based on supplied capabilities.
	usb_pd_pm_evaluate_src_caps(dev->port);

	pe_set_state(dev, PE_SNK_SELECT_CAPABILITY);

	return;
}

static void pe_snk_select_capability_entry(usb_pd_port_t *dev)
{
	uint16_t threshold_25mv;

	if (dev->explicit_contract)
	{
		if (PDO_MIN_VOLTAGE(dev->selected_pdo) < PDO_MIN_VOLTAGE(dev->prev_selected_pdo))
		{
			// Calculate threshold equal to 80% of new min voltage.
			threshold_25mv = (PDO_MIN_VOLTAGE(dev->selected_pdo) * 16) / 10;

			// Set new lower threshold to detect VBUS sink disconnect and
			// start discharge when AUTO_DISCHARGE_DISCONNECT is enabled.
			tcpm_set_sink_disconnect_threshold(dev->port, threshold_25mv);
		}
	}

	// Clear sink Wait and GotoMin flags.
	dev->snk_wait = false;
	dev->snk_goto_min = false;

	// Build RDO based on policy manager response.
	build_rdo(dev->port);

	// Send RDO.
	usb_pd_pe_tx_data_msg(dev->port, DATA_MSG_TYPE_REQUEST, TCPC_TX_SOP);

	return;
}

static void pe_snk_transition_sink_entry(usb_pd_port_t *dev)
{
	uint16_t rdo_operational_curr_or_pwr;

	timer_start(&dev->timer, T_PS_TRANSITION_MS, timeout_ps_transition);

	// Request policy manager to transition to new power level and wait for PS_RDY from source.
	if (dev->snk_goto_min)
	{
		rdo_operational_curr_or_pwr = RDO_MIN_OPERATIONAL_CURRENT_OR_POWER(dev->rdo);
	}
	else
	{
		rdo_operational_curr_or_pwr = RDO_OPERATIONAL_CURRENT_OR_POWER(dev->rdo);
	}

	if (PDO_SUPPLY_TYPE(dev->selected_pdo) == SUPPLY_TYPE_FIXED)
	{
		tcpm_sink_vbus(dev->port, true, 
					   PDO_VOLT_TO_MV(PDO_MIN_VOLTAGE(dev->selected_pdo)),
					   PDO_CURR_TO_MA(rdo_operational_curr_or_pwr));
	}
	else if (PDO_SUPPLY_TYPE(dev->selected_pdo) == SUPPLY_TYPE_VARIABLE)
	{
		tcpm_sink_vbus_vari(dev->port,
							PDO_VOLT_TO_MV(PDO_MIN_VOLTAGE(dev->selected_pdo)),
							PDO_VOLT_TO_MV(PDO_MAX_VOLTAGE(dev->selected_pdo)),
							PDO_CURR_TO_MA(rdo_operational_curr_or_pwr));
	}
	else /* Battery source */
	{
		tcpm_sink_vbus_batt(dev->port,
							PDO_VOLT_TO_MV(PDO_MIN_VOLTAGE(dev->selected_pdo)),
							PDO_VOLT_TO_MV(PDO_MAX_VOLTAGE(dev->selected_pdo)),
							PDO_PWR_TO_MW(rdo_operational_curr_or_pwr));
	}

	return;
}

#define DEFAULT_5V  (5000 / 25)  /* 25mV LSB */

static void pe_snk_ready_entry(usb_pd_port_t *dev)
{
	uint16_t min_voltage;
	uint16_t threshold;

	// Notify PRL of end of AMS.
	dev->non_interruptable_ams = false;

	dev->explicit_contract = true;

	if (dev->snk_wait)
	{
		// Start SinkRequest timer.
		timer_start(&dev->timer, T_SINK_REQUEST_MS, timeout_sink_request);
	}

	// Get PDO min voltage (multiply by 2 to convert to 25mV LSB) so we can set discharge voltage threshold. 
	min_voltage = PDO_MIN_VOLTAGE(dev->selected_pdo) << 1;

	if (min_voltage == DEFAULT_5V)
	{
		// Set Sink Disconnect Threshold to zero.
		threshold = 0;
	}
	else
	{
		// Set Sink Disconnect Threshold to 80% of min voltage.
		threshold = (min_voltage * 8) / 10;
		DEBUG("SNK VBUS Disconn Thres = %u mV.\n", threshold * 25);
	}

	// Set non-zero value to specify VBUS threshold for sink disconnect detection.
	// Otherwise, the default threshold (3.5V per TCPC spec) for VBUS present will be used.
	tcpm_set_sink_disconnect_threshold(dev->port, threshold);

	usb_pd_pe_handle_snk_src_ready_state(dev);

	return;
}

static void pe_snk_hard_reset_entry(usb_pd_port_t *dev)
{
	dev->hard_reset_cnt++;
	tcpm_transmit(dev->port, NULL, TCPC_TX_HARD_RESET);

	return;
}

static void pe_snk_not_supported_received_entry(usb_pd_port_t *dev)
{
	// Notify policy manager.

	pe_set_state(dev, PE_SNK_READY);
	return;
}

static void pe_snk_transition_to_default_entry(usb_pd_port_t *dev)
{
	if (dev->active_alt_modes)
	{
		// Isolate any pins (including SBUs) previously configured for alt mode.
		// Pins will be configured for USB after VBUS reaches vSafe0V.
		tcpm_mux_control(dev->port, dev->data_role, MUX_DISABLE, dev->tcpc_dev->plug_polarity);
	}

	usb_pd_pe_handle_snk_src_default_state(dev);

	// Disable AutoDischargeDisconnect and VBUS sink.
	tcpm_snk_swap_standby(dev->port);

	// Request policy manager to set data role to UFP.
	if (dev->data_role != PD_DATA_ROLE_UFP)
	{
		dev->data_role = PD_DATA_ROLE_UFP;

		tcpm_update_msg_header_info(dev->port, dev->data_role, dev->power_role);

#ifdef CONFIG_TUSB422_PAL
		// BQ - should we notify in PE_SNK_Wait_for_Capabilities when VBUS is present instead? 
		usb_pd_pal_notify_connect_state(dev->port, dev->tcpc_dev->state, dev->tcpc_dev->plug_polarity);
//        usb_pd_pal_data_role_swap(dev->port, PD_DATA_ROLE_UFP);
#endif
	}

	// After a Hard Reset, the sink must receive SRC_CAPS within tNoResponse.
	timer_start_no_response(dev);

	// During a Hard Reset the Source voltage will transition to vSafe0V and then 
	// transition to vSafe5V. Sinks need to ensure that VBUS present is not indicated 
	// until after the Source has completed the Hard Reset process by detecting both of these transitions.
	dev->vbus_present = false;
	tcpm_set_voltage_alarm_lo(dev->port, VSAFE0V_MAX);

	// Set flag to re-enable sink VBUS in PE_SNK_WAIT_FOR_CAPS.
	dev->hard_reset_complete = true;

	// Assume sink has reached default level and transition to SNK_STARTUP.
	pe_set_state(dev, PE_SNK_STARTUP);

	return;
}

static void pe_snk_give_sink_cap_entry(usb_pd_port_t *dev)
{
	// Request Sink Caps from policy manager.

	usb_pd_pe_tx_data_msg(dev->port, DATA_MSG_TYPE_SNK_CAPS, TCPC_TX_SOP);
	return;
}

static void pe_snk_get_source_cap_entry(usb_pd_port_t *dev)
{
	usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_GET_SRC_CAP, TCPC_TX_SOP);
	return;
}

static void timeout_bist_cont_mode(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

	if (dev->power_role == PD_PWR_ROLE_SNK)
	{
		pe_set_state(dev, PE_SNK_TRANSITION_TO_DEFAULT);
	}
	else
	{
		pe_set_state(dev, PE_SRC_TRANSITION_TO_DEFAULT);
	}

	return;
}


static void pe_bist_carrier_mode_entry(usb_pd_port_t *dev)
{
	// Start BIST transmit.  TCPC HW will automatically stop transmit after tBISTContMode.
	tcpm_transmit(dev->port, NULL, TCPC_TX_BIST_MODE2);

	timer_start(&dev->timer, T_BIST_CONT_MODE_MS, timeout_bist_cont_mode);

	return;
}

static void pe_bist_test_mode_entry(usb_pd_port_t *dev)
{
	return;
}

static void pe_error_recovery_entry(usb_pd_port_t *dev)
{
	tcpm_execute_error_recovery(dev->port);
	return;
}

static void pe_dr_src_give_sink_caps_entry(usb_pd_port_t *dev)
{
	if (dev->tcpc_dev->role == ROLE_DRP)
	{
		usb_pd_pe_tx_data_msg(dev->port, DATA_MSG_TYPE_SNK_CAPS, TCPC_TX_SOP);
	}
	else
	{
#if (PD_SPEC_REV == PD_REV20)
		usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_REJECT, TCPC_TX_SOP);
#else
		usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_NOT_SUPPORTED, TCPC_TX_SOP);
#endif
	}

	return;
}


static void pe_dr_snk_give_source_caps_entry(usb_pd_port_t *dev)
{
	if (dev->tcpc_dev->role == ROLE_DRP)
	{
		usb_pd_pe_tx_data_msg(dev->port, DATA_MSG_TYPE_SRC_CAPS, TCPC_TX_SOP);
	}
	else
	{
#if (PD_SPEC_REV == PD_REV20)
		usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_REJECT, TCPC_TX_SOP);
#else
		usb_pd_prl_tx_ctrl_msg(dev->port, buf, CTRL_MSG_TYPE_NOT_SUPPORTED, TCPC_TX_SOP);
#endif
	}

	return;
}

static void pe_dummy_state_entry(usb_pd_port_t *dev)
{
	INFO("%s\n", __func__);
	return;
}

typedef void (*state_entry_fptr)(usb_pd_port_t *dev);

static const state_entry_fptr pe_state_entry[PE_NUM_STATES] =
{
	pe_dummy_state_entry,                 /* PE_UNATTACHED                 */
	pe_src_startup_entry,                 /* PE_SRC_STARTUP                */
	pe_src_discovery_entry,               /* PE_SRC_DISCOVERY              */
	pe_src_send_caps_entry,               /* PE_SRC_SEND_CAPS              */
	pe_src_negotiate_capability_entry,    /* PE_SRC_NEGOTIATE_CAPABILITY   */
	pe_src_transition_supply_entry,       /* PE_SRC_TRANSITION_SUPPLY      */
	pe_src_transition_supply_exit,        /* PE_SRC_TRANSITION_SUPPLY_EXIT */
	pe_src_ready_entry,                   /* PE_SRC_READY                  */
	pe_src_disabled_entry,                /* PE_SRC_DISABLED               */
	pe_src_capability_response_entry,     /* PE_SRC_CAPABILITY_RESPONSE    */
	pe_src_hard_reset_entry,              /* PE_SRC_HARD_RESET             */
	pe_src_hard_reset_received_entry,     /* PE_SRC_HARD_RESET_RECEIVED    */
	pe_src_transition_to_default_entry,   /* PE_SRC_TRANSITION_TO_DEFAULT  */
	pe_src_get_sink_cap_entry,            /* PE_SRC_GET_SINK_CAP           */
	pe_src_wait_new_caps_entry,           /* PE_SRC_WAIT_NEW_CAPS          */
	pe_send_soft_reset_entry,             /* PE_SRC_SEND_SOFT_RESET        */
	pe_send_accept_entry,                 /* PE_SRC_SOFT_RESET             */
	pe_send_not_supported_entry,          /* PE_SRC_SEND_NOT_SUPPORTED     */
	pe_src_not_supported_received_entry,  /* PE_SRC_NOT_SUPPORTED_RECEIVED */
	pe_src_ping_entry,                    /* PE_SRC_PING                   */
	pe_dummy_state_entry,                 /* PE_SRC_SEND_SOURCE_ALERT      */
	pe_dummy_state_entry,                 /* PE_SRC_SINK_ALERT_RECEIVED    */
	pe_dummy_state_entry,                 /* PE_SRC_GIVE_SOURCE_CAP_EXT    */
	pe_dummy_state_entry,                 /* PE_SRC_GIVE_SOURCE_STATUS     */
	pe_dummy_state_entry,                 /* PE_SRC_GET_SINK_STATUS        */

	pe_snk_startup_entry,                 /* PE_SNK_STARTUP                */
	pe_snk_discovery_entry,               /* PE_SNK_DISCOVERY              */
	pe_snk_wait_for_caps_entry,           /* PE_SNK_WAIT_FOR_CAPS          */
	pe_snk_evaluate_capability_entry,     /* PE_SNK_EVALUATE_CAPABILITY    */
	pe_snk_select_capability_entry,       /* PE_SNK_SELECT_CAPABILITY      */
	pe_snk_transition_sink_entry,         /* PE_SNK_TRANSITION_SINK        */
	pe_snk_ready_entry,                   /* PE_SNK_READY                  */
	pe_snk_hard_reset_entry,              /* PE_SNK_HARD_RESET             */
	pe_snk_transition_to_default_entry,   /* PE_SNK_TRANSITION_TO_DEFAULT  */
	pe_snk_give_sink_cap_entry,           /* PE_SNK_GIVE_SINK_CAP          */
	pe_snk_get_source_cap_entry,          /* PE_SNK_GET_SOURCE_CAP         */
	pe_send_soft_reset_entry,             /* PE_SNK_SEND_SOFT_RESET        */
	pe_send_accept_entry,                 /* PE_SNK_SOFT_RESET             */
	pe_send_not_supported_entry,          /* PE_SNK_SEND_NOT_SUPPORTED     */
	pe_snk_not_supported_received_entry,  /* PE_SNK_NOT_SUPPORTED_RECEIVED */
	pe_dummy_state_entry,                 /* PE_SNK_SOURCE_ALERT_RECEIVED  */
	pe_dummy_state_entry,                 /* PE_SNK_SEND_SINK_ALERT        */
	pe_dummy_state_entry,                 /* PE_SNK_GET_SOURCE_CAP_EXT     */
	pe_dummy_state_entry,                 /* PE_SNK_GET_SOURCE_STATUS      */
	pe_dummy_state_entry,                 /* PE_SNK_GIVE_SINK_STATUS       */

	pe_dr_src_give_sink_caps_entry,       /* PE_DR_SRC_GIVE_SINK_CAP       */
	pe_dr_snk_give_source_caps_entry,     /* PE_DR_SNK_GIVE_SOURCE_CAP     */

	pe_bist_carrier_mode_entry,           /* PE_BIST_CARRIER_MODE          */
	pe_bist_test_mode_entry,              /* PE_BIST_TEST_MODE             */

	pe_error_recovery_entry,              /* PE_ERROR_RECOVERY             */

	pe_drs_send_swap_entry,               /* PE_DRS_SEND_SWAP              */
	pe_drs_evaluate_swap_entry,           /* PE_DRS_EVALUATE_SWAP          */
	pe_send_reject_entry,                 /* PE_DRS_REJECT_SWAP            */
	pe_send_accept_entry,                 /* PE_DRS_ACCEPT_SWAP            */
	pe_drs_change_role_entry,             /* PE_DRS_CHANGE_ROLE            */

	pe_prs_send_swap_entry,               /* PE_PRS_SEND_SWAP              */
	pe_prs_evaluate_swap_entry,           /* PE_PRS_EVALUATE_SWAP          */
	pe_send_reject_entry,                 /* PE_PRS_REJECT_SWAP            */
	pe_send_wait_entry,                   /* PE_PRS_WAIT_SWAP              */
	pe_send_accept_entry,                 /* PE_PRS_ACCEPT_SWAP            */
	pe_prs_transition_to_off_entry,       /* PE_PRS_TRANSITION_TO_OFF      */
	pe_prs_assert_rd_entry,               /* PE_PRS_ASSERT_RD              */
	pe_prs_wait_source_on_entry,          /* PE_PRS_WAIT_SOURCE_ON         */
	pe_prs_assert_rp_entry,               /* PE_PRS_ASSERT_RP              */
	pe_prs_source_on_entry,               /* PE_PRS_SOURCE_ON              */
	pe_prs_source_on_exit,                /* PE_PRS_SOURCE_ON_EXIT         */

	pe_vcs_send_swap_entry,               /* PE_VCS_SEND_SWAP              */
	pe_vcs_evaluate_swap_entry,           /* PE_VCS_EVALUATE_SWAP          */
	pe_send_reject_entry,                 /* PE_VCS_REJECT_SWAP            */
	pe_send_accept_entry,                 /* PE_VCS_ACCEPT_SWAP            */
	pe_vcs_wait_for_vconn_entry,          /* PE_VCS_WAIT_FOR_VCONN         */
	pe_vcs_turn_off_vconn_entry,          /* PE_VCS_TURN_OFF_VCONN         */
	pe_vcs_turn_on_vconn_entry,           /* PE_VCS_TURN_ON_VCONN          */
	pe_vcs_send_ps_rdy_entry,             /* PE_VCS_SEND_PS_RDY            */

#ifdef ENABLE_VDM_SUPPORT
	pe_src_vdm_identity_request_entry,    /* PE_SRC_VDM_IDENTITY_REQUEST   */  
	pe_init_port_vdm_identity_request_entry,  /* PE_INIT_PORT_VDM_IDENTITY_REQUEST  */
	pe_init_vdm_svids_request_entry,      /* PE_INIT_VDM_SVIDS_REQUEST     */
	pe_init_vdm_modes_request_entry,      /* PE_INIT_VDM_MODES_REQUEST     */
	pe_init_vdm_attention_request_entry,  /* PE_INIT_VDM_ATTENTION_REQUEST */
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	pe_init_vdm_dp_status_update_entry,   /* PE_INIT_VDM_DP_STATUS_UPDATE  */
	pe_init_vdm_dp_config_entry,          /* PE_INIT_VDM_DP_CONFIG         */
#endif

	pe_resp_vdm_nak_entry,                /* PE_RESP_VDM_NAK               */
	pe_resp_vdm_get_identity_entry,       /* PE_RESP_VDM_GET_IDENTITY      */
	pe_resp_vdm_get_svids_entry,          /* PE_RESP_VDM_GET_SVIDS         */
	pe_resp_vdm_get_modes_entry,          /* PE_RESP_VDM_GET_MODES         */
	pe_dummy_state_entry,                 /* PE_RCV_VDM_ATTENTION_REQUEST  */
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
	pe_resp_vdm_dp_status_update_entry,   /* PE_RESP_VDM_DP_STATUS_UPDATE  */
	pe_resp_vdm_dp_config_entry,          /* PE_RESP_VDM_DP_CONFIG         */
#endif

	pe_dfp_vdm_mode_entry_request,        /* PE_DFP_VDM_MODE_ENTRY_REQUEST */
	pe_dfp_vdm_mode_exit_request,         /* PE_DFP_VDM_MODE_EXIT_REQUEST  */

	pe_ufp_vdm_eval_mode_entry,           /* PE_UFP_VDM_EVAL_MODE_ENTRY    */
	pe_ufp_vdm_mode_exit_entry,           /* PE_UFP_VDM_MODE_EXIT          */
#endif
};


void usb_pd_pe_state_machine(unsigned int port)
{
	usb_pd_port_t *dev = &pd[port];

#if defined ENABLE_VDM_SUPPORT && defined ENABLE_DP_ALT_MODE_SUPPORT
	if ((dev->active_alt_modes & DP_ALT_MODE) && dev->hpd_in_changed)
	{
		dev->hpd_in_changed = false;

		if (dev->hpd_in_value == 0)
		{
			// Start timer to see if this is an HPD IRQ.
			tusb422_lfo_timer_start(&dev->timer2, T_IRQ_HPD_MAX_MS, timeout_irq_hpd);
		}
		else /* HPD high */
		{
			if (dev->timer2.function == timeout_irq_hpd)
			{
				// Timer did not expire. IRQ_HPD occured.
				tusb422_lfo_timer_cancel(&dev->timer2);
				dev->hpd_in_queue[dev->hpd_in_queue_idx++] = NOTIFY_HPD_IRQ; 
			}
			else
			{
				dev->hpd_in_queue[dev->hpd_in_queue_idx++] = NOTIFY_HPD_HIGH; 
			}

			if (dev->hpd_in_queue_idx >= HPD_IN_QUEUE_SIZE)
			{
				dev->hpd_in_queue_idx = 0;
			}

				dev->attention_pending = true;
		}
	}
#endif

	while (dev->state_change)
	{
		dev->state_change = false;

		// Use branch table to execute "actions on entry or exit" for the current state.
		if (*dev->current_state < PE_NUM_STATES)
		{
			pe_state_entry[*dev->current_state](dev);
		}
	}

#ifdef ENABLE_VDM_SUPPORT
	if (dev->attention_pending && dev->active_alt_modes &&
		((*dev->current_state == PE_SNK_READY) ||
		 (*dev->current_state == PE_SRC_READY)))
	{
		dev->attention_pending = false;
		pe_set_state(dev, PE_INIT_VDM_ATTENTION_REQUEST);
		dev->state_change = false;
		pe_state_entry[*dev->current_state](dev);
	}
#endif
	return;
}


void usb_pd_pe_notify(unsigned int port, usb_pd_prl_alert_t prl_alert)
{
	usb_pd_port_t *dev = &pd[port];

	switch (prl_alert)
	{
		case PRL_ALERT_HARD_RESET_RECEIVED:
			// PD message passing is enabled in protocol layer.

			if (dev->power_role == PD_PWR_ROLE_SNK)
			{
				// Stop sender response timer.
				timer_cancel(&dev->timer);

				pe_set_state(dev, PE_SNK_TRANSITION_TO_DEFAULT);
			}
			else /* SRC */
			{
				pe_set_state(dev, PE_SRC_HARD_RESET_RECEIVED);
			}
			break;

		case PRL_ALERT_MSG_TX_SUCCESS:	 /* GoodCRC received */
			switch (*dev->current_state)
			{
				case PE_SRC_SEND_CAPS:
					dev->caps_cnt = 0;
					dev->hard_reset_cnt = 0;
					timer_cancel_no_response(dev);
					dev->non_interruptable_ams = true;
					dev->pd_connected = true;
					dev->pd_connected_since_attach = true;
					timer_start(&dev->timer, T_SENDER_RESPONSE_MS, timeout_sender_response);
					break;

				case PE_SRC_HARD_RESET:
					// Do nothing.  Wait for PSHardReset timer to expire to transition to default.
					break;

				case PE_SNK_HARD_RESET:
					pe_set_state(dev, PE_SNK_TRANSITION_TO_DEFAULT);
					break;

				case PE_SRC_GET_SINK_CAP:
				case PE_DRS_SEND_SWAP:
				case PE_PRS_SEND_SWAP:
				case PE_VCS_SEND_SWAP:
				case PE_SNK_SEND_SOFT_RESET:
				case PE_SRC_SEND_SOFT_RESET:
				case PE_SNK_SELECT_CAPABILITY:
					dev->non_interruptable_ams = true;
					timer_start(&dev->timer, T_SENDER_RESPONSE_MS, timeout_sender_response);
					break;

				case PE_SNK_SOFT_RESET:
					// Accept msg was successfully sent.
					dev->non_interruptable_ams = false;
					pe_set_state(dev, PE_SNK_WAIT_FOR_CAPS);
					break;

				case PE_SRC_SOFT_RESET:
					// Accept msg was successfully sent.
					dev->non_interruptable_ams = false;
					pe_set_state(dev, PE_SRC_SEND_CAPS);
					break;

				case PE_SRC_TRANSITION_SUPPLY:
					timer_start(&dev->timer, T_SRC_TRANSITION_MS, timeout_src_transition);
					break;

				case PE_SRC_CAPABILITY_RESPONSE:
					if (dev->explicit_contract /* || send_wait*/)
					{
						// Reject and Current contract is still valid or wait sent.
						pe_set_state(dev, PE_SRC_READY);

						// If explicit contract and current contract is invalid go to hard reset. - BQ
					}
					else /* No explicit contract */
					{
						pe_set_state(dev, PE_SRC_WAIT_NEW_CAPS);
					}
					break;

				case PE_SRC_TRANSITION_SUPPLY_EXIT:
				case PE_DR_SRC_GIVE_SINK_CAP:
				case PE_SRC_PING:
				case PE_SRC_SEND_NOT_SUPPORTED:
					pe_set_state(dev, PE_SRC_READY);
					break;

				case PE_SNK_GET_SOURCE_CAP:
				case PE_SNK_GIVE_SINK_CAP:
				case PE_DR_SNK_GIVE_SOURCE_CAP:
				case PE_SNK_SEND_NOT_SUPPORTED:
					pe_set_state(dev, PE_SNK_READY);
					break;

				case PE_PRS_SOURCE_ON_EXIT:
					pe_set_state(dev, PE_SRC_STARTUP);
					break;

				case PE_DRS_ACCEPT_SWAP:
					pe_set_state(dev, PE_DRS_CHANGE_ROLE);
					break;

				case PE_PRS_ACCEPT_SWAP:
					dev->power_role_swap_in_progress = true;
					pe_set_state(dev, PE_PRS_TRANSITION_TO_OFF);
					break;

				case PE_VCS_ACCEPT_SWAP:
					if (dev->vconn_source)
					{
						pe_set_state(dev, PE_VCS_WAIT_FOR_VCONN);
					}
					else
					{
						pe_set_state(dev, PE_VCS_TURN_ON_VCONN);
					}
					break;

#ifdef ENABLE_VDM_SUPPORT
				case PE_SRC_VDM_IDENTITY_REQUEST:
				case PE_INIT_PORT_VDM_IDENTITY_REQUEST:
				case PE_INIT_VDM_SVIDS_REQUEST:
				case PE_INIT_VDM_MODES_REQUEST:
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
				case PE_INIT_VDM_DP_STATUS_UPDATE:
				case PE_INIT_VDM_DP_CONFIG:
#endif
					// Start VDMResponseTimer.
					timer_start(&dev->timer, T_VDM_SENDER_RESPONSE_MS, timeout_vdm_sender_response);
					break;

				case PE_DFP_VDM_MODE_ENTRY_REQUEST:
					// Start VDMModeEntryTimer.
					timer_start(&dev->timer, T_VDM_WAIT_MODE_ENTRY_MS, timeout_vdm_mode_entry);
					break;

				case PE_DFP_VDM_MODE_EXIT_REQUEST:
					// Start VDMModeExitTimer.
					timer_start(&dev->timer, T_VDM_WAIT_MODE_EXIT_MS, timeout_vdm_mode_exit);
					break;

				case PE_INIT_VDM_ATTENTION_REQUEST:
#ifdef ENABLE_DP_ALT_MODE_SUPPORT
					// IRQ_HPD is has been sent, clear the status. 
					dev->displayport_status &= ~IRQ_HPD;
					// Fall through...
				case PE_RESP_VDM_DP_STATUS_UPDATE:
				case PE_RESP_VDM_DP_CONFIG:
#endif
				case PE_RESP_VDM_GET_IDENTITY:
				case PE_RESP_VDM_GET_SVIDS:
				case PE_RESP_VDM_GET_MODES:
				case PE_RESP_VDM_NAK:
				case PE_UFP_VDM_EVAL_MODE_ENTRY: /* for ACK Resp */
				case PE_UFP_VDM_MODE_EXIT:       /* for ACK Resp */
					dev->vdm_in_progress = false;
					// Fall through...
#endif /* ENABLE_VDM_SUPPORT */
				case PE_PRS_REJECT_SWAP:
				case PE_PRS_WAIT_SWAP:
				case PE_DRS_REJECT_SWAP:
				case PE_VCS_REJECT_SWAP:
				case PE_VCS_SEND_PS_RDY:
					if (dev->power_role == PD_PWR_ROLE_SNK)
					{
						pe_set_state(dev, PE_SNK_READY);
					}
					else
					{
						pe_set_state(dev, PE_SRC_READY);
					}
					break;

				default:
					break;
			}
			break;

		case PRL_ALERT_MSG_TX_DISCARDED: /* Msg was received or Hard Reset issued before Tx */
			// Do nothing. Proceed to handle the received message or Hard Reset.
			break;

		case PRL_ALERT_MSG_TX_FAILED: /* No GoodCRC response */
#ifdef ENABLE_VDM_SUPPORT
			dev->vdm_in_progress = false;
#endif
			if (*dev->current_state == PE_PRS_WAIT_SOURCE_ON)
			{
				// Cancel PSSourceOn timer.
				timer_cancel(&dev->timer);

				// PS_RDY msg failed.
				pe_set_state(dev, PE_ERROR_RECOVERY);
			}
			else if (*dev->current_state == PE_PRS_SOURCE_ON_EXIT)
			{
				// PS_RDY msg failed.
				pe_set_state(dev, PE_ERROR_RECOVERY);
			}
			else if (dev->power_role == PD_PWR_ROLE_SNK)
			{
				if (*dev->current_state == PE_SNK_SEND_SOFT_RESET)
				{
					// Any failure in the Soft Reset process will trigger a Hard Reset.
					pe_set_state(dev, PE_SNK_HARD_RESET);
				}
				else if (*dev->current_state != PE_UNATTACHED)
				{
					// Failure to see a GoodCRC when a port pair 
					// is connected will result in a Soft Reset.
					pe_set_state(dev, PE_SNK_SEND_SOFT_RESET);
				}
			}
			else /* SRC */
			{
				if (*dev->current_state == PE_SRC_SEND_SOFT_RESET)
				{
					// Any failure in the Soft Reset process will trigger a Hard Reset.
					pe_set_state(dev, PE_SRC_HARD_RESET);
				}
				else if ((*dev->current_state == PE_SRC_SEND_CAPS) &&
						 (!dev->pd_connected))
				{
					pe_set_state(dev, PE_SRC_DISCOVERY);
				}
#ifdef ENABLE_VDM_SUPPORT
				else if (*dev->current_state == PE_SRC_VDM_IDENTITY_REQUEST)
				{
					// VDM Discover Identity msg failed.
					pe_set_state(dev, PE_SRC_DISCOVERY);
				}
#endif
				else if (*dev->current_state != PE_UNATTACHED)
				{
					// Failure to see a GoodCRC when a port pair 
					// is connected will result in a Soft Reset.
					pe_set_state(dev, PE_SRC_SEND_SOFT_RESET);
				}
			}
			break;

		case PRL_ALERT_MSG_RECEIVED:
			if (dev->rx_msg_data_len)
			{
				usb_pd_pe_data_msg_rx_handler(dev);
			}
			else
			{
				usb_pd_pe_ctrl_msg_rx_handler(dev);
			}
			break;

		default:
			break;
	}

	return;
}


int usb_pd_policy_manager_request(unsigned int port, pd_policy_manager_request_t req)
{
	usb_pd_port_t *dev = &pd[port];
	usb_pd_pe_status_t status = STATUS_OK;

	if (*dev->current_state == PE_SRC_READY)
	{
		if (req == PD_POLICY_MNGR_REQ_GET_SINK_CAPS)
		{
			pe_set_state(dev, PE_SRC_GET_SINK_CAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_SRC_CAPS_CHANGE)
		{
			pe_set_state(dev, PE_SRC_SEND_CAPS);
		}
		else if (req == PD_POLICY_MNGR_REQ_GOTO_MIN)
		{
			dev->request_goto_min = true;
			pe_set_state(dev, PE_SRC_TRANSITION_SUPPLY);
		}
		else if (req == PD_POLICY_MNGR_REQ_PR_SWAP)
		{
			pe_set_state(dev, PE_PRS_SEND_SWAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_DR_SWAP)
		{
			pe_set_state(dev, PE_DRS_SEND_SWAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_VCONN_SWAP)
		{
			pe_set_state(dev, PE_VCS_SEND_SWAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_HARD_RESET)
		{
			pe_set_state(dev, PE_SRC_HARD_RESET);
		}
		else
		{
			status = STATUS_REQUEST_NOT_SUPPORTED_IN_CURRENT_STATE;
		}
	}
	else if (*dev->current_state == PE_SRC_WAIT_NEW_CAPS)
	{
		if (req == PD_POLICY_MNGR_REQ_SRC_CAPS_CHANGE)
		{
			pe_set_state(dev, PE_SRC_SEND_CAPS);
		}
		else
		{
			status = STATUS_REQUEST_NOT_SUPPORTED_IN_CURRENT_STATE;
		}
	}
	else if (*dev->current_state == PE_SNK_READY)
	{
		if (req == PD_POLICY_MNGR_REQ_UPDATE_REMOTE_CAPS)
		{
			pe_set_state(dev, PE_SNK_GET_SOURCE_CAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_PR_SWAP)
		{
			pe_set_state(dev, PE_PRS_SEND_SWAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_DR_SWAP)
		{
			pe_set_state(dev, PE_DRS_SEND_SWAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_VCONN_SWAP)
		{
			pe_set_state(dev, PE_VCS_SEND_SWAP);
		}
		else if (req == PD_POLICY_MNGR_REQ_HARD_RESET)
		{
			pe_set_state(dev, PE_SNK_HARD_RESET);
		}
		else
		{
			status = STATUS_REQUEST_NOT_SUPPORTED_IN_CURRENT_STATE;
		}
	}
	else
	{
		status = STATUS_REQUEST_NOT_SUPPORTED_IN_CURRENT_STATE;
	}

	usb_pd_pe_state_machine(0);

	return status;
}

bool usb_pd_pe_is_remote_externally_powered(unsigned int port)
{
	return pd[port].remote_externally_powered;
}

static void timeout_src_settling(unsigned int port)
{
	pe_set_state(&pd[port], PE_SRC_TRANSITION_SUPPLY_EXIT);
	return;
}

void usb_pd_pe_voltage_alarm_handler(unsigned int port, bool hi_voltage)
{
	usb_pd_port_t *dev = &pd[port];
	usb_pd_port_config_t *config = usb_pd_pm_get_config(port);

	if (hi_voltage)
	{
		if (*dev->current_state == PE_SRC_TRANSITION_SUPPLY)
		{
			// Positive voltage transition complete.  
			// Start power supply settling timeout.
			timer_start(&dev->timer, config->src_settling_time_ms, timeout_src_settling);
		}
		else if (*dev->current_state == PE_PRS_SOURCE_ON)
		{
			timer_start(&dev->timer, T_VBUS_5V_STABLIZE_MS, timeout_vbus_5v_stabilize);
		}
		else if (*dev->current_state == PE_SRC_TRANSITION_TO_DEFAULT)
		{
			pe_set_state(dev, PE_SRC_STARTUP);
		}
		else if ((*dev->current_state == PE_SNK_STARTUP) ||
				 (*dev->current_state == PE_SNK_DISCOVERY))
		{
			dev->vbus_present = true;

			if (*dev->current_state == PE_SNK_DISCOVERY)
			{
				pe_set_state(dev, PE_SNK_WAIT_FOR_CAPS);
			}
		}
	}
	else /* Low voltage */
	{
		if (*dev->current_state == PE_SRC_TRANSITION_TO_DEFAULT)
		{
			// Request policy manager to set the Port Data Role to DFP.
			if (dev->data_role != PD_DATA_ROLE_DFP)
			{
				dev->data_role = PD_DATA_ROLE_DFP;

				tcpm_update_msg_header_info(dev->port, dev->data_role, dev->power_role);

#ifdef CONFIG_TUSB422_PAL
				usb_pd_pal_notify_connect_state(dev->port, dev->tcpc_dev->state, dev->tcpc_dev->plug_polarity);
//                usb_pd_pal_data_role_swap(dev->port, PD_DATA_ROLE_DFP);
#endif
			}

			// Start source recovery timer to enable VBUS 5V.
			timer_start(&dev->timer, T_SRC_RECOVER_MS, timeout_src_recover);
		}
		else if (*dev->current_state == PE_PRS_TRANSITION_TO_OFF)
		{
			if (dev->power_role == PD_PWR_ROLE_SRC)
			{
				pe_set_state(dev, PE_PRS_ASSERT_RD);
			}
		}
		else if (*dev->current_state == PE_SRC_TRANSITION_SUPPLY)
		{
			// Negative voltage transition complete.  
			// Start power supply settling timeout.
			timer_start(&dev->timer, config->src_settling_time_ms, timeout_src_settling);
		}
		else if ((*dev->current_state == PE_SNK_STARTUP) ||
				 (*dev->current_state == PE_SNK_DISCOVERY))
		{
			if ((dev->tcpc_dev->cc_status & CC_STATUS_CC1_CC2_STATE_MASK) == CC_STATE_OPEN)
			{
				tcpm_enable_vbus_detect(dev->port);
			}
			else
			{
			// Configure pins for USB mode.
			tcpm_mux_control(dev->port, dev->data_role, MUX_USB, dev->tcpc_dev->plug_polarity);

			// Set Hi voltage alarm to determine when VBUS is present.
			tcpm_set_voltage_alarm_hi(dev->port, VSAFE5V_MIN);
			}
		}
	}

	return;
}


void usb_pd_pe_current_change_handler(unsigned int port, tcpc_cc_snk_state_t curr_adv)
{
	usb_pd_port_t *dev = &pd[port];

	// Used by SNK for collision avoidance.
	if (curr_adv == CC_SNK_STATE_POWER30)
	{
		// Sink Tx OK.  Sink can initiate an AMS.
	}
	else if (curr_adv == CC_SNK_STATE_POWER15)
	{
		// Sink Tx NG.
	}

	if (!dev->explicit_contract && !dev->power_role_swap_in_progress)
	{
		// Notify system of current change.
		tcpm_sink_vbus(port, false, 5000, GET_SRC_CURRENT_MA(dev->tcpc_dev->src_current_adv));
	}

	return;
}

void usb_pd_pe_init(unsigned int port, usb_pd_port_config_t *config)
{
	usb_pd_port_t *dev = &pd[port];
	unsigned int i;

	dev->tcpc_dev = tcpm_get_device(port);

	pd[port].state_idx = 0;

	for (i = 0; i < PD_STATE_HISTORY_LEN; i++)
	{
		pd[port].state[i] = (usb_pd_pe_state_t)0xEE;
	}

	dev->state_change = false;
	dev->current_state = &dev->state[0];

	dev->port = port;
	dev->timer.data = port;
	dev->timer2.data = port;

	// Copy externally_powered from config as this parameter could change dynamically.
	dev->externally_powered = config->externally_powered;

	usb_pd_pe_handle_unattach(dev);

	return;
}

