/* Copyright (c) 2008-2014, Hisilicon Tech. Co., Ltd. All rights reserved.
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 and
* only version 2 as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#include "hisi_fb.h"

#define DTS_COMP_JDI_ER63450 "hisilicon,mipi_jdi_eR63450"

static struct hisi_fb_panel_data g_panel_data;
static int g_lcd_fpga_flag;

#define LCD_POWER_STATUS_CHECK  (1)
//lint -save -e569 -e730 -e838 -e846 -e514 -e866 -e30 -e31 -e84 -e747 -e516 -e712 -e732 -e778 -e438 -e737 -e702 -e713 -e774 -e734 -e574  -e785  -e64 -e655 -e527 -e655

/*******************************************************************************
** Power ON Sequence video mode(sleep mode to Normal mode) begin
*/
static char bl_enable[] = {
	0x53,
	0x24,
};

static char te_enable[] = {
	0x35,
	0x00,
};

static char nvm_access_protect_open[] = {
	0xB0,
	0x04,
};

static char dsi_control[] = {
	0xb6,
	0x39,
	0xb3,
};

static char display_setting_2[] = {
	0xC2,
	0x01,
	0x1A,
	0x00,
	0x38,
	0x00,
	0x34,
	0xF0,
	0x00,
	0x00,
};

static char interface_setting_video[] = {
	0xB3,
	0x31,//0x10,//0x31
	0x00,
	0x82,
};

static char interface_setting_cmd[] = {
	0xB3,
	0x00,
	0x00,
	0x82,
};

static char compression_method_video[] = {
	0xEB,
	0x83,
	0x00,
};

static char compression_method_cmd[] = {
	0xEB,
	0x00,
	0x83,
};

static char compression_mode_setting[] = {
	0xF7,
	0x07,
};

static char compression_setting[] = {
	0xFB,
	0x00,
};

static char test_control_1[] = {
	0xD6,
	0x01,
};

static char nvm_access_protect_close[] = {
	0xB0,
	0x03,
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

/*******************************************************************************
** Power OFF Sequence(Normal to power off) begin
*/
static char display_off[] = {
	0x28,
};

static char enter_sleep[] = {
	0x10,
};

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00,0x05,0x9f
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00,0x09,0xfF
};

static struct dsi_cmd_desc set_display_address[] = {
	{DTYPE_DCS_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(lcd_disp_y), lcd_disp_y},
};

/*
** Power OFF Sequence(Normal to power off) end
*******************************************************************************/

static struct dsi_cmd_desc lg_display_on_video_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(te_enable), te_enable},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(nvm_access_protect_open), nvm_access_protect_open},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(dsi_control), dsi_control},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(display_setting_2), display_setting_2},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(interface_setting_video), interface_setting_video},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(compression_method_video), compression_method_video},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(compression_mode_setting), compression_mode_setting},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(compression_setting), compression_setting},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_control_1), test_control_1},
	{DTYPE_GEN_LWRITE, 0, 50, WAIT_TYPE_US,
		sizeof(nvm_access_protect_close), nvm_access_protect_close},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc lg_display_on_cmd_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(te_enable), te_enable},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(nvm_access_protect_open), nvm_access_protect_open},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(interface_setting_cmd), interface_setting_cmd},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(compression_method_cmd), compression_method_cmd},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(compression_mode_setting), compression_mode_setting},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(compression_setting), compression_setting},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_control_1), test_control_1},
	{DTYPE_GEN_LWRITE, 0, 50, WAIT_TYPE_US,
		sizeof(nvm_access_protect_close), nvm_access_protect_close},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc lg_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 52, WAIT_TYPE_MS,sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 102, WAIT_TYPE_MS,sizeof(enter_sleep), enter_sleep},
};

/******************************************************************************
*
** Display Effect Sequence(smart color, edge enhancement, smart contrast, cabc)
*/
static char PWM_OUT_0x51[] = {
	0x51,
	0xFE,
};

static struct dsi_cmd_desc pwm_out_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(PWM_OUT_0x51), PWM_OUT_0x51},
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME      "lcdio-vcc"
#define VCC_LCDANALOG_NAME  "lcdanalog-vcc"

static struct regulator *vcc_lcdio;
static struct regulator *vcc_lcdanalog;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0},
	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDIO_NAME, &vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_ENABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCDIO_NAME, &vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
	{DTYPE_VCC_DISABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
};


/*******************************************************************************
** LCD IOMUX
*/
static struct pinctrl_data pctrl;

static struct pinctrl_cmd_desc lcd_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc lcd_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc lg_lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &pctrl, 0},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_BL_ENABLE_NAME "gpio_lcd_bl_enable"
#define GPIO_LCD_RESET_NAME "gpio_lcd_reset"
#define GPIO_LCD_P5V5_ENABLE_NAME   "gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"

static uint32_t gpio_lcd_bl_enable;
static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_p5v5_enable;
static uint32_t gpio_lcd_n5v5_enable;

static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
    /* AVDD_5.5V */
    {DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
    /* AVEE_-5.5V */
    {DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
    /* reset */
    {DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_normal_cmds[] = {
    /* AVDD_5.5V */
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 1},
    /* AVEE_-5.5V */
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
        GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 1},
    /* reset */
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
     /* backlight enable */
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {
    /* AVEE_-5.5V */
    {DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
    /* AVDD_5.5V */
    {DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
    /* reset */
    {DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
    /* reset */
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
    /* AVEE_-5.5V */
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
    /* AVDD_5.5V*/
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
    /* reset */
    {DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},

    /* AVEE_-5.5V input */
    {DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
    /* AVDD_5.5V input */
    {DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
        GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
    /* reset input */
    {DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
        GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

/*******************************************************************************
**
*/
static int mipi_lg_panel_set_fastboot(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	// lcd pinctrl normal
	pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
	ARRAY_SIZE(lcd_pinctrl_normal_cmds));

	// lcd gpio request
	gpio_cmds_tx(asic_lcd_gpio_request_cmds, \
		ARRAY_SIZE(asic_lcd_gpio_request_cmds));

	// backlight on
	hisi_lcd_backlight_on(pdev);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_lg_panel_on(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
#if LCD_POWER_STATUS_CHECK
	uint32_t status = 0;
	uint32_t try_times = 0;
#endif

	char bl_level_adjust[2] = {
		0x51,
		0x64,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	if (pdev == NULL) {
		return -1;
	}
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return -1;
	}

	HISI_FB_INFO("fb%d, +!\n", hisifd->index);

	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {

		// lcd vcc enable
		vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
		ARRAY_SIZE(lcd_vcc_enable_cmds));

		// lcd pinctrl normal
		pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
			ARRAY_SIZE(lcd_pinctrl_normal_cmds));

		/* lcd gpio request */
		gpio_cmds_tx(asic_lcd_gpio_request_cmds, \
			ARRAY_SIZE(asic_lcd_gpio_request_cmds));

		/* lcd gpio normal */
		gpio_cmds_tx(asic_lcd_gpio_normal_cmds, \
			ARRAY_SIZE(asic_lcd_gpio_normal_cmds));


		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {

    mipi_dsi_cmds_tx(set_display_address, \
		ARRAY_SIZE(set_display_address), mipi_dsi0_base);
   if (pinfo->type == PANEL_MIPI_CMD) {
	mipi_dsi_cmds_tx(lg_display_on_cmd_cmds, \
			ARRAY_SIZE(lg_display_on_cmd_cmds), mipi_dsi0_base);
	} else {
	mipi_dsi_cmds_tx(lg_display_on_video_cmds, \
		ARRAY_SIZE(lg_display_on_video_cmds), mipi_dsi0_base);
	}
	mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
		ARRAY_SIZE(lcd_bl_level_adjust), mipi_dsi0_base);
	if ((pinfo->bl_set_type & BL_SET_BY_BLPWM) || (pinfo->bl_set_type & BL_SET_BY_SH_BLPWM)) {
		mipi_dsi_cmds_tx(pwm_out_on_cmds, \
			ARRAY_SIZE(pwm_out_on_cmds), mipi_dsi0_base);
	}
	//udelay(500);

	//#if LCD_POWER_STATUS_CHECK
	//check lcd power status
	outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0x0A06);
	status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	while (status & 0x10) {
		udelay(50);
		if (++try_times > 100) {
			try_times = 0;
			HISI_FB_ERR("Read lcd power status timeout!\n");
			break;
		}

		status = inp32(mipi_dsi0_base + MIPIDSI_CMD_PKT_STATUS_OFFSET);
	}

	status = inp32(mipi_dsi0_base + MIPIDSI_GEN_PLD_DATA_OFFSET);
	HISI_FB_ERR("LCD Power State = 0x%x.\n", status);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		;
	} else {
		HISI_FB_ERR("failed to init lcd!\n");
	}

	/* backlight on */
	hisi_lcd_backlight_on(pdev);

	HISI_FB_INFO("fb%d, -!\n", hisifd->index);

	return 0;
}

static int mipi_lg_panel_off(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	if (pdev == NULL) {
		return -1;
	}
	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return -1;
	}

	HISI_FB_INFO("fb%d, +!\n", hisifd->index);

	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		// backlight off
		hisi_lcd_backlight_off(pdev);
		mipi_dsi_cmds_tx(lg_display_off_cmds, \
			ARRAY_SIZE(lg_display_off_cmds), mipi_dsi0_base);
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (g_lcd_fpga_flag == 0) {
			// lcd gpio lowpower
			gpio_cmds_tx(asic_lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds));

			/* lcd gpio free */
			gpio_cmds_tx(asic_lcd_gpio_free_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_free_cmds));

			/* lcd pinctrl lowpower */
			pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

			/* lcd vcc disable */
			vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
				ARRAY_SIZE(lcd_vcc_disable_cmds));
		}
	} else {
			HISI_FB_ERR("failed to uninit lcd!\n");
		}

	HISI_FB_INFO("fb%d, -!\n", hisifd->index);

	return 0;
}

static int mipi_lg_panel_remove(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);

	if (!hisifd) {
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (g_lcd_fpga_flag == 0) {
		// lcd vcc finit
		vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
			ARRAY_SIZE(lcd_vcc_finit_cmds));

		// lcd pinctrl finit
		pinctrl_cmds_tx(pdev, lg_lcd_pinctrl_finit_cmds,
			ARRAY_SIZE(lg_lcd_pinctrl_finit_cmds));
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_lg_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	int ret = 0;
	char __iomem *mipi_dsi0_base = NULL;
	struct hisi_fb_data_type *hisifd = NULL;

	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
		sizeof(bl_level_adjust), bl_level_adjust},
	};

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, bl_level=%d.\n", hisifd->index, bl_level);

	if (hisifd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		ret = hisi_pwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {
		ret = hisi_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM) {
		ret = hisi_sh_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		mipi_dsi0_base = hisifd->mipi_dsi0_base;
		// cppcheck-suppress *
		bl_level_adjust[1] = bl_level * 255 / hisifd->panel_info.bl_max;
		mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
		ARRAY_SIZE(lcd_bl_level_adjust), mipi_dsi0_base);
	} else {
		HISI_FB_ERR("fb%d, not support this bl_set_type(%d)!\n",
		hisifd->index, hisifd->panel_info.bl_set_type);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static int mipi_lg_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	struct hisi_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL || dirty == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	pinfo = &(hisifd->panel_info);

	if ((dirty->x != 0) || (dirty->w != pinfo->xres) ||
		((dirty->y % 32) != 0) || ((dirty->h % 32) != 0) ||
		(dirty->x >= pinfo->xres) || (dirty->w > pinfo->xres) || ((dirty->x + dirty->w) > pinfo->xres) ||
		(dirty->y >= pinfo->yres) || (dirty->h > pinfo->yres) || ((dirty->y + dirty->h) > pinfo->yres)) {
		HISI_FB_ERR("dirty_region(%d,%d, %d,%d) not support!\n",
		dirty->x, dirty->y, dirty->w, dirty->h);

		BUG_ON(1);
	}

	lcd_disp_x[1] = (dirty->x >> 8) & 0xff;
	lcd_disp_x[2] = dirty->x & 0xff;
	lcd_disp_x[3] = ((dirty->x + dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = (dirty->x + dirty->w - 1) & 0xff;
	lcd_disp_y[1] = (dirty->y >> 8) & 0xff;
	lcd_disp_y[2] = dirty->y & 0xff;
	lcd_disp_y[3] = ((dirty->y + dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = (dirty->y + dirty->h - 1) & 0xff;

	mipi_dsi_cmds_tx(set_display_address, \
		ARRAY_SIZE(set_display_address), mipi_dsi0_base);

	return 0;
}

/******************************************************************************/
static ssize_t mipi_lg_panel_lcd_model_show(struct platform_device *pdev,
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

	ret = snprintf(buf, PAGE_SIZE, "LG_eR63450 ' CMD TFT Chicago\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_lg_panel_lcd_check_reg_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[4] = {0};
	uint32_t expected_value[4] = {0x1c, 0x00, 0x07, 0x00};
	uint32_t read_mask[4] = {0xFF, 0xFF, 0x07, 0xFF};
	char* reg_name[4] = {"power mode", "MADCTR", "pixel format", "image mode"};
	char lcd_reg_0a[] = {0x0a};
	char lcd_reg_0b[] = {0x0b};
	char lcd_reg_0c[] = {0x0c};
	char lcd_reg_0d[] = {0x0d};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0b), lcd_reg_0b},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0c), lcd_reg_0c},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0d), lcd_reg_0d},
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
		ret = snprintf(buf, PAGE_SIZE, "OK\n");
	} else {
		ret = snprintf(buf, PAGE_SIZE, "ERROR\n");
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}


/*******************************************************************************
**
*/
static struct hisi_panel_info g_panel_info = {0};
static struct hisi_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_lg_panel_set_fastboot,
	.on = mipi_lg_panel_on,
	.off = mipi_lg_panel_off,
	.remove = mipi_lg_panel_remove,
	.set_backlight = mipi_lg_panel_set_backlight,
	.set_display_region = mipi_lg_panel_set_display_region,

	.lcd_model_show = mipi_lg_panel_lcd_model_show,
	.lcd_check_reg  = mipi_lg_panel_lcd_check_reg_show,
};

/*******************************************************************************
**
*/
static int mipi_jdi_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_panel_info *pinfo = NULL;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	uint32_t lcd_ifbc_type = 0;
	np = of_find_compatible_node(NULL, NULL, DTS_COMP_JDI_ER63450);
	if (!np) {
		HISI_FB_ERR("NOT FOUND device node %s!\n", DTS_COMP_JDI_ER63450);
		goto err_return;
	}
	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, &lcd_display_type);
	if (ret) {
		HISI_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}
	HISI_FB_INFO("lcd_display_type=%u!\n", lcd_display_type);

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME, &lcd_ifbc_type);
	if (ret) {
		HISI_FB_ERR("get ifbc_type failed!\n");
		lcd_ifbc_type = IFBC_TYPE_NONE;
	}
	HISI_FB_INFO("lcd_ifbc_type=%u!\n", lcd_ifbc_type);

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &bl_type);
	if (ret) {
		HISI_FB_ERR("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	HISI_FB_INFO("bl_type=0x%x.", bl_type);

	if (hisi_fb_device_probe_defer(lcd_display_type, bl_type)) {
		goto err_probe_defer;
	}
	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &g_lcd_fpga_flag);
	if (ret) {
		HISI_FB_WARNING("need to get g_lcd_fpga_flag resource in fpga, not needed in asic!\n");
	}

	HISI_FB_DEBUG("+.\n");

	HISI_FB_INFO("%s\n", DTS_COMP_JDI_ER63450);

	gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", 0);// gpio_15_5;
	gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", 1);//gpio_4_5
	gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 2);//gpio_11_0;
	gpio_lcd_reset = of_get_named_gpio(np, "gpios", 3);//gpio_5_0;


	HISI_FB_INFO("enn=%d,enp=%d,rst=%d",gpio_lcd_n5v5_enable,gpio_lcd_p5v5_enable,gpio_lcd_reset);
	pdev->id = 1;
	// init lcd panel info
	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct hisi_panel_info));
	pinfo->xres = 1440;
	pinfo->yres = 2560;
	pinfo->width = 75;
	pinfo->height = 133;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;
	pinfo->bl_min = 4;
	pinfo->bl_max = 255;
	pinfo->bl_default = 102;
	pinfo->type = lcd_display_type;//PANEL_MIPI_CMD;
	pinfo->ifbc_type =IFBC_TYPE_RSP3X;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->lcd_adjust_support = 0;
	pinfo->color_temperature_support = 0;

	if (is_mipi_cmd_panel_ext(pinfo)) {
		pinfo->ldi.h_back_porch = 46;
		pinfo->ldi.h_front_porch = 50;
		pinfo->ldi.h_pulse_width = 48;
		pinfo->ldi.v_back_porch = 38;
		pinfo->ldi.v_front_porch = 14;
		pinfo->ldi.v_pulse_width = 8;
	} else {
        pinfo->ldi.h_back_porch = 46;
		pinfo->ldi.h_front_porch = 106;
		pinfo->ldi.h_pulse_width = 48;
		pinfo->ldi.v_back_porch = 51;
		pinfo->ldi.v_front_porch = 14;
		pinfo->ldi.v_pulse_width = 8;

	}

	pinfo->pxl_clk_rate = 260 * 1000000UL;
	//mipi
	pinfo->mipi.dsi_bit_clk = 480;
	pinfo->dsi_bit_clk_upt_support = 0;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;


	pinfo->mipi.non_continue_en = 0;
	//mipi
	pinfo->mipi.clk_post_adjust = 300;
	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;

	pinfo->pxl_clk_rate_div = 1;
	if((pinfo->ifbc_type == IFBC_TYPE_RSP3X)&&(pinfo->type == PANEL_MIPI_CMD)) {
		pinfo->pxl_clk_rate_div *= 3;
		pinfo->ifbc_cmp_dat_rev0 = 0;
		pinfo->ifbc_cmp_dat_rev1 = 0;
		pinfo->ifbc_auto_sel = 1;

		pinfo->ldi.h_back_porch *= 2;
		pinfo->ldi.h_front_porch *= 2;
		pinfo->ldi.h_pulse_width *= 2;
		pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.v_back_porch /= 2;
		pinfo->ldi.v_front_porch /= 2;
		pinfo->ldi.v_pulse_width /= 2;
	} else 	if((pinfo->ifbc_type == IFBC_TYPE_RSP3X)&&(pinfo->type == PANEL_MIPI_VIDEO)) {

		pinfo->pxl_clk_rate_div *= 3;
		pinfo->ifbc_cmp_dat_rev0 = 0;
		pinfo->ifbc_cmp_dat_rev1 = 0;
		pinfo->ifbc_auto_sel = 1;

		pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;

	}

	//is command_mode or video_mode
	if (is_mipi_cmd_panel_ext(pinfo)) {
		pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;
		pinfo->dirty_region_updt_support = 0;
		pinfo->dirty_region_info.left_align = -1;
		pinfo->dirty_region_info.right_align = -1;
		pinfo->dirty_region_info.top_align = 32;
		pinfo->dirty_region_info.bottom_align = 32;
		pinfo->dirty_region_info.w_align = -1;
		pinfo->dirty_region_info.h_align = -1;
		pinfo->dirty_region_info.w_min = pinfo->xres;
		pinfo->dirty_region_info.h_min = -1;
		pinfo->dirty_region_info.top_start = -1;
		pinfo->dirty_region_info.bottom_start = -1;
	} else {
		pinfo->vsync_ctrl_type = 0;
		pinfo->dirty_region_updt_support = 0;
	}

	if (g_lcd_fpga_flag == 0) {
		// lcd vcc init
		ret = vcc_cmds_tx(pdev, lcd_vcc_init_cmds,
			ARRAY_SIZE(lcd_vcc_init_cmds));
		if (ret != 0) {
			HISI_FB_ERR("LCD vcc init failed!\n");
			goto err_return;
		}

		// lcd pinctrl init
		ret = pinctrl_cmds_tx(pdev, lcd_pinctrl_init_cmds,
			ARRAY_SIZE(lcd_pinctrl_init_cmds));
		if (ret != 0) {
			HISI_FB_ERR("Init pinctrl failed, defer\n");
			goto err_return;
		}

		// lcd vcc enable
		if (is_fastboot_display_enable()) {
			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
				ARRAY_SIZE(lcd_vcc_enable_cmds));
		}
	}

	// alloc panel device data
	ret = platform_device_add_data(pdev, &g_panel_data,
		sizeof(struct hisi_fb_panel_data));
	if (ret) {
		HISI_FB_ERR("platform_device_add_data failed!\n");
		goto err_device_put;
	}

	hisi_fb_add_device(pdev);

	HISI_FB_DEBUG("-.\n");

	return 0;

err_device_put:
	platform_device_put(pdev);
err_return:
	return ret;
err_probe_defer:
	return -EPROBE_DEFER;

	return ret;
}

static const struct of_device_id hisi_panel_match_table[] = {
	{
		.compatible = DTS_COMP_JDI_ER63450,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_jdi_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
	.name = "mipi_jdi_eR63450",
	.owner = THIS_MODULE,
	.of_match_table = of_match_ptr(hisi_panel_match_table),
	},
};


static int __init mipi_jdi_ER63450_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

/*lint -save -e528 -specific(-e528)*/
module_init(mipi_jdi_ER63450_panel_init);
/*lint -restore*/




//lint -restore

