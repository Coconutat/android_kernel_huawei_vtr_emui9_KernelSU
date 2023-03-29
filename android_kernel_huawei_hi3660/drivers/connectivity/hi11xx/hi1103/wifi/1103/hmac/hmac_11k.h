


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_mem.h"
#include "mac_vap.h"
#include "hmac_user.h"
#include "hmac_vap.h"
#include "oal_schedule.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_11k_H

/*****************************************************************************/
/*****************************************************************************
  2 宏定义
*****************************************************************************/

/* 1us精度 */
#define HMAC_TIME_USEC_INT64(_pst_time) \
    ((oal_int64)((_pst_time)->i_sec) * 1000000 + (oal_int64)((_pst_time)->i_usec))

#define HMAC_RRM_CAL_CHN_LOAD(_pst_chan_results) \
    (oal_uint8)((0 == (_pst_chan_results)->ul_total_stats_time_us)? 0 :                     \
    ((((_pst_chan_results)->ul_total_stats_time_us)                                \
                    - ((_pst_chan_results)->ul_total_free_time_20M_us))*255/      \
                    ((_pst_chan_results)->ul_total_stats_time_us)))   \

#define HMAC_RRM_CAL_RCPI(_rssi) ((oal_uint8)(((_rssi)+110)*2))

#define HMAC_RRM_CAL_TSF_DIFF(_start_time, _end_time) \
        (((_end_time)>(_start_time))?((_end_time)-(_start_time)):(0xFFFFFFFF -(_start_time) + (_end_time)))

#define HMAC_RRM_CAL_DURATION(_start_time, _end_time) \
        (((HMAC_RRM_CAL_TSF_DIFF((_start_time), (_end_time)))+ (1<<9))>>10)

#define HMAC_RRM_TRANS_CHN_LOAD(_chn_load) (oal_uint8)((_chn_load)*255/1000)

#define MAC_RRM_BCN_REPORTING_DETAIL_LEN        1
#define MAC_RRM_BCN_EID_REPORTING_DATAIL        2
#define MAC_RRM_BCN_REQ_PASSIVE_SCAN_TIME       1200

#define MAC_RRM_VAP_MEAS_STAUTS_TIME            (10*1000)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*Beacon Request*/
/*Reporting Detail Values*/
typedef enum {
    MAC_BCN_REPORTING_DETAIL_NO_FIXED_FIELD_OR_ELEM         = 0,
    MAC_BCN_REPORTING_DETAIL_FIXED_FILELD_AND_ANY_ELEM      = 1,
    MAC_BCN_REPORTING_DETAIL_FIXED_FIELD_AND_ELEM           = 2,
    MAC_BCN_REPORTING_DETAIL_BUTT
}mac_rrm_bcn_reporting_detail;
typedef oal_uint8 mac_rrm_bcn_reporting_detail_uint8;

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
/* 通知链钩子指针定义 */
typedef oal_uint32 (*p_rrm_rpt_notify_func)(hmac_user_stru *pst_user, mac_rrm_state_enum en_rpt_state);
/* 钩子函数指针数组结构体 */
typedef struct
{
    p_rrm_rpt_notify_func          pa_rrm_rpt_notify_func[HMAC_RRM_RPT_NOTIFY_BUTT][MAC_RRM_MEAS_TYPE_BUTT];
}hmac_rrm_rpt_hook_stru;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/




/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_void hmac_rrm_hook_init(oal_void);
extern oal_uint32 hmac_register_rrm_rpt_notify_func(mac_rrm_rpt_notify_enum en_notify_sub_type,
       mac_rrm_type_enum en_cur_reqtype, p_rrm_rpt_notify_func p_func);
extern oal_uint32 hmac_rrm_bcn_rpt_notify_hook(hmac_user_stru *pst_hmac_user, mac_rrm_state_enum en_rpt_state);
extern oal_uint32 hmac_rrm_proc_rm_report(hmac_vap_stru* pst_hmac_vap,  hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf);
extern oal_uint32  hmac_config_send_meas_req(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
        mac_rrm_req_cfg_stru *pst_req_cfg);
extern oal_uint32 hmac_rrm_proc_rm_request(hmac_vap_stru* pst_hmac_vap, hmac_user_stru *pst_hmac_user,oal_netbuf_stru *pst_netbuf);
extern oal_uint32 hmac_rrm_get_meas_start_time(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_rrm_proc_neighbor_request(hmac_vap_stru* pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf);
extern oal_uint32 hmac_rrm_proc_neighbor_report(hmac_vap_stru* pst_hmac_vap,  hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf);
extern oal_uint32 hmac_rrm_neighbor_rpt_notify_hook(hmac_user_stru *pst_hmac_user, mac_rrm_state_enum en_rpt_state);
extern oal_uint32 hmac_rrm_neighbor_scan_do(hmac_vap_stru *pst_hmac_vap, mac_neighbor_req_info_stru *pst_neighbor_req,
                 oal_bool_enum_uint8 en_local_meas);
extern oal_uint32 hmac_rrm_free_rpt_list(hmac_user_stru *pst_hmac_user, mac_rrm_type_enum en_reqtype);

extern oal_void hmac_11k_init_vap(hmac_vap_stru *pst_hmac_vap);
extern oal_void hmac_11k_exit_vap(hmac_vap_stru *pst_hmac_vap);
extern oal_void hmac_11k_init_user(hmac_user_stru *pst_hmac_user);
extern oal_void hmac_11k_exit_user(hmac_user_stru *pst_hmac_user);

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif




