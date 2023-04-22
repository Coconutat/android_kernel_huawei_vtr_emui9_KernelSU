

#ifndef __MAC_DEVICE_H__
#define __MAC_DEVICE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_workqueue.h"
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "wlan_mib.h"
#include "hal_ext_if.h"
#include "mac_regdomain.h"
#include "mac_frame.h"
#include "wlan_types.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_DEVICE_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define MAC_FTM_TIMER_CNT                   4

#define MAC_NET_DEVICE_NAME_LENGTH          16
#define MAC_BAND_CAP_NAME_LENGTH            16

#define MAC_DATARATES_PHY_80211G_NUM        12

#define MAC_RX_BA_LUT_BMAP_LEN             ((HAL_MAX_RX_BA_LUT_SIZE + 7) >> 3)
#define MAC_TX_BA_LUT_BMAP_LEN             ((HAL_MAX_TX_BA_LUT_SIZE + 7) >> 3)

/* 异常超时上报时间 */
#define MAC_EXCEPTION_TIME_OUT              10000

/* DMAC SCANNER 扫描模式 */
#define MAC_SCAN_FUNC_MEAS           0x1
#define MAC_SCAN_FUNC_STATS          0x2
#define MAC_SCAN_FUNC_RADAR          0x4
#define MAC_SCAN_FUNC_BSS            0x8
#define MAC_SCAN_FUNC_P2P_LISTEN     0x10
#define MAC_SCAN_FUNC_ALL            (MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS | MAC_SCAN_FUNC_RADAR | MAC_SCAN_FUNC_BSS)

#define MAC_ERR_LOG(_uc_vap_id, _puc_string)
#define MAC_ERR_LOG1(_uc_vap_id, _puc_string, _l_para1)
#define MAC_ERR_LOG2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define MAC_ERR_LOG3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define MAC_ERR_LOG4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define MAC_ERR_VAR(_uc_vap_id, _c_fmt, ...)

#define MAC_WARNING_LOG(_uc_vap_id, _puc_string)
#define MAC_WARNING_LOG1(_uc_vap_id, _puc_string, _l_para1)
#define MAC_WARNING_LOG2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define MAC_WARNING_LOG3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define MAC_WARNING_LOG4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define MAC_WARNING_VAR(_uc_vap_id, _c_fmt, ...)

#define MAC_INFO_LOG(_uc_vap_id, _puc_string)
#define MAC_INFO_LOG1(_uc_vap_id, _puc_string, _l_para1)
#define MAC_INFO_LOG2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define MAC_INFO_LOG3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define MAC_INFO_LOG4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define MAC_INFO_VAR(_uc_vap_id, _c_fmt, ...)

/* 获取dev的算法私有结构体 */
#define MAC_DEV_ALG_PRIV(_pst_dev)                  ((_pst_dev)->p_alg_priv)

#define MAC_CHIP_ALG_PRIV(_pst_chip)                ((_pst_chip)->p_alg_priv)


/*复位状态*/
#define MAC_DEV_RESET_IN_PROGRESS(_pst_device,uc_value)    ((_pst_device)->uc_device_reset_in_progress = uc_value)
#define MAC_DEV_IS_RESET_IN_PROGRESS(_pst_device)          ((_pst_device)->uc_device_reset_in_progress)

#define MAC_DFS_RADAR_WAIT_TIME_MS    60000

#ifdef _PRE_WLAN_FEATURE_HILINK

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
#define HMAC_FBT_MAX_USER_NUM   64
#else
#define HMAC_FBT_MAX_USER_NUM   32
#endif

#define FBT_DEFAULT_SCAN_CHANNEL            (0)     /* 默认配置FBT scan channel为home信道*/
#define FBT_DEFAULT_SCAN_INTERVAL           (200)   /* 默认配置FBT scan interval 200ms */
#define FBT_DEFAULT_SCAN_REPORT_PERIOD      (200)   /* 默认配置未关联用户侦听上报周期1000ms -- 需要修改为200ms */
#define FBT_DEFAULT_SCAN_CHANNEL_STAY_TIME  (50)   /* 默认侦听信道驻留时长 */
#define FBT_DEFAULT_WORK_CHANNEL_STAY_TIME  (150)   /* 默认工作信道驻留时长 */
#endif

#define MAC_SCAN_CHANNEL_INTERVAL_DEFAULT               6           /* 间隔6个信道，切回工作信道工作一段时间 */
#define MAC_WORK_TIME_ON_HOME_CHANNEL_DEFAULT           110         /* 背景扫描时，返回工作信道工作的时间 */
#define MAC_SCAN_CHANNEL_INTERVAL_PERFORMANCE           2           /* 间隔2个信道，切回工作信道工作一段时间 */
#define MAC_WORK_TIME_ON_HOME_CHANNEL_PERFORMANCE       60          /* WLAN未关联，P2P关联，返回工作信道工作的时间 */
#define MAC_SCAN_CHANNEL_INTERVAL_HIDDEN_SSID             3           /* 携带隐藏SSID的背景扫描，间隔3个信道回工作信道工作一段时间 */

#define MAC_FCS_DBAC_IGNORE           0   /* 不是DBAC场景 */
#define MAC_FCS_DBAC_NEED_CLOSE       1   /* DBAC需要关闭 */
#define MAC_FCS_DBAC_NEED_OPEN        2   /* DBAC需要开启 */

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
#define MAC_MAX_IP_FILTER_BTABLE_SIZE 512 /* rx ip数据包过滤功能的黑名单大小 */
#endif //_PRE_WLAN_FEATURE_IP_FILTER

#define MAC_DEVICE_GET_CAP_BW(_pst_device)      ((_pst_device)->st_device_cap.en_channel_width)
#define MAC_DEVICE_GET_NSS_NUM(_pst_device)     ((_pst_device)->st_device_cap.en_nss_num)
#define MAC_DEVICE_GET_CAP_LDPC(_pst_device)    ((_pst_device)->st_device_cap.en_ldpc_is_supp)
#define MAC_DEVICE_GET_CAP_TXSTBC(_pst_device)  ((_pst_device)->st_device_cap.en_tx_stbc_is_supp)
#define MAC_DEVICE_GET_CAP_RXSTBC(_pst_device)  ((_pst_device)->st_device_cap.en_rx_stbc_is_supp)
#define MAC_DEVICE_GET_CAP_SUBFER(_pst_device)  ((_pst_device)->st_device_cap.en_su_bfmer_is_supp)
#define MAC_DEVICE_GET_CAP_SUBFEE(_pst_device)  ((_pst_device)->st_device_cap.en_su_bfmee_is_supp)
#define MAC_DEVICE_GET_CAP_MUBFER(_pst_device)  ((_pst_device)->st_device_cap.en_mu_bfmer_is_supp)
#define MAC_DEVICE_GET_CAP_MUBFEE(_pst_device)  ((_pst_device)->st_device_cap.en_mu_bfmee_is_supp)
#ifdef _PRE_WLAN_FEATURE_SMPS
#define MAC_DEVICE_GET_MODE_SMPS(_pst_device)   ((_pst_device)->en_mac_smps_mode)
#endif

#define MAC_M2S_CALI_SMPS_MODE(en_nss)   \
        ((en_nss == WLAN_SINGLE_NSS) ? WLAN_MIB_MIMO_POWER_SAVE_STATIC: WLAN_MIB_MIMO_POWER_SAVE_MIMO)

#ifdef _PRE_WLAN_FEATURE_M2S
#define MAC_M2S_CALI_NSS_FROM_SMPS_MODE(en_smps_mode)     \
        ((en_smps_mode == WLAN_MIB_MIMO_POWER_SAVE_STATIC) ? WLAN_SINGLE_NSS: WLAN_DOUBLE_NSS)
#endif

#define MAX_FTM_RANGE_ENTRY_COUNT                 15
#define MAX_FTM_ERROR_ENTRY_COUNT                 11
#define MAX_MINIMUN_AP_COUNT                      14
#define MAX_REPEATER_NUM                          3         /* 支持的最大定位ap数量 */

#define WLAN_USER_MAX_SUPP_RATES                  16        /* 用于记录对端设备支持的速率最大个数 */


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/* SDT操作模式枚举 */
typedef enum
{
    MAC_SDT_MODE_WRITE = 0,
    MAC_SDT_MODE_READ,
    MAC_SDT_MODE_WRITE16,
    MAC_SDT_MODE_READ16,

    MAC_SDT_MODE_BUTT
}mac_sdt_rw_mode_enum;
typedef oal_uint8 mac_sdt_rw_mode_enum_uint8;

typedef enum
{
    MAC_CH_TYPE_NONE      = 0,
    MAC_CH_TYPE_PRIMARY   = 1,
    MAC_CH_TYPE_SECONDARY = 2,
}mac_ch_type_enum;
typedef oal_uint8 mac_ch_type_enum_uint8;

typedef enum
{
    MAC_CSA_FLAG_NORMAL   = 0,
    MAC_CSA_FLAG_START_DEBUG,/*固定csa ie 在beacon帧中*/
    MAC_CSA_FLAG_CANCLE_DEBUG,

    MAC_CSA_FLAG_BUTT
}mac_csa_flag_enum;
typedef oal_uint8 mac_csa_flag_enum_uint8;


typedef enum
{
    MAC_SCAN_OP_INIT_SCAN,
    MAC_SCAN_OP_FG_SCAN_ONLY,
    MAC_SCAN_OP_BG_SCAN_ONLY,

    MAC_SCAN_OP_BUTT
}mac_scan_op_enum;
typedef oal_uint8 mac_scan_op_enum_uint8;

typedef enum
{
    MAC_CHAN_NOT_SUPPORT = 0,        /* 管制域不支持该信道 */
    MAC_CHAN_AVAILABLE_ALWAYS,       /* 信道一直可以使用 */
    MAC_CHAN_AVAILABLE_TO_OPERATE,   /* 经过检测(CAC, etc...)后，该信道可以使用 */
    MAC_CHAN_DFS_REQUIRED,           /* 该信道需要进行雷达检测 */
    MAC_CHAN_BLOCK_DUE_TO_RADAR,     /* 由于检测到雷达导致该信道变的不可用 */

    MAC_CHAN_STATUS_BUTT
}mac_chan_status_enum;
typedef oal_uint8 mac_chan_status_enum_uint8;

#ifdef _PRE_WLAN_DFT_STAT
typedef enum
{
    MAC_DEV_MGMT_STAT_TYPE_TX = 0,
    MAC_DEV_MGMT_STAT_TYPE_RX,
    MAC_DEV_MGMT_STAT_TYPE_TX_COMPLETE,

    MAC_DEV_MGMT_STAT_TYPE_BUTT
}mac_dev_mgmt_stat_type_enum;
typedef oal_uint8 mac_dev_mgmt_stat_type_enum_uint8;
#endif

/* device reset同步子类型枚举 */
typedef enum
{
    MAC_RESET_SWITCH_SET_TYPE,
    MAC_RESET_SWITCH_GET_TYPE,
    MAC_RESET_STATUS_GET_TYPE,
    MAC_RESET_STATUS_SET_TYPE,
    MAC_RESET_SWITCH_SYS_TYPE = MAC_RESET_SWITCH_SET_TYPE,
    MAC_RESET_STATUS_SYS_TYPE = MAC_RESET_STATUS_SET_TYPE,

    MAC_RESET_SYS_TYPE_BUTT
}mac_reset_sys_type_enum;
typedef oal_uint8 mac_reset_sys_type_enum_uint8;

typedef enum
{
    MAC_TRY_INIT_SCAN_VAP_UP,
    MAC_TRY_INIT_SCAN_SET_CHANNEL,
    MAC_TRY_INIT_SCAN_START_DBAC,
    MAC_TRY_INIT_SCAN_RESCAN,

    MAC_TRY_INIT_SCAN_BUTT
}mac_try_init_scan_type;
typedef oal_uint8 mac_try_init_scan_type_enum_uint8;

typedef enum
{
    MAC_INIT_SCAN_NOT_NEED,
    MAC_INIT_SCAN_NEED,
    MAC_INIT_SCAN_IN_SCAN,

}mac_need_init_scan_res;
typedef oal_uint8 mac_need_init_scan_res_enum_uint8;

/* p2p结构中包含此状态成员，该结构挂在mac device下，此VAP状态枚举移动到mac_device.h中 */
/* VAP状态机，AP STA共用一个状态枚举 */
typedef enum
{
    /* ap sta公共状态 */
    MAC_VAP_STATE_INIT               = 0,
    MAC_VAP_STATE_UP                 = 1,       /* VAP UP */
    MAC_VAP_STATE_PAUSE              = 2,       /* pause , for ap &sta */

    /* ap 独有状态 */
    MAC_VAP_STATE_AP_WAIT_START      = 3,

    /* sta独有状态 */
    MAC_VAP_STATE_STA_FAKE_UP        = 4,
    MAC_VAP_STATE_STA_WAIT_SCAN      = 5,
    MAC_VAP_STATE_STA_SCAN_COMP      = 6,
    MAC_VAP_STATE_STA_JOIN_COMP      = 7,
    MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2 = 8,
    MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4 = 9,
    MAC_VAP_STATE_STA_AUTH_COMP      = 10,
    MAC_VAP_STATE_STA_WAIT_ASOC      = 11,
    MAC_VAP_STATE_STA_OBSS_SCAN      = 12,
    MAC_VAP_STATE_STA_BG_SCAN        = 13,
    MAC_VAP_STATE_STA_LISTEN         = 14,/* p2p0 监听 */
    MAC_VAP_STATE_ROAMING            = 15,/* 漫游 */
    MAC_VAP_STATE_BUTT,
}mac_vap_state_enum;
typedef oal_uint8  mac_vap_state_enum_uint8;

#define     MAC_FCS_MAX_CHL_NUM    2
#define     MAC_FCS_TIMEOUT_JIFFY  2
#define     MAC_FCS_DEFAULT_PROTECT_TIME_OUT    5120    /* us */
#define     MAC_FCS_DEFAULT_PROTECT_TIME_OUT2   1024    /* us */
#define     MAC_FCS_DEFAULT_PROTECT_TIME_OUT3   15000   /* us */
#define     MAC_FCS_DEFAULT_PROTECT_TIME_OUT4   16000   /* us */
#define     MAC_ONE_PACKET_TIME_OUT             1000
#define     MAC_ONE_PACKET_TIME_OUT3            2000
#define     MAC_ONE_PACKET_TIME_OUT4            2000
#define     MAC_FCS_CTS_MAX_DURATION            32767   /* us */
#define     MAC_FCS_CTS_MAX_BTCOEX_NOR_DURATION            30000  /* us */
#define     MAC_FCS_CTS_MAX_BTCOEX_LDAC_DURATION           65535  /* us */

/*
 self CTS
+-------+-----------+----------------+
|frame  | duration  |      RA        |     len=10
|control|           |                |
+-------+-----------+----------------+

null data
+-------+-----------+---+---+---+--------+
|frame  | duration  |A1 |A2 |A3 |Seq Ctl | len=24
|control|           |   |   |   |        |
+-------+-----------+---+---+---+--------+

*/

typedef enum
{
    MAC_FCS_NOTIFY_TYPE_SWITCH_AWAY    = 0,
    MAC_FCS_NOTIFY_TYPE_SWITCH_BACK,

    MAC_FCS_NOTIFY_TYPE_BUTT
}mac_fcs_notify_type_enum;
typedef oal_uint8 mac_fcs_notify_type_enum_uint8;

/* 51 tbd, 注意host和device指针大小区别 */
typedef struct
{
    mac_channel_stru                st_dst_chl;
    mac_channel_stru                st_src_chl;
    hal_one_packet_cfg_stru        *pst_one_packet_cfg;
    oal_uint8                       auc_res_rom[OAL_SIZEOF(hal_one_packet_cfg_stru) - 4];
    oal_uint16                      us_hot_cnt;
    oal_uint8                       auc_resv[2];
    hal_to_dmac_device_stru        *pst_hal_device;
    hal_tx_dscr_queue_header_stru  *pst_src_fake_queue;  /*记录此vap自己的fake队列指针 */
    mac_channel_stru                st_src_chl2;
    hal_one_packet_cfg_stru        *pst_one_packet_cfg2;
    oal_uint8                       auc_res_rom2[OAL_SIZEOF(hal_one_packet_cfg_stru) - 4];
}mac_fcs_cfg_stru;

typedef enum
{
    MAC_FCS_HOOK_ID_DBAC,
    MAC_FCS_HOOK_ID_ACS,

    MAC_FCS_HOOK_ID_BUTT
}mac_fcs_hook_id_enum;
typedef oal_uint8   mac_fcs_hook_id_enum_uint8;

typedef struct
{
    mac_fcs_notify_type_enum_uint8  uc_notify_type;
    oal_uint8                       uc_chip_id;
    oal_uint8                       uc_device_id;
    oal_uint8                       uc_resv[1];
    mac_fcs_cfg_stru                st_fcs_cfg;
}mac_fcs_event_stru;

typedef void (* mac_fcs_notify_func)(const mac_fcs_event_stru*);

typedef struct
{
    mac_fcs_notify_func    p_func;
}mac_fcs_notify_node_stru;

typedef struct
{
    mac_fcs_notify_node_stru   ast_notify_nodes[MAC_FCS_HOOK_ID_BUTT];
}mac_fcs_notify_chain_stru;

typedef enum
{
    MAC_FCS_STATE_STANDBY        = 0,  // free to use
    MAC_FCS_STATE_REQUESTED,           // requested by other module, but not in switching
    MAC_FCS_STATE_IN_PROGESS,          // in switching

    MAC_FCS_STATE_BUTT
}mac_fcs_state_enum;
typedef oal_uint8 mac_fcs_state_enum_uint8;

typedef enum
{
    MAC_FCS_SUCCESS = 0,
    MAC_FCS_ERR_NULL_PTR,
    MAC_FCS_ERR_INVALID_CFG,
    MAC_FCS_ERR_BUSY,
    MAC_FCS_ERR_UNKNOWN_ERR,
}mac_fcs_err_enum;
typedef oal_uint8   mac_fcs_err_enum_uint8;

typedef struct
{
    oal_uint32  ul_offset_addr;
    oal_uint32  ul_value[MAC_FCS_MAX_CHL_NUM];
}mac_fcs_reg_record_stru;

typedef struct tag_mac_fcs_mgr_stru
{
    volatile oal_bool_enum_uint8    en_fcs_done;
    oal_uint8                       uc_chip_id;
    oal_uint8                       uc_device_id;
    oal_uint8                       uc_fcs_cnt;
    oal_spin_lock_stru              st_lock;
    mac_fcs_state_enum_uint8        en_fcs_state;
    hal_fcs_service_type_enum_uint8 en_fcs_service_type;
    oal_uint8                       uc_resv[2];
    mac_fcs_cfg_stru               *pst_fcs_cfg;
    mac_fcs_notify_chain_stru       ast_notify_chain[MAC_FCS_NOTIFY_TYPE_BUTT];
}mac_fcs_mgr_stru;

#define MAC_FCS_VERIFY_MAX_ITEMS    1
typedef enum
{
    // isr
    MAC_FCS_STAGE_INTR_START,
    MAC_FCS_STAGE_INTR_POST_EVENT,
    MAC_FCS_STAGE_INTR_DONE,

    // event
    MAC_FCS_STAGE_EVENT_START,
    MAC_FCS_STAGE_PAUSE_VAP,
    MAC_FCS_STAGE_ONE_PKT_START,
    MAC_FCS_STAGE_ONE_PKT_INTR,
    MAC_FCS_STAGE_ONE_PKT_DONE,
    MAC_FCS_STAGE_RESET_HW_START,
    MAC_FCS_STAGE_RESET_HW_DONE,
    MAC_FCS_STAGE_RESUME_VAP,
    MAC_FCS_STAGE_EVENT_DONE,

    MAC_FCS_STAGE_COUNT
}mac_fcs_stage_enum;
typedef mac_fcs_stage_enum mac_fcs_stage_enum_uint8;

typedef struct
{
    oal_bool_enum_uint8         en_enable;
    oal_uint8                   auc_resv[3];
    oal_uint32                  ul_item_cnt;
    oal_uint32                  aul_timestamp[MAC_FCS_VERIFY_MAX_ITEMS][MAC_FCS_STAGE_COUNT];
}mac_fcs_verify_stat_stru;

/* 上报关键信息的flags标记信息，对应标记位为1，则对上报对应信息 */
typedef enum
{
    MAC_REPORT_INFO_FLAGS_HARDWARE_INFO          = BIT(0),
    MAC_REPORT_INFO_FLAGS_QUEUE_INFO             = BIT(1),
    MAC_REPORT_INFO_FLAGS_MEMORY_INFO            = BIT(2),
    MAC_REPORT_INFO_FLAGS_EVENT_INFO             = BIT(3),
    MAC_REPORT_INFO_FLAGS_VAP_INFO               = BIT(4),
    MAC_REPORT_INFO_FLAGS_USER_INFO              = BIT(5),
    MAC_REPORT_INFO_FLAGS_TXRX_PACKET_STATISTICS = BIT(6),
    MAC_REPORT_INFO_FLAGS_BUTT                   = BIT(7),
}mac_report_info_flags;

#ifdef _PRE_WLAN_FEATURE_HILINK
/* 用来定义FBT SCAN的两种模式:关闭和开启扫描模式 */
typedef enum
{
    HMAC_FBT_SCAN_CLOSE     = 0,
    HMAC_FBT_SCAN_OPEN      = 1,

    HMAC_FBT_SCAN_BUTT
}hmac_fbt_scan_enum;
typedef oal_uint8 hmac_fbt_scan_enum_uint8;

/* 侦听结果成员使用状况*/
typedef enum
{
    HMAC_FBT_SCAN_USER_NOT_USED = 0, /*0表示未使用*/
    HMAC_FBT_SCAN_USER_IS_USED  = 1, /*1表示已使用并写入MAC地址*/

    HMAC_FBT_SCAN_USER_BUTT
}hmac_fbt_scan_user_used_state;
#endif

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
/* rx ip数据包过滤的配置命令 */
typedef enum
{
    MAC_IP_FILTER_ENABLE         = 0, /* 开/关ip数据包过滤功能 */
    MAC_IP_FILTER_UPDATE_BTABLE  = 1, /* 更新黑名单 */
    MAC_IP_FILTER_CLEAR          = 2, /* 清除黑名单 */

    MAC_IP_FILTER_BUTT
}mac_ip_filter_cmd_enum;
typedef oal_uint8 mac_ip_filter_cmd_enum_uint8;

/* 黑名单条目格式 */
typedef struct
{
    oal_uint16                     us_port;          /* 目的端口号，以主机字节序格式存储 */
    oal_uint8                      uc_protocol;
    oal_uint8                      uc_resv;
    //oal_uint32                  ul_filter_cnt;     /* 目前未接受"统计过滤包数量"的需求，此成员暂不使用 */
}mac_ip_filter_item_stru;

/* 配置命令格式 */
typedef struct
{
    oal_uint8                       uc_item_count;
    oal_bool_enum_uint8             en_enable;       /* 下发功能使能标志 */
    mac_ip_filter_cmd_enum_uint8    en_cmd;
    oal_uint8                       uc_resv;
    mac_ip_filter_item_stru         ast_filter_items[1];
}mac_ip_filter_cmd_stru;

#endif //_PRE_WLAN_FEATURE_IP_FILTER

#ifdef _PRE_WLAN_FEATURE_APF
#define APF_PROGRAM_MAX_LEN 512
#define APF_VERSION 2
typedef enum
{
    APF_SET_FILTER_CMD,
    APF_GET_FILTER_CMD
}mac_apf_cmd_type_enum;
typedef oal_uint8 mac_apf_cmd_type_uint8;

typedef struct
{
    oal_bool_enum_uint8             en_is_enabled;
    oal_uint16                      us_program_len;
    oal_uint32                      ul_install_timestamp;
    oal_uint32                      ul_flt_pkt_cnt;
    oal_uint8                       auc_program[APF_PROGRAM_MAX_LEN];
}mac_apf_stru;

typedef struct
{
    mac_apf_cmd_type_uint8          en_cmd_type;
    oal_uint16                      us_program_len;
    oal_uint8                      *puc_program;
}mac_apf_filter_cmd_stru;
#endif

typedef enum
{
    MAC_DEVICE_DISABLE =0,
    MAC_DEVICE_2G,
    MAC_DEVICE_5G,
    MAC_DEVICE_2G_5G,

    MAC_DEVICE_BUTT,
}mac_device_radio_cap_enum;
typedef oal_uint8 mac_device_radio_cap_enum_uint8;

/* 配置命令使用，mimo-siso切换mode */
typedef enum
{
    MAC_M2S_MODE_QUERY        = 0,  /* 参数查询模式 */
    MAC_M2S_MODE_MSS          = 1,  /* MSS下发模式 */
    MAC_M2S_MODE_DELAY_SWITCH = 2,  /* 延迟切换测试模式 */
    MAC_M2S_MODE_SW_TEST      = 3,  /* 软切换测试模式,测试siso和mimo */
    MAC_M2S_MODE_HW_TEST      = 4,  /* 硬切换测试模式,测试siso和mimo */
    MAC_M2S_MODE_RSSI         = 5,  /* rssi切换 */

    MAC_M2S_MODE_BUTT,
}mac_m2s_mode_enum;
typedef oal_uint8 mac_m2s_mode_enum_uint8;

/* 配置命令使用，期望切换状态 */
typedef enum
{
    MAC_M2S_COMMAND_STATE_SISO_C0   = 0,
    MAC_M2S_COMMAND_STATE_SISO_C1   = 1,
    MAC_M2S_COMMAND_STATE_MIMO      = 2,
    MAC_M2S_COMMAND_STATE_MISO_C0   = 3,
    MAC_M2S_COMMAND_STATE_MISO_C1   = 4,

    MAC_M2S_COMMAND_STATE_BUTT,
}mac_m2s_command_state_enum;
typedef oal_uint8 mac_m2s_command_state_enum_uint8;

/* MSS使用时命令形式 */
typedef enum
{
    MAC_M2S_COMMAND_MODE_SET_AUTO     = 0,
    MAC_M2S_COMMAND_MODE_SET_SISO_C0  = 1,
    MAC_M2S_COMMAND_MODE_SET_SISO_C1  = 2,
    MAC_M2S_COMMAND_MODE_SET_MIMO     = 3,
    MAC_M2S_COMMAND_MODE_GET_STATE    = 4,

    MAC_M2S_COMMAND_MODE_BUTT,
}mac_m2s_command_mode_enum;
typedef oal_uint8 mac_m2s_command_mode_enum_uint8;

/* 配置接收功率参数 */
typedef enum
{
    HAL_M2S_RSSI_SHOW_TH,
    HAL_M2S_RSSI_SHOW_MGMT,
    HAL_M2S_RSSI_SHOW_DATA,
    HAL_M2S_RSSI_SET_MIN_TH,
    HAL_M2S_RSSI_SET_DIFF_TH,
}hal_dev_rssi_enum;
typedef oal_uint8 hal_dev_rssi_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
typedef struct
{
    oal_uint8           uc_core_idx;
    oal_bool_enum_uint8 en_need_improved;
    oal_uint8           auc_rev[2];
}mac_cfg_tas_pwr_ctrl_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
typedef struct
{
    oal_bool_enum_uint8             en_is_pk_mode;
    wlan_bw_cap_enum_uint8          en_curr_bw_cap;             /* 目前使用的带宽，本参数仅在lagency sta模式下生效 */
    wlan_protocol_cap_enum_uint8    en_curr_protocol_cap;       /* 目前使用的协议模式，本参数仅在lagency sta模式下生效 */
    wlan_nss_enum_uint8             en_curr_num_spatial_stream; /* 目前单双流的计数 */

    oal_uint32          ul_tx_bytes;            /* WIFI 业务发送帧统计 */
    oal_uint32          ul_rx_bytes;            /* WIFI 业务接收帧统计 */
    oal_uint32          ul_dur_time;            /* 统计时间间隔 */
}mac_cfg_pk_mode_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_FTM
typedef enum
{
    NO_LOCATION    = 0,
    ROOT_AP        = 1,
    REPEATER       = 2,
    STA            = 3,
    LOCATION_TYPE_BUTT
}oal_location_type_enum;
typedef oal_uint8 oal_location_type_enum_uint8;
#endif

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
extern oal_uint8  g_auc_valid_blacklist_idx[];
#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
extern mac_cfg_pk_mode_stru g_st_wifi_pk_mode_status;
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
/* device reset事件同步结构体 */
typedef struct
{
    mac_reset_sys_type_enum_uint8  en_reset_sys_type;  /* 复位同步类型 */
    oal_uint8                      uc_value;           /* 同步信息值 */
    oal_uint8                      uc_resv[2];
}mac_reset_sys_stru;

typedef void (*mac_scan_cb_fn)(void *p_scan_record);

typedef struct
{
    oal_uint16                    us_num_networks;
    mac_ch_type_enum_uint8        en_ch_type;
#ifdef _PRE_WLAN_FEATURE_DFS
    mac_chan_status_enum_uint8    en_ch_status;
#else
    oal_uint8                     auc_resv[1];
#endif
}mac_ap_ch_info_stru;

typedef struct
{
    oal_uint8                           uc_p2p_device_num;                      /* 当前device下的P2P_DEVICE数量 */
    oal_uint8                           uc_p2p_goclient_num;                    /* 当前device下的P2P_CL/P2P_GO数量 */
    oal_uint8                           uc_p2p0_vap_idx;                        /* P2P 共存场景下，P2P_DEV(p2p0) 指针 */
    mac_vap_state_enum_uint8            en_last_vap_state;                      /* P2P0/P2P_CL 共用VAP 结构，监听场景下保存VAP 进入监听前的状态 */
    oal_uint8                           uc_resv[2];                             /* 保留 */
    oal_uint8                           en_roc_need_switch;                     /* remain on channel后需要切回原信道*/
    oal_uint8                           en_p2p_ps_pause;                        /* P2P 节能是否处于pause状态*/
    oal_net_device_stru                *pst_p2p_net_device;                     /* P2P 共存场景下主net_device(p2p0) 指针 */
    oal_uint64                          ull_send_action_id;                     /* P2P action id/cookie */
    oal_uint64                          ull_last_roc_id;
    oal_ieee80211_channel_stru          st_listen_channel;
    oal_nl80211_channel_type            en_listen_channel_type;
    oal_net_device_stru                *pst_primary_net_device;                 /* P2P 共存场景下主net_device(wlan0) 指针 */
    oal_net_device_stru                *pst_second_net_device;                 /*信道跟随增加,后续不使用可以删除*/
}mac_p2p_info_stru;

typedef struct
{
    oal_uint16    us_num_networks;    /* 记录当前信道下扫描到的BSS个数 */
    oal_uint8     auc_resv[2];
    oal_uint8     auc_bssid_array[WLAN_MAX_SCAN_BSS_PER_CH][WLAN_MAC_ADDR_LEN];  /* 记录当前信道下扫描到的所有BSSID */
}mac_bss_id_list_stru;

#define MAX_PNO_SSID_COUNT          16
#define MAX_PNO_REPEAT_TIMES        4
#define PNO_SCHED_SCAN_INTERVAL     (60 * 1000)

/* PNO扫描信息结构体 */
typedef struct
{
    oal_uint8           auc_ssid[WLAN_SSID_MAX_LEN];
    oal_bool_enum_uint8 en_scan_ssid;
    oal_uint8           auc_resv[2];
}pno_match_ssid_stru;

typedef struct
{
    pno_match_ssid_stru   ast_match_ssid_set[MAX_PNO_SSID_COUNT];
    oal_int32             l_ssid_count;                           /* 下发的需要匹配的ssid集的个数 */
    oal_int32             l_rssi_thold;                           /* 可上报的rssi门限 */
    oal_uint32            ul_pno_scan_interval;                   /* pno扫描间隔 */
    oal_uint8             auc_sour_mac_addr[WLAN_MAC_ADDR_LEN];   /* probe req帧中携带的发送端地址 */
    oal_uint8             uc_pno_scan_repeat;                     /* pno扫描重复次数 */
    oal_bool_enum_uint8   en_is_random_mac_addr_scan;             /* 是否随机mac */

    mac_scan_cb_fn        p_fn_cb;                                /* 函数指针必须放最后否则核间通信出问题 */
}mac_pno_scan_stru;

/* PNO调度扫描管理结构体 */
typedef struct
{
    mac_pno_scan_stru       st_pno_sched_scan_params;             /* pno调度扫描请求的参数 */
    //frw_timeout_stru        st_pno_sched_scan_timer;              /* pno调度扫描定时器 */
    oal_void               *p_pno_sched_scan_timer;               /* pno调度扫描rtc时钟定时器，此定时器超时后，能够唤醒睡眠的device */
    oal_uint8               uc_curr_pno_sched_scan_times;         /* 当前pno调度扫描次数 */
    oal_bool_enum_uint8     en_is_found_match_ssid;               /* 是否扫描到了匹配的ssid */
    oal_uint8               auc_resv[2];
}mac_pno_sched_scan_mgmt_stru;


typedef struct
{
    oal_uint8  auc_ssid[WLAN_SSID_MAX_LEN];
    oal_uint8  auc_resv[3];
}mac_ssid_stru;

typedef struct
{
    oal_uint8   uc_mac_rate; /* MAC对应速率 */
    oal_uint8   uc_phy_rate; /* PHY对应速率 */
    oal_uint8   uc_mbps;     /* 速率 */
    oal_uint8   auc_resv[1];
}mac_data_rate_stru;

/* 扫描参数结构体 */
typedef struct
{
    wlan_mib_desired_bsstype_enum_uint8 en_bss_type;            /* 要扫描的bss类型 */
    wlan_scan_type_enum_uint8           en_scan_type;           /* 主动/被动 */
    oal_uint8                           uc_bssid_num;           /* 期望扫描的bssid个数 */
    oal_uint8                           uc_ssid_num;            /* 期望扫描的ssid个数 */

    oal_uint8                           auc_sour_mac_addr[WLAN_MAC_ADDR_LEN];       /* probe req帧中携带的发送端地址 */
    oal_uint8                           uc_p2p0_listen_channel;
    oal_uint8                           uc_max_scan_count_per_channel;              /* 每个信道的扫描次数 */

    oal_uint8                           auc_bssid[WLAN_SCAN_REQ_MAX_BSSID][WLAN_MAC_ADDR_LEN];  /* 期望的bssid */
    mac_ssid_stru                       ast_mac_ssid_set[WLAN_SCAN_REQ_MAX_SSID];               /* 期望的ssid */

    oal_uint8                           uc_max_send_probe_req_count_per_channel;                /* 每次信道发送扫描请求帧的个数，默认为1 */
    oal_uint8                           bit_is_p2p0_scan       : 1;   /* 是否为p2p0 发起扫描 */
    oal_uint8                           bit_is_radom_mac_saved : 1;   /* 是否已经保存随机mac */
    oal_uint8                           bit_radom_mac_saved_to_dev : 2; /* 用于并发扫描 */
    oal_uint8                           bit_desire_fast_scan       : 1;   /* 本次扫描期望使用并发 */
    oal_uint8                           bit_rsv                    : 3;   /* 保留位 */

    oal_bool_enum_uint8                 en_abort_scan_flag;           /* 终止扫描 */
    oal_bool_enum_uint8                 en_is_random_mac_addr_scan;   /* 是否是随机mac addr扫描 */


    oal_bool_enum_uint8                 en_need_switch_back_home_channel;       /* 背景扫描时，扫描完一个信道，判断是否需要切回工作信道工作 */
    oal_uint8                           uc_scan_channel_interval;               /* 间隔n个信道，切回工作信道工作一段时间 */
    oal_uint16                          us_work_time_on_home_channel;           /* 背景扫描时，返回工作信道工作的时间 */

    mac_channel_stru                    ast_channel_list[WLAN_MAX_CHANNEL_NUM];

    oal_uint8                           uc_channel_nums;        /* 信道列表中信道的个数 */
    oal_uint8                           uc_probe_delay;         /* 主动扫描发probe request帧之前的等待时间 */
    oal_uint16                          us_scan_time;           /* 扫描在某一信道停留此时间后，扫描结束, ms，必须是10的整数倍 */

    wlan_scan_mode_enum_uint8           en_scan_mode;                   /* 扫描模式:前景扫描 or 背景扫描 */
    oal_uint8                           uc_scan_flag;            /*内核下发的扫描模式,每个bit意义见wlan_scan_flag_enum，暂时只解析是否为并发扫描*/
    oal_uint8                           uc_scan_func;                   /* DMAC SCANNER 扫描模式 */
    oal_uint8                           uc_vap_id;                      /* 下发扫描请求的vap id */
    oal_uint64                          ull_cookie;             /* P2P 监听下发的cookie 值 */
#ifdef _PRE_SUPPORT_ACS
    oal_uint8                           uc_acs_type;            /* acs触发类型:初始/命令触发/空闲自动触发acs */
    oal_bool_enum_uint8                 en_switch_chan;         /* acs命令触发是否切换信道 */
    oal_uint8                           auc_resv[2];
#endif
    /* 重要:回调函数指针:函数指针必须放最后否则核间通信出问题 */
    mac_scan_cb_fn                      p_fn_cb;
}mac_scan_req_stru;

/* 打印接收报文的rssi信息的调试开关相关的结构体 */
typedef struct
{
    oal_uint16            us_data_len;                 /*  单音采样点个数  */
    oal_uint8             uc_tone_tran_switch;         /*  单音发送开关  */
    oal_uint8             uc_chain_idx;               /*  单音发送通道号  */
}mac_tone_transmit_stru;

/* 打印接收报文的rssi信息的调试开关相关的结构体 */
typedef struct
{
    oal_bool_enum_uint8     en_debug_switch;             /* 打印总开关 */
    oal_bool_enum_uint8     en_rssi_debug_switch;        /* 打印接收报文的rssi信息的调试开关 */
    oal_bool_enum_uint8     en_snr_debug_switch;         /* 打印接收报文的snr信息的调试开关 */
    oal_bool_enum_uint8     en_trlr_debug_switch;        /* 打印接收报文的trailer信息的调试开关 */
    oal_bool_enum_uint8     en_evm_debug_switch;         /* 打印接收报文的evm信息的调试开关 */
    oal_uint8               auc_resv[3];
    oal_uint32              ul_curr_rx_comp_isr_count;   /* 一轮间隔内，接收完成中断的产生个数 */
    oal_uint32              ul_rx_comp_isr_interval;     /* 间隔多少个接收完成中断打印一次rssi信息 */
    mac_tone_transmit_stru  st_tone_tran;                /* 单音发送参数 */
    oal_uint8               auc_trlr_sel_info[5];        /* trailer选择上报计数, 一个字节高4bit指示trlr or vect,低4个bit指示选择 */
    oal_uint8               uc_trlr_sel_num;             /* 记录单次命令输入选项的最大值 */
    oal_uint8               uc_iq_cali_switch;           /* iq校准调试命令  */
    oal_bool_enum_uint8     en_pdet_debug_switch;        /* 打印芯片上报pdet值的调试开关 */
    oal_bool_enum_uint8     en_tsensor_debug_switch;
    oal_uint8               uc_force_work_switch;
    oal_uint8               uc_dfr_reset_switch;         /* dfr_reset调试命令: 高4bit为reset_mac_submod, 低4bit为reset_hw_type */
    oal_uint8               uc_fsm_info_switch;          /* hal fsm debug info */
    oal_uint8               uc_report_radar_switch;      /* radar上报开关 */
    oal_uint8               uc_extlna_chg_bypass_switch; /* 功耗测试关闭外置LNA开关: 0/1/2:no_bypass/dyn_bypass/force_bypass */
    oal_uint8               uc_edca_param_switch;        /* EDCA参数设置开关 */
    oal_uint8               uc_edca_aifsn;      /* edca参数AIFSN */
    oal_uint8               uc_edca_cwmin;      /* edca参数CWmin */
    oal_uint8               uc_edca_cwmax;      /* edca参数CWmax */
    oal_uint16              us_edca_txoplimit;      /* edca参数TXOP limit */

}mac_phy_debug_switch_stru;

typedef struct
{
    wlan_csa_mode_tx_enum_uint8         en_mode;
    oal_uint8                           uc_channel;
    oal_uint8                           uc_cnt;
    wlan_channel_bandwidth_enum_uint8   en_bandwidth;
    mac_csa_flag_enum_uint8             en_debug_flag;/*0:正常切信道; 1:仅beacon帧中含有csa,信道不切换;2:取消beacon帧中含有csa*/
    oal_uint8                           auc_reserv[3];
}mac_csa_debug_stru;


/* 带宽调试开关相关的结构体 */
typedef struct
{
    oal_uint32                              ul_cmd_bit_map;
    oal_bool_enum_uint8                     en_band_force_switch_bit0;    /* 恢复40M带宽命令*/
    oal_bool_enum_uint8                     en_2040_ch_swt_prohi_bit1;    /* 不允许20/40带宽切换开关*/
    oal_bool_enum_uint8                     en_40_intolerant_bit2;        /* 不容忍40M带宽开关*/
    oal_uint8                               uc_resv;
    mac_csa_debug_stru                      st_csa_debug_bit3;
#ifdef _PRE_WLAN_FEATURE_HWBW_20_40
    oal_bool_enum_uint8                     en_2040_user_switch_bit4;
    oal_uint8                               auc_resv[3];
#endif
    oal_bool_enum_uint8                     en_lsigtxop_bit5;           /*lsigtxop使能*/
    oal_uint8                               auc_resv0[3];
}mac_protocol_debug_switch_stru;

#ifdef _PRE_WLAN_FEATURE_FTM
/* FTM调试开关相关的结构体 */
typedef struct
{
    oal_uint8             uc_channel_num;
    oal_uint8             uc_burst_num;
    oal_bool_enum_uint8   measure_req;
    oal_uint8             uc_ftms_per_burst;

    oal_bool_enum_uint8   en_asap;
    oal_uint8             auc_resv[1];
    oal_uint8             auc_bssid[WLAN_MAC_ADDR_LEN];
}mac_send_iftmr_stru;

typedef struct
{
    oal_uint32             ul_ftm_correct_time1;
    oal_uint32             ul_ftm_correct_time2;
    oal_uint32             ul_ftm_correct_time3;
    oal_uint32             ul_ftm_correct_time4;
}mac_set_ftm_time_stru;

typedef struct
{
    oal_uint8             auc_resv[2];
    oal_uint8             auc_mac[WLAN_MAC_ADDR_LEN];
}mac_send_ftm_stru;

typedef struct
{
    oal_uint8                           auc_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                           uc_dialog_token;
    oal_uint8                           uc_meas_token;

    oal_uint16                          us_num_rpt;

    oal_uint16                          us_start_delay;
    oal_uint8                           auc_reserve[1];
    oal_uint8                           uc_minimum_ap_count;
    oal_uint8                           aauc_bssid[MAX_MINIMUN_AP_COUNT][WLAN_MAC_ADDR_LEN];
    oal_uint8                           auc_channel[MAX_MINIMUN_AP_COUNT];
} mac_send_ftm_range_req_stru; /*和ftm_range_req_stru 同步修改*/

//typedef struct
//{
//    oal_location_type_enum_uint8            en_location_type;                                        /*定位类型 0:关闭定位 1:root ap;2:repeater;3station*/
//    oal_uint8                               auc_location_ap[MAX_REPEATER_NUM][WLAN_MAC_ADDR_LEN];    /*各location ap的mac地址，第一个为root */
//    oal_uint8                               auc_station[WLAN_MAC_ADDR_LEN];                          /*STA MAC地址，暂时只支持一个sta的测量*/
//} mac_location_stru;

typedef struct
{
    oal_uint8                               uc_tx_chain_selection;
    oal_bool_enum_uint8                     en_is_mimo;
    oal_uint8                               auc_reserve[2];
} ftm_m2s_stru;

typedef struct
{
    oal_uint32                      ul_cmd_bit_map;

    oal_bool_enum_uint8             en_ftm_initiator_bit0;        /* ftm_initiator命令*/
    mac_send_iftmr_stru             st_send_iftmr_bit1;           /* 发送iftmr命令*/
    oal_bool_enum_uint8             en_enable_bit2;               /* 使能FTM*/
    oal_bool_enum_uint8             en_cali_bit3;                 /* FTM环回*/
    mac_send_ftm_stru               st_send_ftm_bit4;             /* 发送ftm命令*/
    oal_bool_enum_uint8             en_ftm_resp_bit5;             /* ftm_resp命令*/
    mac_set_ftm_time_stru           st_ftm_time_bit6;             /* ftm_time命令*/
    mac_send_ftm_range_req_stru     st_send_range_req_bit7;       /* 发送ftm_range_req命令*/
    oal_bool_enum_uint8             en_ftm_range_bit8;            /* ftm_range命令*/
    oal_uint8                       uc_get_cali_reserv_bit9;
    //mac_location_stru               st_location_bit10;
    ftm_m2s_stru                    st_m2s_bit11;
}mac_ftm_debug_switch_stru;
#endif
typedef struct
{
    oal_uint8                uc_category;
    oal_uint8                uc_action_code;
    oal_uint8                auc_oui[3];
    oal_uint8                uc_eid;
    oal_uint8                uc_lenth;
    oal_uint8                uc_location_type;
    oal_uint8                auc_mac_server[WLAN_MAC_ADDR_LEN];
    oal_uint8                auc_mac_client[WLAN_MAC_ADDR_LEN];
    oal_uint8                auc_payload[4];
}mac_location_event_stru;

typedef enum
{
    MAC_PM_DEBUG_SISO_RECV_BCN = 0,
    MAC_PM_DEBUG_DYN_TBTT_OFFSET = 1,
    MAC_PM_DEBUG_NO_PS_FRM_INT = 2,
    MAC_PM_DEBUG_APF = 3,
    MAC_PM_DEBUG_AO = 4,

    MAC_PM_DEBUG_CFG_BUTT
}mac_pm_debug_cfg_enum_uint8;

typedef struct
{
    oal_uint32            ul_cmd_bit_map;
    oal_uint8             uc_srb_switch; /* siso收beacon开关 */
    oal_uint8             uc_dto_switch; /* 动态tbtt offset开关 */
    oal_uint8             uc_nfi_switch;
    oal_uint8             uc_apf_switch;
    oal_uint8             uc_ao_switch;
}mac_pm_debug_cfg_stru;

typedef enum
{
    MAC_DBDC_CHANGE_HAL_DEV = 0,   /* vap change hal device hal vap */
    MAC_DBDC_SWITCH         = 1,   /* DBDC软件开关 */
    MAC_FAST_SCAN_SWITCH    = 2,   /* 并发扫描开关 */
    MAC_DBDC_STATUS         = 3,   /* DBDC状态查询 */

    MAC_DBDC_CMD_BUTT
}mac_dbdc_cmd_enum;
typedef oal_uint8 mac_dbdc_cmd_enum_uint8;

typedef struct
{
    oal_uint32            ul_cmd_bit_map;
    oal_uint8             uc_dst_hal_dev_id; /*需要迁移到的hal device id */
    oal_uint8             uc_dbdc_enable;
    oal_bool_enum_uint8   en_fast_scan_enable; /*是否可以并发,XX原因即使硬件支持也不能快速扫描*/
    oal_uint8             uc_dbdc_status;
}mac_dbdc_debug_switch_stru;

/* ACS 命令及回复格式 */
typedef struct
{
    oal_uint8  uc_cmd;
    oal_uint8  uc_chip_id;
    oal_uint8  uc_device_id;
    oal_uint8  uc_resv;

    oal_uint32 ul_len;      /* 总长度，包括上面前4个字节 */
    oal_uint32 ul_cmd_cnt;  /* 命令的计数 */
}mac_acs_response_hdr_stru;

typedef struct
{
    oal_uint8   uc_cmd;
    oal_uint8   auc_arg[3];
    oal_uint32  ul_cmd_cnt;  /* 命令的计数 */
    oal_uint32  ul_cmd_len;  /* 总长度，特指auc_data的实际负载长度 */
    oal_uint8   auc_data[4];
}mac_acs_cmd_stru;

typedef mac_acs_cmd_stru    mac_init_scan_req_stru;

typedef enum
{
    MAC_ACS_RSN_INIT,
    MAC_ACS_RSN_LONG_TX_BUF,
    MAC_ACS_RSN_LARGE_PER,
    MAC_ACS_RSN_MWO_DECT,
    MAC_ACS_RSN_RADAR_DECT,

    MAC_ACS_RSN_BUTT
}mac_acs_rsn_enum;
typedef oal_uint8 mac_acs_rsn_enum_uint8;

typedef enum
{
    MAC_ACS_SW_NONE = 0x0,
    MAC_ACS_SW_INIT = 0x1,
    MAC_ACS_SW_DYNA = 0x2,
    MAC_ACS_SW_BOTH = 0x3,

    MAC_ACS_SW_BUTT
}en_mac_acs_sw_enum;
typedef oal_uint8 en_mac_acs_sw_enum_uint8;

typedef enum
{
    MAC_ACS_SET_CH_DNYA = 0x0,
    MAC_ACS_SET_CH_INIT = 0x1,

    MAC_ACS_SET_CH_BUTT
}en_mac_acs_set_ch_enum;
typedef oal_uint8 en_mac_acs_set_ch_enum_uint8;

typedef struct
{
    oal_bool_enum_uint8               en_sw_when_connected_enable : 1;
    oal_bool_enum_uint8               en_drop_dfs_channel_enable  : 1;
    oal_bool_enum_uint8               en_lte_coex_enable          : 1;
    en_mac_acs_sw_enum_uint8          en_acs_switch               : 5;
}mac_acs_switch_stru;

/* DMAC SCAN 信道扫描BSS信息摘要结构 */
typedef struct
{

    oal_int8                            c_rssi;             /* bss的信号强度 */
    oal_uint8                           uc_channel_number;  /* 信道号 */
    oal_uint8                           auc_bssid[WLAN_MAC_ADDR_LEN];

    /* 11n, 11ac信息 */
    oal_bool_enum_uint8                 en_ht_capable;             /* 是否支持ht */
    oal_bool_enum_uint8                 en_vht_capable;            /* 是否支持vht */
    wlan_bw_cap_enum_uint8              en_bw_cap;                 /* 支持的带宽 0-20M 1-40M */
    wlan_channel_bandwidth_enum_uint8   en_channel_bandwidth;      /* 信道带宽配置 */
}mac_scan_bss_stats_stru;

typedef struct
{
    oal_int8                            c_rssi;             /* bss的信号强度 */
    oal_uint8                           uc_channel_number;  /* 信道号 */

    oal_bool_enum_uint8                 en_ht_capable   : 1;             /* 是否支持ht */
    oal_bool_enum_uint8                 en_vht_capable  : 1;            /* 是否支持vht */
    wlan_bw_cap_enum_uint8              en_bw_cap       : 3;            /* 支持的带宽 0-20M 1-40M */
    wlan_channel_bandwidth_enum_uint8   en_channel_bandwidth : 3;      /* 信道带宽配置 */
}mac_scan_bss_stats_simple_stru;

typedef struct
{
    oal_uint32  us_total_stats_time_ms  : 9;  // max 512 ms
    oal_uint32  uc_bandwidth_mode       : 3;
    oal_uint32  uc_radar_detected       : 1;  // FIXME: feed
    oal_uint32  uc_dfs_check_needed     : 1;
    oal_uint32  uc_radar_bw             : 3;
    oal_uint32  uc_radar_type           : 4;
    oal_uint32  uc_radar_freq_offset    : 3;
    oal_uint8   uc_channel_number;      /* 信道号 */

    oal_int8    s_free_power_20M;           // dBm
    oal_int8    s_free_power_40M;
    oal_int8    s_free_power_80M;
    oal_uint8   uc_free_time_20M_rate;      // percent, 255 scaled
    oal_uint8   uc_free_time_40M_rate;
    oal_uint8   uc_free_time_80M_rate;
    oal_uint8   uc_total_send_time_rate;    // percent, 255 scaled
    oal_uint8   uc_total_recv_time_rate;
}mac_scan_chan_stats_simple_stru;

/* DMAC SCAN 回调事件结构体 */
typedef struct
{
    oal_uint8                           uc_nchans;      /* 信道数量       */
    oal_uint8                           uc_nbss;        /* BSS数量 */
    oal_uint8                           uc_scan_func;   /* 扫描启动的功能 */

    oal_uint8                           uc_need_rank    : 1; // kernel write, app read
    oal_uint8                           uc_obss_on      : 1;
    oal_uint8                           uc_dfs_on       : 1;
    oal_uint8                           uc_dbac_on      : 1;
    oal_uint8                           uc_chip_id      : 2;
    oal_uint8                           uc_device_id    : 2;

    oal_uint8                           uc_acs_type;    /* 0:初始acs,1:命令触发acs,2:空闲时acs*/
    wlan_channel_band_enum_uint8        en_band;
    oal_uint8                           uc_pre_channel;
    wlan_channel_bandwidth_enum_uint8   en_pre_bw;
    oal_uint32                          ul_time_stamp;
    oal_bool_enum_uint8                 en_switch_chan;
    oal_uint8                           auc_resv[3];
}mac_scan_event_stru;


#ifdef _PRE_WLAN_FEATURE_DFS
typedef struct
{
    oal_bool_enum_uint8    en_dfs_switch;                               /* DFS使能开关 bit0:dfs使能,bit1:标示AP因为DFS特性暂时处于邋wait_start */
    oal_bool_enum_uint8    en_cac_switch;
    oal_bool_enum_uint8    en_offchan_cac_switch;
    oal_uint8              uc_debug_level;                              /* bit0:打印雷达带业务，bit1:打印雷达无业务 */
    oal_uint8              uc_offchan_flag;                             /* bit0:0表示homechan,1表示offchan; bit1:0表示普通,1表示offchancac */
    oal_uint8              uc_offchan_num;
    oal_uint8              uc_timer_cnt;
    oal_uint8              uc_timer_end_cnt;
    oal_uint8              uc_cts_duration;
    oal_uint8              uc_dmac_channel_flag;                        /* dmac用于标示当前信道off or home */
    oal_bool_enum_uint8    en_dfs_init;
    oal_uint8              uc_custom_next_chnum;                        /* 应用层指定的DFS下一跳信道号 */
    oal_uint32             ul_dfs_cac_outof_5600_to_5650_time_ms;       /* CAC检测时长，5600MHz ~ 5650MHz频段外，默认60秒 */
    oal_uint32             ul_dfs_cac_in_5600_to_5650_time_ms;          /* CAC检测时长，5600MHz ~ 5650MHz频段内，默认10分钟 */
    oal_uint32             ul_off_chan_cac_outof_5600_to_5650_time_ms;  /* Off-Channel CAC检测时长，5600MHz ~ 5650MHz频段外，默认6分钟 */
    oal_uint32             ul_off_chan_cac_in_5600_to_5650_time_ms;     /* Off-Channel CAC检测时长，5600MHz ~ 5650MHz频段内，默认60分钟 */
    oal_uint16             us_dfs_off_chan_cac_opern_chan_dwell_time;   /* Off-channel CAC在工作信道上驻留时长 */
    oal_uint16             us_dfs_off_chan_cac_off_chan_dwell_time;     /* Off-channel CAC在Off-Channel信道上驻留时长 */
    oal_uint32             ul_radar_detected_timestamp;
    oal_int32              l_radar_th;                                 //设置的雷达检测门限，单位db
    oal_uint32             ul_custom_chanlist_bitmap;                  //应用层同步下来的信道列表
                                                                       //(0x0000000F) /*36--48*/
                                                                       //(0x000000F0) /*52--64*/
                                                                       //(0x000FFF00) /*100--144*/
                                                                       //(0x01F00000) /*149--165*/
    wlan_channel_bandwidth_enum_uint8   en_next_ch_width_type;         //设置的下一跳信道的带宽模式
    oal_uint8                           uac_resv[3];
    oal_uint32             ul_dfs_non_occupancy_period_time_ms;
#ifdef _PRE_WLAN_FEATURE_DFS_ENABLE
    oal_uint8              uc_radar_type;
    oal_uint8              uc_band;
    oal_uint8              uc_channel_num;
    oal_uint8              uc_flag;
#else
    oal_uint8              _rom[4];
#endif


}mac_dfs_info_stru;

typedef struct
{
    oal_uint8             uc_chan_idx;        /* 信道索引 */
    oal_uint8             uc_device_id;       /* device id */
    oal_uint8             auc_resv[2];
    frw_timeout_stru      st_dfs_nol_timer;   /* NOL节点定时器 */
    oal_dlist_head_stru   st_entry;           /* NOL链表 */
}mac_dfs_nol_node_stru;

typedef struct
{
    frw_timeout_stru      st_dfs_cac_timer;                   /* CAC定时器 */
    frw_timeout_stru      st_dfs_off_chan_cac_timer;          /* Off-Channel CAC定时器 */
    frw_timeout_stru      st_dfs_chan_dwell_timer;            /* 信道驻留定时器，定时器到期，切离该信道 */
    frw_timeout_stru      st_dfs_radar_timer;
    mac_dfs_info_stru     st_dfs_info;
    oal_dlist_head_stru   st_dfs_nol;
}mac_dfs_core_stru;
#endif


/* 扫描完成事件返回状态码 */
typedef enum
{
    MAC_SCAN_SUCCESS = 0,       /* 扫描成功 */
    MAC_SCAN_PNO     = 1,       /* pno扫描结束 */
    MAC_SCAN_TIMEOUT = 2,       /* 扫描超时 */
    MAC_SCAN_REFUSED = 3,       /* 扫描被拒绝 */
    MAC_SCAN_ABORT   = 4,       /* 终止扫描 */
    MAC_SCAN_ABORT_SYNC = 5,    /* 扫描被终止同步状态，用于上层去关联命令时强制abort不往内核上报等dmac响应abort往上报 */
    MAC_SCAN_STATUS_BUTT,       /* 无效状态码，初始化时使用此状态码 */
}mac_scan_status_enum;
typedef oal_uint8   mac_scan_status_enum_uint8;

/* 扫描结果 */
typedef struct
{
    mac_scan_status_enum_uint8  en_scan_rsp_status;
    oal_uint8                   auc_resv[3];
    oal_uint64                  ull_cookie;
}mac_scan_rsp_stru;


/* 扫描到的BSS描述结构体 */
typedef struct
{
    /* 基本信息 */
    wlan_mib_desired_bsstype_enum_uint8 en_bss_type;                        /* bss网络类型 */
    oal_uint8                           uc_dtim_period;                     /* dtime周期 */
    oal_uint8                           uc_dtim_cnt;                        /* dtime cnt */
    oal_bool_enum_uint8                 en_11ntxbf;                         /* 11n txbf */
    oal_bool_enum_uint8                 en_new_scan_bss;                    /* 是否是新扫描到的BSS */
    wlan_ap_chip_oui_enum_uint8         en_is_tplink_oui;
    oal_int8                            c_rssi;                             /* bss的信号强度 */
    oal_int8                            ac_ssid[WLAN_SSID_MAX_LEN];         /* 网络ssid */
    oal_uint16                          us_beacon_period;                   /* beacon周期 */
    oal_uint16                          us_cap_info;                        /* 基本能力信息 */
    oal_uint8                           auc_mac_addr[WLAN_MAC_ADDR_LEN];    /* 基础型网络 mac地址与bssid相同 */
    oal_uint8                           auc_bssid[WLAN_MAC_ADDR_LEN];       /* 网络bssid */
    mac_channel_stru                    st_channel;                         /* bss所在的信道 */
    oal_uint8                           uc_wmm_cap;                         /* 是否支持wmm */
    oal_uint8                           uc_uapsd_cap;                       /* 是否支持uapsd */
    oal_bool_enum_uint8                 en_desired;                         /* 标志位，此bss是否是期望的 */
    oal_uint8                           uc_num_supp_rates;                  /* 支持的速率集个数 */
    oal_uint8                           auc_supp_rates[WLAN_USER_MAX_SUPP_RATES];/* 支持的速率集 */

#ifdef _PRE_WLAN_FEATURE_11D
    oal_int8                            ac_country[WLAN_COUNTRY_STR_LEN];   /* 国家字符串 */
    oal_uint8                           auc_resv2[1];
    oal_uint8                          *puc_country_ie;                     /* 用于存放country ie */
#endif

    /* 安全相关的信息 */
    oal_uint8                          *puc_rsn_ie;          /* 用于存放beacon、probe rsp的rsn ie */
    oal_uint8                          *puc_wpa_ie;          /* 用于存放beacon、probe rsp的wpa ie */

    /* 11n 11ac信息 */
    oal_bool_enum_uint8                 en_ht_capable;                      /* 是否支持ht */
    oal_bool_enum_uint8                 en_vht_capable;                     /* 是否支持vht */
    oal_bool_enum_uint8                 en_vendor_vht_capable;              /* 是否支持hidden vendor vht */
    wlan_bw_cap_enum_uint8              en_bw_cap;                          /* 支持的带宽 0-20M 1-40M */
    wlan_channel_bandwidth_enum_uint8   en_channel_bandwidth;               /* 信道带宽 */
    oal_uint8                           uc_coex_mgmt_supp;                  /* 是否支持共存管理 */
    oal_bool_enum_uint8                 en_ht_ldpc;                         /* 是否支持ldpc */
    oal_bool_enum_uint8                 en_ht_stbc;                         /* 是否支持stbc */
    oal_uint8                           uc_wapi;
    oal_uint8                           en_vendor_novht_capable;            /* 私有vendor中不需再携带 */
#ifdef _PRE_WLAN_1103_DDC_BUGFIX
    oal_bool_enum_uint8                 en_ddc_whitelist_chip_oui;
#else
    oal_uint8                           uc_resv0;
#endif
    oal_bool_enum_uint8                 en_btcoex_blacklist_chip_oui;       /* ps机制one pkt帧类型需要修订为self-cts等 */
    oal_uint32                          ul_timestamp;                       /* 更新此bss的时间戳 */
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    oal_uint8                           uc_phy_type;
    oal_int8                            ac_rsni[2];
    oal_uint8                           auc_resv3[1];
    oal_uint32                          ul_parent_tsf;
#endif
#ifdef _PRE_WLAN_WEB_CMD_COMM
    oal_uint32                          ul_max_rate_kbps;                   /* 该AP支持的最高速率(单位:kbps) */
    oal_uint8                           uc_max_nss;                         /* 该AP支持的最大空间流数 */
    oal_uint8                           auc_resv4[3];
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    wlan_nss_enum_uint8                 en_support_max_nss;                 /* 该AP支持的最大空间流数 */
    oal_bool_enum_uint8                 en_support_opmode;                  /* 该AP是否支持OPMODE */
    oal_uint8                           uc_num_sounding_dim;                /* 该AP发送txbf的天线数 */
#endif

#if defined(_PRE_WLAN_FEATURE_11AX) || defined(_PRE_WLAN_FEATURE_11AX_ROM)
    oal_bool_enum_uint8                 en_he_capable;                      /*是否支持11ax*/
#endif

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN) || defined(_PRE_WLAN_FEATURE_FTM) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)
    oal_bool_enum_uint8                 en_support_rrm;                     /*是否支持RRM*/
#endif

#ifdef _PRE_WLAN_FEATURE_1024QAM
    oal_bool_enum_uint8                 en_support_1024qam;
#endif

#ifdef _PRE_WLAN_NARROW_BAND
    oal_bool_enum_uint8                 en_nb_capable;                      /* 是否支持nb */
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    oal_bool_enum_uint8                 en_roam_blacklist_chip_oui;         /* 不支持roam */
#endif

    oal_int8                            c_ant0_rssi;                        /* 天线0的rssi */
    oal_int8                            c_ant1_rssi;                        /* 天线1的rssi */

    /* 管理帧信息 */
    oal_uint32                          ul_mgmt_len;                        /* 管理帧的长度 */
    oal_uint8                           auc_mgmt_buff[MAC_80211_QOS_HTC_4ADDR_FRAME_LEN];  /* 记录beacon帧或probe rsp帧 */
    //oal_uint8                         *puc_mgmt_buff;                      /* 记录beacon帧或probe rsp帧 */
}mac_bss_dscr_stru;

#ifdef _PRE_WLAN_DFT_STAT
/* 管理帧统计信息 */
typedef struct
{
    /* 接收管理帧统计 */
    oal_uint32          aul_rx_mgmt[WLAN_MGMT_SUBTYPE_BUTT];

    /* 挂到硬件队列的管理帧统计 */
    oal_uint32          aul_tx_mgmt_soft[WLAN_MGMT_SUBTYPE_BUTT];

    /* 发送完成的管理帧统计 */
    oal_uint32          aul_tx_mgmt_hardware[WLAN_MGMT_SUBTYPE_BUTT];
}mac_device_mgmt_statistic_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
typedef struct
{
    oal_bool_enum_uint8             en_psta_enable;
    oal_uint8                       uc_proxysta_cnt;                                        /* 创建proxy sta的个数(不包括main ProxySTA) */
    oal_uint8                       auc_resv[2];
}mac_device_psta_stru;

#define mac_dev_xsta_num(dev) ((dev)->st_psta.uc_proxysta_cnt)
#define mac_is_proxysta_enabled(pst_dev) ((pst_dev)->st_psta.en_psta_enable)
#endif


#ifdef _PRE_WLAN_DFT_STAT
/* 上报空口环境类维测参数的控制结构 */
typedef struct
{
    oal_uint32                          ul_non_directed_frame_num;             /* 接收到非本机帧的数目 */
    oal_uint8                           uc_collect_period_cnt;                 /* 采集周期的次数，到达100后就上报参数，然后清零重新开始 */
    oal_bool_enum_uint8                 en_non_directed_frame_stat_flg;        /* 是否统计非本机地址帧个数的标志 */
    oal_int16                           s_ant_power;                           /* 天线口功率 */
    frw_timeout_stru                    st_collect_period_timer;               /* 采集周期定时器 */
}mac_device_dbb_env_param_ctx_stru;
#endif

typedef enum
{
    MAC_DFR_TIMER_STEP_1 = 0,
    MAC_DFR_TIMER_STEP_2 = 1,

}mac_dfr_timer_step_enum;
typedef oal_uint8 mac_dfr_timer_step_enum_uint8;

typedef struct
{
    oal_uint32                         ul_tx_seqnum;                           /* 最近一次tx上报的SN号 */
    oal_uint16                         us_seqnum_used_times;                   /* 软件使用了ul_tx_seqnum的次数 */
    oal_uint16                         us_incr_constant;                       /* 维护非Qos 分片帧的递增常量 */
}mac_tx_seqnum_struc;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
typedef struct
{
    oal_bool_enum_uint8                 en_brk_limit_aggr_enable;
    oal_uint8                           uc_resv[3];
    oal_uint32                          ul_tx_dataflow_brk_cnt;
    oal_uint32                          ul_last_tx_complete_isr_cnt;
}mac_tx_dataflow_brk_bypass_struc;
#endif

/* 屏幕状态 */
typedef enum
{
    MAC_SCREEN_OFF = 0,
    MAC_SCREEN_ON  = 1,
}mac_screen_state_enum;

#ifdef _PRE_WLAN_FEATURE_HILINK
/* 记录未关联单个用户侦听结果 */
typedef struct
{
    oal_uint8   uc_is_used;                 /* 该用户是否被使用 */
    oal_uint8   uc_user_channel;            /* 网卡当前工作的信道 */
    oal_uint8   auc_user_bssid[6];          /* 网卡关联的当前AP的bssid */
    oal_uint8   auc_user_mac_addr[6];       /* 当前网卡的MAC地址 */
    oal_int16   s_rssi;                     /* 当前网卡的信号强度 */
    oal_uint8   uc_is_found;                /* 0-没有找到该网卡，1-找到该网卡 */
    oal_uint32  ul_rssi_timestamp;          /* 接收到该帧的时间戳 */
    oal_uint32  ul_total_pkt_cnt;           /* 接收到该STA的总包数目 */
}mac_fbt_scan_result_stru;


/* 记录未关联用户侦听信息的结构体 */
typedef struct
{
    oal_uint8  uc_fbt_scan_enable;              /* 配置未关联用户侦听特性开关 */
    oal_uint8  uc_scan_channel;                 /* 配置侦听信道，默认为0，表示当前信道 */
    oal_uint8  auc_reserve[2];
    oal_uint32 ul_scan_interval;                /* 配置未关联用户每个信道侦听的最大时长，单位为毫秒 */
    oal_uint32 ul_scan_report_period;           /* 配置侦听上报周期 */
    oal_uint16 us_scan_channel_stay_time;       /* 侦听信道驻留时长 */
    oal_uint16 us_work_channel_stay_time;       /* 工作信道驻留时长 */
    oal_uint32 ul_scan_timestamp;               /* 扫描开始时间戳 */
    mac_fbt_scan_result_stru ast_fbt_scan_user_list[HMAC_FBT_MAX_USER_NUM]; /* 记录32个用户的侦听结果信息 */
    frw_timeout_stru st_timer;                  /* 侦听未关联用户信息上报定时器 */
    frw_timeout_stru st_scan_timer;             /* 侦听未关联用户使用的扫描定时器 */
}mac_fbt_scan_mgmt_stru;

/* 快速切换侦听指定用户 fbt_monitor_specified_sta命令用于传递mac地址*/
typedef struct
{
    oal_uint8               auc_mac_addr[WLAN_MAC_ADDR_LEN];    /* MAC地址 */
    oal_uint8               auc_resv[2];
}mac_fbt_scan_sta_addr_stru;

#endif

#ifdef _PRE_WLAN_MAC_ADDR_EDCA_FIX
typedef struct
{
    oal_bool_enum_uint8     en_enable;
    oal_uint8               uc_up_edca;
    oal_uint8               uc_down_edca;
    oal_uint8               uc_resv[1];
}mac_custom_edca_stru;
#endif

typedef struct
{

    /* 支持2*2 */    /* 支持MU-MIMO */
    wlan_nss_enum_uint8                     en_nss_num;              /* device Nss 空间流最大个数 */
    wlan_bw_cap_enum_uint8                  en_channel_width;        /* 支持的带宽 */
    oal_bool_enum_uint8                     en_nb_is_supp;           /* 支持窄带 */
    oal_bool_enum_uint8                     en_1024qam_is_supp;      /* 支持1024QAM速率 */

    oal_bool_enum_uint8                     en_80211_mc_is_supp;    /* 支持80211 mc */
    oal_bool_enum_uint8                     en_ldpc_is_supp;        /* 是否支持接收LDPC编码的包 */
    oal_bool_enum_uint8                     en_tx_stbc_is_supp;     /* 是否支持最少2x1 STBC发送 */
    oal_bool_enum_uint8                     en_rx_stbc_is_supp;     /* 是否支持stbc接收,支持2个空间流 */

    oal_bool_enum_uint8                     en_su_bfmer_is_supp;     /* 是否支持单用户beamformer */
    oal_bool_enum_uint8                     en_su_bfmee_is_supp;     /* 是否支持单用户beamformee */
    oal_bool_enum_uint8                     en_mu_bfmer_is_supp;     /* 是否支持多用户beamformer */
    oal_bool_enum_uint8                     en_mu_bfmee_is_supp;     /* 是否支持多用户beamformee */
    oal_bool_enum_uint8                     en_11ax_switch;          /*11ax开关*/
    oal_uint8                               _rom[3];
}mac_device_capability_stru;

typedef struct
{
    oal_bool_enum_uint8                     en_11k;
    oal_bool_enum_uint8                     en_11v;
    oal_bool_enum_uint8                     en_11r;
    oal_uint8                               auc_rsv[1];
}mac_device_voe_custom_stru;

typedef struct
{
    mac_m2s_mode_enum_uint8         en_cfg_m2s_mode;     /* 0:参数查询模式; 1:参数配置模式;2.切换模式;3.软切换测试模式;4.硬切换测试模式 5.RSSI配置命令 */
    union
    {
        struct
        {
            oal_bool_enum_uint8      en_m2s_type;    /* 切换类型 */
            oal_uint8                uc_master_id;   /* 主辅路id */
            mac_m2s_command_state_enum_uint8 uc_m2s_state;   /* 期望切换到状态 */
            wlan_m2s_trigger_mode_enum_uint8 uc_trigger_mode; /* 切换触发业务模式 */
        }test_mode;

        struct
        {
            oal_bool_enum_uint8  en_mss_on;
        }mss_mode;

        struct
        {
            oal_uint8                uc_opt;
            oal_int8                 c_value;
        }rssi_mode;
    }pri_data;
}mac_m2s_mgr_stru;

typedef struct
{
    wlan_m2s_mgr_vap_stru           ast_m2s_blacklist[WLAN_M2S_BLACKLIST_MAX_NUM];
    oal_uint8                       uc_blacklist_cnt;
}mac_m2s_ie_stru;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
typedef struct
{
    oal_uint8                uc_cfg_btcoex_mode;     /* 0:参数查询模式; 1:参数配置模式 */
    oal_uint8                uc_cfg_btcoex_type;     /* 0:门限类型; 1:聚合大小类型 2.rssi detect门限参数配置模式 */
    union
    {
        struct
        {
            wlan_nss_enum_uint8      en_btcoex_nss;          /* 0:siso 1:mimo */
            oal_uint8                uc_20m_low;   /* 2G 20M low */
            oal_uint8                uc_20m_high;  /* 2G 20M high */
            oal_uint8                uc_40m_low;   /* 2G 40M low */
            oal_uint16               us_40m_high;  /* 2G 40M high */
        }threhold;
        struct
        {
            wlan_nss_enum_uint8      en_btcoex_nss;          /* 0:siso 1:mimo */
            oal_uint8                uc_grade;   /* 0级或者1级 */
            oal_uint8                uc_rx_size0;   /* size0大小 */
            oal_uint8                uc_rx_size1;   /* size1大小 */
            oal_uint8                uc_rx_size2;   /* size2大小 */
            oal_uint8                uc_rx_size3;   /* size3大小 */
        }rx_size;
        struct
        {
            oal_bool_enum_uint8      en_rssi_limit_on;
            oal_bool_enum_uint8      en_rssi_log_on;
            oal_uint8                uc_cfg_rssi_detect_cnt;     /* 6 rssi 配置时用于防护次数  高低门限配置 */
            oal_int8                 c_cfg_rssi_detect_m2s_th;
            oal_int8                 c_cfg_rssi_detect_s2m_th;
        }rssi_param;
    }pri_data;
}mac_btcoex_mgr_stru;

typedef struct
{
    oal_uint8                uc_cfg_preempt_mode;     /* 0:硬件preempt; 1:软件preempt 2:ps 提前slot量*/
    oal_uint8                uc_cfg_preempt_type;     /* 0 noframe 1 self-cts 2 nulldata 3 qosnull  0/1 软件ps打开或者关闭 */
}mac_btcoex_preempt_mgr_stru;
#endif

/* device结构体 */
typedef struct
{
    oal_uint32                          ul_core_id;
    oal_uint8                           auc_vap_id[WLAN_SERVICE_VAP_MAX_NUM_PER_DEVICE];   /* device下的业务vap，此处只记录VAP ID */
    oal_uint8                           uc_cfg_vap_id;                          /* 配置vap ID */
    oal_uint8                           uc_device_id;                           /* 芯片ID */
    oal_uint8                           uc_chip_id;                             /* 设备ID */
    oal_uint8                           uc_device_reset_in_progress;            /* 复位处理中*/

    oal_bool_enum_uint8                 en_device_state;                        /* 标识是否已经被分配，(OAL_TRUE初始化完成，OAL_FALSE未初始化 ) */
    oal_uint8                           uc_vap_num;                             /* 当前device下的业务VAP数量(AP+STA) */
    oal_uint8                           uc_sta_num;                             /* 当前device下的STA数量 */
/* begin: P2P */
    mac_p2p_info_stru                   st_p2p_info;                            /* P2P 相关信息 */
/* end: P2P */

    oal_uint8                           auc_hw_addr[WLAN_MAC_ADDR_LEN];         /* 从eeprom或flash获得的mac地址，ko加载时调用hal接口赋值 */
    /* device级别参数 */
    oal_uint8                           uc_max_channel;                         /* 已配置VAP的信道号，其后的VAP配置值不能与此值矛盾，仅在非DBAC时使用 */
    wlan_channel_band_enum_uint8        en_max_band;                            /* 已配置VAP的频段，其后的VAP配置值不能与此值矛盾，仅在非DBAC时使用 */

    oal_bool_enum_uint8                 en_wmm;                                 /* wmm使能开关 */
    wlan_tidno_enum_uint8               en_tid;
    oal_uint8                           en_reset_switch;                        /* 是否使能复位功能*/
    oal_uint8                           uc_csa_vap_cnt;                         /* 每个running AP发送一次CSA帧，该计数减1，减到零后，AP停止当前硬件发送，准备开始切换信道 */

    oal_uint32                          ul_beacon_interval;                     /*device级别beacon interval,device下所有VAP约束为同一值*/

    oal_bool_enum_uint8                 en_delayed_shift;                       /* 退出dbdc时候延迟迁移 */
    oal_uint8                           uc_auth_req_sendst;
    oal_uint8                           uc_asoc_req_sendst;
    oal_uint8                           auc_resv0[1];

    oal_uint32                          ul_resv1;
    oal_uint32                          ul_resv2;
    oal_uint32                          ul_resv3;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_bool_enum_uint8                 en_is_wavetest;                         /* wavetest用户识别 1:是  0:不是    */
    oal_uint8                           uc_lock_channel;                        /* AGC绑定通道 0:绑定通道0  1:绑定通道1   2:自适应  */
    oal_uint8                           auc_rev[2];

    wlan_chan_ratio_stru                 st_chan_ratio;                          /* 信道繁忙度相关统计量 */
#endif

    /* device能力 */
    wlan_protocol_cap_enum_uint8        en_protocol_cap;                        /* 协议能力 */
    wlan_band_cap_enum_uint8            en_band_cap;                            /* 频段能力 */
    wlan_channel_bandwidth_enum_uint8   en_max_bandwidth;                       /* 已配置VAP的最带带宽值，其后的VAP配置值不能与此值矛盾，仅在非DBAC时使用 */

    oal_int16                           s_upc_amend;                            /* UPC修正值 */

    oal_uint16                          us_device_reset_num;                    /* 复位的次数统计*/
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_device_psta_stru                st_psta;
#endif

    mac_data_rate_stru                  st_mac_rates_11g[MAC_DATARATES_PHY_80211G_NUM];  /* 11g速率 */

    mac_pno_sched_scan_mgmt_stru       *pst_pno_sched_scan_mgmt;                    /* pno调度扫描管理结构体指针，内存动态申请，从而节省内存 */
    mac_scan_req_stru                   st_scan_params;                             /* 最新一次的扫描参数信息 */
    frw_timeout_stru                    st_obss_scan_timer;                         /* obss扫描定时器，循环定时器 */
    mac_channel_stru                    st_p2p_vap_channel;                     /* p2p listen时记录p2p的信道，用于p2p listen结束后恢复 */

    oal_bool_enum_uint8                 en_2040bss_switch;                      /* 20/40 bss检测开关 */
    oal_uint8                           uc_in_suspend;

#ifdef _PRE_SUPPORT_ACS
    /* DMAC ACS核心 */
    oal_void                           *pst_acs;
    mac_acs_switch_stru                 st_acs_switch;
#endif

    /* linux内核中的device物理信息 */
    oal_wiphy_stru                     *pst_wiphy;                             /* 用于存放和VAP相关的wiphy设备信息，在AP/STA模式下均要使用；可以多个VAP对应一个wiphy */
    mac_bss_id_list_stru                st_bss_id_list;                        /* 当前信道下的扫描结果 */

    oal_uint8                           uc_mac_vap_id;                         /* 多vap共存时，保存睡眠前的mac vap id */
    oal_bool_enum_uint8                 en_dbac_enabled;
    oal_bool_enum_uint8                 en_dbac_running;                       /* DBAC是否在运行 */
    oal_bool_enum_uint8                 en_dbac_has_vip_frame;                 /* 标记DBAC运行时收到了关键帧 */

    oal_uint8                           uc_arpoffload_switch;
    oal_uint8                           uc_wapi;
    oal_uint8                           uc_reserve;
    oal_bool_enum_uint8                 en_is_random_mac_addr_scan;            /* 随机mac扫描开关,从hmac下发 */

    oal_uint8                           auc_mac_oui[WLAN_RANDOM_MAC_OUI_LEN];  /* 随机mac地址OUI,由Android下发 */
    oal_bool_enum_uint8                 en_dbdc_running;                       /* DBDC是否在运行 */

    mac_device_capability_stru          st_device_cap;                         /* device的部分能力，包括定制化 */

#ifdef _PRE_WLAN_FEATURE_SMPS
    /* SMPS是MAC的能力，需要device上所有的VAP都支持SMPS才会开启MAC的SMPS能力 */
    wlan_mib_mimo_power_save_enum_uint8 en_mac_smps_mode;                       /* 记录当前MAC工作的SMPS能力(放在mac侧，是因为hmac也会用于做判断) */
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
    mac_dfs_core_stru                   st_dfs;
#endif
    mac_ap_ch_info_stru                 st_ap_channel_list[MAC_MAX_SUPP_CHANNEL];

#ifdef _PRE_WLAN_FEATURE_HILINK
    mac_fbt_scan_mgmt_stru             st_fbt_scan_mgmt;                        /* 管理未关联用户侦听信息的结构体 */
#endif
    /*针对Device的成员，待移动到dmac_device*/
#if IS_DEVICE
    oal_uint16                          us_total_mpdu_num;                      /* device下所有TID中总共的mpdu_num数量 */
    oal_uint16                          aus_ac_mpdu_num[WLAN_WME_AC_BUTT];      /* device下各个AC对应的mpdu_num数 */
    oal_uint16                          aus_vap_mpdu_num[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];      /* 统计各个vap对应的mpdu_num数 */

    oal_uint16                          us_dfs_timeout;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_tx_dataflow_brk_bypass_struc    st_dataflow_brk_bypass;
#endif
    oal_uint32                          ul_first_timestamp;                         /*记录性能统计第一次时间戳*/

    /* 扫描相关成员变量 */
    oal_uint32                          ul_scan_timestamp;                      /* 记录最新一次扫描开始的时间 */

    mac_scan_state_enum_uint8           en_curr_scan_state;                     /* 当前扫描状态，根据此状态处理obss扫描和host侧下发的扫描请求，以及扫描结果的上报处理 */
    oal_uint8                           uc_resume_qempty_flag;                  /* 使能恢复qempty标识, 默认不使能 */
    oal_uint8                           uc_scan_count;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_int8                            c_ppm_val;
#else
    oal_uint8                           auc_resv2[1];
#endif
// 1151上不可以使用module地址空间的RAM发送帧体
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_fcs_cfg_stru                   *pst_fcs_cfg;                             /* 快速切信道结构体 */
#else
    mac_fcs_cfg_stru                    st_fcs_cfg;
#endif

#ifdef _PRE_WLAN_DFT_STAT
    /* 管理帧统计信息 */
    mac_device_mgmt_statistic_stru      st_mgmt_stat;
    mac_device_dbb_env_param_ctx_stru   st_dbb_env_param_ctx;                  /* 上报空口环境类维测参数的控制结构 */
#endif
    mac_fcs_mgr_stru                    st_fcs_mgr;

    oal_uint8                           uc_csa_cnt;                            /* 每个AP发送一次CSA帧，该计数加1。AP切换完信道后，该计数清零 */
    oal_bool_enum_uint8                 en_txop_enable;                        /* 发送无聚合时采用TXOP模式 */
    oal_uint8                           uc_tx_ba_num;                  /* 发送方向BA会话个数 */
    oal_uint8                           auc_resv[1];


    frw_timeout_stru                    st_keepalive_timer;                     /* keepalive定时器 */

    oal_uint32                          aul_mac_err_cnt[HAL_MAC_ERROR_TYPE_BUTT];   /*mac 错误计数器*/

#ifdef _PRE_WLAN_FEATURE_AP_PM
    oal_void*                           pst_pm_arbiter;                        /*device结构体下节能仲裁管理结构*/
    oal_bool_enum_uint8                 en_pm_enable;                          /*PM整体功能开启关闭开关*/
    oal_uint8                           auc_resv7[3];
#endif

#ifdef _PRE_WLAN_REALTIME_CALI
    frw_timeout_stru                    st_realtime_cali_timer;                    /* 实时校准超时定时器*/
#endif
#endif /* IS_DEVICE */

    /*针对Host的成员，待移动到hmac_device*/
#if IS_HOST
#ifndef _PRE_WLAN_FEATURE_AMPDU_VAP
    oal_uint8                           uc_rx_ba_session_num;                   /* 该device下，rx BA会话的数目 */
    oal_uint8                           uc_tx_ba_session_num;                   /* 该device下，tx BA会话的数目 */
    oal_uint8                           auc_resv11[2];
#endif
    oal_bool_enum_uint8                 en_vap_classify;                        /* 是否使能基于vap的业务分类 */
    oal_uint8                           uc_ap_chan_idx;                        /* 当前扫描信道索引 */
    oal_uint8                           auc_resv4[2];
#endif /* IS_HOST */

    oal_bool_enum_uint8                 en_40MHz_intol_bit_recd;
    oal_uint8                           uc_ftm_vap_id;                           /*ftm中断对应 vap ID */
    oal_uint8                           auc_resv5[2];

#ifdef _PRE_WLAN_FEATURE_STA_PM
    hal_mac_key_statis_info_stru      st_mac_key_statis_info;                   /* mac关键统计信息 */
#endif

#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)
    oal_work_stru                       auto_cali_work;
    oal_uint8                           uc_cali_type;
    oal_bool_enum_uint8                 en_cali_rdy;
    oal_uint8                           auc_resv6[2];
#endif

    frw_timeout_stru                    st_send_frame;                     /* send frame定时器 */

#ifdef _PRE_WLAN_FEATUER_PCIE_TEST
    oal_uint8                               uc_pcie_test_flag;
    oal_uint32                              *pst_buff_start;
#endif
    oal_uint8                               _rom[4];
#ifdef _PRE_WLAN_MAC_ADDR_EDCA_FIX
    mac_custom_edca_stru                    st_custom_edca;
#endif
}mac_device_stru;

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
#define MAC_DEV_GET_FCS_CFG(mac_dev) ((mac_dev)->pst_fcs_cfg)
#else
#define MAC_DEV_GET_FCS_CFG(mac_dev) (&(mac_dev)->st_fcs_cfg)
#endif

#pragma pack(push,1)
/* 上报的扫描结果的扩展信息，放在上报host侧的管理帧netbuf的后面 */
typedef struct
{
    oal_int32                           l_rssi;                     /* 信号强度 */
    wlan_mib_desired_bsstype_enum_uint8 en_bss_type;                /* 扫描到的bss类型 */
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    oal_int8                            c_snr_ant0;                 /* ant0 SNR */
    oal_int8                            c_snr_ant1;                 /* ant1 SNR */
    oal_uint8                           auc_resv[1];                /* 预留字段 */
    oal_uint32                          ul_parent_tsf;              /* 收帧TSF Timer*/
#else
    oal_uint8                           auc_resv[3];                /* 预留字段 */
#endif
#ifdef _PRE_WLAN_WEB_CMD_COMM
    oal_uint32                          ul_max_rate_kbps;           /* 该AP支持的最高速率(单位:kbps) */
    oal_uint8                           uc_max_nss;                 /* 该AP支持的最大空间流数 */
    hal_channel_assemble_enum_uint8     en_bw;
    oal_uint8                           auc_resv1[2];
#endif

    oal_int8                            c_ant0_rssi;                /* 天线0的rssi */
    oal_int8                            c_ant1_rssi;                /* 天线1的rssi */
    oal_uint8                           auc_resv2[2];

#ifdef _PRE_WLAN_FEATURE_M2S
    wlan_nss_enum_uint8                 en_support_max_nss;         /* 该AP支持的最大空间流数 */
    oal_bool_enum_uint8                 en_support_opmode;          /* 该AP是否支持OPMODE */
    oal_uint8                           uc_num_sounding_dim;        /* 该AP发送txbf的天线数 */
#endif
}mac_scanned_result_extend_info_stru;
#pragma pack(pop)


typedef struct
{
    oal_uint8                   auc_tx_ba_index_table[MAC_TX_BA_LUT_BMAP_LEN];          /* 发送端BA LUT表位图 */
    oal_uint8                   auc_rx_ba_lut_idx_table[MAC_RX_BA_LUT_BMAP_LEN];        /* 接收端BA LUT表位图 */
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    oal_uint8                   auc_rx_ba_lut_idx_status_table[MAC_RX_BA_LUT_BMAP_LEN]; /* 接收端BA LUT 是否在 hal BA LUT寄存器里 1: 表示在  0: 表示不在 当前已被置换出去 */
#endif
    oal_uint8                   auc_ra_lut_index_table[WLAN_ACTIVE_USER_IDX_BMAP_LEN];  /* 关联用户 LUT表位图 */
}mac_lut_table_stru;

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
typedef struct
{
    oal_dlist_head_stru         st_active_user_list_head;   /* 活跃用户节点双向链表，使用user结构内的dlist，只有用户扩展特性用 */
    oal_switch_enum_uint8       en_flag;                    /* chip级用户扩展开关 */
    oal_uint8                   auc_resv[3];                /* 预留字段，手动对其 */
}mac_chip_user_extend_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
typedef struct
{
    oal_uint32         ul_addr_h;     /* mac地址高位 */
    oal_uint32         ul_addr_l;     /* mac地址低位 */
    oal_uint32         ul_ba_param;   /* ba 参数*/
    oal_uint32         ul_bitmap_h;   /* bitmap 高位 */
    oal_uint32         ul_bitmap_l;   /* bitmap 低位 */
}mac_chip_ba_lut_stru;
typedef struct
{
    mac_chip_ba_lut_stru     ast_rx_ba_lut_entry[HAL_MAX_RX_BA_LUT_SIZE];   /* 存储BA表的详细信息 保存和恢复BA表时使用 */
    oal_uint8                auc_hal_to_dmac_lut_index_map[WLAN_MAX_RX_BA]; /* hal硬件BA index 和 软件ba index的映射关系表 随着实际BA替换过程改变 */
    oal_uint8                auc_rx_ba_seq_to_lut_index_map[HAL_MAX_RX_BA_LUT_SIZE]; /* 收到的聚合包的顺序和实际ba index的映射关系表 */
    oal_uint8                uc_rx_ba_seq_index;                                  /* 收到waveapp发送的聚合包的顺序统计 */
    oal_uint8                uc_prev_handle_ba_index;                             /* 记录前一次处理的ba index*/
    oal_uint16               us_rx_ba_seq_phase_one_count;
}mac_chip_rx_aggr_extend_stru;
#endif


/* chip结构体 */
typedef struct
{
    oal_uint8                   auc_device_id[WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP];    /* CHIP下挂的DEV，仅记录对应的ID索引值 */
    oal_uint8                   uc_device_nums;                                 /* chip下device的数目 */
    oal_uint8                   uc_chip_id;                                     /* 芯片ID */
    oal_bool_enum_uint8         en_chip_state;                                  /* 标识是否已初始化，OAL_TRUE初始化完成，OAL_FALSE未初始化 */
    oal_uint32                  ul_chip_ver;                                    /* 芯片版本 */
    hal_to_dmac_chip_stru      *pst_hal_chip;                                   /* 硬mac结构指针，HAL提供，用于逻辑和物理chip的对应 */
    mac_lut_table_stru          st_lut_table;                                   /* 软件维护LUT表资源的结构体 */
    oal_void                   *p_alg_priv;                                     /* chip级别算法私有结构体 */

    /* 用户相关成员变量 */
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    mac_chip_user_extend_stru   st_user_extend;                                 /* 用户扩展用 */
#endif
    frw_timeout_stru            st_active_user_timer;                           /* 用户活跃定时器 */
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    mac_chip_rx_aggr_extend_stru *pst_rx_aggr_extend;
    oal_bool_enum_uint8           en_waveapp_32plus_user_enable;
#endif
    oal_uint8                   uc_assoc_user_cnt;                              /* 关联用户数 */
    oal_uint8                   uc_active_user_cnt;                             /* 活跃用户数 */
}mac_chip_stru;

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
typedef enum
{
    MAC_RX_IP_FILTER_STOPED  = 0, //功能关闭，未使能、或者其他状况不允许过滤动作。
    MAC_RX_IP_FILTER_WORKING = 1, //功能打开，按照规则正常过滤
    MAC_RX_IP_FILTER_BUTT
}mac_ip_filter_state_enum;
typedef oal_uint8 mac_ip_filter_state_enum_uint8;

typedef struct
{
    mac_ip_filter_state_enum_uint8 en_state;        //功能状态：过滤、非过滤等
    oal_uint8                  uc_btable_items_num; //黑名单中目前存储的items个数
    oal_uint8                  uc_btable_size;      //黑名单大小，表示最多存储的items个数
    oal_uint8                  uc_resv;
    mac_ip_filter_item_stru   *pst_filter_btable;   //黑名单指针
}mac_rx_ip_filter_struc;
#endif //_PRE_WLAN_FEATURE_IP_FILTER

/* board结构体 */
typedef struct
{
    mac_chip_stru               ast_chip[WLAN_CHIP_MAX_NUM_PER_BOARD];              /* board挂接的芯片 */
    oal_uint8                   uc_chip_id_bitmap;                                  /* 标识chip是否被分配的位图 */
    oal_uint8                   auc_resv[3];                                        /* 字节对齐 */
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    mac_rx_ip_filter_struc      st_rx_ip_filter;                                    /* rx ip过滤功能的管理结构体 */
#endif //_PRE_WLAN_FEATURE_IP_FILTER
}mac_board_stru;

typedef struct
{
    mac_device_stru                    *pst_mac_device;
}mac_wiphy_priv_stru;

/* 黑名单 */
typedef struct
{
    oal_uint8                       auc_mac_addr[OAL_MAC_ADDR_LEN];       /* mac地址          */
    oal_uint8                       auc_reserved[2];                      /* 字节对齐         */
    oal_uint32                      ul_cfg_time;                          /* 加入黑名单的时间 */
    oal_uint32                      ul_aging_time;                        /* 老化时间         */
    oal_uint32                      ul_drop_counter;                      /* 报文丢弃统计     */
} mac_blacklist_stru;

/* 自动黑名单 */
typedef struct
{
    oal_uint8                       auc_mac_addr[OAL_MAC_ADDR_LEN];       /* mac地址  */
    oal_uint8                       auc_reserved[2];                      /* 字节对齐 */
    oal_uint32                      ul_cfg_time;                          /* 初始时间 */
    oal_uint32                      ul_asso_counter;                      /* 关联计数 */
} mac_autoblacklist_stru;

/* 自动黑名单信息 */
typedef struct
{
    oal_uint8                       uc_enabled;                             /* 使能标志 0:未使能  1:使能 */
    oal_uint8                       list_num;                               /* 有多少个自动黑名单        */
    oal_uint8                       auc_reserved[2];                        /* 字节对齐                  */
    oal_uint32                      ul_threshold;                           /* 门限                      */
    oal_uint32                      ul_reset_time;                          /* 重置时间                  */
    oal_uint32                      ul_aging_time;                          /* 老化时间                  */
    mac_autoblacklist_stru          ast_autoblack_list[WLAN_BLACKLIST_MAX]; /* 自动黑名单表              */
} mac_autoblacklist_info_stru;
#if 1
/* 黑白名单信息 */
typedef struct
{
    oal_uint8                          uc_mode;                                  /* 黑白名单模式   */
    oal_uint8                          uc_list_num;                              /* 名单数         */
    oal_uint8                          uc_blacklist_vap_index;                      /* 黑名单vap index */
    oal_uint8                          uc_blacklist_device_index;                   /* 黑名单device index */
    mac_autoblacklist_info_stru        st_autoblacklist_info;                    /* 自动黑名单信息 */
    mac_blacklist_stru                 ast_black_list[WLAN_BLACKLIST_MAX];       /* 有效黑白名单表 */
} mac_blacklist_info_stru;
#endif

/* m2s device信息结构体 */
typedef struct
{
    oal_uint8                           uc_device_id;         /* 业务vap id */
    wlan_nss_enum_uint8                 en_nss_num;           /* device的接收空间流个数 */
    wlan_mib_mimo_power_save_enum_uint8 en_smps_mode;         /* mac device的smps能力，用于切换后vap能力初始化 */
    oal_uint8                           auc_reserved[1];
}mac_device_m2s_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
/* 主逻辑中不想看到宏 */
#ifdef _PRE_WLAN_FEATURE_DBAC
#define MAC_DBAC_ENABLE(_pst_device) (_pst_device->en_dbac_enabled == OAL_TRUE)
#else
#define MAC_DBAC_ENABLE(_pst_device) (OAL_FALSE)
#endif

#define   MAC_CHIP_GET_HAL_CHIP(_pst_chip)  (((mac_chip_stru *)(_pst_chip))->pst_hal_chip)

extern mac_device_capability_stru g_st_mac_device_capability[];
extern mac_device_capability_stru *g_pst_mac_device_capability;

#ifdef _PRE_WLAN_FEATURE_WMMAC
extern oal_bool_enum_uint8 g_en_wmmac_switch_etc;
#endif

extern mac_board_stru g_st_mac_board;
extern mac_board_stru *g_pst_mac_board;



OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8 mac_is_dbac_enabled(mac_device_stru *pst_device)
{
    return  pst_device->en_dbac_enabled;
}


OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8 mac_is_dbac_running(mac_device_stru *pst_device)
{
    if (OAL_FALSE == pst_device->en_dbac_enabled)
    {
        return OAL_FALSE;
    }

    return  pst_device->en_dbac_running;
}

OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8 mac_is_dbdc_running(mac_device_stru *pst_mac_device)
{
    return  pst_mac_device->en_dbdc_running;
}

#ifdef _PRE_SUPPORT_ACS

OAL_STATIC  OAL_INLINE  en_mac_acs_sw_enum_uint8 mac_get_acs_switch(mac_device_stru *pst_mac_device)
{
    if (pst_mac_device->pst_acs == OAL_PTR_NULL)
    {
        return MAC_ACS_SW_NONE;
    }

    return pst_mac_device->st_acs_switch.en_acs_switch;
}

OAL_STATIC  OAL_INLINE  oal_void mac_set_acs_switch(mac_device_stru *pst_mac_device, en_mac_acs_sw_enum_uint8 en_switch)
{
    if (pst_mac_device->pst_acs == OAL_PTR_NULL)
    {
        return;
    }

    pst_mac_device->st_acs_switch.en_acs_switch = en_switch;
}
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8 mac_get_2040bss_switch(mac_device_stru *pst_mac_device)
{
    return pst_mac_device->en_2040bss_switch;
}
OAL_STATIC  OAL_INLINE  oal_void mac_set_2040bss_switch(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_switch)
{
    pst_mac_device->en_2040bss_switch = en_switch;
}
#endif
#if IS_DEVICE
OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8 mac_device_is_scaning(mac_device_stru *pst_mac_device)
{
    return (pst_mac_device->en_curr_scan_state == MAC_SCAN_STATE_RUNNING)?OAL_TRUE:OAL_FALSE;
}

OAL_STATIC  OAL_INLINE  oal_bool_enum_uint8 mac_device_is_listening(mac_device_stru *pst_mac_device)
{
    return ((pst_mac_device->en_curr_scan_state == MAC_SCAN_STATE_RUNNING)
            && (pst_mac_device->st_scan_params.uc_scan_func & MAC_SCAN_FUNC_P2P_LISTEN))?OAL_TRUE:OAL_FALSE;
}
#endif /* IS_DEVICE */
OAL_STATIC  OAL_INLINE oal_bool_enum_uint8 mac_chip_run_band(oal_uint8 uc_chip_id, wlan_channel_band_enum_uint8 en_band)
{
    /*
     * 判断指定芯片是否可以运行在指定BAND：
     *     -双芯片时各芯片只运行在指定的BAND。若后续有双芯片四频，修改此处
     *     -单芯片双频时可以运行在两个BAND
     *     -单芯片单频时只可以运行在宏定义指定的BAND
     *     -note:目前所有witp wifi芯片均支持双频，若后续有单频芯片，需要增加诸如
     *      plat_chip_supp_band(chip_id, band)的接口，并在此处额外判断
     */

    if ((en_band != WLAN_BAND_2G) && (en_band != WLAN_BAND_5G))
    {
        return OAL_FALSE;
    }

#if defined(_PRE_WLAN_FEATURE_DOUBLE_CHIP)    // 目前双芯片均为双芯片各自单频
    return ((en_band == WLAN_BAND_2G) && (uc_chip_id == g_uc_wlan_double_chip_2g_id))
        || ((en_band == WLAN_BAND_5G) && (uc_chip_id == g_uc_wlan_double_chip_5g_id)) ? OAL_TRUE : OAL_FALSE;
#elif defined(_PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND)
    return OAL_TRUE;
#elif defined(_PRE_WLAN_FEATURE_SINGLE_CHIP_SINGLE_BAND)
    return en_band == WLAN_SINGLE_CHIP_SINGLE_BAND_WORK_BAND ? OAL_TRUE : OAL_FALSE;
#else
    return OAL_TRUE;
#endif
}

/*****************************************************************************
  10 函数声明
*****************************************************************************/
/*****************************************************************************
  10.1 公共结构体初始化、删除
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_DFS
extern oal_void  mac_dfs_init(mac_device_stru *pst_mac_device);
#endif /*#ifdef _PRE_WLAN_FEATURE_DFS  */
extern oal_uint32  mac_device_init_etc(mac_device_stru *pst_mac_device, oal_uint32 ul_chip_ver, oal_uint8 chip_id, oal_uint8 uc_device_id);
extern oal_uint32  mac_chip_init_etc(mac_chip_stru *pst_chip, oal_uint8 uc_device_max);
extern oal_uint32  mac_board_init_etc(void);

extern oal_uint32  mac_device_exit_etc(mac_device_stru *pst_device);
extern oal_uint32  mac_chip_exit_etc(mac_board_stru *pst_board, mac_chip_stru *pst_chip);
extern oal_uint32  mac_board_exit_etc(mac_board_stru *pst_board);


/*****************************************************************************
  10.2 公共成员访问部分
*****************************************************************************/
extern oal_void mac_chip_inc_assoc_user(mac_chip_stru *pst_mac_chip);
extern oal_void mac_chip_dec_assoc_user(mac_chip_stru *pst_mac_chip);
extern oal_void mac_chip_inc_active_user(mac_chip_stru *pst_mac_chip);
extern oal_void mac_chip_dec_active_user(mac_chip_stru *pst_mac_chip);

#ifdef _PRE_WLAN_FEATURE_11AX
extern oal_uint8  mac_device_trans_bandwith_to_he_capinfo(wlan_bw_cap_enum_uint8 en_max_op_bd);
#endif

extern oal_void mac_device_set_dfr_reset_etc(mac_device_stru *pst_mac_device, oal_uint8 uc_device_reset_in_progress);
extern oal_void mac_device_set_state_etc(mac_device_stru *pst_mac_device, oal_uint8 en_device_state);

extern oal_void mac_device_set_beacon_interval_etc(mac_device_stru *pst_mac_device, oal_uint32 ul_beacon_interval);

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
extern oal_void mac_blacklist_get_pointer(wlan_vap_mode_enum_uint8 en_vap_mode, oal_uint8 uc_dev_id, oal_uint8 uc_chip_id, oal_uint8 uc_vap_id, mac_blacklist_info_stru **pst_blacklist_info);
#endif

#if 0
extern oal_void mac_device_inc_assoc_user(mac_device_stru *pst_mac_device);
extern oal_void mac_device_dec_assoc_user(mac_device_stru *pst_mac_device);
extern oal_void mac_device_set_dfs(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_dfs_switch, oal_uint8 uc_debug_level);
#endif
extern oal_void * mac_device_get_all_rates_etc(mac_device_stru *pst_dev);
#ifdef _PRE_WLAN_FEATURE_HILINK
extern oal_uint32  mac_device_clear_fbt_scan_list(mac_device_stru *pst_mac_dev, oal_uint8 *puc_param);
extern oal_uint32  mac_device_set_fbt_scan_sta(mac_device_stru *pst_mac_dev, mac_fbt_scan_sta_addr_stru *pst_fbt_scan_sta);
extern oal_uint32  mac_device_set_fbt_scan_interval(mac_device_stru *pst_mac_dev, oal_uint32 ul_scan_interval);
extern oal_uint32  mac_device_set_fbt_scan_channel(mac_device_stru *pst_mac_dev, oal_uint8 uc_fbt_scan_channel);
extern oal_uint32  mac_device_set_fbt_scan_report_period(mac_device_stru *pst_mac_dev, oal_uint32 ul_fbt_scan_report_period);
extern oal_uint32  mac_device_set_fbt_scan_enable(mac_device_stru *pst_mac_device, oal_uint8 uc_cfg_fbt_scan_enable);
#endif
extern oal_uint32  mac_device_check_5g_enable(oal_uint8 uc_device_id);

#ifdef _PRE_WLAN_FEATURE_DFS
extern oal_void  mac_dfs_set_cac_enable(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_val);
extern oal_void  mac_dfs_set_offchan_cac_enable(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_val);
extern oal_bool_enum_uint8  mac_dfs_get_offchan_cac_enable(mac_device_stru *pst_mac_device);
extern oal_void  mac_dfs_set_offchan_number(mac_device_stru *pst_mac_device, oal_uint32 ul_val);
extern oal_bool_enum_uint8  mac_dfs_get_cac_enable(mac_device_stru *pst_mac_device);
extern oal_void  mac_dfs_set_dfs_enable(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_val);
extern oal_bool_enum_uint8  mac_dfs_get_dfs_enable(mac_device_stru *pst_mac_device);
extern oal_void  mac_dfs_set_debug_level(mac_device_stru *pst_mac_device, oal_uint8 uc_debug_lev);
extern oal_uint8  mac_dfs_get_debug_level(mac_device_stru *pst_mac_device);
extern oal_void  mac_dfs_set_cac_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms, oal_bool_enum_uint8 en_waether);
extern oal_void  mac_dfs_set_off_cac_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms, oal_bool_enum_uint8 en_waether);
extern oal_void  mac_dfs_set_opern_chan_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms);
extern oal_void  mac_dfs_set_off_chan_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms);
extern oal_void  mac_dfs_set_next_radar_ch(mac_device_stru *pst_mac_device, oal_uint8 uc_ch, wlan_channel_bandwidth_enum_uint8 en_width);
extern oal_void  mac_dfs_set_ch_bitmap(mac_device_stru *pst_mac_device, oal_uint32 ul_ch_bitmap);
extern oal_void  mac_dfs_set_non_occupancy_period_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time);
#endif

extern mac_device_voe_custom_stru   g_st_mac_voe_custom_param;

/*****************************************************************************
  10.3 杂项，待归类
*****************************************************************************/
#if 0
#ifdef _PRE_WLAN_FEATURE_DBAC
extern oal_uint32  mac_dbac_update_chl_config(mac_device_stru *pst_mac_device, mac_channel_stru *pst_chl);
#endif
#endif

/*****************************************************************************
  10.4 待移除
*****************************************************************************/

/*****************************************************************************
  11 inline函数定义
*****************************************************************************/

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 mac_device_check_5g_enable_per_chip(oal_void)
{
    oal_uint8            uc_dev_idx    = 0;

    while(uc_dev_idx < WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP)
    {
        if (mac_device_check_5g_enable(uc_dev_idx))
        {
            return OAL_TRUE;
        }
        uc_dev_idx++;
    }

    return OAL_FALSE;
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8  mac_is_hide_ssid(oal_uint8 *puc_ssid_ie, oal_uint8 uc_ssid_len)
{
    return (oal_bool_enum_uint8)((OAL_PTR_NULL == puc_ssid_ie) || (0 == uc_ssid_len) || ('\0' == puc_ssid_ie[0]));
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8  mac_device_is_auto_chan_sel_enabled(mac_device_stru *pst_mac_device)
{
    /* BSS启动时，如果用户没有设置信道，则默认为开启自动信道选择 */
    if (0 == pst_mac_device->uc_max_channel)
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}

extern wlan_mib_vht_supp_width_enum_uint8  mac_device_trans_bandwith_to_vht_capinfo(
                wlan_bw_cap_enum_uint8 en_max_op_bd);


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 mac_fcs_is_same_channel(mac_channel_stru *pst_channel_dst,
                                                                  mac_channel_stru *pst_channel_src)
{
    return  pst_channel_dst->uc_chan_number == pst_channel_src->uc_chan_number ? OAL_TRUE : OAL_FALSE;
}

#if (_PRE_TEST_MODE_BOARD_ST == _PRE_TEST_MODE)
extern  oal_void mac_fcs_verify_init(oal_void);
extern  oal_void mac_fcs_verify_start(oal_void);
extern  oal_void mac_fcs_verify_timestamp(mac_fcs_stage_enum_uint8 en_stage);
extern  oal_void mac_fcs_verify_stop(oal_void);

#else
#define          mac_fcs_verify_init()
#define          mac_fcs_verify_start()
#define          mac_fcs_verify_timestamp(a)
#define          mac_fcs_verify_stop()
#endif  // _PRE_DEBUG_MODE
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of mac_device.h */
