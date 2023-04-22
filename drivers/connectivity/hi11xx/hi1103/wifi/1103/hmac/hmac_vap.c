


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_net.h"
#include "oal_ext_if.h"
#include "oam_ext_if.h"

#include "wlan_spec.h"

#include "mac_ie.h"

#include "frw_event_main.h"

#include "hmac_vap.h"
#include "hmac_resource.h"
#include "hmac_tx_amsdu.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_fsm.h"
#include "hmac_ext_if.h"
#include "hmac_chan_mgmt.h"

#include "hmac_edca_opt.h"
#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif
#include "hmac_blockack.h"
#include "hmac_p2p.h"
#ifdef _PRE_WLAN_TCP_OPT
#include "hmac_tcp_opt.h"
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
#include "hmac_fbt_main.h"
#endif
#include "hmac_mgmt_sta.h"
#include "hmac_mgmt_ap.h"
#ifdef _PRE_WLAN_FEATURE_ISOLATION
#include "hmac_isolation.h"
#endif
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
#include "hmac_blacklist.h"
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
#include "hmac_wds.h"
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
#include "hmac_11k.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_VAP_C
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
hmac_vap_stru g_ast_hmac_vap[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];
#endif

#if defined (_PRE_WLAN_FEATURE_RX_AGGR_EXTEND) || defined (_PRE_FEATURE_WAVEAPP_CLASSIFY)
/* 是否仪器识别来控制cpu0绑核 */
oal_uint8 g_en_wave_bind_cpu0_ctrl = OAL_FALSE;
#endif

oal_uint8  g_uc_host_rx_ampdu_amsdu = OAL_FALSE;

#define HMAC_NETDEVICE_WDT_TIMEOUT      (5*OAL_TIME_HZ)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
OAL_STATIC oal_int32  hmac_cfg_vap_if_open(oal_net_device_stru *pst_dev);
OAL_STATIC oal_int32  hmac_cfg_vap_if_close(oal_net_device_stru *pst_dev);
OAL_STATIC oal_net_dev_tx_enum  hmac_cfg_vap_if_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev);
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    OAL_STATIC oal_net_device_ops_stru gst_vap_net_dev_cfg_vap_ops = {
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
        .ndo_open               = hmac_cfg_vap_if_open,
        .ndo_stop               = hmac_cfg_vap_if_close,
        .ndo_start_xmit         = hmac_cfg_vap_if_xmit,
#else
        .ndo_get_stats          = oal_net_device_get_stats,
        .ndo_open               = oal_net_device_open,
        .ndo_stop               = oal_net_device_close,
        .ndo_start_xmit         = OAL_PTR_NULL,
        .ndo_set_multicast_list = OAL_PTR_NULL,
        .ndo_do_ioctl           = oal_net_device_ioctl,
        .ndo_change_mtu         = oal_net_device_change_mtu,
        .ndo_init               = oal_net_device_init,
        .ndo_set_mac_address    = oal_net_device_set_macaddr
#endif
    };
#elif (_PRE_OS_VERSION_WIN32 == _PRE_OS_VERSION)
    OAL_STATIC oal_net_device_ops_stru gst_vap_net_dev_cfg_vap_ops = {
         oal_net_device_init,
         oal_net_device_open,
         oal_net_device_close,
         OAL_PTR_NULL,
         OAL_PTR_NULL,
         oal_net_device_get_stats,
         oal_net_device_ioctl,
         oal_net_device_change_mtu,
         oal_net_device_set_macaddr};
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
extern oal_void hmac_del_virtual_inf_worker_etc(oal_work_stru *pst_del_virtual_inf_work);
#endif

/*****************************************************************************
  3 函数实现
*****************************************************************************/


extern oal_uint8 g_wlan_ps_mode;
oal_uint32  hmac_vap_init_etc(
                hmac_vap_stru              *pst_hmac_vap,
                oal_uint8                   uc_chip_id,
                oal_uint8                   uc_device_id,
                oal_uint8                   uc_vap_id,
                mac_cfg_add_vap_param_stru *pst_param)
{
    oal_uint32      ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_vap_init_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_vap_init_etc(&pst_hmac_vap->st_vap_base_info,
                          uc_chip_id,
                          uc_device_id,
                          uc_vap_id,
                          pst_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(uc_vap_id, OAM_SF_ANY, "{hmac_vap_init_etc::mac_vap_init_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /* 统计信息清零 */
    oam_stats_clear_vap_stat_info_etc(uc_vap_id);
#endif

    /* 初始化预设参数 */
    pst_hmac_vap->st_preset_para.en_protocol   = pst_hmac_vap->st_vap_base_info.en_protocol;
    pst_hmac_vap->st_preset_para.en_bandwidth  = pst_hmac_vap->st_vap_base_info.st_channel.en_bandwidth;
    pst_hmac_vap->st_preset_para.en_band       = pst_hmac_vap->st_vap_base_info.st_channel.en_band;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    /* ProxySTA起来后一段时间ap信号消失,原因是AP踢出用户后，用户重新关联后采用en_band设置tcp值 */
    if(1 == pst_hmac_vap->st_vap_base_info.uc_chip_id)  /* 目前835产品2G芯片都是在芯片1上 后续如果其他产品有变动，需要增加修改 TODO */
    {
       pst_hmac_vap->st_preset_para.en_band = WLAN_BAND_2G;
    }
    else
    {
       pst_hmac_vap->st_preset_para.en_band = WLAN_BAND_5G;
    }
#endif
    /* 初始化配置私有结构体 */
    //对于P2P CL 不能再初始化队列
    pst_hmac_vap->st_cfg_priv.dog_tag = OAL_DOG_TAG;
    OAL_WAIT_QUEUE_INIT_HEAD(&(pst_hmac_vap->st_cfg_priv.st_wait_queue_for_sdt_reg));
    OAL_WAIT_QUEUE_INIT_HEAD(&(pst_hmac_vap->st_mgmt_tx.st_wait_queue));

#ifdef _PRE_WLAN_FEATURE_11D
    pst_hmac_vap->en_updata_rd_by_ie_switch = OAL_FALSE;
#endif

#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
    hmac_wds_init_table(pst_hmac_vap);
#endif
#ifdef _PRE_WLAN_FEATURE_11R_AP
    pst_hmac_vap->pst_mlme = OAL_PTR_NULL;
#endif

    //1103默认支持接收方向的AMPDU+AMSDU联合聚合,通过定制化决定
    pst_hmac_vap->bit_rx_ampduplusamsdu_active = g_uc_host_rx_ampdu_amsdu;

    OAL_WAIT_QUEUE_INIT_HEAD(&pst_hmac_vap->query_wait_q);
    /* 根据配置VAP，将对应函数挂接在业务VAP，区分AP、STA和WDS模式 */
    switch(pst_param->en_vap_mode)
    {
        case WLAN_VAP_MODE_BSS_AP:

         /* 组播转单播初始化函数 */
        #if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
            hmac_m2u_attach(pst_hmac_vap);
        #endif

        #ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
            pst_hmac_vap->ul_edca_opt_time_ms       = HMAC_EDCA_OPT_TIME_MS;
            FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->st_edca_opt_timer), hmac_edca_opt_timeout_fn_etc, pst_hmac_vap->ul_edca_opt_time_ms, pst_hmac_vap, OAL_TRUE, OAM_MODULE_ID_HMAC, pst_hmac_vap->st_vap_base_info.ul_core_id);
            /* also open for 1102 at 2015-10-16 */
            pst_hmac_vap->uc_edca_opt_flag_ap       = 1;
            FRW_TIMER_RESTART_TIMER(&(pst_hmac_vap->st_edca_opt_timer), pst_hmac_vap->ul_edca_opt_time_ms, OAL_TRUE);
            pst_hmac_vap->uc_idle_cycle_num         = 0;
        #endif

        #ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
            /* AP模式默认业务识别功能开启 */
            mac_mib_set_TxTrafficClassifyFlag(&pst_hmac_vap->st_vap_base_info, OAL_SWITCH_ON);
        #endif

 		#ifdef _PRE_WLAN_FEATURE_HILINK
            hmac_fbt_init(pst_hmac_vap);    /* 快速切换特性初始化 */
		#endif
        #if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
            mac_blacklist_get_pointer(pst_param->en_vap_mode, uc_chip_id, uc_device_id, uc_vap_id, &(pst_hmac_vap->pst_blacklist_info));
            if(OAL_PTR_NULL != pst_hmac_vap->pst_blacklist_info)
            {
                OAL_MEMZERO(pst_hmac_vap->pst_blacklist_info->ast_black_list, sizeof(mac_blacklist_stru) * WLAN_BLACKLIST_MAX);
                OAL_MEMZERO(&(pst_hmac_vap->pst_blacklist_info->st_autoblacklist_info), sizeof(mac_autoblacklist_info_stru));
                pst_hmac_vap->pst_blacklist_info->uc_mode = 0;
                pst_hmac_vap->pst_blacklist_info->uc_list_num = 0;
            }
        #endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        pst_hmac_vap->bit_rx_ampduplusamsdu_active = OAL_TRUE;  //51的AP模式的vap默认支持接收方向的AMPDU+AMSDU联合聚合

        #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
            mac_mib_set_AmsduPlusAmpduActive(&pst_hmac_vap->st_vap_base_info,(oal_uint8)!!hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_AP_TX_AMSDU));
            pst_hmac_vap->bit_rx_ampduplusamsdu_active = (oal_uint8)!!hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_AP_RX_AMSDU);
        #endif
#endif
            break;

        case WLAN_VAP_MODE_BSS_STA:
             /* 组播转单播初始化函数 */
        #if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
            hmac_m2u_attach(pst_hmac_vap);
        #endif
        #if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
            mac_blacklist_get_pointer(pst_param->en_vap_mode, uc_chip_id, uc_device_id, uc_vap_id, &(pst_hmac_vap->pst_blacklist_info));
            if(OAL_PTR_NULL != pst_hmac_vap->pst_blacklist_info)
            {
                OAL_MEMZERO(pst_hmac_vap->pst_blacklist_info->ast_black_list, sizeof(mac_blacklist_stru) * WLAN_BLACKLIST_MAX);
                OAL_MEMZERO(&(pst_hmac_vap->pst_blacklist_info->st_autoblacklist_info), sizeof(mac_autoblacklist_info_stru));
                pst_hmac_vap->pst_blacklist_info->uc_mode = 0;
                pst_hmac_vap->pst_blacklist_info->uc_list_num = 0;
            }
        #endif

        #ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
            pst_hmac_vap->uc_edca_opt_flag_sta = 0;
            pst_hmac_vap->uc_edca_opt_weight_sta    = WLAN_EDCA_OPT_WEIGHT_STA;
        #endif

        #ifdef _PRE_WLAN_FEATURE_DFS
            mac_mib_set_SpectrumManagementImplemented(&pst_hmac_vap->st_vap_base_info, OAL_TRUE);
        #endif
            pst_hmac_vap->bit_sta_protocol_cfg = OAL_SWITCH_OFF;
        #ifdef _PRE_WLAN_FEATURE_STA_PM
            pst_hmac_vap->uc_cfg_sta_pm_manual = 0xFF;
            pst_hmac_vap->uc_ps_mode           = g_wlan_ps_mode;
        #endif

        #ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
            /* STA模式默认业务识别功能开启 */
            mac_mib_set_TxTrafficClassifyFlag(&pst_hmac_vap->st_vap_base_info, OAL_SWITCH_ON);
        #endif
            break;

        case WLAN_VAP_MODE_WDS:
            break;

        case WLAN_VAP_MODE_CONFIG:
            /* 配置VAP直接返回 */
            return OAL_SUCC;

        default:
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_vap_init_etc::unsupported mod=%d.}", pst_param->en_vap_mode);
            return OAL_ERR_CODE_INVALID_CONFIG;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    oal_netbuf_head_init(&(pst_hmac_vap->st_tx_queue_head[0]));
    oal_netbuf_head_init(&(pst_hmac_vap->st_tx_queue_head[1]));
    pst_hmac_vap->uc_in_queue_id  = 0;
    pst_hmac_vap->uc_out_queue_id = 1;
    oal_atomic_set(&pst_hmac_vap->ul_tx_event_num,1);  /* ul_tx_event_num初始值修改为1，防止hmac_tx_post_event可能连续抛两个以上事件 */
    pst_hmac_vap->ul_tx_quata = 256;
    oal_spin_lock_init(&pst_hmac_vap->st_smp_lock);
#endif

    oal_spin_lock_init(&pst_hmac_vap->st_lock_state);
    oal_dlist_init_head(&(pst_hmac_vap->st_pmksa_list_head));

    /* 创建vap时 初始状态为init */
	mac_vap_state_change_etc(&(pst_hmac_vap->st_vap_base_info), MAC_VAP_STATE_INIT);

    /* 初始化重排序超时时间 */
    pst_hmac_vap->us_rx_timeout[WLAN_WME_AC_BK] = HMAC_BA_RX_BK_TIMEOUT;
    pst_hmac_vap->us_rx_timeout[WLAN_WME_AC_BE] = HMAC_BA_RX_BE_TIMEOUT;
    pst_hmac_vap->us_rx_timeout[WLAN_WME_AC_VI] = HMAC_BA_RX_VI_TIMEOUT;
    pst_hmac_vap->us_rx_timeout[WLAN_WME_AC_VO] = HMAC_BA_RX_VO_TIMEOUT;

    pst_hmac_vap->us_rx_timeout_min[WLAN_WME_AC_BE] = HMAC_BA_RX_BE_TIMEOUT_MIN;
    pst_hmac_vap->us_rx_timeout_min[WLAN_WME_AC_BK] = HMAC_BA_RX_BK_TIMEOUT_MIN;
    pst_hmac_vap->us_rx_timeout_min[WLAN_WME_AC_VI] = HMAC_BA_RX_VI_TIMEOUT_MIN;
    pst_hmac_vap->us_rx_timeout_min[WLAN_WME_AC_VO] = HMAC_BA_RX_VO_TIMEOUT_MIN;

#ifdef _PRE_WLAN_FEATURE_P2P
    /* 初始化删除虚拟网络接口工作队列 */
    OAL_INIT_WORK(&(pst_hmac_vap->st_del_virtual_inf_worker), hmac_del_virtual_inf_worker_etc);
    pst_hmac_vap->pst_del_net_device    = OAL_PTR_NULL;
    pst_hmac_vap->pst_p2p0_net_device   = OAL_PTR_NULL;
#endif
#ifdef _PRE_WLAN_TCP_OPT
    pst_hmac_vap->st_hamc_tcp_ack[HCC_TX].filter_info.ul_ack_limit = DEFAULT_TX_TCP_ACK_THRESHOLD;
    pst_hmac_vap->st_hamc_tcp_ack[HCC_RX].filter_info.ul_ack_limit = DEFAULT_RX_TCP_ACK_THRESHOLD;
    hmac_tcp_opt_init_filter_tcp_ack_pool_etc(pst_hmac_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND
    pst_hmac_vap->en_restrict_band = WLAN_BAND_BUTT;    // 默认不限制
#endif
#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE)
    pst_hmac_vap->en_omit_acs_chan = OAL_FALSE;         // 默认不忽略
#endif

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN) || defined(_PRE_WLAN_FEATURE_FTM)
#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || defined(_PRE_PRODUCT_ID_HI110X_DEV))
    if(IS_LEGACY_STA(&pst_hmac_vap->st_vap_base_info))
#endif
    {
        pst_hmac_vap->bit_11k_enable   = g_st_mac_voe_custom_param.en_11k;
        pst_hmac_vap->bit_11v_enable   = g_st_mac_voe_custom_param.en_11v;
        pst_hmac_vap->bit_bcn_table_switch = g_st_mac_voe_custom_param.en_11k;
    }

    mac_mib_set_dot11RadioMeasurementActivated(&(pst_hmac_vap->st_vap_base_info), pst_hmac_vap->bit_11k_enable);
    mac_mib_set_dot11RMBeaconTableMeasurementActivated(&(pst_hmac_vap->st_vap_base_info), pst_hmac_vap->bit_bcn_table_switch);
#endif

#if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)
#ifdef _PRE_WLAN_FEATURE_11V_ENABLE
    mac_mib_set_MgmtOptionBSSTransitionActivated(&(pst_hmac_vap->st_vap_base_info), pst_hmac_vap->bit_11v_enable);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11R
#if (defined(_PRE_PRODUCT_ID_HI110X_HOST) || defined(_PRE_PRODUCT_ID_HI110X_DEV))
    if(IS_LEGACY_STA(&pst_hmac_vap->st_vap_base_info))
#endif
    {
        pst_hmac_vap->bit_11r_enable   = g_st_mac_voe_custom_param.en_11r;
    }
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    hmac_11k_init_vap(pst_hmac_vap);
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_vap_get_priv_cfg_etc(mac_vap_stru *pst_mac_vap, hmac_vap_cfg_priv_stru **ppst_cfg_priv)
{
    hmac_vap_stru   *pst_hmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_vap_get_priv_cfg_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_vap_get_priv_cfg_etc::mac_res_get_hmac_vap fail.vap_id = %u}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }


    *ppst_cfg_priv = &pst_hmac_vap->st_cfg_priv;

    return OAL_SUCC;
}


oal_int8*  hmac_vap_get_desired_country_etc(oal_uint8 uc_vap_id)
{
    hmac_vap_stru   *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ANY, "{hmac_vap_get_desired_country_etc::pst_hmac_vap null.}");
        return OAL_PTR_NULL;
    }

    return pst_hmac_vap->ac_desired_country;
}
#ifdef _PRE_WLAN_FEATURE_11D

oal_uint32  hmac_vap_get_updata_rd_by_ie_switch_etc(oal_uint8 uc_vap_id,oal_bool_enum_uint8 *us_updata_rd_by_ie_switch)
{
    hmac_vap_stru   *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(0);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_vap_get_updata_rd_by_ie_switch_etc::pst_hmac_vap null.}");
        return OAL_PTR_NULL;
    }

    *us_updata_rd_by_ie_switch = pst_hmac_vap->en_updata_rd_by_ie_switch;
    return OAL_SUCC;
}
#endif

oal_net_device_stru  *hmac_vap_get_net_device_etc(oal_uint8 uc_vap_id)
{
    hmac_vap_stru   *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ANY, "{hmac_vap_get_net_device_etc::pst_hmac_vap null.}");
        return OAL_PTR_NULL;
    }

    return (pst_hmac_vap->pst_net_device);
}



oal_uint32  hmac_vap_creat_netdev_etc(hmac_vap_stru *pst_hmac_vap, oal_int8 *puc_netdev_name, oal_int8 *puc_mac_addr)

{
    oal_net_device_stru *pst_net_device;
    oal_uint32           ul_return;
    mac_vap_stru        *pst_vap;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == puc_netdev_name)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ASSOC, "{hmac_vap_creat_netdev_etc::param null %d %d.}", pst_hmac_vap, puc_netdev_name);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_vap = &pst_hmac_vap->st_vap_base_info;

    pst_net_device = oal_net_alloc_netdev(0, puc_netdev_name, oal_ether_setup);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_device))
    {
        OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_vap_creat_netdev_etc::pst_net_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如下对netdevice的赋值暂时按如下操作 */
    OAL_NETDEVICE_OPS(pst_net_device)             = &gst_vap_net_dev_cfg_vap_ops;
    OAL_NETDEVICE_DESTRUCTOR(pst_net_device)      = oal_net_free_netdev;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
        /* TBD  need to check the net device diff*/
#else
    OAL_NETDEVICE_MASTER(pst_net_device)          = OAL_PTR_NULL;
#endif

    OAL_NETDEVICE_IFALIAS(pst_net_device)         = OAL_PTR_NULL;
    OAL_NETDEVICE_WATCHDOG_TIMEO(pst_net_device)  = HMAC_NETDEVICE_WDT_TIMEOUT;
    oal_memcopy(OAL_NETDEVICE_MAC_ADDR(pst_net_device), puc_mac_addr, WLAN_MAC_ADDR_LEN);
    OAL_NET_DEV_PRIV(pst_net_device) = pst_vap;
    OAL_NETDEVICE_QDISC(pst_net_device, OAL_PTR_NULL);

    ul_return = (oal_uint32)oal_net_register_netdev(pst_net_device);
    if (OAL_UNLIKELY(OAL_SUCC != ul_return))
    {
        MAC_WARNING_LOG(pst_vap->uc_vap_id, "mac_device_init_etc:oal_net_register_netdev return fail.");
        OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_vap_creat_netdev_etc::oal_net_register_netdev failed.}");

        return ul_return;
    }

    pst_hmac_vap->pst_net_device = pst_net_device;
    /* 包括'\0' */
    oal_memcopy(pst_hmac_vap->auc_name,pst_net_device->name,OAL_IF_NAME_SIZE);

#ifdef _PRE_WLAN_REPORT_PRODUCT_LOG
    //保存vap id 对应的 chip id
    g_auc_vapid_to_chipid[pst_vap->uc_vap_id] = pst_vap->uc_chip_id;
#endif

    return OAL_SUCC;
}



oal_uint32  hmac_vap_destroy_etc(hmac_vap_stru *pst_hmac_vap)
{
    mac_cfg_down_vap_param_stru   st_down_vap;
    mac_cfg_del_vap_param_stru    st_del_vap_param;
    oal_uint32                    ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_vap_destroy_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
    if (WLAN_VAP_MODE_BSS_AP == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        pst_hmac_vap->uc_edca_opt_flag_ap   = 0;
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_edca_opt_timer));
    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        pst_hmac_vap->uc_edca_opt_flag_sta = 0;
    }
#endif

    /* 先down vap */
#ifdef _PRE_WLAN_FEATURE_P2P
    oal_cancel_work_sync(&(pst_hmac_vap->st_del_virtual_inf_worker));
    st_down_vap.en_p2p_mode = pst_hmac_vap->st_vap_base_info.en_p2p_mode;
#endif
    st_down_vap.pst_net_dev = pst_hmac_vap->pst_net_device;
    ul_ret = hmac_config_down_vap_etc(&pst_hmac_vap->st_vap_base_info,
                                  OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                                  (oal_uint8 *)&st_down_vap);
    if (ul_ret != OAL_SUCC)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_vap_destroy_etc::hmac_config_down_vap_etc failed[%d].}", ul_ret);
        return ul_ret;
    }
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL == _PRE_WLAN_FEATURE_BLACKLIST_VAP)
    mac_blacklist_free_pointer(&pst_hmac_vap->st_vap_base_info, pst_hmac_vap->pst_blacklist_info);
#endif
    /* 然后再delete vap */
    st_del_vap_param.en_p2p_mode = pst_hmac_vap->st_vap_base_info.en_p2p_mode;
    st_del_vap_param.en_vap_mode = pst_hmac_vap->st_vap_base_info.en_vap_mode;
    ul_ret = hmac_config_del_vap_etc(&pst_hmac_vap->st_vap_base_info,
                        OAL_SIZEOF(mac_cfg_del_vap_param_stru),
                        (oal_uint8 *)&st_del_vap_param);
    if (ul_ret != OAL_SUCC)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_vap_destroy_etc::hmac_config_del_vap_etc failed[%d].}", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint16 hmac_vap_check_ht_capabilities_ap_etc(
                hmac_vap_stru                   *pst_hmac_vap,
                oal_uint8                       *puc_payload,
                oal_uint32                       ul_msg_len,
                hmac_user_stru                   *pst_hmac_user_sta)
{
    oal_bool_enum           en_prev_asoc_ht = OAL_FALSE;
    oal_bool_enum           en_prev_asoc_non_ht = OAL_FALSE;
    mac_user_ht_hdl_stru   *pst_ht_hdl      = &(pst_hmac_user_sta->st_user_base_info.st_ht_hdl);
    hmac_amsdu_stru        *pst_amsdu;
    oal_uint32              ul_amsdu_idx;
    mac_protection_stru    *pst_protection;
    oal_uint8              *puc_ie          = OAL_PTR_NULL;

    if (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(&pst_hmac_vap->st_vap_base_info))
    {
        return MAC_SUCCESSFUL_STATUSCODE;
    }
    /* 检查STA是否是作为一个HT capable STA与AP关联 */
    if ((MAC_USER_STATE_ASSOC == pst_hmac_user_sta->st_user_base_info.en_user_asoc_state) && (OAL_TRUE == pst_ht_hdl->en_ht_capable))
    {
        mac_user_set_ht_capable_etc(&(pst_hmac_user_sta->st_user_base_info), OAL_FALSE);
        en_prev_asoc_ht = OAL_TRUE;
    }

    /* 检查STA是否是作为一个non HT capable STA与AP关联 */
    else if (MAC_USER_STATE_ASSOC == pst_hmac_user_sta->st_user_base_info.en_user_asoc_state)
    {
        en_prev_asoc_non_ht = OAL_TRUE;
    }
    else
    {

    }

    puc_ie = mac_find_ie_etc(MAC_EID_HT_CAP, puc_payload, (oal_int32)ul_msg_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        /* 不允许HT STA 使用 TKIP/WEP 加密 */
        if (mac_is_wep_enabled(&pst_hmac_vap->st_vap_base_info))
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                             "{hmac_vap_check_ht_capabilities_ap_etc::Rejecting a HT STA because of its Pairwise Cipher[%d].}",
                             pst_hmac_user_sta->st_user_base_info.st_key_info.en_cipher_type);
            return MAC_MISMATCH_HTCAP;
        }

        /* 搜索 HT Capabilities Element */
        hmac_search_ht_cap_ie_ap_etc(pst_hmac_vap, pst_hmac_user_sta, puc_ie, 0, en_prev_asoc_ht);

        if ((pst_ht_hdl->uc_rx_mcs_bitmask[3] == 0) && (pst_ht_hdl->uc_rx_mcs_bitmask[2] == 0)
            &&(pst_ht_hdl->uc_rx_mcs_bitmask[1] == 0) && (pst_ht_hdl->uc_rx_mcs_bitmask[0]) == 0)
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_vap_check_ht_capabilities_ap_etc::STA support ht capability but support none space_stream.}");
            /* 对端ht能力置为不支持 */
            mac_user_set_ht_capable_etc(&(pst_hmac_user_sta->st_user_base_info), OAL_FALSE);
        }
    }

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    puc_ie = mac_find_ie_etc(MAC_EID_EXT_CAPS, puc_payload, (oal_int32)ul_msg_len);
    if (OAL_PTR_NULL != puc_ie)
    {
        mac_ie_proc_ext_cap_ie_etc(&(pst_hmac_user_sta->st_user_base_info), puc_ie);
    }
#endif  /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */

    /*走到这里，说明sta已经被统计到ht相关的统计量中*/
    pst_hmac_user_sta->st_user_stats_flag.bit_no_ht_stats_flag = OAL_TRUE;
    pst_hmac_user_sta->st_user_stats_flag.bit_no_gf_stats_flag = OAL_TRUE;
    pst_hmac_user_sta->st_user_stats_flag.bit_20M_only_stats_flag = OAL_TRUE;
    pst_hmac_user_sta->st_user_stats_flag.bit_no_lsig_txop_stats_flag = OAL_TRUE;
    pst_hmac_user_sta->st_user_stats_flag.bit_no_40dsss_stats_flag = OAL_TRUE;

    pst_protection = &(pst_hmac_vap->st_vap_base_info.st_protection);
    if (OAL_FALSE == pst_ht_hdl->en_ht_capable) /*STA不支持HT*/
    {
        /*  如果STA之前没有与AP关联*/
        if (MAC_USER_STATE_ASSOC != pst_hmac_user_sta->st_user_base_info.en_user_asoc_state)
        {
            pst_protection->uc_sta_non_ht_num++;
        }
        /*如果STA之前已经作为HT站点与AP关联*/
        else if (OAL_TRUE == en_prev_asoc_ht)
        {
            pst_protection->uc_sta_non_ht_num++;

            if ((OAL_FALSE == pst_ht_hdl->bit_supported_channel_width) && (0 != pst_protection->uc_sta_20M_only_num))
            {
                pst_protection->uc_sta_20M_only_num--;
            }

            if ((OAL_FALSE == pst_ht_hdl->bit_ht_green_field) && (0 != pst_protection->uc_sta_non_gf_num))
            {
                pst_protection->uc_sta_non_gf_num--;
            }

            if ((OAL_FALSE == pst_ht_hdl->bit_lsig_txop_protection) && (0 != pst_protection->uc_sta_no_lsig_txop_num))
            {
                pst_protection->uc_sta_no_lsig_txop_num--;
            }
        }
        else /*STA 之前已经作为非HT站点与AP关联*/
        {
        }
    }
    else  /*STA支持HT*/
    {
        for (ul_amsdu_idx = 0; ul_amsdu_idx < WLAN_TID_MAX_NUM; ul_amsdu_idx++)
        {
            pst_amsdu = &(pst_hmac_user_sta->ast_hmac_amsdu[ul_amsdu_idx]);
            hmac_amsdu_set_maxsize(pst_amsdu, pst_hmac_user_sta, 7936);
            hmac_amsdu_set_maxnum(pst_amsdu, WLAN_AMSDU_MAX_NUM);
            oal_spin_lock_init(&pst_amsdu->st_amsdu_lock);
            pst_amsdu->uc_msdu_num      = 0;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
            pst_amsdu->uc_short_pkt_num = 0x00;
#endif
        }

        /* 设置amsdu域 */
        pst_hmac_user_sta->uc_amsdu_supported = 255;

        /*  如果STA之前已经以non-HT站点与AP关联, 则将uc_sta_non_ht_num减1*/
        if ((OAL_TRUE == en_prev_asoc_non_ht) && (0 != pst_protection->uc_sta_non_ht_num))
        {
            pst_protection->uc_sta_non_ht_num--;
        }
    }

    return MAC_SUCCESSFUL_STATUSCODE;
}


oal_void  hmac_search_txbf_cap_ie_ap_etc(mac_user_ht_hdl_stru *pst_ht_hdl,
                                     oal_uint32            ul_info_elem)
{
    oal_uint32  ul_tmp_txbf_elem = ul_info_elem;

    pst_ht_hdl->bit_imbf_receive_cap                = (ul_tmp_txbf_elem & BIT0);
    pst_ht_hdl->bit_receive_staggered_sounding_cap  = ((ul_tmp_txbf_elem & BIT1) >> 1);
    pst_ht_hdl->bit_transmit_staggered_sounding_cap = ((ul_tmp_txbf_elem & BIT2) >> 2);
    pst_ht_hdl->bit_receive_ndp_cap                 = ((ul_tmp_txbf_elem & BIT3) >> 3);
    pst_ht_hdl->bit_transmit_ndp_cap                = ((ul_tmp_txbf_elem & BIT4) >> 4);
    pst_ht_hdl->bit_imbf_cap                        = ((ul_tmp_txbf_elem & BIT5) >> 5);
    pst_ht_hdl->bit_calibration                     = ((ul_tmp_txbf_elem & 0x000000C0) >> 6);
    pst_ht_hdl->bit_exp_csi_txbf_cap                = ((ul_tmp_txbf_elem & BIT8) >> 8);
    pst_ht_hdl->bit_exp_noncomp_txbf_cap            = ((ul_tmp_txbf_elem & BIT9) >> 9);
    pst_ht_hdl->bit_exp_comp_txbf_cap               = ((ul_tmp_txbf_elem & BIT10) >> 10);
    pst_ht_hdl->bit_exp_csi_feedback                = ((ul_tmp_txbf_elem & 0x00001800) >> 11);
    pst_ht_hdl->bit_exp_noncomp_feedback            = ((ul_tmp_txbf_elem & 0x00006000) >> 13);

    pst_ht_hdl->bit_exp_comp_feedback               = ((ul_tmp_txbf_elem & 0x0001C000) >> 15);
    pst_ht_hdl->bit_min_grouping                    = ((ul_tmp_txbf_elem & 0x00060000) >> 17);
    pst_ht_hdl->bit_csi_bfer_ant_number             = ((ul_tmp_txbf_elem & 0x001C0000) >> 19);
    pst_ht_hdl->bit_noncomp_bfer_ant_number         = ((ul_tmp_txbf_elem & 0x00600000) >> 21);
    pst_ht_hdl->bit_comp_bfer_ant_number            = ((ul_tmp_txbf_elem & 0x01C00000) >> 23);
    pst_ht_hdl->bit_csi_bfee_max_rows               = ((ul_tmp_txbf_elem & 0x06000000) >> 25);
    pst_ht_hdl->bit_channel_est_cap                 = ((ul_tmp_txbf_elem & 0x18000000) >> 27);
}



oal_uint32  hmac_search_ht_cap_ie_ap_etc(
                hmac_vap_stru               *pst_hmac_vap,
                hmac_user_stru              *pst_hmac_user_sta,
                oal_uint8                   *puc_payload,
                oal_uint16                   us_current_offset,
                oal_bool_enum                en_prev_asoc_ht)
{
    oal_uint8                   uc_smps;
    oal_uint8                   uc_supported_channel_width;
    oal_uint8                   uc_ht_green_field;
    oal_uint8                   uc_lsig_txop_protection_support;
    oal_uint8                   uc_mcs_bmp_index;
    oal_uint8                  *puc_tmp_payload;
    oal_uint16                  us_tmp_info_elem = 0;
    oal_uint16                  us_tmp_txbf_low = 0;
    oal_uint32                  ul_tmp_txbf_elem = 0;

    mac_user_ht_hdl_stru        *pst_ht_hdl;
    mac_user_ht_hdl_stru         st_ht_hdl;
    mac_vap_stru                *pst_mac_vap;
    mac_user_stru               *pst_mac_user;
    mac_device_stru             *pst_mac_device;

    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                                 "{hmac_search_ht_cap_ie_ap_etc::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 保存 入参 */
    puc_tmp_payload =  puc_payload;

    pst_ht_hdl    = &st_ht_hdl;
    mac_user_get_ht_hdl_etc(&(pst_hmac_user_sta->st_user_base_info), pst_ht_hdl);
    pst_mac_vap   = &(pst_hmac_vap->st_vap_base_info);
    pst_mac_user  = &(pst_hmac_user_sta->st_user_base_info);

    /* 带有 HT Capability Element 的 STA，标示它具有HT capable. */
    pst_ht_hdl->en_ht_capable = 1;
    us_current_offset += MAC_IE_HDR_LEN;

    /***************************************************************************
                    解析 HT Capabilities Info Field
    ***************************************************************************/

    us_tmp_info_elem = OAL_MAKE_WORD16(puc_tmp_payload[us_current_offset], puc_tmp_payload[us_current_offset + 1]);

    /* 检查STA所支持的LDPC编码能力 B0，0:不支持，1:支持 */
    pst_ht_hdl->bit_ldpc_coding_cap = (us_tmp_info_elem & BIT0);

    /* 检查STA所支持的信道宽度 B1，0:仅20M运行，1:20M与40M运行 */
    uc_supported_channel_width = (us_tmp_info_elem & BIT1) >> 1;
    pst_ht_hdl->bit_supported_channel_width = mac_ie_proc_ht_supported_channel_width_etc(pst_mac_user, pst_mac_vap, uc_supported_channel_width, en_prev_asoc_ht);

    /* 检查空间复用节能模式 B2~B3 */
    uc_smps = ((us_tmp_info_elem & (BIT3 | BIT2)) >> 2);
    pst_ht_hdl->bit_sm_power_save = mac_ie_proc_sm_power_save_field_etc(pst_mac_user, uc_smps);

    /* 检查Greenfield 支持情况 B4， 0:不支持，1:支持*/
    uc_ht_green_field = (us_tmp_info_elem & BIT4) >> 4;
    pst_ht_hdl->bit_ht_green_field = mac_ie_proc_ht_green_field_etc(pst_mac_user, pst_mac_vap, uc_ht_green_field, en_prev_asoc_ht);

    /* 检查20MHz Short-GI B5,  0:不支持，1:支持，之后与AP取交集  */
    pst_ht_hdl->bit_short_gi_20mhz = ((us_tmp_info_elem & BIT5) >> 5);
    pst_ht_hdl->bit_short_gi_20mhz &= mac_mib_get_ShortGIOptionInTwentyImplemented(pst_mac_vap);
    /* 检查40MHz Short-GI B6,  0:不支持，1:支持，之后与AP取交集 */
    pst_ht_hdl->bit_short_gi_40mhz = ((us_tmp_info_elem & BIT6) >> 6);
    pst_ht_hdl->bit_short_gi_40mhz &= mac_mib_get_ShortGIOptionInFortyImplemented(&pst_hmac_vap->st_vap_base_info);

    /* 检查支持接收STBC PPDU B8,  0:不支持，1:支持 */
    pst_ht_hdl->bit_rx_stbc = ((us_tmp_info_elem & 0x0300) >> 8);

    /* 检查最大A-MSDU长度 B11，0:3839字节, 1:7935字节 */
    pst_hmac_user_sta->us_amsdu_maxsize = (0 == (us_tmp_info_elem & BIT11)) ? WLAN_MIB_MAX_AMSDU_LENGTH_SHORT : WLAN_MIB_MAX_AMSDU_LENGTH_LONG;

    /* 检查在40M上DSSS/CCK的支持情况 B12 */
    /* 在非Beacon帧或probe rsp帧中时 */
    /* 0: STA在40MHz上不使用DSSS/CCK, 1: STA在40MHz上使用DSSS/CCK */
    pst_ht_hdl->bit_dsss_cck_mode_40mhz = ((us_tmp_info_elem & BIT12) >> 12);

    if ((0 == pst_ht_hdl->bit_dsss_cck_mode_40mhz)
     && (1 == pst_ht_hdl->bit_supported_channel_width))
    {
        pst_hmac_vap->st_vap_base_info.st_protection.uc_sta_no_40dsss_cck_num++;
    }

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    /* 检查Forty MHz Intolerant */
    pst_ht_hdl->bit_forty_mhz_intolerant = ((us_tmp_info_elem & BIT14) >> 14);
#endif
    /*  检查对L-SIG TXOP 保护的支持情况 B15, 0:不支持，1:支持 */
    uc_lsig_txop_protection_support = (us_tmp_info_elem & BIT15) >> 15;
    pst_ht_hdl->bit_lsig_txop_protection = mac_ie_proc_lsig_txop_protection_support_etc(pst_mac_user, pst_mac_vap, uc_lsig_txop_protection_support, en_prev_asoc_ht);

    us_current_offset += MAC_HT_CAPINFO_LEN;

    /***************************************************************************
                        解析 A-MPDU Parameters Field
    ***************************************************************************/

    /* 提取 Maximum Rx A-MPDU factor (B1 - B0) */
    pst_ht_hdl->uc_max_rx_ampdu_factor = (puc_tmp_payload[us_current_offset] & 0x03);

    /* 提取 the Minimum MPDU Start Spacing (B2 - B4) */
    pst_ht_hdl->uc_min_mpdu_start_spacing = (puc_tmp_payload[us_current_offset] >> 2)  & 0x07;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    /* MMSS 若小于WLAN_AMPDU_MIN_START_SPACING，则限制支持为WLAN_AMPDU_MIN_START_SPACING，
       以保证仪器吞吐量问题, 并且规避可能出现的dataflow break, 02不存在此问题 */
    if (pst_ht_hdl->uc_min_mpdu_start_spacing < WLAN_AMPDU_MIN_START_SPACING)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_search_ht_cap_ie_ap_etc::uc_min_mpdu_start_spacing [%d].}", pst_ht_hdl->uc_min_mpdu_start_spacing);
        pst_ht_hdl->uc_min_mpdu_start_spacing = WLAN_AMPDU_MIN_START_SPACING;
    }
#endif

    us_current_offset += MAC_HT_AMPDU_PARAMS_LEN;

    /***************************************************************************
                        解析 Supported MCS Set Field
    ***************************************************************************/
    for (uc_mcs_bmp_index = 0; uc_mcs_bmp_index < WLAN_HT_MCS_BITMASK_LEN; uc_mcs_bmp_index++)
    {
        pst_ht_hdl->uc_rx_mcs_bitmask[uc_mcs_bmp_index] =
        //        (mac_mib_get_SupportedMCSTxValue(&pst_hmac_vap->st_vap_base_info, uc_mcs_bmp_index))&
              (*(oal_uint8 *)(puc_tmp_payload + us_current_offset + uc_mcs_bmp_index));
    }

    pst_ht_hdl->uc_rx_mcs_bitmask[WLAN_HT_MCS_BITMASK_LEN - 1] &= 0x1F;

    us_current_offset += MAC_HT_SUP_MCS_SET_LEN;

    /***************************************************************************
                        解析 HT Extended Capabilities Info Field
    ***************************************************************************/
    us_tmp_info_elem = OAL_MAKE_WORD16(puc_tmp_payload[us_current_offset], puc_tmp_payload[us_current_offset + 1]);

    /* 提取 HTC support Information */
    if (0 != (us_tmp_info_elem & BIT10))
    {
        pst_ht_hdl->uc_htc_support = 1;
    }

    us_current_offset += MAC_HT_EXT_CAP_LEN;

    /***************************************************************************
                        解析 Tx Beamforming Field
    ***************************************************************************/
    us_tmp_info_elem = OAL_MAKE_WORD16(puc_tmp_payload[us_current_offset], puc_tmp_payload[us_current_offset + 1]);
    us_tmp_txbf_low	 = OAL_MAKE_WORD16(puc_tmp_payload[us_current_offset + 2], puc_tmp_payload[us_current_offset + 3]);
    ul_tmp_txbf_elem = OAL_MAKE_WORD32(us_tmp_info_elem, us_tmp_txbf_low);
    hmac_search_txbf_cap_ie_ap_etc(pst_ht_hdl, ul_tmp_txbf_elem);


    mac_user_set_ht_hdl_etc(&(pst_hmac_user_sta->st_user_base_info), pst_ht_hdl);
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    hmac_chan_update_40M_intol_user_etc(pst_mac_vap);
    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_search_ht_cap_ie_ap_etc::en_40M_intol_user[%d].}", pst_mac_vap->en_40M_intol_user);

    /* 如果存在40M不容忍的user，则不允许AP在40MHz运行 */
    if (OAL_TRUE == pst_mac_vap->en_40M_intol_user)
    {
        if(pst_mac_vap->st_cap_flag.bit_2040_autoswitch)
        {
            hmac_40M_intol_sync_data(pst_mac_vap, WLAN_BAND_WIDTH_BUTT, pst_mac_vap->en_40M_intol_user);
        }

    }
    else
    {
        mac_mib_set_FortyMHzIntolerant(pst_mac_vap, OAL_FALSE);
    }
#endif
    return OAL_SUCC;
}


oal_void hmac_vap_net_stopall_etc(oal_void)
{
    oal_uint8 uc_vap_id;
    oal_net_device_stru  *pst_net_device = NULL;
    hmac_vap_stru    *pst_hmac_vap = NULL;
    for (uc_vap_id = 0; uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++)
    {
        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
        if(NULL == pst_hmac_vap)
            break;

        pst_net_device = pst_hmac_vap->pst_net_device;

        if(NULL == pst_net_device)
            break;

        oal_net_tx_stop_all_queues(pst_net_device);
        //OAL_IO_PRINT("stop net device:%p\n", pst_net_device);
    }
}

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW

oal_void hmac_set_ampdu_worker(oal_delayed_work *pst_work)
{
    hmac_vap_stru     *pst_hmac_vap;

    pst_hmac_vap = OAL_CONTAINER_OF(pst_work, hmac_vap_stru, st_ampdu_work);

    /* 配置完毕后再开启使能聚合 */
    mac_mib_set_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info, OAL_TRUE);
    OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_set_ampdu_worker:: enable ampdu!}");
}


oal_void hmac_set_ampdu_hw_worker(oal_delayed_work *pst_work)
{
    oal_uint32                  ul_ret;
    hmac_vap_stru     *pst_hmac_vap;
    mac_cfg_ampdu_tx_on_param_stru      st_ampdu_tx_on = {0};

    pst_hmac_vap = OAL_CONTAINER_OF(pst_work, hmac_vap_stru, st_set_hw_work);

    /* 聚合硬化:4 */
    if ((pst_hmac_vap->st_mode_set.uc_aggr_tx_on >> 2) & BIT0)
    {
        st_ampdu_tx_on.uc_aggr_tx_on = 1;
        st_ampdu_tx_on.uc_snd_type   = pst_hmac_vap->st_mode_set.uc_snd_type;
    }
    else/* 切换为软件聚合:8 */
    {
        st_ampdu_tx_on.uc_aggr_tx_on = 0;
        st_ampdu_tx_on.uc_snd_type   = 0;
    }

    /***************************************************************************
            抛事件到DMAC层, 同步DMAC数据
     ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(&pst_hmac_vap->st_vap_base_info, WLAN_CFGID_AMPDU_TX_ON, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru), (oal_uint8*)&st_ampdu_tx_on);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG,"hmac_set_ampdu_hw_worker::hmac_config_send_event_etc fail(%u)",ul_ret);
    }
    OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_set_ampdu_hw_worker:: en_tx_aggr_on[0x%x],snd type[%d]!}",
         pst_hmac_vap->st_mode_set.uc_aggr_tx_on, pst_hmac_vap->st_mode_set.uc_snd_type);
}
#endif

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

oal_bool_enum_uint8 hmac_flowctl_check_device_is_sta_mode_etc(oal_void)
{
    mac_device_stru         *pst_dev;
    mac_vap_stru            *pst_vap;
    oal_uint8                uc_vap_index;
    oal_uint8                uc_device_max;
    oal_uint8                uc_device;
    oal_bool_enum_uint8      en_device_is_sta = OAL_FALSE;
    mac_chip_stru           *pst_mac_chip;

    pst_mac_chip = hmac_res_get_mac_chip(0);

    /* OAL接口获取支持device个数 */
    uc_device_max = oal_chip_get_device_num_etc(pst_mac_chip->ul_chip_ver);

    for (uc_device = 0; uc_device < uc_device_max; uc_device++)
    {
        pst_dev = mac_res_get_dev_etc(pst_mac_chip->auc_device_id[uc_device]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dev))
        {
            continue;
        }

        if (pst_dev->uc_vap_num >= 1)
        {
            for(uc_vap_index = 0; uc_vap_index < pst_dev->uc_vap_num; uc_vap_index++)
            {
                pst_vap = mac_res_get_mac_vap(pst_dev->auc_vap_id[uc_vap_index]);
                if (OAL_PTR_NULL == pst_vap)
                {
                    OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_flowctl_check_device_is_sta_mode_etc::mac_res_get_mac_vap fail.vap_index = %u}",uc_vap_index);
                    continue;
                }

                if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
                {
                    en_device_is_sta = OAL_TRUE;
                    break;
                }
            }
        }

    }
    return en_device_is_sta;
}


OAL_STATIC oal_void hmac_vap_wake_subq(oal_uint8 uc_vap_id, oal_uint16 us_queue_idx)
{
    oal_net_device_stru    *pst_net_device = NULL;
    hmac_vap_stru          *pst_hmac_vap = NULL;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
    if(NULL == pst_hmac_vap)
    {
        return;
    }

#if 0
    if ((MAC_VAP_STATE_UP != pst_hmac_vap->st_vap_base_info.en_vap_state))
    {
        return;
    }
#endif

    pst_net_device = pst_hmac_vap->pst_net_device;
    if(NULL == pst_net_device)
    {
        return;
    }

    oal_net_wake_subqueue(pst_net_device, us_queue_idx);
}


oal_void hmac_vap_net_start_subqueue_etc(oal_uint16 us_queue_idx)
{
    oal_uint8               uc_vap_id;
    OAL_STATIC oal_uint8    g_uc_start_subq_flag = 0;

     /*自旋锁内，任务和软中断都被锁住，不需要FRW锁*/

    if (0 == g_uc_start_subq_flag)
    {
        g_uc_start_subq_flag = 1;

        /* vap id从低到高恢复 跳过配置vap */
        for (uc_vap_id = oal_board_get_service_vap_start_id(); uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++)
        {
            hmac_vap_wake_subq(uc_vap_id, us_queue_idx);
        }
    }
    else
    {
        g_uc_start_subq_flag = 0;

        /* vap id从高到低恢复 */
        for (uc_vap_id = WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id > oal_board_get_service_vap_start_id(); uc_vap_id--)
        {
            hmac_vap_wake_subq(uc_vap_id - 1, us_queue_idx);
        }
    }

}


OAL_STATIC oal_void  hmac_vap_stop_subq(oal_uint8 uc_vap_id, oal_uint16 us_queue_idx)
{
    oal_net_device_stru    *pst_net_device = NULL;
    hmac_vap_stru          *pst_hmac_vap = NULL;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
    if(NULL == pst_hmac_vap)
    {
        return;
    }

#if 0
    if ((MAC_VAP_STATE_UP != pst_hmac_vap->st_vap_base_info.en_vap_state))
    {
        return;
    }
#endif

    pst_net_device = pst_hmac_vap->pst_net_device;
    if(NULL == pst_net_device)
    {
        return;
    }

    oal_net_stop_subqueue(pst_net_device, us_queue_idx);
}



oal_void hmac_vap_net_stop_subqueue_etc(oal_uint16 us_queue_idx)
{
    oal_uint8               uc_vap_id;
    OAL_STATIC oal_uint8    g_uc_stop_subq_flag = 0;

    /*自旋锁内，任务和软中断都被锁住，不需要FRW锁*/

    /* 由按照VAP ID顺序停止subq，改为不依据VAP ID顺序 */
    if (0 == g_uc_stop_subq_flag)
    {
        g_uc_stop_subq_flag = 1;

        for (uc_vap_id = oal_board_get_service_vap_start_id(); uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++)
        {
            hmac_vap_stop_subq(uc_vap_id, us_queue_idx);
        }
    }
    else
    {
        g_uc_stop_subq_flag = 0;

        for (uc_vap_id = WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id > oal_board_get_service_vap_start_id(); uc_vap_id--)
        {
            hmac_vap_stop_subq(uc_vap_id - 1, us_queue_idx);
        }
    }
}
#endif


oal_void hmac_vap_net_startall_etc(oal_void)
{
    oal_uint8 uc_vap_id;
    oal_net_device_stru  *pst_net_device = NULL;
    hmac_vap_stru    *pst_hmac_vap = NULL;
    for (uc_vap_id = 0; uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++)
    {
        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
        if(NULL == pst_hmac_vap)
            break;

        pst_net_device = pst_hmac_vap->pst_net_device;

        if(NULL == pst_net_device)
            break;

        oal_net_tx_wake_all_queues(pst_net_device);
        //OAL_IO_PRINT("start net device:%p\n", pst_net_device);
    }
}

#ifdef _PRE_WLAN_FEATURE_P2P

oal_void hmac_del_virtual_inf_worker_etc(oal_work_stru *pst_del_virtual_inf_work)
{
    oal_net_device_stru         *pst_net_dev;
    hmac_vap_stru               *pst_hmac_vap;
    oal_wireless_dev_stru       *pst_wireless_dev;
    hmac_device_stru            *pst_hmac_device;

    pst_hmac_vap     = OAL_CONTAINER_OF(pst_del_virtual_inf_work, hmac_vap_stru, st_del_virtual_inf_worker);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_P2P, "{hmac_del_virtual_inf_worker_etc:: hmac_vap is null}");
        return;
    }
    pst_net_dev      = pst_hmac_vap->pst_del_net_device;
    if (OAL_PTR_NULL == pst_net_dev)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P,"{hmac_del_virtual_inf_worker_etc:: net_dev is null}");
        return;
    }
    pst_wireless_dev = OAL_NETDEVICE_WDEV(pst_net_dev);

    /* 不存在rtnl_lock锁问题 */
    oal_net_unregister_netdev(pst_net_dev);

	/* 在释放net_device 后释放wireless_device 内存 */
    OAL_MEM_FREE(pst_wireless_dev, OAL_TRUE);

    pst_hmac_vap->pst_del_net_device = OAL_PTR_NULL;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (pst_hmac_device == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P,
                    "{hmac_del_virtual_inf_worker_etc::pst_hmac_device is null !device_id[%d]}",
                    pst_hmac_vap->st_vap_base_info.uc_device_id);
        return;
    }

    hmac_clr_p2p_status_etc(&pst_hmac_device->ul_p2p_intf_status, P2P_STATUS_IF_DELETING);

    OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_P2P,
                    "{hmac_del_virtual_inf_worker_etc::end !pst_hmac_device->ul_p2p_intf_status[%x]}",
                    pst_hmac_device->ul_p2p_intf_status);
    OAL_SMP_MB();
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&pst_hmac_device->st_netif_change_event);

}
#endif	/* _PRE_WLAN_FEATURE_P2P */


oal_void hmac_handle_disconnect_rsp_etc(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user,
                                                  mac_reason_code_enum_uint16  en_disasoc_reason)
{
     /* 修改 state & 删除 user */
    switch (pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        case WLAN_VAP_MODE_BSS_AP:
            {
                /* 抛事件上报内核，已经去关联某个STA */
                hmac_handle_disconnect_rsp_ap_etc(pst_hmac_vap, pst_hmac_user);
            }
            break;

         case WLAN_VAP_MODE_BSS_STA:
            {
                /* 上报内核sta已经和某个ap去关联 */
                hmac_sta_handle_disassoc_rsp_etc(pst_hmac_vap, en_disasoc_reason);
            }
             break;
         default:
             break;
    }
    return;
}


oal_uint8 * hmac_vap_get_pmksa_etc(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_bssid)
{
    hmac_pmksa_cache_stru              *pst_pmksa_cache = OAL_PTR_NULL;
    oal_dlist_head_stru                *pst_pmksa_entry;
    oal_dlist_head_stru                *pst_pmksa_entry_tmp;

    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == puc_bssid)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_vap_get_pmksa_etc param null}\r\n");
        return OAL_PTR_NULL;
    }

    if (oal_dlist_is_empty(&(pst_hmac_vap->st_pmksa_list_head)))
    {
        return OAL_PTR_NULL;
    }

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_pmksa_entry, pst_pmksa_entry_tmp, &(pst_hmac_vap->st_pmksa_list_head))
    {
        pst_pmksa_cache = OAL_DLIST_GET_ENTRY(pst_pmksa_entry, hmac_pmksa_cache_stru, st_entry);

        if (0 == oal_compare_mac_addr(puc_bssid, pst_pmksa_cache->auc_bssid))
        {
            oal_dlist_delete_entry(pst_pmksa_entry);
            OAM_WARNING_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG,
                             "{hmac_vap_get_pmksa_etc:: FIND Pmksa of [%02X:XX:XX:XX:%02X:%02X]}",
                             puc_bssid[0], puc_bssid[4], puc_bssid[5]);
            break;
        }
        pst_pmksa_cache = OAL_PTR_NULL;
    }

    if (pst_pmksa_cache)
    {
        oal_dlist_add_head(&(pst_pmksa_cache->st_entry), &(pst_hmac_vap->st_pmksa_list_head));
        return pst_pmksa_cache->auc_pmkid;
    }
    return OAL_PTR_NULL;
}


oal_uint32 hmac_tx_get_mac_vap_etc(oal_uint8 uc_vap_id, mac_vap_stru **pst_vap_stru)
{
    mac_vap_stru         *pst_vap;

    /* 获取vap结构信息 */
    pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_TX, "{hmac_tx_get_mac_vap_etc::pst_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAP模式判断 */
    if (WLAN_VAP_MODE_BSS_AP != pst_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "hmac_tx_get_mac_vap_etc::vap_mode error[%d]", pst_vap->en_vap_mode);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    /* VAP状态判断 */
    if (MAC_VAP_STATE_UP != pst_vap->en_vap_state && MAC_VAP_STATE_PAUSE != pst_vap->en_vap_state)
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_TX, "hmac_tx_get_mac_vap_etc::vap_state[%d] error", pst_vap->en_vap_state);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    *pst_vap_stru = pst_vap;

    return OAL_SUCC;
}


oal_uint32 hmac_restart_all_work_vap(oal_uint8 uc_chip_id)
{
    mac_chip_stru      *pst_mac_chip;
    mac_device_stru    *pst_mac_dev;
    hmac_vap_stru      *pst_hmac_vap;
    oal_uint8           uc_dev_idx;
    oal_uint8           uc_vap_idx;
    mac_cfg_down_vap_param_stru   st_down_vap;
    mac_cfg_start_vap_param_stru  st_start_vap;
    oal_uint32          ul_ret;

    pst_mac_chip = hmac_res_get_mac_chip(uc_chip_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_chip))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_restart_all_work_vap::pst_mac_chip null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 遍历chip下的所有vap，将work的vap down掉后再up */
    for (uc_dev_idx = 0; uc_dev_idx < pst_mac_chip->uc_device_nums; uc_dev_idx++)
    {
        pst_mac_dev = mac_res_get_dev_etc(pst_mac_chip->auc_device_id[uc_dev_idx]);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
        {
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_restart_all_work_vap::pst_mac_dev null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        /* 遍历dev下的所有vap */
        for (uc_vap_idx = 0; uc_vap_idx < pst_mac_dev->uc_vap_num; uc_vap_idx++)
        {
            pst_hmac_vap = (hmac_vap_stru*)mac_res_get_hmac_vap(pst_mac_dev->auc_vap_id[uc_vap_idx]);
            if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
            {
                OAM_WARNING_LOG0(pst_mac_dev->auc_vap_id[uc_vap_idx], OAM_SF_CFG, "{hmac_restart_all_work_vap::pst_hmac_vap null.}");
                return OAL_ERR_CODE_PTR_NULL;
            }

            /* update vap's maximum user num */
            mac_mib_set_MaxAssocUserNums(&pst_hmac_vap->st_vap_base_info, mac_chip_get_max_asoc_user(uc_chip_id));

            /* return if not work vap */
            if (MAC_VAP_STATE_INIT == pst_hmac_vap->st_vap_base_info.en_vap_state)
            {
                continue;
            }

            /* restart work vap */
            /* down vap */
            st_down_vap.pst_net_dev = pst_hmac_vap->pst_net_device;
            ul_ret = hmac_config_down_vap_etc(&pst_hmac_vap->st_vap_base_info,
                                          OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                                          (oal_uint8 *)&st_down_vap);
            if (ul_ret != OAL_SUCC)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_restart_all_work_vap::hmac_config_down_vap_etc failed[%d].}", ul_ret);
                return ul_ret;
            }
            /* down netdev */
            oal_net_device_close(pst_hmac_vap->pst_net_device);

            /* start vap */
            st_start_vap.pst_net_dev = pst_hmac_vap->pst_net_device;
            st_start_vap.en_mgmt_rate_init_flag = OAL_TRUE;
            ul_ret = hmac_config_start_vap_etc(&pst_hmac_vap->st_vap_base_info,
                                          OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                                          (oal_uint8 *)&st_start_vap);
            if (ul_ret != OAL_SUCC)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_config_wifi_enable::hmac_config_start_vap_etc failed[%d].}", ul_ret);
                return ul_ret;
            }
            /* up netdev */
            oal_net_device_open(pst_hmac_vap->pst_net_device);
        }
    }

    return OAL_SUCC;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(3,10,44))
OAL_STATIC oal_int32  hmac_cfg_vap_if_open(oal_net_device_stru *pst_dev)
{
    pst_dev->flags |= OAL_IFF_RUNNING;

    return OAL_SUCC;
}

OAL_STATIC oal_int32  hmac_cfg_vap_if_close(oal_net_device_stru *pst_dev)
{
    pst_dev->flags &= ~OAL_IFF_RUNNING;

    return OAL_SUCC;
}

OAL_STATIC oal_net_dev_tx_enum  hmac_cfg_vap_if_xmit(oal_netbuf_stru *pst_buf, oal_net_device_stru *pst_dev)
{
    if (pst_buf)
    {
        oal_netbuf_free(pst_buf);
    }
    return OAL_NETDEV_TX_OK;
}
#endif

/*lint -e19*/
oal_module_symbol(hmac_vap_get_priv_cfg_etc);
oal_module_symbol(hmac_vap_get_net_device_etc);
oal_module_symbol(hmac_vap_get_desired_country_etc);
oal_module_symbol(hmac_vap_destroy_etc);

#if defined (_PRE_WLAN_FEATURE_RX_AGGR_EXTEND) || defined (_PRE_FEATURE_WAVEAPP_CLASSIFY)
oal_module_symbol(g_en_wave_bind_cpu0_ctrl);
#endif

#ifdef _PRE_WLAN_FEATURE_11D
oal_module_symbol(hmac_vap_get_updata_rd_by_ie_switch_etc);
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
oal_module_symbol(hmac_del_virtual_inf_worker_etc);
#endif /* _PRE_WLAN_FEATURE_P2P */

/*lint +e19*/


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

