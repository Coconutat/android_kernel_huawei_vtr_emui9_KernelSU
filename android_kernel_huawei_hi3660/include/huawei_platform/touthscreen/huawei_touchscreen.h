/*
 * Huawei Touchpanel driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#ifndef __HUAWEI_TOUCHSCREEN_H_
#define __HUAWEI_TOUCHSCREEN_H_

#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/of.h>
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/wakelock.h>

#define HUAWEI_CHARGER_FB	/*define HUAWEI_CHARGER_FB here to enable charger notify callback*/
#if defined(HUAWEI_CHARGER_FB)
#include <linux/hisi/usb/hisi_usb.h>
#endif
#include <linux/semaphore.h>
/* marco define*/
#define NO_ERR 0

#define RAW_DATA_TEST_TYPE_OLD	0
#define RAW_DATA_TEST_TYPE_SINGLE_POINT	1

#define AFT_WAITQ_IDLE	 (0)
#define AFT_WAITQ_WAIT	 (1)
#define AFT_WAITQ_WAKEUP (2)
#define AFT_WAITQ_IGNORE (3)
#define ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR 1

//#define ANTI_FALSE_TOUCH_STRING_NUM 27
#define ANTI_FALSE_TOUCH_FEATURE_ALL "feature_all"
#define ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT "feature_resend_point"
#define ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT "feature_orit_support"
#define ANTI_FALSE_TOUCH_FEATURE_REBACK_BT "feature_reback_bt"
#define ANTI_FALSE_TOUCH_LCD_WIDTH "lcd_width"
#define ANTI_FALSE_TOUCH_LCD_HEIGHT "lcd_height"
#define ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT "click_time_limit"
#define ANTI_FALSE_TOUCH_CLICK_TIME_BT "click_time_bt"
#define ANTI_FALSE_TOUCH_EDGE_POISION "edge_position"
#define ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE "edge_postion_secondline"
#define ANTI_FALSE_TOUCH_BT_EDGE_X "bt_edge_x"
#define ANTI_FALSE_TOUCH_BT_EDGE_Y "bt_edge_y"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_X "move_limit_x"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_Y "move_limit_y"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T "move_limit_x_t"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T "move_limit_y_t"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT "move_limit_x_bt"
#define ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT "move_limit_y_bt"
#define ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T "edge_y_confirm_t"
#define ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T "edge_y_dubious_t"
#define ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT "edge_y_avg_bt"
#define ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT "edge_xy_down_bt"
#define ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T "edge_xy_confirm_t"
#define ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM "max_points_bak_num"
//for driver
#define ANTI_FALSE_TOUCH_DRV_STOP_WIDTH "drv_stop_width"
#define ANTI_FALSE_TOUCH_SENSOR_X_WIDTH "sensor_x_width"
#define ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH "sensor_y_width"
#define ANTI_FALSE_TOUCH_FW_FUN_SWITCH "fw_anti_false_touch_switch"

/* emui5.1 new support */
#define ANTI_FALSE_TOUCH_FEATURE_SG "feature_sg"
#define ANTI_FALSE_TOUCH_SG_MIN_VALUE "sg_min_value"
#define ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST "feature_support_list"
#define ANTI_FALSE_TOUCH_MAX_DISTANCE_DT "max_distance_dt"
#define ANTI_FALSE_TOUCH_FEATURE_BIG_DATA "feature_big_data"
#define ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION "feature_click_inhibition"
#define ANTI_FALSE_TOUCH_MIN_CLICK_TIME "min_click_time"


#define ROI
#ifdef ROI
#define ROI_HEAD_DATA_LENGTH		4
#define ROI_DATA_READ_LENGTH		102
#define ROI_DATA_SEND_LENGTH		(ROI_DATA_READ_LENGTH-ROI_HEAD_DATA_LENGTH)
#define ROI_CTRL_DEFAULT_ADDR		0x0446
#define ROI_DATA_DEFAULT_ADDR		0x0418
#endif
#define TS_DEV_NAME "huawei,touchscreen"
#define RAW_DATA_SIZE 8192
#define TS_WATCHDOG_TIMEOUT		1000

#define NO_SYNC_TIMEOUT		0
#define SHORT_SYNC_TIMEOUT		5
#define LONG_SYNC_TIMEOUT		10
#define LONG_LONG_SYNC_TIMEOUT	50

#define TS_CMD_QUEUE_SIZE  20
#define TS_MAX_FINGER 10
#define TS_ERR_NEST_LEVEL  5
#define TS_RAWDATA_BUFF_MAX 1600
#define TS_HYBRID_BUFF_MAX 100
#define TS_RAWDATA_RESULT_MAX	80
#define TS_FB_LOOP_COUNTS 100
#define TS_FB_WAIT_TIME 5

#define TS_CHIP_TYPE_MAX_SIZE 300
#define TS_CHIP_BUFF_MAX_SIZE 1024
#define TS_CHIP_WRITE_MAX_TYPE_COUNT 4
#define TS_CHIP_TYPE_RESERVED 6
#define TS_CHIP_BRIGHTNESS_TYPE 2
#define TS_CHIP_TYPE_LEN_RESERVED 5
#define TS_CHIP_NO_FLASH_ERROR 2
#define TS_CHIP_MAX_COUNT_ERROR 3
#define TS_CHIP_WRITE_ERROR 1
#define TS_CHIP_READ_ERROR 1
#define TS_CHIP_READ_OEM_INFO_ERROR 4
//For NV strucure  0.5 interface

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


#define I2C_WAIT_TIME 25	/*25ms wait period*/
#define I2C_RW_TRIES 3		/*retry 3 times*/
#define I2C_DEFAULT_ADDR 0x70
#define TS_SUSPEND_LEVEL 1
#define TS_MAX_REG_VALUE_NUM 80

#define TS_NO_KEY_PRESS  (0)
#define TS_IO_UNDEFINE  (-1)
#define TS_IRQ_CFG_UNDEFINE  (-1)

#define TS_FINGER_SUPPRESS		(1 << 1)
#define TS_FINGER_AMP			(1 << 2)
#define TS_FINGER_VECTOR		(1 << 3)
#define TS_FINGER_MOVE		(1 << 4)
#define TS_FINGER_RELEASE		(1 << 5)
#define TS_FINGER_PRESS		(1 << 6)
#define TS_FINGER_DETECT		(1 << 7)

#define TS_ALGO_FUNC_0		(1<<0)
#define TS_ALGO_FUNC_1		(1<<1)
#define TS_ALGO_FUNC_2		(1<<2)
#define TS_ALGO_FUNC_3		(1<<3)
#define TS_ALGO_FUNC_4		(1<<4)
#define TS_ALGO_FUNC_5		(1<<5)
#define TS_ALGO_FUNC_6		(1<<6)
#define TS_ALGO_FUNC_7		(1<<7)
#define TS_ALGO_FUNC_ALL	(0xFF)

#define MAX_STR_LEN 32
#define FULL_NAME_MAX_LEN	128
#define TS_CAP_TEST_TYPE_LEN 100
#define TS_GESTURE_COMMAND 0x7ff
#define TS_GESTURE_INVALID_COMMAND 0xFFFF
#define TS_GESTURE_PALM_BIT 0x0800
#define TS_GET_CALCULATE_NUM 2048
#define TS_GESTURE_INVALID_CONTROL_NO 0xFF

#define CHIP_INFO_LENGTH	16
#define RAWDATA_NUM 8
#define MAX_POSITON_NUMS 6
#ifndef CONFIG_OF
#define SUPPORT_IC_NUM 4
#define IC_NAME_UNDEF "null"
#define CHIP_FULL_NAME "touchscreen/support_chip_name_"
#define CHIP_SLAVE_ADDR_NAME "/slave_address"
#define INT_CONVERT_OFFSET	49
#endif

#define VIRTUAL_KEY_ELEMENT_SIZE	5
#define MAX_PRBUF_SIZE	PIPE_BUF
#define HWLOG_TAG	TS
HWLOG_REGIST();
#define TS_LOG_INFO(x...)		_hwlog_info(HWLOG_TAG, ##x)
#define TS_LOG_ERR(x...)		_hwlog_err(HWLOG_TAG, ##x)
#define TS_LOG_DEBUG(x...)	\
do { \
	if (g_ts_log_cfg)       \
		_hwlog_info(HWLOG_TAG, ##x);	\
} while (0)

/* external varible*/
extern u8 g_ts_log_cfg;

/* struct define*/
enum ts_cmd;
enum ts_bus_type;
enum ts_irq_config;
enum ts_action_status;
enum ts_dev_state;
struct ts_finger;
struct ts_fingers;
struct ts_algo_func;
struct algo_param;
struct fw_param;
struct ts_rawdata_info;
struct ts_chip_info_param;
struct ts_calibrate_info;
struct ts_glove_info;
struct ts_holster_info;
struct ts_hand_info;
struct ts_feature_info;
struct ts_dsm_info;
struct ts_cmd_param;
struct ts_cmd_node;
struct ts_cmd_queue;
struct ts_device_ops;
struct ts_device_data;
struct ts_bus_info;
struct ts_data;
struct ts_diff_data_info;

enum ts_cmd {
	TS_INT_PROCESS = 0,
	TS_INPUT_ALGO,
	TS_REPORT_INPUT,
	TS_POWER_CONTROL,
	TS_FW_UPDATE_BOOT,
	TS_FW_UPDATE_SD,
	TS_OEM_INFO_SWITCH,
	TS_GET_CHIP_INFO,
	TS_READ_RAW_DATA,
	TS_CALIBRATE_DEVICE,
	TS_CALIBRATE_DEVICE_LPWG,
	TS_DSM_DEBUG,
	TS_CHARGER_SWITCH,
	TS_GLOVE_SWITCH,
	TS_HAND_DETECT,
	TS_FORCE_RESET,
	TS_INT_ERR_OCCUR,
	TS_ERR_OCCUR,
	TS_CHECK_STATUS,
	TS_TEST_CMD,
	TS_HOLSTER_SWITCH,
	TS_WAKEUP_GESTURE_ENABLE,
	TS_ROI_SWITCH,
	TS_TOUCH_WINDOW,
	TS_PALM_SWITCH,
	TS_REGS_STORE,
	TS_SET_INFO_FLAG,
	TS_TOUCH_WEIGHT_SWITCH,
	TS_TEST_TYPE,
	TS_HARDWARE_TEST,
	TS_DEBUG_DATA,
	TS_READ_CALIBRATION_DATA,
	TS_GET_CALIBRATION_INFO,
	TS_READ_BRIGHTNESS_INFO,
	TS_TP_INIT,
	TS_INVAILD_CMD = 255,
};

enum ts_pm_type {
	TS_BEFORE_SUSPEND = 0,
	TS_SUSPEND_DEVICE,
	TS_RESUME_DEVICE,
	TS_AFTER_RESUME,
	TS_IC_SHUT_DOWN,
};

enum ts_bus_type {
	TS_BUS_I2C = 0,
	TS_BUS_SPI,
	TS_BUS_UNDEF = 255,
};

enum ts_irq_config {
	TS_IRQ_LOW_LEVEL,
	TS_IRQ_HIGH_LEVEL,
	TS_IRQ_RAISE_EDGE,
	TS_IRQ_FALL_EDGE,
};

enum ts_action_status {
	TS_ACTION_READ,
	TS_ACTION_WRITE,
	TS_ACTION_SUCCESS,
	TS_ACTION_FAILED,
	TS_ACTION_UNDEF,
};

enum ts_dev_state {
	TS_UNINIT = 0,
	TS_SLEEP,
	TS_WORK,
	TS_WORK_IN_SLEEP,
	TS_MMI_CAP_TEST,
	TS_STATE_UNDEFINE = 255,
};

enum ts_esd_state {
	TS_NO_ESD = 0,
	TS_ESD_HAPPENDED,
};

enum ts_rawdata_arange_type{
	TS_RAWDATA_TRANS_NONE = 0,
	TS_RAWDATA_TRANS_ABCD2CBAD,
	TS_RAWDATA_TRANS_ABCD2ADCB,
};

#if defined (CONFIG_HUAWEI_DSM)
enum FW_uptate_state {
	fwu_enter_flash_prog_fail = 0,
	fwu_write_bootloader_id_fail,
	fwu_write_f34_command_fail,
	fwu_wait_for_idle_fail,
	fwu_write_firmware_fail,
	fwu_write_configuration_fail,
	TS_UPDATE_STATE_UNDEFINE = 255,
};
#endif
enum ts_gesture_num {
/*      TS_NUM_TOTAL = 12, /* total gesture numbers  */
	TS_DOUBLE_CLICK = KEY_F1,	/*0.Double tap:KEY_F1 */
	TS_SLIDE_L2R = KEY_F2,	/*1.Single finger slide from left to right:KEY_F2 */
	TS_SLIDE_R2L = KEY_F3,	/*2.Single finger slide from right to left:KEY_F3 */
	TS_SLIDE_T2B = KEY_F4,	/*3.Single finger slide from top to bottom:KEY_F4 */
	TS_SLIDE_B2T = KEY_F5,	/*4.Single finger slide from bottom to top:KEY_F5 */
	TS_CIRCLE_SLIDE = KEY_F7,	/*5.Single finger slide circle:KEY_F7 */
	TS_LETTER_c = KEY_F8,	/*6.Single finger write letter c*:KEY_F8 */
	TS_LETTER_e = KEY_F9,	/*7.Single finger write letter e:KEY_F9 */
	TS_LETTER_m = KEY_F10,	/*8.Single finger write letter m:KEY_F10 */
	TS_LETTER_w = KEY_F11,	/*9.Single finger write letter w:KEY_F11 */
	TS_PALM_COVERED = KEY_F12,	/*10.Palm off screen:KEY_F12 */
	TS_GESTURE_INVALID = 0xFF,	/*FF.No gesture */
};
enum ts_gesture_enable_bit {
	GESTURE_DOUBLE_CLICK = 0,
	GESTURE_SLIDE_L2R,
	GESTURE_SLIDE_R2L,
	GESTURE_SLIDE_T2B,
	GESTURE_SLIDE_B2T,
	GESTURE_CIRCLE_SLIDE = 6,
	GESTURE_LETTER_c,
	GESTURE_LETTER_e,
	GESTURE_LETTER_m,
	GESTURE_LETTER_w,
	GESTURE_PALM_COVERED,
	GESTURE_MAX,
	GESTURE_LETTER_ENABLE = 29,
	GESTURE_SLIDE_ENABLE = 30,
};
enum ts_touchplus_num {
	TS_TOUCHPLUS_KEY0 = KEY_F21,
	TS_TOUCHPLUS_KEY1 = KEY_F22,
	TS_TOUCHPLUS_KEY2 = KEY_F23,
	TS_TOUCHPLUS_KEY3 = KEY_F19,
	TS_TOUCHPLUS_KEY4 = KEY_F20,
	TS_TOUCHPLUS_INVALID = 0xFF,
};

enum ts_rawdata_debug_type {
	READ_DIFF_DATA = 0,
	READ_RAW_DATA,
};

struct ts_rawdata_limit_tab {
	int32_t* MutualRawMax;
	int32_t* MutualRawMin;
};

enum ts_panel_id {
	TS_PANEL_OFILIM 	 	= 0,
	TS_PANEL_EELY		 	= 1,
	TS_PANEL_TRULY	 		= 2,
	TS_PANEL_MUTTO	 		= 3,
	TS_PANEL_GIS		 	= 4,
	TS_PANEL_JUNDA	 		= 5,
	TS_PANEL_LENS 		 	= 6,
	TS_PANEL_YASSY	 		= 7,
	TS_PANEL_JDI 		 	= 6,
	TS_PANEL_SAMSUNG  		= 9,
	TS_PANEL_LG 		 	= 10,
	TS_PANEL_TIANMA 	 	= 11,
	TS_PANEL_CMI 		 	= 12,
	TS_PANEL_BOE  		 	= 13,
	TS_PANEL_CTC 		 	= 14,
	TS_PANEL_EDO 		 	= 15,
	TS_PANEL_SHARP 			= 16,
	TS_PANEL_AUO 			= 17,
	TS_PANEL_TOPTOUCH 		= 18,
	TS_PANEL_BOE_BAK		= 19,
	TS_PANEL_CTC_BAK 		= 20,
	TS_PANEL_UNKNOWN 		= 0xFF,
};

struct ts_finger {
	int status;
	int x;
	int y;
	int area;
	int pressure;
	int orientation;
	int major;
	int minor;
	int wx;
	int wy;
	int ewx;
	int ewy;
	int xer;
	int yer;
	int sg;
	int event;
};

struct ts_fingers {
	struct ts_finger fingers[TS_MAX_FINGER];
	int cur_finger_number;
	unsigned int gesture_wakeup_value;
	unsigned int special_button_key;
	unsigned int special_button_flag;
	unsigned int add_release_flag;
};

struct ts_algo_func {
	int algo_index;		/*from 0 to max*/
	char *algo_name;
	struct list_head node;
	int (*chip_algo_func) (struct ts_device_data *dev_data,
			       struct ts_fingers *in_info,
			       struct ts_fingers *out_info);
};

struct algo_param {
	u32 algo_order;
	struct ts_fingers info;
};

struct fw_param {
	char fw_name[MAX_STR_LEN * 4];	/*firmware name contain 4 parts*/
};

struct ts_rawdata_info {
	int status;
	int op_action;
	int used_size;		/* fill in rawdata size*/
	int used_synaptics_short_size;
	int used_synaptics_resistance_size;
	int used_synaptics_self_cap_size;
	int hybrid_buff[TS_HYBRID_BUFF_MAX];
	int used_size_3d;
	int used_sharp_selcap_single_ended_delta_size;
	int used_sharp_selcap_touch_delta_size;
	ktime_t time_stamp;
	int buff[TS_RAWDATA_BUFF_MAX];
	int buff_3d[TS_RAWDATA_BUFF_MAX];
	char result[TS_RAWDATA_RESULT_MAX];
};

#define CALIBRATION_DATA_SIZE 6144

struct ts_calibration_data_info {
	int status;
	ktime_t time_stamp;
	char data[CALIBRATION_DATA_SIZE];
	int used_size;
	int tx_num;
	int rx_num;
};

struct ts_calibration_info_param{
	int status;
	int calibration_crc;
};

struct ts_diff_data_info {
	int status;
	int op_action;
	int used_size;
	ktime_t time_stamp;
	int debug_type;
	int buff[TS_RAWDATA_BUFF_MAX];
	char result[TS_RAWDATA_RESULT_MAX];
};

struct ts_chip_info_param {
	int status;
	u8 chip_name[CHIP_INFO_LENGTH * 2];
	u8 ic_vendor[CHIP_INFO_LENGTH * 2];
	u8 fw_vendor[CHIP_INFO_LENGTH * 2];
	u8 mod_vendor[CHIP_INFO_LENGTH];
	u16 ttconfig_version;
	u8 fw_verctrl_num[CHIP_INFO_LENGTH];
};

enum ts_nv_structure_type {
        TS_NV_STRUCTURE_PROID        = 0,
        TS_NV_STRUCTURE_BAR_CODE     = 1,
        TS_NV_STRUCTURE_BRIGHTNESS   = 2,
        TS_NV_STRUCTURE_WHITE_POINT  = 3,
        TS_NV_STRUCTURE_BRI_WHITE    = 4,
        TS_NV_STRUCTURE_REPAIR       = 5,
        TS_NV_STRUCTURE_RESERVED     = 6,
};

struct ts_oem_info_param {
	int status;
	int op_action;
	u8 data_switch;
	u8 buff[TS_CHIP_BUFF_MAX_SIZE];
	u8 data[TS_CHIP_TYPE_MAX_SIZE];
	u8 length;
};

struct ts_calibrate_info {
	int status;
};

struct ts_dsm_debug_info {
	int status;
};

#if defined(HUAWEI_CHARGER_FB)
struct ts_charger_info {
	u8 charger_supported;
	u8 charger_switch;
	int op_action;
	int status;
	u16 charger_switch_addr;
	u16 charger_switch_bit;
};
#endif

struct ts_special_hardware_test_info {
	u8 switch_value;
	int op_action;
	int status;
	char *result;
};

struct ts_glove_info {
	u8 glove_switch;
	int op_action;
	int status;
	u16 glove_switch_addr;
	u16 glove_switch_bit;
};

struct ts_holster_info {
	u8 holster_switch;
	int op_action;
	int status;
	u16 holster_switch_addr;
	u16 holster_switch_bit;
};

struct ts_roi_info {
	u8 roi_supported;
	u8 roi_switch;
	int op_action;
	int status;
	u16 roi_control_addr;
	u8 roi_control_bit;
	u16 roi_data_addr;
};

enum ts_sleep_mode {
	TS_POWER_OFF_MODE = 0,
	TS_GESTURE_MODE,
};

struct ts_easy_wakeup_info {
	enum ts_sleep_mode sleep_mode;
	int off_motion_on;
	unsigned int easy_wakeup_gesture;
	int easy_wakeup_flag;
	int palm_cover_flag;
	int palm_cover_control;
	unsigned char easy_wakeup_fastrate;
	unsigned int easywake_position[MAX_POSITON_NUMS];
};

struct ts_wakeup_gesture_enable_info {
	u8 switch_value;
	int op_action;
	int status;
};

struct ts_regs_info {
	unsigned int addr;
	unsigned int bit;
	u8 values[TS_MAX_REG_VALUE_NUM];
	int num;
	u8 op_action;
	int status;
};

struct ts_window_info {
	int window_enable;
	int top_left_x0;
	int top_left_y0;
	int bottom_right_x1;
	int bottom_right_y1;
	int status;
};

struct ts_test_type_info {
	char tp_test_type[TS_CAP_TEST_TYPE_LEN];
	int op_action;
	int status;
};

#if defined (CONFIG_HUAWEI_DSM)
struct ts_dsm_info {
	char fw_update_result[8];
	unsigned char constraints_LDO17_status;
	unsigned char constraints_LSW50_status;
	int constraints_I2C_status;
	int constraints_UPDATE_status;
};
#endif

struct ts_hand_info {
	u8 hand_value;
	int op_action;
	int status;
};

struct ts_feature_info {
	struct ts_glove_info glove_info;
	struct ts_holster_info holster_info;
	struct ts_window_info window_info;
	struct ts_wakeup_gesture_enable_info wakeup_gesture_enable_info;
	struct ts_roi_info roi_info;
#if defined(HUAWEI_CHARGER_FB)
	struct ts_charger_info charger_info;
#endif
	struct ts_special_hardware_test_info hardware_test_info;
};

struct ts_cmd_param {
	union {
		struct algo_param algo_param;	/*algo cal*/
		struct ts_fingers report_info;	/*report input*/
		struct fw_param firmware_info;	/*firmware update*/
		enum ts_pm_type pm_type;
	} pub_params;
	void *prv_params;
};

enum ts_timeout_flag {
	TS_TIMEOUT = 0,
	TS_NOT_TIMEOUT,
	TS_UNDEF = 255,
};

enum ts_rawdata_work_flag {
	TS_RAWDATA_IDLE = 0,
	TS_RAWDATA_WORK,
};

struct ts_palm_info {
	u8 palm_switch;
	int op_action;
	int status;
};

struct ts_single_touch_info {
	u16 single_touch_switch;
	int op_action;
	int status;
};
struct ts_cmd_sync {
	atomic_t timeout_flag;
	struct completion done;
};

struct ts_cmd_node {
	enum ts_cmd command;
	struct ts_cmd_sync *sync;
	struct ts_cmd_param cmd_param;
};

struct ts_regulator_contrl {
	int vci_value;
	int vddio_value;
	int need_set_vddio_value;
};

struct ts_cmd_queue {
	int wr_index;
	int rd_index;
	int cmd_count;
	int queue_size;
	spinlock_t spin_lock;
	struct ts_cmd_node ring_buff[TS_CMD_QUEUE_SIZE];
};

struct ts_device_ops {
	int (*chip_detect) (struct device_node *device,
			    struct ts_device_data *data,
			    struct platform_device *ts_dev);
	int (*chip_wrong_touch) (void);
	int (*chip_init) (void);
	int (*chip_get_brightness_info) (void);
	int (*chip_parse_config) (struct device_node *device,
				  struct ts_device_data *data);
	int (*chip_input_config) (struct input_dev *input_dev);
	int (*chip_register_algo) (struct ts_device_data *data);
	int (*chip_irq_top_half) (struct ts_cmd_node *cmd);
	int (*chip_irq_bottom_half) (struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd);
	int (*chip_reset) (void);
	int (*chip_debug_switch) (u8 loglevel);
	void (*chip_shutdown) (void);
	int (*oem_info_switch) (struct ts_oem_info_param *info);
	int (*chip_get_info) (struct ts_chip_info_param *info);
	int (*chip_set_info_flag) (struct ts_data *info);
	int (*chip_fw_update_boot) (char *file_name);
	int (*chip_fw_update_sd) (void);
	int (*chip_calibrate) (void);
	int (*chip_calibrate_wakeup_gesture) (void);
	int (*chip_dsm_debug) (void);
	int (*chip_get_rawdata) (struct ts_rawdata_info *info,
				 struct ts_cmd_node *out_cmd);
	int (*chip_get_calibration_data)(struct ts_calibration_data_info *info,
			struct ts_cmd_node *out_cmd);
	int (*chip_get_calibration_info)(struct ts_calibration_info_param *info,
			struct ts_cmd_node *out_cmd);
	int (*chip_glove_switch) (struct ts_glove_info *info);
	int (*chip_palm_switch) (struct ts_palm_info *info);
	int (*chip_single_touch_switch) (struct ts_single_touch_info *info);
	int (*chip_wakeup_gesture_enable_switch) (struct
						  ts_wakeup_gesture_enable_info
						  *info);
	int (*chip_charger_switch) (struct ts_charger_info *info);
	int (*chip_holster_switch) (struct ts_holster_info *info);
	int (*chip_roi_switch) (struct ts_roi_info *info);
	unsigned char *(*chip_roi_rawdata) (void);
	int (*chip_hand_detect) (struct ts_hand_info *info);
	int (*chip_before_suspend) (void);
	int (*chip_suspend) (void);
	int (*chip_resume) (void);
	int (*chip_after_resume) (void *feature_info);
	int (*chip_test) (struct ts_cmd_node *in_cmd,
			  struct ts_cmd_node *out_cmd);
	int (*chip_check_status) (void);
	int (*chip_hw_reset) (void);
	int (*chip_regs_operate) (struct ts_regs_info *info);
	int (*chip_get_capacitance_test_type) (struct ts_test_type_info *info);
	int (*chip_get_debug_data) (struct ts_diff_data_info *info,
				    struct ts_cmd_node *out_cmd);
	void (*chip_special_hardware_test_swtich) (unsigned int value);
	int (*chip_special_hardware_test_result) (char *buf);
	void (*chip_ghost_detect) (int value);
	void (*chip_work_after_input) (void);
};

struct anti_false_touch_param{
	int feature_all;
	int feature_resend_point;
	int feature_orit_support;
	int feature_reback_bt;
	int lcd_width;
	int lcd_height;
	int click_time_limit;
	int click_time_bt;
	int edge_position;
	int edge_postion_secondline;
	int bt_edge_x;
	int bt_edge_y;
	int move_limit_x;
	int move_limit_y;
	int move_limit_x_t;
	int move_limit_y_t;
	int move_limit_x_bt;
	int move_limit_y_bt;
	int edge_y_confirm_t;
	int edge_y_dubious_t;
	int edge_y_avg_bt;
	int edge_xy_down_bt;
	int edge_xy_confirm_t;
	int max_points_bak_num;

	/* emui5.1 new support */
	int feature_sg;
	int sg_min_value;
	int feature_support_list;
	int max_distance_dt;
	int feature_big_data;
	int feature_click_inhibition;
	int min_click_time;

	//for driver
	int drv_stop_width;
	int sensor_x_width;
	int sensor_y_width;

	/* if x > drv_stop_width, and then the same finger x < drv_stop_width, report it */
	unsigned int edge_status;
};

struct aft_abs_param_major{
	u8 edgex;
	u8 edgey;
	u8 orientation;
	u8 version;
};

struct aft_abs_param_minor{
	u8 float_reserved1;
	u8 float_reserved2;
	u8 float_reserved3;
	u8 float_reserved4;
};

struct ts_device_data {
	bool is_parade_solution;
	bool is_direct_proc_cmd;
	bool is_i2c_one_byte;
	bool is_new_oem_structure;
	bool disable_reset;
	bool is_in_cell;
	bool report_tui_enable;
	u8 tui_set_flag;
	bool need_wd_check_status;
	int check_status_watchdog_timeout;
	int rawdata_arrange_swap;
	int rawdata_get_timeout;
	int has_virtualkey;
	int bootloader_update_enable;
	// syna_wx_wy
	int is_multi_protocal;
	unsigned char adv_width[4];
	char chip_name[MAX_STR_LEN];
	char module_name[MAX_STR_LEN];
	char version_name[MAX_STR_LEN];
	char tp_test_type[TS_CAP_TEST_TYPE_LEN];
	int raw_limit_buf[RAWDATA_NUM];
	u8 reg_values[TS_MAX_REG_VALUE_NUM];
	int rawdata_debug_type;
	struct device_node *cnode;
	int slave_addr;
	int irq_gpio;
	int irq_config;		/* 0 - LOW LEVEL  1 - HIGH LEVEL  2 - RAISE EDGE  3 - FALL EDGE*/
	struct ts_regulator_contrl regulator_ctr;
	int vci_gpio_type;
	int vci_regulator_type;
	int vci_gpio_ctrl;
	int vci_pmic_type;
	int vddio_gpio_type;
	int vddio_regulator_type;
	int vddio_gpio_ctrl;
	int vddio_pmic_type;
	unsigned short tp_func_flag;
	int reset_gpio;
	int algo_size;
	int algo_id;
	int reg_num;
	int ic_type;
	int projectid_len;
	int cover_force_glove;
	int unite_cap_test_interface;
	int report_rate_test;
	u8 rawdata_report_type;
	int raw_test_type;
	int capacitance_test_config;
	int rawdata_arrange_type;
	int supported_func_indicater;
	int support_3d_func;
	int test_rawdata_normalizing;
	int tddi_ee_short_test_support;
	int tddi_ee_short_test_partone_limit;
	int tddi_ee_short_test_parttwo_limit;
	int self_cap_max;
	int x_max;
	int y_max;
	int x_max_mt;
	int y_max_mt;
	bool flip_x;
	bool flip_y;
	int lcd_full;
	int test_enhance_raw_data_capacitance;
	int test_enhance_delta_trx;
	int self_cap_noise_max;
	int should_check_tp_calibration_info;
	int *upper;
	int *lower;
	int *tx_delta;
	int *rx_delta;
	int *high_resistance;
	u32 *self_cap_hybrid_lower;
	u32 *self_cap_hybrid_upper;
	int ghost_detect_support;
	int noise_state_reg;
	int noise_record_num;
	struct ts_device_ops *ops;
	void *prv_data;
	struct ts_bus_info *bops;
	struct i2c_client *client;	/*client need to be used  modified by mengkun*/
	struct list_head algo_head;	/*algo function list*/
	struct ts_data *ts_data;
	struct ts_easy_wakeup_info easy_wakeup_info;
	int fw_update_logic;
#if defined (CONFIG_TEE_TUI)
	void *tui_data;
#endif
	int use_ub_supported;
	int delay_for_fw_update;
	struct anti_false_touch_param anti_false_touch_param_data;
	bool isbootupdate_finish;
	bool is_can_device_use_int;
	struct mutex device_call_lock;
};

struct ts_bus_info {
	enum ts_bus_type btype;
	int bus_id;
	int (*bus_write) (u8 *buf, u16 length);
	int (*bus_read) (u8 *reg_addr, u16 reg_len, u8 *buf, u16 len);
};
struct ts_aft_algo_param
{
	char aft_enable_flag;
	int drv_stop_width;
	int lcd_width;
	int lcd_height;
};
struct ts_data {
	char product_name[MAX_STR_LEN];
	int dev_id;		/*reserved for more ts devices in handset*/
	int irq_id;
	int get_info_flag;
	int fpga_flag;
	atomic_t state;
	atomic_t ts_esd_state;
	int edge_wideth;
	struct device_node *node;
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct task_struct *ts_task;
	struct ts_device_data *chip_data;
	struct ts_bus_info *bops;
	struct platform_device *ts_dev;
	struct ts_chip_info_param chip_info;
	struct ts_aft_algo_param  aft_param;
	struct ts_fingers fingers_send_aft_info;
	struct semaphore fingers_aft_send;
	atomic_t fingers_waitq_flag;
#if defined(HUAWEI_CHARGER_FB)
	struct notifier_block charger_detect_notify;
#endif
#if defined(CONFIG_FB)
	struct notifier_block fb_notify;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	struct ts_cmd_queue queue;
	struct ts_cmd_queue no_int_queue;
	struct timer_list watchdog_timer;
	struct work_struct watchdog_work;
	struct ts_feature_info feature_info;
	struct ts_window_info window_info;
	struct device_node *dev_node;
	int *virtual_keys_values;
	int virtual_keys_size;
#if defined (CONFIG_HUAWEI_DSM)
	struct ts_dsm_info dsm_info;
#endif
	struct wake_lock ts_wake_lock;
};

int ts_power_control_notify(enum ts_pm_type pm_type, int timeout);
void ts_thread_stop_notify(void);
int put_one_cmd(struct ts_cmd_node *cmd, int timeout);
int register_algo_func(struct ts_device_data *chip_data,
		       struct ts_algo_func *fn);
void ts_anti_false_touch_param_achieve(struct ts_device_data *chip_data);
int ts_parse_csvfile(int columns, int rows, struct ts_rawdata_limit_tab* limit_tab, char *file_path);
extern volatile int g_tp_power_ctrl;

#ifdef HUAWEI_TOUCHSCREEN_TEST
int test_dbg_cmd_test(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
#endif

void rotate_rawdata(int row, int column, int *data_start, int rotate_type);
#endif
