


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "mac_regdomain.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_DEVICE_ROM_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_WMMAC
oal_bool_enum_uint8 g_en_wmmac_switch_etc = OAL_TRUE;
#endif

/* 动态/静态DBDC */
/* 这里指的是每个chip上mac device的频道能力 */
oal_uint8  g_auc_mac_device_radio_cap[WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP] = {
    MAC_DEVICE_2G_5G,
#if (WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP > 1)
    MAC_DEVICE_DISABLE
#endif
};

mac_board_stru *g_pst_mac_board = &g_st_mac_board;
mac_device_capability_stru *g_pst_mac_device_capability = &g_st_mac_device_capability[0];


/*****************************************************************************
  3 函数实现
*****************************************************************************/

wlan_mib_vht_supp_width_enum_uint8  mac_device_trans_bandwith_to_vht_capinfo(wlan_bw_cap_enum_uint8 en_max_op_bd)
{
    switch (en_max_op_bd)
    {
        case WLAN_BW_CAP_20M:
        case WLAN_BW_CAP_40M:
        case WLAN_BW_CAP_80M:
            return WLAN_MIB_VHT_SUPP_WIDTH_80;

        case WLAN_BW_CAP_160M:
            return WLAN_MIB_VHT_SUPP_WIDTH_160;

        case WLAN_BW_CAP_80PLUS80:
            return WLAN_MIB_VHT_SUPP_WIDTH_80PLUS80;

        default:
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{mac_device_trans_bandwith_to_vht_capinfo::bandwith[%d] is invalid.}", en_max_op_bd);
            return WLAN_MIB_VHT_SUPP_WIDTH_BUTT;
    }
}



oal_uint32  mac_device_check_5g_enable(oal_uint8 uc_device_id)
{
    oal_uint8           uc_device_id_per_chip;
    mac_device_stru    *pst_mac_device;

    pst_mac_device = mac_res_get_dev_etc(uc_device_id);

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "mac_device_check_5g_enable:get dev fail uc_device_id[%d]", uc_device_id);
        return OAL_FALSE;
    }

    /* 03两个业务device,00 01,取不同定制化,51双芯片00 11,取同一个定制化*/
    uc_device_id_per_chip = uc_device_id - pst_mac_device->uc_chip_id;

    return !!(g_auc_mac_device_radio_cap[uc_device_id_per_chip] & MAC_DEVICE_5G);
}
oal_uint32  mac_chip_init_etc(mac_chip_stru *pst_chip, oal_uint8 uc_device_max)
{
    pst_chip->uc_assoc_user_cnt = 0;
    pst_chip->uc_active_user_cnt = 0;

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    oal_dlist_init_head(&pst_chip->st_user_extend.st_active_user_list_head);
    /* 默认打开用户扩展开关 */
    pst_chip->st_user_extend.en_flag = OAL_SWITCH_ON;
#endif

#ifdef  _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    pst_chip->pst_rx_aggr_extend = (mac_chip_rx_aggr_extend_stru*)oal_memalloc(sizeof(mac_chip_rx_aggr_extend_stru));
    if(OAL_PTR_NULL == pst_chip->pst_rx_aggr_extend)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_data_acq_mem_alloc::puc_start_addr null.}");
        return OAL_FAIL;
    }
#endif

    /* 保存device数量 */
    pst_chip->uc_device_nums = uc_device_max;

    /* 初始化最后再将state置为TRUE */
    pst_chip->en_chip_state = OAL_TRUE;

    return OAL_SUCC;
}
oal_void  mac_device_set_beacon_interval_etc(mac_device_stru *pst_mac_device, oal_uint32 ul_beacon_interval)
{
    pst_mac_device->ul_beacon_interval = ul_beacon_interval;
}

oal_void  mac_chip_inc_assoc_user(mac_chip_stru *pst_mac_chip)
{
    pst_mac_chip->uc_assoc_user_cnt++;
    OAM_WARNING_LOG1(0, OAM_SF_UM, "{mac_chip_inc_assoc_user::uc_asoc_user_cnt[%d].}", pst_mac_chip->uc_assoc_user_cnt);
    if (0xFF == pst_mac_chip->uc_assoc_user_cnt)
    {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{mac_chip_inc_assoc_user::uc_asoc_user_cnt=0xFF now!}");
        oam_report_backtrace();
    }
}

oal_void  mac_chip_dec_assoc_user(mac_chip_stru *pst_mac_chip)
{
    OAM_WARNING_LOG1(0, OAM_SF_UM, "{mac_chip_dec_assoc_user::uc_asoc_user_cnt[%d].}", pst_mac_chip->uc_assoc_user_cnt);
    if (0 == pst_mac_chip->uc_assoc_user_cnt)
    {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{mac_chip_dec_assoc_user::uc_assoc_user_cnt is already zero.}");
        oam_report_backtrace();
    }
    else
    {
        pst_mac_chip->uc_assoc_user_cnt--;
    }
}

oal_void  mac_chip_inc_active_user(mac_chip_stru *pst_mac_chip)
{
    pst_mac_chip->uc_active_user_cnt++;
}

oal_void  mac_chip_dec_active_user(mac_chip_stru *pst_mac_chip)
{
    if (0 == pst_mac_chip->uc_active_user_cnt)
    {
        OAM_ERROR_LOG0(0, OAM_SF_UM, "{mac_chip_dec_active_user::uc_active_user_cnt is already zero.}");
    }
    else
    {
        pst_mac_chip->uc_active_user_cnt--;
    }
}

#if 0
oal_void  mac_device_set_dfs(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_dfs_switch, oal_uint8 uc_debug_level)
{
    /*待整改dfs变量后 生效*/
#if 0
    pst_mac_device->en_dfs_switch = en_dfs_switch;
    pst_mac_device->uc_debug_level = uc_debug_level;
#endif
}
#endif

oal_void* mac_device_get_all_rates_etc(mac_device_stru *pst_dev)
{
    return (oal_void *)pst_dev->st_mac_rates_11g;
}
#ifdef _PRE_WLAN_FEATURE_HILINK

oal_uint32  mac_device_clear_fbt_scan_list(mac_device_stru *pst_mac_dev, oal_uint8 *puc_param)
{

    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;
    oal_uint8                   uc_idx;
    mac_fbt_scan_result_stru   *pst_user;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_clear_fbt_scan_list::null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_clear_fbt_scan_list::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);
    /* 遍历所有用户，清零 */
    for(uc_idx = 0; uc_idx < HMAC_FBT_MAX_USER_NUM; uc_idx++)
    {
        pst_user = &(pst_fbt_scan_info->ast_fbt_scan_user_list[uc_idx]);
        OAL_MEMZERO(pst_user, OAL_SIZEOF(mac_fbt_scan_result_stru));
    }

    return OAL_SUCC;
}



oal_uint32  mac_device_set_fbt_scan_sta(mac_device_stru *pst_mac_dev, mac_fbt_scan_sta_addr_stru *pst_fbt_scan_sta)
{
    mac_fbt_scan_mgmt_stru                     *pst_fbt_scan_info;
    oal_uint32                                  ul_idx;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_set_fbt_scan_sta::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_fbt_scan_sta)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_set_fbt_scan_sta::null param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);

    /* 遍历所有用户，在空用户处写入mac地址，更改使用状态 */
    for (ul_idx = 0; ul_idx < HMAC_FBT_MAX_USER_NUM; ul_idx++)
    {
        /* 如果需要侦听的STA已经在列表中，则不再重新添加 */
        if (HMAC_FBT_SCAN_USER_IS_USED == pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].uc_is_used)
        {
            if (0 == oal_memcmp(pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr, pst_fbt_scan_sta->auc_mac_addr, WLAN_MAC_ADDR_LEN))
            {
                break;
            }
        }
        else
        {
            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].uc_is_used = HMAC_FBT_SCAN_USER_IS_USED;
            oal_memcopy(pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr,
                        pst_fbt_scan_sta->auc_mac_addr, WLAN_MAC_ADDR_LEN);

            OAM_WARNING_LOG4(pst_mac_dev->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_sta::idx:%d MAC[%2x:**:**:**:%2x:%2x]}",
                            ul_idx,
                            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr[0],
                            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr[4],
                            pst_fbt_scan_info->ast_fbt_scan_user_list[ul_idx].auc_user_mac_addr[5]);
            break;
        }
    }

    /* 列表已满 */
    if (OAL_UNLIKELY(HMAC_FBT_MAX_USER_NUM == ul_idx))
    {
        OAM_WARNING_LOG0(0, OAM_SF_HILINK, "{mac_device_set_fbt_scan_sta::scan user list is full}");
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    return OAL_SUCC;

}


oal_uint32  mac_device_set_fbt_scan_interval(mac_device_stru *pst_mac_dev, oal_uint32 ul_scan_interval)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;
    oal_uint32                  ul_current_time;
    oal_uint32                  ul_last_scan_remain_time;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_set_fbt_scan_interval::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);
    ul_current_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
    if (0 != pst_fbt_scan_info->ul_scan_timestamp)//上次扫描尚未结束
    {
        ul_last_scan_remain_time = pst_fbt_scan_info->ul_scan_interval - (ul_current_time - pst_fbt_scan_info->ul_scan_timestamp);
        pst_fbt_scan_info->ul_scan_interval  = ul_last_scan_remain_time > ul_scan_interval ? ul_last_scan_remain_time:ul_scan_interval;
        pst_fbt_scan_info->ul_scan_timestamp = ul_current_time;
    }
    else
    {
        pst_fbt_scan_info->ul_scan_interval = ul_scan_interval;
    }

    OAM_INFO_LOG1(pst_mac_dev->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_interval::=%d.}", pst_fbt_scan_info->ul_scan_interval);

    return OAL_SUCC;
}



oal_uint32  mac_device_set_fbt_scan_channel(mac_device_stru *pst_mac_dev, oal_uint8 uc_fbt_scan_channel)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_set_fbt_scan_channel::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);
    pst_fbt_scan_info->uc_scan_channel = uc_fbt_scan_channel;

    OAM_INFO_LOG1(pst_mac_dev->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_channel::=%d.}", pst_fbt_scan_info->uc_scan_channel);

    return OAL_SUCC;
}


oal_uint32  mac_device_set_fbt_scan_report_period(mac_device_stru *pst_mac_dev, oal_uint32 ul_fbt_scan_report_period)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_set_fbt_scan_report_period::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);
    pst_fbt_scan_info->ul_scan_report_period = ul_fbt_scan_report_period;

    OAM_INFO_LOG1(pst_mac_dev->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_report_period::ul_scan_report_period=%d.}", pst_fbt_scan_info->ul_scan_report_period);

    return OAL_SUCC;
}


oal_uint32  mac_device_set_fbt_scan_enable(mac_device_stru *pst_mac_device, oal_uint8 uc_cfg_fbt_scan_enable)
{

    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_mgmt;

    /* 入参检查 */
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{mac_device_set_fbt_scan_enable::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (uc_cfg_fbt_scan_enable > HMAC_FBT_SCAN_OPEN)
    {
        OAM_WARNING_LOG1(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_enable::invalid uc_cfg_fbt_scan_enable=%d.}", uc_cfg_fbt_scan_enable);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_fbt_scan_mgmt = &(pst_mac_device->st_fbt_scan_mgmt);

    /* 如果要配置的模式与fbg scan管理实体中的模式一致，
       则不做处理，直接返回，并给出提示信息；*/
    if (uc_cfg_fbt_scan_enable == pst_fbt_scan_mgmt->uc_fbt_scan_enable)
    {
        OAM_WARNING_LOG0(pst_mac_device->uc_device_id, OAM_SF_HILINK, "{mac_device_set_fbt_scan_enable::uc_cfg_fbt_scan_enable eq pst_fbt_scan_mgmt->uc_fbt_scan_enable,return.}");
        return OAL_SUCC;
    }


    /* 记录配置的模式到fbt scan管理实体中，当前只支持侦听一个用户 */
    pst_fbt_scan_mgmt->uc_fbt_scan_enable = uc_cfg_fbt_scan_enable;

    OAM_WARNING_LOG4(pst_mac_device->uc_device_id, OAM_SF_HILINK, "mac_device_set_fbt_scan_enable::user uc_cfg_fbt_scan_enable=%d, ul_scan_report_period=%d, uc_scan_channel=%d ul_scan_interval=%d",
                                    uc_cfg_fbt_scan_enable, pst_fbt_scan_mgmt->ul_scan_report_period, pst_fbt_scan_mgmt->uc_scan_channel, pst_fbt_scan_mgmt->ul_scan_interval);

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DFS

oal_void  mac_dfs_set_cac_enable(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_val)
{
    pst_mac_device->st_dfs.st_dfs_info.en_cac_switch = en_val;
}


oal_void  mac_dfs_set_offchan_cac_enable(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_val)
{
    pst_mac_device->st_dfs.st_dfs_info.en_offchan_cac_switch = en_val;
}


oal_bool_enum_uint8  mac_dfs_get_offchan_cac_enable(mac_device_stru *pst_mac_device)
{
    mac_regdomain_info_stru   *pst_rd_info;

    mac_get_regdomain_info_etc(&pst_rd_info);
    if(MAC_DFS_DOMAIN_ETSI == pst_rd_info->en_dfs_domain)
    {
        return pst_mac_device->st_dfs.st_dfs_info.en_offchan_cac_switch;
    }

    return OAL_FALSE;
}



oal_void  mac_dfs_set_offchan_number(mac_device_stru *pst_mac_device, oal_uint32 ul_val)
{
    pst_mac_device->st_dfs.st_dfs_info.uc_offchan_num = (oal_uint8)ul_val;
}



oal_bool_enum_uint8  mac_dfs_get_cac_enable(mac_device_stru *pst_mac_device)
{
    return pst_mac_device->st_dfs.st_dfs_info.en_cac_switch;
}


 oal_void  mac_dfs_set_dfs_enable(mac_device_stru *pst_mac_device, oal_bool_enum_uint8 en_val)
{
    pst_mac_device->st_dfs.st_dfs_info.en_dfs_switch = en_val;

    /* 如果 软件雷达检测使能 关闭，则关闭CAC检测 */
    if (OAL_FALSE == en_val)
    {
        pst_mac_device->st_dfs.st_dfs_info.en_cac_switch = OAL_FALSE;
    }
}


oal_bool_enum_uint8  mac_dfs_get_dfs_enable(mac_device_stru *pst_mac_device)
{
    if (WLAN_BAND_5G == pst_mac_device->en_max_band)
    {
        return pst_mac_device->st_dfs.st_dfs_info.en_dfs_switch;
    }

    return OAL_FALSE;
}


oal_void  mac_dfs_set_debug_level(mac_device_stru *pst_mac_device, oal_uint8 uc_debug_lev)
{
    pst_mac_device->st_dfs.st_dfs_info.uc_debug_level = uc_debug_lev;
}


oal_uint8  mac_dfs_get_debug_level(mac_device_stru *pst_mac_device)
{
    return pst_mac_device->st_dfs.st_dfs_info.uc_debug_level;
}


oal_void  mac_dfs_set_cac_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms, oal_bool_enum_uint8 en_waether)
{
    if (en_waether)
    {
        pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_in_5600_to_5650_time_ms = ul_time_ms;
    }
    else
    {
        pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_outof_5600_to_5650_time_ms = ul_time_ms;
    }
}


oal_void  mac_dfs_set_off_cac_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms, oal_bool_enum_uint8 en_waether)
{
    if (en_waether)
    {
        pst_mac_device->st_dfs.st_dfs_info.ul_off_chan_cac_in_5600_to_5650_time_ms = ul_time_ms;
    }
    else
    {
        pst_mac_device->st_dfs.st_dfs_info.ul_off_chan_cac_outof_5600_to_5650_time_ms = ul_time_ms;
    }
}



oal_void  mac_dfs_set_opern_chan_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms)
{
    pst_mac_device->st_dfs.st_dfs_info.us_dfs_off_chan_cac_opern_chan_dwell_time = (oal_uint16)ul_time_ms;
}


oal_void  mac_dfs_set_off_chan_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time_ms)
{
    pst_mac_device->st_dfs.st_dfs_info.us_dfs_off_chan_cac_off_chan_dwell_time = (oal_uint16)ul_time_ms;
}


oal_void  mac_dfs_set_next_radar_ch(mac_device_stru *pst_mac_device, oal_uint8 uc_ch, wlan_channel_bandwidth_enum_uint8 en_width)
{
    pst_mac_device->st_dfs.st_dfs_info.en_next_ch_width_type = en_width;
    pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum = uc_ch;
}


oal_void  mac_dfs_set_ch_bitmap(mac_device_stru *pst_mac_device, oal_uint32 ul_ch_bitmap)
{
    pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap = ul_ch_bitmap;
}

oal_void  mac_dfs_set_non_occupancy_period_time(mac_device_stru *pst_mac_device, oal_uint32 ul_time)
{
    pst_mac_device->st_dfs.st_dfs_info.ul_dfs_non_occupancy_period_time_ms = ul_time;
}

#endif

/*lint -e19*/
oal_module_symbol(mac_device_set_beacon_interval_etc);
oal_module_symbol(mac_chip_inc_active_user);
oal_module_symbol(mac_chip_dec_active_user);
oal_module_symbol(mac_chip_inc_assoc_user);
oal_module_symbol(mac_chip_dec_assoc_user);
#if 0
oal_module_symbol(mac_device_inc_assoc_user);
oal_module_symbol(mac_device_dec_assoc_user);
oal_module_symbol(mac_device_set_dfs);
#endif

oal_module_symbol(mac_chip_init_etc);

#ifdef _PRE_WLAN_FEATURE_HILINK
oal_module_symbol(mac_device_clear_fbt_scan_list);
oal_module_symbol(mac_device_set_fbt_scan_sta);
oal_module_symbol(mac_device_set_fbt_scan_interval);
oal_module_symbol(mac_device_set_fbt_scan_channel);
oal_module_symbol(mac_device_set_fbt_scan_report_period);
oal_module_symbol(mac_device_set_fbt_scan_enable);
#endif

oal_module_symbol(mac_device_check_5g_enable);
oal_module_symbol(g_auc_mac_device_radio_cap);
oal_module_symbol(g_pst_mac_board);

#ifdef _PRE_WLAN_FEATURE_DFS
oal_module_symbol(mac_dfs_set_cac_enable);
oal_module_symbol(mac_dfs_set_offchan_cac_enable);
oal_module_symbol(mac_dfs_get_offchan_cac_enable);
oal_module_symbol(mac_dfs_set_offchan_number);
oal_module_symbol(mac_dfs_get_cac_enable);
oal_module_symbol(mac_dfs_set_dfs_enable);
oal_module_symbol(mac_dfs_get_dfs_enable);
oal_module_symbol(mac_dfs_set_debug_level);
oal_module_symbol(mac_dfs_get_debug_level);
oal_module_symbol(mac_dfs_set_cac_time);
oal_module_symbol(mac_dfs_set_off_cac_time);
oal_module_symbol(mac_dfs_set_opern_chan_time);
oal_module_symbol(mac_dfs_set_off_chan_time);
oal_module_symbol(mac_dfs_set_next_radar_ch);
oal_module_symbol(mac_dfs_set_ch_bitmap);
oal_module_symbol(mac_dfs_set_non_occupancy_period_time);
#endif

/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


