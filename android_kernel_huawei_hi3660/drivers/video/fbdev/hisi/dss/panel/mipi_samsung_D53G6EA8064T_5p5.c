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
#include "display_effect_samsung_D53G6EA8064T_5p5.h"

#define DTS_COMP_SAMSUNG_D53G6EA8064T "hisilicon,mipi_samsung_D53G6EA8064T_5P5_AMOLED"
#define DRIVER_NAME "SAMSUNG_D53G6EA8064T"
#define USE_LDO17 "use_ldo17"

#define	DISPLAY_NORMAL_MODE	0
#define	DISPLAY_FRESH_MODE	1
#define	DISPLAY_UNKNOWN_MODE	9
#define	DISPLAY_NORMAL_MODE_VALUE	0x40
#define	DISPLAY_FRESH_MODE_VALUE	0xa4

#define USE_LDO17_ENABLE 1
#define USE_LDO17_DISABLE 0

static struct hisi_fb_panel_data g_panel_data;
static bool g_debug_enable = false;
static bool lcd_rs_poweroff = false;
static int g_acl_mode = 0;
static unsigned int g_ic_color_enhancement_local = DISPLAY_NORMAL_MODE;

static bool g_lp2hs_mipi_test = false;
static bool g_lp2hs_mipi_test_result = false;
#define AMOLED_CHECK_INT

#define TP_RS_CALL 1
extern bool g_lcd_control_tp_power;

/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/
/* common setting */
static char tear_on[] = {
	0x35,
	0x00,
};

static char delay_te[] = {
	0x44,
	0x00, 0x00,
};
/* brightness contrl */
static char bl_enable[] = {
	0x53,
	0x20,
};

static char seed_disable[] = {
	0x57,
	0x40,
};

static char bl_setting[]= {
	0x51,
	0x00,
};

static char acl_mode[] = {
	0x55,
	0x00,
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

static char lock_setting[] = {
	0xF0,
	0xA5, 0xA5,
};

static char unlock_setting[] = {
	0xF0,
	0x5A, 0x5A,
};

static char test_key_enable[] = {
	0xF0,
	0x5A, 0x5A,
};

static char global_para1[] = {
	0xB0,
	0x03,
};

static char aid_cycle_1[] = {
	0xB2,
	0x00,
};

static char aid_cycle_4[] = {
	0xB2,
	0x20,
};

static char global_para2[] = {
	0xB0,
	0x01,
};

static char aid_duty_70[] = {
	0xB2,
	0x05, 0x4B,
};

static char aid_duty_off[] = {
	0xB2,
	0x00, 0x10,
};

static char ltps_update[] = {
	0xF7,
	0x03,
};

static char test_key_disable[] = {
	0xF0,
	0xA5, 0xA5,
};


static char pwd_open_unlock[] = {
	0xF0,
	0x5A, 0x5A,
};

static char edge_enhancement_level_setting[] = {
	0xED,
	0xBE,
};

static char gp_cr[] = {
	0xB0,
	0x06,
};

static char cr_level_setting[] = {
	0xED,
	0x70,
};

static char gp_srgb[] = {
	0xB0,
	0x08,
};

static char srgb_lut_setting[] = {
	0xED,
	0xA0, 0x02, 0x04, 0x40, 0xD3,
	0x13, 0x05, 0x08, 0xB4, 0x4C,
	0xEE, 0xCD, 0xB7, 0x0A, 0xBE,
	0xDD, 0xE1, 0x18, 0xFF, 0xFF,
	0xFF,
};

static char gp_adobe[] = {
	0xB0,
	0x1D,
};

static char adobe_lut_setting[] = {
	0xED,
	0xD9, 0x03, 0x07, 0x00, 0xC7,
	0x06, 0x05, 0x08, 0xBB, 0x00,
	0xE4, 0xC4, 0xF4, 0x0C, 0xCB,
	0xDC, 0xDF, 0x0E, 0xFF, 0xFF,
	0xFF,
};

static char gp_user[] = {
	0xB0,
	0x32,
};

static char user_lut_setting[] = {
	0xED,
	0xDE, 0x00, 0x07, 0x1B, 0xFF,
	0x01, 0x01, 0x00, 0xEA, 0x09,
	0xFA, 0xFF, 0xC1, 0x00, 0xCF,
	0xF2, 0xFE, 0x01, 0xFF, 0xFF,
	0xFF,
};

static char pwd_close_lock[] = {
	0xF0,
	0xA5, 0xA5,
};

static char mode_selection_sRGB_select[] = {
	0x57,
	0x4C,
};

static char mode_selection_Adobe_RGB_select[] = {
	0x57,
	0x44,
};

static char mode_selection_User_Mode_select[] = {
	0x57,
	0xA4,
};

static char mode_selection_Original_Mode[] = {
	0x57,
	0x40,
};

static char mode_selection_read[] = {
	0x58,
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

/* for lp2hs_mipi test */
static char lp2hs_mipi[] = {
	0x36,
	0x08,
};

static struct dsi_cmd_desc samsung_lp2hs_mipi_cmds[] = {
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
};

static struct dsi_cmd_desc dsp_normal_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(mode_selection_User_Mode_select), mode_selection_User_Mode_select},
};

static struct dsi_cmd_desc dsp_fresh_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(mode_selection_Original_Mode), mode_selection_Original_Mode},
};



static struct dsi_cmd_desc display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(tear_on), tear_on},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(delay_te), delay_te},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(bl_setting), bl_setting},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(acl_mode), acl_mode},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE, 0, 25, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},

	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(pwd_open_unlock), pwd_open_unlock},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(edge_enhancement_level_setting), edge_enhancement_level_setting},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(gp_cr), gp_cr},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(cr_level_setting), cr_level_setting},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(gp_srgb), gp_srgb},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(srgb_lut_setting), srgb_lut_setting},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(gp_adobe), gp_adobe},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(adobe_lut_setting), adobe_lut_setting},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(gp_user), gp_user},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(user_lut_setting), user_lut_setting},
	{DTYPE_DCS_LWRITE, 0,85, WAIT_TYPE_MS,
		sizeof(pwd_close_lock), pwd_close_lock},
	{DTYPE_DCS_WRITE, 0, 60, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
};

static struct dsi_cmd_desc display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 40, WAIT_TYPE_MS,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 160, WAIT_TYPE_MS,
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
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
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
	int error = 0;
	static struct lcd_reg_read_t lcd_status_reg[] = {
		{0x0A, 0x9C, 0xFF, "lcd power state"},
	};
	u32 lp2hs_mipi_check_read_value[1] = {0};
	u32 lp2hs_mipi_check_expected_value[1] = {0x20};
	u32 lp2hs_mipi_check_read_mask[1] = {0xFF};
	char* lp2hs_mipi_check_reg_name[1] = {"power mode"};
	char lp2hs_mipi_check_lcd_reg_36[] = {0x54};
	u32 retry=0;

	struct dsi_cmd_desc lp2hs_mipi_check_lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lp2hs_mipi_check_lcd_reg_36), lp2hs_mipi_check_lcd_reg_36},
	};

	struct mipi_dsi_read_compare_data g_lp2hs_mipi_check_data = {
		.read_value = lp2hs_mipi_check_read_value,
		.expected_value = lp2hs_mipi_check_expected_value,
		.read_mask = lp2hs_mipi_check_read_mask,
		.reg_name = lp2hs_mipi_check_reg_name,
		.log_on = 1,
		.cmds = lp2hs_mipi_check_lcd_check_reg,
		.cnt = ARRAY_SIZE(lp2hs_mipi_check_lcd_check_reg),
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

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		if (!lcd_rs_poweroff) {
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
		HISI_FB_INFO("Init lcd_init_step is lp send mode.\n");

		if (TP_RS_CALL != g_debug_enable_lcd_sleep_in) {
			HISI_FB_INFO("TP resume and after resume\n");
			error = ts_power_control_notify(TS_RESUME_DEVICE, NO_SYNC_TIMEOUT);
			error = ts_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);
		}
		/* lcd gpio normal */
		mdelay(10);
		gpio_cmds_tx(lcd_gpio_reset_cmds, \
			ARRAY_SIZE(lcd_gpio_reset_cmds));

		mipi_dsi_cmds_tx(display_on_cmds, \
			ARRAY_SIZE(display_on_cmds), mipi_dsi0_base);

		if(runmode_is_factory()) {
			HISI_FB_INFO("factory mode, do nothing\n");
		}else{
			if ( DISPLAY_FRESH_MODE == g_ic_color_enhancement_local){
				HISI_FB_INFO("reflash fresh mode\n");
				mipi_dsi_cmds_tx(dsp_fresh_cmds, \
					ARRAY_SIZE(dsp_fresh_cmds), mipi_dsi0_base);
			}else if (DISPLAY_NORMAL_MODE == g_ic_color_enhancement_local){
				HISI_FB_INFO("reflash normal mode\n");
				mipi_dsi_cmds_tx(dsp_normal_cmds, \
					ARRAY_SIZE(dsp_normal_cmds), mipi_dsi0_base);
			}else{
				HISI_FB_INFO("reflash unknown mode, g_ic_color_enhancement_local:%u\n",
					g_ic_color_enhancement_local);
			}

		}

		panel_check_status_and_report_by_dsm(lcd_status_reg, \
			ARRAY_SIZE(lcd_status_reg), mipi_dsi0_base);
		g_debug_enable = true;
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		if (g_lp2hs_mipi_test) {
			HISI_FB_INFO("downloading code for lp2hs check\n");
			mipi_dsi_cmds_tx(samsung_lp2hs_mipi_cmds, ARRAY_SIZE(samsung_lp2hs_mipi_cmds), mipi_dsi0_base);
tryagain:
			mdelay(20);
			if (!mipi_dsi_read_compare(&g_lp2hs_mipi_check_data, mipi_dsi0_base)) {
				HISI_FB_INFO("lp2hs test OK\n");
				g_lp2hs_mipi_test_result = true;
			} else {
				if (!lp2hs_mipi_check_read_value[0] && retry++ < 6){
					goto tryagain;
				}
				if (!lp2hs_mipi_check_read_value[0]) {
					HISI_FB_INFO("lp2hs test2 OK\n");
					g_lp2hs_mipi_test_result = true;
				}else{
					HISI_FB_INFO("lp2hs test fail\n");
					g_lp2hs_mipi_test_result = false;
				}
			}
		}
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

		/* lcd display off sequence */
		mipi_dsi_cmds_tx(display_off_cmds, \
			ARRAY_SIZE(display_off_cmds), hisifd->mipi_dsi0_base);

		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (!hisifd->fb_shutdown) {
			if (!lcd_rs_poweroff) {
				HISI_FB_INFO("display off(regulator disabling).\n");
				msleep(1);
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
			} else {
				HISI_FB_INFO("display off(regulator not need disabling).\n");
				msleep(1);
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
				/*if g_debug_enable_lcd_sleep_in == 1, it means don't turn off TP/LCD power
				but only let lcd get into sleep.*/
				if (TP_RS_CALL != g_debug_enable_lcd_sleep_in) {
					HISI_FB_INFO("TP before suspend and suspend\n");
					error = ts_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);
					error = ts_power_control_notify(TS_SUSPEND_DEVICE, SHORT_SYNC_TIMEOUT);
				}
		} else {
			HISI_FB_INFO("display shutting down(regulator disabling).\n");
			HISI_FB_INFO("display off(regulator disabling).\n");
			msleep(1);
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
	uint32_t expected_value[6] = {0x9C, 0x00, 0x00, 0x00, 0x80, 0xC0};
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

	ret = snprintf(buf, PAGE_SIZE, "samsung_D53G6EA8064T 5.5' CMD AMOLED\n");

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

static ssize_t g_d53g6ea8064t_hbm_ctrl=0;
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

	ret = snprintf(buf, PAGE_SIZE, "%ld\n", g_d53g6ea8064t_hbm_ctrl);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_hbm_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	static char hbm_enable[] = {
		0x53,
		0xe8,
	};
	struct dsi_cmd_desc hbm_enable_cmds[] = {
		{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
			sizeof(hbm_enable), hbm_enable},
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

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
		return ret;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	g_d53g6ea8064t_hbm_ctrl = (int)val;
	hbm_enable[1] = g_d53g6ea8064t_hbm_ctrl? 0xe8:0x20;

	down(&hisifd->blank_sem);
	if (!hisifd->panel_power_on){
		g_d53g6ea8064t_hbm_ctrl = 0;
		HISI_FB_DEBUG("panel off, power on first\n");
		goto hbm_set_err;
	}
	hisifb_set_vsync_activate_state(hisifd, true);
	hisifb_activate_vsync(hisifd);
	mipi_dsi_cmds_tx(hbm_enable_cmds, \
		ARRAY_SIZE(hbm_enable_cmds), hisifd->mipi_dsi0_base);
	hisifb_set_vsync_activate_state(hisifd, false);
	hisifb_deactivate_vsync(hisifd);

hbm_set_err:
	up(&hisifd->blank_sem);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return snprintf((char *)buf, count, "%ld\n", g_d53g6ea8064t_hbm_ctrl);
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

	ret = snprintf(buf, PAGE_SIZE, "ACL mode:%d\n", g_acl_mode);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

#define ACL_DEBUG			0
#define ACL_SETTING			1
#define ACL_OFF				0
#define ACL_OFFSET1_ON		1
#define ACL_OFFSET2_ON		2
#define ACL_OFFSET3_ON		3
static ssize_t mipi_samsung_panel_acl_ctrl_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	unsigned int value[100];
	char *token, *cur;
	int i = 0;
	char payload_acl_para[6] = {0};
	struct dsi_cmd_desc acl_debug_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(unlock_setting), unlock_setting},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(payload_acl_para), payload_acl_para},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(lock_setting), lock_setting},
	};
	char payload_acl_switch[2] = {0};
	struct dsi_cmd_desc acl_switch_cmd[] = {
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(unlock_setting), unlock_setting},
		{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
			sizeof(payload_acl_switch), payload_acl_switch},
		{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
			sizeof(lock_setting), lock_setting},
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
		acl_debug_cmd[1].payload[0] = 0xb5;
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
			g_acl_mode = ACL_OFF;
			HISI_FB_INFO("ACL OFF\n");
		} else if (ACL_OFFSET1_ON == value[1]) {
			acl_switch_cmd[1].payload[0] = 0x55;
			acl_switch_cmd[1].payload[1] = ACL_OFFSET1_ON;
			mipi_dsi_cmds_tx(acl_switch_cmd, ARRAY_SIZE(acl_switch_cmd), mipi_dsi0_base);
			g_acl_mode = ACL_OFFSET1_ON;
			HISI_FB_INFO("ACL OFFSET 1 ON\n");
		} else if (ACL_OFFSET2_ON == value[1]) {
			acl_switch_cmd[1].payload[0] = 0x55;
			acl_switch_cmd[1].payload[1] = ACL_OFFSET2_ON;
			mipi_dsi_cmds_tx(acl_switch_cmd, ARRAY_SIZE(acl_switch_cmd), mipi_dsi0_base);
			g_acl_mode = ACL_OFFSET2_ON;
			HISI_FB_INFO("ACL OFFSET 2 ON\n");
		} else if (ACL_OFFSET3_ON == value[1]) {
			acl_switch_cmd[1].payload[0] = 0x55;
			acl_switch_cmd[1].payload[1] = ACL_OFFSET3_ON;
			mipi_dsi_cmds_tx(acl_switch_cmd, ARRAY_SIZE(acl_switch_cmd), mipi_dsi0_base);
			g_acl_mode = ACL_OFFSET3_ON;
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
	uint32_t expected_value[2] = {0x9C, 0x80};
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

	/* we need 20 ms time to ensure that read 0A normal */
	mdelay(20);
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

static ssize_t mipi_samsung_panel_lcd_ic_color_enhancement_mode_show(struct platform_device *pdev,
	char *buf)
{
	struct dsi_cmd_desc mode_selection_read_cmds[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(mode_selection_read), mode_selection_read},
	};
	uint32_t out=0;
	struct hisi_fb_data_type *hisifd = NULL;
	int ret=0;

	HISI_FB_INFO("+\n");
	if (NULL == pdev) {
		HISI_FB_ERR("pdev NULL pointer\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd NULL pointer\n");
		return 0;
	}

	hisifb_set_vsync_activate_state(hisifd, true);
	hisifb_activate_vsync(hisifd);

	mipi_dsi_cmds_rx(&out,
		mode_selection_read_cmds, ARRAY_SIZE(mode_selection_read_cmds), hisifd->mipi_dsi0_base);
	HISI_FB_INFO("read 0x58 reg:%02x\n", out);

	hisifb_set_vsync_activate_state(hisifd, false);
	hisifb_deactivate_vsync(hisifd);

	switch(out){
		case DISPLAY_NORMAL_MODE_VALUE:
			ret = DISPLAY_FRESH_MODE;
			break;
		case DISPLAY_FRESH_MODE_VALUE:
			ret = DISPLAY_NORMAL_MODE;
			break;
		default:
			ret = DISPLAY_UNKNOWN_MODE;
			break;
	}

	HISI_FB_INFO("-\n");
	return snprintf(buf, PAGE_SIZE, "%d\n", ret);
}

static void mipi_samsung_display_set_normal_mode(struct hisi_fb_data_type *hisifd){

	HISI_FB_INFO("+\n");
	hisifb_set_vsync_activate_state(hisifd, true);
	hisifb_activate_vsync(hisifd);

	mipi_dsi_cmds_tx(dsp_normal_cmds,
		ARRAY_SIZE(dsp_normal_cmds), hisifd->mipi_dsi0_base);

	hisifb_set_vsync_activate_state(hisifd, false);
	hisifb_deactivate_vsync(hisifd);

	HISI_FB_INFO("-\n");
	return ;
}

static void mipi_samsung_display_set_fresh_mode(struct hisi_fb_data_type *hisifd){

	HISI_FB_INFO("+\n");
	hisifb_set_vsync_activate_state(hisifd, true);
	hisifb_activate_vsync(hisifd);

	mipi_dsi_cmds_tx(dsp_fresh_cmds,
		ARRAY_SIZE(dsp_fresh_cmds), hisifd->mipi_dsi0_base);

	hisifb_set_vsync_activate_state(hisifd, false);
	hisifb_deactivate_vsync(hisifd);

	HISI_FB_INFO("-\n");
	return ;
}

static ssize_t mipi_samsung_panel_lcd_ic_color_enhancement_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag = -1;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;

	HISI_FB_INFO("+\n");

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

	ret = strict_strtoul(buf, 0, &val);
	if (ret){
		HISI_FB_ERR("string to l int error:%d\n", ret);
		return ret;
	}

	flag = (int)val;

	switch(flag){
		case DISPLAY_NORMAL_MODE:
			HISI_FB_INFO("amoled set normal display mode\n");
			mipi_samsung_display_set_normal_mode(hisifd);
			g_ic_color_enhancement_local = DISPLAY_NORMAL_MODE;
			break;
		case DISPLAY_FRESH_MODE:
			HISI_FB_INFO("amoled set fresh display mode\n");
			mipi_samsung_display_set_fresh_mode(hisifd);
			g_ic_color_enhancement_local = DISPLAY_FRESH_MODE;
			break;
		default:
			HISI_FB_ERR("amoled set unknown mode, set default normal mode\n");
			g_ic_color_enhancement_local = DISPLAY_NORMAL_MODE;
			break;
	}
	HISI_FB_INFO("-\n");
	return count;
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

static ssize_t mipi_samsung_panel_support_checkmode_show(struct platform_device *pdev,
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

	ret = snprintf(buf, PAGE_SIZE, "checksum:0;lp2hs_mipi_check:0\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_samsung_panel_lp2hs_mipi_check_show(struct platform_device *pdev,
	char *buf)
{
	if (g_lp2hs_mipi_test_result) {
		return snprintf(buf, PAGE_SIZE, "OK\n");
	} else {
		return snprintf(buf, PAGE_SIZE, "FAIL\n");
	}
}

#define LP2HS_MIPI_TEST_ON		1
#define LP2HS_MIPI_TEST_OFF	0
static ssize_t mipi_samsung_panel_lp2hs_mipi_check_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag = -1;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;

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

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
		return ret;

	flag = (int)val;

	if (flag == LP2HS_MIPI_TEST_OFF) {
		g_lp2hs_mipi_test = false;
		g_lp2hs_mipi_test_result = false;
		hisifd->lcd_self_testing = false;
		HISI_FB_INFO("lp2hs_mipi test OFF\n");
	} else if (flag == LP2HS_MIPI_TEST_ON) {
		g_lp2hs_mipi_test = true;
		hisifd->lcd_self_testing = true;
		HISI_FB_INFO("lp2hs_mipi test ON\n");
	}

	return count;
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
	.esd_handle = mipi_samsung_panel_check_esd,
	.amoled_pcd_errflag_check = mipi_samsung_pcd_errflag_check,
	.lcd_test_config_show = mipi_samsung_panel_test_config_show,
	.lcd_test_config_store = mipi_samsung_panel_test_config_store,
	.lcd_support_mode_show = mipi_samsung_panel_support_mode_show,
	.lcd_support_mode_store = mipi_samsung_panel_support_mode_store,
	.lcd_support_checkmode_show = mipi_samsung_panel_support_checkmode_show,
	.lcd_lp2hs_mipi_check_show = mipi_samsung_panel_lp2hs_mipi_check_show,
	.lcd_lp2hs_mipi_check_store = mipi_samsung_panel_lp2hs_mipi_check_store,
	.lcd_ic_color_enhancement_mode_show = mipi_samsung_panel_lcd_ic_color_enhancement_mode_show,
	.lcd_ic_color_enhancement_mode_store = mipi_samsung_panel_lcd_ic_color_enhancement_mode_store,
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

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_SAMSUNG_D53G6EA8064T);
	if (!np) {
		HISI_FB_ERR("NOT FOUND device node %s!\n", DTS_COMP_SAMSUNG_D53G6EA8064T);
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

	HISI_FB_INFO("%s\n", DTS_COMP_SAMSUNG_D53G6EA8064T);

	gpio_lcd_1v8_en = of_get_named_gpio(np, "gpios", 0);    /*gpio_125*/
	gpio_lcd_reset = of_get_named_gpio(np, "gpios", 1);     /*gpio_040*/
	gpio_lcd_id0 = of_get_named_gpio(np, "gpios", 2);       /*gpio_046*/
	gpio_lcd_pcd = of_get_named_gpio(np, "gpios", 3);       /*gpio_181*/
	gpio_lcd_err_flag = of_get_named_gpio(np, "gpios", 4);  /*gpio_182*/
	HISI_FB_INFO("gpio_lcd_1v8_en = %d, gpio_lcd_reset = %d, gpio_lcd_id0 = %d, gpio_lcd_pcd = %d, gpio_lcd_err_flag = %d\n", \
		gpio_lcd_1v8_en, gpio_lcd_reset, gpio_lcd_id0, gpio_lcd_pcd, gpio_lcd_err_flag);

	pdev->id = 1;
	/* init lcd panel info */
	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct hisi_panel_info));
	pinfo->xres = 1080;
	pinfo->yres = 1920;
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
	pinfo->esd_enable = 1;
	pinfo->esd_skip_mipi_check = 1;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->lcd_adjust_support = 1;

	pinfo->dirty_region_updt_support = 0;

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
		pinfo->acm_valid_num = acm_valid_num;
		pinfo->r0_hh = acm_r0_hh;
		pinfo->r0_lh = acm_r0_lh;
		pinfo->r1_hh = acm_r1_hh;
		pinfo->r1_lh = acm_r1_lh;
		pinfo->r2_hh = acm_r2_hh;
		pinfo->r2_lh = acm_r2_lh;
		pinfo->r3_hh = acm_r3_hh;
		pinfo->r3_lh = acm_r3_lh;
		pinfo->r4_hh = acm_r4_hh;
		pinfo->r4_lh = acm_r4_lh;
		pinfo->r5_hh = acm_r5_hh;
		pinfo->r5_lh = acm_r5_lh;
		pinfo->r6_hh = acm_r6_hh;
		pinfo->r6_lh = acm_r6_lh;

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
	pinfo->ldi.h_back_porch = 23;
	pinfo->ldi.h_front_porch = 50;
	pinfo->ldi.h_pulse_width = 20;
	pinfo->ldi.v_back_porch = 28;
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
	pinfo->mipi.clk_t_lpx_adjust = -56;
	pinfo->mipi.clk_post_adjust=150;
	pinfo->mipi.dsi_bit_clk = 472;
	pinfo->mipi.dsi_bit_clk_val1 = 444;
	pinfo->mipi.dsi_bit_clk_val2 = 459;
	pinfo->mipi.dsi_bit_clk_val3 = 472;
	pinfo->dsi_bit_clk_upt_support = 1;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	pinfo->pxl_clk_rate = 146 * 1000000UL;

	pinfo->ifbc_type = IFBC_TYPE_NONE;

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
		.compatible = DTS_COMP_SAMSUNG_D53G6EA8064T,
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
		.name = "mipi_samsung_D53G6EA8064T_5P5_AMOLED",
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
