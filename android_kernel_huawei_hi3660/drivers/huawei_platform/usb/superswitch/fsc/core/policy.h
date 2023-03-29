/*
 * policy.h
 *
 * Defines functionality for the Policy Engine state machine.
 */
#ifndef FSCPM_POLICY_H_
#define FSCPM_POLICY_H_

#include "platform.h"
#include "port.h"

void FUSB3601_USBPDPolicyEngine(struct Port *port);
void FUSB3601_PolicyErrorRecovery(struct Port *port);

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
void FUSB3601_PolicySourceSendHardReset(struct Port *port);
void FUSB3601_PolicySourceSendSoftReset(struct Port *port);
void FUSB3601_PolicySourceSoftReset(struct Port *port);
void FUSB3601_PolicySourceStartup(struct Port *port);
void FUSB3601_PolicySourceDiscovery(struct Port *port);
void FUSB3601_PolicySourceSendCaps(struct Port *port);
void FUSB3601_PolicySourceDisabled(struct Port *port);
void FUSB3601_PolicySourceTransitionDefault(struct Port *port);
void FUSB3601_PolicySourceNegotiateCap(struct Port *port);
void FUSB3601_PolicySourceTransitionSupply(struct Port *port);
void FUSB3601_PolicySourceCapabilityResponse(struct Port *port);
void FUSB3601_PolicySourceReady(struct Port *port);
void FUSB3601_PolicySourceGiveSourceCap(struct Port *port);
void FUSB3601_PolicySourceGetSinkCap(struct Port *port);
void FUSB3601_PolicySourceSendPing(struct Port *port);
void FUSB3601_PolicySourceGotoMin(struct Port *port);
void FUSB3601_PolicySourceGiveSinkCap(struct Port *port);
void FUSB3601_PolicySourceGetSourceCap(struct Port *port);
void FUSB3601_PolicySourceSendDRSwap(struct Port *port);
void FUSB3601_PolicySourceEvaluateDRSwap(struct Port *port);
void FUSB3601_PolicySourceSendVCONNSwap(struct Port *port);
void FUSB3601_PolicySourceSendPRSwap(struct Port *port);
void FUSB3601_PolicySourceEvaluatePRSwap(struct Port *port);
void FUSB3601_PolicySourceWaitNewCapabilities(struct Port *port);
void FUSB3601_PolicySourceEvaluateVCONNSwap(struct Port *port);
#endif /* FSC_HAVE_SRC || (FSC_HAVE_SNK && FSC_HAVE_ACC) */
#ifdef FSC_HAVE_SNK
void FUSB3601_PolicySinkStartup(struct Port *port);
void FUSB3601_PolicySinkSendHardReset(struct Port *port);
void FUSB3601_PolicySinkSoftReset(struct Port *port);
void FUSB3601_PolicySinkSendSoftReset(struct Port *port);
void FUSB3601_PolicySinkTransitionDefault(struct Port *port);
void FUSB3601_PolicySinkDiscovery(struct Port *port);
void FUSB3601_PolicySinkWaitCaps(struct Port *port);
void FUSB3601_PolicySinkEvaluateCaps(struct Port *port);
void FUSB3601_PolicySinkSelectCapability(struct Port *port);
void FUSB3601_PolicySinkTransitionSink(struct Port *port);
void FUSB3601_PolicySinkReady(struct Port *port);
void FUSB3601_PolicySinkGiveSinkCap(struct Port *port);
void FUSB3601_PolicySinkGetSourceCap(struct Port *port);
void FUSB3601_PolicySinkGetSinkCap(struct Port *port);
void FUSB3601_PolicySinkGiveSourceCap(struct Port *port);
void FUSB3601_PolicySinkSendDRSwap(struct Port *port);
void FUSB3601_PolicySinkEvaluateDRSwap(struct Port *port);
void FUSB3601_PolicySinkEvaluateVCONNSwap(struct Port *port);
void FUSB3601_PolicySinkSendPRSwap(struct Port *port);
void FUSB3601_PolicySinkEvaluatePRSwap(struct Port *port);
#endif /* FSC_HAVE_SNK */

void FUSB3601_PolicyDfpCblVdmIdentityRequest(struct Port *port);

void FUSB3601_UpdateCapabilitiesRx(struct Port *port, FSC_BOOL is_source_cap_update);



/* BIST functionality */
void FUSB3601_ProcessDmtBist(struct Port *port);
void FUSB3601_PolicyBISTReceiveMode(struct Port *port);   /* Not Implemented */
void FUSB3601_PolicyBISTFrameReceived(struct Port *port); /* Not Implemented */
void FUSB3601_PolicyBISTCarrierMode2(struct Port *port);
void FUSB3601_PolicyBISTTestData(struct Port *port);
void FUSB3601_PolicyInvalidState(struct Port *port);

/*  VDM policy state machine */
void FUSB3601_PolicyVdm(struct Port *port);
/*  Handles the port's peGiveVdm policy state */
void FUSB3601_PolicyGiveVdm(struct Port *port);
/*  Converts the port's VDM message and processes it */

void FUSB3601_PolicySourceStartupHelper(struct Port *port);

#endif /* FSCPM_POLICY_H_ */
void FUSB3601_SetPDLimitVoltage(int vol);
extern void pd_dpm_set_optional_max_power_status(bool status);