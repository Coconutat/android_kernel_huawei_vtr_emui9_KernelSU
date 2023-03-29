

#ifndef __WAL_LINUX_IOCTL_H__
#define __WAL_LINUX_IOCTL_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_types.h"
#include "wlan_spec.h"
#include "mac_vap.h"
#include "hmac_ext_if.h"
#include "wal_ext_if.h"
#include "wal_config.h"
//#include "dmac_alg_if.h"

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#ifdef CONFIG_DOZE_FILTER
#include <huawei_platform/power/wifi_filter/wifi_filter.h>
#endif /* CONFIG_DOZE_FILTER */
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_IOCTL_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define WAL_HIPRIV_CMD_MAX_LEN       (WLAN_MEM_LOCAL_SIZE2 - 4)     /* 私有配置命令字符串最大长度，对应本地内存池一级大小 */

#define WAL_HIPRIV_CMD_NAME_MAX_LEN     80                          /* 字符串中每个单词的最大长度(原20) */
#define WAL_HIPRIV_CMD_VALUE_MAX_LEN    10                          /* 字符串中某个对应变量取值的最大位数 */

#define WAL_HIPRIV_PROC_ENTRY_NAME   "hipriv"

#define WAL_SIOCDEVPRIVATE              0x89F0  /* SIOCDEVPRIVATE */

#define WAL_HIPRIV_HT_MCS_MIN           0
#define WAL_HIPRIV_HT_MCS_MAX           32
#define WAL_HIPRIV_VHT_MCS_MIN          0
#define WAL_HIPRIV_VHT_MCS_MAX          11
#define WAL_HIPRIV_HE_MCS_MIN          0
#define WAL_HIPRIV_HE_MCS_MAX          11
#define WAL_HIPRIV_NSS_MIN              1
#define WAL_HIPRIV_NSS_MAX              4
#define WAL_HIPRIV_CH_NUM               4

#define WAL_HIPRIV_BOOL_NIM             0
#define WAL_HIPRIV_BOOL_MAX             1
#define WAL_HIPRIV_FREQ_SKEW_ARG_NUM    8
#define WAL_PHY_DEBUG_TEST_WORD_CNT     5           /* trailer上报个数 */

#define WAL_HIPRIV_MS_TO_S                   1000   /* ms和s之间倍数差 */
#define WAL_HIPRIV_KEEPALIVE_INTERVAL_MIN    5000   /* 受默认老化计数器出发时间所限制 */
#define WAL_HIPRIV_KEEPALIVE_INTERVAL_MAX    0xffff /* timer间隔时间限制所致(oal_uin16) */

#define WAL_IWPRIV_CAP_NUM              14          /* ???vap????????? */
#define WAL_IWPRIV_IGMP_MIN_LEN         50

/* 用户pwr ref reg的定制化保护阈值 */
#define WAL_HIPRIV_PWR_REF_DELTA_HI         40
#define WAL_HIPRIV_PWR_REF_DELTA_LO         -40


/* IOCTL私有配置命令宏定义 */
#define WAL_IOCTL_PRIV_SETPARAM          (OAL_SIOCIWFIRSTPRIV + 0)
#define WAL_IOCTL_PRIV_GETPARAM          (OAL_SIOCIWFIRSTPRIV + 1)
#define WAL_IOCTL_PRIV_SET_WMM_PARAM     (OAL_SIOCIWFIRSTPRIV + 3)
#define WAL_IOCTL_PRIV_GET_WMM_PARAM     (OAL_SIOCIWFIRSTPRIV + 5)
#define WAL_IOCTL_PRIV_SET_COUNTRY       (OAL_SIOCIWFIRSTPRIV + 8)
#define WAL_IOCTL_PRIV_GET_COUNTRY       (OAL_SIOCIWFIRSTPRIV + 9)

#define WAL_IOCTL_PRIV_GET_MODE     (OAL_SIOCIWFIRSTPRIV + 17)      /* 读取模式 */
#define WAL_IOCTL_PRIV_SET_MODE     (OAL_SIOCIWFIRSTPRIV + 13)      /* 设置模式 包括协议 频段 带宽 */

#define WAL_IOCTL_PRIV_AP_GET_STA_LIST               (OAL_SIOCIWFIRSTPRIV + 21)
#define WAL_IOCTL_PRIV_AP_MAC_FLTR                   (OAL_SIOCIWFIRSTPRIV + 22)
/* netd将此配置命令作为GET方式下发，get方式命令用奇数，set用偶数 */
#define WAL_IOCTL_PRIV_SET_AP_CFG                    (OAL_SIOCIWFIRSTPRIV + 23)
#define WAL_IOCTL_PRIV_AP_STA_DISASSOC               (OAL_SIOCIWFIRSTPRIV + 24)

#define WAL_IOCTL_PRIV_SET_MGMT_FRAME_FILTERS        (OAL_SIOCIWFIRSTPRIV + 28)      /* 设置管理帧过滤 */
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT_DEBUG
#define WAL_IOCTL_PRIV_SET_VENDOR_REQ                (OAL_SIOCIWFIRSTPRIV + 30)     /* HERA产品接口调试IOCTL命令字 */
#endif

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
#define WAL_IOCTL_PRIV_GET_BLACKLIST        (OAL_SIOCIWFIRSTPRIV + 27)
#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
#define WAL_IOCTL_PRIV_SET_ESSID            (OAL_SIOCIWFIRSTPRIV + 4)
#define WAL_IOCTL_PRIV_GET_STRU             (OAL_SIOCIWFIRSTPRIV + 7)
#define WAL_IOCTL_PRIV_SET_HWADDR           (OAL_SIOCIWFIRSTPRIV + 10)
#define WAL_IOCTL_PRIV_GET_PARAM_CHAR       (OAL_SIOCIWFIRSTPRIV + 11)
#define WAL_IOCTL_PRIV_SET_MAC              (OAL_SIOCIWFIRSTPRIV + 12)
#define WAL_IOCTL_PRIV_SET_PARAM_CHAR       (OAL_SIOCIWFIRSTPRIV + 15)
#define WAL_IOCTL_PRIV_GET_ESSID            (OAL_SIOCIWFIRSTPRIV + 19)
#define WAL_IOCTL_PRIV_GET_STA_STAT_INFO    (OAL_SIOCIWFIRSTPRIV + 25)
#endif
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
#define WAL_IOCTL_PRIV_SET_EQUIPMENT_PARAM        (OAL_SIOCIWFIRSTPRIV + 27)
#endif
#ifdef _PRE_WLAN_FEATURE_DFS
#define WAL_IOCTL_PRIV_GET_DFS_CHN_STAT      (OAL_SIOCIWFIRSTPRIV + 29)
#endif

#define WAL_IOCTL_PRIV_SUBCMD_MAX_LEN          20


#define WAL_VAP_FOREACH_USER(_pst_user, _pst_vap, _pst_list_pos)       \
             for ((_pst_list_pos) = (_pst_vap)->st_mac_user_list_head.pst_next,  \
                  (_pst_user) = OAL_DLIST_GET_ENTRY((_pst_list_pos), mac_user_stru, st_user_dlist);      \
                  (_pst_list_pos) != &((_pst_vap)->st_mac_user_list_head);                               \
                  (_pst_list_pos) = (_pst_list_pos)->pst_next,                                           \
                  (_pst_user) = OAL_DLIST_GET_ENTRY((_pst_list_pos), mac_user_stru, st_user_dlist))     \
                  if (OAL_PTR_NULL != (_pst_user))


typedef oal_uint32  (*wal_hipriv_cmd_func)(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

typedef enum
{
    WAL_DSCR_PARAM_PA_GAIN_LEVEL = 0,      /* pa增益等级 */
    WAL_DSCR_PARAM_MICRO_TX_POWER_GAIN_LEVEL,
    WAL_DSCR_PARAM_TXRTS_ANTENNA,
    WAL_DSCR_PARAM_RXCTRL_ANTENNA,
    WAL_DSCR_PARAM_CHANNAL_CODE,
    WAL_DSCR_PARAM_DATA_RATE0,
    WAL_DSCR_PARAM_DATA_RATE1,
    WAL_DSCR_PARAM_DATA_RATE2,
    WAL_DSCR_PARAM_DATA_RATE3,
    WAL_DSCR_PARAM_POWER,
    WAL_DSCR_PARAM_SHORTGI,
    WAL_DSCR_PARAM_PREAMBLE_MODE,
    WAL_DSCR_PARAM_RTSCTS,
    WAL_DSCR_PARAM_LSIGTXOP,
    WAL_DSCR_PARAM_SMOOTH,
    WAL_DSCR_PARAM_SOUNDING,
    WAL_DSCR_PARAM_TXBF,
    WAL_DSCR_PARAM_STBC,
    WAL_DSCR_PARAM_GET_ESS,
    WAL_DSCR_PARAM_DYN_BW,
    WAL_DSCR_PARAM_DYN_BW_EXIST,
    WAL_DSCR_PARAM_CH_BW_EXIST,
    WAL_DSCR_PARAM_RATE,
    WAL_DSCR_PARAM_MCS,
    WAL_DSCR_PARAM_MCSAC,
    WAL_DSCR_PARAM_MCSAX,
    WAL_DSCR_PARAM_NSS,
    WAL_DSCR_PARAM_BW,
    WAL_DSCR_PARAM_LTF,
    WLA_DSCR_PARAM_GI,
    WLA_DSCR_PARAM_TXCHAIN,

    WAL_DSCR_PARAM_BUTT
}wal_dscr_param_enum;

typedef oal_uint8 wal_dscr_param_enum_uint8;

/* rx ip数据包过滤功能和上层协定(格式)结构体，TBD 上层接口尚未明确，需澄清后修改 */
#ifdef CONFIG_DOZE_FILTER
typedef hw_wifi_filter_item wal_hw_wifi_filter_item;
typedef struct hw_wlan_filter_ops wal_hw_wlan_filter_ops;

#else
typedef struct {
    unsigned short protocol;      //协议类型
    unsigned short port;          //目的端口号
    unsigned int   filter_cnt;    //过滤报文数
}wal_hw_wifi_filter_item;

typedef struct {
    int (*set_filter_enable)(int);
    int (*add_filter_items)(wal_hw_wifi_filter_item*, int);
    int (*clear_filters)(void);
    int (*get_filter_pkg_stat)(wal_hw_wifi_filter_item*, int, int*);
}wal_hw_wlan_filter_ops;
#endif

typedef enum
{
    WAL_TX_POW_PARAM_SET_RF_REG_CTL = 0,
    WAL_TX_POW_PARAM_SET_FIX_LEVEL,
    WAL_TX_POW_PARAM_SET_MAG_LEVEL,
    WAL_TX_POW_PARAM_SET_CTL_LEVEL,
    WAL_TX_POW_PARAM_SET_AMEND,
    WAL_TX_POW_PARAM_SET_NO_MARGIN,
    WAL_TX_POW_PARAM_SET_SHOW_LOG,
    WAL_TX_POW_PARAM_SET_SAR_LEVEL,
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    WAL_TX_POW_PARAM_TAS_POW_CTRL,
    WAL_TX_POW_PARAM_TAS_RSSI_MEASURE,
    WAL_TX_POW_PARAM_TAS_ANT_SWITCH,
#endif

    WAL_TX_POW_PARAM_BUTT
}wal_tx_pow_param_enum;
typedef oal_uint8 wal_tx_pow_param_enum_uint8;

typedef enum
{
    WAL_AMPDU_DISABLE,
    WAL_AMPDU_ENABLE,

    WAL_AMPDU_CFG_BUTT
}wal_ampdu_idx_enum;

typedef oal_uint8 wal_ampdu_idx_enum_uint8;

/*wifi 能力开关状态枚举值，通知到上层使用*/
typedef enum
{
    WAL_WIFI_FEATURE_SUPPORT_11K                   = 0,
    WAL_WIFI_FEATURE_SUPPORT_11V                   = 1,
    WAL_WIFI_FEATURE_SUPPORT_11R                   = 2,
    WAL_WIFI_FEATURE_SUPPORT_VOWIFI_NAT_KEEP_ALIVE = 3,

    WAL_WIFI_FEATURE_SUPPORT_BUTT
}wal_wifi_feature_capbility_enum;



#define WAL_IOCTL_IS_INVALID_FIXED_RATE(_l_val, _pc_stu)   \
((WAL_HIPRIV_RATE_INVALID == (_l_val)) && (WAL_DSCR_PARAM_RATE <= (_pc_stu)->uc_function_index) && (WAL_DSCR_PARAM_MCSAC >= (_pc_stu)->uc_function_index)) || (0 == (_l_val) && (WAL_DSCR_PARAM_RATE == (_pc_stu)->uc_function_index))


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
extern oal_iw_handler_def_stru g_st_iw_handler_def_etc;
extern oal_net_device_ops_stru g_st_wal_net_dev_ops_etc;
extern oal_ethtool_ops_stru g_st_wal_ethtool_ops_etc;
#ifdef _PRE_WLAN_WEB_CMD_COMM
extern oal_iw_handler_def_stru g_st_iw_cfg_handler_def;
#endif
/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/* 私有命令入口结构定义 */
typedef struct
{
    oal_int8           *pc_cmd_name;    /* 命令字符串 */
    wal_hipriv_cmd_func p_func;         /* 命令对应处理函数 */
}wal_hipriv_cmd_entry_stru;

#ifdef _PRE_WLAN_FEATURE_VOWIFI
/* VoWiFi命令的转换结构体 */
typedef struct
{
    oal_int8                 *pc_vowifi_cmd_name;    /* 命令字符串 */
    mac_vowifi_cmd_enum_uint8 en_vowifi_cmd;         /* 命令对应类型 */
    oal_uint8                 auc_resv[3];
}wal_vowifi_cmd_stru;
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

/* 协议模式与字符串映射 */
typedef struct
{
    oal_int8                           *pc_name;        /* 模式名字符串 */
    wlan_protocol_enum_uint8            en_mode;        /* 协议模式 */
    wlan_channel_band_enum_uint8        en_band;        /* 频段 */
    wlan_channel_bandwidth_enum_uint8   en_bandwidth;   /* 带宽 */
    oal_uint8                           auc_resv[1];
}wal_ioctl_mode_map_stru;

/* 算法参数配置结构体 */
typedef struct
{
    oal_int8                           *pc_name;        /* 配置命令字符串 */
    mac_dyn_cali_cfg_enum_uint8         en_dyn_cali_cfg;     /* 配置命令对应的枚举值 */
    oal_uint8                           auc_resv[3];    /* 字节对齐 */
}wal_ioctl_dyn_cali_stru;

/* 算法参数配置结构体 */
typedef struct
{
    oal_int8                           *pc_name;        /* 配置命令字符串 */
    mac_alg_cfg_enum_uint8              en_alg_cfg;     /* 配置命令对应的枚举值 */
    oal_uint8                           auc_resv[3];    /* 字节对齐 */
}wal_ioctl_alg_cfg_stru;

/* 1102 使用wpa_supplicant 下发命令 */
typedef struct wal_android_wifi_priv_cmd {
    oal_int32    l_total_len;
    oal_int32    l_used_len;
    oal_uint8   *puc_buf;
}wal_android_wifi_priv_cmd_stru;

typedef struct
{
    oal_int8                           *pc_name;        /* 配置命令字符串 */
    wlan_cfgid_enum_uint16              en_tlv_cfg_id;     /* 配置命令对应的枚举值 */
    oal_uint8                           auc_resv[2];    /* 字节对齐 */
}wal_ioctl_tlv_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  wal_hipriv_ampdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_amsdu_tx_on(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_set_rate_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_set_mcs_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_set_mcsac_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_11AX
extern oal_uint32  wal_hipriv_set_mcsax(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
extern oal_uint32  wal_hipriv_vap_info_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
extern oal_uint32  wal_hipriv_pk_mode_debug(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern oal_uint32  wal_hipriv_set_ip_filter_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif //_PRE_WLAN_FEATURE_IP_FILTER
extern oal_uint32  wal_hipriv_create_proc_etc(oal_void *p_proc_arg);
extern oal_int32   wal_netdev_stop_etc(oal_net_device_stru *pst_net_dev);
extern oal_int32   wal_netdev_open_ext(oal_net_device_stru *pst_net_dev);
extern oal_int32   wal_netdev_open_etc(oal_net_device_stru *pst_net_dev,oal_uint8 uc_entry_flag);
extern oal_uint32  wal_hipriv_del_vap_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
extern oal_uint32  wal_hipriv_remove_proc_etc(oal_void);
extern oal_uint32  wal_alloc_cfg_event_etc(oal_net_device_stru *pst_net_dev,frw_event_mem_stru **ppst_event_mem,
                 oal_void*     pst_resp_addr, wal_msg_stru  **ppst_cfg_msg, oal_uint16 us_len);
extern oal_int32   wal_send_cfg_event_etc(
                                   oal_net_device_stru      *pst_net_dev,
                                   wal_msg_type_enum_uint8   en_msg_type,
                                   oal_uint16                us_len,
                                   oal_uint8                *puc_param,
                                   oal_bool_enum_uint8       en_need_rsp,
                                   wal_msg_stru            **ppst_rsp_msg);
extern oal_uint32 wal_hipriv_process_rate_params(oal_net_device_stru *pst_net_dev, oal_int8 *pc_cmd, mac_cfg_set_dscr_param_stru *pc_stu);

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
extern oal_int32 wal_start_vap_etc(oal_net_device_stru *pst_net_dev);
extern oal_int32  wal_stop_vap_etc(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_netdev_stop_ap_etc(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_init_wlan_vap_etc(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_deinit_wlan_vap_etc(oal_net_device_stru *pst_net_dev);
extern oal_int32 wal_init_wlan_netdev_etc(oal_wiphy_stru *pst_wiphy, char *dev_name);
extern oal_int32  wal_setup_ap_etc(oal_net_device_stru *pst_net_dev);
extern oal_int32  wal_host_dev_init_etc(oal_net_device_stru *pst_net_dev);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
extern oal_uint32  wal_regdomain_update_sta_etc(oal_uint8 uc_vap_id);
oal_int32 wal_regdomain_update_country_code_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_country);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_int32  wal_set_random_mac_to_mib_etc(oal_net_device_stru *pst_net_dev);
#endif

extern oal_uint8* wal_get_reduce_sar_ctrl_params(oal_uint8 uc_tx_power_lvl);

wlan_p2p_mode_enum_uint8 wal_wireless_iftype_to_mac_p2p_mode_etc(enum nl80211_iftype en_iftype);
#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
extern oal_uint32 wal_hipriv_arp_offload_enable(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
oal_uint32 wal_hipriv_show_arpoffload_info(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
extern oal_uint32  wal_ioctl_set_m2s_mss(oal_net_device_stru *pst_net_dev, oal_uint8 uc_m2s_mode);
#endif
extern oal_int32  wal_cfg_vap_h2d_event_etc(oal_net_device_stru *pst_net_dev);

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_uint32 hwifi_config_init_dts_main_etc(oal_net_device_stru *pst_cfg_net_dev);
extern oal_int32 wal_set_custom_process_func_etc(oal_void);
extern oal_uint32 hwifi_config_init_nvram_main_etc(oal_net_device_stru * pst_cfg_net_dev);

extern oal_uint32 wal_custom_cali_etc(oal_void);
extern oal_void hwifi_config_init_force_etc(oal_void);
extern oal_int32 hwifi_config_host_global_ini_param(oal_void);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
extern oal_uint32  wal_hipriv_proc_write_process_rsp(frw_event_mem_stru  *pst_event_mem);
#endif
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
extern oal_uint32  wal_ioctl_set_essid_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
extern oal_uint32  wal_get_cmd_one_arg_etc(oal_int8 *pc_cmd, oal_int8 *pc_arg, oal_uint32 *pul_cmd_offset);
extern oal_uint32  wal_hipriv_send_cfg_uint32_data_etc(oal_net_device_stru *pst_net_dev,
    oal_int8 *pc_param, wlan_cfgid_enum_uint16 cfgid);
#ifdef _PRE_WLAN_CFGID_DEBUG
extern oal_uint32 wal_hipriv_get_debug_cmd_size_etc(oal_void);
#endif
extern oal_uint32  wal_hipriv_alg_cfg_etc(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
oal_int32  wal_hipriv_wait_rsp(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
oal_int32   wal_ioctl_get_power_param(oal_net_device_stru *pst_net_dev, oal_iw_request_info_stru *pst_info, oal_void *p_param, oal_int8 *pc_extra);
#endif

extern oal_uint32  wal_hipriv_parse_cmd_etc(oal_int8 *pc_cmd);
extern oal_int32 wal_set_ip_filter_enable_etc(oal_int32 l_on);
extern oal_int32 wal_add_ip_filter_items_etc(wal_hw_wifi_filter_item *pst_items, oal_int32 l_count);
extern oal_int32 wal_clear_ip_filter_etc(void);
#ifdef _PRE_WLAN_FEATURE_M2S
extern oal_uint32  wal_hipriv_set_m2s_switch(oal_net_device_stru *pst_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern oal_int32 wal_register_ip_filter_etc(wal_hw_wlan_filter_ops *pg_st_ip_filter_ops);
extern oal_int32 wal_unregister_ip_filter_etc(void);
#endif /* _PRE_WLAN_FEATURE_IP_FILTER */

#ifdef _PRE_WLAN_FEATURE_AP_PM
extern oal_uint32  wal_hipriv_pm_enable(oal_net_device_stru *pst_cfg_net_dev, oal_int8 *pc_param);
#endif
#ifdef _PRE_WLAN_WEB_CMD_COMM
extern oal_int32  wal_io2cfg_get_chan_info(oal_net_device_stru *pst_net_dev, mac_cfg_channel_param_stru *pst_channel_param, mac_beacon_param_stru *st_beacon_param);
#endif
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_uint32  wal_hipriv_dyn_cali_cfg(oal_net_device_stru *pst_net_dev, oal_int8 *puc_param);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of wal_linux_ioctl.h */

