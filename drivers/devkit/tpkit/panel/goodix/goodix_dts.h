

#ifndef __GOODIX_DTS_H__
#define __GOODIX_DTS_H__

#include <linux/i2c.h>


#define GTP_VENDOR_COMP_NAME_LEN	32

#define GTP_CHIP_NAME		"goodix"
#define HUAWEI_TS_KIT		"huawei,ts_kit"

#define GTP_IRQ_CFG		"irq_config"
#define GTP_ALGO_ID		"algo_id"
#define GTP_IC_TYPES		"ic_type"
#define GTP_WD_CHECK		"need_wd_check_status"
#define GTP_WD_TIMEOUT         "check_status_watchdog_timeout"
#define GTP_X_MAX		"x_max"
#define GTP_Y_MAX		"y_max"
#define GTP_X_MAX_MT		"x_max_mt"
#define GTP_Y_MAX_MT		"y_max_mt"
#define GTP_VCI_GPIO_TYPE		"vci_gpio_type"
#define GTP_VCI_REGULATOR_TYPE		"vci_regulator_type"
#define GTP_VDDIO_GPIO_TYPE		"vddio_gpio_type"
#define GTP_VDDIO_REGULATOR_TYPE	"vddio_regulator_type"
#define GTP_COVER_FORCE_GLOVE		"force_glove_in_smart_cover"
#define GTP_TEST_TYPE			"tp_test_type"
#define GTP_TEST_TYPE_DEFAULT	"Normalize_type:judge_different_reslut"
#define GTP_HOLSTER_SUPPORTED			"holster_supported"
#define GTP_HOSTLER_SWITCH_ADDR		"holster_switch_addr"
#define GTP_HOSTLER_SWITCH_BIT		"holster_switch_bit"
#define GTP_GLOVE_SUPPORTED			"glove_supported"
#define GTP_GLOVE_SWITCH_ADDR		"glove_switch_addr"
#define GTP_GLOVE_SWITCH_BIT		"glove_switch_bit"
#define GTP_ROI_SUPPORTED			"roi_supported"
#define GTP_ROI_SWITCH_ADDR		"roi_switch_addr"
#define GTP_GESTURE_SUPPORTED		"gesture_supported"
#define GTP_PRAM_PROJECTID_ADDR		"pram_projectid_addr"

#define GTP_VCI_LDO_VALUE		"vci_value"
#define GTP_VDDIO_LDO_VALUE		"vddio_value"
#define GTP_NEED_SET_VDDIO_VALUE	"need_set_vddio_value"

#define GTP_FW_UPDATE_LOGIC		"fw_update_logic"
#define GOODIX_SLAVE_ADDR			"slave_address"

#define TEST_CAPACITANCE_VIA_CSVFILE "huawei,test_capacitance_via_csvfile"
#define CSVFILE_USE_PRODUCT_SYSTEM_TYPE "huawei,csvfile_use_product_system"
#define GTP_OPEN_ONCE_THRESHOLD		"only_open_once_captest_threshold"

#define GTP_LOAD_CFG_VIA_PROJECT_ID "load_cfg_via_project_id"

#define GTP_BOOT_PROJ_CODE_ADDR2	 0x20

int goodix_parse_dts(struct goodix_ts_data *ts);
int goodix_parse_specific_dts(struct goodix_ts_data *ts);
int goodix_parse_cfg_data(struct goodix_ts_data *ts,
				char *cfg_type, u8 *cfg, int *cfg_len, u8 sid);
int goodix_chip_parse_config(struct device_node *device,
				struct ts_kit_device_data *chip_data);

#endif
