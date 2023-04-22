/*
 * Huawei Touchpanel driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#ifndef __HUAWEI_TS_KIT_H_
#define __HUAWEI_TS_KIT_H_

#include <linux/version.h>
#include <linux/i2c.h>
#include <linux/spi/spi.h>
#include <linux/amba/pl022.h>
#include <linux/input.h>
#include <linux/completion.h>
#include <linux/kernel.h>
#include <linux/of.h>
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>
#include <lcd_kit_core.h>

#define NO_ERR 0
#define RESULT_ERR (-1)
#define NO_SYNC_TIMEOUT		0
#define SHORT_SYNC_TIMEOUT		5
#define LONG_SYNC_TIMEOUT		10
#define LONG_LONG_SYNC_TIMEOUT	50

#define TS_RAWDATA_BUFF_MAX 11000

#define TS_RAWDATA_DEVINFO_MAX	80
#define TS_RAWDATA_RESULT_MAX	200
#define MAX_CAP_DATA_SIZE 8
#define DELTA_TEST_TITLE_LEN 6
#define TX_RX_BUF_MAX 3000
#define PART_ONE_LIMIT_LENGTH	11
#define EESHORT_PRINT_SHIFT_LENGTH	1
#define MAX_CAP_DATA_SIZE_FOR_EESHORT	8

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

#define I2C_RW_TRIES 3		//retry 3 times

#define TS_MAX_REG_VALUE_NUM 80


/* ts switch func begin */
#define TS_SWITCH_TYPE_DOZE	(1 << 0)
#define TS_SWITCH_TYPE_GAME	(1 << 1)
#define TS_SWITCH_TYPE_SCENE	(1 << 2)

#define TS_SWITCH_DOZE_ENABLE	1
#define TS_SWITCH_DOZE_DISABLE	2
#define TS_SWITCH_GAME_ENABLE	3
#define TS_SWITCH_GAME_DISABLE	4
#define TS_SWITCH_SCENE_ENTER	3
#define TS_SWITCH_SCENE_EXIT	4

enum ts_scene_code {
	TS_SWITCH_SCENE_3 = 3,
	TS_SWITCH_SCENE_4,
	TS_SWITCH_SCENE_5,
	TS_SWITCH_SCENE_6,
	TS_SWITCH_SCENE_7,
	TS_SWITCH_SCENE_8,
	TS_SWITCH_SCENE_9,
	TS_SWITCH_SCENE_10,
	TS_SWITCH_SCENE_11,
	TS_SWITCH_SCENE_12,
	TS_SWITCH_SCENE_13,
	TS_SWITCH_SCENE_14,
	TS_SWITCH_SCENE_15,
	TS_SWITCH_SCENE_16,
	TS_SWITCH_SCENE_17,
	TS_SWITCH_SCENE_18,
	TS_SWITCH_SCENE_19,
	TS_SWITCH_SCENE_20
};
/* ts switch func end */

#define MAX_STR_LEN 32
#define TS_CAP_TEST_TYPE_LEN 100

#define RAWDATA_CAP_TEST_FAIL "1F"
#define RAWDATA_CAP_TEST_PASS "1P"
#define TRX_DELTA_CAP_TEST_FAIL "-2F"
#define TRX_DELTA_CAP_TEST_PASS "-2P"
#define TD43XX_EE_SHORT_TEST_FAIL "-5F"
#define TD43XX_EE_SHORT_TEST_PASS "-5P"
#define CAP_TEST_FORCEKEY_VALUE_NUM 2

#define CHIP_INFO_LENGTH	16
#define RAWDATA_NUM 16
#define MAX_POSITON_NUMS 6

#define TS_BOOT_DETECTION_SUPPORT  1

#define ROI_HEAD_DATA_LENGTH		4
#define ROI_DATA_READ_LENGTH		102
#define ROI_DATA_SEND_LENGTH		(ROI_DATA_READ_LENGTH-ROI_HEAD_DATA_LENGTH)
#define ROI_CTRL_DEFAULT_ADDR		0x0446
#define ROI_DATA_DEFAULT_ADDR		0x0418
#define CALIBRATION_DATA_SIZE 6144

#define DIFF_DATA_MAX_LEN		100

#define TP_I2C_HWSPIN_LOCK_CODE   28
#define GET_HARDWARE_TIMEOUT 100000

#define TS_MAX_FINGER 10
#define TS_MAX_PEN_BUTTON 2

extern u8 g_ts_kit_log_cfg;
#define HWLOG_TAG	TS_KIT
HWLOG_REGIST();
#define TS_LOG_INFO(x...)		_hwlog_info(HWLOG_TAG, ##x)
#define TS_LOG_ERR(x...)		_hwlog_err(HWLOG_TAG, ##x)
#define TS_LOG_DEBUG(x...)	\
	do { \
		if (g_ts_kit_log_cfg)	\
			_hwlog_info(HWLOG_TAG, ##x);	\
	}while(0)
/*
 * +-------+--------+---------+----------------------------------------------+
 * | BIT7  |  BIT6  |  BIT5   |                 BIT4~BIT0                    |
 * +-------------------------------------------------------------------------+
 * |       |release |  press  |                event type                    |
 * +----------------+---------+----------------------------------------------+
 */

/* event status begin*/
#define TS_FINGER_RELEASE	(1 << 5)
#define TS_FINGER_PRESS		(1 << 6)
/* event status end */

/* event type begin */
#define TP_NONE_FINGER	0
#define TP_FINGER 	1
#define TP_STYLUS	2
#define TP_GLOVE 	6
/* event type end */



#define FILTER_GLOVE_NUMBER	4

#define GPIO_HIGH        1

#define TS_CMD_CHECK_KEY 0X8080

#define TS_RAWDATA_FAILED_REASON_LEN	20
#define TS_RAWDATA_STATISTICS_DATA_LEN  32
#define TS_RAWDATA_TEST_NAME_LEN	50
#define TS_RAWDATA_RESULT_CODE_LEN	5
#define TS_CMD_QUEUE_SIZE  20

#define FW_UPDATE_TO_HIGH 1
#define FW_UPDATE_DIFF 0

enum TP_ic_type {
	ONCELL = 0,		//lcd & tp have separate regulator
	HYBRID,			//lcd &tp share 1.8V
	TDDI,			//lcd ctrl all the regulator
	AMOLED,			//Amoled need to set 1.8 and 3.3 for itself.
	TP_IC_TYPE_MAX = 255,
};

enum TP_state_machine {
	INIT_STATE = 0,
	ZERO_STATE = 1,		//switch finger to glove
	FINGER_STATE = 2,	//finger state
	GLOVE_STATE = 3		//glove state
};
/* external varible*/

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
struct ts_kit_device_data;
struct ts_bus_info;
struct ts_kit_platform_data;
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
	TS_TOUCH_SWITCH,
	TS_CHIP_DETECT,
	TS_REPORT_PEN,
	TS_FREEBUFF,
	TS_SET_SENSIBILITY,
	TS_BOOT_DETECTION,
	TS_PALM_KEY,
	TS_INVAILD_CMD = 255,
};

enum ts_bus_type {
	TS_BUS_I2C = 0,
	TS_BUS_SPI,
	TS_BUS_UNDEF = 255,
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
	TS_GOTO_SLEEP,
	TS_GOTO_WORK,
	TS_STATE_UNDEFINE = 255,
};
enum ts_esd_state {
	TS_NO_ESD = 0,
	TS_ESD_HAPPENDED,
};
enum ts_register_state {
	TS_UNREGISTER = 0,
	TS_REGISTER_DONE,
	TS_REGISTER_UNDEFINE = 255,
};
enum ts_rawdata_arange_type {
	TS_RAWDATA_TRANS_NONE = 0,
	TS_RAWDATA_TRANS_ABCD2CBAD,
	TS_RAWDATA_TRANS_ABCD2ADCB,
};

enum STYLUS_WAKEUP_CTRL {
	STYLUS_WAKEUP_DISABLE = 0,
	STYLUS_WAKEUP_NORMAL_STATUS = 1,
	STYLUS_WAKEUP_LOW_FREQENCY = 2,
	STYLUS_WAKEUP_TESTMODE = 3,
	MAX_STATUS = 255,
};
#define KEY_F26 196 /* ESD Key Event */
#define KEY_F27  197
#define KEY_F28  198
enum ts_gesture_num {
	//      TS_NUM_TOTAL = 12, /* total gesture numbers  */
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
	TS_KEY_IRON = KEY_F26,	/* ESD_avoiding to report KEY_F26(196) */
	TS_STYLUS_WAKEUP_TO_MEMO = KEY_F27,
	TS_STYLUS_WAKEUP_SCREEN_ON = KEY_F28,
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
	GESTURE_STYLUS_WAKE_UP,
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
	READ_CB_DATA,
	READ_FORCE_DATA,
};
#if defined (CONFIG_TEE_TUI)
struct ts_tui_data {
	char device_name[11];
};
#endif
enum ts_charger_state {
	USB_PIUG_OUT = 0,
	USB_PIUG_IN,
};
typedef enum {
	TS_PEN_OUT_RANGE = 0,	/* pen out of range */
	TS_PEN_IN_RANGE,	/* pen in range */

	/* add event before here */
	TS_EVENT_MAX,		/* max event type */
} ts_notify_event_type;

struct ts_unique_capacitance_test {
	int Read_only_support_unique;
	int32_t *Rx_delta_abslimit;
	int32_t *Tx_delta_abslimit;
	int32_t *ee_short_data_limit;
	int32_t *noise_data_limit;
};
struct ts_rawdata_limit_tab {
	int32_t *MutualRawMax;
	int32_t *MutualRawMin;
	struct ts_unique_capacitance_test *unique_test;
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

struct ts_tool {
	int pen_inrange_status;
	int tip_status;
	int x;
	int y;
	int pressure;
	signed char tilt_x;
	signed char tilt_y;
	int tool_type;		// BTN_TOOL_RUBBER  BTN_TOOL_PEN
};

struct ts_button {
	int status;
	int key;		//BTN_STYLUS
};

struct ts_pens {
	struct ts_tool tool;
	struct ts_button buttons[TS_MAX_PEN_BUTTON];
};

struct ts_algo_func {
	int algo_index;		//from 0 to max
	char *algo_name;
	struct list_head node;
	int (*chip_algo_func) (struct ts_kit_device_data * dev_data,
			       struct ts_fingers * in_info,
			       struct ts_fingers * out_info);
};

struct algo_param {
	u32 algo_order;
	struct ts_fingers info;
};

struct fw_param {
	char fw_name[MAX_STR_LEN * 4];	//firmware name contain 4 parts
};

/*
 New data format for rawdata
*/
#define RAWDATA_TEST_FAIL_CHAR 'F'
#define RAWDATA_TEST_PASS_CHAR 'P'

enum ts_rawdata_formattype {
	TS_RAWDATA_OLDFORMAT = 0,
	TS_RAWDATA_NEWFORMAT,
};
enum ts_raw_data_type {
	RAW_DATA_TYPE_IC = 0,
	RAW_DATA_TYPE_CAPRAWDATA,
	RAW_DATA_TYPE_TrxDelta,
	RAW_DATA_TYPE_Noise,
	RAW_DATA_TYPE_FreShift,
	RAW_DATA_TYPE_OpenShort,
	RAW_DATA_TYPE_SelfCap,
	RAW_DATA_TYPE_CbCctest,
	RAW_DATA_TYPE_highResistance,
	RAW_DATA_TYPE_SelfNoisetest,
	RAW_DATA_TYPE_forcekey,
	RAW_DATA_END,
};
struct ts_rawdata_newnodeinfo {
	struct list_head node;
	u8 typeindex;		/* enum ts_raw_data_type */
	char testresult;	/* 'P' o 'F' */
	int *values;
	size_t size;
	char test_name[TS_RAWDATA_TEST_NAME_LEN];
	char statistics_data[TS_RAWDATA_STATISTICS_DATA_LEN];	/* [%d,%d,%d] */
	char tptestfailedreason[TS_RAWDATA_FAILED_REASON_LEN];	/* "-panel_reason" o "-software reason" */
};

struct ts_rawdata_info_new {
	int status;
	int rx;
	int tx;
	ktime_t time_stamp;
	char deviceinfo[TS_RAWDATA_DEVINFO_MAX];	/* ic info */
	char i2cinfo[TS_RAWDATA_RESULT_CODE_LEN];	/* i2c test result:0P or 0F */
	char i2cerrinfo[TS_RAWDATA_FAILED_REASON_LEN];	/* i2c test fail reason */
	int listnodenum;
	struct list_head rawdata_head;	//ts_rawdata_newnodeinfo  list
};

struct ts_rawdata_info {
	int status;
	int op_action;
	int used_size;		// fill in rawdata size
	int used_synaptics_self_cap_size;
	int used_size_3d;
	int used_sharp_selcap_single_ended_delta_size;
	int used_sharp_selcap_touch_delta_size;
	ktime_t time_stamp;
	int buff[TS_RAWDATA_BUFF_MAX];
	int hybrid_buff[TS_RAWDATA_BUFF_MAX];
	int hybrid_buff_used_size;
	int buff_3d[TS_RAWDATA_BUFF_MAX];
	char result[TS_RAWDATA_RESULT_MAX];
	int *tx_delta_buf;
	int *rx_delta_buf;
	signed int *td43xx_rt95_part_one;
	signed int *td43xx_rt95_part_two;
};
struct ts_calibration_data_info {
	int status;
	ktime_t time_stamp;
	char data[CALIBRATION_DATA_SIZE];
	int used_size;
	int tx_num;
	int rx_num;
};

struct ts_calibration_info_param {
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
	TS_NV_STRUCTURE_PROID = 0,
	TS_NV_STRUCTURE_BAR_CODE = 1,
	TS_NV_STRUCTURE_BRIGHTNESS = 2,
	TS_NV_STRUCTURE_WHITE_POINT = 3,
	TS_NV_STRUCTURE_BRI_WHITE = 4,
	TS_NV_STRUCTURE_REPAIR = 5,
	TS_NV_STRUCTURE_RESERVED = 6,
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

struct ts_charger_info {
	int op_action;
	int status;
	u16 charger_switch_addr;
	u16 charger_switch_bit;
	u8 charger_supported;
	u8 charger_switch;
};

struct ts_glove_info {
	int op_action;
	int status;
	u16 glove_switch_addr;
	u16 glove_switch_bit;
	u8 glove_supported;
	u8 glove_switch;
};

struct ts_holster_info {
	int op_action;
	int status;
	u16 holster_switch_addr;
	u16 holster_switch_bit;
	u8 holster_supported;
	u8 holster_switch;

};

struct ts_roi_info {
	int op_action;
	int status;
	u16 roi_control_addr;
	u16 roi_data_addr;
	u8 roi_supported;
	u8 roi_switch;
	u8 roi_control_bit;
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
	int bit;
	int num;
	int status;
	u8 values[TS_MAX_REG_VALUE_NUM];
	u8 op_action;
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

struct ts_pen_info {
	u8 pen_supported;
	int supported_pen_alg;
	int supported_pen_mmitest;
};
struct ts_feature_info {
	struct ts_glove_info glove_info;
	struct ts_holster_info holster_info;
	struct ts_window_info window_info;
	struct ts_wakeup_gesture_enable_info wakeup_gesture_enable_info;
	struct ts_roi_info roi_info;
	struct ts_charger_info charger_info;
	struct ts_pen_info pen_info;
};

struct ts_cmd_param {
	union {
		struct algo_param algo_param;	//algo cal
		struct ts_fingers report_info;	//report input
		struct ts_pens report_pen_info;	//report pen
		struct fw_param firmware_info;	//firmware update
		enum lcd_kit_ts_pm_type pm_type;
		struct ts_kit_device_data *chip_data;
		int sensibility_cfg;
		unsigned int ts_key;
	} pub_params;
	void *prv_params;
	void (*ts_cmd_freehook) (void *);
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
	int ts_cmd_check_key;
	enum ts_cmd command;
	struct ts_cmd_sync *sync;
	struct ts_cmd_param cmd_param;
};

struct ts_cmd_queue {
	int wr_index;
	int rd_index;
	int cmd_count;
	int queue_size;
	spinlock_t spin_lock;
	struct ts_cmd_node ring_buff[TS_CMD_QUEUE_SIZE];
};

struct ts_regulator_contrl {
	int vci_value;
	int vddio_value;
	int need_set_vddio_value;
};


struct aft_abs_param_major {
	u8 edgex;
	u8 edgey;
	u8 orientation;
	u8 version;
};

struct aft_abs_param_minor {
	u8 float_reserved1;
	u8 float_reserved2;
	u8 float_reserved3;
	u8 float_reserved4;
};

struct ts_device_ops {
	int (*chip_detect) (struct ts_kit_platform_data * data);
	int (*chip_wrong_touch) (void);
	int (*chip_init) (void);
	int (*chip_get_brightness_info) (void);
	int (*chip_parse_config) (struct device_node * device,
				  struct ts_kit_device_data * data);
	int (*chip_input_config) (struct input_dev * input_dev);
	int (*chip_input_pen_config) (struct input_dev * input_dev);
	int (*chip_register_algo) (struct ts_kit_device_data * data);
	int (*chip_irq_top_half) (struct ts_cmd_node * cmd);
	int (*chip_irq_bottom_half) (struct ts_cmd_node * in_cmd,
				     struct ts_cmd_node * out_cmd);
	int (*chip_reset) (void);
	int (*chip_debug_switch) (u8 loglevel);
	void (*chip_shutdown) (void);
	int (*oem_info_switch) (struct ts_oem_info_param * info);
	int (*chip_get_info) (struct ts_chip_info_param * info);
	int (*chip_set_info_flag) (struct ts_kit_platform_data * info);
	int (*chip_fw_update_boot) (char *file_name);
	int (*chip_fw_update_sd) (void);
	int (*chip_calibrate) (void);
	int (*chip_calibrate_wakeup_gesture) (void);
	int (*chip_dsm_debug) (void);
	int (*chip_get_rawdata) (struct ts_rawdata_info * info,
				 struct ts_cmd_node * out_cmd);
	int (*chip_get_calibration_data) (struct ts_calibration_data_info *
					  info, struct ts_cmd_node * out_cmd);
	int (*chip_get_calibration_info) (struct ts_calibration_info_param *
					  info, struct ts_cmd_node * out_cmd);
	int (*chip_glove_switch) (struct ts_glove_info * info);
	int (*chip_palm_switch) (struct ts_palm_info * info);
	int (*chip_single_touch_switch) (struct ts_single_touch_info * info);
	int (*chip_wakeup_gesture_enable_switch) (struct ts_wakeup_gesture_enable_info * info);
	int (*chip_charger_switch) (struct ts_charger_info * info);
	int (*chip_holster_switch) (struct ts_holster_info * info);
	int (*chip_roi_switch) (struct ts_roi_info * info);
	unsigned char *(*chip_roi_rawdata) (void);
	int (*chip_hand_detect) (struct ts_hand_info * info);
	int (*chip_before_suspend) (void);
	int (*chip_suspend) (void);
	int (*chip_resume) (void);
	int (*chip_after_resume) (void *feature_info);
	int (*chip_test) (struct ts_cmd_node * in_cmd, struct ts_cmd_node * out_cmd);
	int (*chip_check_status) (void);
	int (*chip_hw_reset) (void);
	int (*chip_regs_operate) (struct ts_regs_info * info);
	int (*chip_get_capacitance_test_type) (struct ts_test_type_info * info);
	int (*chip_get_debug_data) (struct ts_diff_data_info * info, struct ts_cmd_node * out_cmd);
	void (*chip_special_hardware_test_swtich) (unsigned int value);
	int (*chip_special_hardware_test_result) (char *buf);
	void (*chip_ghost_detect) (int value);
	void (*chip_touch_switch) (void);
	void (*chip_set_sensibility_cfg) (int value);
	void (*chip_work_after_input) (void);
	int (*chip_special_rawdata_proc_printf) (struct seq_file * m,
						 struct ts_rawdata_info * info,
						 int range_size, int row_size);
	int (*chip_boot_detection) (void);
};
struct anti_false_touch_param {
	unsigned int feature_all;
	unsigned int feature_resend_point;
	unsigned int feature_orit_support;
	unsigned int feature_reback_bt;
	unsigned int lcd_width;
	unsigned int lcd_height;
	unsigned int click_time_limit;
	unsigned int click_time_bt;
	unsigned int edge_position;
	unsigned int edge_postion_secondline;
	unsigned int bt_edge_x;
	unsigned int bt_edge_y;
	unsigned int move_limit_x;
	unsigned int move_limit_y;
	unsigned int move_limit_x_t;
	unsigned int move_limit_y_t;
	unsigned int move_limit_x_bt;
	unsigned int move_limit_y_bt;
	unsigned int edge_y_confirm_t;
	unsigned int edge_y_dubious_t;
	unsigned int edge_y_avg_bt;
	unsigned int edge_xy_down_bt;
	unsigned int edge_xy_confirm_t;
	unsigned int max_points_bak_num;

	/* emui5.1 new support */
	unsigned int feature_sg;
	unsigned int sg_min_value;
	unsigned int feature_support_list;
	unsigned int max_distance_dt;
	unsigned int feature_big_data;
	unsigned int feature_click_inhibition;
	unsigned int min_click_time;

	//for driver
	unsigned int drv_stop_width;
	unsigned int sensor_x_width;
	unsigned int sensor_y_width;

	/* if x > drv_stop_width, and then the same finger x < drv_stop_width, report it */
	unsigned int edge_status;
};

#define TS_KIT_POWER_ON 1
#define TS_KIT_POWER_OFF 0

enum ts_kit_power_type {
	TS_KIT_POWER_UNUSED = 0,
	TS_KIT_POWER_LDO = 1,
	TS_KIT_POWER_GPIO = 2,
	TS_KIT_POWER_INVALID_TYPE,
};

enum ts_kit_power_id {
	TS_KIT_IOVDD = 0,
	TS_KIT_VCC = 1,
	TS_KIT_POWER_ID_MAX,
};

struct ts_kit_power_supply {
	int use_count;
	int type;
	int gpio;
	int ldo_value;
	struct regulator *regulator;
};

struct ts_kit_device_data {
	struct device_node *cnode;
	struct ts_device_ops *ops;
	struct ts_easy_wakeup_info easy_wakeup_info;
	struct list_head algo_head;	//algo function list
	struct ts_kit_platform_data *ts_platform_data;
	struct anti_false_touch_param anti_false_touch_param_data;
	struct ts_regulator_contrl regulator_ctr;
	struct mutex device_call_lock;
	int raw_limit_buf[RAWDATA_NUM];
	char chip_name[MAX_STR_LEN];
	char module_name[MAX_STR_LEN];
	char version_name[MAX_STR_LEN];
	char project_id[MAX_STR_LEN];
	const char *tp_test_type;
	const char *vendor_name;
	char touch_switch_info[MAX_STR_LEN];
	u8 reg_values[TS_MAX_REG_VALUE_NUM];
	unsigned int is_parade_solution;
	unsigned int is_ic_rawdata_proc_printf;
	unsigned int is_direct_proc_cmd;
	unsigned int support_forcekey_cap_value_test;
	unsigned int is_i2c_one_byte;
	unsigned int is_new_oem_structure;
	unsigned int is_in_cell;
	unsigned int report_tui_enable;
	unsigned int need_wd_check_status;
	unsigned int vci_gpio_type;
	unsigned int vci_regulator_type;
	unsigned int vci_gpio_ctrl;
	unsigned int vddio_gpio_type;
	unsigned int vddio_regulator_type;
	unsigned int vddio_gpio_ctrl;
	unsigned int vddio_default_on;
	unsigned int algo_size;
	unsigned int algo_id;
	unsigned int slave_addr;
	unsigned int irq_config;		// 0 - LOW LEVEL  1 - HIGH LEVEL  2 - RAISE EDGE  3 - FALL EDGE
	unsigned int ic_type;
	unsigned int projectid_len;
	unsigned int rawdata_arrange_type;
	unsigned int x_max;
	unsigned int y_max;
	unsigned int x_max_mt;
	unsigned int y_max_mt;
	unsigned int flip_x;
	unsigned int flip_y;
	unsigned int reg_num;
	unsigned int touch_switch_flag;
	unsigned short touch_switch_reg;
	unsigned short touch_switch_hold_off_reg;
	unsigned short touch_game_reg;//game mode config reg
	unsigned short touch_scene_reg;
	unsigned short aft_data_addr;
	unsigned int support_aft;
	unsigned int self_cap_test;
	unsigned int ic_status_reg;
	unsigned int enable_ghost_dmd_report;
	unsigned int check_bulcked;
	unsigned int get_brightness_info_flag;
	unsigned int isbootupdate_finish;
	unsigned int is_can_device_use_int;
	unsigned int send_stylus_gesture_switch;//for stylus wakup,TS_WAKEUP_GESTURE_ENABLE will just be used in working mode,instead of be used in sleep mode.
	unsigned int diff_data_report_supported;
	unsigned int diff_data_len;
	unsigned int diff_data_control_addr;
	unsigned char * diff_data;
	int check_status_watchdog_timeout;
	int rawdata_get_timeout;
	int has_virtualkey;
	int supported_func_indicater;
	int rawdata_debug_type;
	int capacitance_test_config;
	int support_3d_func;
	int unite_cap_test_interface;
	int lcd_full;
	int should_check_tp_calibration_info;
	int fw_update_logic;
	int test_capacitance_via_csvfile;
	int csvfile_use_product_system;
	int trx_delta_test_support;
	int forcekey_test_support;
	int td43xx_ee_short_test_support;
	int tddi_ee_short_test_partone_limit;
	int tddi_ee_short_test_parttwo_limit;
	int *tx_delta;
	int *rx_delta;
	int tx_num;
	int rx_num;
	int raw_test_type;
	int report_rate_test;
	int noise_state_reg;
	int noise_record_num;
	int sleep_in_mode;
	int frequency_selection_reg;
	int check_fw_right_flag;
	int fp_tp_report_touch_minor_event;
	int read_2dbarcode_oem_type;
	int support_2dbarcode_info;
	int rawdata_report_type;
	int suspend_no_config;
	u8 game_control_bit;//the mask bit of touch_game_reg.
	u8 rawdata_newformatflag;	// 0 - old format   1 - new format
	u8 tui_set_flag;
	u8 provide_panel_id_support;
	u8 print_all_trx_diffdata_for_newformat_flag; /*print all tr&rx difference cap data (TX*RX) */
	unsigned int boot_detection_addr;
	unsigned int boot_detection_threshold;
	u8 boot_detection_flag;
#if defined (CONFIG_TEE_TUI)
	void *tui_data;
#endif
};

struct ts_bus_info {
	enum ts_bus_type btype;
	unsigned int bus_id;
	int (*bus_write) (u8 * buf, u16 length);
	int (*bus_read) (u8 * reg_addr, u16 reg_len, u8 * buf, u16 len);
};
struct ts_aft_algo_param {
	int aft_enable_flag;
	int drv_stop_width;
	int lcd_width;
	int lcd_height;
};
struct tp_i2c_hwlock {
	int tp_i2c_hwlock_flag;
	struct hwspinlock *hwspin_lock;
};

struct ts_kit_platform_data {
	char product_name[MAX_STR_LEN];
	atomic_t state;
	atomic_t power_state;	//only used in dmd report
	atomic_t ts_esd_state;
	atomic_t register_flag;
	int get_info_flag;
	int irq_id;
	int edge_wideth;
	int irq_gpio;
	int cs_gpio;
	int reset_gpio;
	u32 fp_tp_enable;
	u32 register_charger_notifier;
	u32 hide_plain_id;
	unsigned int udfp_enable_flag;
	unsigned int spi_max_frequency;
	unsigned int spi_mode;
	unsigned int cs_reset_low_delay;
	unsigned int cs_reset_high_delay;
	struct device_node *node;
	struct i2c_client *client;
	struct spi_device *spi;
	struct pl022_config_chip spidev0_chip_info;
	struct ts_bus_info *bops;
	struct tp_i2c_hwlock i2c_hwlock;
	struct task_struct *ts_task;
	struct task_struct *ts_init_task;
	struct platform_device *ts_dev;
	struct ts_kit_device_data *chip_data;
	struct ts_feature_info feature_info;
	struct ts_chip_info_param chip_info;
	struct ts_cmd_queue queue;
	struct wake_lock ts_wake_lock;
	struct ts_cmd_queue no_int_queue;
	struct timer_list watchdog_timer;
	struct work_struct watchdog_work;
	struct input_dev *input_dev;
	struct input_dev *pen_dev;
	struct notifier_block lcdkit_notify;
	struct ts_aft_algo_param aft_param;
	struct ts_fingers fingers_send_aft_info;
	struct semaphore fingers_aft_send;
	struct semaphore diff_data_report_flag;
	atomic_t diff_data_status;
	struct ts_kit_power_supply ts_kit_powers[TS_KIT_POWER_ID_MAX];
	atomic_t fingers_waitq_flag;
	atomic_t aft_in_slow_status;
	atomic_t last_input_fingers_status;
	atomic_t last_aft_filtered_fingers;
#if defined (CONFIG_HUAWEI_DSM)
	struct ts_dsm_info dsm_info;
#endif
	struct notifier_block charger_detect_notify;
	u8 panel_id;
};

int ts_kit_put_one_cmd(struct ts_cmd_node *cmd, int timeout);
int register_ts_algo_func(struct ts_kit_device_data *chip_data,
			  struct ts_algo_func *fn);
int huawei_ts_chip_register(struct ts_kit_device_data *chipdata);
extern volatile bool ts_kit_gesture_func;
#ifdef HUAWEI_TOUCHSCREEN_TEST
int test_dbg_cmd_test(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd);
#endif

void ts_kit_rotate_rawdata_abcd2cbad(int row, int column, int *data_start,
				     int rotate_type);
int ts_kit_parse_csvfile(char *file_path, char *target_name, int32_t * data,
			 int rows, int columns);
#if defined (CONFIG_HUAWEI_DSM)
extern void ts_dmd_report(int dmd_num, const char *pszFormat, ...);
#endif
int ts_event_notify(ts_notify_event_type event);
int ts_change_spi_mode(struct spi_device *spi, u16 mode);

int ts_parse_panel_specific_config(struct device_node *np,
			       struct ts_kit_device_data *chip_data);
int ts_kit_power_supply_get(enum ts_kit_power_id power_id);
int ts_kit_power_supply_put(enum ts_kit_power_id power_id);
int ts_kit_power_supply_ctrl(enum ts_kit_power_id power_id, int status, unsigned int delay_ms);
int ts_kit_get_pt_station_status(int *status);
#endif

