

#ifndef __HMAC_M2S_C__
#define __HMAC_M2S_C__

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

#ifdef _PRE_WLAN_FEATURE_M2S

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_ext_if.h"
#include "mac_data.h"
#include "hmac_resource.h"
#include "hmac_m2s.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "hmac_fsm.h"
#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_M2S_C

/*****************************************************************************
  2 函数声明
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_void hmac_m2s_vap_arp_probe_start(oal_void *p_arg)
{
    mac_vap_stru                 *pst_mac_vap = (mac_vap_stru *)p_arg;
    hmac_vap_stru                *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_m2s_vap_arp_probe_start_etc::pst_hmac_vap null.}");
        return;
    }

    pst_hmac_vap->st_hmac_vap_m2s.en_arp_probe_on = OAL_TRUE;
}


OAL_STATIC oal_uint32 hmac_m2s_arp_probe_timeout(oal_void *p_arg)
{
    hmac_vap_m2s_stru            *pst_hmac_vap_m2s;
    hmac_vap_stru                *pst_hmac_vap;
    oal_uint32                    ui_val;

    pst_hmac_vap = (hmac_vap_stru *)p_arg;
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap_m2s = &(pst_hmac_vap->st_hmac_vap_m2s);

    ui_val = oal_atomic_read(&(pst_hmac_vap_m2s->ul_rx_unicast_pkt_to_lan));
    if(0 == ui_val)
    {
        pst_hmac_vap_m2s->uc_rx_no_pkt_count++;

        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2S, "{hmac_m2s_arp_probe_timeout::rx_arp_pkt fail cnt[%d]!}",
            pst_hmac_vap_m2s->uc_rx_no_pkt_count);

        if(pst_hmac_vap_m2s->uc_rx_no_pkt_count > M2S_ARP_FAIL_REASSOC_NUM)
        {
            /* 重关联逻辑暂时关闭，等统计出现哪些场景出现不通，才限制场景放开 */
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_M2S, "{hmac_m2s_arp_probe_timeout::need to reassoc to resume.}");

            /* 停止arp探测 */
            pst_hmac_vap_m2s->en_arp_probe_on = OAL_FALSE;

            pst_hmac_vap_m2s->uc_rx_no_pkt_count = 0;

            /* 发起reassoc req */
            hmac_roam_start_etc(pst_hmac_vap, ROAM_SCAN_CHANNEL_ORG_0, OAL_TRUE, ROAM_TRIGGER_M2S);

#ifdef _PRE_WLAN_1103_CHR
            CHR_EXCEPTION_REPORT(CHR_PLATFORM_EXCEPTION_EVENTID, CHR_SYSTEM_WIFI, CHR_LAYER_DRV, CHR_WIFI_DRV_EVENT_MIMO_TO_SISO_FAIL, 0);
#endif
        }
    }
    else
    {
        /* 停止arp探测 */
        pst_hmac_vap_m2s->en_arp_probe_on = OAL_FALSE;

        pst_hmac_vap_m2s->uc_rx_no_pkt_count = 0;
    }

    oal_atomic_set(&pst_hmac_vap_m2s->ul_rx_unicast_pkt_to_lan, 0);

    return OAL_SUCC;
}


oal_void hmac_m2s_arp_fail_process(oal_netbuf_stru *pst_netbuf, oal_void *p_arg)
{
    hmac_vap_stru                    *pst_hmac_vap;
    hmac_vap_m2s_stru                *pst_hmac_vap_m2s;
    mac_ether_header_stru            *pst_mac_ether_hdr;
    oal_uint8                         uc_data_type;

    pst_hmac_vap = (hmac_vap_stru *)p_arg;

    pst_hmac_vap_m2s = &(pst_hmac_vap->st_hmac_vap_m2s);

    /* 只要统计功能打开，就需要做一次探测 */
    if(OAL_TRUE == pst_hmac_vap_m2s->en_arp_probe_on)
    {
        pst_mac_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_netbuf);

        /* 参数外面已经做检查，里面没必要再做检查了 */
        uc_data_type =  mac_get_data_type_from_8023_etc((oal_uint8 *)pst_mac_ether_hdr, MAC_NETBUFF_PAYLOAD_ETH);

        /* 发送方向创建定时器，多次创建定时器 */
        if((MAC_DATA_ARP_REQ == uc_data_type) && (OAL_FALSE == pst_hmac_vap_m2s->st_arp_probe_timer.en_is_registerd))
        {
            /* 每次重启定时器之前清零,保证统计的时间 */
            oal_atomic_set(&(pst_hmac_vap_m2s->ul_rx_unicast_pkt_to_lan), 0);

            FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap_m2s->st_arp_probe_timer),
                       hmac_m2s_arp_probe_timeout,
                       M2S_ARP_PROBE_TIMEOUT,
                       pst_hmac_vap,
                       OAL_FALSE,
                       OAM_MODULE_ID_HMAC,
                       pst_hmac_vap->st_vap_base_info.ul_core_id);
        }
    }
}

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of __HMAC_M2S_C__ */

