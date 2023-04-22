/* Copyright (c) 2008-2019, Huawei Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
* GNU General Public License for more details.
*
*/
#include <linux/module.h>
#include <linux/init.h>
#include <linux/regmap.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/kobject.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/dp_source/dp_factory.h>
#include <huawei_platform/dp_source/dp_dsm.h>

#define HWLOG_TAG dp_factory
HWLOG_REGIST();

#ifndef DP_FACTORY_MODE_ENABLE // not defined
bool dp_factory_mode_is_enable(void)
{
	//hwlog_info("factory_mode disable!\n");
	return false;
}
EXPORT_SYMBOL_GPL(dp_factory_mode_is_enable);

void dp_factory_link_cr_or_ch_eq_fail(bool is_cr)
{
}
EXPORT_SYMBOL_GPL(dp_factory_link_cr_or_ch_eq_fail);

bool dp_factory_is_4k_60fps(uint8_t rate, uint8_t lanes, uint16_t h_active, uint16_t v_active, uint8_t fps)
{
	// not check 4k@60fps
	hwlog_info("%s: not check 4k@60fps, skip!\n", __func__);
	return true;
}
EXPORT_SYMBOL_GPL(dp_factory_is_4k_60fps);

int dp_factory_get_test_result(char *buffer, int size)
{
	if ((buffer != NULL) && (size > 0)) {
		return snprintf(buffer, (unsigned long)size, "not support factory test!\n");
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dp_factory_get_test_result);

bool dp_factory_need_report_event(void)
{
	return true;
}
EXPORT_SYMBOL_GPL(dp_factory_need_report_event);

void dp_factory_set_link_event_no(uint32_t event_no, bool cablein, char *event, int hotplug)
{
}
EXPORT_SYMBOL_GPL(dp_factory_set_link_event_no);

void dp_factory_set_link_rate_lanes(uint8_t rate, uint8_t lanes, uint8_t sink_rate, uint8_t sink_lanes)
{
}
EXPORT_SYMBOL_GPL(dp_factory_set_link_rate_lanes);

void dp_factory_set_mmie_test_flag(bool test_enable)
{
}
EXPORT_SYMBOL_GPL(dp_factory_set_mmie_test_flag);

#else // DP_FACTORY_MODE_ENABLE defined

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif

#define DP_FACTORY_RATE_HBR2 (2)
#define DP_FACTORY_LANES     (4)
#define DP_FACTORY_H_ACTIVE  (3840)
#define DP_FACTORY_V_ACTIVE  (2160)
#define DP_FACTORY_FPS       (60)

#define DP_FACTORY_CHECK_RET(a, b) (a), (((a) >= (b)) ? "ok" : "error")

typedef enum {
	DP_FACTORY_LINK_SUCC,
	DP_FACTORY_LINK_CR_FAILED,
	DP_FACTORY_LINK_CH_EQ_FAILED,
} dp_factory_link_status_t;

typedef struct
{
	int hotplug_retval;
	uint32_t link_event_no;
	char link_event[DP_LINK_EVENT_BUF_MAX * DP_LINK_STATE_MAX];

	bool test_enable;         // whether or not to check external combinations
	bool check_lanes_rate;    // check 4lanes+hbr2
	bool check_display_4k;    // check 4k, >=3840*2160
	bool check_display_60fps; // check 60fps, FPS: Frames Per Second
	bool need_report_event;   // new program in MMIE

	// according to typec cable: dock or dongle
	uint8_t max_rate;
	uint8_t max_lanes;
	// according to sink devices
	uint8_t sink_rate;
	uint8_t sink_lanes;
	// actual link rate and lanes
	uint8_t link_rate;
	uint8_t link_lanes;
	uint16_t h_active;
	uint16_t v_active;
	uint8_t fps;

	dp_factory_link_status_t link_status;
} dp_factory_priv_t;

dp_factory_priv_t *g_dp_factory_priv = NULL;

#define DP_FACTORY_GET_DTS_PROP(p, n, c) \
	do { \
		if (of_property_read_bool(n, #c)) { \
			p->c = true; \
			hwlog_info("%s: need to %s.\n", __func__, #c); \
		} else { \
			hwlog_info("%s: not need to %s.\n", __func__, #c); \
		} \
	} while(0)

bool dp_factory_mode_is_enable(void)
{
	hwlog_info("factory_mode enable!\n");
	return true;
}
EXPORT_SYMBOL_GPL(dp_factory_mode_is_enable);

void dp_factory_link_cr_or_ch_eq_fail(bool is_cr)
{
	dp_factory_priv_t *priv = g_dp_factory_priv;

	if (priv != NULL) {
		if (is_cr) {
			priv->link_status = DP_FACTORY_LINK_CR_FAILED;
		} else {
			priv->link_status = DP_FACTORY_LINK_CH_EQ_FAILED;
		}
		dp_set_hotplug_state(DP_LINK_STATE_LINK_REDUCE_RATE);
	}
}
EXPORT_SYMBOL_GPL(dp_factory_link_cr_or_ch_eq_fail);

bool dp_factory_is_4k_60fps(uint8_t rate, uint8_t lanes, uint16_t h_active, uint16_t v_active, uint8_t fps)
{
	dp_factory_priv_t *priv = g_dp_factory_priv;

	if (priv == NULL) {
		hwlog_err("%s: priv == NULL!\n", __func__);
		return true;
	}

	priv->max_rate = rate;
	priv->max_lanes = lanes;
	priv->h_active = h_active;
	priv->v_active = v_active;
	priv->fps = fps;

	rate = MIN(rate, priv->link_rate);
	lanes = MIN(lanes, priv->link_lanes);

	if ((priv->check_lanes_rate && (rate < DP_FACTORY_RATE_HBR2))
		|| (priv->check_lanes_rate && (lanes < DP_FACTORY_LANES))
		|| (priv->check_display_4k && (h_active < DP_FACTORY_H_ACTIVE))
		|| (priv->check_display_4k && (v_active < DP_FACTORY_V_ACTIVE))
		|| (priv->check_display_60fps && (fps < DP_FACTORY_FPS))) {
		hwlog_info("current link config, rate=%u/%u/%u, lanes=%u/%u/%u, h_active=%u, v_active=%u, fps=%u.\n",
			priv->max_rate, priv->sink_rate, priv->link_rate,
			priv->max_lanes, priv->sink_lanes, priv->link_lanes,
			h_active, v_active, fps);

		// return false: need to check external connection combinations
		// return true: not need to check
		if (priv->test_enable) {
			dp_set_hotplug_state(DP_LINK_STATE_INVALID_COMBINATIONS);
			return false;
		} else {
			return true;
		}
	}

	hwlog_info("current link config, rate=%u, lanes=%u, h_active=%u, v_active=%u, fps=%u.\n",
		rate, lanes, h_active, v_active, fps);
	return true;
}
EXPORT_SYMBOL_GPL(dp_factory_is_4k_60fps);

static bool dp_factory_check_test_result(dp_factory_priv_t *priv)
{
	if (priv != NULL) {
		// 1. check_lanes_rate + need_report_event
		// 2. test_enable(debug): disable
		// 3. hotplug success
		// Here, MMIE test failed by link event(safe_mode or edid_failed)
		if ((priv->check_lanes_rate || priv->check_display_4k || priv->check_display_60fps)
			&& priv->need_report_event && !priv->test_enable && (0 == priv->hotplug_retval)) {
			if (priv->link_event_no & ((1 << DP_LINK_STATE_SAFE_MODE) | (1 << DP_LINK_STATE_EDID_FAILED))) {
				return false;
			}
		}

		return (0 == priv->hotplug_retval);
	}

	return false;
}

int dp_factory_get_test_result(char *buffer, int size)
{
	dp_factory_priv_t *priv = g_dp_factory_priv;

	if ((priv != NULL) && (buffer != NULL) && (size > 0)) {
		if (DP_FACTORY_LINK_CR_FAILED == priv->link_status) {
			return snprintf(buffer, (unsigned long)size, "test failed: 0x%x %s\nlink cr failed!\n",
				priv->link_event_no, priv->link_event);
		}

		if (DP_FACTORY_LINK_CH_EQ_FAILED == priv->link_status) {
			return snprintf(buffer, (unsigned long)size, "test failed: 0x%x %s\nlink ch_eq failed!\n",
				priv->link_event_no, priv->link_event);
		}

		if ((0 == priv->link_rate) && (0 == priv->link_lanes) && (0 == priv->hotplug_retval)) {
			return snprintf(buffer, (unsigned long)size, "Not yet tested!\n");
		}

		return snprintf(buffer, (unsigned long)size,
			"check external combinations: %s\ntest %s: 0x%x %s\nsrc_rate=%u[%s]\nsrc_lanes=%u[%s]\nsink_rate=%u[%s]\nsink_lanes=%u[%s]\n*link_rate=%u[%s]\n*link_lanes=%u[%s]\nh_active=%u[%s]\nv_active=%u[%s]\nfps=%u[%s]\n",
			priv->test_enable ? "enable" : "disable",
			dp_factory_check_test_result(priv) ? "success" : "failed",
			priv->link_event_no, priv->link_event,
			DP_FACTORY_CHECK_RET(priv->max_rate, DP_FACTORY_RATE_HBR2),
			DP_FACTORY_CHECK_RET(priv->max_lanes, DP_FACTORY_LANES),
			DP_FACTORY_CHECK_RET(priv->sink_rate, DP_FACTORY_RATE_HBR2),
			DP_FACTORY_CHECK_RET(priv->sink_lanes, DP_FACTORY_LANES),
			DP_FACTORY_CHECK_RET(priv->link_rate, DP_FACTORY_RATE_HBR2),
			DP_FACTORY_CHECK_RET(priv->link_lanes, DP_FACTORY_LANES),
			DP_FACTORY_CHECK_RET(priv->h_active, DP_FACTORY_H_ACTIVE),
			DP_FACTORY_CHECK_RET(priv->v_active, DP_FACTORY_V_ACTIVE),
			DP_FACTORY_CHECK_RET(priv->fps, DP_FACTORY_FPS));
	}

	return 0;
}
EXPORT_SYMBOL_GPL(dp_factory_get_test_result);

bool dp_factory_need_report_event(void)
{
	return (g_dp_factory_priv != NULL) ? g_dp_factory_priv->need_report_event : false;
}
EXPORT_SYMBOL_GPL(dp_factory_need_report_event);

void dp_factory_set_link_event_no(uint32_t event_no, bool cablein, char *event, int hotplug)
{
	dp_factory_priv_t *priv = g_dp_factory_priv;

	if (priv != NULL) {
		if (event != NULL) {
			int len = strlen(event);
			int offset = strlen("MANUFACTURE_DP_LINK_EVENT=");

			if ((event_no > DP_LINK_STATE_CABLE_OUT) && (len > offset)) {
				if (0 == strlen(priv->link_event)) {
					strcpy(priv->link_event, event + offset);
				} else {
					strcat(priv->link_event, ",");
					strcat(priv->link_event, event + offset);
				}
			}
		} else if (cablein) {
			priv->hotplug_retval = 0;
			priv->link_event_no = 0;
			priv->max_rate = 0;
			priv->max_lanes = 0;
			priv->sink_rate = 0;
			priv->sink_lanes = 0;
			priv->link_rate = 0;
			priv->link_lanes = 0;
			priv->h_active = 0;
			priv->v_active = 0;
			priv->fps = 0;
			priv->link_status = DP_FACTORY_LINK_SUCC;
			memset(priv->link_event, 0, DP_LINK_EVENT_BUF_MAX * DP_LINK_STATE_MAX);
		} else {
			priv->hotplug_retval = hotplug;
			priv->link_event_no = event_no;
		}
	}
}
EXPORT_SYMBOL_GPL(dp_factory_set_link_event_no);

void dp_factory_set_link_rate_lanes(uint8_t rate, uint8_t lanes, uint8_t sink_rate, uint8_t sink_lanes)
{
	dp_factory_priv_t *priv = g_dp_factory_priv;

	if (priv != NULL) {
		priv->sink_rate = sink_rate;
		priv->sink_lanes = sink_lanes;
		priv->link_rate = rate;
		priv->link_lanes = lanes;
	}
}
EXPORT_SYMBOL_GPL(dp_factory_set_link_rate_lanes);

void dp_factory_set_mmie_test_flag(bool test_enable)
{
	dp_factory_priv_t *priv = g_dp_factory_priv;

	if (priv != NULL) {
		priv->test_enable = test_enable;
		hwlog_info("%s: mmie test %s.\n", __func__, test_enable ? "enable" : "disable");
	}
}
EXPORT_SYMBOL_GPL(dp_factory_set_mmie_test_flag);

static void dp_factory_parse_dts(dp_factory_priv_t *priv)
{
	struct device_node *np = NULL;

	if (priv == NULL) {
		hwlog_err("%s: priv == NULL!\n", __func__);
		return;
	}

	np = of_find_compatible_node(NULL, NULL, "huawei,dp_source_switch");
	if (NULL == np) {
		hwlog_info("%s: dts node dp_source_switch not existed.\n", __func__);
		return;
	}

	if (!of_device_is_available(np)) {
		hwlog_info("%s: dts node dp_source_switch not available.\n", __func__);
		return;
	}

	DP_FACTORY_GET_DTS_PROP(priv, np, check_lanes_rate);
	DP_FACTORY_GET_DTS_PROP(priv, np, check_display_4k);
	DP_FACTORY_GET_DTS_PROP(priv, np, check_display_60fps);
	DP_FACTORY_GET_DTS_PROP(priv, np, need_report_event);
}

static void dp_factory_release(dp_factory_priv_t *priv)
{
	if (priv != NULL) {
		kfree(priv);
	}
}

static int __init dp_factory_init(void)
{
	dp_factory_priv_t *priv = NULL;
	int ret = 0;

	hwlog_info("%s: enter...\n", __func__);
	priv = (dp_factory_priv_t *)kzalloc(sizeof(dp_factory_priv_t), GFP_KERNEL);
	if (priv == NULL) {
		hwlog_err("%s: kzalloc priv failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	priv->test_enable = true; // default: need to check lanes/rate/4k/60fps in factory version
	priv->check_lanes_rate = false;
	priv->check_display_4k = false;
	priv->check_display_60fps = false;
	priv->need_report_event = false;
	dp_factory_parse_dts(priv);

	g_dp_factory_priv = priv;
	hwlog_info("%s: init success!!!\n", __func__);
	return 0;

err_out:
	dp_factory_release(priv);
	return ret;
}

static void __exit dp_factory_exit(void)
{
	hwlog_info("%s: enter...\n", __func__);
	dp_factory_release(g_dp_factory_priv);
	g_dp_factory_priv = NULL;
}

module_init(dp_factory_init);
module_exit(dp_factory_exit);

#endif // DP_FACTORY_MODE_ENABLE end.

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei dp factory driver");
MODULE_AUTHOR("<wangping48@huawei.com>");

