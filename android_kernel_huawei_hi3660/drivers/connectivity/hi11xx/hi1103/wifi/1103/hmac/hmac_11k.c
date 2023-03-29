


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_11k.h"
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
#include "hmac_mgmt_bss_comm.h"
#include "hmac_device.h"
#include "mac_device.h"
#include "mac_vap.h"
#include "hmac_scan.h"
#include "hmac_config.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_11k_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
hmac_rrm_rpt_hook_stru gst_rrm_rpt_hook;
oal_uint8              guc_dialog_token = 0;
oal_uint8              guc_meas_token   = 0;

OAL_STATIC oal_uint32 hmac_wait_meas_rsp_timeout(oal_void *p_arg);
OAL_STATIC oal_uint32  hmac_rrm_chn_load_scan_do(hmac_vap_stru *pst_hmac_vap, mac_chn_load_req_stru *pst_chn_load_req);
OAL_STATIC oal_void hmac_rrm_chn_load_scan_cb(void *p_scan_record);
OAL_STATIC oal_uint32 hmac_rrm_bcn_scan_do(hmac_vap_stru *pst_hmac_vap, mac_bcn_req_stru *pst_bcn_req, mac_scan_req_stru *pst_scan_req);
OAL_STATIC oal_void hmac_rrm_bcn_scan_cb(void *p_scan_record);
OAL_STATIC oal_void hmac_rrm_encap_meas_rpt_bcn(hmac_vap_stru *pst_hmac_vap, oal_dlist_head_stru *pst_bss_list_head);
OAL_STATIC oal_void hmac_rrm_get_bcn_info_from_rpt(hmac_user_stru *pst_hmac_user, mac_meas_rpt_ie_stru *pst_meas_rpt_ie);
OAL_STATIC oal_uint32 hmac_rrm_fill_basic_rm_rpt_action(hmac_vap_stru *pst_hmac_vap);
OAL_STATIC oal_uint32 hmac_rrm_send_rm_rpt_action(hmac_vap_stru* pst_hmac_vap);
OAL_STATIC oal_void hmac_rrm_parse_beacon_req(hmac_vap_stru *pst_hmac_vap, mac_meas_req_ie_stru  *pst_meas_req_ie);
OAL_STATIC oal_void hmac_rrm_parse_chn_load_req(hmac_vap_stru *pst_hmac_vap, mac_meas_req_ie_stru  *pst_meas_req_ie);
OAL_STATIC oal_uint32 hmac_rrm_rpt_notify(hmac_user_stru *pst_user);
OAL_STATIC oal_void hmac_rrm_neighbor_scan_cb(void *p_scan_record);
OAL_STATIC oal_void hmac_rrm_encap_meas_rpt_neighbor(hmac_vap_stru *pst_hmac_vap, oal_dlist_head_stru *pst_bss_list_head);
OAL_STATIC oal_void hmac_rrm_encap_meas_rpt_reject(hmac_vap_stru *pst_hmac_vap, mac_meas_rpt_mode_stru *pst_rptmode);
OAL_STATIC oal_void hmac_rrm_get_chn_load_info_from_rpt(hmac_user_stru *pst_hmac_user, mac_meas_rpt_ie_stru *pst_meas_rpt_ie);
OAL_STATIC oal_void hmac_rrm_get_neighbor_info_from_rpt(hmac_user_stru *pst_hmac_user, mac_neighbor_rpt_ie_stru *pst_neighbor_rpt_ie);
OAL_STATIC void hmac_scan_update_bcn_rpt_detail(hmac_vap_stru *pst_hmac_vap,
                                                mac_bss_dscr_stru *pst_bss_dscr,
                                                oal_uint8 *puc_rpt_detail_data,
                                                oal_uint32 *pul_rpt_detail_act_len);
OAL_STATIC oal_uint32 hmac_rrm_bcn_rpt_filter(hmac_vap_stru *pst_hmac_vap, mac_bss_dscr_stru *pst_bss_dscr, mac_scan_req_stru *pst_scan_params);
OAL_STATIC oal_uint32 hmac_rrm_encap_meas_rpt_refuse_new_req(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
        mac_action_rm_req_stru *pst_rm_req);
OAL_STATIC oal_uint32 hmac_rrm_check_user_cap(mac_rrm_enabled_cap_ie_stru *pst_rrm_enabled_cap, mac_rrm_req_cfg_stru *pst_req_cfg);
OAL_STATIC oal_uint32 hmac_rrm_encap_local_bssid_rpt(hmac_vap_stru *pst_hmac_vap);

/*****************************************************************************
  3 函数实现
*****************************************************************************/
#define HMAC_11K_INIT

oal_void hmac_11k_init_vap(hmac_vap_stru *pst_hmac_vap)
{
    if(OAL_TRUE == pst_hmac_vap->bit_11k_enable
#ifdef _PRE_WLAN_FEATURE_P2P
       && WLAN_P2P_CL_MODE != pst_hmac_vap->st_vap_base_info.en_p2p_mode
#endif
      )
    {
        pst_hmac_vap->pst_rrm_info = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_vap_rrm_info_stru), OAL_TRUE);
        if (OAL_PTR_NULL == pst_hmac_vap->pst_rrm_info)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_11k_init_vap::pst_rrm_info null.}");
            return;
        }
        OAL_MEMZERO(pst_hmac_vap->pst_rrm_info, OAL_SIZEOF(mac_vap_rrm_info_stru));
    }

    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_11k_init_vap!}");

}


oal_void hmac_11k_exit_vap(hmac_vap_stru *pst_hmac_vap)
{
    if(OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info)
    {
        if(OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info->st_neighbor_req_info.puc_ssid)
        {
            OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info->st_neighbor_req_info.puc_ssid, OAL_TRUE);
        }
        if(OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_ssid)
        {
            OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_ssid, OAL_TRUE);
        }
        if(OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid)
        {
            OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid, OAL_TRUE);
        }

        if (OAL_TRUE == pst_hmac_vap->pst_rrm_info->st_meas_status_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_vap->pst_rrm_info->st_meas_status_timer);
        }

        OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info, OAL_TRUE);
        pst_hmac_vap->pst_rrm_info = OAL_PTR_NULL;
    }

    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_11k_exit_vap!}");
}


oal_void hmac_11k_init_user(hmac_user_stru *pst_hmac_user)
{
    /*关联时申请用户结构体空间*/
    if (OAL_PTR_NULL == pst_hmac_user->pst_user_rrm_info)
    {
        pst_hmac_user->pst_user_rrm_info = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_user_rrm_info_stru), OAL_TRUE);
        if (OAL_PTR_NULL == pst_hmac_user->pst_user_rrm_info)
        {
            OAM_ERROR_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_11k_init_user::pst_user_rrm_info null.}");
            return;
        }

        OAL_MEMZERO(pst_hmac_user->pst_user_rrm_info, OAL_SIZEOF(mac_user_rrm_info_stru));
        oal_dlist_init_head(&(pst_hmac_user->pst_user_rrm_info->st_meas_rpt_list));
    }

    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_11k_init_user!}");
}



oal_void hmac_11k_exit_user(hmac_user_stru *pst_hmac_user)
{
    if(OAL_PTR_NULL != pst_hmac_user->pst_user_rrm_info)
    {
        if (OAL_TRUE == pst_hmac_user->pst_user_rrm_info->st_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_user->pst_user_rrm_info->st_timer);
        }

        /*释放链表和恢复状态*/
        hmac_rrm_free_rpt_list(pst_hmac_user, pst_hmac_user->pst_user_rrm_info->en_reqtype);

        OAL_MEM_FREE(pst_hmac_user->pst_user_rrm_info, OAL_TRUE);
        pst_hmac_user->pst_user_rrm_info = OAL_PTR_NULL;
    }

    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_11k_exit_user!}");
}
#define HMAC_11K_EXTERNAL_FUNC

oal_uint32  hmac_register_rrm_rpt_notify_func(mac_rrm_rpt_notify_enum en_notify_sub_type,
       mac_rrm_type_enum en_cur_reqtype, p_rrm_rpt_notify_func p_func)
{
    if(en_notify_sub_type >= HMAC_RRM_RPT_NOTIFY_BUTT || en_cur_reqtype >= MAC_RRM_MEAS_TYPE_BUTT)
    {
        return OAL_FAIL;
    }

    if(OAL_PTR_NULL == p_func)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    gst_rrm_rpt_hook.pa_rrm_rpt_notify_func[en_notify_sub_type][en_cur_reqtype] = p_func;

    return OAL_SUCC;
}


oal_uint32 hmac_rrm_bcn_rpt_notify_hook(hmac_user_stru *pst_hmac_user, mac_rrm_state_enum en_rpt_state)
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
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_bcn_rpt_notify_hook:pst_rrm_info->st_meas_rpt_list is null, en_meas_status[%d].",
            pst_rrm_info->en_meas_status);

        OAM_WARNING_LOG3(0, OAM_SF_RRM, "{hmac_rrm_bcn_rpt_notify_hook::bit_late[%d],bit_incapable[%d],bit_refused[%d]!}",
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

        OAM_WARNING_LOG4(0, OAM_SF_RRM, "{hmac_rrm_bcn_rpt_notify_hook::rcpi=%x, rssi=%d, mac_addr=%x:%x!}",
            pst_meas_rpt_bcn_item->uc_rcpi,
            (oal_int8)(pst_meas_rpt_bcn_item->uc_rcpi/2 - 110),
            pst_meas_rpt_bcn_item->auc_bssid[4],
            pst_meas_rpt_bcn_item->auc_bssid[5]);
    }

    return OAL_SUCC;

}


oal_uint32 hmac_rrm_proc_rm_request(hmac_vap_stru* pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf)
{
    mac_action_rm_req_stru          *pst_rm_req;
    mac_meas_req_ie_stru            *pst_meas_req_ie;
    dmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint16                      us_framebody_len;
    mac_vap_rrm_info_stru           *pst_rrm_info;
    mac_meas_rpt_mode_stru          st_rptmode;

    OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));

    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == pst_hmac_user || OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG3(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_request::input is NULL, pst_hmac_vap[%p],pst_hmac_user[%p],pst_netbuf[%p]!}",
            pst_hmac_vap, pst_hmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_request::pst_rrm_info is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rx_ctrl      = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_request::pst_rx_ctrl is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_framebody_len = MAC_GET_RX_CB_PAYLOAD_LEN(&(pst_rx_ctrl->st_rx_info));
    pst_rm_req = (mac_action_rm_req_stru *)mac_netbuf_get_payload(pst_netbuf);

    /*************************************************************************/
    /*                    Radio Measurement Request Frame - Frame Body       */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Number of Repetitions|Meas Req Eles |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          | 2                    |var            */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    /*判断VAP是否正在进行测量*/
    if ( OAL_TRUE == pst_rrm_info->en_is_measuring)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_request::vap is handling one request now.}");
        hmac_rrm_encap_meas_rpt_refuse_new_req(pst_hmac_vap, pst_hmac_user, pst_rm_req);
        return OAL_FAIL;
    }

    /*保存req*/
    pst_rrm_info->uc_action_code  = pst_rm_req->uc_action_code;
    pst_rrm_info->uc_dialog_token = pst_rm_req->uc_dialog_token;
    pst_rrm_info->us_req_user_id  = pst_hmac_user->st_user_base_info.us_assoc_id;

    /* 是否有Meas Req */
    if (us_framebody_len <= MAC_RADIO_MEAS_ACTION_REQ_FIX_LEN)
    {
        /* 如果没有MR IE，也回一个不带Meas Rpt的Radio Meas Rpt */
        /* 申请管理帧内存并填充头部信息 */
        if ( OAL_SUCC != hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap))
        {
            return OAL_FAIL;
        }
        hmac_rrm_send_rm_rpt_action(pst_hmac_vap);
        return OAL_FAIL;
    }

    pst_meas_req_ie = (mac_meas_req_ie_stru *)&(pst_rm_req->auc_req_ies[0]);
    pst_rrm_info->pst_meas_req_ie = pst_meas_req_ie;

    /* 重复测试次数暂不处理，如果对端要求重复测试，则回复incapable bit */
    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_proc_rm_request::framebody::Category[%d],Action[%d],Dialog Token[%d],Number of Repetitions[%d].",
        pst_rm_req->uc_category,
        pst_rm_req->uc_action_code,
        pst_rm_req->uc_dialog_token,
        pst_rm_req->us_num_rpt);

    if (0 != pst_rm_req->us_num_rpt
        && OAL_FALSE == mac_mib_get_dot11RMRepeatedMeasurementsActivated(&pst_hmac_vap->st_vap_base_info))
    {
        st_rptmode.bit_incapable = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_proc_rm_request::RepeatedMeasurements not support, num_rpt[%u].",
                pst_rm_req->us_num_rpt);
        return OAL_FAIL;
    }

    /*************************************************************************/
    /*                    Measurement Request IE                             */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Req Mode|Meas Type  | Meas Req |*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        | 1            |1          |var       |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    /* TO BE DONE:可能有多个Measurement Req IEs */
    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_proc_rm_request::framebody::Element ID[%d],Length[%d],Meas Token[%d],Meas Req Mode[%d].",
        pst_meas_req_ie->uc_eid,
        pst_meas_req_ie->uc_len,
        pst_meas_req_ie->uc_token,
        *((oal_uint8*)(&(pst_meas_req_ie->st_reqmode))));
    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_proc_rm_request::framebody::Meas Type[%d].",
        pst_meas_req_ie->uc_reqtype);

    if (MAC_EID_MEASREQ == pst_meas_req_ie->uc_eid)
    {
        if(1 == pst_meas_req_ie->st_reqmode.bit_enable)
        {
            /* Req中拒绝某种类型的测量,一旦对端拒绝，在当前关联状态无法恢复测量能力位，需要重新关联 */
            if (0 == pst_meas_req_ie->st_reqmode.bit_request)
            {
                OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_request::update user rrm capability,reqtype[%d] not support!}",
                        pst_meas_req_ie->uc_reqtype);
                if(RM_RADIO_MEAS_BCN == pst_meas_req_ie->uc_reqtype)
                {
                    pst_hmac_user->st_user_base_info.st_rrm_enabled_cap.bit_bcn_active_cap  = 0;
                    pst_hmac_user->st_user_base_info.st_rrm_enabled_cap.bit_bcn_passive_cap = 0;
                    pst_hmac_user->st_user_base_info.st_rrm_enabled_cap.bit_bcn_table_cap   = 0;
                    pst_hmac_user->st_user_base_info.st_rrm_enabled_cap.bit_bcn_meas_rpt_cond_cap = 0;
                }
                else if(RM_RADIO_MEAS_CHANNEL_LOAD == pst_meas_req_ie->uc_reqtype)
                {
                    pst_hmac_user->st_user_base_info.st_rrm_enabled_cap.bit_chn_load_cap    = 0;
                }
            }

            /* Req中不允许发送对应的report */
            if (0 == pst_meas_req_ie->st_reqmode.bit_rpt)
            {
                OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_request::user not expect rpt!}");
                return OAL_FAIL;
            }
        }

        /*并行测试暂不处理，如果对端要求则回复incapable bit*/
        if (0 != pst_meas_req_ie->st_reqmode.bit_parallel
            && OAL_FALSE == mac_mib_get_dot11RMParallelMeasurementsActivated(&pst_hmac_vap->st_vap_base_info))
        {
            st_rptmode.bit_incapable = 1;
            hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_proc_rm_request::ParallelMeasurement not support.");
            return OAL_FAIL;
        }

        /* 处理Beacon req */
        if(RM_RADIO_MEAS_BCN == pst_meas_req_ie->uc_reqtype)
        {
            pst_rrm_info->st_bcn_req_info.uc_meas_token = pst_meas_req_ie->uc_token;
            pst_rrm_info->st_bcn_req_info.uc_meas_type  = pst_meas_req_ie->uc_reqtype;
            pst_rrm_info->st_bcn_req_info.us_repetition = OAL_NTOH_16(pst_rm_req->us_num_rpt);
            pst_rrm_info->st_bcn_req_info.uc_dialog_token = pst_rm_req->uc_dialog_token;
            hmac_rrm_parse_beacon_req(pst_hmac_vap, pst_meas_req_ie);
        }
        /* 处理Chn load req */
        else if(RM_RADIO_MEAS_CHANNEL_LOAD == pst_meas_req_ie->uc_reqtype)
        {
            hmac_rrm_parse_chn_load_req(pst_hmac_vap, pst_meas_req_ie);
        }
        else
        {
            st_rptmode.bit_incapable = 1;
            hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "{Error Request, Expect Measurement Request, but got reqtype[%d]!}", pst_meas_req_ie->uc_reqtype);
            return OAL_FAIL;
        }
    }
    /* MR IE错误，不回，报错 */
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "{Error Request, Expect Measurement Request, but got EID[%d]!}", pst_meas_req_ie->uc_eid);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_rrm_proc_rm_report(hmac_vap_stru* pst_hmac_vap,  hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf)
{
    mac_action_rm_rpt_stru          *pst_rm_rpt;
    mac_meas_rpt_ie_stru            *pst_meas_rpt_ie;
    dmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_int16                       s_framebody_len;
    mac_user_rrm_info_stru          *pst_rrm_info;
    oal_uint8                       uc_rpttype = RM_RADIO_MEAS_BCN;

    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == pst_hmac_user || OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG3(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::input is NULL, pst_hmac_vap[%p],pst_hmac_user[%p],pst_netbuf[%p]!}",
            pst_hmac_vap, pst_hmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_hmac_user->pst_user_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::pst_rrm_info is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rx_ctrl      = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::pst_rx_ctrl is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    s_framebody_len = (oal_int16)MAC_GET_RX_CB_PAYLOAD_LEN(&(pst_rx_ctrl->st_rx_info));
    pst_rm_rpt = (mac_action_rm_rpt_stru *)mac_netbuf_get_payload(pst_netbuf);

    /*************************************************************************/
    /*                    Radio Measurement Request Frame - Frame Body       */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Number of Repetitions|Meas Req Eles |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          | 2                    |var            */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    pst_meas_rpt_ie = (mac_meas_rpt_ie_stru*)pst_rm_rpt->auc_rpt_ies;

    /*check dialog token is same*/
    if ( pst_rm_rpt->uc_dialog_token != pst_rrm_info->uc_dialog_token
        || MAC_RRM_STATE_WAIT_RSP != pst_hmac_user->pst_user_rrm_info->en_meas_status)
    {
        OAM_WARNING_LOG3(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::uc_dialog_token diff: request[%d], report[%d]},en_meas_status[%d].",
            pst_rrm_info->uc_dialog_token, pst_rm_rpt->uc_dialog_token,
            pst_hmac_user->pst_user_rrm_info->en_meas_status);
        return OAL_FAIL;
    }

    if(s_framebody_len < WLAN_MEM_NETBUF_SIZE2 - MAC_80211_FRAME_LEN - MAC_BEACON_RPT_FIX_LEN)
    {
        /* 取消定时器 */
        if (OAL_TRUE == pst_hmac_user->pst_user_rrm_info->st_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_user->pst_user_rrm_info->st_timer));
            OAM_WARNING_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_proc_rm_report:: cancel Timer.}");
        }
    }
    else
    {
        OAM_WARNING_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_proc_rm_report:: wait for another report, s_framebody_len=%d.}",
            (oal_uint16)s_framebody_len);
    }

    s_framebody_len -= MAC_ACTION_RPT_FIX_LEN;

    do
    {
        /*************************************************************************/
        /*                    Measurement Report IE                             */
        /* --------------------------------------------------------------------- */
        /* |Element ID |Length |Meas Token| Meas Rpt Mode|Meas Type  | Meas Req |*/
        /* --------------------------------------------------------------------- */
        /* |1          |1      | 1        | 1            |1          |var       |*/
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/
        /* TO BE DONE:可能有多个Measurement Req IEs */
        if (MAC_EID_MEASREP == pst_meas_rpt_ie->uc_eid)
        {
            uc_rpttype = pst_meas_rpt_ie->uc_rpttype;

            /* check Measurement Report field exist*/
            if ((1 == pst_meas_rpt_ie->st_rptmode.bit_late) || (1 == pst_meas_rpt_ie->st_rptmode.bit_incapable)
                || (1 == pst_meas_rpt_ie->st_rptmode.bit_refused))
            {
                OAM_WARNING_LOG4(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::rpt now allowed, reqtype[%d],bit_late[%d],bit_incapable[%d],bit_refused[%d]!}",
                        pst_meas_rpt_ie->uc_rpttype,
                        pst_meas_rpt_ie->st_rptmode.bit_late,
                        pst_meas_rpt_ie->st_rptmode.bit_incapable,
                        pst_meas_rpt_ie->st_rptmode.bit_refused);
                s_framebody_len -= pst_meas_rpt_ie->uc_len + MAC_IE_HDR_LEN;

                /*record Measurement Report field*/
                oal_memcopy(&(pst_rrm_info->st_rptmode), &(pst_meas_rpt_ie->st_rptmode), OAL_SIZEOF(mac_meas_rpt_mode_stru));

                break;
            }

            /*check Meas token is same*/
            if ( pst_meas_rpt_ie->uc_token != pst_rrm_info->uc_meas_token)
            {
                OAM_WARNING_LOG2(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::meas token diff,req meas token[%d],rpr meas token[%d]!}",
                    pst_rrm_info->uc_meas_token, pst_meas_rpt_ie->uc_token);
                s_framebody_len -= pst_meas_rpt_ie->uc_len + MAC_IE_HDR_LEN;
                break;
            }

            /*判断Measurement Report Filed是否为空*/
            if ( pst_meas_rpt_ie->uc_len <= MAC_MEASUREMENT_RPT_FIX_LEN - MAC_IE_HDR_LEN )
            {
                OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::Measurement Report filed empty, length=%d!}",
                    pst_meas_rpt_ie->uc_len);
                break;
            }

            /*beacon report */
            if (RM_RADIO_MEAS_BCN == pst_meas_rpt_ie->uc_rpttype)
            {
                hmac_rrm_get_bcn_info_from_rpt(pst_hmac_user, pst_meas_rpt_ie);
            }
            /*channel load report */
            else if(RM_RADIO_MEAS_CHANNEL_LOAD == pst_meas_rpt_ie->uc_rpttype)
            {
                hmac_rrm_get_chn_load_info_from_rpt(pst_hmac_user, pst_meas_rpt_ie);
            }
            else
            {
                OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_proc_rm_report::rpttype[%d] not support!}", pst_meas_rpt_ie->uc_rpttype);
            }
        }
        /* MR IE错误*/
        else
        {
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "{Error Request, Expect Measurement Request, but got EID[%d]!}", pst_meas_rpt_ie->uc_eid);
        }

        s_framebody_len -= pst_meas_rpt_ie->uc_len + MAC_IE_HDR_LEN;

        /* 下一个Measurement Report的位置 */
        pst_meas_rpt_ie = (mac_meas_rpt_ie_stru *)((oal_uint8 *)pst_meas_rpt_ie + pst_meas_rpt_ie->uc_len + 2);

    }while(s_framebody_len>0);

    if ( OAL_TRUE == pst_hmac_user->pst_user_rrm_info->st_timer.en_is_enabled)
    {
        return OAL_SUCC;
    }

    pst_hmac_user->pst_user_rrm_info->en_meas_status = MAC_RRM_STATE_GET_RSP;
    /*收发类型一致则通知对应模块*/
    if((RM_RADIO_MEAS_BCN == uc_rpttype && MAC_RRM_TYPE_BCN == pst_hmac_user->pst_user_rrm_info->en_reqtype)
        ||(RM_RADIO_MEAS_CHANNEL_LOAD == uc_rpttype && MAC_RRM_TYPE_CHANNEL_LOAD == pst_hmac_user->pst_user_rrm_info->en_reqtype))
    {
        hmac_rrm_rpt_notify(pst_hmac_user);
    }

    /*释放链表和恢复状态*/
    hmac_rrm_free_rpt_list(pst_hmac_user, pst_rrm_info->en_reqtype);

    return OAL_SUCC;
}


oal_uint32 hmac_rrm_proc_neighbor_request(hmac_vap_stru* pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf)
{
    mac_action_neighbor_req_stru    *pst_neighbor_req;
    dmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint16                      us_framebody_len;
    mac_vap_rrm_info_stru           *pst_rrm_info;
    oal_uint8                       *puc_ssid_search_addr;
    oal_uint8                       *puc_ssid_sub_element;

    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == pst_hmac_user || OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG3(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::input is NULL, pst_hmac_vap[%p],pst_hmac_user[%p],pst_netbuf[%p]!}",
            pst_hmac_vap, pst_hmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*是否支持neighbor Report测试*/
    if (OAL_FALSE == mac_mib_get_dot11RMNeighborReportActivated(&pst_hmac_vap->st_vap_base_info))
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::NeighborReport not support!}");
        return OAL_FAIL;
    }

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::pst_rrm_info is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*判断VAP是否正在进行测量*/
    if ( OAL_TRUE == pst_rrm_info->en_is_measuring)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::vap is handling one request now.}");
        return OAL_FAIL;
    }

    pst_rx_ctrl      = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::pst_rx_ctrl is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_framebody_len = MAC_GET_RX_CB_PAYLOAD_LEN(&(pst_rx_ctrl->st_rx_info));
    pst_neighbor_req = (mac_action_neighbor_req_stru *)mac_netbuf_get_payload(pst_netbuf);

    /*************************************************************************/
    /*                    Neighbor Report Request Frame - Frame Body         */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Optional subs                       |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          | var                                  */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;
    /*保存req*/
    pst_rrm_info->uc_action_code  = pst_neighbor_req->uc_action_code;
    pst_rrm_info->uc_dialog_token = pst_neighbor_req->uc_dialog_token;
    pst_rrm_info->us_req_user_id  = pst_hmac_user->st_user_base_info.us_assoc_id;

    /* 获取SSID, Optional Subelement ID */
    OAL_MEMZERO(&(pst_rrm_info->st_neighbor_req_info), OAL_SIZEOF(mac_neighbor_req_info_stru));
    if(us_framebody_len > MAC_NEIGHBOR_REPORT_ACTION_REQ_FIX_LEN)
    {
        puc_ssid_search_addr = pst_neighbor_req->auc_subelm;
        puc_ssid_sub_element = mac_find_ie_etc(MAC_EID_SSID, puc_ssid_search_addr, us_framebody_len - MAC_NEIGHBOR_REPORT_ACTION_REQ_FIX_LEN);
        if (OAL_PTR_NULL != puc_ssid_sub_element)
        {
            pst_rrm_info->st_neighbor_req_info.uc_ssid_len = *(puc_ssid_sub_element + 1);
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::ssid_len[%d]}",
                pst_rrm_info->st_neighbor_req_info.uc_ssid_len);
            //OAL_IO_PRINT("hmac_rrm_proc_neighbor_request::ssid_len[%d]\n", pst_rrm_info->st_neighbor_req_info.uc_ssid_len);

            if (0 != pst_rrm_info->st_neighbor_req_info.uc_ssid_len)
            {
                pst_rrm_info->st_neighbor_req_info.puc_ssid = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, pst_rrm_info->st_neighbor_req_info.uc_ssid_len, OAL_TRUE);
                if (OAL_PTR_NULL == pst_rrm_info->st_neighbor_req_info.puc_ssid)
                {
                    pst_rrm_info->st_neighbor_req_info.uc_ssid_len = 0;
                    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::memalloc ssid fail}");
                }
                else
                {
                    oal_memcopy(pst_rrm_info->st_neighbor_req_info.puc_ssid, (puc_ssid_sub_element + MAC_IE_HDR_LEN), pst_rrm_info->st_neighbor_req_info.uc_ssid_len);

                    /*判断ssid信息是否与本AP一致*/
                    if(WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode &&
                        !OAL_MEMCMP(pst_rrm_info->st_neighbor_req_info.puc_ssid,
                        mac_mib_get_DesiredSSID(&(pst_hmac_vap->st_vap_base_info)), pst_rrm_info->st_neighbor_req_info.uc_ssid_len))
                    {
                        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_request::sta ask ap report local info.}");
                        OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info->st_neighbor_req_info.puc_ssid, OAL_TRUE);
                        pst_hmac_vap->pst_rrm_info->st_neighbor_req_info.puc_ssid = OAL_PTR_NULL;
                        /*一致则上报本vap信息*/
                        hmac_rrm_encap_local_bssid_rpt(pst_hmac_vap);
                        return OAL_SUCC;
                    }
                }
            }
        }
    }

    /*发起扫描*/
    hmac_rrm_neighbor_scan_do(pst_hmac_vap, &(pst_rrm_info->st_neighbor_req_info), OAL_FALSE);

    return OAL_SUCC;
}


oal_uint32 hmac_rrm_proc_neighbor_report(hmac_vap_stru* pst_hmac_vap,  hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf)
{
    mac_action_rm_rpt_stru          *pst_rm_rpt;
    mac_neighbor_rpt_ie_stru        *pst_neighbor_rpt_ie;
    dmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_int16                       s_framebody_len;
    mac_user_rrm_info_stru          *pst_rrm_info;

    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == pst_hmac_user || OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG3(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_report::input is NULL, pst_hmac_vap[%p],pst_hmac_user[%p],pst_netbuf[%p]!}",
            pst_hmac_vap, pst_hmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_hmac_user->pst_user_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_report::pst_rrm_info is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rx_ctrl      = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (OAL_PTR_NULL == pst_rx_ctrl)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_report::pst_rx_ctrl is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    s_framebody_len = (oal_int16)MAC_GET_RX_CB_PAYLOAD_LEN(&(pst_rx_ctrl->st_rx_info));
    pst_rm_rpt = (mac_action_rm_rpt_stru *)mac_netbuf_get_payload(pst_netbuf);

    /*************************************************************************/
    /*                    Neighbor Report Response Frame - Frame Body        */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Neigbor Report Elements             |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          |  var                                 */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /*check dialog token is same*/
    if ( pst_rm_rpt->uc_dialog_token != pst_rrm_info->uc_dialog_token
        || MAC_RRM_STATE_WAIT_RSP != pst_hmac_user->pst_user_rrm_info->en_meas_status)
    {
        OAM_WARNING_LOG3(0, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_report::uc_dialog_token diff: request[%d], report[%d]},en_meas_status[%d].",
            pst_rrm_info->uc_dialog_token, pst_rm_rpt->uc_dialog_token,
            pst_hmac_user->pst_user_rrm_info->en_meas_status);
        return OAL_FAIL;
    }

    if(s_framebody_len < WLAN_MEM_NETBUF_SIZE2 - MAC_80211_FRAME_LEN - MAC_NEIGHBOR_RPT_FIX_LEN)
    {
        /* 取消定时器 */
        if (OAL_TRUE == pst_hmac_user->pst_user_rrm_info->st_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_user->pst_user_rrm_info->st_timer));
            OAM_WARNING_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_proc_neighbor_report:: cancel Timer.}");
        }
    }
    else
    {
        OAM_WARNING_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_proc_rm_report:: wait for another report, s_framebody_len=%d.}",
            (oal_uint16)s_framebody_len);
    }

    pst_neighbor_rpt_ie = (mac_neighbor_rpt_ie_stru*)pst_rm_rpt->auc_rpt_ies;

    s_framebody_len -= MAC_ACTION_RPT_FIX_LEN;

    do
    {
        hmac_rrm_get_neighbor_info_from_rpt(pst_hmac_user, pst_neighbor_rpt_ie);

        s_framebody_len -= pst_neighbor_rpt_ie->uc_len + MAC_IE_HDR_LEN;

        /* 下一个Measurement Report的位置 */
        pst_neighbor_rpt_ie = (mac_neighbor_rpt_ie_stru *)((oal_uint8 *)pst_neighbor_rpt_ie + pst_neighbor_rpt_ie->uc_len + 2);

    }while(s_framebody_len>0);

    if ( OAL_TRUE == pst_hmac_user->pst_user_rrm_info->st_timer.en_is_enabled)
    {
        return OAL_SUCC;
    }

    /*notify other module*/
    pst_hmac_user->pst_user_rrm_info->en_meas_status = MAC_RRM_STATE_GET_RSP;
    hmac_rrm_rpt_notify(pst_hmac_user);

    /*释放链表和恢复状态*/
    hmac_rrm_free_rpt_list(pst_hmac_user, pst_rrm_info->en_reqtype);

    return OAL_SUCC;
}


oal_uint32  hmac_config_send_meas_req(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,
        mac_rrm_req_cfg_stru *pst_req_cfg)
{
    oal_uint32                              ul_ret                      = OAL_SUCC;
    oal_uint16                              us_radio_meas_req_frm_len   = 0;
    mac_tx_ctl_stru                         *pst_tx_ctl;
    oal_netbuf_stru                         *pst_mgmt_buf;
    mac_meas_req_ie_stru                    *pst_meas_req_ie            = OAL_PTR_NULL;
    oal_uint8                               *puc_buffer;
    mac_action_rm_req_stru                  *pst_rm_req;
    hmac_user_stru                          *pst_hmac_user;
    mac_action_neighbor_req_stru            *pst_neighbor_req;
    oal_uint8                               *puc_req_mode;
    mac_bcn_req_stru                        *pst_bcn_req;
    oal_uint8                               *puc_reporting_detail;
    oal_bool_enum_uint8                     en_timer_flag = OAL_TRUE;

    /*获取USER结构体*/
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(pst_mac_user->us_assoc_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(pst_mac_user->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*检测用户是否支持11K*/
    if ( OAL_FALSE == pst_hmac_user->st_user_base_info.st_cap_info.bit_11k_enable)
    {
        OAM_WARNING_LOG0(pst_mac_user->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::user not support 11k.}");
        return OAL_SUCC;
    }

    if (OAL_PTR_NULL == pst_hmac_user->pst_user_rrm_info)
    {
        OAM_ERROR_LOG0(pst_mac_user->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::pst_user_rrm_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*清除rpt mode记录*/
    OAL_MEMZERO(&(pst_hmac_user->pst_user_rrm_info->st_rptmode), OAL_SIZEOF(mac_meas_rpt_mode_stru));

    /*检查用户11k具体能力*/
    ul_ret = hmac_rrm_check_user_cap(&(pst_hmac_user->st_user_base_info.st_rrm_enabled_cap), pst_req_cfg);
    if ( OAL_SUCC != ul_ret )
    {
        return ul_ret;
    }

    /*记录测试模式*/
    pst_hmac_user->pst_user_rrm_info->en_reqtype = pst_req_cfg->en_reqtype;
    pst_hmac_user->pst_user_rrm_info->en_rpt_notify_id = pst_req_cfg->en_rpt_notify_id;

    /*检查该用户是否正在进行测量*/
    if ( MAC_RRM_STATE_INIT != pst_hmac_user->pst_user_rrm_info->en_meas_status )
    {
       OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::en_meas_status[%d] not allow new req.}",
                pst_hmac_user->pst_user_rrm_info->en_meas_status);

       /*通知模块*/
       hmac_rrm_rpt_notify(pst_hmac_user);
       return OAL_SUCC;
    }

    /*申请管理帧BUF*/
    pst_mgmt_buf = (oal_netbuf_stru *)OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if(OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::pst_mgmt_buf null.}");
        return OAL_PTR_NULL;
    }

    OAL_MEMZERO(oal_netbuf_cb(pst_mgmt_buf), OAL_NETBUF_CB_SIZE());

    puc_buffer = (oal_uint8 *)oal_netbuf_header(pst_mgmt_buf);

    /*************************************************************************/
    /*                        Management Frame Format                        */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BSSID|Sequence Control|Frame Body|FCS|  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |6    |2               |0 - 2312  |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    /* All the fields of the Frame Control Field are set to zero. Only the   */
    /* Type/Subtype field is set.                                            */
    mac_hdr_set_frame_control(puc_buffer, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* duration */
    puc_buffer[2] = 0;
    puc_buffer[3] = 0;

    /* DA is address of STA requesting association */
    oal_set_mac_addr(puc_buffer + WLAN_HDR_ADDR1_OFFSET, pst_mac_user->auc_user_mac_addr);

    /*SA*/
    oal_set_mac_addr(puc_buffer + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(pst_mac_vap));
    oal_set_mac_addr(puc_buffer + WLAN_HDR_ADDR3_OFFSET, pst_mac_vap->auc_bssid);

    /* seq control */
    puc_buffer[22] = 0;
    puc_buffer[23] = 0;

    /*************************************************************************/
    /*                    Radio Measurement Request Frame - Frame Body       */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Number of Repetitions|Meas Req Eles |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          | 2                    |var            */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    puc_buffer                  += MAC_80211_FRAME_LEN;

    /*dialog_token记录*/
    guc_dialog_token++;
    guc_meas_token++;
    pst_hmac_user->pst_user_rrm_info->uc_dialog_token   = guc_dialog_token;
    pst_hmac_user->pst_user_rrm_info->uc_meas_token     = guc_meas_token;

    /*Radio Measurement Request*/
    if(pst_req_cfg->en_reqtype <= MAC_RRM_TYPE_BCN)
    {
        if ( OAL_PTR_NULL == pst_req_cfg->p_arg )
        {
            oal_netbuf_free(pst_mgmt_buf);
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::radio meas req element null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_rm_req                  = (mac_action_rm_req_stru *)puc_buffer;
        pst_rm_req->uc_category     = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;
        pst_rm_req->uc_action_code  = MAC_RM_ACTION_RADIO_MEASUREMENT_REQUEST;
        pst_rm_req->uc_dialog_token = guc_dialog_token;
        pst_rm_req->us_num_rpt      = pst_req_cfg->us_rpt_num;
        pst_meas_req_ie             = (mac_meas_req_ie_stru *)pst_rm_req->auc_req_ies;

        /*************************************************************************/
        /*                    Measurement Request IE                             */
        /* --------------------------------------------------------------------- */
        /* |Element ID |Length |Meas Token| Meas Req Mode|Meas Type  | Meas Req |*/
        /* --------------------------------------------------------------------- */
        /* |1          |1      | 1        | 1            |1          |var       |*/
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/
        pst_meas_req_ie->uc_eid     = MAC_EID_MEASREQ;
        pst_meas_req_ie->uc_token   = guc_meas_token;
        puc_req_mode = (oal_uint8*)(&(pst_meas_req_ie->st_reqmode));
        *puc_req_mode = pst_req_cfg->uc_req_mode;

        if ( MAC_RRM_TYPE_BCN == pst_req_cfg->en_reqtype )
        {
            pst_meas_req_ie->uc_len     = 16;
            pst_meas_req_ie->uc_reqtype = RM_RADIO_MEAS_BCN;
            oal_memcopy(pst_meas_req_ie->auc_meas_req, (mac_bcn_req_stru*)(pst_req_cfg->p_arg), OAL_SIZEOF(mac_bcn_req_stru));

            /*optional subelement*/
            pst_bcn_req = (mac_bcn_req_stru*)pst_meas_req_ie->auc_meas_req;
            puc_reporting_detail = pst_bcn_req->auc_subelm;
            /*2,Reporting Detail*/
            *puc_reporting_detail                       = MAC_RRM_BCN_EID_REPORTING_DATAIL;
            *(puc_reporting_detail + 1)                 = MAC_RRM_BCN_REPORTING_DETAIL_LEN;
            *(puc_reporting_detail + 2)                 = MAC_BCN_REPORTING_DETAIL_FIXED_FILELD_AND_ANY_ELEM;
            pst_meas_req_ie->uc_len += MAC_IE_HDR_LEN + MAC_RRM_BCN_REPORTING_DETAIL_LEN;
        }
        else if ( MAC_RRM_TYPE_CHANNEL_LOAD == pst_req_cfg->en_reqtype )
        {
            pst_meas_req_ie->uc_len     = 9;
            pst_meas_req_ie->uc_reqtype = RM_RADIO_MEAS_CHANNEL_LOAD;
            oal_memcopy(pst_meas_req_ie->auc_meas_req, (mac_chn_load_req_stru*)(pst_req_cfg->p_arg), OAL_SIZEOF(mac_chn_load_req_stru));
        }

        us_radio_meas_req_frm_len = MAC_80211_FRAME_LEN + 5 + MAC_IE_HDR_LEN + pst_meas_req_ie->uc_len;

        /* Radio Measurement Req中不允许对端发送report，则不起定时器 */
        if((1 == pst_meas_req_ie->st_reqmode.bit_enable) && (0 == pst_meas_req_ie->st_reqmode.bit_rpt))
        {
            OAM_WARNING_LOG0(pst_mac_user->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::not expect report.}");
            en_timer_flag = OAL_FALSE;
        }
    }
    /*Neighbor Report Request*/
    else if(MAC_RRM_TYPE_NEIGHBOR_RPT == pst_req_cfg->en_reqtype)
    {
        /*************************************************************************/
        /*                    Neighbor Report Request Frame - Frame Body         */
        /* --------------------------------------------------------------------- */
        /* |Category |Action |Dialog Token| Optional subs                       |*/
        /* --------------------------------------------------------------------- */
        /* |1        |1      | 1          | var                                  */
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/
        pst_neighbor_req                  = (mac_action_neighbor_req_stru *)puc_buffer;
        pst_neighbor_req->uc_category     = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;
        pst_neighbor_req->uc_action_code  = MAC_RM_ACTION_NEIGHBOR_REPORT_REQUEST;
        pst_neighbor_req->uc_dialog_token = guc_dialog_token;
        us_radio_meas_req_frm_len         = MAC_80211_FRAME_LEN + 3;
    }
    else
    {
        oal_netbuf_free(pst_mgmt_buf);
        return OAL_FAIL;
    }

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)  = us_radio_meas_req_frm_len;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)  = pst_mac_user->us_assoc_id;                        /* ????????user??? */

    oal_netbuf_put(pst_mgmt_buf, us_radio_meas_req_frm_len);

    ul_ret = hmac_tx_mgmt_send_event_etc(pst_mac_vap, pst_mgmt_buf, us_radio_meas_req_frm_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_RRM, "{hmac_config_send_meas_req::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* Radio Measurement Req中不允许对端发送report，则不起定时器 */
    if ( OAL_FALSE == en_timer_flag )
    {
        return OAL_SUCC;
    }

    /* 更改状态 */
    pst_hmac_user->pst_user_rrm_info->en_meas_status = MAC_RRM_STATE_WAIT_RSP;

    /* 启动measurement超时定时器, 如果超时没回rsp则启动超时处理 */
    pst_hmac_user->pst_user_rrm_info->st_timer.ul_timeout = 5000; //5s

    FRW_TIMER_CREATE_TIMER(&(pst_hmac_user->pst_user_rrm_info->st_timer),
                           hmac_wait_meas_rsp_timeout,
                           pst_hmac_user->pst_user_rrm_info->st_timer.ul_timeout,
                           (oal_void *)((oal_uint)(pst_mac_user->us_assoc_id)),
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_mac_vap->ul_core_id);

    return ul_ret;
}

#define HMAC_11K_INTERNAL_FUNC

OAL_STATIC oal_uint32 hmac_rrm_rpt_notify(hmac_user_stru *pst_user)
{
    mac_rrm_state_enum                   en_meas_status;            /* 测量状态 */
    mac_rrm_rpt_notify_enum              en_rpt_notify_id;
    mac_rrm_type_enum                    en_reqtype;                /* 测量类型 */

    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_rpt_notify caller!}");

    en_rpt_notify_id = pst_user->pst_user_rrm_info->en_rpt_notify_id;
    en_reqtype       = pst_user->pst_user_rrm_info->en_reqtype;
    en_meas_status   = pst_user->pst_user_rrm_info->en_meas_status;

    /*notify other module*/
    if (OAL_PTR_NULL != gst_rrm_rpt_hook.pa_rrm_rpt_notify_func[en_rpt_notify_id][en_reqtype])
    {
        gst_rrm_rpt_hook.pa_rrm_rpt_notify_func[en_rpt_notify_id][en_reqtype](pst_user, en_meas_status);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_rrm_check_user_cap(mac_rrm_enabled_cap_ie_stru *pst_rrm_enabled_cap, mac_rrm_req_cfg_stru *pst_req_cfg)
{
    oal_uint32                  ul_ret  = OAL_SUCC;
    mac_bcn_req_stru            *pst_bcn_req;

    /*Neighbor report Request*/
    if(MAC_RRM_TYPE_NEIGHBOR_RPT == pst_req_cfg->en_reqtype
        && OAL_FALSE == pst_rrm_enabled_cap->bit_neighbor_rpt_cap)
    {
        OAM_WARNING_LOG2(0, OAM_SF_RRM, "{hmac_rrm_check_user_cap::en_reqtype[%d],bit_neighbor_rpt_cap[%d] not support.}",
            pst_req_cfg->en_reqtype, pst_rrm_enabled_cap->bit_neighbor_rpt_cap);
        return OAL_FAIL;
    }

    /*Channel Load Request*/
    if(MAC_RRM_TYPE_CHANNEL_LOAD == pst_req_cfg->en_reqtype
        && OAL_FALSE == pst_rrm_enabled_cap->bit_chn_load_cap)
    {
        OAM_WARNING_LOG2(0, OAM_SF_RRM, "{hmac_rrm_check_user_cap::en_reqtype[%d],bit_chn_load_cap[%d] not support.}",
            pst_req_cfg->en_reqtype, pst_rrm_enabled_cap->bit_chn_load_cap);
        return OAL_FAIL;
    }

    /*Beacon Report*/
    if(MAC_RRM_TYPE_BCN == pst_req_cfg->en_reqtype)
    {
        if ( OAL_PTR_NULL == pst_req_cfg->p_arg )
        {
            OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_check_user_cap::bcn req p_arg null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        pst_bcn_req = (mac_bcn_req_stru*)(pst_req_cfg->p_arg);

        if ( RM_BCN_REQ_MEAS_MODE_PASSIVE == pst_bcn_req->en_mode
            && OAL_FALSE == pst_rrm_enabled_cap->bit_bcn_passive_cap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_check_user_cap::bcn::bit_bcn_passive_cap[%d] not support.}",
                                pst_rrm_enabled_cap->bit_bcn_passive_cap);
            return OAL_FAIL;
        }

        if ( RM_BCN_REQ_MEAS_MODE_ACTIVE == pst_bcn_req->en_mode
            && OAL_FALSE == pst_rrm_enabled_cap->bit_bcn_active_cap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_check_user_cap::bcn::bit_bcn_active_cap[%d] not support.}",
                                pst_rrm_enabled_cap->bit_bcn_active_cap);
            return OAL_FAIL;
        }

        if ( RM_BCN_REQ_MEAS_MODE_TABLE == pst_bcn_req->en_mode
            && OAL_FALSE == pst_rrm_enabled_cap->bit_bcn_table_cap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_check_user_cap::bcn::bit_bcn_table_cap[%d] not support.}",
                                pst_rrm_enabled_cap->bit_bcn_table_cap);
            return OAL_FAIL;
        }
    }

    return ul_ret;
}


oal_uint32 hmac_rrm_free_rpt_list(hmac_user_stru *pst_hmac_user, mac_rrm_type_enum en_reqtype)
{
    mac_user_rrm_info_stru          *pst_rrm_info;
    mac_meas_rpt_bcn_stru           *pst_meas_rpt_bcn;
    mac_meas_rpt_bcn_item_stru      *pst_meas_rpt_bcn_item;
    oal_dlist_head_stru             *pst_meas_rpt_node      = OAL_PTR_NULL;
    mac_meas_rpt_chn_load_stru      *pst_meas_rpt_chn_load;
    mac_meas_rpt_neighbor_stru      *pst_meas_rpt_neighbor;

    pst_rrm_info        = pst_hmac_user->pst_user_rrm_info;

    /*11K状态恢复*/
    pst_hmac_user->pst_user_rrm_info->en_meas_status = MAC_RRM_STATE_INIT;

    if (oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)))
    {
        return OAL_SUCC;
    }

    /* 遍历扫描到的bss信息 */
    /*Beacon Report*/
    if ( MAC_RRM_TYPE_BCN == en_reqtype )
    {
        do
        {
            /*获取节点*/
            pst_meas_rpt_node = pst_rrm_info->st_meas_rpt_list.pst_next;
            pst_meas_rpt_bcn = OAL_DLIST_GET_ENTRY(pst_meas_rpt_node, mac_meas_rpt_bcn_stru, st_dlist_head);
            if (OAL_PTR_NULL == pst_meas_rpt_bcn )
            {
                continue;
            }

            if (OAL_PTR_NULL != pst_meas_rpt_bcn->pst_meas_rpt_bcn_item)
            {
                pst_meas_rpt_bcn_item = pst_meas_rpt_bcn->pst_meas_rpt_bcn_item;
                OAL_MEM_FREE(pst_meas_rpt_bcn_item, OAL_TRUE);
                pst_meas_rpt_bcn_item = OAL_PTR_NULL;
            }

            if (OAL_PTR_NULL != pst_meas_rpt_bcn->puc_rpt_detail_data)
            {
                OAL_MEM_FREE(pst_meas_rpt_bcn->puc_rpt_detail_data, OAL_TRUE);
                pst_meas_rpt_bcn->puc_rpt_detail_data = OAL_PTR_NULL;
            }

            /*释放节点*/
            oal_dlist_delete_entry(pst_meas_rpt_node);
            OAL_MEM_FREE(pst_meas_rpt_bcn, OAL_TRUE);
            pst_meas_rpt_bcn = OAL_PTR_NULL;

        } while (!oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)));
    }

    /*CHANNEL LOAD Report*/
    if ( MAC_RRM_TYPE_CHANNEL_LOAD == en_reqtype )
    {
        do
        {
            /*获取节点*/
            pst_meas_rpt_node = pst_rrm_info->st_meas_rpt_list.pst_next;
            pst_meas_rpt_chn_load = OAL_DLIST_GET_ENTRY(pst_meas_rpt_node, mac_meas_rpt_chn_load_stru, st_dlist_head_chn_load);
            if (OAL_PTR_NULL == pst_meas_rpt_chn_load )
            {
                continue;
            }

            if (OAL_PTR_NULL != pst_meas_rpt_chn_load->pst_meas_rpt_chn_load_item)
            {
                OAL_MEM_FREE(pst_meas_rpt_chn_load->pst_meas_rpt_chn_load_item, OAL_TRUE);
                pst_meas_rpt_chn_load->pst_meas_rpt_chn_load_item = OAL_PTR_NULL;
            }

            /*释放节点*/
            oal_dlist_delete_entry(pst_meas_rpt_node);
            OAL_MEM_FREE(pst_meas_rpt_chn_load, OAL_TRUE);
            pst_meas_rpt_chn_load = OAL_PTR_NULL;

        } while (!oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)));
    }

    /*Neighbor Report*/
    if ( MAC_RRM_TYPE_NEIGHBOR_RPT == en_reqtype )
    {
        do
        {
            /*获取节点*/
            pst_meas_rpt_node = pst_rrm_info->st_meas_rpt_list.pst_next;
            pst_meas_rpt_neighbor = OAL_DLIST_GET_ENTRY(pst_meas_rpt_node, mac_meas_rpt_neighbor_stru, st_dlist_head_neighbor);
            if (OAL_PTR_NULL == pst_meas_rpt_neighbor )
            {
                continue;
            }

            if (OAL_PTR_NULL != pst_meas_rpt_neighbor->pst_meas_rpt_neighbor_item)
            {
                OAL_MEM_FREE(pst_meas_rpt_neighbor->pst_meas_rpt_neighbor_item, OAL_TRUE);
                pst_meas_rpt_neighbor->pst_meas_rpt_neighbor_item = OAL_PTR_NULL;
            }

            /*释放节点*/
            oal_dlist_delete_entry(pst_meas_rpt_node);
            OAL_MEM_FREE(pst_meas_rpt_neighbor, OAL_TRUE);
            pst_meas_rpt_neighbor = OAL_PTR_NULL;

        } while (!oal_dlist_is_empty(&(pst_rrm_info->st_meas_rpt_list)));
    }

    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_free_rpt_list: free meas_rpt_list node and set meas_status as MAC_RRM_STATE_INIT.}");

    return OAL_SUCC;

}



OAL_STATIC oal_uint32 hmac_wait_meas_rsp_timeout(oal_void *p_arg)
{
    hmac_user_stru  *pst_hmac_user;
    oal_uint16       us_assoc_id;

    us_assoc_id = (oal_uint16)(oal_uint)(p_arg);

    /*获取USER结构体*/
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_assoc_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_wait_meas_rsp_timeout::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_hmac_user->pst_user_rrm_info)
    {
        OAM_WARNING_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_wait_meas_rsp_timeout::pst_hmac_user->pst_user_rrm_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_wait_meas_rsp_timeout::en_reqtype=%d.}",
        pst_hmac_user->pst_user_rrm_info->en_reqtype);

    /*更改状态*/
    pst_hmac_user->pst_user_rrm_info->en_meas_status = MAC_RRM_STATE_RSP_TIMEOUT;

    /*通知模块*/
    hmac_rrm_rpt_notify(pst_hmac_user);

    /*释放链表和恢复状态*/
    hmac_rrm_free_rpt_list(pst_hmac_user, pst_hmac_user->pst_user_rrm_info->en_reqtype);

    return OAL_SUCC;
}



OAL_STATIC oal_uint32 hmac_vap_meas_status_timeout(oal_void *p_arg)
{
    mac_vap_rrm_info_stru *pst_rrm_info;
    pst_rrm_info = (mac_vap_rrm_info_stru*)p_arg;

    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_vap_meas_status_timeout::pst_rrm_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_vap_meas_status_timeout::en_is_measuring set OAL_FALSE.}");

    /*更改状态*/
    pst_rrm_info->en_is_measuring = OAL_FALSE;

    return OAL_SUCC;
}


oal_uint32  hmac_rrm_get_meas_start_time(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru       *pst_hmac_vap;
    /* 入参检查 */
    if((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_RRM, "{hmac_rrm_get_meas_start_time:: null param vap:0x%x puc_param:0x%x.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_RRM, "{hmac_rrm_get_meas_start_time::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ( pst_hmac_vap->bit_11k_enable && OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info)
    {
        /* Actual Meas Start Time */
        oal_memcopy(pst_hmac_vap->pst_rrm_info->aul_act_meas_start_time, puc_param, OAL_SIZEOF(oal_uint32)*2);
    }

   return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_rrm_fill_basic_rm_rpt_action(hmac_vap_stru *pst_hmac_vap)
{
    oal_uint8                       *puc_mac_header;
    oal_uint8                       *puc_payload;
    mac_vap_rrm_info_stru           *pst_rrm_info;
    mac_user_stru                   *pst_mac_user;

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_fill_basic_rm_rpt_action::pst_hmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;
    if (OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_fill_basic_rm_rpt_action::pst_rrm_info NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ( OAL_PTR_NULL != pst_rrm_info->pst_rm_rpt_mgmt_buf )
    {
        oal_netbuf_free(pst_rrm_info->pst_rm_rpt_mgmt_buf);
        pst_hmac_vap->pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_PTR_NULL;
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_fill_basic_rm_rpt_action::pst_mgmt_buf not be freed in history.}");
        //return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请管理帧内存 */
    pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_rrm_info->pst_rm_rpt_mgmt_buf)
    {
       OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_fill_basic_rm_rpt_action::pst_mgmt_buf null.}");
       return OAL_ERR_CODE_PTR_NULL;
    }

    oal_set_netbuf_prev(pst_rrm_info->pst_rm_rpt_mgmt_buf, OAL_PTR_NULL);
    oal_set_netbuf_next(pst_rrm_info->pst_rm_rpt_mgmt_buf,OAL_PTR_NULL);

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
    puc_mac_header = oal_netbuf_header(pst_rrm_info->pst_rm_rpt_mgmt_buf);

    /* 帧控制字段全为0，除了type和subtype */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* 设置分片序号为0 */
    mac_hdr_set_fragment_number(puc_mac_header, 0);

    pst_mac_user = mac_res_get_mac_user_etc(pst_rrm_info->us_req_user_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "{hmac_rrm_fill_basic_rm_rpt_action::pst_mac_user[%d] null.", pst_rrm_info->us_req_user_id);
        oal_netbuf_free(pst_rrm_info->pst_rm_rpt_mgmt_buf);
        pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_PTR_NULL;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置地址1，用户地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_mac_user->auc_user_mac_addr);

    /* 设置地址2为自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info));

    /* 地址3，为VAP自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_hmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************/
    /*                    Radio Measurement Report Frame - Frame Body        */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Measurement Report Elements         |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          |  var                                 */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    puc_payload = mac_netbuf_get_payload(pst_rrm_info->pst_rm_rpt_mgmt_buf);

    pst_rrm_info->pst_rm_rpt_action                  = (mac_action_rm_rpt_stru *)puc_payload;
    pst_rrm_info->pst_rm_rpt_action->uc_category     = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;
    pst_rrm_info->pst_rm_rpt_action->uc_action_code  = pst_rrm_info->uc_action_code + 1;//radio or neighbor reponse
    pst_rrm_info->pst_rm_rpt_action->uc_dialog_token = pst_rrm_info->uc_dialog_token;

    pst_rrm_info->us_rm_rpt_action_len = MAC_ACTION_RPT_FIX_LEN + MAC_80211_FRAME_LEN;
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_rrm_send_rm_rpt_action(hmac_vap_stru* pst_hmac_vap)
{
    mac_tx_ctl_stru                 *pst_tx_ctl;
    oal_netbuf_stru                 *pst_mgmt_buf;
    oal_uint32                      ul_ret = OAL_SUCC;
    mac_user_stru                   *pst_mac_user;

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_send_rm_rpt_action::pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_hmac_vap->pst_rrm_info)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_send_rm_rpt_action::pst_rrm_info NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap->pst_rrm_info->en_is_measuring = OAL_FALSE;
    /* 取消定时器 */
    if (OAL_TRUE == pst_hmac_vap->pst_rrm_info->st_meas_status_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->pst_rrm_info->st_meas_status_timer));
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_send_rm_rpt_action:: cancel st_meas_status_timer.}");
    }

    pst_mgmt_buf = pst_hmac_vap->pst_rrm_info->pst_rm_rpt_mgmt_buf;
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_send_rm_rpt_action::pst_mgmt_buf NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_user = mac_res_get_mac_user_etc(pst_hmac_vap->pst_rrm_info->us_req_user_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_send_rm_rpt_action::pst_mac_user[%d] null.", pst_hmac_vap->pst_rrm_info->us_req_user_id);
        oal_netbuf_free(pst_hmac_vap->pst_rrm_info->pst_rm_rpt_mgmt_buf);
        pst_hmac_vap->pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_PTR_NULL;
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写netbuf的cb字段，供发送管理帧和发送完成接口使用 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);
    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = pst_mac_user->us_assoc_id;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl)   = WLAN_WME_AC_MGMT;

    /* 调用发送管理帧接口 */
    ul_ret = hmac_tx_mgmt_send_event_etc(&(pst_hmac_vap->st_vap_base_info), pst_mgmt_buf, pst_hmac_vap->pst_rrm_info->us_rm_rpt_action_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        pst_hmac_vap->pst_rrm_info->pst_rm_rpt_mgmt_buf = OAL_PTR_NULL;
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_send_rm_rpt_action::tx link meas rpt action failed.}");
        return ul_ret;
    }

    /*释放空间*/
    if(OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info->st_neighbor_req_info.puc_ssid)
    {
        OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info->st_neighbor_req_info.puc_ssid, OAL_TRUE);
    }
    if(OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_ssid)
    {
        OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_ssid, OAL_TRUE);
    }
    if(OAL_PTR_NULL != pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid)
    {
        OAL_MEM_FREE(pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid, OAL_TRUE);
    }
    OAL_MEMZERO(pst_hmac_vap->pst_rrm_info, OAL_SIZEOF(mac_vap_rrm_info_stru));

    return ul_ret;
}


OAL_STATIC oal_void hmac_rrm_encap_meas_rpt_reject(hmac_vap_stru *pst_hmac_vap, mac_meas_rpt_mode_stru *pst_rptmode)
{
    mac_vap_rrm_info_stru               *pst_rrm_info       = OAL_PTR_NULL;
    mac_meas_rpt_ie_stru                *pst_meas_rpt_ie;

    if((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_rptmode))
    {
        OAM_ERROR_LOG2(0, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_reject:: null param, vap:0x%x rptmode:0x%x!}", pst_hmac_vap, pst_rptmode);
        return ;
    }
    if(OAL_PTR_NULL == pst_hmac_vap->pst_rrm_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_reject:: pst_hmac_vap->pst_rrm_info null!}");
        return ;
    }
    pst_rrm_info       = pst_hmac_vap->pst_rrm_info;
    pst_rrm_info->en_is_measuring = OAL_FALSE;
    /* 取消定时器 */
    if (OAL_TRUE == pst_hmac_vap->pst_rrm_info->st_meas_status_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->pst_rrm_info->st_meas_status_timer));
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_reject:: cancel st_meas_status_timer.}");
    }

    if ( OAL_SUCC != hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap))
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_reject:: hmac_rrm_fill_basic_rm_rpt_action fail!}");
        return;
    }

    pst_meas_rpt_ie   = (mac_meas_rpt_ie_stru *)pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;

    /*************************************************************************/
    /*                   Measurement Report IE - Frame Body         */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Rpt Mode | Meas Type | Meas Rpt|*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        |  1            | 1         | var      */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    pst_meas_rpt_ie->uc_eid       = MAC_EID_MEASREP;
    pst_meas_rpt_ie->uc_token     = pst_rrm_info->pst_meas_req_ie->uc_token;
    pst_meas_rpt_ie->uc_rpttype   = pst_rrm_info->pst_meas_req_ie->uc_reqtype;
    oal_memcopy(&(pst_meas_rpt_ie->st_rptmode), pst_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
    pst_meas_rpt_ie->uc_len       = MAC_MEASUREMENT_RPT_FIX_LEN - MAC_IE_HDR_LEN;

    pst_rrm_info->us_rm_rpt_action_len          += MAC_MEASUREMENT_RPT_FIX_LEN;

    hmac_rrm_send_rm_rpt_action(pst_hmac_vap);
}


OAL_STATIC oal_uint32 hmac_rrm_encap_meas_rpt_refuse_new_req(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
        mac_action_rm_req_stru *pst_rm_req)
{
    oal_netbuf_stru                     *pst_mgmt_buf;
    mac_meas_rpt_ie_stru                *pst_meas_rpt_ie;
    oal_uint8                           *puc_mac_header;
    oal_uint8                           *puc_payload;
    mac_user_stru                       *pst_mac_user;
    mac_action_rm_rpt_stru              *pst_rm_rpt_action;
    oal_uint16                          us_rm_rpt_action_len;
    mac_tx_ctl_stru                     *pst_tx_ctl;
    oal_uint32                          ul_ret = OAL_SUCC;
    mac_meas_req_ie_stru                *pst_meas_req_ie;

    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == pst_hmac_user || OAL_PTR_NULL == pst_rm_req )
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_refuse_new_req::input is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_meas_req_ie   = (mac_meas_req_ie_stru *)&(pst_rm_req->auc_req_ies[0]);

    /* Req中不允许发送对应的report */
    if ((1 == pst_meas_req_ie->st_reqmode.bit_enable) && (0 == pst_meas_req_ie->st_reqmode.bit_rpt))
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_refuse_new_req::rpt now allowed!}");
        return OAL_SUCC;
    }

    /* 申请管理帧内存 */
    pst_mgmt_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_mgmt_buf)
    {
       OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_refuse_new_req::pst_mgmt_buf null.}");
       return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(oal_netbuf_cb(pst_mgmt_buf), OAL_NETBUF_CB_SIZE());

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
    puc_mac_header = oal_netbuf_header(pst_mgmt_buf);

    /* 帧控制字段全为0，除了type和subtype */
    mac_hdr_set_frame_control(puc_mac_header, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* 设置分片序号为0 */
    mac_hdr_set_fragment_number(puc_mac_header, 0);

    pst_mac_user = &(pst_hmac_user->st_user_base_info);

    /* 设置地址1，用户地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR1_OFFSET, pst_mac_user->auc_user_mac_addr);

    /* 设置地址2为自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info));

    /* 地址3，为VAP自己的MAC地址 */
    oal_set_mac_addr(puc_mac_header + WLAN_HDR_ADDR3_OFFSET, pst_hmac_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************/
    /*                    Radio Measurement Report Frame - Frame Body        */
    /* --------------------------------------------------------------------- */
    /* |Category |Action |Dialog Token| Measurement Report Elements         |*/
    /* --------------------------------------------------------------------- */
    /* |1        |1      | 1          |  var                                 */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    puc_payload = mac_netbuf_get_payload(pst_mgmt_buf);

    pst_rm_rpt_action                  = (mac_action_rm_rpt_stru *)puc_payload;
    pst_rm_rpt_action->uc_category     = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;
    pst_rm_rpt_action->uc_action_code  = MAC_RM_ACTION_RADIO_MEASUREMENT_REPORT;
    pst_rm_rpt_action->uc_dialog_token = pst_rm_req->uc_dialog_token;

    us_rm_rpt_action_len = MAC_ACTION_RPT_FIX_LEN + MAC_MEASUREMENT_RPT_FIX_LEN + MAC_80211_FRAME_LEN;

    pst_meas_rpt_ie   = (mac_meas_rpt_ie_stru *)pst_rm_rpt_action->auc_rpt_ies;

    /*************************************************************************/
    /*                   Measurement Report IE - Frame Body         */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Rpt Mode | Meas Type | Meas Rpt|*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        |  1            | 1         | var      */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    pst_meas_rpt_ie->uc_eid       = MAC_EID_MEASREP;
    pst_meas_rpt_ie->uc_token     = pst_meas_req_ie->uc_token;
    pst_meas_rpt_ie->uc_rpttype   = pst_meas_req_ie->uc_reqtype;
    pst_meas_rpt_ie->st_rptmode.bit_refused = 1;
    pst_meas_rpt_ie->uc_len       = MAC_ACTION_RPT_FIX_LEN;

    /* 填写netbuf的cb字段，供发送管理帧和发送完成接口使用 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_mgmt_buf);
    OAL_MEMZERO(pst_tx_ctl, sizeof(mac_tx_ctl_stru));

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = pst_mac_user->us_assoc_id;
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl)   = WLAN_WME_AC_MGMT;

    /* 调用发送管理帧接口 */
    ul_ret = hmac_tx_mgmt_send_event_etc(&(pst_hmac_vap->st_vap_base_info), pst_mgmt_buf, us_rm_rpt_action_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_mgmt_buf);
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_refuse_new_req::tx link meas rpt action failed.}");
        return ul_ret;
    }

    return OAL_SUCC;
}

#define HMAC_11K_INTERNAL_FUNC_CHN_LOAD

OAL_STATIC oal_uint32  hmac_rrm_chn_load_scan_do(hmac_vap_stru *pst_hmac_vap, mac_chn_load_req_stru *pst_chn_load_req)
{
    mac_scan_req_stru                   st_scan_req;
    wlan_channel_band_enum_uint8        en_chan_band;
    oal_uint32                          ul_ret;
    mac_meas_rpt_mode_stru              st_rptmode;

    en_chan_band = mac_get_band_by_channel_num(pst_chn_load_req->uc_channum);

    OAL_MEMZERO(&st_scan_req, OAL_SIZEOF(st_scan_req));

    /*设置扫描参数*/
    st_scan_req.uc_vap_id               = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    st_scan_req.en_scan_type            = WLAN_SCAN_TYPE_ACTIVE;

    st_scan_req.en_bss_type             = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req.en_scan_type            = WLAN_SCAN_TYPE_PASSIVE;
    st_scan_req.uc_probe_delay          = 0;
    st_scan_req.uc_scan_func            = MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS;
    st_scan_req.uc_max_scan_count_per_channel = WLAN_DEFAULT_BG_SCAN_COUNT_PER_CHANNEL;

    st_scan_req.us_scan_time            = pst_chn_load_req->us_duration;
    st_scan_req.en_scan_mode            = WLAN_SCAN_MODE_BACKGROUND_CCA;

    /*设置扫描信道*/
    st_scan_req.uc_channel_nums = 1;
    ul_ret = mac_get_channel_idx_from_num_etc(en_chan_band, pst_chn_load_req->uc_channum, &st_scan_req.ast_channel_list[0].uc_chan_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_chn_load_scan_do::chn num invalid[%d]}", pst_chn_load_req->uc_channum);
        /*无效信道回复refuse bit*/
        OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
        st_rptmode.bit_refused = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
        return ul_ret;
    }

    st_scan_req.ast_channel_list[0].uc_chan_number = pst_chn_load_req->uc_channum;
    st_scan_req.ast_channel_list[0].en_band        = en_chan_band;

    /*回调函数指针*/
    st_scan_req.p_fn_cb                  = hmac_rrm_chn_load_scan_cb;

    /* 直接调用扫描模块扫描请求处理函数 */
    ul_ret = hmac_scan_proc_scan_req_event_etc(pst_hmac_vap, &st_scan_req);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "hmac_rrm_chn_load_scan_do:hmac_scan_proc_scan_req_event_etc failed, ret=%d", ul_ret);
        /*扫描失败回复refuse bit*/
        OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
        st_rptmode.bit_refused = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
    }
    else
    {
        pst_hmac_vap->pst_rrm_info->en_is_measuring = OAL_TRUE;

        pst_hmac_vap->pst_rrm_info->st_meas_status_timer.ul_timeout = MAC_RRM_VAP_MEAS_STAUTS_TIME;

        FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->pst_rrm_info->st_meas_status_timer),
                               hmac_vap_meas_status_timeout,
                               pst_hmac_vap->pst_rrm_info->st_meas_status_timer.ul_timeout,
                               (oal_void *)pst_hmac_vap->pst_rrm_info,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
    }

    return ul_ret;
}


OAL_STATIC oal_void hmac_rrm_chn_load_scan_cb(void *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record;
    hmac_vap_stru                   *pst_hmac_vap     = OAL_PTR_NULL;
    oal_uint8                       uc_chn_load;
    mac_vap_rrm_info_stru           *pst_rrm_info;
    mac_meas_rpt_mode_stru          st_rptmode;
    mac_chn_load_req_stru           *pst_chn_load_req;
    mac_chn_load_rpt_stru           *pst_chn_load_rpt;
    mac_meas_rpt_ie_stru            *pst_meas_rpt_ie;
    oal_uint32                      ul_ret;

    /* 判断入参合法性 */
    if (OAL_PTR_NULL == p_scan_record)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_chn_load_scan_cb: input pointer is null!}");
        return;
    }

    pst_scan_record = (hmac_scan_record_stru *)p_scan_record;

    /*获取hmac VAP*/
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_chn_load_scan_cb: pst_hmac_vap is null!}");
        return;
    }

    OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
    /*判断扫描状态是否成功*/
    if (MAC_SCAN_SUCCESS != pst_scan_record->en_scan_rsp_status )
    {
        st_rptmode.bit_refused = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_chn_load_scan_cb::scan fail, status[%d].",
                pst_scan_record->en_scan_rsp_status);
        return;
    }

    pst_rrm_info     = pst_hmac_vap->pst_rrm_info;
    pst_chn_load_req = (mac_chn_load_req_stru *)&(pst_rrm_info->pst_meas_req_ie->auc_meas_req[0]);

    /*************************************************************************/
    /*                    Channel Load Request                                     */
    /* --------------------------------------------------------------------- */
    /* |Operating Class |Channel Number |Rand Interval| Meas Duration       |*/
    /* --------------------------------------------------------------------- */
    /* |1               |1              | 2           | 2                   |*/
    /* --------------------------------------------------------------------- */
    /*************************************************************************/
    /*申请RPT管理帧内存*/
    ul_ret = hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap);
    if ( OAL_SUCC != ul_ret)
    {
       OAM_ERROR_LOG1(0, OAM_SF_RRM, "{hmac_chn_load_scan_cb: hmac_rrm_fill_basic_rm_rpt_action ret[%d].}", ul_ret);
       return;
    }

    /*************************************************************************/
    /*                    Measurement Report IE                             */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Req Mode|Meas Type  | Meas Req |*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        | 1            |1          |var       |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /*设置rpt IE point*/
    pst_meas_rpt_ie = (mac_meas_rpt_ie_stru *)pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;
    pst_rrm_info->pst_meas_rpt_ie = pst_meas_rpt_ie;

    /*Measurement Report IE*/
    pst_meas_rpt_ie->uc_eid       = MAC_EID_MEASREP;
    pst_meas_rpt_ie->uc_token     = pst_rrm_info->pst_meas_req_ie->uc_token;
    OAL_MEMZERO(&(pst_meas_rpt_ie->st_rptmode), OAL_SIZEOF(mac_meas_rpt_mode_stru));
    pst_meas_rpt_ie->uc_rpttype   = pst_rrm_info->pst_meas_req_ie->uc_reqtype;

    /*************************************************************************/
    /*                   channel load Report - Frame Body                     */
    /* --------------------------------------------------------------------- */
    /* |oper class|chn num|Actual Meas Start Time|Meas Duration|Channel Load|*/
    /* --------------------------------------------------------------------- */
    /* |1         |1      |8                     |2            | 1          |*/
    /* --------------------------------------------------------------------- */
    /*************************************************************************/

    /*Channel Load IE*/
    pst_chn_load_rpt = (mac_chn_load_rpt_stru*)pst_meas_rpt_ie->auc_meas_rpt;

    pst_chn_load_rpt->uc_optclass  = pst_chn_load_req->uc_optclass;
    pst_chn_load_rpt->uc_channum   = pst_chn_load_req->uc_channum;
    /*Actual Meas Start Time, To be added in scan process*/

    /*计算channel Load*/
    uc_chn_load = HMAC_RRM_CAL_CHN_LOAD(&(pst_scan_record->ast_chan_results[0]));

    oal_memcopy(pst_chn_load_rpt->aul_act_meas_start_time, pst_rrm_info->aul_act_meas_start_time,
                OAL_SIZEOF(pst_rrm_info->aul_act_meas_start_time));

    /*fill duration*/
    pst_chn_load_rpt->us_duration = pst_chn_load_req->us_duration;

    /*fill channel load*/
    pst_chn_load_rpt->uc_channel_load = uc_chn_load;

    /*send action report*/
    pst_rrm_info->pst_meas_rpt_ie->uc_len       = MAC_MEASUREMENT_RPT_FIX_LEN - MAC_IE_HDR_LEN + 13;
    pst_rrm_info->us_rm_rpt_action_len          += MAC_IE_HDR_LEN + pst_rrm_info->pst_meas_rpt_ie->uc_len;

    /*发帧*/
    hmac_rrm_send_rm_rpt_action(pst_hmac_vap);

    return;
}


OAL_STATIC oal_void hmac_rrm_parse_chn_load_req(hmac_vap_stru *pst_hmac_vap, mac_meas_req_ie_stru  *pst_meas_req_ie)
{
    mac_meas_rpt_mode_stru         st_rptmode;
    mac_chn_load_req_stru          *pst_chn_load_req;

    /*判断是否支持channel load测量*/
    OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
    if (OAL_FALSE == mac_mib_get_dot11RMChannelLoadMeasurementActivated(&pst_hmac_vap->st_vap_base_info))
    {
        st_rptmode.bit_incapable = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "channel load report not enable");
        return;
    }

    pst_chn_load_req = (mac_chn_load_req_stru *)&(pst_meas_req_ie->auc_meas_req[0]);

    /*小于最小测量周期则回复拒绝测量*/
    if(pst_chn_load_req->us_duration < 10)
    {
        st_rptmode.bit_refused = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_parse_chn_load_req: us_duration[%d] not support.}", pst_chn_load_req->us_duration);
        return;
    }

    /*CCA历史扫描结果仅扫描150ms,为了与request扫描时间对齐，单独发起扫描*/
    hmac_rrm_chn_load_scan_do(pst_hmac_vap, pst_chn_load_req);

}


OAL_STATIC oal_void hmac_rrm_get_chn_load_info_from_rpt(hmac_user_stru *pst_hmac_user, mac_meas_rpt_ie_stru *pst_meas_rpt_ie)
{
    mac_user_rrm_info_stru      *pst_rrm_info;
    mac_meas_rpt_chn_load_stru  *pst_meas_rpt_chn_load;
    mac_meas_rpt_chn_load_item_stru  *pst_meas_rpt_chn_load_item = OAL_PTR_NULL;
    mac_chn_load_rpt_stru       *pst_chn_load_rpt;

    pst_rrm_info = pst_hmac_user->pst_user_rrm_info;

    /*************************************************************************/
    /*                   channel load Report - Frame Body                     */
    /* --------------------------------------------------------------------- */
    /* |oper class|chn num|Actual Meas Start Time|Meas Duration|Channel Load|*/
    /* --------------------------------------------------------------------- */
    /* |1         |1      |8                     |2            | 1          |*/
    /* --------------------------------------------------------------------- */
    /*************************************************************************/

    pst_meas_rpt_chn_load = (mac_meas_rpt_chn_load_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_meas_rpt_chn_load_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_chn_load)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_get_chn_load_info_from_rpt::pst_meas_rpt_chn_load NULL}");
        return;
    }

    OAL_MEMZERO(pst_meas_rpt_chn_load, OAL_SIZEOF(mac_meas_rpt_chn_load_stru));

    pst_meas_rpt_chn_load_item = (mac_meas_rpt_chn_load_item_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_meas_rpt_chn_load_item_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_chn_load_item)
    {
        OAL_MEM_FREE(pst_meas_rpt_chn_load, OAL_TRUE);
        //pst_meas_rpt_chn_load = OAL_PTR_NULL;
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_get_chn_load_info_from_rpt::pst_meas_rpt_chn_load_item NULL}");
        return;
    }

    pst_meas_rpt_chn_load->pst_meas_rpt_chn_load_item = pst_meas_rpt_chn_load_item;

    /*fill node begin*/
    pst_meas_rpt_chn_load_item->uc_eid       = pst_meas_rpt_ie->uc_eid;
    pst_meas_rpt_chn_load_item->uc_len       = pst_meas_rpt_ie->uc_len;
    pst_meas_rpt_chn_load_item->uc_token     = pst_meas_rpt_ie->uc_token;
    oal_memcopy(&(pst_meas_rpt_chn_load_item->st_rptmode), &(pst_meas_rpt_ie->st_rptmode),
                    OAL_SIZEOF(pst_meas_rpt_ie->st_rptmode));

    pst_chn_load_rpt = (mac_chn_load_rpt_stru*)pst_meas_rpt_ie->auc_meas_rpt;

    pst_meas_rpt_chn_load_item->uc_optclass  = pst_chn_load_rpt->uc_optclass;
    pst_meas_rpt_chn_load_item->uc_channum   = pst_chn_load_rpt->uc_channum;
    oal_memcopy(pst_meas_rpt_chn_load_item->aul_act_meas_start_time, pst_chn_load_rpt->aul_act_meas_start_time,
                OAL_SIZEOF(pst_chn_load_rpt->aul_act_meas_start_time));
    pst_meas_rpt_chn_load_item->us_duration  = pst_chn_load_rpt->us_duration;
    pst_meas_rpt_chn_load_item->uc_chn_load  = pst_chn_load_rpt->uc_channel_load;
    /*fill node end*/

    OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_rrm_get_chn_load_info_from_rpt::uc_chn_load=%d.}",
        pst_meas_rpt_chn_load_item->uc_chn_load);

    oal_dlist_add_tail(&(pst_meas_rpt_chn_load->st_dlist_head_chn_load), &(pst_rrm_info->st_meas_rpt_list));
}

#define HMAC_11K_INTERNAL_FUNC_BCN

OAL_STATIC oal_uint32 hmac_rrm_bcn_rpt_filter(hmac_vap_stru *pst_hmac_vap, mac_bss_dscr_stru *pst_bss_dscr, mac_scan_req_stru *pst_scan_params)
{
    mac_vap_rrm_info_stru      *pst_rrm_info = pst_hmac_vap->pst_rrm_info;
    oal_uint8                   uc_ssid_len;
    oal_uint8                   uc_chan_idx;

    /*遍历信道*/
    for (uc_chan_idx = 0; uc_chan_idx < pst_scan_params->uc_channel_nums; uc_chan_idx++)
    {
        if ( pst_bss_dscr->st_channel.uc_chan_number ==
            pst_scan_params->ast_channel_list[uc_chan_idx].uc_chan_number)
        {
            break;
        }
    }

    if ( pst_scan_params->uc_channel_nums == uc_chan_idx)
    {
        return OAL_FAIL;
    }

    /* BSSID过滤 */
    if (!ETHER_IS_BROADCAST(pst_rrm_info->st_bcn_req_info.auc_bssid))
    {
        /* 请求中的bssid不是wildcard，则需要bssid匹配，不匹配时不上报 */
        if (OAL_MEMCMP(pst_rrm_info->st_bcn_req_info.auc_bssid, pst_bss_dscr->auc_bssid, WLAN_MAC_ADDR_LEN))
        {
            return OAL_FAIL;
        }
    }

    /* SSID过滤，若请求中ssid长度为0，则不过滤 */
    if (0 != pst_rrm_info->st_bcn_req_info.uc_ssid_len)
    {
        uc_ssid_len = (oal_uint8)OAL_STRLEN(pst_bss_dscr->ac_ssid);
        /* ssid不匹配的不处理 */
        if ((pst_rrm_info->st_bcn_req_info.uc_ssid_len != uc_ssid_len )
        || (0 != OAL_MEMCMP(pst_rrm_info->st_bcn_req_info.puc_ssid, pst_bss_dscr->ac_ssid, pst_rrm_info->st_bcn_req_info.uc_ssid_len)))
        {
            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_rrm_encap_meas_rpt_bcn(hmac_vap_stru *pst_hmac_vap, oal_dlist_head_stru *pst_bss_list_head)
{
    mac_vap_rrm_info_stru       *pst_rrm_info;
    mac_bcn_rpt_stru            *pst_bcn_rpt_item;
    mac_meas_rpt_ie_stru        *pst_meas_rpt_ie;
    oal_uint8                   uc_curr_beacon_item_len;
    oal_dlist_head_stru         *pst_entry;
    mac_bss_dscr_stru           *pst_bss_dscr;
    hmac_scanned_bss_info       *pst_scanned_bss;
    oal_uint32                  ul_rpt_detail_act_len = 0;
    mac_device_stru             *pst_mac_device;
    oal_uint32                  ul_ret;
    oal_uint8                   uc_bcn_item_num = 0;

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;
    pst_meas_rpt_ie = pst_rrm_info->pst_meas_rpt_ie;

    /* 获取hmac device */
    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_bcn::pst_hmac_device[%d] null.}",
                        pst_hmac_vap->st_vap_base_info.uc_device_id);
        return;
    }

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, pst_bss_list_head)
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        if (OAL_PTR_NULL == pst_scanned_bss)
        {
            continue;
        }

        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);

        ul_ret = hmac_rrm_bcn_rpt_filter(pst_hmac_vap, pst_bss_dscr, &(pst_mac_device->st_scan_params));
        if (OAL_FAIL == ul_ret)
        {
            continue;
        }

        /*record beacon report ie num*/
        uc_bcn_item_num++;

        if((pst_rrm_info->us_rm_rpt_action_len + MAC_MEASUREMENT_RPT_FIX_LEN + MAC_BEACON_RPT_FIX_LEN) > WLAN_LARGE_NETBUF_SIZE)
        {
            break;
        }

        /*************************************************************************/
        /*                   Measurement Report IE - Frame Body         */
        /* --------------------------------------------------------------------- */
        /* |Element ID |Length |Meas Token| Meas Rpt Mode | Meas Type | Meas Rpt|*/
        /* --------------------------------------------------------------------- */
        /* |1          |1      | 1        |  1            | 1         | var      */
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/

        pst_meas_rpt_ie->uc_eid       = MAC_EID_MEASREP;
        pst_meas_rpt_ie->uc_token     = pst_rrm_info->st_bcn_req_info.uc_meas_token;
        pst_meas_rpt_ie->uc_rpttype   = pst_rrm_info->st_bcn_req_info.uc_meas_type;
        OAL_MEMZERO(&(pst_meas_rpt_ie->st_rptmode), OAL_SIZEOF(mac_meas_rpt_mode_stru));

        /*************************************************************************/
        /*                   Beacon Report - Frame Body                          */
        /* --------------------------------------------------------------------- */
        /* |oper class|chn num|Actual Meas Start Time|Meas Duration|Rpt Frm Info|*/
        /* --------------------------------------------------------------------- */
        /* |1         |1      |8                     |2            | 1          |*/
        /* --------------------------------------------------------------------- */
        /* --------------------------------------------------------------------- */
        /* | RCPI | RSNI | BSSID | Antenna ID | Parent TSF| Optional Subelements|*/
        /* --------------------------------------------------------------------- */
        /* |1     |1     |6      |1           |4          | Var                 |*/
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/

        /*fixed bcn rpt*/
        pst_bcn_rpt_item = (mac_bcn_rpt_stru *)(pst_meas_rpt_ie->auc_meas_rpt);
        pst_bcn_rpt_item->uc_optclass               = pst_rrm_info->st_bcn_req_info.uc_opt_class;
        pst_bcn_rpt_item->uc_channum                = pst_bss_dscr->st_channel.uc_chan_number;
        oal_memcopy(pst_bcn_rpt_item->aul_act_meas_start_time, pst_hmac_vap->pst_rrm_info->aul_act_meas_start_time,
            OAL_SIZEOF(pst_hmac_vap->pst_rrm_info->aul_act_meas_start_time));

        /* Meas Duration,参考商用设备, bcn report与req duration填写一致 */
        pst_bcn_rpt_item->us_duration = pst_rrm_info->st_bcn_req_info.us_meas_duration;

        uc_curr_beacon_item_len = MAC_BEACON_RPT_FIX_LEN;

        pst_bcn_rpt_item->bit_condensed_phy_type    = pst_bss_dscr->uc_phy_type;
        pst_bcn_rpt_item->bit_rpt_frm_type          = 0; /* Beacon/Probe rsp */
        pst_bcn_rpt_item->uc_rcpi                   = HMAC_RRM_CAL_RCPI(pst_bss_dscr->c_rssi);
        pst_bcn_rpt_item->uc_rsni                   = (oal_uint8)((pst_bss_dscr->ac_rsni[0] + pst_bss_dscr->ac_rsni[1])/2);
        oal_memcopy(pst_bcn_rpt_item->auc_bssid, pst_bss_dscr->auc_bssid, OAL_SIZEOF(pst_bss_dscr->auc_bssid));
        pst_bcn_rpt_item->uc_antenna_id             = 0;//unknown
        pst_bcn_rpt_item->ul_parent_tsf             = pst_bss_dscr->ul_parent_tsf;

        /* Reported Frame Body EID&Len */
        /*目前不支持req info解析*/
        if(0)//(pst_rrm_info->st_bcn_req_info.uc_rpt_detail)
        {
            hmac_scan_update_bcn_rpt_detail(pst_hmac_vap, pst_bss_dscr, pst_bcn_rpt_item->auc_subelm + 2, &ul_rpt_detail_act_len);
            if (ul_rpt_detail_act_len)
            {
                uc_curr_beacon_item_len += (oal_uint8)ul_rpt_detail_act_len;
                *(pst_bcn_rpt_item->auc_subelm) = 1;
                *(pst_bcn_rpt_item->auc_subelm + 1) = (oal_uint8)ul_rpt_detail_act_len;
                uc_curr_beacon_item_len += MAC_IE_HDR_LEN;
            }
        }

        pst_meas_rpt_ie->uc_len = MAC_MEASUREMENT_RPT_FIX_LEN - MAC_IE_HDR_LEN  + uc_curr_beacon_item_len;
        pst_rrm_info->us_rm_rpt_action_len    += MAC_IE_HDR_LEN + pst_meas_rpt_ie->uc_len;

        /* 下一个Measurement Report的位置 */
        pst_meas_rpt_ie = (mac_meas_rpt_ie_stru *)((oal_uint8 *)pst_meas_rpt_ie + pst_meas_rpt_ie->uc_len + MAC_IE_HDR_LEN);
    }

    /*未找到测量结果则标示拒绝*/
    if ( 0 == uc_bcn_item_num )
    {
        pst_meas_rpt_ie->uc_eid       = MAC_EID_MEASREP;
        pst_meas_rpt_ie->uc_token     = pst_rrm_info->st_bcn_req_info.uc_meas_token;
        pst_meas_rpt_ie->uc_rpttype   = pst_rrm_info->st_bcn_req_info.uc_meas_type;
        OAL_MEMZERO(&(pst_meas_rpt_ie->st_rptmode), OAL_SIZEOF(mac_meas_rpt_mode_stru));
        pst_meas_rpt_ie->st_rptmode.bit_refused = 1;

        pst_meas_rpt_ie->uc_len = MAC_MEASUREMENT_RPT_FIX_LEN - MAC_IE_HDR_LEN;
        pst_rrm_info->us_rm_rpt_action_len    += MAC_IE_HDR_LEN + pst_meas_rpt_ie->uc_len;
    }
}


OAL_STATIC oal_uint32 hmac_rrm_bcn_scan_do(hmac_vap_stru *pst_hmac_vap, mac_bcn_req_stru *pst_bcn_req, mac_scan_req_stru *pst_scan_req)
{
    oal_uint32                      ul_ret;
    mac_meas_rpt_mode_stru          st_rptmode;

    pst_scan_req->en_bss_type = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    pst_scan_req->en_scan_mode = WLAN_SCAN_MODE_FOREGROUND;

    /*BSSID*过滤*/
    oal_set_mac_addr(pst_scan_req->auc_bssid[0], pst_bcn_req->auc_bssid);
    pst_scan_req->uc_bssid_num = 1;
    pst_scan_req->uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    pst_scan_req->uc_scan_func = MAC_SCAN_FUNC_BSS;
    pst_scan_req->uc_max_scan_count_per_channel = 1;
    pst_scan_req->uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;
    oal_set_mac_addr(pst_scan_req->auc_sour_mac_addr, mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info));
    /*SSID过滤*/
    oal_memcopy(pst_scan_req->ast_mac_ssid_set[0].auc_ssid,
                pst_hmac_vap->pst_rrm_info->st_bcn_req_info.puc_ssid,
                pst_hmac_vap->pst_rrm_info->st_bcn_req_info.uc_ssid_len);

    /*回调函数指针*/
    pst_scan_req->p_fn_cb                  = hmac_rrm_bcn_scan_cb;

    /* 直接调用扫描模块扫描请求处理函数 */
    ul_ret = hmac_scan_proc_scan_req_event_etc(pst_hmac_vap, pst_scan_req);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "hmac_rrm_bcn_scan_do:hmac_scan_add_req failed, ret=%d", ul_ret);
        /*扫描失败回复refuse bit*/
        OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
        st_rptmode.bit_refused = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
    }
    else
    {
        pst_hmac_vap->pst_rrm_info->en_is_measuring = OAL_TRUE;

        pst_hmac_vap->pst_rrm_info->st_meas_status_timer.ul_timeout = MAC_RRM_VAP_MEAS_STAUTS_TIME;

        FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->pst_rrm_info->st_meas_status_timer),
                               hmac_vap_meas_status_timeout,
                               pst_hmac_vap->pst_rrm_info->st_meas_status_timer.ul_timeout,
                               (oal_void *)pst_hmac_vap->pst_rrm_info,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
    }

    return ul_ret;
}


OAL_STATIC oal_void hmac_rrm_bcn_scan_cb(void *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    oal_uint32                       ul_ret;
    hmac_vap_stru                   *pst_hmac_vap;
    mac_meas_rpt_ie_stru            *pst_meas_rpt_ie;
    mac_meas_rpt_mode_stru          st_rptmode;

    /* 判断入参合法性 */
    if (OAL_PTR_NULL == p_scan_record)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_bcn_scan_cb: input pointer is null!}");
        return;
    }

    pst_scan_record = (hmac_scan_record_stru *)p_scan_record;

    /*获取hmac VAP*/
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_bcn_scan_cb: pst_hmac_vap is null!}");
        return;
    }

    OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
    /*判断扫描状态是否成功*/
    if (MAC_SCAN_SUCCESS != pst_scan_record->en_scan_rsp_status )
    {
        st_rptmode.bit_refused = 1;
        hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_bcn_scan_cb::scan fail, status[%d].",
                pst_scan_record->en_scan_rsp_status);
        return;
    }

    /*申请RPT管理帧内存*/
    ul_ret = hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap);
    if ( OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_RRM, "{hmac_bcn_scan_cb: hmac_rrm_fill_basic_rm_rpt_action ret[%d].}", ul_ret);
        return;
    }

    /*设置rpt IE point*/
    pst_meas_rpt_ie = (mac_meas_rpt_ie_stru *)pst_hmac_vap->pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;
    pst_hmac_vap->pst_rrm_info->pst_meas_rpt_ie = pst_meas_rpt_ie;

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    hmac_rrm_encap_meas_rpt_bcn(pst_hmac_vap, &(pst_bss_mgmt->st_bss_list_head));

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    /*发帧*/
    hmac_rrm_send_rm_rpt_action(pst_hmac_vap);

    return;
}


OAL_STATIC void hmac_scan_update_bcn_rpt_detail(hmac_vap_stru *pst_hmac_vap,
                                                mac_bss_dscr_stru *pst_bss_dscr,
                                                oal_uint8 *puc_rpt_detail_data,
                                                oal_uint32 *pul_rpt_detail_act_len)
{
#if 0
    oal_int32                   l_tmp_len;
    oal_uint8                   uc_fix_len;
    mac_vap_rrm_info_stru       *pst_rrm_info;
    oal_uint8                   uc_element_num;
    oal_uint8                   *puc_req_elements;
    oal_uint8                   uc_element_idx;
    oal_uint8                   *puc_rx_frame;
    oal_bool_enum_uint8         en_found_ie = OAL_FALSE;
    oal_uint8                   *puc_frame_body;
    oal_uint16                  us_frame_len;
    oal_uint8                   uc_subelm_len = 0;

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;

    puc_frame_body  = pst_bss_dscr->auc_mgmt_buff + MAC_80211_FRAME_LEN;
    us_frame_len    = (oal_uint16)pst_bss_dscr->ul_mgmt_len;

    /* 定长field拷贝 */
    uc_fix_len = MAC_TIME_STAMP_LEN + MAC_BEACON_INTERVAL_LEN + MAC_CAP_INFO_LEN;
    oal_memcopy(puc_rpt_detail_data, puc_frame_body, uc_fix_len);
    puc_rpt_detail_data += uc_fix_len;
    *pul_rpt_detail_act_len = uc_fix_len;

    /* 要寻找的IE个数及IEID */
    puc_req_elements = pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid;
    uc_element_num = pst_rrm_info->st_bcn_req_info.uc_req_ie_num;

    puc_rx_frame = puc_frame_body + uc_fix_len;

    l_tmp_len = us_frame_len - MAC_80211_FRAME_LEN - uc_fix_len;

    /* 存在有多个相同IE的场景，要找全 */
    while (l_tmp_len > MAC_IE_HDR_LEN )
    {
        /* 沿着被搜索对象，依次核对是否有被搜索EID */
        for (uc_element_idx = 0; uc_element_idx < uc_element_num; uc_element_idx++)
        {
            if (puc_rx_frame[0] == puc_req_elements[uc_element_idx])
            {
                en_found_ie = OAL_TRUE;
                break;
            }
        }

        if (OAL_TRUE == en_found_ie)
        {
            uc_subelm_len = *(puc_rx_frame + 1);
            if (*pul_rpt_detail_act_len + MAC_IE_HDR_LEN + uc_subelm_len > MAC_MAX_RPT_DETAIL_LEN)
            {
                OAM_WARNING_LOG1(0, OAM_SF_RRM, "{hmac_scan_update_bcn_rpt_detail::rpt detail over len[%d]}", *pul_rpt_detail_act_len + MAC_IE_HDR_LEN + uc_subelm_len);
                break;
            }
            oal_memcopy(puc_rpt_detail_data, puc_rx_frame, MAC_IE_HDR_LEN + uc_subelm_len);
            puc_rpt_detail_data += MAC_IE_HDR_LEN + uc_subelm_len;
            *pul_rpt_detail_act_len += MAC_IE_HDR_LEN + uc_subelm_len;
            en_found_ie = OAL_FALSE;
        }

        l_tmp_len   -= uc_subelm_len + MAC_IE_HDR_LEN;
        puc_rx_frame += uc_subelm_len + MAC_IE_HDR_LEN;

        if ((l_tmp_len < MAC_IE_HDR_LEN) || (l_tmp_len < (MAC_IE_HDR_LEN + uc_subelm_len)))
        {
            break;
        }
    }
#endif
}


oal_uint32 hmac_rrm_meas_bcn(hmac_vap_stru *pst_hmac_vap, mac_bcn_req_stru *pst_bcn_req, mac_scan_req_stru   *pst_scan_req)
{
    mac_vap_rrm_info_stru               *pst_rrm_info;
    mac_vap_rrm_trans_req_info_stru     st_trans_req_info;
    mac_meas_rpt_mode_stru              st_rptmode;

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;

    /* 根据模式进行测量 */
    OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));
    switch (pst_bcn_req->en_mode)
    {
        /* Passive:触发扫描，不发probe req，测量收到的Beacon和probe rsp */
        case RM_BCN_REQ_MEAS_MODE_PASSIVE:
            if (OAL_FALSE == mac_mib_get_dot11RMBeaconPassiveMeasurementActivated(&pst_hmac_vap->st_vap_base_info))
            {
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "passive measurement mode not enable");
                st_rptmode.bit_incapable = 1;
                hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
                return OAL_FAIL;
            }

            pst_scan_req->en_scan_type = WLAN_SCAN_TYPE_PASSIVE;
            /* 测量时间, 需要足够长的时间以保证可以收到指定beacon */
            pst_scan_req->us_scan_time = MAC_RRM_BCN_REQ_PASSIVE_SCAN_TIME;//WLAN_DEFAULT_PASSIVE_SCAN_TIME;

            hmac_rrm_bcn_scan_do(pst_hmac_vap, pst_bcn_req, pst_scan_req);
            break;

        /* Active:触发扫描，发probe req，测量收到的Beacon和probe rsp */
        case RM_BCN_REQ_MEAS_MODE_ACTIVE:
            if (OAL_FALSE == mac_mib_get_dot11RMBeaconActiveMeasurementActivated(&pst_hmac_vap->st_vap_base_info))
            {
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "active measurement mode not enable");
                st_rptmode.bit_incapable = 1;
                hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
                return OAL_FAIL;
            }

            pst_scan_req->en_scan_type = WLAN_SCAN_TYPE_ACTIVE;
            /* 测量时间 TBD*/
            pst_scan_req->us_scan_time = WLAN_DEFAULT_ACTIVE_SCAN_TIME;

            hmac_rrm_bcn_scan_do(pst_hmac_vap, pst_bcn_req, pst_scan_req);
            break;

        /* Table:上报保存的扫描结果 */
        case RM_BCN_REQ_MEAS_MODE_TABLE:
            if (OAL_FALSE == pst_hmac_vap->bit_bcn_table_switch
                || OAL_FALSE == mac_mib_get_dot11RMBeaconTableMeasurementActivated(&pst_hmac_vap->st_vap_base_info))
            {
                st_rptmode.bit_incapable = 1;
                hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_meas_bcn:table mode is shutdown!");
                return OAL_FAIL;
            }

            oal_memcopy(st_trans_req_info.auc_bssid, pst_rrm_info->st_bcn_req_info.auc_bssid, WLAN_MAC_ADDR_LEN);
            oal_memcopy(st_trans_req_info.auc_ssid, pst_rrm_info->st_bcn_req_info.puc_ssid, pst_rrm_info->st_bcn_req_info.uc_ssid_len);
            st_trans_req_info.us_ssid_len = (oal_uint16)pst_rrm_info->st_bcn_req_info.uc_ssid_len;
            st_trans_req_info.uc_action_dialog_token = pst_rrm_info->st_bcn_req_info.uc_dialog_token;
            st_trans_req_info.uc_meas_token = pst_rrm_info->st_bcn_req_info.uc_meas_token;
            st_trans_req_info.uc_oper_class = pst_rrm_info->st_bcn_req_info.uc_opt_class;
            st_trans_req_info.us_duration = pst_rrm_info->st_bcn_req_info.us_meas_duration;

            hmac_scan_rrm_proc_save_bss_etc(&(pst_hmac_vap->st_vap_base_info), OAL_SIZEOF(st_trans_req_info), (oal_uint8 *)&st_trans_req_info);
            break;

        default:
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "unkown measurement mode.");
            st_rptmode.bit_refused = 1;
            hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
            return OAL_FAIL;

    }
    return OAL_SUCC;
}


oal_uint32 hmac_rrm_get_bcn_rpt_channels(mac_bcn_req_stru *pst_bcn_req, mac_ap_chn_rpt_stru **ppst_ap_chn_rpt, oal_uint8 uc_ap_chn_rpt_num, mac_scan_req_stru *pst_scan_req, wlan_channel_band_enum_uint8 en_chan_band)
{
    oal_uint8                       uc_chan_idx;
    oal_uint8                       uc_chan_num = 0;
    oal_uint8                       uc_chan_avail_idx = 0;
    oal_uint8                       uc_ap_chan_num;
    oal_uint8                       uc_ap_chan_idx = 0;
    oal_uint8                       uc_ap_chan_rpt_count;
    oal_uint8                       uc_chan_count;
    oal_uint32                      ul_ret = OAL_SUCC;

    /* 对应指定用例进行打桩 */
    if (WLAN_BAND_2G == en_chan_band)
    {
        uc_chan_count = (oal_uint8)MAC_CHANNEL_FREQ_2_BUTT;
    }
    else
    {
        uc_chan_count = (oal_uint8)MAC_CHANNEL_FREQ_5_BUTT;
    }

    /* 当前operating class下的所有chan，先不处理regclass，使用2.4G信道总集 */
    if (0 == pst_bcn_req->uc_channum)
    {
        /* 根据channel_bitmap解析出对应的信道号集合 */
        for (uc_chan_idx = 0; uc_chan_idx < uc_chan_count; uc_chan_idx++)
        {
            /* 判断信道是不是在管制域内 */
            if (OAL_SUCC == mac_is_channel_idx_valid_etc(en_chan_band, uc_chan_idx))
            {
                mac_get_channel_num_from_idx_etc(en_chan_band, uc_chan_idx, &uc_chan_num);
                pst_scan_req->ast_channel_list[uc_chan_avail_idx].uc_chan_number = uc_chan_num;
                pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_band        = en_chan_band;
                pst_scan_req->ast_channel_list[uc_chan_avail_idx++].uc_chan_idx       = uc_chan_idx;
            }
        }
        pst_scan_req->uc_channel_nums = uc_chan_avail_idx;
    }
    /* 当前operating class与AP chan rpt的交集 */
    else if (255 == pst_bcn_req->uc_channum)
    {
        if (0 == uc_ap_chn_rpt_num)
        {
            OAM_WARNING_LOG0(0, OAM_SF_RRM, "hmac_rrm_get_bcn_rpt_channels: channum is 255,but NO ap_chan_rpt ie");
            return OAL_FAIL;
        }

        for (uc_ap_chan_rpt_count = 0; uc_ap_chan_rpt_count < uc_ap_chn_rpt_num; uc_ap_chan_rpt_count++)
        {
            /* 请求的信道个数，根据长度计算 */
            uc_ap_chan_num = ppst_ap_chn_rpt[uc_ap_chan_rpt_count]->uc_length - 1;

            for (uc_chan_idx = 0; uc_chan_idx < uc_ap_chan_num; uc_chan_idx++)
            {
                uc_chan_num = *(&(ppst_ap_chn_rpt[uc_ap_chan_rpt_count]->auc_chan[0]) + uc_chan_idx);
                en_chan_band = mac_get_band_by_channel_num(uc_chan_num);
                if(OAL_SUCC != mac_get_channel_idx_from_num_etc(en_chan_band, uc_chan_num, &uc_ap_chan_idx))
                {
                    continue;
                }

                /* 检查信道号是否有效 */
                if (OAL_SUCC != mac_is_channel_idx_valid_etc(en_chan_band, uc_ap_chan_idx))
                {
                    continue;
                }
                pst_scan_req->ast_channel_list[uc_chan_avail_idx].uc_chan_number = uc_chan_num;
                pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_band        = en_chan_band;
                pst_scan_req->ast_channel_list[uc_chan_avail_idx++].uc_chan_idx  = uc_ap_chan_idx;

                /*AP chan rpt信道个数不超出管制域扫描信道个数*/
                if (uc_chan_count == uc_chan_avail_idx)
                {
                    break;
                }
            }

            if (uc_chan_count == uc_chan_avail_idx)
            {
                break;
            }
        }
        pst_scan_req->uc_channel_nums = uc_chan_avail_idx;
    }
    /* 当前chan num */
    else
    {
        uc_chan_num = pst_bcn_req->uc_channum;
        pst_scan_req->uc_channel_nums = 1;
        en_chan_band = mac_get_band_by_channel_num(uc_chan_num);
        ul_ret = mac_get_channel_idx_from_num_etc(en_chan_band, uc_chan_num, &uc_ap_chan_idx);
        if(ul_ret != OAL_SUCC)
        {
            OAM_WARNING_LOG1(0, OAM_SF_RRM, "hmac_rrm_get_bcn_rpt_channels: mac_get_channel_idx_from_num_etc fail,error_code=%d", ul_ret);
            return ul_ret;
        }
        /* 临时调试，需要更新管制类表格 */
        pst_scan_req->ast_channel_list[0].uc_chan_number = uc_chan_num;
        pst_scan_req->ast_channel_list[0].uc_chan_idx         = uc_ap_chan_idx;
        pst_scan_req->ast_channel_list[0].en_band        = en_chan_band;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_void hmac_rrm_parse_beacon_req(hmac_vap_stru *pst_hmac_vap, mac_meas_req_ie_stru  *pst_meas_req_ie)
{
    mac_bcn_req_stru               *pst_bcn_req;
    mac_ap_chn_rpt_stru            *apst_ap_chn_rpt[MAC_11K_SUPPORT_AP_CHAN_RPT_NUM];
    oal_uint8                       uc_ap_chn_rpt_count = 0;
    oal_int8                        c_bcn_req_sub_len;
    oal_uint8                      *puc_ap_ch_rpt_search_addr;
    oal_int8                        c_ap_ch_rpt_search_len;
    oal_uint8                      *puc_rpt_detail_search_addr;
    oal_uint8                      *puc_reporting_detail;
    oal_uint8                      *puc_reqinfo_search_addr;
    oal_uint8                      *puc_ssid_search_addr;
    oal_uint8                      *puc_ssid_sub_element;
    oal_uint8                      *puc_rpt_info_search_addr;
    oal_uint8                      *puc_rpt_info_sub_element;
    oal_uint8                      *puc_reqinfo;
    mac_scan_req_stru               st_scan_req;
    mac_vap_rrm_info_stru          *pst_rrm_info;
    mac_meas_rpt_mode_stru          st_rptmode;
    oal_uint32                      ul_ret = OAL_SUCC;

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;

    OAL_MEMZERO(&st_scan_req, OAL_SIZEOF(mac_scan_req_stru));

    pst_bcn_req = (mac_bcn_req_stru *)&(pst_meas_req_ie->auc_meas_req[0]);

    oal_memcopy(pst_rrm_info->st_bcn_req_info.auc_bssid, pst_bcn_req->auc_bssid, WLAN_MAC_ADDR_LEN);
    OAL_MEMZERO(&st_rptmode, OAL_SIZEOF(mac_meas_rpt_mode_stru));

    /*************************************************************************/
    /*                    Beacon Request                                     */
    /* --------------------------------------------------------------------- */
    /* |Operating Class |Channel Number |Rand Interval| Meas Duration       |*/
    /* --------------------------------------------------------------------- */
    /* |1               |1              | 2           | 2                   |*/
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |Meas Mode       |BSSID          |Optional Subelements               |*/
    /* --------------------------------------------------------------------- */
    /* |1               |6              | var                               |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    OAM_WARNING_LOG4(0, OAM_SF_RRM, "{hmac_rrm_parse_beacon_req::frame_body::Operating Class[%d],Channel Number[%d],Rand Interval[%u],Meas Duration[%u]}",
        pst_bcn_req->uc_optclass,
        pst_bcn_req->uc_channum,
        pst_bcn_req->us_random_ivl,
        pst_bcn_req->us_duration);

    OAM_WARNING_LOG4(0, OAM_SF_RRM, "{hmac_rrm_parse_beacon_req::frame_body::Meas Mode[%d], BSSID[%x:%x:%x]}",
        pst_bcn_req->en_mode,
        pst_bcn_req->auc_bssid[3],
        pst_bcn_req->auc_bssid[4],
        pst_bcn_req->auc_bssid[5]);

    /* 从Subelements中获取被测信道集合,AP Channel Report可能会有多个 */
    c_bcn_req_sub_len = (oal_int8)pst_meas_req_ie->uc_len - 16;
    if (c_bcn_req_sub_len <= 0)
    {
        ul_ret = hmac_rrm_get_bcn_rpt_channels(pst_bcn_req, apst_ap_chn_rpt,  uc_ap_chn_rpt_count, &st_scan_req, pst_hmac_vap->st_vap_base_info.st_channel.en_band);
        if ( OAL_SUCC !=  ul_ret)
        {
            st_rptmode.bit_refused = 1;
            hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
            return;
        }
    }
    else
    {
        puc_ap_ch_rpt_search_addr = pst_bcn_req->auc_subelm;
        c_ap_ch_rpt_search_len = c_bcn_req_sub_len;
        do {
            apst_ap_chn_rpt[uc_ap_chn_rpt_count] = (mac_ap_chn_rpt_stru *)mac_find_ie_etc(MAC_EID_AP_CHAN_REPORT, puc_ap_ch_rpt_search_addr, c_ap_ch_rpt_search_len);
            if (OAL_PTR_NULL == apst_ap_chn_rpt[uc_ap_chn_rpt_count])
            {
                break;
            }
            puc_ap_ch_rpt_search_addr = (oal_uint8 *)apst_ap_chn_rpt[uc_ap_chn_rpt_count] + 2 + apst_ap_chn_rpt[uc_ap_chn_rpt_count]->uc_length;
            c_ap_ch_rpt_search_len = (oal_int8)(c_bcn_req_sub_len - (oal_uint8)((oal_uint8 *)apst_ap_chn_rpt[uc_ap_chn_rpt_count] - pst_bcn_req->auc_subelm) - (2 + apst_ap_chn_rpt[uc_ap_chn_rpt_count]->uc_length));
            uc_ap_chn_rpt_count++;
            if (MAC_11K_SUPPORT_AP_CHAN_RPT_NUM == uc_ap_chn_rpt_count)
            {
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "ap chan rpt num is larger than 8, truncate the later ones");
                break;
            }
        }while (c_ap_ch_rpt_search_len > 0);

        ul_ret = hmac_rrm_get_bcn_rpt_channels(pst_bcn_req, apst_ap_chn_rpt,  uc_ap_chn_rpt_count, &st_scan_req, pst_hmac_vap->st_vap_base_info.st_channel.en_band);
        if ( OAL_SUCC !=  ul_ret)
        {
            st_rptmode.bit_refused = 1;
            hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
            return;
        }

        /*获取Beacon Reporting Information*/
        puc_rpt_info_search_addr = pst_bcn_req->auc_subelm;
        puc_rpt_info_sub_element = mac_find_ie_etc(1, puc_rpt_info_search_addr, c_bcn_req_sub_len);
        if (OAL_PTR_NULL != puc_rpt_info_sub_element)
        {
            pst_rrm_info->st_bcn_req_info.uc_rpt_condition = *(puc_rpt_info_sub_element + 2);
            pst_rrm_info->st_bcn_req_info.uc_rpt_ref_val   = *(puc_rpt_info_sub_element + 3);
            OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_parse_beacon_req::frame_body::rpt_condition[%d],rpt_ref_val[%d].",
                pst_rrm_info->st_bcn_req_info.uc_rpt_condition, pst_rrm_info->st_bcn_req_info.uc_rpt_ref_val);
            /*Repeat模式下使能Report Condition*/
            if ((0!=pst_rrm_info->st_bcn_req_info.uc_rpt_condition)&&
                (OAL_FALSE == mac_mib_get_dot11RMRepeatedMeasurementsActivated(&pst_hmac_vap->st_vap_base_info)
                || OAL_FALSE == mac_mib_get_dot11RMBeaconMeasurementReportingConditionsActivated(&pst_hmac_vap->st_vap_base_info)
                || 0 == pst_rrm_info->st_bcn_req_info.us_repetition))
            {
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "BeaconMeasurementReportingConditions not enable");
                st_rptmode.bit_incapable = 1;
                hmac_rrm_encap_meas_rpt_reject(pst_hmac_vap, &st_rptmode);
                return;
            }
        }

        /* 获取SSID */
        puc_ssid_search_addr = pst_bcn_req->auc_subelm;
        puc_ssid_sub_element = mac_find_ie_etc(MAC_EID_SSID, puc_ssid_search_addr, c_bcn_req_sub_len);
        if (OAL_PTR_NULL != puc_ssid_sub_element)
        {
            pst_rrm_info->st_bcn_req_info.uc_ssid_len = *(puc_ssid_sub_element + 1);
            if (0 != pst_rrm_info->st_bcn_req_info.uc_ssid_len)
            {
                pst_rrm_info->st_bcn_req_info.puc_ssid = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, pst_rrm_info->st_bcn_req_info.uc_ssid_len, OAL_TRUE);
                if (OAL_PTR_NULL == pst_rrm_info->st_bcn_req_info.puc_ssid)
                {
                    OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_parse_beacon_req::memalloc ssid fail}");
                    return;
                }

                oal_memcopy(pst_rrm_info->st_bcn_req_info.puc_ssid, (puc_ssid_sub_element + 2), pst_rrm_info->st_bcn_req_info.uc_ssid_len);
            }
            else
            {
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_parse_beacon_req::frame_body::ssid_len is 0.");
            }
        }

        /* 获取Reporting detail */
        puc_rpt_detail_search_addr = pst_bcn_req->auc_subelm;
        puc_reporting_detail = mac_find_ie_etc(2, puc_rpt_detail_search_addr, c_bcn_req_sub_len);
        if (OAL_PTR_NULL != puc_reporting_detail)
        {
            pst_rrm_info->st_bcn_req_info.uc_rpt_detail = *(puc_reporting_detail + 2);
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_parse_beacon_req::frame_body::rpt_detail[%d].",
                pst_rrm_info->st_bcn_req_info.uc_rpt_detail);
        }

        /* 获取ReqInfo */
        puc_reqinfo_search_addr = pst_bcn_req->auc_subelm;
        puc_reqinfo = mac_find_ie_etc(MAC_EID_REQINFO, puc_reqinfo_search_addr, c_bcn_req_sub_len);
        if (OAL_PTR_NULL != puc_reqinfo)
        {
            pst_rrm_info->st_bcn_req_info.uc_req_ie_num = *(puc_reqinfo + 1);
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_parse_beacon_req::frame_body::req_ie_num[%d].",
                pst_rrm_info->st_bcn_req_info.uc_req_ie_num);
            pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, pst_rrm_info->st_bcn_req_info.uc_req_ie_num, OAL_TRUE);
            if (OAL_PTR_NULL == pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid)
            {
                OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_parse_beacon_req::memalloc reqinfo_ieid fail}");
                return;
            }
            oal_memcopy(pst_rrm_info->st_bcn_req_info.puc_reqinfo_ieid, (puc_reqinfo + 2), pst_rrm_info->st_bcn_req_info.uc_req_ie_num);
        }
    }

    pst_rrm_info->st_bcn_req_info.uc_opt_class = pst_bcn_req->uc_optclass;
    pst_rrm_info->st_bcn_req_info.us_meas_duration = pst_bcn_req->us_duration;

    hmac_rrm_meas_bcn(pst_hmac_vap, pst_bcn_req, &st_scan_req);
}



OAL_STATIC oal_void hmac_rrm_get_bcn_info_from_rpt(hmac_user_stru *pst_hmac_user, mac_meas_rpt_ie_stru *pst_meas_rpt_ie)
{
    mac_user_rrm_info_stru      *pst_rrm_info;
    mac_meas_rpt_bcn_stru       *pst_meas_rpt_bcn;
    mac_meas_rpt_bcn_item_stru  *pst_meas_rpt_bcn_item = OAL_PTR_NULL;
    mac_bcn_rpt_stru            *pst_bcn_rpt;

    pst_rrm_info = pst_hmac_user->pst_user_rrm_info;
    /*************************************************************************/
    /*                   Beacon Report - Frame Body                          */
    /* --------------------------------------------------------------------- */
    /* |oper class|chn num|Actual Meas Start Time|Meas Duration|Rpt Frm Info|*/
    /* --------------------------------------------------------------------- */
    /* |1         |1      |8                     |2            | 1          |*/
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | RCPI | RSNI | BSSID | Antenna ID | Parent TSF| Optional Subelements|*/
    /* --------------------------------------------------------------------- */
    /* |1     |1     |6      |1           |4          | Var                 |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    pst_meas_rpt_bcn = (mac_meas_rpt_bcn_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_meas_rpt_bcn_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_bcn)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_get_bcn_info_from_rpt::pst_meas_rpt_bcn NULL}");
        return;
    }

    OAL_MEMZERO(pst_meas_rpt_bcn, OAL_SIZEOF(mac_meas_rpt_bcn_stru));

    pst_meas_rpt_bcn_item = (mac_meas_rpt_bcn_item_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_meas_rpt_bcn_item_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_bcn_item)
    {
        OAL_MEM_FREE(pst_meas_rpt_bcn, OAL_TRUE);
        pst_meas_rpt_bcn = OAL_PTR_NULL;
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_rrm_get_bcn_info_from_rpt::pst_meas_rpt_bcn_item NULL}");
        return;
    }

    pst_meas_rpt_bcn->pst_meas_rpt_bcn_item = pst_meas_rpt_bcn_item;

    /*fill node begin*/
    pst_meas_rpt_bcn_item->uc_eid       = pst_meas_rpt_ie->uc_eid;
    pst_meas_rpt_bcn_item->uc_len       = pst_meas_rpt_ie->uc_len;
    pst_meas_rpt_bcn_item->uc_token     = pst_meas_rpt_ie->uc_token;
    pst_meas_rpt_bcn_item->bit_late     = pst_meas_rpt_ie->st_rptmode.bit_late;
    pst_meas_rpt_bcn_item->bit_incapable= pst_meas_rpt_ie->st_rptmode.bit_incapable;
    pst_meas_rpt_bcn_item->bit_refused  = pst_meas_rpt_ie->st_rptmode.bit_refused;

    pst_bcn_rpt = (mac_bcn_rpt_stru*)pst_meas_rpt_ie->auc_meas_rpt;

    pst_meas_rpt_bcn_item->uc_optclass  = pst_bcn_rpt->uc_optclass;
    pst_meas_rpt_bcn_item->uc_channum   = pst_bcn_rpt->uc_channum;
    oal_memcopy(pst_meas_rpt_bcn_item->aul_act_meas_start_time, pst_bcn_rpt->aul_act_meas_start_time,
                OAL_SIZEOF(pst_bcn_rpt->aul_act_meas_start_time));
    pst_meas_rpt_bcn_item->us_duration  = pst_bcn_rpt->us_duration;

    /* Rpt Frm Info */
    pst_meas_rpt_bcn_item->bit_rpt_frm_type        = pst_bcn_rpt->bit_rpt_frm_type;                /* Beacon/Probe rsp */
    pst_meas_rpt_bcn_item->bit_condensed_phy_type  = pst_bcn_rpt->bit_condensed_phy_type;    /* need modify */

    pst_meas_rpt_bcn_item->uc_rcpi                 = pst_bcn_rpt->uc_rcpi;
    pst_meas_rpt_bcn_item->uc_rsni                 = pst_bcn_rpt->uc_rsni;

    /* BSSID */
    oal_set_mac_addr(pst_meas_rpt_bcn_item->auc_bssid, pst_bcn_rpt->auc_bssid);
    pst_meas_rpt_bcn_item->uc_antenna_id           = pst_bcn_rpt->uc_antenna_id;
    /* Parent TSF */
    pst_meas_rpt_bcn_item->ul_parent_tsf           = pst_bcn_rpt->ul_parent_tsf;
    /*fill node end*/

    OAM_WARNING_LOG4(0, OAM_SF_RRM, "{hmac_rrm_get_bcn_info_from_rpt::rcpi=%x, rssi=%d, mac_addr=%x:%x:%x!}",
        pst_meas_rpt_bcn_item->uc_rcpi,
        (oal_int8)(pst_meas_rpt_bcn_item->uc_rcpi/2 - 110),
        pst_meas_rpt_bcn_item->auc_bssid[4],
        pst_meas_rpt_bcn_item->auc_bssid[5]);

    oal_dlist_add_tail(&(pst_meas_rpt_bcn->st_dlist_head), &(pst_rrm_info->st_meas_rpt_list));
}

#define HMAC_11K_INTERNAL_FUNC_NEIGHBOR

OAL_STATIC oal_void hmac_rrm_get_neighbor_rpt_channels(mac_scan_req_stru *pst_scan_req)
{
    oal_uint8                       uc_chan_idx;
    oal_uint8                       uc_chan_num;
    oal_uint8                       uc_chan_avail_idx = 0;

    /*2G全信道扫描*/
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++)
    {
        /* 检查信道号是否有效 */
        if (OAL_SUCC != mac_is_channel_idx_valid_etc(WLAN_BAND_2G, uc_chan_idx))
        {
            continue;
        }

        mac_get_channel_num_from_idx_etc(WLAN_BAND_2G, uc_chan_idx, &uc_chan_num);
        pst_scan_req->ast_channel_list[uc_chan_avail_idx].uc_chan_number = uc_chan_num;
        pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_band        = WLAN_BAND_2G;
        pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_bandwidth   = WLAN_BAND_WIDTH_20M;
        pst_scan_req->ast_channel_list[uc_chan_avail_idx++].uc_chan_idx       = uc_chan_idx;
    }

    /*5G全信道扫描*/
    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++)
    {
        /* 检查信道号是否有效 */
        if (OAL_SUCC != mac_is_channel_idx_valid_etc(WLAN_BAND_5G, uc_chan_idx))
        {
            continue;
        }

        mac_get_channel_num_from_idx_etc(WLAN_BAND_5G, uc_chan_idx, &uc_chan_num);
        pst_scan_req->ast_channel_list[uc_chan_avail_idx].uc_chan_number = uc_chan_num;
        pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_band        = WLAN_BAND_5G;
        pst_scan_req->ast_channel_list[uc_chan_avail_idx].en_bandwidth   = WLAN_BAND_WIDTH_20M;
        pst_scan_req->ast_channel_list[uc_chan_avail_idx++].uc_chan_idx       = uc_chan_idx;
    }

    /*扫描信道个数*/
    pst_scan_req->uc_channel_nums = uc_chan_avail_idx;

}


OAL_STATIC oal_void hmac_rrm_encap_meas_rpt_neighbor(hmac_vap_stru *pst_hmac_vap, oal_dlist_head_stru *pst_bss_list_head)
{
    mac_vap_rrm_info_stru       *pst_rrm_info;
    mac_neighbor_rpt_ie_stru    *pst_neighbor_rpt_ie;
    oal_dlist_head_stru         *pst_entry;
    mac_bss_dscr_stru           *pst_bss_dscr;
    hmac_scanned_bss_info       *pst_scanned_bss;
    mac_cap_info_stru           *pst_cap_info;
    mac_device_stru             *pst_mac_device;
    oal_uint8                   uc_ssid_len;

    pst_rrm_info = pst_hmac_vap->pst_rrm_info;
    if(OAL_PTR_NULL == pst_rrm_info)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_neighbor::pst_rrm_info null.}");
        return;
    }

    /*设置rpt IE point*/
    pst_neighbor_rpt_ie = (mac_neighbor_rpt_ie_stru *)pst_hmac_vap->pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;
    if(OAL_PTR_NULL == pst_neighbor_rpt_ie)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_neighbor::pst_neighbor_rpt_ie null.}");
        return;
    }

    /*************************************************************************/
    /*                   Measurement Report IE - Frame Body         */
    /* --------------------------------------------------------------------- */
    /* |Element ID |Length |Meas Token| Meas Rpt Mode | Meas Type | Meas Rpt|*/
    /* --------------------------------------------------------------------- */
    /* |1          |1      | 1        |  1            | 1         | var      */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    /* 获取hmac device */
    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "{hmac_rrm_encap_meas_rpt_neighbor::pst_hmac_device[%d] null.}",
                        pst_hmac_vap->st_vap_base_info.uc_device_id);
        return;
    }

    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, pst_bss_list_head)
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        if (OAL_PTR_NULL == pst_scanned_bss)
        {
            continue;
        }

        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if (OAL_PTR_NULL == pst_bss_dscr)
        {
            continue;
        }

        /* SSID过滤，若请求中ssid长度为0，则不过滤 */
        if (0 != pst_rrm_info->st_neighbor_req_info.uc_ssid_len)
        {
            uc_ssid_len = (oal_uint8)OAL_STRLEN(pst_bss_dscr->ac_ssid);
            /* ssid不匹配的不处理 */
            if ((pst_rrm_info->st_neighbor_req_info.uc_ssid_len != uc_ssid_len )
            || (0 != OAL_MEMCMP(pst_rrm_info->st_neighbor_req_info.puc_ssid, pst_bss_dscr->ac_ssid, pst_rrm_info->st_bcn_req_info.uc_ssid_len)))
            {
                continue;
            }
        }

        if((pst_rrm_info->us_rm_rpt_action_len + MAC_IE_HDR_LEN + MAC_NEIGHBOR_REPORT_IE_LEN) > WLAN_LARGE_NETBUF_SIZE)
        {
            break;
        }
        /*Neighbor Report IE*/
        pst_neighbor_rpt_ie->uc_eid       = MAC_EID_NEIGHBOR_REPORT;
        oal_memcopy(pst_neighbor_rpt_ie->auc_bssid, pst_bss_dscr->auc_bssid, OAL_SIZEOF(pst_bss_dscr->auc_bssid));

        /*BSSID Information*/
        pst_neighbor_rpt_ie->st_bssid_info.bit_ap_reachability    = 0; //not support preauth
        pst_neighbor_rpt_ie->st_bssid_info.bit_security           = 0; //not support
        pst_neighbor_rpt_ie->st_bssid_info.bit_key_scope          = 0; //not support key_scope
        pst_cap_info = (mac_cap_info_stru *)&(pst_bss_dscr->us_cap_info);
        pst_neighbor_rpt_ie->st_bssid_info.bit_spec_management    = pst_cap_info->bit_spectrum_mgmt;
        pst_neighbor_rpt_ie->st_bssid_info.bit_qos                = pst_cap_info->bit_qos;
        pst_neighbor_rpt_ie->st_bssid_info.bit_apsd               = pst_cap_info->bit_apsd;
        pst_neighbor_rpt_ie->st_bssid_info.bit_radio_meas         = pst_cap_info->bit_radio_measurement;
        pst_neighbor_rpt_ie->st_bssid_info.bit_delayed_ba         = pst_cap_info->bit_delayed_block_ack;
        pst_neighbor_rpt_ie->st_bssid_info.bit_immediate_ba       = pst_cap_info->bit_immediate_block_ack;

        /*optclass*/
        pst_neighbor_rpt_ie->uc_optclass            = 0;
        pst_neighbor_rpt_ie->uc_channum             = pst_bss_dscr->st_channel.uc_chan_number;
        pst_neighbor_rpt_ie->uc_phy_type            = pst_bss_dscr->uc_phy_type;

        pst_neighbor_rpt_ie->uc_len                 = MAC_NEIGHBOR_REPORT_IE_LEN;
        pst_rrm_info->us_rm_rpt_action_len          += MAC_IE_HDR_LEN + pst_neighbor_rpt_ie->uc_len;

        /* 下一个Measurement Report的位置 */
        pst_neighbor_rpt_ie = (mac_neighbor_rpt_ie_stru *)((oal_uint8 *)pst_neighbor_rpt_ie + pst_neighbor_rpt_ie->uc_len + 2);
    }
}


oal_uint32 hmac_rrm_neighbor_scan_do(hmac_vap_stru *pst_hmac_vap, mac_neighbor_req_info_stru *pst_neighbor_req,
                 oal_bool_enum_uint8 en_local_meas)
{
    mac_scan_req_stru               st_scan_req;
    oal_uint32                      ul_ret;

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_neighbor_scan_do::input para NULL.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(&st_scan_req, OAL_SIZEOF(st_scan_req));

    st_scan_req.en_scan_type = WLAN_SCAN_TYPE_ACTIVE;
    st_scan_req.us_scan_time = WLAN_NEIGHBOR_SCAN_TIME;

    st_scan_req.en_bss_type = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req.en_scan_mode = WLAN_SCAN_MODE_FOREGROUND;

    st_scan_req.uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    st_scan_req.uc_scan_func = MAC_SCAN_FUNC_BSS;
    st_scan_req.uc_max_scan_count_per_channel = 1;
    st_scan_req.uc_max_send_probe_req_count_per_channel = WLAN_DEFAULT_SEND_PROBE_REQ_COUNT_PER_CHANNEL;
    st_scan_req.uc_probe_delay = 0;
    /*SSID过滤*/
    if(0 != pst_neighbor_req->uc_ssid_len)
    {
        oal_memcopy(st_scan_req.ast_mac_ssid_set[0].auc_ssid, pst_neighbor_req->puc_ssid, pst_neighbor_req->uc_ssid_len);
    }

    /*全信道扫描*/
    hmac_rrm_get_neighbor_rpt_channels(&st_scan_req);

    /*回调函数指针*/
    if ( OAL_FALSE == en_local_meas )
    {
        st_scan_req.p_fn_cb = hmac_rrm_neighbor_scan_cb;

    }

    /* 直接调用扫描模块扫描请求处理函数 */
    ul_ret = hmac_scan_proc_scan_req_event_etc(pst_hmac_vap, &st_scan_req);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_RRM, "hmac_rrm_neighbor_scan_do:hmac_scan_proc_scan_req_event_etc failed, ret=%d", ul_ret);
        if ( OAL_FALSE == en_local_meas )
        {
            /*扫描失败回复不带IE的neighbor Report*/
            if ( OAL_SUCC != hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap))
            {
                return OAL_FAIL;
            }
            hmac_rrm_send_rm_rpt_action(pst_hmac_vap);
        }
    }
    else
    {
        if (OAL_FALSE == en_local_meas)
        {
            pst_hmac_vap->pst_rrm_info->en_is_measuring = OAL_TRUE;

            pst_hmac_vap->pst_rrm_info->st_meas_status_timer.ul_timeout = MAC_RRM_VAP_MEAS_STAUTS_TIME;

            FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->pst_rrm_info->st_meas_status_timer),
                               hmac_vap_meas_status_timeout,
                               pst_hmac_vap->pst_rrm_info->st_meas_status_timer.ul_timeout,
                               (oal_void *)pst_hmac_vap->pst_rrm_info,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_vap->st_vap_base_info.ul_core_id);
        }
    }

    return ul_ret;
}


OAL_STATIC oal_void hmac_rrm_neighbor_scan_cb(void *p_scan_record)
{
    hmac_scan_record_stru           *pst_scan_record;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    oal_uint32                       ul_ret;
    hmac_vap_stru                   *pst_hmac_vap     = OAL_PTR_NULL;

    /* 判断入参合法性 */
    if (OAL_PTR_NULL == p_scan_record)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_neighbor_scan_cb: input pointer is null!}");
        return;
    }

    pst_scan_record = (hmac_scan_record_stru *)p_scan_record;

    /*获取hmac VAP*/
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_scan_record->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_neighbor_scan_cb: pst_hmac_vap is null!}");
        return;
    }

    /*判断扫描状态是否成功*/
    if (MAC_SCAN_SUCCESS != pst_scan_record->en_scan_rsp_status)
    {
        /*扫描失败回复不带IE的neighbor Report*/
        if ( OAL_SUCC != hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap))
        {
            return;
        }
        hmac_rrm_send_rm_rpt_action(pst_hmac_vap);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RRM, "hmac_rrm_neighbor_scan_cb::scan fail, status[%d].",
                pst_scan_record->en_scan_rsp_status);
        return;
    }

    /*申请RPT管理帧内存*/
    ul_ret = hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap);
    if ( OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_RRM, "{hmac_rrm_neighbor_scan_cb: hmac_rrm_fill_basic_rm_rpt_action ret[%d].}", ul_ret);
        return;
    }

    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_scan_record->st_bss_mgmt);

    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));

    /* 遍历扫描到的bss信息 */
    hmac_rrm_encap_meas_rpt_neighbor(pst_hmac_vap, &(pst_bss_mgmt->st_bss_list_head));

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    /*发帧*/
    hmac_rrm_send_rm_rpt_action(pst_hmac_vap);

    return;
}


OAL_STATIC oal_void hmac_rrm_get_neighbor_info_from_rpt(hmac_user_stru *pst_hmac_user, mac_neighbor_rpt_ie_stru *pst_neighbor_rpt_ie)
{
    mac_user_rrm_info_stru      *pst_rrm_info;
    mac_meas_rpt_neighbor_stru  *pst_meas_rpt_neighbor;
    mac_neighbor_rpt_ie_stru    *pst_meas_rpt_neighbor_item = OAL_PTR_NULL;
    oal_uint32                  *pul_bssid_info;

    pst_rrm_info = pst_hmac_user->pst_user_rrm_info;
    /*************************************************************************/
    /*                   Beacon Report - Frame Body                          */
    /* --------------------------------------------------------------------- */
    /* |oper class|chn num|Actual Meas Start Time|Meas Duration|Rpt Frm Info|*/
    /* --------------------------------------------------------------------- */
    /* |1         |1      |8                     |2            | 1          |*/
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* | RCPI | RSNI | BSSID | Antenna ID | Parent TSF| Optional Subelements|*/
    /* --------------------------------------------------------------------- */
    /* |1     |1     |6      |1           |4          | Var                 |*/
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    pst_meas_rpt_neighbor = (mac_meas_rpt_neighbor_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_meas_rpt_neighbor_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_neighbor)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_get_neighbor_info_from_rpt::pst_meas_rpt_neighbor NULL}");
        return;
    }

    OAL_MEMZERO(pst_meas_rpt_neighbor, OAL_SIZEOF(mac_meas_rpt_bcn_stru));

    pst_meas_rpt_neighbor_item = (mac_neighbor_rpt_ie_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(mac_neighbor_rpt_ie_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_meas_rpt_neighbor_item)
    {
        OAL_MEM_FREE(pst_meas_rpt_neighbor, OAL_TRUE);
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_rrm_get_neighbor_info_from_rpt::pst_meas_rpt_neighbor_item NULL}");
        return;
    }

    pst_meas_rpt_neighbor->pst_meas_rpt_neighbor_item = pst_meas_rpt_neighbor_item;

    /*fill node begin*/
    oal_memcopy(pst_meas_rpt_neighbor->pst_meas_rpt_neighbor_item, pst_neighbor_rpt_ie, OAL_SIZEOF(mac_neighbor_rpt_ie_stru));
    OAM_WARNING_LOG4(0, OAM_SF_RRM, "{hmac_rrm_get_neighbor_info_from_rpt:auc_bssid =%x:ff:ff:%x:%x:%x.",
    pst_meas_rpt_neighbor_item->auc_bssid[0],
    pst_meas_rpt_neighbor_item->auc_bssid[3],
    pst_meas_rpt_neighbor_item->auc_bssid[4],
    pst_meas_rpt_neighbor_item->auc_bssid[5]);

    pul_bssid_info = (oal_uint32*)(&(pst_meas_rpt_neighbor_item->st_bssid_info));
    OAM_WARNING_LOG4(0, OAM_SF_RRM, "{hmac_rrm_get_neighbor_info_from_rpt:st_bssid_info=0x%x, uc_optclass=%d, uc_channum=%d, phy_type=%d.",
        *pul_bssid_info,
        pst_meas_rpt_neighbor_item->uc_optclass,
        pst_meas_rpt_neighbor_item->uc_channum,
        pst_meas_rpt_neighbor_item->uc_phy_type);
    /*fill node end*/

    oal_dlist_add_tail(&(pst_meas_rpt_neighbor->st_dlist_head_neighbor), &(pst_rrm_info->st_meas_rpt_list));
}


OAL_STATIC oal_uint32 hmac_rrm_encap_local_bssid_rpt(hmac_vap_stru *pst_hmac_vap)
{
    mac_neighbor_rpt_ie_stru            *pst_neighbor_rpt_ie;
    mac_vap_stru                        *pst_mac_vap;
    oal_uint32                          ul_ret;

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    /*申请RPT管理帧内存*/
    ul_ret = hmac_rrm_fill_basic_rm_rpt_action(pst_hmac_vap);
    if ( OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_RRM, "{hmac_rrm_encap_local_bssid_rpt: hmac_rrm_fill_basic_rm_rpt_action ret[%d].}", ul_ret);
        return ul_ret;
    }
    pst_neighbor_rpt_ie = (mac_neighbor_rpt_ie_stru *)pst_hmac_vap->pst_rrm_info->pst_rm_rpt_action->auc_rpt_ies;

    /*Neighbor Report IE*/
    pst_neighbor_rpt_ie->uc_eid       = MAC_EID_NEIGHBOR_REPORT;
    oal_memcopy(pst_neighbor_rpt_ie->auc_bssid, pst_mac_vap->auc_bssid, OAL_SIZEOF(pst_mac_vap->auc_bssid));

    /*BSSID Information*/
    pst_neighbor_rpt_ie->st_bssid_info.bit_ap_reachability    = 0; //not support preauth
    pst_neighbor_rpt_ie->st_bssid_info.bit_security           = 0; //not support
    pst_neighbor_rpt_ie->st_bssid_info.bit_key_scope          = 0; //not support key_scope

    pst_neighbor_rpt_ie->st_bssid_info.bit_spec_management    = mac_mib_get_dot11SpectrumManagementRequired(pst_mac_vap);
    pst_neighbor_rpt_ie->st_bssid_info.bit_qos                = mac_mib_get_dot11QosOptionImplemented(pst_mac_vap);
    pst_neighbor_rpt_ie->st_bssid_info.bit_apsd               = mac_mib_get_dot11APSDOptionImplemented(pst_mac_vap);
    pst_neighbor_rpt_ie->st_bssid_info.bit_radio_meas         = mac_mib_get_dot11RadioMeasurementActivated(pst_mac_vap);
    pst_neighbor_rpt_ie->st_bssid_info.bit_delayed_ba         = mac_mib_get_dot11DelayedBlockAckOptionImplemented(pst_mac_vap);
    pst_neighbor_rpt_ie->st_bssid_info.bit_immediate_ba       = 0;

    /*optclass*/
    pst_neighbor_rpt_ie->uc_optclass            = 0;
    pst_neighbor_rpt_ie->uc_channum             = pst_mac_vap->st_channel.uc_chan_number;
    if ( WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
    {
        pst_neighbor_rpt_ie->uc_phy_type = PHY_TYPE_DSSS;
    }
    else
    {
        pst_neighbor_rpt_ie->uc_phy_type = PHY_TYPE_OFDM;
    }

    pst_neighbor_rpt_ie->uc_len                             = 13;
    pst_hmac_vap->pst_rrm_info->us_rm_rpt_action_len        += MAC_IE_HDR_LEN + pst_neighbor_rpt_ie->uc_len;

    /*发帧*/
    hmac_rrm_send_rm_rpt_action(pst_hmac_vap);

    return OAL_SUCC;
}

/*lint -e578*//*lint -e19*/
/*lint +e578*//*lint +e19*/

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


