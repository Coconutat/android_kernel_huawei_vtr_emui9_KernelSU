/*
 * systempolicy.h
 *
 * Defines a class that contains the SystemPolicy interface
 */
#ifndef FSCPM_SYSTEMPOLICY_H_
#define FSCPM_SYSTEMPOLICY_H_

#include "platform.h"
#include "port.h"

/* Called from the while(1) loop, process events as needed */
void FUSB3601_SystemPolicyProcess(struct Port *port);

#endif /* FSCPM_SYSTEMPOLICY_H_ */

