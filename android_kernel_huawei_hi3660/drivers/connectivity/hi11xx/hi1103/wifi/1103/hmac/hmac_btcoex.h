

#ifndef __HMAC_BTCOEX_H__
#define __HMAC_BTCOEX_H__

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "frw_ext_if.h"
#include "oal_ext_if.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_BTCOEX_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define BTCOEX_BSS_NUM_IN_BLACKLIST 16
#define BTCOEX_ARP_FAIL_DELBA_NUM   2
#define BTCOEX_ARP_FAIL_REASSOC_NUM 4
#define BTCOEX_ARP_PROBE_TIMEOUT        (300)  //ms

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    BTCOEX_BLACKLIST_TPYE_FIX_BASIZE  = 0,
    BTCOEX_BLACKLIST_TPYE_NOT_AGGR    = 1,

    BTCOEX_BLACKLIST_TPYE_BUTT
}btcoex_blacklist_type_enum;
typedef oal_uint8 btcoex_blacklist_type_enum_uint8;

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
typedef struct
{
    oal_uint16 us_last_baw_start;       /* 上一次接收到ADDBA REQ中的baw_start值 */
    oal_uint16 us_last_seq_num;         /* 上一次接收到ADDBA REQ中的seq_num值 */
    btcoex_blacklist_type_enum_uint8 en_blacklist_tpye;     /* 黑名单方案 */
    oal_bool_enum_uint8              en_ba_handle_allow;    /* 黑名单方案1:是否允许共存删建聚合 黑名单方案2:是否允许建立聚合 */
} hmac_btcoex_addba_req_stru;

typedef struct
{
    oal_uint8 auc_user_mac_addr[WLAN_MAC_ADDR_LEN];     /* 黑名单MAC地址 */
    oal_uint8 uc_type;                                  /* 写入黑名单的类型 */
    oal_uint8 uc_used;                                  /* 是否已经写过黑名单MAC地址 */
} hmac_btcoex_delba_exception_stru;

typedef struct
{
    frw_timeout_stru st_delba_opt_timer;    /* 发送ARP REQ后启动定时器 */
    oal_atomic ul_rx_unicast_pkt_to_lan;    /* 接收到的单播帧个数 */
} hmac_btcoex_arp_req_process_stru;

typedef struct
{
    hmac_btcoex_delba_exception_stru ast_hmac_btcoex_delba_exception[BTCOEX_BSS_NUM_IN_BLACKLIST];
    oal_uint8 uc_exception_bss_index;       /* 黑名单MAC地址的数组下标 */
    oal_uint8 auc_resv[3];
} hmac_device_btcoex_stru;

typedef struct
{
    hmac_btcoex_arp_req_process_stru st_hmac_btcoex_arp_req_process;
    hmac_btcoex_addba_req_stru       st_hmac_btcoex_addba_req;
    oal_uint16                       us_ba_size;
    oal_uint8                        uc_rx_no_pkt_count;                /* 超时时间内没有收到帧的次数 */
    oal_bool_enum_uint8              en_delba_btcoex_trigger;           /* 是否btcoex触发删建BA */
    oal_bool_enum_uint8              en_arp_probe_on;                   /* 是否打开arp统计，做重关联保护 */
} hmac_user_btcoex_stru;

#define HMAC_BTCOEX_GET_BLACKLIST_TYPE(_pst_hmac_user) \
    ((_pst_hmac_user)->st_hmac_user_btcoex.st_hmac_btcoex_addba_req.en_blacklist_tpye)

#define HMAC_BTCOEX_GET_BLACKLIST_DELBA_HANDLE_ALLOW(_pst_hmac_user) \
    ((_pst_hmac_user)->st_hmac_user_btcoex.st_hmac_btcoex_addba_req.en_ba_handle_allow)

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void hmac_btcoex_check_rx_same_baw_start_from_addba_req_etc(oal_void *p_arg,
                                                        oal_void *p_arg1,
                                                        mac_ieee80211_frame_stru *pst_frame_hdr,
                                                        oal_uint8 *puc_action);
extern oal_uint32 hmac_btcoex_check_exception_in_list_etc(oal_void *p_arg, oal_uint8 *auc_addr);
extern oal_void hmac_btcoex_blacklist_handle_init(oal_void *p_arg);


#endif /* #ifdef _PRE_WLAN_FEATURE_BTCOEX */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of __HMAC_BTCOEX_H__ */

