


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif
#ifdef _PRE_WLAN_CFGID_DEBUG

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_types.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"
#include "wlan_types.h"

#include "mac_device.h"
#include "mac_vap.h"
#include "mac_resource.h"
//#include "mac_11i.h"

#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_scan.h"
#include "hmac_ext_if.h"
#include "hmac_config.h"
#include "wal_ext_if.h"
#include "wal_main.h"
#include "wal_config.h"
#include "wal_linux_bridge.h"
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

#include "hmac_cali_mgmt.h"
#endif

#ifdef _PRE_WLAN_CHIP_TEST
#include "hmac_test_main.h"
#endif

#ifdef _PRE_WLAN_DFT_REG
#include "hal_witp_debug.h"
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
#include "hmac_wapi.h"
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
#include "hmac_fbt_main.h"
#endif
#ifdef _PRE_WLAN_FEATURE_WMMAC
#include "hmac_wmmac.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_WAL_CONFIG_DEBUG_C

/*****************************************************************************
  2 结构体定义
*****************************************************************************/
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*****************************************************************************
  3 函数实现
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

OAL_STATIC oal_uint32  wal_config_get_hipkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_hipkt_stat(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_flowctl_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_flowctl_param(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_flowctl_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_flowctl_stat(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_get_fem_pa_status(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_fem_pa_status(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST

OAL_STATIC oal_uint32  wal_config_chip_check(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_chip_check(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_event_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_event_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_eth_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_eth_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_80211_ucast_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_80211_ucast_switch(pst_mac_vap, us_len, puc_param);
}


#ifdef _PRE_DEBUG_MODE_USER_TRACK

OAL_STATIC oal_uint32  wal_config_report_thrput_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_report_thrput_stat(pst_mac_vap, us_len, puc_param);
}

#endif


#ifdef _PRE_WLAN_FEATURE_TXOPPS


OAL_STATIC oal_uint32  wal_config_set_txop_ps_machw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_txop_ps_machw(pst_mac_vap, us_len, puc_param);
}

#endif


OAL_STATIC oal_uint32  wal_config_80211_mcast_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_80211_mcast_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_probe_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_probe_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_rssi_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_rssi_switch(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_report_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_report_vap_info(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#if 0

OAL_STATIC oal_uint32  wal_config_ota_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_ota_switch(pst_mac_vap, us_len, puc_param);
}
#endif
#endif

#ifdef _PRE_WLAN_RF_110X_CALI_DPD
OAL_STATIC oal_uint32  wal_config_start_dpd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
   return hmac_config_start_dpd(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  wal_config_beacon_offload_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_beacon_offload_test(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_ota_beacon_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_ota_beacon_switch(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_ota_rx_dscr_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_ota_rx_dscr_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_all_ota(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_all_ota(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_dhcp_arp_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_dhcp_arp_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_oam_output(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_oam_output(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_ampdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_ampdu_start(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_amsdu_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_amsdu_start(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_ampdu_end(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_ampdu_end(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_auto_ba_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_auto_ba_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_amsdu_ampdu_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_amsdu_ampdu_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_profiling_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_profiling_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_addba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_addba_req(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_delba_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_delba_req(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_WMMAC

OAL_STATIC oal_uint32  wal_config_addts_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_addts_req(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_delts(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_delts(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_wmmac_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_wmmac_switch(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_reassoc_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_reassoc_req(pst_mac_vap);
}

#endif

OAL_STATIC oal_uint32  wal_config_voe_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_voe_enable(pst_mac_vap, us_len, puc_param);
}
#ifdef _PRE_WLAN_FEATURE_11K
OAL_STATIC oal_uint32  wal_config_bcn_table_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_bcn_table_switch(pst_mac_vap, us_len, puc_param);
}

#endif

OAL_STATIC oal_uint32  wal_config_list_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_list_ap(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_list_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_list_sta(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_list_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_list_channel(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_start_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_sta_initiate_scan(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_start_join(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_bss_dscr_stru   *pst_bss_dscr;
    oal_uint32           ul_bss_idx;

    ul_bss_idx = (oal_uint8)oal_atoi((oal_int8 *)puc_param);

    /* ??bss???,??????bss dscr??? */
    pst_bss_dscr = hmac_scan_find_scanned_bss_dscr_by_index(pst_mac_vap->uc_device_id, ul_bss_idx);
    if (OAL_PTR_NULL == pst_bss_dscr)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_start_join::find bss failed by index!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    return hmac_sta_initiate_join(pst_mac_vap, pst_bss_dscr);
}


OAL_STATIC oal_uint32  wal_config_start_deauth(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_config_send_deauth(pst_mac_vap, pst_mac_vap->auc_bssid);
    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_dump_timer(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dump_timer(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_pause_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_pause_tid(pst_mac_vap, us_len, puc_param);
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_user_vip(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_user_vip(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_vap_host(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_vap_host(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_send_bar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_bar(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_alg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_alg(pst_mac_vap, us_len, puc_param);
}



oal_netbuf_stru*  wal_config_create_packet_ap(oal_uint32    ul_size,
                                              oal_uint8     uc_tid,
                                              oal_uint8    *puc_mac_ra,
                                              oal_uint8    *puc_mac_ta)
{
    oal_netbuf_stru         *pst_buf;
    mac_ether_header_stru   *pst_ether_header;
    mac_ip_header_stru      *pst_ip;
    oal_uint32               ul_loop = 0;
    oal_uint32               l_reserve = 256;
    pst_buf = oal_netbuf_alloc(ul_size+l_reserve, (oal_int32)l_reserve, 4);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_buf))
    {
        return OAL_PTR_NULL;
    }

    oal_netbuf_put(pst_buf, ul_size);
    oal_set_mac_addr(&pst_buf->data[0], puc_mac_ra);
    oal_set_mac_addr(&pst_buf->data[6], puc_mac_ta);

    /* 帧体内容 最后6个字节保持为0x00*/
    for (ul_loop = 0; ul_loop < ul_size - 20; ul_loop++)
    {
        pst_buf->data[14 + ul_loop] = (oal_uint8)ul_loop;
    }

    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    /*lint -e778*/
    pst_ether_header->us_ether_type = OAL_HOST2NET_SHORT(ETHER_TYPE_IP);
    /*lint +e778*/
    pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);      /* 偏移一个以太网头，取ip头 */

    pst_ip->uc_tos = (oal_uint8)(uc_tid << WLAN_IP_PRI_SHIFT);

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    pst_ip->uc_protocol = MAC_UDP_PROTOCAL;
#endif

    pst_buf->next = OAL_PTR_NULL;
    pst_buf->prev = OAL_PTR_NULL;

    OAL_MEMZERO(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE());

    return pst_buf;

}



OAL_STATIC oal_uint32  wal_config_packet_xmit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_ampdu_tx_on_param;
    oal_uint8                       uc_skb_num;
    oal_uint8                       uc_skb_idx;
    oal_netbuf_stru                *pst_netbuf;
    oal_uint8                       uc_tid;
    oal_uint16                      us_packet_len;
    oal_net_device_stru            *pst_dev;
    oal_netbuf_stru                *past_netbuf[32] = {OAL_PTR_NULL};

    pst_ampdu_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)puc_param;
    uc_skb_num            = pst_ampdu_tx_on_param->uc_packet_num;
    uc_tid                = pst_ampdu_tx_on_param->uc_tid;
    us_packet_len         = pst_ampdu_tx_on_param->us_packet_len;

    pst_dev = hmac_vap_get_net_device(pst_mac_vap->uc_vap_id);

    /*为profiling测试修改，只测量xmit时间*/
    if (uc_skb_num <= 32)
    {
        for (uc_skb_idx = 0; uc_skb_idx < uc_skb_num; uc_skb_idx++)
        {
            past_netbuf[uc_skb_idx] = wal_config_create_packet_ap(us_packet_len, uc_tid,
                                                    pst_ampdu_tx_on_param->auc_ra_mac,
                                                    pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

        }

        OAM_PROFILING_TX_STATISTIC(OAL_PTR_NULL, OAM_PROFILING_FUNC_CONFIG_XMIT_START);

        for (uc_skb_idx = 0; uc_skb_idx < uc_skb_num; uc_skb_idx++)
        {
            #ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
                wal_vap_start_xmit(past_netbuf[uc_skb_idx], pst_dev);
            #else
                wal_bridge_vap_xmit(past_netbuf[uc_skb_idx], pst_dev);
            #endif
        }

        OAM_PROFILING_TX_STATISTIC(OAL_PTR_NULL, OAM_PROFILING_FUNC_CONFIG_XMIT_END);

    }
    else
    {
        for (uc_skb_idx = 0; uc_skb_idx < uc_skb_num; uc_skb_idx++)
        {
            pst_netbuf = wal_config_create_packet_ap(us_packet_len, uc_tid,
                                                    pst_ampdu_tx_on_param->auc_ra_mac,
                                                    pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
            #ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
                wal_vap_start_xmit(pst_netbuf, pst_dev);
            #else
                wal_bridge_vap_xmit(pst_netbuf, pst_dev);
            #endif
        }
    }

    return OAL_SUCC;
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_dump_ba_bitmap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dump_ba_bitmap(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  wal_config_timer_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8           uc_timer_switch;

    uc_timer_switch = *(puc_param);

    if (OAL_TRUE == uc_timer_switch)
    {
        oal_5115timer_init();
    }
    else
    {
        oal_5115timer_exit();
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_show_profiling(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_PROFILING_MODE

    oal_uint32          ul_value;

    ul_value = *((oal_uint32 *)puc_param);

    if (OAM_PROFILING_RX == ul_value)
    {
        oam_profiling_rx_show_offset();
    }
    else if (OAM_PROFILING_TX == ul_value)
    {
        oam_profiling_tx_show_offset();
    }
    else if (OAM_PROFILING_ALG == ul_value)
    {
        oam_profiling_alg_show_offset();
    }
    else if (OAM_PROFILING_CHIPSTART == ul_value)
    {
        oam_profiling_starttime_show_offset();
    }
    else if (OAM_PROFILING_CHSWTICH == ul_value)
    {
        oam_profiling_chswitch_show_offset();
    }
    else
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_show_profiling:: invalide profiling type %d!}\r\n", ul_value);
    }
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_amsdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_amsdu_tx_on(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_ampdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_ampdu_tx_on(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_txbf_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    mac_cfg_ampdu_tx_on_param_stru *pst_txbf_on_param;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_txbf_switch:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_txbf_on_param = (mac_cfg_ampdu_tx_on_param_stru *)puc_param;

    if (1 == pst_txbf_on_param->uc_aggr_tx_on || 0 == pst_txbf_on_param->uc_aggr_tx_on)
    {
        //pst_mac_vap->st_cap_flag.bit_bfer = pst_txbf_on_param->uc_aggr_tx_on;
    }
    else
    {

        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_txbf_switch:: pst_ampdu_tx_on_param->uc_aggr_tx_on %d!}\r\n", pst_txbf_on_param->uc_aggr_tx_on);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_reset_hw(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_reset_operate(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_reset_operate(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_dfr_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dfr_enable(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_trig_pcie_reset(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_trig_pcie_reset(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_trig_loss_tx_comp(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_trig_loss_tx_comp(pst_mac_vap, us_len, puc_param);
}
#endif
#endif


#ifdef _PRE_WLAN_FEATURE_UAPSD
OAL_STATIC oal_uint32  wal_config_uapsd_debug(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
     return  hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_UAPSD_DEBUG,us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_DFT_STAT
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_phy_stat_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_phy_stat_en(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_dbb_env_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dbb_env_param(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_usr_queue_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_usr_queue_stat(pst_mac_vap, us_len, puc_param);
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_report_vap_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_report_vap_stat(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_report_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_report_all_stat(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

#endif


OAL_STATIC oal_uint32  wal_config_dump_rx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dump_rx_dscr(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_dump_tx_dscr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_dump_tx_dscr(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_dump_memory(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_dump_memory_stru *pst_param;
    oal_uint8                *puc_addr;
    oal_uint32                ul_i;

    pst_param = (mac_cfg_dump_memory_stru *)puc_param;
    puc_addr  = (oal_uint8 *)(pst_param->ul_addr);

    for (ul_i = 0; ul_i < pst_param->ul_len; ul_i++)
    {
        OAL_IO_PRINT("%02x ", puc_addr[ul_i]);
        if (ul_i && (ul_i % 20 == 0))
        {
            OAL_IO_PRINT("\n");
        }
    }

    OAL_IO_PRINT("\n");

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_11D

OAL_STATIC oal_uint32  wal_config_set_rd_by_ie_switch(mac_vap_stru *pst_mac_vap,oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_rd_by_ie_switch(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_vap_pkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_vap_pkt_stat(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_set_feature_log(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_FEATURE_LOG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_feature_log::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32 wal_config_set_log_lowpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_SET_LOG_PM, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_set_feature_log::hmac_config_send_event failed[%d].}", ul_ret);
    }

    return ul_ret;

}
#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_nss::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_nss(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_nss::hmac_config_set_nss error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_rfch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_rfch::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置参数 */
    ul_ret = hmac_config_set_rfch(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_rfch::hmac_config_set_rfch error!}\r\n");
        return ul_ret;
    }

    return ul_ret;
}
#endif //#ifdef _PRE_DEBUG_MODE


oal_netbuf_stru*  wal_config_create_al_tx_packet(oal_uint32 ul_size,
                                                    oal_uint8 uc_tid,
                                                    mac_rf_payload_enum_uint8        en_payload_flag,
                                                    oal_bool_enum_uint8 en_init_flag)
{
    static oal_netbuf_stru         *pst_buf = OAL_PTR_NULL;
    oal_uint32               ul_loop = 0;
    oal_uint32               l_reserve = 256;

    if (OAL_TRUE == en_init_flag)
    {
        pst_buf = oal_netbuf_alloc(ul_size+l_reserve, (oal_int32)l_reserve, 4);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_buf))
        {
            OAM_ERROR_LOG0(0, OAM_SF_TX, "wal_config_create_al_tx_packet::alloc Fail");
            return OAL_PTR_NULL;
        }
        oal_netbuf_put(pst_buf, ul_size);
    }

    if (OAL_PTR_NULL == pst_buf)
    {
        OAM_ERROR_LOG0(0, OAM_SF_TX, "wal_config_create_al_tx_packet::pst_buf is not initiate");
        return OAL_PTR_NULL;
    }

    switch(en_payload_flag)
    {
        case RF_PAYLOAD_ALL_ZERO:
            OAL_MEMZERO(pst_buf->data, ul_size);
            break;
        case RF_PAYLOAD_ALL_ONE:
            oal_memset(pst_buf->data, 0xFF, ul_size);
            break;
        case RF_PAYLOAD_RAND:
            pst_buf->data[0] = oal_gen_random(18, 1);
            for (ul_loop = 1; ul_loop < ul_size; ul_loop++)
            {
                pst_buf->data[ul_loop] = oal_gen_random(18, 0);
            }
            break;
        default:
        	break;
    }

    pst_buf->next = OAL_PTR_NULL;
    pst_buf->prev = OAL_PTR_NULL;

    OAL_MEMZERO(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE());
    return pst_buf;
}
#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_bcast_pkt(mac_vap_stru *pst_mac_vap, oal_uint32 ul_payload_len)
{
    oal_netbuf_stru                *pst_netbuf;
    hmac_vap_stru                  *pst_hmac_vap;
    oal_uint8                       uc_tid;
    oal_uint32                      ul_ret = OAL_SUCC;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_bcast_pkt::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{wal_config_bcast_pkt::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_tid = 0;

    /* 组包 */
    pst_netbuf = wal_config_create_al_tx_packet(ul_payload_len, uc_tid, (oal_uint8)pst_mac_vap->bit_payload_flag, (oal_uint8)pst_hmac_vap->bit_init_flag);

    if (OAL_PTR_NULL == pst_netbuf)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_bcast_pkt::return null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(OAL_NETBUF_CB(pst_netbuf), OAL_NETBUF_CB_SIZE());

    ul_ret = hmac_tx_lan_to_wlan(pst_mac_vap, pst_netbuf);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_bcast_pkt::hmac_tx_lan_to_wlan return error %d!}\r\n", ul_ret);
    }
    return ul_ret;
}


OAL_STATIC oal_uint32  wal_config_always_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;
    mac_cfg_tx_comp_stru            *pst_event_set_bcast;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_always_tx::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event_set_bcast = (mac_cfg_tx_comp_stru *)puc_param;

    ul_ret = hmac_config_always_tx(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_always_tx::hmac_config_always_tx failed!}\r\n");
        return ul_ret;
    }

    if (OAL_SWITCH_ON == pst_event_set_bcast->uc_param)
    {
        ul_ret = wal_config_bcast_pkt(pst_mac_vap, pst_event_set_bcast->ul_payload_len);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_always_tx::wal_config_bcast_pkt failed!}\r\n");
            return ul_ret;
        }
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_rxch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_rxch::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_rxch(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_rxch::hmac_config_set_rxch failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_dync_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_dync_txpower::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_dync_txpower(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_dync_txpower::hmac_config_dync_txpower failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_get_thruput(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_get_thruput::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_get_thruput(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_get_thruput::hmac_config_get_thruput failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_freq_skew(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_set_freq_skew::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_set_freq_skew(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_set_freq_skew::hmac_config_set_freq_skew failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_adjust_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{wal_config_adjust_ppm::pst_mac_vap/puc_param is null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_adjust_ppm(pst_mac_vap, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_adjust_ppm::hmac_config_set_freq_skew failed!}\r\n");
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif //#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  wal_config_dbb_scaling_amend(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
     return hmac_config_dbb_scaling_amend(pst_mac_vap, us_len, puc_param);
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_beacon_chain_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_beacon_chain_switch(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  wal_config_2040_channel_switch_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_2040_channel_switch_prohibited(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_FortyMHzIntolerant(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_FortyMHzIntolerant(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_2040_coext_support(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_2040_coext_support(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_config_resume_rx_intr_fifo(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_resume_rx_intr_fifo(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_SUPPORT_ACS

OAL_STATIC oal_uint32  wal_config_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_acs(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_PERFORM_STAT

OAL_STATIC oal_uint32  wal_config_pfm_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_pfm_stat(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_pfm_display(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_pfm_display(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_CHIP_TEST

OAL_STATIC oal_uint32  wal_config_lpm_tx_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_lpm_tx_data(pst_mac_vap,us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_lpm_tx_probe_request(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return  hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_TX_PROBE_REQUEST,us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_set_lpm_chip_state(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_CHIP_STATE ,us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_set_lpm_soc_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_SOC_MODE ,us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_lpm_psm_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_PSM_PARAM ,us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_set_lpm_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_SMPS_MODE ,us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_lpm_smps_stub(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_SMPS_STUB ,us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_lpm_txop_ps(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_TXOP_PS_SET ,us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_set_lpm_txop_tx_stub(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_LPM_TXOP_TX_STUB ,us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_remove_user_lut(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_REMOVE_LUT, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_send_frame(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SEND_FRAME, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_rx_pn(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_RX_PN_REG, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_soft_retry(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_SOFT_RETRY, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_open_addr4(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_OPEN_ADDR4, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_open_wmm_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_OPEN_WMM_TEST, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_chip_test_open(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_CHIP_TEST
        /* 设置一下hmac的芯片验证开关 */
        hmac_test_set_chip_test(*(oal_uint8*)puc_param);
#endif

    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_CHIP_TEST_OPEN, us_len, puc_param);
}



OAL_STATIC oal_uint32 wal_config_set_coex(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /*通过函数调用，hmac具体实现*/

#ifdef _PRE_WLAN_CHIP_TEST
    return (hmac_config_set_coex(pst_mac_vap, us_len, puc_param));
#else
    return OAL_SUCC;
#endif
}


OAL_STATIC oal_uint32  wal_config_set_dfx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_CHIP_TEST
    return hmac_config_set_dfx(pst_mac_vap, us_len, puc_param);
#else
     return OAL_SUCC;
#endif
}



#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)


OAL_STATIC oal_uint32 wal_config_enable_pmf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /* 设置一下hmac的芯片验证开关 */
    hmac_enable_pmf(pst_mac_vap, puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_PMF_ENABLE, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}
#endif


OAL_STATIC oal_uint32 wal_config_send_action(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_test_send_action(pst_mac_vap, puc_param);
}


OAL_STATIC oal_uint32  wal_config_send_pspoll(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_pspoll(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_send_nulldata(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_nulldata(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_clear_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_clear_all_stat(pst_mac_vap, us_len, puc_param);
}

#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_hide_ssid(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  wal_config_set_mib(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_mib(pst_mac_vap, WLAN_CFGID_SET_MIB, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_mib(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_mib(pst_mac_vap, WLAN_CFGID_GET_MIB, us_len, puc_param);
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_thruput_bypass(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_thruput_bypass(pst_mac_vap, WLAN_CFGID_SET_THRUPUT_BYPASS, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE


OAL_STATIC oal_uint32  wal_config_set_auto_protection(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8 uc_auto_protection_flag;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_auto_protection_flag = (oal_uint8)*((oal_uint32 *)puc_param);

    return hmac_config_set_auto_protection(pst_mac_vap, uc_auto_protection_flag);
}


OAL_STATIC oal_uint32  wal_config_send_2040_coext(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_2040_coext(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_2040_coext_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_2040_coext_info(pst_mac_vap, us_len, puc_param);
}



#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_config_get_all_reg_value(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_all_reg_value(pst_mac_vap, us_len, puc_param);
}

#endif
#ifdef _PRE_WLAN_FEATURE_DAQ

OAL_STATIC oal_uint32  wal_config_data_acq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_data_acq(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_get_smps_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_smps_info(pst_mac_vap, us_len, puc_param);
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

oal_uint32  wal_config_set_oma(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_oma(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_proxysta_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_proxysta_switch(pst_mac_vap, us_len, puc_param);
}

#endif

#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY


OAL_STATIC oal_uint32  wal_config_set_opmode_notify(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_opmode_notify(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_user_rssbw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_user_rssbw(pst_mac_vap, us_len, puc_param);
}

#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_vap_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_vap_nss(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

#ifdef _PRE_WLAN_DFT_REG

oal_uint32  wal_config_dump_reg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru            *pst_mac_dev = OAL_PTR_NULL;
    hal_to_dmac_device_stru    *pst_hal_dmac_dev = OAL_PTR_NULL;

    pst_mac_dev = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{wal_config_dump_reg::pst_device[id:%d] is NULL!}",
                pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hal_dmac_dev = pst_mac_dev->pst_device_stru;
	if(OAL_SUCC != hi1151_debug_refresh_reg(pst_hal_dmac_dev, OAM_REG_EVT_USR))
    {
        return OAL_SUCC;
    }
    hi1151_debug_frw_evt(pst_hal_dmac_dev);
	return OAL_SUCC;
}
#endif
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_get_mpdu_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_mpdu_num(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY

OAL_STATIC oal_uint32  wal_config_isolation_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_show_isolation(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_autoblacklist_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_autoblacklist_enable(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_autoblacklist_aging(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_autoblacklist_aging(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_autoblacklist_threshold(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_autoblacklist_threshold(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_autoblacklist_reset_time(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_autoblacklist_reset_time(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_isolation_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_isolation_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_isolation_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_isolation_type(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_isolation_forward(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_isolation_forword(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_isolation_clear(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_isolation_clear(pst_mac_vap, us_len, puc_param);
}
#endif


#ifdef _PRE_WLAN_FEATURE_PROXY_ARP

#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_config_proxyarp_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_proxy_display_info(pst_mac_vap);
    return OAL_SUCC;
}
#endif/* #ifdef _PRE_DEBUG_MODE */

#endif

#ifdef _PRE_WLAN_FEATURE_WAPI

#ifdef _PRE_WAPI_DEBUG
OAL_STATIC oal_uint32  wal_config_wapi_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_user_info_param_stru *pst_user_info;
    pst_user_info = (mac_cfg_user_info_param_stru *)puc_param;

    hmac_wapi_display_info(pst_mac_vap, pst_user_info->us_user_idx);
    return OAL_SUCC;
}
#endif/* #ifdef _PRE_DEBUG_MODE */

#endif /* _PRE_WLAN_FEATURE_WAPI */

#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_rx_filter_val(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_rx_filter_val(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_rx_filter_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_rx_filter_en(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_get_rx_filter_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_rx_filter_en(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_report_ampdu_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_report_ampdu_stat(pst_mac_vap, us_len, puc_param);
}

#endif


OAL_STATIC oal_uint32  wal_config_set_ampdu_aggr_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_ampdu_aggr_num(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_config_set_ampdu_mmss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_ampdu_mmss(pst_mac_vap, us_len, puc_param);
}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

OAL_STATIC oal_uint32  wal_config_freq_adjust(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_freq_adjust(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_set_stbc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_stbc_cap(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_set_ldpc_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_ldpc_cap(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_MCAST

OAL_STATIC oal_uint32  wal_config_m2u_snoop_on(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_m2u_snoop_on_param_stru *pst_m2u_snoop_on_param;
    hmac_vap_stru *pst_hmac_vap;
    hmac_m2u_stru *pst_m2u;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    pst_m2u = (hmac_m2u_stru *)(pst_hmac_vap->pst_m2u);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_config_m2u_snoop_on:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_m2u_snoop_on_param = (mac_cfg_m2u_snoop_on_param_stru *)puc_param;

	/* uc_m2u_snoop_on */
    if (1 == pst_m2u_snoop_on_param->uc_m2u_snoop_on || 0 == pst_m2u_snoop_on_param->uc_m2u_snoop_on)
    {
        pst_m2u->en_snoop_enable = pst_m2u_snoop_on_param->uc_m2u_snoop_on;
    }
	else
	{
		OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U, "{wal_config_m2u_snoop_on::pst_m2u_snoop_on_param->uc_m2u_snoop_on %d!}\r\n", pst_m2u_snoop_on_param->uc_m2u_snoop_on);
		return OAL_FAIL;
	}

	/* uc_m2u_mcast_mode */
    if (1 == pst_m2u_snoop_on_param->uc_m2u_mcast_mode || 0 == pst_m2u_snoop_on_param->uc_m2u_mcast_mode ||
                2 == pst_m2u_snoop_on_param->uc_m2u_mcast_mode)
    {
        pst_m2u->en_mcast_mode = pst_m2u_snoop_on_param->uc_m2u_mcast_mode;
    }
    else
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U, "{wal_config_m2u_snoop_on::pst_m2u_snoop_on_param->uc_m2u_mcast_mode %d!}\r\n", pst_m2u_snoop_on_param->uc_m2u_mcast_mode);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_add_m2u_deny_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_add_m2u_deny_table_stru *pst_m2u_deny_table_param;
    hmac_vap_stru *pst_hmac_vap;
    oal_uint32  ul_deny_group_addr1;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_add_m2u_deny_table:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_m2u_deny_table_param = (mac_add_m2u_deny_table_stru *)puc_param;

    ul_deny_group_addr1 = oal_byteorder_host_to_net_uint32(pst_m2u_deny_table_param->ul_deny_group_addr);

    if (HMAC_M2U_MIN_DENY_GROUP <= ul_deny_group_addr1 && HMAC_M2U_MAX_DENY_GROUP >= ul_deny_group_addr1)
    {
        hmac_m2u_add_snoop_deny_entry(pst_hmac_vap, &ul_deny_group_addr1);
    }
    else
    {
    	OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U, "{wal_add_m2u_deny_table::pst_m2u_deny_table_param->ul_deny_group_addr %x!}\r\n", pst_m2u_deny_table_param->ul_deny_group_addr);
    	return OAL_FAIL;
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_config_m2u_deny_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_clg_m2u_deny_table_stru *pst_clg_m2u_deny_table_param;
    hmac_vap_stru *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_config_m2u_deny_table:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_clg_m2u_deny_table_param = (mac_clg_m2u_deny_table_stru *)puc_param;

    /* 清空组播组黑名单 */
    if (1 == pst_clg_m2u_deny_table_param->uc_m2u_clear_deny_table)
    {
        hmac_m2u_clear_deny_table(pst_hmac_vap);
    }
    /* show组播组黑名单 */
    if (1 == pst_clg_m2u_deny_table_param->uc_m2u_show_deny_table)
    {
        hmac_m2u_show_snoop_deny_table(pst_hmac_vap);
    }
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  wal_show_m2u_snoop_table(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_show_m2u_snoop_table_stru *pst_show_m2u_snoop_table_param;
    hmac_vap_stru *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_M2U, "{wal_show_m2u_snoop_table:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_show_m2u_snoop_table_param = (mac_show_m2u_snoop_table_stru *)puc_param;

    /* show snoop 表 */
    if (1 == pst_show_m2u_snoop_table_param->uc_m2u_show_snoop_table)
    {
        hmac_m2u_print_all_snoop_list(pst_hmac_vap);
    }
    else
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2U, "{wal_show_m2u_snoop_table::pst_m2u_snoop_on_param->uc_m2u_snoop_on %d!}\r\n", pst_show_m2u_snoop_table_param->uc_m2u_show_snoop_table);
        return OAL_FAIL;
    }
    return OAL_SUCC;
}



oal_netbuf_stru*  wal_config_create_igmp_packet(oal_uint32 ul_size,
                                                    oal_uint8 uc_tid,
                                                    oal_uint8 *puc_mac_ra,
                                                    oal_uint8 *puc_mac_ta)
{
    oal_netbuf_stru         *pst_buf;
    mac_ether_header_stru   *pst_ether_header;
    mac_ip_header_stru      *pst_ip;
    oal_uint32               ul_loop = 0;
    oal_uint32               l_reserve = 256;
    mac_igmp_header_stru     *pst_igmp_hdr;                         /* igmp header for v1 v2 */
    pst_buf = oal_netbuf_alloc(ul_size+l_reserve, (oal_int32)l_reserve, 4);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_buf))
    {
        return OAL_PTR_NULL;
    }

    oal_netbuf_put(pst_buf, ul_size);
    oal_set_mac_addr(&pst_buf->data[0], puc_mac_ra);
    oal_set_mac_addr(&pst_buf->data[6], puc_mac_ta);

    /* 帧体内容 最后6个字节保持为0x00*/
    for (ul_loop = 0; ul_loop < ul_size - 50; ul_loop++)
    {
        pst_buf->data[14 + ul_loop] = (oal_uint8)ul_loop;
    }

    pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    /*lint -e778*/
    pst_ether_header->us_ether_type = OAL_HOST2NET_SHORT(ETHER_TYPE_IP);
    /*lint +e778*/
    pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);      /* 偏移一个以太网头，取ip头 */
    pst_ip->uc_version_ihl = 0x45;
    pst_ip->uc_protocol = IPPROTO_IGMP;
    //pst_ip->ul_saddr = oal_byteorder_host_to_net_uint32(0xc064642c);     /* STA的IP地址 192.100.100.44 */
    //pst_ip->ul_daddr = oal_byteorder_host_to_net_uint32(0xe0804020);     /* 组播组的IP地址 224.128.64.32 */

    pst_ip->uc_tos = (oal_uint8)(uc_tid << WLAN_IP_PRI_SHIFT);
    /* 指向igmp头指针 */
    pst_igmp_hdr = (mac_igmp_header_stru *)(pst_ip + 1);
    pst_igmp_hdr->uc_type = MAC_IGMPV2_REPORT_TYPE;
    pst_igmp_hdr->ul_group = oal_byteorder_host_to_net_uint32(0xe0804020);

    pst_buf->next = OAL_PTR_NULL;
    pst_buf->prev = OAL_PTR_NULL;

    OAL_MEMZERO(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE());

    return pst_buf;

}



OAL_STATIC oal_uint32  wal_config_igmp_packet_xmit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mpdu_ampdu_tx_param_stru *pst_ampdu_tx_on_param;
    oal_uint8                       uc_skb_num;
    oal_uint8                       uc_skb_idx;
    oal_uint8                       uc_tid;
    oal_uint16                      us_packet_len;
    oal_net_device_stru            *pst_dev;
    oal_netbuf_stru                *past_netbuf[32] = {OAL_PTR_NULL};

    pst_ampdu_tx_on_param = (mac_cfg_mpdu_ampdu_tx_param_stru *)puc_param;
    uc_skb_num            = pst_ampdu_tx_on_param->uc_packet_num;
    uc_tid                = pst_ampdu_tx_on_param->uc_tid;
    us_packet_len         = pst_ampdu_tx_on_param->us_packet_len;

    pst_dev = hmac_vap_get_net_device(pst_mac_vap->uc_vap_id);

    if (uc_skb_num <= 32)
    {
        for (uc_skb_idx = 0; uc_skb_idx < uc_skb_num; uc_skb_idx++)
        {
            past_netbuf[uc_skb_idx] = wal_config_create_igmp_packet(us_packet_len, uc_tid,
                                                    pst_ampdu_tx_on_param->auc_ra_mac,
                                                    pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
            #ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
                wal_vap_start_xmit(past_netbuf[uc_skb_idx], pst_dev);
            #else
                wal_bridge_vap_xmit(past_netbuf[uc_skb_idx], pst_dev);
            #endif

        }


    }
    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_FEATURE_PM
OAL_STATIC oal_uint32  wal_config_wifi_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{wal_config_wifi_enable::pst_mac_vap or puc_param null ptr error [%x],[%x].}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_config_wifi_enable(pst_mac_vap, us_len, puc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{wal_config_wifi_enable:: return error code [%d].}\r\n", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  wal_config_pm_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_PM_INFO,us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_pm_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap,WLAN_CFGID_PM_EN,us_len, puc_param);
}

#endif


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_DEBUG_MODE
OAL_STATIC oal_uint32  wal_config_show_device_memleak(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_DEVICE_MEM_LEAK, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_show_device_meminfo(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_DEVICE_MEM_INFO, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM
OAL_STATIC oal_uint32  wal_config_set_pm_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common(pst_mac_vap, WLAN_CFGID_SET_PSM_PARAM, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_vap_classify_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_vap_classify_en(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_vap_classify_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_vap_classify_tid(pst_mac_vap, us_len, puc_param);
}
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_scan_test(pst_mac_vap, us_len, puc_param);
}
#endif


OAL_STATIC oal_uint32  wal_config_bgscan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_bgscan_enable(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_mcs_set_check_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_mcs_set_check_enable(pst_mac_vap, us_len, puc_param);
}

#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_p2p_ps_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_p2p_ps_stat(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS

OAL_STATIC oal_uint32  wal_config_set_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_mips(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32  wal_config_show_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_show_mips(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_enable_arp_offload(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32 wal_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_show_arpoffload_info(pst_mac_vap, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_WLAN_TCP_OPT

OAL_STATIC oal_uint32  wal_config_get_tcp_ack_stream_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_get_tcp_ack_stream_info(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_tx_tcp_ack_opt_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_tx_tcp_ack_opt_enable(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_rx_tcp_ack_opt_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_rx_tcp_ack_opt_enable(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_tx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_tx_tcp_ack_limit(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_rx_tcp_ack_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_rx_tcp_ack_limit(pst_mac_vap, us_len, puc_param);
}
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44)) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_DFT_STAT
#ifdef _PRE_DEBUG_MODE

OAL_STATIC oal_uint32  wal_config_set_performance_log_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_performance_log_switch(pst_mac_vap, WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, us_len, puc_param);
}
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM

OAL_STATIC oal_uint32  wal_config_roam_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_roam_enable(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_roam_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_roam_band(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_roam_org(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_roam_org(pst_mac_vap, us_len, puc_param);
}

OAL_STATIC oal_uint32  wal_config_roam_start(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_roam_start(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32  wal_config_roam_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_roam_info(pst_mac_vap, us_len, puc_param);
}
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

OAL_STATIC oal_uint32  wal_config_enable_2040bss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_enable_2040bss(pst_mac_vap, us_len, puc_param);
}
#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

OAL_STATIC oal_uint32  wal_config_set_txrx_chain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_txrx_chain(pst_mac_vap, us_len, puc_param);
}
#endif


#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
OAL_STATIC oal_uint32  wal_config_set_auto_freq_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_auto_freq_enable(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK


OAL_STATIC oal_uint32 wal_config_fbt_set_mode(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
   return hmac_config_fbt_set_mode(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_fbt_scan_list_clear(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_scan_list_clear(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_fbt_scan_specified_sta(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_scan_specified_sta(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32 wal_config_fbt_print_scan_list(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_print_scan_list(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32 wal_config_fbt_scan_enable(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_scan_enable(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32 wal_config_fbt_scan_interval(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_scan_interval(pst_mac_vap, us_len, puc_param);
}


OAL_STATIC oal_uint32 wal_config_fbt_scan_channel(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_scan_channel(pst_mac_vap, us_len, puc_param);
}



OAL_STATIC oal_uint32 wal_config_fbt_scan_report_period(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    return hmac_config_fbt_scan_report_period(pst_mac_vap, us_len, puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN

OAL_STATIC oal_uint32  wal_config_set_tx_classify_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_set_tx_classify_switch(pst_mac_vap, us_len, puc_param);
}
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST

OAL_STATIC oal_uint32  wal_config_send_cw_signal(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_cw_signal(pst_mac_vap, us_len, puc_param);
}
#endif

OAL_CONST wal_wid_op_stru g_ast_board_wid_op_debug[] =
{
    {WLAN_CFGID_CHECK_FEM_PA,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_fem_pa_status},
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    {WLAN_CFGID_CHIP_CHECK_SWITCH, OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_chip_check},
    {WLAN_CFGID_SEND_CW_SIGNAL,             OAL_FALSE,  {0},  OAL_PTR_NULL,        wal_config_send_cw_signal},
#endif
    {WLAN_CFGID_EVENT_SWITCH,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_event_switch},
    {WLAN_CFGID_ETH_SWITCH,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_eth_switch},
    {WLAN_CFGID_80211_UCAST_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_80211_ucast_switch},
#ifdef _PRE_DEBUG_MODE_USER_TRACK
    {WLAN_CFGID_USR_THRPUT_STAT,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_report_thrput_stat},
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    {WLAN_CFGID_TXOP_PS_MACHW,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_txop_ps_machw},
#endif
    {WLAN_CFGID_80211_MCAST_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_80211_mcast_switch},
    {WLAN_CFGID_PROBE_SWITCH,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_probe_switch},
    {WLAN_CFGID_RSSI_SWITCH,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_rssi_switch},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_REPORT_VAP_INFO,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_report_vap_info},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_GET_MPDU_NUM,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_mpdu_num},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_WLAN_CHIP_TEST
    {WLAN_CFGID_SET_BEACON_OFFLOAD_TEST,   OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_beacon_offload_test},
#endif
    {WLAN_CFGID_OTA_BEACON_SWITCH,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ota_beacon_switch},
    {WLAN_CFGID_OTA_RX_DSCR_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ota_rx_dscr_switch},
    {WLAN_CFGID_SET_ALL_OTA,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_all_ota},
    {WLAN_CFGID_SET_DHCP_ARP,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_dhcp_arp_switch},
#ifdef _PRE_WLAN_RF_110X_CALI_DPD
    {WLAN_CFGID_START_DPD,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_dpd},
#endif
    {WLAN_CFGID_OAM_OUTPUT_TYPE,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_oam_output},
    {WLAN_CFGID_AMPDU_START,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ampdu_start},
    {WLAN_CFGID_AMSDU_START,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_amsdu_start},
    {WLAN_CFGID_AMPDU_END,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ampdu_end},
    {WLAN_CFGID_AUTO_BA_SWITCH,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_auto_ba_switch},
    {WLAN_CFGID_PROFILING_SWITCH,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_profiling_switch},
    {WLAN_CFGID_ADDBA_REQ,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_addba_req},
    {WLAN_CFGID_DELBA_REQ,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_delba_req},
#ifdef _PRE_WLAN_FEATURE_WMMAC
    {WLAN_CFGID_ADDTS_REQ,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_addts_req},
    {WLAN_CFGID_DELTS,                  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_delts},
    {WLAN_CFGID_REASSOC_REQ,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_reassoc_req},
    {WLAN_CFGID_WMMAC_SWITCH,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_wmmac_switch},
#endif
#ifdef _PRE_WLAN_FEATURE_11K
    {WLAN_CFGID_BCN_TABLE_SWITCH,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_bcn_table_switch},
#endif //_PRE_WLAN_FEATURE_11K
    {WLAN_CFGID_VOE_ENABLE,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_voe_enable},
    {WLAN_CFGID_SET_FEATURE_LOG,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_feature_log},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_SET_LOG_PM,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_log_lowpower},
#endif
    {WLAN_CFGID_LIST_AP,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_list_ap},
    {WLAN_CFGID_LIST_STA,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_list_sta},
    {WLAN_CFGID_START_SCAN,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_scan},
    {WLAN_CFGID_START_JOIN,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_join},
    {WLAN_CFGID_START_DEAUTH,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_start_deauth},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DUMP_TIEMR,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_timer},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_PAUSE_TID,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_pause_tid},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_USER_VIP,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_user_vip},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_VAP_HOST,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_vap_host},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_AMPDU_TX_ON,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_ampdu_tx_on},
    {WLAN_CFGID_AMSDU_TX_ON,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_amsdu_tx_on},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SEND_BAR,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_send_bar},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_PACKET_XMIT,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_packet_xmit},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DUMP_BA_BITMAP,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_ba_bitmap},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_VAP_PKT_STAT,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_vap_pkt_stat},
    {WLAN_CFGID_TIMER_START,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_timer_start},
    {WLAN_CFGID_SHOW_PROFILING,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_profiling},
    {WLAN_CFGID_AMSDU_AMPDU_SWITCH,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_amsdu_ampdu_switch},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_RESET_HW,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_reset_hw},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_RESET_HW_OPERATE,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_reset_operate},

#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGIG_DFR_ENABLE,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dfr_enable},
    {WLAN_CFGID_TRIG_PCIE_RESET,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_trig_pcie_reset},
    {WLAN_CFGID_TRIG_LOSS_TX_COMP,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_trig_loss_tx_comp},
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_UAPSD
    {WLAN_CFGID_UAPSD_DEBUG,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_uapsd_debug},
#endif
#ifdef _PRE_WLAN_DFT_STAT
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_PHY_STAT_EN,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_phy_stat_en},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DBB_ENV_PARAM,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dbb_env_param},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_USR_QUEUE_STAT,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_usr_queue_stat},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_VAP_STAT,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_report_vap_stat},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_ALL_STAT,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_report_all_stat},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
    {WLAN_CFGID_DUMP_RX_DSCR,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_rx_dscr},
    {WLAN_CFGID_DUMP_TX_DSCR,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_tx_dscr},
    {WLAN_CFGID_DUMP_MEMORY,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dump_memory},
    {WLAN_CFGID_ALG,                    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_alg},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_BEACON_CHAIN_SWITCH,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_beacon_chain_switch},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_2040_CHASWI_PROHI,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_2040_channel_switch_prohibited},
    {WLAN_CFGID_2040_INTOLERANT,        OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_FortyMHzIntolerant},
    {WLAN_CFGID_2040_COEXISTENCE,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_2040_coext_support},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_RESUME_RX_INTR_FIFO,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_resume_rx_intr_fifo},
#endif
#ifdef _PRE_SUPPORT_ACS
    {WLAN_CFGID_ACS_CONFIG,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_acs},
#endif
    {WLAN_CFGID_LIST_CHAN,              OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_list_channel},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_NSS,                OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_nss},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_RFCH,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_rfch},
#endif //#ifdef _PRE_DEBUG_MODE
#if defined (_PRE_WLAN_CHIP_TEST) || defined (_PRE_WLAN_FEATURE_ALWAYS_TX)
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_ALWAYS_TX,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_always_tx},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_RXCH,               OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_rxch},
    {WLAN_CFGID_DYNC_TXPOWER,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dync_txpower},
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_GET_THRUPUT,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_get_thruput},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_FREQ_SKEW,          OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_freq_skew},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_ADJUST_PPM,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_adjust_ppm},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DBB_SCALING_AMEND,      OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_dbb_scaling_amend},
    {WLAN_CFGID_TXBF_SWITCH,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_txbf_switch},
#ifdef _PRE_WLAN_PERFORM_STAT
    {WLAN_CFGID_PFM_STAT,          OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pfm_stat},
    {WLAN_CFGID_PFM_DISPLAY,       OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pfm_display},
#endif
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    {WLAN_CFGID_GET_HIPKT_STAT,  	  OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_get_hipkt_stat},
    {WLAN_CFGID_SET_FLOWCTL_PARAM,    OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_set_flowctl_param},
    {WLAN_CFGID_GET_FLOWCTL_STAT,     OAL_FALSE,  {0},   OAL_PTR_NULL,           wal_config_get_flowctl_stat},
#endif
#ifdef _PRE_WLAN_CHIP_TEST
    {WLAN_CFGID_LPM_TX_DATA,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_lpm_tx_data},
    {WLAN_CFGID_LPM_TX_PROBE_REQUEST,        OAL_FALSE,  {0},   OAL_PTR_NULL,   wal_config_lpm_tx_probe_request},
    {WLAN_CFGID_LPM_CHIP_STATE,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_chip_state},
    {WLAN_CFGID_LPM_SOC_MODE,      OAL_FALSE,  {0},   OAL_PTR_NULL,             wal_config_set_lpm_soc_mode},
    {WLAN_CFGID_LPM_PSM_PARAM,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_psm_param},
    {WLAN_CFGID_LPM_SMPS_MODE,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_smps_mode},
    {WLAN_CFGID_LPM_SMPS_STUB,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_smps_stub},
    {WLAN_CFGID_LPM_TXOP_PS_SET,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_txop_ps},
    {WLAN_CFGID_LPM_TXOP_TX_STUB,   OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_lpm_txop_tx_stub},
    {WLAN_CFGID_REMOVE_LUT,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_remove_user_lut},
    {WLAN_CFGID_SEND_FRAME,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_send_frame},
    {WLAN_CFGID_SET_RX_PN_REG,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_rx_pn},
    {WLAN_CFGID_SET_SOFT_RETRY,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_soft_retry},
    {WLAN_CFGID_OPEN_ADDR4,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_open_addr4},
    {WLAN_CFGID_OPEN_WMM_TEST,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_open_wmm_test},

    {WLAN_CFGID_CHIP_TEST_OPEN,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_chip_test_open},
    {WLAN_CFGID_SET_COEX,           OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_coex},
    {WLAN_CFGID_DFX_SWITCH,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_dfx},
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    {WLAN_CFGID_PMF_ENABLE,         OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_enable_pmf},
#endif
    {WLAN_CFGID_SEND_ACTION,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_send_action},
    {WLAN_CFGID_CLEAR_ALL_STAT,         OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_clear_all_stat},
    {WLAN_CFGID_SEND_PSPOLL,        OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_send_pspoll},
    {WLAN_CFGID_SEND_NULLDATA,      OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_send_nulldata},
#endif /* #ifdef _PRE_WLAN_CHIP_TEST */
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_HIDE_SSID,          OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_hide_ssid},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_MIB,            OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_mib},
    {WLAN_CFGID_GET_MIB,            OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_mib},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_THRUPUT_BYPASS, OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_thruput_bypass},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_AUTO_PROTECTION,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_auto_protection},
    {WLAN_CFGID_SEND_2040_COEXT,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_send_2040_coext},
    {WLAN_CFGID_2040_COEXT_INFO,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_2040_coext_info},
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_GET_ALL_REG_VALUE,  OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_all_reg_value},
#endif
#ifdef _PRE_WLAN_FEATURE_DAQ
    {WLAN_CFGID_DATA_ACQ,           OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_data_acq},
#endif
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    {WLAN_CFGID_SET_OMA,            OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_oma},
	{WLAN_CFGID_PROXYSTA_SWITCH,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_proxysta_switch},
#endif
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
    {WLAN_CFGID_SET_OPMODE_NOTIFY,  OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_opmode_notify},
    {WLAN_CFGID_GET_USER_RSSBW,     OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_user_rssbw},
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_VAP_NSS,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_vap_nss},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_WLAN_DFT_REG
    {WLAN_CFGID_DUMP_REG,       OAL_FALSE,  {0},   OAL_PTR_NULL,                wal_config_dump_reg},
#endif
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_RX_FILTER_VAL,      OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_rx_filter_val},
    {WLAN_CFGID_SET_RX_FILTER_EN,   OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_rx_filter_en},
    {WLAN_CFGID_GET_RX_FILTER_EN,   OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_rx_filter_en},
    {WLAN_CFGID_REPORT_AMPDU_STAT,  OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_report_ampdu_stat},
#endif
    {WLAN_CFGID_SET_AGGR_NUM,       OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_ampdu_aggr_num},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_FREQ_ADJUST,        OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_freq_adjust},
#endif

    {WLAN_CFGID_SET_STBC_CAP,       OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_stbc_cap},
    {WLAN_CFGID_SET_LDPC_CAP,       OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_ldpc_cap},

#ifdef _PRE_WLAN_FEATURE_CUSTOM_SECURITY
    /* 自动黑名单参数配置 */
    {WLAN_CFGID_AUTOBLACKLIST_ON        ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_autoblacklist_enable},
    {WLAN_CFGID_AUTOBLACKLIST_AGING     ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_autoblacklist_aging},
    {WLAN_CFGID_AUTOBLACKLIST_THRESHOLD ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_autoblacklist_threshold},
    {WLAN_CFGID_AUTOBLACKLIST_RESET     ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_autoblacklist_reset_time},

    /* 用户隔离参数配置 */
    {WLAN_CFGID_ISOLATION_MODE          ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_mode},
    {WLAN_CFGID_ISOLATION_TYPE          ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_type},
    {WLAN_CFGID_ISOLATION_FORWARD       ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_forward},
    {WLAN_CFGID_ISOLATION_CLEAR         ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_isolation_clear},
    {WLAN_CFGID_ISOLATION_SHOW          ,OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_isolation_show},
#endif

#ifdef _PRE_WLAN_FEATURE_MCAST
    {WLAN_CFGID_M2U_SNOOP_ON,      OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_m2u_snoop_on},
    {WLAN_ADD_M2U_DENY_TABLE,      OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_add_m2u_deny_table},
    {WLAN_CFGID_M2U_DENY_TABLE,    OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_m2u_deny_table},
    {WLAN_SHOW_M2U_SNOOP_TABLE,    OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_show_m2u_snoop_table},
    {WLAN_CFGID_IGMP_PACKET_XMIT,  OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_igmp_packet_xmit},
#endif
#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_PROXYARP_INFO           ,OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_proxyarp_info},
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif

#ifdef _PRE_WLAN_FEATURE_WAPI
    //{WLAN_CFGID_WAPI_EN             ,OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_wapi_en},
#ifdef _PRE_WAPI_DEBUG
    {WLAN_CFGID_WAPI_INFO           ,OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_wapi_info},
#endif /* #ifdef _PRE_DEBUG_MODE */
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_GET_SMPS_INFO,           OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_get_smps_info},
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_PM
    {WLAN_CFGID_WIFI_EN,            OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_wifi_enable},
    {WLAN_CFGID_PM_INFO,            OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pm_info},
    {WLAN_CFGID_PM_EN,              OAL_FALSE,  {0},   OAL_PTR_NULL,              wal_config_pm_enable},
#endif

    {WLAN_CFGID_VAP_CLASSIFY_EN,    OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_vap_classify_en},
    {WLAN_CFGID_VAP_CLASSIFY_TID,   OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_vap_classify_tid},

#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SCAN_TEST,          OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_scan_test},
#endif
    {WLAN_CFIGD_BGSCAN_ENABLE,      OAL_FALSE,  {0},    OAL_PTR_NULL,           wal_config_bgscan_enable},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DEVICE_MEM_LEAK,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_device_memleak},
#endif //#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_DEVICE_MEM_INFO,       OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_device_meminfo},
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM
    {WLAN_CFGID_SET_PSM_PARAM,         OAL_FALSE,  {0},   OAL_PTR_NULL,          wal_config_set_pm_param},
#endif
#ifdef _PRE_WLAN_FEATURE_11D
    {WLAN_CFGID_SET_RD_IE_SWITCH,      OAL_FALSE, {0},      OAL_PTR_NULL,       wal_config_set_rd_by_ie_switch},
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_P2P_PS_STAT,    OAL_FALSE,  {0},   OAL_PTR_NULL,            wal_config_set_p2p_ps_stat},
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS
    {WLAN_CFGID_SET_MIPS,            OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_set_mips},
    {WLAN_CFGID_SHOW_MIPS,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_mips},
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_AMPDU_MMSS,          OAL_FALSE,  {0},    OAL_PTR_NULL,             wal_config_set_ampdu_mmss},
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_ENABLE_ARP_OFFLOAD,             OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_enable_arp_offload},
#endif //#ifdef _PRE_DEBUG_MODE
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SHOW_ARPOFFLOAD_INFO,           OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_show_arpoffload_info},
#endif //#ifdef _PRE_DEBUG_MODE
#endif

#ifdef _PRE_WLAN_TCP_OPT
    {WLAN_CFGID_GET_TCP_ACK_STREAM_INFO,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_get_tcp_ack_stream_info},
    {WLAN_CFGID_TX_TCP_ACK_OPT_ENALBE,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_tx_tcp_ack_opt_enable},
    {WLAN_CFGID_RX_TCP_ACK_OPT_ENALBE,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_rx_tcp_ack_opt_enable},
    {WLAN_CFGID_TX_TCP_ACK_OPT_LIMIT,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_tx_tcp_ack_limit},
    {WLAN_CFGID_RX_TCP_ACK_OPT_LIMIT,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_rx_tcp_ack_limit},
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44)) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_DFT_STAT
#ifdef _PRE_DEBUG_MODE
    {WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_set_performance_log_switch},
#endif //#ifdef _PRE_DEBUG_MODE
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    {WLAN_CFGID_ROAM_ENABLE,  OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_enable},
    {WLAN_CFGID_ROAM_ORG,     OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_org},
    {WLAN_CFGID_ROAM_BAND,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_band},
    {WLAN_CFGID_ROAM_START,   OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_start},
    {WLAN_CFGID_ROAM_INFO,    OAL_FALSE,  {0},    OAL_PTR_NULL,            wal_config_roam_info},
#endif  //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    {WLAN_CFGID_2040BSS_ENABLE,  OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_enable_2040bss},
#endif
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
    {WLAN_CFGID_SET_AUTO_FREQ_ENABLE,         OAL_FALSE,  {0},  OAL_PTR_NULL,          wal_config_set_auto_freq_enable},
#endif
#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
    {WLAN_CFGID_TX_CLASSIFY_LAN_TO_WLAN_SWITCH,     OAL_FALSE,  {0},   OAL_PTR_NULL,        wal_config_set_tx_classify_switch},
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    {WLAN_CFGID_SET_TXRX_CHAIN,  OAL_FALSE,  {0},    OAL_PTR_NULL,          wal_config_set_txrx_chain},
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
    {WLAN_CFGID_FBT_SET_MODE,                   OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_set_mode},
    {WLAN_CFGID_FBT_SCAN_LIST_CLEAR,            OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_scan_list_clear},
    {WLAN_CFGID_FBT_SCAN_SPECIFIED_STA,         OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_scan_specified_sta},
    {WLAN_CFGID_FBT_PRINT_SCAN_LIST,            OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_print_scan_list},
    {WLAN_CFGID_FBT_SCAN_ENABLE,                OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_scan_enable},
    {WLAN_CFGID_FBT_SCAN_INTERVAL,              OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_scan_interval},
    {WLAN_CFGID_FBT_SCAN_CHANNEL,               OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_scan_channel},
    {WLAN_CFGID_FBT_SCAN_REPORT_PERIOD,         OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_fbt_scan_report_period},
#endif
    {WLAN_CFIGD_MCS_SET_CHECK_ENABLE,           OAL_FALSE,  {0},    OAL_PTR_NULL,         wal_config_mcs_set_check_enable},

    {WLAN_CFGID_BUTT,                  OAL_FALSE,  {0},    0,                       0},
};

oal_uint32 wal_config_get_debug_wid_arrysize(oal_void)
{
    return OAL_ARRAY_SIZE(g_ast_board_wid_op_debug);
}
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

