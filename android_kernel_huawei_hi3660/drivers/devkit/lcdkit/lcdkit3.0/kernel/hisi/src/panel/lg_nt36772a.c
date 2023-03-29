/*
*panel adapter
*product:hima
*panel:lg-nt36772a
*/
static int lg_nt36772a_rgbw_setting(struct hisi_fb_data_type* hisifd, struct lcd_kit_dsi_panel_cmds* rgbw_cmds)
{
	uint32_t rgbw_saturation_control;
	uint32_t frame_gain_limit;
	uint32_t color_distortion_allowance;
	uint32_t pixel_gain_limit;
	uint32_t pwm_duty_gain;
	static uint32_t rgbw_saturation_control_old = 0;
	static uint32_t frame_gain_limit_old = 0;
	static uint32_t color_distortion_allowance_old = 0;
	static uint32_t pixel_gain_limit_old = 0;
	static uint32_t pwm_duty_gain_old = 0;
	int ret = LCD_KIT_OK;

	if ((rgbw_cmds->cmds) && (rgbw_cmds->cmd_cnt>0))
	{
		LCD_KIT_DEBUG("[RGBW] rgbw_mode = %d!\n", hisifd->de_info.ddic_rgbw_mode);
		rgbw_saturation_control = (uint32_t)hisifd->de_info.rgbw_saturation_control;
		frame_gain_limit = (uint32_t)hisifd->de_info.frame_gain_limit;
		color_distortion_allowance = (uint32_t)hisifd->de_info.color_distortion_allowance;
		pwm_duty_gain = (uint32_t)hisifd->de_info.pwm_duty_gain;
		pixel_gain_limit = (uint32_t)hisifd->de_info.pixel_gain_limit;
		if ((rgbw_saturation_control_old != rgbw_saturation_control) || (frame_gain_limit_old != frame_gain_limit) || \
					(color_distortion_allowance_old != color_distortion_allowance) || (pixel_gain_limit_old != pixel_gain_limit) || \
					(pwm_duty_gain_old != pwm_duty_gain))
		{
			//rgbw_saturation_control setting
			rgbw_cmds->cmds[1].payload[1] = (rgbw_saturation_control >> 24) & 0xff;
			rgbw_cmds->cmds[1].payload[2] = (rgbw_saturation_control >> 16) & 0xff;
			rgbw_cmds->cmds[1].payload[3] = (rgbw_saturation_control >> 8) & 0xff;
			rgbw_cmds->cmds[1].payload[4] = rgbw_saturation_control & 0xff;
			LCD_KIT_DEBUG("[RGBW] rgbw_saturation_control=%d, rgbw_saturation_control_old = %d!\n", rgbw_saturation_control, rgbw_saturation_control_old);
			//frame_gain_limit
			rgbw_cmds->cmds[1].payload[5] = (frame_gain_limit >> 8) & 0x3f;
			rgbw_cmds->cmds[1].payload[6] = frame_gain_limit & 0xff;
			LCD_KIT_DEBUG("[RGBW] frame_gain_limit=%d frame_gain_limit_old = %d!\n", frame_gain_limit, frame_gain_limit_old);
			//color_distortion_allowance
			rgbw_cmds->cmds[1].payload[9] = (color_distortion_allowance >> 8) & 0x3f;
			rgbw_cmds->cmds[1].payload[10] = color_distortion_allowance & 0xff;
			LCD_KIT_DEBUG("[RGBW] color_distortion_allowance=%d color_distortion_allowance_old = %d!\n", color_distortion_allowance, color_distortion_allowance_old);
			//pixel_gain_limit
			rgbw_cmds->cmds[1].payload[11] = (pixel_gain_limit >> 8) & 0x3f;
			rgbw_cmds->cmds[1].payload[12] = pixel_gain_limit & 0xff;
			LCD_KIT_DEBUG("[LG-NT36772A-RGBW] pixel_gain_limit=%d pixel_gain_limit_old = %d!\n", pixel_gain_limit, pixel_gain_limit_old);
			//pwm_duty_gain
			rgbw_cmds->cmds[1].payload[15] = (pwm_duty_gain >> 8) & 0x3f;
			rgbw_cmds->cmds[1].payload[16] = pwm_duty_gain & 0xff;
			LCD_KIT_DEBUG("[RGBW] pwm_duty_gain=%d pwm_duty_gain_old = %d!\n", pwm_duty_gain, pwm_duty_gain_old);

			ret = lcd_kit_dsi_cmds_tx(hisifd, rgbw_cmds);
		}
		rgbw_saturation_control_old = rgbw_saturation_control;
		frame_gain_limit_old = frame_gain_limit;
		color_distortion_allowance_old = color_distortion_allowance;
		pixel_gain_limit_old = pixel_gain_limit;
		pwm_duty_gain_old = pwm_duty_gain;
	}

	return ret;
}

static int lg_nt36772a_rgbw(struct hisi_fb_data_type* hisifd, int mode)
{
	int rgbw_mode = 0;
	int ret = LCD_KIT_OK;

	rgbw_mode = hisifd->de_info.ddic_rgbw_mode;
	switch (rgbw_mode)
	{
		case RGBW_SET1_MODE :
			ret = lg_nt36772a_rgbw_setting(hisifd, &disp_info->rgbw.mode1_cmds);
			break;
		case RGBW_SET2_MODE :
			ret = lg_nt36772a_rgbw_setting(hisifd, &disp_info->rgbw.mode2_cmds);
			break;
		case RGBW_SET3_MODE :
			ret = lg_nt36772a_rgbw_setting(hisifd, &disp_info->rgbw.mode3_cmds);
			break;
		case RGBW_SET4_MODE :
			ret = lg_nt36772a_rgbw_setting(hisifd, &disp_info->rgbw.mode4_cmds);
			break;
		default:
			ret = LCD_KIT_FAIL;
			break;
	}

	return ret;
}


static struct lcd_kit_panel_ops lg_nt36772a_ops = {
	.lcd_kit_rgbw_set_mode = lg_nt36772a_rgbw,
};

int lg_nt36772a_proble(void)
{
	int ret = LCD_KIT_OK;

	ret = lcd_kit_panel_ops_register(&lg_nt36772a_ops);
	if (ret) {
		LCD_KIT_ERR("failed\n");
		return LCD_KIT_FAIL;
	}
	return LCD_KIT_OK;
}
