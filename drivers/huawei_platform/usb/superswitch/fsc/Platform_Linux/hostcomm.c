#ifdef FSC_DEBUG

#include "fusb3601_global.h"
#include "FSCTypes.h"
#include "platform_helpers.h"
#include "../core/core.h"
#include "../core/PDTypes.h"
#include "../core/TypeCTypes.h"
#include "hostcomm.h"

#ifdef FSC_HAVE_VDM
#include "../core/vdm.h"
#ifdef FSC_HAVE_DP
#include "../core/display_port.h"
#endif /* FSC_HAVE_DP */
#endif /* FSC_HAVE_VDM */

void FUSB3601_ProcessTCPDStatus(FSC_U8 *inMsgBuffer, FSC_U8 *outMsgBuffer,
                       struct Port *port);
void FUSB3601_ProcessTCPDControl(FSC_U8 *inMsgBuffer, FSC_U8 *outMsgBuffer,
                        struct Port *port);
void FUSB3601_ProcessTCSetState(FSC_U8 *inMsgBuffer, struct Port *port);
void FUSB3601_SendUSBPDMessage(FSC_U8 *data, struct Port *port);
void FUSB3601_ReadSourceCapabilities(FSC_U8 *data, struct Port *port);
#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
void FUSB3601_WriteSourceCapabilities(FSC_U8 *data, struct Port *port);
#endif /* FSC_HAVE_SRC || (FSC_HAVE_SNK && FSC_HAVE_ACC) */
void FUSB3601_ReadSinkCapabilities(FSC_U8 *data, struct Port *port);
void FUSB3601_WriteSinkCapabilities(FSC_U8 *data, struct Port *port);
void FUSB3601_ReadSinkRequestSettings(FSC_U8 *data, struct Port *port);
void FUSB3601_WriteSinkRequestSettings(FSC_U8 *data, struct Port *port);
void FUSB3601_ConfigurePortType(FSC_U8 Control, struct Port *port);
void FUSB3601_ConfigurePortType_WithoutUnattach(FSC_U8 Control, struct Port *port);


void FUSB3601_fusb_ProcessMsg(struct Port *port,
                     FSC_U8* inMsgBuffer, FSC_U8* outMsgBuffer)
{
    FSC_U8 i;

    /* Echo the request */
    outMsgBuffer[0] = inMsgBuffer[0];
    outMsgBuffer[1] = 0x00; /* Return command was recognized */

    switch (inMsgBuffer[0])
    {
        case CMD_GETDEVICEINFO:
            if (inMsgBuffer[1] != 0)
            {
                outMsgBuffer[1] = 0x01; /* Return command not recognized */
            }
            else
            {
                outMsgBuffer[1] = 0x00; /* Return command was recognized */
                outMsgBuffer[4] = MY_MCU;
                outMsgBuffer[5] = MY_DEV_TYPE;

                outMsgBuffer[6] = FUSB3601_core_get_rev_lower();
                outMsgBuffer[7] = FUSB3601_core_get_rev_upper();

                outMsgBuffer[8] = 0xFF & MY_BC;
                outMsgBuffer[9] = 0xFF & (MY_BC >> 8);

                for (i = 0; i < 16; i++)
                {
                    outMsgBuffer[i + 10] = 0x00;
                }

                outMsgBuffer[26] = TEST_FIRMWARE;
            }
            break;
#ifdef FSC_LOGGING
        case CMD_USBPD_BUFFER_READ:
            FUSB3601_ReadPDLog(&port->log_, &outMsgBuffer[4],
                FSC_HOSTCOMM_BUFFER_SIZE-6);
            break;
#endif /* FSC_LOGGING */
        case CMD_USBPD_STATUS:
            FUSB3601_ProcessTCPDStatus(inMsgBuffer, outMsgBuffer, port);
            break;
        case CMD_USBPD_CONTROL:
            FUSB3601_ProcessTCPDControl(inMsgBuffer, outMsgBuffer, port);
            break;
#ifdef FSC_HAVE_SRC
        case CMD_GET_SRC_CAPS:
            FUSB3601_ReadSourceCapabilities(outMsgBuffer, port);
            break;
#endif /* FSC_HAVE_SRC */
#ifdef FSC_HAVE_SNK
        case CMD_GET_SINK_CAPS:
            FUSB3601_ReadSinkCapabilities(outMsgBuffer, port);
            break;
        case CMD_GET_SINK_REQ:
            FUSB3601_ReadSinkRequestSettings(outMsgBuffer, port);
            break;
#endif /* FSC_HAVE_SNK */
        case CMD_ENABLE_PD:
            port->pd_enabled_ = TRUE;
            break;
        case CMD_DISABLE_PD:
            port->pd_enabled_ = FALSE;
            break;
        case CMD_GET_ALT_MODES:
            //outMsgBuffer[1] = core_get_alternate_modes();
            break;
        case CMD_GET_MANUAL_RETRIES:
            //outMsgBuffer[1] = core_get_manual_retries();
            break;
        case CMD_SET_STATE_UNATTACHED:
            //core_set_state_unattached(port);
            break;
        case CMD_ENABLE_TYPEC_SM:
            port->tc_enabled_ = TRUE;
            break;
        case CMD_DISABLE_TYPEC_SM:
            port->tc_enabled_ = FALSE;
            break;
        case CMD_DEVICE_LOCAL_REGISTER_READ:
          //core_process_local_register_request(port,inMsgBuffer, outMsgBuffer);
            break;
        case CMD_SET_STATE:
            FUSB3601_ProcessTCSetState(inMsgBuffer, port);
            break;
#ifdef FSC_LOGGING
        case CMD_READ_STATE_LOG:
            FUSB3601_ReadTCLog(&port->log_, &outMsgBuffer[3],
                FSC_HOSTCOMM_BUFFER_SIZE-3);
            break;
        case CMD_READ_PD_STATE_LOG:
            FUSB3601_ReadPELog(&port->log_, &outMsgBuffer[3],
                FSC_HOSTCOMM_BUFFER_SIZE-3);
            break;
#endif /* FSC_LOGGING */
        case CMD_READ_I2C:
            FUSB3601_fusb_hc_Handle_I2CRead(inMsgBuffer, outMsgBuffer);
            break;
        case CMD_WRITE_I2C:
            FUSB3601_fusb_hc_Handle_I2CWrite(inMsgBuffer, outMsgBuffer);
            break;
        case CMD_GET_VBUS5V:
            FUSB3601_fusb_hc_GetVBus5V(outMsgBuffer);
            break;
        case CMD_SET_VBUS5V:
            FUSB3601_fusb_hc_SetVBus5V(inMsgBuffer, outMsgBuffer);
            break;
        case CMD_GET_INTN:
            FUSB3601_fusb_hc_GetIntN(outMsgBuffer);
            break;
        case CMD_SEND_HARD_RESET:
            //core_send_hard_reset(port);
            break;
#ifdef FSC_DEBUG
        case CMD_GET_TIMER_TICKS:
            FUSB3601_fusb_hc_GetTimerTicks(outMsgBuffer);
            break;
        case CMD_GET_SM_TICKS:
            FUSB3601_fusb_hc_GetSMTicks(outMsgBuffer);
            break;
        case CMD_GET_GPIO_SM_TOGGLE:
            FUSB3601_fusb_hc_GetGPIO_SM_Toggle(outMsgBuffer);
            break;
        case CMD_SET_GPIO_SM_TOGGLE:
            FUSB3601_fusb_hc_SetGPIO_SM_Toggle(inMsgBuffer, outMsgBuffer);
            break;
#endif  /* FSC_DEBUG */
        default:
            outMsgBuffer[1] = 0x01; /* Request is not implemented */
            break;
    }
}

void FUSB3601_fusb_hc_Handle_I2CRead(FSC_U8* inBuf, FSC_U8* outBuf)
{
    if (!FUSB3601_fusb_I2C_ReadData(inBuf[1], outBuf))
    {
        pr_err("FUSB  %s - Error: Could not read I2C Data!\n", __func__);
    }
}

void FUSB3601_fusb_hc_Handle_I2CWrite(FSC_U8* inBuf, FSC_U8* outBuf)
{
    if (!FUSB3601_fusb_I2C_WriteData(inBuf[1], inBuf[2], &inBuf[3]))
    {
        pr_err("FUSB  %s - Error: Could not write I2C Data!\n", __func__);
        outBuf[0] = 0;  /* Notify failure */
    }
    else
    {
        outBuf[0] = 1;  /* Notify success */
    }
}

void FUSB3601_fusb_hc_GetVBus5V(FSC_U8* outMsgBuffer)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        outMsgBuffer[0] = 0;
        return;
    }

    outMsgBuffer[0] = 1;
    //outMsgBuffer[1] = fusb_GPIO_Get_VBus5v() ? 1 : 0;
}

void FUSB3601_fusb_hc_SetVBus5V(FSC_U8* inMsgBuffer, FSC_U8* outMsgBuffer)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        outMsgBuffer[0] = 0;
        return;
    }

    FUSB3601_fusb_GPIO_Set_VBus5v(inMsgBuffer[1]);
    outMsgBuffer[0] = 1;
    //outMsgBuffer[1] = fusb_GPIO_Get_VBus5v() ? 1 : 0;
}

void FUSB3601_fusb_hc_GetIntN(FSC_U8* outMsgBuffer)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        outMsgBuffer[0] = 0;
        return;
    }

    outMsgBuffer[0] = 1;
    outMsgBuffer[1] = FUSB3601_fusb_GPIO_Get_IntN() ? 1 : 0;
}

void FUSB3601_fusb_hc_GetTimerTicks(FSC_U8* outMsgBuffer)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        outMsgBuffer[0] = 0;
        return;
    }

    outMsgBuffer[0] = 1; /* Success */
    outMsgBuffer[1] = chip->dbgTimerTicks;
    outMsgBuffer[2] = chip->dbgTimerRollovers;
}

void FUSB3601_fusb_hc_GetSMTicks(FSC_U8* outMsgBuffer)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        outMsgBuffer[0] = 0;
        return;
    }

    outMsgBuffer[0] = 1; /* Success */
    outMsgBuffer[1] = chip->dbgSMTicks;
    outMsgBuffer[2] = chip->dbgSMRollovers;
}

void FUSB3601_fusb_hc_GetGPIO_SM_Toggle(FSC_U8* outMsgBuffer)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        outMsgBuffer[0] = 0;
        return;
    }

    outMsgBuffer[0] = 1;
    outMsgBuffer[1] = FUSB3601_dbg_fusb_GPIO_Get_SM_Toggle() ? 1 : 0;
}

void FUSB3601_fusb_hc_SetGPIO_SM_Toggle(FSC_U8* inMsgBuffer, FSC_U8* outMsgBuffer)
{
    struct fusb3601_chip* chip = fusb3601_GetChip();
    if (!chip)
    {
        pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
        outMsgBuffer[0] = 0;
        return;
    }

    FUSB3601_dbg_fusb_GPIO_Set_SM_Toggle(inMsgBuffer[1]);

    outMsgBuffer[0] = 1; /* Success */
    outMsgBuffer[1] = FUSB3601_dbg_fusb_GPIO_Get_SM_Toggle() ? 1 : 0;
}

void FUSB3601_ProcessTCPDStatus(FSC_U8 *inMsgBuffer, FSC_U8 *outMsgBuffer,
                       struct Port *port)
{
  FSC_U8 i = 0, j = 0;

  /* TypeC Status at outMsgBuffer[4] */
  FSC_U8 status = 0;
  FSC_U32 intIndex = 8;

  /* Port Type - Sink:0b00, Source:0b01, DRP:0b10 */
  switch (port->port_type_) {
  case USBTypeC_Source:
    status |= 0x01;
    break;
  case USBTypeC_DRP:
    status |= 0x02;
    break;
  default:
    break;
  }

  if (port->acc_support_)
    status |= 0x04;

  status |= (port->src_current_ << 4);

  if (port->src_preferred_)
    status |= 0x08;

  if (port->snk_preferred_)
    status |= 0x40;

  if (port->tc_enabled_)
    status |= 0x80;

  outMsgBuffer[4] = status;
  outMsgBuffer[5] = port->tc_state_;

//  if (port->cc_pin_ == CC1)
//    outMsgBuffer[6] = (port->cc_term_previous_ & 0x07) +
//                      ((port->vconn_term_ & 0x07) << 4);
//  else if (port->cc_pin_ == CC2)
//    outMsgBuffer[6] = (port->vconn_term_ & 0x07) +
//                      ((port->cc_term_previous_ & 0x07) << 4);

  outMsgBuffer[7] = port->snk_current_;

  /* PD Status at outMsgBuffer[8] */
  status = 0;
  if (port->pd_enabled_) status |= 0x01;
  if (port->pd_active_)  status |= 0x02;
  if (port->policy_is_source_) status |= 0x04;
  if (port->policy_is_dfp_) status |= 0x08;
  if (port->policy_has_contract_) status |= 0x10;
  if (port->source_caps_updated_) status |= 0x20;

#ifdef FSC_LOGGING
  if (FUSB3601_get_pd_overflow(&port->log_)) status |= 0x80;
#endif /* FSC_LOGGING */

  port->source_caps_updated_ = FALSE;

  outMsgBuffer[intIndex++] = status;

#ifdef FSC_LOGGING
  outMsgBuffer[intIndex++] = FUSB3601_get_pd_bytes(&port->log_);
#endif /* FSC_LOGGING */

  outMsgBuffer[intIndex++] = port->policy_state_;
  outMsgBuffer[intIndex++] = port->policy_subindex_;
  outMsgBuffer[intIndex++] = (port->protocol_state_ << 4) | port->pd_tx_status_;

  /* Current Contract */
  for (i = 0; i < 4; i++) {
    outMsgBuffer[intIndex++] = port->usb_pd_contract_.byte[i];
  }

  /* Source capabilities */
  if (port->policy_is_source_) {
    outMsgBuffer[intIndex++] = port->caps_header_source_.byte[0];
    outMsgBuffer[intIndex++] = port->caps_header_source_.byte[1];

    for (i = 0; i < 7; i++) {
      for (j = 0; j < 4; j++) {
        outMsgBuffer[intIndex++] = port->caps_source_[i].byte[j];
      }
    }
  }
  /* Sink capabilities */
  else {
    outMsgBuffer[intIndex++] = port->caps_header_received_.byte[0];
    outMsgBuffer[intIndex++] = port->caps_header_received_.byte[1];

    for (i = 0; i < 7; i++) {
      for (j = 0; j < 4; j++) {
        outMsgBuffer[intIndex++] = port->caps_received_[i].byte[j];
      }
    }
  }
}
void FUSB3601_ConfigurePortType(FSC_U8 control,struct Port *port)
{
	FSC_U8 value;
	FSC_BOOL setUnattached = FALSE;
	value = control & 0x03;

	if (port->port_type_ != value) {
		switch (value) {
		case 1:
#ifdef FSC_HAVE_SRC
			port->port_type_= USBTypeC_Source;
#endif /* FSC_HAVE_SRC */
			break;
		case 2:
#ifdef FSC_HAVE_DRP
			port->port_type_ = USBTypeC_DRP;
#endif /* FSC_HAVE_DRP */
			break;
		default:
#ifdef FSC_HAVE_SNK
			port->port_type_ = USBTypeC_Sink;
#endif /* FSC_HAVE_SNK */
			break;
		}
		setUnattached = TRUE;
	}
#ifdef FSC_HAVE_ACC
	if (((control & 0x04) >> 2) != port->acc_support_) {
		port->acc_support_ = control & 0x04 ? TRUE : FALSE;
		setUnattached = TRUE;
	}
#endif /* FSC_HAVE_ACC */
#ifdef FSC_HAVE_DRP
	if ((control & 0x08) && !port->src_preferred_) {
		port->src_preferred_ = TRUE;
		setUnattached = TRUE;
	} else if (!(control & 0x08) && port->src_preferred_) {
		port->src_preferred_ = FALSE;
		setUnattached = TRUE;
	}

	if ((control & 0x40) && !port->snk_preferred_) {
		port->snk_preferred_ = TRUE;
		setUnattached = TRUE;
	} else if (!(control & 0x40) && port->snk_preferred_) {
		port->snk_preferred_ = FALSE;
		setUnattached = TRUE;
	}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_SRC
	value = (control & 0x30) >> 4;
	if (port->src_current_ != value) {
		switch (value) {
		case 1:
			port->src_current_ = utccDefault;
			break;
		case 2:
			port->src_current_ = utcc1p5A;
			break;
		case 3:
			port->src_current_ = utcc3p0A;
			break;
		default:
			port->src_current_ = utccOpen;
			break;
		}
		FUSB3601_UpdateSourceCurrent(port, port->src_current_);
	}
#endif /* FSC_HAVE_SRC */

	if(setUnattached)
		FUSB3601_SetStateUnattached(port);

	if (control & 0x80) {
		port->tc_enabled_ = TRUE;
	}
}
void FUSB3601_ConfigurePortType_WithoutUnattach(FSC_U8 control,struct Port *port)
{
	FSC_U8 value;
	value = control & 0x03;
	pr_info("%s +++ \n",__func__);

	if (port->port_type_ != value) {
		switch (value) {
		case 1:
#ifdef FSC_HAVE_SRC
			port->port_type_= USBTypeC_Source;
#endif /* FSC_HAVE_SRC */
			break;
		case 2:
#ifdef FSC_HAVE_DRP
			port->port_type_ = USBTypeC_DRP;
#endif /* FSC_HAVE_DRP */
			break;
		default:
#ifdef FSC_HAVE_SNK
			port->port_type_ = USBTypeC_Sink;
#endif /* FSC_HAVE_SNK */
			break;
		}
	}
#ifdef FSC_HAVE_ACC
	if (((control & 0x04) >> 2) != port->acc_support_) {
		port->acc_support_ = control & 0x04 ? TRUE : FALSE;
	}
#endif /* FSC_HAVE_ACC */
#ifdef FSC_HAVE_DRP
	if ((control & 0x08) && !port->src_preferred_) {
		port->src_preferred_ = TRUE;
	} else if (!(control & 0x08) && port->src_preferred_) {
		port->src_preferred_ = FALSE;
	}

	if ((control & 0x40) && !port->snk_preferred_) {
		port->snk_preferred_ = TRUE;
	} else if (!(control & 0x40) && port->snk_preferred_) {
		port->snk_preferred_ = FALSE;
	}
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_SRC
	value = (control & 0x30) >> 4;
	if (port->src_current_ != value) {
		switch (value) {
		case 1:
			port->src_current_ = utccDefault;
			break;
		case 2:
			port->src_current_ = utcc1p5A;
			break;
		case 3:
			port->src_current_ = utcc3p0A;
			break;
		default:
			port->src_current_ = utccOpen;
			break;
		}
		FUSB3601_UpdateSourceCurrent(port, port->src_current_);
	}
#endif /* FSC_HAVE_SRC */

	if (control & 0x80) {
		port->tc_enabled_ = TRUE;
	}

	pr_info("%s --- \n",__func__);
}
void FUSB3601_ProcessTCPDControl(FSC_U8 *inMsgBuffer, FSC_U8 *outMsgBuffer,
                        struct Port *port)
{
  FSC_U8 control = 0;
  FSC_U8 value = 0;
  FSC_BOOL setUnattached = FALSE;

  /* Echo back the actual command */
  outMsgBuffer[4] = inMsgBuffer[4];

  /* Decode command */
  switch (inMsgBuffer[4]) {
  case 0x01:                              /* Reset the state machine */
    /* TODO - this doesn't actually do anything??? */
    /* TODO - probably check and change this to setstateunattached(port) */
    port->tc_enabled_ = FALSE;
    port->tc_enabled_ = TRUE;
    break;
  case 0x02:                              /* Disable state machine */
    port->tc_enabled_ = FALSE;
    break;
  case 0x03:                              /* Enable state machine */
    port->tc_enabled_ = TRUE;
    break;
  case 0x04:                              /* Configure port type */
    control = inMsgBuffer[5];
    setUnattached = FALSE;

    port->tc_enabled_ = FALSE;

    value = control & 0x03;

    if (port->port_type_ != value) {
      switch (value) {
      case 1:
#ifdef FSC_HAVE_SRC
        port->port_type_ = USBTypeC_Source;
#endif /* FSC_HAVE_SRC */
        break;
      case 2:
#ifdef FSC_HAVE_DRP
        port->port_type_ = USBTypeC_DRP;
#endif /* FSC_HAVE_DRP */
        break;
      default:
#ifdef FSC_HAVE_SNK
        port->port_type_ = USBTypeC_Sink;
#endif /* FSC_HAVE_SNK */
        break;
      }

      setUnattached = TRUE;
    }

#ifdef FSC_HAVE_ACC
    if (((control & 0x04) >> 2) != port->acc_support_) {
      port->acc_support_ = control & 0x04 ? TRUE : FALSE;
      setUnattached = TRUE;
    }
#endif /* FSC_HAVE_ACC */

#ifdef FSC_HAVE_DRP
    if ((control & 0x08) && !port->src_preferred_) {
      port->src_preferred_ = TRUE;
      setUnattached = TRUE;
    }
    else if (!(control & 0x08) && port->src_preferred_) {
      port->src_preferred_ = FALSE;
      setUnattached = TRUE;
    }

    if ((control & 0x40) && !port->snk_preferred_) {
      port->snk_preferred_ = TRUE;
      setUnattached = TRUE;
    }
    else if (!(control & 0x40) && port->snk_preferred_) {
      port->snk_preferred_ = FALSE;
      setUnattached = TRUE;
    }
#endif /* FSC_HAVE_DRP */

#ifdef FSC_HAVE_SRC
    value = (control & 0x30) >> 4;
    if (port->src_current_ != value) {
      switch (value) {
      case 1:
        port->src_current_ = utccDefault;
        break;
      case 2:
        port->src_current_ = utcc1p5A;
        break;
      case 3:
        port->src_current_ = utcc3p0A;
        break;
      default:
        port->src_current_ = utccOpen;
        break;
      }

      FUSB3601_UpdateSourceCurrent(port, port->src_current_);
    }
#endif /* FSC_HAVE_SRC */

    if(setUnattached) FUSB3601_SetStateUnattached(port);

    if (control & 0x80) {
      port->tc_enabled_ = TRUE;
    }
    break;
  case 0x05:                              /* Update current advertisement */
    value = inMsgBuffer[5];

    switch (value) {
    case 1:
      port->src_current_ = utccDefault;
      break;
    case 2:
      port->src_current_ = utcc1p5A;
      break;
    case 3:
      port->src_current_ = utcc3p0A;
      break;
    default:
      port->src_current_ = utccOpen;
      break;
    }

    FUSB3601_UpdateSourceCurrent(port, port->src_current_);
    break;
  case 0x06:                              /* Enable USB PD */
    port->pd_enabled_ = TRUE;
    break;
  case 0x07:                              /* Disable USB PD */
    port->pd_enabled_ = FALSE;
    break;
  case 0x08:                              /* Send USB PD Command/Message */
    port->policy_msg_tx_sop_ = FUSB3601_DecodeSopFromPdMsg(inMsgBuffer[6]);
    FUSB3601_SendUSBPDMessage(&inMsgBuffer[5], port);
    break;
#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
  case 0x09:                              /* Update the source capabilities */
    FUSB3601_WriteSourceCapabilities(&inMsgBuffer[5], port);
    break;
#endif /* FSC_HAVE_SRC || (FSC_HAVE_SNK && FSC_HAVE_ACC) */
  case 0x0A:                              /* Read the source capabilities */
    FUSB3601_ReadSourceCapabilities(&outMsgBuffer[5], port);
    break;
#ifdef FSC_HAVE_SNK
  case 0x0B:                              /* Update the sink capabilities */
    FUSB3601_WriteSinkCapabilities(&inMsgBuffer[5], port);
    break;
  case 0x0C:                              /* Read the sink capabilities */
    FUSB3601_ReadSinkCapabilities(&outMsgBuffer[5], port);
    break;
  case 0x0D:                              /* Update the default sink settings */
    FUSB3601_WriteSinkRequestSettings(&inMsgBuffer[5], port);
    break;
  case 0x0E:                              /* Read the default sink settings */
    FUSB3601_ReadSinkRequestSettings(&outMsgBuffer[5], port);
    break;
#endif /* FSC_HAVE_SNK */
  case 0x0F:                              /* Send USB PD Hard Reset */
    if (port->policy_is_source_)
      port->policy_state_ = peSourceSendHardReset;
    else
      port->policy_state_ = peSinkSendHardReset;

    port->policy_subindex_ = 0;
    port->pd_tx_status_ = txIdle;
    break;

#ifdef FSC_HAVE_VDM
  case 0x10:                              /* Configure SVIDs/Modes */
    port->policy_msg_tx_sop_  = FUSB3601_DecodeSopFromPdMsg(inMsgBuffer[6]);
    FUSB3601_ConfigureVdmResponses(port, &inMsgBuffer[5]);
    break;
  case 0x11:                              /* Read SVIDs/Modes */
    port->policy_msg_tx_sop_ = FUSB3601_DecodeSopFromPdMsg(inMsgBuffer[6]);
    FUSB3601_ReadVdmConfiguration(port, &outMsgBuffer[5]);
    break;
#endif /* FSC_HAVE_VDM */

#ifdef FSC_HAVE_DP
  case 0x12:
    FUSB3601_WriteDpControls(port, &inMsgBuffer[5]);
    break;
  case 0x13:
    FUSB3601_ReadDpControls(port, &outMsgBuffer[5]);
    break;
  case 0x14:
    FUSB3601_ReadDpStatus(port, &outMsgBuffer[5]);
    break;
#endif /* FSC_HAVE_DP */
  default:
    break;
  }
}

void FUSB3601_ProcessTCSetState(FSC_U8 *inMsgBuffer, struct Port *port)
{
  TypeCState state = (TypeCState)inMsgBuffer[3];

  switch(state) {
    case(Disabled):
      FUSB3601_SetStateDisabled(port);
      break;
    case(ErrorRecovery):
      FUSB3601_SetStateErrorRecovery(port);
      break;
    case(Unattached):
      FUSB3601_SetStateUnattached(port);
      break;
#ifdef FSC_HAVE_SNK
    case(AttachWaitSink):
      FUSB3601_SetStateAttachWaitSink(port);
      break;
    case(AttachedSink):
      FUSB3601_SetStateAttachedSink(port);
      break;
#ifdef FSC_HAVE_DRP
    case(TryWaitSink):
      FUSB3601_SetStateTryWaitSink(port);
      break;
    case(TrySink):
      FUSB3601_SetStateTrySink(port);
      break;
#endif /* FSC_HAVE_DRP */
#endif /* FSC_HAVE_SNK */
#ifdef FSC_HAVE_SRC
    case(AttachWaitSource):
      FUSB3601_SetStateAttachWaitSource(port);
      break;
    case(AttachedSource):
      FUSB3601_SetStateAttachedSource(port);
      break;
#ifdef FSC_HAVE_DRP
    case(TrySource):
      FUSB3601_SetStateTrySource(port);
      break;
    case(TryWaitSource):
      FUSB3601_SetStateTryWaitSource(port);
      break;
#endif /* FSC_HAVE_DRP */
#endif /* FSC_HAVE_SRC */
#ifdef FSC_HAVE_ACC
    case(AudioAccessory):
      FUSB3601_SetStateAudioAccessory(port);
      break;
//    case(DebugAccessory):
//      SetStateDebugAccessory(port);
//      break;
    case(AttachWaitAccessory):
      FUSB3601_SetStateAttachWaitAccessory(port);
      break;
    case(PoweredAccessory):
      FUSB3601_SetStatePoweredAccessory(port);
      break;
    case(UnsupportedAccessory):
      FUSB3601_SetStateUnsupportedAccessory(port);
      break;
#endif /* FSC_HAVE_ACC */
    case(DelayUnattached):
      FUSB3601_SetStateDelayUnattached(port);
      break;
    default:
      FUSB3601_SetStateDelayUnattached(port);
      break;
  }
}

void FUSB3601_SendUSBPDMessage(FSC_U8 *data, struct Port *port)
{
  FSC_U32 i = 0, j = 0;

  /* 2 header bytes */
  port->pd_transmit_header_.byte[0] = *data++;
  port->pd_transmit_header_.byte[1] = *data++;

  /* Data objects */
  for (i = 0; i < port->pd_transmit_header_.NumDataObjects; ++i) {
    for (j = 0; j < 4; ++j) {
      port->pd_transmit_objects_[i].byte[j] = *data++;
    }
  }

  port->pd_tx_flag_ = TRUE;
}

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
void FUSB3601_WriteSourceCapabilities(FSC_U8 *data, struct Port *port)
{
  FSC_U32 i = 0, j = 0;
  sopMainHeader_t header = {0};

  header.byte[0] = *data++;
  header.byte[1] = *data++;

  /* Only do anything if we decoded a source capabilities message */
  if ((header.NumDataObjects > 0) &&
      (header.MessageType == DMTSourceCapabilities)) {
    port->caps_header_source_.word = header.word;

    for (i = 0; i < port->caps_header_source_.NumDataObjects; i++) {
      for (j = 0; j < 4; j++) {
          port->caps_source_[i].byte[j] = *data++;
      }
    }

    if (port->policy_is_source_) {
      port->pd_transmit_header_.word = port->caps_header_source_.word;
      port->pd_tx_flag_ = TRUE;
      port->source_caps_updated_ = TRUE;
    }
  }
}

#endif /* FSC_HAVE_SRC */

void FUSB3601_ReadSourceCapabilities(FSC_U8 *data, struct Port *port)
{
  FSC_U32 i = 0, j = 0;
  FSC_U32 index = 0;

  data[index++] = port->caps_header_source_.byte[0];
  data[index++] = port->caps_header_source_.byte[1];

  for (i = 0; i < port->caps_header_source_.NumDataObjects; i++) {
    for (j = 0; j < 4; j++) {
      data[index++] = port->caps_source_[i].byte[j];
    }
  }
}

#ifdef FSC_HAVE_SNK
void FUSB3601_WriteSinkCapabilities(FSC_U8 *data, struct Port *port)
{
  FSC_U32 i = 0, j = 0;
  sopMainHeader_t header = {0};

  header.byte[0] = *data++;
  header.byte[1] = *data++;

  /* Only do anything if we decoded a sink capabilities message */
  if ((header.NumDataObjects > 0) &&
      (header.MessageType == DMTSinkCapabilities)) {
    port->caps_header_sink_.word = header.word;
    for (i = 0; i < port->caps_header_sink_.NumDataObjects; i++) {
      for (j = 0; j < 4; j++) {
        port->caps_sink_[i].byte[j] = *data++;
      }
    }

    /* We could also trigger sending the caps or re-evaluating,
     * but we don't do anything with this info here...
     */
  }
}

void FUSB3601_WriteSinkRequestSettings(FSC_U8 *data, struct Port *port)
{
  FSC_U32 value;

  port->sink_goto_min_compatible_ = (*data & 0x01 ? TRUE : FALSE);
  port->sink_usb_suspend_compatible_ = (*data & 0x02 ? TRUE : FALSE);
  port->sink_usb_comm_capable_ = (*data++ & 0x04 ? TRUE : FALSE);

  /* Voltage resolution is 50mV */
  value = (FSC_U32) *data++;
  value |= ((FSC_U32) (*data++) << 8);
  port->sink_request_max_voltage_ = value;

  /* Power resolution is 0.5mW */
  value = (FSC_U32) *data++;
  value |= ((FSC_U32) (*data++) << 8);
  value |= ((FSC_U32) (*data++) << 16);
  value |= ((FSC_U32) (*data++) << 24);
  port->sink_request_op_power_ = value;

  /* Power resolution is 0.5mW */
  value = (FSC_U32) *data++;
  value |= ((FSC_U32) (*data++) << 8);
  value |= ((FSC_U32) (*data++) << 16);
  value |= ((FSC_U32) (*data++) << 24);
  port->sink_request_max_power_ = value;

  /* We could try resetting and re-evaluating the source caps here, but lets
   * not do anything until requested by the user (soft reset or detach)
   */
}

void FUSB3601_ReadSinkRequestSettings(FSC_U8 *data, struct Port *port)
{
  *data = port->sink_goto_min_compatible_ ? 0x01 : 0;
  *data |= port->sink_usb_suspend_compatible_ ? 0x02 : 0;
  *data++ |= port->sink_usb_comm_capable_ ? 0x04 : 0;

  *data++ = (FSC_U8) (port->sink_request_max_voltage_ & 0xFF);
  *data++ = (FSC_U8) ((port->sink_request_max_voltage_ >> 8) & 0xFF);

  *data++ = (FSC_U8) (port->sink_request_op_power_ & 0xFF);
  *data++ = (FSC_U8) ((port->sink_request_op_power_ >> 8) & 0xFF);
  *data++ = (FSC_U8) ((port->sink_request_op_power_ >> 16) & 0xFF);
  *data++ = (FSC_U8) ((port->sink_request_op_power_ >> 24) & 0xFF);

  *data++ = (FSC_U8) (port->sink_request_max_power_ & 0xFF);
  *data++ = (FSC_U8) ((port->sink_request_max_power_ >> 8) & 0xFF);
  *data++ = (FSC_U8) ((port->sink_request_max_power_ >> 16) & 0xFF);
  *data++ = (FSC_U8) ((port->sink_request_max_power_ >> 24) & 0xFF);
}
#endif /* FSC_HAVE_SNK */

void FUSB3601_ReadSinkCapabilities(FSC_U8 *data, struct Port *port)
{
  FSC_U32 i = 0, j = 0;
  FSC_U32 index = 0;

  data[index++] = port->caps_header_sink_.byte[0];
  data[index++] = port->caps_header_sink_.byte[1];

  for (i = 0; i < port->caps_header_sink_.NumDataObjects; i++) {
    for (j = 0; j < 4; j++) {
      data[index++] = port->caps_sink_[i].byte[j];
    }
  }
}

void FUSB3601_tx_pd_cmd_msg(struct Port *port, FSC_U8 message_type) {
	FSC_U8 data[2];
	sopMainHeader_t tx_header;

	tx_header.MessageType = message_type;
	tx_header.NumDataObjects = 0;

	data[0] = tx_header.byte[0];
	data[1] = tx_header.byte[1];

	FUSB3601_SendUSBPDMessage(data, port);
}

#endif  /* FSC_DEBUG */
