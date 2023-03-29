#include "core.h"
#include "port.h"
#include "typec.h"
#include "bc12.h"
#include "scp.h"
#include "accp.h"
#include "moisture_detection.h"
#include "../Platform_Linux/platform_helpers.h"
#include "../Platform_Linux/fusb3601_global.h"
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger.h>
#include <linux/delay.h>
#define HWLOG_TAG FUSB3601_TAG
HWLOG_REGIST();
#ifdef PLATFORM_ARM
#include "stdlib.h"
#endif

#define LATENCY_MAX 6000
#define DEAD_LOOP_CNT 50
static FSC_U8 AlertL;
static FSC_U8 AlertH;
static FSC_U8 CCSTAT;
static FSC_U8 PWRSTAT;
static FSC_U8 FAULTSTAT;
static int counter = 0;
int state_machine_need_resched = 0;
int ignore_unattach_once = 0; /*if reset happened,do not report unattach event to the system*/
void FUSB3601_dump_register(void);
FSC_U16 FUSB3601_GetVBusVoltage(struct Port *port);
bool FUSB3601_in_factory_mode(void)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port *port;

	if (!chip) {
		pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
		return false;
	}
	port = &chip->port;
	if (!port) {
		pr_err("FUSB  %s - port structure is NULL!\n", __func__);
		return false;
	}
	if (port->factory_mode_) {
		return true;
	}
	FUSB3601_ReadRegisters(port, regALERTL, 2);
	hwlog_info("FUSB %s - AlertL: [0x%x], AlertH: [0x%x] \n",__func__, port->registers_.AlertL.byte, port->registers_.AlertH.byte);
	if(port->registers_.AlertH.byte == 0x79){
		hwlog_info("%s ture \n",__func__);
		return true;
	}
	return false;
}
void FUSB3601_core_set_drp(struct Port *port)
{
	FUSB3601_ConfigurePortType(0x96, port);
}
void FUSB3601_core_set_try_snk(struct Port *port)
{
	FUSB3601_ConfigurePortType(0xD6, port);
}
void FUSB3601_core_set_try_src(struct Port *port)
{
	FUSB3601_ConfigurePortType(0x9E, port);
}
void FUSB3601_core_set_source(struct Port *port)
{
	FUSB3601_ConfigurePortType(0x95, port);
}
void FUSB3601_core_set_sink(struct Port *port)
{
	FUSB3601_ConfigurePortType(0x90, port);
}
void FUSB3601_core_initialize(struct Port *port)
{
	FUSB3601_InitializeVars(port, 1, 0x4A);
	FUSB3601_InitializePort(port);
}

void FUSB3601_core_state_machine(struct Port *port)
{
	FSC_U8 data;
	int ret;

	/* Read Type-C/PD interrupts - AlertL, AlertH */
	if((port->registers_.AlertMskL.byte || port->registers_.AlertMskH.byte)	&& ((FUSB3601_platform_get_device_irq_state(port->port_id_) || state_machine_need_resched))){
		state_machine_need_resched = 0;
		FUSB3601_ReadRegisters(port, regALERTL, 2);
		FUSB3601_ReadRegisters(port, regCCSTAT, 3);
		if (port->registers_.AlertL.byte == AlertL && port->registers_.AlertH.byte == AlertH && port->registers_.CCStat.byte == CCSTAT && port->registers_.PwrStat.byte == PWRSTAT && port->registers_.FaultStat.byte == FAULTSTAT) {
			counter++;
		} else {
			counter = 0;
		}
		AlertL = port->registers_.AlertL.byte;
		AlertH = port->registers_.AlertH.byte;
		CCSTAT = port->registers_.CCStat.byte;
		PWRSTAT =  port->registers_.PwrStat.byte;
		FAULTSTAT = port->registers_.FaultStat.byte;
		hwlog_info("FUSB %s - counter:[%d], AlertL: [0x%x], AlertH: [0x%x] \n",__func__, counter, port->registers_.AlertL.byte, port->registers_.AlertH.byte);
		/* Read Type-C/PD statuses - CCStat, PwrStat, FaultStat */
		if(port->registers_.AlertL.I_CCSTAT || port->registers_.AlertL.I_PORT_PWR || port->registers_.AlertH.I_FAULT) {
			FUSB3601_ReadRegisters(port, regCCSTAT, 3);
			if (counter < DEAD_LOOP_CNT) {
				hwlog_info("FUSB %s - CCStat: [0x%x], PwrStat: [0x%x] FaultStat: [0x%x]\n",__func__, port->registers_.CCStat.byte, port->registers_.PwrStat.byte, port->registers_.FaultStat.byte);
			}
		  }

		/* Check and handle a chip reset */
		if (port->registers_.FaultStat.ALL_REGS_RESET) {
			hwlog_info("full chip reset detected\n");
			ignore_unattach_once = 1;
			FUSB3601_WriteTCState(&port->log_, 555, port->tc_state_);
			FUSB3601_InitializeVars(port, 1, 0x4A);
			FUSB3601_InitializePort(port);
			return;
		}
		/* Check for factory mode */
		if(port->registers_.AlertH.byte == 0x79){
			port->factory_mode_ = TRUE;
			FUSB3601_WriteTCState(&port->log_, 498, port->tc_state_);
			FUSB3601_platform_delay(10 * 1000);
		}
	}

	//TODO: Stick this in with the above interrupts, but also needs a timer interrupt flag
	/* Start with TypeC/PD state machines */
	if(!port->factory_mode_) {
		FUSB3601_StateMachineTypeC(port);

		/* Fault Handling */
		if(port->registers_.AlertH.I_FAULT) {
			hwlog_info("FUSB %s - Fault Interrupt, FaultStat: [0x%x]\n",__func__, port->registers_.FaultStat.byte);
			if (port->registers_.FaultStat.VBUS_OVP) {
				hwlog_info("FUSB %s - VBUS_OVP Interrupt, ADC Voltage: [%d]\n",__func__,FUSB3601_GetVBusVoltage(port));
				superswitch_dsm_report(ERROR_SUPERSWITCH_VBUS_OVP);
			}
			if (port->registers_.FaultStat.VCONN_OCP) {
				hwlog_info("FUSB %s - VCONN_OCP Interrupt\n",__func__);
				superswitch_dsm_report(ERROR_SUPERSWITCH_VCONN_OCP);
			}
			FUSB3601_ClearInterrupt(port, regFAULTSTAT, MSK_FAULTSTAT_ALL);
			FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_FAULT);
		}
	}

	/* Read MUS interrupts */
	if(port->registers_.MUSIntMask.byte) {
		FUSB3601_ReadRegister(port, regMUS_INTERRUPT);
		/* Check BC1.2 Attach status */
		if (port->registers_.MUSInterrupt.I_BC_ATTACH) {
			ConnectBC12(port);
		} else if (port->bc12_active_) {
			ProcessBC12(port);
		} else if(port->registers_.MUSInterrupt.I_H2O_DET) {
			moisture_detection_complete();
		}
		/* Process MUS */
		if (port->registers_.MUSInterrupt.byte) {
			hwlog_info("MUS interrupt reg = 0x%x\n", port->registers_.MUSInterrupt.byte);
			FUSB3601_ClearInterrupt(port, regMUS_INTERRUPT, MSK_MUS_ALL);
			if (port->registers_.MUSInterrupt.I_SCP_EVENT == 1) {
				FUSB3601_ReadRegister(port, regEVENT_1);
				if (port->registers_.Event1.CC_PLGIN == 1) {
					if(port->tc_state_ != AttachedSink && port->tc_state_ != DebugAccessorySink) {
						hwlog_info("MUS interrupt error happened\n");
						FUSB3601_dump_register();
					}
				}
			}
		}
	}

	/* Read OVP interrupts */
	if(port->registers_.DataOVPMsk.byte) {
		FUSB3601_ReadRegister(port, regDATA_OVP_INT);
		/* Process Vendor Defined */
		if (port->registers_.DataOVPInt.byte) {
			hwlog_info("data ovp reg = 0x%x\n", port->registers_.DataOVPInt.byte);
			FUSB3601_ClearInterrupt(port, regDATA_OVP_INT, MSK_OVP_ALL);
		}
	}
}

void FUSB3601_core_enable_typec(struct Port *port, FSC_BOOL enable)
{
	port->tc_enabled_ = enable;
}

FSC_U32 FUSB3601_core_get_next_timeout(struct Port *port)
{
	FSC_U32 time = 0;
	FSC_U32 nexttime = 0xFFFFFFFF;

	time = FUSB3601_TimerRemaining(&port->tc_state_timer_);
	if (time > 0 && time < nexttime)
		nexttime = time;
	time = FUSB3601_TimerRemaining(&port->policy_state_timer_);
	if (time > 0 && time < nexttime)
		nexttime = time;
	time = FUSB3601_TimerRemaining(&port->pd_debounce_timer_);
	if (time > 0 && time < nexttime)
		nexttime = time;
	time = FUSB3601_TimerRemaining(&port->cc_debounce_timer_);
	if (time > 0 && time < nexttime)
		nexttime = time;
	time = FUSB3601_TimerRemaining(&port->vdm_timer_);
	if (time > 0 && time < nexttime)
		nexttime = time;
	if (nexttime == 0xFFFFFFFF)
		nexttime = 0;

	return nexttime;
}

/*
 * Call this function to get the lower 8-bits of the core revision number.
 */
FSC_U8 FUSB3601_core_get_rev_lower(void)
{
	return FSC_TYPEC_CORE_FW_REV_LOWER;
}

/*
 * Call this function to get the middle 8-bits of the core revision number.
 */
FSC_U8 FUSB3601_core_get_rev_middle(void)
{
	return FSC_TYPEC_CORE_FW_REV_MIDDLE;
}

/*
 * Call this function to get the upper 8-bits of the core revision number.
 */
FSC_U8 FUSB3601_core_get_rev_upper(void)
{
	return FSC_TYPEC_CORE_FW_REV_UPPER;
}

FSC_U8 FUSB3601_core_get_cc_orientation(void)
{
	/* TODO */
	return 0;
}

void FUSB3601_core_send_hard_reset(struct Port *port)
{
	/* Set the send hard reset TRANSMIT register code */
	FSC_U8 data = TRANSMIT_HARDRESET;

	/* Send the hard reset */
	FUSB3601_platform_i2c_write(port->port_id_, regTRANSMIT, 1, &data);
}

void FUSB3601_core_set_state_unattached(struct Port *port)
{
	FUSB3601_SetStateErrorRecovery(port);
}

void FUSB3601_core_redo_bc12(struct Port *port)
{
	struct fusb3601_chip* chip = fusb3601_GetChip();
	if (!chip) {
        	pr_err("FUSB  %s - Chip structure is NULL!\n", __func__);
        	return;
	}
	if (get_stop_charge_sync_flag()) {
		pr_err("FUSB  %s - charge stop, no need to redo bc12\n", __func__);
		return;
	}
	if (charge_get_hiz_state()) {
		pr_err("%s - hiz_enable, return!\n", __func__);
		return;
	}
	pr_info("%s ++\n", __func__);
	down(&chip->suspend_lock);
	if((port->tc_state_ == AttachedSink) || (port->tc_state_ == DebugAccessorySink)) {
		FUSB3601_ReadRegister(port,regCCSTAT);
		if((port->registers_.CCStat.CC1_STAT == 0) && (port->registers_.CCStat.CC2_STAT == 0)){
			pr_info("%s delay 200ms\n", __func__);
			msleep(200);
		}
		FUSB3601_set_usb_switch(port, NONE);
		port->registers_.PwrCtrl.AUTO_DISCH = 0;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		msleep(1);
		FUSB3601_SendCommand(port, DisableSinkVbus);
		msleep(1);
		FUSB3601_SendCommand(port, DisableVbusDetect);
		msleep(1);
		FUSB3601_SendCommand(port, EnableVbusDetect);
		msleep(1);
		port->registers_.PwrCtrl.AUTO_DISCH = 1;
		FUSB3601_WriteRegister(port, regPWRCTRL);
		msleep(1);
		FUSB3601_SendCommand(port, SinkVbus);
	}
	usleep_range(800000,801000);
	up(&chip->suspend_lock);
	pr_info("%s --\n", __func__);
}

void FUSB3601_core_redo_bc12_limited(struct Port *port)
{
	if (port->redobc12_cnt >= REDOBC12_MAX_CNT) {
		return;
	}
	FUSB3601_core_redo_bc12(port);
	port->redobc12_cnt++;
}
