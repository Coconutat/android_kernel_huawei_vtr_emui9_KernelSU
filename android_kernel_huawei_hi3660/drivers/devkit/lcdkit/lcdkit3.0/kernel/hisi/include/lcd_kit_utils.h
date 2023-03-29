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

#ifndef __LCD_KIT_UTILS_H_
#define __LCD_KIT_UTILS_H_
#include <linux/kernel.h>
#include "lcd_kit_common.h"
#include "lcd_kit_panel.h"
#include "lcd_kit_sysfs.h"
#include "lcd_kit_adapt.h"
#include <linux/hisi/hw_cmdline_parse.h> //for runmode_is_factory

/*macro*/
/*default panel*/
#define LCD_KIT_DEFAULT_PANEL     "auo_otm1901a_5p2_1080p_video_default"

#define ARRAY_3_SIZE(array) (sizeof(array)/sizeof(array[0][0][0]))

/* lcd fps scence */
#define LCD_KIT_FPS_SCENCE_IDLE          BIT(0)
#define LCD_KIT_FPS_SCENCE_VIDEO       BIT(1)
#define LCD_KIT_FPS_SCENCE_GAME        BIT(2)
#define LCD_KIT_FPS_SCENCE_WEB          BIT(3)
#define LCD_KIT_FPS_SCENCE_EBOOK      BIT(4)
#define LCD_KIT_FPS_SCENCE_FORCE_30FPS    BIT(5)
#define LCD_KIT_FPS_SCENCE_FUNC_DEFAULT_ENABLE    BIT(6)
#define LCD_KIT_FPS_SCENCE_FUNC_DEFAULT_DISABLE    BIT(7)
/* lcd fps value */
#define LCD_KIT_FPS_30 (30)
#define LCD_KIT_FPS_55 (55)
#define LCD_KIT_FPS_60 (60)
#define MAX_BUF 60
#define LCD_REG_LENGTH_MAX 200
/*checksum*/
#define TEST_PIC_0  0
#define TEST_PIC_1  1
#define TEST_PIC_2  2
#define CHECKSUM_CHECKCOUNT 5
#define LCD_DDIC_INFO_LEN 64
#define LCD_KIT_CHECKSUM_SIZE   8
/*2d barcode*/
#define BARCODE_LENGTH 46
/*ldo check*/
#define LDO_CHECK_COUNT  20
#define LDO_NUM_MAX  3
#define LDO_NAME_LEN_MAX 32
/*lcd panel version*/
#define VERSION_VALUE_NUM_MAX 10
#define VERSION_NUM_MAX 10

/*enum*/
enum {
	LCD_KIT_CHECKSUM_START = 0,
	LCD_KIT_CHECKSUM_END = 1,
};

enum {
	RGBW_SET1_MODE = 1,
	RGBW_SET2_MODE = 2,
	RGBW_SET3_MODE = 3,
	RGBW_SET4_MODE = 4,
};

enum
{
	RGBW_PANEL_ID_MIN = 0,
	JDI_NT36860C_PANEL_ID = 1,
	LG_NT36870_PANEL_ID = 2,
	SHARP_NT36870_PANEL_ID = 3,
	JDI_HX83112C_PANLE_ID = 4,
	SHARP_HX83112C_PANEL_ID = 5,
	JDI_TD4336_PANEL_ID = 6,
	SHARP_TD4336_PANEL_ID = 7,
	LG_NT36772A_PANEL_ID = 8,
	BOE_HX83112E_PANEL_ID = 9,
	JDI_TD4336_HMA_PANEL_ID = 10,
	SHARP_TD4336_HMA_PANEL_ID = 11,
	LG_NT36772A_HMA_PANEL_ID = 12,
	BOE_HX83112E_HMA_PANEL_ID = 13,
	RGBW_PANEL_ID_MAX,
};


enum {
	LCD_OFFLINE = 0,
	LCD_ONLINE = 1,
};

/*struct define*/
struct lcd_kit_checksum {
	u32 support;
	u32 pic_index;
	u32 status;
	u32 check_count;
	struct lcd_kit_dsi_panel_cmds checksum_cmds;
	struct lcd_kit_dsi_panel_cmds enable_cmds;
	struct lcd_kit_dsi_panel_cmds disable_cmds;
	struct lcd_kit_array_data value;
};

struct lcd_kit_hkadc {
	u32 support;
	int value;
};

struct lcd_kit_gamma {
	u32 support;
	u32 addr;
	u32 length;
	struct lcd_kit_dsi_panel_cmds cmds;
};

struct lcd_kit_brightness_color_uniform {
	u32 support;
	/* color consistency support*/
	struct lcd_kit_dsi_panel_cmds brightness_color_cmds;
};

struct lcd_kit_panel_id {
	u32 modulesn;
	u32 equipid;
	u32 modulemanufactdate;
	u32 vendorid;
};

struct lcd_kit_color_uniform_params {
	u32 c_lmt[3];
	u32 mxcc_matrix[3][3];
	u32 white_decay_luminace;
};

struct lcd_kit_color_measure_data {
	u32 chroma_coordinates[4][2];
	u32 white_luminance;
};

struct lcd_kit_brightness_color_oeminfo
{
	uint32_t id_flag;
	uint32_t tc_flag;
	struct lcd_kit_panel_id  panel_id;
	struct lcd_kit_color_uniform_params color_params;
	struct lcd_kit_color_measure_data color_mdata;
};

struct lcd_kit_2d_barcode {
	u32 support;
	struct lcd_kit_dsi_panel_cmds cmds;
};

struct lcd_kit_oem_info {
	u32 support;
	/*2d barcode*/
	struct lcd_kit_2d_barcode barcode_2d;
	/*brightness and color uniform*/
	struct lcd_kit_brightness_color_uniform brightness_color_uniform;
};

struct lcd_kit_ldo_check {
	u32 support;
	u32 ldo_num;
	u32 ldo_channel[LDO_NUM_MAX];
	int ldo_current[LDO_NUM_MAX];
	int curr_threshold[LDO_NUM_MAX];
	char ldo_name[LDO_NUM_MAX][LDO_NAME_LEN_MAX];
};

struct lcd_kit_project_id {
	u32 support;
	char* default_project_id;
	char id[LCD_DDIC_INFO_LEN];
	struct lcd_kit_dsi_panel_cmds cmds;
};

struct lcd_kit_panel_version {
    u32 support;
    u32 value_number;
    u32 version_number;
    char read_value[VERSION_VALUE_NUM_MAX];
    char lcd_version_name[VERSION_NUM_MAX][LCD_PANEL_VERSION_SIZE];
    struct lcd_kit_arrays_data value;
    struct lcd_kit_dsi_panel_cmds cmds;
};

struct lcd_kit_fps {
	u32 support;
	struct lcd_kit_dsi_panel_cmds dfr_enable_cmds;
	struct lcd_kit_dsi_panel_cmds dfr_disable_cmds;
	struct lcd_kit_dsi_panel_cmds fps_to_30_cmds;
	struct lcd_kit_dsi_panel_cmds fps_to_60_cmds;
	struct lcd_kit_array_data low_frame_porch;
	struct lcd_kit_array_data normal_frame_porch;
};

struct lcd_kit_rgbw {
	u32 support;
	u32 rgbw_bl_max;
	struct lcd_kit_dsi_panel_cmds mode1_cmds;
	struct lcd_kit_dsi_panel_cmds mode2_cmds;
	struct lcd_kit_dsi_panel_cmds mode3_cmds;
	struct lcd_kit_dsi_panel_cmds mode4_cmds;
	struct lcd_kit_dsi_panel_cmds backlight_cmds;
	struct lcd_kit_dsi_panel_cmds saturation_ctrl_cmds;
	struct lcd_kit_dsi_panel_cmds frame_gain_limit_cmds;
	struct lcd_kit_dsi_panel_cmds frame_gain_speed_cmds;
	struct lcd_kit_dsi_panel_cmds color_distor_allowance_cmds;
	struct lcd_kit_dsi_panel_cmds pixel_gain_limit_cmds;
	struct lcd_kit_dsi_panel_cmds pixel_gain_speed_cmds;
	struct lcd_kit_dsi_panel_cmds pwm_gain_cmds;
};

struct lcd_kit_current_detect {
	u32 support;
	struct lcd_kit_dsi_panel_cmds detect_cmds;
	struct lcd_kit_array_data value;
};

struct lcd_kit_lv_detect {
	u32 support;
	struct lcd_kit_dsi_panel_cmds detect_cmds;
	struct lcd_kit_array_data value;
};

struct lcd_kit_alpm {
	u32 support;
	u32 state;
	struct lcd_kit_dsi_panel_cmds exit_cmds;
	struct lcd_kit_dsi_panel_cmds off_cmds;
	struct lcd_kit_dsi_panel_cmds low_light_cmds;
	struct lcd_kit_dsi_panel_cmds high_light_cmds;
};

struct lcd_kit_snd_disp {
	u32 support;
	struct lcd_kit_dsi_panel_cmds on_cmds;
	struct lcd_kit_dsi_panel_cmds off_cmds;
};

struct lcd_kit_quickly_sleep_out {
	u32 support;
	u32 interval;
	u32 panel_on_tag;
	struct timeval panel_on_record_tv;
};

/*function declare*/
extern int mipi_dsi_ulps_cfg(struct hisi_fb_data_type *hisifd, int enable);
struct hisi_fb_data_type* dev_get_hisifd(struct device* dev);
int lcd_kit_lread_reg(void* pdata, uint32_t* out, struct lcd_kit_dsi_cmd_desc* cmds, uint32_t len);
int lcd_kit_rgbw_set_mode(struct hisi_fb_data_type* hisifd, int mode);
int lcd_kit_rgbw_set_backlight(struct hisi_fb_data_type* hisifd, int bl_level);
int lcd_kit_rgbw_set_handle(struct hisi_fb_data_type* hisifd);
int lcd_kit_updt_fps(struct platform_device* pdev);
int lcd_kit_updt_fps_scence(struct platform_device* pdev, uint32_t scence);
int lcd_kit_get_bl_set_type(struct hisi_panel_info* pinfo);
int lcd_kit_rgbw_set_bl(struct hisi_fb_data_type* hisifd, uint32_t level);
int lcd_kit_blpwm_set_backlight(struct hisi_fb_data_type* hisifd, uint32_t level);
int lcd_kit_mipi_set_backlight(struct hisi_fb_data_type* hisifd, uint32_t level);
int lcd_kit_is_enter_sleep_mode(void);
int lcd_kit_checksum_set(struct hisi_fb_data_type* hisifd, int pic_index);
int lcd_kit_checksum_check(struct hisi_fb_data_type* hisifd);
int lcd_kit_is_enter_pt_mode(void);
int lcd_kit_current_det(struct hisi_fb_data_type* hisifd);
int lcd_kit_lv_det(struct hisi_fb_data_type* hisifd);
int lcd_kit_read_gamma(struct hisi_fb_data_type* hisifd, uint8_t *read_value);
int lcd_kit_parse_switch_cmd(struct hisi_fb_data_type* hisifd, char *command);
int lcd_kit_read_project_id(void);
int lcd_kit_panel_version_init(struct hisi_fb_data_type* hisifd);
int lcd_kit_alpm_setting(struct hisi_fb_data_type* hisifd, uint32_t mode);
int lcd_kit_utils_init(struct device_node* np, struct hisi_panel_info* pinfo);
int lcd_kit_dsi_fifo_is_full(char __iomem* dsi_base);
int lcd_kit_dsi_fifo_is_empty(char __iomem* dsi_base);
int lcd_kit_realtime_set_xcc(struct hisi_fb_data_type *hisifd, char *buf, size_t count);
int lcd_kit_get_power_status(void);
bool lcd_kit_support(void);
void lcd_kit_effect_switch_ctrl(struct hisi_fb_data_type* hisifd, bool ctrl);
void lcd_kit_disp_on_check_delay(void);
void lcd_kit_disp_on_record_time(void);
void lcd_kit_factory_init(struct hisi_panel_info* pinfo);
void lcd_kit_read_power_status(struct hisi_fb_data_type* hisifd);
struct lcd_kit_brightness_color_oeminfo *lcd_kit_get_brightness_color_oeminfo(void);
void lcd_kit_set_mipi_tx_link(struct hisi_fb_data_type *hisifd, struct lcd_kit_dsi_panel_cmds* cmds);
void lcd_kit_set_mipi_rx_link(struct hisi_fb_data_type *hisifd, struct lcd_kit_dsi_panel_cmds* cmds);
void lcd_kit_set_mipi_clk(struct hisi_fb_data_type* hisifd, uint32_t clk);
int lcd_kit_get_value_from_dts(char *compatible, char *dts_name, u32 *value);
#endif
