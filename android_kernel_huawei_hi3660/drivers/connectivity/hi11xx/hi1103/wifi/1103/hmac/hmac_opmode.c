


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_opmode.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_OPMODE_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_uint32 hmac_check_opmode_notify_etc(
                hmac_vap_stru                   *pst_hmac_vap,
                oal_uint8                       *puc_mac_hdr,
                oal_uint8                       *puc_payload_offset,
                oal_uint32                       ul_msg_len,
                hmac_user_stru                  *pst_hmac_user)
{
    oal_uint8              *puc_opmode_notify_ie;
    mac_vap_stru           *pst_mac_vap;
    mac_user_stru          *pst_mac_user;
    mac_opmode_notify_stru *pst_opmode_notify = OAL_PTR_NULL;
//    oal_uint8               uc_mgmt_frm_type;
    oal_uint32              ul_relt;

    /* 入参指针已经在调用函数保证非空，这里直接使用即可 */
    pst_mac_vap   = &(pst_hmac_vap->st_vap_base_info);
    pst_mac_user  = &(pst_hmac_user->st_user_base_info);

    if ((OAL_FALSE == mac_mib_get_VHTOptionImplemented(pst_mac_vap))
        || (OAL_FALSE == mac_mib_get_OperatingModeNotificationImplemented(pst_mac_vap)))
    {
        return OAL_SUCC;
    }

    //puc_opmode_notify_ie = mac_get_opmode_notify_ie(puc_payload, (oal_uint16)ul_msg_len, us_info_elem_offset);
    puc_opmode_notify_ie = mac_find_ie_etc(MAC_EID_OPMODE_NOTIFY, puc_payload_offset, (oal_int32)ul_msg_len);

    /* STA关联在vht下,且vap在双空间流的情况下才解析此ie */
    if ((OAL_PTR_NULL != puc_opmode_notify_ie) && (puc_opmode_notify_ie[1] == MAC_OPMODE_NOTIFY_LEN))
    {
        pst_opmode_notify = (mac_opmode_notify_stru *)(puc_opmode_notify_ie + MAC_IE_HDR_LEN);

        /* SMPS已经解析并更新空间流，因此若空间流不等则SMPS和OPMODE的空间流信息不同 */
        if(pst_mac_user->en_avail_num_spatial_stream > pst_opmode_notify->bit_rx_nss ||
            (WLAN_SINGLE_NSS == pst_mac_user->en_avail_num_spatial_stream && WLAN_SINGLE_NSS != pst_opmode_notify->bit_rx_nss))
        {
            OAM_WARNING_LOG0(0, OAM_SF_OPMODE, "{hmac_check_opmode_notify_etc::SMPS and OPMODE show different nss!}");
            if(WLAN_HT_MODE == pst_mac_user->en_cur_protocol_mode || WLAN_HT_ONLY_MODE == pst_mac_user->en_cur_protocol_mode)
            {
                return OAL_FALSE;
            }
        }

        ul_relt = mac_ie_proc_opmode_field_etc(pst_mac_vap, pst_mac_user, pst_opmode_notify);
        if (OAL_UNLIKELY(OAL_SUCC != ul_relt))
        {
            OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_OPMODE, "{hmac_check_opmode_notify_etc::mac_ie_proc_opmode_field_etc failed[%d].}", ul_relt);
            return ul_relt;
        }
    }
    return OAL_SUCC;
}


oal_uint32  hmac_mgmt_rx_opmode_notify_frame_etc(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf)
{
    mac_opmode_notify_stru     *pst_opmode_notify = OAL_PTR_NULL;
//    oal_uint8                   uc_mgmt_frm_type;
    oal_uint8                  *puc_frame_payload;
    mac_user_stru              *pst_mac_user;
    mac_rx_ctl_stru           *pst_rx_ctrl;
    oal_uint8                  *puc_data;
    oal_uint32                  ul_ret;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user)|| (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG3(0, OAM_SF_OPMODE, "{hmac_mgmt_rx_opmode_notify_frame_etc::pst_hmac_vap = [%x],pst_hmac_user = [%x], pst_netbuf = [%x]!}\r\n",
                        pst_hmac_vap, pst_hmac_user, pst_netbuf);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if ((OAL_FALSE == mac_mib_get_VHTOptionImplemented(&pst_hmac_vap->st_vap_base_info))
        || (OAL_FALSE == mac_mib_get_OperatingModeNotificationImplemented(&pst_hmac_vap->st_vap_base_info)))
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_OPMODE, "{hmac_mgmt_rx_opmode_notify_frame_etc::the vap is not support OperatingModeNotification!}\r\n");
        return OAL_SUCC;
    }

    pst_rx_ctrl = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    pst_mac_user = &(pst_hmac_user->st_user_base_info);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_OPMODE, "{hmac_mgmt_rx_opmode_notify_frame_etc::pst_mac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取帧体指针 */
    puc_data = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl);

    /****************************************************/
    /*   OperatingModeNotification Frame - Frame Body   */
    /* -------------------------------------------------*/
    /* |   Category   |   Action   |   OperatingMode   |*/
    /* -------------------------------------------------*/
    /* |   1          |   1        |   1               |*/
    /* -------------------------------------------------*/
    /*                                                  */
    /****************************************************/

    /* 获取payload的指针 */
    puc_frame_payload = (oal_uint8 *)puc_data + MAC_80211_FRAME_LEN;
    pst_opmode_notify = (mac_opmode_notify_stru *)(puc_frame_payload + MAC_ACTION_OFFSET_ACTION + 1);

    ul_ret = mac_ie_proc_opmode_field_etc(&(pst_hmac_vap->st_vap_base_info), pst_mac_user, pst_opmode_notify);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_OPMODE, "{hmac_mgmt_rx_opmode_notify_frame_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

