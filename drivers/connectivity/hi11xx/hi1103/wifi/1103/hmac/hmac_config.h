

#ifndef __HMAC_CONFIG_H__
#define __HMAC_CONFIG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "hmac_device.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CONFIG_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_MAX_MCS_NUM                    8   /* 单流和双流支持的mac最大个数 */
#define HAMC_QUERY_INFO_FROM_DMAC_TIME      (5 * OAL_TIME_HZ)

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
/* hmac_vap结构中，一部分成员的大小，这些成员在linux和windows下的定义可能不同 */
typedef struct
{
    oal_uint32      ul_hmac_vap_cfg_priv_stru_size;
    oal_uint32      ul_frw_timeout_stru_size;
    oal_uint32      ul_oal_spin_lock_stru_size;
    oal_uint32      ul_mac_key_mgmt_stru_size;
    oal_uint32      ul_mac_pmkid_cache_stru_size;
    oal_uint32      ul_mac_curr_rateset_stru_size;
    oal_uint32      ul_hmac_vap_stru_size;
}hmac_vap_member_size_stru;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 hmac_config_start_vap_event_etc(mac_vap_stru  *pst_mac_vap, oal_bool_enum_uint8 en_mgmt_rate_init_flag);
extern oal_uint32 hmac_set_mode_event_etc(mac_vap_stru *pst_mac_vap);
extern oal_uint32  hmac_config_sta_update_rates_etc(mac_vap_stru *pst_mac_vap, mac_cfg_mode_param_stru *pst_cfg_mode, mac_bss_dscr_stru *pst_bss_dscr);
extern oal_uint32  hmac_event_config_syn_etc(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  hmac_event_log_syn(frw_event_mem_stru *pst_event_mem);

extern oal_uint32  hmac_protection_update_from_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_40M_intol_sync_event(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_config_set_tlv_cmd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32 hmac_get_thruput_info_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_void hcc_msg_slave_thruput_bypass_etc(oal_void);
oal_uint32 hmac_config_set_tx_ampdu_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_STA_PM
extern oal_uint32  hmac_config_sta_pm_on_syn(mac_vap_stru *pst_mac_vap);
extern oal_uint32  hmac_set_ipaddr_timeout_etc(void   *puc_para);
#endif
extern oal_uint32 hmac_config_pm_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
extern oal_uint32 hmac_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_ip_addr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM
oal_uint32 hmac_config_roam_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_org_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_band_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_start_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_roam_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_11R
oal_uint32 hmac_config_set_ft_ies_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif //_PRE_WLAN_FEATURE_11R

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
oal_uint32 hmac_config_enable_2040bss_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_get_2040bss_sw(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#endif
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
oal_uint32  hmac_config_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_polynomial_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_polynomial_params(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_cali_power_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_upc_params_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_load_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

oal_uint32  hmac_config_get_dieid_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_dieid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)
oal_uint32 hmac_config_auto_cali(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
oal_uint32  hmac_config_get_cali_status(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_cali_vref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
oal_uint32 hmac_config_set_auto_freq_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_FEATURE_WAVEAPP_CLASSIFY
oal_uint32  hmac_config_get_waveapp_flag_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_get_waveapp_flag(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
oal_uint32  hmac_config_set_sta_pm_on_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_set_sta_pm_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
oal_uint32 hmac_config_load_ini_power_gain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_set_all_log_level_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_set_cus_rf_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_cus_dts_cali_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_cus_nvram_params_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_dev_customize_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_cus_dyn_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)
oal_uint32 hmac_scan_rrm_proc_save_bss_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
extern oal_uint32  hmac_hipriv_proc_write_process_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
extern oal_uint32  hmac_get_rx_pkcg_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint32 ul_param);
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
oal_uint32 hmac_config_set_txrx_chain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 hmac_config_set_2g_txrx_path(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_VOWIFI
extern oal_uint32  hmac_config_vowifi_report_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
extern oal_uint32  hmac_config_stop_altx(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
oal_uint32 hmac_config_user_extend_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_WDS
oal_uint32  hmac_config_wds_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_wds_get_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_wds_vap_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_wds_sta_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_wds_sta_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_wds_sta_age(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_wds_get_sta_num(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_11K_STAT
extern oal_uint32 hmac_config_query_stat_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
extern oal_uint32 hmac_config_send_radio_meas_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
extern oal_uint32 hmac_config_set_11k_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_FTM
extern oal_uint32  hmac_config_ftm_dbg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_APF
extern oal_uint32  hmac_config_apf_filter_cmd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
extern oal_uint32  hmac_config_remove_app_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef  _PRE_WLAN_FEATURE_FORCE_STOP_FILTER
extern oal_uint32  hmac_config_force_stop_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_main */
