/* Copyright (c) 2017-2018, Huawei terminal Tech. Co., Ltd. All rights reserved.
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

#include "lcd_kit_utils.h"
#include "lcd_kit_disp.h"
#include "lcd_kit_common.h"
#include "lcd_kit_power.h"
#include "lcd_kit_parse.h"
#include "lcd_kit_adapt.h"
#include "lcd_kit_core.h"


bool lcd_kit_support(void)
{
	struct device_node* lcdkit_np = NULL;
	const char* support_type = NULL;
	ssize_t ret = 0;

	lcdkit_np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_KIT_PANEL_TYPE);
	if (!lcdkit_np) {
		LCD_KIT_ERR("NOT FOUND device node!\n");
		return false;
	}
	ret = of_property_read_string(lcdkit_np, "support_lcd_type", &support_type);
	if (ret) {
		LCD_KIT_ERR("failed to get support_type.\n");
		return false;
	}
	if (!strncmp(support_type, "LCD_KIT", strlen("LCD_KIT"))) {
		LCD_KIT_INFO("lcd_kit is support!\n");
		return true;
	} else {
		LCD_KIT_INFO("lcd_kit is not support!\n");
		return false;
	}
}

int lcd_kit_lread_reg(void* pdata, uint32_t* out, struct lcd_kit_dsi_cmd_desc* cmds, uint32_t len)
{
	int ret = LCD_KIT_OK;
	return ret;
}

int lcd_kit_read_project_id(void)
{
	int ret = LCD_KIT_OK;
	return ret;
}

int lcd_kit_updt_fps(struct platform_device* pdev)
{
	int ret = LCD_KIT_OK;
	return ret;
}

int lcd_kit_updt_fps_scence(struct platform_device* pdev, uint32_t scence)
{
	int ret = LCD_KIT_OK;
	return ret;
}

void lcd_kit_disp_on_check_delay(void)
{
	long delta_time_backlight_to_panel_on = 0;
	u32 delay_margin = 0;
	struct timeval tv;

	memset(&tv, 0, sizeof(struct timeval));
	do_gettimeofday(&tv);
	LCD_KIT_INFO("set backlight at %lu seconds %lu mil seconds\n",
				 tv.tv_sec, tv.tv_usec);
	delta_time_backlight_to_panel_on = (tv.tv_sec - disp_info->quickly_sleep_out.panel_on_record_tv.tv_sec) * 1000000
									   + tv.tv_usec - disp_info->quickly_sleep_out.panel_on_record_tv.tv_usec;
	delta_time_backlight_to_panel_on /= 1000;
	if (delta_time_backlight_to_panel_on >= disp_info->quickly_sleep_out.interval) {
		LCD_KIT_INFO("%lu > %d, no need delay\n",
					 delta_time_backlight_to_panel_on,
					 disp_info->quickly_sleep_out.interval);
		goto CHECK_DELAY_END;
	}
	delay_margin = disp_info->quickly_sleep_out.interval - delta_time_backlight_to_panel_on;
	if (delay_margin > 200) {
		LCD_KIT_INFO("something maybe error");
		goto CHECK_DELAY_END;
	}
	msleep(delay_margin);
CHECK_DELAY_END:
	disp_info->quickly_sleep_out.panel_on_tag = false;
	return;
}

void lcd_kit_disp_on_record_time(void)
{
	do_gettimeofday(&disp_info->quickly_sleep_out.panel_on_record_tv);
	LCD_KIT_INFO("display on at %lu seconds %lu mil seconds\n", disp_info->quickly_sleep_out.panel_on_record_tv.tv_sec,
				 disp_info->quickly_sleep_out.panel_on_record_tv.tv_usec);
	disp_info->quickly_sleep_out.panel_on_tag = true;
	return;
}

int lcd_kit_is_enter_sleep_mode(void)
{
	int sleep_mode = 0;

	if (common_info->pt.support) {
		sleep_mode = common_info->pt.mode;
	}
	return sleep_mode;
}

void lcd_kit_pinfo_init(struct device_node* np, struct mtk_panel_info* pinfo)
{
	if (!pinfo) {
		LCD_KIT_ERR("pinfo is null\n");
		return;
	}

	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-xres", &pinfo->xres, 1440);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-yres", &pinfo->yres, 2560);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-width", &pinfo->width, 73);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-height", &pinfo->height, 130);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-orientation", &pinfo->orientation, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bpp", &pinfo->bpp, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bgr-fmt", &pinfo->bgr_fmt, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-type", &pinfo->bl_set_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-min", &pinfo->bl_min, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-max", &pinfo->bl_max, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-def", &pinfo->bl_default, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-cmd-type", &pinfo->type, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,panel-frc-enable", &pinfo->frc_enable, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,panel-esd-skip-mipi-check", &pinfo->esd_skip_mipi_check, 1);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ifbc-type", &pinfo->ifbc_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-ic-ctrl-type", &pinfo->bl_ic_ctrl_mode, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-gpio-offset", &pinfo->gpio_offset, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-pwm-out-div-value", &pinfo->blpwm_out_div_value, 0);
	OF_PROPERTY_READ_U64_RETURN(np, "lcd-kit,panel-pxl-clk", &pinfo->pxl_clk_rate);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-pxl-clk-div", &pinfo->pxl_clk_rate_div, 1);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-data-rate", &pinfo->data_rate, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-vsyn-ctr-type", &pinfo->vsync_ctrl_type, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-bl-pwm-preci-type", &pinfo->blpwm_precision_type, 0);

	/*effect info*/
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,sbl-support", &pinfo->sbl_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,gamma-support", &pinfo->gamma_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,gmp-support", &pinfo->gmp_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,color-temp-support", &pinfo->color_temperature_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,color-temp-rectify-support", &pinfo->color_temp_rectify_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,comform-mode-support", &pinfo->comform_mode_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,cinema-mode-support", &pinfo->cinema_mode_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,xcc-support", &pinfo->xcc_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,hiace-support", &pinfo->hiace_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,panel-ce-support", &pinfo->panel_effect_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,arsr1p-sharpness-support", &pinfo->arsr1p_sharpness_support, 0);

	/*ldi info*/
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,h-back-porch", &pinfo->ldi.h_back_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,h-front-porch", &pinfo->ldi.h_front_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,h-pulse-width", &pinfo->ldi.h_pulse_width);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,v-back-porch", &pinfo->ldi.v_back_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,v-front-porch", &pinfo->ldi.v_front_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-kit,v-pulse-width", &pinfo->ldi.v_pulse_width);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-hsync-plr", &pinfo->ldi.hsync_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-vsync-plr", &pinfo->ldi.vsync_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-pixel-clk-plr", &pinfo->ldi.pixelclk_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-kit,ldi-data-en-plr", &pinfo->ldi.data_en_plr);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,ldi-dpi0-overlap-size", &pinfo->ldi.dpi0_overlap_size, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,ldi-dpi1-overlap-size", &pinfo->ldi.dpi1_overlap_size, 0);

	/*mipi info*/
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-lane-nums", &pinfo->mipi.lane_nums, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-color-mode", &pinfo->mipi.color_mode, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-vc", &pinfo->mipi.vc, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-burst-mode", &pinfo->mipi.burst_mode, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk", &pinfo->mipi.dsi_bit_clk, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-max-tx-esc-clk", &pinfo->mipi.max_tx_esc_clk, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val1", &pinfo->mipi.dsi_bit_clk_val1, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val2", &pinfo->mipi.dsi_bit_clk_val2, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val3", &pinfo->mipi.dsi_bit_clk_val3, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val4", &pinfo->mipi.dsi_bit_clk_val4, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-dsi-bit-clk-val5", &pinfo->mipi.dsi_bit_clk_val5, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-non-continue-enable", &pinfo->mipi.non_continue_en, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-post-adjust", &pinfo->mipi.clk_post_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-pre-adjust", &pinfo->mipi.clk_pre_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-prepare-adjust", &pinfo->mipi.clk_t_hs_prepare_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-lpx-adjust", &pinfo->mipi.clk_t_lpx_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-trail-adjust", &pinfo->mipi.clk_t_hs_trial_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-exit-adjust", &pinfo->mipi.clk_t_hs_exit_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-clk-t-hs-zero-adjust", &pinfo->mipi.clk_t_hs_zero_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-data-t-hs-trail-adjust", &pinfo->mipi.data_t_hs_trial_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-rg-vcm-adjust", &pinfo->mipi.rg_vrefsel_vcm_adjust, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-phy-mode", &pinfo->mipi.phy_mode, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-lp11_flag", &pinfo->mipi.lp11_flag, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-hs-wr-to-time", &pinfo->mipi.hs_wr_to_time, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,mipi-phy-update", &pinfo->mipi.phy_m_n_count_update, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "lcd-kit,mipi-dsi-upt-support", &pinfo->dsi_bit_clk_upt_support, 0);

	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-lcm-type", &pinfo->panel_lcm_type, 0);
	LCD_KIT_ERR("pinfo->panel_lcm_type [%d]\n", pinfo->panel_lcm_type);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ldi-dsi-mode", &pinfo->panel_dsi_mode, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-dsi-switch-mode", &pinfo->panel_dsi_switch_mode, 0);
	//OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-dsi-switch-mode-en", &pinfo->mipi.phy_m_n_count_update, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ldi-trans-seq", &pinfo->panel_trans_seq, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ldi-data-padding", &pinfo->panel_data_padding, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ldi-packet-size", &pinfo->panel_packtet_size, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-ldi-ps", &pinfo->panel_ps, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-vfp-lp", &pinfo->ldi.v_front_porch_forlp, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-fbk-div", &pinfo->pxl_fbk_div, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,panel-density", &pinfo->panel_density, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-kit,ssc-disable", &pinfo->ssc_disable, 0);

	if (common_info->esd.support) {
		pinfo->esd_enable = 1;
	}
	return;
}
//
//void lcd_kit_factory_init(struct hisi_panel_info* pinfo)
//{
//	if (runmode_is_factory()) {
//		pinfo->esd_enable = 0;
//		pinfo->dirty_region_updt_support = 0;
//		pinfo->prefix_ce_support = 0;
//		pinfo->prefix_sharpness1D_support = 0;
//		pinfo->prefix_sharpness2D_support = 0;
//		pinfo->sbl_support = 0;
//		pinfo->acm_support = 0;
//		pinfo->acm_ce_support = 0;
//		pinfo->esd_enable = 0;
//		pinfo->comform_mode_support = 0;
//		pinfo->color_temp_rectify_support = 0;
//		pinfo->hiace_support = 0;
//		pinfo->arsr1p_sharpness_support = 0;
//		pinfo->blpwm_input_ena = 0;
//		pinfo->gmp_support = 0;
//		common_info->effect_on.support = 0;
//		common_info->effect_color.support = 0;
//		disp_info->fps.support = 0;
//	}
//}

void lcd_kit_parse_running(struct device_node* np)
{
	return ;
}

void lcd_kit_parse_effect(struct device_node* np)
{
	return ;
}

void lcd_kit_parse_util(struct device_node* np)
{
	return ;
}

void lcd_kit_parse_dt(struct device_node* np)
{
	if (!np) {
		LCD_KIT_ERR("np is null\n");
		return ;
	}
	/*parse running test*/
	lcd_kit_parse_running(np);
	/*parse effect info*/
	lcd_kit_parse_effect(np);
	/*parse normal function*/
	lcd_kit_parse_util(np);
}

int lcd_kit_get_bl_max_nit_from_dts(void)
{
	int ret = 0;
	struct device_node* np = NULL;

	if(common_info->blmaxnit.get_blmaxnit_type == GET_BLMAXNIT_FROM_DDIC)
	{
		np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_KIT_PANEL_TYPE);
		if (!np) {
			LCD_KIT_ERR("NOT FOUND device node %s!\n", DTS_COMP_LCD_KIT_PANEL_TYPE);
			ret = -1;
			return ret;
		}

		OF_PROPERTY_READ_U32_RETURN(np, "panel_ddic_max_brightness", &common_info->actual_bl_max_nit);
		LCD_KIT_INFO("max nit is  %d!\n", common_info->actual_bl_max_nit);
	}
	return LCD_KIT_OK;
}
int lcd_kit_is_enter_pt_mode(void)
{
	if (common_info->pt.support) {
		return common_info->pt.mode;
	}
	return LCD_KIT_OK;
}

static int lcd_kit_get_project_id(char *buff)
{
	if (buff == NULL) {
		LCD_KIT_ERR("buff is null\n");
		return LCD_KIT_FAIL;
	}
	strncpy(buff, disp_info->project_id.id, strlen(disp_info->project_id.id));
	return LCD_KIT_OK;
}

int lcd_kit_get_pt_station_status(void)
{
	int mode = 0;

	if (common_info->pt.support) {
		mode = common_info->pt.mode;
	}
	LCD_KIT_INFO("mode = %d\n", mode);
	return mode;
}

int lcd_kit_get_online_status(void)
{
	int status = LCD_ONLINE;

	if (!strncmp(disp_info->compatible, LCD_KIT_DEFAULT_PANEL, strlen(disp_info->compatible))) {
		/*panel is online*/
		status = LCD_OFFLINE;
	}
	LCD_KIT_INFO("status = %d\n", status);
	return status;
}

int lcd_kit_get_status_by_type(int type, int *status)
{
	int ret = 0;

	if (status == NULL) {
		LCD_KIT_ERR("status is null\n");
		return LCD_KIT_FAIL;
	}
	switch (type) {
	case LCD_ONLINE_TYPE:
		*status = lcd_kit_get_online_status();
		ret = LCD_KIT_OK;
		break;
	case PT_STATION_TYPE:
		*status = lcd_kit_get_pt_station_status();
		ret = LCD_KIT_OK;
		break;
	default:
		LCD_KIT_ERR("not support type\n");
		ret = LCD_KIT_FAIL;
		break;
	}
	return ret;
}

struct lcd_kit_ops g_lcd_ops = {
	.lcd_kit_support = lcd_kit_support,
	.get_project_id = lcd_kit_get_project_id,
	.create_sysfs = lcd_kit_create_sysfs,
	.get_status_by_type = lcd_kit_get_status_by_type,
	.get_pt_station_status = lcd_kit_get_pt_station_status,
};

int lcd_kit_utils_init(struct device_node* np, struct mtk_panel_info* pinfo)
{
	/*init sem*/
	sema_init(&disp_info->lcd_kit_sem, 1);
	/*init mipi lock*/
	mutex_init(&disp_info->mipi_lock);
	/*parse display dts*/
	lcd_kit_parse_dt(np);
	/*init hisi pinfo*/
	lcd_kit_pinfo_init(np, pinfo);
	lcd_kit_ops_register(&g_lcd_ops);
	return LCD_KIT_OK;
}
