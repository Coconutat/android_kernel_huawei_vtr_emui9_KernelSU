/*
 * typec.c
 *
 * Implements the Type-C FUSB3601_State machine functions
 */

#include <linux/delay.h>
#include "typec.h"

#include "log.h"
#include "protocol.h"
#include "policy.h"
#include "callbacks.h"
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#include <linux/hisi/usb/hisi_hifi_usb.h>
#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();
#define IN_FUNCTION hwlog_info("%s ++\n", __func__);
#define OUT_FUNCTION hwlog_info("%s --\n", __func__);
#define LINE_FUNCTION hwlog_info("%s %d++\n", __func__,__LINE__);
/* This is a value for checking illegal cable issues. */
#define MAX_CABLE_LOOP  20
extern int ignore_unattach_once;
extern int state_machine_need_resched;
extern void usb_analog_hs_fsa4476_set_gpio_state(int enn, int en1, int en2);
extern void FUSB3601_platform_notify_dual_role_instance_changed(void);
void FUSB3601_resetDp(struct Port *port);
/* Entry point to the Type-C FUSB3601_State machine */
void FUSB3601_dump_register(void);
void FUSB3601_StateMachineTypeC(struct Port *port)
{
	static bool log_print = true;
	if (port->tc_enabled_ == TRUE) {
		/* PD FUSB3601_State Machines */
		if (port->pd_active_) {
			FUSB3601_USBPDProtocol(port);
			FUSB3601_USBPDPolicyEngine(port);
		}

		/* Type-C FUSB3601_State Machine */
		switch (port->tc_state_) {
		case Disabled:
			FUSB3601_StateMachineDisabled(port);
			break;
		case ErrorRecovery:
			FUSB3601_StateMachineErrorRecovery(port);
			break;
		case Unattached:
			FUSB3601_StateMachineUnattached(port);
			break;
#ifdef FSC_HAVE_SNK
		case AttachedSink:
			FUSB3601_StateMachineAttachedSink(port);
			break;
#ifdef FSC_HAVE_DRP
		case TryWaitSink:
			FUSB3601_StateMachineTryWaitSink(port);
			break;
		case TrySink:
			FUSB3601_StateMachineTrySink(port);
			break;
#endif /* FSC_HAVE_DRP */
#endif /* FSC_HAVE_SNK */
#ifdef FSC_HAVE_SRC
		case AttachWaitSource:
			FUSB3601_StateMachineAttachWaitSource(port);
			break;
		case AttachedSource:
			FUSB3601_StateMachineAttachedSource(port);
			break;
#ifdef FSC_HAVE_DRP
		case TryWaitSource:
			FUSB3601_StateMachineTryWaitSource(port);
			break;
		case TrySource:
			FUSB3601_StateMachineTrySource(port);
			break;
#endif /* FSC_HAVE_DRP */
#endif /* FSC_HAVE_SRC */
#ifdef FSC_HAVE_ACC
		case AudioAccessory:
			FUSB3601_StateMachineAudioAccessory(port);
			break;
		case DebugAccessorySource:
			FUSB3601_StateMachineDebugAccessorySource(port);
			break;
		case DebugAccessorySink:
			FUSB3601_StateMachineDebugAccessorySink(port);
			break;
		case AttachWaitAccessory:
			FUSB3601_StateMachineAttachWaitAccessory(port);
			break;
		case PoweredAccessory:
			FUSB3601_StateMachinePoweredAccessory(port);
			break;
		case UnsupportedAccessory:
			FUSB3601_StateMachineUnsupportedAccessory(port);
			break;
#endif /* FSC_HAVE_ACC */
		case DelayUnattached:
			FUSB3601_StateMachineDelayUnattached(port);
			break;
		case IllegalCable:
			FUSB3601_StateMachineIllegalCable(port);
			break;
		default:
			/* We shouldn't get here, so go to the unattached FUSB3601_State just in case */
			hwlog_err("error happened for typec state machine\n");
			FUSB3601_SetStateUnattached(port);
			break;
		}
		log_print = true;
	} else {
		if (log_print) {
			hwlog_info("typec state machine is disabled\n");
			log_print = false;
		}
	}
}

void FUSB3601_StateMachineDisabled(struct Port *port)
{
	FUSB3601_SetStateUnattached(port);
}

void FUSB3601_StateMachineErrorRecovery(struct Port *port)
{
	if (FUSB3601_TimerExpired(&port->tc_state_timer_)) {
		FUSB3601_SetStateUnattached(port);
	}
}

void FUSB3601_StateMachineDelayUnattached(struct Port *port)
{
	if (FUSB3601_TimerExpired(&port->tc_state_timer_)) {
		FUSB3601_SetStateUnattached(port);
	}
}

void FUSB3601_StateMachineUnattached(struct Port *port)
{
	if (port->registers_.AlertL.I_PORT_PWR == 1) {
		FUSB3601_platform_log(port->port_id_, "FUSB Unattached I_PORT_PWR, VBUS_VAL: ", port->registers_.PwrStat.VBUS_VAL);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
		FUSB3601_ReadRegister(port, regPWRSTAT);
		if(port->registers_.PwrStat.VBUS_VAL == 1) {
			FUSB3601_TimerStart(&port->tc_state_timer_, 100 * kMSTimeFactor);
			FUSB3601_TimerStart(&port->policy_state_timer_, 500 * kMSTimeFactor);
			FUSB3601_platform_log(port->port_id_, "FUSB Unattached Starting C2A Timer ", 1);
		} else {
			FUSB3601_TimerDisable(&port->tc_state_timer_);
			FUSB3601_TimerDisable(&port->policy_state_timer_);
			port->registers_.PwrCtrl.EN_BLEED_DISCH = 0;
			FUSB3601_WriteRegister(port, regPWRCTRL);
			FUSB3601_SendCommand(port, DisableSinkVbus);
			FUSB3601_platform_notify_unattached_vbus_only();
			FUSB3601_set_usb_switch(port, NONE);
			FUSB3601_SetStateUnattached(port);
			return;
			/* VBUS Only Removed Event */
		}
	}

	if(FUSB3601_TimerExpired(&port->policy_state_timer_)) {
		FUSB3601_TimerDisable(&port->policy_state_timer_);
		FUSB3601_ReadRegister(port, regPWRCTRL);
		port->registers_.PwrCtrl.EN_BLEED_DISCH = 1;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		FUSB3601_SendCommand(port, SinkVbus);
		FUSB3601_platform_notify_attached_vbus_only();
		FUSB3601_dump_register();
		/* VBUS Only Attach Event */
	}

	/*
	* If we got an interrupt for a CCStat change and if LOOK4CON is clear,
	* then the device is done looking for a connection.
	*/
	if (port->registers_.AlertL.I_CCSTAT == 1 ) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_ReadRegister(port, regCCSTAT);
		//TODO: Can this happen?
		if (port->registers_.CCStat.LOOK4CON == 1) {
			/* Still looking for a connection... */
			port->idle_ = TRUE;
			return;
		}
		if (port->port_type_ == USBTypeC_Source || (port->port_type_ == USBTypeC_DRP && port->registers_.CCStat.CON_RES == 0 && 1 == port->registers_.RoleCtrl.DRP)) {
			port->source_or_sink_ = Source;
		} else {
			port->source_or_sink_ = Sink;
		}

		if (port->port_type_ == USBTypeC_Source || (port->port_type_ == USBTypeC_DRP && port->registers_.CCStat.CON_RES == 0 && 1 == port->registers_.RoleCtrl.DRP)) {
			/* Operating as a Src or DRP-Src */
			FUSB3601_SetStateAttachWaitSource(port);
		} else if (port->port_type_ == USBTypeC_Sink || (port->port_type_ == USBTypeC_DRP && port->registers_.CCStat.CON_RES == 1 && 1 == port->registers_.RoleCtrl.DRP)) {
			FUSB3601_SetStateAttachWaitSink(port);
		} else if (port->acc_support_ && port->port_type_ == USBTypeC_Sink && port->registers_.RoleCtrl.DRP == 1 && port->registers_.CCStat.CON_RES == 0) {
			FUSB3601_SetStateAttachWaitAccessory(port);
		} else {
			/* Reset our CC detection variables for next time through */
			FUSB3601_platform_log(port->port_id_, "SMUnattached Try Again.", -1);
			FUSB3601_SetStateUnattached(port);
		}
		pd_dpm_handle_pe_event(PD_DPM_PE_ABNORMAL_CC_CHANGE_HANDLER, NULL);
	  }
}

#ifdef FSC_HAVE_SRC
void FUSB3601_StateMachineAttachWaitSource(struct Port *port)
{
	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_TimerStart(&port->cc_debounce_timer_, ktCCDebounce);
		FUSB3601_LogTCState(port);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
	}

	if(port->registers_.AlertH.I_VBUS_ALRM_LO) {
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
	}

	if (port->registers_.AlertL.I_PORT_PWR) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
	}

	if ((port->cc_term_ != SRC_RD) && (port->vconn_term_ == SRC_OPEN)) {
		FUSB3601_SetStateUnattached(port);
	} else if(FUSB3601_TimerExpired(&port->cc_debounce_timer_)) {
		if ((port->cc_term_ == SRC_RA) && (port->vconn_term_ == SRC_RA)) {
			if(port->acc_support_ == TRUE)
				FUSB3601_SetStateAudioAccessory(port);
		} else if (port->cc_term_ == SRC_RD && port->vconn_term_ == SRC_RD) {
			FUSB3601_SetStateDebugAccessorySource(port);
		} else if (port->cc_term_ == SRC_RD) {
			/* We have already checked for Rd-Rd
		 	* therefore exactly one pin has Rd.
			* QED
		 	*/
			if (FUSB3601_IsVbusVSafe0V(port)) {
		#ifdef FSC_HAVE_DRP
				if (port->snk_preferred_) {
					FUSB3601_SetStateTrySink(port);
				} else {
		#endif /* FSC_HAVE_DRP */
					FUSB3601_SetStateAttachedSource(port);
				}
			} else if (!FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE_FOR_DOCK_LEAKAGE) && port->tc_substate_ == 0){
				port->tc_substate_ ++;
				hwlog_info("%s: invalid voltage detected on vbus, 0.8v < vbus < 4v\n", __func__);
				FUSB3601_TimerStart(&port->tc_state_timer_, ktSafe0V);
			} else {
				/*do nothing,wait vbus fall*/
				if (FUSB3601_TimerExpired(&port->tc_state_timer_) && port->tc_substate_ == 1){
					hwlog_info("%s: timer expire\n", __func__);
					if (!FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE_FOR_DOCK_LEAKAGE)) {
						hwlog_info("%s: timer expire vbus < 4v\n", __func__);
						#ifdef FSC_HAVE_DRP
						if (port->snk_preferred_) {
							FUSB3601_SetStateTrySink(port);
						} else {
						#endif /* FSC_HAVE_DRP */
							FUSB3601_SetStateAttachedSource(port);
						}
					}
				}
			}
			hwlog_info("%s: port->cc_term = %d, port->vconn_term = %d\n", __func__, port->cc_term_, port->vconn_term_);
			if (port->vconn_term_ == SRC_RA) {
				usb_analog_hs_fsa4476_set_gpio_state(0, 0, 0);
			}
		}
	}
}
#endif /* FSC_HAVE_SRC */

#ifdef FSC_HAVE_ACC
void FUSB3601_StateMachineAttachWaitAccessory(struct Port *port)
{
	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
	}

	if(FUSB3601_TimerExpired(&port->cc_debounce_timer_)) {
		if ((port->cc_term_ == SRC_RA) && (port->vconn_term_ == SRC_RA)) {
			/* If both pins are Ra, it's an audio accessory */
			if(port->acc_support_ == TRUE)
				FUSB3601_SetStateAudioAccessory(port);
		}
	} else if ((port->cc_term_ == SRC_RD) && (port->vconn_term_ == SRC_RA)) {
		FUSB3601_SetStatePoweredAccessory(port);
	} else if (port->cc_term_ == CCTypeOpen && port->vconn_term_ == CCTypeOpen) {
		FUSB3601_SetStateUnattached(port);
	}
}
#endif /* FSC_HAVE_ACC */

#ifdef FSC_HAVE_SNK
void FUSB3601_StateMachineAttachedSink(struct Port *port)
{
	FSC_BOOL vbus_sink_disch_low = FALSE;

	if (port->registers_.FaultStat.VCONN_OCP && port->is_vconn_source_) {
		FUSB3601_platform_log(port->port_id_, "FUSB VCONN OCP", -1);
		FUSB3601_ClearInterrupt(port, regFAULTSTAT, MSK_VCONN_OCP);
		port->registers_.PwrCtrl.EN_VCONN = 0;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		port->registers_.PwrCtrl.EN_VCONN = 1;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		FUSB3601_platform_set_vconn(TRUE);
	}

	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_TimerStart(&port->pd_debounce_timer_, ktPDDebounce);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_ReadRegister(port, regPWRSTAT);
		if(port->registers_.PwrStat.DEBUG_ACC == 1) {
			hwlog_info("%s:DEBUG_ACC = 1", __func__);
			FUSB3601_PDDisable(port); // stop hardreset
			FUSB3601_SetStateDebugAccessorySink(port);
			FUSB3601_ReadRegister(port, regPWRCTRL);
			port->registers_.PwrCtrl.DIS_VALARM = 1;
			port->registers_.PwrCtrl.DIS_VBUS_MON = 1;
			FUSB3601_WriteRegister(port, regPWRCTRL);
			return;
		}
	}
	if (port->registers_.AlertL.I_PORT_PWR) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
		FUSB3601_ReadRegister(port, regPWRSTAT);
	}
	if (port->registers_.AlertH.I_VBUS_SNK_DISC) {
		FUSB3601_platform_log(port->port_id_, "FUSB VBusSnkDiscL: ", port->registers_.VBusSnkDiscL.byte);
		FUSB3601_platform_log(port->port_id_, "FUSB VBusSnkDiscH: ", port->registers_.VBusSnkDiscH.byte);
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_SNK_DISC);
		vbus_sink_disch_low = TRUE;
	}

	/* Update sink current from CC level */
	if(FUSB3601_TimerExpired(&port->pd_debounce_timer_)) {
		FUSB3601_TimerDisable(&port->pd_debounce_timer_);
		FUSB3601_UpdateSinkCurrent(port);
	}

	/* A VBus disconnect should generate an interrupt to wake us up */
	if ((vbus_sink_disch_low || port->registers_.AlertMskL.M_PORT_PWR && !port->registers_.PwrStat.VBUS_VAL)){
		port->registers_.AlertMskH.M_VBUS_SNK_DISC = 0;
		FUSB3601_WriteRegisters(port, regALERTMSKH, 1);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_SNK_DISC);
		if ((port->is_pr_swap_ == FALSE) && (port->registers_.FMControl4.VBUS_DETATCH_DET == 1)) {
			if (port->is_hard_reset_ == FALSE) {
				hwlog_info("%s:unattach while hardreset is false, vbus_sink_disch_low = %d\n", __func__, vbus_sink_disch_low);
				FUSB3601_platform_delay(5 * 1000);
				FUSB3601_SetStateUnattached(port);
			} else {
				FUSB3601_ReadRegister(port, regTCPC_CTRL);
				if(port->registers_.TcpcCtrl.ORIENT) {
					port->registers_.RoleCtrl.CC1_TERM = CCRoleOpen;
					port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
				} else {
					port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
					port->registers_.RoleCtrl.CC2_TERM = CCRoleOpen;
				}
				FUSB3601_WriteRegister(port, regROLECTRL);
				FUSB3601_platform_delay(10 * 1000);
				FUSB3601_DetectCCPin(port);
				if (port->cc_term_ == SNK_OPEN && port->vconn_term_ == SNK_OPEN) {
					hwlog_info("%s:unattach while hardreset is true%d\n", __func__);
					FUSB3601_platform_delay(5 * 1000);
					FUSB3601_SetStateUnattached(port);
				}
			}
		}
	}
}
#endif /* FSC_HAVE_SNK */

#ifdef FSC_HAVE_SRC
void FUSB3601_StateMachineAttachedSource(struct Port *port)
{
	if (port->registers_.FaultStat.VCONN_OCP && port->is_vconn_source_) {
		FUSB3601_platform_log(port->port_id_, "FUSB VCONN OCP", -1);
		FUSB3601_ClearInterrupt(port, regFAULTSTAT, MSK_VCONN_OCP);
		port->registers_.PwrCtrl.EN_VCONN = 0;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		port->registers_.PwrCtrl.EN_VCONN = 1;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		FUSB3601_platform_set_vconn(TRUE);
		LINE_FUNCTION
	}
	hwlog_info("cc_tem = %d, pr_swap = %d, tx_status = %d,I_CCSTAT = %d\n", port->cc_term_, port->is_pr_swap_, port->pd_tx_status_,port->registers_.AlertL.I_CCSTAT);

	switch(port->tc_substate_) {
	case 0:
		if (port->registers_.AlertL.I_CCSTAT) {
			LINE_FUNCTION
			if((port->pd_tx_status_ != txSend)) {
				FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
			}
			FUSB3601_DetectCCPin(port);
			if(port->cc_term_ == SRC_OPEN) {
				if(FUSB3601_TimerDisabled(&port->cc_debounce_timer_)) {
					FUSB3601_TimerStart(&port->cc_debounce_timer_,ktSourceDetach);
					hwlog_info("ktSourceDetach\n");
					state_machine_need_resched = 1;
				}
			} else {
				FUSB3601_TimerDisable(&port->cc_debounce_timer_);
				hwlog_info("cc_debounce_timer_\n");
				hisi_usb_check_hifi_usb_status(HIFI_USB_TCPC);
				hwlog_info("hisi_usb_check_hifi_usb_status\n");
			}
		}
			/* Look for detach on CC1 */
			if ((FUSB3601_TimerExpired(&port->cc_debounce_timer_)) && (port->is_pr_swap_ == FALSE) && (port->pd_tx_status_ != txSend)) {
				LINE_FUNCTION
				/* Disable PD so we don't disconnect during a Tx */
				FUSB3601_PDDisable(port);
				port->registers_.RoleCtrl.DRP = 0;
				port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
				port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
				FUSB3601_WriteRegister(port, regROLECTRL);
				FUSB3601_start_vbus_discharge(port);
				port->registers_.PwrCtrl.EN_VCONN = 0;
				FUSB3601_WriteRegister(port, regPWRCTRL);
				/* Start timer in case there is contention with Hard Reset */
				FUSB3601_TimerStart(&port->tc_state_timer_, ktPDDebounce);
				port->tc_substate_++;
			} else if (FUSB3601_TimerExpired(&port->tc_state_timer_) && port->unattach_loop_counter_ != 0) {
				LINE_FUNCTION
				port->unattach_loop_counter_ = 0;
			} else {
				LINE_FUNCTION
			}
		break;
	case 1:
		LINE_FUNCTION
		if (port->registers_.AlertH.I_VBUS_ALRM_LO || !FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE_DISCH)) {
			hwlog_info("port->registers_.AlertH.I_VBUS_ALRM_LO = %d\n", port->registers_.AlertH.I_VBUS_ALRM_LO);
			FUSB3601_SetVBusAlarm(port, FSC_VSAFE0V, FSC_VSAFE_DISCH);
			FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
			FUSB3601_set_force_discharge(port);
			  /* Start timer in case force discharge is started after vSafe0V */
			FUSB3601_TimerStart(&port->tc_state_timer_, ktPDDebounce);
			port->tc_substate_++;
			LINE_FUNCTION
		}
		break;
	default:
		LINE_FUNCTION
		if (FUSB3601_IsVbusVSafe0V(port)) {
			/* We've reached vSafe0V */
			FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
#ifdef FSC_HAVE_DRP
			if (port->port_type_ == USBTypeC_DRP && port->src_preferred_ == TRUE) {
				FUSB3601_SetStateTryWaitSink(port);
				LINE_FUNCTION
			} else {
#endif /* FSC_HAVE_DRP */
				FUSB3601_SetStateUnattached(port);
			}
		} else {
			LINE_FUNCTION
		}
		break;
	}
	LINE_FUNCTION
}
#endif /* FSC_HAVE_SRC */

#ifdef FSC_HAVE_DRP
void FUSB3601_StateMachineTryWaitSink(struct Port *port)
{
	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_TimerRestart(&port->pd_debounce_timer_);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
	}

	if (FUSB3601_TimerExpired(&port->pd_debounce_timer_) && (port->cc_term_ == SNK_OPEN) && (port->vconn_term_ == SNK_OPEN)) {
		FUSB3601_SetStateUnattached(port);
	} else if(FUSB3601_TimerExpired(&port->cc_debounce_timer_) && (port->registers_.PwrStat.VBUS_VAL)) {
		FUSB3601_SetStateAttachedSink(port);
	}
}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_DRP
void FUSB3601_StateMachineTrySource(struct Port *port)
{
	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_TimerRestart(&port->pd_debounce_timer_);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
	}

	if (FUSB3601_TimerExpired(&port->pd_debounce_timer_) && (port->cc_term_ == SRC_RD) && (port->vconn_term_ != SRC_RD)) {
		FUSB3601_SetStateAttachedSource(port);
	} else if (FUSB3601_TimerExpired(&port->tc_state_timer_) && (port->cc_term_ != SRC_RD)) {
		FUSB3601_SetStateTryWaitSink(port);
	}
}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_ACC
void FUSB3601_StateMachineDebugAccessorySource(struct Port *port)
{
	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
	}

	if ((port->cc_term_ == SRC_OPEN) || (port->vconn_term_ == SRC_OPEN)) {
		FUSB3601_SetStateUnattached(port);
	} else if(!port->pd_active_ && (port->cc_term_ > port->vconn_term_)) {
		FUSB3601_PDEnable(port, TRUE);
	}
}

void FUSB3601_StateMachineDebugAccessorySink(struct Port *port)
{

	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
	}
	if (port->registers_.AlertL.I_PORT_PWR) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
		FUSB3601_ReadRegister(port, regPWRSTAT);
	}

	if (port->registers_.AlertMskL.M_PORT_PWR && 
		!port->registers_.PwrStat.VBUS_VAL ||
		port->registers_.AlertH.I_VBUS_SNK_DISC) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_SNK_DISC);
		if (port->registers_.FMControl4.VBUS_DETATCH_DET == 1) {
			FUSB3601_SetStateUnattached(port);
		}
	}
}

void FUSB3601_StateMachineAudioAccessory(struct Port *port)
{
	if(port->registers_.AlertL.I_PORT_PWR == 1) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
		FUSB3601_ReadRegister(port, regPWRSTAT);
		if(port->registers_.PwrStat.VBUS_VAL == 1) {
			FUSB3601_SendCommand(port, SinkVbus);
			FUSB3601_platform_notify_attached_vbus_only();
			/* VBUS Only Attach Event */
		} else {
			FUSB3601_SendCommand(port, DisableSinkVbus);
			FUSB3601_platform_notify_unattached_vbus_only();
			/* VBUS Only Removed Event */
		}
	}

	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
		FUSB3601_TimerRestart(&port->cc_debounce_timer_);
		FUSB3601_platform_log(port->port_id_, "FUSB I_CCSTAT:", port->registers_.CCStat.byte);
	}

	if(FUSB3601_TimerExpired(&port->cc_debounce_timer_)) {
		if ((port->cc_term_ == SRC_OPEN) && (port->vconn_term_ == SRC_OPEN)) {
			FUSB3601_SetStateUnattached(port);
		}
	}
}

void FUSB3601_StateMachinePoweredAccessory(struct Port *port)
{
	if (port->cc_term_ == SRC_OPEN) {
		FUSB3601_SetStateUnattached(port);
	} else if (FUSB3601_TimerExpired(&port->tc_state_timer_)) {
		FUSB3601_SetStateUnsupportedAccessory(port);
	} else if (port->mode_entered_ == TRUE) {
		FUSB3601_TimerDisable(&port->tc_state_timer_);
	}
}

void FUSB3601_StateMachineUnsupportedAccessory(struct Port *port)
{
	if (port->cc_term_ == SRC_OPEN) {
		FUSB3601_SetStateUnattached(port);
	}
}
#endif /* FSC_HAVE_ACC */

#ifdef FSC_HAVE_DRP
void FUSB3601_StateMachineTrySink(struct Port *port)
{
	switch (port->tc_substate_) {
	case 0:
		if (port->registers_.AlertL.I_CCSTAT) {
			FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
			FUSB3601_DetectCCPinOriented(port);
		}
		if (FUSB3601_TimerExpired(&port->tc_state_timer_)) {
			FUSB3601_TimerStart(&port->tc_state_timer_, ktDRPTryWait);
			FUSB3601_TimerStart(&port->pd_debounce_timer_, ktPDDebounce);
			port->tc_substate_++;
		}
		break;
	case 1:
		if (port->registers_.AlertL.I_CCSTAT) {
			FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
			FUSB3601_DetectCCPinOriented(port);
			FUSB3601_TimerRestart(&port->pd_debounce_timer_);
		}
		if (port->registers_.AlertL.I_PORT_PWR) {
			FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
		}
		if (FUSB3601_TimerExpired(&port->pd_debounce_timer_)) {
			if (port->registers_.PwrStat.VBUS_VAL
				&& port->cc_term_ != SNK_OPEN
				&& port->vconn_term_ == SNK_OPEN) {
				FUSB3601_SetStateAttachedSink(port);
			} else if (port->cc_term_ == SNK_OPEN
				&& port->vconn_term_ == SNK_OPEN) {
				FUSB3601_SetStateTryWaitSource(port);
			}
		}
#ifdef FSC_HAVE_ACC
		else if ((port->port_type_ == USBTypeC_Sink) && (FUSB3601_TimerExpired(&port->tc_state_timer_))) {
			FUSB3601_SetStateUnsupportedAccessory(port);
		}
#endif /* FSC_HAVE_ACC */
		break;
	default:
		hwlog_err("%s switch default\n", __func__);
		break;
	}
	FUSB3601_platform_log(port->port_id_, "FUSB TrySink State: ", port->tc_substate_);
	FUSB3601_platform_log(port->port_id_, "FUSB TrySink CC: ", port->cc_term_);
	FUSB3601_platform_log(port->port_id_, "FUSB TrySink VCONN: ", port->vconn_term_);
}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_DRP
void FUSB3601_StateMachineTryWaitSource(struct Port *port)
{
	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
		FUSB3601_DetectCCPin(port);
		FUSB3601_TimerRestart(&port->pd_debounce_timer_);
	}
	if(port->registers_.AlertH.I_VBUS_ALRM_LO) {
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
	}

	if (FUSB3601_TimerExpired(&port->pd_debounce_timer_) &&
		!FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE_FOR_DOCK_LEAKAGE) &&
		port->cc_term_ == SRC_RD &&
		port->vconn_term_ != SRC_RD) {
		FUSB3601_SetStateAttachedSource(port);
	} else if (FUSB3601_TimerExpired(&port->tc_state_timer_) &&
		port->cc_term_ != SRC_RD &&
		port->vconn_term_ != SRC_RD) {
		FUSB3601_SetStateUnattached(port);
	}
}
#endif /* FSC_HAVE_DRP */

void FUSB3601_StateMachineIllegalCable(struct Port *port)
{
	if (port->registers_.AlertL.I_CCSTAT) {
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);
	}

	/* Look for detach */
	if (port->cc_term_ == SRC_OPEN) {
		FUSB3601_SetStateUnattached(port);
	}
}

/*
 *  State Machine Configuration
 */
void FUSB3601_SetStateDisabled(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS DIS", -1);

	port->idle_ = FALSE;
	port->tc_state_ = Disabled;
	FUSB3601_TimerDisable(&port->tc_state_timer_);
	FUSB3601_ClearState(port);
}

void FUSB3601_SetStateErrorRecovery(struct Port *port)
{
	IN_FUNCTION
	/* Present Open Open for tErrorRecovery */
	port->registers_.RoleCtrl.CC1_TERM = CCRoleOpen;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleOpen;
	FUSB3601_WriteRegister(port, regROLECTRL);
	FUSB3601_platform_log(port->port_id_, "SS ER", -1);
	port->idle_ = FALSE;
	port->tc_state_ = ErrorRecovery;
	FUSB3601_ClearState(port);
	FUSB3601_TimerStart(&port->tc_state_timer_, ktErrorRecovery);
}

void FUSB3601_SetStateDelayUnattached(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS DU", -1);
	FUSB3601_SetStateUnattached(port);
}

/* SetStateUnattached configures the Toggle state machine in the device to */
/* handle all of the unattached states. */
/* This allows for the MCU to be placed in a low power mode until */
/* the device wakes it up upon detecting something */
void FUSB3601_SetStateUnattached(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS UN", -1);

	port->redobc12_cnt = 0;
	port->idle_ = TRUE;
	port->tc_state_ = Unattached;
	port->activemode_ = Mode_None;
	port->mode_entered_ = FALSE;
	FUSB3601_resetDp(port);
	port->auto_vdm_state_ = AUTO_VDM_INIT;
	FUSB3601_ClearState(port);
	if (ignore_unattach_once == 0) {
		FUSB3601_platform_notify_cc_orientation(NONE);
	} else {
		ignore_unattach_once = 0;
		hwlog_info("%s:reset happened, do not report unattached event to system\n", __func__);
	}
	FUSB3601_UpdateSourceCurrent(port, port->src_current_);

	/* Disable monitoring except for CCStat, */
	port->registers_.AlertMskL.byte = MSK_I_CCSTAT | MSK_I_PORT_PWR;
	port->registers_.AlertMskH.byte = 0;
	port->registers_.AlertMskH.M_FAULT = 1;
	port->registers_.PwrStatMsk.byte = MSK_VBUS_VAL;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 3);

	/* Clear all alert interrupts */
	FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_ALARM_LO_ALL);
	FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_ALARM_HI_ALL);

	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.RP_VAL = 1;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	FUSB3601_WriteRegister(port, regROLECTRL);
	port->registers_.RoleCtrl.RP_VAL = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	FUSB3601_WriteRegister(port, regROLECTRL);
	FUSB3601_platform_delay(1*1000);

	/* Disable monitoring and reconfigure to look for the next connection */
	if (port->port_type_ == USBTypeC_DRP) {
		/* Config as DRP */
		port->registers_.RoleCtrl.DRP = 1;
		/* Start toggling with pull-downs */
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	} else if (port->port_type_ == USBTypeC_Source) {
		/* Config as a source with Rp-Rp */
		port->registers_.RoleCtrl.DRP = 0;
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	} else {
		/* Config as a sink with Rd-Rd - toggle for acc if supported */
		port->registers_.RoleCtrl.DRP = port->acc_support_ ? 1 : 0;
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	}
	FUSB3601_WriteRegister(port, regROLECTRL);
	/* Driver will wait until device detects a new connection */
	FUSB3601_SendCommand(port, Look4Con);
	FUSB3601_ReadRegister(port, regPWRSTAT);
	FUSB3601_ReadRegister(port, regSCP_ENABLE1);
	port->registers_.SCPEnable1.ENABLE_SCP = 1;
	FUSB3601_WriteRegister(port, regSCP_ENABLE1);
	if(port->registers_.PwrStat.VBUS_VAL == 1) {
		FUSB3601_TimerStart(&port->tc_state_timer_, 100 * kMSTimeFactor);
		FUSB3601_TimerStart(&port->policy_state_timer_, 500 * kMSTimeFactor);
	}
	hwlog_info("%s: port->cc_term = %d, port->vconn_term = %d\n", __func__, port->cc_term_, port->vconn_term_);
	usb_analog_hs_fsa4476_set_gpio_state(1, 0, 0);
}

#ifdef FSC_HAVE_SNK
void FUSB3601_SetStateAttachWaitSink(struct Port *port)
{
	IN_FUNCTION
	/* Decide between DebugAcc and Sink */
	FUSB3601_platform_log(port->port_id_, "FUSB SS AW Si", -1);

	if(FUSB3601_TimerExpired(&port->tc_state_timer_)) {
		port->c2a_cable_ = TRUE;
	} else {
		port->c2a_cable_ = FALSE;
	}
	FUSB3601_TimerDisable(&port->tc_state_timer_);

	//TODO: Consider only vbus_snk_dis for PD
	port->registers_.AlertMskL.byte |= MSK_I_PORT_PWR;
	port->registers_.PwrStatMsk.byte = MSK_VBUS_VAL;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 3);

	if(port->registers_.PwrStat.DEBUG_ACC == 0) {
		FUSB3601_SetStateAttachedSink(port);
	} else {
		FUSB3601_SetStateDebugAccessorySink(port);
	}
}
#endif /* FSC_HAVE_SNK */

#ifdef FSC_HAVE_SRC
void FUSB3601_SetStateAttachWaitSource(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS AW Sr", -1);

	/* Disable monitoring except for CCStat, */
	FUSB3601_SetVBusAlarm(port, FSC_VSAFE0V, FSC_VBUS_LVL_HIGHEST);
	port->registers_.AlertMskL.byte = MSK_I_CCSTAT;
	port->registers_.AlertMskH.byte |= MSK_I_VBUS_ALRM_LO;
	port->registers_.PwrStatMsk.byte = 0;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 3);

	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	FUSB3601_WriteRegister(port, regROLECTRL);

	FUSB3601_SetOrientation(port);

	port->tc_state_ = AttachWaitSource;
	FUSB3601_SetStateSource(port, FALSE);

	FUSB3601_TimerStart(&port->cc_debounce_timer_, ktCCDebounce);

	FUSB3601_DetectCCPin(port);
	FUSB3601_platform_delay(5 * 1000);


	port->tc_substate_ = 0;
}
#endif /* FSC_HAVE_SRC */

#ifdef FSC_HAVE_ACC
void FUSB3601_SetStateAttachWaitAccessory(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS AW Acc", -1);

	/* Check for cable looping */
	if (port->unattach_loop_counter_ > MAX_CABLE_LOOP) {
		FUSB3601_SetStateIllegalCable(port);
		return;
	} else {
		FUSB3601_UpdateSourceCurrent(port, port->src_current_);
		port->unattach_loop_counter_++;
	}

	FUSB3601_SetOrientation(port);
	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	FUSB3601_WriteRegister(port, regROLECTRL);
	port->tc_state_ = AttachWaitAccessory;
	FUSB3601_SetStateSource(port, FALSE);
	FUSB3601_TimerDisable(&port->tc_state_timer_);
}
#endif /* FSC_HAVE_ACC */

#ifdef FSC_HAVE_SRC
void FUSB3601_SetStateAttachedSource(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "FUSB SS ASr", -1);

	port->tc_state_ = AttachedSource;
	port->tc_substate_ = 0;

	FUSB3601_DetectCCPin(port);
	FUSB3601_SetOrientation(port);

	if(port->registers_.TcpcCtrl.ORIENT) {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleOpen;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	} else {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleOpen;
	}
	FUSB3601_WriteRegister(port, regROLECTRL);
	FUSB3601_SetStateSource(port, TRUE);

	/* Set up to wait for vSafe5V for PD */
	FUSB3601_SetVBusAlarm(port, 0, FSC_VSAFE5V_L);

	FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI | MSK_I_PORT_PWR);
	FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

	port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
	port->registers_.AlertMskL.M_VBUS_ALRM_HI = 1;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

	/* Enable only the 5V output */
	FUSB3601_SendCommand(port, SourceVbusDefaultV);
	FUSB3601_SetVBusSource5V(port, TRUE);

	/* Start dangling illegal cable timeout */
	FUSB3601_TimerStart(&port->tc_state_timer_, ktIllegalCable);
	FUSB3601_TimerDisable(&port->cc_debounce_timer_);
	port->registers_.PwrCtrl.EN_BLEED_DISCH = 0;
	port->registers_.PwrCtrl.AUTO_DISCH = 0;
	FUSB3601_WriteRegister(port, regPWRCTRL);
	FUSB3601_set_usb_switch(port, port->registers_.TcpcCtrl.ORIENT);
	FUSB3601_platform_notify_cc_orientation(port->registers_.TcpcCtrl.ORIENT);
	FUSB3601_PDEnable(port, TRUE);
}
#endif /* FSC_HAVE_SRC */

#ifdef FSC_HAVE_SNK
void FUSB3601_SetStateAttachedSink(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "FUSB SS ASi, c2a_cable: ", port->c2a_cable_);

	/* Enable ADC for VBUS OVP Value Debug Reporting */
	port->registers_.PwrCtrl.DIS_VBUS_MON = 0;
	FUSB3601_WriteRegister(port, regPWRCTRL);

	FUSB3601_SendCommand(port, DisableSinkVbus);
	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	FUSB3601_WriteRegister(port, regROLECTRL);
	FUSB3601_SendCommand(port, SinkVbus);

	port->unattach_loop_counter_ = 0;
	port->tc_state_ = AttachedSink;
	FUSB3601_SetStateSink(port);
	FUSB3601_SendCommand(port, SinkVbus);

	/* Sink Rp Detection */
	FUSB3601_platform_delay(3 * 1000);
	FUSB3601_DetectCCPin(port);
	FUSB3601_TimerStart(&port->pd_debounce_timer_, ktPDDebounce);

	FUSB3601_ReadRegister(port, regTCPC_CTRL);
	FUSB3601_platform_notify_cc_orientation(port->registers_.TcpcCtrl.ORIENT);

	FUSB3601_PDEnable(port, FALSE);
	hwlog_info("%s: port->cc_term = %d, port->vconn_term = %d\n", __func__, port->cc_term_, port->vconn_term_);
	if (port->vconn_term_ == SRC_RA) {
		usb_analog_hs_fsa4476_set_gpio_state(0, 0, 0);
	}
}
#endif /* FSC_HAVE_SNK */

#ifdef FSC_HAVE_DRP
void FUSB3601_RoleSwapToAttachedSink(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "RS ASi", -1);

	port->tc_state_ = AttachedSink;
	port->source_or_sink_ = Sink;

	if(port->registers_.TcpcCtrl.ORIENT) {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleOpen;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	} else {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleOpen;
	}

	FUSB3601_platform_notify_dual_role_instance_changed();
	FUSB3601_WriteRegister(port, regROLECTRL);

	/* Manually disable VBus */
	FUSB3601_SendCommand(port, DisableSourceVbus);
	FUSB3601_SetVBusSource5V(port, FALSE);

	/* Set up the Sink Disconnect threshold/interrupt */
	FUSB3601_SetVBusSnkDisc(port, FSC_VSAFE5V_DISC);
	FUSB3601_SetVBusStopDisc(port, FSC_VSAFE0V_DISCH);

	port->registers_.AlertMskH.M_VBUS_SNK_DISC = 0;
	FUSB3601_WriteRegisters(port, regALERTMSKH, 1);

	FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_SNK_DISC);

	port->registers_.AlertMskL.byte |= MSK_I_PORT_PWR;
	port->registers_.PwrStatMsk.byte = MSK_VBUS_VAL;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 3);
	FUSB3601_SetStateSink(port);

	/* Set the current advertisement variable to none until */
	/* we determine what the current is */
	port->snk_current_ = utccOpen;
}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_DRP
void FUSB3601_RoleSwapToAttachedSource(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "RS ASr", -1);

	port->tc_state_ = AttachedSource;
	port->source_or_sink_ = Source;

	if(port->registers_.TcpcCtrl.ORIENT) {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleOpen;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	} else {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleOpen;
	}

	FUSB3601_platform_notify_dual_role_instance_changed();
	FUSB3601_WriteRegister(port, regROLECTRL);
	port->registers_.AlertMskH.M_VBUS_SNK_DISC = 0;
	FUSB3601_WriteRegisters(port, regALERTMSKH, 1);
	FUSB3601_SendCommand(port, SourceVbusDefaultV);
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL,NULL);
	FUSB3601_SetVBusSource5V(port, TRUE);
	FUSB3601_SetStateSource(port, FALSE);

	/* Set the Sink current to none (not used in Src) */
	port->snk_current_ = utccOpen;
}
#endif /* FSC_HAVE_DRP */

#if defined(FSC_HAVE_DRP) && defined(FSC_HAVE_SNK)
void FUSB3601_SetStateTryWaitSink(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS TWSi", -1);

	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	FUSB3601_WriteRegister(port, regROLECTRL);

	/* Mask all */
	port->registers_.AlertMskL.byte = MSK_I_ALARM_LO_ALL;
	port->registers_.AlertMskH.byte = MSK_I_ALARM_HI_ALL;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 2);
	FUSB3601_PDDisable(port);
	port->tc_state_ = TryWaitSink;
	FUSB3601_SetStateSink(port);
	FUSB3601_TimerStart(&port->cc_debounce_timer_, ktCCDebounce);
}
#endif /* FSC_HAVE_DRP && FSC_HAVE_SNK */

#ifdef FSC_HAVE_DRP
void FUSB3601_SetStateTrySource(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS TSr", -1);

	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	FUSB3601_WriteRegister(port, regROLECTRL);
	port->idle_ = FALSE;
	port->tc_state_ = TrySource;
	FUSB3601_SetStateSource(port, FALSE);
	FUSB3601_TimerStart(&port->tc_state_timer_, ktDRPTry);
}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_DRP
void FUSB3601_SetStateTrySink(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS TSi", -1);

	port->idle_ = FALSE;
	port->tc_state_ = TrySink;
	port->tc_substate_ = 0;

	/* RevD */
	port->registers_.RoleCtrl.DRP = 0;
	if(port->registers_.TcpcCtrl.ORIENT) {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleOpen;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleRd;
	} else {
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleOpen;
	}
	FUSB3601_WriteRegister(port, regROLECTRL);
	FUSB3601_SetStateSink(port);
	port->registers_.AlertMskL.byte |= MSK_I_PORT_PWR;
	port->registers_.AlertMskH.byte = 0;
	port->registers_.AlertMskH.M_FAULT = 1;
	port->registers_.PwrStatMsk.byte = MSK_VBUS_VAL;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 3);

	/* Set the state timer to tDRPTry to timeout if Rd isn't detected */
	FUSB3601_TimerStart(&port->tc_state_timer_, ktDRPTry);
	FUSB3601_platform_delay(5 * 1000);
	FUSB3601_DetectCCPinOriented(port);
	port->vconn_term_ = SNK_OPEN;
	FUSB3601_platform_log(port->port_id_, "FUSB SetTrySink CC: ", port->cc_term_);
	FUSB3601_platform_log(port->port_id_, "FUSB SetTrySink VCONN: ", port->vconn_term_);
}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_DRP
void FUSB3601_SetStateTryWaitSource(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS TWSr", -1);

	port->idle_ = FALSE;
	port->tc_state_ = TryWaitSource;
	port->registers_.PwrStatMsk.byte = 0;
	FUSB3601_WriteRegister(port, regPWRSTATMSK);
	FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR);
	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	FUSB3601_WriteRegister(port, regROLECTRL);
	FUSB3601_SendCommand(port, DisableSourceVbus);
	FUSB3601_SetVBusSource5V(port, FALSE);
	FUSB3601_SetVBusAlarm(port, FSC_VSAFE0V, FSC_VBUS_LVL_HIGHEST);
	port->registers_.AlertMskH.byte = MSK_I_VBUS_ALRM_LO;
	FUSB3601_SetStateSource(port, FALSE);
	FUSB3601_platform_delay(3 * 1000);
	FUSB3601_ReadRegister(port, regCCSTAT);
	FUSB3601_DetectCCPin(port);
	FUSB3601_TimerStart(&port->tc_state_timer_, ktDRPTry);
	FUSB3601_TimerStart(&port->pd_debounce_timer_, ktPDDebounce);
}
#endif /* FSC_HAVE_DRP */

#if defined(FSC_HAVE_ACC) && (defined(FSC_HAVE_SNK) || defined(FSC_HAVE_SRC))
void FUSB3601_SetStateDebugAccessorySource(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS DA Src", -1);

	port->registers_.RoleCtrl.DRP = 0;
	port->registers_.RoleCtrl.CC1_TERM = CCRoleRp;
	port->registers_.RoleCtrl.CC2_TERM = CCRoleRp;
	FUSB3601_WriteRegister(port, regROLECTRL);

	port->idle_ = FALSE;
	port->registers_.AlertMskL.M_CCSTAT = 1;
	FUSB3601_WriteRegister(port, regALERTMSKL);
	port->unattach_loop_counter_ = 0;
	port->tc_state_ = DebugAccessorySource;
	FUSB3601_SetStateSource(port, FALSE);
	port->registers_.PwrCtrl.AUTO_DISCH = 1;
	FUSB3601_WriteRegister(port, regPWRCTRL);
	/* Enable only the 5V output */
	FUSB3601_SendCommand(port, SourceVbusDefaultV);
	//FUSB3601_SetVBusSource5V(port, TRUE);
}

void FUSB3601_SetStateDebugAccessorySink(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS DA Snk", -1);
	FUSB3601_set_usb_switch(port, NONE);

	/* Disable monitoring except for CCStat, PORT_PWR, FAULT, SNK_DISC */
	port->registers_.AlertMskL.byte = MSK_I_CCSTAT | MSK_I_PORT_PWR;
	port->registers_.AlertMskH.byte = 0;
	port->registers_.AlertMskH.M_FAULT = 1;
	port->registers_.AlertMskH.M_VBUS_SNK_DISC = 1;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

	/*Double 56k Cable Workaround (Huawei) */
	if(port->registers_.PwrStat.DEBUG_ACC && (port->registers_.CCStat.CC1_STAT == SNK_DEFAULT) 
		&& (port->registers_.CCStat.CC2_STAT == SNK_DEFAULT)) {

		port->registers_.TcpcCtrl.ORIENT = 0;
		FUSB3601_WriteRegister(port, regTCPC_CTRL);

		FUSB3601_SendCommand(port, DisableSinkVbus);
		port->registers_.RoleCtrl.DRP = 0;
		port->registers_.RoleCtrl.CC1_TERM = CCRoleRd;
		port->registers_.RoleCtrl.CC2_TERM = CCRoleOpen;
		FUSB3601_WriteRegister(port, regROLECTRL);
		FUSB3601_SendCommand(port, SinkVbus);
		FUSB3601_platform_notify_double_56k();
		port->double56k = TRUE;
	} else {
		FUSB3601_SendCommand(port, DisableSinkVbus);
		FUSB3601_platform_notify_DebugAccessorySink();
	}

	port->tc_state_ = DebugAccessorySink;
	FUSB3601_SetStateSink(port);
}

void FUSB3601_SetStateAudioAccessory(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS AA", -1);
	port->idle_ = FALSE;
	port->unattach_loop_counter_ = 0;
	port->registers_.AlertMskL.byte |= MSK_I_PORT_PWR;
	port->registers_.AlertMskH.byte = MSK_I_FAULT;
	port->registers_.PwrStatMsk.byte = MSK_VBUS_VAL;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 3);
	FUSB3601_platform_notify_audio_accessory();
	FUSB3601_ReadRegister(port, regPWRSTAT);
	if(port->registers_.PwrStat.VBUS_VAL == 1) {
		FUSB3601_SendCommand(port, SinkVbus);
		FUSB3601_platform_notify_attached_vbus_only();
		/* VBUS Only Attach Event */
	}
	port->tc_state_ = AudioAccessory;
	FUSB3601_LogTCState(port);
	FUSB3601_set_usb_switch(port, NONE);
	FUSB3601_set_sbu_switch(port, Sbu_None);
}

void FUSB3601_SetStatePoweredAccessory(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS PA", -1);
	port->idle_ = TRUE;
	port->tc_state_ = PoweredAccessory;

	/* Note that sourcing VBus for powered accessories is not supported in
	* Type-C 1.2, but is done here because not all accessories work without it.
	*/
	FUSB3601_SendCommand(port, SourceVbusDefaultV);
	FUSB3601_SetVBusSource5V(port, TRUE);
	FUSB3601_SetStateSource(port, TRUE);
	FUSB3601_PDEnable(port, TRUE);
	port->registers_.PwrCtrl.AUTO_DISCH = 1;
	FUSB3601_WriteRegister(port, regPWRCTRL);

	/* Note/TODO: State timer should be enabled here to transition to
	* UnsupportedAccessory if an alternate mode is not entered in time.
	* This isn't working 100% so the timer has been disabled for now.
	*/
	FUSB3601_TimerStart(&port->tc_state_timer_, ktAMETimeout);
}

void FUSB3601_SetStateUnsupportedAccessory(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS UA", -1);
	port->idle_ = TRUE;
	/* Mask for COMP */
	port->registers_.AlertMskL.M_CCSTAT = 1;
	FUSB3601_WriteRegister(port, regALERTMSKL);
	/* Vbus was enabled in PoweredAccessory - disable it here. */
	FUSB3601_SendCommand(port, DisableSourceVbus);
	FUSB3601_SetVBusSource5V(port, FALSE);
	port->tc_state_ = UnsupportedAccessory;
	/* Must advertise default current */
	FUSB3601_UpdateSourceCurrent(port, utccDefault);
	/* Turn off VConn */
	FUSB3601_SetStateSource(port, FALSE);
	/* port->platform_notify_unsupported_accessory(); */
	port->registers_.PwrCtrl.AUTO_DISCH = 1;
	FUSB3601_WriteRegister(port, regPWRCTRL);
}
#endif /* FSC_HAVE_ACC && (FSC_HAVE_SNK || FSC_HAVE_SRC) */

void FUSB3601_SetStateIllegalCable(struct Port *port)
{
	IN_FUNCTION
	FUSB3601_platform_log(port->port_id_, "SS IllCab", -1);
	port->idle_ = TRUE;
	port->tc_state_ = IllegalCable;
	port->unattach_loop_counter_ = 0;
	FUSB3601_SetStateSource(port, FALSE);
	/* Advertise Default current */
	FUSB3601_UpdateSourceCurrent(port, utccDefault);
	/* Turn on VBus Bleed Discharge resistor to provide path to ground */
	port->registers_.PwrCtrl.EN_BLEED_DISCH = 1;
	FUSB3601_WriteRegister(port, regPWRCTRL);
	/* Wait for 10ms for the CC line level to stabilize */
	FUSB3601_platform_delay(10 * 1000);

	/* Set AUTO_DISCH and wait for a detatch */
	port->registers_.PwrCtrl.AUTO_DISCH = 1;
	FUSB3601_WriteRegister(port, regPWRCTRL);

}
