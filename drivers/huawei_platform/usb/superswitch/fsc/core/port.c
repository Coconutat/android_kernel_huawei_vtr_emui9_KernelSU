/* **************************************************************************
 * port.cpp
 *
 * Implements the port interface for the port manager.
 * ************************************************************************** */

#include "port.h"
#include "policy.h"
#include "callbacks.h"
#include "vendor_info.h"
#include <linux/init.h>
#include <linux/string.h>
#include <linux/mutex.h>
#include "../Platform_Linux/fusb3601_global.h"
#include "../Platform_Linux/platform_helpers.h"

#include <huawei_platform/power/power_dsm.h>

/* Initialize
 *
 * Initializes the port data object's members, populates the register map with
 * I2C read data from the device, configures capability objects, and writes
 * initial configuration values to the device.
 */
#if defined(CONFIG_HUAWEI_DSM)
#define SUPERSWITCH_DMDLOG_SIZE      (2048)
#define SUPERSWITCH_DMD_STRING_SIZE 128
static int reg_addr[] = {
	0x10, 0x11, 0x12, 0x13, 0x14, 0x15,
	0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d,
	0x1e, 0x1f, 0x80, 0x81, 0x82, 0x83,
	0x86, 0x87, 0x88, 0xd1, 0xd2, 0xd3,
	0xd4, 0xd5, 0xd6, 0xdc, 0xdd, 0xde,
	0xdf, 0xe0, 0xe1, 0xe7, 0xe8, 0xe9
};
#endif
static DEFINE_MUTEX(FMControl4_mutex);
void FUSB3601_dump_register(void);
int get_source_max_output_current(void);
#if defined(CONFIG_HUAWEI_DSM)
void superswitch_dsm_report(int num)
{
	FSC_U8 i = 0;
	FSC_U8 data;
	int ret;
	char dsm_buff[SUPERSWITCH_DMDLOG_SIZE] = { 0 };
	char buf[SUPERSWITCH_DMD_STRING_SIZE] = { 0 };
	struct fusb3601_chip* chip = fusb3601_GetChip();
	struct Port* port;
	if (!chip) {
		pr_err("FUSB  %s - Error: Chip structure is NULL!\n", __func__);
		return;
	}
	port = &chip->port;
	if (!port) {
		pr_err("FUSB  %s - Error: port structure is NULL!\n", __func__);
		return;
	}

	snprintf(dsm_buff, sizeof(dsm_buff), "tc_state= 0x%x\n", port->tc_state_);

	for (i = 0; i < sizeof(reg_addr)/sizeof(reg_addr[0]); ++i) {
		(void)FUSB3601_fusb_I2C_ReadData(reg_addr[i],&data);
		memset(buf, 0, sizeof(buf));
		snprintf(buf, sizeof(buf), "reg[0x%x]= 0x%x\n", reg_addr[i], data);
		strncat(dsm_buff, buf, strlen(buf));
	}

	power_dsm_dmd_report(POWER_DSM_SUPERSWITCH, num, dsm_buff);
}
#endif

void FUSB3601_InitializeVars(struct Port *port, FSC_U8 id, FSC_U8 i2c_addr)
{
	FSC_U32 i = 0;

	port->port_id_ = id;
	port->i2c_addr_ = i2c_addr;
	port->idle_ = TRUE;
	port->initialized_ = FALSE;

	port->bc12_active_ = FALSE;
	port->activemode_ = Mode_None;
	port->watchdogenabled_ = FALSE;

	port->accp_state_ = ACCP_Idle;
	port->accp_substate_ = 0;
	port->accp_waitonack_ = FALSE;

	port->scp_state_ = SCP_Idle;
	port->scp_substate_ = 0;
	port->scp_waitonack_ = FALSE;

	port->port_type_ = USBTypeC_UNDEFINED;
	port->source_or_sink_ = Source;
	port->tc_enabled_ = TRUE;
	port->tc_state_ = Disabled;
	port->tc_substate_ = 0;
	port->src_preferred_ = FALSE;
	port->snk_preferred_ = TRUE;
	port->acc_support_ = TRUE;
	port->snk_current_ = utccOpen;
	port->src_current_ = utccDefault;
	port->cc_term_ = CCTypeUndefined;
	port->is_hard_reset_ = FALSE;
	port->is_pr_swap_ = FALSE;
	port->is_vconn_swap_ = FALSE;
	port->unattach_loop_counter_ = 0;
	port->pd_active_ = FALSE;
	port->pd_enabled_ = TRUE;
	port->protocol_state_ = PRLDisabled;
	port->pd_tx_status_ = txIdle;
	port->pd_tx_flag_ = FALSE;
	port->policy_msg_tx_sop_ = SOP_TYPE_SOP;
	port->protocol_msg_rx_ = FALSE;
	port->protocol_msg_rx_sop_ = SOP_TYPE_SOP;
	port->protocol_msg_tx_sop_ = SOP_TYPE_SOP;
	port->protocol_tx_bytes_ = 0;
	port->protocol_check_rx_b4_tx_ = FALSE;
	port->protocol_use_sinktx_ = FALSE;
	port->policy_state_ = peDisabled;
	port->last_policy_state_ = peDisabled;
	port->policy_subindex_ = 0;
	port->policy_is_source_ = TRUE;
	port->policy_is_dfp_ = TRUE;
	port->is_contract_valid_ = FALSE;
	port->is_vconn_source_ = FALSE;
	port->collision_counter_ = 0;
	port->hard_reset_counter_ = 0;
	port->caps_counter_ = 0;
	port->policy_has_contract_ = FALSE;
	port->renegotiate_ = FALSE;
	port->sink_request_max_voltage_ = 180;  /* 9V (50mV resolution) */
	port->sink_request_max_power_ = 36000;
	port->sink_request_op_power_ = 36000;
	port->sink_partner_max_power_ = 0;
	port->sink_request_low_power_ = FALSE;
	port->sink_goto_min_compatible_ = FALSE;
	port->sink_usb_suspend_compatible_ = FALSE;
	port->sink_usb_comm_capable_ = TRUE;
	port->partner_caps_.object = 0;
	port->pd_HV_option_ = FSC_VBUS_12_V;
	port->ams = FALSE;
	port->flag = 2;
#if defined(FSC_DEBUG) || defined(FSC_HAVE_USBHID)
	port->source_caps_updated_ = FALSE;
#endif /* defined(FSCDEBUG) || defined(FSC_HAVE_USBHID) */

	FUSB3601_TimerDisable(&port->tc_state_timer_);
	FUSB3601_TimerDisable(&port->policy_state_timer_);
	FUSB3601_TimerDisable(&port->cc_debounce_timer_);
	FUSB3601_TimerDisable(&port->pd_debounce_timer_);

	/*
	 * Initialize SOP-related arrays.
	 * NOTE: Update this loop condition if supporting additional SOP types!
	 */
	for (i = 0; i <= SOP_TYPE_SOP1; ++i) {
		port->message_id_counter_[i] = 0;
		port->message_id_[i] = 0xFF;
	}

	for (i = 0; i < 7; ++i) {
		port->policy_rx_data_obj_[i].object = 0;
		port->policy_tx_data_obj_[i].object = 0;
		port->pd_transmit_objects_[i].object = 0;
		port->caps_sink_[i].object = 0;
		port->caps_source_[i].object = 0;
		port->caps_received_[i].object = 0;
	}

	/* Pick a port type.  These get sent to the device in SetStateUnattached. */
#if defined(FSC_HAVE_DRP)
	if (strstr(saved_command_line, "androidboot.mode=charger")) {
		port->port_type_ = USBTypeC_Sink;
	} else {
		port->port_type_ = USBTypeC_DRP;
	}
#elif defined(FSC_HAVE_SRC)
	port->port_type_ = USBTypeC_Source;
#elif defined(FSC_HAVE_SNK)
	port->port_type_ = USBTypeC_Sink;
#endif /* FSC_HAVE_DRP/SRC/SNK */
	/* Set up the capabilities objects */
#ifdef FSC_HAVE_SNK
	port->caps_header_sink_.NumDataObjects = 2;
	port->caps_header_sink_.PortDataRole = 0;               /* UFP */
	port->caps_header_sink_.PortPowerRole = 0;              /* Sink */
	port->caps_header_sink_.SpecRevision = 1;               /* Spec revision - 2.0 */

	port->caps_sink_[0].FPDOSink.Voltage = 100;             /* 5V - 1st supply option */
	port->caps_sink_[0].FPDOSink.OperationalCurrent = 200;   /* 2A for this object */
	port->caps_sink_[0].FPDOSink.DataRoleSwap = 1;          /* Enable DR_SWAP */
	port->caps_sink_[0].FPDOSink.USBCommCapable = 1;
	port->caps_sink_[0].FPDOSink.ExternallyPowered = 0;     /* Externally powered */
	port->caps_sink_[0].FPDOSink.HigherCapability = FALSE;  /* Don't req more than vSafe5V */
	port->caps_sink_[0].FPDOSink.DualRolePower = 1;         /* Enable PR_SWAP */

	port->caps_sink_[1].FPDOSink.Voltage = 180;             /* 9V - 2nd supply option */
	port->caps_sink_[1].FPDOSink.OperationalCurrent = 200;   /* 2A for this object */
	port->caps_sink_[1].FPDOSink.DataRoleSwap = 0;          /* Not used */
	port->caps_sink_[1].FPDOSink.USBCommCapable = 0;        /* Not used */
	port->caps_sink_[1].FPDOSink.ExternallyPowered = 0;     /* Not used */
	port->caps_sink_[1].FPDOSink.HigherCapability = 0;      /* Not used */
	port->caps_sink_[1].FPDOSink.DualRolePower = 0;         /* Not used */

#endif /* FSC_HAVE_SNK */

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
	port->caps_header_source_.NumDataObjects = 1;
	port->caps_header_source_.PortDataRole = 0;                 /* UFP */
	port->caps_header_source_.PortPowerRole = 1;                /* Source */
	port->caps_header_source_.SpecRevision = 1;                 /* Spec revision - 2.0 */

	port->caps_source_[0].FPDOSupply.Voltage = 100;             /* 5V */
	port->caps_source_[0].FPDOSupply.MaxCurrent = get_source_max_output_current();
	port->caps_source_[0].FPDOSupply.PeakCurrent = 0;           /* Set peak equal to max */
	port->caps_source_[0].FPDOSupply.DataRoleSwap = TRUE;       /* Enable DR_SWAP */
	port->caps_source_[0].FPDOSupply.USBCommCapable = TRUE;
	port->caps_source_[0].FPDOSupply.ExternallyPowered = FALSE;  /* Externally powered */
	port->caps_source_[0].FPDOSupply.USBSuspendSupport = FALSE; /* No USB Suspend */
	port->caps_source_[0].FPDOSupply.DualRolePower = TRUE;      /* Enable PR_SWAP */
	port->caps_source_[0].FPDOSupply.SupplyType = 0;            /* Fixed supply */
#endif /* FSC_HAVE_SRC */

#ifdef FSC_HAVE_VDM
	FUSB3601_TimerDisable(&port->vdm_timer_);

	port->vdm_next_ps_ = peDisabled;
	port->original_policy_state_ = peDisabled;
	port->expecting_vdm_response_ = FALSE;
	port->sending_vdm_data_ = FALSE;
	port->auto_vdm_state_ = AUTO_VDM_INIT;
	port->vdm_msg_length_ = 0;
	port->vdm_msg_tx_sop_ = SOP_TYPE_SOP;
	port->svid_enable_ = TRUE;
	port->mode_enable_ = TRUE;
	port->my_svid_ = DP_SID;
	port->my_mode_ = 0x00003C46;//0x00000C46;
	port->mode_entered_ = FALSE;
	port->auto_mode_disc_tracker_ = 0;

#ifdef FSC_HAVE_DP
	port->display_port_data_.AutoModeEntryObjPos = 0;
	port->display_port_data_.DpAutoModeEntryEnabled = TRUE;
	port->display_port_data_.DpEnabled = TRUE;
	port->display_port_data_.DpModeEntryMask.word = 0xFFFFFFFF;
	port->display_port_data_.DpModeEntryMask.UfpDCapable = 0;
	port->display_port_data_.DpModeEntryMask.DfpDCapable = 0;

	port->display_port_data_.DpModeEntryValue.word = 0;
	port->display_port_data_.DpModeEntryValue.UfpDCapable = 1;

	port->display_port_data_.DpCaps.word = 0;
	port->display_port_data_.DpConfig.word = 0;

	port->display_port_data_.DpStatus.word = 0;
	port->display_port_data_.DpStatus.Connection = DP_CONN_DFP_D;

	port->display_port_data_.DpPpConfig.word = 0;
	port->display_port_data_.DpPpRequestedConfig.word = 0;
	port->display_port_data_.DpPpStatus.word = 0;
	port->display_port_data_.DpModeEntered = 0;
#endif /* FSC_HAVE_DP */
#endif /* FSC_HAVE_VDM */

	port->pd_notified_ = FALSE;
	port->vbus_enabled_ = FALSE;

	port->factory_mode_ = FALSE;
	port->double56k = FALSE;
	port->c2a_cable_ = FALSE;
}

void FUSB3601_InitializePort(struct Port *port)
{
	/* Read all of the register values to update our cache */
	FUSB3601_ReadAllRegisters(port);
	FUSB3601_dump_register();
	port->registers_.MsgHeadr.USBPD_REV = USBPDSPECREV;
	FUSB3601_WriteRegister(port, regMSGHEADR);
	/* Clear reset flag */
	FUSB3601_ClearInterrupt(port, regFAULTSTAT, MSK_ALL_REGS_RESET);
	port->registers_.SCPInt1Msk.byte = 0;
	FUSB3601_WriteRegister(port, regSCP_INT1_MSK);
	port->registers_.SCPInt2Msk.byte = 0;
	FUSB3601_WriteRegister(port, regSCP_INT2_MSK);
	FUSB3601_ClearInterrupt(port, regEVENT_1, MSK_SCP_EVENT1_ALL);
	FUSB3601_ClearInterrupt(port, regEVENT_2, MSK_SCP_EVENT2_ALL);
	FUSB3601_ClearInterrupt(port, regEVENT_3, MSK_SCP_EVENT3_ALL);
	/* Setup the WatchDog */

	/* Initially mask all interrupts - unmask/remask as needed */
	port->registers_.AlertMskL.byte = 0;
	FUSB3601_WriteRegister(port, regALERTMSKL);
	port->registers_.AlertMskH.byte = 0;
	port->registers_.AlertMskH.M_FAULT = 1;
	FUSB3601_WriteRegister(port, regALERTMSKH);
	port->registers_.PwrStatMsk.byte = 0;
	FUSB3601_WriteRegister(port, regPWRSTATMSK);

	/* Disable force-to-sink */
	port->registers_.MUSControl1.SNK_ENABLE = 0;
	FUSB3601_WriteRegister(port, regMUS_CONTROL_1);
	/* Disable SCP timers and clear global INT Mask */
	port->registers_.SCPEnable1.EN_ACTIMER = 0;
	port->registers_.SCPEnable1.EN_ATIMER = 0;
	port->registers_.SCPEnable1.EN_STIMER = 0;
	port->registers_.SCPEnable1.INT_MASK = 0;
	FUSB3601_WriteRegister(port, regSCP_ENABLE1);

	port->registers_.MUSIntMask.M_SCP_EVENT = 0;
	FUSB3601_WriteRegister(port, regMUS_INTERRUPT_MSK);

	port->registers_.MUSControl2.OVP_LEVEL = 0b001; /*set ovp level to 9v*/
	FUSB3601_WriteRegister(port, regMUS_CONTROL_2);
	port->initialized_ = TRUE;
}

void FUSB3601_FeedTheDog(struct Port *port)
{
	port->registers_.WDReset = 0xFD; /* "Magic" watchdog reset value */
	FUSB3601_WriteRegister(port, regWD_RESET);
	port->registers_.WDReset = 0x00;
	/* TODO - Need this for Linux platform */
}

/* Register Update Function - Single register read */
FSC_BOOL FUSB3601_ReadRegister(struct Port *port, enum RegAddress regaddress)
{
	return FUSB3601_platform_i2c_read(port->port_id_, (FSC_U8)regaddress, 1, FUSB3601_AddressToRegister(&port->registers_, regaddress));
}

/* Register Update Function - Multi-byte read to reduce overhead */
FSC_BOOL FUSB3601_ReadRegisters(struct Port *port, enum RegAddress regaddr, FSC_U8 cnt)
{
	return FUSB3601_platform_i2c_read(port->i2c_addr_, (FSC_U8)regaddr, cnt, FUSB3601_AddressToRegister(&port->registers_, regaddr));
}

/*
 * ReadStatusRegisters
 *
 * Updates register map with the device's interrupt and status register data.
 */
void FUSB3601_ReadStatusRegisters(struct Port *port)
{
	FUSB3601_ReadRegisters(port, regCCSTAT, 3);
	FUSB3601_ReadRegisters(port, regALERTL, 2);
	FUSB3601_ReadRegisters(port, regEVENT_1, 3);
	FUSB3601_ReadRegister(port, regMUS_INTERRUPT);
	FUSB3601_ReadRegister(port, regDATA_OVP_INT);
}

void FUSB3601_ReadAllRegisters(struct Port *port)
{
	FUSB3601_ReadRegister(port, regVENDIDL);
	FUSB3601_ReadRegister(port, regVENDIDH);
	FUSB3601_ReadRegister(port, regPRODIDL);
	FUSB3601_ReadRegister(port, regPRODIDH);
	FUSB3601_ReadRegister(port, regDEVIDL);
	FUSB3601_ReadRegister(port, regDEVIDH);
	FUSB3601_ReadRegister(port, regTYPECREVL);
	FUSB3601_ReadRegister(port, regTYPECREVH);
	FUSB3601_ReadRegister(port, regUSBPDVER);
	FUSB3601_ReadRegister(port, regUSBPDREV);
	FUSB3601_ReadRegister(port, regPDIFREVL);
	FUSB3601_ReadRegister(port, regPDIFREVH);
	FUSB3601_ReadRegister(port, regALERTL);
	FUSB3601_ReadRegister(port, regALERTH);
	FUSB3601_ReadRegister(port, regALERTMSKL);
	FUSB3601_ReadRegister(port, regALERTMSKH);
	FUSB3601_ReadRegister(port, regPWRSTATMSK);
	FUSB3601_ReadRegister(port, regFAULTSTATMSK);
	FUSB3601_ReadRegister(port, regSTD_OUT_CFG);
	FUSB3601_ReadRegister(port, regTCPC_CTRL);
	FUSB3601_ReadRegister(port, regROLECTRL);
	FUSB3601_ReadRegister(port, regFAULTCTRL);
	FUSB3601_ReadRegister(port, regPWRCTRL);
	FUSB3601_ReadRegister(port, regCCSTAT);
	FUSB3601_ReadRegister(port, regPWRSTAT);
	FUSB3601_ReadRegister(port, regFAULTSTAT);
	FUSB3601_ReadRegister(port, regCOMMAND);
	FUSB3601_ReadRegister(port, regDEVCAP1L);
	FUSB3601_ReadRegister(port, regDEVCAP1H);
	FUSB3601_ReadRegister(port, regDEVCAP2L);
	FUSB3601_ReadRegister(port, regSTD_OUT_CAP);
	FUSB3601_ReadRegister(port, regMSGHEADR);
	FUSB3601_ReadRegister(port, regRXDETECT);
	FUSB3601_ReadRegister(port, regRXBYTECNT);
	FUSB3601_ReadRegister(port, regRXSTAT);
	FUSB3601_ReadRegister(port, regTRANSMIT);
	FUSB3601_ReadRegister(port, regTXBYTECNT);
	FUSB3601_ReadRegister(port, regVBUS_VOLTAGE_L);
	FUSB3601_ReadRegister(port, regVBUS_VOLTAGE_H);
	FUSB3601_ReadRegister(port, regVBUS_SNK_DISCL);
	FUSB3601_ReadRegister(port, regVBUS_SNK_DISCH);
	FUSB3601_ReadRegister(port, regVBUS_STOP_DISCL);
	FUSB3601_ReadRegister(port, regVBUS_STOP_DISCH);
	FUSB3601_ReadRegister(port, regVALARMHCFGL);
	FUSB3601_ReadRegister(port, regVALARMHCFGH);
	FUSB3601_ReadRegister(port, regVALARMLCFGL);
	FUSB3601_ReadRegister(port, regVALARMLCFGH);
	FUSB3601_ReadRegister(port, regSCP_ENABLE1);
	FUSB3601_ReadRegister(port, regSCP_ENABLE2);
	FUSB3601_ReadRegister(port, regSCP_INT1_MSK);
	FUSB3601_ReadRegister(port, regSCP_INT2_MSK);
	FUSB3601_ReadRegister(port, regTIMER_SET1);
	FUSB3601_ReadRegister(port, regTIMER_SET2);
	FUSB3601_ReadRegister(port, regEVENT_1);
	FUSB3601_ReadRegister(port, regEVENT_2);
	FUSB3601_ReadRegister(port, regEVENT_3);
	FUSB3601_ReadRegister(port, regDEFAULT_CMD);
	FUSB3601_ReadRegister(port, regDEFAULT_ADDR);
	FUSB3601_ReadRegister(port, regAUTO_CMD);
	FUSB3601_ReadRegister(port, regAUTO_ADDR);
	FUSB3601_ReadRegister(port, regAUTO_DATA0);
	FUSB3601_ReadRegister(port, regAUTO_DATA1);
	FUSB3601_ReadRegister(port, regAUTO_DATA2);
	FUSB3601_ReadRegister(port, regAUTO_DATA3);
	FUSB3601_ReadRegister(port, regAUTO_DATA4);
	FUSB3601_ReadRegister(port, regRT_CMD);
	FUSB3601_ReadRegister(port, regRT_ADDR);
	FUSB3601_ReadRegister(port, regMUS_CONTROL_1);
	FUSB3601_ReadRegister(port, regMUS_CONTROL_2);
	FUSB3601_ReadRegister(port, regMUS_INTERRUPT);
	FUSB3601_ReadRegister(port, regMUS_INTERRUPT_MSK);
	FUSB3601_ReadRegister(port, regMUS_TIMING);
	FUSB3601_ReadRegister(port, regWD_RESET);
	FUSB3601_ReadRegister(port, regWD_TIMING);
	FUSB3601_ReadRegister(port, regJIG_TIMING);
	FUSB3601_ReadRegister(port, regWD_STATUS);
	FUSB3601_ReadRegister(port, regWD_HISTORY_RESET);
	FUSB3601_ReadRegister(port, regFM_CONTROL1);
	FUSB3601_ReadRegister(port, regFM_CONTROL2);
	FUSB3601_ReadRegister(port, regFM_CONTROL3);
	FUSB3601_ReadRegister(port, regFM_CONTROL4);
	FUSB3601_ReadRegister(port, regFM_STATUS);
	FUSB3601_ReadRegister(port, regVCONN_OCP);
	FUSB3601_ReadRegister(port, regPD_RESET);
	FUSB3601_ReadRegister(port, regDRP_TOGGLE);
	FUSB3601_ReadRegister(port, regSINK_TRANSMIT);
	FUSB3601_ReadRegister(port, regSRC_FRSWAP);
	FUSB3601_ReadRegister(port, regSNK_FRSWAP);
	FUSB3601_ReadRegister(port, regDATA_OVP_CONTROL);
	FUSB3601_ReadRegister(port, regDATA_OVP_INT);
	FUSB3601_ReadRegister(port, regDATA_OVP_INT_MSK);
}

void FUSB3601_ReadRxRegisters(struct Port *port, FSC_U8 numbytes)
{
	/* Check length limit */
	if (numbytes > COMM_BUFFER_LENGTH)
		numbytes = COMM_BUFFER_LENGTH;
	FUSB3601_platform_i2c_read(port->port_id_, regRXDATA_00, numbytes, port->registers_.RxData);
}

void FUSB3601_WriteRegister(struct Port *port, enum RegAddress regaddress)
{
	FUSB3601_platform_i2c_write(port->port_id_, (FSC_U8)regaddress, 1, FUSB3601_AddressToRegister(&port->registers_, regaddress));
}

void FUSB3601_WriteRegisters(struct Port *port, enum RegAddress regaddress, FSC_U8 cnt)
{
	FUSB3601_platform_i2c_write(port->port_id_, (FSC_U8)regaddress, cnt, FUSB3601_AddressToRegister(&port->registers_, regaddress));
}

void FUSB3601_WriteTxRegisters(struct Port *port, FSC_U8 numbytes)
{
	/* Check length limit */
	if (numbytes > COMM_BUFFER_LENGTH)
		numbytes = COMM_BUFFER_LENGTH;
	FUSB3601_platform_i2c_write(port->port_id_, regTXDATA_00, numbytes, port->registers_.TxData);
}

/*
 * Sets bits indicated by mask in interrupt register at address. This has the
 * effect of clearing the specified interrupt(s).
 */
void FUSB3601_ClearInterrupt(struct Port *port, enum RegAddress address, FSC_U8 mask)
{
	FSC_U8 data = mask;
	FUSB3601_platform_i2c_write(port->port_id_, (FSC_U8)address, 1, &data);
	FUSB3601_RegClearBits(&(port->registers_), address, mask);
}

/*
 * SendCommand
 *
 * Sets the port's command register to cmd and writes it to the device.
 */
void FUSB3601_SendCommand(struct Port *port, enum DeviceCommand cmd)
{
	port->registers_.Command = cmd;
	FUSB3601_WriteRegister(port, regCOMMAND);
}

void FUSB3601_SetRpValue(struct Port *port, USBTypeCCurrent currentVal)
{
	switch (currentVal) {
	case utccDefault: /* Rp Default */
		port->registers_.RoleCtrl.RP_VAL = 0b00;
		break;
	case utcc1p5A:  /* Rp 1.5A */
		port->registers_.RoleCtrl.RP_VAL = 0b01;
		break;
	case utcc3p0A:  /* Rp 3.0A*/
		port->registers_.RoleCtrl.RP_VAL = 0b10;
		break;
	default:        /* Go to default */
		port->registers_.RoleCtrl.RP_VAL = 0b00;
		break;
	}

	FUSB3601_WriteRegister(port, regROLECTRL);
}

/* Type-C Functionality */
void FUSB3601_UpdateSourceCurrent(struct Port *port, USBTypeCCurrent currentVal)
{
	FUSB3601_SetRpValue(port, currentVal);
	FUSB3601_WriteRegister(port, regROLECTRL);
}

void FUSB3601_UpdateSinkCurrent(struct Port *port)
{
	port->snk_current_ = port->cc_term_;
	FUSB3601_platform_notify_sink_current(port);
}

void FUSB3601_SetVBusSource5V(struct Port *port, FSC_BOOL enable)
{
	/* FMControl1 register can now only be written during a valid attach.
	 * This means VBUS can't be turned off after a src detach.  Until this
	 * is fixed, we will use the test_mode register 0xF0 bit 0 to enable
	 * writes to the FMControl1 register as needed.
	 */
#ifdef PLATFORM_ARM
	if (enable) {
		port->registers_.FMControl1.FAST_ROLE_SWAP = 0b10;
		port->registers_.FMControl1.VBUS_5V = 0b10;
	} else {
		port->registers_.FMControl1.FAST_ROLE_SWAP = 0b01;
		port->registers_.FMControl1.VBUS_5V = 0b01;
	}
	FUSB3601_WriteRegister(port, regFM_CONTROL1);
#endif /* PLATFORM_ARM */
#ifdef FSC_PLATFORM_LINUX
	if (enable) {
		port->registers_.FMControl3.VOUT_SWITCH = 0b10;
		FUSB3601_WriteRegister(port, regFM_CONTROL3);
	}
	FUSB3601_platform_set_vbus_output(enable);
#endif /* FSC_PLATFORM_LINUX */
}

/* Returns the VBus voltage, multiplied by VBUS_SCALE */
//TODO: Consolidate - replace with register status?
FSC_U16 FUSB3601_GetVBusVoltage(struct Port *port)
{
	/* Max scaled voltage is 0xFFC, min is 0 */
	FSC_U16 voltage = 0;

	/* Read the current register values */
	FUSB3601_ReadRegister(port, regVBUS_VOLTAGE_L);
	FUSB3601_ReadRegister(port, regVBUS_VOLTAGE_H);

	/* Combine value bytes */
	voltage = ((FSC_U16)port->registers_.VBusVoltageH.VBUS_V_HI) << 8;
	voltage |= port->registers_.VBusVoltageL.VBUS_V_LO;

	/* Scale value as needed */
	switch (port->registers_.VBusVoltageH.VBUS_SCALE) {
	case 0b01: /* Scaled by 2 */
		voltage *= 2;
		break;
	case 0b10: /* Scaled by 4 */
		voltage *= 4;
		break;
	case 0: /* No scaling, fall through */
		default:
		break;
	}

	/* Voltage measurement LSB = 0.025V (25mV), scale to integer */
	return voltage;
}


FSC_BOOL FUSB3601_IsVbusVSafe0V(struct Port *port)
{
	/* Returns true when voltage is < 0.8V */
	FSC_U8 voltage = FUSB3601_GetVBusVoltage(port);
	return (voltage < FSC_VSAFE0V) ? TRUE : FALSE;
}

FSC_BOOL FUSB3601_IsVbusVSafe5V(struct Port *port)
{
	/* Returns true when voltage is within 4.75V - 5.5V */
	FSC_U8 measurement = FUSB3601_GetVBusVoltage(port);
	return ((measurement > FSC_VSAFE5V_L) && (measurement < FSC_VSAFE5V_H)) ? TRUE : FALSE;
}

FSC_BOOL FUSB3601_IsVbusOverVoltage(struct Port *port, FSC_U8 voltage)
{
	/* Returns true when voltage is > the voltage argument */
	FSC_U8 measurement = FUSB3601_GetVBusVoltage(port);
	return (measurement > voltage) ? TRUE : FALSE;
}

void FUSB3601_SetVBusSnkDisc(struct Port *port, FSC_U16 level)
{
	port->registers_.VBusSnkDiscL.byte = level & 0x00FF;
	port->registers_.VBusSnkDiscH.byte = (level & 0x0300) >> 8;
	FUSB3601_WriteRegisters(port, regVBUS_SNK_DISCL, 2);
}

void FUSB3601_SetVBusStopDisc(struct Port *port, FSC_U16 level)
{
	port->registers_.VBusStopDiscL.byte = level & 0x00FF;
	port->registers_.VBusStopDiscH.byte = (level & 0x0300) >> 8;
	FUSB3601_WriteRegisters(port, regVBUS_STOP_DISCL, 2);
}

//TODO: Split function into high and low
void FUSB3601_SetVBusAlarm(struct Port *port, FSC_U16 levelL, FSC_U16 levelH)
{
	port->registers_.VAlarmLCfgL.byte = levelL & 0x00FF;
	port->registers_.VAlarmLCfgH.byte = (levelL & 0x0300) >> 8;
	FUSB3601_WriteRegisters(port, regVALARMLCFGL, 2);

	port->registers_.VAlarmHCfgL.byte = levelH & 0x00FF;
	port->registers_.VAlarmHCfgH.byte = (levelH & 0x0300) >> 8;
	FUSB3601_WriteRegisters(port, regVALARMHCFGL, 2);
}

#if defined(FSC_HAVE_SRC) || (defined(FSC_HAVE_SNK) && defined(FSC_HAVE_ACC))
/*
 * SetStateSource
 *
 * Set up common items for a source connection.
 *
 * If vconn == TRUE, then this also enables VConn.
 *
 * This function does not enable or disable VBus - this is up to the caller.
 */
void FUSB3601_SetStateSource(struct Port *port, FSC_BOOL vconn)
{
	port->source_or_sink_ = Source;
	port->snk_current_ = utccOpen;
	port->tc_substate_ = 0;

	/* Enable VConn if so desired */
	if ((vconn == TRUE) && (port->vconn_term_ == SRC_RA)) {
#ifdef FSC_PLATFORM_LINUX
		FUSB3601_platform_set_vconn(TRUE);
#endif /* FSC_PLATFORM_LINUX */
		port->registers_.PwrCtrl.EN_VCONN = 1;
	} else {
#ifdef FSC_PLATFORM_LINUX
		FUSB3601_platform_set_vconn(FALSE);
#endif /* FSC_PLATFORM_LINUX */
		port->registers_.PwrCtrl.EN_VCONN = 0;
	}

	/* Enable the device's vbus monitor and alarm */
	port->registers_.PwrCtrl.DIS_VBUS_MON = 0; /* TODO: Setting to 0 causes issue in RevB */
	port->registers_.PwrCtrl.DIS_VALARM = 0;
	FUSB3601_WriteRegister(port, regPWRCTRL);

#ifdef FSC_LOGGING
	FUSB3601_LogTCState(port);
#endif /* FSC_LOGGING */
}
#endif /* FSC_HAVE_SRC || (FSC_HAVE_SNK && FSC_HAVE_ACC) */

#ifdef FSC_HAVE_SNK
/*
 * SetStateSink
 *
 * Set up common items for a source connection.
 */
void FUSB3601_SetStateSink(struct Port *port)
{
	port->source_or_sink_ = Sink;

#ifdef FSC_LOGGING
	FUSB3601_LogTCState(port);
#endif /* FSC_LOGGING */
}
#endif /* FSC_HAVE_SNK */

/*
 * DetectCCPin
 *
 * Called to discover which pin is CC.
 * This sets port->cc_term_
 * according to the termination found on the CC line.
 */
void FUSB3601_DetectCCPin(struct Port *port)
{
	/* Decode CC1 termination */
	FSC_U8 cc1;
	FSC_U8 cc2;

	FUSB3601_ReadRegister(port, regCCSTAT);
	cc1 = port->registers_.CCStat.CC1_STAT;
	cc2 = port->registers_.CCStat.CC2_STAT;
	if(cc1 > cc2) {
		port->cc_term_ = cc1;
		port->vconn_term_ = cc2;
	} else {
		port->cc_term_ = cc2;
		port->vconn_term_ = cc1;
	}
}

void FUSB3601_DetectCCPinOriented(struct Port *port)
{
	/* Decode CC1 termination */
	FSC_U8 cc1;
	FSC_U8 cc2;

	FUSB3601_ReadRegister(port, regCCSTAT);
	cc1 = port->registers_.CCStat.CC1_STAT;
	cc2 = port->registers_.CCStat.CC2_STAT;
	if(port->registers_.TcpcCtrl.ORIENT == 0) {
		port->cc_term_ = cc1;
	} else {
		port->cc_term_ = cc2;
	}
}

void FUSB3601_SetOrientation(struct Port *port)
{
	  /* Decode CC1 termination */
	FSC_U8 cc1 = port->registers_.CCStat.CC1_STAT;
	FSC_U8 cc2 = port->registers_.CCStat.CC2_STAT;

	if(cc1 > cc2) {
		port->registers_.TcpcCtrl.ORIENT = 0;
	} else {
		port->registers_.TcpcCtrl.ORIENT = 1;
	}
	FUSB3601_WriteRegister(port, regTCPC_CTRL);
}

void FUSB3601_ClearState(struct Port *port)
{
	FUSB3601_PDDisable(port);
	FUSB3601_enable_vbus_adc(port);

	FUSB3601_set_sbu_switch(port, Sbu_None);
	FUSB3601_set_usb_switch(port, NONE);

	/* Disable VBus and VBus detection */
	if (port->source_or_sink_ == Source) {
		FUSB3601_SendCommand(port, DisableSourceVbus);
		FUSB3601_SetVBusSource5V(port, FALSE);
	} else {
		FUSB3601_SendCommand(port, DisableSinkVbus);
	}

	port->registers_.AlertMskH.M_VBUS_SNK_DISC = 0;
	FUSB3601_WriteRegisters(port, regALERTMSKH, 1);
	/* Set up the Sink Disconnect threshold/interrupt */
	FUSB3601_SetVBusSnkDisc(port, FSC_VSAFE5V_DISC);
	FUSB3601_SetVBusStopDisc(port, FSC_VSAFE0V_DISCH);
	FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_VBUS_SNK_DISC);

	/* Must set to PWRCTRL to 0x60 for Look4Con to work (RevC) */
	port->registers_.PwrCtrl.EN_VCONN = 0;
	port->registers_.PwrCtrl.DIS_VBUS_MON = 1; /* Must be 1 for look4Con to work */
	port->registers_.PwrCtrl.DIS_VALARM = 1; /* Must be 1 for look4Con to work */
	port->registers_.PwrCtrl.AUTO_DISCH = 0;
	port->registers_.PwrCtrl.EN_BLEED_DISCH = 0;
	port->registers_.PwrCtrl.FORCE_DISCH = 0;
	FUSB3601_WriteRegister(port, regPWRCTRL);
	port->registers_.TcpcCtrl.ORIENT = 0;
	FUSB3601_WriteRegister(port, regTCPC_CTRL);

	FUSB3601_TimerDisable(&port->pd_debounce_timer_);
	FUSB3601_TimerDisable(&port->cc_debounce_timer_);
	FUSB3601_TimerDisable(&port->tc_state_timer_);

	port->c2a_cable_ = FALSE;
	port->double56k = FALSE;

#ifdef FSC_LOGGING
	FUSB3601_LogTCState(port);
#endif /* FSC_LOGGING */
}

void FUSB3601_set_policy_msg_tx_sop(struct Port *port, SopType sop)
{
	if ((sop == SOP_TYPE_SOP) || (sop == SOP_TYPE_SOP1))
		port->policy_msg_tx_sop_ = sop;
}

SopType FUSB3601_DecodeSopFromPdMsg(FSC_U8 msg)
{
	switch (msg & 0b111) {
	case SOP_TYPE_SOP2: /* SOP'' */
		return SOP_TYPE_SOP2;
	case SOP_TYPE_SOP1: /* SOP' */
		return SOP_TYPE_SOP1;
	case SOP_TYPE_SOP:  /* SOP */
		default:
	return SOP_TYPE_SOP;
	}
}

void FUSB3601_set_msg_id_count(struct Port *port, SopType sop, FSC_U32 count)
{
	if ((sop == SOP_TYPE_SOP) || (sop == SOP_TYPE_SOP1))
		port->message_id_counter_[sop] = count;
}

void FUSB3601_set_message_id(struct Port *port, SopType sop, FSC_U32 id)
{
	if ((sop == SOP_TYPE_SOP) || (sop == SOP_TYPE_SOP1))
		port->message_id_[sop] = id;
}

void FUSB3601_PDDisable(struct Port *port)
{
#ifdef FSC_LOGGING
	/* If we were previously active, store the PD detach token */
	if (port->pd_active_ == TRUE)
		FUSB3601_WritePDToken(&port->log_, TRUE, pdtDetach);
#endif /* FSC_LOGGING */

#ifdef FSC_HAVE_USBHID
	/* Set the source caps updated flag to trigger an update of the GUI */
	port->source_caps_updated_ = TRUE;
#endif /* FSC_HAVE_USBHID */

	port->is_hard_reset_ = FALSE;
	port->pd_active_ = FALSE;
	port->protocol_state_ = PRLDisabled;
	FUSB3601_set_policy_state(port, peDisabled);
	port->pd_tx_status_ = txIdle;
	port->policy_is_source_ = FALSE;
	port->policy_has_contract_ = FALSE;
	port->is_contract_valid_ = FALSE;
	/* platform_notify_pd_contract(FALSE); */
	FUSB3601_TimerDisable(&port->policy_state_timer_);

	/* Disable PD in the device */
	port->registers_.RxDetect.byte = 0;
	FUSB3601_WriteRegister(port, regRXDETECT);
	port->registers_.AlertMskL.M_TX_DISC = 0;
	port->registers_.AlertMskL.M_TXFAIL = 0;
	port->registers_.AlertMskL.M_TXSUCC = 0;
	port->registers_.AlertMskL.M_RXSTAT = 0;
	port->registers_.AlertMskL.M_RXHRDRST = 0;
	FUSB3601_WriteRegister(port, regALERTMSKL);
	FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_RXHRDRST | MSK_I_TXSUCC | MSK_I_TXFAIL | MSK_I_TXDISC | MSK_I_RXSTAT);

	FUSB3601_platform_notify_pd_contract(FALSE, 0, 0, FALSE);
}

//TODO: Looks like is_source is superfluous
void FUSB3601_PDEnable(struct Port *port, FSC_BOOL is_source)
{
	port->is_hard_reset_ = FALSE;
	port->is_pr_swap_ = FALSE;
	port->hard_reset_counter_ = 0;

	if (port->pd_enabled_ == TRUE) {
		port->registers_.PDReset.PD_RST = 1;
		port->pd_active_ = TRUE;
		port->idle_ = FALSE;
		port->policy_is_source_ = is_source;
		port->policy_is_dfp_ = is_source;
		port->is_vconn_source_ = is_source;
		port->policy_subindex_ = 0;
		port->last_policy_state_ = peDisabled;
		FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_TXSUCC | MSK_I_TXDISC | MSK_I_TXFAIL | MSK_I_RXHRDRST | MSK_I_RXSTAT);

		/* Set the initial data port direction */
		if (port->policy_is_source_) {
			FUSB3601_set_policy_state(port, peSourceStartup);
			FUSB3601_TimerDisable(&port->policy_state_timer_);
			FUSB3601_PolicySourceStartupHelper(port);
		} else {
			/* Policy is sink */
			FUSB3601_set_policy_state(port, peSinkStartup);
      		}

		port->expecting_vdm_response_ = FALSE;

#ifdef FSC_LOGGING
		FUSB3601_WritePDToken(&port->log_, TRUE, pdtAttach);
#endif /* FSC_LOGGING */

		if(port->policy_state_ == peSourceStartup)
			FUSB3601_PolicySourceStartup(port);
		else
			FUSB3601_PolicySinkStartup(port);
	}
}

void FUSB3601_ResetProtocolLayer(struct Port *port)
{
	FSC_U8 i = 0;

	port->protocol_state_ = PRLIdle;
	port->pd_tx_status_ = txIdle;
#ifdef FSC_HAVE_VDM
	FUSB3601_TimerDisable(&port->vdm_timer_);
#endif /* FSC_HAVE_VDM */

	port->message_id_counter_[SOP_TYPE_SOP] = 0;
	port->message_id_[SOP_TYPE_SOP] = 0xFF;
	port->message_id_counter_[SOP_TYPE_SOP1] = 0;
	port->message_id_[SOP_TYPE_SOP1] = 0xFF;
	port->protocol_check_rx_b4_tx_ = FALSE;
	port->protocol_msg_rx_ = FALSE;
	port->protocol_msg_rx_sop_ = SOP_TYPE_SOP;
	port->protocol_msg_tx_sop_ = SOP_TYPE_SOP;
	port->pd_tx_flag_ = FALSE;
	port->protocol_tx_bytes_ = 0;
	port->policy_has_contract_ = FALSE;

#ifdef FSC_HAVE_USBHID
	/* Set the source caps updated flag to trigger an update of the GUI */
	port->source_caps_updated_ = TRUE;
#endif /* FSC_HAVE_USBHID */

	port->usb_pd_contract_.object = 0;
	port->caps_header_received_.word = 0;

	for (i = 0; i < 7; i++) {
		port->caps_received_[i].object = 0;
	}

	/* Enable PD messaging */
	port->registers_.RxDetect.EN_SOP = SOP_Capable;
	if (port->policy_is_source_) {
		port->registers_.RxDetect.EN_SOP1 = SOP_P_Capable;
		port->registers_.RxDetect.EN_SOP2 = SOP_PP_Capable;
		port->registers_.RxDetect.EN_SOP1_DBG = SOP_P_Debug_Capable;
		port->registers_.RxDetect.EN_SOP2_DBG = SOP_PP_Debug_Capable;
	} else {
		port->registers_.RxDetect.EN_SOP1 = 0;
		port->registers_.RxDetect.EN_SOP2 = 0;
		port->registers_.RxDetect.EN_SOP1_DBG = 0;
		port->registers_.RxDetect.EN_SOP2_DBG = 0;
	}
	port->registers_.RxDetect.EN_HRD_RST = 1;
	/* Commit the configuration to the device */
	FUSB3601_WriteRegister(port, regRXDETECT);
}

int FUSB3601_GetStateCC(struct Port *port)
{
	int val = 0;
	if (port->double56k) {
		val = 0x5;
		return val;
	} else {
		FUSB3601_ReadRegister(port, regCCSTAT);
		val = port->registers_.CCStat.byte;
		pr_info("FUSB  %s - CCStat: [0x%x], val = %d\n", __func__, port->registers_.CCStat.byte, val);
		return val;
	}
}

void FUSB3601_set_policy_state(struct Port *port, PolicyState_t state)
{
	port->policy_state_ = state;
	port->policy_subindex_ = 0;

#ifdef FSC_LOGGING
	if (port->last_policy_state_ != port->policy_state_) {
		FUSB3601_LogPEState(port);
		port->last_policy_state_ = port->policy_state_;
	}
#endif /* FSC_LOGGING */
	FUSB3601_platform_log(port->port_id_, "FUSB PE SS", (FSC_U32)(state));
}

void FUSB3601_f_set_policy_state(struct Port *port, void (*f)(struct Port *))
{
	port->policy_subindex_ = 0;
#ifdef FSC_LOGGING
	if (port->last_policy_state_ != port->policy_state_) {
		FUSB3601_LogPEState(port);
		port->last_policy_state_ = port->policy_state_;
	}
#endif /* FSC_LOGGING */
	f(port);
}

#ifdef FSC_HAVE_VDM
/* VDM-specific functionality */
void FUSB3601_set_vdm_msg_tx_sop(struct Port *port, SopType sop)
{
	if ((sop == SOP_TYPE_SOP) || (sop == SOP_TYPE_SOP1))
		port->vdm_msg_tx_sop_ = sop;
}
#endif /* FSC_HAVE_VDM */

void FUSB3601_LogTCState(struct Port *port)
{
#ifdef FSC_LOGGING
	FUSB3601_WriteTCState(&port->log_, FUSB3601_platform_timestamp(), port->tc_state_);
#endif /* FSC_LOGGING */
}

void FUSB3601_LogPEState(struct Port *port)
{
#ifdef FSC_LOGGING
	FUSB3601_WritePEState(&port->log_, FUSB3601_platform_timestamp(), port->policy_state_);
#endif /* FSC_LOGGING */
}

void FUSB3601_PolicySourceStartupHelper(struct Port *port){

	/* Unmask for a source */
	port->registers_.AlertMskL.M_TXSUCC = 1;
	port->registers_.AlertMskL.M_TX_DISC = 1;
	port->registers_.AlertMskL.M_TXFAIL = 1;
	port->registers_.AlertMskL.M_RXHRDRST = 1;
 	port->registers_.AlertMskL.M_RXSTAT = 1;
 	port->registers_.AlertMskL.M_CCSTAT = 1;
	FUSB3601_WriteRegister(port, regALERTMSKL);

	/* Clear the BIST TMODE bit, if needed. */
	if(port->registers_.TcpcCtrl.BIST_TMODE == 1) {
		port->registers_.TcpcCtrl.BIST_TMODE = 0;
		FUSB3601_WriteRegister(port, regTCPC_CTRL);
	}

	port->usb_pd_contract_.object = 0;
	port->sink_partner_max_power_ = 0;
	port->partner_caps_.object = 0;
	port->is_pr_swap_ = FALSE;
	port->policy_is_source_ = TRUE;
	port->registers_.MsgHeadr.USBPD_REV = USBPDSPECREV;
	port->registers_.MsgHeadr.POWER_ROLE = port->policy_is_source_;
	port->registers_.MsgHeadr.DATA_ROLE = port->policy_is_dfp_;
	FUSB3601_WriteRegister(port, regMSGHEADR);
	FUSB3601_ResetProtocolLayer(port);

	port->registers_.PwrCtrl.DIS_VBUS_MON = 0;
	FUSB3601_WriteRegister(port, regPWRCTRL);

	port->caps_counter_ = 0;
	port->collision_counter_ = 0;
	port->pd_tx_flag_ = FALSE;
}

/* CC1 - close, CC2 - close, NONE - open */
void FUSB3601_set_usb_switch(struct Port *port, CCOrientation orientation) {

	mutex_lock(&FMControl4_mutex);
	FUSB3601_ReadRegister(port, regFM_CONTROL4);
	switch(orientation) {
	case CC1:
		port->registers_.FMControl4.USB_SWITCH = 0b011; /* Close Top USB */
		break;
	case CC2:
		port->registers_.FMControl4.USB_SWITCH = 0b100; /* Close Bot USB */
		break;
	case NONE:
	default:
		port->registers_.FMControl4.USB_SWITCH = 0b000; /* Open USB */
		break;
	}
	FUSB3601_WriteRegister(port, regFM_CONTROL4);
	mutex_unlock(&FMControl4_mutex);
}
void FUSB3601_set_vbus_detach(struct Port *port, VbusDetach_t enable)
{
	mutex_lock(&FMControl4_mutex);
	FUSB3601_ReadRegister(port, regFM_CONTROL4);
	switch(enable) {
		case VBUS_DETACH_ENABLE:
			port->registers_.FMControl4.VBUS_DETATCH_DET = 1;
			break;
		case VBUS_DETACH_DISABLE:
			port->registers_.FMControl4.VBUS_DETATCH_DET = 0;
			break;
		default:
			break;
	}
	FUSB3601_WriteRegister(port, regFM_CONTROL4);
	mutex_unlock(&FMControl4_mutex);
}
void FUSB3601_set_sbu_switch(struct Port *port, SbuSwitch_t SbuSwitch)
{
	FUSB3601_ReadRegister(port, regFM_CONTROL3);

	switch(SbuSwitch){
  	case Sbu_None:
		port->registers_.FMControl3.SBU_SWITCH = 0b000;
		break;
	case Sbu_Close_Aux:
		port->registers_.FMControl3.SBU_SWITCH = 0b010;
		break;
	case Sbu_Cross_Close_Aux:
		port->registers_.FMControl3.SBU_SWITCH = 0b011;
		break;
	case Sbu_Close_TXD_RXD:
		port->registers_.FMControl3.SBU_SWITCH = 0b100;
		break;
	case Sbu_Cross_Close_TXD_RXD:
		port->registers_.FMControl3.SBU_SWITCH = 0b101;
		break;
	default:
		return;
	}
	FUSB3601_WriteRegister(port, regFM_CONTROL3);
}
/* VBUS Discharge Stage 1 */
/* Set VALARMs, Enable Bleed, and Disable VBUS */
void FUSB3601_start_vbus_discharge(struct Port *port)
{
	/* Set up alert to wait for vSafeDischarge */
	FUSB3601_SetVBusAlarm(port, FSC_VSAFE_DISCH, FSC_VSAFE_DISCH);

	port->registers_.AlertMskL.M_PORT_PWR = 0;
	port->registers_.AlertMskL.M_VBUS_ALRM_HI = 0;
	port->registers_.AlertMskH.M_VBUS_ALRM_LO = 1;
	FUSB3601_WriteRegisters(port, regALERTMSKL, 2);

	FUSB3601_ClearInterrupt(port, regALERTL, MSK_I_ALARM_LO_ALL);
	FUSB3601_ClearInterrupt(port, regALERTH, MSK_I_ALARM_HI_ALL);

	FUSB3601_SetVBusStopDisc(port, FSC_VSAFE0V_DISCH);
	port->registers_.PwrCtrl.EN_BLEED_DISCH = 1;
	port->registers_.PwrCtrl.DIS_VALARM = 0;
	FUSB3601_WriteRegister(port, regPWRCTRL);

	/* Disable VBUS */
	FUSB3601_SendCommand(port, DisableSourceVbus);
	FUSB3601_SetVBusSource5V(port, FALSE);
}

/* VBUS Discharge Stage 2 */
/* Disable Bleed, Enabled Force Discharge */
void FUSB3601_set_force_discharge(struct Port *port)
{
	port->registers_.FMControl3.VOUT_SWITCH = 0b00;
	FUSB3601_WriteRegister(port, regFM_CONTROL3);

	port->registers_.PwrCtrl.EN_BLEED_DISCH = 0;
	port->registers_.PwrCtrl.FORCE_DISCH = 1;
	FUSB3601_WriteRegister(port, regPWRCTRL);
}

void FUSB3601_disable_vbus_adc(struct Port *port)
{
	return;
}

void FUSB3601_enable_vbus_adc(struct Port *port)
{
	return;
}
void reset_adc(struct Port *port)
{
	FSC_U8 data;

	/* Expand Addr */
	data = 0x10;
	FUSB3601_platform_i2c_write(port->port_id_, 0xF0, 1, &data);
	data = 0x0C;
	FUSB3601_platform_i2c_write(port->port_id_, 0x0E, 1, &data);
	/* De-Expand Addr */
	data = 0x00;
	FUSB3601_platform_i2c_write(port->port_id_, 0xF0, 1, &data);
	data = 0x05;
	FUSB3601_platform_i2c_write(port->port_id_, 0xF3, 1, &data);
	data = 0x00;
	FUSB3601_platform_i2c_write(port->port_id_, 0xF3, 1, &data);
	/* Expand Addr */
	data = 0x10;
	FUSB3601_platform_i2c_write(port->port_id_, 0xF0, 1, &data);
	/* Set ADC_TCTR = 0 */
	data = 0x08;
	FUSB3601_platform_i2c_write(port->port_id_, 0x0E, 1, &data);
	/* De-Expand Addr */
	data = 0x00;
	FUSB3601_platform_i2c_write(port->port_id_, 0xF0, 1, &data);
}
