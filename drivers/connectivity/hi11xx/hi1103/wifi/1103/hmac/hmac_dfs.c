


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_DFS

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_fsm.h"
#include "hmac_chan_mgmt.h"
//#include "dmac_scan.h"
#include "hmac_dfs.h"
#include "hmac_scan.h"
#include "hmac_resource.h"
#include "hmac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_DFS_C


/*****************************************************************************
  2 结构体定义
*****************************************************************************/


/*****************************************************************************
  3 宏定义
*****************************************************************************/
#define HMAC_DFS_IS_CHAN_WEATHER_RADAR(_us_freq) \
    ((5600 <= (_us_freq)) && ((_us_freq) <= 5650))


/*****************************************************************************
  4 全局变量定义
*****************************************************************************/


/*****************************************************************************
  5 内部静态函数声明
*****************************************************************************/
OAL_STATIC oal_uint32  hmac_dfs_nol_addchan(mac_device_stru *pst_mac_device, oal_uint8 uc_chan_idx);
OAL_STATIC oal_uint32  hmac_dfs_nol_delchan(mac_device_stru *pst_mac_device, mac_dfs_nol_node_stru *pst_nol_node);
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC
OAL_STATIC oal_uint32  hmac_scan_switch_channel_back(mac_vap_stru *pst_mac_vap);
OAL_STATIC oal_uint32  hmac_scan_switch_channel_off(mac_vap_stru *pst_mac_vap);
#endif
OAL_STATIC oal_uint32  hmac_dfs_off_chan_cac_off_ch_dwell_timeout(oal_void *p_arg);
OAL_STATIC oal_uint32  hmac_dfs_off_chan_cac_opern_ch_dwell_timeout(oal_void *p_arg);


/*****************************************************************************
  4 函数实现
*****************************************************************************/

oal_void  hmac_dfs_init_etc(mac_device_stru *pst_mac_device)
{
    mac_dfs_info_stru   *pst_dfs_info;

    /* 初始化Non-Occupancy List链表 */
    oal_dlist_init_head(&(pst_mac_device->st_dfs.st_dfs_nol));

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* CAC检测默认不使能 */
    mac_dfs_set_cac_enable(pst_mac_device, OAL_TRUE);
    /* OFFCHAN-CAC检测默认不使能 */
    mac_dfs_set_offchan_cac_enable(pst_mac_device, OAL_FALSE);
#else
    /* CAC检测默认不使能 */
    mac_dfs_set_cac_enable(pst_mac_device, OAL_TRUE);
    /* OFFCHAN-CAC检测默认不使能 */
    mac_dfs_set_offchan_cac_enable(pst_mac_device, OAL_FALSE);
#endif

    /* 设置CAC, Off-Channel CAC, etc... 超时时间 */
    pst_dfs_info = &(pst_mac_device->st_dfs.st_dfs_info);

    pst_dfs_info->ul_dfs_cac_outof_5600_to_5650_time_ms      = HMAC_DFS_CAC_OUTOF_5600_TO_5650_MHZ_TIME_MS;
    pst_dfs_info->ul_dfs_cac_in_5600_to_5650_time_ms         = HMAC_DFS_CAC_IN_5600_TO_5650_MHZ_TIME_MS;
    pst_dfs_info->ul_off_chan_cac_outof_5600_to_5650_time_ms = HMAC_DFS_OFF_CH_CAC_OUTOF_5600_TO_5650_MHZ_TIME_MS;
    pst_dfs_info->ul_off_chan_cac_in_5600_to_5650_time_ms    = HMAC_DFS_OFF_CH_CAC_IN_5600_TO_5650_MHZ_TIME_MS;
    pst_dfs_info->us_dfs_off_chan_cac_opern_chan_dwell_time  = HMAC_DFS_OFF_CHAN_CAC_PERIOD_TIME_MS;
    pst_dfs_info->us_dfs_off_chan_cac_off_chan_dwell_time    = HMAC_DFS_OFF_CHAN_CAC_DWELL_TIME_MS;
    pst_dfs_info->ul_dfs_non_occupancy_period_time_ms        = HMAC_DFS_NON_OCCUPANCY_PERIOD_TIME_MS;
    pst_dfs_info->en_dfs_init                                = OAL_FALSE;

    /* 默认当前信道为home channel */
    pst_dfs_info->uc_offchan_flag = 0;
}


oal_void  hmac_dfs_channel_list_init_etc(mac_device_stru *pst_mac_device)
{
    mac_chan_status_enum_uint8    en_ch_status;
    oal_uint8                     uc_idx;
    oal_uint32                    ul_ret;
#if 0
    if (WLAN_BAND_5G != pst_mac_device->en_max_band)
    {
        OAL_IO_PRINT("[DFS]hmac_dfs_channel_list_init_etc::band is not 5G.\n.");
        return;
    }
#endif
    for (uc_idx = 0; uc_idx < MAC_MAX_SUPP_CHANNEL; uc_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_5, uc_idx);
        if (OAL_SUCC == ul_ret)
        {
            if (OAL_TRUE == mac_is_ch_in_radar_band(MAC_RC_START_FREQ_5, uc_idx))
            {
                /* DFS信道 */
                en_ch_status = MAC_CHAN_DFS_REQUIRED;
            }
            else
            {
                en_ch_status = MAC_CHAN_AVAILABLE_ALWAYS;
            }
        }
        else
        {
            /* 管制域不支持 */
            en_ch_status = MAC_CHAN_NOT_SUPPORT;
        }

        pst_mac_device->st_ap_channel_list[uc_idx].en_ch_status = en_ch_status;
    }
}


OAL_STATIC oal_uint8  hmac_dfs_find_lowest_available_channel(mac_device_stru *pst_mac_device)
{
    oal_uint8     uc_channel_num = 36;
    oal_uint8     uc_chan_idx;
    oal_uint8     uc_num_supp_chan = mac_get_num_supp_channel(pst_mac_device->en_max_band);
    oal_uint32    ul_ret;

    for (uc_chan_idx = 0; uc_chan_idx < uc_num_supp_chan; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(pst_mac_device->en_max_band, uc_chan_idx);
        if (OAL_SUCC != ul_ret)
        {
            continue;
        }

        if ((MAC_CHAN_NOT_SUPPORT        != pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status) &&
            (MAC_CHAN_BLOCK_DUE_TO_RADAR != pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status))
        {
            mac_get_channel_num_from_idx_etc(pst_mac_device->en_max_band, uc_chan_idx, &uc_channel_num);

            return uc_channel_num;
        }
    }

    /* should not be here */
    return uc_channel_num;
}


oal_uint32  hmac_dfs_recalculate_channel_etc(
                    mac_device_stru                     *pst_mac_device,
                    oal_uint8                           *puc_freq,
                    wlan_channel_bandwidth_enum_uint8   *pen_bandwidth)
{
    mac_channel_list_stru    st_chan_info;
    oal_uint8                 uc_chan_idx;
    oal_uint8                 uc_loop;
    oal_uint32                ul_ret;
    oal_bool_enum_uint8       en_recalc = OAL_FALSE;

    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_device->en_max_band, *puc_freq, &uc_chan_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_device->uc_device_id, OAM_SF_DFS, "{hmac_dfs_recalculate_channel_etc::mac_get_channel_idx_from_num_etc fail.max_band:%d  freq:%x}",pst_mac_device->en_max_band,puc_freq);
        return ul_ret;
    }

    mac_get_ext_chan_info(uc_chan_idx, *pen_bandwidth, &st_chan_info);

    for (uc_loop = 0; uc_loop < st_chan_info.ul_channels; uc_loop++)
    {
        uc_chan_idx = st_chan_info.ast_channels[uc_loop].uc_idx;

        if ((MAC_CHAN_NOT_SUPPORT        == pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status) ||
            (MAC_CHAN_BLOCK_DUE_TO_RADAR == pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status))
        {
            en_recalc = OAL_TRUE;
            break;
        }
    }

    if (OAL_FALSE == en_recalc)
    {
        return OAL_FALSE;
    }


    /* 如果由于雷达干扰导致当前信道不可用，则需要从管制域支持的信道中，选择最低可用信道，带宽20MHz */
    *puc_freq = hmac_dfs_find_lowest_available_channel(pst_mac_device);

    *pen_bandwidth = WLAN_BAND_WIDTH_20M;

    return OAL_TRUE;
}

static OAL_INLINE oal_uint32  hmac_dfs_cac_event_report(mac_vap_stru *pst_mac_vap,
                                                        hmac_cac_event_stru *pst_cac_event)
{
    frw_event_mem_stru       *pst_event_mem;
    frw_event_stru           *pst_event;

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_cac_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{[DFS]hmac_dfs_cac_event_report::pst_event_mem null.}");
        return OAL_FAIL;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_CAC_REPORT,
                       OAL_SIZEOF(hmac_cac_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* cac事件 */
    oal_memcopy(frw_get_event_payload(pst_event_mem), (const oal_void*)pst_cac_event,OAL_SIZEOF(hmac_cac_event_stru));

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);
    return OAL_SUCC;
}



oal_uint32  hmac_dfs_radar_detect_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru         *pst_event;
    hmac_vap_stru          *pst_hmac_vap;
    hmac_misc_input_stru    st_misc_input;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{hmac_dfs_radar_detect_event_etc::pst_event_mem is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{hmac_dfs_radar_detect_event_etc::pst_hmac_vap is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    st_misc_input.en_type = HMAC_MISC_RADAR;

    hmac_fsm_call_func_ap_etc(pst_hmac_vap, HMAC_FSM_INPUT_MISC, &st_misc_input);

    return OAL_SUCC;
}

oal_uint32  hmac_dfs_radar_detect_event_test(oal_uint8 uc_vap_id)
{
    hmac_vap_stru          *pst_hmac_vap;
    hmac_misc_input_stru    st_misc_input;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{hmac_dfs_radar_detect_event_test::pst_hmac_vap is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    st_misc_input.en_type = HMAC_MISC_RADAR;

    hmac_fsm_call_func_ap_etc(pst_hmac_vap, HMAC_FSM_INPUT_MISC, &st_misc_input);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_dfs_update_available_channel_list(
                mac_device_stru       *pst_mac_device,
                oal_uint8              uc_chan_idx,
                oal_bool_enum_uint8    en_radar_detected)
{
    oal_uint32    ul_ret;

    ul_ret = mac_is_channel_idx_valid_etc(pst_mac_device->en_max_band, uc_chan_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_DFS, "{hmac_dfs_update_available_channel_list::chan_idx(%d) invalid!}", uc_chan_idx);
        OAL_IO_PRINT("[DFS]hmac_dfs_update_available_channel_list::ch is not available.\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    if (MAC_CHAN_AVAILABLE_ALWAYS == pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "{hmac_dfs_update_available_channel_list::Radar detected in Non-Radar Channel(%d)!}", uc_chan_idx);
        //OAL_IO_PRINT("[DFS]hmac_dfs_update_available_channel_list::ch status is always available.\n");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    if (OAL_TRUE == en_radar_detected)
    {
        pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status = MAC_CHAN_BLOCK_DUE_TO_RADAR;
        OAL_IO_PRINT("[DFS]ch status is changed to due to radar.\n");
    }
    else
    {
        pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status = MAC_CHAN_AVAILABLE_TO_OPERATE;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    hmac_config_ch_status_sync(pst_mac_device);
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_dfs_cac_timeout_fn_etc(oal_void *p_arg)
{
    mac_device_stru          *pst_mac_device;
    hmac_device_stru         *pst_hmac_device;
    hmac_vap_stru            *pst_hmac_vap;
    mac_channel_list_stru    st_chan_info;
    oal_uint8                 uc_idx;
    mac_dfs_info_stru        *pst_dfs_info;
    oal_uint32                ul_ret;
    hmac_cac_event_stru       st_cac_event;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_arg))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_cac_timeout_fn_etc::p_arg is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    OAL_IO_PRINT("[DFS]hmac_dfs_cac_timeout_fn_etc, CAC expired, channel number:%d.\n",
                 pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_number);

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device || OAL_PTR_NULL == pst_hmac_device->pst_device_base_info)
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_cac_timeout_fn_etc::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_hmac_device->pst_device_base_info;

    pst_dfs_info = &pst_mac_device->st_dfs.st_dfs_info;

    pst_dfs_info->uc_timer_cnt++;

    if(pst_dfs_info->uc_timer_cnt != pst_dfs_info->uc_timer_end_cnt)
    {
        /* 启动 CAC 定时器 */
        FRW_TIMER_CREATE_TIMER(&pst_mac_device->st_dfs.st_dfs_cac_timer,
                               hmac_dfs_cac_timeout_fn_etc,
                               HMAC_DFS_ONE_MIN_IN_MS,
                               pst_hmac_vap,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
        return OAL_SUCC;
    }

    mac_get_ext_chan_info(pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_idx,
                    pst_hmac_vap->st_vap_base_info.st_channel.en_bandwidth,
                    &st_chan_info);

    /* 将当前信道设置为工作信道 */
    for (uc_idx = 0; uc_idx < st_chan_info.ul_channels; uc_idx++)
    {
        hmac_dfs_update_available_channel_list(pst_mac_device, st_chan_info.ast_channels[uc_idx].uc_idx, OAL_FALSE);
    }

    if (MAC_VAP_STATE_AP_WAIT_START == pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
        pst_hmac_device->en_init_scan      = OAL_FALSE;
        pst_hmac_device->en_in_init_scan   = OAL_FALSE;
#endif
        /* 在该信道启动 BSS */
        hmac_start_all_bss_of_device_etc(pst_hmac_device);

    } /* mayuan TBD pause状态需要待定 */
    else if ((MAC_VAP_STATE_PAUSE == pst_hmac_vap->st_vap_base_info.en_vap_state) ||
             (MAC_VAP_STATE_UP    == pst_hmac_vap->st_vap_base_info.en_vap_state))
    {
        //hmac_vap_resume_tx_by_chl(pst_hmac_vap);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
        /*cac超时后把vap状态置为up,恢复发送队列*/
        hmac_cac_chan_ctrl_machw_tx(&(pst_hmac_vap->st_vap_base_info), OAL_TRUE);
#else
        hmac_chan_restart_network_after_switch_etc(&(pst_hmac_vap->st_vap_base_info));
#endif
    }

    /* off-channel cac start */
    if(OAL_TRUE == mac_dfs_get_offchan_cac_enable(pst_mac_device))
    {
        hmac_dfs_off_chan_cac_start_etc(pst_mac_device, pst_hmac_vap);
    }


    //调用内核接口通知应用层CAC启动
    uc_idx = pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_idx;
    ul_ret = mac_get_channel_idx_from_num_etc(MAC_RC_START_FREQ_5,pst_mac_device->uc_max_channel,&uc_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "{[DFS]hmac_dfs_cac_timeout_fn_etc::mac_get_channel_idx_from_num_etc failed=%d}",ul_ret);
    }
    st_cac_event.en_type = HMAC_CAC_FINISHED;
    st_cac_event.ul_freq = g_ast_freq_map_5g_etc[uc_idx].us_freq;
    st_cac_event.en_bw_mode = pst_hmac_vap->st_vap_base_info.st_channel.en_bandwidth;
    ul_ret = hmac_dfs_cac_event_report(&pst_hmac_vap->st_vap_base_info, &st_cac_event);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "{[DFS]hmac_dfs_cac_timeout_fn_etc::hmac_dfs_cac_event_report failed=%d}",ul_ret);
    }

    return OAL_SUCC;
}



oal_uint32  hmac_dfs_start_bss_etc(hmac_vap_stru *pst_hmac_vap)
{
    mac_device_stru          *pst_mac_device;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_start_bss_etc::pst_hmac_vap is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_cac_timeout_fn_etc::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 在该信道启动 BSS */
    return hmac_start_bss_in_available_channel_etc(pst_hmac_vap);

}



OAL_STATIC oal_uint32  hmac_chan_get_cac_time(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    mac_regdomain_info_stru   *pst_rd_info;
    mac_channel_list_stru     st_chan_info;
    oal_uint8                  uc_idx;
    oal_uint32                 ul_ret;

    mac_get_regdomain_info_etc(&pst_rd_info);

    if (MAC_DFS_DOMAIN_ETSI == pst_rd_info->en_dfs_domain)
    {
        ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number, &uc_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG2(0, OAM_SF_DFS, "{hmac_chan_get_cac_time::mac_get_channel_idx_from_num_etc fail. en_band:%d, offchan_num:%u}",
                    pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number);
            return ul_ret;
        }
        mac_get_ext_chan_info(uc_idx,
                        pst_mac_vap->st_channel.en_bandwidth, &st_chan_info);

        for (uc_idx = 0; uc_idx < st_chan_info.ul_channels; uc_idx++)
        {
            if (HMAC_DFS_IS_CHAN_WEATHER_RADAR(st_chan_info.ast_channels[uc_idx].us_freq))
            {
                return pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_in_5600_to_5650_time_ms;
            }
        }
    }

    return pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_outof_5600_to_5650_time_ms;
}



OAL_STATIC oal_uint32  hmac_chan_get_off_chan_cac_time(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    mac_channel_list_stru     st_chan_info;
    oal_uint8                  uc_idx;

    mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, pst_mac_device->st_dfs.st_dfs_info.uc_offchan_num, &uc_idx);

    mac_get_ext_chan_info(uc_idx,
                    pst_mac_vap->st_channel.en_bandwidth, &st_chan_info);

    for (uc_idx = 0; uc_idx < st_chan_info.ul_channels; uc_idx++)
    {
        if (HMAC_DFS_IS_CHAN_WEATHER_RADAR(st_chan_info.ast_channels[uc_idx].us_freq))
        {
            return pst_mac_device->st_dfs.st_dfs_info.ul_off_chan_cac_in_5600_to_5650_time_ms;
        }
    }

    return pst_mac_device->st_dfs.st_dfs_info.ul_off_chan_cac_outof_5600_to_5650_time_ms;
}



oal_void  hmac_dfs_cac_stop_etc(mac_device_stru *pst_mac_device,mac_vap_stru *pst_mac_vap)
{
    oal_uint32  ul_ret;
    oal_uint8   uc_idx = 0;
    hmac_cac_event_stru   st_cac_event;

    if (OAL_TRUE == pst_mac_device->st_dfs.st_dfs_cac_timer.en_is_enabled)
    {
        /* 关闭CAC检测时长定时器 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_mac_device->st_dfs.st_dfs_cac_timer));
        pst_mac_device->st_dfs.st_dfs_cac_timer.en_is_enabled = OAL_FALSE;

        //调用内核接口通知应用层CAC停止
        uc_idx = pst_mac_vap->st_channel.uc_chan_idx;
        st_cac_event.en_type = HMAC_CAC_ABORTED;
        st_cac_event.ul_freq = g_ast_freq_map_5g_etc[uc_idx].us_freq;
        st_cac_event.en_bw_mode = pst_mac_vap->st_channel.en_bandwidth;
        ul_ret = hmac_dfs_cac_event_report(pst_mac_vap, &st_cac_event);

        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_DFS, "{[DFS]hmac_dfs_cac_stop_etc::hmac_dfs_cac_event_report failed=%d}",ul_ret);
        }

        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_cac_stop_etc::[DFS]CAC timer stopped.}");
    }
}

oal_void  hmac_dfs_off_cac_stop_etc(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    if (OAL_TRUE == pst_mac_device->st_dfs.st_dfs_off_chan_cac_timer.en_is_enabled)
    {
        /* 关闭OFF CAC检测时长定时器 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_mac_device->st_dfs.st_dfs_off_chan_cac_timer));

        OAM_INFO_LOG0(0, OAM_SF_DFS, "{hmac_dfs_cac_stop_etc::[DFS]CAC big timer stopped.}");
    }
    if (OAL_TRUE == pst_mac_device->st_dfs.st_dfs_chan_dwell_timer.en_is_enabled)
    {
        /* 关闭OFFCAC信道驻留定时器 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_mac_device->st_dfs.st_dfs_chan_dwell_timer));

        OAM_INFO_LOG0(0, OAM_SF_DFS, "{hmac_dfs_cac_stop_etc::[DFS]CAC timer stopped.}");
    }
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC
    /* 若VAP在Off-Channel信道上则切回工作信道 */
    if (pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag & BIT0)
    {
        OAM_INFO_LOG0(0, OAM_SF_DFS, "{hmac_dfs_cac_stop_etc::[DFS]switch back to home channel.}");
        hmac_scan_switch_channel_back(pst_mac_vap);
    }
#endif
    pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag = 0;
}



oal_void  hmac_dfs_cac_start_etc(mac_device_stru *pst_mac_device, hmac_vap_stru *pst_hmac_vap)
{
    oal_uint32   ul_scan_time = 0;
    oal_uint32   ul_ret;
    oal_uint8    uc_idx = 0;
    hmac_cac_event_stru st_cac_event;


    /* 如果已经启动 CAC 定时器，则直接返回 */
    if (OAL_TRUE == pst_mac_device->st_dfs.st_dfs_cac_timer.en_is_enabled)
    {
        return;
    }

    /* 设置 CAC 检测时间 */
    ul_scan_time = hmac_chan_get_cac_time(pst_mac_device, &(pst_hmac_vap->st_vap_base_info));
    OAM_WARNING_LOG2(0, OAM_SF_DFS, "start cac time=%d ms ch=%d\n", ul_scan_time, pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_number);

    if(HMAC_DFS_ONE_MIN_IN_MS > ul_scan_time)
    {
        pst_mac_device->st_dfs.st_dfs_info.uc_timer_cnt     = 0;
        pst_mac_device->st_dfs.st_dfs_info.uc_timer_end_cnt = 1;

        /* 启动 CAC 定时器 */
        FRW_TIMER_CREATE_TIMER(&pst_mac_device->st_dfs.st_dfs_cac_timer,
                               hmac_dfs_cac_timeout_fn_etc,
                               ul_scan_time,
                               pst_hmac_vap,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
    }
    else
    {
        pst_mac_device->st_dfs.st_dfs_info.uc_timer_cnt     = 0;
        pst_mac_device->st_dfs.st_dfs_info.uc_timer_end_cnt = (oal_uint8)(ul_scan_time / HMAC_DFS_ONE_MIN_IN_MS);

        /* 启动 CAC 定时器 */
        FRW_TIMER_CREATE_TIMER(&pst_mac_device->st_dfs.st_dfs_cac_timer,
                               hmac_dfs_cac_timeout_fn_etc,
                               HMAC_DFS_ONE_MIN_IN_MS,
                               pst_hmac_vap,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
    }

    //通知wal CAC启动
    uc_idx = pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_idx;
    st_cac_event.en_type = HMAC_CAC_STARTED;
    st_cac_event.ul_freq = g_ast_freq_map_5g_etc[uc_idx].us_freq;
    st_cac_event.en_bw_mode = pst_hmac_vap->st_vap_base_info.st_channel.en_bandwidth;
    ul_ret = hmac_dfs_cac_event_report(&pst_hmac_vap->st_vap_base_info, &st_cac_event);

    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "{[DFS]hmac_dfs_cac_start_etc::hmac_dfs_cac_event_report failed=%d}",ul_ret);
    }


   OAM_WARNING_LOG2(0, OAM_SF_DFS, "{[DFS]hmac_dfs_cac_start_etc, CAC start, channel number:%d, timer cnt:%d.}",
                 pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_number, pst_mac_device->st_dfs.st_dfs_info.uc_timer_end_cnt);
}


oal_void  hmac_dfs_radar_wait_etc(mac_device_stru *pst_mac_device, hmac_vap_stru *pst_hmac_vap)
{
    mac_vap_stru   *pst_mac_vap;
    oal_uint8       uc_vap_idx;
    oal_uint8       uc_5g_vap_cnt = 0;

    if (!pst_hmac_vap || (WLAN_BAND_5G != pst_hmac_vap->st_vap_base_info.st_channel.en_band))
    {
        return;
    }

    /* 如果雷达使能没有开启直接返回 */
    if (OAL_FALSE == mac_dfs_get_dfs_enable(pst_mac_device))
    {
        return;
    }

    /* 遍历device下的所有vap，当所有5G vap都down后，将当前信道状态置为MAC_CHAN_DFS_REQUIRED */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_DFS, "{hmac_dfs_radar_wait_etc::pst_mac_vap null, vap id=%d.", pst_mac_device->auc_vap_id[uc_vap_idx]);

            continue;
        }

        if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_AP_WAIT_START == pst_mac_vap->en_vap_state) && (WLAN_BAND_5G == pst_mac_vap->st_channel.en_band))
        {
            uc_5g_vap_cnt++;
        }
    }

    if (0 == uc_5g_vap_cnt)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "{hmac_dfs_radar_wait_etc::reset dfs status for device[%d] after all 5g vap down", pst_mac_device->uc_device_id);

        hmac_dfs_channel_list_init_etc(pst_mac_device);
        pst_mac_device->st_dfs.st_dfs_info.en_dfs_init = OAL_TRUE;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        hmac_config_ch_status_sync(pst_mac_device);
#endif
    }
}

OAL_STATIC oal_uint32  hmac_dfs_nol_timeout_fn(oal_void *p_arg)
{
    mac_device_stru         *pst_mac_device;
    mac_dfs_nol_node_stru   *pst_nol_node;
    oal_uint8                uc_chan_num = 0;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_arg))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_nol_timeout_fn::p_arg is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_nol_node = (mac_dfs_nol_node_stru *)p_arg;

    pst_mac_device = mac_res_get_dev_etc(pst_nol_node->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_nol_timeout_fn::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_get_channel_num_from_idx_etc(pst_mac_device->en_max_band, pst_nol_node->uc_chan_idx, &uc_chan_num);

    OAM_INFO_LOG1(0, OAM_SF_DFS, "{[DFS]hmac_dfs_nol_timeout_fn, Non-Occupancy Period expired, remove channel %d from NOL.}", uc_chan_num);

    return hmac_dfs_nol_delchan(pst_mac_device, pst_nol_node);
}


OAL_STATIC oal_uint32  hmac_dfs_nol_addchan(mac_device_stru *pst_mac_device, oal_uint8 uc_chan_idx)
{
    mac_dfs_nol_node_stru   *pst_nol_node;
    oal_uint8                uc_chan_num = 0;
    oal_uint32               ul_ret;

    /*如果不可占用周期为0，则不添加新的nol信道*/
    if(0 == pst_mac_device->st_dfs.st_dfs_info.ul_dfs_non_occupancy_period_time_ms)
    {
        return OAL_SUCC;
    }

    pst_nol_node = (mac_dfs_nol_node_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_dfs_nol_node_stru), OAL_TRUE);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_nol_node))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{hmac_dfs_nol_addchan::memory not enough.}");

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    OAL_MEMZERO(pst_nol_node, OAL_SIZEOF(mac_dfs_nol_node_stru));

    pst_nol_node->uc_chan_idx  = uc_chan_idx;
    pst_nol_node->uc_device_id = pst_mac_device->uc_device_id;

    oal_dlist_add_tail(&(pst_nol_node->st_entry), &(pst_mac_device->st_dfs.st_dfs_nol));

    mac_get_channel_num_from_idx_etc(pst_mac_device->en_max_band, uc_chan_idx, &uc_chan_num);
    OAM_WARNING_LOG1(0, OAM_SF_DFS, "{[DFS]hmac_dfs_nol_addchan, add channel %d to NOL.}", uc_chan_num);

    /* 更新可用信道列列表 */
    ul_ret = hmac_dfs_update_available_channel_list(pst_mac_device, uc_chan_idx, OAL_TRUE);
    if(OAL_SUCC == ul_ret)
    {
        /* 启动Non-Occupancy Peroid定时器 */
        FRW_TIMER_CREATE_TIMER(&pst_nol_node->st_dfs_nol_timer,
                                hmac_dfs_nol_timeout_fn,
                                pst_mac_device->st_dfs.st_dfs_info.ul_dfs_non_occupancy_period_time_ms,
                                pst_nol_node,
                                OAL_FALSE,
                                OAM_MODULE_ID_HMAC,
                                pst_mac_device->ul_core_id);
    }

    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_dfs_nol_delchan(mac_device_stru *pst_mac_device, mac_dfs_nol_node_stru *pst_nol_node)
{
    /* 更新可用信道列列表 */
    pst_mac_device->st_ap_channel_list[pst_nol_node->uc_chan_idx].en_ch_status = MAC_CHAN_DFS_REQUIRED;

    oal_dlist_delete_entry(&pst_nol_node->st_entry);

    OAL_MEM_FREE(pst_nol_node, OAL_FALSE);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    hmac_config_ch_status_sync(pst_mac_device);
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_dfs_nol_clear(oal_dlist_head_stru *pst_dlist_head)
{
    mac_device_stru         *pst_mac_device;
    mac_dfs_nol_node_stru   *pst_nol_node;
    oal_dlist_head_stru     *pst_dlist_pos = OAL_PTR_NULL;
    oal_dlist_head_stru     *pst_dlist_temp = OAL_PTR_NULL;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dlist_head))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_nol_clear::pst_dlist_head is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_dlist_pos,pst_dlist_temp,pst_dlist_head)
    {
        pst_nol_node = OAL_DLIST_GET_ENTRY(pst_dlist_pos, mac_dfs_nol_node_stru, st_entry);
        pst_mac_device = mac_res_get_dev_etc(pst_nol_node->uc_device_id);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_nol_clear::pst_mac_device is null.}");

            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 删除相应的Non-Occupancy Peroid定时器 */
        FRW_TIMER_DESTROY_TIMER(&pst_nol_node->st_dfs_nol_timer);

        hmac_dfs_nol_delchan(pst_mac_device, pst_nol_node);
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  hmac_dfs_channel_mark_radar(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    mac_channel_list_stru    st_chan_info;
    oal_uint8                 uc_idx;
    oal_uint32                ul_cnt = 0;
    oal_uint32                ul_ret = 0;

    if(pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag == (BIT0|BIT1))
    {
        ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, pst_mac_device->st_dfs.st_dfs_info.uc_offchan_num, &uc_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                           "{hmac_dfs_channel_mark_radar::mac_get_channel_idx_from_num_etc failed = %d.}", ul_ret);
            return OAL_FAIL;
        }

        mac_get_ext_chan_info(uc_idx,
                    pst_mac_vap->st_channel.en_bandwidth, &st_chan_info);
    }
    else
    {
        mac_get_ext_chan_info(pst_mac_vap->st_channel.uc_chan_idx,
                    pst_mac_vap->st_channel.en_bandwidth, &st_chan_info);
    }

    for (uc_idx = 0; uc_idx < st_chan_info.ul_channels; uc_idx++)
    {
        //只要有一条信道标记成功，就需要切换信道
        if(OAL_SUCC == hmac_dfs_nol_addchan(pst_mac_device, st_chan_info.ast_channels[uc_idx].uc_idx))
        {
            ul_cnt++;
        }
    }

    return (oal_uint32)((ul_cnt > 0)?OAL_SUCC : OAL_FAIL);
}
#define HMAC_40MHZ_BITMAP_MASK     0x1EFFFFFF
#define HMAC_80MHZ_BITMAP_MASK     0x00FFFFFF
#define HMAC_160MHZ_BITMAP_MASK    0x00FFFF


OAL_STATIC oal_uint32 hmac_dfs_is_channel_support_bw(
                mac_vap_stru                        *pst_mac_vap,
                oal_uint8                           uc_channel,
                wlan_channel_bandwidth_enum_uint8   en_bandwidth)
{
    mac_device_stru   *pst_mac_device;
    oal_uint8          uc_num_supp_chan = mac_get_num_supp_channel(pst_mac_vap->st_channel.en_band);
    oal_uint32         ul_chan_bitmap = 0;
    oal_uint32         ul_window = 0;
    oal_uint8          uc_step = 0;
    oal_uint8          uc_idx;
    oal_uint8          uc_target_idx;
    oal_uint8          uc_offset;
    oal_uint32         ul_ret;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                       "{hmac_dfs_is_channel_support_bw::pst_mac_device null, device_id = %d.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band,uc_channel,&uc_target_idx);
    if(ul_ret != OAL_SUCC)
    {
         OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                       "{hmac_dfs_is_channel_support_bw::get channel idx failed(%d).}", pst_mac_vap->uc_device_id);
         return OAL_FAIL;
    }

hmac_dfs_is_channel_support_bw_update:

    /* 获取可用信道位图 */
    for (uc_idx = 0; uc_idx < uc_num_supp_chan; uc_idx++)
    {
        if ((MAC_CHAN_NOT_SUPPORT        != pst_mac_device->st_ap_channel_list[uc_idx].en_ch_status) &&
                (MAC_CHAN_BLOCK_DUE_TO_RADAR != pst_mac_device->st_ap_channel_list[uc_idx].en_ch_status))
        {
            if(pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap)
            {
                if(pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap & (0x01 << uc_idx))
                {
                    ul_chan_bitmap |= (0x1 << uc_idx);
                }
            }
            else
            {
                ul_chan_bitmap |= (0x1 << uc_idx);
            }
        }
    }

    if(!ul_chan_bitmap)
    {
        //此策略经产品讨论，当因为雷达检测导致已经无可用信道可跳时，将
        //原来被标记为雷达的信道全部清掉，重新开始选择，然后CAC
        if(hmac_dfs_nol_clear(&pst_mac_device->st_dfs.st_dfs_nol) != OAL_SUCC)
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                                 "{hmac_dfs_is_channel_support_bw::hmac_dfs_nol_clear() Fail!}");

            return OAL_FAIL;
        }

        /*lint -e801*/
        goto hmac_dfs_is_channel_support_bw_update;
        /*lint +e801*/
    }

    /* 20MHz */
    if (WLAN_BAND_WIDTH_20M == en_bandwidth)
    {
        ul_window = 0x1;   /* 1b */
        uc_step = 1;

        for (uc_idx = 0; uc_idx < uc_num_supp_chan; uc_idx += uc_step)
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                if(uc_idx == uc_target_idx)
                {
                    return OAL_SUCC;
                }
            }
        }
    }
    /* 40MHz */
    else if (en_bandwidth < WLAN_BAND_WIDTH_80PLUSPLUS)
    {
        ul_window = 0x3;   /* 11b */
        uc_step = 2;
        ul_chan_bitmap &= HMAC_40MHZ_BITMAP_MASK;
        uc_offset = (en_bandwidth == WLAN_BAND_WIDTH_40PLUS)?0:1;

        /* channel 36 ~ 144 */
        for (uc_idx = 0; uc_idx < MAC_CHANNEL149; uc_idx += uc_step)                /* skip channel 140 */
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                if((uc_idx+uc_offset) == uc_target_idx)
                {
                    return OAL_SUCC;
                }
            }
        }

        /* channel 149 ~ 161 */
        for (uc_idx = MAC_CHANNEL149; uc_idx < MAC_CHANNEL165; uc_idx += uc_step)   /* skip channel 165 */
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                if((uc_idx+uc_offset) == uc_target_idx)
                {
                    return OAL_SUCC;
                }
            }
        }

        /* channel 184 ~ 196 */
        for (uc_idx = MAC_CHANNEL184; uc_idx < uc_num_supp_chan; uc_idx += uc_step)
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                if((uc_idx+uc_offset) == uc_target_idx)
                {
                    return OAL_SUCC;
                }
            }
        }
    }
    /* 80MHz */
    else if(en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)
    {
        ul_window = 0xF    /* 1111b */;
        uc_step = 4;
        ul_chan_bitmap &= HMAC_80MHZ_BITMAP_MASK;
        switch (en_bandwidth)
        {
            case WLAN_BAND_WIDTH_80PLUSMINUS:
                uc_offset = 2;
                break;
            case WLAN_BAND_WIDTH_80MINUSPLUS:
                uc_offset = 1;
                break;
            case WLAN_BAND_WIDTH_80MINUSMINUS:
                uc_offset = 3;
                break;
            default:
                uc_offset = 0;
        }

        /* channel 36 ~ 161 */
        for (uc_idx = 0; uc_idx < MAC_CHANNEL165; uc_idx += uc_step)                /* JP channel 184, 188, 192, 196, */
        {                                                                           /* 4条信道组不了80MHz, 参考802.11 ac Annex E */
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                if((uc_idx+uc_offset) == uc_target_idx)
                {
                    return OAL_SUCC;
                }
            }
        }
    }
#ifdef _PRE_WLAN_FEATURE_160M
    /* 160MHz */
    else
    {
        ul_window = 0xff;
        uc_step = 8;
        ul_chan_bitmap &= HMAC_160MHZ_BITMAP_MASK;
        switch (en_bandwidth)
        {
            case WLAN_BAND_WIDTH_160PLUSPLUSMINUS:
                uc_offset = 4;
                break;
            case WLAN_BAND_WIDTH_160PLUSMINUSPLUS:
                uc_offset = 2;
                break;
            case WLAN_BAND_WIDTH_160PLUSMINUSMINUS:
                uc_offset = 6;
                break;
            case WLAN_BAND_WIDTH_160MINUSPLUSPLUS:
                uc_offset = 1;
                break;
            case WLAN_BAND_WIDTH_160MINUSPLUSMINUS:
                uc_offset = 5;
                break;
            case WLAN_BAND_WIDTH_160MINUSMINUSPLUS:
                uc_offset = 3;
                break;
            case WLAN_BAND_WIDTH_160MINUSMINUSMINUS:
                uc_offset = 7;
                break;
            default:
                uc_offset = 0;
        }

        /* channel 36 ~ 161 */
        for (uc_idx = 0; uc_idx < MAC_CHANNEL165; uc_idx += uc_step)
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                if((uc_idx+uc_offset) == uc_target_idx)
                {
                    return OAL_SUCC;
                }
            }
        }
    }
#endif

    return OAL_FAIL;
}


OAL_STATIC oal_uint32 hmac_dfs_select_random_channel(
                mac_vap_stru                        *pst_mac_vap,
                oal_uint8                           *puc_new_channel,
                wlan_channel_bandwidth_enum_uint8   *pen_new_bandwidth)
{
    mac_device_stru   *pst_mac_device;
    oal_uint8          uc_num_supp_chan = mac_get_num_supp_channel(pst_mac_vap->st_channel.en_band);
    oal_uint32         ul_chan_bitmap = 0;
    oal_uint32         ul_window = 0;
    oal_uint8          uc_step = 0;
    oal_uint8          auc_available_chan_idx[MAC_CHANNEL_FREQ_5_BUTT];
    oal_uint8          uc_available_chan_cnt = 0;
    oal_uint8          uc_idx;
    oal_uint8          uc_cur_idx;
    oal_uint8          uc_sub;
    oal_uint8          uc_cur_sub;
    wlan_channel_bandwidth_enum_uint8   en_try_bandwidth;

    OAL_MEMZERO(auc_available_chan_idx, OAL_SIZEOF(auc_available_chan_idx));

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                       "{hmac_dfs_select_random_channel::pst_mac_device null, device_id = %d.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap)
    {
        en_try_bandwidth = WLAN_BAND_WIDTH_20M; //强制20M带宽
    }
    else
    {
        en_try_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
    }

hmac_dfs_select_random_channel_update:
    /* 获取可用信道位图 */
    for (uc_idx = 0; uc_idx < uc_num_supp_chan; uc_idx++)
    {
        if ((MAC_CHAN_NOT_SUPPORT        != pst_mac_device->st_ap_channel_list[uc_idx].en_ch_status) &&
            (MAC_CHAN_BLOCK_DUE_TO_RADAR != pst_mac_device->st_ap_channel_list[uc_idx].en_ch_status))
        {
            if(pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap)
            {
                if(pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap & (0x01 << uc_idx))
                {
                    ul_chan_bitmap |= (0x1 << uc_idx);
                }
            }
            else
            {
                ul_chan_bitmap |= (0x1 << uc_idx);
            }
        }
    }

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                     "{hmac_dfs_select_random_channel::custom_chanlist_bitmap=0x%x,chan_bitmap=0x%x!}",
                      pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap,
                      ul_chan_bitmap);

    /* 20MHz */
    if (WLAN_BAND_WIDTH_20M == en_try_bandwidth)
    {
        ul_window = 0x1;   /* 1b */
        uc_step = 1;

        for (uc_idx = 0; uc_idx < uc_num_supp_chan; uc_idx += uc_step)
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                auc_available_chan_idx[uc_available_chan_cnt++] = uc_idx;
            }
        }
    }
    /* 40MHz */
    else if (en_try_bandwidth < WLAN_BAND_WIDTH_80PLUSPLUS)
    {
        ul_window = 0x3;   /* 11b */
        uc_step = 2;
        ul_chan_bitmap &= HMAC_40MHZ_BITMAP_MASK;

        /* channel 36 ~ 144 */
        for (uc_idx = 0; uc_idx < MAC_CHANNEL149; uc_idx += uc_step)                /* skip channel 140 */
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                auc_available_chan_idx[uc_available_chan_cnt++] = uc_idx;
            }
        }

        /* channel 149 ~ 161 */
        for (uc_idx = MAC_CHANNEL149; uc_idx < MAC_CHANNEL165; uc_idx += uc_step)   /* skip channel 165 */
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                auc_available_chan_idx[uc_available_chan_cnt++] = uc_idx;
            }
        }

        /* channel 184 ~ 196 */
        for (uc_idx = MAC_CHANNEL184; uc_idx < uc_num_supp_chan; uc_idx += uc_step)
        {
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                auc_available_chan_idx[uc_available_chan_cnt++] = uc_idx;
            }
        }
    }
    /* 80MHz */
    else if(en_try_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS)
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
        /*80MHz上检测到有问题,固定选择36-64的信道,带宽80M*/
        auc_available_chan_idx[uc_available_chan_cnt++] = MAC_CHANNEL36;
#else
        ul_window = 0xF    /* 1111b */;
        uc_step = 4;
        ul_chan_bitmap &= HMAC_80MHZ_BITMAP_MASK;

        /* channel 36 ~ 161 */
        for (uc_idx = 0; uc_idx < MAC_CHANNEL165; uc_idx += uc_step)                /* JP channel 184, 188, 192, 196, */
        {                                                                           /* 4条信道组不了80MHz, 参考802.11 ac Annex E */
            if ((ul_window << uc_idx) == (ul_chan_bitmap & (ul_window << uc_idx)))
            {
                auc_available_chan_idx[uc_available_chan_cnt++] = uc_idx;
            }
        }
#endif
    }
    /* 160MHz */
    else
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
        /*160MHz上检测到有问题,固定选择主信道36,带宽80M*/
        auc_available_chan_idx[uc_available_chan_cnt++] = MAC_CHANNEL36;
        en_try_bandwidth = WLAN_BAND_WIDTH_80PLUSPLUS;
#endif
    }

    /* 如果找不到可用信道，从管制域支持的信道中，选择最低可用信道，带宽20MHz */
    if (0 == uc_available_chan_cnt)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                     "{hmac_dfs_select_random_channel::No availabe channel,NOL will be cleared!}");
        //此策略经产品讨论，当因为雷达检测导致已经无可用信道可跳时，将
        //原来被标记为雷达的信道全部清掉，重新开始选择，然后CAC
        if(hmac_dfs_nol_clear(&pst_mac_device->st_dfs.st_dfs_nol) != OAL_SUCC)
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                                 "{hmac_dfs_select_random_channel::hmac_dfs_nol_clear() Fail!}");

            return OAL_FAIL;
        }
        en_try_bandwidth = WLAN_BAND_WIDTH_20M;
        /*lint -e801*/
        goto hmac_dfs_select_random_channel_update; //需要重新获取信道位图
        /*lint +e801*/
    }

    uc_sub = 0;
    for (ul_window = 0; ul_window < uc_available_chan_cnt; ul_window++)
    {
        uc_cur_idx = auc_available_chan_idx[ul_window];
        switch (en_try_bandwidth)
        {
            case WLAN_BAND_WIDTH_40MINUS:
            case WLAN_BAND_WIDTH_80MINUSPLUS:
                uc_cur_idx += 1;
                break;

            case WLAN_BAND_WIDTH_80PLUSMINUS:
                uc_cur_idx += 2;
                break;

            case WLAN_BAND_WIDTH_80MINUSMINUS:
                uc_cur_idx += 3;
                break;
            default:
                break;
        }

        mac_get_channel_num_from_idx_etc(pst_mac_vap->st_channel.en_band, uc_cur_idx, puc_new_channel);
        uc_cur_sub = OAL_ABSOLUTE_SUB(pst_mac_vap->st_channel.uc_chan_number, *puc_new_channel);

        if(uc_cur_sub > uc_sub)
        {
            uc_sub = uc_cur_sub;
            uc_idx = uc_cur_idx;
        }
    }

    mac_get_channel_num_from_idx_etc(pst_mac_vap->st_channel.en_band, uc_idx, puc_new_channel);

    *pen_new_bandwidth = en_try_bandwidth;

    OAL_IO_PRINT("[DFS]select_channel::bandwidth = %d, channelnum = %d.\n", *pen_new_bandwidth, *puc_new_channel);
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN,
                 "{hmac_dfs_select_random_channel::[DFS]select_channel::bandwidth = %d, channelnum = %d!}", *pen_new_bandwidth, *puc_new_channel);

    return OAL_SUCC;
}


oal_uint32  hmac_dfs_ap_wait_start_radar_handler_etc(hmac_vap_stru *pst_hmac_vap)
{
    mac_device_stru                     *pst_mac_device;
    mac_vap_stru                        *pst_mac_vap;
    oal_uint8                            uc_new_channel   = 0;
    wlan_channel_bandwidth_enum_uint8    en_new_bandwidth = WLAN_BAND_WIDTH_BUTT;
    oal_uint32                           ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{hmac_dfs_ap_wait_start_radar_handler_etc::pst_hmac_vap is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                       "{hmac_dfs_ap_wait_start_radar_handler_etc::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 关闭 CAC 检测 */
    hmac_dfs_cac_stop_etc(pst_mac_device,pst_mac_vap);

    /* 标记主、次信道检测到雷达 */
    if(hmac_dfs_channel_mark_radar(pst_mac_device, pst_mac_vap) != OAL_SUCC)
    {
        return OAL_FAIL;
    }

    /* 重新选择一条信道 */
    if(pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum)
    {
        uc_new_channel = pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum;
        en_new_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                         "{hmac_dfs_ap_wait_start_radar_handler_etc::next ch(%d) and bw(%d) set by user.}",
                         uc_new_channel,en_new_bandwidth);

        if(hmac_dfs_is_channel_support_bw(pst_mac_vap, uc_new_channel, en_new_bandwidth) != OAL_SUCC)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                    "{hmac_dfs_switch_channel_for_radar_etc::new channel(%d) not support current bw mode(%d).}",uc_new_channel,en_new_bandwidth);
            //如果应用设定的下一跳信道不支持当前的带宽模式，那么按照驱动的随机策略选择
            ul_ret = hmac_dfs_select_random_channel(pst_mac_vap, &uc_new_channel, &en_new_bandwidth);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                         "{hmac_dfs_ap_wait_start_radar_handler_etc::hmac_dfs_select_random_channel failed.}");
                return ul_ret;
            }
        }
        pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum = 0;
        pst_mac_device->st_dfs.st_dfs_info.en_next_ch_width_type = WLAN_BAND_WIDTH_BUTT;
    }
    else
    {
        ul_ret = hmac_dfs_select_random_channel(pst_mac_vap, &uc_new_channel, &en_new_bandwidth);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                         "{hmac_dfs_ap_wait_start_radar_handler_etc::hmac_dfs_select_random_channel failed.}");
            return ul_ret;
        }
    }

    ul_ret = mac_is_channel_num_valid_etc(pst_mac_vap->st_channel.en_band, uc_new_channel);
    if ((OAL_SUCC != ul_ret) || (en_new_bandwidth >= WLAN_BAND_WIDTH_BUTT))
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                         "{hmac_dfs_ap_wait_start_radar_handler_etc::Could not start network using the selected channel[%d] or bandwidth[%d].}",
                         uc_new_channel, en_new_bandwidth);
        return ul_ret;
    }

    /* mayuan TBD 只需要设置硬件寄存器一次，然后同步一下软件vap的配置即可 */
    hmac_chan_multi_select_channel_mac_etc(pst_mac_vap, uc_new_channel, en_new_bandwidth);

    //hmac_chan_select_channel_mac(&(pst_hmac_vap->st_vap_base_info), uc_new_channel, en_new_bandwidth);

    /* 判断是否需要进行 */
    if (OAL_TRUE == hmac_dfs_need_for_cac(pst_mac_device, &pst_hmac_vap->st_vap_base_info))
    {
        hmac_dfs_cac_start_etc(pst_mac_device, pst_hmac_vap);

        return OAL_SUCC;
    }
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    /*切换到非雷达信道,把vap状态置为up,恢复发送队列*/
    hmac_cac_chan_ctrl_machw_tx(pst_mac_vap, OAL_TRUE);
#endif
    /* 否则，直接启动BSS */
    return hmac_start_bss_in_available_channel_etc(pst_hmac_vap);
}

oal_uint32 hmac_dfs_switch_channel_for_radar_etc(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap)
{
    oal_uint8                            uc_new_channel   = 0;
    wlan_channel_bandwidth_enum_uint8    en_new_bandwidth = WLAN_BAND_WIDTH_BUTT;
    oal_uint32                           ul_ret;

    /* 关闭 CAC 检测 */
    hmac_dfs_cac_stop_etc(pst_mac_device,pst_mac_vap);

    /* 标记主、次信道检测到雷达 */
    if(hmac_dfs_channel_mark_radar(pst_mac_device, pst_mac_vap) != OAL_SUCC)
    {
        return OAL_FAIL;
    }

    /* 如果AP已经准备进行信道切换，则直接返回，不做任何处理 */
    if (WLAN_CH_SWITCH_STATUS_1 == pst_mac_vap->st_ch_switch_info.en_ch_switch_status)
    {
        return OAL_SUCC;
    }

    if(pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum)
    {
        uc_new_channel = pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum;
        if(WLAN_BAND_WIDTH_BUTT != pst_mac_device->st_dfs.st_dfs_info.en_next_ch_width_type)
        {
            en_new_bandwidth = pst_mac_device->st_dfs.st_dfs_info.en_next_ch_width_type;
        }
        else
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                         "{hmac_dfs_switch_channel_for_radar_etc::user not set next ch bw.}");
            en_new_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
        }

        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN,
                         "{hmac_dfs_switch_channel_for_radar_etc::next ch(%d) and bw(%d) set by user.}",
                         uc_new_channel,en_new_bandwidth);

        if(hmac_dfs_is_channel_support_bw(pst_mac_vap, uc_new_channel, en_new_bandwidth) != OAL_SUCC)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                    "{hmac_dfs_switch_channel_for_radar_etc::new channel(%d) not support current bw mode(%d).}",uc_new_channel,en_new_bandwidth);
            //如果应用设定的下一跳信道不支持当前的带宽模式，那么按照驱动的随机策略选择
            ul_ret = hmac_dfs_select_random_channel(pst_mac_vap, &uc_new_channel, &en_new_bandwidth);
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                        "{hmac_dfs_switch_channel_for_radar_etc::hmac_dfs_select_random_channel failed. }");
                return ul_ret;
            }
        }
        pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum = 0;
        pst_mac_device->st_dfs.st_dfs_info.en_next_ch_width_type = WLAN_BAND_WIDTH_BUTT;
    }
    else
    {
        ul_ret = hmac_dfs_select_random_channel(pst_mac_vap, &uc_new_channel, &en_new_bandwidth);
        if (OAL_SUCC != ul_ret)
        {
             OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                     "{hmac_dfs_switch_channel_for_radar_etc::hmac_dfs_select_random_channel failed.}");
            return ul_ret;
        }
    }

    pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt = WLAN_CHAN_SWITCH_DETECT_RADAR_CNT;
    hmac_chan_multi_switch_to_new_channel_etc(pst_mac_vap, uc_new_channel, en_new_bandwidth);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_dfs_off_chan_cac_timeout_fn(oal_void *p_arg)
{
    mac_device_stru     *pst_mac_device;
    hmac_vap_stru       *pst_hmac_vap;
    mac_dfs_info_stru   *pst_dfs_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_arg))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_off_chan_cac_timeout_fn::p_arg is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_off_chan_cac_timeout_fn::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dfs_info = &pst_mac_device->st_dfs.st_dfs_info;

    pst_dfs_info->uc_timer_cnt++;

    if(pst_dfs_info->uc_timer_cnt != pst_dfs_info->uc_timer_end_cnt)
    {
        /* 启动OFF CHAN CAC 定时器 */
        FRW_TIMER_CREATE_TIMER(&pst_mac_device->st_dfs.st_dfs_off_chan_cac_timer,
                               hmac_dfs_off_chan_cac_timeout_fn,
                               HMAC_DFS_ONE_MIN_IN_MS,
                               pst_hmac_vap,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
        return OAL_SUCC;
    }


    OAL_IO_PRINT("[DFS]hmac_dfs_off_chan_cac_timeout_fn::off-channel cac end.\n");

    /* 关闭Off-channel CAC 信道驻留定时器 */
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_mac_device->st_dfs.st_dfs_chan_dwell_timer));
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC
    /* 若VAP在Off-Channel信道上则切回工作信道 */
    if (pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag & BIT0)
    {
        hmac_scan_switch_channel_back(&(pst_hmac_vap->st_vap_base_info));
    }
#endif
    pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag = 0;

    return OAL_SUCC;
}



oal_uint32  hmac_dfs_ap_up_radar_handler_etc(hmac_vap_stru *pst_hmac_vap)
{
    mac_device_stru                     *pst_mac_device;
    mac_vap_stru                        *pst_mac_vap;
    oal_uint8                            uc_offchan_flag;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{hmac_dfs_ap_wait_start_radar_handler_etc::pst_hmac_vap is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                       "{hmac_dfs_ap_up_radar_handler_etc::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
    uc_offchan_flag = pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag;
    /*off-chan cac 期间*/
    if(uc_offchan_flag & BIT1)
    {
        /* home channel检测到radar */
        if(!(uc_offchan_flag & BIT0))
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{[DFS]radar detected in channel %d.}", pst_mac_vap->st_channel.uc_chan_number);

            /* 关闭Off-channel CAC 定时器 */
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_mac_device->st_dfs.st_dfs_off_chan_cac_timer));

            hmac_dfs_off_chan_cac_timeout_fn(pst_hmac_vap);

            return hmac_dfs_switch_channel_for_radar_etc(pst_mac_device, pst_mac_vap);
        }
        /* off channel检测到radar */
        else
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{[DFS]radar detected in channel %d.}", pst_mac_device->st_dfs.st_dfs_info.uc_offchan_num);
            /* 标记主、次信道检测到雷达 */
            hmac_dfs_channel_mark_radar(pst_mac_device, pst_mac_vap);
            return OAL_SUCC;
        }
    }
    /* in service monitor期间 */
    else
    {
        return hmac_dfs_switch_channel_for_radar_etc(pst_mac_device, pst_mac_vap);
    }

}
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC

OAL_STATIC oal_uint32  hmac_scan_switch_channel_off(mac_vap_stru *pst_mac_vap)
{
    frw_event_mem_stru   *pst_event_mem;
    frw_event_stru       *pst_event;
    oal_uint32            ul_ret;

    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_scan_switch_channel_off::pst_event_mem null.}");

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_OFF_CHAN,
                    0,
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                       "{hmac_scan_switch_channel_off::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);

        return ul_ret;
    }

    /* 释放事件 */
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_scan_switch_channel_back(mac_vap_stru *pst_mac_vap)
{
    frw_event_mem_stru   *pst_event_mem;
    frw_event_stru       *pst_event;
    oal_uint32            ul_ret;

    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_scan_switch_channel_back::pst_event_mem null.}");

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    DMAC_WLAN_CTX_EVENT_SUB_TYPR_SWITCH_TO_HOME_CHAN,
                    0,
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS,
                       "{hmac_scan_switch_channel_back::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);

        return ul_ret;
    }

    /* 释放事件 */
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}
#endif


OAL_STATIC oal_uint32  hmac_dfs_off_chan_cac_opern_ch_dwell_timeout(oal_void *p_arg)
{
    mac_device_stru   *pst_mac_device;
    hmac_vap_stru     *pst_hmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_arg))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_off_chan_cac_opern_ch_dwell_timeout::p_arg is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_off_chan_cac_opern_ch_dwell_timeout::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

#if 0
    pst_mac_device->st_scan_params.en_scan_mode = WLAN_SCAN_MODE_BACKGROUND_AP;
    pst_mac_device->st_scan_params.us_scan_time = pst_mac_device->st_dfs.st_dfs_info.us_dfs_off_chan_cac_off_chan_dwell_time;
    pst_mac_device->uc_scan_chan_idx = 0;
    /* pst_mac_device->st_scan_params.ast_channel_list[pst_mac_device->uc_scan_chan_idx] = st_channel; mayuan TBD */
#endif

    /* 当前信道为offchan 标志 */
    pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag = (BIT0|BIT1);
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC
    /* 切换到Off-channel CAC检测信道 */
    hmac_scan_switch_channel_off(&pst_hmac_vap->st_vap_base_info);
#endif
    /* 将当前off-channel cac信道信息赋值给VAP */
    /* pst_hmac_vap->st_vap_base_info.st_channel = st_channel; mayuan TBD*/

    /* 启动Off-channel信道上检测时长定时器 */
    FRW_TIMER_CREATE_TIMER(&pst_mac_device->st_dfs.st_dfs_chan_dwell_timer,
                          hmac_dfs_off_chan_cac_off_ch_dwell_timeout,
                          pst_mac_device->st_dfs.st_dfs_info.us_dfs_off_chan_cac_off_chan_dwell_time,
                          pst_hmac_vap,
                          OAL_FALSE,
                          OAM_MODULE_ID_HMAC,
                          pst_hmac_vap->st_vap_base_info.ul_core_id);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_dfs_off_chan_cac_off_ch_dwell_timeout(oal_void *p_arg)
{
    mac_device_stru   *pst_mac_device;
    hmac_vap_stru     *pst_hmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_arg))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_off_chan_cac_off_ch_dwell_timeout::p_arg is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_DFS, "{hmac_dfs_off_chan_cac_off_ch_dwell_timeout::pst_mac_device is null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 当前信道为homechan标志 */
    pst_mac_device->st_dfs.st_dfs_info.uc_offchan_flag = BIT1;
#ifdef _PRE_WLAN_FEATURE_OFFCHAN_CAC
    /* 切换回工作信道 */
    hmac_scan_switch_channel_back(&(pst_hmac_vap->st_vap_base_info));
#endif
    /* 启动在工作信道上的工作时长定时器 */
    FRW_TIMER_CREATE_TIMER(&pst_mac_device->st_dfs.st_dfs_chan_dwell_timer,
                           hmac_dfs_off_chan_cac_opern_ch_dwell_timeout,
                           pst_mac_device->st_dfs.st_dfs_info.us_dfs_off_chan_cac_opern_chan_dwell_time,
                           pst_hmac_vap,
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_hmac_vap->st_vap_base_info.ul_core_id);

    return OAL_SUCC;
}


oal_void  hmac_dfs_off_chan_cac_start_etc(mac_device_stru *pst_mac_device, hmac_vap_stru *pst_hmac_vap)
{
    oal_uint32 ul_scan_time;

    /* 如果已经启动OFF-CHAN-CAC 定时器, 直接返回 */
    if(OAL_TRUE == pst_mac_device->st_dfs.st_dfs_off_chan_cac_timer.en_is_enabled)
    {
        OAM_INFO_LOG0(0, OAM_SF_DFS, "{hmac_dfs_off_chan_cac_start_etc::off-chan-cac is already started by another vap.}");
        return;
    }

    /* 设置 Off-Channel CAC 检测时间 */
    ul_scan_time = hmac_chan_get_off_chan_cac_time(pst_mac_device, &pst_hmac_vap->st_vap_base_info);

    pst_mac_device->st_dfs.st_dfs_info.uc_timer_cnt     = 0;
    pst_mac_device->st_dfs.st_dfs_info.uc_timer_end_cnt = (oal_uint8)(ul_scan_time / HMAC_DFS_ONE_MIN_IN_MS);

    /* 启动 CAC 定时器 */
    FRW_TIMER_CREATE_TIMER(&pst_mac_device->st_dfs.st_dfs_off_chan_cac_timer,
                           hmac_dfs_off_chan_cac_timeout_fn,
                           HMAC_DFS_ONE_MIN_IN_MS,
                           pst_hmac_vap,
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_hmac_vap->st_vap_base_info.ul_core_id);

    OAM_INFO_LOG2(0, OAM_SF_DFS, "{[DFS]hmac_dfs_off_chan_cac_start_etc, OFF-CHAN-CAC start, channel number:%d, scan time = %d.}",
                 pst_mac_device->st_dfs.st_dfs_info.uc_offchan_num, ul_scan_time);


    hmac_dfs_off_chan_cac_opern_ch_dwell_timeout(pst_hmac_vap);
}

oal_bool_enum_uint8 hmac_dfs_try_cac_etc(hmac_device_stru *pst_hmac_device, mac_vap_stru *pst_mac_vap)
{
    hmac_vap_stru    *pst_hmac_vap;
    mac_device_stru  *pst_mac_device;
    mac_channel_stru *pst_channel;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_device || OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_dfs_try_cac_etc::pst_device_base_info null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_hmac_device->pst_device_base_info;



    /* 如果已经启动 CAC 定时器，则直接返回 */
    if (OAL_TRUE == pst_mac_device->st_dfs.st_dfs_cac_timer.en_is_enabled)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ACS, "cac started\n");
        return OAL_TRUE;
    }
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id,OAM_SF_ANY,"{hmac_dfs_try_cac_etc::mac_res_get_hmac_vap fail.vap_id[%u]}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    // 至此，vap的信道结构为最终的信道,但是可能并未设置到硬件
    pst_channel = &pst_hmac_vap->st_vap_base_info.st_channel;
    if (IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info)))
    {
        /* 如果需要进行 CAC 检测，这里启动定时器，直到CAC结束后才启动BSS(VAP UP) */
        if (OAL_TRUE == hmac_dfs_need_for_cac(pst_mac_device, &pst_hmac_vap->st_vap_base_info))
        {
            // 此时设置硬件信道
            hmac_chan_sync_etc(&pst_hmac_vap->st_vap_base_info, pst_channel->uc_chan_number,
                            pst_channel->en_bandwidth, OAL_TRUE);
        #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
            /*把vap状态置为pause, 抛事件给dmac关发送队列, 待cac超时后把vap状态置为up,恢复发送队列*/
            hmac_cac_chan_ctrl_machw_tx(pst_mac_vap, OAL_FALSE);
        #endif
            hmac_dfs_cac_start_etc(pst_mac_device, pst_hmac_vap);
            pst_hmac_device->en_init_scan    = OAL_FALSE; // 进入CAC，认为初始扫描完成。
            pst_hmac_device->en_in_init_scan = OAL_FALSE;
            pst_mac_device->st_dfs.st_dfs_info.en_dfs_switch &= ~BIT1;

            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}

oal_uint32  hmac_dfs_init_scan_hook_etc(hmac_scan_record_stru   *pst_scan_record,
                                    hmac_device_stru        *pst_hmac_device)
{
    oal_uint8       uc_vap_id;
    mac_vap_stru    *pst_vap = NULL;
    mac_device_stru *pst_mac_device = NULL;

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "dfs init scan hook run\n");

    pst_mac_device = pst_hmac_device->pst_device_base_info;

    for (uc_vap_id=0; uc_vap_id < pst_mac_device->uc_vap_num; uc_vap_id++)
    {
        pst_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_id]);
        if (pst_vap && WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode)
        {
            /* 只要当前device上已经开始进行CAC过程，那么同device的其他VAP不再进行 */
            /* CAC完成之后会统一设置信道并启动VAP */
            /* DBAC场景下依然可以满足 */
            if (OAL_TRUE == hmac_dfs_try_cac_etc(pst_hmac_device, pst_vap))
            {
                OAM_WARNING_LOG0(0, OAM_SF_ACS, "cac started\n");
                return OAL_SUCC;
            }
        }
    }

    OAM_WARNING_LOG0(0, OAM_SF_ACS, "cac not need\n");
    return OAL_FAIL;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_void test_dfs(oal_uint8 uc_vap_id)
{
    frw_event_mem_stru  *pst_event_mem;
    frw_event_stru      *pst_event;
    mac_vap_stru        *pst_mac_vap;


    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{test_dfs::pst_mac_vap null.}");
        return;
    }
    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{test_dfs::pst_event_mem null.}");
        return;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPR_DFS_TEST,
                       0,
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);
}
oal_void test_csa(oal_uint8 uc_vap_id, oal_uint8 uc_chan_id, oal_uint8 uc_sw_cnt)
{
    frw_event_mem_stru       *pst_event_mem;
    frw_event_stru           *pst_event;
    mac_vap_stru             *pst_mac_vap;
    oal_netbuf_stru          *pst_netbuf;
    oal_uint8                *puc_data;
    mac_rx_ctl_stru         *pst_rx_ctrl;
    hcc_event_stru           *pst_hcc_event_payload;
    oal_uint32                ul_ret = OAL_SUCC;


    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DFS, "{test_csa::pst_mac_vap null.}");
        return;
    }
    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{test_csa::pst_event_mem null.}");
        return;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CRX,
                       DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX,
                       0,
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{test_csa::pst_netbuf null.}");
        return;
    }

    pst_rx_ctrl      = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl) = (oal_uint32 *)oal_netbuf_data(pst_netbuf);
    pst_rx_ctrl->uc_mac_header_len      = 24;
    pst_rx_ctrl->us_frame_len           = 31;

    puc_data = oal_netbuf_data(pst_netbuf);
    puc_data[0]  = 0xd0;
    puc_data[24] = 0;
    puc_data[25] = 4;
    puc_data[26] = 37;
    puc_data[27] = 3;
    puc_data[28] = 1;
    puc_data[29] = uc_chan_id;
    puc_data[30] = uc_sw_cnt;
    oal_netbuf_put(pst_netbuf, 31);


    pst_hcc_event_payload = (hcc_event_stru *)pst_event->auc_event_data;
    pst_hcc_event_payload->en_nest_type     = FRW_EVENT_TYPE_WLAN_CRX;
    pst_hcc_event_payload->uc_nest_sub_type = DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX;
    pst_hcc_event_payload->pst_netbuf       = pst_netbuf;

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        /* 将netbuf归还内存池 */
        oal_netbuf_free(pst_netbuf);
    }

    FRW_EVENT_FREE(pst_event_mem);
}
#endif

#endif   /* end of _PRE_WLAN_FEATURE_DFS */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

