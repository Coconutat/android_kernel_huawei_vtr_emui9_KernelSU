

#ifndef __WAL_LINUX_SCAN_H__
#define __WAL_LINUX_SCAN_H__

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
#include "hmac_device.h"
#include "wal_linux_rx_rsp.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_LINUX_SCAN_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/

/* 扫描上报"最近时间" 范围内的扫描结果 */
#define WAL_SCAN_REPORT_LIMIT         5000       /* 5000 milliseconds */

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
extern oal_void  wal_inform_all_bss_etc(oal_wiphy_stru  *pst_wiphy, hmac_bss_mgmt_stru  *pst_bss_mgmt, oal_uint8   uc_vap_id);
extern oal_uint32 wal_scan_work_func_etc(hmac_scan_stru                *pst_scan_mgmt,
                                         oal_net_device_stru              *pst_netdev,
                                         oal_cfg80211_scan_request_stru   *pst_request);
extern oal_int32 wal_force_scan_complete_etc(oal_net_device_stru *pst_net_dev, oal_bool_enum en_is_aborted);
extern oal_int32 wal_force_scan_complete_for_disconnect_scene(oal_net_device_stru   *pst_net_dev);
extern oal_int32 wal_stop_sched_scan_etc(oal_net_device_stru *pst_netdev);

#define IS_P2P_SCAN_REQ(pst_request) ((pst_request->n_ssids > 0) && (NULL != pst_request->ssids )\
        && (pst_request->ssids[0].ssid_len == OAL_STRLEN("DIRECT-")) \
        && (0 == oal_memcmp(pst_request->ssids[0].ssid, "DIRECT-", OAL_STRLEN("DIRECT-"))))

oal_void wal_update_bss_etc(oal_wiphy_stru *pst_wiphy, hmac_bss_mgmt_stru *pst_bss_mgmt, oal_uint8 *puc_bssid);


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of wal_linux_scan.h */
