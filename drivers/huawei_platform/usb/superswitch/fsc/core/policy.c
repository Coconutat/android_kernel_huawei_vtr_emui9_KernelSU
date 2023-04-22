/*
 * policy.c
 *
 * Implements the Policy state machine functions
 */

#include "policy.h"

#include "platform.h"
#include "PDTypes.h"
#include "timer.h"
#include "typec.h"
#include "protocol.h"
#ifdef FSC_HAVE_VDM
#include "vdm.h"
#ifdef FSC_HAVE_DP
#include "display_port.h"
#endif /* FSC_HAVE_DP */
#endif /* FSC_HAVE_VDM */
#include "vendor_info.h"
#include "callbacks.h"
#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG FUSB3601_TAG
#define PD_FIXED_POWER_VOL_STEP 50
HWLOG_REGIST();
#define IN_FUNCTION hwlog_info("%s ++\n", __func__);
#define OUT_FUNCTION hwlog_info("%s --\n", __func__);
#define LINE_FUNCTION hwlog_info("%s%d --\n", __func__,__LINE__);
static FSC_U32 pd_limit_voltage = PD_09_V;

void FUSB3601_SetPDLimitVoltage(int vol)
{
	if (vol < PD_ADAPTER_5V || vol > PD_ADAPTER_20V) {
		hwlog_info("%s,Set limit voltage over range\n", __func__);
		return;
	}
	pd_limit_voltage = vol / PD_FIXED_POWER_VOL_STEP;
}

void FUSB3601_USBPDPolicyEngine(struct Port *port)
{
	switch (port->policy_state_) {
	case peDisabled:
		break;
	case peErrorRecovery:
		FUSB3601_PolicyErrorRecovery(port);
		break;
	/* ###################### Source States  ##################### */
#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined (FSC_HAVE_ACC))
	case peSourceSendHardReset:
		FUSB3601_PolicySourceSendHardReset(port);
		break;
	case peSourceSendSoftReset:
		FUSB3601_PolicySourceSendSoftReset(port);
		break;
	case peSourceSoftReset:
		FUSB3601_PolicySourceSoftReset(port);
		break;
	case peSourceStartup:
		FUSB3601_PolicySourceStartup(port);
		break;
	case peSourceDiscovery:
		FUSB3601_PolicySourceDiscovery(port);
		break;
	case peSourceSendCaps:
		FUSB3601_PolicySourceSendCaps(port);
		break;
	case peSourceDisabled:
		FUSB3601_PolicySourceDisabled(port);
		break;
	case peSourceTransitionDefault:
		FUSB3601_PolicySourceTransitionDefault(port);
		break;
	case peSourceNegotiateCap:
		FUSB3601_PolicySourceNegotiateCap(port);
		break;
	case peSourceCapabilityResponse:
		FUSB3601_PolicySourceCapabilityResponse(port);
		break;
	case peSourceTransitionSupply:
		FUSB3601_PolicySourceTransitionSupply(port);
		break;
	case peSourceReady:
		FUSB3601_PolicySourceReady(port);
		break;
	case peSourceGiveSourceCaps:
		FUSB3601_PolicySourceGiveSourceCap(port);
		break;
	case peSourceGetSinkCaps:
		FUSB3601_PolicySourceGetSinkCap(port);
		break;
	case peSourceSendPing:
		FUSB3601_PolicySourceSendPing(port);
		break;
	case peSourceGotoMin:
		FUSB3601_PolicySourceGotoMin(port);
		break;
	case peSourceGiveSinkCaps:
		FUSB3601_PolicySourceGiveSinkCap(port);
		break;
	case peSourceGetSourceCaps:
		FUSB3601_PolicySourceGetSourceCap(port);
		break;
	case peSourceSendDRSwap:
		FUSB3601_PolicySourceSendDRSwap(port);
		break;
	case peSourceEvaluateDRSwap:
		FUSB3601_PolicySourceEvaluateDRSwap(port);
		break;
	case peSourceSendVCONNSwap:
		FUSB3601_PolicySourceSendVCONNSwap(port);
		break;
	case peSourceSendPRSwap:
		FUSB3601_PolicySourceSendPRSwap(port);
		break;
	case peSourceEvaluatePRSwap:
		FUSB3601_PolicySourceEvaluatePRSwap(port);
		break;
	case peSourceWaitNewCapabilities:
		FUSB3601_PolicySourceWaitNewCapabilities(port);
		break;
	case peSourceEvaluateVCONNSwap:
		FUSB3601_PolicySourceEvaluateVCONNSwap(port);
		break;
#endif /* FSC_HAVE_SRC */
/* ###################### Sink States  ####################### */
#ifdef FSC_HAVE_SNK
	case peSinkStartup:
		FUSB3601_PolicySinkStartup(port);
		break;
	case peSinkSendHardReset:
		FUSB3601_PolicySinkSendHardReset(port);
		break;
	case peSinkSoftReset:
		FUSB3601_PolicySinkSoftReset(port);
		break;
	case peSinkSendSoftReset:
		FUSB3601_PolicySinkSendSoftReset(port);
		break;
	case peSinkTransitionDefault:
		FUSB3601_PolicySinkTransitionDefault(port);
		break;
	case peSinkDiscovery:
		FUSB3601_PolicySinkDiscovery(port);
		break;
	case peSinkWaitCaps:
		FUSB3601_PolicySinkWaitCaps(port);
		break;
	case peSinkEvaluateCaps:
		FUSB3601_PolicySinkEvaluateCaps(port);
		break;
	case peSinkSelectCapability:
		FUSB3601_PolicySinkSelectCapability(port);
		break;
	case peSinkTransitionSink:
		FUSB3601_PolicySinkTransitionSink(port);
		break;
	case peSinkReady:
		FUSB3601_PolicySinkReady(port);
		break;
	case peSinkGiveSinkCap:
		FUSB3601_PolicySinkGiveSinkCap(port);
		break;
	case peSinkGetSourceCap:
		FUSB3601_PolicySinkGetSourceCap(port);
		break;
	case peSinkGetSinkCap:
		FUSB3601_PolicySinkGetSinkCap(port);
		break;
	case peSinkGiveSourceCap:
		FUSB3601_PolicySinkGiveSourceCap(port);
		break;
	case peSinkSendDRSwap:
		FUSB3601_PolicySinkSendDRSwap(port);
		break;
	case peSinkEvaluateDRSwap:
		FUSB3601_PolicySinkEvaluateDRSwap(port);
		break;
	case peSinkEvaluateVCONNSwap:
		FUSB3601_PolicySinkEvaluateVCONNSwap(port);
		break;
	case peSinkSendPRSwap:
		FUSB3601_PolicySinkSendPRSwap(port);
		break;
	case peSinkEvaluatePRSwap:
		FUSB3601_PolicySinkEvaluatePRSwap(port);
		break;
#endif /* FSC_HAVE_SNK */
	/* ---------- VDM --------------------- */
	case peDfpCblVdmIdentityRequest:
		FUSB3601_PolicyDfpCblVdmIdentityRequest(port);
		break;
	/* ---------- BIST Receive Mode --------------------- */
	case PE_BIST_Receive_Mode:      /* Bist Receive Mode */
		FUSB3601_PolicyBISTReceiveMode(port);
		break;
	case PE_BIST_Frame_Received:    /* Test Frame received by Protocol layer */
		FUSB3601_PolicyBISTFrameReceived(port);
		break;
	/* ---------- BIST Carrier Mode and Eye Pattern ----- */
	case PE_BIST_Carrier_Mode_2:     /* BIST Carrier Mode 2 */
		FUSB3601_PolicyBISTCarrierMode2(port);
		break;
	case PE_BIST_Test_Data:
		FUSB3601_PolicyBISTTestData(port);
		break;
	default:
		/* invalid state, reset */
		FUSB3601_PolicyInvalidState(port);
		break;
	}
}

void FUSB3601_PolicyErrorRecovery(struct Port *port)
{
	FUSB3601_SetStateErrorRecovery(port);
}

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
void FUSB3601_PolicySourceSendHardReset(struct Port *port)
{
	FUSB3601_ProtocolSendHardReset(port);
}

void FUSB3601_PolicySourceSendSoftReset(struct Port *port)
{
	switch (port->policy_subindex_) {
	case 0:
		hwlog_info("%s,send soft reset\n", __func__);
		FUSB3601_ProtocolSendData(port, CMTSoftReset, 0, 0, ktSenderResponse, peSourceSendHardReset, TRUE, SOP_TYPE_SOP);
		port->auto_vdm_state_ = AUTO_VDM_INIT;
		port->policy_subindex_++;
		break;
	default:
		if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
			port->protocol_msg_rx_ = FALSE;
			port->pd_tx_status_ = txIdle;
			if ((port->policy_rx_header_.NumDataObjects == 0) && (port->policy_rx_header_.MessageType == CMTAccept)) {
				hwlog_info("%s,soft reset was accepted\n", __func__);
				FUSB3601_set_policy_state(port, peSourceSendCaps);
				FUSB3601_TimerDisable(&port->policy_state_timer_);
				FUSB3601_PolicySourceSendCaps(port);
			} else {
				hwlog_info("%s,soft reset was not accepted\n", __func__);
				FUSB3601_ProtocolSendHardReset(port);
			}
		} else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
			hwlog_info("%s,soft reset timeout\n", __func__);
			FUSB3601_ProtocolSendHardReset(port);
		}
		break;
	}
}

void FUSB3601_PolicySourceSoftReset(struct Port *port)
{
	switch(port->policy_subindex_) {
	case 0:
		hwlog_info("%s,receive soft reset,send accept cmd\n", __func__);
		port->auto_vdm_state_ = AUTO_VDM_INIT;
		FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0, 0, peSourceSendHardReset, TRUE, SOP_TYPE_SOP);
		port->policy_subindex_++;
		break;
	default:
		if(port->pd_tx_status_ == txSuccess) {
			hwlog_info("%s,send accept succ\n", __func__);
			port->pd_tx_status_ = txIdle;
			FUSB3601_set_policy_state(port, peSourceSendCaps);
			FUSB3601_PolicySourceSendCaps(port);
		}
		break;
	}
}

void FUSB3601_PolicySourceStartup(struct Port *port)
{
#ifdef FSC_HAVE_VDM
	FSC_U8 i = 0;
#endif /* FSC_HAVE_VDM */
   switch(port->policy_subindex_){

     case 0:
	/* Wait until we reach vSafe5V and delay if coming from PR Swap */
	if ((FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE5V_L) && ((FUSB3601_TimerExpired(&port->policy_state_timer_))
		|| FUSB3601_TimerDisabled(&port->policy_state_timer_))) || port->tc_state_ == PoweredAccessory) {
		port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
		port->registers_.AlertMskL.M_VBUS_ALRM_HI = 0;
		FUSB3601_WriteRegisters(port, regALERTMSKL, 2);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI | MSK_I_PORT_PWR);
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
		FUSB3601_TimerDisable(&port->policy_state_timer_);

#ifdef FSC_HAVE_VDM
		port->auto_mode_disc_tracker_ = 0;
		port->core_svid_info_.num_svids = 0;
		for (i = 0; i < MAX_NUM_SVIDS; i++) {
			port->core_svid_info_.svids[i] = 0;
		}
#endif /* FSC_HAVE_VDM */
#ifdef FSC_HAVE_DP
		port->display_port_data_.AutoModeEntryObjPos = -1;
#endif /* FSC_HAVE_DP */
        FUSB3601_TimerStart(&port->policy_state_timer_,ktSourceDetach + ktPDDebounce);
		port->policy_subindex_++;
		}
		break;
	default :
	    if(FUSB3601_TimerExpired(&port->policy_state_timer_)){
		FUSB3601_set_policy_state(port, peDfpCblVdmIdentityRequest);
		FUSB3601_PolicyDfpCblVdmIdentityRequest(port);
      }
	break;
   }
}

void FUSB3601_PolicySourceDiscovery(struct Port *port)
{
	switch (port->policy_subindex_) {
	case 0:
		FUSB3601_TimerStart(&port->policy_state_timer_, ktTypeCSendSourceCap);
		port->policy_subindex_++;
		break;
	default:
		if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
			if (port->caps_counter_ > MAX_CAPS_COUNT) {
				/* No PD sink connected */
				FUSB3601_set_policy_state(port, peSourceDisabled);
				FUSB3601_PolicySourceDisabled(port);
			} else {
				FUSB3601_set_policy_state(port, peSourceSendCaps);
				FUSB3601_PolicySourceSendCaps(port);
			}
		}
		break;
	}
}

void FUSB3601_PolicySourceSendCaps(struct Port *port)
{
    switch (port->policy_subindex_) {
		case 0:
			port->expecting_vdm_response_ = FALSE;
		port->protocol_msg_rx_ = FALSE;
			FUSB3601_ProtocolSendData(port, DMTSourceCapabilities,
					   port->caps_header_source_.NumDataObjects,
					   port->caps_source_, ktSenderResponse, peSourceDiscovery,
					   TRUE, SOP_TYPE_SOP);
			port->policy_subindex_++;
        break;
      default:
        if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {

          port->protocol_msg_rx_ = FALSE;
          port->pd_tx_status_ = txIdle;

          port->is_hard_reset_ = FALSE;
          port->hard_reset_counter_ = 0;
          port->caps_counter_ = 0;

          if ((port->policy_rx_header_.NumDataObjects == 1) &&
              (port->policy_rx_header_.MessageType == DMTRequest)) {
            FUSB3601_set_policy_state(port, peSourceNegotiateCap);
            FUSB3601_PolicySourceNegotiateCap(port);
          }
          else {
            /* Unexpected message */
            FUSB3601_set_policy_state(port, peSourceSendSoftReset);
            FUSB3601_PolicySourceSendSoftReset(port);
          }
        }
        else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
          port->protocol_msg_rx_ = FALSE;
          FUSB3601_ProtocolSendHardReset(port);
        }
        break;
    }
}

void FUSB3601_PolicySourceDisabled(struct Port *port)
{
  port->usb_pd_contract_.object = 0;
}

void FUSB3601_PolicySourceTransitionDefault(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {

	  FUSB3601_platform_notify_pd_contract(FALSE, 0, 0, FALSE);

        port->is_hard_reset_ = TRUE;
        port->policy_has_contract_ = FALSE;

//        port->registers_.PwrCtrl.AUTO_DISCH = 0;
//        WriteRegister(port, regPWRCTRL);

        if (port->is_vconn_source_) {
          port->registers_.PwrCtrl.EN_VCONN = 0;
        }
        FUSB3601_WriteRegister(port, regPWRCTRL);

        if (!port->policy_is_dfp_) {
          port->policy_is_dfp_ = TRUE;
          port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
          FUSB3601_WriteRegister(port, regMSGHEADR);
        }

        /* Set first stage discharge */
        FUSB3601_start_vbus_discharge(port);
		
		FUSB3601_TimerStart(&port->policy_state_timer_, ktPSHardReset);
		
        port->policy_subindex_++;
      }
      break;
    case 1:
      if (port->registers_.AlertH.I_VBUS_ALRM_LO
    		  || (FUSB3601_TimerExpired(&port->policy_state_timer_)
    		  && !FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE_DISCH))) {
    	  FUSB3601_SetVBusAlarm(port, FSC_VSAFE0V, FSC_VSAFE_DISCH);
    	  FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

        FUSB3601_set_force_discharge(port);
        /* Start timer in case force discharge is started after vSafe0V */
        FUSB3601_TimerStart(&port->policy_state_timer_, ktPDDebounce);

        port->policy_subindex_++;
      }
	  else if(FUSB3601_TimerExpired(&port->policy_state_timer_)) {
    	  FUSB3601_TimerStart(&port->policy_state_timer_, ktPSHardReset);
      }
      break;
    case 2:
      if(!FUSB3601_IsVbusOverVoltage(port, FSC_VEXTRASAFE0V)) {
        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

        /* We've reached vSafe0V */
        port->registers_.PwrCtrl.FORCE_DISCH = 0;
        FUSB3601_WriteRegister(port, regPWRCTRL);

	port->registers_.AlertMskL.M_VBUS_ALRM_HI = 0;
	port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

        FUSB3601_TimerStart(&port->policy_state_timer_, ktSrcRecover);

        port->policy_subindex_++;
      }
      break;
    default:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
	  /* Set up to wait for vSafe5V for PD */
		FUSB3601_SetVBusAlarm(port, 0, FSC_VSAFE5V_L);

		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI | MSK_I_PORT_PWR);
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

		port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
		port->registers_.AlertMskL.M_VBUS_ALRM_HI = 1;
		FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

        FUSB3601_SendCommand(port, SourceVbusDefaultV);
        FUSB3601_SetVBusSource5V(port, TRUE);

        //port->registers_.PwrCtrl.AUTO_DISCH = 1;
        port->registers_.PwrCtrl.EN_VCONN = 1;
        FUSB3601_WriteRegister(port, regPWRCTRL);
        port->is_vconn_source_ = TRUE;

        FUSB3601_set_policy_state(port, peSourceStartup);
        FUSB3601_PolicySourceStartupHelper(port);
        FUSB3601_PolicySourceStartup(port);
      }
      break;
  }
}

void FUSB3601_PolicySourceNegotiateCap(struct Port *port)
{
  /* This state evaluates if the sink request can be met or not */
  /* and sets the next state accordingly */
  FSC_BOOL req_accept = FALSE;
  FSC_U8 obj_position =
    port->policy_rx_data_obj_[0].FVRDO.ObjectPosition;

  if ((obj_position > 0) &&
      (obj_position <= port->caps_header_source_.NumDataObjects)) {
    if (port->policy_rx_data_obj_[0].FVRDO.OpCurrent <=
         port->caps_source_[obj_position - 1].FPDOSupply.MaxCurrent) {
      req_accept = TRUE;
    }
  }

  if (req_accept) {
    FUSB3601_set_policy_state(port, peSourceTransitionSupply);
    FUSB3601_PolicySourceTransitionSupply(port);
  }
  else {
    FUSB3601_set_policy_state(port, peSourceCapabilityResponse);
    FUSB3601_PolicySourceCapabilityResponse(port);
  }
}

void FUSB3601_PolicySourceCapabilityResponse(struct Port *port)
{
	static int next_state;

	switch(port->policy_subindex_) {
	case 0:
	  if (port->policy_has_contract_) {
		if (port->is_contract_valid_) {
		  FUSB3601_ProtocolSendData(port, CMTReject, 0, 0, 0, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
		  next_state = peSourceReady;
		}
		else {
		  FUSB3601_ProtocolSendData(port, CMTReject, 0, 0,
							0, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
		  next_state = peSourceSendHardReset;
		}
	  }
	  else {
		FUSB3601_ProtocolSendData(port, CMTReject, 0, 0,
						  0, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
		next_state = peSourceWaitNewCapabilities;
	  }
	  port->policy_subindex_++;
	  break;
	default:
		if(port->pd_tx_status_ == txSuccess) {
			port->pd_tx_status_= txIdle;
			FUSB3601_set_policy_state(port, next_state);
		}
		break;
	}
}

//TODO: Clean out unneeded register settings
//TODO: Higher voltage than 5V
void FUSB3601_PolicySourceTransitionSupply(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0, ktSrcTransition, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
      port->policy_subindex_++;
      break;
    case 1:
      if ((port->pd_tx_status_ == txSuccess) && FUSB3601_TimerExpired(&port->policy_state_timer_))
      {
	  port->pd_tx_status_ = txIdle;

          port->policy_has_contract_ = TRUE;
          port->usb_pd_contract_.object = port->policy_rx_data_obj_[0].object;

          //TODO: Set voltage-

          FUSB3601_TimerStart(&port->policy_state_timer_, ktSrcTransitionSupply);
          port->policy_subindex_++;
      }
      break;
    case 2:
      /* Give VBUS time to source */
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {

          /* Now that we have a contract, set Rp value to 3.0A to represent
                 *   sink transmit Go.
                 * TODO - Toggle between 1.5A and 3.0A when we are ready to
                 *   allow/disallow sink transmit functionality.
                 */
          FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0, 0, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
          port->policy_subindex_++;
      }
      break;
    case 3:
    	if(port->pd_tx_status_ == txSuccess) {
    		port->pd_tx_status_ = txIdle;
                FUSB3601_SetRpValue(port, utcc3p0A);
    		FUSB3601_platform_notify_pd_contract(TRUE, port->caps_source_[port->usb_pd_contract_.FVRDO.ObjectPosition -1].FPDOSupply.Voltage,
    				port->usb_pd_contract_.FVRDO.OpCurrent, port->caps_source_[0].FPDOSupply.ExternallyPowered);
			FUSB3601_set_policy_state(port, peSourceReady);
			FUSB3601_PolicySourceReady(port);
    	}
    	break;
	default:
		FUSB3601_set_policy_state(port, peSourceReady);
    		FUSB3601_PolicySourceReady(port);
		break;
  }
}

void FUSB3601_PolicySourceReady(struct Port *port)
{
	/* Handle expired VDM timer */
  if (FUSB3601_TimerExpired(&port->vdm_timer_)) {
	FUSB3601_ResetPolicyState(port);
	FUSB3601_TimerDisable(&port->vdm_timer_);
  }
  /* Pick up any leftover successful sends */
  if(port->pd_tx_status_ == txSuccess) port->pd_tx_status_ = txIdle;

  if (port->protocol_msg_rx_) {
    port->protocol_msg_rx_ = FALSE;
    if (port->policy_rx_header_.NumDataObjects == 0) {
      switch (port->policy_rx_header_.MessageType) {
        case CMTGetSourceCap:
          FUSB3601_set_policy_state(port, peSourceGiveSourceCaps);
          FUSB3601_PolicySourceGiveSourceCap(port);
          break;
        case CMTGetSinkCap:
          FUSB3601_set_policy_state(port, peSourceGiveSinkCaps);
          FUSB3601_PolicySourceGiveSinkCap(port);
          break;
        case CMTDR_Swap:
          FUSB3601_set_policy_state(port, peSourceEvaluateDRSwap);
          FUSB3601_PolicySourceEvaluateDRSwap(port);
          break;
        case CMTPR_Swap:
          FUSB3601_set_policy_state(port, peSourceEvaluatePRSwap);
          FUSB3601_PolicySourceEvaluatePRSwap(port);
          break;
        case CMTVCONN_Swap:
          FUSB3601_set_policy_state(port, peSourceEvaluateVCONNSwap);
          FUSB3601_PolicySourceEvaluateVCONNSwap(port);
          break;
        case CMTSoftReset:
          FUSB3601_set_policy_state(port, peSourceSoftReset);
          FUSB3601_PolicySourceSoftReset(port);
          break;
        default:
          /* Send a reject message for all other commands */
          break;
      }
    }
    else {
      switch (port->policy_rx_header_.MessageType) {
        case DMTRequest:
          FUSB3601_set_policy_state(port, peSourceNegotiateCap);
          FUSB3601_PolicySourceNegotiateCap(port);
          break;
#ifdef FSC_HAVE_VDM
        case DMTVendorDefined:
		FUSB3601_ProcessVdmMessage(port);
          break;
#endif /* FSC_HAVE_VDM */
        case DMTBIST:
          FUSB3601_ProcessDmtBist(port);
          break;
        default:
          /* Otherwise we've rec'd a message we don't know how to handle yet */
          break;
      }
    }
  }
  else if (port->pd_tx_flag_) {
	port->pd_tx_flag_ = FALSE;
    if (port->pd_transmit_header_.NumDataObjects == 0) {
      switch (port->pd_transmit_header_.MessageType) {
        case CMTGetSinkCap:
          FUSB3601_set_policy_state(port, peSourceGetSinkCaps);
          FUSB3601_PolicySourceGetSinkCap(port);
          break;
        case CMTGetSourceCap:
          FUSB3601_set_policy_state(port, peSourceGetSourceCaps);
          FUSB3601_PolicySourceGetSourceCap(port);
          break;
        case CMTPing:
          FUSB3601_set_policy_state(port, peSourceSendPing);
          FUSB3601_PolicySourceSendPing(port);
          break;
        case CMTGotoMin:
          FUSB3601_set_policy_state(port, peSourceGotoMin);
          FUSB3601_PolicySourceGotoMin(port);
          break;
#ifdef FSC_HAVE_DRP
        case CMTPR_Swap:
          FUSB3601_set_policy_state(port, peSourceSendPRSwap);
          FUSB3601_PolicySourceSendPRSwap(port);
          break;
#endif /* FSC_HAVE_DRP */
        case CMTDR_Swap:
          FUSB3601_set_policy_state(port, peSourceSendDRSwap);
          FUSB3601_PolicySourceSendDRSwap(port);
          break;
        case CMTVCONN_Swap:
          FUSB3601_set_policy_state(port, peSourceSendVCONNSwap);
          FUSB3601_PolicySourceSendVCONNSwap(port);
          break;
        case CMTSoftReset:
          FUSB3601_set_policy_state(port, peSourceSendSoftReset);
          FUSB3601_PolicySourceSendSoftReset(port);
          break;
        default:
          /* Unknown command */
          break;
      }
    }
    else {
      switch (port->pd_transmit_header_.MessageType) {
        case DMTSourceCapabilities:
          FUSB3601_set_policy_state(port, peSourceSendCaps);
          FUSB3601_PolicySourceSendCaps(port);
          break;
        case DMTVendorDefined:
#ifdef FSC_HAVE_VDM
#endif /* FSC_HAVE_VDM */
          break;
        default:
          break;
      }
    }
  }
  else if (port->partner_caps_.object == 0) {
    FUSB3601_set_policy_state(port, peSourceGetSinkCaps);
    FUSB3601_PolicySourceGetSinkCap(port);
  }
#ifdef FSC_HAVE_VDM
  else if((port->policy_is_dfp_ == TRUE) &&
          (port->auto_vdm_state_ != AUTO_VDM_DONE) &&
          !port->expecting_vdm_response_ &&
		  FUSB3601_TimerExpired(&port->policy_state_timer_))
  {
    FUSB3601_AutoVdmDiscovery(port);
  }
#endif /* FSC_HAVE_VDM */
  else if (port->renegotiate_) {
    port->renegotiate_ = FALSE;
    /* TODO - untested - not sure if this is right... */
    FUSB3601_set_policy_state(port, peSourceGiveSourceCaps);
    FUSB3601_PolicySourceGiveSourceCap(port);
  }
}

void FUSB3601_PolicySourceGiveSourceCap(struct Port *port)
{
//	switch (port->policy_subindex_) {
//	case 0:
//  ProtocolSendData(port, DMTSourceCapabilities,
//                 port->caps_header_source_.NumDataObjects,
//                 port->caps_source_, 0, peSourceSendSoftReset, SOP_TYPE_SOP);
//  port->policy_subindex_++;
//  break;
//	default:
//		if (port->pd_tx_status_ == txSuccess) {
//			port->pd_tx_status_ = txIdle;
//			set_policy_state(port, peSourceReady);
//			PolicySourceReady(port);
//		}
//	}
	  FUSB3601_set_policy_state(port, peSourceSendCaps);
	  FUSB3601_PolicySourceSendCaps(port);
}

void FUSB3601_PolicySourceGetSinkCap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTGetSinkCap, 0, 0,
                            ktSenderResponse, peSourceSendSoftReset, FALSE, SOP_TYPE_SOP);
      port->policy_subindex_++;
      break;
    case 1:
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if ((port->policy_rx_header_.NumDataObjects > 0) &&
            (port->policy_rx_header_.MessageType == DMTSinkCapabilities)) {
			FUSB3601_UpdateCapabilitiesRx(port, FALSE);
			FUSB3601_TimerStart(&port->policy_state_timer_, ktVDMWait);
			FUSB3601_set_policy_state(port, peSourceReady);
			FUSB3601_PolicySourceReady(port);
        }
        else {
          /* No valid sink caps message */
        	FUSB3601_PolicySourceReady(port);
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
    	  FUSB3601_PolicySourceReady(port);
      }
      break;
	default:
	    if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
			FUSB3601_set_policy_state(port, peSourceReady);
    		FUSB3601_PolicySourceReady(port);
		}
	  break;
  }
}

void FUSB3601_PolicySourceSendPing(struct Port *port)
{
	switch(port->policy_subindex_) {
	case 0:
  FUSB3601_ProtocolSendData(port, CMTPing, 0, 0, 0, peSourceSendSoftReset, FALSE, SOP_TYPE_SOP);
  /* FIXME - Send the ping to the right SOP */
#if 0
  ProtocolSendData(port, CMTPing, 0, 0,
                    0, peSourceSendSoftReset, port->policy_msg_tx_sop_);
#endif /* 0 */
  port->policy_subindex_++;
  break;
	default:
		if(port->pd_tx_status_ == txSuccess) {
			port->pd_tx_status_= txIdle;
			FUSB3601_set_policy_state(port, peSourceReady);
			FUSB3601_PolicySourceReady(port);
		}
		break;
	}
}

void FUSB3601_PolicySourceGotoMin(struct Port *port)
{
  if (port->protocol_msg_rx_) {
    port->protocol_msg_rx_ = FALSE;
    if (port->policy_rx_header_.NumDataObjects == 0) {
      /* Handling a control message */
      switch (port->policy_rx_header_.MessageType) {
        case CMTSoftReset:
          FUSB3601_set_policy_state(port, peSourceSoftReset);
          FUSB3601_PolicySourceSoftReset(port);
          break;
        default:
          /* If we receive any other command (including Reject and Wait), */
          /* just go back to the ready state without changing. */
          break;
      }
    }
  }
  else {
	  //TODO: This functionality
    switch (port->policy_subindex_) {
      case 0:
        FUSB3601_ProtocolSendData(port, CMTGotoMin, 0, 0,
                          ktSrcTransition, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
        break;
      case 1:
        if ((port->pd_tx_status_ == txSuccess) && FUSB3601_TimerExpired(&port->policy_state_timer_)) {
		port->pd_tx_status_= txIdle;
          port->policy_subindex_++;
        }
        break;
      case 2:
        /* Adjust the power supply if necessary... */
        port->policy_subindex_++;
        break;
      case 3:
        /* Validate the output is ready prior to sending the ready message */
        port->policy_subindex_++;
        break;
      case 4:
        FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0, 0, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
        break;
      default:
	  if(port->pd_tx_status_ == txSuccess) {
		  port->pd_tx_status_= txIdle;
		  FUSB3601_set_policy_state(port, peSourceReady);
	  }
	  break;
    }
  }
}

void FUSB3601_PolicySourceGiveSinkCap(struct Port *port)
{
	switch(port->policy_subindex_) {
	case 0:
	#ifdef FSC_HAVE_DRP
	  if ((port->protocol_msg_rx_sop_ == SOP_TYPE_SOP) &&
		  (port->port_type_ == USBTypeC_DRP))
		FUSB3601_ProtocolSendData(port, DMTSinkCapabilities,
					   port->caps_header_sink_.NumDataObjects,
					   port->caps_sink_, 0, peSourceSendSoftReset, FALSE, SOP_TYPE_SOP);
	  else
	#endif /* FSC_HAVE_DRP */
		FUSB3601_ProtocolSendData(port, CMTReject, 0, 0,
						  0, peSourceSendSoftReset, FALSE, port->protocol_msg_rx_sop_);
		port->policy_subindex_++;
	default:
		if(port->pd_tx_status_ == txSuccess) {
			port->pd_tx_status_ = txIdle;
			FUSB3601_set_policy_state(port, peSourceReady);
			FUSB3601_PolicySourceReady(port);
		}
		break;
	}
}

void FUSB3601_PolicySourceGetSourceCap(struct Port *port)
{

	switch(port->policy_subindex_) {
	case 0:
		FUSB3601_ProtocolSendData(port, CMTGetSourceCap, 0, 0, 0, peSourceSendSoftReset, FALSE, SOP_TYPE_SOP);
		port->policy_subindex_++;
	default:
		if(port->pd_tx_status_ == txSuccess) {
			port->pd_tx_status_ = txIdle;
			FUSB3601_set_policy_state(port, peSourceReady);
			FUSB3601_PolicySourceReady(port);
		}
		break;
	}
}

void FUSB3601_PolicySourceSendDRSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTDR_Swap, 0, 0,
                                 ktSenderResponse, peErrorRecovery, FALSE, SOP_TYPE_SOP);
      port->policy_subindex_++;
      break;
    default:
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          /* Received a control message */
          switch (port->policy_rx_header_.MessageType) {
            case CMTAccept:
              port->policy_is_dfp_ = (port->policy_is_dfp_ == TRUE)?FALSE:TRUE;
              port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
	      port->auto_vdm_state_ = AUTO_VDM_INIT;
              FUSB3601_WriteRegister(port, regMSGHEADR);
              FUSB3601_set_policy_state(port, peSourceReady);
              FUSB3601_PolicySourceReady(port);
              break;
            case CMTSoftReset:
              FUSB3601_set_policy_state(port, peSourceSoftReset);
              FUSB3601_PolicySourceSoftReset(port);
              break;
            default:
              /* For other commands, including Reject & Wait, */
              /* just go back to the ready state without changing. */
              FUSB3601_set_policy_state(port, peSourceReady);
              FUSB3601_PolicySourceReady(port);
              break;
          }
        }
        else {
          /* Received data message */
          FUSB3601_set_policy_state(port, peSourceReady);
          FUSB3601_PolicySourceReady(port);
        }
        port->pd_tx_status_ = txIdle;
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peSourceReady);
        port->pd_tx_status_ = txIdle;
        FUSB3601_PolicySourceReady(port);
      }
      break;
  }
}

void FUSB3601_PolicySourceEvaluateDRSwap(struct Port *port)
{
	switch(port->policy_subindex_) {
	case 0:
		#ifdef FSC_HAVE_VDM
		  if (port->mode_entered_ == TRUE) {
			  FUSB3601_ProtocolSendHardReset(port);
			return;
		  }
		#endif /* FSC_HAVE_VDM */
		  if (port->protocol_msg_rx_sop_ == SOP_TYPE_SOP) {
			FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0,
									   0, peErrorRecovery, FALSE, SOP_TYPE_SOP);
			port->auto_vdm_state_ = AUTO_VDM_INIT;
		  }
		  else {
			FUSB3601_ProtocolSendData(port, CMTReject, 0, 0,
									   0, peErrorRecovery, FALSE, SOP_TYPE_SOP);
		  }
		port->policy_subindex_++;
	default:
		if(port->pd_tx_status_ == txSuccess) {
			port->pd_tx_status_ = txIdle;
			port->policy_is_dfp_ = (port->policy_is_dfp_ == TRUE) ? FALSE : TRUE;
			port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
			FUSB3601_WriteRegister(port, regMSGHEADR);
			FUSB3601_set_policy_state(port, peSourceReady);
			FUSB3601_PolicySourceReady(port);
		}
		break;
	}
}

//TODO:
void FUSB3601_PolicySourceSendVCONNSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTVCONN_Swap, 0, 0,
                            ktSenderResponse, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
      break;
    case 1:
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTAccept:
              port->policy_subindex_++;
              FUSB3601_TimerDisable(&port->policy_state_timer_);
              break;
            case CMTWait:
            case CMTReject:
              FUSB3601_set_policy_state(port, peSourceReady);
              port->pd_tx_status_ = txIdle;
              break;
            default:
              /* Ignore all other commands */
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peSourceReady);
        port->pd_tx_status_ = txIdle;
      }
      else {
        port->idle_ = TRUE;
      }
      break;
    case 2:
      if (port->is_vconn_source_) {
        FUSB3601_TimerStart(&port->policy_state_timer_, ktVCONNSourceOn);
        port->policy_subindex_++;
      }
      else {
        /* Apply VConn */
        port->registers_.PwrCtrl.EN_VCONN = 1;
        FUSB3601_WriteRegister(port, regPWRCTRL);
        port->is_vconn_source_ = TRUE;

        /* Skip next state and send the PS_RDY msg after the timer expires */
        port->policy_subindex_ = 4;
      }
      break;
    case 3:
      if (port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTPS_RDY:
              /* Disable VCONN source */
              port->registers_.PwrCtrl.EN_VCONN = 0;
              FUSB3601_WriteRegister(port, regPWRCTRL);
              port->is_vconn_source_ = FALSE;

              FUSB3601_set_policy_state(port, peSourceReady);
              port->pd_tx_status_ = txIdle;
              break;
            default:
              /* Ignore all other commands received */
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        port->pd_tx_status_ = txIdle;
        FUSB3601_ProtocolSendHardReset(port);
      }
      break;
    case 4:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0, 0, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
      }
      break;
    default:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_set_policy_state(port, peSourceReady);
	}
	break;
  }
}

void FUSB3601_PolicySourceSendPRSwap(struct Port *port)
{
#ifdef FSC_HAVE_DRP
  switch (port->policy_subindex_) {
    case 0:
      /* Send the PRSwap command */
      FUSB3601_ProtocolSendData(port, CMTPR_Swap, 0, 0,
                            ktSenderResponse, peSourceSendSoftReset, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
      break;
    case 1:
      /* Require Accept message to move on or go back to ready state */
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_= txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTAccept:
              port->is_pr_swap_ = TRUE;
              port->policy_has_contract_ = FALSE;
              FUSB3601_TimerStart(&port->policy_state_timer_, ktSrcTransition);
              port->policy_subindex_++;
              break;
            case CMTWait:
            case CMTReject:
              FUSB3601_set_policy_state(port, peSourceReady);
              port->is_pr_swap_ = FALSE;
              port->pd_tx_status_ = txIdle;
              FUSB3601_PolicySourceReady(port);
              break;
            default:
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peSourceReady);
        port->is_pr_swap_ = FALSE;
        port->pd_tx_status_ = txIdle;
      }
      break;
    case 2:
      /* Wait for tSrcTransition and then turn off power (and Rd on/Rp off) */
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        /* Disable VBUS, set alarm (vSafe0V), and force discharge */
        port->registers_.PwrCtrl.AUTO_DISCH = 0;
        port->registers_.PwrCtrl.DIS_VBUS_MON = 0;
        FUSB3601_WriteRegister(port, regPWRCTRL);

        FUSB3601_SetVBusAlarm(port, FSC_VSAFE0V, FSC_VBUS_LVL_HIGHEST);
        port->registers_.AlertMskH.M_VBUS_ALRM_LO = 1;
        FUSB3601_WriteRegister(port, regALERTMSKH);
        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

        FUSB3601_SendCommand(port, DisableSourceVbus);
        FUSB3601_SetVBusSource5V(port, FALSE);

        FUSB3601_SetVBusStopDisc(port, FSC_VSAFE0V_DISCH);

        port->registers_.PwrCtrl.FORCE_DISCH = 1;
        port->registers_.PwrCtrl.DIS_VALARM = 0;
        FUSB3601_WriteRegister(port, regPWRCTRL);

        port->policy_subindex_++;
      }
      break;
    case 3:
      if (port->registers_.AlertH.I_VBUS_ALRM_LO) {
        /* We've reached vSafe0V */
        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

        port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
        FUSB3601_WriteRegister(port, regALERTMSKH);

        port->registers_.PwrCtrl.FORCE_DISCH = 0;
        port->registers_.PwrCtrl.DIS_VALARM = 1;
        FUSB3601_WriteRegister(port, regPWRCTRL);

        FUSB3601_RoleSwapToAttachedSink(port);

        port->policy_is_source_ = FALSE;
        port->registers_.MsgHeadr.POWER_ROLE = port->policy_is_source_;
        FUSB3601_WriteRegister(port, regMSGHEADR);
        FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0,
                                 ktPSSourceOn, peErrorRecovery, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
      }
      break;
    case 4:
      /* Wait to receive a PS_RDY message from the new DFP */
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_= txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTPS_RDY:
              port->policy_subindex_++;
              FUSB3601_TimerStart(&port->policy_state_timer_, ktGoodCRCDelay);
              break;
            default:
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peErrorRecovery);
        port->pd_tx_status_ = txIdle;
        FUSB3601_PolicyErrorRecovery(port);
      }
      break;
    default:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peSinkStartup);

        FUSB3601_SetVBusSnkDisc(port, FSC_VSAFE5V_DISC);

        FUSB3601_SendCommand(port, SinkVbus);

        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_SNK_DISC);
        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
        FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI);
        FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);

        port->registers_.AlertMskH.M_VBUS_SNK_DISC = 0;
        FUSB3601_WriteRegister(port, regALERTMSKH);

        port->registers_.PwrCtrl.AUTO_DISCH = 1;
        FUSB3601_WriteRegister(port, regPWRCTRL);

        FUSB3601_TimerDisable(&port->policy_state_timer_);
        FUSB3601_PolicySinkStartup(port);
      }
      break;
  }
#endif /* FSC_HAVE_DRP */
}

void FUSB3601_PolicySourceEvaluatePRSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      /* Sending accept or reject */
      if (port->protocol_msg_rx_sop_ != SOP_TYPE_SOP ||
          port->caps_source_[0].FPDOSupply.DualRolePower == FALSE) {
        /* Send the reject if we are not a DRP */
        FUSB3601_ProtocolSendData(port, CMTReject, 0, 0,
                          0, peSourceSendSoftReset, TRUE, port->protocol_msg_rx_sop_);
        port->policy_subindex_ = 1;
      }
      else {
	  FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0,
                         ktSrcTransition, peSourceSendSoftReset, TRUE, port->protocol_msg_rx_sop_);
	        port->policy_subindex_ = 2;
      }
      break;
    case 1:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_set_policy_state(port, peSourceReady);
		FUSB3601_PolicySourceReady(port);
	}
    case 2:
      if ((port->pd_tx_status_ == txSuccess) && FUSB3601_TimerExpired(&port->policy_state_timer_)) {

	  FUSB3601_platform_notify_pd_contract(FALSE, 0, 0, FALSE);

        /* Disable VBUS, set alarm (vSafe0V), and force discharge */
	  port->pd_tx_status_= txIdle;
	  port->is_pr_swap_ = TRUE;
        port->policy_has_contract_ = FALSE;
        port->registers_.PwrCtrl.AUTO_DISCH = 0;
        port->registers_.PwrCtrl.DIS_VBUS_MON = 0;
        FUSB3601_WriteRegister(port, regPWRCTRL);

        /* Set first stage discharge */
        FUSB3601_start_vbus_discharge(port);

        port->policy_subindex_++;
      }
      break;
    case 3:
      if (port->registers_.AlertH.I_VBUS_ALRM_LO) {
	  FUSB3601_SetVBusAlarm(port, FSC_VSAFE0V, FSC_VSAFE_DISCH);
		  FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

		  FUSB3601_set_force_discharge(port);
		  /* Start timer in case force discharge is started after vSafe0V */
		  FUSB3601_TimerStart(&port->policy_state_timer_, ktPDDebounce);

        port->policy_subindex_++;
      }
	break;
    case 4:
      if (FUSB3601_IsVbusVSafe0V(port)) {
        /* We've reached vSafe0V */
        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

        /* We've reached vSafe0V */
		port->registers_.PwrCtrl.FORCE_DISCH = 0;
		port->registers_.PwrCtrl.DIS_VALARM = 1;
		FUSB3601_WriteRegister(port, regPWRCTRL);

		port->registers_.AlertMskL.M_VBUS_ALRM_HI = 0;
		port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
		FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

        FUSB3601_RoleSwapToAttachedSink(port);

        port->policy_is_source_ = FALSE;
        port->registers_.MsgHeadr.POWER_ROLE = port->policy_is_source_;
        FUSB3601_WriteRegister(port, regMSGHEADR);

		  FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0, ktPSSourceOn,
									 peErrorRecovery, TRUE, port->protocol_msg_rx_sop_);
		  port->policy_subindex_++;
      }
    default:
      /* Wait to receive a PS_RDY message from the new DFP */
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTPS_RDY:
              port->policy_subindex_++;
              port->is_pr_swap_ = FALSE;

			  FUSB3601_SetVBusSnkDisc(port, FSC_VSAFE5V_DISC);

			  FUSB3601_SendCommand(port, SinkVbus);

			  FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_SNK_DISC);
			  FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);
			  FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI);
			  FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_CCSTAT);

			  port->registers_.AlertMskH.M_VBUS_SNK_DISC = 0;
			  FUSB3601_WriteRegister(port, regALERTMSKH);

			  port->registers_.PwrCtrl.AUTO_DISCH = 1;
			  FUSB3601_WriteRegister(port, regPWRCTRL);

              FUSB3601_set_policy_state(port, peSinkStartup);
              FUSB3601_PolicySinkStartup(port);
              break;
            default:
		FUSB3601_set_policy_state(port, peErrorRecovery);
		FUSB3601_PolicyErrorRecovery(port);
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        port->is_pr_swap_ = FALSE;
        FUSB3601_set_policy_state(port, peErrorRecovery);
        port->pd_tx_status_ = txIdle;
        FUSB3601_PolicyErrorRecovery(port);
      }
      break;
  }
}

void FUSB3601_PolicySourceWaitNewCapabilities(struct Port *port)
{
  if (port->unattach_loop_counter_ == 0) {
    port->idle_ = TRUE;
    port->registers_.AlertMskL.M_RXHRDRST = 1;
    port->registers_.AlertMskL.M_RXSTAT = 1;
    port->registers_.AlertMskL.M_CCSTAT = 1;
    FUSB3601_WriteRegister(port, regALERTMSKL);
  }

  switch (port->policy_subindex_) {
    case 0:
      /* Wait for Policy Manager to change source capabilities */
      break;
    default:
      FUSB3601_set_policy_state(port, peSourceSendCaps);
      break;
  }
}

void FUSB3601_PolicySourceEvaluateVCONNSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      if (port->protocol_msg_rx_sop_ == SOP_TYPE_SOP) {
        FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0,
                          0, peSourceSendSoftReset, TRUE, port->protocol_msg_rx_sop_);
        port->policy_subindex_++;
      }
      else {
        FUSB3601_ProtocolSendData(port, CMTReject, 0, 0,
                          0, peSourceSendSoftReset, TRUE, port->protocol_msg_rx_sop_);
        port->policy_subindex_ = 4;
      }
      break;
    case 1:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		  if (port->is_vconn_source_) {
			FUSB3601_TimerStart(&port->policy_state_timer_, ktVCONNSourceOn);
			port->policy_subindex_++;
		  }
		  else {
			/* Apply VConn */
			port->registers_.PwrCtrl.EN_VCONN = 1;
			FUSB3601_WriteRegister(port, regPWRCTRL);
			FUSB3601_platform_set_vconn(TRUE);
			port->is_vconn_source_ = TRUE;
			FUSB3601_TimerStart(&port->policy_state_timer_, ktVCONNOnDelay);
			port->policy_subindex_ = 3;
		  }
	}
      break;
    case 2:
      if (port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTPS_RDY:
              /* Disable VConn */
              port->registers_.PwrCtrl.EN_VCONN = 0;
              FUSB3601_WriteRegister(port, regPWRCTRL);
			  FUSB3601_platform_set_vconn(FALSE);
              port->is_vconn_source_ = FALSE;

              FUSB3601_set_policy_state(port, peSourceReady);
              port->pd_tx_status_ = txIdle;
              FUSB3601_PolicySourceReady(port);
              break;
            default:
		FUSB3601_ProtocolSendHardReset(port);
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        port->pd_tx_status_ = txIdle;
        FUSB3601_ProtocolSendHardReset(port);
      }
      break;
    case 3:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0,
                          0, peSourceSendSoftReset, TRUE, port->protocol_msg_rx_sop_);
        port->policy_subindex_++;
      }
      break;
    default:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_set_policy_state(port, peSourceReady);
		FUSB3601_PolicySourceReady(port);
	}
	break;
  }
}
#endif /* FSC_HAVE_SRC */

#ifdef FSC_HAVE_SNK
void FUSB3601_PolicySinkStartup(struct Port *port)
{

#ifdef FSC_HAVE_VDM
  FSC_U8 i = 0;
#endif /* FSC_HAVE_VDM */


  /* Enable Masks */
  port->registers_.AlertMskL.M_PORT_PWR = 1;
  port->registers_.AlertMskL.M_VBUS_ALRM_HI = 0;
  port->registers_.AlertMskL.M_TX_DISC = 1;
  port->registers_.AlertMskL.M_TXFAIL = 1;
  port->registers_.AlertMskL.M_TXSUCC = 1;
  port->registers_.AlertMskL.M_RXSTAT = 1;
  port->registers_.AlertMskL.M_RXHRDRST = 1;
  FUSB3601_WriteRegister(port, regALERTMSKL);

  /* Disable BIST TMODE bit if needed */
  if(port->registers_.TcpcCtrl.BIST_TMODE == 1) {
    port->registers_.TcpcCtrl.BIST_TMODE = 0;
    FUSB3601_WriteRegister(port, regTCPC_CTRL);
  }

  port->usb_pd_contract_.object = 0;
  port->sink_partner_max_power_ = 0;
  port->partner_caps_.object = 0;
  port->is_pr_swap_ = FALSE;
  port->is_hard_reset_ = FALSE;
  port->policy_is_source_ = FALSE;
  port->registers_.MsgHeadr.USBPD_REV = USBPDSPECREV;
  port->registers_.MsgHeadr.POWER_ROLE = port->policy_is_source_;
  port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
  FUSB3601_WriteRegister(port, regMSGHEADR);
  FUSB3601_ResetProtocolLayer(port);

  /* If coming out of a hard reset, this will signal the Type-C state
   * machine to re-check the VBus level.
   */
  port->registers_.AlertL.I_PORT_PWR = 1;
  port->pd_tx_flag_ = FALSE;

  port->caps_counter_ = 0;
  port->collision_counter_ = 0;
  FUSB3601_TimerDisable(&port->policy_state_timer_);
  FUSB3601_set_policy_state(port, peSinkDiscovery);

#ifdef FSC_HAVE_VDM
  port->auto_mode_disc_tracker_ = 0;
  port->core_svid_info_.num_svids = 0;
  for (i = 0; i < MAX_NUM_SVIDS; i++) {
    port->core_svid_info_.svids[i] = 0;
  }
#endif /* FSC_HAVE_VDM */
#ifdef FSC_HAVE_DP
  port->display_port_data_.AutoModeEntryObjPos = -1;

#endif /* FSC_HAVE_DP */

//  port->registers_.PwrCtrl.DIS_VBUS_MON = 0;
//  WriteRegister(port, regPWRCTRL);

  //port->auto_vdm_state_ = AUTO_VDM_DONE;

  FUSB3601_disable_vbus_adc(port);
  /*
  if(port->double56k == TRUE) {
	  port->registers_.FMControl3.VOUT_SWITCH = 0b10;
	  WriteRegister(port, regFM_CONTROL3);
  }
*/
  FUSB3601_PolicySinkDiscovery(port);
}

void FUSB3601_PolicySinkSendHardReset(struct Port *port)
{
	FUSB3601_ProtocolSendHardReset(port);
}

void FUSB3601_PolicySinkSoftReset(struct Port *port)
{
	switch(port->policy_subindex_) {
	case 0:
		FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0, ktSinkWaitCap, peSinkSendHardReset, TRUE, SOP_TYPE_SOP);
		port->policy_subindex_++;
		port->auto_vdm_state_ = AUTO_VDM_INIT;
		break;
	default:
		if(port->pd_tx_status_ == txSuccess) {
			port->pd_tx_status_ = txIdle;
			FUSB3601_set_policy_state(port, peSinkWaitCaps);
			FUSB3601_PolicySinkWaitCaps(port);
		}
		break;
	}
}

void FUSB3601_PolicySinkSendSoftReset(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTSoftReset, 0, 0,
                            ktSenderResponse, peSinkSendHardReset, TRUE, SOP_TYPE_SOP);
      port->policy_subindex_++;
      port->auto_vdm_state_ = AUTO_VDM_INIT;
      break;
    default:
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if ((port->policy_rx_header_.NumDataObjects == 0) &&
            (port->policy_rx_header_.MessageType == CMTAccept)) {
          FUSB3601_set_policy_state(port, peSinkWaitCaps);
          FUSB3601_TimerStart(&port->policy_state_timer_, ktSinkWaitCap);
          FUSB3601_PolicySinkWaitCaps(port);
        }
        else {
		FUSB3601_ProtocolSendHardReset(port);
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
	  FUSB3601_ProtocolSendHardReset(port);
      }
      break;
  }
}

void FUSB3601_PolicySinkTransitionDefault(struct Port *port)
{
	switch (port->policy_subindex_) {
	case 0:
		FUSB3601_LogPEState(port);
		FUSB3601_enable_vbus_adc(port);
		port->hard_reset_counter_++;
		port->is_hard_reset_ = TRUE;
		port->policy_has_contract_ = FALSE;

		/* Disable auto-discharge state machine and enable VAlarm */
		port->registers_.PwrCtrl.AUTO_DISCH = 0;
		port->registers_.PwrCtrl.DIS_VALARM = 0;
		port->registers_.PwrCtrl.DIS_VBUS_MON = 0;

		/* Disable VConn source */
		if (port->is_vconn_source_) {
			port->registers_.PwrCtrl.EN_VCONN = 0;
			port->is_vconn_source_ = FALSE;
		}

		FUSB3601_WriteRegister(port, regPWRCTRL);

		/* Slow Rp-Rp Cable - Assumes RoleCtrl is DRP:0, CC1/CC2: Rd */
		FUSB3601_SendCommand(port, DisableSinkVbus);
		port->registers_.RoleCtrl.RP_VAL = 1;
		FUSB3601_WriteRegister(port, regROLECTRL);
		port->registers_.RoleCtrl.RP_VAL = 0;
		FUSB3601_WriteRegister(port, regROLECTRL);
		FUSB3601_SendCommand(port, SinkVbus);
		FUSB3601_TimerStart(&port->policy_state_timer_, ktPSHardResetMax + ktSafe0V);
		if (port->policy_is_dfp_) {
			port->policy_is_dfp_ = FALSE;
			port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
			FUSB3601_WriteRegister(port, regMSGHEADR);
		}
		/* Disable VBus sinking during the reset */
		//SendCommand(port, DisableSinkVbus);

		/* Set up alert to wait for vSafe0V */
		FUSB3601_SetVBusAlarm(port, FSC_VSAFE0V, FSC_VBUS_LVL_HIGHEST);
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_ALARM_LO_ALL);
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_ALARM_HI_ALL);
		port->registers_.AlertMskH.M_VBUS_ALRM_LO = 1;
		FUSB3601_WriteRegister(port, regALERTMSKH);
		port->policy_subindex_++;
		break;
	    case 1:
	      if (port->registers_.AlertH.I_VBUS_ALRM_LO || !FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE0V)) {
		  FUSB3601_LogPEState(port);
	        /* We've reached vSafe0V */
	        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO | MSK_I_VBUS_SNK_DISC);

	        /* Set up to wait for vSafe5V */
	        FUSB3601_SetVBusAlarm(port, 0, FSC_VSAFE5V_L);

	        FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI | MSK_I_PORT_PWR);
	        FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

	        port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
	        port->registers_.AlertMskL.M_VBUS_ALRM_HI = 1;
	        FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

	        FUSB3601_TimerStart(&port->policy_state_timer_, ktSrcRecoverMax + ktSrcTurnOn);

	        port->policy_subindex_++;
	      }
	      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
		  ////writepestate(&port->log_, 220, port->policy_state_);
	        /* TODO - check handling if we don't see VBUS change correctly */
		  port->registers_.PwrCtrl.AUTO_DISCH = 1;
		          port->registers_.PwrCtrl.DIS_VALARM = 1;
		          port->registers_.PwrCtrl.DIS_VBUS_MON = 1;
		          FUSB3601_WriteRegister(port, regPWRCTRL);
		          //SendCommand(port, SinkVbus);
		          FUSB3601_set_policy_state(port, peSinkStartup);
		          FUSB3601_PolicySinkStartup(port);
	      }
	      break;
	    case 2:
	      if (port->registers_.AlertL.I_VBUS_ALRM_HI || (FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE5V_L))) {
		  //LogPEState(port);
	        /* We've reached vSafe5V */
	        FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_PORT_PWR | MSK_I_VBUS_ALRM_HI);

	        /* Re-enable sinking VBus and discharge system */
	        //SendCommand(port, SinkVbus);

	        //SetVBusSnkDisc(port, FSC_VSAFE5V_DISC);

	        port->registers_.PwrCtrl.AUTO_DISCH = 1;
	        port->registers_.PwrCtrl.DIS_VALARM = 1;
	        port->registers_.PwrCtrl.DIS_VBUS_MON = 1;
	        FUSB3601_WriteRegister(port, regPWRCTRL);

	        port->pd_tx_status_ = txIdle;

	        FUSB3601_set_policy_state(port, peSinkStartup);
	        FUSB3601_PolicySinkStartup(port);

	      }
	      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
		  //writepestate(&port->log_, 221, port->policy_state_);
	        /* TODO - check handling if we don't see VBUS change correctly */
	        FUSB3601_set_policy_state(port, peErrorRecovery);
	        FUSB3601_TimerDisable(&port->policy_state_timer_);
	        FUSB3601_PolicyErrorRecovery(port);
	      }
	      break;
	    default:
		//writepestate(&port->log_, 222, port->policy_state_);
	      FUSB3601_set_policy_state(port, peErrorRecovery);
	      FUSB3601_PolicyErrorRecovery(port);
	      break;
	  }
}

void FUSB3601_PolicySinkDiscovery(struct Port *port)
{
  FUSB3601_set_policy_state(port, peSinkWaitCaps);
  FUSB3601_TimerStart(&port->policy_state_timer_, ktSinkWaitCap);
  FUSB3601_PolicySinkWaitCaps(port);
}

void FUSB3601_PolicySinkWaitCaps(struct Port *port)
{
	//writepestate(&port->log_, TimerRemaining(&port->policy_state_timer_)/1000 , port->policy_state_);
	//writepestate(&port->log_, TimerExpired(&port->policy_state_timer_) , port->policy_state_);
  if (port->protocol_msg_rx_) {
    port->protocol_msg_rx_ = FALSE;
    if ((port->policy_rx_header_.NumDataObjects > 0) &&
        (port->policy_rx_header_.MessageType == DMTSourceCapabilities)) {
      FUSB3601_UpdateCapabilitiesRx(port, TRUE);
      FUSB3601_set_policy_state(port, peSinkEvaluateCaps);
      FUSB3601_TimerDisable(&port->policy_state_timer_);
      FUSB3601_PolicySinkEvaluateCaps(port);
    }
    else if ((port->policy_rx_header_.NumDataObjects == 0) &&
             (port->policy_rx_header_.MessageType == CMTSoftReset)) {
      FUSB3601_set_policy_state(port, peSinkSoftReset);
      FUSB3601_PolicySinkSoftReset(port);
    }
  }
  else if (FUSB3601_TimerExpired(&port->policy_state_timer_) &&
           (port->hard_reset_counter_ <= HARD_RESET_COUNT)) {
	  FUSB3601_ProtocolSendHardReset(port);
  }
  else if ((port->policy_has_contract_ == FALSE) &&
           FUSB3601_TimerExpired(&port->policy_state_timer_) &&
           (port->hard_reset_counter_ > HARD_RESET_COUNT)) {
		if(FUSB3601_TimerExpired(&port->policy_state_timer_) &&
				(port->hard_reset_counter_ < 10)) {
			/* Slow Rp-Rp Cable - Assumes RoleCtrl is DRP:0, CC1/CC2: Rd */
			FUSB3601_SendCommand(port, DisableSinkVbus);
			port->registers_.RoleCtrl.RP_VAL = 1;
			FUSB3601_WriteRegister(port, regROLECTRL);
			port->registers_.RoleCtrl.RP_VAL = 0;
			FUSB3601_WriteRegister(port, regROLECTRL);
			FUSB3601_SendCommand(port, SinkVbus);

			FUSB3601_TimerStart(&port->policy_state_timer_, ktSinkWaitCap);
			port->hard_reset_counter_++;
		}
	}
}

void FUSB3601_PolicySinkEvaluateCaps(struct Port *port)
{
  FSC_U8 i = 0;
  FSC_S32 req_position = 0;
  FSC_U32 obj_voltage = 0;
  FSC_U32 obj_current = 0;
  FSC_U32 sel_voltage = 0;
  FSC_U32 max_power = 0;
  FSC_U32 obj_power = 0;
  FSC_U32 req_current = 0;
  FSC_U32 optional_max_power = 0;

  FUSB3601_TimerDisable(&port->policy_state_timer_);
  port->hard_reset_counter_ = 0;

  /* Select the highest power object that we are compatible with */
  for (i = 0; i < port->caps_header_received_.NumDataObjects; i++) {
    switch (port->caps_received_[i].PDO.SupplyType) {
      case pdoTypeFixed:
        obj_voltage = port->caps_received_[i].FPDOSupply.Voltage;
        if (obj_voltage > port->sink_request_max_voltage_) {
          continue;
        }
        else {
          obj_current = port->caps_received_[i].FPDOSupply.MaxCurrent;
          obj_power = obj_voltage * obj_current;
        }
        break;
      case pdoTypeVariable:
        obj_voltage = port->caps_received_[i].VPDO.MaxVoltage;
        if (obj_voltage > port->sink_request_max_voltage_) {
          continue;
        }
        else {
          obj_voltage = port->caps_received_[i].VPDO.MinVoltage;
          obj_current = port->caps_received_[i].VPDO.MaxCurrent;
          obj_power = obj_voltage * obj_current;
        }
        break;
      case pdoTypeBattery:
        /* Fall through to ignore battery powered sources (for now) */
      default:
        /* Ignore undefined/unsupported supply types */
        obj_power = 0;
        break;
    }

    if (obj_power > optional_max_power) {
        optional_max_power = obj_power;
    }
    if (obj_voltage > pd_limit_voltage) {
        continue;
    }

    /* Track object with highest power */
    if (obj_power >= max_power) {
      max_power = obj_power;
      sel_voltage = obj_voltage;
      req_position = i + 1;

      port->sink_partner_max_power_ = max_power;
    }
  }

  /* If another port is sinking the highest power available, we'll just */
  /* request a basic low power PDO here. */
  if (port->sink_request_low_power_) {
    if (port->caps_received_[0].PDO.SupplyType == pdoTypeFixed) {
      sel_voltage = port->caps_received_[0].FPDOSupply.Voltage;
    }
    else if (port->caps_received_[0].PDO.SupplyType == pdoTypeVariable){
      sel_voltage = port->caps_received_[0].VPDO.MaxVoltage;
    }
    else {
      /* Skipping battery sources for now... */
    }

    /* Make sure the first position is a 5V object */
    if (sel_voltage == PD_05_V) {
      req_position = 1;
    }
  }

  if ((req_position > 0) && (sel_voltage > 0)) {
    port->partner_caps_.object = port->caps_received_[0].object;
    port->sink_request_.FVRDO.ObjectPosition = req_position & 0x07;
    port->sink_request_.FVRDO.GiveBack = port->sink_goto_min_compatible_;
    port->sink_request_.FVRDO.NoUSBSuspend = port->sink_usb_suspend_compatible_;
    port->sink_request_.FVRDO.USBCommCapable = port->sink_usb_comm_capable_;
    req_current = port->sink_request_op_power_ / sel_voltage;
    /* Set the current based on the selected voltage (in 10mA units) */
    port->sink_request_.FVRDO.OpCurrent = (req_current & 0x3FF);
    req_current = port->sink_request_max_power_ / sel_voltage;
    /* Set the min/max current based on the selected voltage (in 10mA units) */
    port->sink_request_.FVRDO.MinMaxCurrent = (req_current & 0x3FF);
    if (port->sink_goto_min_compatible_) {
      port->sink_request_.FVRDO.CapabilityMismatch = FALSE;
    }
    else {
      if (obj_current < req_current) {
        /* Indicate that we need more power */
        port->sink_request_.FVRDO.CapabilityMismatch = TRUE;
        port->sink_request_.FVRDO.MinMaxCurrent = obj_current;
        port->sink_request_.FVRDO.OpCurrent = obj_current;
      }
      else {
        port->sink_request_.FVRDO.CapabilityMismatch = FALSE;
      }
    }

    if (optional_max_power >= PD_18_W) {
	 pd_dpm_set_optional_max_power_status(true);
    } else {
        pd_dpm_set_optional_max_power_status(false);
    }

    FUSB3601_set_policy_state(port, peSinkSelectCapability);
    FUSB3601_PolicySinkSelectCapability(port);
  }
  else {
    /* TODO: For now, we just go back to the wait state instead of */
    /* sending a reject or reset (may change in future) */
    FUSB3601_set_policy_state(port, peSinkWaitCaps);
    FUSB3601_TimerStart(&port->policy_state_timer_, ktTypeCSinkWaitCap);
    port->idle_ = TRUE;

    port->sink_partner_max_power_ = 0;
    FUSB3601_PolicySinkWaitCaps(port);
  }
}

//TODO: Why is the sender response timeout so long? 33ms
void FUSB3601_PolicySinkSelectCapability(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
	FUSB3601_ProtocolSendData(port, DMTRequest, 1, &port->sink_request_,
			ktSenderResponse, peSinkSendSoftReset, TRUE, SOP_TYPE_SOP);
	port->policy_subindex_++;
      break;
    case 1:
      if ((port->pd_tx_status_ = txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTAccept:
              port->policy_has_contract_ = TRUE;
              port->usb_pd_contract_.object = port->sink_request_.object;

              /* TODO - Not reliable if received caps have changed/cleared */
              port->sink_selected_voltage_ = port->caps_received_
          [port->usb_pd_contract_.FVRDO.ObjectPosition - 1].FPDOSupply.Voltage;

              FUSB3601_TimerStart(&port->policy_state_timer_, ktPSTransition);
              FUSB3601_set_policy_state(port, peSinkTransitionSink);
              FUSB3601_PolicySinkTransitionSink(port);
              break;
            case CMTWait:
            case CMTReject:
              if (port->policy_has_contract_) {
                FUSB3601_set_policy_state(port, peSinkReady);
                FUSB3601_PolicySinkReady(port);
              }
              else {
                FUSB3601_set_policy_state(port, peSinkWaitCaps);
                /* Set the counter to avoid a hard reset loop */
                port->hard_reset_counter_ = HARD_RESET_COUNT + 1;
                FUSB3601_PolicySinkWaitCaps(port);
              }
              break;
            case CMTSoftReset:
              FUSB3601_set_policy_state(port, peSinkSoftReset);
              FUSB3601_PolicySinkSoftReset(port);
              break;
            default:
              FUSB3601_set_policy_state(port, peSinkSendSoftReset);
              FUSB3601_PolicySinkSendSoftReset(port);
              break;
          }
        }
        else {
          switch (port->policy_rx_header_.MessageType) {
            case DMTSourceCapabilities:
              FUSB3601_UpdateCapabilitiesRx(port, TRUE);
              FUSB3601_set_policy_state(port, peSinkEvaluateCaps);
              FUSB3601_PolicySinkEvaluateCaps(port);
              break;
            default:
              FUSB3601_set_policy_state(port, peSinkSendSoftReset);
              FUSB3601_PolicySinkSendSoftReset(port);
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
	  FUSB3601_ProtocolSendHardReset(port);
      }
      break;
  }
}

void FUSB3601_PolicySinkTransitionSink(struct Port *port)
{
  if (port->protocol_msg_rx_) {
    port->protocol_msg_rx_ = FALSE;
    if (port->policy_rx_header_.NumDataObjects == 0) {
      switch (port->policy_rx_header_.MessageType) {
        case CMTPS_RDY:
          FUSB3601_ReadRegister(port, regSCP_ENABLE1);
          port->registers_.SCPEnable1.ENABLE_SCP = 0;
          FUSB3601_WriteRegister(port, regSCP_ENABLE1);
          FUSB3601_platform_notify_pd_contract(TRUE, port->sink_selected_voltage_,
			  port->sink_request_.FVRDO.OpCurrent, port->caps_received_[0].FPDOSupply.ExternallyPowered);

          FUSB3601_set_policy_state(port, peSinkReady);
          FUSB3601_PolicySinkReady(port);
          break;
        case CMTSoftReset:
          FUSB3601_set_policy_state(port, peSinkSoftReset);
          FUSB3601_PolicySinkSoftReset(port);
          break;
        default:
          FUSB3601_set_policy_state(port, peSinkSendSoftReset);
          FUSB3601_ProtocolSendHardReset(port);
          break;
      }
    }
    else {
      switch (port->policy_rx_header_.MessageType) {
        case DMTSourceCapabilities:
          FUSB3601_UpdateCapabilitiesRx(port, TRUE);
          FUSB3601_set_policy_state(port, peSinkEvaluateCaps);
          FUSB3601_PolicySinkEvaluateCaps(port);
          break;
        default:
          /* Unexpected data message */
          FUSB3601_set_policy_state(port, peSinkSendSoftReset);
          FUSB3601_ProtocolSendHardReset(port);
          break;
      }
    }
    port->pd_tx_status_ = txIdle;
  }
  else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
	  FUSB3601_ProtocolSendHardReset(port);
  }
}

void FUSB3601_PolicySinkReady(struct Port *port)
{
	/* Handle expired VDM timer */
  if (FUSB3601_TimerExpired(&port->vdm_timer_)) {
	FUSB3601_ResetPolicyState(port);
	FUSB3601_TimerDisable(&port->vdm_timer_);
  }
  /* Pick up any leftover successful sends */
  if(port->pd_tx_status_ == txSuccess) port->pd_tx_status_ = txIdle;

  if (port->protocol_msg_rx_) {
    port->protocol_msg_rx_ = FALSE;
    if (port->policy_rx_header_.NumDataObjects == 0) {
      switch (port->policy_rx_header_.MessageType) {
        case CMTGotoMin:
          FUSB3601_set_policy_state(port, peSinkTransitionSink);
          FUSB3601_TimerStart(&port->policy_state_timer_, ktPSTransition);
          FUSB3601_PolicySinkTransitionSink(port);
          break;
        case CMTGetSinkCap:
          FUSB3601_set_policy_state(port, peSinkGiveSinkCap);
          FUSB3601_PolicySinkGiveSinkCap(port);
          break;
        case CMTGetSourceCap:
          FUSB3601_set_policy_state(port, peSinkGiveSourceCap);
          FUSB3601_PolicySinkGiveSourceCap(port);
          break;
        case CMTDR_Swap:
          FUSB3601_set_policy_state(port, peSinkEvaluateDRSwap);
          FUSB3601_PolicySinkEvaluateDRSwap(port);
          break;
        case CMTPR_Swap:
          FUSB3601_set_policy_state(port, peSinkEvaluatePRSwap);
          FUSB3601_PolicySinkEvaluatePRSwap(port);
          break;
        case CMTVCONN_Swap:
          FUSB3601_set_policy_state(port, peSinkEvaluateVCONNSwap);
          FUSB3601_PolicySinkEvaluateVCONNSwap(port);
          break;
        case CMTSoftReset:
          FUSB3601_set_policy_state(port, peSinkSoftReset);
          FUSB3601_PolicySinkSoftReset(port);
          break;
        default:
          break;
      }
    }
    else {
      switch (port->policy_rx_header_.MessageType) {
        case DMTSourceCapabilities:
          FUSB3601_UpdateCapabilitiesRx(port, TRUE);
          FUSB3601_set_policy_state(port, peSinkEvaluateCaps);
          FUSB3601_PolicySinkEvaluateCaps(port);
          break;
#ifdef FSC_HAVE_VDM
        case DMTVendorDefined:
		FUSB3601_ProcessVdmMessage(port);
          break;
#endif /* FSC_HAVE_VDM */
        case DMTBIST:
          FUSB3601_ProcessDmtBist(port);
          break;
        default:
          /* Ignore unexpected messages */
          break;
      }
    }
  }
  else if (port->pd_tx_flag_) {
	port->pd_tx_flag_ = FALSE;
    if (port->pd_transmit_header_.NumDataObjects == 0) {
      switch (port->pd_transmit_header_.MessageType) {
        case CMTGetSourceCap:
          FUSB3601_set_policy_state(port, peSinkGetSourceCap);
          FUSB3601_PolicySinkGetSourceCap(port);
          break;
        case CMTGetSinkCap:
          FUSB3601_set_policy_state(port, peSinkGetSinkCap);
          FUSB3601_PolicySinkGetSinkCap(port);
          break;
        case CMTDR_Swap:
          FUSB3601_set_policy_state(port, peSinkSendDRSwap);
          FUSB3601_PolicySinkSendDRSwap(port);
          break;
#ifdef FSC_HAVE_DRP
        case CMTPR_Swap:
          FUSB3601_set_policy_state(port, peSinkSendPRSwap);
          FUSB3601_PolicySinkSendPRSwap(port);
          break;
#endif /* FSC_HAVE_DRP */
        case CMTSoftReset:
          FUSB3601_set_policy_state(port, peSinkSendSoftReset);
          FUSB3601_PolicySinkSendSoftReset(port);
          break;
        default:
          break;
      }
    }
    else {
      switch (port->pd_transmit_header_.MessageType) {
        case DMTRequest:
          port->sink_request_.object = port->pd_transmit_objects_[0].object;
          FUSB3601_set_policy_state(port, peSinkSelectCapability);
          FUSB3601_TimerStart(&port->policy_state_timer_, ktSenderResponse);
          FUSB3601_PolicySinkSelectCapability(port);
          break;
        case DMTVendorDefined:
#ifdef FSC_HAVE_VDM
#endif /* FSC_HAVE_VDM */
          break;
        default:
          break;
      }
    }
  }
#ifdef FSC_HAVE_VDM
  else if (port->policy_is_dfp_ && port->auto_vdm_state_ != AUTO_VDM_DONE
		  && port->expecting_vdm_response_ == FALSE) {
    FUSB3601_AutoVdmDiscovery(port);
  }
#endif /* FSC_HAVE_VDM */
  else if (port->renegotiate_) {
    port->renegotiate_ = FALSE;
    FUSB3601_set_policy_state(port, peSinkEvaluateCaps);
    FUSB3601_PolicySinkEvaluateCaps(port);
  }
}

void FUSB3601_PolicySinkGiveSinkCap(struct Port *port)
{
	  switch (port->policy_subindex_) {
	    case 0:
		  FUSB3601_ProtocolSendData(port, DMTSinkCapabilities,
						 port->caps_header_sink_.NumDataObjects,
						 port->caps_sink_, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
		  port->policy_subindex_++;
		  break;
	    case 1:
		if(port->pd_tx_status_ == txSuccess) {
			port->pd_tx_status_ = txIdle;
			FUSB3601_set_policy_state(port, peSinkReady);
			FUSB3601_PolicySinkReady(port);
		}
		break;
	  }
}

void FUSB3601_PolicySinkGetSourceCap(struct Port *port)
{


  switch (port->policy_subindex_) {
    case 0:
	  FUSB3601_ProtocolSendData(port, CMTGetSourceCap, 0, 0, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
	  port->policy_subindex_++;
	  break;
    case 1:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_set_policy_state(port, peSinkReady);
		FUSB3601_PolicySinkReady(port);
	}
	break;
  }
}

void FUSB3601_PolicySinkGetSinkCap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
	  FUSB3601_ProtocolSendData(port, CMTGetSinkCap, 0, 0, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
	  port->policy_subindex_++;
	  break;
    case 1:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_set_policy_state(port, peSinkReady);
		FUSB3601_PolicySinkReady(port);
	}
	break;
  }
}

void FUSB3601_PolicySinkGiveSourceCap(struct Port *port)
{

  switch (port->policy_subindex_) {
    case 0:
	#ifdef FSC_HAVE_DRP
	  if (port->port_type_ == USBTypeC_DRP)
		FUSB3601_ProtocolSendData(port, DMTSourceCapabilities,
					   port->caps_header_source_.NumDataObjects,
					   port->caps_source_, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
	  else
	#endif /* FSC_HAVE_DRP */
		FUSB3601_ProtocolSendData(port, CMTReject, 0, 0, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
	  port->policy_subindex_++;
	  break;
    case 1:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_set_policy_state(port, peSinkReady);
		FUSB3601_PolicySinkReady(port);
	}
	break;
  }
}

void FUSB3601_PolicySinkSendDRSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTDR_Swap, 0, 0,
                                 ktSenderResponse, peErrorRecovery, FALSE, SOP_TYPE_SOP);
      port->policy_subindex_++;
    default:
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTAccept:
              port->policy_is_dfp_ =
                  (port->policy_is_dfp_ == TRUE) ? FALSE : TRUE;
              port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
              FUSB3601_WriteRegister(port, regMSGHEADR);
              FUSB3601_set_policy_state(port, peSinkReady);
              FUSB3601_PolicySinkReady(port);
	      port->auto_vdm_state_ = AUTO_VDM_INIT;
              break;
            case CMTSoftReset:
              FUSB3601_set_policy_state(port, peSinkSoftReset);
              FUSB3601_PolicySinkSoftReset(port);
              break;
            default:
              FUSB3601_set_policy_state(port, peSinkReady);
              FUSB3601_PolicySinkReady(port);
              break;
          }
        }
        else {
          FUSB3601_set_policy_state(port, peSinkReady);
          FUSB3601_PolicySinkReady(port);
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peSinkReady);
        FUSB3601_PolicySinkReady(port);
      }
      break;
  }
}

void FUSB3601_PolicySinkEvaluateDRSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
		#ifdef FSC_HAVE_VDM
		  if (port->mode_entered_ == TRUE) {
			  FUSB3601_ProtocolSendHardReset(port);
			return;
		  }
		#endif /* FSC_HAVE_VDM */
	FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0,
	                             0, peErrorRecovery, FALSE, SOP_TYPE_SOP);
	  port->policy_subindex_++;
	  break;
    case 1:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
	    port->policy_is_dfp_ = (port->policy_is_dfp_ == TRUE) ? FALSE : TRUE;
	    port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
	    FUSB3601_WriteRegister(port, regMSGHEADR);
		FUSB3601_set_policy_state(port, peSinkReady);
		FUSB3601_PolicySinkReady(port);
		port->auto_vdm_state_ = AUTO_VDM_INIT;
		FUSB3601_platform_notify_data_role(port->policy_is_dfp_);
	}
	break;
  }
}

void FUSB3601_PolicySinkEvaluateVCONNSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0,
                        0, peSinkSendSoftReset, TRUE, SOP_TYPE_SOP);
      port->policy_subindex_++;
      break;
    case 1:
    	if(port->pd_tx_status_ == txSuccess) {
    		port->pd_tx_status_ = txIdle;
			  if (port->is_vconn_source_) {
				FUSB3601_TimerStart(&port->policy_state_timer_, ktVCONNSourceOn);
				port->policy_subindex_++;
			  }
			  else {
				/* Apply VConn */
				port->registers_.PwrCtrl.EN_VCONN = 1;
				FUSB3601_WriteRegister(port, regPWRCTRL);
				FUSB3601_platform_set_vconn(TRUE); // denny added for try
				port->is_vconn_source_ = TRUE;
				FUSB3601_TimerStart(&port->policy_state_timer_, ktVCONNOnDelay);
				port->policy_subindex_ = 3;
			  }
	}
      break;
    case 2:
      if (port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTPS_RDY:
              /* Disable VConn source */
              port->registers_.PwrCtrl.EN_VCONN = 0;
              FUSB3601_WriteRegister(port, regPWRCTRL);
			  FUSB3601_platform_set_vconn(FALSE);// denny added for try
              port->is_vconn_source_ = FALSE;

              FUSB3601_set_policy_state(port, peSinkReady);
              port->pd_tx_status_ = txIdle;
              break;
            default:
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        port->pd_tx_status_ = txIdle;
        FUSB3601_ProtocolSendHardReset(port);
      }
      break;
    case 3:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0, 0, peSinkSendSoftReset, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
      }
      break;
    default:
    	if(port->pd_tx_status_ == txSuccess) {
    		port->pd_tx_status_ = txIdle;
    		FUSB3601_set_policy_state(port, peSinkReady);
    		FUSB3601_PolicySinkReady(port);
    	}
    	break;
  }
}

void FUSB3601_PolicySinkSendPRSwap(struct Port *port)
{
#ifdef FSC_HAVE_DRP
  switch (port->policy_subindex_) {
    case 0:
      FUSB3601_ProtocolSendData(port, CMTPR_Swap, 0, 0,
                            ktSenderResponse, peSinkSendSoftReset, TRUE, SOP_TYPE_SOP);
      port->policy_subindex_++;
      break;
    case 1:
      /* Require Accept message to move on or go back to ready state */
      if ((port->pd_tx_status_ == txSuccess) && port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTAccept:
              port->is_pr_swap_ = TRUE;
              port->policy_has_contract_ = FALSE;

              port->registers_.PwrCtrl.AUTO_DISCH = 0;
              FUSB3601_WriteRegister(port, regPWRCTRL);

              FUSB3601_SendCommand(port, DisableSinkVbus);

              FUSB3601_TimerStart(&port->policy_state_timer_, ktPSSourceOff);
              port->policy_subindex_++;
              break;
            case CMTWait:
            case CMTReject:
              FUSB3601_set_policy_state(port, peSinkReady);
              port->is_pr_swap_ = FALSE;
              FUSB3601_PolicySinkReady(port);
              break;
            default:
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peSinkReady);
        port->is_pr_swap_ = FALSE;
        FUSB3601_PolicySinkReady(port);
      }
      break;
    case 2:
      /* Wait for a PS_RDY message to be received to indicate that the */
      /* original source is no longer supplying VBUS */
      if (port->protocol_msg_rx_) {
        port->protocol_msg_rx_ = FALSE;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTPS_RDY:
              port->policy_is_source_ = TRUE;

              FUSB3601_RoleSwapToAttachedSource(port);

              port->registers_.MsgHeadr.POWER_ROLE = port->policy_is_source_;
              FUSB3601_WriteRegister(port, regMSGHEADR);
              FUSB3601_TimerStart(&port->policy_state_timer_, 0);
              port->policy_subindex_++;
              break;
            default:
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peErrorRecovery);
        FUSB3601_PolicyErrorRecovery(port);
      }
      break;
    case 3:
      if (FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE5V_L)) {
        FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0,
                                   ktSwapSourceStart, peErrorRecovery, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
      }
        else {
          //port->registers_.PwrCtrl.AUTO_DISCH = 1;
          //WriteRegister(port, regPWRCTRL);
        }
      break;
  default:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;
		FUSB3601_set_policy_state(port, peSourceStartup);
		FUSB3601_PolicySourceStartupHelper(port);
		FUSB3601_PolicySourceStartup(port);
	}
	  break;
  }
#endif /* FSC_HAVE_DRP */
}

void FUSB3601_PolicySinkEvaluatePRSwap(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      if (port->protocol_msg_rx_sop_ != SOP_TYPE_SOP ||
          (port->partner_caps_.FPDOSupply.SupplyType == pdoTypeFixed &&
           port->partner_caps_.FPDOSupply.DualRolePower == FALSE)) {
        FUSB3601_ProtocolSendData(port, CMTReject, 0, 0,
                          0, peSinkSendSoftReset, TRUE, port->protocol_msg_rx_sop_);
        FUSB3601_set_policy_state(port, peSinkReady);
      }
      else {
	  FUSB3601_ProtocolSendData(port, CMTAccept, 0, 0,
                                 ktPSSourceOff, peSinkSendSoftReset, TRUE, SOP_TYPE_SOP);
	  FUSB3601_enable_vbus_adc(port);
        port->is_pr_swap_ = TRUE;
        port->policy_has_contract_ = FALSE;

        port->registers_.PwrCtrl.AUTO_DISCH = 0;
        FUSB3601_WriteRegister(port, regPWRCTRL);

        FUSB3601_SendCommand(port, DisableSinkVbus);
        FUSB3601_platform_set_vbus_output(FALSE);
        port->policy_subindex_++;
      }
      break;
    case 1:
      if ((port->protocol_msg_rx_) && (port->pd_tx_status_ == txSuccess)){
        port->protocol_msg_rx_ = FALSE;
        port->pd_tx_status_ = txIdle;
        if (port->policy_rx_header_.NumDataObjects == 0) {
          switch (port->policy_rx_header_.MessageType) {
            case CMTPS_RDY:
              port->policy_is_source_ = TRUE;

              /* Set up to wait for vSafe5V for PD */
		FUSB3601_SetVBusAlarm(port, 0, FSC_VSAFE5V_L);

		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI | MSK_I_PORT_PWR);
		FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);

		port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
		port->registers_.AlertMskL.M_VBUS_ALRM_HI = 1;
		FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

              FUSB3601_RoleSwapToAttachedSource(port);

              port->registers_.MsgHeadr.POWER_ROLE = port->policy_is_source_;
              FUSB3601_WriteRegister(port, regMSGHEADR);
              FUSB3601_TimerStart(&port->policy_state_timer_, ktSrcRecoverMax);


              port->policy_subindex_++;
              break;
            default:
              break;
          }
        }
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peErrorRecovery);
        port->pd_tx_status_ = txIdle;
        FUSB3601_PolicyErrorRecovery(port);
      }
      break;
    case 2:
      if (FUSB3601_IsVbusOverVoltage(port, FSC_VSAFE5V_L)) {

	 FUSB3601_ReadRegister(port, regCCSTAT);
	 FUSB3601_DetectCCPin(port);

	  port->registers_.AlertMskH.M_VBUS_ALRM_LO = 0;
			port->registers_.AlertMskL.M_VBUS_ALRM_HI = 0;
			FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

		  FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_VBUS_ALRM_HI | MSK_I_PORT_PWR);
			FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_ALRM_LO);


        FUSB3601_ProtocolSendData(port, CMTPS_RDY, 0, 0, ktSwapSourceStart,
                                   peErrorRecovery, TRUE, SOP_TYPE_SOP);
        port->policy_subindex_++;
      }
      else if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        FUSB3601_set_policy_state(port, peErrorRecovery);
        port->pd_tx_status_ = txIdle;
        FUSB3601_PolicyErrorRecovery(port);
      }
      break;
    default:
	if(port->pd_tx_status_ == txSuccess) {
		port->pd_tx_status_ = txIdle;

		port->is_pr_swap_ = FALSE;

//			  port->registers_.PwrCtrl.AUTO_DISCH = 1;
//			  WriteRegister(port, regPWRCTRL);
//			  port->registers_.PwrCtrl.AUTO_DISCH = 0;
//			  WriteRegister(port, regPWRCTRL);
		FUSB3601_set_policy_state(port, peSourceStartup);
		FUSB3601_PolicySourceStartupHelper(port);
		FUSB3601_PolicySourceStartup(port);
	}
	break;
  }
}

#endif /* FSC_HAVE_SNK */
void FUSB3601_UpdateCapabilitiesRx(struct Port *port, FSC_BOOL is_source_cap_update)
{
  FSC_U8 i = 0;

#ifdef FSC_HAVE_USBHID
  /* Set the source caps updated flag to trigger an update of the GUI */
  port->source_caps_updated_ = is_source_cap_update;
#endif /* FSC_HAVE_USBHID */

  port->caps_header_received_.word = port->policy_rx_header_.word;
  for (i = 0; i < port->caps_header_received_.NumDataObjects; i++) {
    port->caps_received_[i].object =
        port->policy_rx_data_obj_[i].object;
  }

  for (i = port->caps_header_received_.NumDataObjects; i < 7; i++) {
    port->caps_received_[i].object = 0;
  }
  port->partner_caps_.object = port->caps_received_[0].object;
}

/* ---------------- VDM ---------------- */
void FUSB3601_PolicyDfpCblVdmIdentityRequest(struct Port *port)
{
	switch (port->policy_subindex_) {
		case 0:
			FUSB3601_platform_log(port->port_id_, "FUSB DiscID SOP' SubState: ", 0);
			FUSB3601_PolicySendVDM(port, DISCOVER_IDENTITY, 0xFF00, SOP_TYPE_SOP1, 0);
			port->policy_subindex_++;
			break;
		default:
		if((port->pd_tx_status_== txSuccess)&&(port->protocol_msg_rx_||FUSB3601_TimerExpired(&port->policy_state_timer_))){
				FUSB3601_platform_log(port->port_id_, "FUSB DiscID SOP' SubState 1 TxSuccess: ", port->protocol_msg_rx_);
				port->protocol_msg_rx_ = FALSE;
				port->pd_tx_status_ = txIdle;
				FUSB3601_TimerDisable(&port->vdm_timer_);

				FUSB3601_set_policy_state(port, peSourceSendCaps);
				FUSB3601_PolicySourceSendCaps(port);
			}
			/*
			else if((port->pd_tx_status_ == txIdle)) {
				FUSB3601_platform_log(port->port_id_, "FUSB DiscID SOP' SubState 1 TxFail: ", -1);
				FUSB3601_set_policy_state(port, peSourceSendCaps);
				FUSB3601_PolicySourceSendCaps(port);
			}
			*/
			break;
	}
}

/* ---------------- BIST Functionality ---------------- */
void FUSB3601_ProcessDmtBist(struct Port *port)
{
  FSC_U8 bdo = port->policy_rx_data_obj_[0].byte[3] >> 4;
  switch (bdo) {
    case BDO_BIST_Carrier_Mode_2:
      if (port->caps_source_[port->usb_pd_contract_.FVRDO.ObjectPosition - 1]
          .FPDOSupply.Voltage == PD_05_V) {
        FUSB3601_set_policy_state(port, PE_BIST_Carrier_Mode_2);
        port->protocol_state_ = PRLIdle;
        FUSB3601_PolicyBISTCarrierMode2(port);
      }
      break;
    case BDO_BIST_Test_Data: /* Fall through */
    default:
      /* Mask everything but HARDRST and VBUSOK */
      if (port->caps_source_[port->usb_pd_contract_.FVRDO.ObjectPosition - 1]
          .FPDOSupply.Voltage == PD_05_V) {
        port->registers_.TcpcCtrl.BIST_TMODE = 1;
        FUSB3601_WriteRegister(port, regTCPC_CTRL);

        FUSB3601_set_policy_state(port, PE_BIST_Test_Data);
        port->protocol_state_ = PRLDisabled;
      }
      break;
  }
}

void FUSB3601_PolicyBISTReceiveMode(struct Port *port)
{
  /* Nothing needed here. */
}

void FUSB3601_PolicyBISTFrameReceived(struct Port *port)
{
  /* Nothing needed here. */
}

//TODO: Exit state transitions
void FUSB3601_PolicyBISTCarrierMode2(struct Port *port)
{
  switch (port->policy_subindex_) {
    case 0:
      port->registers_.Transmit.TX_SOP = TRANSMIT_BIST_CM2;
      FUSB3601_WriteRegister(port, regTRANSMIT);
      FUSB3601_TimerStart(&port->policy_state_timer_, ktBISTContMode);
      FUSB3601_SendCommand(port, DisableSinkVbus);
      port->policy_subindex_=2;
      break;
    case 1:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        /* Delay for >200us to allow preamble to finish */
        FUSB3601_TimerStart(&port->policy_state_timer_, ktGoodCRCDelay);
        port->policy_subindex_++;
      }
      break;
    case 2:
      if (FUSB3601_TimerExpired(&port->policy_state_timer_)) {
        if (port->policy_is_source_) {
#ifdef FSC_HAVE_SRC
          FUSB3601_set_policy_state(port, peSourceReady);
#endif /* FSC_HAVE_SRC */
        }
        else {
#ifdef FSC_HAVE_SNK
          FUSB3601_SendCommand(port, SinkVbus);
          FUSB3601_set_policy_state(port, peSinkReady);
#endif /* FSC_HAVE_SNK */
        }
      }
      break;
  }
}

void FUSB3601_PolicyBISTTestData(struct Port *port)
{
  /* Nothing needed here.  Wait for detach or reset to end this mode. */
}

void FUSB3601_PolicyInvalidState(struct Port *port)
{
  /* reset if we get to an invalid state */
	FUSB3601_ProtocolSendHardReset(port);
}
