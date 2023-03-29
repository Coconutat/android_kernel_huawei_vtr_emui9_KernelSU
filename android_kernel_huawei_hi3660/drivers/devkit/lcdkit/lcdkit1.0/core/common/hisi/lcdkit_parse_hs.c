#include "hisi_fb.h"
#include "lcdkit_panel.h"
#include "lcdkit_dbg.h"
#include "lcdkit_disp.h"
#include <linux/hisi/hw_cmdline_parse.h> //for runmode_is_factory
#include <linux/of.h>
/***********************************************************
*function definition
***********************************************************/
extern struct lcd_ldo g_lcd_ldo_info;
static void get_lcd_ldo_info_for_rt(void)
{
	int i = 0;
	int ret = -1;
	const char* ldo_name = NULL;
	struct device_node *of_node = NULL;
	memset(&g_lcd_ldo_info, 0, sizeof(struct lcd_ldo));

	of_node = of_find_node_by_path("/huawei,lcd_panel");/*lint !e838 */
	if(of_node == NULL)
	{
		LCDKIT_ERR("Getting lcd ldo node from platform dts is failed!\n");
	}

	g_lcd_ldo_info.lcd_ldo_num = of_property_count_elems_of_size(of_node, "lcd_ldo_channel", sizeof(u32));
	if(g_lcd_ldo_info.lcd_ldo_num > 0)
	{
		ret = of_property_read_u32_array(of_node, "lcd_ldo_channel",&g_lcd_ldo_info.lcd_ldo_channel,g_lcd_ldo_info.lcd_ldo_num);
		if (ret < 0)
		{
			LCDKIT_ERR("Getting lcd_ldo_channel from platform dts is failed!\n");
		}

		ret = of_property_read_u32_array(of_node, "lcd_ldo_threshold",&g_lcd_ldo_info.lcd_ldo_threshold,g_lcd_ldo_info.lcd_ldo_num);
		if (ret < 0)
		{
			LCDKIT_ERR("Getting lcd_ldo_threshold from platform dts is failed!\n");
		}

		for (i = 0; i < g_lcd_ldo_info.lcd_ldo_num; i++)
		{
			ret = of_property_read_string_index(of_node, "lcd_ldo_name", i, &ldo_name);
			if (ret < 0)
			{
				LCDKIT_ERR("Getting lcd_ldo_name from platform dts is failed!\n");
			}
			strncpy(g_lcd_ldo_info.lcd_ldo_name[i], ldo_name, strlen(ldo_name));
		}
	}
}

void lcdkit_parse_platform_dts(struct device_node* np,  void* pdata)
{
    struct hisi_panel_info* pinfo;

    pinfo = (struct hisi_panel_info*) pdata;

    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-orientation", &pinfo->orientation);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-bpp", &pinfo->bpp);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-bgrfmt", &pinfo->bgr_fmt);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-bl-type", &pinfo->bl_set_type);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-blmin", &pinfo->bl_min);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-blmax", &pinfo->bl_max);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-bl-def", &pinfo->bl_default);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-cmd-type", &pinfo->type);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-frc-enable", &pinfo->frc_enable);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-esd-skip-mipi-check", &pinfo->esd_skip_mipi_check);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-ifbctype", &pinfo->ifbc_type);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-bl-ic-ctrl-type", &pinfo->bl_ic_ctrl_mode);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-bl-pwm-out-div-value", &pinfo->blpwm_out_div_value);
    OF_PROPERTY_READ_U8_DEFAULT(np, "hw,lcdkit-panel-fps-updt-only", &pinfo->fps_updt_panel_only, 0);
    OF_PROPERTY_READ_U8_DEFAULT(np, "hw,lcdkit-panel-fps-updt-support", &pinfo->fps_updt_support, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-panel-bl-v200", &pinfo->bl_v200,0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-panel-bl-otm", &pinfo->bl_otm,0);

    OF_PROPERTY_READ_U64_RETURN(np, "hw,lcdkit-panel-pxl-clk", &pinfo->pxl_clk_rate);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-pxl-clk-div", &pinfo->pxl_clk_rate_div);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-dirt-updt-support", &pinfo->dirty_region_updt_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-dsi-upt-support", &pinfo->dsi_bit_clk_upt_support);
    if(pinfo->dsi_bit_clk_upt_support){
    	OF_PROPERTY_READ_U8_RETURN(np, "hw,mipiclk-updt-support-new", &pinfo->mipiclk_updt_support_new);
    }
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-vsyn-ctr-type", &pinfo->vsync_ctrl_type);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-step-support", &pinfo->lcd_uninit_step_support);
	OF_PROPERTY_READ_U8_DEFAULT(np, "hw,lcdkit-panel-xcc-set-in-isr-support", &pinfo->xcc_set_in_isr_support, 0);
	OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-bl-pwm-preci-type", &pinfo->blpwm_precision_type);
	OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-panel-bl-pwm-preci-no-convert", &pinfo->blpwm_preci_no_convert);

    /*effect info*/
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-sbl-support", &pinfo->sbl_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-gamma-support", &pinfo->gamma_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-gmp-support", &pinfo->gmp_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-colormode-support", &pinfo->colormode_support);

    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-color-temp-support",&pinfo->color_temperature_support,0);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-color-temp-rectify-support",&pinfo->color_temp_rectify_support,0);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-comform-mode-support",&pinfo->comform_mode_support,0);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-cinema-mode-support",&pinfo->cinema_mode_support,0);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-xcc-support",&pinfo->xcc_support,0);

    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-hiace-support",&pinfo->hiace_support,0);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-panel-ce-support",&pinfo->panel_effect_support,0);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-arsr1p-sharpness-support",&pinfo->arsr1p_sharpness_support,0);

    if(pinfo->hiace_support){
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-iglobal-hist-black-pos",&pinfo->hiace_param.iGlobalHistBlackPos,0);
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-iglobal-hist-white-pos",&pinfo->hiace_param.iGlobalHistWhitePos,0);
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-iglobal-hist-black-weight",&pinfo->hiace_param.iGlobalHistBlackWeight,0);
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-iglobal-hist-white-weight",&pinfo->hiace_param.iGlobalHistWhiteWeight,0);
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-iglobal-hist-zero-cut-ratio",&pinfo->hiace_param.iGlobalHistZeroCutRatio,0);
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-iglobal-hist-slope-cut-ratio",&pinfo->hiace_param.iGlobalHistSlopeCutRatio,0);
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-imax-lcd-luminance",&pinfo->hiace_param.iMaxLcdLuminance,0);
        OF_PROPERTY_READ_U32_DEFAULT(np,"hw,lcdkit-imin-lcd-luminance",&pinfo->hiace_param.iMinLcdLuminance,0);
        lcdkit_info.panel_infos.hiace_chCfgName = (char*)of_get_property(np, "hw,lcdkit-cfg-name", NULL);
        strncpy(pinfo->hiace_param.chCfgName, lcdkit_info.panel_infos.hiace_chCfgName, sizeof(pinfo->hiace_param.chCfgName)-1);
        LCDKIT_INFO("chname:%s\n",lcdkit_info.panel_infos.hiace_chCfgName);
    }

    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-color-temp-rectify-r", &pinfo->color_temp_rectify_R);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-color-temp-rectify-g", &pinfo->color_temp_rectify_G);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-color-temp-rectify-b", &pinfo->color_temp_rectify_B);

    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-prefix-ce-support", &pinfo->prefix_ce_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-prefix-sharp-one-d-support", &pinfo->prefix_sharpness1D_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-prefix-sharp-two-d-support", &pinfo->prefix_sharpness2D_support);

    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-acm-support", &pinfo->acm_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-acm-ce-support", &pinfo->acm_ce_support);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-smart-color-mode-support", &pinfo->smart_color_mode_support);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-acm-valid-num", &pinfo->acm_valid_num);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_hh0", &pinfo->r0_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_lh0", &pinfo->r0_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_hh1", &pinfo->r1_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_lh1", &pinfo->r1_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_hh2", &pinfo->r2_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_lh2", &pinfo->r2_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_hh3", &pinfo->r3_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_lh3", &pinfo->r3_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_hh4", &pinfo->r4_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_lh4", &pinfo->r4_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_hh5", &pinfo->r5_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_lh5", &pinfo->r5_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_hh6", &pinfo->r6_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_lh6", &pinfo->r6_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_hh0", &pinfo->video_r0_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_lh0", &pinfo->video_r0_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_hh1", &pinfo->video_r1_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_lh1", &pinfo->video_r1_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_hh2", &pinfo->video_r2_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_lh2", &pinfo->video_r2_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_hh3", &pinfo->video_r3_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_lh3", &pinfo->video_r3_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_hh4", &pinfo->video_r4_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_lh4", &pinfo->video_r4_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_hh5", &pinfo->video_r5_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_lh5", &pinfo->video_r5_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_hh6", &pinfo->video_r6_hh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-r_video_lh6", &pinfo->video_r6_lh);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mask-delay-time-before-fp", &pinfo->mask_delay_time_before_fp);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mask-delay-time-after-fp", &pinfo->mask_delay_time_after_fp);

    /*sbl info*/
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-stren-limit", &pinfo->smart_bl.strength_limit);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-cal-a", &pinfo->smart_bl.calibration_a);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-cal-b", &pinfo->smart_bl.calibration_b);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-cal-c", &pinfo->smart_bl.calibration_c);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-cal-d", &pinfo->smart_bl.calibration_d);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-tf-ctl", &pinfo->smart_bl.t_filter_control);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-bl-min", &pinfo->smart_bl.backlight_min);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-bl-max", &pinfo->smart_bl.backlight_max);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-bl-scale", &pinfo->smart_bl.backlight_scale);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-am-light-min", &pinfo->smart_bl.ambient_light_min);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-filter-a", &pinfo->smart_bl.filter_a);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-filter-b", &pinfo->smart_bl.filter_b);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-logo-left", &pinfo->smart_bl.logo_left);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-logo-top", &pinfo->smart_bl.logo_top);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-variance-intensity-space", &pinfo->smart_bl.variance_intensity_space);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-slope-max", &pinfo->smart_bl.slope_max);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-sbl-slope-min", &pinfo->smart_bl.slope_min);

    /*ldi info*/
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-h-back-porch", &pinfo->ldi.h_back_porch);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-h-front-porch", &pinfo->ldi.h_front_porch);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-h-pulse-width", &pinfo->ldi.h_pulse_width);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-v-back-porch", &pinfo->ldi.v_back_porch);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-v-front-porch", &pinfo->ldi.v_front_porch);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-v-pulse-width", &pinfo->ldi.v_pulse_width);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-ldi-hsync-plr", &pinfo->ldi.hsync_plr);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-ldi-vsync-plr", &pinfo->ldi.vsync_plr);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-ldi-pixel-clk-plr", &pinfo->ldi.pixelclk_plr);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-ldi-data-en-plr", &pinfo->ldi.data_en_plr);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-non-check-ldi-porch",&pinfo->non_check_ldi_porch,0);
    OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-dpi01-set-change",&pinfo->dpi01_exchange_flag,0);

    /*mipi info*/
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-mipi-lane-nums", &pinfo->mipi.lane_nums);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-mipi-color-mode", &pinfo->mipi.color_mode);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-mipi-vc", &pinfo->mipi.vc);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-burst-mode", &pinfo->mipi.burst_mode);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-dsi-bit-clk", &pinfo->mipi.dsi_bit_clk);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-max-esc-clk", &pinfo->mipi.max_tx_esc_clk);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-dsi-bit-clk-val-a", &pinfo->mipi.dsi_bit_clk_val1);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-dsi-bit-clk-val-b", &pinfo->mipi.dsi_bit_clk_val2);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-dsi-bit-clk-val-c", &pinfo->mipi.dsi_bit_clk_val3);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-dsi-bit-clk-val-d", &pinfo->mipi.dsi_bit_clk_val4);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-dsi-bit-clk-val-e", &pinfo->mipi.dsi_bit_clk_val5);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-dsi-bit-clk-upt", &pinfo->mipi.dsi_bit_clk_upt);
    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-mipi-non-continue-enable", &pinfo->mipi.non_continue_en);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-clk-post-adjust", &pinfo->mipi.clk_post_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-clk-pre-adjust", &pinfo->mipi.clk_pre_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-clk-t-hs-prepare-adjust", &pinfo->mipi.clk_t_hs_prepare_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-clk-t-lpx-adjust", &pinfo->mipi.clk_t_lpx_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-clk-t-hs-trail-adjust", &pinfo->mipi.clk_t_hs_trial_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-clk-t-hs-exit-adjust", &pinfo->mipi.clk_t_hs_exit_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-clk-t-hs-zero-adjust", &pinfo->mipi.clk_t_hs_zero_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-data-t-hs-trail-adjust", &pinfo->mipi.data_t_hs_trial_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-data-t-hs-prepare-adjust", &pinfo->mipi.data_t_hs_prepare_adjust);
    OF_PROPERTY_READ_U32_RETURN(np, "hw,lcdkit-mipi-data-t-hs-zero-adjust", &pinfo->mipi.data_t_hs_zero_adjust);
    OF_PROPERTY_READ_S8_RETURN(np, "hw,lcdkit-mipi-data-t-lpx-adjust", &pinfo->mipi.data_t_lpx_adjust);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-platform-esd-support", &g_lcdkit_pri_info.platform_esd_support, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,hw,lcdkit-platform-esd-reg", &g_lcdkit_pri_info.platform_esd_reg, 0xc0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,hw,lcdkit-platform-esd-value", &g_lcdkit_pri_info.platform_esd_value, 0x80);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-rg-vcm-adjust", &pinfo->mipi.rg_vrefsel_vcm_adjust, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-rg-lptx-adjust", &pinfo->mipi.rg_vrefsel_lptx_adjust, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-rg-sri-adjust", &pinfo->mipi.rg_lptx_sri_adjust, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-rg-vcm-clk-adjust", &pinfo->mipi.rg_vrefsel_vcm_clk_adjust, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-rg-vcm-data-adjust", &pinfo->mipi.rg_vrefsel_vcm_data_adjust, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-phy-mode", &pinfo->mipi.phy_mode, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-lp11_flag", &pinfo->mipi.lp11_flag, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-hs-wr-to-time", &pinfo->mipi.hs_wr_to_time, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-phy_update", &pinfo->mipi.phy_m_n_count_update, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-power-off-simut-support", &g_lcdkit_pri_info.power_off_simult_support, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-mipi-eotp-disable-flag", &pinfo->mipi.eotp_disable_flag, 0);
    OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-blpwm-input-disable", &pinfo->blpwm_input_disable, 0);

    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-left-align", &pinfo->dirty_region_info.left_align);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-right-align", &pinfo->dirty_region_info.right_align);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-top-align", &pinfo->dirty_region_info.top_align);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-bott-align", &pinfo->dirty_region_info.bottom_align);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-width-align", &pinfo->dirty_region_info.w_align);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-height-align", &pinfo->dirty_region_info.h_align);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-width-min", &pinfo->dirty_region_info.w_min);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-height-min", &pinfo->dirty_region_info.h_min);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-top-start", &pinfo->dirty_region_info.top_start);
    OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, "hw,lcdkit-dirt-bott-start", &pinfo->dirty_region_info.bottom_start);

    OF_PROPERTY_READ_U8_RETURN(np, "hw,lcdkit-panel-bl-type", &lcdkit_info.panel_infos.bl_type);
    if (lcdkit_info.panel_infos.snd_cmd_before_frame_support){
        pinfo->snd_cmd_before_frame_support = 1;
    }else{
        pinfo->snd_cmd_before_frame_support = 0;
    }
    LCDKIT_INFO("send cmd before frame:%u\n", pinfo->snd_cmd_before_frame_support);
    if (lcdkit_info.panel_infos.rgbw_support){
        pinfo->rgbw_support = 1;
    }else{
        pinfo->rgbw_support = 0;
    }
    LCDKIT_INFO("rgbw_support:%d\n", pinfo->rgbw_support);

    if (lcdkit_info.panel_infos.hbm_support){
        pinfo->hbm_support = 1;
    }else{
        pinfo->hbm_support = 0;
    }
    LCDKIT_INFO("hbm_support:%d\n", pinfo->hbm_support);
    LCDKIT_INFO("lcdkit-panel-bl-ic-ctrl-type = %d\n", pinfo->bl_ic_ctrl_mode);
    if(pinfo->bl_ic_ctrl_mode == COMMON_IC_MODE)
    {
        lcdkit_info.panel_infos.backlight_ic_common = 1;
    }
    else
    {
        lcdkit_info.panel_infos.backlight_ic_common = 0;
    }
    if(runmode_is_factory()){
        get_lcd_ldo_info_for_rt();
    }
    if (lcdkit_info.panel_infos.esd_support) {
        OF_PROPERTY_READ_U32_DEFAULT(np, "hw,lcdkit-hisi-esd-support", &g_lcdkit_pri_info.esd_support, 0);
        if (g_lcdkit_pri_info.esd_support) {
            OF_PROPERTY_READ_U8_DEFAULT(np,"hw,lcdkit-panel-esd-expect-value-type",&pinfo->esd_expect_value_type,0);
            lcdkit_parse_dcs_cmds(np, "hw,lcdkit-hisi-esd-reg-command", "hw,lcdkit-hisi-esd-reg-command-state", &g_lcdkit_pri_info.esd_cmds);
            lcdkit_parse_array_data(np, "hw,lcdkit-hisi-esd-value", &g_lcdkit_pri_info.esd_value);
        }
    }
}

