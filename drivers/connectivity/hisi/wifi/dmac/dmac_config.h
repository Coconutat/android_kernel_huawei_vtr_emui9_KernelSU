

#ifndef __DMAC_CONFIG_H__
#define __DMAC_CONFIG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "frw_ext_if.h"
#include "mac_vap.h"
#ifdef _PRE_WLAN_FEATURE_P2P
#include "dmac_p2p.h"
#endif
#include "dmac_alg_if.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_CONFIG_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define DMAC_HIPRIV_CMD_NAME_MAX_LEN  16                             /* 字符串中每个单词的最大长度 */
#define DMAC_RX_LPF_GAIN  6                             /* 字符串中每个单词的最大长度 */
#define DMAC_DBM_CH  (-13)                             /* 字符串中每个单词的最大长度 */
#define DMAC_SINGLE_DOUBLE_SWITCH_GAIN  (3)                             /* 字符串中每个单词的最大长度 */
#define DMAC_RSSI_REF_DIFFERENCE  (20)                             /* 字符串中每个单词的最大长度 */
#define DMAC_RADAR_REF_DIFFERENCE  (24)                             /* 字符串中每个单词的最大长度 */

#define DMAC_SET_NETBUF_CB(  evt_type, \
                            tx_usr_idx,  \
                            ismcast, \
                            is_bar,  \
                            tid)  do{\
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);\
    OAL_MEMZERO(pst_tx_ctl, OAL_NETBUF_CB_SIZE());\
    MAC_GET_CB_EVENT_TYPE(pst_tx_ctl)               = evt_type;    \
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)             = tx_usr_idx;   \
        MAC_GET_CB_IS_MCAST(pst_tx_ctl)             = ismcast; \
        mac_set_cb_is_bar(pst_tx_ctl, is_bar);               \
        mac_set_cb_tid(pst_tx_ctl, tid);                        \
        mac_set_cb_ac(pst_tx_ctl, WLAN_WME_AC_MGMT);}while(0)

#define DMAC_GET_VAP_RATE(_uc_protocol,_un_nss_rate)                        \
        ((WLAN_VHT_PHY_PROTOCOL_MODE == (_uc_protocol)) ? (_un_nss_rate.st_vht_nss_mcs.bit_vht_mcs):          \
         (WLAN_HT_PHY_PROTOCOL_MODE == (_uc_protocol)) ? (_un_nss_rate.st_ht_rate.bit_ht_mcs):  \
         g_auc_legacy_rate_idx_table[(_un_nss_rate.st_legacy_rate.bit_legacy_rate)])

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
/* 信道与带宽模式映射 */
typedef struct
{
    oal_uint8                           uc_channel;     /* 信道 */
    wlan_channel_bandwidth_enum_uint8   en_bandwidth_40;/* 带宽 */
    wlan_channel_bandwidth_enum_uint8   en_bandwidth_80;/* 带宽 */
    oal_uint8                           auc_resv;
}dmac_config_channel_bw_map_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
extern oal_uint32  dmac_event_config_syn(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  dmac_config_set_qap_cwmin(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_config_set_qap_cwmax(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_config_set_qap_aifsn(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_config_set_qap_txop_limit(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_get_cmd_one_arg(oal_int8 *pc_cmd, oal_int8 *pc_arg, oal_uint32 *pul_cmd_offset);
extern oal_uint32  dmac_config_set_qap_msdu_lifetime(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_SMARTANT
extern oal_uint32  dmac_dual_antenna_set_ant(oal_uint8 uc_param);
extern oal_uint32  dmac_dual_antenna_set_ant_at(oal_uint8 uc_param);
extern oal_uint32  dmac_config_dual_antenna_set_ant(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_config_dual_antenna_vap_check(mac_vap_stru *pst_mac_vap);
#endif
#if 0
extern oal_void  dmac_config_set_wmm_open_cfg(hal_to_dmac_vap_stru *pst_hal_device, mac_wme_param_stru  *pst_wmm);
extern oal_void  dmac_config_set_wmm_close_cfg(hal_to_dmac_vap_stru *pst_hal_device, mac_wme_param_stru  *pst_wmm);
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
extern oal_uint32 dmac_sta_set_ps_enable(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#ifdef _PRE_PSM_DEBUG_MODE
//extern oal_void dmac_pm_info(dmac_vap_stru *pst_dmac_vap);
extern oal_uint32 dmac_show_ps_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif
extern oal_uint32 dmac_config_set_sta_pm_on(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif

#if ((_PRE_OS_VERSION_WIN32_RAW == _PRE_OS_VERSION) ||(_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION) )
extern oal_uint32  dmac_config_set_meta(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
extern  oal_uint32  dmac_config_set_p2p_ps_noa(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern  oal_uint32  dmac_config_set_p2p_ps_ops(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif


#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#ifdef _PRE_DEBUG_MODE
extern  oal_uint32  dmac_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif //#ifdef _PRE_DEBUG_MODE
extern  oal_uint32  dmac_config_set_ip_addr(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#ifdef _PRE_DEBUG_MODE
extern  oal_uint32  dmac_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
extern oal_uint32  dmac_dpd_data_processed_recv(frw_event_mem_stru *pst_event_mem);
extern oal_void dmac_start_dpd_calibration(mac_vap_stru *pst_mac_vap);

#endif
extern oal_uint32  dmac_cali_hmac2dmac_recv(frw_event_mem_stru *pst_event_mem);
extern oal_uint32 dmac_cali_to_hmac(frw_event_mem_stru *pst_event_mem);
extern oal_void  dmac_config_get_tx_rate_info(hal_tx_txop_alg_stru    *pst_tx_alg,
                                       mac_data_rate_stru      *pst_mac_rates_11g,
                                       mac_rate_info_stru      *pst_rate_info);
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
extern oal_void dmac_clear_ip_filter_btable(mac_vap_stru *pst_mac_vap);
extern oal_uint32 dmac_config_update_ip_filter(frw_event_mem_stru *pst_event_mem);
#endif //_PRE_WLAN_FEATURE_IP_FILTER
#ifdef _PRE_WLAN_FEATURE_PKT_MEM_OPT
extern oal_uint32 dmac_pkt_mem_opt_stat_event_process(frw_event_mem_stru *pst_event_mem);
#endif /* */
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
extern oal_void dmac_config_update_rate_pow_table(oal_void);
extern oal_void dmac_config_update_scaling_reg(oal_uint8 uc_device_id);
extern oal_void dmac_config_update_dsss_scaling_reg(dmac_alg_tpc_user_distance_enum_uint8 en_dmac_device_distance_enum);
#endif  /* _PRE_PLAT_FEATURE_CUSTOMIZE */
extern oal_uint32  dmac_send_sys_event(mac_vap_stru *pst_mac_vap,
                                                wlan_cfgid_enum_uint16 en_cfg_id,
                                                oal_uint16 us_len,
                                                oal_uint8 *puc_param);
extern oal_uint32  dmac_config_d2h_user_info_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);


#ifdef _PRE_WLAN_FEATURE_VOWIFI
extern oal_uint32  dmac_config_vowifi_report(dmac_vap_stru *pst_dmac_vap);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */


#ifdef _PRE_WLAN_FEATURE_SMARTANT
extern oal_uint32 dmac_dual_antenna_init(oal_void);
oal_uint32 dmac_config_dual_antenna_check(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_alg_cfg_get_ant_info_notify(mac_vap_stru *pst_vap, oal_uint8 *puc_param,
    oal_uint32 *pul_param1, oal_uint32 *pul_param2, oal_uint32 *pul_param3, oal_uint32 *pul_param4, oal_uint32 *pul_param5);
extern oal_uint32 dmac_config_get_ant_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  dmac_alg_cfg_double_ant_switch_notify(mac_device_stru *pst_mac_device, oal_uint8 uc_param);
extern oal_uint32 dmac_config_double_ant_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_config */
