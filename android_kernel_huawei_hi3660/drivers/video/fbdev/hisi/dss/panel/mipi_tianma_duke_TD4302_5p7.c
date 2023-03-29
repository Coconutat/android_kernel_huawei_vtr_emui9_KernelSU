/* Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
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

#include "hisi_fb.h"
#include <huawei_platform/touthscreen/huawei_touchscreen.h>
#include <huawei_platform/log/log_jank.h>
#include "include/lcd_common.h"
#include "include/mipi_tianma_duke_TD4302_5p7.h"
#include "../voltage/rt4801h.h"
#define ESD_OCCUR_COUNT (3)

static int mipi_lcd_panel_set_fastboot(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if(CHECKED_ERROR == mipi_lcd_btb_check()) {
		HISI_FB_INFO("btb checked failed!");
	}

	pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
		ARRAY_SIZE(lcd_pinctrl_normal_cmds));

	gpio_cmds_tx(lcd_gpio_request_cmds, \
		ARRAY_SIZE(lcd_gpio_request_cmds));

	hisi_lcd_backlight_on(pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_lcd_panel_on(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	int error = 0;
	int ret = 0;
	/*info: reg, value, mask, describe*/
	static struct lcd_reg_read_t lcd_status_reg[] = {
		{0x0A, 0x1C, 0xFF, "lcd power state"},
		{0x0E, 0x80, 0xC1, "lcd signal mode"},
		{0x05, 0x00, 0xFF, "mipi dsi error number"},
		{0xDA, 0x00, 0x00, "RDID1"},
		{0x0A, 0x1C, 0xFF, "lcd power state", true},
		{0x0B, 0x00, 0xFF, "lcd address state", true},
		{0x0C, 0x77, 0xFF, "lcd pixedl format", true},
		{0x0D, 0x00, 0xFF, "lcd regsiter 0x0D", true},
		{0x0E, 0x80, 0xC1, "lcd signal mode", true},
		{0x0F, 0x40, 0xFF, "lcd diagnostic result", true},
		{0x05, 0x00, 0xFF, "mipi dsi error number",true},
	};

	if (pdev == NULL) {
		return ERROR;
	}
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return ERROR;
	}

	HISI_FB_INFO("[%s]fb%d, +!\n", DRIVER_NAME,hisifd->index);

	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		if(CHECKED_ERROR == mipi_lcd_btb_check()) {
			HISI_FB_INFO("btb checked failed!");
		}

		g_debug_enable = BACKLIGHT_PRINT_TIMES;
		LOG_JANK_D(JLID_KERNEL_LCD_POWER_ON, "%s", "JL_KERNEL_LCD_POWER_ON");
		if (!gesture_func && !g_debug_enable_lcd_sleep_in) {
			HISI_FB_INFO("Init power on(regulator enabling).\n");

			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
				ARRAY_SIZE(lcd_vcc_enable_cmds));

			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));

			gpio_cmds_tx(lcd_gpio_request_cmds, \
				ARRAY_SIZE(lcd_gpio_request_cmds));

			gpio_cmds_tx(lcd_gpio_normal_cmds_sub1, \
				ARRAY_SIZE(lcd_gpio_normal_cmds_sub1));

			if(check_rt4801h_device()) {
				rt4801h_set_voltage();
			}
		} else {
			HISI_FB_INFO("power on (gesture_func:%d)\n", gesture_func);
			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));

			gpio_cmds_tx(lcd_gpio_sleep_request_cmds, \
					ARRAY_SIZE(lcd_gpio_sleep_request_cmds));

			gpio_cmds_tx(lcd_gpio_sleep_normal_cmds, \
					ARRAY_SIZE(lcd_gpio_sleep_normal_cmds));
			msleep(GPIO_DELAY);
		}

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		/* after LP11 delay 12ms */
		mdelay(LP_DELAY);

		/* IC spec requir LCD reset signal after MIPI come into LP11 */
		gpio_cmds_tx(lcd_gpio_normal_cmds_sub2, \
			ARRAY_SIZE(lcd_gpio_normal_cmds_sub2));
		/* control touch timing */
		if (TP_RS_CALL != g_debug_enable_lcd_sleep_in) {
			HISI_FB_INFO("TP resume and after resume\n");
			error = ts_power_control_notify(TS_RESUME_DEVICE,
						SHORT_SYNC_TIMEOUT);
			if (error) {
				HISI_FB_ERR("ts resume device err\n");
			}
			error = ts_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);
			if (error) {
				HISI_FB_ERR("ts resume device err\n");
			}
		}

		if (pinfo->panel_effect_support) {
			mipi_dsi_cmds_tx(lcd_display_effect_on_cmd, \
				ARRAY_SIZE(lcd_display_effect_on_cmd), mipi_dsi0_base);
		}

		mipi_dsi_cmds_tx(lcd_display_on_cmd, \
			ARRAY_SIZE(lcd_display_on_cmd), mipi_dsi0_base);

		if ((pinfo->bl_set_type & BL_SET_BY_BLPWM)
			|| (pinfo->bl_set_type & BL_SET_BY_SH_BLPWM)) {
			mipi_dsi_cmds_tx(pwm_out_on_cmds, \
				ARRAY_SIZE(pwm_out_on_cmds), mipi_dsi0_base);
		}

		g_cabc_mode = G_CABC_MODE1;
		g_ce_mode = G_CE_ON;

		panel_check_status_and_report_by_dsm(lcd_status_reg, \
			ARRAY_SIZE(lcd_status_reg), mipi_dsi0_base);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		;
	} else {
		HISI_FB_ERR("failed to init lcd!\n");
	}

	hisi_lcd_backlight_on(pdev);

	HISI_FB_INFO("[%s]fb%d, -!\n", DRIVER_NAME,hisifd->index);

	return ret;
}

static int mipi_lcd_panel_off(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int error = 0;
	int ret = 0;

	if (pdev == NULL) {
		return ERROR;
	}
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return ERROR;
	}

	HISI_FB_INFO("[%s]fb%d, +!\n", DRIVER_NAME,hisifd->index);

	pinfo = &(hisifd->panel_info);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "JL_KERNEL_LCD_POWER_OFF");

		hisi_lcd_backlight_off(pdev);

		mipi_dsi_cmds_tx(lcd_display_off_cmd, \
			ARRAY_SIZE(lcd_display_off_cmd), hisifd->mipi_dsi0_base);

		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (!hisifd->fb_shutdown) {
			if (!gesture_func && !g_debug_enable_lcd_sleep_in) {
				HISI_FB_INFO("display off(regulator disabling).\n");

				gpio_cmds_tx(lcd_gpio_lowpower_cmds, \
					ARRAY_SIZE(lcd_gpio_lowpower_cmds));

				gpio_cmds_tx(lcd_gpio_free_cmds, \
					ARRAY_SIZE(lcd_gpio_free_cmds));

				pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
					ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

				vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
					ARRAY_SIZE(lcd_vcc_disable_cmds));
			} else {
				HISI_FB_INFO("display_off (gesture_func:%d)\n", gesture_func);

				gpio_cmds_tx(lcd_gpio_sleep_free_cmds, \
					ARRAY_SIZE(lcd_gpio_sleep_free_cmds));

				pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
					ARRAY_SIZE(lcd_pinctrl_normal_cmds));
		}

			/*if g_debug_enable_lcd_sleep_in == 1,
			**it means don't turn off TP/LCD power
			**but only let lcd get into sleep.
			*/
			if (TP_RS_CALL != g_debug_enable_lcd_sleep_in) {
				HISI_FB_INFO("TP before suspend and suspend\n");
				error = ts_power_control_notify(TS_BEFORE_SUSPEND,
							SHORT_SYNC_TIMEOUT);
				if (error) {
					HISI_FB_ERR("ts before suspend err\n");
				}
				error = ts_power_control_notify(TS_SUSPEND_DEVICE,
					SHORT_SYNC_TIMEOUT);
				if (error) {
					HISI_FB_ERR("ts suspend device err\n");
				}
			}
			/* delay 200ms to make sure 1.8V completely to 0 */
			mdelay(VCC_DELAY);
		}else {
			HISI_FB_INFO("display shutting down(regulator disabling).\n");

			gpio_cmds_tx(lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(lcd_gpio_lowpower_cmds));

			gpio_cmds_tx(lcd_gpio_free_cmds, \
				ARRAY_SIZE(lcd_gpio_free_cmds));

			pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

			vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
				ARRAY_SIZE(lcd_vcc_disable_cmds));

			/* delay 200ms to make sure 1.8V completely to 0 */
			mdelay(VCC_DELAY);

			ts_thread_stop_notify();
		}

	} else {
		HISI_FB_ERR("failed to uninit lcd!\n");
	}

	HISI_FB_INFO("[%s]fb%d, -!\n", DRIVER_NAME,hisifd->index);

	return ret;
}

static ssize_t mipi_lcd_panel_gram_check_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	if(!buf) {
		return ERROR;
	}

	ret = snprintf(buf, PAGE_SIZE, "0");

	return ret;
}

static ssize_t mipi_lcd_panel_gram_check_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	return count;
}

static int mipi_lcd_panel_remove(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);

	if (!hisifd) {
		return ret;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
		ARRAY_SIZE(lcd_vcc_finit_cmds));

	pinctrl_cmds_tx(pdev, lcd_pinctrl_finit_cmds,
		ARRAY_SIZE(lcd_pinctrl_finit_cmds));

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_lcd_panel_set_backlight(struct platform_device *pdev,
	uint32_t bl_level)
{
	int ret = 0;
	char __iomem *mipi_dsi0_base = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	/*info: bl_level_adjust reg, value*/
	char bl_level_adjust[BL_FLAG] = {
		0x51,
		0x00,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, BL_VC, BL_WAIT, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	HISI_FB_DEBUG("fb%d, bl_level=%d.\n", hisifd->index, bl_level);

	if (unlikely(g_debug_enable)) {
		HISI_FB_INFO("Set backlight to %d. (remain times of backlight print: %d)\n",
			hisifd->bl_level, g_debug_enable);

		if (g_debug_enable == BACKLIGHT_PRINT_TIMES) {
			LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_ON,
			"JL_KERNEL_LCD_BACKLIGHT_ON,%u", hisifd->bl_level);
		}

		g_debug_enable = (g_debug_enable > G_DEBUG_ENABLE0) ? (g_debug_enable - G_DEBUG_ENABLE1) : G_DEBUG_ENABLE0;
	}

	if (!bl_level) {
		HISI_FB_INFO("Set backlight to 0 !!!\n");
	}

	if (hisifd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		ret = hisi_pwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {
		ret = hisi_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM) {
		ret = hisi_sh_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		mipi_dsi0_base = hisifd->mipi_dsi0_base;
		bl_level_adjust[BL_FLAG_NUM1] = bl_level * BL_LEVEL_255 / hisifd->panel_info.bl_max;
		mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
			ARRAY_SIZE(lcd_bl_level_adjust), mipi_dsi0_base);
	} else {
		HISI_FB_ERR("fb%d, not support this bl_set_type(%d)!\n",
			hisifd->index, hisifd->panel_info.bl_set_type);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_lcd_panel_model_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "tianma_duke_TD4302_WQHD 5.7' CMD TFT\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_lcd_panel_cabc_mode_show(struct platform_device *pdev,
	char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", g_cabc_mode);
}

static ssize_t mipi_lcd_panel_cabc_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag = -1;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	ret = strict_strtoul(buf, STR_BASE, &val);
	if (ret)
		return ret;

	flag = (int)val;

	if (flag < CABC_OFF || flag > CABC_MOVING_MODE)
		return -EINVAL;

	if (flag == CABC_OFF){
		g_cabc_mode = CABC_OFF;
		mipi_dsi_cmds_tx(lcd_cabc_off_cmds, \
			ARRAY_SIZE(lcd_cabc_off_cmds),\
			mipi_dsi0_base);
	}else if (flag == CABC_UI_MODE) {
		g_cabc_mode = CABC_UI_MODE;
		mipi_dsi_cmds_tx(lcd_cabc_ui_on_cmds, \
			ARRAY_SIZE(lcd_cabc_ui_on_cmds),\
			mipi_dsi0_base);
	} else if (flag == CABC_STILL_MODE ) {
		g_cabc_mode = CABC_STILL_MODE;
		mipi_dsi_cmds_tx(lcd_cabc_still_on_cmds, \
			ARRAY_SIZE(lcd_cabc_still_on_cmds),\
			mipi_dsi0_base);
	} else if (flag == CABC_MOVING_MODE ){
		g_cabc_mode = CABC_MOVING_MODE;
		mipi_dsi_cmds_tx(lcd_cabc_moving_on_cmds, \
			ARRAY_SIZE(lcd_cabc_moving_on_cmds),\
			mipi_dsi0_base);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return snprintf((char *)buf, count, "%d\n", g_cabc_mode);
}

static ssize_t mipi_tianma_panel_lcd_ce_mode_show(struct platform_device *pdev,
	char *buf)
{
	if (NULL == pdev || NULL == buf) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	return snprintf(buf, PAGE_SIZE, "%d\n", g_ce_mode);
}

static ssize_t mipi_tianma_panel_lcd_ce_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev || NULL == buf) {
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

	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		return ret;
	}

	down(&lcd_cmd_sem);
	if (lcd_check_mipi_fifo_empty(hisifd->mipi_dsi0_base)) {
		HISI_FB_ERR("ce mode cmd send fail!\n");
		up(&lcd_cmd_sem);
		return -EPERM;
	}

	flag = (int)val;
	if (flag == CE_OFF){
		g_ce_mode = CE_OFF;
		mipi_dsi_cmds_tx(tianma_ce_nomal_cmds, \
			ARRAY_SIZE(tianma_ce_nomal_cmds), hisifd->mipi_dsi0_base);
	} else  if (flag == CE_SRGB_MODE) {
		g_ce_mode = CE_SRGB_MODE;
		mipi_dsi_cmds_tx(tianma_ce_nomal_cmds, \
			ARRAY_SIZE(tianma_ce_nomal_cmds), hisifd->mipi_dsi0_base);
	} else if (flag == CE_USER_MODE){
		g_ce_mode = CE_USER_MODE;
		mipi_dsi_cmds_tx(tianma_ce_cinema_cmds, \
			ARRAY_SIZE(tianma_ce_cinema_cmds), hisifd->mipi_dsi0_base);
	} else if (flag == CE_VIVID_MODE){
		g_ce_mode = CE_VIVID_MODE;
		mipi_dsi_cmds_tx(tianma_ce_nomal_cmds, \
			ARRAY_SIZE(tianma_ce_nomal_cmds), hisifd->mipi_dsi0_base);
	}

	if (lcd_check_mipi_fifo_empty(hisifd->mipi_dsi0_base)) {
		HISI_FB_ERR("ce mode cmd send not finish!\n");
	}
	up(&lcd_cmd_sem);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return (ssize_t)count;
}

static ssize_t mipi_lcd_panel_check_reg_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	/*info: reg, expected value, mask*/
	uint32_t read_value[VALUE_NUM5] = { 0 };
	uint32_t expected_value[VALUE_NUM5] = { 0x38, 0x00, 0x07, 0x00, 0x00 };
	uint32_t read_mask[VALUE_NUM5] = { 0xFF, 0xFF, 0x07, 0xFF, 0xFF };
	char *reg_name[VALUE_NUM5] = {
		"power mode",
		"MADCTR",
		"pixel format",
		"image mode",
		"mipi error"
	};
	char lcd_reg_0a[] = { 0x0a };
	char lcd_reg_0b[] = { 0x0b };
	char lcd_reg_0c[] = { 0x0c };
	char lcd_reg_0d[] = { 0x0d };
	char lcd_reg_ab[] = { 0xab };

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_0b), lcd_reg_0b},
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_0c), lcd_reg_0c},
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_0d), lcd_reg_0d},
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_ab), lcd_reg_ab},
	};

	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = LOG_ON,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (!mipi_dsi_read_compare(&data, mipi_dsi0_base)) {
		ret = snprintf(buf, PAGE_SIZE, "P-0x0a:0x%x, 0x0b:0x%x, 0x0c:0x%x, 0x0d:0x%x, 0xab:0x%x\n",
			data.read_value[READ_VALUE_NUM0], data.read_value[READ_VALUE_NUM1], data.read_value[READ_VALUE_NUM2],
			data.read_value[READ_VALUE_NUM3], data.read_value[READ_VALUE_NUM4]);
	} else {
		ret = snprintf(buf, PAGE_SIZE, "F-0x0a:0x%x, 0x0b:0x%x, 0x0c:0x%x, 0x0d:0x%x, 0xab:0x%x\n",
			data.read_value[READ_VALUE_NUM0], data.read_value[READ_VALUE_NUM1], data.read_value[READ_VALUE_NUM2],
			data.read_value[READ_VALUE_NUM3], data.read_value[READ_VALUE_NUM4]);
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_lcd_panel_mipi_detect_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	/*info: reg, expected value, mask*/
	uint32_t read_value[VALUE_NUM2] = { 0 };
	uint32_t expected_value[VALUE_NUM2] = { 0x80, 0x00 };
	uint32_t read_mask[VALUE_NUM2] = { 0xFF, 0xFF };
	char *reg_name[VALUE_NUM2] = { "signal mode", "dsi error number" };
	char lcd_reg_0e[] = { 0x0e };
	char lcd_reg_05[] = { 0x05 };

	struct dsi_cmd_desc set_read_size[] = {
		{DTYPE_MAX_PKTSIZE, REG_VC, REG_WAIT, WAIT_TYPE_US,
			REG_DLEN2, REG_PAYLOAD},
	};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e},
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_05), lcd_reg_05},
	};

	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = LOG_ON,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	mipi_dsi_max_return_packet_size(set_read_size, mipi_dsi0_base);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (!mipi_dsi_read_compare(&data, mipi_dsi0_base)) {
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	} else {
		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*info: dirty region x:y - reg, value*/
static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00, 0x04, 0x37,
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00, 0x07, 0x7F,
};

static struct dsi_cmd_desc set_display_address[] = {
	{DTYPE_DCS_LWRITE, REG_VC, REG_WAIT5, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, REG_VC, REG_WAIT5, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y},
};

static int mipi_lcd_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int ret = 0;

	BUG_ON(pdev == NULL || dirty == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);
	if (((dirty->x % pinfo->dirty_region_info.left_align) != 0)
		|| (dirty->w < pinfo->dirty_region_info.w_min)
		|| ((dirty->w % pinfo->dirty_region_info.w_align) != 0)
		|| ((dirty->y % pinfo->dirty_region_info.top_align) != 0)
		|| ((dirty->h % pinfo->dirty_region_info.h_align) != 0)
		|| (dirty->h < pinfo->dirty_region_info.h_min)
		|| (dirty->x >= pinfo->xres) || (dirty->w > pinfo->xres)
		|| ((dirty->x + dirty->w) > pinfo->xres)
		|| (dirty->y >= pinfo->yres) || (dirty->h > pinfo->yres)
		|| ((dirty->y + dirty->h) > pinfo->yres)) {
		HISI_FB_ERR("dirty_region(%d,%d, %d,%d) not support!\n",
				dirty->x, dirty->y, dirty->w, dirty->h);

		BUG_ON(BUG_ON1);
	}

	lcd_disp_x[DISP_NUM1] = (dirty->x >> RIGHT_SHIFT8) & KEEP_BITS;
	lcd_disp_x[DISP_NUM2] = dirty->x & KEEP_BITS;
	lcd_disp_x[DISP_NUM3] = ((dirty->x + dirty->w - SPARE_ONE) >> RIGHT_SHIFT8) & KEEP_BITS;
	lcd_disp_x[DISP_NUM4] = (dirty->x + dirty->w - SPARE_ONE) & KEEP_BITS;
	lcd_disp_y[DISP_NUM1] = (dirty->y >> RIGHT_SHIFT8) & KEEP_BITS;
	lcd_disp_y[DISP_NUM2] = dirty->y & KEEP_BITS;
	lcd_disp_y[DISP_NUM3] = ((dirty->y + dirty->h - SPARE_ONE) >> RIGHT_SHIFT8) & KEEP_BITS;
	lcd_disp_y[DISP_NUM4] = (dirty->y + dirty->h - SPARE_ONE) & KEEP_BITS;

	down(&lcd_cmd_sem);
	if (lcd_check_mipi_fifo_empty(hisifd->mipi_dsi0_base)) {
		HISI_FB_ERR("dirty region cmd send fail!\n");
		up(&lcd_cmd_sem);
		return -EPERM;
	}

	mipi_dsi_cmds_tx(set_display_address, \
		ARRAY_SIZE(set_display_address), hisifd->mipi_dsi0_base);

	if (lcd_check_mipi_fifo_empty(hisifd->mipi_dsi0_base)) {
		HISI_FB_ERR("dirty region cmd send not finish!\n");
	}
	up(&lcd_cmd_sem);

	return ret;
}

static ssize_t mipi_lcd_panel_sleep_ctrl_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (pdev == NULL) {
		return ERROR;
	}
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return ERROR;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = snprintf(buf, PAGE_SIZE,
			"enable_lcd_sleep_in=%d,pinfo->lcd_adjust_support=%d\n",
			g_debug_enable_lcd_sleep_in, hisifd->panel_info.lcd_adjust_support);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_lcd_panel_sleep_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	ssize_t ret = 0;
	unsigned long val = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	ret = strict_strtoul(buf, STR_BASE, &val);
	if (ret) {
		HISI_FB_ERR("strict_strtoul error, buf=%s", buf);
		return ret;
	}

	if (pdev == NULL) {
		return ERROR;
	}
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return ERROR;
	}
	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (hisifd->panel_info.lcd_adjust_support) {
		g_debug_enable_lcd_sleep_in = val;
	}

	if (g_debug_enable_lcd_sleep_in == G_DEBUG2) {
		HISI_FB_INFO("LCD power off and Touch goto sleep\n");
		g_tp_power_ctrl = G_TP1;
	} else {
		HISI_FB_INFO("g_debug_enable_lcd_sleep_in is %d\n",
			g_debug_enable_lcd_sleep_in);
		g_tp_power_ctrl = G_TP0;
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_lcd_panel_check_esd(struct platform_device* pdev)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	uint32_t read_value[VALUE_NUM2] = { 0 };
	uint32_t expected_value[VALUE_NUM2] = { 0x1C, 0x80 };		/* expect the value of reg*/
	uint32_t read_mask[VALUE_NUM2] = { 0xFF, 0xFF };			/* mask code */
	char *reg_name[VALUE_NUM2] = { "power mode", "0E" };
	char lcd_reg_0a[] = { 0x0a };					/*the reg need to check*/
	char lcd_reg_0e[] = { 0x0e };
	int status_reg_detect = 0;
	static unsigned int esd_occur_times = 0;

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, REG_VC, REG_WAIT, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e},
	};

	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = LOG_OFF,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	BUG_ON(pdev == NULL);
	hisifd = (struct hisi_fb_data_type *)platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = mipi_dsi_read_compare(&data, hisifd->mipi_dsi0_base);
	status_reg_detect = (ret != 0);
	if (ret){
		esd_occur_times++;
		HISI_FB_ERR("ESD ERROR:esd_occur_times = %d\n", esd_occur_times);
		HISI_FB_ERR("esd 0A or 0E detect abnormal:0x0a=%02x,0x0e=%02x\n",read_value[0], read_value[1]);
		if (ESD_OCCUR_COUNT == esd_occur_times) {
				esd_occur_times = 0;
				ret = dsm_client_ocuppy(lcd_dclient);
			if ( !ret ) {
					dsm_client_record(lcd_dclient, "esd 0A or 0E detect abnormal:0x0a=0x%x,0x0e=0x%x\n",read_value[0], read_value[1]);
					dsm_client_notify(lcd_dclient, DSM_LCD_ESD_RECOVERY_NO);
					} else {
							HISI_FB_ERR("dsm_client_ocuppy ERROR:retVal = %d\n", ret);
					}
		}
	} else {
		HISI_FB_DEBUG("esd 0A or 0E detect normal:0x0a=%02x,0x0e=%02x\n",read_value[0], read_value[1]);
		esd_occur_times = 0;
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
	return status_reg_detect;
}

static ssize_t mipi_lcd_panel_test_config_show(struct platform_device *pdev,
	char *buf)
{
	int i = 0;

	if(!buf) {
		return ERROR;
	}

	for(i = 0;i < SENCE_ARRAY_SIZE;i++) {
		if (!strncmp(lcd_cmd_now, sence_array[i], strlen(sence_array[i]))) {
			HISI_FB_INFO("current test cmd:%s,return cmd:%s\n",
						lcd_cmd_now,cmd_array[i]);
			return snprintf(buf, PAGE_SIZE, cmd_array[i]);
		}
	}

	HISI_FB_INFO("cmd invaild,current test cmd:%s\n", lcd_cmd_now);
	return snprintf(buf, PAGE_SIZE, "INVALID");
}

static ssize_t mipi_lcd_panel_test_config_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;
	//char __iomem *mipi_dsi0_base = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);
	//mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (strlen(buf) < LCD_CMD_NAME_MAX) {
		memcpy(lcd_cmd_now, buf, strlen(buf) + ACTUAL_ONE);
		HISI_FB_INFO("current test cmd:%s\n", lcd_cmd_now);
	} else {
		memcpy(lcd_cmd_now, "INVALID", strlen("INVALID") + ACTUAL_ONE);
		HISI_FB_INFO("invalid test cmd:%s\n", lcd_cmd_now);
	}

	return count;
}

static int g_support_mode = 0;

static ssize_t mipi_lcd_panel_support_mode_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (pdev == NULL) {
		return ERROR;
	}

	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return ERROR;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "%d\n", g_support_mode);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_lcd_panel_support_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (pdev == NULL) {
		return ERROR;
	}

	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return ERROR;
	}

	ret = strict_strtoul(buf, STR_BASE, &val);
	if (ret)
		return ret;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	flag = (int)val;

	g_support_mode = flag;

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return snprintf((char *)buf, count, "%d\n", g_support_mode);
}

static ssize_t mipi_lcd_panel_support_checkmode_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
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

	ret = snprintf(buf, PAGE_SIZE, "bl_open_short:1\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_lcd_panel_info_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev NULL pointer\n");
		return ret;
	};
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd NULL pointer\n");
		return ret;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (buf) {
		ret = snprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,lcdtype:%s,\n",
				hisifd->panel_info.bl_max, hisifd->panel_info.bl_min, "LCD");
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static struct hisi_panel_info g_panel_info = {0};
static struct hisi_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_lcd_panel_set_fastboot,
	.on = mipi_lcd_panel_on,
	.off = mipi_lcd_panel_off,
	.remove = mipi_lcd_panel_remove,
	.set_backlight = mipi_lcd_panel_set_backlight,
	.lcd_model_show = mipi_lcd_panel_model_show,
	.lcd_cabc_mode_show = mipi_lcd_panel_cabc_mode_show,
	.lcd_cabc_mode_store = mipi_lcd_panel_cabc_mode_store,
	.lcd_ce_mode_show = mipi_tianma_panel_lcd_ce_mode_show,
	.lcd_ce_mode_store = mipi_tianma_panel_lcd_ce_mode_store,
	.lcd_check_reg = mipi_lcd_panel_check_reg_show,
	.lcd_mipi_detect = mipi_lcd_panel_mipi_detect_show,
	.lcd_gram_check_show = mipi_lcd_panel_gram_check_show,
	.lcd_gram_check_store = mipi_lcd_panel_gram_check_store,
	.set_display_region = mipi_lcd_panel_set_display_region,
	.lcd_sleep_ctrl_show = mipi_lcd_panel_sleep_ctrl_show,
	.lcd_sleep_ctrl_store = mipi_lcd_panel_sleep_ctrl_store,
	.esd_handle = mipi_lcd_panel_check_esd,
	.lcd_test_config_show = mipi_lcd_panel_test_config_show,
	.lcd_test_config_store = mipi_lcd_panel_test_config_store,
	.lcd_support_mode_show = mipi_lcd_panel_support_mode_show,
	.lcd_support_mode_store = mipi_lcd_panel_support_mode_store,
	.lcd_support_checkmode_show = mipi_lcd_panel_support_checkmode_show,
	.panel_info_show = mipi_lcd_panel_info_show,
};

static int mipi_lcd_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_panel_info *pinfo = NULL;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t support_mode = 0;
	int ret_ok = 0;

	/* not use fb_notify to control touch timing. */
	g_lcd_control_tp_power = true;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_TIANMA_DUKE_TD4302_5P7);
	if (!np) {
		HISI_FB_ERR("NOT FOUND device node %s!\n", DTS_COMP_TIANMA_DUKE_TD4302_5P7);
		goto err_return;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, &lcd_display_type);
	if (ret) {
		HISI_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}
	HISI_FB_INFO("lcd_display_type=%u!\n", lcd_display_type);

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &bl_type);
	if (ret) {
		HISI_FB_ERR("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	HISI_FB_INFO("bl_type=0x%x.", bl_type);

	if (hisi_fb_device_probe_defer(lcd_display_type, bl_type)) {
		HISI_FB_INFO("proeb defer\n");
		goto err_probe_defer;
	}

	HISI_FB_DEBUG("+.\n");

	HISI_FB_INFO("%s\n", DTS_COMP_TIANMA_DUKE_TD4302_5P7);

	gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", GPIOS_NUM0);
	gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", GPIOS_NUM1);
	gpio_lcd_reset = of_get_named_gpio(np, "gpios", GPIOS_NUM2);
	gpio_lcd_btb = of_get_named_gpio(np, "gpios", GPIOS_NUM6);

	HISI_FB_INFO("enn=%d,enp=%d,rst=%d,btb=%d",gpio_lcd_n5v5_enable,
		gpio_lcd_p5v5_enable,gpio_lcd_reset,gpio_lcd_btb);
	lcd_btb_init(DTS_COMP_TIANMA_DUKE_TD4302_5P7);
	pdev->id = PDEV_ID;
	sema_init(&lcd_cmd_sem, 1);

	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct hisi_panel_info));
	pinfo->xres = 1440;
	pinfo->yres = 2560;
	pinfo->width = 71;
	pinfo->height = 126;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;
	pinfo->type = lcd_display_type;

	if (pinfo->bl_set_type == BL_SET_BY_BLPWM)
		pinfo->blpwm_input_ena = BLPWM_EN;

	/* 10000stage 60,2048stage 435 for 3~4nit */
	pinfo->bl_min = 60;
	/* 10000stage 7198,2048stage 1938 for 450nit */
	pinfo->bl_max = 8200;
	pinfo->bl_default = 4000;
	pinfo->blpwm_precision_type = BLPWM_PRECISION_2048_TYPE;
	pinfo->bl_ic_ctrl_mode = REG_ONLY_MODE;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 1;
	pinfo->esd_skip_mipi_check = 1;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->lcd_adjust_support = 1;
	pinfo->color_temperature_support = 1;

	/* prefix ce & sharpness */
	pinfo->prefix_ce_support = 0;
	pinfo->prefix_sharpness1D_support = 0;

	/*enable arsr1p sharpness*/
	pinfo->arsr1p_sharpness_support = 1;
	/*enable arsr2p sharpness*/
	pinfo->prefix_sharpness2D_support = 1;

	/* ACM */
	pinfo->acm_support = 0;
	if (pinfo->acm_support == ACM_SUPPORT) {
		pinfo->acm_lut_hue_table = acm_lut_hue_table;
		pinfo->acm_lut_hue_table_len = ARRAY_SIZE(acm_lut_hue_table);
		pinfo->acm_lut_sata_table = acm_lut_sata_table;
		pinfo->acm_lut_sata_table_len = ARRAY_SIZE(acm_lut_sata_table);
		pinfo->acm_lut_satr_table = acm_lut_satr_table;
		pinfo->acm_lut_satr_table_len = ARRAY_SIZE(acm_lut_satr_table);
		/*for cinema mode */
		pinfo->cinema_acm_lut_hue_table = cinema_acm_lut_hue_table;
		pinfo->cinema_acm_lut_hue_table_len
			= ARRAY_SIZE(cinema_acm_lut_hue_table);
		pinfo->cinema_acm_lut_sata_table
			= cinema_acm_lut_sata_table;
		pinfo->cinema_acm_lut_sata_table_len
			= ARRAY_SIZE(cinema_acm_lut_sata_table);
		pinfo->cinema_acm_lut_satr_table
			= cinema_acm_lut_satr_table;
		pinfo->cinema_acm_lut_satr_table_len
			= ARRAY_SIZE(cinema_acm_lut_satr_table);

		pinfo->acm_valid_num = 7;
		pinfo->r0_hh = 0x7f;
		pinfo->r0_lh = 0x0;
		pinfo->r1_hh = 0xff;
		pinfo->r1_lh = 0x80;
		pinfo->r2_hh = 0x17f;
		pinfo->r2_lh = 0x100;
		pinfo->r3_hh = 0x1ff;
		pinfo->r3_lh = 0x180;
		pinfo->r4_hh = 0x27f;
		pinfo->r4_lh = 0x200;
		pinfo->r5_hh = 0x2ff;
		pinfo->r5_lh = 0x280;
		pinfo->r6_hh = 0x37f;
		pinfo->r6_lh = 0x300;
		/* ACM_CE */
/*		pinfo->acm_ce_support = 1;*/
	}

	/* Contrast Algorithm */
	if (pinfo->prefix_ce_support == CE_SUPPORT || pinfo->acm_ce_support == CE_SUPPORT) {
		pinfo->ce_alg_param.iDiffMaxTH = 900;
		pinfo->ce_alg_param.iDiffMinTH = 100;
		pinfo->ce_alg_param.iFlatDiffTH = 500;
		pinfo->ce_alg_param.iAlphaMinTH = 16;
		pinfo->ce_alg_param.iBinDiffMaxTH = 40000;

		pinfo->ce_alg_param.iDarkPixelMinTH = 16;
		pinfo->ce_alg_param.iDarkPixelMaxTH = 24;
		pinfo->ce_alg_param.iDarkAvePixelMinTH = 40;
		pinfo->ce_alg_param.iDarkAvePixelMaxTH = 80;
		pinfo->ce_alg_param.iWhitePixelTH = 236;
		pinfo->ce_alg_param.fweight = 42;
		pinfo->ce_alg_param.fDarkRatio = 51;
		pinfo->ce_alg_param.fWhiteRatio = 51;

		pinfo->ce_alg_param.iDarkPixelTH = 64;
		pinfo->ce_alg_param.fDarkSlopeMinTH = 149;
		pinfo->ce_alg_param.fDarkSlopeMaxTH = 161;
		pinfo->ce_alg_param.fDarkRatioMinTH = 18;
		pinfo->ce_alg_param.fDarkRatioMaxTH = 38;

		pinfo->ce_alg_param.iBrightPixelTH = 192;
		pinfo->ce_alg_param.fBrightSlopeMinTH = 149;
		pinfo->ce_alg_param.fBrightSlopeMaxTH = 174;
		pinfo->ce_alg_param.fBrightRatioMinTH = 20;
		pinfo->ce_alg_param.fBrightRatioMaxTH = 36;

		pinfo->ce_alg_param.iZeroPos0MaxTH = 120;
		pinfo->ce_alg_param.iZeroPos1MaxTH = 128;

		pinfo->ce_alg_param.iDarkFMaxTH = 16;
		pinfo->ce_alg_param.iDarkFMinTH = 12;
		pinfo->ce_alg_param.iPos0MaxTH = 120;
		pinfo->ce_alg_param.iPos0MinTH = 96;

		pinfo->ce_alg_param.fKeepRatio = 61;
	}

	/* Gama LCP */
	pinfo->gamma_support = 1;
	if (pinfo->gamma_support == GAMMA_SUPPORT) {
		pinfo->gamma_lut_table_R = gamma_lut_table_R;
		pinfo->gamma_lut_table_G = gamma_lut_table_G;
		pinfo->gamma_lut_table_B = gamma_lut_table_B;
		pinfo->gamma_lut_table_len = ARRAY_SIZE(gamma_lut_table_R);

		pinfo->cinema_gamma_lut_table_R = cinema_gamma_lut_table_R;
		pinfo->cinema_gamma_lut_table_G = cinema_gamma_lut_table_G;
		pinfo->cinema_gamma_lut_table_B = cinema_gamma_lut_table_B;
		pinfo->cinema_gamma_lut_table_len
			= ARRAY_SIZE(cinema_gamma_lut_table_R);

		pinfo->igm_lut_table_R = igm_lut_table_R;
		pinfo->igm_lut_table_G = igm_lut_table_G;
		pinfo->igm_lut_table_B = igm_lut_table_B;
		pinfo->igm_lut_table_len = ARRAY_SIZE(igm_lut_table_R);

		pinfo->gmp_support = 1;
		pinfo->gmp_lut_table_low32bit = &gmp_lut_table_low32bit[GMP_LUT0][GMP_LUT0][GMP_LUT0];
		pinfo->gmp_lut_table_high4bit = &gmp_lut_table_high4bit[GMP_LUT0][GMP_LUT0][GMP_LUT0];
		pinfo->gmp_lut_table_len = ARRAY_SIZE(gmp_lut_table_low32bit);

		pinfo->xcc_support = 1;
		pinfo->xcc_table = xcc_table;
		pinfo->xcc_table_len = ARRAY_SIZE(xcc_table);
		pinfo->comform_mode_support = 1;
		pinfo->cinema_mode_support = 0;		/*cinema_mode1.0,not for use now*/
	}

	if(pinfo->comform_mode_support == COMFORM_SUPPORT){
		support_mode = (support_mode | COMFORM_MODE);
	}
	if(pinfo->cinema_mode_support == CINEMA_SUPPORT){
		support_mode = (support_mode | CINEMA_MODE);
	}
	g_support_mode = support_mode | LED_RG_COLOR_TEMP_MODE;

	pinfo->color_temp_rectify_support = 1;
	pinfo->color_temp_rectify_R = 32768; /*100% percent*/
	pinfo->color_temp_rectify_G = 32768; /*100% percent*/
	pinfo->color_temp_rectify_B = 32768; /*100% percent*/

	pinfo->panel_effect_support = 1;

	/* hiace */
	pinfo->hiace_support = 1;
	if (pinfo->hiace_support == HIACE_SUPPORT) {
		pinfo->hiace_param.iGlobalHistBlackPos = 16;
		pinfo->hiace_param.iGlobalHistWhitePos = 240;
		pinfo->hiace_param.iGlobalHistBlackWeight = 51;
		pinfo->hiace_param.iGlobalHistWhiteWeight = 51;
		pinfo->hiace_param.iGlobalHistZeroCutRatio = 486;
		pinfo->hiace_param.iGlobalHistSlopeCutRatio = 410;
		pinfo->hiace_param.iMaxLcdLuminance = 500;
		pinfo->hiace_param.iMinLcdLuminance = 3;
		strncpy(pinfo->hiace_param.chCfgName, "/product/etc/display/effect/algorithm/hdr_engine_DUKE.xml", sizeof(pinfo->hiace_param.chCfgName) - 1);
	}

	/* ldi */
	pinfo->ldi.h_back_porch = 23;
	pinfo->ldi.h_front_porch = 50;
	pinfo->ldi.h_pulse_width = 20;
	pinfo->ldi.v_back_porch = 40;
	pinfo->ldi.v_front_porch = 10;
	pinfo->ldi.v_pulse_width = 8;
	pinfo->pxl_clk_rate = 288 * 1000000UL;

	/* mipi clock begin */
	pinfo->mipi.dsi_bit_clk = 333;
	pinfo->mipi.dsi_bit_clk_val1 = 471;
	pinfo->mipi.dsi_bit_clk_val2 = 480;
	pinfo->mipi.dsi_bit_clk_val3 = 490;
	pinfo->mipi.dsi_bit_clk_val4 = 500;
	/* pinfo->mipi.dsi_bit_clk_val5 = ; */
	pinfo->dsi_bit_clk_upt_support = 0;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = 0;

	/* non_continue adjust : measured in UI
	* sharp requires clk_post >= 60ns + 252ui,
	* Here 300 is used,300 means about 400ns measure
	* by oscilloscope.
	*/
	pinfo->mipi.clk_post_adjust = 300;
	pinfo->mipi.clk_pre_adjust= 0;
	pinfo->mipi.clk_t_hs_prepare_adjust= 0;
	pinfo->mipi.clk_t_lpx_adjust= 0;
	pinfo->mipi.clk_t_hs_trial_adjust= 0;
	pinfo->mipi.clk_t_hs_exit_adjust= 0;
	pinfo->mipi.clk_t_hs_zero_adjust= 0;
	pinfo->mipi.non_continue_en = 1;
	pinfo->pxl_clk_rate_div = 1;
	pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF
		| VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;
	/* mipi clock end */

	/* Dirty Region Update begin */
	pinfo->dirty_region_updt_support = 1;
	pinfo->dirty_region_info.left_align = -1;
	pinfo->dirty_region_info.right_align = -1;
	pinfo->dirty_region_info.top_align = 2;
	pinfo->dirty_region_info.bottom_align = 2;
	pinfo->dirty_region_info.w_align = -1;
	pinfo->dirty_region_info.h_align = -1;
	pinfo->dirty_region_info.w_min = 1440;
	pinfo->dirty_region_info.h_min = 8;
	pinfo->dirty_region_info.top_start = -1;
	pinfo->dirty_region_info.bottom_start = -1;

	/*The host processor must wait for more than 15us from
	**the end of write data transfer to a command 2Ah/2Bh
	*/
	if (pinfo->dirty_region_updt_support == DIRTY_SUPPORT)
		/* measured in nS */
		pinfo->mipi.hs_wr_to_time = 17000;
	/* Dirty Region Update end */
	/* IFBC Setting begin */
	pinfo->ifbc_type = IFBC_TYPE_RSP3X;
	pinfo->ifbc_cmp_dat_rev0 = 0;
	pinfo->ifbc_cmp_dat_rev1 = 0;
	pinfo->ifbc_auto_sel = 1;

	pinfo->pxl_clk_rate_div *= RATE_DIV;

	if (pinfo->pxl_clk_rate_div > RATE_DIV_SET) {
		pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
	}
	/* IFBC Setting end */

	ret = vcc_cmds_tx(pdev, lcd_vcc_init_cmds,
		ARRAY_SIZE(lcd_vcc_init_cmds));
	if (ret != 0) {
		HISI_FB_ERR("LCD vcc init failed!\n");
		goto err_return;
	}

	ret = pinctrl_cmds_tx(pdev, lcd_pinctrl_init_cmds,
		ARRAY_SIZE(lcd_pinctrl_init_cmds));
	if (ret != 0) {
		HISI_FB_ERR("Init pinctrl failed, defer\n");
		goto err_return;
	}

	if (is_fastboot_display_enable()) {
		vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
			ARRAY_SIZE(lcd_vcc_enable_cmds));
	}

	ret = platform_device_add_data(pdev, &g_panel_data,
		sizeof(struct hisi_fb_panel_data));
	if (ret) {
		HISI_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	hisi_fb_add_device(pdev);

	HISI_FB_DEBUG("-.\n");

	return ret_ok;

err_device_put:
	platform_device_put(pdev);
err_return:
	return ret;
err_probe_defer:
	return -EPROBE_DEFER;
}

static const struct of_device_id hisi_panel_match_table[] = {
	{
		.compatible = DTS_COMP_TIANMA_DUKE_TD4302_5P7,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_lcd_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		   .name = "mipi_tianma_duke_TD4302_5P7",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(hisi_panel_match_table),
	},
};

static int __init mipi_lcd_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_lcd_panel_init);
