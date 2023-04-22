/*
 * fts.c
 *
 * FTS Capacitive touch screen controller (FingerTipS)
 *
 * Copyright (C) 2016, STMicroelectronics Limited.
 * Authors: AMG(Analog Mems Group)
 *
 * 		marco.cali@st.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef _LINUX_FTS_I2C_H_
#define _LINUX_FTS_I2C_H_

#include <linux/wakelock.h>
#include "fts_lib/ftsSoftware.h"
#include "fts_lib/ftsHardware.h"
#include "../../huawei_ts_kit.h"

#define FTS_POWER_ON     1
#define FTS_POWER_OFF    0

#define FTS_TRUE 1
#define FTS_FALSE 0

/****************** CONFIGURATION SECTION ******************/

//**** CODE CONFIGURATION ****

#define FTS_TS_DRV_NAME                     "fts"
#define FTS_TS_DRV_VERSION                  "4.2.13" //version

#define SCRIPTLESS			// allow to work in scriptless mode with the GUI
#ifdef SCRIPTLESS
#define SCRIPTLESS_DEBUG 		// uncomment this macro definition to print debug  message for script less  support
#endif

#define DRIVER_TEST

//#define PHONE_GESTURE			//allow to use the gestures
#ifdef PHONE_GESTURE
	#define USE_GESTURE_MASK
	#define USE_CUSTOM_GESTURES
#endif

#define USE_ONE_FILE_NODE		//allow to enable/disable all the features just using one file node

//#define EDGE_REJ			//allow edge rej feature (comment to disable)

//#define CORNER_REJ			//allow corn rej feature (comment to disable)

//#define EDGE_PALM_REJ			//allow edge palm rej feature (comment to disable)

#define CHARGER_MODE			//allow charger mode feature (comment to disable)

#define GLOVE_MODE			//allow glove mode feature (comment to disable)
#define COVER_MODE			//allow cover mode feature (comment to disable)

//#define USE_NOISE_PARAM			//set noise params during resume (comment to disable)

//**** END ****

//**** PANEL SPECIFICATION ****
#define X_AXIS_MAX                          1440
#define X_AXIS_MIN                          0
#define Y_AXIS_MAX                          2560
#define Y_AXIS_MIN                          0

#define PRESSURE_MIN                        0
#define PRESSURE_MAX                        127

#define TOUCH_ID_MAX                        10

#define AREA_MIN                            PRESSURE_MIN
#define AREA_MAX                            PRESSURE_MAX
//**** END ****

/*********************************************************/




/* Flash programming */

#define INIT_FLAG_CNT                       3

/* KEYS */
#define KEY1		0x02
#define KEY2		0x01
#define KEY3		0x04

/*
 * Configuration mode
 */
//bitmask which can assume the value defined as features in ftsSoftware.h or the following values

#define MODE_NOTHING				0x00000000
#define MODE_SENSEON 	                        0x10000000
#define MODE_SENSEOFF				0x20000000
#define FEAT_GESTURE				0x40000000


/*
 * Status Event Field:
 *     id of command that triggered the event
 */
#define FTS_FLASH_WRITE_CONFIG              0x03
#define FTS_FLASH_WRITE_COMP_MEMORY         0x04
#define FTS_FORCE_CAL_SELF_MUTUAL           0x05
#define FTS_FORCE_CAL_SELF                  0x06
#define FTS_WATER_MODE_ON                   0x07
#define FTS_WATER_MODE_OFF                  0x08


#define EXP_FN_WORK_DELAY_MS  		1000

#define CMD_STR_LEN			32

#ifdef SCRIPTLESS
/*
 * I2C Command Read/Write Function
 */

#define CMD_RESULT_STR_LEN     2048
#endif

#define TSP_BUF_SIZE           4096

/* lock data type */
#define PROJECT_ID_TYPE		0x00
#define BARCODE_TYPE 		0x01
#define BRIGHTNESS_TYPE		0x02


/* calibrate type */
#define TOUCH_CALIBRATE_TYPE 0x01
#define PRESSURE_CALIBRATE_TYPE 0x02

struct fts_i2c_platform_data {
	int (*power) (bool on);
	int irq_gpio;
	int reset_gpio;
	const char *pwr_reg_name;
	const char *bus_reg_name;
};

/*
 * Forward declaration
 */
struct fts_ts_info;

#define ST_IRQ_CFG "irq_config"
#define ST_ALGO_ID "algo_id"
#define ST_X_MAX	 "x_max"
#define ST_Y_MAX	 "y_max"
#define ST_X_MAX_MT	 "x_max_mt"
#define ST_Y_MAX_MT	 "y_max_mt"
#define ST_VCI_GPIO_TYPE	 "vci_gpio_type"
#define ST_VCI_PMIC_TYPE	 "vci_pmic_type"
#define ST_VCI_REGULATOR_TYPE	 "vci_regulator_type"
#define ST_VDDIO_GPIO_TYPE	 "vddio_gpio_type"
#define ST_VDDIO_PMIC_TYPE	 "vddio_pmic_type"
#define ST_VDDIO_REGULATOR_TYPE	 "vddio_regulator_type"
#define ST_IOVDD_VOL_VALUE	 "iovdd_vol_value"
#define ST_AVDD_VOL_VALUE	 "avdd_vol_value"
#define ST_COVER_FORCE_GLOVE	 "force_glove_in_smart_cover"
#define ST_VCI_GPIO_CTRL "vci_ctrl_gpio"
#define ST_VDDIO_GPIO_CTRL "vddio_ctrl_gpio"
#define ST_COVER_FORCE_GLOVE	 "force_glove_in_smart_cover"

#define ST_TS_IOVCC_NAME "st-io"
#define ST_TS_AVCC_NAME "st-vci"

#define ST_FW_PATH_SD "ts/touch_screen_firmware.img"

#define ST_FW_NAME_MAX_LEN 50
#define ST_PROJECT_ID_LEN 10

#define GLOVE_SWITCH_ON 1
#define GLOVE_SWITCH_OFF 0

#define RAWDATA_LIMIT_NUM	20
#define FTS_BARCODE_SIZE 39
#define LOCKDOWN_2D_BAR_INFO_HEAD_LEN 2

/*
 * Dispatch event handler
 */
typedef void (*event_dispatch_handler_t)
						(struct fts_ts_info *info, unsigned char *data);

/*
 * struct fts_ts_info - FTS capacitive touch screen device information
 * @dev:                  Pointer to the structure device
 * @client:               I2C client structure
 * @input_dev             Input device structure
 * @event_dispatch_table  Event dispatch table handlers
 * @attrs                 SysFS attributes
 * @mode                  Device operating mode (bitmask)
 * @touch_id              Bitmask for touch id (mapped to input slots)
 * @power                 Power on/off routine
 * @bdata                 HW info retrived from device tree
 * @pwr_reg               DVDD power regulator
 * @bus_reg               AVDD power regulator
 * @resume_bit            Indicate if screen off/on
 * @series of switches    to store the enabling status of a particular feature from the host
 */
struct fts_ts_info{
	struct device            *dev;
	struct i2c_client        *client;
	struct input_dev         *input_dev;
	event_dispatch_handler_t *event_dispatch_table;
	struct attribute_group    attrs;
	struct ts_kit_device_data *chip_data;
	struct ts_fingers *fingers_info;
	char project_id[ST_PROJECT_ID_LEN + 1];
	char barcode[ FTS_BARCODE_SIZE + LOCKDOWN_2D_BAR_INFO_HEAD_LEN + 1];
	int barcode_status ;/* 0:read success ; <0 read fail*/
	unsigned int              mode;
	unsigned long             touch_id;
#ifdef SCRIPTLESS
	/*I2C cmd*/
	struct device *i2c_cmd_dev;
	char cmd_read_result[CMD_RESULT_STR_LEN];
	char cmd_wr_result[CMD_RESULT_STR_LEN];
	char cmd_write_result[20];
#endif
#ifdef DRIVER_TEST
	struct device *test_cmd_dev;
#endif
	struct fts_i2c_platform_data bdata;
	struct regulator *pwr_reg;
	struct regulator *bus_reg;
	int resume_bit;
	/* switches for features */
	int gesture_enabled;
	int glove_enabled;
	int charger_enabled;
	int cover_enabled;
	int edge_rej_enabled;
	int corner_rej_enabled;
	int edge_palm_rej_enabled;
	int avdd_value;
	int iovdd_value;
	int st_raw_limit_buf[RAWDATA_LIMIT_NUM];
	char *fake_project_id;
	int check_MutualRawGap_after_callibrate;
	struct firmware *fw ;
};



int fts_chip_powercycle(struct fts_ts_info *info);
int st_get_rawdata_aftertest(struct ts_rawdata_info *info,u32 signature);

#ifdef SCRIPTLESS
extern struct attribute_group i2c_cmd_attr_group;
#endif

#ifdef DRIVER_TEST
extern struct attribute_group test_cmd_attr_group;
#endif
extern struct fts_ts_info* fts_get_info(void);
extern int st_get_project_id(struct fts_ts_info *fts_info);

#endif
