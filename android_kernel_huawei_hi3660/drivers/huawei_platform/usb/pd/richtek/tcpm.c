/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * Power Delivery Managert Driver
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

#include <huawei_platform/usb/pd/richtek/tcpm.h>
#include <huawei_platform/usb/pd/richtek/pd_core.h>
#include <huawei_platform/usb/pd/richtek/tcpci.h>
#include <huawei_platform/usb/pd/richtek/pd_policy_engine.h>
#include <huawei_platform/usb/pd/richtek/pd_dpm_core.h>
#include <huawei_platform/usb/pd/richtek/tcpci_typec.h>

/* Inquire TCPC status */

int tcpm_shutdown(struct tcpc_device *tcpc_dev)
{
#ifdef CONFIG_TCPC_SHUTDOWN_VBUS_DISABLE
	if (tcpc_dev->typec_power_ctrl)
		tcpci_disable_vbus_control(tcpc_dev);
#endif	/* CONFIG_TCPC_SHUTDOWN_VBUS_DISABLE */

	if (tcpc_dev->ops->deinit && tcpc_dev->pd_port.pd_connected)
		tcpc_dev->ops->deinit(tcpc_dev);

	return 0;
}

int tcpm_inquire_remote_cc(struct tcpc_device *tcpc_dev,
	uint8_t *cc1, uint8_t *cc2, bool from_ic)
{
	int rv = 0;

	if (from_ic) {
		rv = tcpci_get_cc(tcpc_dev);
		if (rv < 0)
			return rv;
	}

	*cc1 = tcpc_dev->typec_remote_cc[0];
	*cc2 = tcpc_dev->typec_remote_cc[1];
	return 0;
}

int tcpm_inquire_vbus_level(
	struct tcpc_device *tcpc_dev, bool from_ic)
{
	int rv = 0;
	uint16_t power_status = 0;

	if (from_ic) {
		rv = tcpci_get_power_status(tcpc_dev, &power_status);
		if (rv < 0)
			return rv;

		tcpci_vbus_level_init(tcpc_dev, power_status);
	}

	return tcpc_dev->vbus_level;
}

bool tcpm_inquire_cc_polarity(
	struct tcpc_device *tcpc_dev)
{
	return tcpc_dev->typec_polarity;
}

uint8_t tcpm_inquire_typec_attach_state(
	struct tcpc_device *tcpc_dev)
{
	return tcpc_dev->typec_attach_new;
}

uint8_t tcpm_inquire_typec_role(
	struct tcpc_device *tcpc_dev)
{
	return tcpc_dev->typec_role;
}

uint8_t tcpm_inquire_typec_local_rp(
	struct tcpc_device *tcpc_dev)
{
	uint8_t level;

	switch (tcpc_dev->typec_local_rp_level) {
	case TYPEC_CC_RP_1_5:
		level = 1;
		break;

	case TYPEC_CC_RP_3_0:
		level = 2;
		break;

	default:
	case TYPEC_CC_RP_DFT:
		level = 0;
		break;
	}

	return level;
}

uint8_t tcpm_inquire_request_result(
	struct tcpc_device *tcpc_dev)
{
#if 0
	uint8_t req_state;

	pd_port_t *pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);
	req_state = pd_port->dpm_req_state;
	pd_port->dpm_req_state = DPM_REQ_NULL;
	mutex_unlock(&pd_port->pd_lock);

	return req_state;
#endif
	/* TODO */
	return 0;
}

int tcpm_typec_set_wake_lock(
	struct tcpc_device *tcpc, bool user_lock)
{
	int ret;

	mutex_lock(&tcpc->access_lock);
	ret = tcpci_set_wake_lock(
		tcpc, tcpc->wake_lock_pd, user_lock);
	tcpc->wake_lock_user = user_lock;
	mutex_unlock(&tcpc->access_lock);

	return ret;
}

int tcpm_typec_set_usb_sink_curr(
	struct tcpc_device *tcpc_dev, int curr)
{
	bool force_sink_vbus = true;

#ifdef CONFIG_USB_POWER_DELIVERY
	pd_port_t *pd_port = &tcpc_dev->pd_port;
	mutex_lock(&pd_port->pd_lock);

	if (pd_port->pd_prev_connected)
		force_sink_vbus = false;
#endif	/* CONFIG_USB_POWER_DELIVERY */

	tcpc_dev->typec_usb_sink_curr = curr;

	if (tcpc_dev->typec_remote_rp_level != TYPEC_CC_VOLT_SNK_DFT)
		force_sink_vbus = false;

	if (force_sink_vbus) {
		tcpci_sink_vbus(tcpc_dev,
			TCP_VBUS_CTRL_TYPEC, TCPC_VBUS_SINK_5V, -1);
	}

#ifdef CONFIG_USB_POWER_DELIVERY
	mutex_unlock(&pd_port->pd_lock);
#endif	/* CONFIG_USB_POWER_DELIVERY */

	return 0;
}

int tcpm_typec_set_rp_level(
	struct tcpc_device *tcpc_dev, uint8_t level)
{
	uint8_t res;

	if (level == 2)
		res = TYPEC_CC_RP_3_0;
	else if (level == 1)
		res = TYPEC_CC_RP_1_5;
	else
		res = TYPEC_CC_RP_DFT;

	return tcpc_typec_set_rp_level(tcpc_dev, res);
}

int tcpm_typec_role_swap(struct tcpc_device *tcpc_dev)
{
	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

#ifdef CONFIG_TYPEC_CAP_ROLE_SWAP
	if (tcpc_typec_swap_role(tcpc_dev) == 0)
		return TCPM_SUCCESS;
#endif /* CONFIG_TYPEC_CAP_ROLE_SWAP */

	return TCPM_ERROR_UNSUPPORT;
}

int tcpm_typec_notify_direct_charge(struct tcpc_device *tcpc_dev, bool dc)
{
	TCPC_INFO("%s,%d", __func__, __LINE__);
	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	TCPC_INFO("%s,%d", __func__, __LINE__);
	return tcpc_typec_notify_direct_charge(tcpc_dev, dc);
}
int tcpm_typec_change_role(
	struct tcpc_device *tcpc_dev, uint8_t typec_role)
{
	return tcpc_typec_change_role(tcpc_dev, typec_role);
}

#ifdef CONFIG_USB_POWER_DELIVERY
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
bool tcpm_inquire_cust_src2_cable_vdo(
        struct tcpc_device *tcpc_dev, uint32_t *vdos, int size)
{
	if(!tcpc_dev){
		return false;
	}
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (size < VDO_MAX_SIZE){
		return false;
	}
	mutex_lock(&pd_port->pd_lock);
	memcpy(vdos, pd_port->cable_vdos,
		sizeof(uint32_t) * VDO_MAX_SIZE);
	mutex_unlock(&pd_port->pd_lock);
	return true;
}
#endif /* CONFIG_TYPEC_CAP_CUSTOM_SRC2 */

bool tcpm_inquire_pd_connected(
	struct tcpc_device *tcpc_dev)
{
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->pd_connected;
}

bool tcpm_inquire_pd_prev_connected(
	struct tcpc_device *tcpc_dev)
{
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->pd_prev_connected;
}

uint8_t tcpm_inquire_pd_data_role(
	struct tcpc_device *tcpc_dev)
{
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->data_role;
}

uint8_t tcpm_inquire_pd_power_role(
	struct tcpc_device *tcpc_dev)
{
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->power_role;
}

uint8_t tcpm_inquire_pd_vconn_role(
	struct tcpc_device *tcpc_dev)
{
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	return pd_port->vconn_source;
}

uint8_t tcpm_inquire_cable_current(
	struct tcpc_device *tcpc_dev)
{
	pd_port_t* pd_port = &tcpc_dev->pd_port;
	if (pd_port->power_cable_present) {
		return PD_VDO_CABLE_CURR(
			pd_port->cable_vdos[VDO_INDEX_CABLE])+1;
	}

	return PD_CABLE_CURR_UNKNOWN;
}

uint32_t tcpm_inquire_dpm_flags(struct tcpc_device *tcpc_dev)
{
	pd_port_t* pd_port = &tcpc_dev->pd_port;

	return pd_port->dpm_flags;
}

uint32_t tcpm_inquire_dpm_caps(struct tcpc_device *tcpc_dev)
{
	pd_port_t* pd_port = &tcpc_dev->pd_port;

	return pd_port->dpm_caps;
}

void tcpm_set_dpm_flags(struct tcpc_device *tcpc_dev, uint32_t flags)
{
	pd_port_t* pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);
	pd_port->dpm_flags = flags;
	mutex_unlock(&pd_port->pd_lock);
	return;
}

void tcpm_set_dpm_caps(struct tcpc_device *tcpc_dev, uint32_t caps)
{
	pd_port_t* pd_port = &tcpc_dev->pd_port;

	mutex_lock(&pd_port->pd_lock);
	pd_port->dpm_caps = caps;
	mutex_unlock(&pd_port->pd_lock);
	return;
}

#endif	/* CONFIG_USB_POWER_DELIVERY */

/* Request TCPC to send PD Request */

#ifdef CONFIG_USB_POWER_DELIVERY

int tcpm_power_role_swap(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected) {
#ifdef CONFIG_USB_PD_AUTO_TYPEC_ROLE_SWAP
		return tcpm_typec_role_swap(tcpc_dev);
#else
		return TCPM_ERROR_NO_PD_CONNECTED;
#endif	/* CONFIG_USB_PD_AUTO_TYPEC_ROLE_SWAP */
	}

	ret = pd_put_dpm_pd_request_event(pd_port,
			PD_DPM_PD_REQUEST_PR_SWAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_power_role_swap);

int tcpm_data_role_swap(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected) {
#ifdef CONFIG_USB_PD_AUTO_TYPEC_ROLE_SWAP
		return tcpm_typec_role_swap(tcpc_dev);
#else
		return TCPM_ERROR_NO_PD_CONNECTED;
#endif	/* CONFIG_USB_PD_AUTO_TYPEC_ROLE_SWAP */
	}

	ret = pd_put_dpm_pd_request_event(pd_port,
			PD_DPM_PD_REQUEST_DR_SWAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_data_role_swap);

int tcpm_vconn_swap(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	ret = pd_put_dpm_pd_request_event(pd_port,
			PD_DPM_PD_REQUEST_VCONN_SWAP);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_vconn_swap);

int tcpm_goto_min(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	ret = pd_put_dpm_pd_request_event(pd_port,
		PD_DPM_PD_REQUEST_GOTOMIN);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_goto_min);

int tcpm_soft_reset(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	ret = pd_put_dpm_pd_request_event(pd_port,
			PD_DPM_PD_REQUEST_SOFTRESET);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_soft_reset);

int tcpm_hard_reset(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	ret = pd_put_dpm_pd_request_event(pd_port,
			PD_DPM_PD_REQUEST_HARDRESET);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_hard_reset);

int tcpm_get_source_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	if (cap == NULL) {
		ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_GET_SOURCE_CAP);
		if (!ret)
			return TCPM_ERROR_PUT_EVENT;
	} else {
		mutex_lock(&pd_port->pd_lock);
		cap->cnt = pd_port->remote_src_cap.nr;
		memcpy(cap->pdos, pd_port->remote_src_cap.pdos,
			sizeof(uint32_t) * cap->cnt);
		mutex_unlock(&pd_port->pd_lock);
	}

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_get_source_cap);

int tcpm_get_sink_cap(
	struct tcpc_device *tcpc_dev, struct tcpm_power_cap *cap)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	if (cap == NULL) {
		ret = pd_put_dpm_pd_request_event(pd_port,
			PD_DPM_PD_REQUEST_GET_SINK_CAP);
		if (!ret)
			return TCPM_ERROR_PUT_EVENT;
	} else {
		mutex_lock(&pd_port->pd_lock);
		cap->cnt = pd_port->remote_snk_cap.nr;
		memcpy(cap->pdos, pd_port->remote_snk_cap.pdos,
			sizeof(uint32_t) * cap->cnt);
		mutex_unlock(&pd_port->pd_lock);
	}

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_get_sink_cap);

int tcpm_get_local_sink_cap(
	struct tcpc_device *tcpc_dev, struct local_sink_cap *cap)
{
	int i;
	pd_port_t *pd_port = &tcpc_dev->pd_port;
	pd_port_power_caps *snk_cap = &pd_port->local_snk_cap;
	struct dpm_pdo_info_t sink;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	for(i = 0; i < snk_cap->nr; i++) {
		dpm_extract_pdo_info(snk_cap->pdos[i], &sink);
		cap[i].mv = sink.vmin;
		cap[i].ma = sink.ma;
		cap[i].uw = sink.uw;
	}

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_get_local_sink_cap);

int tcpm_bist_cm2(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	ret = pd_put_dpm_pd_request_event(pd_port,
				PD_DPM_PD_REQUEST_BIST_CM2);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_bist_cm2);

int tcpm_request(struct tcpc_device *tcpc_dev, int mv, int ma)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	mutex_lock(&pd_port->pd_lock);
	ret = pd_dpm_send_request(pd_port, mv, ma);
	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_request);

int tcpm_error_recovery(struct tcpc_device *tcpc_dev)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	ret = pd_put_dpm_event(pd_port, PD_DPM_ERROR_RECOVERY);
	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}

int tcpm_discover_cable(struct tcpc_device *tcpc_dev, uint32_t *vdos)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	if (vdos == NULL) {
		mutex_lock(&pd_port->pd_lock);
		pd_port->dpm_flags |= DPM_FLAGS_CHECK_CABLE_ID;
		ret = vdm_put_dpm_discover_cable_event(pd_port);
		mutex_unlock(&pd_port->pd_lock);

		if (!ret)
			return TCPM_ERROR_PUT_EVENT;
	} else {
		if (!pd_port->power_cable_present)
			return TCPM_ERROR_NO_POWER_CABLE;

		mutex_lock(&pd_port->pd_lock);
		memcpy(vdos, pd_port->cable_vdos,
			sizeof(uint32_t) * VDO_MAX_SIZE);
		mutex_unlock(&pd_port->pd_lock);
	}

	return TCPM_SUCCESS;
}

int tcpm_vdm_request_id(struct tcpc_device *tcpc_dev,
				uint8_t *cnt, uint32_t *vdos)
{
	bool ret;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	if (cnt == NULL || vdos == NULL) {
		mutex_lock(&pd_port->pd_lock);
		ret = vdm_put_dpm_vdm_request_event(
			pd_port, PD_DPM_VDM_REQUEST_DISCOVER_ID);
		mutex_unlock(&pd_port->pd_lock);

		if (!ret)
			return TCPM_ERROR_PUT_EVENT;
	} else {
		mutex_lock(&pd_port->pd_lock);
		*cnt = pd_port->id_vdo_nr;
		memcpy(vdos, pd_port->id_vdos,
			sizeof(uint32_t) * pd_port->id_vdo_nr);
		mutex_unlock(&pd_port->pd_lock);
	}

	return TCPM_SUCCESS;
}

#ifdef CONFIG_USB_PD_ALT_MODE

int tcpm_dp_attention(
	struct tcpc_device *tcpc_dev, uint32_t dp_status)
{
	bool ret = false;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	mutex_lock(&pd_port->pd_lock);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_ATTENTION);

	if (ret) {
		pd_port->dp_status = dp_status;
		pd_port->mode_svid = USB_SID_DISPLAYPORT;
	}

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_dp_attention);

#ifdef CONFIG_USB_PD_ALT_MODE_DFP

int tcpm_dp_status_update(
	struct tcpc_device *tcpc_dev, uint32_t dp_status)
{
	bool ret = false;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	mutex_lock(&pd_port->pd_lock);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_DP_STATUS_UPDATE);

	if (ret) {
		pd_port->dp_status = dp_status;
		pd_port->mode_svid = USB_SID_DISPLAYPORT;
	}

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_dp_status_update);

int tcpm_dp_configuration(
	struct tcpc_device *tcpc_dev, uint32_t dp_config)
{
	bool ret = false;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	mutex_lock(&pd_port->pd_lock);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_DP_CONFIG);

	if (ret) {
		pd_port->local_dp_config = dp_config;
		pd_port->mode_svid = USB_SID_DISPLAYPORT;
	}

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_dp_configuration);

#endif	/* CONFIG_USB_PD_ALT_MODE_DFP */
#endif	/* CONFIG_USB_PD_ALT_MODE */

#ifdef CONFIG_USB_PD_UVDM
int tcpm_send_uvdm(struct tcpc_device *tcpc_dev,
	uint8_t cnt, uint32_t *data, bool wait_resp)
{
	bool ret = false;
	pd_port_t *pd_port = &tcpc_dev->pd_port;

	if (tcpc_dev->typec_attach_old == TYPEC_UNATTACHED)
		return TCPM_ERROR_UNATTACHED;

	if (!pd_port->pd_prev_connected)
		return TCPM_ERROR_NO_PD_CONNECTED;

	if (cnt > VDO_MAX_SIZE)
		return TCPM_ERROR_PARAMETER;

	mutex_lock(&pd_port->pd_lock);

	pd_port->uvdm_cnt = cnt;
	pd_port->uvdm_wait_resp = wait_resp;
	memcpy(pd_port->uvdm_data, data, sizeof(uint32_t) * cnt);

	ret = vdm_put_dpm_vdm_request_event(
		pd_port, PD_DPM_VDM_REQUEST_UVDM);

	mutex_unlock(&pd_port->pd_lock);

	if (!ret)
		return TCPM_ERROR_PUT_EVENT;

	return 0;
}
#endif	/* CONFIG_USB_PD_UVDM */

int tcpm_notify_vbus_stable(
	struct tcpc_device *tcpc_dev)
{
#if CONFIG_USB_PD_VBUS_STABLE_TOUT
	tcpc_disable_timer(tcpc_dev, PD_TIMER_VBUS_STABLE);
#endif

	pd_put_vbus_stable_event(tcpc_dev);
	return TCPM_SUCCESS;
}
EXPORT_SYMBOL(tcpm_notify_vbus_stable);

#ifdef CONFIG_POGO_PIN
int tcpm_typec_disable_function(
        struct tcpc_device *tcpc_dev, bool disable)
{
	if (disable)
		return tcpc_typec_disable(tcpc_dev);

	return tcpc_typec_enable(tcpc_dev);
}
#endif
#endif /* CONFIG_USB_POWER_DELIVERY */
