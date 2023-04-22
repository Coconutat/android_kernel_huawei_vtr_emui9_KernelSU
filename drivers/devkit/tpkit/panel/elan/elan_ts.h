#ifndef _LINUX_ELAN_KTF_H
#define _LINUX_ELAN_KTF_H
#include <linux/device.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#elif defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#endif
#include <asm/unaligned.h>
#include <linux/debugfs.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/hid.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/kernel.h>
#include <linux/limits.h>
#include <linux/module.h>
#include <linux/of_device.h>
#include <linux/of.h>
#include <linux/pm_runtime.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/suspend.h>
#include <linux/stringify.h>
#include <linux/types.h>
#include <linux/uaccess.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/miscdevice.h>
#include <linux/stringify.h>
#include "../../huawei_ts_kit_algo.h"
#include "../../huawei_ts_kit.h"

#define ELAN_KTF_NAME 	"elan"
#define ELAN_IC_NAME 	"ekth5512c_"
#define ELAN_TP_IC_TYPE "ic_type"
#define LCD_PANEL_TYPE_DEVICE_NODE_NAME     "huawei,lcd_panel_type"
#define MAX_FINGER_PRESSURE		255
#define MAX_FINGER_SIZE		10
#define ELAN_RECV_DATA_LEN	67
#define ELAN_SEND_DATA_LEN	37
#define MAX_WRITE_LEN	8192
#define PEN_REPORT_ID	0x07
#define FINGER_REPORT_ID	0x01
#define ELAN_SLAVE_ADDR			"slave_address"
#define FW_SUFFIX	".bin"
#define ELAN_I2C_ADRR	 0x10
#define CHIP_NAME_LEN 	10
#define TP_MODULE_NAME	"null"
#define FwPageSize	132
#define WriteDataValidLen	28
#define WritePages	30
#define ELAN_TOOL_TYPE_NONE  0
#define FINGER_OSR	64
#define PEN_OSR		260
#define MAX_PEN_PRESSURE	2047
#define ELAN_IAP
#define TEN_FINGER_DATA_LEN	90
#define REPORT_DATA_LEN	18
#define ResultMaxLen	10

#define FWID_HIGH_BYTE_IN_EKT 54763
#define FWID_LOW_BYTE_IN_EKT 54762
#define FWVER_HIGH_BYTE_IN_EKT 53727
#define FWVER_LOW_BYTE_IN_EKT 53726
#define TP_NORMAL_DATA	0x20
#define TP_RECOVER_DATA	0xa6
#define MAX_NAME_LEN	64
#define ELAN_GET_RAWDATA_TIMEOUT	10
#define IC_ACK	0xaa

#define PenX_Point_HByte	5
#define PenX_Point_LByte	4
#define PenY_Point_HByte	7
#define PenY_Point_LByte	6
#define Pen_Press_HByte	9
#define Pen_Press_LByte	8

#define FingerX_Point_HByte	4
#define FingerX_Point_LByte	3
#define FingerY_Point_HByte	6
#define FingerY_Point_LByte	5
#define Finger_Pressure	255
#define Finger_Major	100
#define Finger_Minor	100
#define Value_Offset	7
#define Fw_Update_Retry	2
#define CUR_FINGER_NUM_BYTE	17
#define TP_NORMAL_DATA_BYTE	4
#define TP_RECOVER_DATA_BYTE	6
#define WriteDataValidLen_Byte 8
#define OFFSET_LBYTE	7
#define OFFSET_HBYTE	6
#define REPORT_ID_BYTE	2
#define POINT_HEAD_LEN	3
#define SUSPEND_OR_REPORT_BYTE 8
#define TP_COLOR_SIZE 15
#define VCI_LDO_TYPE 1
#define VCI_GPIO_TYPE 0
#define VDD_LDO_TYPE 1
#define VDD_GPIO_TYPE 0
#define PROJECT_ID_INDEX 7
#define COLOR_ID_INDEX 17
#define FW_INFO_INDEX 5
#define RX_NUM_INDEX 6
#define TX_NUM_INDEX 7
#define PROJECT_ID_POLL	5
#define LCD_PANEL_INFO_MAX_LEN  128
#define ELAN_PANEL_ID_START_BIT	6

struct elan_ktf_ts_data {
	int gpio_3v3_en;
	int gpio_1v8_en;
 	int reset_gpio;
	int int_gpio;
	int irq_id;
	int fw_ver;
	int fw_id;
	int tx_num;
	int rx_num;
	int finger_x_resolution;
	int finger_y_resolution;
	int pen_x_resolution;
	int pen_y_resolution;
	int cur_finger_num;
	struct miscdevice elan_device;	//fw debug node
	struct ts_kit_device_data *elan_chip_data;
	struct ts_kit_platform_data *elan_chip_client;
	struct platform_device *elan_dev;
	int self_ctrl_power;
	struct regulator *vddd;
	struct regulator *vdda;
	struct mutex ktf_mutex;
	atomic_t tp_mode;
	char project_id[10];
	char color_id[2];
	struct wake_lock wake_lock;
	bool sd_fw_updata;
	bool pen_detected;
	char lcd_panel_info[LCD_PANEL_INFO_MAX_LEN];
	char lcd_module_name[MAX_STR_LEN];
};

extern int elan_i2c_write(u8* buf, u16 length);
extern int elan_i2c_read(u8 *reg_addr, u16 reg_len, u8 *buf, u16 len);

#endif /* _LINUX_ELAN_KTF_H */

