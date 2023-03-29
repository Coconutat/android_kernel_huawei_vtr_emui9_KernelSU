



#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_fbt_main.h"
#include "hmac_scan.h"
#include "oal_cfg80211.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_FBT_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/



/*****************************************************************************
  3 函数实现
*****************************************************************************/
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)


oal_void hmac_fbt_result_report( mac_fbt_scan_mgmt_stru *pst_fbt_scan_mgmt)
{
    oal_uint8                   uc_user_idx;
    hmac_fbt_notify_stru        st_fbt_notify;
    hmac_fbt_notify_stru       *pst_fbt_notify;
    mac_fbt_scan_result_stru   *pst_fbt_scan_result;

    pst_fbt_notify = &st_fbt_notify;

    for (uc_user_idx = 0; uc_user_idx < HMAC_FBT_MAX_USER_NUM; uc_user_idx++)   // HMAC_FBT_MAX_USER_NUM 3 for test
    {
        /* 找到FBT scan侦听管理实体，初始化FBT scan侦听结果索引i=0 */
        pst_fbt_scan_result = &(pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx]);

        if (OAL_TRUE == pst_fbt_scan_result->uc_is_found)
        {
            oal_memcopy(pst_fbt_notify->auc_user_bssid, pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_bssid, WLAN_MAC_ADDR_LEN);
            pst_fbt_notify->uc_user_channel = pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].uc_user_channel;
            pst_fbt_notify->uc_user_rssi = (oal_uint8)(oal_get_real_rssi(pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].s_rssi) + HMAC_FBT_RSSI_ADJUST_VALUE);
            pst_fbt_notify->en_fbt_notify_type = HMAC_FBT_STA_FOUND;
            oal_memcopy(pst_fbt_notify->auc_user_mac_addr, pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_mac_addr, WLAN_MAC_ADDR_LEN);
            pst_fbt_notify->ul_rssi_timestamp = pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].ul_rssi_timestamp;
            oal_unconnect_sta_report((void *)pst_fbt_notify);
            OAM_WARNING_LOG4(0, OAM_SF_HILINK, "{hmac_fbt_result_report::STA:[xx:%0x2:%02x] assoc to AP:[xx:%0x2:%02x]}",
                                st_fbt_notify.auc_user_mac_addr[4],st_fbt_notify.auc_user_mac_addr[5],
                                st_fbt_notify.auc_user_bssid[4],st_fbt_notify.auc_user_bssid[5]);
            OAM_WARNING_LOG2(0, OAM_SF_HILINK, "hmac_fbt_result_report::rssi=%d channel=%d\n",
                                st_fbt_notify.uc_user_rssi,st_fbt_notify.uc_user_channel);
        }
     }
}
#endif


oal_uint32  hmac_config_fbt_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    mac_fbt_mgmt_stru       *pst_hmac_fbt_mgmt;
    hmac_vap_stru           *pst_hmac_vap;
    oal_uint8               uc_fbt_cfg_mode = 0;
    oal_uint8               uc_user_index = 0;


    /* 入参检查 */
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_fbt_set_mode::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_HILINK, "{hmac_config_fbt_set_mode::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (HMAC_FBT_MODE_AC < (*puc_param))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_fbt_set_mode::invalid uc_fbt_cfg_mode=%d.}", uc_fbt_cfg_mode);
        return OAL_FAIL;
    }

    /* 获取用户下发的要配置到fbt管理实体中的参数 */
    uc_fbt_cfg_mode = *puc_param;

    /* 获取hmac vap实体中的fbt管理实体 */
    pst_hmac_fbt_mgmt = &(pst_hmac_vap->st_vap_base_info.st_fbt_mgmt);


    /* 如果要配置的模式与fbg管理实体中的模式一致，
       则不做处理，直接返回，并给出提示信息；*/
    if (uc_fbt_cfg_mode == pst_hmac_fbt_mgmt->uc_fbt_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_fbt_set_mode::uc_fbt_cfg_mode eq pst_hmac_fbt_mgmt->uc_fbt_mode,return.}");
        return OAL_SUCC;
    }

    /* 关闭FBT模式需要将禁止连接列表及索引清零； */
    if (uc_fbt_cfg_mode == HMAC_FBT_MODE_CLOSE)
    {
        for (uc_user_index = 0; uc_user_index < HMAC_FBT_MAX_USER_NUM; uc_user_index++)
        {
            oal_memset(pst_hmac_fbt_mgmt->ast_fbt_disable_connect_user_list + uc_user_index, 0, OAL_SIZEOF(mac_fbt_disable_user_info_stru));
        }
        pst_hmac_fbt_mgmt->uc_disabled_user_cnt = 0;

    }

    /* 记录配置的fbt模式到fbt管理实体中 */
    pst_hmac_fbt_mgmt->uc_fbt_mode = uc_fbt_cfg_mode;

    return OAL_SUCC;
}


oal_uint32  hmac_fbt_stop_scan(mac_vap_stru *pst_mac_vap)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    oal_uint8                   uc_user_index = 0;
    mac_device_stru            *pst_mac_device;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_start_scan::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);
    pst_fbt_scan_mgmt->uc_fbt_scan_enable = HMAC_FBT_SCAN_CLOSE;

    hmac_fbt_stop_scan_timer(pst_mac_vap);
    hmac_fbt_stop_scan_report_timer(pst_mac_vap);

#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
    hmac_fbt_result_report(pst_fbt_scan_mgmt);
#endif

    /* 关闭FBT scan模式需要将侦听列表清零；*/
    for (uc_user_index = 0; uc_user_index < HMAC_FBT_MAX_USER_NUM; uc_user_index++)
    {
        oal_memset(pst_fbt_scan_mgmt->ast_fbt_scan_user_list + uc_user_index, 0, OAL_SIZEOF(mac_fbt_scan_result_stru));
    }

    /* 恢复默认值 */
    pst_fbt_scan_mgmt->uc_scan_channel           = FBT_DEFAULT_SCAN_CHANNEL;
    pst_fbt_scan_mgmt->ul_scan_interval          = FBT_DEFAULT_SCAN_INTERVAL;
    pst_fbt_scan_mgmt->ul_scan_report_period     = FBT_DEFAULT_SCAN_REPORT_PERIOD;
    pst_fbt_scan_mgmt->ul_scan_timestamp         = 0;

    return OAL_SUCC;
}



oal_void  hmac_fbt_sta_scan_cb(void *p_scan_record)
{
    return;
}


void hmac_fbt_sta_scan(mac_vap_stru *pst_mac_vap)
{

    mac_scan_req_stru                st_mac_scan_param;
    oal_uint8                        uc_chan_idx;
    hmac_vap_stru                   *pst_hmac_vap;
    mac_fbt_scan_mgmt_stru          *pst_fbt_scan_mgmt;
    mac_device_stru                 *pst_mac_device;
    wlan_channel_band_enum_uint8     en_channel_band;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{Hmac_fbt_sta_scan::pst_mac_vap null.}");
        return ;
    }

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_mac_device    = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    OAL_MEMZERO(&st_mac_scan_param, OAL_SIZEOF(st_mac_scan_param));
    en_channel_band = pst_fbt_scan_mgmt->uc_scan_channel > WLAN_2G_CHANNEL_NUM ? WLAN_BAND_5G : WLAN_BAND_2G;
    mac_get_channel_idx_from_num_etc(en_channel_band,pst_fbt_scan_mgmt->uc_scan_channel,&uc_chan_idx);

    st_mac_scan_param.ast_channel_list[0].uc_chan_number = pst_fbt_scan_mgmt->uc_scan_channel;
    st_mac_scan_param.ast_channel_list[0].en_band        = en_channel_band;
    st_mac_scan_param.ast_channel_list[0].uc_chan_idx         = uc_chan_idx;
    st_mac_scan_param.ast_channel_list[0].en_bandwidth   = WLAN_BAND_WIDTH_20M;

    st_mac_scan_param.en_bss_type                        = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_mac_scan_param.en_scan_type                       = WLAN_SCAN_TYPE_PASSIVE;

    st_mac_scan_param.en_need_switch_back_home_channel   = OAL_TRUE;
    st_mac_scan_param.en_scan_mode                       = WLAN_SCAN_MODE_BACKGROUND_HILINK;
    st_mac_scan_param.uc_channel_nums                    = 1;
    st_mac_scan_param.uc_max_scan_count_per_channel      = 1;
    st_mac_scan_param.uc_scan_channel_interval           = 1;
    st_mac_scan_param.uc_scan_func                       = MAC_SCAN_FUNC_ALL;
    st_mac_scan_param.uc_vap_id                          = pst_mac_vap->uc_vap_id;
    st_mac_scan_param.us_scan_time                       = pst_fbt_scan_mgmt->us_scan_channel_stay_time;
    st_mac_scan_param.us_work_time_on_home_channel       = pst_fbt_scan_mgmt->us_work_channel_stay_time;
    st_mac_scan_param.p_fn_cb                            = hmac_fbt_sta_scan_cb;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    OAM_WARNING_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_sta_scan::scaning channel %d ,scan time %d,home channel stay time %d\n!}\r\n",
                        st_mac_scan_param.ast_channel_list[0].uc_chan_number,
                        st_mac_scan_param.us_scan_time,
                        st_mac_scan_param.us_work_time_on_home_channel);
    hmac_scan_proc_scan_req_event_etc(pst_hmac_vap, &st_mac_scan_param);

    return ;
}


OAL_STATIC oal_uint32  hmac_fbt_scan_timeout(oal_void *p_arg)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    mac_device_stru            *pst_mac_device;
    hmac_vap_stru              *pst_hmac_vap;
    mac_vap_stru               *pst_mac_vap;
    oal_uint32                  ul_current_time;
    oal_uint32                  ul_scan_period;

    /* 入参检查 */
    if (OAL_PTR_NULL == p_arg)
    {
        OAM_WARNING_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_scan_timeout::p_arg null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap  = (mac_vap_stru *)p_arg;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_fbt_scan_timeout::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 先获取device id */
    pst_mac_device  = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_fbt_scan_timeout::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }


    /* 获取mac device实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    /* 如果没有开启FBT scan 功能，不做处理直接返回 */
    if (HMAC_FBT_SCAN_CLOSE == pst_fbt_scan_mgmt->uc_fbt_scan_enable)
    {
        return OAL_SUCC;
    }

    ul_current_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    ul_scan_period  = pst_fbt_scan_mgmt->us_work_channel_stay_time + pst_fbt_scan_mgmt->us_scan_channel_stay_time;

    if (ul_current_time - pst_fbt_scan_mgmt->ul_scan_timestamp + ul_scan_period <= pst_fbt_scan_mgmt->ul_scan_interval)
    {
        hmac_fbt_sta_scan(pst_mac_vap);
    }
    else
    {
        hmac_fbt_stop_scan(pst_mac_vap);
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_fbt_scan_timeout::hilink scan timeout!.}");
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  hmac_fbt_scan_report_timeout(oal_void *p_arg)
{
#define HILINK_DETECT_NOTIFY_PKT_NUM       3
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    mac_fbt_scan_result_stru   *pst_fbt_scan_result;
    oal_uint8                   uc_user_idx;
    mac_device_stru            *pst_mac_device;
    hmac_fbt_notify_stru        st_fbt_notify;
    hmac_fbt_notify_stru       *pst_fbt_notify;
    hmac_vap_stru              *pst_hmac_vap;

    /* 入参检查 */
    if (OAL_PTR_NULL == p_arg)
    {
        OAM_WARNING_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_scan_report_timeout::p_arg null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap  = (hmac_vap_stru *)p_arg;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_scan_report_timeout::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 先获取device id */
    pst_mac_device  = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_scan_report_timeout::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac device实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    /* 如果没有开启FBT scan 功能，不做处理直接返回 */
    if(HMAC_FBT_SCAN_CLOSE == pst_fbt_scan_mgmt->uc_fbt_scan_enable)
    {
        return OAL_SUCC;
    }

    /* 如果 FBT scan侦听列表中所有用户还没有处理完，继续下面的处理 */
    for (uc_user_idx = 0; uc_user_idx < HMAC_FBT_MAX_USER_NUM; uc_user_idx++)   // HMAC_FBT_MAX_USER_NUM 3 for test
    {
        /* 找到FBT scan侦听管理实体，初始化FBT scan侦听结果索引i=0 */
        pst_fbt_scan_result = &(pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx]);

        if (OAL_TRUE == pst_fbt_scan_result->uc_is_found)
        {
            /* 填充上报结构体 */
            pst_fbt_notify = &st_fbt_notify;
            oal_memcopy(pst_fbt_notify->auc_user_bssid, pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_bssid, WLAN_MAC_ADDR_LEN);
            pst_fbt_notify->uc_user_channel = pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].uc_user_channel;
            pst_fbt_notify->uc_user_rssi = (oal_uint8)(oal_get_real_rssi(pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].s_rssi) + HMAC_FBT_RSSI_ADJUST_VALUE);
            pst_fbt_notify->en_fbt_notify_type = HMAC_FBT_STA_FOUND;
            oal_memcopy(pst_fbt_notify->auc_user_mac_addr, pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_mac_addr, WLAN_MAC_ADDR_LEN);
            pst_fbt_notify->ul_rssi_timestamp = pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].ul_rssi_timestamp;
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
            /* 调用上报接口上报 */
            if(pst_fbt_scan_result->ul_total_pkt_cnt > HILINK_DETECT_NOTIFY_PKT_NUM)
            {
                hmac_fbt_detect_notify(pst_hmac_vap, pst_fbt_notify);
            }
#endif
            hmac_fbt_notify(pst_hmac_vap, pst_fbt_notify);
            OAM_INFO_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{hmac_fbt_scan_report_timeout::uc_user_idx=%d MAC:0x%x:0x%x:0x%x is found!}",
                                        uc_user_idx,
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_mac_addr[3],
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_mac_addr[4],
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_mac_addr[5]);

            OAM_INFO_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{hmac_fbt_scan_report_timeout::bssid MAC:0x%x:0x%x:0x%x:0x%x.}",
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_bssid[2],
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_bssid[3],
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_bssid[4],
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].auc_user_bssid[5]);

            OAM_INFO_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{hmac_fbt_scan_report_timeout::channel =%d,c_rssi=%d,ul_rssi_timestamp=%d,ul_total_pkt_cnt=%d.}",
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].uc_user_channel,
                                        (oal_uint8)oal_get_real_rssi(pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].s_rssi),
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].ul_rssi_timestamp,
                                        pst_fbt_scan_mgmt->ast_fbt_scan_user_list[uc_user_idx].ul_total_pkt_cnt);

        }

    }

    return OAL_SUCC;
}



oal_uint32  hmac_fbt_start_scan_timer(mac_vap_stru *pst_mac_vap, oal_uint32 ul_period)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    hmac_vap_stru              *pst_hmac_vap;
    mac_device_stru            *pst_mac_device;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_start_scan_timer::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_start_scan_timer::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_start_scan_timer::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac device实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    /* 创建FBT scan结果上报定时器*/
    FRW_TIMER_CREATE_TIMER(&pst_fbt_scan_mgmt->st_scan_timer,
                           hmac_fbt_scan_timeout,
                           ul_period,
                           pst_mac_vap,
                           OAL_TRUE,
                           OAM_MODULE_ID_HMAC,
                           pst_mac_vap->ul_core_id);
    return OAL_SUCC;
}



oal_uint32  hmac_fbt_start_scan_report_timer(mac_vap_stru *pst_mac_vap, oal_uint32 ul_period)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    hmac_vap_stru               *pst_hmac_vap;
    mac_device_stru            *pst_mac_device;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_start_monitor_timer::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_start_monitor_timer::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_start_scan_report_timer::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac device实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);


    /* 创建FBT scan结果上报定时器*/
    FRW_TIMER_CREATE_TIMER(&pst_fbt_scan_mgmt->st_timer,
                           hmac_fbt_scan_report_timeout,
                           ul_period,
                           pst_hmac_vap,
                           OAL_TRUE,
                           OAM_MODULE_ID_HMAC,
                           pst_mac_vap->ul_core_id);
    return OAL_SUCC;
}




oal_uint32  hmac_fbt_stop_scan_timer(mac_vap_stru *pst_mac_vap)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    frw_timeout_stru           *pst_timer;
    mac_device_stru            *pst_mac_device;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_stop_scan_report_timer: pst_mac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_stop_scan_report_timer::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    pst_timer = &(pst_fbt_scan_mgmt->st_scan_timer);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_timer))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "hmac_fbt_stop_scan_report_timer::pst_timer is NULL");
        return OAL_FAIL;
    }

    FRW_TIMER_DESTROY_TIMER(pst_timer);

    return OAL_SUCC;

}



oal_uint32  hmac_fbt_stop_scan_report_timer(mac_vap_stru *pst_mac_vap)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    frw_timeout_stru           *pst_timer;
    mac_device_stru            *pst_mac_device;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_stop_scan_report_timer: pst_mac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_stop_scan_report_timer::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    pst_timer = &(pst_fbt_scan_mgmt->st_timer);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_timer))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "hmac_fbt_stop_scan_report_timer::pst_timer is NULL");
        return OAL_FAIL;
    }

    FRW_TIMER_DESTROY_TIMER(pst_timer);

    return OAL_SUCC;

}

oal_uint32  hmac_fbt_start_scan(mac_vap_stru *pst_mac_vap)
{

    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    mac_device_stru            *pst_mac_device;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_start_scan::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    pst_fbt_scan_mgmt->ul_scan_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();


    /* 如果配置参数是开启fbt模式，则要按照上报周期启动定时器，注册回调函数；
       否则是关闭fbt模式，关闭定时器；*/
    if ((HMAC_FBT_SCAN_OPEN == pst_fbt_scan_mgmt->uc_fbt_scan_enable) && (0 != pst_fbt_scan_mgmt->ul_scan_interval))
    {
        hmac_fbt_start_scan_timer(pst_mac_vap, pst_fbt_scan_mgmt->us_work_channel_stay_time + pst_fbt_scan_mgmt->us_scan_channel_stay_time);
        hmac_fbt_start_scan_report_timer(pst_mac_vap, pst_fbt_scan_mgmt->ul_scan_report_period);
    }
    else
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_start_scan::hilink scan stop.}");
        hmac_fbt_stop_scan(pst_mac_vap);
    }

    return OAL_SUCC;
}



oal_uint32  hmac_fbt_init(hmac_vap_stru *pst_hmac_vap)
{
    mac_fbt_mgmt_stru          *pst_hmac_fbt_mgmt;
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;
    mac_device_stru            *pst_mac_device;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_init::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_init::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取hmac vap实体中的fbt管理实体 */
    pst_hmac_fbt_mgmt = &(pst_hmac_vap->st_vap_base_info.st_fbt_mgmt);

    /* 将vap里面的fbt管理实体清零，然后初始化切换的阈值为-40dbm，fbt开关为关闭，
    监控已关联用户的时间为5s */
    oal_memset(pst_hmac_fbt_mgmt, 0, OAL_SIZEOF(mac_fbt_mgmt_stru));

    /* 默认开启hilink fbt模式，可通过命令hipriv.sh "wlan0 fbt_set_mode 0|1"开关 */
    pst_hmac_fbt_mgmt->uc_fbt_mode = HMAC_FBT_MODE_AC;

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    /* 将vap里面的fbt scan管理实体清零，然后再配置默认值  */
    oal_memset(pst_fbt_scan_mgmt, 0, OAL_SIZEOF(mac_fbt_scan_mgmt_stru));
    pst_fbt_scan_mgmt->uc_fbt_scan_enable        = HMAC_FBT_SCAN_CLOSE;
    pst_fbt_scan_mgmt->uc_scan_channel           = FBT_DEFAULT_SCAN_CHANNEL;
    pst_fbt_scan_mgmt->ul_scan_interval          = FBT_DEFAULT_SCAN_INTERVAL;
    pst_fbt_scan_mgmt->ul_scan_report_period     = FBT_DEFAULT_SCAN_REPORT_PERIOD;
    pst_fbt_scan_mgmt->us_work_channel_stay_time = FBT_DEFAULT_WORK_CHANNEL_STAY_TIME;
    pst_fbt_scan_mgmt->us_scan_channel_stay_time = FBT_DEFAULT_SCAN_CHANNEL_STAY_TIME;

    return OAL_SUCC;
}





oal_uint32  hmac_fbt_notify(hmac_vap_stru *pst_hmac_vap, hmac_fbt_notify_stru *pst_hmac_fbt_notify)
{

    oal_station_info_stru       st_station_info;
    oal_uint32                  ul_ret;
    oal_net_device_stru        *pst_net_device;
    oal_uint8                   auc_user_addr[WLAN_MAC_ADDR_LEN];

    /* 判断入参，若为空，提示信息，并返回 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == pst_hmac_fbt_notify))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_fbt_notify::null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取net_device*/
    pst_net_device = hmac_vap_get_net_device_etc(pst_hmac_vap->st_vap_base_info.uc_vap_id);
    if (OAL_PTR_NULL == pst_net_device)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_notify::get net device ptr is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memcopy(auc_user_addr, (oal_uint8 *)pst_hmac_fbt_notify->auc_user_mac_addr, WLAN_MAC_ADDR_LEN);

    /* 调用oal_cfg80211_new_sta上报到hostapd，注意在hostapd接收的地方消息要转成EVENT_DETECT_STA_FRAME */
    oal_memset(&st_station_info, 0, OAL_SIZEOF(oal_station_info_stru));

    st_station_info.signal = (oal_int8)pst_hmac_fbt_notify->uc_user_rssi;
    st_station_info.filled |=  STATION_INFO_SIGNAL;

    st_station_info.inactive_time = pst_hmac_fbt_notify->ul_rssi_timestamp;
    st_station_info.filled |=  STATION_INFO_INACTIVE_TIME;

    /* 调用内核接口，上报STA关联结果 */
    ul_ret = oal_cfg80211_fbt_notify_find_sta(pst_net_device, auc_user_addr, &st_station_info, GFP_ATOMIC);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_notify::oal_cfg80211_fbt_notify_find_sta fail[%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    OAM_INFO_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_notify::mac[%x:**:**:%x:%x:%x] OK!}\r\n",
                                                                        auc_user_addr[0],
                                                                        auc_user_addr[3],
                                                                        auc_user_addr[4],
                                                                        auc_user_addr[5]);

    return OAL_SUCC;

}

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT

void  hmac_fbt_detect_notify(hmac_vap_stru *pst_hmac_vap, hmac_fbt_notify_stru *pst_hmac_fbt_notify)
{
    hmac_fbt_detect_notify_stru st_fbt_detect_notify;
    oal_iwreq_data_union       wrqu;
    oal_net_device_stru        *pst_net_device;

    pst_net_device = pst_hmac_vap->pst_net_device;
    if (OAL_PTR_NULL == pst_net_device)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_detect_notify::get net device ptr is null!}\r\n");
        return ;
    }
    st_fbt_detect_notify.timestamp = pst_hmac_fbt_notify->ul_rssi_timestamp;
    st_fbt_detect_notify.ver = 0;
    st_fbt_detect_notify.rssi = pst_hmac_fbt_notify->uc_user_rssi;
    oal_memcopy(st_fbt_detect_notify.sta, pst_hmac_fbt_notify->auc_user_mac_addr, WLAN_MAC_ADDR_LEN);

    OAL_MEMZERO(&wrqu, sizeof(wrqu));
    wrqu.data.flags = IEEE80211_EV_HI_DETECTED_STA;
    wrqu.data.length = sizeof(hmac_fbt_detect_notify_stru);

    OAM_INFO_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_fbt_detect_notify::sta MAC:0x%x:0x%x:0x%x is found!}",
                                pst_hmac_fbt_notify->auc_user_mac_addr[3],
                                pst_hmac_fbt_notify->auc_user_mac_addr[4],
                                pst_hmac_fbt_notify->auc_user_mac_addr[5]);
#ifndef WIN32
    wireless_send_event(pst_net_device, IWEVCUSTOM, &wrqu, (char *)&st_fbt_detect_notify);
#endif
}

#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN

oal_uint32 hmac_hilink_bcn_rpt_notify_hook(hmac_user_stru *pst_hmac_user, mac_rrm_state_enum en_rpt_state)
{
    mac_user_rrm_info_stru          *pst_rrm_info;
    mac_meas_rpt_bcn_stru           *pst_meas_rpt_bcn;
    mac_meas_rpt_bcn_item_stru      *pst_meas_rpt_bcn_item;
    oal_dlist_head_stru             *pst_meas_rpt_node      = OAL_PTR_NULL;

    pst_rrm_info        = pst_hmac_user->pst_user_rrm_info;
    if ( MAC_RRM_STATE_GET_RSP != en_rpt_state )
    {
        return OAL_FAIL;
    }

    if (oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)))
    {
        OAM_WARNING_LOG1(0, OAM_SF_HILINK, "{hmac_hilink_bcn_rpt_notify_hook:pst_rrm_info->st_meas_rpt_list is null, en_meas_status[%d].",
            pst_rrm_info->en_meas_status);

        OAM_WARNING_LOG3(0, OAM_SF_HILINK, "{hmac_hilink_bcn_rpt_notify_hook::bit_late[%d],bit_incapable[%d],bit_refused[%d]!}",
        pst_rrm_info->st_rptmode.bit_late,
        pst_rrm_info->st_rptmode.bit_incapable,
        pst_rrm_info->st_rptmode.bit_refused);
        return OAL_FAIL;
    }

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_meas_rpt_node, &(pst_rrm_info->st_meas_rpt_list))
    {
        pst_meas_rpt_bcn = OAL_DLIST_GET_ENTRY(pst_meas_rpt_node, mac_meas_rpt_bcn_stru, st_dlist_head);
        if (OAL_PTR_NULL == pst_meas_rpt_bcn->pst_meas_rpt_bcn_item)
        {
            continue;
        }
        pst_meas_rpt_bcn_item = pst_meas_rpt_bcn->pst_meas_rpt_bcn_item;

        OAM_WARNING_LOG4(0, OAM_SF_HILINK, "{hmac_hilink_bcn_rpt_notify_hook::rssi=%d, mac_addr=%x:%x:%x!}",
            pst_meas_rpt_bcn_item->uc_rcpi,
            pst_meas_rpt_bcn_item->auc_bssid[3], pst_meas_rpt_bcn_item->auc_bssid[4],
            pst_meas_rpt_bcn_item->auc_bssid[5]);
    }
    return OAL_SUCC;
}
#endif
/*lint -e19*/
oal_module_symbol(hmac_config_fbt_set_mode);
oal_module_symbol(hmac_fbt_start_scan);
oal_module_symbol(hmac_fbt_stop_scan);
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
oal_module_symbol(hmac_hilink_bcn_rpt_notify_hook);
#endif
/*lint +e19*/
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


