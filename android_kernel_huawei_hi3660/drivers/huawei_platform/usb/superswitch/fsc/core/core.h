
#ifndef _FSC_CORE_H_
#define _FSC_CORE_H_

#include "port.h"
#include "typec.h"
#include "log.h"
#include "version.h"
#include "../Platform_Linux/FSCTypes.h"

typedef enum {
	DEVICE_ROLE_UFP_ONLY = 0,
	DEVICE_ROLE_DFP_ONLY,
	DEVICE_ROLE_DRP,
} device_role;
#define FUSB3601_PORT_TYPE_DRP 0x96
#define FUSB3601_PORT_TYPE_TRY_SNK 0xD6
#define FUSB3601_PORT_TYPE_TRY_SRC 0x9E
#define FUSB3601_PORT_TYPE_SET_SRC 0x95
#define FUSB3601_PORT_TYPE_SET_SNK 0x90
device_role fusb3601_get_device_role(void);
void FUSB3601_core_initialize(struct Port *port);
void FUSB3601_core_state_machine(struct Port *port);

FSC_U32 FUSB3601_core_get_next_timeout(struct Port *port);

void FUSB3601_core_enable_typec(struct Port *port, FSC_BOOL enable);
void FUSB3601_core_send_hard_reset(struct Port *port);

#ifdef FSC_DEBUG

void FUSB3601_core_set_state_unattached(struct Port *port);
void FUSB3601_core_reset_pd(void);

FSC_U16 FUSB3601_core_get_advertised_current(void);

FSC_U8 FUSB3601_core_get_cc_orientation(void);
#endif // FSC_DEBUG

void FUSB3601_core_redo_bc12(struct Port *port);
void FUSB3601_core_redo_bc12_limited(struct Port *port);
bool FUSB3601_in_factory_mode(void);

#endif /* _FSC_CORE_H_ */

