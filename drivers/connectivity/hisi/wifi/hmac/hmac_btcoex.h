

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
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define MAX_BTCOEX_BSS_IN_BL 16

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


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
    oal_uint8 uc_delba_enable;          /* 是否允许共存删建BA */
    oal_uint8 auc_resv[3];
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
    hmac_btcoex_delba_exception_stru ast_hmac_btcoex_delba_exception[MAX_BTCOEX_BSS_IN_BL];
    oal_uint8 uc_exception_bss_index;       /* 黑名单MAC地址的数组下标 */
    oal_uint8 auc_resv[3];
} hmac_device_btcoex_stru;

typedef struct
{
    hmac_btcoex_arp_req_process_stru st_hmac_btcoex_arp_req_process;
    hmac_btcoex_addba_req_stru st_hmac_btcoex_addba_req;
    oal_uint16 us_ba_size;
    oal_uint8 uc_rx_no_pkt_count;           /* 超时时间内没有收到帧的次数 */
    oal_uint8 auc_resv[1];
} hmac_user_btcoex_stru;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/


#endif /* #ifdef _PRE_WLAN_FEATURE_COEXIST_BT */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of __HMAC_BTCOEX_H__ */

