


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_WMMAC

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "dmac_wmmac.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_WMMAC_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_uint32  dmac_mgmt_tx_addts_req(
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_ctx_action_event,
                oal_netbuf_stru                *pst_net_buff)
{
    oal_uint8                   uc_tidno;
    dmac_user_stru             *pst_dmac_user;
    //oal_uint16                  us_frame_len;
    mac_tx_ctl_stru            *pst_tx_ctl;
    oal_uint32                  ul_ret;
    oal_uint8                   uc_ac;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_ctx_action_event) || (OAL_PTR_NULL == pst_net_buff))
    {
        OAM_ERROR_LOG3(0, OAM_SF_WMMAC, "{dmac_mgmt_tx_addts_req::param null, pst_dmac_vap=%p pst_ctx_action_event=%p.pst_net_buff=%p}",
                       pst_dmac_vap, pst_ctx_action_event, pst_net_buff);
        oal_netbuf_free(pst_net_buff);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tidno                = pst_ctx_action_event->uc_tidno;
    //us_frame_len            = pst_ctx_action_event->us_frame_len;

    if (uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{dmac_mgmt_tx_addts_req::invalid uc_tidno[%d].}", uc_tidno);
        oal_netbuf_free(pst_net_buff);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    /* 获取对应用户 */
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_ctx_action_event->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{dmac_mgmt_tx_addts_req::get dmac user[%d] failed.}", pst_ctx_action_event->us_user_idx);
        oal_netbuf_free(pst_net_buff);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_ac = WLAN_WME_TID_TO_AC(uc_tidno);
    pst_dmac_user->st_user_base_info.st_ts_info[uc_ac].en_ts_status       = MAC_TS_INPROGRESS;
    pst_dmac_user->st_user_base_info.st_ts_info[uc_ac].en_direction       = pst_ctx_action_event->uc_initiator;
    pst_dmac_user->st_user_base_info.st_ts_info[uc_ac].uc_tsid            = pst_ctx_action_event->uc_tsid;
    pst_dmac_user->st_user_base_info.st_ts_info[uc_ac].uc_ts_dialog_token = pst_ctx_action_event->uc_ts_dialog_token;
    pst_dmac_user->st_user_base_info.st_ts_info[uc_ac].uc_up              = uc_tidno;
    pst_dmac_user->st_user_base_info.st_ts_info[uc_ac].us_mac_user_idx    = pst_dmac_user->st_user_base_info.us_assoc_id;
    pst_dmac_user->st_user_base_info.st_ts_info[uc_ac].ul_medium_time     = 0;

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_net_buff);

    oal_set_netbuf_next(pst_net_buff, OAL_PTR_NULL);
    oal_set_netbuf_prev(pst_net_buff, OAL_PTR_NULL);

    /* 填写netbuf的cb字段，共发送管理帧和发送完成接口使用 */
    MAC_GET_CB_IS_MCAST(pst_tx_ctl)        = OAL_FALSE;
    mac_set_cb_ac(pst_tx_ctl, uc_ac);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)     = (oal_uint8)pst_dmac_user->st_user_base_info.us_assoc_id;
    mac_set_cb_frame_hdr(pst_tx_ctl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buff));

    /* 调用发送管理帧接口 */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_net_buff, pst_ctx_action_event->us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_net_buff);
        return ul_ret;
    }

    return OAL_SUCC;
}

oal_uint32  dmac_mgmt_tx_addts_rsp(
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_ctx_action_event,
                oal_netbuf_stru                *pst_net_buff)
{
    oal_uint32 ul_ret = OAL_SUCC;
    return ul_ret;
}

oal_uint32  dmac_mgmt_tx_delts(
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_ctx_action_event_stru     *pst_ctx_action_event,
                oal_netbuf_stru                *pst_net_buff)
{
    oal_uint8                   uc_tidno;
    dmac_user_stru             *pst_dmac_user;
    //oal_uint16                  us_frame_len;
    mac_tx_ctl_stru            *pst_tx_ctl;
    oal_uint32                  ul_ret;
    oal_uint8                   uc_ac;
    oal_uint8                   uc_ac_idx;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_ctx_action_event) || (OAL_PTR_NULL == pst_net_buff))
    {
        OAM_ERROR_LOG3(0, OAM_SF_WMMAC, "{dmac_mgmt_tx_delts::param null, pst_dmac_vap=%d pst_ctx_action_event=%d, pst_net_buff=%d.}",
                       pst_dmac_vap, pst_ctx_action_event, pst_net_buff);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tidno                = pst_ctx_action_event->uc_tidno;
    //us_frame_len            = pst_ctx_action_event->us_frame_len;

    /* 获取对应用户 */
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_ctx_action_event->us_user_idx);
    if (OAL_PTR_NULL == pst_dmac_user)
    {
        OAM_ERROR_LOG0(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{dmac_mgmt_tx_delts::pst_dmac_user null.}");
        oal_netbuf_free(pst_net_buff);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_ERROR_LOG1(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{dmac_mgmt_tx_delts::invalid uc_tidno[%d].}", uc_tidno);
        oal_netbuf_free(pst_net_buff);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    uc_ac = WLAN_WME_TID_TO_AC(uc_tidno);
    if (IS_STA(&pst_dmac_vap->st_vap_base_info))
    {
        /* 遍历所有AC，找到对应TSID的AC */
        for (uc_ac_idx = 0; uc_ac_idx < WLAN_WME_AC_BUTT; uc_ac_idx++)
        {
            if(pst_ctx_action_event->uc_tsid == pst_dmac_user->st_user_base_info.st_ts_info[uc_ac_idx].uc_tsid)
            {
                /* TS状态为none，表示不需要建立TS，直接返回，否则均可发送delts，并需更改其状态 */
                if (pst_dmac_user->st_user_base_info.st_ts_info[uc_ac_idx].en_ts_status == MAC_TS_NONE)
                {
                    oal_netbuf_free(pst_net_buff);
                    return OAL_SUCC;
                }
                OAL_MEMZERO(&(pst_dmac_user->st_user_base_info.st_ts_info[uc_ac_idx]), OAL_SIZEOF(mac_ts_stru));
                pst_dmac_user->st_user_base_info.st_ts_info[uc_ac_idx].en_ts_status  = MAC_TS_INIT;
                break;
            }
        }
    }

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_net_buff);

    oal_set_netbuf_next(pst_net_buff, OAL_PTR_NULL);
    oal_set_netbuf_prev(pst_net_buff, OAL_PTR_NULL);
    /* 填写netbuf的cb字段，共发送管理帧和发送完成接口使用 */
    MAC_GET_CB_IS_MCAST(pst_tx_ctl)         = OAL_FALSE;
    mac_set_cb_ac(pst_tx_ctl, uc_ac);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)      = (oal_uint8)pst_dmac_user->st_user_base_info.us_assoc_id;
    mac_set_cb_frame_hdr(pst_tx_ctl, (mac_ieee80211_frame_stru *)oal_netbuf_header(pst_net_buff));

    /* 调用发送管理帧接口 */
    ul_ret = dmac_tx_mgmt(pst_dmac_vap, pst_net_buff, pst_ctx_action_event->us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_net_buff);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_mgmt_rx_addts_rsp(
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_user_stru                 *pst_dmac_user,
                oal_uint8                      *puc_payload)
{
    oal_uint8                   uc_tsid;
    oal_uint8                   uc_ac_num;
    oal_uint8                   uc_user_prio;
    oal_uint8                   uc_dialog_token;
    oal_uint8                   uc_status;
#ifdef _PRE_WLAN_FEATURE_UAPSD
    oal_uint8                   uc_uapsd_en;
#endif
    mac_ts_stru                *pst_ts;
    mac_wmm_tspec_stru         *pst_tspec_info;

    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == puc_payload))
    {
        OAM_ERROR_LOG3(0, OAM_SF_WMMAC,"{dmac_mgmt_rx_addts_rsp::param null, pst_dmac_vap=%p, pst_dmac_user=%p, pst_payload=%p.}", pst_dmac_vap, pst_dmac_user, puc_payload);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tspec_info = (mac_wmm_tspec_stru *)(puc_payload + 12);
    uc_user_prio = pst_tspec_info->ts_info.bit_user_prio;

    /* 协议支持tid为0~15,02只支持tid0~7 */
    if(uc_user_prio >= WLAN_TID_MAX_NUM)
    {
        /* 对于tid > 7的rsp直接忽略 */
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{dmac_mgmt_rx_addts_rsp::addts rsp tsid[%d] status[%d]} ", uc_user_prio, puc_payload[3]);
        return OAL_SUCC;
    }
    uc_ac_num = WLAN_WME_TID_TO_AC(uc_user_prio);
    uc_tsid = pst_tspec_info->ts_info.bit_tsid;
#ifdef _PRE_WLAN_FEATURE_UAPSD
    mac_vap_set_uapsd_en(&(pst_dmac_vap->st_vap_base_info), OAL_TRUE);
    uc_uapsd_en = pst_tspec_info->ts_info.bit_apsd;
    if(uc_uapsd_en)
    {
        pst_dmac_user->st_uapsd_status.uc_ac_trigger_ena[uc_ac_num] = 1;
        pst_dmac_user->st_uapsd_status.uc_ac_delievy_ena[uc_ac_num] = 1;
    }
    else
    {
        pst_dmac_user->st_uapsd_status.uc_ac_trigger_ena[uc_ac_num] = 0;
        pst_dmac_user->st_uapsd_status.uc_ac_delievy_ena[uc_ac_num] = 0;
    }
#endif

    /* 获得之前保存的ts信息 */
    pst_ts   = &(pst_dmac_user->st_user_base_info.st_ts_info[uc_ac_num]);
    uc_dialog_token = puc_payload[2];

    if(uc_tsid != pst_ts->uc_tsid)
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{dmac_mgmt_rx_addts_rsp::addts rsp tsid[%d] old tsid[%d]} ", uc_tsid, pst_ts->uc_tsid);
        return OAL_SUCC;
    }

    if(uc_dialog_token != pst_ts->uc_ts_dialog_token)
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::addts rspuc_dialog_token wrong.rsp dialog[%d], req dialog[%d]}",uc_dialog_token, pst_ts->uc_ts_dialog_token);
        return OAL_SUCC;
    }
    uc_status = puc_payload[3];

    if((MAC_TS_SUCCESS == pst_ts->en_ts_status) && (MAC_SUCCESSFUL_STATUSCODE != uc_status))
    {
        OAM_WARNING_LOG2(pst_dmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{dmac_mgmt_rx_addts_rsp::addts rsp status[%d] old status[%d]} ", uc_status, pst_ts->en_ts_status);
        return OAL_SUCC;
    }

    /* 根据ADDTS RSP信息，更新dmac TS状态 */
    if(MAC_SUCCESSFUL_STATUSCODE == uc_status)
    {
        pst_ts->en_ts_status = MAC_TS_SUCCESS;
    }
    else
    {
        OAL_MEMZERO(pst_ts, OAL_SIZEOF(mac_ts_stru));
        pst_ts->en_ts_status = MAC_TS_INIT;
    }
    return OAL_SUCC;
}


oal_uint32  dmac_mgmt_rx_delts(
                dmac_vap_stru                  *pst_dmac_vap,
                dmac_user_stru                 *pst_dmac_user,
                oal_uint8                      *puc_payload)
{
    oal_uint8                   uc_tsid;
    oal_uint8                   uc_ac_num;
    mac_ts_stru                *pst_ts;
    mac_wmm_tspec_stru         *pst_tspec_info;
    if ((OAL_PTR_NULL == pst_dmac_vap) || (OAL_PTR_NULL == pst_dmac_user) || (OAL_PTR_NULL == puc_payload))
    {
        OAM_ERROR_LOG3(0, OAM_SF_WMMAC, "{dmac_mgmt_rx_delts::param null, pst_dmac_vap=%d, pst_dmac_user=%d, pst_payload=%d.}", pst_dmac_vap, pst_dmac_user, puc_payload);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tspec_info = (mac_wmm_tspec_stru *)(puc_payload + 12);
    uc_tsid = pst_tspec_info->ts_info.bit_tsid;

    /* 获得之前保存的ts信息 */
    for (uc_ac_num = 0; uc_ac_num < WLAN_WME_AC_BUTT; uc_ac_num++)
    {
        pst_ts = &(pst_dmac_user->st_user_base_info.st_ts_info[uc_ac_num]);

        if(uc_tsid == pst_ts->uc_tsid)
        {
            /* 清空已保存的TS信息 */
            OAL_MEMZERO(pst_ts, OAL_SIZEOF(mac_ts_stru));
            pst_ts->en_ts_status = MAC_TS_INIT;
            pst_ts->uc_tsid      = 0xFF;
        }
    }

    return OAL_SUCC;
}

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

