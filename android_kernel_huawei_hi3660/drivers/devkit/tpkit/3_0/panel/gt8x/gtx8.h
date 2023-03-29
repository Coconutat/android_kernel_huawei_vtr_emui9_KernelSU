#ifndef _GTX8_H_
#define _GTX8_H_

#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/ioctl.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/vmalloc.h>
#include <linux/firmware.h>
#include <linux/completion.h>
#include <linux/workqueue.h>
#include <linux/miscdevice.h>
#include <linux/platform_device.h>

#ifdef CONFIG_OF
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#endif
#include "../../huawei_ts_kit.h"

#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#define GTP_DRIVER_VERSION			"v1.0<2018/05/17>"

#define LCD_PANEL_INFO_MAX_LEN			128
#define LCD_PANEL_TYPE_DEVICE_NODE_NAME		"huawei,lcd_panel_type"

#define GTX8_OF_NAME				"gtx8"
#define GTX8_9886_CHIP_ID			"9886"
#define GTX8_FW_SD_NAME				"ts/touch_screen_firmware.img"
#define GTX8_CONFIG_FW_SD_NAME 			"ts/gtx8_cfg.bin"
#define GTX8_DEFAULT_CFG_NAME			"goodix_config.cfg"
#define TS_CFG_BIN_HEAD_LEN (sizeof(struct goodix_cfg_bin_head) + 6)
#define TS_PKG_CONST_INFO_LEN  (sizeof(struct goodix_cfg_pkg_const_info))
#define TS_PKG_REG_INFO_LEN (sizeof(struct goodix_cfg_pkg_reg_info))
#define TS_PKG_HEAD_LEN (TS_PKG_CONST_INFO_LEN + TS_PKG_REG_INFO_LEN)
#define GTX8_PRODUCT_ID_LEN			4
#define  GOODIX_VENDER_NAME   "goodix"
#define SWITCH_OFF      			0
#define SWITCH_ON               		1
#define GOODIX_CFG_MAX_SIZE			2048
#define GTX8_DOZE_ENABLE_DATA			0xCC
#define GTX8_DOZE_DISABLE_DATA			0xAA
#define GTX8_DOZE_CLOSE_OK_DATA			0xBB
#define GTX8_RETRY_NUM_3			3
#define GTX8_RETRY_NUM_10			10
#define GTX8_RETRY_NUM_20			20
#define GTX8_CFG_HEAD_LEN			4
#define GTX8_CFG_MAX_LEN			1024

#define GTX8_POWER_BY_LDO 			1
#define GTX8_SMALL_OR_LARGE_CFG_LEN 		32
#define GTX8_CFG_BAG_NUM_INDEX 			2
#define GTX8_CFG_BAG_START_INDEX		4
#define GTX8_FW_NAME_LEN			64
#define GT_RAWDATA_CSV_VERTICAL_SCREEN		1


#define GTX8_EXIST				1
#define GTX8_NOT_EXIST				0

#define GTX8_ABS_MAX_VALUE 			255
#define GTX8_MAX_PEN_PRESSURE			4096
#define GTP_MAX_TOUCH   			TS_MAX_FINGER
#define BYTES_PER_COORD 			8
#define GTX8_EDGE_DATA_SIZE			20

#define GTX8_RESET_SLEEP_TIME			80
#define GTX8_RESET_PIN_D_U_TIME			150

#define GTX8_HOTKNOT_EVENT			0x10
#define GTX8_GESTURE_EVENT			0x20
#define GTX8_REQUEST_EVENT			0x40
#define GTX8_TOUCH_EVENT			0x80

#define REQUEST_HANDLED				0x00
#define REQUEST_CONFIG				0x01
#define REQUEST_BAKREF				0x02
#define REQUEST_RESET				0x03
#define REQUEST_MAINCLK				0x04
#define REQUEST_IDLE				0x05

#define SLEEP_MODE				0x01
#define GESTURE_MODE				0x02

/*GTX8 CMD*/
#define GTX8_CMD_NORMAL				0x00
#define GTX8_CMD_RAWDATA			0x01
#define GTX8_CMD_SLEEP				0x05
#define GTX8_CMD_CHARGER_ON			0x06
#define GTX8_CMD_CHARGER_OFF		0x07
#define GTX8_CMD_IDLE_PERMIT		0x11
#define GTP_CMD_STYLUS_LOW_FREQENCY	0x12
#define GTP_CMD_STYLUS_NORMAL		0x13
#define GTP_CMD_STYLUS_SWITCH_OFF	0x14
#define GTX8_CMD_STYLUS_RESPONSE_MODE 0x16
#define GTX8_CMD_GESTURE			0x08
#define GTX8_CMD_START_SEND_CFG			0x80
#define GTX8_CMD_END_SEND_CFG			0x83
#define GTX8_CMD_SEND_SMALL_CFG			0x81
#define GTX8_CMD_READ_CFG			0x86
#define GTX8_CMD_CFG_READY			0x85
#define GTX8_CMD_COMPLETED			0xFF

/* Register define */
#define GTP_REG_DOZE_CTRL			0x30F0
#define GTP_REG_ESD_TICK_W			0x30F3
#define GTP_REG_DOZE_STAT			0x3100
#define GTP_REG_ESD_TICK_R			0x3103
#define GTP_REG_SENSOR_NUM			0x5473
#define GTP_REG_DRIVER_GROUP_A_NUM		0x5477
#define GTP_REG_DRIVER_GROUP_B_NUM		0x5478
#define GTP_RAWDATA_BASE_ADDR			0x8FA0
#define GTP_DIFFDATA_BASE_ADDR			0x9D20
#define GTP_REG_CFG_ADDR			0x6F78
#define GTP_REG_CMD				0x6F68
#define GTP_REG_COOR				0x4100
#define GTP_REG_GESTURE			0x4100
#define GTP_REG_PID				0x4535
#define GTP_REG_VID				0x453D
#define GTP_REG_SENSOR_ID			0x4541
#define GTP_REG_VERSION_BASE			0x452C
#define GTP_REG_FW_REQUEST			0x6F6D
#define GTP_REG_STATUS			0x4B64

#define WD_TRI_TIMES_5				5
#define GTP_REG_LEN_6			6
#define GTP_REG_LEN_30			30
#define GTP_PID_LEN				4
#define GTP_VID_LEN				4
#define GTP_VERSION_LEN				72
#define GTP_SENSOR_ID_MASK			0x0F

#define HW_REG_CPU_CTRL				0x2180
#define HW_REG_DSP_MCU_POWER			0x2010
#define HW_REG_RESET				0x2184
#define HW_REG_SCRAMBLE				0x2218
#define HW_REG_BANK_SELECT			0x2048
#define HW_REG_ACCESS_PATCH0			0x204D
#define HW_REG_EC_SRM_START			0x204F
#define HW_REG_CPU_RUN_FROM			0x4506 /* for nor_L is 0x4006 */
#define HW_REG_ISP_RUN_FLAG			0x6006
#define HW_REG_ISP_ADDR				0xC000
#define HW_REG_ISP_BUFFER			0x6100
#define HW_REG_SUBSYS_TYPE			0x6020
#define HW_REG_FLASH_FLAG			0x6022


/* cfg parse from bin */
#define CFG_BIN_SIZE_MIN 			279
#define BIN_CFG_START_LOCAL  			6
#define MODULE_NUM				22
#define CFG_NUM					23
#define CFG_INFO_BLOCK_BYTES 			8
#define CFG_HEAD_BYTES 				32

/* GTX8 cfg name */
#define GTX8_NORMAL_CONFIG			"normal_config"
#define GTX8_TEST_CONFIG			"tptest_config"
#define GTX8_NORMAL_NOISE_CONFIG		"normal_noise_config"
#define GTX8_GLOVE_CONFIG			"glove_config"
#define GTX8_GLOVE_NOISE_CONFIG			"glove_noise_config"
#define GTX8_HOLSTER_CONFIG			"holster_config"
#define GTX8_HOLSTER_NOISE_CONFIG		"holster_noise_config"
#define GTX8_NOISE_TEST_CONFIG			"tpnoise_test_config"
#define GTX8_GAME_CONFIG			"game_config"
#define GTX8_GAME_NOISE_CONFIG			"game_noise_config"

#define NORMAL_PKG				0x01
#define NORMAL_NOISE_PKG			0x02
#define GLOVE_PKG				0x03
#define GLOVE_NOISE_PKG				0x04
#define HOLSTER_PKG				0x05
#define HOLSTER_NOISE_PKG			0x06
#define TEST_PKG				0x07
#define NOISE_TEST_PKG				0x08

#define GTX8_NEED_SLEEP				1
#define GTX8_NOT_NEED_SLEEP			0

/* send config delay time /ms */
#define SEND_CFG_FLASH					150
#define SEND_CFG_RAM					60
#define SEND_CMD_AFTER_CONFIG_DELAY		50
#define SEND_CMD_AFTER_CMD_DELAY		20

/* fw update */
#define FW_SUBSYS_MAX_NUM			28
#define FW_UPDATE_0_PERCENT			0
#define FW_UPDATE_10_PERCENT			10
#define FW_UPDATE_20_PERCENT			20
#define FW_UPDATE_100_PERCENT			100
#define FW_UPDATE_RETRY				2
#define FW_SUBSYS_INFO_OFFSET			32
#define FW_HEADER_SIZE				256
#define FW_SUBSYS_INFO_SIZE			8
#define ISP_MAX_BUFFERSIZE			(1024 * 4)

/* roi data*/
//TODO:remain to confirm
#define ROI_STA_REG				0x5EF0
#define ROI_HEAD_LEN				4
#define ROI_DATA_REG				(ROI_STA_REG + ROI_HEAD_LEN)
#define ROI_READY_MASK				0x10
#define ROI_TRACKID_MASK			0x0f
#define GTX8_BIT_AND_0x0F			0x0f
#define GTX8_ROI_SRC_STATUS_INDEX		2

/* Regiter for short  test*/
#define SHORT_STATUS_REG			0x5095
#define WATCH_DOG_TIMER_REG			0x20B0

#define TXRX_THRESHOLD_REG			0x8408
#define GNDVDD_THRESHOLD_REG			0x840A
#define ADC_DUMP_NUM_REG			0x840C

#define DIFFCODE_SHORT_THRESHOLD		0x880A

#define DRIVER_CH0_REG_9PT			0x8101
#define	SENSE_CH0_REG_9PT			0x80E1
#define DRIVER_CH0_REG_9P			0x80fc
#define	SENSE_CH0_REG_9P			0x80dc

#define DRV_CONFIG_REG				0x880E
#define SENSE_CONFIG_REG			0x882E
#define DRVSEN_CHECKSUM_REG			0x884E
#define CONFIG_REFRESH_REG			0x813E
#define	ADC_RDAD_VALUE				0x96
#define GNDAVDD_SHORT_VALUE			16
#define ADC_DUMP_NUM				200
#define	DIFFCODE_SHORT_VALUE			0x14
#define SHORT_CAL_SIZE(a)			(4 + (a) * 2 + 2)

/* Regiter for rawdata test*/
#define GTP_RAWDATA_ADDR_9886			0x8FA0
#define GTP_NOISEDATA_ADDR_9886			0x9D20
#define GTP_BASEDATA_ADDR_9886			0xA980
#define GTP_SELF_RAWDATA_ADDR_9886		0x4C0C
#define GTP_SELF_NOISEDATA_ADDR_9886		0x4CA4

#define GTP_RAWDATA_ADDR_6861			0x9078
#define GTP_NOISEDATA_ADDR_6861			0x9B92
#define GTP_BASEDATA_ADDR_6861			0xB0DE

#define GTP_RAWDATA_ADDR_6862			0x9078
#define GTP_NOISEDATA_ADDR_6862			0x9B92
#define GTP_BASEDATA_ADDR_6862			0xB0DE

/* may used only in test.c */
#define SHORT_TESTEND_REG			0x8400
#define TEST_RESULT_REG				0x8401
#define TX_SHORT_NUM				0x8402

#define DIFF_CODE_REG				0xA97A
#define DRV_SELF_CODE_REG			0xA8E0
#define TX_SHORT_NUM_REG			0x8802

#define MAX_DRV_NUM				40
#define MAX_SEN_NUM				36

#define MAX_DRV_NUM_9886			40
#define MAX_SEN_NUM_9886			36

#define MAX_DRV_NUM_6861			47
#define MAX_SEN_NUM_6861			36

#define MAX_DRV_NUM_6862			47
#define MAX_SEN_NUM_6862			29

/*  end  */

/* easy wakeup */
#define GTX8_PEN_WAKEUP_ENTER_MEMO_EVENT_TYPE 	  0x80
#define GTX8_PEN_WAKEUP_ONLI_SCREEN_ON_EVENT_TYPE 	  0x81
#define GTX8_GESTURE_EVENT_FLAG_BIT     (1<<5)

#define PID_DATA_MAX_LEN		8
#define VID_DATA_MAX_LEN		8
#define GTX8_POWER_CONTRL_BY_SELF				1


/* errno define */
#define EBUS					1000
#define ETIMEOUT				1001
#define ECHKSUM					1002
#define EMEMCMP					1003

#define IC_TYPE_9PT				0
#define IC_TYPE_9P				1
#define IC_TYPE_9886				2
#define IC_TYPE_6861				3
#define IC_TYPE_6862				4

#define TS_PEN_BUTTON_NONE      0
#define TS_PEN_BUTTON_RELEASE   (1 << 5)
#define TS_PEN_BUTTON_PRESS     (1 << 6)
#define TS_PEN_KEY_NONE         0

#define TS_SWITCH_PEN_RESPON_FAST	    3
#define TS_SWITCH_PEN_RESPON_NORMAL	4

#define TS_SWITCH_PEN_RESPON_FAST_CMD_DATA	    0
#define TS_SWITCH_PEN_RESPON_NORMAL_CMD_DATA	1


#define  GTX8_AFTER_REST_DELAY  100

#define getU32(a) ((u32)getUint((u8 *)(a), 4))
#define getU16(a) ((u16)getUint((u8 *)(a), 2))

#if defined (CONFIG_HUAWEI_DSM)
extern struct dsm_client *ts_dclient;
#endif


u32 getUint(u8 *buffer, int len);

enum gtx8_chip_id {
	GT_DEFALT_ID = 0,
	GT_9886_ID = 54,
};

/*
 * gtx8_ts_feature - special touch feature
 * @TS_FEATURE_NONE: no special touch feature
 * @TS_FEATURE_GLOVE: glove feature
 * @TS_FEATURE_HOLSTER: holster feature
 */
enum gtx8_ts_feature {
	TS_FEATURE_NONE = 0,
	TS_FEATURE_GLOVE,
	TS_FEATURE_HOLSTER,
	TS_FEATURE_GAME,
	TS_FEATURE_CHARGER,
};

/*
 * gtx8_ts_roi - finger sense data structure
 * @enabled: enable/disable finger sense feature
 * @data_ready: flag to indicate data ready state
 * @roi_rows: rows of roi data
 * @roi_clos: columes of roi data
 * @mutex: mutex
 * @rawdata: rawdata buffer
 */
struct gtx8_ts_roi {
	bool enabled;
	bool data_ready;
	int track_id;
	unsigned int roi_rows;
	unsigned int roi_cols;
	struct mutex mutex;
	u16 *rawdata;
};

struct roi_matrix {
	short *data;
	unsigned int row;
	unsigned int col;
};

/**
 * fw_subsys_info - subsytem firmware infomation
 * @type: sybsystem type
 * @size: firmware size
 * @flash_addr: flash address
 * @data: firmware data
 */
struct fw_subsys_info {
	u8 type;
	u32 size;
	u16 flash_addr;
	const u8 *data;
};

#pragma pack(1)
/**
 * firmware_info
 * @size: fw total length
 * @checksum: checksum of fw
 * @hw_pid: mask pid string
 * @hw_pid: mask vid code
 * @fw_pid: fw pid string
 * @fw_vid: fw vid code
 * @subsys_num: number of fw subsystem
 * @chip_type: chip type
 * @protocol_ver: firmware packing
 *   protocol version
 * @subsys: sybsystem info
 */
struct firmware_info {
	u32 size;
	u16 checksum;
	u8 hw_pid[6];
	u8 hw_vid[3];
	u8 fw_pid[8];
	u8 fw_vid[4];
	u8 subsys_num;
	u8 chip_type;
	u8 protocol_ver;
	u8 reserved[2];
	struct fw_subsys_info subsys[FW_SUBSYS_MAX_NUM];
};

#pragma pack()

/**
 * firmware_data - firmware data structure
 * @fw_info: firmware infomation
 * @firmware: firmware data structure
 */
struct firmware_data {
	struct firmware_info fw_info;
	const struct firmware *firmware;
};

enum update_status {
	UPSTA_NOTWORK = 0,
	UPSTA_PREPARING,
	UPSTA_UPDATING,
	UPSTA_ABORT,
	UPSTA_SUCCESS,
	UPSTA_FAILED
};

/**
 * fw_update_ctrl - sturcture used to control the
 *  firmware update process
 * @status: update status
 * @progress: indicate the progress of update
 * @fw_data: firmware data
 * @ts_dev: touch device
 * @fw_name: firmware name
 * @attr_fwimage: sysfs bin attrs, for storing fw image
 * @fw_from_sysfs: whether the firmware image is loadind
 *		from sysfs
 */
struct fw_update_ctrl {
	enum update_status  status;
	unsigned int progress;
	bool force_update;

	struct firmware_data fw_data;
	char fw_name[GTX8_FW_NAME_LEN + 1];
	struct bin_attribute attr_fwimage;
	bool fw_from_sysfs;
};

/*
 * struct gtx8_ts_version - firmware version
 * @valid: whether these infomation is valid
 * @pid: product id string
 * @vid: firmware version code
 * @cid: customer id code
 * @sensor_id: sendor id
 */
struct gtx8_ts_version {
	bool valid;
	char pid[PID_DATA_MAX_LEN];
	char vid[VID_DATA_MAX_LEN];
	u8 cid;
	u8 sensor_id;
};

/*
 * struct gtx8_ts_config - chip config data
 * @initialized: whether intialized
 * @name: name of this config
 * @lock: mutex
 * @reg_base: register base of config data
 * @length: bytes of the config
 * @delay: delay time after sending config
 * @data: config data buffer
 */
struct gtx8_ts_config {
	bool initialized;
	char name[MAX_STR_LEN + 1];
	struct mutex lock;
	unsigned int reg_base;
	unsigned int length;
	unsigned int delay; /*ms*/
	unsigned char data[GOODIX_CFG_MAX_SIZE];
};



struct gtx8_ts_regs {
	u16 cfg_send_flag;
	u16 version_base;
	u16 pid;
	u16 vid;
	u16 sensor_id;
	u16 fw_mask;
	u16 fw_status;
	u16 cfg_addr;
	u16 esd;
	u16 command;
	u16 coor;
	u16 gesture;
	u16 fw_request;
	u16 proximity;
	u8 version_len;
	u8  pid_len;
	u8  vid_len;
	u8  sensor_id_mask;
};

struct gtx8_ts_data;

struct gtx8_ts_ops {
	int (*i2c_write)(u16 addr, u8 *buffer, u32 len);//need to wakeup from doze
	int (*i2c_read)(u16 addr, u8 *buffer, u32 len);
	int (*write_trans)(u16 addr, u8 *buffer, u32 len);//no need to wakeup from doze
	int (*read_trans)(u16 addr, u8 *buffer, u32 len);
	int (*chip_reset)(void);
	int (*send_cmd)(u8 cmd, u8 data, u8 issleep);
	int (*send_cfg)(struct gtx8_ts_config *config);
	int (*read_cfg)(u8 *config_data, u32 config_len);
	int (*read_version)(struct gtx8_ts_version *version);
	int (*parse_cfg_data)(const struct firmware *cfg_bin,
				char *cfg_type, u8 *cfg, int *cfg_len, u8 sid);
	int (*feature_resume)(struct gtx8_ts_data *ts);
};

struct gtx8_ts_data {
	struct platform_device *pdev;
	struct regulator *vci;
	struct regulator *vddio;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_suspend;
	struct pinctrl_state *pins_gesture;
	struct ts_kit_device_data *dev_data;
	struct gtx8_ts_regs reg;
	struct gtx8_ts_ops ops;
	struct gtx8_ts_version hw_info;
	struct gtx8_ts_roi roi;
	struct mutex doze_mode_lock;
	struct gtx8_ts_config normal_cfg;
	struct gtx8_ts_config normal_noise_cfg;
	struct gtx8_ts_config glove_cfg;
	struct gtx8_ts_config glove_noise_cfg;
	struct gtx8_ts_config holster_cfg;
	struct gtx8_ts_config holster_noise_cfg;
	struct gtx8_ts_config test_cfg;
	struct gtx8_ts_config noise_test_cfg;
	struct gtx8_ts_config game_cfg;
	struct gtx8_ts_config game_noise_cfg;

	u32 flip_x;
	u32 flip_y;
	u32 noise_env;
	u32 tools_support;
	u32 gtx8_static_pid_support;
	volatile bool rawdiff_mode;
	u32 doze_mode_set_count;
	int tool_esd_disable;
	int max_x;
	int max_y;
	int fw_only_depend_on_lcd;/* 0 : fw depend on TP and others ,1 : fw only depend on lcd. */
	int gtx8_roi_fw_supported;
	int support_get_tp_color;
	int ic_type;
	u32 power_self_ctrl;/*0-LCD control, 1-tp controlled*/
	u32 vci_power_type;/*0 - gpio control  1 - ldo  2 - not used*/
	u32 vddio_power_type;/*0 - gpio control  1 - ldo  2 - not used*/
	u32 create_project_id_supported;
	u32 gtx8_edge_add;
	u32 gtx8_roi_data_add;
	u32 game_switch;/*1-enter game mode, 0-out of  game mode*/
	u32 easy_wakeup_supported;
	u32 config_flag;/* 0 - normal config; 1 - extern config*/
	char project_id[MAX_STR_LEN + 1];
	char chip_name[GTX8_PRODUCT_ID_LEN + 1];
	char lcd_panel_info[LCD_PANEL_INFO_MAX_LEN];
	char lcd_module_name[MAX_STR_LEN];
};

/*
 * checksum helper functions
 * checksum can be u8/le16/be16/le32/be32 format
 * NOTE: the caller shoule be responsible for the
 * legality of @data and @size parameters, so be
 * careful when call these functions.
 */
static inline u8 checksum_u8(u8 *data, u32 size)
{
	u8 checksum = 0;
	u32 i = 0;

	for (i = 0; i < size; i++)
		checksum += data[i];
	return checksum;
}

static inline u16 checksum_be16(u8 *data, u32 size)
{
	u16 checksum = 0;
	u32 i = 0;

	for (i = 0; i < size; i += 2)
		checksum += be16_to_cpup((__be16 *)(data + i));
	return checksum;
}

extern struct gtx8_ts_data *gtx8_ts;
extern struct fw_update_ctrl update_ctrl;

int gtx8_chip_reset(void);
int gtx8_update_firmware(void);
int gtx8_set_i2c_doze_mode(int enable);
 int gtx8_get_channel_num(u32 *sen_num, u32 *drv_num, u8 *cfg_data);

int gtx8_init_tool_node(void);
int gtx8_get_rawdata(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);
#endif
