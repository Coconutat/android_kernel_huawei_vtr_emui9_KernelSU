/* **************************************************************************
 *  vdm.h
 *
 *  Declares VDM functionality API.
 * ************************************************************************** */
#ifndef FSCPM_VDM_H_
#define FSCPM_VDM_H_

#ifdef FSC_HAVE_VDM

#ifdef FSC_HAVE_DP
#include "display_port_types.h"
#endif /*  FSC_HAVE_DP */
#include "platform.h" /*  Driver typedefs */
#include "PDTypes.h"  /*  SopType, etc */
#include "port.h"     /*  Port class interface */

#define NUM_VDM_MODES 6
#define MAX_NUM_SVIDS_PER_SOP 30
#define MAX_SVIDS_PER_MESSAGE 12
#define MIN_DISC_ID_RESP_SIZE 3

/*
 *  Configures the VDM code
 */
void FUSB3601_ConfigureVdmResponses(struct Port *port, FSC_U8 *bytes);
void FUSB3601_ReadVdmConfiguration(struct Port *port, FSC_U8 *data);


void FUSB3601_ConvertAndProcessVdmMessage(struct Port *port);
/*  Processes the port's current VDM command */
void FUSB3601_DoVdmCommand(struct Port *port);
/*  This assumes we're already in either Source or Sink Ready states! */
void FUSB3601_AutoVdmDiscovery (struct Port *port);

/*  Sends vdm messages */
void FUSB3601_SendVdmMessage(struct Port *port, SopType sop, FSC_U32 *arr, FSC_U32 length, PolicyState_t next_ps);
void FUSB3601_SendVdmMessageWithTimeout (struct Port *port, SopType sop, FSC_U32 *arr, FSC_U32 length, PolicyState_t n_pe);

/*
 * Initiations from DPM
 * Issues Discover Identity commands
 * Discover Identity only valid for SOP/SOP'
 * returns 0 if successful
 * returns > 0 if not SOP or SOP', or if Policy State is wrong
 */
FSC_S32 FUSB3601_RequestDiscoverIdentity(struct Port *port, SopType sop);

/*  Issues Discover SVID commands, valid with SOP* */
FSC_S32 FUSB3601_RequestDiscoverSvids(struct Port *port, SopType sop);

/*  Issues Discover Modes command, valid with SOP*. */
FSC_S32 FUSB3601_RequestDiscoverModes(struct Port *port, SopType sop, FSC_U16 svid);

/*  DPM (UFP) calls this to request sending an attention command to specified sop */
FSC_S32 FUSB3601_RequestSendAttention(struct Port *port, SopType sop, FSC_U16 svid, FSC_U8 mode);

/*  Enter mode specified by SVID and mode index */
FSC_S32 FUSB3601_RequestEnterMode(struct Port *port, SopType sop, FSC_U16 svid, FSC_U32 mode_index);

/*  exit mode specified by SVID and mode index */
FSC_S32 FUSB3601_RequestExitMode(struct Port *port, SopType sop, FSC_U16 svid, FSC_U32 mode_index);

/*  exits all modes (TODO) */
FSC_S32 FUSB3601_RequestExitAllModes(struct Port *port);

/*  receiving end */
/*  Call when VDM messages are received */
FSC_S32 FUSB3601_ProcessVdmMessage (struct Port *port);/*  returns 0 on success, 1+ otherwise */
FSC_S32 FUSB3601_ProcessDiscoverIdentity (struct Port *port, SopType sop);
FSC_S32 FUSB3601_ProcessDiscoverSvids (struct Port *port, SopType sop);
FSC_S32 FUSB3601_ProcessDiscoverModes (struct Port *port, SopType sop);
FSC_S32 FUSB3601_ProcessEnterMode (struct Port *port, SopType sop);
FSC_S32 FUSB3601_ProcessExitMode (struct Port *port, SopType sop);
FSC_S32 FUSB3601_ProcessAttention (struct Port *port, SopType sop);
FSC_S32 FUSB3601_ProcessSvidSpecific (struct Port *port, SopType sop);

void FUSB3601_StartVdmTimer (struct Port *port);
void FUSB3601_ResetPolicyState (struct Port *port);

/*  TODO: These are the "vdm callback" functions from the 302 - make sure this is the best place to put these */
/*  VDM handling functions that will likely be customized by vendors */
Identity FUSB3601_VdmRequestIdentityInfo (struct Port *port);
SvidInfo FUSB3601_VdmRequestSvidInfo (struct Port *port);
ModesInfo FUSB3601_VdmRequestModesInfo (struct Port *port, FSC_U16 svid);
FSC_BOOL FUSB3601_VdmModeEntryRequest (struct Port *port, FSC_U16 svid, FSC_U32 mode_index);
FSC_BOOL FUSB3601_VdmModeExitRequest (struct Port *port, FSC_U16 svid, FSC_U32 mode_index);
FSC_BOOL FUSB3601_VdmEnterModeResult (struct Port *port, FSC_BOOL success, FSC_U16 svid, FSC_U32 mode_index);
void FUSB3601_VdmExitModeResult (struct Port *port, FSC_BOOL success, FSC_U16 svid, FSC_U32 mode_index);
void FUSB3601_VdmInformIdentity (struct Port *port, FSC_BOOL success, SopType sop, Identity id);
void FUSB3601_VdmInformSvids (struct Port *port, FSC_BOOL success, SopType sop, SvidInfo svid_info);
void FUSB3601_VdmInformModes (struct Port *port, SopType sop, ModesInfo modes_info);
void FUSB3601_VdmInformAttention (struct Port *port, FSC_U16 svid, FSC_U8 mode_index);
void FUSB3601_VdmInitDpm (struct Port *port);

FSC_U32 FUSB3601_GetVdmTimer(Command command);

#endif /*  FSC_HAVE_VDM */
#endif /*  FSCPM_VDM_H_ */
