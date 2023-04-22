/* Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS" AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR
 * BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
 * OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
 * IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */

#include "hisi_fb.h"
#include <linux/hisi/hisi_adc.h>
#include <linux/hisi/hw_cmdline_parse.h>
#include <huawei_platform/touthscreen/huawei_touchscreen.h>
#include "hw_lcd_panel.h"
#include "hw_lcd_effects.h"
#include <huawei_platform/log/log_jank.h>
#include <securec.h>

/***********************************************************
*platform driver definition
***********************************************************/
/*
*probe match table
*/
static struct of_device_id hw_lcd_match_table[] = {
	{
		.compatible = "hisilicon,auo_otm1901a_5p2_1080p_video",//default is tianma-nt35521
		.data = NULL,
	},
	{},
};

/*
*panel platform driver
*/
static struct platform_driver this_driver = {
	.probe = hw_lcd_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "hw_mipi_panel",
		.of_match_table = hw_lcd_match_table,
	},
};

/***********************************************************
*variable definition
***********************************************************/
static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00, 0x07, 0x7F
};

static struct dsi_cmd_desc set_display_address[] = {
	{
		DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x
	},
	{
		DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y
	},
};

static int g_sre_enable = 0;
/***********************************************************
*function definition
***********************************************************/
/*
*name:hw_lcd_parse_array_data
*function:parse panel data from dtsi
*@np:device tree node
*@name:parse name
*@out:output data
*/
static int hw_lcd_parse_array_data(struct device_node* np, char* name, struct array_data* out)
{
	const char* data;
	int blen = 0;
	char* buf;
	int i;

	data = of_get_property(np, name, &blen);

	if (!data) {
		HISI_FB_ERR("Parse array name: %s, fail\n", name);
		return -ENOMEM;
	}

	/*The property is 4bytes long per element in cells: <>*/
	blen = blen / 4;
	/*If use bype property: [], this division should be removed*/
	buf = kzalloc(sizeof(char) * blen, GFP_KERNEL);

	if (!buf) {
		HISI_FB_ERR("Allocate memory fail: buf\n");
		return -ENOMEM;
	}

	//For use byte property: []
	//memcpy(buf, data, blen);

	//For use cells property: <>
	for (i = 0; i < blen; i++) {
		buf[i] = data[i * 4 + 3];
	}

	out->buf = buf;
	out->cnt = blen;
	return 0;
}

/*
*name:hw_lcd_parse_dcs_cmds
*function:parse panel dcs cmds from dtsi
*@np:device tree node
*@cmd_name:parse name
*@cmd_set:output dsi command
*/
static int hw_lcd_parse_dcs_cmds(struct device_node* np, char* cmd_name, struct dsi_cmd_set* cmd_set)
{
	const char* data;
	int blen = 0, len, cmd_len;
	char* buf, *bp;
	int i, cnt;
	struct dsi_cmd_desc* dsi_cmds;
	const int len_cmd_hdr = 5;
	const int index_cmd_dlen = 4;
	const int index_cmd_waittype = 3;
	const int index_cmd_wait = 2;
	const int index_cmd_vc = 1;
	const int index_cmd_dtype = 0;

	data = of_get_property(np, cmd_name, &blen);

	if (!data) {
		HISI_FB_ERR("Parse panel dcs commands: %s, fail\n", cmd_name);
		return -ENOMEM;
	}

	/*The property is 4bytes long per element in cells: <>*/
	blen = blen / 4;
	/*If use bype property: [], this division should be removed*/
	buf = kzalloc(sizeof(char) * blen, GFP_KERNEL);

	if (!buf) {
		HISI_FB_ERR("Allocate memory fail: buf\n");
		return -ENOMEM;
	}

	//For use byte property: []
	//memcpy(buf, data, blen);

	//For use cells property: <>
	for (i = 0; i < blen; i++) {
		buf[i] = data[i * 4 + 3];
	}

	/* scan dcs commands */
	bp = buf;
	len = blen;
	cnt = 0;

	while (len >= len_cmd_hdr) {
		cmd_len = (int) * (bp + index_cmd_dlen);

		if ((cmd_len + len_cmd_hdr) > len) {
			HISI_FB_ERR("DSC commands: %s, at %dth command with len=%d, WRONG\n", cmd_name, cnt + 1, cmd_len);
			goto exit_free;
		}

		bp += (cmd_len + len_cmd_hdr);
		len -= (cmd_len + len_cmd_hdr);
		cnt++;
	}

	if (len != 0) {
		HISI_FB_ERR("DSC commands incomplete!");
		goto exit_free;
	}

	dsi_cmds = kzalloc(cnt * sizeof(struct dsi_cmd_desc), GFP_KERNEL);

	if (!dsi_cmds) {
		HISI_FB_ERR("Allocate memory fail: dsi_cmd_desc\n");
		goto exit_free;
	}

	bp = buf;

	for (i = 0; i < cnt; i++) {
		dsi_cmds[i].dtype = (int) * (bp + index_cmd_dtype);
		dsi_cmds[i].vc = (int) * (bp + index_cmd_vc);
		dsi_cmds[i].wait = (int) * (bp + index_cmd_wait);
		dsi_cmds[i].waittype = (int) * (bp + index_cmd_waittype);
		dsi_cmds[i].dlen = (int) * (bp + index_cmd_dlen);
		dsi_cmds[i].payload = bp + len_cmd_hdr;
		bp += (dsi_cmds[i].dlen + len_cmd_hdr);
	}

	//TODO: return the dynamic allocate memory: buf, for future clean up.
	cmd_set->buf = buf;
	cmd_set->size_buf = blen;
	cmd_set->cmd_set = dsi_cmds;
	cmd_set->cmd_cnt = cnt;

	return 0;


exit_free:
	kfree(buf);
	return -ENOMEM;

}

/*
*name:hw_lcd_parse_dts
*function:parse dts data
*@np:device tree node
*/
static int hw_lcd_parse_dts(struct device_node* np)
{
	int ret = 0;

	/*Parse panel on cmds*/
	ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-on-cmds", &lcd_info.display_on_cmds);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-on-cmds failed!\n");
		return -ENOMEM;
	}

	/*Parse panel off cmds*/
	ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-off-cmds", &lcd_info.display_off_cmds);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-off-cmds failed!\n");
		return -ENOMEM;
	}

	/*Parse cabc off cmds*/
	ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-cabc-off-mode", &lcd_info.cabc_off_cmds);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-cabc-off-mode failed!\n");
		return -ENOMEM;
	}

	/*Parse cabc ui cmds*/
	ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-cabc-ui-mode", &lcd_info.cabc_ui_cmds);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-cabc-ui-mode failed!\n");
		return -ENOMEM;
	}

	/*Parse cabc still cmds*/
	ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-cabc-still-mode", &lcd_info.cabc_still_cmds);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-cabc-still-mode failed!\n");
		return -ENOMEM;
	}

	/*Parse cabc moving cmds*/
	ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-cabc-moving-mode", &lcd_info.cabc_moving_cmds);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-cabc-moving-mode failed!\n");
		return -ENOMEM;
	}

	/*esd check*/
	ret = hw_lcd_parse_array_data(np, "hisilicon,dss-esd-reg", &lcd_info.esd_reg);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-esd-reg failed!\n");
		return -ENOMEM;
	}
	ret = hw_lcd_parse_array_data(np, "hisilicon,dss-esd-value", &lcd_info.esd_value);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-esd-value failed!\n");
		return -ENOMEM;
	}

	/*mipi check reg*/
	ret = hw_lcd_parse_array_data(np, "hisilicon,dss-mipi-check-reg", &lcd_info.mipi_check_reg);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-mipi-check-reg failed!\n");
		return -ENOMEM;
	}

	ret = hw_lcd_parse_array_data(np, "hisilicon,dss-mipi-check-value", &lcd_info.mipi_check_value);
	if (ret) {
		HISI_FB_ERR("parse hisilicon,dss-mipi-check-value failed!\n");
		return -ENOMEM;
	}

	/*power on sequence*/
	ret = hw_lcd_parse_array_data(np, "power-on-sequence", &lcd_info.power_on_seq);
	if (ret) {
		HISI_FB_ERR("parse power-on-sequence failed!\n");
		return -ENOMEM;
	}
	if (lcd_info.power_on_seq.cnt > POWER_ON_SEQ_MAX) {
		HISI_FB_ERR("power on seq count:%d greater than max count:%d, failed!\n", lcd_info.power_on_seq.cnt, POWER_ON_SEQ_MAX);
		return -ENOMEM;
	}
	/*power off sequence*/
	ret = hw_lcd_parse_array_data(np, "power-off-sequence", &lcd_info.power_off_seq);
	if (ret) {
		HISI_FB_ERR("parse power-off-sequence failed!\n");
		return -ENOMEM;
	}
	if (lcd_info.power_off_seq.cnt > (int)POWER_OFF_SEQ_MAX) {
		HISI_FB_ERR("power off seq count:%d greater than max count:%d, failed!\n", lcd_info.power_off_seq.cnt, POWER_OFF_SEQ_MAX);
		return -ENOMEM;
	}
	return 0;
}

/*
*name:hw_lcd_info_init
*function:parse panel info from dtsi
*@np:device tree node
*@pinfo:lcd panel info
*/
static int hw_lcd_info_init(struct device_node* np, struct hisi_panel_info* pinfo)
{
	int ret = 0;

	/*parse panel name*/
	lcd_info.panel_name = (char*)of_get_property(np, "hisilicon,dss-panel-name", NULL);
	/*parse panel info*/
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-xres", &pinfo->xres);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-yres", &pinfo->yres);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-width", &pinfo->width);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-height", &pinfo->height);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-orientation", &pinfo->orientation);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-bpp", &pinfo->bpp);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-bgrfmt", &pinfo->bgr_fmt);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-bl-type", &pinfo->bl_set_type);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-blmin", &pinfo->bl_min);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-blmax", &pinfo->bl_max);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-bl-def", &pinfo->bl_default);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-type", &pinfo->type);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-ifbc-type", &pinfo->ifbc_type);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-bl-pwm-preci-type", &pinfo->blpwm_precision_type);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-panel-frc-enable", &pinfo->frc_enable);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-panel-esd-enable", &pinfo->esd_enable);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-panel-esd-skip-mipi-check", &pinfo->esd_skip_mipi_check);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-pxl-clk", &pinfo->pxl_clk_rate);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-pxl-clk-div", &pinfo->pxl_clk_rate_div);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-panel-dirt-updt-support", &pinfo->dirty_region_updt_support);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-panel-dsi-upt-support", &pinfo->dsi_bit_clk_upt_support);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-panel-vsyn-ctr-type", &pinfo->vsync_ctrl_type);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-panel-step-support", &pinfo->lcd_uninit_step_support);
	OF_PROPERTY_READ_U8_DEFAULT(np, "hisilicon,dss-panel-xcc-set-in-isr-support", &pinfo->xcc_set_in_isr_support, 0);
	OF_PROPERTY_READ_U8_DEFAULT(np, "hisilicon,dss-panel-sre-support", &pinfo->sre_support, 0);

	/*effect info*/
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-sbl-support", &pinfo->sbl_support);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-gamma-support", &pinfo->gamma_support);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-gmp-support", &pinfo->gmp_support);

	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-color-temp-support", &pinfo->color_temperature_support);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-xcc-support", &pinfo->xcc_support);
	OF_PROPERTY_READ_U8_DEFAULT(np, "hisilicon,dss-comform-mode-support", &pinfo->comform_mode_support,0);

	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-prefix-ce-support", &pinfo->prefix_ce_support);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-prefix-sharp-one-d-support", &pinfo->prefix_sharpness1D_support);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-prefix-sharp-two-d-support", &pinfo->prefix_sharpness2D_support);

	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-acm-support", &pinfo->acm_support);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-acm-ce-support", &pinfo->acm_ce_support);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-valid-num", &pinfo->acm_valid_num);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_hh0", &pinfo->r0_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_lh0", &pinfo->r0_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_hh1", &pinfo->r1_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_lh1", &pinfo->r1_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_hh2", &pinfo->r2_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_lh2", &pinfo->r2_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_hh3", &pinfo->r3_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_lh3", &pinfo->r3_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_hh4", &pinfo->r4_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_lh4", &pinfo->r4_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_hh5", &pinfo->r5_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_lh5", &pinfo->r5_lh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_hh6", &pinfo->r6_hh);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-acm-r_lh6", &pinfo->r6_lh);

	/*sbl info*/
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-stren-limit", &pinfo->smart_bl.strength_limit);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-cal-a", &pinfo->smart_bl.calibration_a);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-cal-b", &pinfo->smart_bl.calibration_b);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-cal-c", &pinfo->smart_bl.calibration_c);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-cal-d", &pinfo->smart_bl.calibration_d);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-tf-ctl", &pinfo->smart_bl.t_filter_control);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-blmin", &pinfo->smart_bl.backlight_min);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-blmax", &pinfo->smart_bl.backlight_max);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-bl-scale", &pinfo->smart_bl.backlight_scale);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-am-light-min", &pinfo->smart_bl.ambient_light_min);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-filter-a", &pinfo->smart_bl.filter_a);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-filter-b", &pinfo->smart_bl.filter_b);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-logo-left", &pinfo->smart_bl.logo_left);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-logo-top", &pinfo->smart_bl.logo_top);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-variance-intensity-space", &pinfo->smart_bl.variance_intensity_space);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-slope-max", &pinfo->smart_bl.slope_max);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-sbl-slope-min", &pinfo->smart_bl.slope_min);

	/*ldi info*/
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-ldi-h-back-porch", &pinfo->ldi.h_back_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-ldi-h-front-porch", &pinfo->ldi.h_front_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-ldi-h-pulse-width", &pinfo->ldi.h_pulse_width);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-ldi-v-back-porch", &pinfo->ldi.v_back_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-ldi-v-front-porch", &pinfo->ldi.v_front_porch);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-ldi-v-pulse-width", &pinfo->ldi.v_pulse_width);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-ldi-hsync-plr", &pinfo->ldi.hsync_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-ldi-vsync-plr", &pinfo->ldi.vsync_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-ldi-pixel-clk-plr", &pinfo->ldi.pixelclk_plr);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-ldi-data-en-plr", &pinfo->ldi.data_en_plr);

	/*dynamic fps info*/
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-panel-fps", &pinfo->fps, 60);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-panel-fps-updt", &pinfo->fps_updt, 60);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-panel-fps-updt-support", &pinfo->fps_updt_support, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-ldi-lfps-h-back-porch", &pinfo->ldi_lfps.h_back_porch, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-ldi-lfps-h-front-porch", &pinfo->ldi_lfps.h_front_porch, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-ldi-lfps-h-pulse-width", &pinfo->ldi_lfps.h_pulse_width, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-ldi-lfps-v-back-porch", &pinfo->ldi_lfps.v_back_porch, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-ldi-lfps-v-front-porch", &pinfo->ldi_lfps.v_front_porch, 0);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-ldi-lfps-v-pulse-width", &pinfo->ldi_lfps.v_pulse_width, 0);

	/*mipi info*/
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-mipi-lane-nums", &pinfo->mipi.lane_nums);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-mipi-color-mode", &pinfo->mipi.color_mode);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-mipi-vc", &pinfo->mipi.vc);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-burst-mode", &pinfo->mipi.burst_mode);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-dsi-bit-clk", &pinfo->mipi.dsi_bit_clk);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-max-esc-clk", &pinfo->mipi.max_tx_esc_clk);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-dsi-bit-clk-val-a", &pinfo->mipi.dsi_bit_clk_val1);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-dsi-bit-clk-val-b", &pinfo->mipi.dsi_bit_clk_val2);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-dsi-bit-clk-val-c", &pinfo->mipi.dsi_bit_clk_val3);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-dsi-bit-clk-val-d", &pinfo->mipi.dsi_bit_clk_val4);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-dsi-bit-clk-val-e", &pinfo->mipi.dsi_bit_clk_val5);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-dsi-bit-clk-upt", &pinfo->mipi.dsi_bit_clk_upt);
	OF_PROPERTY_READ_U8_RETURN(np, "hisilicon,dss-mipi-non-continue-enable", &pinfo->mipi.non_continue_en);
	OF_PROPERTY_READ_U32_RETURN(np, "hisilicon,dss-mipi-clk-post-adjust", &pinfo->mipi.clk_post_adjust);
	OF_PROPERTY_READ_U32_DEFAULT(np, "hisilicon,dss-mipi-rg-vcm-adjust", &pinfo->mipi.rg_vrefsel_vcm_adjust, 0);

	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-left-align", &pinfo->dirty_region_info.left_align);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-right-align", &pinfo->dirty_region_info.right_align);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-top-align", &pinfo->dirty_region_info.top_align);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-bott-align", &pinfo->dirty_region_info.bottom_align);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-width-align", &pinfo->dirty_region_info.w_align);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-height-align", &pinfo->dirty_region_info.h_align);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-width-min", &pinfo->dirty_region_info.w_min);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-height-min", &pinfo->dirty_region_info.h_min);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-top-start", &pinfo->dirty_region_info.top_start);
	OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hisilicon,dss-dirt-bott-start", &pinfo->dirty_region_info.bottom_start);

	/*lcd info*/
	OF_PROPERTY_READ_U32_RETURN(np, "lcdanalog-vcc", &lcd_info.lcdanalog_vcc);
	OF_PROPERTY_READ_U32_RETURN(np, "lcdio-vcc", &lcd_info.lcdio_vcc);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-bias", &lcd_info.lcd_bias);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-vsp", &lcd_info.lcd_vsp);
	OF_PROPERTY_READ_U32_RETURN(np, "lcd-vsn", &lcd_info.lcd_vsn);
	OF_PROPERTY_READ_U8_RETURN(np, "lcd-ctrl-tp-power", &g_lcd_control_tp_power);
	OF_PROPERTY_READ_U8_RETURN(np, "lock-cmd-support", &lcd_info.lock_cmd_support);
	OF_PROPERTY_READ_U8_RETURN(np, "read-power-status", &lcd_info.read_power_status);
	OF_PROPERTY_READ_U8_RETURN(np, "esd-set-backlight", &lcd_info.esd_set_bl);
	OF_PROPERTY_READ_U8_DEFAULT(np, "pt-station-test-support", &lcd_info.pt_test_support, 1);
	OF_PROPERTY_READ_U8_DEFAULT(np, "read-data-type", &lcd_info.read_data_type,DTYPE_GEN_READ1);
	OF_PROPERTY_READ_U32_DEFAULT(np, "lcd-power-ctrl-mode", &lcd_info.power_ctrl_mode,POWER_CTRL_BY_REGULATOR);

	if(pinfo->sre_support){
		/*Parse sre on cmds*/
		ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-sre-on-cmd", &lcd_info.sre_on_cmds);
		if (ret) {
			HISI_FB_ERR("parse hisilicon,dss-sre-on-cmd failed!\n");
			return -ENOMEM;
		}
		/*Parse sre off cmds*/
		ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-sre-off-cmd", &lcd_info.sre_off_cmds);
		if (ret) {
			HISI_FB_ERR("parse hisilicon,dss-sre-off-cmd failed!\n");
			return -ENOMEM;
		}
	}

	pinfo->pxl_clk_rate *= 1000000UL;
	pinfo->mipi.max_tx_esc_clk *= 1000000;

	if (pinfo->bl_set_type == BL_SET_BY_BLPWM) {
		pinfo->blpwm_input_ena = 1;
	}

	if (pinfo->ifbc_type == IFBC_TYPE_ORISE3X) {
		pinfo->ifbc_cmp_dat_rev0 = 0;
		pinfo->ifbc_cmp_dat_rev1 = 0;
		pinfo->ifbc_auto_sel = 1;
		pinfo->ifbc_orise_ctr = 1;

		//FIXME:
		pinfo->pxl_clk_rate_div = 3;

		pinfo->ifbc_orise_ctl = IFBC_ORISE_CTL_FRAME;
	}

	/*ce rely on acm*/
	if (pinfo->acm_support == 0) {
		pinfo->acm_ce_support = 0;
	}

	return ret;
}

/*
*name:hw_lcd_vcc_init
*function:init lcd vcc parameter
*@cmds:vcc cmds
*@cnt:vcc number
*/
static void hw_lcd_vcc_init(struct vcc_desc* cmds, int cnt)
{
	int i = 0;
	struct vcc_desc* cm = NULL;

	cm = cmds;
	for (i = 0; i < cnt; i++) {
		if (cm->dtype == DTYPE_VCC_SET_VOLTAGE) {
			if (0 == strncmp(cm->id, HW_LCD_VCC_LCDANALOG_NAME, strlen(cm->id))) {
				cm->min_uV = lcd_info.lcdanalog_vcc;
				cm->max_uV = lcd_info.lcdanalog_vcc;
			} else if (0 == strncmp(cm->id, HW_LCD_VCC_LCDIO_NAME, strlen(cm->id))) {
				cm->min_uV = lcd_info.lcdio_vcc;
				cm->max_uV = lcd_info.lcdio_vcc;
			} else if (0 == strncmp(cm->id, VCC_LCDBIAS_NAME, strlen(cm->id))) {
				cm->min_uV = lcd_info.lcd_bias;
				cm->max_uV = lcd_info.lcd_bias;
			} else if (0 == strncmp(cm->id, VCC_LCD_VSP_NAME, strlen(cm->id))) {
				cm->min_uV = lcd_info.lcd_bias;
				cm->max_uV = lcd_info.lcd_bias;
			} else if (0 == strncmp(cm->id, VCC_LCD_VSN_NAME, strlen(cm->id))) {
				cm->min_uV = lcd_info.lcd_bias;
				cm->max_uV = lcd_info.lcd_bias;
			}
		}
		cm++;
	}
}

/*
*name:hw_lcd_gpio_seq_init
*function:init gpio sequence
*@cmds:gpio cmds
*@step:step
*/
static int hw_lcd_gpio_seq_init(struct gpio_desc* cmds, int step)
{
	int ret = 0;
	struct gpio_desc* cm = NULL;

	cm = cmds;
	switch(step)
	{
		case LCD_GPIO_RESET_ON_STEP1:
			if (!strcmp(cm->label, GPIO_LCD_RESET_NAME)) {
				if (lcd_info.power_on_seq.buf[FST_RESET_H1_DELAY_SEQ] == 0 &&
					lcd_info.power_on_seq.buf[FST_RESET_L_DELAY_SEQ] == 0 &&
					lcd_info.power_on_seq.buf[FST_RESET_H2_DELAY_SEQ] == 0) {
					ret = 1;
					return ret;
				}
				cm->wait = lcd_info.power_on_seq.buf[FST_RESET_H1_DELAY_SEQ];
				cm++;
				cm->wait = lcd_info.power_on_seq.buf[FST_RESET_L_DELAY_SEQ];
				cm++;
				cm->wait = lcd_info.power_on_seq.buf[FST_RESET_H2_DELAY_SEQ];
			}
			break;
		case LCD_GPIO_RESET_ON_STEP2:
			if (!strcmp(cm->label, GPIO_LCD_RESET_NAME)) {
				if (lcd_info.power_on_seq.buf[SEC_RESET_H1_DELAY_SEQ] == 0 &&
					lcd_info.power_on_seq.buf[SEC_RESET_L_DELAY_SEQ] == 0 &&
					lcd_info.power_on_seq.buf[SEC_RESET_H2_DELAY_SEQ] == 0) {
					ret = 1;
					return ret;
				}
				cm->wait = lcd_info.power_on_seq.buf[SEC_RESET_H1_DELAY_SEQ];
				cm++;
				cm->wait = lcd_info.power_on_seq.buf[SEC_RESET_L_DELAY_SEQ];
				cm++;
				cm->wait = lcd_info.power_on_seq.buf[SEC_RESET_H2_DELAY_SEQ];
			}
			break;
		case LCD_GPIO_RESET_OFF_STEP1:
			if (!strcmp(cm->label, GPIO_LCD_RESET_NAME)) {
				if (lcd_info.power_off_seq.buf[FST_RESET_LOW_DELAY_SEQ] == 0) {
					ret = 1;
					return ret;
				}
				cm->wait = lcd_info.power_off_seq.buf[FST_RESET_LOW_DELAY_SEQ];
			}
			break;
		case LCD_GPIO_RESET_OFF_STEP2:
			if (!strcmp(cm->label, GPIO_LCD_RESET_NAME)) {
				if (lcd_info.power_off_seq.buf[SEC_RESET_LOW_DELAY_SEQ] == 0) {
					ret = 1;
					return ret;
				}
				cm->wait = lcd_info.power_off_seq.buf[SEC_RESET_LOW_DELAY_SEQ];
			}
			break;
		case LCD_GPIO_VSP_VSN_ON_STEP:
			if (!strcmp(cm->label, GPIO_LCD_VSP_NAME)) {
				cm->wait = lcd_info.power_on_seq.buf[VSP_ON_DELAY_SEQ];
				cm++;
				cm->wait = lcd_info.power_on_seq.buf[VSN_ON_DELAY_SEQ];
			}
			break;
		case LCD_GPIO_VSP_VSN_OFF_STEP:
			if (!strcmp(cm->label, GPIO_LCD_VSN_NAME)) {
				cm->wait = lcd_info.power_off_seq.buf[VSN_OFF_DELAY_SEQ];
				cm++;
				cm->wait = lcd_info.power_off_seq.buf[VSP_OFF_DELAY_SEQ];
			}
			break;
		default:
			HISI_FB_ERR("no this gpio.\n");
	}
	return ret;
}

/*
*name:hw_lcd_vcc_seq_init
*function:init regulator sequence
*@cmds:gpio cmds
*@step:step
*/
static void hw_lcd_vcc_seq_init(struct vcc_desc* cmds, int cnt, int step)
{
	int i = 0;
	struct vcc_desc* cm = NULL;

	cm = cmds;
	switch(step) {
		case LCD_VCC_POWER_ON_STEP:
			for (i = 0; i < cnt; i++) {
				if (cm->dtype == DTYPE_VCC_ENABLE) {
					if (0 == strncmp(cm->id, HW_LCD_VCC_LCDANALOG_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_on_seq.buf[VCI_ON_DELAY_SEQ];
					} else if (0 == strncmp(cm->id, HW_LCD_VCC_LCDIO_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_on_seq.buf[IOVCC_ON_DELAY_SEQ];
					} else if (0 == strncmp(cm->id, VCC_LCD_VSP_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_on_seq.buf[VSP_ON_DELAY_SEQ];
					} else if (0 == strncmp(cm->id, VCC_LCD_VSN_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_on_seq.buf[VSN_ON_DELAY_SEQ];
					}
				}
				cm++;
			}
			break;
		case LCD_VCC_POWER_OFF_STEP:
			for (i = 0; i < cnt; i++) {
				if (cm->dtype == DTYPE_VCC_ENABLE) {
					if (0 == strncmp(cm->id, HW_LCD_VCC_LCDANALOG_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_off_seq.buf[VCI_OFF_DELAY_SEQ];
					} else if (0 == strncmp(cm->id, HW_LCD_VCC_LCDIO_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_off_seq.buf[IOVCC_OFF_DELAY_SEQ];
					} else if (0 == strncmp(cm->id, VCC_LCD_VSP_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_off_seq.buf[VSP_OFF_DELAY_SEQ];
					} else if (0 == strncmp(cm->id, VCC_LCD_VSN_NAME, strlen(cm->id))) {
						cm->wait = lcd_info.power_off_seq.buf[VSN_OFF_DELAY_SEQ];
					}
				}
				cm++;
			}
			break;
		default:
			HISI_FB_ERR("no this regulator.\n");
	}
}

/*
*name:hw_lcd_is_default
*function:judge lcd exist or not
*/
int hw_lcd_is_default(void)
{
	if (lcd_info.lcd_compatible && !strncmp(lcd_info.lcd_compatible, HW_LCD_DEFAULT_PANEL, strlen(lcd_info.lcd_compatible))) {
		return true;
	} else {
		return false;
	}
}

/*
*name:hw_lcd_get_id
*function:power on panel
*@pdev:platform device
*/
static int hw_lcd_get_id(struct platform_device* pdev)
{
	int pulldown_value = 0;
	int pullup_value = 0;
	int lcd_status = 0;
	int lcd_id0 = 0;
	int lcd_id1 = 0;

	/*set gpio direction to out, set id0 to low*/
	gpio_cmds_tx(hw_lcd_gpio_id0_low_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id0_low_cmds));
	/*set gpio direction to input*/
	gpio_cmds_tx(hw_lcd_gpio_id0_input_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id0_input_cmds));
	/*read id0 value*/
	pulldown_value = gpio_get_value(lcd_info.lcd_id0);

	/*set gpio direction to out, set id0 to high*/
	gpio_cmds_tx(hw_lcd_gpio_id0_high_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id0_high_cmds));
	/*set gpio direction to input*/
	gpio_cmds_tx(hw_lcd_gpio_id0_input_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id0_input_cmds));
	/*read id0 value*/
	pullup_value = gpio_get_value(lcd_info.lcd_id0);

	if (pulldown_value != pullup_value) {
		lcd_id0 = 2; //floating
	} else {
		lcd_id0 = pulldown_value; //high or low
	}

	/*set gpio direction to out, set id1 to low*/
	gpio_cmds_tx(hw_lcd_gpio_id1_low_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id1_low_cmds));
	/*set gpio direction to input*/
	gpio_cmds_tx(hw_lcd_gpio_id1_input_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id1_input_cmds));
	/*read id1 value*/
	pulldown_value = gpio_get_value(lcd_info.lcd_id1);

	/*set gpio direction to out, set id1 to low*/
	gpio_cmds_tx(hw_lcd_gpio_id1_high_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id1_high_cmds));
	/*set gpio direction to input*/
	gpio_cmds_tx(hw_lcd_gpio_id1_input_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id1_input_cmds));
	/*read id1 value*/
	pullup_value = gpio_get_value(lcd_info.lcd_id1);

	if (pulldown_value != pullup_value) {
		lcd_id1 = 2; //floating
	} else {
		lcd_id1 = pulldown_value; //high or low
	}

	lcd_status = (lcd_id0 | (lcd_id1 << 2));
	HISI_FB_INFO("lcd_id0:%d, lcd_id1:%d, lcd_status = 0x%x.\n", lcd_id0, lcd_id1, lcd_status);
	return lcd_status;
}

/*
*name:hw_lcd_enter_ulps
*function:enter Ultra-Low Power State
*@hisifd:hisi fb data type fd
*/
static int hw_lcd_enter_ulps(struct hisi_fb_data_type *hisifd)
{
	BUG_ON(hisifd == NULL);

	/* switch to cmd mode */
	set_reg(hisifd->mipi_dsi0_base + MIPIDSI_MODE_CFG_OFFSET, 0x1, 1, 0);
	/* cmd mode: low power mode */
	set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x7f, 7, 8);
	set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0xf, 4, 16);
	set_reg(hisifd->mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 24);

	/* disable generate High Speed clock */
	set_reg(hisifd->mipi_dsi0_base + MIPIDSI_LPCLK_CTRL_OFFSET, 0x0, 1, 0);
	mipi_dsi_ulps_cfg(hisifd, 0);
	udelay(10);
	return 0;
}

/*
*name:hw_lcd_is_enter_sleep_mode
*function:judge enter sleep or not
*/
static int hw_lcd_is_enter_sleep_mode(void)
{
	return (gesture_func || g_enable_PT_test);
}

/*
*name:hw_lcd_is_ctl_tp_power
*function:judge lcd control tp power or not
*/
static int hw_lcd_is_ctl_tp_power(void)
{
	return g_lcd_control_tp_power;
}

/*
*name:hw_lcd_read_power_status
*function:read lcd power on status
*@hisifd:hisi fb data type fd
*/
static void hw_lcd_read_power_status(struct hisi_fb_data_type* hisifd)
{
	uint32_t status = 0;
	uint32_t try_times = 0;
	// check lcd power state
	if (lcd_info.read_power_status) {
		outp32(hisifd->mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A06);
		status = inp32(hisifd->mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		while (status & 0x10) {
			udelay(50);
			if (++try_times > 100) {
				try_times = 0;
				HISI_FB_ERR("Read lcd power status timeout!\n");
				HISI_FB_ERR("DSI CMD PKT STATUS = 0x%x\n", status);
				break;
			}

			status = inp32(hisifd->mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
		}
		status = inp32(hisifd->mipi_dsi0_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
		HISI_FB_INFO("Read LCD panel status, 0x0A = 0x%x.\n", status);
	}

}

/*
*name:hw_lcd_get_on_time
*function:get current time
*@void
*/
static void hw_lcd_get_on_time(void)
{
	do_gettimeofday(&lcd_init_done);
}

/*
*name:hw_lcd_delay_on
*function:delay on
*@void
*/
static void hw_lcd_delay_on(uint32_t bl_level)
{
	static uint32_t last_bl_level = 255;
	int interval = 0;
	struct timeval bl_on;

	if (last_bl_level == 0 && bl_level != 0) {
		if (lcd_info.power_on_seq.buf[PANEL_ON_TO_BL_ON_DELAY_SEQ] > 0) {
			do_gettimeofday(&bl_on);
			interval = (bl_on.tv_sec - lcd_init_done.tv_sec)*1000 + (bl_on.tv_usec - lcd_init_done.tv_usec)/1000;
			if (interval < lcd_info.power_on_seq.buf[PANEL_ON_TO_BL_ON_DELAY_SEQ] && interval > 0) {
				mdelay(lcd_info.power_on_seq.buf[PANEL_ON_TO_BL_ON_DELAY_SEQ] - interval);
			}
			HISI_FB_DEBUG("interval=%d\n", interval);
		}
	}
	last_bl_level = bl_level;
}

/*
*name:hw_lcd_delay_on
*function:delay off
*@void
*/
static void hw_lcd_delay_off(void)
{
	int interval = 0;
	struct timeval panel_off;

	if (lcd_info.power_on_seq.buf[PANEL_ON_TO_BL_ON_DELAY_SEQ] > 0) {
		do_gettimeofday(&panel_off);
		interval = (panel_off.tv_sec - lcd_init_done.tv_sec)*1000 + (panel_off.tv_usec - lcd_init_done.tv_usec)/1000;
		if (interval < lcd_info.power_on_seq.buf[PANEL_ON_TO_BL_ON_DELAY_SEQ] && interval > 0) {
			mdelay(lcd_info.power_on_seq.buf[PANEL_ON_TO_BL_ON_DELAY_SEQ] - interval);
		}
		HISI_FB_DEBUG("interval=%d\n", interval);
	}
}

/*
*name:hw_lcd_on
*function:power on panel
*@pdev:platform device
*/
static int hw_lcd_on(struct platform_device* pdev)
{
	struct hisi_fb_data_type* hisifd = NULL;
	struct hisi_panel_info* pinfo = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	int error = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_INFO("fb%d, +!\n", hisifd->index);

#if HW_LCD_DEBUG
	if(is_enable_mipiclk_debug()){
		hisifd->panel_info.mipi.dsi_bit_clk = get_mipiclk_debug();
		hisifd->panel_info.mipi.dsi_bit_clk_upt = get_mipiclk_debug();
	}
#endif
	pinfo = &(hisifd->panel_info);

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		g_debug_enable = BACKLIGHT_PRINT_TIMES;
		LOG_JANK_D(JLID_KERNEL_LCD_POWER_ON, "%s", "JL_KERNEL_LCD_POWER_ON");
#if HW_LCD_DEBUG
	if (is_enable_vsp_vsn_debug()){
		lcd_debug_set_vsp_vsn(hw_lcd_scharger_vcc_set_cmds, ARRAY_SIZE(hw_lcd_scharger_vcc_set_cmds));
			/*set scharger vcc*/
		(void)vcc_cmds_tx(NULL, hw_lcd_scharger_vcc_set_cmds, \
					ARRAY_SIZE(hw_lcd_scharger_vcc_set_cmds));
	}
#endif
		if (!hw_lcd_is_enter_sleep_mode()) {
			//lcd vcc enable
			if (hw_lcd_is_ctl_tp_power()) {
				(void)vcc_cmds_tx(pdev, hw_lcdanalog_vcc_enable_cmds,
				            ARRAY_SIZE(hw_lcdanalog_vcc_enable_cmds));
			}

			(void)vcc_cmds_tx(pdev, hw_lcdio_vcc_enable_cmds,
						ARRAY_SIZE(hw_lcdio_vcc_enable_cmds));

			// lcd pinctrl normal
			pinctrl_cmds_tx(pdev, hw_lcd_pinctrl_normal_cmds,
							ARRAY_SIZE(hw_lcd_pinctrl_normal_cmds));

			// lcd gpio request
			gpio_cmds_tx(hw_lcd_gpio_request_cmds, \
						 ARRAY_SIZE(hw_lcd_gpio_request_cmds));

			if (POWER_CTRL_BY_GPIO == lcd_info.power_ctrl_mode) {
				gpio_cmds_tx(lcd_bias_request_cmds, \
							ARRAY_SIZE(lcd_bias_request_cmds));
			}

			// lcd gpio normal
			if (!hw_lcd_gpio_seq_init(hw_lcd_gpio_normal_cmds, LCD_GPIO_RESET_ON_STEP1)) {
				if (!hw_lcd_is_enter_sleep_mode()) {
					gpio_cmds_tx(hw_lcd_gpio_normal_cmds, \
								 ARRAY_SIZE(hw_lcd_gpio_normal_cmds));
				}
			}

			if (POWER_CTRL_BY_GPIO == lcd_info.power_ctrl_mode) {
				gpio_cmds_tx(lcd_bias_enable_cmds, \
							ARRAY_SIZE(lcd_bias_enable_cmds));
				HISI_FB_INFO("lcd is ctrl by gpio! lcd_info.power_ctrl_mode = %d\n", lcd_info.power_ctrl_mode);
			}

			if (POWER_CTRL_BY_REGULATOR == lcd_info.power_ctrl_mode) {
				//vsp/vsn enable
				(void)vcc_cmds_tx(NULL, hw_lcd_scharger_vcc_enable_cmds, \
							ARRAY_SIZE(hw_lcd_scharger_vcc_enable_cmds));
				HISI_FB_INFO("lcd is ctrl by regulator! lcd_info.power_ctrl_mode = %d\n", lcd_info.power_ctrl_mode);
			}
		} else {
			HISI_FB_DEBUG("GESTURE&&PT STATION TEST\n");
			// lcd pinctrl normal
			pinctrl_cmds_tx(pdev, hw_lcd_pinctrl_normal_cmds,
							ARRAY_SIZE(hw_lcd_pinctrl_normal_cmds));

			// lcd gpio request
			gpio_cmds_tx(hw_lcd_gpio_request_cmds, \
						 ARRAY_SIZE(hw_lcd_gpio_request_cmds));
		}

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		// lcd gpio normal
		if (!hw_lcd_gpio_seq_init(hw_lcd_gpio_normal_cmds, LCD_GPIO_RESET_ON_STEP2)) {
			/*after LP11 delay*/
			mdelay(lcd_info.power_on_seq.buf[LP11_DELAY_SEQ]);
			if (!hw_lcd_is_enter_sleep_mode()) {
				gpio_cmds_tx(hw_lcd_gpio_normal_cmds, \
						 	ARRAY_SIZE(hw_lcd_gpio_normal_cmds));
			}
		} else {
			/*after LP11 delay*/
			mdelay(lcd_info.power_on_seq.buf[LP11_DELAY_SEQ]);
		}

#ifdef CONFIG_HUAWEI_TS
		if (hw_lcd_is_ctl_tp_power()) {
			error = ts_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts resume device err\n");
			mdelay(lcd_info.power_on_seq.buf[TP_RESET_TO_LCD_INIT_DELAY_SEQ]);
		}
#endif

#if HW_LCD_DEBUG
		if(is_enable_initcode_debug()) {
			mipi_dsi_cmds_tx(g_panel_cmds.cmds, \
			                g_panel_cmds.cmd_cnt, mipi_dsi0_base);
		} else
#endif
		mipi_dsi_cmds_tx(lcd_info.display_on_cmds.cmd_set, \
		                lcd_info.display_on_cmds.cmd_cnt, mipi_dsi0_base);

		lcd_info.cabc_mode = CABC_UI_MODE;
		g_sre_enable = 0;

#if HW_LCD_POWER_STATUS_CHECK
		hw_lcd_read_power_status(hisifd);
#endif
		HISI_FB_INFO("lcd name = %s.\n", lcd_info.panel_name);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
#ifdef CONFIG_HUAWEI_TS
		if (hw_lcd_is_ctl_tp_power()) {
			error = ts_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts after resume err\n");
		}
#endif
		hw_lcd_get_on_time();
	} else {
		HISI_FB_ERR("failed to init lcd!\n");
	}

	// backlight on
	hisi_lcd_backlight_on(pdev);

	HISI_FB_INFO("fb%d, -!\n", hisifd->index);

	return 0;
}

/*
*name:hw_lcd_off
*function:power off panel
*@pdev:platform device
*/
static int hw_lcd_off(struct platform_device* pdev)
{
	struct hisi_fb_data_type* hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int error = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_INFO("fb%d, +!\n", hisifd->index);

	pinfo = &(hisifd->panel_info);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "JL_KERNEL_LCD_POWER_OFF");

		// backlight off
		hisi_lcd_backlight_off(pdev);
		/*panel delay off*/
		hw_lcd_delay_off();

		// lcd display off sequence
		mipi_dsi_cmds_tx(lcd_info.display_off_cmds.cmd_set, \
		                 lcd_info.display_off_cmds.cmd_cnt, hisifd->mipi_dsi0_base);
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		if (hw_lcd_is_enter_sleep_mode()) {
			hw_lcd_enter_ulps(hisifd);
		}
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
#ifdef CONFIG_HUAWEI_TS
		//if g_debug_enable_lcd_sleep_in == 1, it means don't turn off TP/LCD power
		//but only let lcd get into sleep.
		if (hw_lcd_is_ctl_tp_power() && !hisifd->fb_shutdown) {
			error = ts_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts before suspend err\n");

			error = ts_power_control_notify(TS_SUSPEND_DEVICE, NO_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts suspend device err\n");
		}
		if (hisifd->fb_shutdown) {
			ts_thread_stop_notify();
		}
#endif
		if (!hw_lcd_is_enter_sleep_mode() || hisifd->fb_shutdown) {
			if (!hw_lcd_gpio_seq_init(hw_lcd_gpio_lowpower_cmds, LCD_GPIO_RESET_OFF_STEP1)) {
				// lcd gpio lowpower
				gpio_cmds_tx(hw_lcd_gpio_lowpower_cmds, \
				             ARRAY_SIZE(hw_lcd_gpio_lowpower_cmds));
			}

			if (POWER_CTRL_BY_GPIO == lcd_info.power_ctrl_mode) {
				gpio_cmds_tx(lcd_bias_disable_cmds, \
							 ARRAY_SIZE(lcd_bias_disable_cmds));
				gpio_cmds_tx(lcd_bias_free_cmds, ARRAY_SIZE(lcd_bias_free_cmds));
			}
			if(POWER_CTRL_BY_REGULATOR == lcd_info.power_ctrl_mode) {
				//vsp/vsn disable
				(void)vcc_cmds_tx(NULL, hw_lcd_scharger_vcc_disable_cmds, \
				ARRAY_SIZE(hw_lcd_scharger_vcc_disable_cmds));
			}

			if (!hw_lcd_gpio_seq_init(hw_lcd_gpio_lowpower_cmds, LCD_GPIO_RESET_OFF_STEP2)) {
				// lcd gpio lowpower
				gpio_cmds_tx(hw_lcd_gpio_lowpower_cmds, \
				             ARRAY_SIZE(hw_lcd_gpio_lowpower_cmds));
			}

			// lcd gpio free
			gpio_cmds_tx(hw_lcd_gpio_free_cmds, \
			             ARRAY_SIZE(hw_lcd_gpio_free_cmds));

			// lcd pinctrl lowpower
			pinctrl_cmds_tx(pdev, hw_lcd_pinctrl_lowpower_cmds,
			                ARRAY_SIZE(hw_lcd_pinctrl_lowpower_cmds));

			//lcd vcc disable
			(void)vcc_cmds_tx(pdev, hw_lcdio_vcc_disable_cmds,
					ARRAY_SIZE(hw_lcdio_vcc_disable_cmds));

			if (hw_lcd_is_ctl_tp_power()) {
				(void)vcc_cmds_tx(pdev, hw_lcdanalog_vcc_disable_cmds,
				            ARRAY_SIZE(hw_lcdanalog_vcc_disable_cmds));
			}

		} else {
			// lcd gpio free
			gpio_cmds_tx(hw_lcd_gpio_free_cmds, \
			             ARRAY_SIZE(hw_lcd_gpio_free_cmds));
			// lcd pinctrl lowpower
			pinctrl_cmds_tx(pdev, hw_lcd_pinctrl_lowpower_cmds,
			                ARRAY_SIZE(hw_lcd_pinctrl_lowpower_cmds));
		}
	} else {
		HISI_FB_ERR("failed to uninit lcd!\n");
	}

	//update lcd fps
	pinfo->fps = pinfo->fps_updt = 60;

	HISI_FB_INFO("fb%d, -!\n", hisifd->index);

	return 0;
}

/*
*name:hw_lcd_remove
*function:panel remove
*@pdev:platform device
*/
static int hw_lcd_remove(struct platform_device* pdev)
{
	struct hisi_fb_data_type* hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (!hisifd) {
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	// lcd vcc finit
	(void)vcc_cmds_tx(pdev, hw_lcd_vcc_finit_cmds,
	            ARRAY_SIZE(hw_lcd_vcc_finit_cmds));

	// scharger vcc finit
	(void)vcc_cmds_tx(pdev, hw_lcd_scharger_vcc_put_cmds,
	            ARRAY_SIZE(hw_lcd_scharger_vcc_put_cmds));

	// lcd pinctrl finit
	pinctrl_cmds_tx(pdev, hw_lcd_pinctrl_finit_cmds,
	                ARRAY_SIZE(hw_lcd_pinctrl_finit_cmds));

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

/*
*name:hw_lcd_set_backlight
*function:set backlight level
*@pdev:platform device
*/
static int hw_lcd_set_backlight(struct platform_device* pdev, uint32_t bl_level)
{
	int ret = 0;
	char __iomem* mipi_dsi0_base = NULL;
	struct hisi_fb_data_type* hisifd = NULL;
	static char last_bl_level = 255;
	u32 level = 0;

	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	char bl_level_adjust1[3] = {
		0x51,
		0x00, 0x00,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{
			DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust
		},
	};

	struct dsi_cmd_desc lcd_bl_level_adjust1[] = {
		{
			DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust1), bl_level_adjust1
		},
	};

	char unlock_payload[3] = {
		0x99,
		0x95, 0x27
	};

	struct dsi_cmd_desc unlock_cmd[] = {
		{
			DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(unlock_payload), unlock_payload
		},
	};

	char lock_payload[3] = {
		0x99,
		0x00, 0x00
	};

	struct dsi_cmd_desc lock_cmd[] = {
		{
			DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(lock_payload), lock_payload
		},
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	HISI_FB_DEBUG("fb%d, bl_level=%d.\n", hisifd->index, bl_level);

	if (unlikely(g_debug_enable)) {
		HISI_FB_INFO("Set backlight to %d. (remain times of backlight print: %d)\n", hisifd->bl_level, g_debug_enable);
		if (g_debug_enable == BACKLIGHT_PRINT_TIMES)
			LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_ON, "JL_KERNEL_LCD_BACKLIGHT_ON,%u", hisifd->bl_level);

		g_debug_enable = (g_debug_enable > 0) ? (g_debug_enable - 1) : 0;
	}

	/*panel on delay*/
	hw_lcd_delay_on(bl_level);

	if (hisifd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		ret = hisi_pwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {
		ret = hisi_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM) {
		ret = hisi_sh_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		mipi_dsi0_base = hisifd->mipi_dsi0_base;

		bl_level = (bl_level < hisifd->panel_info.bl_max) ? bl_level : hisifd->panel_info.bl_max;
		bl_level_adjust[1] = bl_level  * 255 / hisifd->panel_info.bl_max;
		level = bl_level_adjust[1];
		/*unlock command one*/
		if (lcd_info.lock_cmd_support) {
			mipi_dsi_cmds_tx(unlock_cmd, \
				ARRAY_SIZE(unlock_cmd), mipi_dsi0_base);
		}
		/*set bl command*/
		if (!strcmp(lcd_info.panel_name, "AUO_ILI9885 5.5' VIDEO TFT 1080 x 1920"))
		{

			bl_level_adjust1[1] = ((level&0xF0)>>4) & 0x0F;
			bl_level_adjust1[2] = ((level&0x0F)<<4) | 0x0F;
			mipi_dsi_cmds_tx(lcd_bl_level_adjust1, \
				ARRAY_SIZE(lcd_bl_level_adjust1), mipi_dsi0_base);
		}
		else
		{
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
				ARRAY_SIZE(lcd_bl_level_adjust), mipi_dsi0_base);
		}
		/*lock command one*/
		if (lcd_info.lock_cmd_support) {
			mipi_dsi_cmds_tx(lock_cmd, \
				ARRAY_SIZE(lock_cmd), mipi_dsi0_base);
		}
		/*enable/disable backlight*/
		down(&lcd_info.bl_sem);
		if (bl_level == 0 && last_bl_level != 0) {
			(void)vcc_cmds_tx(NULL, hw_lcd_scharger_bl_disable_cmds, \
					ARRAY_SIZE(hw_lcd_scharger_bl_disable_cmds));
		} else if (last_bl_level == 0 && bl_level != 0) {
			(void)vcc_cmds_tx(NULL, hw_lcd_scharger_bl_enable_cmds, \
					ARRAY_SIZE(hw_lcd_scharger_bl_enable_cmds));
		}
		last_bl_level = bl_level;
		up(&lcd_info.bl_sem);
	} else {
		HISI_FB_ERR("fb%d, not support this bl_set_type(%d)!\n",
		            hisifd->index, hisifd->panel_info.bl_set_type);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_set_fastboot
*function:set fastboot display
*@pdev:platform device
*/
static int hw_lcd_set_fastboot(struct platform_device* pdev)
{
	struct hisi_fb_data_type* hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (POWER_CTRL_BY_REGULATOR == lcd_info.power_ctrl_mode) {
		/*set scharger vcc*/
		(void)vcc_cmds_tx(NULL, hw_lcd_scharger_vcc_set_cmds, \
		            ARRAY_SIZE(hw_lcd_scharger_vcc_set_cmds));

		/*scharger vcc enable*/
		(void)vcc_cmds_tx(NULL, hw_lcd_scharger_vcc_enable_cmds, \
		            ARRAY_SIZE(hw_lcd_scharger_vcc_enable_cmds));

		/*scharger bl enable*/
		(void)vcc_cmds_tx(NULL, hw_lcd_scharger_bl_enable_cmds, \
		            ARRAY_SIZE(hw_lcd_scharger_bl_enable_cmds));
	}
	// lcd pinctrl normal
	pinctrl_cmds_tx(pdev, hw_lcd_pinctrl_normal_cmds,
	                ARRAY_SIZE(hw_lcd_pinctrl_normal_cmds));

	// lcd gpio request
	gpio_cmds_tx(hw_lcd_gpio_request_cmds,
	             ARRAY_SIZE(hw_lcd_gpio_request_cmds));

	if (POWER_CTRL_BY_GPIO == lcd_info.power_ctrl_mode) {
		gpio_cmds_tx(lcd_bias_request_cmds, ARRAY_SIZE(lcd_bias_request_cmds));
	}
	// backlight on
	hisi_lcd_backlight_on(pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

/*
*name:hw_lcd_model_show
*function:show lcd information
*@pdev:platform device
*/
static ssize_t hw_lcd_model_show(struct platform_device* pdev,
                                 char* buf)
{
	struct hisi_fb_data_type* hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%s\n", lcd_info.panel_name);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_cabc_mode_show
*function:show cabc mode
*@pdev:platform device
*/
static ssize_t hw_lcd_cabc_mode_show(struct platform_device* pdev,
                                     char* buf)
{
	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%d\n", lcd_info.cabc_mode);
}

/*
*name:hw_lcd_cabc_mode_store
*function:set cabc mode
*@pdev:platform device
*/
static ssize_t hw_lcd_cabc_mode_store(struct platform_device* pdev,
                                      const char* buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int mode = -1;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base =hisifd->mipi_dsi0_base;

	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		return ret;
	}

	mode = (int)val;
	if (hisifd->panel_info.sre_support && hisifd->sre_enable) {
		HISI_FB_INFO("sre is enable, can't set cabc.\n");
		lcd_info.cabc_mode = mode;
		return count;
	}

	if (mode == CABC_OFF ) {
			  lcd_info.cabc_mode = CABC_OFF;
			  mipi_dsi_cmds_tx(lcd_info.cabc_off_cmds.cmd_set, \
							   lcd_info.cabc_off_cmds.cmd_cnt,\
							   mipi_dsi0_base);
	}else  if (mode == CABC_UI_MODE) {
			  lcd_info.cabc_mode = CABC_UI_MODE;
			  mipi_dsi_cmds_tx(lcd_info.cabc_ui_cmds.cmd_set, \
							   lcd_info.cabc_ui_cmds.cmd_cnt,\
							   mipi_dsi0_base);
	} else if (mode == CABC_STILL_MODE ) {
			  lcd_info.cabc_mode = CABC_STILL_MODE;
			  mipi_dsi_cmds_tx(lcd_info.cabc_still_cmds.cmd_set, \
							   lcd_info.cabc_still_cmds.cmd_cnt,\
							   mipi_dsi0_base);
	}else if (mode == CABC_MOVING_MODE ) {
			  lcd_info.cabc_mode = CABC_MOVING_MODE;
			  mipi_dsi_cmds_tx(lcd_info.cabc_moving_cmds.cmd_set, \
							   lcd_info.cabc_moving_cmds.cmd_cnt,\
							   mipi_dsi0_base);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

static int hw_lcd_sbl_ctrl(struct platform_device *pdev, int enable)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (!hisifd->panel_info.sre_support) {
		return ret;
	}

	mipi_dsi0_base =hisifd->mipi_dsi0_base;

	/*enable sre*/
	if (enable) {
		/*enable sre, and disable cabc*/
		mipi_dsi_cmds_tx(lcd_info.sre_on_cmds.cmd_set, \
							lcd_info.sre_on_cmds.cmd_cnt,\
							mipi_dsi0_base);
		g_sre_enable = 1;		
	}else {
        if (g_sre_enable) {
    		//disable sre
    		mipi_dsi_cmds_tx(lcd_info.sre_off_cmds.cmd_set, \
    							lcd_info.sre_off_cmds.cmd_cnt,\
    							mipi_dsi0_base);
    		/*restore cabc func*/
    		if (lcd_info.cabc_mode == CABC_OFF ) {
    				  mipi_dsi_cmds_tx(lcd_info.cabc_off_cmds.cmd_set, \
    								   lcd_info.cabc_off_cmds.cmd_cnt,\
    								   mipi_dsi0_base);
    		}else  if (lcd_info.cabc_mode == CABC_UI_MODE) {
    				  mipi_dsi_cmds_tx(lcd_info.cabc_ui_cmds.cmd_set, \
    								   lcd_info.cabc_ui_cmds.cmd_cnt,\
    								   mipi_dsi0_base);
    		} else if (lcd_info.cabc_mode == CABC_STILL_MODE ) {
    				  mipi_dsi_cmds_tx(lcd_info.cabc_still_cmds.cmd_set, \
    								   lcd_info.cabc_still_cmds.cmd_cnt,\
    								   mipi_dsi0_base);
    		}else if (lcd_info.cabc_mode == CABC_MOVING_MODE ) {
    				  mipi_dsi_cmds_tx(lcd_info.cabc_moving_cmds.cmd_set, \
    								   lcd_info.cabc_moving_cmds.cmd_cnt,\
    								   mipi_dsi0_base);
    		}
            g_sre_enable = 0;
		}
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_inversion_store
*function:set dot/column inversion
*@pdev:platform device
*/
static ssize_t hw_lcd_inversion_store(struct platform_device* pdev,
                                      const char* buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long val = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	struct device_node* np = NULL;
	static int para_parse_flag = 0;
	char __iomem *ldi_base = NULL;
	int switch_mode = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		HISI_FB_ERR("strict_strtoul error, buf=%s", buf);
		return ret;
	}

	lcd_info.inversion_mode = (int)val;

	if (!para_parse_flag) {
		para_parse_flag = 1;
		np = of_find_compatible_node(NULL, NULL, lcd_info.lcd_compatible);
		if (!np) {
			HISI_FB_ERR("NOT FOUND device node %s!\n", lcd_info.lcd_compatible);
			ret = -1;
			return ret;
		}

		/*Parse mipi tr dot inversion cmds*/
		ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-dot-inversion", &lcd_info.dot_inversion_cmds);
		if (ret) {
			HISI_FB_ERR("parse hisilicon,dss-dot-inversion failed!\n");
			ret = -1;
			return ret;
		}

		/*Parse mipi tr column inversion cmds*/
		ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-column-inversion", &lcd_info.column_inversion_cmds);
		if (ret) {
			HISI_FB_ERR("parse hisilicon,dss-column-inversion failed!\n");
			ret = -1;
			return ret;
		}
	}

	//switch to low-power mode
	if (is_mipi_cmd_panel(hisifd)) {
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 14);
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 9);
	} else {
		if (!(0x8000&inp32(hisifd->mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET))) {
			set_reg(mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x1, 1, 15);
			switch_mode = 1;
		}
	}

	//switch dot and column inversion
	if (lcd_info.inversion_mode == INVERSION_COLUMN ) {
		mipi_dsi_cmds_tx(lcd_info.column_inversion_cmds.cmd_set, \
		                 lcd_info.column_inversion_cmds.cmd_cnt, mipi_dsi0_base);
	} else  if (lcd_info.inversion_mode == INVERSION_DOT) {
		mipi_dsi_cmds_tx(lcd_info.dot_inversion_cmds.cmd_set, \
		                 lcd_info.dot_inversion_cmds.cmd_cnt, mipi_dsi0_base);
	}

	//switch to high-speed mode
	if (is_mipi_cmd_panel(hisifd)) {
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 14);
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 9);
	} else {
		if (switch_mode == 1) {
			set_reg(mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 15);
		}
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

/*
*name:hw_lcd_inversion_show
*function:show dot/column mode
*@pdev:platform device
*/
static ssize_t hw_lcd_inversion_show(struct platform_device* pdev, char* buf)
{
	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%d\n", lcd_info.inversion_mode);
}

/*
*name:hw_lcd_scan_store
*function:set scan mode
*@pdev:platform device
*/
static ssize_t hw_lcd_scan_store(struct platform_device* pdev,
                                      const char* buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long val = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	struct device_node* np = NULL;
	static int para_parse_flag = 0;
	char __iomem *ldi_base = NULL;
	int switch_mode = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (hisifd->index == PRIMARY_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI0_OFFSET;
	} else if (hisifd->index == EXTERNAL_PANEL_IDX) {
		ldi_base = hisifd->dss_base + DSS_LDI1_OFFSET;
	} else {
		HISI_FB_ERR("fb%d, not support!", hisifd->index);
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		HISI_FB_ERR("strict_strtoul error, buf=%s", buf);
		return ret;
	}

	lcd_info.scan_mode = (int)val;

	if (!para_parse_flag) {
		para_parse_flag = 1;
		np = of_find_compatible_node(NULL, NULL, lcd_info.lcd_compatible);
		if (!np) {
			HISI_FB_ERR("NOT FOUND device node %s!\n", lcd_info.lcd_compatible);
			ret = -1;
			return ret;
		}

		/*Parse mipi tr revert scan cmds*/
		ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-revert-scan", &lcd_info.revert_scan_cmds);
		if (ret) {
			HISI_FB_ERR("parse hisilicon,dss-revert-scan failed!\n");
			ret = -1;
			return ret;
		}

		/*Parse mipi tr forword scan cmds*/
		ret = hw_lcd_parse_dcs_cmds(np, "hisilicon,dss-forword-scan", &lcd_info.forword_scan_cmds);
		if (ret) {
			HISI_FB_ERR("parse hisilicon,dss-forword-scan failed!\n");
			ret = -1;
			return ret;
		}
	}

	//switch to low-power mode
	if (is_mipi_cmd_panel(hisifd)) {
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 14);
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x1, 1, 9);
	} else {
		if (!(0x8000&inp32(hisifd->mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET))) {
			set_reg(mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x1, 1, 15);
			switch_mode = 1;
		}
	}

	//switch revert and scan mode
	if (lcd_info.scan_mode == SCAN_TYPE_FORWORD) {
		mipi_dsi_cmds_tx(lcd_info.forword_scan_cmds.cmd_set, \
		                 lcd_info.forword_scan_cmds.cmd_cnt, mipi_dsi0_base);
		HISI_FB_INFO("SCAN_TYPE_FORWORD!\n");
	} else  if (lcd_info.scan_mode == SCAN_TYPE_REVERT) {
		mipi_dsi_cmds_tx(lcd_info.revert_scan_cmds.cmd_set, \
		                 lcd_info.revert_scan_cmds.cmd_cnt, mipi_dsi0_base);
		HISI_FB_INFO("SCAN_TYPE_REVERT!\n");
	}

	//switch to high-speed mode
	if (is_mipi_cmd_panel(hisifd)) {
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 14);
		set_reg(mipi_dsi0_base + MIPIDSI_CMD_MODE_CFG_OFFSET, 0x0, 1, 9);
	} else {
		if (switch_mode == 1) {
			set_reg(mipi_dsi0_base + MIPIDSI_VID_MODE_CFG_OFFSET, 0x0, 1, 15);
		}
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

/*
*name:hw_lcd_scan_show
*function:show scan mode
*@pdev:platform device
*/
static ssize_t hw_lcd_scan_show(struct platform_device* pdev, char* buf)
{
	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%d\n", lcd_info.scan_mode);
}

/*
*name:hw_lcd_check_reg_show
*function:read lcd reg status
*@pdev:platform device
*/
static ssize_t hw_lcd_check_reg_show(struct platform_device* pdev, char* buf)
{
	int ret = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	uint32_t read_value = 0, expect_value = 0;
	uint32_t i = 0;
	char lcd_reg[] = {0x00};
	char* reg_ptr = NULL, *expect_ptr = NULL;


	struct dsi_cmd_desc lcd_check_reg[] = {
		{
			DTYPE_GEN_READ1, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg), lcd_reg
		}
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	lcd_check_reg[0].dtype = lcd_info.read_data_type;

	reg_ptr = lcd_info.mipi_check_reg.buf;
	expect_ptr = lcd_info.mipi_check_value.buf;
	for (i = 0; i < lcd_info.mipi_check_reg.cnt; i++) {
		lcd_check_reg[0].payload = reg_ptr++;
		ret = mipi_dsi_cmds_rx(&read_value, lcd_check_reg, sizeof(lcd_reg), mipi_dsi0_base);
		if (ret) {
			HISI_FB_ERR("Read error number: %d\n", ret);
			ret = 1;
			break;
		}
		expect_value = *expect_ptr++;
		if (read_value != expect_value) {
			HISI_FB_ERR("Read reg does not match expect value,read value: %d, expect value:%d\n", read_value, expect_value);
			ret = 1;
			break;
		} else {
			HISI_FB_DEBUG("Read success, read value: %d, expect value:%d\n", read_value, expect_value);
		}
	}
	if (!ret) {
		ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "OK\n");
	} else {
		ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "ERROR\n");
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_mipi_detect_show
*function:detect mipi status
*@pdev:platform device
*/
static ssize_t hw_lcd_mipi_detect_show(struct platform_device* pdev, char* buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	uint32_t read_value[2] = {0};
	uint32_t expected_value[2] = {0x80, 0x00};
	uint32_t read_mask[2] = {0xFF, 0xFF};
	char* reg_name[2] = {"signal mode", "dsi error number"};
	char lcd_reg_0e[] = {0x0e};
	char lcd_reg_05[] = {0x05};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{
			DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e
		},
		{
			DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_05), lcd_reg_05
		},
	};

	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = 1,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (!mipi_dsi_read_compare(&data, mipi_dsi0_base)) {
		ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "OK\n");
	} else {
		ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "ERROR\n");
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_hkadc_show
*function:show hkadc voltage
*@pdev:platform device
*/
static ssize_t hw_lcd_hkadc_show(struct platform_device* pdev, char* buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%d\n", hkadc_buf * 4);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_hkadc_store
*function:set hkadc channel
*@pdev:platform device
*/
static ssize_t hw_lcd_hkadc_store(struct platform_device* pdev,
                                        const char* buf, size_t count)
{
	int ret = 0;
	int channel = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = sscanf(buf, "%u", &channel);
	if (ret <= 0) {
		HISI_FB_ERR("Sscanf return invalid, ret = %d\n", ret);
		return count;
	}

	hkadc_buf = hisi_adc_get_value(channel);
	HISI_FB_INFO("channel[%d] value is %d\n", channel, hkadc_buf);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

/*
*name:hw_lcd_gram_check_show
*function:check gram checksum
*@pdev:platform device
*/
static ssize_t hw_lcd_gram_check_show(struct platform_device* pdev, char* buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	uint32_t rd[CHECKSUM_SIZE] = {0};
	int i = 0;
	char cmdF_page0_select[] = {0xFF, 0xF0};
	char cmd1_page0_select[] = {0xFF, 0x10};
	char checksum_read[] = {0x73};

	struct dsi_cmd_desc packet_size_set_cmd = {DTYPE_MAX_PKTSIZE, 0, 10, WAIT_TYPE_US, 1, NULL};

	struct dsi_cmd_desc lcd_checksum_select_cmds[] = {
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmdF_page0_select), cmdF_page0_select
		},
	};

	struct dsi_cmd_desc lcd_checksum_dis_select_cmds[] = {
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmd1_page0_select), cmd1_page0_select
		},
	};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{
			DTYPE_DCS_READ, 0, 20, WAIT_TYPE_US,
			sizeof(checksum_read), checksum_read
		},
	};

	if (!checksum_enable_ctl) {
		HISI_FB_INFO("Checksum disabled\n");
		return ret;
	}

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_INFO("fb%d, +.\n", hisifd->index);

	mipi_dsi_max_return_packet_size(&packet_size_set_cmd, mipi_dsi0_base);

	mipi_dsi_cmds_tx(lcd_checksum_select_cmds, \
	                 ARRAY_SIZE(lcd_checksum_select_cmds), mipi_dsi0_base);
	for (i = 0; i < CHECKSUM_SIZE; i++) {
		char* data = lcd_check_reg[0].payload;
		*data = 0x73 + i;
		(void)mipi_dsi_cmds_rx((rd + i), lcd_check_reg, \
		                 ARRAY_SIZE(lcd_check_reg), mipi_dsi0_base);
	}

	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "%d %d %d %d %d %d %d %d\n", \
	               rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7]);
	HISI_FB_INFO("%d %d %d %d %d %d %d %d\n", \
	             rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7]);

	mipi_dsi_cmds_tx(lcd_checksum_dis_select_cmds, \
	                 ARRAY_SIZE(lcd_checksum_dis_select_cmds), mipi_dsi0_base);

	HISI_FB_INFO("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_gram_check_store
*function:enable/disable gram checksum test
*@pdev:platform device
*/
static ssize_t hw_lcd_gram_check_store(struct platform_device* pdev,
                                       const char* buf, size_t count)
{
	struct hisi_fb_data_type* hisifd = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	char cmdF_page0_select[] = {0xFF, 0xF0};
	char checksum_init[] = {0x7B, 0x00};
	char checksum_ena[] = {0x92, 0x01};
	char checksum_dis[] = {0x92, 0x00};
	char cmd1_page0_select[] = {0xFF, 0x10};

	struct dsi_cmd_desc lcd_checksum_enable_cmds[] = {
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmdF_page0_select), cmdF_page0_select
		},
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(checksum_init), checksum_init
		},
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(checksum_ena), checksum_ena
		},
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmd1_page0_select), cmd1_page0_select
		},
	};

	struct dsi_cmd_desc lcd_checksum_disable_cmds[] = {
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmdF_page0_select), cmdF_page0_select
		},
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(checksum_dis), checksum_dis
		},
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmd1_page0_select), cmd1_page0_select
		},
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if ((!*buf) || (!strchr("yY1nN0", *buf))) {
		HISI_FB_ERR("Input param is error(valid: yY1nN0): %s. \n", buf);
		return -EINVAL;
	}

	checksum_enable_ctl = !!strchr("yY1", *buf);
	if (checksum_enable_ctl == true) {
		mipi_dsi_cmds_tx(lcd_checksum_enable_cmds, \
		                 ARRAY_SIZE(lcd_checksum_enable_cmds), mipi_dsi0_base);
		HISI_FB_INFO("Enable checksum\n");
	} else {
		mipi_dsi_cmds_tx(lcd_checksum_disable_cmds, \
		                 ARRAY_SIZE(lcd_checksum_disable_cmds), mipi_dsi0_base);
		HISI_FB_INFO("Disable checksum\n");
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

/*
*name:hw_lcd_set_display_region
*function:set partial display region
*@pdev:platform device
*/
static int hw_lcd_set_display_region(struct platform_device* pdev,
                                     struct dss_rect* dirty)
{
	struct hisi_fb_data_type* hisifd = NULL;

	char unlock_payload[3] = {0x99, 0x95, 0x27};
	struct dsi_cmd_desc unlock_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(unlock_payload), unlock_payload},
	};
	char lock_payload[3] = {0x99, 0x00, 0x00};
	struct dsi_cmd_desc lock_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(lock_payload), lock_payload},
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	lcd_disp_x[1] = (dirty->x >> 8) & 0xff;
	lcd_disp_x[2] = dirty->x & 0xff;
	lcd_disp_x[3] = ((dirty->x + dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = (dirty->x + dirty->w - 1) & 0xff;
	lcd_disp_y[1] = (((unsigned int)dirty->y) >> 8) & 0xff;
	lcd_disp_y[2] = ((unsigned int)dirty->y) & 0xff;
	lcd_disp_y[3] = ((dirty->y + dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = (dirty->y + dirty->h - 1) & 0xff;

	if (lcd_info.lock_cmd_support) {
		mipi_dsi_cmds_tx(unlock_cmd, ARRAY_SIZE(unlock_cmd), hisifd->mipi_dsi0_base);
	}
	mipi_dsi_cmds_tx(set_display_address, \
	                 ARRAY_SIZE(set_display_address), hisifd->mipi_dsi0_base);
	if (lcd_info.lock_cmd_support) {
		mipi_dsi_cmds_tx(lock_cmd, ARRAY_SIZE(lock_cmd), hisifd->mipi_dsi0_base);
	}
	return 0;
}

/*
*name:hw_lcd_sleep_ctrl_show
*function:show lpt station test status
*@pdev:platform device
*/
static ssize_t hw_lcd_sleep_ctrl_show(struct platform_device* pdev, char* buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "g_enable_PT_test=%d, lcd_info.pt_test_support=%d\n",
		g_enable_PT_test, lcd_info.pt_test_support);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_sleep_ctrl_store
*function:enable/disable lpt station test
*@pdev:platform device
*/
static ssize_t hw_lcd_sleep_ctrl_store(struct platform_device* pdev,
                                      const char* buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		HISI_FB_ERR("strict_strtoul error, buf=%s", buf);
		return ret;
	}

	if (lcd_info.pt_test_support) {
		g_enable_PT_test = val;
	}

	if (g_enable_PT_test == 2) {
		HISI_FB_DEBUG("Both LCD  and Touch goto sleep\n");
		g_tp_power_ctrl = 1;	//used for pt  current test, tp sleep
	} else {
		HISI_FB_DEBUG("g_enable_PT_test is %d\n", g_enable_PT_test);
		g_tp_power_ctrl = 0;	//used for pt  current test, tp power off
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

#if HW_LCD_DEBUG
/*
*name:hw_lcd_esd_debug
*function:esd check debug
*@pdev:platform device
*/
static int hw_lcd_esd_debug(struct hisi_fb_data_type* hisifd)
{
	int ret = 0;
	uint32_t i = 0;
	uint32_t read_value = 0;
	char lcd_reg[] = {0x00};
	char bl_enable[2] = {
		0x53,
		0x24,
	};

	struct dsi_cmd_desc lcd_bl_enable[] = {
		{
			DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_enable), bl_enable
		},
	};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{
			DTYPE_GEN_READ1, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg), lcd_reg
		}
	};

	if (is_mipi_cmd_panel(hisifd)) {
		lcd_check_reg[0].dtype = DTYPE_DCS_READ;
	} else {
		lcd_check_reg[0].dtype = DTYPE_GEN_READ1;
	}

	/*check reg, read reg and compire the expect value*/
	for (i = 0; i < g_esd_debug.check_count; i++) {
		lcd_check_reg[0].payload = &g_esd_debug.esd_check_reg[i];
		ret = mipi_dsi_cmds_rx(&read_value, lcd_check_reg, sizeof(lcd_reg), hisifd->mipi_dsi0_base);
		if (ret) {
			HISI_FB_ERR("Read error number: %d\n", ret);
			ret = 0;
			continue;
		}

		if (g_esd_debug.esd_check_reg[i] == 0x0e) {
			if (read_value&0x80) {
				HISI_FB_DEBUG("Esd debug:Read reg:0x0e success, read value: %d\n", read_value);
			} else {
				HISI_FB_ERR("Esd debug:Read reg:0x0e failed, read value: %d\n", read_value);
				ret = 1;
				break;
			}
		} else {
			if (read_value != g_esd_debug.esd_reg_value[i]) {
				HISI_FB_ERR("Esd debug:Read reg 0x%x does not match expect value,read value: 0x%x, expect value:0x%x\n", g_esd_debug.esd_check_reg[i], read_value, g_esd_debug.esd_reg_value[i]);
				ret = 1;
				break;
			} else {
				HISI_FB_DEBUG("Esd debug:Read 0x%x success, read value: 0x%x, expect value:0x%x\n", g_esd_debug.esd_check_reg[i], read_value, g_esd_debug.esd_reg_value[i]);
			}
		}
	}

	/*set backlight per 5s*/
	if (g_esd_debug.esd_bl_set) {
		hw_lcd_set_backlight(hisifd->pdev, hisifd->bl_level);
	}

	/*set bl enable per 5s*/
	if (g_esd_debug.esd_bl_enable) {
		mipi_dsi_cmds_tx(lcd_bl_enable, \
		                 ARRAY_SIZE(lcd_bl_enable), hisifd->mipi_dsi0_base);
	}

	return ret;
}
#endif

/*
*name:hw_lcd_esd_set_bl
*function:esd set backlight
*@pdev:platform device
*/
static void hw_lcd_esd_set_bl(struct hisi_fb_data_type* hisifd)
{
	char __iomem* mipi_dsi0_base = NULL;
	u32 level = 0;

	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};
	char bl_enable[2] = {
		0x53,
		0x24,
	};
	char bl_level_adjust1[3] = {
		0x51,
		0x00, 0x00,
	};
	struct dsi_cmd_desc lcd_set_bl_long[] = {
		{
			DTYPE_DCS_LWRITE, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust1), bl_level_adjust1
		},
	};

	struct dsi_cmd_desc lcd_set_bl[] = {
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust
		},
		{
			DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(bl_enable), bl_enable
		},
	};
	char unlock_payload[3] = {
		0x99,
		0x95, 0x27
	};
	struct dsi_cmd_desc unlock_cmd[] = {
		{
			DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(unlock_payload), unlock_payload
		},
	};
	char lock_payload[3] = {
		0x99,
		0x00, 0x00
	};
	struct dsi_cmd_desc lock_cmd[] = {
		{
			DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(lock_payload), lock_payload
		},
	};

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (hisifd->bl_level == 0) {
		return ;
	}

	if (hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		mipi_dsi0_base = hisifd->mipi_dsi0_base;
		hisifd->bl_level = (hisifd->bl_level < hisifd->panel_info.bl_max) ? hisifd->bl_level : hisifd->panel_info.bl_max;
		bl_level_adjust[1] = hisifd->bl_level  * 255 / hisifd->panel_info.bl_max;
		level = bl_level_adjust[1];
		/*unlock command one*/
		if (lcd_info.lock_cmd_support) {
			mipi_dsi_cmds_tx(unlock_cmd, \
				ARRAY_SIZE(unlock_cmd), mipi_dsi0_base);
		}
		/*set bl command*/
		if (!strcmp(lcd_info.panel_name, "AUO_ILI9885 5.5' VIDEO TFT 1080 x 1920"))
		{

			bl_level_adjust1[1] = ((level&0xF0)>>4) & 0x0F;
			bl_level_adjust1[2] = ((level&0x0F)<<4) | 0x0F;
			mipi_dsi_cmds_tx(lcd_set_bl_long, \
				ARRAY_SIZE(lcd_set_bl_long), mipi_dsi0_base);
		}
		else
		{
			mipi_dsi_cmds_tx(lcd_set_bl, \
				ARRAY_SIZE(lcd_set_bl), mipi_dsi0_base);
		}
		/*lock command one*/
		if (lcd_info.lock_cmd_support) {
			mipi_dsi_cmds_tx(lock_cmd, \
				ARRAY_SIZE(lock_cmd), mipi_dsi0_base);
		}
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ;
}

/*
*name:hw_lcd_check_esd
*function:check esd status
*@pdev:platform device
*/
static int hw_lcd_check_esd(struct platform_device* pdev)
{
	int ret = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	char __iomem* mipi_dsi0_base = NULL;
	uint32_t read_value = 0, expect_value = 0;
	char lcd_reg[] = {0x00};
	char reg_data;
	static uint32_t count = 0, check_count = 0;
	struct dsi_cmd_desc lcd_check_reg[] = {
		{
			DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg), lcd_reg
		}
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	/*esd debug*/
#if HW_LCD_DEBUG
	if (g_esd_debug.esd_enable == 1) {
		ret = hw_lcd_esd_debug(hisifd);
		return ret;
	}

	if (g_esd_debug.esd_recover_test) {
		return -1;
	}
#endif

	if (!lcd_info.esd_reg.buf || !lcd_info.esd_value.buf) {
		HISI_FB_ERR("esd reg or esd value is null\n");
		return -1;
	}

	if (count >= lcd_info.esd_reg.cnt) {
		count = 0;
	}
	reg_data = lcd_info.esd_reg.buf[count];
	expect_value = lcd_info.esd_value.buf[count];
	lcd_check_reg[0].payload = &reg_data;
	lcd_check_reg[0].dtype = lcd_info.read_data_type;
	ret = mipi_dsi_cmds_rx(&read_value, lcd_check_reg, sizeof(lcd_reg), mipi_dsi0_base);
	if (ret) {
		HISI_FB_ERR("Read error number: %d\n", ret);
		ret = 0;
		return ret;
	}
	if (read_value != expect_value) {
		HISI_FB_ERR("Read reg 0x%x does not match expect value,read value: 0x%x, expect value:0x%x\n", lcd_info.esd_reg.buf[count], read_value, expect_value);
		ret = 1;
#if defined (CONFIG_HUAWEI_DSM)
		if (check_count >= 2) {
			check_count = 0;
			if ( !dsm_client_ocuppy(lcd_dclient) ) {
				dsm_client_record(lcd_dclient, "ESD ERROR:reg 0x%x = 0x%x\n", lcd_info.esd_reg.buf[count], read_value);
				dsm_client_notify(lcd_dclient, DSM_LCD_ESD_RECOVERY_NO);
			}else{
				HISI_FB_ERR("dsm_client_ocuppy ERROR:retVal = %d\n", ret);
			}
		}
		check_count++;
#endif
		return ret;
	} else {
		HISI_FB_DEBUG("Read reg 0x%x success, read value: 0x%x, expect value:0x%x\n", lcd_info.esd_reg.buf[count], read_value, expect_value);
	}
	count++;
	/*write backlight*/
	if (lcd_info.esd_set_bl) {
		hw_lcd_esd_set_bl(hisifd);
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*
*name:hw_lcd_bl_info_show
*function:show backlight range
*@pdev:platform device
*/
static ssize_t hw_lcd_bl_info_show(struct platform_device* pdev, char* buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev NULL pointer\n");
		return 0;
	};
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd NULL pointer\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (buf) {
		ret = snprintf_s(buf, PAGE_SIZE, PAGE_SIZE - 1, "blmax:%u,blmin:%u,lcdtype:%s,\n",
				hisifd->panel_info.bl_max, hisifd->panel_info.bl_min, "LCD");
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/* only for video */
int hw_panel_lcd_fps_scence_handle(struct platform_device *pdev, uint32_t scence)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);
	pinfo = &(hisifd->panel_info);

	if ((scence & LCD_FPS_SCENCE_EBOOK) == LCD_FPS_SCENCE_EBOOK) {
		pinfo->ldi_updt.h_back_porch = pinfo->ldi_lfps.h_back_porch;
		pinfo->ldi_updt.h_front_porch = pinfo->ldi_lfps.h_front_porch;
		pinfo->ldi_updt.h_pulse_width = pinfo->ldi_lfps.h_pulse_width;
		pinfo->ldi_updt.v_back_porch = pinfo->ldi_lfps.v_back_porch;
		pinfo->ldi_updt.v_front_porch = pinfo->ldi_lfps.v_front_porch;
		pinfo->ldi_updt.v_pulse_width = pinfo->ldi_lfps.v_pulse_width;
		pinfo->fps_updt = 55;
		HISI_FB_INFO("scence is LCD_FPS_SCENCE_EBOOK, framerate is 55fps\n");
	} else {
		pinfo->ldi_updt.h_back_porch = pinfo->ldi.h_back_porch;
		pinfo->ldi_updt.h_front_porch = pinfo->ldi.h_front_porch;
		pinfo->ldi_updt.h_pulse_width = pinfo->ldi.h_pulse_width;
		pinfo->ldi_updt.v_back_porch = pinfo->ldi.v_back_porch;
		pinfo->ldi_updt.v_front_porch = pinfo->ldi.v_front_porch;
		pinfo->ldi_updt.v_pulse_width = pinfo->ldi.v_pulse_width;
		pinfo->fps_updt = 60;
		HISI_FB_INFO("scence is LCD_FPS_SCENCE_NORMAL, framerate is 60fps\n");
	}

	return 0;
}


/*******************************************************************************
**
*/
static struct hisi_panel_info hw_lcd_info = {0};
static struct hisi_fb_panel_data hw_lcd_data = {
	.panel_info = &hw_lcd_info,
	.set_fastboot = hw_lcd_set_fastboot,
	.on = hw_lcd_on,
	.off = hw_lcd_off,
	.remove = hw_lcd_remove,
	.set_backlight = hw_lcd_set_backlight,
	.lcd_model_show = hw_lcd_model_show,
	.lcd_cabc_mode_show = hw_lcd_cabc_mode_show,
	.lcd_cabc_mode_store = hw_lcd_cabc_mode_store,
	.lcd_check_reg = hw_lcd_check_reg_show,
	.lcd_mipi_detect = hw_lcd_mipi_detect_show,
	.lcd_hkadc_debug_show = hw_lcd_hkadc_show,
	.lcd_hkadc_debug_store = hw_lcd_hkadc_store,
	.lcd_gram_check_show = hw_lcd_gram_check_show,
	.lcd_gram_check_store = hw_lcd_gram_check_store,
	.set_display_region = hw_lcd_set_display_region,
	.set_display_resolution = NULL,
	.lcd_inversion_store = hw_lcd_inversion_store,
	.lcd_inversion_show = hw_lcd_inversion_show,
	.lcd_sleep_ctrl_show = hw_lcd_sleep_ctrl_show,
	.lcd_sleep_ctrl_store = hw_lcd_sleep_ctrl_store,
	.esd_handle = hw_lcd_check_esd,
	.get_lcd_id = hw_lcd_get_id,
	.panel_info_show = hw_lcd_bl_info_show,
	.lcd_scan_store = hw_lcd_scan_store,
	.lcd_scan_show = hw_lcd_scan_show,
	.lcd_fps_scence_handle = hw_panel_lcd_fps_scence_handle,
	.sbl_ctrl = hw_lcd_sbl_ctrl,
};

/*
*name:hw_lcd_probe
*function:panel driver probe
*@pdev:platform device
*/
static int __init hw_lcd_probe(struct platform_device* pdev)
{
	struct hisi_panel_info* pinfo = NULL;
	struct device_node* np = NULL;
	int ret = 0;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;

	HISI_FB_INFO("enter succ!!!!\n");

	np = of_find_compatible_node(NULL, NULL, lcd_info.lcd_compatible);
	if (!np) {
		HISI_FB_ERR("NOT FOUND device node %s!\n", lcd_info.lcd_compatible);
		ret = -1;
		return ret;
	}

	ret = of_property_read_u32(np, "hisilicon,dss-panel-type", &lcd_display_type);
	if (ret) {
		HISI_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}

	ret = of_property_read_u32(np, "hisilicon,dss-panel-bl-type", &bl_type);
	if (ret) {
		HISI_FB_ERR("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	HISI_FB_INFO("lcd_display_type=%d, bl_type=%d\n", bl_type, lcd_display_type);
	if (hisi_fb_device_probe_defer(lcd_display_type, bl_type)) {
		goto err_probe_defer;
	}

	/*init sem*/
	sema_init(&lcd_info.bl_sem, 1);

	/*id0 gpio*/
	lcd_info.lcd_id0 = of_get_named_gpio(np, "gpios", 0);
	/*reset gpio*/
	gpio_lcd_reset = of_get_named_gpio(np, "gpios", 1);
	/*vsp gpio*/
	gpio_lcd_vsp = of_get_named_gpio(np, "gpios", 3);
	/*vsn gpio*/
	gpio_lcd_vsn = of_get_named_gpio(np, "gpios", 4);
	/*id1 gpio*/
	lcd_info.lcd_id1 = of_get_named_gpio(np, "gpios", 5);

	pdev->id = 1;
	pinfo = hw_lcd_data.panel_info;
	(void)memset_s(pinfo, sizeof(struct hisi_panel_info), 0, sizeof(struct hisi_panel_info));
	/*Parse panel info*/
	hw_lcd_info_init(np, pinfo);

	/*effect init*/
	hw_lcd_effect_get_data(hw_lcd_get_panel_id(lcd_info.lcd_compatible), pinfo);

	if (pinfo->pxl_clk_rate_div > 1) {
		pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
	}

	/*Parse data from dtsi*/
	ret = hw_lcd_parse_dts(np);
	if (ret) {
		HISI_FB_ERR("parse dtsi failed!\n");
		goto err_probe_defer;
	}

	if(runmode_is_factory()) {
		pinfo->esd_enable = 0;
	}

	/*init bias/vsp/vsn*/
	hw_lcd_vcc_init(hw_lcd_scharger_vcc_set_cmds, ARRAY_SIZE(hw_lcd_scharger_vcc_set_cmds));

	/*init lcdio vcc*/
	hw_lcd_vcc_init(hw_lcdio_vcc_init_cmds, ARRAY_SIZE(hw_lcdio_vcc_init_cmds));

	/*init power sequence*/
	hw_lcd_vcc_seq_init(hw_lcdanalog_vcc_enable_cmds, ARRAY_SIZE(hw_lcdanalog_vcc_enable_cmds), LCD_VCC_POWER_ON_STEP);
	hw_lcd_vcc_seq_init(hw_lcdio_vcc_enable_cmds, ARRAY_SIZE(hw_lcdio_vcc_enable_cmds), LCD_VCC_POWER_ON_STEP);
	hw_lcd_vcc_seq_init(hw_lcd_scharger_vcc_enable_cmds, ARRAY_SIZE(hw_lcd_scharger_vcc_enable_cmds), LCD_VCC_POWER_ON_STEP);
	hw_lcd_vcc_seq_init(hw_lcd_scharger_vcc_disable_cmds, ARRAY_SIZE(hw_lcd_scharger_vcc_disable_cmds), LCD_VCC_POWER_OFF_STEP);
	hw_lcd_vcc_seq_init(hw_lcdio_vcc_disable_cmds, ARRAY_SIZE(hw_lcdio_vcc_disable_cmds), LCD_VCC_POWER_OFF_STEP);
	hw_lcd_vcc_seq_init(hw_lcdanalog_vcc_disable_cmds, ARRAY_SIZE(hw_lcdanalog_vcc_disable_cmds), LCD_VCC_POWER_OFF_STEP);
	hw_lcd_gpio_seq_init(lcd_bias_enable_cmds, LCD_GPIO_VSP_VSN_ON_STEP);
	hw_lcd_gpio_seq_init(lcd_bias_enable_cmds, LCD_GPIO_VSP_VSN_OFF_STEP);

	ret = vcc_cmds_tx(pdev, hw_lcdio_vcc_init_cmds,
	                  ARRAY_SIZE(hw_lcdio_vcc_init_cmds));
	if (ret != 0) {
		HISI_FB_ERR("LCD vcc init failed!\n");
		goto err_return;
	}
	/*init lcdanalog vcc*/
	if (hw_lcd_is_ctl_tp_power()) {
		hw_lcd_vcc_init(hw_lcdanalog_vcc_init_cmds, ARRAY_SIZE(hw_lcdanalog_vcc_init_cmds));
		ret = vcc_cmds_tx(pdev, hw_lcdanalog_vcc_init_cmds,
		                  ARRAY_SIZE(hw_lcdanalog_vcc_init_cmds));
		if (ret != 0) {
			HISI_FB_ERR("LCD vcc init failed!\n");
			goto err_return;
		}
	}

	// lcd pinctrl init
	ret = pinctrl_cmds_tx(pdev, hw_lcd_pinctrl_init_cmds,
	                      ARRAY_SIZE(hw_lcd_pinctrl_init_cmds));
	if (ret != 0) {
		HISI_FB_ERR("Init pinctrl failed, defer\n");
		goto err_return;
	}

	if (POWER_CTRL_BY_REGULATOR == lcd_info.power_ctrl_mode) {
		/* lcd scharger vcc get*/
		ret = vcc_cmds_tx(pdev, hw_lcd_scharger_vcc_get_cmds, \
		            ARRAY_SIZE(hw_lcd_scharger_vcc_get_cmds));
		if (ret != 0) {
			HISI_FB_ERR("Scharger get vcc failed!\n");
			goto err_return;
		}
	}

	// lcd vcc enable
	if (is_fastboot_display_enable()) {
		if (hw_lcd_is_ctl_tp_power()) {
			(void)vcc_cmds_tx(pdev, hw_lcdanalog_vcc_enable_cmds,
			            ARRAY_SIZE(hw_lcdanalog_vcc_enable_cmds));
		}
		(void)vcc_cmds_tx(pdev, hw_lcdio_vcc_enable_cmds,
				ARRAY_SIZE(hw_lcdio_vcc_enable_cmds));
	}
	/*id0 && id1 gpio request*/
	gpio_cmds_tx(hw_lcd_gpio_id_request_cmds, \
				 ARRAY_SIZE(hw_lcd_gpio_id_request_cmds));

	// alloc panel device data
	ret = platform_device_add_data(pdev, &hw_lcd_data,
	                               sizeof(struct hisi_fb_panel_data));
	if (ret) {
		HISI_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	hisi_fb_add_device(pdev);
	HISI_FB_INFO("exit succ!!!!\n");
#if HW_LCD_DEBUG
	lcd_debugfs_init();
#endif
	return 0;

err_device_put:
	platform_device_put(pdev);
err_return:
	return ret;
err_probe_defer:
	return -EPROBE_DEFER;

	return ret;
}

int is_normal_lcd(void)
{
	struct device_node* np = NULL;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_PANEL_TYPE);
	if (!np) {
		HISI_FB_WARNING("NOT FOUND device node %s!\n", DTS_COMP_LCD_PANEL_TYPE);
		return 0;
	}

	if (of_get_property(np, "lcd_panel_type", NULL)){
		HISI_FB_INFO("normal lcd\n");
		return 1;
	} else {
		HISI_FB_INFO("not normal lcd\n");
		return 0;
	}
}

int get_vsp_voltage(void)
{
	int i = 0;

	for(i = 0;i < sizeof(voltage_table) / sizeof(struct vsp_vsn_voltage);i ++) {
		if(voltage_table[i].voltage == lcd_info.lcd_vsp) {
			HISI_FB_INFO("vsp voltage:%ld\n",voltage_table[i].voltage);
			return (voltage_table[i].value);
		}
	}

	if (i >= sizeof(voltage_table) / sizeof(struct vsp_vsn_voltage)) {
		HISI_FB_ERR("not found vsp voltage, use default voltage:TPS65132_VOL_55\n");
	}
	return TPS65132_VOL_55;
}

int get_vsn_voltage(void)
{
	int i = 0;

	for(i = 0;i < sizeof(voltage_table) / sizeof(struct vsp_vsn_voltage);i ++) {
		if(voltage_table[i].voltage == lcd_info.lcd_vsn) {
			HISI_FB_INFO("vsn voltage:%ld\n",voltage_table[i].voltage);
			return (voltage_table[i].value);
		}
	}

	if (i >= sizeof(voltage_table) / sizeof(struct vsp_vsn_voltage)) {
		HISI_FB_ERR("not found vsp voltage, use default voltage:TPS65132_VOL_55\n");
	}
	return TPS65132_VOL_55;
}

/*
*name:hw_lcd_init
*function:panel init
*/
static int __init hw_lcd_init(void)
{
	int ret = 0, len = 0;
	struct device_node* np = NULL;

#if defined(CONFIG_LCDKIT_DRIVER)
    if(get_lcdkit_support())
    {
       HISI_FB_INFO("lcdkit is support!\n");
       return ret;
    }
#endif

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_LCD_PANEL_TYPE);
	if (!np) {
		HISI_FB_WARNING("NOT FOUND device node %s!\n", DTS_COMP_LCD_PANEL_TYPE);
		ret = -1;
		return ret;
	}

	(void)memset_s(&lcd_info, sizeof(struct hw_lcd_information), 0, sizeof(struct hw_lcd_information));
	lcd_info.lcd_compatible = (char*)of_get_property(np, "lcd_panel_type", NULL);
	if (!lcd_info.lcd_compatible) {
		HISI_FB_INFO("Is not normal lcd and return\n");
		return ret;
	} else {
		len = strlen(lcd_info.lcd_compatible);
		(void)memset_s(this_driver.driver.of_match_table->compatible, PANEL_COMP_LENGTH, 0, PANEL_COMP_LENGTH);
		strncpy_s(this_driver.driver.of_match_table->compatible, PANEL_COMP_LENGTH, lcd_info.lcd_compatible,  PANEL_COMP_LENGTH - 1);
		HISI_FB_INFO("lcd_info.lcd_compatible=%s, len = %zd!\n", lcd_info.lcd_compatible, strlen(lcd_info.lcd_compatible));
	}
	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}
	return ret;
}

module_init(hw_lcd_init);
