/*
 * Copyright (C) 2006-2017 ILITEK TECHNOLOGY CORP.
 *
 * Description: ILITEK I2C touchscreen driver for linux platform.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see the file COPYING, or write
 * to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Johnson Yeh
 * Maintain: Luca Hsu, Tigers Huang, Dicky Chiang
 */

/**
 *
 * @file    mstar_common.h
 *
 * @brief   This file defines the interface of touch screen
 *
 *
 */

#ifndef __MSTAR_COMMON_H__
#define __MSTAR_COMMON_H__

/*--------------------------------------------------------------------------*/
/* INCLUDE FILE                                                             */
/*--------------------------------------------------------------------------*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/types.h>
#include <linux/input.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/vmalloc.h>
#ifdef CONFIG_HAS_EARLYSUSPEND
#include <linux/earlysuspend.h>
#endif //CONFIG_HAS_EARLYSUSPEND
#include <linux/i2c.h>
#include <linux/proc_fs.h>
#include <linux/file.h>
#include <linux/fs.h>
#include <linux/string.h>
#include <linux/miscdevice.h>
#include <linux/time.h>
#include <linux/input/mt.h>
#include <linux/kobject.h>
#include <linux/version.h>
#include <asm/unistd.h>
#include <asm/uaccess.h>
#include <asm/irq.h>
#include <asm/io.h>

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
#endif //DEBUG_NETLINK
#include <huawei_platform/log/hw_log.h>
#include "../../huawei_ts_kit.h"

#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif
#if defined (CONFIG_HUAWEI_DSM)
extern struct dsm_client *ts_dclient;
#endif


/*--------------------------------------------------------------------------*/
/* TOUCH DEVICE DRIVER RELEASE VERSION                                      */
/*--------------------------------------------------------------------------*/

#define DEVICE_DRIVER_RELEASE_VERSION   ("8.0.3.2")
#define DEVICE_FIRMWARE_VERSION         "00008.00009"

/*--------------------------------------------------------------------------*/
/* COMPILE OPTION DEFINITION                                                */
/*--------------------------------------------------------------------------*/

#define FALSE 0
#define TRUE  1
#define false 0
#define true  1

#define SLAVE_I2C_ID_DBBUS  0x62
#define SLAVE_I2C_ID_DWI2C  0x26

#define MSTAR_ROI_ENABLE 1

#define LENS_AUO_SWID  0x4007
#define LENS_EBBG_SWID  0x8007
#define EELY_AUO_SWID  0x400a
#define EELY_EBBG_SWID  0x800a

#define GESTURE_PACKET_HEADER  0x5A
/*
 * Note.
 * One or more than one the below compile option can be enabled based on the touch ic that customer project are used.
 * By default, the below compile option are all disabled.
 */
//#define CONFIG_ENABLE_CHIP_TYPE_MSG22XX
#define CONFIG_ENABLE_CHIP_TYPE_MSG28XX // This compile option can be used for MSG28XX/MSG58XX/MSG58XXA/ILI2117A/ILI2118A
//#define CONFIG_ENABLE_CHIP_TYPE_ILI21XX               // This compile option can be used for ILI2120/ILI2121

/*
 * Note.
 * The below compile option is used to enable touch pin control for specific SPRD/QCOM platform.
 * This compile option is used for specific SPRD/QCOM platform only.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_TOUCH_PIN_CONTROL

/*
 * Note.
 * The below flag is used to enable debug mode data log for firmware.
 * Please make sure firmware support debug mode data log firstly, then you can enable this flag.
 * By default, this flag is enabled.
 */
#define CONFIG_ENABLE_FIRMWARE_DATA_LOG (1) // 1 : Enable, 0 : Disable

/*
 * Note.
 * The below compile option is used to enable segment read debug mode finger touch data for MSG28XX/MSG58XX/MSG58XXA only.
 * Since I2C transaction length limitation for some specific MTK BB chip(EX. MT6589/MT6572/...) or QCOM BB chip, the debug mode finger touch data of MSG28XX/MSG58XX/MSG58XXA can not be retrieved by one time I2C read operation.
 * So we need to retrieve the complete finger touch data by segment read.
 * By default, this compile option is enabled.
 */
#define CONFIG_ENABLE_SEGMENT_READ_FINGER_TOUCH_DATA

/*
 * Note.
 * The below flag is used to enable output specific debug value for firmware by MTPTool APK/TP Test Studio Tool.
 * Please make sure firmware support ENABLE_APK_PRINT_FW_LOG_DEBUG function firstly, then you can use this feature.
 * By default, this flag is enabled.
 */
#define CONFIG_ENABLE_APK_PRINT_FIRMWARE_SPECIFIC_LOG (0)   // 1 : Enable, 0 : Disable

/*
 * Note.
 * The below compile option is used to enable gesture wakeup.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_GESTURE_WAKEUP

/*
 * Note.
 * The below compile option is used to enable the specific short test item of 2R triangle pattern for self-capacitive touch ic.
 * This compile option is used for MSG22XX only.
 * Please enable the compile option if the ITO pattern is 2R triangle pattern for MSG22XX.
 * Please disable the compile option if the ITO pattern is H(horizontal) triangle pattern for MSG22XX.
 * By default, this compile option is enabled.
 */
#define CONFIG_ENABLE_MP_TEST_ITEM_FOR_2R_TRIANGLE
#define MP_TEST_FUNCTION_2  //for Camaro and cobra

/*
 * Note.
 * If this compile option is defined, the update firmware bin file shall be stored in a two dimensional array format.
 * Else, the update firmware bin file shall be stored in an one dimensional array format.
 * Be careful, MSG22XX only support storing update firmware bin file in an one dimensional array format, it does not support two dimensional array format.
 * By default, this compile option is enabled.
 */
#define CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY

/*
 * Note.
 * The below compile option is used to enable jni interface.
 * By default, this compile option is enabled.
 */
#define CONFIG_ENABLE_JNI_INTERFACE

/*
 * Note.
 * The below compile option is used to enable the debug code for clarifying some issues. For example, to debug the delay time issue for IC hardware reset.
 * By the way, this feature is supported for MSG28XX/MSG58XX/MSG58XXA only.
 * By default, this compile option is disabled.
 */
#define CONFIG_ENABLE_CODE_FOR_DEBUG



/*
 * Note.
 * The below compile option is used to enable update firmware with I2C data rate 400KHz for MSG22XX.
 * If this compile option is disabled, then update firmware with I2C data rate less than 400KHz for MSG22XX.
 * By default, this compile option is disabled.
 */
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

// ------------------- #ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K ------------------- //
#ifdef CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

/*
 * Note.
 * There are three methods to update firmware for MSG22XX(with chip revision >= 0x04) when I2C data rate is 400KHz.
 * Method A. Enable I2C 400KHz burst write mode, let e-flash discard the last 2 dummy byte.
 * Method B. Enable I2C 400KHz burst write mode, let e-flash discard the last 3 dummy byte.
 * Method C. Enable I2C 400KHz non-burst write mode, only one byte can be written each time.
 * By default, the compile option CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A is enabled.
 */
#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_A
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_B
//#define CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K_BY_METHOD_C

#endif //CONFIG_ENABLE_UPDATE_FIRMWARE_WITH_SUPPORT_I2C_SPEED_400K

#include <linux/sched.h>
#include <linux/kthread.h>
//#include <linux/rtpm_prio.h>
#include <linux/wait.h>
#include <linux/time.h>

#include <linux/namei.h>
#include <linux/vmalloc.h>

#ifdef CONFIG_ENABLE_PROXIMITY_DETECTION
extern int mstar_proximity_enable(int mode);
#endif //CONFIG_ENABLE_PROXIMITY_DETECTION

#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/of_irq.h>
#include <linux/gpio.h>

#ifdef TIMER_DEBUG
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#endif //TIMER_DEBUG

#ifdef CONFIG_MTK_SENSOR_HUB_SUPPORT
#include <mach/md32_ipi.h>
#include <mach/md32_helper.h>
#endif //CONFIG_MTK_SENSOR_HUB_SUPPORT

#ifdef CONFIG_ENABLE_REGULATOR_POWER_ON
#include <linux/regulator/consumer.h>
#endif //CONFIG_ENABLE_REGULATOR_POWER_ON
#define CONFIG_MT_RT_MONITOR
#ifdef CONFIG_MT_RT_MONITOR
#define MT_ALLOW_RT_PRIO_BIT 0x10000000
#else
#define MT_ALLOW_RT_PRIO_BIT 0x0
#endif //CONFIG_MT_RT_MONITOR
#define REG_RT_PRIO(x) ((x) | MT_ALLOW_RT_PRIO_BIT)

#define RTPM_PRIO_TPD  REG_RT_PRIO(4)

#ifdef CONFIG_TP_HAVE_KEY
#define TOUCH_KEY_MENU    KEY_MENU
#define TOUCH_KEY_HOME    KEY_HOMEPAGE
#define TOUCH_KEY_BACK    KEY_BACK
#define TOUCH_KEY_SEARCH  KEY_SEARCH

#define MAX_KEY_NUM (4)
#endif //CONFIG_TP_HAVE_KEY

/********************************* huawei tpkit *********************************/
#define MSTAR_CHIP_NAME               "mstar"
#define HUAWEI_TS_KIT                 "huawei,ts_kit"
#define MSTAR_DETECT_I2C_RETRY_TIMES  2

#define MSTAR_VENDOR_COMP_NAME_LEN    32
#define MSTAR_VENDOR_NAME_LEN         8
#define MSTAR_PROJECT_ID_LEN          11

#define MSTAR_VENDOR_ID_HLT           0x08
#define MSTAR_VENDOR_NAME_HLT         "hlt"

#define MSTAR_PROJECT_ID_ADDR         0x0780
#define MSTAR_COLOR_ID_ADDR           0x078A
#define MSTAR_GOLDEN_VER_ADDR         0x07A0

#define TP_COLOR_SIZE                 15
#define IS_APP_ENABLE_GESTURE(x)      ((u32)(1<<x))
/********************************* huawei tpkit *********************************/

#define u8   unsigned char
#define u16  unsigned short
#define u32  unsigned int
#define s8   signed char
#define s16  signed short
#define s32  signed int
#define s64  int64_t
#define u64  uint64_t

// Chip Id
#define CHIP_TYPE_MSG21XX   (0x01)  // EX. MSG2133
#define CHIP_TYPE_MSG21XXA  (0x02)  // EX. MSG2133A/MSG2138A(Besides, use version to distinguish MSG2133A/MSG2138A, you may refer to _DrvFwCtrlUpdateFirmwareCash())
#define CHIP_TYPE_MSG26XXM  (0x03)  // EX. MSG2633M
#define CHIP_TYPE_MSG22XX   (0x7A)  // EX. MSG2238/MSG2256
#define CHIP_TYPE_MSG28XX   (0x85)  // EX. MSG2833/MSG2835/MSG2836/MSG2840/MSG2856/MSG5846
#define CHIP_TYPE_MSG28XXA   (0xBF) // EX. MSG2856
#define CHIP_TYPE_MSG58XXA  (0xBF)  // EX. MSG5846A
#define CHIP_TYPE_MSG2836A  (0x2836)    // EX. MSG2836A
#define CHIP_TYPE_MSG2846A  (0x2846)    // EX. MSG2846A
#define CHIP_TYPE_MSG5846A  (0x5846)    // EX. MSG5846A
#define CHIP_TYPE_MSG5856A  (0x5856)    // EX. MSG5856A

// Chip Revision
#define CHIP_TYPE_MSG22XX_REVISION_U05   (0x04) // U05

#define PACKET_TYPE_SELF_FREQ_SCAN  (0x02)
#define PACKET_TYPE_TOOTH_PATTERN   (0x20)
#define PACKET_TYPE_CSUB_PATTERN    (0x30)
#define PACKET_TYPE_FOUT_PATTERN    (0x40)
#define PACKET_TYPE_GESTURE_WAKEUP  (0x50)
#define PACKET_TYPE_FREQ_PATTERN    (0x60)
#define PACKET_TYPE_GESTURE_DEBUG       (0x51)
#define PACKET_TYPE_GESTURE_INFORMATION (0x52)
#define PACKET_TYPE_ESD_CHECK_HW_RESET  (0x60)

#define TOUCH_SCREEN_X_MIN   (0)
#define TOUCH_SCREEN_Y_MIN   (0)
/*
 * Note.
 * Please change the below touch screen resolution according to the touch panel that you are using.
 */
//#define TOUCH_SCREEN_X_MAX   (720)  //LCD_WIDTH
//#define TOUCH_SCREEN_Y_MAX   (1440) //LCD_HEIGHT
/*
 * Note.
 * Please do not change the below setting.
 */
#define TPD_WIDTH   (2048)
#define TPD_HEIGHT  (2048)

#define BIT0  (1<<0)        // 0x0001
#define BIT1  (1<<1)        // 0x0002
#define BIT2  (1<<2)        // 0x0004
#define BIT3  (1<<3)        // 0x0008
#define BIT4  (1<<4)        // 0x0010
#define BIT5  (1<<5)        // 0x0020
#define BIT6  (1<<6)        // 0x0040
#define BIT7  (1<<7)        // 0x0080
#define BIT8  (1<<8)        // 0x0100
#define BIT9  (1<<9)        // 0x0200
#define BIT10 (1<<10)       // 0x0400
#define BIT11 (1<<11)       // 0x0800
#define BIT12 (1<<12)       // 0x1000
#define BIT13 (1<<13)       // 0x2000
#define BIT14 (1<<14)       // 0x4000
#define BIT15 (1<<15)       // 0x8000

#define MAX_DEBUG_REGISTER_NUM     (10)
#define MAX_DEBUG_COMMAND_ARGUMENT_NUM      (4)

#define MAX_UPDATE_FIRMWARE_BUFFER_SIZE    (130)    // 130KB. The size shall be large enough for stored any kind firmware size of MSG22XX(48.5KB)/MSG28XX(130KB)/MSG58XX(130KB)/MSG58XXA(130KB).

#define MAX_I2C_TRANSACTION_LENGTH_LIMIT      (250) //(128) // Please change the value depends on the I2C transaction limitation for the platform that you are using.
#define MAX_TOUCH_IC_REGISTER_BANK_SIZE       (256) // It is a fixed value and shall not be modified.

#define PROCFS_AUTHORITY (0666)

#define LETTER_LOCUS_NUM 6
#define LINEAR_LOCUS_NUM 2

#define KEY_CODE_DOUBLE_CLICK (0x58)
#define KEY_CODE_M            (0X64)
#define KEY_CODE_W            (0x65)
#define KEY_CODE_C            (0X66)
#define KEY_CODE_E            (0x67)

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#define GESTURE_WAKEUP_MODE_DOUBLE_CLICK_FLAG     0x00000001
#define GESTURE_WAKEUP_MODE_UP_DIRECT_FLAG        0x00000002
#define GESTURE_WAKEUP_MODE_DOWN_DIRECT_FLAG      0x00000004
#define GESTURE_WAKEUP_MODE_LEFT_DIRECT_FLAG      0x00000008
#define GESTURE_WAKEUP_MODE_RIGHT_DIRECT_FLAG     0x00000010
#define GESTURE_WAKEUP_MODE_m_CHARACTER_FLAG      0x00000020
#define GESTURE_WAKEUP_MODE_W_CHARACTER_FLAG      0x00000040
#define GESTURE_WAKEUP_MODE_C_CHARACTER_FLAG      0x00000080
#define GESTURE_WAKEUP_MODE_e_CHARACTER_FLAG      0x00000100
#define GESTURE_WAKEUP_MODE_V_CHARACTER_FLAG      0x00000200
#define GESTURE_WAKEUP_MODE_O_CHARACTER_FLAG      0x00000400
#define GESTURE_WAKEUP_MODE_S_CHARACTER_FLAG      0x00000800
#define GESTURE_WAKEUP_MODE_Z_CHARACTER_FLAG      0x00001000
#define GESTURE_WAKEUP_MODE_RESERVE1_FLAG         0x00002000
#define GESTURE_WAKEUP_MODE_RESERVE2_FLAG         0x00004000
#define GESTURE_WAKEUP_MODE_RESERVE3_FLAG         0x00008000

#ifdef CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE
#define GESTURE_WAKEUP_MODE_RESERVE4_FLAG         0x00010000
#define GESTURE_WAKEUP_MODE_RESERVE5_FLAG         0x00020000
#define GESTURE_WAKEUP_MODE_RESERVE6_FLAG         0x00040000
#define GESTURE_WAKEUP_MODE_RESERVE7_FLAG         0x00080000
#define GESTURE_WAKEUP_MODE_RESERVE8_FLAG         0x00100000
#define GESTURE_WAKEUP_MODE_RESERVE9_FLAG         0x00200000
#define GESTURE_WAKEUP_MODE_RESERVE10_FLAG        0x00400000
#define GESTURE_WAKEUP_MODE_RESERVE11_FLAG        0x00800000
#define GESTURE_WAKEUP_MODE_RESERVE12_FLAG        0x01000000
#define GESTURE_WAKEUP_MODE_RESERVE13_FLAG        0x02000000
#define GESTURE_WAKEUP_MODE_RESERVE14_FLAG        0x04000000
#define GESTURE_WAKEUP_MODE_RESERVE15_FLAG        0x08000000
#define GESTURE_WAKEUP_MODE_RESERVE16_FLAG        0x10000000
#define GESTURE_WAKEUP_MODE_RESERVE17_FLAG        0x20000000
#define GESTURE_WAKEUP_MODE_RESERVE18_FLAG        0x40000000
#define GESTURE_WAKEUP_MODE_RESERVE19_FLAG        0x80000000

#define GESTURE_WAKEUP_MODE_RESERVE20_FLAG        0x00000001
#define GESTURE_WAKEUP_MODE_RESERVE21_FLAG        0x00000002
#define GESTURE_WAKEUP_MODE_RESERVE22_FLAG        0x00000004
#define GESTURE_WAKEUP_MODE_RESERVE23_FLAG        0x00000008
#define GESTURE_WAKEUP_MODE_RESERVE24_FLAG        0x00000010
#define GESTURE_WAKEUP_MODE_RESERVE25_FLAG        0x00000020
#define GESTURE_WAKEUP_MODE_RESERVE26_FLAG        0x00000040
#define GESTURE_WAKEUP_MODE_RESERVE27_FLAG        0x00000080
#define GESTURE_WAKEUP_MODE_RESERVE28_FLAG        0x00000100
#define GESTURE_WAKEUP_MODE_RESERVE29_FLAG        0x00000200
#define GESTURE_WAKEUP_MODE_RESERVE30_FLAG        0x00000400
#define GESTURE_WAKEUP_MODE_RESERVE31_FLAG        0x00000800
#define GESTURE_WAKEUP_MODE_RESERVE32_FLAG        0x00001000
#define GESTURE_WAKEUP_MODE_RESERVE33_FLAG        0x00002000
#define GESTURE_WAKEUP_MODE_RESERVE34_FLAG        0x00004000
#define GESTURE_WAKEUP_MODE_RESERVE35_FLAG        0x00008000
#define GESTURE_WAKEUP_MODE_RESERVE36_FLAG        0x00010000
#define GESTURE_WAKEUP_MODE_RESERVE37_FLAG        0x00020000
#define GESTURE_WAKEUP_MODE_RESERVE38_FLAG        0x00040000
#define GESTURE_WAKEUP_MODE_RESERVE39_FLAG        0x00080000
#define GESTURE_WAKEUP_MODE_RESERVE40_FLAG        0x00100000
#define GESTURE_WAKEUP_MODE_RESERVE41_FLAG        0x00200000
#define GESTURE_WAKEUP_MODE_RESERVE42_FLAG        0x00400000
#define GESTURE_WAKEUP_MODE_RESERVE43_FLAG        0x00800000
#define GESTURE_WAKEUP_MODE_RESERVE44_FLAG        0x01000000
#define GESTURE_WAKEUP_MODE_RESERVE45_FLAG        0x02000000
#define GESTURE_WAKEUP_MODE_RESERVE46_FLAG        0x04000000
#define GESTURE_WAKEUP_MODE_RESERVE47_FLAG        0x08000000
#define GESTURE_WAKEUP_MODE_RESERVE48_FLAG        0x10000000
#define GESTURE_WAKEUP_MODE_RESERVE49_FLAG        0x20000000
#define GESTURE_WAKEUP_MODE_RESERVE50_FLAG        0x40000000
#define GESTURE_WAKEUP_MODE_RESERVE51_FLAG        0x80000000
#endif //CONFIG_SUPPORT_64_TYPES_GESTURE_WAKEUP_MODE

#define GESTURE_WAKEUP_PACKET_LENGTH    (6)

#define GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH    (128)

#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#define GESTURE_WAKEUP_PACKET_LENGTH_MAX    (128)

#define FEATURE_GESTURE_WAKEUP_MODE         0x0001
#define FEATURE_GESTURE_DEBUG_MODE          0x0002
#define FEATURE_GESTURE_INFORMATION_MODE    0x0003

#define FEATURE_TOUCH_DRIVER_DEBUG_LOG      0x0010
#define FEATURE_FIRMWARE_DATA_LOG           0x0011
#define FEATURE_FORCE_TO_UPDATE_FIRMWARE    0x0012
#define FEATURE_DISABLE_ESD_PROTECTION_CHECK    0x0013
#define FEATURE_APK_PRINT_FIRMWARE_SPECIFIC_LOG    0x0014
#define FEATURE_SELF_FREQ_SCAN    0x0015

#define I2C_WRITE_COMMAND_DELAY_FOR_FIRMWARE   (20) // delay 20ms
#define I2C_SMBUS_WRITE_COMMAND_DELAY_FOR_PLATFORM   (5)    // delay 5ms
#define I2C_SMBUS_READ_COMMAND_DELAY_FOR_PLATFORM   (5) // delay 5ms

#define FIRMWARE_FILE_PATH_ON_SD_CARD      "/mnt/sdcard/msctp_update.bin"
#define FIRMWARE_FILE_PATH_FOR_HW         "ts/touch_screen_firmware.bin"
#define POWER_SUPPLY_BATTERY_STATUS_PATCH  "/sys/class/power_supply/battery/status"
#define ILITEK_TRIMCODE_INITIAL_PATH_ON_SD_CARD      "/mnt/sdcard/trimcode.txt"
#define CHARGER_DETECT_CHECK_PERIOD   (100) // delay 1s

#define ESD_PROTECT_CHECK_PERIOD   (300)    // delay 3s
#define ESD_CHECK_HW_RESET_PACKET_LENGTH    (8)

#ifdef CONFIG_ENABLE_JNI_INTERFACE
#define MSGTOOL_MAGIC_NUMBER               96
#define MSGTOOL_IOCTL_RUN_CMD              _IO(MSGTOOL_MAGIC_NUMBER, 1)

#define MSGTOOL_RESETHW           0x01
#define MSGTOOL_REGGETXBYTEVALUE  0x02
#define MSGTOOL_HOTKNOTSTATUS     0x03
#define MSGTOOL_FINGERTOUCH       0x04
#define MSGTOOL_BYPASSHOTKNOT     0x05
#define MSGTOOL_DEVICEPOWEROFF    0x06
#define MSGTOOL_GETSMDBBUS        0x07
#define MSGTOOL_SETIICDATARATE    0x08
#define MSGTOOL_ERASE_FLASH       0x09
#endif //CONFIG_ENABLE_JNI_INTERFACE

#define PROC_NODE_CLASS                       "class"
#define PROC_NODE_MS_TOUCHSCREEN_MSG20XX      "ms-touchscreen-msg20xx"
#define PROC_NODE_DEVICE                      "device"
#define PROC_NODE_CHIP_TYPE                   "chip_type"
#define PROC_NODE_FIRMWARE_DATA               "data"
#define PROC_NODE_FIRMWARE_UPDATE             "update"
#define PROC_NODE_CUSTOMER_FIRMWARE_VERSION   "version"
#define PROC_NODE_PLATFORM_FIRMWARE_VERSION   "platform_version"
#define PROC_NODE_DEVICE_DRIVER_VERSION       "driver_version"
#define PROC_NODE_SD_CARD_FIRMWARE_UPDATE     "sdcard_update"
#define PROC_NODE_FIRMWARE_DEBUG              "debug"
#define PROC_NODE_FIRMWARE_SET_DEBUG_VALUE    "set_debug_value"
#define PROC_NODE_FIRMWARE_SMBUS_DEBUG        "smbus_debug"

#define PROC_NODE_MP_TEST_CUSTOMISED          "mp_test_customised"

#define PROC_NODE_FIRMWARE_SET_DQMEM_VALUE    "set_dqmem_value"

#define PROC_NODE_FIRMWARE_MODE               "mode"
#define PROC_NODE_FIRMWARE_SENSOR             "sensor"
#define PROC_NODE_FIRMWARE_PACKET_HEADER      "header"

#define PROC_NODE_QUERY_FEATURE_SUPPORT_STATUS   "query_feature_support_status"
#define PROC_NODE_CHANGE_FEATURE_SUPPORT_STATUS  "change_feature_support_status"

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#define PROC_NODE_GESTURE_WAKEUP_MODE         "gesture_wakeup_mode"
#define PROC_NODE_GESTURE_INFORMATION_MODE    "gesture_infor"
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#define PROC_NODE_GLOVE_MODE                  "glove_mode"
#define PROC_NODE_OPEN_GLOVE_MODE             "open_glove_mode"
#define PROC_NODE_CLOSE_GLOVE_MODE            "close_glove_mode"
#define PROC_NODE_PROXIMITY_MODE              "proximity_mode"
#define PROC_NODE_DEBUG_MESSAGE              "debug_message"
#define PROC_NODE_DEBUG_MESSAGE_SWITCH              "debug_message_switch"

#define PROC_NODE_LEATHER_SHEATH_MODE         "leather_sheath_mode"

#ifdef CONFIG_ENABLE_JNI_INTERFACE
#define PROC_NODE_JNI_NODE                    "msgtool"
#endif //CONFIG_ENABLE_JNI_INTERFACE

#define PROC_NODE_SELINUX_LIMIT_FIRMWARE_UPDATE     "selinux_limit_update"
#define PROC_NODE_FORCE_FIRMWARE_UPDATE             "force_fw_update"
#define PROC_NODE_TRIM_CODE                         "trimcode"
#define PROC_NODE_CONTROL_FILM_MODE                 "film"

#define MUTUAL_DEMO_MODE_PACKET_LENGTH    (43)  // for MSG28xx

#define MUTUAL_MAX_TOUCH_NUM           (10) // for MSG28xx

#define MUTUAL_DEBUG_MODE_PACKET_LENGTH    (1280)   // for MSG28xx. It is a predefined maximum packet length, not the actual packet length which queried from firmware.

#define MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE (128)  //128K
#define MSG28XX_FIRMWARE_INFO_BLOCK_SIZE (2)    //2K
#define MSG28XX_FIRMWARE_WHOLE_SIZE (MSG28XX_FIRMWARE_MAIN_BLOCK_SIZE+MSG28XX_FIRMWARE_INFO_BLOCK_SIZE) //130K

#define MSG28XX_EMEM_SIZE_BYTES_PER_ONE_PAGE  (128)
#define MSG28XX_EMEM_SIZE_BYTES_ONE_WORD  (4)

#define MSG28XX_EMEM_MAIN_MAX_ADDR  (0x3FFF)    //0~0x3FFF = 0x4000 = 16384 = 65536/4
#define MSG28XX_EMEM_INFO_MAX_ADDR  (0x1FF) //0~0x1FF = 0x200 = 512 = 2048/4

#define MSG28XX_FIRMWARE_MODE_UNKNOWN_MODE (0xFF)
#define MSG28XX_FIRMWARE_MODE_DEMO_MODE    (0x00)
#define MSG28XX_FIRMWARE_MODE_DEBUG_MODE   (0x01)

#define UPDATE_FIRMWARE_RETRY_COUNT (2)

#ifdef CONFIG_ENABLE_GESTURE_WAKEUP
#define FIRMWARE_GESTURE_INFORMATION_MODE_A (0x00)
#define FIRMWARE_GESTURE_INFORMATION_MODE_B (0x01)
#define FIRMWARE_GESTURE_INFORMATION_MODE_C (0x02)
#endif //CONFIG_ENABLE_GESTURE_WAKEUP

#define FEATURE_FILM_MODE_LOW_STRENGTH          0x01
#define FEATURE_FILM_MODE_HIGH_STRENGTH         0x02
#define FEATURE_FILM_MODE_DEFAULT               0x00


#define MIN(a,b) (((a)<(b))?(a):(b))
#define MAX(a,b) (((a)>(b))?(a):(b))

#define LCD_PANEL_INFO_MAX_LEN  128

/*--------------------------------------------------------------------------*/
/* DATA TYPE DEFINITION                                                     */
/*--------------------------------------------------------------------------*/

typedef enum {
    EMEM_ALL = 0,
    EMEM_MAIN,
    EMEM_INFO
} EmemType_e;

typedef enum {
    ADDRESS_MODE_8BIT = 0,
    ADDRESS_MODE_16BIT = 1
} AddressMode_e;

typedef enum {
    MUTUAL_MODE = 0x5705,
    MUTUAL_SINE = 0x5706,
    MUTUAL_KEY = 0x6734,    // latter FW v1007.
    MUTUAL_SINE_KEY = 0x6733,   // latter FW v1007.
    SELF = 0x6278,
    WATERPROOF = 0x7992,
    MUTUAL_SINGLE_DRIVE = 0x0158,
    SINE_PHASE = 0xECFA,
    SET_PHASE,
    DEEP_STANDBY = 0x6179,
    GET_BG_SUM = 0x7912,
} ItoTestMsg28xxFwMode_e;

typedef enum {
    _50p,
    _40p,
    _30p,
    _20p,
    _10p
} ItoTestCfbValue_e;

typedef enum {
    DISABLE = 0,
    ENABLE,
} ItoTestChargePumpStatus_e;

typedef enum {
    GND = 0x00,
    POS_PULSE = 0x01,
    NEG_PULSE = 0x02,
    HIGH_IMPEDENCE = 0x03,
} ItoTestSensorPADState_e;

typedef struct {
    u16 nX;
    u16 nY;
} SelfTouchPoint_t;

typedef struct {
    u8 nTouchKeyMode;
    u8 nTouchKeyCode;
    u8 nFingerNum;
    SelfTouchPoint_t tPoint[2];
} SelfTouchInfo_t;

typedef struct {
    u8 nFirmwareMode;
    u8 nLogModePacketHeader;
    u16 nLogModePacketLength;
    u8 nIsCanChangeFirmwareMode;
} SelfFirmwareInfo_t;

typedef struct {
    u16 nId;
    u16 nX;
    u16 nY;
    u16 nP;
} MutualTouchPoint_t;

/// max 80+1+1 = 82 bytes
typedef struct {
    u8 nCount;
    u8 nKeyCode;
    MutualTouchPoint_t tPoint[10];
} MutualTouchInfo_t;

typedef struct {
    u16 nFirmwareMode;
    u8 nType;
    u8 nLogModePacketHeader;
    u8 nMy;
    u8 nMx;
    u8 nSd;
    u8 nSs;
    u16 nLogModePacketLength;
} MutualFirmwareInfo_t;

typedef struct {
    u16 nSwId;

#ifdef CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY
    u8 **pUpdateBin;
#else               // ONE DIMENSIONAL ARRAY
    u8 *pUpdateBin;
#endif              //CONFIG_UPDATE_FIRMWARE_BY_TWO_DIMENSIONAL_ARRAY

} SwIdData_t;

#ifdef CONFIG_ENABLE_JNI_INTERFACE
typedef struct {
    u64 nCmdId;
    u64 nSndCmdDataPtr; //send data to fw
    u64 nSndCmdLen;
    u64 nRtnCmdDataPtr; //receive data from fw
    u64 nRtnCmdLen;
} MsgToolDrvCmd_t;
#endif //CONFIG_ENABLE_JNI_INTERFACE

#define EMEM_SIZE_MSG28XX (1024*130)
#define EMEM_SIZE_MSG22XX ((1024*48) + 512 )
#define EMEM_TYPE_ALL_BLOCKS    0x00
#define EMEM_TYPE_MAIN_BLOCK    0x01
#define EMEM_TYPE_INFO_BLOCK    0x02
#define ROI_DATA_LENGTH 98

//for tpkit mp test
#define MSTAR_CONNECT_TEST_PASS     "0P-"
#define MSTAR_CONNECT_TEST_FAIL     "0F-"
#define MSTAR_ALLNODE_TEST_PASS     "1P-"
#define MSTAR_ALLNODE_TEST_FAIL     "1F-"
#define MSTAR_OPEN_PASS             "2P-"
#define MSTAR_OPEN_FAIL             "2F-"
#define MSTAR_SHORT_TEST_PASS       "3P-"
#define MSTAR_SHORT_TEST_FAIL       "3F-"
#define MSTAR_UNIFORMITY_TEST_PASS  "4P-"
#define MSTAR_UNIFORMITY_TEST_FAIL  "4F-"
#define MSTAR_MP_TEST_END           ";"

#define ERR_ALLOC_MEM(X)    ((IS_ERR(X) || X == NULL) ? 1 : 0)

struct mstar_gesture_data{
	u8 wakeup_packet[GESTURE_WAKEUP_PACKET_LENGTH_MAX];
	u32 log_info[GESTURE_WAKEUP_INFORMATION_PACKET_LENGTH];
	bool wakeup_flag;

	u32 wakeup_mode[2];

};

struct mstar_fw_data{
	bool support_seg;
	u16 packet_flag_addr;
	u16 packet_data_addr;
	u8 packet_flag[2];
	u8 *platform_inter_ver; // internal use firmware version for MStar
	u32 platform_ver[3];

	u8 ver_flag;       // 0: old 1:new ,platform FW version V01.010.03
	char cust_ver[32];   // customer firmware version
};


struct apk_node_status {
    u8 g_IsEnableLeatherSheathMode;
    u8 g_IsEnableGloveMode;
    bool g_GestureState;
};

struct apk_data_info{
	struct apk_node_status ans;
	u8 firmware_log_enable;
	u8 firmware_special_log_enable;
	u8 self_freq_scan_enable;
	u8 force_update_firmware_enable;
	u8 disable_esd_protection_check;
       u8 switch_mode;
       u8 log_gesture_info_type;
       MsgToolDrvCmd_t *msg_tool_cmd_in;
       u8 *snd_cmd_data;
       u8 *rtn_cmd_data;
       u32 debug_reg_count;
       char debug_buf[1024];
       u16 debug_reg[MAX_DEBUG_REGISTER_NUM];
       u16 debug_reg_value[MAX_DEBUG_REGISTER_NUM];
       u8 debug_log_time_stamp;
       u32 feature_support_status;
       u8 debug_cmd_argu[MAX_DEBUG_COMMAND_ARGUMENT_NUM];
       u16 debug_cmd_arg_count;
       u32 debug_read_data_size;
       u8 update_complete;
       u8 gesture_wakeup_enable;
       u8 gesture_debug_mode_enable;
       u8 gesture_information_mode_enable;
       struct kset *gesture_kset;
       struct kobject *gesture_kobj;
       struct kset *touch_kset;
       struct kobject *touch_kobj;

	unsigned char **fw_debug_buf;
	struct mutex debug_mutex;
	struct mutex debug_read_mutex;
	bool debug_node_open;
	int debug_data_frame;
	wait_queue_head_t inq;

};

struct firmware_update_data{
	SwIdData_t swid_data;
      u8 update_retry_cont;
      u8 *one_dimen_fw_data;
      u8 **two_dimen_fw_data;
      u8 *fw_data_buf;
      u32 fw_data_cont;
      u16 sfr_addr3_byte0_to_1;
      u16 sfr_addr3_byte2_to_3;
};

struct mstar_core_data{
	struct device *dev;
	struct ts_kit_device_data *mstar_chip_data;
	struct platform_device *mstar_dev;
	struct input_dev *input;
	struct regulator *vddd;
	struct regulator *vdda;

	int pinctrl_set;
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
	int self_ctrl_power;

	u16 vendor_id;
	char project_id [MSTAR_PROJECT_ID_LEN + 1];
	char lcd_panel_info[LCD_PANEL_INFO_MAX_LEN] ;
	char lcd_module_name[MAX_STR_LEN];
	bool support_get_tp_color;/*for tp color */
	int lcd_panel_name_from_lcdkit;//0 : lcd name from others ,1 : lcd name from lcdkit_tp.c
	int fw_only_depend_on_lcd ;//0 : fw depend on TP and others ,1 : fw only depend on lcd.

	bool fw_updating;
	bool esd_check;
	bool mp_testing;
	bool finger_touch_disable;
 	u16 fw_mode;
	bool esd_enable;
	bool int_flag;
	u8 roi_enabled;
	int avoid_roi_switch_flag;
	u16 chip_type;
	u16 chip_type_ori;
	spinlock_t irq_lock;
       u8 demo_packet[MUTUAL_DEMO_MODE_PACKET_LENGTH];
       u8 debug_packet[MUTUAL_DEBUG_MODE_PACKET_LENGTH] ;
       u8 log_packet[MUTUAL_DEBUG_MODE_PACKET_LENGTH];
	unsigned char roi_data[ROI_DATA_LENGTH + 4];
	unsigned char roi_data_send[ROI_DATA_LENGTH + 4];

       MutualFirmwareInfo_t mutual_fw_info;

      struct mutex mutex_common;
      struct mutex mutex_protect;
	uint8_t *i2cuart_data;
	uint16_t i2cuart_len;

	struct mstar_gesture_data gesture_data;
	struct mstar_fw_data fw_data;
	struct apk_data_info *apk_info;
       struct firmware_update_data fw_update_data;

	bool ges_self_debug;
	bool reset_touch_flag;
};

extern struct mstar_core_data *tskit_mstar_data;

#define ERROR_CODE_SUCCESS 0
#define ERROR_I2C_READ -1
#define ERROR_I2C_WRITE -2
#define ALLOCATE_ERROR  -3
extern u16 mstar_get_reg_16bit(u16 nAddr);
extern u8 mstar_get_reg_low_byte(u16 nAddr);
extern int mstar_get_reg_xbit(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit);
extern s32 mstar_set_reg_16bit(u16 nAddr, u16 nData);
extern int mstar_set_reg_low_byte(u16 nAddr, u8 nData);
extern int mstar_set_reg_16bit_on(u16 nAddr, u16 nData);
extern int mstar_set_reg_16bit_off(u16 nAddr, u16 nData);
extern u16 mstar_get_reg_16bit_by_addr(u16 nAddr, AddressMode_e eAddressMode);
extern s32 mstar_dbbus_enter_serial_debug(void);
extern int mstar_dbbus_exit_serial_debug(void);
extern int mstar_dbbus_i2c_response_ack(void);
extern int mstar_dbbus_iic_use_bus(void);
extern int mstar_dbbus_iic_not_use_bus(void);
extern int mstar_dbbus_iic_reshape(void);
extern int mstar_dbbus_stop_mcu(void);
extern int mstar_dbbus_not_stop_mcu(void);
extern int mstar_dbbus_wait_mcu(void);
extern s32 mstar_iic_write_data(u8 nSlaveId, u8 * pBuf, u16 nSize);
extern s32 mstar_iic_read_data(u8 nSlaveId, u8 * pBuf, u16 nSize);

extern void mstar_dev_hw_reset(void);
extern void mstar_finger_touch_report_enable(void);
extern void mstar_finger_touch_report_disable(void);
extern int mstar_get_customer_fw_ver(char *ppVersion);

extern int mstar_reg_get_xbit_write_4byte_value(u16 nAddr, u8 * pRxData, u16 nLength, u16 nMaxI2cLengthLimit);

extern int mstar_get_raw_data(struct ts_rawdata_info *info, struct ts_cmd_node *out_cmd);

/* important vars externed from main.c */
extern u16 mstar_change_fw_mode(u16 nMode);
extern int mstar_get_touch_packet_addr(u16 * pDataAddress, u16 * pFlagAddress);
extern int mstar_mutual_get_fw_info(MutualFirmwareInfo_t * pInfo);
extern int mstar_enter_sleep_mode(void);
extern s32 mstar_update_fw_sdcard(const char *pFilePath);
extern s32 mstar_update_fw(u8 ** szFwData, EmemType_e eEmemType);
extern u32 mstar_convert_char_to_hex_digit(char *pCh, u32 nLength);
extern int mstar_get_platform_fw_ver(u8 ** ppVersion);

/* Externed from update.c */
extern void mstar_erase_emem(EmemType_e eEmemType);
extern void mstar_write_dq_mem_value(u16 nAddr, u32 nData);
extern u32 mstar_read_dq_mem_value(u16 nAddr);
extern s32 mstar_fw_update_sdcard(const char *pFilePath, u8 mode);
extern u16 mstar_get_swid(EmemType_e eEmemType);
extern u16 mstar_get_chip_type(void);
extern s32 mstar_update_fw_cash(u8 ** szFwData, EmemType_e eEmemType);

extern void mstar_fw_update_swid_entry(void);
extern char *trans_lcd_panel_name_to_tskit(void);

#endif /* __MSTAR_COMMON_H__ */
