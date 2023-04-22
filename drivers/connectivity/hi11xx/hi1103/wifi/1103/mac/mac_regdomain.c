


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_regdomain.h"
#include "mac_device.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_REGDOMAIN_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
OAL_CONST mac_supp_mode_table_stru   g_bw_mode_table_2g[] =
{
    /* 1  */    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } },
    /* 2  */    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } },
    /* 3  */    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } },
    /* 4  */    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS  } },
    /* 5  */    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } },
    /* 6  */    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } },
    /* 7  */    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } },
    /* 8  */    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } },
    /* 9  */    { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS, WLAN_BAND_WIDTH_40MINUS  } },
    /* 10 */    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } },
    /* 11 */    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } },
    /* 12 */    { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } },
    /* 13  */   { 2, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS } },
    /* 14  */   { 1, { WLAN_BAND_WIDTH_20M } },
};
// see http://support.huawei.com/ecommunity/bbs/10212257.html
OAL_CONST mac_supp_mode_table_stru   g_bw_mode_table_5g[] =
{

#ifdef _PRE_WLAN_FEATURE_160M
    /* 36  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS,   WLAN_BAND_WIDTH_160PLUSPLUSPLUS   } },
    /* 40  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS,  WLAN_BAND_WIDTH_160MINUSPLUSPLUS  } },
    /* 44  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS,  WLAN_BAND_WIDTH_160PLUSMINUSPLUS  } },
    /* 48  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS, WLAN_BAND_WIDTH_160MINUSMINUSPLUS } },
    /* 52  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS,   WLAN_BAND_WIDTH_160PLUSPLUSMINUS  } },
    /* 56  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS,  WLAN_BAND_WIDTH_160MINUSPLUSMINUS } },
    /* 60  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS,  WLAN_BAND_WIDTH_160PLUSMINUSMINUS } },
    /* 64  */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS, WLAN_BAND_WIDTH_160MINUSMINUSMINUS} },

    /* 100 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS,   WLAN_BAND_WIDTH_160PLUSPLUSPLUS   } },
    /* 104 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS,  WLAN_BAND_WIDTH_160MINUSPLUSPLUS  } },
    /* 108 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS,  WLAN_BAND_WIDTH_160PLUSMINUSPLUS  } },
    /* 112 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS, WLAN_BAND_WIDTH_160MINUSMINUSPLUS } },
    /* 116 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS,   WLAN_BAND_WIDTH_160PLUSPLUSMINUS  } },
    /* 120 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS,  WLAN_BAND_WIDTH_160MINUSPLUSMINUS } },
    /* 124 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS,  WLAN_BAND_WIDTH_160PLUSMINUSMINUS } },
    /* 128 */   { 4, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS, WLAN_BAND_WIDTH_160MINUSMINUSMINUS} },
#else
    /* 36  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } },
    /* 40  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } },
    /* 44  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } },
    /* 48  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} },
    /* 52  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } },
    /* 56  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } },
    /* 60  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } },
    /* 64  */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} },

    /* 100 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } },
    /* 104 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } },
    /* 108 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } },
    /* 112 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} },
    /* 116 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } },
    /* 120 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } },
    /* 124 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } },
    /* 128 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} },
#endif

    /* 132 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS} },
    /* 136 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS} },
    /* 140 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS} },
    /* 144 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} },

    /* 149 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS  } },
    /* 153 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS } },
    /* 157 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS } },
    /* 161 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} },
    /* 165 */   { 1, { WLAN_BAND_WIDTH_20M } },

    /* for JP 4.9G */
    /* 184 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSPLUS } },
    /* 188 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSPLUS} },
    /* 192 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40PLUS,  WLAN_BAND_WIDTH_80PLUSMINUS} },
    /* 196 */   { 3, { WLAN_BAND_WIDTH_20M, WLAN_BAND_WIDTH_40MINUS, WLAN_BAND_WIDTH_80MINUSMINUS} },
};




/*****************************************************************************
  3 函数实现
*****************************************************************************/
/*lint -save -e662 */

oal_uint8   mac_regdomain_get_channel_to_bw_mode_idx(oal_uint8 uc_channel_number)
{
    oal_uint8   uc_idx = 0;

    if(0 == uc_channel_number)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY,"{mac_regdomain_get_channel_to_bw_mode_idx::unknow channel number=%d",uc_channel_number);
        return uc_idx;
    }

    if (uc_channel_number <= 14)
    {
        uc_idx =  uc_channel_number - 1;
    }
    else
    {
        if(uc_channel_number <= 64)
        {
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 36) >> 2); // [0,7]
        }
        else if(uc_channel_number <= 144)
        {
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 100) >> 2) + 8; //  [8, 19]
        }
        else if(uc_channel_number <= 165)
        {
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 149) >> 2) + 20; //  [20, 24]
        }
        else if(uc_channel_number <= 196)
        {
            uc_idx =  (oal_uint8)((oal_uint32)(uc_channel_number - 184) >> 2) + 25; //  [25, 28]
        }
        else
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY,"{mac_regdomain_get_channel_to_bw_mode_idx::unknow channel=%d, force uc_idx = chan 36",uc_channel_number);
            uc_idx =  0;
        }
    }

    return uc_idx;
}


wlan_channel_bandwidth_enum_uint8  mac_regdomain_get_support_bw_mode(wlan_channel_bandwidth_enum_uint8 en_cfg_bw, oal_uint8 uc_channel)
{
    oal_uint8                               uc_idx;
    oal_uint8                               i;
    wlan_channel_bandwidth_enum_uint8       en_bw_mode = WLAN_BAND_WIDTH_20M;

    if(0 == uc_channel)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,"{mac_regdomain_get_support_bw_mode::channel not set yet!");
        return en_bw_mode;
    }

    if(WLAN_BAND_WIDTH_20M == en_cfg_bw)
    {
        return en_bw_mode;
    }

    /*lint -save -e661 */
    uc_idx = mac_regdomain_get_channel_to_bw_mode_idx(uc_channel);
    if(uc_channel <= 14)
    {
        if(WLAN_BAND_WIDTH_40M == en_cfg_bw )
        {
            //未配置带宽扩展方向时，默认用第一种带宽扩展方式
            en_bw_mode = g_bw_mode_table_2g[uc_idx].aen_supp_bw[1];
        }
        else if((WLAN_BAND_WIDTH_40PLUS == en_cfg_bw) || (WLAN_BAND_WIDTH_40MINUS == en_cfg_bw))
        {
            //配置了带宽扩展方向时，检查当前信道是否支持该扩展方向
            for(i = 0; i < g_bw_mode_table_2g[uc_idx].uc_cnt; i++)
            {
                if(g_bw_mode_table_2g[uc_idx].aen_supp_bw[i] == en_cfg_bw)
                {
                    break;
                }
            }

            if(i == g_bw_mode_table_2g[uc_idx].uc_cnt)
            {
                //该信道不支持设置的带宽模式,提示用户带宽扩展方向被驱动自适应调整了
                en_bw_mode = g_bw_mode_table_2g[uc_idx].aen_supp_bw[1];
                OAM_WARNING_LOG3(0, OAM_SF_ANY,"{mac_regdomain_get_support_bw_mode::current ch(%d)not support the cfg bw_mode(%d), change to new bw_mode(%d)",
                uc_channel,en_cfg_bw,en_bw_mode);
            }
            else
            {
                en_bw_mode = en_cfg_bw;
            }
        }
        else
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY,"{mac_regdomain_get_support_bw_mode::2G not support bw_mode=%d,force to 20M",en_cfg_bw);
        }
    }
    else
    {
        if(WLAN_BAND_WIDTH_40M == en_cfg_bw )
        {
            //未配置带宽扩展方向时，使用该信道支持的40M带宽扩展模式
            en_bw_mode = g_bw_mode_table_5g[uc_idx].aen_supp_bw[1];
        }
        else if(WLAN_BAND_WIDTH_80M == en_cfg_bw )
        {
            //未配置带宽扩展方向时，使用该信道支持的80M带宽扩展模式
            en_bw_mode = g_bw_mode_table_5g[uc_idx].aen_supp_bw[2];
        }
        else if((WLAN_BAND_WIDTH_40PLUS <= en_cfg_bw) && (en_cfg_bw <= WLAN_BAND_WIDTH_80MINUSMINUS))
        {
            //配置了带宽扩展方向时，检查当前信道是否支持该扩展方向
            for(i = 0; i < g_bw_mode_table_5g[uc_idx].uc_cnt; i++)
            {
                if(g_bw_mode_table_5g[uc_idx].aen_supp_bw[i] == en_cfg_bw)
                {
                    break;
                }
            }

            if(i == g_bw_mode_table_5g[uc_idx].uc_cnt)
            {
                //该信道不支持设置的带宽模式,提示用户带宽扩展方向被驱动自适应调整了
                if((WLAN_BAND_WIDTH_40PLUS == en_cfg_bw) || (WLAN_BAND_WIDTH_40MINUS == en_cfg_bw))
                {
                    en_bw_mode = g_bw_mode_table_5g[uc_idx].aen_supp_bw[1];
                }
                else
                {
                    en_bw_mode = g_bw_mode_table_5g[uc_idx].aen_supp_bw[2];
                }

                OAM_WARNING_LOG3(0, OAM_SF_ANY,"{mac_regdomain_get_support_bw_mode::current ch(%d)not support the cfg bw_mode(%d), change to new bw_mode(%d)",
                uc_channel,en_cfg_bw,en_bw_mode);
            }
            else
            {
                en_bw_mode = en_cfg_bw;
            }
        }
        else
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY,"{mac_regdomain_get_support_bw_mode::5G not support bw_mode=%d,force to 20M",en_cfg_bw);
        }
    }
    /*lint -restore */

    return en_bw_mode;
}
/*lint -restore */



oal_uint32  mac_regdomain_set_country_etc(oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_country_stru    *pst_country_param;
    mac_regdomain_info_stru *pst_mac_regdom;
    oal_uint8                uc_rc_num;
    oal_uint32               ul_size;

    pst_country_param = (mac_cfg_country_stru *)puc_param;

    pst_mac_regdom = (mac_regdomain_info_stru *)pst_country_param->p_mac_regdom;

    /* 获取管制类的个数 */
    uc_rc_num = pst_mac_regdom->uc_regclass_num;

    /* 计算配置命令 */
    ul_size = (oal_uint32)(OAL_SIZEOF(mac_regclass_info_stru) * uc_rc_num + MAC_RD_INFO_LEN);

    /* 更新管制域信息 */
    oal_memcopy((oal_uint8 *)&g_st_mac_regdomain, (oal_uint8 *)pst_mac_regdom, ul_size);

    /* 更新信道的管制域信息 */
    mac_init_channel_list_etc();

    return OAL_SUCC;
}


oal_int8*  mac_regdomain_get_country_etc(oal_void)
{
    return g_st_mac_regdomain.ac_country;
}

oal_void  mac_get_ext_chan_info(
                oal_uint8                            uc_pri20_channel_idx,
                wlan_channel_bandwidth_enum_uint8    en_bandwidth,
                mac_channel_list_stru                *pst_chan_info)
{
    oal_uint8 uc_start_idx = uc_pri20_channel_idx;
    switch (en_bandwidth)
    {
        case WLAN_BAND_WIDTH_20M:
            pst_chan_info->ul_channels = 1;
            break;

        case WLAN_BAND_WIDTH_40PLUS:
            pst_chan_info->ul_channels = 2;
            break;

        case WLAN_BAND_WIDTH_40MINUS:
            pst_chan_info->ul_channels = 2;
            uc_start_idx = uc_pri20_channel_idx - 1;
            break;

        case WLAN_BAND_WIDTH_80PLUSPLUS:
            pst_chan_info->ul_channels = 4;
            break;

        case WLAN_BAND_WIDTH_80PLUSMINUS:
            pst_chan_info->ul_channels = 4;
            uc_start_idx = uc_pri20_channel_idx - 2;
            break;

        case WLAN_BAND_WIDTH_80MINUSPLUS:
            pst_chan_info->ul_channels = 4;
            uc_start_idx = uc_pri20_channel_idx - 1;
            break;

        case WLAN_BAND_WIDTH_80MINUSMINUS:
            pst_chan_info->ul_channels = 4;
            uc_start_idx = uc_pri20_channel_idx - 3;
            break;

    #ifdef _PRE_WLAN_FEATURE_160M
        case WLAN_BAND_WIDTH_160PLUSPLUSPLUS:
            pst_chan_info->ul_channels = 8;
            break;

        case WLAN_BAND_WIDTH_160PLUSPLUSMINUS:
            pst_chan_info->ul_channels = 8;
            uc_start_idx = uc_pri20_channel_idx - 4;
            break;

        case WLAN_BAND_WIDTH_160PLUSMINUSPLUS:
            pst_chan_info->ul_channels = 8;
            uc_start_idx = uc_pri20_channel_idx - 2;
            break;

        case WLAN_BAND_WIDTH_160PLUSMINUSMINUS:
            pst_chan_info->ul_channels = 8;
            uc_start_idx = uc_pri20_channel_idx - 6;
            break;

        case WLAN_BAND_WIDTH_160MINUSPLUSPLUS:
            pst_chan_info->ul_channels = 8;
            uc_start_idx = uc_pri20_channel_idx - 1;
            break;

        case WLAN_BAND_WIDTH_160MINUSPLUSMINUS:
            pst_chan_info->ul_channels = 8;
            uc_start_idx = uc_pri20_channel_idx - 5;
            break;

        case WLAN_BAND_WIDTH_160MINUSMINUSPLUS:
            pst_chan_info->ul_channels = 8;
            uc_start_idx = uc_pri20_channel_idx - 3;
            break;

        case WLAN_BAND_WIDTH_160MINUSMINUSMINUS:
            pst_chan_info->ul_channels = 8;
            uc_start_idx = uc_pri20_channel_idx - 7;
            break;
    #endif

        default:
            pst_chan_info->ul_channels = 0;
            OAM_ERROR_LOG1(0, OAM_SF_DFS, "{mac_get_ext_chan_info::Invalid bandwidth %d.}", en_bandwidth);
            break;
    }
    if (pst_chan_info->ul_channels)
    {
        oal_memcopy(pst_chan_info->ast_channels, &g_ast_freq_map_5g_etc[uc_start_idx], pst_chan_info->ul_channels*OAL_SIZEOF(mac_freq_channel_map_stru));
    }
}


oal_bool_enum_uint8 mac_is_cover_dfs_channel(
                                oal_uint8                           uc_band,
                                wlan_channel_bandwidth_enum_uint8   en_bandwidth,
                                oal_uint8                           uc_channel_num)
{
    mac_channel_list_stru       st_chan_info;
    oal_uint8                   uc_channel_idx = 0xff;
    oal_uint32                  i;

    if(uc_band != MAC_RC_START_FREQ_5)
    {
        return OAL_FALSE;
    }

    if (OAL_SUCC != mac_get_channel_idx_from_num_etc(uc_band, uc_channel_num, &uc_channel_idx))
    {
        return OAL_FALSE;
    }

    mac_get_ext_chan_info(uc_channel_idx,en_bandwidth,&st_chan_info);

    for(i = 0; i < st_chan_info.ul_channels; i++)
    {
        if (mac_is_ch_in_radar_band(uc_band, st_chan_info.ast_channels[i].uc_idx))
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}
/*lint -e19*/


oal_bool_enum mac_regdomain_channel_is_support_bw(wlan_channel_bandwidth_enum_uint8 en_cfg_bw, oal_uint8 uc_channel)
{
    oal_uint8                               uc_idx;
    oal_uint8                               uc_bw_loop;
    mac_supp_mode_table_stru                st_supp_mode_table;
    wlan_channel_band_enum_uint8            en_channel_band;

    if(0 == uc_channel)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY,"{mac_regdomain_channel_is_support_bw::channel not set yet!");
        return OAL_FALSE;
    }

    if(WLAN_BAND_WIDTH_20M == en_cfg_bw)
    {
        return OAL_TRUE;
    }

    en_channel_band = mac_get_band_by_channel_num(uc_channel);
    /*lint -save -e661 */
    uc_idx = mac_regdomain_get_channel_to_bw_mode_idx(uc_channel);
    if((WLAN_BAND_2G == en_channel_band) && (uc_idx < OAL_SIZEOF(g_bw_mode_table_2g)/OAL_SIZEOF(g_bw_mode_table_2g[0])))
    {
        st_supp_mode_table = g_bw_mode_table_2g[uc_idx];
    }
    else if((WLAN_BAND_5G == en_channel_band) &&  (uc_idx < OAL_SIZEOF(g_bw_mode_table_5g)/OAL_SIZEOF(g_bw_mode_table_5g[0])))
    {
        st_supp_mode_table = g_bw_mode_table_5g[uc_idx];
    }
    else
    {
        return OAL_FALSE;
    }

    for(uc_bw_loop = 0; uc_bw_loop < st_supp_mode_table.uc_cnt ; uc_bw_loop++)
    {
        if(en_cfg_bw == st_supp_mode_table.aen_supp_bw[uc_bw_loop])
        {
            return OAL_TRUE;
        }
    }

    return OAL_FALSE;
}


oal_module_symbol(mac_get_ext_chan_info);
oal_module_symbol(mac_is_cover_dfs_channel);
oal_module_symbol(mac_regdomain_set_country_etc);
oal_module_symbol(mac_regdomain_get_country_etc);
oal_module_symbol(mac_regdomain_get_support_bw_mode);
oal_module_symbol(mac_regdomain_channel_is_support_bw);

/*lint +e19*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

