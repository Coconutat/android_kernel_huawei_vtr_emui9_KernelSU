/*
 * Copyright (C) 2016 Richtek Technology Corp.
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

#ifndef __LINUX_RT_TCPC_H
#define __LINUX_RT_TCPC_H

#include <linux/device.h>
#include <linux/hrtimer.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/err.h>
#include <linux/cpu.h>
#include <linux/delay.h>
#include <linux/hisi/contexthub/tca.h>

#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/usb/class-dual-role.h>
#endif /* CONFIG_DUAL_ROLE_USB_INTF */

#include <huawei_platform/usb/pd/richtek/tcpci_core.h>
#include <huawei_platform/usb/pd/richtek/tcpci_timer.h>

#ifdef CONFIG_PD_DBG_INFO
#include <huawei_platform/usb/pd/richtek/pd_dbg_info.h>
#endif /* CONFIG_PD_DBG_INFO */

#ifdef CONFIG_USB_POWER_DELIVERY
#include <huawei_platform/usb/pd/richtek/pd_core.h>
#endif /* CONFIG_USB_POWER_DELIVERY */

#define PE_STATE_FULL_NAME	0
static TCPC_MUX_CTRL_TYPE g_mux_type = TCPC_DP;

extern void pd_dpm_send_event(enum pd_dpm_cable_event_type event);

#ifdef CONFIG_CONTEXTHUB_PD
extern void dp_aux_switch_op(uint32_t value);
extern void dp_aux_uart_switch_enable(void);
extern int support_dp;
#endif

/* provide to TCPC interface */
extern int tcpci_report_usb_port_changed(struct tcpc_device *tcpc);
extern int tcpci_set_wake_lock(
	struct tcpc_device *tcpc, bool pd_lock, bool user_lock);
extern int tcpci_report_power_control(struct tcpc_device *tcpc, bool en);
extern int tcpc_typec_init(struct tcpc_device *tcpc, uint8_t typec_role);
extern void tcpc_typec_deinit(struct tcpc_device *tcpc);
extern int tcpc_dual_role_phy_init(struct tcpc_device *tcpc);

extern struct tcpc_device *tcpc_device_register(
		struct device *parent, struct tcpc_desc *tcpc_desc,
		struct tcpc_ops *ops, void *drv_data);
extern void tcpc_device_unregister(
			struct device *dev, struct tcpc_device *tcpc);

extern int tcpc_schedule_init_work(struct tcpc_device *tcpc);

extern void *tcpc_get_dev_data(struct tcpc_device *tcpc);
extern void tcpci_lock_typec(struct tcpc_device *tcpc);
extern void tcpci_unlock_typec(struct tcpc_device *tcpc);
extern int tcpci_alert(struct tcpc_device *tcpc);

extern void tcpci_vbus_level_init(
		struct tcpc_device *tcpc, uint16_t power_status);

extern void rt1711h_set_cc_mode(int mode);
extern int rt1711h_get_cc_state(void);
static inline int tcpci_check_vbus_valid(struct tcpc_device *tcpc)
{
	return tcpc->vbus_level >= TCPC_VBUS_VALID;
}

static inline int tcpci_check_vsafe0v(struct tcpc_device *tcpc, bool detect_en)
{
	int ret = 0;

#ifdef CONFIG_TCPC_VSAFE0V_DETECT_IC
	ret = (tcpc->vbus_level == TCPC_VBUS_SAFE0V);
#else
	ret = (tcpc->vbus_level == TCPC_VBUS_INVALID);
#endif

	return ret;
}

static inline int tcpci_alert_status_clear(
		struct tcpc_device *tcpc, uint32_t mask)
{
	return tcpc->ops->alert_status_clear(tcpc, mask);
}

static inline int tcpci_fault_status_clear(
	struct tcpc_device *tcpc, uint8_t status)
{
	if (tcpc->ops->fault_status_clear)
		return tcpc->ops->fault_status_clear(tcpc, status);
	return 0;
}

static inline int tcpci_get_alert_status(
		struct tcpc_device *tcpc, uint32_t *alert)
{
	return tcpc->ops->get_alert_status(tcpc, alert);
}

static inline int tcpci_get_fault_status(
		struct tcpc_device *tcpc, uint8_t *fault)
{
	if (tcpc->ops->get_fault_status)
		return tcpc->ops->get_fault_status(tcpc, fault);
	*fault = 0;
	return 0;
}

static inline int tcpci_get_power_status(
		struct tcpc_device *tcpc, uint16_t *pw_status)
{
	return tcpc->ops->get_power_status(tcpc, pw_status);
}

static inline int tcpci_init(struct tcpc_device *tcpc, bool sw_reset)
{
	int ret;
	uint16_t power_status;

	ret = tcpc->ops->init(tcpc, sw_reset);
	if (ret)
		return ret;

	ret = tcpci_get_power_status(tcpc, &power_status);
	if (ret)
		return ret;

	tcpci_vbus_level_init(tcpc, power_status);
	return 0;
}

static inline int tcpci_get_cc(struct tcpc_device *tcpc)
{
	int ret, cc1, cc2;

	ret = tcpc->ops->get_cc(tcpc, &cc1, &cc2);
	if (ret < 0)
		return ret;

	if ((cc1 == tcpc->typec_remote_cc[0]) &&
			(cc2 == tcpc->typec_remote_cc[1])) {
		return 0;
	}

	tcpc->typec_remote_cc[0] = cc1;
	tcpc->typec_remote_cc[1] = cc2;

	return 1;
}

static inline int tcpci_set_cc(struct tcpc_device *tcpc, int pull)
{
#ifdef CONFIG_USB_PD_DBG_ALWAYS_LOCAL_RP
	if (pull == TYPEC_CC_RP)
		pull = tcpc->typec_local_rp_level;
#endif /* CONFIG_USB_PD_DBG_ALWAYS_LOCAL_RP */

#ifdef CONFIG_TYPEC_CHECK_LEGACY_CABLE
	if (pull == TYPEC_CC_DRP && tcpc->typec_legacy_cable) {
		pull = TYPEC_CC_RP_1_5;
		TCPC_DBG("LC->Toggling\r\n");
	}
#endif /* CONFIG_TYPEC_CHECK_LEGACY_CABLE */

	if (pull & TYPEC_CC_DRP) {
		tcpc->typec_remote_cc[0] =
		tcpc->typec_remote_cc[1] =
			TYPEC_CC_DRP_TOGGLING;
	}

	if (pull == TYPEC_CC_DRP_DFT)
		pull = TYPEC_CC_DRP_1_5;

	tcpc->typec_local_cc = pull;
	return tcpc->ops->set_cc(tcpc, pull);
}

static inline int tcpci_set_polarity(struct tcpc_device *tcpc, int polarity)
{
	return tcpc->ops->set_polarity(tcpc, polarity);
}

static inline int tcpci_set_vconn(struct tcpc_device *tcpc, int enable)
{
	struct tcp_notify tcp_noti;

	tcp_noti.en_state.en = enable != 0;
	srcu_notifier_call_chain(&tcpc->evt_nh,
				TCP_NOTIFY_SOURCE_VCONN, &tcp_noti);

	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VCONN, &enable);
	return tcpc->ops->set_vconn(tcpc, enable);
}

static inline int tcpci_mask_vsafe0v(struct tcpc_device *tcpc, int enable)
{
	return tcpc->ops->mask_vsafe0v(tcpc, enable);
}

static inline int tcpci_is_low_power_mode(struct tcpc_device *tcpc)
{
	int rv = 1;

#ifdef CONFIG_TCPC_LOW_POWER_MODE
	if (tcpc->ops->is_low_power_mode)
		rv = tcpc->ops->is_low_power_mode(tcpc);
#endif	/* CONFIG_TCPC_LOW_POWER_MODE */

	return rv;
}

static inline int tcpci_set_low_power_mode(
	struct tcpc_device *tcpc, bool en, int pull)
{
	int rv = 0;

#ifdef CONFIG_TCPC_LOW_POWER_MODE
	rv = tcpc->ops->set_low_power_mode(tcpc, en, pull);
#endif

#ifdef CONFIG_TYPEC_CAP_A2C_C2C
	if (en) {
		TYPEC_DBG("A2C_C2C: reset1\r\n");
		tcpc->typec_a2c_cable = false;
		tcpc_disable_timer(tcpc, TYPEC_RT_TIMER_A2C_C2C);
	}
#endif	/* CONFIG_TYPEC_CAP_A2C_C2C */

	return rv;
}

static inline int tcpci_idle_poll_ctrl(
		struct tcpc_device *tcpc, bool en, bool lock)
{
	int rv = 0;
#ifdef CONFIG_TCPC_IDLE_MODE
	bool update_mode = false;

	if (lock)
		mutex_lock(&tcpc->access_lock);

	if (en) {
		if (tcpc->tcpc_busy_cnt == 0)
			update_mode = true;
		tcpc->tcpc_busy_cnt++;
	} else {	/* idle mode */
		if (tcpc->tcpc_busy_cnt <= 0)
			TCPC_DBG("tcpc_busy_cnt<=0\r\n");
		else
			tcpc->tcpc_busy_cnt--;
		if (tcpc->tcpc_busy_cnt == 0)
			update_mode = true;
	}
	if (lock)
		mutex_unlock(&tcpc->access_lock);
	if (update_mode)
		rv = tcpc->ops->set_idle_mode(tcpc, !en);
#endif
	return rv;
}

static inline int tcpci_set_watchdog(struct tcpc_device *tcpc, bool en)
{
	int rv = 0;

#ifdef CONFIG_TCPC_WATCHDOG_EN
	rv = tcpc->ops->set_watchdog(tcpc, en);
#endif

	return rv;
}

#ifdef CONFIG_USB_POWER_DELIVERY

static inline int tcpci_set_msg_header(
	struct tcpc_device *tcpc, int power_role, int data_role)
{
	return tcpc->ops->set_msg_header(tcpc, power_role, data_role);
}

static inline int tcpci_set_rx_enable(struct tcpc_device *tcpc, uint8_t enable)
{
	return tcpc->ops->set_rx_enable(tcpc, enable);
}

static inline int tcpci_get_message(struct tcpc_device *tcpc,
	uint32_t *payload, uint16_t *head, enum tcpm_transmit_type *type)
{
	return tcpc->ops->get_message(tcpc, payload, head, type);
}

static inline int tcpci_transmit(struct tcpc_device *tcpc,
	enum tcpm_transmit_type type, uint16_t header, const uint32_t *data)
{
	return tcpc->ops->transmit(tcpc, type, header, data);
}

static inline int tcpci_set_bist_test_mode(struct tcpc_device *tcpc, bool en)
{
	return tcpc->ops->set_bist_test_mode(tcpc, en);
}

static inline int tcpci_set_bist_carrier_mode(
		struct tcpc_device *tcpc, uint8_t pattern)
{
	if (pattern)	/* wait for GoodCRC */
		udelay(240);

	return tcpc->ops->set_bist_carrier_mode(tcpc, pattern);
}

#ifdef CONFIG_USB_PD_RETRY_CRC_DISCARD
static inline int tcpci_retransmit(struct tcpc_device *tcpc)
{
	return tcpc->ops->retransmit(tcpc);
}
#endif	/* CONFIG_USB_PD_RETRY_CRC_DISCARD */
#endif	/* CONFIG_USB_POWER_DELIVERY */

static inline int tcpci_notify_typec_state(
	struct tcpc_device *tcpc)
{
	struct pd_dpm_typec_state typec_state;
	memset(&typec_state, 0, sizeof(typec_state));
	typec_state.polarity = tcpc->typec_polarity;
	typec_state.old_state = tcpc->typec_attach_old;
	typec_state.new_state = tcpc->typec_attach_new;

#ifdef CONFIG_TYPEC_CAP_A2C_C2C
	if (tcpc->typec_attach_new == TYPEC_ATTACHED_SNK || tcpc->typec_attach_new == TYPEC_ATTACHED_CUSTOM_SRC) {
		if (tcpc->typec_a2c_cable)
			TYPEC_INFO("** A2C_CABLE\r\n");
		else
			TYPEC_INFO("** C2C_CABLE\r\n");
	}
#endif	/* CONFIG_TYPEC_CAP_A2C_C2C */

	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_TYPEC_STATE, &typec_state);
	return 0;
}

static inline int tcpci_notify_role_swap(
	struct tcpc_device *tcpc, uint8_t event, uint8_t role)
{
#if 1
	uint8_t dpm_event;
	struct pd_dpm_swap_state swap_state;

	switch (event) {
	case TCP_NOTIFY_DR_SWAP:
		dpm_event = PD_DPM_PE_EVT_DR_SWAP;
		break;
	case TCP_NOTIFY_PR_SWAP:
		dpm_event = PD_DPM_PE_EVT_PR_SWAP;
		break;
	case TCP_NOTIFY_VCONN_SWAP:
		dpm_event = PD_DPM_PE_EVT_VCONN_SWAP;
		break;
	default:
		return 0;
	}

	swap_state.new_role = role;
	return pd_dpm_handle_pe_event(event, &swap_state);
#else
	return 0;
#endif
}

static inline int tcpci_notify_pd_state(
	struct tcpc_device *tcpc, uint8_t connect)
{
	struct pd_dpm_pd_state pd_state;

	pd_state.connected = connect;

	return pd_dpm_handle_pe_event(
		PD_DPM_PE_EVT_PD_STATE, &pd_state);
}

static inline int tcpci_enable_watchdog(struct tcpc_device *tcpc, bool en)
{
	TCPC_DBG("enable_WG: %d\r\n", en);

#ifdef CONFIG_TCPC_WATCHDOG_EN
	if (tcpc->ops->set_watchdog)
		tcpc->ops->set_watchdog(tcpc, en);
#endif	/* CONFIG_TCPC_WATCHDOG_EN */

#ifdef CONFIG_TCPC_INTRST_EN
	if (tcpc->ops->set_intrst)
		tcpc->ops->set_intrst(tcpc, en);
#endif	/* CONFIG_TCPC_INTRST_EN */

	return 0;
}

static inline int tcpci_source_vbus(
	struct tcpc_device *tcpc, uint8_t type, int mv, int ma)
{
	struct pd_dpm_vbus_state vbus_state;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (type >= TCP_VBUS_CTRL_PD && tcpc->pd_port.pd_prev_connected)
		type |= TCP_VBUS_CTRL_PD_DETECT;
#endif	/* CONFIG_USB_POWER_DELIVERY */

	if (ma < 0) {
		if (mv != 0) {
			switch (tcpc->typec_local_rp_level) {
			case TYPEC_CC_RP_1_5:
				ma = 1500;
				break;
			case TYPEC_CC_RP_3_0:
				ma = 3000;
				break;
			default:
			case TYPEC_CC_RP_DFT:
				ma = CONFIG_TYPEC_SRC_CURR_DFT;
				break;
			}
		} else
			ma = 0;
	}

	vbus_state.ma = ma;
	vbus_state.mv = mv;
	vbus_state.vbus_type = type;

	tcpci_enable_watchdog(tcpc, mv != 0);
	TCPC_DBG("source_vbus: %d mV, %d mA\r\n", mv, ma);
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SOURCE_VBUS, &vbus_state);
	return 0;
}

static inline int tcpci_sink_vbus(
	struct tcpc_device *tcpc, uint8_t type, int mv, int ma)
{
	struct pd_dpm_vbus_state vbus_state;
	uint32_t dpm_flag = 0;

#ifdef CONFIG_USB_POWER_DELIVERY
	if (type >= TCP_VBUS_CTRL_PD && tcpc->pd_port.pd_prev_connected)
	{
		type |= TCP_VBUS_CTRL_PD_DETECT;
		dpm_flag = tcpm_inquire_dpm_flags(tcpc);
		vbus_state.ext_power = dpm_flag & DPM_FLAGS_PARTNER_EXTPOWER;
	}
#endif	/* CONFIG_USB_POWER_DELIVERY */

#ifdef CONFIG_TYPEC_CAP_A2C_C2C
	if (!(type & TCP_VBUS_CTRL_PD_DETECT)) {
		if (tcpc->typec_a2c_cable) {
			TCPC_INFO("** A2C_CABLE\r\n");
		}
	}
#endif	/* CONFIG_TYPEC_CAP_A2C_C2C */

	if (ma < 0) {
		if (mv != 0) {
			vbus_state.remote_rp_level = tcpc->typec_remote_rp_level;
			switch (tcpc->typec_remote_rp_level) {
			case TYPEC_CC_VOLT_SNK_1_5:
				ma = 1500;
				break;
			case TYPEC_CC_VOLT_SNK_3_0:
				ma = 3000;
				break;
			default:
			case TYPEC_CC_VOLT_SNK_DFT:
				ma = tcpc->typec_usb_sink_curr;
				break;
			}
#if CONFIG_TYPEC_SNK_CURR_LIMIT > 0
		if (ma > CONFIG_TYPEC_SNK_CURR_LIMIT)
			ma = CONFIG_TYPEC_SNK_CURR_LIMIT;
#endif	/* CONFIG_TYPEC_SNK_CURR_LIMIT */
		} else
			ma = 0;
	}

	vbus_state.ma = ma;
	vbus_state.mv = mv;
	vbus_state.vbus_type = type;

	TCPC_DBG("sink_vbus: %d mV, %d mA\r\n", mv, ma);
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_SINK_VBUS, &vbus_state);
	return 0;
}

static inline int tcpci_disable_vbus_control(struct tcpc_device *tcpc)
{
#ifdef CONFIG_TYPEC_USE_DIS_VBUS_CTRL
	TCPC_DBG("disable_vbus\r\n");
	tcpci_enable_watchdog(tcpc, false);
	pd_dpm_handle_pe_event(PD_DPM_PE_EVT_DIS_VBUS_CTRL, NULL);
	return 0;
#else
	tcpci_sink_vbus(tcpc, TCP_VBUS_CTRL_REMOVE, TCPC_VBUS_SINK_0V, 0);
	tcpci_source_vbus(tcpc, TCP_VBUS_CTRL_REMOVE, TCPC_VBUS_SOURCE_0V, 0);
	return 0;
#endif
}

#ifdef CONFIG_USB_POWER_DELIVERY

static inline int tcpci_enter_mode(struct tcpc_device *tcpc,
	uint16_t svid, uint8_t ops, uint32_t mode)
{
	/* DFP_U : DisplayPort Mode, USB Configuration */
	TCPC_DBG("EnterMode\r\n");
	return 0;
}

static inline int tcpci_exit_mode(
	struct tcpc_device *tcpc, uint16_t svid)
{
	TCPC_DBG("ExitMode\r\n");
	return 0;
}

#ifdef CONFIG_USB_PD_ALT_MODE

static inline int tcpci_report_hpd_state(
		struct tcpc_device *tcpc, uint32_t dp_status)
{
	struct tcp_notify tcp_noti;
	int ret = 0;

	/* UFP_D to DFP_D only */

	if (PD_DP_CFG_DFP_D(tcpc->pd_port.local_dp_config)) {
		tcp_noti.ama_dp_hpd_state.irq = PD_VDO_DPSTS_HPD_IRQ(dp_status);
		tcp_noti.ama_dp_hpd_state.state =
					PD_VDO_DPSTS_HPD_LVL(dp_status);
		DP_INFO("+++ hpd_state: %d +++\r\n\r\n",
			tcp_noti.ama_dp_hpd_state.state);
		srcu_notifier_call_chain(&tcpc->evt_nh,
			TCP_NOTIFY_AMA_DP_HPD_STATE, &tcp_noti);

#ifdef CONFIG_CONTEXTHUB_PD
		if (!support_dp) {
			return 0;
		}
		struct pd_dpm_combphy_event event;
		event.dev_type = TCA_DP_IN;
		event.irq_type = TCA_IRQ_HPD_IN;
		event.mode_type = g_mux_type;
		event.typec_orien = tcpc->typec_polarity;

		pr_info("\n%s + state = %d,irq = %d,dp_status = %d\n", __func__,tcp_noti.ama_dp_hpd_state.state ,tcp_noti.ama_dp_hpd_state.irq, dp_status);

		if (!tcp_noti.ama_dp_hpd_state.state) {
			event.dev_type = TCA_DP_OUT;
			event.irq_type = TCA_IRQ_HPD_OUT;
			ret = pd_dpm_handle_combphy_event(event);
			pd_dpm_set_last_hpd_status(false);

			pd_dpm_send_event(DP_CABLE_OUT_EVENT);
		}
		else
		{
			event.dev_type = TCA_DP_IN;
			ret = pd_dpm_handle_combphy_event(event);
			pd_dpm_set_last_hpd_status(true);

			pd_dpm_send_event(DP_CABLE_IN_EVENT);
		}

		if (tcp_noti.ama_dp_hpd_state.irq) {
			event.irq_type = TCA_IRQ_SHORT;
			ret = pd_dpm_handle_combphy_event(event);
		}

		pr_info("\n%s - ret = %d\n", __func__, ret);
#endif
	}

	return 0;
}

static inline int tcpci_dp_status_update(
	struct tcpc_device *tcpc, uint32_t dp_status)
{
	DP_INFO("Status0: 0x%x\r\n", dp_status);
	tcpci_report_hpd_state(tcpc, dp_status);
	return 0;
}

static inline int tcpci_dp_configure(
	struct tcpc_device *tcpc, uint32_t dp_config)
{
	struct tcp_notify tcp_noti;
	int ret = 0;

	DP_INFO("LocalCFG: 0x%x\r\n", dp_config);

	switch (dp_config & 0x03) {
	case 0:
		tcp_noti.ama_dp_state.sel_config = SW_USB;
		break;
	case MODE_DP_SRC:
		tcp_noti.ama_dp_state.sel_config = SW_UFP_D;
		tcp_noti.ama_dp_state.pin_assignment = (dp_config >> 16) & 0xff;
		break;
	case MODE_DP_SNK:
		tcp_noti.ama_dp_state.sel_config = SW_DFP_D;
		tcp_noti.ama_dp_state.pin_assignment = (dp_config >> 8) & 0xff;
		break;
	}

	tcp_noti.ama_dp_state.signal = (dp_config >> 2) & 0x0f;
	tcp_noti.ama_dp_state.polarity = tcpc->typec_polarity;
	tcp_noti.ama_dp_state.active = 1;
#ifdef CONFIG_CONTEXTHUB_PD
	if (!support_dp) {
		return srcu_notifier_call_chain(&tcpc->evt_nh,
				TCP_NOTIFY_AMA_DP_STATE, &tcp_noti);
	}
	/* add aux switch */
	dp_aux_switch_op(tcp_noti.ama_dp_state.polarity);
	/* add aux uart switch*/
	dp_aux_uart_switch_enable();

	if(MODE_DP_PIN_C == tcp_noti.ama_dp_state.pin_assignment || MODE_DP_PIN_E == tcp_noti.ama_dp_state.pin_assignment)
		g_mux_type = TCPC_DP;
	else if(MODE_DP_PIN_D == tcp_noti.ama_dp_state.pin_assignment || MODE_DP_PIN_F == tcp_noti.ama_dp_state.pin_assignment)
		g_mux_type = TCPC_USB31_AND_DP_2LINE;

	struct pd_dpm_combphy_event event;
	event.dev_type = TCA_ID_RISE_EVENT;
	event.irq_type = TCA_IRQ_HPD_OUT;
	event.mode_type = TCPC_NC;
	event.typec_orien = tcpc->typec_polarity;

	ret = pd_dpm_handle_combphy_event(event);
	pd_dpm_set_combphy_status(g_mux_type);

	event.dev_type = TCA_ID_FALL_EVENT;
	event.irq_type = TCA_IRQ_HPD_IN;
	event.mode_type = g_mux_type;
	event.typec_orien = tcpc->typec_polarity;
	ret = pd_dpm_handle_combphy_event(event);

	pr_info("\nhuawei_pd %s pd_event_notify ret = %d, mux_type = %d\n", __func__, ret, g_mux_type);
#endif
	return srcu_notifier_call_chain(&tcpc->evt_nh,
				TCP_NOTIFY_AMA_DP_STATE, &tcp_noti);
}


static inline int tcpci_dp_attention(
	struct tcpc_device *tcpc, uint32_t dp_status)
{
	/* DFP_U : Not call this function during internal flow */
	struct tcp_notify tcp_noti;

	DP_INFO("Attention: 0x%x\r\n", dp_status);
	tcp_noti.ama_dp_attention.state = (uint8_t) dp_status;
	srcu_notifier_call_chain(&tcpc->evt_nh,
		TCP_NOTIFY_AMA_DP_ATTENTION, &tcp_noti);
	return tcpci_report_hpd_state(tcpc, dp_status);
}

static inline int tcpci_dp_notify_status_update_done(
	struct tcpc_device *tcpc, uint32_t dp_status, bool ack)
{
	/* DFP_U : Not call this function during internal flow */
	DP_INFO("Status1: 0x%x, ack=%d\r\n", dp_status, ack);
	return 0;
}

static inline int tcpci_dp_notify_config_start(struct tcpc_device *tcpc)
{
	/* DFP_U : Put signal & mux into the Safe State */
	struct tcp_notify tcp_noti;

	DP_INFO("ConfigStart\r\n");
	tcp_noti.ama_dp_state.sel_config = SW_USB;
	tcp_noti.ama_dp_state.active = 0;
	srcu_notifier_call_chain(&tcpc->evt_nh,
		TCP_NOTIFY_AMA_DP_STATE, &tcp_noti);
	return 0;
}

static inline int tcpci_dp_notify_config_done(struct tcpc_device *tcpc,
	uint32_t local_cfg, uint32_t remote_cfg, bool ack)
{
	/* DFP_U : If DP success,
	 * internal flow will enter this function finally */
	DP_INFO("ConfigDone, L:0x%x, R:0x%x, ack=%d\r\n",
		local_cfg, remote_cfg, ack);

	if (ack)
		tcpci_dp_configure(tcpc, local_cfg);

	return 0;
}

#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
static inline int tcpci_notify_uvdm(struct tcpc_device *tcpc, bool ack)
{
	struct tcp_notify tcp_noti;
	pd_port_t *pd_port = &tcpc->pd_port;

	tcp_noti.uvdm_msg.ack = ack;

	if (ack) {
		tcp_noti.uvdm_msg.uvdm_cnt = pd_port->uvdm_cnt;
		tcp_noti.uvdm_msg.uvdm_svid = pd_port->uvdm_svid;
		tcp_noti.uvdm_msg.uvdm_data = pd_port->uvdm_data;
	}

	srcu_notifier_call_chain(&tcpc->evt_nh,
		TCP_NOTIFY_UVDM, &tcp_noti);

	return 0;
}
#endif	/* CONFIG_USB_PD_UVDM */

#ifdef CONFIG_USB_PD_ALT_MODE_RTDC
static inline int tcpci_dc_notify_en_unlock(struct tcpc_device *tcpc)
{
	struct tcp_notify tcp_noti;

	DC_INFO("DirectCharge en_unlock\r\n");
	return srcu_notifier_call_chain(&tcpc->evt_nh,
		TCP_NOTIFY_DC_EN_UNLOCK, &tcp_noti);
}
#endif	/* CONFIG_USB_PD_ALT_MODE_RTDC */

#endif	/* CONFIG_USB_POWER_DELIVERY */

#endif /* #ifndef __LINUX_RT_TCPC_H */
