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
#ifndef __HW_LCD_PANEL_H_
#define __HW_LCD_PANEL_H_
/***********************************************************
*macro definition
***********************************************************/
#include "hw_lcd_debug.h"
#define CHECKSUM_SIZE   (8)
#define DTS_COMP_LCD_PANEL_TYPE     "huawei,lcd_panel_type"
#define PANEL_COMP_LENGTH       128
#define ESD_DEBUG   0
#define HW_LCD_POWER_STATUS_CHECK   1
#define BACKLIGHT_PRINT_TIMES	10
#define HW_LCD_DEFAULT_PANEL "hisilicon,auo_otm1901a_5p2_1080p_video_default"

#define CABC_OFF	(0)
#define CABC_UI_MODE	(1)
#define CABC_STILL_MODE	(2)
#define CABC_MOVING_MODE	(3)

#include "voltage/tps65132.h"

enum{
	POWER_CTRL_BY_REGULATOR,
	POWER_CTRL_BY_GPIO,
};
/*parse dirtyregion info node*/
#define OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, propname, ptr_out_value) \
	do { \
		of_property_read_u32(np, propname, ptr_out_value); \
		if( 0xffff == *ptr_out_value ) { \
			*ptr_out_value = -1; \
		} \
	} while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U32_RETURN(np, propname, ptr_out_value) \
	do { \
		if( of_property_read_u32(np, propname, ptr_out_value) ) { \
			HISI_FB_ERR("of_property_read_u32: %s, fail\n", propname); \
		} \
	} while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U8_RETURN(np, propname, ptr_out_value) \
	do { \
		int temp; \
		if( of_property_read_u32(np, propname, &temp) ) { \
			HISI_FB_ERR("of_property_read: %s, fail\n", propname); \
		} \
		*ptr_out_value = (char)temp; \
	} while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U32_DEFAULT(np, propname, ptr_out_value, default) \
	do { \
		if( of_property_read_u32(np, propname, ptr_out_value) ) { \
			HISI_FB_ERR("of_property_read_u32: %s, fail, use default: %d\n", propname, default); \
			*ptr_out_value = default;  \
		} \
	} while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U8_DEFAULT(np, propname, ptr_out_value, default) \
	do { \
		int temp; \
		if( of_property_read_u32(np, propname, &temp) ) { \
			HISI_FB_ERR("of_property_read: %s, fail, use default: %d\n", propname, default); \
			temp = default;  \
		} \
		*ptr_out_value = (char)temp; \
	} while (0)

/***********************************************************
*struct definition
***********************************************************/
/*dsi cmd struct*/
struct dsi_cmd_set {
	char* buf;
	int size_buf;
	struct dsi_cmd_desc* cmd_set;
	int cmd_cnt;
};

struct array_data {
	char* buf;
	int cnt;
};

struct hw_lcd_information {
	/*LcdanalogVcc*/
	u32 lcdanalog_vcc;
	/*LcdioVcc*/
	u32 lcdio_vcc;
	/*LcdBias*/
	u32 lcd_bias;
	/*LcdVsp*/
	u32 lcd_vsp;
	/*LcdVsn*/
	u32 lcd_vsn;
	u32 power_ctrl_mode;
	/*lcd on command*/
	struct dsi_cmd_set display_on_cmds;
	/*lcd off command*/
	struct dsi_cmd_set display_off_cmds;
	/*mipi tr inversion*/
	struct dsi_cmd_set dot_inversion_cmds;
	struct dsi_cmd_set column_inversion_cmds;
	/*lcd forword/revert scan test*/
	struct dsi_cmd_set forword_scan_cmds;
	struct dsi_cmd_set revert_scan_cmds;
	/*cabc off command*/
	struct dsi_cmd_set cabc_off_cmds;
	/*cabc ui command*/
	struct dsi_cmd_set cabc_ui_cmds;
	/*cabc still command*/
	struct dsi_cmd_set cabc_still_cmds;
	/*cabc moving command*/
	struct dsi_cmd_set cabc_moving_cmds;
	/*sre on command*/
	struct dsi_cmd_set sre_on_cmds;
	/*sre off command*/
	struct dsi_cmd_set sre_off_cmds;
	/*esd check*/
	struct array_data esd_reg;
	struct array_data esd_value;
	/*mipi tr check*/
	struct array_data mipi_check_reg;
	struct array_data mipi_check_value;
	/*backlight sem*/
	struct semaphore bl_sem;
	/*lcd power on sequence*/
	struct array_data power_on_seq;
	/*lcd power on sequence*/
	struct array_data power_off_seq;
	/*inversion mode*/
	int inversion_mode;
	/*scan mode*/
	int scan_mode;
	/*cabc function*/
	int cabc_mode;
	/*panel compatible*/
	char* lcd_compatible;
	/*panel name*/
	char* panel_name;
	/*focal ic need lock cmd one to avoid esd disturb*/
	u8 lock_cmd_support;
	/*read lcd power status*/
	u8 read_power_status;
	/*ID0*/
	u32 lcd_id0;
	/*ID1*/
	u32 lcd_id1;
	/*esd set backlight*/
	u8 esd_set_bl;
	/*pt station test support*/
	u8 pt_test_support;
	/*Read data type*/
	u8 read_data_type;
};

enum {
	INVERSION_COLUMN,
	INVERSION_DOT,
};

enum {
	SCAN_TYPE_FORWORD = 0,
	SCAN_TYPE_REVERT,
};

enum {
	LCD_GPIO_RESET_ON_STEP1 = 0,
	LCD_GPIO_RESET_ON_STEP2,
	LCD_GPIO_RESET_OFF_STEP1,
	LCD_GPIO_RESET_OFF_STEP2,
	LCD_GPIO_VSP_VSN_ON_STEP,
	LCD_GPIO_VSP_VSN_OFF_STEP,
};

enum {
	LCD_VCC_POWER_ON_STEP = 0,
	LCD_VCC_POWER_OFF_STEP,
};

enum {
	VCI_ON_DELAY_SEQ,
	IOVCC_ON_DELAY_SEQ,
	FST_RESET_H1_DELAY_SEQ,
	FST_RESET_L_DELAY_SEQ,
	FST_RESET_H2_DELAY_SEQ,
	VSP_ON_DELAY_SEQ,
	VSN_ON_DELAY_SEQ,
	LP11_DELAY_SEQ,
	SEC_RESET_H1_DELAY_SEQ,
	SEC_RESET_L_DELAY_SEQ,
	SEC_RESET_H2_DELAY_SEQ,
	TP_RESET_TO_LCD_INIT_DELAY_SEQ,
	PANEL_ON_TO_BL_ON_DELAY_SEQ,
	POWER_ON_SEQ_MAX,
};

enum {
	FST_RESET_LOW_DELAY_SEQ,
	VSN_OFF_DELAY_SEQ,
	VSP_OFF_DELAY_SEQ,
	SEC_RESET_LOW_DELAY_SEQ,
	IOVCC_OFF_DELAY_SEQ,
	VCI_OFF_DELAY_SEQ,
	POWER_OFF_SEQ_MAX,
};

/***********************************************************
*variable declaration
***********************************************************/
/*extern variable*/
extern bool gesture_func;
extern volatile bool g_lcd_control_tp_power;
#if defined(CONFIG_LCDKIT_DRIVER)
extern ssize_t get_lcdkit_support(void);
#endif
static int g_debug_enable;

static struct hw_lcd_information lcd_info;
/*variable define*/
static bool checksum_enable_ctl = false;
static int hkadc_buf = 0;
static u8 g_enable_PT_test = 0;
static struct timeval lcd_init_done;

/************************************************************
 *
 * LCD VCC
 *
 ************************************************************/
#define VCC_BACKLIGHT_NAME      "lcd_backlight"
#define VCC_LCDBIAS_NAME        "vcc_lcdbias"
#define VCC_LCD_VSN_NAME        "lcd_vsn"
#define VCC_LCD_VSP_NAME        "lcd_vsp"

/* scharg regulator */
static struct regulator* lcd_bl_vcc;
static struct regulator* lcd_bias_vcc;
static struct regulator* lcd_vsn_vcc;
static struct regulator* lcd_vsp_vcc;

static struct vcc_desc hw_lcd_scharger_vcc_get_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, VCC_BACKLIGHT_NAME, &lcd_bl_vcc,   0, 0, 0, 0},
	{DTYPE_VCC_GET, VCC_LCDBIAS_NAME,   &lcd_bias_vcc, 0, 0, 0, 0},
	{DTYPE_VCC_GET, VCC_LCD_VSN_NAME,   &lcd_vsn_vcc,  0, 0, 0, 0},
	{DTYPE_VCC_GET, VCC_LCD_VSP_NAME,   &lcd_vsp_vcc,  0, 0, 0, 0},
};

static struct vcc_desc hw_lcd_scharger_vcc_set_cmds[] = {
	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCDBIAS_NAME, &lcd_bias_vcc, 5400000, 5400000, 0, 0},
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCD_VSP_NAME, &lcd_vsp_vcc,  5400000, 5400000, 0, 0},
	{DTYPE_VCC_SET_VOLTAGE, VCC_LCD_VSN_NAME, &lcd_vsn_vcc,  5400000, 5400000, 0, 0},
};

static struct vcc_desc hw_lcd_scharger_vcc_put_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, VCC_BACKLIGHT_NAME, &lcd_bl_vcc,   0, 0, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCDBIAS_NAME,   &lcd_bias_vcc, 0, 0, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCD_VSN_NAME,   &lcd_vsn_vcc,  0, 0, 0, 0},
	{DTYPE_VCC_PUT, VCC_LCD_VSP_NAME,   &lcd_vsp_vcc,  0, 0, 0, 0},
};

static struct vcc_desc hw_lcd_scharger_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, VCC_LCDBIAS_NAME,   &lcd_bias_vcc, 0, 0, WAIT_TYPE_MS, 5},
	{DTYPE_VCC_ENABLE, VCC_LCD_VSP_NAME,   &lcd_vsp_vcc,  0, 0, WAIT_TYPE_MS, 5},
	{DTYPE_VCC_ENABLE, VCC_LCD_VSN_NAME,   &lcd_vsn_vcc,  0, 0, WAIT_TYPE_MS, 5},
};

static struct vcc_desc hw_lcd_scharger_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, VCC_LCD_VSN_NAME,   &lcd_vsn_vcc,  0, 0, WAIT_TYPE_MS, 5},
	{DTYPE_VCC_DISABLE, VCC_LCD_VSP_NAME,   &lcd_vsp_vcc,  0, 0, WAIT_TYPE_MS, 5},
	{DTYPE_VCC_DISABLE, VCC_LCDBIAS_NAME,   &lcd_bias_vcc, 0, 0, WAIT_TYPE_MS, 1},
};

static struct vcc_desc hw_lcd_scharger_bl_enable_cmds[] = {
	/* backlight enable */
	{DTYPE_VCC_ENABLE, VCC_BACKLIGHT_NAME, &lcd_bl_vcc,   0, 0, WAIT_TYPE_MS, 10},
};

static struct vcc_desc hw_lcd_scharger_bl_disable_cmds[] = {
	/* backlight disable */
	{DTYPE_VCC_DISABLE, VCC_BACKLIGHT_NAME, &lcd_bl_vcc,  0, 0, WAIT_TYPE_MS, 1},
};

struct vsp_vsn_voltage{
	u32 voltage;
	int value;
};

/*******************************************************************************
** LCD VCC
*/
#define HW_LCD_VCC_LCDIO_NAME       "lcdio-vcc"
#define HW_LCD_VCC_LCDANALOG_NAME   "lcdanalog-vcc"

static struct regulator* hw_lcd_vcc_lcdio;
static struct regulator* hw_lcd_vcc_lcdanalog;

static struct vcc_desc hw_lcdio_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, HW_LCD_VCC_LCDIO_NAME, &hw_lcd_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	/* io set voltage */
	{DTYPE_VCC_SET_VOLTAGE, HW_LCD_VCC_LCDIO_NAME, &hw_lcd_vcc_lcdio, 1800000, 1800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc hw_lcdanalog_vcc_init_cmds[] = {
	/* vcc get */
	{DTYPE_VCC_GET, HW_LCD_VCC_LCDANALOG_NAME, &hw_lcd_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
	/* vcc set voltage */
	{DTYPE_VCC_SET_VOLTAGE, HW_LCD_VCC_LCDANALOG_NAME, &hw_lcd_vcc_lcdanalog, 2800000, 2800000, WAIT_TYPE_MS, 0},
};

static struct vcc_desc hw_lcd_vcc_finit_cmds[] = {
	/* vcc put */
	{DTYPE_VCC_PUT, HW_LCD_VCC_LCDIO_NAME, &hw_lcd_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 0},
	{DTYPE_VCC_PUT, HW_LCD_VCC_LCDANALOG_NAME, &hw_lcd_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 0},
};

static struct vcc_desc hw_lcdio_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, HW_LCD_VCC_LCDIO_NAME, &hw_lcd_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc hw_lcdanalog_vcc_enable_cmds[] = {
	/* vcc enable */
	{DTYPE_VCC_ENABLE, HW_LCD_VCC_LCDANALOG_NAME, &hw_lcd_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc hw_lcdio_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, HW_LCD_VCC_LCDIO_NAME, &hw_lcd_vcc_lcdio, 0, 0, WAIT_TYPE_MS, 3},
};

static struct vcc_desc hw_lcdanalog_vcc_disable_cmds[] = {
	/* vcc disable */
	{DTYPE_VCC_DISABLE, HW_LCD_VCC_LCDANALOG_NAME, &hw_lcd_vcc_lcdanalog, 0, 0, WAIT_TYPE_MS, 3},
};

/*************************
**vcc and bl gpio
*/
#define GPIO_LCD_VSP_NAME		"gpio_lcd_vsp"
#define GPIO_LCD_VSN_NAME		"gpio_lcd_vsn"

static uint32_t gpio_lcd_vsp;
static uint32_t gpio_lcd_vsn;

static struct gpio_desc lcd_bias_request_cmds[] = {
	/*AVDD +5.5V*/
	{
		DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_VSP_NAME, &gpio_lcd_vsp, 0
	},
	/* AVEE_-5.5V */
	{
		DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_VSN_NAME, &gpio_lcd_vsn, 0
	},
};

static struct gpio_desc lcd_bias_enable_cmds[] = {
	/* AVDD_5.5V */
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_VSP_NAME, &gpio_lcd_vsp, 1
	},
	/* AVEE_-5.5V */
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_VSN_NAME, &gpio_lcd_vsn, 1
	},
};
static struct gpio_desc lcd_bias_free_cmds[] = {
	/* AVEE_-5.5V */
	{
		DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_VSN_NAME, &gpio_lcd_vsn, 0
	},
	/* AVDD_5.5V */
	{
		DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LCD_VSP_NAME, &gpio_lcd_vsp, 0
	},
};

static struct gpio_desc lcd_bias_disable_cmds[] = {
	/* AVEE_-5.5V */
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 20,
		GPIO_LCD_VSN_NAME, &gpio_lcd_vsn, 0
	},

	/* AVDD_5.5V */
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_VSP_NAME, &gpio_lcd_vsp, 0
	},

	/* AVEE_-5.5V input */
	{
		DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_VSN_NAME, &gpio_lcd_vsn, 0
	},
	/* AVDD_5.5V input */
	{
		DTYPE_GPIO_INPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_VSP_NAME, &gpio_lcd_vsp, 0
	},
};

static struct vsp_vsn_voltage voltage_table[] = {
		{4000000,TPS65132_VOL_40},
		{4100000,TPS65132_VOL_41},
		{4200000,TPS65132_VOL_42},
		{4300000,TPS65132_VOL_43},
		{4400000,TPS65132_VOL_44},
		{4500000,TPS65132_VOL_45},
		{4600000,TPS65132_VOL_46},
		{4700000,TPS65132_VOL_47},
		{4800000,TPS65132_VOL_48},
		{4900000,TPS65132_VOL_49},
		{5000000,TPS65132_VOL_50},
		{5100000,TPS65132_VOL_51},
		{5200000,TPS65132_VOL_52},
		{5300000,TPS65132_VOL_53},
		{5400000,TPS65132_VOL_54},
		{5500000,TPS65132_VOL_55},
		{5600000,TPS65132_VOL_56},
		{5700000,TPS65132_VOL_57},
		{5800000,TPS65132_VOL_58},
		{5900000,TPS65132_VOL_59},
		{6000000,TPS65132_VOL_60},
};

/*******************************************************************************
** LCD IOMUX
*/
static struct pinctrl_data pctrl;

static struct pinctrl_cmd_desc hw_lcd_pinctrl_init_cmds[] = {
	{DTYPE_PINCTRL_GET, &pctrl, 0},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
	{DTYPE_PINCTRL_STATE_GET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc hw_lcd_pinctrl_normal_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_DEFAULT},
};

static struct pinctrl_cmd_desc hw_lcd_pinctrl_lowpower_cmds[] = {
	{DTYPE_PINCTRL_SET, &pctrl, DTYPE_PINCTRL_STATE_IDLE},
};

static struct pinctrl_cmd_desc hw_lcd_pinctrl_finit_cmds[] = {
	{DTYPE_PINCTRL_PUT, &pctrl, 0},
};


/*******************************************************************************
 ** LCD GPIO
 */

#define GPIO_LCD_RESET_NAME "gpio_lcd_reset"
#define GPIO_LCD_ID0_NAME	"gpio_lcd_id0"
#define GPIO_LCD_ID1_NAME	"gpio_lcd_id1"
static uint32_t gpio_lcd_reset;
static struct gpio_desc hw_lcd_gpio_request_cmds[] = {
	/* reset */
	{
		DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0
	},
};

static struct gpio_desc hw_lcd_gpio_free_cmds[] = {
	/* reset */
	{
		DTYPE_GPIO_FREE, WAIT_TYPE_US, 100,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0
	},
};

static struct gpio_desc hw_lcd_gpio_normal_cmds[] = {
	/* reset */
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1
	},
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0
	},
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 15,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 1
	},
};

static struct gpio_desc hw_lcd_gpio_lowpower_cmds[] = {
	/* reset */
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 5,
		GPIO_LCD_RESET_NAME, &gpio_lcd_reset, 0
	},
};

static struct gpio_desc hw_lcd_gpio_id_request_cmds[] = {
	/*id0*/
	{
		DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &lcd_info.lcd_id0, 0
	},
	/*id1*/
	{
		DTYPE_GPIO_REQUEST, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID1_NAME, &lcd_info.lcd_id1, 0
	},
};

static struct gpio_desc hw_lcd_gpio_id0_low_cmds[] = {
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &lcd_info.lcd_id0, 0
	},
};

static struct gpio_desc hw_lcd_gpio_id0_high_cmds[] = {
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID0_NAME, &lcd_info.lcd_id0, 1
	},
};

static struct gpio_desc hw_lcd_gpio_id1_low_cmds[] = {
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID1_NAME, &lcd_info.lcd_id1, 0
	},
};

static struct gpio_desc hw_lcd_gpio_id1_high_cmds[] = {
	{
		DTYPE_GPIO_OUTPUT, WAIT_TYPE_MS, 0,
		GPIO_LCD_ID1_NAME, &lcd_info.lcd_id1, 1
	},
};

static struct gpio_desc hw_lcd_gpio_id0_input_cmds[] = {
	{
		DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_ID0_NAME, &lcd_info.lcd_id0, 0
	},
};

static struct gpio_desc hw_lcd_gpio_id1_input_cmds[] = {
	{
		DTYPE_GPIO_INPUT, WAIT_TYPE_US, 100,
		GPIO_LCD_ID1_NAME, &lcd_info.lcd_id1, 0
	},
};

/***********************************************************
*function declaration
***********************************************************/
extern int mipi_dsi_ulps_cfg(struct hisi_fb_data_type *hisifd, int enable);

static int hw_lcd_parse_array_data(struct device_node* np, char* name, struct array_data* out);
static int hw_lcd_parse_dts(struct device_node* np);
static void hw_lcd_vcc_init(struct vcc_desc* cmds, int cnt);
static int hw_lcd_on(struct platform_device* pdev);
static int hw_lcd_off(struct platform_device* pdev);
static int hw_lcd_remove(struct platform_device* pdev);
static int hw_lcd_set_backlight(struct platform_device* pdev, uint32_t bl_level);
static int hw_lcd_set_fastboot(struct platform_device* pdev);
static ssize_t hw_lcd_model_show(struct platform_device* pdev,
                                 char* buf);
static ssize_t hw_lcd_check_reg_show(struct platform_device* pdev, char* buf);
static ssize_t hw_lcd_mipi_detect_show(struct platform_device* pdev, char* buf);
static ssize_t hw_lcd_hkadc_show(struct platform_device* pdev, char* buf);
static ssize_t hw_lcd_hkadc_store(struct platform_device* pdev,
                                        const char* buf, size_t count);
static ssize_t hw_lcd_gram_check_show(struct platform_device* pdev, char* buf);
static ssize_t hw_lcd_gram_check_store(struct platform_device* pdev,
                                       const char* buf, size_t count);
static int hw_lcd_set_display_region(struct platform_device* pdev,
                                     struct dss_rect* dirty);
static int __init hw_lcd_probe(struct platform_device* pdev);
#endif
