/*
 * typec.h
 *
 * Defines the Type-C state machine functions
 */
#ifndef FSCPM_TYPEC_H_
#define FSCPM_TYPEC_H_

#include "platform.h"
#include "port.h"

#define SLEEP_DELAY_US    80    /* 0.08ms */
#define REDOBC12_MAX_CNT  8

void FUSB3601_StateMachineTypeC(struct Port *port);
void FUSB3601_StateMachineDisabled(struct Port *port);
void FUSB3601_StateMachineErrorRecovery(struct Port *port);
void FUSB3601_StateMachineDelayUnattached(struct Port *port);
void FUSB3601_StateMachineUnattached(struct Port *port);

#ifdef FSC_HAVE_SNK
void FUSB3601_StateMachineAttachWaitSink(struct Port *port);
void FUSB3601_StateMachineAttachedSink(struct Port *port);
void FUSB3601_StateMachineTryWaitSink(struct Port *port);
void FUSB3601_StateMachineTrySink(struct Port *port);
#endif /* FSC_HAVE_SNK */

#ifdef FSC_HAVE_SRC
void FUSB3601_StateMachineAttachWaitSource(struct Port *port);
void FUSB3601_StateMachineTryWaitSource(struct Port *port);
/* UnattachedSource is a temporary workaround for entering */
/* DRP in Source mode - not part of GUI */
void FUSB3601_StateMachineUnattachedSource(struct Port *port);
void FUSB3601_StateMachineAttachedSource(struct Port *port);
void FUSB3601_StateMachineTrySource(struct Port *port);
#endif /* FSC_HAVE_SRC */

#ifdef FSC_HAVE_ACC
void FUSB3601_StateMachineAttachWaitAccessory(struct Port *port);
void FUSB3601_StateMachineDebugAccessorySource(struct Port *port);
void FUSB3601_StateMachineDebugAccessorySink(struct Port *port);
void FUSB3601_StateMachineAudioAccessory(struct Port *port);
void FUSB3601_StateMachinePoweredAccessory(struct Port *port);
void FUSB3601_StateMachineUnsupportedAccessory(struct Port *port);
#endif /* FSC_HAVE_ACC */

void FUSB3601_StateMachineIllegalCable(struct Port *port);

void FUSB3601_SetStateDisabled(struct Port *port);
void FUSB3601_SetStateErrorRecovery(struct Port *port);
void FUSB3601_SetStateDelayUnattached(struct Port *port);
void FUSB3601_SetStateUnattached(struct Port *port);

#ifdef FSC_HAVE_SNK
void FUSB3601_SetStateAttachWaitSink(struct Port *port);
void FUSB3601_SetStateAttachedSink(struct Port *port);
#ifdef FSC_HAVE_DRP
void FUSB3601_RoleSwapToAttachedSink(struct Port *port);
#endif /* FSC_HAVE_DRP */
void FUSB3601_SetStateTryWaitSink(struct Port *port);
void FUSB3601_SetStateTrySink(struct Port *port);
#endif /* FSC_HAVE_SNK */

#ifdef FSC_HAVE_SRC
void FUSB3601_SetStateAttachWaitSource(struct Port *port);
void FUSB3601_SetStateAttachedSource(struct Port *port);
#ifdef FSC_HAVE_DRP
void FUSB3601_RoleSwapToAttachedSource(struct Port *port);
#endif /* FSC_HAVE_DRP */
void FUSB3601_SetStateTrySource(struct Port *port);
void FUSB3601_SetStateTryWaitSource(struct Port *port);
/* UnattachedSource is a temporary workaround for entering */
/* DRP in Source mode - not part of GUI */
#ifdef FSC_HAVE_DRP
void FUSB3601_SetStateUnattachedSource(struct Port *port);
#endif /* FSC_HAVE_DRP */
#endif /* FSC_HAVE_SRC */

#if defined(FSC_HAVE_ACC) && (defined(FSC_HAVE_SNK) || defined(FSC_HAVE_SRC))
void FUSB3601_SetStateAttachWaitAccessory(struct Port *port);
void FUSB3601_SetStateDebugAccessorySource(struct Port *port);
void FUSB3601_SetStateDebugAccessorySink(struct Port *port);
void FUSB3601_SetStateAudioAccessory(struct Port *port);
void FUSB3601_SetStatePoweredAccessory(struct Port *port);
void FUSB3601_SetStateUnsupportedAccessory(struct Port *port);
#endif /* FSC_HAVE_ACC || (FSC_HAVE_SNK && FSC_HAVE_SRC) */

void FUSB3601_SetStateIllegalCable(struct Port *port);

#endif /* FSCPM_TYPEC_H_ */

