


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_fsm.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_scan.h"
#include "hmac_mgmt_sta.h"
#include "hmac_mgmt_ap.h"
#include "frw_ext_if.h"
#include "hmac_config.h"
#ifdef _PRE_WLAN_FEATURE_ROAM
#include "hmac_roam_main.h"
#endif //_PRE_WLAN_FEATURE_ROAM
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_FSM_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/* 全局状态机函数表 */
OAL_STATIC hmac_fsm_func   g_pa_hmac_ap_fsm_func[MAC_VAP_AP_STATE_BUTT][HMAC_FSM_AP_INPUT_TYPE_BUTT];
OAL_STATIC hmac_fsm_func   g_pa_hmac_sta_fsm_func[MAC_VAP_STA_STATE_BUTT][HMAC_FSM_STA_INPUT_TYPE_BUTT];



oal_void  hmac_fsm_change_state_etc(hmac_vap_stru *pst_hmac_vap, mac_vap_state_enum_uint8 en_vap_state)
{
    mac_cfg_mode_param_stru  st_cfg_mode;
    mac_vap_state_enum_uint8 en_old_state;
    oal_uint32 ul_ret;
    en_old_state = pst_hmac_vap->st_vap_base_info.en_vap_state;

    /*将vap状态改变信息上报*/
    mac_vap_state_change_etc(&pst_hmac_vap->st_vap_base_info, en_vap_state);

    ul_ret = hmac_config_vap_state_syn_etc(&(pst_hmac_vap->st_vap_base_info), sizeof(en_vap_state), (oal_uint8 *)(&en_vap_state));
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG3(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC,
                       "{hmac_fsm_change_state_etc::hmac_syn_vap_state failed[%d], old_state=%d, new_state=%d.}",
                       ul_ret, en_old_state, en_vap_state);
    }

#ifdef _PRE_WLAN_FEATURE_STA_PM
    if((MAC_VAP_STATE_UP == en_vap_state)
       && (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA) && (OAL_FALSE == pst_hmac_vap->st_ps_sw_timer.en_is_registerd))
    {
        FRW_TIMER_CREATE_TIMER(&(pst_hmac_vap->st_ps_sw_timer),
                   hmac_set_ipaddr_timeout_etc,
                   HMAC_SWITCH_STA_PSM_PERIOD,
                   (void *)pst_hmac_vap,
                   OAL_FALSE,
                   OAM_MODULE_ID_HMAC,
                   pst_hmac_vap->st_vap_base_info.ul_core_id);

        pst_hmac_vap->us_check_timer_pause_cnt = 0;
    }
#endif

    if ((MAC_VAP_STATE_STA_FAKE_UP == en_vap_state)
        && (pst_hmac_vap->st_vap_base_info.en_vap_mode == WLAN_VAP_MODE_BSS_STA))
    {
#ifdef _PRE_WLAN_FEATURE_STA_PM
        if (OAL_TRUE == pst_hmac_vap->st_ps_sw_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_vap->st_ps_sw_timer);
        }
#endif
        st_cfg_mode.en_protocol  = pst_hmac_vap->st_preset_para.en_protocol;
        st_cfg_mode.en_band      = pst_hmac_vap->st_preset_para.en_band;
        st_cfg_mode.en_bandwidth = pst_hmac_vap->st_preset_para.en_bandwidth;

        pst_hmac_vap->st_vap_base_info.st_channel.uc_chan_number = 0;

        hmac_config_sta_update_rates_etc(&(pst_hmac_vap->st_vap_base_info), &st_cfg_mode, OAL_PTR_NULL);
    }
}

/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_uint32  hmac_fsm_null_fn_etc(hmac_vap_stru *pst_hmac_vap, oal_void *p_param)
{
    /* 什么都不做 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG0(0, OAM_SF_RX, "{wifi_wake_src:hmac_fsm_null_fn_etc::rcv mgmt frame,drop it}");
    }
#endif
#endif
    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_fsm_init_ap(oal_void)
{
    oal_uint32  ul_state;
    oal_uint32  ul_input;

    for (ul_state = 0; ul_state < MAC_VAP_AP_STATE_BUTT; ul_state++)
    {
        for (ul_input = 0; ul_input < HMAC_FSM_AP_INPUT_TYPE_BUTT; ul_input++)
        {
            g_pa_hmac_ap_fsm_func[ul_state][ul_input] = hmac_fsm_null_fn_etc;
        }
    }

    /* 接收管理帧输入
    +----------------------------------+---------------------
     | FSM State                        | FSM Function
     +----------------------------------+---------------------
     | MAC_VAP_STATE_INIT               | null_fn
     | MAC_VAP_STATE_UP                 | hmac_ap_up_rx_mgmt_etc
     | MAC_VAP_STATE_PAUSE              | null_fn
     +----------------------------------+---------------------
    */
    //g_pa_hmac_ap_fsm_func[MAC_VAP_STATE_AP_WAIT_START][HMAC_FSM_INPUT_RX_MGMT] = hmac_ap_wait_start_rx_mgmt;
    g_pa_hmac_ap_fsm_func[MAC_VAP_STATE_AP_WAIT_START][HMAC_FSM_INPUT_MISC]    = hmac_ap_wait_start_misc_etc;

    g_pa_hmac_ap_fsm_func[MAC_VAP_STATE_UP][HMAC_FSM_INPUT_RX_MGMT] = hmac_ap_up_rx_mgmt_etc;
    g_pa_hmac_ap_fsm_func[MAC_VAP_STATE_UP][HMAC_FSM_INPUT_MISC]    = hmac_ap_up_misc_etc;

    g_pa_hmac_ap_fsm_func[MAC_VAP_STATE_PAUSE][HMAC_FSM_INPUT_RX_MGMT] = hmac_ap_up_rx_mgmt_etc;
#if (_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1103_HOST)
    g_pa_hmac_ap_fsm_func[MAC_VAP_STATE_PAUSE][HMAC_FSM_INPUT_MISC]    = hmac_ap_wait_start_misc_etc;
#else
    g_pa_hmac_ap_fsm_func[MAC_VAP_STATE_PAUSE][HMAC_FSM_INPUT_MISC]    = hmac_ap_up_misc_etc;
#endif

}


OAL_STATIC oal_void hmac_fsm_init_sta(oal_void)
{
    oal_uint32  ul_state;

    /* 初始化扫描请求输入函数 HMAC_FSM_INPUT_SCAN_REQ
     +----------------------------------+---------------------
     | FSM State                        | FSM Function
     +----------------------------------+---------------------
     | MAC_VAP_STATE_INIT               | null_fn
     | MAC_VAP_STATE_UP                 | null_fn
     | MAC_VAP_STATE_FAKE_UP            | hmac_mgmt_scan_req
     | MAC_VAP_STATE_WAIT_SCAN          | null_fn
     | MAC_VAP_STATE_SCAN_COMP          | hmac_mgmt_scan_req
     | MAC_VAP_STATE_WAIT_JOIN          | null_fn
     | MAC_VAP_STATE_JOIN_COMP          | hmac_mgmt_scan_req
     | MAC_VAP_STATE_WAIT_AUTH_SEQ2     | null_fn
     | MAC_VAP_STATE_WAIT_AUTH_SEQ4     | null_fn
     | MAC_VAP_STATE_AUTH_COMP          | hmac_mgmt_scan_req
     | MAC_VAP_STATE_WAIT_ASOC          | null_fn
     | MAC_VAP_STATE_OBSS_SCAN          | null_fn
     | MAC_VAP_STATE_BG_SCAN            | null_fn
     +----------------------------------+---------------------
    */
    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_SCAN_REQ] = hmac_scan_proc_scan_req_event_exception_etc;
    }

    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_INIT][HMAC_FSM_INPUT_SCAN_REQ]   = hmac_scan_proc_scan_req_event_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_FAKE_UP][HMAC_FSM_INPUT_SCAN_REQ]   = hmac_scan_proc_scan_req_event_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_SCAN_COMP][HMAC_FSM_INPUT_SCAN_REQ] = hmac_scan_proc_scan_req_event_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_JOIN_COMP][HMAC_FSM_INPUT_SCAN_REQ] = hmac_scan_proc_scan_req_event_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_AUTH_COMP][HMAC_FSM_INPUT_SCAN_REQ] = hmac_scan_proc_scan_req_event_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_UP][HMAC_FSM_INPUT_SCAN_REQ]            = hmac_scan_proc_scan_req_event_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_LISTEN][HMAC_FSM_INPUT_SCAN_REQ]    = hmac_scan_proc_scan_req_event_etc;


    /* Initialize the elements in the JOIN_REQ input column. This input is   */
    /* valid only in the SCAN_COMP state.                                    */
    /*                                                                       */
    /* +------------------+---------------------+                            */
    /* | FSM State        | FSM Function        |                            */
    /* +------------------+---------------------+                            */
    /* | SCAN_COMP        | hmac_sta_wait_join_etc  |                            */
    /* | All other states | null_fn             |                            */
    /* +------------------+---------------------+                            */
    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_JOIN_REQ] = hmac_fsm_null_fn_etc;
    }

    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_FAKE_UP][HMAC_FSM_INPUT_JOIN_REQ]   = hmac_sta_wait_join_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_SCAN_COMP][HMAC_FSM_INPUT_JOIN_REQ] = hmac_sta_wait_join_etc;

    /* Initialize the elements in the AUTH_REQ input column. This input is   */
    /* valid only in the JOIN_COMP state.                                    */
    /*                                                                       */
    /* +------------------+---------------------+                            */
    /* | FSM State        | FSM Function        |                            */
    /* +------------------+---------------------+                            */
    /* | JOIN_COMP        | hmac_sta_wait_auth_etc  |                            */
    /* | All other states | null_fn             |                            */
    /* +------------------+---------------------+                            */

    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_AUTH_REQ] = hmac_fsm_null_fn_etc;
    }

    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_JOIN_COMP][HMAC_FSM_INPUT_AUTH_REQ] = hmac_sta_wait_auth_etc;


    /* Initialize the elements in the ASOC_REQ input column. This input is   */
    /* valid only in the AUTH_COMP state.                                    */
    /*                                                                       */
    /* +------------------+---------------------+                            */
    /* | FSM State        | FSM Function        |                            */
    /* +------------------+---------------------+                            */
    /* | AUTH_COMP        | hmac_sta_wait_asoc_etc       |                            */
    /* | All other states | null_fn             |                            */
    /* +------------------+---------------------+                            */

    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_ASOC_REQ] = hmac_fsm_null_fn_etc;
    }

    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_AUTH_COMP][HMAC_FSM_INPUT_ASOC_REQ] = hmac_sta_wait_asoc_etc;


    /* Initialize the elements in the HMAC_FSM_INPUT_RX_MGMT input column. The   */
    /* functions for all the states with this input is listed below.         */
    /*                                                                       */
    /*
     +----------------------------------+---------------------
     | FSM State                        | FSM Function
     +----------------------------------+---------------------
     | MAC_VAP_STATE_INIT               | null_fn
     | MAC_VAP_STATE_UP                 | hmac_sta_up_rx_mgmt_etc
     | MAC_VAP_STATE_FAKE_UP            | null_fn
     | MAC_VAP_STATE_WAIT_SCAN          | hmac_sta_wait_scan_rx
     | MAC_VAP_STATE_SCAN_COMP          | null_fn
     | MAC_VAP_STATE_WAIT_JOIN          | hmac_sta_wait_join_rx_etc
     | MAC_VAP_STATE_JOIN_COMP          | null_fn
     | MAC_VAP_STATE_WAIT_AUTH_SEQ2     | hmac_sta_wait_auth_seq2_rx_etc
     | MAC_VAP_STATE_WAIT_AUTH_SEQ4     | hmac_sta_wait_auth_seq4_rx_etc
     | MAC_VAP_STATE_AUTH_COMP          | null_fn
     | MAC_VAP_STATE_WAIT_ASOC          | hmac_sta_wait_asoc_rx_etc
     | MAC_VAP_STATE_OBSS_SCAN          | null_fn
     | MAC_VAP_STATE_BG_SCAN            | null_fn
     +----------------------------------+---------------------
    */

    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_RX_MGMT] = hmac_fsm_null_fn_etc;
    }

    /* 增加 HMAC_FSM_INPUT_RX_MGMT事件 处理函数 */
    /* g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_JOIN_COMP][HMAC_FSM_INPUT_RX_MGMT]      = hmac_sta_scan_comp_rx;*/
    //g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_JOIN][HMAC_FSM_INPUT_RX_MGMT]      = hmac_sta_wait_join_rx_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2][HMAC_FSM_INPUT_RX_MGMT] = hmac_sta_wait_auth_seq2_rx_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4][HMAC_FSM_INPUT_RX_MGMT] = hmac_sta_wait_auth_seq4_rx_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_ASOC][HMAC_FSM_INPUT_RX_MGMT]      = hmac_sta_wait_asoc_rx_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_UP][HMAC_FSM_INPUT_RX_MGMT]                 = hmac_sta_up_rx_mgmt_etc;

#ifdef _PRE_WLAN_FEATURE_ROAM
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_ROAMING][HMAC_FSM_INPUT_RX_MGMT]            = hmac_sta_roam_rx_mgmt_etc;
#endif //_PRE_WLAN_FEATURE_ROAM

    /* 增加 HotSpot中状态机sta侧处理函数 */
#if defined(_PRE_WLAN_FEATURE_HS20) || defined(_PRE_WLAN_FEATURE_P2P) || defined(_PRE_WLAN_FEATURE_HILINK)
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_SCAN_COMP][HMAC_FSM_INPUT_RX_MGMT]      = hmac_sta_not_up_rx_mgmt_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_SCAN][HMAC_FSM_INPUT_RX_MGMT]      = hmac_sta_not_up_rx_mgmt_etc;
#endif

#ifdef _PRE_WLAN_FEATURE_P2P
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_FAKE_UP][HMAC_FSM_INPUT_RX_MGMT]        = hmac_sta_not_up_rx_mgmt_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_LISTEN][HMAC_FSM_INPUT_RX_MGMT]         = hmac_sta_not_up_rx_mgmt_etc;
#endif

    /* 初始化timer0超时请求输入函数 HMAC_FSM_INPUT_TIMER0_OUT
     +----------------------------------+---------------------
     | FSM State                        | FSM Function
     +----------------------------------+---------------------
     | MAC_VAP_STATE_INIT               | null_fn
     | MAC_VAP_STATE_PAUSE              | null_fn
     | MAC_VAP_STATE_FAKE_UP            | null_fn
     | MAC_VAP_STATE_WAIT_SCAN          | null_fn
     | MAC_VAP_STATE_SCAN_COMP          | null_fn
     | MAC_VAP_STATE_WAIT_JOIN          | null_fn
     | MAC_VAP_STATE_JOIN_COMP          | null_fn
     | MAC_VAP_STATE_WAIT_AUTH_SEQ2     | null_fn
     | MAC_VAP_STATE_WAIT_AUTH_SEQ4     | null_fn
     | MAC_VAP_STATE_AUTH_COMP          | null_fn
     | MAC_VAP_STATE_WAIT_ASOC          | null_fn
     | MAC_VAP_STATE_OBSS_SCAN          | null_fn
     | MAC_VAP_STATE_BG_SCAN            | null_fn
     +----------------------------------+---------------------
    */
    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_TIMER0_OUT] = hmac_fsm_null_fn_etc;
    }

    //g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_JOIN][HMAC_FSM_INPUT_TIMER0_OUT]      = hmac_sta_wait_join_timeout_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2][HMAC_FSM_INPUT_TIMER0_OUT] = hmac_sta_auth_timeout_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4][HMAC_FSM_INPUT_TIMER0_OUT] = hmac_sta_auth_timeout_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_ASOC][HMAC_FSM_INPUT_TIMER0_OUT]      = hmac_sta_wait_asoc_timeout_etc;

    /* 初始化HMAC_FSM_INPUT_MISC事件请求的处理函数
       +----------------------------------+---------------------
       | FSM State                        | FSM Function
       +----------------------------------+---------------------
       | MAC_VAP_STATE_INIT               | null_fn
       | MAC_VAP_STATE_PAUSE              | null_fn
       | MAC_VAP_STATE_FAKE_UP            | null_fn
       | MAC_VAP_STATE_WAIT_SCAN          | null_fn
       | MAC_VAP_STATE_SCAN_COMP          | null_fn
       | MAC_VAP_STATE_WAIT_JOIN          | null_fn
       | MAC_VAP_STATE_JOIN_COMP          | null_fn
       | MAC_VAP_STATE_WAIT_AUTH_SEQ2     | null_fn
       | MAC_VAP_STATE_WAIT_AUTH_SEQ4     | null_fn
       | MAC_VAP_STATE_AUTH_COMP          | null_fn
       | MAC_VAP_STATE_WAIT_ASOC          | null_fn
       | MAC_VAP_STATE_OBSS_SCAN          | null_fn
       | MAC_VAP_STATE_BG_SCAN            | null_fn
       +----------------------------------+---------------------
      */
    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
      g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_MISC] = hmac_fsm_null_fn_etc;
    }

    //g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_WAIT_JOIN][HMAC_FSM_INPUT_MISC] = hmac_sta_wait_join_misc_etc;

#ifdef _PRE_WLAN_FEATURE_P2P
    /* Initialize the elements in the LISTEN_REQ input column. This input is */
    /* valid in the SCAN_COMP and FAKE_UP state.                             */
    /*                                                                       */
    /* +------------------+---------------------+                            */
    /* | FSM State        | FSM Function        |                            */
    /* +------------------+---------------------+                            */
    /* | FAKE_UP          | hmac_sta_wait_listen|                            */
    /* | SCAN_COMP        | hmac_sta_wait_listen|                            */
    /* | All other states | null_fn             |                            */
    /* +------------------+---------------------+                            */
    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_LISTEN_REQ] = hmac_fsm_null_fn_etc;
    }

    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_FAKE_UP][HMAC_FSM_INPUT_LISTEN_REQ]   = hmac_p2p_remain_on_channel_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_SCAN_COMP][HMAC_FSM_INPUT_LISTEN_REQ] = hmac_p2p_remain_on_channel_etc;
    
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_LISTEN][HMAC_FSM_INPUT_LISTEN_REQ]    = hmac_p2p_remain_on_channel_etc;//在监听状态接收到新的监听命令，则执行新的监听。
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_UP][HMAC_FSM_INPUT_LISTEN_REQ]            = hmac_p2p_remain_on_channel_etc;

    /* Initialize the elements in the HMAC_FSM_INPUT_LISTEN_TIMEOUT          */
    /* input column. This input is valid in MAC_VAP_STATE_STA_LISTEN state.  */
    /*                                                                       */
    /* +----------------------------+------------------------+               */
    /* | FSM State                  | FSM Function           |               */
    /* +----------------------------+------------------------+               */
    /* | MAC_VAP_STATE_STA_LISTEN   | hmac_p2p_listen_timeout_etc|               */
    /* | All other states           | null_fn                |               */
    /* +----------------------------+------------------------+               */

    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_LISTEN_TIMEOUT] = hmac_fsm_null_fn_etc;
    }
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_LISTEN][HMAC_FSM_INPUT_LISTEN_TIMEOUT] = hmac_p2p_listen_timeout_etc;

#endif  /* _PRE_WLAN_FEATURE_P2P */

    /* Initialize the elements in the SCHED_SCAN_REQ input column. This input is */
    /* valid in the SCAN_COMP and FAKE_UP state.                             */
    /*                                                                       */
    /* +------------------+------------------------------------+             */
    /* | FSM State        | FSM Function                       |             */
    /* +------------------+------------------------------------+             */
    /* | FAKE_UP          | hmac_scan_proc_sched_scan_req_event_etc|             */
    /* | SCAN_COMP        | hmac_scan_proc_sched_scan_req_event_etc|             */
    /* | All other states | null_fn                            |             */
    /* +------------------+------------------------------------+             */
    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
        g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_SCHED_SCAN_REQ] = hmac_fsm_null_fn_etc;
    }

    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_FAKE_UP][HMAC_FSM_INPUT_SCHED_SCAN_REQ]   = hmac_scan_proc_sched_scan_req_event_etc;
    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_STA_SCAN_COMP][HMAC_FSM_INPUT_SCHED_SCAN_REQ] = hmac_scan_proc_sched_scan_req_event_etc;
#ifdef _PRE_WLAN_FEATURE_ROAM
    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
      g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_ROAMING_START] = hmac_fsm_null_fn_etc;
    }

    g_pa_hmac_sta_fsm_func[MAC_VAP_STATE_UP][HMAC_FSM_INPUT_ROAMING_START]       = hmac_roam_pause_user_etc;

    for (ul_state = 0; ul_state < MAC_VAP_STA_STATE_BUTT; ul_state++)
    {
      g_pa_hmac_sta_fsm_func[ul_state][HMAC_FSM_INPUT_ROAMING_STOP] = hmac_roam_resume_user_etc;
    }
#endif //_PRE_WLAN_FEATURE_ROAM
}


oal_void  hmac_fsm_init_etc(oal_void)
{
    /* 初始化ap状态机函数表 */
    hmac_fsm_init_ap();

    /* 初始化sta状态机函数表 */
    hmac_fsm_init_sta();
}


oal_uint32  hmac_fsm_call_func_ap_etc(hmac_vap_stru *pst_hmac_vap, hmac_fsm_input_type_enum_uint8 en_input, oal_void *p_param)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_fsm_call_func_ap_etc::vap is null ptr.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (MAC_VAP_STATE_BUTT == pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                         "{hmac_fsm_call_func_ap_etc::the vap has been deleted, its state is STATE_BUTT, input type is %d}",
                         en_input);
        return OAL_SUCC;
    }
    else
#endif
    {
        if (pst_hmac_vap->st_vap_base_info.en_vap_state >= MAC_VAP_AP_STATE_BUTT ||
            en_input >= HMAC_FSM_AP_INPUT_TYPE_BUTT)
        {
            OAM_ERROR_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                            "{hmac_fsm_call_func_ap_etc::vap state[%d] or input type[%d] is over limit!}",
                            pst_hmac_vap->st_vap_base_info.en_vap_state, en_input);
            return OAL_ERR_CODE_ARRAY_OVERFLOW;
        }
    }

    return g_pa_hmac_ap_fsm_func[pst_hmac_vap->st_vap_base_info.en_vap_state][en_input](pst_hmac_vap, p_param);
}


oal_uint32  hmac_fsm_call_func_sta_etc(hmac_vap_stru *pst_hmac_vap, hmac_fsm_input_type_enum_uint8 en_input, oal_void *p_param)
{
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_fsm_call_func_sta_etc::vap is null ptr.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    if (MAC_VAP_STA_STATE_BUTT == pst_hmac_vap->st_vap_base_info.en_vap_state)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                         "{hmac_fsm_call_func_sta_etc::the vap has been deleted, its state is STATE_BUTT, input type is %d}",
                         en_input);
        return OAL_SUCC;
    }
    else
#endif
    {
        if (pst_hmac_vap->st_vap_base_info.en_vap_state >= MAC_VAP_STA_STATE_BUTT ||
            en_input >= HMAC_FSM_STA_INPUT_TYPE_BUTT)
        {
            OAM_ERROR_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY,
                            "{hmac_fsm_call_func_sta_etc::vap state[%d] or input type[%d] is over limit!}",
                            pst_hmac_vap->st_vap_base_info.en_vap_state, en_input);
            return OAL_ERR_CODE_ARRAY_OVERFLOW;
        }
    }

    return g_pa_hmac_sta_fsm_func[pst_hmac_vap->st_vap_base_info.en_vap_state][en_input](pst_hmac_vap, p_param);
}











#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

