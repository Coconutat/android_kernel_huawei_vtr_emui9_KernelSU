#ifdef FSC_HAVE_DP

#include "dp.h"
#include "interface_dp.h"
#include "../vdm.h"
#include "../bitfield_translators.h"
#include "../../PD_Types.h"

/* TODO: HPD GPIO Bridge */

/* Automatic DisplayPort features! */
/* Automatic Mode Entry */
/* Uses a value/mask scheme */
/* 1 in mask = don't care. 0 in mask = compare to specified value */
FSC_BOOL DpEnabled;
FSC_BOOL DpAutoModeEntryEnabled;
FSC_BOOL AutoModeEntryEnabled;
DisplayPortCaps_t DpModeEntryMask;
DisplayPortCaps_t DpModeEntryValue;

/* DisplayPort Status/Config objects */
DisplayPortCaps_t DpCaps;
DisplayPortStatus_t DpStatus;
DisplayPortConfig_t DpConfig;

/* Port Partner Status/Config objects */
DisplayPortStatus_t DpPpStatus;
DisplayPortConfig_t DpPpRequestedConfig;
DisplayPortConfig_t DpPpConfig;
FSC_U32 DpModeEntered;

/* Externals */
extern VdmManager* vdmm;
extern PolicyState_t PolicyState;

#define MODE_DP_PIN_C 0x04
#define MODE_DP_PIN_D 0x08
#define MODE_DP_PIN_E 0x10
#define MODE_DP_PIN_F 0x20
#define INIT_WORD_DATA 0xFFFFFFFF

void initializeDp(void)
{
    DpCaps.word = 0x0;
    DpStatus.word = 0x0;
    DpStatus.Connection = DP_CONN_DFP_D;
    DpConfig.word = 0x0;

    DpPpStatus.word = 0x0;
    DpPpRequestedConfig.word = 0x0;
    DpPpConfig.word = 0x0;

    DpModeEntryMask.word = INIT_WORD_DATA;
    DpModeEntryMask.UfpDCapable = 0;
    DpModeEntryMask.DfpDCapable = 0;

    DpModeEntryValue.word = 0x0;
    DpModeEntryValue.UfpDCapable = 1;


    DpEnabled = platform_get_dp_enabled();
    DpAutoModeEntryEnabled = TRUE;
    DpModeEntered = 0x0;

    AutoModeEntryEnabled = FALSE;
}

void resetDp(void)
{
    DpStatus.word = 0x0;
    DpStatus.Connection = DP_CONN_DFP_D;
    DpConfig.word = 0x0;

    DpPpStatus.word = 0x0;
    DpPpRequestedConfig.word = 0x0;
    DpPpConfig.word = 0x0;

    DpModeEntered = 0x0;
}

/* Process special DisplayPort commands */
/* Returns TRUE when the message isn't processed */
/* and FALSE otherwise */
FSC_BOOL processDpCommand(FSC_U32* arr_in)
{
	doDataObject_t		__svdmh_in = {0};
    DisplayPortStatus_t     __stat;
    DisplayPortConfig_t     __config;

    if (DpEnabled == FALSE) return TRUE;
	__svdmh_in.object = arr_in[0];

	switch (__svdmh_in.SVDM.Command) {
	case DP_COMMAND_STATUS:
		if (__svdmh_in.SVDM.CommandType == INITIATOR) {
            if (DpModeEntered == FALSE) return TRUE;
            __stat.word = arr_in[1];
            informStatus(__stat);
			updateStatusData();                 // get updated info from system
			sendStatusData(__svdmh_in);	// send it out
		} else {
            __stat.word = arr_in[1];
			informStatus(__stat);
		}
        break;
	case DP_COMMAND_CONFIG:
		if (__svdmh_in.SVDM.CommandType == INITIATOR) {
            if (DpModeEntered == FALSE) return TRUE;
            __config.word = arr_in[1];
			if (DpReconfigure(__config) == TRUE) {
				/* if pin reconfig is successful */
				replyToConfig(__svdmh_in, TRUE);
			} else {
				/* if pin reconfig is NOT successful */
				replyToConfig(__svdmh_in, FALSE);
			}
		} else {
			if (__svdmh_in.SVDM.CommandType == RESPONDER_ACK) {
				informConfigResult(TRUE);
			} else {
				informConfigResult(FALSE);
			}
		}
        break;
	default:
		/* command not recognized */
		return TRUE;
	}
	return FALSE;
}

void sendStatusData(doDataObject_t svdmh_in)
{
	doDataObject_t  __svdmh_out = {0};
	FSC_U32         __length_out;
	FSC_U32			__arr_out[2] = {0};

	__length_out = 0;

	/* reflect most fields */
	__svdmh_out.object = svdmh_in.object;

	__svdmh_out.SVDM.Version        = STRUCTURED_VDM_VERSION;
	__svdmh_out.SVDM.CommandType	= RESPONDER_ACK;

	__arr_out[0] = __svdmh_out.object;
	__length_out++;

	__arr_out[1] = DpStatus.word;
	__length_out++;

	sendVdmMessage(SOP_TYPE_SOP, __arr_out, __length_out, PolicyState);
}

void replyToConfig(doDataObject_t svdmh_in, FSC_BOOL success)
{
	doDataObject_t  __svdmh_out = {0};
	FSC_U32          __length_out;
	FSC_U32			__arr_out[2] = {0};

	__length_out = 0;

	/* reflect most fields */
	__svdmh_out.object = svdmh_in.object;

	__svdmh_out.SVDM.Version	= STRUCTURED_VDM_VERSION;
	if (success == TRUE) {
		__svdmh_out.SVDM.CommandType = RESPONDER_ACK;
	} else {
		__svdmh_out.SVDM.CommandType	= RESPONDER_NAK;
	}

	__arr_out[0] = __svdmh_out.object;
	__length_out++;

	sendVdmMessage(SOP_TYPE_SOP, __arr_out, __length_out, PolicyState);
}

/* Evaluate DP Mode Entry */
/* Returns TRUE if mode can be entered, FALSE otherwise */
FSC_BOOL dpEvaluateModeEntry (FSC_U32 mode_in)
{
    DisplayPortCaps_t field_mask;
    DisplayPortCaps_t temp;

    if (DpEnabled == FALSE) return FALSE;
    if (DpAutoModeEntryEnabled == FALSE) return FALSE;

    DpCaps.word = mode_in;

    /* mask works on fields at a time, so fix that here for incomplete values */
    /* field must be set to all 0s in order to be unmasked */
    field_mask.word = DpModeEntryMask.word;
    if (field_mask.field0) field_mask.field0 = 0x3;
    if (field_mask.field1) field_mask.field1 = 0x3;
    if (field_mask.field2) field_mask.field2 = 0x1;
    if (field_mask.field3) field_mask.field3 = 0x1;
    if (field_mask.field4) field_mask.field4 = 0x3F;
    if (field_mask.field5) field_mask.field5 = 0x1F;
    field_mask.fieldrsvd0  = 0x3;
    field_mask.fieldrsvd1  = 0x3;
    field_mask.fieldrsvd2  = 0x7FF;

    /* for unmasked fields, at least one bit must match */
    temp.word = mode_in & DpModeEntryValue.word;

    /* then, forget about the masked fields */
    temp.word = temp.word | field_mask.word;

    /* at this point, if every field is non-zero, enter the mode */
    if ((temp.field0 != 0) && (temp.field1 != 0) &&
        (temp.field2 != 0) && (temp.field3 != 0) &&
        (temp.field4 != 0) && (temp.field5 != 0)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

FSC_BOOL evaluateModeEntry (FSC_U32 mode_in)
{
    return AutoModeEntryEnabled ? TRUE : FALSE;
}

void requestDpStatus(void)
{
    doDataObject_t  __svdmh = {0};
	FSC_U32          __length = 0;
	FSC_U32          __arr[2] = {0};

    __svdmh.SVDM.SVID           = DP_SID;					// DisplayPort SID
	__svdmh.SVDM.VDMType        = STRUCTURED_VDM;			// structured VDM Header
	__svdmh.SVDM.Version        = STRUCTURED_VDM_VERSION;	// version 1.0 = 0
	__svdmh.SVDM.ObjPos		= DpModeEntered & 0x7;		// saved mode position
	__svdmh.SVDM.CommandType    = INITIATOR;				// we are initiating
	__svdmh.SVDM.Command        = DP_COMMAND_STATUS;		// DisplayPort Status

    __arr[0] = __svdmh.object;
    __length++;

    __arr[1] = DpStatus.word;
    __length++;

    sendVdmMessageWithTimeout(SOP_TYPE_SOP, __arr, __length, peDpRequestStatus);
}

/* Initiate config request - called by 'system' to configure port partner */
void requestDpConfig(void)
{
    doDataObject_t  __svdmh = {0};
    FSC_U32          __length = 0;
    FSC_U32          __arr[2] = {0};
    bool fsc_polarity = FALSE;
    //FSC_U32 pin_assignment = 0;
    //int ret = 0;

    DpPpRequestedConfig.Conf = DP_CONF_UFP_D;
    DpPpRequestedConfig.SigConf = DP_CONF_SIG_DP_V1P3;
    DpPpRequestedConfig.Rsvd = 0;
    DpPpRequestedConfig.DfpPa = 0;
    DpPpRequestedConfig.UfpPa = 0;

    if (DpPpStatus.MultiFunctionPreferred) {
        if (DpCaps.ReceptacleIndication)
        {
            if (DpCaps.UFP_DPinAssignD) {
                DpPpRequestedConfig.DfpPa = DP_DFPPA_D;
            }
            else if (DpCaps.UFP_DPinAssignC) {
                DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
            }
            else if (DpCaps.UFP_DPinAssignE) {
                DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
            }
            else {
                return;
            }
        } else {
           if(DpCaps.DFP_DPinAssignD) {
               DpPpRequestedConfig.DfpPa = DP_DFPPA_D;
           }
           else if(DpCaps.DFP_DPinAssignC) {
               DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
           }
           else if(DpCaps.DFP_DPinAssignF) {
               DpPpRequestedConfig.DfpPa = DP_DFPPA_F;
           }
           else if(DpCaps.DFP_DPinAssignE) {
               DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
           }
           else {
               return;
           }
        }
    } else {
        if (DpCaps.ReceptacleIndication) {
            if(DpCaps.UFP_DPinAssignC) {
                DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
            } else if(DpCaps.UFP_DPinAssignE) {
                DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
            } else {
                return;
            }
        } else {
            if(DpCaps.DFP_DPinAssignC) {
                DpPpRequestedConfig.DfpPa = DP_DFPPA_C;
            } else if(DpCaps.DFP_DPinAssignE) {
                DpPpRequestedConfig.DfpPa = DP_DFPPA_E;
            } else {
                return;
            }
        }
    }

    if(DpPpStatus.Connection == DP_CONN_NEITHER)
    {
        return;
    }

    __svdmh.SVDM.SVID			= DP_SID;					// DisplayPort SID
	__svdmh.SVDM.VDMType		= STRUCTURED_VDM;			// structured VDM Header
	__svdmh.SVDM.Version        = STRUCTURED_VDM_VERSION;	// version 1.0 = 0
	__svdmh.SVDM.ObjPos		= DpModeEntered & 0x7;		// saved mode position
	__svdmh.SVDM.CommandType	= INITIATOR;				// we are initiating
	__svdmh.SVDM.Command		= DP_COMMAND_CONFIG;		// DisplayPort Config

    __arr[0] = __svdmh.object;
    __length++;

    __arr[1] = DpPpRequestedConfig.word;
    __length++;

    sendVdmMessage(SOP_TYPE_SOP, __arr, __length, PolicyState);
}

#endif // FSC_HAVE_DP
