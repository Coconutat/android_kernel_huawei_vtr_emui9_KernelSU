


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_ie.h"
#include "mac_regdomain.h"
#include "mac_device.h"
#include "hmac_mgmt_sta.h"
#include "hmac_sme_sta.h"
#include "hmac_fsm.h"
#include "hmac_dfs.h"
#include "hmac_chan_mgmt.h"
#include "mac_device.h"
#include "hmac_scan.h"
#include "frw_ext_if.h"
#include "hmac_resource.h"
#include "hmac_encap_frame.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CHAN_MGMT_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#define HMAC_CENTER_FREQ_2G_40M_OFFSET    2   /* 中心频点相对于主信道idx的偏移量 */
#define HMAC_AFFECTED_CH_IDX_OFFSET       5   /* 2.4GHz下，40MHz带宽所影响的信道半径，中心频点 +/- 5个信道 */


/*****************************************************************************
  3 函数声明
*****************************************************************************/

/*****************************************************************************
  4 函数实现
*****************************************************************************/

oal_uint32 hmac_dump_chan_etc(mac_vap_stru *pst_mac_vap, oal_uint8* puc_param)
{
    dmac_set_chan_stru *pst_chan;

    if ( (OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param) )
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_chan = (dmac_set_chan_stru*)puc_param;
    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "channel mgmt info\r\n");
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "uc_chan_number=%d\r\n", pst_chan->st_channel.uc_chan_number);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_band=%d\r\n", pst_chan->st_channel.en_band);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_bandwidth=%d\r\n", pst_chan->st_channel.en_bandwidth);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "uc_idx=%d\r\n", pst_chan->st_channel.uc_chan_idx);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "c_announced_channel=%d\r\n", pst_chan->st_ch_switch_info.uc_announced_channel);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_announced_bandwidth=%d\r\n", pst_chan->st_ch_switch_info.en_announced_bandwidth);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "uc_ch_switch_cnt=%d\r\n", pst_chan->st_ch_switch_info.uc_ch_switch_cnt);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_ch_switch_status=%d\r\n", pst_chan->st_ch_switch_info.en_ch_switch_status);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_bw_switch_status=%d\r\n", pst_chan->st_ch_switch_info.en_bw_switch_status);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_csa_present_in_bcn=%d\r\n", pst_chan->st_ch_switch_info.en_csa_present_in_bcn);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "uc_start_chan_idx=%d\r\n", pst_chan->st_ch_switch_info.uc_start_chan_idx);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "uc_end_chan_idx=%d\r\n", pst_chan->st_ch_switch_info.uc_end_chan_idx);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_user_pref_bandwidth=%d\r\n", pst_chan->st_ch_switch_info.en_user_pref_bandwidth);

    //OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_bw_change=%d\r\n", pst_chan->st_ch_switch_info.en_bw_change);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "uc_new_channel=%d\r\n", pst_chan->st_ch_switch_info.uc_new_channel);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_new_bandwidth=%d\r\n", pst_chan->st_ch_switch_info.en_new_bandwidth);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_waiting_to_shift_channel=%d\r\n",
                pst_chan->st_ch_switch_info.en_waiting_to_shift_channel);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_channel_swt_cnt_zero=%d\r\n", pst_chan->st_ch_switch_info.en_channel_swt_cnt_zero);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "en_te_b=%d\r\n",pst_chan->st_ch_switch_info.en_te_b);
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "ul_chan_report_for_te_a=%d\r\n", pst_chan->st_ch_switch_info.ul_chan_report_for_te_a);

    return OAL_SUCC;
}


oal_void  hmac_chan_initiate_switch_to_new_channel(mac_vap_stru *pst_mac_vap, oal_uint8 uc_channel, wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    frw_event_mem_stru            *pst_event_mem;
    frw_event_stru                *pst_event;
    oal_uint32                     ul_ret;
    dmac_set_ch_switch_info_stru  *pst_ch_switch_info;
    mac_device_stru               *pst_mac_device;

    /* AP准备切换信道 */
    pst_mac_vap->st_ch_switch_info.en_ch_switch_status    = WLAN_CH_SWITCH_STATUS_1;
    pst_mac_vap->st_ch_switch_info.uc_announced_channel   = uc_channel;
    pst_mac_vap->st_ch_switch_info.en_announced_bandwidth = en_bandwidth;

    /* 在Beacon帧中添加Channel Switch Announcement IE */
    pst_mac_vap->st_ch_switch_info.en_csa_present_in_bcn  = OAL_TRUE;

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "{hmac_chan_initiate_switch_to_new_channel::uc_announced_channel=%d,en_announced_bandwidth=%d csa_cnt=%d}",
        uc_channel, en_bandwidth,pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_chan_initiate_switch_to_new_channel::pst_mac_device null.}");
        return;
    }
    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_set_ch_switch_info_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_chan_initiate_switch_to_new_channel::pst_event_mem null.}");
        return;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    DMAC_WLAN_CTX_EVENT_SUB_TYPE_SWITCH_TO_NEW_CHAN,
                    OAL_SIZEOF(dmac_set_ch_switch_info_stru),
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    /* 填写事件payload */
    pst_ch_switch_info = (dmac_set_ch_switch_info_stru *)pst_event->auc_event_data;
    pst_ch_switch_info->en_ch_switch_status    = WLAN_CH_SWITCH_STATUS_1;
    pst_ch_switch_info->uc_announced_channel   = uc_channel;
    pst_ch_switch_info->en_announced_bandwidth = en_bandwidth;
    pst_ch_switch_info->uc_ch_switch_cnt       = pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt;
    pst_ch_switch_info->en_csa_present_in_bcn  = OAL_TRUE;
    pst_ch_switch_info->uc_csa_vap_cnt         = pst_mac_device->uc_csa_vap_cnt;

    pst_ch_switch_info->en_csa_mode            = pst_mac_vap->st_ch_switch_info.en_csa_mode;

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_chan_initiate_switch_to_new_channel::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return;
    }

    /* 释放事件 */
    FRW_EVENT_FREE(pst_event_mem);
}


#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

oal_uint32 hmac_check_ap_channel_follow_sta(mac_vap_stru *pst_check_mac_vap,const mac_channel_stru *pst_set_mac_channel,oal_uint8 *puc_ap_follow_channel)
{
    mac_device_stru   *pst_mac_device = OAL_PTR_NULL;
    oal_uint8       uc_vap_idx;
    mac_vap_stru   *pst_index_mac_vap = OAL_PTR_NULL;

    if(OAL_PTR_NULL == pst_set_mac_channel || OAL_PTR_NULL == puc_ap_follow_channel || OAL_PTR_NULL == pst_check_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_2040,"{hmac_check_ap_channel_follow_sta:: input param is null,return}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(!IS_LEGACY_VAP(pst_check_mac_vap))
    {
        OAM_WARNING_LOG1(pst_check_mac_vap->uc_vap_id, OAM_SF_2040,
               "{hmac_check_ap_channel_follow_sta:: check_vap_p2p_mode=%d, not neet to check}",pst_check_mac_vap->en_p2p_mode);
        return OAL_FAIL;
    }

    if(!IS_STA(pst_check_mac_vap) && !IS_AP(pst_check_mac_vap))
    {
        OAM_ERROR_LOG1(pst_check_mac_vap->uc_vap_id, OAM_SF_2040,
            "{hmac_check_ap_channel_follow_sta:: pst_mac_vap_current->en_vap_mode=%d,not neet to check}\n",pst_check_mac_vap->en_vap_mode);
        return OAL_FAIL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_check_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_check_mac_vap->uc_vap_id, OAM_SF_2040,"{hmac_check_ap_channel_follow_sta::get mac_device(%d) is null. Return}",pst_check_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG3(pst_check_mac_vap->uc_vap_id, OAM_SF_2040,
           "{hmac_check_ap_channel_follow_sta:: check_vap_state=%d, check_vap_band=%d  check_vap_channel=%d.}",
           pst_check_mac_vap->en_vap_state,pst_set_mac_channel->en_band, pst_set_mac_channel->uc_chan_number);

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_index_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);

        if ((OAL_PTR_NULL == pst_index_mac_vap) || (pst_check_mac_vap->uc_vap_id == pst_index_mac_vap->uc_vap_id) || (WLAN_LEGACY_VAP_MODE != pst_index_mac_vap->en_p2p_mode))
        {
            continue;
        }
        if((MAC_VAP_STATE_UP != pst_index_mac_vap->en_vap_state) && (MAC_VAP_STATE_PAUSE != pst_index_mac_vap->en_vap_state))
        {
            continue;
        }

        OAM_WARNING_LOG4(pst_index_mac_vap->uc_vap_id, OAM_SF_2040,
               "{hmac_check_ap_channel_follow_sta::index_vap_state=%d,index_vap_mode=%d,index_vap_band=%d ,index_vap_channel=%d.}",
               pst_index_mac_vap->en_vap_state,pst_check_mac_vap->en_vap_mode,pst_index_mac_vap->st_channel.en_band, pst_index_mac_vap->st_channel.uc_chan_number);

        if(IS_STA(pst_check_mac_vap) && IS_AP(pst_index_mac_vap))
        {/*AP先启动;STA后启动*/
            if((pst_set_mac_channel->en_band == pst_index_mac_vap->st_channel.en_band) &&
                (pst_set_mac_channel->uc_chan_number != pst_index_mac_vap->st_channel.uc_chan_number))
            {/*CSA*/
                OAM_WARNING_LOG2(pst_index_mac_vap->uc_vap_id, OAM_SF_2040,"{hmac_check_ap_channel_follow_sta::<vap_current_mode=STA vap_index_mode=Ap> SoftAp CSA Operate, Channel from [%d] To [%d]}.\n",
                    pst_index_mac_vap->st_channel.uc_chan_number,pst_set_mac_channel->uc_chan_number);
                pst_mac_device->uc_csa_vap_cnt++;
                pst_index_mac_vap->st_ch_switch_info.uc_ch_switch_cnt = HMAC_CHANNEL_SWITCH_COUNT;/*CSA cnt 设置为5*/
                pst_index_mac_vap->st_ch_switch_info.en_csa_mode = WLAN_CSA_MODE_TX_DISABLE;
                hmac_chan_initiate_switch_to_new_channel(pst_index_mac_vap,pst_set_mac_channel->uc_chan_number,pst_index_mac_vap->st_channel.en_bandwidth);
                *puc_ap_follow_channel = pst_set_mac_channel->uc_chan_number;
                return OAL_SUCC;
            }
        }
        else if(IS_AP(pst_check_mac_vap) && IS_STA(pst_index_mac_vap))
        {/*STA先启动;AP后启动*/
            if((pst_set_mac_channel->en_band == pst_index_mac_vap->st_channel.en_band) &&
                (pst_set_mac_channel->uc_chan_number != pst_index_mac_vap->st_channel.uc_chan_number))
            {/*替换信道值*/
                *puc_ap_follow_channel = pst_index_mac_vap->st_channel.uc_chan_number;
                OAM_WARNING_LOG2(pst_index_mac_vap->uc_vap_id, OAM_SF_2040,"{hmac_check_ap_channel_follow_sta::<vap_current_mode=Ap vap_index_mode=Sta> SoftAp change Channel from [%d] To [%d]}.\n",pst_set_mac_channel->uc_chan_number,*puc_ap_follow_channel);
                return OAL_SUCC;
            }
        }
    }

    return OAL_FAIL;
}
#endif




#ifdef _PRE_WLAN_FEATURE_DFS
oal_void hmac_dfs_set_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_channel)
{
    oal_uint8          uc_vap_idx;
    mac_device_stru   *pst_mac_device;
    mac_vap_stru      *pst_vap;
    oal_uint8          uc_chan_idx;
    oal_uint32         ul_ret;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_dfs_set_channel_etc::pst_device(%d) null.}", pst_mac_vap->uc_device_id);
        return;
    }

    if (0 == pst_mac_device->uc_vap_num)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_dfs_set_channel_etc::none vap.}");
        return;
    }

    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, uc_channel, &uc_chan_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_dfs_set_channel_etc::mac_get_channel_idx_from_num_etc failed[%d].}", ul_ret);

        return;
    }

    pst_mac_device->uc_csa_vap_cnt = 0;

    /* 遍历device下所有ap，设置ap信道参数，发送CSA帧，准备切换至新信道运行 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_dfs_set_channel_etc::pst_ap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        /* 只有running AP需要发送CSA帧 */
        if ((WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode) &&
            (MAC_VAP_STATE_UP     == pst_vap->en_vap_state))
        {
            pst_mac_device->uc_csa_vap_cnt++;
            if(uc_channel == pst_vap->st_channel.uc_chan_number)
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_dfs_set_channel_etc::same channel num %d, no need to change.}", uc_channel);
                return;
            }
            pst_vap->st_ch_switch_info.en_csa_mode = WLAN_CSA_MODE_TX_DISABLE;
            hmac_chan_initiate_switch_to_new_channel(pst_vap, uc_channel, pst_vap->st_channel.en_bandwidth);
        }
    }
}
oal_void  hmac_chan_multi_switch_to_new_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_channel, wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    oal_uint8          uc_vap_idx;
    mac_device_stru   *pst_mac_device;
    mac_vap_stru      *pst_vap;
    oal_uint8          uc_chan_idx;
    oal_uint32         ul_ret;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "{hmac_chan_multi_switch_to_new_channel_etc::uc_channel=%d,en_bandwidth=%d}",
        uc_channel, en_bandwidth);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_multi_switch_to_new_channel_etc::pst_device(%d) null.}", pst_mac_vap->uc_device_id);
        return;
    }

    if (0 == pst_mac_device->uc_vap_num)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_multi_switch_to_new_channel_etc::none vap.}");
        return;
    }

    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, uc_channel, &uc_chan_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_multi_switch_to_new_channel_etc::mac_get_channel_idx_from_num_etc failed[%d].}", ul_ret);

        return;
    }

    pst_mac_device->uc_csa_vap_cnt = 0;

    /* 遍历device下所有ap，设置ap信道参数，发送CSA帧，准备切换至新信道运行 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_multi_switch_to_new_channel_etc::pst_ap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        /* 只有running AP需要发送CSA帧 */
        if ((WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode) &&
            (MAC_VAP_STATE_UP     == pst_vap->en_vap_state))
        {
            pst_mac_device->uc_csa_vap_cnt++;
            pst_vap->st_ch_switch_info.en_csa_mode = WLAN_CSA_MODE_TX_DISABLE;
            hmac_chan_initiate_switch_to_new_channel(pst_vap, uc_channel, en_bandwidth);
        }
        else  /* 其它站点只需要更新信道信息 */
        {
            /* 更新VAP下的主20MHz信道号、带宽模式、信道索引 */
            pst_vap->st_channel.uc_chan_number = uc_channel;
            pst_vap->st_channel.uc_chan_idx         = uc_chan_idx;
            pst_vap->st_channel.en_bandwidth   = en_bandwidth;
        }
    }
}

#else

oal_void  hmac_chan_multi_switch_to_new_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_channel, wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    oal_uint8          uc_vap_idx;
    mac_device_stru   *pst_device;
    mac_vap_stru      *pst_ap;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040,
        "{hmac_chan_multi_switch_to_new_channel_etc::uc_channel=%d,en_bandwidth=%d}",
        uc_channel, en_bandwidth);

    pst_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_chan_multi_switch_to_new_channel_etc::pst_device null.}");
        return;
    }

    if (0 == pst_device->uc_vap_num)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_chan_multi_switch_to_new_channel_etc::none vap.}");
        return;
    }

    /* 遍历device下所有ap，设置ap信道参数，准备切换至新信道运行 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
    {
        pst_ap = (mac_vap_stru *)mac_res_get_mac_vap(pst_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_ap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_chan_multi_switch_to_new_channel_etc::pst_ap null.}",
                           pst_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        /* 只更新AP侧的信道切换信息 */
        if (WLAN_VAP_MODE_BSS_AP != pst_ap->en_vap_mode)
        {
            continue;
        }

        pst_ap->st_ch_switch_info.en_csa_mode = WLAN_CSA_MODE_TX_DISABLE;
        hmac_chan_initiate_switch_to_new_channel(pst_ap, uc_channel, en_bandwidth);
    }
}

#endif /* end of _PRE_WLAN_FEATURE_DFS */

oal_void  hmac_chan_sync_init_etc(mac_vap_stru *pst_mac_vap, dmac_set_chan_stru *pst_set_chan)
{
    oal_memset(pst_set_chan, 0, OAL_SIZEOF(dmac_set_chan_stru));
    oal_memcopy(&pst_set_chan->st_channel, &pst_mac_vap->st_channel,
                    OAL_SIZEOF(mac_channel_stru));
    oal_memcopy(&pst_set_chan->st_ch_switch_info, &pst_mac_vap->st_ch_switch_info,
                    OAL_SIZEOF(mac_ch_switch_info_stru));
}


oal_void  hmac_chan_do_sync_etc(mac_vap_stru *pst_mac_vap, dmac_set_chan_stru *pst_set_chan)
{
    frw_event_mem_stru       *pst_event_mem;
    frw_event_stru           *pst_event;
    oal_uint32                ul_ret;
    oal_uint8                 uc_idx;

    hmac_dump_chan_etc(pst_mac_vap, (oal_uint8*)pst_set_chan);
    /* 更新VAP下的主20MHz信道号、带宽模式、信道索引 */
    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, pst_set_chan->st_channel.uc_chan_number, &uc_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_chan_sync_etc::mac_get_channel_idx_from_num_etc failed[%d].}", ul_ret);

        return;
    }

    pst_mac_vap->st_channel.uc_chan_number = pst_set_chan->st_channel.uc_chan_number;
    pst_mac_vap->st_channel.en_bandwidth   = pst_set_chan->st_channel.en_bandwidth;
    pst_mac_vap->st_channel.uc_chan_idx         = uc_idx;


    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_set_chan_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_chan_sync_etc::pst_event_mem null.}");
        return;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    DMAC_WALN_CTX_EVENT_SUB_TYPR_SELECT_CHAN,
                    OAL_SIZEOF(dmac_set_chan_stru),
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    oal_memcopy(frw_get_event_payload(pst_event_mem), (oal_uint8 *)pst_set_chan, OAL_SIZEOF(dmac_set_chan_stru));

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_chan_sync_etc::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return;
    }

    /* 释放事件 */
    FRW_EVENT_FREE(pst_event_mem);
}



oal_void hmac_chan_sync_etc(mac_vap_stru *pst_mac_vap,
            oal_uint8 uc_channel, wlan_channel_bandwidth_enum_uint8 en_bandwidth,
            oal_bool_enum_uint8 en_switch_immediately)
{
    dmac_set_chan_stru st_set_chan;

    hmac_chan_sync_init_etc(pst_mac_vap, &st_set_chan);
    st_set_chan.st_channel.uc_chan_number = uc_channel;
    st_set_chan.st_channel.en_bandwidth = en_bandwidth;
    st_set_chan.en_switch_immediately = en_switch_immediately;
    st_set_chan.en_dot11FortyMHzIntolerant = mac_mib_get_FortyMHzIntolerant(pst_mac_vap);
    hmac_chan_do_sync_etc(pst_mac_vap, &st_set_chan);
}



oal_void  hmac_chan_multi_select_channel_mac_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_channel, wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    oal_uint8          uc_vap_idx;
    mac_device_stru   *pst_device;
    mac_vap_stru      *pst_vap;

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_multi_select_channel_mac_etc::uc_channel=%d,en_bandwidth=%d}",
            uc_channel, en_bandwidth);

    pst_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                       "{hmac_chan_multi_select_channel_mac_etc::pst_device null,device_id=%d.}", pst_mac_vap->uc_device_id);
        return;
    }

    if (0 == pst_device->uc_vap_num)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_multi_select_channel_mac_etc::none vap.}");
        return;
    }

    if (mac_is_dbac_running(pst_device))
    {
        hmac_chan_sync_etc(pst_mac_vap, uc_channel, en_bandwidth, OAL_TRUE);
        return;
    }

    /* 遍历device下所有vap， */
    for (uc_vap_idx = 0; uc_vap_idx < pst_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                           "{hmac_chan_multi_select_channel_mac_etc::pst_vap null,vap_id=%d.}",
                           pst_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        hmac_chan_sync_etc(pst_vap, uc_channel, en_bandwidth, OAL_TRUE);
    }
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST


oal_void hmac_chan_update_40M_intol_user_etc(mac_vap_stru *pst_mac_vap)
{
    oal_dlist_head_stru     *pst_entry;
    mac_user_stru           *pst_mac_user;

    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_mac_user = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_update_40M_intol_user_etc::pst_user null pointer.}");
            continue;
        }
        else
        {
            if(pst_mac_user->st_ht_hdl.bit_forty_mhz_intolerant)
            {
                pst_mac_vap->en_40M_intol_user = OAL_TRUE;
                return;
            }
        }
    }

    pst_mac_vap->en_40M_intol_user = OAL_FALSE;
}

#endif


OAL_STATIC OAL_INLINE oal_uint8  hmac_chan_get_user_pref_primary_ch(mac_device_stru *pst_mac_device)
{
    return pst_mac_device->uc_max_channel;
}


OAL_STATIC OAL_INLINE wlan_channel_bandwidth_enum_uint8  hmac_chan_get_user_pref_bandwidth(mac_vap_stru *pst_mac_vap)
{
    return pst_mac_vap->st_ch_switch_info.en_user_pref_bandwidth;
}


oal_void  hmac_chan_reval_bandwidth_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint32 ul_change)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_chan_reval_bandwidth_sta_etc::pst_mac_vap null.}");
        return;
    }

    /* 需要进行带宽切换 */
    if (MAC_BW_CHANGE & ul_change)
    {
        hmac_chan_multi_select_channel_mac_etc(pst_mac_vap, pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_channel.en_bandwidth);
    }
}



OAL_STATIC oal_void  hmac_chan_ctrl_machw_tx(mac_vap_stru *pst_mac_vap, oal_uint8 uc_sub_type)
{
    frw_event_mem_stru       *pst_event_mem;
    frw_event_stru           *pst_event;
    oal_uint32                ul_ret;

    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_chan_ctrl_machw_tx::pst_event_mem null.}");
        return;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    uc_sub_type,
                    0,
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_chan_ctrl_machw_tx::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return;
    }

    /* 释放事件 */
    FRW_EVENT_FREE(pst_event_mem);
}


oal_void  hmac_chan_disable_machw_tx_etc(mac_vap_stru *pst_mac_vap)
{
    hmac_chan_ctrl_machw_tx(pst_mac_vap, DMAC_WALN_CTX_EVENT_SUB_TYPR_DISABLE_TX);
}


oal_void  hmac_chan_enable_machw_tx_etc(mac_vap_stru *pst_mac_vap)
{
    hmac_chan_ctrl_machw_tx(pst_mac_vap, DMAC_WALN_CTX_EVENT_SUB_TYPR_ENABLE_TX);
}

#if defined(_PRE_WLAN_FEATURE_DFS) && (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)

OAL_STATIC oal_void  hmac_cac_chan_ctrl_machw_tx_event(mac_vap_stru *pst_mac_vap, oal_uint8 uc_cac_machw_en)
{
    frw_event_mem_stru       *pst_event_mem;
    frw_event_stru           *pst_event;
    oal_uint32                ul_ret;
    dmac_set_cac_machw_info_stru  *pst_cac_machw_info;

    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_set_cac_machw_info_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_cac_chan_ctrl_machw_tx::pst_event_mem null.}");
        return;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    DMAC_WALN_CTX_EVENT_SUB_TYPR_DFS_CAC_CTRL_TX,
                    OAL_SIZEOF(dmac_set_cac_machw_info_stru),
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    /* 填写事件payload */
    pst_cac_machw_info = (dmac_set_cac_machw_info_stru *)pst_event->auc_event_data;
    pst_cac_machw_info->uc_cac_machw_en = uc_cac_machw_en;

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_cac_chan_ctrl_machw_tx::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return;
    }

    /* 释放事件 */
    FRW_EVENT_FREE(pst_event_mem);
}


oal_void  hmac_cac_chan_ctrl_machw_tx(mac_vap_stru *pst_mac_vap, oal_uint8 uc_cac_machw_en)
{
    if(OAL_FALSE == uc_cac_machw_en)
    {
        mac_vap_state_change_etc(pst_mac_vap, MAC_VAP_STATE_PAUSE);
    }
    else if(OAL_TRUE == uc_cac_machw_en)
    {
        mac_vap_state_change_etc(pst_mac_vap, MAC_VAP_STATE_UP);
    }
    else
    {
        return;
    }
    
    hmac_cac_chan_ctrl_machw_tx_event(pst_mac_vap, uc_cac_machw_en);
}
#endif


OAL_STATIC OAL_INLINE oal_bool_enum_uint8  hmac_chan_check_channnel_avaible(
                wlan_channel_band_enum_uint8    en_band,
                oal_uint8                      *puc_start_ch_idx,
                oal_uint8                      *puc_end_ch_idx)
{
    oal_int32    l_ch_idx;
    oal_uint8    uc_num_supp_chan = mac_get_num_supp_channel(en_band);
    oal_uint32   ul_ret;

    /* 取低有效信道 */
    for (l_ch_idx = *puc_start_ch_idx; l_ch_idx < uc_num_supp_chan; l_ch_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(en_band, (oal_uint8)l_ch_idx);
        if (OAL_SUCC == ul_ret)
        {
            *puc_start_ch_idx = (oal_uint8)l_ch_idx;
            break;
        }
    }

    if (l_ch_idx == uc_num_supp_chan)
    {
        return OAL_FALSE;
    }

    /* 取高有效信道 */
    for (l_ch_idx = *puc_end_ch_idx; l_ch_idx >= 0; l_ch_idx--)
    {
        ul_ret = mac_is_channel_idx_valid_etc(en_band, (oal_uint8)l_ch_idx);
        if (OAL_SUCC == ul_ret)
        {
            *puc_end_ch_idx = (oal_uint8)l_ch_idx;
            break;
        }
    }

    if (l_ch_idx < 0)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


oal_uint32  hmac_start_bss_in_available_channel_etc(hmac_vap_stru *pst_hmac_vap)
{
    hmac_ap_start_rsp_stru          st_ap_start_rsp;
    oal_uint32                      ul_ret;

    mac_vap_init_rates_etc(&(pst_hmac_vap->st_vap_base_info));

    /* 设置AP侧状态机为 UP */
    hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_UP);

    /* 调用hmac_config_start_vap_event，启动BSS */
    ul_ret = hmac_config_start_vap_event_etc(&(pst_hmac_vap->st_vap_base_info), OAL_TRUE);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_INIT);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                        "{hmac_start_bss_in_available_channel_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 设置bssid */
    mac_vap_set_bssid_etc(&pst_hmac_vap->st_vap_base_info,  mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info));

    /* 入网优化，不同频段下的能力不一样 */
    if (WLAN_BAND_2G == pst_hmac_vap->st_vap_base_info.st_channel.en_band)
    {
//        mac_mib_set_ShortPreambleOptionImplemented(&(pst_hmac_vap->st_vap_base_info), WLAN_LEGACY_11B_MIB_SHORT_PREAMBLE);
        mac_mib_set_SpectrumManagementRequired(&(pst_hmac_vap->st_vap_base_info), OAL_FALSE);
    }
    else
    {
//        mac_mib_set_ShortPreambleOptionImplemented(&(pst_hmac_vap->st_vap_base_info), WLAN_LEGACY_11B_MIB_LONG_PREAMBLE);
        mac_mib_set_SpectrumManagementRequired(&(pst_hmac_vap->st_vap_base_info), OAL_TRUE);
    }

    /* 将结果上报至sme */
    st_ap_start_rsp.en_result_code = HMAC_MGMT_SUCCESS;
    hmac_send_rsp_to_sme_ap_etc(pst_hmac_vap, HMAC_AP_SME_START_RSP, (oal_uint8 *)&st_ap_start_rsp);

    return OAL_SUCC;
}


oal_uint32  hmac_chan_start_bss_etc(hmac_vap_stru *pst_hmac_vap, mac_channel_stru *pst_channel, wlan_protocol_enum_uint8 en_protocol)
{
    oal_uint32  ul_ret;

    //同步信道和模式
    ul_ret = hmac_sta_sync_vap(pst_hmac_vap, pst_channel, en_protocol);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN, "{hmac_chan_start_bss_etc::hmac_sta_sync_vap failed[%d].}", ul_ret);
        return ul_ret;
    }

    //启动vap
    return hmac_start_bss_in_available_channel_etc(pst_hmac_vap);
}


oal_uint32  hmac_chan_restart_network_after_switch_etc(mac_vap_stru *pst_mac_vap)
{
    frw_event_mem_stru   *pst_event_mem;
    frw_event_stru       *pst_event;
    oal_uint32            ul_ret;

    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_chan_restart_network_after_switch_etc}");

    /* 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_chan_restart_network_after_switch_etc::pst_event_mem null.}");

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    DMAC_WLAN_CTX_EVENT_SUB_TYPR_RESTART_NETWORK,
                    0,
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                       "{hmac_chan_restart_network_after_switch_etc::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
        FRW_EVENT_FREE(pst_event_mem);

        return ul_ret;
    }
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32  hmac_chan_switch_to_new_chan_complete_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru     *pst_event;
    mac_device_stru    *pst_mac_device;
    hmac_vap_stru      *pst_hmac_vap;
    mac_vap_stru       *pst_mac_vap;
    dmac_set_chan_stru *pst_set_chan;
    oal_uint32          ul_ret;
    oal_uint8           uc_idx;
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    oal_uint8 uc_ap_follow_channel = 0;
#endif

    OAM_INFO_LOG0(0, OAM_SF_2040, "{hmac_chan_restart_network_after_switch_etc}");

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_2040, "{hmac_switch_to_new_chan_complete::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
	pst_set_chan = (dmac_set_chan_stru *)pst_event->auc_event_data;
    OAM_INFO_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_2040, "hmac_chan_switch_to_new_chan_complete_etc\r\n");

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_2040, "{hmac_switch_to_new_chan_complete::pst_hmac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &pst_hmac_vap->st_vap_base_info;

    OAM_INFO_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_2040, "hmac_chan_switch_to_new_chan_complete_etc");
    hmac_dump_chan_etc(pst_mac_vap, (oal_uint8*)pst_set_chan);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_2040, "{hmac_switch_to_new_chan_complete::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band,
                pst_set_chan->st_channel.uc_chan_number, &uc_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                "{hmac_switch_to_new_chan_complete::mac_get_channel_idx_from_num_etc failed[%d].}", ul_ret);

        return OAL_FAIL;
    }

    pst_mac_vap->st_channel.uc_chan_number = pst_set_chan->st_channel.uc_chan_number;
    pst_mac_vap->st_channel.en_bandwidth   = pst_set_chan->st_channel.en_bandwidth;
    pst_mac_vap->st_channel.uc_chan_idx         = uc_idx;


    //pst_mac_vap->st_ch_switch_info.en_waiting_for_ap = pst_set_chan->st_ch_switch_info.en_waiting_for_ap;
    pst_mac_vap->st_ch_switch_info.en_waiting_to_shift_channel = pst_set_chan->st_ch_switch_info.en_waiting_to_shift_channel;

    pst_mac_vap->st_ch_switch_info.en_ch_switch_status = pst_set_chan->st_ch_switch_info.en_ch_switch_status;
    pst_mac_vap->st_ch_switch_info.en_bw_switch_status = pst_set_chan->st_ch_switch_info.en_bw_switch_status;

    /* aput切完信道同步切信道的标志位,防止再有用户关联,把此变量又同步下去 */
    pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt   = pst_set_chan->st_ch_switch_info.uc_ch_switch_cnt;
    pst_mac_vap->st_ch_switch_info.en_csa_present_in_bcn = pst_set_chan->st_ch_switch_info.en_csa_present_in_bcn;

    /*同步device信息*/
    pst_mac_device->uc_max_channel   = pst_mac_vap->st_channel.uc_chan_number;
    pst_mac_device->en_max_band      = pst_mac_vap->st_channel.en_band;
    pst_mac_device->en_max_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /*信道跟随检查*/
    if(IS_STA(pst_mac_vap))
    {
        ul_ret = hmac_check_ap_channel_follow_sta(pst_mac_vap,&pst_mac_vap->st_channel,&uc_ap_follow_channel);
        if(OAL_SUCC == ul_ret)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_chan_switch_to_new_chan_complete_etc::after hmac_check_ap_channel_follow_sta ap channel change from %d to %d}",
                pst_mac_vap->st_channel.uc_chan_number,uc_ap_follow_channel);
        }
    }
#endif

    if (OAL_FALSE == pst_set_chan->en_check_cac)
    {
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_DFS
    if (OAL_TRUE == hmac_dfs_need_for_cac(pst_mac_device, &pst_hmac_vap->st_vap_base_info))
    {
        hmac_dfs_cac_start_etc(pst_mac_device, pst_hmac_vap);
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_2040,
                "{hmac_chan_switch_to_new_chan_complete_etc::[DFS]CAC START!}");

        return OAL_SUCC;
    }

    hmac_chan_restart_network_after_switch_etc(&(pst_hmac_vap->st_vap_base_info));
#endif

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_DBAC

oal_uint32  hmac_dbac_status_notify_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru     *pst_event;
    mac_device_stru    *pst_mac_device;
    oal_bool_enum_uint8 *pen_dbac_enable;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_2040, "{hmac_dbac_status_notify_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
	pen_dbac_enable = (oal_bool_enum_uint8 *)pst_event->auc_event_data;
    OAM_INFO_LOG1(pst_event->st_event_hdr.uc_vap_id, OAM_SF_2040,
        "hmac_dbac_status_notify_etc::dbac switch to enable=%d", *pen_dbac_enable);

    pst_mac_device = mac_res_get_dev_etc(pst_event->st_event_hdr.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_2040,
            "{hmac_dbac_status_notify_etc::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device->en_dbac_running = *pen_dbac_enable;

    return OAL_SUCC;
}
#endif

oal_void hmac_40M_intol_sync_data(mac_vap_stru *pst_mac_vap, wlan_channel_bandwidth_enum_uint8 en_40M_bandwidth, oal_bool_enum_uint8 en_40M_intol_user)
{
    mac_bandwidth_stru           st_band_prot;

    OAL_MEMZERO(&st_band_prot, OAL_SIZEOF(mac_bandwidth_stru));

    st_band_prot.en_40M_bandwidth = en_40M_bandwidth;
    st_band_prot.en_40M_intol_user = en_40M_intol_user;
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    hmac_40M_intol_sync_event(pst_mac_vap, OAL_SIZEOF(st_band_prot),(oal_uint8*)&st_band_prot);
#endif
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

