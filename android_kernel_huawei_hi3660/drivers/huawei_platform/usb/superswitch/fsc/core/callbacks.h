/*
 * callbacks.h
 *
 * Defines a set of callbacks to be implemented/handled by the platform.
 */
#ifndef FSC_CALLBACKS_H_
#define FSC_CALLBACKS_H_

#include "port.h"

void FUSB3601_platform_do_something_with_port(struct Port *port);

/*******************************************************************************
* Function:        platform_notify_cc_orientation
* Input:           orientation - Orientation of CC (NONE, CC1, CC2)
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current CC orientation. Called in SetStateAttached... and
*                  SetStateUnattached functions.
******************************************************************************/
void FUSB3601_platform_notify_cc_orientation(CCOrientation orientation);

void FUSB3601_platform_notify_audio_accessory(void);
void FUSB3601_platform_notify_attached_vbus_only(void);
void FUSB3601_platform_notify_unattached_vbus_only(void);
void FUSB3601_platform_notify_double_56k(void);
void FUSB3601_platform_notify_DebugAccessorySink(void);
/*******************************************************************************
* Function:        platform_notify_pd_contract
* Input:           contract - TRUE: Contract, FALSE: No Contract
* Return:          None
* Description:     A callback used by the core to report to the platform the
*                  current PD contract status. Called in PDPolicy.
*******************************************************************************/
void FUSB3601_platform_notify_pd_contract(FSC_BOOL contract, FSC_U32 pd_voltage, FSC_U32 pd_current, FSC_BOOL externally_powered);

void FUSB3601_platform_notify_pd_state(SourceOrSink state);

/*******************************************************************************
* Function:        platform_notify_unsupported_accessory
* Input:           None
* Return:          None
* Description:     A callback used by the core to report entry to the
*                  Unsupported Accessory state. The platform may implement
*                  USB Billboard.
*******************************************************************************/
void FUSB3601_platform_notify_unsupported_accessory(void);

/*******************************************************************************
* Function:        platform_notify_data_role
* Input:           PolicyIsDFP - Current data role
* Return:          None
* Description:     A callback used by the core to report the new data role after
*                  a data role swap.
*******************************************************************************/
void FUSB3601_platform_notify_data_role(FSC_BOOL PolicyIsDFP);

/*******************************************************************************
* Function:        platform_notify_bist
* Input:           bistEnabled - TRUE when BIST enabled, FALSE when disabled
* Return:          None
* Description:     A callback that may be used to limit current sinking during
*                  BIST
*******************************************************************************/
void FUSB3601_platform_notify_bist(FSC_BOOL bistEnabled);

void FUSB3601_platform_notify_sink_current(struct Port *port);

#endif /* FSC_CALLBACKS_H_ */
