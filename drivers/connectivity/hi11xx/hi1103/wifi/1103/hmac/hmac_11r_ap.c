


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11R_AP

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_cfg80211.h"
#include "oam_ext_if.h"
#include "hmac_mgmt_ap.h"
#include "hmac_encap_frame.h"
#include "hmac_encap_frame_ap.h"
#include "hmac_mgmt_bss_comm.h"
#include "mac_frame.h"
#include "hmac_rx_data.h"
#include "mac_ie.h"
#include "mac_user.h"
#include "hmac_user.h"
#include "hmac_11i.h"
#include "hmac_fsm.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"
#include "hmac_11r_ap.h"
#include "hmac_mgmt_bss_comm.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_11R_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/
oal_void  hmac_ft_ap_up_rx_auth_req(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_auth_req)
{
    oal_uint8         auc_addr2[ETHER_ADDR_LEN]      = {0};
    oal_uint8         uc_is_seq1;
    oal_uint16        us_auth_seq       = 0;
    oal_uint16        us_user_index          = 0xffff;
    oal_uint8         uc_auth_resend    = 0;
    oal_uint32        ul_ret;

    hmac_rx_mgmt_send_to_host_etc(pst_hmac_vap, pst_auth_req);
    /* 获取STA的地址 */
    mac_get_address2(oal_netbuf_header(pst_auth_req), auc_addr2);
    if(mac_addr_is_zero_etc(auc_addr2))
    {
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH, "{hmac_ap_rx_auth_req::user mac:%02X:XX:XX:%02X:%02X:%02X is all 0 and invaild!}",
                                auc_addr2[0],auc_addr2[3],auc_addr2[4],auc_addr2[5]);
        return;
    }

    /* 解析auth transaction number */
    us_auth_seq  = mac_get_auth_seq_num(oal_netbuf_header(pst_auth_req));
    if (us_auth_seq > HMAC_AP_AUTH_SEQ3_WEP_COMPLETE)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,"{hmac_ap_rx_auth_req::auth recieve invalid seq, auth seq [%d]}", us_auth_seq);
        return;
    }
    /* 获取用户idx */
    uc_is_seq1 = (WLAN_AUTH_TRASACTION_NUM_ONE == us_auth_seq);
    ul_ret = hmac_encap_auth_rsp_get_user_idx_etc(&(pst_hmac_vap->st_vap_base_info),
                                        auc_addr2,
                                        uc_is_seq1,
                                        &uc_auth_resend,
                                        &us_user_index);
    if(OAL_SUCC != ul_ret)
    {
        if(OAL_ERR_CODE_CONFIG_EXCEED_SPEC == ul_ret)
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,"{hmac_ft_ap_up_rx_auth_req::hmac_ap_get_user_idx fail, users exceed config spec!}");
        }
        else
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AUTH,"{hmac_ft_ap_up_rx_auth_req::hmac_ap_get_user_idx Err!}");
        }
    }

    return;
}

oal_uint32  hmac_ft_ap_up_rx_assoc_req(
                hmac_vap_stru                  *pst_hmac_vap,
                hmac_user_stru                 *pst_hmac_user,
                oal_uint8                       uc_mgmt_frm_type,
                oal_uint8                      *puc_payload,
                oal_uint32                      ul_payload_len)
{
    oal_uint32 ul_ret;

    /* AP 保存STA 的关联请求帧信息，以备上报内核 */
    ul_ret = hmac_ap_save_user_assoc_req(pst_hmac_user, puc_payload, ul_payload_len, uc_mgmt_frm_type);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ASSOC, "{hmac_ft_ap_up_rx_assoc_req :: hmac_ap_save_user_assoc_req fail[%d].}", ul_ret);
        return ul_ret;
    }
    /* 上报WAL层(WAL上报内核) AP关联上了一个新的STA */
    hmac_handle_connect_rsp_ap(pst_hmac_vap, pst_hmac_user);
    OAM_WARNING_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_ft_ap_up_rx_assoc_req::new station to hostapd.}");

    return OAL_SUCC;
}

oal_uint32 hmac_ft_assoc(mac_vap_stru *pst_mac_vap, oal_mlme_ie_stru *pst_mlme_ie)
{
    oal_uint32                      ul_rslt;
    oal_netbuf_stru                *pst_asoc_rsp;
    hmac_user_stru                 *pst_hmac_user;
    oal_uint16                      us_user_idx = 0;
    oal_uint32                      ul_asoc_rsp_len  = 0;
    mac_status_code_enum_uint16     en_status_code;
    mac_tx_ctl_stru                *pst_tx_ctl;
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;
    mac_cfg_user_info_param_stru    st_hmac_user_info_event;
    oal_uint8                       uc_chip_id;
    oal_uint16                      us_other_user_idx;
    hmac_vap_stru                  *pst_hmac_other_vap;
    hmac_user_stru                 *pst_hmac_other_user;
    hmac_vap_stru                  *pst_hmac_vap;
    ul_rslt = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, pst_mlme_ie->auc_macaddr, &us_user_idx);
    if (OAL_SUCC != ul_rslt)
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_ap_up_rx_asoc_req::failed find user:%02X:XX:XX:%02X:%02X:%02X.}",
                         pst_mlme_ie->auc_macaddr[0], pst_mlme_ie->auc_macaddr[3], pst_mlme_ie->auc_macaddr[4], pst_mlme_ie->auc_macaddr[5]);
        hmac_mgmt_send_deauth_frame_etc(pst_mac_vap, pst_mlme_ie->auc_macaddr, MAC_ASOC_NOT_AUTH, OAL_FALSE);

        return ul_rslt;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);

    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_ap_up_rx_asoc_req::pst_hmac_user[%d] null.}", us_user_idx);

        /* 没有查到对应的USER,发送去认证消息 */
        hmac_mgmt_send_deauth_frame_etc(pst_mac_vap, pst_mlme_ie->auc_macaddr, MAC_ASOC_NOT_AUTH, OAL_FALSE);

        return OAL_ERR_CODE_PTR_NULL;
    }

    en_status_code = pst_mlme_ie->us_reason;

    pst_asoc_rsp = (oal_netbuf_stru *)OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_asoc_rsp)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_ft_assoc::pst_asoc_rsp null.}");
        /*异常返回之前删除user*/
        hmac_user_del_etc(pst_mac_vap, pst_hmac_user);

        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_asoc_rsp);
    OAL_MEMZERO(pst_tx_ctl, OAL_NETBUF_CB_SIZE());

    OAL_MEM_NETBUF_TRACE(pst_asoc_rsp, OAL_TRUE);

    /* 获取hmac vap指针 */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        oal_netbuf_free(pst_asoc_rsp);
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_AUTH,
            "{hmac_ft_assoc::pst_hmac_vap[%d] is NULL!}", pst_mac_vap->uc_vap_id);

        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->pst_mlme = pst_mlme_ie;
    if (OAL_IEEE80211_MLME_ASSOC == pst_mlme_ie->en_mlme_type)
    {
        ul_asoc_rsp_len = hmac_mgmt_encap_asoc_rsp_ap_etc(pst_mac_vap,
                                                      pst_mlme_ie->auc_macaddr,
                                                      pst_hmac_user->st_user_base_info.us_assoc_id,
                                                      en_status_code,
                                                      OAL_NETBUF_HEADER(pst_asoc_rsp),
                                                      WLAN_FC0_SUBTYPE_ASSOC_RSP);
    }
    else if (OAL_IEEE80211_MLME_REASSOC == pst_mlme_ie->en_mlme_type)
    {
        ul_asoc_rsp_len = hmac_mgmt_encap_asoc_rsp_ap_etc(pst_mac_vap,
                                                      pst_mlme_ie->auc_macaddr,
                                                      pst_hmac_user->st_user_base_info.us_assoc_id,
                                                      en_status_code,
                                                      OAL_NETBUF_HEADER(pst_asoc_rsp),
                                                      WLAN_FC0_SUBTYPE_REASSOC_RSP);
    }
    pst_hmac_vap->pst_mlme = OAL_PTR_NULL;

    if (0 == ul_asoc_rsp_len)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_ft_assoc::hmac_mgmt_encap_asoc_rsp_ap_etc encap msg fail.}");
        oal_netbuf_free(pst_asoc_rsp);

        /*异常返回之前删除user*/
        hmac_user_del_etc(pst_mac_vap, pst_hmac_user);

        return OAL_FAIL;
    }

    oal_netbuf_put(pst_asoc_rsp, ul_asoc_rsp_len);

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_hmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)    = (oal_uint16)ul_asoc_rsp_len;

    /* 发送关联响应帧之前，将用户的节能状态复位 */
    hmac_mgmt_reset_psm_etc(pst_mac_vap, MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));

    if (MAC_SUCCESSFUL_STATUSCODE == en_status_code)
    {
        hmac_user_set_asoc_state_etc(pst_mac_vap, &pst_hmac_user->st_user_base_info, MAC_USER_STATE_ASSOC);
    }

    ul_rslt = hmac_tx_mgmt_send_event_etc(pst_mac_vap, pst_asoc_rsp, (oal_uint16)ul_asoc_rsp_len);

    if (OAL_SUCC != ul_rslt)
    {
        OAM_WARNING_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ASSOC,
                         "{hmac_ft_assoc::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_rslt);
        oal_netbuf_free(pst_asoc_rsp);

        /*异常返回之前删除user*/
        hmac_user_del_etc(pst_mac_vap, pst_hmac_user);

        return ul_rslt;
    }

    if (MAC_SUCCESSFUL_STATUSCODE == en_status_code)
    {
        /* AP检测STA成功，允许其关联成功*/
#ifdef _PRE_DEBUG_MODE_USER_TRACK
        mac_user_change_info_event(pst_hmac_user->st_user_base_info.auc_user_mac_addr,
                                       pst_mac_vap->uc_vap_id,
                                       pst_hmac_user->st_user_base_info.en_user_asoc_state,
                                       MAC_USER_STATE_ASSOC, OAM_MODULE_ID_HMAC,
                                       OAM_USER_INFO_CHANGE_TYPE_ASSOC_STATE);
#endif
        /* 打开80211单播管理帧开关，观察关联过程，关联成功了就关闭 */
        st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_TX;
        st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
        st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_OFF;
        oal_memcopy(st_80211_ucast_switch.auc_user_macaddr,
                (const oal_void *)pst_hmac_user->st_user_base_info.auc_user_mac_addr,
                OAL_SIZEOF(st_80211_ucast_switch.auc_user_macaddr));
        hmac_config_80211_ucast_switch_etc(pst_mac_vap,OAL_SIZEOF(st_80211_ucast_switch),(oal_uint8 *)&st_80211_ucast_switch);

        st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_RX;
        st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
        st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_OFF;
        hmac_config_80211_ucast_switch_etc(pst_mac_vap,OAL_SIZEOF(st_80211_ucast_switch),(oal_uint8 *)&st_80211_ucast_switch);

        ul_rslt = hmac_config_user_rate_info_syn_etc(pst_mac_vap, &pst_hmac_user->st_user_base_info);
        if (OAL_SUCC != ul_rslt)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC,
                           "{hmac_ft_assoc::hmac_config_user_rate_info_syn_etc failed[%d].}", ul_rslt);
        }

        /*  user已经关联上，抛事件给DMAC，在DMAC层挂用户算法钩子 */
        hmac_user_add_notify_alg_etc(pst_mac_vap, us_user_idx);
        for (uc_chip_id = 0; uc_chip_id < WLAN_CHIP_MAX_NUM_PER_BOARD; uc_chip_id++)
        {
            if (uc_chip_id != pst_mac_vap->uc_chip_id)
            {
                /* 若在同一板子下的其他VAP下找到给用户，删除之。否则sta有可能仍然在另一个AP发包 */
                if (OAL_SUCC == mac_chip_find_user_by_macaddr(uc_chip_id, pst_mlme_ie->auc_macaddr, &us_other_user_idx))
                {
                    pst_hmac_other_user = mac_res_get_hmac_user_etc(us_other_user_idx);
                    if (OAL_PTR_NULL != pst_hmac_other_user)
                    {
                        pst_hmac_other_vap  = mac_res_get_hmac_vap(pst_hmac_other_user->st_user_base_info.uc_vap_id);
                        /* 抛事件上报内核，已经删除某个STA */
                        hmac_mgmt_send_disassoc_frame_etc(&(pst_hmac_other_vap->st_vap_base_info), pst_mlme_ie->auc_macaddr, MAC_DISAS_LV_SS, OAL_FALSE);
                        hmac_handle_disconnect_rsp_ap_etc(pst_hmac_other_vap, pst_hmac_other_user);
                        hmac_user_del_etc(&(pst_hmac_other_vap->st_vap_base_info), pst_hmac_other_user);
                    }
                }
            }
        }

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_ft_assoc::STA assoc AP SUCC! STA_indx=%d.}", us_user_idx);
    }
    else
    {
        /* AP检测STA失败，将其删除 */
        if (MAC_REJECT_TEMP != en_status_code)
        {
            hmac_user_del_etc(pst_mac_vap, pst_hmac_user);
        }
    }

    /*  STA 入网后，上报VAP 信息和用户信息 */
    st_hmac_user_info_event.us_user_idx = us_user_idx;

    hmac_config_vap_info_etc(pst_mac_vap, OAL_SIZEOF(oal_uint32), (oal_uint8 *)&ul_rslt);
    hmac_config_user_info_etc(pst_mac_vap, OAL_SIZEOF(mac_cfg_user_info_param_stru), (oal_uint8 *)&st_hmac_user_info_event);

    return OAL_SUCC;
}

oal_uint16  hmac_ft_encap_auth_rsp(mac_vap_stru *pst_mac_vap, oal_netbuf_stru *pst_auth_rsp, oal_mlme_ie_stru *pst_mlme)
{
    oal_uint16       us_auth_rsp_len        = 0;
    oal_uint8       *puc_frame              = OAL_PTR_NULL;
    oal_uint16       us_index               = 0;
    oal_uint16       us_auth_type           = 0;
    oal_uint16       us_user_index          = 0xffff;
    oal_uint32       ul_ret                 = 0;
    oal_uint8       *puc_data;
    mac_tx_ctl_stru *pst_tx_ctl;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == pst_auth_rsp || OAL_PTR_NULL == pst_mlme)
    {
        OAM_ERROR_LOG3(0, OAM_SF_AUTH,"{hmac_ft_encap_auth_rsp::pst_mac_vap[0x%p], puc_data[0x%p], pst_mlme[0x%p]}", pst_mac_vap, pst_auth_rsp, pst_mlme);
        return us_auth_rsp_len;
    }

    puc_data    = (oal_uint8 *)OAL_NETBUF_HEADER(pst_auth_rsp);
    pst_tx_ctl  = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_auth_rsp);

    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /*************************************************************************/
    /*                Set the fields in the frame header                     */
    /*************************************************************************/

    /* 设置函数头的frame control字段 */
    mac_hdr_set_frame_control(puc_data, WLAN_FC0_SUBTYPE_AUTH);

    /* 将DA设置为STA的地址 */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address1, pst_mlme->auc_macaddr);

    /* 将SA设置为dot11MacAddress */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address2, mac_mib_get_StationID(pst_mac_vap));
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address3, pst_mac_vap->auc_bssid);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*              Authentication Frame - Frame Body                        */
    /* --------------------------------------------------------------------- */
    /* |Auth Algo Number|Auth Trans Seq Number|Status Code| Challenge Text | */
    /* --------------------------------------------------------------------- */
    /* | 2              |2                    |2          | 3 - 256        | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    us_index = MAC_80211_FRAME_LEN;
    puc_frame = (oal_uint8 *)(puc_data + us_index);

    /* 计算认证相应帧的长度 */
    us_auth_rsp_len = MAC_80211_FRAME_LEN + MAC_AUTH_ALG_LEN + MAC_AUTH_TRANS_SEQ_NUM_LEN +
                      MAC_STATUS_CODE_LEN;

    /* 解析认证类型 */
    us_auth_type = WLAN_WITP_AUTH_FT;


    /* 设置认证类型IE */
    puc_frame[0] = (us_auth_type & 0x00FF);
    puc_frame[1] = (us_auth_type & 0xFF00) >> 8;

    /* 根据下发内容填写认证响应帧的transaction number */
    puc_frame[2] = ((pst_mlme->uc_seq) & 0x00FF);
    puc_frame[3] = 0;

    /* 认证状态 */
    puc_frame[4] = (pst_mlme->us_reason & 0x00FF);
    puc_frame[5] = (pst_mlme->us_reason & 0xFF00) >> 8;
    puc_frame = (oal_uint8 *)(puc_data + us_auth_rsp_len);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_ft_encap_auth_rsp::user mac:%02X:XX:XX:%02X:%02X:%02X}",
                                pst_mlme->auc_macaddr[0],pst_mlme->auc_macaddr[3],pst_mlme->auc_macaddr[4],pst_mlme->auc_macaddr[5]);
    oal_memcopy(puc_frame, pst_mlme->auc_optie, pst_mlme->us_optie_len);
    us_auth_rsp_len += pst_mlme->us_optie_len;
    puc_frame = (oal_uint8 *)(puc_data + us_index);

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = MAC_INVALID_USER_ID;
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)    = us_auth_rsp_len;

    if(mac_addr_is_zero_etc(pst_mlme->auc_macaddr))
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_ft_encap_auth_rsp::user mac:%02X:XX:XX:%02X:%02X:%02X is all 0 and invaild!}",
                                pst_mlme->auc_macaddr[0],pst_mlme->auc_macaddr[3],pst_mlme->auc_macaddr[4],pst_mlme->auc_macaddr[5]);
        puc_frame[4] = MAC_UNSPEC_FAIL;
        return us_auth_rsp_len;
    }

    /* 获取hmac user指针 */
    ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, pst_mlme->auc_macaddr, &us_user_index);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_AUTH,"{hmac_ft_encap_auth_rsp::pst_hmac_user_sta[%d] is NULL}", us_user_index);
        puc_frame[4] = MAC_UNSPEC_FAIL;
        return us_auth_rsp_len;
    }

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = us_user_index;
    return us_auth_rsp_len;
}



oal_uint32 hmac_ft_auth(mac_vap_stru *pst_mac_vap, oal_mlme_ie_stru *pst_mlme_ie)
{
    oal_netbuf_stru  *pst_auth_rsp      = OAL_PTR_NULL;
    oal_uint16        us_auth_rsp_len   = 0;
    oal_uint32        ul_ret;
    mac_tx_ctl_stru  *pst_tx_ctl;
    hmac_vap_stru    *pst_hmac_vap      = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == pst_mlme_ie)
    {
        OAM_ERROR_LOG2(0, OAM_SF_AUTH, "{hmac_ft_auth::param null, pst_mac_vap=0x%p pst_mlme_ie=0x%p.}", pst_mac_vap, pst_mlme_ie);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* AP接收到STA发来的认证请求帧组相应的认证响应帧 */
    pst_auth_rsp = (oal_netbuf_stru *)OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if(OAL_PTR_NULL == pst_auth_rsp)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_ft_auth::pst_auth_rsp null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_auth_rsp, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_auth_rsp), OAL_NETBUF_CB_SIZE());

    us_auth_rsp_len = hmac_ft_encap_auth_rsp(pst_mac_vap,
                                          pst_auth_rsp,
                                          pst_mlme_ie);
    if (0 == us_auth_rsp_len)
    {
        oal_netbuf_free(pst_auth_rsp);
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_ft_auth::us_auth_rsp_len is 0.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_netbuf_put(pst_auth_rsp, us_auth_rsp_len);

    /* 获取hmac vap指针 */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        oal_netbuf_free(pst_auth_rsp);
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_AUTH,
            "{hmac_ft_auth::pst_hmac_vap[%d] is NULL!}", pst_mac_vap->uc_vap_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_mgmt_update_auth_mib(pst_hmac_vap, pst_auth_rsp);

    /* 发送认证响应帧之前，将用户的节能状态复位 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_auth_rsp);
    if (OAL_PTR_NULL != mac_res_get_hmac_user_etc(MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)))
    {
        hmac_mgmt_reset_psm_etc(pst_mac_vap, MAC_GET_CB_TX_USER_IDX(pst_tx_ctl));
    }

    /* 抛事件给dmac发送认证帧 */
    ul_ret = hmac_tx_mgmt_send_event_etc(pst_mac_vap, pst_auth_rsp, us_auth_rsp_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_auth_rsp);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "{hmac_ft_auth::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
    }
    return OAL_SUCC;
}


#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

