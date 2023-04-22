#ifndef __ILITEK_TS_H__
#define __ILITEK_TS_H__
#include <linux/module.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/i2c.h>
#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <asm/uaccess.h>
#include <linux/version.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/proc_fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/ctype.h>
#include <linux/hrtimer.h>
#include <linux/pm_runtime.h>
//#include <misc/app_info.h>
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#include <linux/wakelock.h>
#endif
//#include <huawei_platform/touchscreen/hw_tp_common.h>
#include <linux/platform_device.h>

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#include "../../huawei_ts_kit.h"
#include "../../huawei_ts_kit_algo.h"
#define ILITEK_CHIP_NAME		"ilitek"
#define HUAWEI_TS_KIT		"huawei,ts_kit"
#define ILITEK_RESET_MODEL_NORMAL_DELAY		100
#define ILITEK_RESET_MODEL_CHECKFW_DELAY		10
#define ILITEK_INT_STATUS_HIGH		1
#define ILITEK_INT_STATUS_LOW		0
#define DERVER_VERSION_MAJOR 		4
#define DERVER_VERSION_MINOR 		0
#define CUSTOMER_ID 				0
#define MODULE_ID					0
#define PLATFORM_ID					0
#define PLATFORM_MODULE				0
#define ENGINEER_ID					0
#define ic2120						1
#define QUALCOM		1
#define PLAT      QUALCOM
#define ILITEK_DETECT_I2C_RETRY_TIMES			1
#define CLOCK_INTERRUPT
#define ILI_UPDATE_FW
#define TOUCH_PROTOCOL_B
#define REPORT_THREAD
#define HALL_CHECK
#define TOOL
#define SENSOR_TEST
#define REPORT_PRESSURE
//#define RESET_GPIO  12
//#define GESTURE
#define DEBUG_NETLINK
#ifdef DEBUG_NETLINK
#include <linux/kernel.h>
#include <linux/init.h>
#include <net/sock.h>
#include <net/netlink.h>
#include <linux/skbuff.h>
#include <linux/module.h>
#include <linux/netlink.h>
#include <linux/sched.h>
#include <net/sock.h>
#include <linux/proc_fs.h>
#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
#include <linux/ip.h>
#include <linux/tcp.h>
#include <linux/icmp.h>
#include <linux/udp.h>
#endif


#define ROI_DATA_LENGTH 98


#define TRANSMIT_ERROR					-3
#define UPGRADE_FAIL					-4
#define UPGRADE_OK							0
#define NOT_NEED_UPGRADE					1
#define FW_REQUEST_ERR					-1
#define SENSOR_TEST_SET_CDC_INITIAL	0xF1
#define SENSOR_TEST_TRCRQ_TRCST_TESET	0x20
#define SENSOR_TEST_COMMAND		0x00
#define SENSOR_TEST_TEAD_DATA_SELECT_CONTROL	0xF6
#define SENSOR_TEST_GET_CDC_RAW_DATA	0xF2
#define POLLINT_INT_TIMES	300
#define ILITEK_PROJECT_ID_LEN			11
#define FW_OK							0x80
#define AP_STARTADDR					0x00
#define AP_ENDADDR						0xDFFF
#define FW_UPGRADE_LEN				0xE000
#define UPGRADE_TRANSMIT_LEN		256
#define FW_VERSION1					0xD100
#define FW_VERSION2					0xD101
#define FW_VERSION3					0xD102

#define PRODUCT_ID_STARTADDR			0xE000
#define PRODUCT_ID_ENDADDR			0xE006

#define SECTOR_SIZE						0x1000
#define SECTOR_ENDADDR				0xD000

#define REG_LEN							4
#define REG_START_DATA				0x25

#define ENTER_ICE_MODE				0x181062
#define ENTER_ICE_MODE_NO_DATA		0X0

#define EXIT_ICE_MODE					0x1810621B

#define REG_FLASH_CMD					0x041000
#define REG_FLASH_CMD_DATA_ERASE		0x20
#define REG_FLASH_CMD_DATA_PROGRAMME		0x02
#define REG_FLASH_CMD_READ_FLASH_STATUS	0x5
#define REG_FLASH_CMD_WRITE_ENABLE			0x6
#define REG_FLASH_CMD_MEMORY_READ			0x3B
#define REG_FLASH_CMD_RELEASE_FROM_POWER_DOWN		0xab

#define REG_PGM_NUM					0x041004
#define REG_PGM_NUM_TRIGGER_KEY	0x66aa5500
#define REG_PGM_NUM_32				0x66aa551F
#define REG_PGM_NUM_256				0x66aa55FF

#define REG_READ_NUM					0x041009
#define REG_READ_NUM_1				0x0

#define REG_CHK_EN						0x04100B
#define REG_CHK_EN_PARTIAL_READ 		0x3

#define REG_TIMING_SET					0x04100d
#define REG_TIMING_SET_10MS			0x00

#define REG_CHK_FLAG					0x041011
#define FLASH_READ_DATA				0x041012
#define FLASH_STATUS					0x041013
#define REG_PGM_DATA					0x041020

#define WDTRLDT                            			0x5200C
#define WDTRLDT_CLOSE                            	0
#define WDTCNT1                     				0x52020
#define WDTCNT1_OPEN                     		1
#define WDTCNT1_CLOSE                     		0

#define CLOSE_10K_WDT1					0x42000
#define CLOSE_10K_WDT1_VALUE			0x0f154900

#define CLOSE_10K_WDT2					0x42014
#define CLOSE_10K_WDT2_VALUE			0x02

#define CLOSE_10K_WDT3					0x42000
#define CLOSE_10K_WDT3_VALUE			0x00000000


#define DATA_SHIFT_0					0x000000FF
#define DATA_SHIFT_8					0x0000FF00
#define DATA_SHIFT_16					0x00FF0000
#define DATA_SHIFT_24					0xFF000000

#define CONFIG_TOUCH_DRIVER_ERR_LOG_LEVEL (1)
#define CONFIG_TOUCH_DRIVER_DEFAULT_LOG_LEVEL (2)
#define CONFIG_TOUCH_DRIVER_INFO_LOG_LEVEL (3)
#define CONFIG_TOUCH_DRIVER_DEBUG_LOG_LEVEL (4)

extern u8 TOUCH_DRIVER_DEBUG_LOG_LEVEL;
#define DEBUG_LEVEL(level, fmt, arg...) do {\
	                                           if (level <= TOUCH_DRIVER_DEBUG_LOG_LEVEL)\
	                                               printk("%s LINE = %d: "fmt, "ILITEK", __LINE__, ##arg);\
                                        } while (0)

#define tp_log_err(fmt, arg...) DEBUG_LEVEL(CONFIG_TOUCH_DRIVER_ERR_LOG_LEVEL, fmt, ##arg)
#define tp_log_info(fmt, arg...) DEBUG_LEVEL(CONFIG_TOUCH_DRIVER_INFO_LOG_LEVEL, fmt, ##arg)
#define tp_log_debug(fmt, arg...) DEBUG_LEVEL(CONFIG_TOUCH_DRIVER_DEBUG_LOG_LEVEL, fmt, ##arg)

struct key_info {
	int id;
	int x;
	int y;
	int status;
	int flag;
};

struct touch_info {
	int id;
	int x;
	int y;
	int status;
	int flag;
};

struct sensor_test_max_min_ave {
	int allnodedata_max;
	int allnodedata_min;
	int allnodedata_ave;

	int Txdelta_max;
	int Txdelta_min;
	int Txdelta_ave;

	int Rxdelta_max;
	int Rxdelta_min;
	int Rxdelta_ave;

	int noise_max;
	int noise_min;
	int noise_ave;

	int open_max;
	int open_min;
	int open_ave;

	int short_max;
	int short_min;
	int short_ave;
};
struct i2c_data {
	struct input_dev *input_dev;
	struct i2c_client *client;
	struct task_struct *thread;
	struct device_node*  cnode;
	struct ts_device_ops* ops;
	struct ts_kit_device_data * ilitek_chip_data;
	struct platform_device *ilitek_dev;
    	atomic_t ts_interrupts;
	int max_x;
	int max_y;
	int min_x;
	int min_y;
	int max_tp;
	int max_btn;
	int x_ch;
	int x_allnode_ch;
	int y_ch;
	int stop_polling;
	struct semaphore wr_sem;
	int protocol_ver;
	int set_polling_mode;
	struct regulator *vdd;
	struct regulator *vcc_i2c;

	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;

	int irq;
	int irq_gpio;
	int vci_gpio;
	u32 irq_gpio_flags;
	int rst;
	u32 reset_gpio_flags;
	unsigned char firmware_ver[4];
	int reset_request_success;
	struct workqueue_struct *irq_work_queue;
	struct work_struct irq_work;
	struct timer_list timer;
	int report_status;
	int reset_gpio;
	int irq_status;
	struct mutex mutex;
	struct mutex roi_mutex;
	struct completion complete;
#if defined(CONFIG_FB)
	struct notifier_block fb_notif;
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	struct early_suspend early_suspend;
#endif
	int keyflag;
	int keycount;
	int key_xlen;
	int key_ylen;
	struct key_info keyinfo[10];
	int touch_flag ;
	struct touch_info touchinfo[10];
	bool firmware_updating;
	short * ilitek_rx_cap_max;
	short * ilitek_tx_cap_max;
	short ilitek_deltarawimage_max;
	short ilitek_open_threshold;
	short ilitek_short_threshold;
	short * ilitek_full_raw_max_cap;
	short * ilitek_full_raw_min_cap;
	struct sensor_test_max_min_ave sensor_test_data_result;
	unsigned char *product_id;

	unsigned char ilitek_roi_data[ROI_DATA_LENGTH + 4];
	unsigned char ilitek_roi_data_send[ROI_DATA_LENGTH + 4];
	u8 ilitek_roi_enabled;

	u8 glove_status;
	u8 hall_status;
	int hall_x0;
	int hall_y0;
	int hall_x1;
	int hall_y1;
	bool suspend;
	int function_ctrl;
	bool force_upgrade;

	bool apk_use;
	bool sensor_testing;
};

#define X_CHANNEL_NUM 16
#define Y_CHANNEL_NUM 28

#define I2C_WRITE_ONE_LENGTH_DATA 1
#define I2C_WRITE_TWO_LENGTH_DATA 2
#define I2C_WRITE_THREE_LENGTH_DATA 3

#define I2C_READ_ZERO_LENGTH_DATA 0
#define I2C_READ_ONE_LENGTH_DATA 1
#define I2C_READ_TWO_LENGTH_DATA 2
#define I2C_READ_THREE_LENGTH_DATA 3

#define ILITEK_SENSOR_TEST_DATA_MIN		0
#define ILITEK_SENSOR_TEST_DATA_MAX		4096

#define TX 28
#define RX 16
#define VIRTUAL_FUN_1	1
#define VIRTUAL_FUN_2	2
#define VIRTUAL_FUN_3	3
#define VIRTUAL_FUN		VIRTUAL_FUN_1
#define BTN_DELAY_TIME	500

#define TOUCH_POINT    0x80
#define TOUCH_KEY      0xC0
#define RELEASE_KEY    0x40
#define RELEASE_POINT    0x00
#define DRIVER_VERSION "aimvF"

#define KEYPAD01_X1	0
#define KEYPAD01_X2	1000
#define KEYPAD02_X1	1000
#define KEYPAD02_X2	2000
#define KEYPAD03_X1	2000
#define KEYPAD03_X2	3000
#define KEYPAD04_X1	3000
#define KEYPAD04_X2	3968
#define KEYPAD_Y	2100
#define ILITEK_VENDOR_COMP_NAME_LEN	32
#define ILITEK_ESD_CHECK_DATA			0xAA
#define ILITEK_ESD_CHECK_DATA_END			0xFF

#define ILITEK_TS_NO_INTERRUPTS			0
#define ILITEK_TS_HAVE_INTERRUPTS			1
#define ILITEK_TP_CMD_READ_DATA_RETRY_TIMES		3
#define ILITEK_WRITE_READ_NOT_DELAY	0
#define ILITEK_WRITE_READ_DELAY	10
#define ILITEK_I2C_RETRY_COUNT			3
#define ILITEK_I2C_DRIVER_NAME			"ilitek_i2c"
#define ILITEK_FILE_DRIVER_NAME		"ilitek_file"
#define ILITEK_FW_NAME_LEN				64
#define ILITEK_TP_CMD_GLOVE			0x06
#define ILITEK_TP_CMD_HALL			0x0C
#define ILITEK_TP_CMD_FINGERSENSE			0x0F
#define ILITEK_TP_DATA_RECEIVE_LEN_MUL2		2

#define ILITEK_I2C_TRANSFER_MAX_LEN		32
#define ILITEK_I2C_SENSOR_TEST_PACKET_HEAD_LEN	2
#define ILITEK_PRODUCT_ID_LEN					7
#define ILITEK_TP_CMD_READ_DATA			    0x10
#define ILITEK_TP_CMD_REPORT_DATA_LEN			    53
#define ILITEK_TP_CMD_READ_SUB_DATA		    0x11
#define ILITEK_TP_CMD_REPORT_STATUS		    0x13
#define ILITEK_TP_CMD_GET_RESOLUTION		0x20
#define ILITEK_TP_CMD_GET_KEY_INFORMATION	0x22
#define ILITEK_TP_CMD_SLEEP                 0x02
#define ILITEK_TP_CMD_GET_FIRMWARE_VERSION	0x21
#define ILITEK_TP_CMD_GET_PROTOCOL_VERSION	0x22
#define ILITEK_TP_CMD_SOFTRESET				0x60
#define	ILITEK_TP_CMD_CALIBRATION			0xCC
#define	ILITEK_TP_CMD_CALIBRATION_STATUS	0xCD
#define ILITEK_TP_CMD_ERASE_BACKGROUND		0xCE
#define ILITEK_TP_CMD_READ_ROI_DATA			    0x0E

#define ILITEK_TP_CMD_SENSOR_TEST			0xF1
#define ILITEK_TP_CMD_READ_DATA_CONTROL		0xF6

#define ILITEK_TP_CMD_READ_DATA_CONTROL_READ_DATA		0xF2

#define ILITEK_TP_SIGNAL_DATA_BASE				4096
#define ILITEK_TP_CMD_SENSOR_TEST_PARM0			0x0
#define ILITEK_TP_CMD_SENSOR_TEST_P2P			0x03
#define ILITEK_TP_CMD_SENSOR_TEST_ALLNODE			0x05
#define ILITEK_TP_CMD_SENSOR_TEST_SHORT			0x04
#define ILITEK_TP_CMD_SENSOR_TEST_OPEN			0x06
#define CHANGE_REPORT_RATE_RESULT_SUCCESS	0xAA
#define CHANGE_REPORT_RATE_RESULT_60HZ_FAIL	0xA1
#define CHANGE_REPORT_RATE_RESULT_120HZ_FAIL	0xA2
#define SENSOR_TEST_FAIL	-1
#define CHANGE_REPORT_RATE_SUCCESS		0
#define CHANGE_REPORT_RATE_60HZ_FAIL	-1
#define CHANGE_REPORT_RATE_120HZ_FAIL	-2
#define CHANGE_REPORT_RATE_FAIL			-3

#define ILITEK_TP_CMD_GESTURE			0x0A
#define ILITEK_TP_CMD_SENSE			0x01
#define FUN_ENABLE						0x1
#define FUN_DISABLE						0x0
#define ILITEK_TP_CMD_TEST_CMD						0xF0

#define ILITEK_MAX_CAP_LIMIT								300
#define ILITEK_MIN_CAP_LIMIT								100
#define ILITEK_SHORT_LIMIT								10
#define ILITEK_NOISE_LIMIT						30
#define ILITEK_OPEN_LIMIT						3200
#define ILITEK_TX_LIMIT							25
#define ILITEK_RX_LIMIT							40
#define ILITEK_DEBUG_DATA			    0x5F
#define ILITEK_REPORT_DATA_HEAD_LEN			3
#define ILITEK_ONE_FINGER_DATA_LEN			    5
#define ILITEK_X_COORD_H			    3
#define ILITEK_X_COORD_L			    4
#define ILITEK_Y_COORD_H			    5
#define ILITEK_Y_COORD_L			    6
#define ILITEK_PRESS			    7

#define INT_POLL_LONG_RETRY							300
#define INT_POLL_SHORT_RETRY							30
#define INT_POLL_SUSPEND_RESUME_RETRY				10
MODULE_AUTHOR("Steward_Fu");
MODULE_DESCRIPTION("ILITEK I2C touch driver for Android platform");
MODULE_LICENSE("GPL");

int ilitek_poll_int(void) ;
int ilitek_check_int_low(int retry);
int ilitek_check_int_high(int retry);
int ilitek_i2c_read_tp_info(void);
int ilitek_suspend(void);
int ilitek_resume(void);
int ilitek_i2c_read(u8 *addrs, u16 addr_size, u8 *values, u16 values_size);
int ilitek_i2c_write(u8 *values, u16 values_size);
int inwrite(unsigned int address);
int outwrite(unsigned int address, unsigned int data, int size);
void ilitek_i2c_irq_enable(void);
void ilitek_i2c_irq_disable(void);
int ilitek_reset(int delay);
void ilitek_set_finish_init_flag(void);
int ilitek_fw_update_boot(char *file_name);
int ilitek_fw_update_sd(char *file_name);
int ilitek_get_raw_data(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);
int ilitek_get_debug_data(struct ts_diff_data_info *info, struct ts_cmd_node *out_cmd);
int ilitek_glove_switch(struct ts_glove_info *info);
int ilitek_holster_switch(struct ts_holster_info *info);
int ilitek_roi_switch(struct ts_roi_info *info);
unsigned char *ilitek_roi_rawdata(void);
int ilitek_charger_switch(struct ts_charger_info *info);
int ilitek_i2c_write_and_read(uint8_t *cmd,
		int write_len, int delay, uint8_t *data, int read_len);
char *ilitek_strncat(unsigned char *dest, char *src, size_t dest_size);
char *ilitek_strncatint(unsigned char *dest, int src, char *format, size_t dest_size);
#endif
