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
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4,1,0)
#include <linux/hisi/hisi_adc.h>
#else
#include <linux/huawei/hisi_adc.h>
#endif
#include <huawei_platform/touthscreen/huawei_touchscreen.h>
#include <huawei_platform/log/log_jank.h>
#include <linux/hisi/hw_cmdline_parse.h>
#include "display_effect_samsung_S6E3HA3X02_5p5.h"

#define DTS_COMP_SAMSUNG_S6E3HA3X02 "hisilicon,mipi_samsung_S6E3HA3X02_5P5_AMOLED"
#define DRIVER_NAME "SAMSUNG_S6E3HA3X02"
#define USE_LDO17 "use_ldo17"
#define USE_LDO17_ENABLE 1
#define USE_LDO17_DISABLE 0
static struct hisi_fb_panel_data g_panel_data;
static bool g_debug_enable = false;
static bool lcd_rs_poweroff = false;
static int g_hbm_ctrl = 0;
static int g_acl_ctrl = 2;

#define AMOLED_CHECK_INT

#define TP_RS_CALL 1
extern bool g_lcd_control_tp_power;

enum {
	HBM_DEBUG = 0,
	HBM_SETTING = 1,
};
enum {
	HBM_OFF = 0,
	HBM_ON = 1,
};

enum {
	ACL_DEBUG = 0,
	ACL_SETTING = 1,
};
enum {
	ACL_OFF = 0,
	ACL_OFFSET1_ON = 1,
	ACL_OFFSET2_ON = 2,
	ACL_OFFSET3_ON = 3,
};

enum {
    COLOR_EN_SRGB = 0,    /* cmb when close color_enhance, need open gmp */
    COLOR_EN_DEFAULT = 1, /* cmb when open color_enhance, need close gmp */
    COLOR_EN_CLOSED = 2,  /* when close all, close gmp */
};

static int g_color_enhancement_mode = COLOR_EN_DEFAULT;

#define ALPM_DISPLAY_OFF_CMD 0
#define ALPM_ON_50NIT_CMD 	1
#define ALPM_OFF_CMD	2
#define ALPM_ON_10NIT_CMD	3

static bool panel_alpm_on = false;
static bool g_display_on = false;

/*ALPM on setting, 0x02:50nit on ; 0x03:10nit on*/
static char alpm_on_50nit_setting[] = {
	0x53, 0x02,
};

static char alpm_on_10nit_setting[] = {
	0x53, 0x03,
};

/* ALPM off sequence */
static char alpm_off_setting[] = {
	0x53, 0x20,
};

static char gamma_aid_update[] = {
	0xF7, 0x03,
};

/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/
static char exit_sleep[] = {
	0x11,
};

static char test_key_enable[] = {
	0xF0,
	0x5A,0x5A,
};

static char dual_control_C4[] = {
	0xC4, 0x03,
};

static char esd_refresh_setting[] = {
	0xF6,
	0x42, 0x57, 0x37, 0x00, 0xAA,
	0xCC, 0xD0, 0x00,
};

static char esd_refresh_setting1[] = {
	0x12,
};

static char dual_control_F9[] = {
	0xF9, 0x03,
};

static char pentile_setting[] = {
	0xC2,
	0x00, 0x00, 0xD8, 0xD8, 0x00,
	0x80, 0x2B, 0x05, 0x08, 0x0E,
	0x07, 0x0B, 0x05, 0x0D, 0x0A,
	0x15, 0x13, 0x20, 0x1E,
};

static char setting_pcd[] = {
	0xCC,
	0x4C, 0x50,
};

/* ERR_FG VLIN Monitoring On */
static char setting_errflag_mipi_err[] = {
	0xED,
	0x44,
};

static char test_key_disable[] = {
	0xF0,
	0xA5,0xA5,
};

static char tear_on[] = {
	0x35, 0x00,
};

/* 0x36 command means " RED color to Blue color, Blue color to RED color "*/
static char reg_36[] = {
	0x36, 0x00,
};

/* brightness dimming contrl */
static char bl_enable[] = {
	0x53,
	0x20,
};

static char bl_level[] = {
	0x51,
	0x00,
};

/*
Power Saving Function Control Mode
0 = ACL off
1 = ACL 10% on(ACL_OFF_MAX_1)
2 = ACL 20% on(ACL_OFF_MAX_2)
3 = ACL 40% on(ACL_OFF_MAX_3)
*/
static char acl_mode[] = {
	0x55, 0x02,
};

static char memory_access[] = {
	0x2C,
};

static char display_on[] = {
	0x29,
};

static struct dsi_cmd_desc alpm_50nit_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_enable), test_key_enable},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(alpm_on_50nit_setting), alpm_on_50nit_setting},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(gamma_aid_update), gamma_aid_update},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_disable), test_key_disable},
};

static struct dsi_cmd_desc alpm_10nit_on_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_enable), test_key_enable},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(alpm_on_10nit_setting), alpm_on_10nit_setting},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(gamma_aid_update), gamma_aid_update},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_disable), test_key_disable},
};

static struct dsi_cmd_desc alpm_off_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_enable), test_key_enable},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(alpm_off_setting), alpm_off_setting},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(gamma_aid_update), gamma_aid_update},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_disable), test_key_disable},
};

/*******************************************************************************
** Power OFF Sequence(Normal to power off)
*/
static char display_off[] = {
	0x28,
};

static char enter_sleep[] = {
	0x10,
};

static struct dsi_cmd_desc display_on_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 6, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_enable), test_key_enable},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(dual_control_C4), dual_control_C4},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(dual_control_F9), dual_control_F9},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(esd_refresh_setting), esd_refresh_setting},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(pentile_setting), pentile_setting},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(reg_36), reg_36},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(setting_pcd), setting_pcd},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(setting_errflag_mipi_err), setting_errflag_mipi_err},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(test_key_disable), test_key_disable},
	{DTYPE_GEN_LWRITE, 0, 125, WAIT_TYPE_MS,
		sizeof(esd_refresh_setting1), esd_refresh_setting1},

	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_US,
		sizeof(tear_on), tear_on},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 120, WAIT_TYPE_US,
		sizeof(bl_level), bl_level},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_US,
		sizeof(acl_mode), acl_mode},
	{DTYPE_GEN_LWRITE, 0, 120, WAIT_TYPE_US,
		sizeof(memory_access), memory_access},
};

static struct dsi_cmd_desc display_on_cmd[] = {
	{DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc display_off_cmd[] = {
	{DTYPE_DCS_WRITE, 0, 200, WAIT_TYPE_US,
		sizeof(display_off), display_off},
};

static struct dsi_cmd_desc enter_sleep_cmd[] = {
	{DTYPE_DCS_WRITE, 0, 150, WAIT_TYPE_MS,
		sizeof(enter_sleep), enter_sleep}
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdanalog;

static struct vcc_desc lcd_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3300000, 3300000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_ldo17_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},

	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3100000, 3100000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 1},
};

static struct vcc_desc lcd_vcc_disable_cmds[] = {
	/* vcc disable */
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

static struct pinctrl_cmd_desc lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &pctrl, 0},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_1V8_EN_NAME   "gpio_lcd_1v8_en"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_ID0_NAME	       "gpio_lcd_id0"
#define GPIO_LCD_PCD_NAME	"gpio_lcd_pcd"
#define GPIO_LCD_ERR_FLAG_NAME "gpio_lcd_errflag"

static uint32_t gpio_lcd_1v8_en;
static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_id0;
static uint32_t gpio_lcd_pcd;
static uint32_t gpio_lcd_err_flag;

static struct gpio_desc lcd_gpio_request_cmds[] = {
	/*pcd*/
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_PCD_NAME, &gpio_lcd_pcd, 0},
	/*err flag*/
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ERR_FLAG_NAME, &gpio_lcd_err_flag, 0},
	/*vdd 1.8v*/
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_1V8_EN_NAME, &gpio_lcd_1v8_en, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* id0 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &gpio_lcd_id0, 0},
};

static struct gpio_desc lcd_gpio_free_cmds[] = {
	/*vdd 1.8v*/
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_1V8_EN_NAME, &gpio_lcd_1v8_en, 0},
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* id */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &gpio_lcd_id0, 0},
	/*pcd*/
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_PCD_NAME, &gpio_lcd_pcd, 0},
	/*err flag*/
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_ERR_FLAG_NAME, &gpio_lcd_err_flag, 0},
};

static struct gpio_desc lcd_gpio_normal_cmds[] = {
	/*vdd 1.8v*/
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_1V8_EN_NAME, &gpio_lcd_1v8_en, 1},
	/* id */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ID0_NAME, &gpio_lcd_id0, 0},
	/*pcd*/
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_PCD_NAME, &gpio_lcd_pcd, 0},
	/*err flag*/
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_ERR_FLAG_NAME, &gpio_lcd_err_flag, 0},
};

static struct gpio_desc lcd_gpio_reset_cmds[] = {
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 1,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 6,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
};

static struct gpio_desc lcd_gpio_lowpower_reset_cmds[] = {
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc lcd_gpio_lowpower_1V8_cmds[] = {
	/* 1.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_RESET_NAME, &gpio_lcd_1v8_en, 0},
};

static u32 g_samsung_pcd_record=0;
#ifdef AMOLED_CHECK_INT
static irqreturn_t pcd_irq_isr_func(int irq, void *handle)
{
	if (gpio_get_value(gpio_lcd_pcd) == 1) {
		HISI_FB_INFO("pcd detect!\n");
		g_samsung_pcd_record++;
		/* disable_irq_nosync(gpio_to_irq(gpio_lcd_pcd)); */
	} else {
		HISI_FB_DEBUG("no pcd detect!\n");
	}

	return IRQ_HANDLED;
}

static irqreturn_t errflag_irq_isr_func(int irq, void *handle)
{
	if (gpio_get_value(gpio_lcd_err_flag) == 1) {
		HISI_FB_INFO("err_flag detect!\n");
	} else {
		HISI_FB_DEBUG("no err_flag detect!\n");
	}

	return IRQ_HANDLED;
}

static void amoled_irq_enable(void)
{
	enable_irq(gpio_to_irq(gpio_lcd_pcd));
	enable_irq(gpio_to_irq(gpio_lcd_err_flag));
}

static void amoled_irq_disable(void)
{
	disable_irq(gpio_to_irq(gpio_lcd_pcd));
	disable_irq(gpio_to_irq(gpio_lcd_err_flag));
}
#endif

/*******************************************************************************
**
*/
static int mipi_samsung_panel_set_fastboot(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	/* lcd pinctrl normal */
	pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
		ARRAY_SIZE(lcd_pinctrl_normal_cmds));

	/* lcd gpio request */
	gpio_cmds_tx(lcd_gpio_request_cmds,
		ARRAY_SIZE(lcd_gpio_request_cmds));

	/* backlight on */
	hisi_lcd_backlight_on(pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_samsung_panel_on(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	char __iomem *dpp_base = NULL;
	char __iomem *acm_base = NULL;

	int error = 0;
	char acl_mode_sel[] = {0x55,0x00,};
	struct dsi_cmd_desc acl_enable_cmds[] = {
		{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
			sizeof(acl_mode_sel), acl_mode_sel},
	};
	static struct lcd_reg_read_t lcd_status_reg[] = {
		{0x0A, 0xB4, 0xFF, "lcd power state"},
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_INFO("[%s]fb%d, +!\n", DRIVER_NAME,hisifd->index);

	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->mipi_dsi0_base;
	dpp_base = hisifd->dss_base + DSS_DPP_OFFSET;
	acm_base = hisifd->dss_base + DSS_DPP_ACM_OFFSET;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		if (!lcd_rs_poweroff) {
			if (hisifd->aod_function){
				HISI_FB_INFO("AOD mode, not need power on.\n");
			} else {
				LOG_JANK_D(JLID_KERNEL_LCD_POWER_ON, "%s", "JL_KERNEL_LCD_POWER_ON");
				HISI_FB_INFO("Init power on(regulator enabling).\n");
				/* lcd pinctrl normal */
				pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
					ARRAY_SIZE(lcd_pinctrl_normal_cmds));
				/* lcd gpio request */
				gpio_cmds_tx(lcd_gpio_request_cmds, \
					ARRAY_SIZE(lcd_gpio_request_cmds));

				/* lcd gpio normal 1.8v */
				gpio_cmds_tx(lcd_gpio_normal_cmds, \
					ARRAY_SIZE(lcd_gpio_normal_cmds));

				/* lcd vcc enable 3.1v */
				vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
					ARRAY_SIZE(lcd_vcc_enable_cmds));
			}

		} else {
			HISI_FB_INFO("Init power on(regulator has enabled).\n");
			/* lcd pinctrl normal */
			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));
			/* lcd gpio request */
			gpio_cmds_tx(lcd_gpio_request_cmds, \
				ARRAY_SIZE(lcd_gpio_request_cmds));
		}

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (hisifd->aod_function){
			HISI_FB_INFO("AOD mode.\n");
			panel_check_status_and_report_by_dsm(lcd_status_reg, \
				ARRAY_SIZE(lcd_status_reg), mipi_dsi0_base);
		} else {
			HISI_FB_INFO("Init lcd_init_step is lp send mode.\n");
			if (TP_RS_CALL != g_debug_enable_lcd_sleep_in) {
				HISI_FB_INFO("TP resume and after resume\n");
				error = ts_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);
				error = ts_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);
			}
			/* lcd gpio normal */
			mdelay(10);
			gpio_cmds_tx(lcd_gpio_reset_cmds, \
				ARRAY_SIZE(lcd_gpio_reset_cmds));

			mipi_dsi_cmds_tx(display_on_cmds, \
				ARRAY_SIZE(display_on_cmds), mipi_dsi0_base);
			g_acl_ctrl = 2;
			g_hbm_ctrl = 0;
			if(runmode_is_factory()) {
				HISI_FB_INFO("Factory mode, disable acl.\n");
				mipi_dsi_cmds_tx(acl_enable_cmds, \
					ARRAY_SIZE(acl_enable_cmds), mipi_dsi0_base);
				g_acl_ctrl = 0;
			}
			panel_check_status_and_report_by_dsm(lcd_status_reg, \
				ARRAY_SIZE(lcd_status_reg), mipi_dsi0_base);
		}
		g_debug_enable = true;
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {

		/* this project: 0 means using sRGB, 1 means using color_enhance */

		HISI_FB_INFO("Init hs send data mode.\n");
#ifdef AMOLED_CHECK_INT
		amoled_irq_enable();
#endif
	} else {
		HISI_FB_ERR("failed to init lcd!\n");
	}

	HISI_FB_INFO("[%s]fb%d, -!\n", DRIVER_NAME,hisifd->index);

	return 0;
}

static int mipi_samsung_panel_off(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	int error = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_INFO("[%s]fb%d, +!\n", DRIVER_NAME,hisifd->index);

	pinfo = &(hisifd->panel_info);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "JL_KERNEL_LCD_POWER_OFF");
		HISI_FB_INFO("display off(download display off sequence).\n");

		if (hisifd->aod_function) {
			HISI_FB_INFO("Alpm mode.\n");
		} else {
			/* lcd display off sequence */
			mipi_dsi_cmds_tx(display_off_cmd, \
				ARRAY_SIZE(display_off_cmd), hisifd->mipi_dsi0_base);
			mipi_dsi_cmds_tx(enter_sleep_cmd, \
				ARRAY_SIZE(enter_sleep_cmd), hisifd->mipi_dsi0_base);
		}
		g_display_on = false;
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (!hisifd->fb_shutdown) {
			if (!lcd_rs_poweroff) {
				if (hisifd->aod_function){
					HISI_FB_INFO("AOD mode\n");
				} else {
					HISI_FB_INFO("display off(regulator disabling).\n");
					/* lcd gpio lowpower */
					gpio_cmds_tx(lcd_gpio_lowpower_reset_cmds, \
						ARRAY_SIZE(lcd_gpio_lowpower_reset_cmds));
					/* lcd vcc disable */
					vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
						ARRAY_SIZE(lcd_vcc_disable_cmds));
					/* lcd gpio lowpower */
					gpio_cmds_tx(lcd_gpio_lowpower_1V8_cmds, \
						ARRAY_SIZE(lcd_gpio_lowpower_1V8_cmds));
					/* lcd gpio free */
					gpio_cmds_tx(lcd_gpio_free_cmds, \
						ARRAY_SIZE(lcd_gpio_free_cmds));
					/* lcd pinctrl lowpower */
					pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
						ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));
				}
			} else {
				HISI_FB_INFO("display off(regulator not need disabling).\n");
				/* lcd gpio lowpower */
				gpio_cmds_tx(lcd_gpio_lowpower_reset_cmds, \
					ARRAY_SIZE(lcd_gpio_lowpower_reset_cmds));
				/* lcd gpio free */
				gpio_cmds_tx(lcd_gpio_free_cmds, \
					ARRAY_SIZE(lcd_gpio_free_cmds));
				/* lcd pinctrl normal */
				pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
					ARRAY_SIZE(lcd_pinctrl_normal_cmds));
			}
				if (!hisifd->aod_function) {
					/*if g_debug_enable_lcd_sleep_in == 1, it means don't turn off TP/LCD power
					but only let lcd get into sleep.*/
					if (TP_RS_CALL != g_debug_enable_lcd_sleep_in) {
						HISI_FB_INFO("TP before suspend and suspend\n");
						error = ts_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);
						error = ts_power_control_notify(TS_SUSPEND_DEVICE, SHORT_SYNC_TIMEOUT);
					}
				}
		} else {
			HISI_FB_INFO("display shutting down(regulator disabling).\n");
			HISI_FB_INFO("display off(regulator disabling).\n");
			/* lcd gpio lowpower */
			gpio_cmds_tx(lcd_gpio_lowpower_reset_cmds, \
				ARRAY_SIZE(lcd_gpio_lowpower_reset_cmds));
			/* lcd vcc disable */
			vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
				ARRAY_SIZE(lcd_vcc_disable_cmds));
			/* lcd gpio lowpower */
			gpio_cmds_tx(lcd_gpio_lowpower_1V8_cmds, \
				ARRAY_SIZE(lcd_gpio_lowpower_1V8_cmds));
			/* lcd gpio free */
			gpio_cmds_tx(lcd_gpio_free_cmds, \
				ARRAY_SIZE(lcd_gpio_free_cmds));
			/* lcd pinctrl lowpower */
			pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));
			ts_thread_stop_notify();
		}
#ifdef AMOLED_CHECK_INT
		amoled_irq_disable();
#endif
	} else {
		HISI_FB_ERR("failed to uninit lcd!\n");
	}
	HISI_FB_INFO("[%s]fb%d, -!\n", DRIVER_NAME,hisifd->index);

	return 0;
}

static int mipi_samsung_panel_remove(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	/* lcd vcc finit */
	vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
		ARRAY_SIZE(lcd_vcc_finit_cmds));

	/* lcd pinctrl finit */
	pinctrl_cmds_tx(pdev, lcd_pinctrl_finit_cmds,
		ARRAY_SIZE(lcd_pinctrl_finit_cmds));

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_samsung_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	char __iomem *mipi_dsi0_base = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;
	static char last_bl_level=0;
	char bl_level_adjust[2] = {
		0x51,
		0x00,
	};

	struct dsi_cmd_desc lcd_bl_level_adjust[] = {
		{DTYPE_DCS_WRITE1, 0, 100, WAIT_TYPE_US,
			sizeof(bl_level_adjust), bl_level_adjust},
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (unlikely(g_debug_enable)) {
		HISI_FB_INFO("set cmd display_cmd(0x29)!\n");
		mipi_dsi_cmds_tx(display_on_cmd, ARRAY_SIZE(display_on_cmd), hisifd->mipi_dsi0_base);
		g_display_on = true;
	}

	if (unlikely(g_debug_enable)) {
		HISI_FB_INFO("Set brightness to %d\n", bl_level);
		LOG_JANK_D(JLID_KERNEL_LCD_BACKLIGHT_ON, "JL_KERNEL_LCD_BACKLIGHT_ON,%u", bl_level);
		g_debug_enable = false;
	}

	HISI_FB_INFO("Set brightness to %d\n", bl_level);

	if (hisifd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		ret = hisi_pwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {
		ret = hisi_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		mipi_dsi0_base = hisifd->mipi_dsi0_base;

		bl_level_adjust[1] = bl_level * 255 / hisifd->panel_info.bl_max;
		if (last_bl_level != bl_level_adjust[1]){
			last_bl_level = bl_level_adjust[1];
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
				ARRAY_SIZE(lcd_bl_level_adjust), mipi_dsi0_base);
		}
	} else {
		HISI_FB_ERR("No such bl_set_type!\n");
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_check_reg_show(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[6] = {0};
	uint32_t expected_value[6] = {0xB4, 0x00, 0x77, 0x00, 0x80, 0xC0};
	uint32_t read_mask[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
	char* reg_name[6] = {
		"power mode",
		"MADCTR",
		"pixel format",
		"image mode"
		"RDDSM"
		"RDDSDR"
	};
	char lcd_reg_0a[] = {0x0a};
	char lcd_reg_0b[] = {0x0b};
	char lcd_reg_0c[] = {0x0c};
	char lcd_reg_0d[] = {0x0d};
	char lcd_reg_0e[] = {0x0e};
	char lcd_reg_0f[] = {0x0f};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0b), lcd_reg_0b},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0c), lcd_reg_0c},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0d), lcd_reg_0d},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0f), lcd_reg_0f},
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
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (!mipi_dsi_read_compare(&data, mipi_dsi0_base)) {
		ret = snprintf(buf, PAGE_SIZE,
			"P-0x0a:0x%x, 0x0b:0x%x, 0x0c:0x%x, 0x0d:0x%x, 0x0e:0x%x, 0x0f:0x%x\n",
			data.read_value[0], data.read_value[1], data.read_value[2], data.read_value[3],
			data.read_value[4], data.read_value[5]);
	} else {
		ret = snprintf(buf, PAGE_SIZE,
			"F-0x0a:0x%x, 0x0b:0x%x, 0x0c:0x%x, 0x0d:0x%x, 0x0e:0x%x, 0x0f:0x%x\n",
			data.read_value[0], data.read_value[1], data.read_value[2], data.read_value[3],
			data.read_value[4], data.read_value[5]);
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}


/******************************************************************************/
static ssize_t mipi_samsung_panel_model_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "samsung_S6E3HA3X02 5.5' CMD AMOLED\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_info_show(struct platform_device *pdev, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev NULL pointer\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd NULL pointer\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (buf) {
		ret = snprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,lcdtype:%s,\n",
				hisifd->panel_info.bl_max, hisifd->panel_info.bl_min, "AMOLED");
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static char lcd_disp_x[] = {
	0x2A,
	0x00, 0x00,0x04,0x37
};

static char lcd_disp_y[] = {
	0x2B,
	0x00, 0x00,0x07,0x7F
};

static struct dsi_cmd_desc set_display_address[] = {
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, 0, 5, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y},
};

static int mipi_samsung_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (pdev == NULL || dirty == NULL) {
		return -1;
	}

	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return -1;
	}

	lcd_disp_x[1] = (dirty->x >> 8) & 0xff;
	lcd_disp_x[2] = dirty->x & 0xff;
	lcd_disp_x[3] = ((dirty->x + dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = (dirty->x + dirty->w - 1) & 0xff;
	lcd_disp_y[1] = (dirty->y >> 8) & 0xff;
	lcd_disp_y[2] = dirty->y & 0xff;
	lcd_disp_y[3] = ((dirty->y + dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = (dirty->y + dirty->h - 1) & 0xff;

	HISI_FB_DEBUG("x[1] = 0x%2x, x[2] = 0x%2x, x[3] = 0x%2x, x[4] = 0x%2x.\n",
		lcd_disp_x[1], lcd_disp_x[2], lcd_disp_x[3], lcd_disp_x[4]);
	HISI_FB_DEBUG("y[1] = 0x%2x, y[2] = 0x%2x, y[3] = 0x%2x, y[4] = 0x%2x.\n",
		lcd_disp_y[1], lcd_disp_y[2], lcd_disp_y[3], lcd_disp_y[4]);

	mipi_dsi_cmds_tx(set_display_address, \
		ARRAY_SIZE(set_display_address), hisifd->mipi_dsi0_base);

	return 0;
}

static ssize_t mipi_samsung_panel_hbm_ctrl_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "%d\n", g_hbm_ctrl);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_hbm_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	unsigned int value[100];
	char *token, *cur;
	int i = 0;
	char payload_hbm_para[5] = {0};
	struct dsi_cmd_desc hbm_debug_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_enable), test_key_enable},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(payload_hbm_para), payload_hbm_para},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_disable), test_key_disable},
	};
	char payload_hbm_switch[2] = {0};
	struct dsi_cmd_desc hbm_switch_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_enable), test_key_enable},
		{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
			sizeof(payload_hbm_switch), payload_hbm_switch},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_disable), test_key_disable},
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	if (NULL == buf) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;
	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	cur = buf;
	while (token = strsep(&cur, ",")) {
		value[i++] = simple_strtol(token, NULL, 0);
	}

	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("panel off, power on first\n");
		goto hbm_set_err;
	}

	hisifb_activate_vsync(hisifd);
	if (HBM_DEBUG == value[0]) { /* debug */
		hbm_debug_cmd[1].payload[0] = 0xb4;
		hbm_debug_cmd[1].payload[1] = value[1];
		hbm_debug_cmd[1].payload[2] = value[2];
		hbm_debug_cmd[1].payload[3] = value[3];
		hbm_debug_cmd[1].payload[4] = value[4];
		mipi_dsi_cmds_tx(hbm_debug_cmd, ARRAY_SIZE(hbm_debug_cmd), mipi_dsi0_base);
	} else if (HBM_SETTING == value[0]) {
		if (HBM_OFF == value[1]) {
			hbm_switch_cmd[1].payload[0] = 0x53;
			hbm_switch_cmd[1].payload[1] = (0x20 & 0xff);
			mipi_dsi_cmds_tx(hbm_switch_cmd, ARRAY_SIZE(hbm_switch_cmd), mipi_dsi0_base);
			g_hbm_ctrl = HBM_OFF;
			HISI_FB_INFO("HBM OFF\n");
		} else if (HBM_ON == value[1]) {
			hbm_switch_cmd[1].payload[0] = 0x53;
			hbm_switch_cmd[1].payload[1] = ((0x20 | 0xc0) & 0xff);
			mipi_dsi_cmds_tx(hbm_switch_cmd, ARRAY_SIZE(hbm_switch_cmd), mipi_dsi0_base);
			g_hbm_ctrl = HBM_ON;
			HISI_FB_INFO("HBM ON\n");
		} else {
			HISI_FB_INFO("invalid parm!\n");
		}
	} else {
		HISI_FB_INFO("invalid parm!\n");
	}
	hisifb_deactivate_vsync(hisifd);

hbm_set_err:
	up(&hisifd->blank_sem);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
	return count;
}

static ssize_t mipi_samsung_panel_acl_ctrl_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "%d\n", g_acl_ctrl);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_acl_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	unsigned int value[100];
	char *token, *cur;
	int i = 0;
	char payload_acl_para[6] = {0};
	struct dsi_cmd_desc acl_debug_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_enable), test_key_enable},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(payload_acl_para), payload_acl_para},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_disable), test_key_disable},
	};
	char payload_acl_switch[2] = {0};
	struct dsi_cmd_desc acl_switch_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_enable), test_key_enable},
		{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
			sizeof(payload_acl_switch), payload_acl_switch},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(test_key_disable), test_key_disable},
	};

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	if (NULL == buf) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}
	mipi_dsi0_base = hisifd->mipi_dsi0_base;
	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	cur = buf;
	while (token = strsep(&cur, ",")) {
		value[i++] = simple_strtol(token, NULL, 0);
	}

	if (ACL_DEBUG == value[0]) { /* debug */
		acl_debug_cmd[1].payload[0] = 0xb4;
		acl_debug_cmd[1].payload[1] = 0x50;
		acl_debug_cmd[1].payload[2] = value[1];
		acl_debug_cmd[1].payload[3] = value[2];
		acl_debug_cmd[1].payload[4] = value[3];
		acl_debug_cmd[1].payload[5] = value[4];
		mipi_dsi_cmds_tx(acl_debug_cmd, ARRAY_SIZE(acl_debug_cmd), mipi_dsi0_base);
	} else if (ACL_SETTING == value[0]) {
		if (ACL_OFF == value[1]) {
			acl_switch_cmd[1].payload[0] = 0x55;
			acl_switch_cmd[1].payload[1] = ACL_OFF;
			mipi_dsi_cmds_tx(acl_switch_cmd, ARRAY_SIZE(acl_switch_cmd), mipi_dsi0_base);
			g_acl_ctrl = ACL_OFF;
			HISI_FB_INFO("ACL OFF\n");
		} else if (ACL_OFFSET1_ON == value[1]) {
			acl_switch_cmd[1].payload[0] = 0x55;
			acl_switch_cmd[1].payload[1] = ACL_OFFSET1_ON;
			mipi_dsi_cmds_tx(acl_switch_cmd, ARRAY_SIZE(acl_switch_cmd), mipi_dsi0_base);
			g_acl_ctrl = ACL_OFFSET1_ON;
			HISI_FB_INFO("ACL OFFSET 1 ON\n");
		} else if (ACL_OFFSET2_ON == value[1]) {
			acl_switch_cmd[1].payload[0] = 0x55;
			acl_switch_cmd[1].payload[1] = ACL_OFFSET2_ON;
			mipi_dsi_cmds_tx(acl_switch_cmd, ARRAY_SIZE(acl_switch_cmd), mipi_dsi0_base);
			g_acl_ctrl = ACL_OFFSET2_ON;
			HISI_FB_INFO("ACL OFFSET 2 ON\n");
		} else if (ACL_OFFSET3_ON == value[1]) {
			acl_switch_cmd[1].payload[0] = 0x55;
			acl_switch_cmd[1].payload[1] = ACL_OFFSET3_ON;
			mipi_dsi_cmds_tx(acl_switch_cmd, ARRAY_SIZE(acl_switch_cmd), mipi_dsi0_base);
			g_acl_ctrl = ACL_OFFSET3_ON;
			HISI_FB_INFO("ACL OFFSET 3 ON\n");
		} else {
			HISI_FB_INFO("invalid parm!\n");
		}
	} else {
		HISI_FB_INFO("invalid parm!\n");
	}
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);
	return count;
}

/* for esd check */
static int mipi_samsung_panel_check_esd(struct platform_device* pdev)
{
	int ret = 0, errflag_detect = 0, status_reg_detect = 0;
	struct hisi_fb_data_type* hisifd = NULL;

	uint32_t read_value[2] = {0};
	uint32_t expected_value[2] = {0xB4, 0x80};
	uint32_t read_mask[2] = {0xff, 0xff};
	char* reg_name[2] = {"power mode","RDDCOLMOD"};
	char lcd_reg_0a[] = {0x0a};
	char lcd_reg_0e[] = {0x0e};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e},
	};

	struct mipi_dsi_read_compare_data data = {
		.read_value = read_value,
		.expected_value = expected_value,
		.read_mask = read_mask,
		.reg_name = reg_name,
		.log_on = 0,
		.cmds = lcd_check_reg,
		.cnt = ARRAY_SIZE(lcd_check_reg),
	};

	struct dsi_cmd_desc pkt_size_cmd;

	memset(&pkt_size_cmd, 0, sizeof(struct dsi_cmd_desc));
	pkt_size_cmd.dtype = DTYPE_MAX_PKTSIZE;
	pkt_size_cmd.dlen = 1;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return 0;
	}

	hisifd = (struct hisi_fb_data_type*)platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return 0;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (gpio_get_value(gpio_lcd_err_flag) == 1) {
		HISI_FB_INFO("esd err_flag detect!\n");
		errflag_detect = 1;
		return errflag_detect;
	} else {
		HISI_FB_DEBUG("no err_flag detect!\n");
		errflag_detect = 0;
	}

	if (false == g_display_on) {
		HISI_FB_DEBUG("display on false right now, stop ESD detect\n");
		return 0;
	}else{
		//we need 20 ms time to ensure that read 0A normal
		mdelay(20);
	}

	mipi_dsi_max_return_packet_size(&pkt_size_cmd, hisifd->mipi_dsi0_base);
	ret = mipi_dsi_read_compare(&data, hisifd->mipi_dsi0_base);
	if (!ret) {
		status_reg_detect = 0;
	} else if (read_value[0] || read_value[1]) {
		status_reg_detect = 1;
		HISI_FB_INFO("esd 0A or 0E detect abnormal:0x0a=%02x,0x0e=%02x\n",
			read_value[0],read_value[1]);
		return status_reg_detect;
	} else {
		status_reg_detect = 0;
		HISI_FB_DEBUG("esd 0A detect 0\n");
	}

	ret = errflag_detect + status_reg_detect;
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_pcd_errflag_check(struct platform_device *pdev,
	char *buf)
{
	ssize_t ret = 0;
	u8 result_value = 0;
	int errflag_gpio = 0;

	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		ret = snprintf(buf, PAGE_SIZE, "%d\n", result_value);
		return ret;
	}

	hisifd = platform_get_drvdata(pdev);
	if (!hisifd) {
		HISI_FB_ERR("hisifd is null\n");
		ret = snprintf(buf, PAGE_SIZE, "%d\n", result_value);
		return ret;
	}

	if (!hisifd->panel_power_on) {
		HISI_FB_INFO("panel is poweroff\n");
		ret = snprintf(buf, PAGE_SIZE, "%d\n", result_value);
		return ret;
	}

	errflag_gpio = gpio_get_value(gpio_lcd_err_flag);
	HISI_FB_INFO("pcd:%u, errflag:%u\n", g_samsung_pcd_record, errflag_gpio);

	if (!g_samsung_pcd_record && !errflag_gpio) {
		result_value = 0; /* PCD_ERR_FLAG_SUCCESS */
	} else if (g_samsung_pcd_record && !errflag_gpio) {
		result_value = 1; /* only  PCD_FAIL */
	} else if (!g_samsung_pcd_record && errflag_gpio) {
		result_value = 2; /* only ERRFLAG FAIL */
	} else if (g_samsung_pcd_record && errflag_gpio) {
		result_value = 3; /* PCD_ERR_FLAG_FAIL */
	} else {
		result_value = 0;
	}

	g_samsung_pcd_record = 0;

	ret = snprintf(buf, PAGE_SIZE, "%d\n", result_value);
	return ret;
}

static ssize_t mipi_samsung_panel_alpm_setting(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;
	unsigned int cmd = 0;
	char alpm_bl_level_set[] = {0x51,0x00,};

	struct dsi_cmd_desc alpm_bl_level_set_cmd[] = {
		{DTYPE_DCS_WRITE1, 0, 500, WAIT_TYPE_US,
			sizeof(alpm_bl_level_set), alpm_bl_level_set},
	};

	if(pdev == NULL){
		HISI_FB_ERR("pdev is null\n");
		return -1;
	}

	hisifd = platform_get_drvdata(pdev);
	if(hisifd == NULL){
		HISI_FB_ERR("hisifd is null\n");
		return -1;
	}

	if(!hisifd->aod_function) {
		HISI_FB_ERR("aod function is not open\n");
		return 0;
	}

	ret = sscanf(buf, "%u", &cmd);
	if (!ret) {
		HISI_FB_ERR("sscanf return invaild:%d\n", ret);
		return ret;
	}

	HISI_FB_INFO("AOD cmd is %d\n", cmd);
	switch (cmd) {
	case ALPM_DISPLAY_OFF_CMD:
		alpm_bl_level_set[1] = 0x02;
		HISI_FB_INFO("set min brightness:%d for(AOD mode) and display off.\n", alpm_bl_level_set[1]);
		mipi_dsi_cmds_tx(alpm_bl_level_set_cmd, ARRAY_SIZE(alpm_bl_level_set_cmd), hisifd->mipi_dsi0_base);
		mipi_dsi_cmds_tx(display_off_cmd, ARRAY_SIZE(display_off_cmd), hisifd->mipi_dsi0_base);
		mdelay(17);
		panel_alpm_on = true;
		break;
	case ALPM_ON_50NIT_CMD:
		mdelay(17);
		HISI_FB_INFO("enter ALPM mode(50nit) and set cmd display_cmd(0x29)!\n");
		/* ALPM display mode 50nit setting */
		mipi_dsi_cmds_tx(alpm_50nit_on_cmds, ARRAY_SIZE(alpm_50nit_on_cmds), hisifd->mipi_dsi0_base);
		mipi_dsi_cmds_tx(display_on_cmd, ARRAY_SIZE(display_on_cmd), hisifd->mipi_dsi0_base);
		panel_alpm_on = true;
		break;
	case ALPM_ON_10NIT_CMD:
		mdelay(17);
		/* ALPM display mode 10nit setting */
		HISI_FB_INFO("enter ALPM mode(10nit) and set cmd display_cmd(0x29)!\n");
		mipi_dsi_cmds_tx(alpm_10nit_on_cmds, ARRAY_SIZE(alpm_10nit_on_cmds), hisifd->mipi_dsi0_base);
		mipi_dsi_cmds_tx(display_on_cmd, ARRAY_SIZE(display_on_cmd), hisifd->mipi_dsi0_base);
		panel_alpm_on = true;
		break;
	case ALPM_OFF_CMD:
		mdelay(17);
		HISI_FB_INFO("exit ALPM mode and set cmd display_cmd(0x29)!\n");
		/* normal display mode setting */
		mipi_dsi_cmds_tx(alpm_off_cmds, ARRAY_SIZE(alpm_off_cmds), hisifd->mipi_dsi0_base);
		mipi_dsi_cmds_tx(display_on_cmds, ARRAY_SIZE(display_on_cmds), hisifd->mipi_dsi0_base);
		mipi_dsi_cmds_tx(display_on_cmd, ARRAY_SIZE(display_on_cmd), hisifd->mipi_dsi0_base);
		alpm_bl_level_set[1] = 0x66;
		HISI_FB_INFO("set brightness:%d for normal mode.\n", alpm_bl_level_set[1]);
		mipi_dsi_cmds_tx(alpm_bl_level_set_cmd, ARRAY_SIZE(alpm_bl_level_set_cmd), hisifd->mipi_dsi0_base);
		panel_alpm_on = false;
		break;
	default:
		break;
	}
	return count;
}

static int g_support_mode = 0;
static ssize_t mipi_samsung_panel_support_mode_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return ret;
	}

	hisifd = platform_get_drvdata(pdev);
	if (!hisifd) {
		HISI_FB_ERR("hisifd is null\n");
		return ret;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "%d\n", g_support_mode);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_support_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag = -1;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return ret;
	}

	hisifd = platform_get_drvdata(pdev);
	if (!hisifd) {
		HISI_FB_ERR("hisifd is null\n");
		return ret;
	}

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
		return ret;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	flag = (int)val;

	g_support_mode = flag;

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return snprintf((char *)buf, count, "%d\n", g_support_mode);
}


/*because of  architecture limitation,  just use this sys node to control acm and gmp */
static ssize_t mipi_samsung_panel_lcd_ic_color_enhancement_mode_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;

	if (pdev == NULL) {
		return -1;
	}

	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return -1;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "%d\n", g_color_enhancement_mode);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_lcd_ic_color_enhancement_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag=-1;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	char __iomem *dpp_base = NULL;
	char __iomem *acm_base = NULL;

	if (pdev == NULL) {
		return -1;
	}

	hisifd = platform_get_drvdata(pdev);
	if (hisifd == NULL) {
		return -1;
	}

	mipi_dsi0_base =hisifd->mipi_dsi0_base;
	dpp_base = hisifd->dss_base + DSS_DPP_OFFSET;
	acm_base = hisifd->dss_base + DSS_DPP_ACM_OFFSET;

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
		return ret;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	flag=(int)val;

	HISI_FB_INFO("fb%d, set val = %d", hisifd->index, flag);

	/*this project: 0 means using sRGB, 1 means using color_enhance*/
	if (flag == COLOR_EN_DEFAULT)
	{
		/*open acm*/
		set_reg(acm_base + ACM_EN, 0x1, 1, 0);

		/*close gmp*/
		g_color_enhancement_mode = COLOR_EN_DEFAULT;
		set_reg(dpp_base + LCP_GMP_BYPASS_EN, 0x1, 1, 0);
	}
	else if (flag == COLOR_EN_SRGB)
	{
		/*close acm*/
		set_reg(acm_base + ACM_EN, 0x0, 1, 0);

		/*open gmp*/
		g_color_enhancement_mode = COLOR_EN_SRGB;
		set_reg(dpp_base + LCP_GMP_BYPASS_EN, 0x0, 1, 0);
	}
	else if (flag == COLOR_EN_CLOSED)
	{
		/*close acm*/
		set_reg(acm_base + ACM_EN, 0x0, 1, 0);

		/*close gmp*/
		g_color_enhancement_mode = COLOR_EN_CLOSED;
		set_reg(dpp_base + LCP_GMP_BYPASS_EN, 0x1, 1, 0);
	}
	else
	{
		HISI_FB_DEBUG(" no this color_enhancement mode!\n");
	};

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return snprintf((char *)buf, count, "%d\n", g_color_enhancement_mode);
}

#define LCD_CMD_NAME_MAX 100
static char lcd_cmd_now[LCD_CMD_NAME_MAX] = {0};
static ssize_t mipi_samsung_panel_test_config_show(struct platform_device *pdev,
	char *buf)
{
	HISI_FB_INFO("%s\n", lcd_cmd_now);

	if (!strncmp(lcd_cmd_now, "PCD", strlen("PCD"))) { /* PCD */
		return snprintf(buf, PAGE_SIZE, "/sys/class/graphics/fb0/amoled_pcd_errflag_check");
	} else if (!strncmp(lcd_cmd_now, "ERRORFLAG", strlen("ERRORFLAG"))) { /* ERRORFLAG */
		return snprintf(buf, PAGE_SIZE, "/sys/class/graphics/fb0/amoled_pcd_errflag_check");
	}else {
		return snprintf(buf, PAGE_SIZE, "INVALID");
	}

}

static ssize_t mipi_samsung_panel_test_config_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		memcpy(lcd_cmd_now, "INVALID", strlen("INVALID") + 1);
		HISI_FB_INFO("invalid test cmd:%s\n", lcd_cmd_now);
		return count;
	}

	if (strlen(buf) < LCD_CMD_NAME_MAX) {
		memcpy(lcd_cmd_now, buf, strlen(buf) + 1);
		HISI_FB_INFO("current test cmd:%s\n", lcd_cmd_now);
	} else {
		memcpy(lcd_cmd_now, "INVALID", strlen("INVALID") + 1);
		HISI_FB_INFO("invalid test cmd:%s\n", lcd_cmd_now);
	}

	return count;
}

static ssize_t mipi_samsung_panel_sleep_ctrl_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	ret = snprintf(buf, PAGE_SIZE, "enable_lcd_sleep_in=%d, pinfo->lcd_adjust_support=%d\n",
		g_debug_enable_lcd_sleep_in, hisifd->panel_info.lcd_adjust_support);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_sleep_ctrl_store(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	unsigned long val = 0;
	struct hisi_fb_data_type *hisifd = NULL;

	ret = strict_strtoul(buf, 0, &val);
	if (ret) {
		HISI_FB_ERR("strict_strtoul error, buf=%s", buf);
		return ret;
	}

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer!\n");
		return 0;
	}

	if (hisifd->panel_info.lcd_adjust_support) {
		g_debug_enable_lcd_sleep_in = val;
	}

	if (g_debug_enable_lcd_sleep_in == 2) {
		HISI_FB_INFO("LCD power off and Touch goto sleep\n");
		lcd_rs_poweroff = true;
		/* used for pt  current test, tp sleep */
		g_tp_power_ctrl = 1;
	} else {
		HISI_FB_INFO("g_debug_enable_lcd_sleep_in is %d\n", g_debug_enable_lcd_sleep_in);
		lcd_rs_poweroff = false;
		/* used for pt  current test, tp power off */
		g_tp_power_ctrl = 0;
	}

	return ret;
}
static struct hisi_panel_info g_panel_info = {0};
static struct hisi_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_samsung_panel_set_fastboot,
	.on = mipi_samsung_panel_on,
	.off = mipi_samsung_panel_off,
	.remove = mipi_samsung_panel_remove,
	.set_backlight = mipi_samsung_panel_set_backlight,
	.lcd_check_reg = mipi_samsung_panel_check_reg_show,
	.lcd_model_show = mipi_samsung_panel_model_show,
	.panel_info_show = mipi_samsung_panel_info_show,
	.lcd_hbm_ctrl_show = mipi_samsung_panel_hbm_ctrl_show,
	.lcd_hbm_ctrl_store = mipi_samsung_panel_hbm_ctrl_store,
	.lcd_sleep_ctrl_show = mipi_samsung_panel_sleep_ctrl_show,
	.lcd_sleep_ctrl_store = mipi_samsung_panel_sleep_ctrl_store,
	.lcd_acl_ctrl_show = mipi_samsung_panel_acl_ctrl_show,
	.lcd_acl_ctrl_store = mipi_samsung_panel_acl_ctrl_store,
	.amoled_alpm_setting_store = mipi_samsung_panel_alpm_setting,
	.esd_handle = mipi_samsung_panel_check_esd,
	.amoled_pcd_errflag_check = mipi_samsung_pcd_errflag_check,
	.lcd_test_config_show = mipi_samsung_panel_test_config_show,
	.lcd_test_config_store = mipi_samsung_panel_test_config_store,
	.set_display_region = mipi_samsung_panel_set_display_region,
	.lcd_support_mode_show = mipi_samsung_panel_support_mode_show,
	.lcd_support_mode_store = mipi_samsung_panel_support_mode_store,
	.lcd_ic_color_enhancement_mode_store = mipi_samsung_panel_lcd_ic_color_enhancement_mode_store,
	.lcd_ic_color_enhancement_mode_show = mipi_samsung_panel_lcd_ic_color_enhancement_mode_show,
};

/*******************************************************************************
**
*/
static int mipi_samsung_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_panel_info *pinfo = NULL;
	struct device_node *np = NULL;
	uint32_t bl_type = 0;
	uint32_t lcd_display_type = 0;
	int support_mode = 0;
	uint32_t use_ldo17 = 0;

	/* not use fb_notify to control touch timing. */
	g_lcd_control_tp_power = true;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_SAMSUNG_S6E3HA3X02);
	if (!np) {
		HISI_FB_ERR("NOT FOUND device node %s!\n", DTS_COMP_SAMSUNG_S6E3HA3X02);
		goto err_return;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, &lcd_display_type);
	if (ret) {
		HISI_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_DUAL_MIPI_CMD;
	}

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &bl_type);
	if (ret) {
		HISI_FB_ERR("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	HISI_FB_INFO("bl_type=0x%x.", bl_type);

	if (hisi_fb_device_probe_defer(lcd_display_type, bl_type)) {
		goto err_probe_defer;
	}

	ret = of_property_read_u32(np, USE_LDO17, &use_ldo17);
	if (ret) {
		HISI_FB_ERR("get lcd_bl_type failed!\n");
		use_ldo17 = USE_LDO17_DISABLE;
	}
	HISI_FB_INFO("use_ldo17=%d.", use_ldo17);

	HISI_FB_INFO("%s\n", DTS_COMP_SAMSUNG_S6E3HA3X02);

	gpio_lcd_1v8_en = of_get_named_gpio(np, "gpios", 0);    /*gpio_125*/
	gpio_lcd_reset = of_get_named_gpio(np, "gpios", 1);     /*gpio_040*/
	gpio_lcd_id0 = of_get_named_gpio(np, "gpios", 2);       /*gpio_046*/
	gpio_lcd_pcd = of_get_named_gpio(np, "gpios", 3);       /*gpio_181*/
	gpio_lcd_err_flag = of_get_named_gpio(np, "gpios", 4);  /*gpio_182*/
	HISI_FB_INFO("gpio_lcd_1v8_en = %d, gpio_lcd_reset = %d, gpio_lcd_id0 = %d, gpio_lcd_pcd = %d, gpio_lcd_err_flag = %d\n", \
		gpio_lcd_1v8_en, gpio_lcd_reset, gpio_lcd_id0, gpio_lcd_pcd, gpio_lcd_err_flag);

	pdev->id = 1;
	/* init panel info */
	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct hisi_panel_info));
	pinfo->xres = 1440;
	pinfo->yres = 2560;
	pinfo->width = 68;
	pinfo->height = 122;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;

	pinfo->bl_min = 4;
	pinfo->bl_max = 255;
	pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;

	pinfo->type = lcd_display_type;

	pinfo->bl_default = 102;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 1;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->lcd_adjust_support = 1;

	pinfo->dirty_region_updt_support = 0;
	pinfo->dirty_region_info.left_align = -1;
	pinfo->dirty_region_info.right_align = -1;
	pinfo->dirty_region_info.top_align = -1;
	pinfo->dirty_region_info.bottom_align = -1;
	pinfo->dirty_region_info.w_align = -1;
	pinfo->dirty_region_info.h_align = -1;
	pinfo->dirty_region_info.w_min = 1440;
	pinfo->dirty_region_info.h_min = 11;
	pinfo->dirty_region_info.top_start = -1;
	pinfo->dirty_region_info.bottom_start = -1;

	/* The host processor must wait for more than 15us from the end of write data transfer to a command 2Ah/2Bh */
	if (pinfo->dirty_region_updt_support == 1)
		pinfo->mipi.hs_wr_to_time = 17000;        /* measured in nS */

	/* prefix ce & sharpness */
	pinfo->prefix_ce_support = 0;
	pinfo->prefix_sharpness1D_support = 0;
	pinfo->prefix_sharpness2D_support = 0;

	/* sbl */
	pinfo->sbl_support = 0;
	pinfo->smart_bl.strength_limit = strength_limit;
	pinfo->smart_bl.calibration_a = calibration_a;
	pinfo->smart_bl.calibration_b = calibration_b;
	pinfo->smart_bl.calibration_c = calibration_c;
	pinfo->smart_bl.calibration_d = calibration_d;
	pinfo->smart_bl.t_filter_control = t_filter_control;
	pinfo->smart_bl.backlight_min = backlight_min;
	pinfo->smart_bl.backlight_max = backlight_max;
	pinfo->smart_bl.backlight_scale = backlight_scale;
	pinfo->smart_bl.ambient_light_min = ambient_light_min;
	pinfo->smart_bl.filter_a = filter_a;
	pinfo->smart_bl.filter_b = filter_b;
	pinfo->smart_bl.logo_left = logo_left;
	pinfo->smart_bl.logo_top = logo_top;
	pinfo->smart_bl.variance_intensity_space = variance_intensity_space;
	pinfo->smart_bl.slope_max = slope_max;
	pinfo->smart_bl.slope_min = slope_min;

	/* ACM */
	pinfo->acm_support = 0;
	if (pinfo->acm_support == 1) {
		pinfo->acm_lut_hue_table = acm_lut_hue_table;
		pinfo->acm_lut_hue_table_len = ARRAY_SIZE(acm_lut_hue_table);
		pinfo->acm_lut_sata_table = acm_lut_sata_table;
		pinfo->acm_lut_sata_table_len = ARRAY_SIZE(acm_lut_sata_table);
		pinfo->acm_lut_satr_table = acm_lut_satr_table;
		pinfo->acm_lut_satr_table_len = ARRAY_SIZE(acm_lut_satr_table);
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

		/*for cinema mode */
		pinfo->cinema_acm_valid_num = 7;
		pinfo->cinema_r0_hh = 0x7f;
		pinfo->cinema_r0_lh = 0x0;
		pinfo->cinema_r1_hh = 0xff;
		pinfo->cinema_r1_lh = 0x80;
		pinfo->cinema_r2_hh = 0x17f;
		pinfo->cinema_r2_lh = 0x100;
		pinfo->cinema_r3_hh = 0x1ff;
		pinfo->cinema_r3_lh = 0x180;
		pinfo->cinema_r4_hh = 0x27f;
		pinfo->cinema_r4_lh = 0x200;
		pinfo->cinema_r5_hh = 0x2ff;
		pinfo->cinema_r5_lh = 0x280;
		pinfo->cinema_r6_hh = 0x37f;
		pinfo->cinema_r6_lh = 0x300;

		/* ACM_CE */
		pinfo->acm_ce_support = 1;
	}

	/* Contrast Algorithm */
	if (pinfo->prefix_ce_support == 1 || pinfo->acm_ce_support == 1) {
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
	pinfo->gamma_support = 0;
	if (pinfo->gamma_support == 1) {
		pinfo->gamma_lut_table_R = gamma_lut_table_R;
		pinfo->gamma_lut_table_G = gamma_lut_table_G;
		pinfo->gamma_lut_table_B = gamma_lut_table_B;
		pinfo->gamma_lut_table_len = ARRAY_SIZE(gamma_lut_table_R);
		pinfo->igm_lut_table_R = igm_lut_table_R;
		pinfo->igm_lut_table_G = igm_lut_table_G;
		pinfo->igm_lut_table_B = igm_lut_table_B;
		pinfo->igm_lut_table_len = ARRAY_SIZE(igm_lut_table_R);

		pinfo->gmp_support = 1;
		pinfo->gmp_lut_table_low32bit = &gmp_lut_table_low32bit[0][0][0];
		pinfo->gmp_lut_table_high4bit = &gmp_lut_table_high4bit[0][0][0];
		pinfo->gmp_lut_table_len = ARRAY_SIZE(gmp_lut_table_low32bit);

		pinfo->xcc_support = 1;
		pinfo->xcc_table = xcc_table;
		pinfo->xcc_table_len = ARRAY_SIZE(xcc_table);
		pinfo->color_temperature_support = 0;
		pinfo->comform_mode_support = 0;
	}

	if (pinfo->comform_mode_support == 1) {
		support_mode = (support_mode | COMFORM_MODE);
	}
	if(pinfo->cinema_mode_support == 1) {
		support_mode = (support_mode | CINEMA_MODE);
	}
	g_support_mode = support_mode;

	/* ldi */
	pinfo->ldi.h_back_porch = 96;
	pinfo->ldi.h_front_porch = 108;
	pinfo->ldi.h_pulse_width = 48;
	pinfo->ldi.v_back_porch = 35;
	pinfo->ldi.v_front_porch = 14;
	pinfo->ldi.v_pulse_width = 8;

	/* mipi */
	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.non_continue_en = 1;
	pinfo->mipi.burst_mode = 0;

	pinfo->mipi.data_t_hs_trial_adjust = 8;
	pinfo->mipi.data_t_hs_zero_adjust = 40;
	pinfo->mipi.clk_t_lpx_adjust = -68;
	pinfo->mipi.clk_post_adjust= 150;
	pinfo->mipi.clk_t_hs_zero_adjust = 40;

	pinfo->mipi.dsi_bit_clk = 472;
	pinfo->mipi.dsi_bit_clk_val1 = 444;
	pinfo->mipi.dsi_bit_clk_val2 = 459;
	pinfo->mipi.dsi_bit_clk_val3 = 472;
	pinfo->dsi_bit_clk_upt_support = 1;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	pinfo->pxl_clk_rate = 288*1000000UL;

	/* is dual_mipi or single_mipi */
	if (is_dual_mipi_panel_ext(pinfo))  {
		pinfo->ifbc_type = IFBC_TYPE_NONE;
		pinfo->pxl_clk_rate_div = 2;
		HISI_FB_INFO("PANEL_DUAL_MIPI_CMD: 8 lane \n");
	} else {
	}

	if (pinfo->pxl_clk_rate_div > 1) {
		pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;
	}

	if(runmode_is_factory()) {
		HISI_FB_INFO("Factory mode, disable features: dirty update etc.\n");
		pinfo->dirty_region_updt_support = 0;
		pinfo->prefix_ce_support = 0;
		pinfo->prefix_sharpness1D_support = 0;
		pinfo->prefix_sharpness2D_support = 0;
		pinfo->sbl_support = 0;
		pinfo->acm_support = 0;
		pinfo->acm_ce_support = 0;
		pinfo->esd_enable = 0;
		pinfo->blpwm_input_ena = 0;
		pinfo->blpwm_precision_type = BLPWM_PRECISION_DEFAULT_TYPE;
		pinfo->bl_min = 4;
		pinfo->bl_max = 255;
		pinfo->comform_mode_support = 0;
		/*in factory mode , use default*/
		g_support_mode = 0;
		g_acl_ctrl = 0;
	}

	/* lcd vcc init */
	if (USE_LDO17_ENABLE == use_ldo17) {
		ret = vcc_cmds_tx(pdev, lcd_ldo17_init_cmds,
			ARRAY_SIZE(lcd_ldo17_init_cmds));
	} else {
		ret = vcc_cmds_tx(pdev, lcd_vcc_init_cmds,
			ARRAY_SIZE(lcd_vcc_init_cmds));
	}

	if (ret != 0) {
		HISI_FB_ERR("LCD vcc init failed!\n");
		goto err_return;
	}

	/* lcd pinctrl init */
	ret = pinctrl_cmds_tx(pdev, lcd_pinctrl_init_cmds,
		ARRAY_SIZE(lcd_pinctrl_init_cmds));
	if (ret != 0) {
		HISI_FB_ERR("Init pinctrl failed, defer\n");
		goto err_return;
	}

	/* lcd vcc enable */
	if (is_fastboot_display_enable()) {
		vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
			ARRAY_SIZE(lcd_vcc_enable_cmds));
	}

#ifdef AMOLED_CHECK_INT
	ret = request_threaded_irq(gpio_to_irq(gpio_lcd_pcd), NULL, pcd_irq_isr_func,
			IRQF_ONESHOT | IRQF_TRIGGER_RISING,
			"pcd_irq", (void *)pdev);
	if (ret != 0) {
		HISI_FB_ERR("request_irq failed, irq_no=%d error=%d!\n", gpio_to_irq(gpio_lcd_pcd), ret);
	}

	ret = request_threaded_irq(gpio_to_irq(gpio_lcd_err_flag), NULL, errflag_irq_isr_func,
			IRQF_ONESHOT | IRQF_TRIGGER_RISING,
			"errflag_irq", (void *)pdev);
	if (ret != 0) {
		HISI_FB_ERR("request_irq failed, irq_no=%d error=%d!\n", gpio_to_irq(gpio_lcd_err_flag), ret);
	}
#endif

	/* alloc panel device data */
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
		.compatible = DTS_COMP_SAMSUNG_S6E3HA3X02,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_samsung_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_samsung_S6E3HA3X02_5P5_AMOLED",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_panel_match_table),
	},
};

static int __init mipi_samsung_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_samsung_panel_init);
