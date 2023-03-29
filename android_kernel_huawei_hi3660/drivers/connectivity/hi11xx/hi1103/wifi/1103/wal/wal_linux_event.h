

#ifndef __WAL_LINUX_EVENT_H__
#define __WAL_LINUX_EVENT_H__

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
#include "mac_vap.h"
#include "mac_device.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_EVENT_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/


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

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  wal_cfg80211_start_scan_etc(oal_net_device_stru *pst_net_dev, mac_cfg80211_scan_param_stru *pst_scan_param);
extern oal_uint32  wal_cfg80211_start_sched_scan_etc(oal_net_device_stru *pst_net_dev, mac_pno_scan_stru *pst_pno_scan_info);
extern oal_int32  wal_cfg80211_start_connect_etc(oal_net_device_stru *pst_net_dev, mac_conn_param_stru *pst_connect_param);
extern oal_int32  wal_cfg80211_start_disconnect_etc(oal_net_device_stru *pst_net_dev, mac_cfg_kick_user_param_stru *pst_disconnect_param);
#ifdef _PRE_WLAN_FEATURE_HILINK
extern oal_int32  wal_cfg80211_fbt_kick_user(oal_net_device_stru *pst_net_dev, mac_cfg_kick_user_param_stru *pst_disconnect_param);
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
extern oal_int32 wal_cfg80211_set_okc_ie(oal_net_device_stru *pst_net_dev, oal_app_ie_stru *pst_okc_ie);
extern oal_int32 wal_cfg80211_set_hiden_whitelist(oal_net_device_stru *pst_net_dev, oal_hilink_white_node_stru *pst_white_lst);
extern oal_int32 wal_cfg80211_start_fbt_scan(oal_net_device_stru *pst_net_dev, oal_hilink_scan_params *pst_mac_cfg_fbt_scan_params);
extern oal_int32 wal_cfg80211_set_mgmt_frame_filter(oal_net_device_stru *pst_net_dev, oal_uint32 *mgmt_frame_filter);
extern oal_int32 wal_cfg80211_set_sensing_bssid(oal_net_device_stru *pst_net_dev, oal_void *pst_sensing_bssid);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_AP_PM
extern oal_uint32 wal_config_sta_scan_connect_event(oal_net_device_stru * pst_net_dev,  oal_uint8 *puc_param);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of wal_linux_event.h */
