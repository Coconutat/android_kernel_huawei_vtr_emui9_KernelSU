

#ifndef __HMAC_MGMT_BSS_COMM_H__
#define __HMAC_MGMT_BSS_COMM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "mac_frame.h"
#include "dmac_ext_if.h"
#include "hmac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MGMT_BSS_COMM_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* HMAC_NCW_INHIBIT_THRED_TIME时间内连续HMAC_RECEIVE_NCW_MAX_CNT次接收到ncw,不上报 */
#define HMAC_NCW_INHIBIT_THRED_TIME   60000    /* 单位ms */
#define HMAC_RECEIVE_NCW_THRED_CNT    6

#define HMAC_FTM_SEND_BUF_LEN    200
#define HMAC_CSI_SEND_BUF_LEN    3000

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
extern oal_uint8 g_auc_avail_protocol_mode_etc[WLAN_PROTOCOL_BUTT][WLAN_PROTOCOL_BUTT];

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32  g_ul_print_wakeup_mgmt_etc;
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
}hmac_location_event_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint16  hmac_mgmt_encap_addba_req_etc(
                       hmac_vap_stru          *pst_vap,
                       oal_uint8              *puc_data,
                       dmac_ba_tx_stru        *pst_tx_ba,
                       oal_uint8               uc_tid);
extern oal_uint16  hmac_mgmt_encap_addba_rsp_etc(
                       hmac_vap_stru      *pst_vap,
                       oal_uint8          *puc_data,
                       hmac_ba_rx_stru    *pst_addba_rsp,
                       oal_uint8           uc_tid,
                       oal_uint8           uc_status);
extern oal_uint16  hmac_mgmt_encap_delba_etc(
                       hmac_vap_stru                      *pst_vap,
                       oal_uint8                          *puc_data,
                       oal_uint8                          *puc_addr,
                       oal_uint8                           uc_tid,
                       mac_delba_initiator_enum_uint8      en_initiator,
                       oal_uint8                           reason);
extern oal_uint32  hmac_mgmt_rx_addba_req_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       oal_uint8                  *puc_payload);
extern oal_uint32  hmac_mgmt_rx_addba_rsp_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       oal_uint8                  *puc_payload);
extern oal_uint32  hmac_mgmt_rx_delba_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       oal_uint8                  *puc_payload);
extern oal_uint32  hmac_mgmt_tx_addba_req_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       mac_action_mgmt_args_stru  *pst_action_args);
extern oal_uint32  hmac_mgmt_tx_addba_rsp_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       hmac_ba_rx_stru            *pst_ba_rx_info,
                       oal_uint8                   uc_tid,
                       oal_uint8                   uc_status);
extern oal_uint32  hmac_mgmt_tx_delba_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       mac_action_mgmt_args_stru  *pst_action_args);
extern oal_uint32  hmac_mgmt_tx_addba_timeout_etc(oal_void *p_arg);
extern oal_uint32  hmac_mgmt_tx_ampdu_start_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       mac_priv_req_args_stru     *pst_priv_req);
extern oal_uint32  hmac_mgmt_tx_ampdu_end_etc(
                       hmac_vap_stru              *pst_hmac_vap,
                       hmac_user_stru             *pst_hmac_user,
                       mac_priv_req_args_stru     *pst_priv_req);


#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
extern oal_uint32  hmac_sa_query_interval_timeout_etc(oal_void *p_arg);
extern oal_void    hmac_send_sa_query_rsp_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *pst_hdr, oal_bool_enum_uint8 en_is_protected);
extern oal_uint32  hmac_start_sa_query_etc(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user, oal_bool_enum_uint8 en_is_protected);
extern oal_uint32  hmac_pmf_check_err_code_etc(mac_user_stru *pst_user_base_info, oal_bool_enum_uint8 en_is_protected, oal_uint8 *puc_mac_hdr);

#endif
extern oal_uint32 hmac_tx_mgmt_send_event_etc(mac_vap_stru *pst_vap, oal_netbuf_stru *pst_mgmt_frame, oal_uint16 us_frame_len);
extern oal_void hmac_mgmt_update_assoc_user_qos_table_etc(
                oal_uint8                      *puc_payload,
                oal_uint16                      ul_msg_len,
                hmac_user_stru                 *pst_hmac_user);
extern  oal_uint32  hmac_check_bss_cap_info_etc(oal_uint16 us_cap_info,mac_vap_stru *pst_mac_vap);

#ifdef _PRE_WLAN_FEATURE_TXBF
extern oal_void hmac_mgmt_update_11ntxbf_cap_etc(oal_uint8 *puc_payload, hmac_user_stru *pst_hmac_user);
#endif
extern  oal_void hmac_set_user_protocol_mode_etc(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user);
extern oal_uint32  hmac_mgmt_reset_psm_etc(mac_vap_stru *pst_vap, oal_uint16 us_user_id);

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
extern oal_void  hmac_rx_sa_query_req_etc(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf, oal_bool_enum_uint8 en_is_protected);
extern oal_void  hmac_rx_sa_query_rsp_etc(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf, oal_bool_enum_uint8 en_is_protected);
#endif
extern oal_void    hmac_send_mgmt_to_host_etc(hmac_vap_stru  *pst_hmac_vap,
                                                oal_netbuf_stru *puc_buf,
                                                oal_uint16       us_len,
                                                oal_int          l_freq);

#if defined(_PRE_WLAN_FEATURE_HS20) || defined(_PRE_WLAN_FEATURE_P2P) || defined(_PRE_WLAN_FEATURE_HILINK) || defined(_PRE_WLAN_FEATURE_11R_AP)
extern oal_void hmac_rx_mgmt_send_to_host_etc(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf);
#endif
#if defined(_PRE_WLAN_FEATURE_LOCATION) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
oal_uint32 hmac_huawei_action_process(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf, oal_uint8 uc_type);
#endif
#ifdef _PRE_WLAN_FEATURE_HS20
extern oal_uint32  hmac_interworking_check(hmac_vap_stru *pst_hmac_vap,  oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_mgmt_tx_event_status_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_void hmac_user_init_rates_etc(hmac_user_stru *pst_hmac_user);
extern oal_uint8 hmac_add_user_rates_etc(hmac_user_stru *pst_hmac_user, oal_uint8 uc_rates_cnt, oal_uint8 *puc_rates);
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
extern oal_void  hmac_rx_ba_session_decr_etc(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_tidno);
extern oal_void  hmac_tx_ba_session_decr_etc(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_tidno);
#else
extern oal_void  hmac_rx_ba_session_decr_etc(mac_device_stru *pst_mac_device, oal_uint8 uc_tidno);
extern oal_void  hmac_tx_ba_session_decr_etc(mac_device_stru *pst_mac_device, oal_uint8 uc_tidno);
#endif
extern oal_void  hmac_vap_set_user_avail_rates_etc(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user);
extern oal_uint32 hmac_proc_ht_cap_ie_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_uint8 *puc_ht_cap_ie);
extern oal_uint32 hmac_proc_vht_cap_ie_etc(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 *puc_vht_cap_ie);
extern oal_uint32 hmac_ie_proc_assoc_user_legacy_rate(oal_uint8 * puc_payload, oal_uint32 us_rx_len, hmac_user_stru * pst_hmac_user);

#ifdef _PRE_WLAN_FEATURE_11AX
extern oal_uint32 hmac_proc_he_cap_ie(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 *puc_he_cap_ie);
extern oal_uint32  hmac_proc_he_bss_color_change_announcement_ie(mac_vap_stru *pst_mac_vap,hmac_user_stru *pst_hmac_user, oal_uint8 *puc_bss_color_ie);

#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
extern oal_uint32  hmac_sta_up_update_rrm_capability(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 *puc_payload, oal_uint32 us_rx_len);
#endif

#ifdef _PRE_WLAN_FEATURE_LOCATION
extern oal_uint32  drv_netlink_location_send(void *buff, oal_uint32  ul_len);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_mgmt_bss_comm.h */
