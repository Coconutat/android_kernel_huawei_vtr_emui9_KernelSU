/*
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 6070 $
 * $Date: 2016-08-24 20:05:37 +0800 (週三, 24 八月 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */
#ifndef 	_LINUX_NVT_TOUCH_H
#define		_LINUX_NVT_TOUCH_H

#include <linux/i2c.h>
#include <linux/input.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

//---INT trigger mode---
//#define IRQ_TYPE_EDGE_RISING 1
//#define IRQ_TYPE_EDGE_FALLING 2
//#define IRQ_TYPE_LEVEL_HIGH 4
//#define IRQ_TYPE_LEVEL_LOW 8
#define INT_TRIGGER_TYPE IRQ_TYPE_EDGE_RISING


//---I2C driver info.---
#define NVT_I2C_NAME "NVT-ts"
#define NVT_HYBRID_I2C_FW_Address 0x01
#define NVT_HYBRID_I2C_BLDR_Address 0x01
#define NVT_HYBRID_I2C_HW_Address 0x62


//---Input device info.---
#define NVT_HYBRID_VENDER_NAME "novatek_hybrid"
#define NVT_HYBRID_TS_NAME "NVTCapacitiveTouchScreen"


//---Touch info.---
#define NVT_HYBRID_TOUCH_MAX_WIDTH 1080
#define NVT_HYBRID_TOUCH_MAX_HEIGHT 1920
#define NVT_HYBRID_TOUCH_MAX_FINGER_NUM 10
#define NVT_HYBRID_TOUCH_KEY_NUM 0
#if NVT_HYBRID_TOUCH_KEY_NUM > 0
extern const uint16_t nvt_hybrid_touch_key_array[NVT_HYBRID_TOUCH_KEY_NUM];
#endif

//---Customerized func.---
#define NVT_HYBRID_TOUCH_PROC 1		// for novatek cmdline tools and apk
#define NVT_HYBRID_TOUCH_EXT_PROC 1	// for novatek debug
#define NVT_HYBRID_TOUCH_MP 1

#define NVT_HYBRID_PROJECT_ID_LEN 9
#define NVT_HYBRID_MP_SELFTEST_MODE_LEN 9

#define NVT_HYBRID_DELAY_FRAME_TIME 18	// 1 frame = 18ms (calculate by report rate 60Hz)
#define NVT_HYBRID_GLOVE_SWITCH_ON 1
#define NVT_HYBRID_GLOVE_SWITCH_OFF 0
#define NVT_HYBRID_HOLSTER_SWITCH_ON 1
#define NVT_HYBRID_HOLSTER_SWITCH_OFF 0
#define ROI_SWITCH_ON 1
#define ROI_SWITCH_OFF 0

#define NVT_RAWDATA_TEST_TIME_DEFAULT 12
#define NVT_ONCELL_TP_COLOR_LEN 2
#define TP_COLOR_SIZE   15
//---Parsing info---
#define NVT_HYBRID_IRQ_CFG	"irq_config"
#define NVT_HYBRID_ALGO_ID	"algo_id"
#define NVT_HYBRID_IC_TYPES	"ic_type"
#define NVT_HYBRID_VCI_LDO_VALUE	"vci_value"
#define NVT_HYBRID_VDDIO_LDO_VALUE	"vddio_value"
#define NVT_HYBRID_NEED_SET_VDDIO_VALUE	"need_set_vddio_value"
#define NVT_HYBRID_WD_CHECK	"need_wd_check_status"
#define NVT_HYBRID_UNIT_CAP_TEST_INTERFACE	"unite_cap_test_interface"
#define NVT_HYBRID_REPORT_RATE_TEST	"report_rate_test"
#define NVT_HYBRID_X_MAX	"x_max"
#define NVT_HYBRID_Y_MAX	"y_max"
#define NVT_HYBRID_X_MAX_MT	"x_max_mt"
#define NVT_HYBRID_Y_MAX_MT	"y_max_mt"
#define NVT_HYBRID_AIN_TX_NUM "ain_tx_num"
#define NVT_HYBRID_AIN_RX_NUM "ain_rx_num"
#define NVT_HYBRID_VCI_GPIO_TYPE	"vci_gpio_type"
#define NVT_HYBRID_VCI_REGULATOR_TYPE	"vci_regulator_type"
#define NVT_HYBRID_VDDIO_GPIO_TYPE	"vddio_gpio_type"
#define NVT_HYBRID_PROJECTID_LEN	"projectid_len"
#define NVT_HYBRID_VDDIO_REGULATOR_TYPE	"vddio_regulator_type"
#define NVT_HYBRID_RAWDATA_TIMEOUT   "rawdata_timeout"
#define NVT_HYBRID_TEST_TYPE	"tp_test_type"
#define NVT_HYBRID_VDDIO_GPIO_CTRL	"vddio_ctrl_gpio"
#define NVT_HYBRID_VCI_GPIO_CTRL	"vci_ctrl_gpio"
#define NVT_HYBRID_COVER_FORCE_GLOVE	"force_glove_in_smart_cover"
#define NVT_HYBRID_ROI_SUPPORTED "roi_supported"
#define NVT_HYBRID_GLOVE_SUPPORTED "glove_supported"
#define NVT_HYBRID_HOLSTER_SUPPORTED "holster_supported"
#define NVT_HYBRID_MP_SELFTEST_MODE "mp_selftest_mode"

struct nvt_hybrid_ts_data {
	struct ts_kit_device_data *chip_data;
	struct platform_device *ts_dev;
	struct regulator *tp_vci;
	struct regulator *tp_vddio;

#ifndef CONFIG_OF
	struct iomux_block *tp_gpio_block;
	struct block_config *tp_gpio_block_config;
#else
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
#endif
	struct mutex i2c_mutex;
	struct mutex mp_mutex;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct work_struct nvt_work;
	struct delayed_work nvt_fwu_work;
	uint16_t addr;
	int8_t phys[32];

#ifdef CONFIG_HAS_EARLYSUSPEND
	struct early_suspend early_suspend;
#endif
	uint16_t abs_x_max;
	uint16_t abs_y_max;
	uint8_t x_num;
	uint8_t y_num;
	uint8_t ain_tx_num;
	uint8_t ain_rx_num;
	uint8_t max_touch_num;
	uint8_t max_button_num;
	uint32_t int_trigger_type;
	int32_t irq_gpio;
	uint32_t irq_flags;
	int32_t reset_gpio;
	uint32_t reset_flags;
	
	bool firmware_updating;
	bool sensor_testing;
	bool print_criteria;
	bool no_power;
	
	char mp_selftest_mode[NVT_HYBRID_MP_SELFTEST_MODE_LEN];
	uint8_t support_get_tp_color;/*for tp color */
};

#if NVT_HYBRID_TOUCH_PROC
struct nvt_hybrid_flash_data{
	rwlock_t lock;
	struct i2c_client *client;
};
#endif

typedef enum {
	NVT_HYBRID_RESET_STATE_INIT = 0xA0,// IC reset
	NVT_HYBRID_RESET_STATE_REK,		// ReK baseline
	NVT_HYBRID_RESET_STATE_REK_FINISH,	// baseline is ready
	NVT_HYBRID_RESET_STATE_NORMAL_RUN	// normal run
} NVT_HYBRID_RST_COMPLETE_STATE;

#endif /* _LINUX_NVT_TOUCH_H */
