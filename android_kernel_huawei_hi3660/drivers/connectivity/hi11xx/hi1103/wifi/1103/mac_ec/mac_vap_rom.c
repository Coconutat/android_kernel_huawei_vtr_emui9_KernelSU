


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_mem.h"
#include "wlan_spec.h"
#include "wlan_types.h"
#include "hal_ext_if.h"
#include "mac_vap.h"
#include "mac_resource.h"
#include "mac_regdomain.h"
#include "mac_ie.h"
#include "dmac_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_VAP_ROM_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_UAPSD
oal_uint8 g_uc_uapsd_cap_etc = WLAN_FEATURE_UAPSD_IS_OPEN;
#endif

mac_vap_cb  g_st_mac_vap_rom_cb = {mac_init_mib_rom_cb,
                                   mac_vap_init_mib_11n_rom_cb,
                                   mac_vap_init_by_protocol_cb,
                                   mac_vap_init_11ac_mcs_singlenss_rom_cb,
                                   mac_vap_init_11ac_mcs_doublenss_rom_cb,
                                   mac_vap_ch_mib_by_bw_cb, //ch_mib_by_bw_cb
                                   mac_vap_init_11ac_rates_cb,
                                   mac_vap_init_11n_rates_cb,
                                   OAL_PTR_NULL, //init_privacy_cb
                                   OAL_PTR_NULL, //init_rates_by_prot_cb
                                   mac_vap_init_11ax_rates,
                                   mac_vap_init_mib_11ax};

/* WME初始参数定义，按照OFDM初始化 AP模式 值来自于TGn 9 Appendix D: Default WMM AC Parameters */

mac_wme_param_stru g_ast_wmm_initial_params_ap[WLAN_WME_AC_BUTT] =
{
    /* BE */
    {
        /* AIFS, cwmin, cwmax, txop */
        3,          4,     6,     0,
    },

    /* BK */
    {
        /* AIFS, cwmin, cwmax, txop */
        7,          4,     10,    0,
    },

    /* VI */
    {
        /* AIFS, cwmin, cwmax, txop */
        1,          3,     4,     3008,
    },

    /* VO */
    {
        /* AIFS, cwmin, cwmax, txop */
        1,          2,     3,     1504,
    },
};

/* WMM初始参数定义，按照OFDM初始化 STA模式 */
mac_wme_param_stru g_ast_wmm_initial_params_sta[WLAN_WME_AC_BUTT] =
{
    /* BE */
    {
        /* AIFS, cwmin, cwmax, txop */
        3,          3,     10,     0,
    },

    /* BK */
    {
        /* AIFS, cwmin, cwmax, txop */
        7,          4,     10,     0,
    },


    /* VI */
    {
        /* AIFS, cwmin, cwmax, txop */
        2,          3,     4,     3008,
    },

    /* VO */
    {
        /* AIFS, cwmin, cwmax, txop */
        2,          2,     3,     1504,
    },
};

/* WMM初始参数定义，aput建立的bss中STA的使用的EDCA参数 */
mac_wme_param_stru g_ast_wmm_initial_params_bss[WLAN_WME_AC_BUTT] =
{
    /* BE */
    {
        /* AIFS, cwmin, cwmax, txop */
        3,          4,     10,     0,
    },

    /* BK */
    {
        /* AIFS, cwmin, cwmax, txop */
        7,          4,     10,     0,
    },


    /* VI */
    {
        /* AIFS, cwmin, cwmax, txop */
        2,          3,     4,     3008,
    },

    /* VO */
    {
        /* AIFS, cwmin, cwmax, txop */
        2,          2,     3,     1504,
    },
};

#ifdef _PRE_WLAN_FEATURE_EDCA_MULTI_USER_MULTI_AC
/* 多用户多优先级使用的EDCA参数 */
mac_wme_param_stru g_ast_wmm_multi_user_multi_ac_params_ap[WLAN_WME_AC_BUTT] =
{
    /* BE */
    {
        /* AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },

    /* BK */
    {
        /* AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },


    /* VI */
    {
        /* AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },

    /* VO */
    {
        /* AIFS, cwmin, cwmax, txop */
        3,          5,     10,     0,
    },
};
#endif

/*****************************************************************************
  3 函数实现
*****************************************************************************/


mac_wme_param_stru  *mac_get_wmm_cfg_etc(wlan_vap_mode_enum_uint8 en_vap_mode)
{
    /* 参考认证项配置，没有按照协议配置，WLAN_VAP_MODE_BUTT表示是ap广播给sta的edca参数 */
    if (WLAN_VAP_MODE_BUTT == en_vap_mode)
    {
        return (mac_wme_param_stru  *)g_ast_wmm_initial_params_bss;
    }
    else if(WLAN_VAP_MODE_BSS_AP == en_vap_mode)
    {
       return (mac_wme_param_stru  *)g_ast_wmm_initial_params_ap;
    }
    return (mac_wme_param_stru  *)g_ast_wmm_initial_params_sta;

}

#ifdef _PRE_WLAN_FEATURE_EDCA_MULTI_USER_MULTI_AC

mac_wme_param_stru  *mac_get_wmm_cfg_multi_user_multi_ac(oal_traffic_type_enum_uint8 uc_traffic_type)
{
    /* 多用户下业务类型采用新参数，否则采用ap模式下的默认值 */
    if (OAL_TRAFFIC_MULTI_USER_MULTI_AC == uc_traffic_type)
    {
        return (mac_wme_param_stru  *)g_ast_wmm_multi_user_multi_ac_params_ap;
    }

    return (mac_wme_param_stru  *)g_ast_wmm_initial_params_ap;

}
#endif


oal_uint32  mac_mib_set_station_id_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_staion_id_param_stru   *pst_param;

    pst_param = (mac_cfg_staion_id_param_stru *)puc_param;

    oal_set_mac_addr(mac_mib_get_StationID(pst_mac_vap),pst_param->auc_station_id);

    return OAL_SUCC;
}
#if 0

oal_uint32  mac_mib_get_station_id(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_len, oal_uint8 *puc_param)
{
    mac_cfg_staion_id_param_stru   *pst_param;

    pst_param = (mac_cfg_staion_id_param_stru *)puc_param;

    oal_memcopy(pst_param->auc_station_id,
                pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID,
                WLAN_MAC_ADDR_LEN);

    *puc_len = OAL_SIZEOF(mac_cfg_staion_id_param_stru);

    return OAL_SUCC;
}
#endif

oal_uint32  mac_mib_set_bss_type_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = *((oal_int32 *)puc_param);

    mac_mib_set_DesiredBSSType(pst_mac_vap, (oal_uint8)l_value);

    return OAL_SUCC;
}


oal_uint32  mac_mib_get_bss_type_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_len, oal_uint8 *puc_param)
{
    *((oal_int32 *)puc_param) = mac_mib_get_DesiredBSSType(pst_mac_vap);

    *puc_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  mac_mib_set_ssid_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_ssid_param_stru    *pst_param;
    oal_uint8                   uc_ssid_len;
    oal_uint8                  *puc_mib_ssid;

    pst_param   = (mac_cfg_ssid_param_stru *)puc_param;
    uc_ssid_len = pst_param->uc_ssid_len;       /* 长度不包括字符串结尾'\0' */

    if (uc_ssid_len > WLAN_SSID_MAX_LEN - 1)
    {
        uc_ssid_len = WLAN_SSID_MAX_LEN - 1;
    }

    puc_mib_ssid = mac_mib_get_DesiredSSID(pst_mac_vap);

    oal_memcopy(puc_mib_ssid, pst_param->ac_ssid, uc_ssid_len);
    puc_mib_ssid[uc_ssid_len] = '\0';

    return OAL_SUCC;
}


oal_uint32  mac_mib_get_ssid_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_len, oal_uint8 *puc_param)
{
    mac_cfg_ssid_param_stru *pst_param;
    oal_uint8                uc_ssid_len;
    oal_uint8               *puc_mib_ssid;

    puc_mib_ssid = mac_mib_get_DesiredSSID(pst_mac_vap);
    uc_ssid_len  = (oal_uint8)OAL_STRLEN((oal_int8 *)puc_mib_ssid);

    pst_param = (mac_cfg_ssid_param_stru *)puc_param;

    pst_param->uc_ssid_len = uc_ssid_len;
    oal_memcopy(pst_param->ac_ssid, puc_mib_ssid, uc_ssid_len);

    *puc_len = OAL_SIZEOF(mac_cfg_ssid_param_stru);

    return OAL_SUCC;
}


oal_uint32  mac_mib_set_beacon_period_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_uint32       ul_value;

    ul_value     = *((oal_uint32 *)puc_param);

    mac_mib_set_BeaconPeriod(pst_mac_vap, ul_value);

    return OAL_SUCC;
}


oal_uint32  mac_mib_get_beacon_period_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_len, oal_uint8 *puc_param)
{
    *((oal_uint32 *)puc_param) = mac_mib_get_BeaconPeriod(pst_mac_vap);

    *puc_len = OAL_SIZEOF(oal_uint32);

    return OAL_SUCC;
}



oal_uint32  mac_mib_set_dtim_period_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value     = *((oal_int32 *)puc_param);

        mac_mib_set_dot11dtimperiod(pst_mac_vap, (oal_uint32)l_value);

    return OAL_SUCC;
}


oal_uint32  mac_mib_get_dtim_period_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_len, oal_uint8 *puc_param)
{
    *((oal_uint32 *)puc_param) = mac_mib_get_dot11dtimperiod(pst_mac_vap);

    *puc_len = OAL_SIZEOF(oal_uint32);

    return OAL_SUCC;
}


oal_uint32  mac_mib_set_shpreamble_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value     = *((oal_int32 *)puc_param);

    if (0 != l_value)
    {
        mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, OAL_TRUE);
    }
    else
    {
        mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, OAL_FALSE);
    }

    return OAL_SUCC;
}


oal_uint32  mac_mib_get_shpreamble_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = mac_mib_get_ShortPreambleOptionImplemented(pst_mac_vap);

    *((oal_int32 *)puc_param) = l_value;

    *puc_len = OAL_SIZEOF(l_value);

    return OAL_SUCC;

}
#if 0

oal_uint32  mac_mib_get_GroupReceivedFrameCount(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_len, oal_uint8 *puc_group_count)
{
    *((oal_uint32*)puc_group_count) = pst_mac_vap->pst_mib_info->st_wlan_mib_counters.ul_dot11GroupReceivedFrameCount;

    *puc_len = OAL_SIZEOF(oal_uint32);

    return OAL_SUCC;
}


oal_uint32  mac_mib_set_GroupReceivedFrameCount(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_group_count)
{
    oal_uint32      ul_value;

    ul_value = *((oal_uint32 *)puc_group_count);

    pst_mac_vap->pst_mib_info->st_wlan_mib_counters.ul_dot11GroupReceivedFrameCount = ul_value;

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS

oal_uint8 mac_vap_get_txopps(mac_vap_stru *pst_vap)
{
    return pst_vap->st_cap_flag.bit_txop_ps;
}


oal_void mac_vap_set_txopps(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->st_cap_flag.bit_txop_ps = uc_value;
}


oal_void mac_vap_update_txopps(mac_vap_stru *pst_vap, mac_user_stru *pst_user)
{
    /* 如果用户使能txop ps，则vap使能 */
    if (OAL_TRUE == pst_user->st_vht_hdl.bit_vht_txop_ps && OAL_TRUE == mac_mib_get_txopps(pst_vap))
    {
        mac_vap_set_txopps(pst_vap, OAL_TRUE);
    }

    /* 只要有一个用户支持txop ps就开启vap能力;对于AP只需要在删除user的时候判断一次是否需要设置为false */
}

#endif


#ifdef _PRE_WLAN_FEATURE_SMPS

wlan_mib_mimo_power_save_enum_uint8 mac_vap_get_smps_mode(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MIMOPowerSave;
}


wlan_mib_mimo_power_save_enum_uint8 mac_vap_get_smps_en(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru   *pst_mac_device;

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{mac_vap_get_smps_en::pst_mac_device[%d] null.}", pst_mac_vap->uc_device_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    return pst_mac_device->en_mac_smps_mode;
}


oal_void mac_vap_set_smps(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MIMOPowerSave = uc_value;
}
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD


oal_uint32  mac_vap_set_uapsd_en_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_value)
{
    pst_mac_vap->st_cap_flag.bit_uapsd = (OAL_TRUE == uc_value)?1:0;

    return OAL_SUCC;
}


oal_uint8  mac_vap_get_uapsd_en_etc(mac_vap_stru *pst_mac_vap)
{
// *puc_param  = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11APSDOptionImplemented;

    return pst_mac_vap->st_cap_flag.bit_uapsd;
}

#endif



oal_uint32 mac_vap_user_exist_etc(oal_dlist_head_stru *pst_new, oal_dlist_head_stru *pst_head)
{
    oal_dlist_head_stru      *pst_user_list_head;
    oal_dlist_head_stru      *pst_member_entry;


    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_member_entry,pst_user_list_head,pst_head)
    {
        if(pst_new == pst_member_entry)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{oal_dlist_check_head:dmac user doule add.}");
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


oal_uint32  mac_vap_add_assoc_user_etc(mac_vap_stru *pst_vap, oal_uint16 us_user_idx)
{
    mac_user_stru              *pst_user;
    oal_dlist_head_stru        *pst_dlist_head;

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    oal_hilink_white_node_stru         st_hilink_white_node;
    oal_uint32                         ul_ret;
#endif


    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{mac_vap_add_assoc_user_etc::pst_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_user = (mac_user_stru *)mac_res_get_mac_user_etc(us_user_idx);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_user))
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_add_assoc_user_etc::pst_user[%d] null.}", us_user_idx);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_user->us_user_hash_idx = MAC_CALCULATE_HASH_VALUE(pst_user->auc_user_mac_addr);

    if(OAL_SUCC == mac_vap_user_exist_etc(&(pst_user->st_user_dlist), &(pst_vap->st_mac_user_list_head)))
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_add_assoc_user_etc::user[%d] already exist.}", us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_spin_lock_bh(&pst_vap->st_cache_user_lock);

    pst_dlist_head = &(pst_vap->ast_user_hash[pst_user->us_user_hash_idx]);
#ifdef _PRE_WLAN_DFT_STAT
    (pst_vap->ul_hash_cnt)++;
#endif

    /* 加入双向hash链表表头 */
    oal_dlist_add_head(&(pst_user->st_user_hash_dlist), pst_dlist_head);

    /* 加入双向链表表头 */
    pst_dlist_head = &(pst_vap->st_mac_user_list_head);
    oal_dlist_add_head(&(pst_user->st_user_dlist), pst_dlist_head);
#ifdef _PRE_WLAN_DFT_STAT
    (pst_vap->ul_dlist_cnt)++;
#endif

    /* 更新cache user */
    oal_set_mac_addr(pst_vap->auc_cache_user_mac_addr, pst_user->auc_user_mac_addr);
    pst_vap->us_cache_user_id = us_user_idx;

    /* 记录STA模式下的与之关联的VAP的id */
    if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
    {
        mac_vap_set_assoc_id_etc(pst_vap, us_user_idx);
    }

    /* vap已关联 user个数++ */
    pst_vap->us_user_nums++;

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    /*将关联的用户添加到AP的已关联的STA列表中 */
    if (WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode)
    {
        OAL_MEMZERO(&st_hilink_white_node, OAL_SIZEOF(oal_hilink_white_node_stru));
        oal_memcopy(&(st_hilink_white_node.auc_mac), pst_user->auc_user_mac_addr, WLAN_MAC_ADDR_LEN);
        st_hilink_white_node.en_white_list_operate = OAL_WHITE_LIST_OPERATE_ADD;
        st_hilink_white_node.uc_flag |= (oal_uint8)BIT1;
        ul_ret = mac_vap_update_hilink_white_list(pst_vap, &st_hilink_white_node);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG0(0, OAM_SF_ASSOC, "{mac_vap_add_assoc_user_etc::add_sta_to_assioc_list failed.}");
        }
    }
#endif

    oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

    return OAL_SUCC;
}



oal_uint32  mac_vap_del_user_etc(mac_vap_stru *pst_vap, oal_uint16 us_user_idx)
{
    mac_user_stru          *pst_user;
    mac_user_stru          *pst_user_temp;
    oal_dlist_head_stru    *pst_hash_head;
    oal_dlist_head_stru    *pst_entry;
    oal_dlist_head_stru    *pst_dlist_tmp       = OAL_PTR_NULL;
    oal_uint32              ul_ret              = OAL_FAIL;
    oal_uint8               uc_txop_ps_user_cnt = 0;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ASSOC, "{mac_vap_del_user_etc::pst_vap null,us_user_idx is %d}", us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_spin_lock_bh(&pst_vap->st_cache_user_lock);

    /* 与cache user id对比 , 相等则清空cache user*/
    if (us_user_idx == pst_vap->us_cache_user_id)
    {
        oal_set_mac_addr_zero(pst_vap->auc_cache_user_mac_addr);
        pst_vap->us_cache_user_id = MAC_INVALID_USER_ID;
    }

    pst_user = (mac_user_stru *)mac_res_get_mac_user_etc(us_user_idx);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_user))
    {
        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_del_user_etc::pst_user[%d] null.}", us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_user_set_asoc_state_etc(pst_user, MAC_USER_STATE_BUTT);

    if(pst_user->us_user_hash_idx >= MAC_VAP_USER_HASH_MAX_VALUE)
    {
        /*ADD USER命令丢失，或者重复删除User都可能进入此分支。*/
        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_del_user_etc::hash idx invaild %u}", pst_user->us_user_hash_idx);
        return OAL_FAIL;
    }

    pst_hash_head = &(pst_vap->ast_user_hash[pst_user->us_user_hash_idx]);

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, pst_hash_head)
    {
        pst_user_temp = (mac_user_stru *)OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_hash_dlist);
        /*lint -save -e774 */
        if (OAL_PTR_NULL == pst_user_temp)
        {
            OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_del_user_etc::pst_user_temp null,us_user_idx is %d}", us_user_idx);

            continue;
        }
        /*lint -restore */

        if (pst_user_temp->st_vht_hdl.bit_vht_txop_ps)
        {
            uc_txop_ps_user_cnt++;
        }

        if (!oal_compare_mac_addr(pst_user->auc_user_mac_addr, pst_user_temp->auc_user_mac_addr))
        {
            oal_dlist_delete_entry(pst_entry);

            /* 从双向链表中拆掉 */
            oal_dlist_delete_entry(&(pst_user->st_user_dlist));

            oal_dlist_delete_entry(&(pst_user->st_user_hash_dlist));
            ul_ret = OAL_SUCC;

#ifdef _PRE_WLAN_DFT_STAT
            (pst_vap->ul_hash_cnt)--;
            (pst_vap->ul_dlist_cnt)--;
#endif
            /* 初始化相应成员 */
            pst_user->us_user_hash_idx = 0xffff;
            pst_user->us_assoc_id      = us_user_idx;
            pst_user->en_is_multi_user = OAL_FALSE;
            OAL_MEMZERO(pst_user->auc_user_mac_addr, WLAN_MAC_ADDR_LEN);
            pst_user->uc_vap_id        = 0xff;
            pst_user->uc_device_id     = 0xff;
            pst_user->uc_chip_id       = 0xff;
            pst_user->en_user_asoc_state = MAC_USER_STATE_BUTT;
        }
    }

    /* 没有关联的用户则去初始化vap能力 */
    if (0 == uc_txop_ps_user_cnt)
    {
#ifdef _PRE_WLAN_FEATURE_TXOPPS
        mac_vap_set_txopps(pst_vap, OAL_FALSE);
#endif
    }

    if (OAL_SUCC == ul_ret)
    {
        /* vap已关联 user个数-- */
        if (pst_vap->us_user_nums)
        {
            pst_vap->us_user_nums--;
        }
        /* STA模式下将关联的VAP的id置为非法值 */
        if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
        {
            mac_vap_set_assoc_id_etc(pst_vap, MAC_INVALID_USER_ID);
        }

        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

        return OAL_SUCC;
    }

    oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);

    OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_del_user_etc::delete user failed,user idx is %d.}", us_user_idx);
    return OAL_FAIL;
}


oal_uint32  mac_vap_find_user_by_macaddr_etc( mac_vap_stru *pst_vap, oal_uint8 *puc_sta_mac_addr, oal_uint16 *pus_user_idx)
{
    mac_user_stru              *pst_mac_user;
    oal_uint32                  ul_user_hash_value;
    oal_dlist_head_stru        *pst_entry;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_vap)
                  || (OAL_PTR_NULL == puc_sta_mac_addr)
                  || ((OAL_PTR_NULL == pus_user_idx))))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_vap_find_user_by_macaddr_etc::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 1.本函数*pus_user_idx先初始化为无效值，防止调用者没有初始化，可能出现使用返回值异常;
       2.根据查找结果刷新*pus_user_idx值，如果是有效，返回SUCC,无效MAC_INVALID_USER_ID返回FAIL;
       3.调用函数根据首先根据本函数返回值做处理，其次根据*pus_user_idx进行其他需要的判断和操作 */
    *pus_user_idx = MAC_INVALID_USER_ID;

    if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
    {
        pst_mac_user = (mac_user_stru *)mac_res_get_mac_user_etc(pst_vap->us_assoc_vap_id);
        if (OAL_PTR_NULL == pst_mac_user)
        {
            return OAL_FAIL;
        }

        if (!oal_compare_mac_addr(pst_mac_user->auc_user_mac_addr, puc_sta_mac_addr))
        {
            *pus_user_idx = pst_vap->us_assoc_vap_id;
        }
    }
    else
    {
    #if !(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) || defined(_PRE_DEBUG_MODE)
        oal_spin_lock_bh(&pst_vap->st_cache_user_lock);
    #endif

        /* 与cache user对比 , 相等则直接返回cache user id*/
        if (!oal_compare_mac_addr(pst_vap->auc_cache_user_mac_addr, puc_sta_mac_addr))
        {
            /* 用户删除后，user macaddr和cache user macaddr地址均为0，但实际上用户已经删除，此时user id无效 */
            *pus_user_idx = pst_vap->us_cache_user_id;
        }
        else
        {
            ul_user_hash_value = MAC_CALCULATE_HASH_VALUE(puc_sta_mac_addr);

            OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_vap->ast_user_hash[ul_user_hash_value]))
            {
                pst_mac_user = (mac_user_stru *)OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_hash_dlist);
                /*lint -save -e774 */
                if (OAL_PTR_NULL == pst_mac_user)
                {
                    OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_find_user_by_macaddr_etc::pst_mac_user null.}");
                    continue;
                }
                /*lint -restore */

                /* 相同的MAC地址 */
                if (!oal_compare_mac_addr(pst_mac_user->auc_user_mac_addr, puc_sta_mac_addr))
                {
                   *pus_user_idx = pst_mac_user->us_assoc_id;
                    /*更新cache user*/
                    oal_set_mac_addr(pst_vap->auc_cache_user_mac_addr, pst_mac_user->auc_user_mac_addr);
                    pst_vap->us_cache_user_id = pst_mac_user->us_assoc_id;
                }
            }
        }
    #if !(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) || defined(_PRE_DEBUG_MODE)
        oal_spin_unlock_bh(&pst_vap->st_cache_user_lock);
    #endif
    }

    /* user id有效的话，返回SUCC给调用者处理，user id无效的话，返回user id为MAC_INVALID_USER_ID，并返回查找结果FAIL给调用者处理 */
    if(*pus_user_idx == MAC_INVALID_USER_ID)
    {
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32  mac_device_find_user_by_macaddr_etc(
                oal_uint8            uc_device_id,
                oal_uint8           *puc_sta_mac_addr,
                oal_uint16          *pus_user_idx)
{
    mac_device_stru            *pst_device;
    mac_vap_stru               *pst_mac_vap;
    oal_uint8                   uc_vap_idx;
    oal_uint32                  ul_ret;

    /* 获取device */
    pst_device = mac_res_get_dev_etc(uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "mac_device_find_user_by_macaddr_etc:get_dev return null ");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 对device下的所有vap进行遍历 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
    {

        /* 配置vap不需要处理 */
        if (pst_device->auc_vap_id[uc_vap_idx] == pst_device->uc_cfg_vap_id)
        {
            continue;
        }

        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }

        /* 只处理AP模式 */
        if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
        {
            continue;
        }

        ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_sta_mac_addr, pus_user_idx);
        if (OAL_SUCC == ul_ret)
        {
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}


oal_uint32  mac_chip_find_user_by_macaddr(
                oal_uint8            uc_chip_id,
                oal_uint8           *puc_sta_mac_addr,
                oal_uint16          *pus_user_idx)
{
    mac_chip_stru              *pst_mac_chip;
    oal_uint8                   uc_device_idx;
    oal_uint32                  ul_ret;

    /* 获取device */
    pst_mac_chip = mac_res_get_mac_chip(uc_chip_id);
    if (OAL_PTR_NULL == pst_mac_chip)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_chip_find_user_by_macaddr:get_chip return nul!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_device_idx = 0; uc_device_idx < pst_mac_chip->uc_device_nums; uc_device_idx++)
    {
       // ul_ret = mac_device_find_user_by_macaddr_etc(uc_device_idx, puc_sta_mac_addr, pus_user_idx);
        ul_ret = mac_device_find_user_by_macaddr_etc(pst_mac_chip->auc_device_id[uc_device_idx], puc_sta_mac_addr, pus_user_idx);
        if(OAL_SUCC == ul_ret)
        {
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}

oal_uint32  mac_board_find_user_by_macaddr(
                oal_uint8           *puc_sta_mac_addr,
                oal_uint16          *pus_user_idx)
{
    oal_uint8                   uc_chip_idx;
    oal_uint32                  ul_ret;

    /* 遍历board下所有chip */
    for(uc_chip_idx = 0; uc_chip_idx < WLAN_CHIP_MAX_NUM_PER_BOARD; uc_chip_idx++)
    {
        ul_ret = mac_chip_find_user_by_macaddr(uc_chip_idx, puc_sta_mac_addr, pus_user_idx);
        if(OAL_SUCC == ul_ret)
        {
            return OAL_SUCC;
        }
    }

    return OAL_FAIL;
}

#if 0

oal_uint32  mac_vap_update_user(mac_vap_stru *pst_vap, oal_uint32   ul_user_dix)
{
    /* 非OFFLOAD方案下不需要实现 */

    return OAL_SUCC;
}
#endif

oal_uint32  mac_vap_init_wme_param_etc(mac_vap_stru *pst_mac_vap)
{
    OAL_CONST mac_wme_param_stru   *pst_wmm_param;
    OAL_CONST mac_wme_param_stru   *pst_wmm_param_sta;
    oal_uint8                       uc_ac_type;

    pst_wmm_param = mac_get_wmm_cfg_etc(pst_mac_vap->en_vap_mode);
    if(OAL_PTR_NULL == pst_wmm_param)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_ac_type = 0; uc_ac_type < WLAN_WME_AC_BUTT; uc_ac_type++)
    {
        /* VAP自身的EDCA参数 */
        mac_mib_set_QAPEDCATableIndex(pst_mac_vap, uc_ac_type, uc_ac_type + 1);  /* 注: 协议规定取值1 2 3 4 */
        mac_mib_set_QAPEDCATableAIFSN(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].ul_aifsn);
        mac_mib_set_QAPEDCATableCWmin(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].ul_logcwmin);
        mac_mib_set_QAPEDCATableCWmax(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].ul_logcwmax);
        mac_mib_set_QAPEDCATableTXOPLimit(pst_mac_vap, uc_ac_type, pst_wmm_param[uc_ac_type].ul_txop_limit);
    }

    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        /* AP模式时广播给STA的EDCA参数，只在AP模式需要初始化此值，使用WLAN_VAP_MODE_BUTT， */
        pst_wmm_param_sta = mac_get_wmm_cfg_etc(WLAN_VAP_MODE_BUTT);

        for (uc_ac_type = 0; uc_ac_type < WLAN_WME_AC_BUTT; uc_ac_type++)
        {
            mac_mib_set_EDCATableIndex(pst_mac_vap, uc_ac_type, uc_ac_type + 1);  /* 注: 协议规定取值1 2 3 4 */
            mac_mib_set_EDCATableAIFSN(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].ul_aifsn);
            mac_mib_set_EDCATableCWmin(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].ul_logcwmin);
            mac_mib_set_EDCATableCWmax(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].ul_logcwmax);
            mac_mib_set_EDCATableTXOPLimit(pst_mac_vap, uc_ac_type, pst_wmm_param_sta[uc_ac_type].ul_txop_limit);
        }
    }

    return OAL_SUCC;
}
#if 0

oal_uint32  mac_vap_get_user_wme_info(
                mac_vap_stru               *pst_vap,
                wlan_wme_ac_type_enum_uint8 en_wme_type,
                mac_wme_param_stru         *pst_wme_info)
{
    if (OAL_PTR_NULL == pst_vap || OAL_PTR_NULL == pst_wme_info)
    {
        MAC_ERR_LOG(0, "mac_get_user_wme_info: input pointers are null!");
        OAM_ERROR_LOG0(0, OAM_SF_WMM, "{mac_vap_get_user_wme_info::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wme_info->ul_aifsn = pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableAIFSN;
    pst_wme_info->ul_logcwmax = pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableCWmax;
    pst_wme_info->ul_logcwmin = pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableCWmin;
    pst_wme_info->ul_txop_limit = pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableTXOPLimit;

    return OAL_SUCC;
}


oal_uint32  mac_vap_set_user_wme_info(
                mac_vap_stru               *pst_vap,
                wlan_wme_ac_type_enum_uint8 en_wme_type,
                mac_wme_param_stru         *pst_wme_info)
{
    if (OAL_PTR_NULL == pst_vap || OAL_PTR_NULL == pst_wme_info)
    {
        MAC_ERR_LOG(0, "mac_get_user_wme_info: input pointers are null!");
        OAM_ERROR_LOG0(0, OAM_SF_WMM, "{mac_vap_set_user_wme_info::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableAIFSN = pst_wme_info->ul_aifsn;
    pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableCWmax = pst_wme_info->ul_logcwmax;
    pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableCWmin = pst_wme_info->ul_logcwmin;
    pst_vap->pst_mib_info->ast_wlan_mib_edca[en_wme_type].ul_dot11EDCATableTXOPLimit = pst_wme_info->ul_txop_limit;

    return OAL_SUCC;
}
#endif






oal_void  mac_vap_init_mib_11n(mac_vap_stru  *pst_mac_vap)
{
    wlan_mib_ieee802dot11_stru    *pst_mib_info;
    mac_device_stru               *pst_dev;

    pst_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_dev)
    {
        MAC_WARNING_LOG(pst_mac_vap->uc_vap_id, "mac_vap_init_mib_11n: pst_dev is null ptr");
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_mib_11n::pst_dev null.}");

        return;
    }

    pst_mib_info = pst_mac_vap->pst_mib_info;

    mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_LDPCCodingOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_LDPC(pst_dev));
    mac_mib_set_TxSTBCOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_TXSTBC(pst_dev));

    /* LDPC 和 STBC Activated能力初始化为device的能力，用于STA的协议协商，到关联请求时再更新该mib值  */
    mac_mib_set_LDPCCodingOptionActivated(pst_mac_vap, MAC_DEVICE_GET_CAP_LDPC(pst_dev));
    mac_mib_set_TxSTBCOptionActivated(pst_mac_vap, HT_TX_STBC_DEFAULT_VALUE);

    mac_mib_set_2GFortyMHzOperationImplemented(pst_mac_vap, (oal_bool_enum_uint8)(!pst_mac_vap->st_cap_flag.bit_disable_2ght40));
    mac_mib_set_5GFortyMHzOperationImplemented(pst_mac_vap, OAL_TRUE);

#ifdef _PRE_WLAN_FEATURE_SMPS
    /* 开启SMPS宏,能力需要按照mac device能力来刷，后续m2s会动态刷新 */
    mac_mib_set_smps(pst_mac_vap, MAC_DEVICE_GET_MODE_SMPS(pst_dev));
#endif

    mac_mib_set_HTGreenfieldOptionImplemented(pst_mac_vap, HT_GREEN_FILED_DEFAULT_VALUE);
    mac_mib_set_ShortGIOptionInTwentyImplemented(pst_mac_vap, OAL_TRUE);
    mac_mib_set_2GShortGIOptionInFortyImplemented(pst_mac_vap, (oal_bool_enum_uint8)(!pst_mac_vap->st_cap_flag.bit_disable_2ght40));
    mac_mib_set_5GShortGIOptionInFortyImplemented(pst_mac_vap, OAL_TRUE);

    mac_mib_set_RxSTBCOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_RXSTBC(pst_dev));
    //pst_mib_info->st_phy_ht.ul_dot11NumberOfSpatialStreamsImplemented   = 2;
    pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MaxAMSDULength         = 0;

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
    mac_mib_set_lsig_txop_protection(pst_mac_vap, OAL_FALSE);
#else
    mac_mib_set_lsig_txop_protection(pst_mac_vap, OAL_TRUE);
#endif

    mac_mib_set_max_ampdu_len_exponent(pst_mac_vap, 3);
    mac_mib_set_min_mpdu_start_spacing(pst_mac_vap, 5);
    mac_mib_set_pco_option_implemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_transition_time(pst_mac_vap, 3);
    mac_mib_set_mcs_fdbk(pst_mac_vap, OAL_FALSE);
    mac_mib_set_htc_sup(pst_mac_vap, OAL_FALSE);
    mac_mib_set_rd_rsp(pst_mac_vap, OAL_FALSE);

    mac_mib_set_ReceiveNDPOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_TransmitNDPOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ImplicitTransmitBeamformingOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_CalibrationOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ExplicitCSITransmitBeamformingOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ExplicitNonCompressedBeamformingMatrixOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ExplicitTransmitBeamformingCSIFeedbackOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ExplicitNonCompressedBeamformingFeedbackOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_NumberBeamFormingCSISupportAntenna(pst_mac_vap, 0);
    mac_mib_set_NumberNonCompressedBeamformingMatrixSupportAntenna(pst_mac_vap, 0);

    /* 天线选择能力信息 */
    mac_mib_set_AntennaSelectionOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_TransmitExplicitCSIFeedbackASOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_TransmitIndicesFeedbackASOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ExplicitCSIFeedbackASOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ReceiveAntennaSelectionOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_TransmitSoundingPPDUOptionImplemented(pst_mac_vap, OAL_FALSE);

    /* obss信息 */
    mac_mib_init_obss_scan(pst_mac_vap);

    /*默认使用2040共存*/
    mac_mib_init_2040(pst_mac_vap);

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.mac_vap_init_mib_11n_cb)
    {
        g_st_mac_vap_rom_cb.mac_vap_init_mib_11n_cb(pst_mac_vap);
    }
}





oal_void  mac_vap_init_11ac_mcs_singlenss(mac_vap_stru           *pst_mac_vap,
                                                     wlan_channel_bandwidth_enum_uint8    en_bandwidth)
{
    mac_tx_max_mcs_map_stru         *pst_tx_max_mcs_map;
    mac_rx_max_mcs_map_stru         *pst_rx_max_mcs_map;

    /* 获取mib值指针 */
    pst_rx_max_mcs_map = (mac_rx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_rx_mcs_map(pst_mac_vap));
    pst_tx_max_mcs_map = (mac_tx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_tx_mcs_map(pst_mac_vap));

    /* 20MHz带宽的情况下，支持MCS0-MCS8 */
    if (WLAN_BAND_WIDTH_20M == en_bandwidth)
    {
#ifdef _PRE_WLAN_FEATURE_11AC_20M_MCS9
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
#else
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
#endif
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_20M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_20M_11AC);
    }

    /* 40MHz带宽的情况下，支持MCS0-MCS9 */
    else if ((WLAN_BAND_WIDTH_40MINUS == en_bandwidth) || (WLAN_BAND_WIDTH_40PLUS == en_bandwidth))
    {
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_40M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_40M_11AC);
    }

    /* 80MHz带宽的情况下，支持MCS0-MCS9 */
    else if ((WLAN_BAND_WIDTH_80MINUSMINUS == en_bandwidth)
             || (WLAN_BAND_WIDTH_80MINUSPLUS == en_bandwidth)
             || (WLAN_BAND_WIDTH_80PLUSMINUS == en_bandwidth)
             || (WLAN_BAND_WIDTH_80PLUSPLUS == en_bandwidth))
    {
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_80M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_80M_11AC);
    }

    if (g_st_mac_vap_rom_cb.mac_vap_init_11ac_mcs_singlenss_cb != OAL_PTR_NULL)
    {
        g_st_mac_vap_rom_cb.mac_vap_init_11ac_mcs_singlenss_cb(pst_mac_vap, en_bandwidth);
    }
}


oal_void  mac_vap_init_11ac_mcs_doublenss(mac_vap_stru *pst_mac_vap,
                wlan_channel_bandwidth_enum_uint8    en_bandwidth)
{
    mac_tx_max_mcs_map_stru         *pst_tx_max_mcs_map;
    mac_rx_max_mcs_map_stru         *pst_rx_max_mcs_map;

    /* 获取mib值指针 */
    pst_rx_max_mcs_map = (mac_rx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_rx_mcs_map(pst_mac_vap));
    pst_tx_max_mcs_map = (mac_tx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_tx_mcs_map(pst_mac_vap));

    /* 20MHz带宽的情况下，支持MCS0-MCS8 */
    if (WLAN_BAND_WIDTH_20M == en_bandwidth)
    {
#ifdef _PRE_WLAN_FEATURE_11AC_20M_MCS9
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
#else
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS8_11AC_EACH_NSS;
#endif
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_20M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_20M_11AC);
    }

    /* 40MHz带宽的情况下，支持MCS0-MCS9 */
    else if ((WLAN_BAND_WIDTH_40MINUS == en_bandwidth) || (WLAN_BAND_WIDTH_40PLUS == en_bandwidth))
    {
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_40M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_40M_11AC);
    }

    /* 80MHz带宽的情况下，支持MCS0-MCS9 */
    else if ((WLAN_BAND_WIDTH_80MINUSMINUS == en_bandwidth)
             || (WLAN_BAND_WIDTH_80MINUSPLUS== en_bandwidth)
             || (WLAN_BAND_WIDTH_80PLUSMINUS== en_bandwidth)
             || (WLAN_BAND_WIDTH_80PLUSPLUS== en_bandwidth))
    {
        pst_rx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_rx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_1ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        pst_tx_max_mcs_map->us_max_mcs_2ss = MAC_MAX_SUP_MCS9_11AC_EACH_NSS;
        mac_mib_set_us_rx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_80M_11AC);
        mac_mib_set_us_tx_highest_rate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_80M_11AC);
    }

    if (g_st_mac_vap_rom_cb.mac_vap_init_11ac_mcs_doublenss_cb != OAL_PTR_NULL)
    {
        g_st_mac_vap_rom_cb.mac_vap_init_11ac_mcs_doublenss_cb(pst_mac_vap, en_bandwidth);
    }
}


oal_void  mac_vap_init_mib_11ac(mac_vap_stru  *pst_mac_vap)
{
    mac_device_stru                 *pst_mac_dev;

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_mib_11ac::pst_mac_dev[%d] null.}", pst_mac_vap->uc_device_id);

        return;
    }

    mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_TRUE);

    mac_mib_set_maxmpdu_length(pst_mac_vap, WLAN_MIB_VHT_MPDU_7991);

    mac_mib_set_VHTLDPCCodingOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_LDPC(pst_mac_dev));
    mac_mib_set_VHTShortGIOptionIn80Implemented(pst_mac_vap, OAL_TRUE);
    mac_mib_set_VHTChannelWidthOptionImplemented(pst_mac_vap, mac_device_trans_bandwith_to_vht_capinfo(MAC_DEVICE_GET_CAP_BW(pst_mac_dev)));

    //暂时以HAL device0的能力代替，考虑dbdc以后还需要修改 TODO
    if (MAC_DEVICE_GET_CAP_BW(pst_mac_dev) >= WLAN_BW_CAP_160M)
    {
        mac_mib_set_VHTShortGIOptionIn160and80p80Implemented(pst_mac_vap, WLAN_HAL0_VHT_SGI_SUPP_160_80P80);
    }
    else
    {
        mac_mib_set_VHTShortGIOptionIn160and80p80Implemented(pst_mac_vap, OAL_FALSE);
    }

    mac_mib_set_VHTTxSTBCOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_TXSTBC(pst_mac_dev));
    mac_mib_set_VHTRxSTBCOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_RXSTBC(pst_mac_dev));

    /* TxBf相关 */
#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_mib_set_VHTSUBeamformerOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_SUBFER(pst_mac_dev));
    mac_mib_set_VHTSUBeamformeeOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_SUBFEE(pst_mac_dev));
    mac_mib_set_VHTNumberSoundingDimensions(pst_mac_vap, MAC_DEVICE_GET_NSS_NUM(pst_mac_dev));
    mac_mib_set_VHTMUBeamformerOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_MUBFER(pst_mac_dev));
    mac_mib_set_VHTMUBeamformeeOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_MUBFEE(pst_mac_dev));
    mac_mib_set_VHTBeamformeeNTxSupport(pst_mac_vap, VHT_BFEE_NTX_SUPP_STS_CAP);
#else
    mac_mib_set_VHTSUBeamformerOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_VHTSUBeamformeeOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_VHTNumberSoundingDimensions(pst_mac_vap, WLAN_SINGLE_NSS);
    mac_mib_set_VHTMUBeamformerOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_VHTMUBeamformeeOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_VHTBeamformeeNTxSupport(pst_mac_vap, 1);
#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS
    mac_mib_set_txopps(pst_mac_vap, OAL_TRUE);
#endif

    mac_mib_set_vht_ctrl_field_cap(pst_mac_vap, OAL_FALSE);

    mac_mib_set_vht_max_rx_ampdu_factor(pst_mac_vap, 5);                        /* 2^(13+factor)-1字节 */

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    mac_mib_set_OperatingModeNotificationImplemented(pst_mac_vap, OAL_TRUE);
#endif
}

#ifdef _PRE_WLAN_FEATURE_11AX

oal_void  mac_vap_rom_init_mib_11ax(mac_vap_stru  *pst_mac_vap)
{
    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.init_11ax_mib_cb)
    {
        g_st_mac_vap_rom_cb.init_11ax_mib_cb(pst_mac_vap);
    }
}
#endif


oal_void  mac_vap_init_mib_11i(mac_vap_stru  *pst_vap)
{
    mac_mib_set_rsnaactivated(pst_vap, OAL_FALSE);
    mac_mib_set_dot11RSNAMFPR(pst_vap, OAL_FALSE);
    mac_mib_set_dot11RSNAMFPC(pst_vap, OAL_FALSE);
    mac_mib_set_pre_auth_actived(pst_vap, OAL_FALSE);
    mac_mib_set_privacyinvoked(pst_vap, OAL_FALSE);
    mac_mib_init_rsnacfg_suites(pst_vap);
    mac_mib_set_rsnacfg_gtksareplaycounters(pst_vap, 0);
    mac_mib_set_rsnacfg_ptksareplaycounters(pst_vap, 0);
}

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)
oal_void  mac_vap_init_mib_11k(mac_vap_stru  *pst_vap)
{
#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || defined(_PRE_PRODUCT_ID_HI110X_DEV))
    if(!IS_LEGACY_STA(pst_vap))
    {
        return;
    }
#endif
    mac_mib_set_dot11RadioMeasurementActivated(pst_vap, OAL_TRUE);
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    if(WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
#endif
    {
        mac_mib_set_dot11RMBeaconActiveMeasurementActivated(pst_vap, OAL_TRUE);
        mac_mib_set_dot11RMBeaconPassiveMeasurementActivated(pst_vap, OAL_TRUE);
        mac_mib_set_dot11RMBeaconTableMeasurementActivated(pst_vap, OAL_TRUE);
    }
#ifdef _PRE_WLAN_FEATURE_11K
    mac_mib_set_dot11RMLinkMeasurementActivated(pst_vap, OAL_TRUE);
#endif
    //pst_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11RMNeighborReportActivated             = OAL_TRUE;

    //pst_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11RMMaxMeasurementDuration              = 1200; //1.2s

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    if(WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
    {
        mac_mib_set_dot11RMNeighborReportActivated(pst_vap, OAL_TRUE);
        mac_mib_set_dot11RMChannelLoadMeasurementActivated(pst_vap, OAL_TRUE);
    }
#endif

}
#endif

oal_void mac_vap_init_legacy_rates(mac_vap_stru *pst_vap,mac_data_rate_stru *pst_rates)
{
    oal_uint8                      uc_rate_index;
    oal_uint8                      uc_curr_rate_index = 0;
    mac_data_rate_stru            *puc_orig_rate;
    mac_data_rate_stru            *puc_curr_rate;
    oal_uint8                      uc_rates_num;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11A;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = MAC_NUM_BR_802_11A;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11A;
    pst_vap->st_curr_sup_rates.uc_min_rate          = 6;
    pst_vap->st_curr_sup_rates.uc_max_rate          = 24;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++)
    {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_curr_rate_index]);

        /* Basic Rates */
        if ((puc_orig_rate->uc_mbps == 6)
            || (puc_orig_rate->uc_mbps == 12)
            || (puc_orig_rate->uc_mbps == 24))
        {
            oal_memcopy(puc_curr_rate,puc_orig_rate,sizeof(mac_data_rate_stru));
            puc_curr_rate->uc_mac_rate |= 0x80;
            uc_curr_rate_index++;
        }

        /* Non-basic rates */
        else if ((puc_orig_rate->uc_mbps== 9)
                || (puc_orig_rate->uc_mbps == 18)
                || (puc_orig_rate->uc_mbps == 36)
                || (puc_orig_rate->uc_mbps == 48)
                || (puc_orig_rate->uc_mbps == 54))
        {
            oal_memcopy(puc_curr_rate,puc_orig_rate,sizeof(mac_data_rate_stru));
            uc_curr_rate_index++;
        }

        if (uc_curr_rate_index == pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates)
        {
            break;
        }
    }
}


oal_void mac_vap_init_11b_rates(mac_vap_stru *pst_vap,mac_data_rate_stru *pst_rates)
{
    oal_uint8                      uc_rate_index;
    oal_uint8                      uc_curr_rate_index = 0;
    mac_data_rate_stru            *puc_orig_rate;
    mac_data_rate_stru            *puc_curr_rate;
    oal_uint8                      uc_rates_num;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11B;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = 0;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11B;
    pst_vap->st_curr_sup_rates.uc_min_rate          = 1;
    pst_vap->st_curr_sup_rates.uc_max_rate          = 2;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++)
    {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_curr_rate_index]);

        /*  Basic Rates  */
        if ((puc_orig_rate->uc_mbps == 1) || (puc_orig_rate->uc_mbps == 2)
           || ((WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode) && ((puc_orig_rate->uc_mbps == 5) || (puc_orig_rate->uc_mbps == 11))))
        {
            pst_vap->st_curr_sup_rates.uc_br_rate_num++;
            oal_memcopy(puc_curr_rate,puc_orig_rate,sizeof(mac_data_rate_stru));
            puc_curr_rate->uc_mac_rate |= 0x80;
            uc_curr_rate_index++;
        }

        /* Non-basic rates */
        else if ((WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode)
                && ((puc_orig_rate->uc_mbps == 5) || (puc_orig_rate->uc_mbps == 11)))
        {
            oal_memcopy(puc_curr_rate,puc_orig_rate,sizeof(mac_data_rate_stru));
            uc_curr_rate_index++;
        }
        else
        {
            continue;
        }

        if (uc_curr_rate_index == pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates)
        {
            break;
        }
    }
}


oal_void mac_vap_init_11g_mixed_one_rates(mac_vap_stru *pst_vap,mac_data_rate_stru *pst_rates)
{
    oal_uint8                      uc_rate_index;
    mac_data_rate_stru            *puc_orig_rate;
    mac_data_rate_stru            *puc_curr_rate;
    oal_uint8                      uc_rates_num;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11G_MIXED;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = MAC_NUM_BR_802_11G_MIXED_ONE;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11G_MIXED_ONE;
    pst_vap->st_curr_sup_rates.uc_min_rate          = 1;
    pst_vap->st_curr_sup_rates.uc_max_rate          = 11;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++)
    {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_rate_index]);

        oal_memcopy(puc_curr_rate,puc_orig_rate,sizeof(mac_data_rate_stru));

        /* Basic Rates */
        if ((puc_orig_rate->uc_mbps== 1)
            || (puc_orig_rate->uc_mbps == 2)
            || (puc_orig_rate->uc_mbps == 5)
            || (puc_orig_rate->uc_mbps == 11))
        {
            puc_curr_rate->uc_mac_rate |= 0x80;
        }
    }
}


oal_void mac_vap_init_11g_mixed_two_rates(mac_vap_stru *pst_vap,mac_data_rate_stru *pst_rates)
{
    oal_uint8                      uc_rate_index;
    mac_data_rate_stru            *puc_orig_rate;
    mac_data_rate_stru            *puc_curr_rate;
    oal_uint8                      uc_rates_num;

    /* 初始化速率集 */
    uc_rates_num = MAC_DATARATES_PHY_80211G_NUM;

    /* 初始化速率个数，基本速率个数，非基本速率个数 */
    pst_vap->st_curr_sup_rates.st_rate.uc_rs_nrates = MAC_NUM_DR_802_11G_MIXED;
    pst_vap->st_curr_sup_rates.uc_br_rate_num       = MAC_NUM_BR_802_11G_MIXED_TWO;
    pst_vap->st_curr_sup_rates.uc_nbr_rate_num      = MAC_NUM_NBR_802_11G_MIXED_TWO;
    pst_vap->st_curr_sup_rates.uc_min_rate          = 1;
    pst_vap->st_curr_sup_rates.uc_max_rate          = 24;

    /* 将速率拷贝到VAP结构体下的速率集中 */
    for (uc_rate_index = 0; uc_rate_index < uc_rates_num; uc_rate_index++)
    {
        puc_orig_rate = &pst_rates[uc_rate_index];
        puc_curr_rate = &(pst_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_rate_index]);

        oal_memcopy(puc_curr_rate,puc_orig_rate,sizeof(mac_data_rate_stru));

        /* Basic Rates */
        if ((puc_orig_rate->uc_mbps== 1)
            || (puc_orig_rate->uc_mbps == 2)
            || (puc_orig_rate->uc_mbps == 5)
            || (puc_orig_rate->uc_mbps == 11)
            || (puc_orig_rate->uc_mbps == 6)
            || (puc_orig_rate->uc_mbps == 12)
            || (puc_orig_rate->uc_mbps == 24))
        {
            puc_curr_rate->uc_mac_rate |= 0x80;
        }
    }
}


oal_void mac_vap_init_11n_rates(mac_vap_stru *pst_mac_vap,mac_device_stru *pst_mac_dev)
{

    /* 初始化速率集 */

    /* MCS相关MIB值初始化 */
    mac_mib_set_TxMCSSetDefined(pst_mac_vap, OAL_TRUE);
    mac_mib_set_TxRxMCSSetNotEqual(pst_mac_vap, OAL_FALSE);
    mac_mib_set_TxUnequalModulationSupported(pst_mac_vap, OAL_FALSE);

    /* 将MIB值的MCS MAP清零 */
    OAL_MEMZERO(mac_mib_get_SupportedMCSTx(pst_mac_vap), WLAN_HT_MCS_BITMASK_LEN);
    OAL_MEMZERO(mac_mib_get_SupportedMCSRx(pst_mac_vap), WLAN_HT_MCS_BITMASK_LEN);

    /* 1个空间流 */
    if (WLAN_SINGLE_NSS == MAC_DEVICE_GET_NSS_NUM(pst_mac_dev))
    {
        mac_mib_set_TxMaximumNumberSpatialStreamsSupported(pst_mac_vap, 1);
        mac_mib_set_SupportedMCSRxValue(pst_mac_vap, 0, 0xFF); /* 支持 RX MCS 0-7，8位全置为1*/
        mac_mib_set_SupportedMCSTxValue(pst_mac_vap, 0, 0xFF); /* 支持 TX MCS 0-7，8位全置为1*/

        mac_mib_set_HighestSupportedDataRate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_20M_11N);

        if ((WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth) || (WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth))
        {
            /* 40M 支持MCS32 */
            mac_mib_set_SupportedMCSRxValue(pst_mac_vap, 4, 0x01);  /* 支持 RX MCS 32,最后一位为1 */
            mac_mib_set_SupportedMCSTxValue(pst_mac_vap, 4, 0x01);  /* 支持 RX MCS 32,最后一位为1 */
            mac_mib_set_HighestSupportedDataRate(pst_mac_vap, MAC_MAX_RATE_SINGLE_NSS_40M_11N);
        }
    }

    /* 2个空间流 */
    else if (WLAN_DOUBLE_NSS == MAC_DEVICE_GET_NSS_NUM(pst_mac_dev))
    {
        mac_mib_set_TxMaximumNumberSpatialStreamsSupported(pst_mac_vap, 2);
        mac_mib_set_SupportedMCSRxValue(pst_mac_vap, 0, 0xFF);  /* 支持 RX MCS 0-7，8位全置为1*/
        mac_mib_set_SupportedMCSRxValue(pst_mac_vap, 1, 0xFF);  /* 支持 RX MCS 8-15，8位全置为1*/

        mac_mib_set_SupportedMCSTxValue(pst_mac_vap, 0, 0xFF);  /* 支持 TX MCS 0-7，8位全置为1*/
        mac_mib_set_SupportedMCSTxValue(pst_mac_vap, 1, 0xFF);  /* 支持 TX MCS 8-15，8位全置为1*/

        mac_mib_set_HighestSupportedDataRate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_20M_11N);

        if ((WLAN_BAND_WIDTH_40MINUS == pst_mac_vap->st_channel.en_bandwidth) || (WLAN_BAND_WIDTH_40PLUS == pst_mac_vap->st_channel.en_bandwidth))
        {
            /* 40M 支持的最大速率为300M */
            mac_mib_set_SupportedMCSRxValue(pst_mac_vap, 4, 0x01);  /* 支持 RX MCS 32,最后一位为1 */
            mac_mib_set_SupportedMCSTxValue(pst_mac_vap, 4, 0x01);  /* 支持 RX MCS 32,最后一位为1 */
            mac_mib_set_HighestSupportedDataRate(pst_mac_vap, MAC_MAX_RATE_DOUBLE_NSS_40M_11N);
        }
    }

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.init_11n_rates_cb)
    {
        g_st_mac_vap_rom_cb.init_11n_rates_cb(pst_mac_vap, pst_mac_dev);
    }
}


oal_void mac_vap_init_11ac_rates(mac_vap_stru *pst_mac_vap, mac_device_stru *pst_mac_dev)
{
    /* 先将TX RX MCSMAP初始化为所有空间流都不支持 0xFFFF*/
    mac_mib_set_vht_rx_mcs_map(pst_mac_vap, 0xFFFF);
    mac_mib_set_vht_tx_mcs_map(pst_mac_vap, 0xFFFF);

    if (WLAN_SINGLE_NSS == MAC_DEVICE_GET_NSS_NUM(pst_mac_dev))
    {
        /* 1个空间流的情况 */
        mac_vap_init_11ac_mcs_singlenss(pst_mac_vap, pst_mac_vap->st_channel.en_bandwidth);
    }
    else if (WLAN_DOUBLE_NSS == MAC_DEVICE_GET_NSS_NUM(pst_mac_dev))
    {
        /* 2个空间流的情况 */
        mac_vap_init_11ac_mcs_doublenss(pst_mac_vap, pst_mac_vap->st_channel.en_bandwidth);
    }
    else
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_11ac_rates::invalid en_nss_num[%d].}", MAC_DEVICE_GET_NSS_NUM(pst_mac_dev));
    }

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.init_11ac_rates_cb)
    {
        g_st_mac_vap_rom_cb.init_11ac_rates_cb(pst_mac_vap, pst_mac_dev);
    }
}


#ifdef _PRE_WLAN_FEATURE_11AX

oal_void mac_vap_rom_init_11ax_rates(mac_vap_stru *pst_mac_vap, mac_device_stru *pst_mac_dev)
{

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.init_11ax_rates_cb)
    {
        g_st_mac_vap_rom_cb.init_11ax_rates_cb(pst_mac_vap, pst_mac_dev);
    }
}
#endif


oal_void mac_vap_init_p2p_rates_etc(mac_vap_stru *pst_vap, wlan_protocol_enum_uint8 en_vap_protocol, mac_data_rate_stru  *pst_rates)
{
    mac_device_stru               *pst_mac_dev;

    pst_mac_dev = mac_res_get_dev_etc(pst_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_p2p_rates_etc::pst_mac_dev[%d] null.}", pst_vap->uc_device_id);

        return;
    }

    mac_vap_init_legacy_rates(pst_vap, pst_rates);

    oal_memcopy(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_5G], &pst_vap->st_curr_sup_rates, OAL_SIZEOF(pst_vap->st_curr_sup_rates));
    oal_memcopy(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_2G], &pst_vap->st_curr_sup_rates, OAL_SIZEOF(pst_vap->st_curr_sup_rates));

    if (WLAN_VHT_MODE == en_vap_protocol)
    {
        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
        mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);
    }
    else
    {
        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
    }
}

oal_void mac_vap_init_rates_by_protocol_etc(mac_vap_stru *pst_vap, wlan_protocol_enum_uint8 en_vap_protocol, mac_data_rate_stru *pst_rates)
{
    mac_device_stru *pst_mac_dev;

    pst_mac_dev = (mac_device_stru *)mac_res_get_dev_etc(pst_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_rates_by_protocol_etc::mac_res_get_dev_etc fail.device_id:[%d].}", pst_vap->uc_device_id);
        return;
    }

    /* STA模式默认协议模式是11ac，初始化速率集为所有速率集 */
#ifdef _PRE_WLAN_FEATURE_P2P
    if (!IS_LEGACY_VAP(pst_vap))
    {
        mac_vap_init_p2p_rates_etc(pst_vap, en_vap_protocol, pst_rates);
        return;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_11AX
    if(WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode && WLAN_HE_MODE == en_vap_protocol)
    {

        /* 用于STA全信道扫描 5G时 填写支持速率集ie */
        mac_vap_init_legacy_rates(pst_vap, pst_rates);
        oal_memcopy(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_5G], &pst_vap->st_curr_sup_rates, OAL_SIZEOF(pst_vap->st_curr_sup_rates));

        /* 用于STA全信道扫描 2G时 填写支持速率集ie */
        mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
        oal_memcopy(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_2G], &pst_vap->st_curr_sup_rates, OAL_SIZEOF(pst_vap->st_curr_sup_rates));

        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
        mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);

        mac_vap_rom_init_11ax_rates(pst_vap, pst_mac_dev);
    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode && WLAN_VHT_MODE == en_vap_protocol)
#else
    if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode && WLAN_VHT_MODE == en_vap_protocol)
#endif
    {
        /* 用于STA全信道扫描 5G时 填写支持速率集ie */
        mac_vap_init_legacy_rates(pst_vap, pst_rates);
        oal_memcopy(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_5G], &pst_vap->st_curr_sup_rates, OAL_SIZEOF(pst_vap->st_curr_sup_rates));

        /* 用于STA全信道扫描 2G时 填写支持速率集ie */
        mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
        oal_memcopy(&pst_vap->ast_sta_sup_rates_ie[WLAN_BAND_2G], &pst_vap->st_curr_sup_rates, OAL_SIZEOF(pst_vap->st_curr_sup_rates));

        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
        mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);
    }
    else if ((WLAN_VHT_ONLY_MODE == en_vap_protocol) || (WLAN_VHT_MODE == en_vap_protocol))
    {
#ifdef _PRE_WLAN_FEATURE_11AC2G
        if (WLAN_BAND_2G == pst_vap->st_channel.en_band)
        {
            mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
        }
        else
        {
            mac_vap_init_legacy_rates(pst_vap, pst_rates);
        }
#else
        mac_vap_init_legacy_rates(pst_vap, pst_rates);
#endif
        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
        mac_vap_init_11ac_rates(pst_vap, pst_mac_dev);
    }
    else if ((WLAN_HT_ONLY_MODE == en_vap_protocol) || (WLAN_HT_MODE == en_vap_protocol))
    {
        if (WLAN_BAND_5G == pst_vap->st_channel.en_band)
        {
            mac_vap_init_legacy_rates(pst_vap, pst_rates);
        }
        else if (WLAN_BAND_2G == pst_vap->st_channel.en_band)
        {
            mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
        }

        mac_vap_init_11n_rates(pst_vap, pst_mac_dev);
    }
    else if ((WLAN_LEGACY_11A_MODE == en_vap_protocol) || (WLAN_LEGACY_11G_MODE == en_vap_protocol))
    {
        mac_vap_init_legacy_rates(pst_vap, pst_rates);
    }
    else if (WLAN_LEGACY_11B_MODE == en_vap_protocol)
    {
        mac_vap_init_11b_rates(pst_vap, pst_rates);
    }
    else if (WLAN_MIXED_ONE_11G_MODE == en_vap_protocol)
    {
        mac_vap_init_11g_mixed_one_rates(pst_vap, pst_rates);
    }
    else if (WLAN_MIXED_TWO_11G_MODE == en_vap_protocol)
    {
        mac_vap_init_11g_mixed_two_rates(pst_vap, pst_rates);
    }
    else
    {
        /* 暂时不处理 */
    }

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.init_rates_by_prot_cb)
    {
        g_st_mac_vap_rom_cb.init_rates_by_prot_cb(pst_vap, en_vap_protocol, pst_rates);
    }
}


oal_void mac_vap_init_rates_etc(mac_vap_stru *pst_vap)
{
    mac_device_stru               *pst_mac_dev;
    wlan_protocol_enum_uint8       en_vap_protocol;
    mac_data_rate_stru            *pst_rates;

    pst_mac_dev = mac_res_get_dev_etc(pst_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_rates_etc::pst_mac_dev[%d] null.}", pst_vap->uc_device_id);

        return;
    }

    /* 初始化速率集 */
    pst_rates   = mac_device_get_all_rates_etc(pst_mac_dev);

    en_vap_protocol = pst_vap->en_protocol;

    mac_vap_init_rates_by_protocol_etc(pst_vap, en_vap_protocol, pst_rates);

}


oal_void mac_sta_init_bss_rates_etc(mac_vap_stru *pst_vap, oal_void *pst_bss_dscr)
{
    mac_device_stru               *pst_mac_dev;
    wlan_protocol_enum_uint8       en_vap_protocol;
    mac_data_rate_stru            *pst_rates;
    oal_uint32                     i,j;
    mac_bss_dscr_stru             *pst_bss = (mac_bss_dscr_stru *)pst_bss_dscr;

    pst_mac_dev = mac_res_get_dev_etc(pst_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_init_rates_etc::pst_mac_dev[%d] null.}", pst_vap->uc_device_id);

        return;
    }


    /* 初始化速率集 */
    pst_rates   = mac_device_get_all_rates_etc(pst_mac_dev);
    if(OAL_PTR_NULL != pst_bss)
    {
        for(i = 0; i < pst_bss->uc_num_supp_rates; i++)
        {
            for(j = 0; j < MAC_DATARATES_PHY_80211G_NUM; j++)
            {
                if((pst_rates[j].uc_mac_rate & 0x7f) == (pst_bss->auc_supp_rates[i] & 0x7f))
                {
                    pst_rates[j].uc_mac_rate = pst_bss->auc_supp_rates[i];
                    break;
                }
            }
        }
    }

    en_vap_protocol = pst_vap->en_protocol;

    mac_vap_init_rates_by_protocol_etc(pst_vap, en_vap_protocol, pst_rates);
}


oal_uint8 mac_vap_get_ap_usr_opern_bandwidth(mac_vap_stru *pst_mac_sta, mac_user_stru *pst_mac_user)
{
    mac_user_ht_hdl_stru                *pst_mac_ht_hdl;
    mac_vht_hdl_stru                    *pst_mac_vht_hdl;
    wlan_channel_bandwidth_enum_uint8    en_bandwidth_ap = WLAN_BAND_WIDTH_20M;
    wlan_channel_bandwidth_enum_uint8    en_sta_new_bandwidth;
#ifdef _PRE_WLAN_FEATURE_11AX
    mac_he_hdl_stru                     *pst_mac_he_hdl;
#endif

    /* 更新用户的带宽能力cap */
    mac_user_update_ap_bandwidth_cap(pst_mac_user);
    /* 获取HT和VHT结构体指针 */
    pst_mac_ht_hdl  = &(pst_mac_user->st_ht_hdl);
    pst_mac_vht_hdl = &(pst_mac_user->st_vht_hdl);
#ifdef _PRE_WLAN_FEATURE_11AX
    pst_mac_he_hdl = &(pst_mac_user->st_he_hdl);
#endif

    /******************* VHT BSS operating channel width ****************

     -----------------------------------------------------------------
     |HT Oper Chl Width |VHT Oper Chl Width |BSS Oper Chl Width|
     -----------------------------------------------------------------
     |       0          |        0          |    20MHZ         |
     -----------------------------------------------------------------
     |       1          |        0          |    40MHZ         |
     -----------------------------------------------------------------
     |       1          |        1          |    80MHZ         |
     -----------------------------------------------------------------
     |       1          |        2          |    160MHZ        |
     -----------------------------------------------------------------
     |       1          |        3          |    80+80MHZ      |
     -----------------------------------------------------------------

    **********************************************************************/

    if (OAL_TRUE == pst_mac_vht_hdl->en_vht_capable)
    {
        en_bandwidth_ap = mac_get_bandwith_from_center_freq_seg0_seg1(pst_mac_vht_hdl->en_channel_width,
                                          pst_mac_sta->st_channel.uc_chan_number, pst_mac_vht_hdl->uc_channel_center_freq_seg0, pst_mac_vht_hdl->uc_channel_center_freq_seg1);
    }

#ifdef _PRE_WLAN_FEATURE_11AX
    if((OAL_TRUE == pst_mac_he_hdl->en_he_capable) &&
        (OAL_TRUE == pst_mac_he_hdl->st_he_oper_ie.st_he_oper_param.bit_vht_operation_info_present))
    {
        en_bandwidth_ap = mac_get_bandwith_from_center_freq_seg0_seg1(pst_mac_he_hdl->st_he_oper_ie.st_vht_operation_info.uc_channel_width,
                                          pst_mac_sta->st_channel.uc_chan_number, pst_mac_he_hdl->st_he_oper_ie.st_vht_operation_info.uc_center_freq_seg0, pst_mac_he_hdl->st_he_oper_ie.st_vht_operation_info.uc_center_freq_seg1);
    }
#endif

    /* ht 20/40M带宽的处理 */
    if ((OAL_TRUE == pst_mac_ht_hdl->en_ht_capable) && (en_bandwidth_ap <=  WLAN_BAND_WIDTH_40MINUS)
        && (OAL_TRUE == mac_mib_get_FortyMHzOperationImplemented(pst_mac_sta)))
    {
        /* 更新带宽模式 */
        en_bandwidth_ap = mac_get_bandwidth_from_sco(pst_mac_ht_hdl->bit_secondary_chan_offset);
    }

    /* 带宽不能超过mac device的最大能力 */
    en_sta_new_bandwidth = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(pst_mac_sta), en_bandwidth_ap);
    if(OAL_FALSE == g_st_mac_regdomain_rom_cb.p_mac_regdomain_channel_is_support_bw(en_sta_new_bandwidth, pst_mac_sta->st_channel.uc_chan_number))
    {
        OAM_WARNING_LOG2(pst_mac_sta->uc_vap_id, OAM_SF_ASSOC,"{mac_vap_get_ap_usr_opern_bandwidth::channel[%d] is not support bw[%d],set 20MHz}",
        pst_mac_sta->st_channel.uc_chan_number, en_sta_new_bandwidth);
        en_sta_new_bandwidth = WLAN_BAND_WIDTH_20M;
    }

    return en_sta_new_bandwidth;
}

#if 0
#ifdef _PRE_WLAN_DFT_STAT

oal_uint32  mac_vap_dft_stat_init(mac_vap_stru *pst_vap)
{
    OAL_MEMZERO(&pst_vap->st_vap_dft, OAL_SIZEOF(mac_vap_dft_stru));

    return OAL_SUCC;
}
#endif

oal_uint32  mac_vap_dft_stat_clear(mac_vap_stru *pst_vap)
{
    pst_vap->st_vap_dft.ul_flg = 0;

    if (OAL_TRUE == pst_vap->st_vap_dft.st_vap_dft_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_vap->st_vap_dft.st_vap_dft_timer);
    }

    if (OAL_PTR_NULL != pst_vap->st_vap_dft.pst_vap_dft_stats)
    {
        OAL_MEM_FREE(pst_vap->st_vap_dft.pst_vap_dft_stats, OAL_TRUE);
    }

    return OAL_SUCC;
}

#endif


oal_void mac_vap_set_tx_power_etc(mac_vap_stru *pst_vap, oal_uint8 uc_tx_power)
{
    pst_vap->uc_tx_power = uc_tx_power;
}


oal_void mac_vap_set_aid_etc(mac_vap_stru *pst_vap, oal_uint16 us_aid)
{
    pst_vap->us_sta_aid = us_aid;
}


oal_void mac_vap_set_assoc_id_etc(mac_vap_stru *pst_vap, oal_uint16 us_assoc_vap_id)
{
    pst_vap->us_assoc_vap_id = us_assoc_vap_id;
}


oal_void mac_vap_set_uapsd_cap_etc(mac_vap_stru *pst_vap, oal_uint8 uc_uapsd_cap)
{
    pst_vap->uc_uapsd_cap = uc_uapsd_cap;
}


oal_void mac_vap_set_p2p_mode_etc(mac_vap_stru *pst_vap, wlan_p2p_mode_enum_uint8 en_p2p_mode)
{
    pst_vap->en_p2p_mode = en_p2p_mode;
}


oal_void mac_vap_set_multi_user_idx_etc(mac_vap_stru *pst_vap, oal_uint16 us_multi_user_idx)
{
    pst_vap->us_multi_user_idx = us_multi_user_idx;
}


oal_void mac_vap_set_rx_nss_etc(mac_vap_stru *pst_vap, wlan_nss_enum_uint8 en_rx_nss)
{
    pst_vap->en_vap_rx_nss = en_rx_nss;
}


oal_void mac_vap_set_al_tx_payload_flag_etc(mac_vap_stru *pst_vap, oal_uint8 uc_paylod)
{
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    pst_vap->bit_payload_flag = uc_paylod;
#endif
}


oal_void mac_vap_set_al_tx_flag_etc(mac_vap_stru *pst_vap, oal_bool_enum_uint8 en_flag)
{
#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    pst_vap->bit_al_tx_flag = en_flag;
#endif
}

#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_void mac_vap_set_uapsd_para_etc(mac_vap_stru *pst_mac_vap, mac_cfg_uapsd_sta_stru *pst_uapsd_info)
{
    oal_uint8                 uc_ac;

    pst_mac_vap->st_sta_uapsd_cfg.uc_max_sp_len = pst_uapsd_info->uc_max_sp_len;

    for (uc_ac = 0; uc_ac < WLAN_WME_AC_BUTT; uc_ac++)
    {
        pst_mac_vap->st_sta_uapsd_cfg.uc_delivery_enabled[uc_ac] = pst_uapsd_info->uc_delivery_enabled[uc_ac];
        pst_mac_vap->st_sta_uapsd_cfg.uc_trigger_enabled[uc_ac]  = pst_uapsd_info->uc_trigger_enabled[uc_ac];
    }

}
#endif


oal_void mac_vap_set_wmm_params_update_count_etc(mac_vap_stru *pst_vap, oal_uint8 uc_update_count)
{
    pst_vap->uc_wmm_params_update_count = uc_update_count;
}


oal_void mac_vap_set_rifs_tx_on_etc(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->st_cap_flag.bit_rifs_tx_on = uc_value;
}
#if 0

oal_void mac_vap_set_tdls_prohibited(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->st_cap_flag.bit_tdls_prohibited = uc_value;
}


oal_void mac_vap_set_tdls_channel_switch_prohibited(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->st_cap_flag.bit_tdls_channel_switch_prohibited = uc_value;
}
#endif
#ifdef _PRE_WLAN_FEATURE_VOWIFI

oal_void mac_vap_vowifi_init(mac_vap_stru *pst_mac_vap)
{
    if (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        return;
    }
    if (OAL_PTR_NULL == pst_mac_vap->pst_vowifi_cfg_param)
    {
        pst_mac_vap->pst_vowifi_cfg_param = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_vowifi_param_stru), OAL_TRUE);
        if (OAL_PTR_NULL == pst_mac_vap->pst_vowifi_cfg_param)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_VOWIFI, "{mac_vap_vowifi_init::pst_vowifi_cfg_param alloc null, size[%d].}", OAL_SIZEOF(mac_vowifi_param_stru));
            return ;
        }
    }
    OAL_MEMZERO(pst_mac_vap->pst_vowifi_cfg_param, OAL_SIZEOF(mac_vowifi_param_stru));
    pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_mode         = MAC_VAP_VOWIFI_MODE_DEFAULT;
    pst_mac_vap->pst_vowifi_cfg_param->uc_trigger_count_thres = MAC_VAP_VOWIFI_TRIGGER_COUNT_DEFAULT;
    pst_mac_vap->pst_vowifi_cfg_param->us_rssi_period_ms      = MAC_VAP_VOWIFI_PERIOD_DEFAULT_MS;
    pst_mac_vap->pst_vowifi_cfg_param->c_rssi_high_thres      = MAC_VAP_VOWIFI_HIGH_THRES_DEFAULT;
    pst_mac_vap->pst_vowifi_cfg_param->c_rssi_low_thres       = MAC_VAP_VOWIFI_LOW_THRES_DEFAULT;

}

oal_void mac_vap_vowifi_exit(mac_vap_stru *pst_mac_vap)
{
    mac_vowifi_param_stru   *pst_vowifi_cfg_param;

    if (OAL_PTR_NULL == pst_mac_vap->pst_vowifi_cfg_param)
    {
        return;
    }

    pst_vowifi_cfg_param = pst_mac_vap->pst_vowifi_cfg_param;

    /*先置空再释放*/
    pst_mac_vap->pst_vowifi_cfg_param = OAL_PTR_NULL;
    OAL_MEM_FREE(pst_vowifi_cfg_param, OAL_TRUE);
}
#endif /* #ifdef _PRE_WLAN_FEATURE_VOWIFI */


oal_void mac_vap_set_11ac2g_etc(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->st_cap_flag.bit_11ac2g = uc_value;
}


oal_void mac_vap_set_hide_ssid_etc(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->st_cap_flag.bit_hide_ssid = uc_value;
}


oal_uint8 mac_vap_get_peer_obss_scan_etc(mac_vap_stru *pst_vap)
{
    return pst_vap->st_cap_flag.bit_peer_obss_scan;
}


oal_void mac_vap_set_peer_obss_scan_etc(mac_vap_stru *pst_vap, oal_uint8 uc_value)
{
    pst_vap->st_cap_flag.bit_peer_obss_scan = uc_value;
}


wlan_p2p_mode_enum_uint8  mac_get_p2p_mode_etc(mac_vap_stru *pst_vap)
{
    return (pst_vap->en_p2p_mode);
}



oal_void mac_dec_p2p_num_etc(mac_vap_stru *pst_vap)
{
    mac_device_stru               *pst_device;

    pst_device     = mac_res_get_dev_etc(pst_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_device))
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_p2p_dec_num::pst_device[%d] null.}", pst_vap->uc_device_id);
        return;
    }

    if (IS_P2P_DEV(pst_vap))
    {
        pst_device->st_p2p_info.uc_p2p_device_num--;
    }
    else if (IS_P2P_GO(pst_vap) || IS_P2P_CL(pst_vap))
    {
        pst_device->st_p2p_info.uc_p2p_goclient_num--;
    }
}

oal_void mac_inc_p2p_num_etc(mac_vap_stru *pst_vap)
{
    mac_device_stru               *pst_dev;

    pst_dev = mac_res_get_dev_etc(pst_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dev))
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_inc_p2p_num::pst_dev[%d] null.}", pst_vap->uc_device_id);
        return;
    }

    if (IS_P2P_DEV(pst_vap))
    {
        /* device下sta个数加1 */
        pst_dev->st_p2p_info.uc_p2p_device_num++;
    }
    else if (IS_P2P_GO(pst_vap))
    {
        pst_dev->st_p2p_info.uc_p2p_goclient_num++;
    }
    else if (IS_P2P_CL(pst_vap))
    {
        pst_dev->st_p2p_info.uc_p2p_goclient_num++;
    }
}


oal_uint32 mac_vap_save_app_ie_etc(mac_vap_stru *pst_mac_vap, oal_app_ie_stru *pst_app_ie, en_app_ie_type_uint8 en_type)
{
    oal_uint8           *puc_ie = OAL_PTR_NULL;
    oal_uint32           ul_ie_len;
    oal_app_ie_stru      st_tmp_app_ie;

    OAL_MEMZERO(&st_tmp_app_ie, OAL_SIZEOF(st_tmp_app_ie));

    if (en_type >= OAL_APP_IE_NUM)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie_etc::invalid en_type[%d].}", en_type);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ul_ie_len = pst_app_ie->ul_ie_len;

    /* 如果输入WPS 长度为0， 则直接释放VAP 中资源 */
    if (0 == ul_ie_len)
    {
        if (OAL_PTR_NULL != pst_mac_vap->ast_app_ie[en_type].puc_ie)
        {
            OAL_MEM_FREE(pst_mac_vap->ast_app_ie[en_type].puc_ie, OAL_TRUE);
        }

        pst_mac_vap->ast_app_ie[en_type].puc_ie         = OAL_PTR_NULL;
        pst_mac_vap->ast_app_ie[en_type].ul_ie_len      = 0;
        pst_mac_vap->ast_app_ie[en_type].ul_ie_max_len  = 0;

        return OAL_SUCC;
    }

    /* 检查该类型的IE是否需要申请内存 */
    if ((pst_mac_vap->ast_app_ie[en_type].ul_ie_max_len < ul_ie_len) || (NULL == pst_mac_vap->ast_app_ie[en_type].puc_ie))
    {
        /* 这种情况不应该出现，维测需要 */
        if (NULL == pst_mac_vap->ast_app_ie[en_type].puc_ie && pst_mac_vap->ast_app_ie[en_type].ul_ie_max_len != 0  )
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie_etc::invalid len[%d].}",
                          pst_mac_vap->ast_app_ie[en_type].ul_ie_max_len);
        }

        /* 如果以前的内存空间小于新信息元素需要的长度，则需要重新申请内存 */
        puc_ie = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, (oal_uint16)(ul_ie_len), OAL_TRUE);
        if (OAL_PTR_NULL == puc_ie)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie_etc::LOCAL_MEM_POOL is empty!,len[%d], en_type[%d].}",
                              pst_app_ie->ul_ie_len, en_type);
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        OAL_MEM_FREE(pst_mac_vap->ast_app_ie[en_type].puc_ie, OAL_TRUE);

        pst_mac_vap->ast_app_ie[en_type].puc_ie = puc_ie;
        pst_mac_vap->ast_app_ie[en_type].ul_ie_max_len = ul_ie_len;
    }

    oal_memcopy((oal_void*)pst_mac_vap->ast_app_ie[en_type].puc_ie, (oal_void*)pst_app_ie->auc_ie, ul_ie_len);
    pst_mac_vap->ast_app_ie[en_type].ul_ie_len = ul_ie_len;
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_save_app_ie_etc::IE:[0x%2x][0x%2x][0x%2x][0x%2x]}",
                          *(pst_mac_vap->ast_app_ie[en_type].puc_ie),
                          *(pst_mac_vap->ast_app_ie[en_type].puc_ie+1),
                          *(pst_mac_vap->ast_app_ie[en_type].puc_ie+2),
                          *(pst_mac_vap->ast_app_ie[en_type].puc_ie+ul_ie_len-1));
    return OAL_SUCC;
}
 
 oal_uint32 mac_vap_clear_app_ie_etc(mac_vap_stru *pst_mac_vap, en_app_ie_type_uint8 en_type)
 {
     if (en_type < OAL_APP_IE_NUM)
     {
         if (OAL_PTR_NULL != pst_mac_vap->ast_app_ie[en_type].puc_ie)
         {
             OAL_MEM_FREE(pst_mac_vap->ast_app_ie[en_type].puc_ie, OAL_TRUE);
             pst_mac_vap->ast_app_ie[en_type].puc_ie    = OAL_PTR_NULL;
         }
         pst_mac_vap->ast_app_ie[en_type].ul_ie_len     = 0;
         pst_mac_vap->ast_app_ie[en_type].ul_ie_max_len = 0;
     }

     return OAL_SUCC;
 }

oal_void mac_vap_free_mib_etc(mac_vap_stru   *pst_vap)
{
    if(NULL == pst_vap)
        return;

    if (OAL_PTR_NULL != pst_vap->pst_mib_info)
    {
        wlan_mib_ieee802dot11_stru  *pst_mib_info = pst_vap->pst_mib_info;
        /*先置空再释放*/
        pst_vap->pst_mib_info = OAL_PTR_NULL;
        OAL_MEM_FREE(pst_mib_info, OAL_TRUE);
    }
}

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
 
oal_uint32  mac_vap_free_ie_set_ptr(mac_vap_stru *pst_vap)
{
    if (OAL_PTR_NULL != pst_vap->pst_rrm_ie_info)
    {
        OAL_MEM_FREE(pst_vap->pst_rrm_ie_info, OAL_TRUE);
        pst_vap->pst_rrm_ie_info = OAL_PTR_NULL;
    }
    if (OAL_PTR_NULL != pst_vap->pst_excap_ie_info)
    {
        OAL_MEM_FREE(pst_vap->pst_excap_ie_info, OAL_TRUE);
        pst_vap->pst_excap_ie_info = OAL_PTR_NULL;
    }
    if (OAL_PTR_NULL != pst_vap->pst_msta_ie_info)
    {
        OAL_MEM_FREE(pst_vap->pst_msta_ie_info, OAL_TRUE);
        pst_vap->pst_msta_ie_info = OAL_PTR_NULL;
    }
    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_11KV_INTERFACE

 
oal_uint32 mac_vap_exit_etc(mac_vap_stru *pst_vap)
{
    mac_device_stru               *pst_device;
    oal_uint8                      uc_index;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_vap_exit_etc::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_vap->uc_init_flag = MAC_VAP_INVAILD;

    /* 释放与hmac有关的内存 */
    mac_vap_free_mib_etc(pst_vap);
#ifdef _PRE_WLAN_FEATURE_VOWIFI
    /* 释放vowifi相关内存 */
    mac_vap_vowifi_exit(pst_vap);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

    /* 释放WPS信息元素内存 */
    for (uc_index = 0; uc_index < OAL_APP_IE_NUM; uc_index++)
    {
        mac_vap_clear_app_ie_etc(pst_vap, uc_index);
    }

    /* 业务vap已删除，从device上去掉 */
    pst_device     = mac_res_get_dev_etc(pst_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_device))
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_exit_etc::pst_device[%d] null.}", pst_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 业务vap已经删除，从device中去掉 */
    for (uc_index = 0; uc_index < pst_device->uc_vap_num; uc_index++)
    {
        /* 从device中找到vap id */
        if (pst_device->auc_vap_id[uc_index] == pst_vap->uc_vap_id)
        {
            /* 如果不是最后一个vap，则把最后一个vap id移动到这个位置，使得该数组是紧凑的 */
            if(uc_index < (pst_device->uc_vap_num - 1))
            {
                pst_device->auc_vap_id[uc_index] = pst_device->auc_vap_id[pst_device->uc_vap_num - 1];
                break;
            }
        }
    }

    if (pst_device->uc_vap_num != 0)
    {
        /* device下的vap总数减1 */
        pst_device->uc_vap_num--;
    }
    else
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG,
                            "{mac_vap_exit_etc::mac_device's vap_num is zero. sta_num = %d}",
                            pst_device->uc_sta_num);
    }

    /* 清除数组中已删除的vap id，保证非零数组元素均为未删除vap */
    pst_device->auc_vap_id[pst_device->uc_vap_num] = 0;

    /* device下sta个数减1 */
    if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
    {
        if (pst_device->uc_sta_num != 0)
        {
            pst_device->uc_sta_num--;
        }
        else
        {
            OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG,
                            "{mac_vap_exit_etc::mac_device's sta_num is zero. vap_num = %d}",
                            pst_device->uc_vap_num);
        }
    }

#ifdef _PRE_WLAN_FEATURE_P2P
    mac_dec_p2p_num_etc(pst_vap);
#endif

    pst_vap->en_protocol  = WLAN_PROTOCOL_BUTT;

    /* 最后1个vap删除时，清除device级带宽信息 */
    if (0 == pst_device->uc_vap_num)
    {
        pst_device->uc_max_channel   = 0;
        pst_device->en_max_band      = WLAN_BAND_BUTT;
        pst_device->en_max_bandwidth = WLAN_BAND_WIDTH_BUTT;
    }

#ifdef _PRE_WLAN_FEATURE_HILINK
    mac_hilink_exit_vap(pst_vap);
#endif
    /* 释放hera产品设置ie时申请的指针内存 */
#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
    mac_vap_free_ie_set_ptr(pst_vap);
#endif
#ifdef _PRE_WLAN_FEATURE_QOS_ENHANCE
    if (WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode)
    {
        mac_tx_clean_qos_enhance_list(pst_vap);
    }
#endif


    /* 删除之后将vap的状态置位非法 */
    mac_vap_state_change_etc(pst_vap, MAC_VAP_STATE_BUTT);

    return OAL_SUCC;

}


oal_void  mac_init_mib_etc(mac_vap_stru *pst_mac_vap)
{
    oal_uint8 uc_idx;
    mac_device_stru                 *pst_mac_dev;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_init_mib_etc::pst_mac_vap null.}");

        return;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_init_mib_etc::pst_mac_dev[%d] null.}", pst_mac_vap->uc_device_id);
        return;
    }

    /* 公共特性mib值初始化 */
    mac_mib_set_dot11dtimperiod(pst_mac_vap, WLAN_DTIM_DEFAULT);
    mac_mib_set_RTSThreshold(pst_mac_vap, WLAN_RTS_MAX);
    mac_mib_set_FragmentationThreshold(pst_mac_vap, WLAN_FRAG_THRESHOLD_MAX);
    mac_mib_set_DesiredBSSType(pst_mac_vap, WLAN_MIB_DESIRED_BSSTYPE_INFRA);
    mac_mib_set_BeaconPeriod(pst_mac_vap, WLAN_BEACON_INTVAL_DEFAULT);
    mac_mib_set_dot11VapMaxBandWidth(pst_mac_vap, MAC_DEVICE_GET_CAP_BW(pst_mac_dev));
    /*设置vap最大用户数*/
    mac_mib_set_MaxAssocUserNums(pst_mac_vap, mac_chip_get_max_asoc_user(pst_mac_vap->uc_chip_id));
    mac_mib_set_WPSActive(pst_mac_vap, OAL_FALSE);

    /* 2040共存信道切换开关初始化 */
#ifdef _PRE_WLAN_FEATURE_P2P
    /*P2P设备关闭20/40代管切换*/
    if (WLAN_LEGACY_VAP_MODE != pst_mac_vap->en_p2p_mode)
    {
        mac_mib_set_2040SwitchProhibited(pst_mac_vap, OAL_TRUE);
    }
    /*非P2P设备开启20/40代管切换*/
    else
    {
        mac_mib_set_2040SwitchProhibited(pst_mac_vap, OAL_FALSE);
    }
#else
    mac_mib_set_2040SwitchProhibited(pst_mac_vap, OAL_FALSE);
#endif

    /* 初始化认证类型为OPEN */
    mac_mib_set_AuthenticationMode(pst_mac_vap, WLAN_WITP_AUTH_OPEN_SYSTEM);

    /* 默认设置为自动触发BA回话的建立 */
    mac_mib_set_AddBaMode(pst_mac_vap, WLAN_ADDBA_MODE_AUTO);

    mac_mib_set_CfgAmpduTxAtive(pst_mac_vap, OAL_TRUE);
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
    mac_mib_set_RxBASessionNumber(pst_mac_vap, 0);
    mac_mib_set_TxBASessionNumber(pst_mac_vap, 0);
#endif

    /* 1151默认不amsdu ampdu 联合聚合功能不开启 1102用于小包优化
     * 因tplink/syslink下行冲包兼容性问题，先关闭02的ampdu+amsdu */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)||(_PRE_TEST_MODE ==_PRE_TEST_MODE_UT)
    mac_mib_set_CfgAmsduTxAtive(pst_mac_vap, OAL_FALSE);
    mac_mib_set_AmsduPlusAmpduActive(pst_mac_vap, OAL_FALSE);
#else
    mac_mib_set_CfgAmsduTxAtive(pst_mac_vap, OAL_TRUE);
    mac_mib_set_AmsduPlusAmpduActive(pst_mac_vap, OAL_TRUE);
#endif

    mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, WLAN_LEGACY_11B_MIB_SHORT_PREAMBLE);
    mac_mib_set_PBCCOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_ChannelAgilityPresent(pst_mac_vap, OAL_FALSE);
    mac_mib_set_dot11MultiDomainCapabilityActivated(pst_mac_vap, OAL_TRUE);
    mac_mib_set_SpectrumManagementRequired(pst_mac_vap, OAL_TRUE);
    mac_mib_set_dot11ExtendedChannelSwitchActivated(pst_mac_vap, OAL_FALSE);
    mac_mib_set_dot11QosOptionImplemented(pst_mac_vap, OAL_TRUE);
    mac_mib_set_dot11APSDOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_dot11QBSSLoadImplemented(pst_mac_vap, OAL_TRUE);
    mac_mib_set_ShortSlotTimeOptionImplemented(pst_mac_vap, OAL_TRUE);
    mac_mib_set_ShortSlotTimeOptionActivated(pst_mac_vap, OAL_TRUE);
    mac_mib_set_dot11RadioMeasurementActivated(pst_mac_vap, OAL_FALSE);


    mac_mib_set_DSSSOFDMOptionActivated(pst_mac_vap, OAL_FALSE);
    mac_mib_set_dot11ImmediateBlockAckOptionImplemented(pst_mac_vap, OAL_TRUE);
    mac_mib_set_dot11DelayedBlockAckOptionImplemented(pst_mac_vap, OAL_FALSE);
    mac_mib_set_AuthenticationResponseTimeOut(pst_mac_vap, WLAN_AUTH_TIMEOUT);

    mac_mib_set_HtProtection(pst_mac_vap, WLAN_MIB_HT_NO_PROTECTION);
    mac_mib_set_RifsMode(pst_mac_vap, OAL_TRUE);
    mac_mib_set_NonGFEntitiesPresent(pst_mac_vap, OAL_FALSE);
    mac_mib_set_LsigTxopFullProtectionActivated(pst_mac_vap, OAL_FALSE);

    mac_mib_set_DualCTSProtection(pst_mac_vap, OAL_FALSE);
    mac_mib_set_PCOActivated(pst_mac_vap, OAL_FALSE);

    mac_mib_set_dot11AssociationResponseTimeOut(pst_mac_vap, WLAN_ASSOC_TIMEOUT);
    mac_mib_set_dot11AssociationSAQueryMaximumTimeout(pst_mac_vap, WLAN_SA_QUERY_MAXIMUM_TIME);
    mac_mib_set_dot11AssociationSAQueryRetryTimeout(pst_mac_vap, WLAN_SA_QUERY_RETRY_TIME);

    /* 认证算法表初始化 */
    //pst_mib_info->st_wlan_mib_auth_alg.uc_dot11AuthenticationAlgorithm = WLAN_WITP_AUTH_OPEN_SYSTEM;
    //pst_mib_info->st_wlan_mib_auth_alg.en_dot11AuthenticationAlgorithmsActivated = OAL_FALSE;

    /* WEP 缺省Key表初始化 */
    for(uc_idx = 0; uc_idx < WLAN_NUM_DOT11WEPDEFAULTKEYVALUE; uc_idx++)
    {
        /* 大小初始化为 WEP-40  */
        mac_mib_set_wep_etc(pst_mac_vap, uc_idx, WLAN_WEP_40_KEY_SIZE);
    }

    /*   WEP Kep 映射表初始化    */
    //oal_memset(pst_mib_info->st_wlan_mib_wep_key_map.auc_dot11WEPKeyMappingValue,
           //0, WLAN_MAX_WEP_STR_SIZE);
    //oal_memset(pst_mib_info->st_wlan_mib_wep_key_map.auc_dot11WEPKeyMappingAddress, 0,
           //WLAN_MAC_ADDR_LEN);
    //pst_mib_info->st_wlan_mib_wep_key_map.en_dot11WEPKeyMappingWEPOn = OAL_FALSE;

    /*    相关私有表初始化  */
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_FALSE);
    mac_set_wep_default_keyid(pst_mac_vap, 0);
    //pst_mib_info->st_wlan_mib_privacy.en_dot11ExcludeUnencrypted             = OAL_TRUE;
    //pst_mib_info->st_wlan_mib_privacy.ul_dot11WEPICVErrorCount               = 0;
    //pst_mib_info->st_wlan_mib_privacy.ul_dot11WEPExcludedCount               = 0;

    /* 更新wmm参数初始值 */
    mac_vap_init_wme_param_etc(pst_mac_vap);

    /* 11i */
    mac_vap_init_mib_11i(pst_mac_vap);

    /* 默认11n 11ac使能关闭，配置协议模式时打开 */
    mac_vap_init_mib_11n(pst_mac_vap);
    mac_vap_init_mib_11ac(pst_mac_vap);
#ifdef _PRE_WLAN_FEATURE_11AX
    mac_vap_rom_init_mib_11ax(pst_mac_vap);
#endif

    /* staut低功耗mib项初始化 */
    mac_mib_set_powermanagementmode(pst_mac_vap, WLAN_MIB_PWR_MGMT_MODE_ACTIVE);

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)
    /* 11k */
    mac_vap_init_mib_11k(pst_mac_vap);
#endif

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.mac_init_mib_cb)
    {
        g_st_mac_vap_rom_cb.mac_init_mib_cb(pst_mac_vap);
    }

}


oal_void  mac_vap_cap_init_legacy(mac_vap_stru *pst_mac_vap)
{
    pst_mac_vap->st_cap_flag.bit_rifs_tx_on = OAL_FALSE;

    /* 非VHT不使能 txop ps */
    pst_mac_vap->st_cap_flag.bit_txop_ps    = OAL_FALSE;

    if(OAL_PTR_NULL != pst_mac_vap->pst_mib_info)
    {
        mac_mib_set_txopps(pst_mac_vap, OAL_FALSE);
    }

    return;
}


oal_uint32  mac_vap_cap_init_htvht(mac_vap_stru *pst_mac_vap)
{
    pst_mac_vap->st_cap_flag.bit_rifs_tx_on = OAL_FALSE;

#ifdef _PRE_WLAN_FEATURE_TXOPPS
    if(OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_cap_init_htvht::pst_mib_info null,vap mode[%d] state[%d] user num[%d].}",
                                     pst_mac_vap->en_vap_mode ,pst_mac_vap->en_vap_state, pst_mac_vap->us_user_nums);
        return OAL_FAIL;
    }

    pst_mac_vap->st_cap_flag.bit_txop_ps    = OAL_FALSE;
    if ((pst_mac_vap->en_protocol == WLAN_VHT_MODE ||
         pst_mac_vap->en_protocol == WLAN_VHT_ONLY_MODE
    #ifdef _PRE_WLAN_FEATURE_11AX
         || pst_mac_vap->en_protocol == WLAN_HE_MODE
    #endif
         ) && mac_mib_get_txopps(pst_mac_vap))
    {
        mac_mib_set_txopps(pst_mac_vap, OAL_TRUE);
    }
    else
    {
        mac_mib_set_txopps(pst_mac_vap, OAL_FALSE);
    }
#endif

    return OAL_SUCC;
}



//l00311403TODO
oal_uint32 mac_vap_config_vht_ht_mib_by_protocol_etc(mac_vap_stru *pst_mac_vap)
{
    if(OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_vap_config_vht_ht_mib_by_protocol_etc::pst_mib_info null,vap mode[%d] state[%d] user num[%d].}",
                                     pst_mac_vap->en_vap_mode ,pst_mac_vap->en_vap_state, pst_mac_vap->us_user_nums);
        return OAL_FAIL;
    }
    /* 根据协议模式更新 HT/VHT mib值 */
    if (WLAN_HT_MODE == pst_mac_vap->en_protocol || WLAN_HT_ONLY_MODE == pst_mac_vap->en_protocol)
    {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_FALSE);

    #ifdef _PRE_WLAN_FEATURE_11AX
        mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_FALSE);
    #endif
    }
    else if (WLAN_VHT_MODE == pst_mac_vap->en_protocol || WLAN_VHT_ONLY_MODE == pst_mac_vap->en_protocol)
    {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_TRUE);
    #ifdef _PRE_WLAN_FEATURE_11AX
        mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_FALSE);
    #endif
    }
#ifdef _PRE_WLAN_FEATURE_11AX
    else if(WLAN_HE_MODE == pst_mac_vap->en_protocol)
    {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_TRUE);
    }
#endif
    else
    {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_FALSE);
    #ifdef _PRE_WLAN_FEATURE_11AX
        mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_FALSE);
    #endif
    }

    if (!pst_mac_vap->en_vap_wmm)
    {
        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, OAL_FALSE);
    }

    return OAL_SUCC;
}


oal_void  mac_vap_init_rx_nss_by_protocol_etc(mac_vap_stru *pst_mac_vap)
{
    wlan_protocol_enum_uint8 en_protocol;
    mac_device_stru         *pst_mac_device;

    en_protocol   = pst_mac_vap->en_protocol;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_init_rx_nss_by_protocol_etc::pst_mac_device[%d] null.}", pst_mac_vap->uc_device_id);
        return;
    }

    switch(en_protocol)
    {
        case WLAN_HT_MODE:
        case WLAN_VHT_MODE:
        case WLAN_HT_ONLY_MODE:
        case WLAN_VHT_ONLY_MODE:
        case WLAN_HT_11G_MODE:
    #ifdef _PRE_WLAN_FEATURE_11AX
        case WLAN_HE_MODE:
    #endif
             pst_mac_vap->en_vap_rx_nss  = WLAN_DOUBLE_NSS;
             break;
        case WLAN_PROTOCOL_BUTT:
             pst_mac_vap->en_vap_rx_nss  = WLAN_NSS_LIMIT;
             return;

        default:
             pst_mac_vap->en_vap_rx_nss  = WLAN_SINGLE_NSS;
             break;
    }

#ifdef _PRE_WLAN_FEATURE_M2S
    pst_mac_vap->en_vap_rx_nss = OAL_MIN(pst_mac_vap->en_vap_rx_nss,
             OAL_MIN(MAC_DEVICE_GET_NSS_NUM(pst_mac_device), MAC_M2S_CALI_NSS_FROM_SMPS_MODE(MAC_DEVICE_GET_MODE_SMPS(pst_mac_device))));
#else
    pst_mac_vap->en_vap_rx_nss = OAL_MIN(pst_mac_vap->en_vap_rx_nss, MAC_DEVICE_GET_NSS_NUM(pst_mac_device));
#endif
}


oal_uint32  mac_vap_init_by_protocol_etc(mac_vap_stru *pst_mac_vap, wlan_protocol_enum_uint8 en_protocol)
{
    pst_mac_vap->en_protocol          = en_protocol;

    if (en_protocol < WLAN_HT_MODE)
    {
        mac_vap_cap_init_legacy(pst_mac_vap);
    }
    else
    {
        if(OAL_SUCC != mac_vap_cap_init_htvht(pst_mac_vap))
        {
            return OAL_FAIL;
        }
    }

    /* 根据协议模式更新mib值 */
    if(OAL_SUCC != mac_vap_config_vht_ht_mib_by_protocol_etc(pst_mac_vap))
    {
        return OAL_FAIL;
    }

    /* 根据协议更新初始化空间流个数 */
    mac_vap_init_rx_nss_by_protocol_etc(pst_mac_vap);

    if (g_st_mac_vap_rom_cb.mac_vap_init_by_protocol_cb != OAL_PTR_NULL)
    {
        g_st_mac_vap_rom_cb.mac_vap_init_by_protocol_cb(pst_mac_vap, en_protocol);
    }

    return OAL_SUCC;
}


wlan_bw_cap_enum_uint8 mac_vap_bw_mode_to_bw(wlan_channel_bandwidth_enum_uint8 en_mode)
{
    OAL_STATIC wlan_bw_cap_enum_uint8 g_bw_mode_to_bw_table[] = {
        WLAN_BW_CAP_20M, // WLAN_BAND_WIDTH_20M                 = 0,
        WLAN_BW_CAP_40M, // WLAN_BAND_WIDTH_40PLUS              = 1,    /* 从20信道+1 */
        WLAN_BW_CAP_40M, // WLAN_BAND_WIDTH_40MINUS             = 2,    /* 从20信道-1 */
        WLAN_BW_CAP_80M, // WLAN_BAND_WIDTH_80PLUSPLUS          = 3,    /* 从20信道+1, 从40信道+1 */
        WLAN_BW_CAP_80M, // WLAN_BAND_WIDTH_80PLUSMINUS         = 4,    /* 从20信道+1, 从40信道-1 */
        WLAN_BW_CAP_80M, // WLAN_BAND_WIDTH_80MINUSPLUS         = 5,    /* 从20信道-1, 从40信道+1 */
        WLAN_BW_CAP_80M, // WLAN_BAND_WIDTH_80MINUSMINUS        = 6,    /* 从20信道-1, 从40信道-1 */
#ifdef _PRE_WLAN_FEATURE_160M
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160PLUSPLUSPLUS,            /* 从20信道+1, 从40信道+1, 从80信道+1 */
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160PLUSPLUSMINUS,           /* 从20信道+1, 从40信道+1, 从80信道-1 */
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160PLUSMINUSPLUS,           /* 从20信道+1, 从40信道-1, 从80信道+1 */
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160PLUSMINUSMINUS,          /* 从20信道+1, 从40信道-1, 从80信道-1 */
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160MINUSPLUSPLUS,           /* 从20信道-1, 从40信道+1, 从80信道+1 */
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160MINUSPLUSMINUS,          /* 从20信道-1, 从40信道+1, 从80信道-1 */
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160MINUSMINUSPLUS,          /* 从20信道-1, 从40信道-1, 从80信道+1 */
        WLAN_BW_CAP_160M, //WLAN_BAND_WIDTH_160MINUSMINUSMINUS,         /* 从20信道-1, 从40信道-1, 从80信道-1 */
#endif

        WLAN_BW_CAP_40M, //WLAN_BAND_WIDTH_40M,
        WLAN_BW_CAP_80M, //WLAN_BAND_WIDTH_80M,
    };

    if(en_mode >= WLAN_BAND_WIDTH_BUTT)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,"mac_vap_bw_mode_to_bw::invalid en_mode = %d, force 20M", en_mode);
        en_mode = 0;
    }

    return g_bw_mode_to_bw_table[en_mode];
}


oal_void  mac_vap_change_mib_by_bandwidth_etc(mac_vap_stru *pst_mac_vap, wlan_channel_band_enum_uint8 en_band,  wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    wlan_mib_ieee802dot11_stru *pst_mib_info;
    wlan_bw_cap_enum_uint8      en_bw;

    pst_mib_info = pst_mac_vap->pst_mib_info;

    if (OAL_PTR_NULL == pst_mib_info)
    {
#ifdef _PRE_WLAN_WEB_CMD_COMM
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_change_mib_by_bandwidth_etc::pst_mib_info null.}");
#else
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_change_mib_by_bandwidth_etc::pst_mib_info null.}");
#endif

        return;
    }

    /* 更新40M使能mib, 默认使能 */
    mac_mib_set_FortyMHzOperationImplemented(pst_mac_vap, OAL_TRUE);

#if (_PRE_TARGET_PRODUCT_TYPE_5630HERA != _PRE_CONFIG_TARGET_PRODUCT)
    //去掉shortGI强制使能，使用iwpriv命令设置，否则用户配置会被覆盖
    /* 更新short gi使能mib, 默认全使能，根据带宽信息更新 */
    mac_mib_set_ShortGIOptionInTwentyImplemented(pst_mac_vap, OAL_TRUE);
    mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_TRUE);
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if ((WLAN_BAND_2G == pst_mac_vap->st_channel.en_band) && (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_disable_2ght40))
    {
        mac_mib_set_FortyMHzOperationImplemented(pst_mac_vap, OAL_FALSE);
    }
#endif

#if (_PRE_TARGET_PRODUCT_TYPE_5630HERA != _PRE_CONFIG_TARGET_PRODUCT)
    //去掉shortGI强制使能，使用iwpriv命令设置，否则用户配置会被覆盖
    mac_mib_set_VHTShortGIOptionIn80Implemented(pst_mac_vap, OAL_TRUE);
#endif

    en_bw = mac_vap_bw_mode_to_bw(en_bandwidth);
    if (WLAN_BW_CAP_20M == en_bw)
    {
        mac_mib_set_FortyMHzOperationImplemented(pst_mac_vap, OAL_FALSE);
    }

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.ch_mib_by_bw_cb)
    {
        g_st_mac_vap_rom_cb.ch_mib_by_bw_cb(pst_mac_vap, en_band, en_bandwidth);
    }
}
#ifdef _PRE_WLAN_FEATURE_DFS

oal_bool_enum_uint8  mac_vap_get_dfs_enable(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru               *pst_mac_device;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_get_dfs_enable::mac_res_get_dev_etc[%d] Error.}",
        pst_mac_vap->uc_device_id);
        return OAL_FALSE;
    }

    /*lint -save -e731 */
    /*先判断vap是否支持，再判断mac_device的dfs开关是否打开*/
    if(OAL_TRUE == MAC_VAP_IS_SUPPORT_DFS(pst_mac_vap))
    {
        return mac_dfs_get_dfs_enable(pst_mac_device);
    }
    /*lint -restore */

    return OAL_FALSE;
}
#endif


oal_uint32 mac_vap_set_bssid_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_bssid)
{
    oal_memcopy (pst_mac_vap->auc_bssid, puc_bssid, WLAN_MAC_ADDR_LEN);
    return OAL_SUCC;
}


oal_uint32 mac_vap_set_current_channel_etc(mac_vap_stru *pst_vap, wlan_channel_band_enum_uint8 en_band, oal_uint8 uc_channel)
{
    oal_uint8  uc_channel_idx = 0;
    oal_uint32 ul_ret;

    /* 检查信道号 */
    ul_ret = mac_is_channel_num_valid_etc(en_band, uc_channel);
    if(OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    /* 根据信道号找到索引号 */
    ul_ret = mac_get_channel_idx_from_num_etc(en_band, uc_channel, &uc_channel_idx);
    if (OAL_SUCC != ul_ret)
    {
        return ul_ret;
    }

    pst_vap->st_channel.uc_chan_number = uc_channel;
    pst_vap->st_channel.en_band        = en_band;
    pst_vap->st_channel.uc_chan_idx         = uc_channel_idx;

    //pst_vap->pst_mib_info->st_wlan_mib_phy_dsss.ul_dot11CurrentChannel = uc_channel_idx;
    return OAL_SUCC;
}


oal_void  mac_vap_state_change_etc(mac_vap_stru *pst_mac_vap, mac_vap_state_enum_uint8 en_vap_state)
{
#if IS_HOST
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_state_change_etc:from[%d]to[%d]}",pst_mac_vap->en_vap_state,en_vap_state);
#endif
    pst_mac_vap->en_vap_state = en_vap_state;
}


oal_bool_enum_uint8  mac_vap_check_bss_cap_info_phy_ap_etc(oal_uint16 us_cap_info,mac_vap_stru *pst_mac_vap)
{
    mac_cap_info_stru  *pst_cap_info = (mac_cap_info_stru *)(&us_cap_info);

    if (WLAN_BAND_2G != pst_mac_vap->st_channel.en_band)
    {
        return OAL_TRUE;
    }

    /* PBCC */
    if ((OAL_FALSE == mac_mib_get_PBCCOptionImplemented(pst_mac_vap)) &&
       (1 == pst_cap_info->bit_pbcc))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_check_bss_cap_info_phy_ap_etc::PBCC is different.}");
    }

    /* Channel Agility */
    if ((OAL_FALSE == mac_mib_get_ChannelAgilityPresent(pst_mac_vap)) &&
       (1 == pst_cap_info->bit_channel_agility))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_check_bss_cap_info_phy_ap_etc::Channel Agility is different.}");
    }

    /* DSSS-OFDM Capabilities */
    if ((OAL_FALSE == mac_mib_get_DSSSOFDMOptionActivated(pst_mac_vap)) &&
       (1 == pst_cap_info->bit_dsss_ofdm))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_check_bss_cap_info_phy_ap_etc::DSSS-OFDM Capabilities is different.}");
    }

    return OAL_TRUE;
}


oal_void  mac_vap_get_bandwidth_cap_etc(mac_vap_stru *pst_mac_vap, wlan_bw_cap_enum_uint8 *pen_cap)
{
    wlan_bw_cap_enum_uint8       en_band_cap = WLAN_BW_CAP_20M;

    if (WLAN_BAND_WIDTH_40PLUS == MAC_VAP_GET_CAP_BW(pst_mac_vap) || WLAN_BAND_WIDTH_40MINUS == MAC_VAP_GET_CAP_BW(pst_mac_vap))
    {
        en_band_cap = WLAN_BW_CAP_40M;
    }
    else if (MAC_VAP_GET_CAP_BW(pst_mac_vap) >= WLAN_BAND_WIDTH_80PLUSPLUS && MAC_VAP_GET_CAP_BW(pst_mac_vap) <= WLAN_BAND_WIDTH_80MINUSMINUS)
    {
        en_band_cap = WLAN_BW_CAP_80M;
    }
#ifdef _PRE_WLAN_FEATURE_160M
    else if(MAC_VAP_GET_CAP_BW(pst_mac_vap) >= WLAN_BAND_WIDTH_160PLUSPLUSPLUS && MAC_VAP_GET_CAP_BW(pst_mac_vap) <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS)
    {
        en_band_cap = WLAN_BW_CAP_160M;
    }
#endif

    *pen_cap = en_band_cap;
}


oal_uint8 mac_vap_get_bandwith(wlan_bw_cap_enum_uint8 en_dev_cap, wlan_channel_bandwidth_enum_uint8 en_bss_cap)
{
    wlan_channel_bandwidth_enum_uint8 en_band_with = WLAN_BAND_WIDTH_20M;

    if (WLAN_BAND_WIDTH_BUTT <= en_bss_cap)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "mac_vap_get_bandwith:bss cap is invaild en_dev_cap[%d] to en_bss_cap[%d]", en_dev_cap, en_bss_cap);
        return en_band_with;
    }

    switch (en_dev_cap)
    {
        case WLAN_BW_CAP_20M:
            break;

        case WLAN_BW_CAP_40M:
            if (WLAN_BAND_WIDTH_40MINUS >= en_bss_cap)
            {
                en_band_with = en_bss_cap;
            }
            else if ((WLAN_BAND_WIDTH_80PLUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_80PLUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_40PLUS;
            }
            else if ((WLAN_BAND_WIDTH_80MINUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_80MINUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_40MINUS;
            }

        #ifdef _PRE_WLAN_FEATURE_160M
            else if ((WLAN_BAND_WIDTH_160PLUSPLUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_160PLUSMINUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_40PLUS;
            }
            else if ((WLAN_BAND_WIDTH_160MINUSPLUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_160MINUSMINUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_40MINUS;
            }
        #endif
            break;

        case WLAN_BW_CAP_80M:
            if (WLAN_BAND_WIDTH_80MINUSMINUS >= en_bss_cap)
            {
                en_band_with = en_bss_cap;
            }
#ifdef _PRE_WLAN_FEATURE_160M
            else if ((WLAN_BAND_WIDTH_160PLUSPLUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_160PLUSPLUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_80PLUSPLUS;
            }
            else if ((WLAN_BAND_WIDTH_160PLUSMINUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_160PLUSMINUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_80PLUSMINUS;
            }
            else if ((WLAN_BAND_WIDTH_160MINUSPLUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_160MINUSPLUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_80MINUSPLUS;
            }
            else if ((WLAN_BAND_WIDTH_160MINUSMINUSPLUS <= en_bss_cap) && (WLAN_BAND_WIDTH_160MINUSMINUSMINUS >= en_bss_cap))
            {
                en_band_with = WLAN_BAND_WIDTH_80MINUSMINUS;
            }
            break;

        case WLAN_BW_CAP_160M:
            if (WLAN_BAND_WIDTH_BUTT > en_bss_cap)
            {
                en_band_with = en_bss_cap;
            }
#endif
            break;
        default:
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "mac_vap_get_bandwith: bandwith en_dev_cap[%d] to en_bss_cap[%d]", en_dev_cap, en_bss_cap);
            break;
    }

    return en_band_with;
}

oal_uint32 mac_dump_protection_etc(mac_vap_stru *pst_mac_vap, oal_uint8* puc_param)
{
    mac_h2d_protection_stru     *pst_h2d_prot;
    mac_protection_stru         *pst_protection;

    if (OAL_PTR_NULL == puc_param)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_h2d_prot = (mac_h2d_protection_stru *)puc_param;
    pst_protection = &pst_h2d_prot->st_protection;

    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "en_dot11RIFSMode=%d en_dot11LSIGTXOPFullProtectionActivated=%d en_dot11NonGFEntitiesPresent=%d en_protection_mode=%d\r\n",
            pst_h2d_prot->en_dot11RIFSMode, pst_h2d_prot->en_dot11LSIGTXOPFullProtectionActivated,
            pst_h2d_prot->en_dot11NonGFEntitiesPresent, pst_protection->en_protection_mode);
    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "uc_obss_non_erp_aging_cnt=%d uc_obss_non_ht_aging_cnt=%d bit_auto_protection=%d bit_obss_non_erp_present=%d\r\n",
            pst_protection->uc_obss_non_erp_aging_cnt, pst_protection->uc_obss_non_ht_aging_cnt,
            pst_protection->bit_auto_protection, pst_protection->bit_obss_non_erp_present);
    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "bit_obss_non_ht_present=%d bit_rts_cts_protect_mode=%d bit_lsig_txop_protect_mode=%d uc_sta_no_short_slot_num=%d\r\n",
            pst_protection->bit_obss_non_ht_present, pst_protection->bit_rts_cts_protect_mode,
            pst_protection->bit_lsig_txop_protect_mode, pst_protection->uc_sta_no_short_slot_num);
    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "uc_sta_no_short_preamble_num=%d uc_sta_non_erp_num=%d uc_sta_non_ht_num=%d uc_sta_non_gf_num=%d\r\n",
            pst_protection->uc_sta_no_short_preamble_num, pst_protection->uc_sta_non_erp_num,
            pst_protection->uc_sta_non_ht_num, pst_protection->uc_sta_non_gf_num);
    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "uc_sta_20M_only_num=%d uc_sta_no_40dsss_cck_num=%d uc_sta_no_lsig_txop_num=%d\r\n",
            pst_protection->uc_sta_20M_only_num, pst_protection->uc_sta_no_40dsss_cck_num,
            pst_protection->uc_sta_no_lsig_txop_num);

    return OAL_SUCC;
}


#if 0
oal_void  mac_vap_set_p2p_channel(mac_vap_stru *pst_mac_vap, mac_channel_stru * pst_p2p_home_channel)
{
    /*
    TBD
    */
}
#endif

wlan_prot_mode_enum_uint8 mac_vap_get_user_protection_mode_etc(mac_vap_stru *pst_mac_vap_sta, mac_user_stru *pst_mac_user)
{
    wlan_prot_mode_enum_uint8           en_protection_mode = WLAN_PROT_NO;

    if ((OAL_PTR_NULL == pst_mac_vap_sta) || (OAL_PTR_NULL == pst_mac_user))
    {
        return en_protection_mode;
    }

    /*在2G频段下，如果AP发送的beacon帧ERP ie中Use Protection bit置为1，则将保护级别设置为ERP保护*/
    if ((WLAN_BAND_2G == pst_mac_vap_sta->st_channel.en_band) && (OAL_TRUE == pst_mac_user->st_cap_info.bit_erp_use_protect))
    {
        en_protection_mode = WLAN_PROT_ERP;
    }

    /*如果AP发送的beacon帧ht operation ie中ht protection字段为mixed或non-member，则将保护级别设置为HT保护*/
    else if ((WLAN_MIB_HT_NON_HT_MIXED == pst_mac_user->st_ht_hdl.bit_HT_protection)
              ||(WLAN_MIB_HT_NONMEMBER_PROTECTION == pst_mac_user->st_ht_hdl.bit_HT_protection))
    {
        en_protection_mode = WLAN_PROT_HT;
    }

    /*如果AP发送的beacon帧ht operation ie中non-gf sta present字段为1，则将保护级别设置为GF保护*/
    else if (OAL_TRUE == pst_mac_user->st_ht_hdl.bit_nongf_sta_present)
    {
        en_protection_mode = WLAN_PROT_GF;
    }

    /*剩下的情况不做保护*/
    else
    {
        en_protection_mode = WLAN_PROT_NO;
    }

    return en_protection_mode;
}

oal_bool_enum mac_protection_lsigtxop_check_etc(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru  *pst_mac_user = OAL_PTR_NULL;

    /*如果不是11n站点，则不支持lsigtxop保护*/
    if ((WLAN_HT_MODE != pst_mac_vap->en_protocol)
        && (WLAN_HT_ONLY_MODE != pst_mac_vap->en_protocol)
        && (WLAN_HT_11G_MODE != pst_mac_vap->en_protocol))
    {
        return OAL_FALSE;
    }

    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        pst_mac_user = (mac_user_stru *)mac_res_get_mac_user_etc(pst_mac_vap->us_assoc_vap_id); /*user保存的是AP的信息*/
        if ((OAL_PTR_NULL == pst_mac_user) || (OAL_FALSE == pst_mac_user->st_ht_hdl.bit_lsig_txop_protection_full_support))
        {
            return OAL_FALSE;
        }
        else
        {
            return OAL_TRUE;
        }
    }
    /*lint -e644*/
    /*BSS 中所有站点都支持Lsig txop protection, 则使用Lsig txop protection机制，开销小, AP和STA采用不同的判断*/
    if ((WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode) && (OAL_TRUE == mac_mib_get_LsigTxopFullProtectionActivated(pst_mac_vap)))
    {
        return OAL_TRUE;
    }
    else
    {
        return OAL_FALSE;
    }
    /*lint +e644*/

}

oal_void mac_protection_set_lsig_txop_mechanism_etc(mac_vap_stru *pst_mac_vap, oal_switch_enum_uint8 en_flag)
{
    /*数据帧/管理帧发送时候，需要根据bit_lsig_txop_protect_mode值填写发送描述符中的L-SIG TXOP enable位*/
    pst_mac_vap->st_protection.bit_lsig_txop_protect_mode = en_flag;
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "mac_protection_set_lsig_txop_mechanism_etc:on[%d]?",en_flag);
}

oal_void mac_protection_set_rts_tx_param_etc(mac_vap_stru *pst_mac_vap,oal_switch_enum_uint8 en_flag,
                                        wlan_prot_mode_enum_uint8 en_prot_mode,mac_cfg_rts_tx_param_stru *pst_rts_tx_param)
{
    /*只有启用erp保护时候，RTS[0~2]速率才设为5.5Mpbs(11b), 其余时候都为24Mpbs(leagcy ofdm) */
    if ((WLAN_PROT_ERP == en_prot_mode) && (OAL_SWITCH_ON == en_flag))
    {
        pst_rts_tx_param->en_band = WLAN_BAND_2G;

        pst_rts_tx_param->auc_protocol_mode[0]    = WLAN_11B_PHY_PROTOCOL_MODE;
        pst_rts_tx_param->auc_rate[0]             = WLAN_SHORT_11b_11_M_BPS;
        pst_rts_tx_param->auc_protocol_mode[1]    = WLAN_11B_PHY_PROTOCOL_MODE;
        pst_rts_tx_param->auc_rate[1]             = WLAN_SHORT_11b_11_M_BPS;
        pst_rts_tx_param->auc_protocol_mode[2]    = WLAN_11B_PHY_PROTOCOL_MODE;
        pst_rts_tx_param->auc_rate[2]             = WLAN_SHORT_11b_11_M_BPS;
        pst_rts_tx_param->auc_protocol_mode[3]    = WLAN_11B_PHY_PROTOCOL_MODE;
        pst_rts_tx_param->auc_rate[3]             = WLAN_LONG_11b_1_M_BPS;

    }
    else
    {
        pst_rts_tx_param->en_band = pst_mac_vap->st_channel.en_band;

        /* RTS[0~2]设为24Mbps */
        pst_rts_tx_param->auc_protocol_mode[0]    = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        pst_rts_tx_param->auc_rate[0]             = WLAN_LEGACY_OFDM_24M_BPS;
        pst_rts_tx_param->auc_protocol_mode[1]    = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        pst_rts_tx_param->auc_rate[1]             = WLAN_LEGACY_OFDM_24M_BPS;
        pst_rts_tx_param->auc_protocol_mode[2]    = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
        pst_rts_tx_param->auc_rate[2]             = WLAN_LEGACY_OFDM_24M_BPS;

        /* 2G的RTS[3]设为1Mbps */
        if (WLAN_BAND_2G == pst_rts_tx_param->en_band)
        {
            pst_rts_tx_param->auc_protocol_mode[3]    = WLAN_11B_PHY_PROTOCOL_MODE;
            pst_rts_tx_param->auc_rate[3]             = WLAN_LONG_11b_1_M_BPS;
        }
        /* 5G的RTS[3]设为24Mbps */
        else
        {
            pst_rts_tx_param->auc_protocol_mode[3]    = WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE;
            pst_rts_tx_param->auc_rate[3]             = WLAN_LEGACY_OFDM_24M_BPS;
        }
    }
}
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

oal_uint32 mac_vap_init_pmf(mac_vap_stru  *pst_mac_vap, oal_uint8 uc_pmf_cap, oal_uint8 uc_mgmt_proteced)
{
    switch(uc_pmf_cap)
    {
        case MAC_PMF_DISABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
        }
        break;
        case MAC_PMF_ENABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
        }
        break;
        case MAC_PMF_REQUIRED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_TRUE);
        }
        break;
        default:
        {
            return OAL_FAIL;
        }
    }

    if (MAC_NL80211_MFP_REQUIRED == uc_mgmt_proteced)
    {
        pst_mac_vap->en_user_pmf_cap = OAL_TRUE;
    }
    else
    {
        pst_mac_vap->en_user_pmf_cap = OAL_FALSE;
    }

    return OAL_SUCC;
}

#endif


oal_uint32 mac_vap_add_wep_key(mac_vap_stru *pst_mac_vap, oal_uint8 us_key_idx, oal_uint8 uc_key_len, oal_uint8 *puc_key)
{
    mac_user_stru                   *pst_multi_user        = OAL_PTR_NULL;
    wlan_priv_key_param_stru        *pst_wep_key           = OAL_PTR_NULL;
    oal_uint32                       ul_cipher_type        = WLAN_CIPHER_SUITE_WEP40;
    oal_uint8                        uc_wep_cipher_type    = WLAN_80211_CIPHER_SUITE_WEP_40;

    /*wep 密钥最大为4个*/
    if(us_key_idx >= WLAN_MAX_WEP_KEY_COUNT)
    {
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    switch (uc_key_len)
    {
        case WLAN_WEP40_KEY_LEN:
            uc_wep_cipher_type = WLAN_80211_CIPHER_SUITE_WEP_40;
            ul_cipher_type     = WLAN_CIPHER_SUITE_WEP40;
            break;
        case WLAN_WEP104_KEY_LEN:
            uc_wep_cipher_type = WLAN_80211_CIPHER_SUITE_WEP_104;
            ul_cipher_type     = WLAN_CIPHER_SUITE_WEP104;
            break;
        default:
            return OAL_ERR_CODE_SECURITY_KEY_LEN;
    }

    /* WEP密钥信息记录到组播用户中*/
    pst_multi_user = mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);

    /*初始化组播用户的安全信息*/
    pst_multi_user->st_key_info.en_cipher_type     = uc_wep_cipher_type;
    pst_multi_user->st_key_info.uc_default_index   = us_key_idx;
    pst_multi_user->st_key_info.uc_igtk_key_index  = 0xff;/* wep时设置为无效 */
    pst_multi_user->st_key_info.bit_gtk            = 0;

    pst_wep_key   = &pst_multi_user->st_key_info.ast_key[us_key_idx];

    pst_wep_key->ul_cipher        = ul_cipher_type;
    pst_wep_key->ul_key_len       = (oal_uint32)uc_key_len;

    oal_memcopy(pst_wep_key->auc_key, puc_key, WLAN_WPA_KEY_LEN);

    /* TBD 挪出去 初始化组播用户的发送信息 */
    pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type      = us_key_idx + HAL_KEY_TYPE_PTK;
    pst_multi_user->st_user_tx_info.st_security.en_cipher_protocol_type = uc_wep_cipher_type;

    return OAL_SUCC;
}


oal_uint32 mac_vap_init_privacy_etc(mac_vap_stru *pst_mac_vap, mac_conn_security_stru *pst_conn_sec)
{
    mac_crypto_settings_stru           *pst_crypto;
    oal_uint32                          ul_ret  = OAL_SUCC;

    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_FALSE);
    /* 初始化 RSNActive 为FALSE */
    mac_mib_set_rsnaactivated(pst_mac_vap, OAL_FALSE);
    /* 清除加密套件信息 */
    mac_mib_init_rsnacfg_suites(pst_mac_vap);

    pst_mac_vap->st_cap_flag.bit_wpa  = OAL_FALSE;
    pst_mac_vap->st_cap_flag.bit_wpa2 = OAL_FALSE;

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    ul_ret = mac_vap_init_pmf(pst_mac_vap, pst_conn_sec->en_pmf_cap, pst_conn_sec->en_mgmt_proteced);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{mac_11i_init_privacy::mac_11w_init_privacy failed[%d].}", ul_ret);
        return ul_ret;
    }
#endif
    /* 不加密 */
    if (OAL_FALSE == pst_conn_sec->en_privacy)
    {
        return OAL_SUCC;
    }

    /* WEP加密 */
    if (0 != pst_conn_sec->uc_wep_key_len)
    {
        return mac_vap_add_wep_key(pst_mac_vap, pst_conn_sec->uc_wep_key_index, pst_conn_sec->uc_wep_key_len, pst_conn_sec->auc_wep_key);
    }

    /* WPA/WPA2加密 */
    pst_crypto = &(pst_conn_sec->st_crypto);

    /* 初始化RSNA mib 为 TRUR */
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);
    mac_mib_set_rsnaactivated(pst_mac_vap, OAL_TRUE);

    /* 初始化单播密钥套件 */
    if (pst_crypto->ul_wpa_versions == WITP_WPA_VERSION_1)
    {
        pst_mac_vap->st_cap_flag.bit_wpa = OAL_TRUE;
        mac_mib_set_wpa_pair_suites(pst_mac_vap, pst_crypto->aul_pair_suite);
        mac_mib_set_wpa_akm_suites(pst_mac_vap, pst_crypto->aul_akm_suite);
        mac_mib_set_wpa_group_suite(pst_mac_vap, pst_crypto->ul_group_suite);
    }
    else if (pst_crypto->ul_wpa_versions == WITP_WPA_VERSION_2)
    {
        pst_mac_vap->st_cap_flag.bit_wpa2 = OAL_TRUE;
        mac_mib_set_rsn_pair_suites(pst_mac_vap, pst_crypto->aul_pair_suite);
        mac_mib_set_rsn_akm_suites(pst_mac_vap, pst_crypto->aul_akm_suite);
        mac_mib_set_rsn_group_suite(pst_mac_vap, pst_crypto->ul_group_suite);
        mac_mib_set_rsn_group_mgmt_suite(pst_mac_vap, pst_crypto->ul_group_mgmt_suite);
    }

    if (OAL_PTR_NULL != g_st_mac_vap_rom_cb.init_privacy_cb)
    {
        g_st_mac_vap_rom_cb.init_privacy_cb(pst_mac_vap, pst_conn_sec);
    }

    return OAL_SUCC;
}

oal_uint32 mac_mib_set_wep_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_id, oal_uint8 uc_key_value)
{
    if (uc_key_id >= WLAN_NUM_DOT11WEPDEFAULTKEYVALUE)
    {
        return OAL_FAIL;
    }

    /*初始化wep相关MIB信息*/
    oal_memset(pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.ast_wlan_mib_wep_dflt_key[uc_key_id].auc_dot11WEPDefaultKeyValue, 0, WLAN_MAX_WEP_STR_SIZE);
    pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.ast_wlan_mib_wep_dflt_key[uc_key_id].auc_dot11WEPDefaultKeyValue[WLAN_WEP_SIZE_OFFSET] = uc_key_value;
    return OAL_SUCC;
}


mac_user_stru  *mac_vap_get_user_by_addr_etc(mac_vap_stru *pst_mac_vap, oal_uint8  *puc_mac_addr)
{
    oal_uint32              ul_ret;
    oal_uint16              us_user_idx   = 0xffff;
    mac_user_stru          *pst_mac_user = OAL_PTR_NULL;

    /*根据mac addr找到sta索引*/
    ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_mac_addr, &us_user_idx);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_vap_get_user_by_addr_etc::find_user_by_macaddr failed[%d].}", ul_ret);
        if(OAL_PTR_NULL != puc_mac_addr)
        {
            OAM_WARNING_LOG3(0, OAM_SF_ANY, "{mac_vap_get_user_by_addr_etc::mac[%x:XX:XX:XX:%x:%x] cant be found!}",
                                    puc_mac_addr[0], puc_mac_addr[4], puc_mac_addr[5]);
        }
        return OAL_PTR_NULL;
    }

    /*根据sta索引找到user内存区域*/
    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user_etc(us_user_idx);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_vap_get_user_by_addr_etc::user[%d] ptr null.}", us_user_idx);
    }

    return pst_mac_user;
}


oal_uint32 mac_vap_set_security(mac_vap_stru *pst_mac_vap, mac_beacon_param_stru *pst_beacon_param)
{
    mac_user_stru                        *pst_multi_user;
    mac_crypto_settings_stru              st_crypto;
    oal_uint16                            us_rsn_cap;
    oal_uint32                            ul_ret = OAL_SUCC;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_beacon_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{mac_vap_add_beacon::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 清除之前的加密配置信息 */
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_FALSE);
    mac_mib_set_rsnaactivated(pst_mac_vap, OAL_FALSE);
    mac_mib_init_rsnacfg_suites(pst_mac_vap);
    pst_mac_vap->st_cap_flag.bit_wpa  = OAL_FALSE;
    pst_mac_vap->st_cap_flag.bit_wpa2 = OAL_FALSE;
    mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
    mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_FALSE);

    /* 清除组播密钥信息 */
    pst_multi_user = mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_add_beacon::pst_multi_user[%d] null.}",
            pst_mac_vap->us_multi_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == pst_beacon_param->en_privacy)
    {
        /* 只在非加密场景下清除，加密场景会重新设置覆盖 */
        mac_user_init_key_etc(pst_multi_user);
        pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type = WLAN_KEY_TYPE_TX_GTK;
        return OAL_SUCC;
    }

    /* 使能加密 */
    mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);

    OAL_MEMZERO(&st_crypto, OAL_SIZEOF(mac_crypto_settings_stru));

    if (MAC_EID_VENDOR == pst_beacon_param->auc_wpa_ie[0])
    {
        pst_mac_vap->st_cap_flag.bit_wpa = OAL_TRUE;
        mac_mib_set_rsnaactivated(pst_mac_vap, OAL_TRUE);
        mac_mib_set_AuthenticationMode(pst_mac_vap, WLAN_WITP_AUTH_OPEN_SYSTEM);
        mac_ie_get_wpa_cipher(pst_beacon_param->auc_wpa_ie, &st_crypto);
        mac_mib_set_wpa_group_suite(pst_mac_vap, st_crypto.ul_group_suite);
        mac_mib_set_wpa_pair_suites(pst_mac_vap, st_crypto.aul_pair_suite);
        mac_mib_set_wpa_akm_suites(pst_mac_vap, st_crypto.aul_akm_suite);
    }

    if (MAC_EID_RSN == pst_beacon_param->auc_rsn_ie[0])
    {
        pst_mac_vap->st_cap_flag.bit_wpa2 = OAL_TRUE;
        mac_mib_set_rsnaactivated(pst_mac_vap, OAL_TRUE);
        mac_mib_set_AuthenticationMode(pst_mac_vap, WLAN_WITP_AUTH_OPEN_SYSTEM);
        ul_ret = mac_ie_get_rsn_cipher(pst_beacon_param->auc_rsn_ie, &st_crypto);
        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
        us_rsn_cap = mac_get_rsn_capability_etc(pst_beacon_param->auc_rsn_ie);
        mac_mib_set_rsn_group_suite(pst_mac_vap, st_crypto.ul_group_suite);
        mac_mib_set_rsn_pair_suites(pst_mac_vap, st_crypto.aul_pair_suite);
        mac_mib_set_rsn_akm_suites(pst_mac_vap, st_crypto.aul_akm_suite);
        mac_mib_set_rsn_group_mgmt_suite(pst_mac_vap, st_crypto.ul_group_mgmt_suite);
        /* RSN 能力 */
        mac_mib_set_dot11RSNAMFPR(pst_mac_vap, (us_rsn_cap & BIT6) ? OAL_TRUE : OAL_FALSE);
        mac_mib_set_dot11RSNAMFPC(pst_mac_vap, (us_rsn_cap & BIT7) ? OAL_TRUE : OAL_FALSE);
        mac_mib_set_pre_auth_actived(pst_mac_vap, us_rsn_cap & BIT0);
        mac_mib_set_rsnacfg_ptksareplaycounters(pst_mac_vap, (oal_uint8)(us_rsn_cap & 0x0C) >> 2);
        mac_mib_set_rsnacfg_gtksareplaycounters(pst_mac_vap, (oal_uint8)(us_rsn_cap & 0x30) >> 4);
    }

    return OAL_SUCC;
}

oal_uint32 mac_vap_add_key_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_uint8 uc_key_id, mac_key_params_stru *pst_key)
{
    oal_uint32                            ul_ret;

    switch ((oal_uint8)pst_key->cipher)
    {
        case WLAN_80211_CIPHER_SUITE_WEP_40:
        case WLAN_80211_CIPHER_SUITE_WEP_104:
            /* 设置mib */
            mac_mib_set_privacyinvoked(pst_mac_vap, OAL_TRUE);
            mac_mib_set_rsnaactivated(pst_mac_vap, OAL_FALSE);
            //TBD 设置组播密钥套件应该放在set default key
            ul_ret = mac_user_add_wep_key_etc(pst_mac_user, uc_key_id, pst_key);
            break;
        case WLAN_80211_CIPHER_SUITE_TKIP:
        case WLAN_80211_CIPHER_SUITE_CCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP:
        case WLAN_80211_CIPHER_SUITE_GCMP_256:
        case WLAN_80211_CIPHER_SUITE_CCMP_256:
            ul_ret = mac_user_add_rsn_key_etc(pst_mac_user, uc_key_id, pst_key);
            break;
        case WLAN_80211_CIPHER_SUITE_BIP:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_128:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_256:
        case WLAN_80211_CIPHER_SUITE_BIP_CMAC_256:
            ul_ret = mac_user_add_bip_key_etc(pst_mac_user, uc_key_id, pst_key);
            break;
        default:
            return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    return ul_ret;
}


oal_uint8 mac_vap_get_default_key_id_etc(mac_vap_stru *pst_mac_vap)
{
    mac_user_stru                *pst_multi_user;
    oal_uint8                     uc_default_key_id;

    /* 根据索引，从组播用户密钥信息中查找密钥 */
    pst_multi_user = mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        /* TBD 调用本函数的地方都没有错误返回处理 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{mac_vap_get_default_key_id_etc::multi_user[%d] NULL}",
            pst_mac_vap->us_multi_user_idx);
        return 0;
    }

    if ((pst_multi_user->st_key_info.en_cipher_type != WLAN_80211_CIPHER_SUITE_WEP_40) &&
        (pst_multi_user->st_key_info.en_cipher_type != WLAN_80211_CIPHER_SUITE_WEP_104))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{mac_vap_get_default_key_id_etc::unexpectd cipher_type[%d]}", pst_multi_user->st_key_info.en_cipher_type);
        return 0;
    }
    uc_default_key_id = pst_multi_user->st_key_info.uc_default_index;
    if (uc_default_key_id >= WLAN_NUM_TK)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{mac_vap_get_default_key_id_etc::unexpectd keyid[%d]}", uc_default_key_id);
        return 0;
    }
    return uc_default_key_id;
}


oal_uint32 mac_vap_set_default_key_etc(mac_vap_stru *pst_mac_vap, oal_uint8  uc_key_index)
{
    wlan_priv_key_param_stru     *pst_wep_key = OAL_PTR_NULL;
    mac_user_stru                *pst_multi_user;

    /* 1.1 如果非wep 加密，则直接返回 */
    if (OAL_TRUE != mac_is_wep_enabled(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    /* 2.1 根据索引，从组播用户密钥信息中查找密钥 */
    pst_multi_user = mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }
    pst_wep_key   = &pst_multi_user->st_key_info.ast_key[uc_key_index];

    if (pst_wep_key->ul_cipher != WLAN_CIPHER_SUITE_WEP40 &&
        pst_wep_key->ul_cipher != WLAN_CIPHER_SUITE_WEP104)
    {
        return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    /* 3.1 更新密钥类型及default id */
    pst_multi_user->st_key_info.en_cipher_type     = (oal_uint8)(pst_wep_key->ul_cipher);
    pst_multi_user->st_key_info.uc_default_index   = uc_key_index;

    /* 4.1 设置mib属性 */
    mac_set_wep_default_keyid(pst_mac_vap, uc_key_index);

    return OAL_SUCC;
}


oal_uint32 mac_vap_set_default_mgmt_key_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_key_index)
{
    mac_user_stru                *pst_multi_user;

    /* 管理帧加密信息保存在组播用户中 */
    pst_multi_user = mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        return OAL_ERR_CODE_SECURITY_USER_INVAILD;
    }

    /* keyid校验 */
    if (uc_key_index < WLAN_NUM_TK || uc_key_index > WLAN_MAX_IGTK_KEY_INDEX)
    {
        return OAL_ERR_CODE_SECURITY_KEY_ID;
    }

    switch ((oal_uint8)pst_multi_user->st_key_info.ast_key[uc_key_index].ul_cipher)
    {
        case WLAN_80211_CIPHER_SUITE_BIP:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_128:
        case WLAN_80211_CIPHER_SUITE_BIP_GMAC_256:
        case WLAN_80211_CIPHER_SUITE_BIP_CMAC_256:
            /* 更新IGTK的keyid */
            pst_multi_user->st_key_info.uc_igtk_key_index   = uc_key_index;
            break;
        default:
            return OAL_ERR_CODE_SECURITY_CHIPER_TYPE;
    }

    return OAL_SUCC;
}


void mac_vap_init_user_security_port_etc(mac_vap_stru  *pst_mac_vap,
                                        mac_user_stru *pst_mac_user)
{
    if (OAL_TRUE != mac_mib_get_rsnaactivated(pst_mac_vap))
    {
        mac_user_set_port_etc(pst_mac_user, OAL_TRUE);
        return;
    }
}


oal_uint8* mac_vap_get_mac_addr_etc(mac_vap_stru *pst_mac_vap)
{
#ifdef _PRE_WLAN_FEATURE_P2P
    if (IS_P2P_DEV(pst_mac_vap))
    {
        /* 获取P2P DEV MAC 地址，赋值到probe req 帧中 */
        return mac_mib_get_p2p0_dot11StationID(pst_mac_vap);
    }
    else
#endif /* _PRE_WLAN_FEATURE_P2P */
    {
        /* 设置地址2为自己的MAC地址 */
        return mac_mib_get_StationID(pst_mac_vap);
    }
}
#ifdef _PRE_WLAN_FEATURE_11R

oal_uint32 mac_mib_init_ft_cfg_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mde)
{

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{mac_mib_init_ft_cfg_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{mac_mib_init_ft_cfg_etc::pst_mib_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_WITP_AUTH_FT != mac_mib_get_AuthenticationMode(pst_mac_vap))
    {
        mac_mib_set_ft_trainsistion(pst_mac_vap, OAL_FALSE);
        return OAL_SUCC;
    }

    if ((OAL_PTR_NULL == puc_mde) || (puc_mde[0] != MAC_EID_MOBILITY_DOMAIN) || (puc_mde[1] != 3))
    {
        mac_mib_set_ft_trainsistion(pst_mac_vap, OAL_FALSE);
        return OAL_SUCC;
    }

    mac_mib_set_ft_trainsistion(pst_mac_vap, OAL_TRUE);
    mac_mib_set_ft_mdid(pst_mac_vap, (oal_uint16)((puc_mde[3] << 8) | puc_mde[2]));
    mac_mib_set_ft_over_ds(pst_mac_vap, (puc_mde[4] & BIT0) ? OAL_TRUE : OAL_FALSE);
    mac_mib_set_ft_resource_req(pst_mac_vap, (puc_mde[4] & BIT1) ? OAL_TRUE : OAL_FALSE);

    return OAL_SUCC;
}

oal_uint32 mac_mib_get_md_id_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_mdid)
{

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{mac_mib_init_ft_cfg_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WPA, "{mac_mib_init_ft_cfg_etc::pst_mib_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == mac_mib_get_ft_trainsistion(pst_mac_vap))
    {
        return OAL_FAIL;
    }

    *pus_mdid = mac_mib_get_ft_mdid(pst_mac_vap);
    return OAL_SUCC;
}
#endif //_PRE_WLAN_FEATURE_11R


#ifdef _PRE_WLAN_FEATURE_VOWIFI

oal_uint32 mac_vap_set_vowifi_param_etc(mac_vap_stru *pst_mac_vap, mac_vowifi_cmd_enum_uint8 en_vowifi_cfg_cmd, oal_uint8 uc_value)
{
    oal_int8 c_value;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_VOWIFI, "{mac_vap_set_vowifi_param_etc::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    switch (en_vowifi_cfg_cmd)
    {
        case VOWIFI_SET_MODE:
        {
            /* 异常值 */
            if (uc_value >= VOWIFI_MODE_BUTT)
            {
                OAM_ERROR_LOG1(0, OAM_SF_VOWIFI, "{mac_vap_set_vowifi_param_etc::MODE Value[%d] error!}", uc_value);
                return OAL_ERR_CODE_VOWIFI_SET_INVALID;
            }

            /*  MODE
                0: disable report of rssi change
                1: enable report when rssi lower than threshold(vowifi_low_thres)
                2: enable report when rssi higher than threshold(vowifi_high_thres)
            */
            pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_mode  = uc_value;
            pst_mac_vap->pst_vowifi_cfg_param->uc_cfg_cmd_cnt |= BIT0;

            OAM_WARNING_LOG1(0, OAM_SF_VOWIFI, "{mac_vap_set_vowifi_param_etc::Set vowifi_mode=[%d]!}",
            pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_mode);
            break;
        }
        case VOWIFI_SET_PERIOD:
        {
            /* 异常值 */
            if ((uc_value < MAC_VOWIFI_PERIOD_MIN)||(uc_value > MAC_VOWIFI_PERIOD_MAX))
            {
                OAM_ERROR_LOG1(0, OAM_SF_VOWIFI, "{mac_vap_set_vowifi_param_etc::PERIOD Value[%d] error!}", uc_value);
                return OAL_ERR_CODE_VOWIFI_SET_INVALID;
            }

            /*单位ms,范围【1s，30s】, the period of monitor the RSSI when host suspended */
            pst_mac_vap->pst_vowifi_cfg_param->us_rssi_period_ms = (oal_uint16)(uc_value *  1000);
            pst_mac_vap->pst_vowifi_cfg_param->uc_cfg_cmd_cnt   |= BIT1;

            break;
        }
        case VOWIFI_SET_LOW_THRESHOLD:
        {
            c_value = (oal_int8)uc_value;
            /* 异常值 */
            if ((c_value < MAC_VOWIFI_LOW_THRESHOLD_MIN)||(c_value > MAC_VOWIFI_LOW_THRESHOLD_MAX))
            {
                OAM_ERROR_LOG1(0, OAM_SF_VOWIFI, "{mac_vap_set_vowifi_param_etc::LOW_THRESHOLD Value[%d] error!}", c_value);
                return OAL_ERR_CODE_VOWIFI_SET_INVALID;
            }

            /* [-1, -100],vowifi_low_thres */
            pst_mac_vap->pst_vowifi_cfg_param->c_rssi_low_thres  = c_value;
            pst_mac_vap->pst_vowifi_cfg_param->uc_cfg_cmd_cnt   |= BIT2;

            break;
        }
        case VOWIFI_SET_HIGH_THRESHOLD:
        {
            c_value = (oal_int8)uc_value;
            /* 异常值 */
            if ((c_value < MAC_VOWIFI_HIGH_THRESHOLD_MIN)||(c_value > MAC_VOWIFI_HIGH_THRESHOLD_MAX))
            {
                OAM_ERROR_LOG1(0, OAM_SF_VOWIFI, "{mac_vap_set_vowifi_param_etc::HIGH_THRESHOLD Value[%d] error!}", c_value);
                return OAL_ERR_CODE_VOWIFI_SET_INVALID;
            }

            /* [-1, -100],vowifi_high_thres */
            pst_mac_vap->pst_vowifi_cfg_param->c_rssi_high_thres = c_value;
            pst_mac_vap->pst_vowifi_cfg_param->uc_cfg_cmd_cnt   |= BIT3;

            break;
        }
        case VOWIFI_SET_TRIGGER_COUNT:
        {
            /* 异常值 */
            if ((uc_value < MAC_VOWIFI_TRIGGER_COUNT_MIN)||(uc_value > MAC_VOWIFI_TRIGGER_COUNT_MAX))
            {
                OAM_ERROR_LOG1(0, OAM_SF_VOWIFI, "{mac_vap_set_vowifi_param_etc::TRIGGER_COUNT Value[%d] error!}", uc_value);
                return OAL_ERR_CODE_VOWIFI_SET_INVALID;
            }

            /*【1，100】, the continuous counters of lower or higher than threshold which will trigger the report to host */
            pst_mac_vap->pst_vowifi_cfg_param->uc_trigger_count_thres = uc_value;
            pst_mac_vap->pst_vowifi_cfg_param->uc_cfg_cmd_cnt        |= BIT4;

            break;
        }
        default:
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_set_vowifi_param_etc::invalid cmd = %d!!}", en_vowifi_cfg_cmd);

            break;
    }

    /* 配置命令收集完毕，初始化vowifi相关上报状态 */
    if (BIT0 & pst_mac_vap->pst_vowifi_cfg_param->uc_cfg_cmd_cnt)
    {
        pst_mac_vap->pst_vowifi_cfg_param->uc_cfg_cmd_cnt     = 0;
        pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_reported = OAL_FALSE;
    }

    return OAL_SUCC;

}
#endif /* _PRE_WLAN_FEATURE_VOWIFI */


oal_switch_enum_uint8 mac_vap_protection_autoprot_is_enabled_etc(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->st_protection.bit_auto_protection;
}


oal_void mac_device_set_vap_id_etc(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap,  oal_uint8 uc_vap_idx, wlan_vap_mode_enum_uint8 en_vap_mode, wlan_p2p_mode_enum_uint8 en_p2p_mode, oal_uint8 is_add_vap)
{
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8                       uc_vap_tmp_idx = 0;
    mac_vap_stru                   *pst_tmp_vap;
#endif

    if (is_add_vap)
    {
        /* ?offload???,????HMAC????? */
        pst_mac_device->auc_vap_id[pst_mac_device->uc_vap_num++] = uc_vap_idx;

        /* device?sta???1 */
        if (WLAN_VAP_MODE_BSS_STA == en_vap_mode)
        {
            pst_mac_device->uc_sta_num++;

            /* ???us_assoc_vap_id??????ap??? */
            pst_mac_vap->us_assoc_vap_id = MAC_INVALID_USER_ID;
        }

    #ifdef _PRE_WLAN_FEATURE_P2P
        pst_mac_vap->en_p2p_mode = en_p2p_mode;
        mac_inc_p2p_num_etc(pst_mac_vap);
        if (IS_P2P_GO(pst_mac_vap))
        {
            for (uc_vap_tmp_idx = 0; uc_vap_tmp_idx < pst_mac_device->uc_vap_num; uc_vap_tmp_idx++)
            {
                pst_tmp_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_tmp_idx]);
                if (OAL_PTR_NULL == pst_tmp_vap)
                {
                    OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{dmac_config_add_vap::pst_mac_vap null,vap_idx=%d.}",
                                   pst_mac_device->auc_vap_id[uc_vap_tmp_idx]);
                    continue;
                }

                if ((MAC_VAP_STATE_UP == pst_tmp_vap->en_vap_state) && (pst_tmp_vap != pst_mac_vap))
                {
                    pst_mac_vap->st_channel.en_band        = pst_tmp_vap->st_channel.en_band;
                    pst_mac_vap->st_channel.en_bandwidth   = pst_tmp_vap->st_channel.en_bandwidth;
                    pst_mac_vap->st_channel.uc_chan_number = pst_tmp_vap->st_channel.uc_chan_number;
                    pst_mac_vap->st_channel.uc_chan_idx         = pst_tmp_vap->st_channel.uc_chan_idx;
                    break;
                }
            }
        }
    #endif
    }
    else
    {
        /* ?offload???,????HMAC????? */
        pst_mac_device->auc_vap_id[pst_mac_device->uc_vap_num--] = 0;

        /* device?sta???1 */
        if (WLAN_VAP_MODE_BSS_STA == en_vap_mode)
        {
            pst_mac_device->uc_sta_num--;

            /* ???us_assoc_vap_id??????ap??? */
            pst_mac_vap->us_assoc_vap_id = MAC_INVALID_USER_ID;
        }

    #ifdef _PRE_WLAN_FEATURE_P2P
        pst_mac_vap->en_p2p_mode = en_p2p_mode;
        mac_dec_p2p_num_etc(pst_mac_vap);
    #endif
    }
}


oal_uint32  mac_device_find_up_vap_etc(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);

            *ppst_mac_vap = OAL_PTR_NULL;

            return OAL_ERR_CODE_PTR_NULL;
        }

        if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state
#ifdef _PRE_WLAN_FEATURE_P2P
            || (MAC_VAP_STATE_STA_LISTEN == pst_mac_vap->en_vap_state && pst_mac_vap->us_user_nums > 0)
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
            )
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


mac_vap_stru * mac_device_find_another_up_vap_etc(mac_device_stru *pst_mac_device, oal_uint8 uc_vap_id_self)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }

        if (uc_vap_id_self == pst_mac_vap->uc_vap_id)
        {
            continue;
        }

        if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state
#ifdef _PRE_WLAN_FEATURE_P2P
            || (MAC_VAP_STATE_STA_LISTEN == pst_mac_vap->en_vap_state && pst_mac_vap->us_user_nums > 0)
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
            )
        {
            return pst_mac_vap;
        }
    }

    return OAL_PTR_NULL;
}


oal_uint32  mac_device_find_up_ap_etc(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }

        if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state)&&
            (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode))
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


oal_uint32  mac_device_calc_up_vap_num_etc(mac_device_stru *pst_mac_device)
{
    mac_vap_stru                  *pst_vap;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_ap_num = 0;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_UP == pst_vap->en_vap_state)
            || (MAC_VAP_STATE_PAUSE == pst_vap->en_vap_state)
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_P2P
            || (MAC_VAP_STATE_STA_LISTEN == pst_vap->en_vap_state && pst_vap->us_user_nums > 0)
#endif
            )
        {
            ul_up_ap_num++;
        }
    }

    return ul_up_ap_num;
}


oal_uint32 mac_device_calc_work_vap_num_etc(mac_device_stru *pst_mac_device)
{
    mac_vap_stru *pst_vap;
    oal_uint8 uc_vap_idx;
    oal_uint8 ul_work_vap_num = 0;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "mac_device_calc_work_vap_numv::vap[%d] is null",
            pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_INIT != pst_vap->en_vap_state) && (MAC_VAP_STATE_BUTT != pst_vap->en_vap_state))
        {
            ul_work_vap_num++;
        }
    }

    return ul_work_vap_num;
}


oal_uint32  mac_device_find_up_p2p_go_etc(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state)&&
            (WLAN_P2P_GO_MODE == pst_mac_vap->en_p2p_mode))
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


oal_uint32  mac_device_get_up_vap_num(mac_device_stru *pst_mac_device)
{
    mac_vap_stru                  *pst_vap;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_ap_num = 0;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (MAC_VAP_STATE_UP == pst_vap->en_vap_state)
        {
            ul_up_ap_num++;
        }
    }

    return ul_up_ap_num;
}


oal_uint32  mac_device_find_2up_vap_etc(
                mac_device_stru *pst_mac_device,
                mac_vap_stru   **ppst_mac_vap1,
                mac_vap_stru   **ppst_mac_vap2)
{
    mac_vap_stru                  *pst_vap;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_vap_num = 0;
    mac_vap_stru                  *past_vap[2] = {0};

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if ((MAC_VAP_STATE_UP == pst_vap->en_vap_state)
            || (MAC_VAP_STATE_PAUSE == pst_vap->en_vap_state)
#ifdef _PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_ROAMING == pst_vap->en_vap_state)
#endif //_PRE_WLAN_FEATURE_ROAM
            || (MAC_VAP_STATE_STA_LISTEN == pst_vap->en_vap_state && pst_vap->us_user_nums > 0))
        {
            past_vap[ul_up_vap_num] = pst_vap;
            ul_up_vap_num++;

            if (ul_up_vap_num >=2)
            {
                break;
            }
        }
    }

    if (ul_up_vap_num < 2)
    {
        return OAL_FAIL;
    }

    *ppst_mac_vap1 = past_vap[0];
    *ppst_mac_vap2 = past_vap[1];

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_DBAC

oal_uint32  mac_fcs_dbac_state_check_etc(mac_device_stru *pst_mac_device)
{
    mac_vap_stru *pst_mac_vap1;
    mac_vap_stru *pst_mac_vap2;
    oal_uint32    ul_ret;

    ul_ret = mac_device_find_2up_vap_etc(pst_mac_device, &pst_mac_vap1, &pst_mac_vap2);
    if(OAL_SUCC != ul_ret)
    {
        return MAC_FCS_DBAC_IGNORE;
    }

    if(pst_mac_vap1->st_channel.uc_chan_number == pst_mac_vap2->st_channel.uc_chan_number)
    {
        return MAC_FCS_DBAC_NEED_CLOSE;
    }

#ifdef _PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE
    //E5 不支持同频段不同信道的场景，所以如果频段相同信道不同，也不需要开启DBAC
    if(pst_mac_vap1->st_channel.en_band== pst_mac_vap2->st_channel.en_band)
    {
        return MAC_FCS_DBAC_NEED_CLOSE;
    }
#endif

    return MAC_FCS_DBAC_NEED_OPEN;
}
#endif

oal_uint32  mac_device_find_up_sta_etc(mac_device_stru *pst_mac_device, mac_vap_stru **ppst_mac_vap)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);

            *ppst_mac_vap = OAL_PTR_NULL;

            return OAL_ERR_CODE_PTR_NULL;
        }

        if ((MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state || MAC_VAP_STATE_PAUSE == pst_mac_vap->en_vap_state)&&
            (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode))
        {
            *ppst_mac_vap = pst_mac_vap;

            return OAL_SUCC;
        }
    }

    *ppst_mac_vap = OAL_PTR_NULL;

    return OAL_FAIL;
}


oal_uint32  mac_device_is_p2p_connected_etc(mac_device_stru *pst_mac_device)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            OAM_WARNING_LOG1(0, OAM_SF_P2P, "vap is null! vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }
        if((IS_P2P_GO(pst_mac_vap)||IS_P2P_CL(pst_mac_vap))&&
           (pst_mac_vap->us_user_nums > 0))
        {
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

mac_vap_stru *mac_find_main_proxysta(mac_device_stru *pst_mac_device)
{
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_mac_vap;

    if(!pst_mac_device || !mac_is_proxysta_enabled(pst_mac_device))
    {
        return OAL_PTR_NULL;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
        {
            continue;
        }

        if (pst_mac_vap->en_vap_mode != WLAN_VAP_MODE_BSS_STA)
        {
            continue;
        }

        if (mac_vap_is_msta(pst_mac_vap))
        {
            return pst_mac_vap;
        }
    }

    return OAL_PTR_NULL;
}
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK

oal_uint32 mac_vap_update_hilink_white_list(mac_vap_stru *pst_mac_vap, oal_hilink_white_node_stru *pst_white_node)
{
    mac_okc_white_list_member_stru         *pst_mac_hilink_white_temp;
    mac_okc_ssid_hidden_white_list_stru    *pst_okc_ssid_hidden_white_list;
    oal_dlist_head_stru                    *pst_entry;
    oal_dlist_head_stru                    *pst_entry_temp;
    mac_okc_white_list_member_stru         *pst_mac_okc_ssid_hidden_white;

    /* 入参检查 */
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_white_node))
    {
        OAM_WARNING_LOG0(0, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::null pst_hmac_vap or pst_white_node}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_okc_ssid_hidden_white_list = &(pst_mac_vap->st_okc_ssid_hidden_white_list);
#ifndef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    /*hilink白名单*/
    if ((0 == pst_okc_ssid_hidden_white_list->uc_white_list_member_nums) && (pst_white_node->en_white_list_operate == OAL_WHITE_LIST_OPERATE_DEL))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::no sta in white list while del member.}");
        return OAL_FAIL;
    }
#else
    /*hilink白名单*/
    if ((pst_white_node->uc_flag & (oal_uint8)BIT0) && (0 == pst_okc_ssid_hidden_white_list->uc_white_list_member_nums) && (pst_white_node->en_white_list_operate == OAL_WHITE_LIST_OPERATE_DEL))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::no sta in white list while del member.}");
        return OAL_FAIL;
    }
    /*已连接的STA列表*/
    if ((pst_white_node->uc_flag & (oal_uint8)BIT1) && (0 == pst_okc_ssid_hidden_white_list->uc_assoc_sta_member_nums) && (pst_white_node->en_white_list_operate == OAL_WHITE_LIST_OPERATE_DEL))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::no sta in associate list while del member.}");
        return OAL_FAIL;
    }
#endif
    if (oal_is_broadcast_ether_addr(pst_white_node->auc_mac))
    {
        if (pst_white_node->en_white_list_operate == OAL_WHITE_LIST_OPERATE_DEL)
        {
            oal_spin_lock(&(pst_okc_ssid_hidden_white_list->st_lock));
            OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_temp, &(pst_okc_ssid_hidden_white_list->st_list_head))
            {
                pst_mac_okc_ssid_hidden_white = OAL_DLIST_GET_ENTRY(pst_entry, mac_okc_white_list_member_stru, st_dlist);
                oal_dlist_delete_entry(&pst_mac_okc_ssid_hidden_white->st_dlist);
                pst_okc_ssid_hidden_white_list->uc_white_list_member_nums--;
                OAL_MEM_FREE(pst_mac_okc_ssid_hidden_white, OAL_TRUE);
            }
            oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::delete broadcase ether addr!}");
        }
        else
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::add broadcast ether addr!}");
        }
        return OAL_SUCC;
    }

    /* 对链表操作前加锁 */
    oal_spin_lock(&(pst_okc_ssid_hidden_white_list->st_lock));

    /* 遍历链表，看目标mac是否在白名单列表内 */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_entry_temp, &(pst_okc_ssid_hidden_white_list->st_list_head))
    {
        pst_mac_okc_ssid_hidden_white = OAL_DLIST_GET_ENTRY(pst_entry, mac_okc_white_list_member_stru, st_dlist);

        if (0 == oal_memcmp(pst_mac_okc_ssid_hidden_white->auc_mac_addr, pst_white_node->auc_mac, WLAN_MAC_ADDR_LEN))
        {
            if (pst_white_node->en_white_list_operate == OAL_WHITE_LIST_OPERATE_DEL)
            {
#ifndef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
                oal_dlist_delete_entry(&pst_mac_okc_ssid_hidden_white->st_dlist);
                pst_okc_ssid_hidden_white_list->uc_white_list_member_nums--;
                OAL_MEM_FREE(pst_mac_okc_ssid_hidden_white, OAL_TRUE);
                OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::[xx:xx:%02x:%02x:%02x:%02x] del seccess!}",
                             pst_white_node->auc_mac[2],pst_white_node->auc_mac[3],pst_white_node->auc_mac[4],pst_white_node->auc_mac[5]);
#else
                if (pst_white_node->uc_flag & (oal_uint8)BIT0) /*hilink白名单中删除*/
                {
                    if (pst_mac_okc_ssid_hidden_white->uc_flag & (oal_uint8)BIT0)
                    {
                        pst_okc_ssid_hidden_white_list->uc_white_list_member_nums--;
                    }
                    if (pst_mac_okc_ssid_hidden_white->uc_flag & (oal_uint8)BIT1)
                    {
                        pst_okc_ssid_hidden_white_list->uc_assoc_sta_member_nums--;
                    }
                    oal_dlist_delete_entry(&pst_mac_okc_ssid_hidden_white->st_dlist);
                    OAL_MEM_FREE(pst_mac_okc_ssid_hidden_white, OAL_TRUE);
                    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::[xx:xx:%02x:%02x:%02x:%02x] del seccess!}",
                             pst_white_node->auc_mac[2],pst_white_node->auc_mac[3],pst_white_node->auc_mac[4],pst_white_node->auc_mac[5]);
                }
                else /*AP已关联的sta列表删除*/
                {
                    if (pst_mac_okc_ssid_hidden_white->uc_flag & (oal_uint8)BIT1)
                    {
                        pst_mac_okc_ssid_hidden_white->uc_flag &= ~(oal_uint8)BIT1;
                        pst_okc_ssid_hidden_white_list->uc_assoc_sta_member_nums--;
                        if (0 == (pst_mac_okc_ssid_hidden_white->uc_flag & (oal_uint8)BIT0))
                        {
                            oal_dlist_delete_entry(&pst_mac_okc_ssid_hidden_white->st_dlist);
                            OAL_MEM_FREE(pst_mac_okc_ssid_hidden_white, OAL_TRUE);
                            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::[xx:xx:%02x:%02x:%02x:%02x] del seccess!}",
                            pst_white_node->auc_mac[2],pst_white_node->auc_mac[3],pst_white_node->auc_mac[4],pst_white_node->auc_mac[5]);

                        }
                    }
                }
#endif
            }
            /* 要添加的mac已经在列表内 */
            else
            {
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
                if (pst_white_node->uc_flag & (oal_uint8)BIT1)
                {
                    if (0 == (pst_mac_okc_ssid_hidden_white->uc_flag & (oal_uint8)BIT1))
                    {
                        pst_okc_ssid_hidden_white_list->uc_assoc_sta_member_nums++;
                    }
                    pst_mac_okc_ssid_hidden_white->uc_flag |= (oal_uint8)BIT1;
                    pst_mac_okc_ssid_hidden_white->uc_resp_nums = 0;
                }
                else
                {
                    if (0 == (pst_mac_okc_ssid_hidden_white->uc_flag & (oal_uint8)BIT0))
                    {
                        pst_okc_ssid_hidden_white_list->uc_white_list_member_nums++;
                    }
                    pst_mac_okc_ssid_hidden_white->uc_flag |= (oal_uint8)BIT0;
                }
#endif
                OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::[xx:xx:%02x:%02x:%02x:%02x]already in white list!}",
                             pst_white_node->auc_mac[2],pst_white_node->auc_mac[3],pst_white_node->auc_mac[4],pst_white_node->auc_mac[5]);
            }
            /* 返回之前，释放内存，释放锁 */
            oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
            return OAL_SUCC;
        }
    }

    /* 目标mac不在白名单链表内 */
    if (pst_white_node->en_white_list_operate == OAL_WHITE_LIST_OPERATE_ADD)
    {
#ifndef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
        /*hilink白名单满时，无法继续添加*/
        if (MAC_HILINK_MAX_WHITE_LIST_MEMBER_CNT <= pst_okc_ssid_hidden_white_list->uc_white_list_member_nums)
        {
            oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::add mem failed!white list member out of limit.}");
            return OAL_FAIL;
        }
#else
        /*hilink白名单满时，无法继续添加*/
        if ((pst_white_node->uc_flag & (oal_uint8)BIT0) && (MAC_HILINK_MAX_WHITE_LIST_MEMBER_CNT <= pst_okc_ssid_hidden_white_list->uc_white_list_member_nums))
        {
            oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::add mem failed!white list member out of limit.}");
            return OAL_FAIL;
        }
        /*已连接的STA列表满时，无法继续添加*/
        if ((pst_white_node->uc_flag & (oal_uint8)BIT1) && (MAC_HILINK_MAX_ASSOCIATE_LIST_MEMBER_CNT <= pst_okc_ssid_hidden_white_list->uc_assoc_sta_member_nums))
        {
            oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::add mem failed!associate list member out of limit.}");
            return OAL_FAIL;
        }
#endif

        pst_mac_hilink_white_temp = (mac_okc_white_list_member_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,OAL_SIZEOF(mac_okc_white_list_member_stru),OAL_TRUE);
        /* 申请之后判空处理 */
        if(OAL_PTR_NULL == pst_mac_hilink_white_temp)
        {
            oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::oal_memalloc fail !}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        OAL_MEMZERO(pst_mac_hilink_white_temp, OAL_SIZEOF(mac_okc_white_list_member_stru));
        oal_memcopy(pst_mac_hilink_white_temp->auc_mac_addr, pst_white_node->auc_mac, WLAN_MAC_ADDR_LEN);
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
        pst_mac_hilink_white_temp->uc_flag = pst_white_node->uc_flag;
#endif
        /*添加到okc 隐藏ssid白名单列表的尾部  数目加一 */
        oal_dlist_add_tail(&(pst_mac_hilink_white_temp->st_dlist), &(pst_okc_ssid_hidden_white_list->st_list_head));
#ifndef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
        pst_okc_ssid_hidden_white_list->uc_white_list_member_nums++;
#else
        if (pst_white_node->uc_flag & (oal_uint8)BIT0)
        {
            pst_okc_ssid_hidden_white_list->uc_white_list_member_nums++;
        }
        else
        {
            pst_okc_ssid_hidden_white_list->uc_assoc_sta_member_nums++;
        }
#endif

        /* 解锁 */
        oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::add[xx:xx:%02x:%02x:%02x:%02x]to white list success!}",
                                 pst_white_node->auc_mac[2],pst_white_node->auc_mac[3],pst_white_node->auc_mac[4],pst_white_node->auc_mac[5]);
        return OAL_SUCC;
    }
    else
    {
         /* 解锁 */
        oal_spin_unlock(&(pst_okc_ssid_hidden_white_list->st_lock));
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{mac_vap_update_hilink_white_list::[xx:xx:%02x:%02x:%02x:%02x] not in white list while del!}",
                                 pst_white_node->auc_mac[2],pst_white_node->auc_mac[3],pst_white_node->auc_mac[4],pst_white_node->auc_mac[5]);
        return OAL_SUCC;
    }
}

#endif

#ifdef _PRE_WLAN_FEATURE_SMPS

oal_uint32 mac_device_find_smps_mode_en(mac_device_stru *pst_mac_device,
                                                                 wlan_mib_mimo_power_save_enum_uint8 en_smps_mode)
{
    mac_vap_stru                         *pst_mac_vap = OAL_PTR_NULL;
    oal_uint8                             uc_vap_idx;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_SMPS, "{mac_device_find_smps_mode_en::pst_mac_vap null.}");
            continue;
        }

        /* 存在不支持HT则MIMO，模式不可配置 */
        if (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{mac_device_find_smps_mode_en::exist none ht vap.}");
            return OAL_FALSE;
        }

        /* 存在一个vap MIB不支持SMPS则MIMO */
        if (WLAN_MIB_MIMO_POWER_SAVE_BUTT == mac_vap_get_smps_mode(pst_mac_vap))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{mac_device_find_smps_mode_en::exist no support SMPS vap.}");
            return OAL_FALSE;
        }

        /* 获取当前SMPS模式，若未改变则直接返回，模式不可配置来处理(vap和device smps mode始终保持一致) */
        if (en_smps_mode == mac_vap_get_smps_mode(pst_mac_vap))
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{mac_device_find_smps_mode_en::vap smps mode[%d]unchanged smps mode[%d].}",
                             pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MIMOPowerSave, en_smps_mode);
            return OAL_FALSE;
        }
    }

    return OAL_TRUE;
}
#endif


oal_uint32  mac_fcs_get_prot_mode(mac_vap_stru *pst_src_vap)
{
    return 0;
}


oal_uint32  mac_fcs_get_prot_datarate(mac_vap_stru *pst_src_vap)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151) && (_PRE_TARGET_PRODUCT_TYPE_E5 == _PRE_CONFIG_TARGET_PRODUCT)
    if ((1 == g_l_rf_single_tran) && (2 == g_l_rf_channel_num))
    {
        return 0x004b0123;
    }
    /* 若配置"通道0"或"双通道", 则保护帧只用通道0发送 */
    else
    {
        return 0x004b0113;
    }
#else
     /* 针对非legacy vap或者5G频段, 配置OFDM 6M */
    if ((!IS_LEGACY_VAP(pst_src_vap)) || (WLAN_BAND_5G == pst_src_vap->st_channel.en_band))
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        /* 若配置"通道1", 则保护帧只用通道1发送 */
        if ((1 == g_l_rf_single_tran) && (2 == g_l_rf_channel_num))
        {
            return 0x004b0123;
        }
        /* 若配置"通道0"或"双通道", 则保护帧只用通道0发送 */
        else
#endif
        {
            return 0x004b0113;
        }
    }
    /* 否则, 配置11b 1M  */
    else
    {
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        /* 若配置"通道1", 则保护帧只用通道1发送 */
        if ((1 == g_l_rf_single_tran) && (2 == g_l_rf_channel_num))
        {
            return 0x08000123;
        }
        /* 若配置"通道0"或"双通道", 则保护帧只用通道0发送 */
        else
#endif
        {
            return 0x08000113;
        }
    }
#endif
}

oal_void  mac_device_set_channel_etc(mac_device_stru *pst_mac_device, mac_cfg_channel_param_stru * pst_channel_param)
{
    pst_mac_device->uc_max_channel = pst_channel_param->uc_channel;
    pst_mac_device->en_max_band = pst_channel_param->en_band;
    pst_mac_device->en_max_bandwidth = pst_channel_param->en_bandwidth;
}

oal_void  mac_device_get_channel_etc(mac_device_stru *pst_mac_device, mac_cfg_channel_param_stru * pst_channel_param)
{
    pst_channel_param->uc_channel = pst_mac_device->uc_max_channel;
    pst_channel_param->en_band = pst_mac_device->en_max_band;
    pst_channel_param->en_bandwidth = pst_mac_device->en_max_bandwidth;
}
#ifdef _PRE_WLAN_WEB_CMD_COMM

oal_uint32  mac_oui_to_cipher_group(oal_uint32 cipher_oui)
{
    switch(cipher_oui)
    {
        case MAC_RSN_CHIPER_CCMP:
        case MAC_WPA_CHIPER_CCMP:
            return WPA_CIPHER_CCMP;
        case MAC_RSN_CHIPER_TKIP:
        case MAC_WPA_CHIPER_TKIP:
            return WPA_CIPHER_TKIP;
        default:
            return WPA_CIPHER_NONE;
    }
}


oal_uint32  mac_oui_to_akm_suite(oal_uint32 cipher_oui)
{
    switch(cipher_oui)
    {
        case MAC_WPA_AKM_PSK:
        case MAC_RSN_AKM_PSK:
            return WPA_KEY_MGMT_PSK;
        case MAC_WPA_AKM_PSK_SHA256:
        case MAC_RSN_AKM_PSK_SHA256:
            return WPA_KEY_MGMT_PSK_SHA256;
        case MAC_RSN_AKM_8021X:
            return WPA_KEY_MGMT_IEEE8021X;
        case MAC_RSN_AKM_OSEN:
            return WPA_KEY_MGMT_OSEN;
        default:
            return WPA_KEY_MGMT_NONE;
    }
}
#endif
/*lint -e19*/
//oal_module_symbol(mac_vap_set_p2p_channel);
#ifdef _PRE_WLAN_FEATURE_UAPSD
oal_module_symbol(g_uc_uapsd_cap_etc);
#endif
oal_module_symbol(mac_vap_set_bssid_etc);
oal_module_symbol(mac_vap_set_current_channel_etc);
oal_module_symbol(mac_vap_init_wme_param_etc);
oal_module_symbol(mac_mib_set_station_id_etc);
oal_module_symbol(mac_vap_state_change_etc);
#if 0
oal_module_symbol(mac_mib_get_station_id);
#endif
oal_module_symbol(mac_mib_set_bss_type_etc);
oal_module_symbol(mac_mib_set_ssid_etc);
oal_module_symbol(mac_get_ssid_etc);
oal_module_symbol(mac_mib_set_beacon_period_etc);
oal_module_symbol(mac_mib_set_dtim_period_etc);
#ifdef _PRE_WLAN_FEATURE_UAPSD
oal_module_symbol(mac_vap_set_uapsd_en_etc);
oal_module_symbol(mac_vap_get_uapsd_en_etc);
#endif
#if 0
oal_module_symbol(mac_mib_get_GroupReceivedFrameCount);
oal_module_symbol(mac_mib_set_GroupReceivedFrameCount);
oal_module_symbol(mac_vap_get_user_wme_info);
oal_module_symbol(mac_vap_set_user_wme_info);
#endif
oal_module_symbol(mac_mib_set_shpreamble_etc);
oal_module_symbol(mac_mib_get_shpreamble_etc);
oal_module_symbol(mac_vap_add_assoc_user_etc);
oal_module_symbol(mac_vap_del_user_etc);
oal_module_symbol(mac_vap_exit_etc);
oal_module_symbol(mac_init_mib_etc);
oal_module_symbol(mac_mib_get_ssid_etc);
oal_module_symbol(mac_mib_get_bss_type_etc);
oal_module_symbol(mac_mib_get_beacon_period_etc);
oal_module_symbol(mac_mib_get_dtim_period_etc);
oal_module_symbol(mac_vap_init_rates_etc);
oal_module_symbol(mac_vap_init_by_protocol_etc);
oal_module_symbol(mac_vap_config_vht_ht_mib_by_protocol_etc);
//oal_module_symbol(mac_mib_set_RSNAAuthenticationSuiteSelected);
oal_module_symbol(mac_vap_check_bss_cap_info_phy_ap_etc);
oal_module_symbol(mac_get_wmm_cfg_etc);
#ifdef _PRE_WLAN_FEATURE_EDCA_MULTI_USER_MULTI_AC
oal_module_symbol(mac_get_wmm_cfg_multi_user_multi_ac);
#endif
oal_module_symbol(mac_vap_get_bandwith);
oal_module_symbol(mac_vap_get_bandwidth_cap_etc);
oal_module_symbol(mac_vap_change_mib_by_bandwidth_etc);
oal_module_symbol(mac_vap_init_rx_nss_by_protocol_etc);
oal_module_symbol(mac_dump_protection_etc);
oal_module_symbol(mac_vap_set_aid_etc);
oal_module_symbol(mac_vap_set_al_tx_payload_flag_etc);
oal_module_symbol(mac_vap_set_assoc_id_etc);
oal_module_symbol(mac_vap_set_al_tx_flag_etc);
oal_module_symbol(mac_vap_set_tx_power_etc);
oal_module_symbol(mac_vap_set_uapsd_cap_etc);
oal_module_symbol(mac_vap_set_multi_user_idx_etc);
oal_module_symbol(mac_vap_set_wmm_params_update_count_etc);
oal_module_symbol(mac_vap_set_rifs_tx_on_etc);
#if 0
oal_module_symbol(mac_vap_set_tdls_prohibited);
oal_module_symbol(mac_vap_set_tdls_channel_switch_prohibited);
#endif
oal_module_symbol(mac_vap_set_11ac2g_etc);
oal_module_symbol(mac_vap_set_hide_ssid_etc);
oal_module_symbol(mac_get_p2p_mode_etc);
oal_module_symbol(mac_vap_get_peer_obss_scan_etc);
oal_module_symbol(mac_vap_set_peer_obss_scan_etc);
oal_module_symbol(mac_vap_clear_app_ie_etc);
oal_module_symbol(mac_vap_save_app_ie_etc);
oal_module_symbol(mac_vap_set_rx_nss_etc);
oal_module_symbol(mac_vap_find_user_by_macaddr_etc);
oal_module_symbol(mac_sta_init_bss_rates_etc);
#ifdef _PRE_WLAN_FEATURE_SMPS
oal_module_symbol(mac_vap_get_smps_mode);
oal_module_symbol(mac_vap_get_smps_en);
oal_module_symbol(mac_vap_set_smps);
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
oal_module_symbol(mac_vap_get_txopps);
oal_module_symbol(mac_vap_set_txopps);
oal_module_symbol(mac_vap_update_txopps);
#endif
oal_module_symbol(mac_device_find_user_by_macaddr_etc);
oal_module_symbol(mac_chip_find_user_by_macaddr);
oal_module_symbol(mac_board_find_user_by_macaddr);
oal_module_symbol(mac_vap_init_privacy_etc);
oal_module_symbol(mac_mib_set_wep_etc);
oal_module_symbol(mac_vap_get_user_by_addr_etc);
oal_module_symbol(mac_vap_set_security);
oal_module_symbol(mac_vap_add_key_etc);
oal_module_symbol(mac_vap_get_default_key_id_etc);
oal_module_symbol(mac_vap_set_default_key_etc);
oal_module_symbol(mac_vap_set_default_mgmt_key_etc);
oal_module_symbol(mac_vap_init_user_security_port_etc);
oal_module_symbol(mac_protection_lsigtxop_check_etc);
oal_module_symbol(mac_protection_set_lsig_txop_mechanism_etc);
oal_module_symbol(mac_vap_get_user_protection_mode_etc);
oal_module_symbol(mac_vap_protection_autoprot_is_enabled_etc);
oal_module_symbol(mac_protection_set_rts_tx_param_etc);
oal_module_symbol(mac_vap_get_ap_usr_opern_bandwidth);
oal_module_symbol(mac_vap_bw_mode_to_bw);

#ifdef _PRE_WLAN_FEATURE_VOWIFI
oal_module_symbol(mac_vap_set_vowifi_param_etc);
#endif /* _PRE_WLAN_FEATURE_VOWIFI */

oal_module_symbol(mac_device_set_vap_id_etc);
oal_module_symbol(mac_device_find_up_vap_etc);
oal_module_symbol(mac_device_find_another_up_vap_etc);
oal_module_symbol(mac_device_find_up_ap_etc);
oal_module_symbol(mac_device_calc_up_vap_num_etc);
oal_module_symbol(mac_device_calc_work_vap_num_etc);
oal_module_symbol(mac_device_find_up_p2p_go_etc);
oal_module_symbol(mac_device_find_2up_vap_etc);
oal_module_symbol(mac_device_is_p2p_connected_etc);
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
oal_module_symbol(mac_find_main_proxysta);
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
oal_module_symbol(mac_vap_update_hilink_white_list);
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
oal_module_symbol(mac_device_find_smps_mode_en);
#endif
oal_module_symbol(mac_device_get_channel_etc);
oal_module_symbol(mac_device_set_channel_etc);
oal_module_symbol(mac_fcs_get_prot_mode);
oal_module_symbol(mac_fcs_get_prot_datarate);
#ifdef _PRE_WLAN_WEB_CMD_COMM
oal_module_symbol(mac_oui_to_cipher_group);
oal_module_symbol(mac_oui_to_akm_suite);
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
oal_module_symbol(mac_vap_get_dfs_enable);
#endif

/*lint +e19*/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif



