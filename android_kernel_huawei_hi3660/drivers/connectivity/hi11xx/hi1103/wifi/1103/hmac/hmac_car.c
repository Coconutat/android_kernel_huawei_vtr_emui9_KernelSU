
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#ifdef _PRE_WLAN_FEATURE_CAR
/*****************************************************************************
  1 头文件包含
*****************************************************************************/

#include "mac_device.h"
#include "hmac_device.h"
#include "mac_resource.h"
#include "hmac_resource.h"
#include "hmac_car.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CAR_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/

OAL_STATIC oal_uint32 hmac_car_inject_token(hmac_device_stru *pst_hmac_dev, hmac_car_limit_stru *pst_car_cfg)
{
    oal_uint64  ull_left_bytes_tmp;

    if (OAL_PTR_NULL == pst_car_cfg)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_inject_token::pst_car_cfg null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 注入令牌 */
    if (OAL_TRUE == pst_car_cfg->en_car_limit_flag)
    {
        if(OAL_FALSE == pst_car_cfg->en_car_same_left_bytes_flag)
        {
            //将Kbit 转化为 Byte; 采用64bit存储数据，64bit除法用do_div; win32下可以直接除/
            ull_left_bytes_tmp = ((oal_uint64)pst_car_cfg->ul_car_limit_kbps * (1024>>3) * pst_hmac_dev->us_car_ctl_cycle_ms);
            ull_left_bytes_tmp = oal_div_u64(ull_left_bytes_tmp, 1000);
            pst_car_cfg->ul_car_limit_left_bytes = (oal_uint32)ull_left_bytes_tmp;
            pst_car_cfg->ul_car_orgin_limit_left_bytes = pst_car_cfg->ul_car_limit_left_bytes;
            pst_car_cfg->en_car_same_left_bytes_flag = OAL_TRUE;
        }
        else if(OAL_TRUE == pst_car_cfg->en_car_same_left_bytes_flag)
        {
            pst_car_cfg->ul_car_limit_left_bytes = pst_car_cfg->ul_car_orgin_limit_left_bytes;
        }
        else
        {
            OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_token::en_car_same_left_bytes_flag[%d]}", pst_car_cfg->en_car_same_left_bytes_flag);
            return OAL_FAIL;
        }

        OAM_INFO_LOG3(0, OAM_SF_CAR, "{hmac_car_inject_token:: en_car_limit_flag[%d], en_car_same_left_bytes_flag[%d], ul_car_limit_left_bytes[%d]}",pst_car_cfg->en_car_limit_flag,
            pst_car_cfg->en_car_same_left_bytes_flag, pst_car_cfg->ul_car_limit_left_bytes);
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_car_inject_device_token(hmac_device_stru *pst_hmac_dev)
{
    oal_uint32       ul_ret;

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_inject_device_token::pst_hmac_dev null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* device_up注入令牌 */
    ul_ret = hmac_car_inject_token(pst_hmac_dev, &pst_hmac_dev->st_car_device_cfg[HMAC_CAR_UPLINK]);
    if(OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_device_token::hmac_car_inject_token up wrong![%d]}",ul_ret);
        return ul_ret;
    }

    /* device_down注入令牌 */
    ul_ret = hmac_car_inject_token(pst_hmac_dev, &pst_hmac_dev->st_car_device_cfg[HMAC_CAR_DOWNLINK]);
    if(OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_device_token::hmac_car_inject_token down wrong![%d]}",ul_ret);
        return ul_ret;
    }

    /* 组播 device_down注入令牌 */
    ul_ret = hmac_car_inject_token(pst_hmac_dev, &pst_hmac_dev->st_car_device_multicast_cfg);
    if(OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_device_token::hmac_car_inject_token device down MULTICAST wrong![%d]}",ul_ret);
        return ul_ret;
    }

    //组播PPS注入
    pst_hmac_dev->st_car_device_multicast_cfg.ul_car_multicast_pps_num_left = pst_hmac_dev->st_car_device_multicast_cfg.ul_car_orgin_multicast_pps_num;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_car_inject_vap_token(hmac_device_stru *pst_hmac_dev, hmac_vap_stru *pst_hmac_vap)
{
    oal_uint32                     ul_ret = OAL_SUCC;

    if ((OAL_PTR_NULL == pst_hmac_dev) || (OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_inject_vap_token::pst_hmac_dev || pst_hmac_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 对于限速VAP,需要注入令牌*/
    /* 为限速vap_up注入令牌 */
    ul_ret = hmac_car_inject_token(pst_hmac_dev, &pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_UPLINK]);
    if(OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_vap_token::hmac_car_inject_token on wrong![%d]}",ul_ret);
        return ul_ret;
    }

    /* 为限速vap_down注入令牌 */
    ul_ret = hmac_car_inject_token(pst_hmac_dev, &pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_DOWNLINK]);
    if(OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_vap_token::hmac_car_inject_token down wrong![%d]}",ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_car_inject_user_token(hmac_device_stru *pst_hmac_dev, hmac_user_stru *pst_hmac_user)
{
    oal_uint32     ul_ret = OAL_SUCC;

    if ((OAL_PTR_NULL == pst_hmac_dev) || (OAL_PTR_NULL == pst_hmac_user))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_inject_vap_token::pst_hmac_dev || pst_hmac_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 为限速user_up注入令牌 */
    ul_ret = hmac_car_inject_token(pst_hmac_dev, &pst_hmac_user->st_car_user_cfg[HMAC_CAR_UPLINK]);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_user_token::hmac_car_inject_token on wrong![%d]}",ul_ret);
        return ul_ret;
    }

    /* 为限速user_down注入令牌 */
    ul_ret = hmac_car_inject_token(pst_hmac_dev, &pst_hmac_user->st_car_user_cfg[HMAC_CAR_DOWNLINK]);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_inject_user_token::hmac_car_inject_token down wrong![%d]}",ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32 hmac_car_limit_timer_timeout_handle(oal_void *pst_void_hmac_dev)
{
    mac_user_stru                   *pst_mac_user;
    mac_vap_stru                    *pst_mac_vap;
    oal_dlist_head_stru             *pst_list_pos;
    oal_uint8                        uc_vap_index;
    mac_device_stru                 *pst_mac_device;
    hmac_device_stru                *pst_hmac_dev;
    hmac_vap_stru                   *pst_hmac_vap;
    hmac_user_stru                  *pst_hmac_user;
    oal_uint32                       ul_ret;

    pst_hmac_dev = (hmac_device_stru*)pst_void_hmac_dev;
    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_limit_timer_timeout_handle::pst_hmac_dev null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_hmac_dev->pst_device_base_info;
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_limit_timer_timeout_handle::pst_mac_device null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* hmac_device 注入令牌 */
    ul_ret = hmac_car_inject_device_token(pst_hmac_dev);
    if(OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_limit_timer_timeout_handle::hmac_car_inject_token down wrong![%d]}",ul_ret);
        return ul_ret;
    }


    /* 遍历device下所有vap */
    for (uc_vap_index = 0; uc_vap_index <pst_mac_device->uc_vap_num; uc_vap_index++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_limit_timer_timeout_handle::pst_vap null,vap_id=%d.}", pst_mac_device->auc_vap_id[uc_vap_index]);
            continue;
        }

        /* VAP模式判断, 只有ap模式下才有CAR, 其他模式下，不注入令牌 */
        if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
        {
             OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{hmac_car_limit_timer_timeout_handle:: CAR only used in AP mode; en_vap_mode=%d.}", pst_mac_vap->en_vap_mode);
             continue;
        }

        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_limit_timer_timeout_handle::pst_hmac_vap null}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 对于限速VAP,需要注入令牌*/
        hmac_car_inject_vap_token(pst_hmac_dev, pst_hmac_vap);


        //遍历vap下面的所有user
        HMAC_VAP_FOREACH_USER(pst_mac_user, pst_mac_vap, pst_list_pos)
        {
            /* 如果用户尚未关联成功，则不访问私有信息结构体 */
            if (MAC_USER_STATE_ASSOC != pst_mac_user->en_user_asoc_state)
            {
                continue;
            }
            //获取hmac user
            pst_hmac_user = mac_res_get_hmac_user_etc(pst_mac_user->us_assoc_id);
            if (OAL_PTR_NULL == pst_hmac_user)
            {
                OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_limit_timer_timeout_handle::pst_hmac_user null}");
                return OAL_ERR_CODE_PTR_NULL;
            }

            hmac_car_inject_user_token(pst_hmac_dev, pst_hmac_user);

        }
    }


    return OAL_SUCC;
}


oal_uint32  hmac_car_device_multicast(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param)
{
    hmac_car_limit_stru          *pst_car_device_cfg;

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_device_multicast::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_car_device_cfg = &pst_hmac_dev->st_car_device_multicast_cfg;

    pst_car_device_cfg->ul_car_limit_kbps = pst_car_cfg_param->ul_bw_limit_kbps;
    pst_car_device_cfg->en_car_limit_flag = ((0 != pst_car_device_cfg->ul_car_limit_kbps) || (0 !=pst_car_device_cfg->ul_car_orgin_multicast_pps_num)) ? 1 : 0;
    //flag置为0，说明下一次需要重新计算剩余令牌数
    pst_car_device_cfg->en_car_same_left_bytes_flag = OAL_FALSE;

    OAM_WARNING_LOG3(0, OAM_SF_CAR, "{hmac_car_device_multicast: uc_device_id[%d], ul_car_limit_kbps =[%d], uc_car_limit_flag =[%d]}",
                                        pst_hmac_dev->pst_device_base_info->uc_device_id,
                                        pst_car_device_cfg->ul_car_limit_kbps,
                                        pst_car_device_cfg->en_car_limit_flag);

    return OAL_SUCC;
}


oal_uint32  hmac_car_device_multicast_pps_num(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param)
{
    hmac_car_limit_stru          *pst_car_device_cfg;

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_device_multicast_pps_num::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_car_device_cfg = &pst_hmac_dev->st_car_device_multicast_cfg;

    pst_car_device_cfg->ul_car_orgin_multicast_pps_num = pst_car_cfg_param->ul_car_multicast_pps_num * pst_hmac_dev->us_car_ctl_cycle_ms / 1000;
    pst_car_device_cfg->en_car_limit_flag = ((0 != pst_car_device_cfg->ul_car_orgin_multicast_pps_num ) || (0 !=pst_car_device_cfg->ul_car_limit_kbps)) ? 1 : 0;

    OAM_WARNING_LOG4(0, OAM_SF_CAR, "{hmac_car_device_multicast_pps_num: uc_device_id[%d], cfg_pps_num =[%d], cycle_ms =[%d], orgin_pps_num[%d] }",
                                        pst_hmac_dev->pst_device_base_info->uc_device_id,
                                        pst_car_cfg_param->ul_car_multicast_pps_num,
                                        pst_hmac_dev->us_car_ctl_cycle_ms,
                                        pst_car_device_cfg->ul_car_orgin_multicast_pps_num);

    return OAL_SUCC;
}


oal_uint32  hmac_car_device_bw_limit(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param)
{
    hmac_car_limit_stru          *pst_car_device_cfg;

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_device_bw_limit::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_car_device_cfg = &pst_hmac_dev->st_car_device_cfg[pst_car_cfg_param->uc_car_up_down_type];


    pst_car_device_cfg->ul_car_limit_kbps = pst_car_cfg_param->ul_bw_limit_kbps;
    pst_car_device_cfg->en_car_limit_flag = (0 != pst_car_device_cfg->ul_car_limit_kbps) ? 1 : 0;
    //flag置为0，说明下一次定时器超时需要重新计算剩余令牌数
    pst_car_device_cfg->en_car_same_left_bytes_flag = OAL_FALSE;

    OAM_WARNING_LOG3(0, OAM_SF_CAR, "{hmac_car_device_bw_limit: uc_device_id[%d], ul_car_limit_kbps =[%d], uc_car_limit_flag =[%d]}",
                                        pst_hmac_dev->pst_device_base_info->uc_device_id,
                                        pst_car_device_cfg->ul_car_limit_kbps,
                                        pst_car_device_cfg->en_car_limit_flag);

    return OAL_SUCC;
}


oal_uint32  hmac_car_vap_bw_limit(hmac_vap_stru *pst_hmac_vap, mac_cfg_car_stru *pst_car_cfg_param)

{
    hmac_car_limit_stru          *pst_car_vap_cfg;

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_vap_bw_limit::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_car_vap_cfg = &pst_hmac_vap->st_car_vap_cfg[pst_car_cfg_param->uc_car_up_down_type];

    pst_car_vap_cfg->ul_car_limit_kbps = pst_car_cfg_param->ul_bw_limit_kbps;
    pst_car_vap_cfg->en_car_limit_flag = (0 != pst_car_vap_cfg->ul_car_limit_kbps) ? 1 : 0;
    //flag置为0，说明下一次定时器超时需要重新计算剩余令牌数
    pst_car_vap_cfg->en_car_same_left_bytes_flag = OAL_FALSE;

    OAM_WARNING_LOG3(0, OAM_SF_CAR, "{hmac_car_vap_bw_limit: uc_vap_id[d], ul_car_limit_kbps =[%d], uc_car_limit_flag =[%d]}",
                                        pst_hmac_vap->st_vap_base_info.uc_vap_id,
                                        pst_car_vap_cfg->ul_car_limit_kbps,
                                        pst_car_vap_cfg->en_car_limit_flag);

    return OAL_SUCC;
}


oal_uint32  hmac_car_user_bw_limit(hmac_vap_stru *pst_hmac_vap, mac_cfg_car_stru *pst_car_cfg_param)
{
    hmac_user_stru                  *pst_hmac_user;
    hmac_car_limit_stru             *pst_car_user_cfg;

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_user_bw_limit::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(&pst_hmac_vap->st_vap_base_info, pst_car_cfg_param->auc_user_macaddr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_MULTI_TRAFFIC, "{hmac_car_user_bw_limit: mac_vap_find_user_by_macaddr_etc failed}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_car_user_cfg = &pst_hmac_user->st_car_user_cfg[pst_car_cfg_param->uc_car_up_down_type];

    /* 配置用户带宽, 如果为0表示不限制带宽 */
    pst_car_user_cfg->ul_car_limit_kbps = pst_car_cfg_param->ul_bw_limit_kbps;
    pst_car_user_cfg->en_car_limit_flag = (0 != pst_car_user_cfg->ul_car_limit_kbps) ? 1 : 0;
    //flag置为0，说明下一次定时器超时需要重新计算剩余令牌数
    pst_car_user_cfg->en_car_same_left_bytes_flag = OAL_FALSE;

    OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_MULTI_TRAFFIC, "{hmac_car_user_bw_limit: uc_vap_id[%d], user_id = %d, bw = %d, flag = %d \n}",
                                                pst_hmac_vap->st_vap_base_info.uc_vap_id,
                                                pst_hmac_user->st_user_base_info.us_assoc_id,
                                                pst_car_user_cfg->ul_car_limit_kbps,
                                                pst_car_user_cfg->en_car_limit_flag);

    return OAL_SUCC;
}


oal_uint32  hmac_car_device_timer_cycle_limit(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param)
{
    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_device_timer_cycle_limit::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_dev->us_car_ctl_cycle_ms = pst_car_cfg_param->us_car_timer_cycle_ms;

    //先销毁原来的定时器
    if (pst_hmac_dev->st_car_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_dev->st_car_timer);
    }
    //重新启动定时器
    if (pst_hmac_dev->pst_device_base_info)
    {
        FRW_TIMER_CREATE_TIMER(&pst_hmac_dev->st_car_timer,
                               hmac_car_limit_timer_timeout_handle,
                               pst_hmac_dev->us_car_ctl_cycle_ms,
                               (oal_void *)pst_hmac_dev,
                               OAL_TRUE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_dev->pst_device_base_info->ul_core_id);
        //若car未使能，则关闭定时器
        if (OAL_FALSE == pst_hmac_dev->en_car_enable_flag)
        {
            FRW_TIMER_STOP_TIMER(&pst_hmac_dev->st_car_timer);
        }
    }



    return OAL_SUCC;
}


oal_uint32  hmac_car_enable_switch(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param)
{
    mac_device_stru         *pst_mac_dev;
    mac_vap_stru            *pst_mac_vap;
    mac_user_stru           *pst_mac_user;
    hmac_vap_stru           *pst_hmac_vap;
    hmac_user_stru          *pst_hmac_user;
    oal_uint8                uc_vap_index;
    oal_dlist_head_stru     *pst_list_pos;

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_enable_switch::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_dev->en_car_enable_flag = pst_car_cfg_param->en_car_enable_flag;

    if (OAL_TRUE == pst_hmac_dev->en_car_enable_flag)
    {
        /* 打开定时器 */
        if(OAL_FALSE == pst_hmac_dev->st_car_timer.en_is_enabled)
        {
            frw_timer_restart_timer_etc(&pst_hmac_dev->st_car_timer, pst_hmac_dev->us_car_ctl_cycle_ms, OAL_TRUE);
            //car 丢包统计清空
            pst_hmac_dev->ul_car_packet_drop_num[HMAC_CAR_UPLINK] = 0;
            pst_hmac_dev->ul_car_packet_drop_num[HMAC_CAR_DOWNLINK] = 0;
            pst_hmac_dev->ul_car_multicast_packet_drop_num = 0;
            pst_hmac_dev->ul_car_multicast_packet_pps_drop_num = 0;

            OAM_WARNING_LOG1(0, OAM_SF_CAR, "{hmac_car_enable_switch:: car_enable[%d].}", pst_hmac_dev->en_car_enable_flag);
        }

        return OAL_SUCC;
    }

    //如果是关闭car，则将device vap user下面的car_flag都关闭
    //1、关闭hmac_dev下面的car_flag
    pst_hmac_dev->st_car_device_cfg[HMAC_CAR_UPLINK].en_car_limit_flag = OAL_FALSE;
    pst_hmac_dev->st_car_device_cfg[HMAC_CAR_UPLINK].en_car_same_left_bytes_flag = OAL_FALSE;
    pst_hmac_dev->st_car_device_cfg[HMAC_CAR_DOWNLINK].en_car_limit_flag = OAL_FALSE;
    pst_hmac_dev->st_car_device_cfg[HMAC_CAR_DOWNLINK].en_car_same_left_bytes_flag = OAL_FALSE;
    pst_hmac_dev->us_car_ctl_cycle_ms = HMAC_CAR_CYCLE_MS;
    pst_hmac_dev->st_car_device_multicast_cfg.en_car_limit_flag = OAL_FALSE;
    pst_hmac_dev->st_car_device_multicast_cfg.ul_car_limit_kbps = 0;
    pst_hmac_dev->st_car_device_multicast_cfg.ul_car_limit_left_bytes = 0;
    pst_hmac_dev->st_car_device_multicast_cfg.en_car_same_left_bytes_flag = OAL_FALSE;
    pst_hmac_dev->st_car_device_multicast_cfg.ul_car_orgin_multicast_pps_num = 0;

    //根据hmac_dev 寻找mac_dev
    pst_mac_dev = pst_hmac_dev->pst_device_base_info;
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_enable_switch::pst_mac_device null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    //遍历mac_dev下面的所有vap
    HMAC_DEVICE_FOREACH_VAP(pst_mac_vap, pst_mac_dev, uc_vap_index)
    {
        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_enable_switch::pst_hmac_vap null}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        //2、关闭hmac_vap下面的car_flag
        pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_UPLINK].en_car_limit_flag = OAL_FALSE;
        pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_UPLINK].en_car_same_left_bytes_flag= OAL_FALSE;
        pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_DOWNLINK].en_car_limit_flag = OAL_FALSE;
        pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_DOWNLINK].en_car_same_left_bytes_flag= OAL_FALSE;

        //遍历vap下面的所有user
        HMAC_VAP_FOREACH_USER(pst_mac_user, pst_mac_vap, pst_list_pos)
        {
            /* 如果用户尚未关联成功，则不访问私有信息结构体 */
            if (MAC_USER_STATE_ASSOC != pst_mac_user->en_user_asoc_state)
            {
                continue;
            }
            //获取hmac user
            pst_hmac_user = mac_res_get_hmac_user_etc(pst_mac_user->us_assoc_id);
            if (OAL_PTR_NULL == pst_hmac_user)
            {
                OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_enable_switch::pst_hmac_user null}");
                return OAL_ERR_CODE_PTR_NULL;
            }
            //3、关闭hmac_vap下面的car_flag
            pst_hmac_user->st_car_user_cfg[HMAC_CAR_UPLINK].en_car_limit_flag = OAL_FALSE;
            pst_hmac_user->st_car_user_cfg[HMAC_CAR_UPLINK].en_car_same_left_bytes_flag= OAL_FALSE;
            pst_hmac_user->st_car_user_cfg[HMAC_CAR_DOWNLINK].en_car_limit_flag = OAL_FALSE;
            pst_hmac_user->st_car_user_cfg[HMAC_CAR_DOWNLINK].en_car_same_left_bytes_flag= OAL_FALSE;

        }
    }

    //关闭car定时器
    FRW_TIMER_STOP_TIMER(&pst_hmac_dev->st_car_timer);

    return OAL_SUCC;
}


oal_uint32  hmac_car_show_info(hmac_device_stru *pst_hmac_dev)
{
    mac_user_stru                   *pst_mac_user;
    oal_dlist_head_stru             *pst_list_pos;
    hmac_user_stru                  *pst_hmac_user;
    mac_vap_stru                    *pst_mac_vap;
    oal_uint8                        uc_vap_index;
    mac_device_stru                 *pst_mac_device;
    hmac_vap_stru                   *pst_hmac_vap;

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_show_info::pst_hmac_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_device = pst_hmac_dev->pst_device_base_info;
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_show_info::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_IO_PRINT("\n******************************************************\n");
    OAL_IO_PRINT("car_enable_switch:[%d]    timer_timeout:[%d]", pst_hmac_dev->en_car_enable_flag, pst_hmac_dev->us_car_ctl_cycle_ms);
    OAL_IO_PRINT("   device: id[%d]\n"
                 "       uplink:   flag:[%d] limit_kbps_bw:[%d] drop_packet_num:[%d]\n"
                 "       downlink: flag:[%d] limit_kbps_bw:[%d] drop_packet_num:[%d]\n"
                 "       multicast: flag:[%d] limit_kbps_bw:[%d] drop_packet_num:[%d];  orgin_pps_num:[%d]  drop_packet_num_pps:[%d]\n",
                 pst_mac_device->uc_device_id,
                 pst_hmac_dev->st_car_device_cfg[HMAC_CAR_UPLINK].en_car_limit_flag, pst_hmac_dev->st_car_device_cfg[HMAC_CAR_UPLINK].ul_car_limit_kbps, pst_hmac_dev->ul_car_packet_drop_num[HMAC_CAR_UPLINK],
                 pst_hmac_dev->st_car_device_cfg[HMAC_CAR_DOWNLINK].en_car_limit_flag, pst_hmac_dev->st_car_device_cfg[HMAC_CAR_DOWNLINK].ul_car_limit_kbps, pst_hmac_dev->ul_car_packet_drop_num[HMAC_CAR_DOWNLINK],
                 pst_hmac_dev->st_car_device_multicast_cfg.en_car_limit_flag, pst_hmac_dev->st_car_device_multicast_cfg.ul_car_limit_kbps, pst_hmac_dev->ul_car_multicast_packet_drop_num,
                 (pst_hmac_dev->st_car_device_multicast_cfg.ul_car_orgin_multicast_pps_num*1000/pst_hmac_dev->us_car_ctl_cycle_ms),
                 pst_hmac_dev->ul_car_multicast_packet_pps_drop_num);

    /* 遍历device下所有vap */
    for (uc_vap_index = 0; uc_vap_index <pst_mac_device->uc_vap_num; uc_vap_index++)
    {
        pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CAR, "{hmac_car_show_info::pst_vap null,vap_id=%d.}", pst_mac_device->auc_vap_id[uc_vap_index]);
            continue;
        }

        /* VAP模式判断, 只有ap模式下才有CAR, 其他模式下，不显示CAR信息 */
        if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
        {
             OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{hmac_car_show_info:: CAR only used in AP mode; en_vap_mode=%d.}", pst_mac_vap->en_vap_mode);
             continue;
        }

        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_show_info::pst_hmac_vap null}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        OAL_IO_PRINT("   vap: id:[%d]\n"
                     "       uplink:   flag:[%d] limit_kbps_bw:[%d]\n"
                     "       downlink: flag:[%d] limit_kbps_bw:[%d]\n",
                     pst_mac_vap->uc_vap_id,
                     pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_UPLINK].en_car_limit_flag, pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_UPLINK].ul_car_limit_kbps,
                     pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_DOWNLINK].en_car_limit_flag, pst_hmac_vap->st_car_vap_cfg[HMAC_CAR_DOWNLINK].ul_car_limit_kbps);

        /* 遍历vap下所有user */
        OAL_IO_PRINT("   user:\n");
        HMAC_VAP_FOREACH_USER(pst_mac_user, &pst_hmac_vap->st_vap_base_info, pst_list_pos)
        {
            /* 如果用户尚未关联成功，则不访问私有信息结构体 */
            if (MAC_USER_STATE_ASSOC != pst_mac_user->en_user_asoc_state)
            {
                continue;
            }
            //获取hmac user
            pst_hmac_user = mac_res_get_hmac_user_etc(pst_mac_user->us_assoc_id);
            if (OAL_PTR_NULL == pst_hmac_user)
            {
                OAM_ERROR_LOG0(0, OAM_SF_CAR, "{hmac_car_show_info::pst_hmac_user null}");
                return OAL_ERR_CODE_PTR_NULL;
            }
            OAL_IO_PRINT("       user id:[%d]  mac addr:[%02x:%02x:%02x:%02x:%02x:%02x]\n", pst_mac_user->us_assoc_id,
                pst_mac_user->auc_user_mac_addr[0],pst_mac_user->auc_user_mac_addr[1],
                pst_mac_user->auc_user_mac_addr[2],pst_mac_user->auc_user_mac_addr[3],
                pst_mac_user->auc_user_mac_addr[4],pst_mac_user->auc_user_mac_addr[5]);
            OAL_IO_PRINT("          uplink:   flag:[%d] limit_kbps_bw:[%d]\n"
                         "          downlink: flag:[%d] limit_kbps_bw:[%d]\n\n",
                         pst_hmac_user->st_car_user_cfg[HMAC_CAR_UPLINK].en_car_limit_flag, pst_hmac_user->st_car_user_cfg[HMAC_CAR_UPLINK].ul_car_limit_kbps,
                         pst_hmac_user->st_car_user_cfg[HMAC_CAR_DOWNLINK].en_car_limit_flag, pst_hmac_user->st_car_user_cfg[HMAC_CAR_DOWNLINK].ul_car_limit_kbps);

        }
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32 hmac_car_consume_process(hmac_car_limit_stru *pst_car_cfg, oal_uint16 us_mpdu_payload_len)
{
    if(OAL_FALSE == pst_car_cfg->en_car_limit_flag)
    {
        return HMAC_TX_PASS;
    }

    /* 对于使能流量限制的device/vap/user, 且已超过流量控制的报文进行丢弃 */
    if (pst_car_cfg->ul_car_limit_left_bytes < (us_mpdu_payload_len >> 1))
    {
        pst_car_cfg->ul_car_limit_left_bytes = 0;
        return HMAC_TX_DROP_CAR_LIMIT;
    }

    pst_car_cfg->ul_car_limit_left_bytes
       = (pst_car_cfg->ul_car_limit_left_bytes > us_mpdu_payload_len) ?
               (pst_car_cfg->ul_car_limit_left_bytes - us_mpdu_payload_len) : 0;

    OAM_INFO_LOG3(0, OAM_SF_CAR, "{hmac_car_consume_process:: ul_car_orgin_limit_left_bytes[%d], ul_car_limit_left_bytes[%d], us_mpdu_payload_len[%d]}",
        pst_car_cfg->ul_car_orgin_limit_left_bytes, pst_car_cfg->ul_car_limit_left_bytes,us_mpdu_payload_len);

    return HMAC_TX_PASS;
}


oal_uint32 hmac_car_process_uplink(oal_uint16 us_ta_user_idx, hmac_vap_stru *pst_hmac_vap,
                                    oal_netbuf_stru *pst_netbuf, hmac_car_up_down_type_enum_uint8 en_car_type)
{
    hmac_device_stru                   *pst_hmac_dev;
    hmac_user_stru                     *pst_hmac_user;
    oal_uint32                          ul_rslt;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    //获取hmac_dev
    pst_hmac_dev = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_dev))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    //若car没使能，则直接返回succ
    if (OAL_FALSE == pst_hmac_dev->en_car_enable_flag)
    {
       return OAL_SUCC;
    }

    //获取发送端的hmac_user
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_ta_user_idx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_user))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CAR, "{hmac_car_process_uplink::mac_res_get_hmac_user_etc error, user id=%d}", us_ta_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_rslt = hmac_car_process(pst_hmac_dev, pst_hmac_vap, pst_hmac_user, pst_netbuf, HMAC_CAR_UPLINK);
    return ul_rslt;

}


OAL_STATIC oal_uint32 hmac_car_judge_packet_drop(hmac_vap_stru *pst_hmac_vap, hmac_car_limit_stru *pst_car_device_cfg, hmac_car_limit_stru *pst_car_vap_cfg, hmac_car_limit_stru *pst_car_user_cfg, oal_uint16 us_mpdu_payload_len)
{
    oal_uint32                    ul_ret = OAL_SUCC;

    /* 判断顺序: user vap device；  */
    /* 使能car的user, 且令牌耗尽; 丢弃报文 */
    ul_ret = hmac_car_consume_process(pst_car_user_cfg, us_mpdu_payload_len);
    if (HMAC_TX_DROP_CAR_LIMIT == ul_ret)
    {
        return HMAC_TX_DROP_CAR_LIMIT;
    }

    /* 使能car的vap, 且令牌耗尽; 丢弃报文 */
    ul_ret = hmac_car_consume_process(pst_car_vap_cfg, us_mpdu_payload_len);
    if (HMAC_TX_DROP_CAR_LIMIT == ul_ret)
    {
        return HMAC_TX_DROP_CAR_LIMIT;
    }

    /* 使能car的device, 且令牌耗尽; 丢弃报文 */
    ul_ret = hmac_car_consume_process(pst_car_device_cfg, us_mpdu_payload_len);
    if (HMAC_TX_DROP_CAR_LIMIT == ul_ret)
    {
        return HMAC_TX_DROP_CAR_LIMIT;
    }

    return HMAC_TX_PASS;
}


oal_uint32  hmac_car_process(hmac_device_stru *pst_hmac_dev, hmac_vap_stru *pst_hmac_vap,
                                              hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_buf,
                                              hmac_car_up_down_type_enum_uint8 en_car_type)

{
    oal_uint16                    us_mpdu_payload_len;
    hmac_car_limit_stru          *pst_car_device_cfg;
    hmac_car_limit_stru          *pst_car_vap_cfg;
    hmac_car_limit_stru          *pst_car_user_cfg;
    oal_uint32                    ul_ret = OAL_SUCC;

    if ((OAL_PTR_NULL == pst_hmac_dev) || (OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user))
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_process:: null pointer!.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAP模式判断, 只有ap模式下才有CAR, 其他模式下，报文直接通过 */
    if (WLAN_VAP_MODE_BSS_AP != pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
         OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX, "{hmac_car_process:: CAR only used in AP mode; en_vap_mode=%d.}", pst_hmac_vap->st_vap_base_info.en_vap_mode);
         return OAL_SUCC;
    }

    us_mpdu_payload_len = (oal_uint16)oal_netbuf_get_len(pst_buf);

    pst_car_device_cfg = &pst_hmac_dev->st_car_device_cfg[en_car_type];
    pst_car_vap_cfg = &pst_hmac_vap->st_car_vap_cfg[en_car_type];
    pst_car_user_cfg = &pst_hmac_user->st_car_user_cfg[en_car_type];

    //上行 RX需要减去20
    if (HMAC_CAR_UPLINK == en_car_type)
    {
        us_mpdu_payload_len -= HMAC_CAR_WIFI_ETH;
    }

    ul_ret = hmac_car_judge_packet_drop(pst_hmac_vap, pst_car_device_cfg, pst_car_vap_cfg, pst_car_user_cfg, us_mpdu_payload_len);
    if (HMAC_TX_DROP_CAR_LIMIT == ul_ret)
    {
        //device下统计car丢包个数，在car enable的时候清空；
        pst_hmac_dev->ul_car_packet_drop_num[en_car_type]++;
        return HMAC_TX_DROP_CAR_LIMIT;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_car_multicast_process(hmac_device_stru *pst_hmac_dev, oal_netbuf_stru *pst_buf)

{
    oal_uint16                    us_mpdu_payload_len;
    hmac_car_limit_stru          *pst_car_device_multicast_cfg;
    oal_uint32                    ul_ret = OAL_SUCC;

    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_multicast_process:: pst_hmac_dev null pointer!.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    us_mpdu_payload_len = (oal_uint16)oal_netbuf_get_len(pst_buf);

    pst_car_device_multicast_cfg = &pst_hmac_dev->st_car_device_multicast_cfg;

    OAM_INFO_LOG4(0, OAM_SF_CAR, "{hmac_car_multicast_process:: en_car_limit_flag[%d] ul_car_limit_left_bytes[%d], us_mpdu_payload_len[%d], pps_left_num[%d]}",
                                    pst_car_device_multicast_cfg->en_car_limit_flag,
                                    pst_car_device_multicast_cfg->ul_car_limit_left_bytes,
                                    us_mpdu_payload_len,
                                    pst_car_device_multicast_cfg->ul_car_multicast_pps_num_left);

    if(OAL_FALSE == pst_car_device_multicast_cfg->en_car_limit_flag)
    {
        return OAL_SUCC;
    }

    //先判断pps数目
    if (pst_car_device_multicast_cfg->ul_car_orgin_multicast_pps_num)
    {
        if (!pst_car_device_multicast_cfg->ul_car_multicast_pps_num_left)
        {
            OAM_WARNING_LOG2(0, OAM_SF_CAR, "{hmac_car_multicast_process:: PPS: orgin_pps_num[%d], left_pps_num[%d]}",
                                            pst_car_device_multicast_cfg->ul_car_orgin_multicast_pps_num,
                                            pst_car_device_multicast_cfg->ul_car_multicast_pps_num_left);
            pst_hmac_dev->ul_car_multicast_packet_pps_drop_num++;
            return HMAC_TX_DROP_CAR_LIMIT;
        }
        pst_car_device_multicast_cfg->ul_car_multicast_pps_num_left--;
    }

    //再判断是否消耗令牌
    if(pst_car_device_multicast_cfg->ul_car_limit_kbps)
    {
        ul_ret = hmac_car_consume_process(pst_car_device_multicast_cfg, us_mpdu_payload_len);
        if (HMAC_TX_DROP_CAR_LIMIT == ul_ret)
        {
            pst_hmac_dev->ul_car_multicast_packet_drop_num++;
            OAM_WARNING_LOG2(0, OAM_SF_CAR, "{hmac_car_multicast_process:: left_bytes[%d], consume[%d]}", pst_car_device_multicast_cfg->ul_car_limit_left_bytes, us_mpdu_payload_len);
            return HMAC_TX_DROP_CAR_LIMIT;
        }
    }

    return OAL_SUCC;
}



oal_uint32  hmac_car_init(hmac_device_stru *pst_hmac_dev)
{
    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_init:: pst_hmac_dev pointer!.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* init */
    pst_hmac_dev->en_car_enable_flag = OAL_FALSE;
    pst_hmac_dev->us_car_ctl_cycle_ms = HMAC_CAR_CYCLE_MS;
    pst_hmac_dev->st_car_device_multicast_cfg.ul_car_orgin_multicast_pps_num = HMAC_CAR_MULTICAST_PPS_NUM;

    if (pst_hmac_dev->pst_device_base_info)
    {
        FRW_TIMER_CREATE_TIMER(&pst_hmac_dev->st_car_timer,
                               hmac_car_limit_timer_timeout_handle,
                               pst_hmac_dev->us_car_ctl_cycle_ms,
                               (oal_void *)pst_hmac_dev,
                               OAL_TRUE,
                               OAM_MODULE_ID_HMAC,
                               pst_hmac_dev->pst_device_base_info->ul_core_id);
        //若car未使能，则关闭定时器
        if(OAL_FALSE == pst_hmac_dev->en_car_enable_flag)
        {
            FRW_TIMER_STOP_TIMER(&pst_hmac_dev->st_car_timer);
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_car_exit(hmac_device_stru *pst_hmac_dev)
{
    if (OAL_PTR_NULL == pst_hmac_dev)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CAR, "{hmac_car_exit:: pst_hmac_dev pointer!.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_hmac_dev->st_car_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_dev->st_car_timer);
    }

    return OAL_SUCC;
}

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

