
#ifndef __HMAC_WDS_H__
#define __HMAC_WDS_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "mac_vap.h"
#include "hmac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_WDS_H
#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
/*****************************************************************************/
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define WDS_CALC_MAC_HASH_VAL(mac_addr) (MAC_CALCULATE_HASH_VALUE(mac_addr) & (WDS_HASH_NUM-1))
#define WDS_TABLE_ADD_ENTRY             0
#define WDS_TABLE_DEL_ENTRY             1
#define WDS_MAX_STAS_NUM                256
#define WDS_MAX_NODE_NUM                4
#define WDS_MAX_NEIGH_NUM               512

#define WDS_MIN_AGE_NUM                 5
#define WDS_MAX_AGE_NUM                 10000

#define WDS_TABLE_DEF_TIMER             15000     /* timer interval as 15 secs */

#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
#define VMSTA_4ADDR_SUPPORT             0
#define VMSTA_4ADDR_UNSUPPORT           -1
#endif
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
    oal_dlist_head_stru                 st_entry;
    oal_uint8                           auc_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                           uc_stas_num;
    oal_uint8                           auc_resv[1];
}hmac_wds_node_stru;

typedef struct
{
    oal_dlist_head_stru                 st_entry;
    oal_uint32                          ul_last_pkt_age;
    hmac_wds_node_stru                  *pst_related_node;
    oal_uint8                           auc_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                           auc_resv[2];
}hmac_wds_stas_stru;

typedef struct
{
    oal_dlist_head_stru                 st_entry;
    oal_uint32                          ul_last_pkt_age;
    oal_uint8                           auc_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                           auc_resv[2];
}hmac_wds_neigh_stru;

typedef struct
{
    oal_uint8                           auc_sta_mac[WLAN_MAC_ADDR_LEN];
    oal_uint8                           auc_node_mac[WLAN_MAC_ADDR_LEN];
}mac_cfg_wds_sta_stru;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
typedef oal_uint32 (*p_hmac_wds_node_func)(hmac_vap_stru *pst_vap, oal_uint8 *puc_addr, oal_void *pst_arg);




/*****************************************************************************
  10 函数声明
*****************************************************************************/

extern oal_uint32  hmac_wds_update_table(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_node_mac, oal_uint8 *puc_sta_mac, oal_uint8 uc_update_mode);
extern oal_uint32  hmac_wds_add_node(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_node_mac);
extern oal_uint32  hmac_wds_del_node(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_node_mac);
extern oal_uint32  hmac_wds_reset_sta_mapping_table(hmac_vap_stru *pst_hmac_vap);
extern oal_uint32  hmac_wds_add_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_node_mac, oal_uint8 *puc_sta_mac);
extern oal_uint32  hmac_wds_del_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_addr);
extern oal_uint32  hmac_find_valid_user_by_wds_sta(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_sta_mac_addr, oal_uint16 *pus_user_idx);
extern oal_uint32  hmac_wds_node_ergodic(hmac_vap_stru *pst_hmac_vap, oal_uint8 *src_addr, p_hmac_wds_node_func pst_hmac_wds_node, oal_void *pst_arg);

extern oal_uint32 hmac_wds_update_neigh(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_addr);
extern oal_uint32 hmac_wds_neigh_not_expired(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_addr);
extern oal_uint32 hmac_wds_reset_neigh_table(hmac_vap_stru *pst_hmac_vap);

extern oal_uint32 hmac_wds_table_create_timer(hmac_vap_stru *pst_hmac_vap);

#ifdef _PRE_WLAN_FEATURE_WDS
extern oal_uint32 hmac_wds_vap_show_all(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_param);
extern oal_uint32 hmac_vap_set_wds_user(hmac_vap_stru *pst_hmac_vap, oal_uint8 *pst_addr);
#endif

#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
extern oal_bool_enum_uint8 hmac_vmsta_get_user_a4_support(hmac_vap_stru *pst_hmac_vap, oal_uint8 *pst_addr);
extern oal_bool_enum_uint8 hmac_vmsta_check_vap_a4_support(oal_uint8 *puc_ie, oal_uint32 ul_ie_len);
extern oal_bool_enum_uint8 hmac_vmsta_check_user_a4_support(oal_uint8 *puc_frame, oal_uint32 ul_frame_len);
extern oal_uint32 hmac_vmsta_set_vap_a4_enable(mac_vap_stru *pst_mac_vap);
#endif

extern oal_void hmac_wds_init_table(hmac_vap_stru *pst_hmac_vap);
extern oal_uint32  hmac_wds_find_sta(hmac_vap_stru *pst_hmac_vap,oal_uint8 *puc_addr,hmac_wds_stas_stru **ppst_wds_node);
extern oal_uint32 hmac_wds_tx_broadcast_pkt(hmac_vap_stru *pst_vap, oal_uint8 *puc_addr, oal_void *pst_arg);


#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_wds.h */


