/* Copyright (c) 2017-2018, Huawei terminal Tech. Co., Ltd. All rights reserved.
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

#ifndef __LCD_KIT_PANEL_H_
#define __LCD_KIT_PANEL_H_
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/clk.h>
#include <linux/time.h>
#include <linux/kthread.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <linux/version.h>
#include <linux/io.h>
#include <linux/mm.h>
#include <linux/highmem.h>
#include <linux/memblock.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <linux/pinctrl/consumer.h>
#include <linux/file.h>
#include <linux/dma-buf.h>
#include <linux/genalloc.h>
#include <linux/platform_device.h>
#include "lcd_kit_bias.h"
#include "lcd_kit_core.h"
#include "lcd_kit_bl.h"

extern int lcd_kit_msg_level;
/*log level*/
#define MSG_LEVEL_ERROR	1
#define MSG_LEVEL_WARNING	2
#define MSG_LEVEL_INFO	3
#define MSG_LEVEL_DEBUG	4

#define LCD_KIT_ERR(msg, ...)    \
	do { if (lcd_kit_msg_level >= MSG_LEVEL_ERROR)  \
		printk(KERN_ERR "[LCD_KIT/E]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCD_KIT_WARNING(msg, ...)    \
	do { if (lcd_kit_msg_level >= MSG_LEVEL_WARNING)  \
		printk(KERN_WARNING "[LCD_KIT/W]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCD_KIT_INFO(msg, ...)    \
	do { if (lcd_kit_msg_level >= MSG_LEVEL_INFO)  \
		printk(KERN_ERR "[LCD_KIT/I]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define LCD_KIT_DEBUG(msg, ...)    \
	do { if (lcd_kit_msg_level >= MSG_LEVEL_DEBUG)  \
		printk(KERN_INFO "[LCD_KIT/D]%s: "msg, __func__, ## __VA_ARGS__); } while (0)

#define SENCE_ARRAY_SIZE 100
#define LCD_KIT_CMD_NAME_MAX 100
#define MAX_REG_READ_COUNT	4

#define LCD_KIT_FAIL -1
#define LCD_KIT_OK 0
/*check thead period*/
#define CHECK_THREAD_TIME_PERIOD	(5000)
#define BITS(x)     (1<<x)
#define BL_MIN (0)
#define BL_MAX (256)
#define BL_NIT (400)
#define BL_REG_NOUSE_VALUE (128)

struct lcd_kit_common_ops *lcd_kit_get_common_ops(void);
#define common_ops	lcd_kit_get_common_ops()
struct lcd_kit_common_info *lcd_kit_get_common_info(void);
#define common_info	lcd_kit_get_common_info()
struct lcd_kit_power_desc *lcd_kit_get_power_handle(void);
#define power_hdl	lcd_kit_get_power_handle()
struct lcd_kit_power_seq *lcd_kit_get_power_seq(void);
#define power_seq	lcd_kit_get_power_seq()

#define LCD_KIT_DELAY(n) \
	do { \
		if (n > 10) { \
			msleep(n); \
		} else { \
			if (n > 0) \
				mdelay(n); \
		} \
	} while (0)

/*parse dirtyregion info node*/
#define OF_PROPERTY_READ_DIRTYREGION_INFO_RETURN(np, propname, ptr_out_value) \
	do { \
		if (of_property_read_u32(np, propname, ptr_out_value)) { \
			LCD_KIT_INFO("of_property_read_u32: %s not find\n", propname); \
			*ptr_out_value = -1; \
		} \
		if( 0xffff == *ptr_out_value ) { \
			*ptr_out_value = -1; \
		} \
	} while (0)

#define OF_PROPERTY_READ_U64_RETURN(np, propname, ptr_out_value) \
	do { \
		u32 temp = 0; \
		if( of_property_read_u32(np, propname, &temp) ) { \
			LCD_KIT_INFO("of_property_read: %s not find\n", propname); \
			temp = 0; \
		} \
		*ptr_out_value = (u64)temp; \
	} while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U32_RETURN(np, propname, ptr_out_value) \
	do { \
		if( of_property_read_u32(np, propname, ptr_out_value) ) { \
			LCD_KIT_INFO("of_property_read_u32: %s not find\n", propname); \
			*ptr_out_value = 0; \
		} \
	} while (0)

#define OF_PROPERTY_READ_S8_RETURN(np, propname, ptr_out_value) \
	do { \
		if( of_property_read_u32(np, propname, ptr_out_value) ) { \
			LCD_KIT_INFO("of_property_read_u32: %s not find\n", propname); \
			*ptr_out_value = 0; \
		} \
		if((*ptr_out_value & 0xff00) == 0xff00){ \
			*ptr_out_value = 0-(*ptr_out_value & 0xff); \
		} \
	} while (0)


/*parse dts node*/
#define OF_PROPERTY_READ_U8_RETURN(np, propname, ptr_out_value) \
	do { \
		int temp = 0; \
		if( of_property_read_u32(np, propname, &temp) ) { \
			LCD_KIT_INFO("of_property_read: %s not find\n", propname); \
			temp = 0; \
		} \
		*ptr_out_value = (char)temp; \
	} while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U32_DEFAULT(np, propname, ptr_out_value, default) \
	do { \
		if( of_property_read_u32(np, propname, ptr_out_value) ) { \
			LCD_KIT_INFO("of_property_read_u32: %s not find, use default: %d\n", propname, default); \
			*ptr_out_value = default;  \
		} \
	} while (0)

/*parse dts node*/
#define OF_PROPERTY_READ_U8_DEFAULT(np, propname, ptr_out_value, default) \
	do { \
		int temp = 0; \
		if( of_property_read_u32(np, propname, &temp) ) { \
			LCD_KIT_INFO("of_property_read: %s not find, use default: %d\n", propname, default); \
			temp = default;  \
		} \
		*ptr_out_value = (char)temp; \
	} while (0)

/*enum*/
enum lcd_kit_mipi_ctrl_mode {
	LCD_KIT_DSI_LP_MODE,
	LCD_KIT_DSI_HS_MODE,
};

/*get blmaxnit*/
enum
{
	GET_BLMAXNIT_FROM_DDIC = 1,
};

enum lcd_kit_power_mode {
	NONE_MODE,
	REGULATOR_MODE,
	GPIO_MODE,
};

enum lcd_kit_power_type {
	LCD_KIT_VCI,
	LCD_KIT_IOVCC,
	LCD_KIT_VSP,
	LCD_KIT_VSN,
	LCD_KIT_RST,
	LCD_KIT_BL,
	LCD_KIT_VDD,
};

enum lcd_kit_event {
	EVENT_NONE,
	EVENT_VCI,
	EVENT_IOVCC,
	EVENT_VSP,
	EVENT_VSN,
	EVENT_RESET,
	EVENT_MIPI,
	EVENT_EARLY_TS,
	EVENT_LATER_TS,
	EVENT_VDD,
};

enum bl_order {
	BL_BIG_ENDIAN,
	BL_LITTLE_ENDIAN,
};

enum cabc_mode {
	CABC_OFF_MODE = 0,
	CABC_UI = 1,
	CABC_STILL = 2,
	CABC_MOVING = 3,
};

enum inversion_mode{
	COLUMN_MODE = 0,
	DOT_MODE,
};

enum scan_mode {
	FORWORD_MODE = 0,
	REVERT_MODE,
};

enum hbm_mode {
	HBM_OFF_MODE = 0,
	HBM_MAX_MODE = 1,
	HBM_MEDIUM_MODE = 2,
};

enum acl_mode {
	ACL_OFF_MODE = 0,
	ACL_HIGH_MODE = 1,
	ACL_MIDDLE_MODE = 2,
	ACL_LOW_MODE = 3,
};

enum vr_mode {
	VR_DISABLE = 0,
	VR_ENABLE = 1,
};

enum ce_mode {
	CE_OFF_MODE = 0,
	CE_SRGB = 1,
	CE_USER = 2,
	CE_VIVID = 3,
};

enum esd_state {
	ESD_RUNNING = 0,
	ESD_STOP = 1,
};

enum lcd_type {
	LCD_TYPE = 1,
	AMOLED_TYPE = 2,
};

enum esd_judge_type {
	ESD_UNEQUAL,
	ESD_EQUAL,
	ESD_BIT_VALID,
};

/***********************************************************
*struct definition
***********************************************************/
//dsi_cmd_desc
struct lcd_kit_dsi_cmd_desc {
	char dtype;/* data type */
	char last;/* last in chain */
	char vc;/* virtual chan */
	char ack;/* ask ACK from peripheral */
	char wait;/* ms */
	char waittype;
	char dlen;/* 8 bits */
	char* payload;
} __packed;

struct lcd_kit_dsi_cmd_desc_header {
	char dtype;/* data type */
	char last;/* last in chain */
	char vc;/* virtual chan */
	char ack;/* ask ACK from peripheral */
	char wait;/* ms */
	char waittype;
	char dlen;/* 8 bits */
};

//dsi_panel_cmds
/*dsi cmd struct*/
struct lcd_kit_dsi_panel_cmds {
	char* buf;
	int blen;
	struct lcd_kit_dsi_cmd_desc* cmds;
	int cmd_cnt;
	int link_state;
	u32 flags;
};

/*get blmaxnit*/
struct lcd_kit_blmaxnit {
	u32 get_blmaxnit_type;
	u32 lcd_kit_brightness_ddic_info;
	struct lcd_kit_dsi_panel_cmds bl_maxnit_cmds;
};

struct lcd_kit_array_data {
	uint32_t* buf;
	int cnt;
};

struct lcd_kit_arrays_data {
	struct lcd_kit_array_data* arry_data;
	int cnt;
};

struct region_rect {
	u32 x;
	u32 y;
	u32 w;
	u32 h;
};

struct lcd_kit_cabc {
	u32 support;
	u32 mode;
	/*cabc off command*/
	struct lcd_kit_dsi_panel_cmds cabc_off_cmds;
	/*cabc ui command*/
	struct lcd_kit_dsi_panel_cmds cabc_ui_cmds;
	/*cabc still command*/
	struct lcd_kit_dsi_panel_cmds cabc_still_cmds;
	/*cabc moving command*/
	struct lcd_kit_dsi_panel_cmds cabc_moving_cmds;
};

struct lcd_kit_hbm {
	u32 support;
	u32 hbm_fp_support;
	u32 hbm_level_max;
	u32 hbm_level_current;
	u32 hbm_level_before_fp_capture;
	struct lcd_kit_dsi_panel_cmds enter_cmds;
	struct lcd_kit_dsi_panel_cmds fp_enter_cmds;
	struct lcd_kit_dsi_panel_cmds hbm_prepare_cmds;
	struct lcd_kit_dsi_panel_cmds prepare_cmds_fir;
	struct lcd_kit_dsi_panel_cmds prepare_cmds_sec;
	struct lcd_kit_dsi_panel_cmds prepare_cmds_thi;
	struct lcd_kit_dsi_panel_cmds prepare_cmds_fou;
	struct lcd_kit_dsi_panel_cmds hbm_cmds;
	struct lcd_kit_dsi_panel_cmds hbm_post_cmds;
	struct lcd_kit_dsi_panel_cmds exit_cmds;
	struct lcd_kit_dsi_panel_cmds exit_cmds_fir;
	struct lcd_kit_dsi_panel_cmds exit_cmds_sec;
	struct lcd_kit_dsi_panel_cmds exit_cmds_thi;
	struct lcd_kit_dsi_panel_cmds exit_cmds_thi_new;
	struct lcd_kit_dsi_panel_cmds exit_cmds_fou;
	struct lcd_kit_dsi_panel_cmds enter_dim_cmds;
	struct lcd_kit_dsi_panel_cmds exit_dim_cmds;
	struct mutex hbm_lock;
};

struct lcd_kit_inversion {
	u32 support;
	u32 mode;
	struct lcd_kit_dsi_panel_cmds dot_cmds;
	struct lcd_kit_dsi_panel_cmds column_cmds;
};

struct lcd_kit_scan {
	u32 support;
	u32 mode;
	struct lcd_kit_dsi_panel_cmds forword_cmds;
	struct lcd_kit_dsi_panel_cmds revert_cmds;
};

struct lcd_kit_esd {
	u32 support;
	u32 status;
	struct lcd_kit_dsi_panel_cmds cmds;
	struct lcd_kit_array_data value;
};

struct lcd_kit_esd_error_info
{
	int esd_error_reg_num;
	int esd_reg_index[MAX_REG_READ_COUNT];
	int esd_expect_reg_val[MAX_REG_READ_COUNT];
	int esd_error_reg_val[MAX_REG_READ_COUNT];
};

struct lcd_kit_checkreg {
	u32 support;
	struct lcd_kit_dsi_panel_cmds cmds;
	struct lcd_kit_array_data value;
};

struct lcd_kit_check_reg_dsm {
	u32 support;
	u32 support_dsm_report;
	struct lcd_kit_dsi_panel_cmds cmds;
	struct lcd_kit_array_data value;
};

struct lcd_kit_mipicheck {
	u32 support;
	u32 mipi_error_report_threshold;
	struct lcd_kit_dsi_panel_cmds cmds;
	struct lcd_kit_array_data value;
};

struct lcd_kit_mipierrors {
	u32 mipi_check_times;
	u32 mipi_error_times;
	u32 total_errors;
};

struct lcd_kit_dirty {
	u32 support;
	struct lcd_kit_dsi_panel_cmds cmds;
};

struct lcd_kit_acl {
	u32 support;
	u32 mode;
	struct lcd_kit_dsi_panel_cmds acl_enable_cmds;
	struct lcd_kit_dsi_panel_cmds acl_off_cmds;
	struct lcd_kit_dsi_panel_cmds acl_low_cmds;
	struct lcd_kit_dsi_panel_cmds acl_middle_cmds;
	struct lcd_kit_dsi_panel_cmds acl_high_cmds;
};

struct lcd_kit_vr {
	u32 support;
	u32 mode;
	struct lcd_kit_dsi_panel_cmds enable_cmds;
	struct lcd_kit_dsi_panel_cmds disable_cmds;
};

struct lcd_kit_ce {
	u32 support;
	u32 mode;
	struct lcd_kit_dsi_panel_cmds off_cmds;
	struct lcd_kit_dsi_panel_cmds srgb_cmds;
	struct lcd_kit_dsi_panel_cmds user_cmds;
	struct lcd_kit_dsi_panel_cmds vivid_cmds;
};

struct lcd_kit_set_vss{
	u32 support;
	u32 power_off;
	u32 new_backlight;
	struct lcd_kit_dsi_panel_cmds cmds_fir;
	struct lcd_kit_dsi_panel_cmds cmds_sec;
	struct lcd_kit_dsi_panel_cmds cmds_thi;
};

struct lcd_kit_effect_on {
	u32 support;
	struct lcd_kit_dsi_panel_cmds cmds;
};

struct lcd_kit_pt_test {
	u32 support;
	u32 panel_ulps_support;
	u32 mode;
};

struct lcd_kit_effect_color {
	u32 support;
	u32 mode;
};

struct lcd_kit_adapt_ops {
	int (*mipi_tx)(void *hld, struct lcd_kit_dsi_panel_cmds *cmds);
	int (*mipi_rx)(void *hld, u8 *out, struct lcd_kit_dsi_panel_cmds *cmds);
	int (*gpio_enable)(u32 type);
	int (*gpio_disable)(u32 type);
	int (*regulator_enable)(u32 type);
	int (*regulator_disable)(u32 type);
	int (*buf_trans)(const char* inbuf, int inlen, char** outbuf, int* outlen);
	int (*lock)(void *hld);
	void (*release)(void *hld);
	void *(*get_pdata_hld)(void);
};

struct lcd_kit_backlight {
	u32 order;
	u32 bl_min;
	u32 bl_max;
	struct lcd_kit_dsi_panel_cmds bl_cmd;
};

struct lcd_kit_check_thread {
	int enable;
	int check_bl_support;
	struct hrtimer hrtimer;
	struct delayed_work check_work;
};

struct lcd_kit_common_ops {
	int (*panel_power_on)(void* hld);
	int (*panel_on_lp)(void* hld);
	int (*panel_on_hs)(void* hld);
	int (*panel_off_hs)(void* hld);
	int (*panel_off_lp)(void* hld);
	int (*panel_power_off)(void* hld);
	int (*get_panel_name)(char* buf);
	int (*get_panel_info)(char* buf);
	int (*get_cabc_mode)(char* buf);
	int (*set_cabc_mode)(void* hld, u32 mode);
	int (*get_ce_mode)(char* buf);
	int (*get_acl_mode)(char* buf);
	int (*set_acl_mode)(void* hld, u32 mode);
	int (*get_vr_mode)(char* buf);
	int (*set_vr_mode)(void* hld, u32 mode);
	int (*esd_handle)(void* hld);
	int (*dirty_region_handle)(void* hld, struct region_rect* dirty);
	int (*set_ce_mode)(void* hld, u32 mode);
	int (*hbm_set_handle)(void* hld, int last_hbm_level, int hbm_dimming, int hbm_level);
	int (*inversion_set_mode)(void* hld, u32 mode);
	int (*inversion_get_mode)(char *buf);
	int (*scan_set_mode)(void* hld, u32 mode);
	int (*scan_get_mode)(char* buf);
	int (*get_test_config)(char* buf);
	int (*set_test_config)(const char* buf);
	int (*check_reg)(void* hld, char* buf);
	int (*set_sleep_mode)(u32 mode);
	int (*get_sleep_mode)(char* buf);
	int (*set_effect_color_mode)(u32 mode);
	int (*get_effect_color_mode)(char* buf);
	int (*set_mipi_backlight)(void* hld, u32 bl_level);
	int (*common_init)(struct device_node* np);
	int (*get_bias_voltage)(int *vpos, int *vneg);
	void (*mipi_check)(void* pdata, char *panel_name, long display_on_record_time);
};

struct lcd_kit_common_info {
	/**********************running test******************/
	/*test config*/
	char lcd_cmd_now[LCD_KIT_CMD_NAME_MAX];
	/*inversion test*/
	struct lcd_kit_inversion inversion;
	/*lcd forword/revert scan test*/
	struct lcd_kit_scan scan;
	/*running test check reg*/
	struct lcd_kit_checkreg check_reg;
	/*power on check reg*/
	struct lcd_kit_check_reg_dsm check_reg_on;
	/*power off check reg*/
	struct lcd_kit_check_reg_dsm check_reg_off;
	/*mipi check commond*/
	struct lcd_kit_mipicheck mipi_check;
	/*PT current test*/
	struct lcd_kit_pt_test pt;
	/*vss*/
	struct lcd_kit_set_vss set_vss;
	/**********************end******************/
	/**********************effect******************/
	int bl_level_max;
	int bl_level_min;
	u32 ul_does_lcd_poweron_tp;
	/*default max nit*/
	u32 bl_max_nit;
	/*actual max nit*/
	u32 actual_bl_max_nit;
	struct lcd_kit_effect_color effect_color;
	/*cabc function*/
	struct lcd_kit_cabc cabc;
	/*hbm function*/
	struct lcd_kit_hbm hbm;
	/*ACL ctrl*/
	struct lcd_kit_acl acl;
	/*vr mode ctrl*/
	struct lcd_kit_vr vr;
	/*ce*/
	struct lcd_kit_ce ce;
	/*effect on after panel on*/
	struct lcd_kit_effect_on effect_on;
	/**********************end******************/
	/**********************normal******************/
	/*panel name*/
	char* panel_name;
	/*panel model*/
	char* panel_model;
	/*panel type*/
	u32 panel_type;
	/*lcd on command*/
	struct lcd_kit_dsi_panel_cmds panel_on_cmds;
	/*lcd off command*/
	struct lcd_kit_dsi_panel_cmds panel_off_cmds;
	/*esd check commond*/
	struct lcd_kit_esd esd;
	/*display region*/
	struct lcd_kit_dirty dirty_region;
	/*backlight*/
	struct lcd_kit_backlight backlight;
	/*check thread*/
	struct lcd_kit_check_thread check_thread;
	/*get_blmaxnit*/
	struct lcd_kit_blmaxnit blmaxnit;
	/**********************end******************/
};

struct lcd_kit_power_desc {
	struct lcd_kit_array_data lcd_vci;
	struct lcd_kit_array_data lcd_iovcc;
	struct lcd_kit_array_data lcd_vsp;
	struct lcd_kit_array_data lcd_vsn;
	struct lcd_kit_array_data lcd_rst;
	struct lcd_kit_array_data lcd_backlight;
	struct lcd_kit_array_data lcd_te0;
	struct lcd_kit_array_data tp_rst;
	struct lcd_kit_array_data lcd_vdd;
};

struct lcd_kit_power_seq {
	struct lcd_kit_arrays_data power_on_seq;
	struct lcd_kit_arrays_data panel_on_lp_seq;
	struct lcd_kit_arrays_data panel_on_hs_seq;
	struct lcd_kit_arrays_data panel_off_hs_seq;
	struct lcd_kit_arrays_data panel_off_lp_seq;
	struct lcd_kit_arrays_data power_off_seq;
};

/*function declare*/
int lcd_kit_adapt_register(struct lcd_kit_adapt_ops* ops);
struct lcd_kit_adapt_ops* lcd_kit_get_adapt_ops(void);
#endif
