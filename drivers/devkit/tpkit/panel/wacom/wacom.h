/*
 * Wacom Penabled Driver for I2C
 *
 * Copyright (c) 2011-2014 Tatsunosuke Tobita, Wacom.
 * <tobita.tatsunosuke@wacom.co.jp>
 *
 * This program is free software; you can redistribute it
 * and/or modify it under the terms of the GNU General
 * Public License as published by the Free Software
 * Foundation; either version of 2 of the License,
 * or (at your option) any later version.
 */

#ifndef WACOM_I2C_H
#define WACOM_I2C_H

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/irq.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/proc_fs.h>
#include <linux/unistd.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/types.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>

#include <linux/input.h>

#include <linux/firmware.h>
#include <asm/unaligned.h>
#ifdef CONFIG_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#endif
#include <linux/version.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "../../huawei_ts_kit_algo.h"
#include "../../huawei_ts_kit_api.h"
#include "../../huawei_ts_kit.h"

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#include "wac_test.h"

#define WACOM_REG_BASE       0x00
#define INVALID_REG_LENGTH       0x00//INVALID_REG_LENGTH used for don't write i2c address before read data.

#define WACOM_TOUCH_INPUTSIZE   42
#define WACOM_AES_INPUTSIZE     18
//#define WACOM_EMR_INPUTSIZE     10

/*For multi-touch operation**/
#define ONE_FINGER_SIZE_PER_FRAME    7
#define FINGERNUM_IN_PACKET     5

#define DATA_LENGTH_SIZE  2

#define TWO_MESSAGE  2
#define ONE_MESSAGE  1

/*Added for using this prog. */
#define CMD_GET_FEATURE         0x02
#define CMD_SET_FEATURE         0x03
#define RTYPE_FEATURE           0x03 /*: Report type -> feature(11b)*/
#define GET_FEATURE             0x02
#define SET_FEATURE             0x03
#define GFEATURE_SIZE           6
#define SFEATURE_SIZE           8


/*wacom report id relate*/
#define WACOM_PEN_REPORTID 0x06
#define WACOM_TP_REPORTID 0x0c

#define HID_REPORTID_BYTE_OFFSET 0x2



/*tp related input date offset*/
#define TP_FINGER_NUM_OFFSET  0X4
#define TP_FINGER_F1_STATUS_OFFSET 0x5
#define TP_FINGER_CONTACT_IDENTIFIER_OFFSET 0x6
#define TP_FINGER_F1_X_COOR_OFFSET 0x8
#define TP_FINGER_F1_Y_COOR_OFFSET 0xA

/*tp related bit*/
#define WACOM_TP_TIP_SWITCH		(1<<0)

/*pen related input date offset*/
#define PEN_TOOLTYPE_BYTE_OFFSET 0x3
#define PEN_X_BYTE_OFFSET 0x4
#define PEN_Y_BYTE_OFFSET 0x6
#define PEN_PRESSURE_BYTE_OFFSET 0x8
#define PEN_TILT_X_BYTE_OFFSET 0x12
#define PEN_TILT_Y_BYTE_OFFSET 0x13

/*pen related bit*/
#define WACOM_TIP_SWITCH		(1<<0)
#define WACOM_SIDE_SWITCH       (1<<1)
#define WACOM_ERASER		(1<<2)
#define WACOM_INVERT		(1<<3)
#define WACOM_2ND_SIDE_SWITCH		(1<<4)
#define WACOM_RDY   		(1<<5)

#define WACOM_DOWN_BUTTON_INDEX     0
#define WACOM_UP_BUTTON_INDEX   1
#define WACOM_MAX_FINGER_NUM_PER_FRAME   5

/*HID over I2C spec*/
#define HID_DESC_REGISTER       0x01
#define USAGE_PAGE              0x05
#define USAGE_PAGE_DIGITIZERS   0x0d
#define USAGE_PAGE_DESKTOP      0x01
#define USAGE                   0x09
#define USAGE_PEN               0x02
#define USAGE_MOUSE             0x02
#define USAGE_FINGER            0x22
#define USAGE_STYLUS            0x20
#define USAGE_TOUCHSCREEN       0x04
#define USAGE_X                 0x30
#define USAGE_TIPPRESSURE       0x30
#define USAGE_Y                 0x31





#define WACOM_ABS_PRESSURE            255

#define REPORT_ABS_TILT_MINUS_MASK	0x80
#define REPORT_ABS_TILT_DEGREE_MIN	-60
#define REPORT_ABS_TILT_DEGREE_MAX	60

#define WACOM_I2C_RW_MAX_SIZE      255

#define WACOM_FW_UPDATE_DOING 1
#define WACOM_FW_UPDATE_INIT_OR_END 0
#define WACOM_CAP_TEST_DOING 1
#define WACOM_CAP_TEST_INIT_OR_END 0


#define WACOM_LCD_PANEL_INFO_MAX_LEN  128
#define WACOM_LCD_PANEL_TYPE_DEVICE_NODE_NAME     "huawei,lcd_panel_type"

#define WACOM_VENDER_NAME  "wacom"


#define TPLCD_MAX_COMBINE_NUM 16
#define LCD_MODULE_NAME_SIZE 10
#define TP_MODULE_NUM 10


enum TS_dsm_status {
	TS_GET_DESC_FAIL = 0,
	TS_REQUEST_FW_FAIL,
	FWU_FW_CRC_ERROR,
	FWU_FW_UPDATE_FAIL,
	TS_REPORT_ID_FAIL,
	TS_WAKEUP_FAIL,
	TS_UNDEFINE = 255,
};

typedef struct hid_descriptor {
	u16 wHIDDescLength;
	u16 bcdVersion;
	u16 wReportDescLength;
	u16 wReportDescRegister;
	u16 wInputRegister;
	u16 wMaxInputLength;//42 BYTES
	u16 wOutputRegister;
	u16 wMaxOutputLength;
	u16 wCommandRegister;//set feature / get feature will use
	u16 wDataRegister;// set /get
	u16 wVendorID;//056A(LISZT)  OR   2D1F
	u16 wProductID;//8(LISZT)    6
	u16 wVersion;//fw version
	u16 RESERVED_HIGH;
	u16 RESERVED_LOW;
} HID_DESC;

struct wacom_features {
	HID_DESC hid_desc;
	u16 input_size;//42 BYTES
	short x_max;//x_pen
	short y_max;//y_pen
	short x_touch;
	short y_touch;
	short pressure_max;//pen
	u16 fw_version;//fw version
	u16 vendorId;
	u16 productId;
};

struct wacom_id {
	u32 uniqueId;
	u16 deviceId;
	u8 designId;
	u8 customerId;
};

struct wacom_coordinate_info{
	int x_max;
	int y_max;
	int touch_report_x_max;
	int touch_report_y_max;
	int pen_report_x_max;
	int pen_report_y_max;
};

struct wacom_pid_info{
	int  pid_num;
	u32  pid[TPLCD_MAX_COMBINE_NUM];
	u32  pid_panelid[TPLCD_MAX_COMBINE_NUM];
	char  pid_lcd_module[TPLCD_MAX_COMBINE_NUM][LCD_MODULE_NAME_SIZE];
};

struct wacom_pinid_info{
	u32  pinid_num;
	u32  pinid[TP_MODULE_NUM];
	u32  pinid_panelid[TP_MODULE_NUM];
};

struct record_finger_id{
	int origin_id;//the finger_id from ic
	int transform_id;//the finger_id for reporting
};

struct wacom_i2c {
	struct ts_kit_device_data *wacom_chip_data;
	struct platform_device *wacom_dev;
	struct i2c_client *client;
	struct input_dev *input;
	struct input_dev *input_pen;
	struct ts_fingers *ts_cache_fingers;
	struct ts_pens *ts_cache_pens;
	struct wacom_features *features;
	struct wacom_id ids;
	struct wake_lock ts_wake_lock;
	int next_pen_tool;
	u8 data[WACOM_TOUCH_INPUTSIZE];
	struct regulator *tp_vci;
	struct regulator *tp_vddio;
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
	char project_id[MAX_STR_LEN];
	char chip_name[MAX_STR_LEN];
	char chip_vendor[MAX_STR_LEN];
	char tp_module_name[MAX_STR_LEN];
	char lcd_module_name[MAX_STR_LEN];
	char lcd_panel_info[WACOM_LCD_PANEL_INFO_MAX_LEN] ;
	u8 panel_id_reserved_bit;
	u16 panel_id;
	unsigned char tp_pin_id;
	bool is_firmware_broken;
	u16 dev_pid;
	u16 dev_fw_version;
	bool fw_restore;
	int pen_max_pressure;
	struct wacom_coordinate_info coordinate_info;
	struct wacom_pid_info pid_info;
	struct wacom_pinid_info pinid_info;
	bool wacom_update_firmware_from_sd_flag;
	int *m_rawdata;
	int *m_actdata;
	int *m_diff;
	struct MY_INI m_ini;
	struct wacom_calInfoStruct m_CalThr[MAX_RX];
	struct PIN_CHECK  m_pinCheck;
	INT m_Error[ERR_END];
	int *upper_record;
	int *lower_record;
	int delay_after_reset;
	struct record_finger_id  record_id[TS_MAX_FINGER];
};

enum wacom_module_id {
	MODULE_OFILM_ID = 0,
	MODULE_JUNDA_ID = 5,
	MODULE_UNKNOW_ID = 99,
};

enum wacom_chip_name {
	CHIP_W9015_ID = 58,
	CHIP_UNKNOWN_ID   = 99,
};

/*
00 - O-Film + Fuji mesh
01 - Junda + Fuji mesh
10 - O-Film + O-Film mesh
*/
enum wacom_hw_pinid {
	HW_PINID_OFILM_FUJIMESH = 0,
	HW_PINID_JUNDA_FUJIMESH = 1,
	HW_PINID_OFILM_OFILMMESH = 2,
	HW_PINID_UNKNOWN = 3,
};

enum wacom_project_id_reserved_bit {
	RESERVED_BIT_ZERO = 0,
	RESERVED_BIT_ONE = 1,
	UNKNOWN_BIT = 9,
};

//u16 pid
enum WACOM_LCM_PID {
	LCM_PID_UNKNOWN  			= 0,
	LCM_OFILM_FUJIMESH_BOE 		= 0x487A,
	LCM_OFILM_FUJIMESH_INX 		= 0x487B,
	LCM_OFILM_OFILMMESH_BOE 		= 0x487C,
	LCM_OFILM_OFILMMESH_INX 		= 0x487D,
	LCM_JUNDA_FUJIMESH_BOE 		= 0x487E,
	LCM_JUNDA_FUJIMESH_INX 		= 0x487F,
};

bool wacom_i2c_set_feature(struct i2c_client *client, u8 report_id, unsigned int buf_size, u8 *data,
			u16 cmdreg, u16 datareg);
bool wacom_i2c_get_feature(struct i2c_client *client, u8 report_id, unsigned int buf_size, u8 *data,
			u16 cmdreg, u16 datareg);
int wacom_query_device(struct i2c_client *client, struct wacom_features *features);
void wacom_report_pen_data(struct wacom_i2c *wac_i2c,u8 *data, struct ts_cmd_node *out_cmd);

int UBL_G11T_SetFeature(u8 report_id, u16 buf_size, u8 *data);
int UBL_G11T_GetFeature(u8 report_id, u16 buf_size, u8 *recv_data);
int wacom_fac_test(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);

//int wacom_ioread(u8 *recv_buf, u16 buf_size);

extern struct ts_kit_platform_data g_ts_kit_platform_data;
#endif
