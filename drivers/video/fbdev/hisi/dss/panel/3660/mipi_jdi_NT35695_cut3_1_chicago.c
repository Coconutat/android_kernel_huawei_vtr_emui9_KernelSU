/* Copyright (c) 2008-2011, Hisilicon Tech. Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *	 * Redistributions of source code must retain the above copyright
 *	   notice, this list of conditions and the following disclaimer.
 *	 * Redistributions in binary form must reproduce the above
 *	   copyright notice, this list of conditions and the following
 *	   disclaimer in the documentation and/or other materials provided
 *	   with the distribution.
 *	 * Neither the name of Code Aurora Forum, Inc. nor the names of its
 *	   contributors may be used to endorse or promote products derived
 *	   from this software without specific prior written permission.
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
#include <huawei_platform/touthscreen/huawei_touchscreen.h>

// only for test video vesa3x
//#define DSI_1_2_VESA3X_VIDEO

#define DTS_COMP_JDI_NT35695_CUT3_1	"hisilicon,mipi_jdi_NT35695_cut3_1"
static int g_lcd_fpga_flag;
extern bool g_lcd_control_tp_power;

extern unsigned int g_led_rg_para1;
extern unsigned int g_led_rg_para2;
extern u8 color_temp_cal_buf[32];


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
static char caset_data[] = {
	0x2A,
	0x00,0x00,0x04, 0x37,
};

static char paset_data[] = {
	0x2B,
	0x00,0x00,0x07,0x7F,
};

static char dsc_vesa[] = {
	0x11,0x00,0x00,0x89,
	0x30,0x80,0x07,0x80,
	0x04,0x38,0x00,0x10,
	0x04,0x38,0x04,0x38,
	0x02,0x00,0x03,0x1C,
	0x00,0x20,0x02,0x05,
	0x00,0x0F,0x00,0x0C,
	0x06,0x67,0x03,0x2E,
	0x18,0x00,0x10,0xF0,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,
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

static char tear_on[] = {
	0x35,
	0x00,
};

static char display_on[] = {
	0x29,
};

static char exit_sleep[] = {
	0x11,
};

static char bl_enable[] = {
	0x53,
	0x24,
};

static char bl_pwm[] = {
	0x51,
	0xFF,
};

static char te_line[] = {
	0x44,
	0x07, 0x68,
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
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(partial_setting_1), partial_setting_1},
	{DTYPE_DCS_WRITE1, 0,10, WAIT_TYPE_US,
		sizeof(tear_on), tear_on},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(caset_data), caset_data},
	{DTYPE_DCS_LWRITE, 0,10, WAIT_TYPE_US,
		sizeof(paset_data), paset_data},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(bl_enable), bl_enable},
	{DTYPE_DCS_WRITE1, 0, 200, WAIT_TYPE_US,
		sizeof(bl_pwm), bl_pwm},
	{DTYPE_GEN_LWRITE, 0, 200, WAIT_TYPE_US,
		sizeof(te_line), te_line},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(exit_sleep), exit_sleep},
	{DTYPE_DCS_WRITE, 0, 120, WAIT_TYPE_MS,
		sizeof(display_on), display_on},
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

static struct dsi_cmd_desc lcd_dsc_vesa3x_on_cmds[] = {
	{DTYPE_DSC_LWRITE, 0,10, WAIT_TYPE_MS,
		sizeof(dsc_vesa), dsc_vesa},
};

/*******************************************************************************
** LCD VCC
*/
#define VCC_LCDIO_NAME		"lcdio-vcc"
#define VCC_LCDANALOG_NAME	"lcdanalog-vcc"

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

static struct pinctrl_cmd_desc lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &pctrl, 0},
};

/*******************************************************************************
** LCD GPIO
*/
#define GPIO_LCD_P5V5_ENABLE_NAME	"gpio_lcd_p5v5_enable"
#define GPIO_LCD_N5V5_ENABLE_NAME "gpio_lcd_n5v5_enable"
#define GPIO_LCD_RESET_NAME	"gpio_lcd_reset"
#define GPIO_LCD_BL_ENABLE_NAME	"gpio_lcd_bl_enable"
#define GPIO_LCD_TP2V85_NAME	"gpio_lcd_tp2v85"
#define GPIO_LCD_TP1V8_NAME	"gpio_lcd_tp1v8"

static uint32_t gpio_lcd_p5v5_enable;
static uint32_t gpio_lcd_n5v5_enable;
static uint32_t gpio_lcd_reset;
static uint32_t gpio_lcd_bl_enable;
static uint32_t gpio_lcd_tp2v85;
static uint32_t gpio_lcd_tp1v8;


static struct gpio_desc fpga_lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* TP_2.85V
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_TP2V85_NAME, &gpio_lcd_tp2v85, 0},*/
	/* TP_1.8V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
	/* TP_RESET
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_TP_RESET_NAME, &gpio_lcd_tp_reset, 0},*/
};

static struct gpio_desc lcd_gpio_free_cmds[] = {
	/* backlight enable */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* AVDD_5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* TP_1.8V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	/* TP reset
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_TP_RESET_NAME, &gpio_lcd_tp_reset, 0},*/
	/* TP_2.85V
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 0,
		GPIO_LCD_TP2V85_NAME, &gpio_lcd_tp2v85, 0},*/
};

static struct gpio_desc fpga_lcd_gpio_normal_cmds[] = {
	/* TP _1.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5000,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 1},
	/* TP _1.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 12,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 0},
	/* AVDD_5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 1},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 10,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 50,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc lcd_gpio_lowpower_cmds[] = {
	/* backlight disable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* AVDD_5.5V*/
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* TP _1.8V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 1},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 3,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},

	/* backlight enable input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* AVEE_-5.5V input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* AVDD_5.5V input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* TP _1.8V */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_TP1V8_NAME, &gpio_lcd_tp1v8, 1},
	/* reset input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},

};

//gpio_lcd_p5v5_enable
//gpio_lcd_n5v5_enable
//gpio_lcd_bl_enable
//gpio_lcd_reset
static struct gpio_desc asic_lcd_gpio_request_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* backlight enable */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* reset */
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
};

static struct gpio_desc asic_lcd_gpio_free_cmds[] = {
	/* AVDD_5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* backlight enable */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* reset */
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
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
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 20,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1},
	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 1},
};

static struct gpio_desc asic_lcd_gpio_lowpower_cmds[] = {
	/* backlight enable */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
	/* AVEE_-5.5V */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_N5V5_ENABLE_NAME, &gpio_lcd_n5v5_enable, 0},
	/* AVDD_5.5V*/
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_P5V5_ENABLE_NAME, &gpio_lcd_p5v5_enable, 0},
	/* reset */
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0},

	/* backlight enable input */
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_BL_ENABLE_NAME, &gpio_lcd_bl_enable, 0},
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

static u32 gamma_lut_table_R[] = {
	0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 gamma_lut_table_G[] = {
	0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 gamma_lut_table_B[] = {
	0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 igm_lut_table_R[] = {
	0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 igm_lut_table_G[] = {
	0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 igm_lut_table_B[] = {
	0,	 16,	 32,	 48,	 64,	 80,	 96,	112,	128,	144,
	160,	176,	192,	208,	224,	240,	256,	272,	288,	304,
	320,	336,	352,	368,	384,	400,	416,	432,	448,	464,
	480,	496,	512,	528,	544,	560,	576,	592,	608,	624,
	640,	656,	672,	688,	704,	720,	736,	752,	768,	784,
	800,	816,	832,	848,	864,	880,	896,	912,	928,	944,
	960,	976,	992,	1008,	1024,	1040,	1056,	1072,	1088,	1104,
	1120,	1136,	1152,	1168,	1184,	1200,	1216,	1232,	1248,	1264,
	1280,	1296,	1312,	1328,	1344,	1360,	1376,	1392,	1408,	1424,
	1440,	1456,	1472,	1488,	1504,	1520,	1536,	1552,	1568,	1584,
	1600,	1616,	1632,	1648,	1664,	1680,	1696,	1712,	1728,	1744,
	1760,	1776,	1792,	1808,	1824,	1840,	1856,	1872,	1888,	1904,
	1920,	1936,	1952,	1968,	1984,	2000,	2016,	2032,	2048,	2064,
	2080,	2096,	2112,	2128,	2144,	2160,	2176,	2192,	2208,	2224,
	2240,	2256,	2272,	2288,	2304,	2320,	2336,	2352,	2368,	2384,
	2400,	2416,	2432,	2448,	2464,	2480,	2496,	2512,	2528,	2544,
	2560,	2576,	2592,	2608,	2624,	2640,	2656,	2672,	2688,	2704,
	2720,	2736,	2752,	2768,	2784,	2800,	2816,	2832,	2848,	2864,
	2880,	2896,	2912,	2928,	2944,	2960,	2976,	2992,	3008,	3024,
	3040,	3056,	3072,	3088,	3104,	3120,	3136,	3152,	3168,	3184,
	3200,	3216,	3232,	3248,	3264,	3280,	3296,	3312,	3328,	3344,
	3360,	3376,	3392,	3408,	3424,	3440,	3456,	3472,	3488,	3504,
	3520,	3536,	3552,	3568,	3584,	3600,	3616,	3632,	3648,	3664,
	3680,	3696,	3712,	3728,	3744,	3760,	3776,	3792,	3808,	3824,
	3840,	3856,	3872,	3888,	3904,	3920,	3936,	3952,	3968,	3984,
	4000,	4016,	4032,	4048,	4064,	4080,	4095
};

static u32 gmp_lut_table_low32bit[9][9][9] = {
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0x00000000, 0x00000200, 0x00000400, 0x00000600, 0x00000800, 0x00000a00, 0x00000c00, 0x00000e00, 0x00000ff0, },
        {0x00200000, 0x00200200, 0x00200400, 0x00200600, 0x00200800, 0x00200a00, 0x00200c00, 0x00200e00, 0x00200ff0, },
        {0x00400000, 0x00400200, 0x00400400, 0x00400600, 0x00400800, 0x00400a00, 0x00400c00, 0x00400e00, 0x00400ff0, },
        {0x00600000, 0x00600200, 0x00600400, 0x00600600, 0x00600800, 0x00600a00, 0x00600c00, 0x00600e00, 0x00600ff0, },
        {0x00800000, 0x00800200, 0x00800400, 0x00800600, 0x00800800, 0x00800a00, 0x00800c00, 0x00800e00, 0x00800ff0, },
        {0x00a00000, 0x00a00200, 0x00a00400, 0x00a00600, 0x00a00800, 0x00a00a00, 0x00a00c00, 0x00a00e00, 0x00a00ff0, },
        {0x00c00000, 0x00c00200, 0x00c00400, 0x00c00600, 0x00c00800, 0x00c00a00, 0x00c00c00, 0x00c00e00, 0x00c00ff0, },
        {0x00e00000, 0x00e00200, 0x00e00400, 0x00e00600, 0x00e00800, 0x00e00a00, 0x00e00c00, 0x00e00e00, 0x00e00ff0, },
        {0x00ff0000, 0x00ff0200, 0x00ff0400, 0x00ff0600, 0x00ff0800, 0x00ff0a00, 0x00ff0c00, 0x00ff0e00, 0x00ff0ff0, },
    },
    {
        {0xf0000000, 0xf0000200, 0xf0000400, 0xf0000600, 0xf0000800, 0xf0000a00, 0xf0000c00, 0xf0000e00, 0xf0000ff0, },
        {0xf0200000, 0xf0200200, 0xf0200400, 0xf0200600, 0xf0200800, 0xf0200a00, 0xf0200c00, 0xf0200e00, 0xf0200ff0, },
        {0xf0400000, 0xf0400200, 0xf0400400, 0xf0400600, 0xf0400800, 0xf0400a00, 0xf0400c00, 0xf0400e00, 0xf0400ff0, },
        {0xf0600000, 0xf0600200, 0xf0600400, 0xf0600600, 0xf0600800, 0xf0600a00, 0xf0600c00, 0xf0600e00, 0xf0600ff0, },
        {0xf0800000, 0xf0800200, 0xf0800400, 0xf0800600, 0xf0800800, 0xf0800a00, 0xf0800c00, 0xf0800e00, 0xf0800ff0, },
        {0xf0a00000, 0xf0a00200, 0xf0a00400, 0xf0a00600, 0xf0a00800, 0xf0a00a00, 0xf0a00c00, 0xf0a00e00, 0xf0a00ff0, },
        {0xf0c00000, 0xf0c00200, 0xf0c00400, 0xf0c00600, 0xf0c00800, 0xf0c00a00, 0xf0c00c00, 0xf0c00e00, 0xf0c00ff0, },
        {0xf0e00000, 0xf0e00200, 0xf0e00400, 0xf0e00600, 0xf0e00800, 0xf0e00a00, 0xf0e00c00, 0xf0e00e00, 0xf0e00ff0, },
        {0xf0ff0000, 0xf0ff0200, 0xf0ff0400, 0xf0ff0600, 0xf0ff0800, 0xf0ff0a00, 0xf0ff0c00, 0xf0ff0e00, 0xf0ff0ff0, },
    },
};

static u32 gmp_lut_table_high4bit[9][9][9] = {
    {
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
        {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, },
    },
    {
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
        {0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, 0x2, },
    },
    {
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
        {0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, 0x4, },
    },
    {
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
        {0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, 0x6, },
    },
    {
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
        {0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, },
    },
    {
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
        {0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, 0xa, },
    },
    {
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
        {0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, 0xc, },
    },
    {
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
        {0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, 0xe, },
    },
    {
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
        {0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, 0xf, },
    },
};

static u32 xcc_table[12] = {0x0, 0x8000, 0x0,0x0,0x0,0x0,0x8000,0x0,0x0,0x0,0x0,0x8000,};

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
        968,  972,  976,  980,  984,  988,  992,  996,  1000, 1004, 1008,
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
**
*/
static int mipi_jdi_panel_set_fastboot(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (g_lcd_fpga_flag == 0) {
		// lcd pinctrl normal
		pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
			ARRAY_SIZE(lcd_pinctrl_normal_cmds));
		// lcd gpio request
		gpio_cmds_tx(asic_lcd_gpio_request_cmds,
			ARRAY_SIZE(asic_lcd_gpio_request_cmds));
	} else {
		// lcd gpio request
		gpio_cmds_tx(fpga_lcd_gpio_request_cmds,
			ARRAY_SIZE(fpga_lcd_gpio_request_cmds));
	}

	// backlight on
	hisi_lcd_backlight_on(pdev);

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_jdi_panel_on(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	uint32_t status = 0;
	uint32_t try_times = 0;
	int error = 0;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);
	pinfo = &(hisifd->panel_info);
	BUG_ON(pinfo == NULL);

	HISI_FB_INFO("fb%d, +.\n", hisifd->index);

	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	if (pinfo->lcd_init_step == LCD_INIT_POWER_ON) {
		// check lcd power state
		if (g_lcd_fpga_flag == 0) {
			// lcd vcc enable
			vcc_cmds_tx(pdev, lcd_vcc_enable_cmds,
			ARRAY_SIZE(lcd_vcc_enable_cmds));
			// lcd pinctrl normal
			pinctrl_cmds_tx(pdev, lcd_pinctrl_normal_cmds,
				ARRAY_SIZE(lcd_pinctrl_normal_cmds));

			// lcd gpio request
			gpio_cmds_tx(asic_lcd_gpio_request_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_request_cmds));

			// lcd gpio normal
			gpio_cmds_tx(asic_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_normal_cmds));
		} else {
			// lcd gpio request
			gpio_cmds_tx(fpga_lcd_gpio_request_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_request_cmds));

			// lcd gpio normal
			gpio_cmds_tx(fpga_lcd_gpio_normal_cmds, \
				ARRAY_SIZE(fpga_lcd_gpio_normal_cmds));
		}
		pinfo->lcd_init_step = LCD_INIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_LP_SEND_SEQUENCE) {
		if (g_lcd_control_tp_power && !g_lcd_fpga_flag) {
			error = ts_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);
			if (error) {
				HISI_FB_ERR("ts resume device err!\n");
			}
		}

		if (pinfo->dirty_region_updt_support) {
			// NT35695 partial update sequence
			mipi_dsi_cmds_tx(lcd_partial_updt_cmds, \
				ARRAY_SIZE(lcd_partial_updt_cmds), mipi_dsi0_base);
		}

		// lcd display on sequence
		mipi_dsi_cmds_tx(lcd_display_on_cmds, \
			ARRAY_SIZE(lcd_display_on_cmds), mipi_dsi0_base);

		if (pinfo->ifbc_type == IFBC_TYPE_VESA3X_SINGLE) {
			if (pinfo->mipi.dsi_version == DSI_1_2_VERSION) {
				if (is_mipi_video_panel(hisifd)) {
					mipi_dsi_cmds_tx(lcd_vesa3x_on_cmds, \
						ARRAY_SIZE(lcd_vesa3x_on_cmds), mipi_dsi0_base);

					mipi_dsi_cmds_tx(lcd_dsc_vesa3x_on_cmds, \
						ARRAY_SIZE(lcd_dsc_vesa3x_on_cmds), mipi_dsi0_base);
				} else {
					mipi_dsi_cmds_tx(lcd_dsc_vesa3x_on_cmds, \
						ARRAY_SIZE(lcd_dsc_vesa3x_on_cmds), mipi_dsi0_base);
				}
			} else {
				mipi_dsi_cmds_tx(lcd_vesa3x_on_cmds, \
					ARRAY_SIZE(lcd_vesa3x_on_cmds), mipi_dsi0_base);
			}
		}

		// check lcd power state
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
		HISI_FB_INFO("second LCD Power State = 0x%x.\n", status);

		pinfo->lcd_init_step = LCD_INIT_MIPI_HS_SEND_SEQUENCE;
	} else if (pinfo->lcd_init_step == LCD_INIT_MIPI_HS_SEND_SEQUENCE) {
		if (g_lcd_control_tp_power && !g_lcd_fpga_flag) {
			error = ts_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);
			if (error) {
				HISI_FB_ERR("ts after resume err!\n");
			}
		}
	} else {
		HISI_FB_ERR("failed to init lcd!\n");
	}

	// backlight on
	hisi_lcd_backlight_on(pdev);
	HISI_FB_INFO("fb%d, -!\n", hisifd->index);

	return 0;
}

static int mipi_jdi_panel_off(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;
	char __iomem *mipi_dsi0_base = NULL;
	int error = 0;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);
	pinfo = &(hisifd->panel_info);
	mipi_dsi0_base = hisifd->mipi_dsi0_base;

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_HS_SEND_SEQUENCE) {
		// backlight off
		hisi_lcd_backlight_off(pdev);

		mipi_dsi_cmds_tx(lcd_display_off_cmds, \
			ARRAY_SIZE(lcd_display_off_cmds), mipi_dsi0_base);
		pinfo->lcd_uninit_step = LCD_UNINIT_MIPI_LP_SEND_SEQUENCE;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_MIPI_LP_SEND_SEQUENCE) {
		pinfo->lcd_uninit_step = LCD_UNINIT_POWER_OFF;
	} else if (pinfo->lcd_uninit_step == LCD_UNINIT_POWER_OFF) {
		if (g_lcd_fpga_flag==0) {
			// lcd gpio lowpower
			gpio_cmds_tx(asic_lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_lowpower_cmds));
			// lcd gpio free
			gpio_cmds_tx(asic_lcd_gpio_free_cmds, \
				ARRAY_SIZE(asic_lcd_gpio_free_cmds));
			// lcd pinctrl lowpower
			pinctrl_cmds_tx(pdev,lcd_pinctrl_lowpower_cmds,
				ARRAY_SIZE(lcd_pinctrl_lowpower_cmds));

			mdelay(3);
			// lcd vcc disable
			vcc_cmds_tx(pdev, lcd_vcc_disable_cmds,
				ARRAY_SIZE(lcd_vcc_disable_cmds));
		} else {
			// lcd gpio lowpower
			gpio_cmds_tx(lcd_gpio_lowpower_cmds, \
				ARRAY_SIZE(lcd_gpio_lowpower_cmds));
			// lcd gpio free
			gpio_cmds_tx(lcd_gpio_free_cmds, \
				ARRAY_SIZE(lcd_gpio_free_cmds));
		}

		if (g_lcd_control_tp_power && !g_lcd_fpga_flag && !hisifd->fb_shutdown) {
			error = ts_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);
			if (error) {
				HISI_FB_ERR("ts before suspend err!\n");
			}

			error = ts_power_control_notify(TS_SUSPEND_DEVICE, NO_SYNC_TIMEOUT);
			if (error) {
				HISI_FB_ERR("ts suspend device err!\n");
			}
		}

		if (hisifd->fb_shutdown) {
			ts_thread_stop_notify();
		}
	} else {
		HISI_FB_ERR("failed to uninit lcd!\n");
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_jdi_panel_remove(struct platform_device *pdev)
{
	struct hisi_fb_data_type *hisifd = NULL;

	BUG_ON(pdev == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (g_lcd_fpga_flag == 0) {
		// lcd vcc finit
		vcc_cmds_tx(pdev, lcd_vcc_finit_cmds,
			ARRAY_SIZE(lcd_vcc_finit_cmds));

		// lcd pinctrl finit
		pinctrl_cmds_tx(pdev, lcd_pinctrl_finit_cmds,
			ARRAY_SIZE(lcd_pinctrl_finit_cmds));
	}

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return 0;
}

static int mipi_jdi_panel_set_backlight(struct platform_device *pdev, uint32_t bl_level)
{
	struct hisi_fb_data_type *hisifd = NULL;
	int ret = 0;

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

	HISI_FB_DEBUG("fb%d, +.\n", hisifd->index);

	if (hisifd->panel_info.bl_set_type & BL_SET_BY_PWM) {
		ret = hisi_pwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_BLPWM) {
		ret = hisi_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_SH_BLPWM) {
		ret = hisi_sh_blpwm_set_backlight(hisifd, bl_level);
	} else if (hisifd->panel_info.bl_set_type & BL_SET_BY_MIPI) {
		bl_level_adjust[1] = bl_level * 255 / hisifd->panel_info.bl_max;
		mipi_dsi_cmds_tx(lcd_bl_level_adjust, \
			ARRAY_SIZE(lcd_bl_level_adjust), hisifd->mipi_dsi0_base);
		HISI_FB_INFO("lw bl_level_adjust[1] = %d.\n",bl_level_adjust[1]);
	} else {
		HISI_FB_ERR("fb%d, not support this bl_set_type(%d)!\n",
			hisifd->index, hisifd->panel_info.bl_set_type);
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

static int mipi_jdi_panel_set_display_region(struct platform_device *pdev,
	struct dss_rect *dirty)
{
	struct hisi_fb_data_type *hisifd = NULL;
	struct hisi_panel_info *pinfo = NULL;

	BUG_ON(pdev == NULL || dirty == NULL);
	hisifd = platform_get_drvdata(pdev);
	BUG_ON(hisifd == NULL);

	pinfo = &(hisifd->panel_info);

	if (((dirty->x % 2) != 0) || ((dirty->y % 2) != 0) ||
		((dirty->w % 2) != 0) || ((dirty->h % 2) != 0) ||
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
		ARRAY_SIZE(set_display_address), hisifd->mipi_dsi0_base);

	return 0;
}

/******************************************************************************/
static ssize_t mipi_jdi_panel_lcd_model_show(struct platform_device *pdev,
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

	ret = snprintf(buf, PAGE_SIZE, "JDI_NT35695 6' CMD TFT Chicago\n");

	HISI_FB_DEBUG("fb%d, -.\n", hisifd->index);

	return ret;
}

static ssize_t mipi_jdi_panel_lcd_check_reg_show(struct platform_device *pdev, char *buf)
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

/*******************************************************************************
**
*/
static struct hisi_panel_info g_panel_info = {0};
static struct hisi_fb_panel_data g_panel_data = {
	.panel_info = &g_panel_info,
	.set_fastboot = mipi_jdi_panel_set_fastboot,
	.on = mipi_jdi_panel_on,
	.off = mipi_jdi_panel_off,
	.remove = mipi_jdi_panel_remove,
	.set_backlight = mipi_jdi_panel_set_backlight,
	.set_display_region = mipi_jdi_panel_set_display_region,

	.lcd_model_show = mipi_jdi_panel_lcd_model_show,
	.lcd_check_reg  = mipi_jdi_panel_lcd_check_reg_show,
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

	g_lcd_control_tp_power = true;

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_JDI_NT35695_CUT3_1);
	if (!np) {
		HISI_FB_ERR("not found device node %s!\n", DTS_COMP_JDI_NT35695_CUT3_1);
		goto err_return;
	}

	ret = of_property_read_u32(np, LCD_DISPLAY_TYPE_NAME, &lcd_display_type);
	if (ret) {
		HISI_FB_ERR("get lcd_display_type failed!\n");
		lcd_display_type = PANEL_MIPI_CMD;
	}

	ret = of_property_read_u32(np, LCD_IFBC_TYPE_NAME, &lcd_ifbc_type);
	if (ret) {
		HISI_FB_ERR("get ifbc_type failed!\n");
		lcd_ifbc_type = IFBC_TYPE_NONE;
	}

	ret = of_property_read_u32(np, LCD_BL_TYPE_NAME, &bl_type);
	if (ret) {
		HISI_FB_ERR("get lcd_bl_type failed!\n");
		bl_type = BL_SET_BY_MIPI;
	}
	HISI_FB_INFO("bl_type=0x%x.", bl_type);

	if (hisi_fb_device_probe_defer(lcd_display_type, bl_type))
		goto err_probe_defer;

	HISI_FB_DEBUG("+.\n");

	ret = of_property_read_u32(np, FPGA_FLAG_NAME, &g_lcd_fpga_flag);
	if (ret) {
		HISI_FB_WARNING("need to get g_lcd_fpga_flag resource in fpga, not needed in asic!\n");
	}

	if (g_lcd_fpga_flag == 1) {
		gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", 1);
		gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", 2);
		gpio_lcd_reset = of_get_named_gpio(np, "gpios", 3);
		gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 4);
		gpio_lcd_tp2v85 = of_get_named_gpio(np, "gpios", 5);
		gpio_lcd_tp1v8 = 49;
	} else {
		gpio_lcd_p5v5_enable = of_get_named_gpio(np, "gpios", 0);
		gpio_lcd_n5v5_enable = of_get_named_gpio(np, "gpios", 1);
		gpio_lcd_bl_enable = of_get_named_gpio(np, "gpios", 2);
		gpio_lcd_reset = of_get_named_gpio(np, "gpios", 3);
	}

	pdev->id = 1;
	//init lcd panel info
	pinfo = g_panel_data.panel_info;
	memset(pinfo, 0, sizeof(struct hisi_panel_info));
	pinfo->xres = 1080;
	pinfo->yres = 1920;
	pinfo->width = 75;
	pinfo->height = 133;
	pinfo->orientation = LCD_PORTRAIT;
	pinfo->bpp = LCD_RGB888;
	pinfo->bgr_fmt = LCD_RGB;
	pinfo->bl_set_type = bl_type;

	pinfo->bl_min = 1;
	pinfo->bl_max = 255;
	pinfo->bl_default = 102;

	pinfo->type = lcd_display_type;
	pinfo->ifbc_type = lcd_ifbc_type;
	pinfo->frc_enable = 0;
	pinfo->esd_enable = 0;
	pinfo->esd_skip_mipi_check = 0;
	pinfo->lcd_uninit_step_support = 1;

	//color temperature
	pinfo->color_temperature_support = 0;
	if (color_temp_cal_buf[0] == 1) {
		pinfo->color_temp_rectify_support = 0;
	} else {
		pinfo->color_temp_rectify_support = 1;
	}
	pinfo->color_temp_rectify_R = 31785; /*97 percent*/
	pinfo->color_temp_rectify_G = 32768; /*100 percent*/
	pinfo->color_temp_rectify_B = 32276; /*98.5 percent*/
	g_led_rg_para1 = 6;
	g_led_rg_para2 = 31238;

	//prefix ce & sharpness
	pinfo->prefix_ce_support = 0;
	pinfo->prefix_sharpness1D_support = 0;

	//sbl
	pinfo->sbl_support = 0;
	pinfo->smart_bl.strength_limit = 0;
	pinfo->smart_bl.calibration_a = 60;
	pinfo->smart_bl.calibration_b = 95;
	pinfo->smart_bl.calibration_c = 5;
	pinfo->smart_bl.calibration_d = 1;
	pinfo->smart_bl.t_filter_control = 5;
	pinfo->smart_bl.backlight_min = 480;
	pinfo->smart_bl.backlight_max = 4096;
	pinfo->smart_bl.backlight_scale = 0xff;
	pinfo->smart_bl.ambient_light_min = 14;
	pinfo->smart_bl.filter_a = 1738;
	pinfo->smart_bl.filter_b = 6;
	pinfo->smart_bl.logo_left = 0;
	pinfo->smart_bl.logo_top = 0;

	pinfo->cinema_mode_support = 0;

	//enable arsr1p sharpness
	pinfo->arsr1p_sharpness_support = 1;
	//enable arsr2p sharpness
	pinfo->prefix_sharpness2D_support = 1;

	//init ACM info
	pinfo->acm_support = 1;
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
	}

	//init GAMMA LCP info
	pinfo->gamma_support = 1;
	if (pinfo->gamma_support == 1) {
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
		strncpy(pinfo->hiace_param.chCfgName, "/product/etc/display/effect/algorithm/hdr_engine_MHA.xml", sizeof(pinfo->hiace_param.chCfgName) - 1);
	}

	if (g_lcd_fpga_flag == 1) {
		//ldi
		pinfo->ldi.h_back_porch = 23;//50;
		pinfo->ldi.h_front_porch = 50;//132;
		pinfo->ldi.h_pulse_width = 20;
		pinfo->ldi.v_back_porch = 12;
		pinfo->ldi.v_front_porch = 14;
		pinfo->ldi.v_pulse_width = 4;

		//mipi
		pinfo->mipi.dsi_bit_clk = 120;
		pinfo->mipi.dsi_bit_clk_val1 = 471;
		pinfo->mipi.dsi_bit_clk_val2 = 480;
		pinfo->mipi.dsi_bit_clk_val3 = 490;
		pinfo->mipi.dsi_bit_clk_val4 = 500;
		pinfo->dsi_bit_clk_upt_support = 0;
		pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
		pinfo->pxl_clk_rate = 20* 1000000UL;
	} else {
		//63fps
		pinfo->ldi.h_back_porch = 23;
		pinfo->ldi.h_front_porch = 50;
		pinfo->ldi.h_pulse_width = 20;
		pinfo->ldi.v_back_porch = 28;
		pinfo->ldi.v_front_porch = 14;
		pinfo->ldi.v_pulse_width = 8;

		pinfo->pxl_clk_rate = 146 * 1000000UL;
		//mipi
		pinfo->mipi.dsi_bit_clk = 480;
		pinfo->mipi.dsi_bit_clk_val1 = 471;
		pinfo->mipi.dsi_bit_clk_val2 = 480;
		pinfo->mipi.dsi_bit_clk_val3 = 490;
		pinfo->mipi.dsi_bit_clk_val4 = 500;
		//pinfo->mipi.dsi_bit_clk_val5 = ;
		pinfo->dsi_bit_clk_upt_support = 0;
		pinfo->mipi.dsi_bit_clk_upt = pinfo->mipi.dsi_bit_clk;
	}

	pinfo->mipi.dsi_version = DSI_1_1_VERSION;

	//non_continue adjust : measured in UI
	//JDI requires clk_post >= 60ns + 252ui, so need a clk_post_adjust more than 200ui. Here 215 is used.
	pinfo->mipi.clk_post_adjust = 215;
	pinfo->mipi.clk_pre_adjust= 0;
	pinfo->mipi.clk_t_hs_prepare_adjust= 0;
	pinfo->mipi.clk_t_lpx_adjust= 0;
	pinfo->mipi.clk_t_hs_trial_adjust= 0;
	pinfo->mipi.clk_t_hs_exit_adjust= 0;
	pinfo->mipi.clk_t_hs_zero_adjust= 0;
	pinfo->mipi.non_continue_en = 1;

	//mipi
	pinfo->mipi.lane_nums = DSI_4_LANES;
	pinfo->mipi.color_mode = DSI_24BITS_1;
	pinfo->mipi.vc = 0;
	pinfo->mipi.max_tx_esc_clk = 10 * 1000000;
	pinfo->mipi.burst_mode = DSI_BURST_SYNC_PULSES_1;

	pinfo->pxl_clk_rate_div = 1;
	pinfo->vsync_ctrl_type = VSYNC_CTRL_ISR_OFF | VSYNC_CTRL_MIPI_ULPS | VSYNC_CTRL_CLK_OFF;

	pinfo->dirty_region_updt_support = 0;
	pinfo->dirty_region_info.left_align = -1;
	pinfo->dirty_region_info.right_align = -1;
	pinfo->dirty_region_info.top_align = 32;
	pinfo->dirty_region_info.bottom_align = 32;
	pinfo->dirty_region_info.w_align = -1;
	pinfo->dirty_region_info.h_align = -1;
	pinfo->dirty_region_info.w_min = 1080;
	pinfo->dirty_region_info.h_min = -1;
	pinfo->dirty_region_info.top_start = -1;
	pinfo->dirty_region_info.bottom_start = -1;

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
		.compatible = DTS_COMP_JDI_NT35695_CUT3_1,
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
		.name = "mipi_jdi_NT35695_cut3_1",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_panel_match_table),
	},
};

static int __init mipi_jdi_NT35695_panel_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&this_driver);
	if (ret) {
		HISI_FB_ERR("platform_driver_register failed, error=%d!\n", ret);
		return ret;
	}

	return ret;
}

module_init(mipi_jdi_NT35695_panel_init);
