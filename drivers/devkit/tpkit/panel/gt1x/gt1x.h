/*
 * gt1x.h - Touch driver header file of Goodix
 * 
 * Copyright (C) 2015 - 2016 Goodix Technology Incorporated   
 * Copyright (C) 2015 - 2016 Yulong Cai <caiyulong@gt1x.com>
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be a reference 
 * to you, when you are integrating the GOODiX's CTP IC into your system, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of 
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 *
 */

#ifndef GT1X_TS_H_
#define GT1X_TS_H_
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <asm/delay.h>
#include <asm/unaligned.h>
#ifdef CONFIG_OF
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#endif
#include <huawei_platform/log/hw_log.h>
#include "../../huawei_ts_kit.h"
#include "../../huawei_ts_kit_algo.h"
#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#define GTP_DRIVER_VERSION          "v1.6<2017/05/20>"

#define LCD_PANEL_INFO_MAX_LEN  128
#define LCD_PANEL_TYPE_DEVICE_NODE_NAME     "huawei,lcd_panel_type"

#define IIC_MAX_TRANSFER_SIZE       250
#define GTP_CONFIG_ORG_LENGTH       239
#define GTP_CONFIG_EXT_LENGTH       128
#define SWITCH_OFF                  0
#define SWITCH_ON                   1
#define GTP_CONFIG_LENGTH (GTP_CONFIG_ORG_LENGTH + GTP_CONFIG_EXT_LENGTH)

/* request type */
#define GTP_RQST_CONFIG                 0x01
#define GTP_RQST_RESET                  0x03
#define GTP_RQST_RESPONDED              0x00
#define GTP_RQST_NOISE_CFG				0x10
#define GTP_RQST_NORMA_CFG				0x11
#define GTP_RQST_IDLE                   0xFF

/* Register define */
#define GTP_READ_COOR_ADDR          0x814E
#define GTP_REG_CMD                 0x8040
#define GTP_REG_SENSOR_ID           0x814A
#define GTP_REG_CONFIG_DATA         0x8050
#define GTP_REG_CONFIG_RESOLUTION   0x8051
#define GTP_REG_CONFIG_TRIGGER      0x8056
#define GTP_REG_CONFIG_CHECKSUM     0x813C
#define GTP_REG_CONFIG_UPDATE       0x813E
#define GTP_REG_EXT_CONFIG          0xBF7B
#define GTP_REG_VERSION             0x8140
#define GTP_REG_HW_INFO             0x4220
#define GTP_REG_REFRESH_RATE	    0x8056
#define GTP_REG_ESD_CHECK           0x8043
#define GTP_REG_FLASH_PASSBY        0x8006
#define GTP_REG_HN_PAIRED           0x81AA
#define GTP_REG_HN_MODE             0x81A8
#define GTP_REG_MODULE_SWITCH3      0x8058
#define GTP_REG_FW_CHK_MAINSYS      0x41E4
#define GTP_REG_FW_CHK_SUBSYS       0x5095
#define GTP_REG_WAKEUP_GESTURE	    0x814C
#define GTP_REG_RQST                0x8044
#define GTP_REG_HAVE_KEY            0x8057
#define GTP_CONFIG_EXT_ADDR         0x805A

/* Regiter for rawdata test*/
#define GTP_REG_NOISEDATA_9PT	0xAC78
#define GTP_RAWDATA_ADDR_9PT 	0xb5f8 
#define GTP_BASEDATA_ADDR_9PT 	0xb138
#define GTP_REG_GESTURE_POSITION_9PT    0xBE0C

#define GTP_REG_NOISEDATA_9P	0xAF58
#define GTP_RAWDATA_ADDR_9P 	0xB798
#define GTP_BASEDATA_ADDR_9P 	0xB378
#define GTP_REG_GESTURE_POSITION_9P    0xBDA8

#define DRV_NUM_REG_9PT 0x8086
#define SEN_NUM_REG_9PT 0x8088
#define KEY_NUM_REG_9PT 0x80A2
#define MODULE_SWITCH_SK_REG_9PT 0x80A0

#define DRV_NUM_REG_9P 0x807E
#define SEN_NUM_REG_9P 0x8080
#define KEY_NUM_REG_9P 0x80a3
#define MODULE_SWITCH_SK_REG_9P 0x805A


/* Regiter for short  test*/
#define SHORT_STATUS_REG	    0x5095
#define SHORT_THRESHOLD_REG 	    0x8808
#define ADC_READ_DELAY_REG  	    0x880C
#define DIFFCODE_SHORT_THRESHOLD    0x880A

#define DRIVER_CH0_REG_9PT     	    0x8101
#define	SENSE_CH0_REG_9PT     	    0x80E1
#define DRIVER_CH0_REG_9P     	    0x80fc
#define	SENSE_CH0_REG_9P     	    0x80dc

#define DRV_CONFIG_REG  	    0x880E
#define SENSE_CONFIG_REG   	    0x882E
#define DRVSEN_CHECKSUM_REG  	    0x884E
#define CONFIG_REFRESH_REG  	    0x813E
#define	ADC_RDAD_VALUE 		    0x96
#define	DIFFCODE_SHORT_VALUE	    0x14
#define SHORT_CAL_SIZE(a )        ( 4 + (a) * 2 + 2)

/* may used only in test.c */
#define TEST_SHORT_STATUS_REG     0x8800
#define TEST_RESTLT_REG           0x8801


#define DIFF_CODE_REG             0xA322
#define DRV_SELF_CODE_REG         0xA2A0
#define TX_SHORT_NUM_REG          0x8802  
#define MAX_DRV_NUM       32
#define MAX_SEN_NUM       32
#define TX_SHORT_NUM              0x8802
/*  end  */
/* cfg parse from bin */
#define CFG_BIN_SIZE_MIN 	279
#define CFG_SIZE_MAX 		367
#define CFG_INFO_BLOCK_BYTES 	8
#define CFG_HEAD_BYTES 		32
#define MODULE_NUM		22	 
#define	CFG_NUM			23
#define	BIN_CFG_START_LOCAL  	6
#define  CFG_SIZE_MIN  		239

#define SEND_CFG_FLASH	 	150
#define SEND_CFG_RAM		60
/* cmd define */
#define GTP_CMD_NORMAL				0x00
#define GTP_CMD_RAWDATA				0x01
#define GTP_CMD_SLEEP               0x05
#define GTP_CMD_CHARGER_ON          0x06
#define GTP_CMD_CHARGER_OFF         0x07
#define GTP_CMD_GESTURE_WAKEUP      0x08
#define GTP_CMD_CLEAR_CFG           0x10
#define GTP_CMD_ESD                 0xAA
#define GTP_CMD_HN_TRANSFER         0x22
#define GTP_CMD_HN_EXIT_SLAVE       0x28
#define GTP_CMD_DOZE_CONFIG			0x60

#define GTP_MAX_TOUCH    			TS_MAX_FINGER
#define GTP_MAX_KEY_NUM  			4
#define GTP_ADDR_LENGTH  			2

#define SLEEP_MODE					0x01
#define GESTURE_MODE				0x02

#define  FW_IS_OK  						0xBE 
#define  FW_IS_RUNING   				0xAA

/* define offset in the config*/
#define RESOLUTION_LOC              (GTP_REG_CONFIG_RESOLUTION - GTP_REG_CONFIG_DATA)
#define TRIGGER_LOC                 (GTP_REG_CONFIG_TRIGGER - GTP_REG_CONFIG_DATA)
#define MODULE_SWITCH3_LOC			(GTP_REG_MODULE_SWITCH3 - GTP_REG_CONFIG_DATA)
#define EXTERN_CFG_OFFSET			( GTP_CONFIG_EXT_ADDR- GTP_REG_CONFIG_DATA)


#define ROI_STA_REG     0xA6A2
#define ROI_HEAD_LEN    4  
#define ROI_DATA_REG    (ROI_STA_REG + ROI_HEAD_LEN)
#define ROI_READY_MASK    0x10
#define ROI_TRACKID_MASK  0x0f


#define GT1X_OF_NAME	"gt1x"
#define GT1X_OF_NAME_LEN	4
#define GT1X_RETRY_NUM 		3
#define GT1X_RETRY_FIVE	5
#define GT1X_FW_SD_NAME "ts/touch_screen_firmware.img"
#define GT1X_CONFIG_FW_SD_NAME "ts/gt1x_cfg.bin"
#define GT1X_FW_NAME_LEN 	64
#define GT1X_MAX_SENSOR_ID 	5
#define GT1X_PRODUCT_ID_LEN 4
#define GT1X_VERSION_LEN 12
#define GT1X_POWER_BY_LDO 1
#define GT1X_POWER_BY_GPIO 0
#define GT1X_POWER_CONTRL_BY_SELF 1
#define GT1X_POWER_CONTRL_BY_LCD 0
#define GT1X_ABS_MAX_VALUE 255

#define GT1X_NORMAL_CONFIG		"normal_config"
#define GT1X_NORMAL_NOISE_CONFIG	"normal_noise_config"
#define GT1X_GLOVE_CONFIG		"glove_config"
#define GT1X_GLOVE_NOISE_CONFIG		"glove_noise_config"
#define GT1X_HOLSTER_CONFIG		"holster_config"
#define GT1X_HOLSTER_NOISE_CONFIG	"holster_noise_config"
#define GT1X_TEST_CONFIG		"tptest_config"
#define GT1X_NOISE_TEST_CONFIG		"tpnoise_test_config"
#define GT1X_GAME_SCENE_CONFIG		"game_scene_config"

#define GT1X_NEED_SLEEP			1
#define GT1X_NOT_NEED_SLEEP		0

#define GT1X_1151Q_CHIP_ID "1158"
#define getU32(a) ((u32)getUint((u8 *)(a), 4))
#define getU16(a) ((u16)getUint((u8 *)(a), 2))

/*easy wakup getrue event config*/
#define GT1X_DOUBLE_TAB_TYPE 0xCC
#define GT1X_DOUBLE_TAB_FLAG_BIT     (1<<0)
#define GT1X_DOUBLE_TAB_POINT_NUM 1
#define GT1X_GESTURE_ONE_POINT_BYTES 4

#define GT1X_EDGE_POINT_DATA_SIZE	8
#define GT1X_ADDR_EDGE_YER		7
#define GT1X_ADDR_EDGE_XER		6
#define GT1X_ADDR_EDGE_W		5

u32 getUint(u8 * buffer, int len);

#pragma pack(1)
struct gt1x_hw_info {
	u8 product_id[GT1X_PRODUCT_ID_LEN + 1];
	u32 patch_id;
	u32 mask_id;
	u8 sensor_id;
};
#pragma pack()

/*
 * fw_update_info - firmware update information
 * @update_type: should be the value below
 *	 UPDATE_TYPE_HEADER - using requset_firmware, fw_update_boot
 *	 UPDATE_TYPE_FILE - using filp_open, fw_update_sd
 * @statue: update status
 * @progress: update progress
 * @max_progress: max progress
 * @force_update: force update firmware
 */
struct fw_update_info {
	int update_type;
	int status;
	int progress;
	int max_progress;
	int force_update;
	struct fw_info *firmware_info;
	u32 fw_length;
	const struct firmware *fw;

	// file update
	u8 *buffer;
	mm_segment_t old_fs;
	struct file *fw_file;

	// header update
	u8 *fw_data;
};

/*
 * gt1x_ts_feature - special touch feature
 * @TS_FEATURE_NONE: no special touch feature
 * @TS_FEATURE_GLOVE: glove feature
 * @TS_FEATURE_HOLSTER: holster feature
 */
enum gt1x_ts_feature {
	TS_FEATURE_NONE = 0,
	TS_FEATURE_GLOVE,
	TS_FEATURE_HOLSTER,
};

enum gt1x_module_id {
	MODULE_OFILM_ID = 0,
	MODULE_EELY_ID   = 1,
	MODULE_TRULY_ID = 2,
	MODULE_MUTTO_ID = 3,
	MODULE_JUNDA_ID = 5,
	MODULE_TOPTOUCH_ID = 18,
	MODULE_UNKNOW_ID = 0xFF,
};
enum gt1x_chip_id {
	GT_DEFALT_ID = 0,
	GT_1151Q_ID = 54,
};
/*
 * gt1x_ts_config - config data structure
 * @initialized: state
 * @data: config data
 * @size: config data size
 * @delay_ms: delay time/ms
 */
struct gt1x_ts_config {
	bool initialized;
	char name[MAX_STR_LEN + 1];
	u8 data[GTP_CONFIG_LENGTH];
	int size;
	int delay_ms;
};


/*
 * gt1x_ts_roi - finger sense data structure
 * @enabled: enable/disable finger sense feature
 * @data_ready: flag to indicate data ready state
 * @roi_rows: rows of roi data
 * @roi_clos: columes of roi data
 * @mutex: mutex
 * @rawdata: rawdata buffer
 */
struct gt1x_ts_roi {
	bool enabled;
	bool data_ready;
	int track_id;
	unsigned int roi_rows;
	unsigned int roi_cols;
	struct mutex mutex;
	u16 *rawdata;
};

struct roi_matrix {
	short *data;
	unsigned int row;
	unsigned int col;
};

struct gt1x_ts_data;

/*
 * gt1x_ts_ops - touch driver operations stub
 */
struct gt1x_ts_ops {
	int (*i2c_write)(u16 addr, u8 * buffer, s32 len);
	int (*i2c_read)(u16 addr, u8 * buffer, s32 len);
	int (*chip_reset)(void);
	int (*send_cmd)(u8 cmd, u8 data, u8 issleep);
	int (*send_cfg)(struct gt1x_ts_config *cfg_ptr);
	int (*i2c_read_dbl_check)(u16 addr, u8 * buffer, s32 len);
	int (*read_version)(struct gt1x_hw_info * hw_info);
	int (*parse_cfg_data)(const struct firmware *cfg_bin,
				char *cfg_type, u8 *cfg, int *cfg_len, u8 sid);
	int (*feature_resume)(struct gt1x_ts_data *ts); 
};

/*
 * gt1x_ts_data - main structure of touch driver
 * @pdev: pointer to huawei_touch platform device
 * @vdd_ana: analog regulator
 * @vcc_i2c: digital regulator
 * @pinctrl: pinctrl
 * @pins_default: pin configuration in default state
 * @pins_suspend: pin configuration in suspend state
 * @pins_gesture: pin configuration in gesture mode
 * @dev_data: pointer to huawei ts_device_data
 * @ops: driver operations stub
 * @hw_info: chip hardware infomation
 * @normal_config: normal config data, normal config data will be
 *		be used by firmware in normal working state
 * @glove_config: glove config data, glove config data will be
 *		be used by firmware in glove state
 * @holster_config: holster config data, holster config data will be
 *		be used by firmware in holster state
 * @roi: finger sense data structure
 * @max_x/@max_y: max resolution in x/y direction
 * @vdd_value
: analog voltage value
 * @vio_value: digital voltage value
 * @flip_x/@flip_y: coordinates transformation
 * @rawdiff_mode: rawdiff mode, this mode is mainly used by
 *		gt1x gt1x debug tools.
 * @tools_support: gt1x gt1x debug tools supprots
 */
struct gt1x_ts_data {
	struct platform_device *pdev;
	struct regulator *vci;
	struct regulator *vddio;
	struct pinctrl *pinctrl;

	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_suspend;
	struct pinctrl_state *pins_release;
	struct pinctrl_state *pins_gesture;
	struct ts_kit_device_data *dev_data;

	struct gt1x_ts_ops ops;
	struct gt1x_hw_info hw_info;
	struct gt1x_ts_config test_config;
	struct gt1x_ts_config normal_config;
	struct gt1x_ts_config normal_noise_config;
	struct gt1x_ts_config glove_config;
	struct gt1x_ts_config glove_noise_config;
	struct gt1x_ts_config holster_config;
	struct gt1x_ts_config holster_noise_config;
	struct gt1x_ts_config noise_test_config;
	struct gt1x_ts_config game_scene_config;
	bool noise_env;

	struct gt1x_ts_roi roi;
	int max_x;
	int max_y;
	u32 vdd_value;
	bool flip_x;
	bool flip_y;
	u32 qcom_adapter_flag;
	u32 power_self_ctrl;/*0-LCD control, 1-tp controlled*/
	u32 vci_power_type;/*0 - gpio control  1 - ldo  2 - not used*/
	u32 vddio_power_type;/*0 - gpio control  1 - ldo  2 - not used*/
	u32 easy_wakeup_supported;
	u32 create_project_id_supported;
	char project_id[MAX_STR_LEN +1];
	char chip_name[GT1X_PRODUCT_ID_LEN +1];
	char firmware_name[GT1X_FW_NAME_LEN +1];
	volatile bool rawdiff_mode;/*get rawdata flag for gtp tool, 0 : position mode, 1: data mode */
	bool tools_support;
	u32 config_flag;/* 0 - normal config; 1 - extern config*/
	bool fw_damage_flag;

	bool sensor_id_valid;
	bool fw_update_ok;
	int ic_type;  /* 9P or 9PT*/
	int support_get_tp_color;
	u32 gt1x_edge_add;
	u32 gt1x_edge_support_xyer;
	int gt1x_support_wxy;
	u32 gt1x_wxy_data_add;
	u32 gt1x_roi_data_add;
	int gt1x_roi_fw_supported;
	bool open_threshold_status;
	int only_open_once_captest_threshold;
	int fw_only_depend_on_lcd;//0 : fw depend on TP and others ,1 : fw only depend on lcd.
	int hide_plain_lcd_log;
	char lcd_panel_info[LCD_PANEL_INFO_MAX_LEN];
	char  lcd_module_name[MAX_STR_LEN];
	char  lcd_hide_module_name[MAX_STR_LEN];
};

#define IC_TYPE_9PT 0
#define IC_TYPE_9P  1
#if defined (CONFIG_HUAWEI_DSM)
enum FW_uptate_state
{
	GT1X_FW_ALLOC_MEMORY_FAIL = 0,
	GT1X_REQUEST_FW_ERROR,
	GT1X_CHECK_UPDATE_FILE_FAIL,
	GT1X_UPDATE_JUDGE_FAIL,
	GT1X_GET_ISP_FAIL,
	GT1X_RUN_ISP_FAIL,
	GT1X_GET_RAWDATA_FAIL,
	GT1X_BURN_SUBSYSTEM_FAIL,
	GT1X_ENTER_UPDATE_MODE_FAIL,
	GT1X_BURN_FW_GWAKE_FAIL,
	GT1X_BURN_FW_SS51_FAIL,
	GT1X_BURN_FW_DSP_FAIL,
	GT1X_BURN_FW_BOOT_FAIL,
	GT1X_BURN_FW_BOOT_ISP_FAIL,
	GT1X_BURN_FW_LINK_FAIL,
	GT1X_BURN_FW_FINISH_FAIL,
	TS_UPDATE_STATE_UNDEFINE = 255,
};

extern struct dsm_client *ts_dclient;
#endif
extern struct gt1x_ts_data *gt1x_ts;
extern struct fw_update_info update_info;

#define UPDATE_TYPE_HEADER 0
#define UPDATE_TYPE_FILE   1
#define UPDATE_TYPE_SDCARD 2

/* Log define */
#define  GTP_FW_UPDATE_VERIFY   0 /* verify fw when updating*/
/*#define INPUT_TYPE_B_PROTOCOL*/


#define IS_NUM_OR_CHAR(x)    (((x) >= 'A' && (x) <= 'Z') || ((x) == 0) || ((x) >='0' && (x) <= '9'))

int gt1x_i2c_write(u16 addr, u8 * buffer, s32 len);
int gt1x_i2c_read(u16 addr, u8 * buffer, s32 len);
int gt1x_chip_reset(void);
int gt1x_read_version(struct gt1x_hw_info * hw_info);
int gt1x_i2c_read_dbl_check(u16 addr, u8 * buffer, s32 len);
int gt1x_get_channel_num(u32 *sen_num, u32 *drv_num);

extern int gt1x_update_firmware(void);
extern int gt1x_init_tool_node(void);
extern void gt1x_deinit_tool_node(void);
extern int gt1x_get_rawdata(struct ts_rawdata_info *info,
				struct ts_cmd_node *out_cmd);
#endif /* _GT1X_TS_H_ */
