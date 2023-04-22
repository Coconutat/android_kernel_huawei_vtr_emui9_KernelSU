

#ifndef __CHR_ERRNO_H__
#define __CHR_ERRNO_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/

/*****************************************************************************
  2 宏定义
*****************************************************************************/

#define CHR_SYSTEM_OFFSET                   (19)
#define CHR_LAYER_OFFSET                    (16)
#define CHR_EVENT_OFFSET                    (8)
#define CHR_ERROR_OFFSET                    (0)
#define CHR_SYSTEM_MASK                     (0x1F)
#define CHR_LAYER_MASK                      (0x07)
#define CHR_EVENT_MASK                      (0xFF)
#define CHR_ERROR_MASK                      (0xFF)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/****************************************************************************
                        CHR ERRNO 错误码结构
|---------------------------------------------------------------------------|
| 31~24 (8)bits| 23~19 (5)bits | 18~16 (3)bits | 15~8 (8)bits | 7~0 (8)bits |
|---------------------------------------------------------------------------|
|   Reserved    |    system     |     layer     |    event    |   error     |
|---------------------------------------------------------------------------|

|---------------------------------------------------------------------------|
|   位段   |      取值       |                    用途                      |
|---------------------------------------------------------------------------|
| Reserved |      保留       |                    保留                      |
|---------------------------------------------------------------------------|
| system   | CHR_SYSTEM_ENUM | 标识异常子系统 plat wifi gnss bt fm nfc ir   |
|---------------------------------------------------------------------------|
| layer    | CHR_LAYER_ENUM  | 标识异常所处位置   dev      drv      hal     |
|---------------------------------------------------------------------------|
| event    |CHR_XX_EVENT_ENUM| 标识异常触发事件                             |
|---------------------------------------------------------------------------|
| error    |CHR_XX_ERROR_ENUM| 标识异常触发因素,详细描述触发异常事件的原因  |
|---------------------------------------------------------------------------|
*****************************************************************************/

typedef enum
{
    CHR_SYSTEM_PLAT    = 0x01 << CHR_SYSTEM_OFFSET,
    CHR_SYSTEM_WIFI    = 0x02 << CHR_SYSTEM_OFFSET,
    CHR_SYSTEM_GNSS    = 0x03 << CHR_SYSTEM_OFFSET,
    CHR_SYSTEM_BT      = 0x04 << CHR_SYSTEM_OFFSET,
    CHR_SYSTEM_FM      = 0x05 << CHR_SYSTEM_OFFSET,
    CHR_SYSTEM_NFC     = 0x06 << CHR_SYSTEM_OFFSET,
    CHR_SYSTEM_IR      = 0x07 << CHR_SYSTEM_OFFSET,
    CHR_SYSTEM_MUTT    = 0x08 << CHR_SYSTEM_OFFSET,

}CHR_SYSTEM_ENUM;

typedef enum
{
    CHR_LAYER_DEV      = 0x01 << CHR_LAYER_OFFSET,
    CHR_LAYER_DRV      = 0x02 << CHR_LAYER_OFFSET,
    CHR_LAYER_HAL      = 0x03 << CHR_LAYER_OFFSET,

}CHR_LAYER_ENUM;

typedef enum
{
    /* CHR_WIFI_HAL */
    CHR_WIFI_HAL_EVENT_OPEN               = 0x01,
    CHR_WIFI_HAL_EVENT_SCAN               = 0x02,
    CHR_WIFI_HAL_EVENT_CONNECT            = 0x03,

    /* CHR_WIFI_DRV */
    CHR_WIFI_DRV_EVENT_OPEN               = 0x01,
    CHR_WIFI_DRV_EVENT_SCAN               = 0x02,
    CHR_WIFI_DRV_EVENT_CONNECT            = 0x03,
    CHR_WIFI_DRV_EVENT_DISCONNECT         = 0x04,
    CHR_WIFI_DRV_EVENT_CLOSE              = 0x05,
    CHR_WIFI_DRV_EVENT_EXCEPTION          = 0x06,
    CHR_WIFI_DRV_EVENT_FAKELINK           = 0x07,

    /* CHR_WIFI_DRV platform event */
    CHR_WIFI_DRV_EVENT_PLAT               = 0x08,


    /* CHR_WIFI_DEV */
    CHR_WIFI_DEV_EVENT_CHIP               = 0x01,
    CHR_WIFI_DRV_ANTS_MSW_FAILED          = 0x02,
    CHR_WIFI_DRV_ANTS_SWM_FAILED          = 0x03,
    CHR_WIFI_DEV_EVENT_ERROR_LOGS         = 0x04,


}CHR_WIFI_EVENT_ENUM;

typedef enum
{
    /* CHR_GNSS_HAL */
    CHR_GNSS_HAL_EVENT_SYSCALL            = 0x01,
    CHR_GNSS_HAL_EVENT_EXCEPTION          = 0x02,
    CHR_GNSS_HAL_EVENT_TIME               = 0x03,
    CHR_GNSS_HAL_EVENT_INJECT             = 0x04,

    /* CHR_GNSS_DRV platform event */
    CHR_GNSS_DRV_EVENT_PLAT               = 0x05,

}CHR_GNSS_EVENT_ENUM;

typedef enum
{
    /* CHR_WIFI_HAL_EVENT_OPEN */
    CHR_WIFI_HAL_ERROR_CONFIG_READ             = 0x01,
    CHR_WIFI_HAL_ERROR_SOCKET_FAIL             = 0x02,
    CHR_WIFI_HAL_ERROR_AP_CONFIG_SET_FAT       = 0x03,

    /* CHR_WIFI_HAL_EVENT_SCAN */
    CHR_WIFI_HAL_ERROR_SCAN_TIME_OUT           = 0x01,

    /* CHR_WIFI_HAL_EVENT_CONNECT */
    CHR_WIFI_HAL_ERROR_CMD_SEND_FAIL           = 0x01,
    CHR_WIFI_HAL_ERROR_CONNECT_FAIL            = 0x02,
    CHR_WIFI_HAL_ERROR_DISCONNECT_EVENT_RECV   = 0x03,
    CHR_WIFI_HAL_ERROR_WPS_FAIL                = 0x04,
    CHR_WIFI_HAL_ERROR_MODE_CHANGE_FAIL        = 0x05,

    /* CHR_WIFI_DRV_EVENT_OPEN */
    CHR_WIFI_DRV_ERROR_INSMOD_KO               = 0x01,
    CHR_WIFI_DRV_ERROR_SYS_VERSION             = 0x02,
    CHR_WIFI_DRV_ERROR_POWER_ON                = 0x03,
    CHR_WIFI_DRV_ERROR_CUSTOM_CALL             = 0x04,

    /* CHR_WIFI_DRV_EVENT_SCAN */
    CHR_WIFI_DRV_ERROR_SCAN_REFUSED            = 0x01,
    CHR_WIFI_DRV_ERROR_SCAN_TIMEOUT            = 0x02,
    CHR_WIFI_DRV_ERROR_SCAN_ZERO               = 0x03,

    /* CHR_WIFI_DRV_EVENT_CONNECT */
    CHR_WIFI_DRV_ERROR_CONNECT_CMD             = 0x01,
    CHR_WIFI_DRV_ERROR_AUTH_TIMEOUT            = 0x02,
    CHR_WIFI_DRV_ERROR_ASSOC_TIMEOUT           = 0x03,
    CHR_WIFI_DRV_ERROR_AUTH_REJECTED           = 0x04,
    CHR_WIFI_DRV_ERROR_ASSOC_REJECTED          = 0x05,

    /* CHR_WIFI_DRV_EVENT_DISCONNECT */
    CHR_WIFI_DRV_ERROR_LINKLOSS                = 0x01,
    CHR_WIFI_DRV_ERROR_KEEPALIVE_TIMEOUT       = 0x02,
    CHR_WIFI_DRV_ERROR_CHANNEL_CHANGE          = 0x03,

    /* CHR_WIFI_DRV_EVENT_CLOSE */
    CHR_WIFI_DRV_ERROR_POWER_OFF               = 0x01,

    /* CHR_WIFI_DRV_EVENT_EXCEPTION */
    CHR_WIFI_DRV_ERROR_BEAT_HEART_TIMEOUT      = 0x01,
    CHR_WIFI_DRV_ERROR_WATCHDOG_TIMEOUT        = 0x02,
    CHR_WIFI_DRV_ERROR_WAKEUP_FAIL             = 0x03,
    CHR_WIFI_DRV_ERROR_DEVICE_PANIC            = 0x04,
    CHR_WIFI_DRV_ERROR_SDIO_TRANS_FAIL         = 0x05,

    /* CHR_WIFI_DRV_EVENT_FAKELINK */
    CHR_WIFI_DRV_ERROR_RX_NO_BUFFER            = 0x01,
    CHR_WIFI_DRV_ERROR_ARP_TX_FAIL             = 0x02,
    CHR_WIFI_DRV_ERROR_EAPOL_TX_FAIL           = 0x03,
    CHR_WIFI_DRV_ERROR_DHCP_TX_FAIL            = 0x04,
    CHR_WIFI_DRV_ERROR_RF_OVERHEAT_EXCEPTION   = 0x05,
    CHR_WIFI_DRV_ERROR_INTERFERENCE            = 0x06,

    /* CHR_WIFI_DRV_EVENT_CHIP */

    /* CHR_WIFI_DEV_EVENT_CHIP */
    CHR_WIFI_DEV_ERROR_FEM_FAIL                = 0x01,
    CHR_WIFI_DEV_ERROR_32K_CLK                 = 0x02,
    CHR_WIFI_DEV_ERROR_GPIO                    = 0x03,
    CHR_WIFI_DEV_ERROR_SDIO_ENUM               = 0x04,
    CHR_WIFI_DEV_ERROR_IOMUX                   = 0x05,
    CHR_WIFI_DEV_ERROR_UART                    = 0x06,

    /* CHR_WIFI_DRV_ANTS_MSW_FAILED */
    CHR_WIFI_DEV_ERROR_MSW_MODEM_PREEMPTIVE    = 0x01,
    CHR_WIFI_DEV_ERROR_MSW_DEVICE_EXCEPTION    = 0x02,
    CHR_WIFI_DEV_ERROR_MSW_ALG_EXCEPTION       = 0x03,
    CHR_WIFI_DEV_ERROR_MSW_UNKNOWN_ERROR       = 0x04,

    /* CHR_WIFI_DRV_ANTS_SWM_FAILED */
    CHR_WIFI_DEV_ERROR_SWM_MODEM_PREEMPTIVE    = 0x01,
    CHR_WIFI_DEV_ERROR_SWM_DEVICE_EXCEPTION    = 0x02,
    CHR_WIFI_DEV_ERROR_SWM_ALG_EXCEPTION       = 0x03,
    CHR_WIFI_DEV_ERROR_SWM_UNKNOWN_ERROR       = 0x04,

    /* CHR_WIFI_DEV_EVENT_ERROR_LOGS */
    CHR_WIFI_DEV_ERROR_SCAN_LOGS               = 0x01,
    CHR_WIFI_DEV_ERROR_AUTH_LOGS               = 0x02,
    CHR_WIFI_DEV_ERROR_ASSOC_LOGS              = 0x03,
    CHR_WIFI_DEV_ERROR_FRAME_FILTER_LOGS       = 0x04,
    CHR_WIFI_DEV_ERROR_WMM_LOGS                = 0x05,
    CHR_WIFI_DEV_ERROR_DFS_LOGS                = 0x06,
    CHR_WIFI_DEV_ERROR_NETWORK_MEASURE_LOGS    = 0x07,
    CHR_WIFI_DEV_ERROR_ENTERPRISE_VO_LOGS      = 0x08,
    CHR_WIFI_DEV_ERROR_HOTSPOTROAM_LOGS        = 0x09,
    CHR_WIFI_DEV_ERROR_NETWROK_ANNOUNCE_LOGS   = 0x0A,
    CHR_WIFI_DEV_ERROR_NETWORK_MGMT_LOGS       = 0x0B,
    CHR_WIFI_DEV_ERROR_NETWORK_PWS_LOGS        = 0x0C,
    CHR_WIFI_DEV_ERROR_PROXYARP_LOGS           = 0x0D,
    CHR_WIFI_DEV_ERROR_TDLS_LOGS               = 0x0E,
    CHR_WIFI_DEV_ERROR_CALIBRATE_LOGS          = 0x0F,
    CHR_WIFI_DEV_ERROR_EQUIP_TEST_LOGS         = 0x10,
    CHR_WIFI_DEV_ERROR_CRYPTO_LOGS             = 0x11,
    CHR_WIFI_DEV_ERROR_WPA_LOGS                = 0x12,
    CHR_WIFI_DEV_ERROR_WEP_LOGS                = 0x13,
    CHR_WIFI_DEV_ERROR_WPS_LOGS                = 0x14,
    CHR_WIFI_DEV_ERROR_PMF_LOGS                = 0x15,
    CHR_WIFI_DEV_ERROR_WAPI_LOGS               = 0x16,
    CHR_WIFI_DEV_ERROR_BA_LOGS_LOGS            = 0x17,
    CHR_WIFI_DEV_ERROR_AMPDU_LOGS              = 0x18,
    CHR_WIFI_DEV_ERROR_AMSDU_LOGS              = 0x19,
    CHR_WIFI_DEV_ERROR_STABILITY_LOGS          = 0x1A,
    CHR_WIFI_DEV_ERROR_TCP_OPT_LOGS            = 0x1B,
    CHR_WIFI_DEV_ERROR_ACS_LOGS                = 0x1C,
    CHR_WIFI_DEV_ERROR_AUTORATE_LOGS           = 0x1D,
    CHR_WIFI_DEV_ERROR_TXBF_LOGS               = 0x1E,
    CHR_WIFI_DEV_ERROR_DYN_RECV_LOGS           = 0x1F,
    CHR_WIFI_DEV_ERROR_VIVO_LOGS               = 0x20,
    CHR_WIFI_DEV_ERROR_MULTI_USER_LOGS         = 0x21,
    CHR_WIFI_DEV_ERROR_MULTI_TRAFFIC_LOGS      = 0x22,
    CHR_WIFI_DEV_ERROR_ANTI_INTF_LOGS          = 0x23,
    CHR_WIFI_DEV_ERROR_EDCA_LOGS               = 0x24,
    CHR_WIFI_DEV_ERROR_SMART_ANTENNA_LOGS      = 0x25,
    CHR_WIFI_DEV_ERROR_TPC_LOGS                = 0x26,
    CHR_WIFI_DEV_ERROR_TX_CHAIN_LOGS           = 0x27,
    CHR_WIFI_DEV_ERROR_RSSI_LOGS               = 0x28,
    CHR_WIFI_DEV_ERROR_WOW_LOGS                = 0x29,
    CHR_WIFI_DEV_ERROR_GREEN_AP_LOGS           = 0x2A,
    CHR_WIFI_DEV_ERROR_PWR_LOGS                = 0x2B,
    CHR_WIFI_DEV_ERROR_SMPS_LOGS               = 0x2C,
    CHR_WIFI_DEV_ERROR_TXOP_LOGS               = 0x2D,
    CHR_WIFI_DEV_ERROR_WIFI_BEACON_LOGS        = 0x2E,
    CHR_WIFI_DEV_ERROR_KA_AP_LOGS              = 0x2F,
    CHR_WIFI_DEV_ERROR_MULTI_VAP_LOGS          = 0x30,
    CHR_WIFI_DEV_ERROR_2040_LOGS               = 0x31,
    CHR_WIFI_DEV_ERROR_DBAC_LOGS               = 0x32,
    CHR_WIFI_DEV_ERROR_PROXYSTA_LOGS           = 0x33,
    CHR_WIFI_DEV_ERROR_UM_LOGS                 = 0x34,
    CHR_WIFI_DEV_ERROR_P2P_LOGS                = 0x35,
    CHR_WIFI_DEV_ERROR_M2U_LOGS                = 0x36,
    CHR_WIFI_DEV_ERROR_IRQ_LOGS                = 0x37,
    CHR_WIFI_DEV_ERROR_TX_LOGS                 = 0x38,
    CHR_WIFI_DEV_ERROR_RX_LOGS                 = 0x39,
    CHR_WIFI_DEV_ERROR_DUG_COEX_LOGS           = 0x3A,
    CHR_WIFI_DEV_ERROR_CFG_LOGS                = 0x3B,
    CHR_WIFI_DEV_ERROR_FRW_LOGS                = 0x3C,
    CHR_WIFI_DEV_ERROR_KEEPALIVE_LOGS          = 0x3D,
    CHR_WIFI_DEV_ERROR_COEX_LOGS               = 0x3E,
    CHR_WIFI_DEV_ERROR_HS20_LOGS               = 0x3F,
    CHR_WIFI_DEV_ERROR_MWO_DET_LOGS            = 0x40,
    CHR_WIFI_DEV_ERROR_CCA_OPT_LOGS            = 0x41,
    CHR_WIFI_DEV_ERROR_ROAM_LOGS               = 0x42,
    CHR_WIFI_DEV_ERROR_DFT_LOGS                = 0x43,
    CHR_WIFI_DEV_ERROR_DFR_LOGS                = 0x44,
    CHR_WIFI_DEV_ERROR_ANY_LOGS                = 0x45,

}CHR_WIFI_ERROR_ENUM;


typedef enum
{
    /*CHR_GNSS_HAL_EVENT_SYSCALL*/
    CHR_GNSS_HAL_ERROR_OPEN_CMD           = 0x01,
    CHR_GNSS_HAL_ERROR_READ_CMD           = 0x02,
    CHR_GNSS_HAL_ERROR_WRITE_CMD          = 0x03,
    CHR_GNSS_HAL_ERROR_FLUSH_CMD          = 0x04,
    CHR_GNSS_HAL_ERROR_CLOSE_CMD          = 0x05,
    CHR_GNSS_HAL_ERROR_CHMOD_CMD          = 0x06,
    CHR_GNSS_HAL_ERROR_SOCKET_CREATE_CMD  = 0x07,
    CHR_GNSS_HAL_ERROR_SOCKET_CONNECT_CMD = 0x08,
    CHR_GNSS_HAL_ERROR_PIPE_CREATE_CMD    = 0x09,
    CHR_GNSS_HAL_ERROR_EPOLL_REGISTER_CMD = 0x0A,
    CHR_GNSS_HAL_ERROR_EPOLL_HUP_CMD      = 0x0B,
    CHR_GNSS_HAL_ERROR_THREAD_CREATE_CMD  = 0x0C,

    /*CHR_GNSS_HAL_EVENT_EXCEPTION*/
    CHR_GNSS_HAL_ERROR_REBOOT_CMD         = 0x01,
    CHR_GNSS_HAL_ERROR_TIMEOUT_CMD        = 0x02,
    CHR_GNSS_HAL_ERROR_DATA_LOST_CMD      = 0x03,
    CHR_GNSS_HAL_ERROR_DATA_WRONG_CMD     = 0x04,
    CHR_GNSS_HAL_ERROR_ACK_LOST_CMD       = 0x05,

    /*CHR_GNSS_HAL_EVENT_TIME*/
    CHR_GNSS_HAL_ERROR_RTC_CMD            = 0x01,

    /*CHR_GNSS_HAL_EVENT_INJECT*/
    CHR_GNSS_HAL_ERROR_TIME_INJECT_CMD    = 0x01,
    CHR_GNSS_HAL_ERROR_LOC_INJECT_CMD     = 0x02,
    CHR_GNSS_HAL_ERROR_EPH_INJECT_CMD     = 0x03,

}CHR_GNSS_ERROR_ENUM;

typedef enum
{
    CHR_PLAT_DRV_ERROR_FIRMWARE_DOWN      = 0x01,
    CHR_PLAT_DRV_ERROR_SDIO_INIT          = 0x02,
    CHR_PLAT_DRV_ERROR_OPEN_UART          = 0x03,
    CHR_PLAT_DRV_ERROR_CFG_UART           = 0x04,
    CHR_PLAT_DRV_ERROR_OPEN_BCPU          = 0x05,
    CHR_PLAT_DRV_ERROR_BCPU_BOOTUP        = 0x06,
    CHR_PLAT_DRV_ERROR_CLOSE_BCPU         = 0x07,
    CHR_PLAT_DRV_ERROR_OPEN_WCPU          = 0x08,
    CHR_PLAT_DRV_ERROR_WCPU_BOOTUP        = 0x09,
    CHR_PLAT_DRV_ERROR_CLOSE_WCPU         = 0x0A,
    CHR_PLAT_DRV_ERROR_OPEN_THREAD        = 0x0B,
    CHR_PLAT_DRV_ERROR_CLOSE_THREAD       = 0x0C,
    CHR_PLAT_DRV_ERROR_WAKEUP_DEV         = 0x0D,
    CHR_PLAT_DRV_ERROR_RECV_LASTWORD      = 0x0E,
    CHR_PLAT_DRV_ERROR_BEAT_TIMEOUT       = 0x0F,

}CHR_PLAT_ERROR_ENUM;

/************************ CHR ERRNO 错误码各位段组装 *************************/
#define CHR_COMBINE_EVENT_ERROR(event, error)   \
    (((event & CHR_EVENT_MASK) << CHR_EVENT_OFFSET) | ((error & CHR_ERROR_MASK) << CHR_ERROR_OFFSET))

#define CHR_PLAT_DEV(event,error) \
    (CHR_SYSTEM_PLAT | CHR_LAYER_DEV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_PLAT_DRV(event,error) \
    (CHR_SYSTEM_PLAT | CHR_LAYER_DRV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_PLAT_HAL(event,error) \
    (CHR_SYSTEM_PLAT | CHR_LAYER_HAL | CHR_COMBINE_EVENT_ERROR(event, error))

#define CHR_WIFI_DEV(event,error) \
    (CHR_SYSTEM_WIFI | CHR_LAYER_DEV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_WIFI_DRV(event,error) \
    (CHR_SYSTEM_WIFI | CHR_LAYER_DRV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_WIFI_HAL(event,error) \
    (CHR_SYSTEM_WIFI | CHR_LAYER_HAL | CHR_COMBINE_EVENT_ERROR(event, error))

#define CHR_GNSS_DEV(event,error) \
    (CHR_SYSTEM_GNSS | CHR_LAYER_DEV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_GNSS_DRV(event,error) \
    (CHR_SYSTEM_GNSS | CHR_LAYER_DRV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_GNSS_HAL(event,error) \
    (CHR_SYSTEM_GNSS | CHR_LAYER_HAL | CHR_COMBINE_EVENT_ERROR(event, error))

#define CHR_BT_DEV(event,error) \
    (CHR_SYSTEM_BT | CHR_LAYER_DEV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_BT_DRV(event,error) \
    (CHR_SYSTEM_BT | CHR_LAYER_DRV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_BT_HAL(event,error) \
    (CHR_SYSTEM_BT | CHR_LAYER_HAL | CHR_COMBINE_EVENT_ERROR(event, error))

#define CHR_FM_DEV(event,error) \
    (CHR_SYSTEM_FM | CHR_LAYER_DEV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_FM_DRV(event,error) \
    (CHR_SYSTEM_FM | CHR_LAYER_DRV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_FM_HAL(event,error) \
    (CHR_SYSTEM_FM | CHR_LAYER_HAL | CHR_COMBINE_EVENT_ERROR(event, error))

#define CHR_NFC_DEV(event,error) \
    (CHR_SYSTEM_NFC | CHR_LAYER_DEV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_NFC_DRV(event,error) \
    (CHR_SYSTEM_NFC | CHR_LAYER_DRV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_NFC_HAL(event,error) \
    (CHR_SYSTEM_NFC | CHR_LAYER_HAL | CHR_COMBINE_EVENT_ERROR(event, error))

#define CHR_IR_DEV(event,error) \
    (CHR_SYSTEM_IR | CHR_LAYER_DEV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_IR_DRV(event,error) \
    (CHR_SYSTEM_IR | CHR_LAYER_DRV | CHR_COMBINE_EVENT_ERROR(event, error))
#define CHR_IR_HAL(event,error) \
    (CHR_SYSTEM_IR | CHR_LAYER_HAL | CHR_COMBINE_EVENT_ERROR(event, error))

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of chr_errno.h */
