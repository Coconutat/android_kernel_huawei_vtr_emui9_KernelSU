


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "wlan_mib.h"
#include "mac_vap.h"
#include "mac_frame.h"
#include "hmac_encap_frame_sta.h"
#include "hmac_user.h"
#include "hmac_main.h"
#include "hmac_tx_data.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_mgmt_sta.h"
#include "hmac_device.h"
#include "hmac_resource.h"
#include "hmac_scan.h"
#include "mac_user.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ENCAP_FRAME_STA_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/


hmac_scanned_bss_info* hmac_vap_get_scan_bss_info(mac_vap_stru *pst_mac_vap)
{
    hmac_device_stru       *pst_hmac_device;
    hmac_bss_mgmt_stru     *pst_bss_mgmt;
    hmac_scanned_bss_info  *pst_scaned_bss;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_QOS, "{mac_vap_get_scan_bss_info::pst_mac_device null.}");
        return OAL_PTR_NULL;
    }
    pst_bss_mgmt = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.st_bss_mgmt);
    pst_scaned_bss = hmac_scan_find_scanned_bss_by_bssid_etc(pst_bss_mgmt, pst_mac_vap->auc_bssid);
    if(OAL_PTR_NULL == pst_scaned_bss)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{mac_tx_qos_enhance_list_init::do not have scan result!!!}");
        return OAL_PTR_NULL;
    }
    return pst_scaned_bss;
}


OAL_STATIC oal_bool_enum hmac_sta_check_need_set_ext_cap_ie(mac_vap_stru *pst_mac_vap)
{
    oal_uint8       *puc_ext_cap_ie;
    oal_uint16       us_ext_cap_index;

    puc_ext_cap_ie = hmac_sta_find_ie_in_probe_rsp_etc(pst_mac_vap, MAC_EID_EXT_CAPS, &us_ext_cap_index);
    if (OAL_PTR_NULL == puc_ext_cap_ie)
    {
        return OAL_FALSE;
    }

    return OAL_TRUE;
}



oal_void  hmac_set_supported_rates_ie_asoc_req_etc(hmac_vap_stru *pst_hmac_sta, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{

    oal_uint8         uc_nrates;
    oal_uint8         uc_idx;

    /**************************************************************************
                        ---------------------------------------
                        |Element ID | Length | Supported Rates|
                        ---------------------------------------
             Octets:    |1          | 1      | 1~8            |
                        ---------------------------------------
    The Information field is encoded as 1 to 8 octets, where each octet describes a single Supported
    Rate or BSS membership selector.
    **************************************************************************/
    puc_buffer[0] = MAC_EID_RATES;

    uc_nrates = mac_mib_get_SupportRateSetNums(&pst_hmac_sta->st_vap_base_info);

    if (uc_nrates > MAC_MAX_SUPRATES)
    {
        uc_nrates = MAC_MAX_SUPRATES;
    }

    for (uc_idx = 0; uc_idx < uc_nrates; uc_idx++)
    {
        puc_buffer[MAC_IE_HDR_LEN + uc_idx] = pst_hmac_sta->auc_supp_rates[uc_idx];
    }

    puc_buffer[1] = uc_nrates;

    *puc_ie_len = MAC_IE_HDR_LEN + uc_nrates;
}

oal_void hmac_set_exsup_rates_ie_asoc_req_etc(hmac_vap_stru *pst_hmac_sta, oal_uint8 *puc_buffer, oal_uint8 *puc_ie_len)
{
    oal_uint8         uc_nrates;
    oal_uint8         uc_idx;


    /***************************************************************************
                   -----------------------------------------------
                   |ElementID | Length | Extended Supported Rates|
                   -----------------------------------------------
       Octets:     |1         | 1      | 1-255                   |
                   -----------------------------------------------
    ***************************************************************************/
    if (mac_mib_get_SupportRateSetNums(&pst_hmac_sta->st_vap_base_info) <= MAC_MAX_SUPRATES)
    {
        *puc_ie_len = 0;

        return;
    }

    puc_buffer[0] = MAC_EID_XRATES;
    uc_nrates     = mac_mib_get_SupportRateSetNums(&pst_hmac_sta->st_vap_base_info) - MAC_MAX_SUPRATES;
    puc_buffer[1] = uc_nrates;

    for (uc_idx = 0; uc_idx < uc_nrates; uc_idx++)
    {
        puc_buffer[MAC_IE_HDR_LEN + uc_idx] = pst_hmac_sta->auc_supp_rates[uc_idx + MAC_MAX_SUPRATES];
    }

    *puc_ie_len = MAC_IE_HDR_LEN + uc_nrates;
}


oal_uint32 hmac_mgmt_encap_asoc_req_sta_etc(hmac_vap_stru *pst_hmac_sta, oal_uint8 *puc_req_frame, oal_uint8 *puc_curr_bssid)
{

    oal_uint8               uc_ie_len            = 0;
    oal_uint32              us_asoc_rsq_len      = 0;
    oal_uint8              *puc_req_frame_origin;
    mac_vap_stru           *pst_mac_vap;
    mac_device_stru        *pst_mac_device;
    oal_uint16              us_app_ie_len        = 0;
    en_app_ie_type_uint8    en_app_ie_type;
    hmac_scanned_bss_info  *pst_scaned_bss;
#ifdef _PRE_WLAN_FEATURE_M2S
    mac_user_stru          *pst_mac_user;
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    hmac_user_stru         *pst_hmac_user;
#endif

#ifdef _PRE_WLAN_FEATURE_TXBF_HT
    mac_txbf_cap_stru *pst_txbf_cap;
#endif

#ifdef _PRE_WLAN_FEATURE_11R
        wlan_wme_ac_type_enum_uint8 en_aci;
        wlan_wme_ac_type_enum_uint8 en_target_ac;
        oal_uint8                   uc_tid;
#endif
#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN) || defined(_PRE_WLAN_FEATURE_FTM) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)
        oal_bool_enum_uint8         en_rrm_enable = OAL_TRUE;
#endif

    if ((OAL_PTR_NULL == pst_hmac_sta) || (OAL_PTR_NULL == puc_req_frame))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ASSOC, "{hmac_mgmt_encap_asoc_req_sta_etc::null param, pst_hmac_sta=%d puc_req_frame=%d.}",
                       pst_hmac_sta, puc_req_frame);
        return us_asoc_rsq_len;
    }

    /* 保存起始地址，方便计算长度*/
    puc_req_frame_origin = puc_req_frame;

    pst_mac_vap = &(pst_hmac_sta->st_vap_base_info);

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_encap_asoc_req_sta_etc::pst_mac_device null.}");
        return us_asoc_rsq_len;
    }

    pst_scaned_bss = hmac_vap_get_scan_bss_info(pst_mac_vap);

#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151) || !defined(WIN32)
    if (OAL_PTR_NULL == pst_scaned_bss)
    {
        OAM_ERROR_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_encap_asoc_req_sta_etc::pst_scaned_bss null.}");
        return us_asoc_rsq_len;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_M2S
    /* VHT中的字段需要根据AP的bf能力设置，故在此提前设置MAC_USER的bf能力*/
    pst_mac_user = mac_res_get_mac_user_etc(pst_mac_vap->us_assoc_vap_id);
#if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1151) || !defined(WIN32)
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_encap_asoc_req_sta_etc::pst_mac_user null.}");
        return us_asoc_rsq_len;
    }
#endif

    pst_mac_user->st_vht_hdl.bit_num_sounding_dim = pst_scaned_bss->st_bss_dscr_info.uc_num_sounding_dim;
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

    /* 设置 Frame Control field */
    /* 判断是否为reassoc操作 */
    if (OAL_PTR_NULL != puc_curr_bssid)
    {
        mac_hdr_set_frame_control(puc_req_frame, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_REASSOC_REQ);
    }
    else
    {
        mac_hdr_set_frame_control(puc_req_frame, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ASSOC_REQ);
    }
    /* 设置 DA address1: AP MAC地址 (BSSID)*/
    oal_set_mac_addr(puc_req_frame + WLAN_HDR_ADDR1_OFFSET, pst_hmac_sta->st_vap_base_info.auc_bssid);

    /* 设置 SA address2: dot11MACAddress */
    oal_set_mac_addr(puc_req_frame + WLAN_HDR_ADDR2_OFFSET, mac_mib_get_StationID(&pst_hmac_sta->st_vap_base_info));

    /* 设置 DA address3: AP MAC地址 (BSSID)*/
    oal_set_mac_addr(puc_req_frame + WLAN_HDR_ADDR3_OFFSET, pst_hmac_sta->st_vap_base_info.auc_bssid);

    puc_req_frame += MAC_80211_FRAME_LEN;

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*              Association Request Frame - Frame Body                   */
    /* --------------------------------------------------------------------- */
    /* | Capability Information | Listen Interval | SSID | Supported Rates | */
    /* --------------------------------------------------------------------- */
    /* |2                       |2                |2-34  |3-10             | */
    /* --------------------------------------------------------------------- */

    /* --------------------------------------------------------------------- */
    /* |Externed Surpported rates| Power Capability | Supported Channels   | */
    /* --------------------------------------------------------------------- */
    /* |3-257                    |4                 |4-256                 | */
    /* --------------------------------------------------------------------- */

    /* --------------------------------------------------------------------- */
    /* | RSN   | QoS Capability | HT Capabilities | Extended Capabilities  | */
    /* --------------------------------------------------------------------- */
    /* |36-256 |3               |28               |3-8                     | */
    /* --------------------------------------------------------------------- */

    /* --------------------------------------------------------------------- */
    /* | WPS   | P2P |                                                       */
    /* --------------------------------------------------------------------- */
    /* |7-257  |X    |                                                       */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    mac_set_cap_info_sta_etc((oal_void *)pst_mac_vap, puc_req_frame);
    puc_req_frame += MAC_CAP_INFO_LEN;

    /* 设置 Listen Interval IE */
    mac_set_listen_interval_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;

    /* Ressoc组帧设置Current AP address */
    if (OAL_PTR_NULL != puc_curr_bssid)
    {
        oal_set_mac_addr(puc_req_frame, puc_curr_bssid);
        puc_req_frame += OAL_MAC_ADDR_LEN;
    }
    /* 设置 SSID IE */
    mac_set_ssid_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len, WLAN_FC0_SUBTYPE_ASSOC_REQ);
    puc_req_frame += uc_ie_len;
#if  defined(_PRE_WIFI_DMT ) || (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    /* 设置 Supported Rates IE */
    mac_set_supported_rates_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;

    /* 设置 Extended Supported Rates IE */
    mac_set_exsup_rates_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;

#else
    /* 设置 Supported Rates IE */
    hmac_set_supported_rates_ie_asoc_req_etc(pst_hmac_sta, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;

    /* 设置 Extended Supported Rates IE */
    hmac_set_exsup_rates_ie_asoc_req_etc(pst_hmac_sta, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;
#endif
    /* 设置 Power Capability IE */
    mac_set_power_cap_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;

    /* 设置 Supported channel IE */
    mac_set_supported_channel_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;

#if LINUX_VERSION_CODE <= KERNEL_VERSION(2,6,34)
    if (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_wpa2)
    {
        /* 设置 RSN Capability IE */
        mac_set_rsn_ie_etc((oal_void *)pst_mac_vap, OAL_PTR_NULL, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }
    else if (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_wpa)
    {
        /* 设置 WPA Capability IE */
        mac_set_wpa_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }
#endif
    /* 填充WMM element */
    if (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_wmm_cap)
    {
        mac_set_wmm_ie_sta_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }

    /* 设置 HT Capability IE  */
    mac_set_ht_capabilities_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
#ifdef _PRE_WLAN_FEATURE_TXBF_HT
    if ((OAL_TRUE == pst_mac_vap->bit_ap_11ntxbf)
    && (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_11ntxbf)
    && (0 != uc_ie_len))
    {
        puc_req_frame += MAC_11N_TXBF_CAP_OFFSET;

        pst_txbf_cap  = (mac_txbf_cap_stru *)puc_req_frame;
        pst_txbf_cap->bit_rx_stagg_sounding             = OAL_TRUE;
        pst_txbf_cap->bit_explicit_compr_bf_fdbk        = 1;
        pst_txbf_cap->bit_compr_steering_num_bf_antssup = 1;
        pst_txbf_cap->bit_minimal_grouping              = 3;
        pst_txbf_cap->bit_chan_estimation               = 1;
        puc_req_frame -= MAC_11N_TXBF_CAP_OFFSET;
    }
#endif
    puc_req_frame += uc_ie_len;

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN) || defined(_PRE_WLAN_FEATURE_FTM) || defined(_PRE_WLAN_FEATURE_11KV_INTERFACE)
    if(OAL_PTR_NULL != pst_scaned_bss)
    {
    #ifndef _PRE_WLAN_FEATURE_11KV_INTERFACE
        en_rrm_enable = pst_hmac_sta->bit_11k_enable;
    #endif
        en_rrm_enable = en_rrm_enable && pst_scaned_bss->st_bss_dscr_info.en_support_rrm;
        if((OAL_TRUE == pst_hmac_sta->bit_11k_auth_flag) || (OAL_TRUE == en_rrm_enable))
        {
            mac_set_rrm_enabled_cap_field_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
            puc_req_frame += uc_ie_len;
        }
    }
#endif //_PRE_WLAN_FEATURE_11K

    /* 设置 Extended Capability IE */
    if (hmac_sta_check_need_set_ext_cap_ie(pst_mac_vap) == OAL_TRUE)
    {
        mac_set_ext_capabilities_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }

    /* 设置 VHT Capability IE */
    if((OAL_PTR_NULL != pst_scaned_bss) && (OAL_TRUE == pst_scaned_bss->st_bss_dscr_info.en_vht_capable)
       && (OAL_FALSE== pst_scaned_bss->st_bss_dscr_info.en_vendor_vht_capable))
    {
        mac_set_vht_capabilities_ie_etc((oal_void *) pst_mac_vap, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }
#ifdef _PRE_WLAN_FEATURE_11AX
    mac_set_he_capabilities_ie((oal_void *)pst_mac_vap,puc_req_frame,&uc_ie_len);
    puc_req_frame += uc_ie_len;
#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
        pst_hmac_user = mac_res_get_hmac_user_etc(pst_mac_vap->us_assoc_vap_id);
        if((!(pst_hmac_user->en_user_ap_type & MAC_AP_TYPE_160M_OP_MODE)) &&
           (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_opmode))
        {
            mac_set_opmode_notify_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
            puc_req_frame += uc_ie_len;
        }
#endif

#ifdef _PRE_WLAN_FEATURE_1024QAM
    if((OAL_PTR_NULL != pst_scaned_bss) && (OAL_TRUE == pst_scaned_bss->st_bss_dscr_info.en_support_1024qam))
    {
        mac_set_1024qam_vendor_ie((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_set_11ntxbf_vendor_ie_etc(pst_mac_vap, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;
#endif

    /* 填充 BCM Vendor VHT IE,解决与BCM AP的私有协议对通问题 */
    if ((OAL_PTR_NULL != pst_scaned_bss) && (OAL_TRUE == pst_scaned_bss->st_bss_dscr_info.en_vendor_vht_capable))
    {
        mac_set_vendor_vht_ie(pst_mac_vap, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }
    if((OAL_PTR_NULL != pst_scaned_bss) && (OAL_TRUE == pst_scaned_bss->st_bss_dscr_info.en_vendor_novht_capable))
    {
        mac_set_vendor_novht_ie(pst_mac_vap, puc_req_frame, &uc_ie_len);
        puc_req_frame += uc_ie_len;
    }

#ifdef _PRE_WLAN_FEATURE_11R
    if (OAL_TRUE == pst_hmac_sta->bit_11r_enable)
    {

        if (OAL_FALSE == pst_hmac_sta->bit_reassoc_flag
#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11R) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)
            || 1 == pst_hmac_sta->bit_voe_11r_auth) /*voe 11r 认证实验室环境必须携带两个mdie否则无法正常漫游*/
#else
        )
#endif
        {
            mac_set_md_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
            puc_req_frame += uc_ie_len;
        }
        else
        {/* Reasoc中包含RIC-Req */
            for (en_aci = WLAN_WME_AC_BE; en_aci < WLAN_WME_AC_BUTT; en_aci++)
            {
                if (mac_mib_get_QAPEDCATableMandatory(&pst_hmac_sta->st_vap_base_info, en_aci))
                {
                    en_target_ac = en_aci;
                    uc_tid = WLAN_WME_AC_TO_TID(en_target_ac);
                    mac_set_rde_ie_etc((oal_void *)pst_mac_vap,puc_req_frame, &uc_ie_len);
                    puc_req_frame += uc_ie_len;

                    mac_set_tspec_ie_etc((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len, uc_tid);
                    puc_req_frame += uc_ie_len;
                }
            }
        }
    }
#endif //_PRE_WLAN_FEATURE_11R
    en_app_ie_type = OAL_APP_ASSOC_REQ_IE;
#ifdef _PRE_WLAN_FEATURE_ROAM
    if (OAL_PTR_NULL != puc_curr_bssid)
    {
        en_app_ie_type = OAL_APP_REASSOC_REQ_IE;
#ifdef _PRE_WLAN_FEATURE_11R
        if(OAL_TRUE == pst_hmac_sta->bit_11r_enable)
        {
            if (OAL_TRUE == mac_mib_get_ft_trainsistion(pst_mac_vap))
            {
                en_app_ie_type = OAL_APP_FT_IE;
            }
        }
#endif //_PRE_WLAN_FEATURE_11R
    }
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_NARROW_BAND
        if ((OAL_PTR_NULL != pst_scaned_bss) && (OAL_TRUE == pst_scaned_bss->st_bss_dscr_info.en_nb_capable)
            && (OAL_TRUE == pst_mac_vap->st_nb.en_open)
                && (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_nb))
        {
            mac_set_nb_ie(puc_req_frame, &uc_ie_len);
            puc_req_frame += uc_ie_len;
        }

#endif

    /* 填充P2P/WPS IE 信息 */
    mac_add_app_ie_etc(pst_mac_vap, puc_req_frame, &us_app_ie_len, en_app_ie_type);
    puc_req_frame += us_app_ie_len;

    /* multi-sta特性下新增4地址ie */
#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
    mac_set_vender_4addr_ie((oal_void *)pst_mac_vap, puc_req_frame, &uc_ie_len);
    puc_req_frame += uc_ie_len;
#endif

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_TX, "{hmac_mgmt_encap_asoc_req_sta_etc::encap assoc req!}\r\n");
#endif

    us_asoc_rsq_len = (oal_uint32)(puc_req_frame - puc_req_frame_origin);

    return us_asoc_rsq_len;
}


oal_uint16  hmac_mgmt_encap_auth_req_etc(hmac_vap_stru *pst_hmac_sta, oal_uint8 *puc_mgmt_frame)
{
    oal_uint16      us_auth_req_len;
    hmac_user_stru *pst_user_ap;
    oal_uint16      us_auth_type;
    oal_uint32      ul_ret;
    oal_uint16      us_user_index;
#ifdef _PRE_WLAN_FEATURE_11R
    oal_uint16      us_app_ie_len;
#endif //_PRE_WLAN_FEATURE_11R

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

    mac_hdr_set_frame_control(puc_mgmt_frame, WLAN_FC0_SUBTYPE_AUTH);

    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_mgmt_frame)->auc_address1, pst_hmac_sta->st_vap_base_info.auc_bssid);

    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_mgmt_frame)->auc_address2, mac_mib_get_StationID(&pst_hmac_sta->st_vap_base_info));

    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_mgmt_frame)->auc_address3, pst_hmac_sta->st_vap_base_info.auc_bssid);
    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*              Authentication Frame (Sequence 1) - Frame Body           */
    /* --------------------------------------------------------------------  */
    /* |Auth Algorithm Number|Auth Transaction Sequence Number|Status Code|  */
    /* --------------------------------------------------------------------  */
    /* | 2                   |2                               |2          |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    if(OAL_FALSE == mac_mib_get_privacyinvoked(&pst_hmac_sta->st_vap_base_info))
    {
        /* Open System */
        puc_mgmt_frame[MAC_80211_FRAME_LEN]     = 0x00;
        puc_mgmt_frame[MAC_80211_FRAME_LEN + 1] = 0x00;
    }
    else
    {
        us_auth_type = (oal_uint16)mac_mib_get_AuthenticationMode(&pst_hmac_sta->st_vap_base_info);

        if(WLAN_WITP_AUTH_SHARED_KEY == us_auth_type)
        {
            OAM_INFO_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_encap_auth_req_etc::WLAN_WITP_AUTH_SHARED_KEY.}");
            us_auth_type = WLAN_WITP_AUTH_SHARED_KEY;
        }
        else
        {
            OAM_INFO_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_encap_auth_req_etc::WLAN_WITP_AUTH_OPEN_SYSTEM.}");
            us_auth_type = WLAN_WITP_AUTH_OPEN_SYSTEM;
        }

        puc_mgmt_frame[MAC_80211_FRAME_LEN]     = (us_auth_type & 0xFF);
        puc_mgmt_frame[MAC_80211_FRAME_LEN + 1] = ((us_auth_type & 0xFF00) >> 8);
    }

    /* 设置 Authentication Transaction Sequence Number 为 1 */
    puc_mgmt_frame[MAC_80211_FRAME_LEN + 2] = 0x01;
    puc_mgmt_frame[MAC_80211_FRAME_LEN + 3] = 0x00;

    /* 设置 Status Code 为0. 这个包的这个字段没用 . */
    puc_mgmt_frame[MAC_80211_FRAME_LEN + 4] = 0x00;
    puc_mgmt_frame[MAC_80211_FRAME_LEN + 5] = 0x00;

    /* 设置 认证帧的长度 */
    us_auth_req_len = MAC_80211_FRAME_LEN + MAC_AUTH_ALG_LEN + MAC_AUTH_TRANS_SEQ_NUM_LEN +
                   MAC_STATUS_CODE_LEN;

#ifdef _PRE_WLAN_FEATURE_11R
    if(OAL_TRUE == pst_hmac_sta->bit_11r_enable)
    {
        if ((OAL_TRUE == mac_mib_get_ft_trainsistion(&pst_hmac_sta->st_vap_base_info))
           && (MAC_VAP_STATE_ROAMING == pst_hmac_sta->st_vap_base_info.en_vap_state))
        {
            /* FT System */
            puc_mgmt_frame[MAC_80211_FRAME_LEN]     = 0x02;
            puc_mgmt_frame[MAC_80211_FRAME_LEN + 1] = 0x00;
            puc_mgmt_frame       += us_auth_req_len;

            mac_add_app_ie_etc((oal_void *)&pst_hmac_sta->st_vap_base_info, puc_mgmt_frame, &us_app_ie_len, OAL_APP_FT_IE);
            us_auth_req_len      += us_app_ie_len;
            puc_mgmt_frame       += us_app_ie_len;
        }
    }
#endif //_PRE_WLAN_FEATURE_11R

    pst_user_ap = (hmac_user_stru *)mac_res_get_hmac_user_etc(pst_hmac_sta->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_user_ap)
    {
        OAM_INFO_LOG0(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_mgmt_encap_auth_req_etc::no present ap, alloc new ap.}");
        ul_ret = hmac_user_add_etc(&pst_hmac_sta->st_vap_base_info, pst_hmac_sta->st_vap_base_info.auc_bssid, &us_user_index);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_hmac_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                             "{hmac_mgmt_encap_auth_req_etc::hmac_user_add_etc failed[%d].}", ul_ret);
            us_auth_req_len = 0;
        }
    }

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_TX, "{hmac_mgmt_encap_auth_req_etc::encap auth req!}\r\n");
#endif

    return us_auth_req_len;
}


oal_uint16  hmac_mgmt_encap_auth_req_seq3_etc(hmac_vap_stru *pst_sta, oal_uint8 *puc_mgmt_frame, oal_uint8 *puc_mac_hrd)
{
    oal_uint8  *puc_data       = OAL_PTR_NULL;
    oal_uint16  us_index        = 0;
    oal_uint16  us_auth_req_len = 0;
    oal_uint8  *puc_ch_text     = OAL_PTR_NULL;
    oal_uint8   uc_ch_text_len  = 0;

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

    mac_hdr_set_frame_control(puc_mgmt_frame, WLAN_FC0_SUBTYPE_AUTH);

    /* 将帧保护字段置1 */
    mac_set_wep(puc_mgmt_frame, 1);

    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_mgmt_frame)->auc_address1, pst_sta->st_vap_base_info.auc_bssid);

    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_mgmt_frame)->auc_address2, mac_mib_get_StationID(&pst_sta->st_vap_base_info));

    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_mgmt_frame)->auc_address3, pst_sta->st_vap_base_info.auc_bssid);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*              Authentication Frame (Sequence 3) - Frame Body           */
    /* --------------------------------------------------------------------- */
    /* |Auth Algo Number|Auth Trans Seq Number|Status Code| Challenge Text | */
    /* --------------------------------------------------------------------- */
    /* | 2              |2                    |2          | 3 - 256        | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/

    /* 获取认证帧payload */
    us_index = MAC_80211_FRAME_LEN;
    puc_data = (oal_uint8 *)(puc_mgmt_frame + us_index);

    /* 设置 认证帧的长度 */
    us_auth_req_len = MAC_80211_FRAME_LEN + MAC_AUTH_ALG_LEN + MAC_AUTH_TRANS_SEQ_NUM_LEN +
                      MAC_STATUS_CODE_LEN;

    /* In case of no failure, the frame must be WEP encrypted. 4 bytes must  */
    /* be   left for the  IV  in  that  case. These   fields will  then  be  */
    /* reinitialized, using the correct index, with offset for IV field.     */
    puc_data[0] = WLAN_WITP_AUTH_SHARED_KEY;    /* Authentication Algorithm Number               */
    puc_data[1] = 0x00;

    puc_data[2] = 0x03;                    /* Authentication Transaction Sequence Number    */
    puc_data[3] = 0x00;

    /* If WEP subfield in the  incoming  authentication frame is 1,  respond */
    /* with  'challenge text failure' status,  since the STA does not expect */
    /* an encrypted frame in this state.                                     */
    if(1 == mac_is_protectedframe(puc_mac_hrd))
    {
        puc_data[4] = MAC_CHLNG_FAIL;
        puc_data[5] = 0x00;
    }

    /* If the STA does not support WEP, respond with 'unsupported algo'      */
    /* status, since WEP is necessary for Shared Key Authentication.         */
    else if(OAL_FALSE == mac_is_wep_enabled(&(pst_sta->st_vap_base_info)))
    {
        puc_data[4] = MAC_UNSUPT_ALG;
        puc_data[5] = 0x00;
    }

    /* If the default WEP key is NULL, respond with 'challenge text failure' */
    /* status, since a NULL key value cannot be used for WEP operations.     */
    else if(mac_get_wep_default_keysize(&pst_sta->st_vap_base_info) == 0)
    {
        puc_data[4] = MAC_CHLNG_FAIL;
        puc_data[5] = 0x00;
    }

    /* If there is a mapping in dot11WEPKeyMappings matching the address of  */
    /* the AP, and the corresponding key is NULL respond with 'challenge     */
    /* text failure' status. This is currently not being used.               */

    /* No error condition detected */
    else
    {

        /* Set Status Code to 'success' */
        puc_data[4] = MAC_SUCCESSFUL_STATUSCODE;
        puc_data[5] = 0x00;

        /* Extract 'Challenge Text' and its 'length' from the incoming       */
        /* authentication frame                                              */
        uc_ch_text_len = puc_mac_hrd[MAC_80211_FRAME_LEN + 7];
        puc_ch_text     = (oal_uint8 *)(&puc_mac_hrd[MAC_80211_FRAME_LEN + 8]);

        /* Challenge Text Element                  */
        /* --------------------------------------- */
        /* |Element ID | Length | Challenge Text | */
        /* --------------------------------------- */
        /* | 1         |1       |1 - 253         | */
        /* --------------------------------------- */

        puc_mgmt_frame[us_index + 6]   = MAC_EID_CHALLENGE;
        puc_mgmt_frame[us_index + 7]   = uc_ch_text_len;
        oal_memcopy(&puc_mgmt_frame[us_index + 8], puc_ch_text, uc_ch_text_len);

        /* Add the challenge text element length to the authentication       */
        /* request frame length. The IV, ICV element lengths will be added   */
        /* after encryption.                                                 */
        us_auth_req_len += (uc_ch_text_len + MAC_IE_HDR_LEN);
    }

#ifdef _PRE_DEBUG_MODE
    OAM_WARNING_LOG0(0, OAM_SF_TX, "{hmac_mgmt_encap_auth_req_seq3_etc::encap auth req seq3!}\r\n");
#endif

    return us_auth_req_len;
}


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

