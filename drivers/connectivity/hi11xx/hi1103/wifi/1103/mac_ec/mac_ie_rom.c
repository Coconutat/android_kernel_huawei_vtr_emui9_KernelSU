


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_ie.h"
#include "mac_frame.h"
#include "mac_device.h"
//#include "dmac_chan_mgmt.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_IE_ROM_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
mac_ie_cb g_st_mac_ie_rom_cb = {OAL_PTR_NULL,
                                OAL_PTR_NULL,
                                OAL_PTR_NULL};


/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_uint8  mac_ie_get_chan_num_etc(oal_uint8 *puc_frame_body, oal_uint16 us_frame_len, oal_uint16 us_offset,oal_uint8 uc_curr_chan)
{
    oal_uint8   uc_chan_num = 0;
    oal_uint8  *puc_ie_start_addr;

    /* 在DSSS Param set ie中解析chan num */
    puc_ie_start_addr = mac_find_ie_etc(MAC_EID_DSPARMS, puc_frame_body + us_offset, us_frame_len - us_offset);
    if ((OAL_PTR_NULL != puc_ie_start_addr) && (puc_ie_start_addr[1] == MAC_DSPARMS_LEN))
    {
        uc_chan_num = puc_ie_start_addr[2];
        if (mac_is_channel_num_valid_etc(mac_get_band_by_channel_num(uc_chan_num), uc_chan_num) == OAL_SUCC)
        {
            return  uc_chan_num;
        }
    }

    /* 在HT operation ie中解析 chan num */
    puc_ie_start_addr = mac_find_ie_etc(MAC_EID_HT_OPERATION, puc_frame_body + us_offset, us_frame_len - us_offset);

    if ((OAL_PTR_NULL != puc_ie_start_addr) && (puc_ie_start_addr[1] >= 1))
    {
        uc_chan_num = puc_ie_start_addr[2];
        if (mac_is_channel_num_valid_etc(mac_get_band_by_channel_num(uc_chan_num), uc_chan_num) == OAL_SUCC)
        {
            return  uc_chan_num;
        }
    }

    uc_chan_num = uc_curr_chan;
    return uc_chan_num;
}
#if 0

oal_uint32  mac_set_channel_switch_wrapper_ie(
                oal_uint8                            uc_channel,
                wlan_channel_bandwidth_enum_uint8    en_bw,
                oal_uint8                           *pauc_buffer,
                oal_uint8                           *puc_output_len)
{
    oal_bool_enum_uint8  en_need_wideband_sub_ie;
    oal_uint8            uc_total_len = 0, uc_sub_len;
    oal_uint8           *puc_wrapper_ie_len;

    if ((OAL_PTR_NULL == pauc_buffer) || (OAL_PTR_NULL == puc_output_len))
    {

        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{mac_set_channel_switch_wrapper_ie::null param.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 默认输出为空 */
    *pauc_buffer    = '\0';
    *puc_output_len = 0;

    /* 11ac 设置Channel Switch Wrapper IE                             */
    /******************************************************************/
    /* -------------------------------------------------------------- */
    /* |ID |Length |New Country IE|Wideband IE |VHT power IE          */
    /* -------------------------------------------------------------- */
    /* |1  |1      |None          |5           |None                  */
    /*                                                                */
    /******************************************************************/
    pauc_buffer[0] = 196;
    pauc_buffer[1] = 0;
    puc_wrapper_ie_len = &pauc_buffer[1];
    pauc_buffer += 2;

    /* COUNTRY SUB ELEMENT --- N/A                       */
    /* 当前的信道切换仅考虑由DFS触发，不会导致管制域切换 */

    /* WIDEBAND SUB ELEMENT  */
    /* 仅对20M以上带宽才有效 */
    en_need_wideband_sub_ie = OAL_TRUE;
    uc_sub_len = 0;
    if (WLAN_BAND_WIDTH_20M == en_bw)
    {
        en_need_wideband_sub_ie = OAL_FALSE;
    }

    if (OAL_TRUE == en_need_wideband_sub_ie)
    {
        uc_sub_len = 0;
        /* 填写Wideband 子IE */
        pauc_buffer[0] = 194;
        pauc_buffer[1] = 3;
        switch(en_bw)
        {
            case WLAN_BAND_WIDTH_40PLUS:
                pauc_buffer[2] = 0;
                pauc_buffer[3] = uc_channel + 2;
                break;

            case WLAN_BAND_WIDTH_40MINUS:
                pauc_buffer[2] = 0;
                pauc_buffer[3] = uc_channel - 2;
                break;

            case WLAN_BAND_WIDTH_80PLUSPLUS:
                pauc_buffer[2] = 1;
                pauc_buffer[3] = uc_channel + 6;
                break;

            case WLAN_BAND_WIDTH_80PLUSMINUS:
                pauc_buffer[2] = 1;
                pauc_buffer[3] = uc_channel - 2;
                break;

            case WLAN_BAND_WIDTH_80MINUSPLUS:
                pauc_buffer[2] = 1;
                pauc_buffer[3] = uc_channel + 2;
                break;

            case WLAN_BAND_WIDTH_80MINUSMINUS:
                pauc_buffer[2] = 1;
                pauc_buffer[3] = uc_channel - 6;
                break;

            default:
                OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{mac_set_channel_switch_wrapper_ie::invalid bandwidth.}");

                return OAL_FAIL;
        }

        pauc_buffer[4] = 0; /* reserved. Not support 80M + 80M */

        uc_sub_len = 5;
    }
    pauc_buffer += uc_sub_len;
    uc_total_len += uc_sub_len;

    /* VHT POWER SUB ELEMENT --- N/A  */
    /* 目前的切换不会导致功率改变     */

    /* 回填WRAPPER IE LEN */
    *puc_wrapper_ie_len = uc_total_len;

    *puc_output_len = uc_total_len + 2;

    return OAL_SUCC;
}
#endif



oal_uint32  mac_set_second_channel_offset_ie_etc(
                wlan_channel_bandwidth_enum_uint8    en_bw,
                oal_uint8                           *pauc_buffer,
                oal_uint8                           *puc_output_len)
{
    if ((OAL_PTR_NULL == pauc_buffer) || (OAL_PTR_NULL == puc_output_len))
    {

        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{mac_set_second_channel_offset_ie_etc::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 默认输出为空 */
    *pauc_buffer    = '\0';
    *puc_output_len = 0;

    /* 11n 设置Secondary Channel Offset Element */
    /******************************************************************/
    /* -------------------------------------------------------------- */
    /* |Ele. ID |Length |Secondary channel offset |                   */
    /* -------------------------------------------------------------- */
    /* |1       |1      |1                        |                   */
    /*                                                                */
    /******************************************************************/
    pauc_buffer[0] = 62;
    pauc_buffer[1] = 1;

    switch(en_bw)
    {
        case WLAN_BAND_WIDTH_20M:
            pauc_buffer[2] = 0;  /* no secondary channel */
            break;

        case WLAN_BAND_WIDTH_40PLUS:
        case WLAN_BAND_WIDTH_80PLUSPLUS:
        case WLAN_BAND_WIDTH_80PLUSMINUS:
    #ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BAND_WIDTH_160PLUSPLUSPLUS:
        case WLAN_BAND_WIDTH_160PLUSPLUSMINUS:
        case WLAN_BAND_WIDTH_160PLUSMINUSPLUS:
        case WLAN_BAND_WIDTH_160PLUSMINUSMINUS:
    #endif
            pauc_buffer[2] = 1;  /* secondary 20M channel above */
            break;

        case WLAN_BAND_WIDTH_40MINUS:
        case WLAN_BAND_WIDTH_80MINUSPLUS:
        case WLAN_BAND_WIDTH_80MINUSMINUS:
    #ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BAND_WIDTH_160MINUSPLUSPLUS:
        case WLAN_BAND_WIDTH_160MINUSPLUSMINUS:
        case WLAN_BAND_WIDTH_160MINUSMINUSPLUS:
        case WLAN_BAND_WIDTH_160MINUSMINUSMINUS:
    #endif
            pauc_buffer[2] = 3;  /* secondary 20M channel below */
            break;

        default:
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{mac_set_second_channel_offset_ie_etc::invalid bandwidth[%d].}", en_bw);

            return OAL_FAIL;
    }

    *puc_output_len = 3;

    return OAL_SUCC;
}


oal_uint32  mac_set_11ac_wideband_ie_etc(
                oal_uint8                            uc_channel,
                wlan_channel_bandwidth_enum_uint8    en_bw,
                oal_uint8                           *pauc_buffer,
                oal_uint8                           *puc_output_len)
{
    if ((OAL_PTR_NULL == pauc_buffer) || (OAL_PTR_NULL == puc_output_len))
    {

        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{mac_set_11ac_wideband_ie_etc::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 默认输出为空 */
    *pauc_buffer     = '\0';
    *puc_output_len  = 0;

    /* 11ac 设置Wide Bandwidth Channel Switch Element                 */
    /******************************************************************/
    /* -------------------------------------------------------------- */
    /* |ID |Length |New Ch width |Center Freq seg1 |Center Freq seg2  */
    /* -------------------------------------------------------------- */
    /* |1  |1      |1            |1                |1                 */
    /*                                                                */
    /******************************************************************/
    pauc_buffer[0] = 194;
    pauc_buffer[1] = 3;
    switch(en_bw)
    {
        case WLAN_BAND_WIDTH_20M:
        case WLAN_BAND_WIDTH_40PLUS:
        case WLAN_BAND_WIDTH_40MINUS:
            pauc_buffer[2] = 0;
            pauc_buffer[3] = 0;
            break;

        case WLAN_BAND_WIDTH_80PLUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 6;
            break;

        case WLAN_BAND_WIDTH_80PLUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 6;
            break;

#ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BAND_WIDTH_160PLUSPLUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 6;
            pauc_buffer[4] = uc_channel + 14;
        break;

        case WLAN_BAND_WIDTH_160PLUSPLUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 6;
            pauc_buffer[4] = uc_channel - 2;
        break;

        /* 从20信道+1, 从40信道-1, 从80信道+1 */
        case WLAN_BAND_WIDTH_160PLUSMINUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 2;
            pauc_buffer[4] = uc_channel + 6;
        break;

        /* 从20信道+1, 从40信道-1, 从80信道-1 */
        case WLAN_BAND_WIDTH_160PLUSMINUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 2;
            pauc_buffer[4] = uc_channel - 10;
        break;

        /* 从20信道-1, 从40信道+1, 从80信道+1 */
        case WLAN_BAND_WIDTH_160MINUSPLUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 2;
            pauc_buffer[4] = uc_channel + 10;
        break;

        /* 从20信道-1, 从40信道+1, 从80信道-1 */
        case WLAN_BAND_WIDTH_160MINUSPLUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel + 2;
            pauc_buffer[4] = uc_channel - 6;
        break;

        /* 从20信道-1, 从40信道-1, 从80信道+1 */
        case WLAN_BAND_WIDTH_160MINUSMINUSPLUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 6;
            pauc_buffer[4] = uc_channel + 2;
        break;

        /* 从20信道-1, 从40信道-1, 从80信道-1 */
        case WLAN_BAND_WIDTH_160MINUSMINUSMINUS:
            pauc_buffer[2] = 1;
            pauc_buffer[3] = uc_channel - 6;
            pauc_buffer[4] = uc_channel - 14;
        break;
#endif

        default:
            OAM_ERROR_LOG1(0, OAM_SF_SCAN, "{mac_set_11ac_wideband_ie_etc::invalid bandwidth[%d].}", en_bw);

            return OAL_FAIL;
    }

    if((en_bw < WLAN_BAND_WIDTH_160PLUSPLUSPLUS) || (en_bw > WLAN_BAND_WIDTH_160MINUSMINUSMINUS))
    {
        pauc_buffer[4] = 0; /* reserved. Not support 80M + 80M */
    }

    *puc_output_len = 5;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_uint32  mac_ie_proc_chwidth_field_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user,oal_uint8 uc_chwidth)
{
    wlan_bw_cap_enum_uint8      en_bwcap_vap = 0;        /* vap自身带宽能力 */
    wlan_bw_cap_enum_uint8      en_bwcap_user = 0;       /* user之前的带宽信息 */

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_user) || (OAL_PTR_NULL == pst_mac_vap)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_2040, "{mac_ie_proc_opmode_field_etc::pst_mac_user = [%x], pst_opmode_notify = [%x], pst_mac_vap = [%x]!}\r\n",
                       pst_mac_user, pst_mac_vap);
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_bwcap_user = pst_mac_user->en_avail_bandwidth;

    mac_vap_get_bandwidth_cap_etc(pst_mac_vap, &en_bwcap_vap);
    en_bwcap_vap = OAL_MIN(en_bwcap_vap, (wlan_bw_cap_enum_uint8)uc_chwidth);
    mac_user_set_bandwidth_info_etc(pst_mac_user, en_bwcap_vap, en_bwcap_vap);

    //l00311403TODO
    if (en_bwcap_user != pst_mac_user->en_avail_bandwidth)
    {
        /* 调用算法钩子函数 */
        //后面需要抛事件到dmac, dmac_alg_cfg_user_spatial_stream_notify(pst_mac_user);
    }

    return OAL_SUCC;
}
#endif


oal_uint32  mac_proc_ht_opern_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload, mac_user_stru *pst_mac_user)
{
    mac_ht_opern_stru           *pst_ht_opern;
    mac_user_ht_hdl_stru         st_ht_hdl;
    oal_uint32                   ul_change = MAC_NO_CHANGE;
    mac_sec_ch_off_enum_uint8    uc_secondary_chan_offset_old;
    oal_uint8                    uc_sta_chan_width_old;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_payload) || (OAL_PTR_NULL == pst_mac_user)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_proc_ht_opern_ie_etc::param null.}");
        return ul_change;
    }

    /* 长度校验，此处仅用到前6字节，后面Basic MCS Set未涉及 */
    if (puc_payload[1] < 6)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{mac_proc_ht_opern_ie_etc::invalid ht opern ie len[%d].}", puc_payload[1]);
        return ul_change;
    }

    mac_user_get_ht_hdl_etc(pst_mac_user, &st_ht_hdl);

    uc_secondary_chan_offset_old = st_ht_hdl.bit_secondary_chan_offset;
    uc_sta_chan_width_old        = st_ht_hdl.bit_sta_chan_width;

    /************************ HT Operation Element *************************************
      ----------------------------------------------------------------------
      |EID |Length |PrimaryChannel |HT Operation Information |Basic MCS Set|
      ----------------------------------------------------------------------
      |1   |1      |1              |5                        |16           |
      ----------------------------------------------------------------------
    ***************************************************************************/

    /************************ HT Information Field ****************************
     |--------------------------------------------------------------------|
     | Primary | Seconday  | STA Ch | RIFS |           reserved           |
     | Channel | Ch Offset | Width  | Mode |                              |
     |--------------------------------------------------------------------|
     |    1    | B0     B1 |   B2   |  B3  |    B4                     B7 |
     |--------------------------------------------------------------------|

     |----------------------------------------------------------------|
     |     HT     | Non-GF STAs | resv      | OBSS Non-HT  | Reserved |
     | Protection |   Present   |           | STAs Present |          |
     |----------------------------------------------------------------|
     | B0     B1  |     B2      |    B3     |     B4       | B5   B15 |
     |----------------------------------------------------------------|

     |-------------------------------------------------------------|
     | Reserved |  Dual  |  Dual CTS  | Seconday | LSIG TXOP Protn |
     |          | Beacon | Protection |  Beacon  | Full Support    |
     |-------------------------------------------------------------|
     | B0    B5 |   B6   |     B7     |     B8   |       B9        |
     |-------------------------------------------------------------|

     |---------------------------------------|
     |  PCO   |  PCO  | Reserved | Basic MCS |
     | Active | Phase |          |    Set    |
     |---------------------------------------|
     |  B10   |  B11  | B12  B15 |    16     |
     |---------------------------------------|
    **************************************************************************/

    pst_ht_opern  = (mac_ht_opern_stru *)(&puc_payload[MAC_IE_HDR_LEN]);

    /* 提取HT Operation IE中的"Secondary Channel Offset" */
    st_ht_hdl.bit_secondary_chan_offset = pst_ht_opern->bit_secondary_chan_offset;

    /* 在2.4G用户声称20M情况下该变量不切换 */
    if ((WLAN_BAND_WIDTH_20M == pst_ht_opern->bit_sta_chan_width) && (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band))
    {
        st_ht_hdl.bit_secondary_chan_offset = MAC_SCN;
    }

    /* 保护相关 */
    st_ht_hdl.bit_rifs_mode                         = pst_ht_opern->bit_rifs_mode;/*发送描述符填写时候需要此值*/
    st_ht_hdl.bit_HT_protection                     = pst_ht_opern->bit_HT_protection;
    st_ht_hdl.bit_nongf_sta_present                 = pst_ht_opern->bit_nongf_sta_present;
    st_ht_hdl.bit_obss_nonht_sta_present            = pst_ht_opern->bit_obss_nonht_sta_present;
    st_ht_hdl.bit_lsig_txop_protection_full_support = pst_ht_opern->bit_lsig_txop_protection_full_support;
    st_ht_hdl.bit_sta_chan_width                    = pst_ht_opern->bit_sta_chan_width;

    mac_user_set_ht_hdl_etc(pst_mac_user, &st_ht_hdl);

    if ((uc_secondary_chan_offset_old    != st_ht_hdl.bit_secondary_chan_offset) ||
        (uc_sta_chan_width_old           != st_ht_hdl.bit_sta_chan_width))
    {
         ul_change =  MAC_HT_CHANGE;
         OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "mac_proc_ht_opern_ie_etc:usr_bw is updated second_chan_offset from [%d] to [%d], chan_with from [%d] to [%d]",
                          uc_secondary_chan_offset_old, st_ht_hdl.bit_secondary_chan_offset, uc_sta_chan_width_old, st_ht_hdl.bit_sta_chan_width);
    }

    if (OAL_PTR_NULL != g_st_mac_ie_rom_cb.ht_opern_cb)
    {
        g_st_mac_ie_rom_cb.ht_opern_cb(pst_mac_vap, puc_payload, pst_mac_user, &ul_change);
    }


    return ul_change;
}



oal_uint32  mac_ie_proc_obss_scan_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload)
{
    oal_uint16 us_trigger_scan_interval;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_payload)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{mac_ie_proc_obss_scan_ie_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /********************Overlapping BSS Scan Parameters element******************
     |ElementID |Length |OBSS    |OBSS   |BSS Channel   |OBSS Scan  |OBSS Scan   |
     |          |       |Scan    |Scan   |Width Trigger |Passive    |Active Total|
     |          |       |Passive |Active |Scan Interval |Total Per  |Per         |
     |          |       |Dwell   |Dwell  |              |Channel    |Channel     |
     ----------------------------------------------------------------------------
     |1         |1      |2       |2      |2             |2          |2           |
     ----------------------------------------------------------------------------
     |BSS Width   |OBSS Scan|
     |Channel     |Activity |
     |Transition  |Threshold|
     |Delay Factor|         |
     ------------------------
     |2           |2        |
    ***************************************************************************/
    if ( puc_payload[1] < MAC_OBSS_SCAN_IE_LEN)
    {
        return OAL_FAIL;
    }

    us_trigger_scan_interval = OAL_MAKE_WORD16(puc_payload[6], puc_payload[7]);
    if (0 == us_trigger_scan_interval)
    {
        mac_vap_set_peer_obss_scan_etc(pst_mac_vap, OAL_FALSE);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    mac_mib_set_OBSSScanPassiveDwell(pst_mac_vap, OAL_MAKE_WORD16(puc_payload[2], puc_payload[3]));
    mac_mib_set_OBSSScanActiveDwell(pst_mac_vap, OAL_MAKE_WORD16(puc_payload[4], puc_payload[5]));
    /* obss扫描周期最小180秒,最大600S, 初始化默认为300秒 */
    mac_mib_set_BSSWidthTriggerScanInterval(pst_mac_vap, OAL_MIN(OAL_MAX(us_trigger_scan_interval, 180), 600));
    mac_mib_set_OBSSScanPassiveTotalPerChannel(pst_mac_vap, OAL_MAKE_WORD16(puc_payload[8], puc_payload[9]));
    mac_mib_set_OBSSScanActiveTotalPerChannel(pst_mac_vap, OAL_MAKE_WORD16(puc_payload[10], puc_payload[11]));
    mac_mib_set_BSSWidthChannelTransitionDelayFactor(pst_mac_vap, OAL_MAKE_WORD16(puc_payload[12], puc_payload[13]));
    mac_mib_set_OBSSScanActivityThreshold(pst_mac_vap, OAL_MAKE_WORD16(puc_payload[14], puc_payload[15]));
    mac_vap_set_peer_obss_scan_etc(pst_mac_vap, OAL_TRUE);

    return OAL_SUCC;
}



oal_uint32  mac_ie_proc_vht_opern_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_payload, mac_user_stru *pst_mac_user)
{
    mac_vht_hdl_stru                     st_vht_hdl;
    oal_uint8                            uc_ret      = MAC_NO_CHANGE;
    oal_uint16                           us_basic_mcs_set_all_user;
    wlan_mib_vht_op_width_enum_uint8     en_channel_width_old;
    oal_uint8                            uc_channel_center_freq_seg0_old;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_payload)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{mac_ie_proc_vht_opern_ie_etc::param null.}");

        return uc_ret;
    }

    /* 长度校验 */
    if (puc_payload[1] < MAC_VHT_OPERN_LEN)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_ie_proc_vht_opern_ie_etc::invalid vht opern len[%d].}", puc_payload[1]);
        return uc_ret;
    }

    mac_user_get_vht_hdl_etc(pst_mac_user, &st_vht_hdl);

    en_channel_width_old = st_vht_hdl.en_channel_width;
    uc_channel_center_freq_seg0_old = st_vht_hdl.uc_channel_center_freq_seg0;

    /* 解析 "VHT Operation Information" */
    st_vht_hdl.en_channel_width            = puc_payload[MAC_IE_HDR_LEN];
    st_vht_hdl.uc_channel_center_freq_seg0 = puc_payload[MAC_IE_HDR_LEN + 1];
    st_vht_hdl.uc_channel_center_freq_seg1 = puc_payload[MAC_IE_HDR_LEN + 2];

    /* 0 -- 20/40M, 1 -- 80M, 2 -- 160M, 3--80M+80M */
    if (st_vht_hdl.en_channel_width > WLAN_MIB_VHT_OP_WIDTH_80PLUS80)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{mac_ie_proc_vht_opern_ie_etc::invalid channel width[%d], use 20M chn width.}", st_vht_hdl.en_channel_width);
        st_vht_hdl.en_channel_width = WLAN_MIB_VHT_OP_WIDTH_20_40;
    }

    /* 解析 "VHT Basic MCS Set field" */
    us_basic_mcs_set_all_user = OAL_MAKE_WORD16(puc_payload[MAC_IE_HDR_LEN + 3], puc_payload[MAC_IE_HDR_LEN + 4]);
    st_vht_hdl.us_basic_mcs_set = us_basic_mcs_set_all_user;

    mac_user_set_vht_hdl_etc(pst_mac_user, &st_vht_hdl);

    if ((en_channel_width_old != st_vht_hdl.en_channel_width) ||
        (uc_channel_center_freq_seg0_old != st_vht_hdl.uc_channel_center_freq_seg0))
    {
        uc_ret =  MAC_VHT_CHANGE;
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "mac_ie_proc_vht_opern_ie_etc:usr_bw is updated chanl_with from [%d] to [%d], chanl_center_freq_seg0 from [%d] to [%d]",
                         en_channel_width_old, st_vht_hdl.en_channel_width, uc_channel_center_freq_seg0_old, st_vht_hdl.uc_channel_center_freq_seg0);
    }

    if (OAL_PTR_NULL != g_st_mac_ie_rom_cb.vht_opern_cb)
    {
        g_st_mac_ie_rom_cb.vht_opern_cb(pst_mac_vap, puc_payload, pst_mac_user, &uc_ret);
    }

    return uc_ret;
}




oal_uint32 mac_ie_get_wpa_cipher(oal_uint8 *puc_ie, mac_crypto_settings_stru *pst_crypto)
{
    oal_uint16                   us_suites_count;
    oal_uint8                    uc_suite_idx;

    if (OAL_PTR_NULL == puc_ie || OAL_PTR_NULL == pst_crypto)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(pst_crypto, OAL_SIZEOF(mac_crypto_settings_stru));
    /*************************************************************************/
    /*                  WPA Element Format                                   */
    /* --------------------------------------------------------------------- */
    /* |Element ID | Length |    WPA OUI    |  Version |  Group Cipher Suite */
    /* --------------------------------------------------------------------- */
    /* |     1     |   1    |        4      |     2    |         4           */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* Pairwise Cipher |  Pairwise Cipher   |                 |              */
    /* Suite Count     |    Suite List      | AKM Suite Count |AKM Suite List*/
    /* --------------------------------------------------------------------- */
    /*        2        |          4*m       |         2       |     4*n      */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    puc_ie += 1 + 1 + 4 + 2;
    pst_crypto->ul_wpa_versions = WITP_WPA_VERSION_1;

    /* Group Cipher Suite */
    pst_crypto->ul_group_suite = *(oal_uint32 *)puc_ie;
    puc_ie += 4;

    us_suites_count = OAL_MAKE_WORD16(puc_ie[0], puc_ie[1]);

    if (us_suites_count > OAL_NL80211_MAX_NR_CIPHER_SUITES)
    {
        return  OAL_ERR_CODE_MSG_LENGTH_ERR;
    }
    puc_ie += 2;

#ifdef _PRE_WLAN_WEB_CMD_COMM
    pst_crypto->n_pair_suite = us_suites_count;
#endif

    /* Pairwise Cipher Suite 最多存2个 */
    for (uc_suite_idx = 0; uc_suite_idx < us_suites_count; uc_suite_idx++)
    {
        if (uc_suite_idx < WLAN_PAIRWISE_CIPHER_SUITES)
        {
            pst_crypto->aul_pair_suite[uc_suite_idx] = *(oal_uint32 *)puc_ie;
        }
        puc_ie += 4;
    }

    us_suites_count = OAL_MAKE_WORD16(puc_ie[0], puc_ie[1]);
    puc_ie += 2;
    if (us_suites_count > OAL_NL80211_MAX_NR_CIPHER_SUITES)
    {
        return  OAL_ERR_CODE_MSG_LENGTH_ERR;
    }

#ifdef _PRE_WLAN_WEB_CMD_COMM
    pst_crypto->n_akm_suites = us_suites_count;
#endif

    /* AKM Suite 最多存2个 */
    for (uc_suite_idx = 0; uc_suite_idx < us_suites_count; uc_suite_idx++)
    {
        if (uc_suite_idx < WLAN_AUTHENTICATION_SUITES)
        {
            pst_crypto->aul_akm_suite[uc_suite_idx] = *(oal_uint32 *)puc_ie;
        }
        puc_ie += 4;
    }
    return OAL_SUCC;
}

/*lint -save -e438 */

oal_uint32 mac_ie_get_rsn_cipher(oal_uint8 *puc_ie, mac_crypto_settings_stru *pst_crypto)
{
    oal_uint16                   us_suites_count;
    oal_uint16                   us_suite_idx;
    oal_uint8                    uc_ie_len;
    oal_uint8                   *puc_src_ie;

    if (OAL_PTR_NULL == puc_ie || OAL_PTR_NULL == pst_crypto)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(pst_crypto, OAL_SIZEOF(mac_crypto_settings_stru));
    /*************************************************************************/
    /*                  RSN Element Format                                   */
    /* --------------------------------------------------------------------- */
    /* |Element ID | Length | Version | Group Cipher Suite | Pairwise Suite */
    /* --------------------------------------------------------------------- */
    /* |     1     |    1   |    2    |         4          |       2         */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /*  Count| Pairwise Cipher Suite List | AKM Suite Count | AKM Suite List */
    /* --------------------------------------------------------------------- */
    /*       |         4*m                |     2           |   4*n          */
    /* --------------------------------------------------------------------- */
    /* --------------------------------------------------------------------- */
    /* |RSN Capabilities|PMKID Count|PMKID List|Group Management Cipher Suite*/
    /* --------------------------------------------------------------------- */
    /* |        2       |    2      |   16 *s  |               4           | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    uc_ie_len  = puc_ie[1];
    puc_src_ie = puc_ie + 2;

    if (mac_ie_check_rsn_cipher_format(puc_src_ie, uc_ie_len) != OAL_SUCC)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{mac_ie_check_rsn_cipher_format, return fail.}");
        return OAL_ERR_CODE_MSG_LENGTH_ERR;
    }

    puc_ie += 1 + 1 + 2;
    pst_crypto->ul_wpa_versions = WITP_WPA_VERSION_2;

    /* Group Cipher Suite */
    pst_crypto->ul_group_suite = *(oal_uint32 *)puc_ie;
    puc_ie += 4;

    us_suites_count = OAL_MAKE_WORD16(puc_ie[0], puc_ie[1]);
    puc_ie += 2;

#ifdef _PRE_WLAN_WEB_CMD_COMM
    pst_crypto->n_pair_suite = us_suites_count;
#endif

    /* Pairwise Cipher Suite 最多存2个 */
    for (us_suite_idx = 0; us_suite_idx < us_suites_count; us_suite_idx++)
    {
        if (us_suite_idx < WLAN_PAIRWISE_CIPHER_SUITES)
        {
            pst_crypto->aul_pair_suite[us_suite_idx] = *(oal_uint32 *)puc_ie;
        }
        puc_ie += 4;
    }

    us_suites_count = OAL_MAKE_WORD16(puc_ie[0], puc_ie[1]);
    puc_ie += 2;

#ifdef _PRE_WLAN_WEB_CMD_COMM
    pst_crypto->n_akm_suites = us_suites_count;
#endif

    /* AKM Suite 最多存2个 */
    for (us_suite_idx = 0; us_suite_idx < us_suites_count; us_suite_idx++)
    {
        if (us_suite_idx < WLAN_AUTHENTICATION_SUITES)
        {
            pst_crypto->aul_akm_suite[us_suite_idx] = *(oal_uint32 *)puc_ie;
        }
        puc_ie += 4;
    }

    /* 越过RSN Capabilities */
    puc_ie += 2;

    /* 目前PMK信息暂不做处理 */
    if(OAL_FALSE == MAC_IE_REAMIN_LEN_IS_ENOUGH(puc_src_ie, puc_ie, uc_ie_len, 2))
    {
        return OAL_SUCC;
    }
    us_suites_count = OAL_MAKE_WORD16(puc_ie[0], puc_ie[1]);
    puc_ie += 2;
    for (us_suite_idx = 0; us_suite_idx < us_suites_count; us_suite_idx++)
    {
        puc_ie += 4;
    }

    /* 获取Group Management Cipher Suite信息 */
    if(OAL_FALSE == MAC_IE_REAMIN_LEN_IS_ENOUGH(puc_src_ie, puc_ie, uc_ie_len, 4))
    {
        return OAL_SUCC;
    }
    if (MAC_RSN_CHIPER_OUI(0) == ((*(oal_uint32 *)puc_ie) & 0xFFFFFF))
    {
        pst_crypto->ul_group_mgmt_suite = *(oal_uint32 *)puc_ie;
        puc_ie += 4;
    }

    return OAL_SUCC;
}
/*lint -restore */

/*lint -e19*/
oal_module_symbol(mac_ie_proc_sm_power_save_field_etc);
oal_module_symbol(mac_ie_proc_ht_green_field_etc);
oal_module_symbol(mac_ie_get_chan_num_etc);
oal_module_symbol(mac_ie_proc_ht_supported_channel_width_etc);
oal_module_symbol(mac_ie_proc_lsig_txop_protection_support_etc);
oal_module_symbol(mac_ie_proc_ext_cap_ie_etc);
oal_module_symbol(mac_ie_get_wpa_cipher);
oal_module_symbol(mac_ie_get_rsn_cipher);
oal_module_symbol(mac_set_second_channel_offset_ie_etc);
oal_module_symbol(mac_set_11ac_wideband_ie_etc);
oal_module_symbol(mac_ie_proc_ht_sta_etc);
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
oal_module_symbol(mac_ie_proc_chwidth_field_etc);
#endif
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
oal_module_symbol(mac_ie_proc_opmode_field_etc);
#endif
oal_module_symbol(mac_ie_proc_obss_scan_ie_etc);
oal_module_symbol(mac_proc_ht_opern_ie_etc);
oal_module_symbol(mac_ie_proc_vht_opern_ie_etc);


/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


