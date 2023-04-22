/* **************************************************************************
 * bc12.cpp
 *
 * Implements the BC 1.2 functionality.
 * ************************************************************************** */

#include "bc12.h"
#include "port.h"
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger.h>

#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();

void ConnectBC12(struct Port *port)
{
	FUSB3601_ClearInterrupt(port, regMUS_INTERRUPT, MSK_I_BC_ATTACH);
	port->bc12_active_ = TRUE;

	/* Mask the plug-in interrupt and unmask the disconnect interrupt */
	port->registers_.MUSIntMask.M_BC_ATTACH = 0;
	port->registers_.MUSIntMask.M_BC_DETACH = 1;
	FUSB3601_WriteRegister(port, regMUS_INTERRUPT_MSK);
	FUSB3601_ReadRegister(port, regDEVICE_TYPE);
	if (port->registers_.DeviceType.Dcp) {
		hwlog_info("dcp detected\n");
		charge_type_dcp_detected_notify();
	}
	FUSB3601_platform_log(port->port_id_, "Connect: BC 1.2", -1);
}

void ProcessBC12(struct Port *port)
{
	/* Detach? */
	if (port->registers_.MUSInterrupt.I_BC_DETACH) {
		FUSB3601_ClearInterrupt(port, regMUS_INTERRUPT, MSK_I_BC_DETACH);
		/* Unmask the plug-in interrupt and mask the disconnect interrupt */
		port->registers_.MUSIntMask.M_BC_ATTACH = 1;
    		port->registers_.MUSIntMask.M_BC_DETACH = 0;
    		FUSB3601_WriteRegister(port, regMUS_INTERRUPT_MSK);
		port->bc12_active_ = FALSE;
		FUSB3601_platform_log(port->port_id_, "Disconnect: BC 1.2", -1);
	}

	if (port->registers_.Event1.DPDN_PLGIN) {
		FUSB3601_ClearInterrupt(port, regEVENT_1, MSK_DPDN_PLGIN);
	}
	if (port->registers_.Event1.CC_PLGIN) {
		FUSB3601_ClearInterrupt(port, regEVENT_1, MSK_CC_PLGIN);
	}

	/* TODO - Other BC 1.2 handling? */
}

