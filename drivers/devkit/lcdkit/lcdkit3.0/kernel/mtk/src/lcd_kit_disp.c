/*
 * Copyright (C) 2015 MediaTek Inc.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define LOG_TAG "LCM"

#include "lcm_drv.h"
#include "lcd_kit_disp.h"
#include "lcd_kit_utils.h"
#include "lcd_kit_common.h"
#include "lcd_kit_power.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_core.h"
#ifndef BUILD_LK
#include <linux/string.h>
#include <linux/kernel.h>
#endif

#include "lcm_drv.h"

#ifdef BUILD_LK
#include <platform/upmu_common.h>
#include <platform/mt_gpio.h>
#include <platform/mt_i2c.h>
#include <platform/mt_pmic.h>
#include <string.h>
#elif defined(BUILD_UBOOT)
#include <asm/arch/mt_gpio.h>
#else
/*#include <mach/mt_pm_ldo.h>*/
#ifdef CONFIG_MTK_LEGACY
#include <mach/mt_gpio.h>
#endif
#endif
#ifdef CONFIG_MTK_LEGACY
#include <cust_gpio_usage.h>
#endif
#ifndef CONFIG_FPGA_EARLY_PORTING
#if defined(CONFIG_MTK_LEGACY)
#include <cust_i2c.h>
#endif
#endif

#include "lcm_drv.h"


#ifndef BUILD_LK
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
/* #include <linux/jiffies.h> */
/* #include <linux/delay.h> */
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#endif

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
static struct dsm_dev dsm_lcd = {
	.name = "dsm_lcd",
	.device_name = NULL,
	.ic_name = NULL,
	.module_name = NULL,
	.fops = NULL,
	.buff_size = 1024,
};

struct dsm_client *lcd_dclient = NULL;
struct dsm_lcd_record lcd_record;
#endif

struct LCM_UTIL_FUNCS lcm_util_mtk;

static struct mtk_panel_info lcd_kit_pinfo = {0};

static struct lcd_kit_disp_info g_lcd_kit_disp_info;
struct lcd_kit_disp_info *lcd_kit_get_disp_info(void)
{
	return &g_lcd_kit_disp_info;
}

int is_mipi_cmd_panel(void)
{
	if(lcd_kit_pinfo.panel_dsi_mode == 0)
	{
		return 1;
	}

	return 0;
}

void  lcd_kit_bl_ic_set_backlight(unsigned int bl_level)
{
	struct lcd_kit_bl_ops *bl_ops = NULL;

	if(lcd_kit_pinfo.bl_ic_ctrl_mode) {
		bl_ops = lcd_kit_get_bl_ops();
		if (!bl_ops) {
			LCD_KIT_INFO("bl_ops is null!\n");
			return;
		}
		if (bl_ops->set_backlight) {
			bl_ops->set_backlight(bl_level);
		}
	}
}

void lcm_set_panel_state(unsigned int state)
{
	lcd_kit_pinfo.panel_state = state;
	return;
}

unsigned int lcm_get_panel_state(void)
{
	return lcd_kit_pinfo.panel_state;
}

static void lcm_set_util_funcs(const struct LCM_UTIL_FUNCS *util)
{
	memcpy(&lcm_util_mtk, util, sizeof(struct LCM_UTIL_FUNCS));
}

static void lcm_get_esd_config(struct LCM_PARAMS *params)
{
	int i = 0;
	int j = 0;
	struct lcd_kit_dsi_cmd_desc *esd_cmds;

	params->dsi.customization_esd_check_enable = common_info->esd.support;
	params->dsi.esd_check_enable = common_info->esd.support;
	if (common_info->esd.cmds.cmds) {
		esd_cmds = common_info->esd.cmds.cmds;
		for (i = 0; i < common_info->esd.cmds.cmd_cnt; i++) {
			params->dsi.lcm_esd_check_table[i].cmd = esd_cmds->payload[0];
			params->dsi.lcm_esd_check_table[i].count = esd_cmds->dlen;
			LCD_KIT_INFO("params->dsi.lcm_esd_check_table[%d].cmd = 0x%x\n", i, params->dsi.lcm_esd_check_table[i].cmd);
			LCD_KIT_INFO("params->dsi.lcm_esd_check_table[%d].count = %d\n", i, params->dsi.lcm_esd_check_table[i].count);
			for (j = 0; j < esd_cmds->dlen; j++) {
				params->dsi.lcm_esd_check_table[i].para_list[j] = common_info->esd.value.buf[i];
				LCD_KIT_INFO("params->dsi.lcm_esd_check_table[%d].para_list[%d] = %d\n", i, j, params->dsi.lcm_esd_check_table[i].para_list[j]);
			}
			esd_cmds++;
		}
	} else {
		LCD_KIT_INFO("esd not config, use default\n");
		params->dsi.lcm_esd_check_table[0].cmd = 0x0A;
		params->dsi.lcm_esd_check_table[0].count = 1;
		params->dsi.lcm_esd_check_table[0].para_list[0] = 0x9C;
	}
}

static void lcm_get_params(struct LCM_PARAMS *params)
{
	struct mtk_panel_info *pinfo = &lcd_kit_pinfo;
	memset(params, 0, sizeof(struct LCM_PARAMS));

	LCD_KIT_INFO(" +!\n");

	params->type = pinfo->panel_lcm_type;

	params->width = pinfo->xres;
	params->height = pinfo->yres;
	params->physical_width = pinfo->width;
	params->physical_height = pinfo->height;
	params->physical_width_um = pinfo->width * 1000;
	params->physical_height_um = pinfo->height * 1000;

	params->dsi.mode = pinfo->panel_dsi_mode;
	params->dsi.switch_mode = pinfo->panel_dsi_switch_mode;
	params->dsi.switch_mode_enable = 0;
	params->density = pinfo->panel_density;

	/* DSI */
	/* Command mode setting */
	params->dsi.LANE_NUM = pinfo->mipi.lane_nums;
	/* The following defined the fomat for data coming from LCD engine. */
	params->dsi.data_format.color_order = pinfo->bgr_fmt;
	params->dsi.data_format.trans_seq = pinfo->panel_trans_seq;
	params->dsi.data_format.padding = pinfo->panel_data_padding;
	params->dsi.data_format.format = pinfo->bpp;

	/* Highly depends on LCD driver capability. */
	params->dsi.packet_size = pinfo->panel_packtet_size;
	/* video mode timing */

	params->dsi.PS = pinfo->panel_ps;

	params->dsi.vertical_sync_active = pinfo->ldi.v_pulse_width;
	params->dsi.vertical_backporch = pinfo->ldi.v_back_porch;
	params->dsi.vertical_frontporch = pinfo->ldi.v_front_porch;
	params->dsi.vertical_frontporch_for_low_power = pinfo->ldi.v_front_porch_forlp;
	params->dsi.vertical_active_line = pinfo->yres;

	params->dsi.horizontal_sync_active = pinfo->ldi.h_pulse_width;
	params->dsi.horizontal_backporch = pinfo->ldi.h_back_porch;
	params->dsi.horizontal_frontporch = pinfo->ldi.h_front_porch;
	params->dsi.horizontal_active_pixel = pinfo->xres;

	params->dsi.PLL_CLOCK = pinfo->pxl_clk_rate;//440;	/* this value must be in MTK suggested table */
	params->dsi.data_rate = pinfo->data_rate;
	params->dsi.fbk_div =  pinfo->pxl_fbk_div;
	params->dsi.CLK_HS_POST = pinfo->mipi.clk_post_adjust;
	params->dsi.ssc_disable = pinfo->ssc_disable;
	params->dsi.clk_lp_per_line_enable = pinfo->mipi.lp11_flag;
	/*esd config*/
	lcm_get_esd_config(params);
	if(0 == pinfo->mipi.non_continue_en)
	{
		params->dsi.cont_clock = 1;
	}
	else
	{
		params->dsi.cont_clock = 0;
	}
}

static void lcd_kit_on(void)
{
	LCD_KIT_INFO(" +!\n");

	if (common_ops->panel_power_on) {
		common_ops->panel_power_on((void*)NULL);
	}
	lcm_set_panel_state(LCD_POWER_STATE_ON);
	/*record panel on time*/
	lcd_kit_disp_on_record_time();

	LCD_KIT_INFO(" -!\n");
	return;
}

static void lcd_kit_off(void)
{
	LCD_KIT_INFO(" +!\n");

	if (common_ops->panel_power_off) {
		common_ops->panel_power_off(NULL);
	}
	lcm_set_panel_state(LCD_POWER_STATE_OFF);
	LCD_KIT_INFO(" -!\n");
}

static void lcm_resume(void)
{
	lcd_kit_on();
}

static void lcd_kit_set_backlight(void *handle, unsigned int level)
{
	int ret = 0;

	LCD_KIT_INFO("%s, backlight: level = %d\n", __func__, level);

	ret = common_ops->set_mipi_backlight(NULL, level);
	if (ret < 0){
		return;
	}
}

struct LCM_DRIVER lcdkit_mtk_common_panel = {
    .panel_info = &lcd_kit_pinfo,
	.name = "lcdkit_mtk_common_panel_drv",
	.set_util_funcs = lcm_set_util_funcs,
	.get_params = lcm_get_params,
	.init = lcd_kit_on,
	.suspend = lcd_kit_off,
	.resume = lcm_resume,
	.set_backlight_cmdq = lcd_kit_set_backlight,
};

static int __init lcd_kit_init(void)
{
	int ret = LCD_KIT_OK;
	struct device_node* np = NULL;

	LCD_KIT_INFO(" +!\n");

	if (!lcd_kit_support()) {
		LCD_KIT_INFO("not lcd_kit driver and return\n");
		return ret;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_KIT_PANEL_TYPE);
	if (!np) {
		LCD_KIT_ERR("NOT FOUND device node %s!\n", DTS_COMP_LCD_KIT_PANEL_TYPE);
		ret = -1;
		return ret;
	}

	OF_PROPERTY_READ_U32_RETURN(np, "product_id", &disp_info->product_id);
	LCD_KIT_INFO("disp_info->product_id = %d\n", disp_info->product_id);
	disp_info->compatible = (char*)of_get_property(np, "lcd_panel_type", NULL);
	if (!disp_info->compatible) {
		LCD_KIT_ERR("can not get lcd kit compatible\n");
		return ret;
	}
	LCD_KIT_INFO("disp_info->compatible: %s\n", disp_info->compatible);

    np = of_find_compatible_node(NULL, NULL, disp_info->compatible);
	if (!np) {
		LCD_KIT_ERR("NOT FOUND device node %s!\n", disp_info->compatible);
		ret = -1;
		return ret;
	}

#if defined (CONFIG_HUAWEI_DSM)
	lcd_dclient = dsm_register_client(&dsm_lcd);
#endif
	/*1.adapt init*/
	lcd_kit_adapt_init();
	/*2.common init*/
	if (common_ops->common_init) {
		common_ops->common_init(np);
	}
	/*3.utils init*/
	lcd_kit_utils_init(np, lcdkit_mtk_common_panel.panel_info);
	/*4.init fnode*/
	lcd_kit_sysfs_init();
	/*5.init factory mode*/
	//lcd_kit_factory_init(pinfo);
	/*6.power init*/
	lcd_kit_power_init();
	/*7.init panel ops*/
	lcd_kit_panel_init();
	/*get lcd max brightness*/
	lcd_kit_get_bl_max_nit_from_dts();
	return ret;
}

fs_initcall(lcd_kit_init);

