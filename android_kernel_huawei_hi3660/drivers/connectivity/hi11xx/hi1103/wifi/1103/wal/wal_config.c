


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "wlan_types.h"

#include "mac_device.h"
#include "mac_vap.h"
#include "mac_resource.h"
//#include "mac_11i.h"

#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"
#include "wal_ext_if.h"
#include "wal_main.h"
#include "wal_config.h"
#include "wal_linux_bridge.h"
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

#include "hmac_cali_mgmt.h"
#endif

#ifdef _PRE_WLAN_CHIP_TEST
#include "hmac_test_main.h"
#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#if ((_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)) || (defined (WIN32) && (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST))
#include "plat_pm_wlan.h"
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
#include "hmac_fbt_main.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/* 静态函数声明 */
OAL_STATIC oal_uint32  wal_config_add_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_del_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_down_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_start_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_bandwidth(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_mac_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_MONITOR
OAL_STATIC oal_uint32  wal_config_get_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_get_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_freq(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_freq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_wmm_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_BTCOEX
OAL_STATIC oal_uint32 wal_config_print_btcoex_status(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_btcoex_preempt_tpye(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_btcoex_set_perf_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#endif
#ifdef _PRE_WLAN_FEATURE_LTECOEX
OAL_STATIC oal_uint32 wal_config_ltecoex_mode_set(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_config_lte_gpio_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wfa_cfg_aifsn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wfa_cfg_cw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#endif

#if 0
OAL_STATIC oal_uint32  wal_config_ota_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_set_random_mac_addr_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_random_mac_oui(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32  wal_config_set_vowifi_nat_keep_alive_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_add_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_del_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_sta_list(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_rd_pwr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
OAL_STATIC oal_uint32  wal_config_reduce_sar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
OAL_STATIC oal_uint32  wal_config_tas_pwr_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
OAL_STATIC oal_uint32  wal_config_tas_rssi_access(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_kick_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_dump_all_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_SMPS
OAL_STATIC oal_uint32  wal_config_set_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
//OAL_STATIC oal_uint32  wal_config_get_smps_mode_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_vap_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32  wal_config_set_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);

#endif

OAL_STATIC oal_uint32  wal_config_set_country(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_country_for_dfs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_country(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_tid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_beacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_mib_by_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

OAL_STATIC oal_uint32 wal_config_add_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_remove_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_default_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_scan_abort(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_cfg80211_start_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_cfg80211_stop_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_cfg80211_start_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_cfg80211_start_join(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_alg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if 0
OAL_STATIC oal_uint32  wal_config_tdls_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_tdls_channel_switch_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_rx_fcs_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_DFS
OAL_STATIC oal_uint32  wal_config_dfs_radartool(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32  wal_config_frag_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_rts_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_VOWIFI
OAL_STATIC oal_uint32  wal_config_vowifi_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
#ifdef _PRE_WLAN_FEATURE_SMARTANT
OAL_STATIC oal_uint32  wal_config_get_ant_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_double_ant_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_IP_FILTER
OAL_STATIC oal_uint32  wal_config_update_ip_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif //_PRE_WLAN_FEATURE_IP_FILTER

OAL_STATIC oal_uint32  wal_config_user_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_dscr_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
//#ifdef    _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  wal_config_set_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_set_pm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_power_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_ant(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_ant(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_set_rate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_mcs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_mcsac(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_11AX
OAL_STATIC oal_uint32  wal_config_set_mcsax(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_set_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
OAL_STATIC oal_uint32  wal_config_always_tx(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
OAL_STATIC oal_uint32  wal_config_always_tx_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_always_tx_hw(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
OAL_STATIC oal_uint32  wal_config_always_tx_51(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
#endif
#endif
OAL_STATIC oal_uint32  wal_config_always_rx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_tx_pow_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
OAL_STATIC oal_uint32  wal_config_always_rx_51(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
#endif
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
OAL_STATIC oal_uint32  wal_config_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_polynomial_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_load_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_get_dieid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_pcie_pm_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_reg_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND
OAL_STATIC oal_uint32  wal_config_set_restrict_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE)
OAL_STATIC oal_uint32  wal_config_set_omit_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
OAL_STATIC oal_uint32  wal_config_sdio_flowctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_reg_write(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);


#ifdef _PRE_WLAN_ONLINE_DPD

OAL_STATIC oal_uint32  wal_config_dpd_cfg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_dpd_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif


#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
OAL_STATIC oal_uint32  wal_config_set_edca_opt_weight_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_edca_opt_switch_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_edca_opt_switch_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_edca_opt_cycle_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32  wal_config_open_wmm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_version(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_bg_noise(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);

OAL_STATIC oal_uint32 wal_config_set_wps_p2p_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
OAL_STATIC oal_uint32  wal_config_blacklist_clr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_blacklist_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_blacklist_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_blacklist_add_only(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_blacklist_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_blacklist_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_blacklist_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
OAL_STATIC oal_uint32  wal_config_get_temp(mac_vap_stru * pst_mac_vap, oal_uint16 *pus_len, oal_uint8 * puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
OAL_STATIC oal_uint32  wal_config_proxyarp_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_DHCP_REQ_DISABLE
OAL_STATIC oal_uint32  wal_config_set_dhcp_req_disable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32 wal_config_set_pmksa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_del_pmksa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_flush_pmksa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_remain_on_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_cancel_remain_on_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_mgmt_tx(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);

OAL_STATIC oal_uint32 wal_config_query_station_stats(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_query_rssi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_query_rate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_query_psst(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32  wal_config_query_ani(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_HS20
OAL_STATIC oal_uint32  wal_config_set_qos_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
OAL_STATIC oal_uint32  wal_config_set_p2p_ps_ops(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_p2p_ps_noa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
OAL_STATIC oal_uint32  wal_config_set_sta_pm_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_PSM_DEBUG_MODE
OAL_STATIC oal_uint32  wal_config_show_pm_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_set_sta_pm_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
OAL_STATIC oal_uint32 wal_config_set_uapsd_para(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32  wal_config_set_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_cfg_vap_h2d(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_host_dev_init(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_host_dev_exit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

oal_uint32 wal_send_cali_data_etc(oal_net_device_stru *pst_net_dev);

#endif

#ifdef _PRE_WLAN_FEATURE_11R
OAL_STATIC oal_uint32  wal_config_set_ft_ies(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif //_PRE_WLAN_FEATURE_11R

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
OAL_STATIC oal_uint32  wal_config_load_ini_power_gain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_all_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_cus_rf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_cus_dts_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_cus_dyn_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_cus_nvram_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_dev_customize_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
OAL_STATIC oal_uint32  wal_config_vap_destroy(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 wal_config_set_vendor_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_FEATURE_HILINK
OAL_STATIC oal_uint32 wal_config_fbt_kick_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_fbt_start_scan(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
oal_uint32 wal_config_set_white_lst_ssidhiden(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 wal_config_get_cur_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32 wal_config_set_stay_time(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
OAL_STATIC oal_uint32 wal_config_set_mgmt_frame_filters(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_sta_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_vap_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_sensing_bssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_get_sensing_bssid_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
OAL_STATIC oal_uint32 wal_config_send_action_frame(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_mgmt_frame_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32 wal_config_set_mgmt_cap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_11R_AP
OAL_STATIC oal_uint32 wal_config_set_mlme(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_11K
OAL_STATIC oal_uint32  wal_config_send_neighbor_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

OAL_STATIC oal_uint32  wal_config_vendor_cmd_get_channel_list(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);

#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)
OAL_STATIC oal_uint32  wal_config_get_cali_staus(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_auto_cali(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param);
OAL_STATIC oal_uint32  wal_config_set_cali_vref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_WDS
OAL_STATIC oal_uint32  wal_config_wds_get_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wds_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wds_vap_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wds_sta_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wds_sta_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wds_sta_age(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_wds_get_sta_num(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_11K_STAT
OAL_STATIC oal_uint32  wal_config_query_stat_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
OAL_STATIC oal_uint32  wal_config_send_radio_meas_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_11k_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
oal_atomic g_wal_config_seq_num_etc = ATOMIC_INIT(0);
oal_module_symbol(g_wal_config_seq_num_etc);
#else
oal_atomic g_wal_config_seq_num_etc = 0;
#endif
#ifdef _PRE_WLAN_CFGID_DEBUG
extern OAL_CONST wal_wid_op_stru g_ast_board_wid_op_debug_etc[];
extern oal_uint32 wal_config_get_debug_wid_arrysize_etc(oal_void);
#endif
#ifdef _PRE_WLAN_FEATURE_GREEN_AP
OAL_STATIC oal_uint32 wal_config_set_green_ap_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif

#ifdef _PRE_WLAN_FEATURE_AP_PM
OAL_STATIC oal_uint32 wal_config_sta_scan_wake_wow(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_get_wmmswitch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);

#ifdef _PRE_WLAN_WEB_CMD_COMM
OAL_STATIC oal_uint32  wal_config_set_hw_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);

OAL_STATIC oal_uint32  wal_config_set_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_priv_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_channel(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_priv_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_assoc_num(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_noise(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_bw(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_pmf(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_band(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_bcast_rate(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
OAL_STATIC oal_uint32  wal_config_set_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_get_ko_version(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_rate_info(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_chan_list(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_vap_cap(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_pwr_ref(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_scan_stat(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_neighbor_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_neighb_no(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_get_ant_rssi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_ant_rssi_report(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
OAL_STATIC oal_uint32  wal_config_set_m2s_switch_blacklist(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  wal_config_set_m2s_switch_mss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifndef CONFIG_HAS_EARLYSUSPEND
OAL_STATIC oal_uint32  wal_config_set_suspend_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_APF
OAL_STATIC oal_uint32  wal_config_apf_filter_cmd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
OAL_STATIC oal_uint32  wal_config_remove_app_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#ifdef  _PRE_WLAN_FEATURE_FORCE_STOP_FILTER
OAL_STATIC oal_uint32  wal_config_force_stop_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
#endif
/* cfgid操作全局变量 */
OAL_STATIC OAL_CONST wal_wid_op_stru g_ast_board_wid_op[] =
{
     /* cfgid                   是否复位mac  保留一字节   get函数              set函数 */
    {WLAN_CFGID_BSS_TYPE,          OAL_TRUE,   {0},   wal_config_get_bss_type,   wal_config_set_bss_type},
    {WLAN_CFGID_ADD_VAP,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_add_vap},
    {WLAN_CFGID_START_VAP,         OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_start_vap},
    {WLAN_CFGID_DEL_VAP,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_del_vap},
    {WLAN_CFGID_DOWN_VAP,          OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_down_vap},
    {WLAN_CFGID_MODE,              OAL_FALSE,  {0},   wal_config_get_mode,       wal_config_set_mode},
    {WLAN_CFGID_BANDWIDTH,         OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_set_bandwidth},

    {WLAN_CFGID_CURRENT_CHANEL,    OAL_FALSE,  {0},   wal_config_get_freq,       wal_config_set_freq},
    {WLAN_CFGID_STATION_ID,        OAL_TRUE,   {0},   OAL_PTR_NULL,              wal_config_set_mac_addr},
    {WLAN_CFGID_SSID,              OAL_FALSE,  {0},   wal_config_get_ssid,       wal_config_set_ssid},
    {WLAN_CFGID_SHORTGI,           OAL_FALSE,  {0},   wal_config_get_shortgi20,  wal_config_set_shortgi20},
    {WLAN_CFGID_SHORTGI_FORTY,     OAL_FALSE,  {0},   wal_config_get_shortgi40,  wal_config_set_shortgi40},
    {WLAN_CFGID_SHORTGI_EIGHTY,    OAL_FALSE,  {0},   wal_config_get_shortgi80,  wal_config_set_shortgi80},

    {WLAN_CFGID_SHORT_PREAMBLE,    OAL_FALSE,  {0},   wal_config_get_shpreamble, wal_config_set_shpreamble},
#ifdef _PRE_WLAN_FEATURE_MONITOR
    {WLAN_CFGID_ADDR_FILTER,       OAL_FALSE,  {0},   wal_config_get_addr_filter,wal_config_set_addr_filter},
#endif
    {WLAN_CFGID_PROT_MODE,         OAL_FALSE,  {0},   wal_config_get_prot_mode,  wal_config_set_prot_mode},
    {WLAN_CFGID_AUTH_MODE,         OAL_FALSE,  {0},   wal_config_get_auth_mode,  wal_config_set_auth_mode},
    {WLAN_CFGID_BEACON_INTERVAL,   OAL_FALSE,  {0},   wal_config_get_bintval,    wal_config_set_bintval},
    {WLAN_CFGID_NO_BEACON,         OAL_FALSE,  {0},   wal_config_get_nobeacon,   wal_config_set_nobeacon},
    {WLAN_CFGID_TX_POWER,          OAL_FALSE,  {0},   wal_config_get_txpower,    wal_config_set_txpower},
#ifdef _PRE_WLAN_FEATURE_SMPS
    {WLAN_CFGID_SMPS_MODE,         OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_set_smps_mode},
//    {WLAN_CFGID_SMPS_EN,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_get_smps_mode_en},
    {WLAN_CFGID_SMPS_VAP_MODE,     OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_set_vap_smps_mode},
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
    {WLAN_CFGID_UAPSD_EN,          OAL_FALSE,  {0},   wal_config_get_uapsd_en,   wal_config_set_uapsd_en},
#endif
    {WLAN_CFGID_DTIM_PERIOD,       OAL_FALSE,  {0},   wal_config_get_dtimperiod, wal_config_set_dtimperiod},

    {WLAN_CFGID_EDCA_TABLE_CWMIN,          OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_EDCA_TABLE_CWMAX,          OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_EDCA_TABLE_AIFSN,          OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT,     OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME,  OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_EDCA_TABLE_MANDATORY,      OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_QEDCA_TABLE_CWMIN,         OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_QEDCA_TABLE_CWMAX,         OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_QEDCA_TABLE_AIFSN,         OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT,    OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME, OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},
    {WLAN_CFGID_QEDCA_TABLE_MANDATORY,     OAL_FALSE,  {0},   OAL_PTR_NULL,      wal_config_set_wmm_params},

    {WLAN_CFGID_VAP_INFO,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_vap_info},
#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
    {WLAN_CFGID_SYNC_PK_MODE,           OAL_FALSE,  {0},    OAL_PTR_NULL,            hmac_config_pk_mode_debug},
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    {WLAN_CFGID_BTCOEX_STATUS_PRINT,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_print_btcoex_status},
    {WLAN_CFGID_BTCOEX_PREEMPT_TYPE,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_btcoex_preempt_tpye},
    {WLAN_CFGID_BTCOEX_SET_PERF_PARAM,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_btcoex_set_perf_param},
#endif
#ifdef _PRE_WLAN_FEATURE_LTECOEX
    {WLAN_CFGID_LTECOEX_MODE_SET,    	OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ltecoex_mode_set},
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_WFA_CFG_AIFSN,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_wfa_cfg_aifsn},
    {WLAN_CFGID_WFA_CFG_CW,        OAL_FALSE,  {0},    OAL_PTR_NULL,                wal_config_wfa_cfg_cw},
    {WLAN_CFGID_CHECK_LTE_GPIO,              OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_lte_gpio_mode},

#endif
#if 0
    {WLAN_CFGID_OTA_SWITCH,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ota_switch},
#endif
    {WLAN_CFGID_SET_RANDOM_MAC_ADDR_SCAN, OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_set_random_mac_addr_scan},
    {WLAN_CFGID_SET_RANDOM_MAC_OUI,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_random_mac_oui},
    {WLAN_CFGID_ADD_USER,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_add_user},
    {WLAN_CFGID_DEL_USER,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_del_user},
    {WLAN_CFGID_SET_LOG_LEVEL,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_log_level},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_vowifi_nat_keep_alive_params},
#endif
#ifdef _PRE_WLAN_FEATURE_GREEN_AP
    {WLAN_CFGID_GREEN_AP_EN,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_green_ap_en},
#endif
#ifdef _PRE_WLAN_FEATURE_AP_PM
    {WLAN_CFGID_STA_SCAN_CONNECT,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_sta_scan_wake_wow},
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_SET_PM_SWITCH,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_pm_switch},
    {WLAN_CFGID_SET_POWER_TEST,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_power_test},
    {WLAN_CFGID_SET_ANT,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_ant},
    {WLAN_CFGID_GET_ANT,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_ant},
#endif
    {WLAN_CFGID_DUMP_ALL_RX_DSCR,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_all_rx_dscr},
    {WLAN_CFGID_KICK_USER,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_kick_user},
    {WLAN_CFGID_COUNTRY,                OAL_FALSE,  {0},    wal_config_get_country,  wal_config_set_country},
    {WLAN_CFGID_COUNTRY_FOR_DFS,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_country_for_dfs},
    {WLAN_CFGID_TID,                    OAL_FALSE,  {0},    wal_config_get_tid,      OAL_PTR_NULL},


#if 0
    {WLAN_CFGID_TDLS_PROHI,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_tdls_prohibited},
    {WLAN_CFGID_TDLS_CHASWI_PROHI,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_tdls_channel_switch_prohibited},
#endif
    {WLAN_CFGID_FRAG_THRESHOLD_REG,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_frag_threshold},
    {WLAN_CFGID_RX_FCS_INFO,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_rx_fcs_info},

#ifdef _PRE_WLAN_FEATURE_DFS
    {WLAN_CFGID_RADARTOOL,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dfs_radartool},
#endif

    {WLAN_CFGID_REGDOMAIN_PWR,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_rd_pwr},
    {WLAN_CFGID_USER_INFO,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_user_info},
    {WLAN_CFGID_SET_DSCR,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_dscr_param},
/*#ifdef _PRE_WLAN_CHIP_TEST  */
    {WLAN_CFGID_SET_RATE,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_rate},
    {WLAN_CFGID_SET_MCS,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_mcs},
    {WLAN_CFGID_SET_MCSAC,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_mcsac},
#ifdef _PRE_WLAN_FEATURE_11AX
    {WLAN_CFGID_SET_MCSAX,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_mcsax},
#endif
    {WLAN_CFGID_SET_BW,                 OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_bw},
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    {WLAN_CFGID_SET_ALWAYS_TX,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_tx},
    {WLAN_CFGID_SET_ALWAYS_TX_NUM,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_tx_num},
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    {WLAN_CFGID_SET_ALWAYS_TX_51,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_tx_51},
#endif
    {WLAN_CFGID_SET_ALWAYS_TX_HW,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_tx_hw},
#endif
    {WLAN_CFGID_SET_ALWAYS_RX,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_rx},
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    {WLAN_CFGID_SET_ALWAYS_RX_51,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_rx_51},
#endif
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
    {WLAN_CFGID_CALI_POWER,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_cali_power},
    {WLAN_CFGID_GET_CALI_POWER,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_cali_power},
    {WLAN_CFGID_SET_POLYNOMIAL_PARA,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_polynomial_param},
    {WLAN_CFGID_GET_UPC_PARA,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_upc_params},
    {WLAN_CFGID_SET_UPC_PARA,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_upc_params},
    {WLAN_CFGID_SET_LOAD_MODE,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_load_mode},
#endif
    {WLAN_CFGID_GET_DIEID,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_dieid},
    {WLAN_CFGID_PCIE_PM_LEVEL,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_pcie_pm_level},
    {WLAN_CFGID_SET_TX_POW,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_tx_pow_param},
    {WLAN_CFGID_REG_INFO,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_reg_info},
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
    {WLAN_CFGID_SDIO_FLOWCTRL,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_sdio_flowctrl},
#endif
    {WLAN_CFGID_REG_WRITE,               OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_reg_write},
#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND
    {WLAN_CFGID_SET_RESTRICT_BAND,       OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_set_restrict_band},
#endif
#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE)
    {WLAN_CFGID_SET_OMIT_ACS,            OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_set_omit_acs},
#endif
#ifdef _PRE_WLAN_ONLINE_DPD
    {WLAN_CFGID_DPD,               OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_dpd_cfg},
    {WLAN_CFGID_DPD_START,         OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_dpd_start},
#endif
    {WLAN_CFGID_SCAN_ABORT,               OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_scan_abort},
    /* 以下为内核cfg80211配置的命令 */
    {WLAN_CFGID_CFG80211_START_SCHED_SCAN,OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_cfg80211_start_sched_scan},
    {WLAN_CFGID_CFG80211_STOP_SCHED_SCAN, OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_cfg80211_stop_sched_scan},
    {WLAN_CFGID_CFG80211_START_SCAN,      OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_cfg80211_start_scan},
    {WLAN_CFGID_CFG80211_START_CONNECT,   OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_cfg80211_start_join},
    {WLAN_CFGID_CFG80211_SET_CHANNEL,     OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_set_channel},
    {WLAN_CFGID_CFG80211_SET_MIB_BY_BW,   OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_set_mib_by_bw},
    {WLAN_CFGID_CFG80211_CONFIG_BEACON,   OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_set_beacon},


    {WLAN_CFGID_ADD_KEY,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_add_key},
    {WLAN_CFGID_GET_KEY,           OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_get_key},
    {WLAN_CFGID_REMOVE_KEY,        OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_remove_key},

    {WLAN_CFGID_ALG_PARAM,         OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_alg_param},


#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    {WLAN_CFGID_EDCA_OPT_SWITCH_STA,  OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_set_edca_opt_switch_sta},
    {WLAN_CFGID_EDCA_OPT_WEIGHT_STA,  OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_set_edca_opt_weight_sta},
    {WLAN_CFGID_EDCA_OPT_SWITCH_AP,   OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_set_edca_opt_switch_ap},
    {WLAN_CFGID_EDCA_OPT_CYCLE_AP,    OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_set_edca_opt_cycle_ap},
#endif

    /* START:开源APP 程序下发的私有命令 */
    {WLAN_CFGID_GET_ASSOC_REQ_IE,  OAL_FALSE,  {0},   wal_config_get_assoc_req_ie_etc,   OAL_PTR_NULL},
    {WLAN_CFGID_SET_WPS_IE,        OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_set_wps_ie},
    {WLAN_CFGID_SET_RTS_THRESHHOLD,OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_rts_threshold},
#ifdef _PRE_WLAN_FEATURE_VOWIFI
    {WLAN_CFGID_VOWIFI_INFO,       OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_vowifi_info},
#endif
#ifdef _PRE_WLAN_FEATURE_IP_FILTER
    {WLAN_CFGID_IP_FILTER,       OAL_FALSE,  {0},   OAL_PTR_NULL,               wal_config_update_ip_filter},
#endif //_PRE_WLAN_FEATURE_IP_FILTER

#ifdef _PRE_WLAN_FEATURE_SMARTANT
    {WLAN_CFGID_GET_ANT_INFO,           OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_get_ant_info},
    {WLAN_CFGID_DOUBLE_ANT_SW,          OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_double_ant_switch},
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
    {WLAN_CFGID_FBT_KICK_USER,              OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_fbt_kick_user},
    {WLAN_CFGID_SET_WHITE_LIST_SSIDHIDEN,   OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_white_lst_ssidhiden},
    {WLAN_CFGID_FBT_FET_CURRENT_CHANNEL,    OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_cur_channel},
    {WLAN_CFGID_FBT_SET_SCAN_TIME_PARAM,    OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_stay_time},
    {WLAN_CFGID_FBT_GET_STA_11V_ABILITY,    OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_sta_11v_abillty},
    {WLAN_CFGID_FBT_GET_STA_11K_ABILITY,    OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_sta_11k_abillty},
    {WLAN_CFGID_FBT_CHANGE_STA_TO_TARGET_AP,OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_change_to_other_ap},
    {WLAN_CFGID_FBT_NEIGHBOR_BCN_REQ,       OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_sta_bcn_request},
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    {WLAN_CFGID_SET_MGMT_FRAME_FILTERS,     OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_mgmt_frame_filters},
    {WLAN_CFGID_GET_STA_DIAG_INFO,          OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_sta_diag_info},
    {WLAN_CFGID_GET_VAP_DIAG_INFO,          OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_vap_diag_info},
    {WLAN_CFGID_SET_SENSING_BSSID,          OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_sensing_bssid},
    {WLAN_CFGID_GET_SENSING_BSSID_INFO,     OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_sensing_bssid_info},
#endif
#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
    {WLAN_CFGID_SEND_ACTION_FRAME,          OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_send_action_frame},
    {WLAN_CFGID_SET_MGMT_FRAME_IE,          OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_mgmt_frame_ie},
    {WLAN_CFGID_SET_MGMT_CAP_INFO,          OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_mgmt_cap_info},
#endif
    {WLAN_CFGID_FBT_GET_STA_11H_ABILITY,    OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_sta_11h_abillty},
    {WLAN_CFGID_SET_VENDOR_IE,                 OAL_FALSE,  {0},   OAL_PTR_NULL,     wal_config_set_vendor_ie},

    /* END:开源APP 程序下发的私有命令 */
#ifdef _PRE_WLAN_FEATURE_11R_AP
    {WLAN_CFGID_FBT_GET_STA_11R_ABILITY,    OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_get_sta_11r_abillty},
    {WLAN_CFGID_SET_MLME,          OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_set_mlme},
#endif

    {WLAN_CFGID_DEFAULT_KEY,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_default_key},
    {WLAN_CFGID_WMM_SWITCH,         OAL_FALSE,  {0},   wal_config_get_wmmswitch,wal_config_open_wmm},
    {WLAN_CFGID_VAP_WMM_SWITCH,     OAL_FALSE,  {0},   wal_config_get_vap_wmm_switch,wal_config_set_vap_wmm_switch},
    {WLAN_CFGID_GET_VERSION,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_version},
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
    /* 黑名单配置 */
    {WLAN_CFGID_CLR_BLACK_LIST,     OAL_FALSE,  {0},   OAL_PTR_NULL,                    wal_config_blacklist_clr},
    {WLAN_CFGID_ADD_BLACK_LIST,     OAL_FALSE,  {0},   OAL_PTR_NULL,                    wal_config_blacklist_add},
    {WLAN_CFGID_DEL_BLACK_LIST,     OAL_FALSE,  {0},   OAL_PTR_NULL,                    wal_config_blacklist_del},
    {WLAN_CFGID_BLACKLIST_MODE,     OAL_FALSE,  {0},   wal_config_get_blacklist_mode,   wal_config_set_blacklist_mode},
    {WLAN_CFGID_BLACKLIST_SHOW          ,OAL_FALSE,  {0},   OAL_PTR_NULL,               wal_config_blacklist_show},
    {WLAN_CFGID_ADD_BLACK_LIST_ONLY    ,OAL_FALSE,  {0},   OAL_PTR_NULL,                wal_config_blacklist_add_only},
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    {WLAN_CFGID_PROXYARP_EN             ,OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_proxyarp_en},
#endif
    {WLAN_CFGID_CFG80211_SET_PMKSA,        OAL_FALSE,  {0},    OAL_PTR_NULL,     wal_config_set_pmksa},
    {WLAN_CFGID_CFG80211_DEL_PMKSA,        OAL_FALSE,  {0},    OAL_PTR_NULL,     wal_config_del_pmksa},
    {WLAN_CFGID_CFG80211_FLUSH_PMKSA,      OAL_FALSE,  {0},    OAL_PTR_NULL,     wal_config_flush_pmksa},

    {WLAN_CFGID_SET_WPS_P2P_IE,                    OAL_FALSE,  {0},    OAL_PTR_NULL,     wal_config_set_wps_p2p_ie},
    {WLAN_CFGID_CFG80211_REMAIN_ON_CHANNEL,        OAL_FALSE,  {0},    OAL_PTR_NULL,     wal_config_remain_on_channel},
    {WLAN_CFGID_CFG80211_CANCEL_REMAIN_ON_CHANNEL, OAL_FALSE,  {0},    OAL_PTR_NULL,     wal_config_cancel_remain_on_channel},
    {WLAN_CFGID_CFG80211_MGMT_TX,   OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_mgmt_tx},

    {WLAN_CFGID_QUERY_STATION_STATS,   OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_query_station_stats},

    {WLAN_CFGID_QUERY_RSSI,             OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_query_rssi},
    {WLAN_CFGID_QUERY_RATE,             OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_query_rate},
    {WLAN_CFGID_QUERY_PSST,             OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_query_psst},
#ifdef _PRE_WLAN_DFT_STAT
    {WLAN_CFGID_QUERY_ANI,              OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_query_ani},
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
    {WLAN_CFGID_SET_PS_MODE,           OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_set_sta_pm_mode},
#ifdef _PRE_PSM_DEBUG_MODE
    {WLAN_CFGID_SHOW_PS_INFO,          OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_show_pm_info},
#endif
    {WLAN_CFGID_SET_STA_PM_ON,          OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_set_sta_pm_on},
#endif

#ifdef _PRE_WLAN_FEATURE_DHCP_REQ_DISABLE
     {WLAN_CFGID_DHCP_REQ_DISABLE_SWITCH,          OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_set_dhcp_req_disable},
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
    {WLAN_CFGID_SET_UAPSD_PARA,        OAL_FALSE, {0},      OAL_PTR_NULL,       wal_config_set_uapsd_para},
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    {WLAN_CFGID_SET_P2P_PS_OPS,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_p2p_ps_ops},
    {WLAN_CFGID_SET_P2P_PS_NOA,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_p2p_ps_noa},
#endif
#ifdef _PRE_WLAN_FEATURE_HS20
    {WLAN_CFGID_SET_QOS_MAP,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_qos_map},
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_CFG_VAP_H2D,  OAL_FALSE,    {0},    OAL_PTR_NULL,            wal_config_cfg_vap_h2d},
    {WLAN_CFGID_HOST_DEV_INIT,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_host_dev_init},
    {WLAN_CFGID_HOST_DEV_EXIT,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_host_dev_exit},
#endif
    {WLAN_CFGID_SET_MAX_USER,        OAL_FALSE,  {0},    wal_config_get_max_user, wal_config_set_max_user},
    {WLAN_CFGID_GET_STA_LIST,        OAL_FALSE,  {0},    wal_config_get_sta_list, OAL_PTR_NULL},
#ifdef _PRE_WLAN_FEATURE_11R
    {WLAN_CFGID_SET_FT_IES,  OAL_FALSE,  {0},    OAL_PTR_NULL,             wal_config_set_ft_ies},
#endif //_PRE_WLAN_FEATURE_11R

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    {WLAN_CFGID_SET_CUS_DYN_CALI_PARAM,    OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_cus_dyn_cali},
    {WLAN_CFGID_LOAD_INI_PWR_GAIN,         OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_load_ini_power_gain},
    {WLAN_CFGID_SET_ALL_LOG_LEVEL,         OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_all_log_level},
    {WLAN_CFGID_SET_CUS_RF,                OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_cus_rf},
    {WLAN_CFGID_SET_CUS_DTS_CALI,          OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_cus_dts_cali},
    {WLAN_CFGID_SET_CUS_NVRAM_PARAM,       OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_set_cus_nvram_params},
    /* SHOW DEIVCE CUSTOMIZE INFOS */
    {WLAN_CFGID_SHOW_DEV_CUSTOMIZE_INFOS,  OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_dev_customize_info},
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

    {WLAN_CFGID_DESTROY_VAP,     OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_vap_destroy},
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
    {WLAN_CFGID_REDUCE_SAR,      OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_reduce_sar},
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    {WLAN_CFGID_TAS_PWR_CTRL,    OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_tas_pwr_ctrl},
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    {WLAN_CFGID_TAS_RSSI_ACCESS, OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_tas_rssi_access},
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
    {WLAN_CFGID_FBT_START_SCAN,  OAL_FALSE,  {0},    OAL_PTR_NULL,      wal_config_fbt_start_scan},
#endif

#ifdef _PRE_WLAN_FEATURE_11K
    {WLAN_CFGID_SEND_NEIGHBOR_REQ,   OAL_FALSE,  {0},  OAL_PTR_NULL,     wal_config_send_neighbor_req},
#endif

    {WLAN_CFGID_VENDOR_CMD_GET_CHANNEL_LIST,   OAL_FALSE,  {0},  wal_config_vendor_cmd_get_channel_list, OAL_PTR_NULL},

#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)
    {WLAN_CFGID_AUTO_CALI,       OAL_FALSE,  {0},    wal_config_get_cali_staus,         wal_config_auto_cali},
    {WLAN_CFGID_SET_CALI_VREF,   OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_set_cali_vref},
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    {WLAN_CFGID_LOAD_INI_PWR_GAIN, OAL_FALSE,  {0},  OAL_PTR_NULL,           wal_config_load_ini_power_gain},
#endif

#ifdef _PRE_WLAN_FEATURE_WDS
    {WLAN_CFGID_WDS_VAP_MODE,               OAL_FALSE,  {0},    wal_config_wds_get_vap_mode,        wal_config_wds_vap_mode},
    {WLAN_CFGID_WDS_VAP_SHOW,               OAL_FALSE,  {0},    OAL_PTR_NULL,                       wal_config_wds_vap_show},
    {WLAN_CFGID_WDS_STA_ADD,                OAL_FALSE,  {0},    OAL_PTR_NULL,                       wal_config_wds_sta_add},
    {WLAN_CFGID_WDS_STA_DEL,                OAL_FALSE,  {0},    OAL_PTR_NULL,                       wal_config_wds_sta_del},
    {WLAN_CFGID_WDS_STA_AGE,                OAL_FALSE,  {0},    OAL_PTR_NULL,                       wal_config_wds_sta_age},
    {WLAN_CFGID_GET_WDS_STA_NUM,            OAL_FALSE,  {0},    wal_config_wds_get_sta_num,                       OAL_PTR_NULL},
#endif

#ifdef _PRE_WLAN_11K_STAT
    {WLAN_CFGID_QUERY_STAT_INFO,             OAL_FALSE,  {0},  OAL_PTR_NULL,         wal_config_query_stat_info},
#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
    {WLAN_CFGID_SET_HWADDR,                 OAL_FALSE,  {0},    OAL_PTR_NULL,                   wal_config_set_hw_addr},
    {WLAN_CFGID_HIDE_SSID,                  OAL_FALSE,  {0},    wal_config_get_hide_ssid,       wal_config_set_hide_ssid},
    {WLAN_CFGID_GLOBAL_SHORTGI,             OAL_FALSE,  {0},    wal_config_get_global_shortgi,  wal_config_set_global_shortgi},
    {WLAN_CFGID_SET_TXBEAMFORM,             OAL_FALSE,  {0},    wal_config_get_txbeamform,      wal_config_set_txbeamform},
    {WLAN_CFGID_SET_NOFORWARD,              OAL_FALSE,  {0},    wal_config_get_noforward,       wal_config_set_noforward},
    {WLAN_CFGID_PRIV_MODE,                  OAL_FALSE,  {0},    OAL_PTR_NULL,                   wal_config_priv_set_mode},
    {WLAN_CFGID_CHANNEL,                    OAL_FALSE,  {0},    wal_config_get_channel,         wal_config_priv_set_channel},
    {WLAN_CFGID_ASSOC_NUM,                  OAL_FALSE,  {0},    wal_config_get_assoc_num,       OAL_PTR_NULL},
    {WLAN_CFGID_NOISE,                      OAL_FALSE,  {0},    wal_config_get_noise,           OAL_PTR_NULL},
    {WLAN_CFGID_GET_BW,                     OAL_FALSE,  {0},    wal_config_get_bw,              OAL_PTR_NULL},
    {WLAN_CFGID_PMF,                        OAL_FALSE,  {0},    wal_config_get_pmf,             OAL_PTR_NULL},
    {WLAN_CFGID_GET_TEMP,                   OAL_FALSE,  {0},    wal_config_get_temp,            OAL_PTR_NULL},
    {WLAN_CFGID_GET_BCAST_RATE,             OAL_FALSE,  {0},    wal_config_get_bcast_rate,      OAL_PTR_NULL},
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    {WLAN_CFGID_BSD_CONFIG,                 OAL_FALSE,  {0},    OAL_PTR_NULL,                   wal_config_set_bsd},
    {WLAN_CFGID_GET_BSD,                    OAL_FALSE,  {0},    wal_config_get_bsd,             OAL_PTR_NULL},
#endif
    {WLAN_CFGID_GET_KO_VERSION,             OAL_FALSE,  {0},    wal_config_get_ko_version,      OAL_PTR_NULL},
    {WLAN_CFGID_GET_WPS_IE,                 OAL_FALSE,  {0},    wal_config_get_wps_ie,          OAL_PTR_NULL},
    {WLAN_CFGID_GET_BAND,                   OAL_FALSE,  {0},    wal_config_get_band,            OAL_PTR_NULL},
    {WLAN_CFGID_GET_RATE_INFO,              OAL_FALSE,  {0},    wal_config_get_rate_info,       OAL_PTR_NULL},
    {WLAN_CFGID_GET_CHAN_LIST,              OAL_FALSE,  {0},    wal_config_get_chan_list,       OAL_PTR_NULL},
    {WLAN_CFGID_GET_VAP_CAP,                OAL_FALSE,  {0},    wal_config_get_vap_cap,         OAL_PTR_NULL},
    {WLAN_CFGID_GET_PWR_REF,                OAL_FALSE,  {0},    wal_config_get_pwr_ref,         OAL_PTR_NULL},
    {WLAN_CFGID_NEIGHBOR_SCAN,              OAL_FALSE,  {0},    wal_config_get_scan_stat,       wal_config_neighbor_scan},
    {WLAN_CFGID_GET_NEIGHB_NO,              OAL_FALSE,  {0},    wal_config_get_neighb_no,       OAL_PTR_NULL},
    {WLAN_CFGID_GET_ANT_RSSI,               OAL_FALSE,  {0},    wal_config_get_ant_rssi,       OAL_PTR_NULL},
    {WLAN_CFGID_ANT_RSSI_REPORT,            OAL_FALSE,  {0},    OAL_PTR_NULL,       wal_config_ant_rssi_report},
#endif
    {WLAN_CFGID_GET_BG_NOISE,               OAL_FALSE,  {0},    wal_config_get_bg_noise,       OAL_PTR_NULL},
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    {WLAN_CFGID_SEND_RADIO_MEAS_REQ,        OAL_FALSE,  {0},  OAL_PTR_NULL,         wal_config_send_radio_meas_req},
    {WLAN_CFGID_SET_11K_SWITCH,             OAL_FALSE,  {0},  OAL_PTR_NULL,         wal_config_set_11k_switch},
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    {WLAN_CFGID_SET_M2S_BLACKLIST,          OAL_FALSE,  {0},  OAL_PTR_NULL,         wal_config_set_m2s_switch_blacklist},
    {WLAN_CFGID_SET_M2S_MSS,                OAL_FALSE,  {0},  OAL_PTR_NULL,         wal_config_set_m2s_switch_mss},
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifndef CONFIG_HAS_EARLYSUSPEND
    {WLAN_CFGID_SET_SUSPEND_MODE,           OAL_FALSE,  {0},  OAL_PTR_NULL,         wal_config_set_suspend_mode},
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_APF
    {WLAN_CFGID_SET_APF_FILTER,            OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_apf_filter_cmd},
#endif
    {WLAN_CFGID_REMOVE_APP_IE,             OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_remove_app_ie},
#ifdef  _PRE_WLAN_FEATURE_FORCE_STOP_FILTER
    {WLAN_CFGID_FORCE_STOP_FILTER,         OAL_FALSE,  {0},   OAL_PTR_NULL,         wal_config_force_stop_filter},
#endif

    {WLAN_CFGID_BUTT,                  OAL_FALSE,  {0},    0,                       0},
};

/*****************************************************************************
  3 函数实现
*****************************************************************************/

OAL_STATIC oal_uint32  wal_config_add_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32 ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_add_vap_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        return ul_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_config_del_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32         ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_del_vap::pst_mac_vap or puc_param null ptr error [%d], [%d]}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_del_vap_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_del_vap_etc:: return error code [%d].}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_start_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_start_vap::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_start_vap_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_start_vap:: return error code %d.}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_config_down_vap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_down_vap::pst_mac_vap or puc_param null ptr error [%x],[%x].}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_down_vap_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_down_vap:: return error code [%d].}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_set_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_bss_type_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_bss_type(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_bss_type_etc(pst_mac_vap, pus_len,  puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_mode_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mib_by_bw_param_stru st_cfg;

    if (!pst_mac_vap || !puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{wal_config_set_mode::null ptr, vap=%p param=%p", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_cfg.en_band = ((mac_cfg_mode_param_stru *)puc_param)->en_band;
    st_cfg.en_bandwidth= ((mac_cfg_mode_param_stru *)puc_param)->en_bandwidth;

    hmac_config_set_mib_by_bw(pst_mac_vap, (oal_uint16)OAL_SIZEOF(st_cfg), (oal_uint8 *)&st_cfg);

    return hmac_config_set_mode_etc(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_set_bandwidth(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mode_param_stru    *pst_prot_param;

    pst_prot_param = (mac_cfg_mode_param_stru *)puc_param;

    pst_prot_param->en_protocol  = pst_mac_vap->en_protocol;
    pst_prot_param->en_band      = pst_mac_vap->st_channel.en_band;

    return hmac_config_set_mode_etc(pst_mac_vap, us_len, (oal_uint8 *)pst_prot_param);
}


OAL_STATIC oal_uint32  wal_config_set_mac_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                     ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_set_mac_addr::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_mac_addr_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_mac_addr_etc:: return error code %d.}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_get_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_ssid_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_ssid_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_shpreamble_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_shpreamble(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shpreamble_etc(pst_mac_vap, pus_len, puc_param);
}
#ifdef _PRE_WLAN_FEATURE_MONITOR

OAL_STATIC oal_uint32  wal_config_set_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_addr_filter(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_addr_filter_etc(pst_mac_vap, pus_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_set_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_shortgi20_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_shortgi40_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_shortgi80_etc(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_get_shortgi20(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shortgi20_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_shortgi40(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shortgi40_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_shortgi80(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_shortgi80_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_prot_mode_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_prot_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_prot_mode_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_auth_mode_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_auth_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_auth_mode_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_bintval_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_bintval(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_bintval_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_dtimperiod_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_dtimperiod(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_dtimperiod_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_nobeacon_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_nobeacon(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_nobeacon_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_txpower_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_txpower_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_freq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_freq_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_freq(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_freq_etc(pst_mac_vap, pus_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_set_wmm_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    return hmac_config_set_wmm_params_etc(pst_mac_vap, us_len, puc_param);
}




oal_uint32  wal_config_get_wmm_params_etc(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_param)
{
    mac_vap_stru               *pst_vap;

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_wmm_params_etc::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_config_get_wmm_params_etc(pst_vap, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_SMPS
OAL_STATIC oal_uint32  wal_config_set_vap_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_vap_smps_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_smps_mode(pst_mac_vap, us_len, puc_param);
}
#if 0

OAL_STATIC oal_uint32  wal_config_get_smps_mode_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_smps_mode_en(pst_mac_vap, us_len, puc_param);
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD


OAL_STATIC oal_uint32  wal_config_set_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_uapsden_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_uapsd_en(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{

    return hmac_config_get_uapsden_etc(pst_mac_vap, pus_len, puc_param);
}

#endif


OAL_STATIC oal_uint32  wal_config_set_mib_by_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_mib_by_bw(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mib_by_bw_param_stru st_cfg;

    if (!pst_mac_vap || !puc_param)
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{wal_config_set_channel::null ptr, vap=%p param=%p", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_cfg.en_band = ((mac_cfg_channel_param_stru *)puc_param)->en_band;
    st_cfg.en_bandwidth= ((mac_cfg_channel_param_stru *)puc_param)->en_bandwidth;

    hmac_config_set_mib_by_bw(pst_mac_vap, (oal_uint16)OAL_SIZEOF(st_cfg), (oal_uint8 *)&st_cfg);

    return hmac_config_set_channel_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_beacon(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                          ul_ret;


    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap) || OAL_UNLIKELY(OAL_PTR_NULL == puc_param) )
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_set_beacon::pst_mac_vap or puc_param is null [%x],[%x].}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_beacon_etc(pst_mac_vap, us_len, puc_param);

    return ul_ret;
}


OAL_STATIC oal_uint32  wal_config_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_vap_info_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_BTCOEX

OAL_STATIC oal_uint32 wal_config_print_btcoex_status(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_print_btcoex_status_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_btcoex_preempt_tpye(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_btcoex_preempt_tpye(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_btcoex_set_perf_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_btcoex_set_perf_param(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_FEATURE_LTECOEX

OAL_STATIC oal_uint32 wal_config_ltecoex_mode_set(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_ltecoex_mode_set(pst_mac_vap, us_len, puc_param);
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)


OAL_STATIC oal_uint32  wal_config_wfa_cfg_aifsn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wfa_cfg_aifsn_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_wfa_cfg_cw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wfa_cfg_cw_etc(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32 wal_config_lte_gpio_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_lte_gpio_mode_etc(pst_mac_vap, us_len, puc_param);
}

#endif


OAL_STATIC oal_uint32  wal_config_set_random_mac_addr_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_random_mac_addr_scan_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_random_mac_oui(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_random_mac_oui_etc(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_config_set_vowifi_nat_keep_alive_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_vowifi_nat_keep_alive_params(pst_mac_vap, us_len, puc_param);
}
#endif



OAL_STATIC oal_uint32  wal_config_add_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_add_user_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_del_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_del_user_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_sta_list(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_sta_list_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_rd_pwr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_regdomain_pwr_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_TPC_OPT

OAL_STATIC oal_uint32  wal_config_reduce_sar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_reduce_sar_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH

OAL_STATIC oal_uint32  wal_config_tas_pwr_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_tas_pwr_ctrl(pst_mac_vap, us_len, puc_param);
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH

OAL_STATIC oal_uint32  wal_config_tas_rssi_access(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_tas_rssi_access(pst_mac_vap, us_len, puc_param);
}
#endif


oal_uint32  wal_config_dump_all_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dump_all_rx_dscr_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_cfg80211_start_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_cfg80211_start_sched_scan_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_cfg80211_stop_sched_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_cfg80211_stop_sched_scan_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_scan_abort(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_scan_abort_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_cfg80211_start_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_cfg80211_start_scan_sta_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_cfg80211_start_join(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_connect_etc(pst_mac_vap, us_len, puc_param);
}


oal_netbuf_stru*  wal_config_create_packet_sta_etc(oal_uint32 ul_size,
                                                oal_int32 l_reserve,
                                                oal_uint32 ul_put_len,
                                                oal_bool_enum_uint8 en_ismcast,
                                                oal_uint8 uc_tid)
{
    oal_netbuf_stru *pst_buf;
    mac_ether_header_stru  *pst_ether_header;
    mac_ip_header_stru     *pst_ip;
    oal_uint32             ul_loop = 0;

    pst_buf = oal_netbuf_alloc(ul_size, l_reserve, 4);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_buf))
    {
        return OAL_PTR_NULL;
    }

    oal_netbuf_put(pst_buf, ul_put_len);

    if (en_ismcast)
    {
        pst_buf->data[0] = 0xFF;
    }
    else
    {
        pst_buf->data[0] = 0x28;
    }

    pst_buf->data[0] = 0x28;
    pst_buf->data[1] = 0x6E;
    pst_buf->data[2] = 0xD4;
    pst_buf->data[3] = 0x89;
    pst_buf->data[4] = 0x00;
    pst_buf->data[5] = 0xC2;

    pst_buf->data[6] = 0x06;
    pst_buf->data[7] = 0x05;
    pst_buf->data[8] = 0x04;
    pst_buf->data[9] = 0x03;
    pst_buf->data[10] = 0x02;
    pst_buf->data[11] = 0x02;


    pst_buf->data[12] = 0x08;
    pst_buf->data[13] = 0x00;

    for (ul_loop = 0; ul_loop < ul_put_len - 20; ul_loop++)
    {
        pst_buf->data[14 + ul_loop] = (oal_uint8)ul_loop;
    }

    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    /*lint -e778*/
    pst_ether_header->us_ether_type = OAL_HOST2NET_SHORT(ETHER_TYPE_IP);
    /*lint +e778*/
    pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);      /* 偏移一个以太网头，取ip头 */

    pst_ip->uc_tos = (oal_uint8)(uc_tid << WLAN_IP_PRI_SHIFT);

    pst_buf->next = OAL_PTR_NULL;
    pst_buf->prev = OAL_PTR_NULL;

    OAL_MEMZERO(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE());

    return pst_buf;

}


OAL_STATIC oal_uint32  wal_config_frag_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_frag_threshold_stru *pst_frag_threshold;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_frag_threshold:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_frag_threshold = (mac_cfg_frag_threshold_stru *)puc_param;

    if (OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_frag_threshold:pst_mib_info[%d],ul_frag_threshold[%d] !}\r\n",
                    pst_mac_vap->pst_mib_info);
		return OAL_ERR_CODE_PTR_NULL;
    }

    //pst_mac_vap->pst_mib_info->st_wlan_mib_operation.ul_dot11FragmentationThreshold = pst_frag_threshold->ul_frag_threshold;
    mac_mib_set_FragmentationThreshold(pst_mac_vap, pst_frag_threshold->ul_frag_threshold);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_rts_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_rts_threshold_stru *pst_rts_threshold;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{wal_config_rts_threshold:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rts_threshold = (mac_cfg_rts_threshold_stru *)puc_param;

    if (OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_rts_threshold: pst_mac_vap->pst_mib_info is null ptr!}\r\n");
		return OAL_ERR_CODE_PTR_NULL;
    }

    mac_mib_set_RTSThreshold(pst_mac_vap, pst_rts_threshold->ul_rts_threshold);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_SMARTANT
OAL_STATIC oal_uint32  wal_config_get_ant_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_ant_info(pst_mac_vap, us_len, puc_param);
}
OAL_STATIC oal_uint32  wal_config_double_ant_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_double_ant_switch(pst_mac_vap, us_len, puc_param);
}
#endif

OAL_STATIC oal_uint32  wal_config_kick_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_kick_user_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_country(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_country_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_country_for_dfs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_country_for_dfs_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_country(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_country_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_tid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_tid_etc(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_user_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
   return hmac_config_user_info_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_dscr_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
   return hmac_config_set_dscr_param_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_log_level_etc(pst_mac_vap, us_len, puc_param);
}


#ifdef _PRE_WLAN_FEATURE_GREEN_AP
OAL_STATIC oal_uint32 wal_config_set_green_ap_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GREEN_AP_EN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_green_ap_en::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}
#endif

#ifdef _PRE_WLAN_FEATURE_AP_PM

OAL_STATIC oal_uint32 wal_config_sta_scan_wake_wow(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sta_scan_wake_wow(pst_mac_vap, us_len, puc_param);
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_set_pm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;
    oal_uint8           uc_en;


    uc_en = (oal_uint8)(*puc_param);

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_config_set_pm_switch:[%d]}\r\n", uc_en);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if((1 == uc_en) || (4 == uc_en))
    {
        g_wlan_pm_switch_etc = OAL_TRUE;
        wlan_pm_enable_etc();
    }
    else
    {
        wlan_pm_disable_etc();
        g_wlan_pm_switch_etc = OAL_FALSE;
    }
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    g_wlan_device_pm_switch = uc_en;
#endif

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_PM_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_feature_log::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

OAL_STATIC oal_uint32 wal_config_set_power_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;
    oal_uint8           uc_en;


    uc_en = (oal_uint8)(*puc_param);

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_config_set_power_test:[%d]}\r\n", uc_en);

    /***************************************************************************
        ....DMAC., ..DMAC..
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_POWER_TEST, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_feature_log::hmac_config_send_event_etc failed[%d].}", ul_ret);

    }

    return ul_ret;

}

OAL_STATIC oal_uint32 wal_config_set_ant(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)

{
    oal_uint32          ul_ret;
    oal_uint8           uc_en;


    uc_en = (oal_uint8)(*puc_param);

    OAM_WARNING_LOG1(0, OAM_SF_PWR, "{wal_config_set_ant:[%d]}\r\n", uc_en);

    /***************************************************************************
        ....DMAC., ..DMAC..
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ANT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_feature_log::hmac_config_send_event_etc failed[%d].}", ul_ret);

    }

    return ul_ret;

}


/*****************************************************************************
 . . .  : wal_config_get_ant
 ....  : ....
 ....  : pst_mac_vap: MAC VAP.....
             us_len     : ....
             puc_param  : ....
 ....  : .
 . . .  :
 ....  :
 ....  :

 ....      :
  1..    .   : 2014.3.28.
    .    .   : zhangyu
    ....   : .....

*****************************************************************************/
OAL_STATIC oal_uint32  wal_config_get_ant(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)

{
    return hmac_config_get_ant_etc(pst_mac_vap, us_len, puc_param);
}

#endif

//#ifdef    _PRE_WLAN_CHIP_TEST
OAL_STATIC oal_uint32  wal_config_set_rate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    mac_cfg_tx_comp_stru            st_event_set_bcast;

    oal_memset(&st_event_set_bcast, 0, OAL_SIZEOF(mac_cfg_tx_comp_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{wal_config_set_rate::pst_mac_vap/puc_param is null ptr %d %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_rate_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {

        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_rate::hmac_config_set_rate_etc error!}\r\n");
        return ul_ret;
    }
#if 0
    /* 重新常发 */
    if (OAL_SWITCH_ON == pst_mac_vap->bit_al_tx_flag)
    {
       st_event_set_bcast.uc_param = OAL_SWITCH_ON;
       st_event_set_bcast.en_protocol_mode = WLAN_PHY_PROTOCOL_BUTT;
       wal_config_always_tx(pst_mac_vap, us_len, (oal_uint8 *)&st_event_set_bcast);
    }
#endif
    return ul_ret;
}


OAL_STATIC oal_uint32  wal_config_set_mcs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY,"{wal_config_set_mcs::pst_mac_vap/puc_param is null ptr}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_mcs_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_mcs::hmac_config_set_mcs_etc error.}\r\n");
        return ul_ret;
    }

    return ul_ret;
}



OAL_STATIC oal_uint32  wal_config_set_mcsac(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_mcsac::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_mcsac_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_mcsac::hmac_config_set_mcsac_etc error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}
#ifdef _PRE_WLAN_FEATURE_11AX
OAL_STATIC oal_uint32  wal_config_set_mcsax(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_mcsax::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_mcsax(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_mcsax::hmac_config_set_mcsac_etc error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}
#endif



OAL_STATIC oal_uint32  wal_config_set_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_bw::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_bw_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_bw::hmac_config_set_bw_etc error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX

OAL_STATIC oal_uint32  wal_config_always_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_tx::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }


    ul_ret = hmac_config_always_tx(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_tx::hmac_config_always_tx failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_always_tx_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_always_tx_num(pst_mac_vap, us_len, puc_param);
}


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

OAL_STATIC oal_uint32  wal_config_always_tx_51(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;
    //mac_cfg_tx_comp_stru            *pst_event_set_bcast;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_tx_51::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    //pst_event_set_bcast = (mac_cfg_tx_comp_stru *)puc_param;

    ul_ret = hmac_config_always_tx_51(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_tx_51::hmac_config_always_tx failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_config_always_tx_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_tx_hw::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_always_tx_hw(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_tx_hw::hmac_config_always_tx_hw failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}

#endif /* #ifdef _PRE_WLAN_FEATURE_ALWAYS_TX */



OAL_STATIC oal_uint32  wal_config_always_rx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_rx::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_always_rx_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_rx::hmac_config_always_rx_etc failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

OAL_STATIC oal_uint32  wal_config_always_rx_51(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_rx::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_always_rx_51(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_rx::hmac_config_always_rx_etc failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  wal_config_set_tx_pow_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
   return hmac_config_set_tx_pow_param(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_pcie_pm_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_pcie_pm_level::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_pcie_pm_level_etc(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_pcie_pm_level::hmac_config_set_freq_skew failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_config_reg_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
     return hmac_config_reg_info_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_reg_write(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_reg_write_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND

OAL_STATIC oal_uint32  wal_config_set_restrict_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_restrict_band(pst_mac_vap, us_len, puc_param);
}
#endif
#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE)

OAL_STATIC oal_uint32  wal_config_set_omit_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_omit_acs(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_ONLINE_DPD

OAL_STATIC oal_uint32  wal_config_dpd_cfg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dpd_cfg(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_dpd_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return OAL_SUCC;
}

#endif

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))


OAL_STATIC oal_uint32  wal_config_sdio_flowctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sdio_flowctrl_etc(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_alg_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    return hmac_config_alg_param_etc(pst_mac_vap, us_len, puc_param);
}

#if 0

OAL_STATIC oal_uint32  wal_config_tdls_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_tdls_prohibited(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_tdls_channel_switch_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_tdls_channel_switch_prohibited(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_rx_fcs_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_rx_fcs_info_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_DFS

OAL_STATIC oal_uint32  wal_config_dfs_radartool(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dfs_radartool_etc(pst_mac_vap, us_len, puc_param);
}
#endif

/*Get the cfgid entry*/
OAL_STATIC wal_wid_op_stru* wal_config_get_wid_map(wal_wid_op_stru* pst_wid_map,
                                                    oal_uint16 en_wid,
                                                    oal_uint32 ul_wid_nums)
{
    oal_uint16 us_cfgid;
    wal_wid_op_stru* pst_current_wid;

    for(us_cfgid = 0; us_cfgid < ul_wid_nums; us_cfgid++)
    {
        pst_current_wid = pst_wid_map + us_cfgid;
        if (pst_current_wid->en_cfgid == en_wid)
        {
            return pst_current_wid;
        }
    }

    return NULL;
}


OAL_STATIC oal_uint32  wal_config_process_query(
                mac_vap_stru       *pst_mac_vap,
                oal_uint8          *puc_req_msg,
                oal_uint16          us_req_msg_len,
                oal_uint8          *puc_rsp_msg,
                oal_uint8          *puc_rsp_msg_len)
{
    oal_uint16          us_req_idx = 0;      /* 请求消息索引 */
    oal_uint16          us_rsp_idx = 0;      /* 返回消息索引 */
    oal_uint16          us_len     = 0;      /* WID对应返回值的长度 */
    wal_msg_query_stru *pst_query_msg;
    wal_msg_write_stru *pst_rsp_msg;
    oal_uint32          ul_ret;
    wal_wid_op_stru*    pst_current_wid;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap)
     || (OAL_PTR_NULL == puc_req_msg)
     || (OAL_PTR_NULL == puc_rsp_msg)
     || (OAL_PTR_NULL == puc_rsp_msg_len)))
    {
        OAM_ERROR_LOG4(0, OAM_SF_ANY, "{wal_config_process_query::pst_mac_vap/puc_req_msg/puc_rsp_msg/puc_rsp_msg_len null ptr error: %d, %d, %d, %d!}\r\n",
                       pst_mac_vap, puc_req_msg, puc_rsp_msg, puc_rsp_msg_len);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 查询消息格式如下:                                                     */
    /* +-------------------------------------------------------------------+ */
    /* | WID0          | WID1         | WID2         | ................... | */
    /* +-------------------------------------------------------------------+ */
    /* |     2 Bytes   |    2 Bytes   |    2 Bytes   | ................... | */
    /* +-------------------------------------------------------------------+ */

    /* 返回消息格式如下:                                                     */
    /* +-------------------------------------------------------------------+ */
    /* | WID0      | WID0 Length | WID0 Value  | ......................... | */
    /* +-------------------------------------------------------------------+ */
    /* | 2 Bytes   | 2 Byte      | WID Length  | ......................... | */
    /* +-------------------------------------------------------------------+ */

    while (us_req_idx < us_req_msg_len)
    {
        /* 从查询消息中得到一个WID值   */
        pst_query_msg = (wal_msg_query_stru *)(&puc_req_msg[us_req_idx]);
        us_req_idx   += WAL_MSG_WID_LENGTH;                       /* 指向下一个WID */

        /* 获取返回消息内存 */
        pst_rsp_msg = (wal_msg_write_stru *)(&puc_rsp_msg[us_rsp_idx]);

        pst_current_wid = wal_config_get_wid_map((wal_wid_op_stru*)g_ast_board_wid_op, pst_query_msg->en_wid, OAL_ARRAY_SIZE(g_ast_board_wid_op));
        if(NULL == pst_current_wid)
        {
#ifdef _PRE_WLAN_CFGID_DEBUG
            pst_current_wid = wal_config_get_wid_map((wal_wid_op_stru*)g_ast_board_wid_op_debug_etc,
                                                     pst_query_msg->en_wid,
                                                     wal_config_get_debug_wid_arrysize_etc());
            if(NULL == pst_current_wid)
            {
                OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_query::cfgid not invalid %d!}\r\n", pst_query_msg->en_wid);
                continue;
            }
            /*else, call the cfgid func.*/
#else
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_query::cfgid not invalid %d!}\r\n", pst_query_msg->en_wid);
            continue;
#endif
        }

        /* 异常情况，cfgid对应的get函数为空 */
        if (OAL_PTR_NULL == pst_current_wid->p_get_func)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_query:: get_func ptr is null, wid is %d!}\r\n", pst_query_msg->en_wid);
            continue;
        }

        ul_ret = pst_current_wid->p_get_func(pst_mac_vap, &us_len, pst_rsp_msg->auc_value);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_query:: func return no SUCC. wid and ret value is:%d, %d!}\r\n", pst_query_msg->en_wid, ul_ret);
            continue;
        }

        pst_rsp_msg->en_wid = pst_query_msg->en_wid;            /* 设置返回消息的WID */
        pst_rsp_msg->us_len = us_len;

        us_rsp_idx += us_len + WAL_MSG_WRITE_MSG_HDR_LENGTH;    /* 消息体的长度 再加上消息头的长度 */

        /*消息Response 接口容易让调用者使用超过消息数组空间长度，
          这里需要加判断，检查长度和狗牌，后续需要整改*/
        if(OAL_UNLIKELY(us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru) > HMAC_RSP_MSG_MAX_LEN))
        {
            OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_query::us_cfgid:%d reponse msg len:%u over limit:%u}",
                                pst_current_wid->en_cfgid,us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru),HMAC_RSP_MSG_MAX_LEN);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            oal_print_hex_dump((oal_uint8*)puc_rsp_msg, HMAC_RSP_MSG_MAX_LEN,
                                        32, "puc_rsp_msg: ");
#endif
        }
    }

    *puc_rsp_msg_len = (oal_uint8)us_rsp_idx;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_process_write(
                mac_vap_stru    *pst_mac_vap,
                oal_uint8       *puc_req_msg,
                oal_uint16       us_msg_len,
                oal_uint8       *puc_rsp_msg,
                oal_uint8       *puc_rsp_msg_len)
{
    oal_uint16              us_req_idx = 0;
    oal_uint16              us_rsp_idx = 0;
    wal_msg_write_stru     *pst_write_msg;
    wal_msg_write_rsp_stru *pst_rsp_msg;
    oal_uint32              ul_ret;
    wal_wid_op_stru*        pst_current_wid;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_req_msg)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_process_write::pst_mac_vap/puc_req_msg null ptr error %d, %d!}\r\n", pst_mac_vap, puc_req_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置消息的格式如下:                                                   */
    /* +-------------------------------------------------------------------+ */
    /* | WID0      | WID0 Length | WID0 Value  | ......................... | */
    /* +-------------------------------------------------------------------+ */
    /* | 2 Bytes   | 2 Byte      | WID Length  | ......................... | */
    /* +-------------------------------------------------------------------+ */

    /* 返回消息的格式如下:                                                   */
    /* +-------------------------------------------------------------------+ */
    /* | WID0     | resv    | WID0 错误码 |  WID1   | resv | WID1错误码 |  | */
    /* +-------------------------------------------------------------------+ */
    /* | 2 Bytes  | 2 Bytes | 4 Byte      | 2 Bytes | 2 B  |  4 Bytes   |  | */
    /* +-------------------------------------------------------------------+ */

    while (us_req_idx < us_msg_len)
    {
        /* 获取一个设置WID消息   */
        pst_write_msg = (wal_msg_write_stru *)(&puc_req_msg[us_req_idx]);

        /* 获取返回消息内存 */
        pst_rsp_msg = (wal_msg_write_rsp_stru *)(&puc_rsp_msg[us_rsp_idx]);

        us_req_idx += pst_write_msg->us_len + WAL_MSG_WRITE_MSG_HDR_LENGTH;   /* 指向下一个WID设置消息 */

        /* 寻找cfgid 对应的write函数 */
        pst_current_wid = wal_config_get_wid_map((wal_wid_op_stru*)g_ast_board_wid_op, pst_write_msg->en_wid, OAL_ARRAY_SIZE(g_ast_board_wid_op));
        if(NULL == pst_current_wid)
        {
#ifdef _PRE_WLAN_CFGID_DEBUG
            pst_current_wid = wal_config_get_wid_map((wal_wid_op_stru*)g_ast_board_wid_op_debug_etc,
                                                     pst_write_msg->en_wid,
                                                     wal_config_get_debug_wid_arrysize_etc());
            if(NULL == pst_current_wid)
            {
                OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_write::cfgid not invalid %d!}\r\n", pst_write_msg->en_wid);
                continue;
            }
            /*else, go on call the cfgid func.*/
#else
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_write::cfgid not invalid %d!}\r\n", pst_write_msg->en_wid);
            continue;
#endif
        }

        /* 异常情况，cfgid对应的set函数为空 */
        if (OAL_PTR_NULL == pst_current_wid->p_set_func)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_write:: get_func ptr is null, wid is %d!}\r\n", pst_write_msg->en_wid);
            continue;
        }


        ul_ret = pst_current_wid->p_set_func(pst_mac_vap, pst_write_msg->us_len, pst_write_msg->auc_value);

        /* 将返回错误码设置到rsp消息中 */
        pst_rsp_msg->en_wid = pst_write_msg->en_wid;
        pst_rsp_msg->ul_err_code = ul_ret;
        us_rsp_idx += OAL_SIZEOF(wal_msg_write_rsp_stru);

        /*消息Response 接口容易让调用者使用超过消息数组空间长度，
          这里需要加判断，检查长度和狗牌，后续需要整改*/
        if(OAL_UNLIKELY(us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru) > HMAC_RSP_MSG_MAX_LEN))
        {
            OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_write::us_cfgid:%d reponse msg len:%u over limit:%u}",
                                pst_current_wid->en_cfgid,us_rsp_idx + OAL_SIZEOF(wal_msg_hdr_stru),HMAC_RSP_MSG_MAX_LEN);
        }

        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_write::func return no SUCC. wid and ret value is %d, %d!}\r\n", pst_write_msg->en_wid, ul_ret);
        }

    }

    *puc_rsp_msg_len = (oal_uint8)us_rsp_idx;

    return OAL_SUCC;
}


oal_uint32  wal_config_process_pkt_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru         *pst_event;
    wal_msg_stru           *pst_msg;
    wal_msg_stru           *pst_rsp_msg = NULL;
    frw_event_hdr_stru     *pst_event_hdr;
    mac_vap_stru           *pst_mac_vap;
    oal_uint16              us_msg_len;
    oal_uint8               uc_rsp_len = 0;
    oal_uint8               uc_rsp_toal_len = 0;
    oal_uint32              ul_ret;
    oal_ulong               ul_request_address;
    oal_uint8               ac_rsp_msg[HMAC_RSP_MSG_MAX_LEN] = {0};

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_process_pkt_etc::pst_event_mem null ptr error!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event     = frw_get_event_stru(pst_event_mem);
    pst_event_hdr = &(pst_event->st_event_hdr);
    ul_request_address = ((wal_msg_rep_hdr*)pst_event->auc_event_data)->ul_request_address;
    pst_msg       = (wal_msg_stru *)(frw_get_event_payload(pst_event_mem) + OAL_SIZEOF(wal_msg_rep_hdr));

    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_event_hdr->uc_vap_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt_etc::hmac_get_vap_by_id return err code!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 取返回消息 */
    pst_rsp_msg  = (wal_msg_stru *)ac_rsp_msg;

    /* 取配置消息的长度 */
    us_msg_len = pst_msg->st_msg_hdr.us_msg_len;

    OAM_INFO_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt_etc::a config event occur!}\r\n");

    switch (pst_msg->st_msg_hdr.en_msg_type)
    {
        case WAL_MSG_TYPE_QUERY:

            ul_ret = wal_config_process_query(pst_mac_vap, pst_msg->auc_msg_data, us_msg_len, pst_rsp_msg->auc_msg_data, &uc_rsp_len);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt_etc::wal_config_process_query return error code %d!}\r\n", ul_ret);
                return ul_ret;
            }

            break;

        case WAL_MSG_TYPE_WRITE:
            ul_ret = wal_config_process_write(pst_mac_vap, pst_msg->auc_msg_data, us_msg_len, pst_rsp_msg->auc_msg_data, &uc_rsp_len);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt_etc::wal_config_process_write return error code %d!}\r\n", ul_ret);
                return ul_ret;
            }

            break;

        default:
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt_etc::pst_msg->st_msg_hdr.en_msg_type error, msg_type is %d!}\r\n", pst_msg->st_msg_hdr.en_msg_type);

            return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /*response 长度要包含头长*/
    uc_rsp_toal_len = uc_rsp_len + OAL_SIZEOF(wal_msg_hdr_stru);

    if(OAL_UNLIKELY(uc_rsp_toal_len > HMAC_RSP_MSG_MAX_LEN))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_process_pkt_etc::invaild response len %u!}\r\n", uc_rsp_toal_len);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 填充返回消息头 */
    pst_rsp_msg->st_msg_hdr.en_msg_type = WAL_MSG_TYPE_RESPONSE;
    pst_rsp_msg->st_msg_hdr.uc_msg_sn   = pst_msg->st_msg_hdr.uc_msg_sn;
    pst_rsp_msg->st_msg_hdr.us_msg_len  = uc_rsp_len;

    if(ul_request_address)
    {
        /*need response*/
        oal_uint8* pst_rsp_msg_tmp = oal_memalloc(uc_rsp_toal_len);
        if(NULL == pst_rsp_msg_tmp)
        {
            /*no mem*/
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_config_process_pkt_etc::wal_config_process_pkt_etc msg alloc %u failed!", uc_rsp_toal_len);
            wal_set_msg_response_by_addr_etc(ul_request_address, NULL, OAL_ERR_CODE_PTR_NULL, uc_rsp_toal_len);
        }
        else
        {
            oal_memcopy((oal_void*)pst_rsp_msg_tmp, (oal_void*)ac_rsp_msg, uc_rsp_toal_len);
            if(OAL_SUCC != wal_set_msg_response_by_addr_etc(ul_request_address, (oal_void*)pst_rsp_msg_tmp, OAL_SUCC, uc_rsp_toal_len))
            {
                OAL_IO_PRINT("wal_config_process_pkt_etc did't found the request msg, request addr:0x%lx\n", ul_request_address);
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_process_pkt_etc::wal_config_process_pkt_etc did't found the request msg!");
                oal_free(pst_rsp_msg_tmp);
            }
        }
    }

    /* 唤醒WAL等待的进程 */
    wal_cfg_msg_task_sched_etc();

    return OAL_SUCC;
}



OAL_STATIC oal_uint32 wal_config_add_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_11i_add_key_etc(pst_mac_vap, us_len, puc_param);
}
 
OAL_STATIC oal_uint32 wal_config_get_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /*通过函数调用，hmac具体实现*/
    return (hmac_config_11i_get_key_etc(pst_mac_vap, us_len, puc_param));
}


OAL_STATIC oal_uint32 wal_config_remove_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /*通过函数调用，hmac具体实现*/
    return (hmac_config_11i_remove_key_etc(pst_mac_vap, us_len, puc_param));
}


OAL_STATIC oal_uint32 wal_config_set_default_key(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /*通过函数调用，hmac具体实现*/
    return (hmac_config_11i_set_default_key_etc(pst_mac_vap, us_len, puc_param));
}



oal_uint32 wal_config_get_assoc_req_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_assoc_req_ie_etc(pst_mac_vap, pus_len, puc_param);
}



oal_uint32 wal_config_set_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_wps_ie_etc(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_set_wps_p2p_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_wps_p2p_ie_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

OAL_STATIC oal_uint32  wal_config_set_edca_opt_switch_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_switch_sta_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_edca_opt_weight_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_weight_sta_etc(pst_mac_vap, us_len, puc_param);
}




OAL_STATIC oal_uint32  wal_config_set_edca_opt_switch_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_switch_ap_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_edca_opt_cycle_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_edca_opt_cycle_ap_etc(pst_mac_vap, us_len, puc_param);
}

#endif


OAL_STATIC oal_uint32  wal_config_open_wmm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_open_wmm::pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 针对配置vap做保护 */
    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_open_wmm::this is config vap! can't get info.}");
        return OAL_FAIL;
    }

    return hmac_config_sync_cmd_common_etc(pst_mac_vap, WLAN_CFGID_WMM_SWITCH, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_VOWIFI

OAL_STATIC oal_uint32  wal_config_vowifi_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_vowifi_info_etc(pst_mac_vap, us_len, puc_param);
}

#endif /* _PRE_WLAN_FEATURE_VOWIFI */

#ifdef _PRE_WLAN_FEATURE_IP_FILTER

OAL_STATIC oal_uint32  wal_config_update_ip_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_update_ip_filter_etc(pst_mac_vap, us_len, puc_param);
}

#endif //_PRE_WLAN_FEATURE_IP_FILTER


OAL_STATIC oal_uint32  wal_config_get_version(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_version_etc(pst_mac_vap, us_len, puc_param);
}



#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)


OAL_STATIC oal_uint32 wal_config_blacklist_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_blacklist_add::not valid vap mode=%d!}\r\n", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }
    return hmac_config_blacklist_add_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_blacklist_add_only(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_blacklist_add_only::not valid vap mode=%d!}\r\n", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }
    return hmac_config_blacklist_add_only_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_blacklist_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_blacklist_del::not valid vap mode=%d!}\r\n", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }
    return hmac_config_blacklist_del_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_blacklist_clr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_blacklist_clr::not valid vap mode=%d!}\r\n", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }

    oal_memcopy(puc_param, BROADCAST_MACADDR, OAL_MAC_ADDR_LEN);

    return hmac_config_blacklist_del_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_blacklist_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_get_blacklist_mode::not valid vap mode=%d!}\r\n", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }
    *pus_len = OAL_SIZEOF(oal_int32);

    return hmac_config_get_blacklist_mode(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_blacklist_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_set_blacklist_mode::not valid vap mode=%d!}\r\n", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }
    return hmac_config_set_blacklist_mode_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_blacklist_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{wal_config_blacklist_show::not valid vap mode=%d!}\r\n", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }
    return hmac_config_show_blacklist_etc(pst_mac_vap, us_len, puc_param);
}
#endif
#ifdef _PRE_WLAN_WEB_CMD_COMM

OAL_STATIC oal_uint32 wal_config_get_temp(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *pus_len = OAL_SIZEOF(oal_int32);

    return hmac_config_get_temp(pst_mac_vap, pus_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP

OAL_STATIC oal_uint32  wal_config_proxyarp_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_proxyarp_en_stru *pst_proxyarp_en_param;

    pst_proxyarp_en_param = (mac_proxyarp_en_stru *)puc_param;

    hmac_proxyarp_on(pst_mac_vap, pst_proxyarp_en_param->en_proxyarp);

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DHCP_REQ_DISABLE

OAL_STATIC oal_uint32  wal_config_set_dhcp_req_disable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_dhcp_req_disable(pst_mac_vap, us_len, puc_param);
}

#endif

oal_int32  wal_recv_config_cmd_etc(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_int8                ac_vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru    *pst_net_dev;
    mac_vap_stru           *pst_mac_vap;
    frw_event_mem_stru     *pst_event_mem;
    frw_event_stru         *pst_event;
    oal_uint32              ul_ret      = OAL_SUCC;
    wal_msg_stru           *pst_msg;
    oal_netbuf_stru        *pst_netbuf;
    oal_uint16              us_netbuf_len; /* 传给sdt的skb数据区不包括头尾空间的长度 */
    wal_msg_stru           *pst_rsp_msg;
    wal_msg_rep_hdr        *pst_rep_hdr = NULL;

    oal_uint16              us_msg_size = us_len;
    oal_uint16              us_need_response = OAL_FALSE;

    DECLARE_WAL_MSG_REQ_STRU(st_msg_request);

    WAL_MSG_REQ_STRU_INIT(st_msg_request);

    oal_memcopy(ac_vap_name, puc_buf, OAL_IF_NAME_SIZE);
    ac_vap_name[OAL_IF_NAME_SIZE - 1] = '\0';   /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_vap_name);
    if (OAL_PTR_NULL == pst_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_config_cmd_etc::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(pst_net_dev);   /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);    /* 获取mac vap */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_recv_config_cmd_etc::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_msg_size -= OAL_IF_NAME_SIZE;

    /* 申请内存 */
    pst_event_mem = FRW_EVENT_ALLOC(us_msg_size + OAL_SIZEOF(wal_msg_rep_hdr));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_config_cmd_etc::request %d mem failed}\r\n", us_msg_size);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CRX,
                       WAL_HOST_CRX_SUBTYPE_CFG,
                       (oal_uint16)(us_msg_size),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* 填写事件payload */
    oal_memcopy(frw_get_event_payload(pst_event_mem) + OAL_SIZEOF(wal_msg_rep_hdr), puc_buf + OAL_IF_NAME_SIZE, us_msg_size);
    pst_msg = (wal_msg_stru *)(puc_buf + OAL_IF_NAME_SIZE);
    pst_rep_hdr = (wal_msg_rep_hdr*)pst_event->auc_event_data;

    if (WAL_MSG_TYPE_QUERY == pst_msg->st_msg_hdr.en_msg_type)
    {
        /*need response*/
        us_need_response = OAL_TRUE;
    }

    if(OAL_TRUE == us_need_response)
    {
        pst_rep_hdr->ul_request_address = (oal_ulong)&st_msg_request;
        wal_msg_request_add_queue_etc(&st_msg_request);
    }
    else
    {
        pst_rep_hdr->ul_request_address = 0;
    }

    ul_ret = wal_config_process_pkt_etc(pst_event_mem);

    if(OAL_TRUE == us_need_response)
    {
        wal_msg_request_remove_queue_etc(&st_msg_request);
    }

    if (OAL_SUCC != ul_ret)
    {
        FRW_EVENT_FREE(pst_event_mem);
        if(NULL != st_msg_request.pst_resp_mem)
        {
            /*异常时内存需要释放*/
            oal_free(st_msg_request.pst_resp_mem);
        }
        return (oal_int32)ul_ret;
    }

    /* 释放内存 */
    FRW_EVENT_FREE(pst_event_mem);

    /* 如果是查询消息类型，结果上报 */
    if(OAL_TRUE == us_need_response)
    {
        if (OAL_UNLIKELY(OAL_PTR_NULL == g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func))
        {
            if(NULL != st_msg_request.pst_resp_mem)
            {
                /*异常时内存需要释放*/
                oal_free(st_msg_request.pst_resp_mem);
            }
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (NULL == st_msg_request.pst_resp_mem)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_config_cmd_etc::get response ptr failed!}\r\n");
            return (oal_int32)ul_ret;
        }

        pst_rsp_msg  = (wal_msg_stru *)st_msg_request.pst_resp_mem;

        us_netbuf_len = pst_rsp_msg->st_msg_hdr.us_msg_len + 1; /* +1是sdt工具的需要 */

        us_netbuf_len = (us_netbuf_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) ? WLAN_SDT_NETBUF_MAX_PAYLOAD : us_netbuf_len;

        pst_netbuf = oam_alloc_data2sdt_etc(us_netbuf_len);
        if (OAL_PTR_NULL == pst_netbuf)
        {
            if(NULL != st_msg_request.pst_resp_mem)
            {
                /*异常时内存需要释放*/
                oal_free(st_msg_request.pst_resp_mem);
            }
            return OAL_ERR_CODE_PTR_NULL;
        }

        oal_netbuf_data(pst_netbuf)[0] = 'M';     /* sdt需要 */
        oal_memcopy(oal_netbuf_data(pst_netbuf) + 1, (oal_uint8 *)pst_rsp_msg->auc_msg_data, us_netbuf_len - 1);
        oal_free(st_msg_request.pst_resp_mem);

        oam_report_data2sdt_etc(pst_netbuf, OAM_DATA_TYPE_CFG, OAM_PRIMID_TYPE_DEV_ACK);
    }

    return OAL_SUCC;
}


oal_int32  wal_recv_memory_cmd_etc(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_netbuf_stru            *pst_netbuf;
    wal_sdt_mem_frame_stru     *pst_mem_frame;
    oal_uint                    ul_mem_addr;    /* 读取内存地址 */
    oal_uint16                  us_mem_len;     /* 需要读取的长度 */
    oal_uint8                   uc_offload_core_mode; /* offload下，表示哪一个核 */

    pst_mem_frame        = (wal_sdt_mem_frame_stru *)puc_buf;
    ul_mem_addr          = pst_mem_frame->ul_addr;
    us_mem_len           = pst_mem_frame->us_len;
    uc_offload_core_mode = pst_mem_frame->en_offload_core_mode;

    if (WAL_OFFLOAD_CORE_MODE_DMAC == uc_offload_core_mode)
    {
        /* 如果是offload情形，并且要读取的内存是wifi芯片侧，需要抛事件，后续开发 */
        return OAL_SUCC;
    }

    if (OAL_PTR_NULL == ul_mem_addr)            /* 读写地址不合理 */
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (us_mem_len > WAL_SDT_MEM_MAX_LEN)
    {
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }



    if (MAC_SDT_MODE_READ== pst_mem_frame->uc_mode)
    {
        if (OAL_UNLIKELY(OAL_PTR_NULL == g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }


        pst_netbuf = oam_alloc_data2sdt_etc(us_mem_len);
        if (OAL_PTR_NULL == pst_netbuf)
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        oal_memcopy(oal_netbuf_data(pst_netbuf), (oal_void *)ul_mem_addr, us_mem_len);

        oam_report_data2sdt_etc(pst_netbuf, OAM_DATA_TYPE_MEM_RW, OAM_PRIMID_TYPE_DEV_ACK);
    }
    else if (MAC_SDT_MODE_WRITE == pst_mem_frame->uc_mode)
    {
        oal_memcopy((oal_void *)ul_mem_addr, pst_mem_frame->auc_data, us_mem_len);
    }

    return OAL_SUCC;
}


oal_int32  wal_parse_global_var_cmd_etc(
                wal_sdt_global_var_stru    *pst_global_frame,
                oal_uint                   ul_global_var_addr)
{
    oal_netbuf_stru            *pst_netbuf;
    oal_uint16                  us_skb_len;

    if (MAC_SDT_MODE_WRITE == pst_global_frame->uc_mode)
    {
        oal_memcopy((oal_void *)ul_global_var_addr, (oal_void *)(pst_global_frame->auc_global_value), pst_global_frame->us_len);
    }
    else if (MAC_SDT_MODE_READ == pst_global_frame->uc_mode)
    {
        if (OAL_UNLIKELY(OAL_PTR_NULL == g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        us_skb_len = pst_global_frame->us_len;

        us_skb_len = (us_skb_len > WLAN_SDT_NETBUF_MAX_PAYLOAD) ? WLAN_SDT_NETBUF_MAX_PAYLOAD : us_skb_len;

        pst_netbuf = oam_alloc_data2sdt_etc(us_skb_len);
        if (OAL_PTR_NULL == pst_netbuf)
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        oal_memcopy(oal_netbuf_data(pst_netbuf), (oal_void *)ul_global_var_addr, us_skb_len);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        oam_report_data2sdt_etc(pst_netbuf, OAM_DATA_TYPE_GVAR_RW, OAM_PRIMID_TYPE_DEV_ACK);
#else
        oam_report_data2sdt_etc(pst_netbuf, OAM_DATA_TYPE_MEM_RW, OAM_PRIMID_TYPE_DEV_ACK);
#endif
    }

    return OAL_SUCC;
}


oal_int32  wal_recv_global_var_cmd_etc(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    wal_sdt_global_var_stru        *pst_global_frame;
    oal_uint                       ul_global_var_addr;

    if (OAL_PTR_NULL == puc_buf)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_global_frame = (wal_sdt_global_var_stru *)puc_buf;

    if (WAL_OFFLOAD_CORE_MODE_DMAC == pst_global_frame->en_offload_core_mode)
    {
        /* offload情形，并且要读取的全局变量在wifi芯片侧，需要抛事件，后续开发 */
        return OAL_SUCC;
    }

    ul_global_var_addr = oal_kallsyms_lookup_name(pst_global_frame->auc_global_value_name);
    if (0 == ul_global_var_addr)    /* not found */
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_global_var_cmd_etc::kernel lookup global var address returns 0!}\r\n");
        return OAL_FAIL;
    }

    return wal_parse_global_var_cmd_etc(pst_global_frame, ul_global_var_addr);
}


oal_int32  wal_recv_reg_cmd_etc(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_int8                     ac_vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru         *pst_net_dev;
    mac_vap_stru                *pst_mac_vap;
    wal_sdt_reg_frame_stru      *pst_reg_frame;
    oal_int32                    l_ret;
    hmac_vap_cfg_priv_stru      *pst_cfg_priv;
    oal_netbuf_stru             *pst_net_buf;
    oal_uint32                   ul_ret;

    oal_memcopy(ac_vap_name, puc_buf, OAL_IF_NAME_SIZE);
    ac_vap_name[OAL_IF_NAME_SIZE - 1] = '\0';   /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_vap_name);
    if (OAL_PTR_NULL == pst_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_reg_cmd_etc::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(pst_net_dev);   /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);    /* 获取mac vap */

    ul_ret = hmac_vap_get_priv_cfg_etc(pst_mac_vap, &pst_cfg_priv);      /* 取配置私有结构体 */
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd_etc::hmac_vap_get_priv_cfg_etc return null_ptr_err!}\r\n");
        return (oal_int32)ul_ret;
    }

    pst_cfg_priv->en_wait_ack_for_sdt_reg = OAL_FALSE;

    ul_ret = hmac_sdt_recv_reg_cmd_etc(pst_mac_vap, puc_buf, us_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd_etc::hmac_sdt_recv_reg_cmd_etc return error code!}\r\n");

        return (oal_int32)ul_ret;
    }

    pst_reg_frame = (wal_sdt_reg_frame_stru *)puc_buf;

    if (MAC_SDT_MODE_READ == pst_reg_frame->uc_mode || MAC_SDT_MODE_READ16 == pst_reg_frame->uc_mode)
    {
        wal_wake_lock();
        /*lint -e730*//* info, boolean argument to function */
        l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_cfg_priv->st_wait_queue_for_sdt_reg,
                                                     OAL_TRUE == pst_cfg_priv->en_wait_ack_for_sdt_reg,
                                                     (2 * OAL_TIME_HZ));
        /*lint +e730*/
        if (0 == l_ret)
        {
            /* 超时 */
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd_etc::wal_netdev_open_etc: wait queue timeout!}\r\n");
            wal_wake_unlock();
            return -OAL_EINVAL;
        }
        else if (l_ret < 0)
        {
            /* 异常 */
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_recv_reg_cmd_etc::wal_netdev_open_etc: wait queue error!}\r\n");
            wal_wake_unlock();
            return -OAL_EINVAL;
        }
        wal_wake_unlock();
        /*lint +e774*/

        /* 读取返回的寄存器值 */
        pst_reg_frame->ul_reg_val = *((oal_uint32 *)(pst_cfg_priv->ac_rsp_msg));

        if (OAL_UNLIKELY(OAL_PTR_NULL != g_st_oam_sdt_func_hook_etc.p_sdt_report_data_func))
        {
            pst_net_buf = oam_alloc_data2sdt_etc((oal_uint16)OAL_SIZEOF(wal_sdt_reg_frame_stru));
            if (OAL_PTR_NULL == pst_net_buf)
            {
                return OAL_ERR_CODE_PTR_NULL;
            }

            oal_memcopy(oal_netbuf_data(pst_net_buf), (oal_uint8 *)pst_reg_frame, (oal_uint16)OAL_SIZEOF(wal_sdt_reg_frame_stru));

            oam_report_data2sdt_etc(pst_net_buf, OAM_DATA_TYPE_REG_RW, OAM_PRIMID_TYPE_DEV_ACK);
        }
    }

    return OAL_SUCC;
}
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)

oal_int32  wal_recv_sample_cmd(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_int8                     ac_vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru         *pst_net_dev;
    mac_vap_stru                *pst_mac_vap;

    oal_memcopy(ac_vap_name, puc_buf, OAL_IF_NAME_SIZE);
    ac_vap_name[OAL_IF_NAME_SIZE - 1] = '\0';   /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_vap_name);
    if (OAL_PTR_NULL == pst_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_sample_cmd::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(pst_net_dev);   /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);    /* 获取mac vap */

    hmac_sdt_recv_sample_cmd(pst_mac_vap, puc_buf, us_len);
    return OAL_SUCC;
}


oal_uint32 wal_sample_report2sdt(frw_event_mem_stru *pst_event_mem)
{
    oal_uint16                  us_payload_len;
    oal_netbuf_stru             *pst_net_buf;
    frw_event_stru              *pst_event;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sample_report2sdt::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    us_payload_len = pst_event->st_event_hdr.us_length - FRW_EVENT_HDR_LEN;

    pst_net_buf = oam_alloc_data2sdt_etc(us_payload_len);
    if (OAL_PTR_NULL == pst_net_buf)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memcopy(oal_netbuf_data(pst_net_buf), pst_event->auc_event_data, us_payload_len);

    oam_report_data2sdt_etc(pst_net_buf, OAM_DATA_TYPE_SAMPLE, OAM_PRIMID_TYPE_DEV_ACK);
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_ONLINE_DPD

oal_uint32 wal_dpd_report2sdt(frw_event_mem_stru *pst_event_mem)
{
    oal_uint16                  us_payload_len;
    oal_netbuf_stru             *pst_net_buf;
    frw_event_stru              *pst_event;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_sample_report2sdt::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    us_payload_len = pst_event->st_event_hdr.us_length - FRW_EVENT_HDR_LEN;

    pst_net_buf = oam_alloc_data2sdt_etc(us_payload_len);
    if (OAL_PTR_NULL == pst_net_buf)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memcopy(oal_netbuf_data(pst_net_buf), pst_event->auc_event_data, us_payload_len);

    oam_report_data2sdt_etc(pst_net_buf, OAM_DATA_TYPE_DPD, OAM_PRIMID_TYPE_DEV_ACK);
    return OAL_SUCC;
}

#endif
#ifdef _PRE_WLAN_RF_AUTOCALI
oal_int32  wal_recv_autocali_cmd(oal_uint8 *puc_buf, oal_uint16 us_len)
{
    oal_int8                     ac_vap_name[OAL_IF_NAME_SIZE];
    oal_net_device_stru         *pst_net_dev;
    mac_vap_stru                *pst_mac_vap;

    oal_memcopy(ac_vap_name, puc_buf, OAL_IF_NAME_SIZE);
    ac_vap_name[OAL_IF_NAME_SIZE - 1] = '\0';   /* 防止字符串异常 */

    /* 根据dev_name找到dev */
    pst_net_dev = oal_dev_get_by_name(ac_vap_name);
    if (OAL_PTR_NULL == pst_net_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_recv_autocali_cmd::oal_dev_get_by_name return null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_dev_put(pst_net_dev);   /* 调用oal_dev_get_by_name后，必须调用oal_dev_put使net_dev的引用计数减一 */
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);    /* 获取mac vap */

    hmac_sdt_recv_autocali_cmd(pst_mac_vap,puc_buf, us_len);
    return OAL_SUCC;
}
oal_uint32 wal_autocali_report2sdt(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru              *pst_event;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_autocali_report2sdt::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_event = frw_get_event_stru(pst_event_mem);

    oam_report_data2sdt_etc(*(oal_netbuf_stru**)pst_event->auc_event_data, OAM_DATA_TYPE_AUTOCALI , OAM_PRIMID_TYPE_DEV_ACK);
    return OAL_SUCC;
}
#endif

oal_void wal_drv_cfg_func_hook_init_etc(oal_void)
{
    g_st_wal_drv_func_hook_etc.p_wal_recv_cfg_data_func     = wal_recv_config_cmd_etc;
    g_st_wal_drv_func_hook_etc.p_wal_recv_mem_data_func     = wal_recv_memory_cmd_etc;
    g_st_wal_drv_func_hook_etc.p_wal_recv_reg_data_func     = wal_recv_reg_cmd_etc;
    g_st_wal_drv_func_hook_etc.p_wal_recv_global_var_func   = wal_recv_global_var_cmd_etc;
#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
    g_st_wal_drv_func_hook_etc.p_wal_recv_sample_data_func  = wal_recv_sample_cmd;
#endif
#ifdef _PRE_WLAN_RF_AUTOCALI
    g_st_wal_drv_func_hook_etc.p_wal_recv_autocali_data_func  = wal_recv_autocali_cmd;
#endif

}


OAL_STATIC oal_uint32  wal_config_set_pmksa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_pmksa_etc(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_del_pmksa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_del_pmksa_etc(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_flush_pmksa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_flush_pmksa_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_remain_on_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_remain_on_channel_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_cancel_remain_on_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_cancel_remain_on_channel_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_STA_PM
OAL_STATIC oal_uint32  wal_config_set_sta_pm_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_ps_mode_param_stru   *pst_ps_mode_param;
    hmac_vap_stru                *pst_hmac_vap;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_ps_mode_param = (mac_cfg_ps_mode_param_stru*)puc_param;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->uc_ps_mode = pst_ps_mode_param->uc_vap_ps_mode;
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id,OAM_SF_PWR,"wal_config_set_sta_pm_mode ps_mode[%d]",pst_hmac_vap->uc_ps_mode);

#if _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if(MAX_FAST_PS==pst_hmac_vap->uc_ps_mode)
    {
        wlan_pm_set_timeout_etc(g_wlan_fast_check_cnt);
    }
    else
#endif
    {
        wlan_pm_set_timeout_etc(WLAN_SLEEP_DEFAULT_CHECK_CNT);
    }
#endif
    return hmac_config_set_sta_pm_mode_etc(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_PSM_DEBUG_MODE
OAL_STATIC oal_uint32  wal_config_show_pm_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "{wal_config_show_pm_info::ap mode has no pm fsm.}");
        return OAL_FAIL;
    }
    return hmac_config_sync_cmd_common_etc(pst_mac_vap, WLAN_CFGID_SHOW_PS_INFO, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_set_sta_pm_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                *pst_hmac_vap;
    mac_cfg_ps_open_stru         *pst_sta_pm_open;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_WARNING_LOG0(0, OAM_SF_PWR, "{wal_config_set_sta_pm_on::pst_mac_vap / puc_param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap    = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{wal_config_set_sta_pm_on::pst_hmac_vap null,vap state[%d].}",pst_mac_vap->en_vap_state);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_sta_pm_open = (mac_cfg_ps_open_stru *)puc_param;

    /* 如果上层主动dhcp成功此时取消超时开低功耗的定时器 */
    if((OAL_TRUE == pst_hmac_vap->st_ps_sw_timer.en_is_registerd) && (pst_sta_pm_open->uc_pm_enable > MAC_STA_PM_SWITCH_OFF))
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_ps_sw_timer));
    }

    return hmac_config_set_sta_pm_on_etc(pst_mac_vap, us_len, puc_param);
}
#endif


#ifdef _PRE_WLAN_FEATURE_STA_UAPSD
OAL_STATIC oal_uint32 wal_config_set_uapsd_para(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_config_set_uapsd_para_etc(pst_mac_vap, us_len, puc_param);
}
#endif

OAL_STATIC oal_uint32  wal_config_mgmt_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_wpas_mgmt_tx_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_query_station_stats(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_query_station_info_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_query_rssi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_query_rssi_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_query_rate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_query_rate_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_query_psst(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_query_psst(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32  wal_config_query_ani(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_query_ani_etc(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_HS20

OAL_STATIC oal_uint32  wal_config_set_qos_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_qos_map(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_P2P

OAL_STATIC oal_uint32  wal_config_set_p2p_ps_ops(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_p2p_ps_ops_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_p2p_ps_noa(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_p2p_ps_noa_etc(pst_mac_vap, us_len, puc_param);
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
OAL_STATIC oal_uint32 wal_config_cfg_vap_h2d(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_cfg_vap_h2d_etc(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32 wal_config_host_dev_init(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_host_dev_init_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_host_dev_exit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_host_dev_exit_etc(pst_mac_vap);
}

oal_uint32 wal_send_cali_matrix_data(oal_net_device_stru *pst_net_dev)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mac_vap_stru               *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap != OAL_PTR_NULL)
    {
        hmac_send_cali_matrix_data(pst_mac_vap);
    }
    else
    {
        OAL_IO_PRINT("wal_send_cali_matrix_data:netdev name %s not have dev priv-vap", pst_net_dev->name);
        OAM_WARNING_LOG1(0, OAM_SF_CALIBRATE, "wal_send_cali_matrix_data:netdev[%p] have no dev priv-vap", pst_net_dev);

        return OAL_FAIL;
    }
#endif
    return OAL_SUCC;
}

oal_uint32 wal_send_cali_data_etc(oal_net_device_stru *pst_net_dev)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mac_vap_stru               *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (pst_mac_vap != OAL_PTR_NULL)
    {
        hmac_send_cali_data_etc(pst_mac_vap);
    }
    else
    {
        OAL_IO_PRINT("wal_send_cali_data_etc:netdev name %s not have dev priv-vap", pst_net_dev->name);
        OAM_WARNING_LOG1(0, OAM_SF_CALIBRATE, "wal_send_cali_data_etc:netdev[%p] have no dev priv-vap", pst_net_dev);

        return OAL_FAIL;
    }
#endif
    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 wal_config_set_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_max_user;

    ul_max_user = *((oal_uint32 *)puc_param);

    return hmac_config_set_max_user_etc(pst_mac_vap, us_len, ul_max_user);
}

#ifdef _PRE_WLAN_FEATURE_11R

OAL_STATIC oal_uint32  wal_config_set_ft_ies(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_ft_ies_etc(pst_mac_vap, us_len, puc_param);
}
#endif //_PRE_WLAN_FEATURE_11R
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
OAL_STATIC oal_uint32  wal_config_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_cali_power(pst_mac_vap, us_len, puc_param);
}
OAL_STATIC oal_uint32  wal_config_get_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_cali_power(pst_mac_vap, us_len, puc_param);
}
OAL_STATIC oal_uint32  wal_config_set_polynomial_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_polynomial_param(pst_mac_vap, us_len, puc_param);
}
OAL_STATIC oal_uint32  wal_config_get_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_upc_params(pst_mac_vap, us_len, puc_param);
}
OAL_STATIC oal_uint32  wal_config_set_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_upc_params(pst_mac_vap, us_len, puc_param);
}
OAL_STATIC oal_uint32  wal_config_set_load_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_load_mode(pst_mac_vap, us_len, puc_param);
}
#endif

OAL_STATIC oal_uint32  wal_config_get_dieid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_dieid(pst_mac_vap, us_len, puc_param);
}
#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)

OAL_STATIC oal_uint32  wal_config_auto_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_config_auto_cali(pst_mac_vap, us_len, puc_param);

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_config_get_cali_staus(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_get_cali_staus::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_get_cali_status(pst_mac_vap, pus_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_get_cali_staus::hmac_config_get_cali_status failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  wal_config_set_cali_vref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_config_set_cali_vref(pst_mac_vap, us_len, puc_param);

    return OAL_SUCC;
}
#endif
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

OAL_STATIC oal_uint32  wal_config_load_ini_power_gain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_config_load_ini_power_gain(pst_mac_vap, us_len, puc_param);

    return OAL_SUCC;
}




OAL_STATIC oal_uint32  wal_config_set_all_log_level(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_all_log_level_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_cus_rf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_cus_rf_etc(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_set_cus_dts_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_cus_dts_cali_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_cus_nvram_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_cus_nvram_params_etc(pst_mac_vap, us_len, puc_param);
}

/* show dev customize info */
OAL_STATIC oal_uint32  wal_config_dev_customize_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dev_customize_info_etc(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_cus_dyn_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_cus_dyn_cali(pst_mac_vap, us_len, puc_param);
}

#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef _PRE_WLAN_FEATURE_HILINK

OAL_STATIC oal_uint32  wal_config_fbt_kick_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;
    oal_uint32                      ul_ret;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{wal_config_fbt_kick_user:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)puc_param;

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_fbt_kick_user::uc_rej_user[%d], uc_kick_user[%d], mac[%x%x]}\r\n",
                                pst_kick_user_param->uc_rej_user,
                                pst_kick_user_param->uc_kick_user,
                                pst_kick_user_param->auc_mac_addr[4],
                                pst_kick_user_param->auc_mac_addr[5]);

    /* 根据rej参数，进行禁止用户连接的管理 */
    ul_ret = hmac_config_fbt_rej_user(pst_mac_vap, us_len, puc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_kick_user_etc::hmac_fbt_add_disable_user failed[%d].}",ul_ret);
        return ul_ret;
    }

    /* 如果非广播地址，并且kick=1，则调用kick user，剔除该用户 */
    if (OAL_FALSE == oal_is_broadcast_ether_addr(pst_kick_user_param->auc_mac_addr))
    {
        if (OAL_TRUE == pst_kick_user_param->uc_kick_user)
        {
            ul_ret = hmac_config_kick_user_etc(pst_mac_vap, us_len, puc_param);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_fbt_kick_user::hmac_config_kick_user_etc failed[%d].}",ul_ret);
            }
        }
    }

    return ul_ret;
}


oal_uint32 wal_config_set_white_lst_ssidhiden(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_white_lst_ssidhiden(pst_mac_vap, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT


oal_uint32 wal_config_set_mgmt_frame_filters(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_mgmt_frame_filters(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_get_sta_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_all_sta_diag_info(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_get_vap_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_vap_diag_info(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_set_sensing_bssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_sensing_bssid(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_get_sensing_bssid_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_sensing_bssid_info(pst_mac_vap, us_len, puc_param);
}

#endif //_PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT


oal_uint32 wal_config_get_sta_11k_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    return hmac_config_get_sta_11k_abillty(pst_mac_vap, us_len, puc_param);
#else
    OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_config_get_sta_11k_abillty:: not support 11k}\r\n");
    return OAL_FAIL;
#endif
}



oal_uint32 wal_config_set_sta_bcn_request(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    return hmac_config_set_sta_bcn_request(pst_mac_vap, us_len, puc_param);
#else
    OAM_WARNING_LOG0(0, OAM_SF_CFG, "{wal_config_set_sta_bcn_request:: not support 11k}\r\n");
    return OAL_FAIL;
#endif
}


oal_uint32 wal_config_get_sta_11v_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_sta_11v_abillty(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_change_to_other_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_change_to_other_ap(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_get_cur_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_cur_channel(pst_mac_vap, puc_param);
}


oal_uint32 wal_config_set_stay_time(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_stay_time(pst_mac_vap, puc_param);
}


OAL_STATIC oal_uint32 wal_config_fbt_start_scan(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_start_scan(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE

oal_uint32 wal_config_send_action_frame(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_action_frame(pst_mac_vap, us_len, puc_param);
}

oal_uint32 wal_config_set_mgmt_frame_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_mgmt_frame_ie(pst_mac_vap, us_len, puc_param);
}

oal_uint32 wal_config_set_mgmt_cap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_mgmt_cap_info(pst_mac_vap, us_len, puc_param);
}
#endif  // _PRE_WLAN_FEATURE_11KV_INTERFACE


oal_uint32 wal_config_set_vendor_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_vendor_ie(pst_mac_vap, us_len, puc_param);
}


oal_uint32 wal_config_get_sta_11h_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_sta_11h_abillty(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_11R_AP

oal_uint32 wal_config_get_sta_11r_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_sta_11r_abillty(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32 wal_config_set_mlme(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{wal_config_set_mlme:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_config_set_mlme(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_vap_destroy(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru *pst_hmac_vap;
    pst_hmac_vap  = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    return hmac_vap_destroy_etc(pst_hmac_vap);
}

#ifdef _PRE_WLAN_FEATURE_11K
OAL_STATIC oal_uint32  wal_config_send_neighbor_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_neighbor_req_etc(pst_mac_vap, us_len, puc_param);
}
#endif //_PRE_WLAN_FEATURE_11K

OAL_STATIC oal_uint32  wal_config_vendor_cmd_get_channel_list(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_vendor_cmd_get_channel_list_etc(pst_mac_vap, pus_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_WDS

OAL_STATIC oal_uint32  wal_config_wds_get_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    return hmac_config_wds_get_vap_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_wds_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wds_vap_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_wds_vap_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wds_vap_show(pst_mac_vap, us_len, OAL_PTR_NULL);
}


OAL_STATIC oal_uint32  wal_config_wds_get_sta_num(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    return hmac_config_wds_get_sta_num(pst_mac_vap, us_len, puc_param);
}


oal_uint32  wal_config_get_wds_vap_info(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_wds_vap_info::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_EFAUL;
    }

    return hmac_config_wds_vap_show(pst_mac_vap, OAL_SIZEOF(oal_wds_info_stru), (oal_uint8*)(&pst_ioctl_data->pri_data.st_wds_info));
}


OAL_STATIC oal_uint32  wal_config_wds_sta_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wds_sta_add(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_wds_sta_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wds_sta_del(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_wds_sta_age(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wds_sta_age(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_11K_STAT
OAL_STATIC oal_uint32  wal_config_query_stat_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_query_stat_info(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
OAL_STATIC oal_uint32  wal_config_send_radio_meas_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_radio_meas_req(pst_mac_vap, us_len, puc_param);
}
OAL_STATIC oal_uint32  wal_config_set_11k_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_11k_switch(pst_mac_vap, us_len, puc_param);
}

#endif


OAL_STATIC oal_uint32  wal_config_get_wmmswitch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_wmmswitch(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_vap_wmm_switch(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_set_vap_wmm_switch::pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 针对配置vap做保护 */
    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_vap_wmm_switch::this is config vap! can't get info.}");
        return OAL_FAIL;
    }

    return hmac_config_set_vap_wmm_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_bg_noise(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
   return hmac_config_bg_noise_info(pst_mac_vap, pus_len, puc_param);
}



OAL_STATIC oal_uint32 wal_config_get_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_max_user(pst_mac_vap, pus_len, puc_param);
}

#ifdef _PRE_WLAN_WEB_CMD_COMM

OAL_STATIC oal_uint32  wal_config_set_hw_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                     ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_set_hw_addr::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_hw_addr(pst_mac_vap, us_len, puc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_hw_addr:: return error code %d.}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_set_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_hide_ssid(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_hide_ssid(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_global_shortgi(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_global_shortgi(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_txbeamform(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_txbeamform(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_noforward(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_noforward(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_priv_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mib_by_bw_param_stru st_cfg;

    if (!pst_mac_vap || !puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{wal_config_set_mode::null ptr, vap=%p param=%p", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_cfg.en_band = ((mac_cfg_mode_param_stru *)puc_param)->en_band;
    st_cfg.en_bandwidth= ((mac_cfg_mode_param_stru *)puc_param)->en_bandwidth;

    hmac_config_set_mib_by_bw(pst_mac_vap, (oal_uint16)OAL_SIZEOF(st_cfg), (oal_uint8 *)&st_cfg);

    return hmac_config_priv_set_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_channel(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_channel(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_priv_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_priv_set_channel(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_assoc_num(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_assoc_num(pst_mac_vap, pus_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING

OAL_STATIC oal_uint32  wal_config_set_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_bsd(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_bsd(pst_mac_vap, pus_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_get_pwr_ref(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_pwr_ref(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_ko_version(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_ko_version(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_rate_info(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_rate_info(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_chan_list(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_chan_list(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_wps_ie(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_vap_cap(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_vap_cap(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_pmf(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_pmf(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_bcast_rate(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_bcast_rate(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_noise(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_noise(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_bw(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_bw(pst_mac_vap, pus_len, puc_param);
}


oal_int32 wal_config_get_sta_rssi(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_mac, oal_uint8 *puc_param)
{
    mac_vap_stru               *pst_vap;

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_wmm_params_etc::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    return hmac_config_get_sta_rssi(pst_vap, puc_mac, puc_param);
}


OAL_STATIC oal_uint32 wal_config_get_band(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_band(pst_mac_vap, pus_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_get_scan_stat(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_scan_stat(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_neighbor_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_neighbor_scan(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_neighb_no(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_neighb_no(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_ant_rssi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_get_ant_rssi(pst_mac_vap, pus_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_ant_rssi_report(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_ant_rssi_report(pst_mac_vap, us_len, puc_param);
}


 oal_int32  wal_config_get_neighb_info(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_neighb_info::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_EFAUL;
    }

    return hmac_config_get_neighb_info(pst_mac_vap, &(pst_ioctl_data->pri_data.st_ap_info));
}


 oal_int32  wal_config_get_hw_flow_stat(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_hw_flow_stat::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_EFAUL;
    }

    return hmac_config_get_hw_flow_stat(pst_mac_vap, &(pst_ioctl_data->pri_data.st_machw_stat));
}


 oal_int32  wal_config_get_wme_stat(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_wme_stat::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_EFAUL;
    }

    return hmac_config_get_wme_stat(pst_mac_vap, &(pst_ioctl_data->pri_data.st_wme_stat));
}

#ifdef _PRE_WLAN_11K_STAT

oal_uint32 *wal_config_get_sta_drop_num(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_mac, oal_uint8 *puc_param)
{
    mac_vap_stru               *pst_vap;

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_wmm_params_etc::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return OAL_PTR_NULL;
    }
    return hmac_config_get_sta_drop_num(pst_vap, puc_mac, puc_param);
}


oal_uint32 *wal_config_get_sta_tx_delay(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_mac, oal_uint8 *puc_param)
{
    mac_vap_stru               *pst_vap;

    pst_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_wmm_params_etc::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr.}\r\n");
        return OAL_PTR_NULL;
    }
    return hmac_config_get_sta_tx_delay(pst_vap, puc_mac, puc_param);
}


 oal_int32  wal_config_get_tx_delay_ac(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_tx_delay_ac::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_EFAUL;
    }

    return hmac_config_get_tx_delay_ac(pst_mac_vap, &(pst_ioctl_data->pri_data.st_tx_delay_ac));
}
#endif

#ifdef _PRE_WLAN_FEATURE_DFS

oal_uint32  wal_config_get_dfs_chn_status(oal_net_device_stru *pst_net_dev, oal_uint8 *puc_param)
{
    mac_vap_stru                   *pst_mac_vap;
    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{wal_config_get_dfs_chn_status::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_EFAUL;
    }

    return hmac_config_get_dfs_chn_status(pst_mac_vap, puc_param);
}
#endif

#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)

 oal_int32  wal_config_get_snoop_table(oal_net_device_stru *pst_net_dev, oal_net_dev_ioctl_data_stru *pst_ioctl_data)
{
    mac_vap_stru                   *pst_mac_vap;

    pst_mac_vap = OAL_NET_DEV_PRIV(pst_net_dev);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{wal_config_get_snoop_table::OAL_NET_DEV_PRIV(pst_net_dev) is null ptr!}\r\n");
        return OAL_EFAUL;
    }

    return hmac_config_get_snoop_table(pst_mac_vap, &(pst_ioctl_data->pri_data.st_all_snoop_group));
}
#endif

#ifdef _PRE_WLAN_FEATURE_M2S

OAL_STATIC oal_uint32  wal_config_set_m2s_switch_blacklist(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_m2s_switch_blacklist(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_set_m2s_switch_mss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_m2s_switch_mss(pst_mac_vap, us_len, puc_param);
}

#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifndef CONFIG_HAS_EARLYSUSPEND

OAL_STATIC oal_uint32  wal_config_set_suspend_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru    *pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (!pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "wal_config_set_suspend_mode:pst_mac_device is null ptr!");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 0:亮屏 1:暗屏 */
    if (0 == *puc_param)
    {
        hmac_do_suspend_action_etc(pst_mac_device, OAL_FALSE);
    }
    else
    {
        hmac_do_suspend_action_etc(pst_mac_device, OAL_TRUE);
    }

    return OAL_SUCC;
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_APF

OAL_STATIC oal_uint32  wal_config_apf_filter_cmd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_apf_filter_cmd(pst_mac_vap, us_len, puc_param);
}

#endif


OAL_STATIC oal_uint32  wal_config_remove_app_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_remove_app_ie(pst_mac_vap, us_len, puc_param);
}

#ifdef  _PRE_WLAN_FEATURE_FORCE_STOP_FILTER

OAL_STATIC oal_uint32  wal_config_force_stop_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_force_stop_filter(pst_mac_vap, us_len, puc_param);
}
#endif

/*lint -e19*/
oal_module_symbol(wal_drv_cfg_func_hook_init_etc);
oal_module_symbol(wal_config_set_vendor_ie);
#ifdef _PRE_WLAN_FEATURE_HILINK
oal_module_symbol(wal_config_fbt_kick_user);
#endif
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

