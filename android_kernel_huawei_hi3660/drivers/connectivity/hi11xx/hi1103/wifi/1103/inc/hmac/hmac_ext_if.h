

#ifndef __HMAC_EXT_IF_H__
#define __HMAC_EXT_IF_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_device.h"
#include "mac_vap.h"
#include "mac_user.h"
#include "mac_frame.h"
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#include "oal_hcc_host_if.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_EXT_IF_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/* 返回值类型定义 */
typedef enum
{
    HMAC_TX_PASS = 0,     /* 继续往下 */
    HMAC_TX_BUFF = 1,     /* 已被缓存 */
    HMAC_TX_DONE = 2,     /* 组播转成单播已发送 */

    HMAC_TX_DROP_PROXY_ARP    = 3, /* PROXY ARP检查后丢弃*/
    HMAC_TX_DROP_USER_UNKNOWN,     /* 未知user*/
    HMAC_TX_DROP_USER_NULL,        /* user结构体为NULL*/
    HMAC_TX_DROP_USER_INACTIVE,    /* 目的user未关联*/
    HMAC_TX_DROP_SECURITY_FILTER,  /* 安全检查过滤掉*/
    HMAC_TX_DROP_BA_SETUP_FAIL,    /* BA会话创建失败*/
    HMAC_TX_DROP_AMSDU_ENCAP_FAIL, /* amsdu封装失败*/
    HMAC_TX_DROP_AMSDU_BUILD_FAIL, /* amsdu组帧失败 */
    HMAC_TX_DROP_MUSER_NULL,       /*组播user为NULL */
    HMAC_TX_DROP_MTOU_FAIL,        /* 组播转单播失败*/
    HMAC_TX_DROP_80211_ENCAP_FAIL, /* 802.11 head封装失败*/
#ifdef _PRE_WLAN_FEATURE_CAR
    HMAC_TX_DROP_CAR_LIMIT,        /* 14 由于car限速导致丢包*/
#endif
#ifdef _PRE_WLAN_FEATURE_HERA_MCAST
    HMAC_TX_DROP_NOSMART,          /* 15 组播报文丢弃(非智能单品)*/
    HMAC_TX_DROP_NOADAP,           /* 16 组播报文丢弃(非配网模式)*/
#endif
    HMAC_TX_BUTT
}hmac_tx_return_type_enum;
typedef oal_uint8 hmac_tx_return_type_enum_uint8;


/*****************************************************************************
  枚举名  : hmac_host_ctx_event_sub_type_enum_uint8
  协议表格:
  枚举说明: HOST CTX事件子类型定义
*****************************************************************************/
typedef enum
{
    HMAC_HOST_CTX_EVENT_SUB_TYPE_SCAN_COMP_STA= 0,      /* STA　扫描完成子类型 */
    HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA,         /* STA 关联完成子类型 */
    HMAC_HOST_CTX_EVENT_SUB_TYPE_DISASOC_COMP_STA,      /* STA 上报去关联完成 */
    HMAC_HOST_CTX_EVENT_SUB_TYPE_STA_CONNECT_AP,        /* AP 上报新加入BSS的STA情况 */
    HMAC_HOST_CTX_EVENT_SUB_TYPE_STA_DISCONNECT_AP,      /* AP 上报离开BSS的STA情况 */
    HMAC_HOST_CTX_EVENT_SUB_TYPE_MIC_FAILURE,           /* 上报MIC攻击*/
    HMAC_HOST_CTX_EVENT_SUB_TYPE_ACS_RESPONSE,          /* 上报ACS命令执行结果 */
    HMAC_HOST_CTX_EVENT_SUB_TYPE_RX_MGMT,               /* 上报接收到的管理帧 */
    HMAC_HOST_CTX_EVENT_SUB_TYPE_LISTEN_EXPIRED,        /* 上报监听超时 */
#ifdef _PRE_WLAN_FEATURE_FLOWCTL
    HMAC_HOST_CTX_EVENT_SUB_TYPE_FLOWCTL_BACKP,         /* 上报流控反压消息 */
#endif
    HMAC_HOST_CTX_EVENT_SUB_TYPE_INIT,
    HMAC_HOST_CTX_EVENT_SUB_TYPE_MGMT_TX_STATUS,
#ifdef _PRE_WLAN_FEATURE_ROAM
    HMAC_HOST_CTX_EVENT_SUB_TYPE_ROAM_COMP_STA,         /* STA 漫游完成子类型 */
#endif  //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R
    HMAC_HOST_CTX_EVENT_SUB_TYPE_FT_EVENT_STA,         /* STA 漫游完成子类型 */
#endif //_PRE_WLAN_FEATURE_11R

#ifdef _PRE_WLAN_FEATURE_DFR
    HMAC_HOST_CTX_EVENT_SUB_TYPE_DEV_ERROR,         /* device异常处理流程 */
#endif //_PRE_WLAN_FEATURE_DFR
#ifdef _PRE_WLAN_FEATURE_VOWIFI
    HMAC_HOST_CTX_EVENT_SUB_TYPE_VOWIFI_REPORT,    /* 上报vowifi质量评估结果的切换请求 */
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
    HMAC_HOST_CTX_EVENT_SUB_TYPE_SAMPLE_REPORT,
#endif

#ifdef _PRE_WLAN_ONLINE_DPD
    HMAC_HOST_CTX_EVENT_SUB_TYPE_DPD,
#endif

#ifdef _PRE_WLAN_RF_AUTOCALI
    HMAC_HOST_CTX_EVENT_SUB_TYPE_AUTOCALI_REPORT,
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
    HMAC_HOST_CTX_EVENT_SUB_TYPE_CAC_REPORT,       /* 上报CAC事件*/
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK_TEMP_PROTECT
    HMAC_HOST_CTX_EVENT_SUB_TYPE_NOTIFY_STA_RSSI,
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    HMAC_HOST_CTX_EVENT_SUB_TYPE_M2S_STATUS,       /* 上报m2s事件*/
#endif

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    HMAC_HOST_CTX_EVENT_SUB_TYPE_TAS_NOTIFY_RSSI,  /* 上报TAS天线测量事件*/
#endif

    HMAC_HOST_CTX_EVENT_SUB_TYPE_BUTT
}hmac_host_ctx_event_sub_type_enum;
typedef oal_uint8 hmac_host_ctx_event_sub_type_enum_uint8;


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
#ifdef _PRE_WLAN_FEATURE_DFR
/* dfr相关功能信息 */
typedef struct
{
    oal_net_device_stru    *past_netdev[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT]; //p2p cl和dev共用一个业务vap,netdev的个数不会大于最大业务vap个数3
    oal_uint32             ul_netdev_num;
    oal_uint32             bit_hw_reset_enable              : 1,        /* 硬件不去关联复位开关 */
                           bit_device_reset_enable          : 1,        /* device挂死异常恢复开关 */
                           bit_soft_watchdog_enable         : 1,        /* 软狗功能开关 */
                           bit_device_reset_process_flag    : 1,        /* device挂死异常复位操作启动 */

                           bit_ready_to_recovery_flag  : 1,
                           bit_user_disconnect_flag    : 1,            /* device挂死异常，需要在dfr恢复后告诉对端去关联的状态 */
                           bit_resv                    : 26;
    oal_uint32             ul_excp_type;       /* 异常类型 */
    oal_uint32             ul_dfr_num;         /* DFR statistics */
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_mutex_stru         wifi_excp_mutex;
#endif
    oal_completion         st_plat_process_comp;           /* 用来检测device异常恢复过程中平台工作是否完成的信号量 */

}hmac_dfr_info_stru;
#endif //_PRE_WLAN_FEATURE_DFR

typedef struct
{
    oal_netbuf_head_stru            st_msdu_head;         /* msdu链表头 */
    frw_timeout_stru                st_amsdu_timer;
    oal_spin_lock_stru              st_amsdu_lock;        /* amsdu task lock */

    oal_uint16                      us_amsdu_maxsize;
    oal_uint16                      us_amsdu_size;        /* Present size of the AMSDU */

    oal_uint8                       uc_amsdu_maxnum;
    oal_uint8                       uc_msdu_num;          /* Number of sub-MSDUs accumulated */
    oal_uint8                       uc_last_pad_len;      //51合入后可删除/* 最后一个msdu的pad长度 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_uint8                       uc_short_pkt_num;     /* 短包计数   */
#else
    oal_uint8                       auc_reserve[1];
#endif

    oal_uint8                       auc_eth_da[WLAN_MAC_ADDR_LEN];  //51合入后可删除
    oal_uint8                       auc_eth_sa[WLAN_MAC_ADDR_LEN];  //51合入后可删除
}hmac_amsdu_stru;

/* hmac配置私有结构 */
typedef struct
{
    oal_wait_queue_head_stru  st_wait_queue_for_sdt_reg;            /* 用于wal_config层线程等待(wal_config-->hmac),给SDT下发读寄存器命令时用 */
    oal_bool_enum_uint8       en_wait_ack_for_sdt_reg;
    oal_uint8                 auc_resv2[3];
    oal_int8                  ac_rsp_msg[HMAC_RSP_MSG_MAX_LEN];     /* get wid返回消息内存空间 */
    oal_uint32                dog_tag;

}hmac_vap_cfg_priv_stru;

/* WAL抛事件给HMAC时的事件PAYLOAD结构体 */
typedef struct
{
    oal_netbuf_stru        *pst_netbuf;         /* netbuf链表一个元素 */
    mac_vap_stru           *pst_vap;
}hmac_tx_event_stru;

/* HMAC抛去关联完成事件结构体 */
typedef struct
{
    oal_uint8              *puc_msg;
}hmac_disasoc_comp_event_stru;

/* 扫描结果 */
typedef struct
{
    oal_uint8     uc_num_dscr;
    oal_uint8     uc_result_code;
    oal_uint8     auc_resv[2];
}hmac_scan_rsp_stru;

/* Status code for MLME operation confirm */
typedef enum
{
    HMAC_MGMT_SUCCESS             = 0,
    HMAC_MGMT_INVALID             = 1,
    HMAC_MGMT_TIMEOUT             = 2,
    HMAC_MGMT_REFUSED             = 3,
    HMAC_MGMT_TOMANY_REQ          = 4,
    HMAC_MGMT_ALREADY_BSS         = 5
}hmac_mgmt_status_enum;
typedef oal_uint8   hmac_mgmt_status_enum_uint8;

/* 关联结果 */
typedef struct
{
    hmac_mgmt_status_enum_uint8  en_result_code;         /* 关联成功,超时等 */
    oal_uint8                    auc_resv1[1];
    mac_status_code_enum_uint16  en_status_code;         /* ieee协议规定的16位状态码  */

    oal_uint8                    auc_addr_ap[WLAN_MAC_ADDR_LEN];
    oal_uint8                    auc_resv2[2];

    oal_uint32                   ul_asoc_req_ie_len;
    oal_uint32                   ul_asoc_rsp_ie_len;

    oal_uint8                    *puc_asoc_req_ie_buff;
    oal_uint8                    *puc_asoc_rsp_ie_buff;
}hmac_asoc_rsp_stru;
/* 漫游结果 */
typedef struct
{
    oal_uint8                    auc_bssid[WLAN_MAC_ADDR_LEN];
    oal_uint8                    auc_resv1[2];
    mac_channel_stru             st_channel;
    oal_uint32                   ul_asoc_req_ie_len;
    oal_uint32                   ul_asoc_rsp_ie_len;
    oal_uint8                   *puc_asoc_req_ie_buff;
    oal_uint8                   *puc_asoc_rsp_ie_buff;
}hmac_roam_rsp_stru;

#ifdef _PRE_WLAN_FEATURE_HILINK_TEMP_PROTECT

#define HMAC_NOTIFY_STA_RSSI_MAX_NUM   32

typedef struct
{
    oal_uint8                    auc_user_mac_addr[WLAN_MAC_ADDR_LEN];
    oal_int8                     c_rssi;
    oal_uint8                    uc_valid;
} hmac_notify_all_sta_rssi_member;

typedef struct
{
    oal_uint32                   ul_start_index;
    oal_uint32                   ul_sta_count;
    hmac_notify_all_sta_rssi_member ast_sta_rssi[HMAC_NOTIFY_STA_RSSI_MAX_NUM];
} hmac_notify_all_sta_rssi_stru;

#endif

#ifdef _PRE_WLAN_FEATURE_11R
typedef struct
{
    oal_uint8                    auc_bssid[WLAN_MAC_ADDR_LEN];
    oal_uint16                   us_ft_ie_len;
    oal_uint8                   *puc_ft_ie_buff;
}hmac_roam_ft_stru;

#endif //_PRE_WLAN_FEATURE_11R

/*mic攻击*/
typedef struct
{
    oal_uint8                  auc_user_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                  auc_reserve[2];
    oal_nl80211_key_type       en_key_type;
    oal_int32                  l_key_id;
}hmac_mic_event_stru;

/* 上报接收到管理帧事件的数据结构 */
typedef struct
{
    oal_uint8                  *puc_buf;
    oal_uint16                  us_len;
    oal_uint8                   uc_rssi;        /* 已经在驱动加上HMAC_FBT_RSSI_ADJUST_VALUE将负值转成正值 */
    oal_uint8                   uc_rsv[1];
    oal_int32                   l_freq;
    oal_int8                    ac_name[OAL_IF_NAME_SIZE];
}hmac_rx_mgmt_event_stru;

/* 上报监听超时数据结构 */
typedef struct
{
    oal_ieee80211_channel_stru  st_listen_channel;
    oal_uint64                  ull_cookie;
    oal_wireless_dev_stru      *pst_wdev;

}hmac_p2p_listen_expired_stru;

/* 上报接收到管理帧事件的数据结构 */
typedef struct
{
    oal_uint8                   uc_dev_mode;
    oal_uint8                   uc_vap_mode;
    oal_uint8                   uc_vap_status;
    oal_uint8                   uc_write_read;
    oal_uint32                  ul_val;
}hmac_cfg_rx_filter_stru;

#if 0
struct mac_rx_ctl_cut
{
    /*word 0*/
    oal_uint8                   bit_vap_id            :5;
    oal_uint8                   bit_amsdu_enable      :1;
    oal_uint8                   bit_is_first_buffer   :1;
    oal_uint8                   bit_is_fragmented     :1;

    oal_uint8                   uc_msdu_in_buffer;

    oal_uint8                   bit_ta_user_idx       :5;
    oal_uint8                   bit_reserved2         :1;
    oal_uint8                   bit_reserved3         :1;
    oal_uint8                   bit_reserved4         :1;

    oal_uint8                   bit_mac_header_len    :6;   /* mac header帧头长度 */
    oal_uint8                   bit_is_beacon         :1;
    oal_uint8                   bit_is_last_buffer    :1;

    /*word 1*/
    oal_uint16                  us_frame_len;               /* 帧头与帧体的总长度 */
    oal_uint8                   uc_mac_vap_id         :4;
    oal_uint8                   bit_buff_nums         :4;   /* 每个MPDU占用的buf数 */
    oal_uint8                   uc_channel_number;          /* 接收帧的信道 */
    /*word 2*/

}__OAL_DECLARE_PACKED;
typedef struct mac_rx_ctl_cut  mac_rx_ctl_cut_stru;
#endif

typedef struct
{
    oal_wait_queue_head_stru           st_wait_queue;
    oal_bool_enum_uint8                mgmt_tx_complete;
    oal_uint32                         mgmt_tx_status;
    oal_uint8                          mgmt_frame_id;
}oal_mgmt_tx_stru;

#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
extern   oal_void    hmac_parse_packet_etc(oal_netbuf_stru *pst_netbuf_eth);

#define WIFI_WAKESRC_TAG "plat:wifi_wake_src,"
#define IPADDR(addr) \
        ((oal_uint8*)&addr)[0], \
        ((oal_uint8*)&addr)[3]

#define IPADDR6(addr) \
        ntohs((addr).s6_addr16[0]), \
        ntohs((addr).s6_addr16[7])

#define IPV6_ADDRESS_SIZEINBYTES 0x10

struct ieee8021x_hdr {
    oal_uint8 version;
    oal_uint8 type;
    oal_uint16 length;
};
#endif

#ifdef _PRE_WLAN_FEATURE_SPECIAL_PKT_LOG
#define WIFI_SEPCIAL_IPV4_PKT_TAG "wifi:special_ipv4_pkt,"
#define GET_PKT_DIRECTION_STR(direction)      ((direction == HMAC_PKT_DIRECTION_TX) ? "tx" : "rx")
#define HWMACSTR "%02x:%02x:%02x:**:**:%02x"
#define HWMAC2STR(a) (a)[0], (a)[1], (a)[2], (a)[5]

typedef enum{
    HMAC_PKT_DIRECTION_TX = 0,
    HMAC_PKT_DIRECTION_RX = 1,
}hmac_pkt_direction_enum;
extern oal_void hmac_parse_special_ipv4_packet(oal_uint8 *puc_pktdata, oal_uint32 ul_datalen, hmac_pkt_direction_enum en_pkt_direction);
#endif

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void  hmac_board_get_instance_etc(mac_board_stru **ppst_hmac_board);
extern oal_int32  hmac_main_init_etc(oal_void);
extern oal_void  hmac_main_exit_etc(oal_void);
extern oal_uint32  hmac_tx_wlan_to_wlan_ap_etc(frw_event_mem_stru *pst_event_mem);
#ifdef _PRE_WLAN_TCP_OPT
extern oal_uint32  hmac_tx_lan_to_wlan_no_tcp_opt_etc(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf);
#endif
extern oal_uint32  hmac_tx_lan_to_wlan_etc(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_buf);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
extern oal_uint32 hmac_tx_post_event(mac_vap_stru *pst_mac_vap);
extern oal_uint32 hmac_tx_event_process(oal_mem_stru *pst_event_mem);
extern oal_net_dev_tx_enum  hmac_vap_start_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev);
#endif
extern oal_net_dev_tx_enum  hmac_bridge_vap_xmit_etc(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev);

extern oal_uint16 hmac_free_netbuf_list_etc(oal_netbuf_stru  *pst_buf);
extern oal_uint32  hmac_vap_get_priv_cfg_etc(mac_vap_stru *pst_mac_vap, hmac_vap_cfg_priv_stru * * ppst_cfg_priv);
extern oal_net_device_stru  *hmac_vap_get_net_device_etc(oal_uint8 uc_vap_id);
extern oal_int8*  hmac_vap_get_desired_country_etc(oal_uint8 uc_vap_id);
#ifdef _PRE_WLAN_FEATURE_11D
extern oal_uint32  hmac_vap_get_updata_rd_by_ie_switch_etc(oal_uint8 uc_vap_id,oal_bool_enum_uint8 *us_updata_rd_by_ie_switch);
#endif
extern oal_uint32  hmac_config_add_vap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_del_vap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_start_vap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_down_vap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_AP_PM
extern oal_uint32 hmac_config_wifi_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_sta_scan_wake_wow(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
//extern oal_uint32  hmac_config_update_mode(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_mac_addr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_wmmswitch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_bg_noise_info(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_WEB_CMD_COMM
extern oal_uint32  hmac_config_set_hw_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_priv_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_channel(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_priv_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_assoc_num(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_noise(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_bw(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_int32   hmac_config_get_sta_rssi(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_pmf(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_bcast_rate(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_temp(mac_vap_stru * pst_mac_vap, oal_uint16 *pus_len, oal_uint8 * puc_param);
extern oal_uint32  hmac_config_get_temp_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_ko_version(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_rate_info(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_chan_list(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_band(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_pwr_ref(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_vap_cap(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_neighbor_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_scan_stat(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_neighb_no(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_int32  hmac_config_get_neighb_info(mac_vap_stru *pst_mac_vap, oal_ap_scan_result_stru *puc_param);
extern oal_int32  hmac_config_get_hw_flow_stat(mac_vap_stru *pst_mac_vap, oal_machw_flow_stat_stru *puc_param);
extern oal_int32  hmac_config_get_wme_stat(mac_vap_stru *pst_mac_vap, oal_wme_stat_stru *puc_param);
extern oal_uint32  hmac_config_get_ant_rssi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_ant_rssi_report(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_11K_STAT
extern oal_uint32 *hmac_config_get_sta_drop_num(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 *puc_param);
extern oal_uint32 *hmac_config_get_sta_tx_delay(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 *puc_param);
extern oal_int32  hmac_config_get_tx_delay_ac(mac_vap_stru *pst_mac_vap, oal_tx_delay_ac_stru *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_DFS
extern oal_uint32  hmac_config_get_dfs_chn_status(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
#endif
#endif
#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
extern oal_int32  hmac_config_get_snoop_table(mac_vap_stru *pst_mac_vap, oal_snoop_all_group_stru *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
extern oal_uint32  hmac_config_set_m2s_switch_blacklist(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_m2s_switch_mss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_get_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_bss_type_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
extern oal_uint32  hmac_config_get_bss_type_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_ssid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
extern oal_uint32  hmac_config_get_ssid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_ampdu(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_shpreamble_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_shpreamble_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_shortgi20_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_shortgi20_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_shortgi40_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_shortgi40_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_shortgi80_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_shortgi80_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_MONITOR
extern oal_uint32  hmac_config_set_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_get_addr_filter_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_prot_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_prot_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_auth_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_auth_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_max_user_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint32 ul_max_user);
extern oal_uint32  hmac_config_set_bintval_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_bintval_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_nobeacon_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_nobeacon_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_txpower_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
extern oal_uint32  hmac_config_chip_check(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_get_mpdu_num_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if 0
extern oal_uint32  hmac_config_ota_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_CHIP_TEST
extern oal_uint32  hmac_config_beacon_offload_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_clear_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_ota_beacon_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_ota_rx_dscr_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_all_ota_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_oam_output_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_dhcp_arp_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_random_mac_addr_scan_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_random_mac_oui_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_txpower_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_freq_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_freq_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_wmm_params_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_wmm_params_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_vap_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
extern oal_uint32 hmac_config_pk_mode_debug(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_event_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_eth_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_80211_ucast_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_mgmt_log_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_start);
extern oal_uint32  hmac_config_80211_mcast_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_probe_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_protocol_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_phy_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_DHCP_REQ_DISABLE
extern oal_uint32 hmac_config_set_dhcp_req_disable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32  hmac_config_report_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_wfa_cfg_aifsn_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_wfa_cfg_cw_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_lte_gpio_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_vowifi_nat_keep_alive_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

extern oal_uint32  hmac_config_log_level_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_dtimperiod_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_dtimperiod_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_user_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_add_user_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_del_user_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_ampdu_start_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_CHIP_FPGA_PCIE_TEST
extern oal_uint32  hmac_config_pcie_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_ampdu_end_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_auto_ba_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_profiling_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_addba_req_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_delba_req_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_tx_pow_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_dscr_param_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_rate_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_mcs_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_mcsac_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_mcsax(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_rfch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_bw_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_always_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_always_tx_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_always_tx_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_always_rx_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
extern oal_uint32  hmac_config_always_tx_51(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_always_rx_51(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_DEBUG_MODE
extern oal_uint32  hmac_config_set_rxch(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
extern oal_uint32  hmac_config_dync_txpower(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
extern oal_uint32  hmac_config_dync_pow_debug_switch(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
#endif
extern oal_uint32  hmac_config_get_thruput(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_freq_skew(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_adjust_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_pcie_pm_level_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_reg_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
extern oal_uint32  hmac_config_sdio_flowctrl_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

extern oal_uint32  hmac_config_amsdu_start_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_list_ap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_list_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_sta_list_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_list_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_sta_initiate_scan_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_cfg80211_start_sched_scan_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_cfg80211_stop_sched_scan_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_cfg80211_start_scan_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_sta_initiate_join_etc(mac_vap_stru *pst_mac_vap, mac_bss_dscr_stru *pst_bss_dscr);
extern oal_void    hmac_mgmt_send_deauth_frame_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_da, oal_uint16 us_err_code, oal_bool_enum_uint8 en_is_protected);
extern oal_uint32  hmac_config_send_deauth_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_da);
extern oal_void    hmac_mgmt_send_disassoc_frame_etc(mac_vap_stru *pst_mac_vap,oal_uint8 *puc_da, oal_uint16 us_err_code, oal_bool_enum_uint8 en_is_protected);

extern oal_uint32 hmac_config_11i_add_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_11i_get_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_11i_remove_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_11i_set_default_key_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
//extern oal_uint32 hmac_config_11i_add_wep_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_kick_user_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_VOWIFI
extern oal_uint32  hmac_config_vowifi_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern oal_uint32 hmac_config_update_ip_filter_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif //_PRE_WLAN_FEATURE_IP_FILTER

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
extern oal_uint32 hmac_config_set_oma(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_proxysta_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_ONLINE_DPD
extern oal_uint32  hmac_config_dpd_cfg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

extern oal_uint32 hmac_config_pause_tid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_dump_timer(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_user_vip(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_vap_host(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_send_bar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_reg_write_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_check_capability_mac_phy_supplicant_etc(mac_vap_stru *pst_mac_vap, mac_bss_dscr_stru *pst_bss_dscr);
oal_uint32  hmac_config_dump_ba_bitmap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_dump_all_rx_dscr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_vap_pkt_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_amsdu_ampdu_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_11i_add_wep_entry_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_sdt_recv_reg_cmd_etc(mac_vap_stru  *pst_mac_vap, oal_uint8 *puc_buf, oal_uint16 us_len);
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
extern oal_uint32 hmac_sdt_recv_sample_cmd(mac_vap_stru  *pst_mac_vap, oal_uint8 *puc_buf, oal_uint16 us_len);
#endif
extern oal_uint32 hmac_init_event_process_etc(frw_event_mem_stru *pst_event_mem);
extern oal_uint32 hmac_config_dump_rx_dscr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_dump_tx_dscr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_alg_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
extern oal_uint32 hmac_config_tcp_ack_buf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_FEATURE_FAST_AGING
extern oal_uint32 hmac_config_fast_aging(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_get_fast_aging(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_CAR
extern oal_uint32 hmac_config_car_cfg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
extern oal_uint32 hmac_config_waveapp_32plus_user_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

extern oal_uint32 hmac_config_rssi_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_PRODUCT_1151V200
extern oal_uint32 hmac_config_80m_rts_debug(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_DEBUG_MODE_USER_TRACK
extern oal_uint32 hmac_config_report_thrput_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
oal_uint32  hmac_config_set_txop_ps_machw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
extern oal_uint32 hmac_config_print_btcoex_status_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_btcoex_preempt_tpye(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_btcoex_set_perf_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_btcoex_rx_delba_trigger_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_void hmac_btcoex_arp_fail_delba_process_etc(oal_netbuf_stru *pst_netbuf, mac_vap_stru *pst_mac_vap);
#endif
#ifdef _PRE_WLAN_FEATURE_LTECOEX
oal_uint32 hmac_config_ltecoex_mode_set(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
extern oal_uint32  hmac_config_set_vap_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
//extern oal_uint32  hmac_config_get_smps_mode_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD
extern oal_uint32  hmac_config_set_uapsden_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_uapsden_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#endif
extern oal_uint32 hmac_config_set_reset_state_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_DFT_STAT
extern oal_uint32 hmac_config_set_phy_stat_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_dbb_env_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_usr_queue_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_report_vap_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_report_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
oal_uint32  hmac_config_get_hipkt_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_flowctl_param_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_flowctl_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
extern oal_uint32 hmac_config_set_rd_by_ie_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_dscp_map_to_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_clean_dscp_tid_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_amsdu_tx_on_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_ampdu_tx_on_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_amsdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_ampdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_country_for_dfs_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_country_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_regdomain_pwr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
extern oal_uint32  hmac_config_reduce_sar_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
extern oal_uint32  hmac_config_tas_pwr_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
extern oal_uint32 hmac_config_tas_rssi_access(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_get_country_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_connect_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_tid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_reset_operate_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

extern oal_uint32 hmac_config_set_mib_by_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_beacon_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_get_assoc_req_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_wps_p2p_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_wps_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_app_ie_to_vap_etc(mac_vap_stru *pst_mac_vap,
                                            oal_app_ie_stru *pst_wps_ie,
                                            en_app_ie_type_uint8 en_type);

extern oal_uint32  hmac_config_alg_param_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND
extern oal_uint32  hmac_config_set_restrict_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE)
extern oal_uint32  hmac_config_set_omit_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
extern oal_uint32  hmac_config_dfs_radartool_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_SUPPORT_ACS
extern oal_uint32  hmac_config_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_chan_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
extern oal_uint32  hmac_config_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_11V
extern oal_uint32  hmac_11v_cfg_wl_mgmt_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_DEBUG_MODE
extern oal_uint32  hmac_11v_ap_tx_request(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif  //  _PRE_WLAN_FEATURE_11V

#if defined(_PRE_WLAN_FEATURE_11V) || defined(_PRE_WLAN_FEATURE_11V_ENABLE)
extern oal_uint32  hmac_11v_cfg_bsst_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_11v_sta_tx_query(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE
extern oal_uint32   hmac_config_dfr_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32   hmac_config_trig_pcie_reset(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32   hmac_config_trig_loss_tx_comp(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif

extern oal_uint32 hmac_config_set_acs_cmd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_beacon_chain_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if 0
extern oal_uint32 hmac_config_tdls_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_tdls_channel_switch_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_set_2040_coext_support_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_rx_fcs_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_PERFORM_STAT
extern oal_uint32  hmac_config_pfm_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_pfm_display(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
extern oal_uint32  hmac_config_set_edca_opt_switch_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_edca_opt_weight_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_edca_opt_switch_ap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_edca_opt_cycle_ap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_sync_cmd_common_etc(mac_vap_stru *pst_mac_vap,wlan_cfgid_enum_uint16 en_cfg_id,oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_open_wmm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_STA_PM
extern oal_uint32  hmac_config_set_pm_by_module_etc(mac_vap_stru *pst_mac_vap, mac_pm_ctrl_type_enum pm_ctrl_type, mac_pm_switch_enum pm_enable);
#endif

#ifdef _PRE_WLAN_CHIP_TEST
extern oal_uint32 hmac_test_send_action(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_send_pspoll(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_send_nulldata(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_lpm_tx_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
extern oal_uint32 hmac_enable_pmf_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_set_thruput_bypass(mac_vap_stru *pst_mac_vap,wlan_cfgid_enum_uint16 en_cfg_id,oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_coex(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_auto_protection_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_dfx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_send_2040_coext_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_2040_coext_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_version_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_ant_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_DEBUG_MODE
extern oal_uint32  hmac_config_get_all_reg_value(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_cali_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_DAQ
extern oal_uint32  hmac_config_data_acq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
extern oal_uint32 hmac_config_get_smps_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
extern oal_uint32  hmac_config_resume_rx_intr_fifo(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

extern oal_uint32  hmac_tx_report_eth_frame_etc(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_netbuf);

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
extern oal_uint32  hmac_config_set_opmode_notify_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_user_rssbw_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
extern oal_uint32  hmac_config_set_m2s_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
extern oal_uint32  hmac_config_get_blacklist_mode(mac_vap_stru *pst_mac_vap, oal_uint16* pus_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_blacklist_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_blacklist_add_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_blacklist_add_only_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_blacklist_del_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_show_blacklist_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_autoblacklist_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_autoblacklist_aging_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_autoblacklist_threshold_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_autoblacklist_reset_time_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_ISOLATION
extern oal_uint32  hmac_config_show_isolation(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_isolation_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_isolation_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_isolation_forword(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_isolation_clear(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
/*extern oal_void hmac_config_get_blacklist(mac_vap_stru *pst_mac_vap,oal_uint8 *pst_info_str,oal_int16 str_len);*/
/*extern oal_void hmac_config_get_isolation(mac_vap_stru *pst_mac_vap,oal_uint8 *pst_info_str,oal_int16 str_len);*/
#endif
extern oal_uint32  hmac_config_set_vap_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_DEBUG_MODE
extern oal_uint32  hmac_config_report_ampdu_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_bgscan_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_ampdu_aggr_num_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
extern oal_uint32  hmac_config_set_ampdu_mmss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_DEBUG_MODE
extern oal_uint32  hmac_config_freq_adjust(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS
extern oal_uint32 hmac_config_set_psd_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_cfg_psd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#endif
#ifdef _PRE_WLAN_FEATURE_CSI
extern oal_uint32  hmac_config_set_csi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

extern oal_uint32  hmac_config_set_stbc_cap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_ldpc_cap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_txbf_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

extern oal_uint32 hmac_config_set_pmksa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_del_pmksa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_flush_pmksa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_scan_abort_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_remain_on_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_cancel_remain_on_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_HS20
extern oal_uint32  hmac_config_set_qos_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
extern oal_uint32  hmac_config_set_p2p_ps_ops_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_p2p_ps_noa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_p2p_ps_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_wpas_mgmt_tx_etc(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_vap_classify_en_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_vap_classify_tid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_fem_pa_status_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_query_station_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_query_rssi_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_query_psst(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_11K_STAT
extern oal_uint32  hmac_config_query_drop_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_query_tx_delay(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_query_rate_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_DFT_STAT
extern oal_uint32  hmac_config_query_ani_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

extern oal_uint32  hmac_config_vap_state_syn_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
extern oal_uint32  hmac_config_set_uapsd_para_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_PROFLING_MIPS
extern oal_uint32  hmac_config_set_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_show_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
oal_uint32 hmac_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_set_ip_addr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
oal_uint32   hmac_config_user_num_spatial_stream_cap_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_uint32   hmac_config_cfg_vap_h2d_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_user_asoc_state_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
extern oal_uint32 hmac_config_user_cap_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
extern oal_uint32  hmac_config_user_rate_info_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
extern oal_uint32  hmac_config_user_info_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
#ifdef _PRE_WLAN_FEATURE_M2S
extern oal_uint32  hmac_config_vap_m2s_info_syn(mac_vap_stru *pst_mac_vap);
#endif
extern oal_uint32  hmac_config_sta_vap_info_syn_etc(mac_vap_stru *pst_mac_vap);
extern oal_uint32 hmac_init_user_security_port_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
extern oal_uint32 hmac_user_set_asoc_state_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, mac_user_asoc_state_enum_uint8 en_value);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32 hmac_config_ch_status_sync(mac_device_stru *pst_mac_dev);
#endif
#ifdef _PRE_WLAN_TCP_OPT
oal_uint32  hmac_config_get_tcp_ack_stream_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_tx_tcp_ack_opt_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_rx_tcp_ack_opt_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_tx_tcp_ack_limit_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_rx_tcp_ack_limit_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44)) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_DFT_STAT
oal_uint32  hmac_config_set_performance_log_switch(mac_vap_stru *pst_mac_vap,wlan_cfgid_enum_uint16 en_cfg_id,oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif
extern oal_void hmac_config_del_p2p_ie_etc(oal_uint8 *puc_ie, oal_uint32 *pul_ie_len);
extern oal_uint32  hmac_find_p2p_listen_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
extern oal_void  hmac_set_device_freq_mode_etc(oal_uint8 uc_device_freq_type);
extern oal_uint32 hmac_config_set_device_freq_etc(oal_uint8 uc_device_freq_type);
#endif

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
extern oal_uint32  hmac_config_set_tx_classify_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
extern oal_uint32 hmac_config_dyn_cali_param(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
extern oal_uint32  hmac_config_fbt_rej_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_scan_list_clear(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_scan_specified_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_start_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_print_scan_list(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_scan_interval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_scan_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_scan_report_period(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_fbt_scan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_cur_channel(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_stay_time(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_white_lst_ssidhiden(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_sta_11v_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_change_to_other_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
extern oal_uint32 hmac_config_set_mgmt_frame_filters(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_get_all_sta_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_get_vap_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_sensing_bssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_get_sensing_bssid_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#endif  // _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
extern oal_uint32 hmac_config_send_action_frame(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_mgmt_frame_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_mgmt_cap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
extern oal_uint32  hmac_config_get_sta_11k_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_sta_bcn_request(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif
extern oal_uint32  hmac_config_get_sta_11h_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_vendor_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_11R_AP
extern oal_uint32  hmac_config_get_sta_11r_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_mlme(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,9,0)
extern oal_int32   hmac_cfg80211_dump_survey_etc(oal_wiphy_stru *pst_wiphy, oal_net_device_stru *pst_netdev,
                                      oal_int32 l_idx, oal_survey_info_stru *pst_info);
#endif
#ifdef _PRE_WLAN_FEATURE_11K
extern oal_uint32  hmac_config_send_neighbor_req_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_bcn_table_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_voe_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
extern oal_uint32  hmac_config_send_cw_signal(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_get_cali_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
oal_uint32 hmac_config_vendor_cmd_get_channel_list_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_SMARTANT
extern oal_uint32  hmac_config_get_ant_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_double_ant_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
extern oal_uint32  hmac_config_packet_capture_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_PRODUCT_1151V200
extern oal_uint32 hmac_calc_up_ap_num_etc(mac_device_stru *pst_mac_device);
#endif
extern oal_uint32 hmac_config_set_priv_flag(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
extern oal_uint32 hmac_config_set_bw_fixed(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
extern oal_uint32 hmac_config_dbdc_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifndef CONFIG_HAS_EARLYSUSPEND
extern oal_void  hmac_do_suspend_action_etc(mac_device_stru    *pst_mac_device, oal_uint8  uc_in_suspend);
#endif
#ifdef _PRE_WLAN_FEATURE_APF
oal_uint32  hmac_config_set_apf_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_ext_if.h */
