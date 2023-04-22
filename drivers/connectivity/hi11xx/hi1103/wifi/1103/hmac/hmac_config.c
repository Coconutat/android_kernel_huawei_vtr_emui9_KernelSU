


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "oam_trace.h"
#include "frw_ext_if.h"
#include "hmac_device.h"
#include "mac_resource.h"
#include "hmac_resource.h"
#include "mac_vap.h"
#include "mac_ie.h"
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
#include "oal_hcc_host_if.h"
#endif
#include "mac_user.h"
#include "mac_regdomain.h"
#include "dmac_ext_if.h"
#include "hmac_ext_if.h"
#include "hmac_fsm.h"
#include "hmac_main.h"
#include "hmac_vap.h"
#include "hmac_tx_amsdu.h"
#include "hmac_rx_data.h"
#include "hmac_mgmt_classifier.h"
#include "hmac_config.h"
#include "hmac_chan_mgmt.h"
#include "hmac_rx_filter.h"
#include "hmac_psm_ap.h"
#ifdef _PRE_WLAN_CHIP_TEST
#include "hmac_test_main.h"
#include "oal_schedule.h"
#endif
#include "hmac_protection.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_encap_frame_sta.h"
#include "hmac_data_acq.h"
#include "hmac_rx_filter.h"
#include "hmac_mgmt_sta.h"
#ifdef _PRE_WLAN_FEATURE_ISOLATION
#include "hmac_isolation.h"
#endif
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
#include "hmac_blacklist.h"
#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
#include "hmac_m2u.h"
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
#include "hmac_proxy_arp.h"
#endif

#include "hmac_scan.h"
#include "hmac_fbt_main.h"
#include "hmac_dfs.h"
#include "hmac_reset.h"
#include "hmac_scan.h"
#include "hmac_blockack.h"
#include "hmac_p2p.h"
#include "hmac_mgmt_ap.h"
#include "oal_kernel_file.h"
#include "oal_profiling.h"

#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
#include "hmac_arp_offload.h"
#endif
#ifdef _PRE_WLAN_TCP_OPT
#include "mac_data.h"
#include "hmac_tcp_opt.h"
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif
#ifdef _PRE_WLAN_DFT_STAT
#include "mac_board.h"
#endif
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#include  "hmac_auto_adjust_freq.h"
#endif
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#include "hmac_proxysta.h"
#endif
#ifdef _PRE_WLAN_FEATURE_SINGLE_PROXYSTA
#include "hmac_single_proxysta.h"
#endif
#include "hmac_dfx.h"

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#include "hisi_customize_wifi.h"
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */
#ifdef _PRE_WLAN_FEATURE_11R_AP
#include "hmac_11r_ap.h"
#endif

#ifdef _PRE_WLAN_FEATURE_SMARTANT
#include "wal_linux_atcmdsrv.h"
#endif

#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
#include "hmac_wds.h"
#endif

#ifdef _PRE_WLAN_FEATURE_CAR
#include "hmac_car.h"
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
#include "hmac_11k.h"
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "board.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CONFIG_C

#ifdef _PRE_WLAN_FEATURE_DFR
extern hmac_dfr_info_stru g_st_dfr_info_etc;
#endif //_PRE_WLAN_FEATURE_DFR


extern oal_uint8    g_sk_pacing_shift_etc;


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_WLAN_CHIP_TEST_ALG)
struct kobject     *g_alg_test_sys_kobject = OAL_PTR_NULL;
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
hmac_tx_pkts_stat_stru   g_host_tx_pkts;
#endif

typedef struct
{
    wlan_protocol_enum_uint8   en_protocol_mode;                /* wid枚举 */
    oal_uint8                  auc_resv[3];
    oal_int8                  *puc_protocol_desc;
}hmac_protocol_stru;

OAL_STATIC hmac_protocol_stru gst_protocol_mode_list[WLAN_PROTOCOL_BUTT] =
{
    {WLAN_LEGACY_11A_MODE,      {0}, "11a" },
    {WLAN_LEGACY_11B_MODE,      {0}, "11b" },
    {WLAN_LEGACY_11G_MODE,      {0}, "abandon_mode" },
    {WLAN_MIXED_ONE_11G_MODE,   {0}, "11bg"},
    {WLAN_MIXED_TWO_11G_MODE,   {0}, "11g"},
    {WLAN_HT_MODE,              {0}, "11n" },
    {WLAN_VHT_MODE,             {0}, "11ac"},
    {WLAN_HT_ONLY_MODE,         {0}, "11n_only"},
    {WLAN_VHT_ONLY_MODE,        {0}, "11ac_only"},
};
#ifdef _PRE_WLAN_CHIP_TEST_ALG
#define HMAC_ALG_TEST_BUF_SIZE  128
typedef struct
{

    oal_wait_queue_head_stru        st_wait_queue;                        /* 线程等待结构体,用于WAL_Linux层线程等待(WAL_Linuc -> WAL_Config) */
    OAL_VOLATILE   oal_uint8        auc_data[HMAC_ALG_TEST_BUF_SIZE];
}alg_test_main_hmac_stru;
alg_test_main_hmac_stru g_st_alg_test_hmac;


#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_WLAN_CHIP_TEST_ALG)
OAL_STATIC oal_ssize_t  hmac_alg_test_result_proc_read(oal_device_stru *dev, oal_device_attribute_stru *attr, char *buf);

OAL_STATIC OAL_DEVICE_ATTR(alg_test_result, OAL_S_IRUGO|OAL_S_IWUSR, hmac_alg_test_result_proc_read, OAL_PTR_NULL);

#endif


#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
OAL_STATIC oal_uint32  hmac_config_get_param_from_dmac_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param, hmac_vap_query_enent_id_enumm_uint8 en_query_id);
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
OAL_STATIC oal_uint32  hmac_config_get_sta_diag_info_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  hmac_config_get_vap_diag_info_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
OAL_STATIC oal_uint32  hmac_config_get_sensing_bssid_info_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_TEMP_PROTECT
OAL_STATIC oal_uint32 hmac_config_receive_all_sta_rssi(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif
#if 0
OAL_STATIC oal_int8 hmac_config_get_free_power(mac_vap_stru *pst_mac_vap);
#endif
extern oal_void  hmac_rx_filter_init_multi_vap(oal_uint32 ul_proxysta_enabled);
oal_uint32  hmac_config_set_freq_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
oal_uint32  hmac_config_set_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param);
/*****************************************************************************
  3 函数实现
*****************************************************************************/
oal_int8* hmac_config_index2string_etc(oal_uint32 ul_index, oal_int8* pst_string[], oal_uint32 ul_max_str_nums)
{
    if(ul_index >= ul_max_str_nums)
    {
        return (oal_int8*)"unkown";
    }
    return pst_string[ul_index];
}

oal_int8* hmac_config_protocol2string_etc(oal_uint32 ul_protocol)
{
    oal_int8  *pac_protocol2string[] = {"11a", "11b", "error", "11bg", "11g", "11n", "11ac", "11nonly", "11aconly", "11ng","11ax","error"};
    return hmac_config_index2string_etc(ul_protocol, pac_protocol2string, OAL_SIZEOF(pac_protocol2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_band2string_etc(oal_uint32 ul_band)
{
    oal_int8        *pac_band2string[]     = {"2.4G", "5G", "error"};
    return hmac_config_index2string_etc(ul_band, pac_band2string, OAL_SIZEOF(pac_band2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_bw2string_etc(oal_uint32 ul_bw)
{

    oal_int8        *pac_bw2string[]       = {"20M", "40+", "40-", "80++", "80+-", "80-+", "80--",
#ifdef _PRE_WLAN_FEATURE_160M
    "160+++", "160++-", "160+-+", "160+--", "160-++","160-+-","160--+","160---",
#endif
    "error"};
    return hmac_config_index2string_etc(ul_bw, pac_bw2string, OAL_SIZEOF(pac_bw2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_ciper2string_etc(oal_uint32 ul_ciper2)
{
    oal_int8        *pac_ciper2string[]    = {"GROUP", "WEP40", "TKIP", "RSV", "CCMP", "WEP104", "BIP", "NONE"};
    return hmac_config_index2string_etc(ul_ciper2, pac_ciper2string, OAL_SIZEOF(pac_ciper2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_akm2string_etc(oal_uint32 ul_akm2)
{
    oal_int8        *pac_akm2string[]      = {"RSV", "1X", "PSK", "FT_1X", "FT_PSK", "1X_SHA256", "PSK_SHA256", "NONE"};
    return hmac_config_index2string_etc(ul_akm2, pac_akm2string, OAL_SIZEOF(pac_akm2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_keytype2string_etc(oal_uint32 ul_keytype)
{
    oal_int8        *pac_keytype2string[] = {"GTK", "PTK", "RX_GTK", "ERR"};
    return hmac_config_index2string_etc(ul_keytype, pac_keytype2string, OAL_SIZEOF(pac_keytype2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_cipher2string_etc(oal_uint32 ul_cipher)
{
    oal_int8        *pac_cipher2string[]  = {"GROUP", "WEP40", "TKIP", "NO_ENCRYP", "CCMP", "WEP104", "BIP", "GROUP_DENYD",};
    return hmac_config_index2string_etc(ul_cipher, pac_cipher2string, OAL_SIZEOF(pac_cipher2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_smps2string_etc(oal_uint32 ul_smps)
{
    oal_int8        *pac_smps2string[] = {"Static", "Dynamic", "MIMO", "error"};
    return hmac_config_index2string_etc(ul_smps, pac_smps2string, OAL_SIZEOF(pac_smps2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_dev2string_etc(oal_uint32 ul_dev)
{
    oal_int8        *pac_dev2string[]  = {"Close", "Open", "error"};
    return hmac_config_index2string_etc(ul_dev, pac_dev2string, OAL_SIZEOF(pac_dev2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_nss2string_etc(oal_uint32 ul_nss)
{
    oal_int8        *pac_nss2string[] = {"Single Nss", "Double Nss", "error"};
    return hmac_config_index2string_etc(ul_nss, pac_nss2string, OAL_SIZEOF(pac_nss2string)/OAL_SIZEOF(oal_int8 *));
}

oal_int8* hmac_config_b_w2string_etc(oal_uint32 ul_b_w)
{
    oal_int8        *pac_bw2string[]  = {"20M", "40M", "80M", "error"};
    return hmac_config_index2string_etc(ul_b_w, pac_bw2string, OAL_SIZEOF(pac_bw2string)/OAL_SIZEOF(oal_int8 *));
}


OAL_STATIC oal_uint32  hmac_config_alloc_event(
                mac_vap_stru                     *pst_mac_vap,
                hmac_to_dmac_syn_type_enum_uint8  en_syn_type,
                hmac_to_dmac_cfg_msg_stru       **ppst_syn_msg,
                frw_event_mem_stru              **ppst_event_mem,
                oal_uint16                        us_len)
{
    frw_event_mem_stru *pst_event_mem;
    frw_event_stru     *pst_event;

    pst_event_mem = FRW_EVENT_ALLOC(us_len + OAL_SIZEOF(hmac_to_dmac_cfg_msg_stru) - 4);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alloc_event::pst_event_mem null, us_len = %d }", us_len);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填充事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                        FRW_EVENT_TYPE_HOST_CRX,
                        en_syn_type,
                        (us_len + OAL_SIZEOF(hmac_to_dmac_cfg_msg_stru) - 4),
                        FRW_EVENT_PIPELINE_STAGE_1,
                        pst_mac_vap->uc_chip_id,
                        pst_mac_vap->uc_device_id,
                        pst_mac_vap->uc_vap_id);

    /* 出参赋值 */
    *ppst_event_mem = pst_event_mem;
    *ppst_syn_msg   = (hmac_to_dmac_cfg_msg_stru *)pst_event->auc_event_data;

    return OAL_SUCC;
}


oal_uint32  hmac_config_send_event_etc(
                mac_vap_stru                     *pst_mac_vap,
                wlan_cfgid_enum_uint16            en_cfg_id,
                oal_uint16                        us_len,
                oal_uint8                        *puc_param)
{
    oal_uint32                  ul_ret;
    frw_event_mem_stru         *pst_event_mem;
    hmac_to_dmac_cfg_msg_stru  *pst_syn_msg;

    ul_ret = hmac_config_alloc_event(pst_mac_vap, HMAC_TO_DMAC_SYN_CFG, &pst_syn_msg, &pst_event_mem, us_len);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_event_etc::hmac_config_alloc_event failed[%d].}",ul_ret);
        return ul_ret;
    }

    HMAC_INIT_SYN_MSG_HDR(pst_syn_msg, en_cfg_id, us_len);

    /* 填写配置同步消息内容 */
    if ((OAL_PTR_NULL != puc_param) && (us_len))
    {
        oal_memcopy(pst_syn_msg->auc_msg_body, puc_param, (oal_uint32)us_len);
    }

    /* 抛出事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_event_etc::frw_event_dispatch_event_etc failed[%d].}",ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return ul_ret;
    }

    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_config_h2d_send_app_ie(mac_vap_stru  *pst_mac_vap, oal_app_ie_stru *pst_app_ie)
{
    frw_event_mem_stru       *pst_event_mem;
    frw_event_stru           *pst_event;
    oal_netbuf_stru          *pst_netbuf_app_ie;
    oal_uint16                us_frame_len;
    dmac_tx_event_stru       *pst_app_ie_event;
    oal_uint32                ul_ret;
    oal_uint8                *puc_param = OAL_PTR_NULL;
    oal_uint8                 uc_app_ie_header_len;

    if(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == pst_app_ie)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_h2d_send_app_ie::param is NULL,%p, %p.}",
          pst_mac_vap, pst_app_ie);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*帧长校验*/
    uc_app_ie_header_len = OAL_SIZEOF(oal_app_ie_stru) - OAL_SIZEOF(pst_app_ie->auc_ie)/OAL_SIZEOF(pst_app_ie->auc_ie[0]);

    us_frame_len = uc_app_ie_header_len + pst_app_ie->ul_ie_len;
    if(us_frame_len >= WLAN_LARGE_NETBUF_SIZE || pst_app_ie->ul_ie_len > WLAN_WPS_IE_MAX_SIZE)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_h2d_send_app_ie::us_frame_len =[%d] ie_len=[%d] invalid.}",
        us_frame_len, pst_app_ie->ul_ie_len);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请netbuf内存  */
    pst_netbuf_app_ie = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF,us_frame_len, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_netbuf_app_ie)
    {
       OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_h2d_send_app_ie::pst_netbuf_app_ie alloc failed.}");
       return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 申请event 事件内存    */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        oal_netbuf_free(pst_netbuf_app_ie);
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_h2d_send_app_ie::pst_event_mem alloc failed.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_WLAN_CTX,
                           DMAC_WLAN_CTX_EVENT_SUB_TYPE_APP_IE_H2D,
                           OAL_SIZEOF(dmac_tx_event_stru),
                           FRW_EVENT_PIPELINE_STAGE_1,
                           pst_mac_vap->uc_chip_id,
                           pst_mac_vap->uc_device_id,
                           pst_mac_vap->uc_vap_id);

    OAL_MEMZERO(oal_netbuf_cb(pst_netbuf_app_ie), OAL_TX_CB_LEN);

    puc_param    = (oal_uint8 *)(OAL_NETBUF_DATA(pst_netbuf_app_ie));
    oal_memcopy(puc_param,(oal_uint8 *)pst_app_ie, us_frame_len);

    pst_app_ie_event               = (dmac_tx_event_stru *)pst_event->auc_event_data;
    pst_app_ie_event->pst_netbuf   = pst_netbuf_app_ie;
    pst_app_ie_event->us_frame_len = us_frame_len;
    pst_app_ie_event->us_remain    = 0;

    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"{hmac_config_h2d_send_app_ie:: dispatch failed, app_ie_type=[%d],app_ie_len=[%d].}",
        pst_app_ie->en_app_ie_type, pst_app_ie->ul_ie_len);
        oal_netbuf_free(pst_netbuf_app_ie);
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_FAIL;
    }

    oal_netbuf_free(pst_netbuf_app_ie);
    FRW_EVENT_FREE(pst_event_mem);
    return OAL_SUCC;
}
#endif



OAL_STATIC oal_uint32  hmac_config_alg_send_event(
                mac_vap_stru                     *pst_mac_vap,
                wlan_cfgid_enum_uint16            en_cfg_id,
                oal_uint16                        us_len,
                oal_uint8                        *puc_param)
{
    oal_uint32                  ul_ret;
    frw_event_mem_stru         *pst_event_mem;
    hmac_to_dmac_cfg_msg_stru  *pst_syn_msg;

    ul_ret = hmac_config_alloc_event(pst_mac_vap, HMAC_TO_DMAC_SYN_ALG, &pst_syn_msg, &pst_event_mem, us_len);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg_send_event::hmac_config_alloc_event failed[%d].}",ul_ret);
        return ul_ret;
    }

    HMAC_INIT_SYN_MSG_HDR(pst_syn_msg, en_cfg_id, us_len);

    /* 填写配置同步消息内容 */
    oal_memcopy(pst_syn_msg->auc_msg_body, puc_param, us_len);



    /* 抛出事件 */
    //frw_event_dispatch_event_etc(pst_event_mem);
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg_send_event::frw_event_dispatch_event_etc failed[%d].}",ul_ret);
        FRW_EVENT_FREE(pst_event_mem);
        return ul_ret;
    }

    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32 hmac_config_start_vap_event_etc(mac_vap_stru  *pst_mac_vap, oal_bool_enum_uint8 en_mgmt_rate_init_flag)
{
    oal_uint32                    ul_ret;
    mac_cfg_start_vap_param_stru  st_start_vap_param;

    /* DMAC不使用netdev成员 */
    st_start_vap_param.pst_net_dev = OAL_PTR_NULL;
    st_start_vap_param.en_mgmt_rate_init_flag = en_mgmt_rate_init_flag;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    st_start_vap_param.uc_protocol = pst_mac_vap->en_protocol;
    st_start_vap_param.uc_band     = pst_mac_vap->st_channel.en_band;
    st_start_vap_param.uc_bandwidth= pst_mac_vap->st_channel.en_bandwidth;
#endif
#ifdef _PRE_WLAN_FEATURE_P2P
    st_start_vap_param.en_p2p_mode = pst_mac_vap->en_p2p_mode;
#endif
    ul_ret = hmac_config_send_event_etc(pst_mac_vap,
                                    WLAN_CFGID_START_VAP,
                                    OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                                    (oal_uint8 *)&st_start_vap_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_event_etc::Start_vap failed[%d].}",ul_ret);
    }
    return ul_ret;
}

oal_uint32 hmac_set_mode_event_etc(mac_vap_stru *pst_mac_vap)
{
    oal_uint32               ul_ret;
    mac_cfg_mode_param_stru  st_prot_param;

    /* 设置带宽模式，直接抛事件到DMAC配置寄存器 */
    st_prot_param.en_protocol  = pst_mac_vap->en_protocol;
    st_prot_param.en_band      = pst_mac_vap->st_channel.en_band;
    st_prot_param.en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_MODE, OAL_SIZEOF(mac_cfg_mode_param_stru), (oal_uint8 *)&st_prot_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_set_mode_event_etc::mode_set failed[%d],protocol[%d], band[%d], bandwidth[%d].}",
            ul_ret, pst_mac_vap->en_protocol, pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.en_bandwidth);
    }
    return ul_ret;
}

#if 0
#if defined(_PRE_WLAN_FEATURE_OPMODE_NOTIFY) || defined(_PRE_WLAN_FEATURE_SMPS) || defined(_PRE_WLAN_FEATURE_M2S)

oal_uint32 hmac_config_update_user_m2s_event(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32              ul_relt;
    mac_user_m2s_stru       st_user_m2s;
    /* opmode息同步dmac */
    st_user_m2s.en_avail_num_spatial_stream    = pst_mac_user->en_avail_num_spatial_stream;
    st_user_m2s.en_avail_bf_num_spatial_stream = pst_mac_user->en_avail_bf_num_spatial_stream;
    st_user_m2s.en_avail_bandwidth = pst_mac_user->en_avail_bandwidth;
    st_user_m2s.en_cur_bandwidth   = pst_mac_user->en_cur_bandwidth;
    st_user_m2s.us_user_idx        = pst_mac_user->us_assoc_id;
//    st_user_opmode.uc_frame_type      = uc_mgmt_frm_type;
    st_user_m2s.en_cur_smps_mode   = (wlan_mib_mimo_power_save_enum_uint8)pst_mac_user->st_ht_hdl.bit_sm_power_save;

    ul_relt = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USER_M2S_INFO_SYN,
                                    OAL_SIZEOF(st_user_m2s),
                                    (oal_uint8 *)(&st_user_m2s));
    if (OAL_UNLIKELY(OAL_SUCC != ul_relt))
    {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_CFG, "{hmac_config_update_user_m2s_event::hmac_config_send_event_etc failed[%d].}", ul_relt);
        return ul_relt;
    }

    return OAL_SUCC;
}
#endif
#endif

oal_uint32  hmac_config_sync_cmd_common_etc(mac_vap_stru *pst_mac_vap,wlan_cfgid_enum_uint16 en_cfg_id,oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, en_cfg_id, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sync_cmd_common_etc::hmac_config_send_event_etc failed[%d].}",ul_ret);
    }

    return ul_ret;

}

oal_uint32  hmac_config_open_wmm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;
    oal_bool_enum_uint8             en_wmm = OAL_TRUE;

    ul_ret = hmac_config_sync_cmd_common_etc(pst_mac_vap, WLAN_CFGID_WMM_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_open_wmm::hmac_config_sync_cmd_common_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    en_wmm = *(oal_bool_enum_uint8 *)puc_param;
    /* 开关WMM，更新host侧mib信息位中的Qos位置 */
    mac_mib_set_dot11QosOptionImplemented(pst_mac_vap, en_wmm);

    return ul_ret;

}


#if 0
OAL_STATIC oal_uint32  hmac_config_check_vap_num(mac_device_stru *pst_mac_device, wlan_vap_mode_enum_uint8 en_vap_mode)
{
    /* VAP个数判断 */
    if (WLAN_VAP_MODE_BSS_AP == en_vap_mode)
    {
        if ((1 == pst_mac_device->uc_sta_num) && (WLAN_AP_STA_COEXIST_VAP_NUM == pst_mac_device->uc_vap_num))
        {
            /* AP STA共存场景，只能创建1个AP */
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_check_vap_num::have created 1AP + 1STA, cannot create another AP.}");
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
        else if ((pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num) >= WLAN_MAX_SERVICE_AP_NUM_PER_DEVICE)
        {
            /* 已创建的AP个数达到最大值 */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::ap num exceeds the supported spec[%d].}", pst_mac_device->uc_vap_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    }
    else if (WLAN_VAP_MODE_BSS_STA == en_vap_mode)
    {
        if (pst_mac_device->uc_sta_num >= WLAN_MAX_SERVICE_STA_NUM_PER_DEVICE)
        {
            /* 已创建的STA个数达到最大值 */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::sta num exceeds the supported spec[%d].}", pst_mac_device->uc_sta_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }

        if (pst_mac_device->uc_vap_num >= WLAN_AP_STA_COEXIST_VAP_NUM)
        {
            /* 已创建了2个AP，不能再创建STA */
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_WEB_CMD_COMM

OAL_STATIC oal_uint32  hmac_config_get_param_from_dmac_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param, hmac_vap_query_enent_id_enumm_uint8 en_query_id)
{
    mac_cfg_param_char_stru *pst_param;
    hmac_vap_stru           *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_param_from_dmac_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_param = (mac_cfg_param_char_stru*)puc_param;
    oal_memcopy(pst_hmac_vap->st_param_char.auc_buff, pst_param->auc_buff, (oal_uint32)pst_param->l_buff_len);
    pst_hmac_vap->st_param_char.l_buff_len = pst_param->l_buff_len;
    pst_hmac_vap->auc_query_flag[en_query_id] = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

oal_uint32  hmac_config_get_hipkt_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_HIPKT_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_flowctl_param_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_flowctl_param_stru  *pst_flowctl_param  = (mac_cfg_flowctl_param_stru *)puc_param;

    /* 设置host flowctl 相关参数*/
    hcc_host_set_flowctl_param_etc(pst_flowctl_param->uc_queue_type, pst_flowctl_param->us_burst_limit,
            pst_flowctl_param->us_low_waterline, pst_flowctl_param->us_high_waterline);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "hcc_host_set_flowctl_param_etc, queue[%d]: burst limit = %d, low_waterline = %d, high_waterline =%d\r\n",
                    pst_flowctl_param->uc_queue_type, pst_flowctl_param->us_burst_limit, pst_flowctl_param->us_low_waterline, pst_flowctl_param->us_high_waterline);

    return OAL_SUCC;
}



oal_uint32  hmac_config_get_flowctl_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /* 设置host flowctl 相关参数*/
    hcc_host_get_flowctl_stat_etc();

    return OAL_SUCC;
}

#endif

OAL_STATIC OAL_INLINE oal_uint32 hmac_normal_check_legacy_vap_num(mac_device_stru *pst_mac_device, wlan_vap_mode_enum_uint8   en_vap_mode)
{
    /* VAP个数判断 */
    if (WLAN_VAP_MODE_BSS_AP == en_vap_mode)
    {
        if ((WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE == pst_mac_device->uc_sta_num) && (WLAN_AP_STA_COEXIST_VAP_NUM == pst_mac_device->uc_vap_num))
        {
            /* AP STA共存场景，只能创建4个AP + 1个STA */
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_normal_check_legacy_vap_num::have created 4AP + 1STA, cannot create another AP.}");
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }

        if ((pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num) >= WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE)
        {
            /* 已创建的AP个数达到最大值4 */
            OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_normal_check_legacy_vap_num::ap num exceeds the supported spec,vap_num[%u],sta_num[%u].}",
                             pst_mac_device->uc_vap_num, pst_mac_device->uc_sta_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    }
    else if (WLAN_VAP_MODE_BSS_STA == en_vap_mode)
    {
        if (pst_mac_device->uc_sta_num >= WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE)
        {
            /* 已创建的STA个数达到最大值 */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_normal_check_legacy_vap_num::have created 2+ AP.can not create STA any more[%d].}", pst_mac_device->uc_sta_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    }


    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_PROXYSTA


OAL_STATIC OAL_INLINE  oal_uint32 hmac_config_proxysta_check_vap_num(mac_device_stru          *pst_mac_device,
                                                                     mac_cfg_add_vap_param_stru *pst_param)
{
    /* VAP个数判断 */
    if (WLAN_VAP_MODE_BSS_AP == pst_param->en_vap_mode)
    {
        if ((1 == (pst_mac_device->uc_sta_num - mac_dev_xsta_num(pst_mac_device))) &&
            (WLAN_AP_STA_COEXIST_VAP_NUM == (pst_mac_device->uc_vap_num - mac_dev_xsta_num(pst_mac_device))))
        {
            /* AP STA共存场景，只能创建4个AP + 1个STA */
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_check_vap_num::have created 4AP + 1STA, cannot create another AP!}");
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }

        if ((pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num) >= WLAN_SERVICE_AP_MAX_NUM_PER_DEVICE)
        {
            /* 已创建的AP个数达到最大值4 */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::ap num[%d] exceeds the supported spec.}", (pst_mac_device->uc_vap_num - pst_mac_device->uc_sta_num));
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_param->en_vap_mode)
    {
        if (mac_dev_xsta_num(pst_mac_device) >= WLAN_MAX_PROXY_STA_NUM)
        {
            /* 已创建的ProxySTA个数达到最大值 */
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::sta num[%d] exceeds the supported spec.", pst_mac_device->uc_sta_num);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }

        if (mac_param_is_msta(pst_param))
        {
            if ((pst_mac_device->uc_sta_num - mac_dev_xsta_num(pst_mac_device)) >= WLAN_SERVICE_STA_MAX_NUM_PER_DEVICE)
            {
                /* 已创建的STA(非ProxySTA)个数达到最大值 */
                OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_check_vap_num::sta num[%d] exceeds the supported spec.", pst_mac_device->uc_sta_num);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
        }
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_SINGLE_PROXYSTA

OAL_STATIC OAL_INLINE  oal_uint32 hmac_config_single_proxysta_check_vap_num(mac_device_stru          *pst_mac_device,
                                                                     mac_cfg_add_vap_param_stru *pst_param)
{
    // 如果是创建repeater的 ProxySTA 则只能创建一个STA  AP数量不限制
    if (pst_mac_device->uc_sta_num == WLAN_SINGLE_PROXY_STA_NUM_PER_DEVICE)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_single_proxysta_check_vap_num::repeater proxysta num exceeds the supported spec[%d].",
            WLAN_SINGLE_PROXY_STA_NUM_PER_DEVICE);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
    return OAL_SUCC;
}
#endif


OAL_STATIC OAL_INLINE oal_uint32  hmac_config_normal_check_vap_num(mac_device_stru *pst_mac_device, mac_cfg_add_vap_param_stru *pst_param)
{
    wlan_vap_mode_enum_uint8   en_vap_mode;
#ifdef _PRE_WLAN_FEATURE_P2P
    wlan_p2p_mode_enum_uint8   en_p2p_mode;
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_is_proxysta_enabled(pst_mac_device))
    {
        return hmac_config_proxysta_check_vap_num(pst_mac_device, pst_param);
    }
#endif
#ifdef _PRE_WLAN_FEATURE_SINGLE_PROXYSTA
    /* proxysta STA模式 进行创建数量校验 其他模式 直接采用原来的校验方式 */
    if(PROXYSTA_MODE_SSTA == pst_param->en_proxysta_mode)
    {
        return hmac_config_single_proxysta_check_vap_num(pst_mac_device, pst_param);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    en_p2p_mode = pst_param->en_p2p_mode;
    if (WLAN_LEGACY_VAP_MODE != en_p2p_mode)
    {
        return hmac_check_p2p_vap_num_etc(pst_mac_device, en_p2p_mode);
    }
#endif

    en_vap_mode = pst_param->en_vap_mode;
    return hmac_normal_check_legacy_vap_num(pst_mac_device, en_vap_mode);
}


OAL_STATIC oal_uint32  hmac_config_check_vap_num(mac_device_stru *pst_mac_device, mac_cfg_add_vap_param_stru *pst_param)
{
    return hmac_config_normal_check_vap_num(pst_mac_device, pst_param);
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_cfg_vap_send_event_etc(mac_device_stru *pst_device)
{
    frw_event_mem_stru   *pst_event_mem;
    frw_event_stru       *pst_event;
    oal_uint32            ul_ret;

    /* 抛事件给DMAC,让DMAC完成配置VAP创建 */
    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_cfg_vap_send_event_etc::pst_event_mem null.}");
        return OAL_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    /* 填写事件头 */
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                    FRW_EVENT_TYPE_HOST_CRX,
                    HMAC_TO_DMAC_SYN_CREATE_CFG_VAP,
                    0,
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_device->uc_chip_id,
                    pst_device->uc_device_id,
                    pst_device->uc_cfg_vap_id);

    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_cfg_vap_send_event_etc::frw_event_dispatch_event_etc failed[%d].}", ul_ret);

    }

    /* 释放事件 */
    FRW_EVENT_FREE(pst_event_mem);

    return ul_ret;

}
#endif


oal_uint32  hmac_config_add_vap_etc(mac_vap_stru *pst_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_add_vap_param_stru    *pst_param;
    hmac_vap_stru                 *pst_hmac_vap;
    oal_uint32                     ul_ret;
    mac_device_stru               *pst_dev;
    oal_uint8                      uc_vap_id;
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    mac_vap_stru                  *pst_msta;
#endif
#if (defined _PRE_WLAN_WEB_CMD_COMM) && (!defined WIN32)
    mac_cfg_staion_id_param_stru  st_sta_id_param;
    oal_uint16                    us_mac_len;
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_add_vap_etc::param null,pst_vap=%d puc_param=%d.}", pst_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_param      = (mac_cfg_add_vap_param_stru *)puc_param;

#ifdef _PRE_WLAN_FEATURE_DBDC
    pst_param->uc_dst_hal_dev_id    = 0;   //默认创建在主路
#endif
    pst_dev = mac_res_get_dev_etc(pst_vap->uc_device_id);

#ifdef _PRE_WLAN_FEATURE_P2P
    if (WLAN_P2P_CL_MODE == pst_param->en_p2p_mode)
    {
        return hmac_add_p2p_cl_vap_etc(pst_vap, us_len, puc_param);
    }
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dev))
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap_etc::pst_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* VAP个数判断 */
    ul_ret = hmac_config_check_vap_num(pst_dev, pst_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap_etc::hmac_config_check_vap_num failed[%d].}", ul_ret);
        return ul_ret;
    }

    if(OAL_PTR_NULL != OAL_NET_DEV_PRIV(pst_param->pst_net_dev))
    {
        OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap_etc::vap created.}");
        return OAL_SUCC;
    }

    /* 从资源池申请hmac vap */
    /*lint -e413*/
    ul_ret = mac_res_alloc_hmac_vap(&uc_vap_id, OAL_OFFSET_OF(hmac_vap_stru, st_vap_base_info));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap_etc::mac_res_alloc_hmac_vap failed[%d].}", ul_ret);
        return ul_ret;
    }
    /*lint +e413*/

    /* 从资源池获取新申请到的hmac vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_param->uc_vap_id = uc_vap_id;

    /* 初始清0 */
    OAL_MEMZERO(pst_hmac_vap, OAL_SIZEOF(hmac_vap_stru));

    /* 初始化HMAC VAP */
    ul_ret = hmac_vap_init_etc(pst_hmac_vap, pst_dev->uc_chip_id, pst_dev->uc_device_id, uc_vap_id, pst_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap_etc::hmac_vap_init_etc failed[%d].}", ul_ret);
        if (OAL_PTR_NULL != pst_hmac_vap->st_vap_base_info.pst_mib_info)
        {
            OAL_MEM_FREE(pst_hmac_vap->st_vap_base_info.pst_mib_info, OAL_TRUE);
        }
    #ifdef _PRE_WLAN_FEATURE_VOWIFI
        if (WLAN_LEGACY_VAP_MODE == pst_hmac_vap->st_vap_base_info.en_p2p_mode)
        {
            mac_vap_vowifi_exit(&(pst_hmac_vap->st_vap_base_info));
        }
    #endif
        /* 异常处理，释放内存 */
        mac_res_free_mac_vap_etc(uc_vap_id);
        return ul_ret;
    }
#ifdef _PRE_WLAN_1103_CHR
    /* 记录TxBASessionNumber mib值到chr全局变量中 */
    if (IS_LEGACY_STA(&pst_hmac_vap->st_vap_base_info))
    {
        hmac_chr_set_ba_session_num(mac_mib_get_TxBASessionNumber(&pst_hmac_vap->st_vap_base_info));
    }
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK
    mac_hilink_init_vap(&(pst_hmac_vap->st_vap_base_info));
#endif
    /* 设置反挂的net_device指针 */
#ifdef _PRE_WLAN_FEATURE_P2P
    if (WLAN_P2P_DEV_MODE == pst_param->en_p2p_mode)
    {
        /* p2p0 DEV 模式vap，采用pst_p2p0_net_device 成员指向对应的net_device */
        pst_hmac_vap->pst_p2p0_net_device = pst_param->pst_net_dev;
        pst_dev->st_p2p_info.uc_p2p0_vap_idx = pst_hmac_vap->st_vap_base_info.uc_vap_id;
    }
#endif
    pst_hmac_vap->pst_net_device = pst_param->pst_net_dev;

    /* 包括'\0' */
    oal_memcopy(pst_hmac_vap->auc_name, pst_param->pst_net_dev->name,OAL_IF_NAME_SIZE);

    /* 将申请到的mac_vap空间挂到net_device ml_priv指针上去 */
    OAL_NET_DEV_PRIV(pst_param->pst_net_dev) = &pst_hmac_vap->st_vap_base_info;

    oal_memset(pst_hmac_vap->auc_dscp_tid_map,HMAC_DSCP_VALUE_INVALID,HMAC_MAX_DSCP_VALUE_NUM);
#ifdef _PRE_WLAN_FEATURE_SINGLE_PROXYSTA
    hmac_proxysta_init_vap(pst_hmac_vap, pst_param);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    hmac_psta_init_vap(pst_hmac_vap, pst_param);
#endif
    /* 申请hmac组播用户 */
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (mac_vap_is_vsta(&pst_hmac_vap->st_vap_base_info))
    {
        /* vsta不创建组播用户，组播用户id配置成和msta一致 */
        pst_msta = mac_find_main_proxysta(pst_dev);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_msta))
        {
            /* 目前proxysta方案msta要先创建，msta未创建，先创建vsta不合理，后续不好管理vsta不创建组播用户 */
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA, "{hmac_config_add_vap_etc::msta is null, vsta cannot create.}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        else
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PROXYSTA, "{hmac_config_add_vap_etc::vsta multi user id is[%d], the same with msta.}", pst_msta->us_multi_user_idx);
            mac_vap_set_multi_user_idx_etc(&(pst_hmac_vap->st_vap_base_info), pst_msta->us_multi_user_idx);
        }
    }
    else
#endif
    {
        hmac_user_add_multi_user_etc(&(pst_hmac_vap->st_vap_base_info), &pst_param->us_muti_user_id);
        mac_vap_set_multi_user_idx_etc(&(pst_hmac_vap->st_vap_base_info), pst_param->us_muti_user_id);
    }

    mac_device_set_vap_id_etc(pst_dev, &(pst_hmac_vap->st_vap_base_info),uc_vap_id, pst_param->en_vap_mode, pst_param->en_p2p_mode, OAL_TRUE);

#if (defined _PRE_WLAN_WEB_CMD_COMM) && (!defined WIN32)
    if (WLAN_VAP_MODE_CONFIG != pst_param->en_vap_mode)
    {
        mac_res_get_mac_addr(st_sta_id_param.auc_station_id);
#ifdef _PRE_WLAN_FEATURE_P2P
        st_sta_id_param.en_p2p_mode = pst_param->en_p2p_mode;
#endif
        us_mac_len = OAL_SIZEOF(mac_cfg_staion_id_param_stru);
        hmac_config_set_mac_addr_etc(&(pst_hmac_vap->st_vap_base_info), us_mac_len, (oal_uint8*)&st_sta_id_param);
    }
#endif

    switch(pst_param->en_vap_mode)
    {
        case WLAN_VAP_MODE_BSS_AP:
        #ifdef _PRE_WLAN_FEATURE_UAPSD
            #if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
                mac_vap_set_uapsd_en_etc(&pst_hmac_vap->st_vap_base_info, OAL_TRUE);
            #endif
                pst_param->bit_uapsd_enable = pst_hmac_vap->st_vap_base_info.st_cap_flag.bit_uapsd;
        #endif
            break;

        case WLAN_VAP_MODE_BSS_STA:
            break;

        case WLAN_VAP_MODE_WDS:

            break;

        default:
            return OAL_ERR_CODE_INVALID_CONFIG;
    }


    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(&pst_hmac_vap->st_vap_base_info,
                                    WLAN_CFGID_ADD_VAP,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        /*此处回退有误，需要对应mac_device_set_vap_id，做回退操作*/
        mac_device_set_vap_id_etc(pst_dev, &(pst_hmac_vap->st_vap_base_info),uc_vap_id, pst_param->en_vap_mode, pst_param->en_p2p_mode, OAL_FALSE);
        hmac_user_del_multi_user_etc(&(pst_hmac_vap->st_vap_base_info));
    #ifdef _PRE_WLAN_FEATURE_VOWIFI
        if (WLAN_LEGACY_VAP_MODE == pst_hmac_vap->st_vap_base_info.en_p2p_mode)
        {
            mac_vap_vowifi_exit(&(pst_hmac_vap->st_vap_base_info));
        }
    #endif

        /* 异常处理，释放内存 */
        OAL_MEM_FREE(pst_hmac_vap->st_vap_base_info.pst_mib_info, OAL_TRUE);

        mac_res_free_mac_vap_etc(uc_vap_id);
        OAL_NET_DEV_PRIV(pst_param->pst_net_dev) = OAL_PTR_NULL;

        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_add_vap_etc::hmac_config_alloc_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    if (IS_P2P_GO(&pst_hmac_vap->st_vap_base_info))
    {
        hmac_config_set_max_user_etc(&pst_hmac_vap->st_vap_base_info, 0, WLAN_P2P_GO_ASSOC_USER_MAX_NUM_SPEC);
    }

    OAM_WARNING_LOG4(uc_vap_id, OAM_SF_ANY, "{hmac_config_add_vap_etc::SUCCESS!!vap_mode[%d], p2p_mode[%d]}, multi user idx[%d], device id[%d]",
                    pst_param->en_vap_mode, pst_param->en_p2p_mode, pst_vap->us_multi_user_idx, pst_hmac_vap->st_vap_base_info.uc_device_id);

#ifdef _PRE_WLAN_REPORT_PRODUCT_LOG
    //保存vap id 对应的 chip id
    g_auc_vapid_to_chipid[pst_hmac_vap->st_vap_base_info.uc_vap_id] = pst_hmac_vap->st_vap_base_info.uc_chip_id;
#endif

    return OAL_SUCC;
}

oal_uint32 hmac_get_chip_vap_num(mac_chip_stru  *pst_chip)
{
    mac_device_stru        *pst_mac_device;
    oal_uint8               uc_device;
    oal_uint8               uc_vap_num = 0;

    for (uc_device = 0; uc_device < pst_chip->uc_device_nums; uc_device++)
    {
        pst_mac_device = mac_res_get_dev_etc(pst_chip->auc_device_id[uc_device]);
        if (OAL_PTR_NULL == pst_mac_device)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "hmac_get_chip_vap_num::mac_res_get_dev_etc id[%d] NULL",pst_chip->auc_device_id[uc_device]);
            continue;
        }

        uc_vap_num += pst_mac_device->uc_vap_num;
    }
    return uc_vap_num;
}


oal_uint32  hmac_config_del_vap_etc(mac_vap_stru *pst_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                 *pst_hmac_vap;
    oal_uint32                     ul_ret;
    mac_device_stru               *pst_device;
    mac_cfg_del_vap_param_stru    *pst_del_vap_param;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mac_chip_stru                 *pst_chip;
    oal_uint8                      uc_vap_num;
    hmac_device_stru              *pst_hmac_device;
#endif
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_del_vap_etc::param null,pst_vap=%d puc_param=%d.}", pst_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_del_vap_param = (mac_cfg_del_vap_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_vap_etc::mac_res_get_hmac_vap failed.}");
        return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    hmac_psta_del_vap(pst_hmac_vap); // just in case
    if ((mac_vap_is_msta(&pst_hmac_vap->st_vap_base_info)) || (mac_vap_is_msta(&pst_hmac_vap->st_vap_base_info)))
    {
        /* 置proxysta的oma地址为0，后续创建vap时候再重新配置oma */
        oal_set_mac_addr_zero(hmac_vap_psta_oma(pst_hmac_vap));
    }
#endif

#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
    /* 删除相应的WDS定时器 */
    if (OAL_TRUE == pst_hmac_vap->st_wds_table.st_wds_timer.en_is_registerd)
    {
         FRW_TIMER_DESTROY_TIMER(&(pst_hmac_vap->st_wds_table.st_wds_timer));
    }
    /* vap销毁前清除所有wds信息 */
    hmac_wds_reset_sta_mapping_table(pst_hmac_vap);
    hmac_wds_reset_neigh_table(pst_hmac_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    if (WLAN_P2P_CL_MODE == pst_vap->en_p2p_mode)
    {
        return hmac_del_p2p_cl_vap_etc(pst_vap, us_len, puc_param);
    }
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
    /*can't return when dfr process!*/
    if((MAC_VAP_STATE_INIT != pst_vap->en_vap_state) && (OAL_TRUE != g_st_dfr_info_etc.bit_device_reset_process_flag))
#else
    if(MAC_VAP_STATE_INIT != pst_vap->en_vap_state)
#endif
    {
        OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_vap_etc::deleting vap failed. vap state is not INIT, en_vap_state=%d,en_vap_mode=%d}",
                         pst_vap->en_vap_state, pst_vap->en_vap_mode);
        oam_report_backtrace();
        return OAL_FAIL;
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

    /* 如果是配置VAP, 去注册配置vap对应的net_device, 释放，返回 */
    if (WLAN_VAP_MODE_CONFIG == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        /*在注销netdevice之前先将指针赋为空*/
        oal_net_device_stru   *pst_net_device = pst_hmac_vap->pst_net_device;
        pst_hmac_vap->pst_net_device = OAL_PTR_NULL;
        OAL_SMP_MB();
        oal_net_unregister_netdev(pst_net_device);

        mac_res_free_mac_vap_etc(pst_hmac_vap->st_vap_base_info.uc_vap_id);
        return  OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    hmac_11k_exit_vap(pst_hmac_vap);
#endif

#if  (defined _PRE_WLAN_WEB_CMD_COMM) && (!defined WIN32)
    if (WLAN_VAP_MODE_CONFIG != pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        mac_res_clear_mac_bitmap(mac_mib_get_StationID(&(pst_hmac_vap->st_vap_base_info)));
    }
#endif

    /* 业务vap net_device已在WAL释放，此处置为null */
#ifdef _PRE_WLAN_FEATURE_P2P
    if (WLAN_P2P_DEV_MODE == pst_del_vap_param->en_p2p_mode)
    {
        /* 针对p2p0,需要删除hmac 中对应的p2p0 netdevice 指针 */
        pst_hmac_vap->pst_p2p0_net_device = OAL_PTR_NULL;
    }
#endif
    pst_hmac_vap->pst_net_device = OAL_PTR_NULL;

    /* 组播转单播的detach */

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
    hmac_m2u_detach(pst_hmac_vap);
#endif

    /* 业务vap已删除，从device上去掉 */
    pst_device     = mac_res_get_dev_etc(pst_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_vap_etc::mac_res_get_dev_etc failed.}");
        return OAL_FAIL;
    }


    /*清理所有的timer*/
    if (OAL_TRUE == pst_hmac_vap->st_mgmt_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_mgmt_timer));
    }
    if (OAL_TRUE == pst_hmac_vap->st_scan_timeout.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_scan_timeout));
    }
#ifdef _PRE_WLAN_FEATURE_STA_PM
    if(OAL_TRUE == pst_hmac_vap->st_ps_sw_timer.en_is_registerd)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_vap->st_ps_sw_timer));
    }
#endif
    /*删除vap时删除TCP ACK的队列*/
#ifdef _PRE_WLAN_TCP_OPT
    hmac_tcp_opt_deinit_list_etc(pst_hmac_vap);
#endif
#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    if (!mac_vap_is_vsta(&pst_hmac_vap->st_vap_base_info))
#endif
    {
        hmac_user_del_multi_user_etc(pst_vap);
    }

    /* 释放pmksa */
    hmac_config_flush_pmksa_etc(pst_vap, us_len, puc_param);

    mac_vap_exit_etc(&(pst_hmac_vap->st_vap_base_info));

    /* TBD 保持原有行为不变，待做变量分拆。拆分为Hmac的变量*/
#ifdef _PRE_WLAN_FEATURE_P2P
    if (0 == pst_device->uc_vap_num)
    {
        #if (!defined(_PRE_PRODUCT_ID_HI110X_HOST))
        /* 1102 wlan0网络设备一直存在 */
        pst_device->st_p2p_info.pst_primary_net_device = OAL_PTR_NULL;
        #endif
    }
#endif

#ifdef _PRE_WLAN_FEATURE_SINGLE_PROXYSTA
    hmac_proxysta_exit_vap(pst_hmac_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXY_ARP
    hmac_proxy_exit(pst_vap);
#endif

    mac_res_free_mac_vap_etc(pst_hmac_vap->st_vap_base_info.uc_vap_id);

    /***************************************************************************
                          抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_vap,
                                    WLAN_CFGID_DEL_VAP,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_vap_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        //不退出，保证Devce挂掉的情况下可以下电。
    }

    OAM_WARNING_LOG4(pst_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_del_vap_etc::Del succ.vap_mode[%d], p2p_mode[%d], multi user idx[%d], device_id[%d]}",
                pst_vap->en_vap_mode, pst_del_vap_param->en_p2p_mode, pst_vap->us_multi_user_idx, pst_vap->uc_device_id);


#ifdef _PRE_WLAN_FEATURE_DFR
    if (g_st_dfr_info_etc.bit_device_reset_process_flag)
    {
        //g_st_dfr_info_etc.bit_ready_to_recovery_flag = (!pst_device->uc_vap_num) ? OAL_TRUE : OAL_FALSE;  //在wal_dfx.c文件中标记开始恢复
        return OAL_SUCC;
    }
#endif //_PRE_WLAN_FEATURE_DFR

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    //如果WIFI sta，wlan0 stop，下电
    pst_chip = hmac_res_get_mac_chip(pst_device->uc_chip_id);
    if (OAL_PTR_NULL == pst_chip)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "hmac_config_del_vap_etc::hmac_res_get_mac_chip id[%d] NULL",pst_device->uc_chip_id);
        return OAL_FAIL;
    }

    uc_vap_num = hmac_get_chip_vap_num(pst_chip);

    /*chip下的所有device的业务vap个数为0,才能给device下电 */
    if ((WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode) && (0 == uc_vap_num))
    {
        /* APUT启动时不关闭device */
        if (OAL_ERR_CODE_FOBID_CLOSE_DEVICE != wlan_pm_close_etc())
        {
            pst_hmac_device = hmac_res_get_mac_dev_etc(pst_vap->uc_device_id);
            if (OAL_LIKELY(OAL_PTR_NULL != pst_hmac_device))
            {
               hmac_scan_clean_scan(&(pst_hmac_device->st_scan_mgmt));
            }
            else
            {
               OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_del_vap_etc::pst_hmac_device[%d] null!}", pst_vap->uc_device_id);
            }

            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_config_del_vap_etc::hmac_config_host_dev_exit_etc! pst_hmac_device[%d]}", pst_vap->uc_device_id);
            hmac_config_host_dev_exit_etc(pst_vap);
        }
    }
#endif

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_config_def_chan_etc(mac_vap_stru *pst_mac_vap)
{
    oal_uint8                     uc_channel ;
    mac_cfg_mode_param_stru       st_param;

    if (((WLAN_BAND_BUTT == pst_mac_vap->st_channel.en_band) ||
        (WLAN_BAND_WIDTH_BUTT == pst_mac_vap->st_channel.en_bandwidth) ||
        (WLAN_PROTOCOL_BUTT == pst_mac_vap->en_protocol))
        && (!IS_P2P_GO(pst_mac_vap)))
    {
        st_param.en_band = WLAN_BAND_2G;
        st_param.en_bandwidth = WLAN_BAND_WIDTH_20M;
        st_param.en_protocol = WLAN_HT_MODE;
        hmac_config_set_mode_etc(pst_mac_vap, OAL_SIZEOF(st_param), (oal_uint8*)&st_param);
    }

    if ((0 == pst_mac_vap->st_channel.uc_chan_number) && (!IS_P2P_GO(pst_mac_vap)))
    {
        pst_mac_vap->st_channel.uc_chan_number = 6;
        uc_channel = pst_mac_vap->st_channel.uc_chan_number;
        hmac_config_set_freq_etc(pst_mac_vap, OAL_SIZEOF(oal_uint32), &uc_channel);
    }

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_start_vap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                    *puc_ssid;
    oal_uint32                    ul_ret;
    mac_device_stru              *pst_mac_device;
    hmac_vap_stru                *pst_hmac_vap;
    mac_cfg_start_vap_param_stru *pst_start_vap_param = (mac_cfg_start_vap_param_stru *)puc_param;
#if defined(_PRE_WLAN_FEATURE_DFS) && (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    hmac_device_stru             *pst_hmac_device;
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_start_vap_etc::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_DAQ
    if (OAL_TRUE == g_uc_data_acq_used)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::DAQ has been enabled. Please reset the board.}");
        return OAL_FAIL;
    }
#endif
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (MAC_VAP_STATE_BUTT == pst_mac_vap->en_vap_state)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::the vap has been deleted.}");

        return OAL_FAIL;
    }

    if ((MAC_VAP_STATE_UP            == pst_mac_vap->en_vap_state) ||
        (MAC_VAP_STATE_AP_WAIT_START == pst_mac_vap->en_vap_state) ||
        (MAC_VAP_STATE_STA_FAKE_UP   == pst_mac_vap->en_vap_state))   /* 如果已经在up状态，则返回成功 */
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::state=%d, duplicate start again}", pst_mac_vap->en_vap_state);
        return OAL_SUCC;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    hmac_psta_add_vap(pst_hmac_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    OAL_INIT_DELAYED_WORK(&pst_hmac_vap->st_ampdu_work, (oal_void*)hmac_set_ampdu_worker);
    oal_spin_lock_init(&pst_hmac_vap->st_ampdu_lock);
    OAL_INIT_DELAYED_WORK(&pst_hmac_vap->st_set_hw_work, (oal_void*)hmac_set_ampdu_hw_worker);
#endif

    if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
        puc_ssid = mac_mib_get_DesiredSSID(pst_mac_vap);
        /* P2P GO 创建后，未设置ssid 信息，设置为up 状态不需要检查ssid 参数 */
        if (0 == OAL_STRLEN((oal_int8 *)puc_ssid) && (!IS_P2P_GO(pst_mac_vap)))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::ssid length=0.}");
            return OAL_FAIL;        /* 没设置SSID，则不启动VAP */
        }

        /* 设置AP侧状态机为 WAIT_START */
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_AP_WAIT_START);


        if (IS_LEGACY_VAP(&(pst_hmac_vap->st_vap_base_info)))
        {

    #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
            hmac_config_def_chan_etc(pst_mac_vap);
    #else
        #if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
            if (hmac_calc_up_ap_num_etc(pst_mac_device) < 2)
            {
                hmac_acs_cfg_stru st_acs_cfg;
                st_acs_cfg.uc_acs_type = HMAC_ACS_TYPE_INIT;
                st_acs_cfg.en_switch_chan = OAL_TRUE;
                st_acs_cfg.en_scan_op = MAC_SCAN_OP_INIT_SCAN;
                if(OAL_SUCC == hmac_init_scan_try_etc(pst_mac_device, pst_mac_vap, MAC_TRY_INIT_SCAN_VAP_UP, &st_acs_cfg))
                {
                    return OAL_SUCC;
                }
            }
        #endif
    #endif
        }

        /* 这里 en_status 等于 MAC_CHNL_AV_CHK_NOT_REQ(无需检测) 或者 MAC_CHNL_AV_CHK_COMPLETE(检测完成) */

        /* 检查协议 频段 带宽是否设置 */
        if (((WLAN_BAND_BUTT == pst_mac_vap->st_channel.en_band) ||
            (WLAN_BAND_WIDTH_BUTT == pst_mac_vap->st_channel.en_bandwidth) ||
            (WLAN_PROTOCOL_BUTT == pst_mac_vap->en_protocol)))
        {
            if (IS_P2P_GO(pst_mac_vap))
            {
                /* wpa_supplicant 会先设置vap up， 此时并未给vap 配置信道、带宽和协议模式信息，
                   wpa_supplicant 在cfg80211_start_ap 接口配置GO 信道、带宽和协议模式信息，
                   故此处如果没有设置信道、带宽和协议模式，直接返回成功，不返回失败。 */
                hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_INIT);
                OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::set band bandwidth protocol first.band[%d], bw[%d], protocol[%d]}",
                                pst_mac_vap->st_channel.en_band,
                                pst_mac_vap->st_channel.en_bandwidth,
                                pst_mac_vap->en_protocol);
                return OAL_SUCC;
            }
            else
            {
                hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_INIT);
                OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::set band bandwidth protocol first.}");
                return OAL_FAIL;
            }
        }

        /* 检查信道号是否设置 */
        if ((0 == pst_mac_vap->st_channel.uc_chan_number) && (!IS_P2P_GO(pst_mac_vap)))
        {
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_INIT);
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::set channel number first.}");
            return OAL_FAIL;
        }

        /* 设置bssid */
        mac_vap_set_bssid_etc(pst_mac_vap,  mac_mib_get_StationID(pst_mac_vap));

        /* 入网优化，不同频段下的能力不一样 */
        if (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
        {
//            mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, WLAN_LEGACY_11B_MIB_SHORT_PREAMBLE);
            mac_mib_set_SpectrumManagementRequired(pst_mac_vap, OAL_FALSE);
        }
        else
        {
//            mac_mib_set_ShortPreambleOptionImplemented(pst_mac_vap, WLAN_LEGACY_11B_MIB_LONG_PREAMBLE);
            mac_mib_set_SpectrumManagementRequired(pst_mac_vap, OAL_TRUE);
        }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* 设置AP侧状态机为 UP */
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_UP);
#else
    #if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
        
        if (OAL_FALSE == hmac_device_in_init_scan_etc(pst_mac_device))
    #endif
        {
            /* 设置AP侧状态机为 UP */
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_UP);
        }
#endif
    }
    else if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
#ifdef _PRE_WLAN_FEATURE_P2P
#ifndef WIN32
        /* 启动p2p device时，vap_param的p2p模式和mac_vap的p2p模式不同 */
        if(WLAN_P2P_DEV_MODE == pst_mac_vap->en_p2p_mode)
        {
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
        }
        /* p2p0和p2p-p2p0 共VAP 结构，对于p2p cl不用修改vap 状态 */
        else
#endif
            // tscancode-suppress *
            if (WLAN_P2P_CL_MODE != pst_start_vap_param->en_p2p_mode ||
                (WLAN_P2P_CL_MODE == pst_start_vap_param->en_p2p_mode && MAC_VAP_STATE_INIT == pst_mac_vap->en_vap_state))
#endif
            {
                hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
            }
    }
    else
    {
        /* TBD 其它分支 暂不支持 待开发 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::Do not surport other mode[%d].}", pst_mac_vap->en_vap_mode);
    }

    mac_vap_init_rates_etc(pst_mac_vap);
    ul_ret = hmac_config_start_vap_event_etc(pst_mac_vap, pst_start_vap_param->en_mgmt_rate_init_flag);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

#if 0
#ifdef _PRE_WLAN_FEATURE_DFR
    /* 异常恢复后需要告诉对端"已去关联"的消息 */
    if (OAL_TRUE == g_st_dfr_info_etc.bit_user_disconnect_flag)
    {
        g_st_dfr_info_etc.bit_user_disconnect_flag = OAL_FALSE;
        hmac_mgmt_send_disassoc_frame_etc(pst_mac_vap, BROADCAST_MACADDR, MAC_UNSPEC_REASON, OAL_FALSE);
    }
#endif //_PRE_WLAN_FEATURE_DFR
#endif

#if defined(_PRE_WLAN_FEATURE_DFS) && (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    if(IS_LEGACY_AP(pst_mac_vap))
    {
        pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_device->uc_device_id);
        hmac_dfs_try_cac_etc(pst_hmac_device, pst_mac_vap);
    }
#endif

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::host start vap ok. now post event to dmac. vap mode[%d],p2p mode[%d]bw[%d]}",
                     pst_mac_vap->en_vap_mode, pst_mac_vap->en_p2p_mode, MAC_VAP_GET_CAP_BW(pst_mac_vap));
    return OAL_SUCC;
}


oal_uint32  hmac_config_sta_update_rates_etc(mac_vap_stru *pst_mac_vap, mac_cfg_mode_param_stru *pst_cfg_mode, mac_bss_dscr_stru *pst_bss_dscr)
{
    oal_uint32                    ul_ret;
    mac_device_stru              *pst_mac_device;
    hmac_vap_stru                *pst_hmac_vap;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_update_rates_etc::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    if (MAC_VAP_STATE_BUTT == pst_mac_vap->en_vap_state)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_update_rates_etc::the vap has been deleted.}");

        return OAL_FAIL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_update_rates_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_cfg_mode->en_protocol >= WLAN_HT_MODE)
    {
        mac_mib_set_TxAggregateActived(&pst_hmac_vap->st_vap_base_info, OAL_TRUE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_TRUE);
    }
    else
    {
        mac_mib_set_TxAggregateActived(&pst_hmac_vap->st_vap_base_info, OAL_FALSE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_FALSE);
    }

    mac_vap_init_by_protocol_etc(pst_mac_vap, pst_cfg_mode->en_protocol);
    pst_mac_vap->st_channel.en_band = pst_cfg_mode->en_band;
    pst_mac_vap->st_channel.en_bandwidth = pst_cfg_mode->en_bandwidth;
#ifdef _PRE_WIFI_DMT
    mac_vap_init_rates_etc(pst_mac_vap);
#else
    mac_sta_init_bss_rates_etc(pst_mac_vap, (oal_void *)pst_bss_dscr);
#endif


    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_start_vap_event_etc(pst_mac_vap, OAL_FALSE);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_update_rates_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        mac_vap_init_by_protocol_etc(pst_mac_vap, pst_hmac_vap->st_preset_para.en_protocol);
        pst_mac_vap->st_channel.en_band      = pst_hmac_vap->st_preset_para.en_band;
        pst_mac_vap->st_channel.en_bandwidth = pst_hmac_vap->st_preset_para.en_bandwidth;
        return ul_ret;
    }

    return OAL_SUCC;
}


#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)

oal_uint8 hmac_calc_up_and_wait_vap_etc(hmac_device_stru *pst_hmac_dev)
{
    mac_vap_stru                  *pst_vap;
    oal_uint8                      uc_vap_idx;
    oal_uint8                      ul_up_ap_num = 0;
    mac_device_stru               *pst_mac_device;

    if (pst_hmac_dev->pst_device_base_info == OAL_PTR_NULL)
    {
        return 0;
    }

    pst_mac_device = pst_hmac_dev->pst_device_base_info;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "vap is null, vap id is %d", pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (MAC_VAP_STATE_UP == pst_vap->en_vap_state || MAC_VAP_STATE_AP_WAIT_START == pst_vap->en_vap_state)
        {
            ul_up_ap_num++;
        }
    }

    return ul_up_ap_num;
}
#endif

oal_uint32 hmac_config_down_vap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru                *pst_mac_device;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_cfg_down_vap_param_stru    *pst_param;
    oal_uint32                      ul_ret;
    oal_dlist_head_stru            *pst_entry;
    oal_dlist_head_stru             *pst_dlist_tmp;
    mac_user_stru                  *pst_user_tmp;
    hmac_user_stru                 *pst_hmac_user_tmp;
    oal_bool_enum_uint8             en_is_protected = OAL_FALSE;
    mac_user_stru                  *pst_multi_user = OAL_PTR_NULL;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_down_vap_etc::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }


    pst_param      = (mac_cfg_down_vap_param_stru *)puc_param;

    if (OAL_PTR_NULL == pst_param->pst_net_dev)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::pst_param->pst_net_dev is null.}");
        return OAL_SUCC;
    }

    /* 如果vap已经在down的状态，直接返回 */
    if (MAC_VAP_STATE_INIT == pst_mac_vap->en_vap_state)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::vap already down.}");
        return OAL_SUCC;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_multi_user = mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
    if (OAL_PTR_NULL == pst_multi_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::multi_user[%d] null.}",
            pst_mac_vap->us_multi_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    oal_cancel_delayed_work_sync(&pst_hmac_vap->st_ampdu_work);
    oal_cancel_delayed_work_sync(&pst_hmac_vap->st_set_hw_work);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
    hmac_psta_del_vap(pst_hmac_vap);
#endif

#if defined (_PRE_WLAN_FEATURE_WDS) || defined (_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA)
    /* vap down前清除所有wds信息 */
    hmac_wds_reset_sta_mapping_table(pst_hmac_vap);
    hmac_wds_reset_neigh_table(pst_hmac_vap);
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    hmac_11k_exit_vap(pst_hmac_vap);
#endif

    /* 考虑与数据面互斥，需要加锁保护 */
    //oal_spin_lock(&pst_mac_vap->st_lock_state);

    /* 遍历vap下所有用户, 删除用户 */
    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_user_tmp      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        if (OAL_PTR_NULL == pst_user_tmp)
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::pst_user_tmp null.}");
            continue;
        }

        pst_hmac_user_tmp = mac_res_get_hmac_user_etc(pst_user_tmp->us_assoc_id);
        if (OAL_PTR_NULL == pst_hmac_user_tmp)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::pst_hmac_user_tmp null.idx:%u}",pst_user_tmp->us_assoc_id);
            continue;
        }

        /* 管理帧加密是否开启*/
        en_is_protected = pst_user_tmp->st_cap_info.bit_pmf_active;

        /* 发去关联帧 */
        hmac_mgmt_send_disassoc_frame_etc(pst_mac_vap, pst_user_tmp->auc_user_mac_addr, MAC_DISAS_LV_SS, en_is_protected);
        /* 删除用户 */
        hmac_user_del_etc(pst_mac_vap, pst_hmac_user_tmp);
    }

    /* VAP下user链表应该为空 */
    if (OAL_FALSE == oal_dlist_is_empty(&pst_mac_vap->st_mac_user_list_head))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::st_mac_user_list_head is not empty.}");
        return OAL_FAIL;
    }

    /* sta模式时 将desired ssid MIB项置空，并清空配置协议标志 */
    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        pst_hmac_vap->bit_sta_protocol_cfg = OAL_SWITCH_OFF;
        if(OAL_PTR_NULL != pst_mac_vap->pst_mib_info)
        {
            OAL_MEMZERO(mac_mib_get_DesiredSSID(pst_mac_vap), WLAN_SSID_MAX_LEN);
        }
        else
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::mib pointer is NULL!!}");
        }
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::sta protocol cfg clear}");
    }
    else if (WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
    {
#ifdef _PRE_WLAN_FEATURE_DFS
        /* 取消 CAC 定时器 */
        hmac_dfs_cac_stop_etc(pst_mac_device,pst_mac_vap);
        hmac_dfs_off_cac_stop_etc(pst_mac_device, pst_mac_vap);
#endif
#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
        {
            hmac_device_stru *pst_hmac_dev = hmac_res_get_mac_dev_etc(pst_mac_device->uc_device_id);
            oal_uint32        ul_pedding_data = 0;

            if (pst_hmac_dev)
            {
                if (pst_hmac_dev->en_in_init_scan && pst_hmac_dev->st_scan_mgmt.en_is_scanning)
                {
                    hmac_config_scan_abort_etc(pst_mac_vap, OAL_SIZEOF(oal_uint32), (oal_uint8 *)&ul_pedding_data);
                }

                if (1 >= hmac_calc_up_and_wait_vap_etc(pst_hmac_dev))
                {
                    hmac_init_scan_cancel_timer_etc(pst_hmac_dev);
                    pst_hmac_dev->en_in_init_scan = OAL_FALSE;
                }
            }
        }
#endif
    }

    /***************************************************************************
                         抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap,
                                    WLAN_CFGID_DOWN_VAP,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        //oal_spin_unlock(&pst_mac_vap->st_lock_state);

        return ul_ret;
    }

    /* 110x也同步下去,不host device写两遍和staut接口统一 */
#ifdef _PRE_WLAN_FEATURE_P2P
    if (pst_param->en_p2p_mode == WLAN_P2P_CL_MODE)
    {
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
    }
    else
#endif
    {
        hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_INIT);
    }

    mac_mib_set_AuthenticationMode(pst_mac_vap, WLAN_WITP_AUTH_OPEN_SYSTEM);

#ifdef _PRE_WLAN_FEATURE_DFS
    hmac_dfs_radar_wait_etc(pst_mac_device, pst_hmac_vap);
#endif

    //oal_spin_unlock(&pst_mac_vap->st_lock_state);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc:: SUCC! Now remaining %d vaps in device[%d].}",
                    pst_mac_device->uc_vap_num,
                    pst_mac_device->uc_device_id);
    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_AP_PM
oal_uint32 hmac_config_wifi_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret = OAL_SUCC;
    mac_device_stru*            pst_mac_device;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru               *pst_service_vap;
    hmac_vap_stru              *pst_hmac_vap;
    mac_cfg_down_vap_param_stru   st_down_vap;
    mac_cfg_start_vap_param_stru  st_start_vap;

    l_value = *((oal_int32 *)puc_param);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_wifi_enable::pst_mac_device[%p] null!}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 遍历device下所有vap，先vap up/down，再向PM管理发事件 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_service_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_service_vap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wifi_enable::pst_mac_vap null, vap id=%d.", pst_mac_device->auc_vap_id[uc_vap_idx]);
            return OAL_ERR_CODE_PTR_NULL;
        }
        pst_hmac_vap = mac_res_get_hmac_vap(pst_service_vap->uc_vap_id);
        if(OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wifi_enable::pst_hmac_vap null, vap id=%d.", pst_service_vap->uc_vap_id);
            return OAL_ERR_CODE_PTR_NULL;
        }
        if(l_value == OAL_TRUE)
        { /*enable*/
            /*需在vap up前通知PM唤醒芯片*/
            ul_ret = hmac_config_send_event_etc(pst_service_vap,
                                            WLAN_CFGID_WIFI_EN,
                                            us_len,
                                            puc_param);
            if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
            {
                OAM_WARNING_LOG1(pst_service_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wifi_enable::hmac_config_send_event_etc failed[%d].}", ul_ret);
                return ul_ret;
            }

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
        }
        else
        {   /*disable*/
            st_down_vap.pst_net_dev = pst_hmac_vap->pst_net_device;
            ul_ret = hmac_config_down_vap_etc(&pst_hmac_vap->st_vap_base_info,
                                          OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                                          (oal_uint8 *)&st_down_vap);
            if (ul_ret != OAL_SUCC)
            {
                OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_config_wifi_enable::hmac_config_down_vap_etc failed[%d].}", ul_ret);
                return ul_ret;
            }


            /*vap down后再通知PM睡眠芯片*/
            ul_ret = hmac_config_send_event_etc(pst_service_vap,
                                            WLAN_CFGID_WIFI_EN,
                                            us_len,
                                            puc_param);
            if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
            {
                OAM_WARNING_LOG1(pst_service_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wifi_enable::hmac_config_send_event_etc failed[%d].}", ul_ret);
                return ul_ret;
            }
        }
    }

    /*如果没有业务VAP，发给配置VAP*/
    if(0 == pst_mac_device->uc_vap_num)
    {
        ul_ret = hmac_config_send_event_etc(pst_mac_vap,
                                        WLAN_CFGID_WIFI_EN,
                                        us_len,
                                        puc_param);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wifi_enable::hmac_config_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

    return ul_ret;
}



oal_uint32 hmac_config_sta_scan_wake_wow(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32 ul_ret;

    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_STA_SCAN_CONNECT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR,
                       "{hmac_config_sta_scan_wake_wow::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif


#if 0
oal_uint32 hmac_config_update_mode(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    mac_cfg_down_vap_param_stru   st_down_vap;
    mac_cfg_start_vap_param_stru  st_start_vap_param;
    hmac_vap_stru                 *pst_hmac_vap;
    oal_uint32                    ul_ret;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 先down vap */
    st_down_vap.pst_net_dev = pst_hmac_vap->pst_net_device;
    ul_ret = hmac_config_down_vap_etc(pst_mac_vap,
                                  OAL_SIZEOF(mac_cfg_down_vap_param_stru),
                                  (oal_uint8 *)&st_down_vap);
    if (ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_down_vap_etc::FAILED!}");
        return ul_ret;
    }

    /* 设置协议模式 */
    ul_ret = hmac_config_set_mode_etc(pst_mac_vap,
                                OAL_SIZEOF(mac_cfg_mode_param_stru),
                                puc_param);
    if (ul_ret != OAL_SUCC)
    {
       OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode_etc::FAILED!}");
       return ul_ret;
    }

    /* 启动vap */
    st_start_vap_param.pst_net_dev = pst_hmac_vap->pst_net_device;
    ul_ret = hmac_config_start_vap_etc(pst_mac_vap,
                                   OAL_SIZEOF(mac_cfg_start_vap_param_stru),
                                   (oal_uint8 *)&st_start_vap_param);
    if (ul_ret != OAL_SUCC)
    {
       OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_start_vap_etc::FAILED!}");
       return ul_ret;
    }
    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_set_bss_type_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /* 设置mib值 */
    mac_mib_set_bss_type_etc(pst_mac_vap, (oal_uint8)us_len, puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_HMAC == _PRE_MULTI_CORE_MODE)
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_BSS_TYPE, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}


oal_uint32  hmac_config_get_bss_type_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_bss_type_etc(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_get_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_cfg_mode_param_stru   *pst_prot_param;

    pst_prot_param = (mac_cfg_mode_param_stru *)puc_param;

    pst_prot_param->en_protocol  = pst_mac_vap->en_protocol;
    pst_prot_param->en_band      = pst_mac_vap->st_channel.en_band;
    pst_prot_param->en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;

    *pus_len = OAL_SIZEOF(mac_cfg_mode_param_stru);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_check_mode_param(mac_vap_stru *pst_mac_vap, mac_cfg_mode_param_stru *pst_prot_param)
{
    mac_device_stru            *pst_mac_device;

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_check_mode_param::pst_mac_device null.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 根据device能力对参数进行检查 */
    switch (pst_prot_param->en_protocol)
    {
        case WLAN_LEGACY_11A_MODE:
        case WLAN_LEGACY_11B_MODE:
        case WLAN_LEGACY_11G_MODE:
        case WLAN_MIXED_ONE_11G_MODE:
        case WLAN_MIXED_TWO_11G_MODE:
            break;

        case WLAN_HT_MODE:
        case WLAN_HT_ONLY_MODE:
        case WLAN_HT_11G_MODE:
            if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_HT)
            {
                /* 设置11n协议，但device不支持HT模式 */
                OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support HT mode,en_protocol=%d en_protocol_cap=%d.}",
                                pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
            break;

        case WLAN_VHT_MODE:
        case WLAN_VHT_ONLY_MODE :
            if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_VHT)
            {
                /* 设置11ac协议，但device不支持VHT模式 */
                OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support VHT mode,en_protocol=%d en_protocol_cap=%d.}",
                                 pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
            break;

    #ifdef _PRE_WLAN_FEATURE_11AX
        case WLAN_HE_MODE :
            if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_HE)
            {
                /* 设置11aX协议，但device不支持HE模式 */
                OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support HE mode,en_protocol=%d en_protocol_cap=%d.}",
                                 pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
                return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
            }
            break;
    #endif

        default:
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_check_mode_param::mode param does not in the list.}");
            break;

    }

    if ((mac_vap_bw_mode_to_bw(pst_prot_param->en_bandwidth) >= WLAN_BW_CAP_80M)
       &&(mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap) < WLAN_BW_CAP_80M))
    {
        /* 设置80M带宽，但device能力不支持80M，返回错误码 */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support 80MHz bandwidth,en_protocol=%d en_protocol_cap=%d.}",
                         pst_prot_param->en_bandwidth, mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap));
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    if ((WLAN_BAND_5G == pst_prot_param->en_band) && (WLAN_BAND_CAP_2G == pst_mac_device->en_band_cap))
    {
        /* 设置5G频带，但device不支持5G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support 5GHz band,en_protocol=%d en_protocol_cap=%d.}",
                         pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
    else if ((WLAN_BAND_2G == pst_prot_param->en_band) && (WLAN_BAND_CAP_5G == pst_mac_device->en_band_cap))
    {
        /* 设置2G频带，但device不支持2G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_check_mode_param::not support 2GHz band,en_protocol=%d en_protocol_cap=%d.}",
                         pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_set_mode_check_bandwith(
                wlan_channel_bandwidth_enum_uint8 en_bw_device,
                wlan_channel_bandwidth_enum_uint8 en_bw_config)
{
    /* 要配置带宽是20M */
    if (WLAN_BAND_WIDTH_20M == en_bw_config)
    {
        return OAL_SUCC;
    }

    /* 要配置带宽与首次配置带宽相同 */
    if (en_bw_device == en_bw_config)
    {
        return OAL_SUCC;
    }

    switch (en_bw_device)
    {
        case WLAN_BAND_WIDTH_80PLUSPLUS:
        case WLAN_BAND_WIDTH_80PLUSMINUS:
            if (WLAN_BAND_WIDTH_40PLUS == en_bw_config)
            {
                return OAL_SUCC;
            }
            break;

        case WLAN_BAND_WIDTH_80MINUSPLUS:
        case WLAN_BAND_WIDTH_80MINUSMINUS:
            if (WLAN_BAND_WIDTH_40MINUS == en_bw_config)
            {
                return OAL_SUCC;
            }
            break;

        case WLAN_BAND_WIDTH_40PLUS:
            if ((WLAN_BAND_WIDTH_80PLUSPLUS == en_bw_config)||(WLAN_BAND_WIDTH_80PLUSMINUS == en_bw_config))
            {
                return OAL_SUCC;
            }
            break;
        case WLAN_BAND_WIDTH_40MINUS:
            if ((WLAN_BAND_WIDTH_80MINUSPLUS == en_bw_config)||(WLAN_BAND_WIDTH_80MINUSMINUS == en_bw_config))
            {
                return OAL_SUCC;
            }
            break;
        case WLAN_BAND_WIDTH_20M:
            return OAL_SUCC;

        default:
            break;

    }

    return OAL_FAIL;
}


oal_uint32  hmac_config_set_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mode_param_stru    *pst_prot_param;
    hmac_vap_stru              *pst_hmac_vap;
    oal_uint32                  ul_ret;
    mac_device_stru            *pst_mac_device;

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode_etc::pst_mac_device null.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 设置模式时，device下必须至少有一个vap */
    if (pst_mac_device->uc_vap_num == 0)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode_etc::no vap in device.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_prot_param = (mac_cfg_mode_param_stru *)puc_param;

    /* 检查配置参数是否在device能力内 */
    ul_ret = hmac_config_check_mode_param(pst_mac_vap, pst_prot_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode_etc::hmac_config_check_mode_param failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* device已经配置时，需要校验下频段、带宽是否一致 */
    if ((WLAN_BAND_WIDTH_BUTT != pst_mac_device->en_max_bandwidth) && (!MAC_DBAC_ENABLE(pst_mac_device))
        && (pst_mac_device->uc_vap_num > 1))
    {
        if (pst_mac_device->en_max_band != pst_prot_param->en_band)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode_etc::previous vap band[%d] mismatch with [%d].}",
                             pst_mac_device->en_max_band,  pst_prot_param->en_band);
            return OAL_FAIL;
        }

        ul_ret = hmac_config_set_mode_check_bandwith(pst_mac_device->en_max_bandwidth, pst_prot_param->en_bandwidth);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                             "{hmac_config_set_mode_etc::hmac_config_set_mode_check_bandwith failed[%d],previous vap bandwidth[%d, current[%d].}",
                             ul_ret, pst_mac_device->en_max_bandwidth, pst_prot_param->en_bandwidth);
            return ul_ret;
        }
    }

    if (pst_prot_param->en_protocol >= WLAN_HT_MODE)
    {
        mac_mib_set_TxAggregateActived(&pst_hmac_vap->st_vap_base_info, OAL_TRUE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_TRUE);
    }
    else
    {
        mac_mib_set_TxAggregateActived(&pst_hmac_vap->st_vap_base_info, OAL_FALSE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_FALSE);
    }

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    pst_mac_vap->st_cap_flag.bit_11ac2g = (oal_uint8)!!hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_11AC2G_ENABLE);

    if((OAL_FALSE == pst_mac_vap->st_cap_flag.bit_11ac2g)&&
        (WLAN_VHT_MODE == pst_mac_vap->en_protocol) &&
        (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"{hmac_config_set_mode_etc::11ac2g not supported.\n");
        OAL_IO_PRINT("{hmac_config_set_mode_etc::11ac2g not supported.\n");
        pst_prot_param->en_protocol = WLAN_HT_MODE;
        //return OAL_FAIL;
    }

    if ((WLAN_BAND_2G == pst_mac_vap->st_channel.en_band) &&
        (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_disable_2ght40) &&
        (WLAN_BAND_WIDTH_20M != pst_prot_param->en_bandwidth))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"{hmac_config_set_mode_etc::2ght40 not supported.\n");
        OAL_IO_PRINT("{hmac_config_set_mode_etc::2ght40 not supported.\n");
        pst_prot_param->en_bandwidth = WLAN_BAND_WIDTH_20M;
        //return OAL_FAIL;
    }
#endif

    /* 更新STA协议配置标志位 */
    if (WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
    {
        pst_hmac_vap->bit_sta_protocol_cfg        = OAL_SWITCH_ON;
        pst_hmac_vap->st_preset_para.en_protocol  = pst_prot_param->en_protocol;
        pst_hmac_vap->st_preset_para.en_bandwidth = pst_prot_param->en_bandwidth;
        pst_hmac_vap->st_preset_para.en_band      = pst_prot_param->en_band;
    }

    /* 记录协议模式, band, bandwidth到mac_vap下 */
    pst_mac_vap->en_protocol                              = pst_prot_param->en_protocol;
    pst_mac_vap->st_channel.en_band                       = pst_prot_param->en_band;
    pst_mac_vap->st_channel.en_bandwidth                  = pst_prot_param->en_bandwidth;
    pst_mac_vap->st_ch_switch_info.en_user_pref_bandwidth = pst_prot_param->en_bandwidth;

#ifdef _PRE_WLAN_FEATURE_TXBF_HT
    if ((pst_prot_param->en_protocol < WLAN_HT_MODE)
        || (OAL_TRUE != MAC_DEVICE_GET_CAP_SUBFEE(pst_mac_device)))
    {
        pst_mac_vap->st_cap_flag.bit_11ntxbf = OAL_FALSE;
    }
#endif
    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                  "{hmac_config_set_mode_etc::protocol=%d, band=%d, bandwidth=%d.}",
                  pst_prot_param->en_protocol, pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.en_bandwidth);

    /* 根据协议更新vap能力 */
    mac_vap_init_by_protocol_etc(pst_mac_vap, pst_prot_param->en_protocol);

    /* 更新device的频段及最大带宽信息 */
    if ((WLAN_BAND_WIDTH_BUTT == pst_mac_device->en_max_bandwidth) || (0 == hmac_calc_up_ap_num_etc(pst_mac_device)))
    {
        pst_mac_device->en_max_bandwidth = pst_prot_param->en_bandwidth;
        pst_mac_device->en_max_band      = pst_prot_param->en_band;
    }

    /***************************************************************************
     抛事件到DMAC层, 配置寄存器
    ***************************************************************************/
    ul_ret = hmac_set_mode_event_etc(pst_mac_vap);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mode_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_mac_addr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_FEATURE_P2P
    mac_cfg_staion_id_param_stru    *pst_station_id_param;
    wlan_p2p_mode_enum_uint8         en_p2p_mode;
#endif
    oal_uint32                       ul_ret;

    if(OAL_PTR_NULL == pst_mac_vap->pst_mib_info)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_mac_addr_etc::vap->mib_info is NULL !}");
        return OAL_ERR_CODE_PTR_NULL;
    }
#ifdef _PRE_WLAN_FEATURE_P2P
    /* P2P 设置MAC 地址mib 值需要区分P2P DEV 或P2P_CL/P2P_GO,P2P_DEV MAC 地址设置到p2p0 MIB 中 */
    pst_station_id_param = (mac_cfg_staion_id_param_stru *)puc_param;
    en_p2p_mode          = pst_station_id_param->en_p2p_mode;
    if (en_p2p_mode == WLAN_P2P_DEV_MODE)
    {
        /* 如果是p2p0 device，则配置MAC 地址到auc_p2p0_dot11StationID 成员中 */
        oal_set_mac_addr(mac_mib_get_p2p0_dot11StationID(pst_mac_vap),pst_station_id_param->auc_station_id);
    }
    else
#endif
    {
        /* 设置mib值, Station_ID */
        mac_mib_set_station_id_etc(pst_mac_vap, (oal_uint8)us_len, puc_param);
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_STATION_ID, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mac_addr_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_wmmswitch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru         *pst_mac_device;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wmmswitch::wmm switch[%d].}", pst_mac_device->en_wmm);

    /* get wmm_en status */
    *puc_param = pst_mac_device->en_wmm;
    *pus_len = OAL_SIZEOF(oal_int32);
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_vap_wmm_switch::wmm switch[%d].}", pst_mac_vap->en_vap_wmm);
    *puc_param = pst_mac_vap->en_vap_wmm;
    *pus_len = OAL_SIZEOF(oal_uint32);
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_vap_wmm_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* set wmm_en status */
    pst_mac_vap->en_vap_wmm = (oal_bool_enum_uint8)*puc_param;
    /* 开关WMM，修改mib信息位中的Qos位 */
    mac_mib_set_dot11QosOptionImplemented(pst_mac_vap, pst_mac_vap->en_vap_wmm);

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vap_wmm_switch::wmm switch[%d].}", pst_mac_vap->en_vap_wmm);
    return OAL_SUCC;
}


oal_uint32  hmac_config_bg_noise_info(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_BG_NOISE, OAL_SIZEOF(oal_uint8), puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_bg_noise_info::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_max_user(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{

    *((oal_int32 *)puc_param) = mac_mib_get_MaxAssocUserNums(pst_mac_vap);
    *pus_len = OAL_SIZEOF(oal_int32);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_max_user::chip us_user_num_max[%d], us_user_nums_max[%d].}", mac_chip_get_max_asoc_user(pst_mac_vap->uc_chip_id), mac_mib_get_MaxAssocUserNums(pst_mac_vap));

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_WEB_CMD_COMM

oal_uint32  hmac_config_set_hw_addr(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_staion_id_param_stru  *pst_param;

    pst_param = (mac_cfg_staion_id_param_stru *)puc_param;
    mac_res_set_hw_addr(pst_param->auc_station_id);
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32           l_value;
    oal_uint32          ul_ret = OAL_SUCC;
    mac_device_stru             *pst_mac_device;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru*               pst_vap;
    l_value = *((oal_int32 *)puc_param);

    pst_mac_device              = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_global_shortgi::mac_res_get_dev_etc fail.device_id = %u}",
                           pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 遍历device下所有vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_global_shortgi::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        /* 只有AP VAP 设置global shortgi */
        if ((WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode))
        {
            if (0 != l_value)
            {
                mac_mib_set_ShortGIOptionInTwentyImplemented(pst_vap, OAL_TRUE);
                mac_mib_set_ShortGIOptionInFortyImplemented(pst_vap, OAL_TRUE);
                mac_mib_set_VHTShortGIOptionIn80Implemented(pst_vap, OAL_TRUE);
            }
            else
            {
                mac_mib_set_ShortGIOptionInTwentyImplemented(pst_vap, OAL_FALSE);
                mac_mib_set_ShortGIOptionInFortyImplemented(pst_vap, OAL_FALSE);
                mac_mib_set_VHTShortGIOptionIn80Implemented(pst_vap, OAL_FALSE);
            }
        }
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{

    *((oal_int32 *)puc_param) = pst_mac_vap->st_cap_flag.bit_hide_ssid;
    *pus_len = OAL_SIZEOF(oal_int32);

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_hide_ssid::bit_hide_ssid[%d].}", pst_mac_vap->st_cap_flag.bit_hide_ssid);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_global_shortgi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{

    *((oal_int32 *)puc_param) = mac_mib_get_ShortGIOptionInTwentyImplemented(pst_mac_vap)
                                | mac_mib_get_ShortGIOptionInFortyImplemented(pst_mac_vap)
                                | mac_mib_get_VHTShortGIOptionIn80Implemented(pst_mac_vap);
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32  l_value;
    l_value = *((oal_int32 *)puc_param);

    if (0 != l_value)
    {
        pst_mac_vap->st_cap_flag.bit_11ntxbf = OAL_TRUE;
    }
    else
    {
        pst_mac_vap->st_cap_flag.bit_11ntxbf = OAL_FALSE;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_txbeamform(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{

    *((oal_int32 *)puc_param) = pst_mac_vap->st_cap_flag.bit_11ntxbf;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret = OAL_SUCC;
    oal_uint32          *pul_cfg_forword;

    pul_cfg_forword = (oal_uint32 *)puc_param;

    if (OAL_TRUE == *pul_cfg_forword)
    {
        ul_ret = hmac_isolation_set_type(pst_mac_vap, CS_ISOLATION_TYPE_SINGLE_BSS, OAL_TRUE);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{hmac_config_set_noforward::set type fail: vap id: %d, ul_ret: %d}\r\n", pst_mac_vap->uc_vap_id, ul_ret);
            return ul_ret;
        }
        /*lint -e655*/
        ul_ret = hmac_isolation_set_mode(pst_mac_vap, (oal_uint8)(CS_ISOLATION_MODE_BROADCAST | CS_ISOLATION_MODE_MULTICAST | CS_ISOLATION_MODE_UNICAST));
        /*lint +e655*/
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{hmac_config_set_noforward::set mode fail: vap id: %d, ul_ret: %d}\r\n", pst_mac_vap->uc_vap_id, ul_ret);
            return ul_ret;
        }

        ul_ret = hmac_isolation_set_forward(pst_mac_vap, CS_ISOLATION_FORWORD_DROP);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{hmac_config_set_noforward::set forward fail: vap id: %d, ul_ret: %d}\r\n", pst_mac_vap->uc_vap_id, ul_ret);
            return ul_ret;
        }
    }
    else
    {
        ul_ret = hmac_isolation_set_type(pst_mac_vap, CS_ISOLATION_TYPE_SINGLE_BSS, OAL_FALSE);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{hmac_config_set_noforward::set type fail: vap id: %d, ul_ret: %d}\r\n", pst_mac_vap->uc_vap_id, ul_ret);
            return ul_ret;
        }

        ul_ret = hmac_isolation_set_forward(pst_mac_vap, CS_ISOLATION_FORWORD_NONE);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                "{hmac_config_set_noforward::set forward fail: vap id: %d, ul_ret: %d}\r\n", pst_mac_vap->uc_vap_id, ul_ret);
            return ul_ret;
        }
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_noforward(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                 *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_noforward::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = (pst_hmac_vap->st_isolation_info.uc_single_type && (0x7 == pst_hmac_vap->st_isolation_info.uc_mode)
                            && (CS_ISOLATION_FORWORD_DROP == pst_hmac_vap->st_isolation_info.uc_forward));
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_priv_set_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mode_param_stru    *pst_prot_param;
    hmac_vap_stru              *pst_hmac_vap;
    oal_uint32                  ul_ret;
    mac_device_stru            *pst_mac_device;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru*               pst_vap;

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_mode::pst_mac_device null.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }
    /* 设置模式时，device下必须至少有一个vap */
    if (pst_mac_device->uc_vap_num == 0)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_mode::no vap in device.}");
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }
    pst_prot_param = (mac_cfg_mode_param_stru *)puc_param;

    /* 检查配置参数是否在device能力内 */
    ul_ret = hmac_config_check_mode_param(pst_mac_vap, pst_prot_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_mode::hmac_config_check_mode_param failed[%d].}", ul_ret);
        return ul_ret;
    }

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    pst_mac_vap->st_cap_flag.bit_11ac2g = (oal_uint8)!!hwifi_get_init_value_etc(CUS_TAG_INI, WLAN_CFG_INIT_11AC2G_ENABLE);

    if((OAL_FALSE == pst_mac_vap->st_cap_flag.bit_11ac2g)&&
        (WLAN_VHT_MODE == pst_mac_vap->en_protocol) &&
        (WLAN_BAND_2G == pst_mac_vap->st_channel.en_band))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"{hmac_config_priv_set_mode::11ac2g not supported.\n");
        OAL_IO_PRINT("{hmac_config_priv_set_mode::11ac2g not supported.\n");
        pst_prot_param->en_protocol = WLAN_HT_MODE;
        //return OAL_FAIL;
    }

    if ((WLAN_BAND_2G == pst_mac_vap->st_channel.en_band) &&
        (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_disable_2ght40) &&
        (WLAN_BAND_WIDTH_20M != pst_prot_param->en_bandwidth))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"{hmac_config_priv_set_mode::2ght40 not supported.\n");
        OAL_IO_PRINT("{hmac_config_priv_set_mode::2ght40 not supported.\n");
        pst_prot_param->en_bandwidth = WLAN_BAND_WIDTH_20M;
        //return OAL_FAIL;
    }
#endif
    /* 更新device的频段及最大带宽信息 */
    if ((WLAN_BAND_WIDTH_BUTT == pst_mac_device->en_max_bandwidth) || (0 == hmac_calc_up_ap_num_etc(pst_mac_device)))
    {
        pst_mac_device->en_max_bandwidth = pst_prot_param->en_bandwidth;
        pst_mac_device->en_max_band      = pst_prot_param->en_band;
    }

    /* 遍历device下所有vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_mode::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
        {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_mode::sta don't set mode: sta protocol[%d].}", pst_vap->en_protocol);
            continue;
        }

        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_vap->uc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_ERROR_LOG0(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_mode::pst_hmac_vap null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }


        if (pst_prot_param->en_protocol >= WLAN_HT_MODE)
        {
            mac_mib_set_TxAggregateActived(&pst_hmac_vap->st_vap_base_info, OAL_TRUE);
            mac_mib_set_AmsduAggregateAtive(pst_vap, OAL_TRUE);
        }
        else
        {
            mac_mib_set_TxAggregateActived(&pst_hmac_vap->st_vap_base_info, OAL_FALSE);
            mac_mib_set_AmsduAggregateAtive(pst_vap, OAL_FALSE);
        }

        /* 更新STA协议配置标志位 */
        if (WLAN_VAP_MODE_BSS_STA == pst_vap->en_vap_mode)
        {
            pst_hmac_vap->bit_sta_protocol_cfg        = OAL_SWITCH_ON;
            pst_hmac_vap->st_preset_para.en_protocol  = pst_prot_param->en_protocol;
            pst_hmac_vap->st_preset_para.en_bandwidth = pst_prot_param->en_bandwidth;
            pst_hmac_vap->st_preset_para.en_band      = pst_prot_param->en_band;
        }

        /* 记录协议模式, band, bandwidth到mac_vap下 */
        pst_vap->en_protocol                              = pst_prot_param->en_protocol;
        pst_vap->st_channel.en_band                       = pst_prot_param->en_band;
        pst_vap->st_channel.en_bandwidth                  = pst_prot_param->en_bandwidth;
        pst_vap->st_ch_switch_info.en_user_pref_bandwidth = pst_prot_param->en_bandwidth;

#ifdef _PRE_WLAN_FEATURE_TXBF_HT
        if ((pst_prot_param->en_protocol >= WLAN_HT_MODE)
            && (OAL_TRUE == MAC_DEVICE_GET_CAP_SUBFEE(pst_mac_device)))
        {
            pst_vap->st_cap_flag.bit_11ntxbf = pst_mac_vap->st_cap_flag.bit_11ntxbf;
        }
        else
        {
            pst_vap->st_cap_flag.bit_11ntxbf = OAL_FALSE;
        }
#endif
        OAM_WARNING_LOG3(pst_vap->uc_vap_id, OAM_SF_CFG,
                      "{hmac_config_priv_set_mode::protocol=%d, band=%d, bandwidth=%d.}",
                      pst_prot_param->en_protocol, pst_vap->st_channel.en_band, pst_vap->st_channel.en_bandwidth);

        /* 根据协议更新vap能力 */
        mac_vap_init_by_protocol_etc(pst_vap, pst_prot_param->en_protocol);


        /***************************************************************************
         抛事件到DMAC层, 配置寄存器
        ***************************************************************************/
        ul_ret = hmac_set_mode_event_etc(pst_vap);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_mode::hmac_config_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

    return ul_ret;
}


oal_uint32  hmac_config_priv_set_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                           ul_ret;
    oal_uint8                            uc_channel = *puc_param;
    mac_device_stru                     *pst_mac_device;

    mac_cfg_channel_param_stru l_channel_param;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru*               pst_vap;
    wlan_channel_bandwidth_enum_uint8   en_bw_origin;
    mac_cfg_mode_param_stru     st_mode_cfg;
#ifdef _PRE_WLAN_FEATURE_DFS
    oal_uint32                  ul_err_code;
    oal_uint8                   uc_chan_idx;
#endif
    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_channel::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_is_channel_num_valid_etc(pst_mac_vap->st_channel.en_band, uc_channel);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_channel::mac_is_channel_num_valid_etc[%d] failed[%d].}", uc_channel, ul_ret);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_channel::ch=%d.}",uc_channel);

#ifdef _PRE_WLAN_FEATURE_DFS
    /* 如果配置的信道被雷达占用，不可配置 */
    ul_err_code = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, uc_channel, &uc_chan_idx);
    if (OAL_ERR_CODE_INVALID_CONFIG != ul_err_code &&
        MAC_CHAN_BLOCK_DUE_TO_RADAR  == pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status)
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "hmac_config_priv_set_channel::The channel %d is used by radar and fail[%d].\n", uc_channel, OAL_ERR_CODE_DFS_CHAN_BLOCK_DUE_TO_RADAR);
        return OAL_ERR_CODE_DFS_CHAN_BLOCK_DUE_TO_RADAR;
    }
#endif

    //设置channel时,需要检查该信道是否支持当前设置的带宽扩展模式，
    //如果当前带宽模式与当前信道不匹配，驱动需要自适应调整带宽的扩展方向。
    if((WLAN_BAND_WIDTH_BUTT != pst_mac_vap->st_channel.en_bandwidth)
    &&(WLAN_BAND_BUTT != pst_mac_vap->st_channel.en_band)
    &&(WLAN_PROTOCOL_BUTT != pst_mac_vap->en_protocol))
    {
        en_bw_origin = pst_mac_vap->st_channel.en_bandwidth;
        pst_mac_vap->st_channel.en_bandwidth = mac_regdomain_get_support_bw_mode(en_bw_origin,uc_channel);
        if(pst_mac_vap->st_channel.en_bandwidth != en_bw_origin)
        {
            //重新调整带宽扩展方向之后，需要重新设置模式
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_config_priv_set_channel::bw_mode[%d] will be change to [%d].}",
            en_bw_origin,
            pst_mac_vap->st_channel.en_bandwidth);
            st_mode_cfg.en_band = pst_mac_vap->st_channel.en_band;
            st_mode_cfg.en_bandwidth = pst_mac_vap->st_channel.en_bandwidth;
            st_mode_cfg.en_protocol = pst_mac_vap->en_protocol;

            ul_ret = hmac_config_priv_set_mode(pst_mac_vap, (oal_uint16)OAL_SIZEOF(st_mode_cfg), (oal_uint8*)&st_mode_cfg);
            if(ul_ret != OAL_SUCC)
            {
                OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_channel::hmac_config_priv_set_mode fail[%d].}", ul_ret);
                return OAL_ERR_CODE_INVALID_CONFIG;
            }
        }
    }

    mac_device_get_channel_etc(pst_mac_device, &l_channel_param);
    l_channel_param.uc_channel = uc_channel;
    mac_device_set_channel_etc(pst_mac_device, &l_channel_param);

    /* 遍历device下所有vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_channel::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        if(IS_STA(pst_vap))
        {
            OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_channel::pst_mac_vap(%d) is a station.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

#ifdef _PRE_WLAN_FEATURE_11D
        /* 信道14特殊处理，只在11b协议模式下有效 */
        if ((14 == uc_channel) && (WLAN_LEGACY_11B_MODE != pst_vap->en_protocol))
        {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG,
                             "{hmac_config_priv_set_channel::channel-14 only available in 11b, curr protocol=%d.}", pst_vap->en_protocol);
            return OAL_ERR_CODE_INVALID_CONFIG;
        }
#endif

        pst_vap->st_channel.uc_chan_number = uc_channel;
        ul_ret = mac_get_channel_idx_from_num_etc(pst_vap->st_channel.en_band, uc_channel, &(pst_vap->st_channel.uc_chan_idx));
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG2(pst_vap->uc_vap_id,OAM_SF_CFG,"{hmac_config_priv_set_channel::mac_get_channel_idx_from_num_etc fail.band[%u] channel[%u]!}",
                pst_vap->st_channel.en_band,pst_vap->st_channel.uc_chan_idx);
            return ul_ret;
        }

        /***************************************************************************
            抛事件到DMAC层, 同步DMAC数据
        ***************************************************************************/
        ul_ret = hmac_config_send_event_etc(pst_vap, WLAN_CFGID_CURRENT_CHANEL, us_len, puc_param);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_priv_set_channel::hmac_config_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_channel(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *pus_len = OAL_SIZEOF(oal_uint32);

    *((oal_uint32 *)puc_param) = pst_mac_vap->st_channel.uc_chan_number;
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_assoc_num(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *pus_len = OAL_SIZEOF(oal_uint32);

    *((oal_uint32 *)puc_param) = pst_mac_vap->us_user_nums;
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_pmf(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint8   uc_ret;

    if((OAL_FALSE == pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAMFPC) &&
        (OAL_FALSE == pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAMFPR))
    {
        uc_ret = MAC_PMF_DISABLED;
    }
    else if((OAL_TRUE == pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAMFPC) &&
            (OAL_FALSE == pst_mac_vap->pst_mib_info->st_wlan_mib_privacy.en_dot11RSNAMFPR))
    {
        uc_ret = MAC_PMF_ENABLED;
    }
    else
    {
        uc_ret = MAC_PMF_REQUIRED;
    }

    *pus_len = OAL_SIZEOF(oal_uint32);
    *((oal_uint32 *)puc_param) = uc_ret;
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_noise(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    wlan_chan_ratio_stru                 *pst_chan_ratio;
    mac_device_stru                     *pst_mac_dev;
    wlan_channel_bandwidth_enum_uint8   en_bw;

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_noise::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(pst_mac_vap->en_vap_state != MAC_VAP_STATE_UP)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_noise::vap state is %d.}", pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }

    pst_chan_ratio     = &(pst_mac_dev->st_chan_ratio);
    en_bw = pst_mac_vap->st_channel.en_bandwidth;

    *pus_len = OAL_SIZEOF(oal_uint32);
    if(en_bw >= WLAN_BAND_WIDTH_BUTT)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_noise::invalid bw=%d.}",en_bw);
        return OAL_FAIL;
    }

    if(WLAN_BAND_WIDTH_20M == en_bw)
    {
        *((oal_uint32 *)puc_param) = pst_chan_ratio->us_chan_ratio_20M;
    }
    else if((WLAN_BAND_WIDTH_40PLUS == en_bw) || (WLAN_BAND_WIDTH_40MINUS == en_bw))
    {
        *((oal_uint32 *)puc_param) = pst_chan_ratio->us_chan_ratio_40M;
    }
    else
    {
        *((oal_uint32 *)puc_param) = pst_chan_ratio->us_chan_ratio_80M;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_bw(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    wlan_bw_cap_enum_uint8 en_bw;

    mac_vap_get_bandwidth_cap_etc(pst_mac_vap, &en_bw);
    *((oal_uint32 *)puc_param) = en_bw;
    *pus_len = OAL_SIZEOF(oal_uint32);
    return OAL_SUCC;
}


oal_int32  hmac_config_get_sta_rssi(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 *puc_param)
{
    hmac_user_stru         *pst_hmac_user;
    mac_cfg_query_rssi_stru st_query_rssi_param;
    hmac_vap_stru          *pst_hmac_vap;
    oal_int32               l_value;
    oal_int32               l_ret;

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, puc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_rssi::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_rssi::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_query_rssi_param.us_user_id = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_hmac_vap->station_info_query_completed_flag = OAL_FALSE;
    hmac_config_query_rssi_etc(pst_mac_vap , (oal_uint16)OAL_SIZEOF(st_query_rssi_param), (oal_uint8 *)&st_query_rssi_param);
    /*lint -e730*/
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q, OAL_TRUE == pst_hmac_vap->station_info_query_completed_flag, (5*OAL_TIME_HZ));
    /*lint +e730*/
    if (l_ret <= 0)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_rssi::OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT [%d].}", l_ret);
        return OAL_EFAIL;
    }
    l_value = (oal_int32)(pst_hmac_user->c_rssi) + (oal_int32)HMAC_FBT_RSSI_ADJUST_VALUE;

    return l_value;
}


oal_uint32  hmac_config_get_ko_version(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_KO_VERSION, OAL_SIZEOF(oal_uint8), puc_param);
}


oal_uint32  hmac_config_get_bcast_rate(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_BCAST_RATE, OAL_SIZEOF(oal_uint8), puc_param);
}


oal_uint32  hmac_config_get_wps_ie(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint8                   uc_version,uc_state;
    oal_int8                    *ast_buff[] = {"Disable","Unconfigured","Configured"};
    oal_int32                   l_ret_len = 0;
    mac_cfg_param_char_stru     *pst_wps_ie;

    if(0 == pst_mac_vap->ast_app_ie[OAL_APP_BEACON_IE].ul_ie_len)
    {
        *pus_len = 0;
        return OAL_SUCC;
    }
    /* 获取版本号 */
    pst_wps_ie = (mac_cfg_param_char_stru*)puc_param;
    uc_version = (*(pst_mac_vap->ast_app_ie[OAL_APP_BEACON_IE].puc_ie + 6)) >> 4;
    pst_wps_ie->auc_buff[0] = '\0';

    if((1 == uc_version) || (2 == uc_version))
    {
        l_ret_len += OAL_SPRINTF((oal_int8*)pst_wps_ie->auc_buff + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "\nVersion   : %d.0\n", uc_version);
    }
    else
    {
        l_ret_len += OAL_SPRINTF((oal_int8*)pst_wps_ie->auc_buff + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "\nVersion   : unknow 0x%x\n", uc_version);
    }

    /* 获取wps状态 */
    uc_state = *(pst_mac_vap->ast_app_ie[OAL_APP_BEACON_IE].puc_ie + 15);
    if((0 == uc_state) || (1 == uc_state) || (2 == uc_state))
    {
        l_ret_len += OAL_SPRINTF((oal_int8*)pst_wps_ie->auc_buff + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "Wps state : %d %s\n", uc_state, ast_buff[uc_state]);
    }
    else
    {
        l_ret_len += OAL_SPRINTF((oal_int8*)pst_wps_ie->auc_buff + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "Wps state : Invalid state 0x%x\n", uc_state);
    }

    /* 字符串回填 */
    *pus_len = OAL_SIZEOF(mac_cfg_param_char_stru);
    pst_wps_ie->l_buff_len = l_ret_len;

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_vap_cap(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_vap_param_stru      *pst_vap_cap;

    pst_vap_cap = (mac_vap_param_stru*)puc_param;

    pst_vap_cap->bit_uapsd        = pst_mac_vap->st_cap_flag.bit_uapsd;
    pst_vap_cap->bit_11ntxbf      = pst_mac_vap->st_cap_flag.bit_11ntxbf;
    pst_vap_cap->bit_ampdu        = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11CfgAmpduTxAtive;
    pst_vap_cap->bit_ampdu_amsdu  = pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.en_dot11AmsduPlusAmpduActive;
    pst_vap_cap->bit_hide_ssid    = pst_mac_vap->st_cap_flag.bit_hide_ssid;

    pst_vap_cap->bit_frameburst       = (mac_mib_get_EDCATableTXOPLimit(pst_mac_vap, 0) == 0)? 0 : 1;
    pst_vap_cap->bit_short_gi_20m     = mac_mib_get_ShortGIOptionInTwentyImplemented(pst_mac_vap);
    pst_vap_cap->bit_short_gi_40m     = mac_mib_get_ShortGIOptionInFortyImplemented(pst_mac_vap);
    pst_vap_cap->bit_short_preamble   = mac_mib_get_ShortPreambleOptionImplemented(pst_mac_vap);
    pst_vap_cap->bit_qos              = mac_mib_get_dot11QosOptionImplemented(pst_mac_vap);
    pst_vap_cap->bit_tx_stbc          = mac_mib_get_TxSTBCOptionImplemented(pst_mac_vap);
    pst_vap_cap->bit_rx_stbc          = mac_mib_get_RxSTBCOptionImplemented(pst_mac_vap);
    pst_vap_cap->bit_ldpc             = mac_mib_get_LDPCCodingOptionImplemented(pst_mac_vap);

    pst_vap_cap->bit_resv             = 0;

    *pus_len = OAL_SIZEOF(mac_vap_param_stru);

    return OAL_SUCC;
}



oal_uint32  hmac_config_get_rate_info(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_cfg_param_char_stru     *pst_rate_info;
    oal_uint8                   uc_index;
    oal_uint8                   uc_acs_index;
    oal_int32                   l_ret_len = 0;
    oal_uint8                   auc_11g_mbps[WLAN_MAX_SUPP_RATES];  /*for 11g*/
    oal_uint8                   uc_11g_rate_num;                    /*for 11g*/
    oal_uint8                   auc_rx_mcs[10];                     /*for ht */
    mac_rx_max_mcs_map_stru     *pst_rx_max_mcs_map;                /*for vht*/
    oal_uint16                  aus_vht_mac_map[2]={0};             /*for vht*/
    oal_uint8                   uc_max_nss_num;

    uc_max_nss_num = WLAN_DOUBLE_NSS + 1;
    pst_rate_info = (mac_cfg_param_char_stru*)puc_param;
    pst_rate_info->auc_buff[0] = '\0';

    if(pst_mac_vap->en_protocol < WLAN_HT_MODE)
    {
        /* 获取并解析11g速率 */
        uc_11g_rate_num = pst_mac_vap->st_curr_sup_rates.st_rate.uc_rs_nrates;
        for(uc_index = 0;uc_index < uc_11g_rate_num;uc_index++)
        {
            auc_11g_mbps[uc_index] = pst_mac_vap->st_curr_sup_rates.st_rate.ast_rs_rates[uc_index].uc_mbps;
            l_ret_len += OAL_SPRINTF((oal_int8*)(pst_rate_info->auc_buff) + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "%dM ", auc_11g_mbps[uc_index]);
        }
    }
    else if((WLAN_HT_MODE == pst_mac_vap->en_protocol)
            || (WLAN_HT_ONLY_MODE == pst_mac_vap->en_protocol)
            || (WLAN_HT_11G_MODE == pst_mac_vap->en_protocol))
    {
        /* 获取并解析ht rate */
        oal_memcopy(auc_rx_mcs, mac_mib_get_SupportedMCSRx(pst_mac_vap), WLAN_HT_MCS_BITMASK_LEN);
        for(uc_index = 0;uc_index < uc_max_nss_num;uc_index++) /* 目前最大能力双流 */
        {
            for(uc_acs_index = 0;uc_acs_index < HMAC_MAX_MCS_NUM;uc_acs_index++)
            {
                if((auc_rx_mcs[uc_index] >> uc_acs_index)& 0x1)
                {
                    if(0 == uc_index)
                    {
                        l_ret_len += OAL_SPRINTF((oal_int8*)(pst_rate_info->auc_buff) + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "MCS%d ", uc_acs_index);
                    }
                    else
                    {
                        l_ret_len += OAL_SPRINTF((oal_int8*)(pst_rate_info->auc_buff) + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "MCS%d ", uc_acs_index + 8);
                    }
                }
            }
        }
    }
    else if((WLAN_VHT_MODE == pst_mac_vap->en_protocol)
            || (WLAN_VHT_ONLY_MODE == pst_mac_vap->en_protocol))
    {
        /* 获取并解析vht速率 */
        pst_rx_max_mcs_map = (mac_rx_max_mcs_map_stru *)(mac_mib_get_ptr_vht_rx_mcs_map(pst_mac_vap));
        aus_vht_mac_map[0] = pst_rx_max_mcs_map->us_max_mcs_1ss;
        aus_vht_mac_map[1] = pst_rx_max_mcs_map->us_max_mcs_2ss;
        for(uc_index = 0;uc_index < uc_max_nss_num;uc_index++) /* 目前最大能力双流 */
        {
            if(MAC_MAX_SUP_MCS7_11AC_EACH_NSS == aus_vht_mac_map[uc_index])
            {
                l_ret_len += OAL_SPRINTF((oal_int8*)(pst_rate_info->auc_buff) + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "%dss-MCS[0-7] ",uc_index+1);
            }
            else if(MAC_MAX_SUP_MCS8_11AC_EACH_NSS == aus_vht_mac_map[uc_index])
            {
                l_ret_len += OAL_SPRINTF((oal_int8*)(pst_rate_info->auc_buff) + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "%dss-MCS[0-8] ",uc_index+1);
            }
            else
            {
                l_ret_len += OAL_SPRINTF((oal_int8*)(pst_rate_info->auc_buff) + l_ret_len, (oal_uint32)(HMAC_RSP_MSG_MAX_LEN - l_ret_len), "%dss-MCS[0-9] ",uc_index+1);
            }
        }
    }
    else
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_rate_info::invalid protocol %d.}", pst_mac_vap->en_protocol);
        return OAL_FAIL;
    }

    /* 字符串回填 */
    *pus_len = OAL_SIZEOF(mac_cfg_param_char_stru);
    pst_rate_info->l_buff_len = l_ret_len;
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_chan_list(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint8                uc_chan_num;
    oal_uint8                uc_chan_idx;
    oal_uint32               ul_ret         = OAL_FAIL;
    mac_chan_list_stru       *pst_chan_list;
    oal_uint8                uc_total_chan_num;

    pst_chan_list = (mac_chan_list_stru*)puc_param;
    uc_total_chan_num = 0;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_list_channel_etc::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_2, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx_etc(MAC_RC_START_FREQ_2, uc_chan_idx, &uc_chan_num);
            if(WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
            {
                pst_chan_list->auc_chan[uc_total_chan_num] = uc_chan_num;
                uc_total_chan_num++;
            }
        }
    }
    if (OAL_FALSE == mac_device_check_5g_enable(pst_mac_vap->uc_device_id))
    {
        return OAL_SUCC;
    }

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx_etc(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_num);
#ifdef _PRE_WLAN_FEATURE_DFS
            if(WLAN_BAND_5G == pst_mac_vap->st_channel.en_band)
            {
                pst_chan_list->auc_chan[uc_total_chan_num] = uc_chan_num;
                uc_total_chan_num++;
            }
#endif
        }
    }

    pst_chan_list->uc_total_chan_num = uc_total_chan_num;
    *pus_len = OAL_SIZEOF(mac_chan_list_stru);
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_band(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *pus_len = OAL_SIZEOF(oal_uint32);
    *((oal_uint32 *)puc_param) = pst_mac_vap->st_channel.en_band;
    return OAL_SUCC;
}


oal_uint32  hmac_config_neighbor_scan(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                   *pst_hmac_vap;
    hmac_device_stru                *pst_hmac_device;
    oal_uint32                      ul_ret = OAL_EFAIL;
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    mac_neighbor_req_info_stru      st_neighbor_req;
    OAL_MEMZERO(&st_neighbor_req, OAL_SIZEOF(mac_neighbor_req_info_stru));
#endif

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_neighbor_scan::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_EFAUL;
    }
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_neighbor_scan::pst_hmac_vap  null ptr error %d.}\r\n", pst_hmac_vap);
        return OAL_EFAUL;
    }

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);

    /*判断device是否正在进行测量*/
    if ( OAL_TRUE == pst_hmac_device->st_scan_mgmt.en_is_scanning)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_config_neighbor_scan::vap is handling one request now.}");
        return OAL_EBUSY;
    }

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    /*发起扫描*/
    ul_ret = hmac_rrm_neighbor_scan_do(pst_hmac_vap, &st_neighbor_req, OAL_TRUE);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_neighbor_scan::hmac_rrm_neighbor_scan_do failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
#else
    return ul_ret;
#endif
}


oal_uint32  hmac_config_get_scan_stat(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_device_stru                *pst_hmac_device;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_get_scan_stat::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_EFAUL;
    }

    /* 获取hmac device和扫描运行记录 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);

    *((oal_uint32 *)puc_param) = pst_hmac_device->st_scan_mgmt.en_is_scanning;
    *pus_len = OAL_SIZEOF(oal_uint32);

    return OAL_SUCC;
}


oal_uint32 hmac_config_get_pwr_ref(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_PWR_REF, OAL_SIZEOF(oal_uint8), puc_param);
}


oal_uint32  hmac_config_get_neighb_no(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_device_stru                *pst_hmac_device;
    hmac_scan_record_stru           *pst_record;
    oal_dlist_head_stru *pst_bss_list_head;
    oal_dlist_head_stru         *pst_entry;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    mac_bss_dscr_stru           *pst_bss_dscr;
    hmac_scanned_bss_info       *pst_scanned_bss;
    oal_uint8                   uc_neighbor_num = 0; /*邻居AP个数*/

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_get_neighb_no::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取hmac device和扫描运行记录 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    /*判断device是否正在进行测量*/
    if ( OAL_TRUE == pst_hmac_device->st_scan_mgmt.en_is_scanning)
    {
        OAM_WARNING_LOG0(0, OAM_SF_RRM, "{hmac_config_get_neighb_no::vap is handling one request now.}");
        *((oal_uint32 *)puc_param) = 0;
        *pus_len = OAL_SIZEOF(oal_uint32);

        return OAL_SUCC;
    }

    pst_record      = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);
    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_record->st_bss_mgmt);
    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));
    pst_bss_list_head = &(pst_bss_mgmt->st_bss_list_head);

    if (MAC_SCAN_SUCCESS != pst_record->en_scan_rsp_status || OAL_PTR_NULL == pst_bss_list_head)
    {
         /* 解除锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
       OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_get_neighb_no::scan status  invalid or bss list null ptr error %d,%d.}\r\n", pst_record->en_scan_rsp_status, pst_bss_list_head);
        return OAL_EFAUL;
    }


    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, pst_bss_list_head)
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        if (OAL_PTR_NULL == pst_scanned_bss)
        {
            continue;
        }

        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if ((OAL_PTR_NULL == pst_bss_dscr) || (OAL_PTR_NULL == pst_bss_dscr->auc_mgmt_buff))
        {
            continue;
        }

        uc_neighbor_num++;
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    *((oal_uint32 *)puc_param) = uc_neighbor_num;
    *pus_len = OAL_SIZEOF(oal_uint32);

    return OAL_SUCC;
}


oal_int32  hmac_config_get_neighb_info(mac_vap_stru *pst_mac_vap, oal_ap_scan_result_stru *puc_param)
{
    hmac_device_stru                *pst_hmac_device;
    hmac_scan_record_stru           *pst_record;
    oal_dlist_head_stru *pst_bss_list_head;
    oal_dlist_head_stru         *pst_entry;
    hmac_bss_mgmt_stru              *pst_bss_mgmt;
    mac_bss_dscr_stru           *pst_bss_dscr;
    hmac_scanned_bss_info       *pst_scanned_bss;
    oal_ap_info_stru            st_ap_info;
    oal_uint16                   us_noise;
    oal_uint16              us_temp;
    mac_crypto_settings_stru st_crypto;
    oal_uint8               uc_idx = 0;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_get_neighb_info::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_EFAUL;
    }
    /* 获取hmac device和扫描运行记录 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    /*判断device是否正在进行测量*/
    if ( OAL_TRUE == pst_hmac_device->st_scan_mgmt.en_is_scanning)
    {
        OAM_ERROR_LOG0(0, OAM_SF_RRM, "{hmac_config_get_neighb_info::vap is handling one request now.}");
        return OAL_EBUSY;
    }

    pst_record      = &(pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt);
    hmac_config_get_noise(pst_mac_vap, &us_temp, (oal_uint8*)(&us_noise));
    /* 获取扫描结果的管理结构地址 */
    pst_bss_mgmt = &(pst_record->st_bss_mgmt);
    /* 获取锁 */
    oal_spin_lock(&(pst_bss_mgmt->st_lock));
    pst_bss_list_head = &(pst_bss_mgmt->st_bss_list_head);

    if (MAC_SCAN_SUCCESS != pst_record->en_scan_rsp_status || OAL_PTR_NULL == pst_bss_list_head)
    {
        /* 解除锁 */
        oal_spin_unlock(&(pst_bss_mgmt->st_lock));
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_get_neighb_info::scan status  invalid or bss list null ptr error %d,%d.}\r\n", pst_record->en_scan_rsp_status, pst_bss_list_head);
        return OAL_EFAUL;
    }

    us_temp = 0;
    /* 遍历扫描到的bss信息 */
    OAL_DLIST_SEARCH_FOR_EACH(pst_entry, pst_bss_list_head)
    {
        pst_scanned_bss = OAL_DLIST_GET_ENTRY(pst_entry, hmac_scanned_bss_info, st_dlist_head);
        if (OAL_PTR_NULL == pst_scanned_bss)
        {
            continue;
        }

        pst_bss_dscr    = &(pst_scanned_bss->st_bss_dscr_info);
        if ((OAL_PTR_NULL == pst_bss_dscr) || (OAL_PTR_NULL == pst_bss_dscr->auc_mgmt_buff))
        {
            continue;
        }

        OAL_MEMZERO(&st_ap_info, sizeof(oal_ap_info_stru));
        st_ap_info.uc_dtim_period = pst_bss_dscr->uc_dtim_period;
        st_ap_info.us_beacon_period = pst_bss_dscr->us_beacon_period;
        st_ap_info.us_noise         = us_noise;
        st_ap_info.c_rssi           = pst_bss_dscr->c_rssi;
        st_ap_info.network_type   = 0;
        st_ap_info.ssid_len       = (OAL_STRLEN(pst_bss_dscr->ac_ssid) > OAL_IEEE80211_MAX_SSID_LEN ? OAL_IEEE80211_MAX_SSID_LEN : OAL_STRLEN(pst_bss_dscr->ac_ssid));
        oal_memcopy(st_ap_info.auc_ssid, pst_bss_dscr->ac_ssid, st_ap_info.ssid_len);
        oal_memcopy(st_ap_info.auc_bssid, pst_bss_dscr->auc_bssid, WLAN_MAC_ADDR_LEN);
        st_ap_info.uc_channel = pst_bss_dscr->st_channel.uc_chan_number;
        switch ( pst_bss_dscr->st_channel.en_bandwidth)
        {
            case WLAN_BAND_WIDTH_40M:
                st_ap_info.uc_bw = 1;
                break;

            case WLAN_BAND_WIDTH_80M:
                st_ap_info.uc_bw = 2;
                break;

#ifdef _PRE_WLAN_FEATURE_160M
            case WLAN_BAND_WIDTH_160MINUSMINUSMINUS:
                st_ap_info.uc_bw = 3;
                break;
#endif

            case WLAN_BAND_WIDTH_20M:
            default:
                st_ap_info.uc_bw = 0;
                break;
        }
        hmac_sta_get_user_protocol_etc(pst_bss_dscr, (wlan_protocol_enum_uint8*)(&(st_ap_info.en_protocol)));
        st_ap_info.uc_num_supp_rates = pst_bss_dscr->uc_num_supp_rates;
        oal_memcopy(st_ap_info.auc_supp_rates, pst_bss_dscr->auc_supp_rates, pst_bss_dscr->uc_num_supp_rates);

        st_ap_info.uc_max_nss        = pst_bss_dscr->uc_max_nss;
        st_ap_info.ul_max_rate_kbps  = pst_bss_dscr->ul_max_rate_kbps;
    #if 0
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_get_neighb_info::<st_ap_info> max_nss = %u, max_rate_kbps = %u}",
            st_ap_info.uc_max_nss, st_ap_info.ul_max_rate_kbps);
    #endif

        st_ap_info.privacy = OAL_GET_BITS(pst_bss_dscr->us_cap_info, 1, BIT_OFFSET_4);
        if (0 != st_ap_info.privacy)
        {
            if (OAL_PTR_NULL != pst_bss_dscr->puc_rsn_ie && OAL_PTR_NULL != pst_bss_dscr->puc_wpa_ie)
            {
                mac_ie_get_rsn_cipher(pst_bss_dscr->puc_rsn_ie, &(st_crypto));
                st_ap_info.crypto.wpa_versions = WITP_WPA_VERSION_C;
                st_ap_info.crypto.cipher_group = mac_oui_to_cipher_group(st_crypto.ul_group_suite);
                st_ap_info.crypto.n_ciphers_pairwise = st_crypto.n_pair_suite;
                st_ap_info.crypto.n_akm_suites = st_crypto.n_akm_suites;
                for (uc_idx = 0; uc_idx < st_crypto.n_pair_suite; uc_idx++)
                {
                    st_ap_info.crypto.ciphers_pairwise[uc_idx] = mac_oui_to_cipher_group(st_crypto.aul_pair_suite[uc_idx]);
                }
                for (uc_idx = 0; uc_idx < st_crypto.n_akm_suites; uc_idx++)
                {
                    st_ap_info.crypto.akm_suites[uc_idx] = mac_oui_to_akm_suite(st_crypto.aul_akm_suite[uc_idx]);
                }
            }
            else if (OAL_PTR_NULL != pst_bss_dscr->puc_rsn_ie)
            {
                mac_ie_get_rsn_cipher(pst_bss_dscr->puc_rsn_ie, &(st_crypto));
                st_ap_info.crypto.wpa_versions = WITP_WPA_VERSION_2;
                st_ap_info.crypto.cipher_group = mac_oui_to_cipher_group(st_crypto.ul_group_suite);
                st_ap_info.crypto.n_ciphers_pairwise = st_crypto.n_pair_suite;
                st_ap_info.crypto.n_akm_suites = st_crypto.n_akm_suites;
                for (uc_idx = 0; uc_idx < st_crypto.n_pair_suite; uc_idx++)
                {
                    st_ap_info.crypto.ciphers_pairwise[uc_idx] = mac_oui_to_cipher_group(st_crypto.aul_pair_suite[uc_idx]);
                }
                for (uc_idx = 0; uc_idx < st_crypto.n_akm_suites; uc_idx++)
                {
                    st_ap_info.crypto.akm_suites[uc_idx] = mac_oui_to_akm_suite(st_crypto.aul_akm_suite[uc_idx]);
                }
            }
            else if (OAL_PTR_NULL != pst_bss_dscr->puc_wpa_ie)
            {
                mac_ie_get_wpa_cipher(pst_bss_dscr->puc_wpa_ie, &st_crypto);
                st_ap_info.crypto.wpa_versions = WITP_WPA_VERSION_1;
                st_ap_info.crypto.cipher_group = mac_oui_to_cipher_group(st_crypto.ul_group_suite);
                st_ap_info.crypto.n_ciphers_pairwise = st_crypto.n_pair_suite;
                st_ap_info.crypto.n_akm_suites = st_crypto.n_akm_suites;
                for (uc_idx = 0; uc_idx < st_crypto.n_pair_suite; uc_idx++)
                {
                    st_ap_info.crypto.ciphers_pairwise[uc_idx] = mac_oui_to_cipher_group(st_crypto.aul_pair_suite[uc_idx]);
                }
                for (uc_idx = 0; uc_idx < st_crypto.n_akm_suites; uc_idx++)
                {
                    st_ap_info.crypto.akm_suites[uc_idx] = mac_oui_to_akm_suite(st_crypto.aul_akm_suite[uc_idx]);
                }
            }
            else
            {
                st_ap_info.auth_type = (enum nl80211_auth_type)NL80211_AUTHTYPE_SHARED_KEY;
            }
        }

        oal_copy_to_user((puc_param->pst_buf + us_temp), &st_ap_info, OAL_SIZEOF(oal_ap_info_stru));
        us_temp++;
        if (us_temp >= puc_param->uc_cnt)
        {
            break;
        }
    }

    /* 解除锁 */
    oal_spin_unlock(&(pst_bss_mgmt->st_lock));

    return OAL_SUCC;
}


oal_int32  hmac_config_get_hw_flow_stat(mac_vap_stru *pst_mac_vap, oal_machw_flow_stat_stru *puc_param)
{
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;
    hmac_vap_stru           *pst_hmac_vap;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_hw_stat::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->station_info_query_completed_flag = OAL_FALSE;
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_HW_FLOW_STAT, OAL_SIZEOF(oal_machw_flow_stat_stru), (oal_uint8*)puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_hw_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return OAL_EFAIL;
    }
    /*lint -e730*/
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q, OAL_TRUE == pst_hmac_vap->station_info_query_completed_flag, (5*OAL_TIME_HZ));
    /*lint +e730*/
    if (l_ret <= 0)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_hw_stat::OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT [%d].}", l_ret);
        return OAL_EFAIL;
    }

    puc_param->ul_rx_bytes = pst_hmac_vap->st_machw_stat.ul_rx_bytes;
    puc_param->ul_rx_discard = pst_hmac_vap->st_machw_stat.ul_rx_discard;
    puc_param->ul_rx_err = pst_hmac_vap->st_machw_stat.ul_rx_err;
    puc_param->ul_rx_pkts = pst_hmac_vap->st_machw_stat.ul_rx_pkts;
    puc_param->ul_tx_bytes = pst_hmac_vap->st_machw_stat.ul_tx_bytes;
    puc_param->ul_tx_discard = pst_hmac_vap->st_machw_stat.ul_tx_discard;
    puc_param->ul_tx_err = pst_hmac_vap->st_machw_stat.ul_tx_err;
    puc_param->ul_tx_pkts = pst_hmac_vap->st_machw_stat.ul_tx_pkts;
    puc_param->ul_tx_bcn_cnt = pst_hmac_vap->st_machw_stat.ul_tx_bcn_cnt;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_get_hw_flow_stat_rsp(mac_vap_stru *pst_mac_vap,  oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_machw_flow_stat_stru    *pst_machw_stat;
    hmac_vap_stru           *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_hw_flow_stat_rsp::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_machw_stat = (oal_machw_flow_stat_stru*)puc_param;
    pst_hmac_vap->st_machw_stat.ul_rx_bytes = pst_machw_stat->ul_rx_bytes;
    pst_hmac_vap->st_machw_stat.ul_rx_discard = pst_machw_stat->ul_rx_discard;
    pst_hmac_vap->st_machw_stat.ul_rx_err = pst_machw_stat->ul_rx_err;
    pst_hmac_vap->st_machw_stat.ul_rx_pkts = pst_machw_stat->ul_rx_pkts;
    pst_hmac_vap->st_machw_stat.ul_tx_bytes = pst_machw_stat->ul_tx_bytes;
    pst_hmac_vap->st_machw_stat.ul_tx_discard = pst_machw_stat->ul_tx_discard;
    pst_hmac_vap->st_machw_stat.ul_tx_err = pst_machw_stat->ul_tx_err;
    pst_hmac_vap->st_machw_stat.ul_tx_pkts = pst_machw_stat->ul_tx_pkts;
    pst_hmac_vap->st_machw_stat.ul_tx_bcn_cnt = pst_machw_stat->ul_tx_bcn_cnt;

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}


oal_int32  hmac_config_get_wme_stat(mac_vap_stru *pst_mac_vap, oal_wme_stat_stru *puc_param)
{
#ifdef _PRE_WLAN_11K_STAT
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;
    hmac_vap_stru           *pst_hmac_vap;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_wme_stat::pst_hmac_vap %d, puc_param %d.}", pst_hmac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->station_info_query_completed_flag = OAL_FALSE;
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_WME_STAT, OAL_SIZEOF(oal_wme_stat_stru), (oal_uint8*)puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wme_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return OAL_EFAIL;
    }
    /*lint -e730*/
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q, OAL_TRUE == pst_hmac_vap->station_info_query_completed_flag, (5*OAL_TIME_HZ));
    /*lint +e730*/
    if (l_ret <= 0)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wme_stat::OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT [%d].}", l_ret);
        return OAL_EFAIL;
    }

    oal_memcopy(puc_param, &(pst_hmac_vap->st_wme_stat), OAL_SIZEOF(oal_wme_stat_stru));

    return OAL_SUCC;
#else
    return OAL_EFAIL;
#endif
}


OAL_STATIC oal_uint32  hmac_config_get_wme_stat_rsp(mac_vap_stru *pst_mac_vap,  oal_uint8 uc_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_11K_STAT
    oal_wme_stat_stru *pst_wme_stat;
    hmac_vap_stru     *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_wme_stat_rsp::pst_hmac_vap %d, puc_param %d.}", pst_hmac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_wme_stat = (oal_wme_stat_stru*)puc_param;
    oal_memcopy(&(pst_hmac_vap->st_wme_stat), pst_wme_stat, OAL_SIZEOF(oal_wme_stat_stru));

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
#else
    return OAL_EFAIL;
#endif
}


OAL_STATIC oal_uint32  hmac_config_get_ant_rssi_rsp(mac_vap_stru *pst_mac_vap,  oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if defined _PRE_WLAN_PRODUCT_1151V200 && defined _PRE_WLAN_RX_DSCR_TRAILER
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_ANT_RSSI);
#else
    return OAL_SUCC;
#endif
}


OAL_STATIC oal_uint32  hmac_config_get_bg_noise_rsp(mac_vap_stru *pst_mac_vap,  oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_BG_NOISE);
}


oal_uint32  hmac_config_get_ant_rssi(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
#if defined _PRE_WLAN_PRODUCT_1151V200 && defined _PRE_WLAN_RX_DSCR_TRAILER
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_ANT_RSSI, OAL_SIZEOF(oal_uint8), puc_param);
#else
    *(oal_int32*)puc_param = 0;
    *pus_len = OAL_SIZEOF(oal_int32);
    return OAL_SUCC;
#endif
}


oal_uint32  hmac_config_ant_rssi_report(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_SCAN, "{hmac_config_ant_rssi_report::mac_vap pointer null}");
        return OAL_ERR_CODE_PTR_NULL;
    }


    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ANT_RSSI_REPORT, us_len,  puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ant_rssi_report::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_11K_STAT

oal_uint32 *hmac_config_get_sta_drop_num(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 *puc_param)
{
    hmac_user_stru             *pst_hmac_user;
    mac_cfg_query_drop_num_stru st_query_drop_num_param;
    hmac_vap_stru              *pst_hmac_vap;
    oal_uint32                 *pst_value;
    oal_int32                   l_ret;

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, puc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_drop_num::pst_hmac_user null.}");
        return OAL_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_drop_num::pst_hmac_vap null.}");
        return OAL_PTR_NULL;
    }

    st_query_drop_num_param.us_user_id = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_hmac_vap->station_info_query_completed_flag = OAL_FALSE;
    hmac_config_query_drop_num(pst_mac_vap , (oal_uint16)OAL_SIZEOF(st_query_drop_num_param), (oal_uint8 *)&st_query_drop_num_param);
    /*lint -e730*/
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q, OAL_TRUE == pst_hmac_vap->station_info_query_completed_flag, (5*OAL_TIME_HZ));
    /*lint +e730*/
    if (l_ret <= 0)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wme_stat::OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT [%d].}", l_ret);
        return OAL_PTR_NULL;
    }
    pst_value = pst_hmac_user->aul_drop_num;

    return pst_value;
}


oal_uint32 *hmac_config_get_sta_tx_delay(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint8 *puc_param)
{
    hmac_user_stru             *pst_hmac_user;
    mac_cfg_query_tx_delay_stru st_query_tx_delay_param;
    hmac_vap_stru              *pst_hmac_vap;
    oal_uint32                 *pst_value;
    oal_int32                   l_ret;

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, puc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_tx_delay::pst_hmac_user null.}");
        return OAL_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_tx_delay::pst_hmac_vap null.}");
        return OAL_PTR_NULL;
    }

    st_query_tx_delay_param.us_user_id = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_hmac_vap->station_info_query_completed_flag = OAL_FALSE;
    hmac_config_query_tx_delay(pst_mac_vap , (oal_uint16)OAL_SIZEOF(st_query_tx_delay_param), (oal_uint8 *)&st_query_tx_delay_param);
    /*lint -e730*/
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q, OAL_TRUE == pst_hmac_vap->station_info_query_completed_flag, (5*OAL_TIME_HZ));
    /*lint +e730*/
    if (l_ret <= 0)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wme_stat::OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT [%d].}", l_ret);
        return OAL_PTR_NULL;
    }
    pst_value = pst_hmac_user->aul_tx_delay;

    return pst_value;
}


oal_int32  hmac_config_get_tx_delay_ac(mac_vap_stru *pst_mac_vap, oal_tx_delay_ac_stru *puc_param)
{
    oal_uint32                  ul_ret;
    oal_int32                   l_ret;
    hmac_vap_stru           *pst_hmac_vap;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_tx_delay_ac::pst_hmac_vap %d, puc_param %d.}", pst_hmac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->station_info_query_completed_flag = OAL_FALSE;
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_TX_DELAY_AC, OAL_SIZEOF(oal_tx_delay_ac_stru), (oal_uint8*)puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_tx_delay_ac::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return OAL_EFAIL;
    }
    /*lint -e730*/
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q, OAL_TRUE == pst_hmac_vap->station_info_query_completed_flag, (5*OAL_TIME_HZ));
    /*lint +e730*/
    if (l_ret <= 0)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_tx_delay_ac::OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT [%d].}", l_ret);
        return OAL_EFAIL;
    }

    oal_memcopy(puc_param, &(pst_hmac_vap->st_tx_delay_ac), OAL_SIZEOF(oal_tx_delay_ac_stru));

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_get_tx_delay_ac_rsp(mac_vap_stru *pst_mac_vap,  oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_tx_delay_ac_stru    *pst_tx_delay_ac;
    hmac_vap_stru           *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_tx_delay_ac_rsp::pst_hmac_vap %d, puc_param %d.}", pst_hmac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tx_delay_ac = (oal_tx_delay_ac_stru*)puc_param;
    oal_memcopy(&(pst_hmac_vap->st_tx_delay_ac), pst_tx_delay_ac, OAL_SIZEOF(oal_tx_delay_ac_stru));

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DFS

oal_uint32  hmac_config_get_dfs_chn_status(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_mac_device;
    oal_uint32                  ul_err_code;
    oal_uint8                   uc_chan_idx;
    oal_uint8                   uc_channel;
    oal_uint8                   uc_ret = OAL_SUCC;

    uc_channel = *((oal_uint8*)puc_param);

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_device_id, OAM_SF_DFS, "{hmac_config_get_dfs_chn_status::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 如果配置的信道被雷达占用，不可配置 */
    ul_err_code = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, uc_channel, &uc_chan_idx);

    if (OAL_ERR_CODE_INVALID_CONFIG != ul_err_code &&
        MAC_CHAN_BLOCK_DUE_TO_RADAR  == pst_mac_device->st_ap_channel_list[uc_chan_idx].en_ch_status)
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "hmac_config_get_dfs_chn_status::The channel %d is blocked by radar.\n", uc_channel);
        uc_ret = 1;
    }
    else
    {
        OAM_WARNING_LOG1(0, OAM_SF_DFS, "hmac_config_get_dfs_chn_status::The channel %d is available.\n", uc_channel);

    }

    return uc_ret;

}
#endif

#endif

#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)

oal_int32  hmac_config_get_snoop_table(mac_vap_stru *pst_mac_vap, oal_snoop_all_group_stru *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_get_snoop_table::pst_mac_vap or puc_param null ptr error %d,%d.}\r\n", pst_mac_vap, puc_param);
        return OAL_EFAUL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_get_snoop_table::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    return (oal_int32)hmac_m2u_print_all_snoop_list(pst_hmac_vap, puc_param);
}
#endif


oal_uint32  hmac_config_get_ssid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_ssid_etc(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_set_ssid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /* 设置mib值 */
    mac_mib_set_ssid_etc(pst_mac_vap, (oal_uint8)us_len, puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) /*hi1102-cb set at both side (HMAC to DMAC) */
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SSID, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}


oal_uint32  hmac_config_set_shpreamble_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /* 设置mib值 */
    mac_mib_set_shpreamble_etc(pst_mac_vap, (oal_uint8)us_len, puc_param);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SHORT_PREAMBLE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_concurrent::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_shpreamble_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* 读mib值 */
    return mac_mib_get_shpreamble_etc(pst_mac_vap, (oal_uint8 * )pus_len, puc_param);
}


oal_uint32  hmac_config_set_shortgi20_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32           l_value;
    oal_uint32          ul_ret = 0;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    shortgi_cfg_stru    shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_20_CFG_ENUM;
#endif
    l_value = *((oal_int32 *)puc_param);

    if (0 != l_value)
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_TRUE;
#endif
        mac_mib_set_ShortGIOptionInTwentyImplemented(pst_mac_vap, OAL_TRUE);
    }
    else
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_FALSE;
#endif
        mac_mib_set_ShortGIOptionInTwentyImplemented(pst_mac_vap, OAL_FALSE);
    }

    /*========================================================================*/
    /* hi1102-cb : Need to send to Dmac via sdio */
    #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* 配置事件的子事件 WLAN_CFGID_SHORTGI 通过新加的接口函数取出关键数据存入skb后通过sdio发出 */
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (oal_uint8 *)&shortgi_cfg);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_shortgi20_etc::hmac_config_send_event_etc failed[%u].}", ul_ret);
    }
    #endif
    /*========================================================================*/

    return ul_ret;
}


oal_uint32  hmac_config_set_shortgi40_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;
    oal_uint32      ul_ret = OAL_SUCC;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    shortgi_cfg_stru    shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_40_CFG_ENUM;
#endif
    l_value = *((oal_int32 *)puc_param);

    if (0 != l_value)
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_TRUE;
#endif
        mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_TRUE);
    }
    else
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_FALSE;
#endif
        mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_FALSE);
    }

    /*========================================================================*/
    /* hi1102-cb : Need to send to Dmac via sdio */
    #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* 配置事件的子事件 WLAN_CFGID_SHORTGI 通过新加的接口函数取出关键数据存入skb后通过sdio发出 */
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (oal_uint8 *)&shortgi_cfg);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_shortgi40_etc::hmac_config_send_event_etc failed[%u].}", ul_ret);
    }
    #endif
    /*========================================================================*/

    return ul_ret;
}


oal_uint32  hmac_config_set_shortgi80_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;
    oal_uint32      ul_ret = 0;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    shortgi_cfg_stru    shortgi_cfg;

    shortgi_cfg.uc_shortgi_type = SHORTGI_40_CFG_ENUM;
#endif

    l_value = *((oal_int32 *)puc_param);

    if (0 != l_value)
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_TRUE;
#endif
        mac_mib_set_VHTShortGIOptionIn80Implemented(pst_mac_vap, OAL_TRUE);
    }
    else
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        shortgi_cfg.uc_enable = OAL_FALSE;
#endif
        mac_mib_set_VHTShortGIOptionIn80Implemented(pst_mac_vap, OAL_FALSE);
    }

    /*========================================================================*/
    /* hi1102-cb : Need to send to Dmac via sdio */
    #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* 配置事件的子事件 WLAN_CFGID_SHORTGI 通过新加的接口函数取出关键数据存入skb后通过sdio发出 */
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SHORTGI, SHORTGI_CFG_STRU_LEN, (oal_uint8 *)&shortgi_cfg);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_shortgi80_etc::hmac_config_send_event_etc failed[%u].}", ul_ret);
    }
    #endif
    /*========================================================================*/

    return ul_ret;
}


oal_uint32  hmac_config_get_shortgi20_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = mac_mib_get_ShortGIOptionInTwentyImplemented(pst_mac_vap);

    *((oal_int32 *)puc_param) = l_value;

    *pus_len = OAL_SIZEOF(l_value);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_shortgi40_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = (oal_int32)mac_mib_get_ShortGIOptionInFortyImplemented(pst_mac_vap);

    *((oal_int32 *)puc_param) = l_value;

    *pus_len = OAL_SIZEOF(l_value);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_shortgi80_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_int32       l_value;

    l_value = mac_mib_get_VHTShortGIOptionIn80Implemented(pst_mac_vap);

    *((oal_int32 *)puc_param) = l_value;

    *pus_len = OAL_SIZEOF(l_value);

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_MONITOR

oal_uint32  hmac_config_set_addr_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;
    oal_uint32                   ul_ret;
    oal_int32                    l_value;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_addr_filter::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_int32 *)puc_param);

    pst_hmac_vap->en_addr_filter = (oal_uint8)l_value;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ADDR_FILTER, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_addr_filter::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

oal_uint32  hmac_config_get_addr_filter_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_addr_filter::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = pst_hmac_vap->en_addr_filter;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_prot_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;

    l_value = *((oal_int32 *)puc_param);

    if (OAL_UNLIKELY(l_value >= WLAN_PROT_BUTT))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_prot_mode_etc::invalid l_value[%d].}", l_value);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_mac_vap->st_protection.en_protection_mode = (oal_uint8)l_value;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PROT_MODE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_prot_mode_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_prot_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_int32 *)puc_param) = pst_mac_vap->st_protection.en_protection_mode;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_auth_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_mib_set_AuthenticationMode(pst_mac_vap, *puc_param);

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_auth_mode_etc::set auth mode[%d] succ.}", mac_mib_get_AuthenticationMode(pst_mac_vap));
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_auth_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_int32 *)puc_param) = mac_mib_get_AuthenticationMode(pst_mac_vap);
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_max_user_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint32 ul_max_user)
{
    /* P2P GO最大用户数不能超过p2p限制，普通模式不能超过芯片最大用户数约束 */
    if((IS_P2P_GO(pst_mac_vap) && (ul_max_user > WLAN_P2P_GO_ASSOC_USER_MAX_NUM_SPEC))
        || (ul_max_user > mac_chip_get_max_asoc_user(pst_mac_vap->uc_chip_id)))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_max_user_etc:: AP/GO want to set the max_user_value[%d] more than the threshold[%d]!}\r\n",
                ul_max_user, (IS_P2P_GO(pst_mac_vap) ? WLAN_P2P_GO_ASSOC_USER_MAX_NUM_SPEC : mac_chip_get_max_asoc_user(pst_mac_vap->uc_chip_id)));
        return OAL_FAIL;
    }

    mac_mib_set_MaxAssocUserNums(pst_mac_vap, (oal_uint16)ul_max_user);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_max_user_etc::chip us_user_num_max[%d], us_user_nums_max[%d].}", mac_chip_get_max_asoc_user(pst_mac_vap->uc_chip_id), mac_mib_get_MaxAssocUserNums(pst_mac_vap));

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_bintval_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_device_stru             *pst_mac_device;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru*               pst_vap;

    pst_mac_device              = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_bintval_etc::mac_res_get_dev_etc fail.device_id = %u}",
                           pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置device下的值*/
#if 0
    pst_mac_device->ul_beacon_interval  = *((oal_uint32 *)puc_param);
#else
    mac_device_set_beacon_interval_etc(pst_mac_device, *((oal_uint32 *)puc_param));
#endif
    /* 遍历device下所有vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_bintval_etc::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }

        /* 只有AP VAP需要beacon interval */
        if ((WLAN_VAP_MODE_BSS_AP == pst_vap->en_vap_mode))
        {
             /* 设置mib值 */
            mac_mib_set_beacon_period_etc(pst_vap, (oal_uint8)us_len, puc_param);
        }
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_BEACON_INTERVAL, us_len,  puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_bintval_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_bintval_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_beacon_period_etc(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_set_dtimperiod_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
#ifdef _PRE_WLAN_WEB_CMD_COMM
    mac_device_stru             *pst_mac_device;
    oal_uint8                   uc_vap_idx;
    mac_vap_stru*               pst_vap;

    pst_mac_device              = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_dtimperiod_etc::mac_res_get_dev_etc fail.device_id = %u}",
                           pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 遍历device下所有vap */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_dtimperiod_etc::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }
        /* 设置mib值 */
        mac_mib_set_dtim_period_etc(pst_vap, (oal_uint8)us_len, puc_param);
    }
#else
    /* 设置mib值 */
    mac_mib_set_dtim_period_etc(pst_mac_vap, (oal_uint8)us_len, puc_param);
#endif

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DTIM_PERIOD, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_bintval_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_dtimperiod_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    /* 读取mib值 */
    return mac_mib_get_dtim_period_etc(pst_mac_vap, (oal_uint8 *)pus_len, puc_param);
}


oal_uint32  hmac_config_set_nobeacon_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;
    oal_uint32                   ul_ret;
    oal_int32                    l_value;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_nobeacon_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    l_value = *((oal_int32 *)puc_param);
    pst_hmac_vap->en_no_beacon = (oal_uint8)l_value;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_NO_BEACON, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_nobeacon_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_nobeacon_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_nobeacon_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *((oal_int32 *)puc_param) = pst_hmac_vap->en_no_beacon;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_txpower_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32                       l_value;
    oal_uint8                       uc_value;
    oal_uint32                      ul_ret = OAL_SUCC;
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT) /* ONT产品软件版本号 */
    mac_device_stru                 *pst_mac_device;
    oal_uint8                       uc_vap_idx;
    mac_vap_stru                    *pst_vap;
#endif
    l_value = (*((oal_int32 *)puc_param) < 0) ? 0 : (*((oal_int32 *)puc_param));
    if (!l_value)
    {
        return ul_ret;
    }
    /* 四舍五入 */
    uc_value = (oal_uint8)((l_value+5)/10);
#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT) /* ONT产品软件版本号 */
    /*  遍历device下所有的vap */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_txpower_etc::pst_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_set_txpower_etc::pst_ap(%d) null.}",
                pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }
        mac_vap_set_tx_power_etc(pst_vap, uc_value);
    }
#else
    mac_vap_set_tx_power_etc(pst_mac_vap, uc_value);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TX_POWER, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_txpower_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#else
#endif

    return ul_ret;
}


oal_uint32  hmac_config_get_txpower_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_int32 *)puc_param) = pst_mac_vap->uc_tx_power;
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_freq_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                           ul_ret;
    oal_uint8                            uc_channel = *puc_param;
    mac_device_stru                     *pst_mac_device;
#if 0
#ifdef _PRE_WLAN_FEATURE_DFS
    wlan_channel_bandwidth_enum_uint8    en_bandwidth = WLAN_BAND_WIDTH_20M;
    mac_cfg_channel_param_stru           st_channel_param;
#endif
#endif
    mac_cfg_channel_param_stru l_channel_param;

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq_etc::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_is_channel_num_valid_etc(pst_mac_vap->st_channel.en_band, uc_channel);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq_etc::mac_is_channel_num_valid_etc[%d] failed[%d].}", uc_channel, ul_ret);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

#ifdef _PRE_WLAN_FEATURE_11D
    /* 信道14特殊处理，只在11b协议模式下有效 */
    if ((14 == uc_channel) && (WLAN_LEGACY_11B_MODE != pst_mac_vap->en_protocol))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_freq_etc::channel-14 only available in 11b, curr protocol=%d.}", pst_mac_vap->en_protocol);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
#endif
/*  gaolin: CAC在初始检查时统一执行 */
#if 0
#ifdef _PRE_WLAN_FEATURE_DFS
    if (OAL_TRUE == mac_dfs_get_dfs_enable(pst_mac_device))
    {
        ul_ret = hmac_dfs_recalculate_channel_etc(pst_mac_device, (oal_uint8 *)&ul_freq, &en_bandwidth);
        if (OAL_TRUE == ul_ret)
        {
            *((oal_uint32 *)puc_param) = ul_freq;
        }

        st_channel_param.en_band = pst_mac_device->en_max_band;
        st_channel_param.en_bandwidth = en_bandwidth;
        st_channel_param.uc_channel = (oal_uint8)ul_freq;

        hmac_config_set_channel_etc(pst_mac_vap, OAL_SIZEOF(mac_cfg_channel_param_stru), (oal_uint8 *)&st_channel_param);
    }
#endif
#endif

    pst_mac_vap->st_channel.uc_chan_number = uc_channel;
    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band, uc_channel, &(pst_mac_vap->st_channel.uc_chan_idx));
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id,OAM_SF_CFG,"{hmac_config_set_freq_etc::mac_get_channel_idx_from_num_etc fail.band[%u] channel[%u]!}",
            pst_mac_vap->st_channel.en_band,pst_mac_vap->st_channel.uc_chan_idx);
        return ul_ret;
    }

    /* 非DBAC时，首次配置信道时设置到硬件 */
    if (1 == pst_mac_device->uc_vap_num || 0 == pst_mac_device->uc_max_channel)
    {
#if 0
        pst_mac_device->uc_max_channel = (oal_uint8)ul_freq;
#else
        mac_device_get_channel_etc(pst_mac_device, &l_channel_param);
        l_channel_param.uc_channel = uc_channel;
        mac_device_set_channel_etc(pst_mac_device, &l_channel_param);
#endif

        /***************************************************************************
            抛事件到DMAC层, 同步DMAC数据
        ***************************************************************************/
        ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CURRENT_CHANEL, us_len, puc_param);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }
    }
#ifdef _PRE_WLAN_FEATURE_DBAC
    else if(mac_is_dbac_enabled(pst_mac_device))
    {
         /***************************************************************************
            抛事件到DMAC层, 同步DMAC数据
        ***************************************************************************/
        ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CURRENT_CHANEL, us_len, puc_param);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }

        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq_etc::do not check channel while DBAC enabled.}");
    }
#endif
    else
    {
        if (pst_mac_device->uc_max_channel != uc_channel)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq_etc::previous vap channel number=%d mismatch [%d].}",
                             pst_mac_device->uc_max_channel, uc_channel);

            return OAL_FAIL;
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_freq_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *pus_len = OAL_SIZEOF(oal_uint32);

#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
    {
        hmac_device_stru *pst_hmac_dev = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
        if (pst_hmac_dev && pst_hmac_dev->en_in_init_scan)
        {
            *((oal_uint32 *)puc_param) = 0;
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_freq_etc::get channel while init scan, retry please}");
            return OAL_SUCC;
        }
    }
#endif
    *((oal_uint32 *)puc_param) = pst_mac_vap->st_channel.uc_chan_number;


    return OAL_SUCC;
}


oal_uint32  hmac_config_set_wmm_params_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                  ul_ac;
    oal_uint32                  ul_value;
    wlan_cfgid_enum_uint16      en_cfg_id;
    hmac_config_wmm_para_stru  *pst_cfg_stru;

    pst_cfg_stru = (hmac_config_wmm_para_stru *)puc_param;
    en_cfg_id    = (oal_uint16)pst_cfg_stru->ul_cfg_id;
    ul_ac        = pst_cfg_stru->ul_ac;
    ul_value     = pst_cfg_stru->ul_value;

    ul_ret = OAL_SUCC;

    if (ul_ac >= WLAN_WME_AC_BUTT)
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wmm_params_etc::invalid param,en_cfg_id=%d, ul_ac=%d, ul_value=%d.}",
                         en_cfg_id, ul_ac, ul_value);
        return OAL_FAIL;
    }

    switch (en_cfg_id)                                                      /* 根据sub-ioctl id填写WID */
    {
        case WLAN_CFGID_EDCA_TABLE_CWMIN:
/*lint -e685*//*lint -e568*/
            if ((ul_value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMIN_MIN))
            {
                ul_ret = OAL_FAIL;
            }
            mac_mib_set_EDCATableCWmin(pst_mac_vap, (oal_uint8)ul_ac, ul_value);

            break;

        case WLAN_CFGID_EDCA_TABLE_CWMAX:
            if ((ul_value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMAX_MIN))
            {
                ul_ret = OAL_FAIL;
            }

            mac_mib_set_EDCATableCWmax(pst_mac_vap, (oal_uint8)ul_ac, ul_value);

            break;

        case WLAN_CFGID_EDCA_TABLE_AIFSN:
            if ((ul_value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (ul_value > WLAN_QEDCA_TABLE_AIFSN_MAX))
            {
                ul_ret = OAL_FAIL;
            }
/*lint +e685*//*lint +e568*/
            mac_mib_set_EDCATableAIFSN(pst_mac_vap, (oal_uint8)ul_ac, ul_value);

            break;

        case WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT:
            if (ul_value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX)
            {
                ul_ret = OAL_FAIL;
            }

            mac_mib_set_EDCATableTXOPLimit(pst_mac_vap, (oal_uint8)ul_ac, ul_value);
            break;
#if 0

        case WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME:
            if (ul_value > WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX)
            {
                ul_ret = OAL_FAIL;
            }
            pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableMSDULifetime = ul_value;


            break;
#endif
        case WLAN_CFGID_EDCA_TABLE_MANDATORY:
            if ((OAL_TRUE != ul_value) &&  (OAL_FALSE != ul_value))
            {
                ul_ret = OAL_FAIL;
            }
            mac_mib_set_EDCATableMandatory(pst_mac_vap, (oal_uint8)ul_ac, (oal_uint8)ul_value);

            break;


        case WLAN_CFGID_QEDCA_TABLE_CWMIN:
/*lint -e685*//*lint -e568*/
            if ((ul_value > WLAN_QEDCA_TABLE_CWMIN_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMIN_MIN))
            {
                ul_ret = OAL_FAIL;
            }
            mac_mib_set_QAPEDCATableCWmin(pst_mac_vap, (oal_uint8)ul_ac, ul_value);
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMAX:
            if ((ul_value > WLAN_QEDCA_TABLE_CWMAX_MAX) || (ul_value < WLAN_QEDCA_TABLE_CWMAX_MIN))
            {
                ul_ret = OAL_FAIL;
            }

            mac_mib_set_QAPEDCATableCWmax(pst_mac_vap, (oal_uint8)ul_ac, ul_value);

            break;

        case WLAN_CFGID_QEDCA_TABLE_AIFSN:
            if ((ul_value < WLAN_QEDCA_TABLE_AIFSN_MIN) || (ul_value > WLAN_QEDCA_TABLE_AIFSN_MAX))
            {
                ul_ret = OAL_FAIL;
            }
/*lint +e685*//*lint +e568*/

            mac_mib_set_QAPEDCATableAIFSN(pst_mac_vap, (oal_uint8)ul_ac, ul_value);

            break;

        case WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT:
            if (ul_value > WLAN_QEDCA_TABLE_TXOP_LIMIT_MAX)
            {
                ul_ret = OAL_FAIL;
            }

            mac_mib_set_QAPEDCATableTXOPLimit(pst_mac_vap, (oal_uint8)ul_ac, ul_value);
            break;

        case WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME:
            if (ul_value > WLAN_QEDCA_TABLE_MSDU_LIFETIME_MAX)
            {
                ul_ret = OAL_FAIL;
            }
            mac_mib_set_QAPEDCATableMSDULifetime(pst_mac_vap, (oal_uint8)ul_ac, ul_value);
            break;

        case WLAN_CFGID_QEDCA_TABLE_MANDATORY:
            if ((OAL_TRUE != ul_value) &&  (OAL_FALSE != ul_value))
            {
                ul_ret = OAL_FAIL;
            }

            mac_mib_set_QAPEDCATableMandatory(pst_mac_vap, (oal_uint8)ul_ac, (oal_uint8)ul_value);
            break;

        default:
            ul_ret = OAL_FAIL;
            break;
    }

    if (OAL_FAIL == ul_ret)
    {
        return ul_ret;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, en_cfg_id, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wmm_params_etc::hmac_config_send_event_etc failed[%d].}",ul_ret);
    }

    return ul_ret;

}



oal_uint32  hmac_config_get_wmm_params_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ac;
    oal_uint32                  ul_value;
    wlan_cfgid_enum_uint16      en_cfg_id;
    hmac_config_wmm_para_stru  *pst_cfg_stru;

    ul_value       = 0xFFFFFFFF;
    pst_cfg_stru = (hmac_config_wmm_para_stru *)puc_param;
    en_cfg_id    = (oal_uint16)pst_cfg_stru->ul_cfg_id;
    ul_ac        = pst_cfg_stru->ul_ac;

    if (ul_ac >= WLAN_WME_AC_BUTT)
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wmm_params_etc::invalid param,en_cfg_id=%d, ul_ac=%d, ul_value=%d.}",
                         en_cfg_id, ul_ac, ul_value);
        return ul_value;
    }

    switch (en_cfg_id)                                                      /* 根据sub-ioctl id填写WID */
    {
        case WLAN_CFGID_EDCA_TABLE_CWMIN:
            ul_value = mac_mib_get_EDCATableCWmin(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_EDCA_TABLE_CWMAX:
            ul_value = mac_mib_get_EDCATableCWmax(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_EDCA_TABLE_AIFSN:
            ul_value = mac_mib_get_EDCATableAIFSN(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_EDCA_TABLE_TXOP_LIMIT:
            ul_value = mac_mib_get_EDCATableTXOPLimit(pst_mac_vap, (oal_uint8)ul_ac);
            break;
#if 0
        case WLAN_CFGID_EDCA_TABLE_MSDU_LIFETIME:
            ul_value = pst_mac_vap->pst_mib_info->ast_wlan_mib_edca[ul_ac].ul_dot11EDCATableMSDULifetime;
            break;
#endif
        case WLAN_CFGID_EDCA_TABLE_MANDATORY:
            ul_value = mac_mib_get_EDCATableMandatory(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMIN:
            ul_value = mac_mib_get_QAPEDCATableCWmin(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_QEDCA_TABLE_CWMAX:
            ul_value = mac_mib_get_QAPEDCATableCWmax(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_QEDCA_TABLE_AIFSN:
            ul_value = mac_mib_get_QAPEDCATableAIFSN(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_QEDCA_TABLE_TXOP_LIMIT:
            ul_value = mac_mib_get_QAPEDCATableTXOPLimit(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_QEDCA_TABLE_MSDU_LIFETIME:
            ul_value = mac_mib_get_QAPEDCATableMSDULifetime(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        case WLAN_CFGID_QEDCA_TABLE_MANDATORY:
            ul_value = mac_mib_get_QAPEDCATableMandatory(pst_mac_vap, (oal_uint8)ul_ac);
            break;

        default:
            break;
    }

    return ul_value;
}

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST

oal_uint32  hmac_config_chip_check(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CHIP_CHECK_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_chip_check::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}


oal_uint32  hmac_config_get_cali_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_CALI_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_cali_info::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
oal_uint32  hmac_config_set_vap_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_cfg_smps_mode_stru      st_smps_mode = {0};
    mac_device_stru             *pst_mac_device;

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{mac_vap_get_smps_en::pst_mac_device[%d] null.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_TRUE != mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_config_set_vap_smps_mode:: not support HT.}");
        return OAL_FAIL;
    }

    st_smps_mode.en_smps_mode = (oal_uint8)*((oal_uint32*)puc_param);

    if(st_smps_mode.en_smps_mode < WLAN_MIB_MIMO_POWER_SAVE_STATIC || st_smps_mode.en_smps_mode > WLAN_MIB_MIMO_POWER_SAVE_MIMO)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_config_set_vap_smps_mode::en_smps_mode[%d] beyond scope.}", st_smps_mode.en_smps_mode);
        return OAL_FAIL;
    }

    if(st_smps_mode.en_smps_mode > pst_mac_device->en_mac_smps_mode)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_config_set_vap_smps_mode:: vap mode[%d] beyond device smps mode[%d].}",
                   st_smps_mode.en_smps_mode, pst_mac_device->en_mac_smps_mode);
        return OAL_FAIL;
    }

    pst_mac_vap->pst_mib_info->st_wlan_mib_ht_sta_cfg.en_dot11MIMOPowerSave = st_smps_mode.en_smps_mode;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_VAP_SMPS, OAL_SIZEOF(mac_cfg_smps_mode_stru), (oal_uint8 *)&st_smps_mode);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_config_set_vap_smps_mode::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_smps_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru                      *pst_mac_device;
    mac_vap_stru                         *pst_mac_vap_tmp;
    mac_cfg_smps_mode_stru                st_smps_mode;
    wlan_mib_mimo_power_save_enum_uint8   en_smps_mode;
    wlan_nss_enum_uint8                   en_avail_num_spatial_stream;
    oal_uint8                             uc_vap_idx;
    oal_uint32                            ul_ret;

    st_smps_mode.en_smps_mode = *((wlan_mib_mimo_power_save_enum_uint8*)puc_param);
    en_smps_mode = st_smps_mode.en_smps_mode;
    if(en_smps_mode < WLAN_MIB_MIMO_POWER_SAVE_STATIC || en_smps_mode > WLAN_MIB_MIMO_POWER_SAVE_MIMO)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_config_set_smps_mode::en_smps_mode[%d] beyond scope.}", en_smps_mode);
        return OAL_FAIL;
    }

    /* 获取device,pst_mac_vap为配置vap */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_config_set_smps_mode::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 确认device是否可以配置新的smps mode */
    ul_ret = mac_device_find_smps_mode_en(pst_mac_device, en_smps_mode);
    if (OAL_TRUE != ul_ret)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_SMPS, "{hmac_config_set_smps_mode::device not support changing smps mode.}");
        return OAL_FAIL;
    }

    /* 根据SMPS mode确认采用单流还是双流来发送，通知算法 */
    en_avail_num_spatial_stream = (WLAN_MIB_MIMO_POWER_SAVE_STATIC == en_smps_mode)? WLAN_SINGLE_NSS: WLAN_DOUBLE_NSS;

    /* 遍历device下所有业务vap，刷新所有vap的SMPS能力 */
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap_tmp = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap_tmp)
        {
            continue;
        }

        if (OAL_TRUE != mac_mib_get_HighThroughputOptionImplemented(pst_mac_vap_tmp))
        {
            continue;
        }

        /* device能力变化，需要修改vap的空间流能力 */
        /* 改变vap下空间流个数能力(给新接入user使用) */
        mac_vap_set_rx_nss_etc(pst_mac_vap_tmp, en_avail_num_spatial_stream);

        /* 设置mib项 */
        mac_vap_set_smps(pst_mac_vap_tmp, en_smps_mode);
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_SMPS, OAL_SIZEOF(mac_cfg_smps_mode_stru), (oal_uint8 *)&st_smps_mode);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_smps_mode::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#if 0

oal_uint32  hmac_config_get_smps_mode_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32     ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_SMPS_EN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_smps_mode_en::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD

oal_uint32  hmac_config_set_uapsden_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /* 设置mib值 */
    mac_vap_set_uapsd_en_etc(pst_mac_vap, *puc_param);
    g_uc_uapsd_cap_etc = *puc_param;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_UAPSD_EN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_uapsden_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_uapsden_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *puc_param = mac_vap_get_uapsd_en_etc(pst_mac_vap);
    *pus_len   = OAL_SIZEOF(oal_uint8);

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_DFT_STAT

oal_uint32  hmac_config_set_phy_stat_en(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PHY_STAT_EN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_phy_stat_en::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dbb_env_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DBB_ENV_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dbb_env_param::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_usr_queue_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USR_QUEUE_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_usr_queue_stat_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
#else
    return OAL_SUCC;
#endif
}


oal_uint32  hmac_config_report_vap_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
#if 0
    if (OAL_FALSE == *puc_param)
    {
        /* 停止上报，并清除资源 */
        dmac_dft_stop_report_vap_stat(pst_mac_vap);
    }
    else
    {
        /* 开始统计，并周期上报,周期2s */
        dmac_dft_start_report_vap_stat(pst_mac_vap);
    }
#endif
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_VAP_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_report_vap_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_report_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ALL_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_report_all_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


#endif

#ifdef _PRE_WLAN_FEATURE_DFR

#ifdef _PRE_DEBUG_MODE

oal_uint32  hmac_config_dfr_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGIG_DFR_ENABLE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dfr_enable::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_trig_pcie_reset(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TRIG_PCIE_RESET, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_trig_pcie_reset::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_trig_loss_tx_comp(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TRIG_LOSS_TX_COMP, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_trig_loss_tx_comp::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif
#endif


oal_uint32  hmac_config_dscp_map_to_tid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;
    mac_map_dscp_to_tid_stru    *pst_map_dscp_to_tid_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_map_dscp_to_tid_param = (mac_map_dscp_to_tid_stru*)puc_param;
    if (OAL_PTR_NULL == puc_param)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_info_by_mac::puc_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap->auc_dscp_tid_map[pst_map_dscp_to_tid_param->uc_dscp] = pst_map_dscp_to_tid_param->uc_tid;

    return OAL_SUCC;
}


oal_uint32  hmac_config_clean_dscp_tid_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_clean_dscp_tid_map::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memset(pst_hmac_vap->auc_dscp_tid_map, HMAC_DSCP_VALUE_INVALID, HMAC_MAX_DSCP_VALUE_NUM);
    OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_clean_dscp_tid_map::dscp_tid_map is cleaned\r\n.}");
    return OAL_SUCC;
}


oal_uint32  hmac_config_reset_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_RESET_HW, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_reset_hw::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_reset_state_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret = OAL_SUCC;
    mac_reset_sys_stru  *pst_reset_sys;
    hmac_device_stru    *pst_hmac_device;

    pst_reset_sys = (mac_reset_sys_stru *)puc_param;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_set_reset_state_etc::pst_hmac_device[%d] is null.}",pst_mac_vap->uc_device_id);
        return OAL_PTR_NULL;
    }
    mac_device_set_dfr_reset_etc(pst_hmac_device->pst_device_base_info, pst_reset_sys->uc_value);

    /*待整改，当前存在DMAC TO HMAC SYNC，待处理。处理后做抛事件处理*/

    return ul_ret;
}



oal_uint32  hmac_config_dump_rx_dscr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DUMP_RX_DSCR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dump_rx_dscr_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dump_tx_dscr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DUMP_TX_DSCR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dump_tx_dscr_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

OAL_STATIC oal_uint32  hmac_config_set_channel_check_param(mac_vap_stru *pst_mac_vap, mac_cfg_channel_param_stru *pst_prot_param)
{
    mac_device_stru   *pst_mac_device;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_set_channel_check_param::pst_mac_device null,divice_id=%d.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 根据device能力对参数进行检查 */
    if ((pst_prot_param->en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) && (mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap) < WLAN_BW_CAP_80M))
    {
        /* 设置80M带宽，但device能力不支持80M，返回错误码 */
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_channel_check_param::not support 80MHz bandwidth,en_protocol=%d en_dot11VapMaxBandWidth=%d.}",
                         pst_prot_param->en_bandwidth, mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap));
        return OAL_ERR_CODE_CONFIG_BW_EXCEED;
    }

    if ((WLAN_BAND_5G == pst_prot_param->en_band) && (WLAN_BAND_CAP_2G == pst_mac_device->en_band_cap))
    {
        /* 设置5G频带，但device不支持5G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_channel_check_param::not support 5GHz band,en_protocol=%d en_protocol_cap=%d.}",
                         pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }
    else if ((WLAN_BAND_2G == pst_prot_param->en_band) && (WLAN_BAND_CAP_5G == pst_mac_device->en_band_cap))
    {
        /* 设置2G频带，但device不支持2G */
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_channel_check_param::not support 2GHz band,en_protocol=%d en_protocol_cap=%d.}",
                         pst_prot_param->en_band, pst_mac_device->en_band_cap);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

    /* 若vap不支持2g 40M则，返回不支持该带宽的错误码 */
    if((WLAN_BAND_2G == pst_prot_param->en_band) && (WLAN_BAND_WIDTH_20M < pst_prot_param->en_bandwidth) &&
        (OAL_FALSE == mac_mib_get_2GFortyMHzOperationImplemented(pst_mac_vap)))
    {
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

/* gaolin: CAC在初始检查时统一执行 */
#if 0
#ifdef _PRE_WLAN_FEATURE_DFS
    if (OAL_TRUE == mac_dfs_get_dfs_enable(pst_mac_device))
    {
        hmac_dfs_recalculate_channel_etc(pst_mac_device, &(pst_prot_param->uc_channel), &(pst_prot_param->en_bandwidth));
    }
#endif
#endif
    return OAL_SUCC;
}

oal_uint32  hmac_config_set_mib_by_bw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_mib_by_bw_param_stru *pst_cfg = (mac_cfg_mib_by_bw_param_stru*)puc_param;
    oal_uint32  ul_ret;

    if (!pst_mac_vap || !puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_mib_by_bw::nul ptr, vap=%p param=%p.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_vap_change_mib_by_bandwidth_etc(pst_mac_vap, pst_cfg->en_band, pst_cfg->en_bandwidth);

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CFG80211_SET_MIB_BY_BW, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mib_by_bw::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_channel_param_stru     *pst_channel_param;
    mac_device_stru                *pst_mac_device;
    mac_vap_stru                   *pst_mac_vap_tmp;
    oal_uint32                      ul_ret;
    oal_bool_enum_uint8             en_set_reg = OAL_FALSE;
    oal_uint8                       uc_vap_idx;
    oal_uint32                      ul_up_vap_cnt;
    oal_bool_enum_uint8             en_override = OAL_FALSE;
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    mac_channel_stru st_set_mac_channel;
    oal_uint8 uc_ap_follow_channel = 0;
#endif

    pst_channel_param = (mac_cfg_channel_param_stru *)puc_param;

    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CHAN, "{hmac_config_set_channel_etc::pst_mac_device null,divice_id=%d.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /*信道跟随检查*/
    if(IS_AP(pst_mac_vap))
    {
        st_set_mac_channel.en_band = pst_channel_param->en_band;
        st_set_mac_channel.en_bandwidth = pst_channel_param->en_bandwidth;
        st_set_mac_channel.uc_chan_number = pst_channel_param->uc_channel;
        ul_ret = hmac_check_ap_channel_follow_sta(pst_mac_vap,&st_set_mac_channel,&uc_ap_follow_channel);
        if(OAL_SUCC == ul_ret)
        {
            pst_channel_param->uc_channel = uc_ap_follow_channel;
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_config_set_channel_etc::after hmac_check_ap_channel_follow_sta. channel from %d change to %d}",
                st_set_mac_channel.uc_chan_number,uc_ap_follow_channel);
        }
    }
#endif

    /* 检查配置参数是否在device能力内 */
    ul_ret = hmac_config_set_channel_check_param(pst_mac_vap, pst_channel_param);
    if (OAL_ERR_CODE_CONFIG_BW_EXCEED == ul_ret)
    {
        pst_channel_param->en_bandwidth = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap),
                                                                  pst_channel_param->en_bandwidth);
    }

    if (OAL_ERR_CODE_CONFIG_EXCEED_SPEC == ul_ret)
    {
       OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_config_set_channel_etc::hmac_config_set_channel_check_param failed[%d].}", ul_ret);
       return ul_ret;
    }

    /* 该错误码表示不支持2g 40M,故更改带宽为20M */
    if (OAL_ERR_CODE_CONFIG_UNSUPPORT == ul_ret)
    {
        pst_channel_param->en_bandwidth = WLAN_BAND_WIDTH_20M;
    }

    ul_up_vap_cnt = hmac_calc_up_ap_num_etc(pst_mac_device);

#ifdef _PRE_SUPPORT_ACS
    en_override |= (mac_get_acs_switch(pst_mac_device) >= MAC_ACS_SW_INIT) ? OAL_TRUE : OAL_FALSE;
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
    en_override |= ((mac_vap_get_dfs_enable(pst_mac_vap) && mac_dfs_get_cac_enable(pst_mac_device))) ? OAL_TRUE : OAL_FALSE;
#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    en_override |= (mac_get_2040bss_switch(pst_mac_device) && (!pst_mac_vap->bit_bw_fixed)) ? OAL_TRUE : OAL_FALSE;
#endif

#ifdef _PRE_WLAN_FEATURE_DBAC
    en_override &= !mac_is_dbac_enabled(pst_mac_device) ? OAL_TRUE : OAL_FALSE;
#endif

    en_override &= (ul_up_vap_cnt > 1) ? OAL_TRUE : OAL_FALSE;

    /* 初始扫描使能时，运行时临时设置不同的信道 */
    if (en_override)
    {
        mac_device_get_channel_etc(pst_mac_device, pst_channel_param);
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CHAN,
            "{hmac_config_set_channel_etc::force chan band=%d ch=%d bw=%d}",
             pst_channel_param->en_band, pst_channel_param->uc_channel, pst_channel_param->en_bandwidth);
    }

    /* 仅在没有VAP up的情况下，配置硬件频带、带宽寄存器 */
    if (1 >= ul_up_vap_cnt)
    {
        /* 记录首次配置的带宽值 */
        mac_device_set_channel_etc(pst_mac_device, pst_channel_param);

        /***************************************************************************
         抛事件到DMAC层, 配置寄存器  置标志位
        ***************************************************************************/
        en_set_reg = OAL_TRUE;
    }
#ifdef _PRE_WLAN_FEATURE_DBAC
    else if (OAL_TRUE == mac_is_dbac_enabled(pst_mac_device))
    {
        /* 开启DBAC不进行信道判断 */
        /* 信道设置只针对AP模式，非AP模式则跳出 */
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        /* 1102  DBAC todo 02支持多STA不同信道，需要修改该判断*/
        if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_config_set_channel_etc::pst_mac_vap is not ap, vap id=%d, mode=%d.}",
                             pst_mac_vap->uc_vap_id, pst_mac_vap->en_vap_mode);
            return OAL_FAIL;
        }
#endif
    }
#endif /* _PRE_WLAN_FEATURE_DBAC */
    else
    {
        /* 信道不是当前信道 */
        if (pst_mac_device->uc_max_channel != pst_channel_param->uc_channel)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_config_set_channel_etc::previous vap channel number=%d mismatch [%d].}",
                             pst_mac_device->uc_max_channel, pst_channel_param->uc_channel);

            return OAL_FAIL;
        }

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
        if (mac_is_proxysta_enabled(pst_mac_device)
            && (hmac_find_is_sta_up_etc(pst_mac_device)) &&(pst_channel_param->en_bandwidth != pst_mac_device->en_max_bandwidth))
        {
            /* 网页未适配带宽同步，启动的ap要保持和rootap一样的能力，也即是和device带宽能力一致才能正常启动 fix web bug */
            pst_channel_param->en_bandwidth = pst_mac_device->en_max_bandwidth;
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CHAN,
                             "{hmac_config_set_channel_etc::proxysta mode ap need to sync device mac bandwidth when sta up firstly!}");
        }
#endif

        /* 带宽不能超出已配置的带宽 */
        ul_ret = hmac_config_set_mode_check_bandwith(pst_mac_device->en_max_bandwidth, pst_channel_param->en_bandwidth);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CHAN,
                             "{hmac_config_set_channel_etc::hmac_config_set_mode_check_bandwith failed[%d],previous vap bandwidth[%d, current[%d].}",
                             ul_ret, pst_mac_device->en_max_bandwidth, pst_channel_param->en_bandwidth);
            return OAL_FAIL;
        }

    }
#ifdef _PRE_WLAN_FEATURE_DBAC
    if (OAL_TRUE == mac_is_dbac_enabled(pst_mac_device))
    {
        pst_mac_vap->st_channel.uc_chan_number  = pst_channel_param->uc_channel;
        pst_mac_vap->st_channel.en_band         = pst_channel_param->en_band;
        pst_mac_vap->st_channel.en_bandwidth    = pst_channel_param->en_bandwidth;

        ul_ret = mac_get_channel_idx_from_num_etc(pst_channel_param->en_band,
                pst_channel_param->uc_channel, &(pst_mac_vap->st_channel.uc_chan_idx));
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CHAN,
                              "{hmac_config_set_channel_etc::mac_get_channel_idx_from_num_etc failed[%d], band[%d], channel[%d].}",
                               ul_ret, pst_channel_param->en_band, pst_channel_param->uc_channel);
            return OAL_FAIL;
        }

        en_set_reg = OAL_TRUE;
    }
    else
#endif /* _PRE_WLAN_FEATURE_DBAC */
    {
        for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
        {
            pst_mac_vap_tmp = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);

            if (OAL_PTR_NULL == pst_mac_vap_tmp)
            {
                continue;
            }
            pst_mac_vap_tmp->st_channel.uc_chan_number  = pst_channel_param->uc_channel;
            pst_mac_vap_tmp->st_channel.en_band         = pst_channel_param->en_band;
            pst_mac_vap_tmp->st_channel.en_bandwidth    = pst_channel_param->en_bandwidth;
            ul_ret = mac_get_channel_idx_from_num_etc(pst_channel_param->en_band, pst_channel_param->uc_channel, &(pst_mac_vap->st_channel.uc_chan_idx));
            if (OAL_SUCC != ul_ret)
            {
                OAM_WARNING_LOG3(pst_mac_vap_tmp->uc_vap_id, OAM_SF_CHAN,
                                  "{hmac_config_set_channel_etc::mac_get_channel_idx_from_num_etc failed[%d], band[%d], channel[%d].}",
                                   ul_ret, pst_channel_param->en_band, pst_channel_param->uc_channel);
                continue;
            }
        }
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "hmac_config_set_channel_etc: channel_num:%d, bw:%d, band:%d",
                        pst_channel_param->uc_channel,
                        pst_channel_param->en_bandwidth,
                        pst_channel_param->en_band);
    /***************************************************************************
     抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    if (OAL_TRUE == en_set_reg)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "hmac_config_set_channel_etc: post event to dmac to set register");
        ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CFG80211_SET_CHANNEL, us_len, puc_param);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CHAN, "{hmac_config_set_channel_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

    /* hostapd配置带宽记录信息，同步dmac操作40m恢复定时器*/
    hmac_40M_intol_sync_data(pst_mac_vap, pst_mac_vap->st_channel.en_bandwidth, OAL_FALSE);

#if 0
#if defined(_PRE_SUPPORT_ACS) || defined(_PRE_WLAN_FEATURE_DFS) || defined(_PRE_WLAN_FEATURE_20_40_80_COEXIST)
    ul_up_vap_cnt = hmac_calc_up_ap_num_etc(pst_mac_device);
    if (ul_up_vap_cnt < 2)
    {
        mac_acs_cfg_stru st_acs_cfg;
        st_acs_cfg.uc_acs_type = MAC_ACS_TYPE_INIT;
        st_acs_cfg.en_switch_chan = OAL_TRUE;
        st_acs_cfg.en_scan_op = MAC_SCAN_OP_INIT_SCAN;
        hmac_init_scan_try_etc(pst_mac_device, pst_mac_vap, MAC_TRY_INIT_SCAN_SET_CHANNEL, &st_acs_cfg);
    }
#endif
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_set_beacon_check_param(mac_device_stru *pst_mac_device, mac_beacon_param_stru *pst_prot_param)
{
    /* 根据device能力对参数进行检查 */
    switch(pst_prot_param->en_protocol)
    {
        case WLAN_LEGACY_11A_MODE:
        case WLAN_LEGACY_11B_MODE:
        case WLAN_LEGACY_11G_MODE:
        case WLAN_MIXED_ONE_11G_MODE:
        case WLAN_MIXED_TWO_11G_MODE:
        break;

        case WLAN_HT_MODE:
        case WLAN_HT_ONLY_MODE:
        case WLAN_HT_11G_MODE:
        if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_HT)
        {
            /* 设置11n协议，但device不支持HT模式 */
            OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_beacon_check_param::not support HT mode,en_protocol=%d en_protocol_cap=%d.}",
                            pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
        break;

        case WLAN_VHT_MODE:
        case WLAN_VHT_ONLY_MODE :
        if (pst_mac_device->en_protocol_cap < WLAN_PROTOCOL_CAP_VHT)
        {
            /* 设置11ac协议，但device不支持VHT模式 */
            OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_beacon_check_param::not support VHT mode,en_protocol=%d en_protocol_cap=%d.}",
                             pst_prot_param->en_protocol, pst_mac_device->en_protocol_cap);
            return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
        }
        break;

        default:
            OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_set_beacon_check_param::mode param does not in the list.}");
            break;

    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_beacon_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_beacon_param_stru          *pst_beacon_param;
    mac_device_stru                *pst_mac_device;
    hmac_vap_stru                  *pst_hmac_vap;
    oal_uint32                      ul_ret;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    oal_uint8                       uc_vap_idx;
    hmac_vap_stru                  *pst_hmac_vap_temp;
#endif
//#ifdef _PRE_WLAN_FEATURE_SMPS
//    hmac_config_wmm_para_stru       st_smps_mode;
//#endif
    /* 获取device */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_beacon_etc::null param,pst_mac_device=%x, puc_param=%x.}",
                       pst_mac_device, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_beacon_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_beacon_param = (mac_beacon_param_stru*)puc_param;

    /* 检查协议配置参数是否在device能力内 */
    ul_ret = hmac_config_set_beacon_check_param(pst_mac_device, pst_beacon_param);
    if (OAL_SUCC != ul_ret)
    {
       OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_beacon_etc::hmac_config_add_beacon_check_param failed[%d].}", ul_ret);
       return ul_ret;
    }

    if (pst_beacon_param->en_protocol >= WLAN_HT_MODE)
    {
        mac_mib_set_TxAggregateActived(pst_mac_vap, OAL_TRUE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_TRUE);
    }
    else
    {
        mac_mib_set_TxAggregateActived(pst_mac_vap, OAL_FALSE);
        mac_mib_set_AmsduAggregateAtive(pst_mac_vap, OAL_FALSE);
    }

    mac_vap_set_hide_ssid_etc(pst_mac_vap, pst_beacon_param->uc_hidden_ssid);

    /* 1102适配新内核start ap和change beacon接口复用此接口，不同的是change beacon时，不再设置beacon周期
       和dtim周期，因此，change beacon时，interval和dtim period参数为全零，此时不应该被设置到mib中 */
    /* 设置VAP beacon interval， dtim_period */
    if ((0 != pst_beacon_param->l_dtim_period) || (0 != pst_beacon_param->l_interval))
    {

        mac_mib_set_dot11dtimperiod(pst_mac_vap, (oal_uint32)pst_beacon_param->l_dtim_period);
        mac_mib_set_BeaconPeriod(pst_mac_vap, (oal_uint32)pst_beacon_param->l_interval);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        /* 遍历device下所有vap，统一配置beacon interval， dtim_period参数 */
        for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
        {
            pst_hmac_vap_temp = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
            if (OAL_PTR_NULL == pst_hmac_vap_temp)
            {
                OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_beacon_etc::pst_hmac_vap_temp null.}");
                continue;
            }
            // 没有必要强行同步DTIM周期
            //mac_mib_set_dot11dtimperiod(&pst_hmac_vap_temp->st_vap_base_info, (oal_uint32)pst_beacon_param->l_dtim_period);
            mac_mib_set_BeaconPeriod(&pst_hmac_vap_temp->st_vap_base_info, (oal_uint32)pst_beacon_param->l_interval);
        }
#endif
    }

    /* 设置short gi */
    mac_mib_set_ShortGIOptionInTwentyImplemented(pst_mac_vap, pst_beacon_param->en_shortgi_20);
    mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, pst_beacon_param->en_shortgi_40);

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    if ((WLAN_BAND_2G == pst_mac_vap->st_channel.en_band) && (OAL_TRUE == pst_mac_vap->st_cap_flag.bit_disable_2ght40))
    {
        mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, OAL_FALSE);
    }
#endif

    mac_mib_set_VHTShortGIOptionIn80Implemented(pst_mac_vap, pst_beacon_param->en_shortgi_80);
#ifdef _PRE_WLAN_FEATURE_11R_AP
    mac_mib_set_ft_mdid(pst_mac_vap, pst_beacon_param->us_mdid);
    mac_mib_set_ft_over_ds(pst_mac_vap, pst_beacon_param->en_ft_over_ds);
    mac_mib_set_ft_resource_req(pst_mac_vap, pst_beacon_param->en_ft_resource_req);
    mac_mib_set_ft_trainsistion(pst_mac_vap, pst_beacon_param->en_ft_bss_transition);
#endif

    ul_ret = mac_vap_set_security(pst_mac_vap, pst_beacon_param);
    if (OAL_SUCC != ul_ret)
    {
       OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_beacon_etc::mac_vap_set_security failed[%d].}", ul_ret);
       return ul_ret;
    }

    mac_vap_init_by_protocol_etc(pst_mac_vap, pst_beacon_param->en_protocol);
    mac_vap_init_rates_etc(pst_mac_vap);

//#ifdef _PRE_WLAN_FEATURE_SMPS
    /* APUT 配置beacon帧判断是否支持smps */
    /* 暂时不采用hostapd来配置SMPS,要配置的话，后续再重新适配 */
    //st_smps_mode.ul_ac = (oal_uint32)(MAC_SMPS_MIMO_MODE == pst_beacon_param->uc_smps_mode) ? MAC_SMPS_MIMO_MODE : (pst_beacon_param->uc_smps_mode + 1);
    //hmac_config_set_smps_mode(pst_mac_vap, OAL_SIZEOF(hmac_config_wmm_para_stru), (oal_uint8 *)&st_smps_mode);
//#endif
    /***************************************************************************
     抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CFG80211_CONFIG_BEACON, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_beacon_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

#ifdef _PRE_WLAN_FEATURE_M2S
    /* 同步vap修改信息到device侧 */
    hmac_config_vap_m2s_info_syn(pst_mac_vap);
#endif

    return ul_ret;
}
/*lint -e801*/

extern oal_bool_enum_uint8 g_en_tas_switch_en;
oal_uint32  hmac_config_vap_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_mac_device;
    hmac_vap_stru               *pst_hmac_vap;
    oal_uint32                   ul_ret;
    wlan_protocol_enum_uint8     en_disp_protocol;
    mac_user_stru               *pst_multi_user;
    mac_user_stru               *pst_mac_user;
    oal_uint32                   ul_group_suit;
    oal_uint32                   aul_pair_suites[2];
    oal_uint32                   aul_akm_suites[2];
    oal_uint8                    uc_loop;

    oal_int8            *pc_print_buff;
    oal_uint32           ul_string_len;
    oal_int32            l_string_tmp_len;

    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info_etc::this is config vap! can't get info.}");
        return OAL_FAIL;
    }

    pst_mac_device = (mac_device_stru *)mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info_etc::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info_etc::pc_print_buff null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info_etc::pst_hmac_vap null.}");
        OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len,
                    (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                    "vap id: %d  device id: %d  chip id: %d\n"
                    "vap state: %d\n"
                    "vap mode: %d   P2P mode:%d\n"
                    "ssid: %.32s\n"
                    "hide_ssid :%d\n"
                    "vap nss[%d] devicve nss[%d]\n",
                    pst_mac_vap->uc_vap_id, pst_mac_vap->uc_device_id, pst_mac_vap->uc_chip_id,
                    pst_mac_vap->en_vap_state,
                    pst_mac_vap->en_vap_mode,
                    pst_mac_vap->en_p2p_mode,
                    mac_mib_get_DesiredSSID(pst_mac_vap),
                    pst_mac_vap->st_cap_flag.bit_hide_ssid,
                    pst_mac_vap->en_vap_rx_nss,
                    MAC_DEVICE_GET_NSS_NUM(pst_mac_device));

    if (l_string_tmp_len < 0)
    {
        goto sprint_fail;
    }
    ul_string_len += (oal_uint32)l_string_tmp_len;

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    if (OAL_TRUE == g_en_tas_switch_en)
    {
        l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                            "tas_gpio[%d]\n", board_get_wifi_tas_gpio_state());
        if (l_string_tmp_len < 0)
        {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }
#endif

    /* AP/STA协议模式显示 */
    if ((WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)&& (NULL != (pst_mac_user = mac_res_get_mac_user_etc(pst_mac_vap->us_assoc_vap_id))))
    {
        en_disp_protocol = pst_mac_user->en_cur_protocol_mode;
        l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                            "protocol: sta|ap[%s|%s],\n"
                            " user bandwidth_cap:[%d] avail_bandwidth[%d] cur_bandwidth[%d],\n"
                            " user id[%d] user nss:[%d] user avail nss[%d]\n",
                            hmac_config_protocol2string_etc(pst_mac_vap->en_protocol),
                            hmac_config_protocol2string_etc(en_disp_protocol),
                            pst_mac_user->en_bandwidth_cap,
                            pst_mac_user->en_avail_bandwidth,
                            pst_mac_user->en_cur_bandwidth,
                            pst_mac_vap->us_assoc_vap_id,
                            pst_mac_user->en_user_num_spatial_stream,
                            pst_mac_user->en_avail_num_spatial_stream);
    }
    else
    {
        en_disp_protocol = pst_mac_vap->en_protocol;
        l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                            "protocol: %s\n",
                            hmac_config_protocol2string_etc(en_disp_protocol));

    }

    if (l_string_tmp_len < 0)
    {
        goto sprint_fail;
    }
    ul_string_len += (oal_uint32)l_string_tmp_len;

    l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len,
                (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                "band: %s  bandwidth: %s\n"
                "channel number:%d \n"
                "associated user number:%d/%d \n"
                "Beacon interval:%d \n"
                "vap feature info:\n"
                "amsdu     uapsd   wpa   wpa2   wps  keepalive\n"
                "%d         %d       %d    %d     %d     %d\n"
                "vap cap info:\n"
                "shpreamble  shslottime  nobeacon  shortgi   2g11ac \n"
                "%d           %d          %d         %d         %d\n"
                "tx power: %d \n"
                "protect mode: %d, auth mode: %d\n"
                "erp aging cnt: %d, ht aging cnt: %d\n"
                "auto_protection: %d\nobss_non_erp_present: %d\nobss_non_ht_present: %d\n"
                "rts_cts_protect_mode: %d\ntxop_protect_mode: %d\n"
                "no_short_slot_num: %d\nno_short_preamble_num: %d\nnon_erp_num: %d\n"
                "non_ht_num: %d\nnon_gf_num: %d\n20M_only_num: %d\n"
                "no_40dsss_cck_num: %d\nno_lsig_txop_num: %d\n",
                hmac_config_band2string_etc(pst_mac_vap->st_channel.en_band),
                hmac_config_bw2string_etc(pst_mac_vap->en_protocol <= WLAN_MIXED_TWO_11G_MODE ? WLAN_BAND_WIDTH_20M : pst_mac_vap->st_channel.en_bandwidth),
                pst_mac_vap->st_channel.uc_chan_number,
                pst_mac_vap->us_user_nums,
                mac_mib_get_MaxAssocUserNums(pst_mac_vap),
                mac_mib_get_BeaconPeriod(pst_mac_vap),
                mac_mib_get_CfgAmsduTxAtive(pst_mac_vap),
                pst_mac_vap->st_cap_flag.bit_uapsd,
                pst_mac_vap->st_cap_flag.bit_wpa,
                pst_mac_vap->st_cap_flag.bit_wpa2,
                mac_mib_get_WPSActive(pst_mac_vap),
                pst_mac_vap->st_cap_flag.bit_keepalive,
                mac_mib_get_ShortPreambleOptionImplemented(pst_mac_vap),
                mac_mib_get_ShortSlotTimeOptionImplemented(pst_mac_vap),
                pst_hmac_vap->en_no_beacon,
                mac_mib_get_ShortGIOptionInTwentyImplemented(pst_mac_vap),
                pst_mac_vap->st_cap_flag.bit_11ac2g,
                pst_mac_vap->uc_tx_power,
                pst_mac_vap->st_protection.en_protection_mode,
                mac_mib_get_AuthenticationMode(pst_mac_vap),
                pst_mac_vap->st_protection.uc_obss_non_erp_aging_cnt,
                pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt,
                pst_mac_vap->st_protection.bit_auto_protection,
                pst_mac_vap->st_protection.bit_obss_non_erp_present,
                pst_mac_vap->st_protection.bit_obss_non_ht_present,
                pst_mac_vap->st_protection.bit_rts_cts_protect_mode,
                pst_mac_vap->st_protection.bit_lsig_txop_protect_mode,
                pst_mac_vap->st_protection.uc_sta_no_short_slot_num,
                pst_mac_vap->st_protection.uc_sta_no_short_preamble_num,
                pst_mac_vap->st_protection.uc_sta_non_erp_num,
                pst_mac_vap->st_protection.uc_sta_non_ht_num,
                pst_mac_vap->st_protection.uc_sta_non_gf_num,
                pst_mac_vap->st_protection.uc_sta_20M_only_num,
                pst_mac_vap->st_protection.uc_sta_no_40dsss_cck_num,
                pst_mac_vap->st_protection.uc_sta_no_lsig_txop_num);

    if (l_string_tmp_len < 0)
    {
        goto sprint_fail;
    }


    pc_print_buff[OAM_REPORT_MAX_STRING_LEN-1] = '\0';
    oam_print_etc(pc_print_buff);

    /* 上述日志量超过OAM_REPORT_MAX_STRING_LEN，分多次oam_print */
    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;

    /* WPA/WPA2 加密参数 */
    if(OAL_TRUE == mac_mib_get_privacyinvoked(pst_mac_vap))
    {
        pst_multi_user = mac_res_get_mac_user_etc(pst_mac_vap->us_multi_user_idx);
        if (OAL_PTR_NULL == pst_multi_user)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info_etc::pst_multi_user[%d] null.}",
                pst_mac_vap->us_multi_user_idx);
            OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (OAL_TRUE == mac_mib_get_rsnaactivated(pst_mac_vap))
        {
            if(1 == pst_mac_vap->st_cap_flag.bit_wpa)
            {
                ul_group_suit       = mac_mib_get_wpa_group_suite(pst_mac_vap);
                mac_mib_get_wpa_pair_suites(pst_mac_vap, aul_pair_suites);
                mac_mib_get_wpa_akm_suites(pst_mac_vap, aul_akm_suites);
                l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1), "Privacy Invoked: \nRSNA-WPA \n "
                        "GRUOP     WPA PAIREWISE0[Actived]    WPA PAIRWISE1[Actived]     AUTH1[Active]     AUTH2[Active]\n "
                        "%s        %s[%s]          %s[%s]             %s[%s]             %s[%s]\n",
                        hmac_config_ciper2string_etc((oal_uint8)ul_group_suit),
                        hmac_config_ciper2string_etc((oal_uint8)aul_pair_suites[0]),
                        (aul_pair_suites[0] == 0) ? "Actived":"Inactived",
                        hmac_config_ciper2string_etc((oal_uint8)aul_pair_suites[1]),
                        (aul_pair_suites[1] == 0) ? "Actived":"Inactived",
                        hmac_config_akm2string_etc((oal_uint8)aul_akm_suites[0]),
                        (aul_akm_suites[0] == 0) ? "Actived":"Inactived",
                        hmac_config_akm2string_etc((oal_uint8)aul_akm_suites[1]),
                        (aul_akm_suites[1] == 0) ? "Actived":"Inactived");
                if (l_string_tmp_len < 0)
                {
                    goto sprint_fail;
                }
                ul_string_len += (oal_uint32)l_string_tmp_len;
            }

            if(1 == pst_mac_vap->st_cap_flag.bit_wpa2)
            {
                ul_group_suit       = mac_mib_get_rsn_group_suite(pst_mac_vap);
                mac_mib_get_rsn_pair_suites(pst_mac_vap, aul_pair_suites);
                mac_mib_get_rsn_akm_suites(pst_mac_vap, aul_akm_suites);
                l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1), "Privacy Invoked: \nRSNA-WPA2 \n"
                        "GRUOP     WPA2 PAIREWISE0[Actived]    WPA2 PAIRWISE1[Actived]     AUTH1[Active]     AUTH2[Active]\n"
                        "%s        %s[%s]          %s[%s]             %s[%s]             %s[%s]\n",
                        hmac_config_ciper2string_etc((oal_uint8)ul_group_suit),
                        hmac_config_ciper2string_etc((oal_uint8)aul_pair_suites[0]),
                        (aul_pair_suites[0] == 0) ? "Actived":"Inactived",
                        hmac_config_ciper2string_etc((oal_uint8)aul_pair_suites[1]),
                        (aul_pair_suites[1] == 0) ? "Actived":"Inactived",
                        hmac_config_akm2string_etc((oal_uint8)aul_akm_suites[0]),
                        (aul_akm_suites[0] == 0) ? "Actived":"Inactived",
                        hmac_config_akm2string_etc((oal_uint8)aul_akm_suites[1]),
                        (aul_akm_suites[1] == 0) ? "Actived":"Inactived");
                if (l_string_tmp_len < 0)
                {
                    goto sprint_fail;
                }
                ul_string_len += (oal_uint32)l_string_tmp_len;
            }
        }

        l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1), "MULTI_USER: cipher_type:%s, key_type:%d \n",
                                                        hmac_config_ciper2string_etc(pst_multi_user->st_key_info.en_cipher_type),
                                                        pst_multi_user->st_user_tx_info.st_security.en_cipher_key_type);

        if (l_string_tmp_len < 0)
        {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;

    }
    else
    {
        l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1), "Privacy NOT Invoked\n");
        if (l_string_tmp_len < 0)
        {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }

    /* APP IE 信息 */
    for (uc_loop = 0; uc_loop < OAL_APP_IE_NUM; uc_loop++)
    {
        l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len, (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1), "APP IE:type= %d, addr = %p, len = %d, max_len = %d\n",
                    uc_loop,
                    pst_mac_vap->ast_app_ie[uc_loop].puc_ie,
                    pst_mac_vap->ast_app_ie[uc_loop].ul_ie_len,
                    pst_mac_vap->ast_app_ie[uc_loop].ul_ie_max_len);
        if (l_string_tmp_len < 0)
        {
            goto sprint_fail;
        }
        ul_string_len += (oal_uint32)l_string_tmp_len;
    }

    pc_print_buff[OAM_REPORT_MAX_STRING_LEN-1] = '\0';
    oam_print_etc(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_VAP_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_info_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

sprint_fail:

    OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_vap_info_etc:: OAL_SPRINTF return error!}");
    pc_print_buff[OAM_REPORT_MAX_STRING_LEN-1] = '\0';
    oam_print_etc(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);

    return OAL_FAIL;

}

#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
/*lint +e801*/

oal_uint32 hmac_config_pk_mode_debug(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_pk_mode_debug:: puc_param is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAM_WARNING_LOG4(0, OAM_SF_CFG, "{hmac_config_pk_mode_debug::set high/low = %u, BW = %u, pro = %u, valid = %u!}",
    puc_param[0],puc_param[1],puc_param[2],puc_param[3]);

    if (2 == puc_param[0])
    {
        g_en_pk_mode_swtich = OAL_TRUE;
        return OAL_SUCC;
    }
    else if (3 == puc_param[0])
    {
        g_en_pk_mode_swtich = OAL_FALSE;

        return OAL_SUCC;
    }

    if (0 == puc_param[0])
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_pk_mode_debug::set high th (%u) -> (%u)!}",
        g_st_pk_mode_high_th_table[puc_param[2]][puc_param[1]],
        puc_param[3]);
        g_st_pk_mode_high_th_table[puc_param[2]][puc_param[1]] = puc_param[3];
        return OAL_SUCC;

    }

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_pk_mode_debug::set low th (%u) -> (%u)!}",
    g_st_pk_mode_low_th_table[puc_param[2]][puc_param[1]],
    puc_param[3]);

    g_st_pk_mode_low_th_table[puc_param[2]][puc_param[1]] = puc_param[3];

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_event_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    oal_int32                   l_value;
    oal_uint32                  ul_ret;
    oal_uint8                   uc_loop_vap_id;

    l_value = *((oal_int32 *)puc_param);

    /* 设置OAM event模块的开关 */
    for (uc_loop_vap_id = 0; uc_loop_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_loop_vap_id++)
    {
        ul_ret = oam_event_set_switch_etc(uc_loop_vap_id, (oal_switch_enum_uint8)l_value);

        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(uc_loop_vap_id, OAM_SF_CFG, "{hmac_config_event_switch_etc::oam_event_set_switch_etc failed[%d].}", ul_ret);
            return ul_ret;
        }
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_eth_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_eth_switch_param_stru  *pst_eth_switch_param;
    oal_uint16                      us_user_idx = 0;
    oal_uint32                      ul_ret;

    pst_eth_switch_param = (mac_cfg_eth_switch_param_stru *)puc_param;

    ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap,
                                          pst_eth_switch_param->auc_user_macaddr,
                                          &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_eth_switch_etc::mac_vap_find_user_by_macaddr_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    ul_ret = oam_report_eth_frame_set_switch_etc(us_user_idx,
                                             pst_eth_switch_param->en_switch,
                                             pst_eth_switch_param->en_frame_direction);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_eth_switch_etc::oam_report_eth_frame_set_switch_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_80211_ucast_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_80211_ucast_switch_stru *pst_80211_switch_param;
    oal_uint16                       us_user_idx = 0;
    oal_uint16                       us_max_user_idx = 0;
    oal_uint32                       ul_ret;

    pst_80211_switch_param = (mac_cfg_80211_ucast_switch_stru *)puc_param;

    us_max_user_idx = mac_board_get_max_user();

    /* 广播地址，操作所有用户的单播帧开关 */
    if (ETHER_IS_BROADCAST(pst_80211_switch_param->auc_user_macaddr))
    {
        for (us_user_idx = 0; us_user_idx < us_max_user_idx; us_user_idx++)
        {
            oam_report_80211_ucast_set_switch_etc(pst_80211_switch_param->en_frame_direction,
                                              pst_80211_switch_param->en_frame_type,
                                              pst_80211_switch_param->en_frame_switch,
                                              pst_80211_switch_param->en_cb_switch,
                                              pst_80211_switch_param->en_dscr_switch,
                                              us_user_idx);
        }
    }
    else
    {
        ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap,
                                              pst_80211_switch_param->auc_user_macaddr,
                                              &us_user_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_80211_ucast_switch_etc::mac_vap_find_user_by_macaddr_etc[%02X:XX:XX:%02X:%02X:%02X]failed !!}",
                            pst_80211_switch_param->auc_user_macaddr[0],
                            pst_80211_switch_param->auc_user_macaddr[3],
                            pst_80211_switch_param->auc_user_macaddr[4],
                            pst_80211_switch_param->auc_user_macaddr[5]);
            return ul_ret;
        }

        ul_ret = oam_report_80211_ucast_set_switch_etc(pst_80211_switch_param->en_frame_direction,
                                                   pst_80211_switch_param->en_frame_type,
                                                   pst_80211_switch_param->en_frame_switch,
                                                   pst_80211_switch_param->en_cb_switch,
                                                   pst_80211_switch_param->en_dscr_switch,
                                                   us_user_idx);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_80211_ucast_switch_etc::Set switch of report_ucast failed[%d]!!frame_switch[%d], cb_switch[%d], dscr_switch[%d].}",
                    ul_ret,
                    pst_80211_switch_param->en_frame_switch,
                    pst_80211_switch_param->en_cb_switch,
                    pst_80211_switch_param->en_dscr_switch);
            return ul_ret;
        }
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_80211_UCAST_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_80211_ucast_switch_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }
#endif /* DMAC_OFFLOAD */

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_mgmt_log_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, oal_bool_enum_uint8 en_start)
{
    mac_cfg_80211_ucast_switch_stru st_80211_ucast_switch;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_mac_user))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_TRUE != en_start)
    {
        st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_TX;
        st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
        st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_OFF;
        oal_set_mac_addr(st_80211_ucast_switch.auc_user_macaddr,pst_mac_user->auc_user_mac_addr);
        hmac_config_80211_ucast_switch_etc(pst_mac_vap, OAL_SIZEOF(st_80211_ucast_switch), (oal_uint8 *)&st_80211_ucast_switch);

        st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_RX;
        st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
        st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_OFF;
        st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_OFF;
        hmac_config_80211_ucast_switch_etc(pst_mac_vap, OAL_SIZEOF(st_80211_ucast_switch), (oal_uint8 *)&st_80211_ucast_switch);
    }
    else
    {
        st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_TX;
        st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
        st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_ON;
        st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_ON;
        st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_ON;
        oal_set_mac_addr(st_80211_ucast_switch.auc_user_macaddr,pst_mac_user->auc_user_mac_addr);

        hmac_config_80211_ucast_switch_etc(pst_mac_vap, OAL_SIZEOF(st_80211_ucast_switch), (oal_uint8 *)&st_80211_ucast_switch);

        st_80211_ucast_switch.en_frame_direction = OAM_OTA_FRAME_DIRECTION_TYPE_RX;
        st_80211_ucast_switch.en_frame_type = OAM_USER_TRACK_FRAME_TYPE_MGMT;
        st_80211_ucast_switch.en_frame_switch = OAL_SWITCH_ON;
        st_80211_ucast_switch.en_cb_switch = OAL_SWITCH_ON;
        st_80211_ucast_switch.en_dscr_switch = OAL_SWITCH_ON;
        hmac_config_80211_ucast_switch_etc(pst_mac_vap, OAL_SIZEOF(st_80211_ucast_switch), (oal_uint8 *)&st_80211_ucast_switch);
    }
    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE_USER_TRACK

oal_uint32  hmac_config_report_thrput_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USR_THRPUT_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_report_thrput_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS


oal_uint32  hmac_config_set_txop_ps_machw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                    ul_ret;
    mac_txopps_machw_param_stru  *pst_txopps;

    pst_txopps = (mac_txopps_machw_param_stru *)puc_param;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TXOP,"{hmac_config_set_txop_ps_machw::txopps enable[%d]}.", pst_txopps->en_machw_txopps_en);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* 110x txopps此测试接口通过设置mib项来打开功能，寄存器的打开在后续逻辑判断中执行 */
    mac_mib_set_txopps(pst_mac_vap, pst_txopps->en_machw_txopps_en);
#endif
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TXOP_PS_MACHW, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TXOP,
                        "{hmac_config_set_txop_ps_machw::send event return err code [%d].}", ul_ret);
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_LTECOEX

oal_uint32  hmac_config_ltecoex_mode_set(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_LTECOEX_MODE_SET, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX,
                        "{hmac_config_ltecoex_mode_set::send event return err code [%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_80211_mcast_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_80211_mcast_switch_stru *pst_80211_switch_param;
    oal_uint32                       ul_ret = 0;

    pst_80211_switch_param = (mac_cfg_80211_mcast_switch_stru *)puc_param;

    ul_ret = oam_report_80211_mcast_set_switch_etc(pst_80211_switch_param->en_frame_direction,
                                               pst_80211_switch_param->en_frame_type,
                                               pst_80211_switch_param->en_frame_switch,
                                               pst_80211_switch_param->en_cb_switch,
                                               pst_80211_switch_param->en_dscr_switch);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_80211_mcast_switch_etc::oam_report_80211_mcast_set_switch_etc failed[%d].}", ul_ret);
        return ul_ret;
    }
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_80211_MCAST_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_80211_mcast_switch_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;

}


oal_uint32  hmac_config_probe_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_probe_switch_stru       *pst_probe_switch;
    oal_uint32                       ul_ret = 0;

    pst_probe_switch = (mac_cfg_probe_switch_stru *)puc_param;

    ul_ret = oam_report_80211_probe_set_switch_etc(pst_probe_switch->en_frame_direction,
                                               pst_probe_switch->en_frame_switch,
                                               pst_probe_switch->en_cb_switch,
                                               pst_probe_switch->en_dscr_switch);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_probe_switch_etc::oam_report_80211_probe_set_switch_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PROBE_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_probe_switch_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_phy_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;
#ifdef _PRE_WLAN_FEATURE_DFS
    mac_phy_debug_switch_stru      *pst_phy_debug_switch;

    pst_phy_debug_switch = (mac_phy_debug_switch_stru *)puc_param;
    if(pst_phy_debug_switch->uc_report_radar_switch == OAL_TRUE)
    {
        ul_ret = hmac_dfs_radar_detect_event_test(pst_mac_vap->uc_vap_id);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_phy_debug_switch::hmac_dfs_radar_detect_event_test failed[%d].}", ul_ret);
            return ul_ret;
        }
    }
#endif
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PHY_DEBUG_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_phy_debug_switch::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_11AX
OAL_STATIC oal_void  hmac_config_protocol_debug_printf_11ax_info(mac_vap_stru *pst_mac_vap)
{
    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_protocol_debug_printf_11ax_info::he_mib_enable=[%d],hal_dev_11ax_enable=[%d],11ax_switch=[%d].",
    mac_mib_get_HEOptionImplemented(pst_mac_vap),IS_HAL_DEVICE_SUPPORT_11AX(pst_mac_vap),
    IS_CUSTOM_OPEN_11AX_SWITCH(pst_mac_vap));
}
#endif


oal_uint32  hmac_config_protocol_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                            ul_ret;
    mac_protocol_debug_switch_stru       *pst_protocol_debug;

    pst_protocol_debug = (mac_protocol_debug_switch_stru *)puc_param;

    /* 恢复40M带宽命令*/
    if(pst_protocol_debug->ul_cmd_bit_map & BIT0)
    {
        /*host无需处理，dmac处理*/
    }
    /* 不允许20/40带宽切换开关*/
    if(pst_protocol_debug->ul_cmd_bit_map & BIT1)
    {
        /*因为不允许20/40带宽切换开关只在dmac用到，host不用处理
        if(OAL_TRUE == pst_bandwidth_switch->en_2040_ch_swt_prohi_bit1)
        {
            mac_mib_set_2040SwitchProhibited(pst_mac_vap, OAL_TRUE);
        }
        else
        {
            mac_mib_set_2040SwitchProhibited(pst_mac_vap, OAL_FALSE);
        }
        */
    }
    /* 不容忍40M带宽开关*/
    if(pst_protocol_debug->ul_cmd_bit_map & BIT2)
    {
        /*只有2.4G才设置该mib值*/
        if(WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
        {
            if(OAL_TRUE == pst_protocol_debug->en_40_intolerant_bit2)
            {
                mac_mib_set_FortyMHzIntolerant(pst_mac_vap, OAL_TRUE);
            }
            else
            {
                mac_mib_set_FortyMHzIntolerant(pst_mac_vap, OAL_FALSE);
            }
        }
        else
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_protocol_debug_switch::band is not 2G,but [%d].", pst_mac_vap->st_channel.en_band);
        }
    }
    /*csa cmd*/
    if(pst_protocol_debug->ul_cmd_bit_map & BIT3)
    {
        if(!IS_LEGACY_AP(pst_mac_vap))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_protocol_debug_switch::vap mode is not ap,return.");
            return OAL_SUCC;
        }

        ul_ret = mac_is_channel_num_valid_etc(pst_mac_vap->st_channel.en_band,pst_protocol_debug->st_csa_debug_bit3.uc_channel);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_protocol_debug_switch::mac_is_channel_num_valid_etc(%d),return.",
                pst_protocol_debug->st_csa_debug_bit3.uc_channel);
            return OAL_SUCC;
        }

        /* 根据device能力对参数进行检查 */
        if ((pst_protocol_debug->st_csa_debug_bit3.en_bandwidth >= WLAN_BAND_WIDTH_80PLUSPLUS) && (mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap) < WLAN_BW_CAP_80M))
        {
            pst_protocol_debug->st_csa_debug_bit3.en_bandwidth = mac_vap_get_bandwith(mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap),
                                                                      pst_protocol_debug->st_csa_debug_bit3.en_bandwidth);

            /* 设置80M带宽，但device能力不支持80M， 刷新成合适带宽做业务 */
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_protocol_debug_switch::not support 80MHz bandwidth,csa_new_bandwidth=%d en_dot11VapMaxBandWidth=%d.}",
                         pst_protocol_debug->st_csa_debug_bit3.en_bandwidth, mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap));
        }

        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"hmac_config_protocol_debug_switch::csa_mode=%d csa_channel=%d csa_cnt=%d debug_flag=%d \n",
            pst_protocol_debug->st_csa_debug_bit3.en_mode,pst_protocol_debug->st_csa_debug_bit3.uc_channel,
            pst_protocol_debug->st_csa_debug_bit3.uc_cnt,pst_protocol_debug->st_csa_debug_bit3.en_debug_flag);

        if(MAC_CSA_FLAG_NORMAL == pst_protocol_debug->st_csa_debug_bit3.en_debug_flag)
        {
            pst_mac_vap->st_ch_switch_info.en_csa_mode           = pst_protocol_debug->st_csa_debug_bit3.en_mode;
            pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt      = pst_protocol_debug->st_csa_debug_bit3.uc_cnt;
            hmac_chan_initiate_switch_to_new_channel(pst_mac_vap,pst_protocol_debug->st_csa_debug_bit3.uc_channel,pst_protocol_debug->st_csa_debug_bit3.en_bandwidth);
            return OAL_SUCC;
        }
    }
#ifdef _PRE_WLAN_FEATURE_HWBW_20_40
    if(pst_protocol_debug->ul_cmd_bit_map & BIT4)
    {
        /*host无需处理，dmac处理*/
    }
#endif
    /*lsigtxop使能*/
    if(pst_protocol_debug->ul_cmd_bit_map & BIT5)
    {
        mac_mib_set_LsigTxopFullProtectionActivated(pst_mac_vap, pst_protocol_debug->en_lsigtxop_bit5);
    }

#ifdef _PRE_WLAN_FEATURE_11AX
    if(pst_protocol_debug->ul_cmd_bit_map & BIT6)
    {
        hmac_config_protocol_debug_printf_11ax_info(pst_mac_vap);
    }
#endif
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PROTOCOL_DBG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_protocol_debug_switch::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_report_vap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_REPORT_VAP_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_report_vap_info::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_wfa_cfg_aifsn_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_WFA_CFG_AIFSN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wfa_cfg_aifsn_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_wfa_cfg_cw_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_WFA_CFG_CW, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wfa_cfg_cw_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

oal_uint32  hmac_config_lte_gpio_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CHECK_LTE_GPIO, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_lte_gpio_mode_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#endif


oal_uint32  hmac_config_get_mpdu_num_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_MPDU_NUM, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_send_event_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32 hmac_config_beacon_offload_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    OAL_IO_PRINT("hmac_config_beacon_offload_test: host_sleep=%d\n", *puc_param);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_BEACON_OFFLOAD_TEST, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_beacon_offload_test::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32 hmac_config_ota_beacon_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                                uc_vap_id_loop;
    oal_uint32                               ul_ret;
    oal_int32                                l_value;

    l_value = *((oal_int32 *)puc_param);

    for (uc_vap_id_loop = 0; uc_vap_id_loop < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id_loop++)
    {
          ul_ret = oam_ota_set_beacon_switch_etc(uc_vap_id_loop,
                                             (oam_sdt_print_beacon_rxdscr_type_enum_uint8)l_value);

        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(uc_vap_id_loop, OAM_SF_ANY, "{hmac_config_ota_beacon_switch_etc::ota beacon switch set failed!}\r\n");
            return ul_ret;
        }
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_OTA_BEACON_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_ota_beacon_switch_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}

oal_uint32 hmac_config_ota_rx_dscr_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                                uc_vap_id_loop;
    oal_uint32                               ul_ret;
    oal_int32                                l_value;

    l_value = *((oal_int32 *)puc_param);

    for (uc_vap_id_loop = 0; uc_vap_id_loop < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id_loop++)
    {
          ul_ret = oam_ota_set_rx_dscr_switch_etc(uc_vap_id_loop,
                                             (oal_switch_enum_uint8)l_value);

        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(uc_vap_id_loop, OAM_SF_ANY, "{hmac_config_ota_rx_dscr_switch_etc::ota rx_dscr switch set failed!}\r\n");
            return ul_ret;
        }
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_OTA_RX_DSCR_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_ota_rx_dscr_switch_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_all_ota_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;
    oal_switch_enum_uint8      en_switch;

    en_switch = *((oal_switch_enum_uint8 *)puc_param);
    oam_report_set_all_switch_etc(en_switch);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALL_OTA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_all_ota_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_oam_output_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_int32                   l_value;
    oal_uint32                  ul_ret;

    l_value = *((oal_int32 *)puc_param);

    /* 设置OAM log模块的开关 */
    ul_ret = oam_set_output_type_etc((oam_output_type_enum_uint8)l_value);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_oam_output_etc::oam_set_output_type_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_OAM_OUTPUT_TYPE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_oam_output_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_dhcp_arp_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    oal_switch_enum_uint8      en_switch;

    en_switch = *((oal_switch_enum_uint8 *)puc_param);
    oam_report_dhcp_arp_set_switch_etc(en_switch);

    return OAL_SUCC;

}

#ifdef _PRE_WLAN_FEATURE_DHCP_REQ_DISABLE

oal_uint32 hmac_config_set_dhcp_req_disable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_switch_enum_uint8      en_switch;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_config_set_dhcp_req_disable::null param,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_switch = *((oal_switch_enum_uint8 *)puc_param);

    pst_mac_vap->en_dhcp_req_disable_switch = (en_switch == 0? 0:1);

    return OAL_SUCC;
}
#endif

oal_uint32 hmac_config_set_random_mac_addr_scan_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_device_stru          *pst_hmac_device = OAL_PTR_NULL;
    oal_bool_enum_uint8        en_random_mac_addr_scan_switch;

    en_random_mac_addr_scan_switch = *((oal_bool_enum_uint8 *)puc_param);

    /* 获取hmac device结构体 */
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_set_random_mac_addr_scan_etc::pst_hmac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
    g_st_wlan_customize_etc.uc_random_mac_addr_scan = en_random_mac_addr_scan_switch;
#else
    pst_hmac_device->st_scan_mgmt.en_is_random_mac_addr_scan = en_random_mac_addr_scan_switch;
#endif

    return OAL_SUCC;

}



oal_uint32 hmac_config_set_random_mac_oui_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                ul_ret;
    mac_device_stru          *pst_mac_device = OAL_PTR_NULL;
    hmac_device_stru         *pst_hmac_device;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_set_random_mac_oui_etc::pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_random_mac_oui_etc::pst_hmac_device is null.device_id %d}",
                        pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(us_len < WLAN_RANDOM_MAC_OUI_LEN)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_random_mac_oui_etc::len is short:%d.}", us_len);
        return OAL_FAIL;
    }

    oal_memcopy(pst_mac_device->auc_mac_oui, puc_param, WLAN_RANDOM_MAC_OUI_LEN);

    /* Android 会在启动wifi 后下发非零mac_oui, wps扫描或hilink连接的场景中,将mac_oui清0,
     * mac_oui 非零时生成扫描随机MAC, wifi 扫描时使用该MAC地址作为源地址 */
    if ((pst_mac_device->auc_mac_oui[0] != 0) || (pst_mac_device->auc_mac_oui[1] != 0) || (pst_mac_device->auc_mac_oui[2] != 0))
    {
        oal_random_ether_addr(pst_hmac_device->st_scan_mgmt.auc_random_mac);
        pst_hmac_device->st_scan_mgmt.auc_random_mac[0] = pst_mac_device->auc_mac_oui[0] & 0xfe;  /*保证是单播mac*/
        pst_hmac_device->st_scan_mgmt.auc_random_mac[1] = pst_mac_device->auc_mac_oui[1];
        pst_hmac_device->st_scan_mgmt.auc_random_mac[2] = pst_mac_device->auc_mac_oui[2];

        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_set_random_mac_oui_etc::rand_mac_addr[%02X:XX:XX:%02X:%02X:%02X].}",
                             pst_hmac_device->st_scan_mgmt.auc_random_mac[0],
                             pst_hmac_device->st_scan_mgmt.auc_random_mac[3],
                             pst_hmac_device->st_scan_mgmt.auc_random_mac[4],
                             pst_hmac_device->st_scan_mgmt.auc_random_mac[5]);
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_RANDOM_MAC_OUI, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_random_mac_oui_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_config_set_vowifi_nat_keep_alive_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                ul_ret;
    mac_device_stru          *pst_mac_device = OAL_PTR_NULL;

    if(OAL_FALSE == IS_LEGACY_STA(pst_mac_vap))
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_set_vowifi_nat_keep_alive_params::vap is not legacy sta.}");
        return OAL_FAIL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_set_vowifi_nat_keep_alive_params::pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_VOWIFI_KEEP_ALIVE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vowifi_nat_keep_alive_params::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif



oal_uint32  hmac_config_ampdu_start_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_ampdu_start_param_stru *pst_ampdu_start;
    hmac_user_stru                 *pst_hmac_user;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_action_mgmt_args_stru       st_action_args;
    oal_uint8                       ul_ret;

    pst_ampdu_start = (mac_cfg_ampdu_start_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_start_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_ampdu_start->auc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_start_etc::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_tid_need_ba_session(pst_hmac_vap, pst_hmac_user, pst_ampdu_start->uc_tidno, OAL_PTR_NULL);
    if (OAL_TRUE == ul_ret)
    {
        /*
            建立BA会话时，st_action_args结构各个成员意义如下
            (1)uc_category:action的类别
            (2)uc_action:BA action下的类别
            (3)ul_arg1:BA会话对应的TID
            (4)ul_arg2:BUFFER SIZE大小
            (5)ul_arg3:BA会话的确认策略
            (6)ul_arg4:TIMEOUT时间
        */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_start_etc::uc_tidno=%d.}", pst_ampdu_start->uc_tidno);
        st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
        st_action_args.uc_action   = MAC_BA_ACTION_ADDBA_REQ;
        st_action_args.ul_arg1     = pst_ampdu_start->uc_tidno;    /* 该数据帧对应的TID号 */
        st_action_args.ul_arg2     = WLAN_AMPDU_TX_MAX_BUF_SIZE;    /* ADDBA_REQ中，buffer_size的默认大小 */
        st_action_args.ul_arg3     = MAC_BA_POLICY_IMMEDIATE;      /* BA会话的确认策略 */
        st_action_args.ul_arg4     = 0;                            /* BA会话的超时时间设置为0 */

        /* 建立BA会话 */
        hmac_mgmt_tx_action_etc(pst_hmac_vap,  pst_hmac_user, &st_action_args);
    }


    return OAL_SUCC;
}

#ifdef _PRE_WLAN_CHIP_FPGA_PCIE_TEST

oal_uint32  hmac_config_pcie_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PCIE_TEST, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_pcie_test::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_ampdu_end_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_ampdu_end_param_stru   *pst_ampdu_end;
    hmac_user_stru                 *pst_hmac_user;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_priv_req_args_stru          st_req_arg;

    pst_ampdu_end = (mac_cfg_ampdu_start_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_end_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_ampdu_end->auc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ampdu_end_etc::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置AMPDU开始时，st_req_arg结构各个成员意义如下 */
    st_req_arg.uc_type  = MAC_A_MPDU_END;
    st_req_arg.uc_arg1  = pst_ampdu_end->uc_tidno;      /* 该数据帧对应的TID号 */

    /* 建立BA会话 */
    hmac_mgmt_tx_priv_req_etc(pst_hmac_vap,  pst_hmac_user, &st_req_arg);

    return OAL_SUCC;
}


oal_uint32  hmac_config_amsdu_ampdu_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_tlv_stru       *pst_amsdu_ampdu_cfg;

    pst_amsdu_ampdu_cfg = (mac_cfg_set_tlv_stru *)puc_param;

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
    g_st_tx_large_amsdu.uc_cur_amsdu_enable = (oal_uint8)pst_amsdu_ampdu_cfg->ul_value;
#endif

    mac_mib_set_AmsduPlusAmpduActive(pst_mac_vap, (oal_uint8)pst_amsdu_ampdu_cfg->ul_value);
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_amsdu_ampdu_switch_etc::ENABLE MODE[%d][0:disable,1:enable].}", pst_amsdu_ampdu_cfg->ul_value);

    return OAL_SUCC;
}



oal_uint32  hmac_config_rx_ampdu_amsdu(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_tlv_stru       *pst_rx_ampdu_amsdu_cfg;
    hmac_vap_stru              *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_rx_ampdu_amsdu::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rx_ampdu_amsdu_cfg = (mac_cfg_set_tlv_stru *)puc_param;

    pst_hmac_vap->bit_rx_ampduplusamsdu_active = (oal_uint8)pst_rx_ampdu_amsdu_cfg->ul_value;

    g_uc_host_rx_ampdu_amsdu = pst_hmac_vap->bit_rx_ampduplusamsdu_active;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_rx_ampdu_amsdu::ENABLE MODE[%d][0:disable,1:enable].}", pst_rx_ampdu_amsdu_cfg->ul_value);

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_addba_rsp_buffer(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_tlv_stru       *pst_rx_buffer_size_cfg;
    hmac_vap_stru              *pst_hmac_vap;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_rx_ampdu_amsdu::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rx_buffer_size_cfg = (mac_cfg_set_tlv_stru *)puc_param;

    g_st_rx_buffer_size_stru.uc_rx_buffer_size_set_en = pst_rx_buffer_size_cfg->uc_cmd_type;
    if(OAL_TRUE == g_st_rx_buffer_size_stru.uc_rx_buffer_size_set_en)
    {
        g_st_rx_buffer_size_stru.uc_rx_buffer_size = pst_rx_buffer_size_cfg->ul_value;
    }


    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_addba_rsp_buffer::ENABLE MODE[%d],buffer size [%d].}",
           g_st_rx_buffer_size_stru.uc_rx_buffer_size_set_en,pst_rx_buffer_size_cfg->ul_value);

    return OAL_SUCC;
}


oal_uint32  hmac_config_auto_ba_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru              *pst_hmac_vap;
    oal_int32                   l_value;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_auto_ba_switch_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_int32 *)puc_param);

    /* 设置自动开启BA会话的开关，0代表关闭，1代表开启 */
    if (0 == l_value)
    {
        mac_mib_set_AddBaMode(pst_mac_vap, WLAN_ADDBA_MODE_MANUAL);
    }
    else
    {
        mac_mib_set_AddBaMode(pst_mac_vap, WLAN_ADDBA_MODE_AUTO);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_profiling_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_PROFILING_MODE
    oal_int32                   l_value;

    l_value = *((oal_int32 *)puc_param);

    /* 设置自动开启BA会话的开关，0代表关闭，1代表开启 */
    if (0 == l_value)
    {
        oam_profiling_set_switch(OAM_PROFILING_TX, OAM_PROFILING_SWITCH_OFF);
        oam_profiling_set_switch(OAM_PROFILING_RX, OAM_PROFILING_SWITCH_OFF);
    }
    else
    {
        oam_profiling_set_switch(OAM_PROFILING_TX, OAM_PROFILING_SWITCH_ON);
        oam_profiling_set_switch(OAM_PROFILING_RX, OAM_PROFILING_SWITCH_ON);
    }
#endif
    return OAL_SUCC;
}


oal_uint32  hmac_config_addba_req_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_addba_req_param_stru   *pst_addba_req;
    hmac_user_stru                 *pst_hmac_user;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_action_mgmt_args_stru       st_action_args;   /* 用于填写ACTION帧的参数 */
    oal_bool_enum_uint8             en_ampdu_support = OAL_FALSE;

    pst_addba_req = (mac_cfg_addba_req_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_addba_req_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_addba_req->auc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_addba_req_etc::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 建立BA会话，是否需要判断VAP的AMPDU的支持情况，因为需要实现建立BA会话时，一定发AMPDU */
    en_ampdu_support = hmac_user_xht_support(pst_hmac_user);

    /*手动创建ba会话，不受其他额外限制*/
    if (en_ampdu_support)
    {
        /*
            建立BA会话时，st_action_args(ADDBA_REQ)结构各个成员意义如下
            (1)uc_category:action的类别
            (2)uc_action:BA action下的类别
            (3)ul_arg1:BA会话对应的TID
            (4)ul_arg2:BUFFER SIZE大小
            (5)ul_arg3:BA会话的确认策略
            (6)ul_arg4:TIMEOUT时间
        */
        st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
        st_action_args.uc_action   = MAC_BA_ACTION_ADDBA_REQ;
        st_action_args.ul_arg1     = pst_addba_req->uc_tidno;       /* 该数据帧对应的TID号 */
        st_action_args.ul_arg2     = pst_addba_req->us_buff_size;   /* ADDBA_REQ中，buffer_size的默认大小 */
        st_action_args.ul_arg3     = pst_addba_req->en_ba_policy;   /* BA会话的确认策略 */
        st_action_args.ul_arg4     = pst_addba_req->us_timeout;     /* BA会话的超时时间设置为0 */

        /* 建立BA会话 */
        hmac_mgmt_tx_action_etc(pst_hmac_vap,  pst_hmac_user, &st_action_args);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_delba_req_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    mac_cfg_delba_req_param_stru   *pst_delba_req;
    hmac_user_stru                 *pst_hmac_user;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_action_mgmt_args_stru       st_action_args;   /* 用于填写ACTION帧的参数 */
    hmac_tid_stru                  *pst_hmac_tid;

    pst_delba_req = (mac_cfg_delba_req_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_delba_req->auc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_delba_req_etc::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_tid = &(pst_hmac_user->ast_tid_info[pst_delba_req->uc_tidno]);

    /* 查看会话是否存在 */
    if (MAC_RECIPIENT_DELBA == pst_delba_req->en_direction)
    {
        if(MAC_DELBA_TRIGGER_COMM == pst_delba_req->en_trigger)
        {
            pst_hmac_tid->en_ba_handle_rx_enable = OAL_FALSE;
        }

        if (OAL_PTR_NULL == pst_hmac_tid->pst_ba_rx_info)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_delba_req_etc::the rx hdl is not exist.}");
            return OAL_SUCC;
        }
    }
    else
    {
        if(MAC_DELBA_TRIGGER_COMM == pst_delba_req->en_trigger)
        {
            pst_hmac_tid->en_ba_handle_tx_enable = OAL_FALSE;
        }

        if (DMAC_BA_INIT == pst_hmac_tid->st_ba_tx_info.en_ba_status)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_delba_req_etc::the tx hdl is not exist.}");
            return OAL_SUCC;
        }
    }

    /*
        建立BA会话时，st_action_args(DELBA_REQ)结构各个成员意义如下
        (1)uc_category:action的类别
        (2)uc_action:BA action下的类别
        (3)ul_arg1:BA会话对应的TID
        (4)ul_arg2:删除ba会话的发起端
        (5)ul_arg3:删除ba会话的原因
        (6)ul_arg5:ba会话对应的用户
    */
    st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
    st_action_args.uc_action   = MAC_BA_ACTION_DELBA;
    st_action_args.ul_arg1     = pst_delba_req->uc_tidno;       /* 该数据帧对应的TID号 */
    st_action_args.ul_arg2     = pst_delba_req->en_direction;   /* ADDBA_REQ中，buffer_size的默认大小 */
    st_action_args.ul_arg3     = MAC_QSTA_TIMEOUT; /* BA会话的确认策略 */
    st_action_args.puc_arg5    = pst_delba_req->auc_mac_addr;   /* ba会话对应的user */

    /* 建立BA会话 */
    hmac_mgmt_tx_action_etc(pst_hmac_vap,  pst_hmac_user, &st_action_args);

    return OAL_SUCC;
}


oal_uint32  hmac_config_amsdu_start_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_amsdu_start_param_stru  *pst_amsdu_param;
    hmac_user_stru                  *pst_hmac_user;
    oal_uint8                        uc_tid_index;

    pst_amsdu_param = (mac_cfg_amsdu_start_param_stru *)puc_param;

    /* 获取用户对应的索引 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_amsdu_param->auc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
       OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_amsdu_start_etc::pst_hamc_user null.}");
       return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_tid_index = 0; uc_tid_index < WLAN_TID_MAX_NUM; uc_tid_index++)
    {
        hmac_amsdu_set_maxnum(&pst_hmac_user->ast_hmac_amsdu[uc_tid_index], pst_amsdu_param->uc_amsdu_max_num);
        hmac_amsdu_set_maxsize(&pst_hmac_user->ast_hmac_amsdu[uc_tid_index], pst_hmac_user, pst_amsdu_param->us_amsdu_max_size);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_user_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_user_stru                  *pst_hmac_user;
    oal_uint32                       ul_ret;
    oal_uint8                        uc_tid_index;
    oam_output_type_enum_uint8       en_output_type       = OAM_OUTPUT_TYPE_BUTT;
    mac_cfg_user_info_param_stru    *pst_hmac_event;

    pst_hmac_event = (mac_cfg_user_info_param_stru *)puc_param;
    pst_hmac_user  = (hmac_user_stru *)mac_res_get_hmac_user_etc(pst_hmac_event->us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_info_etc::pst_hmac_user[%d] null.}", pst_hmac_event->us_user_idx);
        return OAL_FAIL;
    }

    oam_get_output_type_etc(&en_output_type);
    if (OAM_OUTPUT_TYPE_SDT != en_output_type)
    {
        OAL_IO_PRINT("en_user_asoc_state :  %d \n", pst_hmac_user->st_user_base_info.en_user_asoc_state);
        OAL_IO_PRINT("uc_is_wds :           %d \n", pst_hmac_user->uc_is_wds);
        OAL_IO_PRINT("us_amsdu_maxsize :    %d \n", pst_hmac_user->us_amsdu_maxsize);
        OAL_IO_PRINT("11ac2g :              %d \n", pst_hmac_user->st_hmac_cap_info.bit_11ac2g);
        OAL_IO_PRINT("\n");

        for (uc_tid_index = 0; uc_tid_index < 8; uc_tid_index ++)
        {
            OAL_IO_PRINT("tid               %d \n", uc_tid_index);
            OAL_IO_PRINT("uc_amsdu_maxnum : %d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].uc_amsdu_maxnum);
            OAL_IO_PRINT("us_amsdu_maxsize :%d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].us_amsdu_maxsize);
            OAL_IO_PRINT("us_amsdu_size :   %d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].us_amsdu_size);
            OAL_IO_PRINT("uc_msdu_num :     %d \n", pst_hmac_user->ast_hmac_amsdu[uc_tid_index].uc_msdu_num);
            OAL_IO_PRINT("\n");
        }

        OAL_IO_PRINT("us_user_hash_idx :    %d \n", pst_hmac_user->st_user_base_info.us_user_hash_idx);
        OAL_IO_PRINT("us_assoc_id :         %d \n", pst_hmac_user->st_user_base_info.us_assoc_id);
        OAL_IO_PRINT("uc_vap_id :           %d \n", pst_hmac_user->st_user_base_info.uc_vap_id);
        OAL_IO_PRINT("uc_device_id :        %d \n", pst_hmac_user->st_user_base_info.uc_device_id);
        OAL_IO_PRINT("uc_chip_id :          %d \n", pst_hmac_user->st_user_base_info.uc_chip_id);
        OAL_IO_PRINT("uc_amsdu_supported :  %d \n", pst_hmac_user->uc_amsdu_supported);
        OAL_IO_PRINT("uc_htc_support :      %d \n", pst_hmac_user->st_user_base_info.st_ht_hdl.uc_htc_support);
        OAL_IO_PRINT("en_ht_support :       %d \n", pst_hmac_user->st_user_base_info.st_ht_hdl.en_ht_capable);
        OAL_IO_PRINT("short gi 20 40 80:    %d %d %d \n", pst_hmac_user->st_user_base_info.st_ht_hdl.bit_short_gi_20mhz,
                                                          pst_hmac_user->st_user_base_info.st_ht_hdl.bit_short_gi_40mhz,
                                                          pst_hmac_user->st_user_base_info.st_vht_hdl.bit_short_gi_80mhz);
        OAL_IO_PRINT("\n");

        OAL_IO_PRINT("Privacy info : \r\n");
        OAL_IO_PRINT("    port_valid   :                     %d \r\n",
                    pst_hmac_user->st_user_base_info.en_port_valid);
        OAL_IO_PRINT("    user_tx_info.security.cipher_key_type:      %s \r\n"
                     "    user_tx_info.security.cipher_protocol_type: %s \r\n",
                    hmac_config_keytype2string_etc(pst_hmac_user->st_user_base_info.st_user_tx_info.st_security.en_cipher_key_type),
                    hmac_config_cipher2string_etc(pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type));

        if(WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        {
            OAL_IO_PRINT("    STA:cipher_type :                           %s \r\n",
                    hmac_config_cipher2string_etc(pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type));
        }
        OAL_IO_PRINT("\n");


    }
    else
    {
        oam_ota_report_etc((oal_uint8 *)pst_hmac_user,
                       (oal_uint16)(OAL_SIZEOF(hmac_user_stru) - OAL_SIZEOF(mac_user_stru)),
                       0, 0, OAM_OTA_TYPE_HMAC_USER);
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USER_INFO, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_info_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_VOWIFI

oal_uint32  hmac_config_vowifi_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32            ul_ret;
    mac_cfg_vowifi_stru  *pst_cfg_vowifi;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_vowifi_info_etc::null param,pst_mac_vap=%d puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (OAL_PTR_NULL == pst_mac_vap->pst_vowifi_cfg_param)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_vowifi_info_etc::pst_vowifi_cfg_param is null.}");
        return OAL_SUCC;
    }

    pst_cfg_vowifi = (mac_cfg_vowifi_stru *)puc_param;

    ul_ret = mac_vap_set_vowifi_param_etc(pst_mac_vap, pst_cfg_vowifi->en_vowifi_cfg_cmd, pst_cfg_vowifi->uc_value);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vowifi_info_etc::param[%d] set failed[%d].}", pst_cfg_vowifi->en_vowifi_cfg_cmd, ul_ret);
        return ul_ret;
    }


    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_VOWIFI, "{hmac_config_vowifi_info_etc::Mode[%d],rssi_thres[%d],period_ms[%d],trigger_count[%d].}",
                    pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_mode,
                    ((VOWIFI_LOW_THRES_REPORT == pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_mode)? pst_mac_vap->pst_vowifi_cfg_param->c_rssi_low_thres : pst_mac_vap->pst_vowifi_cfg_param->c_rssi_high_thres),
                    pst_mac_vap->pst_vowifi_cfg_param->us_rssi_period_ms,
                    pst_mac_vap->pst_vowifi_cfg_param->uc_trigger_count_thres);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_VOWIFI_INFO, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_info_etc::hmac_config_vowifi_info_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
#ifdef _PRE_WLAN_FEATURE_IP_FILTER

oal_uint32 hmac_config_update_ip_filter_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                ul_ret;
    dmac_tx_event_stru       *pst_tx_event;
    frw_event_mem_stru       *pst_event_mem;
    oal_netbuf_stru          *pst_netbuf_cmd;
    frw_event_stru           *pst_hmac_to_dmac_ctx_event;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_update_ip_filter_etc::null param,pst_mac_vap=%d puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_netbuf_cmd = *((oal_netbuf_stru **)puc_param);
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_update_ip_filter_etc::pst_event_mem null.}");
        oal_netbuf_free(pst_netbuf_cmd);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_to_dmac_ctx_event = (frw_event_stru *)pst_event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(pst_hmac_to_dmac_ctx_event->st_event_hdr),
                    FRW_EVENT_TYPE_WLAN_CTX,
                    DMAC_WLAN_CTX_EVENT_SUB_TYPE_IP_FILTER,
                    OAL_SIZEOF(dmac_tx_event_stru),
                    FRW_EVENT_PIPELINE_STAGE_1,
                    pst_mac_vap->uc_chip_id,
                    pst_mac_vap->uc_device_id,
                    pst_mac_vap->uc_vap_id);

    pst_tx_event = (dmac_tx_event_stru *)(pst_hmac_to_dmac_ctx_event->auc_event_data);
    pst_tx_event->pst_netbuf    = pst_netbuf_cmd;
    pst_tx_event->us_frame_len  = OAL_NETBUF_LEN(pst_netbuf_cmd);

    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_update_ip_filter_etc::frw_event_dispatch_event_etc failed[%d].}", ul_ret);

    }
    oal_netbuf_free(pst_netbuf_cmd);
    FRW_EVENT_FREE(pst_event_mem);

    return ul_ret;
}

#endif //_PRE_WLAN_FEATURE_IP_FILTER

oal_uint32  hmac_config_kick_user_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;
    oal_uint16                      us_user_idx;
    oal_uint32                      ul_ret;
    hmac_user_stru                 *pst_hmac_user;
    oal_bool_enum_uint8             en_is_protected = OAL_FALSE;
    oal_dlist_head_stru            *pst_entry;
    oal_dlist_head_stru            *pst_dlist_tmp;
    mac_user_stru                  *pst_user_tmp;
    hmac_user_stru                 *pst_hmac_user_tmp;
    hmac_vap_stru                  *pst_hmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_kick_user_etc::null param,pst_mac_vap=%d puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_kick_user_etc::en_vap_mode is WLAN_VAP_MODE_CONFIG.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)puc_param;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_kick_user_etc::null param,pst_hmac_vap[%d].}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_kick_user_etc::kick user mac[%02X:XX:XX:XX:%02X:%02X] reason code[%d]}",
                     pst_kick_user_param->auc_mac_addr[0], pst_kick_user_param->auc_mac_addr[4], pst_kick_user_param->auc_mac_addr[5], pst_kick_user_param->us_reason_code);

    /* 踢掉全部user */
    if(oal_is_broadcast_ether_addr(pst_kick_user_param->auc_mac_addr))
    {
        
#if (_PRE_TEST_MODE == _PRE_TEST_MODE_UT)
        /* STAUT模式不需要发广播去关联帧，staut自身linkloss机制能保证ap突然下电等情况，能及时去关联；另外dfr重启wifi，staut发的此帧可能不释放，平台无法及时睡眠而出现异常 */
        if(IS_AP(pst_mac_vap))
        {
            hmac_mgmt_send_disassoc_frame_etc(pst_mac_vap, pst_kick_user_param->auc_mac_addr, pst_kick_user_param->us_reason_code, OAL_FALSE);
        }
#endif

        /* 遍历vap下所有用户, 删除用户 */
        OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
        {
            pst_user_tmp      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
            if (OAL_PTR_NULL == pst_user_tmp)
            {
                OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_kick_user_etc::pst_user_tmp null.}");
                continue;
            }

            pst_hmac_user_tmp = mac_res_get_hmac_user_etc(pst_user_tmp->us_assoc_id);
            if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_user_tmp))
            {
                OAM_ERROR_LOG1(0, OAM_SF_UM, "{hmac_config_kick_user_etc::null param,pst_hmac_user_tmp[%d].}",pst_user_tmp->us_assoc_id);
                continue;
            }

            /* 管理帧加密是否开启*/
            en_is_protected = pst_user_tmp->st_cap_info.bit_pmf_active;

            /* 发去关联帧 */
            hmac_mgmt_send_disassoc_frame_etc(pst_mac_vap, pst_user_tmp->auc_user_mac_addr, pst_kick_user_param->us_reason_code, en_is_protected);

            /* 修改 state & 删除 user */
            hmac_handle_disconnect_rsp_etc(pst_hmac_vap, pst_hmac_user_tmp, pst_kick_user_param->us_reason_code);

            /* 删除用户 */
            hmac_user_del_etc(pst_mac_vap, pst_hmac_user_tmp);
        }

        /* VAP下user头指针不应该为空 */
        if (OAL_FALSE == oal_dlist_is_empty(&pst_mac_vap->st_mac_user_list_head))
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_kick_user_etc::st_mac_user_list_head is not empty.}");
        }

        return OAL_SUCC;
    }

    ul_ret = mac_vap_find_user_by_macaddr_etc(pst_mac_vap, pst_kick_user_param->auc_mac_addr, &us_user_idx);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_kick_user_etc::mac_vap_find_user_by_macaddr_etc failed[%d].}",ul_ret);
        if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
        {
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        }
        return ul_ret;
    }

    pst_hmac_user = mac_res_get_hmac_user_etc(us_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_kick_user_etc::pst_hmac_user null,us_user_idx:%d}", us_user_idx);
        if(WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
        {
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_FAKE_UP);
        }
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (MAC_USER_STATE_ASSOC != pst_hmac_user->st_user_base_info.en_user_asoc_state)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_UM, "{hmac_config_kick_user_etc::the user is unassociated,us_user_idx:%d}", us_user_idx);
    }

    en_is_protected = pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active;

    /* 发去认证帧 */
    hmac_mgmt_send_disassoc_frame_etc(pst_mac_vap, pst_hmac_user->st_user_base_info.auc_user_mac_addr, pst_kick_user_param->us_reason_code, en_is_protected);

    /* 修改 state & 删除 user */
    hmac_handle_disconnect_rsp_etc(pst_hmac_vap, pst_hmac_user, pst_kick_user_param->us_reason_code);

    /* 删除用户 */
    hmac_user_del_etc(pst_mac_vap, pst_hmac_user);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PROXYSTA

oal_uint32  hmac_config_set_oma(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_oma_param_stru     *pst_set_oma_param;
    hmac_vap_stru                  *pst_hmac_vap;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_oma::null param,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (!pst_hmac_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_set_oma::null hmac_vap,vapid=%d puc_param=%d.}", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_set_oma_param = (mac_cfg_set_oma_param_stru *)puc_param;

    /* 设置proxy sta的oma地址 */
    oal_set_mac_addr(hmac_vap_psta_oma(pst_hmac_vap), pst_set_oma_param->auc_mac_addr);

    return OAL_SUCC;
}


oal_uint32  hmac_config_proxysta_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    oal_uint32       ul_value;
    oal_uint32       ul_ret;
    mac_device_stru *pst_mac_device;

    ul_value = *((oal_uint32 *)puc_param);

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_proxysta_switch::mac_res_get_dev_etc null.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置proxysta模块使能开关 */
    mac_is_proxysta_enabled(pst_mac_device) = ul_value ? OAL_TRUE : OAL_FALSE;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PROXYSTA_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_proxysta_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#endif


oal_uint32  hmac_config_set_tx_pow_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_TX_POW, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_tx_pow_param::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_dscr_param_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_DSCR, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_dscr_param_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_log_level_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_LOG_LEVEL, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_log_level_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_rate_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_RATE, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_rate_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_set_mcs_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_MCS, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mcs_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_mcsac_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_MCSAC, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mcsac_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#ifdef _PRE_WLAN_FEATURE_11AX
oal_uint32  hmac_config_set_mcsax(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_MCSAX, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mcsac_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif



oal_uint32  hmac_config_set_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_NSS, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_nss::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_rfch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_RFCH, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_rfch_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_set_bw_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_BW, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_bw_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_always_tx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_TX, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_tx::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_always_tx_hw(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_TX_HW, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_tx::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_always_tx_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_TX_NUM, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_tx::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_always_rx_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_RX, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_rx_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

oal_uint32  hmac_config_always_tx_51(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
   oal_uint32                       ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_TX_51, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_tx::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_always_rx_51(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALWAYS_RX_51, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_rx_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_DEBUG_MODE

oal_uint32 hmac_config_set_rxch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret = OAL_SUCC;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_RXCH, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_rxch::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32 hmac_config_dync_txpower(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DYNC_TXPOWER, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dync_txpower::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32 hmac_config_dync_pow_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DYNC_POW_DEBUG, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dync_pow_debug_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif


oal_uint32  hmac_config_get_thruput(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_THRUPUT, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_thruput::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_freq_skew(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_FREQ_SKEW, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_freq_skew::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_adjust_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    mac_cfg_adjust_ppm_stru         *pst_adjust_ppm;
    mac_device_stru                 *pst_device;

    pst_device= mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_adjust_ppm = (mac_cfg_adjust_ppm_stru*)puc_param;
    pst_device->c_ppm_val = pst_adjust_ppm->c_ppm_val;
#endif
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ADJUST_PPM, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_adjust_ppm::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_get_ppm(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_device;

    pst_device= mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    *((oal_int8 *)puc_param) = pst_device->c_ppm_val;
#else
    *((oal_int8 *)puc_param) = 0;
#endif
    *pus_len = OAL_SIZEOF(oal_int8);

    return OAL_SUCC;
}

oal_uint32  hmac_config_pcie_pm_level_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PCIE_PM_LEVEL, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_adjust_ppm::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_list_ap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_ap_etc::invalid vap mode[%d].}", pst_mac_vap->en_vap_mode);
        return OAL_FAIL;
    }

    /* 打印扫描到的bss信息 */
    hmac_scan_print_scanned_bss_info_etc(pst_mac_vap->uc_device_id);

    return OAL_SUCC;
}


oal_uint32  hmac_config_list_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint16                  us_user_idx;
    mac_user_stru              *pst_mac_user;
    oal_dlist_head_stru        *pst_head;
    wlan_protocol_enum_uint8    en_protocol_mode;
    oal_int8                    ac_tmp_buff[256]    = {0};
    oal_int32                   l_remainder_len    = 0;
    oal_int8                   *pc_print_tmp = OAL_PTR_NULL;
    oal_int8                   *pc_print_buff;
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    oal_int8                   *pc_print_buff2;
    oal_int8                   *pc_print_buff3;
    oal_int8                   *pc_print_buff4;
#endif

    /* AP侧的信息才能打印相关信息 */
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_sta_etc::invalid en_vap_mode[%d].}", pst_mac_vap->en_vap_mode);
        return OAL_FAIL;
    }

    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);

    OAL_SPRINTF(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, "Total user num is %d \n", pst_mac_vap->us_user_nums);
    oal_strcat(pc_print_buff, "User assoc id         ADDR         Protocol Type \n");

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    pc_print_buff2 = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    pc_print_buff3 = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    pc_print_buff4 = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff2
        || OAL_PTR_NULL == pc_print_buff3
        || OAL_PTR_NULL == pc_print_buff4)
    {
        /* 释放所有内存 */
        OAL_MEM_FREE(pc_print_buff2, OAL_TRUE);
        OAL_MEM_FREE(pc_print_buff3, OAL_TRUE);
        OAL_MEM_FREE(pc_print_buff4, OAL_TRUE);
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(pc_print_buff2, OAM_REPORT_MAX_STRING_LEN);
    OAL_MEMZERO(pc_print_buff3, OAM_REPORT_MAX_STRING_LEN);
    OAL_MEMZERO(pc_print_buff4, OAM_REPORT_MAX_STRING_LEN);
#endif

    pc_print_tmp = pc_print_buff;
    l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_tmp));

    oal_spin_lock_bh(&pst_mac_vap->st_cache_user_lock);

    /* AP侧的USER信息 */
    for (us_user_idx = 0; us_user_idx < MAC_VAP_USER_HASH_MAX_VALUE; us_user_idx++)
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_head, &(pst_mac_vap->ast_user_hash[us_user_idx]))
        {
            /* 找到相应用户 */
            pst_mac_user = (mac_user_stru *)OAL_DLIST_GET_ENTRY(pst_head, mac_user_stru, st_user_hash_dlist);

            if (OAL_PTR_NULL == pst_mac_user)
            {
               OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_sta_etc::pst_mac_user null.}");
               continue;
            }
            /* user结构体下的协议模式不区分a和g，需要根据频段区分 */
            en_protocol_mode = pst_mac_user->en_protocol_mode;
            if (en_protocol_mode >= WLAN_PROTOCOL_BUTT)
            {
                OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_sta_etc:: protocol_mode wrong.}",en_protocol_mode);
                continue;
            }
            if ((WLAN_LEGACY_11G_MODE == en_protocol_mode)  && (WLAN_BAND_5G == pst_mac_vap->st_channel.en_band))
            {
                en_protocol_mode = WLAN_LEGACY_11A_MODE;
            }

            OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), "     %d       %02X:XX:XX:%02X:%02X:%02X       %s \n",
                        pst_mac_user->us_assoc_id,
                        pst_mac_user->auc_user_mac_addr[0],
                        pst_mac_user->auc_user_mac_addr[3],
                        pst_mac_user->auc_user_mac_addr[4],
                        pst_mac_user->auc_user_mac_addr[5],
                        gst_protocol_mode_list[en_protocol_mode].puc_protocol_desc);

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
            /* if buffer not enough(only in user extend mode), use next pc_print_buff(2/3/4) */
            if (l_remainder_len-1 <= (oal_int32)OAL_STRLEN(ac_tmp_buff))
            {
                pc_print_tmp = (0 == OAL_STRLEN(pc_print_buff2)) ? pc_print_buff2:
                               (0 == OAL_STRLEN(pc_print_buff3)) ? pc_print_buff3:
                               pc_print_buff4;
                l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_tmp));
            }
#endif
            oal_strncat(pc_print_tmp, ac_tmp_buff, l_remainder_len-1);
            OAL_MEMZERO(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff));
            l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_tmp));

        }

    }

    oal_spin_unlock_bh(&pst_mac_vap->st_cache_user_lock);

    oam_print_etc(pc_print_buff);
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    oam_print_etc(pc_print_buff2);
    oam_print_etc(pc_print_buff3);
    oam_print_etc(pc_print_buff4);
#endif
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
    OAL_MEM_FREE(pc_print_buff2, OAL_TRUE);
    OAL_MEM_FREE(pc_print_buff3, OAL_TRUE);
    OAL_MEM_FREE(pc_print_buff4, OAL_TRUE);
#endif
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_sta_list_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    oal_uint16                  us_user_idx;
    mac_user_stru              *pst_mac_user;
    oal_dlist_head_stru        *pst_head;
    oal_int8                    ac_tmp_buff[256]    = {0};
    oal_int32                   l_remainder_len    = 0;
    oal_int8                   *pc_sta_list_buff;
    oal_netbuf_stru*            pst_netbuf;
    oal_uint32                  ul_netbuf_len;

    /* 事件传递指针值，此处异常返回前将其置为NULL */
    *(oal_ulong*)puc_param = (oal_ulong)OAL_PTR_NULL;

    /* AP侧的信息才能打印相关信息 */
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_list_etc::invalid en_vap_mode[%d].}", pst_mac_vap->en_vap_mode);
        return OAL_FAIL;
    }

    pc_sta_list_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_sta_list_buff)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_sta_list_etc, OAL_MEM_ALLOC failed.\n");
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEMZERO(pc_sta_list_buff, OAM_REPORT_MAX_STRING_LEN);
    l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_sta_list_buff));

    oal_spin_lock_bh(&pst_mac_vap->st_cache_user_lock);

    /* AP侧的USER信息 */
    for (us_user_idx = 0; us_user_idx < MAC_VAP_USER_HASH_MAX_VALUE; us_user_idx++)
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_head, &(pst_mac_vap->ast_user_hash[us_user_idx]))
        {
            /* 找到相应用户 */
            pst_mac_user = (mac_user_stru *)OAL_DLIST_GET_ENTRY(pst_head, mac_user_stru, st_user_hash_dlist);
            if (OAL_PTR_NULL == pst_mac_user)
            {
               OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_list_etc::pst_mac_user null.}");
               continue;
            }
            /* 检查用户关联状态 */
            if (MAC_USER_STATE_ASSOC != pst_mac_user->en_user_asoc_state)
            {
               continue;
            }
            OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), "%02X:%02X:%02X:%02X:%02X:%02X\n",
                        pst_mac_user->auc_user_mac_addr[0],
                        pst_mac_user->auc_user_mac_addr[1],
                        pst_mac_user->auc_user_mac_addr[2],
                        pst_mac_user->auc_user_mac_addr[3],
                        pst_mac_user->auc_user_mac_addr[4],
                        pst_mac_user->auc_user_mac_addr[5]);

            OAL_IO_PRINT("hmac_config_get_sta_list_etc,STA:%02X:XX:XX:%02X:%02X:%02X\n",
                    pst_mac_user->auc_user_mac_addr[0],
                    pst_mac_user->auc_user_mac_addr[3],
                    pst_mac_user->auc_user_mac_addr[4],
                    pst_mac_user->auc_user_mac_addr[5]);

            oal_strncat(pc_sta_list_buff, ac_tmp_buff, l_remainder_len-1);
            OAL_MEMZERO(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff));
            l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_sta_list_buff));

        }
    }

    oal_spin_unlock_bh(&pst_mac_vap->st_cache_user_lock);

    ul_netbuf_len = OAL_STRLEN(pc_sta_list_buff);
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF,ul_netbuf_len, OAL_NETBUF_PRIORITY_MID);
    if(OAL_PTR_NULL!= pst_netbuf)
    {
        oal_memcopy(oal_netbuf_put(pst_netbuf, ul_netbuf_len), pc_sta_list_buff, ul_netbuf_len);
    }
    else
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_sta_list_etc::Alloc netbuf(size %d) NULL in normal_netbuf_pool!",
                        ul_netbuf_len);
    }

    *(oal_ulong*)puc_param = (oal_ulong)pst_netbuf;

    /* 事件传递指针，此处记录指针长度 */
    *us_len = (oal_uint16)OAL_SIZEOF(pst_netbuf);

    OAL_MEM_FREE(pc_sta_list_buff, OAL_TRUE);
    return OAL_SUCC;
}



oal_uint32  hmac_config_dump_ba_bitmap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dump_ba_bitmap::config vap do not have ba bitmap.}");
        return OAL_FAIL;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DUMP_BA_BITMAP, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dump_ba_bitmap::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_dump_all_rx_dscr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DUMP_ALL_RX_DSCR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dump_all_rx_dscr_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_vap_pkt_stat_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

    return oam_report_vap_pkt_stat_to_sdt_etc(pst_mac_vap->uc_vap_id);
#else
    return OAL_SUCC;
#endif

}


oal_uint32  hmac_config_set_country_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_country_stru      *pst_country_param;
    mac_regdomain_info_stru   *pst_mac_regdom;
    mac_device_stru           *pst_mac_device;
    oal_uint32                 ul_ret;
    oal_uint8                uc_rc_num;
    oal_uint32               ul_size;
#ifdef _PRE_WLAN_FEATURE_DFS
    oal_int8                 *pc_current_country;
#endif

    pst_country_param = (mac_cfg_country_stru *)puc_param;
    pst_mac_regdom    = (mac_regdomain_info_stru *)pst_country_param->p_mac_regdom;
    if(OAL_PTR_NULL == pst_mac_regdom)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_country_etc::pst_mac_regdom null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        if(OAL_PTR_NULL != pst_mac_regdom)
        {
            OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);
            pst_mac_regdom = OAL_PTR_NULL;
        }
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_country_etc::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_country_etc::country code=%c%c sideband_flag[%d].}",
                     (oal_uint8)pst_mac_regdom->ac_country[0],(oal_uint8)pst_mac_regdom->ac_country[1], pst_mac_regdom->en_regdomain);

#ifdef _PRE_WLAN_FEATURE_DFS
    pc_current_country = mac_regdomain_get_country_etc();
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    pst_mac_device->st_dfs.st_dfs_info.en_dfs_init = OAL_FALSE;
#endif
    /* 当前国家码与要设置的国家码不一致，需要重新初始化雷达信道 */
    if ((pst_mac_regdom->ac_country[0] != pc_current_country[0])
        || (pst_mac_regdom->ac_country[1] != pc_current_country[1]))
    {
        pst_mac_device->st_dfs.st_dfs_info.en_dfs_init = OAL_FALSE;
    }
#endif

    mac_regdomain_set_country_etc(us_len, puc_param);

#ifdef _PRE_WLAN_FEATURE_DFS
    /* 只有5G 芯片才进行雷达信道初始化 */
    if ((OAL_FALSE == pst_mac_device->st_dfs.st_dfs_info.en_dfs_init)
#ifdef _PRE_WLAN_FEATURE_DOUBLE_CHIP
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
        && (g_uc_wlan_double_chip_5g_id == pst_mac_device->uc_chip_id)
#endif
#endif
       )
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_country_etc::hmac_dfs_channel_list_init_etc.}");
        hmac_dfs_channel_list_init_etc(pst_mac_device);
        pst_mac_device->st_dfs.st_dfs_info.en_dfs_init = OAL_TRUE;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        hmac_config_ch_status_sync(pst_mac_device);
#endif
    }
#endif

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/

    /* 获取管制类的个数 */
    uc_rc_num = pst_mac_regdom->uc_regclass_num;

    /* 计算配置命令 */
    ul_size = (oal_uint32)(OAL_SIZEOF(mac_regclass_info_stru) * uc_rc_num + MAC_RD_INFO_LEN);

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_COUNTRY, (oal_uint16)ul_size, (oal_uint8 *)pst_mac_regdom);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        if(OAL_PTR_NULL != pst_mac_regdom)
        {
            OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);
            pst_mac_regdom = OAL_PTR_NULL;
        }
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_country_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);

        return ul_ret;
    }

    /* WAL层抛内存下来，此处释放 */
    if(OAL_PTR_NULL != pst_mac_regdom)
    {
        OAL_MEM_FREE(pst_mac_regdom, OAL_TRUE);
        pst_mac_regdom = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}



oal_uint32  hmac_config_set_amsdu_tx_on_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
#ifdef _PRE_WLAN_FEATURE_AMSDU
    mac_cfg_ampdu_tx_on_param_stru *pst_ampdu_tx_on_param;
    hmac_vap_stru                  *pst_hmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_set_amsdu_tx_on_etc:: parma null ptr!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_amsdu_tx_on_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_ampdu_tx_on_param = (mac_cfg_ampdu_tx_on_param_stru *)puc_param;

    mac_mib_set_CfgAmsduTxAtive(&pst_hmac_vap->st_vap_base_info, pst_ampdu_tx_on_param->uc_aggr_tx_on);

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_amsdu_tx_on_etc::ENABLE[%d].}", pst_ampdu_tx_on_param->uc_aggr_tx_on);
#endif

    return OAL_SUCC;
}



oal_uint32  hmac_config_set_ampdu_tx_on_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_ampdu_tx_on_param_stru     *pst_ampdu_tx_on;
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    hmac_vap_stru                      *pst_hmac_vap;
    oal_uint8                           uc_tidno;
    oal_uint8                           uc_vap_idx;
    oal_uint8                           uc_device;
    oal_uint8                           uc_device_max;
    hmac_user_stru                     *pst_hmac_user;
    mac_chip_stru                      *pst_mac_chip;
    mac_device_stru                    *pst_mac_device;
    oal_dlist_head_stru                *pst_entry;
    oal_dlist_head_stru                *pst_next_entry;
    mac_user_stru                      *pst_mac_user;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_ampdu_tx_on_etc:: param null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ampdu_tx_on = (mac_cfg_ampdu_tx_on_param_stru *)puc_param;

    OAM_WARNING_LOG3(0, OAM_SF_CFG, "{hmac_config_set_ampdu_tx_on_etc:: tx_aggr_on[0x%x], snd type[%d],aggr mode[%d]!}",
              pst_ampdu_tx_on->uc_aggr_tx_on, pst_ampdu_tx_on->uc_snd_type, pst_ampdu_tx_on->en_aggr_switch_mode);

    /* ampdu_tx_on为0、1,删建聚合 */
    if (0 == (oal_uint8)(pst_ampdu_tx_on->uc_aggr_tx_on & (~(BIT1 | BIT0))))
    {
        mac_mib_set_CfgAmpduTxAtive(pst_mac_vap, pst_ampdu_tx_on->uc_aggr_tx_on & BIT0);

        /* ampdu_tx_on为2、3,删建聚合,并且切换硬件聚合 */
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
        /* 切换为硬件聚合时才需要下发事件 */
        if (pst_ampdu_tx_on->uc_aggr_tx_on & BIT1)
        {
            pst_ampdu_tx_on->uc_aggr_tx_on &= BIT0; /* enable hw ampdu */
            hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_AMPDU_TX_ON, us_len, puc_param);
        }
#endif
        return OAL_SUCC;
    }


#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
    /* 1.无需删建BA切换方式 */
    if (AMPDU_SWITCH_BY_BA_LUT == pst_ampdu_tx_on->en_aggr_switch_mode)
    {
        pst_ampdu_tx_on->uc_aggr_tx_on  &= BIT2; /* 4:enable hw ampdu; 8:disable */
        pst_ampdu_tx_on->uc_aggr_tx_on = pst_ampdu_tx_on->uc_aggr_tx_on >> 2;
        hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_AMPDU_TX_ON, us_len, puc_param);
        return OAL_SUCC;
    }

    /* 2.需删建BA切换方式 */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_ampdu_tx_on_etc:: pst_hmac_vap null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_ampdu_tx_on->uc_aggr_tx_on & (~(BIT1 | BIT0)))
    {
        /* 提交切换硬件聚合work */
        oal_memcopy(&pst_hmac_vap->st_mode_set, pst_ampdu_tx_on, OAL_SIZEOF(mac_cfg_ampdu_tx_on_param_stru));
        oal_workqueue_delay_schedule(&(pst_hmac_vap->st_set_hw_work), OAL_MSECS_TO_JIFFIES(2000));

        /* 遍历删除BA */
        pst_mac_chip = hmac_res_get_mac_chip(pst_mac_vap->uc_chip_id);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_chip))
        {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_ampdu_tx_on_etc:: pst_mac_chip null!}");
            return OAL_ERR_CODE_PTR_NULL;
        }
        uc_device_max = oal_chip_get_device_num_etc(pst_mac_chip->ul_chip_ver);

        for (uc_device = 0; uc_device < uc_device_max; uc_device++)
        {
            pst_mac_device = mac_res_get_dev_etc(pst_mac_chip->auc_device_id[uc_device]);
            if (OAL_PTR_NULL == pst_mac_device)
            {
                continue;
            }

            for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
            {
                pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
                if (OAL_PTR_NULL == pst_hmac_vap)
                {
                    continue;
                }

                oal_spin_lock_bh(&pst_hmac_vap->st_ampdu_lock);

                /* 先删除聚合,切换完毕后再使能聚合 */
                mac_mib_set_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info, OAL_FALSE);

                OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_next_entry, &(pst_hmac_vap->st_vap_base_info.st_mac_user_list_head))
                {
                    pst_mac_user      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
                    if (OAL_PTR_NULL == pst_mac_user)
                    {
                        continue;
                    }

                    pst_hmac_user = mac_res_get_hmac_user_etc(pst_mac_user->us_assoc_id);
                    if (OAL_PTR_NULL == pst_hmac_user)
                    {
                        continue;
                    }

                    for (uc_tidno = 0; uc_tidno < WLAN_TID_MAX_NUM; uc_tidno++)
                    {
                       if (DMAC_BA_COMPLETE == pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status)
                       {
                           hmac_tx_ba_del(pst_hmac_vap, pst_hmac_user, uc_tidno);
                       }
                    }
                }

                oal_spin_unlock_bh(&pst_hmac_vap->st_ampdu_lock);
                oal_workqueue_delay_schedule(&(pst_hmac_vap->st_ampdu_work), OAL_MSECS_TO_JIFFIES(3000));
            }
        }
    }
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_ampdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_uint32 *)puc_param) = mac_mib_get_CfgAmpduTxAtive(pst_mac_vap);
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_amsdu_tx_on(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    *((oal_uint32 *)puc_param) = mac_mib_get_CfgAmsduTxAtive(pst_mac_vap);
    *pus_len = OAL_SIZEOF(oal_int32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_hide_ssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8             uc_hide_ssid;
    oal_uint32            ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_hide_ssid::param null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_hide_ssid = *(oal_uint8 *)puc_param;
    mac_vap_set_hide_ssid_etc(pst_mac_vap, uc_hide_ssid);
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{dmac_config_hide_ssid::mac_vap_set_hide_ssid_etc [%d].}", uc_hide_ssid);

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_HIDE_SSID, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_hide_ssid::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_country_for_dfs_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_COUNTRY_FOR_DFS, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_country_for_dfs_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_regdomain_pwr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_regdomain_max_pwr_stru *pst_cfg;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32                      ul_ret;
#endif
    pst_cfg = (mac_cfg_regdomain_max_pwr_stru *)puc_param;

    mac_regdomain_set_max_power_etc(pst_cfg->uc_pwr, pst_cfg->en_exceed_reg);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_REGDOMAIN_PWR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_regdomain_pwr_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);

        return ul_ret;
    }

#endif
    return OAL_SUCC;

}

#ifdef _PRE_WLAN_FEATURE_TPC_OPT

oal_uint32  hmac_config_reduce_sar_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_REDUCE_SAR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_reduce_sar_etc::hmac_config_send_event_etc failed, error no[%d]!", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH

oal_uint32  hmac_config_tas_pwr_ctrl(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TAS_PWR_CTRL, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_tas_pwr_ctrl::hmac_config_send_event_etc failed, error no[%d]!", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH

oal_uint32 hmac_config_tas_rssi_access(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TAS_RSSI_ACCESS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_tas_rssi_access::hmac_config_send_event_etc failed, error no[%d]!", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_get_country_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
#if 1
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_int8      ac_tmp_buff[OAM_PRINT_FORMAT_LENGTH];
    mac_regdomain_info_stru *pst_regdomain_info                     = OAL_PTR_NULL;


    mac_get_regdomain_info_etc(&pst_regdomain_info);

    OAL_SPRINTF(ac_tmp_buff, sizeof(ac_tmp_buff), "getcountry code is : %c%c.\n", pst_regdomain_info->ac_country[0], pst_regdomain_info->ac_country[1]);
    oam_print_etc(ac_tmp_buff);
#else
    oal_int8                 *pc_curr_cntry;
    mac_cfg_get_country_stru *pst_param;

    pst_param = (mac_cfg_get_country_stru *)puc_param;

    pc_curr_cntry = mac_regdomain_get_country_etc();

    pst_param->ac_country[0] = pc_curr_cntry[0];
    pst_param->ac_country[1] = pc_curr_cntry[1];
    pst_param->ac_country[2] = pc_curr_cntry[2];

    *pus_len = OAL_SIZEOF(mac_cfg_get_country_stru);

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_country_etc::country[0]=%c, country[1]=%c.}",
                  (oal_uint8)pst_param->ac_country[0], (oal_uint8)pst_param->ac_country[1]);
#endif
#endif
    OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_country_etc");

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_free_connect_param_resource(mac_conn_param_stru *pst_conn_param)
{
    if (OAL_PTR_NULL != pst_conn_param->puc_wep_key)
    {
        oal_free(pst_conn_param->puc_wep_key);
        pst_conn_param->puc_wep_key = OAL_PTR_NULL;
    }
    if (OAL_PTR_NULL != pst_conn_param->puc_ie)
    {
        oal_free(pst_conn_param->puc_ie);
        pst_conn_param->puc_ie = OAL_PTR_NULL;
    }
}


oal_uint32  hmac_config_connect_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                          ul_ret;
    hmac_vap_stru                      *pst_hmac_vap;
    mac_bss_dscr_stru                  *pst_bss_dscr = OAL_PTR_NULL;
    mac_conn_param_stru                *pst_connect_param;
#ifdef _PRE_WLAN_FEATURE_11R
    oal_uint8                          *puc_mde;
    oal_uint8                           uc_akm_type;
    oal_uint32                          ul_akm_suite;
#endif
    oal_app_ie_stru                     st_app_ie;
    mac_conn_security_stru              st_conn_sec;
#ifdef _PRE_WLAN_FEATURE_WAPI
    mac_device_stru                    *pst_mac_device;
#endif
#ifdef _PRE_WLAN_FEATURE_ROAM
    hmac_user_stru                     *pst_hmac_user;
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_connect_etc:: connect failed,puc_param is null!}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_connect_param = (mac_conn_param_stru *)puc_param;
    if (us_len != OAL_SIZEOF(mac_conn_param_stru))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_connect_etc:: connect failed, unexpected param len ! [%x]!}\r\n", us_len);
        hmac_free_connect_param_resource(pst_connect_param);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    if (pst_connect_param->ul_ie_len > WLAN_WPS_IE_MAX_SIZE)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_connect_etc:: connect failed, pst_connect_param ie_len[%x] error!}\r\n", pst_connect_param->ul_ie_len);
        hmac_free_connect_param_resource(pst_connect_param);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* TBD 先判断其他VAP 的状态是否允许本VAP 入网连接 */
    /* 如果允许本VAP 入网，则返回设备忙状态           */

    /* 根据内核下发的关联能力，赋值加密相关的mib 值 */
    /* 根据下发的join,提取出安全相关的内容 */
    OAL_MEMZERO(&st_conn_sec, sizeof(mac_conn_security_stru));
    st_conn_sec.uc_wep_key_len        = pst_connect_param->uc_wep_key_len;
    st_conn_sec.en_auth_type          = pst_connect_param->en_auth_type;
    st_conn_sec.en_privacy            = pst_connect_param->en_privacy;
    st_conn_sec.st_crypto             = pst_connect_param->st_crypto;
    st_conn_sec.uc_wep_key_index      = pst_connect_param->uc_wep_key_index;
    st_conn_sec.en_mgmt_proteced      = pst_connect_param->en_mfp;
    if (st_conn_sec.uc_wep_key_len > WLAN_WEP104_KEY_LEN)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
                       "{hmac_config_connect_etc:: wep_key_len[%d] > WLAN_WEP104_KEY_LEN!}\r\n", st_conn_sec.uc_wep_key_len);
        st_conn_sec.uc_wep_key_len = WLAN_WEP104_KEY_LEN;
    }
    oal_memcopy(st_conn_sec.auc_wep_key, pst_connect_param->puc_wep_key, st_conn_sec.uc_wep_key_len);

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    st_conn_sec.en_pmf_cap = mac_get_pmf_cap_etc(pst_connect_param->puc_ie, pst_connect_param->ul_ie_len);
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_connect_etc:: connect param en_mfp[%d] pmf_cap[%d]!}\r\n",
                    st_conn_sec.en_mgmt_proteced, st_conn_sec.en_pmf_cap);
#endif
    st_conn_sec.en_wps_enable = OAL_FALSE;
    if (mac_find_vendor_ie_etc(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, pst_connect_param->puc_ie, (oal_int32)(pst_connect_param->ul_ie_len)))
    {
        st_conn_sec.en_wps_enable = OAL_TRUE;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc::connect failed, pst_hmac_vap null.uc_vap_id[%d]}",
                       pst_mac_vap->uc_vap_id);
        hmac_free_connect_param_resource(pst_connect_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    mac_mib_set_AuthenticationMode(pst_mac_vap, st_conn_sec.en_auth_type);

#ifdef _PRE_WLAN_FEATURE_11R
   if(OAL_TRUE == pst_hmac_vap->bit_11r_enable)
   {
        ul_akm_suite = OAL_NTOH_32(st_conn_sec.st_crypto.aul_akm_suite[0]);
        uc_akm_type  = ul_akm_suite & 0xFF;
        if ((WLAN_AUTH_SUITE_FT_1X == uc_akm_type) ||
           (WLAN_AUTH_SUITE_FT_PSK == uc_akm_type) ||
           (WLAN_AUTH_SUITE_FT_SHA256 == uc_akm_type))
        {
            mac_mib_set_AuthenticationMode(&pst_hmac_vap->st_vap_base_info, WLAN_WITP_AUTH_FT);
        }
   }
#endif

    /* 获取扫描的bss信息 */
    pst_bss_dscr = (mac_bss_dscr_stru *)hmac_scan_get_scanned_bss_by_bssid(pst_mac_vap, pst_connect_param->auc_bssid);
    if (OAL_PTR_NULL == pst_bss_dscr)
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_connect_etc::find the bss failed by bssid:%02X:XX:XX:%02X:%02X:%02X}",
                         pst_connect_param->auc_bssid[0],
                         pst_connect_param->auc_bssid[3],
                         pst_connect_param->auc_bssid[4],
                         pst_connect_param->auc_bssid[5]);
        hmac_free_connect_param_resource(pst_connect_param);
        return OAL_FAIL;
    }



#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
    /* check bssid blacklist from Framework/WIFI HAL Configuration */
    ul_ret = hmac_blacklist_filter_etc(pst_mac_vap, pst_bss_dscr->auc_bssid);
    if (OAL_TRUE == ul_ret)
    {
        hmac_free_connect_param_resource(pst_connect_param);
        return OAL_FAIL;
    }
#endif
#endif

    if (oal_memcmp(pst_connect_param->auc_ssid, pst_bss_dscr->ac_ssid, (oal_uint32)pst_connect_param->uc_ssid_len))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc::find the bss failed by ssid.}");
        hmac_free_connect_param_resource(pst_connect_param);
        return OAL_FAIL;
    }

    if ((pst_connect_param->uc_channel != pst_bss_dscr->st_channel.uc_chan_number)
        && (0 != pst_connect_param->uc_channel))
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_connect_etc::p2p_mode=%d, connect req channel=%u, pst_bss_dscr chan_number=%u in scan results.}",
            pst_mac_vap->en_p2p_mode, pst_connect_param->uc_channel, pst_bss_dscr->st_channel.uc_chan_number);
        hmac_free_connect_param_resource(pst_connect_param);
        return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_HS20
    if (oal_memcmp(pst_mac_vap->auc_bssid, pst_connect_param->auc_bssid, OAL_MAC_ADDR_LEN) ||
            (hmac_interworking_check(pst_hmac_vap, (oal_uint8 *)pst_bss_dscr)))
    {
        pst_hmac_vap->bit_reassoc_flag = OAL_FALSE;
    }
    else
    {
        pst_hmac_vap->bit_reassoc_flag = OAL_TRUE;
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc:: assoc ap with ressoc frame.}");
    }

     /* TBD passpoint网络中暂不考虑重关联 */
#endif  //_PRE_WLAN_FEATURE_HS20
    pst_hmac_vap->bit_reassoc_flag = OAL_FALSE;

#ifdef _PRE_WLAN_FEATURE_ROAM
    if (MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state)
    {
        /* 通知ROAM主状态机, ABORT Roaming FSM */
        hmac_roam_connect_complete_etc(pst_hmac_vap, OAL_FAIL);

        /* After roam_to_old_bss, pst_mac_vap->en_vap_state should be MAC_VAP_STATE_UP,
         * hmac_roam_info_stru     *pst_roam_info;
         * pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
         * pst_roam_info->en_main_state and pst_roam_info->st_connect.en_state should be 0 */
    }

    if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state)
    {
        /* 相同ssid时，走漫游流程 */
        if (!oal_memcmp(mac_mib_get_DesiredSSID(pst_mac_vap), pst_connect_param->auc_ssid, pst_connect_param->uc_ssid_len)
            && (OAL_STRLEN(mac_mib_get_DesiredSSID(pst_mac_vap)) == pst_connect_param->uc_ssid_len))
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc:: roaming AP with ressoc frame, XX:XX:XX:%02X:%02X:%02X}",
                             pst_connect_param->auc_bssid[3], pst_connect_param->auc_bssid[4], pst_connect_param->auc_bssid[5]);

            if (!oal_memcmp(pst_mac_vap->auc_bssid, pst_connect_param->auc_bssid, OAL_MAC_ADDR_LEN)) {
                /* reassociation */
                hmac_free_connect_param_resource(pst_connect_param);
                return hmac_roam_start_etc(pst_hmac_vap, ROAM_SCAN_CHANNEL_ORG_0, OAL_TRUE, ROAM_TRIGGER_APP);
            }
            else /* roaming */{
                hmac_free_connect_param_resource(pst_connect_param);
                return hmac_roam_start_etc(pst_hmac_vap, ROAM_SCAN_CHANNEL_ORG_0, OAL_FALSE, ROAM_TRIGGER_APP);
            }
        }

        /* 先删除用户，再connect */
        pst_hmac_user = mac_res_get_hmac_user_etc(pst_mac_vap->us_assoc_vap_id);
        if (OAL_PTR_NULL != pst_hmac_user)
        {
            hmac_user_del_etc(pst_mac_vap, pst_hmac_user);
        }
    }
#endif  /* _PRE_WLAN_FEATURE_ROAM */

#ifdef _PRE_WLAN_FEATURE_WAPI
    pst_bss_dscr->uc_wapi = pst_connect_param->uc_wapi;
    if (pst_bss_dscr->uc_wapi)
    {
        pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
        if (OAL_PTR_NULL == pst_mac_device)
        {
            OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc::connect failed, pst_mac_device null! uc_device_id[%d]}\r\n",
                    pst_mac_vap->uc_device_id);
            hmac_free_connect_param_resource(pst_connect_param);
            return OAL_ERR_CODE_MAC_DEVICE_NULL;
        }

        if (OAL_SUCC == mac_device_is_p2p_connected_etc(pst_mac_device))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc:: wapi connect failed for p2p having been connected!.}");
            hmac_free_connect_param_resource(pst_connect_param);
            return OAL_FAIL;
        }
    }
#endif

    /* 设置P2P/WPS IE 信息到 vap 结构体中 */
    if (IS_LEGACY_VAP(pst_mac_vap))
    {
        hmac_config_del_p2p_ie_etc(pst_connect_param->puc_ie, &(pst_connect_param->ul_ie_len));
    }


    st_app_ie.ul_ie_len      = pst_connect_param->ul_ie_len;
    oal_memcopy(st_app_ie.auc_ie, pst_connect_param->puc_ie, st_app_ie.ul_ie_len);
    st_app_ie.en_app_ie_type = OAL_APP_ASSOC_REQ_IE;
    hmac_free_connect_param_resource(pst_connect_param);
    ul_ret = hmac_config_set_app_ie_to_vap_etc(pst_mac_vap, &st_app_ie, st_app_ie.en_app_ie_type);
    if(ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc:: hmac_config_set_app_ie_to_vap_etc fail,err_code=%d.}", ul_ret);
        return ul_ret;
    }
    mac_mib_set_dot11dtimperiod(pst_mac_vap,  pst_bss_dscr->uc_dtim_period);

    /* 设置关联用户的能力信息 */
    pst_mac_vap->us_assoc_user_cap_info = pst_bss_dscr->us_cap_info;
    pst_mac_vap->bit_ap_11ntxbf         = (pst_bss_dscr->en_11ntxbf == OAL_TRUE) ? 1 : 0;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* 入网选择的热点rssi，同步到dmac传给tpc算法做管理报文tpc */
    st_conn_sec.c_rssi = pst_bss_dscr->c_rssi;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    st_conn_sec.c_ant0_rssi = pst_bss_dscr->c_ant0_rssi;
    st_conn_sec.c_ant1_rssi = pst_bss_dscr->c_ant1_rssi;
#endif
#endif /* _PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE */

    ul_ret = mac_vap_init_privacy_etc(pst_mac_vap, &st_conn_sec);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_connect_etc:: mac_11i_init_privacy failed[%d]!}\r\n", ul_ret);
        return ul_ret;
    }

    mac_mib_set_WPSActive(pst_mac_vap, st_conn_sec.en_wps_enable);

#ifdef _PRE_WLAN_FEATURE_11R
    if(OAL_TRUE == pst_hmac_vap->bit_11r_enable)
    {
        puc_mde = mac_find_ie_etc(MAC_EID_MOBILITY_DOMAIN, pst_bss_dscr->auc_mgmt_buff + MAC_80211_FRAME_LEN + MAC_SSID_OFFSET, pst_bss_dscr->ul_mgmt_len - MAC_80211_FRAME_LEN - MAC_SSID_OFFSET);
        if (OAL_PTR_NULL != puc_mde)
        {
            oal_memcopy(st_conn_sec.auc_mde, puc_mde, 5);
        }
        mac_mib_init_ft_cfg_etc(pst_mac_vap, st_conn_sec.auc_mde);
    }
#endif //_PRE_WLAN_FEATURE_11R

    ul_ret = hmac_check_capability_mac_phy_supplicant_etc(pst_mac_vap, pst_bss_dscr);
    if (OAL_SUCC != ul_ret)
    {
    }

    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CONNECT_REQ, OAL_SIZEOF(st_conn_sec), (oal_uint8 *)&st_conn_sec);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA,
                       "{hmac_config_connect_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return hmac_sta_initiate_join_etc(pst_mac_vap, pst_bss_dscr);

}


#ifdef _PRE_WLAN_FEATURE_11D

oal_uint32  hmac_config_set_rd_by_ie_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8       *pc_param;
    hmac_vap_stru   *pst_hmac_vap;

    pc_param = (oal_bool_enum_uint8 *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_config_set_rd_by_ie_switch_etc::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->en_updata_rd_by_ie_switch = *pc_param;

    return OAL_SUCC;
}
#endif

oal_uint32  hmac_config_get_tid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru          *pst_mac_dev;
    mac_cfg_get_tid_stru     *pst_tid;

    pst_tid = (mac_cfg_get_tid_stru *)puc_param;
    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_tid->en_tid = pst_mac_dev->en_tid;
    *pus_len = OAL_SIZEOF(pst_tid->en_tid);

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_tid_etc::en_tid=%d.}", pst_tid->en_tid);
    return OAL_SUCC;
}


oal_uint32  hmac_config_list_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                uc_chan_num;
    oal_uint8                uc_chan_idx;
    oal_uint32               ul_ret                                 = OAL_FAIL;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_list_channel_etc::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_2, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx_etc(MAC_RC_START_FREQ_2, uc_chan_idx, &uc_chan_num);

            /* 输出2G信道号 */
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_channel_etc::2gCHA.NO=%d}\n", uc_chan_num);
        }
    }
    if (OAL_FALSE == mac_device_check_5g_enable(pst_mac_vap->uc_device_id))
    {
        return OAL_SUCC;
    }

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT/2; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx_etc(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_num);

#ifdef _PRE_WLAN_FEATURE_DFS
            /* 检测5G 36~120信道上的DFS雷达标记 */
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_channel_etc::5gCHA.NO=%d,DFS_REQUIRED[%c]}\n",
                                  uc_chan_num, ((OAL_TRUE == mac_is_ch_in_radar_band(MAC_RC_START_FREQ_5, uc_chan_idx)) ? 'Y' : 'N'));
#endif
        }
    }

    for (uc_chan_idx = MAC_CHANNEL_FREQ_5_BUTT/2; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx_etc(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_num);
#ifdef _PRE_WLAN_FEATURE_DFS
            /* 检测5G 124~196信道上的DFS雷达标记 */
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_list_channel_etc::5gCHA.NO=%d,DFS_REQUIRED[%c]}\n",
                                  uc_chan_num, ((OAL_TRUE == mac_is_ch_in_radar_band(MAC_RC_START_FREQ_5, uc_chan_idx)) ? 'Y' : 'N'));
#endif
        }
    }
    return OAL_SUCC;
}


oal_uint32 hmac_config_get_assoc_req_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{

    oal_net_dev_ioctl_data_stru *pst_assoc_req_ioctl_data;
    hmac_user_stru              *pst_hmac_user;
    oal_uint8                   *puc_mac;
    oal_uint8                   *puc_assoc_req_ie;
    oal_uint32                   ul_ret;
    oal_uint32                   ul_len;

    pst_assoc_req_ioctl_data = (oal_net_dev_ioctl_data_stru *)puc_param;

    *pus_len = OAL_SIZEOF(oal_net_dev_ioctl_data_stru);

    /* 根据mac 地址查找用户 */
    puc_mac = (oal_uint8 *)pst_assoc_req_ioctl_data->pri_data.assoc_req_ie.auc_mac;
    pst_hmac_user  = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, puc_mac);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_assoc_req_ie_etc::pst_hmac_user null.}");
        return OAL_FAIL;
    }

    /* 拷贝关联请求帧信息 */
    puc_assoc_req_ie = pst_assoc_req_ioctl_data->pri_data.assoc_req_ie.puc_buf;
    ul_len = pst_hmac_user->ul_assoc_req_ie_len;
    if(ul_len > pst_assoc_req_ioctl_data->pri_data.assoc_req_ie.ul_buf_size)
    {
        ul_len = pst_assoc_req_ioctl_data->pri_data.assoc_req_ie.ul_buf_size;
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_assoc_req_ie_etc::user space[%d] not enough,need[%d].}",
            pst_assoc_req_ioctl_data->pri_data.assoc_req_ie.ul_buf_size,
            pst_hmac_user->ul_assoc_req_ie_len);
    }

    ul_ret = oal_copy_to_user(puc_assoc_req_ie, pst_hmac_user->puc_assoc_req_ie_buff, ul_len);
    if (ul_ret != 0)
    {
        OAM_ERROR_LOG0(0,OAM_SF_CFG,"hmac_config_get_assoc_req_ie_etc::hmac oal_copy_to_user fail.");
        return OAL_FAIL;
    }
    pst_assoc_req_ioctl_data->pri_data.assoc_req_ie.ul_buf_size = pst_hmac_user->ul_assoc_req_ie_len;

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_app_ie_to_vap_etc(mac_vap_stru           *pst_mac_vap,
                                            oal_app_ie_stru     *pst_app_ie,
                                            en_app_ie_type_uint8 en_type)
{
    oal_uint32                      ul_ret = OAL_SUCC;
    oal_uint8                      *puc_ie;
    oal_uint32                      remain_len;
    hmac_vap_stru                   *pst_hmac_vap;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == pst_app_ie))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_app_ie_to_vap_etc::scan failed, set ie null ptr, %p, %p.}",
          pst_mac_vap, pst_app_ie);

        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_app_ie_to_vap_etc::pst_hmac_vap null ptr.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*移除驱动侧重复MAC_EID_EXT_CAPS */
    puc_ie = mac_find_ie_etc(MAC_EID_EXT_CAPS, pst_app_ie->auc_ie, (oal_int32)pst_app_ie->ul_ie_len);
    if(puc_ie != OAL_PTR_NULL)
    {
        pst_app_ie->ul_ie_len  -= (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN);
        remain_len              = pst_app_ie->ul_ie_len - (oal_uint32)(puc_ie - pst_app_ie->auc_ie);
        oal_memmove(puc_ie, puc_ie + (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN), remain_len);
    }

    /* remove type 为1表示移除该IE，0为恢复IE，不处理即可，支持处理多个IE需在此扩展 */
    if (pst_hmac_vap->st_remove_ie.uc_type)
    {
        puc_ie = mac_find_ie_etc(pst_hmac_vap->st_remove_ie.uc_eid, pst_app_ie->auc_ie, (oal_int32)pst_app_ie->ul_ie_len);
        if(puc_ie != OAL_PTR_NULL)
        {
            pst_app_ie->ul_ie_len  -= (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN);
            remain_len              = pst_app_ie->ul_ie_len - (oal_uint32)(puc_ie - pst_app_ie->auc_ie);
            oal_memmove(puc_ie, puc_ie + (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN), remain_len);
        }
    }
    puc_ie = mac_find_ie_etc(MAC_EID_OPERATING_CLASS, pst_app_ie->auc_ie, (oal_int32)pst_app_ie->ul_ie_len);
    if( (puc_ie != OAL_PTR_NULL) && (!mac_mib_get_dot11ExtendedChannelSwitchActivated(pst_mac_vap)) )
    {
        pst_app_ie->ul_ie_len  -= (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN);
        remain_len              = pst_app_ie->ul_ie_len - (oal_uint32)(puc_ie - pst_app_ie->auc_ie);
        oal_memmove(puc_ie, puc_ie + (oal_uint32)(puc_ie[1] + MAC_IE_HDR_LEN), remain_len);
    }


    ul_ret = mac_vap_save_app_ie_etc(pst_mac_vap, pst_app_ie, en_type);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_app_ie_to_vap_etc::mac_vap_save_app_ie_etc failed[%d], en_type[%d], len[%d].}",
                    ul_ret,
                    en_type,
                    pst_app_ie->ul_ie_len);
        return ul_ret;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (pst_app_ie->en_app_ie_type >= OAL_APP_ASSOC_REQ_IE)
    {
        /* 只有OAL_APP_BEACON_IE、OAL_APP_PROBE_REQ_IE、OAL_APP_PROBE_RSP_IE 才需要保存到device */
        return OAL_SUCC;
    }

    hmac_config_h2d_send_app_ie(pst_mac_vap, pst_app_ie);
#endif

    return ul_ret;
}


oal_uint32 hmac_config_set_wps_p2p_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_w2h_app_ie_stru            *pst_w2h_wps_p2p_ie;
    oal_app_ie_stru                 st_app_ie;
    hmac_vap_stru                  *pst_hmac_vap;
    oal_uint32                      ul_ret = OAL_SUCC;

    pst_w2h_wps_p2p_ie = (oal_w2h_app_ie_stru *)puc_param;

    if((pst_w2h_wps_p2p_ie->en_app_ie_type >= OAL_APP_IE_NUM) ||
        (pst_w2h_wps_p2p_ie->ul_ie_len >= WLAN_WPS_IE_MAX_SIZE))
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_p2p_ie_etc::app_ie_type=[%d] app_ie_len=[%d],param invalid.}",
        pst_w2h_wps_p2p_ie->en_app_ie_type, pst_w2h_wps_p2p_ie->ul_ie_len);
        return OAL_FAIL;
    }

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_p2p_ie_etc::p2p_ie_type=[%d], p2p_ie_len=[%d].}",
    pst_w2h_wps_p2p_ie->en_app_ie_type, pst_w2h_wps_p2p_ie->ul_ie_len);

    OAL_MEMZERO(&st_app_ie, OAL_SIZEOF(st_app_ie));
    st_app_ie.en_app_ie_type = pst_w2h_wps_p2p_ie->en_app_ie_type;
    st_app_ie.ul_ie_len      = pst_w2h_wps_p2p_ie->ul_ie_len;
    oal_memcopy(st_app_ie.auc_ie, pst_w2h_wps_p2p_ie->puc_data_ie, st_app_ie.ul_ie_len);

    /* 设置WPS/P2P 信息 */
    ul_ret = hmac_config_set_app_ie_to_vap_etc(pst_mac_vap, &st_app_ie, st_app_ie.en_app_ie_type);
    if (ul_ret != OAL_SUCC)
    {
        return ul_ret;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_p2p_ie_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 检测beacon 信息中是否有WPS 信息元素 */
    if (st_app_ie.en_app_ie_type == OAL_APP_BEACON_IE)
    {
        if (0 != st_app_ie.ul_ie_len
            /* && OAL_PTR_NULL != mac_get_wps_ie(pst_wps_p2p_ie->auc_ie, (oal_uint16)pst_wps_p2p_ie->ul_ie_len, 0))*/
            && OAL_PTR_NULL != mac_find_vendor_ie_etc(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, st_app_ie.auc_ie, (oal_int32)(st_app_ie.ul_ie_len)))
        {
            /* 设置WPS 功能使能 */
            mac_mib_set_WPSActive(pst_mac_vap, OAL_TRUE);
            OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_p2p_ie_etc::set wps enable.}");
        }
        else
        {
            mac_mib_set_WPSActive(pst_mac_vap, OAL_FALSE);
        }
    }

    return ul_ret;
}



oal_uint32 hmac_config_set_wps_ie_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                  *pst_hmac_vap;
    oal_app_ie_stru                *pst_wps_ie;
    oal_uint8                      *puc_ie;
    oal_uint32                      ul_ret = OAL_SUCC;

    pst_wps_ie = (oal_app_ie_stru *)puc_param;

    /* 设置WPS 信息 */
    ul_ret = hmac_config_set_app_ie_to_vap_etc(pst_mac_vap, pst_wps_ie, pst_wps_ie->en_app_ie_type);

    if (ul_ret != OAL_SUCC)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                        "{hmac_config_set_wps_ie_etc::ul_ret=[%d].}",
                        ul_ret);
        return ul_ret;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_ie_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 检测beacon 信息中是否有WPS 信息元素 */
    if ((OAL_APP_BEACON_IE == pst_wps_ie->en_app_ie_type) && (0 != pst_wps_ie->ul_ie_len))
    {
        //puc_ie = mac_get_wps_ie(pst_wps_ie->auc_ie, (oal_uint16)pst_wps_ie->ul_ie_len, 0);
        puc_ie = mac_find_vendor_ie_etc(MAC_WLAN_OUI_MICROSOFT, MAC_WLAN_OUI_TYPE_MICROSOFT_WPS, pst_wps_ie->auc_ie, (oal_int32)(pst_wps_ie->ul_ie_len));
        if (OAL_PTR_NULL != puc_ie)
        {
            /* 设置WPS 功能使能 */
            mac_mib_set_WPSActive(pst_mac_vap, OAL_TRUE);
            OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_ie_etc::set wps enable.}");
        }
    }
    else if ((0 == pst_wps_ie->ul_ie_len) &&
             (OAL_APP_BEACON_IE == pst_wps_ie->en_app_ie_type))
    {
        mac_mib_set_WPSActive(pst_mac_vap, OAL_FALSE);
        OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_wps_ie_etc::set wps disable.}");
    }

    return ul_ret;
}



oal_uint32  hmac_config_pause_tid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PAUSE_TID, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_pause_tid_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dump_timer(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret = OAL_SUCC;

    frw_timer_dump_timer_etc(pst_mac_vap->ul_core_id);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DUMP_TIEMR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dump_timer::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

#endif
    return ul_ret;
}



oal_uint32  hmac_config_set_user_vip(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_USER_VIP, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_user_vip::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_vap_host(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_VAP_HOST, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vap_host::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_reg_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_REG_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_reg_info_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))


oal_uint32  hmac_config_sdio_flowctrl_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SDIO_FLOWCTRL, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sdio_flowctrl_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_send_bar(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SEND_BAR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_bar::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_reg_write_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_REG_WRITE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_reg_write_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

#ifdef _PRE_WLAN_ONLINE_DPD

oal_uint32  hmac_config_dpd_cfg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DPD, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_reg_write_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

#endif


oal_uint32  hmac_config_alg_param_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    /***************************************************************************
        抛事件到ALG层, 同步ALG数据
    ***************************************************************************/

#ifdef _PRE_WLAN_FEATURE_TXBF
    /* 同步更新txbf的mib值 */
    mac_ioctl_alg_param_stru            *pst_alg_param;
    pst_alg_param = (mac_ioctl_alg_param_stru *)puc_param;

    if (MAC_ALG_CFG_TXBF_11N_BFEE_ENABLE == pst_alg_param->en_alg_cfg)
    {
        pst_mac_vap->st_cap_flag.bit_11ntxbf = pst_alg_param->ul_value;
    }
#endif


    return hmac_config_alg_send_event(pst_mac_vap, WLAN_CFGID_ALG_PARAM, us_len, puc_param);
}
#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND

oal_uint32  hmac_config_set_restrict_band(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;
    wlan_channel_band_enum_uint8 en_band;

    if (!pst_mac_vap || !puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_restrict_band::null ptr, vap=%p puc_param=%p}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (!pst_hmac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_restrict_band::null hmac_vap, vap id=%d}", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_band = (wlan_channel_band_enum_uint8)puc_param[0];

    if ((en_band != WLAN_BAND_2G) && (en_band != WLAN_BAND_5G) && (en_band != WLAN_BAND_BUTT))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_restrict_band::invalid restrict band=%d. 0=2G 1=5G 3=2G/5G}", puc_param);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_hmac_vap->en_restrict_band = en_band;

    return OAL_SUCC;
}
#endif /* _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND */

#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE)

oal_uint32  hmac_config_set_omit_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru   *pst_hmac_vap;

    if (!pst_mac_vap || !puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_omit_acs::null ptr, vap=%p puc_param=%p}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (!pst_hmac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_omit_acs::null hmac_vap, vap id=%d}", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap->en_omit_acs_chan = puc_param[0] ? OAL_TRUE : OAL_FALSE;

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_DFS

oal_uint32  hmac_config_dfs_radartool_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device;
    oal_int8              *pc_token;
    oal_int8              *pc_end;
    oal_int8              *pc_ctx;
    oal_int8              *pc_sep = " ";
    oal_bool_enum_uint8    en_val;
    oal_uint32             ul_val;
    oal_uint32             ul_ret;
    oal_uint32             ul_copy_len;
    oal_int8               auc_param[WLAN_MEM_LOCAL_SIZE2];
    wlan_channel_bandwidth_enum_uint8   en_width = WLAN_BAND_WIDTH_BUTT;

    ul_copy_len = OAL_STRLEN((oal_int8 *)puc_param);
    if (ul_copy_len > WLAN_MEM_LOCAL_SIZE2 - 1)
    {
        ul_copy_len = WLAN_MEM_LOCAL_SIZE2 - 1;
    }
    oal_memcopy(auc_param, puc_param, ul_copy_len);
    auc_param[ul_copy_len] = '\0';

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取命令类型 */
    pc_token = oal_strtok((oal_int8 *)auc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (0 == oal_strcmp(pc_token, "dfsenable"))
    {
        /* 获取DFS使能开关*/
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_dfs_enable(pst_mac_device, en_val);
    }
    else if (0 == oal_strcmp(pc_token, "cacenable"))
    {
        /* 获取CAC检测使能开关*/
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_cac_enable(pst_mac_device, en_val);
    }
    else if (0 == oal_strcmp(pc_token, "shownol"))
    {

    }
    else if (0 == oal_strcmp(pc_token, "cac"))
    {
        /* 获取频段 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (0 == oal_strcmp(pc_token, "weather"))
        {
            en_val = OAL_TRUE;
        }
        else if (0 == oal_strcmp(pc_token, "nonweather"))
        {
            en_val = OAL_FALSE;
        }
        else
        {
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        /* 获取CAC检测时间 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 10);

        if((ul_val > HMAC_DFS_ONE_MIN_IN_MS)&&(ul_val % HMAC_DFS_ONE_MIN_IN_MS != 0))
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::cac time should be a multiple of 1min.");
        }

        mac_dfs_set_cac_time(pst_mac_device, ul_val, en_val);
    }
    else if (0 == oal_strcmp(pc_token, "get_cac"))
    {
        /* 获取频段 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (0 == oal_strcmp(pc_token, "weather"))
        {
            ul_val = pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_in_5600_to_5650_time_ms;

            OAM_WARNING_LOG1(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::get_cac(weather radar)=%d.",ul_val);

            OAL_IO_PRINT("current weather radar CAC time = %d\r\n",ul_val);
        }
        else if (0 == oal_strcmp(pc_token, "nonweather"))
        {
            ul_val = pst_mac_device->st_dfs.st_dfs_info.ul_dfs_cac_outof_5600_to_5650_time_ms;

            OAM_WARNING_LOG1(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::get_cac(nonweather radar)=%d.",ul_val);

            OAL_IO_PRINT("current nonweather radar CAC time = %d\r\n",ul_val);
        }
        else
        {
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        return OAL_SUCC;
    }
    else if (0 == oal_strcmp(pc_token, "dfsdebug"))
    {
        /* 获取debug level */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

        mac_dfs_set_debug_level(pst_mac_device, (oal_uint8)ul_val);
    }
    else if(0 == oal_strcmp(pc_token, "offchanenable"))
    {
        /* 获取OFF-CHAN CAC检测使能开关*/
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);
        OAL_IO_PRINT("en_val = %u.\n", en_val);

        mac_dfs_set_offchan_cac_enable(pst_mac_device, en_val);
    }
    else if(0 == oal_strcmp(pc_token, "offchannum"))
    {
        /* 获取OFF-CHAN CAC检测信道*/
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_offchan_number(pst_mac_device, ul_val);
    }
    else if(0 == oal_strcmp(pc_token, "operntime"))
    {
        /* 获取OFF-CHAN CAC检测工作信道驻留时间 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_opern_chan_time(pst_mac_device, ul_val);
    }
    else if(0 == oal_strcmp(pc_token, "offchantime"))
    {
        /* 获取OFF-CHAN CAC检测OFF信道驻留时间 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);

        mac_dfs_set_off_chan_time(pst_mac_device, ul_val);
    }
    else if(0 == oal_strcmp(pc_token, "dfstrig"))
    {
    }
    else if(0 == oal_strcmp(pc_token, "set_next_chan"))
    {
        /* 获取下一条信道 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 10);

        if(mac_is_channel_num_valid_etc(MAC_RC_START_FREQ_5, (oal_uint8)ul_val) != OAL_SUCC)
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::next chan should be a valid channel.");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (pc_token)
        {
            en_width = mac_vap_str_to_width(pc_token);
        }

        if(WLAN_BAND_WIDTH_BUTT == en_width)
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "dmac_config_dfs_radartool::next chan width mode invalid or not set.");
        }

        mac_dfs_set_next_radar_ch(pst_mac_device,(oal_uint8)ul_val,en_width);
    }
    else if(0 == oal_strcmp(pc_token, "get_next_chan"))
    {
        ul_val = pst_mac_device->st_dfs.st_dfs_info.uc_custom_next_chnum;
        en_width = pst_mac_device->st_dfs.st_dfs_info.en_next_ch_width_type;
        if(ul_val)
        {
            OAM_WARNING_LOG1(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::get_next_chan=%d.",ul_val);

            OAL_IO_PRINT("next radar ch = %d(%s)\r\n",ul_val, mac_vap_width_to_str(en_width));
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::get_next_chan=random.");

            OAL_IO_PRINT("next radar ch is random\r\n");
        }

        return OAL_SUCC;
    }
    else if(0 == oal_strcmp(pc_token, "set_5g_channel_bitmap"))
    {
        /* 获取bitmap */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 16);

        if(0 == ul_val)
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::5G channel bitmap should not be zero.");
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        OAM_WARNING_LOG1(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::set 5G channel bitmap 0x%x.",ul_val);
        mac_dfs_set_ch_bitmap(pst_mac_device,ul_val);
    }
    else if(0 == oal_strcmp(pc_token, "get_5g_channel_bitmap"))
    {
        ul_val = pst_mac_device->st_dfs.st_dfs_info.ul_custom_chanlist_bitmap;
        if(ul_val)
        {
            OAM_WARNING_LOG1(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::get_5G_channel_bitmap=0x%x.",ul_val);

            OAL_IO_PRINT("custom 5G channel bitmap = 0x%x\r\n",ul_val);
        }
        else
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::custom 5G channel bitmap not set.");

            OAL_IO_PRINT("custom 5G channel bitmap not set\r\n");
        }

        return OAL_SUCC;
    }
    else if(0 == oal_strcmp(pc_token, "set_radar_th"))
    {

    }
    else if(0 == oal_strcmp(pc_token, "get_radar_th"))
    {

    }
    else if(0 == oal_strcmp(pc_token, "radarfilter"))
    {

    }
    else if(0 == oal_strcmp(pc_token, "radarfilter_get"))
    {

    }
    else if(0 == oal_strcmp(pc_token, "ctsdura"))
    {

    }
    else if(0 == oal_strcmp(pc_token, "offcactime"))
    {
        /* 获取频段 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (0 == oal_strcmp(pc_token, "weather"))
        {
            en_val = OAL_TRUE;
        }
        else if (0 == oal_strcmp(pc_token, "nonweather"))
        {
            en_val = OAL_FALSE;
        }
        else
        {
            return OAL_ERR_CODE_INVALID_CONFIG;
        }

        /* 获取off CAC检测时间 */
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 10);

        if(ul_val % HMAC_DFS_ONE_MIN_IN_MS != 0)
        {
            OAM_WARNING_LOG0(0, OAM_SF_DFS, "hmac_config_dfs_radartool_etc::off cac time should be a multiple of 1min.");
        }

        mac_dfs_set_off_cac_time(pst_mac_device, ul_val, en_val);
    }
    else if(0 == oal_strcmp(pc_token, "enabletimer"))
    {

    }
    else if(0 == oal_strcmp(pc_token, "set_chan"))
    {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            return OAL_ERR_CODE_PTR_NULL;
        }
        ul_val = (oal_uint32)oal_strtol(pc_token, &pc_end, 10);
        pst_mac_vap->st_ch_switch_info.uc_ch_switch_cnt = WLAN_CHAN_SWITCH_DEFAULT_CNT;
        hmac_dfs_set_channel_etc(pst_mac_vap, (oal_uint8)ul_val);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DFS, "{hmac_config_dfs_radartool_etc::set channel to %d.}", ul_val);
        return OAL_SUCC;
    }
    else if(0 == oal_strcmp(pc_token, "non_occupancy_period"))
    {

    }
#ifdef _PRE_WLAN_FEATURE_DFS_OPTIMIZE
    else if (0 == oal_strcmp(pc_token, "log_switch"))
    {

    }
    else if (0 == oal_strcmp(pc_token, "read_pulse"))
    {

    }
    else if(0 == oal_strcmp(pc_token, "pulse_check_filter"))
    {

    }
#endif
    else
    {
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_RADARTOOL, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_always_tx::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_SUPPORT_ACS
extern oal_uint32 hmac_acs_process_scan(mac_device_stru *pst_mac_dev, mac_vap_stru *pst_mac_vap, mac_scan_op_enum_uint8 en_op);

oal_uint32  hmac_config_acs(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru         *pst_mac_device;
    hmac_device_stru        *pst_hmac_device;
    oal_int8                *pc_token;
    oal_int8                *pc_end;
    oal_int8                *pc_ctx;
    oal_int8                *pc_sep = " ";
    oal_bool_enum_uint8     en_val;
    oal_uint32              ul_ret;
    oal_int8                auc_param[WLAN_MEM_LOCAL_SIZE2];
    oal_uint32              ul_len = OAL_STRLEN((oal_int8 *)puc_param);
    hmac_acs_cfg_stru       st_acs_cfg;

    if(ul_len >= OAL_SIZEOF(auc_param)-1)
    {
        return OAL_FAIL;
    }

    oal_memcopy(auc_param, (oal_void *)puc_param, ul_len);
    auc_param[ul_len] = 0;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device || OAL_PTR_NULL == pst_hmac_device->pst_device_base_info)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = pst_hmac_device->pst_device_base_info;

    /* 获取命令类型 */
    pc_token = oal_strtok((oal_int8 *)auc_param, pc_sep, &pc_ctx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (0 == oal_strcmp(pc_token, "sw"))
    {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            OAL_IO_PRINT("error : hmac_config_acs\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);
        if(en_val > MAC_ACS_SW_BUTT)
        {
            OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ACS, "{hmac_config_acs::invalid switch=%d, force to NONE}", en_val);
            en_val = MAC_ACS_SW_NONE;
        }
        mac_set_acs_switch(pst_mac_device, en_val);
        OAL_IO_PRINT("set acs sw=%d\n", en_val);
    }
    else if (0 == oal_strcmp(pc_token, "fscan"))
    {
        OAL_IO_PRINT("fg scan\n");
        st_acs_cfg.en_switch_chan = OAL_FALSE;
        st_acs_cfg.uc_acs_type = HMAC_ACS_TYPE_INIT;
        st_acs_cfg.en_scan_op = MAC_SCAN_OP_FG_SCAN_ONLY;
        hmac_init_scan_process_etc(pst_hmac_device, pst_mac_vap, &st_acs_cfg);
    }
    else if (0 == oal_strcmp(pc_token, "bscan"))
    {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ACS, "{hmac_config_acs::error bscan}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10)? OAL_TRUE : OAL_FALSE;
        st_acs_cfg.en_scan_op     = MAC_SCAN_OP_INIT_SCAN;
        st_acs_cfg.en_switch_chan = en_val;
        st_acs_cfg.uc_acs_type    = HMAC_ACS_TYPE_CMD;
        OAL_IO_PRINT("bg scan,switch channel=%d, acs_type=%d\n", en_val, HMAC_ACS_TYPE_CMD);

        //bscan时，如果ACS未开启，则return
        if (MAC_ACS_SW_NONE == mac_get_acs_switch(pst_mac_device))
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ACS, "{hmac_config_acs:: acs not enable, bscan stop}");
            return OAL_FAIL;
        }

        hmac_init_scan_process_etc(pst_hmac_device, pst_mac_vap, &st_acs_cfg);
    }
    else if (0 == oal_strcmp(pc_token, "idle_scan"))
    {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_PTR_NULL != pc_token)
        {
            pst_hmac_device->en_rescan_idle = oal_strtol(pc_token, &pc_end, 10) ? OAL_TRUE : OAL_FALSE;
        }
    }
    else if (0 == oal_strcmp(pc_token, "set_dcs"))
    {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_PTR_NULL != pc_token)
        {
            oal_uint32 ul_bak = pst_hmac_device->ul_rescan_timeout;
            /* oal_strtol return int,so config_value*1000*60 should not more than ((max uint32)+1)/2 = 35791 */
            /* limit to 24*60min */
            if((oal_strtol(pc_token, &pc_end, 10) > 1440) || (oal_strtol(pc_token, &pc_end, 10) <= 0))
            {
                OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ACS, "hmac_config_acs::set fail for %d is more than 1440 or less than 0\n", oal_strtol(pc_token, &pc_end, 10));
                return OAL_FAIL;
            }
            pst_hmac_device->ul_rescan_timeout = (oal_uint32)(60 * 1000 * oal_strtol(pc_token, &pc_end, 10));

            if (!pst_hmac_device->ul_rescan_timeout)
            {
                pst_hmac_device->ul_rescan_timeout = ul_bak;
            }

            frw_timer_restart_timer_etc(&pst_hmac_device->st_rescan_timer,
                                     pst_hmac_device->ul_rescan_timeout, OAL_TRUE);
        }
    }
    else if (0 == oal_strcmp(pc_token, "get_dcs"))
    {
        OAL_IO_PRINT("device_id=%d dcs=%d min\n", pst_mac_vap->uc_device_id, pst_hmac_device->ul_rescan_timeout/(60*1000));
    }
#if 0
    else if (0 == oal_strcmp(pc_token, "obss"))
    {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            OAL_IO_PRINT("error : hmac_config_acs\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);
        pst_mac_device->en_obss_switch = en_val;
        OAL_IO_PRINT("set obss=%d in hmac\n", en_val);
        return OAL_SUCC;
    }
    else if (0 == oal_strcmp(pc_token, "dfs"))
    {
        pc_token = oal_strtok(OAL_PTR_NULL, pc_sep, &pc_ctx);
        if (OAL_UNLIKELY(OAL_PTR_NULL == pc_token))
        {
            OAL_IO_PRINT("error : hmac_config_acs\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        en_val = (oal_bool_enum_uint8)oal_strtol(pc_token, &pc_end, 10);
        mac_dfs_set_dfs_enable(pst_mac_device, en_val);
        mac_dfs_set_cac_enable(pst_mac_device, en_val);
        OAL_IO_PRINT("set dfs=%d in hmac\n", en_val);
        return OAL_SUCC;
    }
#endif
    else
    {
        OAL_IO_PRINT("unknown acs cmd=[%s]\n", pc_token);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ACS_CONFIG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_acs::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32 hmac_do_chan_stat(hmac_device_stru *pst_hmac_dev, mac_vap_stru *pst_mac_vap, mac_chan_stat_param_stru *pst_param)
{
    extern oal_void hmac_init_scan_cb_etc(void *p_scan_record);

    oal_uint8                           uc_idx, uc_cnt;
    oal_uint32                          ul_ret = OAL_SUCC;
    oal_uint8                           uc_channel;
    wlan_channel_band_enum_uint8        en_band;
    wlan_channel_bandwidth_enum_uint8   en_bandwidth;
    mac_scan_req_stru                   st_scan_req;

    pst_hmac_dev->en_init_scan    = OAL_FALSE;
    pst_hmac_dev->en_in_init_scan = OAL_TRUE;

    OAL_MEMZERO(&st_scan_req, OAL_SIZEOF(st_scan_req));
    st_scan_req.en_scan_mode  = WLAN_SCAN_MODE_FOREGROUND;
    st_scan_req.en_scan_type  = WLAN_SCAN_TYPE_ACTIVE;
    st_scan_req.uc_scan_func  = MAC_SCAN_FUNC_MEAS | MAC_SCAN_FUNC_STATS | MAC_SCAN_FUNC_BSS;
    st_scan_req.en_bss_type   = WLAN_MIB_DESIRED_BSSTYPE_INFRA;
    st_scan_req.uc_bssid_num  = 0;
    st_scan_req.uc_ssid_num   = 0;

    st_scan_req.uc_max_scan_count_per_channel = 1;
    st_scan_req.uc_max_send_probe_req_count_per_channel = 2;

    st_scan_req.us_scan_time        = pst_param->us_duration_ms;
    st_scan_req.uc_probe_delay      = 0;
    st_scan_req.uc_vap_id           = pst_mac_vap->uc_vap_id;
    st_scan_req.p_fn_cb             = hmac_init_scan_cb_etc;

    uc_cnt = 0;
    for (uc_idx = 0; uc_idx < pst_param->uc_chan_cnt; uc_idx++)
    {
        uc_channel   = pst_param->auc_channels[uc_idx];
        en_band      = uc_channel > MAX_CHANNEL_NUM_FREQ_2G ? WLAN_BAND_5G : WLAN_BAND_2G;
        en_bandwidth = WLAN_BAND_WIDTH_20M;
        if (OAL_SUCC != mac_is_channel_num_valid_etc(en_band, uc_channel))
        {
            continue;
        }

        st_scan_req.ast_channel_list[uc_cnt].uc_chan_number = uc_channel;
        st_scan_req.ast_channel_list[uc_cnt].en_band        = en_band;
        st_scan_req.ast_channel_list[uc_cnt].en_bandwidth   = en_bandwidth;
        if(OAL_SUCC != mac_get_channel_idx_from_num_etc(en_band, uc_channel, &st_scan_req.ast_channel_list[uc_cnt].uc_chan_idx))
        {
            continue;
        }
        uc_cnt++;
    }

    st_scan_req.uc_channel_nums = uc_cnt;

    if (uc_cnt != 0)
    {

        /* 直接调用扫描模块扫描请求处理函数 */
        ul_ret = hmac_scan_proc_scan_req_event_etc(OAL_DLIST_GET_ENTRY(pst_mac_vap, hmac_vap_stru, st_vap_base_info), &st_scan_req);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG1(0, OAM_SF_SCAN, "hmac_init_scan_do_etc:hmac_scan_add_req failed, ret=%d", ul_ret);
        }
    }

    return ul_ret;
}


oal_uint32  hmac_config_chan_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_device_stru      *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device || OAL_PTR_NULL == pst_hmac_device->pst_device_base_info)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }


    return hmac_do_chan_stat(pst_hmac_device, pst_mac_vap, (mac_chan_stat_param_stru *)puc_param);
}
#endif

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING

oal_uint32  hmac_config_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_BSD_CONFIG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_bsd::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#ifdef _PRE_WLAN_WEB_CMD_COMM

oal_uint32  hmac_config_get_bsd(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_cfg_query_bsd_stru      st_param;
    oal_uint32                  ul_ret;
    hmac_vap_stru              *pst_hmac_vap;
    oal_int32                   l_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    st_param.uc_vap_id = pst_mac_vap->uc_vap_id;
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_BSD, OAL_SIZEOF(mac_cfg_query_bsd_stru), (oal_uint8 *)&st_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_bsd::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_bsd::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*lint -e730*/
    l_ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(pst_hmac_vap->query_wait_q, OAL_TRUE == pst_hmac_vap->station_info_query_completed_flag, (5*OAL_TIME_HZ));
    /*lint +e730*/
    if (l_ret <= 0)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_wme_stat::OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT [%d].}", l_ret);
        return OAL_EFAIL;
    }
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_bsd::bsd switch=%d vap bsd=%d}", st_param.uc_bsd,pst_mac_vap->uc_bsd_switch);

    *pus_len = OAL_SIZEOF(oal_uint32);
    *((oal_uint32 *)puc_param) = pst_mac_vap->uc_bsd_switch;

    return OAL_SUCC;
}
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11V

oal_uint32  hmac_11v_cfg_wl_mgmt_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8     en_11v_wl_mgt_switch = (oal_bool_enum_uint8)(*puc_param);
    oal_bool_enum_uint8     en_11v_wl_mgt_flag = OAL_FALSE;
    /* 调用接口配置11v特性开关 */
    mac_mib_set_WirelessManagementImplemented(pst_mac_vap,en_11v_wl_mgt_switch);
    /* 获取一把校验下 */
    en_11v_wl_mgt_flag = mac_mib_get_WirelessManagementImplemented(pst_mac_vap);

    if (OAL_UNLIKELY(en_11v_wl_mgt_flag != en_11v_wl_mgt_switch))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_cfg_wl_mgmt_switch::config 11v wireless management capability failed.}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}

#if defined(_PRE_DEBUG_MODE)

oal_uint32  hmac_11v_ap_tx_request(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret = 0;
     /* 仅AP模式下支持触发发送request帧 */
    if ( WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode )
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_sta_tx_query::vap mode:[%d] not support this.}", pst_mac_vap->en_vap_mode);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_11V_TX_REQUEST, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_ap_tx_request::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return OAL_SUCC;
}
#endif //   _PRE_DEBUG_MODE
#endif //   _PRE_WLAN_FEATURE_11V

#if (defined(_PRE_WLAN_FEATURE_11V) && defined(_PRE_DEBUG_MODE)) || defined(_PRE_WLAN_FEATURE_11V_ENABLE)

oal_uint32  hmac_11v_cfg_bsst_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8     en_11v_cfg_switch = (oal_bool_enum_uint8)(*puc_param);
    oal_uint32              ul_ret;

    /* 调用接口配置11v特性开关 */
    mac_mib_set_MgmtOptionBSSTransitionActivated(pst_mac_vap, en_11v_cfg_switch);

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_cfg_bsst_switch:: Set BSST_Actived=[%d].}",
    mac_mib_get_MgmtOptionBSSTransitionActivated(pst_mac_vap));

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_11V_BSST_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_cfg_bsst_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return OAL_SUCC;
}


oal_uint32  hmac_11v_sta_tx_query(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret = 0;
    /* 仅STA模式下支持触发发送query帧 */
    if ( WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_sta_tx_query::vap mode:[%d] not support this.}", pst_mac_vap->en_vap_mode);
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    if(OAL_FALSE == mac_mib_get_MgmtOptionBSSTransitionActivated(pst_mac_vap))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_sta_tx_query::en_dot11MgmtOptionBSSTransitionActivated is FALSE.}");
        return OAL_ERR_CODE_CONFIG_UNSUPPORT;
    }

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_11V_TX_QUERY, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_11v_sta_tx_query::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_beacon_chain_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret = 0;

    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_BEACON_CHAIN_SWITCH, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_beacon_chain_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#if 0

oal_uint32  hmac_config_tdls_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /* 配置tdls prohibited,1为开启禁止,0为关闭禁止 */
    mac_vap_set_tdls_prohibited(pst_mac_vap, *((oal_uint8 *)puc_param));

    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TDLS_PROHI, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_tdls_prohibited::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_tdls_channel_switch_prohibited(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    mac_vap_set_tdls_channel_switch_prohibited(pst_mac_vap, *((oal_uint8 *)puc_param));

    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TDLS_CHASWI_PROHI, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_tdls_channel_switch_prohibited::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_set_2040_coext_support_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if ((0 != *puc_param) && (1 != *puc_param))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_2040_coext_support_etc::invalid param[%d].", *puc_param);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 如果是配置VAP, 直接返回 */
    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_2040_coext_support_etc::this is config vap! can't set.}");
        return OAL_FAIL;
    }

    mac_mib_set_2040BSSCoexistenceManagementSupport(pst_mac_vap, (oal_bool_enum_uint8)(*puc_param));

    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_2040_coext_support_etc::end func,puc_param=%d.}", *puc_param);
    return OAL_SUCC;
}


oal_uint32  hmac_config_rx_fcs_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_RX_FCS_INFO, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_rx_fcs_info_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_resume_rx_intr_fifo(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_RESUME_RX_INTR_FIFO, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_resume_rx_intr_fifo::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_PERFORM_STAT

oal_uint32  hmac_config_pfm_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PFM_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_pfm_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_pfm_display(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PFM_DISPLAY, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_pfm_display::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

oal_uint32  hmac_config_set_edca_opt_switch_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8        uc_flag         = 0;
    oal_uint32       ul_ret          = 0;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* 获取hmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta_etc, mac_res_get_hmac_vap fail.vap_id = %u",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取配置参数 */
    uc_flag = *puc_param;

    /* 参数没有更改，不需要重新配置 */
    if (uc_flag == pst_hmac_vap->uc_edca_opt_flag_sta)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta_etc, change nothing to flag:%d", pst_hmac_vap->uc_edca_opt_flag_sta);
        return OAL_SUCC;
    }

    /* 设置参数，并启动或者停止edca参数调整定时器 */
    pst_hmac_vap->uc_edca_opt_flag_sta = uc_flag;

    if (0 == pst_hmac_vap->uc_edca_opt_flag_sta)
    {
        ul_ret = mac_vap_init_wme_param_etc(pst_mac_vap);
        if (OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta_etc: mac_vap_init_wme_param_etc failed");
            return ul_ret;
        }

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "mac_vap_init_wme_param_etc succ");
    }
    else
    {
#if 0 //验收通过后，此函数会删除-wanran
        hmac_edca_opt_adj_param_sta((oal_void *)pst_hmac_vap);
#endif

        OAM_WARNING_LOG0(0, OAM_SF_ANY, "hmac_edca_opt_adj_param_sta succ");
    }

    /* 更新EDCA相关的MAC寄存器 */
    ul_ret = hmac_sta_up_update_edca_params_machw_etc(pst_hmac_vap, MAC_WMM_SET_PARAM_TYPE_UPDATE_EDCA);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta_etc: hmac_sta_up_update_edca_params_machw_etc failed");
        return ul_ret;
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_sta_etc,config sucess, %d", pst_hmac_vap->uc_edca_opt_flag_sta);

    return OAL_SUCC;

}



oal_uint32  hmac_config_set_edca_opt_weight_sta_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8        uc_weight      = 0;
    hmac_vap_stru   *pst_hmac_vap   = OAL_PTR_NULL;

    /* 获取hmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_weight_sta_etc, mac_res_get_hmac_vap fail.vap_id = %u",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_weight = *puc_param;

    /* 判断edca权重是否有调整 */
    if (uc_weight == pst_hmac_vap->uc_edca_opt_weight_sta)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_weight_sta_etc, change nothing to cycle:%d", pst_hmac_vap->uc_edca_opt_weight_sta);
        return OAL_SUCC;
    }

    /* 更新权重 */
    pst_hmac_vap->uc_edca_opt_weight_sta = uc_weight;
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_weight_sta_etc succ, wieight = %d", pst_hmac_vap->uc_edca_opt_weight_sta);

    return OAL_SUCC;
}




oal_uint32  hmac_config_set_edca_opt_switch_ap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8        uc_flag         = 0;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* 获取hmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_ap_etc, mac_res_get_hmac_vap fail.vap_id = %u",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取配置参数 */
    uc_flag = *puc_param;

    /* 参数没有更改，不需要重新配置 */
    if (uc_flag == pst_hmac_vap->uc_edca_opt_flag_ap)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "wal_hipriv_set_edca_opt_switch_ap, change nothing to flag:%d", pst_hmac_vap->uc_edca_opt_flag_ap);
        return OAL_SUCC;
    }

    /* 设置参数，并启动或者停止edca参数调整定时器 */
    if (1 == uc_flag)
    {
        pst_hmac_vap->uc_edca_opt_flag_ap = 1;
        FRW_TIMER_RESTART_TIMER(&(pst_hmac_vap->st_edca_opt_timer), pst_hmac_vap->ul_edca_opt_time_ms, OAL_TRUE);
    }
    else
    {
        pst_hmac_vap->uc_edca_opt_flag_ap = 0;
        FRW_TIMER_STOP_TIMER(&(pst_hmac_vap->st_edca_opt_timer));
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_switch_ap_etc succ, flag = %d", pst_hmac_vap->uc_edca_opt_flag_ap);

    return OAL_SUCC;

}


oal_uint32  hmac_config_set_edca_opt_cycle_ap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32       ul_cycle_ms     = 0;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* 获取hmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_cycle_ap_etc, mac_res_get_hmac_vap fail.vap_id = %u",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_cycle_ms = *((oal_uint32 *)puc_param);

    /* 判断edca调整周期是否有更新 */
    if (ul_cycle_ms == pst_hmac_vap->ul_edca_opt_time_ms)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_cycle_ap_etc, change nothing to cycle:%d", pst_hmac_vap->ul_edca_opt_time_ms);
        return OAL_SUCC;
    }

    /* 如果edca调整定时器正在运行，则需要先停止后，再根据新的参数restart */
    if (1 == pst_hmac_vap->uc_edca_opt_flag_ap)
    {
        pst_hmac_vap->ul_edca_opt_time_ms = ul_cycle_ms;
        FRW_TIMER_STOP_TIMER(&(pst_hmac_vap->st_edca_opt_timer));
        FRW_TIMER_RESTART_TIMER(&(pst_hmac_vap->st_edca_opt_timer), pst_hmac_vap->ul_edca_opt_time_ms, OAL_TRUE);
    }
    else    /* 仅更新参数即可 */
    {
        pst_hmac_vap->ul_edca_opt_time_ms = ul_cycle_ms;
    }

    OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac_config_set_edca_opt_cycle_ap_etc succ, cycle = %d", pst_hmac_vap->ul_edca_opt_time_ms);

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_uint32  hmac_config_set_pm_by_module_etc(mac_vap_stru *pst_mac_vap, mac_pm_ctrl_type_enum pm_ctrl_type, mac_pm_switch_enum pm_enable)
{
    oal_uint32              ul_ret      = OAL_SUCC;
    mac_cfg_ps_open_stru    st_ps_open  = {0};

    if(MAC_STA_PM_SWITCH_BUTT    <= pm_enable    ||
       MAC_STA_PM_CTRL_TYPE_BUTT <= pm_ctrl_type ||
       OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG3(0, OAM_SF_ANY, "hmac_config_set_pm_by_module_etc, PARAM ERROR! pst_mac_vap = 0x%X, pm_ctrl_type = %d, pm_enable = %d ",
                       pst_mac_vap,pm_ctrl_type, pm_enable);
        return OAL_FAIL;
    }

    st_ps_open.uc_pm_enable      = pm_enable;
    st_ps_open.uc_pm_ctrl_type   = pm_ctrl_type;

#ifdef _PRE_WLAN_FEATURE_STA_PM
    ul_ret = hmac_config_set_sta_pm_on_etc(pst_mac_vap,OAL_SIZEOF(mac_cfg_ps_open_stru), (oal_uint8 *)&st_ps_open);
#endif

    OAM_WARNING_LOG3(0, OAM_SF_PWR, "hmac_config_set_pm_by_module_etc, pm_module = %d, pm_enable = %d, cfg ret = %d ",
                   pm_ctrl_type, pm_enable,ul_ret);

    return ul_ret;
}
#endif


oal_uint32  hmac_config_alg_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                   ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_user_stru               *pst_user;
    oal_int8                    *pac_argv[DMAC_ALG_CONFIG_MAX_ARG + 1] = {0};
    mac_ioctl_alg_config_stru   *pst_alg_config;
    oal_uint8                    uc_idx;
    oal_uint32                   ul_bw_limit_kbps;
    oal_dlist_head_stru         *pst_list_pos;

    pst_alg_config = (mac_ioctl_alg_config_stru *)puc_param;

    for (uc_idx = OAL_SIZEOF(mac_ioctl_alg_config_stru); uc_idx < us_len; uc_idx++)
    {
        if(puc_param[uc_idx] == ' ')
        {
            puc_param[uc_idx] = 0;
        }
    }

    for(uc_idx = 0; uc_idx < pst_alg_config->uc_argc; uc_idx++)
    {
        pac_argv[uc_idx] = (oal_int8 *)puc_param + OAL_SIZEOF(mac_ioctl_alg_config_stru) + pst_alg_config->auc_argv_offset[uc_idx];
    }

    /* 如果为用户限速，则需要同步hmac_vap的状态信息 */
    if ((0 == oal_strcmp(pac_argv[0], "sch"))
        && (0 == oal_strcmp(pac_argv[1], "usr_bw")))
    {
        pst_user = mac_vap_get_user_by_addr_etc(pst_mac_vap, (oal_uint8*)(pac_argv[2]));
        if (OAL_PTR_NULL == pst_user)
        {
            OAM_ERROR_LOG0(0, OAM_SF_MULTI_TRAFFIC, "{alg_schedule_config_user_bw_limit: mac_vap_find_user_by_macaddr_etc failed}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        ul_bw_limit_kbps = (oal_uint32)oal_atoi(pac_argv[3]);

        pst_mac_vap->bit_has_user_bw_limit = OAL_FALSE;
        for ((pst_list_pos) = (pst_mac_vap)->st_mac_user_list_head.pst_next, (pst_user) = OAL_DLIST_GET_ENTRY((pst_list_pos), mac_user_stru, st_user_dlist);
                (pst_list_pos) != &((pst_mac_vap)->st_mac_user_list_head);
                (pst_list_pos) = (pst_list_pos)->pst_next, (pst_user) = OAL_DLIST_GET_ENTRY((pst_list_pos), mac_user_stru, st_user_dlist))

        {
            /* 如果限速值不为0,表示该user已被限速，则更新vap的状态 */
            if ((OAL_PTR_NULL != pst_user) && (0 != ul_bw_limit_kbps))
            {
                pst_mac_vap->bit_has_user_bw_limit = OAL_TRUE;
                break;
            }
        }
    }
#else
#ifdef _PRE_WLAN_FEATURE_DBAC
    {
        mac_device_stru *pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
        hmac_acs_cfg_stru   st_acs_cfg;
        if (pst_mac_device != OAL_PTR_NULL
          && mac_is_dbac_enabled(pst_mac_device)
          && !oal_strcmp((const oal_int8 *)puc_param + OAL_SIZEOF(mac_ioctl_alg_config_stru), " dbac start"))
        {
            st_acs_cfg.uc_acs_type = HMAC_ACS_TYPE_INIT;
            st_acs_cfg.en_switch_chan = OAL_TRUE;
            st_acs_cfg.en_scan_op = MAC_SCAN_OP_INIT_SCAN;
            if(OAL_SUCC == hmac_init_scan_try_etc(pst_mac_device, pst_mac_vap, MAC_TRY_INIT_SCAN_START_DBAC, &st_acs_cfg))
            {
                return OAL_SUCC;
            }
        }
    }
#endif

#endif

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ALG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
oal_uint32 hmac_config_tcp_ack_buf(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TCP_ACK_BUF, us_len, puc_param);
}
#endif

#ifdef _PRE_FEATURE_FAST_AGING
oal_uint32 hmac_config_fast_aging(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FAST_AGING, us_len, puc_param);
}
oal_uint32 hmac_config_get_fast_aging(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_FAST_AGING, OAL_SIZEOF(oal_uint8), puc_param);
}
oal_uint32 hmac_config_get_fast_aging_rsp(mac_vap_stru *pst_mac_vap,  oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_FAST_AGING);
}

#endif

#ifdef _PRE_WLAN_FEATURE_CAR

oal_uint32  hmac_config_car_cfg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_device_stru               *pst_hmac_dev;
    hmac_vap_stru                  *pst_hmac_vap;
    mac_cfg_car_stru               *pst_car_cfg_param;

    /* VAP模式判断 */
    if ((WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode) && (WLAN_VAP_MODE_CONFIG != pst_mac_vap->en_vap_mode))
    {
         OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_TX, "{hmac_config_car_cfg:: CAR only used in AP mode; en_vap_mode=%d.}", pst_mac_vap->en_vap_mode);
         return OAL_FAIL;
    }

    pst_hmac_dev = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    pst_car_cfg_param = (mac_cfg_car_stru *)puc_param;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    OAM_WARNING_LOG4(0, OAM_SF_CAR, "{hmac_config_car_cfg::en_car_flag[%d], uc_car_up_down_type[%d], timer[%d] kbps[%d]!}\r\n",
        pst_car_cfg_param->en_car_flag,
        pst_car_cfg_param->uc_car_up_down_type,
        pst_car_cfg_param->us_car_timer_cycle_ms,
        pst_car_cfg_param->ul_bw_limit_kbps);
    OAM_WARNING_LOG2(0, OAM_SF_CAR, "{hmac_config_car_cfg::uc_car_enable[%d], multicast_pps_num[%d]!}\r\n", pst_car_cfg_param->en_car_enable_flag, pst_car_cfg_param->ul_car_multicast_pps_num);

    switch (pst_car_cfg_param->en_car_flag)
    {
        case MAC_CAR_DEVICE_LIMIT:
            hmac_car_device_bw_limit(pst_hmac_dev, pst_car_cfg_param); break;
        case MAC_CAR_VAP_LIMIT:
            hmac_car_vap_bw_limit(pst_hmac_vap, pst_car_cfg_param);break;
        case MAC_CAR_USER_LIMIT:
            hmac_car_user_bw_limit(pst_hmac_vap, pst_car_cfg_param);break;
        case MAC_CAR_TIMER_CYCLE_MS:
            hmac_car_device_timer_cycle_limit(pst_hmac_dev, pst_car_cfg_param); break;
        case MAC_CAR_ENABLE:
            hmac_car_enable_switch(pst_hmac_dev, pst_car_cfg_param); break;
        case MAC_CAR_SHOW_INFO:
            hmac_car_show_info(pst_hmac_dev); break;
        case MAC_CAR_MULTICAST:
            hmac_car_device_multicast(pst_hmac_dev, pst_car_cfg_param); break;
        case MAC_CAR_MULTICAST_PPS:
            hmac_car_device_multicast_pps_num(pst_hmac_dev, pst_car_cfg_param); break;
        default:
            break;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND

oal_uint32  hmac_config_waveapp_32plus_user_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;
    mac_chip_stru  *pst_mac_chip;
    pst_mac_chip = hmac_res_get_mac_chip(pst_mac_vap->uc_chip_id);
    if (OAL_PTR_NULL == pst_mac_chip)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_waveapp_32plus_user_enable::pst_mac_chip null.}");
        return OAL_FAIL;
    }
    pst_mac_chip->en_waveapp_32plus_user_enable = *puc_param;
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "hmac_config_waveapp_32plus_user_enable enter, en_waveapp_32plus_user_enable = %d\r\n", pst_mac_chip->en_waveapp_32plus_user_enable);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_WAVEAPP_32PLUS_USER_ENABLE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_waveapp_32plus_user_enable::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return ul_ret;
}
#endif


oal_uint32  hmac_config_rssi_limit(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_RSSI_LIMIT_CFG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_pcie_test::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

#ifdef _PRE_WLAN_PRODUCT_1151V200
oal_uint32 hmac_config_80m_rts_debug(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_80M_RTS_DEBUG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_80m_rts_debug::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_CHIP_TEST


oal_uint32  hmac_config_lpm_tx_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_lpm_tx_data_stru    *pst_lpm_tx_data;
    hmac_vap_stru               *pst_hmac_vap;
    oal_uint16                  us_send_num;
    oal_netbuf_stru             *pst_buf;
    mac_ether_header_stru       *pst_ether_header;
    mac_ip_header_stru          *pst_ip;
    oal_uint8                   uc_tid;
    oal_uint32                   ul_ret;


    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_alg_etc::pst_hmac_vap null.}");
        return OAL_FAIL;
    }

    pst_lpm_tx_data = (mac_cfg_lpm_tx_data_stru *)puc_param;

    for(us_send_num = 0; us_send_num < pst_lpm_tx_data->us_num; us_send_num++)
    {
        /* 创建SKB */
        pst_buf = oal_netbuf_alloc(pst_lpm_tx_data->us_len, 0, 4);
        oal_netbuf_put(pst_buf, pst_lpm_tx_data->us_len);

        oal_set_mac_addr(&pst_buf->data[0],pst_lpm_tx_data->auc_da);
        oal_set_mac_addr(&pst_buf->data[6], mac_mib_get_StationID(pst_mac_vap));

        pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
        /*lint -e778*/
        pst_ether_header->us_ether_type = OAL_HOST2NET_SHORT(ETHER_TYPE_IP);
        /*lint +e778*/
        pst_ip = (mac_ip_header_stru *)(pst_ether_header + 1);      /* 偏移一个以太网头，取ip头 */

        uc_tid = WLAN_WME_AC_TO_TID(pst_lpm_tx_data->uc_ac);

        pst_ip->uc_version_ihl = 0x45;
        pst_ip->uc_tos = (oal_uint8)(uc_tid << WLAN_IP_PRI_SHIFT);
        pst_ip->us_tot_len = oal_byteorder_host_to_net_uint16(pst_lpm_tx_data->us_len - 34);
        pst_ip->us_id = oal_byteorder_host_to_net_uint16(0x4000);
        pst_ip->us_frag_off = 0;
        pst_ip->uc_ttl = 128;
        pst_ip->uc_protocol = 0x06;
        pst_ip->us_check = 0;
        pst_ip->ul_saddr = oal_byteorder_host_to_net_uint32(0x010101c4);
        pst_ip->ul_daddr = oal_byteorder_host_to_net_uint32(0x010101c2);

        pst_buf->next = OAL_PTR_NULL;
        pst_buf->prev = OAL_PTR_NULL;
        OAL_MEMZERO(oal_netbuf_cb(pst_buf), OAL_NETBUF_CB_SIZE());

        ul_ret = hmac_tx_lan_to_wlan_etc(&pst_hmac_vap->st_vap_base_info, pst_buf);
        /* 调用失败，要释放内核申请的netbuff内存池 */
        if(OAL_SUCC != ul_ret)
        {
            hmac_free_netbuf_list_etc(pst_buf);
        }
    }

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_uint32  hmac_40M_intol_sync_event(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_40M_INTOL_UPDATE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
            "{hmac_40M_intol_sync_event::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_protection_update_from_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    mac_dump_protection_etc(pst_mac_vap, puc_param);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PROTECTION_UPDATE_STA_USER, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
            "{hmac_config_set_protection::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32  hmac_config_set_coex(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /*芯片验证功能，暂不保存到device属性*/

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_COEX, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_coex::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_dfx(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DFX_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_dfx::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32  hmac_config_set_thruput_bypass(mac_vap_stru *pst_mac_vap,wlan_cfgid_enum_uint16 en_cfg_id,oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret = OAL_SUCC;
    mac_cfg_set_thruput_bypass_stru *pst_set_thruput_bypass = (mac_cfg_set_thruput_bypass_stru *)puc_param;

    /* 如果是配置VAP, 直接返回 */
    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_thruput_bypass::this is config vap! can't set.}");
        return OAL_FAIL;
    }

    OAL_SET_THRUPUT_BYPASS_ENABLE(pst_set_thruput_bypass->uc_bypass_type, pst_set_thruput_bypass->uc_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if(OAL_TX_SDIO_SLAVE_BYPASS == pst_set_thruput_bypass->uc_bypass_type && 0 == pst_set_thruput_bypass->uc_value)
    {
        hcc_msg_slave_thruput_bypass_etc();
    }
#endif
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_thruput_bypass::bypass type:%u, value:%u.}",
                                        pst_set_thruput_bypass->uc_bypass_type, pst_set_thruput_bypass->uc_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_THRUPUT_BYPASS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_thruput_bypass::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}


oal_uint32  hmac_config_set_auto_protection_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret = 0;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_AUTO_PROTECTION, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_probe_switch_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_vap_state_syn_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_VAP_STATE_SYN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vap_state_syn_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_uint32  hmac_set_ipaddr_timeout_etc(void   *puc_para)
{
    oal_uint32          ul_ret;
    hmac_vap_stru       *pst_hmac_vap = (hmac_vap_stru *)puc_para;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(MAX_FAST_PS==pst_hmac_vap->uc_ps_mode)
    {
        wlan_pm_set_timeout_etc(g_wlan_fast_check_cnt);
    }
    else
    {
        wlan_pm_set_timeout_etc(WLAN_SLEEP_DEFAULT_CHECK_CNT);
    }
#endif

    /* 未主动dhcp成功,超时开低功耗 */
    ul_ret = hmac_config_set_pm_by_module_etc(&pst_hmac_vap->st_vap_base_info, MAC_STA_PM_CTRL_TYPE_HOST, MAC_STA_PM_SWITCH_ON);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_PWR, "{hmac_set_ipaddr_timeout_etc::hmac_config_set_pm_by_module_etc failed[%d].}", ul_ret);
    }

    return OAL_SUCC;
}

#endif
#endif


oal_uint32  hmac_config_user_asoc_state_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                     ul_ret;
    mac_h2d_user_asoc_state_stru   st_h2d_user_asoc_state_stru;

    st_h2d_user_asoc_state_stru.us_user_idx   = pst_mac_user->us_assoc_id;
    st_h2d_user_asoc_state_stru.en_asoc_state = pst_mac_user->en_user_asoc_state;

    /***************************************************************************
        抛事件到DMAC层, 同步user关联状态到device侧
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USER_ASOC_STATE_SYN, OAL_SIZEOF(mac_h2d_user_asoc_state_stru), (oal_uint8 *)(&st_h2d_user_asoc_state_stru));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_asoc_state_syn_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_user_cap_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                  ul_ret;
    mac_h2d_usr_cap_stru        st_mac_h2d_usr_cap;

    st_mac_h2d_usr_cap.us_user_idx = pst_mac_user->us_assoc_id;
    oal_memcopy((oal_uint8 *)(&st_mac_h2d_usr_cap.st_user_cap_info), (oal_uint8 *)(&pst_mac_user->st_cap_info), OAL_SIZEOF(mac_user_cap_info_stru));

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USER_CAP_SYN, OAL_SIZEOF(mac_h2d_usr_cap_stru), (oal_uint8 *)(&st_mac_h2d_usr_cap));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_cap_syn_etc::hmac_config_sta_vap_info_syn_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}


oal_uint32  hmac_config_user_rate_info_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                  ul_ret;
    mac_h2d_usr_rate_info_stru  st_mac_h2d_usr_rate_info;

    st_mac_h2d_usr_rate_info.us_user_idx   = pst_mac_user->us_assoc_id;
    st_mac_h2d_usr_rate_info.en_protocol_mode = pst_mac_user->en_protocol_mode;

    /* legacy速率集信息，同步到dmac */
    st_mac_h2d_usr_rate_info.uc_avail_rs_nrates = pst_mac_user->st_avail_op_rates.uc_rs_nrates;
    oal_memcopy(st_mac_h2d_usr_rate_info.auc_avail_rs_rates, pst_mac_user->st_avail_op_rates.auc_rs_rates, WLAN_MAX_SUPP_RATES);

    /* ht速率集信息，同步到dmac */
    mac_user_get_ht_hdl_etc(pst_mac_user, &st_mac_h2d_usr_rate_info.st_ht_hdl);

    /* vht速率集信息，同步到dmac */
    mac_user_get_vht_hdl_etc(pst_mac_user, &st_mac_h2d_usr_rate_info.st_vht_hdl);

#ifdef _PRE_WLAN_FEATURE_11AX
    /* he速率集信息，同步到dmac */
    mac_user_get_he_hdl(pst_mac_user, &(st_mac_h2d_usr_rate_info.st_he_hdl));
#endif

    /***************************************************************************
        抛事件到DMAC层, 同步user关联状态到device侧
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USER_RATE_SYN, sizeof(mac_h2d_usr_rate_info_stru), (oal_uint8 *)(&st_mac_h2d_usr_rate_info));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_rate_info_syn_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}



oal_uint32  hmac_config_user_info_syn_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                  ul_ret;
    mac_h2d_usr_info_stru       st_mac_h2d_usr_info;


    st_mac_h2d_usr_info.en_avail_bandwidth = pst_mac_user->en_avail_bandwidth;
    st_mac_h2d_usr_info.en_cur_bandwidth   = pst_mac_user->en_cur_bandwidth;
    st_mac_h2d_usr_info.us_user_idx        = pst_mac_user->us_assoc_id;
    st_mac_h2d_usr_info.en_user_pmf        = pst_mac_user->st_cap_info.bit_pmf_active;
    st_mac_h2d_usr_info.uc_arg1            = pst_mac_user->st_ht_hdl.uc_max_rx_ampdu_factor;
    st_mac_h2d_usr_info.uc_arg2            = pst_mac_user->st_ht_hdl.uc_min_mpdu_start_spacing;
    st_mac_h2d_usr_info.en_user_asoc_state = pst_mac_user->en_user_asoc_state;


    /* 协议模式信息同步到dmac */
    st_mac_h2d_usr_info.en_avail_protocol_mode  = pst_mac_user->en_avail_protocol_mode;

    st_mac_h2d_usr_info.en_cur_protocol_mode    = pst_mac_user->en_cur_protocol_mode;
    st_mac_h2d_usr_info.en_protocol_mode        = pst_mac_user->en_protocol_mode;
    st_mac_h2d_usr_info.en_bandwidth_cap        = pst_mac_user->en_bandwidth_cap;

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USR_INFO_SYN, OAL_SIZEOF(st_mac_h2d_usr_info), (oal_uint8 *)(&st_mac_h2d_usr_info));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_info_syn_etc::hmac_config_send_event_etc failed[%d],user_id[%d].}",
                    ul_ret, pst_mac_user->us_assoc_id);
    }

    return ul_ret;
}


oal_uint32  hmac_config_sta_vap_info_syn_etc(mac_vap_stru *pst_mac_vap)
{
    oal_uint32                  ul_ret;
    mac_h2d_vap_info_stru       st_mac_h2d_vap_info;

    st_mac_h2d_vap_info.us_sta_aid = pst_mac_vap->us_sta_aid;
    st_mac_h2d_vap_info.uc_uapsd_cap = pst_mac_vap->uc_uapsd_cap;
#ifdef _PRE_WLAN_FEATURE_TXOPPS
    st_mac_h2d_vap_info.en_txop_ps = mac_vap_get_txopps(pst_mac_vap);
#endif /* #ifdef _PRE_WLAN_FEATURE_TXOPPS */

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_STA_VAP_INFO_SYN, OAL_SIZEOF(mac_h2d_vap_info_stru), (oal_uint8 *)(&st_mac_h2d_vap_info));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_sta_vap_info_syn_etc::hmac_config_sta_vap_info_syn_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_init_user_security_port_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    oal_uint32                      ul_ret;
    mac_cfg80211_init_port_stru     st_init_port;

    /* 初始化认证端口信息 */
    mac_vap_init_user_security_port_etc(pst_mac_vap, pst_mac_user);

    oal_memcopy(st_init_port.auc_mac_addr, pst_mac_user->auc_user_mac_addr, OAL_MAC_ADDR_LEN);
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_INIT_SECURTIY_PORT, OAL_SIZEOF(st_init_port), (oal_uint8 *)&st_init_port);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WPA, "{hmac_config_user_security_port::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_user_set_asoc_state_etc(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, mac_user_asoc_state_enum_uint8 en_value)
{
    oal_uint32 ul_ret;

    mac_user_set_asoc_state_etc(pst_mac_user, en_value);

    /* dmac offload架构下，同步user关联状态信息到dmac */
    ul_ret = hmac_config_user_asoc_state_syn_etc(pst_mac_vap, pst_mac_user);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_user_set_asoc_state_etc::hmac_config_user_asoc_state_syn_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_config_ch_status_sync(mac_device_stru *pst_mac_dev)
{
    oal_uint32           ul_ret;
    mac_ap_ch_info_stru  ast_ap_channel_list[MAC_MAX_SUPP_CHANNEL] = {{0}};
    mac_vap_stru        *pst_mac_vap;

    if (!pst_mac_dev)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_ch_status_sync::pst_mac_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = mac_res_get_mac_vap(pst_mac_dev->auc_vap_id[0]);
    if (!pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_ch_status_sync::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memcopy((oal_uint8 *)ast_ap_channel_list, (oal_uint8 *)(pst_mac_dev->st_ap_channel_list), OAL_SIZEOF(ast_ap_channel_list));

    /***************************************************************************
        抛事件到DMAC层
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SYNC_CH_STATUS, OAL_SIZEOF(ast_ap_channel_list), (oal_uint8 *)ast_ap_channel_list);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_ch_status_sync::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif
#if 0

oal_uint32 hmac_add_vap_sysnc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_add_vap_sync_data_stru     *pst_mac_add_vap_sync_data;

    pst_mac_add_vap_sync_data = (mac_add_vap_sync_data_stru *)puc_param;

    pst_mac_vap->uc_p2p0_hal_vap_id = pst_mac_add_vap_sync_data->uc_p2p0_hal_vap_id;
    pst_mac_vap->uc_p2p_gocl_hal_vap_id = pst_mac_add_vap_sync_data->uc_p2p_gocl_hal_vap_id;

    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                    "{hmac_add_vap_sysnc::func out hal_vap_id[%d] gocl_hal_vap_id[%d].}",
                    pst_mac_vap->uc_p2p0_hal_vap_id,
                    pst_mac_vap->uc_p2p_gocl_hal_vap_id);

    return OAL_SUCC;
}
#endif

oal_uint32  hmac_config_send_2040_coext_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_2040_coexist_stru *pst_2040_coexist;
    oal_netbuf_stru   *pst_netbuf = OAL_PTR_NULL;
    mac_tx_ctl_stru   *pst_tx_ctl;
    oal_uint32         ul_ret;
    oal_uint16         us_frame_len = 0;

    /*只有STA需要处理*/
    if(WLAN_VAP_MODE_BSS_STA != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,"hmac_config_send_2040_coext_etc::en_vap_mode is[%d] not STAUT,return", pst_mac_vap->en_vap_mode);
        return OAL_SUCC;
    }

    pst_2040_coexist = (mac_cfg_set_2040_coexist_stru*)puc_param;
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_2040, "hmac_config_send_2040_coext_etc::coinfo=%d chan=%d",
        pst_2040_coexist->ul_coext_info, pst_2040_coexist->ul_channel_report);

    /* 申请管理帧内存 */
    pst_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_netbuf)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_config_send_2040_coext_etc::pst_netbuf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_NETBUF_PREV(pst_netbuf) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_netbuf) = OAL_PTR_NULL;

    /* 封装20/40 共存管理帧 */
    us_frame_len = mac_encap_2040_coext_mgmt_etc((oal_void *)pst_mac_vap, pst_netbuf,
            (oal_uint8)pst_2040_coexist->ul_coext_info, pst_2040_coexist->ul_channel_report);

    oal_netbuf_put(pst_netbuf, us_frame_len);

    /* 填写netbuf的cb字段，供发送管理帧和发送完成接口使用 */
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    OAL_MEMZERO(pst_tx_ctl, OAL_NETBUF_CB_SIZE());
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl) = us_frame_len;
    ul_ret = mac_vap_set_cb_tx_user_idx(pst_mac_vap, pst_tx_ctl, pst_mac_vap->auc_bssid);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_2040, "(hmac_config_send_2040_coext_etc::fail to find user by xx:xx:xx:0x:0x:0x.}",
        pst_mac_vap->auc_bssid[3],
        pst_mac_vap->auc_bssid[4],
        pst_mac_vap->auc_bssid[5]);
    }
    MAC_GET_CB_WME_AC_TYPE(pst_tx_ctl) = WLAN_WME_AC_MGMT;

    /* 抛事件到DMAC发送管理帧 */
    ul_ret = hmac_tx_mgmt_send_event_etc(pst_mac_vap, pst_netbuf, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_netbuf);

        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040, "{hmac_config_send_2040_coext_etc::hmac_tx_mgmt_send_event_etc failed.}", ul_ret);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_2040_coext_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device;
#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    mac_ap_ch_info_stru   *pst_ch_list;
    oal_uint32             ul_idx;
#endif
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_2040_coext_info_etc::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
    for (ul_idx = 0; ul_idx < MAC_MAX_SUPP_CHANNEL; ul_idx++)
    {
        pst_ch_list = &(pst_mac_device->st_ap_channel_list[ul_idx]);
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_config_2040_coext_info_etc::chan_idx=%d num_networks=%d, chan_type=%d.}",
                       ul_idx, pst_ch_list->us_num_networks, pst_ch_list->en_ch_type);
    }
#endif

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_FTM

oal_uint32  hmac_config_ftm_dbg(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                            ul_ret;
    mac_ftm_debug_switch_stru            *pst_ftm_debug;

    pst_ftm_debug = (mac_ftm_debug_switch_stru *)puc_param;

    /* ftm_initiator命令*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT0)
    {
        mac_mib_set_FineTimingMsmtInitActivated(pst_mac_vap, pst_ftm_debug->en_ftm_initiator_bit0);
        //mac_mib_set_WirelessManagementImplemented(pst_mac_vap, pst_ftm_debug->en_ftm_initiator_bit0);
    }
    /* 发送iftmr命令*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT1)
    {
        //dmac 发送
    }
    /*使能ftm命令*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT2)
    {
        //dmac 处理
    }
    if (pst_ftm_debug->ul_cmd_bit_map & BIT3)
    {
        //dmac 处理
    }
    /* 发送ftm命令*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT4)
    {
        //dmac 处理
    }
    /* ftm_resp命令*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT5)
    {
        mac_mib_set_FineTimingMsmtRespActivated(pst_mac_vap, pst_ftm_debug->en_ftm_resp_bit5);
        //dmac 处理
    }
    /*  设置校准时间*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT6)
    {
        //dmac 处理
    }
    /*  设置校准时间*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT7)
    {
        //dmac 处理
    }
    /* ftm_range命令*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT8)
    {
        mac_mib_set_FineTimingMsmtRangeRepActivated(pst_mac_vap, pst_ftm_debug->en_ftm_range_bit8);
        //dmac 处理
    }
    /* 获取环回校准*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT9)
    {
        //dmac 处理
    }
    /* 设置location*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT10)
    {
        //dmac 处理
    }
    /* 设置m2s*/
    if (pst_ftm_debug->ul_cmd_bit_map & BIT11)
    {
        //dmac 处理
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FTM_DBG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_protocol_debug_switch::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_get_version_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_VERSION, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_version_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}


oal_uint32  hmac_config_get_ant_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_ANT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_version_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}


oal_uint32  hmac_config_get_fem_pa_status_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CHECK_FEM_PA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_fem_pa_status_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}




#ifdef _PRE_DEBUG_MODE
oal_uint32  hmac_config_get_all_reg_value(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_ALL_REG_VALUE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_all_reg_value:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_get_cali_data(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret = OAL_SUCC;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_CALI_DATA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_cali_data:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif
#ifdef _PRE_WLAN_FEATURE_DAQ

oal_uint32  hmac_config_data_acq(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                   ul_ret;

    /* 如果是使能数据采集，则Down所有VAP */
    if (puc_param[0] == '2')
    {
        hmac_data_acq_down_vap(pst_mac_vap);
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DATA_ACQ, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_data_acq::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif
#ifdef _PRE_WLAN_FEATURE_SMPS
#ifdef _PRE_DEBUG_MODE

oal_uint32  hmac_config_get_smps_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru             *pst_mac_device;
    oal_uint32                   uc_user_idx = 0;
    oal_dlist_head_stru         *pst_head;
    mac_user_stru               *pst_user_tmp;
    oal_int8                     ac_tmp_buff[512]   = {0};
    oal_int32                    l_remainder_len    = 0;
    oal_int8                     uc_vap_index;
    oal_int8                    *pc_print_buff;
    hmac_vap_stru               *pst_hmac_vap;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_smps_info::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru*)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_smps_info::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);

    OAL_SPRINTF(pc_print_buff, OAM_REPORT_MAX_STRING_LEN, "The device nosmps num is %d.\r\n",
                pst_hmac_vap->uc_no_smps_user_cnt_ap);
    l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));

    for (uc_vap_index = 0; uc_vap_index < pst_mac_device->uc_vap_num; uc_vap_index++)
    {
        pst_mac_vap = mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_index]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_get_smps_info::pst_mac_vap null.}");
            continue;
        }

        OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), "VAP Index[%d] SMPS CAP IS [%s]; SMPS MODE IS [%s]!\r\n",
                    pst_mac_device->auc_vap_id[uc_vap_index],
                    hmac_config_smps2string_etc(mac_vap_get_smps_mode(pst_mac_vap) - 1),
                    hmac_config_smps2string_etc(mac_mib_get_smps(pst_mac_vap) - 1));

        oal_strncat(pc_print_buff, ac_tmp_buff, l_remainder_len-1);
        OAL_MEMZERO(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff));
        l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));

        /* AP侧的USER信息 */
        for (uc_user_idx = 0; uc_user_idx < MAC_VAP_USER_HASH_MAX_VALUE; uc_user_idx++)
        {
            OAL_DLIST_SEARCH_FOR_EACH(pst_head, &(pst_mac_vap->ast_user_hash[uc_user_idx]))
            {
                /* 找到相应用户 */
                pst_user_tmp = (mac_user_stru *)OAL_DLIST_GET_ENTRY(pst_head, mac_user_stru, st_user_hash_dlist);
                if (OAL_PTR_NULL == pst_user_tmp)
                {
                    OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_smps_info::pst_mac_user null.}");
                    continue;
                }

                OAL_SPRINTF(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff), "[%d] USER MACADDR [%02X:XX:XX:%02X:%02X:%02X] SMPS CAP is [%s]! \r\n",
                            uc_user_idx,
                            pst_user_tmp->auc_user_mac_addr[0],
                            pst_user_tmp->auc_user_mac_addr[3],
                            pst_user_tmp->auc_user_mac_addr[4],
                            pst_user_tmp->auc_user_mac_addr[5],
                            hmac_config_smps2string_etc(pst_user_tmp->st_ht_hdl.bit_sm_power_save - 1));
                oal_strncat(pc_print_buff, ac_tmp_buff, l_remainder_len-1);
                OAL_MEMZERO(ac_tmp_buff, OAL_SIZEOF(ac_tmp_buff));
                l_remainder_len = (oal_int32)(OAM_REPORT_MAX_STRING_LEN - OAL_STRLEN(pc_print_buff));

            }

        }

    }

    oam_print_etc(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
    return OAL_SUCC;
}
#endif
#endif


#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY

oal_uint32  hmac_config_set_opmode_notify_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8       uc_value;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32      ul_ret = OAL_SUCC;
#endif

    uc_value     = *puc_param;

    if (OAL_TRUE == mac_mib_get_VHTOptionImplemented(pst_mac_vap))
    {
        mac_mib_set_OperatingModeNotificationImplemented(pst_mac_vap, (oal_bool_enum_uint8)uc_value);
    }
    else
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{hmac_config_set_opmode_notify_etc::pst_mac_vap is not 11ac. en_protocol = [%d].}\r\n", pst_mac_vap->en_protocol);
        return OAL_FAIL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_OPMODE_NOTIFY, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{hmac_config_set_opmode_notify_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_user_rssbw_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_add_user_param_stru    *pst_user;
    hmac_vap_stru                  *pst_hmac_vap;
    hmac_user_stru                 *pst_hmac_user;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    oal_uint32                      ul_ret;
#endif

    pst_user = (mac_cfg_add_user_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_OPMODE, "{hmac_config_get_user_rssbw_etc::pst_hmac_vap null.}");
        return OAL_FAIL;
    }

    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_user->auc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{hmac_config_get_user_rssbw_etc::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{hmac_config_get_user_rssbw_etc:: user macaddr %02X:XX:XX:%02X:%02X:%02X.}",
                pst_user->auc_mac_addr[0], pst_user->auc_mac_addr[3], pst_user->auc_mac_addr[4], pst_user->auc_mac_addr[5]);
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{hmac_config_get_user_rssbw_etc::nss_cap[%d]avail_nss[%d]user bw_cap[%d]avail_bw[%d].}",
                pst_hmac_user->st_user_base_info.en_user_num_spatial_stream, pst_hmac_user->st_user_base_info.en_avail_num_spatial_stream,
                pst_hmac_user->st_user_base_info.en_bandwidth_cap, pst_hmac_user->st_user_base_info.en_avail_bandwidth);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_USER_RSSBW, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_OPMODE, "{hmac_config_get_user_rssbw_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }
#endif

    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_M2S

oal_uint32  hmac_config_set_m2s_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                    ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_M2S_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_config_set_m2s_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_set_vap_nss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8            uc_value;
    oal_uint32           ul_ret;

    uc_value = *puc_param;

    if (((WLAN_FOUR_NSS + 1) < uc_value) || ((WLAN_SINGLE_NSS + 1) > uc_value))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vap_nss::uc_value is limit! value = [%d].}\r\n", uc_value);
        return OAL_FAIL;
    }

    if (MAC_VAP_STATE_INIT != pst_mac_vap->en_vap_state)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vap_nss::vap is up,please down vap first. VAP STATE = [%d].}\r\n", pst_mac_vap->en_vap_state);
        oam_report_backtrace();
        return OAL_FAIL;
    }

    if (WLAN_PROTOCOL_BUTT == pst_mac_vap->en_protocol)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{mac_vap_init_rx_nss_by_protocol_etc::please set mode first!}\r\n");
        return OAL_FAIL;
    }

    mac_vap_init_rx_nss_by_protocol_etc(pst_mac_vap);
    mac_vap_set_rx_nss_etc(pst_mac_vap, OAL_MIN(pst_mac_vap->en_vap_rx_nss, (uc_value - 1)));

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_VAP_NSS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_vap_nss::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE


oal_uint32  hmac_config_report_ampdu_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_REPORT_AMPDU_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_report_ampdu_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif


oal_uint32  hmac_config_set_ampdu_aggr_num_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32    ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_AGGR_NUM, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ampdu_aggr_num_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)

oal_uint32  hmac_config_set_ampdu_mmss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_ampdu_mmss_stru   *pst_ampdu_mmss_ctrl;

    pst_ampdu_mmss_ctrl = (mac_cfg_ampdu_mmss_stru *)puc_param;

    mac_mib_set_min_mpdu_start_spacing(pst_mac_vap, (oal_uint8)pst_ampdu_mmss_ctrl->uc_mmss_val);

    OAL_IO_PRINT("hmac_config_set_ampdu_mmss: mmss[%d] \n", pst_ampdu_mmss_ctrl->uc_mmss_val);

    return OAL_SUCC;
}
#endif

#ifdef _PRE_DEBUG_MODE

oal_uint32  hmac_config_freq_adjust(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FREQ_ADJUST, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_freq_adjust::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS

oal_uint32  hmac_config_set_psd_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_PSD, us_len, puc_param);
}

oal_uint32  hmac_config_cfg_psd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CFG_PSD, us_len, puc_param);
}

#endif
#ifdef _PRE_WLAN_FEATURE_CSI

oal_uint32  hmac_config_set_csi(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_CSI, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_csi::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

oal_uint32  hmac_config_set_stbc_cap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8   en_value;

    en_value = *puc_param;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap->pst_mib_info))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_stbc_cap_etc::pst_mac_vap->pst_mib_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* siso能力时，配置命令不打开stbc的TX能力 */
    if (pst_mac_vap->en_vap_rx_nss >= WLAN_DOUBLE_NSS)
    {
        mac_mib_set_TxSTBCOptionImplemented(pst_mac_vap, en_value);
        mac_mib_set_TxSTBCOptionActivated(pst_mac_vap, en_value);
        mac_mib_set_VHTTxSTBCOptionImplemented(pst_mac_vap, en_value);
    }
    else
    {
        mac_mib_set_TxSTBCOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_TxSTBCOptionActivated(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTTxSTBCOptionImplemented(pst_mac_vap, OAL_FALSE);
    }

    mac_mib_set_RxSTBCOptionImplemented(pst_mac_vap, en_value);
    mac_mib_set_VHTRxSTBCOptionImplemented(pst_mac_vap, en_value);

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_stbc_cap_etc::set stbc cap [%d].}", en_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) /*hi1102-cb set at both side (HMAC to DMAC) */
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_STBC_CAP, us_len, puc_param);
#else
    return OAL_SUCC;
#endif

}


oal_uint32  hmac_config_set_ldpc_cap_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8   uc_value;

    uc_value = (oal_bool_enum_uint8)(*puc_param);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap->pst_mib_info))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ldpc_cap_etc::pst_mac_vap->pst_mib_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (1 == uc_value)
    {
        mac_mib_set_LDPCCodingOptionImplemented(pst_mac_vap, OAL_TRUE);
        mac_mib_set_LDPCCodingOptionActivated(pst_mac_vap, OAL_TRUE);
        mac_mib_set_VHTLDPCCodingOptionImplemented(pst_mac_vap, OAL_TRUE);
    }
    else if (0 == uc_value)
    {
        mac_mib_set_LDPCCodingOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_LDPCCodingOptionActivated(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTLDPCCodingOptionImplemented(pst_mac_vap, OAL_FALSE);
    }
    else
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ldpc_cap_etc::ldpc_value is limit! value = [%d].}\r\n", uc_value);
        return OAL_ERR_CODE_CONFIG_EXCEED_SPEC;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) /*hi1102-cb set at both side (HMAC to DMAC) */
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_LDPC_CAP, us_len, puc_param);
#else
    return OAL_SUCC;
#endif

}


oal_uint32  hmac_config_set_txbf_cap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru      *pst_mac_device = OAL_PTR_NULL;
    oal_uint8             uc_value       = 0;
    oal_bool_enum_uint8   en_rx_switch   = OAL_FALSE;
    oal_bool_enum_uint8   en_tx_switch   = OAL_FALSE;
    oal_uint8             uc_rx_sts_num  = 0;

    uc_value = *puc_param;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap->pst_mib_info))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_txbf_cap::pst_mac_vap->pst_mib_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_txbf_cap::pst_mac_dev[%d] null.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_rx_switch  = uc_value & 0x1;
    en_tx_switch  = (uc_value & 0x2) >> 1;
    uc_rx_sts_num = (en_rx_switch & OAL_TRUE) ? VHT_BFEE_NTX_SUPP_STS_CAP : 1;

    /* siso能力时，配置命令不打开txbf的TX能力 */
    if (pst_mac_vap->en_vap_rx_nss >= WLAN_DOUBLE_NSS)
    {
#ifdef _PRE_WLAN_FEATURE_TXBF_HT
        mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap, en_tx_switch);
        pst_mac_vap->st_txbf_add_cap.bit_exp_comp_txbf_cap = en_tx_switch;
#endif
        mac_mib_set_VHTSUBeamformerOptionImplemented(pst_mac_vap, en_tx_switch);
        mac_mib_set_VHTNumberSoundingDimensions(pst_mac_vap, MAC_DEVICE_GET_NSS_NUM(pst_mac_device));
    }
    else
    {
        mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTSUBeamformerOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_VHTNumberSoundingDimensions(pst_mac_vap, WLAN_SINGLE_NSS);
    }


#ifdef _PRE_WLAN_FEATURE_TXBF_HT
    mac_mib_set_ReceiveStaggerSoundingOptionImplemented(pst_mac_vap, en_rx_switch);
    mac_mib_set_NumberCompressedBeamformingMatrixSupportAntenna(pst_mac_vap, uc_rx_sts_num);
    mac_mib_set_ExplicitCompressedBeamformingFeedbackOptionImplemented(pst_mac_vap, en_rx_switch & WLAN_MIB_HT_ECBF_DELAYED);
    pst_mac_vap->st_txbf_add_cap.bit_channel_est_cap = en_rx_switch;
#endif
    mac_mib_set_VHTSUBeamformeeOptionImplemented(pst_mac_vap, en_rx_switch);
    mac_mib_set_VHTBeamformeeNTxSupport(pst_mac_vap, uc_rx_sts_num);
#if (WLAN_MU_BFEE_ACTIVED == WLAN_MU_BFEE_ENABLE)
    mac_mib_set_VHTMUBeamformeeOptionImplemented(pst_mac_vap, en_rx_switch);
#endif

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_txbf_cap::rx_cap[%d], tx_cap[%d], rx_sts_nums[%d].}",
                                en_rx_switch, en_tx_switch, uc_rx_sts_num);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE) /*hi110x-cb set at both side (HMAC to DMAC) */
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_TXBF_SWITCH, us_len, puc_param);
#else
    return OAL_SUCC;
#endif

}


#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)

oal_uint32  hmac_config_set_blacklist_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_mode;

    pul_cfg_mode = (oal_uint32 *)puc_param;
    ul_ret = hmac_blacklist_set_mode_etc(pst_mac_vap, (oal_uint8)*pul_cfg_mode);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_blacklist_set_mode_etc fail: ret=%d; mode=%d}\r\n", ul_ret, *pul_cfg_mode);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_get_blacklist_mode(mac_vap_stru *pst_mac_vap, oal_uint16* pus_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    ul_ret = hmac_blacklist_get_mode(pst_mac_vap, puc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_blacklist_get_mode fail: ret=%d; mode=%d}\r\n", ul_ret, *puc_param);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_blacklist_add_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_blacklist_stru         *pst_blklst;
    mac_vap_stru               *pst_assoc_vap = OAL_PTR_NULL;
    oal_bool_enum_uint8         en_assoc_vap = OAL_FALSE;

    pst_blklst  = (mac_blacklist_stru*)puc_param;
    en_assoc_vap = hmac_blacklist_get_assoc_ap(pst_mac_vap, pst_blklst->auc_mac_addr, &pst_assoc_vap);
    if (OAL_TRUE == en_assoc_vap)
    {
        ul_ret = hmac_blacklist_add_etc(pst_assoc_vap, pst_blklst->auc_mac_addr, pst_blklst->ul_aging_time);
    }
    else
    {
        ul_ret = hmac_blacklist_add_etc(pst_mac_vap, pst_blklst->auc_mac_addr, pst_blklst->ul_aging_time);
    }
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_blacklist_add_etc fail: ret=%d;}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_blacklist_add_only_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_blacklist_stru         *pst_blklst;
    mac_vap_stru               *pst_assoc_vap = OAL_PTR_NULL;
    oal_bool_enum_uint8         en_assoc_vap = OAL_FALSE;

    pst_blklst  = (mac_blacklist_stru*)puc_param;
    en_assoc_vap = hmac_blacklist_get_assoc_ap(pst_mac_vap, pst_blklst->auc_mac_addr, &pst_assoc_vap);
    if (OAL_TRUE == en_assoc_vap)
    {
        ul_ret = hmac_blacklist_add_only_etc(pst_assoc_vap, pst_blklst->auc_mac_addr, 0);
    }
    else
    {
        ul_ret = hmac_blacklist_add_only_etc(pst_mac_vap, pst_blklst->auc_mac_addr, 0);
    }
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_blacklist_add_only_etc fail: ret=%d;}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}



oal_uint32  hmac_config_blacklist_del_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_vap_stru               *pst_assoc_vap = OAL_PTR_NULL;
    oal_bool_enum_uint8         en_assoc_vap = OAL_FALSE;

    en_assoc_vap = hmac_blacklist_get_assoc_ap(pst_mac_vap, puc_param, &pst_assoc_vap);
    if (OAL_TRUE == en_assoc_vap)
    {
        ul_ret = hmac_blacklist_del_etc(pst_assoc_vap, puc_param);
    }
    else
    {
        ul_ret = hmac_blacklist_del_etc(pst_mac_vap, puc_param);
    }
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_blacklist_del_etc::blacklist_del fail: ret=%d;}\r\n", ul_ret);

        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_show_blacklist_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_show_blacklist_info_etc(pst_mac_vap);
    return OAL_SUCC;
}


oal_uint32  hmac_config_autoblacklist_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32  ul_ret;
    oal_uint8   uc_enable;

    uc_enable = *puc_param;

    ul_ret = hmac_autoblacklist_enable_etc(pst_mac_vap, uc_enable);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_autoblacklist_enable_etc fail: ret=%d; cfg=%d}\r\n", ul_ret, *puc_param);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_autoblacklist_aging_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_aging_time;

    pul_cfg_aging_time = (oal_uint32 *)puc_param;
    ul_ret = hmac_autoblacklist_set_aging_etc(pst_mac_vap, *pul_cfg_aging_time);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_autoblacklist_set_aging_etc fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_aging_time);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_autoblacklist_threshold_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_threshold;

    pul_cfg_threshold = (oal_uint32 *)puc_param;
    ul_ret = hmac_autoblacklist_set_threshold_etc(pst_mac_vap, *pul_cfg_threshold);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_autoblacklist_set_threshold_etc fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_threshold);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_autoblacklist_reset_time_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_reset_time;

    pul_cfg_reset_time = (oal_uint32 *)puc_param;
    ul_ret = hmac_autoblacklist_set_reset_time_etc(pst_mac_vap, *pul_cfg_reset_time);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_autoblacklist_set_reset_time_etc fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_reset_time);
        return ul_ret;
    }
    return OAL_SUCC;
}
#endif /* (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE) */

#ifdef _PRE_WLAN_WEB_CMD_COMM

oal_uint32  hmac_config_get_temp(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint32                      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_TEMP, *pus_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_temp::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_get_temp_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32                        *pl_param;
    hmac_vap_stru               *pst_hmac_vap;

    pl_param = (oal_int32 *)puc_param;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_temp_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->l_temp = *pl_param;
    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ISOLATION

oal_uint32  hmac_config_show_isolation(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_show_isolation_info(pst_mac_vap);
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_mode;

    pul_cfg_mode = (oal_uint32 *)puc_param;
    ul_ret = hmac_isolation_set_mode(pst_mac_vap, (oal_uint8)*pul_cfg_mode);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_isolation_set_mode fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_mode);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    ul_ret = hmac_isolation_set_type(pst_mac_vap, puc_param[0], puc_param[1]);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_isolation_set_type fail: ret=%d; bss_type=%d, isolation_type=%d}\r\n", ul_ret, puc_param[0], puc_param[1]);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_forword(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    oal_uint32                 *pul_cfg_forword;

    pul_cfg_forword = (oal_uint32 *)puc_param;
    ul_ret = hmac_isolation_set_forward(pst_mac_vap, (oal_uint8)*pul_cfg_forword);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_isolation_set_forward fail: ret=%d; cfg=%d}\r\n", ul_ret, *pul_cfg_forword);
        return ul_ret;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_set_isolation_clear(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    ul_ret = hmac_isolation_clear_counter(pst_mac_vap);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_isolation_clear_counter fail: ret=%d; cfg=%d}\r\n", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;
}
#endif  /* _PRE_WLAN_FEATURE_ISOLATION */


oal_uint32 hmac_config_set_pmksa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_pmksa_param_stru           *pst_cfg_pmksa;
    hmac_pmksa_cache_stru              *pst_pmksa_cache;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_dlist_head_stru                *pst_pmksa_entry;
    oal_dlist_head_stru                *pst_pmksa_entry_tmp;
    oal_uint32                          ul_pmksa_count = 0;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_pmksa_etc param null}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_pmksa    = (mac_cfg_pmksa_param_stru *)puc_param;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_pmksa_etc::pst_hmac_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (oal_dlist_is_empty(&(pst_hmac_vap->st_pmksa_list_head)))
    {
        oal_dlist_init_head(&(pst_hmac_vap->st_pmksa_list_head));
    }

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_pmksa_entry, pst_pmksa_entry_tmp, &(pst_hmac_vap->st_pmksa_list_head))
    {
        pst_pmksa_cache = OAL_DLIST_GET_ENTRY(pst_pmksa_entry, hmac_pmksa_cache_stru, st_entry);
        /* 已存在时，先删除，保证最新的pmk在dlist头部 */
        if (0 == oal_compare_mac_addr(pst_cfg_pmksa->auc_bssid, pst_pmksa_cache->auc_bssid))
        {
            oal_dlist_delete_entry(pst_pmksa_entry);
            OAL_MEM_FREE(pst_pmksa_cache, OAL_TRUE);
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                             "{hmac_config_set_pmksa_etc:: DEL first [%02X:XX:XX:XX:%02X:%02X]}",
                             pst_cfg_pmksa->auc_bssid[0], pst_cfg_pmksa->auc_bssid[4], pst_cfg_pmksa->auc_bssid[5]);
        }
        ul_pmksa_count++;
    }

    if (ul_pmksa_count > WLAN_PMKID_CACHE_SIZE)
    {
        /* 超过最大个数时，先队列尾，保证最新的pmk在dlist头部 */
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_pmksa_etc:: can't store more pmksa for [%02X:XX:XX:XX:%02X:%02X]}",
                         pst_cfg_pmksa->auc_bssid[0], pst_cfg_pmksa->auc_bssid[4], pst_cfg_pmksa->auc_bssid[5]);
        pst_pmksa_entry  = oal_dlist_delete_tail(&(pst_hmac_vap->st_pmksa_list_head));
        pst_pmksa_cache  = OAL_DLIST_GET_ENTRY(pst_pmksa_entry, hmac_pmksa_cache_stru, st_entry);
        OAL_MEM_FREE(pst_pmksa_cache, OAL_TRUE);
    }

    pst_pmksa_cache = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAL_SIZEOF(hmac_pmksa_cache_stru), OAL_TRUE);
    if (OAL_PTR_NULL == pst_pmksa_cache)
    {
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_set_pmksa_etc:: OAL_MEM_ALLOC fail [%02X:XX:XX:XX:%02X:%02X]}",
                         pst_cfg_pmksa->auc_bssid[0], pst_cfg_pmksa->auc_bssid[4], pst_cfg_pmksa->auc_bssid[5]);
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memcopy(pst_pmksa_cache->auc_bssid, pst_cfg_pmksa->auc_bssid, OAL_MAC_ADDR_LEN);
    oal_memcopy(pst_pmksa_cache->auc_pmkid, pst_cfg_pmksa->auc_pmkid, WLAN_PMKID_LEN);

    oal_dlist_add_head(&(pst_pmksa_cache->st_entry), &(pst_hmac_vap->st_pmksa_list_head));

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                     "{hmac_config_set_pmksa_etc:: SET pmksa for [%02X:XX:XX:XX:%02X:%02X] OK!}",
                     pst_cfg_pmksa->auc_bssid[0], pst_cfg_pmksa->auc_bssid[4], pst_cfg_pmksa->auc_bssid[5]);

    return OAL_SUCC;
}


oal_uint32 hmac_config_del_pmksa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_pmksa_param_stru           *pst_cfg_pmksa;
    hmac_pmksa_cache_stru              *pst_pmksa_cache;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_dlist_head_stru                *pst_pmksa_entry;
    oal_dlist_head_stru                *pst_pmksa_entry_tmp;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_del_pmksa_etc param null}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_pmksa    = (mac_cfg_pmksa_param_stru *)puc_param;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_del_pmksa_etc::pst_hmac_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (oal_dlist_is_empty(&(pst_hmac_vap->st_pmksa_list_head)))
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_del_pmksa_etc:: pmksa dlist is null [%02X:XX:XX:XX:%02X:%02X]}",
                         pst_cfg_pmksa->auc_bssid[0], pst_cfg_pmksa->auc_bssid[4], pst_cfg_pmksa->auc_bssid[5]);
    }

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_pmksa_entry, pst_pmksa_entry_tmp, &(pst_hmac_vap->st_pmksa_list_head))
    {
        pst_pmksa_cache = OAL_DLIST_GET_ENTRY(pst_pmksa_entry, hmac_pmksa_cache_stru, st_entry);

        if (0 == oal_compare_mac_addr(pst_cfg_pmksa->auc_bssid, pst_pmksa_cache->auc_bssid))
        {

            oal_dlist_delete_entry(pst_pmksa_entry);
            OAL_MEM_FREE(pst_pmksa_cache, OAL_TRUE);
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                             "{hmac_config_del_pmksa_etc:: DEL pmksa of [%02X:XX:XX:XX:%02X:%02X]}",
                             pst_cfg_pmksa->auc_bssid[0], pst_cfg_pmksa->auc_bssid[4], pst_cfg_pmksa->auc_bssid[5]);
            return OAL_SUCC;
        }
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                     "{hmac_config_del_pmksa_etc:: NO pmksa of [%02X:XX:XX:XX:%02X:%02X]}",
                     pst_cfg_pmksa->auc_bssid[0], pst_cfg_pmksa->auc_bssid[4], pst_cfg_pmksa->auc_bssid[5]);
    return OAL_SUCC;
}


oal_uint32 hmac_config_flush_pmksa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_pmksa_cache_stru              *pst_pmksa_cache;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_dlist_head_stru                *pst_pmksa_entry;
    oal_dlist_head_stru                *pst_pmksa_entry_tmp;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_flush_pmksa_etc param null}\r\n");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_flush_pmksa_etc::pst_hmac_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (oal_dlist_is_empty(&(pst_hmac_vap->st_pmksa_list_head)))
    {
        return OAL_SUCC;
    }

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_pmksa_entry, pst_pmksa_entry_tmp, &(pst_hmac_vap->st_pmksa_list_head))
    {
        pst_pmksa_cache = OAL_DLIST_GET_ENTRY(pst_pmksa_entry, hmac_pmksa_cache_stru, st_entry);

        oal_dlist_delete_entry(pst_pmksa_entry);
        OAL_MEM_FREE(pst_pmksa_cache, OAL_TRUE);
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                         "{hmac_config_flush_pmksa_etc:: DEL pmksa of [%02X:XX:XX:XX:%02X:%02X]}",
                         pst_pmksa_cache->auc_bssid[0], pst_pmksa_cache->auc_bssid[4], pst_pmksa_cache->auc_bssid[5]);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_scan_abort_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru           *pst_hmac_vap;
    hmac_device_stru        *pst_hmac_device;
    oal_uint32               ul_ret;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_config_scan_abort_etc::pst_hmac_device is null, dev_id[%d].}",
                         pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                     "{hmac_config_scan_abort_etc::vap_id[%d] scan abort,curr_scan_vap_id:%d}",
                     pst_mac_vap->uc_vap_id, pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.uc_vap_id);

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN,
                         "{hmac_config_scan_abort_etc::pst_hmac_vap is null, vap_id[%d].}",
                         pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_MAC_DEVICE_NULL;
    }

    /* 根据当前扫描的类型和当前vap的状态，决定切换vap的状态，如果是前景扫描，才需要切换vap的状态 */
    if (WLAN_VAP_MODE_BSS_STA == pst_hmac_vap->st_vap_base_info.en_vap_mode)
    {
        if (MAC_VAP_STATE_STA_WAIT_SCAN == pst_hmac_vap->st_vap_base_info.en_vap_state)
        {
            /* 改变vap状态到SCAN_COMP */
            hmac_fsm_change_state_etc(pst_hmac_vap, MAC_VAP_STATE_STA_SCAN_COMP);
        }

    }

    if ((WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        && (MAC_VAP_STATE_BUTT != pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state))
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_config_scan_abort_etc::en_vap_last_state:%d}",
                         pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state);
        hmac_fsm_change_state_etc(pst_hmac_vap, pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state);
        pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.en_vap_last_state = MAC_VAP_STATE_BUTT;
    }



    /* 清除扫描结果上报的回调函数，无需上报 */
    if (pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.uc_vap_id == pst_mac_vap->uc_vap_id)
    {
        pst_hmac_device->st_scan_mgmt.st_scan_record_mgmt.p_fn_cb = OAL_PTR_NULL;
        pst_hmac_device->st_scan_mgmt.en_is_scanning = OAL_FALSE;
    }

    /***************************************************************************
                         抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap,
                                    WLAN_CFGID_SCAN_ABORT,
                                    us_len,
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_SCAN, "{hmac_config_scan_abort_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_remain_on_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_remain_on_channel_param_stru   *pst_remain_on_channel;
    mac_device_stru                    *pst_mac_device;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_uint32                          ul_ret;

    /* 1.1 判断入参 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_P2P, "{hmac_config_remain_on_channel_etc null ptr: pst_mac_vap=%d; puc_param=%d}\r\n",
                        pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_remain_on_channel = (mac_remain_on_channel_param_stru *)puc_param;
    pst_mac_device        = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_remain_on_channel_etc::pst_mac_device[%p] null!}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 1.2 检查是否能进入监听状态 */
    ul_ret = hmac_p2p_check_can_enter_state_etc(pst_mac_vap, HMAC_FSM_INPUT_LISTEN_REQ);
    if (ul_ret != OAL_SUCC)
    {
        /* 不能进入监听状态，返回设备忙 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                        "{hmac_config_remain_on_channel_etc fail,device busy: ul_ret=%d}\r\n", ul_ret);
        return OAL_ERR_CODE_CONFIG_BUSY;
    }


    /* 1.3 获取home 信道和信道类型。如果返回主信道为0，表示没有设备处于up 状态，监听后不需要返回主信道 */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                        "{hmac_config_remain_on_channel_etc::mac_res_get_hmac_vap null.vap_id = %d}",pst_mac_vap->uc_vap_id);
        return OAL_PTR_NULL;
    }

    /* 保存内核下发的监听信道信息，用于监听超时或取消监听时返回 */
    pst_mac_device->st_p2p_info.st_listen_channel                  = pst_remain_on_channel->st_listen_channel;

    /* 由于p2p0和 p2p cl 共用一个VAP 结构，故在进入监听时，需要保存之前的状态，便于监听结束时返回 */
    if (pst_mac_vap->en_vap_state != MAC_VAP_STATE_STA_LISTEN)
    {
        pst_mac_device->st_p2p_info.en_last_vap_state           = pst_mac_vap->en_vap_state;
    }
    pst_remain_on_channel->en_last_vap_state = pst_mac_device->st_p2p_info.en_last_vap_state;

    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                  "{hmac_config_remain_on_channel_etc :: listen_channel=%d, current_channel=%d, last_state=%d}\r\n",
                  pst_remain_on_channel->uc_listen_channel,
                  pst_mac_vap->st_channel.uc_chan_number,
                  pst_mac_device->st_p2p_info.en_last_vap_state);

    /* 3.1 修改VAP 状态为监听 */
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_P2P,
                        "{hmac_config_remain_on_channel_etc fail!pst_hmac_vap is null}\r\n");
        return OAL_PTR_NULL;
    }

    /* 状态机调用:  hmac_p2p_remain_on_channel_etc */
    ul_ret = hmac_fsm_call_func_sta_etc(pst_hmac_vap, HMAC_FSM_INPUT_LISTEN_REQ, (oal_void *)(pst_remain_on_channel));
    if (ul_ret != OAL_SUCC)
    {
        /* DMAC 设置切换信道失败 */
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{hmac_config_remain_on_channel_etc fail: ul_ret=%d}\r\n", ul_ret);
        return OAL_ERR_CODE_CONFIG_BUSY;
    }

    OAM_INFO_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "{hmac_config_remain_on_channel_etc succ: l_channel=%d, ul_duration=%d, band=%d}\r\n",
                   pst_remain_on_channel->uc_listen_channel, pst_remain_on_channel->ul_listen_duration, pst_remain_on_channel->en_band);
    return OAL_SUCC;
}


oal_uint32 hmac_config_cancel_remain_on_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap    = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_P2P, "hmac_config_cancel_remain_on_channel_etc::mac_res_get_hmac_vap fail.vap_id = %u", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (MAC_VAP_STATE_STA_LISTEN == pst_mac_vap->en_vap_state)
    {
#ifdef _PRE_WLAN_FEATURE_P2P
        hmac_p2p_listen_timeout_etc(pst_hmac_vap, pst_mac_vap);
#endif
    }
    else
    {
        hmac_p2p_send_listen_expired_to_host_etc(pst_hmac_vap);
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_vap_classify_en_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    mac_device_stru    *pst_mac_device;
    oal_int8            ac_string[OAM_PRINT_FORMAT_LENGTH];

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_WARNING_LOG1(0, OAM_SF_P2P, "hmac_config_vap_classify_en_etc::mac_res_get_dev_etc fail.device_id = %u", pst_mac_vap->uc_device_id);

        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (0xff == ul_val)
    {
        /* 打印当前的值 */
        OAL_SPRINTF(ac_string, sizeof(ac_string), "device classify en is %d\n",
                    pst_mac_device->en_vap_classify);

        oam_print_etc(ac_string);

        return OAL_SUCC;
    }

    if (0 == ul_val)
    {
        pst_mac_device->en_vap_classify = OAL_FALSE;
    }
    else
    {
        pst_mac_device->en_vap_classify = OAL_TRUE;
    }

    return OAL_SUCC;
}

oal_uint32  hmac_config_query_station_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_STATION_STATS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_phy_stat_info::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_query_rssi_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_RSSI, us_len, puc_param);

    return ul_ret;
}


oal_uint32  hmac_config_query_psst(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_PSST, us_len, puc_param);

    return ul_ret;
}

#ifdef _PRE_WLAN_WEB_CMD_COMM
#ifdef _PRE_WLAN_11K_STAT

oal_uint32  hmac_config_query_drop_num(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_DROP_NUM, us_len, puc_param);

    return ul_ret;
}


oal_uint32  hmac_config_query_tx_delay(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_TX_DELAY, us_len, puc_param);

    return ul_ret;
}
#endif
#endif


oal_uint32  hmac_config_query_rate_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_RATE, us_len, puc_param);

    return ul_ret;
}

#ifdef _PRE_WLAN_DFT_STAT

oal_uint32  hmac_config_query_ani_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_ANI, us_len, puc_param);

    return ul_ret;
}
#endif



oal_uint32  hmac_config_vap_classify_tid_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_vap_stru      *pst_hmac_vap;
    oal_int8            ac_string[OAM_PRINT_FORMAT_LENGTH];

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_vap_classify_tid_etc::mac_res_get_hmac_vap fail.vap_id = %u", pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (0xff == ul_val)
    {
        /* 打印当前的值 */
        OAL_SPRINTF(ac_string, sizeof(ac_string), "vap classify tid is %d\n",
                    mac_mib_get_VAPClassifyTidNo(pst_mac_vap));

        oam_print_etc(ac_string);

        return OAL_SUCC;
    }

    if (ul_val >= WLAN_TIDNO_BUTT)
    {
        /* 打印当前的值 */
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "val is invalid:%d, vap classify tid is %d", ul_val, mac_mib_get_VAPClassifyTidNo(pst_mac_vap));
        return OAL_SUCC;
    }

    mac_mib_set_VAPClassifyTidNo(pst_mac_vap, (oal_uint8)ul_val);

    return OAL_SUCC;
}


oal_uint32  hmac_atcmdsrv_fem_pa_response_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event      *pst_atcmdsrv_fem_pa_response_event;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_fem_pa_response_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_atcmdsrv_fem_pa_response_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if(OAL_ATCMDSRV_FEM_PA_INFO_EVENT == pst_atcmdsrv_fem_pa_response_event->uc_event_id)
    {
        pst_hmac_vap->st_atcmdsrv_get_status.ul_check_fem_pa_status = pst_atcmdsrv_fem_pa_response_event->ul_event_para;
    }
    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_check_fem_pa_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

   return OAL_SUCC;
}

oal_uint32  hmac_atcmdsrv_dbb_num_response_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event       *pst_atcmdsrv_dbb_num_response_event;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_dbb_num_response_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_atcmdsrv_dbb_num_response_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if(OAL_ATCMDSRV_DBB_NUM_INFO_EVENT == pst_atcmdsrv_dbb_num_response_event->uc_event_id)
    {
        pst_hmac_vap->st_atcmdsrv_get_status.ul_dbb_num = pst_atcmdsrv_dbb_num_response_event->ul_event_para;
    }
    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_dbb_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

   return OAL_SUCC;
}


oal_uint32  hmac_atcmdsrv_get_ant_response_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event       *pst_atcmdsrv_dbb_num_response_event;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_dbb_num_response_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_atcmdsrv_dbb_num_response_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if(OAL_ATCMDSRV_GET_ANT == pst_atcmdsrv_dbb_num_response_event->uc_event_id)
    {
        pst_hmac_vap->st_atcmdsrv_get_status.uc_ant_status = pst_atcmdsrv_dbb_num_response_event->ul_event_para;
    }
    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_ant_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

   return OAL_SUCC;
}

#ifdef _PRE_WLAN_WEB_CMD_COMM
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING

oal_uint32  hmac_config_query_bsd_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_query_bsd_stru  *pst_param;
    hmac_vap_stru           *pst_hmac_vap;

    pst_param = (mac_cfg_query_bsd_stru*)puc_param;
    pst_mac_vap->uc_bsd_switch = pst_param->uc_bsd;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_bsd_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_query_ko_version_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_KO_VERSION);
}


oal_uint32  hmac_config_query_bcast_rate_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_BCAST_RATE);
}


oal_uint32  hmac_config_query_pwr_ref_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_PWR_REF);
}
#endif

#ifdef _PRE_WLAN_FEATURE_SMARTANT
oal_uint32  hmac_atcmdsrv_get_ant_info_response(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_ant_info_response_event   *pst_atcmdsrv_ant_info;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_dbb_num_response_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_atcmdsrv_ant_info = (dmac_atcmdsrv_ant_info_response_event *)(puc_param);
    if(OAL_ATCMDSRV_GET_ANT_INFO == pst_atcmdsrv_ant_info->uc_event_id)
    {
        g_st_atcmdsrv_ant_info.uc_ant_type                   = pst_atcmdsrv_ant_info->uc_ant_type;
        g_st_atcmdsrv_ant_info.ul_last_ant_change_time_ms    = pst_atcmdsrv_ant_info->ul_last_ant_change_time_ms;
        g_st_atcmdsrv_ant_info.ul_ant_change_number          = pst_atcmdsrv_ant_info->ul_ant_change_number;
        g_st_atcmdsrv_ant_info.ul_main_ant_time_s            = pst_atcmdsrv_ant_info->ul_main_ant_time_s;
        g_st_atcmdsrv_ant_info.ul_aux_ant_time_s             = pst_atcmdsrv_ant_info->ul_aux_ant_time_s;
        g_st_atcmdsrv_ant_info.ul_total_time_s               = pst_atcmdsrv_ant_info->ul_total_time_s;
    }
    pst_hmac_vap->en_ant_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
   return OAL_SUCC;
}
oal_uint32  hmac_atcmdsrv_double_ant_switch_info_response(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_query_response_event               *pst_atcmdsrv_double_ant_switch;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_double_ant_switch_info_response::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    pst_atcmdsrv_double_ant_switch = (dmac_query_response_event *)(puc_param);
    if(OAL_ATCMDSRV_DOUBLE_ANT_SW == pst_atcmdsrv_double_ant_switch->query_event)
    {
        pst_hmac_vap->ul_double_ant_switch_ret = (oal_uint32)(oal_uint8)pst_atcmdsrv_double_ant_switch->reserve[0];
    }
    pst_hmac_vap->en_double_ant_switch_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
   return OAL_SUCC;
}
#endif

oal_uint32  hmac_atcmdsrv_get_rx_pkcg_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event       *pst_atcmdsrv_get_rx_pkcg_event;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_get_rx_pkcg_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_atcmdsrv_get_rx_pkcg_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if(OAL_ATCMDSRV_GET_RX_PKCG == pst_atcmdsrv_get_rx_pkcg_event->uc_event_id)
    {
        pst_hmac_vap->st_atcmdsrv_get_status.ul_rx_pkct_succ_num = pst_atcmdsrv_get_rx_pkcg_event->ul_event_para;
#if defined( _PRE_WLAN_WEB_CMD_COMM) && defined(_PRE_WLAN_11K_STAT)
        pst_hmac_vap->st_wme_stat.ul_rx_fail_num[OAL_WME_AC_BE]= pst_atcmdsrv_get_rx_pkcg_event->ul_fail_num;
#endif
        pst_hmac_vap->st_atcmdsrv_get_status.s_rx_rssi = pst_atcmdsrv_get_rx_pkcg_event->s_always_rx_rssi;
    }
    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

   return OAL_SUCC;
}
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
extern oal_uint8 g_uc_dev_lte_gpio_level_etc;
#endif
oal_uint32  hmac_atcmdsrv_lte_gpio_check_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    dmac_atcmdsrv_atcmd_response_event       *pst_atcmdsrv_lte_gpio_check_event;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_lte_gpio_check_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_atcmdsrv_lte_gpio_check_event = (dmac_atcmdsrv_atcmd_response_event *)(puc_param);
    if(OAL_ATCMDSRV_LTE_GPIO_CHECK == pst_atcmdsrv_lte_gpio_check_event->uc_event_id)
    {
        /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
        pst_hmac_vap->st_atcmdsrv_get_status.uc_lte_gpio_check_flag = OAL_TRUE;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        g_uc_dev_lte_gpio_level_etc = pst_atcmdsrv_lte_gpio_check_event->uc_reserved;
#endif
        OAL_WAIT_QUEUE_WAKE_UP(&(pst_hmac_vap->query_wait_q));
    }

    return OAL_SUCC;
}
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)

extern oal_uint16                      g_us_efuse_buffer_etc[32];


oal_uint32  hmac_atcmdsrv_report_efuse_reg_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    oal_uint16                               ul_loop = 0;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_lte_gpio_check_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    for(ul_loop = 0;ul_loop < 32;ul_loop++)
    {
        g_us_efuse_buffer_etc[ul_loop] = *(oal_uint16*)(puc_param);
        puc_param = puc_param + 2;
    }
    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_report_efuse_reg_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP(&(pst_hmac_vap->query_wait_q));


    return OAL_SUCC;
}
#endif
#endif
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

oal_uint32  hmac_atcmdsrv_report_reg(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_report_reg::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    //OAL_IO_PRINT("puc_param value 0x%08x\n", *(oal_uint32 *)puc_param);
    pst_hmac_vap->st_atcmdsrv_get_status.ul_reg_value = *(oal_uint32 *)puc_param;
    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_report_reg_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP(&(pst_hmac_vap->query_wait_q));


    return OAL_SUCC;
}
#endif

OAL_STATIC oal_uint32  hmac_config_d2h_user_info_syn(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_d2h_syn_info_stru   *pst_syn_info;
    mac_user_stru           *pst_mac_user;
    oal_uint8                uc_idx;
    oal_uint32               ul_ret;

    if ((MAC_VAP_INVAILD == pst_mac_vap->uc_init_flag)||(OAL_PTR_NULL == pst_mac_vap->pst_mib_info)||(OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG3(0, OAM_SF_CFG, "{hmac_config_d2h_user_info_syn::pst_mac_vap->uc_init_flag[%d], pst_mac_vap->pst_mib_info[%p], puc_param[%p]!}",
            pst_mac_vap->uc_init_flag, pst_mac_vap->pst_mib_info, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_syn_info = (mac_d2h_syn_info_stru *)puc_param;

    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user_etc(pst_syn_info->us_user_idx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_user_info_syn::pst_mac_user null.user idx [%d]}", pst_syn_info->us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 同步USR带宽 */
    mac_user_set_bandwidth_cap_etc(pst_mac_user, pst_syn_info->en_bandwidth_cap);
    mac_user_set_bandwidth_info_etc(pst_mac_user, pst_syn_info->en_avail_bandwidth, pst_syn_info->en_cur_bandwidth);

    /* 同步信道信息 */
    ul_ret = mac_get_channel_idx_from_num_etc(pst_mac_vap->st_channel.en_band,
                pst_syn_info->st_channel.uc_chan_number, &uc_idx);

    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_2040,
                "{hmac_d2h_user_info_syn::mac_get_channel_idx_from_num_etc failed[%d].}", ul_ret);

        return ul_ret;
    }

    pst_mac_vap->st_channel.uc_chan_number = pst_syn_info->st_channel.uc_chan_number;
    pst_mac_vap->st_channel.en_bandwidth   = pst_syn_info->st_channel.en_bandwidth;
    pst_mac_vap->st_channel.uc_chan_idx         = uc_idx;

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_d2h_user_info_syn::channe[%d],bw[%d]avail bw[%d],current[%d]!}",pst_mac_vap->st_channel.uc_chan_number,
                                                           pst_mac_vap->st_channel.en_bandwidth,pst_mac_user->en_avail_bandwidth,pst_mac_user->en_cur_bandwidth);

 #endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_d2h_vap_mib_update(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_d2h_mib_update_info_stru   *pst_mib_update_info;
    mac_device_stru                  *pst_mac_dev;

    if ((MAC_VAP_INVAILD == pst_mac_vap->uc_init_flag)||(OAL_PTR_NULL == pst_mac_vap->pst_mib_info)||(OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG3(0, OAM_SF_CFG, "{hmac_config_d2h_vap_mib_update::pst_mac_vap->uc_init_flag[%d], pst_mac_vap->pst_mib_info[%p], puc_param[%p]!}",
            pst_mac_vap->uc_init_flag, pst_mac_vap->pst_mib_info, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{dmac_config_vap_mib_update::pst_mac_dev[%d] null.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mib_update_info = (mac_d2h_mib_update_info_stru *)puc_param;
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
        "{hmac_config_d2h_vap_mib_update::en_11ax_cap=[%d],en_radar_cap=[%d],en_wlan_bw_max=[%d], beacon_period=[%d].}",
        pst_mib_update_info->en_11ax_cap, pst_mib_update_info->en_radar_detector_cap,
        pst_mib_update_info->en_wlan_bw_max, pst_mib_update_info->us_beacon_period);

    /*设置带宽mib能力*/
    mac_mib_set_dot11VapMaxBandWidth(pst_mac_vap,pst_mib_update_info->en_wlan_bw_max);
    /*更新VHT相关mib能力 */
    mac_mib_set_VHTChannelWidthOptionImplemented(pst_mac_vap, mac_device_trans_bandwith_to_vht_capinfo(mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap)));
    if (mac_mib_get_dot11VapMaxBandWidth(pst_mac_vap) >= WLAN_BW_CAP_160M)
    {
        mac_mib_set_VHTShortGIOptionIn160and80p80Implemented(pst_mac_vap, WLAN_HAL0_VHT_SGI_SUPP_160_80P80);
    }
    else
    {
        mac_mib_set_VHTShortGIOptionIn160and80p80Implemented(pst_mac_vap, OAL_FALSE);
    }

#ifdef _PRE_WLAN_FEATURE_11AX
    pst_mac_vap->en_11ax_hal_cap = pst_mib_update_info->en_11ax_cap;
    if(IS_11AX_VAP(pst_mac_vap))
    {
       mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_TRUE);
    }
    else
    {
        mac_mib_set_HEOptionImplemented(pst_mac_vap, OAL_FALSE);
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_d2h_vap_mib_update::en_11ax_hal=[%d],ax_custom=[%d],mib_11ax=[%d]}",
    pst_mac_vap->en_11ax_hal_cap,pst_mac_vap->en_11ax_custom_switch,mac_mib_get_HEOptionImplemented(pst_mac_vap));
#endif

#ifdef _PRE_WLAN_FEATURE_TXOPPS
    mac_mib_set_txopps(pst_mac_vap,pst_mib_update_info->en_txopps_is_supp);
#endif


#ifdef _PRE_WLAN_FEATURE_DFS
    MAC_VAP_GET_SUPPORT_DFS(pst_mac_vap) = pst_mib_update_info->en_radar_detector_cap;
#endif

    /*11n sounding mib设置*/
#if (defined(_PRE_WLAN_FEATURE_TXBF) && defined(_PRE_WLAN_FEATURE_TXBF_HT))
    if(OAL_TRUE == pst_mib_update_info->en_11n_sounding)
    {
        mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_SUBFER(pst_mac_dev));
        mac_mib_set_ReceiveStaggerSoundingOptionImplemented(pst_mac_vap, MAC_DEVICE_GET_CAP_SUBFEE(pst_mac_dev));
        mac_mib_set_ExplicitCompressedBeamformingFeedbackOptionImplemented(pst_mac_vap, WLAN_MIB_HT_ECBF_DELAYED);
        mac_mib_set_NumberCompressedBeamformingMatrixSupportAntenna(pst_mac_vap, HT_BFEE_NTX_SUPP_ANTA_NUM);

    }
    else
    {
        mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_ReceiveStaggerSoundingOptionImplemented(pst_mac_vap, OAL_FALSE);
        mac_mib_set_ExplicitCompressedBeamformingFeedbackOptionImplemented(pst_mac_vap, WLAN_MIB_HT_ECBF_INCAPABLE);
        mac_mib_set_NumberCompressedBeamformingMatrixSupportAntenna(pst_mac_vap, 1);

    }
#endif

    /*green filed mib设置*/
    /*暂时未使用，使用时打开*/
    //mac_mib_set_HTGreenfieldOptionImplemented(pst_mac_vap,pst_hal_device->st_cfg_cap_info.en_green_field);

#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_mib_set_VHTMUBeamformeeOptionImplemented(pst_mac_vap, pst_mib_update_info->en_mu_beamformee_cap);
#endif

    mac_mib_set_BeaconPeriod(pst_mac_vap, pst_mib_update_info->us_beacon_period);

    /* 设置su_bfee能力 */
    mac_mib_set_VHTBeamformeeNTxSupport(pst_mac_vap, pst_mib_update_info->uc_su_bfee_num);
    mac_mib_set_ShortGIOptionInFortyImplemented(pst_mac_vap, pst_mib_update_info->en_40m_shortgi);

    mac_mib_set_FortyMHzOperationImplemented(pst_mac_vap, pst_mib_update_info->en_40m_enable);

    /*设置11n txbf 的能力*/
    pst_mac_vap->st_cap_flag.bit_11ntxbf = pst_mib_update_info->en_11n_txbf;
#endif

    return OAL_SUCC;
}



OAL_STATIC oal_uint32  hmac_config_d2h_vap_cap_update(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_cap_flag_stru                 *pst_mac_cap_flag;

    pst_mac_cap_flag = (mac_cap_flag_stru *)(puc_param);
    pst_mac_vap->st_cap_flag.bit_1024qam = !!pst_mac_cap_flag->bit_1024qam;
    pst_mac_vap->st_cap_flag.bit_nb = pst_mac_cap_flag->bit_nb;

    /* 2g5g定制化能力刷新 */
    pst_mac_vap->st_cap_flag.bit_2g_custom_siso = pst_mac_cap_flag->bit_2g_custom_siso;
    pst_mac_vap->st_cap_flag.bit_5g_custom_siso = pst_mac_cap_flag->bit_5g_custom_siso;

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                        "{hmac_config_d2h_vap_cap_update::1024_cap=[%d],nb_cap=[%d],2g_custm_siso[%d]5g_custom_siso[%d].}",
                            pst_mac_vap->st_cap_flag.bit_1024qam, pst_mac_vap->st_cap_flag.bit_nb,
                            pst_mac_vap->st_cap_flag.bit_2g_custom_siso, pst_mac_vap->st_cap_flag.bit_5g_custom_siso);
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_d2h_vap_channel_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_channel_param_stru      *pst_channel_param;
    oal_uint32                       ul_ret;

    pst_channel_param = (mac_cfg_channel_param_stru *)puc_param;
    pst_mac_vap->st_channel.en_band = pst_channel_param->en_band;
    pst_mac_vap->st_channel.en_bandwidth = pst_channel_param->en_bandwidth;
    pst_mac_vap->st_channel.uc_chan_number = pst_channel_param->uc_channel;

    ul_ret = mac_get_channel_idx_from_num_etc(pst_channel_param->en_band, pst_channel_param->uc_channel, &pst_mac_vap->st_channel.uc_chan_idx);
    if(OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_d2h_vap_channel_info::mac_get_channel_idx_from_num_etc fail!");
    }

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_d2h_vap_channel_info::chan[%d] band[%d] bandwidth[%d]",
                            pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.en_bandwidth);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_config_get_dbdc_info(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru     *pst_mac_device;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if(OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_dbdc_info::device id [%d] is null", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device->en_dbdc_running = (oal_bool_enum_uint8)*puc_param;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_dbdc_info::dbdc state[%d]", pst_mac_device->en_dbdc_running);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_VOWIFI

oal_uint32  hmac_config_vowifi_report_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    frw_event_mem_stru              *pst_event_mem;
    frw_event_stru                  *pst_event;

    /* 目前仅Legacy sta支持这种操作 */
    if (OAL_PTR_NULL == pst_mac_vap->pst_vowifi_cfg_param)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_vowifi_report_etc::pst_vowifi_cfg_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设备up，切使能了vowifi状态才能触发切换vowifi状态 */
    if (VOWIFI_DISABLE_REPORT == pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_mode)
    {
        return OAL_SUCC;
    }

    /* "申请vowifi逻辑切换"仅上报一次直到重新更新vowifi模式 */
    if (OAL_TRUE == pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_reported)
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_vowifi_report_etc::vowifi been reported once!}");
        return OAL_SUCC;
    }

    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_vowifi_report_etc::FRW_EVENT_ALLOC fail,size=0!}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_VOWIFI_REPORT,
                       0,
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);
    pst_mac_vap->pst_vowifi_cfg_param->en_vowifi_reported = OAL_TRUE;
    return OAL_SUCC;

}
#endif /* _PRE_WLAN_FEATURE_VOWIFI */
#if defined(_PRE_WLAN_FEATURE_OPMODE_NOTIFY) || defined(_PRE_WLAN_FEATURE_SMPS)

oal_void hmac_show_m2s_sync_cap(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, mac_user_m2s_stru *pst_syn_info)
{
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "hmac_show_m2s_sync_cap::original user cap user_nss[%d] avail_nss[%d] bf_nss[%d] smps_mode[%d]",
                            pst_mac_user->en_user_num_spatial_stream, pst_mac_user->en_avail_num_spatial_stream, pst_mac_user->en_avail_bf_num_spatial_stream, pst_mac_user->st_ht_hdl.bit_sm_power_save);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "hmac_show_m2s_sync_cap::avail_bw[%d] cur_bw[%d]",
                            pst_mac_user->en_avail_bandwidth, pst_mac_user->en_cur_bandwidth);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "hmac_show_m2s_sync_cap::new user cap user_nss[%d] avail_nss[%d] bf_nss[%d] smps_mode[%d]",
                            pst_syn_info->en_user_num_spatial_stream, pst_syn_info->en_avail_num_spatial_stream, pst_syn_info->en_avail_bf_num_spatial_stream, pst_syn_info->en_cur_smps_mode);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "hmac_show_m2s_sync_cap::avail_bw[%d] cur_bw[%d]",
                            pst_syn_info->en_avail_bandwidth, pst_syn_info->en_cur_bandwidth);
}


oal_uint32  hmac_config_user_m2s_info_syn(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_user_m2s_stru    *pst_syn_info;
    mac_user_stru        *pst_mac_user;

    if ((MAC_VAP_INVAILD == pst_mac_vap->uc_init_flag)||(OAL_PTR_NULL == pst_mac_vap->pst_mib_info)||(OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG3(0, OAM_SF_M2S, "{hmac_config_user_m2s_info_syn::pst_mac_vap->uc_init_flag[%d], pst_mac_vap->pst_mib_info[%p], puc_param[%p]!}",
            pst_mac_vap->uc_init_flag, pst_mac_vap->pst_mib_info, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_syn_info = (mac_user_m2s_stru *)puc_param;

    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user_etc(pst_syn_info->us_user_idx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_user))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_config_user_m2s_info_syn::pst_mac_user null.user idx [%d]}", pst_syn_info->us_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 打印同步前后user的带宽空间流信息 */
    hmac_show_m2s_sync_cap(pst_mac_vap, pst_mac_user, pst_syn_info);

    /* 同步USR带宽 */
    mac_user_set_bandwidth_info_etc(pst_mac_user, pst_syn_info->en_avail_bandwidth, pst_syn_info->en_cur_bandwidth);

    /* 同步用户空间流 */
    mac_user_set_num_spatial_stream_etc(pst_mac_user, pst_syn_info->en_user_num_spatial_stream);
    mac_user_set_avail_num_spatial_stream_etc(pst_mac_user, pst_syn_info->en_avail_num_spatial_stream);
    mac_user_avail_bf_num_spatial_stream_etc(pst_mac_user, pst_syn_info->en_avail_bf_num_spatial_stream);
#ifdef _PRE_WLAN_FEATURE_SMPS
    mac_user_set_sm_power_save(pst_mac_user, pst_syn_info->en_cur_smps_mode);
#endif

#endif

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_M2S

oal_uint32 hmac_config_d2h_device_m2s_info_syn(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_m2s_stru          *pst_syn_info;
    mac_device_stru              *pst_mac_device;

    /* 该vap是配置vap，mib指针为空，此处不需要判断mib */
    if ((MAC_VAP_INVAILD == pst_mac_vap->uc_init_flag)||(OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG3(0, OAM_SF_M2S, "{hmac_config_device_m2s_info_syn::pst_mac_vap->uc_init_flag[%d], pst_mac_vap->pst_mib_info[%p], puc_param[%p]!}",
            pst_mac_vap->uc_init_flag, pst_mac_vap->pst_mib_info, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_syn_info = (mac_device_m2s_stru *)puc_param;

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG1(0, OAM_SF_M2S, "{hmac_config_device_m2s_info_syn: mac device is null ptr. device id:%d.}", pst_mac_vap->uc_device_id);
       return OAL_ERR_CODE_PTR_NULL;
    }

    /* 1.mac device的能力需要同步, 防止硬切换重启之后host没有恢复，其他能力TBD */
    MAC_DEVICE_GET_NSS_NUM(pst_mac_device) = pst_syn_info->en_nss_num;

    /* 2.smps能力需要刷新，防止硬切换时候，vap smps mib初始化不正常 */
    MAC_DEVICE_GET_MODE_SMPS(pst_mac_device) = pst_syn_info->en_smps_mode;
    /* 关键信息同步提示 */
    OAM_WARNING_LOG2(0, OAM_SF_M2S,
                       "{hmac_config_device_m2s_info_syn::en_nss_num:[%d] smps mode[%d].}",
                           pst_syn_info->en_nss_num, MAC_DEVICE_GET_MODE_SMPS(pst_mac_device));

    return OAL_SUCC;
}


oal_uint32  hmac_config_vap_m2s_info_syn(mac_vap_stru *pst_mac_vap)
{
    oal_uint32                  ul_ret;
    mac_vap_m2s_stru            st_m2s_vap_info;
    mac_device_stru            *pst_mac_dev;

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_config_vap_m2s_info_syn::pst_mac_dev[%d] null.}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(WLAN_BAND_2G == pst_mac_vap->st_channel.en_band)
    {
        mac_mib_set_VHTShortGIOptionIn160and80p80Implemented(pst_mac_vap, OAL_FALSE);

        /* 根据当前带宽刷新mib，5g默认160M支持 */
        mac_mib_set_VHTChannelWidthOptionImplemented(pst_mac_vap, WLAN_MIB_VHT_SUPP_WIDTH_80);
    }
    else/* 5G恢复成最大默认能力 */
    {
        if (MAC_DEVICE_GET_CAP_BW(pst_mac_dev) >= WLAN_BW_CAP_160M)
        {
            mac_mib_set_VHTShortGIOptionIn160and80p80Implemented(pst_mac_vap, WLAN_HAL0_VHT_SGI_SUPP_160_80P80);
        }
        else
        {
            mac_mib_set_VHTShortGIOptionIn160and80p80Implemented(pst_mac_vap, OAL_FALSE);
        }

        mac_mib_set_VHTChannelWidthOptionImplemented(pst_mac_vap, mac_device_trans_bandwith_to_vht_capinfo(MAC_DEVICE_GET_CAP_BW(pst_mac_dev)));
    }

    /* host暂时只是opmode的cap flag需要同步 TBD */
    st_m2s_vap_info.en_support_opmode = pst_mac_vap->st_cap_flag.bit_opmode;

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_VAP_M2S_INFO_SYN, OAL_SIZEOF(mac_vap_m2s_stru), (oal_uint8 *)(&st_m2s_vap_info));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_config_vap_m2s_info_syn::hmac_config_send_event_etc failed[%d],user_id[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_d2h_vap_m2s_info_syn(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_vap_m2s_stru             *pst_syn_info;
    mac_cfg_kick_user_param_stru  st_kick_user_param;
    oal_uint32                    ul_ret;
    oal_uint8                     auc_mac_addr[WLAN_MAC_ADDR_LEN] = {0xff,0xff,0xff,0xff,0xff,0xff};

    if ((MAC_VAP_INVAILD == pst_mac_vap->uc_init_flag)||(OAL_PTR_NULL == pst_mac_vap->pst_mib_info)||(OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG3(0, OAM_SF_M2S, "{hmac_config_d2h_vap_m2s_info_syn::pst_mac_vap->uc_init_flag[%d], pst_mac_vap->pst_mib_info[%p], puc_param[%p]!}",
            pst_mac_vap->uc_init_flag, pst_mac_vap->pst_mib_info, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_syn_info = (mac_vap_m2s_stru *)puc_param;

    /* 1.m2s vap信息同步hmac */
    mac_vap_set_rx_nss_etc(pst_mac_vap, pst_syn_info->en_vap_rx_nss);

    if(MAC_VAP_SPEC_IS_SW_NEED_M2S_SWITCH(pst_mac_vap))
    {
        mac_vap_init_rates_etc(pst_mac_vap);
    }

    /* 2. 修改HT能力 */
#ifdef _PRE_WLAN_FEATURE_SMPS
    mac_mib_set_smps(pst_mac_vap,  pst_syn_info->en_sm_power_save);
#endif
#ifdef _PRE_WLAN_FEATURE_TXBF_HT
    mac_mib_set_TransmitStaggerSoundingOptionImplemented(pst_mac_vap, pst_syn_info->en_transmit_stagger_sounding);
#endif
    mac_mib_set_TxSTBCOptionImplemented(pst_mac_vap, pst_syn_info->en_tx_stbc);

    mac_mib_set_vht_ctrl_field_cap(pst_mac_vap, pst_syn_info->en_vht_ctrl_field_supported);
#ifdef _PRE_WLAN_FEATURE_TXBF
    mac_mib_set_VHTTxSTBCOptionImplemented(pst_mac_vap, pst_syn_info->en_tx_vht_stbc_optionimplemented);
    mac_mib_set_VHTNumberSoundingDimensions(pst_mac_vap, pst_syn_info->en_vht_number_sounding_dimensions);
    mac_mib_set_VHTSUBeamformerOptionImplemented(pst_mac_vap, pst_syn_info->en_vht_su_beamformer_optionimplemented);
#endif

    if(WLAN_M2S_TYPE_HW == pst_syn_info->en_m2s_type)
    {
        /* 硬切换需要踢除所有主路上的用户 */
        st_kick_user_param.us_reason_code = MAC_UNSPEC_REASON;
        oal_set_mac_addr(st_kick_user_param.auc_mac_addr, auc_mac_addr);

        /*1. 踢掉该vap的用户 */
        ul_ret = hmac_config_kick_user_etc(pst_mac_vap, OAL_SIZEOF(oal_uint32), (oal_uint8 *)&st_kick_user_param);
        if(OAL_SUCC != ul_ret)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_config_d2h_vap_m2s_info_syn::hmac_config_kick_user_etc fail!}");
        }

        /* 2. 更新速率集，vap的空间流，暂时未看到其他需要更新的地方 */
        mac_vap_init_rates_etc(pst_mac_vap);
    }

    /* 关键信息同步提示 */
    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_M2S,
                       "{hmac_config_d2h_vap_m2s_info_syn::en_vap_rx_nss:[%d],en_sm_power_save:[%d],en_m2s_type:[%d].}",
                         pst_syn_info->en_vap_rx_nss, pst_syn_info->en_sm_power_save, pst_syn_info->en_m2s_type);

    return OAL_SUCC;
}


oal_uint32 hmac_m2s_switch_protect_comp_event_status(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_m2s_complete_syn_stru          *pst_m2s_switch_comp_status;
    frw_event_mem_stru                  *pst_event_mem;
    frw_event_stru                      *pst_event;
    oal_uint32                           ul_ret = OAL_SUCC;

    if ((MAC_VAP_INVAILD == pst_mac_vap->uc_init_flag)||(OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_M2S, "{hmac_m2s_switch_protect_comp_event_status::pst_mac_vap->uc_init_flag[%d], puc_param[%p]!}",
            pst_mac_vap->uc_init_flag, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_m2s_switch_comp_status = (dmac_m2s_complete_syn_stru *)puc_param;

    /* 对于驱动自身触发不用上报，对应于butt状态，触发arp probe即可 */
    if(MAC_M2S_COMMAND_MODE_BUTT == pst_m2s_switch_comp_status->uc_m2s_state)
    {
        hmac_m2s_vap_arp_probe_start(pst_mac_vap);
    }
    else
    {
        /* 抛扫描完成事件到WAL*/
        pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_crx_mgmt_tx_status_stru));
        if (OAL_PTR_NULL == pst_event_mem)
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_m2s_switch_protect_comp_event_status::pst_event_mem null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 填写事件 */
        pst_event = frw_get_event_stru(pst_event_mem);

        FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_HOST_CTX,
                           HMAC_HOST_CTX_EVENT_SUB_TYPE_M2S_STATUS,
                           OAL_SIZEOF(dmac_m2s_complete_syn_stru),
                           FRW_EVENT_PIPELINE_STAGE_0,
                           pst_mac_vap->uc_chip_id,
                           pst_mac_vap->uc_device_id,
                           pst_mac_vap->uc_vap_id);

        oal_memcopy((oal_uint8 *)frw_get_event_payload(pst_event_mem), (oal_uint8 *)pst_m2s_switch_comp_status, OAL_SIZEOF(dmac_m2s_complete_syn_stru));

        /* 分发事件 */
        ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
        if(OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_m2s_switch_protect_comp_event_status::frw_event_dispatch_event_etc fail.}");
        }

        FRW_EVENT_FREE(pst_event_mem);
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_m2s_switch_blacklist(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_M2S_BLACKLIST, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_config_set_m2s_switch_blacklist::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_m2s_switch_mss(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_M2S_MSS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_M2S, "{hmac_config_set_m2s_switch_blacklist::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif

#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH

oal_uint32 hmac_ant_tas_switch_rssi_notify_event_status(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    frw_event_mem_stru                  *pst_event_mem;
    frw_event_stru                      *pst_event;
    oal_uint32                           ul_ret;

    if ((MAC_VAP_INVAILD == pst_mac_vap->uc_init_flag)||(OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_ant_tas_switch_rssi_notify_event_status::pst_mac_vap->uc_init_flag[%d], puc_param[%p]!}",
                         pst_mac_vap->uc_init_flag, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

     /* 抛RSSI测量完成事件到WAL*/
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_crx_mgmt_tx_status_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_ant_tas_switch_rssi_notify_event_status::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_TAS_NOTIFY_RSSI,
                       OAL_SIZEOF(dmac_tas_rssi_notify_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    oal_memcopy((oal_uint8 *)frw_get_event_payload(pst_event_mem), puc_param, OAL_SIZEOF(dmac_tas_rssi_notify_stru));

    /* 分发事件 */
    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if(OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_ant_tas_switch_rssi_notify_event_status::frw_event_dispatch_event_etc fail.}");
    }

    FRW_EVENT_FREE(pst_event_mem);

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX

oal_uint32  hmac_config_stop_altx(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    pst_mac_vap->bit_al_tx_flag = OAL_SWITCH_OFF;

    return OAL_SUCC;
}

#endif


OAL_STATIC oal_uint32  hmac_config_query_sta_mngpkt_sendstat_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_device_stru                      *pst_mac_dev;
    hmac_vap_stru                        *pst_hmac_vap;
    mac_cfg_query_mngpkt_sendstat_stru   *pst_param;
    pst_param = (mac_cfg_query_mngpkt_sendstat_stru *)puc_param;

    pst_mac_dev     = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_query_sta_mngpkt_sendstat_rsp::mac_res_get_dev_etc failed.}");
        return OAL_FAIL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_sta_mngpkt_sendstat_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev->uc_auth_req_sendst = pst_param->uc_auth_req_st;
    pst_mac_dev->uc_asoc_req_sendst = pst_param->uc_asoc_req_st;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_query_rssi_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_query_rssi_stru     *pst_param;
    hmac_user_stru              *pst_hmac_user;
    hmac_vap_stru               *pst_hmac_vap;

    pst_param = (mac_cfg_query_rssi_stru *)puc_param;

    pst_hmac_user = mac_res_get_hmac_user_etc(pst_param->us_user_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_rssi_rsp: pst_hmac_user is null ptr. user id:%d", pst_param->us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_rssi_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user->c_rssi = pst_param->c_rssi;
    pst_hmac_user->c_free_power = pst_param->c_free_power;

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_query_psst_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_query_psst_stru     *pst_param;
    hmac_user_stru              *pst_hmac_user;
    hmac_vap_stru               *pst_hmac_vap;

    pst_param = (mac_cfg_query_psst_stru *)puc_param;

    pst_hmac_user = mac_res_get_hmac_user_etc(pst_param->us_user_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_psst_rsp: pst_hmac_user is null ptr. user id:%d", pst_param->us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_psst_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user->uc_ps_st = pst_param->uc_ps_st;

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
    return OAL_SUCC;
}

#ifdef _PRE_WLAN_WEB_CMD_COMM
#ifdef _PRE_WLAN_11K_STAT

OAL_STATIC oal_uint32  hmac_config_query_drop_num_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_query_drop_num_stru    *pst_param;
    hmac_user_stru                 *pst_hmac_user;
    hmac_vap_stru                  *pst_hmac_vap;
    oal_uint8                       uc_idx;

    pst_param = (mac_cfg_query_drop_num_stru *)puc_param;

    pst_hmac_user = mac_res_get_hmac_user_etc(pst_param->us_user_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_drop_num_rsp: pst_hmac_user is null ptr. user id:%d", pst_param->us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_drop_num_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    //pst_hmac_user->ul_drop_num = pst_param->ul_tx_dropped;
    for (uc_idx = 0; uc_idx < WLAN_WME_AC_BUTT; uc_idx++)
    {
        pst_hmac_user->aul_drop_num[uc_idx] = pst_param->aul_tx_dropped[uc_idx];
    }

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_config_query_tx_delay_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_query_tx_delay_stru     *pst_param;
    hmac_user_stru              *pst_hmac_user;
    hmac_vap_stru               *pst_hmac_vap;

    pst_param = (mac_cfg_query_tx_delay_stru *)puc_param;

    pst_hmac_user = mac_res_get_hmac_user_etc(pst_param->us_user_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_tx_delay_rsp: pst_hmac_user is null ptr. user id:%d", pst_param->us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_tx_delay_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user->aul_tx_delay[0] = pst_param->ul_max_tx_delay;
    pst_hmac_user->aul_tx_delay[1] = pst_param->ul_min_tx_delay;
    pst_hmac_user->aul_tx_delay[2] = pst_param->ul_ave_tx_delay;

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));
    return OAL_SUCC;
}
#endif
#endif


OAL_STATIC oal_uint32  hmac_config_query_rate_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_query_rate_stru     *pst_param;
    hmac_user_stru              *pst_hmac_user;
    hmac_vap_stru               *pst_hmac_vap;

    pst_param = (mac_cfg_query_rate_stru *)puc_param;
    pst_hmac_user = mac_res_get_hmac_user_etc(pst_param->us_user_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_rate_rsp: pst_hmac_user is null ptr. user id:%d", pst_param->us_user_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_rate_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user->ul_tx_rate       = pst_param->ul_tx_rate;
    pst_hmac_user->ul_tx_rate_min   = pst_param->ul_tx_rate_min;
    pst_hmac_user->ul_tx_rate_max   = pst_param->ul_tx_rate_max;
    pst_hmac_user->ul_rx_rate       = pst_param->ul_rx_rate;
    pst_hmac_user->ul_rx_rate_min   = pst_param->ul_rx_rate_min;
    pst_hmac_user->ul_rx_rate_max   = pst_param->ul_rx_rate_max;
#ifdef _PRE_WLAN_DFT_STAT
    pst_hmac_user->uc_cur_per       = pst_param->uc_cur_per;
    pst_hmac_user->uc_bestrate_per  = pst_param->uc_bestrate_per;
#endif

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_DFT_STAT

OAL_STATIC oal_uint32  hmac_config_query_ani_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    mac_cfg_query_ani_stru      *pst_param;
    hmac_vap_stru               *pst_hmac_vap;

    pst_param = (mac_cfg_query_ani_stru *)puc_param;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_query_ani_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap->uc_device_distance    = pst_param->uc_device_distance;
    pst_hmac_vap->uc_intf_state_cca     = pst_param->uc_intf_state_cca;
    pst_hmac_vap->uc_intf_state_co      = pst_param->uc_intf_state_co;

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}

#endif

/*****************************************************************************
    g_ast_hmac_config_syn: dmac向hmac同步控制面数据处理函数表
*****************************************************************************/
OAL_STATIC OAL_CONST hmac_config_syn_stru g_ast_hmac_config_syn[] =
{
    /* 同步ID                    保留2个字节            函数操作 */
    {WLAN_CFGID_QUERY_STATION_STATS,    {0, 0}, hmac_proc_query_response_event_etc},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_RESET_HW_OPERATE,       {0, 0}, hmac_reset_sys_event_etc},
    {WLAN_CFGID_THRUPUT_INFO,           {0, 0}, hmac_get_thruput_info_etc},
#ifdef _PRE_WLAN_FEATURE_BTCOEX
    {WLAN_CFGID_BTCOEX_RX_DELBA_TRIGGER,{0, 0}, hmac_btcoex_rx_delba_trigger_etc},
#endif
#endif
    {WLAN_CFGID_QUERY_RSSI,             {0, 0}, hmac_config_query_rssi_rsp},
    {WLAN_CFGID_QUERY_PSST,             {0, 0}, hmac_config_query_psst_rsp},
#ifdef _PRE_WLAN_WEB_CMD_COMM
#ifdef _PRE_WLAN_11K_STAT
    {WLAN_CFGID_QUERY_DROP_NUM,         {0, 0}, hmac_config_query_drop_num_rsp},
    {WLAN_CFGID_QUERY_TX_DELAY,         {0, 0}, hmac_config_query_tx_delay_rsp},
#endif
#endif
    {WLAN_CFGID_QUERY_RATE,             {0, 0}, hmac_config_query_rate_rsp},
#ifdef _PRE_WLAN_DFT_STAT
    {WLAN_CFGID_QUERY_ANI,              {0, 0}, hmac_config_query_ani_rsp},
#endif

    {WLAN_CFGID_CHECK_FEM_PA,           {0, 0}, hmac_atcmdsrv_fem_pa_response_etc},
    {WLAN_CFGID_GET_VERSION,            {0, 0}, hmac_atcmdsrv_dbb_num_response_etc},
    {WLAN_CFGID_GET_ANT,                {0, 0}, hmac_atcmdsrv_get_ant_response_etc},
#ifdef _PRE_WLAN_WEB_CMD_COMM
#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
    {WLAN_CFGID_GET_BSD,                {0, 0}, hmac_config_query_bsd_rsp},
#endif
    {WLAN_CFGID_GET_KO_VERSION,         {0, 0}, hmac_config_query_ko_version_rsp},
    {WLAN_CFGID_GET_PWR_REF,            {0, 0}, hmac_config_query_pwr_ref_rsp},
    {WLAN_CFGID_GET_BCAST_RATE,         {0, 0}, hmac_config_query_bcast_rate_rsp},
#endif
#ifdef _PRE_WLAN_FEATURE_SMARTANT
    {WLAN_CFGID_GET_ANT_INFO,           {0, 0}, hmac_atcmdsrv_get_ant_info_response},
    {WLAN_CFGID_DOUBLE_ANT_SW,          {0, 0}, hmac_atcmdsrv_double_ant_switch_info_response},
#endif
    {WLAN_CFGID_RX_FCS_INFO,            {0, 0}, hmac_atcmdsrv_get_rx_pkcg_etc},
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
    {WLAN_CFGID_CALI_POWER,             {0, 0}, hmac_config_get_polynomial_params},
    {WLAN_CFGID_GET_CALI_POWER,         {0, 0}, hmac_config_get_cali_power_rsp},
    {WLAN_CFGID_GET_UPC_PARA,           {0, 0}, hmac_config_get_upc_params_rsp},
#endif
    {WLAN_CFGID_GET_DIEID,              {0, 0}, hmac_config_get_dieid_rsp},
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    {WLAN_CFGID_CHECK_LTE_GPIO,         {0, 0}, hmac_atcmdsrv_lte_gpio_check_etc},
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    {WLAN_CFGID_REG_INFO,               {0, 0}, hmac_atcmdsrv_report_efuse_reg_etc},
#endif
#else
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
    {WLAN_CFGID_REG_INFO,               {0, 0}, hmac_atcmdsrv_report_reg},
#endif
#endif
//    {WLAN_CFGID_ADD_VAP,                   {0, 0},         hmac_add_vap_sysnc},
    {WLAN_CFGID_CFG80211_MGMT_TX_STATUS,   {0, 0},         hmac_mgmt_tx_event_status_etc},
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
    {WLAN_CFGID_CHIP_CHECK_SWITCH,         {0, 0},         hmac_hipriv_proc_write_process_rsp},
#endif
    {WLAN_CFGID_USR_INFO_SYN,              {0, 0},         hmac_config_d2h_user_info_syn},
    {WLAN_CFGID_VAP_MIB_UPDATE,            {0, 0},         hmac_config_d2h_vap_mib_update},
    {WLAN_CFGID_VAP_CAP_UPDATE,            {0, 0},         hmac_config_d2h_vap_cap_update},
    {WLAN_CFGID_VAP_CHANNEL_INFO_SYN,      {0, 0},         hmac_config_d2h_vap_channel_info},
    {WLAN_CFGID_GET_MNGPKT_SENDSTAT,       {0, 0},         hmac_config_query_sta_mngpkt_sendstat_rsp},

#if defined(_PRE_WLAN_FEATURE_OPMODE_NOTIFY) || defined(_PRE_WLAN_FEATURE_SMPS)
    {WLAN_CFGID_USER_M2S_INFO_SYN,             {0, 0},     hmac_config_user_m2s_info_syn},
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
    {WLAN_CFGID_DEVICE_M2S_INFO_SYN,    {0, 0},            hmac_config_d2h_device_m2s_info_syn},
    {WLAN_CFGID_VAP_M2S_INFO_SYN,       {0, 0},            hmac_config_d2h_vap_m2s_info_syn},
    {WLAN_CFGID_M2S_SWITCH_COMP,        {0, 0},            hmac_m2s_switch_protect_comp_event_status},
#endif
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
    {WLAN_CFGID_ANT_TAS_SWITCH_RSSI_NOTIFY, {0, 0},        hmac_ant_tas_switch_rssi_notify_event_status},
#endif
#ifdef _PRE_WLAN_FEATURE_11K
    {WLAN_CFGID_REQ_SAVE_BSS_INFO,          {0, 0},        hmac_scan_rrm_proc_save_bss_etc},
#endif

#ifdef _PRE_WLAN_FEATURE_VOWIFI
    {WLAN_CFGID_VOWIFI_REPORT,               {0, 0},         hmac_config_vowifi_report_etc},
#endif

#ifdef _PRE_WLAN_FEATURE_ALWAYS_TX
    {HAL_TX_COMP_SUB_TYPE_AL_TX, {0, 0},                   hmac_config_stop_altx},
#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
    {WLAN_CFGID_GET_TEMP,               {0, 0},         hmac_config_get_temp_rsp},
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
    {WLAN_CFGID_GET_MEAS_START_TSF,          {0, 0},       hmac_rrm_get_meas_start_time},
#endif

#ifdef _PRE_WLAN_WEB_CMD_COMM
    {WLAN_CFGID_GET_HW_FLOW_STAT,              {0, 0},         hmac_config_get_hw_flow_stat_rsp},
    {WLAN_CFGID_GET_WME_STAT,              {0, 0},         hmac_config_get_wme_stat_rsp},
    {WLAN_CFGID_GET_ANT_RSSI,              {0, 0},         hmac_config_get_ant_rssi_rsp},
    {WLAN_CFGID_GET_BG_NOISE,              {0, 0},         hmac_config_get_bg_noise_rsp},
#ifdef _PRE_WLAN_11K_STAT
    {WLAN_CFGID_GET_TX_DELAY_AC,              {0, 0},         hmac_config_get_tx_delay_ac_rsp},
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    {WLAN_CFGID_GET_STA_DIAG_INFO_RSP,              {0, 0},         hmac_config_get_sta_diag_info_rsp},
    {WLAN_CFGID_GET_VAP_DIAG_INFO_RSP,              {0, 0},         hmac_config_get_vap_diag_info_rsp},
    {WLAN_CFGID_GET_SENSING_BSSID_INFO,             {0, 0},         hmac_config_get_sensing_bssid_info_rsp},
#endif
#endif
#ifdef _PRE_FEATURE_FAST_AGING
    {WLAN_CFGID_GET_FAST_AGING,                     {0, 0},         hmac_config_get_fast_aging_rsp},
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_TEMP_PROTECT
    {WLAN_CFGID_NOTIFY_STA_RSSI,                     {0, 0},         hmac_config_receive_all_sta_rssi},
#endif
#ifdef _PRE_FEATURE_WAVEAPP_CLASSIFY
    {WLAN_CFGID_GET_WAVEAPP_FLAG,              {0, 0},         hmac_config_get_waveapp_flag_rsp},
#endif
#ifdef _PRE_WLAN_FEATURE_DBDC
    {WLAN_CFGID_GET_DBDC_INFO,             {0, 0},             hmac_config_get_dbdc_info},
#endif
    {WLAN_CFGID_BUTT,                      {0, 0},         OAL_PTR_NULL},
};


oal_uint32  hmac_event_config_syn_etc(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru             *pst_event;
    frw_event_hdr_stru         *pst_event_hdr;
    dmac_to_hmac_cfg_msg_stru  *pst_dmac2hmac_msg;
    mac_vap_stru               *pst_mac_vap;
    mac_device_stru            *pst_mac_device;
    oal_uint32                  ul_ret;
    oal_uint16                  us_cfgid;


    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_event_config_syn_etc::pst_event_mem null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取事件 */
    pst_event         = frw_get_event_stru(pst_event_mem);
    pst_event_hdr     = &(pst_event->st_event_hdr);
    pst_dmac2hmac_msg = (dmac_to_hmac_cfg_msg_stru *)pst_event->auc_event_data;

    OAM_INFO_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_CFG, "{hmac_event_config_syn_etc::a dmac config syn event occur, cfg_id=%d.}", pst_dmac2hmac_msg->en_syn_id);
    /* 获取dmac vap */
    pst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_event_hdr->uc_vap_id);

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_CFG, "{hmac_event_config_syn_etc::pst_mac_vap null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac device */
    pst_mac_device = (mac_device_stru *)mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_event_hdr->uc_vap_id, OAM_SF_CFG, "{hmac_event_config_syn_etc::pst_mac_device null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获得cfg id对应的操作函数 */
    for (us_cfgid = 0; WLAN_CFGID_BUTT != g_ast_hmac_config_syn[us_cfgid].en_cfgid; us_cfgid++)
    {
        if (g_ast_hmac_config_syn[us_cfgid].en_cfgid == pst_dmac2hmac_msg->en_syn_id)
        {
            break;
        }
    }

    /* 异常情况，cfgid在g_ast_dmac_config_syn中不存在 */
    if (WLAN_CFGID_BUTT == g_ast_hmac_config_syn[us_cfgid].en_cfgid)
    {
        OAM_WARNING_LOG1(pst_event_hdr->uc_vap_id, OAM_SF_CFG, "{hmac_event_config_syn_etc::invalid en_cfgid[%d].", pst_dmac2hmac_msg->en_syn_id);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /* 执行操作函数 */
    ul_ret = g_ast_hmac_config_syn[us_cfgid].p_set_func(pst_mac_vap, (oal_uint8)(pst_dmac2hmac_msg->us_len), (oal_uint8 *)pst_dmac2hmac_msg->auc_msg_body);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG2(pst_event_hdr->uc_vap_id, OAM_SF_CFG,
                         "{hmac_event_config_syn_etc::p_set_func failed, ul_ret=%d en_syn_id=%d.", ul_ret, pst_dmac2hmac_msg->en_syn_id);
        return ul_ret;
    }

    return OAL_SUCC;
}

#ifdef _PRE_DEBUG_MODE

oal_uint32  hmac_config_scan_test(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_scan_test(pst_mac_vap, us_len, puc_param);

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_bgscan_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_bgscan_enable_etc(pst_mac_vap, us_len, puc_param);
}

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_WLAN_CHIP_TEST_ALG)

OAL_STATIC oal_ssize_t hmac_alg_test_result_proc_read(oal_device_stru *dev, oal_device_attribute_stru *attr, char *buf)
{
#define ALG_READ_STR  "test read"
    oal_uint32                 ul_ret;
    mac_ioctl_alg_config_stru *pst_config;
    hmac_vap_stru             *pst_vap = mac_res_get_hmac_vap(1);
    oal_uint8                  auc_param[128];
    oal_uint16                 us_config_len = (oal_uint16)(OAL_SIZEOF(mac_ioctl_alg_config_stru) + OAL_STRLEN(ALG_READ_STR) + 1);

    pst_config = (mac_ioctl_alg_config_stru *)auc_param;
    if (OAL_PTR_NULL == pst_vap)
    {
        return 0;
    }
    oal_memset(pst_config, 0, 128);
    pst_config->uc_argc = 2;
    pst_config->auc_argv_offset[0] = 0;
    pst_config->auc_argv_offset[1] = 5;
    OAL_SPRINTF((oal_int8 *)(auc_param + OAL_SIZEOF(mac_ioctl_alg_config_stru)), 128, ALG_READ_STR);

    ul_ret = hmac_config_alg_etc(&pst_vap->st_vap_base_info, us_config_len, auc_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_DBAC, "{hmac_alg_test_result_proc_read:: failed[%d].}", ul_ret);
        return 0;
    }

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /* wait dmac result */
    OAL_INTERRUPTIBLE_SLEEP_ON(&g_st_alg_test_hmac.st_wait_queue);
#endif

    /* DMAC返回的结果中，指示出错，为保证ACT正常运行，返回一个E */
    if (g_st_alg_test_hmac.auc_data[0] == 0xFF)
    {
        g_st_alg_test_hmac.auc_data[0] = 1;
        g_st_alg_test_hmac.auc_data[1] = 'E';
    }

    oal_memcopy(buf, (oal_void *)(g_st_alg_test_hmac.auc_data + 1), g_st_alg_test_hmac.auc_data[0]);

    OAL_IO_PRINT("DEBUG:: sysfs return len: %d\r\n", g_st_alg_test_hmac.auc_data[0]);

    return g_st_alg_test_hmac.auc_data[0];
}

oal_uint32  hmac_alg_test_result_process(frw_event_mem_stru  *pst_event_mem)
{
    frw_event_stru      *pst_event;
    hmac_vap_stru       *pst_hmac_vap;

    pst_event = frw_get_event_stru(pst_event_mem);

    pst_hmac_vap = mac_res_get_hmac_vap(pst_event->st_event_hdr.uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_event->st_event_hdr.uc_vap_id, OAM_SF_ANY, "{hmac_alg_test_result_process::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_memcopy((oal_void *)g_st_alg_test_hmac.auc_data, (const oal_void *)pst_event->auc_event_data, HMAC_ALG_TEST_BUF_SIZE);

    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&g_st_alg_test_hmac.st_wait_queue);

   return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_alg_test_result_create_proc(oal_void *p_proc_arg)
{
    /* hi1102-cb add sys for 51/02 */
    oal_int32           l_ret = OAL_SUCC;

    if(OAL_PTR_NULL == g_alg_test_sys_kobject)
    {
        g_alg_test_sys_kobject = kobject_create_and_add("alg", OAL_PTR_NULL);
        l_ret = sysfs_create_file(g_alg_test_sys_kobject,&dev_attr_alg_test_result.attr);
    }

    return l_ret;
}


OAL_STATIC oal_uint32 hmac_alg_test_result_delete_proc(void)
{
    if(OAL_PTR_NULL != g_alg_test_sys_kobject)
    {
        sysfs_remove_file(g_alg_test_sys_kobject,&dev_attr_alg_test_result.attr);
        kobject_del(g_alg_test_sys_kobject);
        g_alg_test_sys_kobject = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_int32  hmac_alg_test_main_common_init(oal_void)
{
    oal_uint32  ul_ret;

    oal_memset(&g_st_alg_test_hmac, 0, OAL_SIZEOF(g_st_alg_test_hmac));

    ul_ret = hmac_alg_test_result_create_proc(OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{alg_test_main_common_init::hmac_alg_test_result_create_proc fail}");
        return (oal_int32)ul_ret;
    }

    OAL_WAIT_QUEUE_INIT_HEAD(&g_st_alg_test_hmac.st_wait_queue);

    return 0;
}

oal_int32  hmac_alg_test_main_common_exit(oal_void)
{
    oal_uint32  ul_ret;

    ul_ret = hmac_alg_test_result_delete_proc();
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{alg_test_main_common_exit::alg_test_result_delete_proc fail}");
        return -1;
    }
    return 0;
}
#endif

#ifdef _PRE_WLAN_FEATURE_STA_UAPSD

oal_uint32  hmac_config_set_uapsd_para_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_uapsd_sta_stru *pst_uapsd_param;
    oal_uint32              ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
       OAM_ERROR_LOG2(0, OAM_SF_PWR, "{hmac_config_set_uapsd_para_etc:: pst_mac_vap/puc_param is null ptr %d, %d!}\r\n", pst_mac_vap, puc_param);
       return OAL_ERR_CODE_PTR_NULL;
    }
    pst_uapsd_param = (mac_cfg_uapsd_sta_stru *)puc_param;

    /* uc_max_sp_len */
    if (pst_uapsd_param->uc_max_sp_len > 6)
    {
       OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{hmac_config_set_uapsd_para_etc::uc_max_sp_len[%d] > 6!}\r\n", pst_uapsd_param->uc_max_sp_len);
       return OAL_FAIL;
    }

#ifdef _PRE_WLAN_FEATURE_STA_PM
    mac_vap_set_uapsd_para_etc(pst_mac_vap, pst_uapsd_param);
#endif

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_UAPSD_PARA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_uapsd_para_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif
#ifdef _PRE_WLAN_FEATURE_STA_PM

oal_uint32 hmac_config_set_sta_pm_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_sync_cmd_common_etc(pst_mac_vap, WLAN_CFGID_SET_PS_MODE, us_len, puc_param);
}

oal_uint32  hmac_config_set_sta_pm_on_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                   ul_ret;
    mac_cfg_ps_mode_param_stru   st_ps_mode_param;
    hmac_vap_stru                *pst_hmac_vap;
    mac_cfg_ps_open_stru         *pst_sta_pm_open = (mac_cfg_ps_open_stru *)puc_param;

    pst_hmac_vap    = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_PWR, "{hmac_config_set_sta_pm_on_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 切换到手动设置为pspoll模式 */
    if (MAC_STA_PM_MANUAL_MODE_ON == pst_sta_pm_open->uc_pm_enable)
    {
        pst_hmac_vap->uc_cfg_sta_pm_manual = OAL_TRUE;
    }
    /* 关闭手动设置pspoll模式,回到fastps模式 */
    else if (MAC_STA_PM_MANUAL_MODE_OFF == pst_sta_pm_open->uc_pm_enable)
    {
        pst_hmac_vap->uc_cfg_sta_pm_manual = 0xFF;
    }

    pst_sta_pm_open->uc_pm_enable = (pst_sta_pm_open->uc_pm_enable > MAC_STA_PM_SWITCH_OFF) ? MAC_STA_PM_SWITCH_ON : MAC_STA_PM_SWITCH_OFF;

    st_ps_mode_param.uc_vap_ps_mode = pst_sta_pm_open->uc_pm_enable ?
                  ((pst_hmac_vap->uc_cfg_sta_pm_manual != 0xFF) ? MIN_PSPOLL_PS : pst_hmac_vap->uc_ps_mode)
                  : NO_POWERSAVE;

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id,OAM_SF_PWR,"hmac_config_set_sta_pm_on_etc,enable[%d], ps_mode[%d]",pst_sta_pm_open->uc_pm_enable,st_ps_mode_param.uc_vap_ps_mode);
    /* 先下发设置低功耗模式 */
    ul_ret = hmac_config_set_sta_pm_mode_etc(pst_mac_vap,OAL_SIZEOF(st_ps_mode_param),(oal_uint8 *)&st_ps_mode_param);
    if (ul_ret != OAL_SUCC)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id,OAM_SF_PWR,"sta_pm sta_pm mode[%d]fail",ul_ret);
        return ul_ret;
    }

    /* 再下发打开低功耗 */
    return hmac_config_sync_cmd_common_etc(pst_mac_vap, WLAN_CFGID_SET_STA_PM_ON, us_len, puc_param);

}
#endif

#ifdef _PRE_WLAN_CHIP_TEST

oal_uint32 hmac_test_send_action(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    mac_cfg_send_action_param_stru  *pst_action_param;
    oal_uint32                       ul_ret;
    hmac_user_stru                  *pst_hmac_user   = OAL_PTR_NULL;
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
    oal_bool_enum_uint8              en_is_protected = OAL_FALSE;
#endif

    if ( OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hmac_test_send_action:: pointer is null: pst_mac_vap[%d],puc_param[%d]", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_action_param = (mac_cfg_send_action_param_stru *)puc_param;
    if (OAL_TRUE != ETHER_IS_MULTICAST(pst_action_param->auc_mac_da))
    {
        pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_action_param->auc_mac_da);
        if (OAL_PTR_NULL == pst_hmac_user)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_test_send_action::pst_hmac_user null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        if (MAC_USER_STATE_ASSOC != pst_hmac_user->st_user_base_info.en_user_asoc_state)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_test_send_action::the user is unassociated.}");
            return OAL_FAIL;
        }
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
        en_is_protected = pst_hmac_user->st_user_base_info.st_cap_info.bit_pmf_active;
#endif
    }

    /* Category */
    switch (pst_action_param->uc_category)
    {
        case MAC_ACTION_CATEGORY_SA_QUERY:
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
            {
                if (OAL_TRUE == ETHER_IS_MULTICAST(pst_action_param->auc_mac_da))
                {
                    break;
                }
                OAM_INFO_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "hmac_test_send_action:: now start to send SA Query Request!");
                ul_ret = hmac_start_sa_query_etc(pst_mac_vap, pst_hmac_user, en_is_protected);
                if (OAL_SUCC != ul_ret)
                {
                    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_start_sa_query_etc::hmac_start_sa_query_etc failed[%d].}",ul_ret);
                    return ul_ret;
                }

            }
#endif
            break;
        default:
            break;
    }
    return OAL_SUCC;
}


oal_uint32  hmac_config_send_pspoll(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;


    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SEND_PSPOLL, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_pspoll::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_send_nulldata(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SEND_NULLDATA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_nulldata::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_clear_all_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CLEAR_ALL_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_clear_all_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif /* #ifdef _PRE_WLAN_CHIP_TEST */

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_get_thruput_info_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    dmac_thruput_info_sync_stru  *pst_thruput_info;

    pst_thruput_info = (dmac_thruput_info_sync_stru *)puc_param;

    if (OAL_PTR_NULL != pst_thruput_info)
    {
        OAL_IO_PRINT("interval cycles: %u \n",          pst_thruput_info->ul_cycles);
        OAL_IO_PRINT("sw tx succ num: %u \n",           pst_thruput_info->ul_sw_tx_succ_num);
        OAL_IO_PRINT("sw tx fail num: %u \n",           pst_thruput_info->ul_sw_tx_fail_num);
        OAL_IO_PRINT("sw rx ampdu succ num: %u \n",     pst_thruput_info->ul_sw_rx_ampdu_succ_num);
        OAL_IO_PRINT("sw rx mpdu succ num: %u \n",      pst_thruput_info->ul_sw_rx_mpdu_succ_num);
        OAL_IO_PRINT("sw rx fail num: %u \n",           pst_thruput_info->ul_sw_rx_ppdu_fail_num);
        OAL_IO_PRINT("hw rx ampdu fcs fail num: %u \n", pst_thruput_info->ul_hw_rx_ampdu_fcs_fail_num);
        OAL_IO_PRINT("hw rx mpdu fcs fail num: %u \n",  pst_thruput_info->ul_hw_rx_mpdu_fcs_fail_num);
        return OAL_SUCC;
    }
    else
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_get_thruput_info_etc::pst_thruput_info null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
}
#endif

#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)

oal_uint32 hmac_enable_pmf_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    oal_bool_enum_uint8        en_pmf_active;
    wlan_pmf_cap_status_uint8 *puc_pmf_cap;
    oal_dlist_head_stru       *pst_entry;
    mac_user_stru             *pst_user_tmp;

    OAL_IO_PRINT("hmac_enable_pmf_etc: func start!");
    if ( OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hmac_enable_pmf_etc:: pointer is null: pst_mac_vap[%d],puc_param[%d]", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    puc_pmf_cap = (wlan_pmf_cap_status_uint8 *)puc_param;

    switch(*puc_pmf_cap)
    {
        case MAC_PMF_DISABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_FALSE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
            mac_mib_set_rsnaactivated(pst_mac_vap, OAL_FALSE);
            en_pmf_active = OAL_FALSE;
        }
        break;
        case MAC_PMF_ENABLED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_FALSE);
            mac_mib_set_rsnaactivated(pst_mac_vap, OAL_TRUE);
            return OAL_SUCC;
        }
        case MAC_PMF_REQUIRED:
        {
            mac_mib_set_dot11RSNAMFPC(pst_mac_vap, OAL_TRUE);
            mac_mib_set_dot11RSNAMFPR(pst_mac_vap, OAL_TRUE);
            mac_mib_set_rsnaactivated(pst_mac_vap, OAL_TRUE);
            en_pmf_active = OAL_TRUE;
        }
        break;
        default:
        {
            OAL_IO_PRINT("hmac_enable_pmf_etc: commend error!");
            return OAL_FALSE;
        }
    }

    if (MAC_VAP_STATE_UP == pst_mac_vap->en_vap_state)
    {
        OAL_DLIST_SEARCH_FOR_EACH(pst_entry, &(pst_mac_vap->st_mac_user_list_head))
        {
            pst_user_tmp      = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
            if (OAL_PTR_NULL == pst_user_tmp)
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_enable_pmf_etc:: pst_user_tmp is null");
                return OAL_ERR_CODE_PTR_NULL;
            }

            mac_user_set_pmf_active_etc(pst_user_tmp, en_pmf_active);
        }
    }

    OAL_IO_PRINT("hmac_enable_pmf_etc: func end!");

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_HS20

oal_uint32  hmac_config_set_qos_map(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                      uc_idx;
    hmac_cfg_qos_map_param_stru   *pst_qos_map;
    hmac_vap_stru  *pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if ( OAL_PTR_NULL == pst_hmac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "hmac_config_set_qos_map:: pointer is null: pst_hmac_vap[%d],puc_param[%d]", pst_hmac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_qos_map = (hmac_cfg_qos_map_param_stru *)puc_param;
    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_qos_map:uc_num_dscp_except=%d, uc_valid=%d\r\n}",
                  pst_qos_map->uc_num_dscp_except,
                  pst_qos_map->uc_valid);

    /* 判断QOS MAP SET的使能开关是否打开 */
    if (!pst_qos_map->uc_valid)
    {
        return OAL_FAIL;
    }

    /* 检查下发的QoS Map Set参数中的DSCP Exception fields 是否超过最大数目21 */
    if (pst_qos_map->uc_num_dscp_except > MAX_DSCP_EXCEPT)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_qos_map:: input exceeds maximum : pst_qos_map->num_dscp_except[%d]",
                       pst_qos_map->uc_num_dscp_except);
        return OAL_FAIL;
    }
    /* 判断DSCP Exception fields是否为空 */
    if ((pst_qos_map->uc_num_dscp_except != 0))
    {
        pst_hmac_vap->st_cfg_qos_map_param.uc_num_dscp_except = pst_qos_map->uc_num_dscp_except;
        for(uc_idx = 0; uc_idx < pst_qos_map->uc_num_dscp_except; uc_idx++)
        {
            pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception[uc_idx] = pst_qos_map->auc_dscp_exception[uc_idx];
            pst_hmac_vap->st_cfg_qos_map_param.auc_dscp_exception_up[uc_idx] = pst_qos_map->auc_dscp_exception_up[uc_idx];
        }
    }

    /* 配置DSCP Exception format中的User Priority的HIGHT和LOW VALUE值 */
    for (uc_idx = 0; uc_idx < MAX_QOS_UP_RANGE; uc_idx++)
    {
        pst_hmac_vap->st_cfg_qos_map_param.auc_up_high[uc_idx] = pst_qos_map->auc_up_high[uc_idx];
        pst_hmac_vap->st_cfg_qos_map_param.auc_up_low[uc_idx] = pst_qos_map->auc_up_low[uc_idx];
    }
    return OAL_SUCC;
}
#endif //_PRE_WLAN_FEATURE_HS20

#ifdef _PRE_WLAN_FEATURE_P2P

oal_uint32  hmac_config_set_p2p_ps_ops_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_cfg_p2p_ops_param_stru *pst_p2p_ops;
    pst_p2p_ops = (mac_cfg_p2p_ops_param_stru *)puc_param;
    OAM_INFO_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_p2p_ps_ops_etc:ctrl:%d, ct_window:%d\r\n}",
                    pst_p2p_ops->en_ops_ctrl,
                    pst_p2p_ops->uc_ct_window);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_P2P_PS_OPS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_p2p_ps_ops_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_p2p_ps_noa_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    mac_cfg_p2p_noa_param_stru  *pst_p2p_noa;
    pst_p2p_noa = (mac_cfg_p2p_noa_param_stru *)puc_param;
    OAM_INFO_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_p2p_ps_noa_etc:start_time:%d, duration:%d, interval:%d, count:%d\r\n}",
                    pst_p2p_noa->ul_start_time,
                    pst_p2p_noa->ul_duration,
                    pst_p2p_noa->ul_interval,
                    pst_p2p_noa->uc_count);
    /* ms to us */
    pst_p2p_noa->ul_start_time *= 1000;
    pst_p2p_noa->ul_duration   *= 1000;
    pst_p2p_noa->ul_interval   *= 1000;
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_P2P_PS_NOA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_p2p_ps_noa_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_p2p_ps_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                    ul_ret;
    mac_cfg_p2p_stat_param_stru  *pst_p2p_stat;
    pst_p2p_stat = (mac_cfg_p2p_stat_param_stru *)puc_param;
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "(hmac_config_set_p2p_ps_stat::ctrl %d\r\n}",
                    pst_p2p_stat->uc_p2p_statistics_ctrl);

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_P2P_PS_STAT, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_p2p_ps_stat::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS

oal_uint32 hmac_config_set_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    oal_mips_type_param_stru      *pst_mips_type_param;

    pst_mips_type_param = (oal_mips_type_param_stru *)puc_param;

    switch (pst_mips_type_param->l_mips_type)
    {
        case OAL_MIPS_TX:
        {
            if (OAL_SWITCH_ON == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_OFF == g_mips_tx_statistic.en_switch)
                {
                    //oal_profiling_mips_tx_init();

                    g_mips_tx_statistic.en_switch = OAL_SWITCH_ON;
                    g_mips_tx_statistic.uc_flag |= BIT0;
                }
            }
            else if (OAL_SWITCH_OFF == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_ON == g_mips_tx_statistic.en_switch)
                {
                    g_mips_tx_statistic.en_switch = OAL_SWITCH_OFF;
                }
            }
        }
        break;

        case OAL_MIPS_RX:
        {
            if (OAL_SWITCH_ON == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_OFF == g_mips_rx_statistic.en_switch)
                {
                    //oal_profiling_mips_rx_init();

                    g_mips_rx_statistic.en_switch = OAL_SWITCH_ON;
                }
            }
            else if (OAL_SWITCH_OFF == pst_mips_type_param->l_switch)
            {
                if (OAL_SWITCH_ON == g_mips_rx_statistic.en_switch)
                {
                    g_mips_rx_statistic.en_switch = OAL_SWITCH_OFF;
                }
            }
        }
        break;

        default:
        {
            OAL_IO_PRINT("hmac_config_set_mips: mips type is wrong!\r\n");
        }
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_MIPS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_mips::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_show_mips(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;
    oal_int32 l_mips_type;

    l_mips_type = *((oal_int32 *)puc_param);

    switch (l_mips_type)
    {
        case OAL_MIPS_TX:
        {
            oal_profiling_tx_mips_show();
        }
        break;

        case OAL_MIPS_RX:
        {
            oal_profiling_rx_mips_show();
        }
        break;

        default:
        {
            OAL_IO_PRINT("hmac_config_show_mips: mips type is wrong!\r\n");
        }
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SHOW_MIPS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_show_mips::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD

oal_uint32 hmac_config_enable_arp_offload(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 配置DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_ENABLE_ARP_OFFLOAD, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_enable_arp_offload::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_ip_addr_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 配置DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_IP_ADDR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_ip_addr_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}


oal_uint32 hmac_config_show_arpoffload_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 配置DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SHOW_ARPOFFLOAD_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_show_arpoffload_info::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif


oal_uint32 hmac_config_user_num_spatial_stream_cap_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    mac_user_nss_stru st_user_nss;
    oal_uint32 ul_ret = OAL_FALSE;
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_user->uc_vap_id, OAM_SF_CFG, "hmac_config_user_num_spatial_stream_cap_syn::mac vap(idx=%d) is null!");
        return ul_ret;
    }

    st_user_nss.en_avail_num_spatial_stream = pst_mac_user->en_avail_num_spatial_stream;
    st_user_nss.en_user_num_spatial_stream  = pst_mac_user->en_user_num_spatial_stream;
    st_user_nss.us_user_idx = pst_mac_user->us_assoc_id;

    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_NSS,
                                    OAL_SIZEOF(mac_user_nss_stru),
                                    (oal_uint8 *)(&st_user_nss));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_user->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_num_spatial_stream_cap_syn::hmac_config_user_num_spatial_stream_cap_syn send event failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32   hmac_config_cfg_vap_h2d_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32            ul_ret;
    mac_device_stru      *pst_dev;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_add_vap_etc::param null,pst_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dev))
    {
        OAM_ERROR_LOG1(0, OAM_SF_ANY, "{hmac_config_cfg_vap_h2d_etc::mac_res_get_dev_etc fail. vap_id[%u]}", pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
    抛事件到DMAC层, 创建dmac cfg vap
    ***************************************************************************/
    ul_ret = hmac_cfg_vap_send_event_etc(pst_dev);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_cfg_vap_send_event_etc::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;

}
#endif
#ifdef _PRE_WLAN_TCP_OPT

oal_uint32  hmac_config_get_tcp_ack_stream_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru    *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_get_tcp_ack_stream_info_etc fail: pst_hmac_vap is null}\r\n");
        return OAL_FAIL;
    }

    hmac_tcp_opt_ack_show_count_etc(pst_hmac_vap);
    return OAL_SUCC;
}


oal_uint32  hmac_config_tx_tcp_ack_opt_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_device_stru    *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_tx_tcp_ack_opt_enable_etc fail: pst_hmac_device is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (0 == ul_val)
    {
        pst_hmac_device->sys_tcp_tx_ack_opt_enable = OAL_FALSE;
    }
    else
    {
        pst_hmac_device->sys_tcp_tx_ack_opt_enable = OAL_TRUE;
    }
    OAM_WARNING_LOG1(0,OAM_SF_ANY,"{hmac_config_tx_tcp_ack_opt_enable_etc:sys_tcp_tx_ack_opt_enable = %d}\r\n",
        pst_hmac_device->sys_tcp_tx_ack_opt_enable);
    return OAL_SUCC;
}


#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ

oal_void  hmac_set_device_freq_mode_etc(oal_uint8 uc_device_enable)
{
    oal_uint32                  ul_ret = OAL_ERR_CODE_PTR_NULL;
    config_device_freq_h2d_stru    st_device_freq_type;
    mac_vap_stru           *pst_mac_vap;
    oal_uint8               uc_index;

    /* 设置Host 调频使能控制 */
    hmac_set_auto_freq_mod_etc(uc_device_enable);

    /* 设置Device 调频使能控制 */
    pst_mac_vap  = mac_res_get_mac_vap(0);
    if(OAL_PTR_NULL == pst_mac_vap)
    {
        return;
    }

    for(uc_index = 0; uc_index < 4; uc_index++)
    {
        st_device_freq_type.st_device_data[uc_index].ul_speed_level = g_host_speed_freq_level_etc[uc_index].ul_speed_level;
        st_device_freq_type.st_device_data[uc_index].ul_cpu_freq_level = g_device_speed_freq_level_etc[uc_index].uc_device_type;
    }

    st_device_freq_type.uc_device_freq_enable = uc_device_enable;
    st_device_freq_type.uc_set_type = FREQ_SET_MODE;

    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_set_device_freq_mode_etc: enable mode[%d][1:enable,0:disable].}", st_device_freq_type.uc_device_freq_enable);

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_DEVICE_FREQ, OAL_SIZEOF(config_device_freq_h2d_stru), (oal_uint8 *)(&st_device_freq_type));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_set_device_freq_mode_etc::hmac_set_device_freq failed[%d].}", ul_ret);
    }
}


oal_uint32 hmac_config_set_device_freq_etc(oal_uint8 uc_device_freq_type)
{
    oal_uint32                  ul_ret;
    config_device_freq_h2d_stru    st_device_freq_type;
    mac_vap_stru           *pst_mac_vap;

    pst_mac_vap  = mac_res_get_mac_vap(0);
    if(OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_device_freq_type.uc_set_freq = uc_device_freq_type;
    /* 调频类型 */
    if (FREQ_HIGHEST >= uc_device_freq_type)
    {
        st_device_freq_type.uc_set_type = FREQ_SET_FREQ;
    }
    else
    {
        st_device_freq_type.uc_set_type = FREQ_SET_PLAT_FREQ;
    }

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_device_freq_etc: set mode[%d],device freq level[%d].}",
           st_device_freq_type.uc_set_type,uc_device_freq_type);

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_DEVICE_FREQ, OAL_SIZEOF(config_device_freq_h2d_stru), (oal_uint8 *)(&st_device_freq_type));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_device_freq_etc::hmac_set_device_freq failed[%d].}", ul_ret);
    }

    return ul_ret;

}

oal_uint32 hmac_config_set_device_freq_testcase(oal_uint8 uc_device_freq_type)
{
    oal_uint32                  ul_ret;
    config_device_freq_h2d_stru    st_device_freq_type;
    mac_vap_stru           *pst_mac_vap;

    pst_mac_vap  = mac_res_get_mac_vap(0);
    if(OAL_PTR_NULL == pst_mac_vap)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_device_freq_type.uc_set_freq = uc_device_freq_type;
    /* 调频类型 */
    if (FREQ_TC_EN== uc_device_freq_type)
    {
        st_device_freq_type.uc_set_type = FREQ_SET_FREQ_TC_EN;
    }
    else if (FREQ_TC_EXIT== uc_device_freq_type)
    {
        st_device_freq_type.uc_set_type = FREQ_SET_FREQ_TC_EXIT;
    }
    else
    {
        st_device_freq_type.uc_set_type = FREQ_SET_BUTT;
    }

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_device_freq_testcase: set mode[%d],device freq testcase enable[%d].}",
           st_device_freq_type.uc_set_type,uc_device_freq_type);

    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_DEVICE_FREQ, OAL_SIZEOF(config_device_freq_h2d_stru), (oal_uint8 *)(&st_device_freq_type));
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_device_freq_testcase:: failed[%d].}", ul_ret);
    }

    return ul_ret;

}

oal_uint32 hmac_config_get_device_freq_etc(oal_void)
{
    oal_uint32                  ul_ret;
    config_device_freq_h2d_stru    st_device_freq_type;
    mac_vap_stru           *pst_mac_vap;

    pst_mac_vap  = mac_res_get_mac_vap(0);

    if(OAL_PTR_NULL != pst_mac_vap)
    {
        st_device_freq_type.uc_set_type = FREQ_GET_FREQ;

        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_get_device_freq_etc!].}");

        /***************************************************************************
            抛事件到DMAC层, 同步VAP最新状态到DMAC
        ***************************************************************************/
        ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_DEVICE_FREQ, OAL_SIZEOF(config_device_freq_h2d_stru), (oal_uint8 *)(&st_device_freq_type));
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_device_freq_etc::hmac_set_device_freq failed[%d].}", ul_ret);
        }
    }
    else
    {
        ul_ret = OAL_ERR_CODE_PTR_NULL;
    }

    return ul_ret;

}

#endif

oal_uint32  hmac_config_rx_tcp_ack_opt_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_device_stru    *pst_hmac_device;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_rx_tcp_ack_opt_enable_etc fail: pst_hmac_device is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (0 == ul_val)
    {
        pst_hmac_device->sys_tcp_rx_ack_opt_enable = OAL_FALSE;
    }
    else
    {
        pst_hmac_device->sys_tcp_rx_ack_opt_enable = OAL_TRUE;
    }
    OAM_WARNING_LOG1(0,OAM_SF_ANY,"{hmac_config_rx_tcp_ack_opt_enable_etc:sys_tcp_tx_ack_opt_enable = %d}\r\n",
        pst_hmac_device->sys_tcp_rx_ack_opt_enable);
    return OAL_SUCC;
}

oal_uint32  hmac_config_tx_tcp_ack_limit_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_vap_stru *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_tx_tcp_ack_limit_etc fail: pst_hmac_vap is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val >= DEFAULT_TX_TCP_ACK_THRESHOLD)
    {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_TX].filter_info.ul_ack_limit = DEFAULT_TX_TCP_ACK_THRESHOLD;
    }
    else
    {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_TX].filter_info.ul_ack_limit = ul_val;
    }
    OAM_WARNING_LOG1(0,OAM_SF_ANY,"{hmac_config_tx_tcp_ack_limit_etc:ul_ack_limit = %ld}\r\n",
        pst_hmac_vap->st_hamc_tcp_ack[HCC_TX].filter_info.ul_ack_limit);
    return OAL_SUCC;
}

oal_uint32  hmac_config_rx_tcp_ack_limit_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_val;
    hmac_vap_stru *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY,
            "{hmac_config_rx_tcp_ack_limit_etc fail: pst_hmac_vap is null}\r\n");
        return OAL_FAIL;
    }

    ul_val = *((oal_uint32 *)puc_param);

    if (ul_val >= DEFAULT_RX_TCP_ACK_THRESHOLD)
    {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_RX].filter_info.ul_ack_limit = DEFAULT_RX_TCP_ACK_THRESHOLD;
    }
    else
    {
        pst_hmac_vap->st_hamc_tcp_ack[HCC_RX].filter_info.ul_ack_limit = ul_val;
    }
    OAM_WARNING_LOG1(0,OAM_SF_ANY,"{hmac_config_rx_tcp_ack_limit_etc:ul_ack_limit = %ld}\r\n",
        pst_hmac_vap->st_hamc_tcp_ack[HCC_RX].filter_info.ul_ack_limit);
    return OAL_SUCC;
}

#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && (LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 44)) && (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_DFT_STAT

oal_uint32  hmac_config_set_performance_log_switch(mac_vap_stru *pst_mac_vap,wlan_cfgid_enum_uint16 en_cfg_id,oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret = OAL_SUCC;
    oal_uint8                       uc_loop_index;
    mac_cfg_set_performance_log_switch_stru *pst_set_performance_log_switch = (mac_cfg_set_performance_log_switch_stru *)puc_param;

    /* 如果是配置VAP, 直接返回 */
    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_performance_log_switch::this is config vap! can't set.}");
        return OAL_FAIL;
    }
    if(pst_set_performance_log_switch->uc_performance_log_switch_type >= DFX_PERFORMANCE_LOG_BUTT)
    {
        for(uc_loop_index = 0;uc_loop_index < DFX_PERFORMANCE_LOG_BUTT;uc_loop_index++)
        {
            DFX_SET_PERFORMANCE_LOG_SWITCH_ENABLE(uc_loop_index,pst_set_performance_log_switch->uc_value);
        }
    }
    else
    {
        DFX_SET_PERFORMANCE_LOG_SWITCH_ENABLE(pst_set_performance_log_switch->uc_performance_log_switch_type,pst_set_performance_log_switch->uc_value);
    }

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_performance_log_switch::log_switch type:%u, value:%u.}",
                                        pst_set_performance_log_switch->uc_performance_log_switch_type, pst_set_performance_log_switch->uc_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_PERFORMANCE_LOG_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_performance_log_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_P2P

oal_uint32  hmac_find_p2p_listen_channel_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8 *puc_p2p_ie = OAL_PTR_NULL;
    oal_uint8 *puc_listen_channel_ie = OAL_PTR_NULL;

    /* 查找P2P IE信息 */
    puc_p2p_ie = mac_find_vendor_ie_etc(MAC_WLAN_OUI_WFA, MAC_WLAN_OUI_TYPE_WFA_P2P, puc_param, (oal_int32)us_len);

    if (OAL_PTR_NULL == puc_p2p_ie)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_find_p2p_listen_channel_etc::p2p ie is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 长度校验 */
    if (puc_p2p_ie[1] < MAC_P2P_MIN_IE_LEN)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_find_p2p_listen_channel_etc::invalid p2p ie len[%d].}", puc_p2p_ie[1]);
        return OAL_FAIL;
    }

    /* 查找P2P Listen channel信息 */
    puc_listen_channel_ie = mac_find_p2p_attribute_etc(MAC_P2P_ATTRIBUTE_LISTEN_CHAN, puc_p2p_ie + 6, (puc_p2p_ie[1] - 4));
    if (OAL_PTR_NULL == puc_listen_channel_ie)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_find_p2p_listen_channel_etc::p2p listen channel ie is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* listen channel长度校验，大端 */
    if (MAC_P2P_LISTEN_CHN_ATTR_LEN != (oal_int32)((puc_listen_channel_ie[2] << 8) + puc_listen_channel_ie[1]))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_find_p2p_listen_channel_etc::invalid p2p listen channel ie len[%d].}", (oal_int32)((puc_listen_channel_ie[2] << 8) + puc_listen_channel_ie[1]));
        return OAL_FAIL;
    }

    /* 获取P2P Listen channel信息 */
    pst_mac_vap->uc_p2p_listen_channel = puc_listen_channel_ie[7];
    OAM_INFO_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_find_p2p_listen_channel_etc::END CHANNEL[%d].}",
                     pst_mac_vap->uc_p2p_listen_channel);

    return OAL_SUCC;
}
#endif

oal_void hmac_config_del_p2p_ie_etc(oal_uint8 *puc_ie, oal_uint32 *pul_ie_len)
{
    oal_uint8       *puc_p2p_ie;
    oal_uint32       ul_p2p_ie_len;
    oal_uint8       *puc_ie_end;
    oal_uint8       *puc_p2p_ie_end;

    if ((OAL_PTR_NULL == puc_ie) || (OAL_PTR_NULL == pul_ie_len) || (0 == *pul_ie_len))
    {
        return;
    }

    puc_p2p_ie = mac_find_vendor_ie_etc(MAC_WLAN_OUI_WFA, MAC_WLAN_OUI_TYPE_WFA_P2P, puc_ie, (oal_int32)(*pul_ie_len));
    if ((OAL_PTR_NULL == puc_p2p_ie) || (puc_p2p_ie[1] < MAC_P2P_MIN_IE_LEN))
    {
        return;
    }

    ul_p2p_ie_len = puc_p2p_ie[1] + MAC_IE_HDR_LEN;

    /* 将p2p ie 后面的内容拷贝到p2p ie 所在位置 */
    puc_ie_end     = (puc_ie + *pul_ie_len);
    puc_p2p_ie_end = (puc_p2p_ie + ul_p2p_ie_len);

    if (puc_ie_end >= puc_p2p_ie_end)
    {
        oal_memmove(puc_p2p_ie, puc_p2p_ie_end, (oal_uint32)(puc_ie_end - puc_p2p_ie_end));
        *pul_ie_len -= ul_p2p_ie_len;
    }
    return;
}
#ifdef _PRE_WLAN_FEATURE_ROAM

oal_uint32 hmac_config_roam_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;
    oal_uint8          uc_enable;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_enable_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_enable = (*puc_param == 0) ? 0 : 1;

    return hmac_roam_enable_etc(pst_hmac_vap, uc_enable);
}


oal_uint32 hmac_config_roam_band_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;
    oal_uint8          uc_band;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_band_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_band = *puc_param;

    return hmac_roam_band_etc(pst_hmac_vap, uc_band);
}


oal_uint32 hmac_config_roam_org_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;
    oal_uint8          uc_scan_orthogonal;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_band_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_scan_orthogonal = *puc_param;

    return hmac_roam_org_etc(pst_hmac_vap, uc_scan_orthogonal);
}



oal_uint32 hmac_config_roam_start_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru       *pst_hmac_vap;
    mac_cfg_set_roam_start_stru *pst_roam_start;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_enable_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_roam_start = (mac_cfg_set_roam_start_stru *)(puc_param);
    return hmac_roam_start_etc(pst_hmac_vap, (roam_channel_org_enum)pst_roam_start->uc_scan_type,
                           pst_roam_start->en_current_bss_ignore, ROAM_TRIGGER_APP);
}


oal_uint32 hmac_config_roam_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru     *pst_hmac_vap;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_roam_enable_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    return hmac_roam_show_etc(pst_hmac_vap);
}
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_11R

oal_uint32 hmac_config_set_ft_ies_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                      *pst_hmac_vap;
    mac_cfg80211_ft_ies_stru           *pst_mac_ft_ies;
    oal_app_ie_stru                     st_ft_ie;
    oal_uint32                          ul_ret;
    oal_uint16                          us_md_id;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_ft_ies_etc::param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(pst_hmac_vap->bit_11r_enable != OAL_TRUE)
    {
        return OAL_SUCC;
    }

    pst_mac_ft_ies = (mac_cfg80211_ft_ies_stru *)puc_param;
    ul_ret = mac_mib_get_md_id_etc(pst_mac_vap, &us_md_id);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies_etc::get_md_id fail[%d].}", ul_ret);
        return ul_ret;
    }

    if (us_md_id != pst_mac_ft_ies->us_mdid)
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                       "{hmac_config_set_ft_ies_etc::UNEXPECTED mdid[%d/%d].}", pst_mac_ft_ies->us_mdid, us_md_id);
        return OAL_FAIL;
    }

    st_ft_ie.en_app_ie_type   = OAL_APP_FT_IE;
    st_ft_ie.ul_ie_len        = pst_mac_ft_ies->us_len;
    oal_memcopy(st_ft_ie.auc_ie, pst_mac_ft_ies->auc_ie, pst_mac_ft_ies->us_len);
    ul_ret = hmac_config_set_app_ie_to_vap_etc(pst_mac_vap, &st_ft_ie, OAL_APP_FT_IE);
    if (ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies_etc::set_app_ie FAIL[%d].}", ul_ret);
        return ul_ret;
    }

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_ft_ies_etc::set_app_ie OK LEN[%d].}", pst_mac_ft_ies->us_len);

    hmac_roam_reassoc_etc(pst_hmac_vap);

    return OAL_SUCC;
}
#endif //_PRE_WLAN_FEATURE_11R

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST

oal_uint32 hmac_config_enable_2040bss_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device;
    oal_bool_enum_uint8    en_2040bss_switch;
        oal_uint8          uc_vap_idx;
    mac_vap_stru          *pst_vap;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_enable_2040bss_etc::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_enable_2040bss_etc:: pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    en_2040bss_switch = (*puc_param == 0) ? OAL_FALSE : OAL_TRUE;
     //同步device下所有vap的mib 2040特性的配置开关
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_vap)
        {
            OAM_ERROR_LOG1(0, OAM_SF_ANY, "{dmac_config_enable_2040bss::pst_mac_vap(%d) null.}",
                           pst_mac_device->auc_vap_id[uc_vap_idx]);
            continue;
        }
        mac_mib_set_2040SwitchProhibited(pst_vap,((en_2040bss_switch == OAL_TRUE)? OAL_FALSE : OAL_TRUE));
    }
    mac_set_2040bss_switch(pst_mac_device, en_2040bss_switch);

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_enable_2040bss_etc:: set 2040bss switch[%d]}", en_2040bss_switch);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    return hmac_config_sync_cmd_common_etc(pst_mac_vap, WLAN_CFGID_2040BSS_ENABLE, us_len, puc_param);
#else
    return OAL_SUCC;
#endif
}


oal_uint32 hmac_config_get_2040bss_sw(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru       *pst_mac_device;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param)))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_get_2040bss_sw::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_2040bss_sw:: pst_mac_device is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    *pus_len = OAL_SIZEOF(oal_uint32);
    *((oal_uint32 *)puc_param) = (oal_uint32)mac_get_2040bss_switch(pst_mac_device);
    return OAL_SUCC;
}


#endif /* _PRE_WLAN_FEATURE_20_40_80_COEXIST */

#ifdef _PRE_FEATURE_WAVEAPP_CLASSIFY


oal_uint32  hmac_config_get_waveapp_flag(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_WAVEAPP_FLAG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_waveapp_flag::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}



oal_uint32  hmac_config_get_waveapp_flag_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru               *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "hmac_config_get_waveapp_flag_rsp: pst_hmac_vap is null ptr.");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->l_temp = (oal_int32)*puc_param;
    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}
#endif


#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)

oal_uint32  hmac_config_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_CALI_POWER, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_cali_power:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

oal_uint32  hmac_config_get_cali_power(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_CALI_POWER, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_cali_power:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

oal_uint32  hmac_config_set_polynomial_param(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_POLYNOMIAL_PARA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_polynomial_param:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

oal_uint32  hmac_config_get_polynomial_params(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    mac_cfg_show_pd_paras_stru              *pst_polynomial_paras;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_get_polynomial_params::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_polynomial_paras = (mac_cfg_show_pd_paras_stru *)(puc_param);

    oal_memcopy(&pst_hmac_vap->st_polynomial_paras,
                    pst_polynomial_paras,
                    OAL_SIZEOF(mac_cfg_show_pd_paras_stru));

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}

oal_uint32  hmac_config_get_cali_power_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    mac_cfg_show_pd_paras_stru              *pst_polynomial_paras;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_get_polynomial_params::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_polynomial_paras = (mac_cfg_show_pd_paras_stru *)(puc_param);

    oal_memcopy(&pst_hmac_vap->st_polynomial_paras,
                    pst_polynomial_paras,
                    OAL_SIZEOF(mac_cfg_show_pd_paras_stru));

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_UPC_PARA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_upc_params:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}


oal_uint32  hmac_config_get_upc_params_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{

    hmac_vap_stru                           *pst_hmac_vap;
    mac_cfg_show_upc_paras_stru             *pst_upc_paras;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_get_upc_params_rsp::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_upc_paras = (mac_cfg_show_upc_paras_stru *)(puc_param);

    oal_memcopy(&pst_hmac_vap->st_upc_paras,
                    pst_upc_paras,
                    OAL_SIZEOF(mac_cfg_show_upc_paras_stru));

    pst_hmac_vap->station_info_query_completed_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}


oal_uint32  hmac_config_set_upc_params(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_UPC_PARA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_upc_params:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}


oal_uint32  hmac_config_set_load_mode(mac_vap_stru *pst_mac_vap,oal_uint16 us_len, oal_uint8 *puc_param)
{
        oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_LOAD_MODE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_load_mode:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#endif


oal_uint32  hmac_config_get_dieid_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                           *pst_hmac_vap;
    mac_cfg_show_dieid_stru                 *pst_dieid;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_get_polynomial_params::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dieid = (mac_cfg_show_dieid_stru *)(puc_param);

    oal_memcopy(&pst_hmac_vap->st_dieid,
                    pst_dieid,
                    OAL_SIZEOF(mac_cfg_show_dieid_stru));

    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

    return OAL_SUCC;
}


oal_uint32  hmac_config_get_dieid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_DIEID, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_cali_power:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}
#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)

oal_uint32  hmac_config_auto_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_AUTO_CALI, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_auto_cali:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}

oal_uint32  hmac_config_get_cali_status(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    mac_device_stru                 *pst_device;

    pst_device= mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_cali_status:en_cali_rdy[%u].}", pst_device->en_cali_rdy);
    *((oal_bool_enum_uint8 *)puc_param) = pst_device->en_cali_rdy;
    *pus_len = OAL_SIZEOF(oal_int8);

    return OAL_SUCC;
}

oal_uint32  hmac_config_set_cali_vref(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_CALI_VREF, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_cali_vref:hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;

}
#endif

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)

oal_uint32 hmac_config_set_txrx_chain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 配置DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_TXRX_CHAIN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_txrx_chain::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}
#ifdef _PRE_WLAN_RF_CALI

oal_uint32 hmac_config_set_2g_txrx_path(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                 ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 配置DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_2G_TXRX_PATH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_2g_txrx_path::hmac_config_send_event_etc fail[%d].", ul_ret);
    }

    return ul_ret;
}
#endif
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_void  hmac_config_set_device_pkt_stat(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_tx_pkts_stat_stru        *pst_pkts_stat;
    oal_uint32                     ul_durance;
    oal_uint32                     ul_snd_mbits;
    mac_cfg_set_tlv_stru          *pst_config_para;

    pst_pkts_stat = &g_host_tx_pkts;
    pst_config_para = (mac_cfg_set_tlv_stru*)puc_param;

    if (PKT_STAT_SET_START_STAT == pst_config_para->uc_cmd_type)
    {
        if (OAL_TRUE == pst_config_para->ul_value)
        {
            pst_pkts_stat->ul_snd_pkts = 0;
            pst_pkts_stat->ul_start_time = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        }
        else
        {
            ul_durance = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            ul_durance -= pst_pkts_stat->ul_start_time;
            if (0 == ul_durance)
            {
                OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_device_pkt_stat::START TIME[%d],NOW TINE[%d].}",
                     pst_pkts_stat->ul_start_time,OAL_TIME_GET_STAMP_MS());
                return;
            }
            ul_snd_mbits = ((pst_pkts_stat->ul_snd_pkts * pst_pkts_stat->ul_pkt_len) / ul_durance) >> 7;

            OAM_ERROR_LOG4(0, OAM_SF_CFG, "{hmac_config_set_device_pkt_stat::snd rate[%d]Mbits,snd pkts[%d],pktlen[%d],time[%d].}",
                 ul_snd_mbits, pst_pkts_stat->ul_snd_pkts,pst_pkts_stat->ul_pkt_len, ul_durance);
        }
    }
    else if (PKT_STAT_SET_FRAME_LEN == pst_config_para->uc_cmd_type)
    {
        pst_pkts_stat->ul_pkt_len = pst_config_para->ul_value;
    }

}
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
oal_uint32 hmac_config_set_tx_ampdu_type(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_set_tlv_stru* pst_set_ampdu_type = (mac_cfg_set_tlv_stru *)puc_param;

    g_uc_tx_ba_policy_select = (oal_uint8)pst_set_ampdu_type->ul_value;
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_tx_ampdu_type::SET TX AMPDU TYPE[%d].}",g_uc_tx_ba_policy_select);

    return OAL_SUCC;
}
#endif


#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
oal_uint32 hmac_config_set_auto_freq_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8 uc_value;
    mac_cfg_set_tlv_stru* pst_set_auto_freq = (mac_cfg_set_tlv_stru *)puc_param;

    uc_value = (oal_uint8)pst_set_auto_freq->ul_value;

    if (CMD_SET_AUTO_FREQ_ENDABLE == pst_set_auto_freq->uc_cmd_type)
    {
        if(FREQ_LOCK_ENABLE == uc_value)
        {
            //设置device调频使能
            hmac_set_device_freq_mode_etc(FREQ_LOCK_ENABLE);
        }
        else
        {
            //设置device调频使能
            hmac_set_device_freq_mode_etc(FREQ_LOCK_DISABLE);
        }
    }
    else if (CMD_SET_DEVICE_FREQ_VALUE == pst_set_auto_freq->uc_cmd_type)
    {
        /* 单独设置DEVICE CPU频率 */
        hmac_config_set_device_freq_etc(uc_value);
    }
    else if (CMD_SET_CPU_FREQ_VALUE == pst_set_auto_freq->uc_cmd_type)
    {
        if(SCALING_MAX_FREQ == uc_value)
        {
            hmac_set_cpu_freq_raw_etc(SCALING_MAX_FREQ, 1805000);
        }
        else
        {
            hmac_set_cpu_freq_raw_etc(SCALING_MIN_FREQ, 807000);
        }
    }
    else if (CMD_SET_DDR_FREQ_VALUE == pst_set_auto_freq->uc_cmd_type)
    {
        if(SCALING_MAX_FREQ == uc_value)
        {
            hmac_set_ddr_freq_raw_etc(SCALING_MAX_FREQ, 1805000);
        }
        else
        {
            hmac_set_ddr_freq_raw_etc(SCALING_MIN_FREQ, 807000);
        }
    }
    else if(CMD_GET_DEVICE_AUTO_FREQ == pst_set_auto_freq->uc_cmd_type)
    {
        hmac_config_get_device_freq_etc();
    }
    else if(CMD_SET_DEVICE_FREQ_TC == pst_set_auto_freq->uc_cmd_type)
    {
        hmac_config_set_device_freq_testcase(uc_value);
    }
    else
    {
        OAM_WARNING_LOG0(0,OAM_SF_ANY,"{hmac_set_device_freq:parameter error!}\r\n");
    }
    OAM_WARNING_LOG2(0,OAM_SF_ANY,"{hmac_config_set_auto_freq_enable_etc:set_auto_freq_enable:uc_cmd_type = %d, uc_value = %d}\r\n",
        pst_set_auto_freq->uc_cmd_type,uc_value);
    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_WDS

oal_uint32  hmac_config_wds_get_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_wds_get_vap_mode::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_get_vap_mode::mac_res_get_hmac_vap failed.}");
        return OAL_FAIL;
    }
    *puc_param = pst_hmac_vap->st_wds_table.en_wds_vap_mode;
    *us_len = sizeof(oal_uint32);

    return OAL_SUCC;
}


oal_uint32  hmac_config_wds_get_sta_num(mac_vap_stru *pst_mac_vap, oal_uint16 *us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;
    oal_wds_info_stru               *pst_wds_info = (oal_wds_info_stru*)puc_param;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_wds_get_sta_num::param null,pst_mac_vap=%p puc_param=%p.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_get_sta_num::mac_res_get_hmac_vap failed, vap id: %d.}", pst_mac_vap->uc_vap_id);
        return OAL_FAIL;
    }

    pst_wds_info->uc_wds_vap_mode = pst_hmac_vap->st_wds_table.en_wds_vap_mode;
    pst_wds_info->uc_neigh_num = pst_hmac_vap->st_wds_table.uc_neigh_num;
    pst_wds_info->uc_wds_node_num = pst_hmac_vap->st_wds_table.uc_wds_node_num;
    pst_wds_info->uc_wds_stas_num = pst_hmac_vap->st_wds_table.uc_wds_stas_num;
    *us_len = OAL_SIZEOF(oal_wds_info_stru);

    return OAL_SUCC;
}


oal_uint32  hmac_config_wds_vap_mode(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8           uc_wds_mode;
    hmac_vap_stru       *pst_hmac_vap = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_wds_vap_mode::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_wds_mode = *puc_param;
    if ( uc_wds_mode >= WDS_MODE_BUTT )
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_wds_vap_mode::invalid parameter.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_vap_mode::mac_res_get_hmac_vap failed.}");
        return OAL_FAIL;
    }

    if (pst_hmac_vap->st_wds_table.en_wds_vap_mode == uc_wds_mode)
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_wds_vap_mode::Wds vap mode already is %d.}", pst_hmac_vap->st_wds_table.en_wds_vap_mode);
        return OAL_SUCC;
    }

    /* 模式切换,之前非NONE状态,清除所有wds信息 */
    if (pst_hmac_vap->st_wds_table.en_wds_vap_mode != WDS_MODE_NONE) {
        /* 删除相应的WDS定时器 */
        if (OAL_TRUE == pst_hmac_vap->st_wds_table.st_wds_timer.en_is_registerd)
        {
            FRW_TIMER_DESTROY_TIMER(&(pst_hmac_vap->st_wds_table.st_wds_timer));
        }
        hmac_wds_reset_sta_mapping_table(pst_hmac_vap);
        hmac_wds_reset_neigh_table(pst_hmac_vap);
    }

    OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_wds_vap_mode::Set wds vap mode from %d to %d.}", pst_hmac_vap->st_wds_table.en_wds_vap_mode, uc_wds_mode);
    pst_hmac_vap->st_wds_table.en_wds_vap_mode = uc_wds_mode;

    hmac_wds_table_create_timer(pst_hmac_vap);

    return OAL_SUCC;
}


oal_uint32  hmac_config_wds_vap_show(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_mac_vap))
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_wds_vap_show::mac vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_vap_show::mac_res_get_hmac_vap failed, vap id: %d.}", pst_mac_vap->uc_vap_id);
        return OAL_FAIL;
    }

    hmac_wds_vap_show_all(pst_hmac_vap, puc_param);

    return OAL_SUCC;
}


oal_uint32  hmac_config_wds_sta_add(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_wds_sta_stru            *pst_cfg_wds_sta = OAL_PTR_NULL;
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_wds_sta_add::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_wds_sta = (mac_cfg_wds_sta_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_sta_add::mac_res_get_hmac_vap failed.}");
        return OAL_FAIL;
    }

    if (OAL_SUCC != hmac_wds_add_sta(pst_hmac_vap, pst_cfg_wds_sta->auc_node_mac, pst_cfg_wds_sta->auc_sta_mac))
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_sta_add::hmac_wds_add_sta fail for node mac[%02x:%02x:%02x:%02x].}",
            pst_cfg_wds_sta->auc_node_mac[2], pst_cfg_wds_sta->auc_node_mac[3], pst_cfg_wds_sta->auc_node_mac[4], pst_cfg_wds_sta->auc_node_mac[5]);
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_sta_add::hmac_wds_add_sta fail for sta mac[%02x:%02x:%02x:%02x].}",
            pst_cfg_wds_sta->auc_sta_mac[2], pst_cfg_wds_sta->auc_sta_mac[3], pst_cfg_wds_sta->auc_sta_mac[4], pst_cfg_wds_sta->auc_sta_mac[5]);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}



oal_uint32  hmac_config_wds_sta_del(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_wds_sta_stru            *pst_cfg_wds_sta = OAL_PTR_NULL;
    hmac_vap_stru                   *pst_hmac_vap = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_wds_sta_del::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cfg_wds_sta = (mac_cfg_wds_sta_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_sta_del::mac_res_get_hmac_vap failed.}");
        return OAL_FAIL;
    }

    if (OAL_SUCC != hmac_wds_del_sta(pst_hmac_vap, pst_cfg_wds_sta->auc_sta_mac))
    {
        OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_sta_del::hmac_wds_del_sta fail for sta mac[%02x:%02x:%02x:%02x].}",
            pst_cfg_wds_sta->auc_sta_mac[2], pst_cfg_wds_sta->auc_sta_mac[3], pst_cfg_wds_sta->auc_sta_mac[4], pst_cfg_wds_sta->auc_sta_mac[5]);

        return OAL_FAIL;
    }

    return OAL_SUCC;
}



oal_uint32  hmac_config_wds_sta_age(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32           ul_wds_age;
    hmac_vap_stru        *pst_hmac_vap = OAL_PTR_NULL;

    if ((OAL_PTR_NULL == pst_mac_vap) || (OAL_PTR_NULL == puc_param))
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_wds_sta_age::param null,pst_mac_vap=%d puc_param=%d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_wds_age = *(oal_uint32 *)puc_param;

    if ((WDS_MIN_AGE_NUM > ul_wds_age) || (WDS_MAX_AGE_NUM <= ul_wds_age))
    {
        OAM_WARNING_LOG0(0, OAM_SF_CFG, "{hmac_config_wds_sta_age::invalid parameter.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_sta_age::mac_res_get_hmac_vap failed.}");
        return OAL_FAIL;
    }

    pst_hmac_vap->st_wds_table.ul_wds_aging = *(oal_uint32 *)puc_param;

    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_wds_sta_age::Set the wds entry aging to %d.}", pst_hmac_vap->st_wds_table.ul_wds_aging);

    return OAL_SUCC;
}
#endif

#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE

oal_uint32  hmac_config_load_ini_power_gain(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_LOAD_INI_PWR_GAIN, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_load_ini_power_gain::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_all_log_level_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret = 0;
    oal_uint8       uc_vap_idx;
    oal_uint8       uc_level;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_all_log_level_etc:: pointer is null,pst_mac_vap[0x%x], puc_param[0x%x] .}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    uc_level = (oal_uint8)(*puc_param);

    for (uc_vap_idx = 0; uc_vap_idx < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_idx++)
    {
        ul_ret += oam_log_set_vap_level_etc(uc_vap_idx, uc_level);

        if (OAL_SUCC != ul_ret)
        {
            return ul_ret;
        }
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_ALL_LOG_LEVEL, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_all_log_level_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_cus_rf_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_CUS_RF, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_cus_rf_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_cus_dts_cali_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_CUS_DTS_CALI, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_cus_dts_cali_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

oal_uint32  hmac_config_set_cus_nvram_params_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_CUS_NVRAM_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_cus_nvram_params_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_set_cus_dyn_cali(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_CUS_DYN_CALI_PARAM, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_cus_dyn_cali::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_dev_customize_info_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32      ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SHOW_DEV_CUSTOMIZE_INFOS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dev_customize_info_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif /* #ifdef _PRE_PLAT_FEATURE_CUSTOMIZE */

#ifdef _PRE_WLAN_FEATURE_HILINK


oal_uint32 hmac_config_set_white_lst_ssidhiden(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    oal_hilink_white_node_stru      *pst_white_node;
    oal_uint32                       ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_white_lst_ssidhiden::null param,pst_mac_vap=%d puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 判断如果是非AP角色，直接返回 */
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_white_lst_ssidhiden::en_vap_mode is WLAN_VAP_MODE_CONFIG.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_white_node = (oal_hilink_white_node_stru *)puc_param;

    if (pst_white_node->en_white_list_operate >= OAL_WHITE_LIST_OPERATE_BUTT)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_white_lst_ssidhiden::[%d] is invalid en_white_list_operate.}",pst_white_node->en_white_list_operate);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    ul_ret = mac_vap_update_hilink_white_list(pst_mac_vap, pst_white_node);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
     抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_WHITE_LIST_SSIDHIDEN, OAL_SIZEOF(oal_hilink_white_node_stru), puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG2(0, OAM_SF_CFG, "{hmac_config_set_white_lst_ssidhiden::hmac_config_send_event_etc failed[%d], vap id[%d].}", ul_ret, pst_mac_vap->uc_vap_id);
    }
#endif

    return ul_ret;
}



oal_uint32  hmac_config_fbt_rej_user(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_kick_user_param_stru   *pst_kick_user_param;
    hmac_vap_stru                  *pst_hmac_vap = OAL_PTR_NULL;
    mac_fbt_mgmt_stru              *pst_fbt_mgmt       = OAL_PTR_NULL;
    oal_uint8                       uc_tmp_idx;
    oal_bool_enum_uint8             en_is_forbidden    = OAL_FALSE;
    oal_uint8                       uc_user_num;
    mac_fbt_disable_user_info_stru *pst_dis_user = OAL_PTR_NULL;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_fbt_rej_user::null param,pst_mac_vap=%d puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (WLAN_VAP_MODE_CONFIG == pst_mac_vap->en_vap_mode)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_fbt_rej_user::en_vap_mode is WLAN_VAP_MODE_CONFIG.}");
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    pst_kick_user_param = (mac_cfg_kick_user_param_stru *)puc_param;
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "{hmac_config_fbt_kick_user::null param,pst_hmac_vap[%d].}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_mgmt = &(pst_hmac_vap->st_vap_base_info.st_fbt_mgmt);

    /* 如果是广播地址 */
    if (oal_is_broadcast_ether_addr(pst_kick_user_param->auc_mac_addr))
    {
        /* 如果rej =0，清空禁止连接列表 */
        if (OAL_FALSE == pst_kick_user_param->uc_rej_user)
        {
            OAL_MEMZERO(pst_fbt_mgmt->ast_fbt_disable_connect_user_list, OAL_SIZEOF(mac_fbt_disable_user_info_stru)*HMAC_FBT_MAX_USER_NUM);
            pst_fbt_mgmt->uc_disabled_user_cnt = 0;
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_fbt_rej_user_mgmt::The disable_user list is empty.}");
        }
        return OAL_SUCC;
    }

    if (OAL_TRUE == pst_kick_user_param->uc_rej_user)
    {
        /* 判断当前数组是否已满 */
        if (pst_fbt_mgmt->uc_disabled_user_cnt >= HMAC_FBT_MAX_USER_NUM)
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_fbt_rej_user_mgmt::ARRAY FULL! disabled_user_cnt = %d.}", pst_fbt_mgmt->uc_disabled_user_cnt);
            return OAL_SUCC;
        }
        /* 若当前列表中已有该用户，直接返回，否则，将该用户添加到列表 */
        else
        {
            for (uc_tmp_idx = 0; uc_tmp_idx < pst_fbt_mgmt->uc_disabled_user_cnt; uc_tmp_idx++)
            {
                pst_dis_user = &(pst_fbt_mgmt->ast_fbt_disable_connect_user_list[uc_tmp_idx]);
                if (0 == oal_memcmp(pst_dis_user->auc_user_mac_addr, pst_kick_user_param->auc_mac_addr, WLAN_MAC_ADDR_LEN))
                {
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
                    pst_dis_user->uc_mlme_phase_mask = pst_kick_user_param->uc_mlme_phase_mask;
#endif
                    return OAL_SUCC;
                }
            }

            oal_memcopy(pst_fbt_mgmt->ast_fbt_disable_connect_user_list[pst_fbt_mgmt->uc_disabled_user_cnt].auc_user_mac_addr, pst_kick_user_param->auc_mac_addr, WLAN_MAC_ADDR_LEN);
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
            pst_fbt_mgmt->ast_fbt_disable_connect_user_list[pst_fbt_mgmt->uc_disabled_user_cnt].uc_mlme_phase_mask = pst_kick_user_param->uc_mlme_phase_mask;
#endif
            pst_fbt_mgmt->uc_disabled_user_cnt++;

        }
    }
    /* 若该用户在禁止连接列表中，删除该用户，后续用户前移，否则, 打印提示信息，直接返回 */
    else
    {
        uc_user_num = pst_fbt_mgmt->uc_disabled_user_cnt;
        for (uc_tmp_idx = 0; uc_tmp_idx < uc_user_num; uc_tmp_idx++)
        {
            pst_dis_user = &(pst_fbt_mgmt->ast_fbt_disable_connect_user_list[uc_tmp_idx]);
            if ((0 != uc_tmp_idx) && (OAL_TRUE == en_is_forbidden))
            {
                oal_memcopy(&(pst_fbt_mgmt->ast_fbt_disable_connect_user_list[uc_tmp_idx - 1]), pst_dis_user, WLAN_MAC_ADDR_LEN);
                OAL_MEMZERO(pst_dis_user, OAL_SIZEOF(mac_fbt_disable_user_info_stru));
            }
            else
            {
                if (0 == oal_memcmp(pst_dis_user->auc_user_mac_addr, pst_kick_user_param->auc_mac_addr, WLAN_MAC_ADDR_LEN))
                {
                    en_is_forbidden = OAL_TRUE;
                    OAL_MEMZERO(pst_dis_user, OAL_SIZEOF(mac_fbt_disable_user_info_stru));
                    pst_fbt_mgmt->uc_disabled_user_cnt--;
                }
            }
        }

        if (OAL_FALSE == en_is_forbidden)
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_fbt_rej_user_mgmt::The user is allowed to access!}");
        }
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN

oal_uint32 hmac_config_get_sta_11k_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_net_dev_ioctl_data_stru        *pst_sta_11k_abillty;
    mac_fbt_mgmt_stru                  *pst_hmac_fbt_mgmt;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_dlist_head_stru                *pst_entry;
    oal_dlist_head_stru                *pst_dlist_tmp;
    mac_user_stru                      *pst_user_tmp;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_get_sta_11k_abillty::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == puc_param)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_get_sta_11k_abillty::puc_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac vap实体中的fbt管理实体 */
    pst_hmac_fbt_mgmt = &(pst_hmac_vap->st_vap_base_info.st_fbt_mgmt);

    /* 如果fbt模式没有打开，则不处理 */
    if (pst_hmac_fbt_mgmt->uc_fbt_mode == HMAC_FBT_MODE_CLOSE)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_config_get_sta_11k_abillty:: open fbt mode first.}");
        return OAL_FAIL;
    }

    pst_sta_11k_abillty = (oal_net_dev_ioctl_data_stru *)puc_param;

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_user_tmp = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        if (OAL_PTR_NULL == pst_user_tmp)
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_config_get_sta_11k_abillty:: pst_user_tmp NULL !!!}");
            continue;
        }

        if (0 == oal_memcmp(pst_user_tmp->auc_user_mac_addr, pst_sta_11k_abillty->pri_data.fbt_get_sta_11k_ability.auc_sta_mac, WLAN_MAC_ADDR_LEN))
        {
            if (OAL_TRUE == pst_user_tmp->st_cap_info.bit_11k_enable)
            {
                pst_sta_11k_abillty->pri_data.fbt_get_sta_11k_ability.en_support_11k = OAL_TRUE;
                oal_memcopy(&(pst_sta_11k_abillty->pri_data.fbt_get_sta_11k_ability.st_sta_11k_ability),&(pst_user_tmp->st_rrm_enabled_cap),OAL_SIZEOF(mac_rrm_enabled_cap_ie_stru));
            }
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}


oal_uint32 hmac_config_set_sta_bcn_request(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_net_dev_ioctl_data_stru            *pst_sta_neighbor_bcn_req;
    mac_user_stru                          *pst_mac_user;
    mac_bcn_req_stru                        st_bcn_req;
    mac_rrm_req_cfg_stru                    st_req_cfg;

    pst_sta_neighbor_bcn_req = (oal_net_dev_ioctl_data_stru *)puc_param;

    /*获取用户*/
    pst_mac_user = mac_vap_get_user_by_addr_etc(pst_mac_vap, pst_sta_neighbor_bcn_req->pri_data.fbt_11k_sta_neighbor_bcn_req.auc_sta_mac);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_sta_bcn_request::mac_user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*optclass打桩*/
    st_bcn_req.uc_optclass      = 0;//hmac_rrm_get_regclass_from_ch_number(pst_radio_meas_cfg->uc_channum);

    st_bcn_req.uc_channum       = pst_sta_neighbor_bcn_req->pri_data.fbt_11k_sta_neighbor_bcn_req.uc_channel_num;
    st_bcn_req.us_random_ivl    = 0;
    st_bcn_req.us_duration      = MAC_BCN_MEASURE_INTERVAL;
    st_bcn_req.en_mode          = RM_BCN_REQ_MEAS_MODE_ACTIVE;
    oal_set_mac_addr(st_bcn_req.auc_bssid, pst_sta_neighbor_bcn_req->pri_data.fbt_11k_sta_neighbor_bcn_req.auc_target_ap_bssid);

    st_req_cfg.en_rpt_notify_id    = HMAC_RRM_RPT_NOTIFY_HILINK;
    st_req_cfg.en_reqtype          = MAC_RRM_TYPE_BCN;
    st_req_cfg.p_arg               = (oal_void*)&st_bcn_req;

    hmac_register_rrm_rpt_notify_func(HMAC_RRM_RPT_NOTIFY_HILINK, MAC_RRM_TYPE_BCN, hmac_hilink_bcn_rpt_notify_hook);
    hmac_config_send_meas_req(pst_mac_vap, pst_mac_user, &st_req_cfg);
    return OAL_SUCC;

}
#endif


oal_uint32 hmac_config_get_sta_11v_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_hilink_get_sta_11v_ability     *pst_sta_11v_abillty;
    mac_fbt_mgmt_stru                  *pst_hmac_fbt_mgmt;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_dlist_head_stru                *pst_entry;
    oal_dlist_head_stru                *pst_dlist_tmp;
    mac_user_stru                      *pst_user_tmp;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_get_sta_11v_abillty::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == puc_param)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_get_sta_11v_abillty::puc_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac vap实体中的fbt管理实体 */
    pst_hmac_fbt_mgmt = &(pst_hmac_vap->st_vap_base_info.st_fbt_mgmt);

    /* 如果fbt模式没有打开，则不处理 */
    if (pst_hmac_fbt_mgmt->uc_fbt_mode == HMAC_FBT_MODE_CLOSE)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_config_get_sta_11v_abillty:: open fbt mode first.}");
        return OAL_FAIL;
    }

    pst_sta_11v_abillty = (oal_hilink_get_sta_11v_ability *)puc_param;

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_user_tmp = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        if (OAL_PTR_NULL == pst_user_tmp)
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_config_get_sta_11v_abillty:: pst_user_tmp NULL !!!}");
            continue;
        }

        if (0 == oal_memcmp(pst_user_tmp->auc_user_mac_addr, pst_sta_11v_abillty->auc_sta_mac, WLAN_MAC_ADDR_LEN))
        {
            if (1 == pst_user_tmp->st_cap_info.bit_bss_transition)
            {
                pst_sta_11v_abillty->en_support_11v = OAL_TRUE;
            }
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}


oal_uint32 hmac_config_change_to_other_ap(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                                  ul_ret = OAL_FAIL;
#ifdef _PRE_WLAN_FEATURE_11V
    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_change_to_other_ap::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    ul_ret = hmac_config_send_event_etc(pst_mac_vap,
                                    WLAN_CFGID_FBT_CHANGE_TO_OTHER_AP,
                                    OAL_SIZEOF(oal_hilink_change_sta_to_target_ap),
                                    puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_change_to_other_ap::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return ul_ret;
#else
    OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_change_to_other_ap::not support 11V !}");
    return ul_ret;
#endif
}



oal_uint32 hmac_config_get_cur_channel(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    oal_hilink_current_channel         *pst_cruuent_channel_info;
    hmac_vap_stru                      *pst_hmac_vap;
    mac_fbt_mgmt_stru                  *pst_hmac_fbt_mgmt;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_get_cur_channel::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取mac vap实体中的fbt管理实体 */
    pst_hmac_fbt_mgmt = &(pst_hmac_vap->st_vap_base_info.st_fbt_mgmt);

    /* 如果fbt模式没有打开，则不处理 */
    if (pst_hmac_fbt_mgmt->uc_fbt_mode == HMAC_FBT_MODE_CLOSE)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_config_get_cur_channel:: open fbt mode first.}");
        return OAL_FAIL;
    }

    pst_cruuent_channel_info = (oal_hilink_current_channel *)puc_param;
    pst_cruuent_channel_info->ul_cur_channel = pst_mac_vap->st_channel.uc_chan_number;
    if (pst_mac_vap->st_channel.uc_chan_number > 14)
    {
        pst_cruuent_channel_info->ul_cur_freq = (oal_uint32)oal_ieee80211_channel_to_frequency((oal_int32)(pst_mac_vap->st_channel.uc_chan_number),IEEE80211_BAND_5GHZ);
    }
    else
    {
        pst_cruuent_channel_info->ul_cur_freq = (oal_uint32)oal_ieee80211_channel_to_frequency((oal_int32)(pst_mac_vap->st_channel.uc_chan_number),IEEE80211_BAND_2GHZ);
    }

    OAM_INFO_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_HILINK, "{hmac_config_get_cur_channel:: cur_channel[%d], freq[%d]}",
                     pst_mac_vap->st_channel.uc_chan_number, pst_mac_vap->st_channel.en_band);

    return OAL_SUCC;

}



oal_uint32 hmac_config_set_stay_time(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_param)
{
    oal_hilink_set_scan_time_param     *pst_fbt_set_stay_time;
    mac_device_stru                    *pst_mac_dev;
    mac_fbt_scan_mgmt_stru             *pst_mac_fbt_mgmt;
    oal_uint16                          us_temp_time;

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_dev)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_device_id, OAM_SF_HILINK, "{hmac_config_set_stay_time::pst_mac_dev null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_fbt_mgmt        = &pst_mac_dev->st_fbt_scan_mgmt;
    pst_fbt_set_stay_time   = (oal_hilink_set_scan_time_param *)puc_param;

    us_temp_time            = pst_fbt_set_stay_time->us_work_channel_stay_time;
    pst_mac_fbt_mgmt->us_work_channel_stay_time = (us_temp_time == 0 ? FBT_DEFAULT_WORK_CHANNEL_STAY_TIME : us_temp_time);

    us_temp_time = pst_fbt_set_stay_time->us_scan_channel_stay_time;
    pst_mac_fbt_mgmt->us_scan_channel_stay_time = (us_temp_time == 0 ? FBT_DEFAULT_SCAN_CHANNEL_STAY_TIME : us_temp_time);
    return OAL_SUCC;

}


oal_uint32  hmac_config_fbt_scan_list_clear(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret = OAL_SUCC;
    mac_device_stru    *pst_mac_dev;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_fbt_scan_list_clear::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_list_clear::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = mac_device_clear_fbt_scan_list(pst_mac_dev, puc_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FBT_SCAN_LIST_CLEAR, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_list_clear::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif
    return ul_ret;

}



oal_uint32  hmac_config_fbt_scan_specified_sta(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_fbt_scan_sta_addr_stru                 *pst_specified_sta_param;
    mac_device_stru                            *pst_mac_dev;
    oal_uint32                                  ul_ret;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_fbt_scan_specified_sta::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_specified_sta::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_specified_sta_param = (mac_fbt_scan_sta_addr_stru *)puc_param;
    ul_ret = mac_device_set_fbt_scan_sta(pst_mac_dev, pst_specified_sta_param);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FBT_SCAN_SPECIFIED_STA, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_specified_sta::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;

}



oal_uint32  hmac_config_fbt_print_scan_list(mac_vap_stru *pst_mac_vap, oal_uint16 uc_len, oal_uint8 *puc_param)
{
    mac_fbt_scan_mgmt_stru     *pst_fbt_scan_info;
    mac_device_stru            *pst_mac_dev;
    hmac_vap_stru              *pst_hmac_vap;
    mac_fbt_mgmt_stru          *pst_fbt_mgmt;
    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_fbt_print_scan_list::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_print_scan_list::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_print_scan_list::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_fbt_mgmt = &(pst_hmac_vap->st_vap_base_info.st_fbt_mgmt);
    pst_fbt_scan_info = &(pst_mac_dev->st_fbt_scan_mgmt);

    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_HILINK,"{hmac_config_fbt_print_scan_list::uc_fbt_mode=%d,uc_disabled_user_cnt=%d}",
                                        pst_fbt_mgmt->uc_fbt_mode,
                                        pst_fbt_mgmt->uc_disabled_user_cnt
                                       );

    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_HILINK,"{hmac_config_fbt_print_scan_list::uc_fbt_scan_enable=%d,uc_scan_channel=%d,ul_scan_report_period=%d}",
                                        pst_fbt_scan_info->uc_fbt_scan_enable,
                                        pst_fbt_scan_info->uc_scan_channel,
                                        pst_fbt_scan_info->ul_scan_report_period);
    return OAL_SUCC;
}



oal_uint32  hmac_config_fbt_scan_interval(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_fbt_scan_interval;
    mac_device_stru            *pst_mac_dev;
    oal_uint32                  ul_ret = OAL_SUCC;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_fbt_scan_interval::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_interval::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_fbt_scan_interval = *((oal_uint32 *)puc_param);
    if (0 == ul_fbt_scan_interval)
    {
        ul_fbt_scan_interval = FBT_DEFAULT_SCAN_INTERVAL;
    }
    ul_ret = mac_device_set_fbt_scan_interval(pst_mac_dev, ul_fbt_scan_interval);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FBT_SCAN_INTERVAL, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_specified_sta::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}

oal_uint32  hmac_config_fbt_scan_channel(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret = OAL_SUCC;
    oal_uint8                   uc_fbt_scan_channel;
    mac_device_stru            *pst_mac_dev;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_fbt_scan_channel::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_channel::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_fbt_scan_channel = *((oal_uint8 *)puc_param);

    /* 判断信道是否有效 */
    ul_ret = mac_is_channel_num_valid_etc(pst_mac_vap->st_channel.en_band, uc_fbt_scan_channel);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_channel::mac_is_channel_num_valid_etc failed.[channel:%d][band:%d][ret:%d].}", uc_fbt_scan_channel,pst_mac_vap->st_channel.en_band, ul_ret);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

#ifdef _PRE_WLAN_FEATURE_11D
    /* 信道14特殊处理，只在11b协议模式下有效 */
    if ((14 == uc_fbt_scan_channel) && (WLAN_LEGACY_11B_MODE != pst_mac_vap->en_protocol))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK,
                         "{hmac_config_fbt_scan_channel::channel-14 only available in 11b, curr protocol=%d.}", pst_mac_vap->en_protocol);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }
#endif

    ul_ret = mac_device_set_fbt_scan_channel(pst_mac_dev, uc_fbt_scan_channel);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FBT_SCAN_CHANNEL, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_specified_sta::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;

}


oal_uint32  hmac_config_fbt_scan_report_period(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  l_value;
    mac_device_stru            *pst_mac_dev;
    oal_uint32                  ul_ret = OAL_SUCC;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_HILINK, "{hmac_config_fbt_scan_report_period::null param,pst_mac_vap=%d puc_param=%d.}",pst_mac_vap, puc_param);
        return OAL_FAIL;
    }

    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_dev))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_report_period::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    l_value = *((oal_uint32 *)puc_param);

    ul_ret = mac_device_set_fbt_scan_report_period(pst_mac_dev, l_value);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FBT_SCAN_REPORT_PERIOD, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_specified_sta::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;

}


oal_uint32  hmac_config_fbt_scan_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{

    oal_uint8                   uc_cfg_fbt_scan_enable;
    mac_device_stru            *pst_mac_device;
    oal_uint32                  ul_ret;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_fbt_scan_enable::pst_mac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 获取mac vap实体中的fbt scan管理实体 */
    pst_mac_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_enable::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户下发的要配置到fbt scan管理实体中的参数 */
    uc_cfg_fbt_scan_enable = *puc_param;

    /* 记录配置的模式到fbt scan管理实体中，当前只支持侦听一个用户 */
    ul_ret = mac_device_set_fbt_scan_enable(pst_mac_device, uc_cfg_fbt_scan_enable);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_FBT_SCAN_ENABLE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_scan_specified_sta::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}


oal_uint32  hmac_config_fbt_start_scan(mac_vap_stru *pst_mac_vap, oal_uint16 uc_len, oal_uint8 *puc_param)
{
    oal_hilink_scan_params         *pst_mac_cfg_fbt_scan_params;
    mac_fbt_scan_sta_addr_stru      st_specified_sta_param;
    mac_device_stru                *pst_mac_dev;
    oal_uint32                      ul_ret;

    pst_mac_cfg_fbt_scan_params = (oal_hilink_scan_params *)puc_param;

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
    if (oal_is_broadcast_ether_addr(pst_mac_cfg_fbt_scan_params->auc_mac) && (pst_mac_cfg_fbt_scan_params->en_is_on == 0))
    {
        hmac_fbt_stop_scan(pst_mac_vap);
        return OAL_SUCC;
    }
#endif
    oal_memcopy(st_specified_sta_param.auc_mac_addr, pst_mac_cfg_fbt_scan_params->auc_mac, WLAN_MAC_ADDR_LEN);

    /* 1、把下发的地址加入到侦听列表 */
    ul_ret = hmac_config_fbt_scan_specified_sta(pst_mac_vap, OAL_SIZEOF(mac_fbt_scan_sta_addr_stru), (oal_uint8*)(&st_specified_sta_param));
    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "hmac_config_fbt_scan_specified_sta failed .ret:%d",ul_ret);
        return ul_ret;
    }
    /* 2、将开关、侦听信道、侦听间隔写入到配置中 */
    pst_mac_dev = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);

    if (0 == pst_mac_cfg_fbt_scan_params->ul_channel)
    {
        ul_ret = hmac_config_fbt_scan_channel(pst_mac_vap, OAL_SIZEOF(oal_uint16), (oal_uint8*)(&(pst_mac_vap->st_channel.uc_chan_number)));
    }
    else
    {
        /* 上次扫描未结束再次发起扫描，且新侦听信道不是工作信道和当前侦听信道*/

        if ((0 != pst_mac_dev->st_fbt_scan_mgmt.uc_scan_channel) &&
            (pst_mac_dev->st_fbt_scan_mgmt.uc_scan_channel != pst_mac_vap->st_channel.uc_chan_number) &&
            (pst_mac_dev->st_fbt_scan_mgmt.uc_scan_channel != pst_mac_cfg_fbt_scan_params->ul_channel))
        {
            OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_start_scan::hilink not support scan 2 channels, work_chan [%d],scanning_chan [%d], new_chan[%02x:%02x]}\r\n",
                            pst_mac_vap->st_channel.uc_chan_number,
                            pst_mac_dev->st_fbt_scan_mgmt.uc_scan_channel,
                            pst_mac_cfg_fbt_scan_params->ul_channel);

            return OAL_ERR_CODE_CONFIG_UNSUPPORT;
        }
        ul_ret = hmac_config_fbt_scan_channel(pst_mac_vap, OAL_SIZEOF(oal_uint16), (oal_uint8*)(&(pst_mac_cfg_fbt_scan_params->ul_channel)));
    }

    if (OAL_SUCC != ul_ret)
    {
         OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "hmac_config_fbt_scan_channel failed .ret:%d",ul_ret);
        return ul_ret;
    }

    if (0 == pst_mac_cfg_fbt_scan_params->ul_interval)
    {
        pst_mac_cfg_fbt_scan_params->ul_interval = FBT_DEFAULT_SCAN_INTERVAL;
    }

    hmac_config_fbt_scan_interval(pst_mac_vap, OAL_SIZEOF(oal_uint16), (oal_uint8*)(&(pst_mac_cfg_fbt_scan_params->ul_interval)));

    hmac_config_fbt_scan_enable(pst_mac_vap, OAL_SIZEOF(oal_uint8), &(pst_mac_cfg_fbt_scan_params->en_is_on));

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_fbt_start_scan::en_is_on [%d],ul_channel [%d], mac[**:**:**:**:%02x:%02x]}\r\n",
                     pst_mac_cfg_fbt_scan_params->en_is_on,
                     pst_mac_cfg_fbt_scan_params->ul_channel,
                     pst_mac_cfg_fbt_scan_params->auc_mac[4],
                     pst_mac_cfg_fbt_scan_params->auc_mac[5]);


    hmac_fbt_start_scan(pst_mac_vap);

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT

oal_uint32 hmac_config_set_mgmt_frame_filters(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret = OAL_SUCC;
    oal_uint32                       ul_mgmt_frame_filters;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_filters::null param,pst_mac_vap=%d puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_mgmt_frame_filters = *(oal_uint32 *)puc_param;
    pst_mac_vap->us_mgmt_frame_filters |= (oal_uint16)ul_mgmt_frame_filters;

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_MGMT_FRAME_FILTERS, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_filters failed[%d].}", ul_ret);
    }
#endif

    return ul_ret;
}

oal_uint32  hmac_config_get_sta_diag_info_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_STA_DIAG_INFO);
}

oal_uint32  hmac_config_get_vap_diag_info_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_VAP_DIAG_INFO);
}

oal_uint32 hmac_config_get_all_sta_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_STA_DIAG_INFO, us_len, puc_param);
}

oal_uint32 hmac_config_get_vap_diag_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_VAP_DIAG_INFO, us_len, puc_param);
}


oal_uint32 hmac_config_set_sensing_bssid(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                       ul_ret;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_HILINK, "{hmac_config_set_sensing_bssid::null param,pst_mac_vap=%d puc_param=%d.}",
                       pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 判断如果是非AP角色，直接返回 */
    if (WLAN_VAP_MODE_BSS_AP != pst_mac_vap->en_vap_mode)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_set_sensing_bssid::en_vap_mode is %d.}", pst_mac_vap->en_vap_mode);
        return OAL_ERR_CODE_INVALID_CONFIG;
    }

    /***************************************************************************
     抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_SENSING_BSSID, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG2(0, OAM_SF_HILINK, "{hmac_config_set_sensing_bssid::hmac_config_send_event_etc failed[%d], vap id[%d].}", ul_ret, pst_mac_vap->uc_vap_id);
    }

    return ul_ret;
}


oal_uint32 hmac_config_get_sensing_bssid_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    return hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_SENSING_BSSID_INFO, us_len, puc_param);
}


oal_uint32  hmac_config_get_sensing_bssid_info_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    return hmac_config_get_param_from_dmac_rsp(pst_mac_vap, uc_len, puc_param, QUERY_ID_SENSING_BSSID_INFO);
}

#endif  // end of _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
#endif  // end of _PRE_WLAN_FEATURE_HILINK


#ifdef _PRE_WLAN_FEATURE_HILINK_TEMP_PROTECT

oal_uint32 hmac_config_receive_all_sta_rssi(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    oal_int32           us_user_idx;
    oal_int8           *pc_rssi = (oal_int8 *)puc_param;
    hmac_user_stru     *pst_hmac_user;
    frw_event_stru      *pst_event;
    frw_event_mem_stru  *pst_event_mem;
    hmac_notify_all_sta_rssi_member *pst_sta_rssi_member;
    hmac_notify_all_sta_rssi_stru   *pst_sta_rssi_stru;

    /* 抛加入完成事件到WAL */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_notify_all_sta_rssi_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_receive_all_sta_rssi::FRW_EVENT_ALLOC fail!}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    pst_sta_rssi_stru = (hmac_notify_all_sta_rssi_stru *)frw_get_event_payload(pst_event_mem);
    oal_memset(pst_sta_rssi_stru, 0, OAL_SIZEOF(hmac_notify_all_sta_rssi_stru));

    for (us_user_idx = 0; us_user_idx < MAC_RES_MAX_USER_LIMIT; us_user_idx++)
    {
        if ((us_user_idx > 0) && (us_user_idx % HMAC_NOTIFY_STA_RSSI_MAX_NUM == 0))
        {
            pst_sta_rssi_stru->ul_start_index = us_user_idx - HMAC_NOTIFY_STA_RSSI_MAX_NUM;
            pst_sta_rssi_stru->ul_sta_count = HMAC_NOTIFY_STA_RSSI_MAX_NUM;
            /* 填写事件 */
            pst_event = frw_get_event_stru(pst_event_mem);
            FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                               FRW_EVENT_TYPE_HOST_CTX,
                               HMAC_HOST_CTX_EVENT_SUB_TYPE_NOTIFY_STA_RSSI,
                               OAL_SIZEOF(hmac_notify_all_sta_rssi_stru),
                               FRW_EVENT_PIPELINE_STAGE_0,
                               pst_mac_vap->uc_chip_id,
                               pst_mac_vap->uc_device_id,
                               pst_mac_vap->uc_vap_id);
            /* 分发事件 */
            frw_event_dispatch_event_etc(pst_event_mem);
            FRW_EVENT_FREE(pst_event_mem);

            /* 抛加入完成事件到WAL */
            pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_notify_all_sta_rssi_stru));
            if (OAL_PTR_NULL == pst_event_mem)
            {
                OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_receive_all_sta_rssi::FRW_EVENT_ALLOC fail!}");
                return OAL_ERR_CODE_ALLOC_MEM_FAIL;
            }
            pst_sta_rssi_stru = (hmac_notify_all_sta_rssi_stru *)frw_get_event_payload(pst_event_mem);
            oal_memset(pst_sta_rssi_stru, 0, OAL_SIZEOF(hmac_notify_all_sta_rssi_stru));
            pst_sta_rssi_stru->ul_start_index = us_user_idx;
        }

        pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(us_user_idx);
        if ((OAL_PTR_NULL != pst_hmac_user) && (pc_rssi[us_user_idx] != -127)) /** 使用-127设置当默认值 */
        {
            pst_hmac_user->c_rssi = pc_rssi[us_user_idx];
            pst_sta_rssi_member = &pst_sta_rssi_stru->ast_sta_rssi[us_user_idx - pst_sta_rssi_stru->ul_start_index];
            oal_memcopy(pst_sta_rssi_member->auc_user_mac_addr, pst_hmac_user->st_user_base_info.auc_user_mac_addr, WLAN_MAC_ADDR_LEN);
            pst_sta_rssi_member->c_rssi = pst_hmac_user->c_rssi;
            pst_sta_rssi_member->uc_valid = 1;
        }
    }

    pst_sta_rssi_stru->ul_sta_count = MAC_RES_MAX_USER_LIMIT - pst_sta_rssi_stru->ul_start_index;
    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_NOTIFY_STA_RSSI,
                       OAL_SIZEOF(hmac_notify_all_sta_rssi_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);
    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);
    return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE


oal_uint32 hmac_config_send_action_frame(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_netbuf_stru                *pst_netbuf_mgmt_tx = OAL_PTR_NULL;
    mac_tx_ctl_stru                *pst_tx_ctl;
    oal_uint32                     ul_ret;
    oal_ieee80211req_send_raw_stru *pst_frame_msg;
    hmac_user_stru                 *pst_hmac_user;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_HILINK, "{hmac_config_send_action_frame::param null, %d %d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_frame_msg = (oal_ieee80211req_send_raw_stru *)puc_param;
    /* 获取用户 */
    pst_hmac_user = mac_vap_get_hmac_user_by_addr_etc(pst_mac_vap, pst_frame_msg->auc_mac_addr);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_send_action_frame::hmac_user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*  申请netbuf 空间*/
    pst_netbuf_mgmt_tx = (oal_netbuf_stru *)OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, pst_frame_msg->us_len, OAL_NETBUF_PRIORITY_MID);

    if(OAL_PTR_NULL == pst_netbuf_mgmt_tx)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_send_action_frame::pst_mgmt_tx null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEM_NETBUF_TRACE(pst_netbuf_mgmt_tx, OAL_TRUE);
    OAL_MEMZERO(oal_netbuf_cb(pst_netbuf_mgmt_tx), OAL_SIZEOF(mac_tx_ctl_stru));

    /*填充netbuf*/
    oal_memcopy( (oal_uint8 *)OAL_NETBUF_HEADER(pst_netbuf_mgmt_tx), pst_frame_msg->puc_msg, pst_frame_msg->us_len);
    oal_netbuf_put(pst_netbuf_mgmt_tx, pst_frame_msg->us_len);

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf_mgmt_tx);                              /* 获取cb结构体 */
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)       = pst_frame_msg->us_len;                      /* dmac发送需要的mpdu长度 */
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)    = pst_hmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_IS_NEED_RESP(pst_tx_ctl)      = OAL_TRUE;                                            /* 发送结果需要上报 */

    /* Buffer this frame in the Memory Queue for transmission */
    ul_ret = hmac_tx_mgmt_send_event_etc(pst_mac_vap, pst_netbuf_mgmt_tx, (oal_uint16)pst_frame_msg->us_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_netbuf_mgmt_tx);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_send_action_frame::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_mgmt_frame_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_vap_ie_set_stru    *pst_ie_data;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::param null, %d %d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_ie_data = (mac_vap_ie_set_stru *)puc_param;

    switch (pst_ie_data->en_eid)
    {
        case MAC_EID_RRM:
        {
            /* 判断IE传入的长度是否正确 */
            if (MAC_RRM_ENABLE_CAP_IE_LEN != pst_ie_data->us_ie_content_len)
            {
                OAM_ERROR_LOG1(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::set rrm ie length:%d invalid.}", pst_ie_data->us_ie_content_len);
                return OAL_FAIL;
            }
            /* 先判断对应IE的指针是否为空 RRM ie为固定长度，如果内存已经申请则替换内容即可 */
            if (OAL_PTR_NULL == pst_mac_vap->pst_rrm_ie_info)
            {
                /* 可变长度指针内存申请 本身结构体长度+IE data字段长度 */
                pst_mac_vap->pst_rrm_ie_info = (mac_vap_ie_set_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
                                               (OAL_SIZEOF(mac_vap_ie_set_stru)+MAC_RRM_ENABLE_CAP_IE_LEN), OAL_TRUE);
                if (OAL_PTR_NULL == pst_mac_vap->pst_rrm_ie_info)
                {
                    OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::mem alloc rrm ie ptr is null.}");
                    return OAL_ERR_CODE_PTR_NULL;
                }
            }
            /* 保存最后一次设置的内存内容 type目前只有4种，oal_uint8存储足够 */
            pst_mac_vap->pst_rrm_ie_info->en_set_type = pst_ie_data->en_set_type;
            pst_mac_vap->pst_rrm_ie_info->en_eid = pst_ie_data->en_eid;
            pst_mac_vap->pst_rrm_ie_info->us_ie_content_len = MAC_RRM_ENABLE_CAP_IE_LEN;
            oal_memcopy(&pst_mac_vap->pst_rrm_ie_info->auc_ie_content, pst_ie_data->auc_ie_content, MAC_RRM_ENABLE_CAP_IE_LEN);
            break;
        }
        case MAC_EID_EXT_CAPS:
        {
            /* 判断IE传入的长度是否正确 */
            if ((MAC_XCAPS_EX_LEN != pst_ie_data->us_ie_content_len)
                && (MAC_XCAPS_EX_FTM_LEN != pst_ie_data->us_ie_content_len))
            {
                OAM_ERROR_LOG1(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::set xcap ie length:%d invalid.}", pst_ie_data->us_ie_content_len);
                return OAL_FAIL;
            }
            if (OAL_PTR_NULL == pst_mac_vap->pst_excap_ie_info)
            {
                /* 可变长度指针内存申请 本身结构体长度+IE data字段长度 */
                pst_mac_vap->pst_excap_ie_info = (mac_vap_ie_set_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
                                                 (OAL_SIZEOF(mac_vap_ie_set_stru) + pst_ie_data->us_ie_content_len), OAL_TRUE);
                if (OAL_PTR_NULL == pst_mac_vap->pst_excap_ie_info)
                {
                    OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::mem alloc xcap ie ptr is null.}");
                    return OAL_ERR_CODE_PTR_NULL;
                }
            }
            /* 保存最后一次设置的内存内容 type目前只有4种，oal_uint8存储足够 */
            pst_mac_vap->pst_excap_ie_info->en_set_type = pst_ie_data->en_set_type;
            pst_mac_vap->pst_excap_ie_info->en_eid  = pst_ie_data->en_eid;
            pst_mac_vap->pst_excap_ie_info->us_ie_content_len = pst_ie_data->us_ie_content_len;
            oal_memcopy(pst_mac_vap->pst_excap_ie_info->auc_ie_content, pst_ie_data->auc_ie_content, pst_ie_data->us_ie_content_len);
            break;
        }
        case MAC_EID_VENDOR:
        {
#ifdef _PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
            /* 开启MULTI-STA特性宏后, 支持管理帧设置4地址IE, 4地址IE为私有IE,仅支持add操作 */
            /* 4地址IE的长度是可变长度 不校验 */
            if (OAL_IE_SET_TYPE_ADD != pst_ie_data->en_set_type)
            {
                OAM_ERROR_LOG1(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::set 4addr ie type:%d invalid, only surpport:2.}", pst_ie_data->en_set_type);
                return OAL_FAIL;
            }
            /* Vendor ie长度可变，前后两次传入的IE长度有可能不同，故需要释放前一次buff再重新申请buff */
            if (OAL_PTR_NULL != pst_mac_vap->pst_msta_ie_info)
            {
                OAL_MEM_FREE(pst_mac_vap->pst_msta_ie_info, OAL_TRUE);
                pst_mac_vap->pst_msta_ie_info = OAL_PTR_NULL;
            }
            /* 可变长度指针内存申请 本身结构体长度+IE data字段长度 */
            pst_mac_vap->pst_msta_ie_info = (mac_vap_ie_set_stru *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,
                                            (OAL_SIZEOF(mac_vap_ie_set_stru)+pst_ie_data->us_ie_content_len), OAL_TRUE);
            if (OAL_PTR_NULL == pst_mac_vap->pst_msta_ie_info)
            {
                OAM_ERROR_LOG0(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::mem alloc 4addr ie ptr is null.}");
                return OAL_ERR_CODE_PTR_NULL;
            }

            /* 保存最后一次设置的内存内容 */
            pst_mac_vap->pst_msta_ie_info->en_set_type = pst_ie_data->en_set_type;
            pst_mac_vap->pst_msta_ie_info->en_eid  = pst_ie_data->en_eid;
            pst_mac_vap->pst_msta_ie_info->us_ie_content_len = pst_ie_data->us_ie_content_len;
            oal_memcopy(pst_mac_vap->pst_msta_ie_info->auc_ie_content, pst_ie_data->auc_ie_content, pst_ie_data->us_ie_content_len);
            if (hmac_vmsta_check_vap_a4_support(pst_ie_data->auc_ie_content, pst_ie_data->us_ie_content_len))
            {
                hmac_vmsta_set_vap_a4_enable(pst_mac_vap);
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::vap surpport 4 address.}");
            }
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT_DEBUG
            /* 打桩 测试用，只要上层传入了4地址IE则开启本地multi-sta能力位 */
            else
            {
                hmac_vmsta_set_vap_a4_enable(pst_mac_vap);
                OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::vap surpport 4 address[debug].}");
            }
#endif
#endif  //_PRE_WLAN_FEATURE_VIRTUAL_MULTI_STA
            break;
        }
        default:
        /* 其他ie类型不支持设置 */
        /* country ie|power constraint ie 驱动在支持11D特性的前提下已默认添加，无需再次实现 */
            OAM_ERROR_LOG1(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_frame_ie::ie id: %d don't support.}", pst_ie_data->en_eid);
            return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_set_mgmt_cap_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_ieee80211req_set_cap_stru   *pst_cap_data;
    mac_cap_info_stru   st_cap_info;
    oal_uint32          ul_set_type;

    if (OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param)
    {
        OAM_ERROR_LOG2(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_cap_info::param null, %d %d.}", pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_cap_data = (oal_ieee80211req_set_cap_stru *)puc_param;
    oal_memcopy(&st_cap_info, &pst_cap_data->us_capbility, OAL_SIZEOF(mac_cap_info_stru));

    /* 根据传入的类型进行设置 cap info 只支持与和或操作 支持设置Radio Measurement */
    ul_set_type = pst_cap_data->ul_type;
    switch(ul_set_type)
    {
        case OAL_IE_SET_TYPE_AND:
        {
        /* 与操作: 0值将对应位置0  1值保持原有值 故只有值为0的位进行了值的变动 */
        /* 11kv的帧直接上报，故只有此bit可以由上层设置，其他bit位请通过hipriv命令关闭mib值，否则会出现异常 */
            if (!st_cap_info.bit_radio_measurement)
            {
                mac_mib_set_dot11RadioMeasurementActivated(pst_mac_vap, OAL_FALSE);
            }
            break;
        }
        case OAL_IE_SET_TYPE_OR:
        {
        /* 或操作: 1值将对应位置1  0值保持原有值 故只有值为1的位进行了值的变动 */
            if (st_cap_info.bit_radio_measurement)
            {
                mac_mib_set_dot11RadioMeasurementActivated(pst_mac_vap, OAL_TRUE);
            }
            break;
        }
        default:
            OAM_ERROR_LOG1(0, OAM_SF_HILINK, "{hmac_config_set_mgmt_cap_info::set type value:%d invalid.}", ul_set_type);
            return OAL_FAIL;
    }
    return OAL_SUCC;
}

#endif  // end of _PRE_WLAN_FEATURE_11KV_INTERFACE


oal_uint32 hmac_config_set_vendor_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_app_ie_stru                *pst_okc_ie;
    oal_uint32                      ul_ret = OAL_SUCC;

    pst_okc_ie = (oal_app_ie_stru *)puc_param;


    /* 设置WPS 信息 */
    ul_ret = hmac_config_set_app_ie_to_vap_etc(pst_mac_vap, pst_okc_ie, pst_okc_ie->en_app_ie_type);
    OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG,"hmac_config_set_vendor_ie::vap_id=%d, ie_type=%d, ie_length=%d\n",
                                    pst_mac_vap->uc_vap_id, pst_okc_ie->en_app_ie_type, pst_okc_ie->ul_ie_len);
    if (ul_ret != OAL_SUCC)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                        "{hmac_config_set_vendor_ie::ul_ret=[%d].}",
                        ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32 hmac_config_get_sta_11h_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_hilink_get_sta_11h_ability     *pst_sta_11h_abillty;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_dlist_head_stru                *pst_entry;
    oal_dlist_head_stru                *pst_dlist_tmp;
    mac_user_stru                      *pst_user_tmp;
    oal_ulong                           ul_temp;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_11h_abillty::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == puc_param)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_11h_abillty::puc_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (us_len == OAL_SIZEOF(oal_hilink_get_sta_11h_ability) + OAL_SIZEOF(ul_temp))
    {
        ul_temp = *((oal_ulong*)(puc_param + OAL_SIZEOF(oal_hilink_get_sta_11h_ability)));
        pst_sta_11h_abillty = (oal_hilink_get_sta_11h_ability *)(ul_temp);
    }
    else
    {
        pst_sta_11h_abillty = (oal_hilink_get_sta_11h_ability *)puc_param;
    }

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_user_tmp = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        if (OAL_PTR_NULL == pst_user_tmp)
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_11h_abillty:: pst_user_tmp NULL !!!}");
            continue;
        }

        if (0 == oal_memcmp(pst_user_tmp->auc_user_mac_addr, pst_sta_11h_abillty->auc_sta_mac, WLAN_MAC_ADDR_LEN))
        {
            if (1 == pst_user_tmp->st_cap_info.bit_spectrum_mgmt)
            {
                pst_sta_11h_abillty->en_support_11h = OAL_TRUE;
            }
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}

#ifdef _PRE_WLAN_FEATURE_11R_AP

oal_uint32 hmac_config_get_sta_11r_abillty(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_hilink_get_sta_11r_ability     *pst_sta_11r_abillty;
    hmac_vap_stru                      *pst_hmac_vap;
    oal_dlist_head_stru                *pst_entry;
    oal_dlist_head_stru                *pst_dlist_tmp;
    mac_user_stru                      *pst_user_tmp;
    oal_ulong                           ul_temp;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_11r_abillty::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if (OAL_PTR_NULL == puc_param)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_device_id, OAM_SF_CFG, "{hmac_config_get_sta_11r_abillty::puc_param null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (us_len == OAL_SIZEOF(oal_hilink_get_sta_11r_ability) + OAL_SIZEOF(ul_temp))
    {
        ul_temp = *((oal_ulong*)(puc_param + OAL_SIZEOF(oal_hilink_get_sta_11r_ability)));
        pst_sta_11r_abillty = (oal_hilink_get_sta_11r_ability *)(ul_temp);
    }
    else
    {
        pst_sta_11r_abillty = (oal_hilink_get_sta_11r_ability *)puc_param;
    }

    OAL_DLIST_SEARCH_FOR_EACH_SAFE(pst_entry, pst_dlist_tmp, &(pst_mac_vap->st_mac_user_list_head))
    {
        pst_user_tmp = OAL_DLIST_GET_ENTRY(pst_entry, mac_user_stru, st_user_dlist);
        if (OAL_PTR_NULL == pst_user_tmp)
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_CFG, "{hmac_config_get_sta_11r_abillty:: pst_user_tmp NULL !!!}");
            continue;
        }

        if (0 == oal_memcmp(pst_user_tmp->auc_user_mac_addr, pst_sta_11r_abillty->auc_sta_mac, WLAN_MAC_ADDR_LEN))
        {
            if (1 == pst_user_tmp->st_cap_info.bit_mdie)
            {
                pst_sta_11r_abillty->en_support_11r = OAL_TRUE;
            }
            return OAL_SUCC;
        }
    }
    return OAL_FAIL;
}

oal_uint32  hmac_config_set_mlme(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_mlme_ie_stru *pst_mlme_ie;
    oal_uint32        ul_ret = OAL_SUCC;

    pst_mlme_ie = (oal_mlme_ie_stru *)puc_param;
    if (OAL_IEEE80211_MLME_AUTH == pst_mlme_ie->en_mlme_type)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mlme::hmac_ft_auth ielength[%d].}", pst_mlme_ie->us_optie_len);
        ul_ret = hmac_ft_auth(pst_mac_vap, pst_mlme_ie);
    }
    else if (OAL_IEEE80211_MLME_ASSOC == pst_mlme_ie->en_mlme_type)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_mlme::hmac_ft_assoc ielength[%d].}", pst_mlme_ie->us_optie_len);
        if (pst_mlme_ie->us_optie_len > 0)
        {
            ul_ret = hmac_ft_assoc(pst_mac_vap, pst_mlme_ie);
        }
    }
    return ul_ret;
}
#endif
#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI

oal_uint32 hmac_config_dyn_cali_param(mac_vap_stru * pst_mac_vap, oal_uint16 us_len, oal_uint8 * puc_param)
{
    oal_uint32    ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DYN_CALI_CFG, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_dyn_cali_param::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif


#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN

oal_uint32  hmac_config_set_tx_classify_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8        uc_flag         = OAL_FALSE;
    hmac_vap_stru   *pst_hmac_vap    = OAL_PTR_NULL;

    /* 获取hmac_vap */
    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_WARNING_LOG1(0,OAM_SF_ANY,"{hmac_config_set_tx_classify_switch_etc::mac_res_get_hmac_vap fail.vap_id[%u]}",pst_mac_vap->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取配置参数 */
    uc_flag = *puc_param;

    /* 参数没有更改，不需要重新配置 */
    if (uc_flag == mac_mib_get_TxTrafficClassifyFlag(pst_mac_vap))
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "hmac_config_set_tx_classify_switch_etc::change nothing to flag:%d", mac_mib_get_TxTrafficClassifyFlag(pst_mac_vap));
        return OAL_SUCC;
    }

    /* 设置参数开关 */
    mac_mib_set_TxTrafficClassifyFlag(pst_mac_vap, uc_flag);

    if (OAL_SWITCH_OFF == mac_mib_get_TxTrafficClassifyFlag(pst_mac_vap))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "hmac_config_set_tx_classify_switch_etc::flag = OAL_SWITCH_OFF(0)");
        return OAL_SUCC;
    }
    else if(OAL_SWITCH_ON == mac_mib_get_TxTrafficClassifyFlag(pst_mac_vap))
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "hmac_config_set_tx_classify_switch_etc::flag = OAL_SWITCH_ON(1)");
        return OAL_SUCC;
    }

    return OAL_FAIL;
}
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST

oal_uint32  hmac_config_send_cw_signal(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SEND_CW_SIGNAL, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_send_cw_signal::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_hipriv_proc_write_process_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param)
{
    hmac_vap_stru       *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_hipriv_proc_write_process_rsp::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(uc_len == sizeof(oal_uint8)*HMAC_HIPRIV_ACK_BUF_SIZE)
    {
        pst_hmac_vap->st_hipriv_ack_stats.auc_data[0] = puc_param[0];
        pst_hmac_vap->st_hipriv_ack_stats.auc_data[1] = puc_param[1];
        pst_hmac_vap->st_hipriv_ack_stats.auc_data[2] = puc_param[2];
    }
    else
    {
        pst_hmac_vap->st_hipriv_ack_stats.auc_data[0] = puc_param[0];
    }

    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_hipriv_ack_stats.uc_get_hipriv_ack_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

   return OAL_SUCC;
}


oal_uint32  hmac_get_rx_pkcg_rsp(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint32 ul_param)
{
    hmac_vap_stru                           *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_atcmdsrv_get_rx_pkcg_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap->st_atcmdsrv_get_status.ul_rx_pkct_succ_num = ul_param;

    /* 唤醒wal_sdt_recv_reg_cmd等待的进程 */
    pst_hmac_vap->st_atcmdsrv_get_status.uc_get_rx_pkct_flag = OAL_TRUE;
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&(pst_hmac_vap->query_wait_q));

   return OAL_SUCC;
}

#endif

#ifdef _PRE_WLAN_FEATURE_11K

oal_uint32  hmac_config_send_neighbor_req_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret = OAL_SUCC;
    oal_netbuf_stru    *pst_action_neighbor_req;
    oal_uint16          us_neighbor_req_frm_len;
    mac_tx_ctl_stru    *pst_tx_ctl;
    oal_uint16          us_index = 0;
    oal_uint8          *puc_data = OAL_PTR_NULL;
    mac_cfg_ssid_param_stru *pst_ssid;
    mac_user_stru       *pst_mac_user;

    pst_ssid = (mac_cfg_ssid_param_stru *)puc_param;

    pst_action_neighbor_req = (oal_netbuf_stru *)OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if(OAL_PTR_NULL == pst_action_neighbor_req)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ASSOC, "{hmac_config_send_neighbor_req_etc::pst_action_neighbor_req null.}");
        return ul_ret;
    }

    OAL_MEMZERO(oal_netbuf_cb(pst_action_neighbor_req), OAL_NETBUF_CB_SIZE());

    puc_data = (oal_uint8 *)OAL_NETBUF_HEADER(pst_action_neighbor_req);

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

    /* All the fields of the Frame Control Field are set to zero. Only the   */
    /* Type/Subtype field is set.                                            */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION| WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* duration */
    puc_data[2] = 0;
    puc_data[3] = 0;

    pst_mac_user = mac_res_get_mac_user_etc(pst_mac_vap->us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_WARNING_LOG1(0, OAM_SF_TX, "{hmac_config_send_neighbor_req_etc::pst_mac_user[%d] null.", pst_mac_vap->us_assoc_vap_id);
        oal_netbuf_free(pst_action_neighbor_req);

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* DA is address of STA requesting association */
    oal_set_mac_addr(puc_data + 4, pst_mac_user->auc_user_mac_addr);

    /* SA is the dot11MACAddress */
    oal_set_mac_addr(puc_data + 10,  mac_mib_get_StationID(pst_mac_vap));

    oal_set_mac_addr(puc_data + 16, pst_mac_vap->auc_bssid);

    /* seq control */
    puc_data[22] = 0;
    puc_data[23] = 0;

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*             Neighbor report request Frame - Frame Body                */
    /*        -------------------------------------------------              */
    /*        | Category | Action |  Dialog Token | Opt SubEle |             */
    /*        -------------------------------------------------              */
    /*        | 1        | 1      |       1       | Var        |             */
    /*        -------------------------------------------------              */
    /*************************************************************************/
    /* Initialize index and the frame data pointer */
    us_index = MAC_80211_FRAME_LEN;

    /* Category */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_RADIO_MEASURMENT;

    /* Action */
    puc_data[us_index++] = MAC_RM_ACTION_NEIGHBOR_REPORT_REQUEST;

    /* Dialog Token */
    puc_data[us_index++]  = 1;

    if (0 != pst_ssid->uc_ssid_len)
    {
        /* Subelement ID */
        puc_data[us_index++] = 0;

        /* length */
        puc_data[us_index++] = pst_ssid->uc_ssid_len;

        /* SSID */
        oal_memcopy(puc_data + us_index, pst_ssid->ac_ssid, pst_ssid->uc_ssid_len);
        us_index += pst_ssid->uc_ssid_len;
    }

    us_neighbor_req_frm_len = us_index;

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_action_neighbor_req);
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)  = us_neighbor_req_frm_len;
    /* 发送完成需要获取user结构体 */
    ul_ret = mac_vap_set_cb_tx_user_idx(pst_mac_vap, pst_tx_ctl, pst_mac_user->auc_user_mac_addr);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_TX, "(hmac_config_send_neighbor_req_etc::fail to find user by xx:xx:xx:0x:0x:0x.}",
        pst_mac_user->auc_user_mac_addr[3],
        pst_mac_user->auc_user_mac_addr[4],
        pst_mac_user->auc_user_mac_addr[5]);
    }

    oal_netbuf_put(pst_action_neighbor_req, us_neighbor_req_frm_len);

    ul_ret = hmac_tx_mgmt_send_event_etc(pst_mac_vap, pst_action_neighbor_req, us_neighbor_req_frm_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_action_neighbor_req);
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_send_neighbor_req_etc::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}


oal_uint32  hmac_config_bcn_table_switch_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_BCN_TABLE_SWITCH, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_bcn_table_switch_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif //_PRE_WLAN_FEATURE_11K


oal_uint32  hmac_config_voe_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                               ul_ret;
    hmac_vap_stru                           *pst_hmac_vap;
    oal_bool_enum_uint8                      en_read_flag;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_voe_enable_etc::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    en_read_flag = (((*puc_param) & 0xFF) & BIT7) ? OAL_TRUE : OAL_FALSE;
    if(OAL_TRUE == en_read_flag)
    {
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_voe_enable_etc::custom_11k=[%d],custom_11v=[%d],custom_11r=[%d].}",
        g_st_mac_voe_custom_param.en_11k, g_st_mac_voe_custom_param.en_11v,
        g_st_mac_voe_custom_param.en_11r);
    #if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11R) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_voe_enable_etc::11k=[%d],11v=[%d],11r=[%d].}",
        pst_hmac_vap->bit_11k_enable, pst_hmac_vap->bit_11v_enable,
        pst_hmac_vap->bit_11r_enable);
    #endif
        return OAL_SUCC;
    }
    else
    {
    #if defined(_PRE_WLAN_FEATURE_11K) || defined(_PRE_WLAN_FEATURE_11R) || defined(_PRE_WLAN_FEATURE_11K_EXTERN)
        pst_hmac_vap->bit_11r_enable         = (((*puc_param) & 0x0F) & BIT0) ? OAL_TRUE : OAL_FALSE;
        pst_hmac_vap->bit_11v_enable         = (((*puc_param) & 0x0F) & BIT1) ? OAL_TRUE : OAL_FALSE;
        pst_hmac_vap->bit_11k_enable         = (((*puc_param) & 0x0F) & BIT2) ? OAL_TRUE : OAL_FALSE;
        pst_hmac_vap->bit_11k_auth_flag      = (((*puc_param) & 0x0F) & BIT3) ? OAL_TRUE : OAL_FALSE;
        pst_hmac_vap->bit_voe_11r_auth       = (((*puc_param) & 0xFF) & BIT4) ? OAL_TRUE : OAL_FALSE;
        pst_hmac_vap->bit_11k_auth_oper_class= (((*puc_param) >> 5) & 0x3);
        pst_hmac_vap->bit_11r_over_ds        =  (((puc_param[1]) & 0xFF) & BIT0) ? OAL_TRUE : OAL_FALSE;

        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_voe_enable_etc::uc_param0 = [0x%X],uc_param1 = [0x%X].}",
        puc_param[0], puc_param[1]);
    #endif
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_VOE_ENABLE, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_voe_enable_etc::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN

oal_uint32  hmac_config_send_radio_meas_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                              ul_ret = OAL_SUCC;
    mac_cfg_radio_meas_info_stru            *pst_radio_meas_cfg;
    mac_user_stru                           *pst_mac_user;
    mac_chn_load_req_stru                   st_chn_load_req;
    mac_bcn_req_stru                        st_bcn_req;
    mac_rrm_req_cfg_stru                    st_req_cfg;

    pst_radio_meas_cfg = (mac_cfg_radio_meas_info_stru *)puc_param;

    /*获取用户*/
    pst_mac_user = mac_vap_get_user_by_addr_etc(pst_mac_vap, pst_radio_meas_cfg->auc_mac_addr);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_send_radio_meas_req::mac_user is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*Neighbor Report Request*/
    if ( MAC_RM_ACTION_NEIGHBOR_REPORT_REQUEST == pst_radio_meas_cfg->uc_action_type )
    {
        st_req_cfg.en_rpt_notify_id    = HMAC_RRM_RPT_NOTIFY_11V;
        st_req_cfg.en_reqtype          = MAC_RRM_TYPE_NEIGHBOR_RPT;
        hmac_config_send_meas_req(pst_mac_vap, pst_mac_user, &st_req_cfg);
        return OAL_SUCC;
    }

    if ( MAC_RM_ACTION_RADIO_MEASUREMENT_REQUEST != pst_radio_meas_cfg->uc_action_type )
    {
        return OAL_FAIL;
    }

    /*Radio Measurement Request*/
    st_req_cfg.us_rpt_num = pst_radio_meas_cfg->us_num_rpt;
    st_req_cfg.uc_req_mode = pst_radio_meas_cfg->uc_req_mode;

    /*param set*/
    pst_radio_meas_cfg->us_random_ivl   = 0;

    /*optclass打桩*/
    pst_radio_meas_cfg->uc_optclass = 0;//hmac_rrm_get_regclass_from_ch_number(pst_radio_meas_cfg->uc_channum);

    switch(pst_radio_meas_cfg->uc_means_type)
    {
        case RM_RADIO_MEAS_CHANNEL_LOAD:
        /*************************************************************************/
        /*                    Channel Load Request                               */
        /* --------------------------------------------------------------------- */
        /* |Operating Class |Channel Number |Rand Interval| Meas Duration       |*/
        /* --------------------------------------------------------------------- */
        /* |1               |1              | 2           | 2                   |*/
        /* --------------------------------------------------------------------- */
        /* --------------------------------------------------------------------- */
        /* |Optional Subelements                                                |*/
        /* --------------------------------------------------------------------- */
        /* | var                                                                |*/
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/
            st_chn_load_req.uc_optclass      = pst_radio_meas_cfg->uc_optclass;
            st_chn_load_req.uc_channum       = pst_radio_meas_cfg->uc_channum;
            st_chn_load_req.us_random_ivl    = pst_radio_meas_cfg->us_random_ivl;
            st_chn_load_req.us_duration      = pst_radio_meas_cfg->us_duration;

            st_req_cfg.en_rpt_notify_id    = HMAC_RRM_RPT_NOTIFY_CHN_LOAD;
            st_req_cfg.en_reqtype          = MAC_RRM_TYPE_CHANNEL_LOAD;
            st_req_cfg.p_arg               = (oal_void*)&st_chn_load_req;
            hmac_config_send_meas_req(pst_mac_vap, pst_mac_user, &st_req_cfg);

            break;
        case RM_RADIO_MEAS_BCN:
        /*************************************************************************/
        /*                    Beacon Request                                     */
        /* --------------------------------------------------------------------- */
        /* |Operating Class |Channel Number |Rand Interval| Meas Duration       |*/
        /* --------------------------------------------------------------------- */
        /* |1               |1              | 2           | 2                   |*/
        /* --------------------------------------------------------------------- */
        /* --------------------------------------------------------------------- */
        /* |Meas Mode       |BSSID          |Optional Subelements               |*/
        /* --------------------------------------------------------------------- */
        /* |1               |6              | var                               |*/
        /* --------------------------------------------------------------------- */
        /*                                                                       */
        /*************************************************************************/
            st_bcn_req.uc_optclass      = pst_radio_meas_cfg->uc_optclass;
            st_bcn_req.uc_channum       = pst_radio_meas_cfg->uc_channum;
            st_bcn_req.us_random_ivl    = pst_radio_meas_cfg->us_random_ivl;
            st_bcn_req.us_duration      = pst_radio_meas_cfg->us_duration;
            st_bcn_req.en_mode          = pst_radio_meas_cfg->uc_bcn_mode;
            oal_set_mac_addr(st_bcn_req.auc_bssid, pst_radio_meas_cfg->auc_bssid);

            st_req_cfg.en_rpt_notify_id    = HMAC_RRM_RPT_NOTIFY_HILINK;
            st_req_cfg.en_reqtype          = MAC_RRM_TYPE_BCN;
            st_req_cfg.p_arg               = (oal_void*)&st_bcn_req;
            hmac_config_send_meas_req(pst_mac_vap, pst_mac_user, &st_req_cfg);
            break;
        default:
            OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_config_send_radio_meas_req::invalid uc_means_type[%d].}",
                pst_radio_meas_cfg->uc_means_type);
            return OAL_FAIL;
    }

    return ul_ret;
}


oal_uint32 hmac_config_set_11k_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_vap_stru       *pst_hmac_vap;

    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_11k_switch::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*11k switch*/
    pst_hmac_vap->bit_11k_enable = *puc_param;
    mac_mib_set_dot11RadioMeasurementActivated(pst_mac_vap, pst_hmac_vap->bit_11k_enable);

    return OAL_SUCC;
}
#endif

oal_uint32 hmac_config_vendor_cmd_get_channel_list_etc(mac_vap_stru *pst_mac_vap, oal_uint16 *pus_len, oal_uint8 *puc_param)
{
    oal_uint8  uc_chan_idx;
    oal_uint8  uc_chan_num;
    oal_uint8  uc_chan_number;
    oal_uint8 *puc_channel_list;
    mac_vendor_cmd_channel_list_stru *pst_channel_list = OAL_PTR_NULL;
    oal_uint32 ul_ret;

    if (pus_len == OAL_PTR_NULL || puc_param == OAL_PTR_NULL)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_vendor_cmd_get_channel_list_etc::len or param is NULL."
                        " len %p, param %p}",
                        pus_len, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_channel_list = (mac_vendor_cmd_channel_list_stru *)puc_param;
    *pus_len = OAL_SIZEOF(mac_vendor_cmd_channel_list_stru);

    /* 获取2G 信道列表 */
    uc_chan_num = 0;
    puc_channel_list   = pst_channel_list->auc_channel_list_2g;

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_2_BUTT; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_2, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx_etc(MAC_RC_START_FREQ_2, uc_chan_idx, &uc_chan_number);
            puc_channel_list[uc_chan_num++] = uc_chan_number;
        }
    }
    pst_channel_list->uc_channel_num_2g = uc_chan_num;

    /* 检查定制化5g开关是否使能 */
    if (OAL_FALSE == mac_device_check_5g_enable(pst_mac_vap->uc_device_id))
    {
        pst_channel_list->uc_channel_num_5g = 0;
        return OAL_SUCC;
    }

    /* 获取5G 信道列表 */
    uc_chan_num = 0;
    puc_channel_list   = pst_channel_list->auc_channel_list_5g;

    for (uc_chan_idx = 0; uc_chan_idx < MAC_CHANNEL_FREQ_5_BUTT; uc_chan_idx++)
    {
        ul_ret = mac_is_channel_idx_valid_etc(MAC_RC_START_FREQ_5, uc_chan_idx);
        if (OAL_SUCC == ul_ret)
        {
            mac_get_channel_num_from_idx_etc(MAC_RC_START_FREQ_5, uc_chan_idx, &uc_chan_number);
            puc_channel_list[uc_chan_num++] = uc_chan_number;
        }
    }
    pst_channel_list->uc_channel_num_5g = uc_chan_num;

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE

oal_uint32  hmac_config_packet_capture_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                   ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PACKET_CAPTURE_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_packet_capture_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_SMARTANT
oal_uint32  hmac_config_get_ant_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_GET_ANT_INFO, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_get_ant_info::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return ul_ret;
}
oal_uint32  hmac_config_double_ant_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret;
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DOUBLE_ANT_SW, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_double_ant_switch::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }
    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_FEATURE_USER_EXTEND

oal_uint32 hmac_config_user_extend_enable(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_chip_stru      *pst_mac_chip;
    oal_uint32          ul_ret;

    pst_mac_chip = hmac_res_get_mac_chip(pst_mac_vap->uc_chip_id);
    if (OAL_PTR_NULL == pst_mac_chip)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_extend_enable::pst_mac_chip null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 设置值和当前值一致，返回 */
    /*lint -e731 */
    if (pst_mac_chip->st_user_extend.en_flag == !!(*puc_param))
    {
        if (OAL_TRUE == pst_mac_chip->st_user_extend.en_flag)
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_extend_enable:: already enabled.}");
        }
        else
        {
            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_extend_enable:: alread disabled.}");
        }
        return OAL_SUCC;
    }

    /* 配置HMAC侧开关 */
    pst_mac_chip->st_user_extend.en_flag = !!(*puc_param);
    /*lint +e731 */

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_USER_EXTEND_ENABLE, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_extend_enable::hmac_config_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 开关发生变化时，重启chip下所有工作的vap */
    ul_ret = hmac_restart_all_work_vap(pst_mac_chip->uc_chip_id);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_user_extend_enable::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

#ifdef _PRE_WLAN_11K_STAT


oal_uint32  hmac_config_query_stat_info(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32          ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_QUERY_STAT_INFO, us_len, puc_param);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_query_stat_info::hmac_config_send_event_etc failed[%d].}", ul_ret);
    }

    return ul_ret;
}
#endif

oal_uint32  hmac_config_set_priv_flag(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    hmac_device_stru      *pst_hmac_device;
    oal_bool_enum_uint8    en_val;

    en_val =  *(oal_bool_enum_uint8 *)puc_param;
    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device || OAL_PTR_NULL == pst_hmac_device->pst_device_base_info)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_device->en_start_via_priv = en_val;
    OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_priv_flag::set start priv=[%d] in hmac.}", pst_hmac_device->en_start_via_priv);
    OAL_IO_PRINT("set start priv=%d in hmac\n", pst_hmac_device->en_start_via_priv);
    return OAL_SUCC;
}

oal_uint32  hmac_config_set_bw_fixed(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                  ul_ret = OAL_SUCC;

    /* 入参检查 */
    if (OAL_PTR_NULL == pst_mac_vap || ((*puc_param != 0) && (*puc_param != 1)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_config_set_bw_fixed::pst_mac_vap null or invalid param.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap->bit_bw_fixed = *puc_param;
    OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_bw_fixed:bw_fixed = [%d].}",pst_mac_vap->bit_bw_fixed);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
     /***************************************************************************
         抛事件到DMAC层, 同步DMAC数据
     ***************************************************************************/
     ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_SET_BW_FIXED, us_len, puc_param);
     if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
     {
         OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_bw_fixed::hmac_config_send_event_etc failed[%d].}", ul_ret);
     }
#endif

    return ul_ret;

}
#ifdef _PRE_WLAN_FEATURE_DBDC

oal_uint32 hmac_config_dbdc_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                          ul_ret;
    hmac_device_stru                   *pst_hmac_device;
    mac_dbdc_debug_switch_stru         *pst_dbdc_debug_switch;

    pst_dbdc_debug_switch = (mac_dbdc_debug_switch_stru *)puc_param;

    pst_hmac_device = hmac_res_get_mac_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_hmac_device)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DBDC, "{hmac_config_dbdc_debug_switch::hmac device[%d] is null.",pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_DBDC_DEBUG_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DBDC, "{hmac_config_dbdc_debug_switch::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_set_tlv_cmd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32              ul_ret;
    mac_cfg_set_tlv_stru   *pst_config_para;

    pst_config_para = (mac_cfg_set_tlv_stru*)puc_param;

    /* HOST需要处理的事件 */
    switch (pst_config_para->us_cfg_id)
    {
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        case WLAN_CFGID_SET_DEVICE_PKT_STAT:
            hmac_config_set_device_pkt_stat(pst_mac_vap, us_len, puc_param);
            return OAL_SUCC;

        case WLAN_CFGID_SET_DEVICE_MEM_FREE_STAT:
            break;

        case WLAN_CFGID_SET_TX_AMPDU_TYPE:
            hmac_config_set_tx_ampdu_type(pst_mac_vap, us_len, puc_param);
            return OAL_SUCC;
#endif

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
        case WLAN_CFGID_SET_DEVICE_FREQ:
            hmac_config_set_auto_freq_enable_etc(pst_mac_vap, us_len, puc_param);
            return OAL_SUCC;
#endif

        case WLAN_CFGID_AMSDU_AMPDU_SWITCH:
            hmac_config_amsdu_ampdu_switch_etc(pst_mac_vap, us_len, puc_param);
            return OAL_SUCC;

        case WLAN_CFGID_SET_RX_AMPDU_AMSDU:
            hmac_config_rx_ampdu_amsdu(pst_mac_vap, us_len, puc_param);
            /* 1103 mpw2 由于存在译码错误导致去使能amsdu默认打开可能导致死机 默认是关闭,不再开启 */
            return OAL_SUCC;
        case WLAN_CFGID_SET_SK_PACING_SHIFT:
            g_sk_pacing_shift_etc =(oal_uint8)((mac_cfg_set_tlv_stru *)puc_param)->ul_value;
            return OAL_SUCC;
        case WLAN_CFGID_SET_ADDBA_RSP_BUFFER:
            hmac_config_set_addba_rsp_buffer(pst_mac_vap,us_len, puc_param);
            return OAL_SUCC;
#ifdef _PRE_WLAN_FEATURE_DFS_ENABLE
        case WLAN_CFGID_SET_DFS_MODE:
            break;
#endif
        case WLAN_CFGID_SET_ADC_DAC_FREQ:
            break;



        default:
            break;
    }

    /* DEVICE需要处理的事件 */
    /***************************************************************************
        抛事件到DMAC层, 同步VAP最新状态到DMAC
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, pst_config_para->us_cfg_id, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_set_tlv_cmd::send msg failed[%d].}", ul_ret);
    }

    return ul_ret;
}

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)

oal_uint32 hmac_config_pm_debug_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32 ul_ret;

    /***************************************************************************
        抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event_etc(pst_mac_vap, WLAN_CFGID_PM_DEBUG_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_DBDC, "{hmac_config_pm_debug_switch::hmac_config_send_event_etc fail[%d].", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}
#endif

#ifdef _PRE_WLAN_FEATURE_APF

OAL_STATIC oal_void hmac_print_apf_program(oal_uint8* puc_program, oal_uint32 ul_program_len)
{
    oal_uint32                  ul_idx, ul_string_len;
    oal_int32                   l_string_tmp_len;
    oal_uint8                  *pc_print_buff;


    pc_print_buff = (oal_int8 *)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, OAM_REPORT_MAX_STRING_LEN, OAL_TRUE);
    if (OAL_PTR_NULL == pc_print_buff)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CFG, "{hmac_print_apf_program::pc_print_buff null.}");
        return;
    }
    OAL_MEMZERO(pc_print_buff, OAM_REPORT_MAX_STRING_LEN);
    ul_string_len    = 0;
    l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len,
                        (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                        "Id           :200\n"
                        "Program len  :%d\n",
                        ul_program_len);

    if (l_string_tmp_len < 0)
    {
        OAM_ERROR_LOG1(0, OAM_SF_CFG, "hmac_print_apf_program:sprintf return error[%d]", l_string_tmp_len);
        OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
        return;
    }
    ul_string_len += l_string_tmp_len;

    for(ul_idx = 0; ul_idx < ul_program_len; ul_idx++)
    {
        l_string_tmp_len = OAL_SPRINTF(pc_print_buff + ul_string_len,
                            (OAM_REPORT_MAX_STRING_LEN - ul_string_len - 1),
                            "%02x",
                            puc_program[ul_idx]);
        if (l_string_tmp_len < 0)
        {
            break;
        }
        ul_string_len += l_string_tmp_len;
    }

    pc_print_buff[OAM_REPORT_MAX_STRING_LEN-1] = '\0';
    oam_print_etc(pc_print_buff);
    OAL_MEM_FREE(pc_print_buff, OAL_TRUE);
}

oal_uint32  hmac_config_apf_filter_cmd(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint32                ul_ret;
    dmac_tx_event_stru       *pst_tx_event;
    frw_event_mem_stru       *pst_event_mem;
    oal_netbuf_stru          *pst_cmd_netbuf = OAL_PTR_NULL;
    frw_event_stru           *pst_hmac_to_dmac_ctx_event;
    mac_apf_filter_cmd_stru  *pst_apf_filter_cmd;


    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_set_apf_program::null param,pst_mac_vap=%d puc_param=%d.}",
                     pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }


    pst_apf_filter_cmd = (mac_apf_filter_cmd_stru*)puc_param;

    /* 申请netbuf */
    pst_cmd_netbuf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, OAL_SIZEOF(mac_apf_cmd_type_uint8) + pst_apf_filter_cmd->us_program_len, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_cmd_netbuf)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_set_apf_program::netbuf alloc null,size %d.}", pst_apf_filter_cmd->us_program_len);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 拷贝命令结构体到netbuf */
    oal_memcopy(OAL_NETBUF_DATA(pst_cmd_netbuf), &pst_apf_filter_cmd->en_cmd_type, OAL_SIZEOF(mac_apf_cmd_type_uint8));
    oal_netbuf_put(pst_cmd_netbuf, OAL_SIZEOF(mac_apf_cmd_type_uint8));

    if (APF_SET_FILTER_CMD == pst_apf_filter_cmd->en_cmd_type)
    {
        /* program内容拷贝到netbuf */
        oal_memcopy(OAL_NETBUF_DATA(pst_cmd_netbuf)+OAL_SIZEOF(mac_apf_cmd_type_uint8), pst_apf_filter_cmd->puc_program, pst_apf_filter_cmd->us_program_len);
        oal_netbuf_put(pst_cmd_netbuf, pst_apf_filter_cmd->us_program_len);
        /* 打印到sdt */
        hmac_print_apf_program(pst_apf_filter_cmd->puc_program, pst_apf_filter_cmd->us_program_len);
    }

    /***************************************************************************
      抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
      OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_apf_program::pst_event_mem null.}");
      oal_netbuf_free(pst_cmd_netbuf);
      return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_to_dmac_ctx_event = (frw_event_stru *)pst_event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(pst_hmac_to_dmac_ctx_event->st_event_hdr),
                  FRW_EVENT_TYPE_WLAN_CTX,
                  DMAC_WLAN_CTX_EVENT_SUB_TYPE_APF_CMD,
                  OAL_SIZEOF(dmac_tx_event_stru),
                  FRW_EVENT_PIPELINE_STAGE_1,
                  pst_mac_vap->uc_chip_id,
                  pst_mac_vap->uc_device_id,
                  pst_mac_vap->uc_vap_id);

    pst_tx_event = (dmac_tx_event_stru *)(pst_hmac_to_dmac_ctx_event->auc_event_data);
    pst_tx_event->pst_netbuf    = pst_cmd_netbuf;
    pst_tx_event->us_frame_len  = OAL_NETBUF_LEN(pst_cmd_netbuf);

    ul_ret = frw_event_dispatch_event_etc(pst_event_mem);
    if (ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{hmac_config_set_apf_program::frw_event_dispatch_event_etc failed[%d].}", ul_ret);
    }

    oal_netbuf_free(pst_cmd_netbuf);
    FRW_EVENT_FREE(pst_event_mem);

    return ul_ret;
}


oal_uint32  hmac_apf_program_report_event(frw_event_mem_stru  *pst_event_mem)
{
    frw_event_stru             *pst_event;
    frw_event_hdr_stru         *pst_event_hdr;
    dmac_apf_report_event_stru *pst_report_event;
    oal_netbuf_stru            *pst_netbuf;
    oal_uint8                  *puc_program;

    /* 获取事件头和事件结构体指针 */
    pst_event        = frw_get_event_stru(pst_event_mem);
    pst_event_hdr    = &(pst_event->st_event_hdr);
    pst_report_event = (dmac_apf_report_event_stru*)pst_event->auc_event_data;

    pst_netbuf = (oal_netbuf_stru*)pst_report_event->p_program;
    if (!pst_netbuf)
    {
        OAM_WARNING_LOG0(0,OAM_SF_CFG, "hmac_apf_program_report_event:netbuf is null");
        return OAL_FAIL;
    }
    puc_program = OAL_NETBUF_DATA(pst_netbuf);
    hmac_print_apf_program(OAL_NETBUF_DATA(pst_netbuf), OAL_NETBUF_LEN(pst_netbuf));

    oal_netbuf_free(pst_netbuf);
    return OAL_SUCC;
}
#endif


oal_uint32  hmac_config_remove_app_ie(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                 uc_type;
    oal_uint8                 uc_eid;
    hmac_vap_stru             *pst_hmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_remove_app_ie::null param,pst_mac_vap=%d puc_param=%d.}",
                     pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap = mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ANY, "{hmac_config_remove_app_ie::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_type = puc_param[0];
    uc_eid = puc_param[1];

    switch(uc_type)
    {
        case 0:
        case 1:
            pst_hmac_vap->st_remove_ie.uc_type = uc_type;
            pst_hmac_vap->st_remove_ie.uc_eid = uc_eid;
            break;
        default:
            OAM_WARNING_LOG1(0, OAM_SF_CFG, "{hmac_config_remove_app_ie::remove type %d is not supported.}", uc_type);
            break;
    }

    return OAL_SUCC;
}

#ifdef  _PRE_WLAN_FEATURE_FORCE_STOP_FILTER
oal_uint32  hmac_config_force_stop_filter(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap || OAL_PTR_NULL == puc_param))
    {
        OAM_ERROR_LOG2(0, OAM_SF_CFG, "{hmac_config_rx_filter_force_switch::null param,pst_mac_vap=%d puc_param=%d.}",
                     pst_mac_vap, puc_param);
        return OAL_ERR_CODE_PTR_NULL;
    }


    return hmac_config_sync_cmd_common_etc(pst_mac_vap, WLAN_CFGID_FORCE_STOP_FILTER, us_len, puc_param);
}
#endif

/*lint -e578*//*lint -e19*/
#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)
oal_module_symbol(hmac_config_get_blacklist_mode);
oal_module_symbol(hmac_config_set_blacklist_mode_etc);
oal_module_symbol(hmac_config_blacklist_add_etc);
oal_module_symbol(hmac_config_blacklist_add_only_etc);
oal_module_symbol(hmac_config_blacklist_del_etc);
oal_module_symbol(hmac_config_show_blacklist_etc);
oal_module_symbol(hmac_config_autoblacklist_enable_etc);
oal_module_symbol(hmac_config_set_autoblacklist_aging_etc);
oal_module_symbol(hmac_config_set_autoblacklist_threshold_etc);
oal_module_symbol(hmac_config_set_autoblacklist_reset_time_etc);
#endif /* (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE) */
#ifdef _PRE_WLAN_FEATURE_ISOLATION
oal_module_symbol(hmac_config_show_isolation);
oal_module_symbol(hmac_config_set_isolation_mode);
oal_module_symbol(hmac_config_set_isolation_type);
oal_module_symbol(hmac_config_set_isolation_forword);
oal_module_symbol(hmac_config_set_isolation_clear);
#endif /* _PRE_WLAN_FEATURE_CUSTOM_SECURITY */
oal_module_symbol(hmac_config_set_pmksa_etc);
oal_module_symbol(hmac_config_del_pmksa_etc);
oal_module_symbol(hmac_config_flush_pmksa_etc);
oal_module_symbol(hmac_config_get_version_etc);
oal_module_symbol(hmac_config_get_ant_etc);
oal_module_symbol(hmac_config_send_2040_coext_etc);
oal_module_symbol(hmac_config_2040_coext_info_etc);
oal_module_symbol(hmac_config_set_auto_protection_etc);
oal_module_symbol(hmac_config_protocol_debug_switch);
oal_module_symbol(hmac_config_phy_debug_switch);
oal_module_symbol(hmac_config_set_random_mac_addr_scan_etc);
oal_module_symbol(hmac_config_set_random_mac_oui_etc);
oal_module_symbol(hmac_config_start_vap_etc);
oal_module_symbol(hmac_config_add_vap_etc);
oal_module_symbol(hmac_config_del_vap_etc);
oal_module_symbol(hmac_config_down_vap_etc);
oal_module_symbol(hmac_config_set_mac_addr_etc);
oal_module_symbol(hmac_config_set_mode_etc);
oal_module_symbol(hmac_config_get_mode_etc);
oal_module_symbol(hmac_config_set_bss_type_etc);
oal_module_symbol(hmac_config_get_bss_type_etc);
oal_module_symbol(hmac_config_set_ssid_etc);
oal_module_symbol(hmac_config_get_ssid_etc);
oal_module_symbol(hmac_config_set_shpreamble_etc);
oal_module_symbol(hmac_config_get_shpreamble_etc);
oal_module_symbol(hmac_config_set_shortgi20_etc);
oal_module_symbol(hmac_config_set_shortgi40_etc);
oal_module_symbol(hmac_config_set_shortgi80_etc);
oal_module_symbol(hmac_config_get_shortgi20_etc);
oal_module_symbol(hmac_config_get_shortgi40_etc);
oal_module_symbol(hmac_config_get_shortgi80_etc);
#ifdef _PRE_WLAN_FEATURE_MONITOR
oal_module_symbol(hmac_config_set_addr_filter);
#endif
oal_module_symbol(hmac_config_get_addr_filter_etc);
oal_module_symbol(hmac_config_set_prot_mode_etc);
oal_module_symbol(hmac_config_get_prot_mode_etc);
oal_module_symbol(hmac_config_set_auth_mode_etc);
oal_module_symbol(hmac_config_get_auth_mode_etc);
oal_module_symbol(hmac_config_set_max_user_etc);
oal_module_symbol(hmac_config_set_bintval_etc);
oal_module_symbol(hmac_config_get_bintval_etc);
oal_module_symbol(hmac_config_set_nobeacon_etc);
oal_module_symbol(hmac_config_get_nobeacon_etc);
oal_module_symbol(hmac_config_set_txpower_etc);
oal_module_symbol(hmac_config_get_txpower_etc);
#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
oal_module_symbol(hmac_config_chip_check);
#endif
oal_module_symbol(hmac_config_set_freq_etc);
oal_module_symbol(hmac_config_get_freq_etc);
oal_module_symbol(hmac_config_get_wmm_params_etc);
oal_module_symbol(hmac_config_set_wmm_params_etc);
oal_module_symbol(hmac_config_vap_info_etc);
#ifdef _PRE_WLAN_FEATURE_NEGTIVE_DET
oal_module_symbol(hmac_config_pk_mode_debug);
#endif
oal_module_symbol(hmac_config_user_info_etc);
oal_module_symbol(hmac_config_add_user_etc);
oal_module_symbol(hmac_config_del_user_etc);
oal_module_symbol(hmac_config_addba_req_etc);
oal_module_symbol(hmac_config_set_tx_pow_param);
oal_module_symbol(hmac_config_set_dscr_param_etc);
oal_module_symbol(hmac_config_set_rate_etc);
oal_module_symbol(hmac_config_log_level_etc);
oal_module_symbol(hmac_config_set_mcs_etc);
oal_module_symbol(hmac_config_set_mcsac_etc);
#ifdef _PRE_WLAN_FEATURE_11AX
oal_module_symbol(hmac_config_set_mcsax);
#endif
oal_module_symbol(hmac_config_set_nss);
oal_module_symbol(hmac_config_set_rfch_etc);
oal_module_symbol(hmac_config_set_bw_etc);
oal_module_symbol(hmac_config_always_rx_etc);
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
oal_module_symbol(hmac_config_always_tx_51);
oal_module_symbol(hmac_config_always_rx_51);
#endif
#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_set_rxch);
oal_module_symbol(hmac_config_dync_txpower);
oal_module_symbol(hmac_config_dync_pow_debug_switch);
#endif
oal_module_symbol(hmac_config_connect_etc);
oal_module_symbol(hmac_config_get_thruput);
oal_module_symbol(hmac_config_set_freq_skew);
oal_module_symbol(hmac_config_adjust_ppm);
oal_module_symbol(hmac_config_get_ppm);
oal_module_symbol(hmac_config_pcie_pm_level_etc);
oal_module_symbol(hmac_config_delba_req_etc);
oal_module_symbol(hmac_config_ampdu_end_etc);
oal_module_symbol(hmac_config_ampdu_start_etc);
#ifdef _PRE_WLAN_CHIP_FPGA_PCIE_TEST
oal_module_symbol(hmac_config_pcie_test);
#endif
oal_module_symbol(hmac_config_event_switch_etc);
oal_module_symbol(hmac_config_profiling_switch_etc);
oal_module_symbol(hmac_config_amsdu_start_etc);
oal_module_symbol(hmac_config_auto_ba_switch_etc);
oal_module_symbol(hmac_config_list_sta_etc);
oal_module_symbol(hmac_config_get_sta_list_etc);
oal_module_symbol(hmac_config_list_ap_etc);
oal_module_symbol(hmac_config_send_bar);
oal_module_symbol(hmac_config_pause_tid_etc);
oal_module_symbol(hmac_config_dump_timer);
oal_module_symbol(hmac_config_set_user_vip);
oal_module_symbol(hmac_config_set_vap_host);
oal_module_symbol(hmac_config_set_dtimperiod_etc);
oal_module_symbol(hmac_config_get_dtimperiod_etc);
oal_module_symbol(hmac_config_alg_param_etc);
oal_module_symbol(hmac_config_hide_ssid);
oal_module_symbol(hmac_config_set_amsdu_tx_on_etc);
oal_module_symbol(hmac_config_set_ampdu_tx_on_etc);
oal_module_symbol(hmac_config_get_amsdu_tx_on);
oal_module_symbol(hmac_config_get_ampdu_tx_on);
oal_module_symbol(hmac_config_get_country_etc);
oal_module_symbol(hmac_config_set_country_etc);
oal_module_symbol(hmac_config_set_country_for_dfs_etc);
oal_module_symbol(hmac_config_amsdu_ampdu_switch_etc);
oal_module_symbol(hmac_config_reset_hw);
/*oal_module_symbol(hmac_config_reset_operate_etc);*/
oal_module_symbol(hmac_config_dump_rx_dscr_etc);
oal_module_symbol(hmac_config_dump_tx_dscr_etc);
oal_module_symbol(hmac_config_set_channel_etc);
oal_module_symbol(hmac_config_set_mib_by_bw);
oal_module_symbol(hmac_config_set_beacon_etc);
oal_module_symbol(hmac_config_get_assoc_req_ie_etc);
oal_module_symbol(hmac_config_set_app_ie_to_vap_etc);
oal_module_symbol(hmac_config_set_wps_p2p_ie_etc);
oal_module_symbol(hmac_config_set_wps_ie_etc);
oal_module_symbol(hmac_config_list_channel_etc);
#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
oal_module_symbol(hmac_config_load_ini_power_gain);
oal_module_symbol(hmac_config_dev_customize_info_etc);
oal_module_symbol(hmac_config_set_cus_nvram_params_etc);
oal_module_symbol(hmac_config_set_cus_dts_cali_etc);
oal_module_symbol(hmac_config_set_cus_rf_etc);
oal_module_symbol(hmac_config_set_all_log_level_etc);
oal_module_symbol(hmac_config_set_cus_dyn_cali);
#endif //#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
#ifdef _PRE_WLAN_FEATURE_SINGLE_CHIP_DUAL_BAND
oal_module_symbol(hmac_config_set_restrict_band);
#endif
#if defined(_PRE_WLAN_FEATURE_DBAC) && defined(_PRE_WLAN_FEATRUE_DBAC_DOUBLE_AP_MODE)
oal_module_symbol(hmac_config_set_omit_acs);
#endif
oal_module_symbol(hmac_config_set_regdomain_pwr_etc);
#ifdef _PRE_WLAN_FEATURE_TPC_OPT
oal_module_symbol(hmac_config_reduce_sar_etc);
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
oal_module_symbol(hmac_config_tas_pwr_ctrl);
#endif
#endif
#ifdef _PRE_WLAN_FEATURE_TAS_ANT_SWITCH
oal_module_symbol(hmac_config_tas_rssi_access);
#endif
oal_module_symbol(hmac_config_reg_write_etc);
#ifdef _PRE_WLAN_FEATURE_11D
oal_module_symbol(hmac_config_set_rd_by_ie_switch_etc);
#endif
#if (defined(_PRE_PRODUCT_ID_HI110X_DEV) || defined(_PRE_PRODUCT_ID_HI110X_HOST))
oal_module_symbol(hmac_config_sdio_flowctrl_etc);
#endif
oal_module_symbol(hmac_config_reg_info_etc);
oal_module_symbol(hmac_config_dump_all_rx_dscr_etc);
oal_module_symbol(hmac_config_alg_etc);
#ifdef _PRE_FEATURE_FAST_AGING
oal_module_symbol(hmac_config_fast_aging);
oal_module_symbol(hmac_config_get_fast_aging);
#endif
#ifdef _PRE_WLAN_FEATURE_CAR
oal_module_symbol(hmac_config_car_cfg);
#endif
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
oal_module_symbol(hmac_config_waveapp_32plus_user_enable);
#endif
oal_module_symbol(hmac_config_rssi_limit);

#ifdef _PRE_WLAN_PRODUCT_1151V200
oal_module_symbol(hmac_config_80m_rts_debug);
#endif
oal_module_symbol(hmac_config_send_event_etc);
oal_module_symbol(hmac_config_sync_cmd_common_etc);
oal_module_symbol(hmac_config_open_wmm);
oal_module_symbol(hmac_config_beacon_chain_switch);
#if 0
oal_module_symbol(hmac_config_tdls_prohibited);
oal_module_symbol(hmac_config_tdls_channel_switch_prohibited);
#endif
oal_module_symbol(hmac_config_set_2040_coext_support_etc);
oal_module_symbol(hmac_config_rx_fcs_info_etc);
oal_module_symbol(hmac_config_get_tid_etc);
oal_module_symbol(hmac_config_dump_ba_bitmap);
oal_module_symbol(hmac_config_eth_switch_etc);
oal_module_symbol(hmac_config_80211_ucast_switch_etc);

oal_module_symbol(hmac_config_80211_mcast_switch_etc);
oal_module_symbol(hmac_config_probe_switch_etc);
oal_module_symbol(hmac_config_get_mpdu_num_etc);
oal_module_symbol(hmac_config_set_thruput_bypass);
#if 0
oal_module_symbol(hmac_config_ota_switch);
#endif
#ifdef _PRE_WLAN_CHIP_TEST
oal_module_symbol(hmac_test_send_action);
oal_module_symbol(hmac_config_send_pspoll);
oal_module_symbol(hmac_config_send_nulldata);
oal_module_symbol(hmac_config_beacon_offload_test);
#endif
#if (_PRE_WLAN_FEATURE_PMF != _PRE_PMF_NOT_SUPPORT)
oal_module_symbol(hmac_enable_pmf_etc);
#endif
oal_module_symbol(hmac_config_ota_beacon_switch_etc);
oal_module_symbol(hmac_config_ota_rx_dscr_switch_etc);
oal_module_symbol(hmac_config_set_all_ota_etc);
oal_module_symbol(hmac_config_oam_output_etc);
oal_module_symbol(hmac_config_set_dhcp_arp_switch_etc);
oal_module_symbol(hmac_config_vap_pkt_stat_etc);

#ifdef _PRE_WLAN_FEATURE_DHCP_REQ_DISABLE
oal_module_symbol(hmac_config_set_dhcp_req_disable);
#endif
#ifdef _PRE_DEBUG_MODE_USER_TRACK
oal_module_symbol(hmac_config_report_thrput_stat);
#endif

#ifdef _PRE_WLAN_FEATURE_DAQ
oal_module_symbol(hmac_config_data_acq);
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
oal_module_symbol(hmac_config_resume_rx_intr_fifo);
#endif

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
oal_module_symbol(hmac_config_tcp_ack_buf);
#endif

#ifdef _PRE_WLAN_FEATURE_SMPS
//oal_module_symbol(hmac_config_get_smps_mode_en);
oal_module_symbol(hmac_config_set_smps_mode);
oal_module_symbol(hmac_config_set_vap_smps_mode);
#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_get_smps_info);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_DFR
#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_dfr_enable);
oal_module_symbol(hmac_config_trig_pcie_reset);
oal_module_symbol(hmac_config_trig_loss_tx_comp);
#endif
#endif

#ifdef _PRE_WLAN_CHIP_TEST
oal_module_symbol(hmac_config_lpm_tx_data);
oal_module_symbol(hmac_config_set_coex);
oal_module_symbol(hmac_config_set_dfx);
oal_module_symbol(hmac_config_clear_all_stat);
#endif

#ifdef _PRE_WLAN_PERFORM_STAT
oal_module_symbol(hmac_config_pfm_stat);
oal_module_symbol(hmac_config_pfm_display);
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
oal_module_symbol(hmac_config_proxysta_switch);
oal_module_symbol(hmac_config_set_oma);
#endif

#ifdef _PRE_WLAN_FEATURE_DFS
oal_module_symbol(hmac_config_dfs_radartool_etc);
#endif   /* end of _PRE_WLAN_FEATURE_DFS */

#ifdef _PRE_SUPPORT_ACS
oal_module_symbol(hmac_config_acs);
oal_module_symbol(hmac_config_chan_stat);
#endif

#ifdef _PRE_WLAN_FEATURE_BAND_STEERING
oal_module_symbol(hmac_config_bsd);
#ifdef _PRE_WLAN_WEB_CMD_COMM
oal_module_symbol(hmac_config_get_bsd);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11V
oal_module_symbol(hmac_11v_cfg_wl_mgmt_switch);
oal_module_symbol(hmac_11v_cfg_bsst_switch);
#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_11v_sta_tx_query);
oal_module_symbol(hmac_11v_ap_tx_request);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_UAPSD
oal_module_symbol(hmac_config_get_uapsden_etc);
oal_module_symbol(hmac_config_set_uapsden_etc);
#endif
#ifdef _PRE_WLAN_FEATURE_OPMODE_NOTIFY
oal_module_symbol(hmac_config_set_opmode_notify_etc);
oal_module_symbol(hmac_config_get_user_rssbw_etc);
#endif
#ifdef _PRE_WLAN_FEATURE_M2S
oal_module_symbol(hmac_config_set_m2s_switch);
#endif
#ifdef _PRE_WLAN_FEATURE_TXOPPS
oal_module_symbol(hmac_config_set_txop_ps_machw);
#endif
#ifdef _PRE_WLAN_FEATURE_BTCOEX
oal_module_symbol(hmac_config_print_btcoex_status_etc);
oal_module_symbol(hmac_config_btcoex_preempt_tpye);
#endif
#ifdef _PRE_WLAN_FEATURE_LTECOEX
oal_module_symbol(hmac_config_ltecoex_mode_set);
#endif
oal_module_symbol(hmac_config_set_vap_nss);

#ifdef _PRE_DEBUG_MODE
oal_module_symbol(hmac_config_get_all_reg_value);
oal_module_symbol(hmac_config_get_cali_data);
oal_module_symbol(hmac_config_report_ampdu_stat);
oal_module_symbol(hmac_config_scan_test);

#endif
oal_module_symbol(hmac_config_bgscan_enable_etc);
oal_module_symbol(hmac_config_set_ampdu_aggr_num_etc);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
oal_module_symbol(hmac_config_set_ampdu_mmss);
#endif
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE) && defined(_PRE_DEBUG_MODE)
oal_module_symbol(hmac_config_freq_adjust);
#endif
#ifdef _PRE_WLAN_FEATURE_PSD_ANALYSIS
oal_module_symbol(hmac_config_set_psd_cap);
oal_module_symbol(hmac_config_cfg_psd);

#endif
#ifdef _PRE_WLAN_FEATURE_CSI
oal_module_symbol(hmac_config_set_csi);
#endif
oal_module_symbol(hmac_config_set_stbc_cap_etc);
oal_module_symbol(hmac_config_set_ldpc_cap_etc);
oal_module_symbol(hmac_config_set_txbf_cap);

#ifdef _PRE_WLAN_DFT_STAT
oal_module_symbol(hmac_config_set_phy_stat_en);
oal_module_symbol(hmac_config_dbb_env_param);
oal_module_symbol(hmac_config_usr_queue_stat_etc);
oal_module_symbol(hmac_config_report_vap_stat);
oal_module_symbol(hmac_config_report_all_stat);
#endif

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP
oal_module_symbol(hmac_config_set_edca_opt_cycle_ap_etc);
oal_module_symbol(hmac_config_set_edca_opt_switch_ap_etc);
oal_module_symbol(hmac_config_set_edca_opt_weight_sta_etc);
oal_module_symbol(hmac_config_set_edca_opt_switch_sta_etc);
#endif

#ifdef _PRE_WLAN_FEATURE_AP_PM
oal_module_symbol(hmac_config_wifi_enable);
oal_module_symbol(hmac_config_sta_scan_wake_wow);
#endif
oal_module_symbol(hmac_config_remain_on_channel_etc);
oal_module_symbol(hmac_config_cancel_remain_on_channel_etc);

oal_module_symbol(hmac_config_vap_classify_en_etc);
oal_module_symbol(hmac_config_vap_classify_tid_etc);
oal_module_symbol(hmac_config_always_tx);
oal_module_symbol(hmac_config_always_tx_num);
oal_module_symbol(hmac_config_always_tx_hw);
oal_module_symbol(hmac_config_scan_abort_etc);
#if defined(_PRE_WLAN_FEATURE_EQUIPMENT_TEST) && (defined _PRE_WLAN_FIT_BASED_REALTIME_CALI)
oal_module_symbol(hmac_config_cali_power);
oal_module_symbol(hmac_config_get_cali_power);
oal_module_symbol(hmac_config_set_polynomial_param);
oal_module_symbol(hmac_config_get_upc_params);
oal_module_symbol(hmac_config_set_upc_params);
oal_module_symbol(hmac_config_set_load_mode);
#endif

oal_module_symbol(hmac_config_get_dieid);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION) && defined(_PRE_WLAN_CHIP_TEST_ALG)
oal_module_symbol(hmac_alg_test_result_process);
oal_module_symbol(hmac_alg_test_main_common_init);
oal_module_symbol(hmac_alg_test_main_common_exit);
#endif

#ifdef _PRE_WLAN_FEATURE_HS20
oal_module_symbol(hmac_config_set_qos_map);
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
oal_module_symbol(hmac_config_set_p2p_ps_ops_etc);
oal_module_symbol(hmac_config_set_p2p_ps_noa_etc);
oal_module_symbol(hmac_config_set_p2p_ps_stat);
#endif

#ifdef _PRE_WLAN_PROFLING_MIPS
oal_module_symbol(hmac_config_set_mips);
oal_module_symbol(hmac_config_show_mips);
#endif


#ifdef _PRE_WLAN_FEATURE_ARP_OFFLOAD
oal_module_symbol(hmac_config_enable_arp_offload);
oal_module_symbol(hmac_config_set_ip_addr_etc);
oal_module_symbol(hmac_config_show_arpoffload_info);
#endif
oal_module_symbol(hmac_config_get_fem_pa_status_etc);
#ifdef _PRE_WLAN_FEATURE_ROAM
oal_module_symbol(hmac_config_roam_enable_etc);
oal_module_symbol(hmac_config_roam_start_etc);
oal_module_symbol(hmac_config_roam_band_etc);
oal_module_symbol(hmac_config_roam_org_etc);
oal_module_symbol(hmac_config_roam_info_etc);
#endif //_PRE_WLAN_FEATURE_ROAM

#ifdef _PRE_WLAN_FEATURE_STA_PM
oal_module_symbol(hmac_config_set_pm_by_module_etc);
#endif //_PRE_WLAN_FEATURE_STA_PM

#ifdef _PRE_WLAN_FEATURE_20_40_80_COEXIST
oal_module_symbol(hmac_config_enable_2040bss_etc);
oal_module_symbol(hmac_config_get_2040bss_sw);
#endif

#ifdef _PRE_WLAN_FEATURE_HILINK
oal_module_symbol(hmac_config_fbt_scan_list_clear);
oal_module_symbol(hmac_config_fbt_scan_specified_sta);
oal_module_symbol(hmac_config_fbt_start_scan);
oal_module_symbol(hmac_config_fbt_print_scan_list);
oal_module_symbol(hmac_config_fbt_scan_interval);
oal_module_symbol(hmac_config_fbt_scan_channel);
oal_module_symbol(hmac_config_fbt_scan_report_period);
oal_module_symbol(hmac_config_get_cur_channel);
oal_module_symbol(hmac_config_get_sta_11v_abillty);
oal_module_symbol(hmac_config_set_stay_time);
oal_module_symbol(hmac_config_set_white_lst_ssidhiden);
oal_module_symbol(hmac_config_fbt_rej_user);
oal_module_symbol(hmac_config_fbt_scan_enable);
oal_module_symbol(hmac_config_change_to_other_ap);
#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
oal_module_symbol(hmac_config_get_sta_11k_abillty);
oal_module_symbol(hmac_config_set_sta_bcn_request);
#endif
#ifdef _PRE_WLAN_FEATURE_HILINK_HERA_PRODUCT
oal_module_symbol(hmac_config_set_mgmt_frame_filters);
oal_module_symbol(hmac_config_get_all_sta_diag_info);
oal_module_symbol(hmac_config_get_vap_diag_info);
oal_module_symbol(hmac_config_get_sta_diag_info_rsp);
oal_module_symbol(hmac_config_get_vap_diag_info_rsp);
oal_module_symbol(hmac_config_set_sensing_bssid);
oal_module_symbol(hmac_config_get_sensing_bssid_info);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_11KV_INTERFACE
oal_module_symbol(hmac_config_send_action_frame);
oal_module_symbol(hmac_config_set_mgmt_frame_ie);
oal_module_symbol(hmac_config_set_mgmt_cap_info);
#endif

oal_module_symbol(hmac_config_get_sta_11h_abillty);
oal_module_symbol(hmac_config_set_vendor_ie);
#ifdef _PRE_WLAN_FEATURE_11R_AP
oal_module_symbol(hmac_config_get_sta_11r_abillty);
oal_module_symbol(hmac_config_set_mlme);
#endif
#ifdef _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN
oal_module_symbol(hmac_config_set_tx_classify_switch_etc);
#endif  /* _PRE_WLAN_FEATURE_TX_CLASSIFY_LAN_TO_WLAN */

#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)
oal_module_symbol(hmac_config_set_txrx_chain);
#ifdef _PRE_WLAN_RF_CALI
oal_module_symbol(hmac_config_set_2g_txrx_path);
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_EQUIPMENT_TEST
oal_module_symbol(hmac_config_send_cw_signal);
oal_module_symbol(hmac_config_get_cali_info);
#endif

oal_module_symbol(hmac_config_query_rssi_etc);
oal_module_symbol(hmac_config_query_rate_etc);
oal_module_symbol(hmac_config_query_psst);

#ifdef _PRE_WLAN_DFT_STAT
oal_module_symbol(hmac_config_query_ani_etc);
#endif
oal_module_symbol(hmac_config_vendor_cmd_get_channel_list_etc);
#ifdef _PRE_WLAN_FEATURE_SMARTANT
oal_module_symbol(hmac_config_get_ant_info);
oal_module_symbol(hmac_config_double_ant_switch);
#endif

#if (defined _PRE_WLAN_RF_CALI) || (defined _PRE_WLAN_RF_CALI_1151V2)
oal_module_symbol(hmac_config_auto_cali);
oal_module_symbol(hmac_config_get_cali_status);
oal_module_symbol(hmac_config_set_cali_vref);
#endif

#ifdef _PRE_WLAN_FIT_BASED_REALTIME_CALI
oal_module_symbol(hmac_config_dyn_cali_param);
#endif

oal_module_symbol(hmac_config_query_station_info_etc);
#ifdef _PRE_WLAN_FEATURE_USER_EXTEND
oal_module_symbol(hmac_config_user_extend_enable);
#endif

#ifdef _PRE_WLAN_FEATURE_WDS
oal_module_symbol(hmac_config_wds_get_vap_mode);
oal_module_symbol(hmac_config_wds_vap_mode);
oal_module_symbol(hmac_config_wds_vap_show);
oal_module_symbol(hmac_config_wds_sta_add);
oal_module_symbol(hmac_config_wds_sta_del);
oal_module_symbol(hmac_config_wds_sta_age);
oal_module_symbol(hmac_config_wds_get_sta_num);
#endif

oal_module_symbol(hmac_config_set_tlv_cmd);
oal_module_symbol(hmac_config_voe_enable_etc);

#ifdef _PRE_WLAN_FEATURE_PACKET_CAPTURE
oal_module_symbol(hmac_config_packet_capture_switch);
#endif
#ifdef _PRE_WLAN_11K_STAT
oal_module_symbol(hmac_config_query_stat_info);
#endif
oal_module_symbol(hmac_config_set_priv_flag);
oal_module_symbol(hmac_config_set_bw_fixed);
oal_module_symbol(hmac_config_get_wmmswitch);
oal_module_symbol(hmac_config_get_vap_wmm_switch);
oal_module_symbol(hmac_config_set_vap_wmm_switch);
oal_module_symbol(hmac_config_get_max_user);
oal_module_symbol(hmac_config_dscp_map_to_tid);
oal_module_symbol(hmac_config_clean_dscp_tid_map);
oal_module_symbol(hmac_config_bg_noise_info);
#ifdef _PRE_WLAN_WEB_CMD_COMM
oal_module_symbol(hmac_config_set_hw_addr);
oal_module_symbol(hmac_config_get_hide_ssid);
oal_module_symbol(hmac_config_set_global_shortgi);
oal_module_symbol(hmac_config_get_global_shortgi);
oal_module_symbol(hmac_config_get_txbeamform);
oal_module_symbol(hmac_config_set_txbeamform);
oal_module_symbol(hmac_config_set_noforward);
oal_module_symbol(hmac_config_get_noforward);
oal_module_symbol(hmac_config_get_channel);
oal_module_symbol(hmac_config_priv_set_mode);
oal_module_symbol(hmac_config_priv_set_channel);
oal_module_symbol(hmac_config_get_assoc_num);
oal_module_symbol(hmac_config_get_noise);
oal_module_symbol(hmac_config_get_bw);
oal_module_symbol(hmac_config_get_sta_rssi);
oal_module_symbol(hmac_config_get_pmf);
oal_module_symbol(hmac_config_get_temp);
oal_module_symbol(hmac_config_get_ko_version);
oal_module_symbol(hmac_config_get_wps_ie);
oal_module_symbol(hmac_config_get_band);
oal_module_symbol(hmac_config_get_rate_info);
oal_module_symbol(hmac_config_get_chan_list);
oal_module_symbol(hmac_config_get_pwr_ref);
oal_module_symbol(hmac_config_get_vap_cap);
oal_module_symbol(hmac_config_neighbor_scan);
oal_module_symbol(hmac_config_get_scan_stat);
oal_module_symbol(hmac_config_get_neighb_no);
oal_module_symbol(hmac_config_get_neighb_info);
oal_module_symbol(hmac_config_get_hw_flow_stat);
oal_module_symbol(hmac_config_get_wme_stat);
#ifdef _PRE_WLAN_11K_STAT
oal_module_symbol(hmac_config_get_sta_drop_num);
oal_module_symbol(hmac_config_get_sta_tx_delay);
oal_module_symbol(hmac_config_get_tx_delay_ac);
#endif
oal_module_symbol(hmac_config_get_bcast_rate);
oal_module_symbol(hmac_config_get_ant_rssi);
oal_module_symbol(hmac_config_ant_rssi_report);
#ifdef _PRE_WLAN_FEATURE_DFS
oal_module_symbol(hmac_config_get_dfs_chn_status);
#endif
#endif
#if defined(_PRE_WLAN_FEATURE_MCAST) || defined(_PRE_WLAN_FEATURE_HERA_MCAST)
oal_module_symbol(hmac_config_get_snoop_table);
#endif

#ifdef _PRE_WLAN_FEATURE_11K_EXTERN
oal_module_symbol(hmac_config_send_radio_meas_req);
oal_module_symbol(hmac_config_set_11k_switch);
#endif
#ifdef _PRE_FEATURE_WAVEAPP_CLASSIFY
oal_module_symbol(hmac_config_get_waveapp_flag);
#endif

/*lint +e578*//*lint +e19*/
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
