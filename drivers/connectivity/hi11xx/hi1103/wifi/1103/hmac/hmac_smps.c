


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_config.h"
#include "hmac_smps.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_SMPS_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/
#if 0

oal_uint32 hmac_smps_update_user_capbility(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32   ul_ret;
    wlan_nss_enum_uint8  en_avail_num_spatial_stream;

    switch(pst_mac_user->st_ht_hdl.bit_sm_power_save)
    {
        case WLAN_MIB_MIMO_POWER_SAVE_STATIC:
            en_avail_num_spatial_stream = WLAN_SINGLE_NSS;
            break;
        case WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC:
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_update_user_capbility: user smps mode is DYNAMIC!}");
            en_avail_num_spatial_stream = WLAN_DOUBLE_NSS;
            break;
        case WLAN_MIB_MIMO_POWER_SAVE_MIMO:
            en_avail_num_spatial_stream = WLAN_DOUBLE_NSS;
            break;
        default:
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_update_user_capbility: en_user_smps_mode mode[%d] fail!}",
            pst_mac_user->st_ht_hdl.bit_sm_power_save);
            return OAL_FAIL;
    }

    pst_mac_user->en_avail_num_spatial_stream = en_avail_num_spatial_stream;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_update_user_m2s_event(pst_mac_vap, pst_mac_user);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_update_user_capbility::hmac_config_update_user_m2s_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif

oal_uint32 hmac_smps_update_user_status(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    wlan_mib_mimo_power_save_enum_uint8 en_user_smps_mode;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_mac_user))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SMPS, "{hmac_smps_update_user_status: NULL PTR pst_mac_vap is [%d] and pst_mac_user is [%d].}", pst_mac_vap, pst_mac_user);
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_user_smps_mode = (wlan_mib_mimo_power_save_enum_uint8)pst_mac_user->st_ht_hdl.bit_sm_power_save;

    switch(en_user_smps_mode)
    {
        case WLAN_MIB_MIMO_POWER_SAVE_STATIC:
            mac_user_set_sm_power_save(pst_mac_user, WLAN_MIB_MIMO_POWER_SAVE_STATIC);
            pst_mac_user->en_avail_num_spatial_stream = WLAN_SINGLE_NSS;
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_update_user_status:user smps_mode update STATIC!}");
            break;
        case WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC:
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_update_user_status:user smps_mode update DYNAMIC!}");
            mac_user_set_sm_power_save(pst_mac_user, WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC);
            pst_mac_user->en_avail_num_spatial_stream = OAL_MIN(pst_mac_vap->en_vap_rx_nss,WLAN_DOUBLE_NSS);
            break;
        case WLAN_MIB_MIMO_POWER_SAVE_MIMO:
            mac_user_set_sm_power_save(pst_mac_user, WLAN_MIB_MIMO_POWER_SAVE_MIMO);
            pst_mac_user->en_avail_num_spatial_stream = OAL_MIN(pst_mac_vap->en_vap_rx_nss,WLAN_DOUBLE_NSS);
            break;
        default:
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_update_user_status: en_user_smps_mode mode[%d] fail!}", en_user_smps_mode);
            return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_mgmt_rx_smps_frame(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 *puc_data)
{
    wlan_mib_mimo_power_save_enum_uint8     en_user_smps_mode;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == puc_data))
    {
        OAM_ERROR_LOG3(0, OAM_SF_SMPS, "{hmac_mgmt_rx_smps_frame::null param, 0x%x 0x%x 0x%x.}", pst_mac_vap, pst_hmac_user, puc_data);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    /* 更新STA的sm_power_save field, 获取enable bit */
    if (0 == (puc_data[MAC_ACTION_OFFSET_ACTION + 1] & BIT0))
    {

        en_user_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_MIMO;
    }
    else
    {
        /* 如果SMPS enable,则配置为相应模式(不考虑动态状态更新，动态只支持配置命令配置) */
        if (0 == (puc_data[MAC_ACTION_OFFSET_ACTION + 1] & BIT1))
        {
            /* 静态SMPS */
            en_user_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_STATIC;
        }
        else/* 动态SMPS */
        {
            en_user_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC;
        }
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_ap_up_rx_smps_frame::user[%d] smps mode[%d] change to[%d]!}",
        pst_hmac_user->st_user_base_info.us_assoc_id, pst_hmac_user->st_user_base_info.st_ht_hdl.bit_sm_power_save, en_user_smps_mode);

    /* 用户更新的smps能力不能超过本vap的能力 */
    if(en_user_smps_mode > mac_mib_get_smps(pst_mac_vap))
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_mgmt_rx_smps_frame::user[%d] new smps mode[%d] beyond vap smps mode[%d]!}",
            pst_hmac_user->st_user_base_info.us_assoc_id, en_user_smps_mode, mac_mib_get_smps(pst_mac_vap));
        return OAL_FAIL;
    }

    /* 如果user的SMPS状态发生改变，需要做user和vap状态更新 */
    if (en_user_smps_mode != pst_hmac_user->st_user_base_info.st_ht_hdl.bit_sm_power_save)
    {
        pst_hmac_user->st_user_base_info.st_ht_hdl.bit_sm_power_save = en_user_smps_mode;

        return hmac_smps_update_user_status(pst_mac_vap, &(pst_hmac_user->st_user_base_info));
    }
    else
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_mgmt_rx_smps_frame::user smps mode donot change!!!}");
        return OAL_SUCC;
    }
}



oal_void hmac_smps_set_vap_mode_sta(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_plus_user)
{
    oal_uint8  uc_user_smps;
    mac_cfg_smps_mode_stru   st_smps_mode = {0};
    oal_uint32               ul_ret;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG2(0, OAM_SF_SMPS, "{hmac_smps_set_vap_mode_sta::pst_mac_vap[%p], pst_mac_user[%p].}",pst_mac_vap, pst_mac_user);
        return;
    }

    if (OAL_TRUE == en_plus_user)
    {
        uc_user_smps = (oal_uint8)pst_mac_user->st_ht_hdl.bit_sm_power_save;

        /* 如果AP支持SMPS, 则VAP默认为动态SMPS */
        if (WLAN_MIB_MIMO_POWER_SAVE_MIMO != uc_user_smps)
        {
            /* 如果AP支持SMPS, 则VAP开启动态SMPS能力 */
            mac_vap_set_smps(pst_mac_vap, WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC);

            /* 同步VAP smps mode */
            st_smps_mode.en_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC;
        }
        else
        {
            /* 如果AP不支持SMPS, 则VAP关闭SMPS能力 */
            mac_vap_set_smps(pst_mac_vap, WLAN_MIB_MIMO_POWER_SAVE_MIMO);

            /* 同步VAP smps mode */
            st_smps_mode.en_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_MIMO;
        }
    }
    else
    {
        /* 去关联AP后需要设置STA为动态 */
        if (WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC == mac_vap_get_smps_mode(pst_mac_vap))
        {
            return;
        }

        /* STA去关联后恢复为动态smps能力 */
        mac_vap_set_smps(pst_mac_vap, WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC);

        /* 同步VAP smps mode */
        st_smps_mode.en_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC;
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_set_vap_mode_sta:: set sta smps mode[%d][1.static 2.dynamic 3.mimo].}",mac_vap_get_smps_mode(pst_mac_vap));

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_VAP_SMPS, OAL_SIZEOF(mac_cfg_smps_mode_stru), (oal_uint8 *)&st_smps_mode);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_set_vap_mode_sta::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return;
}



oal_void hmac_smps_set_vap_mode_ap(mac_device_stru *pst_mac_device, mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_plus_user)
{
    oal_bool_enum_uint8                     en_ht_cap;
    mac_cfg_smps_mode_stru                  st_smps_mode = {0};
    wlan_mib_mimo_power_save_enum_uint8     en_user_smps_mode;
    oal_uint32                              ul_ret;
    hmac_vap_stru                          *pst_hmac_vap;

    if (OAL_PTR_NULL == pst_mac_device || OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG3(0, OAM_SF_SMPS, "{hmac_smps_set_vap_mode_ap::pst_mac_device[%p],pst_mac_vap[%p], pst_mac_user[%p].}",pst_mac_device, pst_mac_vap, pst_mac_user);
        return;
    }

    pst_hmac_vap = (hmac_vap_stru*)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_SMPS, "{hmac_smps_set_vap_mode_ap::mac_res_get_hmac_vap fail.uc_vap_id[%u]}",pst_mac_vap->uc_vap_id);
        return;
    }

    en_user_smps_mode = (wlan_mib_mimo_power_save_enum_uint8)pst_mac_user->st_ht_hdl.bit_sm_power_save;
    en_ht_cap = (oal_bool_enum_uint8)pst_mac_user->st_ht_hdl.en_ht_capable;

    /* 关联一个用户，注意ap默认是MIMO */
    if (OAL_TRUE == en_plus_user)
    {
        /* 加入一个支持SMPS的用户 */
        if ((OAL_TRUE == en_ht_cap) && (WLAN_MIB_MIMO_POWER_SAVE_MIMO != en_user_smps_mode))
        {
            /* 如果不是第一个用户，则已经设置过无需重复设置 */
            if (1 < pst_mac_vap->us_user_nums)
            {
                /* VAP如果有一个用户不支持，则已经配置为不支持；新增一个支持也无需配置 */
                return ;
            }

            /* 如果是Device第一个用户，则进入SMPS模式 */
            mac_vap_set_smps(pst_mac_vap, WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC);

            st_smps_mode.en_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC;
        }
        else
        {
            /* device加入一个不支持SMPS的用户 */
            pst_hmac_vap->uc_no_smps_user_cnt_ap++;
            /* 如果VAP已经有不支持SMPS的用户则无需再配置AP模式下MAC SMPS寄存器 */
            if (1 < pst_hmac_vap->uc_no_smps_user_cnt_ap)
            {
                return ;
            }

            /* 如果AP有一个不支持的用户，则VAP能力需要设置为不支持 */
            mac_vap_set_smps(pst_mac_vap, WLAN_MIB_MIMO_POWER_SAVE_MIMO);

            st_smps_mode.en_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_MIMO;
        }
     }
     else
     {
         /* 去关联一个支持SMPS的用户 */
         if ((OAL_TRUE == en_ht_cap) && (WLAN_MIB_MIMO_POWER_SAVE_MIMO != en_user_smps_mode))
         {
             return ;
         }

         /* 去关联一个不支持SMPS的用户 */
         if (0 < pst_hmac_vap->uc_no_smps_user_cnt_ap)
         {
             pst_hmac_vap->uc_no_smps_user_cnt_ap--;
         }

         /* 使能AP的MAC SMPS能力 */
         if (0 == pst_hmac_vap->uc_no_smps_user_cnt_ap)
         {
             /* 设置SMPS模式DYNAMIC */
             mac_vap_set_smps(pst_mac_vap, WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC);

             st_smps_mode.en_smps_mode = WLAN_MIB_MIMO_POWER_SAVE_DYNAMIC;
         }
         else
         {
             return ;
         }
     }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_set_vap_mode_ap:: set ap smps mode[%d][1.static 2.dynamic 3.mimo].}",mac_vap_get_smps_mode(pst_mac_vap));

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_VAP_SMPS, OAL_SIZEOF(mac_cfg_smps_mode_stru), (oal_uint8 *)&st_smps_mode);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_set_vap_mode_ap::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return;
}


oal_uint32 hmac_smps_update_status(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_plus_user)
{
    mac_device_stru                        *pst_mac_device;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_mac_user))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SMPS, "{hmac_smps_update_status: NULL PTR pst_mac_vap is [%d] and pst_mac_user is [%d].}", pst_mac_vap, pst_mac_user);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_smps_update_status: pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap))
    {
        return OAL_SUCC;
    }

    /* STA处理逻辑 */
    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        /* 根据AP能力，设置STA smps能力 */
        hmac_smps_set_vap_mode_sta(pst_mac_device, pst_mac_vap, pst_mac_user, en_plus_user);
    }
    else
    {
        /* 更新AP vap的smps能力 */
        hmac_smps_set_vap_mode_ap(pst_mac_device, pst_mac_vap, pst_mac_user, en_plus_user);
    }

    return OAL_SUCC;

}


oal_uint32 hmac_smps_user_asoc_update(oal_uint8 uc_prev_smps_mode, mac_user_stru *pst_mac_user, mac_vap_stru *pst_mac_vap)
{
    wlan_mib_mimo_power_save_enum_uint8     en_user_smps_mode;

    if (WLAN_MIB_MIMO_POWER_SAVE_MIMO == uc_prev_smps_mode)
    {
        en_user_smps_mode = (wlan_mib_mimo_power_save_enum_uint8)pst_mac_user->st_ht_hdl.bit_sm_power_save;

        if (WLAN_MIB_MIMO_POWER_SAVE_MIMO != en_user_smps_mode)
        {
            mac_user_set_sm_power_save(pst_mac_user, WLAN_MIB_MIMO_POWER_SAVE_MIMO);
            hmac_smps_update_status(pst_mac_vap, pst_mac_user, OAL_FALSE);
            mac_user_set_sm_power_save(pst_mac_user, en_user_smps_mode);
            hmac_smps_update_status((pst_mac_vap), pst_mac_user, OAL_TRUE);
        }
    }
    else
    {
        if (OAL_FALSE == pst_mac_user->st_ht_hdl.en_ht_capable)
        {
            mac_user_set_sm_power_save(pst_mac_user, WLAN_MIB_MIMO_POWER_SAVE_MIMO);
        }

        hmac_smps_update_status((pst_mac_vap), pst_mac_user, OAL_TRUE);
    }

    return OAL_SUCC;
}
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

