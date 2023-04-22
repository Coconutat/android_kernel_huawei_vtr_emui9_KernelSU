

#ifndef __GOODIX_DTS_H__
#define __GOODIX_DTS_H__

#include <linux/i2c.h>


#define GTP_VENDOR_COMP_NAME_LEN	32

#define GTP_GT1X_CHIP_NAME	"gt1x"
#define HUAWEI_TS_KIT		"huawei,ts_kit"

#define GTP_IRQ_CFG		"irq_config"
#define GTP_ALGO_ID		"algo_id"
#define GTP_IC_TYPES		"ic_type"
#define GTP_WD_CHECK		"need_wd_check_status"
#define GTP_WD_TIMEOUT		"check_status_watchdog_timeout"
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
#define GTP_HOLSTER_SUPPORTED		"holster_mode_supported"
#define GTP_HOSTLER_SWITCH_ADDR		"holster_switch_addr"
#define GTP_HOSTLER_SWITCH_BIT		"holster_switch_bit"
#define GTP_GLOVE_SUPPORTED		"glove_mode_supported"
#define GTP_GLOVE_SWITCH_ADDR		"glove_switch_addr"
#define GTP_GLOVE_SWITCH_BIT		"glove_switch_bit"
#define GTP_ROI_SUPPORTED		"roi_supported"
#define GTP_ROI_SWITCH_ADDR		"roi_switch_addr"
#define GTP_ROI_DATA_SIZE		"roi_data_size"
#define GTP_GESTURE_SUPPORTED		"gesture_supported"
#define GTP_PRAM_PROJECTID_ADDR		"pram_projectid_addr"
#define GTP_PROJECT_ID			"project_id"
#define GTP_IC_TYPE			"ic_type"
#define GTP_EASY_WAKE_SUPPORTED		"easy_wakeup_supported"
#define GTP_CREATE_PROJECT_ID		"create_project_id_flag"
#define GTP_TOOL_SUPPORT		"tools_support"
#define GTP_CHARGER_SUPPORT		"charger_supported"
#define GTP_TP_TEST_TYPE		"tp_test_type"
#define GTP_CHIP_NAME			"chip_name"
#define GTP_MODULE_VENDOR		"module_vendor"
#define GTP_EDGE_ADD		"gt1x_edge_add"
#define GTP_SUPPORT_EDGE_XYER	"gt1x_support_edge_xyer"
#define GTP_ROI_DATA_ADD		"gt1x_roi_data_add"
#define GTP_ROI_FW_SUPPORTED	"gt1x_roi_fw_supported"
#define GTP_PANEL_ID	"panel_id"
#define GTP_TOUCH_SWITH_FLAG	"touch_switch_flag"

#define GTP_POWER_SELF_CTRL		"power_self_ctrl"
#define GTP_VCI_LDO_VALUE		"vci_ldo_value"
#define GTP_VDDIO_LDO_VALUE		"vddio_ldo_value"
#define GTP_VCI_POWER_TYPE		"vci_power_type"
#define GTP_VDDIO_POWER_TYPE		"vddio_power_type"
#define GTP_NEED_SET_VDDIO_VALUE	"vddio_ldo_need_set"

#define GTP_FW_UPDATE_LOGIC		"fw_update_logic"
#define GT1X_SLAVE_ADDR			"slave_address"

#define GTP_SUPPORT_WXY			"gt1x_support_wxy"
#define GTP_WXY_DATA_ADD		"gt1x_wxy_data_add"

#define GT1X_INIT_DELAY "gt1x_init_delay"
#define GT1X_SUPPORT_TP_COLOR			"support_get_tp_color"
#define GT1X_PROVIDE_PANEL_ID_SUPPORT			"provide_panel_id_suppot"
#define GT1X_OPEN_ONCE_THRESHOLD		"only_open_once_captest_threshold"
#define FW_ONLY_DEPEND_ON_LCD					"fw_only_depend_on_lcd"
#define GTP_BOOT_PROJ_CODE_ADDR2	0x20
#define GT1X_DEFAULT_SLAVE_ADDR		0x14

#endif
