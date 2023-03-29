#ifndef __ILITEK_DTS_H__
#define __ILITEK_DTS_H__

#define ILITEK_DTS_NODE                        "ilitek,ts_kit"
#define ILITEK_DTS_HW_TS_NODE                  "huawei,ts_kit"

#define ILITEK_DTS_IC_TYEP                     "ic_type"
#define ILITEK_DTS_CHIP_ID                     "chip_id"
#define ILITEK_DTS_SLAVE_ADDR                  "slave_address"

#define ILITEK_DTS_ALGO_ID                     "algo_id"
#define ILITEK_DTS_IS_IN_CELL                  "is_in_cell"
#define ILITEK_DTS_IRQ_CFG                     "irq_config"
#define ILITEK_DTS_USE_IC_RES                  "use_ic_res"

#define ILITEK_DTS_X_MAX                       "x_max"
#define ILITEK_DTS_X_MIN                       "x_min"
#define ILITEK_DTS_Y_MAX                       "y_max"
#define ILITEK_DTS_Y_MIN                       "y_min"
#define ILITEK_DTS_FINGER_NUMS                 "finger_nums"

#define ILITEK_DTS_ROI                         "support_roi"
#define ILITEK_DTS_GESTURE                     "support_gesture"
#define ILITEK_DTS_PRESSURE                    "support_pressure"
#define ILITEK_DTS_TP_COLOR                    "support_get_tp_color"

#define ILITEK_DTS_LCD_POWER_NOTIFY            "use_lcdkit_power_notify"
#define ILITEK_DTS_PROJECT_ID_CTRL             "project_id_length_control"

#define ILITEK_DTS_RAW_DATA_PRINT              "is_ic_rawdata_proc_printf"
#define ILITEK_DTS_OPEN_ONCE_THRESHOLD         "only_open_once_captest_threshold"
#define ILITEK_DTS_TEST_TYPE                   "tp_test_type"

/* ic special config */
#define ILITEK_DTS_PRODUCER                    "producer"

/* default config if dts not config */
#define ILITEK_DTS_DEF_ALGO                    1
#define ILITEK_DTS_DEF_IS_IN_CELL              1
#define ILITEK_DTS_DEF_IRQ_CONFIG              3/* fall edge*/
#define ILITEK_DTS_DEF_USE_IC_RES              0

#define ILITEK_DTS_DEF_SUPPORT_ROI             0
#define ILITEK_DTS_DEF_SUPPORT_GESTURE         0
#define ILITEK_DTS_DEF_SUPPORT_PRESSURE        0
#define ILITEK_DTS_DEF_TP_COLOR                1

#define ILITEK_DTS_DEF_LCD_POWER_NOTIFY        1 /* tddi close fb_notify log */
#define ILITEK_DTS_DEF_PROJECT_ID_CTRL         0

#define ILITEK_DTS_DEF_RAWDATA_PRINT           1
#define ILITEK_DTS_DEF_TH_OPEN_ONCE            0

#define ILITEK_DTS_DEF_PRODUCER                "unknown"
#define ILITEK_DTS_DEF_TEST_TYPE               "Normalize_type:judge_last_result"

#define ILITEK_DTS_PIN_ACTIVE                  "active"
#define ILITEK_DTS_PIN_DEFAULT                 "default"
#define ILITEK_DTS_PIN_IDLE                    "idle"

int ilitek_prase_ic_config_dts(struct device_node *np);
int ilitek_parse_ic_special_dts(struct device_node *np);
int ilitek_parse_dts(struct device_node *np);
#endif
