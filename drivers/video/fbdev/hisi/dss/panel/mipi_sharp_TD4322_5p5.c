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
#include "mipi_sharp_TD4322_5p5.h"
#include <huawei_platform/log/log_jank.h>
#include <linux/hisi/hw_cmdline_parse.h>

#define JDI_TD4322_ERR -1;
#define JDI_TD4322_OK 0

#define DTS_COMP_SHARP_TD4322_5P5 "hisilicon,mipi_sharp_TD4322_5P5"
#define LCD_VDDIO_TYPE_NAME	"lcd-vddio-type"
#define CABC_OFF	(0)
#define CABC_UI_MODE	(1)
#define CABC_STILL_MODE	(2)
#define CABC_MOVING_MODE	(3)

#define PACKET_PIXELS_SIZE	(216)
#define PACKET_SIZE	(PACKET_PIXELS_SIZE*3+1)
#define PATTERN_PIXELS_X_SIZE	1080
#define PATTERN_PIXELS_Y_SIZE	1920
#define GPIO_TE0 23

#define BACKLIGHT_PRINT_TIMES	10
#define CHECKSUM_SIZE	(8)
#define CHECKSUM_PIC_NUM  (10)
#define CHECKSUM_PIC_N  (4)
#define DISPLAY_BL_MAX_NIT_STANDARD	550

static int checksum_pic_n = 0;
static uint32_t expected_checksum[CHECKSUM_PIC_N][CHECKSUM_SIZE] = {
	{0x5B, 0x5E, 0x8E, 0xEB, 0xE5, 0x2B, 0x46, 0xF9}, //mmi
	{0xBE, 0x8A, 0x67, 0x5C, 0x6, 0xFA, 0xFB, 0xDB}, //runningtest1
	{0xFB, 0xCE, 0x5C, 0x3D, 0x4D, 0x0F, 0x41, 0x6}, //runningtest2
	{0xA6, 0x91, 0x1B, 0xD7, 0x71, 0x44, 0x3F, 0x75}, //runningtest3
};

static struct hisi_fb_panel_data g_panel_data;

static int hkadc_buf = 0;
static bool checksum_enable_ctl = false;
static int g_debug_enable = 0;
static int g_cabc_mode = 2;

extern bool gesture_func;
extern bool g_lcd_control_tp_power;
extern unsigned int g_led_rg_para1;
extern unsigned int g_led_rg_para2;
extern int g_lcd_brightness_info;
extern u8 color_temp_cal_buf[32];
extern unsigned int g_esd_recover_disable;

#define GPIO_LCD_P5V5_ENABLE_NAME	"gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"

#define GPIO_LCD_VDDIO_NAME "gpio_lcd_vddio_switch"

struct jdi_td4322_device {
	uint32_t bl_type;
	uint32_t gpio_n5v5_enable;
	uint32_t gpio_p5v5_enable;
	uint32_t gpio_reset;
	uint32_t gpio_vddio_ctrl;
	uint32_t gpio_id0;
	uint32_t lcd_support_dynamic_gamma;
	char bl_ic_name_buf[LCD_BL_IC_NAME_MAX];
};

static struct jdi_td4322_device g_dev_info;

#define SCALING_UP_FUNC (0)

static int mipi_sharp_panel_set_display_region(struct platform_device *pdev, struct dss_rect *dirty);
extern int synaptics_get_project_id(unsigned char *projectid, int plen);

//static uint32_t g_vddio_type = 0;
extern void ts_power_gpio_enable(void);
extern void ts_power_gpio_disable(void);

/*******************************************************************************
** Scaling up function
*/
#if SCALING_UP_FUNC
static char ena_scale[] = {
	0x50,
	0x01,
};
#endif

/*******************************************************************************
** Partial update setting
*/
static char partial_setting_1[] = {
	0xFF,
	0x10,
};

static char partial_setting_2[] = {
	0xFB,
	0x01,
};

static char partial_setting_3[] = {
	0xC9,
	0x4B,0x04,0x21,0x00,0x0F,0x03,0x19,0x01,0x97,0x10,0xF0,
};

/*******************************************************************************
** Power ON Sequence(sleep mode to Normal mode)
*/
static char bl_val[] = {
	0x51,
	0xFF,
};

static char color_enhancement[] = {
	0x30,
	0x00,0x00,0x02,0xA7,
};

static char te_enable[] = {
	0x35,
	0x00,
};

static char te_line[] = {
	0x44,
	0x03, 0x80,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

static char exit_sleep[] = {
	0x11,
};

static char display_on[] = {
	0x29,
};

static char vesa_dsc0[] = {
	0xFF,
	0x10,
};

//0x03:vesa enable, 0x00: disable
static char vesa_dsc1[] = {
	0xC0,
	0x03,
};

static char vesa_dsc2[] = {
	0xC1,
	0x09,0x20,0x00,0x10,0x02,
	0x00,0x03,0x1C,0x02,0x05,
	0x00,0x0F,0x06,0x67,0x03,
	0x2E,
};

static char vesa_dsc3[] = {
	0xC2,
	0x10,0xF0,
};

static char vesa_dsc4[] = {
	0xFB,
	0x01,
};

//select mipi lane numbers, 0x00:1 lane, 0x01: 2 lane, and so on. Default 4 lanes
static char lane_nums[] = {
	0xBA,
	0x03,
};

/*******************************************************************************
** setting PWM frequency to 58KHZ
*/
static char set_page3[] = {
	0xFF,
	0x23,
};

static char set_pwm_freq[] = {
	0x08,
	0x03,
};

static char set_page0[] = {
	0xFF,
	0x10,
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

static struct dsi_cmd_desc lcd_display_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(te_enable), te_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_val), bl_val},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement), color_enhancement},
	{DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_US,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 0, 80, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc lcd_display_on_new_seq_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(te_enable), te_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(bl_val), bl_val},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement), color_enhancement},
	{DTYPE_DCS_WRITE, 0, 10, WAIT_TYPE_US,
		sizeof(display_on), display_on},
	{DTYPE_DCS_WRITE, 0, 70, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
};

static struct dsi_cmd_desc lcd_display_off_cmds[] = {
	{DTYPE_DCS_WRITE, 0, 60, WAIT_TYPE_MS,
		sizeof(display_off), display_off},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(enter_sleep), enter_sleep}
};

static struct dsi_cmd_desc lcd_partial_updt_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(partial_setting_1), partial_setting_1},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(partial_setting_2), partial_setting_2},
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(partial_setting_3), partial_setting_3},
};

static struct dsi_cmd_desc lcd_vesa3x_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(vesa_dsc0), vesa_dsc0},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(vesa_dsc1), vesa_dsc1},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(vesa_dsc2), vesa_dsc2},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(vesa_dsc3), vesa_dsc3},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(vesa_dsc4), vesa_dsc4},
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

static char non_reload_0xFB[] = {
	0xFB,
	0x01,
};

static char cmd1_0xFF[] = {
	0xFF,
	0x10,
};

static char page_selection_0xFF_0x20[] = {
	0xFF,
	0x20,
};

static char gamma_dither_0x68[] = {
	0x68,
	0x03,
};

static char color_enhancement_0xFF[] = {
	0xFF,
	0x22,
};

static char color_enhancement_0xFB[] = {
	0xFB,
	0x01,
};

static char color_enhancement_0x00[] = {
	0x00,
	0x50,
};

static char color_enhancement_0x01[] = {
	0x01,
	0x50,
};

static char color_enhancement_0x02[] = {
	0x02,
	0x50,
};

static char color_enhancement_0x03[] = {
	0x03,
	0x50,
};

static char color_enhancement_0x04[] = {
	0x04,
	0x50,
};

static char color_enhancement_0x05[] = {
	0x05,
	0x50,
};

static char color_enhancement_0x06[] = {
	0x06,
	0x4C,
};

static char color_enhancement_0x07[] = {
	0x07,
	0x48,
};

static char color_enhancement_0x08[] = {
	0x08,
	0x44,
};

static char color_enhancement_0x09[] = {
	0x09,
	0x40,
};

static char color_enhancement_0x0A[] = {
	0x0A,
	0x3C,
};

static char color_enhancement_0x0B[] = {
	0x0B,
	0x38,
};

static char color_enhancement_0x0C[] = {
	0x0C,
	0x38,
};

static char color_enhancement_0x0D[] = {
	0x0D,
	0x30,
};

static char color_enhancement_0x0E[] = {
	0x0E,
	0x28,
};

static char color_enhancement_0x0F[] = {
	0x0F,
	0x20,
};

static char color_enhancement_0x10[] = {
	0x10,
	0x10,
};

static char color_enhancement_0x11[] = {
	0x11,
	0x00,
};

static char color_enhancement_0x12[] = {
	0x12,
	0x00,
};

static char color_enhancement_0x13[] = {
	0x13,
	0x00,
};

static char color_enhancement_0x1A[] = {
	0x1A,
	0x00,
};

static char color_enhancement_0x1B[] = {
	0x1B,
	0x00,
};

static char color_enhancement_0x1C[] = {
	0x1C,
	0x00,
};

static char color_enhancement_0x1D[] = {
	0x1D,
	0x00,
};

static char color_enhancement_0x1E[] = {
	0x1E,
	0x00,
};

static char color_enhancement_0x1F[] = {
	0x1F,
	0x00,
};

static char color_enhancement_0x20[] = {
	0x20,
	0x00,
};

static char color_enhancement_0x21[] = {
	0x21,
	0x00,
};

static char color_enhancement_0x22[] = {
	0x22,
	0x00,
};

static char color_enhancement_0x23[] = {
	0x23,
	0x00,
};

static char color_enhancement_0x24[] = {
	0x24,
	0x03,
};

static char color_enhancement_0x25[] = {
	0x25,
	0x08,
};

static char color_enhancement_0x26[] = {
	0x26,
	0x0E,
};

static char color_enhancement_0x27[] = {
	0x27,
	0x14,
};

static char color_enhancement_0x28[] = {
	0x28,
	0x1A,
};

static char color_enhancement_0x29[] = {
	0x29,
	0x20,
};

static char color_enhancement_0x2A[] = {
	0x2A,
	0x26,
};

static char color_enhancement_0x2B[] = {
	0x2B,
	0x27,
};

static char color_enhancement_0x2F[] = {
	0x2F,
	0x22,
};

static char color_enhancement_0x30[] = {
	0x30,
	0x1F,
};

static char color_enhancement_0x31[] = {
	0x31,
	0x1B,
};

static char color_enhancement_0x32[] = {
	0x32,
	0x92,
};

static char color_enhancement_0x33[] = {
	0x33,
	0x90,
};

static char color_enhancement_0x34[] = {
	0x34,
	0x8C,
};

static char color_enhancement_0x35[] = {
	0x35,
	0x89,
};

static char color_enhancement_0x36[] = {
	0x36,
	0x86,
};

static char color_enhancement_0x37[] = {
	0x37,
	0x83,
};

static char color_enhancement_0x38[] = {
	0x38,
	0x80,
};

static char color_enhancement_0x39[] = {
	0x39,
	0x80,
};

static char color_enhancement_0x3A[] = {
	0x3A,
	0x80,
};

static char color_enhancement_0x3B[] = {
	0x3B,
	0x80,
};

static char color_enhancement_0x3F[] = {
	0x3F,
	0x80,
};

static char color_enhancement_0x40[] = {
	0x40,
	0x80,
};

static char color_enhancement_0x41[] = {
	0x41,
	0x80,
};

static char color_enhancement_0x42[] = {
	0x42,
	0x80,
};

static char color_enhancement_0x43[] = {
	0x43,
	0x80,
};

static char color_enhancement_0x44[] = {
	0x44,
	0x80,
};

static char color_enhancement_0x45[] = {
	0x45,
	0x80,
};

static char color_enhancement_0x46[] = {
	0x46,
	0x80,
};

static char color_enhancement_0x47[] = {
	0x47,
	0x80,
};

static char color_enhancement_0x48[] = {
	0x48,
	0x80,
};

static char color_enhancement_0x49[] = {
	0x49,
	0x80,
};

static char color_enhancement_0x4A[] = {
	0x4A,
	0x80,
};

static char color_enhancement_0x4B[] = {
	0x4B,
	0x85,
};

static char color_enhancement_0x4C[] = {
	0x4C,
	0x8A,
};

static char color_enhancement_0x4D[] = {
	0x4D,
	0x00,
};

static char color_enhancement_0x4E[] = {
	0x4E,
	0x00,
};

static char color_enhancement_0x4F[] = {
	0x4F,
	0x0A,
};

static char color_enhancement_0x50[] = {
	0x50,
	0x00,
};

static char color_enhancement_0x51[] = {
	0x51,
	0x00,
};

static char color_enhancement_0x52[] = {
	0x52,
	0x00,
};

static char color_enhancement_0x53[] = {
	0x53,
	0x07,
};

static char color_enhancement_0x54[] = {
	0x54,
	0x03,
};

static char color_enhancement_0x55[] = {
	0x55,
	0x07,
};

static char color_enhancement_0x56[] = {
	0x56,
	0x00,
};

static char color_enhancement_0x68[] = {
	0x68,
	0x00,
};

static char color_enhancement_0x7D[] = {
	0x7D,
	0x04,
};

static char color_enhancement_0x84[] = {
	0x84,
	0x00,
};

static char color_enhancement_0x85[] = {
	0x85,
	0x00,
};

static char color_enhancement_0x86[] = {
	0x86,
	0x00,
};

static char color_enhancement_0x87[] = {
	0x87,
	0xE0,
};

static char color_enhancement_0x88[] = {
	0x88,
	0xFF,
};

static char color_enhancement_0x99[] = {
	0x99,
	0x34,
};

static char color_enhancement_0x9A[] = {
	0x9A,
	0x2D,
};

static char color_enhancement_0x9C[] = {
	0x9C,
	0x7F,
};

static char color_enhancement_0x9D[] = {
	0x9D,
	0x00,
};

static char color_enhancement_0x9E[] = {
	0x9E,
	0xFF,
};

static char color_enhancement_0x9F[] = {
	0x9F,
	0x00,
};

static char color_enhancement_0xA2[] = {
	0xA2,
	0x20,
};

static char color_enhancement_0xC9[] = {
	0xC9,
	0x44,
};

static char color_enhancement_0xCA[] = {
	0xCA,
	0x34,
};

static char cabc_0xFF[] = {
	0xFF,
	0x23,
};

static char cabc_0xFB[] = {
	0xFB,
	0x01,
};

static char cabc_0x00[] = {
	0x00,
	0x02,
};

static char cabc_0x01[] = {
	0x01,
	0x84,
};

static char cabc_0x05[] = {
	0x05,
	0x24,
};

static char cabc_0x09[] = {
	0x09,
	0x00,
};

static char cabc_0x0A[] = {
	0x0A,
	0x02,
};

static char cabc_0x0B[] = {
	0x0B,
	0x03,
};

static char cabc_0x0C[] = {
	0x0C,
	0x04,
};

static char cabc_0x0D[] = {
	0x0D,
	0x05,
};

static char cabc_0x0E[] = {
	0x0E,
	0x06,
};

static char cabc_0x0F[] = {
	0x0F,
	0x07,
};

static char cabc_0x10[] = {
	0x10,
	0x08,
};

static char cabc_0x11[] = {
	0x11,
	0x08,
};

static char cabc_0x12[] = {
	0x12,
	0x0C,
};

static char cabc_0x17[] = {
	0x17,
	0xFF,
};

static char cabc_0x18[] = {
	0x18,
	0xFB,
};

static char cabc_0x19[] = {
	0x19,
	0xF5,
};

static char cabc_0x1A[] = {
	0x1A,
	0xEF,
};

static char cabc_0x1B[] = {
	0x1B,
	0xEA,
};

static char cabc_0x1C[] = {
	0x1C,
	0xE6,
};

static char cabc_0x1D[] = {
	0x1D,
	0xE3,
};

static char cabc_0x1E[] = {
	0x1E,
	0xE2,
};

static char cabc_0x1F[] = {
	0x1F,
	0xE1,
};

static char cabc_0x20[] = {
	0x20,
	0xE0,
};

static char cabc_0x21[] = {
	0x21,
	0xF4,
};

static char cabc_0x22[] = {
	0x22,
	0xEA,
};

static char cabc_0x23[] = {
	0x23,
	0xDF,
};

static char cabc_0x24[] = {
	0x24,
	0xCF,
};

static char cabc_0x25[] = {
	0x25,
	0xC1,
};

static char cabc_0x26[] = {
	0x26,
	0xB2,
};

static char cabc_0x27[] = {
	0x27,
	0xA8,
};

static char cabc_0x28[] = {
	0x28,
	0xA3,
};

static char cabc_0x29[] = {
	0x29,
	0x9B,
};

static char cabc_0x2A[] = {
	0x2A,
	0x96,
};

static char cabc_0x2D[] = {
	0x2D,
	0x80,
};

static char cabc_0x33[] = {
	0x33,
	0x16,
};

static char cabc_0x40[] = {
	0x40,
	0xFF,
};

static char cabc_0x41[] = {
	0x41,
	0xe0,
};

static char cabc_off[] = {
	0x55,
	0x90,
};

static char cabc_set_mode_UI[] = {
	0x55,
	0x91,
};

static char cabc_set_mode_STILL[] = {
	0x55,
	0x92,
};

static char cabc_set_mode_MOVING[] = {
	0x55,
	0x93,
};

static struct dsi_cmd_desc sharp_cabc_off_cmds[] = {
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_off), cabc_off},
};

static struct dsi_cmd_desc sharp_cabc_ui_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_set_mode_UI), cabc_set_mode_UI},
};

static struct dsi_cmd_desc sharp_cabc_still_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_set_mode_STILL), cabc_set_mode_STILL},
};

static struct dsi_cmd_desc sharp_cabc_moving_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_set_mode_MOVING), cabc_set_mode_MOVING},
};

static struct dsi_cmd_desc sharp_display_effect_on_cmds[] = {
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(page_selection_0xFF_0x20), page_selection_0xFF_0x20},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof( non_reload_0xFB),  non_reload_0xFB},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(gamma_dither_0x68), gamma_dither_0x68},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0xFF), color_enhancement_0xFF},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0xFB), color_enhancement_0xFB},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x00), color_enhancement_0x00},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x01), color_enhancement_0x01},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x02), color_enhancement_0x02},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x03), color_enhancement_0x03},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x04), color_enhancement_0x04},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x05), color_enhancement_0x05},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x06), color_enhancement_0x06},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x07), color_enhancement_0x07},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x08), color_enhancement_0x08},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x09), color_enhancement_0x09},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x0A), color_enhancement_0x0A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x0B), color_enhancement_0x0B},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x0C), color_enhancement_0x0C},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x0D), color_enhancement_0x0D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x0E), color_enhancement_0x0E},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x0F), color_enhancement_0x0F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x10), color_enhancement_0x10},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x11), color_enhancement_0x11},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x12), color_enhancement_0x12},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x13), color_enhancement_0x13},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x1A), color_enhancement_0x1A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x1B), color_enhancement_0x1B},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x1C), color_enhancement_0x1C},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x1D), color_enhancement_0x1D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x1E), color_enhancement_0x1E},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x1F), color_enhancement_0x1F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x20), color_enhancement_0x20},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x21), color_enhancement_0x21},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x22), color_enhancement_0x22},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x23), color_enhancement_0x23},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x24), color_enhancement_0x24},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x25), color_enhancement_0x25},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x26), color_enhancement_0x26},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x27), color_enhancement_0x27},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x28), color_enhancement_0x28},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x29), color_enhancement_0x29},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x2A), color_enhancement_0x2A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x2B), color_enhancement_0x2B},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x2F), color_enhancement_0x2F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x30), color_enhancement_0x30},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x31), color_enhancement_0x31},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x32), color_enhancement_0x32},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x33), color_enhancement_0x33},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x34), color_enhancement_0x34},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x35), color_enhancement_0x35},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x36), color_enhancement_0x36},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x37), color_enhancement_0x37},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x38), color_enhancement_0x38},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x39), color_enhancement_0x39},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x3A), color_enhancement_0x3A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x3B), color_enhancement_0x3B},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x3F), color_enhancement_0x3F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x40), color_enhancement_0x40},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x41), color_enhancement_0x41},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x42), color_enhancement_0x42},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x43), color_enhancement_0x43},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x44), color_enhancement_0x44},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x45), color_enhancement_0x45},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x46), color_enhancement_0x46},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x47), color_enhancement_0x47},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x48), color_enhancement_0x48},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x49), color_enhancement_0x49},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x4A), color_enhancement_0x4A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x4B), color_enhancement_0x4B},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x4C), color_enhancement_0x4C},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x4D), color_enhancement_0x4D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x4E), color_enhancement_0x4E},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x4F), color_enhancement_0x4F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x50), color_enhancement_0x50},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x51), color_enhancement_0x51},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x52), color_enhancement_0x52},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x53), color_enhancement_0x53},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x54), color_enhancement_0x54},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x55), color_enhancement_0x55},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x56), color_enhancement_0x56},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x68), color_enhancement_0x68},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x7D), color_enhancement_0x7D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x84), color_enhancement_0x84},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x85), color_enhancement_0x85},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x86), color_enhancement_0x86},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x87), color_enhancement_0x87},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x88), color_enhancement_0x88},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x99), color_enhancement_0x99},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x9A), color_enhancement_0x9A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x9C), color_enhancement_0x9C},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x9D), color_enhancement_0x9D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x9E), color_enhancement_0x9E},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0x9F), color_enhancement_0x9F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0xA2), color_enhancement_0xA2},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0xC9), color_enhancement_0xC9},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(color_enhancement_0xCA), color_enhancement_0xCA},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0xFF), cabc_0xFF},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0xFB), cabc_0xFB},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x00), cabc_0x00},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x05), cabc_0x05},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x09), cabc_0x09},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x0A), cabc_0x0A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x0B), cabc_0x0B},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x0C), cabc_0x0C},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x0D), cabc_0x0D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x0E), cabc_0x0E},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x0F), cabc_0x0F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x10), cabc_0x10},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x11), cabc_0x11},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x12), cabc_0x12},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x17), cabc_0x17},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x18), cabc_0x18},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x19), cabc_0x19},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x1A), cabc_0x1A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x1B), cabc_0x1B},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x1C), cabc_0x1C},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x1D), cabc_0x1D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x1E), cabc_0x1E},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x1F), cabc_0x1F},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x20), cabc_0x20},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x21), cabc_0x21},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x22), cabc_0x22},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x23), cabc_0x23},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x24), cabc_0x24},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x25), cabc_0x25},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x26), cabc_0x26},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x27), cabc_0x27},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x28), cabc_0x28},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x29), cabc_0x29},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x2A), cabc_0x2A},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x2D), cabc_0x2D},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x33), cabc_0x33},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x40), cabc_0x40},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_0x41), cabc_0x41},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(cmd1_0xFF), cmd1_0xFF},
	{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
		sizeof(cabc_set_mode_STILL), cabc_set_mode_STILL},
};

/******************************************************************************
*
** SRAM test sequence
*/
static char MCAP[] = {
	0xB0,
	0x04,
};

static char test_reg_FEh_pre[] = {
	0xFE,
	0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x00,
};

static char test_reg_F1h_pre[] = {
	0xF1,
	0x80, 0x00,
};

static char test_reg_FCh_pre[] = {
	0xFC,
	0x02, 0x14, 0x48, 0x00, 0x01, 0x3C,
	0x03, 0x00,
};

static struct dsi_cmd_desc sram_test_pre_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(MCAP), MCAP},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_reg_FEh_pre), test_reg_FEh_pre},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_reg_F1h_pre), test_reg_F1h_pre},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_reg_FCh_pre), test_reg_FCh_pre},
};

static char test_reg_FEh_post[] = {
	0xFE,
	0x00, 0x00, 0x80, 0x01, 0x00, 0x00, 0x00, 0x09,
};

static char test_reg_F1h_post[] = {
	0xF1,
	0x00, 0x02,
};

static char test_reg_FCh_post[] = {
	0xFC,
	0x04, 0x14, 0x80, 0x10, 0x02, 0x3C,
	0x63, 0x00,
};

static struct dsi_cmd_desc sram_test_post_cmds[] = {
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_reg_FEh_post), test_reg_FEh_post},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_reg_F1h_post), test_reg_F1h_post},
	{DTYPE_GEN_LWRITE, 0, 10, WAIT_TYPE_US,
		sizeof(test_reg_FCh_post), test_reg_FCh_post},
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

static struct regulator *vcc_lcdanalog;

static struct vcc_desc lcd_vcc_lcdanalog_init_cmds[] = {
	{DTYPE_VCC_GET, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 3300000, 3300000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_lcdanalog_finit_cmds[] = {
	{DTYPE_VCC_PUT, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc lcd_vcc_lcdanalog_enable_cmds[] = {
	{DTYPE_VCC_ENABLE, VCC_LCDANALOG_NAME, &vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 10},
};

static struct vcc_desc lcd_vcc_lcdanalog_disable_cmds[] = {
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
static struct gpio_desc lcd_gpio_vddio_switch_on_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_VDDIO_NAME, &g_dev_info.gpio_vddio_ctrl, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_VDDIO_NAME, &g_dev_info.gpio_vddio_ctrl, 1},
};

static struct gpio_desc lcd_gpio_vddio_switch_off_cmds[] = {
	/* lcd vddio gpio down */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_VDDIO_NAME, &g_dev_info.gpio_vddio_ctrl, 0},
	/* set vddio gpio input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_VDDIO_NAME, &g_dev_info.gpio_vddio_ctrl, 0},
	/* free vddio gpio */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_VDDIO_NAME, &g_dev_info.gpio_vddio_ctrl, 0},
};

static struct gpio_desc lcd_gpio_vsp_n_enable_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_dev_info.gpio_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_dev_info.gpio_n5v5_enable, 0},
	/* AVDD_5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 10,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_dev_info.gpio_p5v5_enable, 1},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_dev_info.gpio_n5v5_enable, 1},
};

static struct gpio_desc lcd_gpio_vsp_n_disable_cmds[] = {
	/* AVEE_-5.5V gpio disable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_dev_info.gpio_n5v5_enable, 0},
	/* AVDD_5.5V gpio disable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_dev_info.gpio_p5v5_enable, 0},
	/* AVEE_-5.5V gpio input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_dev_info.gpio_n5v5_enable, 0},
	/* AVDD_5.5V gpio input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_dev_info.gpio_p5v5_enable, 0},
	/* AVEE_-5.5V gpio free */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_N5V5_ENABLE_NAME, &g_dev_info.gpio_n5v5_enable, 0},
	/* AVDD_5.5V gpio free */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_P5V5_ENABLE_NAME, &g_dev_info.gpio_p5v5_enable, 0},
};

static struct gpio_desc lcd_gpio_reset_cmds[] = {
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},

	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 85,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 1},
};

static struct gpio_desc lcd_gpio_reset_new_seq_cmds[] = {
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},

	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 1},
};

static struct gpio_desc lcd_gpio_reset_down_cmds[] = {
	/* lcd reset gpio down */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 8,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
	/* lcd reset gpio input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
	/* lcd reset free */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
};

static struct gpio_desc lcd_gpio_sleep_request_cmds[] = {
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
	/* id0 */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &g_dev_info.gpio_id0, 0},
};

static struct gpio_desc lcd_gpio_sleep_free_cmds[] = {
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
	/* lcd id */
	{DTYPE_GPIO_FREE, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &g_dev_info.gpio_id0, 0},
};

static struct gpio_desc lcd_gpio_sleep_normal_cmds[] = {
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 20,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
		GPIO_LCD_RESET_NAME, &g_dev_info.gpio_reset, 1},
	/* lcd id */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &g_dev_info.gpio_id0, 0},
};

/*******************************************************************************
** ACM
*/
static u32 acm_lut_hue_table[] = {
	  0,    4,    8,   12,   16,   20,   24,   28,   32,   36,   40,
	 44,   48,   52,   56,   60,   64,   68,   72,   76,   80,   84,
	 88,   92,   96,  100,  104,  108,  112,  116,  120,  124,  128,
	132,  136,  140,  144,  148,  152,  156,  160,  164,  168,  172,
	176,  180,  184,  188,  192,  196,  200,  204,  208,  212,  216,
	220,  224,  228,  232,  236,  240,  244,  248,  252,  256,  260,
	264,  268,  272,  276,  280,  284,  288,  292,  296,  300,  304,
	308,  312,  316,  320,  324,  328,  332,  336,  340,  344,  348,
	352,  356,  360,  364,  368,  372,  376,  380,  384,  388,  392,
	396,  400,  404,  408,  412,  416,  420,  424,  428,  432,  436,
	440,  444,  448,  452,  456,  460,  464,  468,  472,  476,  480,
	484,  488,  492,  496,  500,  504,  508,  512,  516,  520,  524,
	528,  532,  536,  540,  544,  548,  552,  556,  560,  564,  568,
	572,  576,  580,  584,  588,  592,  596,  600,  604,  608,  612,
	616,  620,  624,  628,  632,  636,  640,  644,  648,  652,  656,
	660,  664,  668,  672,  676,  680,  684,  688,  692,  696,  700,
	704,  708,  712,  716,  720,  724,  728,  732,  736,  740,  744,
	748,  752,  756,  760,  764,  768,  772,  776,  780,  784,  788,
	792,  796,  800,  804,  808,  812,  816,  820,  824,  828,  832,
	836,  840,  844,  848,  852,  856,  860,  864,  868,  872,  876,
	880,  884,  888,  892,  896,  900,  904,  908,  912,  916,  920,
	924,  928,  932,  936,  940,  944,  948,  952,  956,  960,  964,
	968,  972,  976,  980,  984,  988,  992,  996, 1000, 1004, 1008,
	1012, 1016, 1020
};

static u32 acm_lut_sata_table[] = {
	 5,  4,  4,  4,  3,  2,  2,  2,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  1,  2,
	 2,  2,  3,  4,  4,  6,  7,  8, 10, 12, 13, 14, 16, 18, 21, 23, 26, 28, 30, 33,
	35, 37, 38, 40, 42, 43, 45, 46, 48, 49, 50, 51, 52, 53, 54, 55, 56, 56, 57, 57,
	58, 58, 58, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59, 59,
	59, 59, 59, 59, 58, 58, 58, 58, 58, 57, 56, 55, 54, 52, 51, 50, 49, 48, 48, 47,
	46, 45, 44, 44, 43, 43, 42, 42, 42, 41, 41, 40, 40, 40, 40, 41, 41, 41, 42, 42,
	42, 42, 42, 41, 41, 41, 40, 40, 40, 41, 42, 42, 43, 44, 44, 45, 46, 47, 48, 49,
	50, 50, 51, 52, 53, 54, 54, 55, 56, 56, 57, 57, 58, 57, 56, 56, 55, 54, 54, 53,
	52, 52, 51, 50, 50, 50, 49, 48, 48, 47, 46, 45, 44, 44, 43, 42, 41, 40, 40, 39,
	38, 37, 36, 36, 35, 35, 35, 35, 34, 34, 34, 34, 34, 34, 34, 33, 33, 33, 32, 32,
	32, 32, 31, 31, 30, 30, 30, 29, 29, 29, 28, 28, 28, 28, 28, 27, 27, 26, 26, 25,
	24, 23, 22, 22, 21, 21, 20, 20, 20, 19, 19, 18, 18, 18, 17, 17, 16, 16, 16, 15,
	15, 15, 14, 14, 14, 13, 13, 12, 12, 11, 10,  9,  8,  8,  7,  6
};

static u32 acm_lut_satr0_table[] = {
	 0,  2,  3,  5,  6,  8, 10, 11, 13, 14,
	16, 16, 17, 17, 18, 18, 18, 19, 19, 20,
	20, 21, 21, 22, 22, 23, 23, 23, 24, 24,
	25, 25, 25, 25, 24, 24, 24, 24, 23, 23,
	23, 23, 22, 22, 22, 22, 21, 21, 21, 21,
	20, 20, 20, 20, 18, 16, 14, 12, 10,  8,
	 6,  4,  2,  0
};

static u32 acm_lut_satr1_table[] = {
	 0,  0,  0,  4,  7, 11, 15, 19, 25, 30,
	35, 39, 41, 44, 46, 48, 50, 53, 55, 57,
	59, 62, 64, 66, 68, 71, 73, 75, 77, 80,
	82, 84, 83, 81, 80, 78, 77, 75, 74, 72,
	71, 69, 68, 67, 65, 64, 62, 61, 59, 58,
	56, 49, 42, 35, 30, 25, 20, 15, 11,  7,
	 4,  0,  0,  0
};

static u32 acm_lut_satr2_table[] = {
	 0,  0,  0,  0,  0,  3,  9, 14, 18, 23,
	27, 32, 36, 41, 44, 48, 51, 56, 59, 62,
	65, 66, 69, 71, 72, 74, 75, 77, 77, 77,
	77, 77, 75, 75, 74, 72, 71, 68, 66, 63,
	60, 57, 54, 50, 47, 42, 38, 35, 30, 26,
	21, 15, 11,  6,  2,  0,  0,  0,  0,  0,
	 0,  0,  0,  0
};

static u32 acm_lut_satr3_table[] = {
	 0,  0,  0,  0,  0,  0,  0,  3,  7,  9,
	13, 16, 19, 22, 25, 28, 30, 34, 36, 39,
	41, 43, 45, 46, 48, 49, 50, 51, 53, 54,
	54, 54, 54, 54, 53, 53, 51, 50, 49, 47,
	46, 44, 42, 40, 38, 35, 33, 29, 26, 24,
	21, 18, 15, 11, 9, 7, 5, 3, 0, 0,
	 0, 0, 0, 0
};

static u32 acm_lut_satr4_table[] = {
	 0,  0,  0,  0,  0,  0,  0,  3,  7,  9,
	13, 16, 19, 22, 25, 28, 30, 34, 36, 39,
	41, 43, 45, 46, 48, 49, 50, 51, 53, 54,
	54, 54, 54, 54, 53, 53, 51, 50, 49, 47,
	46, 44, 42, 40, 38, 35, 33, 29, 26, 24,
	21, 18, 15, 11,  9,  7,  5,  3,  0,  0,
	 0,  0,  0,  0
};


static u32 acm_lut_satr5_table[] = {
	 0,  0,  0,  0,  0,  0,  0,  2,  6,  9,
	13, 16, 19, 22, 25, 28, 30, 34, 36, 39,
	41, 43, 45, 46, 48, 49, 50, 51, 53, 54,
	54, 54, 54, 54, 53, 53, 51, 50, 49, 47,
	46, 44, 42, 40, 38, 35, 33, 29, 26, 24,
	21, 18, 15, 11,  7,  4,  1,  0,  0,  0,
	 0,  0,  0,  0
};

static u32 acm_lut_satr6_table[] = {
	 0,  0,  0,  5, 10, 16, 22, 28, 33, 39,
	46, 50, 51, 52, 53, 54, 54, 55, 56, 56,
	57, 58, 59, 59, 60, 61, 61, 62, 63, 64,
	64, 65, 64, 63, 63, 62, 61, 60, 59, 58,
	58, 57, 56, 55, 54, 54, 53, 52, 51, 50,
	49, 48, 47, 43, 38, 33, 28, 21, 15, 10,
	 5,  0,  0,  0
};

static u32 acm_lut_satr7_table[] = {
	 0,  4,  7, 11, 15, 18, 22, 26, 30, 33,
	37, 38, 38, 39, 40, 40, 41, 42, 42, 43,
	44, 44, 45, 46, 46, 47, 48, 48, 49, 50,
	50, 51, 50, 50, 49, 48, 48, 47, 47, 46,
	45, 45, 44, 43, 43, 42, 41, 41, 40, 40,
	39, 38, 38, 37, 33, 30, 26, 22, 18, 15,
	11,  7,  4,  0
};

/*******************************************************************************
** 2d sharpness
*/
static sharp2d_t sharp2d_table = {
	.sharp_en = 1, .sharp_mode = 0,
	.flt0_c0 = 0xf0, .flt0_c1 = 0xf0, .flt0_c2 = 0xf0,
	.flt1_c0 = 0x1c, .flt1_c1 = 0x1c, .flt1_c2 = 0xf0,
	.flt2_c0 = 0x20, .flt2_c1 = 0x1c, .flt2_c2 = 0xf0,
	.ungain = 0x64, .ovgain = 0x64,
	.lineamt1 = 0xff, .linedeten = 0x1, .linethd2 = 0x0, .linethd1 = 0x1f,
	.sharpthd1 = 0x10, .sharpthd1mul = 0x100, .sharpamt1 = 0x40,
	.edgethd1 = 0xff, .edgethd1mul = 0xfff, .edgeamt1 = 0xfff,
};

/*******************************************************************************
**
*/
static int mipi_sharp_panel_set_fastboot(struct platform_device *pdev)
{
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

	// lcd pinctrl normal
	pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
		ARRAY_SIZE(lcd_pinctrl_normal_cmds));

	// backlight on
	hisi_lcd_backlight_on(pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static u8 esd_recovery_times = 0;
static int mipi_sharp_panel_on(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	int error = 0;
	static struct lcd_reg_read_t lcd_status_reg[] = {
		{0x0A, 0x1C, 0xFF, "lcd power state"},
	};
	unsigned char tp_project_id[32] = {0};
	bool new_seq_tag = false;

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

	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		g_debug_enable = BACKLIGHT_PRINT_TIMES;
		LOG_JANK_D(JLID_KERNEL_LCD_POWER_ON, "%s", "JL_KERNEL_LCD_POWER_ON");
		if (false == gesture_func && !g_debug_enable_lcd_sleep_in) {
			/* vout17 3.3v on */
			vcc_cmds_tx(pdev, lcd_vcc_lcdanalog_enable_cmds,
				ARRAY_SIZE(lcd_vcc_lcdanalog_enable_cmds));

			/* 1.8v on */
			gpio_cmds_tx(lcd_gpio_vddio_switch_on_cmds, \
				ARRAY_SIZE(lcd_gpio_vddio_switch_on_cmds));

			/* vsp/vsn on */
			gpio_cmds_tx(lcd_gpio_vsp_n_enable_cmds, \
				ARRAY_SIZE(lcd_gpio_vsp_n_enable_cmds));
		} else {
			HISI_FB_INFO("power on (gesture_func:%d)\n", gesture_func);

			gpio_cmds_tx(lcd_gpio_sleep_request_cmds, \
					ARRAY_SIZE(lcd_gpio_sleep_request_cmds));

			gpio_cmds_tx(lcd_gpio_sleep_normal_cmds, \
					ARRAY_SIZE(lcd_gpio_sleep_normal_cmds));
		}

		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if ((g_lcd_control_tp_power || pinfo->esd_recover_step == LCD_ESD_RECOVER_POWER_ON)
			&& !g_debug_enable_lcd_sleep_in) {
			error = ts_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts resume device err\n");
		}
		synaptics_get_project_id(tp_project_id, sizeof(tp_project_id));
		HISI_FB_INFO("get project id:[%s]\n", tp_project_id);
		if (!strcmp(tp_project_id, "VCTO21161")){
			new_seq_tag = true;
		}

		if (false == gesture_func && !g_debug_enable_lcd_sleep_in) {
			/* time of mipi on to lcd reset on shoud be larger than 10ms */
			msleep(12);

			if (new_seq_tag){
				HISI_FB_INFO("lcd reset new sequence");
				/* lcd reset */
				gpio_cmds_tx(lcd_gpio_reset_new_seq_cmds, \
					ARRAY_SIZE(lcd_gpio_reset_new_seq_cmds));
			}else{
				HISI_FB_INFO("lcd reset old sequence");
				/* lcd reset */
				gpio_cmds_tx(lcd_gpio_reset_cmds, \
					ARRAY_SIZE(lcd_gpio_reset_cmds));
			}
		} else {
			msleep(50);
			HISI_FB_INFO("lp send sequence (gesture_func:%d)\n", gesture_func);
		}

		//update mipi lane numbers
		lane_nums[1] = pinfo->mipi.lane_nums;

		if ((pinfo->bl_set_type & BL_SET_BY_BLPWM) || (pinfo->bl_set_type & BL_SET_BY_SH_BLPWM)) {
			mipi_dsi_cmds_tx(pwm_out_on_cmds, \
				ARRAY_SIZE(pwm_out_on_cmds), mipi_dsi0_base);
		}

		if (new_seq_tag){
			HISI_FB_INFO("lcd display on new sequence");
			// lcd display on sequence
			mipi_dsi_cmds_tx(lcd_display_on_new_seq_cmds, \
				ARRAY_SIZE(lcd_display_on_new_seq_cmds), mipi_dsi0_base);
		}else{
			HISI_FB_INFO("lcd display on old sequence");
			// lcd display on sequence
			mipi_dsi_cmds_tx(lcd_display_on_cmds, \
				ARRAY_SIZE(lcd_display_on_cmds), mipi_dsi0_base);
		}
		g_cabc_mode = 2;

		if (ESD_RECOVER_STATE_START != hisifd->esd_recover_state){
			panel_check_status_and_report_by_dsm(lcd_status_reg, \
				ARRAY_SIZE(lcd_status_reg), mipi_dsi0_base);

			HISI_FB_INFO("recovery:%u, esd_recovery_times:%u\n", lcd_status_reg[0].recovery, esd_recovery_times);
			if (lcd_status_reg[0].recovery && esd_recovery_times < 3){
				HISI_FB_ERR("start recovery!!!\n");
				hisifd->esd_recover_state = ESD_RECOVER_STATE_START;
				if (hisifd->esd_ctrl.esd_check_wq) {
					queue_work(hisifd->esd_ctrl.esd_check_wq, &(hisifd->esd_ctrl.esd_check_work));
				}
				esd_recovery_times++;
			}
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		if ((g_lcd_control_tp_power || pinfo->esd_recover_step == LCD_ESD_RECOVER_POWER_ON)
			&& !g_debug_enable_lcd_sleep_in) {
			error = ts_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts after resume err\n");
		}
	} else {
		HISI_FB_ERR("failed to init lcd!\n");
	}

	/* backlight on */
	hisi_lcd_backlight_on(pdev);

	HISI_FB_INFO("fb%d, -!\n", hisifd->index);

	return 0;
}

static int mipi_sharp_panel_off(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
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

	HISI_FB_INFO("fb%d, +!\n", hisifd->index);

	pinfo = &(hisifd->panel_info);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		LOG_JANK_D(JLID_KERNEL_LCD_POWER_OFF, "%s", "JL_KERNEL_LCD_POWER_OFF");

		/* backlight off */
		hisi_lcd_backlight_off(pdev);

		// lcd display off sequence
		mipi_dsi_cmds_tx(lcd_display_off_cmds, \
			ARRAY_SIZE(lcd_display_off_cmds), hisifd->mipi_dsi0_base);

		if ((g_lcd_control_tp_power || pinfo->esd_recover_step == LCD_ESD_RECOVER_POWER_OFF)
			&& !hisifd->fb_shutdown && !g_debug_enable_lcd_sleep_in) {
			error = ts_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts before suspend err\n");

			error = ts_power_control_notify(TS_SUSPEND_DEVICE, NO_SYNC_TIMEOUT);
			if (error)
				HISI_FB_ERR("ts suspend device err\n");
		}
		if (((false == gesture_func) || hisifd->fb_shutdown) && !g_debug_enable_lcd_sleep_in) {
		}
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (((false == gesture_func) || hisifd->fb_shutdown) && !g_debug_enable_lcd_sleep_in) {
			/* need 5ms between lp11 and reset */
			msleep(5);

			/* lcd pinctrl lowpower */
			pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

			/* lcd reset down */
			gpio_cmds_tx(lcd_gpio_reset_down_cmds, \
				ARRAY_SIZE(lcd_gpio_reset_down_cmds));

			/* vsp/vsn off */
			gpio_cmds_tx(lcd_gpio_vsp_n_disable_cmds, \
				ARRAY_SIZE(lcd_gpio_vsp_n_disable_cmds));

			/* 1.8v off */
			gpio_cmds_tx(lcd_gpio_vddio_switch_off_cmds, \
				ARRAY_SIZE(lcd_gpio_vddio_switch_off_cmds));

			/* vout17 3.3v off */
			vcc_cmds_tx(pdev, lcd_vcc_lcdanalog_disable_cmds,
				ARRAY_SIZE(lcd_vcc_lcdanalog_disable_cmds));
		} else {
			HISI_FB_INFO("display_off (gesture_func:%d)\n", gesture_func);

			/* lcd gpio free */
			gpio_cmds_tx(lcd_gpio_sleep_free_cmds, \
				ARRAY_SIZE(lcd_gpio_sleep_free_cmds));
		}

		if (g_lcd_control_tp_power && hisifd->fb_shutdown) {
			ts_thread_stop_notify();
		}
		checksum_enable_ctl = false;
		esd_recovery_times = 0;
	} else {
		HISI_FB_ERR("failed to uninit lcd!\n");
	}

	HISI_FB_INFO("fb%d, -!\n", hisifd->index);

	return 0;
}

static int mipi_sharp_panel_remove(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return 0;
	}

	hisifd = platform_get_drvdata(pdev);
	if (!hisifd) {
		return 0;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	vcc_cmds_tx(pdev, lcd_vcc_lcdanalog_finit_cmds,
		ARRAY_SIZE(lcd_vcc_lcdanalog_finit_cmds));

	// lcd pinctrl finit
	pinctrl_cmds_tx(pdev, lcd_pinctrl_finit_cmds,
		ARRAY_SIZE(lcd_pinctrl_finit_cmds));

		HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_sharp_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	int ret = 0;
	char __iomem *mipi_dsi0_base = NULL;
	struct hisi_fb_data_type *hisifd = NULL;
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
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
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

		bl_level_adjust[1] = bl_level  * 255 / hisifd->panel_info.bl_max;
		if (last_bl_level != bl_level_adjust[1]){
			last_bl_level = bl_level_adjust[1];
			mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
				ARRAY_SIZE(lcd_bl_level_adjust), mipi_dsi0_base);
		}
	} else {
		HISI_FB_ERR("fb%d, not support this bl_set_type(%d)!\n",
			hisifd->index, hisifd->panel_info.bl_set_type);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}


/******************************************************************************/
static ssize_t mipi_sharp_panel_lcd_model_show(struct platform_device *pdev,
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

	ret = snprintf(buf, PAGE_SIZE, "SHARP_TD4322 5'5 CMD TFT\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_sharp_panel_lcd_cabc_mode_show(struct platform_device *pdev,
	char *buf)
{
	return snprintf(buf, PAGE_SIZE, "%d\n", g_cabc_mode);
}

static ssize_t mipi_sharp_panel_lcd_cabc_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag=-1;
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

	mipi_dsi0_base =hisifd->mipi_dsi0_base;

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
               return ret;

	flag=(int)val;
	if (flag==CABC_OFF ){
              g_cabc_mode=0;
              mipi_dsi_cmds_tx(sharp_cabc_off_cmds, \
                               ARRAY_SIZE(sharp_cabc_off_cmds),\
                               mipi_dsi0_base);
	}else  if (flag==CABC_UI_MODE) {
              g_cabc_mode=1;
              mipi_dsi_cmds_tx(sharp_cabc_ui_on_cmds, \
                               ARRAY_SIZE(sharp_cabc_ui_on_cmds),\
                               mipi_dsi0_base);
	} else if (flag==CABC_STILL_MODE ){
              g_cabc_mode=2;
              mipi_dsi_cmds_tx(sharp_cabc_still_on_cmds, \
                               ARRAY_SIZE(sharp_cabc_still_on_cmds),\
                               mipi_dsi0_base);
	}else if (flag==CABC_MOVING_MODE ){
              g_cabc_mode=3;
              mipi_dsi_cmds_tx(sharp_cabc_moving_on_cmds, \
                               ARRAY_SIZE(sharp_cabc_moving_on_cmds),\
                               mipi_dsi0_base);
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return snprintf((char *)buf, PAGE_SIZE, "%d\n", g_cabc_mode);
}

static ssize_t mipi_sharp_panel_lcd_check_reg_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[4] = {0};
	uint32_t expected_value[4] = {0x9c, 0x00, 0x77, 0x00};
	uint32_t read_mask[4] = {0xFF, 0xFF, 0xFF, 0xFF};
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

static ssize_t mipi_sharp_panel_lcd_mipi_detect_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t read_value[3] = {0};
	uint32_t expected_value[3] = {0x80, 0x00, 0x00};
	uint32_t read_mask[3] = {0xFF, 0xFF, 0xFFFF};
	char* reg_name[3] = {"signal mode", "dsi error number", "MIPI error report"};
	char lcd_reg_0e[] = {0x0e};
	char lcd_reg_05[] = {0x05};
	char lcd_reg_ab[] = {0xab};

	struct dsi_cmd_desc set_read_size[] = {
		{DTYPE_MAX_PKTSIZE, 0, 10, WAIT_TYPE_US,
			0x2, NULL},
	};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0e), lcd_reg_0e},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_05), lcd_reg_05},
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_ab), lcd_reg_ab},
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

static ssize_t mipi_sharp_panel_lcd_hkadc_debug_show(struct platform_device *pdev, char *buf)
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
	ret = snprintf(buf, PAGE_SIZE, "%d\n", hkadc_buf*4);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_sharp_panel_lcd_hkadc_debug_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	int channel = 0;
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

	ret = sscanf(buf, "%u", &channel);
	if(ret <= 0) {
		HISI_FB_ERR("Sscanf return invalid, ret = %d\n", ret);
		return count;
	}

	hkadc_buf = hisi_adc_get_value(channel);
	HISI_FB_INFO("channel[%d] value is %d\n", channel, hkadc_buf);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

static ssize_t mipi_sharp_panel_lcd_gram_check_show(struct platform_device *pdev, char *buf)
{
	ssize_t ret = 0;
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t rd[CHECKSUM_SIZE] = {0};
	int i = 0;
	char cmdF_page0_select[] = {0xFF, 0xF0};
	char cmd1_page0_select[] = {0xFF, 0x10};
	char checksum_read[] = {0x73};
	int checksum_result = 1; //0:success; 1:fail

	struct dsi_cmd_desc packet_size_set_cmd = {DTYPE_MAX_PKTSIZE, 0, 10, WAIT_TYPE_US, 1, NULL};

	struct dsi_cmd_desc lcd_checksum_select_cmds[] = {
		{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmdF_page0_select), cmdF_page0_select},
	};

	struct dsi_cmd_desc lcd_checksum_dis_select_cmds[] = {
		{DTYPE_DCS_WRITE1, 0, 10, WAIT_TYPE_US,
			sizeof(cmd1_page0_select), cmd1_page0_select},
	};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 20, WAIT_TYPE_US,
			sizeof(checksum_read), checksum_read},
	};

	if (!checksum_enable_ctl) {
		HISI_FB_INFO("Checksum disabled\n");
		return ret;
	}

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_INFO("fb%d, +.\n", hisifd->index);

	mipi_dsi_max_return_packet_size(&packet_size_set_cmd, mipi_dsi0_base);

	mipi_dsi_cmds_tx(lcd_checksum_select_cmds, \
		ARRAY_SIZE(lcd_checksum_select_cmds), mipi_dsi0_base);
	for (i = 0; i < CHECKSUM_SIZE; i++) {
		char *data = lcd_check_reg[0].payload;
		*data = 0x73 + i;
		mipi_dsi_cmds_rx((rd + i), lcd_check_reg, \
			ARRAY_SIZE(lcd_check_reg), mipi_dsi0_base);
		if (rd[i] != expected_checksum[checksum_pic_n][i]) {
			HISI_FB_ERR("rd[%d] = 0x%x, pic_num=%d, expected = 0x%x\n", i, rd[i], checksum_pic_n,
				expected_checksum[checksum_pic_n][i]);
			checksum_result = 1;
			break;
		}
		checksum_result = 0;
	}

	ret = snprintf(buf, PAGE_SIZE, "%d\n", checksum_result);

	HISI_FB_INFO("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x, pic_num=%d\n", \
			rd[0], rd[1], rd[2], rd[3], rd[4], rd[5], rd[6], rd[7], checksum_pic_n);

	mipi_dsi_cmds_tx(lcd_checksum_dis_select_cmds, \
		ARRAY_SIZE(lcd_checksum_dis_select_cmds), mipi_dsi0_base);

	HISI_FB_INFO("fb%d, -.\n", hisifd->index);

	return ret;
}

static int bist_read_and_check(struct platform_device *pdev, int step)
{
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	u32 D8_value = 0xFF, D9_value = 0xFF, BC_value = 0xFF;
	int ret = LCD_BIST_CHECK_PASS;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}
	mipi_dsi0_base = hisifd->dss_base + DSS_MIPI_DSI0_OFFSET;

	outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0xD806);
	if (!mipi_dsi_read(&D8_value, mipi_dsi0_base)) {
		HISI_FB_ERR("LCD bist check read register D8 timeout\n");
		ret = LCD_BIST_CHECK_TIMEOUT;
		goto read_reg_failed;
	}

	outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0xD906);
	if (!mipi_dsi_read(&D9_value, mipi_dsi0_base)) {
		HISI_FB_ERR("LCD bist check read register D9 timeout\n");
		ret = LCD_BIST_CHECK_TIMEOUT;
		goto read_reg_failed;
	}

	outp32(mipi_dsi0_base + MIPIDSI_GEN_HDR_OFFSET, 0xBC06);
	if (!mipi_dsi_read(&BC_value, mipi_dsi0_base)) {
		HISI_FB_ERR("LCD bist check read register BC timeout\n");
		ret = LCD_BIST_CHECK_TIMEOUT;
		goto read_reg_failed;
	}

	HISI_FB_INFO("D8:%x, D9:%x, BC:%x\n", D8_value, D9_value, BC_value);

	if (step <= 7) {
		D8_value &= 0xFF;
		D9_value &= 0xFF;
		BC_value &= 0x60;
		ret = ((D8_value == 0x3F) && (D9_value == 0x00) && (BC_value == 0x60)) ? \
					LCD_BIST_CHECK_PASS : LCD_BIST_CHECK_FAIL;
	}

	if (step == 8) {
		BC_value &= 0x60;
		ret = (BC_value == 0x60) ? \
					LCD_BIST_CHECK_PASS : LCD_BIST_CHECK_FAIL;
	}

read_reg_failed:
	return ret;
}

static int sram_test_feedback = 0;
static irqreturn_t mipi_sharp_panel_lcd_bist_check_isr(int irq, void *ptr)
{
	struct hisi_fb_data_type *hisifd = NULL;

	hisifd = (struct hisi_fb_data_type *)ptr;
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	sram_test_feedback = 1;
	HISI_FB_INFO("Get a te interrupt\n");

	return IRQ_HANDLED;
}

static ssize_t mipi_sharp_panel_lcd_bist_check_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	struct hisi_panel_info *pinfo = NULL;
	struct dss_rect disp_rect = {0};
	ssize_t ret, error, error_pw = 0;
	uint32_t te0_vs_wd = 0;
	int irq_id = 0;
	int i = 0;
	int x, y = 0;
	int val = 0;
	int sent_pixels = 0;
	int remain_pixels = PATTERN_PIXELS_X_SIZE * PATTERN_PIXELS_Y_SIZE;
	char lcd_pattern_packet[PACKET_SIZE] = {0};

	disp_rect.w = PATTERN_PIXELS_X_SIZE;
	disp_rect.h = PATTERN_PIXELS_Y_SIZE;

	struct dsi_cmd_desc lcd_write_pattern[] = {
		{DTYPE_DCS_LWRITE, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_pattern_packet), lcd_pattern_packet},
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

	if (NULL == hisifd->dss_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->dss_base + DSS_MIPI_DSI0_OFFSET;
	hisifd->lcd_self_testing = true;

	error = ts_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);
	error_pw = ts_power_control_notify(TS_SUSPEND_DEVICE, SHORT_SYNC_TIMEOUT);
	HISI_FB_INFO("Disable touchscreen during test.\n");

	// lcd display off sequence
	mipi_dsi_cmds_tx(lcd_display_off_cmds, \
		ARRAY_SIZE(lcd_display_off_cmds), mipi_dsi0_base);

	mipi_sharp_panel_set_display_region(pdev, &disp_rect);

	mipi_dsi_cmds_tx(sram_test_pre_cmds, \
		ARRAY_SIZE(sram_test_pre_cmds), mipi_dsi0_base);

	mdelay(3);

	while (remain_pixels > 0) {
		lcd_pattern_packet[0] = (sent_pixels == 0) ? 0x2c : 0x3c;
		for (i = 0; i < PACKET_PIXELS_SIZE; i++) {
			y = (sent_pixels + i)/PATTERN_PIXELS_X_SIZE;
			x = (sent_pixels + i)%PATTERN_PIXELS_X_SIZE;
			val = y % 4;
			val = (val == 0 || val == 1) ? 0x55 : 0xAA;
			lcd_pattern_packet[i * 3 + 1] = val;
			lcd_pattern_packet[i * 3 + 2] = val;
			lcd_pattern_packet[i * 3 + 3] = val;
		}

/* Fail test
		if (sent_pixels == 0) {
			lcd_pattern_packet[1] = 0xff;
			lcd_pattern_packet[2] = 0xff;
			lcd_pattern_packet[3] = 0xff;
		}
*/

		mipi_dsi_cmds_tx(lcd_write_pattern, \
			ARRAY_SIZE(lcd_write_pattern), mipi_dsi0_base);

		remain_pixels -= PACKET_PIXELS_SIZE;
		sent_pixels += PACKET_PIXELS_SIZE;
	}

	sram_test_feedback = 0;

	// lcd pinctrl lowpower
	pinctrl_cmds_tx(pdev, lcd_pinctrl_lowpower_cmds,
		ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

	ret = gpio_request(GPIO_TE0, "gpio_te0");
	if(ret < 0){
		HISI_FB_INFO("could not reuquest gpio-%d \n",  GPIO_TE0);
		goto err_gpio_request;
	}

	gpio_direction_input(GPIO_TE0);
	irq_id = gpio_to_irq(GPIO_TE0);
	ret = request_irq(irq_id, mipi_sharp_panel_lcd_bist_check_isr, IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND, "gpio_te0_irq", (void *)hisifd);
	if (ret) {
		HISI_FB_INFO("gpio irq register failed\n");
		goto err_irq_request;
	}

	mipi_dsi_cmds_tx(sram_test_post_cmds, \
		ARRAY_SIZE(sram_test_post_cmds), mipi_dsi0_base);

	mdelay(5);

	free_irq(irq_id, (void *)hisifd);

err_irq_request:
	gpio_free(GPIO_TE0);

err_gpio_request:
	if (sram_test_feedback == 0)
		ret = snprintf(buf, PAGE_SIZE, "pass\n");
	else
		ret = snprintf(buf, PAGE_SIZE, "fail\n");

	pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
		ARRAY_SIZE(lcd_pinctrl_normal_cmds));

	hisifd->off_fnc(hisifd);

	mdelay(500);

	hisifd->on_fnc(hisifd);

	if (!error_pw)
		error_pw = ts_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);
	if (!error)
		error = ts_power_control_notify(TS_AFTER_RESUME, SHORT_SYNC_TIMEOUT);
	HISI_FB_INFO("Enable touchscreen after test.\n");

	mipi_sharp_panel_set_backlight(pdev, hisifd->bl_level);
	hisifd->lcd_self_testing = false;

	return ret;
}

static ssize_t mipi_sharp_panel_lcd_sleep_ctrl_show(struct platform_device *pdev, char *buf)
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
	ret = snprintf(buf, PAGE_SIZE, "enable_lcd_sleep_in=%d, pinfo->lcd_adjust_support=%d\n",
		g_debug_enable_lcd_sleep_in, hisifd->panel_info.lcd_adjust_support);
	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_sharp_panel_lcd_sleep_ctrl_store(struct platform_device *pdev, char *buf)
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
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}


	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (hisifd->panel_info.lcd_adjust_support) {
		g_debug_enable_lcd_sleep_in = val;
	}

	if (g_debug_enable_lcd_sleep_in == 2) {
		HISI_FB_INFO("LCD power off and Touch goto sleep\n");
		g_tp_power_ctrl = 1;	//used for pt  current test, tp sleep
	} else {
		HISI_FB_INFO("g_debug_enable_lcd_sleep_in is %d\n", g_debug_enable_lcd_sleep_in);
		g_tp_power_ctrl = 0;	//used for pt  current test, tp power off
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

/*for esd check*/
static int mipi_sharp_panel_check_esd(struct platform_device* pdev)
{
	int ret = 0, dsm_ret = 0;
	struct hisi_fb_data_type* hisifd = NULL;
	uint32_t read_value[1] = {0};
	uint32_t expected_value[1] = {0x1c};
	uint32_t read_mask[1] = {0x1c};
	char* reg_name[1] = {"power mode"};
	char lcd_reg_0a[] = {0x0a};

	struct dsi_cmd_desc lcd_check_reg[] = {
		{DTYPE_DCS_READ, 0, 10, WAIT_TYPE_US,
			sizeof(lcd_reg_0a), lcd_reg_0a},
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

	if (NULL == pdev) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}
	hisifd = (struct hisi_fb_data_type*)platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	ret = mipi_dsi_read_compare(&data, hisifd->mipi_dsi0_base);
	if (ret) {
		HISI_FB_ERR("ESD ERROR:ret = %d\n", ret);
		dsm_ret = dsm_client_ocuppy(lcd_dclient);
		if ( !dsm_ret ) {
			dsm_client_record(lcd_dclient, "ESD ERROR:ret = %d\n", ret);
			dsm_client_notify(lcd_dclient, DSM_LCD_ESD_RECOVERY_NO);
		}else{
			HISI_FB_ERR("dsm_client_ocuppy ERROR:retVal = %d\n", ret);
		}
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
	{DTYPE_DCS_LWRITE, 0, 15, WAIT_TYPE_US,
		sizeof(lcd_disp_x), lcd_disp_x},
	{DTYPE_DCS_LWRITE, 0, 15, WAIT_TYPE_US,
		sizeof(lcd_disp_y), lcd_disp_y},
};

#define LCD_CMD_NAME_MAX 100
static char lcd_cmd_now[LCD_CMD_NAME_MAX] = {0};
static ssize_t mipi_sharp_panel_lcd_test_config_show(struct platform_device *pdev,
	char *buf)
{
	if (!strncmp(lcd_cmd_now, "RUNNINGTEST10", strlen("RUNNINGTEST10"))) {
		return snprintf(buf, PAGE_SIZE, "BL_OPEN_SHORT");
	} else if (!strncmp(lcd_cmd_now, "BL_OPEN_SHORT", strlen("BL_OPEN_SHORT"))) {
		return snprintf(buf, PAGE_SIZE, "/sys/class/lm36923/lm36923/self_test");

	} else if (!strncmp(lcd_cmd_now, "RUNNINGTEST0", strlen("RUNNINGTEST0"))) {
		return snprintf(buf, PAGE_SIZE, "CURRENT1_0");
	} else if (!strncmp(lcd_cmd_now, "RUNNINGTEST1", strlen("RUNNINGTEST1"))) {
		return snprintf(buf, PAGE_SIZE, "CURRENT1_1");
	} else if (!strncmp(lcd_cmd_now, "RUNNINGTEST2", strlen("RUNNINGTEST2"))) {
		return snprintf(buf, PAGE_SIZE, "CURRENT1_2");
	} else if (!strncmp(lcd_cmd_now, "RUNNINGTEST3", strlen("RUNNINGTEST3"))) {
		return snprintf(buf, PAGE_SIZE, "CURRENT1_3");
	} else if (!strncmp(lcd_cmd_now, "RUNNINGTEST4", strlen("RUNNINGTEST4"))) {
		return snprintf(buf, PAGE_SIZE, "CURRENT1_4");
	} else if (!strncmp(lcd_cmd_now, "RUNNINGTEST5", strlen("RUNNINGTEST5"))) {
		return snprintf(buf, PAGE_SIZE, "CURRENT1_5");

	/* lcd support over current and over voltage detect */
	} else if (!strncmp(lcd_cmd_now, "CURRENT_TEST_MODE", strlen("CURRENT_TEST_MODE"))) {
		return snprintf(buf, PAGE_SIZE, "LCD_CUR_DET_MODE");
	/* over current detect */
	} else if (!strncmp(lcd_cmd_now, "OVER_CURRENT_DETECTION", strlen("OVER_CURRENT_DETECTION"))) {
		return snprintf(buf, PAGE_SIZE, "0");
	/* over voltage detect */
	} else if (!strncmp(lcd_cmd_now, "OVER_VOLTAGE_DETECTION", strlen("OVER_VOLTAGE_DETECTION"))) {
		return snprintf(buf, PAGE_SIZE, "0");

	} else {
		return snprintf(buf, PAGE_SIZE, "INVALID");
	}
}

static ssize_t mipi_sharp_panel_lcd_test_config_store(struct platform_device *pdev,
	const char *buf, size_t count)
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

	if (NULL == hisifd->mipi_dsi0_base) {
		HISI_FB_ERR("NULL Pointer\n");
		return -EINVAL;
	}

	mipi_dsi0_base =hisifd->mipi_dsi0_base;

	if (strlen(buf) < LCD_CMD_NAME_MAX) {
		memcpy(lcd_cmd_now, buf, strlen(buf) + 1);
		HISI_FB_INFO("current test cmd:%s\n", lcd_cmd_now);
	} else {
		memcpy(lcd_cmd_now, "INVALID", strlen("INVALID") + 1);
		HISI_FB_INFO("invalid test cmd\n");
	}

	return count;
}

static int mipi_sharp_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct hisi_fb_data_type *hisifd = NULL;

	if (NULL == pdev || NULL == dirty) {
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

	lcd_disp_x[1] = (dirty->x >> 8) & 0xff;
	lcd_disp_x[2] = dirty->x & 0xff;
	lcd_disp_x[3] = ((dirty->x + dirty->w - 1) >> 8) & 0xff;
	lcd_disp_x[4] = (dirty->x + dirty->w - 1) & 0xff;
	lcd_disp_y[1] = (dirty->y >> 8) & 0xff;
	lcd_disp_y[2] = dirty->y & 0xff;
	lcd_disp_y[3] = ((dirty->y + dirty->h - 1) >> 8) & 0xff;
	lcd_disp_y[4] = (dirty->y + dirty->h - 1) & 0xff;

	mipi_dsi_cmds_tx(set_display_address, \
		ARRAY_SIZE(set_display_address), hisifd->mipi_dsi0_base);

	return 0;
}

static int g_support_mode = 0;
static ssize_t mipi_sharp_panel_lcd_support_mode_show(struct platform_device *pdev,
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

	ret = snprintf(buf, PAGE_SIZE, "%d\n", g_support_mode);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_sharp_panel_lcd_support_mode_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	int ret = 0;
	unsigned long val = 0;
	int flag = -1;
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

	ret = strict_strtoul(buf, 0, &val);
	if (ret)
               return ret;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	flag = (int)val;

	g_support_mode = flag;

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return snprintf((char *)buf, PAGE_SIZE, "%d\n", g_support_mode);
}

static ssize_t mipi_sharp_panel_lcd_support_checkmode_show(struct platform_device *pdev,
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

	ret = snprintf(buf, PAGE_SIZE, "bl_open_short:1;bist:1\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_sharp_panel_sharpness2d_table_store(struct platform_device *pdev,
	const char *buf, size_t count)
{
	struct hisi_fb_data_type *hisifd = NULL;
	sharp2d_t* sharp2d;
	int i = 0;
	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (hisifd->panel_info.sharp2d_table && buf) {
		sharp2d = (sharp2d_t*)buf;
		memcpy((void *)hisifd->panel_info.sharp2d_table, (void *)sharp2d, sizeof(sharp2d_t));

		for (i = 0; i < sizeof(sharp2d_t)/sizeof(uint32_t); i++) {
			HISI_FB_DEBUG("sharp2d_table[%d] = 0x%x \n", i, ((uint32_t*)(hisifd->panel_info.sharp2d_table))[i]);
		}
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return count;
}

static ssize_t mipi_sharp_panel_sharpness2d_table_show(struct platform_device *pdev, char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;
	int i = 0;
	int buf_len = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev is NULL");
		return -EINVAL;
	}
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd is NULL");
		return -EINVAL;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (hisifd->panel_info.sharp2d_table && buf) {
		for (i = 0; i < sizeof(sharp2d_t)/sizeof(uint32_t); i++) {
			ret = snprintf(buf + buf_len, PAGE_SIZE, "sharp2d_table[%d] = 0x%x\n", i, ((uint32_t*)hisifd->panel_info.sharp2d_table)[i]);
			if (ret < 0) break;
			buf_len += ret;
		}
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return buf_len;
}

static ssize_t mipi_lcd_panel_info_show(struct platform_device *pdev,
	char *buf)
{
	struct hisi_fb_data_type *hisifd = NULL;
	ssize_t ret = 0;
	int bl_max_nit = 0;

	if (NULL == pdev) {
		HISI_FB_ERR("pdev NULL pointer\n");
		return 0;
	};
	hisifd = platform_get_drvdata(pdev);
	if (NULL == hisifd) {
		HISI_FB_ERR("hisifd NULL pointer\n");
		return 0;
	}

	if(g_lcd_brightness_info){
	        bl_max_nit = g_lcd_brightness_info;
	} else {
	        bl_max_nit = DISPLAY_BL_MAX_NIT_STANDARD;
	}

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);
	if (buf) {
		ret = snprintf(buf, PAGE_SIZE, "blmax:%u,blmin:%u,blmax_nit_actual:%d,blmax_nit_standard:%d,lcdtype:%s,\n",
				hisifd->panel_info.bl_max, hisifd->panel_info.bl_min, bl_max_nit, DISPLAY_BL_MAX_NIT_STANDARD, "LCD");
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static struct hisi_panel_info g_panel_info = {0};
static struct hisi_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_sharp_panel_set_fastboot,
	.on = mipi_sharp_panel_on,
	.off = mipi_sharp_panel_off,
	.remove = mipi_sharp_panel_remove,
	.set_backlight = mipi_sharp_panel_set_backlight,
	.lcd_model_show = mipi_sharp_panel_lcd_model_show,
	.lcd_sleep_ctrl_show = mipi_sharp_panel_lcd_sleep_ctrl_show,
	.lcd_sleep_ctrl_store = mipi_sharp_panel_lcd_sleep_ctrl_store,
	.panel_info_show = mipi_lcd_panel_info_show,
	.lcd_support_checkmode_show = mipi_sharp_panel_lcd_support_checkmode_show,
	.lcd_test_config_show = mipi_sharp_panel_lcd_test_config_show,
	.lcd_test_config_store = mipi_sharp_panel_lcd_test_config_store,
	.lcd_bist_check = mipi_sharp_panel_lcd_bist_check_show,
	.lcd_support_mode_show = mipi_sharp_panel_lcd_support_mode_show,
	.lcd_support_mode_store = mipi_sharp_panel_lcd_support_mode_store,
	.esd_handle = mipi_sharp_panel_check_esd,
	.set_display_region = mipi_sharp_panel_set_display_region,
};

static int mipi_sharp_get_dts_config(void){
	struct device_node *np = NULL;
	int ret = 0;
	const char *bl_ic_name;

	HISI_FB_INFO("%s +\n", __func__);
	memset(&g_dev_info, 0, sizeof(struct jdi_td4322_device));

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_SHARP_TD4322_5P5);
	if (!np) {
		HISI_FB_ERR("not found device node %s!\n", DTS_COMP_SHARP_TD4322_5P5);
		return JDI_TD4322_ERR;
	}

	//"lcd-bl-type"
	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &g_dev_info.bl_type);
	if (ret) {
		HISI_FB_ERR("get lcd_bl_type failed!\n");
		g_dev_info.bl_type = BL_SET_BY_MIPI;
	}

	//"lcd-bl-ic-name"
	ret = of_property_read_string_index(np, "lcd-bl-ic-name", 0, &bl_ic_name);
	if (ret) {
		snprintf(g_dev_info.bl_ic_name_buf, sizeof(g_dev_info.bl_ic_name_buf), "INVALID");
	} else {
		snprintf(g_dev_info.bl_ic_name_buf, sizeof(g_dev_info.bl_ic_name_buf), "%s", bl_ic_name);
	}

	ret = of_property_read_u32(np,  "support-dynamic-gamma", &g_dev_info.lcd_support_dynamic_gamma);
	if (ret) {
		g_dev_info.lcd_support_dynamic_gamma = 0;
	}
	HISI_FB_INFO("lcd_support_dynamic_gamma = %d\n", g_dev_info.lcd_support_dynamic_gamma);

	//gpio 037
	g_dev_info.gpio_n5v5_enable = of_get_named_gpio(np, "gpios", 0);
	// gpio 125
	g_dev_info.gpio_p5v5_enable = of_get_named_gpio(np, "gpios", 2);
	//gpio 40
	g_dev_info.gpio_reset = of_get_named_gpio(np, "gpios", 3);
	//gpio 46
	g_dev_info.gpio_id0 = of_get_named_gpio(np, "gpios", 4);
	/* vddio switch gpio 67 */
	g_dev_info.gpio_vddio_ctrl = of_get_named_gpio(np, "gpios", 6);

	HISI_FB_INFO("%s: g_dev_info.bl_ic_name_buf:%s\n", __func__, g_dev_info.bl_ic_name_buf);
	HISI_FB_INFO("%s: g_dev_info.bl_type:%u\n", __func__, g_dev_info.bl_type);

	HISI_FB_INFO("%s: g_dev_info.gpio_id0:%u\n", __func__, g_dev_info.gpio_id0);
	HISI_FB_INFO("%s: g_dev_info.gpio_vddio_ctrl:%u\n", __func__, g_dev_info.gpio_vddio_ctrl);
	HISI_FB_INFO("%s: g_dev_info.gpio_n5v5_enable:%u\n", __func__, g_dev_info.gpio_n5v5_enable);
	HISI_FB_INFO("%s: g_dev_info.gpio_p5v5_enable:%u\n", __func__, g_dev_info.gpio_p5v5_enable);
	HISI_FB_INFO("%s: g_dev_info.gpio_reset:%u\n", __func__, g_dev_info.gpio_reset);
	HISI_FB_INFO("%s: g_dev_info.lcd_support_dynamic_gamma:%u\n", __func__, g_dev_info.lcd_support_dynamic_gamma);

	HISI_FB_INFO("%s -\n", __func__);
	return JDI_TD4322_OK;
}

/*******************************************************************************
**
*/
static int mipi_sharp_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct hisi_panel_info *pinfo = NULL;
	struct device_node *np = NULL;

	g_lcd_control_tp_power = true;
	if (mipi_sharp_get_dts_config())
		goto err_return;

	if (hisi_fb_device_probe_defer(PANEL_MIPI_CMD, g_dev_info.bl_type))
		goto err_probe_defer;

	HISI_FB_DEBUG("+.\n");

	HISI_FB_INFO("%s\n", DTS_COMP_SHARP_TD4322_5P5);

	pdev->id = 1;
	// init lcd panel info
	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct hisi_panel_info));
	pinfo->xres = 1080;
	pinfo->yres = 1920;
	pinfo->width = 64;
	pinfo->height = 113;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = g_dev_info.bl_type;

	if (pinfo->bl_set_type == BL_SET_BY_BLPWM)
		pinfo->blpwm_input_ena = 0;

	if (!strncmp(g_dev_info.bl_ic_name_buf, "LM36923YFFR", strlen("LM36923YFFR"))) {
		pinfo->bl_min = 33;
		/* 10000stage 7992,2048stage 1973 for 450nit */
		pinfo->bl_max = 8820;
		pinfo->bl_default = 4000;
		pinfo->blpwm_precision_type = BLPWM_PRECISION_2048_TYPE;
		pinfo->bl_ic_ctrl_mode = REG_ONLY_MODE;
	} else {
		pinfo->bl_min = 33;
		/* 10000stage 8710,2048stage 1973 for 450nit */
		pinfo->bl_max = 8820;
		pinfo->bl_default = 4000;
		pinfo->blpwm_precision_type = BLPWM_PRECISION_2048_TYPE;
		pinfo->bl_ic_ctrl_mode = REG_ONLY_MODE;
	}
	pinfo->type = PANEL_MIPI_CMD;
	pinfo->ifbc_type = IFBC_TYPE_NONE;

	pinfo->frc_enable = 0;
	pinfo->esd_enable = 1;
	pinfo->esd_skip_mipi_check = 1;
	g_esd_recover_disable = 1;
	pinfo->lcd_uninit_step_support = 1;
	pinfo->lcd_adjust_support = 1;

	pinfo->color_temperature_support = 1;
	if(color_temp_cal_buf[0] == 1){
		pinfo->color_temp_rectify_support = 0;
	}else{
		pinfo->color_temp_rectify_support =0;
	}

	pinfo->color_temp_rectify_R = 32768; /*100 percent*/
	pinfo->color_temp_rectify_G = 32768; /*100 percent*/
	pinfo->color_temp_rectify_B = 32768; /*100 percent*/

	pinfo->comform_mode_support = 1;
	if(pinfo->comform_mode_support == 1){
		g_support_mode |= COMFORM_MODE;
	}

	g_led_rg_para1 = 7;
	g_led_rg_para2 = 30983;
	g_support_mode |= LED_RG_COLOR_TEMP_MODE;

	//prefix ce & sharpness
	pinfo->prefix_ce_support = 0;
	pinfo->prefix_sharpness1D_support = 0;
	pinfo->arsr1p_sharpness_support = 1;
	pinfo->prefix_sharpness2D_support = 1;
	pinfo->sharp2d_table = &sharp2d_table;

	//sbl
	pinfo->sbl_support = 0;
	pinfo->smart_bl.strength_limit = 128;
	pinfo->smart_bl.calibration_a = 25;
	pinfo->smart_bl.calibration_b = 95;
	pinfo->smart_bl.calibration_c = 3;
	pinfo->smart_bl.calibration_d = 0;
	pinfo->smart_bl.t_filter_control = 5;
	pinfo->smart_bl.backlight_min = 480;
	pinfo->smart_bl.backlight_max = 4096;
	pinfo->smart_bl.backlight_scale = 0xff;
	pinfo->smart_bl.ambient_light_min = 14;
	pinfo->smart_bl.filter_a = 1738;
	pinfo->smart_bl.filter_b = 6;
	pinfo->smart_bl.logo_left = 0;
	pinfo->smart_bl.logo_top = 0;
	pinfo->smart_bl.variance_intensity_space = 145;
	pinfo->smart_bl.slope_max = 54;
	pinfo->smart_bl.slope_min = 160;

	pinfo->cinema_mode_support = 0;

	//init ACM info
	pinfo->acm_support = 0;
	if (pinfo->acm_support == 1) {
		pinfo->acm_lut_hue_table = acm_lut_hue_table;
		pinfo->acm_lut_hue_table_len = sizeof(acm_lut_hue_table) / sizeof(acm_lut_hue_table[0]);
		pinfo->acm_lut_sata_table = acm_lut_sata_table;
		pinfo->acm_lut_sata_table_len = sizeof(acm_lut_sata_table) / sizeof(acm_lut_sata_table[0]);
		pinfo->acm_lut_satr0_table = acm_lut_satr0_table;
		pinfo->acm_lut_satr0_table_len = sizeof(acm_lut_satr0_table) / sizeof(acm_lut_satr0_table[0]);
		pinfo->acm_lut_satr1_table = acm_lut_satr1_table;
		pinfo->acm_lut_satr1_table_len = sizeof(acm_lut_satr1_table) / sizeof(acm_lut_satr1_table[0]);
		pinfo->acm_lut_satr2_table = acm_lut_satr2_table;
		pinfo->acm_lut_satr2_table_len = sizeof(acm_lut_satr2_table) / sizeof(acm_lut_satr2_table[0]);
		pinfo->acm_lut_satr3_table = acm_lut_satr3_table;
		pinfo->acm_lut_satr3_table_len = sizeof(acm_lut_satr3_table) / sizeof(acm_lut_satr3_table[0]);
		pinfo->acm_lut_satr4_table = acm_lut_satr4_table;
		pinfo->acm_lut_satr4_table_len = sizeof(acm_lut_satr4_table) / sizeof(acm_lut_satr4_table[0]);
		pinfo->acm_lut_satr5_table = acm_lut_satr5_table;
		pinfo->acm_lut_satr5_table_len = sizeof(acm_lut_satr5_table) / sizeof(acm_lut_satr5_table[0]);
		pinfo->acm_lut_satr6_table = acm_lut_satr6_table;
		pinfo->acm_lut_satr6_table_len = sizeof(acm_lut_satr6_table) / sizeof(acm_lut_satr6_table[0]);
		pinfo->acm_lut_satr7_table = acm_lut_satr7_table;
		pinfo->acm_lut_satr7_table_len = sizeof(acm_lut_satr7_table) / sizeof(acm_lut_satr7_table[0]);
		pinfo->r0_hh = 0x80;
		pinfo->r0_lh = 0x0;
		pinfo->r1_hh = 0xff;
		pinfo->r1_lh = 0x81;
		pinfo->r2_hh = 0x17f;
		pinfo->r2_lh = 0x100;
		pinfo->r3_hh = 0x1ff;
		pinfo->r3_lh = 0x180;
		pinfo->r4_hh = 0x281;
		pinfo->r4_lh = 0x200;
		pinfo->r5_hh = 0x2ff;
		pinfo->r5_lh = 0x282;
		pinfo->r6_hh = 0x380;
		pinfo->r6_lh = 0x300;

		//ACM_CE
		pinfo->acm_ce_support = 1;
	}

	// Contrast Algorithm
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

	//Gama LCP
	pinfo->gamma_support = 1;
	if (pinfo->gamma_support == 1) {
		if(g_dev_info.lcd_support_dynamic_gamma ==1){
			hisifb_update_gm_from_reserved_mem(gamma_lut_table_R,
				gamma_lut_table_G,
				gamma_lut_table_B,
				igm_lut_table_R,
				igm_lut_table_G,
				igm_lut_table_B);
		}
		pinfo->igm_lut_table_R = igm_lut_table_R;
		pinfo->igm_lut_table_G = igm_lut_table_G;
		pinfo->igm_lut_table_B = igm_lut_table_B;
		pinfo->igm_lut_table_len = ARRAY_SIZE(igm_lut_table_R);

		pinfo->gamma_lut_table_R = gamma_lut_table_R;
		pinfo->gamma_lut_table_G = gamma_lut_table_G;
		pinfo->gamma_lut_table_B = gamma_lut_table_B;
		pinfo->gamma_lut_table_len = ARRAY_SIZE(gamma_lut_table_R);

		pinfo->xcc_support = 1;
		pinfo->xcc_table = xcc_table;
		pinfo->xcc_table_len = ARRAY_SIZE(xcc_table);

		pinfo->gmp_support = 1;
		pinfo->gmp_lut_table_low32bit = &gmp_lut_table_low32bit[0][0][0];
		pinfo->gmp_lut_table_high4bit = &gmp_lut_table_high4bit[0][0][0];
		pinfo->gmp_lut_table_len = ARRAY_SIZE(gmp_lut_table_low32bit);
	}

	// hiace
	pinfo->hiace_support = 1;
	if (pinfo->hiace_support == 1) {
		pinfo->hiace_param.iGlobalHistBlackPos = 16;
		pinfo->hiace_param.iGlobalHistWhitePos = 240;
		pinfo->hiace_param.iGlobalHistBlackWeight = 51;
		pinfo->hiace_param.iGlobalHistWhiteWeight = 51;
		pinfo->hiace_param.iGlobalHistZeroCutRatio = 486;
		pinfo->hiace_param.iGlobalHistSlopeCutRatio = 410;
		pinfo->hiace_param.iMaxLcdLuminance = 500;
		pinfo->hiace_param.iMinLcdLuminance = 3;
		strncpy(pinfo->hiace_param.chCfgName, "/product/etc/display/effect/algorithm/hdr_engine.xml", sizeof(pinfo->hiace_param.chCfgName) - 1);
	}

	//ldi
	pinfo->ldi.h_back_porch = 23;//50;
	pinfo->ldi.h_front_porch = 50;//132;
	pinfo->ldi.h_pulse_width = 20;
	pinfo->ldi.v_back_porch = 28;
	pinfo->ldi.v_front_porch = 14;
	pinfo->ldi.v_pulse_width = 8;

	//mipi
	pinfo->mipi.dsi_bit_clk = 490;
	pinfo->mipi.dsi_bit_clk_val1 = 471;
	pinfo->mipi.dsi_bit_clk_val2 = 500;
	pinfo->mipi.dsi_bit_clk_val3 = 490;
	pinfo->mipi.dsi_bit_clk_val4 = 500;
	//pinfo->mipi.dsi_bit_clk_val5 = ;
	pinfo->dsi_bit_clk_upt_support = 1;
	pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;

	//non_continue adjust : measured in UI
	//LG requires clk_post >= 60ns + 252ui, so need a clk_post_adjust more than 200ui. Here 215 is used.
	pinfo->mipi.clk_post_adjust = 215;
	pinfo->mipi.clk_pre_adjust= 0;
	pinfo->mipi.clk_t_hs_prepare_adjust= 0;
	pinfo->mipi.clk_t_lpx_adjust= 0;
	pinfo->mipi.clk_t_hs_trial_adjust= 0;
	pinfo->mipi.clk_t_hs_exit_adjust= 0;
	pinfo->mipi.clk_t_hs_zero_adjust= 0;
	pinfo->mipi.non_continue_en = 1;

	pinfo->pxl_clk_rate = 146 * 1000000UL;

	//mipi
	pinfo->mipi.lane_nums = DSI_4_LANES;
	//pinfo->mipi.lane_nums_select_support = DSI_1_LANES_SUPPORT | DSI_2_LANES_SUPPORT |
	//	DSI_3_LANES_SUPPORT | DSI_4_LANES_SUPPORT;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = 0;

	pinfo->pxl_clk_rate_div = 1;

	pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS  | VSYNC_CTRL_CLK_OFF;
	pinfo->dirty_region_updt_support = 0;
	pinfo->dirty_region_info.left_align = 16;
	pinfo->dirty_region_info.right_align = 16;
	pinfo->dirty_region_info.top_align = 2;
	pinfo->dirty_region_info.bottom_align = 2;
	pinfo->dirty_region_info.w_align = -1;
	pinfo->dirty_region_info.h_align = -1;
	pinfo->dirty_region_info.w_min = 912;
	pinfo->dirty_region_info.h_min = 6;
	pinfo->dirty_region_info.top_start = -1;
	pinfo->dirty_region_info.bottom_start = -1;

	if(runmode_is_factory()) {
		HISI_FB_INFO("Factory mode, disable features: dirty update etc.\n");
		pinfo->dirty_region_updt_support = 0;
		pinfo->prefix_ce_support = 0;
		pinfo->prefix_sharpness1D_support = 0;
		pinfo->arsr1p_sharpness_support = 0;
		pinfo->prefix_sharpness2D_support = 0;
		pinfo->sbl_support = 0;
		pinfo->acm_support = 0;
		pinfo->acm_ce_support = 0;
		pinfo->hiace_support = 0;
		pinfo->esd_enable = 0;
		pinfo->ifbc_type = IFBC_TYPE_NONE;
		pinfo->blpwm_input_ena = 0;
		pinfo->blpwm_precision_type = BLPWM_PRECISION_DEFAULT_TYPE;
		pinfo->comform_mode_support = 0;
		g_support_mode = 0;
		pinfo->color_temp_rectify_support = 0;
	}

	//The host processor must wait for more than 15us from the end of write data transfer to a command 2Ah/2Bh
	if (pinfo->dirty_region_updt_support == 1)
		pinfo->mipi.hs_wr_to_time = 17000;        // measured in nS

	if (pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE) {
		pinfo->pxl_clk_rate_div = 3;

		//ldi
		pinfo->ldi.h_back_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_front_porch /= pinfo->pxl_clk_rate_div;
		pinfo->ldi.h_pulse_width /= pinfo->pxl_clk_rate_div;

		//dsc parameter info
		pinfo->vesa_dsc.bits_per_component = 8;
		pinfo->vesa_dsc.bits_per_pixel = 8;
		pinfo->vesa_dsc.slice_width = 1079;
		pinfo->vesa_dsc.slice_height = 15;

		pinfo->vesa_dsc.initial_xmit_delay = 512;
		pinfo->vesa_dsc.first_line_bpg_offset = 12;
		pinfo->vesa_dsc.mux_word_size = 48;

		// DSC_CTRL
		pinfo->vesa_dsc.block_pred_enable = 1;
		pinfo->vesa_dsc.linebuf_depth = 9;

		//RC_PARAM3
		pinfo->vesa_dsc.initial_offset = 6144;

		//FLATNESS_QP_TH
		pinfo->vesa_dsc.flatness_min_qp = 3;
		pinfo->vesa_dsc.flatness_max_qp = 12;

		//DSC_PARAM4
		pinfo->vesa_dsc.rc_edge_factor= 0x6;
		pinfo->vesa_dsc.rc_model_size = 8192;

		//DSC_RC_PARAM5: 0x330b0b
		pinfo->vesa_dsc.rc_tgt_offset_lo = (0x330b0b >> 20) & 0xF;
		pinfo->vesa_dsc.rc_tgt_offset_hi = (0x330b0b >> 16) & 0xF;
		pinfo->vesa_dsc.rc_quant_incr_limit1 = (0x330b0b >> 8) & 0x1F;
		pinfo->vesa_dsc.rc_quant_incr_limit0 = (0x330b0b >> 0) & 0x1F;

		//DSC_RC_BUF_THRESH0: 0xe1c2a38
		pinfo->vesa_dsc.rc_buf_thresh0 = (0xe1c2a38 >> 24) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh1 = (0xe1c2a38 >> 16) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh2 = (0xe1c2a38 >> 8) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh3 = (0xe1c2a38 >> 0) & 0xFF;

		//DSC_RC_BUF_THRESH1: 0x46546269
		pinfo->vesa_dsc.rc_buf_thresh4 = (0x46546269 >> 24) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh5 = (0x46546269 >> 16) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh6 = (0x46546269 >> 8) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh7 = (0x46546269 >> 0) & 0xFF;

		//DSC_RC_BUF_THRESH2: 0x7077797b
		pinfo->vesa_dsc.rc_buf_thresh8 = (0x7077797b >> 24) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh9 = (0x7077797b >> 16) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh10 = (0x7077797b >> 8) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh11 = (0x7077797b >> 0) & 0xFF;

		//DSC_RC_BUF_THRESH3: 0x7d7e0000
		pinfo->vesa_dsc.rc_buf_thresh12 = (0x7d7e0000 >> 24) & 0xFF;
		pinfo->vesa_dsc.rc_buf_thresh13 = (0x7d7e0000 >> 16) & 0xFF;

		//DSC_RC_RANGE_PARAM0: 0x1020100
		pinfo->vesa_dsc.range_min_qp0 = (0x1020100 >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp0 = (0x1020100 >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset0 = (0x1020100 >> 16) & 0x3F;
		pinfo->vesa_dsc.range_min_qp1 = (0x1020100 >> 11) & 0x1F;
		pinfo->vesa_dsc.range_max_qp1 = (0x1020100 >> 6) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset1 = (0x1020100 >> 0) & 0x3F;

		//DSC_RC_RANGE_PARAM1: 0x94009be,
		pinfo->vesa_dsc.range_min_qp2 = (0x94009be >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp2 = (0x94009be >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset2 = (0x94009be >> 16) & 0x3F;
		pinfo->vesa_dsc.range_min_qp3 = (0x94009be >> 11) & 0x1F;
		pinfo->vesa_dsc.range_max_qp3 = (0x94009be >> 6) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset3 = (0x94009be >> 0) & 0x3F;

		//DSC_RC_RANGE_PARAM2, 0x19fc19fa,
		pinfo->vesa_dsc.range_min_qp4 = (0x19fc19fa >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp4 = (0x19fc19fa >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset4 = (0x19fc19fa >> 16) & 0x3F;
		pinfo->vesa_dsc.range_min_qp5 = (0x19fc19fa >> 11) & 0x1F;
		pinfo->vesa_dsc.range_max_qp5 = (0x19fc19fa >> 6) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset5 = (0x19fc19fa >> 0) & 0x3F;

		//DSC_RC_RANGE_PARAM3, 0x19f81a38,
		pinfo->vesa_dsc.range_min_qp6 = (0x19f81a38 >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp6 = (0x19f81a38 >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset6 = (0x19f81a38 >> 16) & 0x3F;
		pinfo->vesa_dsc.range_min_qp7 = (0x19f81a38 >> 11) & 0x1F;
		pinfo->vesa_dsc.range_max_qp7 = (0x19f81a38 >> 6) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset7 = (0x19f81a38 >> 0) & 0x3F;

		//DSC_RC_RANGE_PARAM4, 0x1a781ab6,
		pinfo->vesa_dsc.range_min_qp8 = (0x1a781ab6 >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp8 = (0x1a781ab6 >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset8 = (0x1a781ab6 >> 16) & 0x3F;
		pinfo->vesa_dsc.range_min_qp9 = (0x1a781ab6 >> 11) & 0x1F;
		pinfo->vesa_dsc.range_max_qp9 = (0x1a781ab6 >> 6) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset9 = (0x1a781ab6 >> 0) & 0x3F;

		//DSC_RC_RANGE_PARAM5, 0x2af62b34,
		pinfo->vesa_dsc.range_min_qp10 = (0x2af62b34 >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp10 = (0x2af62b34 >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset10 = (0x2af62b34 >> 16) & 0x3F;
		pinfo->vesa_dsc.range_min_qp11 = (0x2af62b34 >> 11) & 0x1F;
		pinfo->vesa_dsc.range_max_qp11 = (0x2af62b34 >> 6) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset11 = (0x2af62b34 >> 0) & 0x3F;

		//DSC_RC_RANGE_PARAM6, 0x2b743b74,
		pinfo->vesa_dsc.range_min_qp12 = (0x2b743b74 >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp12 = (0x2b743b74 >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset12 = (0x2b743b74 >> 16) & 0x3F;
		pinfo->vesa_dsc.range_min_qp13 = (0x2b743b74 >> 11) & 0x1F;
		pinfo->vesa_dsc.range_max_qp13 = (0x2b743b74 >> 6) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset13 = (0x2b743b74 >> 0) & 0x3F;

		//DSC_RC_RANGE_PARAM7, 0x6bf40000,
		pinfo->vesa_dsc.range_min_qp14 = (0x6bf40000 >> 27) & 0x1F;
		pinfo->vesa_dsc.range_max_qp14 = (0x6bf40000 >> 22) & 0x1F;
		pinfo->vesa_dsc.range_bpg_offset14 = (0x6bf40000 >> 16) & 0x3F;
	};

	ret = vcc_cmds_tx(pdev, lcd_vcc_lcdanalog_init_cmds,
		ARRAY_SIZE(lcd_vcc_lcdanalog_init_cmds));
	if (ret != 0) {
		HISI_FB_ERR("LCD vcc lcdanalog init failed!\n");
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
		vcc_cmds_tx(pdev, lcd_vcc_lcdanalog_enable_cmds,
			ARRAY_SIZE(lcd_vcc_lcdanalog_enable_cmds));
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
		.compatible = DTS_COMP_SHARP_TD4322_5P5,
		.data = NULL,
	},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_panel_match_table);

static struct platform_driver this_driver = {
	.probe = mipi_sharp_probe,
	.remove = NULL,
	.suspend = NULL,
	.resume = NULL,
	.shutdown = NULL,
	.driver = {
		.name = "mipi_sharp_TD4322_5P5",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_panel_match_table),
	},
};

static int __init mipi_sharp_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_sharp_panel_init);
