

#ifndef __HMAC_BTCOEX_C__
#define __HMAC_BTCOEX_C__

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_vap.h"
#include "hmac_ext_if.h"
#include "mac_data.h"
#include "hmac_resource.h"
#include "hmac_btcoex.h"
#include "hmac_fsm.h"
#include "hmac_mgmt_sta.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_BTCOEX_C

/*****************************************************************************
  2 函数声明
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/


OAL_STATIC oal_uint32 hmac_btcoex_delba_foreach_tid(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, mac_cfg_delba_req_param_stru *pst_mac_cfg_delba_param)
{
    oal_uint32 ul_ret = 0;

    oal_set_mac_addr(pst_mac_cfg_delba_param->auc_mac_addr, pst_mac_user->auc_user_mac_addr);

    for (pst_mac_cfg_delba_param->uc_tidno = 0; pst_mac_cfg_delba_param->uc_tidno < WLAN_TID_MAX_NUM; pst_mac_cfg_delba_param->uc_tidno++) {

        ul_ret = hmac_config_delba_req(pst_mac_vap, 0, (oal_uint8 *)pst_mac_cfg_delba_param);

        if (OAL_SUCC != ul_ret) {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_delba_foreach_tid::ul_ret: %d, tid: %d}", ul_ret, pst_mac_cfg_delba_param->uc_tidno);
            return ul_ret;
        }
    }
    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_btcoex_delba_from_user(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user)
{
    oal_uint32 ul_ret = 0;
    mac_cfg_delba_req_param_stru st_mac_cfg_delba_param;

    st_mac_cfg_delba_param.en_direction = MAC_RECIPIENT_DELBA;

    if (OAL_FALSE == pst_hmac_user->st_hmac_user_btcoex.st_hmac_btcoex_addba_req.uc_delba_enable)
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{hmac_btcoex_delba_from_user::DO NOT DELBA.}");
        return OAL_FAIL;
    }

    ul_ret = hmac_btcoex_delba_foreach_tid(pst_mac_vap, &(pst_hmac_user->st_user_base_info), &st_mac_cfg_delba_param);

    return ul_ret;
}


oal_uint32 hmac_btcoex_rx_delba_trigger(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru *pst_hmac_vap;
    hmac_user_stru *pst_hmac_user;
    dmac_to_hmac_btcoex_rx_delba_trigger_event_stru *pst_dmac_to_hmac_btcoex_rx_delba;
    oal_uint32 ul_ret;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_rx_delba_trigger::pst_hmac_vap is null!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_to_hmac_btcoex_rx_delba = (dmac_to_hmac_btcoex_rx_delba_trigger_event_stru *)puc_param;

    pst_hmac_user = mac_res_get_hmac_user(pst_dmac_to_hmac_btcoex_rx_delba->us_user_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_user))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                        "{hmac_btcoex_rx_delba_trigger::pst_hmac_user is null! user_id is %d.}",
                        pst_dmac_to_hmac_btcoex_rx_delba->us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user->st_hmac_user_btcoex.us_ba_size = pst_dmac_to_hmac_btcoex_rx_delba->us_ba_size;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_rx_delba_trigger:delba size: %d}", pst_dmac_to_hmac_btcoex_rx_delba->us_ba_size);
    if (pst_dmac_to_hmac_btcoex_rx_delba->uc_need_delba)
    {
        ul_ret = hmac_btcoex_delba_from_user(pst_mac_vap, pst_hmac_user);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_rx_delba_trigger:delba send failed:ul_ret: %d}", ul_ret);
            return ul_ret;
        }
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_delba_send_timeout(oal_void *p_arg)
{
    hmac_user_stru *pst_hmac_user;
    hmac_user_btcoex_stru *pst_hmac_user_btcoex;
    hmac_btcoex_arp_req_process_stru *pst_hmac_btcoex_arp_req_process;
    oal_uint32 ui_val;

    pst_hmac_user = (hmac_user_stru *)p_arg;
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{hmac_delba_send_timeout::pst_hmac_user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user_btcoex = &(pst_hmac_user->st_hmac_user_btcoex);

    pst_hmac_btcoex_arp_req_process = &(pst_hmac_user_btcoex->st_hmac_btcoex_arp_req_process);

    ui_val = oal_atomic_read(&(pst_hmac_btcoex_arp_req_process->ul_rx_unicast_pkt_to_lan));
    if(0 == ui_val)
    {
        mac_vap_stru *pst_mac_vap;

        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_hmac_user->st_user_base_info.uc_vap_id);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_COEX, "{hmac_delba_send_timeout::pst_mac_vap is null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_delba_send_timeout:rx_pkt:0}");

        pst_hmac_user_btcoex->uc_rx_no_pkt_count++;
        if (pst_hmac_user_btcoex->uc_rx_no_pkt_count > 2)
        {
            hmac_btcoex_delba_from_user(pst_mac_vap, pst_hmac_user);
        }
    }
    else
    {
        pst_hmac_user_btcoex->uc_rx_no_pkt_count = 0;
    }

    oal_atomic_set(&pst_hmac_btcoex_arp_req_process->ul_rx_unicast_pkt_to_lan, 0);

    return OAL_SUCC;
}


oal_void hmac_btcoex_arp_fail_delba_process(oal_netbuf_stru *pst_netbuf, mac_vap_stru *pst_mac_vap)
{

    oal_uint8              uc_data_type;
    hmac_btcoex_arp_req_process_stru *pst_hmac_btcoex_arp_req_process;
    hmac_user_stru *pst_hmac_user;
    hmac_user_btcoex_stru *pst_hmac_user_btcoex;
    mac_ether_header_stru *pst_mac_ether_hdr;

    if (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        return;
    }

    pst_mac_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);

    pst_hmac_user = mac_res_get_hmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_user))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                    "{hmac_btcoex_arp_fail_delba_process::pst_hmac_user is null!assoc_vap is %d.}",
                    pst_mac_vap->uc_assoc_vap_id);
        return;
    }

    pst_hmac_user_btcoex = &(pst_hmac_user->st_hmac_user_btcoex);

    if((pst_hmac_user_btcoex->us_ba_size > 0) && (pst_hmac_user_btcoex->us_ba_size < WLAN_AMPDU_RX_BA_LUT_WSIZE))
    {
        /* 参数外面已经做检查，里面没必要再做检查了 */
        uc_data_type =  mac_get_data_type_from_8023((oal_uint8 *)pst_mac_ether_hdr, MAC_NETBUFF_PAYLOAD_ETH);

        pst_hmac_btcoex_arp_req_process = &(pst_hmac_user_btcoex->st_hmac_btcoex_arp_req_process);

        /* 发送方向创建定时器 */
        if((MAC_DATA_ARP_REQ == uc_data_type) && (OAL_FALSE == pst_hmac_btcoex_arp_req_process->st_delba_opt_timer.en_is_registerd))
        {
            /* 每次重启定时器之前清零,保证统计的时间 */
            oal_atomic_set(&(pst_hmac_btcoex_arp_req_process->ul_rx_unicast_pkt_to_lan), 0);

            FRW_TIMER_CREATE_TIMER(&(pst_hmac_btcoex_arp_req_process->st_delba_opt_timer),
                       hmac_delba_send_timeout,
                       300,
                       pst_hmac_user,
                       OAL_FALSE,
                       OAM_MODULE_ID_HMAC,
                       pst_mac_vap->ul_core_id);


        }
    }
}


oal_uint32 hmac_btcoex_check_exception_in_list(hmac_vap_stru *pst_hmac_vap, oal_uint8 *auc_addr)
{
    hmac_btcoex_delba_exception_stru *pst_btcoex_exception;
    oal_uint8 uc_index;
    hmac_device_stru *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_check_exception_in_list::pst_hmac_device null}");
        return OAL_FALSE;
    }

    for (uc_index = 0; uc_index < MAX_BTCOEX_BSS_IN_BL; uc_index++)
    {
        pst_btcoex_exception = &(pst_hmac_device->st_hmac_device_btcoex.ast_hmac_btcoex_delba_exception[uc_index]);

        if ((pst_btcoex_exception->uc_used != 0)
            && (0 == oal_compare_mac_addr(pst_btcoex_exception->auc_user_mac_addr, auc_addr)))
        {
            OAM_WARNING_LOG4(0, OAM_SF_COEX,
                "{hmac_btcoex_check_exception_in_list::Find in blacklist, addr->%02x:%02x:XX:XX:%02x:%02x.}",
                auc_addr[0], auc_addr[1], auc_addr[4], auc_addr[5]);
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


OAL_STATIC oal_void hmac_btcoex_add_exception_to_list(hmac_vap_stru *pst_hmac_vap, oal_uint8 *auc_mac_addr)
{
    hmac_btcoex_delba_exception_stru *pst_btcoex_exception;
    hmac_device_btcoex_stru *pst_hmac_device_btcoex;
    hmac_device_stru *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_check_exception_in_list::pst_hmac_device null}");
        return;
    }

    pst_hmac_device_btcoex = &(pst_hmac_device->st_hmac_device_btcoex);

    if (pst_hmac_device_btcoex->uc_exception_bss_index >= MAX_BTCOEX_BSS_IN_BL)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_add_exception_to_list::already reach max num:%d.}", MAX_BTCOEX_BSS_IN_BL);
        pst_hmac_device_btcoex->uc_exception_bss_index = 0;
    }
    pst_btcoex_exception = &(pst_hmac_device_btcoex->ast_hmac_btcoex_delba_exception[pst_hmac_device_btcoex->uc_exception_bss_index]);
    oal_set_mac_addr(pst_btcoex_exception->auc_user_mac_addr, auc_mac_addr);
    pst_btcoex_exception->uc_type = 0;
    pst_btcoex_exception->uc_used = 1;

    pst_hmac_device_btcoex->uc_exception_bss_index++;
}


oal_void hmac_btcoex_check_rx_same_baw_start_from_addba_req(hmac_vap_stru *pst_hmac_vap,
                                                                                    hmac_user_stru *pst_hmac_user,
                                                                                    mac_ieee80211_frame_stru *pst_frame_hdr,
                                                                                    oal_uint8 *puc_action)
{
    hmac_user_btcoex_stru *pst_hmac_user_btcoex;
    hmac_btcoex_addba_req_stru *pst_hmac_btcoex_addba_req;
    oal_uint16 us_baw_start;
    oal_uint8 uc_tid;

    pst_hmac_user_btcoex = &(pst_hmac_user->st_hmac_user_btcoex);

    pst_hmac_btcoex_addba_req = &(pst_hmac_user_btcoex->st_hmac_btcoex_addba_req);

    /* 两次收到addba req的start num一样且不是重传帧，认为对端移窗卡死  */
    if (OAL_TRUE == pst_frame_hdr->st_frame_control.bit_retry
        && pst_frame_hdr->bit_seq_num == pst_hmac_btcoex_addba_req->us_last_seq_num)
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX,
                "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::retry addba req.}");
        return;
    }

    /******************************************************************/
    /*       ADDBA Request Frame - Frame Body                         */
    /* ---------------------------------------------------------------*/
    /* | Category | Action | Dialog | Parameters | Timeout | SSN     |*/
    /* ---------------------------------------------------------------*/
    /* | 1        | 1      | 1      | 2          | 2       | 2       |*/
    /* ---------------------------------------------------------------*/
    /*                                                                */
    /******************************************************************/
    us_baw_start = (puc_action[7] >> 4) | (puc_action[8] << 4);
    uc_tid = (puc_action[3] & 0x3C) >> 2;
    if (uc_tid != 0)
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX,
                "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::uc_tid != 0.}");
        return;
    }

    if ((us_baw_start != 0)
        && (us_baw_start == pst_hmac_btcoex_addba_req->us_last_baw_start)
        && (pst_hmac_user_btcoex->uc_rx_no_pkt_count > 2))
    {
        OAM_WARNING_LOG1(0, OAM_SF_COEX,
                    "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::baw_start:%d, delba will forbidden.}", us_baw_start);
        pst_hmac_btcoex_addba_req->uc_delba_enable = OAL_FALSE;

        if (OAL_FALSE == hmac_btcoex_check_exception_in_list(pst_hmac_vap, pst_frame_hdr->auc_address2))
        {
            OAM_WARNING_LOG0(0, OAM_SF_COEX,
                    "{hmac_btcoex_check_rx_same_baw_start_from_addba_req::write down to file.}");
            hmac_btcoex_add_exception_to_list(pst_hmac_vap, pst_frame_hdr->auc_address2);
        }

        /* 发送去认证帧到AP */
        hmac_mgmt_send_disassoc_frame(&pst_hmac_vap->st_vap_base_info,
                                    pst_frame_hdr->auc_address2,
                                    MAC_UNSPEC_REASON,
                                    pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active);

        /* 删除对应用户 */
        hmac_user_del(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);
        /* 设置状态为FAKE UP */
        hmac_fsm_change_state(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        hmac_sta_handle_disassoc_rsp(pst_hmac_vap, MAC_AUTH_NOT_VALID);
    }

    pst_hmac_btcoex_addba_req->us_last_baw_start = us_baw_start;
    pst_hmac_btcoex_addba_req->us_last_seq_num = pst_frame_hdr->bit_seq_num;
}



oal_uint32  hmac_config_print_btcoex_status(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    hmac_btcoex_delba_exception_stru *pst_btcoex_exception;
    oal_uint8 uc_index;
    hmac_device_stru *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{hmac_btcoex_check_exception_in_list::pst_hmac_device null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_BTCOEX_STATUS_PRINT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                        "{hmac_config_print_btcoex_status::send event return err code [%d].}", ul_ret);
    }

    for (uc_index = 0; uc_index < MAX_BTCOEX_BSS_IN_BL; uc_index++)
    {
        pst_btcoex_exception = &(pst_hmac_device->st_hmac_device_btcoex.ast_hmac_btcoex_delba_exception[uc_index]);
        if (pst_btcoex_exception->uc_used != 0)
        {
            OAM_WARNING_LOG4(0, OAM_SF_COEX,
                    "{hmac_config_print_btcoex_status::addr->%02x:%02x:XX:XX:%02x:%02x.}",
                    pst_btcoex_exception->auc_user_mac_addr[0],
                    pst_btcoex_exception->auc_user_mac_addr[1],
                    pst_btcoex_exception->auc_user_mac_addr[4],
                    pst_btcoex_exception->auc_user_mac_addr[5]);
        }
    }

    return ul_ret;
}


oal_uint32 hmac_btcoex_check_by_ba_size(hmac_user_stru *pst_hmac_user)
{
    hmac_user_btcoex_stru *pst_hmac_user_btcoex;

    pst_hmac_user_btcoex = &(pst_hmac_user->st_hmac_user_btcoex);

    if ((pst_hmac_user_btcoex->us_ba_size > 0) && (pst_hmac_user_btcoex->us_ba_size < WLAN_AMPDU_RX_BA_LUT_WSIZE))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
#endif /* #ifdef _PRE_WLAN_FEATURE_COEXIST_BT */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of __HMAC_BTCOEX_C__ */

