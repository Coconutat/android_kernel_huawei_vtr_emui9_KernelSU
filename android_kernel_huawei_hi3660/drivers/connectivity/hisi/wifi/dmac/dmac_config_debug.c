


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_sdio_comm.h"
#include "oal_sdio.h"
#include "oal_types.h"
#include "oal_pci_if.h"
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hal_mac.h"
#include "hal_device.h"
#endif
#include "oal_mem.h"
#ifdef _PRE_WLAN_ALG_ENABLE
#include "alg_ext_if.h"
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "alg_tpc.h"
#endif
#endif
#include "mac_data.h"
#include "mac_device.h"
#include "mac_ie.h"
#include "mac_regdomain.h"
#include "mac_vap.h"
#include "dmac_ext_if.h"
#include "dmac_main.h"
#include "dmac_vap.h"
#include "dmac_psm_ap.h"
#include "dmac_rx_data.h"
#include "dmac_mgmt_classifier.h"
#include "dmac_mgmt_sta.h"
#include "dmac_tx_complete.h"
#include "dmac_tx_bss_comm.h"
#include "dmac_uapsd.h"
#include "dmac_blockack.h"
#include "dmac_beacon.h"
#include "dmac_user.h"
#include "dmac_11i.h"
#include "dmac_wep.h"
#include "dmac_uapsd.h"
#include "dmac_acs.h"
#include "dmac_reset.h"
#include "dmac_config.h"
#include "dmac_stat.h"
#include "dmac_data_acq.h"
#include "dmac_user_track.h"
#include "dmac_mgmt_bss_comm.h"
#include "dmac_txopps.h"
#include "dmac_dft.h"
#include "dmac_ap_pm.h"
#include "dmac_beacon.h"
#include "dmac_scan.h"
#include "dmac_psm_ap.h"
#include "dmac_device.h"
#include "dmac_resource.h"

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV))
#include "cali_data.h"
#include "pm_extern.h"
#endif
#ifdef _PRE_WLAN_CHIP_TEST
#include "dmac_test_main.h"
#include "dmac_lpm_test.h"
#include "dmac_frame_filter_test.h"
#include "dmac_wmm_test.h"
#endif
#ifdef _PRE_WLAN_DFT_STAT
#include "hal_witp_phy_reg.h"
#include "hal_witp_pa_reg.h"
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#include "hal_phy_reg.h"
#include "hal_mac_reg.h"
#endif
#endif

#ifdef _PRE_WIFI_DMT
#include "hal_witp_dmt_if.h"
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
#include "dmac_psm_sta.h"
#include "pm_extern.h"
#include "dmac_sta_pm.h"
#endif

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

#include "dmac_11w.h"
#endif
#include "dmac_chan_mgmt.h"

#include "dmac_reset.h"
#include "oal_net.h"
#include "dmac_config.h"
#include "dmac_main.h"
#include "dmac_rx_filter.h"

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "dmac_btcoex.h"
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
#include "core_cr4.h"
#endif
#include "oal_profiling.h"

#ifdef _PRE_SUPPORT_ACS
#include "dmac_acs.h"
#endif
#ifdef _PRE_WLAN_DFT_STAT
#include "mac_board.h"
#endif

#include "dmac_arp_offload.h"

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#include "dmac_auto_adjust_freq.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_CONFIG_DEBUG_C

mac_rssi_debug_switch_stru  g_st_rssi_debug_switch_info = {0};

#ifdef _PRE_WLAN_CFGID_DEBUG
oal_uint32 g_ul_al_mpdu_len                = 1510; /*指示常发mpdu长度， 可根据实际情况修改*/
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)
extern oal_void  hi1102_rf_cali_read_phy_reg_bits(
                oal_uint32          ul_reg_addr,
                oal_uint8           uc_offset,
                oal_uint8           uc_bits,
                oal_uint32         *pul_reg_val);

extern oal_void  hi1102_rf_cali_write_phy_reg_bits(
                oal_uint32          ul_reg_addr,
                oal_uint8           uc_offset,
                oal_uint8           uc_bits,
                oal_uint32          ul_reg_val);
extern oal_uint32  hi1102_rf_cali_get_scaling_addr(
                oal_uint8 uc_offset_addr_a);
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
extern oal_void  hi1103_rf_cali_read_phy_reg_bits(
                oal_uint32          ul_reg_addr,
                oal_uint8           uc_offset,
                oal_uint8           uc_bits,
                oal_uint32         *pul_reg_val);

extern oal_void  hi1103_rf_cali_write_phy_reg_bits(
                oal_uint32          ul_reg_addr,
                oal_uint8           uc_offset,
                oal_uint8           uc_bits,
                oal_uint32          ul_reg_val);
extern oal_uint32  hi1103_rf_cali_get_scaling_addr(
                oal_uint8 uc_offset_addr_a);
#endif



OAL_STATIC oal_uint32 dmac_config_set_stbc_cap(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    oal_bool_enum_uint8   uc_value;

    uc_value = (oal_bool_enum_uint8)*puc_param;

    if (uc_value > 1)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_stbc_cap::stbc_value is limit! value = [%d].}", uc_value);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxSTBCOptionImplemented                = uc_value;
    pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11RxSTBCOptionImplemented                = uc_value;
    pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11TxSTBCOptionActivated                  = uc_value;
    pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTTxSTBCOptionImplemented   = uc_value;
    pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTRxSTBCOptionImplemented   = uc_value;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_config_set_ldpc_cap(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8   uc_value;

    uc_value = (oal_bool_enum_uint8)(*puc_param);

    if (uc_value > 1)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_ldpc_cap::ldpc_value is limit! value = [%d].}\r\n", uc_value);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11LDPCCodingOptionImplemented                = uc_value;
    pst_mac_vap->pst_mib_info->st_phy_ht.en_dot11LDPCCodingOptionActivated                  = uc_value;
    pst_mac_vap->pst_mib_info->st_wlan_mib_phy_vht.en_dot11VHTLDPCCodingOptionImplemented   = uc_value;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_pause_tid(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_pause_tid_param_stru   *pst_pause_tid_param;
    dmac_user_stru                 *pst_dmac_user;
    oal_uint8                       uc_tid;
    mac_device_stru                *pst_mac_device;

    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)puc_param;
    uc_tid = pst_pause_tid_param->uc_tid;

    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_pause_tid_param->auc_mac_addr);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pause_tid::pst_dmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pause_tid::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if  (OAL_TRUE == pst_pause_tid_param->uc_is_paused)
    {
        dmac_tid_pause(&(pst_dmac_user->ast_tx_tid_queue[uc_tid]), DMAC_TID_PAUSE_RESUME_TYPE_BA);
    }
    else
    {
        dmac_tid_resume(pst_mac_device->pst_device_stru, &(pst_dmac_user->ast_tx_tid_queue[uc_tid]), DMAC_TID_PAUSE_RESUME_TYPE_BA);
    }

    return OAL_SUCC;
}
#ifdef _PRE_DEBUG_MODE
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  dmac_config_dump_timer(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    frw_timer_dump_timer(pst_mac_vap->ul_core_id);
    return OAL_SUCC;
}
#endif
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_user_vip(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_user_vip_param_stru    *pst_user_vip_param;
    dmac_user_stru                 *pst_dmac_user;

    pst_user_vip_param = (mac_cfg_user_vip_param_stru *)puc_param;

    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_user_vip_param->auc_mac_addr);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_user_vip::pst_dmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_user->en_vip_flag = pst_user_vip_param->uc_vip_flag;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_user_vip, vip_flag = %d}", pst_dmac_user->en_vip_flag);

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_vap_host(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_vap_stru                  *pst_dmac_vap;

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_set_vap_host::param null.}");
         return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap->en_is_host_vap = *puc_param;

    OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_vap_host, host_flag = %d}", pst_dmac_vap->en_is_host_vap);

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_send_bar(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_pause_tid_param_stru   *pst_pause_tid_param;
    dmac_user_stru                 *pst_dmac_user;
    oal_uint8                       uc_tid;
    dmac_tid_stru                  *pst_tid;

    pst_pause_tid_param = (mac_cfg_pause_tid_param_stru *)puc_param;
    uc_tid = pst_pause_tid_param->uc_tid;

    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_pause_tid_param->auc_mac_addr);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_bar::pst_dmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tid = &pst_dmac_user->ast_tx_tid_queue[uc_tid];

    if (OAL_PTR_NULL == pst_tid->pst_ba_tx_hdl || DMAC_BA_COMPLETE != pst_tid->pst_ba_tx_hdl->en_ba_conn_status)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_bar::ba not established.}");
        return OAL_FAIL;
    }

    return dmac_ba_send_bar(pst_tid->pst_ba_tx_hdl, pst_dmac_user, pst_tid);
}
#endif //#ifdef _PRE_DEBUG_MODE


oal_uint32  dmac_config_alg(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint32        ul_ret = OAL_FAIL;
    oal_uint8         uc_idx;

    oal_int8         *pac_argv[DMAC_ALG_CONFIG_MAX_ARG + 1];

    mac_ioctl_alg_config_stru *pst_alg_config = (mac_ioctl_alg_config_stru *)puc_param;

    for (uc_idx = OAL_SIZEOF(mac_ioctl_alg_config_stru); uc_idx < uc_len; uc_idx++)
    {
        if(puc_param[uc_idx] == ' ')
        {
            puc_param[uc_idx] = 0;
        }
    }

    for(uc_idx = 0; uc_idx < pst_alg_config->uc_argc; uc_idx++)
    {
        pac_argv[uc_idx] = (oal_int8 *)puc_param + OAL_SIZEOF(mac_ioctl_alg_config_stru) + pst_alg_config->auc_argv_offset[uc_idx];
    }

    pac_argv[uc_idx] = NULL;

    for(uc_idx = 0; uc_idx < DMAC_ALG_CONFIG_ID_BUTT; uc_idx++)
    {
        if(0 == oal_strcmp(pac_argv[0], g_ast_dmac_alg_config_table[uc_idx].puc_alg_name))
        {
            break;
        }
    }

    if (uc_idx == DMAC_ALG_CONFIG_ID_BUTT)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_alg: uc_idx error: %d}", uc_idx);

        return OAL_FAIL;
    }


    if(gst_alg_main.pa_alg_config_notify_func[uc_idx])
    {
        ul_ret = gst_alg_main.pa_alg_config_notify_func[uc_idx](pst_mac_vap, pst_alg_config->uc_argc - 1, pac_argv + 1);
    }
    else
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_alg::p_func null.}");
    }

    return ul_ret;
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_dump_ba_bitmap(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_get_mpdu_num(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_get_mpdu_num_stru   *pst_param;
    mac_device_stru             *pst_mac_dev;
    dmac_user_stru              *pst_dmac_user;
    oam_report_mpdu_num_stru     st_mpdu_num;

    pst_param = (mac_cfg_get_mpdu_num_stru *)puc_param;

    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_param->auc_user_macaddr);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{dmac_config_get_mpdu_num:: pst_dmac_user is Null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{dmac_config_get_mpdu_num:: pst_mac_dev is Null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_mpdu_num.us_total_mpdu_num_in_device = pst_mac_dev->us_total_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid0         = pst_dmac_user->ast_tx_tid_queue[0].us_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid1         = pst_dmac_user->ast_tx_tid_queue[1].us_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid2         = pst_dmac_user->ast_tx_tid_queue[2].us_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid3         = pst_dmac_user->ast_tx_tid_queue[3].us_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid4         = pst_dmac_user->ast_tx_tid_queue[4].us_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid5         = pst_dmac_user->ast_tx_tid_queue[5].us_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid6         = pst_dmac_user->ast_tx_tid_queue[6].us_mpdu_num;
    st_mpdu_num.us_mpdu_num_in_tid7         = pst_dmac_user->ast_tx_tid_queue[7].us_mpdu_num;

    return oam_report_mpdu_num(pst_dmac_user->st_user_base_info.auc_user_mac_addr, &st_mpdu_num);
}
#endif //#ifdef _PRE_DEBUG_MODE

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32 dmac_config_start_dpd(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

 dmac_start_dpd_calibration(pst_mac_vap);


   return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32 dmac_config_beacon_offload_test(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_sta_beacon_offload_test(pst_mac_vap, puc_param);
    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32 dmac_config_ota_beacon_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint8                                uc_vap_id_loop;
    oal_uint32                               ul_ret;
    oal_int32                                l_value;

    l_value = *((oal_int32 *)puc_param);

    for (uc_vap_id_loop = 0; uc_vap_id_loop < WLAN_VAP_SUPPOTR_MAX_NUM_SPEC; uc_vap_id_loop++)
    {
        ul_ret = oam_ota_set_beacon_switch(uc_vap_id_loop,
                                          (oam_sdt_print_beacon_rxdscr_type_enum_uint8)l_value);

        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(uc_vap_id_loop, OAM_SF_CFG, "{dmac_config_ota_beacon_switch::ota beacon switch set failed!}\r\n");
            return ul_ret;
        }
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_config_ota_rx_dscr_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint8                                uc_vap_id_loop;
    oal_uint32                               ul_ret;
    oal_int32                                l_value;

    l_value = *((oal_int32 *)puc_param);

    for (uc_vap_id_loop = 0; uc_vap_id_loop < WLAN_VAP_SUPPOTR_MAX_NUM_SPEC; uc_vap_id_loop++)
    {
          ul_ret = oam_ota_set_rx_dscr_switch(uc_vap_id_loop,
                                             (oal_switch_enum_uint8)l_value);

        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(uc_vap_id_loop, OAM_SF_CFG, "{dmac_config_ota_rx_dscr_switch::ota rx_dscr switch set failed!}\r\n");
            return ul_ret;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_config_set_all_ota(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_switch_enum_uint8      en_switch;

    en_switch = *((oal_switch_enum_uint8 *)puc_param);
    return oam_report_set_all_switch(en_switch);
}


OAL_STATIC oal_uint32 dmac_config_oam_output(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;

    l_value = *((oal_int32 *)puc_param);

    /* 设置OAM log模块的开关 */
    ul_ret = oam_set_output_type((oam_output_type_enum_uint8)l_value);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_oam_output::oam_set_output_type failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_config_probe_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#ifndef _PRE_WLAN_PROFLING_MIPS

    mac_cfg_probe_switch_stru       *pst_probe_switch;
    oal_uint32                       ul_ret = 0;

    pst_probe_switch = (mac_cfg_probe_switch_stru *)puc_param;

    ul_ret = oam_report_80211_probe_set_switch(pst_probe_switch->en_frame_direction,
                                               pst_probe_switch->en_frame_switch,
                                               pst_probe_switch->en_cb_switch,
                                               pst_probe_switch->en_dscr_switch);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_probe_switch::oam_report_80211_probe_set_switch failed[%d].}", ul_ret);
        return ul_ret;
    }
#endif

    return OAL_SUCC;
}

OAL_STATIC oal_uint32 dmac_config_80211_mcast_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_80211_mcast_switch_stru *pst_80211_switch_param;
    oal_uint32                       ul_ret = 0;

    pst_80211_switch_param = (mac_cfg_80211_mcast_switch_stru *)puc_param;

    ul_ret = oam_report_80211_mcast_set_switch(pst_80211_switch_param->en_frame_direction,
                                               pst_80211_switch_param->en_frame_type,
                                               pst_80211_switch_param->en_frame_switch,
                                               pst_80211_switch_param->en_cb_switch,
                                               pst_80211_switch_param->en_dscr_switch);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_80211_mcast_switch::oam_report_80211_mcast_set_switch failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_config_rssi_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_rssi_debug_switch_stru      *pst_rssi_switch;

    pst_rssi_switch = (mac_rssi_debug_switch_stru *)puc_param;

    /* 设置rssi开关信息 */
    oal_memcopy(&g_st_rssi_debug_switch_info, pst_rssi_switch, OAL_SIZEOF(mac_rssi_debug_switch_stru));

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_WMMAC

OAL_STATIC oal_uint32 dmac_config_wmmac_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint8                            uc_wmmac_switch;

    uc_wmmac_switch   = (oal_uint8)*puc_param;
    g_en_wmmac_switch = (uc_wmmac_switch == OAL_FALSE) ? OAL_FALSE : OAL_TRUE;

    return OAL_SUCC;
}
#endif

OAL_STATIC oal_uint32  dmac_config_80211_ucast_switch(mac_vap_stru *pst_mac_vap, oal_uint8 us_len, oal_uint8 *puc_param)
{
    mac_cfg_80211_ucast_switch_stru *pst_80211_switch_param;
    oal_uint16                       us_user_idx = 0;
    oal_uint32                       ul_ret;

    pst_80211_switch_param = (mac_cfg_80211_ucast_switch_stru *)puc_param;

    /* 广播地址，操作所有用户的单播帧开关 */
    if (ETHER_IS_BROADCAST(pst_80211_switch_param->auc_user_macaddr))
    {
        for (us_user_idx = 0; us_user_idx < WLAN_ACTIVE_USER_MAX_NUM + WLAN_MAX_MULTI_USER_NUM_SPEC; us_user_idx++)
        {
            oam_report_80211_ucast_set_switch(pst_80211_switch_param->en_frame_direction,
                                              pst_80211_switch_param->en_frame_type,
                                              pst_80211_switch_param->en_frame_switch,
                                              pst_80211_switch_param->en_cb_switch,
                                              pst_80211_switch_param->en_dscr_switch,
                                              us_user_idx);
        }
        return OAL_SUCC;
    }

    ul_ret = mac_vap_find_user_by_macaddr(pst_mac_vap,
                                          pst_80211_switch_param->auc_user_macaddr,
                                          &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_80211_ucast_switch::mac_vap_find_user_by_macaddr[%02X:XX:XX:%02X:%02X:%02X]failed !!}",
                        pst_80211_switch_param->auc_user_macaddr[0],
                        pst_80211_switch_param->auc_user_macaddr[3],
                        pst_80211_switch_param->auc_user_macaddr[4],
                        pst_80211_switch_param->auc_user_macaddr[5]);
        return ul_ret;
    }

    ul_ret = oam_report_80211_ucast_set_switch(pst_80211_switch_param->en_frame_direction,
                                               pst_80211_switch_param->en_frame_type,
                                               pst_80211_switch_param->en_frame_switch,
                                               pst_80211_switch_param->en_cb_switch,
                                               pst_80211_switch_param->en_dscr_switch,
                                               us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_80211_ucast_switch::Set switch of report_ucast failed[%d]!!frame_switch[%d], cb_switch[%d], dscr_switch[%d].}",
                ul_ret,
                pst_80211_switch_param->en_frame_switch,
                pst_80211_switch_param->en_cb_switch,
                pst_80211_switch_param->en_dscr_switch);
        return ul_ret;
    }
    return OAL_SUCC;
}
#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#ifndef _PRE_WLAN_PROFLING_MIPS

    mac_device_stru    *pst_device;
    oal_uint32                      ul_off_set = 0;
    oal_int8                        ac_name[DMAC_HIPRIV_CMD_NAME_MAX_LEN];
    dmac_reset_para_stru            st_reset_param;

    /* 复位硬件phy&mac: hipriv "Hisilicon0 reset_hw 0|1|2|3(all|phy|mac|debug) 0|1(reset phy reg) 0|1(reset mac reg)"*/
    /* 命令复用来debug显示lut表和寄存器:hipriv "Hisilicon0 reset_hw 3(debug) 0|1|2(all|mac reg|phy reg|lut) "*/

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    /* 设置配置命令参数 */
    if(OAL_PTR_NULL == pst_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{dmac_config_reset_hw::pst_device is null,id=%d}", pst_mac_vap->uc_device_id);

        return OAL_FAIL;
    }

    OAL_MEMZERO(ac_name, DMAC_HIPRIV_CMD_NAME_MAX_LEN);
    /* 获取复位类型*/
    dmac_get_cmd_one_arg((oal_int8*)puc_param, ac_name, &ul_off_set);
    st_reset_param.uc_reset_type = (oal_uint8)oal_atoi(ac_name);
    /* 偏移，取下一个参数 */
    puc_param = puc_param + ul_off_set;

    //if (st_reset_param.uc_reset_type <= HAL_RESET_HW_TYPE_MAC_PHY)
    if (st_reset_param.uc_reset_type < HAL_RESET_HW_TYPE_DUMP_MAC)
    {
        /* 获取是否复位phy reg */
        dmac_get_cmd_one_arg((oal_int8*)puc_param, ac_name, &ul_off_set);
        st_reset_param.uc_reset_phy_reg = (oal_uint8)oal_atoi(ac_name);
        /* 偏移，取下一个参数 */
        puc_param = puc_param + ul_off_set;

        /* 获取是否复位mac reg */
        dmac_get_cmd_one_arg((oal_int8*)puc_param, ac_name, &ul_off_set);
        st_reset_param.uc_reset_mac_reg = (oal_uint8)oal_atoi(ac_name);
        /* 偏移，取下一个参数 */
        puc_param = puc_param + ul_off_set;

        st_reset_param.uc_reset_mac_mod = HAL_RESET_MAC_ALL;    /* HAL_RESET_MAC_ALL*/
        st_reset_param.en_reason = DMAC_RESET_REASON_HW_ERR;           /*DMAC_RESET_REASON_CONFIG*/
        dmac_reset_hw(pst_device,(oal_uint8*)&st_reset_param);
    }
    else
    {

        /* 获取debug类型:0|1|2(all|mac reg|phy reg|lut) */
        dmac_get_cmd_one_arg((oal_int8*)puc_param, ac_name, &ul_off_set);
        st_reset_param.uc_debug_type= (oal_uint8)oal_atoi(ac_name);
        /* 偏移，取下一个参数 */
        puc_param = puc_param + ul_off_set;

        dmac_reset_debug_all(pst_device,&st_reset_param);
    }
#endif

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  dmac_config_dump_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#ifndef _PRE_WLAN_PROFLING_MIPS

    hal_rx_dscr_queue_header_stru   *pst_rx_dscr_queue;
    mac_device_stru                 *pst_mac_device;
    hal_to_dmac_device_stru         *pst_hal_device;
    oal_uint32                      *pul_curr_dscr;
    hal_rx_dscr_stru                *pst_hal_to_dmac_dscr;
    oal_uint32                       ul_value;
    oal_uint32                       ul_rx_dscr_len = 0;
#ifdef _PRE_DEBUG_MODE
    oal_uint32                       ul_loop;
    oal_uint32                      *pul_original_dscr;
    oal_uint32                       ul_dscr_num;
#endif

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_dump_rx_dscr::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = pst_mac_device->pst_device_stru;

    ul_value = *((oal_uint32 *)puc_param);

    if (ul_value >= HAL_RX_DSCR_QUEUE_ID_BUTT)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{dmac_config_dump_rx_dscr::invalid rx dscr queue[%d].}", ul_value);
        return OAL_FAIL;
    }

    hal_rx_get_size_dscr(pst_hal_device, &ul_rx_dscr_len);

#ifdef _PRE_DEBUG_MODE
    /* 将描述符原始地址打出来 */
    OAM_INFO_LOG0(0, OAM_SF_CFG, "the origin dscr phy addr and virtual addr:\n");

    if (HAL_RX_DSCR_NORMAL_PRI_QUEUE == ul_value)
    {
        pul_original_dscr = pst_hal_device->aul_nor_rx_dscr;
        ul_dscr_num       = HAL_NORMAL_RX_MAX_BUFFS;
    }
    else
    {
        pul_original_dscr = pst_hal_device->aul_hi_rx_dscr;
        ul_dscr_num       = HAL_HIGH_RX_MAX_BUFFS;
    }

    for (ul_loop = 0; ul_loop < ul_dscr_num; ul_loop++)
    {
        pul_curr_dscr = OAL_DSCR_PHY_TO_VIRT(pul_original_dscr[ul_loop]);
        OAM_INFO_LOG3(0, OAM_SF_CFG, "%2d 0x%08x, 0x%08x\n",
                     ul_loop,
                     pul_original_dscr[ul_loop],
                     (oal_uint32)pul_curr_dscr);

        oam_report_dscr(BROADCAST_MACADDR, (oal_uint8 *)pul_curr_dscr, (oal_uint16)ul_rx_dscr_len, OAM_OTA_TYPE_RX_DSCR);
    }

    OAM_INFO_LOG1(0, OAM_SF_CFG, "dscr exception free cnt is %d\n", pst_hal_device->ul_exception_free);
#endif

    pst_rx_dscr_queue = &(pst_hal_device->ast_rx_dscr_queue[ul_value]);

    pul_curr_dscr = pst_rx_dscr_queue->pul_element_head;

    //OAM_INFO_LOG1(0, OAM_SF_CFG, "the current dscr cnt is: %d\n", pst_rx_dscr_queue->us_element_cnt);
    //OAM_INFO_LOG1(0, OAM_SF_CFG, "head ptr virtual addr is: 0x%08x\n", (oal_uint32)pst_rx_dscr_queue->pul_element_head);
    //OAM_INFO_LOG1(0, OAM_SF_CFG, "tail ptr virtual addr is: 0x%08x\n", (oal_uint32)pst_rx_dscr_queue->pul_element_tail);

    OAM_INFO_LOG0(0, OAM_SF_CFG, "the current dscr addr and dscr's content:\n");
    while(OAL_PTR_NULL != pul_curr_dscr)
    {
        OAM_INFO_LOG2(0, OAM_SF_CFG, "virtual addr:0x%08x, phy addr:0x%08x\n", (oal_uint32)pul_curr_dscr, (oal_uint32)OAL_DSCR_VIRT_TO_PHY(HAL_RX_DSCR_GET_REAL_ADDR(pul_curr_dscr)));
        oam_report_dscr(BROADCAST_MACADDR, (oal_uint8 *)pul_curr_dscr, (oal_uint16)ul_rx_dscr_len, OAM_OTA_TYPE_RX_DSCR);

        pst_hal_to_dmac_dscr = (hal_rx_dscr_stru *)pul_curr_dscr;
        if (NULL != pst_hal_to_dmac_dscr->pul_next_rx_dscr)
        {
            pul_curr_dscr = HAL_RX_DSCR_GET_SW_ADDR((oal_uint32 *)OAL_DSCR_PHY_TO_VIRT((oal_uint)(pst_hal_to_dmac_dscr->pul_next_rx_dscr)));
        }
        else
        {
            pul_curr_dscr = HAL_RX_DSCR_GET_SW_ADDR(pst_hal_to_dmac_dscr->pul_next_rx_dscr);
        }
    }

    hal_rx_show_dscr_queue_info(pst_hal_device, (oal_uint8)ul_value);
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_dump_tx_dscr(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_mac_device;
    hal_to_dmac_device_stru         *pst_hal_device;
    oal_uint32                       ul_value;
    hal_tx_dscr_stru                *pst_dscr;
    oal_dlist_head_stru             *pst_pos;
    oal_netbuf_stru                 *pst_netbuf;
    mac_tx_ctl_stru                 *pst_tx_cb;
    oal_uint32                       ul_dscr_one_size = 0;
    oal_uint32                       ul_dscr_two_size = 0;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_dump_tx_dscr::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }


    pst_hal_device = pst_mac_device->pst_device_stru;

    ul_value = *((oal_uint32 *)puc_param);

    if (ul_value >= HAL_TX_QUEUE_NUM)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{dmac_config_dump_rx_dscr::invalid tx dscr queue[%d].}", ul_value);
        return OAL_FAIL;
    }

    OAL_DLIST_SEARCH_FOR_EACH(pst_pos, &(pst_hal_device->ast_tx_dscr_queue[ul_value].st_header))
    {
        pst_dscr   = OAL_DLIST_GET_ENTRY(pst_pos, hal_tx_dscr_stru, st_entry);
        pst_netbuf = pst_dscr->pst_skb_start_addr;
        pst_tx_cb  = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_netbuf);

        hal_tx_get_size_dscr(pst_hal_device, MAC_GET_CB_NETBUF_NUM(pst_tx_cb), &ul_dscr_one_size, &ul_dscr_two_size);
        oam_report_dscr(BROADCAST_MACADDR,
                        (oal_uint8 *)pst_dscr,
                        (oal_uint16)(ul_dscr_one_size + ul_dscr_two_size),
                        OAM_OTA_TYPE_TX_DSCR);
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE_USER_TRACK

OAL_STATIC oal_uint32  dmac_config_report_thrput_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_usr_thrput_stru             *pst_usr_thrput_param;
    dmac_user_stru                      *pst_dmac_user;

    pst_usr_thrput_param = (mac_cfg_usr_thrput_stru *)puc_param;

    /* 获取用户 */
    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_usr_thrput_param->auc_user_macaddr);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_report_thrput_stat::dmac_user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == pst_usr_thrput_param->uc_param)
    {
        /* 停止上报 */
        return dmac_user_track_clear_usr_thrput_stat(&pst_dmac_user->st_user_base_info);
    }
    else
    {
        return dmac_user_track_report_usr_thrput_stat(&pst_dmac_user->st_user_base_info);
    }
}
#endif

#ifdef _PRE_WLAN_DFT_STAT
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_report_irq_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

    mac_device_stru    *pst_device;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_show_irq_info::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    hal_show_irq_info(pst_device->pst_device_stru, *puc_param);
#endif
    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_phy_stat_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return OAL_SUCC;
}



OAL_STATIC oal_uint32  dmac_config_machw_stat_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_report_mgmt_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_mac_dev;
    oam_stats_mgmt_stat_stru         st_mgmt_stat;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_report_mgmt_stat::dev is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == *puc_param)
    {
        /* 清零mac统计的发送beacon帧数目、高优先级队列发送数目:MAC统计寄存器的bit4和bit9 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
        hal_reg_write(pst_mac_dev->pst_device_stru, (oal_uint32)HI1102_MAC_COUNTER_CLEAR_REG, 0x210);
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        hal_reg_write(pst_mac_dev->pst_device_stru, (oal_uint32)HI1103_MAC_COUNTER_CLEAR_REG, 0x210);
#endif

        /* 清零软件统计的管理帧收发数目 */
        OAL_MEMZERO(&pst_mac_dev->st_mgmt_stat, OAL_SIZEOF(mac_device_mgmt_statistic_stru));

        return OAL_SUCC;
    }
    else
    {
        /* 获取mac统计的发送beacon帧数目和高优先级队列发送数目 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV)
        hal_reg_info(pst_mac_dev->pst_device_stru, (oal_uint32)HI1102_MAC_TX_BCN_COUNT_REG, &st_mgmt_stat.ul_machw_stat_tx_bcn_cnt);
        hal_reg_info(pst_mac_dev->pst_device_stru, (oal_uint32)HI1102_MAC_TX_HI_PRI_MPDU_CNT_REG, &st_mgmt_stat.ul_machw_stat_tx_hi_cnt);
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV)
        hal_reg_info(pst_mac_dev->pst_device_stru, (oal_uint32)HI1103_MAC_TX_BCN_COUNT_REG, &st_mgmt_stat.ul_machw_stat_tx_bcn_cnt);
        hal_reg_info(pst_mac_dev->pst_device_stru, (oal_uint32)HI1103_MAC_TX_HI_PRI_MPDU_CNT_REG, &st_mgmt_stat.ul_machw_stat_tx_hi_cnt);
#endif

        /* 获取软件的管理帧收发统计 */
        oal_memcopy(st_mgmt_stat.aul_sw_rx_mgmt_cnt,
                    &pst_mac_dev->st_mgmt_stat,
                    OAL_SIZEOF(mac_device_mgmt_statistic_stru));

        /* 将统计值上报 */
        return oam_report_dft_params(BROADCAST_MACADDR, (oal_uint8 *)&st_mgmt_stat, (oal_uint16)OAL_SIZEOF(oam_stats_mgmt_stat_stru),OAM_OTA_TYPE_MGMT_STAT);
    }
}
#endif //#ifdef _PRE_DEBUG_MODE
#else
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_phy_stat_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oam_stats_phy_stat_stru         st_phy_stat;
    mac_device_stru                *pst_macdev;
    oal_uint8                       uc_loop;
    oal_uint32                      ul_reg_addr;
    oal_uint32                      ul_reg_val = 0;

    pst_macdev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_macdev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_phy_stat_info::dev is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(&st_phy_stat, OAL_SIZEOF(oam_stats_phy_stat_stru));

    /* 获取用户指定统计的4个phy统计值 */
    for (ul_reg_addr = (oal_uint32)WITP_PHY_PHY_STA0_RPT_REG, uc_loop = 0;
         ul_reg_addr <= (oal_uint32)WITP_PHY_PHY_STA3_RPT_REG && uc_loop < OAM_PHY_STAT_NODE_ENABLED_MAX_NUM;
         ul_reg_addr += 4, uc_loop++)
    {
        hal_reg_info(pst_macdev->pst_device_stru, ul_reg_addr, &ul_reg_val);
        st_phy_stat.aul_user_requested_stat_cnt[uc_loop] = ul_reg_val;
    }

    /* 获取phy按照协议模式统计的接收正确和错误帧个数 */
    for (ul_reg_addr = (oal_uint32)WITP_PHY_DOTB_OK_FRM_NUM_REG, uc_loop = 0;
         ul_reg_addr <= (oal_uint32)WITP_PHY_LEGA_ERR_FRM_NUM_REG && uc_loop < OAM_PHY_STAT_RX_PPDU_CNT;
         ul_reg_addr += 4, uc_loop++)
    {
        hal_reg_info(pst_macdev->pst_device_stru, ul_reg_addr, &ul_reg_val);
        st_phy_stat.aul_phy_stat_rx_ppdu_cnt[uc_loop] = ul_reg_val;
    }

    /* 将获取到的统计值上报 */
    return oam_report_dft_params(BROADCAST_MACADDR, (oal_uint8 *)&st_phy_stat,(oal_uint16)OAL_SIZEOF(oam_stats_phy_stat_stru), OAM_OTA_TYPE_PHY_STAT);
}



OAL_STATIC oal_uint32  dmac_config_machw_stat_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_mac_dev;
    oam_stats_machw_stat_stru        st_machw_stat;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_machw_stat_info::dev is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == *puc_param)
    {
        /* 清零mac统计,mac统计清零寄存器的bit2~bit22,bit4是发送beacon帧数目，bit9是高优先级队列发送数目，不清零，
           这两个统计值放在管理帧统计功能控制
        */
        hal_reg_write(pst_mac_dev->pst_device_stru, (oal_uint32)WITP_PA_COUNTER_CLEAR_REG, 0x7FFDEC);

        return OAL_SUCC;
    }
    else
    {
        OAL_MEMZERO(&st_machw_stat, OAL_SIZEOF(oam_stats_machw_stat_stru));

        /* 从MAC寄存器获取统计值 */
        dmac_dft_get_machw_stat_info(pst_mac_dev->pst_device_stru, &st_machw_stat);
        return oam_report_dft_params(BROADCAST_MACADDR, (oal_uint8 *)&st_machw_stat,(oal_uint16)OAL_SIZEOF(oam_stats_machw_stat_stru), OAM_OTA_TYPE_MACHW_STAT);
    }
}


OAL_STATIC oal_uint32  dmac_config_report_mgmt_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_mac_dev;
    oam_stats_mgmt_stat_stru         st_mgmt_stat;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_report_mgmt_stat::dev is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == *puc_param)
    {
        /* 清零mac统计的发送beacon帧数目、高优先级队列发送数目:MAC统计寄存器的bit4和bit9 */
        hal_reg_write(pst_mac_dev->pst_device_stru, (oal_uint32)WITP_PA_COUNTER_CLEAR_REG, 0x210);

        /* 清零软件统计的管理帧收发数目 */
        OAL_MEMZERO(&pst_mac_dev->st_mgmt_stat, OAL_SIZEOF(mac_device_mgmt_statistic_stru));

        return OAL_SUCC;
    }
    else
    {
        /* 获取mac统计的发送beacon帧数目和高优先级队列发送数目 */
        hal_reg_info(pst_mac_dev->pst_device_stru, (oal_uint32)WITP_PA_TX_BCN_COUNT_REG, &st_mgmt_stat.ul_machw_stat_tx_bcn_cnt);
        hal_reg_info(pst_mac_dev->pst_device_stru, (oal_uint32)WITP_PA_TX_HI_PRI_MPDU_CNT_REG, &st_mgmt_stat.ul_machw_stat_tx_hi_cnt);

        /* 获取软件的管理帧收发统计 */
        oal_memcopy(st_mgmt_stat.aul_sw_rx_mgmt_cnt,
                    &pst_mac_dev->st_mgmt_stat,
                    OAL_SIZEOF(mac_device_mgmt_statistic_stru));

        /* 将统计值上报 */
        return oam_report_dft_params(BROADCAST_MACADDR, (oal_uint8 *)&st_mgmt_stat,(oal_uint16)OAL_SIZEOF(oam_stats_mgmt_stat_stru), OAM_OTA_TYPE_MGMT_STAT);
    }
}
#endif //#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  dmac_config_usr_queue_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_usr_queue_param_stru        *pst_usr_queue_param;
    dmac_user_stru                      *pst_dmac_user;

    pst_usr_queue_param = (mac_cfg_usr_queue_param_stru *)puc_param;

    /* 获取用户 */
    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_usr_queue_param->auc_user_macaddr);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_usr_queue_stat::dmac_user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == pst_usr_queue_param->uc_param)
    {
        /* 清空用户队列统计信息 */
        return dmac_dft_clear_usr_queue_stat(pst_dmac_user);
    }
    else
    {
        return dmac_dft_report_usr_queue_stat(pst_dmac_user);
    }
}

#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_report_vap_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_vap_stru               *pst_dmac_vap;
    dmac_vap_query_stats_stru    st_vap_dft_stats;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{dmac_config_report_vap_stat::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_OS_VERSION == _PRE_OS_VERSION_RAW)
    OAM_WARNING_LOG_ALTER(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{dmac_config_report_vap_stat::drv_tx_pkts[%lu], hw_tx_pkts_succ[%lu], tx_fail[%lu], rx_mpdus[%lu], rx_drops[%lu], rx_null[%lu], rx_mgmt[%lu]}", 7,
                        pst_dmac_vap->st_query_stats.ul_drv_tx_pkts, pst_dmac_vap->st_query_stats.ul_hw_tx_pkts, pst_dmac_vap->st_query_stats.ul_tx_failed,
                        pst_dmac_vap->st_query_stats.ul_rx_mpdu_total_num,
                        (pst_dmac_vap->st_query_stats.ul_rx_dropped_misc + pst_dmac_vap->st_query_stats.ul_rx_alg_process_dropped + pst_dmac_vap->st_query_stats.ul_rx_security_check_fail_dropped),
                        pst_dmac_vap->st_query_stats.ul_rx_null_frame_dropped,
                        pst_dmac_vap->st_query_stats.ul_rx_mgmt_mpdu_num_cnt);
#endif
    oal_memcopy(&st_vap_dft_stats,
                &(pst_dmac_vap->st_query_stats),
                OAL_SIZEOF(dmac_vap_query_stats_stru));

    return oam_report_dft_params(BROADCAST_MACADDR, (oal_uint8 *)&st_vap_dft_stats,(oal_uint16)OAL_SIZEOF(dmac_vap_query_stats_stru),OAM_OTA_TYPE_VAP_STAT);
}
#endif //#ifdef _PRE_DEBUG_MODE

#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_dfr_enable(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru             *pst_mac_dev;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(0, OAM_SF_TX, "dmac_config_dfr_enable: mac_res_get_dev null: uc_device_id = %d", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev->pst_device_stru->en_dfr_enable = *puc_param;

    OAM_WARNING_LOG1(0, OAM_SF_CFG, "dmac_config_dfr_enable enter, dfr_enable = %d\r\n", pst_mac_dev->pst_device_stru->en_dfr_enable);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_trig_loss_tx_comp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_dev;

    pst_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_dev)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "dmac_config_trig_loss_tx_comp: mac_res_get_dev null: uc_device_id = %d", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dev->pst_device_stru->ul_cfg_loss_tx_comp_cnt = *((oal_uint32 *)puc_param);

    OAL_IO_PRINT("dmac_config_trig_loss_tx_comp enter, cnt = %d\r\n", pst_dev->pst_device_stru->ul_cfg_loss_tx_comp_cnt);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "dmac_config_trig_loss_tx_comp enter, cnt = %d\r\n", pst_dev->pst_device_stru->ul_cfg_loss_tx_comp_cnt);

    return OAL_SUCC;
}

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

OAL_STATIC oal_uint32  dmac_config_trig_pcie_reset(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    OAM_WARNING_LOG0(0, OAM_SF_CFG, "dmac_config_trig_pcie_reset succ");
    OAL_IO_PRINT("dmac_config_trig_pcie_reset succ\r\n");
    return  oal_pci_hand_reset(pst_mac_vap->uc_chip_id);
}
#endif
#endif

#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_phy_stat_en(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oam_stats_phy_node_idx_stru     *pst_phy_node_idx;
    mac_device_stru                 *pst_mac_dev;

    pst_phy_node_idx = (oam_stats_phy_node_idx_stru *)puc_param;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_phy_stat_en::dev is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    return dmac_dft_set_phy_stat_node(pst_mac_dev, pst_phy_node_idx);
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_dbb_env_param(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    if (OAL_FALSE == *puc_param)
    {
        /* 停止上报，并清除资源 */
        return dmac_dft_stop_report_dbb_env(pst_mac_vap);
    }
    else
    {
        /* 开始周期(20ms)采集,周期(2s)上报 */
        return dmac_dft_start_report_dbb_env(pst_mac_vap);
    }
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_report_all_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int8             *pc_token_type;
    oal_int8             *pc_token;
    oal_int8             *pc_end;
    oal_int8             *pc_ctx;
    oal_int8             *pc_sep = " ";
    oal_uint8             uc_val;

    /* 获取要读取的寄存器类型 */
    pc_token_type = oal_strtok((oal_int8 *)puc_param, pc_sep, &pc_ctx);
    if (NULL == pc_token_type)
    {
        return OAL_FAIL;
    }
    pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
    if (NULL == pc_token)
    {
        return OAL_FAIL;
    }

    uc_val = (oal_uint8)oal_strtol(pc_token, &pc_end, 16);

    if (0 == oal_strcmp(pc_token, "soc"))
    {
        if (OAL_TRUE == uc_val)
        {
            dmac_config_phy_stat_info(pst_mac_vap, uc_len, &uc_val);
        }
    }
    else if (0 == oal_strcmp(pc_token, "machw"))
    {
        dmac_config_machw_stat_info(pst_mac_vap, uc_len, &uc_val);
    }
    else if (0 == oal_strcmp(pc_token, "mgmt"))
    {
        dmac_config_report_mgmt_stat(pst_mac_vap, uc_len, &uc_val);
    }
    else if (0 == oal_strcmp(pc_token, "irq"))
    {
        dmac_config_report_irq_info(pst_mac_vap, uc_len, &uc_val);
    }
    else
    {
        dmac_config_report_irq_info(pst_mac_vap, uc_len, &uc_val);
        dmac_config_report_mgmt_stat(pst_mac_vap, uc_len, &uc_val);
        dmac_config_machw_stat_info(pst_mac_vap, uc_len, &uc_val);
        if (OAL_TRUE == uc_val)
        {
            dmac_config_phy_stat_info(pst_mac_vap, uc_len, &uc_val);
        }
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 dmac_config_report_vap_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_DFT_STAT
    oal_uint32               ul_flags_value;

    /* 参数合法性判断 */
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{dmac_config_report_vap_info::input params is invalid, %p, %p.}",
                         pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    ul_flags_value = *(oal_uint32 *)puc_param;

    OAM_WARNING_LOG2(0, OAM_SF_ANY, "{dmac_config_report_vap_info::vap_id[%d], flags_value[0x%08x].}",
                     pst_mac_vap->uc_vap_id, ul_flags_value);

    /* 上报硬件信息 */
    if (ul_flags_value & MAC_REPORT_INFO_FLAGS_HARDWARE_INFO)
    {
        dmac_dft_report_mac_hardware_info(pst_mac_vap);
    }

    /* 上报队列信息 */
    if (ul_flags_value & MAC_REPORT_INFO_FLAGS_QUEUE_INFO)
    {
        dmac_dft_report_dmac_queue_info(pst_mac_vap);
    }

    /* 上报内存信息 */
    if (ul_flags_value & MAC_REPORT_INFO_FLAGS_MEMORY_INFO)
    {
        dmac_dft_report_dmac_memory_info(pst_mac_vap);
    }

    /* 上报事件信息 */
    if (ul_flags_value & MAC_REPORT_INFO_FLAGS_EVENT_INFO)
    {
        dmac_dft_report_dmac_event_info(pst_mac_vap);
    }

    /* 上报vap信息 */
    if (ul_flags_value & MAC_REPORT_INFO_FLAGS_VAP_INFO)
    {
        dmac_dft_report_dmac_vap_info(pst_mac_vap);
    }

    /* 上报用户信息 */
    if (ul_flags_value & MAC_REPORT_INFO_FLAGS_USER_INFO)
    {
        dmac_dft_report_dmac_user_info(pst_mac_vap);
    }

    /* 上报收发包统计信息 */
    if (ul_flags_value & MAC_REPORT_INFO_FLAGS_TXRX_PACKET_STATISTICS)
    {
        dmac_config_report_vap_stat(pst_mac_vap, uc_len, puc_param);
    }
#else
    OAM_WARNING_LOG0(0, OAM_SF_ANY,
                     "{dmac_config_report_vap_info::DFT macro switch is not open, do nothing.}");
#endif

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif


OAL_STATIC oal_uint32 dmac_config_set_feature_log(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    // 设置device log level，业务添加处理逻辑
    oal_uint16      us_param;
    us_param = *(oal_uint16 *)puc_param;
    return oam_log_set_feature_level(pst_mac_vap->uc_vap_id, (oal_uint8)(us_param>>8), (oal_uint8)us_param) ;
}

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST

OAL_STATIC oal_uint32 dmac_config_chip_check(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_vap_stru                   *pst_dmac_vap;
    hal_to_dmac_device_stru         *pst_hal_device;
    oal_uint32                       ul_result;
    oal_uint8                        uc_hipriv_ack = OAL_FALSE;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_chip_check::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = pst_dmac_vap->pst_hal_device;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_chip_check::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 写测试寄存器与读测试结果寄存器，检查寄存器配置是否生效*/
    hal_check_test_value_reg(pst_hal_device, 0xf000, &ul_result);

    if (0xf != ul_result)
    {
        uc_hipriv_ack = 0xFF;
        dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8), &uc_hipriv_ack);
        return ul_result;
    }

    uc_hipriv_ack = OAL_TRUE;
    dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8), &uc_hipriv_ack);

    return OAL_SUCC;

}
#endif

#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_nss(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_tx_comp_stru            *pst_event_set_nss;
    dmac_vap_stru                   *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap || OAL_PTR_NULL == pst_dmac_vap->pst_hal_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_nss::pst_dmac_vap or pst_dmac_vap->pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 设置数据 */
    pst_event_set_nss = (mac_cfg_tx_comp_stru *)puc_param;
    pst_dmac_vap->st_tx_data_mcast.ast_per_rate[0].rate_bit_stru.un_nss_rate.st_vht_nss_mcs.bit_nss_mode = pst_event_set_nss->uc_param;
    pst_dmac_vap->st_tx_data_mcast.ast_per_rate[0].rate_bit_stru.un_nss_rate.st_legacy_rate.bit_protocol_mode = pst_event_set_nss->en_protocol_mode;

    /* 更新协议速率 */
    pst_dmac_vap->uc_protocol_rate_dscr = (oal_uint8)((pst_event_set_nss->en_protocol_mode << 6) | pst_event_set_nss->uc_param);

    if (OAL_SWITCH_ON == pst_dmac_vap->pst_hal_device->uc_al_tx_flag)
    {
        hal_set_tx_dscr_field(pst_dmac_vap->pst_hal_device, pst_dmac_vap->st_tx_data_mcast.ast_per_rate[0].ul_value, HAL_RF_TEST_DATA_RATE_ZERO);
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_mcs::tx dscr nss=%d.", pst_event_set_nss->uc_param);

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_rfch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_tx_comp_stru            *pst_event_set_rfch;
    dmac_vap_stru                   *pst_dmac_vap;

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    oal_uint8                        uc_hipriv_ack = OAL_FALSE;
#endif

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap || OAL_PTR_NULL == pst_dmac_vap->pst_hal_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_rfch::pst_dmac_vap or pst_dmac_vap->pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 设置数据 */
    pst_event_set_rfch = (mac_cfg_tx_comp_stru *)puc_param;
    pst_dmac_vap->st_tx_data_mcast.ast_per_rate[0].rate_bit_stru.bit_tx_chain_selection = pst_event_set_rfch->uc_param;

    /* 常发模式下在线配置 */
    if (OAL_SWITCH_ON == pst_dmac_vap->pst_hal_device->uc_al_tx_flag)
    {
        hal_set_tx_dscr_field(pst_dmac_vap->pst_hal_device, pst_dmac_vap->st_tx_data_mcast.ast_per_rate[0].ul_value, HAL_RF_TEST_DATA_RATE_ZERO);
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_mcs::tx dscr tx chain selection=%d.", pst_event_set_rfch->uc_param);

    /* 命令成功返回Success */
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    uc_hipriv_ack = OAL_TRUE;
    dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8), &uc_hipriv_ack);
#endif

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_always_tx(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_tx_comp_stru            *pst_event_set_al_tx;
    dmac_vap_stru                   *pst_dmac_vap;
    mac_device_stru                 *pst_device;
    oal_uint32                       ul_ret;
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    oal_uint8                        uc_hipriv_ack = OAL_FALSE;
#endif

    pst_event_set_al_tx = (mac_cfg_tx_comp_stru *)puc_param;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap || OAL_PTR_NULL == pst_dmac_vap->pst_hal_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_always_tx::pst_dmac_vap or pst_dmac_vap->pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果新下发的命令开关状态没变，则直接返回 */
    if ( pst_event_set_al_tx->uc_param == pst_dmac_vap->pst_hal_device->uc_al_tx_flag )
    {
        /* 命令成功返回Success */
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
        uc_hipriv_ack = OAL_TRUE;
        dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8), &uc_hipriv_ack);
#endif
        return OAL_SUCC;
    }

    /*关闭中断*/
    oal_irq_disable();

    /* 挂起硬件发送 */
    hal_set_machw_tx_suspend(pst_dmac_vap->pst_hal_device);

    /* 停止mac phy接收 */
    hal_disable_machw_phy_and_pa(pst_dmac_vap->pst_hal_device);

    /*首先清空发送完成事件队列*/
    frw_event_flush_event_queue(FRW_EVENT_TYPE_WLAN_TX_COMP);
    frw_event_flush_event_queue(FRW_EVENT_TYPE_WLAN_CRX);
    frw_event_flush_event_queue(FRW_EVENT_TYPE_WLAN_DRX);

    /* flush硬件5个发送队列 */
    dmac_tx_reset_flush(pst_dmac_vap->pst_hal_device);

    /*清除硬件发送缓冲区*/
    hal_clear_hw_fifo(pst_dmac_vap->pst_hal_device);

    /*复位macphy*/
    hal_reset_phy_machw(pst_dmac_vap->pst_hal_device,
                         HAL_RESET_HW_TYPE_ALL,
                         HAL_RESET_MAC_ALL,
                         OAL_FALSE,
                         OAL_FALSE);

    /* 设置常发模式标志 */
    mac_vap_set_al_tx_flag(pst_mac_vap, pst_event_set_al_tx->uc_param);
    mac_vap_set_al_tx_payload_flag(pst_mac_vap, pst_event_set_al_tx->en_payload_flag);
    pst_dmac_vap->pst_hal_device->uc_al_tx_flag = pst_event_set_al_tx->uc_param;
    pst_dmac_vap->pst_hal_device->bit_al_tx_flag = pst_event_set_al_tx->uc_param;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if (OAL_SWITCH_OFF == pst_dmac_vap->pst_hal_device->uc_al_tx_flag)
    {
        pst_mac_vap->st_cap_flag.bit_keepalive = OAL_TRUE;
        hal_rf_test_disable_al_tx(pst_dmac_vap->pst_hal_device);
        mac_vap_set_al_tx_first_run(pst_mac_vap, OAL_FALSE);

#ifdef _PRE_WLAN_FEATURE_ANTI_INTERF
        /* 常发关闭，打开弱干扰免疫算法 */
        ul_ret = dmac_alg_anti_intf_switch(pst_device, 2);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_always_tx: dmac_alg_anti_intf_switch fail!}");
        }
#endif
    }
    else
    {
        hal_rf_test_enable_al_tx(pst_dmac_vap->pst_hal_device, OAL_PTR_NULL);

#ifdef _PRE_WLAN_FEATURE_ANTI_INTERF
        /* 常发打开，关闭弱干扰免疫算法,并将agc unlock门限配置为0x0 */
        ul_ret = dmac_alg_anti_intf_switch(pst_device, OAL_FALSE);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_always_tx: dmac_alg_anti_intf_switch fail!}");
        }
        hal_set_agc_unlock_min_th(pst_device->pst_device_stru, 0, 0);
#endif

        ul_ret = dmac_alg_cfg_channel_notify(pst_mac_vap, CH_BW_CHG_TYPE_MOVE_WORK);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_always_tx:dmac_config_al_tx_set_pow fail!}");
            return ul_ret;
        }
        /* 设置AGC门限，关掉接收 */
        hal_al_tx_set_agc_phy_reg(pst_dmac_vap->pst_hal_device, 0x00b4e0e0);
    }

    /*避免复位过程中接收描述符队列异常，重新初始化接收描述符队列*/
    dmac_reset_rx_dscr_queue_flush(pst_dmac_vap->pst_hal_device);

    /*使能中断*/
    oal_irq_enable();

    /* 恢复 mac phy接收*/
    hal_enable_machw_phy_and_pa(pst_dmac_vap->pst_hal_device);

    /* 使能硬件发送 */
    hal_set_machw_tx_resume(pst_dmac_vap->pst_hal_device);

    /* 命令成功返回Success */
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    uc_hipriv_ack = OAL_TRUE;
    dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8), &uc_hipriv_ack);
#endif

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_rxch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint8                        uc_rxch = 0;
    mac_device_stru                 *pst_mac_device;
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    oal_uint8                        uc_hipriv_ack = OAL_FALSE;
#endif

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_rxch = *puc_param;

    mac_device_set_rxchain(pst_mac_device, uc_rxch);

    hal_set_rx_multi_ant(pst_mac_device->pst_device_stru, pst_mac_device->uc_rx_chain);

    /* 命令成功返回Success */
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    uc_hipriv_ack = OAL_TRUE;
    dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8), &uc_hipriv_ack);
#endif

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  dmac_config_dync_txpower(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hal_to_dmac_device_stru         *pst_hal_device_base;
    mac_device_stru                 *pst_device;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device_base = pst_device->pst_device_stru;

    if (*puc_param == pst_hal_device_base->ul_dync_txpower_flag)
    {
        return OAL_SUCC;
    }

    pst_hal_device_base->ul_dync_txpower_flag = *puc_param;

    #ifdef _PRE_WLAN_REALTIME_CALI
#ifdef _PRE_WLAN_FEATURE_DOUBLE_CHIP
    if (WLAN_DOUBLE_CHIP_5G_ID == pst_hal_device_base->uc_chip_id)
#else
    if (WLAN_SINGLE_CHIP_5G_ID == pst_hal_device_base->uc_chip_id)
#endif
    {
        OAL_IO_PRINT("dmac_config_dync_txpower:5G dync txpower can not use!\n");
        return OAL_SUCC;
    }

    /* 创建定时器 */
    if (1 == pst_hal_device_base->ul_dync_txpower_flag)
    {
        FRW_TIMER_CREATE_TIMER(&(pst_device->st_realtime_cali_timer),
                        dmac_rf_realtime_cali_timeout,
                        WLAN_REALTIME_CALI_INTERVAL,
                        pst_device,
                        OAL_TRUE,
                        OAM_MODULE_ID_DMAC,
                        pst_device->ul_core_id);
    }
    /* 删除定时器 */
    else if (0 == pst_hal_device_base->ul_dync_txpower_flag)
    {
        FRW_TIMER_STOP_TIMER(&pst_device->st_realtime_cali_timer);
    }
    #endif

    OAM_INFO_LOG1(0, OAM_SF_CFG, "dmac_config_dync_txpower:uc_dync_power_flag = %d\n", pst_hal_device_base->ul_dync_txpower_flag);

    return OAL_SUCC;
}
#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_get_thruput(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_vap_stru                   *pst_dmac_vap;
    oal_uint32                       ul_rx_octects_in_ampdu;
    oal_uint32                       ul_tx_octects_in_ampdu;
    oal_uint32                       ul_rx_normal_mdpu_succ_num;
    oal_uint32                       ul_rx_ampdu_succ_num;
    oal_uint32                       ul_tx_ppdu_succ_num;
    oal_uint32                       ul_rx_ppdu_fail_num;
    oal_uint32                       ul_tx_ppdu_fail_num;
    oal_uint32                       ul_rx_ampdu_fcs_num;
    oal_uint32                       ul_rx_delimiter_fail_num;
    oal_uint32                       ul_rx_mpdu_fcs_num;
    oal_uint32                       ul_rx_phy_err_mac_passed_num;
    oal_uint32                       ul_rx_phy_longer_err_num;
    oal_uint32                       ul_rx_phy_shorter_err_num;
    oal_uint32                       ul_timestamp;
    oal_uint8                        uc_stage;       /*0为开始统计阶段， 1为结束统计阶段*/
    oal_uint32                       ul_rx_rate;     /*单位mpbs*/
    oal_uint32                       ul_tx_rate;     /*单位mpbs*/
    oal_uint32                       ul_time_offest; /*统计时间差, 单位ms*/
#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    oal_cpu_usage_stat_stru          st_cpu_stat;
    oal_uint64                       ull_alltime;
#endif
    mac_device_stru                 *pst_mac_device;
    oal_uint32                       ul_cycles = 0;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    frw_event_mem_stru             *pst_event_mem;
    frw_event_stru                 *pst_event_up;
    dmac_thruput_info_sync_stru     *pst_thruput_info_sync;
#endif

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_get_thruput::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap || OAL_PTR_NULL == pst_dmac_vap->pst_hal_device))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_get_thruput::pst_dmac_vap or pst_dmac_vap->pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }


    uc_stage = *(oal_bool_enum_uint8 *)puc_param;

    if(0 == uc_stage)
    {
        pst_mac_device->ul_first_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();

    #if defined(_PRE_PRODUCT_ID_HI110X_DEV)
    #if (!defined(_PRE_PC_LINT) && !defined(WIN32))
        enable_cycle_counter();
    #endif
    #endif

        /*清零*/
        hal_set_counter_clear(pst_dmac_vap->pst_hal_device);
        pst_dmac_vap->pst_hal_device->ul_rx_normal_mdpu_succ_num = 0;
        pst_dmac_vap->pst_hal_device->ul_rx_ampdu_succ_num = 0;
        pst_dmac_vap->pst_hal_device->ul_tx_ppdu_succ_num = 0;
        pst_dmac_vap->pst_hal_device->ul_rx_ppdu_fail_num = 0;
        pst_dmac_vap->pst_hal_device->ul_tx_ppdu_fail_num = 0;
    }
    else
    {
        ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        ul_time_offest = ul_timestamp - pst_mac_device->ul_first_timestamp;

        #if defined(_PRE_PRODUCT_ID_HI110X_DEV)
        #if (!defined(_PRE_PC_LINT) && !defined(WIN32))
            disable_cycle_counter();

            ul_cycles = get_cycle_count();
        #endif
        #endif

        ul_rx_normal_mdpu_succ_num = pst_dmac_vap->pst_hal_device->ul_rx_normal_mdpu_succ_num;
        ul_rx_ampdu_succ_num = pst_dmac_vap->pst_hal_device->ul_rx_ampdu_succ_num;
        ul_tx_ppdu_succ_num = pst_dmac_vap->pst_hal_device->ul_tx_ppdu_succ_num;
        ul_rx_ppdu_fail_num = pst_dmac_vap->pst_hal_device->ul_rx_ppdu_fail_num;
        ul_tx_ppdu_fail_num = pst_dmac_vap->pst_hal_device->ul_tx_ppdu_fail_num;

        if (HAL_ALWAYS_TX_MPDU == pst_dmac_vap->pst_hal_device->bit_al_tx_flag)
        {
            ul_tx_rate = ((ul_tx_ppdu_succ_num + ul_tx_ppdu_fail_num)*g_ul_al_mpdu_len/(ul_time_offest))*8;
            ul_rx_rate = ((ul_rx_normal_mdpu_succ_num + ul_rx_ppdu_fail_num)*g_ul_al_mpdu_len/(ul_time_offest))*8;
        }
        else
        {
            hal_get_ampdu_bytes(pst_dmac_vap->pst_hal_device, &ul_tx_octects_in_ampdu, &ul_rx_octects_in_ampdu);

            ul_rx_rate = (ul_rx_octects_in_ampdu/(ul_time_offest))*8;
            ul_tx_rate = (ul_tx_octects_in_ampdu/(ul_time_offest))*8;

            OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_dump_all_rx_dscr::tx octects=%u, rx octects=%u, cycles=%u.}",
                           ul_tx_octects_in_ampdu, ul_rx_octects_in_ampdu, ul_cycles);

        }

        /*错误检查*/
        hal_get_rx_err_count(pst_dmac_vap->pst_hal_device,
                        &ul_rx_ampdu_fcs_num,
                        &ul_rx_delimiter_fail_num,
                        &ul_rx_mpdu_fcs_num,
                        &ul_rx_phy_err_mac_passed_num,
                        &ul_rx_phy_longer_err_num,
                        &ul_rx_phy_shorter_err_num);

        OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{tx succ num: %u, tx fail num: %u}", ul_tx_ppdu_succ_num, ul_tx_ppdu_fail_num);
        OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{rx normal succ num: %u, rx ampdu succ num: %u, rx fail num: %u}", ul_rx_normal_mdpu_succ_num, ul_rx_ampdu_succ_num, ul_rx_ppdu_fail_num);
        OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{tx rate(Mbps): %u, rx rate(Mbps): %u, ul_cycles: %u,}", ul_tx_rate, ul_rx_rate, ul_cycles);
        OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{rx ampdu fcs num: %u, rx delimiter fail num: %u,}", ul_rx_ampdu_fcs_num, ul_rx_delimiter_fail_num);
        OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{rx mpdu fcs num: %u, rx phy err mac passed num: %u,}", ul_rx_mpdu_fcs_num, ul_rx_phy_err_mac_passed_num);
        OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{rx phy longer err num: %u, rx phy shorter err num: %u,}", ul_rx_phy_longer_err_num, ul_rx_phy_shorter_err_num);

#if(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        oal_get_cpu_stat(&st_cpu_stat);
        ull_alltime = st_cpu_stat.ull_user + st_cpu_stat.ull_nice + st_cpu_stat.ull_system + st_cpu_stat.ull_idle + st_cpu_stat.ull_iowait +
                      st_cpu_stat.ull_irq + st_cpu_stat.ull_softirq + st_cpu_stat.ull_steal + st_cpu_stat.ull_guest;
	    OAL_IO_PRINT("user=%llu, nice=%llu, system=%llu, idle=%llu, iowait=%llu, irq=%llu, softirq=%llu, steal=%llu, guest=%llu, alltime=%llu\r\n",
	                 st_cpu_stat.ull_user, st_cpu_stat.ull_nice, st_cpu_stat.ull_system, st_cpu_stat.ull_idle, st_cpu_stat.ull_iowait,
	                 st_cpu_stat.ull_irq,  st_cpu_stat.ull_softirq, st_cpu_stat.ull_steal, st_cpu_stat.ull_guest, ull_alltime);
#endif

        OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_dump_all_rx_dscr::TX succ num=%d,fail num=%d;RX succ num=%d, fail num=%d}",
                       ul_tx_ppdu_succ_num, ul_tx_ppdu_fail_num, ul_rx_normal_mdpu_succ_num, ul_rx_ppdu_fail_num);
        OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_dump_all_rx_dscr::tx rate(Mbps)=%d, rx rate(Mbps)=%d, ul_cycles=%d}",
                       ul_tx_rate, ul_rx_rate, ul_cycles);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* 将需要同步的数据抛事件同步到hmac */
        pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_thruput_info_sync_stru));
        if (OAL_PTR_NULL == pst_event_mem)
        {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_query_event_response::pst_event_memory null.}");

            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_event_up              = (frw_event_stru *)pst_event_mem->puc_data;
        pst_thruput_info_sync = (dmac_thruput_info_sync_stru *)pst_event_up->auc_event_data;
        pst_thruput_info_sync->ul_cycles        = ul_cycles;
        pst_thruput_info_sync->ul_sw_tx_succ_num = ul_tx_ppdu_succ_num;
        pst_thruput_info_sync->ul_sw_tx_fail_num = ul_tx_ppdu_fail_num;
        pst_thruput_info_sync->ul_sw_rx_ampdu_succ_num = ul_rx_ampdu_succ_num;
        pst_thruput_info_sync->ul_sw_rx_ppdu_fail_num = ul_rx_ppdu_fail_num;
        pst_thruput_info_sync->ul_sw_rx_mpdu_succ_num  = ul_rx_normal_mdpu_succ_num;
        pst_thruput_info_sync->ul_hw_rx_ampdu_fcs_fail_num = ul_rx_ampdu_fcs_num;
        pst_thruput_info_sync->ul_hw_rx_mpdu_fcs_fail_num = ul_rx_mpdu_fcs_num;
        dmac_send_sys_event((mac_vap_stru *)&(pst_dmac_vap->st_vap_base_info), WLAN_CFGID_THRUPUT_INFO, OAL_SIZEOF(dmac_thruput_info_sync_stru), (oal_uint8 *)pst_thruput_info_sync);
        FRW_EVENT_FREE(pst_event_mem);
#endif
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_freq_skew(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_PHY_PLL_DIV
    mac_cfg_freq_skew_stru *pst_freq_skew;
    mac_device_stru      *pst_device;
    hal_to_dmac_device_stru         *pst_hal_device_base;

    pst_device= mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 参数判断 */
    pst_freq_skew = (mac_cfg_freq_skew_stru*)puc_param;
    if (pst_freq_skew->us_idx >= WITP_RF_SUPP_NUMS)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_freq_skew::us_idx=%d.", pst_freq_skew->us_idx);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    pst_hal_device_base = pst_device->pst_device_stru;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device_base))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_freq_skew::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hal_rf_set_freq_skew(pst_device->pst_device_stru, pst_freq_skew->us_idx, pst_freq_skew->us_chn, pst_freq_skew->as_corr_data);
#endif
    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_adjust_ppm(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    mac_cfg_adjust_ppm_stru         *pst_adjust_ppm;
    mac_device_stru                 *pst_device;
	dmac_vap_stru                   *pst_dmac_vap;

    pst_device= mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取device下的配置vap */
    pst_dmac_vap = mac_res_get_dmac_vap(pst_device->auc_vap_id[0]);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 参数判断 */
    pst_adjust_ppm = (mac_cfg_adjust_ppm_stru*)puc_param;

    hal_rf_adjust_ppm(pst_device->pst_device_stru, pst_adjust_ppm->c_ppm_val, pst_dmac_vap->st_vap_base_info.st_channel.en_bandwidth, pst_adjust_ppm->uc_clock_freq);
#endif

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_dbb_scaling_amend(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1102_HOST)

    oal_uint8                   uc_offset_addr_a = 0;
    oal_uint8                   uc_offset_addr_b = 0;
    //oal_uint32                  ul_scaling_addr = HI1102_PHY_SCALING_VALUE_11B_REG;
    oal_uint32                  ul_scaling_addr = 0;
    oal_uint32                  ul_dbb_scaling_value = 0;
    oal_uint16                  us_delta_gain = (1 << MAC_DBB_SCALING_FIX_POINT_BITS);
    mac_cfg_dbb_scaling_stru    *pst_dbb_scaling;

    pst_dbb_scaling = (mac_cfg_dbb_scaling_stru *)puc_param;

    if (OAL_PTR_NULL == pst_dbb_scaling)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{dmac_config_dbb_scaling_amend::  param NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }


    uc_offset_addr_a = pst_dbb_scaling->uc_offset_addr_a;
    uc_offset_addr_b = pst_dbb_scaling->uc_offset_addr_b;
    us_delta_gain = pst_dbb_scaling->us_delta_gain;

    /*更新dbb scaling值*/
    //ul_scaling_addr += uc_offset_addr_a*4;
    ul_scaling_addr = hi1102_rf_cali_get_scaling_addr(uc_offset_addr_a);

    hi1102_rf_cali_read_phy_reg_bits(ul_scaling_addr,
                                         (oal_uint8)(uc_offset_addr_b*MAC_DBB_SCALING_CFG_BITS),
                                         MAC_DBB_SCALING_CFG_BITS,
                                         &ul_dbb_scaling_value);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{dmac_config_dbb_scaling_amend::  old ul_dbb_scaling_value = %d. \r\n",
                                                    ul_dbb_scaling_value);
    ul_dbb_scaling_value = ((ul_dbb_scaling_value * us_delta_gain) >> MAC_DBB_SCALING_FIX_POINT_BITS);
    ul_dbb_scaling_value = ul_dbb_scaling_value > 0xff ? 0xff : ul_dbb_scaling_value; //超过0xff，取值0xff
    hi1102_rf_cali_write_phy_reg_bits(ul_scaling_addr,
                                         (oal_uint8)(uc_offset_addr_b*MAC_DBB_SCALING_CFG_BITS),
                                         MAC_DBB_SCALING_CFG_BITS,
                                         ul_dbb_scaling_value);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{dmac_config_dbb_scaling_amend::  new ul_dbb_scaling_value = %d. \r\n",
                                                    ul_dbb_scaling_value);
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_DEV) || (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)

    oal_uint8                   uc_offset_addr_a = 0;
    oal_uint8                   uc_offset_addr_b = 0;
    //oal_uint32                  ul_scaling_addr = HI1102_PHY_SCALING_VALUE_11B_REG;
    oal_uint32                  ul_scaling_addr = 0;
    oal_uint32                  ul_dbb_scaling_value = 0;
    oal_uint16                  us_delta_gain = (1 << MAC_DBB_SCALING_FIX_POINT_BITS);
    mac_cfg_dbb_scaling_stru    *pst_dbb_scaling;

    pst_dbb_scaling = (mac_cfg_dbb_scaling_stru *)puc_param;

    if (OAL_PTR_NULL == pst_dbb_scaling)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{dmac_config_dbb_scaling_amend::  param NULL");
        return OAL_ERR_CODE_PTR_NULL;
    }


    uc_offset_addr_a = pst_dbb_scaling->uc_offset_addr_a;
    uc_offset_addr_b = pst_dbb_scaling->uc_offset_addr_b;
    us_delta_gain = pst_dbb_scaling->us_delta_gain;

    /*更新dbb scaling值*/
    //ul_scaling_addr += uc_offset_addr_a*4;
    ul_scaling_addr = hi1103_rf_cali_get_scaling_addr(uc_offset_addr_a);

    hi1103_rf_cali_read_phy_reg_bits(ul_scaling_addr,
                                         (oal_uint8)(uc_offset_addr_b*MAC_DBB_SCALING_CFG_BITS),
                                         MAC_DBB_SCALING_CFG_BITS,
                                         &ul_dbb_scaling_value);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{dmac_config_dbb_scaling_amend::  old ul_dbb_scaling_value = %d. \r\n",
                                                    ul_dbb_scaling_value);
    ul_dbb_scaling_value = ((ul_dbb_scaling_value * us_delta_gain) >> MAC_DBB_SCALING_FIX_POINT_BITS);
    ul_dbb_scaling_value = ul_dbb_scaling_value > 0xff ? 0xff : ul_dbb_scaling_value; //超过0xff，取值0xff
    hi1103_rf_cali_write_phy_reg_bits(ul_scaling_addr,
                                         (oal_uint8)(uc_offset_addr_b*MAC_DBB_SCALING_CFG_BITS),
                                         MAC_DBB_SCALING_CFG_BITS,
                                         ul_dbb_scaling_value);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{dmac_config_dbb_scaling_amend::  new ul_dbb_scaling_value = %d. \r\n",
                                                    ul_dbb_scaling_value);
#endif
    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_HILINK

OAL_STATIC oal_uint32  dmac_config_clear_fbt_scan_list(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32           l_value;
    mac_device_stru    *pst_mac_device;
    oal_uint32          ul_ret = OAL_SUCC;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_clear_fbt_scan_list::pst_mac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取device */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_clear_fbt_scan_list::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_device_clear_fbt_scan_list(pst_mac_device, puc_param);

    return ul_ret;
}



OAL_STATIC oal_uint32  dmac_config_fbt_scan_specified_sta(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32           l_value;
    mac_device_stru    *pst_mac_device;
    mac_fbt_scan_sta_addr_stru *pst_fbt_scan_sta;
    oal_uint32          ul_ret = OAL_SUCC;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_fbt_scan_specified_sta::pst_mac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取device */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_fbt_scan_specified_sta::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_sta = (mac_fbt_scan_sta_addr_stru *)puc_param;

    ul_ret = mac_device_set_fbt_scan_sta(pst_mac_device, pst_fbt_scan_sta);


    return ul_ret;
}



oal_uint32  dmac_config_fbt_scan_interval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_fbt_scan_interval;
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;
    mac_device_stru            *pst_mac_dev;
    oal_uint32                  ul_ret = OAL_SUCC;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_fbt_scan_interval::pst_mac_vap or puc_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_fbt_scan_interval::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_fbt_scan_interval = *((oal_uint32 *)puc_param);
    ul_ret = mac_device_set_fbt_scan_interval(pst_mac_dev, ul_fbt_scan_interval);

    return ul_ret;
}


oal_uint32  dmac_config_fbt_scan_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret = OAL_SUCC;
    oal_uint8                   uc_fbt_scan_channel;
    mac_device_stru            *pst_mac_dev;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_fbt_scan_channel::pst_mac_vap or puc_param null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_fbt_scan_channel::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_fbt_scan_channel = *((oal_uint8 *)puc_param);

    /* 判断信道是否有效 */
    ul_ret = mac_is_channel_num_valid(pst_mac_vap->st_channel.en_band, uc_fbt_scan_channel);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_fbt_scan_channel::mac_is_channel_num_valid[%d] failed[%d].}", uc_fbt_scan_channel, ul_ret);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

#ifdef _PRE_WLAN_FEATURE_11D
    /* 信道14特殊处理，只在11b协议模式下有效 */
    if ((14 == uc_fbt_scan_channel) && (WLAN_LEGACY_11B_MODE != pst_mac_vap->en_protocol))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_fbt_scan_channel::channel-14 only available in 11b, curr protocol=%d.}", pst_mac_vap->en_protocol);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
#endif

    ul_ret = mac_device_set_fbt_scan_channel(pst_mac_dev, uc_fbt_scan_channel);

    return ul_ret;

}



oal_uint32  dmac_config_fbt_scan_report_period(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  l_value;
    mac_device_stru            *pst_mac_dev;
    oal_uint32                  ul_ret = OAL_SUCC;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{dmac_config_fbt_scan_report_period::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_fbt_scan_report_period::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_uint32 *)puc_param);

    ul_ret = mac_device_set_fbt_scan_report_period(pst_mac_dev, l_value);

    return ul_ret;

}


oal_uint32  dmac_config_fbt_scan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    oal_uint8                   uc_cfg_fbt_scan_enable = 0;
    oal_uint8                   uc_user_index = 0;
    mac_device_stru            *pst_mac_dev = OAL_PTR_NULL;
    oal_uint32                  ul_ret = OAL_SUCC;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{dmac_config_fbt_scan_enable::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_fbt_scan_enable::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户下发的要配置到fbt scan管理实体中的参数 */
    uc_cfg_fbt_scan_enable = *puc_param;

    /* 记录配置的模式到fbt scan管理实体中，当前只支持侦听一个用户 */
    ul_ret = mac_device_set_fbt_scan_enable(pst_mac_dev, uc_cfg_fbt_scan_enable);

    return ul_ret;
}
#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_beacon_chain_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint8         uc_value;
    dmac_vap_stru    *pst_dmac_vap;

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_beacon_chain_switch::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_value = *((oal_uint8 *)puc_param);

    /* 配置beacon帧关闭/开启双路轮流发送，1为开启,0为关闭 */
    pst_dmac_vap->en_beacon_chain_active = uc_value;

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint8         uc_value;

    uc_value = *((oal_uint8 *)puc_param);
    mac_vap_set_hide_ssid(pst_mac_vap, uc_value);
    OAM_ERROR_LOG1(0, OAM_SF_CFG, "{dmac_config_hide_ssid::mac_vap_set_hide_ssid [%d].}", uc_value);

    //OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "dmac_config_hide_ssid[%d] succ!", uc_value);
#endif
    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE


#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32  dmac_config_wifi_en(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    oal_int32                    l_value;
    dmac_vap_stru               *pst_dmac_vap;
    oal_uint16                   us_type;
    mac_device_stru             *pst_mac_dev;
    oal_uint8                    uc_state_to;

    l_value = *((oal_int32 *)puc_param);
    if(1 == l_value)
    {
        us_type = AP_PWR_EVENT_WIFI_ENABLE;
    }
    else
    {
        us_type = AP_PWR_EVENT_WIFI_DISABLE;
    }

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_config_wifi_en::pst_mac_dev[%d] is NULL!}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(pst_mac_dev->uc_cfg_vap_id == pst_mac_vap->uc_vap_id)
    {
        /*cfg vap,直接配置device*/
        if (AP_PWR_EVENT_WIFI_ENABLE == us_type)
        {
            uc_state_to = DEV_PWR_STATE_WORK;
        }
        else
        {
            uc_state_to = DEV_PWR_STATE_DEEP_SLEEP;
        }
        mac_pm_set_hal_state(pst_mac_dev, pst_mac_vap, uc_state_to);
    }
    else
    {
        if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        {
        /*业务VAP，给PM推送事件*/
            pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);

            dmac_pm_post_event(pst_dmac_vap,us_type,0,OAL_PTR_NULL);
        }
    }
    return OAL_SUCC;

}



OAL_STATIC oal_uint32  dmac_config_pm_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru             *pst_device;
    dmac_vap_stru               *pst_dmac_vap;
    mac_pm_arbiter_stru         *pst_pm_arbiter;
    mac_pm_handler_stru         *pst_pm_handler;
    mac_fsm_stru                *pst_pm_fsm;
    oal_uint32                   ul_loop;
    oal_uint8                    uc_vap_idx;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pm_info::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*device下arbiter的信息*/
    pst_pm_arbiter = pst_device->pst_pm_arbiter;
    if(OAL_PTR_NULL == pst_pm_arbiter)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "Requestor num = %d,id_bitmap = 0x%x\n",pst_pm_arbiter->uc_requestor_num,pst_pm_arbiter->ul_id_bitmap);
    for(ul_loop=0;ul_loop<pst_pm_arbiter->uc_requestor_num;ul_loop++)
    {
        if(0 == ul_loop%4)
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "\n");
        }
        OAL_IO_PRINT("Requestor id %d:%s ,",ul_loop,pst_pm_arbiter->requestor[ul_loop].auc_id_name);

    }
    /*state bitmap*/
    for(ul_loop=0;ul_loop<DEV_PWR_STATE_BUTT;ul_loop++)
    {
        if(0 == ul_loop%4)
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "\n");
        }
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "state %d bitmap:0x%x ,",
                        ul_loop,pst_pm_arbiter->ul_state_bitmap[ul_loop]);
    }

    OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "\n Curr State:(%d), Prev State:(%d)\n",
                  pst_pm_arbiter->uc_cur_state, pst_pm_arbiter->uc_prev_state);
   /*每VAP状态机FSM*/
   /* 遍历device下所有vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
    {
      pst_dmac_vap = mac_res_get_dmac_vap(pst_device->auc_vap_id[uc_vap_idx]);
      if (OAL_PTR_NULL == pst_dmac_vap)
      {
          OAM_WARNING_LOG1(pst_device->uc_cfg_vap_id, OAM_SF_CFG, "{hmac_config_wifi_enable::pst_mac_vap null, vap id=%d.", pst_device->auc_vap_id[uc_vap_idx]);
          return OAL_ERR_CODE_PTR_NULL;
      }
      pst_pm_handler = pst_dmac_vap->pst_pm_handler;
      if(!pst_pm_handler)
      {
        return OAL_ERR_CODE_PTR_NULL;
      }
      pst_pm_fsm = pst_pm_handler->p_mac_fsm;
      if(!pst_pm_fsm)
      {
        return OAL_ERR_CODE_PTR_NULL;
      }
      OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"Arbiter id = %d,max_inactive_time = %d\n",
                    pst_pm_handler->ul_pwr_arbiter_id,pst_pm_handler->ul_max_inactive_time);
      OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"inactive_time = %d,user_check_count = %d,user_num = %d\n",
                    pst_pm_handler->ul_inactive_time,pst_pm_handler->ul_user_check_count,pst_dmac_vap->st_vap_base_info.us_user_nums);
      OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"auto_sleep_en = %d, wow_en = %d, siso_en = %d\n",
                    pst_pm_handler->bit_pwr_sleep_en,pst_pm_handler->bit_pwr_wow_en,
                    pst_pm_handler->bit_pwr_siso_en);
      OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"Cur State:(%d), Prev state:(%d), Last event:(%d)\n",
                    pst_pm_fsm->uc_cur_state, pst_pm_fsm->uc_prev_state, pst_pm_fsm->us_last_event);

    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_pm_en(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_device;
    dmac_vap_stru               *pst_dmac_vap;
    mac_pm_arbiter_stru         *pst_pm_arbiter;
    oal_uint8                    uc_vap_idx;
    oal_int32                    l_value;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pm_en::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*device下arbiter的信息*/
    pst_pm_arbiter = pst_device->pst_pm_arbiter;
    if(OAL_PTR_NULL == pst_pm_arbiter)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pm_en::pst_pm_arbiter null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    l_value = *((oal_int32 *)puc_param);
    pst_device->en_pm_enable = (1==l_value)?OAL_TRUE:OAL_FALSE;


    /*使能,为每个VAP attach pm handler*/
    /*去使能,为每个VAP deattach pm handler*/
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
    {
      pst_dmac_vap = mac_res_get_dmac_vap(pst_device->auc_vap_id[uc_vap_idx]);
      if (OAL_PTR_NULL == pst_dmac_vap)
      {
          OAM_WARNING_LOG1(pst_device->uc_cfg_vap_id, OAM_SF_CFG, "{dmac_config_pm_en::pst_mac_vap null, vap id=%d.", pst_device->auc_vap_id[uc_vap_idx]);
          return OAL_ERR_CODE_PTR_NULL;
      }

      /*STA模式待开发*/
      if(WLAN_VAP_MODE_BSS_STA ==  pst_dmac_vap->st_vap_base_info.en_vap_mode)
      {
        continue;
      }

      if(OAL_TRUE == pst_device->en_pm_enable)
      {
        dmac_pm_ap_attach(pst_dmac_vap);
      }
      else
      {
        dmac_pm_ap_deattach(pst_dmac_vap);
      }
    }
    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_PERFORM_STAT

OAL_STATIC oal_uint32  dmac_config_pfm_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_stat_param_stru        *pst_stat_param  = OAL_PTR_NULL;
    mac_user_stru                  *pst_user        = OAL_PTR_NULL;
    dmac_tid_stru                  *pst_tid         = OAL_PTR_NULL;
    oal_void                       *p_void          = OAL_PTR_NULL;
    oal_uint32                      ul_ret          = OAL_SUCC;

    pst_stat_param = (mac_cfg_stat_param_stru *)puc_param;

    switch (pst_stat_param->en_stat_type)
    {
        case MAC_STAT_TYPE_TID_DELAY:
        case MAC_STAT_TYPE_TID_PER:
        case MAC_STAT_TYPE_TID_THRPT:
            pst_user = mac_vap_get_user_by_addr(pst_mac_vap, pst_stat_param->auc_mac_addr);

            ul_ret = dmac_user_get_tid_by_num(pst_user, pst_stat_param->uc_tidno, &pst_tid);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_stat::dmac_user_get_tid_by_num failed[%d].}", ul_ret);
                return ul_ret;
            }

            p_void = (oal_void *)pst_tid;

            break;

        case MAC_STAT_TYPE_USER_THRPT:
            pst_user = mac_vap_get_user_by_addr(pst_mac_vap, pst_stat_param->auc_mac_addr);

            p_void = (oal_void *)pst_user;

            break;

        case MAC_STAT_TYPE_VAP_THRPT:
            p_void = (oal_void *)pst_mac_vap;

            break;

        default:
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_stat::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
            return OAL_FAIL;
    }

    /* 注册统计节点 */
    ul_ret = dmac_stat_register(OAM_MODULE_ID_PERFORM_STAT, pst_stat_param->en_stat_type, p_void, OAL_PTR_NULL, OAL_PTR_NULL,pst_mac_vap->ul_core_id);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_stat::invalid en_stat_type[%d].}", pst_stat_param->en_stat_type);
        return ul_ret;
    }

    /* 同时开始启动统计 */
    ul_ret = dmac_stat_start(OAM_MODULE_ID_PERFORM_STAT,
                             pst_stat_param->en_stat_type,
                             pst_stat_param->us_stat_period,
                             pst_stat_param->us_stat_num,
                             p_void);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_stat::dmac_stat_start failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  dmac_config_pfm_display(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_display_param_stru     *pst_display_param  = OAL_PTR_NULL;
    mac_user_stru                  *pst_user            = OAL_PTR_NULL;
    dmac_tid_stru                  *pst_tid             = OAL_PTR_NULL;
    oal_void                       *p_void              = OAL_PTR_NULL;
    oal_uint32                      ul_ret              = OAL_SUCC;

    pst_display_param = (mac_cfg_display_param_stru *)puc_param;

    switch (pst_display_param->en_stat_type)
    {
        case MAC_STAT_TYPE_TID_DELAY:
        case MAC_STAT_TYPE_TID_PER:
        case MAC_STAT_TYPE_TID_THRPT:
            pst_user = mac_vap_get_user_by_addr(pst_mac_vap, pst_display_param->auc_mac_addr);

            ul_ret = dmac_user_get_tid_by_num(pst_user, pst_display_param->uc_tidno, &pst_tid);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_display::dmac_user_get_tid_by_num failed[%d].}", ul_ret);
                return ul_ret;
            }

            p_void = (oal_void *)pst_tid;

            break;

        case MAC_STAT_TYPE_USER_THRPT:
            pst_user = mac_vap_get_user_by_addr(pst_mac_vap, pst_display_param->auc_mac_addr);

            p_void = (oal_void *)pst_user;

            break;

        case MAC_STAT_TYPE_VAP_THRPT:
            p_void = (oal_void *)pst_mac_vap;

            break;

        default:
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_display::invalid en_stat_type[%d].}", pst_display_param->en_stat_type);
            return OAL_FAIL;
    }

    /* 显示统计信息 */
    ul_ret = dmac_stat_display(OAM_MODULE_ID_PERFORM_STAT, pst_display_param->en_stat_type, p_void);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_display::dmac_stat_display failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 注销统计节点 */
    ul_ret = dmac_stat_unregister(OAM_MODULE_ID_PERFORM_STAT, pst_display_param->en_stat_type, p_void);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_pfm_display::dmac_stat_unregister failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  dmac_config_set_lpm_chip_state(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_device;
    hal_lpm_state_param_stru     st_para;
    mac_cfg_lpm_sleep_para_stru  *pst_set_para;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_lpm_chip_state::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_set_para = (mac_cfg_lpm_sleep_para_stru*)puc_param;
    if (MAC_LPM_STATE_SOFT_SLEEP == pst_set_para->uc_pm_switch)
    {
        st_para.bit_soft_sleep_en = 1;
        st_para.ul_sleep_time     = 1000*(pst_set_para->us_sleep_ms);   /*15ms*/

        //dmac_test_lpm_create_sleep_timer(pst_device,st_para.us_sleep_time);

        hal_set_lpm_state(pst_device->pst_device_stru,HAL_LPM_STATE_NORMAL_WORK,HAL_LPM_STATE_DEEP_SLEEP,&st_para, OAL_PTR_NULL);
    }

    if (MAC_LPM_STATE_GPIO_SLEEP == pst_set_para->uc_pm_switch)
    {
        st_para.bit_gpio_sleep_en = 1;
        hal_set_lpm_state(pst_device->pst_device_stru,HAL_LPM_STATE_NORMAL_WORK,HAL_LPM_STATE_DEEP_SLEEP, &st_para, OAL_PTR_NULL);
    }

    if (MAC_LPM_STATE_WORK == pst_set_para->uc_pm_switch)
    {
        /*唤醒*/
        hal_set_lpm_state(pst_device->pst_device_stru,HAL_LPM_STATE_DEEP_SLEEP,HAL_LPM_STATE_NORMAL_WORK,OAL_PTR_NULL, OAL_PTR_NULL);
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  dmac_config_set_lpm_soc_mode(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru                  *pst_device;
    mac_cfg_lpm_soc_set_stru         *pst_soc_para;
    wlan_channel_bandwidth_enum_uint8 en_cur_bandwidth;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        return OAL_FAIL ;
    }
    pst_soc_para = (mac_cfg_lpm_soc_set_stru*)puc_param;

    en_cur_bandwidth = pst_device->en_max_bandwidth;

    switch(pst_soc_para->en_mode)
    {
        case MAC_LPM_SOC_BUS_GATING:
            hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_BUS_GATING,pst_soc_para->uc_on_off,pst_soc_para->uc_pcie_idle);
            break;
        case MAC_LPM_SOC_PCIE_RD_BYPASS:
            hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_PCIE_RD_BYPASS,pst_soc_para->uc_on_off,pst_soc_para->uc_pcie_idle);
            break;
        case MAC_LPM_SOC_MEM_PRECHARGE:
            hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_MEM_PRECHARGE,pst_soc_para->uc_on_off,pst_soc_para->uc_pcie_idle);
            break;
        case MAC_LPM_SOC_PCIE_L0_S:
            hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_PCIE_L0,pst_soc_para->uc_on_off,pst_soc_para->uc_pcie_idle);
            break;
        case MAC_LPM_SOC_PCIE_L1_0:
            hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_PCIE_L1_PM,pst_soc_para->uc_on_off,pst_soc_para->uc_pcie_idle);
            break;
        case MAC_LPM_SOC_AUTOCG_ALL:
            hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_AUTOCG_ALL,pst_soc_para->uc_on_off,pst_soc_para->uc_pcie_idle);
            break;
        case MAC_LPM_SOC_ADC_FREQ:
            if (mac_is_dbac_enabled(pst_device))
            {
                hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_ADC_FREQ,0,pst_soc_para->uc_pcie_idle);
            }
            else
            {
                hal_set_soc_lpm(pst_device->pst_device_stru,HAL_LPM_SOC_ADC_FREQ,pst_soc_para->uc_on_off,en_cur_bandwidth);
            }
            break;
        default:
            break;
    }

    return OAL_SUCC ;

}


OAL_STATIC oal_uint32  dmac_config_set_lpm_psm_param(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru                  *pst_device;
    mac_cfg_lpm_psm_param_stru       *pst_psm_para;
    dmac_vap_stru                    *pst_dmac_vap;
    hal_to_dmac_vap_stru             *pst_hal_vap;

    /* 转化为DMAC VAP */
    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    pst_hal_vap = pst_dmac_vap->pst_hal_vap;

	pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if(OAL_PTR_NULL == pst_hal_vap || OAL_PTR_NULL == pst_device)
    {
        return OAL_FAIL;
    }
    pst_psm_para = (mac_cfg_lpm_psm_param_stru*)puc_param;
    if(0 == pst_psm_para->uc_psm_on||1 == pst_psm_para->uc_psm_on)
    {
        hal_set_psm_listen_interval(pst_hal_vap, pst_psm_para->us_psm_listen_interval);
        hal_set_psm_tbtt_offset(pst_hal_vap, pst_psm_para->us_psm_tbtt_offset);
        hal_set_psm_wakeup_mode(pst_device->pst_device_stru, pst_psm_para->uc_psm_wakeup_mode);
        hal_set_psm_status(pst_device->pst_device_stru, pst_psm_para->uc_psm_on);

        hal_test_lpm_set_psm_en(pst_psm_para->uc_psm_on);
    }
    else
    {
        /*debug显示结果*/
        hal_test_lpm_psm_dump_record();
    }

    return OAL_SUCC;

}



OAL_STATIC oal_uint32  dmac_config_set_lpm_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru    *pst_device;
    oal_uint8           uc_mode;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_lpm_smps_mode::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_mode = *puc_param;
    hal_set_smps_mode(pst_device->pst_device_stru,uc_mode);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_set_lpm_smps_stub(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru						 *pst_device;
    mac_cfg_lpm_smps_stub_stru           *pst_smps_stub;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        return OAL_FAIL;
    }

    pst_smps_stub = (mac_cfg_lpm_smps_stub_stru*)puc_param;
    g_st_dmac_test_mng.st_lpm_smps_stub.uc_stub_type= pst_smps_stub->uc_stub_type;
    g_st_dmac_test_mng.st_lpm_smps_stub.uc_rts_en = pst_smps_stub->uc_rts_en;

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  dmac_config_set_lpm_txop_ps(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru                 *pst_device;
    mac_cfg_lpm_txopps_set_stru* pst_txopps_set;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        return OAL_FAIL;
    }
    pst_txopps_set = (mac_cfg_lpm_txopps_set_stru*)puc_param;
    if((0 == pst_txopps_set->uc_txop_ps_on)||(1 == pst_txopps_set->uc_txop_ps_on))
    {
        hal_set_txop_ps_condition1(pst_device->pst_device_stru, pst_txopps_set->uc_conditon1);
        hal_set_txop_ps_condition2(pst_device->pst_device_stru, pst_txopps_set->uc_conditon2);
        hal_set_txop_ps_enable(pst_device->pst_device_stru,     pst_txopps_set->uc_txop_ps_on);

        dmac_test_lpm_txopps_en(pst_txopps_set->uc_txop_ps_on);
    }
    else
    {
        dmac_test_lpm_txopps_debug();
    }

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  dmac_config_set_lpm_txop_ps_tx_stub(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru						 *pst_device;
    mac_cfg_lpm_txopps_tx_stub_stru      *pst_txopps_tx_stub;
    dmac_lpm_txopps_tx_stub_stru         *pst_txop_stub;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        return OAL_FAIL;
    }
    pst_txopps_tx_stub = (mac_cfg_lpm_txopps_tx_stub_stru*)puc_param;
    pst_txop_stub = &g_st_dmac_test_mng.st_lpm_txop_stub;
    pst_txop_stub->uc_stub_on = pst_txopps_tx_stub->uc_stub_on;
    pst_txop_stub->us_begin_num = pst_txopps_tx_stub->us_begin_num;
    pst_txop_stub->us_curr_num = 0;
    pst_txop_stub->us_partial_aid_real = 0;

    return OAL_SUCC;

}


OAL_STATIC oal_uint32  dmac_config_lpm_tx_probe_request(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_mac_device ;
    dmac_vap_stru                   *pst_dmac_vap ;
    mac_cfg_lpm_tx_data_stru        *pst_lpm_tx_data ;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        return OAL_FAIL ;
    }

    if (0 == pst_mac_device->uc_vap_num)
    {
       OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_lpm_tx_probe_request::can't find existed vap to send probe_req.}");
       return OAL_FAIL ;
    }

   pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_device->auc_vap_id[0]);
   if (OAL_PTR_NULL == pst_dmac_vap)
   {
       OAM_ERROR_LOG0(0, OAM_SF_CFG, "{dmac_config_lpm_tx_probe_request::pst_dmac_vap null.}");

       return OAL_ERR_CODE_PTR_NULL;
   }

   pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru*)puc_param;

   dmac_test_lpm_send_probe_requst(pst_dmac_vap,pst_lpm_tx_data->uc_positive,pst_lpm_tx_data->auc_da);

   return OAL_SUCC ;
}


OAL_STATIC oal_uint32  dmac_config_remove_user_lut(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_remove_lut_stru *pst_param;    /* 复用删除用户结构体 */
    dmac_user_stru          *pst_dmac_user;


    pst_param = (mac_cfg_remove_lut_stru *)puc_param;

    pst_dmac_user = mac_res_get_dmac_user(pst_param->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_remove_user_lut::pst_dmac_user null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (1 == pst_param->uc_is_remove)
    {
        /* remove lut */
        dmac_user_inactive(pst_dmac_user);


    }
    else
    {
        /* resume lut */
        dmac_user_active(pst_dmac_user);
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32 dmac_config_send_frame(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_user_stru                  *pst_dmac_user;
    dmac_vap_stru                   *pst_dmac_vap;
    oal_netbuf_stru                 *pst_netbuf;
    oal_uint32                       ul_frame_len = 0;
    mac_tx_ctl_stru                 *pst_tx_ctl;
    oal_uint32                       ul_ret;
    mac_cfg_send_frame_param_stru   *pst_param = (mac_cfg_send_frame_param_stru  *)puc_param;
    dmac_test_encap_frame            pst_test_encap_frame_fun;

    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_param->auc_mac_ra);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{dmac_config_send_frame::cannot find user by addr!.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_dmac_user->st_user_base_info.uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG1(pst_dmac_user->st_user_base_info.uc_vap_id, OAM_SF_CFG, "{dmac_config_send_frame::cannot find vap by user[user_id is %d]!}",
                                pst_dmac_user->st_user_base_info.uc_vap_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请帧内存 */
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{dmac_config_send_frame::cannot alloc netbuff in size[%d].}", WLAN_LARGE_NETBUF_SIZE);

        return OAL_ERR_CODE_PTR_NULL;
    }
     /* 初始化前后指针为NULL */
    oal_set_netbuf_prev(pst_netbuf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_netbuf, OAL_PTR_NULL);


    /* 填写netbuf的cb字段，供发送管理帧和发送完成接口使用 */
    DMAC_SET_NETBUF_CB(FRW_EVENT_TYPE_HOST_CRX, pst_dmac_user->st_user_base_info.us_assoc_id,
                        OAL_FALSE, OAL_FALSE, 0);
    MAC_GET_CB_FRAME_TYPE(pst_tx_ctl) = WLAN_CONTROL;

    /* 组帧 */
    pst_test_encap_frame_fun = dmac_test_get_encap_func(pst_param->en_frame_type);
    if(OAL_PTR_NULL == pst_test_encap_frame_fun)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{dmac_config_send_frame::pst_test_encap_frame_fun null.}");

        oal_netbuf_free(pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_frame_len = pst_test_encap_frame_fun(&(pst_dmac_vap->st_vap_base_info),
                                            (oal_uint8 *)OAL_NETBUF_HEADER(pst_netbuf),
                                            puc_param, uc_len);

    if (0 == ul_frame_len)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{dmac_config_send_frame::ul_frame_len=0.}");


        oal_netbuf_free(pst_netbuf);

        return OAL_FAIL;
    }

    /* 调用发送管理帧接口 */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_netbuf, (oal_uint16)ul_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_netbuf);
        return ul_ret;
    }
    return OAL_SUCC;
}



OAL_STATIC oal_uint32  dmac_config_set_rx_pn(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    mac_device_stru                 *pst_device;
    mac_cfg_set_rx_pn_stru          *pst_rx_pn;
    dmac_user_stru                  *pst_dmac_usr;
    hal_pn_lut_cfg_stru             st_pn_lut_cfg;

    pst_rx_pn = (mac_cfg_set_rx_pn_stru *)puc_param;

    pst_dmac_usr = mac_res_get_dmac_user(pst_rx_pn->us_user_idx);
    if(OAL_PTR_NULL == pst_dmac_usr)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_rx_pn::pst_dmac_usr null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }


    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_rx_pn::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
    st_pn_lut_cfg.uc_pn_key_type = 1;
    st_pn_lut_cfg.uc_pn_peer_idx = pst_dmac_usr->uc_lut_index;
    st_pn_lut_cfg.uc_pn_tid      = 0;
    st_pn_lut_cfg.ul_pn_msb      = 0;
    st_pn_lut_cfg.ul_pn_lsb      = pst_rx_pn->us_rx_pn;
    hal_set_rx_pn(pst_device->pst_device_stru,&st_pn_lut_cfg);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_set_soft_retry(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_test_set_software_retry(puc_param);
	return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_open_addr4(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_test_open_addr4(puc_param);
	return OAL_SUCC;
}

OAL_STATIC oal_uint32  dmac_config_open_wmm_test(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_test_open_wmm_test(pst_mac_vap, *((oal_uint8 *)puc_param));
	return OAL_SUCC;
}



OAL_STATIC oal_uint32  dmac_config_chip_test_open(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_test_set_chip_test(*(oal_uint8*)puc_param);
	return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_set_coex(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_device;
    mac_cfg_coex_ctrl_param_stru    *pst_coex_ctrl;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        return OAL_FAIL;
    }
    pst_coex_ctrl = (mac_cfg_coex_ctrl_param_stru *)puc_param;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_coex::ul_mac_ctrl=%d ul_rf_ctrl=%d.}",
                  pst_coex_ctrl->ul_mac_ctrl, pst_coex_ctrl->ul_rf_ctrl);

    hal_set_coex_ctrl(pst_device->pst_device_stru, pst_coex_ctrl->ul_mac_ctrl, pst_coex_ctrl->ul_rf_ctrl);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_set_dfx(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = *((oal_int32 *)puc_param);

    g_st_dmac_test_mng.en_cfg_tx_cnt = (oal_switch_enum_uint8)l_value;

    return OAL_SUCC;
}


#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

OAL_STATIC oal_uint32 dmac_config_enable_pmf(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8        en_pmf_active;
    wlan_pmf_cap_status_uint8 *puc_pmf_cap;
    oal_dlist_head_stru       *pst_entry;
    oal_dlist_head_stru       *pst_user_list_head;
    mac_user_stru             *pst_user_tmp;
    dmac_vap_stru             *pst_dmac_vap;
    mac_device_stru           *pst_device;

    puc_pmf_cap = (wlan_pmf_cap_status_uint8 *)puc_param;
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_PMF, "{dmac_config_enable_pmf:: pointer is null,pst_mac_vap[%d], puc_param[%d] .}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PMF, "{dmac_config_enable_pmf::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap = (dmac_vap_stru *)pst_mac_vap;

    switch(*puc_pmf_cap)
    {
        case MAC_PMF_DISABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAActivated(pst_mac_vap, OAL_FALSE);
            en_pmf_active = OAL_FALSE;

             /* 配置pmf的加解密总开关 */
            hal_set_pmf_crypto(pst_dmac_vap->pst_hal_vap, OAL_FALSE);
        }
        break;
        /* enable状态，不改变现有user和硬件的 */
        case MAC_PMF_ENABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAActivated(pst_mac_vap, OAL_TRUE);
            return OAL_SUCC;
        }
        case MAC_PME_REQUIRED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAActivated(pst_mac_vap, OAL_TRUE);
            en_pmf_active = OAL_TRUE;

             /* 配置pmf的加解密总开关 */
            hal_set_pmf_crypto(pst_dmac_vap->pst_hal_vap, OAL_TRUE);

        }
        break;
        default:
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PMF, "{dmac_config_enable_pmf: commend error!}");
            return OAL_FALSE;
        }
    }

    if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state)
    {
        pst_user_list_head = &(pst_mac_vap->st_mac_user_list_head);

        for (pst_entry = pst_user_list_head->pst_next; pst_entry != pst_user_list_head;)
        {
            pst_user_tmp      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);

            /* 指向双向链表下一个节点 */
            pst_entry = pst_entry->pst_next;
            if (OAL_PTR_NULL == pst_user_tmp)
            {
                OAM_ERROR_LOG0(0, OAM_SF_PMF, "dmac_config_enable_pmf:: pst_user_tmp is null");
                return OAL_ERR_CODE_PTR_NULL;
            }
            mac_user_set_pmf_active(pst_user_tmp, en_pmf_active);

        }

    }

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  dmac_config_send_pspoll(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_netbuf_stru         *pst_netbuf;
    oal_uint8               *puc_data;
    mac_tx_ctl_stru         *pst_tx_cb;
    oal_uint32               ul_ret;

    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_SHORT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_netbuf))
    {
        return OAL_PTR_NULL;
    }

    oal_netbuf_put(pst_netbuf, 16);

    puc_data = OAL_NETBUF_HEADER(pst_netbuf);

    /* frame control,发往DS的ps-poll */
    puc_data[0] = 0xA4;
    puc_data[1] = 0x01;

    /* AID */
    puc_data[2] = (oal_uint8)pst_mac_vap->us_sta_aid;
    puc_data[3] = 0xC0;

    /* BSSID */
    oal_set_mac_addr(&puc_data[4], pst_mac_vap->auc_bssid);

    /* Transmitter address */
    oal_set_mac_addr(&puc_data[10], pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    oal_set_netbuf_next(pst_netbuf, OAL_PTR_NULL);
    oal_set_netbuf_prev(pst_netbuf, OAL_PTR_NULL);

    pst_tx_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    OAL_MEMZERO(pst_tx_cb, OAL_NETBUF_CB_SIZE());
    MAC_GET_CB_TX_USER_IDX(pst_tx_cb)   = pst_mac_vap->uc_assoc_vap_id;
    MAC_GET_CB_IS_MCAST(pst_tx_cb)   = OAL_FALSE;
    mac_set_cb_ac(pst_tx_cb, WLAN_WME_AC_MGMT);
    MAC_GET_CB_IS_FIRST_MSDU(pst_tx_cb)          = OAL_TRUE;
    MAC_GET_CB_TX_VAP_INDEX(pst_tx_cb)           = pst_mac_vap->uc_vap_id;
    MAC_GET_CB_FRAME_TYPE(pst_tx_cb)          = WLAN_CONTROL;
    MAC_GET_CB_IS_MCAST(pst_tx_cb)            = OAL_FALSE;
    mac_set_cb_is_amsdu(pst_tx_cb, OAL_FALSE);
    MAC_GET_CB_IS_FROM_PS_QUEUE(pst_tx_cb)   = OAL_TRUE;
    mac_set_cb_frame_hdr(pst_tx_cb, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_netbuf));
    MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_cb)    = 16;
    MAC_GET_CB_MPDU_NUM(pst_tx_cb)               = 1;
    MAC_GET_CB_NETBUF_NUM(pst_tx_cb)             = 1;
    MAC_GET_CB_MPDU_LEN(pst_tx_cb)               = 0;

    ul_ret = dmac_tx_mgmt((dmac_vap_stru *)pst_mac_vap, pst_netbuf, 16);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_pspoll::dmac_tx_mgmt failed[%d].", ul_ret);
        oal_netbuf_free(pst_netbuf);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_send_nulldata(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_netbuf_stru                 *pst_net_buf;
    mac_tx_ctl_stru                 *pst_tx_ctrl;
    oal_uint32                       ul_ret;
    oal_uint16                       us_frame_ctl_second_byte = 0;
    mac_user_stru                   *pst_user;
    oal_uint8                       *puc_frame;
    mac_ieee80211_qos_frame_stru    *pst_qos_header;
    mac_cfg_tx_nulldata_stru        *pst_tx_nulldata;

    /* 申请net_buff */
    pst_net_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_SHORT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_net_buf)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_nulldata::pst_net_buf null.}");

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_tx_nulldata = (mac_cfg_tx_nulldata_stru *)puc_param;

    oal_set_netbuf_prev(pst_net_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_net_buf, OAL_PTR_NULL);

    if (pst_tx_nulldata->l_is_qos)
    {
        oal_netbuf_put(pst_net_buf, OAL_SIZEOF(mac_ieee80211_qos_frame_stru));
    }
    else
    {
        oal_netbuf_put(pst_net_buf, 24);
    }

    puc_frame = OAL_NETBUF_HEADER(pst_net_buf);

    if (0 == pst_tx_nulldata->l_is_psm)
    {
        us_frame_ctl_second_byte = 0x0100;
    }
    else
    {
        us_frame_ctl_second_byte = 0x1100;
    }

    pst_user = mac_res_get_mac_user((oal_uint16)pst_mac_vap->uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_user)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_nulldata::pst_user null.}");
        dmac_tx_excp_free_netbuf(pst_net_buf);

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写帧头,其中from ds为1，to ds为0，因此frame control的第二个字节为02 */
    if (pst_tx_nulldata->l_is_qos)
    {
        mac_hdr_set_frame_control(puc_frame, (oal_uint16)(WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_QOS_NULL) | us_frame_ctl_second_byte);
    }
    else
    {
        mac_hdr_set_frame_control(puc_frame, (oal_uint16)(WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_NODATA) | us_frame_ctl_second_byte);
    }

    if (pst_tx_nulldata->l_is_qos)
    {
        pst_qos_header = (mac_ieee80211_qos_frame_stru *)puc_frame;
        pst_qos_header->bit_qc_tid = (oal_uint8)pst_tx_nulldata->l_tidno;
        pst_qos_header->bit_qc_eosp = 1;
        pst_qos_header->bit_qc_ack_polocy = WLAN_TX_NORMAL_ACK;
    }

    /* BSSID */
    oal_set_mac_addr(&puc_frame[4], pst_user->auc_user_mac_addr);

    /* SA */
    oal_set_mac_addr(&puc_frame[10], pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    /* DA */
    oal_set_mac_addr(&puc_frame[16], pst_user->auc_user_mac_addr);

    /* 填写cb字段 */
    pst_tx_ctrl = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_net_buf);
    OAL_MEMZERO(pst_tx_ctrl, OAL_SIZEOF(mac_tx_ctl_stru));

    /* 填写tx部分 */
    mac_set_cb_ack_policy(pst_tx_ctrl, WLAN_TX_NORMAL_ACK);
    MAC_GET_CB_EVENT_TYPE(pst_tx_ctrl)            = FRW_EVENT_TYPE_WLAN_DTX;
    mac_set_cb_is_bar(pst_tx_ctrl, OAL_FALSE);

    if (pst_tx_nulldata->l_is_qos)
    {
        mac_set_cb_ac(pst_tx_ctrl, WLAN_WME_TID_TO_AC(pst_tx_nulldata->l_tidno));
    }
    else
    {
        mac_set_cb_ac(pst_tx_ctrl, WLAN_WME_AC_BE);
    }
    MAC_GET_CB_IS_FIRST_MSDU(pst_tx_ctrl)        = OAL_TRUE;
    MAC_GET_CB_RETRIED_NUM(pst_tx_ctrl)          = 0;
    if (pst_tx_nulldata->l_is_qos)
    {
        mac_set_cb_tid(pst_tx_ctrl, (oal_uint8)pst_tx_nulldata->l_tidno);
    }
    else
    {
        mac_set_cb_tid(pst_tx_ctrl, WLAN_TID_FOR_DATA);
    }

    MAC_GET_CB_TX_VAP_INDEX(pst_tx_ctrl)          = pst_mac_vap->uc_vap_id;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctrl)           = pst_mac_vap->uc_assoc_vap_id;

    /* 填写tx rx公共部分 */
    //MAC_GET_CB_FRAME_TYPE(pst_tx_ctrl)               = WLAN_DATA_NULL;
    MAC_GET_CB_IS_MCAST(pst_tx_ctrl)                 = OAL_FALSE;
    mac_set_cb_is_amsdu(pst_tx_ctrl, OAL_FALSE);
    MAC_GET_CB_IS_FROM_PS_QUEUE(pst_tx_ctrl)         = OAL_TRUE;
    MAC_GET_CB_IS_PROBE_DATA(pst_tx_ctrl)            = OAL_FALSE;
    mac_set_cb_is_use_4_addr(pst_tx_ctrl, OAL_FALSE);
    mac_set_cb_frame_hdr(pst_tx_ctrl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buf));
    if (pst_tx_nulldata->l_is_qos)
    {
        MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctrl)     = OAL_SIZEOF(mac_ieee80211_qos_frame_stru);
    }
    else
    {
        MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctrl)     = OAL_SIZEOF(mac_ieee80211_frame_stru);
    }

    MAC_GET_CB_MPDU_NUM(pst_tx_ctrl)                = 1;
    MAC_GET_CB_NETBUF_NUM(pst_tx_ctrl)              = 1;
    MAC_GET_CB_MPDU_LEN(pst_tx_ctrl)                = 0;

    ul_ret = dmac_tx_process_data(((dmac_vap_stru *)pst_mac_vap)->pst_hal_device, (dmac_vap_stru *)pst_mac_vap, pst_net_buf);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_nulldata::dmac_tx_process_data failed[%d].}", ul_ret);

        dmac_tx_excp_free_netbuf(pst_net_buf);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_clear_all_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru    *pst_device;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_clear_all_stat::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 清除中断统计信息 */
    hal_clear_irq_stat(pst_device->pst_device_stru);

    return OAL_SUCC;
}
#endif /* CHIP_TEST */


OAL_STATIC oal_uint32  dmac_config_get_fem_pa_status(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                       *pst_device;
    dmac_atcmdsrv_atcmd_response_event     st_atcmdsrv_fem_pa_info;
    oal_uint32                             ul_cali_check_fem_pa_status = 0;

    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_get_fem_pa_status::pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    st_atcmdsrv_fem_pa_info.uc_event_id = OAL_ATCMDSRV_FEM_PA_INFO_EVENT;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    hal_get_hw_status(pst_device->pst_device_stru,&ul_cali_check_fem_pa_status);
#endif
    st_atcmdsrv_fem_pa_info.ul_event_para = ul_cali_check_fem_pa_status;
    dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHECK_FEM_PA, OAL_SIZEOF(dmac_atcmdsrv_atcmd_response_event), (oal_uint8 *)&st_atcmdsrv_fem_pa_info);
    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  dmac_config_set_mib(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return mac_config_set_mib(pst_mac_vap, uc_len, puc_param);
}
#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_thruput_bypass(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_set_thruput_bypass_stru *pst_set_thruput_bypass = (mac_cfg_set_thruput_bypass_stru *)puc_param;

    OAL_SET_THRUPUT_BYPASS_ENABLE(pst_set_thruput_bypass->uc_bypass_type, pst_set_thruput_bypass->uc_value);

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

#ifdef _PRE_WLAN_DFT_STAT
OAL_STATIC oal_uint32  dmac_config_set_performance_log_switch(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_set_performance_log_switch_stru *pst_set_performance_log_switch = (mac_cfg_set_performance_log_switch_stru *)puc_param;
    oal_uint8                    uc_loop_index;
    dmac_vap_stru *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if(pst_set_performance_log_switch->uc_performance_log_switch_type >= DFX_PERFORMANCE_LOG_BUTT)
    {
        for(uc_loop_index = 0;uc_loop_index < DFX_PERFORMANCE_LOG_BUTT;uc_loop_index++)
        {
            DFX_SET_PERFORMANCE_LOG_SWITCH_ENABLE(uc_loop_index,pst_set_performance_log_switch->uc_value);
        }
    }
    else if(pst_set_performance_log_switch->uc_performance_log_switch_type == DFX_PERFORMANCE_DUMP)
    {
        if(0 == pst_set_performance_log_switch->uc_value )
        {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{ampdu length (1-14)%d  (15-17)%d  (18-30)%d  (31-32)%d\n.}",
                pst_dmac_vap->st_query_stats.aul_tx_count_per_apmpdu_length[DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_0],
                pst_dmac_vap->st_query_stats.aul_tx_count_per_apmpdu_length[DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_1],
                pst_dmac_vap->st_query_stats.aul_tx_count_per_apmpdu_length[DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_2],
                pst_dmac_vap->st_query_stats.aul_tx_count_per_apmpdu_length[DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_3]
            );
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{ul_tx_hardretry_count = %d,ul_tx_cts_fail = %d\n.}",
                pst_dmac_vap->st_query_stats.ul_tx_retries,
                pst_dmac_vap->st_query_stats.ul_tx_cts_fail);
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{ul_tx_mpdu_succ_num = %d, ul_tx_mpdu_fail_num = %d\n.}",
                pst_dmac_vap->st_query_stats.ul_tx_mpdu_succ_num,
                pst_dmac_vap->st_query_stats.ul_tx_mpdu_fail_num);
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_RX, "{ul_tx_ampdu_succ_num = %d, ul_tx_mpdu_in_ampdu = %d,ul_tx_ampdu_fail_num = %d, ul_tx_mpdu_fail_in_ampdu = %d\n.}",
                pst_dmac_vap->st_query_stats.ul_tx_ampdu_succ_num,
                pst_dmac_vap->st_query_stats.ul_tx_mpdu_in_ampdu,
                pst_dmac_vap->st_query_stats.ul_tx_ampdu_fail_num,
                pst_dmac_vap->st_query_stats.ul_tx_mpdu_fail_in_ampdu);
        }
        else
        {
            for(uc_loop_index = 0;uc_loop_index < DMAC_COUNT_BY_AMPDU_LENGTH_INDEX_BUTT;uc_loop_index++)
            {
                pst_dmac_vap->st_query_stats.aul_tx_count_per_apmpdu_length[uc_loop_index] = 0;
            }
        }
    }
    else
    {
        DFX_SET_PERFORMANCE_LOG_SWITCH_ENABLE(pst_set_performance_log_switch->uc_performance_log_switch_type,pst_set_performance_log_switch->uc_value);
    }

    return OAL_SUCC;
}
#endif
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_vap_nss(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
	oal_uint8            uc_value;

	uc_value = *puc_param;

	mac_vap_init_rx_nss_by_protocol(pst_mac_vap);
	mac_vap_set_rx_nss(pst_mac_vap, OAL_MIN(pst_mac_vap->en_vap_rx_nss, (uc_value - 1)));

	return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32   dmac_config_resume_rx_intr_fifo(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_resume_rx_intr_fifo_stru   *pst_param;
    mac_device_stru              *pst_mac_dev;

    pst_param = (mac_cfg_resume_rx_intr_fifo_stru *)puc_param;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_resume_rx_intr_fifo::pst_mac_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev->pst_device_stru->en_rx_intr_fifo_resume_flag = pst_param->uc_is_on;

    return OAL_SUCC;
}
#endif

#ifdef  _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_get_all_reg_value(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_device;


    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_get_all_reg_value:pst_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 读取寄存器的值 */
    hal_get_all_reg_value(pst_device->pst_device_stru);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_config_report_ampdu_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_ampdu_stat_stru     *pst_ampdu_stat;
    dmac_user_stru              *pst_dmac_user;
    dmac_tid_stru               *pst_tid;

    pst_ampdu_stat = (mac_cfg_ampdu_stat_stru *)puc_param;

    pst_dmac_user = mac_vap_get_dmac_user_by_addr(pst_mac_vap, pst_ampdu_stat->auc_user_macaddr);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{dmac_config_report_ampdu_stat:: user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取tid，如果tid_no为8，代表对所有tid进行操作 */
    if (pst_ampdu_stat->uc_tid_no > WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{dmac_config_report_ampdu_stat::tid_no invalid, tid_no = [%d]}", pst_ampdu_stat->uc_tid_no);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    if (WLAN_TID_MAX_NUM == pst_ampdu_stat->uc_tid_no)
    {
        return dmac_dft_report_all_ampdu_stat(pst_dmac_user, pst_ampdu_stat->uc_param);
    }
    else
    {
        pst_tid = &pst_dmac_user->ast_tx_tid_queue[pst_ampdu_stat->uc_tid_no];

        return dmac_dft_report_ampdu_stat(pst_tid, pst_ampdu_stat->auc_user_macaddr, pst_ampdu_stat->uc_param);
    }
}
#endif


OAL_STATIC oal_uint32  dmac_config_set_ampdu_aggr_num(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_aggr_num_stru   *pst_aggr_num_ctrl;

    pst_aggr_num_ctrl = (mac_cfg_aggr_num_stru *)puc_param;

    g_uc_aggr_num_switch = pst_aggr_num_ctrl->uc_aggr_num_switch;

    if (0 != pst_aggr_num_ctrl->uc_aggr_num_switch)
    {
        g_uc_max_aggr_num = pst_aggr_num_ctrl->uc_aggr_num;

        //OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_ampdu_aggr_num::aggr num start, num is [%d].}", g_uc_max_aggr_num);
    }
    else
    {
        g_uc_max_aggr_num = 0;

        //OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_set_ampdu_aggr_num::aggr num stop.}");
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  dmac_config_freq_adjust(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_freq_adjust_stru    *pst_freq_adjust;
    mac_device_stru             *pst_mac_device;

    /* 获取device */
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_freq_adjust::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_freq_adjust = (mac_cfg_freq_adjust_stru *)puc_param;

    hal_freq_adjust(pst_mac_device->pst_device_stru, pst_freq_adjust->us_pll_int, pst_freq_adjust->us_pll_frac);

    return OAL_SUCC;
}
#endif
#ifdef _PRE_DEBUG_MODE

oal_uint32 dmac_config_show_device_memleak(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_device_pool_id_stru          *pst_pool_id_param;
    oal_uint8                         uc_pool_id;

    pst_pool_id_param = (mac_device_pool_id_stru *)puc_param;
    uc_pool_id  = pst_pool_id_param->uc_pool_id;

    if (uc_pool_id >=  OAL_MEM_POOL_ID_BUTT)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_show_device_memleak::uc_pool_id %d >= OAL_MEM_POOL_ID_BUTT.}", uc_pool_id);
        return OAL_FAIL;
    }

    if (uc_pool_id < OAL_MEM_POOL_ID_NETBUF)
    {
        oal_mem_leak(uc_pool_id);
    }
    else if (OAL_MEM_POOL_ID_NETBUF == uc_pool_id)
    {
        oal_mem_netbuf_leak();
    }
#endif
    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE


oal_uint32 dmac_config_show_device_meminfo(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_device_pool_id_stru          *pst_pool_id_param;
    oal_uint8                         uc_pool_id;

    pst_pool_id_param = (mac_device_pool_id_stru *)puc_param;
    uc_pool_id  = pst_pool_id_param->uc_pool_id;

    if (uc_pool_id >=  OAL_MEM_POOL_ID_BUTT)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_show_device_meminfo::uc_pool_id %d >= OAL_MEM_POOL_ID_BUTT.}", uc_pool_id);
        return OAL_FAIL;
    }

    OAL_MEM_INFO_PRINT(uc_pool_id);
#endif
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  dmac_config_set_p2p_ps_stat(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_p2p_stat_param_stru *pst_p2p_stat;
    hal_to_dmac_device_stru     *pst_hal_device;
    pst_p2p_stat = (mac_cfg_p2p_stat_param_stru *)puc_param;
    //OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "dmac_config_set_p2p_ps_stat::ctrl:%d\r\n",
    //                pst_p2p_stat->uc_p2p_statistics_ctrl);
    /* 获取hal_device 结构体 */
    hal_get_hal_to_dmac_device(pst_mac_vap->uc_chip_id, pst_mac_vap->uc_device_id, &pst_hal_device);
    if (pst_p2p_stat->uc_p2p_statistics_ctrl == 0)
    {
        /* 清除统计值 */
        hal_clear_irq_stat(pst_hal_device);
    }
    else if (pst_p2p_stat->uc_p2p_statistics_ctrl == 1)
    {
        /* 打印统计值 */
        hal_show_irq_info(pst_hal_device, 0);
    }
    else
    {
        /* 错误控制命令 */
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "dmac_config_set_p2p_ps_stat:: wrong p2p ps ctrl vale \r\n");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS

OAL_STATIC oal_uint32 dmac_config_set_mips(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_mips_type_param_stru      *pst_mips_type_param;

    pst_mips_type_param = (oal_mips_type_param_stru *)puc_param;

    switch (pst_mips_type_param->l_mips_type)
    {
        case OAL_MIPS_TX:
        {
            if (OAL_SWITCH_ON == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_OFF == g_mips_tx_statistic.en_switch)
                {
                    oal_profiling_mips_tx_init();
                    oal_profiling_enable_cycles();

                    g_mips_tx_statistic.en_switch = OAL_SWITCH_ON;
                    g_mips_tx_statistic.uc_flag |= BIT0;
                }
            }
            else if (OAL_SWITCH_OFF == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_ON == g_mips_tx_statistic.en_switch)
                {
                    oal_profiling_disable_cycles();

                    g_mips_tx_statistic.en_switch = OAL_SWITCH_OFF;
                }
            }
        }
        break;

        case OAL_MIPS_RX:
        {
            if (OAL_SWITCH_ON == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_OFF == g_mips_rx_statistic.en_switch)
                {
                    oal_profiling_mips_rx_init();
                    oal_profiling_enable_cycles();

                    g_mips_rx_statistic.en_switch = OAL_SWITCH_ON;
                }
            }
            else if (OAL_SWITCH_OFF == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_ON == g_mips_rx_statistic.en_switch)
                {
                    oal_profiling_disable_cycles();

                    g_mips_rx_statistic.en_switch = OAL_SWITCH_OFF;
                }
            }
        }
        break;

        default:
        {
            OAL_IO_PRINT("dmac_config_set_mips: mips type is wrong!\r\n");
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_config_show_mips(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32 l_mips_type;

    l_mips_type = *((oal_int32 *)puc_param);

    switch (l_mips_type)
    {
        case OAL_MIPS_TX:
        {
            oal_profiling_tx_mips_show();
        }
        break;

        case OAL_MIPS_RX:
        {
            oal_profiling_rx_mips_show();
        }
        break;

        default:
        {
            OAL_IO_PRINT("dmac_config_show_mips: mips type is wrong!\r\n");
        }
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#ifdef _PRE_DEBUG_MODE

oal_uint32 dmac_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_vap_stru *pst_dmac_vap  = (dmac_vap_stru *)pst_mac_vap;

    pst_dmac_vap->en_arp_offload_switch = *(oal_switch_enum_uint8 *)puc_param;
    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

oal_uint32 dmac_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_arpoffload_info_stru    *pst_ps_info;
    mac_device_stru                 *pst_mac_dev;
    dmac_vap_stru                   *pst_dmac_vap;
    oal_uint8                        uc_show_ip_addr;
    oal_uint8                        uc_show_arpoffload_info;
    oal_uint32                       ul_loop;

    pst_mac_dev  = mac_res_get_dev(pst_mac_vap->uc_device_id);
    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);

    pst_ps_info                 = (mac_cfg_arpoffload_info_stru *)puc_param;
    uc_show_ip_addr             = pst_ps_info->uc_show_ip_addr;
    uc_show_arpoffload_info     = pst_ps_info->uc_show_arpoffload_info;
    if ((OAL_PTR_NULL == pst_mac_dev) || (OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_ps_info))
    {
        OAM_ERROR_LOG3(0, OAM_SF_CFG, "{dmac_config_show_arpoffload_info:: pointer is null,pst_mac_de[0x%x],vpst_mac_vap[0x%x],puc_param[0x%x]",pst_mac_dev,pst_dmac_vap,pst_ps_info);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (1 == uc_show_ip_addr)
    {
        for (ul_loop = 0; ul_loop < DMAC_MAX_IPV4_ENTRIES; ul_loop++)
        {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{dmac_config_show_ip_addr::IPv4 index[%d]: %d.X.X.%d. MASK[0x%08X]}",
                             ul_loop,
                             pst_dmac_vap->pst_ip_addr_info->ast_ipv4_entry[ul_loop].un_local_ip.auc_value[0],
                             pst_dmac_vap->pst_ip_addr_info->ast_ipv4_entry[ul_loop].un_local_ip.auc_value[3],
                             pst_dmac_vap->pst_ip_addr_info->ast_ipv4_entry[ul_loop].un_mask.ul_value);
        }

        for (ul_loop = 0; ul_loop < DMAC_MAX_IPV6_ENTRIES; ul_loop++)
        {
            OAM_WARNING_LOG_ALTER(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{dmac_config_show_ip_addr::IPv6 index[%d]: %04x:%04x:XXXX:XXXX:XXXX:XXXX:%04x:%04x.}",
                                  5,
                                  ul_loop,
                                  OAL_NET2HOST_SHORT(((oal_uint16 *)&(pst_dmac_vap->pst_ip_addr_info->ast_ipv6_entry[ul_loop].auc_ip_addr))[0]),
                                  OAL_NET2HOST_SHORT(((oal_uint16 *)&(pst_dmac_vap->pst_ip_addr_info->ast_ipv6_entry[ul_loop].auc_ip_addr))[1]),
                                  OAL_NET2HOST_SHORT(((oal_uint16 *)&(pst_dmac_vap->pst_ip_addr_info->ast_ipv6_entry[ul_loop].auc_ip_addr))[6]),
                                  OAL_NET2HOST_SHORT(((oal_uint16 *)&(pst_dmac_vap->pst_ip_addr_info->ast_ipv6_entry[ul_loop].auc_ip_addr))[7]));
        }
    }

    if (1 == uc_show_arpoffload_info)
    {
        OAM_WARNING_LOG3(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR,"suspend state:[%d],arpofflad drop frame:[%d],send arp rsp:[%d]",pst_mac_dev->uc_in_suspend,g_ul_arpoffload_drop_frame,g_ul_arpoffload_send_arprsp);
    }
    /* 统计清零 */
    else if (0 == uc_show_arpoffload_info)
    {
        g_ul_arpoffload_drop_frame  = 0;
        g_ul_arpoffload_send_arprsp = 0;
    }
    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_uint32 dmac_config_enable_2040bss(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru           *pst_mac_device;
    oal_bool_enum_uint8        en_2040bss_switch;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{dmac_config_enable_2040bss:: pointer is null,pst_mac_vap[%d], puc_param[%d] .}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_enable_2040bss::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_2040bss_switch = (*puc_param == 0) ? OAL_FALSE : OAL_TRUE;
    mac_set_2040bss_switch(pst_mac_device, en_2040bss_switch);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_enable_2040bss::set 2040bss switch[%d].}", en_2040bss_switch);

    return OAL_SUCC;
}
#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST

OAL_STATIC oal_uint32 dmac_config_send_cw_signal(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8
*puc_param)
{
    dmac_vap_stru                   *pst_dmac_vap;
    hal_to_dmac_device_stru         *pst_hal_device;
    wlan_channel_band_enum_uint8     en_band;
    oal_uint8                        uc_hipriv_ack = OAL_FALSE;

    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_cw_signal::pst_dmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = pst_dmac_vap->pst_hal_device;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
                {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_config_send_cw_signal::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 写测试寄存器与读测试结果寄存器，检查寄存器配置是否生效*/
    en_band = pst_mac_vap->st_channel.en_band;

    /*0: 关闭单音*/
    if ( 0 == *puc_param )
    {
      hal_revert_cw_signal_reg(pst_hal_device, en_band);
    }
    /*1: 通道0单音*/
    else if ( 1 == *puc_param )
    {
      hal_cfg_cw_signal_reg(pst_hal_device, 0, en_band);
    }
    /*2: 通道1单音*/
    else if ( 2 == *puc_param )
    {
      hal_cfg_cw_signal_reg(pst_hal_device, 1, en_band);
    }
    /*3: 读相关寄存器*/
    else if ( 3 == *puc_param )
    {
      hal_get_cw_signal_reg(pst_hal_device, 0, en_band);
      hal_get_cw_signal_reg(pst_hal_device, 1, en_band);
    }

    /* 命令成功返回Success */
    uc_hipriv_ack = OAL_TRUE;
    dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_SIZEOF(oal_uint8), &uc_hipriv_ack);

    return OAL_SUCC;
}
#endif

OAL_CONST dmac_config_syn_stru g_ast_dmac_config_syn_debug[] =
{
    {WLAN_CFGID_CHECK_FEM_PA,        {0, 0},            dmac_config_get_fem_pa_status},
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    {WLAN_CFGID_CHIP_CHECK_SWITCH,          {0, 0},       dmac_config_chip_check},
    {WLAN_CFGID_SEND_CW_SIGNAL,             {0, 0},       dmac_config_send_cw_signal},
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_80211_UCAST_SWITCH, {0, 0},         dmac_config_80211_ucast_switch},
#ifdef _PRE_DEBUG_MODE
	{WLAN_CFGID_REPORT_VAP_INFO,         {0, 0},    dmac_config_report_vap_info},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#ifdef _PRE_DEBUG_MODE_USER_TRACK
    {WLAN_CFGID_USR_THRPUT_STAT,            {0, 0},         dmac_config_report_thrput_stat},
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    {WLAN_CFGID_TXOP_PS_MACHW,              {0, 0},         dmac_config_set_txop_ps_machw},
#endif
    {WLAN_CFGID_80211_MCAST_SWITCH, {0, 0},         dmac_config_80211_mcast_switch},
    {WLAN_CFGID_PROBE_SWITCH,       {0, 0},         dmac_config_probe_switch},
    {WLAN_CFGID_RSSI_SWITCH,        {0, 0},         dmac_config_rssi_switch},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_WMMAC
    {WLAN_CFGID_WMMAC_SWITCH,        {0, 0},        dmac_config_wmmac_switch},
#endif
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_GET_MPDU_NUM,       {0, 0},         dmac_config_get_mpdu_num},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_WLAN_CHIP_TEST
    {WLAN_CFGID_SET_BEACON_OFFLOAD_TEST, {0, 0},    dmac_config_beacon_offload_test},
#endif
    {WLAN_CFGID_OTA_BEACON_SWITCH,  {0, 0},         dmac_config_ota_beacon_switch},
    {WLAN_CFGID_OTA_RX_DSCR_SWITCH, {0, 0},         dmac_config_ota_rx_dscr_switch},
    {WLAN_CFGID_SET_ALL_OTA,        {0, 0},         dmac_config_set_all_ota},
#ifdef _PRE_WLAN_RF_110X_CALI_DPD
    {WLAN_CFGID_START_DPD,         {0, 0},          dmac_config_start_dpd},
#endif
    {WLAN_CFGID_OAM_OUTPUT_TYPE,    {0, 0},         dmac_config_oam_output},
    {WLAN_CFGID_SET_FEATURE_LOG,    {0, 0},         dmac_config_set_feature_log},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DUMP_TIEMR,         {0, 0},         dmac_config_dump_timer},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_STBC_CAP,       {0, 0},         dmac_config_set_stbc_cap},
    {WLAN_CFGID_SET_LDPC_CAP,       {0, 0},         dmac_config_set_ldpc_cap},
#endif
    {WLAN_CFGID_PAUSE_TID,          {0, 0},         dmac_config_pause_tid},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_USER_VIP,       {0, 0},         dmac_config_set_user_vip},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_VAP_HOST,       {0, 0},         dmac_config_set_vap_host},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SEND_BAR,                   {0, 0},         dmac_config_send_bar},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DUMP_BA_BITMAP,    {0, 0},              dmac_config_dump_ba_bitmap},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_RESET_HW,                   {0, 0},         dmac_config_reset_hw},
#endif //#ifdef _PRE_DEBUG_MODE
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_RESET_HW_OPERATE,           {0, 0},         dmac_reset_sys_event},
    {WLAN_CFGID_SET_MIB,            {0, 0},             dmac_config_set_mib},
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
    {WLAN_CFGID_UAPSD_DEBUG,        {0, 0},         dmac_config_uapsd_debug},
#endif

#ifdef _PRE_WLAN_DFT_STAT
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_PHY_STAT_EN,                {0, 0},         dmac_config_set_phy_stat_en},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DBB_ENV_PARAM,              {0, 0},         dmac_config_dbb_env_param},
#endif //#ifdef _PRE_DEBUG_MODE
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    {WLAN_CFGID_USR_QUEUE_STAT,             {0, 0},         dmac_config_usr_queue_stat},
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_VAP_STAT,                   {0, 0},         dmac_config_report_vap_stat},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_ALL_STAT,                   {0, 0},         dmac_config_report_all_stat},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
    {WLAN_CFGID_DUMP_RX_DSCR,               {0, 0},         dmac_config_dump_rx_dscr},
    {WLAN_CFGID_DUMP_TX_DSCR,               {0, 0},         dmac_config_dump_tx_dscr},
    {WLAN_CFGID_ALG,               {0, 0},              dmac_config_alg},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_BEACON_CHAIN_SWITCH,       {0, 0},      dmac_config_beacon_chain_switch},
#endif //#ifdef _PRE_DEBUG_MODE
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_RESUME_RX_INTR_FIFO, {0, 0},            dmac_config_resume_rx_intr_fifo},
#endif
#ifdef _PRE_SUPPORT_ACS
    {WLAN_CFGID_ACS_CONFIG,         {0, 0},             dmac_config_acs},
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGIG_DFR_ENABLE,                 {0, 0},         dmac_config_dfr_enable},
    {WLAN_CFGID_TRIG_LOSS_TX_COMP,          {0, 0},         dmac_config_trig_loss_tx_comp},
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    {WLAN_CFGID_TRIG_PCIE_RESET,            {0, 0},         dmac_config_trig_pcie_reset},
#endif
#endif
#endif
#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_NSS,           {0, 0},              dmac_config_set_nss},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_RFCH,          {0, 0},              dmac_config_set_rfch},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_ALWAYS_TX,     {0, 0},              dmac_config_set_always_tx},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_RXCH,          {0, 0},              dmac_config_set_rxch},
    {WLAN_CFGID_DYNC_TXPOWER,      {0, 0},              dmac_config_dync_txpower},
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_GET_THRUPUT,       {0, 0},              dmac_config_get_thruput},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_FREQ_SKEW,     {0, 0},              dmac_config_set_freq_skew},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_ADJUST_PPM,        {0, 0},              dmac_config_adjust_ppm},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DBB_SCALING_AMEND,          {0, 0},     dmac_config_dbb_scaling_amend},
#ifdef _PRE_WLAN_PERFORM_STAT
    {WLAN_CFGID_PFM_STAT,                  {0, 0},      dmac_config_pfm_stat},
    {WLAN_CFGID_PFM_DISPLAY,               {0, 0},      dmac_config_pfm_display},
#endif
#ifdef _PRE_WLAN_CHIP_TEST

    {WLAN_CFGID_LPM_TX_PROBE_REQUEST,     {0, 0},       dmac_config_lpm_tx_probe_request},
    {WLAN_CFGID_LPM_CHIP_STATE,     {0, 0},             dmac_config_set_lpm_chip_state},
    {WLAN_CFGID_LPM_SOC_MODE,      {0, 0},              dmac_config_set_lpm_soc_mode},
    {WLAN_CFGID_LPM_PSM_PARAM,      {0, 0},             dmac_config_set_lpm_psm_param},
    {WLAN_CFGID_LPM_SMPS_MODE,      {0, 0},             dmac_config_set_lpm_smps_mode},
    {WLAN_CFGID_LPM_SMPS_STUB,      {0, 0},             dmac_config_set_lpm_smps_stub},
    {WLAN_CFGID_LPM_TXOP_PS_SET,    {0, 0},             dmac_config_set_lpm_txop_ps},
    {WLAN_CFGID_LPM_TXOP_TX_STUB,   {0, 0},             dmac_config_set_lpm_txop_ps_tx_stub},
    {WLAN_CFGID_SEND_FRAME,         {0, 0},             dmac_config_send_frame},
    {WLAN_CFGID_SET_RX_PN_REG,      {0, 0},             dmac_config_set_rx_pn},
    {WLAN_CFGID_SET_SOFT_RETRY,     {0, 0},             dmac_config_set_soft_retry},
    {WLAN_CFGID_OPEN_ADDR4,         {0, 0},             dmac_config_open_addr4},
    {WLAN_CFGID_OPEN_WMM_TEST,      {0, 0},             dmac_config_open_wmm_test},
    {WLAN_CFGID_CHIP_TEST_OPEN,     {0, 0},             dmac_config_chip_test_open},
    {WLAN_CFGID_SET_COEX,           {0, 0},             dmac_config_set_coex},
    {WLAN_CFGID_DFX_SWITCH,         {0, 0},             dmac_config_set_dfx},
    {WLAN_CFGID_REMOVE_LUT,         {0, 0},             dmac_config_remove_user_lut},
    {WLAN_CFGID_SEND_PSPOLL,        {0, 0},             dmac_config_send_pspoll},
    {WLAN_CFGID_SEND_NULLDATA,      {0, 0},             dmac_config_send_nulldata},
    {WLAN_CFGID_CLEAR_ALL_STAT,    {0, 0},              dmac_config_clear_all_stat},

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    {WLAN_CFGID_PMF_ENABLE,         {0, 0},             dmac_config_enable_pmf},
#endif
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_HIDE_SSID,                 {0, 0},      dmac_config_hide_ssid},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_THRUPUT_BYPASS,  {0, 0},            dmac_config_set_thruput_bypass},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_GET_ALL_REG_VALUE,   {0, 0},            dmac_config_get_all_reg_value},
#endif
#ifdef _PRE_WLAN_FEATURE_DAQ
    {WLAN_CFGID_DATA_ACQ,            {0, 0},            dmac_config_data_acq},
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_VAP_NSS,         {0, 0},            dmac_config_set_vap_nss},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_REPORT_AMPDU_STAT,   {0, 0},            dmac_config_report_ampdu_stat},
#endif
    {WLAN_CFGID_SET_AGGR_NUM,        {0, 0},            dmac_config_set_ampdu_aggr_num},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_FREQ_ADJUST,         {0, 0},            dmac_config_freq_adjust},
#endif

#ifdef _PRE_WLAN_FEATURE_PM
    {WLAN_CFGID_WIFI_EN,          {0, 0},               dmac_config_wifi_en},
    {WLAN_CFGID_PM_INFO,          {0, 0},               dmac_config_pm_info},
    {WLAN_CFGID_PM_EN,          {0, 0},                 dmac_config_pm_en},
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DEVICE_MEM_LEAK,   {0, 0},              dmac_config_show_device_memleak},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DEVICE_MEM_INFO,   {0, 0},              dmac_config_show_device_meminfo},
#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_P2P_PS_STAT,    {0, 0},                 dmac_config_set_p2p_ps_stat},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#ifdef _PRE_WLAN_PROFLING_MIPS
    {WLAN_CFGID_SET_MIPS,           {0, 0},             dmac_config_set_mips},
    {WLAN_CFGID_SHOW_MIPS,          {0, 0},             dmac_config_show_mips},
#endif
#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_ENABLE_ARP_OFFLOAD,         {0, 0},     dmac_config_enable_arp_offload},
    {WLAN_CFGID_SHOW_ARPOFFLOAD_INFO,       {0, 0},     dmac_config_show_arpoffload_info},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_WLAN_DFT_STAT
    {WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH,  {0, 0},            dmac_config_set_performance_log_switch},
#endif
#endif //#ifdef _PRE_DEBUG_MODE

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    {WLAN_CFGID_2040BSS_ENABLE,     {0, 0},             dmac_config_enable_2040bss},
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
    {WLAN_CFGID_FBT_SCAN_LIST_CLEAR,        {0, 0},     dmac_config_clear_fbt_scan_list},
    {WLAN_CFGID_FBT_SCAN_SPECIFIED_STA,     {0, 0},     dmac_config_fbt_scan_specified_sta},
    {WLAN_CFGID_FBT_SCAN_INTERVAL,          {0, 0},     dmac_config_fbt_scan_interval},
    {WLAN_CFGID_FBT_SCAN_CHANNEL,           {0, 0},     dmac_config_fbt_scan_channel},
    {WLAN_CFGID_FBT_SCAN_REPORT_PERIOD,     {0, 0},     dmac_config_fbt_scan_report_period},
    {WLAN_CFGID_FBT_SCAN_ENABLE,            {0, 0},     dmac_config_fbt_scan_enable},
#endif
    {WLAN_CFGID_BUTT,               {0, 0},             OAL_PTR_NULL},
};

oal_uint32 dmac_get_config_debug_arrysize(oal_void)
{
    return OAL_ARRAY_SIZE(g_ast_dmac_config_syn_debug);
}

#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

