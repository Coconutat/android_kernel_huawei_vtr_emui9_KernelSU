


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "mac_resource.h"
#include "hmac_encap_frame.h"
#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ENCAP_FRAME_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_uint16 hmac_encap_sa_query_req_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_data, oal_uint8 *puc_da,oal_uint16 us_trans_id)
{
    oal_uint16 us_len = 0;

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
    /* All the fields of the Frame Control Field are set to zero. Only the   */
    /* Type/Subtype field is set.                                            */
    mac_hdr_set_frame_control(puc_data, WLAN_FC0_SUBTYPE_ACTION);
    /*  Set DA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address1, puc_da);
    /*  Set SA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address2,  mac_mib_get_StationID(pst_mac_vap));
    /*  Set SSID  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address3, pst_mac_vap->auc_bssid);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                  SA Query Frame - Frame Body                          */
    /* --------------------------------------------------------------------- */
    /* |   Category   |SA Query Action |  Transaction Identifier           | */
    /* --------------------------------------------------------------------- */
    /* |1             |1               |2 Byte                             | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
     puc_data[MAC_80211_FRAME_LEN] = MAC_ACTION_CATEGORY_SA_QUERY;
     puc_data[MAC_80211_FRAME_LEN+1] = MAC_SA_QUERY_ACTION_REQUEST;
     puc_data[MAC_80211_FRAME_LEN+2] = (us_trans_id & 0x00FF);
     puc_data[MAC_80211_FRAME_LEN+3] = (us_trans_id & 0xFF00) >> 8;

     us_len = MAC_80211_FRAME_LEN + MAC_SA_QUERY_LEN;
     return us_len;
}


oal_uint16 hmac_encap_sa_query_rsp_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *pst_hdr, oal_uint8 *puc_data)
{
    oal_uint16 us_len = 0;

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
    /* All the fields of the Frame Control Field are set to zero. Only the   */
    /* Type/Subtype field is set.                                            */
    mac_hdr_set_frame_control(puc_data, WLAN_FC0_SUBTYPE_ACTION);
    /* Set DA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address1, ((mac_ieee80211_frame_stru *)pst_hdr)->auc_address2);
    /*  Set SA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address2,  mac_mib_get_StationID(pst_mac_vap));
    /*  Set SSID  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address3, pst_mac_vap->auc_bssid);
    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                  SA Query Frame - Frame Body                          */
    /* --------------------------------------------------------------------- */
    /* |   Category   |SA Query Action |  Transaction Identifier           | */
    /* --------------------------------------------------------------------- */
    /* |1             |1               |2 Byte                             | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    puc_data[MAC_80211_FRAME_LEN] = pst_hdr[MAC_80211_FRAME_LEN];
    puc_data[MAC_80211_FRAME_LEN+1] = MAC_SA_QUERY_ACTION_RESPONSE;
    puc_data[MAC_80211_FRAME_LEN+2] = pst_hdr[MAC_80211_FRAME_LEN+2];
    puc_data[MAC_80211_FRAME_LEN+3] = pst_hdr[MAC_80211_FRAME_LEN+3];

    us_len = MAC_80211_FRAME_LEN + MAC_SA_QUERY_LEN;
    return us_len;
}


oal_uint16  hmac_mgmt_encap_deauth_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_data, oal_uint8 *puc_da, oal_uint16 us_err_code)
{
    oal_uint16          us_deauth_len = 0;
#ifdef _PRE_WLAN_FEATURE_P2P
    mac_device_stru    *pst_mac_device;
    mac_vap_stru       *pst_up_vap1;
    mac_vap_stru       *pst_up_vap2;
    oal_uint32          ul_ret;
#endif

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

    /* All the fields of the Frame Control Field are set to zero. Only the   */
    /* Type/Subtype field is set.                                            */
    mac_hdr_set_frame_control(puc_data, WLAN_FC0_SUBTYPE_DEAUTH);

    /* Set DA to address of unauthenticated STA */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address1, puc_da);

#ifdef _PRE_WLAN_FEATURE_P2P
    if (us_err_code & MAC_SEND_TWO_DEAUTH_FLAG)
    {
        us_err_code = us_err_code & ~MAC_SEND_TWO_DEAUTH_FLAG;

        pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
        {
            us_deauth_len = 0;
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_mgmt_encap_deauth_etc::pst_mac_device[%d] null!}", pst_mac_vap->uc_device_id);
            return us_deauth_len;
        }

        ul_ret = mac_device_find_2up_vap_etc(pst_mac_device, &pst_up_vap1, &pst_up_vap2);
        if (OAL_SUCC == ul_ret)
        {
            /* 获取另外一个VAP，组帧时修改地址2为另外1个VAP的MAC地址 */
            if (pst_mac_vap->uc_vap_id != pst_up_vap1->uc_vap_id)
            {
                pst_up_vap2 = pst_up_vap1;
            }

            if (OAL_PTR_NULL == pst_up_vap2->pst_mib_info)
            {
                us_deauth_len = 0;
                OAM_ERROR_LOG0(pst_up_vap2->uc_vap_id, OAM_SF_AUTH, "hmac_mgmt_encap_deauth_etc: pst_up_vap2 mib ptr null.");
                return us_deauth_len;
            }
            oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address2, mac_mib_get_StationID(pst_up_vap2));
            oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address3, pst_up_vap2->auc_bssid);
        }

        OAM_WARNING_LOG1(0, OAM_SF_AUTH, "hmac_mgmt_encap_deauth_etc: send the second deauth frame. error code:%d", us_err_code);
    }
    else
#endif
    {
        if (OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
        {
            us_deauth_len = 0;
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_AUTH, "hmac_mgmt_encap_deauth_etc: pst_mac_vap mib ptr null.");
            return us_deauth_len;
        }

        /* SA is the dot11MACAddress */
        oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address2, mac_mib_get_StationID(pst_mac_vap));
        oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address3, pst_mac_vap->auc_bssid);
    }
    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                  Deauthentication Frame - Frame Body                  */
    /* --------------------------------------------------------------------- */
    /* |                           Reason Code                             | */
    /* --------------------------------------------------------------------- */
    /* |2 Byte                                                             | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* Set Reason Code to 'Class2 error' */
    puc_data[MAC_80211_FRAME_LEN]     = (us_err_code & 0x00FF);
    puc_data[MAC_80211_FRAME_LEN + 1] = (us_err_code & 0xFF00) >> 8;

    us_deauth_len = MAC_80211_FRAME_LEN + WLAN_REASON_CODE_LEN;

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_TX, "{hmac_mgmt_encap_deauth_etc::encap deauth!}\r\n");
#endif

    return us_deauth_len;
}


oal_uint16  hmac_mgmt_encap_disassoc_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_data, oal_uint8 *puc_da, oal_uint16 us_err_code)
{
    oal_uint16 us_disassoc_len = 0;

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
    /*                            设置帧头                                   */
    /*************************************************************************/

    /* 设置subtype   */
    mac_hdr_set_frame_control(puc_data, WLAN_FC0_SUBTYPE_DISASSOC);

    if (OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        us_disassoc_len = 0;
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "hmac_mgmt_encap_disassoc_etc: pst_mac_vap mib ptr null.");
        return us_disassoc_len;
    }
    /* 设置DA */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address1, puc_da);

    /* 设置SA */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address2, mac_mib_get_StationID(pst_mac_vap));

    /* 设置bssid */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address3, pst_mac_vap->en_vap_mode == WLAN_VAP_MODE_BSS_AP ?
                                                            mac_mib_get_StationID(pst_mac_vap) : pst_mac_vap->auc_bssid);



    /*************************************************************************/
    /*                  Disassociation 帧 - 帧体                  */
    /* --------------------------------------------------------------------- */
    /* |                           Reason Code                             | */
    /* --------------------------------------------------------------------- */
    /* |2 Byte                                                             | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* 设置reason code*/
    puc_data[MAC_80211_FRAME_LEN]     = (us_err_code & 0x00FF);
    puc_data[MAC_80211_FRAME_LEN + 1] = (us_err_code & 0xFF00) >> 8;

    us_disassoc_len = MAC_80211_FRAME_LEN + WLAN_REASON_CODE_LEN;

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_ASSOC, "{hmac_mgmt_encap_disassoc_etc::encap disasoc!}\r\n");
#endif

    return us_disassoc_len;
}

oal_uint16 hmac_encap_notify_chan_width_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_data, oal_uint8 *puc_da)
{
    oal_uint16 us_len = 0;

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
    /* All the fields of the Frame Control Field are set to zero. Only the   */
    /* Type/Subtype field is set.                                            */
    mac_hdr_set_frame_control(puc_data, WLAN_FC0_SUBTYPE_ACTION);
    /*  Set DA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address1, puc_da);
    /*  Set SA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address2, mac_mib_get_StationID(pst_mac_vap));
    /*  Set SSID  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_data)->auc_address3, pst_mac_vap->auc_bssid);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                  SA Query Frame - Frame Body                          */
    /* --------------------------------------------------------------------- */
    /* |   Category   |SA Query Action |  Transaction Identifier           | */
    /* --------------------------------------------------------------------- */
    /* |1             |1               |2 Byte                             | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
     puc_data[MAC_80211_FRAME_LEN] = MAC_ACTION_CATEGORY_HT;
     puc_data[MAC_80211_FRAME_LEN+1] = MAC_HT_ACTION_NOTIFY_CHANNEL_WIDTH;
     puc_data[MAC_80211_FRAME_LEN+2] = (pst_mac_vap->st_channel.en_bandwidth > WLAN_BAND_WIDTH_20M) ? 1 : 0;

     us_len = MAC_80211_FRAME_LEN + MAC_HT_NOTIFY_CHANNEL_WIDTH_LEN;
     return us_len;

}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

