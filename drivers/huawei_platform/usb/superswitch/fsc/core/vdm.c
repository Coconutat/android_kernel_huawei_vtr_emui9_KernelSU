/* **************************************************************************
 *  vdm.cpp
 *
 *  An example implementation of the VDM functionality API.
 * ************************************************************************** */
#ifdef FSC_HAVE_VDM

#ifdef FSC_HAVE_DP
#include "display_port.h"
#endif /*  FSC_HAVE_DP */
#include "port.h"     /*  Port class */
#include "timer.h"    /*  Timer values */
#include "vdm_types.h"
#include "vdm.h"
#include "vendor_info.h"
#include "protocol.h"
#include "policy.h"
#include <huawei_platform/log/hw_log.h>

#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();
#define IN_FUNCTION hwlog_info("%s ++\n", __func__);
#define OUT_FUNCTION hwlog_info("%s --\n", __func__);
#define VDM_ARRAY_LEN 1

/*  Public interface used by the policy engine */
/* Hostcomm functions */
void FUSB3601_ConfigureVdmResponses(struct Port *port, FSC_U8* bytes)
{
  IN_FUNCTION
  FSC_U16 _svid = 0;
  FSC_U32 _mode = 0;

  /*  TODO - Replace/explain magic numbers */
  if (*(bytes + 2)) {
    port->svid_enable_ = TRUE;
  }
  else {
    port->svid_enable_ = FALSE;
  }

  _svid |= *(bytes + 2 + 4 + 1); /*  TODO - Replace/explain magic numbers */
  _svid <<= 8; /*  TODO - Replace/explain magic numbers */
  _svid |= *(bytes + 2 + 4 + 0); /*  TODO - Replace/explain magic numbers */
  port->my_svid_ = _svid;

  /*  TODO - Replace/explain magic numbers */
  if (*(bytes + 2 + 8)) {
    port->mode_enable_ = TRUE;
  }
  else {
    port->mode_enable_ = FALSE;
  }

  _mode = _mode | *(bytes + 2 + 12 + 3); /*  TODO - Replace/explain magic numbers */
  _mode = _mode << 8;
  _mode = _mode | *(bytes + 2 + 12 + 2); /*  TODO - Replace/explain magic numbers */
  _mode = _mode << 8;
  _mode = _mode | *(bytes + 2 + 12 + 1); /*  TODO - Replace/explain magic numbers */
  _mode = _mode << 8;
  _mode = _mode | *(bytes + 2 + 12 + 0); /*  TODO - Replace/explain magic numbers */
  port->my_mode_ = _mode;
  port->mode_entered_ = FALSE;
}

void FUSB3601_ReadVdmConfiguration(struct Port *port, FSC_U8* data)
{
  /*  TODO - Replace/explain magic numbers in this entire function */
  IN_FUNCTION
  if (port->svid_enable_) {
    *(data + 0 + 0) = 1;
  }
  else {
    *(data + 0 + 1) = 0;
  }
  *(data + 0 + 1) = 0;
  *(data + 0 + 2) = 0;
  *(data + 0 + 3) = 0;

  *(data + 4 + 0) = (port->my_svid_ & 0xFF);
  *(data + 4 + 1) = ((port->my_svid_ >> 8) & 0xFF);
  *(data + 4 + 2) = 0;
  *(data + 4 + 3) = 0;

  if (port->mode_enable_) {
    *(data + 8 + 0) = 1;
  }
  else {
    *(data + 8 + 0) = 0;
  }
  *(data + 8 + 1) = 0;
  *(data + 8 + 2) = 0;
  *(data + 8 + 3) = 0;

  *(data + 12 + 0) = (port->my_mode_ & 0xFF);
  *(data + 12 + 1) = ((port->my_mode_ >> 8) & 0xFF);
  *(data + 12 + 2) = ((port->my_mode_ >> 16) & 0xFF);
  *(data + 12 + 3) = ((port->my_mode_ >> 24) & 0xFF);

  /*  add in mode entry status */
  if (port->mode_entered_) {
    *(data + 16 + 0) = 1;
  }
  else {
    *(data + 16 + 0) = 0;
  }
  *(data + 16 + 1) = 0;
  *(data + 16 + 2) = 0;
  *(data + 16 + 3) = 0;
}
/* End hostcomm functions */

void FUSB3601_AutoVdmDiscovery(struct Port *port)
{

    switch (port->auto_vdm_state_) {
      case AUTO_VDM_INIT:
      case AUTO_VDM_DISCOVER_ID_PP:
	  FUSB3601_PolicySendVDM(port, DISCOVER_IDENTITY, 0xFF00, SOP_TYPE_SOP, 0);
        //RequestDiscoverIdentity(port, SOP_TYPE_SOP);
        break;
      case AUTO_VDM_DISCOVER_SVIDS_PP:
	  FUSB3601_PolicySendVDM(port, DISCOVER_SVIDS, 0xFF00, SOP_TYPE_SOP, 0);
        //RequestDiscoverSvids(port, SOP_TYPE_SOP);
        break;
		case AUTO_VDM_DISCOVER_MODES_PP:
			FUSB3601_platform_log(port->port_id_, "FUSB Send DiscMode, auto_mode_disc_tracker_: ", port->auto_mode_disc_tracker_);
			FUSB3601_PolicySendVDM(port, DISCOVER_MODES,
			  port->core_svid_info_.svids[port->auto_mode_disc_tracker_],
			  SOP_TYPE_SOP, 0);
        break;
#ifdef FSC_HAVE_DP
	case AUTO_VDM_ENTER_MODE_PP:
        if (port->display_port_data_.AutoModeEntryObjPos > 0) {
          FUSB3601_PolicySendVDM(port, ENTER_MODE, 0xFF01, SOP_TYPE_SOP, port->display_port_data_.AutoModeEntryObjPos);
        }
        else {
          port->auto_vdm_state_ = AUTO_VDM_DONE;
        }
        break;
	case AUTO_VDM_DP_GET_STATUS:
        if (port->display_port_data_.DpModeEntered) {
          FUSB3601_requestDpStatus(port);
        }
		else {
			port->auto_vdm_state_ = AUTO_VDM_DONE;
		}
        break;
	case AUTO_VDM_DP_SET_CONFIG:
        FUSB3601_requestDpConfig(port);
        break;
#endif /*  FSC_HAVE_DP */
      default:
        port->auto_vdm_state_ = AUTO_VDM_DONE;
        break;
    }

}

/*  Receiving end VDM functionality */
FSC_S32 FUSB3601_ProcessVdmMessage(struct Port *port)
{
	port->expecting_vdm_response_ = FALSE;

  if (port->policy_rx_data_obj_[0].SVDM.VDMType == STRUCTURED_VDM) {
    switch (port->policy_rx_data_obj_[0].SVDM.Command) {
      case DISCOVER_IDENTITY:
        return FUSB3601_ProcessDiscoverIdentity(port, port->protocol_msg_rx_sop_);
      case DISCOVER_SVIDS:
        return FUSB3601_ProcessDiscoverSvids(port, port->protocol_msg_rx_sop_);
      case DISCOVER_MODES:
        return FUSB3601_ProcessDiscoverModes(port, port->protocol_msg_rx_sop_);
      case ENTER_MODE:
        return FUSB3601_ProcessEnterMode(port, port->protocol_msg_rx_sop_);
      case EXIT_MODE:
        return FUSB3601_ProcessExitMode(port, port->protocol_msg_rx_sop_);
      case ATTENTION:
        return FUSB3601_ProcessAttention(port, port->protocol_msg_rx_sop_);
      default:
        /*  SVID-Specific commands go here */
        return FUSB3601_ProcessSvidSpecific(port, port->protocol_msg_rx_sop_);
    }
  }
  else {
    /* Unstructured messages */
    return 1;
  }
}

//TODO: AMA Support
FSC_S32 FUSB3601_ProcessDiscoverIdentity(struct Port *port, SopType sop)
{
  IN_FUNCTION
  doDataObject_t vdm_out[7] = {{0}};
  FSC_U8 num_data_obj = 4;

  /*  Must NAK or not respond to Discover ID with wrong SVID */

  if (port->policy_rx_data_obj_[0].SVDM.SVID != PD_SID) return -1;

  if (port->policy_rx_data_obj_[0].SVDM.CommandType == INITIATOR) {

    vdm_out[0].SVDM.SVID = PD_SID;
    vdm_out[0].SVDM.VDMType = STRUCTURED_VDM;
    vdm_out[0].SVDM.Version = Structured_VDM_Version_SOP;
    vdm_out[0].SVDM.ObjPos = 0;
    vdm_out[0].SVDM.CommandType = RESPONDER_ACK;
    vdm_out[0].SVDM.Command = DISCOVER_IDENTITY;

    vdm_out[1].ID_HEADER_VDO.ModalOperationSupported = Modal_Operation_Supported_SOP;
    vdm_out[1].ID_HEADER_VDO.ProductType = Product_Type_SOP;
    vdm_out[1].ID_HEADER_VDO.USBDeviceCapable = Data_Capable_as_USB_Device_SOP;
    vdm_out[1].ID_HEADER_VDO.USBHostCapable = Data_Capable_as_USB_Host_SOP;
    vdm_out[1].ID_HEADER_VDO.VID = USB_VID_SOP;

    vdm_out[2].CERT_STAT_VDO.XID = XID_SOP;

    vdm_out[3].PRODUCT_VDO.PID = PID_SOP;
	vdm_out[3].PRODUCT_VDO.bcdDevice = bcdDevice_SOP;

	if (vdm_out[1].ID_HEADER_VDO.ProductType == AMA) {

		vdm_out[4].AMA_VDO.USBSuperSpeed = AMA_Superspeed_Support;
		vdm_out[4].AMA_VDO.VbusRequired = AMA_VBUS_reqd;
		vdm_out[4].AMA_VDO.VconnRequired = AMA_VCONN_reqd;
		vdm_out[4].AMA_VDO.VconnPower = AMA_VCONN_power;
		vdm_out[4].AMA_VDO.VDOVersion = 0x0;
		vdm_out[4].AMA_VDO.FirmwareVersion = AMA_FW_Vers;
		vdm_out[4].AMA_VDO.HWVersion = AMA_HW_Vers;

		num_data_obj++;
	}

	//TODO: set correct softreset for power role
	FUSB3601_ProtocolSendData(port, DMTVendorDefined, num_data_obj, vdm_out, 0, peSinkSendSoftReset, FALSE, sop);
  }
  else {
    /* Incoming responses, ACKs, NAKs, BUSYs */

    if (port->policy_rx_data_obj_[0].SVDM.CommandType == RESPONDER_ACK) {
      port->partner_id_header_vdo = port->policy_rx_data_obj_[1];
      port->partner_cert_stat_vdo = port->policy_rx_data_obj_[2];
      port->partner_product_vdo = port->policy_rx_data_obj_[3];

      if ((port->policy_rx_header_.NumDataObjects == 5)) {
        port->partner_product_type_vdo = port->policy_rx_data_obj_[4];
      }
      //TODO: platform callback for success
      if(port->auto_vdm_state_ == AUTO_VDM_DISCOVER_ID_PP
		  || port->auto_vdm_state_ == AUTO_VDM_INIT) {
	  port->auto_vdm_state_  = AUTO_VDM_DISCOVER_SVIDS_PP;
      }
    }
    else {
	port->auto_vdm_state_ = AUTO_VDM_DONE;
    }
    return 0;
  }
  return 0;
}

FSC_S32 FUSB3601_ProcessDiscoverSvids(struct Port *port, SopType sop)
{
  IN_FUNCTION
	doDataObject_t vdm_out[(Num_SVIDs_min_SOP >> 1) + 2] = {{0}};
	FSC_U8 i;
	FSC_U8 num_svids;
	FSC_U8 max_data_obj;
	FSC_U8 length = 1;

  /* Must NAK or not respond to Discover SVIDs with wrong SVID */
  if (port->policy_rx_data_obj_[0].SVDM.SVID != PD_SID) return -1;

  if (port->policy_rx_data_obj_[0].SVDM.CommandType == INITIATOR) {

	  vdm_out[0].SVDM.SVID = PD_SID;
	  vdm_out[0].SVDM.VDMType = STRUCTURED_VDM;
	  vdm_out[0].SVDM.Version = Structured_VDM_Version_SOP;
	  vdm_out[0].SVDM.ObjPos = 0;
	  vdm_out[0].SVDM.Command = DISCOVER_SVIDS;

	  if(Num_SVIDs_min_SOP != 0) {
		  vdm_out[0].SVDM.CommandType = RESPONDER_ACK;
		  //TODO: Implement #SVIDS from vendor_info.h
		  vdm_out[1].SVID_VDO.SVID0 = port->my_svid_;
		  vdm_out[1].SVID_VDO.SVID1 = 0x0000;
		  length = 2;
	  }
	  else {
		  vdm_out[0].SVDM.CommandType = RESPONDER_NAK;
	  }

	  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length, vdm_out, 0, peSinkSendSoftReset, FALSE, sop);
  }
  else {
    /* Incoming responses, ACKs, NAKs, BUSYs */
    num_svids = 0;
    max_data_obj = (MAX_NUM_SVIDS >> 1) + 2;

    if (port->policy_rx_data_obj_[0].SVDM.CommandType == RESPONDER_ACK) {
	if(max_data_obj < port->policy_rx_header_.NumDataObjects) {
		port->policy_rx_header_.NumDataObjects = max_data_obj;
	}
	for(i = 1; i < port->policy_rx_header_.NumDataObjects; i++) {
		if(port->policy_rx_data_obj_[i].SVID_VDO.SVID0 != 0) {
			port->core_svid_info_.svids[num_svids] = port->policy_rx_data_obj_[i].SVID_VDO.SVID0;
			num_svids++;
		}
		if(port->policy_rx_data_obj_[i].SVID_VDO.SVID1 != 0){
			port->core_svid_info_.svids[num_svids] = port->policy_rx_data_obj_[i].SVID_VDO.SVID1;
			num_svids++;
		}
	}

	port->core_svid_info_.num_svids = num_svids;
		FUSB3601_platform_log(port->port_id_, "FUSB SVIDS ACK, Num: ", num_svids);

        if(port->auto_vdm_state_ == AUTO_VDM_DISCOVER_SVIDS_PP) {
            port->auto_vdm_state_ = AUTO_VDM_DISCOVER_MODES_PP;
        }
		if(num_svids == 12) {
			FUSB3601_PolicySendVDM(port, DISCOVER_SVIDS, 0xFF00, SOP_TYPE_SOP, 0);
		}
    }
    else {
	port->auto_vdm_state_ = AUTO_VDM_DONE;
    }
    return 0;
  }
  return 0;
}

FSC_S32 FUSB3601_ProcessDiscoverModes(struct Port *port, SopType sop)
{
  IN_FUNCTION
	doDataObject_t vdm_out[SVID1_num_modes_min_SOP + 1] = {{0}};
	ModesInfo modes_info;
	FSC_U8 i;
	FSC_U8 length = 1;

	FUSB3601_platform_log(port->port_id_, "FUSB Discover Modes", -1);

  if (port->policy_rx_data_obj_[0].SVDM.CommandType == INITIATOR) {

	  vdm_out[0].SVDM.SVID = port->policy_rx_data_obj_[0].SVDM.SVID;
	  vdm_out[0].SVDM.VDMType = STRUCTURED_VDM;
	  vdm_out[0].SVDM.Version = Structured_VDM_Version_SOP;
	  vdm_out[0].SVDM.ObjPos = 0;
	  vdm_out[0].SVDM.Command = DISCOVER_MODES;

	  if((SVID1_num_modes_min_SOP != 0)
			  && (port->policy_rx_data_obj_[0].SVDM.SVID == port->my_svid_)) {
		  vdm_out[0].SVDM.CommandType = RESPONDER_ACK;
		  //TODO: Implement #Modes from vendor_info.h
		  vdm_out[1].object = port->my_mode_;
		  length = 2;
	  }
	  else {
		  vdm_out[0].SVDM.CommandType = RESPONDER_NAK;
	  }

	  FUSB3601_ProtocolSendData(port, DMTVendorDefined, length, vdm_out, 0, peSinkSendSoftReset, FALSE, sop);
    return 0;
  }
  else {
	port->auto_mode_disc_tracker_++;
    /*  Incoming responses, ACKs, NAKs, BUSYs */
      if (port->policy_rx_data_obj_[0].SVDM.CommandType == RESPONDER_ACK) {
	  FUSB3601_platform_log(port->port_id_, "FUSB Discover Modes ACK", -1);

        modes_info.svid = port->policy_rx_data_obj_[0].SVDM.SVID;
        modes_info.num_modes = port->policy_rx_header_.NumDataObjects - 1;
        if(modes_info.num_modes > MAX_MODES_PER_SVID) {
		modes_info.num_modes = MAX_MODES_PER_SVID;
        }
        for (i = 0; i < modes_info.num_modes; i++) {
          modes_info.modes[i] = port->policy_rx_data_obj_[i + 1].object;
        }

		modes_info.nack = FALSE;

        if(port->auto_mode_disc_tracker_ == (port->core_svid_info_.num_svids)) {
			FUSB3601_platform_log(port->port_id_, "FUSB Discover Modes: All Modes Discovered", -1);
	  port->auto_vdm_state_ = AUTO_VDM_ENTER_MODE_PP;
        }
      }
      else {
	      modes_info.nack = TRUE;
	      port->auto_vdm_state_ = AUTO_VDM_ENTER_MODE_PP;
      }

      FUSB3601_VdmInformModes(port, sop, modes_info);
    }
    return 0;

}

FSC_S32 FUSB3601_ProcessEnterMode(struct Port *port, SopType sop)
{
	doDataObject_t vdm_out[1] = {{0}};
  FSC_BOOL mode_entered;




  IN_FUNCTION
  if (port->policy_rx_data_obj_[0].SVDM.CommandType == INITIATOR) {

      mode_entered = FUSB3601_VdmModeEntryRequest(port, port->policy_rx_data_obj_[0].SVDM.SVID,
		  port->policy_rx_data_obj_[0].SVDM.ObjPos);

      /*  if DPM says OK, respond with ACK */
      if (mode_entered) {
        /*  entered mode successfully */
	  vdm_out[0].SVDM.CommandType = RESPONDER_ACK;
      }
      else {
        /*  NAK if mode not entered */
	  vdm_out[0].SVDM.CommandType = RESPONDER_NAK;
      }

    /*  most of the message response will be the same whether we entered the mode or not */
      vdm_out[0].SVDM.SVID = port->policy_rx_data_obj_[0].SVDM.SVID;
      vdm_out[0].SVDM.VDMType = STRUCTURED_VDM;
      vdm_out[0].SVDM.Version = Structured_VDM_Version_SOP;
      vdm_out[0].SVDM.ObjPos = port->policy_rx_data_obj_[0].SVDM.ObjPos;
      vdm_out[0].SVDM.Command = ENTER_MODE;

      FUSB3601_ProtocolSendData(port, DMTVendorDefined, 1, vdm_out, 0, peSinkSendSoftReset, FALSE, sop);
    return 0;
  }
  else { /* Incoming responses, ACKs, NAKs, BUSYs */
    if (port->policy_rx_data_obj_[0].SVDM.CommandType != RESPONDER_ACK) {
		FUSB3601_VdmEnterModeResult(port, FALSE, port->policy_rx_data_obj_[0].SVDM.SVID,
		  port->policy_rx_data_obj_[0].SVDM.ObjPos);
		  FUSB3601_AutoVdmDiscovery(port);
    }
    else {
		FUSB3601_VdmEnterModeResult(port, TRUE, port->policy_rx_data_obj_[0].SVDM.SVID,
		  port->policy_rx_data_obj_[0].SVDM.ObjPos);
		if(port->auto_vdm_state_ == AUTO_VDM_ENTER_MODE_PP) {
			port->auto_vdm_state_ = AUTO_VDM_DP_GET_STATUS;
		}
		port->mode_entered_ = TRUE;
    }
  }
  return 0;
}

FSC_S32 FUSB3601_ProcessExitMode(struct Port *port, SopType sop)
{
	doDataObject_t vdm_out[1] = {{0}};
  FSC_BOOL mode_exited;

  IN_FUNCTION
  if (port->policy_rx_data_obj_[0].SVDM.CommandType == INITIATOR) {

      mode_exited = FUSB3601_VdmModeExitRequest(port, port->policy_rx_data_obj_[0].SVDM.SVID,
		  port->policy_rx_data_obj_[0].SVDM.ObjPos);
      /*  if DPM says OK, respond with ACK */
      if (mode_exited) {
	  vdm_out[0].SVDM.CommandType = RESPONDER_ACK;
      }
      else {
        /*  NAK if mode not exited */
	  vdm_out[0].SVDM.CommandType = RESPONDER_NAK;
      }

      vdm_out[0].SVDM.SVID = port->policy_rx_data_obj_[0].SVDM.SVID;
      vdm_out[0].SVDM.VDMType = STRUCTURED_VDM;
      vdm_out[0].SVDM.Version = Structured_VDM_Version_SOP;\
      vdm_out[0].SVDM.ObjPos = port->policy_rx_data_obj_[0].SVDM.ObjPos;
      vdm_out[0].SVDM.Command = EXIT_MODE;

      FUSB3601_ProtocolSendData(port, DMTVendorDefined, 1, vdm_out, 0, peSinkSendSoftReset, FALSE, sop);
    return 0;
  }
  else {
    if (port->policy_rx_data_obj_[0].SVDM.CommandType != RESPONDER_ACK) {
      FUSB3601_VdmExitModeResult(port, FALSE, port->policy_rx_data_obj_[0].SVDM.SVID,
		  port->policy_rx_data_obj_[0].SVDM.ObjPos);
      /*  when exit mode not ACKed, go to hard reset state! */
      FUSB3601_ProtocolSendHardReset(port);
    }
    else {
      FUSB3601_VdmExitModeResult(port, TRUE, port->policy_rx_data_obj_[0].SVDM.SVID,
		  port->policy_rx_data_obj_[0].SVDM.ObjPos);
    }
    return 0;
  }
  return 0;
}

FSC_S32 FUSB3601_ProcessAttention(struct Port *port, SopType sop)
{
  IN_FUNCTION
	DisplayPortStatus_t __attention_status;
	FUSB3601_platform_log(port->port_id_, "FUSB Process Attention, DpPpConfig: ", port->display_port_data_.DpPpConfig.word);

	if (!port->mode_entered_) {
		/* Unexpected message */
		FUSB3601_set_policy_state(port, peSourceSoftReset);
		FUSB3601_PolicySourceSendSoftReset(port);
		return 0;
	}
	if ((port->policy_rx_data_obj_[0].SVDM.SVID == DP_SID)
			&& (port->policy_rx_header_.NumDataObjects == 2)) {
	FUSB3601_platform_log(port->port_id_, "FUSB Process Attention, AutoVdm: ", port->auto_vdm_state_);
		__attention_status.word = port->policy_rx_data_obj_[1].object;
		FUSB3601_informStatus(port, __attention_status);

		if((port->display_port_data_.DpPpConfig.word == 0)) {
			FUSB3601_requestDpConfig(port);
		}
	}

  return 0;
}

FSC_S32 FUSB3601_ProcessSvidSpecific(struct Port *port, SopType sop)
{
	doDataObject_t vdm_out[1] = {{0}};

  IN_FUNCTION
#ifdef FSC_HAVE_DP
  if (port->policy_rx_data_obj_[0].SVDM.SVID == DP_SID) {
    if (!FUSB3601_processDpCommand(port, &port->policy_rx_data_obj_[0].object)) {
      return 0; /* DP code will send response, so return */
    }
  }
#endif /*  FSC_HAVE_DP */
  /*  in this case the command is unrecognized. Reply with a NAK. */
  vdm_out[0].SVDM.SVID = port->policy_rx_data_obj_[0].SVDM.SVID;
  vdm_out[0].SVDM.VDMType = STRUCTURED_VDM;
  vdm_out[0].SVDM.Version = Structured_VDM_Version_SOP;
  vdm_out[0].SVDM.ObjPos = 0;
  vdm_out[0].SVDM.CommandType = RESPONDER_NAK;
  vdm_out[0].SVDM.Command = port->policy_rx_data_obj_[0].SVDM.Command;

  FUSB3601_ProtocolSendData(port, DMTVendorDefined, 0, vdm_out, 0, peSinkSendSoftReset, FALSE, sop);
  return 0;
}

FSC_U32 FUSB3601_GetVdmTimer(Command command) {
	FSC_U32 timer;
  IN_FUNCTION
	  /*  start the appropriate timer */
	  switch (command) {
	    case DISCOVER_IDENTITY:
	    case DISCOVER_SVIDS:
	    case DISCOVER_MODES:
	    case ATTENTION:
	      timer = ktVDMSenderResponse;
	      break;
	    case ENTER_MODE:
	      timer = ktVDMWaitModeEntry;
	      break;
	    case EXIT_MODE:
	      timer = ktVDMWaitModeExit;
	      break;
	    default:
	      timer = 0;
	      break;
	  }
	  return timer;
}

void FUSB3601_ResetPolicyState(struct Port *port) {

// Need a callback that informs that a vdm request failed
  IN_FUNCTION

	port->expecting_vdm_response_ = FALSE;
}

FSC_BOOL FUSB3601_VdmModeEntryRequest(struct Port *port, FSC_U16 svid, FSC_U32 mode_index)
{
  IN_FUNCTION
  if ((port->svid_enable_ == TRUE) &&
      (port->mode_enable_ == TRUE) &&
      (svid == port->my_svid_) &&
      (mode_index == 1)) {
    port->mode_entered_ = TRUE;

#ifdef FSC_HAVE_DP
    if (port->my_svid_ == DP_SID) {
      port->display_port_data_.DpModeEntered = mode_index;
    }
#endif /*  FSC_HAVE_DP */
    return TRUE;
  }
  return FALSE;
}

FSC_BOOL FUSB3601_VdmModeExitRequest(struct Port *port, FSC_U16 svid, FSC_U32 mode_index)
{
  IN_FUNCTION
  if (port->mode_entered_ == TRUE &&
      svid == port->my_svid_ &&
      mode_index == 1) {
    port->mode_entered_ = FALSE;

#ifdef FSC_HAVE_DP
    if (port->display_port_data_.DpModeEntered &&
        (port->display_port_data_.DpModeEntered == mode_index) &&
        (svid == DP_SID)) {
      port->display_port_data_.DpModeEntered = 0;
    }
#endif /*  FSC_HAVE_DP */
    return TRUE;
  }
  return FALSE;
}

FSC_BOOL FUSB3601_VdmEnterModeResult(struct Port *port, FSC_BOOL success, FSC_U16 svid,
                            FSC_U32 mode_index)
{
  IN_FUNCTION
#ifdef FSC_HAVE_DP
  if (port->display_port_data_.AutoModeEntryObjPos > 0) {
    port->display_port_data_.AutoModeEntryObjPos = 0;
  }

  if (svid == DP_SID) {
    port->display_port_data_.DpModeEntered = mode_index;
  }
#endif /*  FSC_HAVE_DP */
  return TRUE;
}

void FUSB3601_VdmExitModeResult(struct Port *port, FSC_BOOL success, FSC_U16 svid,
                       FSC_U32 mode_index)
{
  IN_FUNCTION
#ifdef FSC_HAVE_DP
  if (svid == DP_SID && port->display_port_data_.DpModeEntered == mode_index) {
    port->display_port_data_.DpModeEntered = 0;
  }
#endif /*  FSC_HAVE_DP */
}

void FUSB3601_VdmInformModes(struct Port *port, SopType sop,
                    ModesInfo modes_info)
{
  IN_FUNCTION
#ifdef FSC_HAVE_DP
  FSC_U32 i;
  if (modes_info.svid == DP_SID && modes_info.nack == FALSE) {
  FUSB3601_platform_log(port->port_id_, "FUSB DP Detected", -1);
    for (i = 0; i < modes_info.num_modes; ++i) {
      if (FUSB3601_dpEvaluateModeEntry(port, modes_info.modes[i])) {
	  FUSB3601_platform_log(port->port_id_, "FUSB DP Accepted", -1);
        port->display_port_data_.AutoModeEntryObjPos = i + 1;
      }
    }
  }
#endif /*  FSC_HAVE_DP */
}

void FUSB3601_VdmInitDpm(struct Port *port)
{
  IN_FUNCTION
  port->svid_enable_ = TRUE;
  port->mode_enable_ = TRUE;
  port->my_svid_ = SVID_DEFAULT;
  port->my_mode_  = MODE_DEFAULT;
  port->mode_entered_ = FALSE;
}

#endif /*  FSC_HAVE_VDM */
