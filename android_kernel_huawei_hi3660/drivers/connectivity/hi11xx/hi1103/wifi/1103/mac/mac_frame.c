


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "wlan_spec.h"
#include "mac_ie.h"
#include "mac_frame.h"
#include "mac_vap.h"
#include "mac_device.h"
#include "mac_regdomain.h"
#include "dmac_ext_if.h"
#include "mac_user.h"
#ifdef _PRE_WLAN_FEATURE_11AX
#include "mac_resource.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_FRAME_C


/*****************************************************************************
  2 函数原型声明
*****************************************************************************/


/*****************************************************************************
  3 全局变量定义
*****************************************************************************/



/*****************************************************************************
  4 函数实现
*****************************************************************************/


oal_uint8 *mac_find_ie_ext_ie(oal_uint8 uc_eid,oal_uint8 uc_ext_eid, oal_uint8 *puc_ies, oal_int32 l_len)
{
    if (OAL_PTR_NULL == puc_ies)
    {
        return OAL_PTR_NULL;
    }

    while ((l_len > MAC_IE_EXT_HDR_LEN) && ((puc_ies[0] != uc_eid)||(puc_ies[2] != uc_ext_eid)))
    {
        l_len   -= puc_ies[1] + MAC_IE_HDR_LEN;
        puc_ies += puc_ies[1] + MAC_IE_HDR_LEN;
    }

    if ((l_len > MAC_IE_EXT_HDR_LEN) && (uc_eid == puc_ies[0]) && (uc_ext_eid == puc_ies[2]) &&
    (l_len >= (MAC_IE_HDR_LEN + puc_ies[1])))
    {
        return puc_ies;
    }

    return OAL_PTR_NULL;
}

#ifdef _PRE_WLAN_FEATURE_11AX

OAL_STATIC oal_void mac_set_he_mac_capinfo_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru              *pst_mac_vap        = (mac_vap_stru *)pst_vap;

    mac_frame_he_mac_cap_stru *pst_he_mac_capinfo = (mac_frame_he_mac_cap_stru *)puc_buffer;

    OAL_MEMZERO(pst_he_mac_capinfo, OAL_SIZEOF(mac_frame_he_mac_cap_stru));
    /*********************** HE MAC 能力信息域 ************************************
    ----------------------------------------------------------------------------
     |-----------------------------------------------------------------------------------------|
     | +HTC    | TWT         | TWT         | Fragmentation | Max Num      | Min                |
     | HE        | Requester | Responder   | support       | Fragmented   | Fragment           |
     | Support | Support     |   Support   |               | MSDUs        |     Size           |
     |-----------------------------------------------------------------------------------------|
     | B0        | B1        | B2          |   B3  B4      |  B5 B6 B7    |  B8 B9             |
     |-----------------------------------------------------------------------------------------|
     |-----------------------------------------------------------------------------------------|
     | Trigger Frame | Muti-TID     | HE           | ALL        | UMRS        | BSR            |
     | MAC Padding   | Aggregation  | Link         | ACK        | Support     | Support        |
     | Duration      | Support      | Adaptation   | Support    |             |                |
     |-----------------------------------------------------------------------------------------|
     |    B10   B11  | B12      B14 | B15    B16   |   B17      |    B18      | B19            |
     |-----------------------------------------------------------------------------------------|
     |-----------------------------------------------------------------------------------------|
     | Broadcast | 32-bit BA | MU       | Ack-Enable             | Group Addressed   Multi-STA |
     | TWT       | bitmap    | Cascade  | Multi-TID              | BlockAck In DL MU           |
     | Support   | Support   | Support  | Aggregation Support    | Support                     |
     |-----------------------------------------------------------------------------------------|
     | B20       | B21       | B22      |   B23                  |   B24                       |
     |-----------------------------------------------------------------------------------------|
     |-----------------------------------------------------------------------------------------|
     | OM        | OFDMA   | Max A-MPDU  | A-MSDU          | Flexible  TWT| Rx Control         |
     | Control   | RA      | Length      | Fragmentation   |  schedule    | frame to           |
     | Support   | Support | Exponent    |  Support        | Support      | MultiBSS           |
     |---------------------------------------------------------------------------------------- |
     | B25       | B26     | B27  B28    |   B29           |   B30        |  B31               |
     |-----------------------------------------------------------------------------------------|
     |-----------------------------------------------------------------------------------------|
     | NSRP BQRP   | QTP     | BQR     | SR       | NDP     | OPS      | A-MSDU in |           |
     | A-MPDU      |         |         |          |Feedback |  Support | A-MPDU    |           |
     | Aggregation | Support | Support |Responder | Report  |          | Support   |           |
     |-----------------------------------------------------------------------------------------|
     | B32         | B33     | B34     |   B35    |   B36   |  B37     |   B38     |           |
     |-----------------------------------------------------------------------------------------|
     |-----------------------------------------------------------------------------------------|
     | Multi-TID            |                                                                  |
     | Aggregation  Tx      |         Reserved                                                 |
     | Support              |                                                                  |
     |-----------------------------------------------------------------------------------------|
     | B39  B40   B41       |         B42--B47                                                 |
     |-----------------------------------------------------------------------------------------|
    ***************************************************************************/

    pst_he_mac_capinfo->bit_htc_he_support                         = mac_mib_get_he_HTControlFieldSupported(pst_mac_vap);

    /*B3-B4:fragmentation support  认证用例:5.30 不支持   */
    pst_he_mac_capinfo->bit_fragmentation_support                  = 0;

    /*B10-11:Trigger Frame MAC Padding Duration   16us*/
    pst_he_mac_capinfo->bit_trigger_mac_padding_duration           = mac_mib_get_he_TriggerMacPaddingDuration(pst_mac_vap);

    /*B12-B14:Multi-TID Aggregation Rx support*/
    pst_he_mac_capinfo->bit_mtid_aggregation_rx_support            = 0;

    /*B17:All Ack Support,MBA*/
    pst_he_mac_capinfo->bit_all_ack_support                        = 1;

    /*B19:BSR Support*/
    pst_he_mac_capinfo->bit_bsr_support                            = 0;

    /*B21:32-bit BA Bitmap Support */
    pst_he_mac_capinfo->bit_32bit_ba_bitmap_support                = 0;

    /*B23:Ack Enabled Aggregation Support */
    pst_he_mac_capinfo->bit_ack_enabled_aggregation_support        = 1;

    /*B24:Group Addressed Multi-STA BA in DL MU Support */
    pst_he_mac_capinfo->bit_group_addressed_msta_ba_dl_mu_support  = 1;

    /*B26:OFDMA RA Support */
    pst_he_mac_capinfo->bit_ofdma_ra_support                       = 0;

    /*B27-28:Maximum AMPDU Length Exponent */
    pst_he_mac_capinfo->bit_max_ampdu_length_exponent              = mac_mib_get_he_MaxAMPDULength(pst_vap);

    /*B36:NDP Feedback Report Support */
    pst_he_mac_capinfo->bit_ndp_feedback_report_support            = 0;

    /*B38:A-MSDU In A-MPDU Support */
    pst_he_mac_capinfo->bit_amsdu_ampdu_support                    = 1;


    return;

 }


 
OAL_STATIC oal_void mac_set_he_phy_capinfo_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    mac_vap_stru              *pst_mac_vap        = (mac_vap_stru *)pst_vap;
    mac_device_stru           *pst_mac_dev;
    mac_frame_he_phy_cap_stru *pst_he_phy_capinfo = (mac_frame_he_phy_cap_stru *)puc_buffer;
    mac_vap_rom_stru          *pst_mac_vap_rom;
#ifdef _PRE_WLAN_FEATURE_M2S
    mac_user_stru             *pst_mac_user;
#endif

    pst_mac_vap_rom = (mac_vap_rom_stru *)pst_mac_vap->_rom;

    OAL_MEMZERO(pst_he_phy_capinfo, OAL_SIZEOF(mac_frame_he_phy_cap_stru));

    pst_mac_dev                                          = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        return;
    }

    /*B1-B7:channel width set   */
    pst_he_phy_capinfo->bit_channel_width_set            = mac_device_trans_bandwith_to_he_capinfo(MAC_DEVICE_GET_CAP_BW(pst_mac_dev));

    /*B13:LDPC Coding in payload,认证用例5.24、5.25要求支持  */
    pst_he_phy_capinfo->bit_ldpc_coding_in_paylod        = mac_mib_get_he_LDPCCodingInPayload(pst_mac_vap);

    /*B31:SU Beamformer*/
    pst_he_phy_capinfo->bit_su_beamformer                = mac_mib_get_he_SUBeamformer(pst_mac_vap);

    /*B32:SU Beamformee*/
    pst_he_phy_capinfo->bit_su_beamformee                = mac_mib_get_he_SUBeamformee(pst_mac_vap);

    /*B33:MU Beamformer*/
    pst_he_phy_capinfo->bit_mu_beamformer                = mac_mib_get_he_MUBeamformer(pst_mac_vap);

    /*B34-B36:Beamformee STS ≤ 80MHz,本地与ap取小,同11ac*/
    pst_he_phy_capinfo->bit_beamformee_sts_below_80mhz   = VHT_BFEE_NTX_SUPP_STS_CAP- 1;

#ifdef _PRE_WLAN_FEATURE_M2S
    /* 参考标杆,该字段根据对端num of sounding和自己的能力取交集*/
    pst_mac_user = mac_res_get_mac_user_etc(pst_mac_vap->us_assoc_vap_id);
    if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode && OAL_PTR_NULL != pst_mac_user)
    {
        pst_he_phy_capinfo->bit_beamformee_sts_below_80mhz = OAL_MIN(pst_he_phy_capinfo->bit_beamformee_sts_below_80mhz,
                                        pst_mac_user->st_he_hdl.st_he_cap_ie.st_he_phy_cap.bit_below_80mhz_sounding_dimensions_num);
    }
#endif

    /*B46:Ng = 16 SU Feedback  */
    pst_he_phy_capinfo->bit_ng16_su_feedback             = 1;

    /*B47:Ng = 16 MU Feedback  */
    pst_he_phy_capinfo->bit_ng16_mu_feedback             = 1;

    /*B48:CodeBook Size(Φ,Ψ)={7,5} SU Feedback  */
    pst_he_phy_capinfo->bit_codebook_42_su_feedback      = 1;
    pst_he_phy_capinfo->bit_codebook_75_mu_feedback      = 1;

    /*B55:PPE Threshold Present  */
    pst_he_phy_capinfo->bit_ppe_threshold_present        = mac_mib_get_PPEThresholdsRequired(pst_mac_vap_rom);

    /*B65:20/40MHz HE PPDU 2.4G   */
    pst_he_phy_capinfo->bit_20mhz_in_40mhz_he_ppdu_2g    = 1;
    pst_he_phy_capinfo->bit_max_nc                       = 1;
}



OAL_STATIC  oal_uint8 mac_set_he_tx_rx_mcs_nss_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    oal_uint8      uc_len                                      = 0;
    mac_fram_he_mac_nsss_set_stru *pst_he_tx_rx_mcs_nss_info   = (mac_fram_he_mac_nsss_set_stru *)puc_buffer;

    OAL_MEMZERO(pst_he_tx_rx_mcs_nss_info, OAL_SIZEOF(mac_fram_he_mac_nsss_set_stru));

    /*03仅支持<=80Mhz*/
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_1ss = MAC_MAX_SUP_MCS7_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_2ss = MAC_MAX_SUP_MCS7_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_3ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_4ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_5ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_6ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_8ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_rx_he_mcs_below_80mhz.bit_max_he_mcs_for_7ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;

    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_1ss = MAC_MAX_SUP_MCS7_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_2ss = MAC_MAX_SUP_MCS7_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_3ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_4ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_5ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_6ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_8ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;
    pst_he_tx_rx_mcs_nss_info->st_tx_he_mcs_below_80mhz.bit_max_he_mcs_for_7ss = MAC_MAX_SUP_INVALID_11AX_EACH_NSS;

    uc_len = OAL_SIZEOF(mac_fram_he_mac_nsss_set_stru);

    /*160MHz相关暂不适配*/

    return uc_len;
}


OAL_STATIC oal_uint8 mac_set_he_ppe_thresholds_field(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
    oal_uint8                                  uc_len = 0;
    mac_frame_ppe_thresholds_pre_field_stru   *pst_ppe_thresholds;

    pst_ppe_thresholds = (mac_frame_ppe_thresholds_pre_field_stru *)puc_buffer;

    OAL_MEMZERO(pst_ppe_thresholds,OAL_SIZEOF(mac_frame_ppe_thresholds_pre_field_stru));

    pst_ppe_thresholds->bit_nss            = 1;/*双流*/
    pst_ppe_thresholds->bit_ru_index0_mask = 1;/*支持RU-242*/
    pst_ppe_thresholds->bit_ru_index1_mask = 1;/*支持RU-484*/
    pst_ppe_thresholds->bit_ru_index2_mask = 1;/*支持RU-996*/

    pst_ppe_thresholds->bit_ppet16_nss1_ru0 = 0;
    pst_ppe_thresholds->bit_ppet8_nss1_ru0  = 7;
    pst_ppe_thresholds->bit_ppet16_nss1_ru1 = 0;
    pst_ppe_thresholds->bit_ppet8_nss1_ru1  = 7;
    pst_ppe_thresholds->bit_ppet16_nss1_ru2 = 0;
    pst_ppe_thresholds->bit_ppet8_nss1_ru2  = 7;

    pst_ppe_thresholds->bit_ppet16_nss2_ru0 = 0;
    pst_ppe_thresholds->bit_ppet8_nss2_ru0  = 7;
    pst_ppe_thresholds->bit_ppet16_nss2_ru1 = 0;
    pst_ppe_thresholds->bit_ppet8_nss2_ru1  = 7;
    pst_ppe_thresholds->bit_ppet16_nss2_ru2 = 0;
    pst_ppe_thresholds->bit_ppet8_nss2_ru2  = 7;

    uc_len = OAL_SIZEOF(mac_frame_ppe_thresholds_pre_field_stru);

    return uc_len;

}




 /*lint -save -e438 */
 oal_void mac_set_he_capabilities_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
 {
     mac_vap_stru              *pst_mac_vap        = (mac_vap_stru *)pst_vap;
     oal_uint8                 *puc_ie_length;
     oal_uint8                  uc_info_length;
     mac_vap_rom_stru          *pst_mac_vap_rom;

    *puc_ie_len = 0;

     if (OAL_TRUE != mac_mib_get_HEOptionImplemented(pst_mac_vap))
     {
         return;
     }

    /*PF认证要求对于WEP、TKIP 加密方式不能关联在HE模式*/
    if ((OAL_TRUE != mac_mib_get_HEOptionImplemented(pst_mac_vap))
            || ((OAL_TRUE == mac_is_wep_enabled(pst_mac_vap))||(OAL_TRUE == mac_is_tkip_only(pst_mac_vap))))
    {
        *puc_ie_len = 0;
        return;
    }

    pst_mac_vap_rom = (mac_vap_rom_stru *)pst_mac_vap->_rom;

    /***************************************************************************
    -------------------------------------------------------------------------
    |EID |Length |EID Extension|HE MAC Capa. Info |HE PHY Capa. Info|
    -------------------------------------------------------------------------
    |1   |1          |1                  |5                         |9                       |
    -------------------------------------------------------------------------
    |Tx Rx HE MCS NSS Support |PPE Thresholds(Optional)|
    -------------------------------------------------------------------------
    |2 or More                         |Variable                       |
    -------------------------------------------------------------------------
    ***************************************************************************/
    *puc_buffer    = MAC_EID_HE;
    puc_ie_length  = puc_buffer + 1;

    puc_buffer    += MAC_IE_HDR_LEN;
    *puc_ie_len   += MAC_IE_HDR_LEN;

    *puc_buffer    = MAC_EID_EXT_HE_CAP;
    puc_buffer    += 1;

    *puc_ie_len   += 1;

    /* 填充HE mac capabilities information域信息 */
    mac_set_he_mac_capinfo_field(pst_vap, puc_buffer);
    puc_buffer    += MAC_HE_MAC_CAP_LEN;
    *puc_ie_len   += MAC_HE_MAC_CAP_LEN;

    /* 填充HE PHY Capabilities Information 域信息 */
    mac_set_he_phy_capinfo_field(pst_vap, puc_buffer);
    puc_buffer    += MAC_HE_PHY_CAP_LEN;
    *puc_ie_len   += MAC_HE_PHY_CAP_LEN;

    /*填充 HE tx rx he mcs nss support*/
    uc_info_length = mac_set_he_tx_rx_mcs_nss_field(pst_vap,puc_buffer);
    puc_buffer    += uc_info_length;
    *puc_ie_len   += uc_info_length;

    /*填充 PPE Thresholds field*/
    if(mac_mib_get_PPEThresholdsRequired(pst_mac_vap_rom))
    {
        uc_info_length = mac_set_he_ppe_thresholds_field(pst_vap,puc_buffer);
        puc_buffer    += uc_info_length;
        *puc_ie_len   += uc_info_length;
    }
    *puc_ie_length = *puc_ie_len - MAC_IE_HDR_LEN;

}
/*lint -restore */
#endif

oal_uint32  mac_vap_set_cb_tx_user_idx(mac_vap_stru *pst_mac_vap, mac_tx_ctl_stru *pst_tx_ctl, oal_uint8 *puc_data)
{

    oal_uint16  us_user_idx = MAC_INVALID_USER_ID;
    oal_uint32  ul_ret;

    ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_data, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {

        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_vap_set_cb_tx_user_idx:: cannot find user_idx from xx:xx:xx:%x:%x:%x, set TX_USER_IDX %d.}",
        puc_data[3],
        puc_data[4],
        puc_data[5],
        MAC_INVALID_USER_ID);
        MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = MAC_INVALID_USER_ID;
        return ul_ret;
    }

    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl) = us_user_idx;

    return OAL_SUCC;
}
/*lint -e578*//*lint -e19*/
oal_module_symbol(mac_vap_set_cb_tx_user_idx);
/*lint -e578*//*lint -e19*/

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE

oal_void mac_set_ie_field(oal_void *pst_data, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    oal_uint8                   uc_index = 0;
    oal_uint8                   *puc_ie_buff;
    mac_vap_ie_set_stru         *pst_set_ie_info;

    /* vap下对应的ie设置指针不为空则需要设置 */
    if (OAL_PTR_NULL == pst_data)
    {
        *puc_ie_len = 0;
        return;
    }
    pst_set_ie_info = (mac_vap_ie_set_stru *)pst_data;
    puc_ie_buff = pst_set_ie_info->auc_ie_content;
    /* 根据ie设置类型进行IE设置 */
    switch (pst_set_ie_info->en_set_type)
    {
        case OAL_IE_SET_TYPE_AND:    /* 与操作 */
        {
            if (0 == *puc_ie_len)       /* 长度为0 说明帧不包含该IE 无法与操作 */
            {
                return;
            }
            /* 与操作 */
            for (uc_index=0; uc_index<pst_set_ie_info->us_ie_content_len; uc_index++)
            {
                /* 从IE的content 开始进行与操作 */
                puc_buffer[MAC_IE_HDR_LEN+uc_index] = puc_buffer[MAC_IE_HDR_LEN+uc_index] & puc_ie_buff[uc_index];
            }
            *puc_ie_len = 0;                            /* 没有新增BUFF DATA 将长度置0 */
            return;
        }
        case OAL_IE_SET_TYPE_OR:
        {
            if (0 == *puc_ie_len)       /* 长度为0 说明帧不包含该IE 则添加IE到BUFF */
            {
                puc_buffer[0] = pst_set_ie_info->en_eid;
                puc_buffer[1] = (oal_uint8)pst_set_ie_info->us_ie_content_len;
                oal_memcopy(puc_buffer+MAC_IE_HDR_LEN, puc_ie_buff, pst_set_ie_info->us_ie_content_len);
                *puc_ie_len = (oal_uint8)(pst_set_ie_info->us_ie_content_len) + MAC_IE_HDR_LEN;   /* 新增IE 修改长度 */
            }
            else
            {
                /* 或操作 */
                for (uc_index=0; uc_index<pst_set_ie_info->us_ie_content_len; uc_index++)
                {
                    /* 从IE的content 开始进行或操作 */
                    puc_buffer[MAC_IE_HDR_LEN+uc_index] = puc_buffer[MAC_IE_HDR_LEN+uc_index] | puc_ie_buff[uc_index];
                }
                *puc_ie_len = 0;                            /* 没有新增BUFF DATA 将长度置0 */
            }
            return;
        }
        case OAL_IE_SET_TYPE_ADD:
        {
            if (0 == *puc_ie_len)       /* 长度为0 说明帧不包含该IE 则添加IE到BUFF */
            {
                puc_buffer[0] = pst_set_ie_info->en_eid;
                puc_buffer[1] = (oal_uint8)pst_set_ie_info->us_ie_content_len;
                oal_memcopy(puc_buffer+MAC_IE_HDR_LEN, puc_ie_buff, pst_set_ie_info->us_ie_content_len);
                *puc_ie_len = (oal_uint8)(pst_set_ie_info->us_ie_content_len) + MAC_IE_HDR_LEN;   /* 新增IE 修改长度 */
            }
            else    /* 存在该IE 则替换现有IE的内容 */
            {
                oal_memcopy(puc_buffer+MAC_IE_HDR_LEN, puc_ie_buff, pst_set_ie_info->us_ie_content_len);
                *puc_ie_len = 0;                            /* 没有新增BUFF DATA 将长度置0 */
            }
            return;
        }
        default:    /* 其他设置类型不支持 直接返回 */
            *puc_ie_len = 0;
            return;
    }
}

#endif // end of _PRE_WLAN_FEATURE_11KV_INTERFACE

#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA

 oal_void mac_set_vender_4addr_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
 {
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *) pst_vap;

    *puc_ie_len = 0;

    /***************************************************************************
    -------------------------------------------------------------------------
    |EID | Length |HUAWEI OUI |WIFI OUT FOUR ADDR |HE PHY Capa. Info| Version |
    -------------------------------------------------------------------------
    |221 |variable|     3     |         4         |        待定     |         |
    -------------------------------------------------------------------------
    ***************************************************************************/
    /* 直接调用11KV接口的设置IE接口 宏未定义编译失败 */
#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
    mac_set_ie_field(pst_mac_vap->pst_msta_ie_info, puc_buffer, puc_ie_len);
#else
#error "the virtual multi-sta feature dependent _PRE_WLAN_FEATURE_11KV_INTERFACE, please define it!"
#endif

}
#endif


oal_void mac_ftm_add_to_ext_capabilities_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
#ifdef _PRE_WLAN_FEATURE_FTM
    mac_vap_stru                *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_ext_cap_ftm_ie_stru     *pst_ext_cap_ftm;
    mac_ftm_mode_enum_uint8      en_ftm_mode = mac_check_ftm_enable(pst_mac_vap);

    if(MAC_FTM_DISABLE_MODE == en_ftm_mode)
    {
        return;
    }

    puc_buffer[1] = MAC_XCAPS_EX_FTM_LEN;

    pst_ext_cap_ftm = (mac_ext_cap_ftm_ie_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    switch (en_ftm_mode)
    {
        case MAC_FTM_RESPONDER_MODE:
            pst_ext_cap_ftm->bit_ftm_resp = OAL_TRUE;
            pst_ext_cap_ftm->bit_ftm_int = OAL_FALSE;
            break;

        case MAC_FTM_INITIATOR_MODE:
            pst_ext_cap_ftm->bit_ftm_resp = OAL_FALSE;
            pst_ext_cap_ftm->bit_ftm_int = OAL_TRUE;
            break;

        case MAC_FTM_MIX_MODE:
            pst_ext_cap_ftm->bit_ftm_resp = OAL_TRUE;
            pst_ext_cap_ftm->bit_ftm_int = OAL_TRUE;
            break;

        default:
            break;
    }

    (*puc_ie_len)++;
#endif
}

#ifdef _PRE_WLAN_FEATURE_1024QAM

oal_void mac_set_1024qam_vendor_ie(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru                        *pst_mac_vap = (mac_vap_stru *)pst_vap;
    mac_ieee80211_vendor_ie_stru          *pst_vendor_ie;

    if (OAL_TRUE != pst_mac_vap->st_cap_flag.bit_1024qam)
    {
        *puc_ie_len = 0;
        return;
    }

    pst_vendor_ie = (mac_ieee80211_vendor_ie_stru *)puc_buffer;
    pst_vendor_ie->uc_element_id = MAC_EID_VENDOR;
    pst_vendor_ie->uc_len = sizeof(mac_ieee80211_vendor_ie_stru) - MAC_IE_HDR_LEN;

    pst_vendor_ie->uc_oui_type = MAC_HISI_1024QAM_IE;

    pst_vendor_ie->auc_oui[0] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 16) & 0xff);
    pst_vendor_ie->auc_oui[1] = (oal_uint8)((MAC_HUAWEI_VENDER_IE >> 8) & 0xff);
    pst_vendor_ie->auc_oui[2] = (oal_uint8)((MAC_HUAWEI_VENDER_IE) & 0xff);

    *puc_ie_len = OAL_SIZEOF(mac_ieee80211_vendor_ie_stru);

}
#endif


oal_void mac_set_vendor_vht_ie(oal_void *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    oal_uint8 uc_ie_len;

    puc_buffer[0] = MAC_EID_VENDOR;
    puc_buffer[1] = MAC_WLAN_OUI_VENDOR_VHT_HEADER; /* The Vendor OUI, type and subtype */
    /*lint -e572*/ /*lint -e778*/
    puc_buffer[2] = (oal_uint8)((MAC_WLAN_OUI_BROADCOM_EPIGRAM >> 16) & 0xff);
    puc_buffer[3] = (oal_uint8)((MAC_WLAN_OUI_BROADCOM_EPIGRAM >> 8)  & 0xff);
    puc_buffer[4] = (oal_uint8)((MAC_WLAN_OUI_BROADCOM_EPIGRAM) & 0xff);
    puc_buffer[5] = MAC_WLAN_OUI_VENDOR_VHT_TYPE;
    puc_buffer[6] = MAC_WLAN_OUI_VENDOR_VHT_SUBTYPE;
    /*lint +e572*/ /*lint +e778*/

    mac_set_vht_capabilities_ie_etc(pst_mac_vap, puc_buffer + puc_buffer[1] + MAC_IE_HDR_LEN, &uc_ie_len);
    if (uc_ie_len)
    {
        puc_buffer[1] += uc_ie_len;
        *puc_ie_len = puc_buffer[1] + MAC_IE_HDR_LEN;
    }
    else
    {
        *puc_ie_len = 0;
    }
}

oal_void mac_set_vendor_novht_ie(oal_void *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    puc_buffer[0] = MAC_EID_VENDOR;
    puc_buffer[1] = MAC_WLAN_OUI_VENDOR_VHT_HEADER; /* The Vendor OUI, type and subtype */
    /*lint -e572*/ /*lint -e778*/
    puc_buffer[2] = (oal_uint8)((MAC_WLAN_OUI_BROADCOM_EPIGRAM >> 16) & 0xff);
    puc_buffer[3] = (oal_uint8)((MAC_WLAN_OUI_BROADCOM_EPIGRAM >> 8)  & 0xff);
    puc_buffer[4] = (oal_uint8)((MAC_WLAN_OUI_BROADCOM_EPIGRAM) & 0xff);
    puc_buffer[5] = MAC_WLAN_OUI_VENDOR_VHT_TYPE;

    puc_buffer[6] = MAC_WLAN_OUI_VENDOR_VHT_SUBTYPE3;
    /*lint +e572*/ /*lint +e778*/
    *puc_ie_len = puc_buffer[1] + MAC_IE_HDR_LEN;
}


oal_void mac_set_ext_capabilities_ie_rom_cb(oal_void *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
#if defined(_PRE_WLAN_FEATURE_11V) || defined(_PRE_WLAN_FEATURE_11V_ENABLE)
    mac_ext_cap_ie_stru     *pst_ext_cap;

    pst_ext_cap = (mac_ext_cap_ie_stru *)(puc_buffer + MAC_IE_HDR_LEN);

     /* 首先需先使能wirelessmanagerment标志 */
     /* 然后如果是站点本地能力位和扩展控制变量均支持BSS TRANSITION 设置扩展能力bit位 */
    if ( (OAL_TRUE == mac_mib_get_WirelessManagementImplemented(pst_mac_vap)) &&
         (OAL_TRUE == mac_mib_get_MgmtOptionBSSTransitionImplemented(pst_mac_vap)) &&
         (OAL_TRUE == mac_mib_get_MgmtOptionBSSTransitionActivated(pst_mac_vap)))
    {
        pst_ext_cap->bit_bss_transition = 1;
    }
#endif
    mac_ftm_add_to_ext_capabilities_ie(pst_mac_vap, puc_buffer, puc_ie_len);
}



oal_void mac_set_ht_cap_ie_rom_cb(oal_void *pst_mac_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_frame_ht_cap_stru *pst_ht_capinfo;

    /* 将puc_buffer地址指向ht_capinfo字段,puc_buffer当前地址在HT_ASEL字段 */
    puc_buffer -= MAC_HT_CAP_LEN - MAC_HT_ASEL_LEN;

    pst_ht_capinfo = (mac_frame_ht_cap_stru *)puc_buffer;

    /* 设置所支持的信道宽度集"，0:仅20MHz运行; 1:20MHz与40MHz运行 */
    pst_ht_capinfo->bit_supported_channel_width = mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap);

    /* 只有支持40M的情况下，才可以宣称支持40M short GI */
    if(pst_ht_capinfo->bit_supported_channel_width)
    {
        pst_ht_capinfo->bit_short_gi_40mhz = mac_mib_get_ShortGIOptionInFortyImplemented(pst_mac_vap);
    }
    else
    {
        pst_ht_capinfo->bit_short_gi_40mhz = 0;
    }

}



oal_void mac_set_ht_opern_ie_rom_cb(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru      *pst_mac_vap  = (mac_vap_stru *)pst_vap;
    mac_ht_opern_stru *pst_ht_opern = (mac_ht_opern_stru *)(puc_buffer + MAC_IE_HDR_LEN);

    /* 设置"次信道偏移量" */
    switch (pst_mac_vap->st_channel.en_bandwidth)
    {
        case WLAN_BAND_WIDTH_40PLUS:
        case WLAN_BAND_WIDTH_80PLUSPLUS:
        case WLAN_BAND_WIDTH_80PLUSMINUS:
            pst_ht_opern->bit_secondary_chan_offset = MAC_SCA;
            break;

        case WLAN_BAND_WIDTH_40MINUS:
        case WLAN_BAND_WIDTH_80MINUSPLUS:
        case WLAN_BAND_WIDTH_80MINUSMINUS:
            pst_ht_opern->bit_secondary_chan_offset = MAC_SCB;
            break;

#ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BAND_WIDTH_160PLUSPLUSPLUS:
        case WLAN_BAND_WIDTH_160PLUSPLUSMINUS:
        case WLAN_BAND_WIDTH_160PLUSMINUSPLUS:
        case WLAN_BAND_WIDTH_160PLUSMINUSMINUS:
            pst_ht_opern->bit_secondary_chan_offset = MAC_SCA;
            break;

        case WLAN_BAND_WIDTH_160MINUSPLUSPLUS:
        case WLAN_BAND_WIDTH_160MINUSPLUSMINUS:
        case WLAN_BAND_WIDTH_160MINUSMINUSPLUS:
        case WLAN_BAND_WIDTH_160MINUSMINUSMINUS:
            pst_ht_opern->bit_secondary_chan_offset = MAC_SCB;
            break;
#endif

        default:
            pst_ht_opern->bit_secondary_chan_offset = MAC_SCN;
            break;
    }

}


oal_void mac_set_vht_capinfo_field_cb(oal_void *pst_vap, oal_uint8 *puc_buffer)
{
#ifdef _PRE_WLAN_FEATURE_M2S
    mac_vap_stru           *pst_mac_vap     = (mac_vap_stru *)pst_vap;
    mac_vht_cap_info_stru  *pst_vht_capinfo = (mac_vht_cap_info_stru *)puc_buffer;
    mac_user_stru          *pst_mac_user;

    pst_vht_capinfo->bit_num_bf_ant_supported    = mac_mib_get_VHTBeamformeeNTxSupport(pst_mac_vap) - 1;

    /* 参考标杆,该字段根据对端空间流能力和自己的能力取交集*/
    pst_mac_user = mac_res_get_mac_user_etc(pst_mac_vap->us_assoc_vap_id);
    if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode && OAL_PTR_NULL != pst_mac_user &&
        (0 != pst_mac_user->st_vht_hdl.bit_num_sounding_dim))
    {
        pst_vht_capinfo->bit_num_bf_ant_supported = OAL_MIN(pst_vht_capinfo->bit_num_bf_ant_supported,
                                                               pst_mac_user->st_vht_hdl.bit_num_sounding_dim);
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* 算法限定mu bfee只在WLAN0开启 */
    if (!IS_LEGACY_STA(pst_mac_vap))
    {
        pst_vht_capinfo->bit_mu_beamformee_cap = OAL_FALSE;
    }
#endif
    if(IS_LEGACY_AP(pst_mac_vap) && ((pst_mac_vap->st_channel.en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS) || (pst_mac_vap->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_40M)))
    {
        pst_vht_capinfo->bit_supported_channel_width = 0;
        pst_vht_capinfo->bit_short_gi_160mhz = 0;
    }
#endif
}



oal_void mac_set_vht_opern_ie_rom_cb(oal_void *pst_vap, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    mac_vap_stru       *pst_mac_vap  = (mac_vap_stru *)pst_vap;
    mac_vht_opern_stru *pst_vht_opern = (mac_vht_opern_stru *)puc_buffer;

    if ((pst_mac_vap->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) && (pst_mac_vap->st_channel.en_bandwidth <= WLAN_BAND_WIDTH_80MINUSMINUS))
    {
        pst_vht_opern->uc_channel_width = WLAN_MIB_VHT_OP_WIDTH_80;
    }
#ifdef _PRE_WLAN_FEATURE_160M
    else if ((pst_mac_vap->st_channel.en_bandwidth >= WLAN_BAND_WIDTH_160PLUSPLUSPLUS) && (pst_mac_vap->st_channel.en_bandwidth <= WLAN_BAND_WIDTH_160MINUSMINUSMINUS))
    {
        pst_vht_opern->uc_channel_width = WLAN_MIB_VHT_OP_WIDTH_80;
    }
#endif
    else
    {
         pst_vht_opern->uc_channel_width = WLAN_MIB_VHT_OP_WIDTH_20_40;
    }

    switch (pst_mac_vap->st_channel.en_bandwidth)
    {

#ifdef _PRE_WLAN_FEATURE_160M
        /* 从20信道+1, 从40信道+1, 从80信道+1 */
        case WLAN_BAND_WIDTH_160PLUSPLUSPLUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 6;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number + 14;
        break;

        /* 从20信道+1, 从40信道+1, 从80信道-1 */
        case WLAN_BAND_WIDTH_160PLUSPLUSMINUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 6;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number - 2;
        break;

        /* 从20信道+1, 从40信道-1, 从80信道+1 */
        case WLAN_BAND_WIDTH_160PLUSMINUSPLUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 2;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number + 6;
        break;

        /* 从20信道+1, 从40信道-1, 从80信道-1 */
        case WLAN_BAND_WIDTH_160PLUSMINUSMINUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 2;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number - 10;
        break;

        /* 从20信道-1, 从40信道+1, 从80信道+1 */
        case WLAN_BAND_WIDTH_160MINUSPLUSPLUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 2;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number + 10;
        break;

        /* 从20信道-1, 从40信道+1, 从80信道-1 */
        case WLAN_BAND_WIDTH_160MINUSPLUSMINUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 2;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number - 6;
        break;

        /* 从20信道-1, 从40信道-1, 从80信道+1 */
        case WLAN_BAND_WIDTH_160MINUSMINUSPLUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 6;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number + 2;
        break;

        /* 从20信道-1, 从40信道-1, 从80信道-1 */
        case WLAN_BAND_WIDTH_160MINUSMINUSMINUS:
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 6;
            pst_vht_opern->uc_channel_center_freq_seg1 = pst_mac_vap->st_channel.uc_chan_number - 14;
        break;
#endif
        case WLAN_BAND_WIDTH_80PLUSPLUS:
            /***********************************************************************
            | 主20 | 从20 | 从40       |
                          |
                          |中心频率相对于主20偏6个信道
            ************************************************************************/
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 6;
            break;

        case WLAN_BAND_WIDTH_80PLUSMINUS:
            /***********************************************************************
            | 从40        | 主20 | 从20 |
                          |
                          |中心频率相对于主20偏-2个信道
            ************************************************************************/
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSPLUS:
            /***********************************************************************
            | 从20 | 主20 | 从40       |
                          |
                          |中心频率相对于主20偏2个信道
            ************************************************************************/
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSMINUS:
            /***********************************************************************
            | 从40        | 从20 | 主20 |
                          |
                          |中心频率相对于主20偏-6个信道
            ************************************************************************/
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 6;
            break;

#if 0 //为了提高兼容性，40M及其以下都填0
        case WLAN_BAND_WIDTH_40MINUS:
            /***********************************************************************
            | 从20 | 主20 |
            |
            | 中心频率相对于主20偏-2个信道
            ************************************************************************/
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number - 2;
            break;

        case WLAN_BAND_WIDTH_40PLUS:
            /***********************************************************************
            | 主20 | 从20 |
            |
            | 中心频率相对于主20偏+2个信道
            ************************************************************************/
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number + 2;
            break;

        case WLAN_BAND_WIDTH_20M:
            /* 中心频率就是主信道频率   */
            pst_vht_opern->uc_channel_center_freq_seg0 = pst_mac_vap->st_channel.uc_chan_number;
            break;
#endif

        default:
            /* 中心频率直接填0  */
            pst_vht_opern->uc_channel_center_freq_seg0 = 0;
            break;

    }

}

#ifdef _PRE_WLAN_NARROW_BAND

oal_void  mac_get_nb_ie( oal_void *pst_vap, oal_uint8 *puc_payload, oal_uint16 us_frame_len)
{
    oal_uint8       *puc_nb;
    oal_uint8        uc_nb_len;
    oal_uint8        uc_loop;
    oal_uint8       *puc_bw;

    mac_vap_stru    *pst_mac_vap = (mac_vap_stru *)pst_vap;

    puc_nb = mac_find_vendor_ie_etc(MAC_EID_VENDOR, MAC_HISI_NB_IE, puc_payload, us_frame_len);
    if (OAL_PTR_NULL == puc_nb)
    {
        return ;
    }

    uc_nb_len = puc_nb[1];

    if (uc_nb_len <=  MAC_OUI_LEN)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_get_nb_ie::nb_len[%u] err!", uc_nb_len);
        return;
    }

    puc_bw = puc_nb + MAC_IE_HDR_LEN + MAC_OUI_LEN + MAC_OUITYPE_LEN;
    for (uc_loop = 0; uc_loop < (uc_nb_len - (MAC_OUI_LEN + MAC_OUITYPE_LEN)); uc_loop++)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "mac_get_nb_ie::bw[%x]!", puc_bw[uc_loop]);
    }

}
#endif

/*lint -e19*/
#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
oal_module_symbol(mac_set_vender_4addr_ie);
#endif
/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

