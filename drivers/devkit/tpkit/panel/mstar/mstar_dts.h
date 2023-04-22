/*******************************************************************
        Copyright 2008 - 2013, Huawei Tech. Co., Ltd.
        ALL RIGHTS RESERVED

Filename      : ilitek_dts.h
Author        :
Creation time : 2016/10/24
Description   :

Version       : 1.0
********************************************************************/
#ifndef __MSTAR_DTS_H__
#define __MSTAR_DTS_H__

#include <linux/i2c.h>
#include "mstar_common.h"

#define MSTAR_CHIP_NAME     "mstar"
#define HUAWEI_TS_KIT       "huawei,ts_kit"

#define MSTAR_IRQ_GPIO      "attn_gpio"
#define MSTAR_RST_GPIO      "reset_gpio"
#define MSTAR_VDDIO_GPIO_CTRL   "vddio_ctrl_gpio"
#define MSTAR_VCI_GPIO_CTRL "vci_ctrl_gpio"
#define MSTAR_IRQ_CFG       "mstar_irq_config"
#define MSTAR_ALGO_ID       "algo_id"
#define MSTAR_IC_TYPES      "ic_type"
#define MSTAR_WD_CHECK      "need_wd_check_status"
#define MSTAR_WD_CHECK_TIMEOUT "check_status_watchdog_timeout"
#define MSTAR_X_MAX     "x_max"
#define MSTAR_Y_MAX     "y_max"
#define MSTAR_VCI_GPIO_TYPE     "vci_gpio_type"
#define MSTAR_VCI_REGULATOR_TYPE        "vci_power_type"
#define MSTAR_VDDIO_GPIO_TYPE       "vddio_gpio_type"
#define MSTAR_VDDIO_REGULATOR_TYPE  "vddio_power_type"
#define MSTAR_COVER_FORCE_GLOVE     "force_glove_in_smart_cover"
#define MSTAR_TEST_TYPE         "tp_test_type"
#define MSTAR_HOSTLER_SWITCH_ADDR       "holster_switch_addr"
#define MSTAR_HOSTLER_SWITCH_BIT        "holster_switch_bit"
#define MSTAR_GLOVE_SWITCH_ADDR     "glove_switch_addr"
#define MSTAR_GLOVE_SWITCH_BIT      "glove_switch_bit"
#define MSTAR_PROJECTID_LEN  "projectid_len"
#define MSTAR_VCI_LDO_VALUE     "vci_ldo_value"
#define MSTAR_VDDIO_LDO_VALUE       "vddio_ldo_value"
#define MSTAR_NEED_SET_VDDIO_VALUE  "vddio_ldo_need_set"
#define MSTAR_UNIT_CAP_TEST_INTERFACE "unite_cap_test_interface"
#define MSTAR_REPORT_RATE_TEST "report_rate_test"
#define MSTAR_FW_UPDATE_LOGIC       "fw_update_logic"

#define MSTAR_HARD_RESET_DELAY      "hard_reset_delay"
#define MSTAR_ERASE_MIN_DELAY       "erase_min_delay"
#define MSTAR_CALC_CRC_DELAY        "calc_crc_delay"
#define MSTAR_REBOOT_DELAY      "reboot_delay"
#define MSTAR_ERASE_QUERY_DELAY     "erase_query_delay"

#define MSTAR_WRITE_FLASH_QUERY_TIMES   "write_flash_query_times"
#define MSTAR_READ_ECC_QUERY_TIMES  "read_ecc_query_times"
#define MSTAR_ERASE_FLASH_QUERY_TIMES   "erase_flash_query_times"
#define MSTAR_UPGRADE_LOOP_TIMES        "upgrade_loop_times"
#define MSTAR_SLAVE_ADDR            "slave_address"

#define DTS_RAW_DATA_MIN        "threshold,raw_data_min"
#define DTS_RAW_DATA_MAX        "threshold,raw_data_max"
#define DTS_RX_TEST         "threshold,rx_test"
#define DTS_TX_TEST         "threshold,tx_test"
#define DTS_NOISE_TEST          "threshold,noise_test"
#define DTS_SHORT_TEST  "threshold,short_test"
#define DTS_OPEN_TEST       "threshold,open_test"
#define MSTAR_TEST_TYPE_DEFAULT "Normalize_type:judge_last_result"
#define MSTAR_RAWDATA_TIMEOUT "rawdata_timeout"
#define MSTAR_MP_FLOW "mstar_mp_flow"
#define MSTAR_MP_RETRY "mstar_mp_retry"
#define MSTAR_SUPPORT_GET_TP_COLOR    "support_get_tp_color"
#define MSTAR_GESTURE_SUPPORTED       "gesture_supported"
#define MSTAR_ROI_SUPPORTED       "roi_supported"
#define MSTAR_PINCTRL_SET       "mstar,pinctrl_set"
#define MSTAR_SELF_CTRL_POWER	"power_self_ctrl"
#define MSTAR_LCD_PANEL_NAME_FROM_LCDKIT	 "lcd_panel_name_from_lcdkit"
#define MSTAR_FW_ONLY_DEPEND_ON_LCD	 "fw_only_depend_on_lcd"

int mstar_get_vendor_compatible_name(const char *project_id, char *comp_name, size_t size);

int mstar_get_vendor_name_from_dts(const char *project_id, char *vendor_name, size_t size);

int mstar_parse_dts(struct device_node *device, struct ts_kit_device_data *chip_data);

int mstar_prase_ic_config_dts(struct device_node *np, struct ts_kit_device_data *dev_data);
int mstar_parse_cap_test_config(void);

#endif
