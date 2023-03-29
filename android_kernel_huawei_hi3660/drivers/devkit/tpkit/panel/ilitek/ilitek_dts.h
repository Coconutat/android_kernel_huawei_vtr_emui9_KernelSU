/*******************************************************************
		Copyright 2008 - 2013, Huawei Tech. Co., Ltd.
		ALL RIGHTS RESERVED

Filename      : ilitek_dts.h
Author        :
Creation time : 2016/10/24
Description   :

Version       : 1.0
********************************************************************/
#ifndef __ILITEK_DTS_H__
#define __ILITEK_DTS_H__

#include <linux/i2c.h>
#include "ilitek_ts.h"

#define ILITEK_CHIP_NAME		"ilitek"
#define HUAWEI_TS_KIT		"huawei,ts_kit"

#define ILITEK_IRQ_GPIO		"attn_gpio"
#define ILITEK_RST_GPIO		"reset_gpio"
#define ILITEK_VDDIO_GPIO_CTRL	"vddio_ctrl_gpio"
#define ILITEK_VCI_GPIO_CTRL	"vci_ctrl_gpio"
#define ILITEK_IRQ_CFG		"irq_config"
#define ILITEK_ALGO_ID		"algo_id"
#define ILITEK_VDD			"ilitek-vdd"
#define ILITEK_VBUS		"ilitek-io"
#define ILITEK_IC_TYPES		"ic_type"
#define ILITEK_WD_CHECK		"need_wd_check_status"
#define ILITEK_X_MAX		"x_max"
#define ILITEK_Y_MAX		"y_max"
#define ILITEK_VCI_GPIO_TYPE		"vci_gpio_type"
#define ILITEK_VCI_REGULATOR_TYPE		"vci_regulator_type"
#define ILITEK_VDDIO_GPIO_TYPE		"vddio_gpio_type"
#define ILITEK_VDDIO_REGULATOR_TYPE	"vddio_regulator_type"
#define ILITEK_COVER_FORCE_GLOVE		"force_glove_in_smart_cover"
#define ILITEK_TEST_TYPE			"tp_test_type"
#define ILITEK_HOSTLER_SWITCH_ADDR		"holster_switch_addr"
#define ILITEK_HOSTLER_SWITCH_BIT		"holster_switch_bit"
#define ILITEK_GLOVE_SWITCH_ADDR		"glove_switch_addr"
#define ILITEK_GLOVE_SWITCH_BIT		"glove_switch_bit"
#define ILITEK_PROJECTID_LEN	 "projectid_len"
#define ILITEK_VCI_LDO_VALUE		"vci_value"
#define ILITEK_VDDIO_LDO_VALUE		"vddio_value"
#define ILITEK_NEED_SET_VDDIO_VALUE	"need_set_vddio_value"
#define ILITEK_UNIT_CAP_TEST_INTERFACE "unite_cap_test_interface"
#define ILITEK_REPORT_RATE_TEST "report_rate_test"
#define ILITEK_FW_UPDATE_LOGIC		"fw_update_logic"

#define ILITEK_HARD_RESET_DELAY		"hard_reset_delay"
#define ILITEK_ERASE_MIN_DELAY		"erase_min_delay"
#define ILITEK_CALC_CRC_DELAY		"calc_crc_delay"
#define ILITEK_REBOOT_DELAY		"reboot_delay"
#define ILITEK_ERASE_QUERY_DELAY		"erase_query_delay"

#define ILITEK_WRITE_FLASH_QUERY_TIMES	"write_flash_query_times"
#define ILITEK_READ_ECC_QUERY_TIMES	"read_ecc_query_times"
#define ILITEK_ERASE_FLASH_QUERY_TIMES	"erase_flash_query_times"
#define ILITEK_UPGRADE_LOOP_TIMES		"upgrade_loop_times"
#define ILITEK_SLAVE_ADDR			"slave_address"

#define DTS_RAW_DATA_MIN		"threshold,raw_data_min"
#define DTS_RAW_DATA_MAX		"threshold,raw_data_max"
#define DTS_RX_TEST			"threshold,rx_test"
#define DTS_TX_TEST			"threshold,tx_test"
#define DTS_NOISE_TEST			"threshold,noise_test"
#define DTS_SHORT_TEST	"threshold,short_test"
#define DTS_OPEN_TEST		"threshold,open_test"
#define ILITEK_TEST_TYPE_DEFAULT	"Normalize_type:judge_last_result"
int ilitek_get_vendor_compatible_name(const char *project_id, char *comp_name, size_t size);

int ilitek_get_vendor_name_from_dts(const char *project_id, char *vendor_name, size_t size);

int ilitek_parse_dts(struct device_node *device,  struct ts_kit_device_data *chip_data);

int ilitek_prase_ic_config_dts(struct device_node *np, struct ts_kit_device_data *dev_data);
int ilitek_parse_cap_test_config(void);

#endif

