/************************************************************
*
* Copyright (C), 1988-1999, Huawei Tech. Co., Ltd.
* FileName: hw_typec.c
* Author: suoandajie(00318894)       Version : 0.1      Date:  2016-5-9
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*  Description:    .c file for type-c core layer which is used to handle
*                  pulic logic management for different chips and to
*                  provide interfaces for exteranl modules.
*  Version:
*  Function List:
*  History:
*  <author>  <time>   <version >   <desc>
***********************************************************/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/wakelock.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/console.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#include <linux/hisi/usb/hisi_usb.h>
#include "huawei_platform/usb/switch/switch_ap/switch_usb_class.h"
#include <linux/usb/class-dual-role.h>
#include <huawei_platform/usb/pd/richtek/tcpm.h>
#include <huawei_platform/power/direct_charger.h>
#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif
#ifdef CONFIG_CONTEXTHUB_PD
#include <linux/hisi/contexthub/tca.h>
#endif
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_charger.h>
#include <huawei_platform/power/wireless_otg.h>
#endif

#include "huawei_platform/audio/usb_analog_hs_interface.h"
#include "huawei_platform/audio/usb_audio_power.h"
#include "huawei_platform/dp_aux_switch/dp_aux_switch.h"

#include <linux/fb.h>
#include <huawei_platform/usb/hw_usb.h>
#ifdef CONFIG_HUAWEI_HISHOW
#include <huawei_platform/usb/hw_hishow.h>
#endif
#ifdef CONFIG_CONTEXTHUB_PD
#define PD_DPM_WAIT_COMBPHY_CONFIGDONE() \
		wait_for_completion_timeout(&pd_dpm_combphy_configdone_completion, msecs_to_jiffies(11500)); \
		reinit_completion(&pd_dpm_combphy_configdone_completion)
#endif
struct pd_dpm_info *g_pd_di = NULL;
static bool g_pd_cc_orientation = false;
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
static bool g_pd_smart_holder = false;
int support_smart_holder = 0;
#endif
static struct class *typec_class = NULL;
static struct device *typec_dev = NULL;
static struct mutex dpm_sink_vbus_lock;
static int pd_dpm_typec_state = PD_DPM_USB_TYPEC_DETACHED;
static int pd_dpm_analog_hs_state = 0;
static struct pd_dpm_vbus_state g_vbus_state;
static unsigned long g_charger_type_event;
struct completion pd_get_typec_state_completion;
#ifdef CONFIG_CONTEXTHUB_PD
static int g_ack = 0;
struct completion pd_dpm_combphy_configdone_completion;
#endif
static bool g_pd_high_power_charging_status = false;
static bool g_pd_high_voltage_charging_status = false;
static bool g_pd_optional_max_power_status = false;
struct cc_check_ops* g_cc_check_ops;
static bool ignore_vbus_on_event = false;
static bool ignore_vbus_off_event = false;
static bool ignore_bc12_event_when_vbuson = false;
static bool ignore_bc12_event_when_vbusoff = false;
static enum pd_dpm_cc_voltage_type remote_rp_level = PD_DPM_CC_VOLT_OPEN;
static struct pd_dpm_ops *g_ops;
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
static struct cable_vdo_ops *g_cable_vdo_ops = NULL;
#endif
static void *g_client;
#ifdef CONFIG_CONTEXTHUB_PD
static bool g_last_hpd_status = false;
static TCPC_MUX_CTRL_TYPE g_combphy_mode = TCPC_NC;
#endif
static int switch_manual_enable = 1;
static unsigned int abnormal_cc_detection = 0;
static unsigned int abnormal_cc_interval = PD_DPM_CC_CHANGE_INTERVAL;
int support_dp = 1;
int otg_channel = 0;
int moisture_detection_by_cc_enable = 0;
int support_analog_audio = 1;
static struct console g_con;
struct mutex typec_state_lock;
struct mutex typec_wait_lock;

void reinit_typec_completion(void);
void typec_complete(enum pd_wait_typec_complete typec_completion);
void pd_dpm_set_cc_mode(int mode);

#ifndef HWLOG_TAG
#define HWLOG_TAG huawei_pd
HWLOG_REGIST();
#endif
extern void chg_set_adaptor_test_result(enum adaptor_name charger_type, enum test_state result);
#ifdef CONFIG_CONTEXTHUB_PD
extern void dp_aux_uart_switch_disable(void);
#endif
static bool g_ignore_vbus_only_event = false;
int pmic_vbus_irq_is_enabled(void);
int g_cur_usb_event = PD_DPM_USB_TYPEC_NONE;
static enum charger_event_type sink_source_type = STOP_SINK;

static int pd_reset_adapter = PD_ADAPTER_9V;
static struct abnomal_change_info abnomal_change[] = {
	{PD_DPM_ABNORMAL_CC_CHANGE, true, 0, 0, {0}, {0}, {0}, {0}},
	{PD_DPM_UNATTACHED_VBUS_ONLY, true, 0, 0, {0}, {0}, {0}, {0}},
};

static int pd_dpm_handle_fb_event(struct notifier_block *self,
                unsigned long event, void *data)
{
        struct fb_event *fb_event = data;
        int *blank = fb_event->data;

        switch (event) {
        case FB_EVENT_BLANK:
                switch (*blank) {
                case FB_BLANK_UNBLANK:
                        hwlog_err("%s set pd to drp\n",__func__);
                        pd_dpm_set_cc_mode(PD_DPM_CC_MODE_DRP);
                        break;
                case FB_BLANK_POWERDOWN:
                        break;
                default:
                        break;
                }
                break;
        default:
                break;
        }

        return NOTIFY_DONE;
}

static struct notifier_block pd_dpm_handle_fb_notifier = {
        .notifier_call = pd_dpm_handle_fb_event,
};

static void init_fb_notification(void)
{
        fb_register_client(&pd_dpm_handle_fb_notifier);
}

#if 0
static void deinit_fb_notification(void)
{
        fb_unregister_client(&pd_dpm_handle_fb_notifier);
}
#endif
static void pd_dpm_set_source_sink_state(enum charger_event_type type)
{
	sink_source_type = type;
	charger_source_sink_event(type);
}
enum charger_event_type pd_dpm_get_source_sink_state(void)
{
	return sink_source_type;
}
int pd_dpm_ops_register(struct pd_dpm_ops *ops, void *client)
{
	int ret = 0;

	if (ops != NULL) {
		g_ops = ops;
		g_client = client;
	} else {
		hwlog_err("pd_dpm ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
int pd_dpm_cable_vdo_ops_register(struct cable_vdo_ops *ops)
{
	int ret = 0;
	if (ops != NULL)
	{
		g_cable_vdo_ops = ops;
	}
	else
	{
		hwlog_err("cable_vdo_ops ops register fail!\n");
		ret = -EPERM;
	}
	return ret;

}
#endif

void pd_dpm_hard_reset(void)
{
	hwlog_err("%s++!\n", __func__);
	if (NULL == g_ops) {
		return;
	}
	if (NULL == g_ops->pd_dpm_hard_reset) {
		return;
	}
	if (g_ops && g_ops->pd_dpm_hard_reset) {
		g_ops->pd_dpm_hard_reset(g_client);
	}
	hwlog_err("%s--!\n", __func__);
}

void pd_dpm_set_cc_mode(int mode)
{
	static int cur_mode = PD_DPM_CC_MODE_DRP;
	hwlog_info("%s cur_mode = %d, new mode = %d\n",__func__, cur_mode, mode);
	if (g_ops && g_ops->pd_dpm_set_cc_mode) {
		if (cur_mode != mode) {
			g_ops->pd_dpm_set_cc_mode(mode);
			cur_mode = mode;
		}
	}
}

bool pd_dpm_get_hw_dock_svid_exist(void)
{
        if (g_ops && g_ops->pd_dpm_get_hw_dock_svid_exist) {
                return g_ops->pd_dpm_get_hw_dock_svid_exist(g_client);
        }

        return false;
}
int pd_dpm_notify_direct_charge_status(bool dc)
{
	hwlog_err("%s,%d", __func__, __LINE__);
        if (g_ops && g_ops->pd_dpm_notify_direct_charge_status) {
		hwlog_err("%s,%d", __func__, __LINE__);
                return g_ops->pd_dpm_notify_direct_charge_status(g_client, dc);
        }

        return false;
}

bool pd_dpm_set_voltage(int vol)
{
	hwlog_err("%s,%d\n", __func__, __LINE__);
	if (g_ops && g_ops->pd_dpm_set_voltage) {
		hwlog_err("%s,%d\n", __func__, __LINE__);
		pd_dpm_set_pd_reset_adapter(vol);
		g_ops->pd_dpm_set_voltage(g_client, vol);
		return true;
	}
	return false;
}

int pd_dpm_get_pd_reset_adapter(void)
{
	return pd_reset_adapter;
}

void pd_dpm_set_pd_reset_adapter(int ra)
{
	pd_reset_adapter = ra;
}

void pd_dpm_set_cc_voltage(int type)
{
	remote_rp_level = type;
}

enum pd_dpm_cc_voltage_type pd_dpm_get_cc_voltage(void)
{
	return remote_rp_level;
}

#ifdef CONFIG_CONTEXTHUB_PD
void pd_dpm_set_combphy_status(TCPC_MUX_CTRL_TYPE mode)
{
	g_combphy_mode = mode;
}

TCPC_MUX_CTRL_TYPE pd_dpm_get_combphy_status(void)
{
	return g_combphy_mode;
}

void pd_dpm_set_last_hpd_status(bool hpd_status)
{
	g_last_hpd_status = hpd_status;
}

bool pd_dpm_get_last_hpd_status(void)
{
	return g_last_hpd_status;
}
#endif


bool pd_dpm_ignore_vbuson_event(void)
{
	return ignore_vbus_on_event;
}
bool pd_dpm_ignore_vbusoff_event(void)
{
	return ignore_vbus_off_event;
}
void pd_dpm_set_ignore_vbuson_event(bool _ignore_vbus_on_event)
{
	ignore_vbus_on_event = _ignore_vbus_on_event;
}
void pd_dpm_set_ignore_vbusoff_event(bool _ignore_vbus_off_event)
{
	ignore_vbus_off_event = _ignore_vbus_off_event;
}
bool pd_dpm_ignore_bc12_event_when_vbuson(void)
{
	return ignore_bc12_event_when_vbuson;
}
bool pd_dpm_ignore_bc12_event_when_vbusoff(void)
{
	return ignore_bc12_event_when_vbusoff;
}

void pd_dpm_set_ignore_bc12_event_when_vbuson(bool _ignore_bc12_event)
{
	ignore_bc12_event_when_vbuson = _ignore_bc12_event;
}

void pd_dpm_set_ignore_bc12_event_when_vbusoff(bool _ignore_bc12_event)
{
	ignore_bc12_event_when_vbusoff = _ignore_bc12_event;
}


void pd_dpm_send_event(enum pd_dpm_cable_event_type event)
{
	char event_buf[32] = {0};
	char *envp[] = {event_buf, NULL};
	int ret = 0;

	if (!typec_dev) {
		hwlog_info("%s do not support pd just return\n", __func__);
		return;
	}

	switch(event) {
		case USB31_CABLE_IN_EVENT:
			snprintf(event_buf, sizeof(event_buf), "USB3_STATE=ON");
		break;

		case USB31_CABLE_OUT_EVENT:
			snprintf(event_buf, sizeof(event_buf), "USB3_STATE=OFF");
		break;

		case DP_CABLE_IN_EVENT:
			snprintf(event_buf, sizeof(event_buf), "DP_STATE=ON");
		break;

		case DP_CABLE_OUT_EVENT:
			snprintf(event_buf, sizeof(event_buf), "DP_STATE=OFF");
		break;

		case ANA_AUDIO_IN_EVENT:
			snprintf(event_buf, sizeof(event_buf), "ANA_AUDIO_STATE=ON");
		break;

		case ANA_AUDIO_OUT_EVENT:
			snprintf(event_buf, sizeof(event_buf), "ANA_AUDIO_STATE=OFF");
		break;

		default :
			return;
		break;
	}

	ret = kobject_uevent_env(&typec_dev->kobj, KOBJ_CHANGE, envp);

	if (ret < 0) {
		hwlog_err("%s,%d: uevent sending failed!!! ret=%d\n", __func__, __LINE__, ret);
	}
	else {
		hwlog_info("%s,%d: sent uevent %s\n", __func__, __LINE__, envp[0]);
	}
}

bool pd_dpm_get_high_power_charging_status()
{
	hwlog_info("%s status =%d\n", __func__, g_pd_high_power_charging_status);
	return g_pd_high_power_charging_status;
}
void pd_dpm_set_high_power_charging_status(bool status)
{
	hwlog_info("%s status =%d\n", __func__, status);
	g_pd_high_power_charging_status = status;
}

bool pd_dpm_get_high_voltage_charging_status()
{
	hwlog_info("%s status =%d\n", __func__, g_pd_high_voltage_charging_status);
	return g_pd_high_voltage_charging_status;
}
void pd_dpm_set_high_voltage_charging_status(bool status)
{
	hwlog_info("%s status =%d\n", __func__, status);
	g_pd_high_voltage_charging_status = status;
}

bool pd_dpm_get_optional_max_power_status()
{
	hwlog_info("%s status =%d\n", __func__, g_pd_optional_max_power_status);
	return g_pd_optional_max_power_status;
}

void pd_dpm_set_optional_max_power_status(bool status)
{
	hwlog_info("%s status =%d\n", __func__, status);
	g_pd_optional_max_power_status = status;
}

void pd_dpm_get_charge_event(unsigned long *event, struct pd_dpm_vbus_state *state)
{
        hwlog_info("%s event =%d\n", __func__, g_charger_type_event);
	*event = g_charger_type_event;
	memcpy(state, &g_vbus_state, sizeof(struct pd_dpm_vbus_state));
}

static void pd_dpm_set_charge_event(unsigned long event, struct pd_dpm_vbus_state *state)
{
        hwlog_info("%s event =%d\n", __func__, event);
	if(NULL != state)
		memcpy(&g_vbus_state, state, sizeof(struct pd_dpm_vbus_state));
	g_charger_type_event = event;
}

int cc_check_ops_register(struct cc_check_ops* ops)
{
	int ret = 0;
	if (ops != NULL)
	{
		g_cc_check_ops = ops;
	}
	else
	{
		hwlog_err("cc_check ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}
static int direct_charge_cable_detect(void)
{
	int ret;
	if (NULL == g_cc_check_ops)
	{
		return -1;
	}
	ret = g_cc_check_ops -> is_cable_for_direct_charge();
	if (ret)
	{
		hwlog_info("%s:cc_check  fail!\n",__func__);
		return -1;
	}
	return 0;
}
static struct direct_charge_cable_detect_ops cable_detect_ops = {
	.direct_charge_cable_detect = direct_charge_cable_detect,
};
bool pd_dpm_get_cc_orientation(void)
{
	hwlog_info("%s cc_orientation =%d\n", __func__, g_pd_cc_orientation);
	return g_pd_cc_orientation;
}

static void pd_dpm_set_cc_orientation(bool cc_orientation)
{
	hwlog_info("%s cc_orientation =%d\n", __func__, cc_orientation);
	g_pd_cc_orientation = cc_orientation;
}

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
static bool pd_dpm_is_smart_holder(void)
{
	return g_pd_smart_holder;
}
#endif

void pd_dpm_get_typec_state(int *typec_state)
{
	hwlog_info("%s pd_dpm_get_typec_state  = %d\n", __func__, pd_dpm_typec_state);

	*typec_state = pd_dpm_typec_state;

	return ;
}
/*for analog audio driver polling*/
int pd_dpm_get_analog_hs_state(void)
{
	hwlog_info("%s analog_hs_state  = %d\n", __func__, pd_dpm_analog_hs_state);

	return pd_dpm_analog_hs_state;
}
static void pd_dpm_set_typec_state(int typec_state)
{
	hwlog_info("%s pd_dpm_set_typec_state  = %d\n", __func__, typec_state);
	blocking_notifier_call_chain(&g_pd_di->pd_port_status_nh,typec_state, NULL);
	pd_dpm_typec_state = typec_state;

	return ;
}


static ssize_t pd_dpm_cc_orientation_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%s\n", pd_dpm_get_cc_orientation()? "2" : "1");
}

static ssize_t pd_dpm_pd_state_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
	hwlog_info("%s  = %d\n", __func__, pd_dpm_get_pd_finish_flag());
	return scnprintf(buf, PAGE_SIZE, "%s\n", pd_dpm_get_pd_finish_flag()? "0" : "1");
}

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
static ssize_t pd_dpm_smart_holder_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
	hwlog_info("%s  = %d\n", __func__, pd_dpm_is_smart_holder());
	return scnprintf(buf, PAGE_SIZE, "%s\n", pd_dpm_is_smart_holder()? "1" : "0");
}
#endif
static ssize_t pd_dpm_cc_state_show(struct device *dev, struct device_attribute *attr,
                char *buf)
{
	return scnprintf(buf, PAGE_SIZE, "%d\n", pd_dpm_get_cc_state_type());
}

static DEVICE_ATTR(cc_orientation, S_IRUGO, pd_dpm_cc_orientation_show, NULL);
static DEVICE_ATTR(pd_state, S_IRUGO, pd_dpm_pd_state_show, NULL);
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
static DEVICE_ATTR(smart_holder, S_IRUGO, pd_dpm_smart_holder_show, NULL);
#endif
static DEVICE_ATTR(cc_state, S_IRUGO, pd_dpm_cc_state_show, NULL);
static struct attribute *pd_dpm_ctrl_attributes[] = {
	&dev_attr_cc_orientation.attr,
	&dev_attr_pd_state.attr,
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
	&dev_attr_smart_holder.attr,
#endif
	&dev_attr_cc_state.attr,
	NULL,
};

static const struct attribute_group pd_dpm_attr_group = {
	.attrs = pd_dpm_ctrl_attributes,
};

int pd_dpm_wake_unlock_notifier_call(struct pd_dpm_info *di, unsigned long event, void *data)
{
        return atomic_notifier_call_chain(&di->pd_wake_unlock_evt_nh,event, data);
}

int pd_dpm_vbus_notifier_call(struct pd_dpm_info *di, unsigned long event, void *data)
{
	if(CHARGER_TYPE_NONE == event) {
		pd_dpm_set_high_power_charging_status(false);
		pd_dpm_set_optional_max_power_status(false);
		pd_dpm_set_high_voltage_charging_status(false);
	}
	if (PD_DPM_VBUS_TYPE_TYPEC != event) {
		pd_dpm_set_charge_event(event, data);
	}
	return blocking_notifier_call_chain(&di->pd_evt_nh,event, data);
}
static int charge_wake_unlock_notifier_call(struct notifier_block *chrg_wake_unlock_nb,
				      unsigned long event, void *data)
{
	if (g_pd_di)
		pd_dpm_wake_unlock_notifier_call(g_pd_di, PD_WAKE_UNLOCK, NULL);

	return NOTIFY_OK;
}

bool pd_dpm_get_pd_finish_flag(void)
{
	if (g_pd_di)
		return g_pd_di->pd_finish_flag;
	else
		return false;
}

bool pd_dpm_get_pd_source_vbus(void)
{
	if (g_pd_di)
		return g_pd_di->pd_source_vbus;
	else
		return false;
}


/*******************************************************
  Function:       pd_dpm_get_cc_state_type
  Description:   pd_dpm_get_cc_state_type
  Input:           NA
  Output:         NA
  Return:
  		open    56k    22k    10k
	cc1    00       01     10     11
	cc2    00       01     10     11
*******************************************************/
int pd_dpm_get_cc_state_type(void)
{
	if (NULL == g_ops || NULL == g_ops->pd_dpm_get_cc_state){
		return -1;
	}

	return g_ops->pd_dpm_get_cc_state();
}

void pd_dpm_report_pd_source_vconn(void *data)
{
	if (data)
#ifdef CONFIG_BOOST_5V
		boost_5v_enable(*(int *)data, BOOST_CTRL_PD_VCONN);
#endif
	hwlog_info("%s - \n", __func__);
}
void pd_dpm_report_pd_source_vbus(struct pd_dpm_info *di, void *data)
{
	struct pd_dpm_vbus_state *vbus_state = data;

	mutex_lock(&di->sink_vbus_lock);
	ignore_bc12_event_when_vbuson= true;

	if (vbus_state->vbus_type & TCP_VBUS_CTRL_PD_DETECT)
		di->pd_finish_flag = true;

	if (vbus_state->mv == 0) {
		hwlog_info("%s : Disable\n", __func__);
#ifdef CONFIG_WIRELESS_CHARGER
		if (otg_channel)
			wireless_otg_detach_handler(true);
		else {
			pd_dpm_vbus_notifier_call(g_pd_di, CHARGER_TYPE_NONE, data);
			pd_dpm_set_source_sink_state(STOP_SOURCE);
		}
#else
		pd_dpm_vbus_notifier_call(g_pd_di, CHARGER_TYPE_NONE, data);
		pd_dpm_set_source_sink_state(STOP_SOURCE);
#endif
	} else {
		di->pd_source_vbus = true;
		hwlog_info("%s : Source %d mV, %d mA\n", __func__, vbus_state->mv, vbus_state->ma);
#ifdef CONFIG_WIRELESS_CHARGER
		if (!otg_channel) {
			pd_dpm_vbus_notifier_call(g_pd_di, PLEASE_PROVIDE_POWER, data);
			pd_dpm_set_source_sink_state(START_SOURCE);
		}
		else
			wireless_otg_attach_handler(true);
#else
		pd_dpm_vbus_notifier_call(g_pd_di, PLEASE_PROVIDE_POWER, data);
		pd_dpm_set_source_sink_state(START_SOURCE);
#endif
	}
	mutex_unlock(&di->sink_vbus_lock);
}

#define VBUS_VOL_9000MV 9000
void pd_dpm_report_pd_sink_vbus(struct pd_dpm_info *di, void *data)
{
	bool skip = false;
	bool high_power_charging = false;
	bool high_voltage_charging = false;
	unsigned long event;
	struct pd_dpm_vbus_state *vbus_state = data;

	mutex_lock(&di->sink_vbus_lock);
	pd_dpm_set_cc_voltage(vbus_state->remote_rp_level);

	if (vbus_state->vbus_type & TCP_VBUS_CTRL_PD_DETECT){
		chg_set_adaptor_test_result(TYPE_PD,PROTOCOL_FINISH_SUCC);
		ignore_bc12_event_when_vbuson = true;
		di->pd_finish_flag = true;
	}

	if (di->pd_finish_flag) {
		event = PD_DPM_VBUS_TYPE_PD;
	} else {
		event = PD_DPM_VBUS_TYPE_TYPEC;
	}

	vbus_state = data;

	if (vbus_state->mv == 0) {
		if(event == PD_DPM_VBUS_TYPE_PD)
		{
			hwlog_info("%s : Disable\n", __func__);
			pd_dpm_vbus_notifier_call(g_pd_di, CHARGER_TYPE_NONE, data);
			pd_dpm_set_source_sink_state(STOP_SINK);
		}
	}
	else {
		di->pd_source_vbus = false;
		hwlog_info("%s : Sink %d mV, %d mA\n", __func__, vbus_state->mv, vbus_state->ma);
		if((vbus_state->mv * vbus_state->ma) >= 18000000)
		{
			hwlog_info("%s : over 18w\n", __func__);
			high_power_charging = true;
		}
		if(vbus_state->mv >=VBUS_VOL_9000MV)
		{
			hwlog_info("%s : over 9V\n", __func__);
			high_voltage_charging = true;
		}
		hwlog_info("%s : %d\n", __func__, vbus_state->mv * vbus_state->ma);
		pd_dpm_set_high_power_charging_status(high_power_charging);
		pd_dpm_set_high_voltage_charging_status(high_voltage_charging);

		if (pd_dpm_typec_state != PD_DPM_USB_TYPEC_DETACHED) {
			pd_dpm_set_source_sink_state(START_SINK);
		}
		pd_dpm_vbus_notifier_call(g_pd_di, event, data);
	}

	mutex_unlock(&di->sink_vbus_lock);
}

void pd_dpm_wakelock_ctrl(unsigned long event)
{
	if (g_pd_di) {
		if (PD_WAKE_LOCK == event || PD_WAKE_UNLOCK == event)
			pd_dpm_wake_unlock_notifier_call(g_pd_di, event, NULL);
	}
}

void pd_dpm_vbus_ctrl(unsigned long event)//CHARGER_TYPE_NONE,PLEASE_PROVIDE_POWER
{
	if (g_pd_di)
	{
#ifdef CONFIG_WIRELESS_CHARGER
		if (!otg_channel) {
			if (event == PLEASE_PROVIDE_POWER) {
				pd_dpm_set_ignore_vbuson_event(true);
				pd_dpm_set_source_sink_state(START_SOURCE);
			} else {
				pd_dpm_set_ignore_vbusoff_event(true);
				pd_dpm_set_source_sink_state(STOP_SOURCE);
			}
			pd_dpm_vbus_notifier_call(g_pd_di, event, NULL);
		} else {
			if (event == PLEASE_PROVIDE_POWER) {
				pd_dpm_set_ignore_vbuson_event(false);
				wireless_otg_attach_handler(false);
			} else {
				pd_dpm_set_ignore_vbusoff_event(false);
				wireless_otg_detach_handler(false);
			}
		}
#else
		if (event == PLEASE_PROVIDE_POWER) {
			pd_dpm_set_ignore_vbuson_event(true);
			pd_dpm_set_source_sink_state(START_SOURCE);
		} else {
			pd_dpm_set_ignore_vbusoff_event(true);
			pd_dpm_set_source_sink_state(STOP_SOURCE);
		}
		pd_dpm_vbus_notifier_call(g_pd_di, event, NULL);
#endif
		hwlog_info("%s event = %d\n", __func__, event);
	}
}
int pd_dpm_report_bc12(struct notifier_block *usb_nb,
                                    unsigned long event, void *data)
{
	struct pd_dpm_vbus_state *vbus_state = data;
	struct pd_dpm_info *di = container_of(usb_nb, struct pd_dpm_info, usb_nb);

	hwlog_info("%s : received event (%d)\n", __func__, event);

	if(CHARGER_TYPE_NONE == event && !di->pd_finish_flag &&
		!pd_dpm_get_pd_source_vbus())
	{
		hwlog_info("%s : PD_WAKE_UNLOCK \n", __func__);
		pd_dpm_wake_unlock_notifier_call(g_pd_di, PD_WAKE_UNLOCK, NULL);
	}

	hwlog_info("[sn]%s : bc12on %d,bc12off %d\n", __func__, ignore_bc12_event_when_vbuson, ignore_bc12_event_when_vbusoff);

	mutex_lock(&g_pd_di->sink_vbus_lock);
	di->bc12_event = event;

	if(PLEASE_PROVIDE_POWER == event) {
		mutex_unlock(&g_pd_di->sink_vbus_lock);
		return NOTIFY_OK;
	}

	if(pd_dpm_get_pd_source_vbus())
	{
		hwlog_info("%s : line (%d)\n", __func__, __LINE__);
		goto End;
	}
	if(ignore_bc12_event_when_vbuson && CHARGER_TYPE_NONE == event)
	{
		hwlog_info("%s : bc1.2 event not match\n", __func__);
		goto End;
	}
	if(!pmic_vbus_irq_is_enabled() &&
		(STOP_SINK == sink_source_type && CHARGER_TYPE_NONE != event))
	{
		hwlog_info("%s : pd aready in STOP_SINK state,"
			"but bc1.2 event not CHARGER_TYPE_NONE, ignore\n", __func__);
		goto End;
	}
	if(!pmic_vbus_irq_is_enabled() && CHARGER_TYPE_NONE == event)
	{
		hwlog_info("%s : ignore CHARGER_TYPE_NONE\n", __func__);
		goto End;
	}

	if((!ignore_bc12_event_when_vbusoff && CHARGER_TYPE_NONE == event) || (!ignore_bc12_event_when_vbuson && CHARGER_TYPE_NONE != event))
	{
		if (!di->pd_finish_flag) {
			hwlog_info("%s : notify event (%d)\n", __func__, event);
			if (g_cur_usb_event == PD_DPM_TYPEC_ATTACHED_AUDIO) {
				event = CHARGER_TYPE_SDP;
				data = NULL;
			}
			pd_dpm_vbus_notifier_call(di,event,data);
		} else
			hwlog_info("%s : igrone\n", __func__);
	}

End:
	if (CHARGER_TYPE_NONE == event) {
		ignore_bc12_event_when_vbusoff = false;
		ignore_bc12_event_when_vbuson = false;
	}
	mutex_unlock(&g_pd_di->sink_vbus_lock);

	return NOTIFY_OK;
}

int register_pd_wake_unlock_notifier(struct notifier_block *nb)
{
        int ret = 0;

        if (!nb)
                return -EINVAL;

        if(g_pd_di == NULL)
                return ret;

        ret = atomic_notifier_chain_register(&g_pd_di->pd_wake_unlock_evt_nh, nb);
        if (ret != 0)
                return ret;

        return ret;
}
EXPORT_SYMBOL(register_pd_wake_unlock_notifier);

int unregister_pd_wake_unlock_notifier(struct notifier_block *nb)
{
        return atomic_notifier_chain_unregister(&g_pd_di->pd_wake_unlock_evt_nh, nb);
}
EXPORT_SYMBOL(unregister_pd_wake_unlock_notifier);

int register_pd_dpm_notifier(struct notifier_block *nb)
{
	int ret = 0;

	if (!nb)
		return -EINVAL;

	if(g_pd_di == NULL)
		return ret;

	ret = blocking_notifier_chain_register(&g_pd_di->pd_evt_nh, nb);
	if (ret != 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL(register_pd_dpm_notifier);

int unregister_pd_dpm_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&g_pd_di->pd_evt_nh, nb);
}
EXPORT_SYMBOL(unregister_pd_dpm_notifier);

int register_pd_dpm_portstatus_notifier(struct notifier_block *nb)
{
	int ret = 0;

	if (!nb)
		return -EINVAL;

	if(g_pd_di == NULL)
		return ret;

	ret = blocking_notifier_chain_register(&g_pd_di->pd_port_status_nh, nb);
	if (ret != 0)
		return ret;

	return ret;
}
EXPORT_SYMBOL(register_pd_dpm_portstatus_notifier);

int unregister_pd_dpm_portstatus_notifier(struct notifier_block *nb)
{
	return blocking_notifier_chain_unregister(&g_pd_di->pd_port_status_nh, nb);
}
EXPORT_SYMBOL(unregister_pd_dpm_portstatus_notifier);


static inline void pd_dpm_report_device_attach(void)
{
#ifdef CONFIG_CONTEXTHUB_PD
	struct pd_dpm_combphy_event event;
#endif

	hwlog_info("%s \r\n",__func__);

#ifdef CONFIG_CONTEXTHUB_PD
	event.dev_type = TCA_CHARGER_CONNECT_EVENT;
	event.irq_type = TCA_IRQ_HPD_IN;
	event.mode_type = TCPC_USB31_CONNECTED;
	event.typec_orien = pd_dpm_get_cc_orientation();
	pd_dpm_handle_combphy_event(event);
#else
	hisi_usb_otg_event(CHARGER_CONNECT_EVENT);
#endif
}

static inline void pd_dpm_report_host_attach(void)
{
	struct console *con;
#ifdef CONFIG_CONTEXTHUB_PD
	struct pd_dpm_combphy_event event;
#endif

	hwlog_info("%s \r\n",__func__);

#ifdef CONFIG_SWITCH_FSA9685
	if (switch_manual_enable) {
		usbswitch_common_dcd_timeout_enable(true);
		usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
	}
#endif

#ifdef CONFIG_CONTEXTHUB_PD
	event.dev_type = TCA_ID_FALL_EVENT;
	event.irq_type = TCA_IRQ_HPD_IN;
	event.mode_type = TCPC_USB31_CONNECTED;
	event.typec_orien = pd_dpm_get_cc_orientation();
	pd_dpm_handle_combphy_event(event);
#else
	hisi_usb_otg_event(ID_FALL_EVENT);
#endif
}

static inline void pd_dpm_report_device_detach(void)
{
	hwlog_info("%s \r\n",__func__);

#ifdef CONFIG_CONTEXTHUB_PD
	struct pd_dpm_combphy_event event;
	event.dev_type = TCA_CHARGER_DISCONNECT_EVENT;
	event.irq_type = TCA_IRQ_HPD_OUT;
	event.mode_type = TCPC_NC;
	event.typec_orien = pd_dpm_get_cc_orientation();
	pd_dpm_handle_combphy_event(event);
#else
	hisi_usb_otg_event(CHARGER_DISCONNECT_EVENT);
#endif
}

static inline void pd_dpm_report_host_detach(void)
{
#ifdef CONFIG_CONTEXTHUB_PD
	struct pd_dpm_combphy_event event;
#endif

	hwlog_info("%s \r\n",__func__);

#ifdef CONFIG_SWITCH_FSA9685
	usbswitch_common_dcd_timeout_enable(false);
#endif

#ifdef CONFIG_CONTEXTHUB_PD
	event.typec_orien = pd_dpm_get_cc_orientation();
	event.mode_type = pd_dpm_get_combphy_status();
	if (true == pd_dpm_get_last_hpd_status())
	{
		event.dev_type = TCA_DP_OUT;
		event.irq_type = TCA_IRQ_HPD_OUT;
		pd_dpm_handle_combphy_event(event);
		pd_dpm_set_last_hpd_status(false);

		pd_dpm_send_event(DP_CABLE_OUT_EVENT);
	}
	event.dev_type = TCA_ID_RISE_EVENT;
	event.irq_type = TCA_IRQ_HPD_OUT;
	event.mode_type = TCPC_NC;
	pd_dpm_set_combphy_status(TCPC_NC);
	pd_dpm_handle_combphy_event(event);
	/*set aux uart switch low*/
	if (support_dp) {
		dp_aux_uart_switch_disable();
	}
#else
	hisi_usb_otg_event(ID_RISE_EVENT);
#endif
}

static void pd_dpm_report_attach(int new_state)
{
	switch (new_state) {
	case PD_DPM_USB_TYPEC_DEVICE_ATTACHED:
		pd_dpm_report_device_attach();
		break;

	case PD_DPM_USB_TYPEC_HOST_ATTACHED:
		pd_dpm_report_host_attach();
		break;
	}
}

static void pd_dpm_report_detach(int last_state)
{
	switch (last_state) {
	case PD_DPM_USB_TYPEC_DEVICE_ATTACHED:
		pd_dpm_report_device_detach();
		break;

	case PD_DPM_USB_TYPEC_HOST_ATTACHED:
		pd_dpm_report_host_detach();
		break;
	}

	hw_usb_host_abnormal_event_notify(USB_HOST_EVENT_NORMAL);
}

static void pd_dpm_usb_update_state(
				struct work_struct *work)
{
	int new_ev, last_ev;
	struct pd_dpm_info *usb_cb_data =
			container_of(to_delayed_work(work),
					struct pd_dpm_info,
					usb_state_update_work);

	mutex_lock(&usb_cb_data->usb_lock);
	new_ev = usb_cb_data->pending_usb_event;
	mutex_unlock(&usb_cb_data->usb_lock);

	last_ev = usb_cb_data->last_usb_event;

	if (last_ev == new_ev)
		return;

	switch (new_ev) {
	case PD_DPM_USB_TYPEC_DETACHED:
		pd_dpm_report_detach(last_ev);
		break;

	case PD_DPM_USB_TYPEC_DEVICE_ATTACHED:
	case PD_DPM_USB_TYPEC_HOST_ATTACHED:
		if (last_ev != PD_DPM_USB_TYPEC_DETACHED)
			pd_dpm_report_detach(last_ev);
		pd_dpm_report_attach(new_ev);
		break;
	default:
		return;
	}

	usb_cb_data->last_usb_event = new_ev;
}
int pd_dpm_get_cur_usb_event(void)
{
	if (g_pd_di)
		return g_pd_di->cur_usb_event;
}

void pd_dpm_handle_abnomal_change(int event)
{
	int i = 0;
	char dsm_buf[PD_DPM_CC_DMD_BUF_SIZE] = {0};
	int time_diff = 0;
	unsigned int time_diff_index = 0;

	struct timespec64 ts64_interval;
	struct timespec64 ts64_now;
	struct timespec64 ts64_sum;

	struct timespec64 ts64_dmd_interval;
	struct timespec64 ts64_dmd_now;
	struct timespec64 ts64_dmd_sum;

	int* change_counter = &abnomal_change[event].change_counter;
	int change_counter_threshold = PD_DPM_CC_CHANGE_COUNTER_THRESHOLD;
	int* dmd_counter = &abnomal_change[event].dmd_counter;
	int dmd_counter_threshold = PD_DPM_CC_DMD_COUNTER_THRESHOLD;
	ts64_interval.tv_sec = 0;
	ts64_interval.tv_nsec = abnormal_cc_interval * NSEC_PER_MSEC;
	ts64_dmd_interval.tv_sec = PD_DPM_CC_DMD_INTERVAL;
	ts64_dmd_interval.tv_nsec = 0;

	ts64_now = current_kernel_time64();
	if (abnomal_change[event].first_enter) {
		abnomal_change[event].first_enter = false;
	} else {
		ts64_sum = timespec64_add_safe(abnomal_change[event].ts64_last, ts64_interval);
		if (ts64_sum.tv_sec == TIME_T_MAX) {
			hwlog_err("%s time overflow happend\n",__func__);
			*change_counter = 0;
		} else if (timespec64_compare(&ts64_sum, &ts64_now) >= 0) {
			++*change_counter;
			hwlog_info("%s change_counter = %d,\n",__func__, *change_counter);

			time_diff = (ts64_now.tv_sec - abnomal_change[event].ts64_last.tv_sec) * PD_DPM_CC_CHANGE_MSEC + (ts64_now.tv_nsec - abnomal_change[event].ts64_last.tv_nsec) / NSEC_PER_MSEC;
			time_diff_index = time_diff/(PD_DPM_CC_CHANGE_INTERVAL / PD_DPM_CC_CHANGE_BUF_SIZE);
			if (time_diff_index >= PD_DPM_CC_CHANGE_BUF_SIZE)
				time_diff_index = PD_DPM_CC_CHANGE_BUF_SIZE - 1;
			++abnomal_change[event].change_data[time_diff_index];
		} else {
			*change_counter = 0;
			memset(abnomal_change[event].change_data, 0, PD_DPM_CC_CHANGE_BUF_SIZE);
		}
	}

	if (*change_counter >= change_counter_threshold) {
		hwlog_err("%s change_counter hit\n",__func__);
		pd_dpm_set_cc_mode(PD_DPM_CC_MODE_UFP);

		for (i = 0; i < PD_DPM_CC_CHANGE_BUF_SIZE; i++) {
			abnomal_change[event].dmd_data[i] += abnomal_change[event].change_data[i];
		}
		*change_counter = 0;
		memset(abnomal_change[event].change_data, 0, PD_DPM_CC_CHANGE_BUF_SIZE);
		++*dmd_counter;

		if (moisture_detection_by_cc_enable) {
			hwlog_err("%s moisture_detected\n",__func__);
			send_water_intrused_event(true);
		}
	}

	if (*dmd_counter >= dmd_counter_threshold) {
		*dmd_counter = 0;

		ts64_dmd_now = current_kernel_time64();
		ts64_dmd_sum = timespec64_add_safe(abnomal_change[event].ts64_dmd_last, ts64_dmd_interval);
		if (ts64_dmd_sum.tv_sec == TIME_T_MAX) {
			hwlog_err("%s time overflow happend when add 24 hours\n",__func__);
		} else if (timespec64_compare(&ts64_dmd_sum, &ts64_dmd_now) < 0) {
			snprintf(dsm_buf, PD_DPM_CC_DMD_BUF_SIZE - 1, "cc abnormal is triggered:");
			for(i = 0; i < PD_DPM_CC_CHANGE_BUF_SIZE; i++) {
				snprintf(dsm_buf + strlen(dsm_buf), PD_DPM_CC_DMD_BUF_SIZE - 1, " %d", abnomal_change[event].dmd_data[i]);
			}
			snprintf(dsm_buf + strlen(dsm_buf), PD_DPM_CC_DMD_BUF_SIZE - 1, "\n");

			power_dsm_dmd_report(POWER_DSM_BATTERY, ERROR_NO_WATER_CHECK_IN_USB, dsm_buf);

			abnomal_change[event].ts64_dmd_last = ts64_dmd_now;
		} else {
			hwlog_info("error: cc abnormal happend within 24 hour, do not report\n");
		}
	}

	abnomal_change[event].ts64_last =  ts64_now;
}
void pd_dpm_ignore_vbus_only_event(bool flag)
{
	g_ignore_vbus_only_event = flag;
}
int pd_dpm_handle_pe_event(unsigned long event, void *data)
{
	int usb_event = PD_DPM_USB_TYPEC_NONE;
	struct pd_dpm_typec_state *typec_state = NULL;
	bool notify_audio = false;
	int event_id = USB_ANA_HS_PLUG_OUT;

	switch (event) {
	case PD_DPM_PE_ABNORMAL_CC_CHANGE_HANDLER:
#ifdef CONFIG_DIRECT_CHARGER
		if (abnormal_cc_detection && !direct_charge_check_sc_mode())
#else
		if (abnormal_cc_detection)
#endif
		{
			pd_dpm_handle_abnomal_change(PD_DPM_ABNORMAL_CC_CHANGE);
		}

		return 0;

	case PD_DPM_PE_EVT_TYPEC_STATE:
		{
			if(!data || !g_pd_di) {
				hwlog_info("%s data is null\r\n", __func__);
				return -1;
			}

			typec_state = data;
			if (!support_analog_audio && (typec_state->new_state == PD_DPM_TYPEC_ATTACHED_AUDIO)) {
				hwlog_err("%s does not support analog audio\n", __func__);
				return -1;
			}
			g_pd_di->cur_usb_event = typec_state->new_state;
			switch (typec_state->new_state) {
			case PD_DPM_TYPEC_ATTACHED_SNK:
				usb_event = PD_DPM_USB_TYPEC_DEVICE_ATTACHED;
				hwlog_info("%s ATTACHED_SINK\r\n", __func__);
				pd_dpm_set_source_sink_state(START_SINK);
				typec_complete(COMPLETE_FROM_TYPEC_CHANGE);
				break;

			case PD_DPM_TYPEC_ATTACHED_SRC:
				usb_event = PD_DPM_USB_TYPEC_HOST_ATTACHED;
				hwlog_info("%s ATTACHED_SOURCE\r\n", __func__);
				typec_complete(COMPLETE_FROM_TYPEC_CHANGE);
				break;

			case PD_DPM_TYPEC_UNATTACHED:
				/*the sequence can not change, would affect sink_vbus command */
				if (pd_dpm_analog_hs_state == 1) {
					usb_event = PD_DPM_USB_TYPEC_AUDIO_DETACHED;
					notify_audio = true;
					pd_dpm_analog_hs_state = 0;
					event_id = USB_ANA_HS_PLUG_OUT;
					hwlog_info("%s AUDIO UNATTACHED\r\n", __func__);
				} else {
					usb_event = PD_DPM_USB_TYPEC_DETACHED;

					if (START_SINK == sink_source_type) {
						pd_dpm_set_source_sink_state(STOP_SINK);
					}
					hwlog_info("%s UNATTACHED\r\n", __func__);
				}
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
				if(support_smart_holder){
					if(g_pd_smart_holder)
					{
						hishow_notify_android_uevent(HISHOW_DEVICE_OFFLINE,HISHOW_USB_DEVICE);
					}
					g_pd_smart_holder = false;
				}
#endif
				reinit_typec_completion();
				break;

			case PD_DPM_TYPEC_ATTACHED_AUDIO:
				notify_audio = true;
				pd_dpm_analog_hs_state = 1;
				event_id = USB_ANA_HS_PLUG_IN;
				usb_event = PD_DPM_USB_TYPEC_AUDIO_ATTACHED;
				hwlog_info("%s ATTACHED_AUDIO\r\n", __func__);
				typec_complete(COMPLETE_FROM_TYPEC_CHANGE);
				break;
			case PD_DPM_TYPEC_ATTACHED_DBGACC_SNK:
			case PD_DPM_TYPEC_ATTACHED_CUSTOM_SRC:
				usb_event = PD_DPM_USB_TYPEC_DEVICE_ATTACHED;
				pd_dpm_set_source_sink_state(START_SINK);
				pd_dpm_set_cc_voltage(PD_DPM_CC_VOLT_SNK_DFT);
				hwlog_info("%s ATTACHED_CUSTOM_SRC\r\n", __func__);
				typec_complete(COMPLETE_FROM_TYPEC_CHANGE);
				break;
#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
			case PD_DPM_TYPEC_ATTACHED_CUSTOM_SRC2:
				if(support_smart_holder) {
					usb_event = PD_DPM_USB_TYPEC_DEVICE_ATTACHED;
					hwlog_info("%s ATTACHED_CUSTOM_SRC2\r\n", __func__);
					if(g_cable_vdo_ops && g_cable_vdo_ops->is_cust_src2_cable())
					{
						if(!g_pd_smart_holder)
						{
							hishow_notify_android_uevent(HISHOW_DEVICE_ONLINE,HISHOW_USB_DEVICE);
						}
						g_pd_smart_holder = true;
					}
					typec_complete(COMPLETE_FROM_TYPEC_CHANGE);
				}
				break;
#endif
			case PD_DPM_TYPEC_ATTACHED_DEBUG:
				pd_dpm_set_cc_voltage(PD_DPM_CC_VOLT_SNK_DFT);
				break;
			case PD_DPM_TYPEC_ATTACHED_VBUS_ONLY:
				if (g_ignore_vbus_only_event) {
					hwlog_err("%s: ignore ATTACHED_VBUS_ONLY\n", __func__);
					return 0;
				}
				pd_dpm_set_source_sink_state(START_SINK);
				usb_event = PD_DPM_USB_TYPEC_DEVICE_ATTACHED;
				hwlog_info("%s ATTACHED_VBUS_ONLY\r\n", __func__);
				break;
			case PD_DPM_TYPEC_UNATTACHED_VBUS_ONLY:
				hwlog_info("%s UNATTACHED_VBUS_ONLY\r\n", __func__);
				pd_dpm_handle_abnomal_change(PD_DPM_UNATTACHED_VBUS_ONLY);
				usb_event = PD_DPM_USB_TYPEC_DETACHED;
				pd_dpm_set_source_sink_state(STOP_SINK);
				break;

			default:
				hwlog_info("%s can not detect typec state\r\n", __func__);
				break;
			}
			pd_dpm_set_typec_state(usb_event);

			if (typec_state->polarity)
				pd_dpm_set_cc_orientation(true);
			else
				pd_dpm_set_cc_orientation(false);

			if(notify_audio)
			{
				usb_analog_hs_plug_in_out_handle(event_id);
			}

		}
		break;

	case PD_DPM_PE_EVT_PD_STATE:
		{
			struct pd_dpm_pd_state *pd_state = data;
			switch (pd_state->connected) {
			case PD_CONNECT_PE_READY_SNK:
			case PD_CONNECT_PE_READY_SRC:
				break;
			}
		}
		break;

	case PD_DPM_PE_EVT_DIS_VBUS_CTRL:
		{
			hwlog_info("%s : Disable VBUS Control  first \n", __func__);
			if(g_pd_di->pd_finish_flag == true || pd_dpm_get_pd_source_vbus())
			{
				struct pd_dpm_vbus_state vbus_state;
				hwlog_info("%s : Disable VBUS Control\n", __func__);
				vbus_state.mv = 0;
				vbus_state.ma = 0;
#ifdef CONFIG_WIRELESS_CHARGER
				if (otg_channel && g_pd_di->pd_source_vbus)
					wireless_otg_detach_handler(true);
				else {
					pd_dpm_vbus_notifier_call(g_pd_di, CHARGER_TYPE_NONE, &vbus_state);
					if (g_pd_di->pd_source_vbus) {
						pd_dpm_set_source_sink_state(STOP_SOURCE);
					} else {
						pd_dpm_set_source_sink_state(STOP_SINK);
					}
				}
#else
				if (g_pd_di->pd_source_vbus) {
					pd_dpm_set_source_sink_state(STOP_SOURCE);
				} else {
					pd_dpm_set_source_sink_state(STOP_SINK);
				}
				pd_dpm_vbus_notifier_call(g_pd_di, CHARGER_TYPE_NONE, &vbus_state);
#endif
				mutex_lock(&g_pd_di->sink_vbus_lock);
				if (g_pd_di->bc12_event != CHARGER_TYPE_NONE)
					ignore_bc12_event_when_vbusoff = true;
				g_pd_di->pd_source_vbus = false;
				g_pd_di->pd_finish_flag = false;
				usb_audio_power_scharger();
				mutex_unlock(&g_pd_di->sink_vbus_lock);
			}
		}
		break;

	case PD_DPM_PE_EVT_SINK_VBUS:
		{
			pd_dpm_report_pd_sink_vbus(g_pd_di, data);
		}
		break;

	case PD_DPM_PE_EVT_SOURCE_VBUS:
		{
			pd_dpm_report_pd_source_vbus(g_pd_di, data);
		}
		break;

	case PD_DPM_PE_EVT_SOURCE_VCONN:
		{
			pd_dpm_report_pd_source_vconn(data);
			break;
		}
	case PD_DPM_PE_EVT_DR_SWAP:
		{
			struct pd_dpm_swap_state *swap_state = data;
			if (swap_state->new_role == PD_ROLE_DFP)
				usb_event = PD_DPM_USB_TYPEC_HOST_ATTACHED;
			else
				usb_event = PD_DPM_USB_TYPEC_DEVICE_ATTACHED;
		}
		break;

	case PD_DPM_PE_EVT_PR_SWAP:
		break;

	default:
		hwlog_info("%s  unkonw event \r\n", __func__);
		break;
	};

	if (usb_event != PD_DPM_USB_TYPEC_NONE) {
		mutex_lock(&g_pd_di->usb_lock);
		if (g_pd_di->pending_usb_event != usb_event) {
		cancel_delayed_work(&g_pd_di->usb_state_update_work);
		g_pd_di->pending_usb_event = usb_event;
		queue_delayed_work(g_pd_di->usb_wq,
				&g_pd_di->usb_state_update_work,
				msecs_to_jiffies(0));
		} else
			pr_info("Pending event is same --> ignore this event %d\n", usb_event);
		mutex_unlock(&g_pd_di->usb_lock);
	}

	return 0;
}
EXPORT_SYMBOL_GPL(pd_dpm_handle_pe_event);

static int pd_dpm_parse_dt(struct pd_dpm_info *info,
	struct device *dev)
{
	struct device_node *np = dev->of_node;

	if (!np)
		return -EINVAL;
	// default name
	if (of_property_read_string(np, "tcp_name",
		&info->tcpc_name) < 0)
		info->tcpc_name = "type_c_port0";
	if (of_property_read_u32(np,"switch_manual_enable", &switch_manual_enable)) {
		hwlog_err("get switch_manual_enable fail!\n");
		switch_manual_enable = 1;
	}
	hwlog_info("switch_manual_enable = %d!\n", switch_manual_enable);

	if (of_property_read_u32(np,"support_dp", &support_dp)) {
		hwlog_err("get support_dp fail!\n");
	}
	hwlog_info("support_dp = %d!\n", support_dp);
	if (of_property_read_u32(np,"otg_channel", &otg_channel)) {
		hwlog_err("get otg_channel fail!\n");
		otg_channel = 0;
	}
	hwlog_info("otg_channel = %d!\n", otg_channel);
	if (of_property_read_u32(np,"moisture_detection_by_cc_enable", &moisture_detection_by_cc_enable)) {
		hwlog_err("get moisture_detection_by_cc_enable fail!\n");
		moisture_detection_by_cc_enable = 0;
	}
	hwlog_info("moisture_detection_by_cc_enable = %d!\n", moisture_detection_by_cc_enable);

#ifdef CONFIG_TYPEC_CAP_CUSTOM_SRC2
	if (of_property_read_u32(np,"support_smart_holder", &support_smart_holder)) {
			hwlog_err("get support_smart_holder fail!\n");
	}
	hwlog_info("support_smart_holder = %d!\n", support_smart_holder);
#endif
	if (of_property_read_u32(np, "support_analog_audio", &support_analog_audio)) {
		hwlog_err("get support_analog_audio fail!\n");
		support_analog_audio = 1;
	}
	hwlog_info("support_analog_audio = %d!\n", support_analog_audio);
	return 0;
}

#ifdef CONFIG_CONTEXTHUB_PD
static int _iput = 0; 
static int _iget = 0; 
static int n = 0; 
struct pd_dpm_combphy_event combphy_notify_event_buffer[COMBPHY_MAX_PD_EVENT_COUNT];
static int addring (int i)
{
        return (i+1) == COMBPHY_MAX_PD_EVENT_COUNT ? 0 : i+1;
}
static void pd_dpm_init_combphy_notify_event_buffer(void)
{
	int i = 0;
	for(i = 0; i < COMBPHY_MAX_PD_EVENT_COUNT; i++)
	{
		combphy_notify_event_buffer[i].irq_type= COMBPHY_PD_EVENT_INVALID_VAL;
		combphy_notify_event_buffer[i].mode_type = COMBPHY_PD_EVENT_INVALID_VAL;
		combphy_notify_event_buffer[i].dev_type = COMBPHY_PD_EVENT_INVALID_VAL;
		combphy_notify_event_buffer[i].typec_orien = COMBPHY_PD_EVENT_INVALID_VAL;
	}
}
static void pd_dpm_combphy_notify_event_copy(struct pd_dpm_combphy_event *dst_event, struct pd_dpm_combphy_event src_event)
{
	(*dst_event).dev_type = src_event.dev_type;
	(*dst_event).irq_type = src_event.irq_type;
	(*dst_event).mode_type = src_event.mode_type;
	(*dst_event).typec_orien = src_event.typec_orien;
}
static void pd_dpm_print_buffer(int idx)
{
#ifdef COMBPHY_NOTIFY_BUFFER_PRINT
	hwlog_info("\n+++++++++++++++++++++++++++++++++\n");
	hwlog_info("\nbuffer[%d].irq_type %d\n", idx, combphy_notify_event_buffer[idx].irq_type);
	hwlog_info("\nbuffer[%d].mode_type %d\n", idx, combphy_notify_event_buffer[idx].mode_type);
	hwlog_info("\nbuffer[%d].dev_type %d\n", idx, combphy_notify_event_buffer[idx].dev_type);
	hwlog_info("\nbuffer[%d].typec_orien %d\n",idx, combphy_notify_event_buffer[idx].typec_orien);
	hwlog_info("\n+++++++++++++++++++++++++++++++++\n");
#endif
}
static int pd_dpm_put_combphy_pd_event(struct pd_dpm_combphy_event event)
{
	if (n<COMBPHY_MAX_PD_EVENT_COUNT){
		pd_dpm_combphy_notify_event_copy(&(combphy_notify_event_buffer[_iput]), event);
		pd_dpm_print_buffer(_iput);
		_iput = addring(_iput);
		n++;
		hwlog_info("%s - input = %d, n = %d \n", __func__, _iput , n);
		return 0;
	}
	else {
		hwlog_info("%s Buffer is full\n", __func__);
		return -1;
	}
}
static int pd_dpm_get_combphy_pd_event(struct pd_dpm_combphy_event *event)
{
	int pos;
	if (n>0) {
		pos = _iget;
		_iget = addring(_iget);
		n--;
		pd_dpm_combphy_notify_event_copy(event,combphy_notify_event_buffer[pos]);
		pd_dpm_print_buffer(pos);
		hwlog_info("%s - _iget = %d, n = %d \n", __func__, _iget , n);
	}
	else {
		hwlog_info("%s Buffer is empty\n", __func__);
		return -1;
	}
	return n;
}
void dp_dfp_u_notify_dp_configuration_done(TCPC_MUX_CTRL_TYPE mode_type, int ack)
{
	g_ack = ack;
	hwlog_info("%s ret = %d \n", __func__, g_ack);
	complete(&pd_dpm_combphy_configdone_completion);
}

static void pd_dpm_handle_ldo_supply_ctrl(struct pd_dpm_combphy_event event, bool enable)
{
	if ((event.mode_type != TCPC_NC) && (enable == true))
		hw_usb_ldo_supply_enable(HW_USB_LDO_CTRL_COMBOPHY);

	if ((0 == g_ack) && (event.mode_type == TCPC_NC) && (enable == false))
		hw_usb_ldo_supply_disable(HW_USB_LDO_CTRL_COMBOPHY);
}


static void pd_dpm_combphy_event_notify(
				struct work_struct *work)
{
	int ret = 0;
	int event_count = 0;
	int busy_count = 10;
	struct pd_dpm_combphy_event event;
	hwlog_info("%s +\n", __func__);
	do {
		mutex_lock(&g_pd_di->pd_combphy_notify_lock);
		event_count = pd_dpm_get_combphy_pd_event(&event);
		mutex_unlock(&g_pd_di->pd_combphy_notify_lock);

		if(event_count < 0)
			break;

		if (!support_dp && (event.dev_type == TCA_DP_OUT || event.dev_type == TCA_DP_IN)) {
			continue;
		}

		if(event.irq_type == COMBPHY_PD_EVENT_INVALID_VAL || event.mode_type == COMBPHY_PD_EVENT_INVALID_VAL
			|| event.dev_type == COMBPHY_PD_EVENT_INVALID_VAL || event.typec_orien == COMBPHY_PD_EVENT_INVALID_VAL) {
			hwlog_err("%s invalid val\n", __func__);
		}
		else {
			pd_dpm_handle_ldo_supply_ctrl(event, true);

			ret = pd_event_notify(event.irq_type, event.mode_type, event.dev_type, event.typec_orien);
			PD_DPM_WAIT_COMBPHY_CONFIGDONE();

			if(-EBUSY == g_ack) {
				do {
					mdelay(100);
					busy_count--;
					ret = pd_event_notify(event.irq_type, event.mode_type, event.dev_type, event.typec_orien);
					PD_DPM_WAIT_COMBPHY_CONFIGDONE();
					if(-EBUSY != g_ack) {
						hwlog_info("%s %d exit busy succ\n", __func__, __LINE__);
						break;
					}
				} while(busy_count != 0);
				if(busy_count == 0) {
					hwlog_err("%s %d BUSY!\n", __func__, __LINE__);
				}
			}
			pd_dpm_handle_ldo_supply_ctrl(event, false);
		}
	}while(event_count);
	hwlog_info("%s -\n", __func__);
}
static bool pd_dpm_combphy_notify_event_compare(struct pd_dpm_combphy_event eventa, struct pd_dpm_combphy_event eventb)
{
	return ((eventa.dev_type == eventb.dev_type) && (eventa.irq_type == eventb.irq_type)
		&& (eventa.mode_type == eventb.mode_type));
}
int pd_dpm_handle_combphy_event(struct pd_dpm_combphy_event event)
{
	int ret = 0;

	hwlog_info("%s +\n", __func__);
	mutex_lock(&g_pd_di->pd_combphy_notify_lock);
	if (g_pd_di->last_combphy_notify_event.dev_type == TCA_DP_IN || g_pd_di->last_combphy_notify_event.dev_type == TCA_DP_OUT || g_pd_di->last_combphy_notify_event.dev_type == TCA_ID_FALL_EVENT) {
		if ((event.dev_type == TCA_CHARGER_CONNECT_EVENT) || event.dev_type == TCA_CHARGER_DISCONNECT_EVENT) {
			hwlog_info("%s invlid event sequence\n", __func__);
			mutex_unlock(&g_pd_di->pd_combphy_notify_lock);
			return -1;
		}
	}
	if (g_pd_di->last_combphy_notify_event.mode_type == TCPC_NC && event.mode_type == TCPC_NC) {
		hwlog_info("%s repeated TCPC_NC event\n", __func__);
		mutex_unlock(&g_pd_di->pd_combphy_notify_lock);
		return -1;
	}

	if (!pd_dpm_combphy_notify_event_compare(g_pd_di->last_combphy_notify_event , event)) {
		pd_dpm_combphy_notify_event_copy(&(g_pd_di->last_combphy_notify_event), event);
		ret = pd_dpm_put_combphy_pd_event(event);

		if(ret < 0) {
			hwlog_err("%s pd_dpm_put_combphy_pd_event fail\n", __func__);
			mutex_unlock(&g_pd_di->pd_combphy_notify_lock);
			return -1;
		}

		queue_delayed_work(g_pd_di->pd_combphy_wq,
			&g_pd_di->pd_combphy_event_work,
			msecs_to_jiffies(0));
	} else
		hwlog_info("%s Pending event is same --> ignore this event\n", __func__);

	mutex_unlock(&g_pd_di->pd_combphy_notify_lock);
	hwlog_info("%s -\n", __func__);
	return 0;
}
#endif
static int pd_dpm_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct pd_dpm_info *di;
	struct dual_role_phy_desc *desc;
	struct dual_role_phy_instance *dual_role;
	enum hisi_charger_type type;
	hwlog_info("%s +\n", __func__);
#ifdef CONFIG_CONTEXTHUB_PD
	pd_dpm_init_combphy_notify_event_buffer();
#endif
	g_charger_type_event = hisi_get_charger_type();

	di = devm_kzalloc(&pdev->dev,sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("%s: alloc dev failed\n", __func__);
		return -ENOMEM;
	}

	di->dev = &pdev->dev;

	g_pd_di = di;
	mutex_init(&di->sink_vbus_lock);

	BLOCKING_INIT_NOTIFIER_HEAD(&di->pd_evt_nh);
	BLOCKING_INIT_NOTIFIER_HEAD(&di->pd_port_status_nh);
	ATOMIC_INIT_NOTIFIER_HEAD(&di->pd_wake_unlock_evt_nh);

	di->usb_nb.notifier_call = pd_dpm_report_bc12;
	ret = hisi_charger_type_notifier_register(&di->usb_nb);
	if (ret < 0) {
		hwlog_err("hisi_charger_type_notifier_register failed\n");
	}
	di->chrg_wake_unlock_nb.notifier_call = charge_wake_unlock_notifier_call;
	ret = blocking_notifier_chain_register(&charge_wake_unlock_list, &di->chrg_wake_unlock_nb);
	if (ret < 0) {
		hwlog_err("charge_wake_unlock_register_notifier failed\n");
	}

	if (of_property_read_u32(di->dev->of_node, "abnormal_cc_detection", &abnormal_cc_detection))
	{
		hwlog_err("get abnormal_cc_detection fail!\n");
	} else {
		hwlog_info("abnormal_cc_detection = %d \n", abnormal_cc_detection);
	}
	if (of_property_read_u32(di->dev->of_node, "abnormal_cc_interval", &abnormal_cc_interval)) {
		hwlog_err("get abnormal_cc_interval fail!\n");
		abnormal_cc_interval = PD_DPM_CC_CHANGE_INTERVAL;
	}
	hwlog_info("abnormal_cc_interval= %d \n", abnormal_cc_interval);

	if (abnormal_cc_detection)
		init_fb_notification();

	//adjust the typec  product
        typec_class = class_create(THIS_MODULE, "hw_typec");
        if (IS_ERR(typec_class)) {
                hwlog_err("%s: cannot create class\n", __func__);
                return PTR_ERR(typec_class);
        }

	if (typec_class) {
		typec_dev = device_create(typec_class, NULL, 0, NULL, "typec");
		ret = sysfs_create_group(&typec_dev->kobj, &pd_dpm_attr_group);
		if (ret) {
			hwlog_err("%s: typec sysfs group create error\n", __func__);
		}
	}

	di->last_usb_event = PD_DPM_USB_TYPEC_NONE;
	di->pending_usb_event = PD_DPM_USB_TYPEC_NONE;

	mutex_init(&di->usb_lock);
	mutex_init(&typec_state_lock);
	mutex_init(&typec_wait_lock);
#ifdef CONFIG_CONTEXTHUB_PD
	mutex_init(&di->pd_combphy_notify_lock);

	di->pd_combphy_wq = create_workqueue("pd_combphy_event_notify_workque");
	INIT_DELAYED_WORK(&di->pd_combphy_event_work,
		pd_dpm_combphy_event_notify);
#endif
	di->usb_wq = create_workqueue("pd_dpm_usb_wq");
	INIT_DELAYED_WORK(&di->usb_state_update_work,
		pd_dpm_usb_update_state);

	platform_set_drvdata(pdev, di);

	pd_dpm_parse_dt(di, &pdev->dev);
	notify_tcp_dev_ready(di->tcpc_name);

	type = hisi_get_charger_type();

	hwlog_info("%s:  bc12 type = %d \n", __func__, type);

	init_completion(&pd_get_typec_state_completion);
#ifdef CONFIG_CONTEXTHUB_PD
	init_completion(&pd_dpm_combphy_configdone_completion);
#endif
	di->bc12_event = type;
	cable_detect_ops_register(&cable_detect_ops);
	hwlog_info("%s - probe ok\n", __func__);
	return ret;
}
EXPORT_SYMBOL_GPL(pd_dpm_probe);

static const struct of_device_id pd_dpm_callback_match_table[] = {
	{.compatible = "huawei,pd_dpm",},
	{},
};

static struct platform_driver pd_dpm_callback_driver = {
	.probe		= pd_dpm_probe,
	.remove		= NULL,
	.driver		= {
		.name	= "huawei,pd_dpm",
		.owner	= THIS_MODULE,
		.of_match_table = pd_dpm_callback_match_table,
	}
};

static int __init pd_dpm_init(void)
{
	hwlog_info("%s \n", __func__);

	return platform_driver_register(&pd_dpm_callback_driver);
}

static void __exit pd_dpm_exit(void)
{
	platform_driver_unregister(&pd_dpm_callback_driver);
}

module_init(pd_dpm_init);
module_exit(pd_dpm_exit);
MODULE_LICENSE("GPL v2");
MODULE_DESCRIPTION("huawei pd dpm");
MODULE_AUTHOR("SuoAnDaJie<suoandajie@huawei.com>");
