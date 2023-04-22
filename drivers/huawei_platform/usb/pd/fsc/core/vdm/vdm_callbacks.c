/****************************************************************************
 * Company:         Fairchild Semiconductor
 *
 * Author           Date          Comment
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * G. Noblesmith
 *
 *
 *~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * Software License Agreement:
 *
 * The software supplied herewith by Fairchild Semiconductor (the Company)
 * is supplied to you, the Company's customer, for exclusive use with its
 * USB Type C / USB PD products.  The software is owned by the Company and/or
 * its supplier, and is protected under applicable copyright laws.
 * All rights are reserved. Any use in violation of the foregoing restrictions
 * may subject the user to criminal sanctions under applicable laws, as well
 * as to civil liability for the breach of the terms and conditions of this
 * license.
 *
 * THIS SOFTWARE IS PROVIDED IN AN AS IS CONDITION. NO WARRANTIES,
 * WHETHER EXPRESS, IMPLIED OR STATUTORY, INCLUDING, BUT NOT LIMITED
 * TO, IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE APPLY TO THIS SOFTWARE. THE COMPANY SHALL NOT,
 * IN ANY CIRCUMSTANCES, BE LIABLE FOR SPECIAL, INCIDENTAL OR
 * CONSEQUENTIAL DAMAGES, FOR ANY REASON WHATSOEVER.
 *
 *****************************************************************************/
#ifdef FSC_HAVE_VDM

#include "vdm.h"
#include "vdm_callbacks.h"
#include "vdm_types.h"
#include "../vendor_info.h"

#ifdef FSC_HAVE_DP
#include "DisplayPort/dp.h"
#include "DisplayPort/interface_dp.h"
#endif // FSC_HAVE_DP
#include <huawei_platform/usb/hw_pd_dev.h>


FSC_BOOL svid_enable;
FSC_BOOL mode_enable;
FSC_U16 my_svid;
FSC_U32 my_mode;

FSC_BOOL mode_entered;
SvidInfo core_svid_info;
static bool huawei_dock_svid_exist = false;

#ifdef FSC_HAVE_DP
int AutoModeEntryObjPos;
#endif // FSC_HAVE_DP

Identity vdmRequestIdentityInfo() {
    Identity id = {0};

    if(Responds_To_Discov_SOP) {
        id.nack = FALSE;

        id.id_header.usb_host_data_capable = Data_Capable_as_USB_Host_SOP;
        id.id_header.usb_device_data_capable = Data_Capable_as_USB_Device_SOP;
        if(platform_get_product_type_ama()) {
            id.id_header.product_type = AMA;
        } else {
            id.id_header.product_type = PERIPHERAL;
        }

        id.id_header.modal_op_supported = platform_get_modal_operation_supported();
        id.id_header.usb_vid = USB_VID_SOP;

        id.cert_stat_vdo.test_id = XID_SOP;

        id.product_vdo.usb_product_id = PID_SOP;
        id.product_vdo.bcd_device = bcdDevice_SOP;

        /* Cable VDO */
        /* Not Currently Implemented */

        /* AMA */
        id.ama_vdo.cable_hw_version	= AMA_HW_Vers;
        id.ama_vdo.cable_fw_version	= AMA_FW_Vers;

        id.ama_vdo.vconn_full_power	= AMA_VCONN_power;
        id.ama_vdo.vconn_requirement = AMA_VCONN_reqd;
        id.ama_vdo.vbus_requirement	= AMA_VBUS_reqd;
        id.ama_vdo.usb_ss_supp = AMA_Superspeed_Support;
        id.ama_vdo.vdo_version = AMA_VDO_Vers;
    } else {
        id.nack = TRUE;
    }

    return id;
}

SvidInfo vdmRequestSvidInfo() {
    SvidInfo svid_info = {0};

    if (Responds_To_Discov_SOP && svid_enable && platform_get_modal_operation_supported() && platform_discover_svid_supported()) {
        svid_info.nack = FALSE;
        svid_info.num_svids = Num_SVIDs_min_SOP;
        svid_info.svids[0] = my_svid;
    } else {
        svid_info.nack = TRUE;
        svid_info.num_svids = 0;
        svid_info.svids[0] = 0x0000;
    }

    return svid_info;
}

ModesInfo vdmRequestModesInfo(FSC_U16 svid) {
    ModesInfo modes_info = {0};

    if (Responds_To_Discov_SOP && svid_enable && mode_enable && (svid == my_svid) && platform_discover_mode_supported()) {
        modes_info.nack = FALSE;
        modes_info.svid = svid;
        modes_info.num_modes = 1;
        modes_info.modes[0] = my_mode;
    } else {
        modes_info.nack = TRUE;
        modes_info.svid = svid;
        modes_info.num_modes = 0;
        modes_info.modes[0] = 0;
    }
    return modes_info;
}

FSC_BOOL vdmModeEntryRequest(FSC_U16 svid, FSC_U32 mode_index) {
    if (svid_enable && mode_enable && (svid == my_svid) && (mode_index == 1) && platform_enter_mode_supported()) {
        mode_entered = TRUE;

#ifdef FSC_HAVE_DP
        if (my_svid == DP_SID) {
            DpModeEntered = mode_index;
        }
#endif // FSC_HAVE_DP
        return TRUE;
    }

    return FALSE;
}

FSC_BOOL vdmModeExitRequest(FSC_U16 svid, FSC_U32 mode_index) {
    if (mode_entered && (svid == my_svid) && (mode_index == 1)) {
        mode_entered = FALSE;

#ifdef FSC_HAVE_DP
        if (DpModeEntered && (DpModeEntered == mode_index) && (svid == DP_SID)) {
            DpModeEntered = 0;
        }
#endif // FSC_HAVE_DP

        return TRUE;
    }
    return FALSE;
}

FSC_BOOL vdmEnterModeResult(FSC_BOOL success, FSC_U16 svid, FSC_U32 mode_index) {

    if (AutoModeEntryObjPos > 0) {
        AutoModeEntryObjPos = 0;
    }

    if(success)
    {
        mode_entered = TRUE;
    }

#ifdef FSC_HAVE_DP
    if (svid == DP_SID) {
        DpModeEntered = mode_index;
		FSC_PRINT("FUSB %s - DpModeEntered = %d",__func__,DpModeEntered);
    }
#endif // FSC_HAVE_DP

    return TRUE;
}

void vdmExitModeResult(FSC_BOOL success, FSC_U16 svid, FSC_U32 mode_index) {

    mode_entered = FALSE;

#ifdef FSC_HAVE_DP
    if (svid == DP_SID && DpModeEntered == mode_index) {
        DpModeEntered = 0;
    }
#endif // FSC_HAVE_DP
}

bool fusb30x_pd_dpm_get_hw_dock_svid_exist(void* client)
{
	return huawei_dock_svid_exist;
}
void fusb30x_pd_dpm_set_hw_dock_svid_exist(FSC_BOOL exist)
{
	huawei_dock_svid_exist = exist;
}
void vdmInformIdentity(FSC_BOOL success, SopType sop, Identity id) {

}

void vdmInformSvids(FSC_BOOL success, SopType sop, SvidInfo svid_info) {
    if (success) {
        FSC_U32 i;
        core_svid_info.num_svids = svid_info.num_svids;
        for (i = 0; (i < svid_info.num_svids) && (i < MAX_NUM_SVIDS); i++) {
            core_svid_info.svids[i] = svid_info.svids[i]; 
            if(PD_DPM_HW_DOCK_SVID == svid_info.svids[i]) {
		    fusb30x_pd_dpm_set_hw_dock_svid_exist(true);
	      }
        }
    }
}

void vdmInformModes(FSC_BOOL success, SopType sop, ModesInfo modes_info) {
#ifdef FSC_HAVE_DP
    FSC_U32 i;

    if (modes_info.svid == DP_SID && modes_info.nack == FALSE) {
        for (i = 0; i < modes_info.num_modes; i++) {
            if (dpEvaluateModeEntry(modes_info.modes[i])) {
                AutoModeEntryObjPos = i+1;
            }
        }
	FSC_PRINT("FUSB %s - DP Detected, AutoModeEntryObjPos: %d\n",__func__,AutoModeEntryObjPos);
    }
#endif // FSC_HAVE_DP
}

void vdmInformAttention(FSC_U16 svid, FSC_U8 mode_index) {

}

void vdmInitDpm() {

    svid_enable = TRUE;
    mode_enable = TRUE;

    my_svid = SVID_DEFAULT;
    my_mode = MODE_DEFAULT;

    mode_entered = FALSE;
}

#endif // FSC_HAVE_VDM
