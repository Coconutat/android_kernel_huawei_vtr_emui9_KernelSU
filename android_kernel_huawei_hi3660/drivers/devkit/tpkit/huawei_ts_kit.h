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
#if defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include <linux/debugfs.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/wakelock.h>
#include <linux/semaphore.h>


#include "../lcdkit/lcdkit1.0/include/lcdkit_ext.h"
#define HUAWEI_CHARGER_FB	/*define HUAWEI_CHARGER_FB here to enable charger notify callback*/
#if defined(HUAWEI_CHARGER_FB)
//#include <linux/hisi/usb/hisi_usb.h>
#endif
/* marco define*/
#ifndef strict_strtoul
#define strict_strtoul kstrtoul
#endif

#ifndef strict_strtol
#define strict_strtol kstrtol
#endif

#define NO_ERR 0
#define RESULT_ERR -1
#define RAWDATA_SIZE_LIMIT 2
#define RAW_DATA_TEST_TYPE_OLD	0
#define RAW_DATA_TEST_TYPE_SINGLE_POINT	1

#define TS_DEV_NAME "huawei,ts_kit"
#define TS_PEN_DEV_NAME "huawei,ts_pen"

#define RAW_DATA_SIZE 8192 * 4
#define TS_WATCHDOG_TIMEOUT		1000

#define AFT_WAITQ_IDLE	 (0)
#define AFT_WAITQ_WAIT	 (1)
#define AFT_WAITQ_WAKEUP (2)
#define AFT_WAITQ_IGNORE (3)

#define ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR 1

#define FP_TP_REPORT_MINOR_FLAG "fp_tp_report_touch_minor_event"
#define SYNA_3718_FW_UPDATA_FLAG "synaptics3718_fw_updata_flag"
#define SYNA_3718_TP_PRESSURE_FLAG "synaptics3718_Tp_Pressure_flag"
#define LDO29_980_DEFAULT_ON_FLAG "LDO29_980_default_on_flag"
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


#define NO_SYNC_TIMEOUT		0
#define SHORT_SYNC_TIMEOUT		5
#define LONG_SYNC_TIMEOUT		10
#define LONG_LONG_SYNC_TIMEOUT	50

#define TS_CMD_QUEUE_SIZE  20
#define TS_MAX_FINGER 10
#define TS_MAX_PEN_BUTTON 2

#define WACOM_TOOL_TYPE_NONE     0

#define WACOM_PEN_TO_RUBBER     249 //after code 248-Mute
#define WACOM_RUBBER_TO_PEN   250

#define TS_ERR_NEST_LEVEL  5
#define TS_RAWDATA_BUFF_MAX 11000

#define TS_RAWDATA_FAILED_REASON_LEN	20
#define TS_RAWDATA_STATISTICS_DATA_LEN  32
#define TS_RAWDATA_TEST_NAME_LEN		50
#define TS_RAWDATA_RESULT_CODE_LEN		4

#define TS_RAWDATA_DEVINFO_MAX	80
#define TS_RAWDATA_RESULT_MAX	200
#define TS_FB_LOOP_COUNTS 100
#define TS_FB_WAIT_TIME 5
#define MAX_CAP_DATA_SIZE 6
#define DELTA_TEST_TITLE_LEN 6
#define TX_RX_BUF_MAX 3000
#define PART_ONE_LIMIT_LENGTH	11
#define EESHORT_PRINT_SHIFT_LENGTH	1
#define MAX_CAP_DATA_SIZE_FOR_EESHORT	5

#define TS_CHIP_DMD_REPORT_SIZE  1024
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

#define TS_NV_BAR_CODE_LEN 3
#define TS_NV_BRIGHTNESS_LEN 1
#define TS_NV_WHITE_POINT_LEN 1
#define TS_NV_BRI_WHITE_LEN 1
#define TS_NV_REPAIR_LEN 1

#define I2C_WAIT_TIME 25 //25ms wait period
#define I2C_RW_TRIES 3 //retry 3 times
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

#define TS_PEN_BUTTON_NONE      0
#define TS_PEN_BUTTON_RELEASE   (1 << 5)
#define TS_PEN_BUTTON_PRESS     (1 << 6)
#define TS_PEN_KEY_NONE         0

#define TS_ALGO_FUNC_0		(1<<0)
#define TS_ALGO_FUNC_1		(1<<1)
#define TS_ALGO_FUNC_2		(1<<2)
#define TS_ALGO_FUNC_3		(1<<3)
#define TS_ALGO_FUNC_4		(1<<4)
#define TS_ALGO_FUNC_5		(1<<5)
#define TS_ALGO_FUNC_6		(1<<6)
#define TS_ALGO_FUNC_7		(1<<7)
#define TS_ALGO_FUNC_ALL	(0xFF)

/* ts switch func begin */
#define TS_SWITCH_TYPE_DOZE		(1<<0)
#define TS_SWITCH_TYPE_GAME		(1<<1)
#define TS_SWITCH_TYPE_SCENE	(1<<2)
#define TS_SWITCH_TYPE_FM		(1<<3)

#define TS_SWITCH_DOZE_ENABLE	1
#define TS_SWITCH_DOZE_DISABLE	2
#define TS_SWITCH_GAME_ENABLE	3
#define TS_SWITCH_GAME_DISABLE	4
#define TS_SWITCH_SCENE_ENTER	3
#define TS_SWITCH_SCENE_EXIT	4
#define TS_SWITCH_FM_ENABLE		1
#define TS_SWITCH_FM_DISABLE	2

#define TS_SWITCH_ROI_DELAY_ENABLE	1

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
#define TS_SWITCH_FM_MODE	21
/* ts switch func end */

#define MAX_STR_LEN 32
#define FULL_NAME_MAX_LEN 	128
#define TS_CAP_TEST_TYPE_LEN 100
#define TS_GESTURE_COMMAND 0x7ff
#define TS_GESTURE_INVALID_COMMAND 0xFFFF
#define TS_GESTURE_PALM_BIT 0x0800
#define TS_GET_CALCULATE_NUM 2048
#define TS_GESTURE_INVALID_CONTROL_NO 0xFF
#define RAWDATA_CAP_TEST_FAIL "1F"
#define RAWDATA_CAP_TEST_PASS "1P"
#define TRX_DELTA_CAP_TEST_FAIL "-2F"
#define TRX_DELTA_CAP_TEST_PASS "-2P"
#define TD43XX_EE_SHORT_TEST_FAIL "-5F"
#define TD43XX_EE_SHORT_TEST_PASS "-5P"

#define CHIP_INFO_LENGTH	16
#define RAWDATA_NUM 16
#define MAX_POSITON_NUMS 6
#ifndef CONFIG_OF
#define SUPPORT_IC_NUM 4
#define IC_NAME_UNDEF "null"
#define CHIP_FULL_NAME "touchscreen/support_chip_name_"
#define CHIP_SLAVE_ADDR_NAME "/slave_address"
#define INT_CONVERT_OFFSET	49
#endif
#define ROI
#ifdef ROI
#define ROI_HEAD_DATA_LENGTH		4
#define ROI_DATA_READ_LENGTH		102
#define ROI_DATA_SEND_LENGTH		(ROI_DATA_READ_LENGTH-ROI_HEAD_DATA_LENGTH)
#define ROI_CTRL_DEFAULT_ADDR		0x0446
#define ROI_DATA_DEFAULT_ADDR		0x0418
#endif
#define VIRTUAL_KEY_ELEMENT_SIZE	5
#define MAX_PRBUF_SIZE	PIPE_BUF
#define CALIBRATION_DATA_SIZE 6144
#define TP_I2C_HWSPIN_LOCK_CODE   28
#define GET_HARDWARE_TIMEOUT 100000
extern u8 g_ts_kit_log_cfg;

#define HWLOG_TAG	TS_KIT
HWLOG_REGIST();
#define TS_LOG_INFO(x...)		_hwlog_info(HWLOG_TAG, ##x)
#define TS_LOG_ERR(x...)		_hwlog_err(HWLOG_TAG, ##x)
#define TS_LOG_DEBUG(x...)	\
    do { \
        if (TS_DBG_MODE == g_ts_kit_log_cfg)	\
            _hwlog_info(HWLOG_TAG, ##x);	\
    }while(0)
#define TS_INF_MODE 0
#define TS_DBG_MODE 1
#define TS_MT_MODE  2
#define TS_MT_MODE_ON (TS_MT_MODE == g_ts_kit_log_cfg)

#define TP_FINGER 				1
#define TP_STYLUS				2
#define TP_GLOVE 				6
#define FILTER_GLOVE_NUMBER	4

#define PEN_MOV_LENGTH      120  //move length (pixels)
#define FINGER_REL_TIME     300  //the time pen checked after finger released shouldn't less than this value(ms)

#define TP_3320_SHORT_ARRAY_NUM	4
#define GPIO_HIGH        1

#define TS_CMD_CHECK_KEY 0X8080

enum TP_ic_type
{
    ONCELL = 0,     //lcd & tp have separate regulator
    HYBRID,		//lcd &tp share 1.8V
    TDDI,	             //lcd ctrl all the regulator
	AMOLED, 			 //Amoled need to set 1.8 and 3.3 for itself.
    TP_IC_TYPE_MAX = 255,	
};

enum TP_state_machine
{
    INIT_STATE = 0,
    ZERO_STATE = 1,		//switch finger to glove
    FINGER_STATE = 2,	//finger state
    GLOVE_STATE = 3	//glove state
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

enum ts_cmd
{
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
    TS_PALM_KEY,
    TS_INVAILD_CMD = 255,
};

enum ts_pm_type
{
    TS_BEFORE_SUSPEND = LCDKIT_TS_BEFORE_SUSPEND,
    TS_SUSPEND_DEVICE = LCDKIT_TS_SUSPEND_DEVICE,
    TS_RESUME_DEVICE = LCDKIT_TS_RESUME_DEVICE,
    TS_AFTER_RESUME = LCDKIT_TS_AFTER_RESUME,
    TS_IC_SHUT_DOWN = LCDKIT_TS_IC_SHUT_DOWN,
};

enum ts_bus_type
{
    TS_BUS_I2C = 0,
    TS_BUS_SPI,
    TS_BUS_UNDEF = 255,
};

enum ts_irq_config
{
    TS_IRQ_LOW_LEVEL,
    TS_IRQ_HIGH_LEVEL,
    TS_IRQ_RAISE_EDGE,
    TS_IRQ_FALL_EDGE,
};

enum ts_action_status
{
    TS_ACTION_READ,
    TS_ACTION_WRITE,
    TS_ACTION_SUCCESS,
    TS_ACTION_FAILED,
    TS_ACTION_UNDEF,
};

enum ts_dev_state
{
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
enum ts_register_state
{
    TS_UNREGISTER = 0,
    TS_REGISTER_DONE,
    TS_REGISTER_UNDEFINE = 255,
};
enum ts_rawdata_arange_type{
	TS_RAWDATA_TRANS_NONE = 0,
	TS_RAWDATA_TRANS_ABCD2CBAD,
	TS_RAWDATA_TRANS_ABCD2ADCB,
};

#define KEY_F26 196
enum ts_gesture_num
{
    //	TS_NUM_TOTAL = 12, /* total gesture numbers  */
    TS_DOUBLE_CLICK = KEY_F1, /*0.Double tap:KEY_F1*/
    TS_SLIDE_L2R = KEY_F2, /*1.Single finger slide from left to right:KEY_F2*/
    TS_SLIDE_R2L = KEY_F3, /*2.Single finger slide from right to left:KEY_F3*/
    TS_SLIDE_T2B = KEY_F4, /*3.Single finger slide from top to bottom:KEY_F4*/
    TS_SLIDE_B2T = KEY_F5, /*4.Single finger slide from bottom to top:KEY_F5*/
    TS_CIRCLE_SLIDE = KEY_F7, /*5.Single finger slide circle:KEY_F7*/
    TS_LETTER_c = KEY_F8, /*6.Single finger write letter c*:KEY_F8*/
    TS_LETTER_e = KEY_F9, /*7.Single finger write letter e:KEY_F9*/
    TS_LETTER_m = KEY_F10, /*8.Single finger write letter m:KEY_F10*/
    TS_LETTER_w = KEY_F11, /*9.Single finger write letter w:KEY_F11*/
    TS_PALM_COVERED = KEY_F12, /*10.Palm off screen:KEY_F12*/
    TS_KEY_IRON = KEY_F26,
    TS_GESTURE_INVALID = 0xFF,/*FF.No gesture*/
};
enum ts_gesture_enable_bit
{
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
enum ts_touchplus_num
{
    TS_TOUCHPLUS_KEY0 = KEY_F21,
    TS_TOUCHPLUS_KEY1 = KEY_F22,
    TS_TOUCHPLUS_KEY2 = KEY_F23,
    TS_TOUCHPLUS_KEY3 = KEY_F19,
    TS_TOUCHPLUS_KEY4 = KEY_F20,
    TS_TOUCHPLUS_INVALID = 0xFF,
};
enum ts_rawdata_debug_type
{
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
#if defined(HUAWEI_CHARGER_FB)
enum ts_charger_state
{
    USB_PIUG_OUT= 0,
    USB_PIUG_IN,
};
#endif
typedef enum {
	TS_PEN_OUT_RANGE = 0,   /* pen out of range */
	TS_PEN_IN_RANGE,		/* pen in range */

	/* add event before here */
	TS_EVENT_MAX,		    /* max event type */
} ts_notify_event_type;

struct ts_unique_capacitance_test {
	int Read_only_support_unique;
	int32_t* Rx_delta_abslimit;
	int32_t* Tx_delta_abslimit;
	int32_t* ee_short_data_limit;
	int32_t* noise_data_limit;
};
struct ts_rawdata_limit_tab {
	int32_t* MutualRawMax;
	int32_t* MutualRawMin;
	struct ts_unique_capacitance_test *unique_test;
};
struct ts_finger
{
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

struct ts_fingers
{
    struct ts_finger fingers[TS_MAX_FINGER];
    int cur_finger_number;
    unsigned int gesture_wakeup_value;
    unsigned int special_button_key;
    unsigned int special_button_flag;
    unsigned int add_release_flag;
};

struct ts_palm_key
{
	unsigned int palm_button_key;
};

struct ts_tool
{
    int pen_inrange_status;
    int tip_status;
    int x;
    int y;
    int pressure;
    signed char tilt_x;
    signed char tilt_y;
    int tool_type;// BTN_TOOL_RUBBER  BTN_TOOL_PEN
};

struct ts_button
{
    int status;
    int key;//BTN_STYLUS
};


struct ts_pens
{
    struct ts_tool tool;
    struct ts_button buttons[TS_MAX_PEN_BUTTON];
};

struct ts_algo_func
{
    int algo_index; //from 0 to max
    char* algo_name;
    struct list_head node;
    int (*chip_algo_func) (struct ts_kit_device_data* dev_data, struct ts_fingers* in_info, struct ts_fingers* out_info);
};

struct algo_param
{
    u32 algo_order;
    struct ts_fingers info;
};

struct fw_param
{
    char fw_name[MAX_STR_LEN * 4]; //firmware name contain 4 parts
};

/*
 New data format for rawdata
*/
#define RAWDATA_TEST_FAIL_CHAR 'F'
#define RAWDATA_TEST_PASS_CHAR 'P'

enum ts_rawdata_formattype
{
    TS_RAWDATA_OLDFORMAT = 0,
    TS_RAWDATA_NEWFORMAT,
};
enum ts_raw_data_type
{
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
	RAW_DATA_END,
};
struct ts_rawdata_newnodeinfo
{    
    struct list_head node;
	u8 typeindex;       /* enum ts_raw_data_type */
	char testresult;    /* 'P' o 'F' */
	int *values;
	size_t size;
	char test_name[TS_RAWDATA_TEST_NAME_LEN];	
	char statistics_data[TS_RAWDATA_STATISTICS_DATA_LEN];   /* [%d,%d,%d] */
	char tptestfailedreason[TS_RAWDATA_FAILED_REASON_LEN];  /* "-panel_reason" o "-software reason" */
};

struct ts_rawdata_info_new
{
    int status;
	int rx;
	int tx;
    ktime_t time_stamp;
    char deviceinfo[TS_RAWDATA_DEVINFO_MAX];       /* ic info */
	char i2cinfo[TS_RAWDATA_RESULT_CODE_LEN];      /* i2c test result:0P or 0F */
	char i2cerrinfo[TS_RAWDATA_FAILED_REASON_LEN]; /* i2c test fail reason*/
	int listnodenum;
	struct list_head rawdata_head;//ts_rawdata_newnodeinfo  list
};

struct ts_rawdata_info
{
    int status;
    int op_action;
    int used_size; // fill in rawdata size
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

struct ts_calibration_info_param{
	int status;
	int calibration_crc;
};
struct ts_diff_data_info
{
    int status;
    int op_action;
    int used_size;
    ktime_t time_stamp;
    int debug_type;
    int buff[TS_RAWDATA_BUFF_MAX];
    char result[TS_RAWDATA_RESULT_MAX];
};

struct ts_chip_info_param
{
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
struct ts_calibrate_info
{
    int status;
};

struct ts_dsm_debug_info
{
    int status;
};

#if defined(HUAWEI_CHARGER_FB)
struct ts_charger_info
{
    u8 charger_supported;
    u8 charger_switch;
    int op_action;
    int status;
    u16 charger_switch_addr;
    u16 charger_switch_bit;
};
#endif

struct ts_special_hardware_test_info
{
    u8 switch_value;
    int op_action;
    int status;
    char* result;
};

struct ts_glove_info
{
    u8 glove_supported;
    u8 glove_switch;
    int op_action;
    int status;
    u32 glove_switch_addr;
    u16 glove_switch_bit;
};

struct ts_holster_info
{
    u8 holster_supported;
    u8 holster_switch;
    int op_action;
    int status;
    u32 holster_switch_addr;
    u16 holster_switch_bit;
};

struct ts_roi_info
{
    u8 roi_supported;
    u8 roi_switch;
    int op_action;
    int status;
    u32 roi_control_addr;
    u8 roi_control_bit;
    u16 roi_data_addr;
};

struct ts_scene_switch_info
{
	u8 scene;
	u8 operator;
};

enum ts_sleep_mode
{
    TS_POWER_OFF_MODE = 0,
    TS_GESTURE_MODE,
};

struct ts_easy_wakeup_info
{
    enum ts_sleep_mode sleep_mode;
    int off_motion_on;
    unsigned int easy_wakeup_gesture;
    int easy_wakeup_flag;
    int palm_cover_flag;
    int palm_cover_control;
    unsigned char easy_wakeup_fastrate;
    unsigned int easywake_position[MAX_POSITON_NUMS];
};

struct ts_wakeup_gesture_enable_info
{
    u8 switch_value;
    int op_action;
    int status;
};

struct ts_regs_info
{
    unsigned int addr;
    int bit;
    u8 values[TS_MAX_REG_VALUE_NUM];
    int num;
    u8 op_action;
    int status;
};

struct ts_window_info
{
    int window_enable;
    int top_left_x0;
    int top_left_y0;
    int bottom_right_x1;
    int bottom_right_y1;
    int status;
};

struct ts_test_type_info
{
    char tp_test_type[TS_CAP_TEST_TYPE_LEN];
    int op_action;
    int status;
};

#if defined (CONFIG_HUAWEI_DSM)
struct ts_dsm_info
{
    char fw_update_result[8];
    unsigned char constraints_LDO17_status;
    unsigned char constraints_LSW50_status;
    int constraints_I2C_status;
    int constraints_UPDATE_status;
};
#endif

struct ts_hand_info
{
    u8 hand_value;
    int op_action;
    int status;
};

struct ts_pen_info
{
    u8 pen_supported;
};
struct ts_feature_info
{
    struct ts_glove_info glove_info;
    struct ts_holster_info holster_info;
    struct ts_window_info window_info;
    struct ts_wakeup_gesture_enable_info wakeup_gesture_enable_info ;
    struct ts_roi_info roi_info;
#if defined(HUAWEI_CHARGER_FB)
    struct ts_charger_info charger_info;
#endif
    struct ts_special_hardware_test_info hardware_test_info;
    struct ts_pen_info pen_info;
};

struct ts_cmd_param
{
    union
    {
        struct algo_param algo_param; //algo cal
        struct ts_fingers report_info; //report input
        struct ts_pens report_pen_info; //report pen
        struct fw_param firmware_info; //firmware update
        enum ts_pm_type pm_type;
        struct ts_kit_device_data *chip_data;
        unsigned int ts_key;
    } pub_params;
    void* prv_params;
	void (*ts_cmd_freehook)(void*);
};

enum ts_timeout_flag
{
    TS_TIMEOUT = 0,
    TS_NOT_TIMEOUT,
    TS_UNDEF = 255,
};

enum ts_rawdata_work_flag
{
    TS_RAWDATA_IDLE = 0,
    TS_RAWDATA_WORK,
};

struct ts_palm_info
{
    u8 palm_switch;
    int op_action;
    int status;
};

struct ts_single_touch_info
{
    u16 single_touch_switch;
    int op_action;
    int status;
};
struct ts_cmd_sync
{
    atomic_t timeout_flag;
    struct completion done;
};

struct ts_cmd_node
{
    int ts_cmd_check_key;
    enum ts_cmd command;
    struct ts_cmd_sync* sync;
    struct ts_cmd_param cmd_param;
};

struct ts_regulator_contrl
{
    int vci_value;
    int vddio_value;
    int need_set_vddio_value;
};

struct ts_cmd_queue
{
    int wr_index;
    int rd_index;
    int cmd_count;
    int queue_size;
    spinlock_t spin_lock;
    struct ts_cmd_node ring_buff[TS_CMD_QUEUE_SIZE];
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

struct ts_device_ops
{
    int (*chip_detect)(struct ts_kit_platform_data* data);
    int (*chip_wrong_touch)(void);
    int (*chip_init)(void);
    int (*chip_get_brightness_info) (void);
    int (*chip_parse_config)(struct device_node* device, struct ts_kit_device_data* data);
    int (*chip_input_config)(struct input_dev* input_dev);
    int (*chip_input_pen_config)(struct input_dev* input_dev);
    int (*chip_register_algo)(struct ts_kit_device_data* data);
    int (*chip_irq_top_half)(struct ts_cmd_node* cmd);
    int (*chip_irq_bottom_half)(struct ts_cmd_node* in_cmd, struct ts_cmd_node* out_cmd);
    int (*chip_reset)(void);
    int (*chip_debug_switch)(u8 loglevel);
    void (*chip_shutdown)(void);
    int (*oem_info_switch) (struct ts_oem_info_param *info);
    int (*chip_get_info)(struct ts_chip_info_param* info);
    int (*chip_set_info_flag)(struct ts_kit_platform_data* info);
    int (*chip_fw_update_boot)(char* file_name);
    int (*chip_fw_update_sd)(void);
    int (*chip_calibrate)(void);
    int (*chip_calibrate_wakeup_gesture)(void);
    int (*chip_dsm_debug)(void);
    int (*chip_get_rawdata)(struct ts_rawdata_info* info, struct ts_cmd_node* out_cmd);
	int (*chip_get_calibration_data)(struct ts_calibration_data_info *info,
			struct ts_cmd_node *out_cmd);
	int (*chip_get_calibration_info)(struct ts_calibration_info_param *info,
			struct ts_cmd_node *out_cmd);
    int (*chip_glove_switch)(struct ts_glove_info* info);
    int (*chip_palm_switch)(struct ts_palm_info* info);
    int (*chip_single_touch_switch)(struct ts_single_touch_info* info);
    int (*chip_wakeup_gesture_enable_switch)(struct ts_wakeup_gesture_enable_info* info);
    int (*chip_charger_switch)(struct ts_charger_info* info);
    int (*chip_holster_switch)(struct ts_holster_info* info);
    int (*chip_roi_switch)(struct ts_roi_info* info);
    unsigned char* (*chip_roi_rawdata)(void);
    int (*chip_hand_detect)(struct ts_hand_info* info);
    int (*chip_before_suspend)(void);
    int (*chip_suspend)(void);
    int (*chip_resume)(void);
    int (*chip_after_resume)(void* feature_info);
    int (*chip_test)(struct ts_cmd_node* in_cmd, struct ts_cmd_node* out_cmd);
    int (*chip_check_status)(void);
    int (*chip_hw_reset)(void);
    int (*chip_regs_operate)(struct ts_regs_info* info);
    int (*chip_get_capacitance_test_type)(struct ts_test_type_info* info);
    int (*chip_get_debug_data)(struct ts_diff_data_info* info, struct ts_cmd_node* out_cmd);
    void (*chip_special_hardware_test_swtich)(unsigned int value);
    int (*chip_special_hardware_test_result)(char* buf);
	void (*chip_ghost_detect) (int value);
    void (*chip_touch_switch) (void);
    void (*chip_work_after_input) (void);
    int (*chip_special_rawdata_proc_printf) (struct seq_file *m, struct ts_rawdata_info *info,
					int range_size, int row_size);
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
struct ts_kit_device_data
{
	bool is_parade_solution;
	bool is_ic_rawdata_proc_printf;
	bool is_direct_proc_cmd;
	bool support_s3320_short_test;
	unsigned int support_ext_trex_short_test;
    bool is_i2c_one_byte;
    bool is_new_oem_structure;
    bool disable_reset;
    bool is_in_cell;
    bool report_tui_enable;
    u8 tui_set_flag;
	u8 provide_panel_id_support;
    bool need_wd_check_status;
    int check_status_watchdog_timeout;
    u16 trx_short_circuit_array[TP_3320_SHORT_ARRAY_NUM];
    int rawdata_arrange_swap;
    int rawdata_get_timeout;
    int has_virtualkey;
    char chip_name[MAX_STR_LEN];
    char module_name[MAX_STR_LEN];
    char version_name[MAX_STR_LEN];
    char project_id[MAX_STR_LEN];
    char tp_test_type[TS_CAP_TEST_TYPE_LEN];
    struct device_node* cnode;
    struct ts_device_ops* ops;
    struct ts_easy_wakeup_info easy_wakeup_info;
    struct list_head algo_head;//algo function list
    struct ts_kit_platform_data* ts_platform_data;
    struct ts_regulator_contrl regulator_ctr;
    int vci_gpio_type;
    int vci_regulator_type;
    int vci_gpio_ctrl;
    int vddio_gpio_type;
    int vddio_regulator_type;
    int vddio_gpio_ctrl;
    int synaptics3718_fw_updata_flag;
    int synaptics3718_Tp_Pressure_flag;
    int LDO29_980_default_on_flag;
    int algo_size;
    int algo_id;
    int slave_addr;
    int irq_config; // 0 - LOW LEVEL  1 - HIGH LEVEL  2 - RAISE EDGE  3 - FALL EDGE
	int ic_type;
	int projectid_len;
	int rawdata_arrange_type;
    int x_max;
    int y_max;
    int x_max_mt;
    int y_max_mt;
    bool flip_x;
    bool flip_y;
    int reset_delay;
    int reg_num;
    u8 reg_values[TS_MAX_REG_VALUE_NUM];
    int raw_limit_buf[RAWDATA_NUM];
    /* touch switch info */
    char touch_switch_info[MAX_STR_LEN];
    unsigned int touch_switch_flag;
    unsigned short touch_switch_reg;
    unsigned short touch_switch_hold_off_reg;
    unsigned short touch_game_reg;
    unsigned int support_aft;
    unsigned short aft_data_addr;
    u8 game_control_bit;
    int supported_func_indicater;
    int rawdata_debug_type;
    int capacitance_test_config ;
    int support_3d_func;
    int test_rawdata_normalizing;
    int cover_force_glove;
    int bootloader_update_enable;
	// syna_wx_wy
	int is_multi_protocal;
	unsigned char adv_width[4];
    int unite_cap_test_interface;
    int lcd_full;
    bool check_bulcked;
	int should_check_tp_calibration_info;
    int fw_update_logic;
    int* upper;
    int* lower;
    int test_enhance_raw_data_capacitance;
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
    u8 rawdata_report_type;
	int raw_test_type;
	bool self_cap_test;
    int report_rate_test;
	int ghost_detect_support;
    int noise_state_reg;
    u32 ic_status_reg;
    int noise_record_num;
    int frequency_selection_reg;
    bool enable_ghost_dmd_report;
#if defined (CONFIG_TEE_TUI)
	void *tui_data;
#endif
	int use_ub_supported;
	int delay_for_fw_update;
	int byte_to_byte_cmp_config_id;
	int delay_for_erase_fw;
	struct anti_false_touch_param anti_false_touch_param_data;
	bool isbootupdate_finish;
	bool is_can_device_use_int;
	struct mutex device_call_lock;
	int sleep_in_mode;
	unsigned int get_brightness_info_flag;
	int check_fw_right_flag;
	int use_lcdkit_power_notify;
	int fp_tp_report_touch_minor_event;
	int support_crc_err_do_reset;
	u8 rawdata_newformatflag;   // 0 - old format   1 - new format
	unsigned int roi_delay_flag;
	u8 download_fw_incharger; // 0 - not support    1 - support
};

struct ts_bus_info
{
    enum ts_bus_type btype;
    int bus_id;
    int (*bus_write) (u8* buf, u16 length);
    int (*bus_read) (u8* reg_addr, u16 reg_len, u8* buf, u16 len);
};
struct ts_aft_algo_param
{
    char aft_enable_flag;
    int drv_stop_width;
    int lcd_width;
    int lcd_height;
};
struct tp_i2c_hwlock
{
    int  tp_i2c_hwlock_flag;
    struct hwspinlock * hwspin_lock;
};

struct ts_kit_platform_data
{
    char product_name[MAX_STR_LEN];
    atomic_t state;
    atomic_t power_state;//only used in dmd report
    atomic_t ts_esd_state;
    atomic_t register_flag;
    int get_info_flag;
    int irq_id;
    int edge_wideth;
    int irq_gpio;
    int cs_gpio;
    int reset_gpio;
    int fpga_flag;
    u32 fp_tp_enable;
    u32 register_charger_notifier;
    u32 hide_plain_id;
    u32 touch_switch_need_process;
    u8 panel_id;
    unsigned int udfp_enable_flag;
    unsigned int spi_max_frequency;
    unsigned int spi_mode;
    unsigned int cs_reset_low_delay;
    unsigned int cs_reset_high_delay;
#if defined(HUAWEI_CHARGER_FB)
    struct notifier_block charger_detect_notify;
#endif
    struct device_node* node;
    struct i2c_client* client;
    struct spi_device *spi;
    struct pl022_config_chip spidev0_chip_info;
    struct ts_bus_info* bops;
    struct tp_i2c_hwlock  i2c_hwlock;
    struct task_struct* ts_task;
    struct task_struct* ts_init_task;
    struct platform_device* ts_dev;
    struct ts_kit_device_data* chip_data;
    struct ts_feature_info feature_info;
    struct ts_chip_info_param chip_info;
    struct ts_cmd_queue queue;
    struct wake_lock ts_wake_lock;
	struct ts_cmd_queue no_int_queue;
    struct timer_list watchdog_timer;
    struct work_struct watchdog_work;

    struct input_dev* input_dev;
    struct input_dev* pen_dev;

    struct notifier_block lcdkit_notify;
    struct ts_aft_algo_param  aft_param;
    struct ts_fingers fingers_send_aft_info;
    struct semaphore fingers_aft_send;
    atomic_t fingers_waitq_flag;
#if defined(CONFIG_FB)
    struct notifier_block fb_notify;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
    struct early_suspend early_suspend;
#endif
#if defined (CONFIG_HUAWEI_DSM)
    struct ts_dsm_info dsm_info;
#endif
};

bool tp_get_prox_status(void);
int ts_kit_power_control_notify(enum ts_pm_type pm_type,  int timeout);
void ts_kit_thread_stop_notify(void);
int  ts_kit_put_one_cmd(struct ts_cmd_node * cmd, int timeout);
int ts_kit_put_one_cmd_thread(struct ts_cmd_node *cmd, int timeout);
int register_ts_algo_func(struct ts_kit_device_data* chip_data, struct ts_algo_func* fn);
void ts_kit_anti_false_touch_param_achieve(struct ts_kit_device_data *chip_data);
int  huawei_ts_chip_register(struct ts_kit_device_data* chipdata);
int ts_kit_proc_command_directly(struct ts_cmd_node *cmd);
extern volatile bool ts_kit_gesture_func;
extern volatile int g_tskit_ic_type;  //this type means oncell incell tddi ... in order to decide the power policy between lcd & tp
extern volatile int g_tskit_pt_station_flag;
extern volatile int   not_get_special_tp_node ;
extern volatile int g_ts_kit_lcd_brightness_info;
extern volatile bool g_tskit_fw_upgrade_flag;
extern void ts_kit_check_bootup_upgrade(void);
#ifdef HUAWEI_TOUCHSCREEN_TEST
int test_dbg_cmd_test(struct ts_cmd_node* in_cmd, struct ts_cmd_node* out_cmd);
#endif
#if defined (CONFIG_TEE_TUI)
void ts_kit_tui_secos_init(void);
void ts_kit_tui_secos_exit(void);
#endif
void ts_check_touch_window(struct ts_fingers* finger);
void ts_work_after_input(void);
void ts_film_touchplus(struct ts_fingers* finger, int finger_num, struct input_dev* input_dev);
void lcdkit_fps_ts_callback(void);
int lcdkit_fps_support_query(void);
int lcdkit_fps_tscall_support_query(void);
void ts_kit_rotate_rawdata_abcd2cbad(int row, int column, int *data_start, int rotate_type);
int ts_kit_parse_csvfile(char *file_path, char *target_name, int32_t  *data, int rows, int columns);
#if defined (CONFIG_HUAWEI_DSM)
extern void ts_dmd_report(int dmd_num, const char* pszFormat, ...);
#endif
int ts_event_notify(ts_notify_event_type event);
int ts_change_spi_mode(struct spi_device *spi, u16 mode);
#endif

