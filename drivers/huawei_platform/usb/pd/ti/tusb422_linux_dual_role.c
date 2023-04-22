/*
 * TUSB422 Power Delivery
 *
 * Author: Brian Quach <brian.quach@ti.com>
 *
 * Copyright (C) 2016 Texas Instruments Incorporated - http://www.ti.com/
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as 
 * published by the Free Software Foundation version 2.
 *
 * This program is distributed "as is" WITHOUT ANY WARRANTY of any
 * kind, whether express or implied; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifdef CONFIG_DUAL_ROLE_USB_INTF

#include "tusb422_linux_dual_role.h"
#include "tcpm.h"
#include "usb_pd.h"
#include "usb_pd_policy_engine.h"
#include <linux/device.h>
#include <linux/module.h>
#include <linux/printk.h>
#include <linux/slab.h>
#include <linux/usb/class-dual-role.h>

extern usb_pd_port_config_t* usb_pd_pm_get_config(unsigned int port);

/* Uncomment the following line to use USB-PD messaging for changing the data or power role */
//#define USE_USB_PD_FOR_ROLE_CHANGE 

struct dual_role_phy_instance *tusb422_dual_role_phy;

static enum dual_role_property tusb422_dual_role_props[] = {
	DUAL_ROLE_PROP_SUPPORTED_MODES,
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
	DUAL_ROLE_PROP_VCONN_SUPPLY
};

static int tusb422_dual_role_get_prop(struct dual_role_phy_instance *dual_role,
									  enum dual_role_property prop,
									  unsigned int *val) 
{
	static uint8_t prop_mode = DUAL_ROLE_PROP_MODE_NONE;
	static uint8_t prop_pr = DUAL_ROLE_PROP_PR_NONE;
	static uint8_t prop_dr = DUAL_ROLE_PROP_DR_NONE;
	static uint8_t prop_vconn = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
	usb_pd_port_t *pd_dev = usb_pd_pe_get_device(0);
	tcpc_device_t *tcpc_dev = tcpm_get_device(0);

	switch (prop) {
	case DUAL_ROLE_PROP_SUPPORTED_MODES:
		if ((tcpc_dev->role == ROLE_DRP) || (tcpc_dev->flags & TC_FLAGS_TEMP_ROLE))
			*val = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
		else if (tcpc_dev->role == ROLE_SRC)
			*val = DUAL_ROLE_SUPPORTED_MODES_DFP;
		else
			*val = DUAL_ROLE_SUPPORTED_MODES_UFP;
		pr_info("%s: supported_modes = %s\n", __func__,
				(*val == DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP) ? "DFP & UFP" :
				(*val == DUAL_ROLE_SUPPORTED_MODES_DFP) ? "DFP" : "UFP");
		break;

	case DUAL_ROLE_PROP_MODE:
		if (tcpc_dev->flags & TC_FLAGS_TEMP_ROLE)
			*val = prop_mode;
		else if (tcpc_dev->state == TCPC_STATE_ATTACHED_SRC)
			*val = DUAL_ROLE_PROP_MODE_DFP;
		else if (tcpc_dev->state == TCPC_STATE_ATTACHED_SNK)
			*val = DUAL_ROLE_PROP_MODE_UFP;
		else
			*val = DUAL_ROLE_PROP_MODE_NONE;
		prop_mode = *val;
		pr_info("%s: mode = %s\n", __func__,
				(*val == DUAL_ROLE_PROP_MODE_DFP) ? "DFP" :
				(*val == DUAL_ROLE_PROP_MODE_UFP) ? "UFP" : "None");
		break;

	case DUAL_ROLE_PROP_PR:
		if (tcpc_dev->flags & TC_FLAGS_TEMP_ROLE)
			*val = prop_pr;
		else if ((tcpc_dev->state == TCPC_STATE_ATTACHED_SRC) || 
				 (tcpc_dev->state == TCPC_STATE_ATTACHED_SNK)) {
			if (pd_dev->power_role == PD_PWR_ROLE_SNK)
				*val = DUAL_ROLE_PROP_PR_SNK;
			else
				*val = DUAL_ROLE_PROP_PR_SRC;
		} else
			*val = DUAL_ROLE_PROP_PR_NONE;
		prop_pr = *val;
		pr_info("%s: pwr_role = %s\n", __func__,
				(*val == DUAL_ROLE_PROP_PR_SNK) ? "Sink" :
				(*val == DUAL_ROLE_PROP_PR_SRC) ? "Source" : "None");
		break;

	case DUAL_ROLE_PROP_DR:
		if (tcpc_dev->flags & TC_FLAGS_TEMP_ROLE)
			*val = prop_dr;
		else if ((tcpc_dev->state == TCPC_STATE_ATTACHED_SRC) || 
				 (tcpc_dev->state == TCPC_STATE_ATTACHED_SNK)) {
			if (pd_dev->data_role == PD_DATA_ROLE_UFP)
				*val = DUAL_ROLE_PROP_DR_DEVICE;
			else
				*val = DUAL_ROLE_PROP_DR_HOST;
		} else
			*val = DUAL_ROLE_PROP_DR_NONE;
		prop_dr = *val;
		pr_info("%s: data_role = %s\n", __func__,
				(*val == DUAL_ROLE_PROP_DR_DEVICE) ? "Device" :
				(*val == DUAL_ROLE_PROP_DR_HOST) ? "Host" : "None");
		break;

	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		if (tcpc_dev->flags & TC_FLAGS_TEMP_ROLE)
			*val = prop_vconn;
		else if (tcpm_is_vconn_enabled(tcpc_dev->port))
			*val = DUAL_ROLE_PROP_VCONN_SUPPLY_YES;
		else
			*val = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
		prop_vconn = *val;
		pr_info("%s: vconn_supply = %s\n", __func__,
				(*val == DUAL_ROLE_PROP_VCONN_SUPPLY_YES) ? "Yes" : "No");
		break;

	default:
		return -EINVAL;
	}


	return 0;
}

static int tusb422_dual_role_set_prop(struct dual_role_phy_instance *dual_role,
									  enum dual_role_property prop,
									  const unsigned int *val) 
{
	int ret = 0;
	usb_pd_port_t *pd_dev = usb_pd_pe_get_device(0);

	switch (prop) {
	case DUAL_ROLE_PROP_PR:
		pr_info("%s: pwr_role = %s\n", __func__,
				(*val == DUAL_ROLE_PROP_PR_SNK) ? "Sink" : "Source");
		if (((*val == DUAL_ROLE_PROP_PR_SNK) && (pd_dev->power_role == PD_PWR_ROLE_SRC)) ||
			((*val == DUAL_ROLE_PROP_PR_SRC) && (pd_dev->power_role == PD_PWR_ROLE_SNK))) {
#ifdef USE_USB_PD_FOR_ROLE_CHANGE
			if (usb_pd_policy_manager_request(pd_dev->port, PD_POLICY_MNGR_REQ_PR_SWAP))
				tcpm_try_role_swap(pd_dev->port);
#else
			tcpm_try_role_swap(pd_dev->port);
#endif
		}
		break;

	case DUAL_ROLE_PROP_DR:
		pr_info("%s: data_role = %s\n", __func__,
				(*val == DUAL_ROLE_PROP_DR_HOST) ? "Host" : "Device");
#ifdef USE_USB_PD_FOR_ROLE_CHANGE
		if (((*val == DUAL_ROLE_PROP_DR_HOST) && (pd_dev->data_role == PD_DATA_ROLE_UFP)) ||
			((*val == DUAL_ROLE_PROP_DR_DEVICE) && (pd_dev->data_role == PD_DATA_ROLE_DFP))) {
			if (usb_pd_policy_manager_request(pd_dev->port, PD_POLICY_MNGR_REQ_DR_SWAP))
				ret = -EBUSY;
		}
#endif
		break;

	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		pr_info("%s: vconn_supply = %s\n", __func__,
				(*val == DUAL_ROLE_PROP_VCONN_SUPPLY_YES) ? "Yes" : "No");
		if (((*val == DUAL_ROLE_PROP_VCONN_SUPPLY_NO) && tcpm_is_vconn_enabled(pd_dev->port))||
			((*val == DUAL_ROLE_PROP_VCONN_SUPPLY_YES) && !tcpm_is_vconn_enabled(pd_dev->port))) {

			if (usb_pd_policy_manager_request(pd_dev->port, PD_POLICY_MNGR_REQ_VCONN_SWAP))
				ret = -EBUSY;
		}
		break;

	default:
		ret = -EINVAL;
	}

	return ret;
}

static int tusb422_dual_role_prop_is_writeable(struct dual_role_phy_instance *dual_role,
											   enum dual_role_property prop)
{
	tcpc_device_t *tcpc_dev = tcpm_get_device(0);
	usb_pd_port_config_t *pd_config = usb_pd_pm_get_config(0);
	int ret = 0;

	switch (prop) {
	case DUAL_ROLE_PROP_PR:
		if (tcpc_dev->role == ROLE_DRP)
			ret = 1;
		break;

	case DUAL_ROLE_PROP_DR:
		if (pd_config->dual_role_data)
			ret = 1;
		break;

	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		ret = 1;
		break;

	default:
		break;
	}

	pr_info("%s: %s = %u\n", __func__,
			(prop == DUAL_ROLE_PROP_MODE) ? "mode" :
			(prop == DUAL_ROLE_PROP_PR) ? "pwr_role" :
			(prop == DUAL_ROLE_PROP_DR) ? "data_role" :
			(prop == DUAL_ROLE_PROP_VCONN_SUPPLY) ? "vconn_supply" : "?", ret);

	return ret;
}


int tusb422_linux_dual_role_init(struct device *dev) 
{
	struct dual_role_phy_desc *drp_desc;

	drp_desc = devm_kzalloc(dev, sizeof(*drp_desc), GFP_KERNEL);
	if (!drp_desc)
		return -ENOMEM;

	drp_desc->name = "otg_default";
	drp_desc->supported_modes = DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
	drp_desc->properties = tusb422_dual_role_props;
	drp_desc->num_properties = ARRAY_SIZE(tusb422_dual_role_props);
	drp_desc->get_property = tusb422_dual_role_get_prop;
	drp_desc->set_property = tusb422_dual_role_set_prop;
	drp_desc->property_is_writeable = tusb422_dual_role_prop_is_writeable;

	tusb422_dual_role_phy = devm_dual_role_instance_register(dev, drp_desc);
	if (IS_ERR(tusb422_dual_role_phy)) {
		dev_err(dev, "failed to register dual role instance\n");
		return -EINVAL;
	}

	pr_info("%s: OK\n", __func__);
	return 0;
}

#endif /* CONFIG_DUAL_ROLE_USB_INTF */
