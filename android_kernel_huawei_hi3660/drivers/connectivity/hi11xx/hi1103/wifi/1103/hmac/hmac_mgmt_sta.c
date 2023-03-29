


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "wlan_mib.h"

#include "mac_frame.h"
#include "mac_ie.h"
#include "mac_regdomain.h"
#include "mac_user.h"
#include "mac_vap.h"

#include "mac_device.h"
#include "hmac_device.h"
#include "hmac_user.h"
#include "hmac_mgmt_sta.h"
#include "hmac_fsm.h"
#include "hmac_rx_data.h"
#include "hmac_chan_mgmt.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_encap_frame_sta.h"
#include "hmac_sme_sta.h"
#include "hmac_scan.h"

#include "hmac_tx_amsdu.h"

#include "hmac_11i.h"

#include "hmac_protection.h"

#include "hmac_config.h"
#include "hmac_ext_if.h"
#include "hmac_p2p.h"
#include "hmac_edca_opt.h"
#include "hmac_mgmt_ap.h"

#ifdef _PRE_WLAN_CHIP_TEST
#include "dmac_test_main.h"
#endif
#include "hmac_blockack.h"
#include "frw_ext_if.h"

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#include "hmac_proxysta.h"
#endif

#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
#include "hmac_wds.h"
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#include "hmac_smps.h"
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
#include "hmac_opmode.h"
#endif

#ifdef _PRE_WLAN_FEATURE_WMMAC
#include "hmac_wmmac.h"
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
#include "hmac_11k.h"
#endif

#ifdef _PRE_WLAN_FEATURE_11V_ENABLE
#include "hmac_11v.h"
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX
#include "hmac_btcoex.h"
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MGMT_STA_C
#define MAC_ADDR(_puc_mac)   ((oal_uint32)(((oal_uint32)_puc_mac[2] << 24) |\
                                              ((oal_uint32)_puc_mac[3] << 16) |\
                                              ((oal_uint32)_puc_mac[4] << 8) |\
                                              ((oal_uint32)_puc_mac[5])))

/* _puc_ie是指向ht cap字段的指针，故偏移5,6,7,8字节分别对应MCS四条空间流所支持的速率 */
#define IS_INVALID_HT_RATE_HP(_puc_ie)  ((0x02 == _puc_ie[5]) && (0x00 == _puc_ie[6]) && (0x05 == _puc_ie[7]) && (0x00 == _puc_ie[8]))

/*****************************************************************************
  2 静态函数声明
*****************************************************************************/

/*****************************************************************************
  3 全局变量定义
*****************************************************************************/
    /* 与dmac层的hi1103_date_rate_stru g_ast_hi1103_rates_11g同步修改 */
    hmac_data_rate_stru g_st_data_rates[DATARATES_80211G_NUM] =
    {
        {0x82, 0x02, 0x00, WLAN_11B_PHY_PROTOCOL_MODE},  /* 1 Mbps   */
        {0x84, 0x04, 0x01, WLAN_11B_PHY_PROTOCOL_MODE},  /* 2 Mbps   */
        {0x8B, 0x0B, 0x02, WLAN_11B_PHY_PROTOCOL_MODE},  /* 5.5 Mbps */
        {140, 0x0C, 0x0B, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE},  /* 6 Mbps   */
        {146, 0x12, 0x0F, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE},  /* 9 Mbps   */
        {0x96, 0x16, 0x03, WLAN_11B_PHY_PROTOCOL_MODE}, /* 11 Mbps  */
        {152, 0x18, 0x0A, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE}, /* 12 Mbps  */
        {164, 0x24, 0x0E, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE}, /* 18 Mbps  */
        {176, 0x30, 0x09, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE}, /* 24 Mbps  */
        {200, 0x48, 0x0D, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE}, /* 36 Mbps  */
        {224, 0x60, 0x08, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE}, /* 48 Mbps  */
        {236, 0x6C, 0x0C, WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE}  /* 54 Mbps  */
    };

/*****************************************************************************
  4 函数实现
*****************************************************************************/

OAL_STATIC oal_uint32  hmac_mgmt_timeout_sta(oal_void *p_arg)
{
    hmac_vap_stru *pst_hmac_vap;
    hmac_mgmt_timeout_param_stru    *pst_timeout_param;

    pst_timeout_param = (hmac_mgmt_timeout_param_stru *)p_arg;
    if (OAL_PTR_NULL == pst_timeout_param)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap      = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_timeout_param->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_fsm_call_func_sta_etc(pst_hmac_vap, HMAC_FSM_INPUT_TIMER0_OUT, pst_timeout_param);
}
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_void  hmac_update_join_req_params_2040_etc(mac_vap_stru *pst_mac_vap, mac_bss_dscr_stru *pst_bss_dscr)
{

    if (((OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap)) && (OAL_FALSE == mac_mib_get_VHTOptionImplemented(pst_mac_vap))) ||
        ((OAL_FALSE == pst_bss_dscr->en_ht_capable) && (OAL_FALSE == pst_bss_dscr->en_vht_capable)))
    {
        pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
        return;
    }

    /* 使能40MHz */
    /* (1) 用户开启"40MHz运行"特性(即STA侧 dot11FortyMHzOperationImplemented为true) */
    /* (2) AP在40MHz运行 */
    if (OAL_TRUE == mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap))
    {
        switch (pst_bss_dscr->en_channel_bandwidth)
        {
            case WLAN_BAND_WIDTH_40PLUS:
            case WLAN_BAND_WIDTH_80PLUSPLUS:
            case WLAN_BAND_WIDTH_80PLUSMINUS:
                pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_40PLUS;
                break;

            case WLAN_BAND_WIDTH_40MINUS:
            case WLAN_BAND_WIDTH_80MINUSPLUS:
            case WLAN_BAND_WIDTH_80MINUSMINUS:
                pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_40MINUS;
                break;

            default:
                pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
                break;
        }
    }

    /* 更新STA侧带宽与AP一致 */
    /* (1) STA AP均支持11AC */
    /* (2) STA支持40M带宽(FortyMHzOperationImplemented为TRUE)，
           定制化禁止2GHT40时，2G下FortyMHzOperationImplemented=FALSE，不更新带宽 */
    /* (3) STA支持80M带宽(即STA侧 dot11VHTChannelWidthOptionImplemented为0) */
    if ((OAL_TRUE == mac_mib_get_VHTOptionImplemented(pst_mac_vap)) &&
        (OAL_TRUE == pst_bss_dscr->en_vht_capable))
    {
        if (OAL_TRUE == mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap))
        {

            /* 不超过mac device最大带宽能力 */
             MAC_VAP_GET_CAP_BW(pst_mac_vap) = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap), pst_bss_dscr->en_channel_bandwidth);

         }
    }

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_update_join_req_params_2040_etc::en_channel_bandwidth=%d, mac vap bw[%d].}",
        pst_bss_dscr->en_channel_bandwidth, MAC_VAP_GET_CAP_BW(pst_mac_vap));

    /* 如果AP和STA同时支持20/40共存管理功能，则使能STA侧频谱管理功能*/
    if ((OAL_TRUE == mac_mib_get_2040BSSCoexistenceManagementSupport(pst_mac_vap)) &&
        (1 == pst_bss_dscr->uc_coex_mgmt_supp))
    {
        mac_mib_set_SpectrumManagementImplemented(pst_mac_vap, OAL_TRUE);
    }
}

#endif

OAL_STATIC oal_void hmac_update_join_req_params_prot_sta(hmac_vap_stru * pst_hmac_vap, hmac_join_req_stru * pst_join_req)
{
    if (WLAN_MIB_DESIRED_BSSTYPE_INFRA  == mac_mib_get_DesiredBSSType(&pst_hmac_vap->st_vap_base_info))
    {
        pst_hmac_vap->st_vap_base_info.st_cap_flag.bit_wmm_cap   = pst_join_req->st_bss_dscr.uc_wmm_cap;
        mac_vap_set_uapsd_cap_etc(&pst_hmac_vap->st_vap_base_info, pst_join_req->st_bss_dscr.uc_uapsd_cap);
    }

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    hmac_update_join_req_params_2040_etc(&(pst_hmac_vap->st_vap_base_info), &(pst_join_req->st_bss_dscr));
#endif
}


OAL_STATIC oal_void hmac_sta_bandwidth_down_by_channel(mac_vap_stru * pst_mac_vap)
{
    oal_bool_enum_uint8 en_bandwidth_change_to_20M = OAL_FALSE;

    switch(pst_mac_vap->st_channel.en_bandwidth)
    {
        case WLAN_BAND_WIDTH_40PLUS:
            /* 1. 64 144 161信道不支持40+ */
            if((pst_mac_vap->st_channel.uc_chan_number >= 64 && pst_mac_vap->st_channel.uc_chan_number < 100)||
                (pst_mac_vap->st_channel.uc_chan_number >= 144 && pst_mac_vap->st_channel.uc_chan_number < 149)||
                (pst_mac_vap->st_channel.uc_chan_number >= 161 && pst_mac_vap->st_channel.uc_chan_number < 184))
            {
                en_bandwidth_change_to_20M = OAL_TRUE;
            }
            break;

        case WLAN_BAND_WIDTH_40MINUS:
            /* 1. 100 149 184信道不支持40- */
            if((pst_mac_vap->st_channel.uc_chan_number > 64 && pst_mac_vap->st_channel.uc_chan_number <= 100)||
                (pst_mac_vap->st_channel.uc_chan_number > 144 && pst_mac_vap->st_channel.uc_chan_number <= 149) ||
                (pst_mac_vap->st_channel.uc_chan_number > 161 && pst_mac_vap->st_channel.uc_chan_number <= 184))
            {
                en_bandwidth_change_to_20M = OAL_TRUE;
            }
            break;

        case WLAN_BAND_WIDTH_80PLUSPLUS:
        case WLAN_BAND_WIDTH_80PLUSMINUS:
        case WLAN_BAND_WIDTH_80MINUSPLUS:
        case WLAN_BAND_WIDTH_80MINUSMINUS:
            /* 165信道不支持80M, 暂时不考虑出现更多信道异常问题 */
            if(165 == pst_mac_vap->st_channel.uc_chan_number)
            {
                en_bandwidth_change_to_20M = OAL_TRUE;
            }
            break;

        /* 160M的带宽校验暂时不考虑 */
        default:
            break;
    }

    /* 需要降带宽 */
    if(OAL_TRUE == en_bandwidth_change_to_20M)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_bandwidth_down_by_channel:: channel[%d] not support bandwidth[%d], need to change to 20M.}",
            pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_channel.en_bandwidth);

        pst_mac_vap->st_channel.en_bandwidth = WLAN_BAND_WIDTH_20M;
    }
}


oal_bool_enum_uint8  hmac_is_rate_support_etc(oal_uint8 *puc_rates, oal_uint8 uc_rate_num, oal_uint8 uc_rate)
{
    oal_bool_enum_uint8  en_rate_is_supp = OAL_FALSE;
    oal_uint8            uc_loop;

    if (OAL_PTR_NULL == puc_rates)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_is_rate_support_etc::puc_rates null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_loop = 0; uc_loop < uc_rate_num; uc_loop++)
    {
        if ((puc_rates[uc_loop] & 0x7F) == uc_rate)
        {
            en_rate_is_supp = OAL_TRUE;
            break;
        }
    }

    return en_rate_is_supp;
}


oal_bool_enum_uint8  hmac_is_support_11grate_etc(oal_uint8 *puc_rates, oal_uint8 uc_rate_num)
{
    if (OAL_PTR_NULL == puc_rates)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_is_rate_support_etc::puc_rates null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x0C))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x12))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x18))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x24))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x30))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x48))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x60))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x6C)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}



oal_bool_enum_uint8  hmac_is_support_11brate_etc(oal_uint8 *puc_rates, oal_uint8 uc_rate_num)
{
    if (OAL_PTR_NULL == puc_rates)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_is_support_11brate_etc::puc_rates null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x02))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x04))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x0B))
        || (OAL_TRUE == hmac_is_rate_support_etc(puc_rates, uc_rate_num, 0x16)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
#if 0

oal_bool_enum_uint8  hmac_bss_is_rate_support(mac_bss_dscr_stru* pst_bss_dscr, oal_uint8 uc_rate)
{
    oal_bool_enum_uint8  en_rate_is_supp = OAL_FALSE;
    oal_uint8            uc_loop;

    if (OAL_PTR_NULL == pst_bss_dscr)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_bss_is_rate_support::pst_bss_dscr null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_loop = 0; uc_loop < pst_bss_dscr->uc_num_supp_rates; uc_loop++)
    {
        if ((pst_bss_dscr->auc_supp_rates[uc_loop] & 0x7F) == uc_rate)
        {
            en_rate_is_supp = OAL_TRUE;
            break;
        }
    }

    return en_rate_is_supp;
}
#endif

oal_uint32 hmac_sta_get_user_protocol_etc(mac_bss_dscr_stru *pst_bss_dscr, wlan_protocol_enum_uint8  *pen_protocol_mode)
{
    /* 入参保护 */
    if (OAL_PTR_NULL == pst_bss_dscr || OAL_PTR_NULL == pen_protocol_mode)
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_sta_get_user_protocol_etc::param null,%d %d.}", pst_bss_dscr, pen_protocol_mode);
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_11AX
    if(OAL_TRUE == pst_bss_dscr->en_he_capable)
    {
        *pen_protocol_mode = WLAN_HE_MODE;
    }
    else if (OAL_TRUE == pst_bss_dscr->en_vht_capable)
#else
    if (OAL_TRUE == pst_bss_dscr->en_vht_capable)
#endif
    {
        *pen_protocol_mode = WLAN_VHT_MODE;
    }
    else if (OAL_TRUE == pst_bss_dscr->en_ht_capable)
    {
        *pen_protocol_mode = WLAN_HT_MODE;
    }
    else
    {
        if (WLAN_BAND_5G == pst_bss_dscr->st_channel.en_band)            /* 判断是否是5G */
        {
            *pen_protocol_mode = WLAN_LEGACY_11A_MODE;
        }
        else
        {
            if (OAL_TRUE == hmac_is_support_11grate_etc(pst_bss_dscr->auc_supp_rates, pst_bss_dscr->uc_num_supp_rates))
            {
                *pen_protocol_mode = WLAN_LEGACY_11G_MODE;
                if (OAL_TRUE == hmac_is_support_11brate_etc(pst_bss_dscr->auc_supp_rates, pst_bss_dscr->uc_num_supp_rates))
                {
                    *pen_protocol_mode = WLAN_MIXED_ONE_11G_MODE;
                }
            }
            else if (OAL_TRUE == hmac_is_support_11brate_etc(pst_bss_dscr->auc_supp_rates, pst_bss_dscr->uc_num_supp_rates))
            {
                *pen_protocol_mode = WLAN_LEGACY_11B_MODE;
            }
            else
            {
                OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_sta_get_user_protocol_etc::get user protocol failed.}");
            }
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_sta_wait_join_etc(hmac_vap_stru *pst_hmac_sta, oal_void *pst_msg)
{
    hmac_join_req_stru                  *pst_join_req;
    hmac_join_rsp_stru                   st_join_rsp;
    oal_uint32                           ul_ret;

    if (OAL_PTR_NULL == pst_hmac_sta || OAL_PTR_NULL == pst_msg)
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_sta_wait_join_etc::param null,%d %d.}", pst_hmac_sta, pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 1102 P2PSTA共存 todo 更新参数失败的话需要返回而不是继续下发Join动作 */
    ul_ret = hmac_p2p_check_can_enter_state_etc(&(pst_hmac_sta->st_vap_base_info), HMAC_FSM_INPUT_ASOC_REQ);
    if (ul_ret != OAL_SUCC)
    {
        /* 不能进入监听状态，返回设备忙 */
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                        "{hmac_sta_wait_join_etc fail,device busy: ul_ret=%d}\r\n", ul_ret);
        return OAL_ERR_CODE_CONFIG_BUSY;
    }

    pst_join_req = (hmac_join_req_stru *)pst_msg;

    /* 主要用于proxysta模式下 MAIN STA发起关联时DOWN掉所有UP状态的AP 不开启proxysta时函数直接返回 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    hmac_psta_proc_wait_join(pst_hmac_sta, pst_join_req);
#endif
    /* 更新JOIN REG params 到MIB及MAC寄存器 */
    ul_ret = hmac_sta_update_join_req_params_etc(pst_hmac_sta, pst_join_req);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_sta_wait_join_etc::get hmac_sta_update_join_req_params_etc fail[%d]!}", ul_ret);
        return ul_ret;
    }

    /* 非proxy sta模式时，需要将dtim参数配置到dmac*/
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_vap_is_vsta(&pst_hmac_sta->st_vap_base_info))
    {
        //打开proxysta宏前提下，开启proxysta模式下不做处理，非开启proxysta模式下才需处理
    }
    else
#endif
    {
        dmac_ctx_set_dtim_tsf_reg_stru      *pst_set_dtim_tsf_reg_params;
        frw_event_mem_stru                  *pst_event_mem;
        frw_event_stru                      *pst_event;

        /* 抛事件到DMAC, 申请事件内存 */
        pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
        if (OAL_PTR_NULL == pst_event_mem)
        {
            OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_join_etc::event_mem alloc null, size[%d].}", OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 填写事件 */
        pst_event = frw_get_event_stru(pst_event_mem);

        FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_WLAN_CTX,
                           DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG,
                           OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru),
                           FRW_EVENT_PIPELINE_STAGE_1,
                           pst_hmac_sta->st_vap_base_info.uc_chip_id,
                           pst_hmac_sta->st_vap_base_info.uc_device_id,
                           pst_hmac_sta->st_vap_base_info.uc_vap_id);

        pst_set_dtim_tsf_reg_params = (dmac_ctx_set_dtim_tsf_reg_stru *)pst_event->auc_event_data;

        /* 将Ap bssid和tsf REG 设置值保存在事件payload中 */
        pst_set_dtim_tsf_reg_params->ul_dtim_cnt      = pst_join_req->st_bss_dscr.uc_dtim_cnt;
        pst_set_dtim_tsf_reg_params->ul_dtim_period   = pst_join_req->st_bss_dscr.uc_dtim_period;
        pst_set_dtim_tsf_reg_params->us_tsf_bit0      = BIT0;
        oal_memcopy(pst_set_dtim_tsf_reg_params->auc_bssid, pst_hmac_sta->st_vap_base_info.auc_bssid, WLAN_MAC_ADDR_LEN);

        /* 分发事件 */
        frw_event_dispatch_event_etc(pst_event_mem);
        FRW_EVENT_FREE(pst_event_mem);
    }

    st_join_rsp.en_result_code = HMAC_MGMT_SUCCESS;
    /* 切换STA状态到JOIN_COMP */
    hmac_fsm_change_state_etc(pst_hmac_sta, MAC_VAP_STATE_STA_JOIN_COMP);

    /* 发送JOIN成功消息给SME */
    hmac_send_rsp_to_sme_sta_etc(pst_hmac_sta, HMAC_SME_JOIN_RSP,(oal_uint8 *)&st_join_rsp);

    OAM_INFO_LOG3(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                   "{hmac_sta_wait_join_etc::Join AP[%08x] HT=%d VHT=%d SUCC.}",
                   MAC_ADDR(pst_join_req->st_bss_dscr.auc_bssid),
                   pst_join_req->st_bss_dscr.en_ht_capable,
                   pst_join_req->st_bss_dscr.en_vht_capable);

    OAM_WARNING_LOG4(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                   "{hmac_sta_wait_join_etc::Join AP channel=%d idx=%d, bandwidth=%d Beacon Period=%d SUCC.}",
                   pst_join_req->st_bss_dscr.st_channel.uc_chan_number,
                   pst_join_req->st_bss_dscr.st_channel.uc_chan_idx,
                   pst_hmac_sta->st_vap_base_info.st_channel.en_bandwidth,
                   pst_join_req->st_bss_dscr.us_beacon_period);

    return OAL_SUCC;
}


oal_uint32  hmac_sta_wait_join_rx_etc(hmac_vap_stru *pst_hmac_sta, oal_void *p_param)
{
    dmac_wlan_crx_event_stru            *pst_mgmt_rx_event;
    dmac_rx_ctl_stru                    *pst_rx_ctrl;
    mac_rx_ctl_stru                     *pst_rx_info;
    oal_uint8                           *puc_mac_hdr;
    frw_event_mem_stru                  *pst_event_mem;
    frw_event_stru                      *pst_event;
    dmac_ctx_set_dtim_tsf_reg_stru       st_set_dtim_tsf_reg_params = {0};
    oal_uint8                           *puc_tim_elm;
    oal_uint16                           us_rx_len;

    oal_uint8                            auc_bssid[6] = {0};

     if ((OAL_PTR_NULL == pst_hmac_sta) || (OAL_PTR_NULL == p_param))
     {
         OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_sta_wait_join_rx_etc::param null,%d %d.}", pst_hmac_sta, p_param);
         return OAL_ERR_CODE_PTR_NULL;
     }

     pst_mgmt_rx_event   = (dmac_wlan_crx_event_stru *)p_param;
     pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_mgmt_rx_event->pst_netbuf);
     pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
     puc_mac_hdr         = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_info);
     us_rx_len           =  pst_rx_ctrl->st_rx_info.us_frame_len;  /* 消息总长度,不包括FCS */

     /* 在WAIT_JOIN状态下，处理接收到的beacon帧 */
    switch (mac_get_frame_type_and_subtype(puc_mac_hdr))
    {
        case WLAN_FC0_SUBTYPE_BEACON|WLAN_FC0_TYPE_MGT:
        {
            /* 获取Beacon帧中的mac地址，即AP的mac地址 */
            mac_get_bssid(puc_mac_hdr, auc_bssid);

            /* 如果STA保存的AP mac地址与接收beacon帧的mac地址匹配，则更新beacon帧中的DTIM count值到STA本地mib库中 */
            if (0 == oal_memcmp(auc_bssid, pst_hmac_sta->st_vap_base_info.auc_bssid, WLAN_MAC_ADDR_LEN))
            {
                //puc_tim_elm = mac_get_tim_ie(puc_mac_hdr, us_rx_len, MAC_TAG_PARAM_OFFSET);
                puc_tim_elm = mac_find_ie_etc(MAC_EID_TIM, puc_mac_hdr + MAC_TAG_PARAM_OFFSET, us_rx_len - MAC_TAG_PARAM_OFFSET);

                /* 从tim IE中提取 DTIM count值,写入到MAC H/W REG中 */
                if ((OAL_PTR_NULL != puc_tim_elm) && (puc_tim_elm[1] >= MAC_MIN_TIM_LEN))
                {
                    mac_mib_set_dot11dtimperiod(&pst_hmac_sta->st_vap_base_info, puc_tim_elm[3]);

                    /* 将dtim_cnt和dtim_period保存在事件payload中 */
                    st_set_dtim_tsf_reg_params.ul_dtim_cnt      = puc_tim_elm[2];
                    st_set_dtim_tsf_reg_params.ul_dtim_period   = puc_tim_elm[3];
                }
                else
                {
                    OAM_WARNING_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                                     "{hmac_sta_wait_join_etc::Do not find Tim ie.}");
                }

                /* 将Ap bssid和tsf REG 设置值保存在事件payload中 */
                oal_memcopy (st_set_dtim_tsf_reg_params.auc_bssid, auc_bssid, WLAN_MAC_ADDR_LEN);
                st_set_dtim_tsf_reg_params.us_tsf_bit0 = BIT0;

                 /* 抛事件到DMAC, 申请事件内存 */
                pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
                if (OAL_PTR_NULL == pst_event_mem)
                {
                    OAM_ERROR_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_sta_wait_join_etc::pst_event_mem null.}");
                    return OAL_ERR_CODE_PTR_NULL;
                }

                /* 填写事件 */
                pst_event = frw_get_event_stru(pst_event_mem);

                FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                                   FRW_EVENT_TYPE_WLAN_CTX,
                                   DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG,
                                   OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru),
                                   FRW_EVENT_PIPELINE_STAGE_1,
                                   pst_hmac_sta->st_vap_base_info.uc_chip_id,
                                   pst_hmac_sta->st_vap_base_info.uc_device_id,
                                   pst_hmac_sta->st_vap_base_info.uc_vap_id);

                /* 拷贝参数 */
                oal_memcopy(frw_get_event_payload(pst_event_mem), (oal_uint8*)&st_set_dtim_tsf_reg_params, OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));

                /* 分发事件 */
                frw_event_dispatch_event_etc(pst_event_mem);
                FRW_EVENT_FREE(pst_event_mem);
            }
        }
        break;

        case WLAN_FC0_SUBTYPE_ACTION|WLAN_FC0_TYPE_MGT:
        {
            /* do nothing  */
        }
        break;

        default:
        {
            /* do nothing */
        }
        break;
    }

   return OAL_SUCC;
}


oal_uint32  hmac_sta_wait_join_timeout_etc(hmac_vap_stru *pst_hmac_sta, oal_void *pst_msg)
{

    hmac_join_rsp_stru      st_join_rsp        = {0};

    if (OAL_PTR_NULL == pst_hmac_sta || OAL_PTR_NULL == pst_msg)
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_sta_wait_join_timeout_etc::param null,%d %d.}", pst_hmac_sta, pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_ERROR_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_sta_wait_join_timeout_etc::join timeout.}");
    /* 进入timeout处理函数表示join没有成功，把join的结果设置为timeout上报给sme */
    st_join_rsp.en_result_code = HMAC_MGMT_TIMEOUT;

    /* 将hmac状态机切换为MAC_VAP_STATE_STA_FAKE_UP */
    hmac_fsm_change_state_etc(pst_hmac_sta, MAC_VAP_STATE_STA_FAKE_UP);

    /* 发送超时消息给SME */
    hmac_send_rsp_to_sme_sta_etc(pst_hmac_sta, HMAC_SME_JOIN_RSP, (oal_uint8 *)&st_join_rsp);

    return OAL_SUCC;
}


oal_uint32  hmac_sta_wait_join_misc_etc(hmac_vap_stru *pst_hmac_sta, oal_void *pst_msg)
{
    hmac_join_rsp_stru      st_join_rsp;
    hmac_misc_input_stru    *st_misc_input = (hmac_misc_input_stru *)pst_msg;

    if((OAL_PTR_NULL == pst_hmac_sta) ||(OAL_PTR_NULL == pst_msg))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_sta_wait_join_misc_etc::param null,%d %d.}", pst_hmac_sta, pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_INFO_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_sta_wait_join_misc_etc::enter func.}");
    switch (st_misc_input->en_type)
    {
        /* 处理TBTT中断  */
        case HMAC_MISC_TBTT:
        {
           /* 接收到TBTT中断，意味着JOIN成功了，所以取消JOIN开始时设置的定时器,发消息通知SME  */
           FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_sta->st_mgmt_timer);

           st_join_rsp.en_result_code = HMAC_MGMT_SUCCESS;

            OAM_INFO_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_sta_wait_join_misc_etc::join succ.}");
            /* 切换STA状态到JOIN_COMP */
            hmac_fsm_change_state_etc(pst_hmac_sta, MAC_VAP_STATE_STA_JOIN_COMP);

            /* 发送JOIN成功消息给SME */
            hmac_send_rsp_to_sme_sta_etc(pst_hmac_sta, HMAC_SME_JOIN_RSP,(oal_uint8 *)&st_join_rsp);
        }
        break;

        default:
        {
            /* Do Nothing */
        }
        break;
    }
    return OAL_SUCC;
}




oal_uint32  hmac_sta_wait_auth_etc(hmac_vap_stru *pst_hmac_sta, oal_void *pst_msg)
{
    hmac_auth_req_stru  *pst_auth_req;
    oal_netbuf_stru     *pst_auth_frame;
    oal_uint16           us_auth_len;
    mac_tx_ctl_stru     *pst_tx_ctl;
    hmac_user_stru      *pst_hmac_user_ap;
    oal_uint32           ul_ret;

    if (OAL_PTR_NULL == pst_hmac_sta || OAL_PTR_NULL == pst_msg)
    {
        OAM_ERROR_LOG2(0, OAM_SF_AUTH, "{hmac_sta_wait_auth_etc::param null,%d %d.}", pst_hmac_sta, pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_auth_req = (hmac_auth_req_stru *)pst_msg;
    /* 申请认证帧空间 */
    pst_auth_frame = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);

    if (OAL_PTR_NULL == pst_auth_frame)
    {
        OAM_ERROR_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_wait_auth_sta::puc_auth_frame null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_auth_frame, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_auth_frame), OAL_NETBUF_CB_SIZE());

    OAL_MEMZERO((oal_uint8 *)oal_netbuf_header(pst_auth_frame), MAC_80211_FRAME_LEN);

    /* 组seq = 1 的认证请求帧 */
    us_auth_len = hmac_mgmt_encap_auth_req_etc(pst_hmac_sta, (oal_uint8 *)(OAL_NETBUF_HEADER(pst_auth_frame)));

    if (us_auth_len == 0)
    {
        /* 组帧失败 */
        OAM_WARNING_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_wait_auth_sta::hmac_mgmt_encap_auth_req_etc failed.}");

        oal_netbuf_free(pst_auth_frame);
        hmac_fsm_change_state_etc(pst_hmac_sta, MAC_VAP_STATE_STA_FAKE_UP);
        /* TBD: 报系统错误，reset MAC 之类的 */
    }
    else
    {
        oal_netbuf_put(pst_auth_frame, us_auth_len);
        pst_hmac_user_ap = mac_res_get_hmac_user_etc(pst_hmac_sta->st_vap_base_info.us_assoc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_user_ap)
        {
            oal_netbuf_free(pst_auth_frame);
            OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_wait_auth_sta::pst_hmac_user[%d] null.}",
                pst_hmac_sta->st_vap_base_info.us_assoc_vap_id);
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 为填写发送描述符准备参数 */
        pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_auth_frame);                              /* 获取cb结构体 */
        MAC_GET_CB_MPDU_LEN(pst_tx_ctl)   = us_auth_len;                                      /* dmac发送需要的mpdu长度 */
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = pst_hmac_user_ap->st_user_base_info.us_assoc_id;  /* 发送完成需要获取user结构体 */

        /*如果是WEP，需要将ap的mac地址写入lut*/
        ul_ret = hmac_init_security_etc(&(pst_hmac_sta->st_vap_base_info), pst_hmac_user_ap->st_user_base_info.auc_user_mac_addr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                           "{hmac_sta_wait_auth_etc::hmac_init_security_etc failed[%d].}", ul_ret);
        }

        /* 抛事件让dmac将该帧发送 */
        ul_ret = hmac_tx_mgmt_send_event_etc(&pst_hmac_sta->st_vap_base_info, pst_auth_frame, us_auth_len);
        if (OAL_SUCC != ul_ret)
        {
            oal_netbuf_free(pst_auth_frame);
            OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_wait_auth_sta::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }

        /* 更改状态 */
        hmac_fsm_change_state_etc(pst_hmac_sta, MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2);

        /* 启动认证超时定时器 */
        pst_hmac_sta->st_mgmt_timetout_param.en_state = MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2;
        pst_hmac_sta->st_mgmt_timetout_param.us_user_index = pst_hmac_user_ap->st_user_base_info.us_assoc_id;
        pst_hmac_sta->st_mgmt_timetout_param.uc_vap_id = pst_hmac_sta->st_vap_base_info.uc_vap_id;
        FRW_TIMER_CREATE_TIMER(&pst_hmac_sta->st_mgmt_timer,
                               hmac_mgmt_timeout_sta,
                               pst_auth_req->us_timeout,
                               &pst_hmac_sta->st_mgmt_timetout_param,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_sta->st_vap_base_info.ul_core_id);

    }

    return OAL_SUCC;
}


oal_uint32  hmac_sta_wait_auth_seq2_rx_etc(hmac_vap_stru *pst_sta, oal_void *pst_msg)
{
    dmac_wlan_crx_event_stru    *pst_crx_event;
    mac_rx_ctl_stru            *pst_rx_ctrl;                    /* 每一个MPDU的控制信息 */
    oal_uint8                   *puc_mac_hdr;
    oal_uint16                   us_auth_frame_len = 0;
    oal_uint16                   us_auth_alg;
    oal_netbuf_stru             *pst_auth_frame    = 0;
    hmac_user_stru              *pst_hmac_user_ap;
    hmac_auth_rsp_stru           st_auth_rsp       = {{0},};
    mac_tx_ctl_stru             *pst_tx_ctl;
    oal_uint32                   ul_ret;

    if (OAL_PTR_NULL == pst_sta || OAL_PTR_NULL == pst_msg)
    {
        OAM_ERROR_LOG2(0, OAM_SF_AUTH, "{hmac_sta_wait_auth_seq2_rx_etc::param null,%d %d.}", pst_sta, pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }


    pst_crx_event  = (dmac_wlan_crx_event_stru *)pst_msg;
    pst_rx_ctrl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);                    /* 每一个MPDU的控制信息 */
    puc_mac_hdr    = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_sta_wait_auth_seq2_rx_etc::wakeup mgmt type[0x%x]}",mac_get_frame_type_and_subtype(puc_mac_hdr));
    }
#endif
#endif

    if ((WLAN_FC0_SUBTYPE_AUTH|WLAN_FC0_TYPE_MGT) != mac_get_frame_type_and_subtype(puc_mac_hdr) ||
        WLAN_AUTH_TRASACTION_NUM_TWO != mac_get_auth_seq_num(puc_mac_hdr))
    {
        return OAL_SUCC;
    }

    /* AUTH alg CHECK*/
    us_auth_alg = mac_get_auth_alg(puc_mac_hdr);
    if ((mac_mib_get_AuthenticationMode(&pst_sta->st_vap_base_info) != us_auth_alg)
        && (WLAN_WITP_AUTH_AUTOMATIC != mac_mib_get_AuthenticationMode(&pst_sta->st_vap_base_info)))
    {
        OAM_WARNING_LOG2(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                         "{hmac_sta_wait_auth_seq2_rx_etc::rcv unexpected auth alg[%d/%d].}", us_auth_alg, mac_mib_get_AuthenticationMode(&pst_sta->st_vap_base_info));
    }
    st_auth_rsp.us_status_code = mac_get_auth_status(puc_mac_hdr);
    if (MAC_SUCCESSFUL_STATUSCODE != st_auth_rsp.us_status_code)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_sta->st_mgmt_timer);

        /* 上报给SME认证结果 */
        hmac_send_rsp_to_sme_sta_etc(pst_sta, HMAC_SME_AUTH_RSP, (oal_uint8 *)&st_auth_rsp);
        OAM_WARNING_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                         "{hmac_sta_wait_auth_seq2_rx_etc::AP refuse STA auth reason[%d]!}", st_auth_rsp.us_status_code);
        if (MAC_AP_FULL != st_auth_rsp.us_status_code)
        {
            CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_CONNECT,CHR_WIFI_DRV_ERROR_AUTH_REJECTED));
        }
        return OAL_SUCC;
    }

    /* auth response status_code 成功处理 */
    if (WLAN_WITP_AUTH_OPEN_SYSTEM == us_auth_alg)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_sta->st_mgmt_timer);

        /* 将状态更改为AUTH_COMP */
        hmac_fsm_change_state_etc(pst_sta, MAC_VAP_STATE_STA_AUTH_COMP);
        st_auth_rsp.us_status_code = HMAC_MGMT_SUCCESS;

        /* 上报给SME认证成功 */
        hmac_send_rsp_to_sme_sta_etc(pst_sta, HMAC_SME_AUTH_RSP, (oal_uint8 *)&st_auth_rsp);
        return OAL_SUCC;
    }
    else if (WLAN_WITP_AUTH_SHARED_KEY == us_auth_alg)
    {
        /* 准备seq = 3的认证帧 */
        pst_auth_frame = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);

        if(OAL_PTR_NULL == pst_auth_frame)
        {
            /* TBD: 复位mac */
            OAM_ERROR_LOG0(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_wait_auth_sta::pst_auth_frame null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        OAL_MEM_NETBUF_TRACE(pst_auth_frame, OAL_TRUE);

        OAL_MEMZERO(oal_netbuf_cb(pst_auth_frame), OAL_NETBUF_CB_SIZE());

        us_auth_frame_len = hmac_mgmt_encap_auth_req_seq3_etc(pst_sta, (oal_uint8 *)OAL_NETBUF_HEADER(pst_auth_frame), puc_mac_hdr);
        oal_netbuf_put(pst_auth_frame, us_auth_frame_len);

        pst_hmac_user_ap = mac_res_get_hmac_user_etc(pst_sta->st_vap_base_info.us_assoc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_user_ap)
        {
            oal_netbuf_free(pst_auth_frame);
            OAM_ERROR_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_wait_auth_sta::pst_hmac_user[%d] null.}",
                pst_sta->st_vap_base_info.us_assoc_vap_id);
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 填写发送和发送完成需要的参数 */
        pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_auth_frame);
        MAC_GET_CB_MPDU_LEN(pst_tx_ctl)   = us_auth_frame_len;                                 /* 发送需要帧长度 */
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_hmac_user_ap->st_user_base_info.us_assoc_id;   /* 发送完成要获取用户 */

        /* 抛事件给dmac发送 */
        ul_ret = hmac_tx_mgmt_send_event_etc(&pst_sta->st_vap_base_info, pst_auth_frame, us_auth_frame_len);
        if (OAL_SUCC != ul_ret)
        {
            oal_netbuf_free(pst_auth_frame);
            OAM_WARNING_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_wait_auth_sta::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }

        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_sta->st_mgmt_timer);

        /* 更改状态为MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4，并启动定时器 */
        hmac_fsm_change_state_etc(pst_sta, MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4);

        FRW_TIMER_CREATE_TIMER(&pst_sta->st_mgmt_timer,
                               hmac_mgmt_timeout_sta,
                               pst_sta->st_mgmt_timer.ul_timeout,
                               &pst_sta->st_mgmt_timetout_param,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_sta->st_vap_base_info.ul_core_id);
        return OAL_SUCC;
    }
    else
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_sta->st_mgmt_timer);

        /* 接收到AP 回复的auth response 中支持认证算法当前不支持的情况下，status code 却是SUCC,
            认为认证成功，并且继续出发关联 */
        OAM_WARNING_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                         "{hmac_sta_wait_auth_seq2_rx_etc::AP's auth_alg [%d] not support!}", us_auth_alg);

        /* 将状态更改为AUTH_COMP */
        hmac_fsm_change_state_etc(pst_sta, MAC_VAP_STATE_STA_AUTH_COMP);
        st_auth_rsp.us_status_code = HMAC_MGMT_SUCCESS;

        /* 上报给SME认证成功 */
        hmac_send_rsp_to_sme_sta_etc(pst_sta, HMAC_SME_AUTH_RSP, (oal_uint8 *)&st_auth_rsp);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_sta_wait_auth_seq4_rx_etc(hmac_vap_stru *pst_sta, oal_void *p_msg)
{
    dmac_wlan_crx_event_stru    *pst_crx_event;
    mac_rx_ctl_stru            *pst_rx_ctrl; /* 每一个MPDU的控制信息 */
    oal_uint8                   *puc_mac_hdr;
    hmac_auth_rsp_stru           st_auth_rsp    = {{0},};

    if (OAL_PTR_NULL == p_msg || OAL_PTR_NULL == pst_sta)
    {
        OAM_ERROR_LOG2(0, OAM_SF_AUTH, "{hmac_sta_wait_auth_seq2_rx_etc::param null,%d %d.}", p_msg, p_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_msg;
    pst_rx_ctrl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf); /* 每一个MPDU的控制信息 */
    puc_mac_hdr    = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_sta_wait_auth_seq4_rx_etc::wakeup mgmt type[0x%x]}",mac_get_frame_type_and_subtype(puc_mac_hdr));
    }
#endif
#endif

    if ((WLAN_FC0_SUBTYPE_AUTH|WLAN_FC0_TYPE_MGT) == mac_get_frame_type_and_subtype(puc_mac_hdr))
    {

        if((WLAN_AUTH_TRASACTION_NUM_FOUR == mac_get_auth_seq_num(puc_mac_hdr)) &&
           (MAC_SUCCESSFUL_STATUSCODE == mac_get_auth_status(puc_mac_hdr)))
        {
            /* 接收到seq = 4 且状态位为succ 取消定时器 */
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_sta->st_mgmt_timer);

            st_auth_rsp.us_status_code = HMAC_MGMT_SUCCESS;

            /* 更改sta状态为MAC_VAP_STATE_STA_AUTH_COMP */
            hmac_fsm_change_state_etc(pst_sta, MAC_VAP_STATE_STA_AUTH_COMP);
            OAM_INFO_LOG0(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_sta_wait_auth_seq4_rx_etc::auth succ.}");
            /* 将认证结果上报SME */
            hmac_send_rsp_to_sme_sta_etc(pst_sta, HMAC_SME_AUTH_RSP, (oal_uint8 *)&st_auth_rsp);
        }
        else
        {
            OAM_WARNING_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_sta_wait_auth_seq4_rx_etc::transaction num.status[%d]}", \
              mac_get_auth_status(puc_mac_hdr));
            /* 等待定时器超时 */
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_sta_wait_asoc_etc(hmac_vap_stru *pst_sta, oal_void *pst_msg)
{
    hmac_asoc_req_stru         *pst_hmac_asoc_req;
    oal_netbuf_stru            *pst_asoc_req_frame;
    mac_tx_ctl_stru            *pst_tx_ctl;
    hmac_user_stru             *pst_hmac_user_ap;
    oal_uint32                  ul_asoc_frame_len;
    oal_uint32                  ul_ret;
    oal_uint8                  *puc_curr_bssid;

    if ((OAL_PTR_NULL == pst_sta) || (OAL_PTR_NULL == pst_msg))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_etc::param null,%d %d.}", pst_sta, pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_asoc_req = (hmac_asoc_req_stru *)pst_msg;

    pst_asoc_req_frame = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);

    if (OAL_PTR_NULL == pst_asoc_req_frame)
    {
        OAM_ERROR_LOG0(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_etc::pst_asoc_req_frame null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEM_NETBUF_TRACE(pst_asoc_req_frame, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_asoc_req_frame), OAL_NETBUF_CB_SIZE());

    /* 将mac header清零 */
    OAL_MEMZERO((oal_uint8 *)oal_netbuf_header(pst_asoc_req_frame), MAC_80211_FRAME_LEN);

    /* 组帧 (Re)Assoc_req_Frame */
    if (pst_sta->bit_reassoc_flag)
    {
        puc_curr_bssid = pst_sta->st_vap_base_info.auc_bssid;
    }
    else
    {
        puc_curr_bssid = OAL_PTR_NULL;
    }
    ul_asoc_frame_len = hmac_mgmt_encap_asoc_req_sta_etc(pst_sta,(oal_uint8 *)(OAL_NETBUF_HEADER(pst_asoc_req_frame)), puc_curr_bssid);
    oal_netbuf_put(pst_asoc_req_frame, ul_asoc_frame_len);

    if (0 == ul_asoc_frame_len)
    {
        OAM_WARNING_LOG0(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_etc::hmac_mgmt_encap_asoc_req_sta_etc null.}");
        oal_netbuf_free(pst_asoc_req_frame);

        return OAL_FAIL;
    }

    if(OAL_UNLIKELY(ul_asoc_frame_len < OAL_ASSOC_REQ_IE_OFFSET))
    {
        OAM_ERROR_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_etc::invalid ul_asoc_req_ie_len[%u].}", ul_asoc_frame_len);
        oam_report_dft_params_etc(BROADCAST_MACADDR, (oal_uint8 *)oal_netbuf_header(pst_asoc_req_frame),(oal_uint16)ul_asoc_frame_len, OAM_OTA_TYPE_80211_FRAME);
        oal_netbuf_free(pst_asoc_req_frame);
        return OAL_FAIL;
    }
    /*Should we change the ie buff from local mem to netbuf ?*/
    /* 此处申请的内存，只在上报给内核后释放 */
    pst_hmac_user_ap = (hmac_user_stru *)mac_res_get_hmac_user_etc(pst_sta->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_user_ap)
    {
        OAM_ERROR_LOG0(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_etc::pst_hmac_user_ap null.}");
        oal_netbuf_free(pst_asoc_req_frame);
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_user_free_asoc_req_ie(pst_sta->st_vap_base_info.us_assoc_vap_id);
    ul_ret = hmac_user_set_asoc_req_ie(pst_hmac_user_ap,
                                           OAL_NETBUF_HEADER(pst_asoc_req_frame) + OAL_ASSOC_REQ_IE_OFFSET,
                                           ul_asoc_frame_len - OAL_ASSOC_REQ_IE_OFFSET,
                                           (oal_uint8)(pst_sta->bit_reassoc_flag));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_etc::hmac_user_set_asoc_req_ie failed}");
        oal_netbuf_free(pst_asoc_req_frame);
        return OAL_FAIL;
    }

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_asoc_req_frame);

    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)   = (oal_uint16)ul_asoc_frame_len;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_hmac_user_ap->st_user_base_info.us_assoc_id;

    /* 抛事件让DMAC将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event_etc(&(pst_sta->st_vap_base_info), pst_asoc_req_frame, (oal_uint16)ul_asoc_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_asoc_req_frame);
        hmac_user_free_asoc_req_ie(pst_sta->st_vap_base_info.us_assoc_vap_id);
        OAM_WARNING_LOG1(pst_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_etc::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 更改状态 */
    hmac_fsm_change_state_etc(pst_sta, MAC_VAP_STATE_STA_WAIT_ASOC);

    /* 启动关联超时定时器, 为对端ap分配一个定时器，如果超时ap没回asoc rsp则启动超时处理 */
    pst_sta->st_mgmt_timetout_param.en_state = MAC_VAP_STATE_STA_WAIT_ASOC;
    pst_sta->st_mgmt_timetout_param.us_user_index = pst_hmac_user_ap->st_user_base_info.us_assoc_id;
    pst_sta->st_mgmt_timetout_param.uc_vap_id = pst_sta->st_vap_base_info.uc_vap_id;
    pst_sta->st_mgmt_timetout_param.en_status_code = MAC_ASOC_RSP_TIMEOUT;

    FRW_TIMER_CREATE_TIMER(&(pst_sta->st_mgmt_timer),
                           hmac_mgmt_timeout_sta,
                           pst_hmac_asoc_req->us_assoc_timeout,
                           &(pst_sta->st_mgmt_timetout_param),
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_sta->st_vap_base_info.ul_core_id);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_P2P


oal_void hmac_p2p_listen_comp_cb_etc(void *p_arg)
{
    hmac_vap_stru                      *pst_hmac_vap;
    mac_device_stru                    *pst_mac_device;
    hmac_scan_record_stru              *pst_scan_record;

    pst_scan_record = (hmac_scan_record_stru *)p_arg;

    /* 判断listen完成时的状态 */
    if (MAC_SCAN_SUCCESS != pst_scan_record->en_scan_rsp_status)
    {
        OAM_WARNING_LOG1(0, OAM_SF_P2P, "{hmac_p2p_listen_comp_cb_etc::listen failed, listen rsp status: %d.}",
                         pst_scan_record->en_scan_rsp_status);
    }

    pst_hmac_vap   = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{hmac_p2p_listen_comp_cb_etc::pst_hmac_vap is null:vap_id %d.}",
                                 pst_scan_record->uc_vap_id);
        return;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_scan_record->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{hmac_p2p_listen_comp_cb_etc::pst_mac_device is null:vap_id %d.}",
                                 pst_scan_record->uc_device_id);
        return;
    }

    
    if (pst_scan_record->ull_cookie == pst_mac_device->st_p2p_info.ull_last_roc_id)
    {
        /* 状态机调用: hmac_p2p_listen_timeout_etc */
        if (OAL_SUCC != hmac_fsm_call_func_sta_etc(pst_hmac_vap, HMAC_FSM_INPUT_LISTEN_TIMEOUT, &(pst_hmac_vap->st_vap_base_info)))
            {
            OAM_WARNING_LOG0(0,OAM_SF_P2P,"{hmac_p2p_listen_comp_cb_etc::hmac_fsm_call_func_sta_etc fail.}");
            }
    }
    else
    {
        OAM_WARNING_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P,
                        "{hmac_p2p_listen_comp_cb_etc::ignore listen complete.scan_report_cookie[%x], current_listen_cookie[%x], ull_last_roc_id[%x].}",
                        pst_scan_record->ull_cookie,
                        pst_mac_device->st_scan_params.ull_cookie,
                        pst_mac_device->st_p2p_info.ull_last_roc_id);
    }


    return;
}

oal_void hmac_mgmt_tx_roc_comp_cb(void *p_arg)
{
    hmac_vap_stru                      *pst_hmac_vap;
    mac_device_stru                    *pst_mac_device;
    hmac_scan_record_stru              *pst_scan_record;

    pst_scan_record = (hmac_scan_record_stru *)p_arg;

    /* 判断listen完成时的状态 */
    if (MAC_SCAN_SUCCESS != pst_scan_record->en_scan_rsp_status)
    {
        OAM_WARNING_LOG1(0, OAM_SF_P2P, "{hmac_mgmt_tx_roc_comp_cb::listen failed, listen rsp status: %d.}",
                         pst_scan_record->en_scan_rsp_status);
    }

    pst_hmac_vap   = mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_P2P, "{hmac_mgmt_tx_roc_comp_cb::pst_hmac_vap is null:vap_id %d.}",
                                 pst_scan_record->uc_vap_id);
        return;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_scan_record->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{hmac_mgmt_tx_roc_comp_cb::pst_mac_device is null:vap_id %d.}",
                                 pst_scan_record->uc_device_id);
        return;
    }

    /* 由于P2P0 和P2P_CL 共用vap 结构体，监听超时，返回监听前保存的状态 */
    mac_vap_state_change_etc(&pst_hmac_vap->st_vap_base_info, pst_mac_device->st_p2p_info.en_last_vap_state);

    OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P, "{hmac_mgmt_tx_roc_comp_cb}");
}


OAL_STATIC oal_void  hmac_cfg80211_prepare_listen_req_param(mac_scan_req_stru *pst_scan_params, oal_int8 *puc_param)
{
    mac_remain_on_channel_param_stru *pst_remain_on_channel;
    mac_channel_stru                 *pst_channel_tmp;

    pst_remain_on_channel = (mac_remain_on_channel_param_stru *)puc_param;

    OAL_MEMZERO(pst_scan_params, OAL_SIZEOF(mac_scan_req_stru));

    /* 设置监听信道信息到扫描参数中 */
    pst_scan_params->ast_channel_list[0].en_band        = pst_remain_on_channel->en_band;
    pst_scan_params->ast_channel_list[0].en_bandwidth   = pst_remain_on_channel->en_listen_channel_type;
    pst_scan_params->ast_channel_list[0].uc_chan_number = pst_remain_on_channel->uc_listen_channel;
    pst_scan_params->ast_channel_list[0].uc_chan_idx         = 0;
    pst_channel_tmp = &(pst_scan_params->ast_channel_list[0]);
    if (OAL_SUCC != mac_get_channel_idx_from_num_etc(pst_channel_tmp->en_band, pst_channel_tmp->uc_chan_number, &(pst_channel_tmp->uc_chan_idx)))
    {
        OAM_WARNING_LOG2(0,OAM_SF_P2P,"{hmac_cfg80211_prepare_listen_req_param::mac_get_channel_idx_from_num_etc fail.band[%u]  channel[%u]}",pst_channel_tmp->en_band, pst_channel_tmp->uc_chan_number);
    }

    /* 设置其它监听参数 */
    pst_scan_params->uc_max_scan_count_per_channel      = 1;
    pst_scan_params->uc_channel_nums = 1;
    pst_scan_params->uc_scan_func    = MAC_SCAN_FUNC_P2P_LISTEN;
    pst_scan_params->us_scan_time    = (oal_uint16)pst_remain_on_channel->ul_listen_duration;
    if (IEEE80211_ROC_TYPE_MGMT_TX == pst_remain_on_channel->en_roc_type)
    {
        pst_scan_params->p_fn_cb         = hmac_mgmt_tx_roc_comp_cb;
    }
    else
    {
        pst_scan_params->p_fn_cb         = hmac_p2p_listen_comp_cb_etc;
    }
    pst_scan_params->ull_cookie      = pst_remain_on_channel->ull_cookie;
    pst_scan_params->bit_is_p2p0_scan = OAL_TRUE;
    pst_scan_params->uc_p2p0_listen_channel = pst_remain_on_channel->uc_listen_channel;

    return;
}


oal_uint32  hmac_p2p_listen_timeout_etc(hmac_vap_stru *pst_hmac_vap_sta, oal_void *p_param)
{
    mac_device_stru                    *pst_mac_device;
    hmac_vap_stru                      *pst_hmac_vap;
    mac_vap_stru                       *pst_mac_vap;
    hmac_device_stru                   *pst_hmac_device;
    hmac_scan_record_stru              *pst_scan_record;

    pst_mac_vap    = (mac_vap_stru *)p_param;
    pst_hmac_vap   = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(0,OAM_SF_P2P,"{hmac_p2p_listen_timeout_etc::mac_res_get_hmac_vap fail.vap_id[%u]!}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(0,OAM_SF_P2P,"{hmac_p2p_listen_timeout_etc::mac_res_get_dev_etc fail.device_id[%u]!}",pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取hmac device */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_device->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{hmac_p2p_listen_timeout_etc::pst_hmac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_INFO_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P,
                  "{hmac_p2p_listen_timeout_etc::current pst_mac_vap channel is [%d] state[%d]}",
                  pst_mac_vap->st_channel.uc_chan_number,
                  pst_hmac_vap->st_vap_base_info.en_vap_state);

    OAM_INFO_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P,
                  "{hmac_p2p_listen_timeout_etc::next pst_mac_vap channel is [%d] state[%d]}",
                  pst_mac_vap->st_channel.uc_chan_number,
                  pst_mac_device->st_p2p_info.en_last_vap_state);

    /* 由于P2P0 和P2P_CL 共用vap 结构体，监听超时，返回监听前保存的状态 */
    mac_vap_state_change_etc(&pst_hmac_vap->st_vap_base_info, pst_mac_device->st_p2p_info.en_last_vap_state);

    pst_scan_record = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);
    if (pst_scan_record->ull_cookie == pst_mac_device->st_p2p_info.ull_last_roc_id)
    {
        /* 3.1 抛事件到WAL ，上报监听结束 */
        hmac_p2p_send_listen_expired_to_host_etc(pst_hmac_vap);
    }

    /* 3.2 抛事件到DMAC ，返回监听信道 */
    hmac_p2p_send_listen_expired_to_device_etc(pst_hmac_vap);
    return OAL_SUCC;
}

#if 0

oal_uint32 hmac_p2p_listen_timeout_fn(void *p_arg)
{
    hmac_vap_stru                      *pst_hmac_vap;
    mac_vap_stru                       *pst_mac_vap;

    pst_mac_vap    = (mac_vap_stru *)p_arg;
    pst_hmac_vap   = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    /* 状态机调用: hmac_p2p_listen_timeout_etc */
    hmac_fsm_call_func_sta_etc(pst_hmac_vap, HMAC_FSM_INPUT_LISTEN_TIMEOUT, pst_mac_vap);

    return OAL_SUCC;
}
#endif


oal_uint32 hmac_p2p_remain_on_channel_etc(hmac_vap_stru *pst_hmac_vap_sta, oal_void *p_param)
{
    mac_device_stru                     *pst_mac_device;
    mac_vap_stru                        *pst_mac_vap;
    mac_remain_on_channel_param_stru    *pst_remain_on_channel;
    mac_scan_req_stru                    st_scan_params;
    oal_uint32                           ul_ret;

    pst_remain_on_channel = (mac_remain_on_channel_param_stru*)p_param;

    pst_mac_vap     = mac_res_get_mac_vap(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id);
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG1(0,OAM_SF_P2P,"{hmac_p2p_remain_on_channel_etc::mac_res_get_mac_vap fail.vap_id[%u]!}",pst_hmac_vap_sta->st_vap_base_info.uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device  = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_p2p_listen_timeout_etc::pst_mac_device[%d](%p) null!}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    
    if (MAC_VAP_STATE_STA_LISTEN == pst_hmac_vap_sta->st_vap_base_info.en_vap_state)
    {
        hmac_p2p_send_listen_expired_to_host_etc(pst_hmac_vap_sta);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                        "{hmac_p2p_remain_on_channel_etc::listen nested, send remain on channel expired to host!curr_state[%d]\r\n}",
                        pst_hmac_vap_sta->st_vap_base_info.en_vap_state);
    }

    /* 修改P2P_DEVICE 状态为监听状态 */
    mac_vap_state_change_etc((mac_vap_stru *)&pst_hmac_vap_sta->st_vap_base_info, MAC_VAP_STATE_STA_LISTEN);
    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                  "{hmac_p2p_remain_on_channel_etc : get in listen state!last_state %d, channel %d, duration %d, curr_state %d}\r\n",
                  pst_mac_device->st_p2p_info.en_last_vap_state,
                  pst_remain_on_channel->uc_listen_channel,
                  pst_remain_on_channel->ul_listen_duration,
                  pst_hmac_vap_sta->st_vap_base_info.en_vap_state);

    /* 准备监听参数 */
    hmac_cfg80211_prepare_listen_req_param(&st_scan_params, (oal_int8 *)pst_remain_on_channel);

    /* 调用扫描入口，准备进行监听动作，不管监听动作执行成功或失败，都返回监听成功 */
    /* 状态机调用: hmac_scan_proc_scan_req_event_etc */
    ul_ret = hmac_fsm_call_func_sta_etc(pst_hmac_vap_sta, HMAC_FSM_INPUT_SCAN_REQ, (oal_void *)(&st_scan_params));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_SCAN, "{hmac_p2p_remain_on_channel_etc::hmac_fsm_call_func_sta_etc fail[%d].}", ul_ret);
    }

    return OAL_SUCC;
}


#endif /* _PRE_WLAN_FEATURE_P2P */

#if defined(_PRE_WLAN_FEATURE_HS20) || defined(_PRE_WLAN_FEATURE_P2P) || defined(_PRE_WLAN_FEATURE_HILINK)


oal_uint32 hmac_sta_not_up_rx_mgmt_etc(hmac_vap_stru *pst_hmac_vap_sta, oal_void *p_param)
{
    dmac_wlan_crx_event_stru   *pst_mgmt_rx_event;
    mac_rx_ctl_stru            *pst_rx_info;
    oal_uint8                  *puc_mac_hdr;
    oal_uint8                  *puc_data;
    oal_uint8                   uc_mgmt_frm_type;
    mac_ieee80211_frame_stru   *pst_frame_hdr;          /* 保存mac帧的指针 */
    hmac_user_stru             *pst_hmac_user;
#ifdef _PRE_WLAN_FEATURE_P2P
    mac_vap_stru               *pst_mac_vap;
#endif
    if (OAL_PTR_NULL == pst_hmac_vap_sta || OAL_PTR_NULL == p_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{hmac_sta_not_up_rx_mgmt_etc::PTR_NULL, hmac_vap_addr[%p], param[%p].}",
                       pst_hmac_vap_sta, p_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mgmt_rx_event  =  (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_info        =  (mac_rx_ctl_stru *)oal_netbuf_cb(pst_mgmt_rx_event->pst_netbuf);
    puc_mac_hdr        =  (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_info);
    if(OAL_PTR_NULL == puc_mac_hdr)
    {
        OAM_ERROR_LOG3(0, OAM_SF_RX, "{hmac_sta_not_up_rx_mgmt_etc::puc_mac_hdr null, vap_id %d,us_frame_len %d, uc_mac_header_len %d}",
                       pst_rx_info->bit_vap_id,
                       pst_rx_info->us_frame_len,
                       pst_rx_info->uc_mac_header_len);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取帧体指针 */
    puc_data = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_info) + pst_rx_info->uc_mac_header_len;
    /* STA在NOT UP状态下接收到各种管理帧处理*/
    uc_mgmt_frm_type = mac_get_frame_type_and_subtype(puc_mac_hdr);


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_sta_not_up_rx_mgmt_etc::wakeup mgmt type[0x%x]}",uc_mgmt_frm_type);
    }
#endif
#endif

    switch (uc_mgmt_frm_type)
    {
        /* 判断接收到的管理帧类型*/
        case WLAN_FC0_SUBTYPE_PROBE_REQ|WLAN_FC0_TYPE_MGT:
            #ifdef _PRE_WLAN_FEATURE_P2P
            pst_mac_vap = &(pst_hmac_vap_sta->st_vap_base_info);
            /* 判断为P2P设备,则上报probe req帧到wpa_supplicant*/
            if (!IS_LEGACY_VAP(pst_mac_vap))
            {
                hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap_sta, pst_mgmt_rx_event->pst_netbuf);
            }
            break;
            #endif
        case WLAN_FC0_SUBTYPE_ACTION|WLAN_FC0_TYPE_MGT:
        #if defined(_PRE_WLAN_FEATURE_LOCATION) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
            if (0 == oal_memcmp(puc_data  + 2, g_auc_huawei_oui, MAC_OUI_LEN))
            {
               hmac_huawei_action_process(pst_hmac_vap_sta, pst_mgmt_rx_event->pst_netbuf, puc_data[2 + MAC_OUI_LEN]);
            }
            else
        #endif
            /* 如果是Action 帧，则直接上报wpa_supplicant*/
            {
                pst_frame_hdr = (mac_ieee80211_frame_stru *)puc_mac_hdr;
                pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(&pst_hmac_vap_sta->st_vap_base_info, pst_frame_hdr->auc_address2);

                OAM_WARNING_LOG0(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_sta_not_up_rx_mgmt_etc::MAC_ACTION_CATEGORY_WNM.}");
                switch (puc_data[MAC_ACTION_OFFSET_CATEGORY])
                {
                    case MAC_ACTION_CATEGORY_WNM:
                        switch (puc_data[MAC_ACTION_OFFSET_ACTION])
                        {
                        #ifdef _PRE_WLAN_FEATURE_11V_ENABLE
                            /* bss transition request 帧处理入口 */
                            case MAC_WNM_ACTION_BSS_TRANSITION_MGMT_REQUEST:
                                if (OAL_PTR_NULL != pst_hmac_user) /* BSS Transtion Request hmac user exist */
                                {
                                    hmac_rx_bsst_req_action(pst_hmac_vap_sta, pst_hmac_user, pst_mgmt_rx_event->pst_netbuf);
                                }
                                else
                                {
                                    OAM_WARNING_LOG0(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                                        "{hmac_sta_not_up_rx_mgmt_etc:: bss transition pst_hmac_user don't exist.}");
                                }
                                break;
                        #endif
                            default:
                                hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap_sta, pst_mgmt_rx_event->pst_netbuf);
                                break;
                        }
                        break;
                    default:
                        hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap_sta, pst_mgmt_rx_event->pst_netbuf);
                        break;
                }
            }
            break;
        case WLAN_FC0_SUBTYPE_PROBE_RSP|WLAN_FC0_TYPE_MGT:
            /* 如果是probe response帧，则直接上报wpa_supplicant*/
            hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap_sta, pst_mgmt_rx_event->pst_netbuf);
            break;
        default:
            break;
    }
    return OAL_SUCC;
}
#endif /* _PRE_WLAN_FEATURE_HS20 and _PRE_WLAN_FEATURE_P2P and _PRE_WLAN_FEATURE_HILINK */


OAL_STATIC oal_uint32  hmac_update_vht_opern_ie_sta(
                    mac_vap_stru            *pst_mac_vap,
                    hmac_user_stru          *pst_hmac_user,
                    oal_uint8               *puc_payload)
{
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == puc_payload)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ASSOC, "{hmac_update_vht_opern_ie_sta::param null,%d %d %d.}", pst_mac_vap, pst_hmac_user, puc_payload);
        return MAC_NO_CHANGE;
    }

    /* 支持11ac，才进行后续的处理 */
    if (OAL_FALSE == mac_mib_get_VHTOptionImplemented(pst_mac_vap))
    {
        return MAC_NO_CHANGE;
    }

    return mac_ie_proc_vht_opern_ie_etc(pst_mac_vap, puc_payload, &(pst_hmac_user->st_user_base_info));
}

#ifdef _PRE_WLAN_FEATURE_11AX

OAL_STATIC oal_uint32  hmac_update_he_opern_ie_sta(
                    mac_vap_stru            *pst_mac_vap,
                    hmac_user_stru          *pst_hmac_user,
                    oal_uint8               *puc_payload)
{
    mac_user_stru                     *pst_mac_user;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == puc_payload)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ASSOC, "{hmac_update_he_opern_ie_sta::param null,%X %X %X.}", pst_mac_vap, pst_hmac_user, puc_payload);
        return MAC_NO_CHANGE;
    }

    /* 支持11ax，才进行后续的处理 */
    if (OAL_FALSE == mac_mib_get_HEOptionImplemented(pst_mac_vap))
    {
        return MAC_NO_CHANGE;
    }

    pst_mac_user = &(pst_hmac_user->st_user_base_info);
    return mac_ie_proc_he_opern_ie(pst_mac_vap,puc_payload,pst_mac_user);
}


OAL_STATIC oal_void hmac_sta_up_update_he_edca_params_mib(hmac_vap_stru  *pst_hmac_sta, mac_frame_he_mu_ac_parameter  *pst_ac_param)
{
    oal_uint8               uc_aci;
    oal_uint8               uc_mu_edca_timer;
    oal_uint32              ul_txop_limit;
    mac_vap_rom_stru       *pst_mac_vap_rom;

    /******** AC Parameters Record Format *********/
    /* ------------------------------------------ */
    /* |     1     |       1       |      1     | */
    /* ------------------------------------------ */
    /* | ACI/AIFSN | ECWmin/ECWmax | MU EDCA Timer | */
    /* ------------------------------------------ */

    /************* ACI/AIFSN Field ***************/
    /*     ---------------------------------- */
    /* bit |   4   |  1  |  2  |    1     |   */
    /*     ---------------------------------- */
    /*     | AIFSN | ACM | ACI | Reserved |   */
    /*     ---------------------------------- */

    uc_aci   = pst_ac_param->bit_ac_index;;

    /* ECWmin/ECWmax Field */
    /*     ------------------- */
    /* bit |   4    |   4    | */
    /*     ------------------- */
    /*     | ECWmin | ECWmax | */
    /*     ------------------- */

    /* 在mib库中和寄存器里保存的TXOP值都是以us为单位的
       ==MU EDCA Timer 单位是8TU --8 * 1024us
    */
    uc_mu_edca_timer = pst_ac_param->uc_mu_edca_timer;
    ul_txop_limit    = (oal_uint32)((uc_mu_edca_timer<<3)<<10);

    pst_mac_vap_rom  = (mac_vap_rom_stru *)pst_hmac_sta->st_vap_base_info._rom;
    if(OAL_PTR_NULL == pst_mac_vap_rom)
    {
        return;
    }

    /* 更新相应的MIB库信息 */
    if (uc_aci < WLAN_WME_AC_BUTT)
    {
        OAM_WARNING_LOG4(0, OAM_SF_ASSOC, "{hmac_sta_up_update_he_edca_params_mib::uc_aci=[%d],aifsn=[%d],ecw_min=[%d],ul_txop_limit=[%d].}",
        uc_aci, pst_ac_param->bit_aifsn, pst_ac_param->bit_ecw_min, ul_txop_limit);
        mac_mib_set_QAPMUEDCATableIndex(pst_mac_vap_rom, uc_aci, uc_aci + 1);  /* 注: 协议规定取值1 2 3 4 */
        mac_mib_set_QAPMUEDCATableCWmin(pst_mac_vap_rom, uc_aci, pst_ac_param->bit_ecw_min);
        mac_mib_set_QAPMUEDCATableCWmax(pst_mac_vap_rom, uc_aci, pst_ac_param->bit_ecw_max);
        mac_mib_set_QAPMUEDCATableAIFSN(pst_mac_vap_rom, uc_aci, pst_ac_param->bit_aifsn);
        mac_mib_set_QAPMUEDCATableTXOPLimit(pst_mac_vap_rom, uc_aci, ul_txop_limit);
        mac_mib_set_QAPMUEDCATableMandatory(pst_mac_vap_rom, uc_aci, pst_ac_param->bit_acm);
    }
}



oal_void hmac_sta_up_update_he_edca_params(
                    oal_uint8               *puc_payload,
                    oal_uint16               us_msg_len,
                    hmac_vap_stru           *pst_hmac_sta,
                    oal_uint8                uc_frame_sub_type,
                    hmac_user_stru          *pst_hmac_user)
{
    oal_uint8                                uc_ac_num_loop;
    oal_uint32                               ul_ret;
    mac_device_stru                         *pst_mac_device;
    oal_uint8                               *puc_ie     = OAL_PTR_NULL;
    mac_frame_he_mu_edca_parameter_ie_stru   st_mu_edca_value;

    if (OAL_FALSE == mac_mib_get_HEOptionImplemented(&(pst_hmac_sta->st_vap_base_info)))
    {
        return;
    }

    pst_mac_device = (mac_device_stru *)mac_res_get_dev_etc(pst_hmac_sta->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(0,OAM_SF_ASSOC,"{hmac_sta_up_update_he_edca_params::mac_res_get_dev_etc fail.device_id[%u]!}",pst_hmac_sta->st_vap_base_info.uc_device_id);
        return;
    }

    /************************ MU EDCA Parameter Set Element ***************************/
    /* ------------------------------------------------------------------------------------------- */
    /* | EID | LEN | Ext EID|MU Qos Info |MU AC_BE Parameter Record | MU AC_BK Parameter Record  | */
    /* ------------------------------------------------------------------------------------------- */
    /* |  1  |  1  |   1    |    1       |     3                    |        3                   | */
    /* ------------------------------------------------------------------------------------------- */
    /* ------------------------------------------------------------------------------ -------------*/
    /* | MU AC_VI Parameter Record | MU AC_VO Parameter Record                                   | */
    /* ------------------------------------------------------------------------------------------- */
    /* |    3                      |     3                                                       | */

    /******************* QoS Info field when sent from WMM AP *****************/
    /* --------------------------------------------------------------------------------------------*/
    /*    | EDCA Parameter Set Update Count | Q-Ack | Queue Request |TXOP Request | More Data Ack| */
    /*---------------------------------------------------------------------------------------------*/
    /*bit |        0~3                      |  1    |  1            |   1         |     1        | */
    /*---------------------------------------------------------------------------------------------*/
    /**************************************************************************/

    puc_ie = mac_find_ie_ext_ie(MAC_EID_HE, MAC_EID_EXT_HE_EDCA,puc_payload,us_msg_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        OAL_MEMZERO(&st_mu_edca_value, OAL_SIZEOF(st_mu_edca_value));
        ul_ret = mac_ie_parse_mu_edca_parameter(puc_ie, &st_mu_edca_value);
        if(OAL_SUCC != ul_ret)
        {
            return;
        }

        /* 如果收到的是beacon帧，并且param_set_count没有改变
           则STA也不用做任何改变，直接返回即可 */
        if ((WLAN_FC0_SUBTYPE_BEACON == uc_frame_sub_type)
            && (st_mu_edca_value.st_qos_info.bit_edca_update_count == pst_hmac_sta->st_vap_base_info.uc_he_mu_edca_update_count))
        {
            return;
        }

        pst_hmac_sta->st_vap_base_info.uc_he_mu_edca_update_count = st_mu_edca_value.st_qos_info.bit_edca_update_count;

        /* 针对每一个AC，更新EDCA参数 */
        for (uc_ac_num_loop = 0; uc_ac_num_loop < WLAN_WME_AC_BUTT; uc_ac_num_loop++)
        {
            hmac_sta_up_update_he_edca_params_mib(pst_hmac_sta,&(st_mu_edca_value.ast_mu_ac_parameter[uc_ac_num_loop]));
        }

        /* 更新EDCA相关的MAC寄存器 */
        hmac_sta_up_update_mu_edca_params_machw(pst_hmac_sta, MAC_WMM_SET_PARAM_TYPE_UPDATE_EDCA);
        return;
    }

}


oal_uint32 hmac_sta_up_update_mu_edca_params_machw(
                      hmac_vap_stru                          *pst_hmac_sta,
                      mac_wmm_set_param_type_enum_uint8       en_type)
{

    frw_event_mem_stru                       *pst_event_mem;
    frw_event_stru                           *pst_event;
    dmac_ctx_sta_asoc_set_edca_reg_stru       st_asoc_set_edca_reg_param = {0};
    mac_vap_rom_stru                         *pst_mac_vap_rom;

    pst_mac_vap_rom  = (mac_vap_rom_stru *)pst_hmac_sta->st_vap_base_info._rom;
    if(OAL_PTR_NULL == pst_mac_vap_rom)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 抛事件到dmac写寄存器 */
    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_update_vht_opern_ie_sta::event_mem alloc null, size[%d].}",
                OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_asoc_set_edca_reg_param.uc_vap_id = pst_hmac_sta->st_vap_base_info.uc_vap_id;
    st_asoc_set_edca_reg_param.en_set_param_type = en_type;

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                   FRW_EVENT_TYPE_WLAN_CTX,
                   DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_MU_EDCA_REG,
                   OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru),
                   FRW_EVENT_PIPELINE_STAGE_1,
                   pst_hmac_sta->st_vap_base_info.uc_chip_id,
                   pst_hmac_sta->st_vap_base_info.uc_device_id,
                   pst_hmac_sta->st_vap_base_info.uc_vap_id);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (MAC_WMM_SET_PARAM_TYPE_DEFAULT != en_type)
    {
        oal_memcopy((oal_uint8 *)&st_asoc_set_edca_reg_param.ast_wlan_mib_qap_edac, (oal_uint8 *)pst_mac_vap_rom->st_wlan_mib_mu_edca.ast_wlan_mib_qap_edac, (OAL_SIZEOF(wlan_mib_Dot11QAPEDCAEntry_stru) * WLAN_WME_AC_BUTT));
    }
#endif

    /* 拷贝参数 */
    oal_memcopy(frw_get_event_payload(pst_event_mem), (oal_uint8 *)&st_asoc_set_edca_reg_param, OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}




#endif


oal_uint32 hmac_sta_up_update_edca_params_machw_etc(
                      hmac_vap_stru                          *pst_hmac_sta,
                      mac_wmm_set_param_type_enum_uint8      en_type)
{
    frw_event_mem_stru                       *pst_event_mem;
    frw_event_stru                           *pst_event;
    dmac_ctx_sta_asoc_set_edca_reg_stru       st_asoc_set_edca_reg_param = {0};

    /* 抛事件到dmac写寄存器 */
    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_update_vht_opern_ie_sta::event_mem alloc null, size[%d].}",
                OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_asoc_set_edca_reg_param.uc_vap_id = pst_hmac_sta->st_vap_base_info.uc_vap_id;
    st_asoc_set_edca_reg_param.en_set_param_type = en_type;

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                   FRW_EVENT_TYPE_WLAN_CTX,
                   DMAC_WLAN_CTX_EVENT_SUB_TYPE_STA_SET_EDCA_REG,
                   OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru),
                   FRW_EVENT_PIPELINE_STAGE_1,
                   pst_hmac_sta->st_vap_base_info.uc_chip_id,
                   pst_hmac_sta->st_vap_base_info.uc_device_id,
                   pst_hmac_sta->st_vap_base_info.uc_vap_id);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (MAC_WMM_SET_PARAM_TYPE_DEFAULT != en_type)
    {
        oal_memcopy((oal_uint8 *)&st_asoc_set_edca_reg_param.ast_wlan_mib_qap_edac, (oal_uint8 *)&pst_hmac_sta->st_vap_base_info.pst_mib_info->st_wlan_mib_edca.ast_wlan_mib_qap_edac, (OAL_SIZEOF(wlan_mib_Dot11QAPEDCAEntry_stru) * WLAN_WME_AC_BUTT));
    }
#endif

    /* 拷贝参数 */
    oal_memcopy(frw_get_event_payload(pst_event_mem), (oal_uint8 *)&st_asoc_set_edca_reg_param, OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_sta_up_update_edca_params_mib(hmac_vap_stru  *pst_hmac_sta, oal_uint8  *puc_payload)
{
    oal_uint8               uc_aifsn;
    oal_uint8               uc_aci;
    oal_uint8               uc_ecwmin;
    oal_uint8               uc_ecwmax;
    oal_uint16              us_txop_limit;
    oal_bool_enum_uint8     en_acm;
    /******** AC Parameters Record Format *********/
    /* ------------------------------------------ */
    /* |     1     |       1       |      2     | */
    /* ------------------------------------------ */
    /* | ACI/AIFSN | ECWmin/ECWmax | TXOP Limit | */
    /* ------------------------------------------ */

    /************* ACI/AIFSN Field ***************/
    /*     ---------------------------------- */
    /* bit |   4   |  1  |  2  |    1     |   */
    /*     ---------------------------------- */
    /*     | AIFSN | ACM | ACI | Reserved |   */
    /*     ---------------------------------- */

    uc_aifsn = puc_payload[0] & MAC_WMM_QOS_PARAM_AIFSN_MASK;
    en_acm   = (puc_payload[0] & BIT4)?OAL_TRUE:OAL_FALSE;
    uc_aci   = (puc_payload[0] >> MAC_WMM_QOS_PARAM_ACI_BIT_OFFSET) & MAC_WMM_QOS_PARAM_ACI_MASK;

    /* ECWmin/ECWmax Field */
    /*     ------------------- */
    /* bit |   4    |   4    | */
    /*     ------------------- */
    /*     | ECWmin | ECWmax | */
    /*     ------------------- */
    uc_ecwmin = (puc_payload[1] & MAC_WMM_QOS_PARAM_ECWMIN_MASK);
    uc_ecwmax = ((puc_payload[1] & MAC_WMM_QOS_PARAM_ECWMAX_MASK) >> MAC_WMM_QOS_PARAM_ECWMAX_BIT_OFFSET);

    /* 在mib库中和寄存器里保存的TXOP值都是以us为单位的，但是传输的时候是以32us为
       单位进行传输的，因此在解析的时候需要将解析到的值乘以32
    */
    us_txop_limit = puc_payload[2] |
                    ((puc_payload[3] & MAC_WMM_QOS_PARAM_TXOPLIMIT_MASK) << MAC_WMM_QOS_PARAM_BIT_NUMS_OF_ONE_BYTE);
    us_txop_limit = (oal_uint16)(us_txop_limit << MAC_WMM_QOS_PARAM_TXOPLIMIT_SAVE_TO_TRANS_TIMES);

    /* 更新相应的MIB库信息 */
    if (uc_aci < WLAN_WME_AC_BUTT)
    {
        mac_mib_set_QAPEDCATableIndex(&pst_hmac_sta->st_vap_base_info, uc_aci, uc_aci + 1);  /* 注: 协议规定取值1 2 3 4 */
        mac_mib_set_QAPEDCATableCWmin(&pst_hmac_sta->st_vap_base_info, uc_aci, uc_ecwmin);
        mac_mib_set_QAPEDCATableCWmax(&pst_hmac_sta->st_vap_base_info, uc_aci, uc_ecwmax);
        mac_mib_set_QAPEDCATableAIFSN(&pst_hmac_sta->st_vap_base_info, uc_aci, uc_aifsn);
        mac_mib_set_QAPEDCATableTXOPLimit(&pst_hmac_sta->st_vap_base_info, uc_aci, us_txop_limit);
        mac_mib_set_QAPEDCATableMandatory(&pst_hmac_sta->st_vap_base_info, uc_aci, en_acm);
    }
}

#if 0

OAL_STATIC oal_void hmac_sta_up_process_erp_ie(
                    oal_uint8               *puc_payload,
                    oal_uint16               us_msg_len,
                    oal_uint16               us_info_elem_offset,
                    hmac_user_stru          *pst_hmac_user)
{
    oal_uint16              us_msg_offset;
    mac_erp_params_stru    *pst_erp_params;

    /* 设置偏移 */
    us_msg_offset = us_info_elem_offset;

    /************************ ERP Element *************************************
    --------------------------------------------------------------------------
    |EID  |Len  |NonERP_Present|Use_Protection|Barker_Preamble_Mode|Reserved|
    --------------------------------------------------------------------------
    |B0-B7|B0-B7|B0            |B1            |B2                  |B3-B7   |
    --------------------------------------------------------------------------
    ***************************************************************************/

    while (us_msg_offset < us_msg_len)
    {
        /* 判断当前的ie是否是ERP ie，如果不是，继续检查下一个ie，如果是，保存use protection 信息 */
        if (MAC_EID_ERP == puc_payload[us_msg_offset])
        {
            us_msg_offset += MAC_IE_HDR_LEN;
            pst_erp_params = (mac_erp_params_stru *)(&puc_payload[us_msg_offset]);

            /* 保存use_protect 信息 */
            pst_hmac_user->st_user_base_info.st_cap_info.bit_erp_use_protect = pst_erp_params->bit_use_protection;
            /*保存preamble mode*/
            mac_user_set_barker_preamble_mode_etc(&pst_hmac_user->st_user_base_info, pst_erp_params->bit_preamble_mode);
            return ;
        }

        us_msg_offset += (puc_payload[us_msg_offset + 1] + MAC_IE_HDR_LEN);
    }

}


OAL_STATIC oal_void hmac_sta_up_process_ht_operation_ie(
                    oal_uint8               *puc_payload,
                    oal_uint16               us_msg_len,
                    oal_uint16               us_info_elem_offset,
                    hmac_user_stru          *pst_hmac_user)
{
    oal_uint16              us_msg_offset;
    mac_ht_opern_stru      *pst_ht_opern;

    /* 设置偏移 */
    us_msg_offset = us_info_elem_offset;

    /************************ HT Operation Element *************************************
      ----------------------------------------------------------------------
      |EID |Length |PrimaryChannel |HT Operation Information |Basic MCS Set|
      ----------------------------------------------------------------------
      |1   |1      |1              |5                        |16           |
      ----------------------------------------------------------------------
    ***************************************************************************/

    /************************ HT Information Field ****************************
     |--------------------------------------------------------------------|
     | Primary | Seconday  | STA Ch | RIFS |           reserved           |
     | Channel | Ch Offset | Width  | Mode |                              |
     |--------------------------------------------------------------------|
     |    1    | B0     B1 |   B2   |  B3  |    B4                     B7 |
     |--------------------------------------------------------------------|

     |----------------------------------------------------------------|
     |     HT     | Non-GF STAs | resv      | OBSS Non-HT  | Reserved |
     | Protection |   Present   |           | STAs Present |          |
     |----------------------------------------------------------------|
     | B0     B1  |     B2      |    B3     |     B4       | B5   B15 |
     |----------------------------------------------------------------|

     |-------------------------------------------------------------|
     | Reserved |  Dual  |  Dual CTS  | Seconday | LSIG TXOP Protn |
     |          | Beacon | Protection |  Beacon  | Full Support    |
     |-------------------------------------------------------------|
     | B0    B5 |   B6   |     B7     |     B8   |       B9        |
     |-------------------------------------------------------------|

     |---------------------------------------|
     |  PCO   |  PCO  | Reserved | Basic MCS |
     | Active | Phase |          |    Set    |
     |---------------------------------------|
     |  B10   |  B11  | B12  B15 |    16     |
     |---------------------------------------|
    **************************************************************************/

    while (us_msg_offset < us_msg_len)
    {
        /* 判断当前的ie是否是ht operation ie，如果不是，继续检查下一个ie，如果是，保存HT信息 */
        if (MAC_EID_HT_OPERATION == puc_payload[us_msg_offset])
        {
            us_msg_offset += MAC_IE_HDR_LEN;
            pst_ht_opern  = (mac_ht_opern_stru *)(&puc_payload[us_msg_offset]);

            /*从帧体中取值*/
            pst_hmac_user->st_user_base_info.st_ht_hdl.bit_rifs_mode                         = pst_ht_opern->bit_rifs_mode;/*发送描述符填写时候需要此值*/
            pst_hmac_user->st_user_base_info.st_ht_hdl.bit_HT_protection                     = pst_ht_opern->bit_HT_protection;
            pst_hmac_user->st_user_base_info.st_ht_hdl.bit_nongf_sta_present                 = pst_ht_opern->bit_nongf_sta_present;
            pst_hmac_user->st_user_base_info.st_ht_hdl.bit_obss_nonht_sta_present            = pst_ht_opern->bit_obss_nonht_sta_present;
            pst_hmac_user->st_user_base_info.st_ht_hdl.bit_lsig_txop_protection_full_support = pst_ht_opern->bit_lsig_txop_protection_full_support;
            return ;
        }

        us_msg_offset += (puc_payload[us_msg_offset + 1] + MAC_IE_HDR_LEN);
    }

}
#endif


oal_void hmac_sta_up_update_edca_params_etc(
                    oal_uint8               *puc_payload,
                    oal_uint16               us_msg_len,
                    hmac_vap_stru           *pst_hmac_sta,
                    oal_uint8                uc_frame_sub_type,
                    hmac_user_stru          *pst_hmac_user)
{
    oal_uint16              us_msg_offset = 0;
    oal_uint8               uc_param_set_cnt;
    oal_uint8               uc_ac_num_loop;
    oal_uint32              ul_ret;
    mac_device_stru        *pst_mac_device;
    oal_uint8               uc_edca_param_set;
    oal_uint8              *puc_ie     = OAL_PTR_NULL;
#ifdef _PRE_WLAN_FEATURE_WMMAC
    oal_bool_enum_uint8     en_wmmac_auth_flag;
#endif

    pst_mac_device = (mac_device_stru *)mac_res_get_dev_etc(pst_hmac_sta->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(0,OAM_SF_ASSOC,"{hmac_sta_up_update_edca_params_etc::mac_res_get_dev_etc fail.device_id[%u]!}",pst_hmac_sta->st_vap_base_info.uc_device_id);
        return;
    }

    /************************ WMM Parameter Element ***************************/
    /* ------------------------------------------------------------------------------ */
    /* | EID | LEN | OUI |OUI Type |OUI Subtype |Version |QoS Info |Resd |AC Params | */
    /* ------------------------------------------------------------------------------ */
    /* |  1  |  1  |  3  |    1    |     1      |    1   |    1    |  1  |    16    | */
    /* ------------------------------------------------------------------------------ */

    /******************* QoS Info field when sent from WMM AP *****************/
    /*        --------------------------------------------                    */
    /*          | Parameter Set Count | Reserved | U-APSD |                   */
    /*          --------------------------------------------                  */
    /*   bit    |        0~3          |   4~6    |   7    |                   */
    /*          --------------------------------------------                  */
    /**************************************************************************/
    puc_ie = mac_get_wmm_ie_etc(puc_payload, us_msg_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        /* 解析wmm ie是否携带EDCA参数 */
        uc_edca_param_set = puc_ie[MAC_OUISUBTYPE_WMM_PARAM_OFFSET];
        uc_param_set_cnt  = puc_ie[HMAC_WMM_QOS_PARAMS_HDR_LEN] & 0x0F;

        /* 如果收到的是beacon帧，并且param_set_count没有改变，说明AP的WMM参数没有变
           则STA也不用做任何改变，直接返回即可 */
        if ((WLAN_FC0_SUBTYPE_BEACON == uc_frame_sub_type)
            && (uc_param_set_cnt == pst_hmac_sta->st_vap_base_info.uc_wmm_params_update_count))
        {
            return;
        }

        pst_mac_device->en_wmm = OAL_TRUE;

        if(WLAN_FC0_SUBTYPE_BEACON == uc_frame_sub_type)
        {
            /* 保存QoS Info */
            mac_vap_set_wmm_params_update_count_etc(&pst_hmac_sta->st_vap_base_info, uc_param_set_cnt);
        }

        if (puc_ie[HMAC_WMM_QOS_PARAMS_HDR_LEN] & BIT7)
        {
            mac_user_set_apsd_etc(&(pst_hmac_user->st_user_base_info), OAL_TRUE);
        }
        else
        {
            mac_user_set_apsd_etc(&(pst_hmac_user->st_user_base_info), OAL_FALSE);
        }

        us_msg_offset = (HMAC_WMM_QOSINFO_AND_RESV_LEN + HMAC_WMM_QOS_PARAMS_HDR_LEN);

        /* wmm ie中不携带edca参数 直接返回 */
        if(MAC_OUISUBTYPE_WMM_PARAM != uc_edca_param_set)
        {
            return;
        }

        /* 针对每一个AC，更新EDCA参数 */
        for (uc_ac_num_loop = 0; uc_ac_num_loop < WLAN_WME_AC_BUTT; uc_ac_num_loop++)
        {
            hmac_sta_up_update_edca_params_mib(pst_hmac_sta, &puc_ie[us_msg_offset]);
#ifdef _PRE_WLAN_FEATURE_WMMAC
            if (OAL_TRUE == mac_mib_get_QAPEDCATableMandatory(&(pst_hmac_sta->st_vap_base_info), uc_ac_num_loop))
            {
                en_wmmac_auth_flag = pst_hmac_sta->en_wmmac_auth_flag &&
                    WLAN_FC0_SUBTYPE_REASSOC_RSP == uc_frame_sub_type &&
                    MAC_TS_SUCCESS == pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num_loop].en_ts_status;
                if(en_wmmac_auth_flag)
                {
                        return;
                }
                pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num_loop].en_ts_status = MAC_TS_INIT;
                pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num_loop].uc_tsid      = 0xFF;
            }
            else
            {
                pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num_loop].en_ts_status = MAC_TS_NONE;
                pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num_loop].uc_tsid      = 0xFF;
            }

            OAM_INFO_LOG2(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_up_update_edca_params_etc::ac num[%d], ts status[%d].}",
                             uc_ac_num_loop, pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num_loop].en_ts_status);
#endif  // _PRE_WLAN_FEATURE_WMMAC
            us_msg_offset += HMAC_WMM_AC_PARAMS_RECORD_LEN;
        }

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
        hcc_host_update_vi_flowctl_param_etc(mac_mib_get_QAPEDCATableCWmin(&pst_hmac_sta->st_vap_base_info, WLAN_WME_AC_BE),
            mac_mib_get_QAPEDCATableCWmin(&pst_hmac_sta->st_vap_base_info, WLAN_WME_AC_VI));
#endif

        /* 更新EDCA相关的MAC寄存器 */
        hmac_sta_up_update_edca_params_machw_etc(pst_hmac_sta, MAC_WMM_SET_PARAM_TYPE_UPDATE_EDCA);

        return;
    }

    puc_ie = mac_find_ie_etc(MAC_EID_HT_CAP, puc_payload, us_msg_len);
    if (OAL_PTR_NULL != puc_ie)
    {
       /* 再查找HT CAP能力第2字节BIT 5 short GI for 20M 能力位 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
       if ((puc_ie[1] >= 2)&&(puc_ie[2] & BIT5))
#endif
       {
           mac_vap_init_wme_param_etc(&pst_hmac_sta->st_vap_base_info);
           pst_mac_device->en_wmm = OAL_TRUE;
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
           hcc_host_update_vi_flowctl_param_etc(mac_mib_get_QAPEDCATableCWmin(&pst_hmac_sta->st_vap_base_info, WLAN_WME_AC_BE),
           mac_mib_get_QAPEDCATableCWmin(&pst_hmac_sta->st_vap_base_info, WLAN_WME_AC_VI));
#endif
           /* 更新EDCA相关的MAC寄存器 */
           hmac_sta_up_update_edca_params_machw_etc(pst_hmac_sta, MAC_WMM_SET_PARAM_TYPE_UPDATE_EDCA);

           return;
        }
    }

    if(WLAN_FC0_SUBTYPE_ASSOC_RSP == uc_frame_sub_type)
    {
        /* 当与STA关联的AP不是QoS的，STA会去使能EDCA寄存器，并默认利用VO级别发送数据 */
        ul_ret = hmac_sta_up_update_edca_params_machw_etc(pst_hmac_sta, MAC_WMM_SET_PARAM_TYPE_DEFAULT);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                            "{hmac_sta_up_update_edca_params_etc::hmac_sta_up_update_edca_params_machw_etc failed[%d].}", ul_ret);
        }

        pst_mac_device->en_wmm = OAL_FALSE;
    }
}

#ifdef _PRE_WLAN_FEATURE_TXOPPS

oal_uint32  hmac_sta_set_txopps_partial_aid(mac_vap_stru *pst_mac_vap)
{
    oal_uint16              us_temp_aid;
    oal_uint8               uc_temp_bssid;
    oal_uint32               ul_ret;
    mac_cfg_txop_sta_stru    st_txop_info = {0};

    /* 此处需要注意:按照协议规定(802.11ac-2013.pdf,9.17a)，ap分配给sta的aid，不可以
       使计算出来的partial aid为0，后续如果ap支持的最大关联用户数目超过512，必须要
       对aid做这个合法性检查!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    */

    if (WLAN_VHT_MODE != pst_mac_vap->en_protocol && WLAN_VHT_ONLY_MODE != pst_mac_vap->en_protocol)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_TXOP,"{hmac_sta_set_txopps_partial_aid::VAP VHT unsupport}.");
        return OAL_SUCC;
    }

    /* 计算partial aid */
    if (OAL_TRUE != mac_vap_get_txopps(pst_mac_vap))
    {
        st_txop_info.us_partial_aid = 0;
    }
    else
    {
        us_temp_aid   = pst_mac_vap->us_sta_aid & 0x1FF;
        uc_temp_bssid = (pst_mac_vap->auc_bssid[5] & 0x0F) ^ ((pst_mac_vap->auc_bssid[5] & 0xF0) >> 4);
        st_txop_info.us_partial_aid = (us_temp_aid + (uc_temp_bssid << 5) ) & ((1 << 9) - 1);
    }

    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_STA_TXOP_AID, OAL_SIZEOF(mac_cfg_txop_sta_stru), (oal_uint8 *)&st_txop_info);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TXOP, "{hmac_sta_set_txopps_partial_aid::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}
#endif


oal_void hmac_sta_update_mac_user_info_etc(hmac_user_stru *pst_hmac_user_ap,oal_uint16 us_user_idx)
{
    mac_vap_stru                   *pst_mac_vap;
    mac_user_stru                  *pst_mac_user_ap;
    oal_uint32                      ul_ret;

    if (OAL_PTR_NULL == pst_hmac_user_ap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_sta_update_mac_user_info_etc::param null.}");
        return;
    }

    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_hmac_user_ap->st_user_base_info.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap)) {
        OAM_ERROR_LOG1(0, OAM_SF_RX, "{hmac_sta_update_mac_user_info_etc::get mac_vap [vap_id:%d] null.}", pst_hmac_user_ap->st_user_base_info.uc_vap_id);
        return;
    }

    pst_mac_user_ap = &(pst_hmac_user_ap->st_user_base_info);


    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                       "{hmac_sta_update_mac_user_info_etc::us_user_idx:%d,en_avail_bandwidth:%d,en_cur_bandwidth:%d}",
                         us_user_idx,
                         pst_mac_user_ap->en_avail_bandwidth,
                         pst_mac_user_ap->en_cur_bandwidth);

    ul_ret = hmac_config_user_info_syn_etc(pst_mac_vap, pst_mac_user_ap);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                       "{hmac_sta_update_mac_user_info_etc::hmac_config_user_info_syn_etc failed[%d].}", ul_ret);
    }

    ul_ret = hmac_config_user_rate_info_syn_etc(pst_mac_vap, pst_mac_user_ap);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RX,
                       "{hmac_sta_wait_asoc_rx_etc::hmac_syn_rate_info failed[%d].}", ul_ret);
    }
    return;
}



oal_uint8 *hmac_sta_find_ie_in_probe_rsp_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_eid, oal_uint16 *pus_index)
{
    mac_bss_dscr_stru                  *pst_bss_dscr = OAL_PTR_NULL;
    oal_uint8                          *puc_ie;
    oal_uint8                          *puc_payload;
    oal_uint8                           us_offset;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_SCAN, "{find ie fail, pst_mac_vap is null.}");
        return OAL_PTR_NULL;
    }

    pst_bss_dscr = (mac_bss_dscr_stru *)hmac_scan_get_scanned_bss_by_bssid(pst_mac_vap, pst_mac_vap->auc_bssid);

    if (OAL_PTR_NULL == pst_bss_dscr)
    {
       OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                        "hmac_sta_find_ie_in_probe_rsp_etc::find the bss failed by bssid:%02X:XX:XX:%02X:%02X:%02X",
                        pst_mac_vap->auc_bssid[0],
                        pst_mac_vap->auc_bssid[3],
                        pst_mac_vap->auc_bssid[4],
                        pst_mac_vap->auc_bssid[5]);

       return OAL_PTR_NULL;
    }


    /* 以IE开头的payload，返回供调用者使用 */
    us_offset = MAC_80211_FRAME_LEN + MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    /*lint -e416*/
    puc_payload = (oal_uint8*)(pst_bss_dscr->auc_mgmt_buff + us_offset);
    /*lint +e416*/
    if (pst_bss_dscr->ul_mgmt_len < us_offset)
    {
        return OAL_PTR_NULL;
    }

    puc_ie = mac_find_ie_etc(uc_eid, puc_payload, (oal_int32)(pst_bss_dscr->ul_mgmt_len - us_offset));
    if (OAL_PTR_NULL == puc_ie)
    {
        return OAL_PTR_NULL;
    }

    /* IE长度初步校验 */
    if (0 == *(puc_ie + 1))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{IE[%d] len in probe rsp is 0, find ie fail.}", uc_eid);
        return OAL_PTR_NULL;
    }

    *pus_index = (oal_uint16)(puc_ie - puc_payload);

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{found ie[%d] in probe rsp.}", uc_eid);

    return puc_payload;
}


oal_void hmac_sta_check_ht_cap_ie(mac_vap_stru    *pst_mac_sta,
                                            oal_uint8       *puc_payload,
                                            mac_user_stru   *pst_mac_user_ap,
                                            oal_uint16      *pus_amsdu_maxsize,
                                            oal_uint16      us_payload_len)
{
    oal_uint8                          *puc_ie;
    oal_uint8                          *puc_payload_for_ht_cap_chk = OAL_PTR_NULL;
    oal_uint16                          us_ht_cap_index;

    if ((OAL_PTR_NULL == pst_mac_sta) || (OAL_PTR_NULL == puc_payload) || (OAL_PTR_NULL == pst_mac_user_ap))
    {
        return;
    }

    puc_ie = mac_find_ie_etc(MAC_EID_HT_CAP, puc_payload, us_payload_len);
    if (OAL_PTR_NULL == puc_ie || puc_ie[1] < MAC_HT_CAP_LEN)
    {
        puc_payload_for_ht_cap_chk = hmac_sta_find_ie_in_probe_rsp_etc(pst_mac_sta, MAC_EID_HT_CAP, &us_ht_cap_index);
        if (OAL_PTR_NULL == puc_payload_for_ht_cap_chk)
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_sta_check_ht_cap_ie::puc_payload_for_ht_cap_chk is null.}");
            return;
        }

        /*lint -e413*/
        if (puc_payload_for_ht_cap_chk[us_ht_cap_index + 1] < MAC_HT_CAP_LEN)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_sta_check_ht_cap_ie::invalid ht cap len[%d].}", puc_payload_for_ht_cap_chk[us_ht_cap_index + 1]);
            return;
        }
        /*lint +e413*/
        puc_ie = puc_payload_for_ht_cap_chk + us_ht_cap_index;      /* 赋值HT CAP IE */
    }
    else
    {
        if (puc_ie < puc_payload)
        {
            return;
        }
        us_ht_cap_index = (oal_uint16)(puc_ie - puc_payload);
        puc_payload_for_ht_cap_chk = puc_payload;
    }

    if ((WLAN_BAND_2G == pst_mac_sta->st_channel.en_band) && IS_INVALID_HT_RATE_HP(puc_ie))
    {
        OAM_WARNING_LOG0(pst_mac_sta->uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_update_ht_cap:: invalid mcs set, disable HT.}");
        mac_user_set_ht_capable_etc(pst_mac_user_ap, OAL_FALSE);
        return;
    }

    /* 支持HT, 默认初始化 */
    /* hmac_amsdu_init_user_etc(pst_hmac_user_ap); */

    /* 根据协议值设置特性，必须在hmac_amsdu_init_user后面调用 */
    mac_ie_proc_ht_sta_etc(pst_mac_sta, puc_payload_for_ht_cap_chk, us_ht_cap_index, pst_mac_user_ap, pus_amsdu_maxsize);

}


oal_void hmac_sta_check_ext_cap_ie(mac_vap_stru    *pst_mac_sta,
                                    mac_user_stru   *pst_mac_user_ap,
                                    oal_uint8       *puc_payload,
                                    oal_uint16       us_rx_len)
{
    oal_uint8       *puc_ie;
    oal_uint8       *puc_payload_proc = OAL_PTR_NULL;
    oal_uint16       us_index;

    puc_ie = mac_find_ie_etc(MAC_EID_EXT_CAPS, puc_payload, us_rx_len);
    if (OAL_PTR_NULL == puc_ie || puc_ie[1] < MAC_MIN_XCAPS_LEN)
    {
        puc_payload_proc = hmac_sta_find_ie_in_probe_rsp_etc(pst_mac_sta, MAC_EID_EXT_CAPS, &us_index);
        if (OAL_PTR_NULL == puc_payload_proc)
        {
            return;
        }

        /*lint -e413*/
        if (puc_payload_proc[us_index + 1] < MAC_MIN_XCAPS_LEN)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_sta_check_ext_cap_ie::invalid ext cap len[%d].}", puc_payload_proc[us_index + 1]);
            return;
        }
        /*lint +e413*/
    }
    else
    {
        puc_payload_proc = puc_payload;
        if (puc_ie < puc_payload)
        {
            return;
        }

        us_index = (oal_uint16)(puc_ie - puc_payload);
    }

    /* 处理 Extended Capabilities IE */
    /*lint -e613*/
    mac_ie_proc_ext_cap_ie_etc(pst_mac_user_ap, &puc_payload_proc[us_index]);
    /*lint +e613*/
}


oal_uint32 hmac_sta_check_ht_opern_ie(mac_vap_stru    *pst_mac_sta,
                                    mac_user_stru   *pst_mac_user_ap,
                                    oal_uint8       *puc_payload,
                                    oal_uint16       us_rx_len)
{
    oal_uint8       *puc_ie;
    oal_uint8       *puc_payload_proc = OAL_PTR_NULL;
    oal_uint16       us_index;
    oal_uint32       ul_change = MAC_NO_CHANGE;

    puc_ie = mac_find_ie_etc(MAC_EID_HT_OPERATION, puc_payload, us_rx_len);
    if (OAL_PTR_NULL == puc_ie || puc_ie[1] < MAC_HT_OPERN_LEN)
    {
        puc_payload_proc = hmac_sta_find_ie_in_probe_rsp_etc(pst_mac_sta, MAC_EID_HT_OPERATION, &us_index);
        if (OAL_PTR_NULL == puc_payload_proc)
        {
            return ul_change;
        }

        /*lint -e413*/
        if (puc_payload_proc[us_index + 1] < MAC_HT_OPERN_LEN)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_sta_check_ht_opern_ie::invalid ht cap len[%d].}", puc_payload_proc[us_index + 1]);
            return ul_change;
        }
        /*lint +e413*/
    }
    else
    {
        puc_payload_proc = puc_payload;
        if (puc_ie < puc_payload)
        {
            return ul_change;
        }

        us_index = (oal_uint16)(puc_ie - puc_payload);
    }

    ul_change |= mac_proc_ht_opern_ie_etc(pst_mac_sta, &puc_payload_proc[us_index], pst_mac_user_ap);


    return ul_change;
}


oal_uint32 hmac_ie_check_ht_sta(mac_vap_stru    *pst_mac_sta,
                               oal_uint8       *puc_payload,
                               oal_uint16       us_rx_len,
                               mac_user_stru   *pst_mac_user_ap,
                               oal_uint16      *pus_amsdu_maxsize)
{
    oal_uint32              ul_change = MAC_NO_CHANGE;

    if ((OAL_PTR_NULL == pst_mac_sta) || (OAL_PTR_NULL == puc_payload) || (OAL_PTR_NULL == pst_mac_user_ap))
    {
        return ul_change;
    }

    /* 初始化HT cap为FALSE，入网时会把本地能力跟随AP能力 */
    mac_user_set_ht_capable_etc(pst_mac_user_ap, OAL_FALSE);

     /* 至少支持11n才进行后续的处理 */
    if (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_mac_sta))
    {
        return ul_change;
    }

    hmac_sta_check_ht_cap_ie(pst_mac_sta, puc_payload, pst_mac_user_ap, pus_amsdu_maxsize, us_rx_len);
    /* sta处理AP的 Extended Capability */
    hmac_sta_check_ext_cap_ie(pst_mac_sta, pst_mac_user_ap, puc_payload, us_rx_len);

    ul_change = hmac_sta_check_ht_opern_ie(pst_mac_sta, pst_mac_user_ap, puc_payload, us_rx_len);

    return ul_change;
}

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN

oal_uint32  hmac_sta_up_update_rrm_capability(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 *puc_payload, oal_uint32 us_rx_len)
{
    mac_cap_info_stru                   *pst_user_cap_info;
    oal_uint8                           *puc_ie;
    oal_uint8                           uc_len;
    mac_rrm_enabled_cap_ie_stru         *pst_rrm_enabled_cap_ie;
    mac_user_stru                       *pst_mac_user = &(pst_hmac_user->st_user_base_info);

    /*发送assoc req前从AP获取的能力*/
    pst_user_cap_info = (mac_cap_info_stru *)(&(pst_mac_vap->us_assoc_user_cap_info));

    if(OAL_FALSE == pst_user_cap_info->bit_radio_measurement)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                 "{hmac_sta_up_update_rrm_capability::user not support MAC_CAP_RADIO_MEAS");
        return OAL_SUCC;
    }

    puc_ie = mac_find_ie_etc(MAC_EID_RRM, puc_payload, (oal_int32)us_rx_len);

    if ( OAL_PTR_NULL == puc_ie)
    {
        OAL_IO_PRINT("hmac_sta_up_update_rrm_capability::user support 11k but not fill rrm_enabled_cap_ie.");
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_sta_up_update_rrm_capability::user support 11k but not fill rrm_enabled_cap_ie.}");
        return OAL_FAIL;
    }

    pst_mac_user->st_cap_info.bit_11k_enable = OAL_TRUE;

    /*user rrm capability list*/
    uc_len = puc_ie[1];
    oal_memcopy(&(pst_mac_user->st_rrm_enabled_cap), puc_ie + MAC_IE_HDR_LEN, uc_len);

    pst_rrm_enabled_cap_ie = (mac_rrm_enabled_cap_ie_stru *)(puc_ie + MAC_IE_HDR_LEN);
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                     "{hmac_sta_up_update_rrm_capability::neighbor[%d], beacon[active:%d][passive:%d][table:%d].}",
                     pst_rrm_enabled_cap_ie->bit_neighbor_rpt_cap,
                     pst_rrm_enabled_cap_ie->bit_bcn_active_cap,
                     pst_rrm_enabled_cap_ie->bit_bcn_passive_cap,
                     pst_rrm_enabled_cap_ie->bit_bcn_table_cap);
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                 "{hmac_sta_up_update_rrm_capability::load[%d].}",
                 pst_rrm_enabled_cap_ie->bit_chn_load_cap);

    hmac_11k_init_user(pst_hmac_user);

    return OAL_SUCC;
}
#endif


oal_uint32 hmac_process_assoc_rsp_etc(hmac_vap_stru *pst_hmac_sta,
                                  hmac_user_stru *pst_hmac_user,
                                  oal_uint8 *puc_mac_hdr,
                                  oal_uint16 us_hdr_len,
                                  oal_uint8 *puc_payload,
                                  oal_uint16 us_msg_len)
{
    oal_uint32                      ul_rslt;
    oal_uint16                      us_aid;
    oal_uint8                      *puc_tmp_ie;
    oal_uint32                      ul_ret;
    mac_vap_stru                   *pst_mac_vap;
    mac_user_stru                  *pst_mac_user;
    oal_uint32                      ul_change = MAC_NO_CHANGE;
    wlan_channel_bandwidth_enum_uint8 en_sta_new_bandwidth;
    oal_uint8                       uc_avail_mode;

    if (OAL_PTR_NULL == pst_hmac_sta)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC,"{hmac_process_assoc_rsp_etc::pst_hmac_sta null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap     = &(pst_hmac_sta->st_vap_base_info);
    pst_mac_user    = &(pst_hmac_user->st_user_base_info);

    /* 更新关联ID */
    us_aid = mac_get_asoc_id(puc_payload);
    if ((us_aid > 0) && (us_aid <= 2007))
    {
        mac_vap_set_aid_etc(pst_mac_vap, us_aid);
    }
    else
    {
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                        "{hmac_process_assoc_rsp_etc::invalid us_sta_aid[%d].}", us_aid);
    }

    puc_payload += (MAC_CAP_INFO_LEN + MAC_STATUS_CODE_LEN + MAC_AID_LEN);
    us_msg_len  -= (MAC_CAP_INFO_LEN + MAC_STATUS_CODE_LEN + MAC_AID_LEN);

    /* 初始化安全端口过滤参数 */
    ul_rslt = hmac_init_user_security_port_etc(&(pst_hmac_sta->st_vap_base_info), &(pst_hmac_user->st_user_base_info));
    if (OAL_SUCC != ul_rslt)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::hmac_init_user_security_port_etc failed[%d].}", ul_rslt);
    }

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    /* STA模式下的pmf能力来源于WPA_supplicant，只有启动pmf和不启动pmf两种类型 */
    mac_user_set_pmf_active_etc(&pst_hmac_user->st_user_base_info, pst_mac_vap->en_user_pmf_cap);
    OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::set user pmf[%d].}", pst_mac_vap->en_user_pmf_cap);
#endif /* #if(_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT) */

    /* sta更新自身的edca parameters, assoc rsp帧是管理帧 */
    hmac_sta_up_update_edca_params_etc(puc_payload, us_msg_len, pst_hmac_sta, mac_get_frame_sub_type(puc_mac_hdr), pst_hmac_user);

#ifdef _PRE_WLAN_FEATURE_11AX
    hmac_sta_up_update_he_edca_params(puc_payload, us_msg_len, pst_hmac_sta, mac_get_frame_sub_type(puc_mac_hdr), pst_hmac_user);
#endif

    /* 更新关联用户的 QoS protocol table */
    hmac_mgmt_update_assoc_user_qos_table_etc(puc_payload, us_msg_len, pst_hmac_user);

    /* 更新关联用户的legacy速率集合 */
    hmac_user_init_rates_etc(pst_hmac_user);
    ul_ret = hmac_ie_proc_assoc_user_legacy_rate(puc_payload, us_msg_len, pst_hmac_user);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_process_assoc_rsp_etc::rates mismatch ret[%d].}", ul_ret);
    }

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    /* 更新对应STA的RRM能力 */
    hmac_sta_up_update_rrm_capability(pst_mac_vap, pst_hmac_user, puc_payload, us_msg_len);
#endif

#ifdef _PRE_WLAN_FEATURE_TXBF
    /* 更新11n txbf能力 */
    puc_tmp_ie = mac_find_vendor_ie_etc(MAC_HUAWEI_VENDER_IE, MAC_EID_11NTXBF, puc_payload, us_msg_len);
    hmac_mgmt_update_11ntxbf_cap_etc(puc_tmp_ie, pst_hmac_user);
#endif

#ifdef _PRE_WLAN_NARROW_BAND
    //mac_get_nb_ie(pst_mac_vap,  puc_payload, us_msg_len);
#endif

    /* 更新11ac VHT capabilities ie */
    oal_memset(&(pst_hmac_user->st_user_base_info.st_vht_hdl), 0, OAL_SIZEOF(mac_vht_hdl_stru));
    puc_tmp_ie = mac_find_ie_etc(MAC_EID_VHT_CAP, puc_payload, us_msg_len);
    if (OAL_PTR_NULL != puc_tmp_ie)
    {
        hmac_proc_vht_cap_ie_etc(pst_mac_vap, pst_hmac_user, puc_tmp_ie);
#ifdef _PRE_WLAN_FEATURE_1024QAM
        puc_tmp_ie = mac_find_vendor_ie_etc(MAC_HUAWEI_VENDER_IE, MAC_HISI_1024QAM_IE, puc_payload, us_msg_len);
       if (OAL_PTR_NULL != puc_tmp_ie)
       {
          pst_hmac_user->st_user_base_info.st_cap_info.bit_1024qam_cap = OAL_TRUE;
       }
#endif
    }

#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
    /* vap开启4地址能力并调用check接口校验用户是否支持4地址 是则将用户的4地址传输能力位置1 */
    if ( pst_hmac_sta->st_wds_table.en_wds_vap_mode == WDS_MODE_REPEATER_STA )
    {
        if (hmac_vmsta_check_user_a4_support(puc_mac_hdr, us_hdr_len+us_msg_len))
        {
            pst_hmac_user->uc_is_wds = OAL_TRUE;
            OAM_WARNING_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_ap_up_rx_asoc_req::user surpport 4 address.}");
        }
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT_DEBUG
    /* 打桩用于调试，只要检测到相应的hilinkie即认为用户支持4地址 */
        else
        {
            if (OAL_PTR_NULL != mac_find_vendor_ie_etc(MAC_WLAN_OUI_HUAWEI, MAC_WLAN_OUI_TYPE_HAUWEI_4ADDR, puc_payload, us_msg_len))
            {
                pst_hmac_user->uc_is_wds = OAL_TRUE;
                OAM_WARNING_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_ap_up_rx_asoc_req::user surpport 4 address.}");
            }
        }
#endif
    }
#endif  //_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA

    /* 更新11ac VHT operation ie */
    puc_tmp_ie = mac_find_ie_etc(MAC_EID_VHT_OPERN, puc_payload, us_msg_len);
    if (OAL_PTR_NULL != puc_tmp_ie)
    {
        ul_change |= hmac_update_vht_opern_ie_sta(pst_mac_vap, pst_hmac_user, puc_tmp_ie);
    }

    /* 更新 HT 参数  以及 EXTEND CAPABILITY */
    oal_memset(&(pst_hmac_user->st_user_base_info.st_ht_hdl), 0, OAL_SIZEOF(mac_user_ht_hdl_stru));
    ul_change |= hmac_ie_check_ht_sta(&pst_hmac_sta->st_vap_base_info, puc_payload, us_msg_len, &pst_hmac_user->st_user_base_info, &pst_hmac_user->us_amsdu_maxsize);

    if (OAL_TRUE == hmac_user_ht_support(pst_hmac_user))
    {
        /*更新 11ax HE Capabilities ie*/
#ifdef _PRE_WLAN_FEATURE_11AX
        oal_memset(&(pst_hmac_user->st_user_base_info.st_he_hdl), 0, OAL_SIZEOF(mac_he_hdl_stru));
        puc_tmp_ie = mac_find_ie_ext_ie(MAC_EID_HE,MAC_EID_EXT_HE_CAP,puc_payload, us_msg_len);
        if (OAL_PTR_NULL != puc_tmp_ie)
        {
            hmac_proc_he_cap_ie(pst_mac_vap, pst_hmac_user, puc_tmp_ie);
        }

        puc_tmp_ie = mac_find_ie_ext_ie(MAC_EID_HE, MAC_EID_EXT_HE_OPERATION, puc_payload, us_msg_len);
        if(OAL_PTR_NULL != puc_tmp_ie)
        {
            ul_change |= hmac_update_he_opern_ie_sta(pst_mac_vap, pst_hmac_user, puc_tmp_ie);
        }

        /*解析BSS Coloe change announcement IE  */
        puc_tmp_ie = mac_find_ie_ext_ie(MAC_EID_HE,MAC_EID_EXT_HE_BSS_COLOR_CHANGE_ANNOUNCEMENT,puc_payload, us_msg_len);
        if (OAL_PTR_NULL != puc_tmp_ie)
        {
            hmac_proc_he_bss_color_change_announcement_ie(pst_mac_vap,pst_hmac_user, puc_tmp_ie);
        }
#endif

        /* 评估是否需要进行带宽切换 */
        //hmac_chan_reval_bandwidth_sta_etc(pst_mac_vap, ul_change);

        /* 根据BRCM VENDOR OUI 适配2G 11AC */
        if (pst_hmac_user->st_user_base_info.st_vht_hdl.en_vht_capable == OAL_FALSE)
        {
            oal_uint8 *puc_vendor_vht_ie;
            oal_uint32 ul_vendor_vht_ie_offset = MAC_WLAN_OUI_VENDOR_VHT_HEADER + MAC_IE_HDR_LEN;
            puc_vendor_vht_ie = mac_find_vendor_ie_etc(MAC_WLAN_OUI_BROADCOM_EPIGRAM, MAC_WLAN_OUI_VENDOR_VHT_TYPE, puc_payload, us_msg_len);

            if ((OAL_PTR_NULL != puc_vendor_vht_ie) && (puc_vendor_vht_ie[1] >= ul_vendor_vht_ie_offset))
            {
                OAM_WARNING_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                    "{hmac_process_assoc_rsp_etc::find broadcom/epigram vendor ie, enable hidden bit_11ac2g}");

                /* 进入此函数代表user支持2G 11ac */
                puc_tmp_ie = mac_find_ie_etc(MAC_EID_VHT_CAP, puc_vendor_vht_ie + ul_vendor_vht_ie_offset, (oal_int32)(puc_vendor_vht_ie[1] - MAC_WLAN_OUI_VENDOR_VHT_HEADER));
                if (OAL_PTR_NULL != puc_tmp_ie)
                {
                    hmac_proc_vht_cap_ie_etc(pst_mac_vap, pst_hmac_user, puc_tmp_ie);
                }

                /* 更新11ac VHT operation ie */
                puc_tmp_ie = mac_find_ie_etc(MAC_EID_VHT_OPERN, puc_vendor_vht_ie + ul_vendor_vht_ie_offset, (oal_int32)(puc_vendor_vht_ie[1] - MAC_WLAN_OUI_VENDOR_VHT_HEADER));
                if (OAL_PTR_NULL != puc_tmp_ie)
                {
                    ul_change |= hmac_update_vht_opern_ie_sta(pst_mac_vap, pst_hmac_user, puc_tmp_ie);
                }
            }
        }
    }
#ifdef _PRE_WLAN_FEATURE_HISTREAM
    /* 更新histream能力 */
    pst_hmac_user->st_user_base_info.st_cap_info.bit_histream_cap = OAL_FALSE;
    puc_tmp_ie = mac_find_vendor_ie_etc(MAC_HUAWEI_VENDER_IE, MAC_HISI_HISTREAM_IE, puc_payload, us_msg_len);
    if (OAL_PTR_NULL != puc_tmp_ie)
    {
        pst_hmac_user->st_user_base_info.st_cap_info.bit_histream_cap = OAL_TRUE;
    }
#endif //_PRE_WLAN_FEATURE_HISTREAM

    /* 获取用户的协议模式 */
    hmac_set_user_protocol_mode_etc(pst_mac_vap, pst_hmac_user);

    uc_avail_mode = g_auc_avail_protocol_mode_etc[pst_mac_vap->en_protocol][pst_hmac_user->st_user_base_info.en_protocol_mode];
#ifdef _PRE_WLAN_FEATURE_11AC2G
    if((WLAN_HT_MODE == pst_mac_vap->en_protocol) && (WLAN_VHT_MODE == pst_hmac_user->st_user_base_info.en_protocol_mode)
        && (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_11ac2g)
        && (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band))
    {
        uc_avail_mode = WLAN_VHT_MODE;
    }
#endif

    if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
    {
        pst_hmac_sta->bit_rx_ampduplusamsdu_active = OAL_FALSE;
    }
    else
    {
        pst_hmac_sta->bit_rx_ampduplusamsdu_active = g_uc_host_rx_ampdu_amsdu;
    }

    /* 获取用户与VAP协议模式交集 */
    mac_user_set_avail_protocol_mode_etc(&pst_hmac_user->st_user_base_info, uc_avail_mode);
    mac_user_set_cur_protocol_mode_etc(&pst_hmac_user->st_user_base_info, pst_hmac_user->st_user_base_info.en_avail_protocol_mode);

    /* 获取用户和VAP 可支持的11a/b/g 速率交集 */
    hmac_vap_set_user_avail_rates_etc(&(pst_hmac_sta->st_vap_base_info), pst_hmac_user);

    /* 获取用户和VAP带宽，并判断是否有带宽变化需要通知硬件切带宽 */
    en_sta_new_bandwidth = mac_vap_get_ap_usr_opern_bandwidth(pst_mac_vap, &(pst_hmac_user->st_user_base_info));
    ul_change |= mac_vap_set_bw_check(pst_mac_vap, en_sta_new_bandwidth);

    /* 同步用户的带宽能力 */
    mac_user_set_bandwidth_info_etc(pst_mac_user, mac_vap_bw_mode_to_bw(en_sta_new_bandwidth), mac_vap_bw_mode_to_bw(en_sta_new_bandwidth));

    /* 获取用户与VAP空间流交集 */
    ul_ret = hmac_user_set_avail_num_space_stream_etc(pst_mac_user, pst_mac_vap->en_vap_rx_nss);
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,"{hmac_process_assoc_rsp_etc:ap max nss is [%d]}",pst_hmac_user->st_user_base_info.en_user_num_spatial_stream);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::mac_user_set_avail_num_space_stream failed[%d].}", ul_ret);
    }

#ifdef _PRE_WLAN_FEATURE_SMPS
    /* 根据smps更新空间流能力 */
    if(!IS_VAP_SINGLE_NSS(pst_mac_vap) && !IS_USER_SINGLE_NSS(pst_mac_user))
    {
        hmac_smps_update_user_status(pst_mac_vap, pst_mac_user);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    /* 处理Operating Mode Notification 信息元素 */
    ul_ret = hmac_check_opmode_notify_etc(pst_hmac_sta, puc_mac_hdr, puc_payload, us_msg_len, pst_hmac_user);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::hmac_check_opmode_notify_etc failed[%d].}", ul_ret);
    }
#endif

    mac_user_set_asoc_state_etc(&pst_hmac_user->st_user_base_info, MAC_USER_STATE_ASSOC);

    /* 同步空间流信息 */
    ul_ret = hmac_config_user_num_spatial_stream_cap_syn(pst_mac_vap, pst_mac_user);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::hmac_config_user_num_spatial_stream_cap_syn failed[%d].}", ul_ret);
    }

    /* dmac offload架构下，同步STA USR信息到dmac */
    ul_ret = hmac_config_user_cap_syn_etc(&(pst_hmac_sta->st_vap_base_info), &pst_hmac_user->st_user_base_info);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::hmac_config_usr_cap_syn failed[%d].}", ul_ret);
    }

    ul_ret = hmac_config_user_info_syn_etc(&(pst_hmac_sta->st_vap_base_info), &pst_hmac_user->st_user_base_info);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::hmac_syn_vap_state failed[%d].}", ul_ret);
    }

    ul_ret = hmac_config_user_rate_info_syn_etc(&(pst_hmac_sta->st_vap_base_info), &pst_hmac_user->st_user_base_info);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::hmac_syn_rate_info failed[%d].}", ul_ret);
    }

    /* dmac offload架构下，同步STA USR信息到dmac */
    ul_ret = hmac_config_sta_vap_info_syn_etc(&(pst_hmac_sta->st_vap_base_info));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_process_assoc_rsp_etc::hmac_syn_vap_state failed[%d].}", ul_ret);
    }

#ifdef _PRE_WLAN_FEATURE_TXOPPS
    /* sta计算自身的partial aid，写入到mac寄存器 */
    hmac_sta_set_txopps_partial_aid(pst_mac_vap);
#endif

    if (MAC_BW_CHANGE & ul_change)
    {
        /* 获取用户与VAP带宽能力交集,通知硬件切带宽 */
        hmac_chan_sync_etc(pst_mac_vap,
                        pst_mac_vap->st_channel.uc_chan_number,
                        pst_mac_vap->st_channel.en_bandwidth,
                        OAL_TRUE);
        OAM_WARNING_LOG4(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_process_assoc_rsp_etc:change BW. change[0x%x], vap band[%d] chnl[%d], bandwidth[%d].}",
                         ul_change,
                         pst_mac_vap->st_channel.en_band,
                         pst_mac_vap->st_channel.uc_chan_number,
                         MAC_VAP_GET_CAP_BW(pst_mac_vap));
        OAM_WARNING_LOG4(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_process_assoc_rsp_etc:mac usr id[%d] bw cap[%d]avil[%d]cur[%d].}",
                         pst_hmac_user->st_user_base_info.us_assoc_id,
                         pst_hmac_user->st_user_base_info.en_bandwidth_cap,
                         pst_hmac_user->st_user_base_info.en_avail_bandwidth,
                         pst_hmac_user->st_user_base_info.en_cur_bandwidth);
    }

    return OAL_SUCC;
}


oal_uint32 hmac_sta_sync_vap(hmac_vap_stru *pst_hmac_vap, mac_channel_stru *pst_channel, wlan_protocol_enum_uint8 en_protocol)
{
    mac_device_stru                     *pst_mac_device;
    mac_vap_stru                        *pst_mac_vap;
    oal_uint32                           ul_ret;
    mac_cfg_mode_param_stru              st_prot_param;
    mac_channel_stru                    *pst_channel_dst = OAL_PTR_NULL;

    if (!pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_sync_vap::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    /* 获取mac device指针 */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_INIT);
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_sync_vap::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_channel_dst = pst_channel ? pst_channel : &pst_mac_vap->st_channel;

    st_prot_param.en_channel_idx = pst_channel_dst->uc_chan_number;
    st_prot_param.en_bandwidth   = pst_channel_dst->en_bandwidth;
    st_prot_param.en_band        = pst_channel_dst->en_band;

    // use input protocol if exists
    st_prot_param.en_protocol = (en_protocol >= WLAN_PROTOCOL_BUTT) ? pst_mac_vap->en_protocol : en_protocol;

    ul_ret = mac_is_channel_num_valid_etc(st_prot_param.en_band, st_prot_param.en_channel_idx);
    if (OAL_SUCC != ul_ret)
    {
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_INIT);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_sta_sync_vap::mac_is_channel_num_valid_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                  "{hmac_sta_sync_vap::AP: Starting network in Channel: %d, bandwidth: %d protocol: %d.}",
                  st_prot_param.en_channel_idx, st_prot_param.en_bandwidth, st_prot_param.en_protocol);

#ifdef _PRE_WLAN_FEATURE_DBAC
    /* 同时更改多个VAP的信道，此时需要强制清除记录 */
    /* 若启动了DBAC，则按照原始流程进行 */
    if (!mac_is_dbac_enabled(pst_mac_device))
#endif
    {
        pst_mac_device->uc_max_channel = 0;
        pst_mac_device->en_max_bandwidth = WLAN_BAND_WIDTH_BUTT;
    }

    // force channel setting is required
    pst_mac_vap->st_channel = *pst_channel_dst;

    ul_ret = hmac_config_set_freq_etc(pst_mac_vap, OAL_SIZEOF(oal_uint8), &st_prot_param.en_channel_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                        "{hmac_sta_sync_vap::hmac_config_set_freq_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    ul_ret = hmac_config_set_mode_etc(pst_mac_vap, (oal_uint16)OAL_SIZEOF(st_prot_param), (oal_uint8 *)&st_prot_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_sync_vap::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifndef _PRE_WLAN_FEATURE_P2P

OAL_STATIC oal_uint32 hmac_sta_sync_bss_freq(hmac_vap_stru *pst_hmac_vap, mac_channel_stru *pst_channel, wlan_protocol_enum_uint8 en_protocol)
{
    mac_cfg_mib_by_bw_param_stru st_cfg;

    if (OAL_SUCC == hmac_ap_clean_bss_etc(pst_hmac_vap))
    {
        st_cfg.en_band      = pst_channel->en_band;
        st_cfg.en_bandwidth = pst_channel->en_bandwidth;

        /* 设置AP侧状态机为 WAIT_START */
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);

        /* 更新bss能力信息 */
        hmac_config_set_mib_by_bw(&pst_hmac_vap->st_vap_base_info, (oal_uint16)OAL_SIZEOF(st_cfg), (oal_uint8 *)&st_cfg);

        return hmac_chan_start_bss_etc(pst_hmac_vap, pst_channel, en_protocol);
    }

    return OAL_FAIL;
}


OAL_STATIC oal_uint32 hmac_sta_sync_bss_freq_all(hmac_vap_stru *pst_hmac_sta)
{
    mac_device_stru *pst_mac_dev;
    hmac_vap_stru   *pst_hmac_vap;
    mac_vap_stru    *pst_mac_vap;
    oal_uint8        uc_idx;
    mac_channel_stru st_channel;
    wlan_protocol_enum_uint8 en_protocol;

    oal_memcopy((void*)&st_channel,(void*)&pst_hmac_sta->st_vap_base_info.st_channel,sizeof(mac_channel_stru));

    pst_mac_dev = mac_res_get_dev_etc(pst_hmac_sta->st_vap_base_info.uc_device_id);
    if (!pst_mac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CHAN, "{hmac_sta_sync_bss_freq_all::null pst_mac_dev}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_is_proxysta_enabled(pst_mac_dev))
    {
        // channel sync is handled by proxysta itself
        return OAL_SUCC;
    }
#endif

    for (uc_idx = 0; uc_idx < pst_mac_dev->uc_vap_num; uc_idx++)
    {
        pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_dev->auc_vap_id[uc_idx]);
        if (!pst_hmac_vap || (pst_hmac_vap == pst_hmac_sta) || (WLAN_VAP_MODE_BSS_AP != pst_hmac_vap->st_vap_base_info.en_vap_mode))
        {
            continue;
        }

        pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
        if (!pst_mac_vap)
        {
            OAM_WARNING_LOG0(0, OAM_SF_ASSOC, "{hmac_sta_sync_bss_freq_all::null pst_mac_vap}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        // sync channel if on same band while channel or bandwidth not same
        if ((pst_mac_vap->st_channel.en_band == st_channel.en_band)
        && ((pst_mac_vap->st_channel.en_bandwidth != st_channel.en_bandwidth)
            || (pst_mac_vap->st_channel.uc_chan_number != st_channel.uc_chan_number)))
        {
            // 本地BSS为WEP加密，rootap为HT使能，则本地BSS维持协议模式不变
            if (mac_is_wep_enabled(pst_mac_vap)
             && mac_mib_get_HighThroughputOptionImplemented(&pst_hmac_sta->st_vap_base_info))
            {
                en_protocol = pst_mac_vap->en_protocol;
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_sta_sync_bss_freq_all::keep protocol of vap%d while wep && HT}", pst_mac_dev->auc_vap_id[uc_idx]);
            }
            else
            {
                en_protocol = pst_hmac_sta->st_vap_base_info.en_protocol;
            }

            if(pst_mac_vap->bit_bw_fixed)
            {
                //固定带宽模式，AP带宽不跟随STA的带宽模式
                st_channel.en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
                OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_sta_sync_bss_freq_all::bit_bw_fixed set,keep vap[%d]'s bw[%d]}", pst_mac_dev->auc_vap_id[uc_idx],st_channel.en_bandwidth);
            }
            else
            {
                OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_sta_sync_bss_freq_all::bit_bw_fixed not set,vap[%d]'s bw[%d] should sync to new bw[%d]!}",
                    pst_mac_dev->auc_vap_id[uc_idx],
                    pst_mac_vap->st_channel.en_bandwidth,
                    st_channel.en_bandwidth);
                //Auto带宽模式，但是是WEP加密模式，只支持20M带宽，所以AP带宽也不跟随
                if (mac_is_wep_enabled(pst_mac_vap))
                {
                    st_channel.en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
                    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_sta_sync_bss_freq_all::keep vap[%d]'s bw[%d] due to WEP mode!}", pst_mac_dev->auc_vap_id[uc_idx],st_channel.en_bandwidth);
                }
            }

            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_sync_bss_freq_all:: mac_vap_pro[%d], cfg_pro[%d], sta_vap_pro[%d].}",
                            pst_mac_vap->en_protocol, en_protocol, pst_hmac_sta->st_vap_base_info.en_protocol);

            //如果是down的vap，则同步信道和协议模式
            if (MAC_VAP_STATE_INIT == pst_mac_vap->en_vap_state)
            {
                if (OAL_SUCC != hmac_sta_sync_vap(pst_hmac_vap, &st_channel, en_protocol))
                {
                    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_sync_bss_freq_all::sync down vap%d failed}", pst_mac_dev->auc_vap_id[uc_idx]);
                }
                continue;
            }

            if (OAL_SUCC != hmac_sta_sync_bss_freq(pst_hmac_vap, &st_channel, en_protocol))
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_sta_sync_bss_freq_all::sync vap%d failed}", pst_mac_dev->auc_vap_id[uc_idx]);
            }
        }
    }

    return OAL_SUCC;
}
#endif

oal_uint32 hmac_sta_wait_asoc_rx_etc(hmac_vap_stru *pst_hmac_sta, oal_void *pst_msg)
{
    mac_status_code_enum_uint16     en_asoc_status;
    oal_uint8                       uc_frame_sub_type;
    dmac_wlan_crx_event_stru       *pst_mgmt_rx_event;
    dmac_rx_ctl_stru               *pst_rx_ctrl;
    mac_rx_ctl_stru                *pst_rx_info;
    oal_uint8                      *puc_mac_hdr;
    oal_uint8                      *puc_payload;
    oal_uint16                      us_msg_len;
    oal_uint16                      us_hdr_len;
    hmac_asoc_rsp_stru              st_asoc_rsp;
    oal_uint8                       auc_addr_sa[WLAN_MAC_ADDR_LEN];
    oal_uint16                      us_user_idx;
    oal_uint32                      ul_rslt;
    hmac_user_stru                 *pst_hmac_user_ap;
    oal_uint32                      ul_ret;
    mac_vap_stru                   *pst_mac_vap;
    mac_cfg_user_info_param_stru    st_hmac_user_info_event;
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    oal_uint8 uc_ap_follow_channel = 0;
#endif

    if ((OAL_PTR_NULL == pst_hmac_sta) || (OAL_PTR_NULL == pst_msg))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_rx_etc::param null,%d %d.}", pst_hmac_sta, pst_msg);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap     = &(pst_hmac_sta->st_vap_base_info);

    pst_mgmt_rx_event   = (dmac_wlan_crx_event_stru *)pst_msg;
    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_mgmt_rx_event->pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    puc_mac_hdr         = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_info);
    puc_payload         = (oal_uint8 *)(puc_mac_hdr) + pst_rx_info->uc_mac_header_len;
    us_msg_len          = pst_rx_info->us_frame_len - pst_rx_info->uc_mac_header_len;   /* 消息总长度,不包括FCS */
    us_hdr_len          = pst_rx_info->uc_mac_header_len;

    uc_frame_sub_type = mac_get_frame_type_and_subtype(puc_mac_hdr);
    en_asoc_status    = mac_get_asoc_status(puc_payload);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_sta_wait_asoc_rx_etc::wakeup mgmt type[0x%x]}",uc_frame_sub_type);
    }
#endif
#endif

    if ((WLAN_FC0_SUBTYPE_ASSOC_RSP|WLAN_FC0_TYPE_MGT)      != uc_frame_sub_type &&
        (WLAN_FC0_SUBTYPE_REASSOC_RSP|WLAN_FC0_TYPE_MGT)    != uc_frame_sub_type)
    {
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                        "{hmac_sta_wait_asoc_rx_etc:: uc_frame_sub_type=0x%02x.}", uc_frame_sub_type);
        /* 打印此netbuf相关信息 */
        mac_rx_report_80211_frame_etc((oal_uint8 *)&(pst_hmac_sta->st_vap_base_info),
                                  (oal_uint8 *)pst_rx_ctrl,
                                  pst_mgmt_rx_event->pst_netbuf,
                                  OAM_OTA_TYPE_RX_HMAC_CB);
        return OAL_FAIL;
    }

    if (MAC_SUCCESSFUL_STATUSCODE != en_asoc_status)
    {
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                        "{hmac_sta_wait_asoc_rx_etc:: AP refuse STA assoc reason=%d.}", en_asoc_status);
        if (MAC_AP_FULL != en_asoc_status)
        {
            CHR_EXCEPTION(CHR_WIFI_DRV(CHR_WIFI_DRV_EVENT_CONNECT,CHR_WIFI_DRV_ERROR_ASSOC_REJECTED));
        }

        pst_hmac_sta->st_mgmt_timetout_param.en_status_code = en_asoc_status;

        return OAL_FAIL;
    }

    if (us_msg_len < OAL_ASSOC_RSP_FIXED_OFFSET)
    {
        OAM_ERROR_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_rx_etc::asoc_rsp_body is too short(%d) to going on!}",
                    us_msg_len);
        return OAL_FAIL;
    }

    /* 获取SA 地址 */
    mac_get_address2(puc_mac_hdr, auc_addr_sa);

    /* 根据SA 找到对应AP USER结构 */
    ul_rslt = mac_vap_find_user_by_macaddr_etc(&(pst_hmac_sta->st_vap_base_info), auc_addr_sa, &us_user_idx);

    if (OAL_SUCC != ul_rslt)
    {
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                        "{hmac_sta_wait_asoc_rx_etc:: mac_vap_find_user_by_macaddr_etc failed[%d].}", ul_rslt);

        return ul_rslt;
    }

    /* 获取STA关联的AP的用户指针 */
    pst_hmac_user_ap = mac_res_get_hmac_user_etc(us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user_ap)
    {
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                       "{hmac_sta_wait_asoc_rx_etc::pst_hmac_user_ap[%d] null.}", us_user_idx);
        return OAL_FAIL;
    }

    /* 取消定时器 */
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_sta->st_mgmt_timer));

    ul_ret = hmac_process_assoc_rsp_etc(pst_hmac_sta, pst_hmac_user_ap, puc_mac_hdr, us_hdr_len, puc_payload, us_msg_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_sta_wait_asoc_rx_etc::hmac_process_assoc_rsp_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    /* 关联上用户之后，初始化黑名单方案 */
    hmac_btcoex_blacklist_handle_init(pst_hmac_user_ap);

    if (OAL_TRUE == hmac_btcoex_check_exception_in_list_etc(pst_hmac_sta, auc_addr_sa))
    {
        if(BTCOEX_BLACKLIST_TPYE_FIX_BASIZE == HMAC_BTCOEX_GET_BLACKLIST_TYPE(pst_hmac_user_ap))
        {
            OAM_WARNING_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
                       "{hmac_sta_wait_asoc_rx_etc::mac_addr in blacklist.}");
            pst_hmac_user_ap->st_hmac_user_btcoex.st_hmac_btcoex_addba_req.en_ba_handle_allow = OAL_FALSE;
        }
        else
        {
            pst_hmac_user_ap->st_hmac_user_btcoex.st_hmac_btcoex_addba_req.en_ba_handle_allow = OAL_TRUE;
        }
    }
    else
    {
        /* 初始允许建立聚合，两个方案保持对齐 */
        pst_hmac_user_ap->st_hmac_user_btcoex.st_hmac_btcoex_addba_req.en_ba_handle_allow = OAL_TRUE;
    }
#endif

    /* STA切换到UP状态 */
    hmac_fsm_change_state_etc(pst_hmac_sta, MAC_VAP_STATE_UP);

    /* user已经关联上，抛事件给DMAC，在DMAC层挂用户算法钩子 */
    hmac_user_add_notify_alg_etc(&(pst_hmac_sta->st_vap_base_info), us_user_idx);

#ifdef _PRE_WLAN_FEATURE_ROAM
    hmac_roam_info_init_etc(pst_hmac_sta);
#endif //_PRE_WLAN_FEATURE_ROAM

    /* 将用户(AP)在本地的状态信息设置为已关联状态 */
#ifdef _PRE_DEBUG_MODE_USER_TRACK
    mac_user_change_info_event(pst_hmac_user_ap->st_user_base_info.auc_user_mac_addr,
                               pst_hmac_sta->st_vap_base_info.uc_vap_id,
                               pst_hmac_user_ap->st_user_base_info.en_user_asoc_state,
                               MAC_USER_STATE_ASSOC, OAM_MODULE_ID_HMAC,
                               OAM_USER_INFO_CHANGE_TYPE_ASSOC_STATE);
#endif

    /* 打开80211单播管理帧开关，观察关联过程，关联成功了就关闭 */
    //hmac_config_set_mgmt_log_etc(&pst_hmac_sta->st_vap_base_info, &pst_hmac_user_ap->st_user_base_info, OAL_FALSE);

    /* 准备消息，上报给APP */
    st_asoc_rsp.en_result_code = HMAC_MGMT_SUCCESS;
    st_asoc_rsp.en_status_code = MAC_SUCCESSFUL_STATUSCODE;

    /* 记录关联响应帧的部分内容，用于上报给内核 */
    st_asoc_rsp.ul_asoc_rsp_ie_len = us_msg_len - OAL_ASSOC_RSP_FIXED_OFFSET;  /* 除去MAC帧头24字节和FIXED部分6字节 */
    st_asoc_rsp.puc_asoc_rsp_ie_buff = puc_mac_hdr + OAL_ASSOC_RSP_IE_OFFSET;

    /* 获取AP的mac地址 */
    mac_get_bssid(puc_mac_hdr, st_asoc_rsp.auc_addr_ap);

    /* 获取关联请求帧信息 */
    st_asoc_rsp.puc_asoc_req_ie_buff = pst_hmac_user_ap->puc_assoc_req_ie_buff;
    st_asoc_rsp.ul_asoc_req_ie_len   = pst_hmac_user_ap->ul_assoc_req_ie_len;

    hmac_send_rsp_to_sme_sta_etc(pst_hmac_sta, HMAC_SME_ASOC_RSP, (oal_uint8 *)(&st_asoc_rsp));

    /* 1102 STA 入网后，上报VAP 信息和用户信息 */
    st_hmac_user_info_event.us_user_idx = us_user_idx;

    hmac_config_vap_info_etc(pst_mac_vap, OAL_SIZEOF(oal_uint32), (oal_uint8 *)&ul_ret);
    hmac_config_user_info_etc(pst_mac_vap, OAL_SIZEOF(mac_cfg_user_info_param_stru), (oal_uint8 *)&st_hmac_user_info_event);
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
    oal_notice_sta_join_result(pst_hmac_sta->st_vap_base_info.uc_chip_id, OAL_TRUE);
#else
    if(OAL_SUCC != oal_notice_sta_join_result(pst_hmac_sta->st_vap_base_info.uc_chip_id, OAL_TRUE))
    {
        OAM_WARNING_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_COEX,
            "{hmac_sta_wait_asoc_rx_etc::oal_notice_sta_join_result fail.}");
    }
#endif

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /*信道跟随检查*/
    ul_rslt = hmac_check_ap_channel_follow_sta(pst_mac_vap,&pst_mac_vap->st_channel,&uc_ap_follow_channel);
    if(OAL_SUCC == ul_rslt)
    {
        OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_wait_asoc_rx_etc:: after hmac_check_ap_channel_follow_sta. ap channel change to %d}",uc_ap_follow_channel);
    }
#endif

    // will not do any sync if proxysta enabled
    // allow running DBAC on different channels of same band while P2P defined
#ifndef _PRE_WLAN_FEATURE_P2P
    hmac_sta_sync_bss_freq_all(pst_hmac_sta);
#endif

    /*主要用于proxysta模式下，MAIN STA关联成功后，UP起来所有前面被DOWN掉的AP，非proxysta模式下该函数会直接返回成功 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    hmac_psta_proc_join_result(pst_hmac_sta, OAL_TRUE);
#endif

    return OAL_SUCC;
}

oal_uint32 hmac_sta_get_mngpkt_sendstat(mac_vap_stru *pst_mac_vap, mac_cfg_query_mngpkt_sendstat_stru *pst_sendstat_info)
{
    mac_device_stru                      *pst_mac_dev;

    if(OAL_PTR_NULL == pst_sendstat_info)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sta_get_mngpkt_sendstat::pst_sendstat_info null.}");
        return OAL_FAIL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sta_get_mngpkt_sendstat::mac_res_get_dev_etc failed.}");
        return OAL_FAIL;
    }

    pst_sendstat_info->uc_auth_req_st = pst_mac_dev->uc_auth_req_sendst;
    pst_sendstat_info->uc_asoc_req_st = pst_mac_dev->uc_asoc_req_sendst;

    return OAL_SUCC;
}

oal_void hmac_sta_clear_auth_req_sendstat(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru                      *pst_mac_dev;

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sta_clear_auth_req_sendstat::mac_res_get_dev_etc failed.}");
        return;
    }

    pst_mac_dev->uc_auth_req_sendst = 0;
}

oal_void hmac_sta_clear_asoc_req_sendstat(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru                      *pst_mac_dev;

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_sta_clear_asoc_req_sendstat::mac_res_get_dev_etc failed.}");
        return;
    }

    pst_mac_dev->uc_asoc_req_sendst = 0;
}


oal_uint32  hmac_sta_auth_timeout_etc(hmac_vap_stru *pst_hmac_sta, oal_void *p_param)
{
    hmac_auth_rsp_stru            st_auth_rsp = {{0,},};
    mac_cfg_query_mngpkt_sendstat_stru   st_mngpkt_sendstat_info;

    /* and send it to the host.                                          */
    if(MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2 == pst_hmac_sta->st_vap_base_info.en_vap_state)
    {
        st_auth_rsp.us_status_code = MAC_AUTH_RSP2_TIMEOUT;
    }
    else if(MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4 == pst_hmac_sta->st_vap_base_info.en_vap_state)
    {
        st_auth_rsp.us_status_code = MAC_AUTH_RSP4_TIMEOUT;
    }
    else
    {
        st_auth_rsp.us_status_code = HMAC_MGMT_TIMEOUT;
    }

    if(OAL_SUCC == hmac_sta_get_mngpkt_sendstat(&pst_hmac_sta->st_vap_base_info, &st_mngpkt_sendstat_info))
    {
        if(st_mngpkt_sendstat_info.uc_auth_req_st > 0)
        {
            st_auth_rsp.us_status_code = MAC_AUTH_REQ_SEND_FAIL_BEGIN + st_mngpkt_sendstat_info.uc_auth_req_st;
            hmac_sta_clear_auth_req_sendstat(&pst_hmac_sta->st_vap_base_info);
        }
    }

    /* Send the response to host now. */
    hmac_send_rsp_to_sme_sta_etc(pst_hmac_sta, HMAC_SME_AUTH_RSP, (oal_uint8 *)&st_auth_rsp);

    return OAL_SUCC;
}


oal_uint32 hmac_get_phy_rate_and_protocol(oal_uint8 uc_pre_rate, wlan_protocol_enum_uint8 *en_protocol, oal_uint8 *uc_rate_idx)
{
    oal_uint32 uc_idx;

    for (uc_idx = 0; uc_idx < DATARATES_80211G_NUM; uc_idx++)
    {
        if (g_st_data_rates[uc_idx].uc_expand_rate == uc_pre_rate || g_st_data_rates[uc_idx].uc_mac_rate == uc_pre_rate)
        {
            *uc_rate_idx = uc_idx;
            *en_protocol = g_st_data_rates[uc_idx].uc_protocol;
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}


oal_uint32 hmac_sta_get_min_rate(dmac_set_rate_stru *pst_rate_params, hmac_join_req_stru *pst_join_req)
{
    oal_uint32 uc_idx;
    oal_uint8 auc_min_rate_idx[2] = {0};/*第一个存储11b协议对应的速率，第二个存储11ag协议对应的速率*/
    oal_uint8 uc_min_rate_idx = 0;
    wlan_protocol_enum_uint8 uc_protocol = 0;

    if (OAL_UNLIKELY(pst_rate_params == OAL_PTR_NULL || pst_join_req == OAL_PTR_NULL))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_sta_get_min_rate::NULL param}");
        return OAL_FAIL;
    }

    OAL_MEMZERO(pst_rate_params, OAL_SIZEOF(dmac_set_rate_stru));

    //get min rate
    for (uc_idx = 0; uc_idx < pst_join_req->st_bss_dscr.uc_num_supp_rates; uc_idx++)
    {
        if (hmac_get_phy_rate_and_protocol(pst_join_req->st_bss_dscr.auc_supp_rates[uc_idx], &uc_protocol, &uc_min_rate_idx) != OAL_SUCC)
        {
            OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_sta_get_min_rate::hmac_get_rate_protocol failed (%d).[%d]}",
                                           uc_min_rate_idx, uc_idx);
            continue;
        }

        /* 根据枚举wlan_phy_protocol_enum 填写数组对应位置的值 */
        if (pst_rate_params->un_capable_flag.uc_value & ((oal_uint32)1 << (uc_protocol)))
        {
            if (auc_min_rate_idx[uc_protocol] > uc_min_rate_idx)
            {
                auc_min_rate_idx[uc_protocol] = uc_min_rate_idx;
            }
        }
        else
        {
            auc_min_rate_idx[uc_protocol] = uc_min_rate_idx;
            pst_rate_params->un_capable_flag.uc_value |= ((oal_uint32)1 << (uc_protocol));
        }

        if (uc_min_rate_idx == 0x08)//st_data_rates第八个是24M
        {
            pst_rate_params->un_capable_flag.st_capable.bit_ht_capable = OAL_TRUE;
            pst_rate_params->un_capable_flag.st_capable.bit_vht_capable = OAL_TRUE;
        }
    }

    /* 与储存在扫描结果描述符中的能力进行比较，看能力是否匹配 */
    pst_rate_params->un_capable_flag.st_capable.bit_ht_capable &= pst_join_req->st_bss_dscr.en_ht_capable;
    pst_rate_params->un_capable_flag.st_capable.bit_vht_capable &= pst_join_req->st_bss_dscr.en_vht_capable;
    pst_rate_params->uc_min_rate[WLAN_11B_PHY_PROTOCOL_MODE] = g_st_data_rates[auc_min_rate_idx[WLAN_11B_PHY_PROTOCOL_MODE]].uc_phy_rate;
    pst_rate_params->uc_min_rate[WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE] = g_st_data_rates[auc_min_rate_idx[WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE]].uc_phy_rate;

    OAM_WARNING_LOG4(0, OAM_SF_ASSOC, "{hmac_sta_get_min_rate:: uc_min_rate_idx[%d]un_capable_flag.uc_value[%d]legacy rate[%d][%d]}",
           uc_min_rate_idx, pst_rate_params->un_capable_flag.uc_value,
           pst_rate_params->uc_min_rate[WLAN_11B_PHY_PROTOCOL_MODE],pst_rate_params->uc_min_rate[WLAN_LEGACY_OFDM_PHY_PROTOCOL_MODE]);

    return OAL_SUCC;
}


oal_uint32 hmac_sta_update_join_req_params_etc(hmac_vap_stru *pst_hmac_vap, hmac_join_req_stru *pst_join_req)
{
    mac_vap_stru                   *pst_mac_vap;
    dmac_ctx_join_req_set_reg_stru *pst_reg_params;
    frw_event_mem_stru             *pst_event_mem;
    frw_event_stru                 *pst_event;
    oal_uint32                      ul_ret;
    mac_device_stru                *pst_mac_device;
    wlan_mib_ieee802dot11_stru     *pst_mib_info;
    mac_cfg_mode_param_stru         st_cfg_mode;
    oal_uint8                      *puc_cur_ssid;

    pst_mac_vap      = &(pst_hmac_vap->st_vap_base_info);
    pst_mib_info     = pst_mac_vap->pst_mib_info;
    if (OAL_PTR_NULL == pst_mib_info)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 设置BSSID */
    mac_vap_set_bssid_etc(pst_mac_vap, pst_join_req->st_bss_dscr.auc_bssid);

    /* 更新mib库对应的dot11BeaconPeriod值 */
    mac_mib_set_BeaconPeriod(pst_mac_vap, (oal_uint32)(pst_join_req->st_bss_dscr.us_beacon_period));


    /* 更新mib库对应的ul_dot11CurrentChannel值*/
    mac_vap_set_current_channel_etc(pst_mac_vap, pst_join_req->st_bss_dscr.st_channel.en_band, pst_join_req->st_bss_dscr.st_channel.uc_chan_number);

#ifdef _PRE_WLAN_FEATURE_11D
    /* 更新sta期望加入的国家字符串 */
    pst_hmac_vap->ac_desired_country[0] = pst_join_req->st_bss_dscr.ac_country[0];
    pst_hmac_vap->ac_desired_country[1] = pst_join_req->st_bss_dscr.ac_country[1];
    pst_hmac_vap->ac_desired_country[2] = pst_join_req->st_bss_dscr.ac_country[2];
#endif

    /* 更新mib库对应的ssid */
    puc_cur_ssid = mac_mib_get_DesiredSSID(pst_mac_vap);
    oal_memcopy(puc_cur_ssid, pst_join_req->st_bss_dscr.ac_ssid, WLAN_SSID_MAX_LEN);
    puc_cur_ssid[WLAN_SSID_MAX_LEN - 1] = '\0';

    /* 更新频带、主20MHz信道号，与AP通信 DMAC切换信道时直接调用 */
    MAC_VAP_GET_CAP_BW(pst_mac_vap) = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap), pst_join_req->st_bss_dscr.en_channel_bandwidth);
    pst_mac_vap->st_channel.uc_chan_number = pst_join_req->st_bss_dscr.st_channel.uc_chan_number;
    pst_mac_vap->st_channel.en_band        = pst_join_req->st_bss_dscr.st_channel.en_band;

    /* 在STA未配置协议模式情况下，根据要关联的AP，更新mib库中对应的HT/VHT能力 */
    if (OAL_SWITCH_OFF == pst_hmac_vap->bit_sta_protocol_cfg)
    {
        oal_memset(&st_cfg_mode, 0, OAL_SIZEOF(mac_cfg_mode_param_stru));

        mac_mib_set_HighThroughputOptionImplemented(pst_mac_vap, pst_join_req->st_bss_dscr.en_ht_capable);
        mac_mib_set_VHTOptionImplemented(pst_mac_vap, pst_join_req->st_bss_dscr.en_vht_capable);

    #ifdef _PRE_WLAN_FEATURE_11AX
        if(IS_LEGACY_STA(pst_mac_vap) && (OAL_TRUE == pst_join_req->st_bss_dscr.en_he_capable) && (IS_11AX_VAP(pst_mac_vap)))
        {
            mac_mib_set_HEOptionImplemented(pst_mac_vap,OAL_TRUE);
        }
        else
        {
            mac_mib_set_HEOptionImplemented(pst_mac_vap,OAL_FALSE);
        }
    #endif

        /* STA更新LDPC和STBC的能力,更新能力保存到Activated mib中 */
        mac_mib_set_LDPCCodingOptionActivated(pst_mac_vap, (oal_bool_enum_uint8)(pst_join_req->st_bss_dscr.en_ht_ldpc & mac_mib_get_LDPCCodingOptionImplemented(pst_mac_vap)));
        mac_mib_set_TxSTBCOptionActivated(pst_mac_vap, (oal_bool_enum_uint8)(pst_join_req->st_bss_dscr.en_ht_stbc & mac_mib_get_TxSTBCOptionImplemented(pst_mac_vap)));

        /* 关联2G AP，且2ght40禁止位为1时，不学习AP的HT 40能力 */
        if (!(WLAN_BAND_2G == pst_mac_vap->st_channel.en_band && pst_mac_vap->st_cap_flag.bit_disable_2ght40))
        {
            if(WLAN_BW_CAP_20M == pst_join_req->st_bss_dscr.en_bw_cap)
            {
                mac_mib_set_FortyMHzOperationImplemented(pst_mac_vap, OAL_FALSE);
            }
            else
            {
                mac_mib_set_FortyMHzOperationImplemented(pst_mac_vap, OAL_TRUE);
            }
        }

        /*根据要加入AP的协议模式更新STA侧速率集 */
        ul_ret = hmac_sta_get_user_protocol_etc(&(pst_join_req->st_bss_dscr), &(st_cfg_mode.en_protocol));
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_sta_update_join_req_params_etc::hmac_sta_get_user_protocol_etc fail %d.}", ul_ret);
            return ul_ret;
        }

        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_update_join_req_params_etc::mac_vap_pro[%d], cfg_pro[%d]}", pst_mac_vap->en_protocol, st_cfg_mode.en_protocol);

        st_cfg_mode.en_band         = pst_join_req->st_bss_dscr.st_channel.en_band;
        st_cfg_mode.en_bandwidth    = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap), pst_join_req->st_bss_dscr.en_channel_bandwidth);
        st_cfg_mode.en_channel_idx  = pst_join_req->st_bss_dscr.st_channel.uc_chan_number;
        ul_ret = hmac_config_sta_update_rates_etc(pst_mac_vap, &st_cfg_mode, (oal_void *)&pst_join_req->st_bss_dscr);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{hmac_sta_update_join_req_params_etc::hmac_config_sta_update_rates_etc fail %d.}", ul_ret);
            return ul_ret;
        }
    }

    /* 有些加密协议只能工作在legacy */
    hmac_sta_protocol_down_by_chipher(pst_mac_vap, &pst_join_req->st_bss_dscr);
#ifndef _PRE_WIFI_DMT
    st_cfg_mode.en_protocol  = pst_mac_vap->en_protocol;
    st_cfg_mode.en_band      = pst_mac_vap->st_channel.en_band;
    st_cfg_mode.en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
    st_cfg_mode.en_channel_idx  = pst_join_req->st_bss_dscr.st_channel.uc_chan_number;
    hmac_config_sta_update_rates_etc(pst_mac_vap, &st_cfg_mode, (oal_void *)&pst_join_req->st_bss_dscr);
#endif

    /* STA首先以20MHz运行，如果要切换到40 or 80MHz运行，需要满足一下条件: */
    /* (1) 用户支持40 or 80MHz运行 */
    /* (2) AP支持40 or 80MHz运行(HT Supported Channel Width Set = 1 && VHT Supported Channel Width Set = 0) */
    /* (3) AP在40 or 80MHz运行(SCO = SCA or SCB && VHT Channel Width = 1) */
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    pst_mac_vap->st_channel.en_bandwidth   = WLAN_BAND_WIDTH_20M;
#endif
    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number, &(pst_mac_vap->st_channel.uc_chan_idx));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_update_join_req_params_etc::band and channel_num are not compatible.band[%d], channel_num[%d]}",
                        pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number);
        return ul_ret;
    }

    /* 更新协议相关信息，包括WMM P2P 11I 20/40M等 */
    hmac_update_join_req_params_prot_sta(pst_hmac_vap, pst_join_req);
    /* 入网优化，不同频段下的能力不一样 */
    if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
    {
        mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, WLAN_LEGACY_11B_MIB_SHORT_PREAMBLE);
        mac_mib_set_SpectrumManagementRequired(pst_mac_vap, OAL_FALSE);
    }
    else
    {
        mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, WLAN_LEGACY_11B_MIB_LONG_PREAMBLE);
        mac_mib_set_SpectrumManagementRequired(pst_mac_vap, OAL_TRUE);
    }

    /* 根据协议信道做带宽约束, 此时已经跟进join参数刷新好带宽和信道，此时直接根据当前信道再次刷新带宽 */
    hmac_sta_bandwidth_down_by_channel(pst_mac_vap);

    if (0 == hmac_calc_up_ap_num_etc(pst_mac_device))
    {
        pst_mac_device->uc_max_channel   = pst_mac_vap->st_channel.uc_chan_number;
        pst_mac_device->en_max_band      = pst_mac_vap->st_channel.en_band;
        pst_mac_device->en_max_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
    }

    /* 抛事件到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_sta_update_join_req_params_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_SET_REG,
                       OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    pst_reg_params = (dmac_ctx_join_req_set_reg_stru *)pst_event->auc_event_data;

    /* 设置需要写入寄存器的BSSID信息 */
    oal_set_mac_addr(pst_reg_params->auc_bssid, pst_join_req->st_bss_dscr.auc_bssid);

    /* 填写信道相关信息 */
    pst_reg_params->st_current_channel.uc_chan_number = pst_mac_vap->st_channel.uc_chan_number;
    pst_reg_params->st_current_channel.en_band        = pst_mac_vap->st_channel.en_band;
    pst_reg_params->st_current_channel.en_bandwidth   = pst_mac_vap->st_channel.en_bandwidth;
    pst_reg_params->st_current_channel.uc_chan_idx         = pst_mac_vap->st_channel.uc_chan_idx;

    /* 填写速率相关信息 */
    hmac_sta_get_min_rate(&pst_reg_params->st_min_rate, pst_join_req);

    /* 设置beaocn period信息 */
    pst_reg_params->us_beacon_period      = (pst_join_req->st_bss_dscr.us_beacon_period);

    /* 同步FortyMHzOperationImplemented */
    pst_reg_params->en_dot11FortyMHzOperationImplemented   = mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap);

    /* 设置beacon filter关闭 */
    pst_reg_params->ul_beacon_filter    = OAL_FALSE;

    /* 设置no frame filter打开 */
    pst_reg_params->ul_non_frame_filter = OAL_TRUE;

    /* 下发ssid */
    oal_memcopy(pst_reg_params->auc_ssid, pst_join_req->st_bss_dscr.ac_ssid, WLAN_SSID_MAX_LEN);
    pst_reg_params->auc_ssid[WLAN_SSID_MAX_LEN - 1] = '\0';

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

#ifdef _PRE_WLAN_FEATURE_M2S
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_sta_update_join_req_params_etc::ap nss[%d],ap band[%d],ap ht cap[%d],ap vht cap[%d].}",
        pst_join_req->st_bss_dscr.en_support_max_nss, pst_join_req->st_bss_dscr.st_channel.en_band,
        pst_join_req->st_bss_dscr.en_ht_capable, pst_join_req->st_bss_dscr.en_vht_capable);

    /* 双流及其以下，三四流ht设备都配置为不支持opmode */
    if((WLAN_SINGLE_NSS == pst_join_req->st_bss_dscr.en_support_max_nss) ||
        (OAL_FALSE == pst_join_req->st_bss_dscr.en_vht_capable) ||
        (WLAN_DOUBLE_NSS == pst_join_req->st_bss_dscr.en_support_max_nss && OAL_FALSE == pst_join_req->st_bss_dscr.en_support_opmode))
    {
        pst_mac_vap->st_cap_flag.bit_opmode = OAL_FALSE;
    }
    else
    {
        pst_mac_vap->st_cap_flag.bit_opmode = OAL_TRUE;
    }

    /* 同步vap修改信息到device侧 */
    hmac_config_vap_m2s_info_syn(pst_mac_vap);
#endif

    return OAL_SUCC;
}


oal_uint32 hmac_sta_wait_asoc_timeout_etc(hmac_vap_stru *pst_hmac_sta, oal_void *p_param)
{
    hmac_asoc_rsp_stru            st_asoc_rsp = {0};
    hmac_mgmt_timeout_param_stru *pst_timeout_param;
    mac_cfg_query_mngpkt_sendstat_stru   st_mngpkt_sendstat_info;

    if ((OAL_PTR_NULL == pst_hmac_sta)|| (OAL_PTR_NULL == p_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_sta_wait_asoc_timeout_etc::param null,%d %d.}", pst_hmac_sta,p_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_timeout_param = (hmac_mgmt_timeout_param_stru *)p_param;

    /* 填写关联结果 */
    st_asoc_rsp.en_result_code = HMAC_MGMT_TIMEOUT;

    /* 关联超时失败,原因码上报wpa_supplicant */
    st_asoc_rsp.en_status_code = pst_timeout_param->en_status_code;

    if(OAL_SUCC == hmac_sta_get_mngpkt_sendstat(&pst_hmac_sta->st_vap_base_info, &st_mngpkt_sendstat_info))
    {
        if(st_mngpkt_sendstat_info.uc_asoc_req_st > 0 && st_asoc_rsp.en_status_code == MAC_ASOC_RSP_TIMEOUT)
        {
            st_asoc_rsp.en_status_code = MAC_ASOC_REQ_SEND_FAIL_BEGIN + st_mngpkt_sendstat_info.uc_asoc_req_st;
            hmac_sta_clear_asoc_req_sendstat(&pst_hmac_sta->st_vap_base_info);
        }
    }

    /* 发送关联结果给SME */
    hmac_send_rsp_to_sme_sta_etc(pst_hmac_sta, HMAC_SME_ASOC_RSP, (oal_uint8 *)&st_asoc_rsp);

    return OAL_SUCC;
}


oal_void  hmac_sta_handle_disassoc_rsp_etc(hmac_vap_stru *pst_hmac_vap, oal_uint16 us_disasoc_reason_code)
{
    frw_event_mem_stru  *pst_event_mem;
    frw_event_stru      *pst_event;

    /* 通告vap sta去关联 */
    if (WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        oal_notice_sta_join_result(pst_hmac_vap->st_vap_base_info.uc_chip_id, OAL_FALSE);
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
        if (OAL_SUCC != oal_notice_sta_join_result(pst_hmac_vap->st_vap_base_info.uc_chip_id, OAL_FALSE))
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                "{hmac_sta_handle_disassoc_rsp_etc::oal_notice_sta_join_result fail.}");
        }
#endif
    }

    /* 抛加入完成事件到WAL */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(oal_uint16));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_sta_handle_disassoc_rsp_etc::pst_event_mem null.}");
        return;
    }

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                     FRW_EVENT_TYPE_HOST_CTX,
                     HMAC_HOST_CTX_EVENT_SUB_TYPE_DISASOC_COMP_STA,
                     OAL_SIZEOF(oal_uint16),
                     FRW_EVENT_PIPELINE_STAGE_0,
                     pst_hmac_vap->st_vap_base_info.uc_chip_id,
                     pst_hmac_vap->st_vap_base_info.uc_device_id,
                     pst_hmac_vap->st_vap_base_info.uc_vap_id);

    *((oal_uint16 *)pst_event->auc_event_data) = us_disasoc_reason_code;      /* 事件payload填写的是错误码 */

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return;
}


OAL_STATIC oal_uint32 hmac_sta_rx_deauth_req(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_mac_hdr, oal_bool_enum_uint8 en_is_protected)
{
    oal_uint8       auc_bssid[6]            = {0};
    hmac_user_stru *pst_hmac_user_vap       = OAL_PTR_NULL;
    oal_uint16      us_user_idx             = 0xffff;
    oal_uint32      ul_ret;
    oal_uint8      *puc_da;
    oal_uint8      *puc_sa = OAL_PTR_NULL;
    oal_uint32      ul_ret_del_user;


    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == puc_mac_hdr)
    {
        OAM_ERROR_LOG2(0, OAM_SF_AUTH, "{hmac_sta_rx_deauth_req::param null,%d %d.}", pst_hmac_vap, puc_mac_hdr);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 增加接收到去认证帧或者去关联帧时的维测信息 */
    mac_rx_get_sa((mac_ieee80211_frame_stru *)puc_mac_hdr, &puc_sa);
    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                     "{hmac_sta_rx_deauth_req::Because of err_code[%d], received deauth or disassoc frame frome source addr, sa xx:xx:xx:%2x:%2x:%2x.}",
                     *((oal_uint16 *)(puc_mac_hdr + MAC_80211_FRAME_LEN)), puc_sa[3], puc_sa[4], puc_sa[5]);

    mac_get_address2(puc_mac_hdr, auc_bssid);

    ul_ret = mac_vap_find_user_by_macaddr_etc(&pst_hmac_vap->st_vap_base_info, auc_bssid, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                         "{hmac_sta_rx_deauth_req::find user failed[%d],other bss deauth frame!}", ul_ret);
        /* 没有查到对应的USER,发送去认证消息 */
        //hmac_mgmt_send_deauth_frame_etc(&(pst_hmac_vap->st_vap_base_info), auc_bssid, MAC_NOT_AUTHED, OAL_FALSE);
        return ul_ret;
    }
    pst_hmac_user_vap = mac_res_get_hmac_user_etc(us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user_vap)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                       "{hmac_sta_rx_deauth_req::pst_hmac_user_vap[%d] null.}", us_user_idx);
        /* 没有查到对应的USER,发送去认证消息 */
        hmac_mgmt_send_deauth_frame_etc(&(pst_hmac_vap->st_vap_base_info), auc_bssid, MAC_NOT_AUTHED, OAL_FALSE);

        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        /* 上报内核sta已经和某个ap去关联 */
        hmac_sta_handle_disassoc_rsp_etc(pst_hmac_vap, *((oal_uint16 *)(puc_mac_hdr + MAC_80211_FRAME_LEN)));
        return OAL_FAIL;
    }

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    /*检查是否需要发送SA query request*/
    if ((MAC_USER_STATE_ASSOC == pst_hmac_user_vap->st_user_base_info.en_user_asoc_state) &&
        (OAL_SUCC == hmac_pmf_check_err_code_etc(&pst_hmac_user_vap->st_user_base_info, en_is_protected, puc_mac_hdr)))
    {
       /*在关联状态下收到未加密的ReasonCode 6/7需要启动SA Query流程*/
       ul_ret = hmac_start_sa_query_etc(&pst_hmac_vap->st_vap_base_info, pst_hmac_user_vap, pst_hmac_user_vap->st_user_base_info.st_cap_info.bit_pmf_active);
       if(OAL_SUCC != ul_ret)
       {
           return OAL_ERR_CODE_PMF_SA_QUERY_START_FAIL;
       }

       return OAL_SUCC;
    }

#endif

    /*如果该用户的管理帧加密属性不一致，丢弃该报文*/
    mac_rx_get_da((mac_ieee80211_frame_stru *)puc_mac_hdr, &puc_da);
    if ((OAL_TRUE != ETHER_IS_MULTICAST(puc_da)) &&
       (en_is_protected != pst_hmac_user_vap->st_user_base_info.st_cap_info.bit_pmf_active))
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                       "{hmac_sta_rx_deauth_req::PMF check failed.}");

        return OAL_FAIL;
    }

    /* TBD 上报system error 复位 mac */

    /* 删除user */
    ul_ret_del_user = hmac_user_del_etc(&pst_hmac_vap->st_vap_base_info, pst_hmac_user_vap);
    if ( OAL_SUCC != ul_ret_del_user)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,
                       "{hmac_sta_rx_deauth_req::hmac_user_del_etc failed.}");

        /* 上报内核sta已经和某个ap去关联 */
        hmac_sta_handle_disassoc_rsp_etc(pst_hmac_vap, *((oal_uint16 *)(puc_mac_hdr + MAC_80211_FRAME_LEN)));
        return OAL_FAIL;
    }

    /* 上报内核sta已经和某个ap去关联 */
    hmac_sta_handle_disassoc_rsp_etc(pst_hmac_vap, *((oal_uint16 *)(puc_mac_hdr + MAC_80211_FRAME_LEN)));

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_sta_up_rx_beacon(hmac_vap_stru *pst_hmac_vap_sta, oal_netbuf_stru *pst_netbuf)
{
    dmac_rx_ctl_stru           *pst_rx_ctrl;
    mac_rx_ctl_stru            *pst_rx_info;
    mac_ieee80211_frame_stru   *pst_mac_hdr;
    oal_uint32                  ul_ret;
    oal_uint16                  us_frame_len;
    oal_uint16                  us_frame_offset;
    oal_uint8                  *puc_frame_body;
    oal_uint8                   uc_frame_sub_type;
    hmac_user_stru             *pst_hmac_user;
    oal_uint8                   auc_addr_sa[WLAN_MAC_ADDR_LEN];
    oal_uint16                  us_user_idx;
#ifdef _PRE_WLAN_FEATURE_TXBF
    oal_uint8                  *puc_txbf_vendor_ie;
#endif

    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    pst_mac_hdr         = (mac_ieee80211_frame_stru *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_info);
    puc_frame_body      = (oal_uint8* )pst_mac_hdr + pst_rx_info->uc_mac_header_len;
    us_frame_len        = pst_rx_info->us_frame_len - pst_rx_info->uc_mac_header_len; /*帧体长度*/

    us_frame_offset = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    uc_frame_sub_type = mac_get_frame_sub_type((oal_uint8 *)pst_mac_hdr);

    /* 来自其它bss的Beacon不做处理 */
    ul_ret = oal_compare_mac_addr(pst_hmac_vap_sta->st_vap_base_info.auc_bssid, pst_mac_hdr->auc_address3);
    if(0 != ul_ret)
    {
        return OAL_SUCC;
    }

    /* 获取管理帧的源地址SA */
    mac_get_address2((oal_uint8 *)pst_mac_hdr, auc_addr_sa);

    /* 根据SA 地地找到对应AP USER结构 */
    ul_ret = mac_vap_find_user_by_macaddr_etc(&(pst_hmac_vap_sta->st_vap_base_info), auc_addr_sa, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                        "{hmac_sta_up_rx_beacon::mac_vap_find_user_by_macaddr_etc failed[%d].}", ul_ret);
        return ul_ret;
    }
    pst_hmac_user = mac_res_get_hmac_user_etc(us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX,
                       "{hmac_sta_up_rx_beacon::pst_hmac_user[%d] null.}", us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }
#ifdef _PRE_WLAN_FEATURE_TXBF
    /* 更新11n txbf能力 */
    puc_txbf_vendor_ie = mac_find_vendor_ie_etc(MAC_HUAWEI_VENDER_IE, MAC_EID_11NTXBF, (oal_uint8 *)pst_mac_hdr + MAC_TAG_PARAM_OFFSET, us_frame_len - us_frame_offset);
    hmac_mgmt_update_11ntxbf_cap_etc(puc_txbf_vendor_ie, pst_hmac_user);
#endif

    /* 评估是否需要进行带宽切换 */
    //hmac_chan_reval_bandwidth_sta_etc(&pst_hmac_vap_sta->st_vap_base_info, ul_change_flag);

    /* 更新edca参数 */
    hmac_sta_up_update_edca_params_etc(puc_frame_body + us_frame_offset, us_frame_len - us_frame_offset, pst_hmac_vap_sta, uc_frame_sub_type, pst_hmac_user);

#ifdef _PRE_WLAN_FEATURE_11AX
    hmac_sta_up_update_he_edca_params(puc_frame_body + us_frame_offset, us_frame_len - us_frame_offset, pst_hmac_vap_sta, uc_frame_sub_type, pst_hmac_user);
#endif

    return OAL_SUCC;
}


#if 0
OAL_STATIC oal_void  hmac_handle_tbtt_chan_mgmt_sta(hmac_vap_stru *pst_hmac_vap)
{
    mac_vap_stru *pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    if (OAL_FALSE == mac_mib_get_SpectrumManagementImplemented(pst_mac_vap))
    {
        return;
    }

    /* 如果AP发送的CSA IE中的"信道切换计数"为零，则立即切换信道 */
    if (OAL_TRUE == pst_mac_vap->st_ch_switch_info.en_channel_swt_cnt_zero)
    {
        if (pst_hmac_vap->us_link_loss > WLAN_LINK_LOSS_THRESHOLD)
        {
            hmac_chan_sta_switch_channel(pst_mac_vap);
            pst_hmac_vap->us_link_loss = 0;
        }
    }

    /* 信道切换已经完成。现在新信道上等待接收AP发送的Beacon帧 */
    if (OAL_TRUE == pst_mac_vap->st_ch_switch_info.en_waiting_for_ap)
    {
        /* 等待一段时间后，在新信道上恢复硬件发送(即便这时AP有可能还没有切换至新信道) */
        if (pst_hmac_vap->us_link_loss > WLAN_LINK_LOSS_THRESHOLD)
        {
            pst_mac_vap->st_ch_switch_info.en_waiting_for_ap = OAL_FALSE;
            hmac_chan_enable_machw_tx_etc(pst_mac_vap);

            pst_hmac_vap->us_link_loss = 0;
        }
    }

    /* 如果AP发送的CSA IE中的"信道切换计数"不为零，则每一次TBTT中断中减一 */
    if (pst_mac_vap->st_ch_switch_info.uc_new_ch_swt_cnt > 0)
    {
        pst_mac_vap->st_ch_switch_info.uc_new_ch_swt_cnt--;

        if (0 == pst_mac_vap->st_ch_switch_info.uc_new_ch_swt_cnt)
        {
            if (OAL_TRUE == pst_mac_vap->st_ch_switch_info.en_waiting_to_shift_channel)
            {
                hmac_chan_sta_switch_channel(pst_mac_vap);
            }
        }
    }
}
#endif


OAL_STATIC oal_void  hmac_sta_up_rx_action_nonuser(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf)
{
    dmac_rx_ctl_stru               *pst_rx_ctrl;
    oal_uint8                      *puc_data;
    mac_ieee80211_frame_stru       *pst_frame_hdr;          /* 保存mac帧的指针 */

    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    /* 获取帧头信息 */
    pst_frame_hdr = (mac_ieee80211_frame_stru *)MAC_GET_RX_CB_MAC_HEADER_ADDR(&pst_rx_ctrl->st_rx_info);


    /* 获取帧体指针 */
    puc_data = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(&pst_rx_ctrl->st_rx_info) + pst_rx_ctrl->st_rx_info.uc_mac_header_len;

    /* Category */
    switch (puc_data[MAC_ACTION_OFFSET_CATEGORY])
    {
        case MAC_ACTION_CATEGORY_PUBLIC:
        {
            /* Action */
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_PUB_VENDOR_SPECIFIC:
                {
                #if defined(_PRE_WLAN_FEATURE_LOCATION) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
                    if (0 == oal_memcmp(puc_data  + MAC_ACTION_CATEGORY_AND_CODE_LEN, g_auc_huawei_oui, MAC_OUI_LEN))
                    {
                       OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_sta_up_rx_action_nonuser::hmac location get.}");
                       hmac_huawei_action_process(pst_hmac_vap, pst_netbuf, puc_data[MAC_ACTION_CATEGORY_AND_CODE_LEN + MAC_OUI_LEN]);
                    }
                #endif
                    break;
                }
                default:
                    break;
            }
        }
        break;

        default:
            break;
    }
    return;
}


OAL_STATIC oal_void  hmac_sta_up_rx_action(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf, oal_bool_enum_uint8 en_is_protected)
{
    dmac_rx_ctl_stru               *pst_rx_ctrl;
    oal_uint8                      *puc_data;
    mac_ieee80211_frame_stru       *pst_frame_hdr;          /* 保存mac帧的指针 */
    hmac_user_stru                 *pst_hmac_user;
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_uint8                      *puc_p2p0_mac_addr;
#endif

    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    /* 获取帧头信息 */
    pst_frame_hdr = (mac_ieee80211_frame_stru *)MAC_GET_RX_CB_MAC_HEADER_ADDR(&pst_rx_ctrl->st_rx_info);
#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P0设备所接受的action全部上报 */
    puc_p2p0_mac_addr = mac_mib_get_p2p0_dot11StationID(&pst_hmac_vap->st_vap_base_info);
    if (0 == oal_compare_mac_addr(pst_frame_hdr->auc_address1, puc_p2p0_mac_addr))
    {
        hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap, pst_netbuf);
    }
#endif

    /* 获取发送端的用户指针 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(&pst_hmac_vap->st_vap_base_info, pst_frame_hdr->auc_address2);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_sta_up_rx_action::mac_vap_find_user_by_macaddr_etc failed.}");
        hmac_sta_up_rx_action_nonuser(pst_hmac_vap, pst_netbuf);
        return;
    }

    /* 获取帧体指针 */
    puc_data = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(&pst_rx_ctrl->st_rx_info) + pst_rx_ctrl->st_rx_info.uc_mac_header_len;

    /* Category */
    switch (puc_data[MAC_ACTION_OFFSET_CATEGORY])
    {
        case MAC_ACTION_CATEGORY_BA:
        {
            switch(puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_BA_ACTION_ADDBA_REQ:
                 #ifdef _PRE_WLAN_FEATURE_BTCOEX
                    hmac_btcoex_check_rx_same_baw_start_from_addba_req_etc(pst_hmac_vap, pst_hmac_user, pst_frame_hdr, puc_data);
                #endif
                    hmac_mgmt_rx_addba_req_etc(pst_hmac_vap, pst_hmac_user, puc_data);
                    break;

                case MAC_BA_ACTION_ADDBA_RSP:
                    hmac_mgmt_rx_addba_rsp_etc(pst_hmac_vap, pst_hmac_user, puc_data);
                    break;

                case MAC_BA_ACTION_DELBA:
                    hmac_mgmt_rx_delba_etc(pst_hmac_vap, pst_hmac_user, puc_data);
                    break;

                default:
                    break;
            }
        }
        break;

        case MAC_ACTION_CATEGORY_WNM:
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_sta_up_rx_action::MAC_ACTION_CATEGORY_WNM action=%d.}",
                puc_data[MAC_ACTION_OFFSET_ACTION]);
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
            #ifdef _PRE_WLAN_FEATURE_11V_ENABLE
                /* bss transition request 帧处理入口 */
                case MAC_WNM_ACTION_BSS_TRANSITION_MGMT_REQUEST:
                    hmac_rx_bsst_req_action(pst_hmac_vap, pst_hmac_user, pst_netbuf);
                    break;
            #endif
                default:
            #ifdef _PRE_WLAN_FEATURE_HS20
                    /* 上报WNM Notification Request Action帧 */
                    hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap, pst_netbuf);
            #endif
                    break;
            }

             break;

        case MAC_ACTION_CATEGORY_PUBLIC:
        {
            /* Action */
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_PUB_VENDOR_SPECIFIC:
            #ifdef _PRE_WLAN_FEATURE_P2P
                /*查找OUI-OUI type值为 50 6F 9A - 09 (WFA P2P v1.0)  */
                /* 并用hmac_rx_mgmt_send_to_host接口上报 */
                    if (OAL_TRUE == mac_ie_check_p2p_action_etc(puc_data + MAC_ACTION_CATEGORY_AND_CODE_LEN))
                    {
                       hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap, pst_netbuf);
                    }
            #endif
            #if defined(_PRE_WLAN_FEATURE_LOCATION) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS)
                    if (0 == oal_memcmp(puc_data  + MAC_ACTION_CATEGORY_AND_CODE_LEN, g_auc_huawei_oui, MAC_OUI_LEN))
                    {
                       OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_sta_up_rx_action::hmac location get.}");
                       hmac_huawei_action_process(pst_hmac_vap, pst_netbuf, puc_data[MAC_ACTION_CATEGORY_AND_CODE_LEN + MAC_OUI_LEN]);
                    }
            #endif
                    break;

                case MAC_PUB_GAS_INIT_RESP:
                case MAC_PUB_GAS_COMBAK_RESP:
            #ifdef _PRE_WLAN_FEATURE_HS20
                /*上报GAS查询的ACTION帧 */
                 hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap, pst_netbuf);
            #endif
                    break;

                default:
                    break;
            }
        }
        break;

#ifdef _PRE_WLAN_FEATURE_WMMAC
        case MAC_ACTION_CATEGORY_WMMAC_QOS:
        {
            if(OAL_TRUE == g_en_wmmac_switch_etc)
            {
                switch(puc_data[MAC_ACTION_OFFSET_ACTION])
                {
                    case MAC_WMMAC_ACTION_ADDTS_RSP:
                        hmac_mgmt_rx_addts_rsp_etc(pst_hmac_vap, pst_hmac_user, puc_data);
                        break;
                    case MAC_WMMAC_ACTION_DELTS:
                        hmac_mgmt_rx_delts_etc(pst_hmac_vap, pst_hmac_user, puc_data);
                        break;
                    default:
                        break;
                }
            }
        }
        break;
#endif //_PRE_WLAN_FEATURE_WMMAC

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
        case MAC_ACTION_CATEGORY_SA_QUERY:
        {
            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_SA_QUERY_ACTION_REQUEST:
                    hmac_rx_sa_query_req_etc(pst_hmac_vap, pst_netbuf, en_is_protected);
                    break;
                case MAC_SA_QUERY_ACTION_RESPONSE:
                    hmac_rx_sa_query_rsp_etc(pst_hmac_vap, pst_netbuf, en_is_protected);
                    break;

                default:
                    break;
            }
        }
        break;
#endif
        case MAC_ACTION_CATEGORY_VENDOR:
        {
#ifdef _PRE_WLAN_FEATURE_P2P
        /*查找OUI-OUI type值为 50 6F 9A - 09 (WFA P2P v1.0)  */
        /* 并用hmac_rx_mgmt_send_to_host接口上报 */
            if (OAL_TRUE == mac_ie_check_p2p_action_etc(puc_data + MAC_ACTION_CATEGORY_AND_CODE_LEN))
            {
               hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap, pst_netbuf);
            }
#endif
        }
        break;
#ifdef _PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R
        case MAC_ACTION_CATEGORY_FAST_BSS_TRANSITION:
        {
            if(OAL_TRUE != pst_hmac_vap->bit_11r_enable)
            {
               break;
            }
            hmac_roam_rx_ft_action_etc(pst_hmac_vap, pst_netbuf);
            break;
        }
#endif //_PRE_WLAN_FEATURE_11R
#endif //_PRE_WLAN_FEATURE_ROAM
        case MAC_ACTION_CATEGORY_VHT:
        {
            switch(puc_data[MAC_ACTION_OFFSET_ACTION])
            {
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
                case MAC_VHT_ACTION_OPREATING_MODE_NOTIFICATION:
                    hmac_mgmt_rx_opmode_notify_frame_etc(pst_hmac_vap, pst_hmac_user, pst_netbuf);
                    break;
#endif
                case MAC_VHT_ACTION_BUTT:
                default:
                    break;
            }
        }
        break;

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
        case MAC_ACTION_CATEGORY_RADIO_MEASURMENT:
        {
            if (OAL_FALSE == pst_hmac_vap->bit_11k_enable)
            {
                break;
            }

            switch (puc_data[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_RM_ACTION_RADIO_MEASUREMENT_REQUEST:
                    hmac_rrm_proc_rm_request(pst_hmac_vap, pst_hmac_user, pst_netbuf);
                    break;

                case MAC_RM_ACTION_NEIGHBOR_REPORT_REQUEST:
                    hmac_rrm_proc_neighbor_request(pst_hmac_vap, pst_hmac_user, pst_netbuf);
                    break;

                default:
                    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_sta_up_rx_action::action code[%d] invalid.}",
                        puc_data[MAC_ACTION_OFFSET_ACTION]);
                    break;
            }
        }
        break;
#endif //_PRE_WLAN_FEATURE_11K

        default:
        break;
    }
}


oal_uint32  hmac_sta_up_rx_mgmt_etc(hmac_vap_stru *pst_hmac_vap_sta, oal_void *p_param)
{
    dmac_wlan_crx_event_stru   *pst_mgmt_rx_event;
    dmac_rx_ctl_stru           *pst_rx_ctrl;
    mac_rx_ctl_stru            *pst_rx_info;
    oal_uint8                  *puc_mac_hdr;
    oal_uint8                   uc_mgmt_frm_type;
    oal_bool_enum_uint8         en_is_protected = OAL_FALSE;

    if (OAL_PTR_NULL == pst_hmac_vap_sta || OAL_PTR_NULL == p_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{hmac_sta_up_rx_mgmt_etc::param null,%d %d.}", pst_hmac_vap_sta, p_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mgmt_rx_event   = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_mgmt_rx_event->pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    puc_mac_hdr         = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_info);
    //en_is_protected     = (pst_rx_ctrl->st_rx_status.bit_cipher_protocol_type == hal_cipher_suite_to_ctype(WLAN_80211_CIPHER_SUITE_CCMP)) ? OAL_TRUE : OAL_FALSE;
    en_is_protected     = mac_is_protectedframe(puc_mac_hdr);
    if(en_is_protected >= OAL_BUTT)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RX, "{hmac_sta_up_rx_mgmt_etc::en_is_protected is %d.}", en_is_protected);
        return OAL_SUCC;
    }

    /* STA在UP状态下 接收到的各种管理帧处理 */
    uc_mgmt_frm_type = mac_get_frame_sub_type(puc_mac_hdr);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
        if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
        {
            wlan_pm_wkup_src_debug_set(OAL_FALSE);
            OAM_WARNING_LOG1(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_sta_up_rx_mgmt_etc::wakeup mgmt type[0x%x]}",uc_mgmt_frm_type);
        }
#endif
#endif

    /*Bar frame proc here*/
    if (WLAN_FC0_TYPE_CTL == mac_get_frame_type(puc_mac_hdr))
    {
        uc_mgmt_frm_type = mac_get_frame_sub_type(puc_mac_hdr);
        if (WLAN_FC0_SUBTYPE_BAR == uc_mgmt_frm_type)
        {
            hmac_up_rx_bar_etc(pst_hmac_vap_sta, pst_rx_ctrl, pst_mgmt_rx_event->pst_netbuf);
        }
    }
    else if(WLAN_FC0_TYPE_MGT == mac_get_frame_type(puc_mac_hdr))
    {
        switch (uc_mgmt_frm_type)
        {
            case WLAN_FC0_SUBTYPE_DEAUTH:
            case WLAN_FC0_SUBTYPE_DISASSOC:
                hmac_sta_rx_deauth_req(pst_hmac_vap_sta, puc_mac_hdr, en_is_protected);
                break;

            case WLAN_FC0_SUBTYPE_BEACON:
                hmac_sta_up_rx_beacon(pst_hmac_vap_sta, pst_mgmt_rx_event->pst_netbuf);
                break;

            case WLAN_FC0_SUBTYPE_ACTION:
                hmac_sta_up_rx_action(pst_hmac_vap_sta, pst_mgmt_rx_event->pst_netbuf, en_is_protected);
                break;
            default:
                break;
        }
    }

    return OAL_SUCC;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_check_capability_mac_phy_supplicant_etc);
/*lint +e578*//*lint +e19*/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

