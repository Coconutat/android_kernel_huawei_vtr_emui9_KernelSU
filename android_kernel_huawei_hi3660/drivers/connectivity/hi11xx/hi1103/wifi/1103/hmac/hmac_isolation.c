

#ifdef _PRE_WLAN_FEATURE_ISOLATION

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_main.h"
#include "hmac_isolation.h"
#include "mac_vap.h"
#include "hmac_vap.h"
#include "mac_resource.h"
#include "hmac_user.h"
#include "hmac_mgmt_ap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ISOLATION_C

/*****************************************************************************
  2 STRUCT定义
*****************************************************************************/
    /*
    CS_ISOLATION_MODE_BROADCAST =0x01
    CS_ISOLATION_MODE_MULTICAST =0x02
    CS_ISOLATION_MODE_UNICAST   =0x04    */
#define BROADCAST_ISOLATION(_a)   ((_a) & CS_ISOLATION_MODE_BROADCAST)
#define MULTICAST_ISOLATION(_a)   ((_a) & CS_ISOLATION_MODE_MULTICAST)
#define UNICAST_ISOLATION(_a)     ((_a) & CS_ISOLATION_MODE_UNICAST)


/*****************************************************************************
  3 全局变量定义
*****************************************************************************/

/*****************************************************************************
  4 函数实现
*****************************************************************************/


oal_uint32 hmac_isolation_set_mode(mac_vap_stru *pst_mac_vap, oal_uint8 uc_mode)
{
    mac_isolation_info_stru       *pst_isolation_info;
    hmac_vap_stru                 *pst_hmac_vap;

    /* 1.1 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_isolation_set_mode::null mac_vap}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_isolation_set_mode::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 2.1 获取参数 */
    uc_mode = uc_mode & 0x7;
    if (0 == uc_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_isolation_set_mode invalid, The valid Para is:(1~7)}\n");
        /* add mode check chenchongbao 2014.7.7 */
        return OAL_ERR_CODE_SECURITY_MODE_INVALID;
    }

    pst_isolation_info = &pst_hmac_vap->st_isolation_info;


    /* 3.1 重新初始化 */
    pst_isolation_info->uc_mode = uc_mode;

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_isolation_set_mode::isolation mode is set to %d}", uc_mode);
    return OAL_SUCC;
}


oal_uint32 hmac_isolation_set_type(mac_vap_stru *pst_mac_vap, oal_uint8 uc_bss_type, oal_uint8 uc_isolation_type)
{
    mac_isolation_info_stru       *pst_isolation_info;
    hmac_vap_stru                 *pst_hmac_vap;

    /* 1.1 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_isolation_set_type::null mac_vap}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 2.1 获取参数 */
    if ((uc_bss_type != CS_ISOLATION_TYPE_MULTI_BSS) && (uc_bss_type != CS_ISOLATION_TYPE_SINGLE_BSS))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_isolation_set_type::bss_type=%d is wrong.}", uc_bss_type);
        return OAL_FAIL;
    }
    if ((uc_isolation_type != OAL_TRUE) && (uc_isolation_type != OAL_FALSE))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_isolation_set_type::isolation_type=%d is wrong.}", uc_isolation_type);
        return OAL_FAIL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_blacklist_init::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_isolation_info = &pst_hmac_vap->st_isolation_info;

    /* 3.1 设置模式 */
    if (CS_ISOLATION_TYPE_MULTI_BSS == uc_bss_type)
    {
        pst_isolation_info->uc_multi_type = uc_isolation_type;
    }
    else
    {
        pst_isolation_info->uc_single_type = uc_isolation_type;
    }

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_isolation_set_type::bss_type=%d, isolation type=%d}", uc_bss_type, uc_isolation_type);
    return OAL_SUCC;
}


oal_uint32 hmac_isolation_set_forward(mac_vap_stru *pst_mac_vap, oal_uint8 uc_forward)
{
    mac_isolation_info_stru       *pst_isolation_info;
    hmac_vap_stru                 *pst_hmac_vap;

    /* 1.1 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_isolation_set_forward::null mac_vap}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_blacklist_init::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 2.1 获取参数 */
    if (uc_forward >= CS_ISOLATION_FORWORD_BUTT)
    {
        uc_forward = CS_ISOLATION_FORWORD_TOLAN;
    }

    pst_isolation_info = &pst_hmac_vap->st_isolation_info;
    pst_isolation_info->uc_forward = uc_forward;
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_isolation_set_forward::isolation forward is set to %d}", uc_forward);

    return OAL_SUCC;
}



oal_uint32 hmac_isolation_clear_counter(mac_vap_stru *pst_mac_vap)
{
    mac_isolation_info_stru       *pst_isolation_info;
    hmac_vap_stru                 *pst_hmac_vap;

    /* 1.1 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_isolation_clear_counter::null mac_vap}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_isolation_clear_counter::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_isolation_info = &pst_hmac_vap->st_isolation_info;

    /* 2.1 刷新计数器 */
    pst_isolation_info->ul_counter_bcast = 0;
    pst_isolation_info->ul_counter_mcast = 0;
    pst_isolation_info->ul_counter_ucast = 0;

    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{isolation counters is cleared }");
    return OAL_SUCC;
}

oal_void hmac_show_isolation_info(mac_vap_stru *pst_mac_vap)
{
    mac_isolation_info_stru         *pst_isolation_info;
    oal_int8                        *pc_print_buff;
    hmac_vap_stru                   *pst_hmac_vap;

    /* 1.1 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_show_isolation_info::null mac_vap}");
        return;
    }
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_show_isolation_info::pst_hmac_vap null.}");
        return;
    }

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_show_autoblacklist_info::pc_print_buff null.}");
        return;
    }
    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);

    pst_isolation_info = &pst_hmac_vap->st_isolation_info;

    /* 1.2 打印隔离信息 */
    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_show_isolation_info::isolation info is :}");
    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{ mode:%d. single_bss_type:%d. multi_bss_type=%d.forward:%d.}",
        pst_isolation_info->uc_mode, pst_isolation_info->uc_single_type, pst_isolation_info->uc_multi_type, pst_isolation_info->uc_forward);
    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{ bcast_cnt: %u.mcast_cnt: %u.ucast_cnt: %u.}",
        pst_isolation_info->ul_counter_bcast,pst_isolation_info->ul_counter_mcast,pst_isolation_info->ul_counter_ucast);

    OAL_SPRINTF(pc_print_buff,OAM_REPORT_MAX_STRING_LEN,
            "vap%d isolation info is :\n"
            "\tmode:%d. single_type:%d. multi_tyep:%d. forward:%d.\n"
            "\tbcast_cnt: %u\n"
            "\tmcast_cnt: %u\n"
            "\tucast_cnt: %u\n",
            pst_mac_vap->uc_vap_id,
            pst_isolation_info->uc_mode,pst_isolation_info->uc_single_type,pst_isolation_info->uc_multi_type,
            pst_isolation_info->uc_forward,
            pst_isolation_info->ul_counter_bcast,
            pst_isolation_info->ul_counter_mcast,
            pst_isolation_info->ul_counter_ucast);

    oam_print_etc(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    return;

}


cs_isolation_forward_enum hmac_isolation_filter(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr)
{
    mac_isolation_info_stru      *pst_isolation_info;
    cs_isolation_forward_enum     uc_ret = CS_ISOLATION_FORWORD_NONE;
    mac_user_stru                *pst_mac_user;
    hmac_vap_stru                *pst_hmac_vap;
    oal_uint16                    us_user_idx;

    /* 1.1 入参检查 */
    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_mac_addr))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_isolation_filter::null mac_vap or null mac addr}");
        return CS_ISOLATION_FORWORD_NONE;
    }
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_isolation_filter::pst_hmac_vap null.}");
        return CS_ISOLATION_FORWORD_NONE;
    }

    pst_isolation_info = &pst_hmac_vap->st_isolation_info;

    /* 1.1 多BSS隔离 */
    if (OAL_TRUE == pst_isolation_info->uc_multi_type)
    {
        /* 1.2 广播隔离 */
        if (ETHER_IS_BROADCAST(puc_mac_addr))
        {
            if (BROADCAST_ISOLATION(pst_isolation_info->uc_mode))
            {
                pst_isolation_info->ul_counter_bcast++;
                OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{hmac_isolation_filter::isolation MultiBSS Bcast=%d}", pst_isolation_info->ul_counter_bcast);
                return (cs_isolation_forward_enum)pst_isolation_info->uc_forward;
            }
            else
            {
                return CS_ISOLATION_FORWORD_NONE;
            }
        }
        /* 1.3 组播隔离 */
        if (ETHER_IS_MULTICAST(puc_mac_addr))
        {
            if (MULTICAST_ISOLATION(pst_isolation_info->uc_mode))
            {
                pst_isolation_info->ul_counter_mcast++;
                OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{hmac_isolation_filter::isolation MultiBSS Mcast=%d}", pst_isolation_info->ul_counter_mcast);
                return (cs_isolation_forward_enum)pst_isolation_info->uc_forward;
            }
            else
            {
                return CS_ISOLATION_FORWORD_NONE;
            }
        }
       /* 2.4 单播隔离,如果在本bss中找到用户，不隔离处理，否则需要在其他bss中找，找到就隔离 */

        if (UNICAST_ISOLATION(pst_isolation_info->uc_mode))
        {
            if (OAL_SUCC != mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_mac_addr, &us_user_idx))
            {
            #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
                if (OAL_SUCC == mac_chip_find_user_by_macaddr(pst_mac_vap->uc_chip_id, puc_mac_addr, &us_user_idx))
            #else
                if (OAL_SUCC == mac_board_find_user_by_macaddr(puc_mac_addr, &us_user_idx))
            #endif
                {
                    pst_mac_user = mac_res_get_mac_user_etc(us_user_idx);
                    if (OAL_PTR_NULL == pst_mac_user)
                    {
                        return CS_ISOLATION_FORWORD_NONE;
                    }
                    pst_isolation_info->ul_counter_ucast++;
                    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_isolation_filter::isolation MultiBSS Ucast=%d. to x.x.x.%02x.%02x.%02x}", \
                                  pst_isolation_info->ul_counter_ucast,puc_mac_addr[3],puc_mac_addr[4],puc_mac_addr[5]);

                    uc_ret = (cs_isolation_forward_enum)pst_isolation_info->uc_forward;
                }
            }
        }
    }

    /* 2.1 单BSS隔离 */
    if (OAL_TRUE == pst_isolation_info->uc_single_type)
    {
        /* 2.2 广播隔离 */
        if (ETHER_IS_BROADCAST(puc_mac_addr))
        {
            if (BROADCAST_ISOLATION(pst_isolation_info->uc_mode))
            {
                pst_isolation_info->ul_counter_bcast++;
                OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{hmac_isolation_filter::isolation SingleBSS Bcast=%d}", pst_isolation_info->ul_counter_bcast);
                return (cs_isolation_forward_enum)pst_isolation_info->uc_forward;
            }
            else
            {
                return CS_ISOLATION_FORWORD_NONE;
            }
        }
        /* 2.3 组播隔离 */
        if (ETHER_IS_MULTICAST(puc_mac_addr))
        {
            if (MULTICAST_ISOLATION(pst_isolation_info->uc_mode))
            {
                pst_isolation_info->ul_counter_mcast++;
                OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                    "{hmac_isolation_filter::isolation SingleBSS Mcast=%d}", pst_isolation_info->ul_counter_mcast);
                return (cs_isolation_forward_enum)pst_isolation_info->uc_forward;
            }
            else
            {
                return CS_ISOLATION_FORWORD_NONE;
            }
        }
        /* 2.4 单播隔离，如果在本bss中找到用户就隔离，否则不处理 */
        if (UNICAST_ISOLATION(pst_isolation_info->uc_mode))
        {
            if (OAL_SUCC == mac_vap_find_user_by_macaddr_etc(pst_mac_vap, puc_mac_addr, &us_user_idx))
            {
                pst_mac_user = mac_res_get_mac_user_etc(us_user_idx);
                if (OAL_PTR_NULL == pst_mac_user)
                {
                    return CS_ISOLATION_FORWORD_NONE;
                }
                pst_isolation_info->ul_counter_ucast++;

                OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                        "{hmac_isolation_filter::isolation SingleBSS Ucast=%d. to x.x.x.%02x.%02x.%02x}",
                        pst_isolation_info->ul_counter_ucast,puc_mac_addr[3],puc_mac_addr[4],puc_mac_addr[5]);

                uc_ret = (cs_isolation_forward_enum)pst_isolation_info->uc_forward;
            }
        }
    }
    return uc_ret;
}
#if 0

oal_void hmac_config_get_isolation(mac_vap_stru *pst_mac_vap,oal_uint8 *pst_info_str,oal_int16 str_len)
{
    oal_int16                    strlen,str_idex;
    oal_int16                    str_len_left;
    mac_isolation_info_stru      *pst_isolation_info;
    oal_time_us_stru             st_cur_time;

    str_len_left = str_len;
    pst_isolation_info = &pst_mac_vap->st_isolation_info;

    str_idex = OAL_SPRINTF(pst_info_str,str_len_left,"isolation type = %d; mode = 0x%x; forward = %d\n"
        "\t[type multi:1; sigle:2] [mode bcast:1; mcast:2; ucast:4] [forward tolan:1; drop:2]",
        pst_isolation_info->uc_type,pst_isolation_info->uc_mode,pst_isolation_info->uc_forward);
    str_len_left -= str_idex;
    if(str_len_left <= 0) return;

    strlen = OAL_SPRINTF(pst_info_str + str_idex,str_len_left,
        "\tisolation counter_bcast=%d\n" "  mcast=%d\n" "  ucast=%d\n",
        pst_isolation_info->ul_counter_bcast,pst_isolation_info->ul_counter_mcast,pst_isolation_info->ul_counter_ucast);
    str_idex += strlen;
    str_len_left -= strlen;
    if(str_len_left <= 0) return;

    oal_time_get_stamp_us(&st_cur_time);
    strlen = OAL_SPRINTF(pst_info_str + str_idex,str_len_left,"\ntotal str_len=%d. curr_time=%d\n",
        str_idex,(oal_uint32)st_cur_time.i_sec);

    oam_print_etc(pst_info_str);    /* Add log to SDT */

    return;
}

/*lint -e578*//*lint -e19*/
oal_module_symbol(hmac_config_get_isolation);
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif	/* #ifdef _PRE_WLAN_FEATURE_ISOLATION */




