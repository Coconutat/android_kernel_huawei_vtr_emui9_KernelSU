/*
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 4017 $
 * $Date: 2016-04-01 09:41:08 +0800 (星期五, 01 四月 2016) $
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

#include <linux/spi/spi.h>
#include <linux/i2c.h>
#include <linux/input.h>

#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif

#include "nt36xxx_mem_map.h"
#include "../../huawei_ts_kit.h"
//---GPIO number---
#define NVTTOUCH_RST_PIN 980
#define NVTTOUCH_INT_PIN 943
#define NVTTOUCH_DISP_RST_PIN 956


//---INT trigger mode---
//#define IRQ_TYPE_EDGE_RISING 1
//#define IRQ_TYPE_EDGE_FALLING 2
//#define IRQ_TYPE_LEVEL_HIGH 4
//#define IRQ_TYPE_LEVEL_LOW 8
#define INT_TRIGGER_TYPE IRQ_TYPE_EDGE_FALLING

#define TS_NV_STRUCTURE_PROID_OFFSET 0
#define TS_NV_STRUCTURE_BAR_CODE_OFFSET1 1
#define TS_NV_STRUCTURE_BAR_CODE_OFFSET2 4
#define TS_NV_STRUCTURE_BRIGHTNESS_OFFSET1 7
#define TS_NV_STRUCTURE_BRIGHTNESS_OFFSET2 9
#define TS_NV_STRUCTURE_WHITE_POINT_OFFSET1 8
#define TS_NV_STRUCTURE_WHITE_POINT_OFFSET2 10
#define TS_NV_STRUCTURE_REPAIR_OFFSET1 11
#define TS_NV_STRUCTURE_REPAIR_OFFSET2 12
#define TS_NV_STRUCTURE_REPAIR_OFFSET3 13
#define TS_NV_STRUCTURE_REPAIR_OFFSET4 14
#define TS_NV_STRUCTURE_REPAIR_OFFSET5 15


//---I2C driver info.---
#define NVT_I2C_NAME "NVT-ts"
#define I2C_BLDR_Address 0x01
#define I2C_FW_Address 0x01
#define I2C_HW_Address 0x62


//---Input device info.---
#define NVT_TS_NAME "NVTCapacitiveTouchScreen"


//---Touch info.---
#define TOUCH_MAX_WIDTH 1080
#define TOUCH_MAX_HEIGHT 2310
#define TOUCH_MAX_FINGER_NUM 10
#define TOUCH_KEY_NUM 0
#define ONE_SIZE 1
#if TOUCH_KEY_NUM > 0
extern const uint16_t touch_key_array[TOUCH_KEY_NUM];
#endif
#define TOUCH_FORCE_NUM 1000

//---Customerized func.---
#define NVT_TOUCH_PROC 1		// for novatek cmdline tools and apk
#define NVT_TOUCH_EXT_PROC 1	// for novatek debug

#define BOOT_UPDATE_FIRMWARE 0
#define BOOT_UPDATE_FIRMWARE_NAME "novatek_ts_fw.bin"
#define NOVATEK_FW_MANUAL_UPDATE_FILE_NAME	"ts/touch_screen_firmware.bin"
#define NOVATEK_MP_MANUAL_UPDATE_FILE_NAME	"ts/touch_screen_mp.bin"

//---ESD Protect.---
#define NVT_TOUCH_WDT_RECOVERY 1

#define PROJECT_ID_LEN 9
#define SUSPEND_CMD_BUF_SIZE	2
#define POWER_SLEEP_MODE	1
#define REAL_PROJECT_ID_LEN 10

#define RBUF_LEN 1025

struct nvt_ts_data {
	struct ts_kit_device_data *chip_data;
	struct platform_device *ts_dev;
	struct regulator *tp_vci;
	struct regulator *tp_vddio;
	char fw_name[MAX_STR_LEN * 4];
	char fw_name_mp[MAX_STR_LEN * 4];
#ifndef CONFIG_OF
	struct iomux_block *tp_gpio_block;
	struct block_config *tp_gpio_block_config;
#else
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
#endif
	struct mutex bus_mutex;
	struct i2c_client *client;
	struct spi_device *spi;
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
	uint8_t max_touch_num;
	uint8_t max_button_num;
	uint32_t int_trigger_type;
	int32_t irq_gpio;
	uint32_t irq_flags;
	int32_t reset_gpio;
	uint32_t support_aft;
	uint32_t reset_flags;
	int32_t disp_rst_gpio;
	uint32_t disp_rst_flags;
	bool print_criteria;
	uint32_t noise_test_frame;
	uint32_t open_test_by_fw;
	uint32_t project_id_flash_address;
	uint32_t csvfile_use_system;
	uint32_t test_capacitance_via_csvfile;
	uint32_t i2c_retry_time;
	bool criteria_threshold_flag;
	bool nvttddi_channel_flag;
	uint32_t *IC_X_CFG_SIZE;
	uint32_t *IC_Y_CFG_SIZE;
	uint32_t *NvtTddi_X_Channel;
	uint32_t *NvtTddi_Y_Channel;
	uint32_t *PS_Config_Lmt_FW_CC_P;
	uint32_t *PS_Config_Lmt_FW_CC_N;
	uint32_t *PS_Config_Lmt_FW_Diff_P;
	int32_t *PS_Config_Lmt_FW_Diff_N;
	uint32_t *PS_Config_Lmt_Short_Rawdata_P;
	uint32_t *PS_Config_Lmt_Short_Rawdata_N;
	uint32_t *mADCOper_Cnt;
	uint32_t *PS_Config_Lmt_FW_Rawdata_P;
	uint32_t *PS_Config_Lmt_FW_Rawdata_N;
	uint32_t *PS_Config_Lmt_FW_Rawdata_X_Delta;
	uint32_t *PS_Config_Lmt_FW_Rawdata_Y_Delta;
	uint32_t *PS_Config_Lmt_Open_Rawdata_P_A;
	uint32_t *PS_Config_Lmt_Open_Rawdata_N_A;
	uint32_t *NVT_TDDI_AIN_X;
	uint32_t *NVT_TDDI_AIN_Y;
	struct mutex lock;
	const struct nvt_ts_mem_map *mmap;
	uint8_t carrier_system;
	uint8_t rbuf[RBUF_LEN];
	uint8_t power_sleep_mode;
	bool gesture_module;
	const char *default_project_id;
	int use_dma_download_firmware;
	int use_pinctrl;
	int tp_status_report_support;
	uint16_t abnormal_status;
	int in_suspend;
	int rawdate_pointer_to_pointer;
	enum ts_bus_type btype;
};

#if NVT_TOUCH_PROC
struct nvt_flash_data{
	rwlock_t lock;
	struct i2c_client *client;
};
#endif

typedef enum {
	RESET_STATE_INIT = 0xA0,// IC reset
	RESET_STATE_REK,		// ReK baseline
	RESET_STATE_REK_FINISH,	// baseline is ready
	RESET_STATE_NORMAL_RUN,	// normal run
	RESET_STATE_MAX  = 0xAF
} RST_COMPLETE_STATE;

typedef enum {
    EVENT_MAP_HOST_CMD                      = 0x50,
    EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE   = 0x51,
    EVENT_MAP_RESET_COMPLETE                = 0x60,
    EVENT_MAP_FWINFO                        = 0x78,
    EVENT_MAP_PROJECTID                     = 0x9A,
} SPI_EVENT_MAP;

//---SPI READ/WRITE---
#define SPI_WRITE_MASK(a)	(a | 0x80)
#define SPI_READ_MASK(a)	(a & 0x7F)

#define DUMMY_BYTES (1)
#define NVT_TANSFER_LEN		(PAGE_SIZE << 4)

typedef enum {
	NVTWRITE = 0,
	NVTREAD  = 1
} NVT_SPI_RW;

#define LOW_EIGHT_BITS(x)			((x)&0xFF)
#define MIDDLE_EIGHT_BITS(x)		(((x)&0xFF00)>>8)
#define HIGHT_EIGHT_BITS(x)			(((x)&0xFF0000)>>16)
#define NVTTDDI_ERR	-1
#define NVTTDDI_IC_X_CFG_SIZE	18
#define NVTTDDI_IC_Y_CFG_SIZE	32
#define NVT_TDDI_IC_ARRAY_SIZE	40
#define NVTTDDI_X_CHANNEL_NUM	18
#define NVTTDDI_Y_CHANNEL_NUM	32
#define NVTTDDI_TWO_BYTES_LENGTH	2
#define NVTTDDI_THREE_BYTES_LENGTH	3
#define NVTTDDI_FIVE_BYTES_LENGTH	5
#define NVTTDDI_DOUBLE_ZERO_CMD	0x00
#define NVTTDDI_ZERO_ONE_CMD		0x01
#define NVTTDDI_ZERO_TWO_CMD		0x02
#define NVTTDDI_ZERO_FIVE_CMD		0x05
#define NVTTDDI_ZERO_SIX_CMD			0x06
#define NVTTDDI_ONE_E_CMD			0x1E
#define NVTTDDI_TWO_ZERO_CMD		0x20
#define NVTTDDI_FOUR_FIVE_CMD		0x45
#define NVTTDDI_FOUR_SEVEN_CMD		0x47
#define NVTTDDI_FIVE_ZERO_CMD		0x50
#define NVTTDDI_DOUBLE_A_CMD		0xAA
#define NVTTDDI_DOUBLE_F_CMD		0xFF
#define NVTTDDI_DELAY_10_MS			10
#define NVTTDDI_DELAY_20_MS			20
#define NVTTDDI_DELAY_30_MS			30
#define NVTTDDI_RETRY_5_TIMES		5
#define NVTTDDI_FRAME_NUMBER		1
#define NVTTDDI_TEST_FRAME_DIVIDE_NUM	10
#define NVTTDDI_MULTIPLY_7_NUM		7
#define NVTTDDI_MULTIPLY_2_NUM		2
#define NVTTDDI_MULTIPLY_256_NUM		256
#define NVTTDDI_PLUS_ONE				1

/*gesture mode*/

#define DOUBLE_CLICK_WAKEUP	15
//const uint16_t gesture_key_array = KEY_POWER;
//static struct wake_lock gestrue_wakelock;
#define EVENT_MAP_HOST_CMD                    0x50
#define IS_APP_ENABLE_GESTURE(x)  ((u32)(1<<x))

#define FLAG_EXIST	1
enum {
	BIT0_GND_CONNECTION = 0,
	BIT1_TX_SNS_CH,
	BIT2_RX_SNS_CH,
	BIT3_PIXEL_SNS,
	BIT4_DISPLAY_NOISE,
	BIT5_CHARGER_NOISE,
	BIT6_CHARGER_NOISE_HOP,
	BIT7_CHARGER_NOISE_EX,
	BIT8_SELF_CAP_NOISE,
	BIT9_MUTUAL_CAP_NOISE,
	BIT10_HIGH_TEMP,
	BIT11_LOW_TEMP,
	BIT12_LARGE_BENDING,
	BIT13_RESERVED,
	BIT14_RESERVED,
	BIT15_RESERVED,
	BIT_MAX,
};

struct dmd_report_charger_status{
	int charge_CHARGER_NOISE_HOP;
	int charge_CHARGER_NOISE_EX;
};
struct tp_status_and_count{
	int bit_status;
	unsigned int bit_count;
};

//---extern functions---
int32_t novatek_ts_kit_read(uint16_t i2c_addr, uint8_t *buf, uint16_t len);
int32_t novatek_ts_kit_write(uint16_t i2c_addr, uint8_t *buf, uint16_t len);
void nvt_kit_bootloader_reset(void);
void nvt_kit_sw_reset(void);
void nvt_kit_sw_reset_idle(void);
void nvt_boot_ready(void);
void nvt_bld_crc_enable(void);
void nvt_fw_crc_enable(void);
int32_t nvt_kit_check_fw_reset_state(RST_COMPLETE_STATE check_reset_state);
int8_t nvt_kit_get_fw_info(void);
int32_t nvt_kit_clear_fw_status(void);
int32_t nvt_kit_check_fw_status(void);

int32_t nvt_set_page(uint32_t addr);
int32_t nvt_write_addr(uint32_t addr, uint8_t data);

int32_t nvt_kit_fw_update_boot(char *file_name);
int32_t nvt_kit_fw_update_boot_spi(char *file_name);
void Boot_Update_Firmware(struct work_struct *work);

#endif /* _LINUX_NVT_TOUCH_H */
