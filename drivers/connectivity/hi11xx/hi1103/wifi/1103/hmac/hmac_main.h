

#ifndef __HMAC_MAIN_H__
#define __HMAC_MAIN_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "hmac_ext_if.h"
#include "dmac_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MAIN_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_ERR_LOG(_uc_vap_id, _puc_string)
#define HMAC_ERR_LOG1(_uc_vap_id, _puc_string, _l_para1)
#define HMAC_ERR_LOG2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define HMAC_ERR_LOG3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define HMAC_ERR_LOG4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define HMAC_ERR_VAR(_uc_vap_id, _c_fmt, ...)

#define HMAC_WARNING_LOG(_uc_vap_id, _puc_string)
#define HMAC_WARNING_LOG1(_uc_vap_id, _puc_string, _l_para1)
#define HMAC_WARNING_LOG2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define HMAC_WARNING_LOG3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define HMAC_WARNING_LOG4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define HMAC_WARNING_VAR(_uc_vap_id, _c_fmt, ...)

#define HMAC_INFO_LOG(_uc_vap_id, _puc_string)
#define HMAC_INFO_LOG1(_uc_vap_id, _puc_string, _l_para1)
#define HMAC_INFO_LOG2(_uc_vap_id, _puc_string, _l_para1, _l_para2)
#define HMAC_INFO_LOG3(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3)
#define HMAC_INFO_LOG4(_uc_vap_id, _puc_string, _l_para1, _l_para2, _l_para3, _l_para4)
#define HMAC_INFO_VAR(_uc_vap_id, _c_fmt, ...)

/* 填写HMAC到DMAC配置同步消息头 */
#define HMAC_INIT_SYN_MSG_HDR(_pst_syn_msg, _en_syn_id, _us_len) \
    do {                                            \
        (_pst_syn_msg)->en_syn_id = (_en_syn_id);   \
        (_pst_syn_msg)->us_len = (_us_len);   \
    } while(0)

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
    oal_uint32  ul_time_stamp;
}hmac_timeout_event_stru;

typedef struct
{
    oal_uint32  ul_cfg_id;
    oal_uint32  ul_ac;
    oal_uint32  ul_value;
}hmac_config_wmm_para_stru;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
typedef struct
{
    struct semaphore            st_rxdata_sema;
    oal_task_stru              *pst_rxdata_thread;
    oal_spin_lock_stru          st_lock;
    oal_wait_queue_head_stru    st_rxdata_wq;
    oal_netbuf_head_stru        st_rxdata_netbuf_head;
    oal_uint32                  ul_pkt_loss_cnt;
    oal_bool_enum_uint8         en_rxthread_enable;
}hmac_rxdata_thread_stru;
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
extern oal_uint32  hmac_event_fsm_register_etc(oal_void);
extern oal_void  hmac_main_exit_etc(oal_void);
extern oal_int32  hmac_main_init_etc(oal_void);
extern oal_uint32  hmac_config_send_event_etc(
                mac_vap_stru                     *pst_mac_vap,
                wlan_cfgid_enum_uint16            en_cfg_id,
                oal_uint16                        us_len,
                oal_uint8                        *puc_param);
extern oal_uint32  hmac_sdt_up_reg_val_etc(frw_event_mem_stru  *pst_event_mem);

#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
extern oal_uint32  hmac_sdt_recv_sample_cmd(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buf, oal_uint16 us_len);
extern oal_uint32  hmac_sdt_up_sample_data(frw_event_mem_stru  *pst_event_mem);

#endif

#ifdef _PRE_WLAN_ONLINE_DPD
extern oal_uint32  hmac_sdt_up_dpd_data(frw_event_mem_stru  *pst_event_mem);
#endif
#ifdef _PRE_WLAN_FEATURE_APF
extern oal_uint32  hmac_apf_program_report_event(frw_event_mem_stru  *pst_event_mem);
#endif
#ifdef _PRE_WLAN_RF_AUTOCALI
extern oal_uint32  hmac_sdt_recv_autocali_cmd(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_buf, oal_uint16 us_len);
extern oal_uint32  hmac_sdt_send_autocali_data(frw_event_mem_stru  *pst_event_mem);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_void  hmac_rxdata_netbuf_enqueue_etc(oal_netbuf_stru  *pst_netbuf);
extern oal_void  hmac_rxdata_sched_etc(oal_void);
extern oal_bool_enum_uint8 hmac_get_rxthread_enable_etc(oal_void);
#endif

extern oal_uint32  hmac_proc_query_response_event_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);

extern oal_wakelock_stru   g_st_hmac_wakelock_etc;
#define hmac_wake_lock()  oal_wake_lock(&g_st_hmac_wakelock_etc)
#define hmac_wake_unlock()  oal_wake_unlock(&g_st_hmac_wakelock_etc)

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_main */
