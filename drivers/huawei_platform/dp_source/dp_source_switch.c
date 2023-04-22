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
#include <huawei_platform/dp_source/dp_factory.h>
#include <huawei_platform/dp_source/dp_dsm.h>
#include "dp_source_switch.h"

#ifndef UNUSED
#define UNUSED(x) ((void)(x))
#endif

#ifndef MIN
#define MIN(x,y)  ((x) < (y) ? (x) : (y))
#endif

extern int hisi_dptx_switch_source(uint32_t user_mode,  uint32_t user_format);
extern int hisi_dptx_main_panel_blank(bool bblank);

struct dp_source_data g_dp_data;

struct dev_flag_device{
    const char  *name;
    struct device  *dev;
    int  index;
};

typedef struct {
	dp_link_state_t state;
	char *event;
} dp_link_state_to_event_t;

// NOTE: need to be treated separately for normal or factory version.
static dp_link_state_to_event_t g_dp_link_event[] = {
#ifndef DP_FACTORY_MODE_ENABLE
	// for normal version
	{ DP_LINK_STATE_CABLE_IN,        "DP_LINK_EVENT=CABLE_IN" },
	{ DP_LINK_STATE_CABLE_OUT,       "DP_LINK_EVENT=CABLE_OUT" },
	{ DP_LINK_STATE_MULTI_HPD,       "DP_LINK_EVENT=MULTI_HPD" },
	{ DP_LINK_STATE_AUX_FAILED,      "DP_LINK_EVENT=AUX_FAILED" },
	{ DP_LINK_STATE_SAFE_MODE,       "DP_LINK_EVENT=SAFE_MODE" },
	{ DP_LINK_STATE_EDID_FAILED,     "DP_LINK_EVENT=EDID_FAILED" },
	{ DP_LINK_STATE_LINK_FAILED,     "DP_LINK_EVENT=LINK_FAILED" },
	{ DP_LINK_STATE_LINK_RETRAINING, "DP_LINK_EVENT=LINK_RETRAINING" },
	{ DP_LINK_STATE_HDCP_FAILED,     "DP_LINK_EVENT=HDCP_FAILED" },
#else
	// for factory version: MMIE test
	{ DP_LINK_STATE_CABLE_IN,             "MANUFACTURE_DP_LINK_EVENT=CABLE_IN" },
	{ DP_LINK_STATE_CABLE_OUT,            "MANUFACTURE_DP_LINK_EVENT=CABLE_OUT" },
	{ DP_LINK_STATE_AUX_FAILED,           "MANUFACTURE_DP_LINK_EVENT=AUX_FAILED" },
	{ DP_LINK_STATE_SAFE_MODE,            "MANUFACTURE_DP_LINK_EVENT=SAFE_MODE" },
	{ DP_LINK_STATE_EDID_FAILED,          "MANUFACTURE_DP_LINK_EVENT=EDID_FAILED" },
	{ DP_LINK_STATE_LINK_FAILED,          "MANUFACTURE_DP_LINK_EVENT=LINK_FAILED" },
	{ DP_LINK_STATE_HPD_NOT_EXISTED,      "MANUFACTURE_DP_LINK_EVENT=HPD_NOT_EXISTED" },
	{ DP_LINK_STATE_LINK_REDUCE_RATE,     "MANUFACTURE_DP_LINK_EVENT=LINK_REDUCE_RATE" },
	{ DP_LINK_STATE_INVALID_COMBINATIONS, "MANUFACTURE_DP_LINK_EVENT=INVALID_COMBINATIONS" },
#endif
};

static struct dev_flag_device dp_source = {
    .name = "source",
    .index = 0,
};

static struct dev_flag_device dp_resolution = {
    .name = "resolution",
    .index = 0,
};

static struct dev_flag_device dp_vr = {
    .name = "vr",
    .index = 0,
};

static struct dev_flag_device dp_external_info = {
    .name = "external_display",
    .index = 0,
};

static struct dev_flag_device dp_lcd_power = {
    .name = "power",
    .index = 0,
};

int set_external_display_type(uint32_t type)
{
	int ret = 0;
	if( type > EXT_UNDEF){
		printk("%s Do not support this type=%d,please check it\n",__func__, type);
		ret = EXT_UNDEF;
		return ret;
	}

	g_dp_data.m_resolution_type = (enum dp_resolution_type)type;
	return ret;
}
EXPORT_SYMBOL_GPL(set_external_display_type);

int update_external_display_timming_info(uint32_t width,uint32_t high,uint32_t fps)
{
	g_dp_data.m_externel_info.m_width = width;
	g_dp_data.m_externel_info.m_high = high;
	g_dp_data.m_externel_info.m_fps = fps;
	return 0;
}
EXPORT_SYMBOL_GPL(update_external_display_timming_info);

int get_current_dp_source_mode(void)
{
	int ret = 0;
	if(g_dp_data.m_parse_dts_flag == false){
		/*DTS is not parsed, need use DTS parsed default data*/
		ret = dp_source_mode_parse_dts();
		if( ret!=0 ){
			printk("%s parse dp source mode failed, using same source as default source mode!\n",__func__);
		}
	}

    return g_dp_data.m_source_mode;
}
EXPORT_SYMBOL_GPL(get_current_dp_source_mode);

static ssize_t dp_source_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = scnprintf(buf, PAGE_SIZE, "%d\n", g_dp_data.m_source_mode);
    return ret;
}

int dp_source_mode_parse_dts(void)
{
	int value = 0;
	int ret = 0;
	struct device_node *np = NULL;

	printk(KERN_INFO "%s: enter\n", __func__);
	if(g_dp_data.m_parse_dts_flag) {
		printk("%s DP source mode dts config has already parsed.\n",__func__);
		return ret;
	}
	g_dp_data.m_parse_dts_flag = true;

	np = of_find_compatible_node(NULL, NULL, DTS_DP_SOURCE_SWITCH);
	if (!np) {
		printk("NOT FOUND device node %s!\n", DTS_DP_SOURCE_SWITCH);
		/*the DTS node is used to set the defalut mode,If no node find will setting the defalut mode as SAME_SOURCE */
		g_dp_data.m_source_mode = SAME_SOURCE;
		return ret;
	}

	if (!of_device_is_available(np)) {
		printk(KERN_INFO "%s: dts node %s not available.\n", __func__, np->name);
		g_dp_data.m_source_mode = SAME_SOURCE;
		return ret;
	}

	ret = of_property_read_u32(np, DTS_DP_DEFAULT_SOURCE_MODE, &value);
	if (ret) {
		printk("%s,[%s] read fail, ret = %d.\n",__func__, DTS_DP_DEFAULT_SOURCE_MODE, ret);
		g_dp_data.m_source_mode = SAME_SOURCE;
		return ret;
	}

	if(value == 0) {
		g_dp_data.m_source_mode = DIFF_SOURCE;
	} else {
		g_dp_data.m_source_mode = SAME_SOURCE;
	}

	if (dp_factory_mode_is_enable()) {
		g_dp_data.m_source_mode = SAME_SOURCE;
		printk(KERN_INFO "%s: only support same source in factory version.\n", __func__);
	}

	printk("%s parse dp source mode = %d. parse DTS finished\n",__func__, g_dp_data.m_source_mode);
	return ret;
}

static ssize_t dp_source_mode_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
    int last_mode =0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

    last_mode = g_dp_data.m_source_mode;
    ret = sscanf(buf, "%d", (int *)(&g_dp_data.m_source_mode));
    if (ret != 1) {
        printk("%s: store error\n", __func__);
        return RETURN_ERR;
    }

    if ((g_dp_data.m_source_mode == SAME_SOURCE) || (g_dp_data.m_source_mode == DIFF_SOURCE)) {
        if(last_mode == g_dp_data.m_source_mode) {
            printk("%s: ,synchronous framewore source mode state g_source_mode=%d\n", __func__, g_dp_data.m_source_mode);
            return count;
        }
        printk("%s: store ok, g_source_mode=%d\n", __func__, g_dp_data.m_source_mode);
        dp_imonitor_set_param(DP_PARAM_SOURCE_SWITCH, &g_dp_data.m_source_mode);
        hisi_dptx_switch_source(0, 0);
    } else {
        printk("%s: invalid g_source_mode value, mode=%d.\n", __func__, g_dp_data.m_source_mode);
    }

    return count;
}

static ssize_t dp_source_resolution_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = scnprintf(buf, PAGE_SIZE, "%d\n", g_dp_data.m_resolution_type);
    return ret;
}

static ssize_t dp_source_resolution_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;
	int resolution_type = 0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}
	printk("%s : enter, data: %s \n", __func__, buf);

	ret = sscanf(buf, "%d", &resolution_type);
    if (ret != 1) {
        printk("%s: store error\n", __func__);
        return RETURN_ERR;
    }

	if (resolution_type >RESOLUTION_UNDEF){
		printk("%s Do not support this resolution_type=%d, please check it\n",__func__, resolution_type);
		//ret = RESOLUTION_UNDEF;
		goto err_out;
	}
	g_dp_data.m_resolution_type = resolution_type;
	switch(g_dp_data.m_resolution_type) {
		case RESOLUTION_640_480_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_640;
			g_dp_data.m_source_resolution.m_high= HIGH_480;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_1;
			break;
		case RESOLUTION_800_600_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_800;
			g_dp_data.m_source_resolution.m_high= HIGH_600;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_2;
			break;
		case RESOLUTION_800_600_75FPS:
			g_dp_data.m_source_resolution.m_with = WITH_800;
			g_dp_data.m_source_resolution.m_high= HIGH_600;
			g_dp_data.m_source_resolution.m_fps= FPS_75;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_11;
			break;
		case RESOLUTION_1024_768_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1024;
			g_dp_data.m_source_resolution.m_high= HIGH_768;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_3;
			break;
		case RESOLUTION_1280_768_60FPS_CVT:
			g_dp_data.m_source_resolution.m_with = WITH_1280;
			g_dp_data.m_source_resolution.m_high= HIGH_768;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_22;
			break;
		case RESOLUTION_1280_768_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1280;
			g_dp_data.m_source_resolution.m_high= HIGH_768;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_13;
			break;
		case RESOLUTION_1280_800_60FPS_CVT:
			g_dp_data.m_source_resolution.m_with = WITH_1280;
			g_dp_data.m_source_resolution.m_high= HIGH_800;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_27;
			break;
		case RESOLUTION_1280_800_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1280;
			g_dp_data.m_source_resolution.m_high= HIGH_800;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_28;
			break;
		case RESOLUTION_1280_960_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1280;
			g_dp_data.m_source_resolution.m_high= HIGH_960;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_4;
			break;
		case RESOLUTION_1280_1024_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1280;
			g_dp_data.m_source_resolution.m_high= HIGH_1024;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_12;
			break;
		case RESOLUTION_1360_768_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1360;
			g_dp_data.m_source_resolution.m_high= HIGH_768;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_17;
			break;
		case RESOLUTION_1400_1050_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1440;
			g_dp_data.m_source_resolution.m_high= HIGH_1050;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_41;
			break;
		case RESOLUTION_1600_1200_60FPS_CVT:
			g_dp_data.m_source_resolution.m_with = WITH_1600;
			g_dp_data.m_source_resolution.m_high= HIGH_1200;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_40;
			break;
		case RESOLUTION_1600_1200_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1600;
			g_dp_data.m_source_resolution.m_high= HIGH_1200;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_6;
			break;
		case RESOLUTION_1680_1050_60FPS_CVT:
			g_dp_data.m_source_resolution.m_with = WITH_1680;
			g_dp_data.m_source_resolution.m_high= HIGH_1050;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_57;
			break;
		case RESOLUTION_1680_1050_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1680;
			g_dp_data.m_source_resolution.m_high= HIGH_1050;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_58;
			break;
		case RESOLUTION_1792_1344_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1792;
			g_dp_data.m_source_resolution.m_high= HIGH_1344;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_62;
			break;
		case RESOLUTION_1920_1080_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1920;
			g_dp_data.m_source_resolution.m_high= HIGH_1080;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_20;
			break;
		case RESOLUTION_1920_1200_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1920;
			g_dp_data.m_source_resolution.m_high= HIGH_1200;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_34;
			break;
		case RESOLUTION_2048_1536_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_2048;
			g_dp_data.m_source_resolution.m_high= HIGH_1536;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = CVT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_41;
			break;
		case RESOLUTION_2560_1600_60FPS_CVT:
			g_dp_data.m_source_resolution.m_with = WITH_2560;
			g_dp_data.m_source_resolution.m_high= HIGH_1600;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_76;
			break;
		case RESOLUTION_2560_1600_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_2560;
			g_dp_data.m_source_resolution.m_high= HIGH_1600;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = DMT;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_77;
			break;
		case RESOLUTION_1440_480_59FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1440;
			g_dp_data.m_source_resolution.m_high= HIGH_480;
			g_dp_data.m_source_resolution.m_fps= FPS_59;
			g_dp_data.m_video_format_type = VCEA;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_15;
			break;
		case RESOLUTION_1440_576_50FPS:
			g_dp_data.m_source_resolution.m_with = WITH_1440;
			g_dp_data.m_source_resolution.m_high= HIGH_576;
			g_dp_data.m_source_resolution.m_fps= FPS_50;
			g_dp_data.m_video_format_type = VCEA;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_30;
			break;
		case RESOLUTION_3840_2160_60FPS:
			g_dp_data.m_source_resolution.m_with = WITH_3840;
			g_dp_data.m_source_resolution.m_high= HIGH_2160;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = VCEA;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_97;
			break;
		case RESOLUTION_1920_1080_60FPS_CEA:
			g_dp_data.m_source_resolution.m_with = WITH_1920;
			g_dp_data.m_source_resolution.m_high= HIGH_1080;
			g_dp_data.m_source_resolution.m_fps= FPS_60;
			g_dp_data.m_video_format_type = VCEA;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_16;
			break;
		case RESOLUTION_3840_2160_30FPS:
			g_dp_data.m_source_resolution.m_with = WITH_3840;
			g_dp_data.m_source_resolution.m_high= HIGH_2160;
			g_dp_data.m_source_resolution.m_fps= FPS_30;
			g_dp_data.m_video_format_type = VCEA;
			g_dp_data.m_source_resolution.m_vesa_id= VESA_ID_95;
			break;
		default:
		printk("%s : Do not support this source resolution\n", __func__);
		goto err_out;
	}

	ret = hisi_dptx_switch_source(g_dp_data.m_source_resolution.m_vesa_id, (int)g_dp_data.m_video_format_type);
	if(ret){
		printk("%s : Do dptx switch source failed!\n", __func__);
	}

err_out:
	printk("%s : m_with=%d, m_high=%d, m_fps=%d, m_video_format_type=%d, m_vesa_id=%d \n", __func__,
		g_dp_data.m_source_resolution.m_with, g_dp_data.m_source_resolution.m_high,g_dp_data.m_source_resolution.m_fps,
		g_dp_data.m_video_format_type, g_dp_data.m_source_resolution.m_vesa_id);

    return count;
}

static ssize_t dp_vr_mode_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = scnprintf(buf, PAGE_SIZE, "%d\n", g_dp_data.m_external_display_type);
    return ret;
}

static ssize_t dp_lcd_power_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	ret = scnprintf(buf, PAGE_SIZE, "%d\n", g_dp_data.m_lcd_power_type);
    return ret;
}

static ssize_t dp_lcd_power_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
    int ret = 0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

    ret = sscanf(buf, "%d", (int *)(&g_dp_data.m_lcd_power_type));
    if (ret != 1) {
        printk("%s: store error\n", __func__);
        return RETURN_ERR;
    }

    if ((g_dp_data.m_lcd_power_type == LCD_POWER_OFF) || (g_dp_data.m_lcd_power_type == LCD_POWER_ON)) {
		if (g_dp_data.m_source_mode == DIFF_SOURCE) {
			printk("%s: store ok, g_lcd_power=%d\n", __func__, g_dp_data.m_lcd_power_type);
			hisi_dptx_main_panel_blank(g_dp_data.m_lcd_power_type);
		}else {
			printk("%s: Current is same source mode, do nothing here.\n", __func__);
		}
    } else {
        printk("%s: invalid g_lcd_power value, mode=%d.\n", __func__, g_dp_data.m_lcd_power_type);
    }

    return count;
}


static ssize_t dp_external_display_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    int ret = 0;

	UNUSED(dev);
	UNUSED(attr);
	if (buf == NULL) {
		printk("%s:buf is null\n", __func__);
		ret = -EINVAL;
		return ret;
	}

	if (g_dp_data.m_externel_info.m_width== 0 || g_dp_data.m_externel_info.m_high== 0){
		ret = scnprintf(buf, PAGE_SIZE, "%s\n", "No External Display");
	}else {
		ret = scnprintf(buf, PAGE_SIZE, "External Display Info: Width=%4d.High=%4d.FPS=%2d\n",
			g_dp_data.m_externel_info.m_width, g_dp_data.m_externel_info.m_high, g_dp_data.m_externel_info.m_fps);
	}

    return ret;
}

void dp_send_event(enum dp_event_type event)
{
	char event_buf[32] = {0};
	char *envp[] = {event_buf, NULL};
	int ret = 0;

	switch(event) {
		case DP_LINK_STATE_GOOD:
			snprintf(event_buf, sizeof(event_buf), "DP_LINK_STATE=GOOD");
		break;

		case DP_LINK_STATE_BAD:
			snprintf(event_buf, sizeof(event_buf), "DP_LINK_STATE=BAD");
		break;

		default :
			return;
		break;
	}

	ret = kobject_uevent_env(&dp_vr.dev->kobj, KOBJ_CHANGE, envp);

	if (ret < 0) {
		printk("%s,%d: uevent sending failed!!! ret=%d\n", __func__, __LINE__, ret);
	}
	else {
		printk("%s,%d: sent uevent %s\n", __func__, __LINE__, envp[0]);
	}

}
EXPORT_SYMBOL_GPL(dp_send_event);

void dp_link_state_event(dp_link_state_t state)
{
	char event_buf[DP_LINK_EVENT_BUF_MAX] = {0};
	char *envp[] = {event_buf, NULL};
	int count = ARRAY_SIZE(g_dp_link_event);
	int ret = 0;
	int i = 0;
	bool find = false;

	if (state >= DP_LINK_STATE_MAX) {
		printk(KERN_ERR "%s: invalid link state %d!\n", __func__, state);
		return;
	}

	for (i = 0; i < count; i++) {
		if ((g_dp_link_event[i].state == state) && (g_dp_link_event[i].event != NULL)
			&& (strlen(g_dp_link_event[i].event) != 0)) {
			find = true;
			strncpy(event_buf, g_dp_link_event[i].event,
				MIN(DP_LINK_EVENT_BUF_MAX - 1, strlen(g_dp_link_event[i].event)));
			dp_factory_set_link_event_no(state, false, event_buf, 0);
			break;
		}
	}

	if (!find) {
		printk(KERN_ERR "%s: not find link event %d!\n", __func__, state);
		return;
	}

	if (dp_factory_mode_is_enable() && !dp_factory_need_report_event()) {
		printk(KERN_INFO "%s: skip event %s in factory!\n", __func__, event_buf);
		return;
	}

	ret = kobject_uevent_env(&dp_source.dev->kobj, KOBJ_CHANGE, envp);
	if (ret < 0) {
		printk(KERN_ERR "%s: report event %s failed %d!\n", __func__, event_buf, ret);
	} else {
		printk(KERN_INFO "%s: report event %s success.\n", __func__, event_buf);
	}
}
EXPORT_SYMBOL_GPL(dp_link_state_event);

static struct device_attribute dev_source_mode = __ATTR(source_mode, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
	dp_source_mode_show, dp_source_mode_store);
static struct device_attribute dev_source_resolution = __ATTR(source_resolution, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
	dp_source_resolution_show, dp_source_resolution_store);
static struct device_attribute dev_vr_mode = __ATTR(vr_mode, (S_IRUSR | S_IRGRP),
	dp_vr_mode_show, NULL);
static struct device_attribute dev_external_display = __ATTR(display_info, (S_IRUSR | S_IRGRP),
	dp_external_display_show, NULL);
static struct device_attribute dev_lcd_power = __ATTR(lcd_power, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
	dp_lcd_power_show, dp_lcd_power_store);


static int __init dp_source_mode_init(void)
{
    int ret = 0;
	struct class *dp_class = NULL;

	printk("%s: enter\n", __func__);
	//Init the default value for g_dp_data
	memset(&g_dp_data, 0, sizeof(struct dp_source_data));
	g_dp_data.m_parse_dts_flag = false;
	g_dp_data.m_video_format_type = VCEA;
	g_dp_data.m_source_mode = SAME_SOURCE;
	g_dp_data.m_source_resolution.m_high = 0;
	g_dp_data.m_source_resolution.m_with = 0;
	g_dp_data.m_source_resolution.m_vesa_id = 0;
	g_dp_data.m_source_resolution.m_fps = 0;
	g_dp_data.m_external_display_type = EXT_PC_MODE;
	g_dp_data.m_lcd_power_type = LCD_POWER_OFF;

	ret = dp_source_mode_parse_dts();
	if(ret!=0){
		printk("%s parse dp source mode failed, using same source as default source mode!\n",__func__);
	}

	dp_class = class_create(THIS_MODULE, "dp");
    if (IS_ERR(dp_class)) {
		printk("%s:create dp source class failed!\n", __func__);
		return IS_ERR(dp_class);
	}
	g_dp_data.class = dp_class;

	dp_source.dev = device_create(g_dp_data.class, NULL, 0, NULL, dp_source.name);
	if (dp_source.dev == NULL) {
		printk("%s:dp source mode device create failed!\n", __func__);
		ret = RETURN_ERR;
		goto err_out;
	}
    ret = device_create_file(dp_source.dev, &dev_source_mode);
	if (ret){
		printk("%s: Error, could not create command\n", __func__);
		goto err_out;
	}

	dp_resolution.dev = device_create(g_dp_data.class, NULL, 0, NULL, dp_resolution.name);
	if (dp_resolution.dev == NULL) {
		printk("%s:dp resolution mode device create failed!\n", __func__);
        ret = RETURN_ERR;
		goto unregister_source_mode;
	}
    ret = device_create_file(dp_resolution.dev, &dev_source_resolution);
    if (ret){
		printk("%s: Error, could not create command\n", __func__);
		goto unregister_source_mode;
	}

	dp_vr.dev = device_create(g_dp_data.class, NULL, 0, NULL, dp_vr.name);
	if (dp_vr.dev == NULL) {
		printk("%s:dp vr mode device create failed!\n", __func__);
		ret = RETURN_ERR;
		goto unregister_dp_resolution;
	}
    ret = device_create_file(dp_vr.dev, &dev_vr_mode);
    if (ret){
		printk("%s: Error, could not create command\n", __func__);
		goto unregister_dp_resolution;
	}

	dp_external_info.dev = device_create(g_dp_data.class, NULL, 0, NULL, dp_external_info.name);
	if (dp_external_info.dev == NULL) {
		printk("%s:dp externel display mode device create failed!\n", __func__);
		ret = RETURN_ERR;
		goto unregister_dp_vr;
	}
    ret = device_create_file(dp_external_info.dev, &dev_external_display);
    if (ret){
		printk("%s: Error, could not create command\n", __func__);
		goto unregister_dp_vr;
	}

	dp_lcd_power.dev = device_create(g_dp_data.class, NULL, 0, NULL, dp_lcd_power.name);
	if (dp_lcd_power.dev == NULL) {
		printk("%s:dp lcd power mode device create failed!\n", __func__);
		ret = RETURN_ERR;
		goto unrehister_dp_externel_info;
	}
    ret = device_create_file(dp_lcd_power.dev, &dev_lcd_power);
    if (ret){
		printk("%s: Error, could not create command\n", __func__);
		goto unrehister_dp_externel_info;
	}

	printk(KERN_INFO "%s: sucess %d\n", __func__, ret);
	return ret;

unrehister_dp_externel_info:
	device_remove_file(dp_external_info.dev, &dev_external_display);
unregister_dp_vr:
	device_remove_file(dp_vr.dev, &dev_vr_mode);
unregister_dp_resolution:
	device_remove_file(dp_resolution.dev, &dev_source_resolution);
unregister_source_mode:
	device_remove_file(dp_source.dev, &dev_source_mode);
err_out:
	printk("%s: Failed %d\n", __func__, ret);
    return ret;
}

static void __exit dp_source_mode_exit(void)
{
    printk("enter %s\n", __func__);
	device_remove_file(dp_source.dev, &dev_source_mode);
	device_remove_file(dp_resolution.dev, &dev_source_resolution);
	device_remove_file(dp_vr.dev, &dev_vr_mode);
	device_remove_file(dp_external_info.dev, &dev_external_display);
	device_remove_file(dp_lcd_power.dev, &dev_lcd_power);
}

module_init(dp_source_mode_init);
module_exit(dp_source_mode_exit);
