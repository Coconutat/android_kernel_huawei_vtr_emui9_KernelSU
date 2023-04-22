#ifndef __ILITEK_COMMON_H__
#define __ILITEK_COMMON_H__
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/gpio.h>
#ifdef CONFIG_OF
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
#endif
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/slab.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/firmware.h>
#include <linux/debugfs.h>
#include <linux/mutex.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/workqueue.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/uaccess.h>
#include <linux/version.h>
#include <linux/cdev.h>
#include <linux/dma-mapping.h>
#include <huawei_platform/log/hw_log.h>
#include "../../huawei_ts_kit.h"
#include "../../huawei_ts_kit_algo.h"
#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

/* ilitek public driver debug define start */
#include <linux/netlink.h>
#include <linux/skbuff.h>
#include <linux/socket.h>
#include <net/sock.h>

/*
 * 2018-7-27 1.1.3.1
 * 1 add fw_update hw crc.
 * 2018-8-6
 * 1 i2c retry use ts_kit i2c_read retry.
 * 2 add 'static' to usersapce debug structuct avoid compile conflict with mstar driver.
 * 3 use ic resolution x[0~2047] y[0~2047] in normal mode to fix map resolution bug when playing games.
 *   driver convert ic resolution to real resoultion to fix inaccurate touch in recovery mode.
 * 4 move 'can't draw line along the edge' func to firmware.
 * 2018-8-8
 * 1 driver send sleep pt_cmd to firmware in power test mode to fix high power consumption bug.
 * 2 set 'use_lcdkit_power_notify' 1 to close fb_notify, tddi use lcdkit power notfiy not fb notify.
 * 2018-8-24
 * 1 fix free mp_test data resource bug when operating capacitance test node asynchronously.
 * 2 update mp_test driver,add some debug log.
 * 2018-9-17
 * 1 fix cdc timeout bug when switching to mp test mode.
 * 2 fix i2c operation error after fw checked.
 */
#define DRIVER_VERSION                         "1.1.3.1"
#define CSV_PATH                               "/data/local/tmp"
#define INI_NAME_PATH                          "/sdcard/mp.ini"
#define UPDATE_FW_PATH                         "/sdcard/ILITEK_FW"

#define DEBUG_DATA_NUM                         32
#define DEBUG_DATA_LEN                         2048

#define ERR_ALLOC_MEM(X)                       ((IS_ERR(X) || X == NULL) ? 1 : 0)
#define CHECK_EQUAL(X, Y)                      ((X == Y) ? 0 : -1)

static inline void ipio_kfree(void **mem)
{
    if (*mem != NULL) {
        kfree(*mem);
        *mem = NULL;
    }
}

static inline u32 HexToDec(char *pHex, s32 nLength)
{
    u32 nRetVal = 0, nTemp = 0, i;
    s32 nShift = (nLength - 1) * 4;

    for (i = 0; i < nLength; nShift -= 4, i++) {
        if ((pHex[i] >= '0') && (pHex[i] <= '9')) {
            nTemp = pHex[i] - '0';
        } else if ((pHex[i] >= 'a') && (pHex[i] <= 'f')) {
            nTemp = (pHex[i] - 'a') + 10;
        } else if ((pHex[i] >= 'A') && (pHex[i] <= 'F')) {
            nTemp = (pHex[i] - 'A') + 10;
        } else {
            return -1;
        }

        nRetVal |= (nTemp << nShift);
    }

    return nRetVal;
}

static inline int katoi(char *string)
{
    int result = 0;
    unsigned int digit;
    int sign;

    if (*string == '-') {
        sign = 1;
        string += 1;
    } else {
        sign = 0;
        if (*string == '+') {
            string += 1;
        }
    }

    for (;; string += 1) {
        digit = *string - '0';
        if (digit > 9)
            break;
        result = (10 * result) + digit;
    }

    if (sign) {
        return -result;
    }
    return result;
}

static inline int str2hex(char *str)
{
    int strlen, result, intermed, intermedtop;
    char *s = str;

    while (*s != 0x0) {
        s++;
    }

    strlen = (int)(s - str);
    s = str;
    if (*s != 0x30) {
        return -1;
    }

    s++;

    if (*s != 0x78 && *s != 0x58) {
        return -1;
    }
    s++;

    strlen = strlen - 3;
    result = 0;
    while (*s != 0x0) {
        intermed = *s & 0x0f;
        intermedtop = *s & 0xf0;
        if (intermedtop == 0x60 || intermedtop == 0x40) {
            intermed += 0x09;
        }
        intermed = intermed << (strlen << 2);
        result = result | intermed;
        strlen -= 1;
        s++;
    }
    return result;
}

static inline u8 calc_checksum(u8 *data, u32 len)
{
    u32 i;
    s32 sum = 0;

    for (i = 0; i < len; i++) {
        sum += data[i];
    }

    return (u8) ((-sum) & 0xFF);
}

int ilitek_proc_init(void);
void ilitek_proc_remove(void);
void netlink_reply_msg(void *raw, int size);
/* ilitek public driver debug define end */

extern u32 ilitek_debug_level;
extern struct ilitek_ts_data *g_ilitek_ts;

#define IS_APP_ENABLE_GESTURE(x)               ((u32)(1 << x))

#define ILITEK_CHIP_NAME                       "ilitek"
#define ILITEK_INI_PATH_PERFIX                 "/odm/etc/firmware/ts/"

/*
 * ts_kit i2c_read(I2C_RW_TRIES = 3, interval 25ms) fail retry 3 times.
 * so driver no needs to retry more.
 */
#define ILITEK_I2C_RETRYS                      1
#define ILITEK_VERSION_LEN                     32
#define ILITEK_PROJECT_ID_LEN                  10
#define ILITEK_FILE_PATH_LEN                   256
#define ILITEK_FILE_NAME_LEN                   128

#define ILITEK_TOUCSCEEN_X_MIN                 0
#define ILITEK_TOUCSCEEN_Y_MIN                 0
#define ILITEK_TOUCSCEEN_X_MAX                 720
#define ILITEK_TOUCSCEEN_Y_MAX                 1520
#define ILITEK_TOUCSCEEN_FINGERS               10
#define ILITEK_VIRTUAL_KEYS                    10
#define ILITEK_ROI_FINGERS                     2

/* define the range on panel */
#define TPD_HEIGHT                             2048
#define TPD_WIDTH                              2048

/* ILI7807 Series */
enum ili7881_types {
    ILI7807_TYPE_F_AA = 0x0000,
    ILI7807_TYPE_F_AB = 0x0001,
    ILI7807_TYPE_H = 0x1100,
};

#define ILI7807_SLAVE_ADDR                     0x41
#define ILI7807_ICE_MODE_ADDR                  0x181062
#define ILI7807_PID_ADDR                       0x4009C
#define ILI7807_WDT_ADDR                       0x5100C

/* ILI9881 Series */
enum ili9881_types {
    ILI9881_TYPE_F = 0x0F,
    ILI9881_TYPE_H = 0x11,
};

#define ILI9881_SLAVE_ADDR                     0x41
#define ILI9881_ICE_MODE_ADDR                  0x181062
#define ILI9881_PID_ADDR                       0x4009C
#define ILI9881_WDT_ADDR                       0x5100C

typedef enum support_ic {
    ILITEK_ILI7807 = 0x7807,
    ILITEK_ILI9881 = 0x9881,
    ILITEK_IC_NUMS = 2,
}support_ic;

enum {
    DEBUG_NONE = 0,
    DEBUG_IRQ = BIT(0),
    DEBUG_FINGER_REPORT = BIT(1),
    DEBUG_FIRMWARE = BIT(2),
    DEBUG_CONFIG = BIT(3),
    DEBUG_I2C = BIT(4),
    DEBUG_BATTERY = BIT(5),
    DEBUG_MP_TEST = BIT(6),
    DEBUG_IOCTL = BIT(7),
    DEBUG_NETLINK = BIT(8),
    DEBUG_PARSER = BIT(9),
    DEBUG_GESTURE = BIT(10),
    DEBUG_ROI = BIT(11),
    DEBUG_ALL = ~0,
};

#define ilitek_info(fmt, arg...) \
    TS_LOG_INFO("[ILITEK][INFO]: (%s, %d): " fmt, __func__, __LINE__, ##arg);

#define ilitek_err(fmt, arg...) \
    TS_LOG_ERR("[ILITEK][ERR]: (%s, %d): " fmt, __func__, __LINE__, ##arg);

#define ilitek_debug(level, fmt, arg...) \
    do { \
        if (level & ilitek_debug_level) \
        TS_LOG_INFO("[ILITEK][DEBUG]: (%s, %d): " fmt, __func__, __LINE__, ##arg); \
    } while (0)

struct ilitek_version {
    char str[ILITEK_VERSION_LEN];
    u8 data[ILITEK_VERSION_LEN];
};

struct ilitek_ts_data{
    struct ts_kit_device_data *ts_dev_data;
    struct platform_device *pdev;
    struct input_dev *input;
    struct device *dev;

    struct mutex wrong_touch_lock;

    struct pinctrl *pctrl;
    struct pinctrl_state *pins_default;
    struct pinctrl_state *pins_active;
    struct pinctrl_state *pins_idle;

    struct ilitek_config *cfg;
    struct ilitek_protocol *pro;
    struct ilitek_report *rpt;
    struct ilitek_test *test;
    struct ilitek_upgrade *upg;

    /* dts config */
    u16 x_max;
    u16 y_max;
    u16 x_min;
    u16 y_min;
    u16 finger_nums;

    u32 chip_id;
    u32 use_ic_res;
    u32 support_roi;
    u32 support_pressure;
    u32 support_gesture;
    u32 support_get_tp_color;
    u32 only_open_once_captest_threshold;
    u32 project_id_length_control;

    bool open_threshold_status;

    char project_id[ILITEK_PROJECT_ID_LEN];
    char ini_path[ILITEK_FILE_PATH_LEN];
    char fw_name[ILITEK_FILE_PATH_LEN];

    /* ilitek public driver debug define */
    bool isEnableFR;
    bool isEnableNetlink;
    bool debug_node_open;
    int debug_data_frame;
    wait_queue_head_t inq;
    u8 *i2cuart_data;
    u32 i2cuart_len;
    u8 *debug_data;
    u32 debug_len;
    u8 debug_buf[DEBUG_DATA_NUM][DEBUG_DATA_LEN];
    struct mutex ilitek_report_irq;
    struct mutex ilitek_debug_mutex;
    struct mutex ilitek_debug_read_mutex;
};

int ilitek_chip_reset(void);

#endif
