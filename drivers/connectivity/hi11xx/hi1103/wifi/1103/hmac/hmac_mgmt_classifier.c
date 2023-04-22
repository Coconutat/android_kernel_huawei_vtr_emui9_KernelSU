


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_rx_data.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_mgmt_classifier.h"
#include "hmac_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_mgmt_sta.h"
#include "hmac_mgmt_ap.h"

#ifdef _PRE_WLAN_CHIP_TEST
#include "dmac_test_main.h"
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_1103_CHR
#include "hmac_dfx.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_MGMT_CLASSIFIER_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) && !defined(_PRE_PC_LINT) && !defined(WIN32))
OAL_STATIC oal_uint8 g_ucLinklossLogSwitch = 0;
#endif

/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_uint32  hmac_mgmt_tx_action_etc(
                hmac_vap_stru              *pst_hmac_vap,
                hmac_user_stru             *pst_hmac_user,
                mac_action_mgmt_args_stru  *pst_action_args)
{
    if ((OAL_PTR_NULL == pst_hmac_vap)
     || (OAL_PTR_NULL == pst_hmac_user)
     || (OAL_PTR_NULL == pst_action_args))
    {
        OAM_ERROR_LOG3(0, OAM_SF_TX, "{hmac_mgmt_tx_action_etc::param null, %d %d %d.}", pst_hmac_vap, pst_hmac_user, pst_action_args);
        return OAL_ERR_CODE_PTR_NULL;
    }

    switch (pst_action_args->uc_category)
    {
        case MAC_ACTION_CATEGORY_BA:

            switch (pst_action_args->uc_action)
            {
                case MAC_BA_ACTION_ADDBA_REQ:
                    OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_mgmt_tx_action_etc::MAC_BA_ACTION_ADDBA_REQ.}");
                    hmac_mgmt_tx_addba_req_etc(pst_hmac_vap, pst_hmac_user, pst_action_args);
                    break;

                #if 0 /* 可以不使用该接口，直接调用 */
                case BA_ACTION_ADDBA_RSP:
                    hmac_mgmt_tx_addba_rsp_etc(hmac_vap_stru * pst_hmac_vap, hmac_user_stru * pst_hmac_user, mac_action_mgmt_args_stru * pst_action_args)
                    break;
                #endif

                case MAC_BA_ACTION_DELBA:
                    OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_mgmt_tx_action_etc::MAC_BA_ACTION_DELBA.}");
                    hmac_mgmt_tx_delba_etc(pst_hmac_vap, pst_hmac_user, pst_action_args);
                    break;

                default:
                    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_mgmt_tx_action_etc::invalid ba type[%d].}", pst_action_args->uc_action);
                    return OAL_FAIL;    /* 错误类型待修改 */
            }
            break;

        default:
            OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_mgmt_tx_action_etc::invalid ba type[%d].}", pst_action_args->uc_category);
            break;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_mgmt_tx_priv_req_etc(
                hmac_vap_stru              *pst_hmac_vap,
                hmac_user_stru             *pst_hmac_user,
                mac_priv_req_args_stru     *pst_priv_req)
{
    mac_priv_req_11n_enum_uint8  en_req_type;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == pst_priv_req))
    {
        OAM_ERROR_LOG3(0, OAM_SF_TX, "{hmac_mgmt_tx_priv_req_etc::param null, %d %d %d.}", pst_hmac_vap, pst_hmac_user, pst_priv_req);
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_req_type = pst_priv_req->uc_type;

    switch (en_req_type)
    {
        case MAC_A_MPDU_START:

            hmac_mgmt_tx_ampdu_start_etc(pst_hmac_vap, pst_hmac_user, pst_priv_req);
            break;

        case MAC_A_MPDU_END:
            hmac_mgmt_tx_ampdu_end_etc(pst_hmac_vap, pst_hmac_user, pst_priv_req);
            break;

       default:

            OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_mgmt_tx_priv_req_etc::invalid en_req_type[%d].}", en_req_type);
            break;
    };

    return OAL_SUCC;
}

#if 0

OAL_STATIC oal_uint32  hmac_mgmt_rx_action(
                hmac_vap_stru          *pst_vap,
                oal_netbuf_stru        *pst_mgmt_netbuf)
{
    oal_uint8                          *puc_frame_payload;
    mac_ieee80211_frame_stru           *pst_frame_hdr;          /* 保存mac帧的指针 */
    hmac_rx_ctl_stru                   *pst_rx_ctrl;
    hmac_user_stru                     *pst_hmac_user;
    oal_uint32                          ul_rslt;
    oal_uint16                          us_user_idx;

    if ((OAL_PTR_NULL == pst_vap) || (OAL_PTR_NULL == pst_mgmt_netbuf))
    {
        OAM_ERROR_LOG2(0, OAM_SF_RX, "{hmac_mgmt_rx_action::param null, %d %d.}", pst_vap, pst_mgmt_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取该MPDU的控制信息 */
    pst_rx_ctrl = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_mgmt_netbuf);

    /* 获取帧头信息 */
    pst_frame_hdr = (mac_ieee80211_frame_stru *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

    /* 获取发送端的用户指针 */
    ul_rslt = mac_vap_find_user_by_macaddr_etc(&pst_vap->st_vap_base_info, pst_frame_hdr->auc_address2, &us_user_idx);

    if (OAL_SUCC != ul_rslt)
    {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_mgmt_rx_action::mac_vap_find_user_by_macaddr_etc failed[%d].}", ul_rslt);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);

    /* 获取payload的指针 */
    puc_frame_payload = (oal_uint8 *)pst_frame_hdr + MAC_80211_FRAME_LEN;

    /* 不同ACTION帧的分发入口 */
    switch(puc_frame_payload[MAC_ACTION_OFFSET_CATEGORY])
    {
        case MAC_ACTION_CATEGORY_BA:
        {
            /* BA相关的ACTION帧的分发 */
            switch(puc_frame_payload[MAC_ACTION_OFFSET_ACTION])
            {
                case MAC_BA_ACTION_ADDBA_REQ:
                    hmac_mgmt_rx_addba_req_etc(pst_vap, pst_hmac_user, puc_frame_payload);
                    break;

                case MAC_BA_ACTION_ADDBA_RSP:
                    hmac_mgmt_rx_addba_rsp_etc(pst_vap, pst_hmac_user, puc_frame_payload);
                    break;

                case MAC_BA_ACTION_DELBA:
                    hmac_mgmt_rx_delba_etc(pst_vap, pst_hmac_user, puc_frame_payload);
                    break;

                default:
                    break;
            }
        }

        break;

        default:
            OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{hmac_mgmt_rx_action::invalid action type.}");
            break;

    }

    return OAL_SUCC;
}
#endif

oal_uint32  hmac_mgmt_rx_delba_event_etc(frw_event_mem_stru *pst_event_mem)
{

    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    dmac_ctx_action_event_stru         *pst_delba_event;
    oal_uint8                          *puc_da;                                 /* 保存用户目的地址的指针 */
    hmac_vap_stru                      *pst_vap;                                /* vap指针 */
    hmac_user_stru                     *pst_hmac_user;
    mac_action_mgmt_args_stru           st_action_args;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_mgmt_rx_delba_event_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event           = frw_get_event_stru(pst_event_mem);
    pst_event_hdr       = &(pst_event->st_event_hdr);
    pst_delba_event     = (dmac_ctx_action_event_stru *)(pst_event->auc_event_data);

    /* 获取vap结构信息 */
    pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_delba_event_etc::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取目的用户的MAC ADDR */
    puc_da = pst_delba_event->auc_mac_addr;

    /* 获取发送端的用户指针 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(&pst_vap->st_vap_base_info, puc_da);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_delba_event_etc::mac_vap_find_user_by_macaddr_etc failed.}");
        return OAL_FAIL;
    }

    st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
    st_action_args.uc_action   = MAC_BA_ACTION_DELBA;
    st_action_args.ul_arg1     = pst_delba_event->uc_tidno;     /* 该数据帧对应的TID号 */
    st_action_args.ul_arg2     = pst_delba_event->uc_initiator; /* DELBA中，触发删除BA会话的发起端 */
    st_action_args.ul_arg3     = pst_delba_event->uc_status;    /* DELBA中代表删除reason */
    st_action_args.puc_arg5    = puc_da;                        /* DELBA中代表目的地址 */

    hmac_mgmt_tx_action_etc(pst_vap, pst_hmac_user, &st_action_args);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT

#define IE_LEN_WITH_HEADER(ptr) (2+*(ptr+1))
#define IE_GET_LEN(ptr) (*(oal_uint8*)(ptr+1))
#define IE_GET_BODY(ptr) (ptr+2)

oal_uint8 *hmac_find_hi_ssid_ie(const oal_uint8 *ies, const oal_uint16 ie_len, const oal_uint8 tag_id)
{
    oal_uint8 *ptr = (oal_uint8 *)ies;
    while(ptr < ies+ie_len)
    {
        if (*ptr == tag_id)
        {
            return ptr;
        }
        ptr += IE_LEN_WITH_HEADER(ptr);
    }
    return NULL;
}

oal_uint32 hmac_is_beacon_with_hi_ssid(const void *data, const oal_uint16 len)
{
#define IEEE80211_ELEMID_SSID       0
#define HILINK_MCAST_MASK "Hi"
#define HILINK_MCAST_LEN 32
//#define HILINK_MCAST_MAX_LEN 29
/* 24(hdr): fc(2) + dur(2) + addr(6*3) + seq(2)*/
#define IEEE80211_HEAD_LEN    24
/* 12: timestamp(8) + interval(2) + capability (2) */
#define HI_SSID_OFFSET_BYTES  (IEEE80211_HEAD_LEN+12)

    oal_uint8 *ies = NULL;
    oal_uint8 *ssid_ie = NULL;
    ies = (oal_uint8 *)data + HI_SSID_OFFSET_BYTES;
    ssid_ie = hmac_find_hi_ssid_ie(ies, len - IEEE80211_HEAD_LEN, IEEE80211_ELEMID_SSID);
    if (NULL == ssid_ie){
        return OAL_FALSE;
    }

    if (IE_GET_LEN(ssid_ie) != HILINK_MCAST_LEN){
        return OAL_FALSE;
    }

    if (oal_memcmp(IE_GET_BODY(ssid_ie), HILINK_MCAST_MASK, sizeof(HILINK_MCAST_MASK)-1) != 0)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}

void hmac_rx_mgmt_frame_notify(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf,
     oal_uint16 us_subtype)
{
    oal_int32 filter_flag =0;
    char *tag=NULL;
    oal_uint16 us_mgmt_frame_filters;

    us_mgmt_frame_filters = pst_hmac_vap->st_vap_base_info.us_mgmt_frame_filters;
    switch (us_subtype)
    {
    case WLAN_BEACON:
        if (hmac_is_beacon_with_hi_ssid(pst_netbuf->data, (oal_uint16)pst_netbuf->len))
        {
            filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_BEACON;
            tag = "Manage.beacon";
        }
        break;
    case WLAN_PROBE_REQ:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_PROBE_REQ;
        tag = "Manage.prob_req";
        break;
    case WLAN_PROBE_RSP:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_PROBE_RESP;
        tag = "Manage.prob_resp";
        break;
    case WLAN_ASSOC_REQ:
    case WLAN_REASSOC_REQ:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_ASSOC_REQ;
        tag = "Manage.assoc_req";
        break;
    case WLAN_ASSOC_RSP:
    case WLAN_REASSOC_RSP:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_ASSOC_RESP;
        tag = "Manage.assoc_resp";
        break;
    case WLAN_AUTH:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_AUTH;
        tag = "Manage.auth";
        break;
    case WLAN_DEAUTH:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_DEAUTH;
        tag = "Manage.deauth";
        break;
    case WLAN_DISASOC:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_DISASSOC;
        tag = "Manage.disassoc";
        break;
    case WLAN_ACTION:
        filter_flag = us_mgmt_frame_filters & IEEE80211_FILTER_TYPE_ACTION;
        tag = "Manage.action";
        break;
    default:
        break;
    }
    if (filter_flag != 0)
    {
#define MGMT_FRAM_TAG_SIZE 30 /* hardcoded in atheros_wireless_event_wireless_custom */
#ifndef IW_GENERIC_IE_MAX
#define IW_GENERIC_IE_MAX   1024
#endif
        const oal_uint32 buf_max = IW_GENERIC_IE_MAX;
        char *buf;
        oal_iwreq_data_union wrqu;
        oal_uint32 buf_size = MGMT_FRAM_TAG_SIZE + pst_netbuf->len;
        oal_net_device_stru        *pst_net_device;
        dmac_rx_ctl_stru           *pst_rx_ctrl;
        oal_int32                   i_rssi_temp;
        /* 获取net_device*/
        pst_net_device = pst_hmac_vap->pst_net_device;
        if (OAL_PTR_NULL == pst_net_device)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK,
                            "{hmac_rx_mgmt_frame_notify::get net device ptr is null!}\r\n");
            return ;
        }

        if(buf_size > buf_max)
        {
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK,
                            "{hmac_rx_mgmt_frame_notify::Event length more than expected!}\r\n");
            return;
        }
        buf = (char *)oal_memalloc(buf_size);
        if (buf == NULL)
        {
            return;
        }
        OAL_MEMZERO(buf, buf_size);
        OAL_MEMZERO(&wrqu, sizeof(wrqu));
        wrqu.data.length = (oal_uint16)buf_size;
        OAL_SPRINTF(buf, buf_size, "%s %d", tag, pst_netbuf->len);
        oal_memcopy(buf+MGMT_FRAM_TAG_SIZE, pst_netbuf->data, pst_netbuf->len);
        /* 获取帧头信息 */
        pst_rx_ctrl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
        i_rssi_temp = pst_rx_ctrl->st_rx_statistic.c_rssi_dbm + HMAC_FBT_RSSI_ADJUST_VALUE;
        if (i_rssi_temp < 0)
        {
            i_rssi_temp = 0;
        }
        if (i_rssi_temp > HMAC_FBT_RSSI_MAX_VALUE)
        {
            i_rssi_temp = HMAC_FBT_RSSI_MAX_VALUE;
        }
        *(oal_int32 *)(buf + MGMT_FRAM_TAG_SIZE - sizeof(oal_int32)) = i_rssi_temp;
#ifndef WIN32
        wireless_send_event(pst_net_device, IWEVASSOCREQIE, &wrqu, buf);
#endif
        oal_free(buf);
#undef  MGMT_FRAM_TAG_SIZE
    }
}

#endif


oal_uint32  hmac_rx_process_mgmt_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    dmac_wlan_crx_event_stru           *pst_crx_event;
    oal_netbuf_stru                    *pst_netbuf;                     /* 用于保存管理帧指向的NETBUF */
    hmac_vap_stru                      *pst_vap;                        /* vap指针 */
    oal_uint32                          ul_ret;
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    dmac_rx_ctl_stru                   *pst_rx_ctl;
#endif
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RX, "{hmac_rx_process_mgmt_event_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event           = frw_get_event_stru(pst_event_mem);
    pst_event_hdr       = &(pst_event->st_event_hdr);
    pst_crx_event       = (dmac_wlan_crx_event_stru *)(pst_event->auc_event_data);
    pst_netbuf     = pst_crx_event->pst_netbuf;

    /* 获取vap结构信息 */
    pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_BA, "{hmac_mgmt_rx_delba_event_etc::pst_vap null.}");
        /* 将管理帧的空间归还内存池 */
        //oal_netbuf_free(pst_netbuf); /* 返回不成功，最外层会统一释放，这里不需要释放 */
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    /* 获取帧头信息 */
    pst_rx_ctl    = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    if (pst_rx_ctl->st_rx_info.bit_mgmt_to_hostapd)
    {
        mac_ieee80211_frame_stru          *pst_frame_hdr;
        pst_frame_hdr = (mac_ieee80211_frame_stru *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));
        hmac_rx_mgmt_frame_notify(pst_vap, pst_netbuf, pst_frame_hdr->st_frame_control.bit_sub_type);
        return OAL_SUCC;
    }
#endif

    /* 接收管理帧是状态机的一个输入，调用状态机接口 */
    if (WLAN_VAP_MODE_BSS_AP == pst_vap->st_vap_base_info.en_vap_mode)
    {
        ul_ret = hmac_fsm_call_func_ap_etc(pst_vap, HMAC_FSM_INPUT_RX_MGMT, pst_crx_event);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0,OAM_SF_BA,"{hmac_rx_process_mgmt_event_etc::hmac_fsm_call_func_ap_etc fail.err code1 [%u]}",ul_ret);
        }
    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_vap->st_vap_base_info.en_vap_mode)
    {
        ul_ret = hmac_fsm_call_func_sta_etc(pst_vap, HMAC_FSM_INPUT_RX_MGMT, pst_crx_event);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0,OAM_SF_BA,"{hmac_rx_process_mgmt_event_etc::hmac_fsm_call_func_ap_etc fail.err code2 [%u]}",ul_ret);
        }
    }

    /* 管理帧统一释放接口 */
    oal_netbuf_free(pst_netbuf);

    return OAL_SUCC;
}


oal_uint32  hmac_mgmt_tbtt_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    hmac_vap_stru                      *pst_hmac_vap;
    hmac_misc_input_stru                st_misc_input;
    oal_uint32                          ul_ret;

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_mgmt_tbtt_event_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(&st_misc_input, OAL_SIZEOF(hmac_misc_input_stru));

    /* 获取事件头和事件结构体指针 */
    pst_event      = frw_get_event_stru(pst_event_mem);
    pst_event_hdr  = &(pst_event->st_event_hdr);

    pst_hmac_vap   = mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{hmac_mgmt_tbtt_event_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_misc_input.en_type = HMAC_MISC_TBTT;

    /* 调用sta状态机，只有sta的tbtt事件上报到hmac */
    ul_ret = hmac_fsm_call_func_sta_etc(pst_hmac_vap, HMAC_FSM_INPUT_MISC, &st_misc_input);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "{hmac_mgmt_tbtt_event_etc::hmac_fsm_call_func_sta_etc fail. erro code is %u}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_mgmt_send_disasoc_deauth_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    frw_event_hdr_stru                 *pst_event_hdr;
    dmac_diasoc_deauth_event           *pst_disasoc_deauth_event;
    oal_uint8                          *puc_da;                                 /* 保存用户目的地址的指针 */
    hmac_vap_stru                      *pst_vap;                                /* vap指针 */
    hmac_user_stru                     *pst_hmac_user = OAL_PTR_NULL;
    oal_uint32                          ul_rslt;
    oal_uint16                          us_user_idx;
    oal_uint8                           uc_event = 0;
    mac_vap_stru                       *pst_mac_vap = OAL_PTR_NULL;
    oal_uint16                          us_err_code = 0;
#ifdef _PRE_WLAN_FEATURE_P2P
    mac_vap_stru                       *pst_up_vap1;
    mac_vap_stru                       *pst_up_vap2;
    mac_device_stru                    *pst_mac_device;
#endif

    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_mgmt_send_disasoc_deauth_event_etc::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件头和事件结构体指针 */
    pst_event           = frw_get_event_stru(pst_event_mem);
    pst_event_hdr       = &(pst_event->st_event_hdr);
    pst_disasoc_deauth_event     = (dmac_diasoc_deauth_event *)(pst_event->auc_event_data);

    /* 获取vap结构信息 */
    pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event_hdr->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_send_disasoc_deauth_event_etc::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &pst_vap->st_vap_base_info;

    /* 获取目的用户的MAC ADDR */
    puc_da      = pst_disasoc_deauth_event->auc_des_addr;
    uc_event    = pst_disasoc_deauth_event->uc_event;
    us_err_code = pst_disasoc_deauth_event->uc_reason;

    /* 发送去认证, 未关联状态收到第三类帧 */
    if(DMAC_WLAN_CRX_EVENT_SUB_TYPE_DEAUTH == uc_event)
    {
        hmac_mgmt_send_deauth_frame_etc(pst_mac_vap,
                                    puc_da,
                                    us_err_code,
                                    OAL_FALSE); // 非PMF

#ifdef _PRE_WLAN_FEATURE_P2P
        pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

        /* 判断是异频DBAC模式时，无法判断是哪个信道收到的数据帧，两个信道都需要发去认证 */
        ul_rslt = mac_device_find_2up_vap_etc(pst_mac_device, &pst_up_vap1, &pst_up_vap2);
        if (OAL_SUCC != ul_rslt)
        {
            return OAL_SUCC;
        }

        if (pst_up_vap1->st_channel.uc_chan_number == pst_up_vap2->st_channel.uc_chan_number)
        {
            return OAL_SUCC;
        }

        /* 获取另一个VAP */
        if (pst_mac_vap->uc_vap_id != pst_up_vap1->uc_vap_id)
        {
            pst_up_vap2 = pst_up_vap1;
        }

        /* 另外一个VAP也发去认证帧。error code加上特殊标记，组去认证帧时要修改源地址 */
        hmac_mgmt_send_deauth_frame_etc(pst_up_vap2,
                                    puc_da,
                                    us_err_code | MAC_SEND_TWO_DEAUTH_FLAG,
                                    OAL_FALSE);
#endif

        return OAL_SUCC;
    }

    /* 获取发送端的用户指针 */
    ul_rslt = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_da, &us_user_idx);
    if (ul_rslt != OAL_SUCC)
    {

        OAM_WARNING_LOG4(0, OAM_SF_RX, "{hmac_mgmt_send_disasoc_deauth_event_etc::Hmac cannot find USER by addr[%02X:XX:XX:%02X:%02X:%02X], just del DMAC user}",
                  puc_da[0],
                  puc_da[3],
                  puc_da[4],
                  puc_da[5]);

        /* 找不到用户，说明用户已经删除，直接返回成功，不需要再抛事件到dmac删除用户(统一由hmac_user_del来管理删除用户) */
        return OAL_SUCC;
    }

    /* 获取到hmac user,使用protected标志 */
    pst_hmac_user = mac_res_get_hmac_user_etc(us_user_idx);

    hmac_mgmt_send_disassoc_frame_etc(pst_mac_vap, puc_da, us_err_code, ((OAL_PTR_NULL==pst_hmac_user) ? OAL_FALSE : pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active));

    if (OAL_PTR_NULL != pst_hmac_user)
    {
        hmac_handle_disconnect_rsp_etc(pst_vap, pst_hmac_user, us_err_code);
    }

    /* 删除用户 */
    hmac_user_del_etc(pst_mac_vap, pst_hmac_user);

    return OAL_SUCC;
}

OAL_STATIC mac_reason_code_enum_uint16 hmac_disassoc_reason_exchange(dmac_disasoc_misc_reason_enum_uint16  en_driver_disasoc_reason)
{
    switch (en_driver_disasoc_reason)
    {
        case DMAC_DISASOC_MISC_LINKLOSS:
        case DMAC_DISASOC_MISC_KEEPALIVE:
        case DMAC_DISASOC_MISC_GET_CHANNEL_IDX_FAIL:
            return  MAC_DEAUTH_LV_SS;

        case DMAC_DISASOC_MISC_CHANNEL_MISMATCH:
            return MAC_UNSPEC_REASON;
        default:
            break;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ASSOC, "{hmac_disassoc_reason_exchange::Unkown driver_disasoc_reason[%d].}", en_driver_disasoc_reason);

    return MAC_UNSPEC_REASON;
}


oal_uint32  hmac_proc_disasoc_misc_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru          *pst_event;
    hmac_vap_stru           *pst_hmac_vap;
    hmac_user_stru          *pst_hmac_user;
    oal_bool_enum_uint8      en_is_protected = OAL_FALSE;  /* PMF */
    dmac_disasoc_misc_stru  *pdmac_disasoc_misc_stru;
    mac_reason_code_enum_uint16 en_disasoc_reason_code = MAC_UNSPEC_REASON;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_proc_disasoc_misc_event_etc::pst_event_mem is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    pdmac_disasoc_misc_stru = (dmac_disasoc_misc_stru*)pst_event->auc_event_data;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ASSOC, "{hmac_proc_disasoc_misc_event_etc::pst_hmac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_proc_disasoc_misc_event_etc::Device noticed to dissasoc user[%d] within reason[%d]!}",
                    pdmac_disasoc_misc_stru->us_user_idx,
                    pdmac_disasoc_misc_stru->en_disasoc_reason);

#ifdef _PRE_WLAN_1103_CHR
    hmac_chr_set_disasoc_reason(pdmac_disasoc_misc_stru->us_user_idx, pdmac_disasoc_misc_stru->en_disasoc_reason);
#endif

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) && !defined(_PRE_PC_LINT) && !defined(WIN32))
    /* 较强信号(大于-65dBm)下出现link loss则通过Bcpu输出全部寄存器，同时该维测默认不生效*/
    /* 因为一旦出错需要重启手机才能正常使用，使用时需要手动修改g_ucLinklossLogSwitch=1，并编译版本 */
    if (g_ucLinklossLogSwitch && (DMAC_DISASOC_MISC_LINKLOSS == pdmac_disasoc_misc_stru->en_disasoc_reason)&& (pst_hmac_vap->station_info.signal > -65))
    {
        wifi_open_bcpu_set_etc(1);

#ifdef PLATFORM_DEBUG_ENABLE
        debug_uart_read_wifi_mem_etc(OAL_TRUE);
#endif
    }
#endif

    #ifdef _PRE_WLAN_CHIP_TEST
    /* CCA测试时，需要保持STA不去关联, 如果有打开chip_test宏开关 */
    if(0 != g_st_dmac_test_mng.uc_chip_test_open)
    {
        /* 如果有打开cca测试 */
        if (1 == g_st_dmac_test_mng.uc_cca_flag)
        {
        }
    }
    #endif


    if (WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        pst_hmac_user = mac_res_get_hmac_user_etc(pdmac_disasoc_misc_stru->us_user_idx);
        if (OAL_PTR_NULL == pst_hmac_user)
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_proc_disasoc_misc_event_etc::pst_hmac_user[%d] is null.}",
                pdmac_disasoc_misc_stru->us_user_idx);
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_is_protected = pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active;
#ifdef _PRE_WLAN_1103_CHR
        CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_SOFTAP_DISCONNECT, pdmac_disasoc_misc_stru->en_disasoc_reason);
#endif
        /* 抛事件上报内核，已经去关联某个STA */
        hmac_handle_disconnect_rsp_ap_etc(pst_hmac_vap, pst_hmac_user);

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
        if(DMAC_DISASOC_MISC_BSD == pdmac_disasoc_misc_stru->en_disasoc_reason)
        {
            en_disasoc_reason_code = MAC_INACTIVITY; //MAC_POOR_CHANNEL;  //此reason code还有待测试
        }
        else
#else
        {
            en_disasoc_reason_code = MAC_ASOC_NOT_AUTH;
        }
#endif
        /* 发去关联帧 */
        hmac_mgmt_send_disassoc_frame_etc(&pst_hmac_vap->st_vap_base_info, pst_hmac_user->st_user_base_info.auc_user_mac_addr, en_disasoc_reason_code, en_is_protected);
        /* 删除用户 */
        hmac_user_del_etc(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);

    }
    else
    {

        /* 获用户 */
        pst_hmac_user = mac_res_get_hmac_user_etc(pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_user)
        {
            /* 和ap侧一样，上层已经删除了的话，属于正常 */
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_proc_disasoc_misc_event_etc::pst_hmac_user[%d] is null.}",
                pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_is_protected = pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active;


        /* 上报断链类型的转化 */
        en_disasoc_reason_code = hmac_disassoc_reason_exchange(pdmac_disasoc_misc_stru->en_disasoc_reason);

        if (pdmac_disasoc_misc_stru->en_disasoc_reason != DMAC_DISASOC_MISC_CHANNEL_MISMATCH)
        {
            /* 发送去认证帧到AP */
            hmac_mgmt_send_disassoc_frame_etc(&pst_hmac_vap->st_vap_base_info, pst_hmac_user->st_user_base_info.auc_user_mac_addr, en_disasoc_reason_code, en_is_protected);
        }

        /* 删除对应用户 */
        hmac_user_del_etc(&pst_hmac_vap->st_vap_base_info, pst_hmac_user);
        hmac_sta_handle_disassoc_rsp_etc(pst_hmac_vap, en_disasoc_reason_code);
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_ROAM

oal_uint32  hmac_proc_roam_trigger_event_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru          *pst_event;
    hmac_vap_stru           *pst_hmac_vap;
    oal_int8                 c_rssi;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_proc_roam_trigger_event_etc::pst_event_mem is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event    = frw_get_event_stru(pst_event_mem);
    c_rssi       = *(oal_int8 *)pst_event->auc_event_data;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_proc_roam_trigger_event_etc::pst_hmac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_roam_trigger_handle_etc(pst_hmac_vap, c_rssi, OAL_FALSE);

    return OAL_SUCC;
}
#endif //_PRE_WLAN_FEATURE_ROAM

/*lint -e19*/
oal_module_symbol(hmac_mgmt_tx_priv_req_etc);
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

