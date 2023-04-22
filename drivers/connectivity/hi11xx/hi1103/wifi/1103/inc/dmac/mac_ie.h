

#ifndef __MAC_IE_H__
#define __MAC_IE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_spec.h"
#include "mac_frame.h"
#include "wlan_mib.h"
#include "wlan_types.h"
#include "mac_user.h"
#include "mac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_IE_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define MAC_IE_REAMIN_LEN_IS_ENOUGH(puc_src_ie, puc_curr_ie, uc_ie_len, uc_remain_len) \
        (((((puc_curr_ie) - (puc_src_ie)) + (uc_remain_len)) <= (uc_ie_len)) ? OAL_TRUE : OAL_FALSE)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
typedef oal_void (*opmode_field)(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, mac_opmode_notify_stru *pst_opmode_notify);

typedef oal_uint32  (*vht_opern)(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload, mac_user_stru *pst_mac_user, oal_uint8 *pst_ch);
typedef oal_uint32  (*ht_opern)(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload, mac_user_stru *pst_mac_user, oal_uint32 *pst_ch);

typedef struct
{
    opmode_field opmode_field_cb;
    vht_opern    vht_opern_cb;
    ht_opern     ht_opern_cb;

}mac_ie_cb;
extern mac_ie_cb g_st_mac_ie_rom_cb;
/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 inline函数定义
*****************************************************************************/

OAL_STATIC OAL_INLINE wlan_channel_bandwidth_enum_uint8  mac_get_bandwidth_from_sco(mac_sec_ch_off_enum_uint8 en_sec_chan_offset)
{
    switch (en_sec_chan_offset)
    {
        case MAC_SCA:   /* Secondary Channel Above */
            return WLAN_BAND_WIDTH_40PLUS;

        case MAC_SCB:   /* Secondary Channel Below */
            return WLAN_BAND_WIDTH_40MINUS;

        default:        /* No Secondary Channel    */
            return WLAN_BAND_WIDTH_20M;
    }
}


OAL_STATIC OAL_INLINE mac_sec_ch_off_enum_uint8  mac_get_sco_from_bandwidth(wlan_channel_bandwidth_enum_uint8 en_bandwidth)
{
    switch (en_bandwidth)
    {
        case WLAN_BAND_WIDTH_40PLUS:
        case WLAN_BAND_WIDTH_80PLUSPLUS:
        case WLAN_BAND_WIDTH_80PLUSMINUS:
            return MAC_SCA;

        case WLAN_BAND_WIDTH_40MINUS:
        case WLAN_BAND_WIDTH_80MINUSPLUS:
        case WLAN_BAND_WIDTH_80MINUSMINUS:
            return MAC_SCB;

        default:
            return MAC_SCN;
    }
}


OAL_STATIC OAL_INLINE wlan_channel_bandwidth_enum_uint8  mac_get_bandwith_from_center_freq_seg0(oal_uint8 uc_chan_width, oal_uint8 uc_channel, oal_uint8 uc_chan_center_freq)
{
    /* 80+80不支持，暂按80M处理 */
    if (WLAN_MIB_VHT_OP_WIDTH_80 & uc_chan_width)
    {
        switch (uc_chan_center_freq - uc_channel)
        {
            case 6:
                /***********************************************************************
                | 主20 | 从20 | 从40       |
                              |
                              |中心频率相对于主20偏6个信道
                ************************************************************************/
                return WLAN_BAND_WIDTH_80PLUSPLUS;

            case -2:
                /***********************************************************************
                | 从40        | 主20 | 从20 |
                              |
                              |中心频率相对于主20偏-2个信道
                ************************************************************************/
                return WLAN_BAND_WIDTH_80PLUSMINUS;

            case 2:
                /***********************************************************************
                | 从20 | 主20 | 从40       |
                              |
                              |中心频率相对于主20偏2个信道
                ************************************************************************/
                return  WLAN_BAND_WIDTH_80MINUSPLUS;

            case -6:
                /***********************************************************************
                | 从40        | 从20 | 主20 |
                              |
                              |中心频率相对于主20偏-6个信道
                ************************************************************************/
                return WLAN_BAND_WIDTH_80MINUSMINUS;

            default:
                return WLAN_BAND_WIDTH_20M;
        }
    }
#ifdef _PRE_WLAN_FEATURE_160M
    else if (WLAN_MIB_VHT_OP_WIDTH_160 == uc_chan_width)
    {
        switch (uc_chan_center_freq - uc_channel)
        {
            case 14:
                return WLAN_BAND_WIDTH_160PLUSPLUSPLUS;

            case 10:
                return WLAN_BAND_WIDTH_160MINUSPLUSPLUS;

            case 6:
                return WLAN_BAND_WIDTH_160PLUSMINUSPLUS;

            case 2:
                return WLAN_BAND_WIDTH_160MINUSMINUSPLUS;

            case -2:
                return WLAN_BAND_WIDTH_160PLUSPLUSMINUS;

            case -6:
                return WLAN_BAND_WIDTH_160MINUSPLUSMINUS;

            case -10:
                return WLAN_BAND_WIDTH_160PLUSMINUSMINUS;

            case -14:
                return WLAN_BAND_WIDTH_160MINUSMINUSMINUS;

            default:
                return WLAN_BAND_WIDTH_20M;
        }
    }
#endif
    else
    {
        return WLAN_BAND_WIDTH_20M;
    }
}

OAL_STATIC OAL_INLINE wlan_channel_bandwidth_enum_uint8  mac_get_bandwith_from_center_freq_seg0_seg1(oal_uint8 uc_chan_width, oal_uint8 uc_channel, oal_uint8 uc_chan_center_freq0, oal_uint8 uc_chan_center_freq1)
{
    /* 80+80不支持，暂按80M处理 */
    if (WLAN_MIB_VHT_OP_WIDTH_80 & uc_chan_width)
    {
#ifdef _PRE_WLAN_FEATURE_160M
        if((uc_chan_center_freq1 - uc_chan_center_freq0 == 8) || (uc_chan_center_freq0 - uc_chan_center_freq1 == 8))
        {
            switch (uc_chan_center_freq1 - uc_channel)
            {
                case 14:
                    return WLAN_BAND_WIDTH_160PLUSPLUSPLUS;

                case 10:
                    return WLAN_BAND_WIDTH_160MINUSPLUSPLUS;

                case 6:
                    return WLAN_BAND_WIDTH_160PLUSMINUSPLUS;

                case 2:
                    return WLAN_BAND_WIDTH_160MINUSMINUSPLUS;

                case -2:
                    return WLAN_BAND_WIDTH_160PLUSPLUSMINUS;

                case -6:
                    return WLAN_BAND_WIDTH_160MINUSPLUSMINUS;

                case -10:
                    return WLAN_BAND_WIDTH_160PLUSMINUSMINUS;

                case -14:
                    return WLAN_BAND_WIDTH_160MINUSMINUSMINUS;

                default:
                    return WLAN_BAND_WIDTH_20M;
            }
        }
#endif

        switch (uc_chan_center_freq0 - uc_channel)
        {
            case 6:
                /***********************************************************************
                | 主20 | 从20 | 从40       |
                              |
                              |中心频率相对于主20偏6个信道
                ************************************************************************/
                return WLAN_BAND_WIDTH_80PLUSPLUS;

            case -2:
                /***********************************************************************
                | 从40        | 主20 | 从20 |
                              |
                              |中心频率相对于主20偏-2个信道
                ************************************************************************/
                return WLAN_BAND_WIDTH_80PLUSMINUS;

            case 2:
                /***********************************************************************
                | 从20 | 主20 | 从40       |
                              |
                              |中心频率相对于主20偏2个信道
                ************************************************************************/
                return  WLAN_BAND_WIDTH_80MINUSPLUS;

            case -6:
                /***********************************************************************
                | 从40        | 从20 | 主20 |
                              |
                              |中心频率相对于主20偏-6个信道
                ************************************************************************/
                return WLAN_BAND_WIDTH_80MINUSMINUS;

            default:
                return WLAN_BAND_WIDTH_20M;
        }
    }
#ifdef _PRE_WLAN_FEATURE_160M
    else if (WLAN_MIB_VHT_OP_WIDTH_160 == uc_chan_width)
    {
        if((uc_chan_center_freq1 - uc_chan_center_freq0 == 8) || (uc_chan_center_freq0 - uc_chan_center_freq1 == 8))
        {
            switch (uc_chan_center_freq1 - uc_channel)
            {
                case 14:
                    return WLAN_BAND_WIDTH_160PLUSPLUSPLUS;

                case 10:
                    return WLAN_BAND_WIDTH_160MINUSPLUSPLUS;

                case 6:
                    return WLAN_BAND_WIDTH_160PLUSMINUSPLUS;

                case 2:
                    return WLAN_BAND_WIDTH_160MINUSMINUSPLUS;

                case -2:
                    return WLAN_BAND_WIDTH_160PLUSPLUSMINUS;

                case -6:
                    return WLAN_BAND_WIDTH_160MINUSPLUSMINUS;

                case -10:
                    return WLAN_BAND_WIDTH_160PLUSMINUSMINUS;

                case -14:
                    return WLAN_BAND_WIDTH_160MINUSMINUSMINUS;

                default:
                    return WLAN_BAND_WIDTH_20M;
            }
        }
        else
        {
            switch (uc_chan_center_freq0 - uc_channel)
            {
                case 14:
                    return WLAN_BAND_WIDTH_160PLUSPLUSPLUS;

                case 10:
                    return WLAN_BAND_WIDTH_160MINUSPLUSPLUS;

                case 6:
                    return WLAN_BAND_WIDTH_160PLUSMINUSPLUS;

                case 2:
                    return WLAN_BAND_WIDTH_160MINUSMINUSPLUS;

                case -2:
                    return WLAN_BAND_WIDTH_160PLUSPLUSMINUS;

                case -6:
                    return WLAN_BAND_WIDTH_160MINUSPLUSMINUS;

                case -10:
                    return WLAN_BAND_WIDTH_160PLUSMINUSMINUS;

                case -14:
                    return WLAN_BAND_WIDTH_160MINUSMINUSMINUS;

                default:
                    return WLAN_BAND_WIDTH_20M;
            }
        }
    }
#endif
    else
    {
        return WLAN_BAND_WIDTH_20M;
    }
}

/*****************************************************************************
  11 函数声明
*****************************************************************************/

#ifdef _PRE_WLAN_FEATURE_SMPS
extern oal_uint32 mac_smps_update_user_status(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
#endif
extern  wlan_mib_mimo_power_save_enum_uint8 mac_ie_proc_sm_power_save_field_etc(mac_user_stru *pst_mac_user ,oal_uint8 uc_smps);

extern  oal_bool_enum_uint8  mac_ie_proc_ht_green_field_etc(
                                        mac_user_stru    *pst_mac_user_sta ,
                                        mac_vap_stru     *pst_mac_vap,
                                        oal_uint8         uc_ht_green_field,
                                        oal_bool_enum     en_prev_asoc_ht);

extern oal_bool_enum_uint8  mac_ie_proc_ht_supported_channel_width_etc(
                                        mac_user_stru    *pst_mac_user_sta ,
                                        mac_vap_stru     *pst_mac_vap,
                                        oal_uint8         uc_supported_channel_width,
                                        oal_bool_enum     en_prev_asoc_ht);

extern oal_bool_enum_uint8  mac_ie_proc_lsig_txop_protection_support_etc(
                                        mac_user_stru    *pst_mac_user_sta,
                                        mac_vap_stru     *pst_mac_vap,
                                        oal_uint8         uc_lsig_txop_protection_support,
                                        oal_bool_enum     en_prev_asoc_ht);
extern oal_uint32  mac_ie_proc_ext_cap_ie_etc(mac_user_stru *pst_mac_user, oal_uint8 *puc_payload);
extern oal_uint8  mac_ie_get_chan_num_etc(
                                    oal_uint8   *puc_frame_body,
                                    oal_uint16   us_frame_len,
                                    oal_uint16   us_offset,
                                    oal_uint8    us_curr_chan);

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
extern oal_uint32  mac_ie_proc_chwidth_field_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,oal_uint8 uc_chwidth);
#endif

#if 0
extern oal_uint32  mac_set_channel_switch_wrapper_ie(
                oal_uint8                            uc_channel,
                wlan_channel_bandwidth_enum_uint8    en_bw,
                oal_uint8                           *pauc_buffer,
                oal_uint8                           *puc_output_len);
#endif
extern oal_uint32  mac_set_second_channel_offset_ie_etc(
                wlan_channel_bandwidth_enum_uint8    en_bw,
                oal_uint8                           *pauc_buffer,
                oal_uint8                           *puc_output_len);
extern oal_uint32  mac_set_11ac_wideband_ie_etc(
                oal_uint8                            uc_channel,
                wlan_channel_bandwidth_enum_uint8    en_bw,
                oal_uint8                           *pauc_buffer,
                oal_uint8                           *puc_output_len);
extern oal_bool_enum_uint8 mac_ie_check_p2p_action_etc(oal_uint8 *puc_payload);
extern oal_uint32 mac_ie_check_rsn_cipher_format(oal_uint8 *puc_src_ie, oal_uint8 uc_ie_len);
extern oal_uint32  mac_ie_proc_ht_sta_etc(
                   mac_vap_stru            *pst_mac_sta,
                   oal_uint8                *puc_payload,
                   oal_uint16                us_offset,
                   mac_user_stru           *pst_mac_user_ap,
                   oal_uint16               *pus_amsdu_maxsize);

extern oal_uint32  mac_ie_proc_obss_scan_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload);
#ifdef _PRE_WLAN_FEATURE_11AX
extern oal_uint32  mac_ie_proc_he_opern_ie(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload, mac_user_stru *pst_mac_user);
extern oal_uint32  mac_ie_parse_he_cap(oal_uint8 *puc_he_cap_ie,mac_frame_he_cap_ie_stru *pst_he_cap_value);
extern oal_uint32  mac_ie_parse_he_oper(oal_uint8 *puc_he_oper_ie,mac_frame_he_oper_ie_stru *pst_he_oper_ie_value);
extern oal_uint32  mac_ie_parse_mu_edca_parameter(oal_uint8 *puc_he_edca_ie,mac_frame_he_mu_edca_parameter_ie_stru *pst_he_mu_edca_value);
extern oal_uint32 mac_ie_parse_spatial_reuse_parameter(oal_uint8 *puc_he_cap_ie,mac_frame_he_spatial_reuse_parameter_set_ie_stru *pst_he_srp_value);
extern oal_uint32  mac_ie_parse_he_bss_color_change_announcement_ie(oal_uint8 *puc_payload, mac_frame_bss_color_change_annoncement_ie_stru *pst_bss_color);

#endif
extern oal_uint32  mac_proc_ht_opern_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload, mac_user_stru *pst_mac_user);
extern oal_uint32  mac_ie_proc_vht_opern_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload, mac_user_stru *pst_mac_user);

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
extern oal_uint32  mac_ie_proc_opmode_field_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, mac_opmode_notify_stru *pst_opmode_notify);
extern oal_uint32  mac_check_is_assoc_frame_etc(oal_uint8 uc_mgmt_frm_type);
#endif
oal_uint32 mac_ie_get_wpa_cipher(oal_uint8 *puc_ie, mac_crypto_settings_stru *pst_crypto);
oal_uint32 mac_ie_get_rsn_cipher(oal_uint8 *puc_ie, mac_crypto_settings_stru *pst_crypto);
extern oal_uint8  *mac_ie_find_vendor_vht_ie(oal_uint8 *puc_frame, oal_uint16 us_frame_len);
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of mac_ie.h */
