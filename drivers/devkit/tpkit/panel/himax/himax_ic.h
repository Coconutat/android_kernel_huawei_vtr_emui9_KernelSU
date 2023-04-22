/* Himax Android Driver Sample Code for Himax chipset
*
* Copyright (C) 2014 Himax Corporation.
*
* This software is licensed under the terms of the GNU General Public
* License version 2, as published by the Free Software Foundation, and
* may be copied, distributed, and modified under those terms.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
*/

#ifndef HIMAX_IC_H
#define HIMAX_IC_H

#include <asm/segment.h>
#include <asm/uaccess.h>
#include <asm/atomic.h>

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/module.h>
#include <linux/async.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/input/mt.h>
#include <linux/firmware.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/buffer_head.h>
#include <linux/wakelock.h>
#include <linux/seq_file.h>
#include <linux/proc_fs.h>
#include "himax_platform.h"
#include <linux/regulator/consumer.h>
#include <dsm/dsm_pub.h>
#include <linux/ctype.h>
#include "../../huawei_ts_kit_algo.h"
#include "../../huawei_ts_kit.h"

#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif

#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif

#define HX_ERR -1
#define POWER_TYPE_GPIO 0
#define HIMAX_DRIVER_VER "0.0.12.0"

#define HIMAX_CORE_NAME "Himax852x"
#define HIMAX_PRODUCT_NAME "Himax"

#define INPUT_DEV_NAME	"himax-touchscreen"
#define FLASH_DUMP_FILE "/data/user/Flash_Dump.bin"

#define STR_IC_VENDOR "HIMAX"
#define STR_IC_NAME "hx8529f"
#define MODULE_NAME "auo"
#define PRODUCE_ID "CHOP41170"

#define HX_FW_NAME_HX8529F "chopin_hx8529f_auo_fw.bin"
#define HX_FW_NAME_HX852X "ts/touch_screen_firmware.bin"
#define HX_FW_SNIFF ".bin"

#define HX_HAND_SHAKING_READ_MAX_SIZE 	1
#define HX_HAND_SHAKING_WRITE_MAX_SIZE 	2
#define HX_READ_SENSOR_ID_READ_MAX_SIZE 1
#define HX_READ_SENSOR_ID_WRITE_MAX_SIZE 3
#define HX_TOUCH_INFORMATION_MAX_SIZE 12
#define HX_IC_PACKAGE_CHECK_MAX_SIZE 3
#define HX_READ_TP_INFO_MAX_SIZE 12
#define HX_RECEIVE_BUF_MAX_SIZE 128
#define HX_HAND_SHAKING_MAX_TIME 100
#define HX_COORDS_MAX_SIZE 4
#define HX_PROJECT_ID_LEN 9
#define CHIP_NAME_LEN 8
#define ABS_X_MAX_DEFAULT 1200
#define ABS_Y_MAX_DEFAULT 1920
#define HX_COORD_BYTE_NUM 4//coord
#define ONEBYTE 1
#define IQ_CMDBUF_LEN 11
#define IQ_VALUEBUF_LEN 16
#define IQ_BACK_VAL_LEN 7
#define IQ_SRAM_CMD_LEN 2
#define	SET_CLK_DATA_LEN 3
#define	SET_CRYSTAL_DATA_LEN 3
#define	R36H_REG_DATA_LEN 2
#define	IRQ_DISABLE 0
#define	IRQ_ENABLE 1
#define RST_DISABLE 0
#define RST_ENABLE 1
#define SLEEP_OFF_BUF_LEN 2
#define SLEEP_ON_BUF_LEN 12
#define GPIO_3V3_ENABLE 1
#define GPIO_3V3_DISABLE 0
#define GPIO_1V8_ENABLE 1
#define GPIO_1V8_DISABLE 0
#define HW_RST_FLAT_ENABLE 1

#define SHIFT_ONE_BYTE	8
#define SHIFT_TWO_BYTE	16
#define SHIFT_THREE_BYTE	24
#define HX_MASK_VALUE 0x000000FF

#define TS_WAKE_LOCK_TIMEOUT 5*HZ
#define DOUBLE_CLICK_WAKEUP  		(0x80)
#define SPECIFIC_LETTER_W  			(0x0b)
#define SPECIFIC_LETTER_M			(0x07)
#define SPECIFIC_LETTER_E			(0x0c)
#define SPECIFIC_LETTER_C 			(0x05)
#define LETTER_LOCUS_NUM 	6
#define LINEAR_LOCUS_NUM 	2
#define GEST_PTLG_ID_LEN    	4
#define GEST_PTLG_HDR_LEN  	4
#define GEST_PTLG_HDR_ID1   			 0xCC
#define GEST_PTLG_HDR_ID2   			 0x44
#define GEST_PT_MAX_NUM     			 128
#define IS_APP_ENABLE_GESTURE(x)  ((u32)(1<<x))

#define HIMAX_THRESHOLD_NAME_LEN		100

enum himax_hand_shaking_result{
	HX_HAND_SHAKING_RUNNING=0,
	HX_HAND_SHAKING_STOP,
	HX_HAND_SHAKING_I2C_FAIL,
};
enum start_get_rawdata_from_event_result{
	HX_OK=0,
	HX_ERROR,
};
enum himax_HW_reset_eunm{
	HX_LOADCONFIG_EN=true,
	HX_LOADCONFIG_DISABLE=false,
	HX_INT_EN=true,
	HX_INT_DISABLE=false,
};
#define HX_SELFTEST_EN	1
#define HX_SELFTEST_DIS		0
#define CHK_FC_FLG_SET		1
#define CHK_FC_FLG_RLS		0
#define CRC_PASS_WD			0
#define MAX_RAW_VAL			255
#define HX_SLEEP_5S			5000
#define HX_SLEEP_3S			3000
#define HX_SLEEP_1S			1000
#define HX_SLEEP_2S			2000
#define HX_SLEEP_500MS		500
#define HX_SLEEP_200MS		200
#define HX_SLEEP_120MS		120
#define HX_SLEEP_100MS		100
#define HX_SLEEP_50MS		50
#define HX_SLEEP_40MS		40
#define HX_SLEEP_30MS		30
#define HX_SLEEP_20MS		20
#define HX_SLEEP_10MS		10
#define HX_SLEEP_5MS		5
#define HX_SLEEP_2MS		2
#define HX_SLEEP_1MS		1
#define SLAVE_I2C_ADRR 0x48
#define RAWDATA_GET_TIME_DEFAULT 0
#define	HX8529_FW_VER_MAJ_FLASH_ADDR	 0xFD85
#define	HX8529_FW_VER_MIN_FLASH_ADDR	 0xFD86
#define	HX8529_CFG_VER_MAJ_FLASH_ADDR	 0xFDA0
#define	HX8529_CFG_VER_MIN_FLASH_ADDR 	 0xFDAC
#define	HX8529_FW_CFG_VER_FLASH_ADDR	 0xFD84
#define	HX_REG_CHECKSUMRESULT		0xAD
#define	HX_REG_SRAM_ADDR			0x8B
#define	HX_REG_SRAM_SWITCH          0x8C
#define	HX_REG_SRAM_TEST_MODE_EN		0x11
#define	HX_REG_E_SRAM_TEST_MODE_EN		0x14

#define	HX_REG_SRAM_TEST_MODE_DISABLE	0x0
#define	HX_REG_IREF					0xED
#define	HX_REG_FLASH_WPLACE			0x40
#define	HX_REG_FLASH_RPLACE			0x5A
#define	HX_REG_RAWDATA_MODE			0xF1
#define	HX_REG_CLOSE_FLASH_RELOAD	0x9B
#define	HX_REG_FLASH_MODE			0xF4
#define	HX_REG_FLASH_MANUAL_MODE	0x42
#define	HX852XES_REG_FLASH_MANUAL_MODE	0x35
#define	HX_REG_FLASH_MANUAL_ON		0x01
#define	HX_REG_FLASH_MANUAL_OFF		0x00
#define	HX_REG_SET_FLASH_EN			0x43//HX_REG_SET_FLASH_EN
#define	HX_REG_SET_FLASH_ADDR		0x44
#define	HX_REG_SET_CLK_ADDR 		0x97

#define HXXES_ADDR_SMWP	0x8F
#define HXXES_DATA_SMWP_ON	0x20
#define HXXES_DATA_SMWP_OFF	0x00

/*
R44:
1st parameter:reserve reserve reserve BYTES_ADDR[4:0]
2st parameter:reserve reserve SECTOR_ADDR[10:5]
3st parameter:reserve reserve reserve reserve reserve PAGE_ADDR[13:11]
*/
#define	HX_REG_SET_FLASH_DATA		0x45
#define	HX_REG_FLASH_TRASFER		0x46
#define	HX_REG_FLASH_BPW_START		0x4A
#define	HX_REG_SET_FLASH_MANUAL_0	0x5B
#define	HX_REG_SET_FLASH_MANUAL_1	0x5C
#define	HX_REG_CHIP_ERASE			0x4F
#define	HX_REG_RELOAD_FLAG			0x36
#define	HX_REG_IC_VER				0xD1
#define	HX_REG_EVENT_STACK			0x86
#define	HX_REG_ID_PIN_DEF			0x56
#define	HX_REG_ID_PIN_STATUS		0x57
#define	HX_REG_FLASH_CRC_SEL		0x94
#define	HX_REG_SET_OSC_4_PUMP		0xa6

#define	HX8527_RX_NUM				14
#define	HX8527_TX_NUM	 			29
#define	HX8527_MAX_PT				10

#define	HX8529_RX_NUM				38
#define	HX8529_TX_NUM	 			24
#define	HX8529_BT_NUM				0
#define	HX8529_X_RES				1200
#define	HX8529_Y_RES				1920
#define	HX8529_MAX_PT				10

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)
#define HX_TP_SYS_DIAG
#define HX_TP_SYS_RESET
#define HX_TP_SYS_REGISTER
#define HX_TP_SYS_DEBUG
#define HX_TP_SYS_FLASH_DUMP
#define HX_TP_SYS_SELF_TEST
#ifdef HX_TP_SYS_SELF_TEST
#define HIMAX_GOLDEN_SELF_TEST
#endif

#endif
//===========Himax Option function============
//#define HX_AUTO_UPDATE_FW

//#ifdef HX_AUTO_UPDATE_FW
#define HX_UPDATE_WITH_BIN_BUILDIN
//#endif

#define HX_ESD_WORKAROUND
//#define HX_CHIP_STATUS_MONITOR		//for ESD 2nd solution,default off


#define HX_85XX_A_SERIES_PWON	1
#define HX_85XX_B_SERIES_PWON	2
#define HX_85XX_C_SERIES_PWON	3
#define HX_85XX_D_SERIES_PWON	4
#define HX_85XX_E_SERIES_PWON	5
#define HX_85XX_ES_SERIES_PWON	6
#define HX_85XX_F_SERIES_PWON	7

#define HX_TP_BIN_CHECKSUM_SW	1
#define HX_TP_BIN_CHECKSUM_HW	2
#define HX_TP_BIN_CHECKSUM_CRC	3

#define HX_KEY_MAX_COUNT		4
#define DEFAULT_RETRY_CNT		3
#define MAX_RETRY_CNT			10

#define HX_VKEY_0   KEY_BACK
#define HX_VKEY_1   KEY_HOME
#define HX_VKEY_2   KEY_RESERVED
#define HX_VKEY_3   KEY_RESERVED
#define HX_KEY_ARRAY    {HX_VKEY_0, HX_VKEY_1, HX_VKEY_2, HX_VKEY_3}

#define SHIFTBITS 5
#define FLASH_SIZE 65536

#define FW_UPDATE_BOOT	0
#define FW_UPDATE_SD	1

#ifdef CONFIG_HUAWEI_DSM

#define	SUSPEND_REJECT	0
#define	SUSPEND_IN	0
#define	RESUME_REJECT	0
#define	RESUME_IN	0
#define	FW_NO_EXIST	0
#define	FW_NO_NEED_TO_UPDATE	0
#define	FW_NEED_TO_UPDATE	1
#define	FW_UPDATE_SUCC	1
#define	FW_UPDATE_FAIL	0
#define	SELF_TEST_PASS	0
#define	SELF_TEST_FAIL	1
#define	FACTORY_RUNNING		-1
#define	FACTORY_CANT_RUN	-1
#define	I2C_ACCESS_FAIL	1
#define	I2C_WORK_ERR	-1
#define	I2C_SEND_FAIL	2
#define	DT_GET_FAIL		0
#define	DT_READ_FAIL	-1
#if defined(HX_CHIP_STATUS_MONITOR)
#define	HANDSHAKE_TIMEOUT_IN_SUSPEND	0
#define	HANDSHAKE_TIMEOUT_IN_RESUME	0
#define	HANDSHAKE_TIMEOUT	1
#endif
#define	ALLOC_FAIL	-1
#define	NO_RESET_OUT	1
#define	LOAD_SENSORCONFIG_OK	1
#define	LOAD_SENSORCONFIG_RUN_FAIL	1
#define	CAL_CHECKSUM_PASS	1
#define	CAL_CHECKSUM_FAIL	0
#define	CAL_CHECKSUM_RUN_FAIL	1
#define	MUTUAL_ALLOC_FAIL	1
#define	IC_PACK_CHECK_SUCC	true
#define IC_PACK_CHECK_FAIL  false

#define	INFO_FAIL	-1
#define	R36H_FAIL	-1
#define	TP_DCLIENT_NO_EXIST	-1
#define	BUFFER_BUSY	-1
#define VCCA_DELAY_TIME 80
#define VCCD_DELAY_TIME 25
#define RESET_LOW_TIME 20
#define RESET_HIGH_TIME 20
#define RAWDATA_TIME_OUT 20

#define CMDLINE_PANEL_CHOPIN_BLACK_NAME  "auo_nt51021b_8p0_1200p_video"
#define CMDLINE_PANEL_CHOPIN_WHITE_NAME  "auo_nt51021_8p0_1200p_video"
#define HX_IC_RAWDATA_PROC_PRINTF	"hx_ic_rawdata_proc_printf"
#define WHITE	0xE1
#define BLACK	0xD2
#define TP_COLOR_SIZE   15
#define ESD_EVENT_ALL_ZERO 0x00
#define ESD_EVENT_ALL_ED 0xED
#define ESD_ALL_AERO_BAK_VALUE 1
#define ESD_ALL_ED_BAK_VALUE 2
#define R36_CHECK_ENABLE_FLAG 1

enum hmx_tp_error_state {
	FWU_GET_SYSINFO_FAIL = 0,
	FWU_GENERATE_FW_NAME_FAIL,
	FWU_REQUEST_FW_FAIL,
	FWU_FW_CRC_ERROR,
	FWU_FW_WRITE_ERROR,
	TP_WOKR_READ_DATA_ERROR = 5,
	TP_WOKR_CHECKSUM_INFO_ERROR,
	TP_WOKR_CHECKSUM_ALL_ERROR,
	TP_WOKR_HEAD_ERROR,
	TS_UPDATE_STATE_UNDEFINE = 255,
};
struct hmx_dsm_info {
	int irq_gpio;
	int rst_gpio;
	int I2C_status;
	int UPDATE_status;
	int WORK_status;
	int ESD_status;
	int rawdata_status;
};
extern struct hmx_dsm_info hmx_tp_dsm_info;
extern struct dsm_client *hmx_tp_dclient;
#endif
struct himax_virtual_key {
	int index;
	int keycode;
	int x_range_min;
	int x_range_max;
	int y_range_min;
	int y_range_max;
};

struct himax_ts_data {
	bool suspended;
	atomic_t suspend_mode;
	atomic_t irq_complete;
	uint8_t x_channel;
	uint8_t y_channel;
	uint8_t useScreenRes;
	uint8_t diag_command;
	uint8_t vendor_fw_ver_H;
	uint8_t vendor_fw_ver_L;
	uint8_t vendor_config_ver;
	uint8_t vendor_sensor_id;

	uint8_t protocol_type;
	uint8_t first_pressed;
	uint8_t coord_data_size;
	uint8_t area_data_size;
	uint8_t raw_data_frame_size;
	uint8_t raw_data_nframes;
	uint8_t nFinger_support;
	uint8_t irq_enabled;

	uint16_t finger_pressed;
	uint16_t last_slot;
	uint16_t pre_finger_mask;

	uint32_t debug_log_level;
	uint32_t widthFactor;
	uint32_t heightFactor;
	uint32_t tw_x_min;
	uint32_t tw_x_max;
	uint32_t tw_y_min;
	uint32_t tw_y_max;
	uint32_t pl_x_min;
	uint32_t pl_x_max;
	uint32_t pl_y_min;
	uint32_t pl_y_max;

	int (*power)(int on);
	int pre_finger_data[10][2];

	int power_support;
	int power_type_sel;

	struct device *dev;
	struct input_dev *input_dev;
	struct himax_i2c_platform_data *pdata;
	struct himax_virtual_key *button;
	struct wake_lock ts_flash_wake_lock;
	struct regulator *vddd;
	struct regulator *vdda;

#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
#ifdef HX_CHIP_STATUS_MONITOR
	struct workqueue_struct *himax_chip_monitor_wq;
	struct delayed_work himax_chip_monitor;
#endif

#ifdef HX_UPDATE_WITH_BIN_BUILDIN
	struct workqueue_struct *himax_update_firmware;
	struct delayed_work himax_update_work;
#endif
#ifdef HX_TP_SYS_FLASH_DUMP
	struct workqueue_struct 			*flash_wq;
	struct work_struct 					flash_work;
#endif
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
	struct ts_kit_device_data *tskit_himax_data;
	struct platform_device *ts_dev;
//===raw data
	struct workqueue_struct *hx_raw_wq;
	struct work_struct hx_raw_work;
	struct hrtimer timer;
//========
	int rst_gpio;
	char project_id[MAX_STR_LEN];
	char module_vendor[MAX_STR_LEN];
	uint8_t support_get_tp_color;/*for tp color */

	bool firmware_updating;
	uint32_t p2p_test_sel;
	uint32_t threshold_associated_with_projectid;
	uint8_t touch_switch_scene_reg;
	unsigned int support_retry_self_test;
};

struct himax_touching_data
{
	int x;
	int y;
	int w;
	uint32_t loop_i;
	uint16_t old_finger;
	uint16_t finger_pressed;
};

#define HX_CMD_NOP						0x00
#define HX_CMD_SETMICROOFF				0x35
#define HX_CMD_SETROMRDY				0x36
#define HX_CMD_TSSLPIN					0x80
#define HX_CMD_TSSLPOUT 				0x81
#define HX_CMD_TSSOFF					0x82
#define HX_CMD_TSSON					0x83
#define HX_CMD_ROE						0x85
#define HX_CMD_RAE						0x86
#define HX_CMD_RLE						0x87
#define HX_CMD_CLRES					0x88
#define HX_CMD_TSSWRESET				0x9E
#define HX_CMD_SETDEEPSTB				0xD7
#define HX_CMD_SET_CACHE_FUN			0xDD
#define HX_CMD_SETIDLE					0xF2
#define HX_CMD_SETIDLEDELAY 			0xF3
#define HX_CMD_SELFTEST_BUFFER			0x8D
#define HX_CMD_MANUALMODE				0x42
#define HX_CMD_FLASH_ENABLE 			0x43
#define HX_CMD_FLASH_SET_ADDRESS		0x44
#define HX_CMD_FLASH_WRITE_REGISTER		0x45
#define HX_CMD_FLASH_SET_COMMAND		0x47
#define HX_CMD_FLASH_WRITE_BUFFER		0x48
#define HX_CMD_FLASH_PAGE_ERASE 		0x4D
#define HX_CMD_FLASH_SECTOR_ERASE		0x4E
#define HX_CMD_CB						0xCB
#define HX_CMD_EA						0xEA
#define HX_CMD_4A						0x4A
#define HX_CMD_4F						0x4F
#define HX_CMD_B9						0xB9
#define HX_CMD_76						0x76
#define HX_CMD_ADDR_RESULT				0X96
#define HX_CMD_ADDR_CRITERIA			0X98
#define HX_VER_FW_MAJ					0x33
#define HX_VER_FW_MIN					0x32
#define HX_VER_FW_CFG				 	0x39
#define HX_MAX_PRBUF_SIZE           	PIPE_BUF

enum input_protocol_type {
	PROTOCOL_TYPE_A	= 0x00,
	PROTOCOL_TYPE_B	= 0x01,
};

enum himax_event_id {
	HIMAX_EV_NO_EVENT,
	HIMAX_EV_TOUCHDOWN,
	HIMAX_EV_MOVE,
	HIMAX_EV_LIFTOFF,
};

#if defined(CONFIG_TOUCHSCREEN_HIMAX_DEBUG)

	//extern uint8_t HX_PROC_SEND_FLAG;
	extern int  touch_monitor_stop_flag;
	extern uint8_t diag_coor[128];// = {0xFF};

#ifdef HX_TP_SYS_DIAG
	extern uint8_t *getMutualBuffer(void);
	extern uint8_t *getSelfBuffer(void);
	extern uint8_t 	getDiagCommand(void);
	extern void setDiagCommand(uint8_t cmd);
	extern uint8_t 	getXChannel(void);
	extern uint8_t 	getYChannel(void);

	extern void 	setMutualBuffer(void);
	extern void 	freeMutualBuffer(void);
	extern void 	setXChannel(uint8_t x);
	extern void 	setYChannel(uint8_t y);
#endif


#ifdef HX_TP_SYS_FLASH_DUMP
	extern bool  getFlashDumpGoing(void);
	extern void setFlashBuffer(void);
	extern void freeFlashBuffer(void);
	extern void setSysOperation(uint8_t operation);
	extern void (*himax_ts_flash_work_func)(struct work_struct *work);
#endif

#endif

#ifdef HX_ESD_WORKAROUND
	extern uint8_t	ESD_R36_FAIL;
	extern uint8_t	g_check_r36h_flag;
#endif

void himax_HW_reset(bool loadconfig,bool int_off);

#ifdef HX_CHIP_STATUS_MONITOR
static int HX_CHIP_POLLING_COUNT = 0;

static int HX_POLLING_TIMES	=2;//ex:5(timer)x2(times)=10sec(polling time)
extern int HX_ON_HAND_SHAKING;
#endif

extern int irq_enable_count;

extern int		HX_RX_NUM;
extern int		HX_TX_NUM;
extern int		HX_BT_NUM;
extern int		HX_X_RES;
extern int		HX_Y_RES;
extern int		HX_MAX_PT;
extern bool		HX_XY_REVERSE;


extern unsigned int 	FW_VER_MAJ_FLASH_ADDR;
extern unsigned int 	FW_VER_MAJ_FLASH_LENG;
extern unsigned int 	FW_VER_MIN_FLASH_ADDR;
extern unsigned int 	FW_VER_MIN_FLASH_LENG;
extern unsigned int 	FW_CFG_VER_FLASH_ADDR;
extern unsigned int 	CFG_VER_MAJ_FLASH_ADDR;
extern unsigned int 	CFG_VER_MAJ_FLASH_LENG;
extern unsigned int 	CFG_VER_MIN_FLASH_ADDR;
extern unsigned int 	CFG_VER_MIN_FLASH_LENG;

extern uint8_t	IC_STATUS_CHECK;
extern unsigned char	IC_CHECKSUM;
extern unsigned char	IC_TYPE;
extern int self_test_inter_flag;
extern atomic_t hmx_mmi_test_status;

//for update test
extern unsigned int   HX_UPDATE_FLAG;
extern unsigned int   HX_RESET_COUNT;
extern unsigned  int  HX_ESD_RESET_COUNT ;
extern uint8_t HW_RESET_ACTIVATE;
//for update test

//define in debug.c
extern int i_update_FW(void);
extern int himax_touch_sysfs_init(void);
extern void himax_touch_sysfs_deinit(void);
extern int (*himax_lock_flash)(int enable);
extern int PowerOnSeq(struct himax_ts_data *ts);
extern int himax_power_on_initCMD(void);

//need to check in probe
extern int himax_hand_shaking(void);
extern int himax_loadSensorConfig(void); ////need after reset
extern int himax_input_register(struct himax_ts_data *ts);
extern int (*fts_ctpm_fw_upgrade_with_fs)(const unsigned char *fw, int len, bool change_iref);
extern uint8_t (*himax_calculateChecksum)(bool change_iref);    //need use FW_VER_MAJ_FLASH_ADDR
extern irqreturn_t himax_ts_thread(int irq, void *ptr);
//used in factory test
extern int himax_chip_self_test(void);
extern void himax_get_rawdata_work(void);
#ifdef CONFIG_HUAWEI_DSM
extern int hmx_tp_report_dsm_err(int type, int err_numb);
#endif
int himax_check_update_firmware_flag(void);
extern int himax_fw_update_boot(char *file_name);
extern int himax_fw_update_sd(void);
#endif
