


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_net.h"
#include "mac_frame.h"
#include "mac_resource.h"
#include "mac_ie.h"
#include "mac_vap.h"
#include "mac_user.h"
#include "frw_ext_if.h"
#include "hal_ext_if.h"
#include "mac_resource.h"
#include "wlan_types.h"
#include "dmac_ext_if.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_11v.h"
#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#include "hmac_roam_connect.h"
#include "hmac_roam_alg.h"
#endif
#include "hmac_scan.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_11V_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/

#ifdef _PRE_WLAN_FEATURE_11V_ENABLE

oal_uint32 hmac_11v_roam_scan_check(hmac_vap_stru *pst_hmac_vap)
{
    hmac_user_stru            *pst_hmac_user;
    hmac_user_11v_ctrl_stru   *pst_11v_ctrl_info;
    hmac_roam_info_stru       *pst_roam_info;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_11v_roam_scan_check::pst_roam_info IS NULL}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 获取发送端的用户指针 */
    pst_hmac_user   = mac_res_get_hmac_user_etc(pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_11v_roam_scan_check::pst_hmac_user is NULL}");
        return OAL_ERR_CODE_ROAM_INVALID_USER;
    }
    pst_11v_ctrl_info = &(pst_hmac_user->st_11v_ctrl_info);

    if(OAL_PTR_NULL  == pst_11v_ctrl_info->mac_11v_callback_fn)
    {
        return OAL_SUCC;
    }

    if(pst_11v_ctrl_info->uc_11v_roam_scan_times < MAC_11V_ROAM_SCAN_ONE_CHANNEL_LIMIT)
    {/*触发单信道扫描漫游*/
        pst_11v_ctrl_info->uc_11v_roam_scan_times++;
        OAM_WARNING_LOG3(0, OAM_SF_ANY, "{hmac_11v_roam_scan_check::Trigger One channel scan roam, uc_11v_roam_scan_times=[%d],limit_times=[%d].channel=[%d]}",
        pst_11v_ctrl_info->uc_11v_roam_scan_times, MAC_11V_ROAM_SCAN_ONE_CHANNEL_LIMIT, pst_roam_info->st_bsst_rsp_info.uc_chl_num);
        hmac_roam_start_etc(pst_hmac_vap, ROAM_SCAN_CHANNEL_ORG_1, OAL_FALSE, ROAM_TRIGGER_11V);
    }
    else if(MAC_11V_ROAM_SCAN_ONE_CHANNEL_LIMIT == pst_11v_ctrl_info->uc_11v_roam_scan_times)
    {/*触发全信道扫描漫游*/
        pst_11v_ctrl_info->uc_11v_roam_scan_times++;
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_11v_roam_scan_check::Trigger ALL Channel scan roam.}");
        hmac_roam_start_etc(pst_hmac_vap, ROAM_SCAN_CHANNEL_ORG_BUTT, OAL_FALSE, ROAM_TRIGGER_11V);
    }
    return OAL_SUCC;
}


oal_uint32 hmac_rx_bsst_req_candidate_info_check(hmac_vap_stru *pst_hmac_vap, oal_uint8 uc_channel, oal_uint8 *puc_bssid)
{
    wlan_channel_band_enum_uint8            en_channel_band;
    oal_uint32                              ul_check;
    mac_bss_dscr_stru                      *pst_bss_dscr;

    en_channel_band = mac_get_band_by_channel_num(uc_channel);
    ul_check        = mac_is_channel_num_valid_etc(en_channel_band, uc_channel);
    if (OAL_SUCC != ul_check)
    {
        /*对于无效channel如果bssid存在扫描列表中则继续11v漫游流程  */
        pst_bss_dscr = (mac_bss_dscr_stru *)hmac_scan_get_scanned_bss_by_bssid(&pst_hmac_vap->st_vap_base_info, puc_bssid);
        if (OAL_PTR_NULL != pst_bss_dscr)
        {
            OAM_WARNING_LOG4(0, OAM_SF_CFG,"{hmac_rx_bsst_req_candidate_info_check::uc_channel=[%d] is invalid,but bssid:XX:XX:XX:XX:%02X:%02X in bssinfo channel=[%d]}",
                             uc_channel, puc_bssid[4], puc_bssid[5], pst_bss_dscr->st_channel.uc_chan_number);
            return OAL_SUCC;
        }

        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_rx_bsst_req_action(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf)
{
    oal_uint16                      us_handle_len = 0;
    dmac_rx_ctl_stru               *pst_rx_ctrl;
    mac_rx_ctl_stru                *pst_rx_info;
    oal_uint16                      us_frame_len = 0;
    oal_uint8                      *puc_data = OAL_PTR_NULL;
    hmac_bsst_req_info_stru         st_bsst_req_info;
    hmac_bsst_rsp_info_stru         st_bsst_rsp_info;
    oal_uint16                      us_url_count = 0;
    hmac_user_11v_ctrl_stru        *pst_11v_ctrl_info;
    mac_user_stru                  *pst_mac_user;
    oal_uint32                      ul_ret;
    hmac_roam_info_stru            *pst_roam_info;
    oal_bool_enum_uint8             en_need_roam = OAL_TRUE;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::null param, vap:0x%x user:0x%x buf:0x%x.}", pst_hmac_vap, pst_hmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*开关未打开不处理*/
    if (OAL_FALSE == mac_mib_get_MgmtOptionBSSTransitionActivated(&pst_hmac_vap->st_vap_base_info))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action:: BSSTransitionActivated is disabled}");
        return OAL_SUCC;
    }

    pst_mac_user = mac_res_get_mac_user_etc(pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::pst_mac_user is NULL}");
        return OAL_ERR_CODE_ROAM_INVALID_USER;
    }

    pst_11v_ctrl_info = &(pst_hmac_user->st_11v_ctrl_info);
    OAL_MEMZERO(pst_11v_ctrl_info, OAL_SIZEOF(hmac_user_11v_ctrl_stru));

    pst_rx_ctrl         = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_rx_info         = (mac_rx_ctl_stru *)(&(pst_rx_ctrl->st_rx_info));
    /* 获取帧体指针 */
    puc_data     = MAC_GET_RX_PAYLOAD_ADDR(pst_rx_info, pst_netbuf);
    us_frame_len = MAC_GET_RX_CB_PAYLOAD_LEN(pst_rx_info);  /*帧体长度*/
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::handle 11v bsst req start.}");
#endif
    /* 帧体的最小长度为7 小于7则格式异常 */
    if (HMAC_11V_REQUEST_FRAME_BODY_FIX > us_frame_len)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action:: frame length error %d.}", us_frame_len);
        return OAL_FAIL;
    }

    /* 将帧的各种参数解析出来 供上层调用 */
    /* 解析Token 如果与当前用户下不一致 刷新Token */
    if (puc_data[2] != pst_11v_ctrl_info->uc_user_bsst_token)
    {
        pst_11v_ctrl_info->uc_user_bsst_token = puc_data[2];
    }
    /* 解析request mode */
    oal_memset(&st_bsst_req_info, 0, sizeof(st_bsst_req_info));
    st_bsst_req_info.st_request_mode.bit_candidate_list_include = puc_data[3]&BIT0;
    st_bsst_req_info.st_request_mode.bit_abridged = (puc_data[3]&BIT1)? OAL_TRUE : OAL_FALSE;
    st_bsst_req_info.st_request_mode.bit_bss_disassoc_imminent = (puc_data[3]&BIT2)? OAL_TRUE : OAL_FALSE;
    st_bsst_req_info.st_request_mode.bit_termination_include = (puc_data[3]&BIT3)? OAL_TRUE : OAL_FALSE;
    st_bsst_req_info.st_request_mode.bit_ess_disassoc_imminent = (puc_data[3]&BIT4)? OAL_TRUE : OAL_FALSE;

    st_bsst_req_info.us_disassoc_time = ((oal_uint16)(puc_data[5]) << 8 ) | puc_data[4];
    st_bsst_req_info.uc_validity_interval = puc_data[6];
    us_handle_len = 7;              /* 前面7个字节已被处理完 */
    /* 12字节的termination duration 如果有的话 */
    if (st_bsst_req_info.st_request_mode.bit_termination_include)
    {
        us_handle_len += MAC_IE_HDR_LEN;                /* 去掉元素头 */
        oal_memcopy(st_bsst_req_info.st_term_duration.auc_termination_tsf, puc_data+us_handle_len, HMAC_11V_TERMINATION_TSF_LENGTH);
        us_handle_len += HMAC_11V_TERMINATION_TSF_LENGTH;
        st_bsst_req_info.st_term_duration.us_duration_min = (((oal_uint16)puc_data[us_handle_len+1]) << 8) | (puc_data[us_handle_len]);
        us_handle_len += 2;
    }
    /* 解析URL */
    /* URL字段 如果有的话 URL第一个字节为URL长度 申请动态内存保存 */
    st_bsst_req_info.puc_session_url = OAL_PTR_NULL;
    if (st_bsst_req_info.st_request_mode.bit_ess_disassoc_imminent)
    {
        if (0 != puc_data[us_handle_len])
        {
            /* 申请内存数量加1 用于存放字符串结束符 */
            us_url_count = puc_data[us_handle_len]*OAL_SIZEOF(oal_uint8)+1;
            st_bsst_req_info.puc_session_url = (oal_uint8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,us_url_count,OAL_TRUE);
            if (OAL_PTR_NULL == st_bsst_req_info.puc_session_url)
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action:: puc_session_url alloc fail.}");
                return OAL_FAIL;
            }
            oal_memcopy(st_bsst_req_info.puc_session_url, puc_data+(us_handle_len+1), puc_data[us_handle_len]);
            /* 转化成字符串 */
            st_bsst_req_info.puc_session_url[puc_data[us_handle_len]] = '\0';
        }
        us_handle_len += (puc_data[us_handle_len] + 1);
    }

    if (us_handle_len > us_frame_len)
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::us_handle_len[%d] > us_frame_len[%d]}", us_handle_len, us_frame_len);
        /* 释放已经申请的内存 */
        if (OAL_PTR_NULL != st_bsst_req_info.puc_session_url)
        {
            OAL_MEM_FREE(st_bsst_req_info.puc_session_url, OAL_TRUE);
            st_bsst_req_info.puc_session_url = OAL_PTR_NULL;
        }
        return OAL_FAIL;
    }

    /* Candidate bss list由于STA的Response frame为可选 需要解析出来放在此结构体中 供上层处理 */
    st_bsst_req_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    if (st_bsst_req_info.st_request_mode.bit_candidate_list_include)
    {
        puc_data += us_handle_len;
        st_bsst_req_info.pst_neighbor_bss_list = hmac_get_neighbor_ie(puc_data, us_frame_len-us_handle_len, &st_bsst_req_info.uc_bss_list_num);
    }

    OAM_WARNING_LOG4(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action:: associated user mac address=xx:xx:xx:%02x:%02x:%02x uc_bss_list_num=%d}",
            pst_mac_user->auc_user_mac_addr[3], pst_mac_user->auc_user_mac_addr[4], pst_mac_user->auc_user_mac_addr[5],
            st_bsst_req_info.uc_bss_list_num);

    /* 根据终端需求实现11v漫游 */
    if (OAL_PTR_NULL != st_bsst_req_info.pst_neighbor_bss_list)
    {
        if ((ETHER_IS_BROADCAST(st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr)
            || ETHER_IS_ALL_ZERO(st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr))
            && (st_bsst_req_info.us_disassoc_time == 0))
        {/*广播地址直接退出*/
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::bsst req candidate bssid is broadcast or all zero address, will not roam}");
        }
        else if (!oal_memcmp(pst_mac_user->auc_user_mac_addr, st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr,
            WLAN_MAC_ADDR_LEN) && (st_bsst_req_info.us_disassoc_time == 0))
        {/*关联AP BSSID 直接退出*/
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::bsst req candidate bssid is the same with the associated AP, will not roam}");
        }
        else
        {
            OAM_WARNING_LOG4(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::candidate bssid=xx:xx:xx:%02x:%02x:%02x, dst AP's chan=%d}",
                             st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr[3],
                             st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr[4],
                             st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr[5],
                             st_bsst_req_info.pst_neighbor_bss_list->uc_chl_num);
            /*检查channel num 是否有效*/
            ul_ret = hmac_rx_bsst_req_candidate_info_check(pst_hmac_vap, st_bsst_req_info.pst_neighbor_bss_list->uc_chl_num,
                         st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr);
            if(OAL_SUCC != ul_ret)
            {
                en_need_roam = OAL_FALSE;
            }

            oal_memset(&st_bsst_rsp_info, 0, sizeof(st_bsst_rsp_info));
            /* 选取第一个BSS 作为目标BSS 发送Response给AP */
            st_bsst_rsp_info.uc_status_code = 0;                            /* 默认设置为同意切换 */
            st_bsst_rsp_info.uc_termination_delay = 0;                      /* 仅当状态码为5时有效，此次无意义设为0 */
            st_bsst_rsp_info.uc_chl_num = st_bsst_req_info.pst_neighbor_bss_list->uc_chl_num;
            /* 调试用不作判断 认为request帧中带有候选AP列表集 */
            oal_memcopy(st_bsst_rsp_info.auc_target_bss_addr, st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr, WLAN_MAC_ADDR_LEN);
            st_bsst_rsp_info.c_rssi = hmac_get_rssi_from_scan_result(pst_hmac_vap, pst_hmac_vap->st_vap_base_info.auc_bssid);

            /* register BSS Transition Response callback function:
             * so that check roaming scan results firstly, and then send bsst rsp frame with right status code */
            pst_11v_ctrl_info->mac_11v_callback_fn = hmac_tx_bsst_rsp_action;

#ifdef _PRE_WLAN_FEATURE_ROAM
            pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
            oal_memcopy(&(pst_roam_info->st_bsst_rsp_info), &st_bsst_rsp_info, sizeof(st_bsst_rsp_info));

            /* broadcast address or assocaited AP's address, && disassociation time > 0, BSST reject */
            if ((ETHER_IS_BROADCAST(st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr)
                || ETHER_IS_ALL_ZERO(st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr)
                || !oal_memcmp(pst_mac_user->auc_user_mac_addr, st_bsst_req_info.pst_neighbor_bss_list->auc_mac_addr, WLAN_MAC_ADDR_LEN))
                && (st_bsst_req_info.us_disassoc_time > 0))
            {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_rx_bsst_req_action::bsst req candidate bssid is invalid, us_disassoc_time=%d, will reject}",
                                 st_bsst_req_info.us_disassoc_time);
                pst_roam_info->st_bsst_rsp_info.uc_status_code = WNM_BSS_TM_REJECT_NO_SUITABLE_CANDIDATES;
                hmac_tx_bsst_rsp_action(pst_hmac_vap, pst_hmac_user, &(pst_roam_info->st_bsst_rsp_info));
                en_need_roam = OAL_FALSE;
            }

            /* Signal Bridge disable 11v roaming */
            ul_ret = hmac_roam_check_signal_bridge_etc(pst_hmac_vap);
            if (OAL_SUCC != ul_ret)
            {
                pst_roam_info->st_bsst_rsp_info.uc_status_code = WNM_BSS_TM_REJECT_NO_SUITABLE_CANDIDATES;
                hmac_tx_bsst_rsp_action(pst_hmac_vap, pst_hmac_user, &(pst_roam_info->st_bsst_rsp_info));
                en_need_roam = OAL_FALSE;
            }

            if ((OAL_TRUE == en_need_roam) && (1 == st_bsst_req_info.uc_bss_list_num))
            {
                pst_11v_ctrl_info->uc_11v_roam_scan_times = 1;
                hmac_roam_start_etc(pst_hmac_vap, ROAM_SCAN_CHANNEL_ORG_1, OAL_FALSE, ROAM_TRIGGER_11V);
            }
            else if (OAL_TRUE == en_need_roam)
            {
                hmac_roam_start_etc(pst_hmac_vap, ROAM_SCAN_CHANNEL_ORG_BUTT, OAL_FALSE, ROAM_TRIGGER_11V);
            }
#endif
        }
    }

    /* 释放指针 */
    if (OAL_PTR_NULL != st_bsst_req_info.puc_session_url)
    {
        OAL_MEM_FREE(st_bsst_req_info.puc_session_url, OAL_TRUE);
        st_bsst_req_info.puc_session_url = OAL_PTR_NULL;
    }
    if (OAL_PTR_NULL != st_bsst_req_info.pst_neighbor_bss_list)
    {
        OAL_MEM_FREE(st_bsst_req_info.pst_neighbor_bss_list, OAL_TRUE);
        st_bsst_req_info.pst_neighbor_bss_list = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_tx_bsst_rsp_action(void *pst_void1, void *pst_void2, void *pst_void3)
{
    hmac_vap_stru                 *pst_hmac_vap = (hmac_vap_stru *)pst_void1;
    hmac_user_stru                *pst_hmac_user = (hmac_user_stru *)pst_void2;
    hmac_bsst_rsp_info_stru       *pst_bsst_rsp_info = (hmac_bsst_rsp_info_stru *) pst_void3;
    oal_netbuf_stru               *pst_bsst_rsp_buf;
    oal_uint16                     us_frame_len;
    mac_tx_ctl_stru               *pst_tx_ctl;
    oal_uint32                     ul_ret;
    hmac_user_11v_ctrl_stru       *pst_11v_ctrl_info;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == pst_bsst_rsp_info))
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "{hmac_tx_bsst_rsp_action::null param, %x %x %x.}", pst_hmac_vap, pst_hmac_user, pst_bsst_rsp_info);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_11v_ctrl_info = &(pst_hmac_user->st_11v_ctrl_info);

    /* 申请bss transition request管理帧内存 */
    pst_bsst_rsp_buf = OAL_MEM_NETBUF_ALLOC(OAL_MGMT_NETBUF, WLAN_MGMT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_HIGH);
    if (OAL_PTR_NULL == pst_bsst_rsp_buf)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_tx_bsst_rsp_action::pst_bsst_rsq_buf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_bsst_rsp_buf, OAL_TRUE);
    oal_set_netbuf_prev(pst_bsst_rsp_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_bsst_rsp_buf, OAL_PTR_NULL);
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_tx_bsst_rsp_action::encap 11v bsst rsp start.}");
#endif
    /* 调用封装管理帧接口 */
    us_frame_len = hmac_encap_bsst_rsp_action(pst_hmac_vap, pst_hmac_user, pst_bsst_rsp_info, pst_bsst_rsp_buf);
    if (0 == us_frame_len)
    {
        oal_netbuf_free(pst_bsst_rsp_buf);
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_tx_bsst_rsp_action::encap bsst rsp action frame failed.}");
        return OAL_FAIL;
    }
    /* 初始化CB */
    OAL_MEMZERO(oal_netbuf_cb(pst_bsst_rsp_buf), OAL_NETBUF_CB_SIZE());
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_bsst_rsp_buf);
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = pst_hmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;
    //MAC_GET_CB_FRAME_TYPE(pst_tx_ctl) = WLAN_CB_FRAME_TYPE_ACTION;
    //MAC_GET_CB_FRAME_SUBTYPE(pst_tx_ctl) = WLAN_FRAME_TYPE_ACTION_BUTT;
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl) = us_frame_len;

    oal_netbuf_put(pst_bsst_rsp_buf, us_frame_len);

    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
        "{hmac_tx_bsst_rsp_action::tx 11v bsst rsp frame, us_frame_len=%d frametype=%d.}",
        us_frame_len, MAC_GET_CB_FRAME_TYPE(pst_tx_ctl));

    /* 抛事件让dmac将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event_etc(&pst_hmac_vap->st_vap_base_info, pst_bsst_rsp_buf, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_bsst_rsp_buf);
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_tx_bsst_req_action::tx bsst rsp action frame failed.}");
        return ul_ret;
    }
    /* STA发送完Response后 一次交互流程就完成了 需要将user下的Token值加1 供下次发送使用 */
    if (HMAC_11V_TOKEN_MAX_VALUE == pst_11v_ctrl_info->uc_user_bsst_token)
    {
        pst_11v_ctrl_info->uc_user_bsst_token = 1;
    }
    else
    {
        pst_11v_ctrl_info->uc_user_bsst_token++;
    }

    return OAL_SUCC;
}


oal_uint16 hmac_encap_bsst_rsp_action(hmac_vap_stru *pst_hmac_vap,
                                      hmac_user_stru *pst_hmac_user,
                                      hmac_bsst_rsp_info_stru *pst_bsst_rsp_info,
                                      oal_netbuf_stru *pst_buffer)
{
    oal_uint16               us_index = 0;
    oal_uint8               *puc_mac_header   = OAL_PTR_NULL;
    oal_uint8               *puc_payload_addr = OAL_PTR_NULL;
    hmac_user_11v_ctrl_stru *pst_11v_ctrl_info;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == pst_bsst_rsp_info) || (OAL_PTR_NULL == pst_buffer))
    {
        OAM_ERROR_LOG4(0, OAM_SF_ANY, "{hmac_encap_bsst_rsp_action::null param.vap:%x user:%x info:%x buf:%x}",
            pst_hmac_vap, pst_hmac_user, pst_bsst_rsp_info, pst_buffer);
        return 0;
    }

    pst_11v_ctrl_info = &(pst_hmac_user->st_11v_ctrl_info);

    puc_mac_header   = oal_netbuf_header(pst_buffer);
    puc_payload_addr = mac_netbuf_get_payload(pst_buffer);
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

    /* Frame Control Field 中只需要设置Type/Subtype值，其他设置为0 */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);
    /* DA is address of STA addr */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_hmac_user->st_user_base_info.auc_user_mac_addr);
    /* SA的值为本身的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info));
    /* TA的值为VAP的BSSID */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_hmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************************************************/
    /*                                  Set the contents of the frame body                                       */
    /*************************************************************************************************************/
    /*************************************************************************************************************/
    /*                       BSS Transition Response Frame - Frame Body                                      */
    /* ----------------------------------------------------------------------------------------------------------*/
    /* |Category |Action | Token| Status Code | Termination Delay | Target BSSID |   BSS Candidate List Entry    */
    /* --------------------------------------------------------------------------------------------------------- */
    /* |1        |1      | 1    |  1          | 1                 | 0-6          |    Optional                   */
    /* --------------------------------------------------------------------------------------------------------- */
    /*                                                                                                           */
    /*************************************************************************************************************/

    /* 将索引指向frame body起始位置 */
    us_index = 0;
    /* 设置Category */
    puc_payload_addr[us_index] = MAC_ACTION_CATEGORY_WNM;
    us_index ++;
    /* 设置Action */
    puc_payload_addr[us_index] = MAC_WNM_ACTION_BSS_TRANSITION_MGMT_RESPONSE;
    us_index ++;
    /* 设置Dialog Token */
    puc_payload_addr[us_index] = pst_11v_ctrl_info->uc_user_bsst_token;
    us_index ++;
    /* 设置Status Code */
    puc_payload_addr[us_index] = pst_bsst_rsp_info->uc_status_code;
    us_index ++;
    /* 设置Termination Delay */
    puc_payload_addr[us_index] = pst_bsst_rsp_info->uc_termination_delay;
    us_index ++;
    /* 设置Target BSSID */
    if (0 == pst_bsst_rsp_info->uc_status_code)
    {
        oal_memcopy(puc_payload_addr+us_index, pst_bsst_rsp_info->auc_target_bss_addr, WLAN_MAC_ADDR_LEN);
        us_index += WLAN_MAC_ADDR_LEN;
    }
    /* 可选的候选AP列表 不添加 减少带宽占用 */
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_encap_bsst_rsp_action::LEN = %d.}", us_index + MAC_80211_FRAME_LEN);
#endif
    return (oal_uint16)(us_index + MAC_80211_FRAME_LEN);
}


hmac_neighbor_bss_info_stru * hmac_get_neighbor_ie(oal_uint8 *puc_data, oal_uint16 us_len, oal_uint8 *puc_bss_num)
{
    oal_uint8   *puc_ie_data_find = OAL_PTR_NULL;
    oal_uint8   *puc_ie_data = OAL_PTR_NULL;
    oal_uint16  us_len_find = us_len;
    oal_uint8   uc_minmum_ie_len = 13;
    oal_uint8   uc_bss_number = 0;
    oal_uint8   uc_bss_list_index = 0;
    oal_uint8   uc_neighbor_ie_len = 0;
    oal_int16   s_sub_ie_len = 0;
    hmac_neighbor_bss_info_stru *pst_bss_list_alloc;

    if ((OAL_PTR_NULL == puc_data) || (OAL_PTR_NULL == puc_bss_num) )
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_get_neighbor_ie::null pointer puc_data[%x] puc_bss_num[%x].}", puc_data, puc_bss_num);
        if (OAL_PTR_NULL != puc_bss_num)
        {
            *puc_bss_num = 0;
        }
        return OAL_PTR_NULL;
    }
    /* 传入的帧长度为0，则不需要进行解析了 */
    if (0 == us_len)
    {
        *puc_bss_num = 0;
        return OAL_PTR_NULL;
    }
    puc_ie_data_find = puc_data;

    /* 先确认含有多少个neighbor list */
    while (OAL_PTR_NULL != puc_ie_data_find)
    {
        puc_ie_data =  mac_find_ie_etc(MAC_EID_NEIGHBOR_REPORT, puc_ie_data_find, us_len_find);
        /* 没找到则退出循环 */
        if (OAL_PTR_NULL == puc_ie_data)
        {
            break;
        }
        uc_bss_number ++;                                   /* Neighbor Report IE 数量加1 */

        if (us_len_find >=  puc_ie_data[1] + MAC_IE_HDR_LEN)
        {
            puc_ie_data_find += (puc_ie_data[1] + MAC_IE_HDR_LEN);
            us_len_find -= (puc_ie_data[1] + MAC_IE_HDR_LEN);
        }
        else
        {
            OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_get_neighbor_ie::ie len[%d] greater than remain frame len[%d]!}", puc_ie_data[1] + MAC_IE_HDR_LEN, us_len_find);
            return OAL_PTR_NULL;
        }
    }
#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_get_neighbor_ie::find neighbor ie= [%d].}", uc_bss_number);
#endif
    /* 如果neighbor ie 长度为0 直接返回 */
    if (0 == uc_bss_number)
    {
        *puc_bss_num = 0;
        return OAL_PTR_NULL;
    }
    /* 数据还原后再次从头解析数据 */
    puc_ie_data_find = puc_data;
    us_len_find = us_len;
    pst_bss_list_alloc = (hmac_neighbor_bss_info_stru *) OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, uc_bss_number*OAL_SIZEOF(hmac_neighbor_bss_info_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_bss_list_alloc)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_get_neighbor_ie::pst_bss_list null pointer.}");
        *puc_bss_num = 0;
        return OAL_PTR_NULL;
    }
    for (uc_bss_list_index=0; uc_bss_list_index<uc_bss_number; uc_bss_list_index++)
    {
        /* 前面已经查询过一次，这里不会返回空，所以不作判断 */
        puc_ie_data =  mac_find_ie_etc(MAC_EID_NEIGHBOR_REPORT, puc_ie_data_find, us_len_find);
        if (OAL_PTR_NULL == puc_ie_data)
        {
            OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_get_neighbor_ie::cannot find ie,this should not occur.uc_bss_list_index[%d], uc_bss_number[%d].}",uc_bss_list_index, uc_bss_number);
            break;
        }

        uc_neighbor_ie_len = puc_ie_data[1];            // 元素长度
        /* 解析Neighbor Report IE结构体 帧中只含有subelement 3 4，其他subelement已被过滤掉 */
        oal_memcopy(pst_bss_list_alloc[uc_bss_list_index].auc_mac_addr, puc_ie_data+2, WLAN_MAC_ADDR_LEN);
        /* 解析BSSID Information */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_ap_reachability = (puc_ie_data[8]&BIT1)|(puc_ie_data[8]&BIT0);        /* bit0-1 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_security = (puc_ie_data[8]&BIT2)? OAL_TRUE: OAL_FALSE;                /* bit2 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_key_scope = (puc_ie_data[8]&BIT3)? OAL_TRUE: OAL_FALSE;               /* bit3 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_spectrum_mgmt  = (puc_ie_data[8]&BIT4)? OAL_TRUE: OAL_FALSE;          /* bit4 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_qos  = (puc_ie_data[8]&BIT5)? OAL_TRUE: OAL_FALSE;                    /* bit5 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_apsd = (puc_ie_data[8]&BIT6)? OAL_TRUE: OAL_FALSE;                    /* bit6 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_radio_meas = (puc_ie_data[8]&BIT7)? OAL_TRUE: OAL_FALSE;              /* bit7 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_delay_block_ack = (puc_ie_data[9]&BIT0)? OAL_TRUE: OAL_FALSE;         /* bit0 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_immediate_block_ack = (puc_ie_data[9]&BIT1)? OAL_TRUE: OAL_FALSE;     /* bit1 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_mobility_domain = (puc_ie_data[9]&BIT2)? OAL_TRUE: OAL_FALSE;         /* bit2 */
        pst_bss_list_alloc[uc_bss_list_index].st_bssid_info.bit_high_throughput = (puc_ie_data[9]&BIT3)? OAL_TRUE: OAL_FALSE;         /* bit3 */
        /* 保留字段不解析 */
        pst_bss_list_alloc[uc_bss_list_index].uc_opt_class = puc_ie_data[12];
        pst_bss_list_alloc[uc_bss_list_index].uc_chl_num = puc_ie_data[13];
        pst_bss_list_alloc[uc_bss_list_index].uc_phy_type = puc_ie_data[14];
        /* 解析Subelement 长度大于最小ie长度才存在subelement 只处理3 4 subelement */
        if (uc_neighbor_ie_len > uc_minmum_ie_len)
        {
            s_sub_ie_len = uc_neighbor_ie_len - uc_minmum_ie_len;        /* subelement长度 */
            puc_ie_data += (uc_minmum_ie_len + MAC_IE_HDR_LEN);           /* 帧体数据移动到subelement处 */
            while (0 < s_sub_ie_len)
            {
                switch (puc_ie_data[0])
                {
                case HMAC_NEIGH_SUB_ID_BSS_CANDIDATE_PERF:      /* 占用3个字节 */
                    {
                        pst_bss_list_alloc[uc_bss_list_index].uc_candidate_perf = puc_ie_data[2];
                        s_sub_ie_len -= (HMAC_11V_PERFERMANCE_ELEMENT_LEN +MAC_IE_HDR_LEN );
                        puc_ie_data += (HMAC_11V_PERFERMANCE_ELEMENT_LEN +MAC_IE_HDR_LEN );
                    }
                    break;
                case HMAC_NEIGH_SUB_ID_TERM_DURATION:   /* 占用12个字节 */
                    {
                        oal_memcopy(pst_bss_list_alloc[uc_bss_list_index].st_term_duration.auc_termination_tsf, puc_ie_data+2, 8);
                        pst_bss_list_alloc[uc_bss_list_index].st_term_duration.us_duration_min = (((oal_uint16)puc_ie_data[11])<<8) | (puc_ie_data[10]);
                        s_sub_ie_len -= (HMAC_11V_TERMINATION_ELEMENT_LEN + MAC_IE_HDR_LEN);
                        puc_ie_data +=  (HMAC_11V_TERMINATION_ELEMENT_LEN + MAC_IE_HDR_LEN);
                    }
                    break;
                /* 其他IE跳过 不处理 */
                default:
                    {
                        s_sub_ie_len -= (puc_ie_data[1] +MAC_IE_HDR_LEN );
                        puc_ie_data += (puc_ie_data[1] +MAC_IE_HDR_LEN );
                    }
                    break;
                }
            }
        }
        puc_ie_data_find += (uc_neighbor_ie_len + MAC_IE_HDR_LEN);
        us_len_find -= (uc_neighbor_ie_len + MAC_IE_HDR_LEN);
    }

    *puc_bss_num = uc_bss_number;

    return pst_bss_list_alloc;
}


#endif //_PRE_WLAN_FEATURE_11V_ENABLE

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

