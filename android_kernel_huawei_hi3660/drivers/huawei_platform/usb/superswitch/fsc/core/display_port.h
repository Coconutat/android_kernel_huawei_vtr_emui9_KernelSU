/*
 *  display_port.h
 *
 *  Declares the display port implementation.
 */
#ifndef FSCPM_DISPLAY_PORT_H_
#define FSCPM_DISPLAY_PORT_H_
#ifdef FSC_HAVE_DP

#include "display_port_types.h"
#include "platform.h"
#include "port.h"

/*  Interface used by VDM/policy engines */
/*  Initiate status request - call to get status of port partner */
void FUSB3601_requestDpStatus(struct Port *port);
/*  Initiate config request - call to configure port partner */
void FUSB3601_requestDpConfig(struct Port *port);

/*  Hostcomm interface functionality */
void FUSB3601_WriteDpControls(struct Port *port, FSC_U8* data);
void FUSB3601_ReadDpControls(struct Port *port, FSC_U8* data);
void FUSB3601_ReadDpStatus(struct Port *port, FSC_U8* data);

/*  Resets the port's DP structure(s) */
void FUSB3601_resetDp(struct Port *port);

/*  Process special DisplayPort commands. */
/*  Returns TRUE when the message isn't processed and FALSE otherwise */
FSC_BOOL FUSB3601_processDpCommand(struct Port *port, FSC_U32* arr_in);

/*  Evaluate a mode VDO for mode entry */
/*  Returns TRUE if mode can be entered, FALSE otherwise */
FSC_BOOL FUSB3601_dpEvaluateModeEntry(struct Port *port, FSC_U32 mode_in);

/*  Configure DP */
void FUSB3601_configDp(struct Port *port, FSC_BOOL enabled, FSC_U32 status);

void FUSB3601_configAutoDpModeEntry(struct Port *port, FSC_BOOL enabled,
                           FSC_U32 mask, FSC_U32 value);

/*  Internal helper functions */
/*  Send status data to port partner */
void FUSB3601_sendStatusData(struct Port *port, doDataObject_t svdmh_in);

/*  Reply to a config request (to port partner) */
void FUSB3601_replyToConfig(struct Port *port, doDataObject_t svdmh_in,
                   FSC_BOOL success);

/*  Internal DP functionality to be customized per system */
void FUSB3601_informStatus(struct Port *port, DisplayPortStatus_t stat);
void FUSB3601_updateStatusData(struct Port *port);
void FUSB3601_informConfigResult (struct Port *port, FSC_BOOL success);
FSC_BOOL FUSB3601_DpReconfigure(struct Port *port, DisplayPortConfig_t config);
#endif /*  FSC_HAVE_DP */
#endif /*  FSCPM_DISPLAY_PORT_H_ */
