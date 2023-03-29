/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * TCPC Type-C Driver for Richtek
 *
 * Author: TH <tsunghan_tsai@richtek.com>
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#include <linux/delay.h>
#include <linux/cpu.h>
#include <linux/hisi/usb/hisi_hifi_usb.h>

#include <huawei_platform/usb/pd/richtek/tcpci.h>
#include <huawei_platform/usb/pd/richtek/tcpci_typec.h>
#include <huawei_platform/usb/pd/richtek/tcpci_timer.h>
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
#include <huawei_platform/usb/pd/richtek/pd_dpm_core.h>
#endif
#include <huawei_platform/usb/pd/richtek/tcpm.h>
#include <huawei_platform/power/huawei_charger.h>
#include "huawei_platform/dp_aux_switch/dp_aux_switch.h"
#include <huawei_platform/usb/hw_usb.h>

#ifdef CONFIG_POGO_PIN
#include <huawei_platform/usb/huawei_pogopin.h>
#endif
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
extern int support_smart_holder;
#endif
#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
#define CONFIG_TYPEC_CAP_TRY_STATE
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
#undef	CONFIG_TYPEC_CAP_TRY_STATE
#define CONFIG_TYPEC_CAP_TRY_STATE
#endif

#define RICHTEK_PD_COMPLIANCE_FAKE_AUDIO_ACC	/* For Rp3A */
#define RICHTEK_PD_COMPLIANCE_FAKE_TRY_SNK_RP	/* For Rp3A */
#define RICHTEK_PD_COMPLIANCE_FAKE_EMRAK_ONLY	/* For Rp3A */
#define RICHTEK_PD_COMPLIANCE_DIRECT_CHARGE	/* For Direct Charge */ 
enum TYPEC_WAIT_PS_STATE {
	TYPEC_WAIT_PS_DISABLE = 0,
	TYPEC_WAIT_PS_SNK_VSAFE5V,
	TYPEC_WAIT_PS_SRC_VSAFE0V,
	TYPEC_WAIT_PS_SRC_VSAFE5V,
};

enum TYPEC_ROLE_SWAP_STATE {
	TYPEC_ROLE_SWAP_NONE = 0,
	TYPEC_ROLE_SWAP_TO_SNK,
	TYPEC_ROLE_SWAP_TO_SRC,
};

int tcpm_typec_notify_direct_charge(struct tcpc_device *tcpc_dev, bool dc);
#if TYPEC_DBG_ENABLE
static const char *const typec_wait_ps_name[] = {
	"Disable",
	"SNK_VSafe5V",
	"SRC_VSafe0V",
	"SRC_VSafe5V",
};
#endif	/* TYPEC_DBG_ENABLE */

static inline void typec_wait_ps_change(struct tcpc_device *tcpc_dev,
					enum TYPEC_WAIT_PS_STATE state)
{
#if TYPEC_DBG_ENABLE
	uint8_t old_state = tcpc_dev->typec_wait_ps_change;
	uint8_t new_state = (uint8_t) state;

	if (new_state != old_state)
		TYPEC_DBG("wait_ps=%s\r\n", typec_wait_ps_name[new_state]);
#endif	/* TYPEC_DBG_ENABLE */

#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
	if (state == TYPEC_WAIT_PS_SRC_VSAFE0V)
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_TOUT);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */

	if (tcpc_dev->typec_wait_ps_change == TYPEC_WAIT_PS_SRC_VSAFE0V
		&& state != TYPEC_WAIT_PS_SRC_VSAFE0V) {
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_DELAY);

#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_TOUT);
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */
	}

	tcpc_dev->typec_wait_ps_change = (uint8_t) state;
}

/* #define TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE */
#define TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS

static inline int typec_enable_low_power_mode(
	struct tcpc_device *tcpc_dev, int pull);

#define typec_get_cc1()		\
	tcpc_dev->typec_remote_cc[0]
#define typec_get_cc2()		\
	tcpc_dev->typec_remote_cc[1]
#define typec_get_cc_res()	\
	(tcpc_dev->typec_polarity ? typec_get_cc2() : typec_get_cc1())

#define typec_check_cc1(cc)	\
	(typec_get_cc1() == cc)

#define typec_check_cc2(cc)	\
	(typec_get_cc2() == cc)

#define typec_check_cc(cc1, cc2)	\
	(typec_check_cc1(cc1) && typec_check_cc2(cc2))

#define typec_check_cc_both(res)	\
	(typec_check_cc(res, res))

#define typec_check_cc_any(res)		\
	(typec_check_cc1(res) || typec_check_cc2(res))
	
#define typec_is_drp_toggling() \
	(typec_get_cc1() == TYPEC_CC_DRP_TOGGLING)

#define typec_is_cc_open()	\
	typec_check_cc_both(TYPEC_CC_VOLT_OPEN)


#define typec_is_sink_with_emark()	\
	(typec_get_cc1() + typec_get_cc2() == \
	TYPEC_CC_VOLT_RA+TYPEC_CC_VOLT_RD)

#ifdef CONFIG_TYPEC_CAP_NORP_SRC
#define typec_is_cc_ra()	\
	typec_check_cc_both(TYPEC_CC_VOLT_RA)

#define typec_is_cc_no_res()	\
	(typec_is_drp_toggling() || typec_is_cc_open())
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

static inline int typec_enable_vconn(struct tcpc_device *tcpc_dev)
{
	if (typec_is_sink_with_emark())
		return tcpci_set_vconn(tcpc_dev, true);
	return 0;
}

/* TYPEC_GET_CC_STATUS */

/*
 * [BLOCK] TYPEC Connection State Definition
 */

enum TYPEC_CONNECTION_STATE {
	typec_disabled = 0,
	typec_errorrecovery,

	typec_unattached_snk,
	typec_unattached_src,

	typec_attachwait_snk,
	typec_attachwait_src,

	typec_attached_snk,
	typec_attached_src,

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	/* Require : Assert Rp
	 * Exit(-> Attached.SRC) : Detect Rd (tPDDebounce).
	 * Exit(-> TryWait.SNK) : Not detect Rd after tDRPTry
	 * */
	typec_try_src,

	/* Require : Assert Rd
	 * Exit(-> Attached.SNK) : Detect Rp (tCCDebounce) and Vbus present.
	 * Exit(-> Unattached.SNK) : Not detect Rp (tPDDebounce)
	 * */

	typec_trywait_snk,
	typec_trywait_snk_pe,
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK

	/* Require : Assert Rd
	 * Wait for tDRPTry and only then begin monitoring CC.
	 * Exit (-> Attached.SNK) : Detect Rp (tPDDebounce) and Vbus present.
	 * Exit (-> TryWait.SRC) : Not detect Rp for tPDDebounce.
	 */
	typec_try_snk,

	/*
	 * Require : Assert Rp
	 * Exit (-> Attached.SRC) : Detect Rd (tCCDebounce)
	 * Exit (-> Unattached.SNK) : Not detect Rd after tDRPTry
	 */

	typec_trywait_src,
	typec_trywait_src_pe,
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_audioaccessory,
	typec_debugaccessory,

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	typec_attached_dbgacc_snk,
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	typec_attached_custom_src,
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
	typec_attached_custom_src2,
#endif  /* CONFIG_TYPEC_CAP_CUSTOM_SRC2 */

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	typec_role_swap,
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	typec_unattachwait_pe,	/* Wait Policy Engine go to Idle */
};

static const char *const typec_state_name[] = {
	"Disabled",
	"ErrorRecovery",

	"Unattached.SNK",
	"Unattached.SRC",

	"AttachWait.SNK",
	"AttachWait.SRC",

	"Attached.SNK",
	"Attached.SRC",

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	"Try.SRC",
	"TryWait.SNK",
	"TryWait.SNK.PE",
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	"Try.SNK",
	"TryWait.SRC",
	"TryWait.SRC.PE",
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	"AudioAccessory",
	"DebugAccessory",

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	"DBGACC.SNK",
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	"Custom.SRC",
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
	"Custom.SRC2",
#endif  /* CONFIG_TYPEC_CAP_CUSTOM_SRC2 */

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	"RoleSwap",
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	"UnattachWait.PE",
};

static inline void typec_transfer_state(struct tcpc_device *tcpc_dev,
					enum TYPEC_CONNECTION_STATE state)
{
	TYPEC_INFO("** %s\r\n", typec_state_name[state]);
	tcpc_dev->typec_state = (uint8_t) state;
}

#define TYPEC_NEW_STATE(state)  \
	(typec_transfer_state(tcpc_dev, state))

/*
 * [BLOCK] TypeC Alert Attach Status Changed
 */

static const char *const typec_attach_name[] = {
	"NULL",
	"SINK",
	"SOURCE",
	"AUDIO",
	"DEBUG",

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	"DBGACC_SNK",
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	"CUSTOM_SRC",
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
	"CUSTOM_SRC2",
#endif  /* CONFIG_TYPEC_CAP_CUSTOM_SRC2 */
};

static int typec_alert_attach_state_change(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable)
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_NOT_LEGACY);
	else
		tcpc_restart_timer(tcpc_dev, TYPEC_RT_TIMER_NOT_LEGACY);
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (tcpc_dev->typec_attach_old == tcpc_dev->typec_attach_new) {
		TYPEC_DBG("Attached-> %s(repeat)\r\n",
			typec_attach_name[tcpc_dev->typec_attach_new]);
		return 0;
	}

	TYPEC_INFO("Attached-> %s\r\n",
		   typec_attach_name[tcpc_dev->typec_attach_new]);

	/*Report function */
	ret = tcpci_report_usb_port_changed(tcpc_dev);

	tcpc_dev->typec_attach_old = tcpc_dev->typec_attach_new;
	return ret;
}

/*
 * [BLOCK] NoRpSRC Entry
 */

#ifdef CONFIG_TYPEC_CAP_NORP_SRC
static void typec_notify_attached_vbus_only(void)
{
	struct pd_dpm_typec_state tc_state;

	TYPEC_DBG("%s + \n", __func__);
	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_ATTACHED_VBUS_ONLY;
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
}

static void typec_notify_unattached_vbus_only(void)
{
	struct pd_dpm_typec_state tc_state;

	TYPEC_DBG("%s + \n", __func__);
	memset(&tc_state, 0, sizeof(tc_state));
	tc_state.new_state = PD_DPM_TYPEC_UNATTACHED_VBUS_ONLY;
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, (void*)&tc_state);
}

static bool typec_try_enter_norp_src(struct tcpc_device *tcpc_dev)
{
	if (tcpci_check_vbus_valid(tcpc_dev)) {
		TYPEC_DBG("norp_src=1\r\n");
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_NORP_SRC);
		return true;
	} else if (!tcpci_check_vsafe0v(tcpc_dev, false) &&
						tcpc_dev->typec_during_direct_charge) {
		TYPEC_DBG("ps_change&direct_chg = 1\r\n");
		return true;
	}

	return false;
}

static bool typec_try_exit_norp_src(struct tcpc_device *tcpc_dev)
{
	if (!typec_is_cc_ra() && !typec_is_cc_no_res()) {
		TYPEC_DBG("norp_src=1\r\n");
		tcpc_disable_timer(tcpc_dev, TYPEC_TIMER_NORP_SRC);
		return true;
	}

	return false;
}

static bool typec_norp_src_detached_entry(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->no_rpsrc_state == 1) {
		tcpc_dev->no_rpsrc_state = 0;
		typec_notify_unattached_vbus_only();
		return true;
	} else {
		tcpc_disable_timer(tcpc_dev, TYPEC_TIMER_NORP_SRC);
	}

	return false;
}

static inline int typec_norp_src_attached_entry(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_A2C_C2C
	tcpc_dev->typec_a2c_cable = true;
#endif	/* CONFIG_TYPEC_CAP_A2C_C2C */

	if (tcpc_dev->no_rpsrc_state == 0) {
		tcpc_dev->no_rpsrc_state = 1;
		typec_notify_attached_vbus_only();
	}
	return 0;
}

static inline int typec_handle_norp_src_debounce_timeout(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_NORP_SRC
	if ((typec_is_cc_no_res() || typec_is_cc_ra()) && tcpci_check_vbus_valid(tcpc_dev))
		typec_norp_src_attached_entry(tcpc_dev);
#endif    /* CONFIG_TYPEC_CAP_NORP_SRC */

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

/*
 * [BLOCK] Unattached Entry
 */

static inline int typec_try_low_power_mode(struct tcpc_device *tcpc_dev)
{
	int ret = tcpci_set_low_power_mode(
		tcpc_dev, true, tcpc_dev->typec_lpm_pull);
	if (ret < 0)
		return ret;

	ret = tcpci_is_low_power_mode(tcpc_dev);
	if (ret < 0)
		return ret;

	if (ret == 1)
		return 0;

	if (tcpc_dev->typec_lpm_retry == 0) {
		TCPC_DBG("EnterLPM Failed\r\n");
		return 0;
	}

	tcpc_dev->typec_lpm_retry--;
	tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LOW_POWER_MODE);

	return 0;
}

static inline int typec_enter_low_power_mode(struct tcpc_device *tcpc_dev)
{
	tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LOW_POWER_MODE);
	return 0;
}

static inline int typec_enable_low_power_mode(
	struct tcpc_device *tcpc_dev, int pull)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable) {
		TYPEC_DBG("LPM_LCOnly\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (tcpc_dev->typec_cable_only) {
		TYPEC_DBG("LPM_RaOnly\r\n");

#ifdef CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG
		if (tcpc_dev->tcpc_flags & TCPC_FLAGS_LPM_WAKEUP_WATCHDOG)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_WAKEUP);
#endif	/* CONFIG_TYPEC_CAP_LPM_WAKEUP_WATCHDOG */

		return 0;
	}

	if (tcpc_dev->typec_lpm != true){
		tcpc_dev->typec_lpm = true;
		tcpc_dev->typec_lpm_retry = 5;
		tcpc_dev->typec_lpm_pull = pull;	
		ret = typec_enter_low_power_mode(tcpc_dev);
	}
	return ret;
}

static inline int typec_disable_low_power_mode(
	struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	if (tcpc_dev->typec_lpm != false){
		tcpc_dev->typec_lpm = false;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_LOW_POWER_MODE);
		ret = tcpci_set_low_power_mode(tcpc_dev, false, TYPEC_CC_DRP);
	}
	return ret;
}

static void typec_unattached_power_entry(struct tcpc_device *tcpc_dev)
{
	hw_usb_ldo_supply_disable(HW_USB_LDO_CTRL_TYPECPD);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (tcpc_dev->typec_power_ctrl) {
		tcpci_set_vconn(tcpc_dev, false);
		tcpci_disable_vbus_control(tcpc_dev);
		tcpci_report_power_control(tcpc_dev, false);
	}
}

static inline void typec_unattached_cc_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_during_direct_charge = false;
#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap) {
		TYPEC_NEW_STATE(typec_role_swap);
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	switch (tcpc_dev->typec_role) {
	case TYPEC_ROLE_SNK:
		TYPEC_NEW_STATE(typec_unattached_snk);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_RD);
		break;
	case TYPEC_ROLE_SRC:
		TYPEC_NEW_STATE(typec_unattached_src);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_RP);
		break;
	default:
		switch (tcpc_dev->typec_state) {
		case typec_attachwait_snk:
		case typec_audioaccessory:
			TYPEC_NEW_STATE(typec_unattached_src);
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_3_0);
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_DRP_SRC_TOGGLE);
			break;
		default:
			TYPEC_NEW_STATE(typec_unattached_snk);
			tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
			typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
			break;
		}
		break;
	}
}

static void typec_unattached_entry(struct tcpc_device *tcpc_dev)
{
	tcpci_mask_vsafe0v(tcpc_dev, 0);
	typec_unattached_cc_entry(tcpc_dev);
	typec_unattached_power_entry(tcpc_dev);
}

static void typec_unattach_wait_pe_idle_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->typec_attach_old) {
		TYPEC_NEW_STATE(typec_unattachwait_pe);
		return;
	}
#endif

	typec_unattached_entry(tcpc_dev);
}

/*
 * [BLOCK] Attached Entry
 */

static inline int typec_set_polarity(struct tcpc_device *tcpc_dev,
					bool polarity)
{
	tcpc_dev->typec_polarity = polarity;
	return tcpci_set_polarity(tcpc_dev, polarity);
}

static inline int typec_set_plug_orient(struct tcpc_device *tcpc_dev,
				uint8_t res, bool polarity)
{
	int rv = typec_set_polarity(tcpc_dev, polarity);

	if (rv)
		return rv;

	return tcpci_set_cc(tcpc_dev, res);
}

static void typec_source_attached_with_vbus_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SRC;
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);
}

static inline void typec_source_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_attached_src);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SRC_VSAFE5V);

	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap) {
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	typec_set_plug_orient(tcpc_dev,
		tcpc_dev->typec_local_rp_level,
		typec_check_cc2(TYPEC_CC_VOLT_RD));

	tcpci_report_power_control(tcpc_dev, true);
	typec_enable_vconn(tcpc_dev);
	tcpci_source_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, -1);
}

static inline void typec_sink_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_attached_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#if 0
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	tcpc_dev->typec_legacy_cable = false;
	tcpc_dev->typec_legacy_cable_suspect = 0;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */
#endif

	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_SNK;

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
	if (tcpc_dev->typec_role >= TYPEC_ROLE_DRP)
		tcpc_reset_typec_try_timer(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap) {
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_NONE;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
	}
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	typec_set_plug_orient(tcpc_dev, TYPEC_CC_RD,
		!typec_check_cc2(TYPEC_CC_VOLT_OPEN));
	tcpc_dev->typec_remote_rp_level = typec_get_cc_res();

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
static inline void cust2_src_send_cable_reset(struct tcpc_device *tcpc_dev)
{
	tcpci_transmit(tcpc_dev, TCPC_TX_CABLE_RESET, 0, NULL);
	mdelay(10);
}
#define PD_DATA_OBJ_SIZE 7
static inline void cust2_src_send_sopp_msg(struct tcpc_device *tcpc_dev)
{
	int ret;
	uint16_t msg_hdr;
	uint32_t payload[PD_DATA_OBJ_SIZE];

	msg_hdr = PD_HEADER_SOP_PRIME(PD_DATA_VENDOR_DEF, 0, 0, 1);

	payload[0] = VDO_S(USB_SID_PD, CMDT_INIT, CMD_DISCOVER_IDENT, 0);

	ret = tcpci_transmit(tcpc_dev, TCPC_TX_SOP_PRIME, msg_hdr, payload);
	if (ret < 0)
		PD_ERR("[SendMsg] Failed, %d\r\n", ret);
	mdelay(1);
}

static inline int cust2_src_polling_rx(struct tcpc_device *tcpc_dev)
{
	uint32_t alert = 0;
	uint8_t i = 0;

	while(i++ < 3) {
		TYPEC_DBG("in loop %d\r\n", i);
		mdelay(2);
		tcpci_get_alert_status(tcpc_dev, &alert);
		if(alert & TCPC_REG_ALERT_RX_STATUS) {
			pd_msg_t *pd_msg;
			enum tcpm_transmit_type type;
			pd_event_t pd_event;

			TYPEC_DBG("in loop RX\r\n");

			pd_msg = pd_alloc_msg(tcpc_dev);
			if (pd_msg) {
				tcpci_get_message(tcpc_dev,
					pd_msg->payload, &pd_msg->msg_hdr, &type);

				pd_event.pd_msg = pd_msg;
				pd_dpm_dfp_inform_cable_vdo(&tcpc_dev->pd_port, &pd_event);

				pd_free_msg(tcpc_dev, pd_msg);
				return 1;
			}
		}
	}
	TYPEC_DBG("out loop\r\n");
	return 0;
}
#endif  /* CONFIG_TYPEC_CAP_CUSTOM_SRC2 */

static inline void typec_custom_src_attached_entry(
	struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();
	struct pd_dpm_typec_state typec_state;

	memset(&typec_state, 0, sizeof(typec_state));
	typec_state.new_state = TYPEC_ATTACHED_CUSTOM_SRC;

	if (cc1 == TYPEC_CC_VOLT_SNK_DFT && cc2 == TYPEC_CC_VOLT_SNK_DFT) {
		TYPEC_NEW_STATE(typec_attached_custom_src);
		tcpc_dev->typec_attach_new = TYPEC_ATTACHED_CUSTOM_SRC;
		pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, &typec_state);
		return;
	}
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
	if (support_smart_holder) {
		if ((cc1 == TYPEC_CC_VOLT_SNK_1_5 && cc2 == TYPEC_CC_VOLT_SNK_1_5)
			|| (cc1 == TYPEC_CC_VOLT_SNK_3_0 && cc2 == TYPEC_CC_VOLT_SNK_3_0)) {
			int ret;

			TYPEC_DBG("Rp 1.5A or 3A\r\n");

			/* 1. treat cc1 as cc */
			tcpci_set_polarity(tcpc_dev, 1);
			/* 2. configure tx/rx cap for sop' message */
			tcpci_set_rx_enable(tcpc_dev, PD_RX_CAP_PE_READY_UFP | TCPC_RX_CAP_SOP_PRIME | TCPC_RX_CAP_CUST_SRC2);
			/* 3. send cable reset to reset msg_id */
			cust2_src_send_cable_reset(tcpc_dev);
			/* 4. Send SOP' DiscoverIdentity message to emark */
			cust2_src_send_sopp_msg(tcpc_dev);
			/* 5. polling to wait sop' message */
			ret = cust2_src_polling_rx(tcpc_dev);
			/* 6. force clear trx alert status */
			tcpci_alert_status_clear(tcpc_dev, TCPC_REG_ALERT_RX_MASK | TCPC_REG_ALERT_TX_MASK);
			/* 7. configure tx/rx cap to disable */
			tcpci_set_rx_enable(tcpc_dev, PD_RX_CAP_PE_DISABLE);
			if (ret) {
				TYPEC_NEW_STATE(typec_attached_custom_src2);
				tcpc_dev->typec_attach_new = TYPEC_ATTACHED_CUSTOM_SRC2;
				typec_state.new_state = TYPEC_ATTACHED_CUSTOM_SRC2;
				pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, &typec_state);
			} else {
				TYPEC_NEW_STATE(typec_attached_custom_src2);
				tcpc_dev->typec_attach_new = TYPEC_ATTACHED_CUSTOM_SRC2;
			}
			return;
		}
	}
#endif  /* CONFIG_TYPEC_CAP_CUSTOM_SRC2 */
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */
#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	TYPEC_DBG("[Warning] Same Rp (%d)\r\n", typec_get_cc1());
#else
	TYPEC_DBG("[Warning] CC Both Rp\r\n");
#endif
}

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK

static inline uint8_t typec_get_sink_dbg_acc_rp_level(
	int cc1, int cc2)
{
	if (cc2 == TYPEC_CC_VOLT_SNK_DFT)
		return cc1;
	else
		return TYPEC_CC_VOLT_SNK_DFT;
}

static inline void typec_sink_dbg_acc_attached_entry(
	struct tcpc_device *tcpc_dev)
{
	bool polarity;
	uint8_t rp_level;

	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();

	if (cc1 == cc2) {
		typec_custom_src_attached_entry(tcpc_dev);
		return;
	}

	TYPEC_NEW_STATE(typec_attached_dbgacc_snk);

	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_DBGACC_SNK;

	polarity = cc2 > cc1;

	if (polarity)
		rp_level = typec_get_sink_dbg_acc_rp_level(cc2, cc1);
	else
		rp_level = typec_get_sink_dbg_acc_rp_level(cc1, cc2);

	typec_set_plug_orient(tcpc_dev, TYPEC_CC_RD, polarity);
	tcpc_dev->typec_remote_rp_level = rp_level;

	tcpci_report_power_control(tcpc_dev, true);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
}
#else
static inline void typec_sink_dbg_acc_attached_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_custom_src_attached_entry(tcpc_dev);
}
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */


/*
 * [BLOCK] Try.SRC / TryWait.SNK
 */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE

static inline bool typec_role_is_try_src(
	struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_role != TYPEC_ROLE_TRY_SRC)
		return false;

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap)
		return false;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	return true;
}

static inline void typec_try_src_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_try_src);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

static inline void typec_trywait_snk_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_trywait_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	tcpci_set_vconn(tcpc_dev, false);
	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
	tcpci_source_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_0V, 0);
	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

	tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
}

static inline void typec_trywait_snk_pe_entry(struct tcpc_device *tcpc_dev)
{
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->typec_attach_old) {
		TYPEC_NEW_STATE(typec_trywait_snk_pe);
		return;
	}
#endif

	typec_trywait_snk_entry(tcpc_dev);
}

#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

/*
 * [BLOCK] Try.SNK / TryWait.SRC
 */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK

static inline bool typec_role_is_try_sink(
	struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_role != TYPEC_ROLE_TRY_SNK)
		return false;

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_dev->typec_during_role_swap)
		return false;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

	return true;
}

static inline void typec_try_snk_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_try_snk);
	tcpc_dev->typec_drp_try_timeout = false;

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

static inline void typec_trywait_src_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_trywait_src);
	tcpc_dev->typec_drp_try_timeout = false;
#ifdef RICHTEK_PD_COMPLIANCE_FAKE_TRY_SNK_RP
	tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE5V_TOUT);
#endif	/* RICHTEK_PD_COMPLIANCE_FAKE_TRY_SNK_RP */

	tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_DFT);
	tcpci_sink_vbus(tcpc_dev, TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_0V, 0);
	tcpc_enable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);
}

#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

/*
 * [BLOCK] Attach / Detach
 */

static inline void typec_cc_snk_detect_vsafe5v_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	if (!typec_check_cc_any(TYPEC_CC_VOLT_OPEN)) {	/* Both Rp */
		typec_sink_dbg_acc_attached_entry(tcpc_dev);
		return;
	}

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (typec_role_is_try_src(tcpc_dev)) {
		if (tcpc_dev->typec_state == typec_attachwait_snk) {
			typec_try_src_entry(tcpc_dev);
			return;
		}
	}
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_sink_attached_entry(tcpc_dev);
}

static inline void typec_cc_snk_detect_entry(struct tcpc_device *tcpc_dev)
{
	/* If Port Partner act as Source without VBUS, wait vSafe5V */
	if (tcpci_check_vbus_valid(tcpc_dev))
		typec_cc_snk_detect_vsafe5v_entry(tcpc_dev);
	else{
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SNK_VSAFE5V);
#ifdef RICHTEK_PD_COMPLIANCE_FAKE_TRY_SNK_RP
	if (tcpc_dev->typec_state == typec_try_snk)
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE5V_TOUT);
#endif	/* RICHTEK_PD_COMPLIANCE_FAKE_TRY_SNK_RP */
	}
}

static inline void typec_cc_src_detect_vsafe0v_entry(
	struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if (typec_role_is_try_sink(tcpc_dev)) {
		if (tcpc_dev->typec_state == typec_attachwait_src) {
			typec_try_snk_entry(tcpc_dev);
			return;
		}
	}
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_source_attached_entry(tcpc_dev);
}

static inline void typec_cc_src_detect_entry(
	struct tcpc_device *tcpc_dev)
{
	/* If Port Partner act as Sink with low VBUS, wait vSafe0v */
	bool vbus_absent = tcpci_check_vsafe0v(tcpc_dev, true);

	if (vbus_absent)
		typec_cc_src_detect_vsafe0v_entry(tcpc_dev);
	else
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_SRC_VSAFE0V);
}

static inline void typec_cc_src_remove_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (typec_role_is_try_src(tcpc_dev)) {
		switch (tcpc_dev->typec_state) {
		case typec_attached_src:
			typec_trywait_snk_pe_entry(tcpc_dev);
			return;
		case typec_try_src:
			typec_trywait_snk_entry(tcpc_dev);
			return;
		}
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
}

static inline void typec_cc_snk_remove_entry(struct tcpc_device *tcpc_dev)
{
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if (tcpc_dev->typec_state == typec_try_snk) {
		typec_trywait_src_entry(tcpc_dev);
		return;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
}

/*
 * [BLOCK] CC Change (after debounce)
 */

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE

static inline bool typec_check_cc_stable_source(
	struct tcpc_device *tcpc_dev)
{
	int ret, cc1a, cc2a, cc1b, cc2b;
	bool check_stable = false;

	if (!(tcpc_dev->tcpc_flags & TCPC_FLAGS_CHECK_CC_STABLE))
		return true;

	cc1a = typec_get_cc1();
	cc2a = typec_get_cc2();

	if ((cc1a == TYPEC_CC_VOLT_RD) && (cc2a == TYPEC_CC_VOLT_RD))
		check_stable = true;

	if ((cc1a == TYPEC_CC_VOLT_RA) || (cc2a == TYPEC_CC_VOLT_RA))
		check_stable = true;

	if (check_stable) {
		TYPEC_INFO("CC Stable Check...\r\n");
		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a)) {
			TYPEC_INFO("CC Unstable... %d/%d\r\n", cc1b, cc2b);

			if ((cc1b == TYPEC_CC_VOLT_RD) &&
				(cc2b != TYPEC_CC_VOLT_RD))
				return true;

			if ((cc1b != TYPEC_CC_VOLT_RD) &&
				(cc2b == TYPEC_CC_VOLT_RD))
				return true;

			typec_cc_src_remove_entry(tcpc_dev);
			return false;
		}

		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a)) {
			TYPEC_INFO("CC Unstable1... %d/%d\r\n", cc1b, cc2b);

			if ((cc1b == TYPEC_CC_VOLT_RD) &&
						(cc2b != TYPEC_CC_VOLT_RD))
				return true;

			if ((cc1b != TYPEC_CC_VOLT_RD) &&
						(cc2b == TYPEC_CC_VOLT_RD))
				return true;

			typec_cc_src_remove_entry(tcpc_dev);
			return false;
		}
	}

	return true;
}

static inline bool typec_check_cc_stable_sink(
	struct tcpc_device *tcpc_dev)
{
	int ret, cc1a, cc2a, cc1b, cc2b;

	if (!(tcpc_dev->tcpc_flags & TCPC_FLAGS_CHECK_CC_STABLE))
		return true;

	cc1a = typec_get_cc1();
	cc2a = typec_get_cc2();

	if ((cc1a != TYPEC_CC_VOLT_OPEN) && (cc2a != TYPEC_CC_VOLT_OPEN)) {
		TYPEC_INFO("CC Stable Check...\r\n");
		typec_set_polarity(tcpc_dev, !tcpc_dev->typec_polarity);
		mdelay(1);

		ret = tcpci_get_cc(tcpc_dev);
		cc1b = typec_get_cc1();
		cc2b = typec_get_cc2();

		if ((cc1b != cc1a) || (cc2b != cc2a))
			TYPEC_INFO("CC Unstable... %d/%d\r\n", cc1b, cc2b);
	}

	return true;
}

#endif

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE

static inline bool typec_legacy_charge(
	struct tcpc_device *tcpc_dev)
{
	int i, vbus_level = 0;

	TYPEC_INFO("LC->Charge\r\n");
	tcpci_source_vbus(tcpc_dev,
		TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_5V, 100);

	for (i = 0; i < 6; i++) { /* 275 ms */
		vbus_level = tcpm_inquire_vbus_level(tcpc_dev, true);
		if (vbus_level >= TCPC_VBUS_VALID)
			return true;
		msleep(50);
	}

	TYPEC_INFO("LC->Charge Failed\r\n");
	return false;
}

static inline bool typec_legacy_discharge(
	struct tcpc_device *tcpc_dev)
{
	int i, vbus_level = 0;

	TYPEC_INFO("LC->Discharge\r\n");
	tcpci_source_vbus(tcpc_dev,
		TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SOURCE_0V, 0);

	for (i = 0; i < 13; i++) { /* 650 ms */
		vbus_level = tcpm_inquire_vbus_level(tcpc_dev, true);
		if (vbus_level < TCPC_VBUS_VALID)
			return true;
		msleep(50);
	}

	TYPEC_INFO("LC->Discharge Failed\r\n");
	return false;
}

static inline bool typec_check_legacy_cable(
	struct tcpc_device *tcpc_dev)
{
	bool check_legacy = false;

	if (typec_check_cc(TYPEC_CC_VOLT_RD, TYPEC_CC_VOLT_OPEN) ||
		typec_check_cc(TYPEC_CC_VOLT_OPEN, TYPEC_CC_VOLT_RD))
		check_legacy = true;

	if (check_legacy &&
		tcpc_dev->typec_legacy_cable_suspect >=
					TCPC_LEGACY_CABLE_CONFIRM) {

		TYPEC_INFO("LC->Suspect\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_1_5);
		tcpc_dev->typec_legacy_cable_suspect = 0;
		mdelay(1);

		if (tcpci_get_cc(tcpc_dev) != 0) {
			TYPEC_INFO("LC->Confirm\r\n");
			tcpc_dev->typec_legacy_cable = true;
			tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_NOT_LEGACY);

			typec_legacy_charge(tcpc_dev);
			typec_legacy_discharge(tcpc_dev);
			TYPEC_INFO("LC->Stable\r\n");

			return true;
		}

		tcpc_dev->typec_legacy_cable = false;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
	}

	return false;
}

#endif /* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

/*
 * [BLOCK] CC Change (after debounce)
 */

static inline bool typec_debug_acc_attached_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_NEW_STATE(typec_debugaccessory);
	TYPEC_DBG("[Debug] CC1&2 Both Rd\r\n");
	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_DEBUG;
	return true;
}

static inline bool typec_is_fake_ra(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_local_cc == TYPEC_CC_RP_3_0
		|| tcpc_dev->typec_local_cc == TYPEC_CC_DRP_3_0) {
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_DFT);
		mdelay(5);
		return tcpci_get_cc(tcpc_dev) != 0;
	}
	return false;
}
static inline bool typec_audio_acc_attached_entry(struct tcpc_device *tcpc_dev)
{
#ifdef RICHTEK_PD_COMPLIANCE_FAKE_AUDIO_ACC
	if (typec_is_fake_ra(tcpc_dev)) {
		TYPEC_DBG("[Audio] Fake Both Ra\r\n");
		if (typec_check_cc_any(TYPEC_CC_VOLT_RD))
			typec_cc_src_detect_entry(tcpc_dev);
		else
			typec_cc_src_remove_entry(tcpc_dev);
		return 0;
	}
#endif	/* RICHTEK_PD_COMPLIANCE_FAKE_AUDIO_ACC */
	tcpci_mask_vsafe0v(tcpc_dev, 0);
	TYPEC_NEW_STATE(typec_audioaccessory);
	TYPEC_DBG("[Audio] CC1&2 Both Ra\r\n");
	tcpc_dev->typec_attach_new = TYPEC_ATTACHED_AUDIO;

	return true;
}

static inline bool typec_cc_change_source_entry(struct tcpc_device *tcpc_dev)
{
	bool src_remove = false;

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE
	if (!typec_check_cc_stable_source(tcpc_dev))
		return true;
#endif

	switch (tcpc_dev->typec_state) {
	case typec_attached_src:
		if (typec_get_cc_res() != TYPEC_CC_VOLT_RD)
			src_remove = true;
		break;
	case typec_audioaccessory:
		if (!typec_check_cc_both(TYPEC_CC_VOLT_RA))
			src_remove = true;
		break;
	case typec_debugaccessory:
		if (!typec_check_cc_both(TYPEC_CC_VOLT_RD))
			src_remove = true;
		break;
	default:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RD))
			typec_debug_acc_attached_entry(tcpc_dev);
		else if (typec_check_cc_both(TYPEC_CC_VOLT_RA))
			typec_audio_acc_attached_entry(tcpc_dev);
		else if (typec_check_cc_any(TYPEC_CC_VOLT_RD))
			typec_cc_src_detect_entry(tcpc_dev);
		else
			src_remove = true;
		break;
	}

	if (src_remove)
		typec_cc_src_remove_entry(tcpc_dev);

	return true;
}

static inline bool typec_attached_snk_cc_change(struct tcpc_device *tcpc_dev)
{
	uint8_t cc_res = typec_get_cc_res();

	if (cc_res != tcpc_dev->typec_remote_rp_level) {
		TYPEC_INFO("RpLvl Change\r\n");
		tcpc_dev->typec_remote_rp_level = cc_res;
		
#ifdef CONFIG_USB_POWER_DELIVERY
		if (tcpc_dev->pd_port.pd_prev_connected)
			return true;
#endif	/* CONFIG_USB_POWER_DELIVERY */
		
		tcpci_sink_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
	}

	return true;
}

static inline bool typec_cc_change_sink_entry(struct tcpc_device *tcpc_dev)
{
	bool snk_remove = false;

#ifdef CONFIG_TYPEC_CHECK_CC_STABLE
	typec_check_cc_stable_sink(tcpc_dev);
#endif

	switch (tcpc_dev->typec_state) {
	case typec_attached_snk:
		if (typec_get_cc_res() == TYPEC_CC_VOLT_OPEN)
			snk_remove = true;
		else
			typec_attached_snk_cc_change(tcpc_dev);
		break;

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	case typec_attached_dbgacc_snk:
		if (typec_get_cc_res() == TYPEC_CC_VOLT_OPEN)
			snk_remove = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	case typec_attached_custom_src:
		if (typec_check_cc_any(TYPEC_CC_VOLT_OPEN))
			snk_remove = true;
		break;
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

	default:
		if (!typec_is_cc_open())
			typec_cc_snk_detect_entry(tcpc_dev);
		else
			snk_remove = true;
	}

	if (snk_remove)
		typec_cc_snk_remove_entry(tcpc_dev);		

	return true;
}

static inline bool typec_is_act_as_sink_role(
	struct tcpc_device *tcpc_dev)
{
	bool as_sink = true;
	uint8_t cc_sum;

	switch (tcpc_dev->typec_local_cc & 0x07) {
	case TYPEC_CC_RP:
		as_sink = false;
		break;
	case TYPEC_CC_RD:
		as_sink = true;
		break;
	case TYPEC_CC_DRP:
		cc_sum = typec_get_cc1() + typec_get_cc2();
		as_sink = (cc_sum >= TYPEC_CC_VOLT_SNK_DFT);
		break;
	}

	return as_sink;
}

static inline bool typec_handle_cc_changed_entry(struct tcpc_device *tcpc_dev)
{
	TYPEC_INFO("[CC_Change] %d/%d\r\n", typec_get_cc1(), typec_get_cc2());

	tcpc_dev->typec_attach_new = tcpc_dev->typec_attach_old;

	if (typec_is_act_as_sink_role(tcpc_dev))
		typec_cc_change_sink_entry(tcpc_dev);
	else
		typec_cc_change_source_entry(tcpc_dev);

	pd_dpm_handle_pe_event(PD_DPM_PE_ABNORMAL_CC_CHANGE_HANDLER, NULL);
	typec_alert_attach_state_change(tcpc_dev);
	return true;
}

/*
 * [BLOCK] Handle cc-change event
 */

static inline void typec_attach_wait_entry(struct tcpc_device *tcpc_dev)
{
	bool as_sink;

#ifdef CONFIG_USB_POWER_DELIVERY
	bool pd_en = tcpc_dev->pd_port.pd_prev_connected;
#else
	bool pd_en = false;
#endif	/* CONFIG_USB_POWER_DELIVERY */

	if (tcpc_dev->typec_attach_old == TYPEC_ATTACHED_SNK && !pd_en) {
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		TYPEC_DBG("RpLvl Alert\r\n");
		return;
	}

	if (tcpc_dev->typec_attach_old ||
		tcpc_dev->typec_state == typec_attached_src) {
		tcpc_reset_typec_debounce_timer(tcpc_dev);
		TYPEC_DBG("Attached, Ignore cc_attach\r\n");
		return;
	}

	tcpci_mask_vsafe0v(tcpc_dev, 1);

	switch (tcpc_dev->typec_state) {

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return;

	case typec_trywait_snk:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		return;
#endif

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_try_snk:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return;

	case typec_trywait_src:
		hw_usb_ldo_supply_enable(HW_USB_LDO_CTRL_TYPECPD);
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_TRYSRCDEBOUNCE);
		return;
#endif

#ifdef CONFIG_USB_POWER_DELIVERY
	case typec_unattachwait_pe:
		TYPEC_INFO("Force PE Idle\r\n");
		tcpc_dev->pd_wait_pe_idle = false;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_PE_IDLE);
		typec_unattached_power_entry(tcpc_dev);
		break;
#endif
	}

	as_sink = typec_is_act_as_sink_role(tcpc_dev);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (!as_sink && typec_check_legacy_cable(tcpc_dev))
		return;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (as_sink)
		TYPEC_NEW_STATE(typec_attachwait_snk);
	else
		TYPEC_NEW_STATE(typec_attachwait_src);

	tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
}

#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
static inline int typec_attached_snk_cc_detach(struct tcpc_device *tcpc_dev)
{
	int vbus_valid = tcpci_check_vbus_valid(tcpc_dev);
	bool detach_by_cc = false;

	if (tcpc_dev->typec_during_direct_charge)
		detach_by_cc = true;

#ifdef CONFIG_USB_POWER_DELIVERY
	/* For Source detach during HardReset */
	if ((!vbus_valid) &&
		tcpc_dev->pd_wait_hard_reset_complete) {
		detach_by_cc = true;
		TYPEC_DBG("Detach_CC (HardReset)\r\n");
	}
#endif

	if (detach_by_cc)
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);

	return 0;
}
#endif	/* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

static inline void typec_detach_wait_entry(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	bool suspect_legacy = false;

	if (tcpc_dev->typec_state == typec_attachwait_src)
		suspect_legacy = true;
	else if (tcpc_dev->typec_state == typec_attached_src) {
		if (tcpc_dev->typec_attach_old != TYPEC_ATTACHED_SRC)
			suspect_legacy = true;
	}

	if (suspect_legacy) {
		tcpc_dev->typec_legacy_cable_suspect++;
		TYPEC_DBG("LC_suspect: %d\r\n",
			tcpc_dev->typec_legacy_cable_suspect);
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	switch (tcpc_dev->typec_state) {
#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
	case typec_attached_snk:
		typec_attached_snk_cc_detach(tcpc_dev);
		break;
#endif /* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

	case typec_audioaccessory:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_CCDEBOUNCE);
		break;

#ifdef TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE
	case typec_attached_src:
		TYPEC_INFO("Exit Attached.SRC immediately\r\n");
		tcpc_reset_typec_debounce_timer(tcpc_dev);

		/* force to terminate TX */
		tcpci_init(tcpc_dev, true);

		typec_cc_src_remove_entry(tcpc_dev);
		typec_alert_attach_state_change(tcpc_dev);
		break;
#endif /* TYPEC_EXIT_ATTACHED_SRC_NO_DEBOUNCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		if (tcpc_dev->typec_drp_try_timeout)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		else {
			tcpc_reset_typec_debounce_timer(tcpc_dev);
			TYPEC_DBG("[Try] Igrone cc_detach\r\n");
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_trywait_src:
		if (tcpc_dev->typec_drp_try_timeout)
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		else {
			tcpc_reset_typec_debounce_timer(tcpc_dev);
			TYPEC_DBG("[Try] Igrone cc_detach\r\n");
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */
	default:
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		break;
	}
}

static inline bool typec_is_cc_attach(struct tcpc_device *tcpc_dev)
{
	bool cc_attach = false;
	int cc1 = typec_get_cc1();
	int cc2 = typec_get_cc2();
	int cc_res = typec_get_cc_res();

	tcpc_dev->typec_cable_only = false;

	switch (tcpc_dev->typec_attach_old) {
	case TYPEC_ATTACHED_SNK:
	case TYPEC_ATTACHED_SRC:
	
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	case TYPEC_ATTACHED_CUSTOM_SRC:
#endif	/* CONFIG_TYPEC_CAP_CUSTOM_SRC */

#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	case TYPEC_ATTACHED_DBGACC_SNK:
#endif	/* CONFIG_TYPEC_CAP_DBGACC_SNK */
		if ((cc_res != TYPEC_CC_VOLT_OPEN) &&
				(cc_res != TYPEC_CC_VOLT_RA))
			cc_attach = true;
		break;
		
	case TYPEC_ATTACHED_AUDIO:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RA))
			cc_attach = true;
		break;

	case TYPEC_ATTACHED_DEBUG:
		if (typec_check_cc_both(TYPEC_CC_VOLT_RD))
			cc_attach = true;
		break;
		
	default:	/* TYPEC_UNATTACHED */
		if (cc1 != TYPEC_CC_VOLT_OPEN)
			cc_attach = true;

		if (cc2 != TYPEC_CC_VOLT_OPEN)
			cc_attach = true;

		/* Cable Only, no device */
		if ((cc1+cc2) == TYPEC_CC_VOLT_RA) {
#ifdef RICHTEK_PD_COMPLIANCE_FAKE_EMRAK_ONLY
			if (typec_is_fake_ra(tcpc_dev)) {
				TYPEC_DBG("[Cable] Fake Ra\r\n");
				if ((cc1+cc2) == TYPEC_CC_VOLT_RD)
					cc_attach = true;
					break;
			}
#endif	/* RICHTEK_PD_COMPLIANCE_FAKE_AUDIO_ACC */
			cc_attach = false;
			tcpc_dev->typec_cable_only = true;
			TYPEC_DBG("[Cable] Ra Only\r\n");
		}
		break;
	}

	return cc_attach;
}

static inline int typec_get_rp_present_flag(struct tcpc_device *tcpc_dev)
{
	uint8_t rp_flag = 0;

	if (tcpc_dev->typec_remote_cc[0] >= TYPEC_CC_VOLT_SNK_DFT
		&& tcpc_dev->typec_remote_cc[0] != TYPEC_CC_DRP_TOGGLING)
		rp_flag |= 1;

	if (tcpc_dev->typec_remote_cc[1] >= TYPEC_CC_VOLT_SNK_DFT
		&& tcpc_dev->typec_remote_cc[1] != TYPEC_CC_DRP_TOGGLING)
		rp_flag |= 2;

	return rp_flag;
}

int tcpc_typec_handle_cc_change(struct tcpc_device *tcpc_dev)
{
	int ret;
	uint8_t rp_present;

	rp_present = typec_get_rp_present_flag(tcpc_dev);

	ret = tcpci_get_cc(tcpc_dev);

	if (ret < 0)
		return ret;

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		if (tcpc_dev->typec_lpm)
			typec_enter_low_power_mode(tcpc_dev);
		return 0;
	}

#ifdef CONFIG_TYPEC_CAP_NORP_SRC
	typec_try_exit_norp_src(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

	TYPEC_INFO("[CC_Alert] %d/%d\r\n", typec_get_cc1(), typec_get_cc2());

	typec_disable_low_power_mode(tcpc_dev);

#ifdef CONFIG_POGO_PIN
	if (is_pogopin_support()){
		if (typec_is_cc_open_state(tcpc_dev))
			return 0;
	}
#endif

#ifdef CONFIG_TYPEC_CAP_A2C_C2C
	if ((rp_present == 0) &&
		typec_get_rp_present_flag(tcpc_dev) != 0) {
		TYPEC_DBG("A2C_C2C: Detect\r\n");
		tcpc_restart_timer(tcpc_dev, TYPEC_RT_TIMER_A2C_C2C);
	}
#endif	/* CONFIG_TYPEC_CAP_A2C_C2C */

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable && !typec_is_cc_open()) {
		TYPEC_INFO("LC->Detached (CC)\r\n");
		tcpc_dev->typec_legacy_cable = false;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
		return 0;
	}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Ignore CC_Alert\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery) {
		TYPEC_DBG("[Recovery] Ignore CC_Alert\r\n");
		return 0;
	}
#endif /* CONFIG_USB_POWER_DELIVERY */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	if ((tcpc_dev->typec_state == typec_try_snk) &&
		(!tcpc_dev->typec_drp_try_timeout)) {
		TYPEC_DBG("[Try.SNK] Ignore CC_Alert\r\n");
		return 0;
	}

	if (tcpc_dev->typec_state == typec_trywait_src_pe) {
		TYPEC_DBG("[Try.PE] Ignore CC_Alert\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SINK */

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	if (tcpc_dev->typec_state == typec_trywait_snk_pe) {
		TYPEC_DBG("[Try.PE] Ignore CC_Alert\r\n");
		return 0;
	}
#endif	/* CONFIG_TYPEC_CAP_TRY_SOURCE */

	if (tcpc_dev->typec_state == typec_attachwait_snk
		|| tcpc_dev->typec_state == typec_attachwait_src)
		typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	hisi_usb_check_hifi_usb_status(HIFI_USB_TCPC);

	if (typec_is_cc_attach(tcpc_dev)) {
		typec_attach_wait_entry(tcpc_dev);
	}
	else
		typec_detach_wait_entry(tcpc_dev);

	return 0;
}

/*
 * [BLOCK] Handle timeout event
 */

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
static inline int typec_handle_drp_try_timeout(struct tcpc_device *tcpc_dev)
{
	bool src_detect = false, en_timer;

	tcpc_dev->typec_drp_try_timeout = true;
	tcpc_disable_timer(tcpc_dev, TYPEC_TRY_TIMER_DRP_TRY);

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	if (typec_check_cc1(TYPEC_CC_VOLT_RD) ||
		typec_check_cc2(TYPEC_CC_VOLT_RD)) {
		src_detect = true;
	}

	switch (tcpc_dev->typec_state) {
#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_try_src:
		en_timer = !src_detect;
		break;
#endif /* CONFIG_TYPEC_CAP_TRY_SOURCE */

#ifdef CONFIG_TYPEC_CAP_TRY_SINK
	case typec_trywait_src:
		en_timer = !src_detect;
		break;

	case typec_try_snk:
		en_timer = true;
		break;
#endif /* CONFIG_TYPEC_CAP_TRY_SINK */

	default:
		en_timer = false;
		break;
	}

	if (en_timer)
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);

	return 0;
}
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

static inline int typec_handle_debounce_timeout(struct tcpc_device *tcpc_dev)
{
	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY);
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	typec_handle_cc_changed_entry(tcpc_dev);
	return 0;
}


#ifdef CONFIG_USB_POWER_DELIVERY

static inline int typec_handle_error_recovery_timeout(
						struct tcpc_device *tcpc_dev)
{
	/* TODO: Check it later */
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;

	mutex_lock(&tcpc_dev->access_lock);
	tcpc_dev->pd_wait_error_recovery = false;
	mutex_unlock(&tcpc_dev->access_lock);

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
	typec_alert_attach_state_change(tcpc_dev);

	return 0;
}

static inline int typec_handle_pe_idle(struct tcpc_device *tcpc_dev)
{
	switch (tcpc_dev->typec_state) {

#ifdef CONFIG_TYPEC_CAP_TRY_SOURCE
	case typec_trywait_snk_pe:
		typec_trywait_snk_entry(tcpc_dev);
		break;
#endif

	case typec_unattachwait_pe:
		typec_unattached_entry(tcpc_dev);
		break;

	default:
		TYPEC_DBG("Dummy pe_idle\r\n");
		break;
	}

	return 0;
}
#endif /* CONFIG_USB_POWER_DELIVERY */

static inline int typec_handle_src_reach_vsafe0v(struct tcpc_device *tcpc_dev)
{
	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
		return 0;
	}

	typec_cc_src_detect_vsafe0v_entry(tcpc_dev);
	return 0;
}

static inline int typec_handle_src_toggle_timeout(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_state == typec_unattached_src) {
		TYPEC_NEW_STATE(typec_unattached_snk);
		tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
		typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
	}

	return 0;
}

static inline int typec_handle_safe0v_tout(struct tcpc_device *tcpc_dev)
{
        int ret = 0;
	int vbus = 0;
        uint16_t power_status;

        if (!tcpci_check_vbus_valid(tcpc_dev))
                ret = tcpc_typec_handle_vsafe0v(tcpc_dev);
        else {
                TYPEC_INFO("VBUS still Valid!!\r\n");
#ifdef CONFIG_HUAWEI_DSM
		vbus = charge_get_vbus();
		power_dsm_dmd_report_format(POWER_DSM_PD, DSM_PD_TYPEC_VBUS_VALID, \
                "\nVBUS still Valid vbus = %d\n", vbus);
#endif

                tcpci_get_power_status(tcpc_dev, &power_status);
                tcpci_vbus_level_init(tcpc_dev, power_status);

                TCPC_INFO("Safe0V TOUT: ps=%d\r\n", tcpc_dev->vbus_level);

                if (!tcpci_check_vbus_valid(tcpc_dev))
                        ret = tcpc_typec_handle_vsafe0v(tcpc_dev);
		else {
#ifdef CONFIG_HUAWEI_DSM
			TYPEC_INFO("VBUS still Valid After Update!!\r\n");
			vbus = charge_get_vbus();
			power_dsm_dmd_report_format(POWER_DSM_PD, DSM_PD_TYPEC_VBUS_STILL_VALID, \
                "VBUS still Valid After Update vbus = %d\n", vbus);
#endif
		}
        }

        return ret;
}


int tcpc_typec_handle_timeout(struct tcpc_device *tcpc_dev, uint32_t timer_id)
{
	int ret = 0;

#ifdef CONFIG_TYPEC_CAP_TRY_STATE
	if (timer_id == TYPEC_TRY_TIMER_DRP_TRY)
		return typec_handle_drp_try_timeout(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_TRY_STATE */

	if (timer_id >= TYPEC_TIMER_START_ID)
		tcpc_reset_typec_debounce_timer(tcpc_dev);
	else if (timer_id >= TYPEC_RT_TIMER_START_ID)
		tcpc_disable_timer(tcpc_dev, timer_id);

#ifdef CONFIG_POGO_PIN
	if (is_pogopin_support()) {
		if (timer_id == TYPEC_TIMER_ERROR_RECOVERY)
			return typec_handle_error_recovery_timeout(tcpc_dev);
		else if (timer_id == TYPEC_RT_TIMER_LEGACY)
			typec_alert_attach_state_change(tcpc_dev);
		else if (typec_is_cc_open_state(tcpc_dev)) {
			TYPEC_DBG("[Open] Ignore timer_evt\n");
			return 0;
		}
	}
#endif

#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Igrone timer_evt\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery &&
		(timer_id != TYPEC_TIMER_ERROR_RECOVERY)) {
		TYPEC_DBG("[Recovery] Igrone timer_evt\r\n");
		return 0;
	}
#endif

	switch (timer_id) {
	case TYPEC_TIMER_CCDEBOUNCE:
	case TYPEC_TIMER_PDDEBOUNCE:
	case TYPEC_TIMER_TRYSRCDEBOUNCE:
		ret = typec_handle_debounce_timeout(tcpc_dev);
		break;
#ifdef CONFIG_TYPEC_CAP_NORP_SRC
	case TYPEC_TIMER_NORP_SRC:
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */
		ret = typec_handle_norp_src_debounce_timeout(tcpc_dev);
		break;

#ifdef CONFIG_USB_POWER_DELIVERY
	case TYPEC_TIMER_ERROR_RECOVERY:
		ret = typec_handle_error_recovery_timeout(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_PE_IDLE:
		ret = typec_handle_pe_idle(tcpc_dev);
		break;
#endif /* CONFIG_USB_POWER_DELIVERY */

	case TYPEC_RT_TIMER_SAFE0V_DELAY:
		ret = typec_handle_src_reach_vsafe0v(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_LOW_POWER_MODE:
		if (tcpc_dev->typec_lpm)
			typec_try_low_power_mode(tcpc_dev);
		break;

	case TYPEC_TIMER_WAKEUP:
		if (tcpc_dev->typec_lpm || tcpc_dev->typec_cable_only) {
			tcpc_dev->typec_lpm = true;
			ret = tcpci_set_low_power_mode(tcpc_dev, true,
				(tcpc_dev->typec_role == TYPEC_ROLE_SRC) ?
				TYPEC_CC_RP : TYPEC_CC_DRP);
		}
		break;

#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT
	case TYPEC_RT_TIMER_SAFE0V_TOUT:
		ret = typec_handle_safe0v_tout(tcpc_dev);
                break;
#endif	/* CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_TIMEOUT */

	case TYPEC_TIMER_DRP_SRC_TOGGLE:
		ret = typec_handle_src_toggle_timeout(tcpc_dev);
		break;

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	case TYPEC_RT_TIMER_ROLE_SWAP_START:
		if (tcpc_dev->typec_during_role_swap == TYPEC_ROLE_SWAP_TO_SNK) {
			TYPEC_INFO("Role Swap to Sink\r\n");
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
			tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
		} else if (tcpc_dev->typec_during_role_swap == TYPEC_ROLE_SWAP_TO_SRC) {
			TYPEC_INFO("Role Swap to Source\r\n");
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RP);
			tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_STOP);
		}
		break;
		
	case TYPEC_RT_TIMER_ROLE_SWAP_STOP:
		if (tcpc_dev->typec_during_role_swap) {
			TYPEC_INFO("TypeC Role Swap Failed\r\n");
			tcpc_dev->typec_during_role_swap = false;
			tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_ROLE_SWAP */

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	case TYPEC_RT_TIMER_LEGACY:
		typec_alert_attach_state_change(tcpc_dev);
		break;

	case TYPEC_RT_TIMER_NOT_LEGACY:
		tcpc_dev->typec_legacy_cable = false;
		tcpc_dev->typec_legacy_cable_suspect = 0;
		break;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */
#ifdef RICHTEK_PD_COMPLIANCE_FAKE_TRY_SNK_RP
	case TYPEC_RT_TIMER_SAFE5V_TOUT:
		if (tcpc_dev->typec_state == typec_try_snk) {
			TYPEC_INFO("TrySNK: VSafe5V timeout\r\n");
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_3_0);
		}
		break;
#endif	/* RICHTEK_PD_COMPLIANCE_FAKE_TRY_SNK_RP */

#ifdef CONFIG_TYPEC_CAP_A2C_C2C
	case TYPEC_RT_TIMER_A2C_C2C:
		if (!typec_is_cc_open() && tcpci_check_vbus_valid(tcpc_dev)) {
			TYPEC_INFO("A2C_C2C: A2C\r\n");
			tcpc_dev->typec_a2c_cable = true;
		}
		break;
#endif	/* CONFIG_TYPEC_CAP_A2C_C2C */

	}

	return ret;
}

/*
 * [BLOCK] Handle ps-change event
 */

static inline int typec_handle_vbus_present(struct tcpc_device *tcpc_dev)
{
	switch (tcpc_dev->typec_wait_ps_change) {
	case TYPEC_WAIT_PS_SNK_VSAFE5V:
		typec_cc_snk_detect_vsafe5v_entry(tcpc_dev);
		typec_alert_attach_state_change(tcpc_dev);
		break;
	case TYPEC_WAIT_PS_SRC_VSAFE5V:
		typec_source_attached_with_vbus_entry(tcpc_dev);

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
		if (typec_get_cc_res() != TYPEC_CC_VOLT_RD) {
			TYPEC_DBG("Postpone AlertChange\r\n");
			tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY);
			break;
		}
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

		typec_alert_attach_state_change(tcpc_dev);
		break;
	}
	return 0;
}

static inline int typec_attached_snk_vbus_absent(struct tcpc_device *tcpc_dev)
{
#ifdef TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS
	if (tcpc_dev->typec_during_direct_charge &&
		!tcpci_check_vsafe0v(tcpc_dev, true)) {
		TYPEC_DBG("Ignore vbus_absent(snk), DirectCharge\r\n");
		return 0;
	}
#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_hard_reset_complete ||
				tcpc_dev->pd_hard_reset_event_pending) {
		if (typec_get_cc_res() != TYPEC_CC_VOLT_OPEN) {
			TYPEC_DBG
			    ("Ignore vbus_absent(snk), HReset & CC!=0\r\n");
			return 0;
		}
	}
#endif /* CONFIG_USB_POWER_DELIVERY */

	typec_unattach_wait_pe_idle_entry(tcpc_dev);
	typec_alert_attach_state_change(tcpc_dev);
#endif /* TYPEC_EXIT_ATTACHED_SNK_VIA_VBUS */

	return 0;
}

static inline int typec_handle_vbus_absent(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_USB_POWER_DELIVERY
	if (tcpc_dev->pd_wait_pr_swap_complete) {
		TYPEC_DBG("[PR.Swap] Igrone vbus_absent\r\n");
		return 0;
	}

	if (tcpc_dev->pd_wait_error_recovery) {
		TYPEC_DBG("[Recovery] Igrone vbus_absent\r\n");
		return 0;
	}
#endif

	switch (tcpc_dev->typec_state) {
	case typec_attached_snk:
#ifdef CONFIG_TYPEC_CAP_DBGACC_SNK
	case typec_attached_dbgacc_snk:
#endif     /* CONFIG_TYPEC_CAP_DBGACC_SNK */

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC
	case typec_attached_custom_src:
#endif     /* CONFIG_TYPEC_CAP_CUSTOM_SRC */
		typec_attached_snk_vbus_absent(tcpc_dev);
		break;
        }

#ifndef CONFIG_TCPC_VSAFE0V_DETECT
	tcpc_typec_handle_vsafe0v(tcpc_dev);
#endif /* #ifdef CONFIG_TCPC_VSAFE0V_DETECT */

	return 0;
}

int tcpc_typec_handle_ps_change(struct tcpc_device *tcpc_dev, int vbus_level)
{
#ifdef CONFIG_TYPEC_CAP_A2C_C2C
	if (vbus_level < TCPC_VBUS_VALID) {
		TYPEC_DBG("A2C_C2C: reset0\r\n");
		tcpc_dev->typec_a2c_cable = false;
		tcpc_disable_timer(tcpc_dev, TYPEC_RT_TIMER_A2C_C2C);
	}
#endif	/* CONFIG_TYPEC_CAP_A2C_C2C */

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (tcpc_dev->typec_legacy_cable) {
		if (vbus_level >= TCPC_VBUS_VALID) {
			TYPEC_INFO("LC->Attached\r\n");
			tcpc_dev->typec_legacy_cable = false;
			tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		} else if (vbus_level == TCPC_VBUS_SAFE0V) {
			TYPEC_INFO("LC->Detached (PS)\r\n");
			tcpc_dev->typec_legacy_cable = false;
			tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
			typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
		}
		return 0;
	}
#endif /* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_TYPEC_CAP_NORP_SRC
	if (typec_is_cc_no_res() || typec_is_cc_ra()) {
		if (!typec_try_enter_norp_src(tcpc_dev))
			typec_norp_src_detached_entry(tcpc_dev);
	}
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

	if (typec_is_drp_toggling()) {
		TYPEC_DBG("[Waring] DRP Toggling\r\n");
#ifdef CONFIG_TYPEC_CAP_NORP_SRC
		if (tcpc_dev->typec_lpm && !tcpc_dev->typec_cable_only)
			typec_enter_low_power_mode(tcpc_dev);
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */
		return 0;
	}

	if (vbus_level >= TCPC_VBUS_VALID)
		return typec_handle_vbus_present(tcpc_dev);
	else
		return typec_handle_vbus_absent(tcpc_dev);
}

/*
 * [BLOCK] Handle PE event
 */

#ifdef CONFIG_USB_POWER_DELIVERY

int tcpc_typec_advertise_explicit_contract(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_local_rp_level == TYPEC_CC_RP_DFT)
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RP_1_5);

	return 0;
}

int tcpc_typec_handle_pe_pr_swap(struct tcpc_device *tcpc_dev)
{
	int ret = 0;

	mutex_lock(&tcpc_dev->typec_lock);
	switch (tcpc_dev->typec_state) {
	case typec_attached_snk:
		TYPEC_NEW_STATE(typec_attached_src);
		tcpc_dev->typec_attach_old = TYPEC_ATTACHED_SRC;
		tcpci_set_cc(tcpc_dev, tcpc_dev->typec_local_rp_level);
		break;
	case typec_attached_src:
		TYPEC_NEW_STATE(typec_attached_snk);
		tcpc_dev->typec_attach_old = TYPEC_ATTACHED_SNK;
		tcpci_set_cc(tcpc_dev, TYPEC_CC_RD);
		break;
	default:
		break;
	}
	mutex_unlock(&tcpc_dev->typec_lock);
	return ret;
}

#endif /* CONFIG_USB_POWER_DELIVERY */

/*
 * [BLOCK] Handle reach vSafe0V event
 */

int tcpc_typec_handle_vsafe0v(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_wait_ps_change == TYPEC_WAIT_PS_SRC_VSAFE0V) {
#ifdef CONFIG_TYPEC_ATTACHED_SRC_SAFE0V_DELAY
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_SAFE0V_DELAY);
#else
		typec_handle_src_reach_vsafe0v(tcpc_dev);
#endif
	}

	return 0;
}

/*
 * [BLOCK] TCPCI TypeC I/F
 */

static const char *const typec_role_name[] = {
	"UNKNOWN",
	"SNK",
	"SRC",
	"DRP",
	"TrySRC",
	"TrySNK",
};

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
int tcpc_typec_swap_role(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_role < TYPEC_ROLE_DRP)
		return -1;

	if (tcpc_dev->typec_during_role_swap)
		return -1;
	
	switch (tcpc_dev->typec_attach_old) {
	case TYPEC_ATTACHED_SNK:
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_TO_SRC;
		break;
	case TYPEC_ATTACHED_SRC:
		tcpc_dev->typec_during_role_swap = TYPEC_ROLE_SWAP_TO_SNK;
		break;
	}

	if (tcpc_dev->typec_during_role_swap) {
		TYPEC_INFO("TypeC Role Swap Start\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
		tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_ROLE_SWAP_START);
		return 0;
	}

	return -1;
}
#endif /* CONFIG_TYPEC_CAP_ROLE_SWAP */

int tcpc_typec_notify_direct_charge(struct tcpc_device *tcpc_dev, bool dc)
{
	mutex_lock(&tcpc_dev->access_lock);
	tcpc_dev->typec_during_direct_charge = dc;
	mutex_unlock(&tcpc_dev->access_lock);
}
int tcpc_typec_set_rp_level(struct tcpc_device *tcpc_dev, uint8_t res)
{
	switch (res) {
	case TYPEC_CC_RP_DFT:
	case TYPEC_CC_RP_1_5:
	case TYPEC_CC_RP_3_0:
		TYPEC_INFO("TypeC-Rp: %d\r\n", res);
		tcpc_dev->typec_local_rp_level = res;
		break;

	default:
		TYPEC_INFO("TypeC-Unknown-Rp (%d)\r\n", res);
		return -1;
	}

#ifdef CONFIG_USB_PD_DBG_ALWAYS_LOCAL_RP
	tcpci_set_cc(tcpc_dev, tcpc_dev->typec_local_rp_level);
#else
	if ((tcpc_dev->typec_attach_old != TYPEC_UNATTACHED) &&
		(tcpc_dev->typec_attach_new != TYPEC_UNATTACHED)) {
		return tcpci_set_cc(tcpc_dev, res);
	}
#endif

	return 0;
}

int tcpc_typec_change_role(
	struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	uint8_t local_cc;
	bool force_unattach = false;

	if (typec_role == TYPEC_ROLE_UNKNOWN ||
		typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\r\n", typec_role);
		return -1;
	}

	mutex_lock(&tcpc_dev->access_lock);

	tcpc_dev->typec_role = typec_role;
	TYPEC_INFO("typec_new_role: %s\r\n", typec_role_name[typec_role]);

	local_cc = tcpc_dev->typec_local_cc & 0x07;

	if (typec_role == TYPEC_ROLE_SNK && local_cc == TYPEC_CC_RP)
		force_unattach = true;

	if (typec_role == TYPEC_ROLE_SRC && local_cc == TYPEC_CC_RD)
		force_unattach = true;

	if (tcpc_dev->typec_attach_new == TYPEC_UNATTACHED) {
		force_unattach = true;
		typec_disable_low_power_mode(tcpc_dev);
	}

	if (force_unattach) {
		TYPEC_DBG("force_unattach\r\n");
		tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
		mutex_unlock(&tcpc_dev->access_lock);
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_PDDEBOUNCE);
		return 0;
	}

	mutex_unlock(&tcpc_dev->access_lock);
	return 0;
}

#ifdef CONFIG_TYPEC_CAP_POWER_OFF_CHARGE
static int typec_init_power_off_charge(struct tcpc_device *tcpc_dev)
{
	bool cc_open;
	int ret = tcpci_get_cc(tcpc_dev);

	if (ret < 0)
		return ret;

	if (tcpc_dev->typec_role == TYPEC_ROLE_SRC)
		return 0;

	cc_open = typec_is_cc_open();

#ifndef CONFIG_TYPEC_CAP_NORP_SRC
	if (cc_open)
		return 0;
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */

	if (!tcpci_check_vbus_valid(tcpc_dev))
		return 0;

	TYPEC_DBG("PowerOffCharge\r\n");

	TYPEC_NEW_STATE(typec_unattached_snk);
	typec_wait_ps_change(tcpc_dev, TYPEC_WAIT_PS_DISABLE);

	tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);
	typec_enable_low_power_mode(tcpc_dev, TYPEC_CC_DRP);
	usleep_range(1000,2000);//wait low power for 1000~2000us
#ifdef CONFIG_TYPEC_CAP_NORP_SRC
	if (cc_open) {
		tcpc_enable_timer(tcpc_dev, TYPEC_TIMER_NORP_SRC);
	}
#endif	/* CONFIG_TYPEC_CAP_NORP_SRC */
	tcpci_set_cc(tcpc_dev, TYPEC_CC_DRP);

	return 1;
}
#endif	/* CONFIG_TYPEC_CAP_POWER_OFF_CHARGE */
static void tcpc_pd_dpm_hard_reset(void* client)
{
	int ret;
	TYPEC_INFO("%s++", __func__);
	if (NULL == client)
		return;
	ret = tcpm_hard_reset((struct tcpc_device*)client);
	if (TCPM_SUCCESS != ret) {
		TYPEC_INFO("hardreset failed, ret = %d", ret);
	}
	TYPEC_INFO("%s--", __func__);
}
static bool tcpc_pd_dpm_get_hw_dock_svid_exist(void* client)
{
	int ret;
	if (NULL == client)
		return false;
	struct tcpc_device* tcpc_dev = ((struct tcpc_device*)client);
	return tcpc_dev->huawei_dock_svid_exist;
}

static void tcpc_pd_dpm_set_voltage(void* client, int set_voltage)
{
	bool overload;
	int vol_mv = 0;
	int cur_ma = 0;
	int max_uw = 0;
	int ret, i;

	struct tcpc_device* tcpc_dev = ((struct tcpc_device*)client);
	struct local_sink_cap sink_cap_info[TCPM_PDO_MAX_SIZE] = {0};

	TYPEC_INFO("%s++", __func__);
	if (NULL == client)
		return;

	ret = tcpm_get_local_sink_cap(tcpc_dev, sink_cap_info);
	if (ret != TCPM_SUCCESS) {
		TYPEC_INFO("get local sink cap failed, ret = %d", ret);
		return;
	}

	for (i = 0; i < TCPM_PDO_MAX_SIZE; i++) {
		if (sink_cap_info[i].mv > set_voltage)
			continue;
		overload = (sink_cap_info[i].uw > max_uw)
			|| ((sink_cap_info[i].uw == max_uw) && (sink_cap_info[i].mv < vol_mv));
		if (overload) {
			max_uw = sink_cap_info[i].uw;
			vol_mv = sink_cap_info[i].mv;
			cur_ma = sink_cap_info[i].ma;
		}
	}

	if (!(vol_mv && cur_ma)) {
		TYPEC_INFO("setVoltage failed, vol or cur = 0");
		return;
	}
	ret = tcpm_request(tcpc_dev, vol_mv, cur_ma);
	if (ret != TCPM_SUCCESS) {
		TYPEC_INFO("setVoltage failed, ret = %d", ret);
	}
	TYPEC_INFO("%s--", __func__);
}

extern int tcpm_typec_notify_direct_charge(struct tcpc_device *tcpc_dev, bool dc);
static struct pd_dpm_ops tcpc_device_pd_dpm_ops = {
	.pd_dpm_hard_reset = tcpc_pd_dpm_hard_reset,
	.pd_dpm_get_hw_dock_svid_exist = tcpc_pd_dpm_get_hw_dock_svid_exist,
	.pd_dpm_notify_direct_charge_status = tcpm_typec_notify_direct_charge,
	.pd_dpm_set_cc_mode = rt1711h_set_cc_mode,
	.pd_dpm_set_voltage = tcpc_pd_dpm_set_voltage,
	.pd_dpm_get_cc_state = rt1711h_get_cc_state,
};
int tcpc_typec_init(struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	int ret = 0;

	if (typec_role >= TYPEC_ROLE_NR) {
		TYPEC_INFO("Wrong TypeC-Role: %d\r\n", typec_role);
		return -2;
	}

	TYPEC_INFO("pd_dpm_ops_register\n");
	pd_dpm_ops_register(&tcpc_device_pd_dpm_ops, tcpc_dev);
	TYPEC_INFO("typec_init: %s\r\n", typec_role_name[typec_role]);

	tcpc_dev->typec_role = typec_role;
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;
	tcpc_dev->typec_attach_old = TYPEC_UNATTACHED;

	tcpc_dev->typec_remote_cc[0] = TYPEC_CC_VOLT_OPEN;
	tcpc_dev->typec_remote_cc[1] = TYPEC_CC_VOLT_OPEN;

	tcpc_dev->wake_lock_pd = 0;
	tcpc_dev->wake_lock_user = true;
	tcpc_dev->typec_usb_sink_curr = CONFIG_TYPEC_SNK_CURR_DFT;

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	tcpc_dev->typec_legacy_cable = false;
	tcpc_dev->typec_legacy_cable_suspect = 0;
#endif	/* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

#ifdef CONFIG_TYPEC_CAP_NORP_SRC
        tcpc_dev->no_rpsrc_state = 0;
#endif

#ifdef CONFIG_TYPEC_CAP_POWER_OFF_CHARGE
	ret = typec_init_power_off_charge(tcpc_dev);
	if (ret != 0)
		return ret;
#endif	/* CONFIG_TYPEC_CAP_POWER_OFF_CHARGE */

#ifdef CONFIG_TYPEC_POWER_CTRL_INIT
	tcpc_dev->typec_power_ctrl = true;
#endif	/* CONFIG_TYPEC_POWER_CTRL_INIT */

	typec_unattached_entry(tcpc_dev);
	return ret;
}

#ifdef CONFIG_POGO_PIN

static bool typec_is_cc_open_state(struct tcpc_device *tcpc_dev)
{
	if ((tcpc_dev->typec_state == typec_errorrecovery) || (tcpc_dev->typec_state == typec_disabled))
		return true;

	return false;
}

static void typec_cc_open_entry(struct tcpc_device *tcpc_dev, uint8_t state)
{
	mutex_lock(&tcpc_dev->access_lock);
	TYPEC_NEW_STATE(state);
	tcpc_dev->typec_attach_new = TYPEC_UNATTACHED;
	mutex_unlock(&tcpc_dev->access_lock);

	tcpci_set_cc(tcpc_dev, TYPEC_CC_OPEN);
	typec_unattached_power_entry(tcpc_dev);

	tcpc_enable_timer(tcpc_dev, TYPEC_RT_TIMER_LEGACY);
}
static inline void typec_disable_entry(struct tcpc_device *tcpc_dev)
{
       typec_cc_open_entry(tcpc_dev, typec_disabled);
}
int tcpc_typec_disable(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_state != typec_disabled) {
		typec_disable_entry(tcpc_dev);
		TYPEC_DBG("%s\n", __func__);
	}

	return 0;
}
int tcpc_typec_enable(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_state == typec_disabled) {
		typec_unattached_entry(tcpc_dev);
		TYPEC_DBG("%s\n", __func__);
	}

	return 0;
}
#endif

void  tcpc_typec_deinit(struct tcpc_device *tcpc_dev)
{
}
