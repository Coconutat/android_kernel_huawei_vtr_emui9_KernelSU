/*
 *  display_port.cpp
 *
 *  The display port implementation.
 */

#ifdef FSC_HAVE_DP

#include "display_port_types.h"
#include "display_port.h"
#include "PDTypes.h"
#include "port.h"
#include "vdm.h"
#include "vdm_types.h"
#include "protocol.h"

/* System DP */
#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/hisi/contexthub/tca.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/dp_aux_switch/dp_aux_switch.h>

#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();
#define IN_FUNCTION hwlog_info("%s ++\n", __func__);
#define OUT_FUNCTION hwlog_info("%s --\n", __func__);
static TCPC_MUX_CTRL_TYPE g_mux_type = TCPC_DP;

#ifdef CONFIG_CONTEXTHUB_PD
extern void dp_aux_switch_op(uint32_t value);
extern void dp_aux_uart_switch_enable(void);
extern int pd_dpm_handle_combphy_event(struct pd_dpm_combphy_event event);
extern void pd_dpm_set_last_hpd_status(bool hpd_status);
#endif
void pd_dpm_send_event(enum pd_dpm_cable_event_type event);


#define MODE_DP_SNK 0x1
#define MODE_DP_SRC 0x2
#define MODE_DP_BOTH 0x3

#define MODE_DP_PIN_A 0x01
#define MODE_DP_PIN_B 0x02
#define MODE_DP_PIN_C 0x04
#define MODE_DP_PIN_D 0x08
#define MODE_DP_PIN_E 0x10
#define MODE_DP_PIN_F 0x20
/* *********** */

//TODO: Update DP to use data objects, not U32
void FUSB3601_requestDpStatus(struct Port *port)
{
  doDataObject_t svdmh = {0};
  FSC_U32 length = 0;
  FSC_U32 arr[2] = {0};
  doDataObject_t temp[2] = {{0}};
  IN_FUNCTION
  svdmh.SVDM.SVID = DP_SID;
  svdmh.SVDM.VDMType = STRUCTURED_VDM;
  svdmh.SVDM.Version = STRUCTURED_VDM_VERSION;
  /* saved mode position */
  svdmh.SVDM.ObjPos = port->display_port_data_.DpModeEntered & 0x7;
  svdmh.SVDM.CommandType = INITIATOR;
  svdmh.SVDM.Command = DP_COMMAND_STATUS;
  arr[0] = svdmh.object;
  length++;
  arr[1] = port->display_port_data_.DpStatus.word;
  length++;

  temp[0].object = arr[0];
    temp[1].object = arr[1];
  //SendVdmMessageWithTimeout(port, SOP_TYPE_SOP, arr, length,
//                            port->policy_state_);
	port->expecting_vdm_response_ = TRUE;
	FUSB3601_ProtocolSendData(port, DMTVendorDefined, length, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_requestDpConfig(struct Port *port)
{
  doDataObject_t svdmh = {0};
  FSC_U32 length = 0;
  FSC_U32 arr[2] = {0};
  doDataObject_t temp[2] = {{0}};
  IN_FUNCTION
  FUSB3601_platform_log(port->port_id_, "FUSB Request DP Config, DPCaps: ", port->display_port_data_.DpCaps.word);
  FUSB3601_platform_log(port->port_id_, "FUSB Request DP Config, DpPpStatus", port->display_port_data_.DpPpStatus.word);

	/* Huawei Config */
    port->display_port_data_.DpPpRequestedConfig.Conf = DP_CONF_UFP_D;
    port->display_port_data_.DpPpRequestedConfig.SigConf = DP_CONF_SIG_DP_V1P3;
    port->display_port_data_.DpPpRequestedConfig.Rsvd = 0;
    port->display_port_data_.DpPpRequestedConfig.DfpPa = 0;
    port->display_port_data_.DpPpRequestedConfig.UfpPa = 0;
    if (port->display_port_data_.DpPpStatus.MultiFunctionPreferred) {
	    if (port->display_port_data_.DpCaps.ReceptacleIndication) {
		if(port->display_port_data_.DpCaps.UFP_DPinAssignD) {
		    hwlog_info("Receptacle PinAssignD\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_D;
		}
		else if(port->display_port_data_.DpCaps.UFP_DPinAssignC) {
		    hwlog_info("Receptacle PinAssignC\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
		}
		else if(port->display_port_data_.DpCaps.UFP_DPinAssignE) {
		    hwlog_info("Receptacle PinAssignE\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
		}
		else {
		    hwlog_info("Fail Caps Check\n");
		    FUSB3601_platform_log(port->port_id_, "FUSB Request DP Config, Fail Caps Check", -1);
		    return;
		}
	    } else {
		if(port->display_port_data_.DpCaps.DFP_DPinAssignD) {
		    hwlog_info("plug PinAssignD\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_D;
		}
		else if(port->display_port_data_.DpCaps.DFP_DPinAssignC) {
		    hwlog_info("plug PinAssignC\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
		}
		else if(port->display_port_data_.DpCaps.DFP_DPinAssignF) {
		    hwlog_info("plug PinAssignF\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_F;
		}
		else if(port->display_port_data_.DpCaps.DFP_DPinAssignE) {
		    hwlog_info("plug PinAssignE\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
		}
		else {
			hwlog_info("Fail Caps Check\n");
			FUSB3601_platform_log(port->port_id_, "FUSB Request DP Config, Fail Caps Check", -1);
			return;
		}
	    }
    } else {
	    if (port->display_port_data_.DpCaps.ReceptacleIndication) {
		if(port->display_port_data_.DpCaps.UFP_DPinAssignC) {
		    hwlog_info("Receptacle PinAssignC with MF not preferred\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
		} else if(port->display_port_data_.DpCaps.UFP_DPinAssignE) {
		    hwlog_info("Receptacle PinAssignE with MF not preferred\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
		} else {
		    hwlog_info("Fail Caps Check\n");
		    FUSB3601_platform_log(port->port_id_, "FUSB Request DP Config, Fail Caps Check", -1);
		    return;
		}
	    } else {
		if(port->display_port_data_.DpCaps.DFP_DPinAssignC) {
		    hwlog_info("plug PinAssignC with MF not preferred\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
		} else if(port->display_port_data_.DpCaps.DFP_DPinAssignE) {
		    hwlog_info("plug PinAssignE with MF not preferred\n");
		    port->display_port_data_.DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
		} else {
			hwlog_info("Fail Caps Check\n");
			FUSB3601_platform_log(port->port_id_, "FUSB Request DP Config, Fail Caps Check", -1);
			return;
		}
	    }
    }
    if(port->display_port_data_.DpPpStatus.Connection == DP_CONN_NEITHER) {
        hwlog_info("CONN_NEITHER Fail Caps Check\n");
        FUSB3601_platform_log(port->port_id_, "FUSB Request DP Config, Fail Status Check", port->display_port_data_.DpPpStatus.word);
        return;
    }

  svdmh.SVDM.SVID = DP_SID;
  svdmh.SVDM.VDMType = STRUCTURED_VDM;
  svdmh.SVDM.Version = STRUCTURED_VDM_VERSION;
  svdmh.SVDM.ObjPos = port->display_port_data_.DpModeEntered & 0x7;
  svdmh.SVDM.CommandType = INITIATOR;
  svdmh.SVDM.Command = DP_COMMAND_CONFIG;
  arr[0] = svdmh.object;
  length++;
  arr[1] = port->display_port_data_.DpPpRequestedConfig.word;
  length++;

  temp[0].object = arr[0];
    temp[1].object = arr[1];
  //SendVdmMessage(port, SOP_TYPE_SOP, arr, length, port->policy_state_);
  port->expecting_vdm_response_ = TRUE;
  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_WriteDpControls(struct Port *port, FSC_U8* data)
{
  IN_FUNCTION
  FSC_BOOL en = FALSE;
  FSC_BOOL ame_en = FALSE;
  FSC_U32 m = 0;
  FSC_U32 v = 0;
  FSC_U32 stat = 0;
  en = *data++ ? TRUE : FALSE;
  stat = (FSC_U32)(*data++ );
  stat |= ((FSC_U32)(*data++ ) << 8);
  stat |= ((FSC_U32)(*data++ ) << 16);
  stat |= ((FSC_U32)(*data++ ) << 24);
  ame_en = *data++ ? TRUE : FALSE;
  m = (FSC_U32)(*data++ );
  m |= ((FSC_U32)(*data++ ) << 8);
  m |= ((FSC_U32)(*data++ ) << 16);
  m |= ((FSC_U32)(*data++ ) << 24);
  v = (FSC_U32)(*data++ );
  v |= ((FSC_U32)(*data++ ) << 8);
  v |= ((FSC_U32)(*data++ ) << 16);
  v |= ((FSC_U32)(*data++ ) << 24);
  FUSB3601_configDp(port, en, stat);
  FUSB3601_configAutoDpModeEntry(port, ame_en, m, v);
}

void FUSB3601_ReadDpControls(struct Port *port, FSC_U8* data)
{
  IN_FUNCTION
  *data++ = (FSC_U8)(port->display_port_data_.DpEnabled);
  *data++ = (FSC_U8)(port->display_port_data_.DpStatus.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpStatus.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpStatus.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpStatus.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpAutoModeEntryEnabled);
  *data++ = (FSC_U8)(port->display_port_data_.DpModeEntryMask.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryMask.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryMask.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryMask.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpModeEntryValue.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryValue.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryValue.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpModeEntryValue.word >> 24) & 0xFF);
}

void FUSB3601_ReadDpStatus(struct Port *port, FSC_U8* data)
{
  IN_FUNCTION
  *data++ = (FSC_U8)(port->display_port_data_.DpConfig.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpConfig.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpConfig.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpConfig.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpPpStatus.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpStatus.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpStatus.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpStatus.word >> 24) & 0xFF);
  *data++ = (FSC_U8)(port->display_port_data_.DpPpConfig.word);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpConfig.word >> 8) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpConfig.word >> 16) & 0xFF);
  *data++ = (FSC_U8)((port->display_port_data_.DpPpConfig.word >> 24) & 0xFF);
}

void FUSB3601_resetDp(struct Port *port)
{
  IN_FUNCTION
  port->display_port_data_.DpStatus.word = 0x0;
  port->display_port_data_.DpStatus.Connection = DP_CONN_DFP_D;

  port->display_port_data_.DpConfig.word = 0x0;
  port->display_port_data_.DpPpStatus.word = 0x0;
  port->display_port_data_.DpPpRequestedConfig.word = 0x0;
  port->display_port_data_.DpPpConfig.word = 0x0;
  port->display_port_data_.DpModeEntered = 0x0;
}

FSC_BOOL FUSB3601_processDpCommand(struct Port *port, FSC_U32* arr_in)
{
  	doDataObject_t svdmh_in = {0};
	DisplayPortStatus_t stat;
	DisplayPortConfig_t config;
	IN_FUNCTION

	if (port->display_port_data_.DpEnabled == FALSE) return TRUE;

	svdmh_in.object = arr_in[0];
	switch (svdmh_in.SVDM.Command) {
		case DP_COMMAND_STATUS:
			if (svdmh_in.SVDM.CommandType == INITIATOR) {
				if (port->display_port_data_.DpModeEntered == FALSE) return TRUE;
				stat.word = arr_in[1];
				FUSB3601_informStatus(port, stat);
				FUSB3601_updateStatusData(port); /*  get updated info from system */
				FUSB3601_sendStatusData(port, svdmh_in); /*  send it out */
			}
			else {
				stat.word = arr_in[1];
				FUSB3601_informStatus(port, stat);
				if(port->auto_vdm_state_ == AUTO_VDM_DP_GET_STATUS) {
					port->auto_vdm_state_ = AUTO_VDM_DP_SET_CONFIG;
					FUSB3601_requestDpConfig(port);
				}
			}
			break;
		case DP_COMMAND_CONFIG:
			if (svdmh_in.SVDM.CommandType == INITIATOR) {
				if (port->display_port_data_.DpModeEntered == FALSE) return TRUE;
				config.word = arr_in[1];
				if (FUSB3601_DpReconfigure(port, config) == TRUE) {
					/* if pin reconfig is successful */
					FUSB3601_replyToConfig(port, svdmh_in, TRUE);
				}
				else {
					/* if pin reconfig is NOT successful */
					FUSB3601_replyToConfig(port, svdmh_in, FALSE);
				}
			}
			else {
				FUSB3601_platform_log(port->port_id_, "FUSB Config Response", -1);
				if (svdmh_in.SVDM.CommandType == RESPONDER_ACK) {
					FUSB3601_informConfigResult(port, TRUE);
				}
				else {
					FUSB3601_informConfigResult(port, FALSE);
				}
				if(port->auto_vdm_state_ == AUTO_VDM_DP_SET_CONFIG) {
					port->auto_vdm_state_ = AUTO_VDM_DONE;
				}
			}
			break;
		default:
			/* command not recognized */
			return TRUE;
			break;
	}
	return FALSE;
}

FSC_BOOL FUSB3601_dpEvaluateModeEntry(struct Port *port, FSC_U32 mode_in)
{
  DisplayPortCaps_t field_mask = {0};
  DisplayPortCaps_t temp = {0};
  IN_FUNCTION
  if (port->display_port_data_.DpEnabled == FALSE) return FALSE;
  if (port->display_port_data_.DpAutoModeEntryEnabled == FALSE) return FALSE;

  port->display_port_data_.DpCaps.word = mode_in;

  /*  Mask works on fields at a time, so fix that here for incomplete values */
  /*  Field must be set to all 0s in order to be unmasked */
  /*  TODO - Magic numbers! */
  field_mask.word = port->display_port_data_.DpModeEntryMask.word;
  if (field_mask.field0) field_mask.field0 = 0x3;
  if (field_mask.field1) field_mask.field1 = 0x3;
  if (field_mask.field2) field_mask.field2 = 0x1;
  if (field_mask.field3) field_mask.field3 = 0x1;
  if (field_mask.field4) field_mask.field4 = 0x3F;
  if (field_mask.field5) field_mask.field5 = 0x1F;
  field_mask.fieldrsvd0 = 0x3;
  field_mask.fieldrsvd1 = 0x3;
  field_mask.fieldrsvd2 = 0x7FF;

  /*  For unmasked fields, at least one bit must match */
  temp.word = mode_in & port->display_port_data_.DpModeEntryValue.word;

  /*  Then, forget about the masked fields */
  temp.word = temp.word | field_mask.word;

  /*  At this point, if every field is non-zero, enter the mode */
  if ((temp.field0 != 0) && (temp.field1 != 0) && (temp.field2 != 0)
      && (temp.field3 != 0) && (temp.field4 != 0) && (temp.field5 != 0)) {
    return TRUE;
  }
  else {
    return FALSE;
  }
}

/*  Internal helper functions */
void FUSB3601_configDp(struct Port *port, FSC_BOOL enabled, FSC_U32 status)
{
  IN_FUNCTION
  port->display_port_data_.DpEnabled = enabled;
  port->display_port_data_.DpStatus.word = status;
}

void FUSB3601_configAutoDpModeEntry(struct Port *port, FSC_BOOL enabled,
                                        FSC_U32 mask, FSC_U32 value)
{
  IN_FUNCTION
  port->display_port_data_.DpAutoModeEntryEnabled = enabled;
  port->display_port_data_.DpModeEntryMask.word = mask;
  port->display_port_data_.DpModeEntryValue.word = value;
}

void FUSB3601_sendStatusData(struct Port *port, doDataObject_t svdmh_in)
{
  doDataObject_t svdmh_out = {0};
  FSC_U32 length_out = 0;
  FSC_U32 arr_out[2] = {0};
  doDataObject_t temp[2] = {{0}};
  IN_FUNCTION

  port->display_port_data_.DpStatus.Enabled = 1;

  /*  Reflect most fields */
  svdmh_out.object = svdmh_in.object;
  svdmh_out.SVDM.Version = STRUCTURED_VDM_VERSION;
  svdmh_out.SVDM.CommandType = RESPONDER_ACK;
  arr_out[0] = svdmh_out.object;
  length_out++;
  arr_out[1] = port->display_port_data_.DpStatus.word;
  length_out++;

  temp[0].object = arr_out[0];
  temp[1].object = arr_out[1];

  //SendVdmMessage(port, SOP_TYPE_SOP, arr_out, length_out,
//                 port->policy_state_);
  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length_out, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_replyToConfig(struct Port *port, doDataObject_t svdmh_in,
                                FSC_BOOL success)
{
  doDataObject_t svdmh_out = {0};
  FSC_U32 length_out = 0;
  FSC_U32 arr_out[2] = {0};
  doDataObject_t temp[2] = {{0}};
  IN_FUNCTION

  /*  Reflect most fields */
  svdmh_out.object = svdmh_in.object;
  svdmh_out.SVDM.Version = STRUCTURED_VDM_VERSION;
  svdmh_out.SVDM.CommandType = success == TRUE ? RESPONDER_ACK : RESPONDER_NAK;
  arr_out[0] = svdmh_out.object;
  length_out++;

  temp[0].object = arr_out[0];
//  SendVdmMessage(port, SOP_TYPE_SOP, arr_out, length_out,
//                 port->policy_state_);
  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length_out, temp, 0, peSinkSendSoftReset, FALSE, SOP_TYPE_SOP);
}

void FUSB3601_informStatus(struct Port *port, DisplayPortStatus_t stat)
{
  /*
   * TODO: 'system' should implement this
   * This function should be called to inform the 'system' of the DP status of
   * the port partner.
   */

  IN_FUNCTION
	int ret =0;
	port->display_port_data_.DpPpStatus.word = stat.word;

	#ifdef CONFIG_CONTEXTHUB_PD
    struct pd_dpm_combphy_event event;
	if(port->display_port_data_.DpPpConfig.word != 0) {
		event.dev_type = TCA_DP_IN;
		event.irq_type = TCA_IRQ_HPD_IN;
		event.mode_type = g_mux_type;
		event.typec_orien = port->registers_.TcpcCtrl.ORIENT;
		pr_info("\n %s + state = %d,irq = %d\n",__func__,stat.HpdState,stat.IrqHpd);
		if(!stat.HpdState) {
			event.dev_type = TCA_DP_OUT;
		    event.irq_type = TCA_IRQ_HPD_OUT;
			ret = pd_dpm_handle_combphy_event(event);
			pd_dpm_set_last_hpd_status(false);
		}
		else {
			event.dev_type = TCA_DP_IN;
			ret = pd_dpm_handle_combphy_event(event);
			pd_dpm_send_event(DP_CABLE_IN_EVENT);
			pd_dpm_set_last_hpd_status(true);
		}
		if(stat.IrqHpd) {
			event.irq_type = TCA_IRQ_SHORT;
			ret = pd_dpm_handle_combphy_event(event);
		}
		pr_info("\n %s - ret = %d\n",__func__,ret);
	}
	#endif
	pr_info("\n %s %d\n",__func__,__LINE__);
}

void FUSB3601_updateStatusData(struct Port *port)
{
  /*
   * TODO: 'system' should implement this
   * Called to get an update of our status - to be sent to the port partner
   */
}

void FUSB3601_informConfigResult(struct Port *port, FSC_BOOL success)
{
  /*
   * TODO: 'system' should implement this
   * Called when a config message is either ACKd or NAKd by the other side
   */

  IN_FUNCTION
	if (success == TRUE) {
		port->display_port_data_.DpPpConfig.word =
			port->display_port_data_.DpPpRequestedConfig.word;

		pr_info("\n %s,%d\n",__func__,__LINE__);

		#ifdef CONFIG_CONTEXTHUB_PD
		FSC_U8 fsc_polarity = 0;
		FSC_U32 pin_assignment = 0;
		enum aux_switch_channel_type channel_type= get_aux_switch_channel(); 
		int ret = 0;

		fsc_polarity = port->registers_.TcpcCtrl.ORIENT;
		dp_aux_switch_op(fsc_polarity);
		dp_aux_uart_switch_enable();

		if (channel_type == channel_superswitch) {
			if(!fsc_polarity) {
				FUSB3601_set_sbu_switch(port, Sbu_Close_Aux);
			} else {
				FUSB3601_set_sbu_switch(port, Sbu_Cross_Close_Aux);
			}
		} else {
			FUSB3601_set_sbu_switch(port, Sbu_None);
		}

		switch(port->display_port_data_.DpPpRequestedConfig.Conf) {
			case DP_CONF_UFP_D:
				pin_assignment = port->display_port_data_.DpPpRequestedConfig.DfpPa & 0xff;
				break;
			case DP_CONF_DFP_D:
				pin_assignment = port->display_port_data_.DpPpRequestedConfig.UfpPa & 0xff;
				break;
		}

		if(MODE_DP_PIN_C == pin_assignment || MODE_DP_PIN_E == pin_assignment) {
			hwlog_info("%s %d\n", __func__,__LINE__);
			g_mux_type = TCPC_DP;
		}
		else if(MODE_DP_PIN_D == pin_assignment || MODE_DP_PIN_F == pin_assignment) {
			hwlog_info("%s %d\n", __func__,__LINE__);
		    g_mux_type = TCPC_USB31_AND_DP_2LINE;
		}

		struct pd_dpm_combphy_event event;
			event.dev_type = TCA_ID_RISE_EVENT;
			event.irq_type = TCA_IRQ_HPD_OUT;
			event.mode_type = TCPC_NC;
			event.typec_orien = port->registers_.TcpcCtrl.ORIENT;
		    ret = pd_dpm_handle_combphy_event(event);

			pd_dpm_set_combphy_status(g_mux_type);

			event.dev_type = TCA_ID_FALL_EVENT;
			event.irq_type = TCA_IRQ_HPD_IN;
			event.mode_type = g_mux_type;
			event.typec_orien = port->registers_.TcpcCtrl.ORIENT;
		    ret = pd_dpm_handle_combphy_event(event);
			if(port->display_port_data_.DpPpStatus.HpdState) {
				FUSB3601_informStatus(port, port->display_port_data_.DpPpStatus);
			}
			pr_info("\n huawei_pd %s pd_event_notify ret = %d,mux_type = %d\n",__func__,ret,g_mux_type);
		#endif
  }
}


FSC_BOOL FUSB3601_DpReconfigure(struct Port *port, DisplayPortConfig_t config) {
  /*
   *  TODO: 'system' should implement this
   *  Called with a DisplayPort configuration to do!
   *  Return TRUE if/when successful, FALSE otherwise
   */
  IN_FUNCTION
  port->display_port_data_.DpConfig.word = config.word;
  /*  Must actually change configurations here before returning TRUE */
  return TRUE;
}

#endif /*  FSC_HAVE_DP */
