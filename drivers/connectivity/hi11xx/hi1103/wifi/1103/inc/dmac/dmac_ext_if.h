

#ifndef __DMAC_EXT_IF_H__
#define __DMAC_EXT_IF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "oal_mm.h"
#include "oal_net.h"
#include "frw_ext_if.h"
#include "wal_ext_if.h"
#include "wlan_types.h"
#include "mac_frame.h"
#include "mac_device.h"
#include "mac_user.h"
#include "mac_vap.h"
#include "mac_data.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_EXT_IF_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#if defined (_PRE_PRODUCT_ID_HI110X_HOST) || defined (_PRE_PRODUCT_ID_HI110X_DEV)
#define DMAC_UCAST_FRAME_TX_COMP_TIMES      10          /* 建立BA会话前，需要产生单播帧的发送完成中断 */
#else
#define DMAC_UCAST_FRAME_TX_COMP_TIMES      5           /* 建立BA会话前，需要产生单播帧的发送完成中断 */
#endif /* _PRE_PRODUCT_ID_HI110X_DEV */

/* DMAC CB中用于不同算法对帧进行标记 */
#define DMAC_CB_ALG_TAGS_MUCTRL_MASK        0x1         /* CB中用于多用户流控算法对帧进行标记 */
#define DMAC_CB_ALG_TAGS_TIDSCH_MASK        0x2         /* CB中用于调度算法对队列调度的帧标记 */

#define DMAC_BA_SEQNO_MASK                  0x0FFF      /* max sequece number */
#define DMAC_BA_MAX_SEQNO_BY_TWO            2048
#define DMAC_BA_RX_ALLOW_MIN_SEQNO_BY_TWO   64
#define DMAC_BA_RX_ALLOW_MAX_SEQNO_BY_TWO   4032

#define DMAC_BA_GREATER_THAN_SEQHI          1
#define DMAC_BA_BETWEEN_SEQLO_SEQHI         2
#define DMAC_BA_AMSDU_BACK_SUPPORTED_FLAG   1           /* BA会话对AMSDU的支持标识 */

#define DMAC_BA_DELBA_TIMEOUT               0
#define DMAC_BATX_WIN_STALL_THRESHOLD       6

#define MAC_TX_CTL_SIZE                     OAL_NETBUF_CB_SIZE()

/* DMAC TID中TCP ACK比例 */
#define DMAC_TID_TCK_ACK_PROPORTION_MAX     32
#define DMAC_TID_TCK_ACK_PROPORTION_MIN     1
#define DMAC_TID_TCK_ACK_PROPORTION_THRES   28

#define DMAC_BA_SEQ_ADD(_seq1, _seq2)       (((_seq1) + (_seq2)) & DMAC_BA_SEQNO_MASK)
#define DMAC_BA_SEQ_SUB(_seq1, _seq2)       (((_seq1) - (_seq2)) & DMAC_BA_SEQNO_MASK)
#define DMAC_BA_SEQNO_ADD(_seq1, _seq2)     (((_seq1) + (_seq2)) & DMAC_BA_SEQNO_MASK)
#define DMAC_BA_SEQNO_SUB(_seq1, _seq2)     (((_seq1) - (_seq2)) & DMAC_BA_SEQNO_MASK)

#define DMAC_BA_BMP_SIZE                    64

#define DMAC_IS_BA_INFO_PRESENT(_flags)     (_flags & BIT0)

#define DMAC_INVALID_SIGNAL_DELTA      (30)
#define DMAC_RSSI_SIGNAL_MIN           (-103)    /*信号强度极小值 */
#define DMAC_RSSI_SIGNAL_MAX           (5)       /*信号强度极大值 */
#define DMAC_INVALID_SIGNAL_INITIAL    (100)     /*非法初始信号极大值*/

#define MAC_INVALID_RX_BA_LUT_INDEX           HAL_MAX_RX_BA_LUT_SIZE

/* 发送BA窗口记录seq number的最大个数，必须是2的整数次幂 */
#define DMAC_TID_MAX_BUFS       128
/* 发送BA窗口记录seq number的bitmap所使用的类型长度 */
#define DMAC_TX_BUF_BITMAP_WORD_SIZE        32
/* 发送BA窗口记录seq number的bit map的长度 */
#define DMAC_TX_BUF_BITMAP_WORDS \
    ((DMAC_TID_MAX_BUFS+DMAC_TX_BUF_BITMAP_WORD_SIZE-1) / DMAC_TX_BUF_BITMAP_WORD_SIZE)

/* 安全加密 :  bss_info 中记录AP 能力标识， WPA or WPA2*/
#define DMAC_WPA_802_11I    BIT0
#define DMAC_RSNA_802_11I   BIT1

#define DMAC_TX_MAX_RISF_NUM    6
#define DMAC_TX_QUEUE_AGGR_DEPTH     2
#define DMAX_TX_QUEUE_SINGLE_DEPTH   2
#define DMAC_TX_QEUEU_MAX_PPDU_NUM   2
#define DMAC_TX_QUEUE_FAIL_CHECK_NUM    1000
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_DFT_STAT
#define DMAC_TID_STATS_INCR(_member, _cnt)      ((_member) += (_cnt))
#else
#define DMAC_TID_STATS_INCR(_member, _cnt)
#endif
#else
#define DMAC_TID_STATS_INCR(_member, _cnt)
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#define CUSTOM_MSG_DATA_HDR_LEN      (OAL_SIZEOF(custom_cfgid_enum_uint32) + OAL_SIZEOF(oal_uint32))   /*抛往dmac侧消息头的长度*/
#endif //#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

#if (defined(_PRE_DEBUG_MODE) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE))
extern oal_uint32 g_ul_desc_addr[HAL_TX_QUEUE_BUTT];
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern oal_uint8 g_auc_ip_filter_btable[MAC_MAX_IP_FILTER_BTABLE_SIZE];  /* rx ip过滤功能的黑名单 */
#endif //_PRE_WLAN_FEATURE_IP_FILTER

#define DMAC_SCAN_MAX_AP_NUM_TO_GNSS            32
#define GNSS_DMAC_SCAN_RESULTS_VALID_MS         5000

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
#define DMAC_WLAN_EVENT_TAS_ANT1_RSSI_INVALID_TYPE   OAL_RSSI_INIT_VALUE      /* 上报TAS天线测量C0不用事件 */
#define DMAC_WLAN_EVENT_TAS_RSSI_MEASURING_TYPE      (-129)                    /* 上报TAS天线测量未完成事件 */
#define DMAC_WLAN_EVENT_TAS_ANT1_AVAILABLE_TYPE      (-130)                    /* 上报TAS天线测量C0恢复可用事件 */
#endif

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*****************************************************************************
  枚举名  : dmac_tx_host_drx_subtype_enum_uint8
  协议表格:
  枚举说明: HOST DRX事件子类型定义
*****************************************************************************/
/* WLAN_CRX子类型定义 */
typedef enum
{
    DMAC_TX_HOST_DRX = 0,
    HMAC_TX_HOST_DRX = 1,

    DMAC_TX_HOST_DRX_BUTT
}dmac_tx_host_drx_subtype_enum;
typedef oal_uint8 dmac_tx_host_drx_subtype_enum_uint8;

/*****************************************************************************
  枚举名  : dmac_tx_wlan_dtx_subtype_enum_uint8
  协议表格:
  枚举说明: WLAN DTX事件子类型定义
*****************************************************************************/
typedef enum
{
    DMAC_TX_WLAN_DTX = 0,

    DMAC_TX_WLAN_DTX_BUTT
}dmac_tx_wlan_dtx_subtype_enum;
typedef oal_uint8 dmac_tx_wlan_dtx_subtype_enum_uint8;

/*****************************************************************************
  枚举名  : dmac_wlan_ctx_event_sub_type_enum_uint8
  协议表格:
  枚举说明: WLAN CTX事件子类型定义
*****************************************************************************/
typedef enum
{
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_MGMT    = 0,  /* 管理帧处理 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ADD_USER,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_NOTIFY_ALG_ADD_USER,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_DEL_USER,

    DMAC_WLAN_CTX_EVENT_SUB_TYPE_BA_SYNC,      /* 收到wlan的Delba和addba rsp用于到dmac的同步 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_PRIV_REQ,     /* 11N自定义的请求的事件类型 */

    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCAN_REQ,               /* 扫描请求 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SCHED_SCAN_REQ,         /* PNO调度扫描请求 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_RESET_PSM,              /* 收到认证请求 关联请求，复位用户的节能状态 */

    DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_SET_REG,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CONN_RESULT,            /* 关联结果 */

    DMAC_WLAN_CTX_EVENT_SUB_TYPE_ASOC_WRITE_REG,         /* AP侧处理关联时，修改SEQNUM_DUPDET_CTRL寄存器*/

    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_EDCA_REG,       /* STA收到beacon和assoc rsp时，更新EDCA寄存器 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_DEFAULT_VO_REG, /* 如果AP不是WMM的，则STA会去使能EDCA寄存器，并设置VO寄存器 */

    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SWITCH_TO_NEW_CHAN,     /* 切换至新信道事件 */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_SELECT_CHAN,            /* 设置信道事件 */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_DISABLE_TX,             /* 禁止硬件发送 */
    DMAC_WALN_CTX_EVENT_SUB_TYPR_ENABLE_TX,              /* 恢复硬件发送 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_RESTART_NETWORK,        /* 切换信道后，恢复BSS的运行 */
#ifdef _PRE_WLAN_FEATURE_DFS
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_OFF_CHAN,     /* 切换到offchan做off-chan cac检测 */
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_HOME_CHAN,    /* 切回home chan */
#endif
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_DFS_TEST,
    DMAC_WALN_CTX_EVENT_SUB_TYPR_DFS_CAC_CTRL_TX,        /* DFS 1min CAC把vap状态位置为pause或者up,同时禁止或者开启硬件发送 */
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    DMAC_WLAN_CTX_EVENT_SUB_TYPR_EDCA_OPT,               /* edca优化中业务识别通知事件 */
#endif
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_HMAC2DMAC,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_DSCR_OPT,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_CALI_MATRIX_HMAC2DMAC,
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_APP_IE_H2D,
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_IP_FILTER,
#endif //_PRE_WLAN_FEATURE_IP_FILTER
#ifdef _PRE_WLAN_FEATURE_APF
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_APF_CMD,
#endif

#ifdef _PRE_WLAN_FEATURE_11AX
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_MU_EDCA_REG,       /* STA收到beacon和assoc rsp时，更新MU EDCA寄存器 */
#endif
    DMAC_WLAN_CTX_EVENT_SUB_TYPE_BUTT
}dmac_wlan_ctx_event_sub_type_enum;
typedef oal_uint8 dmac_wlan_ctx_event_sub_type_enum_uint8;

/* DMAC模块 WLAN_DRX子类型定义 */
typedef enum
{
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_RX_DATA,     /* AP模式: DMAC WLAN DRX 流程 */
    DMAC_WLAN_DRX_EVENT_SUB_TYPE_TKIP_MIC_FAILE,/* DMAC tkip mic faile 上报给HMAC */

    DMAC_WLAN_DRX_EVENT_SUB_TYPE_BUTT
}dmac_wlan_drx_event_sub_type_enum;
typedef oal_uint8 dmac_wlan_drx_event_sub_type_enum_uint8;

/* DMAC模块 WLAN_CRX子类型定义 */
typedef enum
{
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_INIT,      /* DMAC 给 HMAC的初始化参数 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX,        /* DMAC WLAN CRX 流程 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DELBA,     /* DMAC自身产生的DELBA帧 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_EVERY_SCAN_RESULT,  /* 扫描到一个bss信息，上报结果 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_SCAN_COMP,          /* DMAC扫描完成上报给HMAC */
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_OBSS_SCAN_COMP,     /* DMAC OBSS扫描完成上报给HMAC */
#endif
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_CHAN_RESULT,        /* 上报一个信道的扫描结果 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_ACS_RESP,           /* DMAC ACS 回复应用层命令执行结果给HMAC */

#ifdef _PRE_WLAN_FEATURE_FLOWCTL
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_FLOWCTL_BACKP,     /* dmac向hmac发送流控制反压信息 */
#endif
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DISASSOC,  /* DMAC上报去关联事件到HMAC, HMAC会删除用户 */
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_DEAUTH,    /* DMAC上报去认证事件到HMAC */

    DMAC_WLAN_CRX_EVENT_SUB_TYPR_CH_SWITCH_COMPLETE,   /* 信道切换完成事件 */
#ifdef _PRE_WLAN_FEATURE_DBAC
    DMAC_WLAN_CRX_EVENT_SUB_TYPR_DBAC,                 /* DBAC enable/disable事件 */
#endif
    DMAC_WLAN_CRX_EVENT_SUB_TYPE_BUTT
}dmac_wlan_crx_event_sub_type_enum;
typedef oal_uint8 dmac_wlan_crx_event_sub_type_enum_uint8;

/* TBTT事件子类型定义 */
typedef enum
{
    DMAC_TBTT_EVENT_SUB_TYPE,

    DMAC_TBTT_EVENT_SUB_TYPE_BUTT
}dmac_tbtt_event_sub_type_enum;
typedef oal_uint8 dmac_tbtt_event_sub_type_enum_uint8;

/* 发向HOST侧的配置事件 */
typedef enum
{
    DMAC_TO_HMAC_SYN_UP_REG_VAL = 1,//FRW_SDT_REG_EVENT_LOG_SYN_SUB_TYPE = 0
    DMAC_TO_HMAC_CREATE_BA      = 2,
    DMAC_TO_HMAC_DEL_BA         = 3,
    DMAC_TO_HMAC_SYN_CFG        = 4,
#ifdef _PRE_WLAN_CHIP_TEST_ALG
    DMAC_TO_HMAC_ALG_TEST       = 5,
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    DMAC_TO_HMAC_ALG_INFO_SYN   = 6,
    DMAC_TO_HMAC_VOICE_AGGR     = 7,
#endif
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
    DMAC_TO_HMAC_SYN_UP_SAMPLE_DATA = 8,
#endif

#ifdef _PRE_WLAN_RF_AUTOCALI
    DMAC_TO_HMAC_AUTOCALI_DATA = 9,
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
        DMAC_TO_HMAC_M2S_DATA = 10,
#endif

    DMAC_TO_HMAC_BANDWIDTH_INFO_SYN  = 11,  /* dmac向hmac同步带宽的信息 */
    DMAC_TO_HMAC_PROTECTION_INFO_SYN = 12, /* dmac向hmac同步保护mib信息 */
    DMAC_TO_HMAC_CH_STATUS_INFO_SYN  = 13,  /* dmac向hmac同步可用信道列表 */

    /* 事件注册时候需要枚举值列出来，防止出现device侧和host特性宏打开不一致，
       造成出现同步事件未注册问题，后续各子特性人注意宏打开的一致性
    */
    DMAC_TO_HMAC_SYN_BUTT
}dmac_to_hmac_syn_type_enum;

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
/* hmac to dmac定制化配置同步消息结构 */
typedef enum
{
    CUSTOM_CFGID_NV_ID                 = 0,
    CUSTOM_CFGID_INI_ID,
    CUSTOM_CFGID_DTS_ID,
    CUSTOM_CFGID_PRIV_INI_ID,

    CUSTOM_CFGID_BUTT,
}custom_cfgid_enum;
typedef unsigned int custom_cfgid_enum_uint32;

typedef enum
{
    CUSTOM_CFGID_INI_ENDING_ID         = 0,
    CUSTOM_CFGID_INI_FREQ_ID,
    CUSTOM_CFGID_INI_PERF_ID,
    CUSTOM_CFGID_INI_LINKLOSS_ID,
    CUSTOM_CFGID_INI_PM_SWITCH_ID,
    CUSTOM_CFGID_INI_PS_FAST_CHECK_CNT_ID,

    /* 私有定制 */
    CUSTOM_CFGID_PRIV_INI_RADIO_CAP_ID,
    CUSTOM_CFGID_PRIV_FASTSCAN_SWITCH_ID,
    CUSTOM_CFGID_PRIV_ANT_SWITCH_ID,
    CUSTOM_CFGID_PRIV_INI_BW_MAX_WITH_ID,
    CUSTOM_CFGID_PRIV_INI_LDPC_CODING_ID,
    CUSTOM_CFGID_PRIV_INI_RX_STBC_ID,
    CUSTOM_CFGID_PRIV_INI_TX_STBC_ID,
    CUSTOM_CFGID_PRIV_INI_SU_BFER_ID,
    CUSTOM_CFGID_PRIV_INI_SU_BFEE_ID,
    CUSTOM_CFGID_PRIV_INI_MU_BFER_ID,
    CUSTOM_CFGID_PRIV_INI_MU_BFEE_ID,
    CUSTOM_CFGID_PRIV_INI_11N_TXBF_ID,
    CUSTOM_CFGID_PRIV_INI_1024_QAM_ID,
    CUSTOM_CFGID_PRIV_INI_CALI_MASK_ID,
    CUSTOM_CFGID_PRIV_CALI_DATA_MASK_ID,
    CUSTOM_CFGID_PRIV_INI_AUTOCALI_MASK_ID,
    CUSTOM_CFGID_PRIV_INI_DOWNLOAD_RATELIMIT_PPS,
    CUSTOM_CFGID_PRIV_INI_TXOPPS_SWITCH_ID,
    CUSTOM_CFGID_PRIV_INI_OVER_TEMPER_PROTECT_THRESHOLD_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_ENABLE_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_REDUCE_PWR_ENABLE_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_SAFE_TH_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_OVER_TH_ID,
    CUSTOM_CFGID_PRIV_INI_TEMP_PRO_PA_OFF_TH_ID,
    CUSTOM_CFGID_PRIV_INI_DSSS2OFDM_DBB_PWR_BO_VAL_ID,
    CUSTOM_CFGID_PRIV_INI_EVM_PLL_REG_FIX_ID,
    CUSTOM_CFGID_PRIV_INI_VOE_SWITCH_ID,
    CUSTOM_CFGID_PRIV_M2S_FUNCTION_MASK_ID,
    CUSTOM_CFGID_PRIV_INI_11AX_SWITCH_ID,
    CUSTOM_CFGID_PRIV_DYN_BYPASS_EXTLNA_ID,
    CUSTOM_CFGID_PRIV_CTRL_FRAME_TX_CHAIN_ID,
    CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C0_ID,
    CUSTOM_CFGID_PRIV_CTRL_UPC_FOR_18DBM_C1_ID,
    CUSTOM_CFGID_PRIV_INI_LDAC_M2S_TH_ID,

    CUSTOM_CFGID_INI_BUTT,
}custom_cfgid_h2d_ini_enum;
typedef unsigned int custom_cfgid_h2d_ini_enum_uint32;

typedef struct
{
    custom_cfgid_enum_uint32            en_syn_id;      /* 同步配置ID*/
    oal_uint32                          ul_len;         /* DATA payload长度 */
    oal_uint8                           auc_msg_body[4];/* DATA payload */
}hmac_to_dmac_cfg_custom_data_stru;
#endif //#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

/* MISC杂散事件 */
typedef enum
{
    DMAC_MISC_SUB_TYPE_RADAR_DETECT,
    DMAC_MISC_SUB_TYPE_DISASOC,
    DMAC_MISC_SUB_TYPE_CALI_TO_HMAC,
    DMAC_MISC_SUB_TYPE_HMAC_TO_CALI,


#ifdef _PRE_WLAN_FEATURE_ROAM
    DMAC_MISC_SUB_TYPE_ROAM_TRIGGER,
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_SUPPORT_ACS
    DMAC_MISC_SUB_TYPE_RESCAN,
#endif

#ifdef _PRE_WLAN_ONLINE_DPD
    DMAC_TO_HMAC_DPD,
#endif

#ifdef _PRE_WLAN_FEATURE_APF
    DMAC_MISC_SUB_TYPE_APF_REPORT,
#endif

    DMAC_MISC_SUB_TYPE_BUTT
}dmac_misc_sub_type_enum;

typedef enum{
    DMAC_DISASOC_MISC_LINKLOSS             = 0,
    DMAC_DISASOC_MISC_KEEPALIVE            = 1,
    DMAC_DISASOC_MISC_CHANNEL_MISMATCH     = 2,
    DMAC_DISASOC_MISC_LOW_RSSI             = 3,
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    DMAC_DISASOC_MISC_BSD                  = 4,
#endif
    DMAC_DISASOC_MISC_GET_CHANNEL_IDX_FAIL = 5,
#ifdef _PRE_FEATURE_FAST_AGING
    DMAC_DISASOC_MISC_FAST_AGINIG          = 6,
#endif

    DMAC_DISASOC_MISC_BUTT
}dmac_disasoc_misc_reason_enum;
typedef oal_uint16 dmac_disasoc_misc_reason_enum_uint16;


/* HMAC to DMAC同步类型 */
typedef enum
{
    HMAC_TO_DMAC_SYN_INIT,
    HMAC_TO_DMAC_SYN_CREATE_CFG_VAP,
    HMAC_TO_DMAC_SYN_CFG,
    HMAC_TO_DMAC_SYN_ALG,
    HMAC_TO_DMAC_SYN_REG,
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
    HMAC_TO_DMAC_SYN_SAMPLE,
#endif
#ifdef _PRE_WLAN_RF_AUTOCALI
    HMAC_TO_DMAC_AUTOCALI_CMD = 7,
#endif

    HMAC_TO_DMAC_SYN_BUTT
}hmac_to_dmac_syn_type_enum;
typedef oal_uint8 hmac_to_dmac_syn_type_enum_uint8;

/* TXRX函数回调出参定义 */
typedef enum
{
    DMAC_TXRX_PASS = 0,     /* 继续往下 */
    DMAC_TXRX_DROP = 1,     /* 需要丢包 */
    DMAC_TXRX_SENT = 2,     /* 已被发送 */
    DMAC_TXRX_BUFF = 3,     /* 已被缓存 */

    DMAC_TXRX_BUTT
}dmac_txrx_output_type_enum;
typedef oal_uint8 dmac_txrx_output_type_enum_uint8;


/* 天线训练状态 */
typedef enum
{
    DMAC_USER_SMARTANT_NON_TRAINING        = 0,
    DMAC_USER_SMARTANT_NULLDATA_TRAINING   = 1,
    DMAC_USER_SMARTANT_DATA_TRAINING       = 2,

    DMAC_USER_SMARTANT_TRAINING_BUTT
}dmac_user_smartant_training_enum;
typedef oal_uint8 dmac_user_smartant_training_enum_uint8;

/* 算法的报文探测标志 (注:对于1102该枚举只允许使用3bit空间, 因此有效枚举值最大为7) */
typedef enum
{
    DMAC_USER_ALG_NON_PROBE                     = 0,    /* 非探测报文 */
    DMAC_USER_ALG_TXBF_SOUNDING                 = 1,    /* TxBf sounding报文 */
    DMAC_USER_ALG_AUOTRATE_PROBE                = 2,    /* Autorate探测报文 */
    DMAC_USER_ALG_AGGR_PROBE                    = 3,    /* 聚合探测报文 */
    DMAC_USER_ALG_TPC_PROBE                     = 4,    /* TPC探测报文 */
    DMAC_USER_ALG_TX_MODE_PROBE                 = 5,    /* 发送模式探测报文(TxBf, STBC, Chain) */
    DMAC_USER_ALG_SMARTANT_NULLDATA_PROBE       = 6,    /* 智能天线NullData训练报文 */
    DMAC_USER_ALG_SMARTANT_DATA_PROBE           = 7,    /* 智能天线Data训练报文 */

    DMAC_USER_ALG_PROBE_BUTT
}dmac_user_alg_probe_enum;
typedef oal_uint8 dmac_user_alg_probe_enum_uint8;

/* BA会话的状态枚举 */
typedef enum
{
    DMAC_BA_INIT        = 0,    /* BA会话未建立 */
    DMAC_BA_INPROGRESS,         /* BA会话建立过程中 */
    DMAC_BA_COMPLETE,           /* BA会话建立完成 */
    DMAC_BA_HALTED,             /* BA会话节能暂停 */
    DMAC_BA_FAILED,             /* BA会话建立失败 */

    DMAC_BA_BUTT
}dmac_ba_conn_status_enum;
typedef oal_uint8 dmac_ba_conn_status_enum_uint8;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* Type of Tx Descriptor status */
typedef enum
{
      DMAC_TX_INVALID   = 0,                /*无效*/
      DMAC_TX_SUCC,                         /*成功*/
      DMAC_TX_FAIL,                         /*发送失败（超过重传限制：接收响应帧超时）*/
      DMAC_TX_TIMEOUT,                      /*lifetime超时（没法送出去）*/
      DMAC_TX_RTS_FAIL,                     /*RTS 发送失败（超出重传限制：接收cts超时）*/
      DMAC_TX_NOT_COMPRASS_BA,              /*收到的BA是非压缩块确认*/
      DMAC_TX_TID_MISMATCH,                 /*收到的BA中TID与发送时填写在描述符中的TID不一致*/
      DMAC_TX_KEY_SEARCH_FAIL,              /* Key search failed*/
      DMAC_TX_AMPDU_MISMATCH,               /*描述符异常*/
      DMAC_TX_PENDING,                      /*02:没有中断均为pending;03:发送过程中为pending */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
      DMAC_TX_FAIL_ACK_ERROR,               /*发送失败（超过重传限制：接收到的响应帧错误）*/
      DMAC_TX_RTS_FAIL_CTS_ERROR,           /*RTS发送失败（超出重传限制：接收到的CTS错误）*/
#else
      DMAC_TX_FAIL_RESV,                    /* resv */
      DMAC_TX_FAIL_BW_TOO_BIG,              /* 带宽超过PHY的最大工作带宽或流数超过最大天线数， 软件回收该帧 */
#endif
      DMAC_TX_FAIL_ABORT,                   /*发送失败（因为abort）*/
      DMAC_TX_FAIL_STATEMACHINE_PHY_ERROR,  /*MAC发送该帧异常结束（状态机超时、phy提前结束等原因）*/
      DMAC_TX_SOFT_PSM_BACK,                /*软件节能回退*/
      DMAC_TX_AMPDU_BITMAP_MISMATCH,        /*硬件解析bitmap，当前mpdu未被确认*/
} dmac_tx_dscr_status_enum;
#else
/* Type of Tx Descriptor status */
typedef enum
{
      DMAC_TX_INVALID   = 0,
      DMAC_TX_SUCC,
      DMAC_TX_FAIL,
      DMAC_TX_TIMEOUT,
      DMAC_TX_RTS_FAIL,
      DMAC_TX_NOT_COMPRASS_BA,
      DMAC_TX_TID_MISMATCH,
      DMAC_TX_KEY_SEARCH_FAIL,
      DMAC_TX_AMPDU_MISMATCH,
      DMAC_TX_PENDING,
      DMAC_TX_SOFT_PSM_BACK,    /*软件节能回退*/
      DMAC_TX_SOFT_RESET_BACK,  /*软件RESET回退*/
} dmac_tx_dscr_status_enum;
#endif
typedef oal_uint8 dmac_tx_dscr_status_enum_uint8;

typedef enum
{
    DMAC_TX_MODE_NORMAL  = 0,
    DMAC_TX_MODE_RIFS    = 1,
    DMAC_TX_MODE_AGGR    = 2,
    DMAC_TX_MODE_BUTT
}dmac_tx_mode_enum;
typedef oal_uint8 dmac_tx_mode_enum_uint8;

typedef enum
{
    DMAC_TID_PAUSE_RESUME_TYPE_BA   = 0,
    DMAC_TID_PAUSE_RESUME_TYPE_PS   = 1,
    DMAC_TID_PAUSE_RESUME_TYPE_BUTT
}dmac_tid_pause_type_enum;
typedef oal_uint8 dmac_tid_pause_type_enum_uint8;

/* 专用于CB字段自定义帧类型、帧子类型枚举值 */
typedef enum
{
    WLAN_CB_FRAME_TYPE_START   = 0,    /* cb默认初始化为0 */
    WLAN_CB_FRAME_TYPE_ACTION  = 1,    /* action帧 */
    WLAN_CB_FRAME_TYPE_DATA    = 2,    /* 数据帧 */
    WLAN_CB_FRAME_TYPE_MGMT    = 3,    /* 管理帧，用于p2p等需要上报host */

    WLAN_CB_FRAME_TYPE_BUTT
}wlan_cb_frame_type_enum;
typedef oal_uint8 wlan_cb_frame_type_enum_uint8;

/* cb字段action帧子类型枚举定义 */
typedef enum
{
    WLAN_ACTION_BA_ADDBA_REQ  = 0,         /* 聚合action */
    WLAN_ACTION_BA_ADDBA_RSP,
    WLAN_ACTION_BA_DELBA,
#ifdef _PRE_WLAN_FEATURE_WMMAC
    WLAN_ACTION_BA_WMMAC_ADDTS_REQ,
    WLAN_ACTION_BA_WMMAC_ADDTS_RSP,
    WLAN_ACTION_BA_WMMAC_DELTS,
#endif
    WLAN_ACTION_SMPS_FRAME_SUBTYPE,        /* SMPS节能action */
    WLAN_ACTION_OPMODE_FRAME_SUBTYPE,      /* 工作模式通知action */
    WLAN_ACTION_P2PGO_FRAME_SUBTYPE,       /* host发送的P2P go帧，主要是discoverability request */

    WLAN_FRAME_TYPE_ACTION_BUTT
}wlan_cb_action_subtype_enum;
typedef oal_uint8 wlan_cb_frame_subtype_enum_uint8;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
typedef enum
{
    BTCOEX_RX_WINDOW_SIZE_INDEX_0   = 0,
    BTCOEX_RX_WINDOW_SIZE_INDEX_1   = 1,
    BTCOEX_RX_WINDOW_SIZE_INDEX_2   = 2,
    BTCOEX_RX_WINDOW_SIZE_INDEX_3   = 3,

    BTCOEX_RX_WINDOW_SIZE_INDEX_BUTT
}btcoex_rx_window_size_index_enum;
typedef oal_uint8 btcoex_rx_window_size_index_enum_uint8;


typedef enum
{
    BTCOEX_RATE_THRESHOLD_MIN   = 0,
    BTCOEX_RATE_THRESHOLD_MAX   = 1,

    BTCOEX_RATE_THRESHOLD_BUTT
}btcoex_rate_threshold_enum;
typedef oal_uint8 btcoex_rate_threshold_enum_uint8;

typedef enum
{
    BTCOEX_RX_WINDOW_SIZE_HOLD       = 0,
    BTCOEX_RX_WINDOW_SIZE_DECREASE   = 1,
    BTCOEX_RX_WINDOW_SIZE_INCREASE   = 2,

    BTCOEX_RX_WINDOW_SIZE_BUTT
}btcoex_rx_window_size_enum;
typedef oal_uint8 btcoex_rx_window_size_enum_uint8;

typedef enum
{
    BTCOEX_RX_WINDOW_SIZE_GRADE_0   = 0,
    BTCOEX_RX_WINDOW_SIZE_GRADE_1   = 1,

    BTCOEX_RX_WINDOW_SIZE_GRADE_BUTT
}btcoex_rx_window_size_grade_enum;
typedef oal_uint8 btcoex_rx_window_size_grade_enum_uint8;

typedef enum
{
    BTCOEX_ACTIVE_MODE_A2DP      = 0,
    BTCOEX_ACTIVE_MODE_SCO       = 1,
    BTCOEX_ACTIVE_MODE_TRANSFER  = 2,

    BTCOEX_ACTIVE_MODE_BUTT
}btcoex_active_mode_enum;
typedef oal_uint8 btcoex_active_mode_enum_uint8;

typedef enum
{
    BTCOEX_RATE_STATE_H       = 0,
    BTCOEX_RATE_STATE_M       = 1,
    BTCOEX_RATE_STATE_L       = 2,
    BTCOEX_RATE_STATE_SL      = 3,

    BTCOEX_RATE_STATE_BUTT
}btcoex_rate_state_enum;
typedef oal_uint8 btcoex_rate_state_enum_uint8;
#endif

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
/* DMAC_WLAN_CRX_EVENT_SUB_TYPE_SCAN_COMP */
typedef struct
{
    dmac_disasoc_misc_reason_enum_uint16     en_disasoc_reason;
    oal_uint16                               us_user_idx;
}dmac_disasoc_misc_stru;

typedef struct
{
    oal_uint8               uc_tid_num;                         /* 需要发送的tid队列号 */
    dmac_tx_mode_enum_uint8 en_tx_mode;                         /* 调度tid的发送模式 */
    oal_uint8               uc_mpdu_num[DMAC_TX_QUEUE_AGGR_DEPTH];   /* 调度得到的需要发送的mpdu个数 */
    oal_uint16              us_user_idx;                        /* 要发送的tid队列隶属的user */
#ifdef _PRE_WLAN_FEATURE_DFR
    oal_bool_enum_uint8     en_ba_is_jamed;                     /* 当前BA窗是否卡死的标志位 */
    oal_uint8               uc_resv;
#else
    oal_uint8               auc_resv[2];
#endif
}mac_tid_schedule_output_stru;

/* DMAC与HMAC模块共用的WLAN DRX事件结构体 */
typedef struct
{
    oal_netbuf_stru        *pst_netbuf;         /* netbuf链表一个元素 */
    oal_uint16              us_netbuf_num;      /* netbuf链表的个数 */
    oal_uint8               auc_resv[2];        /* 字节对齐 */
}dmac_wlan_drx_event_stru;

/* DMAC与HMAC模块共用的WLAN CRX事件结构体 */
typedef struct
{
    oal_netbuf_stru        *pst_netbuf;         /* 指向管理帧对应的netbuf */
//    oal_uint8              *puc_chtxt;          /* Shared Key认证用的challenge text */
}dmac_wlan_crx_event_stru;

#ifdef _PRE_WLAN_FEATURE_BTCOEX
typedef struct
{
    oal_uint16 us_user_id;
    oal_uint8  uc_ba_size;
    oal_bool_enum_uint8 en_need_delba;
}dmac_to_hmac_btcoex_rx_delba_trigger_event_stru;
#endif

typedef struct
{
    oal_uint16      us_user_index;
    oal_uint8       uc_tid;
    oal_uint8       uc_vap_id;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint8       uc_cur_protocol;
    oal_uint8       auc_reserve[3];
#endif
}dmac_to_hmac_ctx_event_stru;


typedef struct
{
    oal_uint16      us_user_index;
    oal_uint8       uc_cur_bandwidth;
    oal_uint8       uc_cur_protocol;
}dmac_to_hmac_syn_info_event_stru;

typedef struct
{
    oal_uint8       uc_vap_id;
    oal_uint8       en_voice_aggr;              /* 是否支持Voice聚合 */
    oal_uint8       auc_resv[2];
}dmac_to_hmac_voice_aggr_event_stru;

typedef struct
{
    oal_uint8                      uc_device_id;
    wlan_nss_enum_uint8            en_m2s_nss;
    wlan_m2s_type_enum_uint8       en_m2s_type;  /*0:软切换 1:硬切换*/
    oal_uint8                      auc_reserve[1];
}dmac_to_hmac_m2s_event_stru;

/*mic攻击*/
typedef struct
{
    oal_uint8                  auc_user_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                  auc_reserve[2];
    oal_nl80211_key_type       en_key_type;
    oal_int32                  l_key_id;
}dmac_to_hmac_mic_event_stru;

/*DMAC与HMAC模块共用的DTX事件结构体 */
typedef struct
{
    oal_netbuf_stru        *pst_netbuf;         /* netbuf链表一个元素 */
    oal_uint16              us_frame_len;
    oal_uint16              us_remain;
}dmac_tx_event_stru;


typedef struct
{
    mac_channel_stru                     st_channel;
    mac_ch_switch_info_stru              st_ch_switch_info;

    oal_bool_enum_uint8                  en_switch_immediately; /* 1 - 马上切换  0 - 暂不切换, 推迟到tbtt中切换*/
    oal_bool_enum_uint8                  en_check_cac;
    oal_bool_enum_uint8                  en_dot11FortyMHzIntolerant;
    oal_uint8                            auc_resv[1];
}dmac_set_chan_stru;

typedef struct
{
    wlan_ch_switch_status_enum_uint8     en_ch_switch_status;      /* 信道切换状态 */
    oal_uint8                            uc_announced_channel;     /* 新信道号 */
    wlan_channel_bandwidth_enum_uint8    en_announced_bandwidth;   /* 新带宽模式 */
    oal_uint8                            uc_ch_switch_cnt;         /* 信道切换计数 */
    oal_bool_enum_uint8                  en_csa_present_in_bcn;    /* Beacon帧中是否包含CSA IE */
    oal_uint8                            uc_csa_vap_cnt;           /* 需要发送csa的vap个数 */
    wlan_csa_mode_tx_enum_uint8          en_csa_mode;
    mac_csa_flag_enum_uint8              en_csa_debug_flag;
}dmac_set_ch_switch_info_stru;

typedef struct
{
    oal_uint8                            uc_cac_machw_en;          /* 1min cac 发送队列开关状态 */
}dmac_set_cac_machw_info_stru;

#ifdef _PRE_WLAN_FEATURE_WMMAC
/*挂载在dmac_ctx_action_event_stru.uc_resv[2]传递给dmac*/
typedef struct
{
    oal_uint8   uc_ac;
    oal_uint8   bit_psb       : 1;
    oal_uint8   bit_direction : 7;
}dmac_addts_info_stru;
#endif

/*
    (1)DMAC与HMAC模块共用的CTX子类型ACTION对应的事件的结构体
    (2)当DMAC自身产生DELBA帧时，使用该结构体向HMAC模块抛事件
*/
 typedef struct
 {
    mac_category_enum_uint8     en_action_category;     /* ACTION帧的类型 */
    oal_uint8                   uc_action;              /* 不同ACTION类下的子帧类型 */
    oal_uint16                  us_user_idx;
    oal_uint16                  us_frame_len;           /* 帧长度 */
    oal_uint8                   uc_hdr_len;             /* 帧头长度 */
    oal_uint8                   uc_tidno;               /* tidno，部分action帧使用 */
    oal_uint8                   uc_initiator;           /* 触发端方向 */

    /* 以下为接收到req帧，发送rsp帧后，需要同步到dmac的内容 */
    oal_uint8                       uc_status;              /* rsp帧中的状态 */
    oal_uint16                      us_baw_start;           /* 窗口开始序列号 */
    oal_uint16                      us_baw_size;            /* 窗口大小 */
    oal_uint8                       uc_ampdu_max_num;       /* BA会话下的最多聚合的AMPDU的个数 */
    oal_bool_enum_uint8             en_amsdu_supp;          /* 是否支持AMSDU */
    oal_uint16                      us_ba_timeout;          /* BA会话交互超时时间 */
    mac_back_variant_enum_uint8     en_back_var;            /* BA会话的变体 */
    oal_uint8                       uc_dialog_token;        /* ADDBA交互帧的dialog token */
    oal_uint8                       uc_ba_policy;           /* Immediate=1 Delayed=0 */
    oal_uint8                       uc_lut_index;           /* LUT索引 */
    oal_uint8                       auc_mac_addr[WLAN_MAC_ADDR_LEN];    /* 用于DELBA/DELTS查找HMAC用户 */
#ifdef _PRE_WLAN_FEATURE_WMMAC
    oal_uint8                       uc_tsid;                /* TS相关Action帧中的tsid值 */
    oal_uint8                       uc_ts_dialog_token;     /* ADDTS交互帧的dialog token */
    dmac_addts_info_stru            st_addts_info;
#endif
 }dmac_ctx_action_event_stru;

/* 添加用户事件payload结构体 */
typedef struct
{
    oal_uint16  us_user_idx;     /* 用户index */
    oal_uint8   auc_user_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_uint16  us_sta_aid;
    oal_uint8   auc_bssid[WLAN_MAC_ADDR_LEN];

    mac_vht_hdl_stru          st_vht_hdl;
    mac_user_ht_hdl_stru      st_ht_hdl;
    mac_ap_type_enum_uint16   en_ap_type;
    oal_int8                  c_rssi;                          /* 用户bss的信号强度 */
    oal_uint8                 auc_rev[1];

//    oal_uint8   bit_transmit_staggered_sounding_cap : 1,
//                bit_exp_comp_feedback               : 2,
//                bit_su_beamformer_cap               : 1,                       /* SU bfer能力，要过AP认证，必须填1 */
//                bit_su_beamformee_cap               : 1,                       /* SU bfee能力，要过STA认证，必须填1 */
//                bit_resv                            : 3;

 //   oal_uint8   bit_num_bf_ant_supported            : 3,                       /* SU时，最大接收NDP的Nsts，最小是1 */
 //               bit_num_sounding_dim                : 3,                       /* SU时，表示Nsts最大值，最小是1 */
 //               bit_mu_beamformer_cap               : 1,                       /* 不支持，set to 0 */
//                bit_mu_beamformee_cap               : 1;                       /* 不支持，set to 0 */
}dmac_ctx_add_user_stru;

/* 删除用户事件结构体 */
typedef struct
{
    oal_uint16                us_user_idx;     /* 用户index */
    oal_uint8                 auc_user_mac_addr[WLAN_MAC_ADDR_LEN];
    mac_ap_type_enum_uint16   en_ap_type;
    oal_uint8                 auc_resv[2];
}dmac_ctx_del_user_stru;

/* 扫描请求事件payload结构体 */
typedef struct
{
    mac_scan_req_stru   *pst_scan_params;   /* 将扫描参数传下去 */
}dmac_ctx_scan_req_stru;

typedef struct
{
    oal_uint8                   uc_scan_idx;
    oal_uint8                   auc_resv[3];
    wlan_scan_chan_stats_stru    st_chan_result;
}dmac_crx_chan_result_stru;

/* Update join req 需要配置的速率集参数 */
typedef struct
{
    union
    {
        oal_uint8           uc_value;
        struct
        {
            oal_uint8     bit_support_11b  : 1;   /* 该AP是否支持11b */
            oal_uint8     bit_support_11ag : 1;  /* 该AP是否支持11ag */
            oal_uint8     bit_ht_capable   : 1;    /* 是否支持ht */
            oal_uint8     bit_vht_capable  : 1;   /* 是否支持vht */
            oal_uint8     bit_reserved     : 4;
        }st_capable;/* 与dmac层wlan_phy_protocol_enum对应 */
    }un_capable_flag;
    oal_uint8               uc_min_rate[2];/*第一个存储11b协议对应的速率，第二个存储11ag协议对应的速率*/
    oal_uint8               uc_reserved;
}dmac_set_rate_stru;

/* Update join req 参数写寄存器的结构体定义 */
typedef struct
{
    oal_uint8               auc_bssid[WLAN_MAC_ADDR_LEN];    /* 加入的AP的BSSID  */
    oal_uint16              us_beacon_period;
    mac_channel_stru        st_current_channel;              /* 要切换的信道信息 */
    oal_uint32              ul_beacon_filter;                /* 过滤beacon帧的滤波器开启标识位 */
    oal_uint32              ul_non_frame_filter;              /* 过滤no_frame帧的滤波器开启标识位 */
    oal_uint8               auc_ssid[WLAN_SSID_MAX_LEN];    /* 加入的AP的SSID  */
    oal_uint8               uc_dtim_period;                 /* dtim period      */
    oal_bool_enum_uint8     en_dot11FortyMHzOperationImplemented;         /* dot11FortyMHzOperationImplemented */
    oal_uint8               auc_resv;
    dmac_set_rate_stru      st_min_rate;          /* Update join req 需要配置的速率集参数 */
}dmac_ctx_join_req_set_reg_stru;

/* wait join写寄存器参数的结构体定义 */
typedef struct
{
    oal_uint32              ul_dtim_period;                  /* dtim period */
    oal_uint32              ul_dtim_cnt;                     /* dtim count  */
    oal_uint8               auc_bssid[WLAN_MAC_ADDR_LEN];    /* 加入的AP的BSSID  */
    oal_uint16              us_tsf_bit0;                     /* tsf bit0  */
}dmac_ctx_set_dtim_tsf_reg_stru;

/* wait join misc写寄存器参数的结构体定义 */
typedef struct
{
    oal_uint32              ul_beacon_filter;                /* 过滤beacon帧的滤波器开启标识位 */
}dmac_ctx_join_misc_set_reg_stru;

/* wait join写寄存器参数的结构体定义 */
typedef struct
{
    oal_uint16             uc_user_index;                    /* user index */
    oal_uint8              auc_resv[2];
}dmac_ctx_asoc_set_reg_stru;

/* sta更新edca参数寄存器的结构体定义 */
typedef struct
{
    oal_uint8                            uc_vap_id;
    mac_wmm_set_param_type_enum_uint8    en_set_param_type;
    oal_uint16                           us_user_index;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    wlan_mib_Dot11QAPEDCAEntry_stru      ast_wlan_mib_qap_edac[WLAN_WME_AC_BUTT];
#ifdef _PRE_WLAN_FEATURE_WMMAC
    mac_ts_info_stru                     st_ts_info[WLAN_WME_AC_BUTT];
#endif
#endif
}dmac_ctx_sta_asoc_set_edca_reg_stru;

#if 0
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#pragma pack(push,1)
/* 裸系统下DMAC模块与HMAC模块共用的接收流程控制信息数据结构定义, 与hal_rx_ctl_stru结构体保持一致*/
typedef struct
{
    /*byte 0*/
    oal_uint8                   bit_vap_id            :5;   /* 必须为5bits，与RX DSCR匹配，其他bss WLAN_HAL_OHTER_BSS_ID = 14 */
    oal_uint8                   bit_amsdu_enable      :1;
    oal_uint8                   bit_is_first_buffer   :1;
    oal_uint8                   bit_is_fragmented     :1;

    /*byte 1*/
    oal_uint8                   uc_msdu_in_buffer;

    /*byte 2*/
    oal_uint8                   bit_ta_user_idx       :5;
    oal_uint8                   bit_reserved2         :1;
    oal_uint8                   bit_reserved3         :1;
    oal_uint8                   bit_reserved4         :1;

    /*byte 3*/
    oal_uint8                   bit_mac_header_len    :6;   /* mac header帧头长度 */
    oal_uint8                   bit_is_beacon         :1;
    oal_uint8                   bit_reserved1         :1;

    /*byte 4-5 */
    oal_uint16                  us_frame_len;               /* 帧头与帧体的总长度 */

    /*byte 6 */
    oal_uint8                   uc_mac_vap_id         :4;
    oal_uint8                   bit_buff_nums         :4; /* 每个MPDU占用的buf数 */

    /*byte 7 */
    oal_uint8                   uc_channel_number;          /* 接收帧的信道 */

}mac_rx_ctl_stru;
#pragma pack(pop)

#else
/* DMAC模块与HMAC模块共用的接收流程控制信息数据结构定义, 与hal_rx_ctl_stru结构体保持一致*/
typedef struct
{
    /*word 0*/
    oal_uint8                   bit_vap_id            :5;
    oal_uint8                   bit_mgmt_to_hostapd   :1;
    oal_uint8                   bit_reserved1         :2;

    oal_uint8                   uc_msdu_in_buffer;
    oal_uint8                   bit_amsdu_enable      :1;
    oal_uint8                   bit_is_first_buffer   :1;
    oal_uint8                   bit_is_fragmented     :1;
    oal_uint8                   bit_is_beacon         :1;
    oal_uint8                   bit_buff_nums         :4;   /* 每个MPDU占用的buf数目 */

    oal_uint8                   uc_mac_header_len;          /* mac header帧头长度 */
    /*word 1*/
    oal_uint16                  us_frame_len;               /* 帧头与帧体的总长度 */
    oal_uint16                  us_da_user_idx;             /* 目的地址用户索引 */
    /*word 2*/
    oal_uint32                 *pul_mac_hdr_start_addr;     /* 对应的帧的帧头地址,虚拟地址 */
    /*word 3*/
    oal_uint16                  us_ta_user_idx;             /* 发送端地址用户索引 */
    oal_uint8                   uc_mac_vap_id;
    oal_uint8                   uc_channel_number;          /* 接收帧的信道 */
}mac_rx_ctl_stru;
#endif
#endif

typedef  hal_rx_ctl_stru   mac_rx_ctl_stru;

/* DMAC模块接收流程控制信息结构，存放于对应的netbuf的CB字段中，最大值为48字节,
   如果修改，一定要通知sdt同步修改，否则解析会有错误!!!!!!!!!!!!!!!!!!!!!!!!!*/
typedef struct
{
    hal_rx_ctl_stru             st_rx_info;         /* dmac需要传给hmac的数据信息 */
    hal_rx_status_stru          st_rx_status;       /* 保存加密类型及帧长信息 */
    hal_rx_statistic_stru       st_rx_statistic;    /* 保存接收描述符的统计信息 */
}dmac_rx_ctl_stru;

/* hmac to dmac配置同步消息结构 */
typedef struct
{
    wlan_cfgid_enum_uint16              en_syn_id;      /* 同步事件ID*/
    oal_uint16                          us_len;         /* 事件payload长度 */
    oal_uint8                           auc_msg_body[4];/* 事件payload */
}hmac_to_dmac_cfg_msg_stru;

typedef hmac_to_dmac_cfg_msg_stru dmac_to_hmac_cfg_msg_stru;

/* HMAC到DMAC配置同步操作 */
typedef struct
{
    wlan_cfgid_enum_uint16      en_cfgid;
    oal_uint8                   auc_resv[2];
    oal_uint32                  (*p_set_func)(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
}dmac_config_syn_stru;
typedef dmac_config_syn_stru hmac_config_syn_stru;



#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* 1字节对齐 */
#pragma pack(push,1)
typedef struct
{
    mac_ieee80211_frame_stru               *pst_frame_header;                           /* 该MPDU的帧头指针 */
    oal_uint16                              us_seqnum;                                  /* 记录软件分配的seqnum*/
    wlan_frame_type_enum_uint8              en_frame_type;                              /* 该帧是控制针、管理帧、数据帧 */
    oal_uint8                               bit_80211_mac_head_type     :1;             /* 0: 802.11 mac头不在skb中，另外申请了内存存放； 1: 802.11 mac头在skb中*/
    oal_uint8                               en_res                      :7;             /* 是否使用4地址，由WDS特性决定 */
}mac_tx_expand_cb_stru;


/* 裸系统cb字段 只有20字节可用, 当前使用19字节; HCC[8]+PAD[1]+CB[19]+MAC HEAD[36] */
struct  mac_tx_ctl
{
    /* byte1 */
    frw_event_type_enum_uint8               bit_event_type              :5;          /* 取值:FRW_EVENT_TYPE_WLAN_DTX和FRW_EVENT_TYPE_HOST_DRX，作用:在释放时区分是内存池的netbuf还是原生态的 */
    oal_uint8                               bit_event_sub_type          :3;
    /* byte2-3 */
    wlan_cb_frame_type_enum_uint8           uc_frame_type;                           /* 自定义帧类型 */
    wlan_cb_frame_subtype_enum_uint8        uc_frame_subtype;                        /* 自定义帧子类型 */
    /* byte4 */
    oal_uint8                               bit_mpdu_num                :7;          /* ampdu中包含的MPDU个数,实际描述符填写的值为此值-1 */
    oal_uint8                               bit_netbuf_num              :1;          /* 每个MPDU占用的netbuf数目 *//* 在每个MPDU的第一个NETBUF中有效 */
    /* byte5-6 */
    oal_uint16                              us_mpdu_payload_len;                     /* 每个MPDU的长度不包括mac header length */
    /* byte7 */
    oal_uint8                               bit_frame_header_length     :6;          /* 51四地址32 */ /* 该MPDU的802.11头长度 */
    oal_uint8                               bit_is_amsdu                :1;          /* 是否AMSDU: OAL_FALSE不是，OAL_TRUE是 */
    oal_uint8                               bit_is_first_msdu           :1;          /* 是否是第一个子帧，OAL_FALSE不是 OAL_TRUE是 */
    /* byte8 */
    oal_uint8                               bit_tid                     :4;          /* dmac tx 到 tx complete 传递的user结构体，目标用户地址 */
    wlan_wme_ac_type_enum_uint8             bit_ac                      :3;          /* ac */
    oal_uint8                               bit_ismcast                 :1;          /* 该MPDU是单播还是多播:OAL_FALSE单播，OAL_TRUE多播 */
    /* byte9 */
    oal_uint8                               bit_retried_num             :4;          /* 重传次数 */
    oal_uint8                               bit_mgmt_frame_id           :4;          /* wpas 发送管理帧的frame id */
    /* byte10 */
    oal_uint8                               bit_tx_user_idx             :6;          /* 比描述符中userindex多一个bit用于标识无效index */
    oal_uint8                               bit_roam_data               :1;          /* 漫游期间帧发送标记 */
    oal_uint8                               bit_is_get_from_ps_queue    :1;          /* 节能特性用，标识一个MPDU是否从节能队列中取出来的 */
    /* byte11 */
    oal_uint8                               bit_tx_vap_index            :3;
    wlan_tx_ack_policy_enum_uint8           en_ack_policy               :3;
    oal_uint8                               bit_is_needretry            :1;
    oal_uint8                               bit_need_rsp                :1;          /* WPAS send mgmt,need dmac response tx status */
    /* byte12 */
    dmac_user_alg_probe_enum_uint8          en_is_probe_data            :3;          /* 是否探测帧 */
    oal_uint8                               bit_is_eapol_key_ptk        :1;          /* 4 次握手过程中设置单播密钥EAPOL KEY 帧标识 */
    oal_uint8                               bit_is_m2u_data             :1;          /* 是否是组播转单播的数据 */
    oal_uint8                               bit_is_large_skb_amsdu      :1;          /* 是否是多子帧大包聚合 */
    oal_uint8                               bit_ether_head_including    :1;          /* offload下netbuf头部LLC之前是否有etherhead */
    oal_uint8                               en_use_4_addr               :1;             /* 是否使用4地址，由WDS特性决定 */
    /* byte13-16 */
    oal_uint32                              ul_timestamp_us;                         /* 维测使用入TID队列的时间戳, 单位1us精度 */
    /* byte17-18 */
    oal_uint8                               uc_alg_pktno;                            /* 算法用到的字段，唯一标示该报文 */
    oal_uint8                               uc_alg_frame_tag;                        /* 用于算法对帧进行标记 */
    /* byte19 */
    oal_uint8                               bit_align_padding_offset    :2;
    oal_uint8                               bit_is_tcp_ack              :1;          /* 用于标记tcp ack */
    oal_uint8                               bit_resv                    :5;

#ifndef _PRE_PRODUCT_ID_HI110X_DEV
    /* OFFLOAD架构下，HOST相对DEVICE的CB增量 */
    mac_tx_expand_cb_stru                   st_expand_cb;
#endif
}__OAL_DECLARE_PACKED;
typedef struct mac_tx_ctl  mac_tx_ctl_stru;
#pragma pack(pop)

#else
/* netbuf控制字段(CB)，总长度为48字节, 如果修改，一定要通知sdt同步修改. */
typedef struct
{
    oal_uint8                               uc_mpdu_num;                                /* ampdu中包含的MPDU个数,实际描述符填写的值为此值-1 */
    oal_uint8                               uc_netbuf_num;                              /* 每个MPDU占用的netbuf数目 */
    oal_uint16                              us_mpdu_payload_len;                        /* 每个MPDU的长度不包括mac header length */
    oal_uint8                               uc_frame_header_length;                     /* 该MPDU的802.11头长度 */

    oal_uint8                               bit_is_amsdu                :1;             /* 是否AMSDU: OAL_FALSE不是，OAL_TRUE是 */
    oal_uint8                               bit_ismcast                 :1;             /* 该MPDU是单播还是多播:OAL_FALSE单播，OAL_TRUE多播 */
    oal_uint8                               en_use_4_addr               :1;             /* 是否使用4地址，由WDS特性决定 */
    oal_uint8                               bit_is_get_from_ps_queue    :1;             /* 节能特性用，标识一个MPDU是否从节能队列中取出来的 */
    oal_uint8                               bit_is_first_msdu           :1;             /* 是否是第一个子帧，OAL_FALSE不是 OAL_TRUE是 */
    oal_uint8                               bit_is_m2u_data             :1;             /* 是否是组播转单播的数据 */
    oal_uint8                               en__res                     :2;

    oal_uint8                               en_is_qosdata               :1;             /* 指示该帧是否是qos data */
    oal_uint8                               bit_80211_mac_head_type     :1;             /* 0: 802.11 mac头不在skb中，另外申请了内存存放； 1: 802.11 mac头在skb中*/
    oal_uint8                               en_is_bar                   :1;
    oal_uint8                               bit_is_needretry            :1;
    oal_uint8                               en_seq_ctrl_bypass          :1;             /* 该帧的SN号由软件维护，硬件禁止维护(目前仅用于非QOS分片帧 ) */
    oal_uint8                               bit_need_rsp                :1;             /* WPAS send mgmt,need dmac response tx status */
    oal_uint8                               bit_is_eapol_key_ptk        :1;             /* 4 次握手过程中设置单播密钥EAPOL KEY 帧标识 */
    oal_uint8                               bit_roam_data               :1;

    wlan_frame_type_enum_uint8              en_frame_type;                              /* 帧类型：数据帧，管理帧... */
    mac_ieee80211_frame_stru               *pst_frame_header;                           /* 该MPDU的帧头指针 */
    wlan_wme_ac_type_enum_uint8             uc_ac;                                      /* ac */
    oal_uint8                               uc_tid;                                     /* tid */
    frw_event_type_enum_uint8               en_event_type;                              /* 取值:FRW_EVENT_TYPE_WLAN_DTX和FRW_EVENT_TYPE_HOST_DRX，作用:在释放时区分是内存池的netbuf还是原生态的 */
    oal_uint8                               uc_event_sub_type;                          /* amsdu抛事件用的 */
    wlan_tx_ack_policy_enum_uint8           en_ack_policy;                              /* ACK 策略 */
    oal_uint8                               uc_tx_vap_index;
    oal_uint16                              us_tx_user_idx;                             /* dmac tx 到 tx complete 传递的user结构体，目标用户地址 */
    oal_uint8                               uc_retried_num;
    dmac_user_alg_probe_enum_uint8          en_is_probe_data;                           /* 是否是探测帧 */
    oal_uint16                              us_seqnum;                                  /* 记录软件分配的seqnum*/
    hal_tx_dscr_stru                       *pst_bar_dscr;


    oal_uint8                               bit_mgmt_frame_id           :4;             /* wpas 发送管理帧的frame id */
    oal_uint8                               bit_is_large_skb_amsdu      :1;          /* 是否是多子帧大包聚合 */
    oal_uint8                               bit_ether_head_including    :1;
    oal_uint8                               bit_reserved3               :2;

    wlan_cb_frame_type_enum_uint8           uc_frame_type;                              /* 自定义帧类型 */
    wlan_cb_frame_subtype_enum_uint8        uc_frame_subtype;                           /* 自定义帧子类型 */


    oal_uint32                              ul_timestamp_us;                            /* 维测使用入TID队列时的时间戳, 单位1us精度 */

    oal_uint32                              ul_alg_pktno;                               /* 算法用到的字段，唯一标示该报文 */
    oal_uint8                               uc_alg_frame_tag;                           /* 用于算法对帧进行标记 */

}mac_tx_ctl_stru;
#endif


typedef struct
{
    oal_uint32      ul_best_rate_goodput_kbps;
    oal_uint32      ul_rate_kbps;           /* 速率大小(单位:kbps) */
    oal_uint8       uc_aggr_subfrm_size;    /* 聚合子帧数门限值 */
    oal_uint8       uc_per;                 /* 该速率的PER(单位:%) */
#ifdef _PRE_WLAN_DFT_STAT
    oal_uint8       uc_best_rate_per;       /* 最优速率的PER(单位:%) */
    oal_uint8       uc_resv[1];
#else
    oal_uint8       uc_resv[2];
#endif
}dmac_tx_normal_rate_stats_stru;

typedef struct
{
    oal_bool_enum_uint8                     in_use;                     /* 缓存BUF是否被使用 */
    oal_uint8                               uc_num_buf;                 /* MPDU对应的描述符的个数 */
    oal_uint16                              us_seq_num;                 /* MPDU对应的序列号 */
    oal_netbuf_head_stru                    st_netbuf_head;             /* MPDU对应的描述符首地址 */
    oal_uint32                              ul_rx_time;                 /* 报文被缓存的时间戳 */
} dmac_rx_buf_stru;


typedef struct
{
    oal_void                               *pst_ba;
    oal_uint8                               uc_tid;
    mac_delba_initiator_enum_uint8          en_direction;
    oal_uint16                              us_mac_user_idx;
    oal_uint8                               uc_vap_id;
    oal_uint16                              us_timeout_times;
    oal_uint8                               uc_resv[1];
}dmac_ba_alarm_stru;


/* 一个站点下的每一个TID下的聚合接收的状态信息 */
typedef struct
{
    dmac_ba_conn_status_enum_uint8  en_ba_conn_status;    /* BA会话的状态 */
    oal_uint8                       uc_lut_index;         /* 接收端Session H/w LUT Index */
    mac_ba_policy_enum_uint8        uc_ba_policy;         /* Immediate=1 Delayed=0 */
    oal_uint8                       resv;
}dmac_ba_rx_stru;

typedef struct
{
    oal_uint8   uc_in_use;
    oal_uint8   uc_resv[1];
    oal_uint16  us_seq_num;
    oal_void*   p_tx_dscr;
}dmac_retry_queue_stru;

typedef struct
{
    oal_uint16                      us_baw_start;           /* 第一个未确认的MPDU的序列号 */
    oal_uint16                      us_last_seq_num;        /* 最后一个发送的MPDU的序列号 */
    oal_uint16                      us_baw_size;            /* Block_Ack会话的buffer size大小 */
    oal_uint16                      us_baw_head;            /* bitmap中记录的第一个未确认的包的位置 */
    oal_uint16                      us_baw_tail;            /* bitmap中下一个未使用的位置 */
    oal_bool_enum_uint8             en_is_ba;               /* Session Valid Flag */
    dmac_ba_conn_status_enum_uint8  en_ba_conn_status;      /* BA会话的状态 */
    mac_back_variant_enum_uint8     en_back_var;            /* BA会话的变体 */
    oal_uint8                       uc_dialog_token;        /* ADDBA交互帧的dialog token */
    oal_uint8                       uc_ba_policy;           /* Immediate=1 Delayed=0 */
    oal_bool_enum_uint8             en_amsdu_supp;          /* BLOCK ACK支持AMSDU的标识 */
    oal_uint8                      *puc_dst_addr;           /* BA会话接收端地址 */
    oal_uint16                      us_ba_timeout;          /* BA会话交互超时时间 */
    oal_uint8                       uc_ampdu_max_num;       /* BA会话下，能够聚合的最大的mpdu的个数 */
    oal_uint8                       uc_tx_ba_lut;           /* BA会话LUT session index */
    oal_uint16                      us_mac_user_idx;
#ifdef _PRE_WLAN_FEATURE_DFR
    oal_uint16                      us_pre_baw_start;       /* 记录前一次判断ba窗是否卡死时的ssn */
    oal_uint16                      us_pre_last_seq_num;    /* 记录前一次判断ba窗是否卡死时的lsn */
    oal_uint16                      us_ba_jamed_cnt;        /* BA窗卡死统计次数 */
#else
    oal_uint8                       auc_resv[2];
#endif
    oal_uint32                      aul_tx_buf_bitmap[DMAC_TX_BUF_BITMAP_WORDS];
}dmac_ba_tx_stru;

/* 11n下的参数，需要在关联时进行设置 */
typedef struct
{
    oal_uint8               uc_ampdu_max_num;
    oal_uint8               uc_he_ampdu_len_exponent;
    oal_uint16              us_ampdu_max_size;
    oal_uint32              ul_ampdu_max_size_vht;
}dmac_ht_handle_stru;

#ifdef _PRE_WLAN_DFT_STAT
typedef struct
{
    /* 入队统计 */
    oal_uint32              ul_tid_enqueue_total_cnt;            /* 入队总数目统计 */
    oal_uint32              ul_tid_enqueue_ptr_null_cnt;         /* 入队函数空指针流程导致丢包统计 */
    oal_uint32              ul_tid_enqueue_full_cnt;             /* 入队时队列满导致丢包统计 */
    oal_uint32              ul_tid_enqueue_head_ptr_null_cnt;    /* 发送完成中入队，空指针流程丢包 */
    oal_uint32              ul_tid_enqueue_head_full_cnt;        /* 发送完成中入队，队列满丢包统计 */

    /* 出队统计 */
    oal_uint32              ul_tid_dequeue_total_cnt;            /* 出队总数目统计 */
    oal_uint32              ul_tid_dequeue_normal_cnt;           /* 非AMPDU时出队个数 */
    oal_uint32              ul_tid_dequeue_ampdu_cnt;            /* AMPDU出队个数 */

    /* 丢包统计 */
    oal_uint32              ul_tid_video_dropped_cnt;            /* 视频丢包个数，从tid拿出来直接删除 */
    oal_uint32              ul_tid_traffic_ctl_dropped_cnt;      /* 流控丢包个数，从tid拿出来直接删除 */
    oal_uint32              ul_tid_txbuf_overflow_dropped_cnt;   /* 发送时buf溢出丢包 */
    oal_uint32              ul_tid_dbac_reset_dropped_cnt;             /* dbac复位case信息丢包 */

    /* 重传统计 */
    oal_uint32              ul_tid_retry_enqueue_cnt;            /* 重传包入队数目统计 */
    oal_uint32              ul_tid_retry_dequeue_cnt;            /* 重传包出队数目统计 */
}dmac_tid_stats_stru;
#endif

#ifdef _PRE_DEBUG_MODE
typedef oam_stats_ampdu_stat_stru dmac_tid_ampdu_stat_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
typedef struct
{
    oal_netbuf_head_stru            st_hdr;
    frw_timeout_stru                st_timer;
    oal_spin_lock_stru              st_spin_lock;
    oal_uint16                      us_tcp_ack_num;
    oal_uint8                       auc_resv[2];
}dmac_tid_tcp_ack_buf_stru;
#endif


typedef struct
{
    oal_uint8               uc_tid;                 /* 通信标识符 */
    oal_uint8               uc_is_paused;           /* TID被暂停调度 */
    oal_uint8               uc_num_dq;              /* 可以加到ba窗的包的个数 */
    oal_uint8               uc_retry_num;           /* tid队列中重传报文的个数 */
    oal_uint16              us_mpdu_num;            /* MPDU个数 */
    oal_uint16              us_user_idx;            /* 无效值为MAC_RES_MAC_USER_NUM */

    oal_uint32              ul_mpdu_avg_len;        /* mpdu滑动平均长度 */
#ifdef _PRE_WLAN_FEATURE_TX_DSCR_OPT
    oal_dlist_head_stru     st_retry_q;             /* 重传队列链表 */
    oal_netbuf_head_stru    st_buff_head;           /* 发送缓存队列链表 */
#else
    oal_dlist_head_stru     st_hdr;                 /* tid缓存队列头 */
#endif /* _PRE_WLAN_FEATURE_TX_DSCR_OPT */
    oal_void               *p_alg_priv;             /* TID级别算法私有结构体 */
    dmac_tx_normal_rate_stats_stru st_rate_stats;   /* 速率算法在发送完成中统计出的信息 */
    dmac_ba_tx_stru        *pst_ba_tx_hdl;
    dmac_ba_rx_stru         st_ba_rx_hdl;           /* dmac rx ba info */

    dmac_ht_handle_stru     st_ht_tx_hdl;
    oal_uint8               uc_num_tx_ba;
    oal_uint8               uc_num_rx_ba;
    oal_uint8               uc_vap_id;
    dmac_tx_mode_enum_uint8 en_tx_mode;             /* 发送模式: rifs,aggr,normal发送 */
    oal_bool_enum_uint8     en_is_delba_ing;        /* 该tid是否正在发delba */
    oal_uint8               uc_rx_wrong_ampdu_num;  /* 该tid未建立BA会话时收到的聚合子帧数(一般是DELBA失败) */
    oal_uint8               uc_tcp_ack_proportion;  /* 衡量tid中tck ack大致比例 */
    oal_uint8               uc_last_is_amsdu;       /* 上一帧是否为amsdu */

    /* ROM化后资源扩展指针 */
    oal_void                           *_rom;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_DFT_STAT
    dmac_tid_stats_stru    *pst_tid_stats;           /* 该TID下入队、出队包数，丢包数统计 */
#endif
#endif
#ifdef _PRE_DEBUG_MODE
    dmac_tid_ampdu_stat_stru *pst_tid_ampdu_stat;    /* ampdu业务流程统计信息 */
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_uint16             us_last_seq_frag_num;          /* 保存的前一个QoS帧的seq + frag num */
    oal_uint8              auc_resv2[2];
#endif

}dmac_tid_stru;

/* 处理MPDU的MSDU的处理状态的结构体的定义 */
typedef struct
{
    oal_netbuf_stru        *pst_curr_netbuf;              /* 当前处理的netbuf指针 */
    oal_uint8              *puc_curr_netbuf_data;         /* 当前处理的netbuf的data指针 */
    oal_uint16              us_submsdu_offset;            /* 当前处理的submsdu的偏移量,   */
    oal_uint8               uc_msdu_nums_in_netbuf;       /* 当前netbuf包含的总的msdu数目 */
    oal_uint8               uc_procd_msdu_in_netbuf;      /* 当前netbuf中已处理的msdu数目 */
    oal_uint8               uc_netbuf_nums_in_mpdu;       /* 当前MPDU的中的总的netbuf的数目 */
    oal_uint8               uc_procd_netbuf_nums;         /* 当前MPDU中已处理的netbuf的数目 */
    oal_uint8               uc_procd_msdu_nums_in_mpdu;   /* 当前MPDU中已处理的MSDU数目 */

    oal_uint8               uc_flag;
}dmac_msdu_proc_state_stru;

/* 每一个MSDU包含的内容的结构体的定义 */
typedef struct
{
    oal_uint8               auc_sa[WLAN_MAC_ADDR_LEN];      /* MSDU发送的源地址 */
    oal_uint8               auc_da[WLAN_MAC_ADDR_LEN];      /* MSDU接收的目的地址 */
    oal_uint8               auc_ta[WLAN_MAC_ADDR_LEN];      /* MSDU接收的发送地址 */
    oal_uint8               uc_priority;
    oal_uint8               auc_resv[1];

    oal_netbuf_stru        *pst_netbuf;                     /* MSDU对应的netbuf指针(可以使clone的netbuf) */
}dmac_msdu_stru;

/* 复位原因定义 */
typedef enum
{
    DMAC_RESET_REASON_SW_ERR = 0,
    DMAC_RESET_REASON_HW_ERR,
    DMAC_RESET_REASON_CONFIG,
    DMAC_RETST_REASON_OVER_TEMP,
    DMAC_RESET_REASON_TX_COMP_TIMEOUT,

    DMAC_RESET_REASON_BUTT
}dmac_reset_mac_submod_enum;
typedef oal_uint8 dmac_reset_mac_submod_enum_uint8;

typedef struct
{
    hal_reset_hw_type_enum_uint8     uc_reset_type;        /*命令类型0|1|2|3(all|phy|mac|debug)*/
    hal_reset_mac_submod_enum_uint8  uc_reset_mac_mod;
    dmac_reset_mac_submod_enum_uint8 en_reason;
    oal_uint8 uc_reset_phy_reg;
    oal_uint8 uc_reset_mac_reg;
    oal_uint8 uc_debug_type;                              /*debug时，参数2复用为类型，0|1|2(mac reg|phy reg|lut)*/
    oal_uint8 uc_reset_rf_reg;
    oal_uint8 auc_resv[1];
}dmac_reset_para_stru;

typedef struct
{
    oal_uint8 uc_reason;
    oal_uint8 uc_event;
    oal_uint8 auc_des_addr[WLAN_MAC_ADDR_LEN];
}dmac_diasoc_deauth_event;

#ifdef _PRE_DEBUG_MODE_USER_TRACK
/* 软件最优速率统计信息结构体 */
typedef struct
{
    oal_uint32      ul_best_rate_kbps;          /* 统计周期内的最优速率 */
    oal_uint16      us_best_rate_per;           /* 统计周期内最优速率下的平均per */
    oal_uint8       uc_best_rate_aggr_num;      /* 统计周期内最优速率的aggr */
    oal_uint8       uc_resv;
}dmac_best_rate_traffic_stat_info_stru;

typedef struct
{
    dmac_best_rate_traffic_stat_info_stru    ast_best_rate_stat[WLAN_WME_AC_BUTT];
}dmac_best_rate_stat_info_stru;
#endif

#define DMAC_QUERY_EVENT_LEN  (48)   /*消息内容的长度 */
typedef enum
{
    OAL_QUERY_STATION_INFO_EVENT      = 1,
    OAL_ATCMDSRV_DBB_NUM_INFO_EVENT   = 2,
    OAL_ATCMDSRV_FEM_PA_INFO_EVENT    = 3,
    OAL_ATCMDSRV_GET_RX_PKCG          = 4,
    OAL_ATCMDSRV_LTE_GPIO_CHECK       = 5,
    OAL_ATCMDSRV_GET_ANT              = 6,
#ifdef _PRE_WLAN_FEATURE_SMARTANT
    OAL_ATCMDSRV_GET_ANT_INFO         = 7,
    OAL_ATCMDSRV_DOUBLE_ANT_SW        = 8,
#endif
    OAL_QUERY_EVNET_BUTT
}oal_query_event_id_enum;

typedef struct
{
    oal_uint8        query_event;
    oal_uint8        auc_query_sta_addr[WLAN_MAC_ADDR_LEN];
}dmac_query_request_event;
typedef struct
{
    oal_uint8        query_event;
    oal_int8        reserve[DMAC_QUERY_EVENT_LEN];
}dmac_query_response_event;
typedef struct
{
    oal_uint32   ul_event_para;       /*消息传输的数据*/
    oal_uint32   ul_fail_num;
    oal_int16    s_always_rx_rssi;
    oal_uint8    uc_event_id;         /*消息号*/
    oal_uint8    uc_reserved;
}dmac_atcmdsrv_atcmd_response_event;

#ifdef _PRE_WLAN_FEATURE_SMARTANT
typedef struct
{
    oal_uint8       uc_event_id;                            /*消息号*/
    oal_uint8       uc_ant_type;                            //当前所用天线，0为WIFI主天线，1为分级天线
    oal_uint8       auc_rsv[2];
    oal_uint32      ul_last_ant_change_time_ms;                //上次切换天线时刻
    oal_uint32      ul_ant_change_number;                   //天线切换次数
    oal_uint32      ul_main_ant_time_s;                       //用在WIFI主天线时长
    oal_uint32      ul_aux_ant_time_s;                        //用在从天线(分级天线)时长
    oal_uint32      ul_total_time_s;                        //WIFI开启时长
}dmac_atcmdsrv_ant_info_response_event;
#endif

typedef struct
{
    oal_uint8        query_event;
    oal_uint8 auc_query_sta_addr[WLAN_MAC_ADDR_LEN];/*sta mac地址*/
}dmac_query_station_info_request_event;
typedef struct
{
    oal_uint8    query_event;        /* 消息号 */
    oal_int8     c_signal;           /* 信号强度 */
    oal_uint16   us_asoc_id;         /* Association ID of the STA */
    oal_uint32   ul_rx_packets;      /* total packets received */
    oal_uint32   ul_tx_packets;      /* total packets transmitted */
    oal_uint32   ul_rx_bytes;        /* total bytes received     */
    oal_uint32   ul_tx_bytes;        /* total bytes transmitted  */
    oal_uint32   ul_tx_retries;      /* 发送重传次数 */
    oal_uint32   ul_rx_dropped_misc; /* 接收失败次数 */
    oal_uint32   ul_tx_failed;       /* 发送失败次数 */
    oal_int16    s_free_power;       /* 底噪 */
    oal_uint16   s_chload;          /* 信道繁忙程度*/

    station_info_extend_stru    st_station_info_extend;
    mac_rate_info_stru st_txrate;    /*vap当前速率*/
}dmac_query_station_info_response_event;

typedef struct
{
    oal_uint8   uc_p2p0_hal_vap_id;
    oal_uint8   uc_p2p_gocl_hal_vap_id;
    oal_uint8   uc_rsv[2];
}mac_add_vap_sync_data_stru;

typedef struct
{
    oal_uint32  ul_cycles;              /* 统计间隔时钟周期数 */
    oal_uint32  ul_sw_tx_succ_num;     /* 软件统计发送成功ppdu个数 */
    oal_uint32  ul_sw_tx_fail_num;     /* 软件统计发送失败ppdu个数 */
    oal_uint32  ul_sw_rx_ampdu_succ_num;     /* 软件统计接收成功的ampdu个数 */
    oal_uint32  ul_sw_rx_mpdu_succ_num;      /* 软件统计接收成功的mpdu个数 */
    oal_uint32  ul_sw_rx_ppdu_fail_num;      /* 软件统计接收失败的ppdu个数 */
    oal_uint32  ul_hw_rx_ampdu_fcs_fail_num;   /* 硬件统计接收ampdu fcs校验失败个数 */
    oal_uint32  ul_hw_rx_mpdu_fcs_fail_num;    /* 硬件统计接收mpdu fcs校验失败个数 */
}dmac_thruput_info_sync_stru;

typedef struct
{
    oal_uint32                   uc_dscr_status;
    oal_uint8                   mgmt_frame_id;
    oal_uint8                   auc_resv[1];
    oal_uint16                  us_user_idx;
}dmac_crx_mgmt_tx_status_stru;

#ifdef _PRE_WLAN_FEATURE_M2S
typedef struct
{
    wlan_m2s_mgr_vap_stru                 ast_m2s_comp_vap[WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE];
    oal_bool_enum_uint8                   en_m2s_result;
    oal_uint8                             uc_m2s_mode;                          /* 当前切换业务 */
    oal_uint8                             uc_m2s_state;                         /* 当前m2s状态 */
    oal_uint8                             uc_vap_num;
}dmac_m2s_complete_syn_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
typedef struct
{
    oal_int32                       l_core_idx;
    oal_int32                       l_rssi;
    oal_int32                       aul_rsv[4];
}dmac_tas_rssi_notify_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
typedef struct
{
    oal_uint32           ul_chan_report_for_te_a;  /* Channel Bit Map to register TE-A */
    oal_bool_enum_uint8  en_te_b;                  /* 20/40M intolerant for TE-B */
}dmac_obss_te_a_b_stru;
#endif


#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
typedef enum
{
    DMAC_CONFIG_IPV4 = 0,                /* 配置IPv4地址 */
    DMAC_CONFIG_IPV6,                    /* 配置IPv6地址 */
    DMAC_CONFIG_BUTT
}dmac_ip_type;
typedef oal_uint8 dmac_ip_type_enum_uint8;

typedef enum
{
    DMAC_IP_ADDR_ADD = 0,                /* 增加IP地址 */
    DMAC_IP_ADDR_DEL,                    /* 删除IP地址 */
    DMAC_IP_OPER_BUTT
}dmac_ip_oper;
typedef oal_uint8 dmac_ip_oper_enum_uint8;

typedef struct
{
    dmac_ip_type_enum_uint8           en_type;
    dmac_ip_oper_enum_uint8           en_oper;

    oal_uint8                         auc_resv[2];

    oal_uint8                         auc_ip_addr[OAL_IP_ADDR_MAX_SIZE];
    oal_uint8                         auc_mask_addr[OAL_IP_ADDR_MAX_SIZE];
}dmac_ip_addr_config_stru;
#endif


/* 函数执行控制枚举 */
typedef enum
{
    DMAC_RX_FRAME_CTRL_GOON,        /* 本数据帧需要继续处理 */
    DMAC_RX_FRAME_CTRL_DROP,        /* 本数据帧需要丢弃 */
    DMAC_RX_FRAME_CTRL_BA_BUF,      /* 本数据帧被BA会话缓存 */

    DMAC_RX_FRAME_CTRL_BUTT
}dmac_rx_frame_ctrl_enum;
typedef oal_uint8 dmac_rx_frame_ctrl_enum_uint8;

#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
typedef enum
{
    DMAC_RX_SAMPLE,           /* DMAC收到数采请求 */
    DMAC_RX_PSD,              /* DMAC收到PSD请求 */
    DMAC_RX_BUTT
}dmac_rx_sdt_sample_enum;
typedef oal_uint8 dmac_rx_sdt_sample_enum_uint8;
typedef struct
{
    oal_uint32      ul_type;
    oal_uint32      ul_reg_num;
    oal_uint32      ul_count;
    oal_uint32      ul_event_data;
}dmac_sdt_sample_frame_stru;
#endif
#ifdef _PRE_WLAN_RF_AUTOCALI
typedef enum
{
    AUTOCALI_DATA,            /*校准数据上报消息*/
    AUTOCALI_ACK,             /*校准自动化同步消息*/
    AUTOCALI_SWITCH,          /*校准自动化开关消息*/
}dmac_sdt_autocali_msg_type;
typedef struct
{
    oal_uint32      ul_type;
    oal_uint8       auc_msg_data[4];
}dmac_sdt_autocali_frame_stru;
#endif

#ifdef _PRE_WLAN_11K_STAT
#define DMAC_STAT_TX_DELAY_HIST_BIN_NUM                  6
/*dot11MACStatistics Group*/
typedef struct
{
    oal_uint32              ul_tx_retry_succ_msdu_num;          /*一次或多次重传成功的MSDU个数*/
    oal_uint32              ul_tx_multi_retry_succ_msud_num;    /*多次重传成功的MSDU个数 */
    oal_uint32              ul_rx_dup_frm_num;                  /*过滤暂不统计*/
    oal_uint32              ul_rts_suc_num;                     /*RTS发送成功个数*/
    oal_uint32              ul_rts_fail_num;                    /*RTS发送失败个数*/
    oal_uint32              ul_ack_fail_mpdu_num;               /*未收到ACK的MPDU个数*/
}dmac_stat_mac_stat_stru;

/* user statistics */
typedef struct
{
    oal_uint32              ul_tx_frag_mpdu_num;                /*发送成功的分片MPDU个数*/
    oal_uint32              ul_tx_fail_msdu_num;                /*发送次数超出限制的失败MSDU个数*/
    dmac_stat_mac_stat_stru st_stat_mac_stat;
    oal_uint32              ul_rx_frag_mpdu_num;                /*接收分片的MPDU个数*/
    oal_uint32              ul_tx_succ_msdu_num;                /*发送成功的MSDU个数*/
    oal_uint32              ul_tx_discard_msdu_num;            /*丢弃的MSDU个数*/
    oal_uint32              ul_rx_mpdu_num;                    /*接收的MPDU个数*/
    oal_uint32              ul_rx_retry_mpdu_num;              /*接收重传的MPDU个数*/
    oal_uint32              ul_rx_fail_num;
    oal_uint32              ul_tx_succ_bytes;                  /*tx succ字节*/
    oal_uint32              ul_rx_succ_bytes;                  /*rx succ字节*/
    oal_uint32              ul_tx_fail_bytes;                  /*tx fail字节*/
    oal_uint32              ul_rx_fail_bytes;                  /*rx fail字节*/
    oal_uint32              ul_forward_num;
    oal_uint32              ul_forward_bytes;                  /*forward字节*/
}dmac_stat_count_tid_stru;

/*dot11Counters Group*/
typedef struct
{
    oal_uint32              ul_tx_frag_mpdu_num;                /*发送成功的分片MPDU个数*/
    oal_uint32              ul_tx_multicast_msdu_num;           /*发送成功的组播MSDU个数*/
    oal_uint32              ul_tx_fail_msdu_num;                /*发送次数超出限制的失败MSDU个数*/
    oal_uint32              ul_rx_frag_mpdu_num;                /*接收分片的MPDU个数*/
    oal_uint32              ul_rx_msdu_num;                     /*接收的MPDU个数*/
    oal_uint32              ul_rx_multicast_msdu_num;           /*组播接收的MPDU个数*/
    oal_uint32              ul_fcs_err_mpdu_num;                /*过滤暂不统计*/
    oal_uint32              ul_tx_succ_msdu_num;                /*发送成功的MSDU个数*/
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    oal_uint32              ul_tx_mcast_bytes;                  /* 发送多播报文字节数:  */
    oal_uint32              ul_rx_mcast_bytes;                  /* 接收多播报文字节数:  */
    oal_uint32              aul_sta_tx_mcs_cnt[16];
    oal_uint32              aul_sta_rx_mcs_cnt[16];
#endif
}dmac_stat_count_mpdu_stru;

typedef struct
{
    oal_uint32                  ul_tx_frag_mpdu_num;                /*发送成功的分片MPDU个数*/
    oal_uint32                  ul_tx_multicast_mpdu_num;           /*发送成功的组播MSDU个数*/
    oal_uint32                  ul_tx_fail_mpdu_num;                /*发送次数超出限制的失败MSDU个数*/
    oal_uint32                  ul_rx_frag_mpdu_num;                /*接收分片的MPDU个数*/
    oal_uint32                  ul_rx_multicast_mpdu_num;           /*接收的MPDU个数*/
    oal_uint32                  ul_fcs_err_mpdu_num;                /*过滤暂不统计*/
    oal_uint32                  ul_tx_succ_mpdu_num;                /*发送成功的MSDU个数*/
    oal_uint32                  ul_tx_retry_succ_mpdu_num;          /*一次或多次重传成功的MSDU个数*/
    oal_uint32                  ul_tx_multi_retry_succ_mpdu_num;    /*多次重传成功的MSDU个数 */
    oal_uint32                  ul_rx_dup_frm_num;                  /*过滤暂不统计*/
    oal_uint32                  ul_rts_suc_num;                     /*RTS发送成功个数*/
    oal_uint32                  ul_rts_fail_num;                    /*RTS发送失败个数*/
    oal_uint32                  ul_ack_fail_mpdu_num;               /*未收到ACK的MPDU个数*/

    oal_uint32                  ul_tx_discard_mpdu_num;            /*丢弃的MSDU个数*/
    oal_uint32                  ul_rx_mpdu_num;                    /*接收的MPDU个数*/
    oal_uint32                  ul_rx_retry_mpdu_num;              /*接收重传的MPDU个数*/
}dmac_stat_count_common_stru;

/*dot11CountersGroup3 (A-MSDU):*/
typedef struct
{
    oal_uint32              ul_tx_succ_num;                         /*发送成功的AMSDU个数*/
    oal_uint32              ul_tx_fail_num;                         /*发送次数超出限制的失败AMSDU个数*/
    oal_uint32              ul_tx_retry_succ_num;                   /*一次或多次重传成功的AMSDU个数*/
    oal_uint32              ul_tx_multi_retry_succ_num;             /*多次重传成功的AMSDU个数 */
    oal_uint64              ull_tx_succ_octets_num;                 /*发送成功的AMSDU帧体的octets个数*/
    oal_uint32              ul_ack_fail_num;                        /*未收到ACK的AMSUD个数*/
    oal_uint32              ul_rx_num;                              /*接收的AMSUD个数*/
    oal_uint64              ull_rx_octets_num;                       /*接收的AMSUD帧体的octets个数*/
}dmac_stat_count_amsdu_stru;

/*dot11CountersGroup3 (A-MPDU)*/
typedef struct
{
    oal_uint32              ul_tx_num;                              /*发送的AMPDU个数*/
    oal_uint32              ul_tx_mpdu_num;                         /*发送的AMPDU中MDPU个数*/
    oal_uint64              ull_tx_octets_num;                      /*发送的AMPDU帧体的octets个数*/
    oal_uint32              ul_rx_num;                              /*接收的AMPDU个数*/
    oal_uint32              ul_rx_mpdu_num;                         /*接收的AMPDU中MPDU个数*/
    oal_uint64              ull_rx_octets_num;                      /*接收的AMPDU帧体的octets个数*/
    //oal_uint32              ul_rx_mpdu_delimiter_crc_err;         /*MPDU分隔符错误， mac不会上报*/
}dmac_stat_count_ampdu_stru;

/*device, vap, use statistic structure*/
typedef struct
{
    dmac_stat_count_mpdu_stru           st_count_mpdu;
    dmac_stat_mac_stat_stru             st_mac_stat;
    dmac_stat_count_amsdu_stru          st_count_amsdu;
    dmac_stat_count_ampdu_stru          st_count_ampdu;

    oal_uint32                          aul_tx_dropped[WLAN_TIDNO_BUTT];
}dmac_stat_count_stru;

/*tid queue delay*/
typedef struct
{
    oal_uint64              ull_queue_delay_sum;                 /*TID队列等待时间累加值*/
    oal_uint32              ul_queue_delay_cnt;                 /*TID dely的统计个数*/
}dmac_stat_tid_queue_delay_stru;

/*tid tx delay*/
typedef struct
{
    oal_uint64              ull_tx_delay_sum;                    /*发送时延:TID入队到上报发送完成中断的时间累加值*/
    oal_uint32              ul_tx_delay_cnt;                    /*tx delay的统计个数*/
    oal_uint32              ul_max_tx_delay;                    /*最大发送延时*/
    oal_uint32              ul_min_tx_delay;                    /*最小发送延时*/
}dmac_stat_tid_tx_delay_stru;

/*tid tx delay hist*/
typedef struct
{
    oal_uint32              auc_tx_delay_hist_bin[DMAC_STAT_TX_DELAY_HIST_BIN_NUM];   /*delay直方图统计结果*/
    oal_uint8               uc_tx_delay_hist_flag;                                    /*是否统计delay直方图*/
    oal_uint8               uc_tidno;
    oal_uint16              aus_hist_range[DMAC_STAT_TX_DELAY_HIST_BIN_NUM];           /*delay直方图统计范围*/
}dmac_stat_tid_tx_delay_hist_stru;

/*frame report*/
typedef struct
{
    oal_uint32                              ul_sum_rcpi;
    oal_uint8                               uc_last_rcpi;
    oal_uint8                               uc_avrg_rcpi;
    oal_uint8                               uc_last_rsni;
    oal_uint8                               auc_res[1];
    oal_uint32                              ul_rx_mag_data_frm_num;
}dmac_stat_frm_rpt_stru;

/*tsc report*/
typedef struct
{
    dmac_stat_tid_queue_delay_stru          ast_tid_queue_delay[WLAN_TIDNO_BUTT];
    dmac_stat_tid_tx_delay_hist_stru        st_tid_tx_delay_hist;
}dmac_stat_tsc_rpt_stru;

typedef struct
{
    oal_uint32  bit_enable                         : 1,
                bit_count                          : 1,
                bit_tid_count                      : 1,
                bit_tid_tx_delay                   : 1,
                bit_tsc_rpt                        : 1,
                bit_frm_rpt                        : 1,
                bit_user_tid_count                 : 1,
                bit_resv                           : 25;
}dmac_stat_cap_flag_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_APF
typedef struct
{
    oal_void         *p_program;         /* netbuf链表一个元素 */
    oal_uint16        us_program_len;
}dmac_apf_report_event_stru;
#endif

#ifdef _PRE_WLAN_FEATURE_GNSS_SCAN
#pragma pack(1)
/* scan parameter */
typedef struct
{
    oal_uint8                           uc_chan_number;     /* 主20MHz信道号 */
    wlan_channel_band_enum_uint8        en_band;            /* 频段 */
}gnss_scan_channel_stru;

typedef struct
{
    oal_uint8              uc_ch_valid_num;
    gnss_scan_channel_stru ast_wlan_channel[WLAN_MAX_CHANNEL_NUM];
} dmac_gscan_params_stru;

/* scan results */
typedef struct
{
    oal_uint8   auc_bssid[WLAN_MAC_ADDR_LEN];   /* 网络bssid */
    oal_uint8   uc_channel_num;
    oal_int8    c_rssi;                       /* bss的信号强度 */
    oal_uint8   uc_serving_flag;
    oal_uint8   uc_rtt_unit;
    oal_uint8   uc_rtt_acc;
    oal_uint32  ul_rtt_value;
}wlan_ap_measurement_info_stru;

/* Change Feature: 上报给GNSS的扫描结果结构体不同于DMAC保存的扫描结果结构体 */
typedef struct
{
    oal_uint8   auc_bssid[WLAN_MAC_ADDR_LEN];   /* 网络bssid */
    oal_uint8   uc_channel_num;
    oal_int8    c_rssi;                       /* bss的信号强度 */
}wlan_ap_report_info_stru;

/* 上报给gnss的扫描结果结构体 */
typedef struct
{
    oal_uint32                     ul_interval_from_last_scan;
    oal_uint8                      uc_ap_valid_number;
    wlan_ap_report_info_stru       ast_wlan_ap_measurement_info[DMAC_SCAN_MAX_AP_NUM_TO_GNSS];
}dmac_gscan_report_info_stru;
#pragma pack()

extern dmac_gscan_params_stru         g_st_gnss_scan_params;
extern dmac_gscan_report_info_stru    g_st_gnss_scan_result_info;

typedef struct
{
    oal_dlist_head_stru            st_entry;                    /* 链表指针 */
    wlan_ap_measurement_info_stru  st_wlan_ap_measurement_info; /*上报gnss的扫描信息 */
}dmac_scanned_bss_info_stru;

typedef struct
{
    oal_uint32                     ul_scan_end_timstamps;/* 记录此次扫描的时间戳,一次扫描记录一次,不按扫到的ap分别记录 */
    oal_dlist_head_stru            st_dmac_scan_info_list;
    oal_dlist_head_stru            st_scan_info_res_list;  /* 扫描信息存储资源链表 */
    dmac_scanned_bss_info_stru     ast_scan_bss_info_member[DMAC_SCAN_MAX_AP_NUM_TO_GNSS];
}dmac_scan_for_gnss_stru;
#endif

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
#define HMAC_TO_DMAC_CFG_MSG_HDR_LEN  (OAL_SIZEOF(hmac_to_dmac_cfg_msg_stru) - 4)   /* 只取头的长度，去掉4字节auc_msg_body长度 */

/* 1us精度 */
#define DMAC_TIME_USEC_INT64(_pst_time) \
    ((oal_int64)((_pst_time)->i_sec) * 1000000 + (oal_int64)((_pst_time)->i_usec))

/* 不区分offload架构的CB字段 */
#define MAC_GET_CB_IS_4ADDRESS(_pst_tx_ctrl)             ((_pst_tx_ctrl)->en_use_4_addr)
#define MAC_GET_CB_IS_AMSDU(_pst_tx_ctrl)                ((_pst_tx_ctrl)->bit_is_amsdu)
#define MAC_GET_CB_IS_LARGE_SKB_AMSDU(_pst_tx_ctrl)      ((_pst_tx_ctrl)->bit_is_large_skb_amsdu)
#define MAC_GET_CB_HAS_EHTER_HEAD(_pst_tx_ctrl)          ((_pst_tx_ctrl)->bit_ether_head_including)
#define MAC_GET_CB_IS_FIRST_MSDU(_pst_tx_ctrl)           ((_pst_tx_ctrl)->bit_is_first_msdu)
#define MAC_GET_CB_IS_NEED_RESP(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_need_rsp)
#define MAC_GET_CB_IS_EAPOL_KEY_PTK(_pst_tx_ctrl)        ((_pst_tx_ctrl)->bit_is_eapol_key_ptk)
#define MAC_GET_CB_IS_ROAM_DATA(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_roam_data)
#define MAC_GET_CB_IS_FROM_PS_QUEUE(_pst_tx_ctrl)        ((_pst_tx_ctrl)->bit_is_get_from_ps_queue)
#define MAC_GET_CB_IS_MCAST(_pst_tx_ctrl)                ((_pst_tx_ctrl)->bit_ismcast)
#define MAC_GET_CB_IS_NEEDRETRY(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_is_needretry)
#define MAC_GET_CB_IS_PROBE_DATA(_pst_tx_ctrl)           ((_pst_tx_ctrl)->en_is_probe_data)
#define MAC_GET_CB_ALG_TAGS(_pst_tx_ctrl)                ((_pst_tx_ctrl)->uc_alg_frame_tag)

#define MAC_GET_CB_MGMT_FRAME_ID(_pst_tx_ctrl)           ((_pst_tx_ctrl)->bit_mgmt_frame_id)
#define MAC_GET_CB_MPDU_LEN(_pst_tx_ctrl)                ((_pst_tx_ctrl)->us_mpdu_payload_len)
#define MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)              ((_pst_tx_ctrl)->uc_frame_type)
#define MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)           ((_pst_tx_ctrl)->uc_frame_subtype)
#define MAC_GET_CB_TIMESTAMP(_pst_tx_ctrl)               ((_pst_tx_ctrl)->ul_timestamp_us)

/* VIP数据帧 */
#define MAC_GET_CB_IS_VIPFRAME(_pst_tx_ctrl)    \
         ((WLAN_CB_FRAME_TYPE_DATA == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) &&    \
          (MAC_DATA_VIP_FRAME >= MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))

#define MAC_GET_CB_IS_SMPS_FRAME(_pst_tx_ctrl)   \
        ((WLAN_CB_FRAME_TYPE_ACTION == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) &&    \
         (WLAN_ACTION_SMPS_FRAME_SUBTYPE == MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))
#define MAC_GET_CB_IS_OPMODE_FRAME(_pst_tx_ctrl)  \
        ((WLAN_CB_FRAME_TYPE_ACTION == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) &&    \
         (WLAN_ACTION_OPMODE_FRAME_SUBTYPE == MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))
#define MAC_GET_CB_IS_P2PGO_FRAME(_pst_tx_ctrl)  \
                ((WLAN_CB_FRAME_TYPE_MGMT == MAC_GET_CB_FRAME_TYPE(_pst_tx_ctrl)) &&    \
                 (WLAN_ACTION_P2PGO_FRAME_SUBTYPE == MAC_GET_CB_FRAME_SUBTYPE(_pst_tx_ctrl)))

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/* 模块发送流程控制信息结构体的信息元素获取 */
#define MAC_GET_CB_MPDU_NUM(_pst_tx_ctrl)                ((_pst_tx_ctrl)->bit_mpdu_num)
#define MAC_GET_CB_NETBUF_NUM(_pst_tx_ctrl)              ((_pst_tx_ctrl)->bit_netbuf_num)
#define MAC_GET_CB_FRAME_HEADER_LENGTH(_pst_tx_ctrl)     ((_pst_tx_ctrl)->bit_frame_header_length)
#define MAC_GET_CB_ACK_POLACY(_pst_tx_ctrl)              ((_pst_tx_ctrl)->en_ack_policy)
#define MAC_GET_CB_TX_VAP_INDEX(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_tx_vap_index)
#define MAC_GET_CB_TX_USER_IDX(_pst_tx_ctrl)             ((_pst_tx_ctrl)->bit_tx_user_idx)
#define MAC_GET_CB_WME_AC_TYPE(_pst_tx_ctrl)             ((_pst_tx_ctrl)->bit_ac)
#define MAC_GET_CB_WME_TID_TYPE(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_tid)
#define MAC_GET_CB_EVENT_TYPE(_pst_tx_ctrl)              ((_pst_tx_ctrl)->bit_event_type)
#define MAC_GET_CB_EVENT_SUBTYPE(_pst_tx_ctrl)           ((_pst_tx_ctrl)->bit_event_sub_type)
#define MAC_GET_CB_RETRIED_NUM(_pst_tx_ctrl)             ((_pst_tx_ctrl)->bit_retried_num)
#define MAC_GET_CB_ALG_PKTNO(_pst_tx_ctrl)               ((_pst_tx_ctrl)->uc_alg_pktno)
#define MAC_GET_CB_TCP_ACK(_pst_tx_ctrl)                 ((_pst_tx_ctrl)->bit_is_tcp_ack)

#define MAC_GET_CB_IS_DATA_FRAME(_pst_tx_ctrl)       \
        ((WLAN_DATA_BASICTYPE == MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)) &&    \
         ((WLAN_DATA == MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)) ||       \
          (WLAN_QOS_DATA == MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl))))

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#define MAC_GET_CB_SEQ_NUM(_pst_tx_ctrl)                 (((mac_ieee80211_frame_stru *)((oal_uint8 *)_pst_tx_ctrl + OAL_MAX_CB_LEN))->bit_seq_num)
#define MAC_SET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl, _flag)
#define MAC_GET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl)      1 /* offload架构,MAC HEAD由netbuf index管理,不需要释放 */

#define MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)     \
        (((mac_ieee80211_frame_stru *)((oal_uint8 *)(_pst_tx_ctrl) + OAL_MAX_CB_LEN))->st_frame_control.bit_type)
#define MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)   \
        (((mac_ieee80211_frame_stru *)((oal_uint8 *)(_pst_tx_ctrl) + OAL_MAX_CB_LEN))->st_frame_control.bit_sub_type)


#define MAC_SET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl, _addr)
#define MAC_GET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl)       ((mac_ieee80211_frame_stru *)((oal_uint8 *)_pst_tx_ctrl + OAL_MAX_CB_LEN))

#define MAC_SET_CB_IS_QOS_DATA(_pst_tx_ctrl, _flag)
#define MAC_GET_CB_IS_QOS_DATA(_pst_tx_ctrl)         \
        ((WLAN_DATA_BASICTYPE == MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)) &&    \
         ((WLAN_QOS_DATA == MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)) ||       \
          (WLAN_QOS_NULL_FRAME == MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl))))

#define MAC_SET_CB_IS_BAR(_pst_tx_ctrl, _flag)
#define MAC_GET_CB_IS_BAR(_pst_tx_ctrl)           \
        ((WLAN_CONTROL == MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)) &&    \
         (WLAN_BLOCKACK_REQ == MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)))

/* OFFLOAD 架构不需要 */
#define MAC_SET_CB_BAR_DSCR_ADDR(_pst_tx_ctrl, _addr)
#define MAC_GET_CB_BAR_DSCR_ADDR(_pst_tx_ctrl)           OAL_PTR_NULL
#else
#define MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)                ((_pst_tx_ctrl)->st_expand_cb.en_frame_type)
#define MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)             (((_pst_tx_ctrl)->pst_frame_header)->st_frame_control.bit_sub_type)
#define MAC_GET_CB_SEQ_NUM(_pst_tx_ctrl)                        ((_pst_tx_ctrl)->st_expand_cb.us_seqnum)
#define MAC_SET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl, _addr)       ((_pst_tx_ctrl)->st_expand_cb.pst_frame_header = _addr)
#define MAC_GET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl)              ((_pst_tx_ctrl)->st_expand_cb.pst_frame_header)
#define MAC_SET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl, _flag)     ((_pst_tx_ctrl)->st_expand_cb.bit_80211_mac_head_type = _flag)
#define MAC_GET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl)            ((_pst_tx_ctrl)->st_expand_cb.bit_80211_mac_head_type)
#define MAC_SET_CB_IS_QOS_DATA(_pst_tx_ctrl, _flag)
#define MAC_GET_CB_IS_QOS_DATA(_pst_tx_ctrl)             OAL_FALSE
#endif //#if defined(_PRE_PRODUCT_ID_HI110X_DEV)


#else
/* 发送控制字段获取 */
#define MAC_GET_CB_MPDU_NUM(_pst_tx_ctrl)                       ((_pst_tx_ctrl)->uc_mpdu_num)
#define MAC_GET_CB_NETBUF_NUM(_pst_tx_ctrl)                     ((_pst_tx_ctrl)->uc_netbuf_num)
#define MAC_GET_CB_FRAME_HEADER_LENGTH(_pst_tx_ctrl)            ((_pst_tx_ctrl)->uc_frame_header_length)
#define MAC_GET_CB_ACK_POLACY(_pst_tx_ctrl)                     ((_pst_tx_ctrl)->en_ack_policy)
#define MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl)                ((_pst_tx_ctrl)->en_frame_type)
#define MAC_GET_CB_WLAN_FRAME_SUBTYPE(_pst_tx_ctrl)             (((_pst_tx_ctrl)->pst_frame_header)->st_frame_control.bit_sub_type)
#define MAC_GET_CB_TX_VAP_INDEX(_pst_tx_ctrl)                   ((_pst_tx_ctrl)->uc_tx_vap_index)
#define MAC_GET_CB_TX_USER_IDX(_pst_tx_ctrl)                    ((_pst_tx_ctrl)->us_tx_user_idx)
#define MAC_GET_CB_WME_AC_TYPE(_pst_tx_ctrl)                    ((_pst_tx_ctrl)->uc_ac)
#define MAC_GET_CB_WME_TID_TYPE(_pst_tx_ctrl)                   ((_pst_tx_ctrl)->uc_tid)
#define MAC_GET_CB_EVENT_TYPE(_pst_tx_ctrl)                     ((_pst_tx_ctrl)->en_event_type)
#define MAC_GET_CB_EVENT_SUBTYPE(_pst_tx_ctrl)                  ((_pst_tx_ctrl)->uc_event_sub_type)
#define MAC_GET_CB_RETRIED_NUM(_pst_tx_ctrl)                    ((_pst_tx_ctrl)->uc_retried_num)
#define MAC_SET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl, _flag)     ((_pst_tx_ctrl)->bit_80211_mac_head_type = _flag)
#define MAC_GET_CB_80211_MAC_HEAD_TYPE(_pst_tx_ctrl)            ((_pst_tx_ctrl)->bit_80211_mac_head_type)
#define MAC_GET_CB_SEQ_CTRL_BYPASS(_pst_tx_ctrl)                ((_pst_tx_ctrl)->en_seq_ctrl_bypass)
#define MAC_SET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl, _addr)       ((_pst_tx_ctrl)->pst_frame_header = _addr)
#define MAC_GET_CB_FRAME_HEADER_ADDR(_pst_tx_ctrl)              ((_pst_tx_ctrl)->pst_frame_header)
#define MAC_GET_CB_BAR_DSCR_ADDR(_pst_tx_ctrl)                  ((_pst_tx_ctrl)->pst_bar_dscr)
#define MAC_SET_CB_BAR_DSCR_ADDR(_pst_tx_ctrl, _addr)           ((_pst_tx_ctrl)->pst_bar_dscr = _addr)
#define MAC_GET_CB_SEQ_NUM(_pst_tx_ctrl)                        ((_pst_tx_ctrl)->us_seqnum)
#define MAC_GET_CB_ALG_PKTNO(_pst_tx_ctrl)                      ((_pst_tx_ctrl)->ul_alg_pktno)

#define MAC_SET_CB_IS_BAR(_pst_tx_ctrl, _flag)                  ((_pst_tx_ctrl)->en_is_bar = _flag)
#define MAC_GET_CB_IS_BAR(_pst_tx_ctrl)                         ((_pst_tx_ctrl)->en_is_bar)
#define MAC_SET_CB_IS_QOS_DATA(_pst_tx_ctrl, _flag)             ((_pst_tx_ctrl)->en_is_qosdata = _flag)
#define MAC_GET_CB_IS_QOS_DATA(_pst_tx_ctrl)                    ((_pst_tx_ctrl)->en_is_qosdata)
#define MAC_GET_CB_IS_DATA_FRAME(_pst_tx_ctrl)                  (WLAN_DATA_BASICTYPE == MAC_GET_CB_WLAN_FRAME_TYPE(_pst_tx_ctrl))
#define MAC_GET_CB_IS_MORE_FRAGMENTS(_pst_tx_ctrl)              (((_pst_tx_ctrl)->pst_frame_header)->st_frame_control.bit_more_frag)
#endif //#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)


#define MAC_GET_RX_CB_FRAME_LEN(_pst_rx_ctl)                    ((_pst_rx_ctl)->us_frame_len)
#define MAC_GET_RX_CB_MAC_HEADER_LEN(_pst_rx_ctl)               ((_pst_rx_ctl)->uc_mac_header_len)
#define MAC_GET_RX_CB_MAC_VAP_ID(_pst_rx_ctl)                   ((_pst_rx_ctl)->uc_mac_vap_id)
#define MAC_GET_RX_CB_HAL_VAP_IDX(_pst_rx_ctl)                  ((_pst_rx_ctl)->bit_vap_id)

/* DMAC模块接收流程控制信息结构体的信息元素获取 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#define MAC_GET_RX_CB_TA_USER_IDX(_pst_rx_ctl)                  ((_pst_rx_ctl)->bit_ta_user_idx)
#define MAC_GET_RX_CB_PAYLOAD_LEN(_pst_rx_ctl)                  ((_pst_rx_ctl)->us_frame_len - (_pst_rx_ctl)->uc_mac_header_len)
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#define MAC_GET_RX_PAYLOAD_ADDR(_pst_rx_ctl,_pst_buf)           (OAL_NETBUF_PAYLOAD(_pst_buf))
#else
#define MAC_GET_RX_CB_MAC_HEADER_ADDR(_pst_rx_ctl)              ((_pst_rx_ctl)->st_expand_cb.pul_mac_hdr_start_addr)
#define MAC_GET_RX_CB_DA_USER_IDX(_pst_rx_ctl)                  ((_pst_rx_ctl)->st_expand_cb.us_da_user_idx)
#define MAC_GET_RX_PAYLOAD_ADDR(_pst_rx_ctl, _pst_buf) \
            ((oal_uint8 *)(mac_get_rx_cb_mac_hdr(_pst_rx_ctl)) + MAC_GET_RX_CB_MAC_HEADER_LEN(_pst_rx_ctl))
#endif

#else
#define MAC_GET_RX_CB_TA_USER_IDX(_pst_rx_ctl)                  ((_pst_rx_ctl)->us_ta_user_idx)
#define MAC_GET_RX_CB_MAC_HEADER_ADDR(_pst_rx_ctl)              ((_pst_rx_ctl)->pul_mac_hdr_start_addr)
#define MAC_GET_RX_CB_DA_USER_IDX(_pst_rx_ctl)                  ((_pst_rx_ctl)->us_da_user_idx)
#define MAC_GET_RX_CB_PAYLOAD_LEN(_pst_rx_ctl)                  ((_pst_rx_ctl)->us_frame_len - (_pst_rx_ctl)->uc_mac_header_len)
#define MAC_GET_RX_PAYLOAD_ADDR(_pst_rx_ctl, _pst_buf) \
            ((oal_uint8 *)(mac_get_rx_cb_mac_hdr(_pst_rx_ctl)) + MAC_GET_RX_CB_MAC_HEADER_LEN(_pst_rx_ctl))
#endif





OAL_STATIC OAL_INLINE oal_void  dmac_board_get_instance(mac_board_stru **ppst_dmac_board)
{
    *ppst_dmac_board = g_pst_mac_board;
}


OAL_STATIC OAL_INLINE oal_uint32  dmac_rx_free_netbuf_list(
                oal_netbuf_head_stru       *pst_netbuf_hdr,
                oal_netbuf_stru           **pst_netbuf,
                oal_uint16                  us_nums)
{
    oal_netbuf_stru    *pst_netbuf_temp;
    oal_uint16          us_netbuf_num;

#if defined(_PRE_MEM_DEBUG_MODE) && (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
#ifdef SW_DEBUG
    oal_uint32 ul_return_addr      = __return_address();
#endif
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_netbuf_hdr) || (OAL_PTR_NULL == pst_netbuf)))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (us_netbuf_num = 0; us_netbuf_num < us_nums; us_netbuf_num++)
    {
        pst_netbuf_temp = oal_get_netbuf_next(*pst_netbuf);

        oal_netbuf_delete(*pst_netbuf, pst_netbuf_hdr);

        if(OAL_ERR_CODE_OAL_MEM_ALREADY_FREE==oal_netbuf_free(*pst_netbuf))
        {
            #if defined(_PRE_MEM_DEBUG_MODE) && (_PRE_OS_VERSION_RAW == _PRE_OS_VERSION)
            #ifdef SW_DEBUG
                OAL_IO_PRINT("double free caller[%x]!\r\n",ul_return_addr);
            #endif
            #endif
        }

        *pst_netbuf = pst_netbuf_temp;
        if (pst_netbuf_hdr ==  (oal_netbuf_head_stru*)(*pst_netbuf))
        {
            break;
        }
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
OAL_STATIC OAL_INLINE oal_uint8 mac_get_cb_is_seq_ctrl_bypass(mac_tx_ctl_stru *pst_cb)
{
    oal_uint8                   uc_frame_type = 0;           /* 802.11头 */

    uc_frame_type = (oal_uint8)((MAC_GET_CB_FRAME_HEADER_ADDR(pst_cb))->st_frame_control.bit_type);

    /* 更新非Qos数据帧的seq_ctl_hw_bypass 标志 */
    if ((OAL_TRUE != MAC_GET_CB_IS_QOS_DATA(pst_cb)) && (WLAN_DATA_BASICTYPE == uc_frame_type))
    {
        return MAC_GET_CB_SEQ_CTRL_BYPASS(pst_cb);
    }

    return OAL_FALSE;
}
#endif


OAL_STATIC OAL_INLINE oal_void mac_set_rx_cb_mac_hdr(mac_rx_ctl_stru *pst_cb_ctrl, oal_uint32 *pul_mac_hdr_start_addr)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    return;
#else
    MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_cb_ctrl) = pul_mac_hdr_start_addr;

#endif
}

OAL_STATIC OAL_INLINE oal_uint32 *mac_get_rx_cb_mac_hdr(mac_rx_ctl_stru *pst_cb_ctrl)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    return (oal_uint32 *)((oal_uint8 *)pst_cb_ctrl + OAL_MAX_CB_LEN);
#else
    return MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_cb_ctrl);
#endif
}


OAL_STATIC OAL_INLINE oal_uint8* mac_netbuf_get_payload(oal_netbuf_stru *pst_netbuf)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    return OAL_NETBUF_PAYLOAD(pst_netbuf);
#else
    return OAL_NETBUF_PAYLOAD(pst_netbuf)+ MAC_80211_FRAME_LEN;
#endif
}

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void  dmac_alg_config_event_register(oal_uint32 p_func(frw_event_mem_stru *));
extern oal_void  dmac_alg_config_event_unregister(oal_void);
#ifdef _PRE_WLAN_PERFORM_STAT

/* 性能统计模块对外接口 */

typedef enum
{
    DMAC_STAT_TX      = 0,      /* 与DMAC_STAT_PER_MAC_TOTAL对应 */
    DMAC_STAT_RX      = 1,      /* 与DMAC_STAT_PER_BUFF_OVERFLOW对应 */
    DMAC_STAT_BOTH    = 2,      /* 与DMAC_STAT_PER_BUFF_BE_SEIZED对应 */

    DMAC_STAT_BUTT
}dmac_stat_direct;
typedef oal_uint8 dmac_stat_direct_enum_uint8;

typedef enum
{
    DMAC_STAT_PER_MAC_TOTAL             = 0,

    DMAC_STAT_PER_BUFF_OVERFLOW         = 1,
    DMAC_STAT_PER_BUFF_BE_SEIZED        = 2,
    DMAC_STAT_PER_DELAY_OVERTIME        = 3,
    DMAC_STAT_PER_SW_RETRY_AMPDU        = 4,
    DMAC_STAT_PER_SW_RETRY_SUB_AMPDU    = 5,
    DMAC_STAT_PER_SW_RETRY_MPDU         = 6,
    DMAC_STAT_PER_SW_RETRY_OVERFLOW     = 7,

    DMAC_STAT_PER_RTS_FAIL              = 8,
    DMAC_STAT_PER_HW_SW_FAIL            = 9,

    DMAC_STAT_PER_BUTT
}dmac_stat_per_reason;
typedef oal_uint8 dmac_stat_per_reason_enum_uint8;

typedef struct
{
    mac_stat_module_enum_uint16    en_module_id;                    /* 对应的模块id */
    mac_stat_type_enum_uint8       en_stat_type;                    /* 统计类型 */
    oal_uint8                      uc_resv[1];

    oal_void                      *p_void;                          /* tid,user或者vap指针 */
    oal_uint32                     aul_stat_avg[DMAC_STAT_PER_BUTT];    /* 某个周期的统计结果 */
}dmac_stat_param_stru;

/* 供内部模块调用的统计定时器到期处理函数指针 */
typedef oal_uint32  (*dmac_stat_timeout_func)(dmac_stat_param_stru *);

extern oal_uint32  dmac_stat_register(  mac_stat_module_enum_uint16     en_module_id,
                                        mac_stat_type_enum_uint8        en_stat_type,
                                        oal_void                       *p_void,
                                        dmac_stat_param_stru           *p_output_param,
                                        dmac_stat_timeout_func          p_func,
										oal_uint32						ul_core_id);

extern oal_uint32	dmac_stat_start( mac_stat_module_enum_uint16   en_module_id,
                                     mac_stat_type_enum_uint8      en_stat_type,
                                     oal_uint16                    us_stat_period,
                                     oal_uint16                    us_stat_num,
                                     oal_void                      *p_void);

extern oal_uint32	dmac_stat_stop(  mac_stat_module_enum_uint16  en_module_id,
                                     mac_stat_type_enum_uint8     en_stat_type,
                                     oal_void                    *p_void);

extern oal_uint32	dmac_stat_unregister(mac_stat_module_enum_uint16    en_module_id,
                                         mac_stat_type_enum_uint8       en_stat_type,
                                         oal_void                      *p_void);

#endif

#ifdef _PRE_DEBUG_MODE_USER_TRACK
extern oal_uint32  mac_user_check_txrx_protocol_change(
                                  mac_user_stru *pst_mac_user,
                                  oal_uint8      uc_present_mode,
                                  oam_user_info_change_type_enum_uint8  en_type);
#endif

#ifdef _PRE_WLAN_PERFORM_STAT
extern oal_uint32 dmac_stat_tid_per(mac_user_stru *pst_dmac_user,
                                    oal_uint8 uc_tidno,
                                    oal_uint16 us_mpdu_num,
                                    oal_uint16 us_err_mpdu_num,
                                    dmac_stat_per_reason_enum_uint8 en_per_reason);
#endif
extern oal_uint32  dmac_tid_pause(dmac_tid_stru *pst_tid, oal_uint8 uc_type);
extern oal_uint32  dmac_tid_resume(hal_to_dmac_device_stru *pst_hal_device, dmac_tid_stru *pst_tid, oal_uint8 uc_type);

extern oal_void mac_set_rx_cb_mac_hdr(mac_rx_ctl_stru *pst_cb_ctrl, oal_uint32 *pul_mac_hdr_start_addr);
extern oal_uint32 *mac_get_rx_cb_mac_hdr(mac_rx_ctl_stru *pst_cb_ctrl);

#if (_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION)
extern oal_uint32 dmac_init_event_process(frw_event_mem_stru *pst_event_mem);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32 dmac_cfg_vap_init_event(frw_event_mem_stru *pst_event_mem);
#endif
#endif
extern oal_uint32 dmac_tx_reinit_tx_queue(mac_device_stru *pst_mac_device, hal_to_dmac_device_stru *pst_hal_device);
extern oal_uint32  mac_vap_set_cb_tx_user_idx(mac_vap_stru *pst_mac_vap, mac_tx_ctl_stru *pst_tx_ctl, oal_uint8 *puc_data);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_ext_if.h */
