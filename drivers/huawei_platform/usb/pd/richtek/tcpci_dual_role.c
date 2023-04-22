/*
 * Copyright (C) 2016 Richtek Technology Corp.
 *
 * TCPC Interface for dual role
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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/timekeeping.h>
#include <linux/time64.h>
#include <huawei_platform/usb/pd/richtek/tcpci.h>
#include <huawei_platform/usb/pd/richtek/tcpci_typec.h>

#ifdef CONFIG_DUAL_ROLE_USB_INTF
#include <linux/usb/class-dual-role.h>
void tcpc_dual_role_instance_changed(struct dual_role_phy_instance *dual_role);
static enum dual_role_property tcpc_dual_role_props[] = {
	DUAL_ROLE_PROP_SUPPORTED_MODES,
	DUAL_ROLE_PROP_MODE,
	DUAL_ROLE_PROP_PR,
	DUAL_ROLE_PROP_DR,
	DUAL_ROLE_PROP_VCONN_SUPPLY,
};

static int tcpc_dual_role_get_prop(struct dual_role_phy_instance *dual_role,
			enum dual_role_property prop, unsigned int *val)
{
	struct tcpc_device *tcpc = dev_get_drvdata(dual_role->dev.parent);
	int ret = 0;

	pr_info("%s + %d\n", __func__, prop);
	switch (prop) {
	case DUAL_ROLE_PROP_SUPPORTED_MODES:
		*val = tcpc->dual_role_supported_modes;
		break;
	case DUAL_ROLE_PROP_MODE:
		*val = tcpc->dual_role_mode;
		break;
	case DUAL_ROLE_PROP_PR:
		*val = tcpc->dual_role_pr;
		break;
	case DUAL_ROLE_PROP_DR:
		*val = tcpc->dual_role_dr;
		break;
	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		*val = tcpc->dual_role_vconn;
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static	int tcpc_dual_role_prop_is_writeable(
	struct dual_role_phy_instance *dual_role, enum dual_role_property prop)
{
	int retval = -ENOSYS;
	struct tcpc_device *tcpc = dev_get_drvdata(dual_role->dev.parent);
	pr_info("%s + %d\n", __func__, prop);
	switch (prop) {
	case DUAL_ROLE_PROP_PR:
	case DUAL_ROLE_PROP_DR:
	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		if (tcpc->dual_role_supported_modes ==
			DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP)
			retval = 1;
		break;
	default:
		break;
	}
	return retval;
}

#define CONFIG_USB_POWER_DELIVERY_DUAL_ROLE_SWAP
static int tcpc_dual_role_set_prop(struct dual_role_phy_instance *dual_role,
			enum dual_role_property prop, const unsigned int *val)
{
	struct tcpc_device *tcpc = dev_get_drvdata(dual_role->dev.parent);
	pr_info("%s + %d\n", __func__, prop);

	switch (prop) {
	#ifdef CONFIG_USB_POWER_DELIVERY_DUAL_ROLE_SWAP
	case DUAL_ROLE_PROP_PR:
		if (*val != tcpc->dual_role_pr) {
			pr_info("%s power role swap (%d->%d)\n",
				__func__, tcpc->dual_role_pr, *val);
			tcpm_power_role_swap(tcpc);
		} else
			pr_info("%s Same Power Role\n", __func__);
		break;
	case DUAL_ROLE_PROP_DR:
		if (*val != tcpc->dual_role_dr) {
			pr_info("%s data role swap (%d->%d)\n",
				__func__, tcpc->dual_role_dr, *val);
			tcpm_data_role_swap(tcpc);
		} else
			pr_info("%s Same Data Role\n", __func__);
		break;
	case DUAL_ROLE_PROP_VCONN_SUPPLY:
		if (*val != tcpc->dual_role_vconn) {
			pr_info("%s vconn swap (%d->%d)\n",
				__func__, tcpc->dual_role_vconn, *val);
			tcpm_vconn_swap(tcpc);
		} else
			pr_info("%s Same Vconn\n", __func__);
		break;
	default:
		break;
	#else /* TypeC Only */
	case DUAL_ROLE_PROP_PR:
		tcpm_typec_role_swap(tcpc);
	default:
		break;
	#endif /* CONFIG_USB_POWER_DELIVERY_DUAL_ROLE_SWAP */
	}
	return 0;
}

static void tcpc_get_dual_desc(struct tcpc_device *tcpc)
{
	struct device_node *np = of_find_node_by_name(NULL, tcpc->desc.name);
	u32 val;

	if (!np)
		return;

	if (of_property_read_u32(np, "rt-dual,supported_modes", &val) >= 0) {
		if (val > DUAL_ROLE_PROP_SUPPORTED_MODES_TOTAL)
			tcpc->dual_role_supported_modes =
					DUAL_ROLE_SUPPORTED_MODES_DFP_AND_UFP;
		else
			tcpc->dual_role_supported_modes = val;
	}
}
void tcpc_dual_role_instance_changed(struct dual_role_phy_instance *dual_role)
{
	static int change_counter = 0;
	static bool first_enter = true;
	static struct timespec64 ts64_last;
	struct timespec64 ts64_interval;
	struct timespec64 ts64_now;
	struct timespec64 ts64_sum;
	int change_counter_threshold = 100;

	ts64_interval.tv_sec = 0;
	ts64_interval.tv_nsec = 100 * NSEC_PER_MSEC;
	ts64_now = current_kernel_time64();
	if (first_enter) {
		first_enter = false;
	} else {
		ts64_sum = timespec64_add_safe(ts64_last, ts64_interval);
		if (ts64_sum.tv_sec == TIME_T_MAX) {
			pr_err("%s time overflow happend\n",__func__);
			change_counter = 0;
		} else if (timespec64_compare(&ts64_sum, &ts64_now) >= 0) {
			++change_counter;
			change_counter = change_counter > change_counter_threshold ? change_counter_threshold: change_counter;
			pr_info("%s change_counter = %d\n",__func__, change_counter);
		} else {
			change_counter = 0;
		}
	}
	if (change_counter >= change_counter_threshold) {
		pr_err("%s change_counter hit\n",__func__);
	} else {
		dual_role_instance_changed(dual_role);
	}
	ts64_last =  ts64_now;
}
EXPORT_SYMBOL(tcpc_dual_role_instance_changed);
int tcpc_dual_role_phy_init(
			struct tcpc_device *tcpc)
{
	struct dual_role_phy_desc *dual_desc;
	int len;

	tcpc->dr_usb = devm_kzalloc(&tcpc->dev,
				sizeof(tcpc->dr_usb), GFP_KERNEL);

	dual_desc = devm_kzalloc(&tcpc->dev, sizeof(*dual_desc), GFP_KERNEL);
	if (!dual_desc)
		return -ENOMEM;

	tcpc_get_dual_desc(tcpc);



	dual_desc->name = "otg_default";

	dual_desc->properties = tcpc_dual_role_props;
	dual_desc->num_properties = ARRAY_SIZE(tcpc_dual_role_props);
	dual_desc->get_property = tcpc_dual_role_get_prop;
	dual_desc->set_property = tcpc_dual_role_set_prop;
	dual_desc->property_is_writeable = tcpc_dual_role_prop_is_writeable;

	tcpc->dr_usb = devm_dual_role_instance_register(&tcpc->dev, dual_desc);
	if (IS_ERR(tcpc->dr_usb)) {
		dev_err(&tcpc->dev, "tcpc fail to register dual role usb\n");
		return -EINVAL;
	}
	/* init dual role phy instance property */
	tcpc->dual_role_pr = DUAL_ROLE_PROP_PR_NONE;
	tcpc->dual_role_dr = DUAL_ROLE_PROP_DR_NONE;
	tcpc->dual_role_mode = DUAL_ROLE_PROP_MODE_NONE;
	tcpc->dual_role_vconn = DUAL_ROLE_PROP_VCONN_SUPPLY_NO;
	return 0;
}
EXPORT_SYMBOL(tcpc_dual_role_phy_init);
#endif /* CONFIG_DUAL_ROLE_USB_INTF */
