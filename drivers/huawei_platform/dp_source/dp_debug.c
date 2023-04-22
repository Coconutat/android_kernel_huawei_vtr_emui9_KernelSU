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
#include <huawei_platform/dp_source/dp_dsm.h>
#include <huawei_platform/dp_source/dp_factory.h>
#include <huawei_platform/dp_source/dp_debug.h>

#define HWLOG_TAG dp_debug
HWLOG_REGIST();

#ifndef DP_DEBUG_ENABLE // not defined
void dp_debug_init_combophy_pree_swing(uint32_t *pv, int count)
{
	//hwlog_info("%s: init skip!!!\n", __func__);
}
EXPORT_SYMBOL_GPL(dp_debug_init_combophy_pree_swing);

int dp_debug_append_info(char *buf, int size, char *fmt, ...)
{
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_append_info);

int dp_debug_get_vs_pe_force(uint8_t *vs_force, uint8_t *pe_force)
{
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(dp_debug_get_vs_pe_force);

int dp_debug_get_lanes_rate_force(uint8_t *lanes_force, uint8_t *rate_force)
{
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(dp_debug_get_lanes_rate_force);

int dp_debug_get_resolution_force(uint8_t *user_mode, uint8_t *user_format)
{
	return -EINVAL;
}
EXPORT_SYMBOL_GPL(dp_debug_get_resolution_force);

#else // DP_DEBUG_ENABLE defined

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#define DP_DEBUG_BUF_SIZE (PAGE_SIZE) // max: 4096 bytes

// dp pe and vs params from dts
#define DP_COMBOPHY_PREE_SWING_INDENT (4)

typedef enum {
	DP_DEBUG_PARAMS_NOT_SET,
	DP_DEBUG_PARAMS_VALID,
	DP_DEBUG_PARAMS_INVALID,
} dp_debug_params_state_t;

typedef struct {
	uint8_t vs_force;
	uint8_t pe_force;

	uint8_t lanes_force;
	uint8_t rate_force;

	uint32_t user_mode;
	uint32_t user_format;

	dp_debug_params_state_t vs_pe_state;
	dp_debug_params_state_t lanes_rate_state;
	dp_debug_params_state_t resolution_state;

	// from hisi dp driver
	uint32_t *combophy_pree_swing;
	int combophy_pree_swing_count;
} dp_debug_priv_t;

static dp_debug_priv_t *g_dp_debug_priv = NULL;

void dp_debug_init_combophy_pree_swing(uint32_t *pv, int count)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;

	if (NULL == priv) {
		hwlog_err("%s: priv is NULL!!!\n", __func__);
		return;
	}

	priv->combophy_pree_swing = pv;
	priv->combophy_pree_swing_count = count;
	hwlog_info("%s: init success(%d).\n", __func__, count);
}
EXPORT_SYMBOL_GPL(dp_debug_init_combophy_pree_swing);

int dp_debug_append_info(char *buf, int size, char *fmt, ...)
{
	char *append = NULL;
	va_list args;

	if ((NULL == buf) || (NULL == fmt)) {
		hwlog_err("%s: buf or fmt is NULL!!!\n", __func__);
		return -EINVAL;
	}

	append = (char *)kzalloc(sizeof(char) * size, GFP_KERNEL);
	if (NULL == append) {
		hwlog_err("%s: append is NULL!!!\n", __func__);
		return -ENOMEM;
	}

	va_start(args, fmt);
	vscnprintf(append, size, (const char *)fmt, args);
	va_end(args);

	strncat(buf, append, size - strlen(buf) - 1);
	kfree(append);
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_append_info);

static int dp_debug_pd_event_get(char *buffer, const struct kernel_param *kp)
{
	UNUSED(kp);
	return dp_get_pd_event_result(buffer, DP_DEBUG_BUF_SIZE);
}

static int dp_debug_hotplug_get(char *buffer, const struct kernel_param *kp)
{
	UNUSED(kp);
	return dp_get_hotplug_result(buffer, DP_DEBUG_BUF_SIZE);
}

static int dp_debug_timing_get(char *buffer, const struct kernel_param *kp)
{
	UNUSED(kp);
	return dp_get_timing_info_result(buffer, DP_DEBUG_BUF_SIZE);
}

static int dp_debug_vs_pe_force_get(char *buffer, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == buffer)) {
		hwlog_err("%s: priv or buffer is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (DP_DEBUG_PARAMS_INVALID == priv->vs_pe_state) {
		priv->vs_pe_state = DP_DEBUG_PARAMS_NOT_SET;
		return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "invalid vs_force or pe_force!\n");
	}

	if ((DP_DSM_VS_PE_MASK == priv->vs_force) || (DP_DSM_VS_PE_MASK == priv->pe_force )) {
		return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "vs_force and pe_force not set!\n");
	}

	return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "vs_force: %d, pe_force: %d\n",
		priv->vs_force, priv->pe_force);
}

static bool dp_debug_is_vs_pe_valid(int vs_force, int pe_force)
{
	int vs_pe_valid[DP_DSM_VS_PE_NUM * 2] = {
			// vs level, pe level max
			0,           3,
			1,           2,
			2,           1,
			3,           0 };
	int i = 0;

	for (i = 0; i < DP_DSM_VS_PE_NUM * 2; i += 2) {
		if (vs_pe_valid[i] == vs_force) {
			if (pe_force <= vs_pe_valid[i + 1]) {
				return true;
			} else {
				return false;
			}
		}
	}

	return false;
}

static int dp_debug_vs_pe_force_set(const char *val, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;
	int vs_force = 0;
	int pe_force = 0;
	int ret = 0;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == val)) {
		hwlog_err("%s: priv or val is NULL!!!\n", __func__);
		return -EINVAL;
	}
	priv->vs_pe_state = DP_DEBUG_PARAMS_INVALID;

	ret = sscanf(val, "0x%x 0x%x", &vs_force, &pe_force);
	if (ret != 2) {
		hwlog_err("%s: invalid params num %d!!!\n", __func__, ret);
		return -EINVAL;
	}
	hwlog_info("%s: vs_force = %d, pe_force = %d\n", __func__, vs_force, pe_force);

	if ((DP_DSM_VS_PE_MASK == vs_force) && (DP_DSM_VS_PE_MASK == pe_force )) {
		priv->vs_force = DP_DSM_VS_PE_MASK;
		priv->pe_force = DP_DSM_VS_PE_MASK;
		hwlog_info("%s: clear vs_force and pe_force!!!\n", __func__);
	} else if (!dp_debug_is_vs_pe_valid(vs_force, pe_force)) {
		hwlog_err("%s: invalid vs_force %d or pe_force %d!!!\n", __func__, vs_force, pe_force);
		return -EINVAL;
	} else {
		priv->vs_force = (uint8_t)vs_force;
		priv->pe_force = (uint8_t)pe_force;
	}

	priv->vs_pe_state = DP_DEBUG_PARAMS_VALID;
	return 0;
}

int dp_debug_get_vs_pe_force(uint8_t *vs_force, uint8_t *pe_force)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;

	if ((NULL == priv) || (NULL == vs_force) || (NULL == pe_force)) {
		hwlog_err("%s: priv, vs_force or pe_force is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if ((DP_DSM_VS_PE_MASK == priv->vs_force) || (DP_DSM_VS_PE_MASK == priv->pe_force )) {
		return -EINVAL;
	}

	*vs_force = priv->vs_force;
	*pe_force = priv->pe_force;
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_get_vs_pe_force);

static int dp_debug_vs_pe_get(char *buffer, const struct kernel_param *kp)
{
	UNUSED(kp);
	return dp_get_vs_pe_result(buffer, DP_DEBUG_BUF_SIZE);
}

static int dp_debug_lanes_rate_force_get(char *buffer, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == buffer)) {
		hwlog_err("%s: priv or buffer is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (DP_DEBUG_PARAMS_INVALID == priv->lanes_rate_state) {
		priv->lanes_rate_state = DP_DEBUG_PARAMS_NOT_SET;
		return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "invalid lanes_force or rate_force!\n");
	}

	if ((DP_LINK_LANES_MASK == priv->lanes_force) || (DP_LINK_RATE_MASK == priv->rate_force )) {
		return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "lanes_force and rate_force not set!\n");
	}

	return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "lanes_force: %d, rate_force: %d\n",
		priv->lanes_force, priv->rate_force);

}

static int dp_debug_lanes_rate_force_set(const char *val, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;
	int lanes_force = 0;
	int rate_force  = 0;
	int ret = 0;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == val)) {
		hwlog_err("%s: priv or val is NULL!!!\n", __func__);
		return -EINVAL;
	}
	priv->lanes_rate_state = DP_DEBUG_PARAMS_INVALID;

	ret = sscanf(val, "0x%x 0x%x", &lanes_force, &rate_force);
	if (ret != 2) {
		hwlog_err("%s: invalid params num %d!!!\n", __func__, ret);
		return -EINVAL;
	}
	hwlog_info("%s: lanes_force = %d, rate_force = %d\n", __func__, lanes_force, rate_force);

	if ((DP_LINK_LANES_MASK == lanes_force) && (DP_LINK_RATE_MASK == rate_force )) {
		priv->lanes_force = DP_LINK_LANES_MASK;
		priv->rate_force  = DP_LINK_RATE_MASK;
		hwlog_info("%s: clear lanes_force and rate_force!!!\n", __func__);
	} else if (((lanes_force != DP_LINK_LANES_1) && (lanes_force != DP_LINK_LANES_2)
		&& (lanes_force != DP_LINK_LANES_4)) || (rate_force >= DP_LINK_RATE_MAX)) {
		hwlog_err("%s: invalid lanes_force %d or rate_force %d!!!\n", __func__, lanes_force, rate_force);
		return -EINVAL;
	} else {
		priv->lanes_force = (uint8_t)lanes_force;
		priv->rate_force  = (uint8_t)rate_force;
	}

	priv->lanes_rate_state = DP_DEBUG_PARAMS_VALID;
	return 0;
}

int dp_debug_get_lanes_rate_force(uint8_t *lanes_force, uint8_t *rate_force)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;

	if ((NULL == priv) || (NULL == lanes_force) || (NULL == rate_force)) {
		hwlog_err("%s: priv, lanes_force or rate_force is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if ((DP_LINK_LANES_MASK == priv->lanes_force) || (DP_LINK_RATE_MASK == priv->rate_force )) {
		return -EINVAL;
	}

	*lanes_force = priv->lanes_force;
	*rate_force = priv->rate_force;
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_get_lanes_rate_force);

static int dp_debug_resolution_force_get(char *buffer, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == buffer)) {
		hwlog_err("%s: priv or buffer is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (DP_DEBUG_PARAMS_INVALID == priv->resolution_state) {
		priv->resolution_state = DP_DEBUG_PARAMS_NOT_SET;
		return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "invalid user_mode or user_format!\n");
	}

	if (DP_VIDEO_MODE_MASK == priv->user_mode) {
		return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "user_mode and user_format not set!\n");
	}

	return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "user_mode: %d, user_format: %d\n",
		priv->user_mode, priv->user_format);
}

static int dp_debug_resolution_force_set(const char *val, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;
	uint32_t user_mode = 0;
	uint32_t user_format = 0;
	int ret = 0;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == val)) {
		hwlog_err("%s: priv or val is NULL!!!\n", __func__);
		return -EINVAL;
	}
	priv->resolution_state = DP_DEBUG_PARAMS_INVALID;

	ret = sscanf(val, "0x%x 0x%x", &user_mode, &user_format);
	if (ret != 2) {
		hwlog_err("%s: invalid params num %d!!!\n", __func__, ret);
		return -EINVAL;
	}
	hwlog_info("%s: user_mode = %d, user_format = %d\n", __func__, user_mode, user_format);

	if (user_mode == DP_VIDEO_MODE_MASK) {
		priv->user_mode   = DP_VIDEO_MODE_MASK;
		priv->user_format = 0;
		hwlog_info("%s: clear user_mode and user_format!!!\n", __func__);
	} else if ((user_mode > DP_VIDEO_MODE_MASK) || (user_format >= DP_VIDEO_FMT_MAX)) {
		hwlog_err("%s: invalid user_mode %d or user_format %d!!!\n", __func__, user_mode, user_format);
		return -EINVAL;
	} else {
		priv->user_mode   = user_mode;
		priv->user_format = user_format;
	}

	priv->resolution_state = DP_DEBUG_PARAMS_VALID;
	return 0;
}

int dp_debug_get_resolution_force(uint8_t *user_mode, uint8_t *user_format)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;

	if ((NULL == priv) || (NULL == user_mode) || (NULL == user_format)) {
		hwlog_err("%s: priv, user_mode or user_format is NULL!!!\n", __func__);
		return -EINVAL;
	}

	if (DP_VIDEO_MODE_MASK == priv->user_mode) {
		return -EINVAL;
	}

	*user_mode   = (uint8_t)priv->user_mode;
	*user_format = (uint8_t)priv->user_format;
	return 0;
}
EXPORT_SYMBOL_GPL(dp_debug_get_resolution_force);

static int dp_debug_preemphasis_swing_get(char *buffer, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;
	int i = 0;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == priv->combophy_pree_swing) || (NULL == buffer)) {
		hwlog_err("%s: priv or buffer is NULL!!!\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < priv->combophy_pree_swing_count; i++) {
		if (0 == (i % DP_COMBOPHY_PREE_SWING_INDENT)) {
			dp_debug_append_info(buffer, DP_DEBUG_BUF_SIZE, "\n[%02d]: 0x%08x,", i, priv->combophy_pree_swing[i]);
		} else {
			dp_debug_append_info(buffer, DP_DEBUG_BUF_SIZE, " 0x%08x,", priv->combophy_pree_swing[i]);
		}
	}

	return strlen(buffer);
}

static int dp_debug_preemphasis_swing_set(const char *val, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;
	uint32_t pv = 0;
	int index = 0;
	int ret = 0;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == val)) {
		hwlog_err("%s: priv or val is NULL!!!\n", __func__);
		return -EINVAL;
	}

	ret = sscanf(val, "%d 0x%x", &index, &pv);
	if (ret != 2) {
		hwlog_err("%s: invalid params %d!!!\n", __func__, ret);
		return -EINVAL;
	}

	if (index >= priv->combophy_pree_swing_count) {
		hwlog_err("%s: invalid index %d!!!\n", __func__, index);
		return -EINVAL;
	}

	priv->combophy_pree_swing[index] = pv;
	hwlog_info("%s: index = %d, pv = 0x%08x\n", __func__, index, pv);

	return 0;
}

static int dp_debug_factory_mode_get(char *buffer, const struct kernel_param *kp)
{
	UNUSED(kp);
	if (NULL == buffer) {
		hwlog_err("%s: buffer is NULL!!!\n", __func__);
		return -EINVAL;
	}

	return snprintf(buffer, (unsigned long)DP_DEBUG_BUF_SIZE, "factory_mode %s\n",
		dp_factory_mode_is_enable() ? "enable" : "disable");
}

static int dp_debug_factory_test_get(char *buffer, const struct kernel_param *kp)
{
	UNUSED(kp);
	return dp_factory_get_test_result(buffer, DP_DEBUG_BUF_SIZE);
}

static int dp_debug_factory_test_set(const char *val, const struct kernel_param *kp)
{
	dp_debug_priv_t *priv = g_dp_debug_priv;
	int test_enable = 0;
	int ret = 0;

	UNUSED(kp);
	if ((NULL == priv) || (NULL == val)) {
		hwlog_err("%s: priv or val is NULL!!!\n", __func__);
		return -EINVAL;
	}

	ret = sscanf(val, "%d", &test_enable);
	if (ret != 1) {
		hwlog_err("%s: invalid params num %d!!!\n", __func__, ret);
		return -EINVAL;
	}

	dp_factory_set_mmie_test_flag(test_enable);
	return 0;
}

static struct kernel_param_ops dp_debug_pd_event_ops = {
	.get = dp_debug_pd_event_get,
};

static struct kernel_param_ops dp_debug_hotplug_ops = {
	.get = dp_debug_hotplug_get,
};

static struct kernel_param_ops dp_debug_timing_ops = {
	.get = dp_debug_timing_get,
};

static struct kernel_param_ops dp_debug_vs_pe_force_ops = {
	.get = dp_debug_vs_pe_force_get,
	.set = dp_debug_vs_pe_force_set,
};

static struct kernel_param_ops dp_debug_vs_pe_ops = {
	.get = dp_debug_vs_pe_get,
};

static struct kernel_param_ops dp_debug_lanes_rate_force_ops = {
	.get = dp_debug_lanes_rate_force_get,
	.set = dp_debug_lanes_rate_force_set,
};

static struct kernel_param_ops dp_debug_resolution_force_ops = {
	.get = dp_debug_resolution_force_get,
	.set = dp_debug_resolution_force_set,
};

static struct kernel_param_ops dp_debug_preemphasis_swing_ops = {
	.get = dp_debug_preemphasis_swing_get,
	.set = dp_debug_preemphasis_swing_set,
};

static struct kernel_param_ops dp_debug_factory_mode_ops = {
	.get = dp_debug_factory_mode_get,
};

static struct kernel_param_ops dp_debug_factory_test_ops = {
	.get = dp_debug_factory_test_get,
	.set = dp_debug_factory_test_set,
};

// dp connnect info
module_param_cb(pd_event, &dp_debug_pd_event_ops, NULL, 0444);
module_param_cb(hotplug, &dp_debug_hotplug_ops, NULL, 0444);
module_param_cb(timing, &dp_debug_timing_ops, NULL, 0444);
module_param_cb(vs_pe_force, &dp_debug_vs_pe_force_ops, NULL, 0644);
module_param_cb(vs_pe, &dp_debug_vs_pe_ops, NULL, 0444);
module_param_cb(lanes_rate_force, &dp_debug_lanes_rate_force_ops, NULL, 0644);
module_param_cb(resolution_force, &dp_debug_resolution_force_ops, NULL, 0644);

// dp preemphasis_swing params debug for hw test
module_param_cb(preemphasis_swing, &dp_debug_preemphasis_swing_ops, NULL, 0644);

// factory test
module_param_cb(factory_mode, &dp_debug_factory_mode_ops, NULL, 0444);
module_param_cb(factory_test, &dp_debug_factory_test_ops, NULL, 0644);

static void dp_debug_release(dp_debug_priv_t *priv)
{
	if (priv != NULL) {
		kfree(priv);
	}
}

static int __init dp_debug_init(void)
{
	dp_debug_priv_t *priv = NULL;
	int ret = 0;

	hwlog_info("%s: enter...\n", __func__);
	priv = (dp_debug_priv_t *)kzalloc(sizeof(dp_debug_priv_t), GFP_KERNEL);
	if (priv == NULL) {
		hwlog_err("%s: kzalloc priv failed!!!\n", __func__);
		ret = -ENOMEM;
		goto err_out;
	}

	priv->vs_force    = DP_DSM_VS_PE_MASK;
	priv->pe_force    = DP_DSM_VS_PE_MASK;
	priv->lanes_force = DP_LINK_LANES_MASK;
	priv->rate_force  = DP_LINK_RATE_MASK;
	priv->user_mode   = DP_VIDEO_MODE_MASK;
	priv->user_format = 0;

	priv->vs_pe_state      = DP_DEBUG_PARAMS_NOT_SET;
	priv->lanes_rate_state = DP_DEBUG_PARAMS_NOT_SET;
	priv->resolution_state = DP_DEBUG_PARAMS_NOT_SET;

	g_dp_debug_priv = priv;
	hwlog_info("%s: init success!!!\n", __func__);
	return 0;

err_out:
	dp_debug_release(priv);
	return ret;
}

static void __exit dp_debug_exit(void)
{
	hwlog_info("%s: enter...\n", __func__);
	dp_debug_release(g_dp_debug_priv);
	g_dp_debug_priv = NULL;
}

module_init(dp_debug_init);
module_exit(dp_debug_exit);

#endif // DP_DEBUG_ENABLE end.

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("Huawei dp debug driver");
MODULE_AUTHOR("<wangping48@huawei.com>");

