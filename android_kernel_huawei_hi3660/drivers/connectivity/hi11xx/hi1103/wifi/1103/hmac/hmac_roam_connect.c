


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_ROAM

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oam_ext_if.h"
#include "mac_ie.h"
#include "mac_device.h"
#include "mac_resource.h"
#include "dmac_ext_if.h"
#include "hmac_fsm.h"
#include "hmac_sme_sta.h"
#include "hmac_resource.h"
#include "hmac_device.h"
#include "hmac_mgmt_sta.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_encap_frame_sta.h"
#include "hmac_tx_amsdu.h"
#include "hmac_rx_data.h"
#include "hmac_chan_mgmt.h"
#include "hmac_11i.h"
#include "hmac_roam_main.h"
#include "hmac_roam_connect.h"
#include "hmac_roam_alg.h"
#include "hmac_user.h"
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_ROAM_CONNECT_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
OAL_STATIC hmac_roam_fsm_func g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_BUTT][ROAM_CONNECT_FSM_EVENT_TYPE_BUTT];

OAL_STATIC oal_uint32  hmac_roam_connect_null_fn(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_start_join(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_send_auth_seq1(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
#ifdef _PRE_WLAN_FEATURE_11R
OAL_STATIC oal_uint32  hmac_roam_send_ft_req(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32 hmac_roam_process_ft_rsp(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
#endif //_PRE_WLAN_FEATURE_11R
OAL_STATIC oal_uint32  hmac_roam_process_auth_seq2(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_process_assoc_rsp(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_process_action(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_connect_succ(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32  hmac_roam_connect_fail(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32 hmac_roam_auth_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32 hmac_roam_assoc_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
OAL_STATIC oal_uint32 hmac_roam_handshaking_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
#ifdef _PRE_WLAN_FEATURE_11R
OAL_STATIC oal_uint32 hmac_roam_ft_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param);
#endif //_PRE_WLAN_FEATURE_11R
/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_void hmac_roam_connect_fsm_init_etc(oal_void)
{
    oal_uint32  ul_state;
    oal_uint32  ul_event;

    for (ul_state = 0; ul_state < ROAM_CONNECT_STATE_BUTT; ul_state++)
    {
        for (ul_event = 0; ul_event < ROAM_CONNECT_FSM_EVENT_TYPE_BUTT; ul_event++)
        {
            g_hmac_roam_connect_fsm_func[ul_state][ul_event] = hmac_roam_connect_null_fn;
        }
    }
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_INIT][ROAM_CONNECT_FSM_EVENT_START]                = hmac_roam_start_join;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_WAIT_AUTH_COMP][ROAM_CONNECT_FSM_EVENT_MGMT_RX]    = hmac_roam_process_auth_seq2;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_WAIT_AUTH_COMP][ROAM_CONNECT_FSM_EVENT_TIMEOUT]    = hmac_roam_auth_timeout;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_WAIT_ASSOC_COMP][ROAM_CONNECT_FSM_EVENT_MGMT_RX]   = hmac_roam_process_assoc_rsp;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_WAIT_ASSOC_COMP][ROAM_CONNECT_FSM_EVENT_TIMEOUT]   = hmac_roam_assoc_timeout;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_HANDSHAKING][ROAM_CONNECT_FSM_EVENT_MGMT_RX]       = hmac_roam_process_action;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_HANDSHAKING][ROAM_CONNECT_FSM_EVENT_KEY_DONE]      = hmac_roam_connect_succ;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_HANDSHAKING][ROAM_CONNECT_FSM_EVENT_TIMEOUT]       = hmac_roam_handshaking_timeout;
#ifdef _PRE_WLAN_FEATURE_11R
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_INIT][ROAM_CONNECT_FSM_EVENT_FT_OVER_DS]           = hmac_roam_send_ft_req;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_WAIT_FT_COMP][ROAM_CONNECT_FSM_EVENT_MGMT_RX]      = hmac_roam_process_ft_rsp;
    g_hmac_roam_connect_fsm_func[ROAM_CONNECT_STATE_WAIT_FT_COMP][ROAM_CONNECT_FSM_EVENT_TIMEOUT]      = hmac_roam_ft_timeout;
#endif //_PRE_WLAN_FEATURE_11R
}


oal_uint32 hmac_roam_connect_fsm_action_etc(hmac_roam_info_stru *pst_roam_info, roam_connect_fsm_event_type_enum en_event, oal_void *p_param)
{
    if (OAL_PTR_NULL == pst_roam_info)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_roam_info->st_connect.en_state >= ROAM_CONNECT_STATE_BUTT)
    {
        return OAL_ERR_CODE_ROAM_STATE_UNEXPECT;
    }

    if (en_event >= ROAM_CONNECT_FSM_EVENT_TYPE_BUTT)
    {
        return OAL_ERR_CODE_ROAM_EVENT_UXEXPECT;
    }

    return g_hmac_roam_connect_fsm_func[pst_roam_info->st_connect.en_state][en_event](pst_roam_info, p_param);
}


OAL_STATIC oal_void  hmac_roam_connect_change_state(hmac_roam_info_stru *pst_roam_info, roam_connect_state_enum_uint8 en_state)
{
    if (pst_roam_info)
    {
        OAM_WARNING_LOG2(0, OAM_SF_ROAM,
                      "{hmac_roam_connect_change_state::[%d]->[%d]}", pst_roam_info->st_connect.en_state, en_state);
        pst_roam_info->st_connect.en_state= en_state;
    }
}


OAL_STATIC oal_uint32  hmac_roam_connect_check_state(hmac_roam_info_stru *pst_roam_info,
                                                                mac_vap_state_enum_uint8     en_vap_state,
                                                                roam_main_state_enum_uint8 en_main_state,
                                                                roam_connect_state_enum_uint8 en_connect_state)
{
    if (pst_roam_info == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (pst_roam_info->pst_hmac_vap == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    if (pst_roam_info->pst_hmac_user == OAL_PTR_NULL)
    {
        return OAL_ERR_CODE_ROAM_INVALID_USER;
    }

    if (0 == pst_roam_info->uc_enable)
    {
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    if ((pst_roam_info->pst_hmac_vap->st_vap_base_info.en_vap_state != en_vap_state) ||
       (pst_roam_info->en_main_state != en_main_state) ||
       (pst_roam_info->st_connect.en_state != en_connect_state))
    {
        OAM_WARNING_LOG3(0, OAM_SF_ROAM, "{hmac_roam_connect_check_state::unexpect vap_state[%d] main_state[%d] connect_state[%d]!}",
                      pst_roam_info->pst_hmac_vap->st_vap_base_info.en_vap_state, pst_roam_info->en_main_state, pst_roam_info->st_connect.en_state);
        return OAL_ERR_CODE_ROAM_INVALID_VAP_STATUS;
    }

    return OAL_SUCC;
}



oal_uint32 hmac_roam_connect_timeout_etc(oal_void *p_arg)
{
    hmac_roam_info_stru *pst_roam_info;

    pst_roam_info = (hmac_roam_info_stru *)p_arg;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_timeout_etc::p_arg is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ROAM, "{hmac_roam_connect_timeout_etc::MAIN_STATE[%d] CONNECT_STATE[%d].}",
                     pst_roam_info->en_main_state, pst_roam_info->st_connect.en_state);

    return hmac_roam_connect_fsm_action_etc(pst_roam_info, ROAM_CONNECT_FSM_EVENT_TIMEOUT, OAL_PTR_NULL);
}

OAL_STATIC oal_uint32  hmac_roam_connect_null_fn(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_null_fn .}");

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
        if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
        {
            wlan_pm_wkup_src_debug_set(OAL_FALSE);
            OAM_WARNING_LOG0(0, OAM_SF_RX, "{wifi_wake_src:hmac_roam_connect_null_fn::rcv mgmt frame,drop it}");
        }
#endif
#endif

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_roam_connect_start_timer(hmac_roam_info_stru *pst_roam_info, oal_uint32 ul_timeout)
{
    frw_timeout_stru    *pst_timer = &(pst_roam_info->st_connect.st_timer);

    OAM_INFO_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_start_timer [%d].}", ul_timeout);

    /* 启动认证超时定时器 */
    FRW_TIMER_CREATE_TIMER(pst_timer,
                           hmac_roam_connect_timeout_etc,
                           ul_timeout,
                           pst_roam_info,
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_roam_info->pst_hmac_vap->st_vap_base_info.ul_core_id);
}


OAL_STATIC oal_uint32  hmac_roam_connect_del_timer(hmac_roam_info_stru *pst_roam_info)
{
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_roam_info->st_connect.st_timer));
    return OAL_SUCC;
}


oal_uint32 hmac_roam_connect_set_join_reg_etc(mac_vap_stru *pst_mac_vap, hmac_user_stru *pst_hmac_user)
{
    frw_event_mem_stru               *pst_event_mem;
    frw_event_stru                   *pst_event;
    dmac_ctx_join_req_set_reg_stru   *pst_reg_params;
    hmac_join_req_stru                st_join_req;

    oal_memset(&st_join_req, 0, OAL_SIZEOF(hmac_join_req_stru));

    /* 抛事件DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_SET_REG到DMAC */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_set_join_reg_etc::pst_event_mem ALLOC FAIL, size = %d.}",
                       OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru));
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_SET_REG,
                       OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    pst_reg_params = (dmac_ctx_join_req_set_reg_stru *)pst_event->auc_event_data;

    /* 设置需要写入寄存器的BSSID信息 */
    oal_set_mac_addr(pst_reg_params->auc_bssid, pst_mac_vap->auc_bssid);

    /* 填写信道相关信息 */
    pst_reg_params->st_current_channel.uc_chan_number = pst_mac_vap->st_channel.uc_chan_number;
    pst_reg_params->st_current_channel.en_band        = pst_mac_vap->st_channel.en_band;
    pst_reg_params->st_current_channel.en_bandwidth   = pst_mac_vap->st_channel.en_bandwidth;
    pst_reg_params->st_current_channel.uc_chan_idx    = pst_mac_vap->st_channel.uc_chan_idx;

    /* 以old user信息塑造虚假的入网结构体，调用函数 */
    /* 填写速率相关信息 */
    st_join_req.st_bss_dscr.uc_num_supp_rates = pst_hmac_user->st_op_rates.uc_rs_nrates;
    oal_memcopy(st_join_req.st_bss_dscr.auc_supp_rates, pst_hmac_user->st_op_rates.auc_rs_rates, OAL_SIZEOF(oal_uint8) * WLAN_MAX_SUPP_RATES);
    st_join_req.st_bss_dscr.en_ht_capable    = pst_hmac_user->st_user_base_info.st_ht_hdl.en_ht_capable;
    st_join_req.st_bss_dscr.en_vht_capable   = pst_hmac_user->st_user_base_info.st_vht_hdl.en_vht_capable;
    hmac_sta_get_min_rate(&pst_reg_params->st_min_rate, &st_join_req);

    /* 设置dtim period信息 */
    pst_reg_params->us_beacon_period = (oal_uint16)mac_mib_get_BeaconPeriod(pst_mac_vap);

    /* 同步FortyMHzOperationImplemented */
    pst_reg_params->en_dot11FortyMHzOperationImplemented   = mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap);

    /* 设置beacon filter关闭 */
    pst_reg_params->ul_beacon_filter = OAL_FALSE;

    /* 设置no frame filter打开 */
    pst_reg_params->ul_non_frame_filter = OAL_TRUE;

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32 hmac_roam_connect_set_dtim_param_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_dtim_cnt, oal_uint8 uc_dtim_period)
{
    frw_event_mem_stru               *pst_event_mem;
    frw_event_stru                   *pst_event;
    dmac_ctx_set_dtim_tsf_reg_stru   *pst_set_dtim_tsf_reg_params;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_set_dtim_param_etc, pst_mac_vap = NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 抛事件 DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG 到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_set_dtim_param_etc::pst_event_mem ALLOC FAIL, size = %d.}",
                       OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 填写事件 */
    pst_event = frw_get_event_stru(pst_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG,
                       OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    pst_set_dtim_tsf_reg_params = (dmac_ctx_set_dtim_tsf_reg_stru *)pst_event->auc_event_data;

    /* 将dtim相关参数抛到dmac */
    pst_set_dtim_tsf_reg_params->ul_dtim_cnt        = uc_dtim_cnt;
    pst_set_dtim_tsf_reg_params->ul_dtim_period     = uc_dtim_period;
    oal_memcopy (pst_set_dtim_tsf_reg_params->auc_bssid, pst_mac_vap->auc_bssid, WLAN_MAC_ADDR_LEN);
    pst_set_dtim_tsf_reg_params->us_tsf_bit0        = BIT0;

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  hmac_roam_connect_notify_wpas(hmac_roam_info_stru *pst_roam_info, oal_uint8 *puc_mac_hdr, oal_uint16 us_msg_len)
{
    hmac_asoc_rsp_stru               st_asoc_rsp;
    hmac_roam_rsp_stru               st_roam_rsp;
    frw_event_mem_stru              *pst_event_mem;
    frw_event_stru                  *pst_event;
    hmac_vap_stru                   *pst_hmac_vap;
    hmac_user_stru                  *pst_hmac_user;
    oal_uint8                       *puc_mgmt_data;

    oal_memset(&st_asoc_rsp, 0, OAL_SIZEOF(hmac_asoc_rsp_stru));
    oal_memset(&st_roam_rsp, 0, OAL_SIZEOF(hmac_roam_rsp_stru));
    /* 获取AP的mac地址 */
    mac_get_address3(puc_mac_hdr, st_roam_rsp.auc_bssid);
    mac_get_address3(puc_mac_hdr, st_asoc_rsp.auc_addr_ap);

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_notify_wpas, pst_hmac_user[%d] = NULL!}",
            pst_hmac_vap->st_vap_base_info.us_assoc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取关联请求帧信息 */
    st_roam_rsp.puc_asoc_req_ie_buff = pst_hmac_user->puc_assoc_req_ie_buff;
    st_roam_rsp.ul_asoc_req_ie_len   = pst_hmac_user->ul_assoc_req_ie_len;
    st_asoc_rsp.puc_asoc_req_ie_buff = pst_hmac_user->puc_assoc_req_ie_buff;
    st_asoc_rsp.ul_asoc_req_ie_len   = pst_hmac_user->ul_assoc_req_ie_len;

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_roam_rsp_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_handle_asoc_rsp_sta_etc::FRW_EVENT_ALLOC fail!}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 记录关联响应帧的部分内容，用于上报给内核 */
    if (us_msg_len < OAL_ASSOC_RSP_IE_OFFSET)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_handle_asoc_rsp_sta_etc::us_msg_len is too short, %d.}",us_msg_len);
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    puc_mgmt_data = (oal_uint8*)oal_memalloc(us_msg_len - OAL_ASSOC_RSP_IE_OFFSET);
    if(OAL_PTR_NULL == puc_mgmt_data)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_handle_asoc_rsp_sta_etc::pst_mgmt_data alloc null,size %d.}",(us_msg_len - OAL_ASSOC_RSP_IE_OFFSET));
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    st_roam_rsp.ul_asoc_rsp_ie_len   = us_msg_len - OAL_ASSOC_RSP_IE_OFFSET;
    oal_memcopy(puc_mgmt_data, (oal_uint8 *)(puc_mac_hdr + OAL_ASSOC_RSP_IE_OFFSET), st_roam_rsp.ul_asoc_rsp_ie_len);
    st_roam_rsp.puc_asoc_rsp_ie_buff = puc_mgmt_data;
    st_asoc_rsp.ul_asoc_rsp_ie_len = st_roam_rsp.ul_asoc_rsp_ie_len;
    st_asoc_rsp.puc_asoc_rsp_ie_buff = st_roam_rsp.puc_asoc_rsp_ie_buff;

    pst_event = frw_get_event_stru(pst_event_mem);

    if (!oal_memcmp(st_roam_rsp.auc_bssid, pst_roam_info->st_old_bss.auc_bssid, WLAN_MAC_ADDR_LEN))
    {
        /* Reassociation to the same BSSID: report NL80211_CMD_CONNECT event to supplicant instead of NL80211_CMD_ROAM event
         * in case supplicant ignore roam event to the same bssid which will cause 4-way handshake failure */
        /* wpa_supplicant: wlan0: WPA: EAPOL-Key Replay Counter did not increase - dropping packet */
        OAM_WARNING_LOG4(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_notify_wpas::roam_to the same bssid [%02X:XX:XX:%02X:%02X:%02X]}",
            st_roam_rsp.auc_bssid[0], st_roam_rsp.auc_bssid[3], st_roam_rsp.auc_bssid[4], st_roam_rsp.auc_bssid[5]);
        FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_HOST_CTX,
                           HMAC_HOST_CTX_EVENT_SUB_TYPE_ASOC_COMP_STA,
                           OAL_SIZEOF(hmac_asoc_rsp_stru),
                           FRW_EVENT_PIPELINE_STAGE_0,
                           pst_hmac_vap->st_vap_base_info.uc_chip_id,
                           pst_hmac_vap->st_vap_base_info.uc_device_id,
                           pst_hmac_vap->st_vap_base_info.uc_vap_id);
        oal_memcopy((oal_uint8 *)frw_get_event_payload(pst_event_mem), (oal_uint8 *)&st_asoc_rsp, OAL_SIZEOF(hmac_asoc_rsp_stru));
    }
    else
    {
        FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                           FRW_EVENT_TYPE_HOST_CTX,
                           HMAC_HOST_CTX_EVENT_SUB_TYPE_ROAM_COMP_STA,
                           OAL_SIZEOF(hmac_roam_rsp_stru),
                           FRW_EVENT_PIPELINE_STAGE_0,
                           pst_hmac_vap->st_vap_base_info.uc_chip_id,
                           pst_hmac_vap->st_vap_base_info.uc_device_id,
                           pst_hmac_vap->st_vap_base_info.uc_vap_id);
        oal_memcopy((oal_uint8 *)frw_get_event_payload(pst_event_mem), (oal_uint8 *)&st_roam_rsp, OAL_SIZEOF(hmac_roam_rsp_stru));
    }

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);
    return OAL_SUCC;
}
#ifdef _PRE_WLAN_FEATURE_11R

OAL_STATIC oal_uint32  hmac_roam_ft_notify_wpas(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_mac_hdr, oal_uint16 us_msg_len)
{
    hmac_roam_ft_stru               *pst_ft_event;
    frw_event_mem_stru              *pst_event_mem;
    frw_event_stru                  *pst_event;
    oal_uint16                       us_ie_offset;

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_roam_rsp_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_ft_notify_wpas::FRW_EVENT_ALLOC fail!}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_event = frw_get_event_stru(pst_event_mem);
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_FT_EVENT_STA,
                       OAL_SIZEOF(hmac_roam_ft_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);

    pst_ft_event = (hmac_roam_ft_stru *)pst_event->auc_event_data;

    mac_get_address3(puc_mac_hdr, pst_ft_event->auc_bssid);

    if ((WLAN_FC0_SUBTYPE_AUTH|WLAN_FC0_TYPE_MGT) == mac_get_frame_type_and_subtype(puc_mac_hdr))
    {
        us_ie_offset = OAL_AUTH_IE_OFFSET;
    }
    else
    {
        us_ie_offset = OAL_FT_ACTION_IE_OFFSET;
    }

    pst_ft_event->puc_ft_ie_buff = puc_mac_hdr + us_ie_offset;
    pst_ft_event->us_ft_ie_len   = us_msg_len - us_ie_offset;

    /* 分发事件 */
    frw_event_dispatch_event_etc(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_roam_send_ft_req(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;
    hmac_vap_stru          *pst_hmac_vap;
    hmac_user_stru         *pst_hmac_user;
    oal_netbuf_stru        *pst_ft_frame;
    oal_uint8              *puc_ft_buff;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint8              *puc_my_mac_addr;
    oal_uint8              *puc_current_bssid;
    oal_uint16              us_ft_len;
    oal_uint16              us_app_ie_len;


    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_UP,ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_INIT);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_send_ft_req::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    if(OAL_TRUE != pst_hmac_vap->bit_11r_enable)
    {
        return OAL_SUCC;
    }

    pst_ft_frame = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_ft_frame)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_send_ft_req::OAL_MEM_NETBUF_ALLOC fail.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    puc_ft_buff = (oal_uint8 *)OAL_NETBUF_HEADER(pst_ft_frame);
    OAL_MEMZERO(oal_netbuf_cb(pst_ft_frame), 48);
    OAL_MEMZERO(puc_ft_buff, MAC_80211_FRAME_LEN);

    puc_my_mac_addr     = mac_mib_get_StationID(&pst_hmac_vap->st_vap_base_info);
    puc_current_bssid   = pst_hmac_vap->st_vap_base_info.auc_bssid;
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
    mac_hdr_set_frame_control(puc_ft_buff, WLAN_FC0_SUBTYPE_ACTION);
    /*  Set DA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_ft_buff)->auc_address1, puc_current_bssid);
    /*  Set SA  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_ft_buff)->auc_address2, puc_my_mac_addr);
    /*  Set SSID  */
    oal_set_mac_addr(((mac_ieee80211_frame_stru *)puc_ft_buff)->auc_address3, puc_current_bssid);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*************************************************************************/

    /*************************************************************************/
    /*                  FT Request Frame - Frame Body                          */
    /* --------------------------------------------------------------------- */
    /* | Category | Action | STA Addr |Target AP Addr | FT Req frame body  |*/
    /* --------------------------------------------------------------------- */
    /* |     1    |   1    |     6    |       6       |       varibal      | */
    /* --------------------------------------------------------------------- */
    /*                                                                       */
    /*************************************************************************/
    puc_ft_buff  += MAC_80211_FRAME_LEN;
    us_ft_len     = MAC_80211_FRAME_LEN;

    puc_ft_buff[0]   = MAC_ACTION_CATEGORY_FAST_BSS_TRANSITION;
    puc_ft_buff[1]   = MAC_FT_ACTION_REQUEST;
    puc_ft_buff  += 2;
    us_ft_len    += 2;

    oal_set_mac_addr(puc_ft_buff, puc_my_mac_addr);
    puc_ft_buff  += OAL_MAC_ADDR_LEN;
    us_ft_len    += OAL_MAC_ADDR_LEN;

    oal_set_mac_addr(puc_ft_buff, pst_roam_info->st_connect.pst_bss_dscr->auc_bssid);
    puc_ft_buff  += OAL_MAC_ADDR_LEN;
    us_ft_len    += OAL_MAC_ADDR_LEN;

    mac_add_app_ie_etc((oal_void *)&pst_hmac_vap->st_vap_base_info, puc_ft_buff, &us_app_ie_len, OAL_APP_FT_IE);
    us_ft_len      += us_app_ie_len;
    puc_ft_buff    += us_app_ie_len;

    oal_netbuf_put(pst_ft_frame, us_ft_len);

    /* 为填写发送描述符准备参数 */
    pst_tx_ctl                   = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_ft_frame);
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)      = us_ft_len;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = pst_hmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_NETBUF_NUM(pst_tx_ctl)    = 1;

    /* 抛事件让dmac将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event_etc(&pst_hmac_vap->st_vap_base_info, pst_ft_frame, us_ft_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_ft_frame);
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_send_ft_req::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_WAIT_FT_COMP);

    /* 启动认证超时定时器 */
    hmac_roam_connect_start_timer(pst_roam_info, ROAM_AUTH_TIME_MAX);

    return OAL_SUCC;
}

OAL_STATIC oal_uint32 hmac_roam_process_ft_rsp(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32                   ul_ret;
    hmac_vap_stru               *pst_hmac_vap;
    dmac_wlan_crx_event_stru    *pst_crx_event;
    mac_rx_ctl_stru            *pst_rx_ctrl;
    oal_uint8                   *puc_mac_hdr;
    oal_uint8                   *puc_ft_frame_body;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_UP, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_FT_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_ft_rsp::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap = pst_roam_info->pst_hmac_vap;

    if(OAL_TRUE != pst_hmac_vap->bit_11r_enable)
    {
        return OAL_SUCC;
    }

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_roam_process_auth_seq2::wakeup mgmt type[0x%x]}",mac_get_frame_type_and_subtype(puc_mac_hdr));
    }
#endif
#endif

    /* 只处理action帧 */
    if ((WLAN_FC0_SUBTYPE_ACTION|WLAN_FC0_TYPE_MGT) != mac_get_frame_type_and_subtype(puc_mac_hdr))
    {
        return OAL_SUCC;
    }

    puc_ft_frame_body = puc_mac_hdr + MAC_80211_FRAME_LEN;

    if ((MAC_ACTION_CATEGORY_FAST_BSS_TRANSITION != puc_ft_frame_body[0]) ||
        (MAC_FT_ACTION_RESPONSE != puc_ft_frame_body[1]))
    {
        return OAL_SUCC;
    }
    /* 上报FT成功消息给APP，以便APP下发新的FT_IE用于发送reassociation */
    ul_ret = hmac_roam_ft_notify_wpas(pst_hmac_vap, puc_mac_hdr, pst_rx_ctrl->us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_process_ft_rsp::hmac_roam_ft_notify_wpas failed[%d].}", ul_ret);
        return ul_ret;
    }

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_WAIT_ASSOC_COMP);

    /* 启动关联超时定时器 */
    hmac_roam_connect_start_timer(pst_roam_info, ROAM_ASSOC_TIME_MAX);
    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_11R

OAL_STATIC oal_uint32 hmac_roam_start_join(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;
    hmac_vap_stru          *pst_hmac_vap;
    mac_bss_dscr_stru      *pst_bss_dscr;
    hmac_join_req_stru      st_join_req;
    oal_app_ie_stru         st_app_ie;
    oal_uint8               uc_ie_len = 0;
    oal_uint8              *puc_pmkid;
    oal_uint8               uc_rate_num;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_INIT);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_start_join::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap = pst_roam_info->pst_hmac_vap;

    pst_bss_dscr = (mac_bss_dscr_stru *)p_param;

    uc_rate_num = (pst_bss_dscr->uc_num_supp_rates < WLAN_MAX_SUPP_RATES) ? pst_bss_dscr->uc_num_supp_rates : WLAN_MAX_SUPP_RATES;
    oal_memcopy(pst_hmac_vap->auc_supp_rates,pst_bss_dscr->auc_supp_rates, uc_rate_num);
    mac_mib_set_SupportRateSetNums(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr->uc_num_supp_rates);

    /* 配置join参数 */
    hmac_prepare_join_req_etc(&st_join_req, pst_bss_dscr);

    ul_ret = hmac_sta_update_join_req_params_etc(pst_hmac_vap, &st_join_req);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_start_join::hmac_sta_update_join_req_params_etc fail[%d].}", ul_ret);
        return ul_ret;
    }

    if (OAL_TRUE == pst_hmac_vap->st_vap_base_info.st_cap_flag.bit_wpa)
    {
        /* 设置 WPA Capability IE */
        mac_set_wpa_ie_etc((oal_void *)&pst_hmac_vap->st_vap_base_info, st_app_ie.auc_ie, &uc_ie_len);
    }

    if (OAL_TRUE == pst_hmac_vap->st_vap_base_info.st_cap_flag.bit_wpa2)
    {
        /* 设置 RSN Capability IE */
        puc_pmkid = hmac_vap_get_pmksa_etc(pst_hmac_vap, pst_bss_dscr->auc_bssid);
        mac_set_rsn_ie_etc((oal_void *)&pst_hmac_vap->st_vap_base_info, puc_pmkid, st_app_ie.auc_ie, &uc_ie_len);
    }

    if (0 != uc_ie_len)
    {
        st_app_ie.en_app_ie_type = OAL_APP_REASSOC_REQ_IE;
        st_app_ie.ul_ie_len      = uc_ie_len;
        ul_ret = hmac_config_set_app_ie_to_vap_etc(&pst_hmac_vap->st_vap_base_info, &st_app_ie, OAL_APP_REASSOC_REQ_IE);
        if (ul_ret != OAL_SUCC)
        {
            OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                             "{hmac_roam_start_join::hmac_config_set_app_ie_to_vap_etc fail, ul_ret=[%d].}", ul_ret);
        }
    }
    else
    {
        mac_vap_clear_app_ie_etc(&pst_hmac_vap->st_vap_base_info, OAL_APP_REASSOC_REQ_IE);
    }

    hmac_roam_connect_set_dtim_param_etc(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr->uc_dtim_cnt, pst_bss_dscr->uc_dtim_cnt);

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_WAIT_JOIN);

    ul_ret = hmac_roam_send_auth_seq1(pst_roam_info, p_param);
    if (OAL_SUCC != ul_ret)
    {
        hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_FAIL);

        /* 通知ROAM主状态机 */
        hmac_roam_connect_complete_etc(pst_hmac_vap, OAL_FAIL);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                         "{hmac_roam_process_beacon::hmac_roam_send_auth_seq1 fail[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_roam_send_auth_seq1(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;
    hmac_vap_stru          *pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    hmac_user_stru         *pst_hmac_user = pst_roam_info->pst_hmac_user;
    oal_netbuf_stru        *pst_auth_frame;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint16              us_auth_len;
    oal_uint8               uc_vap_id;

    uc_vap_id = pst_hmac_vap->st_vap_base_info.uc_vap_id;

    pst_auth_frame = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_auth_frame)
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_auth_seq1::OAL_MEM_NETBUF_ALLOC fail.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    OAL_MEM_NETBUF_TRACE(pst_auth_frame, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_auth_frame), OAL_NETBUF_CB_SIZE());

    OAL_MEMZERO((oal_uint8 *)oal_netbuf_header(pst_auth_frame), MAC_80211_FRAME_LEN);

    /* 更新用户mac */
    oal_set_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, pst_roam_info->st_connect.pst_bss_dscr->auc_bssid);

    us_auth_len = hmac_mgmt_encap_auth_req_etc(pst_hmac_vap, (oal_uint8 *)(OAL_NETBUF_HEADER(pst_auth_frame)));
    if (us_auth_len < OAL_AUTH_IE_OFFSET)
    {
        OAM_WARNING_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_auth_seq1::hmac_mgmt_encap_auth_req_etc failed.}");
        oal_netbuf_free(pst_auth_frame);
        return OAL_ERR_CODE_ROAM_FRAMER_LEN;
    }

    oal_netbuf_put(pst_auth_frame, us_auth_len);

    /* 为填写发送描述符准备参数 */
    pst_tx_ctl                   = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_auth_frame);
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)      = us_auth_len;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = pst_hmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_NETBUF_NUM(pst_tx_ctl)    = 1;

    /* 抛事件让dmac将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event_etc(&pst_hmac_vap->st_vap_base_info, pst_auth_frame, us_auth_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_auth_frame);
        OAM_ERROR_LOG1(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_auth_seq1::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_WAIT_AUTH_COMP);

    /* 启动认证超时定时器 */
    hmac_roam_connect_start_timer(pst_roam_info, ROAM_AUTH_TIME_MAX);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_roam_send_reassoc_req(hmac_roam_info_stru *pst_roam_info)
{
    oal_uint32              ul_ret;
    hmac_vap_stru          *pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    hmac_user_stru         *pst_hmac_user = pst_roam_info->pst_hmac_user;
    mac_vap_stru           *pst_mac_vap;
    oal_netbuf_stru        *pst_assoc_req_frame;
    mac_tx_ctl_stru        *pst_tx_ctl;
    oal_uint32              ul_assoc_len;

    pst_mac_vap = &pst_hmac_vap->st_vap_base_info;

    pst_assoc_req_frame = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if ((OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == pst_assoc_req_frame))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::pst_hmac_user[%d] or pst_assoc_req_frame[%d] is NULL.}",
                        pst_hmac_user, pst_assoc_req_frame);
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEM_NETBUF_TRACE(pst_assoc_req_frame, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_assoc_req_frame), OAL_NETBUF_CB_SIZE());

    /* 将mac header清零 */
    OAL_MEMZERO((oal_uint8 *)oal_netbuf_header(pst_assoc_req_frame), MAC_80211_FRAME_LEN);

    pst_hmac_vap->bit_reassoc_flag = OAL_TRUE;

    ul_assoc_len = hmac_mgmt_encap_asoc_req_sta_etc(pst_hmac_vap,(oal_uint8 *)(OAL_NETBUF_HEADER(pst_assoc_req_frame)), pst_roam_info->st_old_bss.auc_bssid);

    oal_netbuf_put(pst_assoc_req_frame, ul_assoc_len);

    /* 帧长异常 */
    if (ul_assoc_len <= OAL_ASSOC_REQ_IE_OFFSET)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::unexpected assoc len[%d].}", ul_assoc_len);
        oal_netbuf_free(pst_assoc_req_frame);
        return OAL_FAIL;
    }
    hmac_user_free_asoc_req_ie(pst_hmac_user->st_user_base_info.us_assoc_id);

    /* 记录关联请求帧的部分内容，用于上报给内核 */
    ul_ret = hmac_user_set_asoc_req_ie(pst_hmac_user,
                                           OAL_NETBUF_HEADER(pst_assoc_req_frame) + OAL_ASSOC_REQ_IE_OFFSET,
                                           ul_assoc_len - OAL_ASSOC_REQ_IE_OFFSET,
                                           OAL_TRUE);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::hmac_user_set_asoc_req_ie failed}");
        oal_netbuf_free(pst_assoc_req_frame);
        return OAL_FAIL;
    }

    /* 为填写发送描述符准备参数 */
    pst_tx_ctl                   = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_assoc_req_frame);
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl)      = (oal_uint16)ul_assoc_len;
    MAC_GET_CB_TX_USER_IDX(pst_tx_ctl)   = pst_hmac_user->st_user_base_info.us_assoc_id;
    MAC_GET_CB_NETBUF_NUM(pst_tx_ctl)    = 1;

    /* 抛事件让dmac将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event_etc(&pst_hmac_vap->st_vap_base_info, pst_assoc_req_frame, (oal_uint16)ul_assoc_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_assoc_req_frame);
        hmac_user_free_asoc_req_ie(pst_hmac_user->st_user_base_info.us_assoc_id);
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::hmac_tx_mgmt_send_event_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* 启动关联超时定时器 */
    hmac_roam_connect_start_timer(pst_roam_info, ROAM_ASSOC_TIME_MAX);

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_WAIT_ASSOC_COMP);

    return OAL_SUCC;
}

OAL_STATIC oal_uint32 hmac_roam_process_auth_seq2(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32                   ul_ret;
    hmac_vap_stru               *pst_hmac_vap;
    hmac_user_stru              *pst_hmac_user;
    dmac_wlan_crx_event_stru    *pst_crx_event;
    mac_rx_ctl_stru            *pst_rx_ctrl;
    oal_uint8                   *puc_mac_hdr;
    oal_uint8                    auc_bssid[6]   = {0};
    oal_uint16                   us_auth_status = MAC_UNSPEC_FAIL;
    oal_uint8                    uc_frame_sub_type;
    oal_uint16                   us_auth_seq_num;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_AUTH_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_auth_seq2::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl);

    mac_get_address3(puc_mac_hdr, auc_bssid);
    if (oal_compare_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, auc_bssid))
    {
        return OAL_SUCC;
    }

    uc_frame_sub_type = mac_get_frame_type_and_subtype(puc_mac_hdr);
    us_auth_seq_num = mac_get_auth_seq_num(puc_mac_hdr);
    us_auth_status = mac_get_auth_status(puc_mac_hdr);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_roam_process_auth_seq2::wakeup mgmt type[0x%x]}",uc_frame_sub_type);
    }
#endif
#endif

    /* auth_seq2帧校验，错误帧不处理，在超时中统一处理 */
    if (((WLAN_FC0_SUBTYPE_AUTH|WLAN_FC0_TYPE_MGT) != uc_frame_sub_type) ||
        (WLAN_AUTH_TRASACTION_NUM_TWO != us_auth_seq_num))
    {
        return OAL_SUCC;
    }

    if(MAC_SUCCESSFUL_STATUSCODE != us_auth_status)
    {
        return OAL_SUCC;
    }

#ifdef _PRE_WLAN_FEATURE_11R
    if(OAL_TRUE == pst_hmac_vap->bit_11r_enable)
    {
        if (WLAN_WITP_AUTH_FT == mac_get_auth_alg(puc_mac_hdr))
        {
            /* 上报FT成功消息给APP，以便APP下发新的FT_IE用于发送reassociation */
            ul_ret = hmac_roam_ft_notify_wpas(pst_hmac_vap, puc_mac_hdr, pst_rx_ctrl->us_frame_len);
            if (OAL_SUCC != ul_ret)
            {
                OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                               "{hmac_roam_process_auth_seq2::hmac_roam_ft_notify_wpas failed[%d].}", ul_ret);
                return ul_ret;
            }

            hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_WAIT_ASSOC_COMP);
            /* 启动关联超时定时器 */
            hmac_roam_connect_start_timer(pst_roam_info, ROAM_ASSOC_TIME_MAX);
            return OAL_SUCC;
        }
    }
#endif //_PRE_WLAN_FEATURE_11R

    if (WLAN_WITP_AUTH_OPEN_SYSTEM != mac_get_auth_alg(puc_mac_hdr))
    {
        return OAL_SUCC;
    }

    /* 发送关联请求 */
    ul_ret = hmac_roam_send_reassoc_req(pst_roam_info);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_process_auth_seq2::hmac_roam_send_assoc_req failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_roam_process_assoc_rsp(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32                       ul_ret;
    hmac_vap_stru                   *pst_hmac_vap;
    hmac_user_stru                  *pst_hmac_user;
    dmac_wlan_crx_event_stru        *pst_crx_event;
    mac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint8                       *puc_mac_hdr;
    oal_uint8                       *puc_payload;
    oal_uint16                       us_msg_len;
    oal_uint16                       us_hdr_len;
    mac_status_code_enum_uint16      en_asoc_status;
    oal_uint8                        auc_bss_addr[WLAN_MAC_ADDR_LEN];
    oal_uint8                        uc_frame_sub_type;

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_ASSOC_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_assoc_rsp::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl);
    puc_payload    = puc_mac_hdr + pst_rx_ctrl->uc_mac_header_len;
    us_msg_len     = pst_rx_ctrl->us_frame_len - pst_rx_ctrl->uc_mac_header_len;
    us_hdr_len     = pst_rx_ctrl->uc_mac_header_len;

    /* mac地址校验 */
    mac_get_address3(puc_mac_hdr, auc_bss_addr);
    if (oal_compare_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, auc_bss_addr))
    {
        return OAL_SUCC;
    }

    /* assoc帧校验，错误帧处理 */
    uc_frame_sub_type = mac_get_frame_type_and_subtype(puc_mac_hdr);
    en_asoc_status = mac_get_asoc_status(puc_payload);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_roam_process_auth_seq2::wakeup mgmt type[0x%x]}",uc_frame_sub_type);
    }
#endif
#endif

    if (((WLAN_FC0_SUBTYPE_REASSOC_RSP|WLAN_FC0_TYPE_MGT)   != uc_frame_sub_type)
         && ((WLAN_FC0_SUBTYPE_ASSOC_RSP|WLAN_FC0_TYPE_MGT) != uc_frame_sub_type))
    {
        return OAL_SUCC;
    }

    /* 关联响应帧长度校验 */
    if (pst_rx_ctrl->us_frame_len <= OAL_ASSOC_RSP_IE_OFFSET)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                         "{hmac_roam_process_assoc_rsp::rsp ie length error, us_frame_len[%d].}", pst_rx_ctrl->us_frame_len);
        return OAL_ERR_CODE_ROAM_FRAMER_LEN;
    }

    if(MAC_SUCCESSFUL_STATUSCODE != en_asoc_status)
    {
        return OAL_SUCC;
    }

    ul_ret = hmac_process_assoc_rsp_etc(pst_hmac_vap, pst_hmac_user, puc_mac_hdr, us_hdr_len, puc_payload, us_msg_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                         "{hmac_roam_process_assoc_rsp::hmac_process_assoc_rsp_etc failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* user已经关联上，抛事件给DMAC，在DMAC层挂用户算法钩子 */
    hmac_user_add_notify_alg_etc(&(pst_hmac_vap->st_vap_base_info), pst_hmac_user->st_user_base_info.us_assoc_id);

    /* 上报关联成功消息给APP */
    ul_ret = hmac_roam_connect_notify_wpas(pst_roam_info, puc_mac_hdr, pst_rx_ctrl->us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_process_assoc_rsp::hmac_roam_connect_notify_wpas failed[%d].}", ul_ret);
        return ul_ret;
    }

    if (OAL_TRUE != mac_mib_get_privacyinvoked(&pst_hmac_vap->st_vap_base_info))
    {
        /* 非加密情况下，漫游成功 */
        hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_HANDSHAKING);
        hmac_roam_connect_succ(pst_roam_info, OAL_PTR_NULL);
    }
    else
    {
#ifdef _PRE_WLAN_FEATURE_11R
        if(OAL_TRUE == pst_hmac_vap->bit_11r_enable)
        {
            if (OAL_TRUE == mac_mib_get_ft_trainsistion(&pst_hmac_vap->st_vap_base_info))
            {
                /* FT情况下，漫游成功 */
                hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_HANDSHAKING);
                hmac_roam_connect_succ(pst_roam_info, OAL_PTR_NULL);
                return OAL_SUCC;
            }
        }
#endif //_PRE_WLAN_FEATURE_11R
        if (OAL_TRUE == mac_mib_get_rsnaactivated(&pst_hmac_vap->st_vap_base_info))
        {
            hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_HANDSHAKING);
            /* 启动握手超时定时器 */
            hmac_roam_connect_start_timer(pst_roam_info, ROAM_HANDSHAKE_TIME_MAX);
        }
        else
        {
            /* 非 WPA 或者 WPA2 加密情况下(WEP_OPEN/WEP_SHARED)，漫游成功 */
            hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_HANDSHAKING);
            hmac_roam_connect_succ(pst_roam_info, OAL_PTR_NULL);
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_roam_process_action(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32                       ul_ret;
    hmac_vap_stru                   *pst_hmac_vap;
    hmac_user_stru                  *pst_hmac_user;
    dmac_wlan_crx_event_stru        *pst_crx_event;
    mac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint8                       *puc_mac_hdr;
    oal_uint8                       *puc_payload;
    oal_uint8                        auc_bss_addr[WLAN_MAC_ADDR_LEN];
    oal_uint8                        uc_frame_sub_type;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_HANDSHAKING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_action::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (mac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl);
    puc_payload    = puc_mac_hdr + pst_rx_ctrl->uc_mac_header_len;

    /* mac地址校验 */
    mac_get_address3(puc_mac_hdr, auc_bss_addr);
    if (oal_compare_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, auc_bss_addr))
    {
        return OAL_SUCC;
    }

    uc_frame_sub_type = mac_get_frame_type_and_subtype(puc_mac_hdr);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
    if(OAL_TRUE == wlan_pm_wkup_src_debug_get())
    {
        wlan_pm_wkup_src_debug_set(OAL_FALSE);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_RX, "{wifi_wake_src:hmac_roam_process_auth_seq2::wakeup mgmt type[0x%x]}",uc_frame_sub_type);
    }
#endif
#endif

    if ((WLAN_FC0_SUBTYPE_ACTION|WLAN_FC0_TYPE_MGT) != uc_frame_sub_type)
    {
        return OAL_SUCC;
    }


    if (MAC_ACTION_CATEGORY_BA == puc_payload[MAC_ACTION_OFFSET_CATEGORY])
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                         "{hmac_roam_process_action::BA_ACTION_TYPE [%d].}", puc_payload[MAC_ACTION_OFFSET_ACTION]);
        switch(puc_payload[MAC_ACTION_OFFSET_ACTION])
        {
            case MAC_BA_ACTION_ADDBA_REQ:
                ul_ret = hmac_mgmt_rx_addba_req_etc(pst_hmac_vap, pst_hmac_user, puc_payload);
                break;

            case MAC_BA_ACTION_ADDBA_RSP:
                ul_ret = hmac_mgmt_rx_addba_rsp_etc(pst_hmac_vap, pst_hmac_user, puc_payload);
                break;

            case MAC_BA_ACTION_DELBA:
                ul_ret = hmac_mgmt_rx_delba_etc(pst_hmac_vap, pst_hmac_user, puc_payload);
                break;

            default:
                break;
        }
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_roam_connect_succ(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32                       ul_ret;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_HANDSHAKING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_succ::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_UP);

    /* 删除定时器 */
    hmac_roam_connect_del_timer(pst_roam_info);

    /* 通知ROAM主状态机 */
    hmac_roam_connect_complete_etc(pst_roam_info->pst_hmac_vap, OAL_SUCC);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_roam_auth_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;
    hmac_vap_stru          *pst_hmac_vap;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_AUTH_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_auth_timeout::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;

    if (++pst_roam_info->st_connect.uc_auth_num >= MAX_AUTH_CNT)
    {
        return hmac_roam_connect_fail(pst_roam_info, p_param);
    }

    ul_ret = hmac_roam_send_auth_seq1(pst_roam_info, p_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_auth_timeout::hmac_roam_send_auth_seq1 failed[%d].}", ul_ret);
    }

    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_roam_assoc_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_ASSOC_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_assoc_timeout::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    if (++pst_roam_info->st_connect.uc_assoc_num >= MAX_ASOC_CNT)
    {
        return hmac_roam_connect_fail(pst_roam_info, p_param);
    }

    ul_ret = hmac_roam_send_reassoc_req(pst_roam_info);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ROAM, "{hmac_roam_assoc_timeout::hmac_roam_send_reassoc_req failed[%d].}", ul_ret);
    }
    return ul_ret;
}


OAL_STATIC oal_uint32 hmac_roam_handshaking_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_HANDSHAKING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_handshaking_timeout::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    return hmac_roam_connect_fail(pst_roam_info, p_param);
}

#ifdef _PRE_WLAN_FEATURE_11R

OAL_STATIC oal_uint32 hmac_roam_ft_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;
    hmac_vap_stru          *pst_hmac_vap;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info,MAC_VAP_STATE_ROAMING, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_FT_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_ft_timeout::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;

    if(OAL_TRUE != pst_hmac_vap->bit_11r_enable)
    {
        return OAL_SUCC;
    }

    if (++pst_roam_info->st_connect.uc_ft_num >= MAX_AUTH_CNT)
    {
        return hmac_roam_connect_fail(pst_roam_info, p_param);
    }

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_INIT);

    ul_ret = hmac_roam_send_ft_req(pst_roam_info, p_param);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_ft_timeout::hmac_roam_send_ft_req failed[%d].}", ul_ret);
    }
    return ul_ret;
}
#endif //_PRE_WLAN_FEATURE_11R

OAL_STATIC oal_uint32 hmac_roam_connect_fail(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    hmac_vap_stru                   *pst_hmac_vap = pst_roam_info->pst_hmac_vap;
    roam_connect_state_enum_uint8    connect_state = pst_roam_info->st_connect.en_state;
    /* connect状态切换 */
    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_FAIL);

    /* connect失败时，需要添加到黑名单 */
    hmac_roam_alg_add_blacklist_etc(pst_roam_info, pst_roam_info->st_connect.pst_bss_dscr->auc_bssid, ROAM_BLACKLIST_TYPE_REJECT_AP);
    OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                     "{hmac_roam_connect_fail::hmac_roam_alg_add_blacklist_etc!}");

    /* 通知ROAM主状态机，BSS回退由主状态机完成 */

    if (ROAM_CONNECT_STATE_HANDSHAKING == connect_state)
    {
        hmac_roam_connect_complete_etc(pst_hmac_vap, OAL_ERR_CODE_ROAM_HANDSHAKE_FAIL);
    }
    else if (ROAM_CONNECT_STATE_WAIT_ASSOC_COMP == connect_state
             || ROAM_CONNECT_STATE_WAIT_AUTH_COMP  == connect_state)
    {
        hmac_roam_connect_complete_etc(pst_hmac_vap, OAL_ERR_CODE_ROAM_NO_RESPONSE);
    }
    else
    {
        hmac_roam_connect_complete_etc(pst_hmac_vap, OAL_FAIL);
    }

    return OAL_SUCC;
}

oal_uint32 hmac_roam_connect_start_etc(hmac_vap_stru *pst_hmac_vap, mac_bss_dscr_stru *pst_bss_dscr)
{
    hmac_roam_info_stru                             *pst_roam_info;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start_etc::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }


    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start_etc::roam_info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 漫游开关没有开时，不处理tbtt中断 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start_etc::roam disabled!}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    pst_roam_info->st_connect.pst_bss_dscr = pst_bss_dscr;
    pst_roam_info->st_connect.uc_auth_num  = 0;
    pst_roam_info->st_connect.uc_assoc_num = 0;
    pst_roam_info->st_connect.uc_ft_num    = 0;

#ifdef _PRE_WLAN_FEATURE_11R
    if(OAL_TRUE == pst_hmac_vap->bit_11r_enable)
    {
        if (OAL_TRUE == (mac_mib_get_ft_trainsistion(&pst_hmac_vap->st_vap_base_info)
            && mac_mib_get_ft_over_ds(&pst_hmac_vap->st_vap_base_info)) && (OAL_TRUE == pst_hmac_vap->bit_11r_over_ds))
        {
            return hmac_roam_connect_fsm_action_etc(pst_roam_info, ROAM_CONNECT_FSM_EVENT_FT_OVER_DS, (oal_void *)pst_bss_dscr);
        }
    }
#endif //_PRE_WLAN_FEATURE_11R

    return hmac_roam_connect_fsm_action_etc(pst_roam_info, ROAM_CONNECT_FSM_EVENT_START, (oal_void *)pst_bss_dscr);

}


oal_uint32 hmac_roam_connect_stop_etc(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru              *pst_roam_info;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start_etc::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }


    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start_etc::roam_info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }
    pst_roam_info->st_connect.en_state = ROAM_CONNECT_STATE_INIT;
    return OAL_SUCC;
}


oal_void  hmac_roam_connect_rx_mgmt_etc(hmac_vap_stru *pst_hmac_vap, dmac_wlan_crx_event_stru *pst_crx_event)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_uint32                        ul_ret;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_rx_mgmt_etc::vap null!}");
        return;
    }


    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        return;
    }

    /* 漫游开关没有开时，不处理管理帧接收 */
    if (0 == pst_roam_info->uc_enable)
    {
        return;
    }

    ul_ret = hmac_roam_connect_fsm_action_etc(pst_roam_info, ROAM_CONNECT_FSM_EVENT_MGMT_RX, (oal_void *)pst_crx_event);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_rx_mgmt_etc::MGMT_RX FAIL[%d]!}", ul_ret);
    }

    return;
}


oal_void hmac_roam_connect_key_done_etc(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_uint32                        ul_ret;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_key_done_etc::vap null!}");
        return;
    }


    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        return;
    }

    /* 漫游开关没有开时，不处理管理帧接收 */
    if (0 == pst_roam_info->uc_enable)
    {
        return;
    }

    /* 主状态机为非CONNECTING状态/CONNECT状态机为非UP状态，失败 */
    if (pst_roam_info->en_main_state != ROAM_MAIN_STATE_CONNECTING)
    {
        return;
    }

    ul_ret = hmac_roam_connect_fsm_action_etc(pst_roam_info, ROAM_CONNECT_FSM_EVENT_KEY_DONE, OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_key_done_etc::KEY_DONE FAIL[%d]!}", ul_ret);
    }
    OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_key_done_etc::KEY_DONE !}");

    return;
}

#endif //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R

oal_uint32 hmac_roam_connect_ft_reassoc_etc(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru                             *pst_roam_info;
    hmac_join_req_stru                               st_join_req;
    oal_uint32                                       ul_ret;
    mac_bss_dscr_stru                               *pst_bss_dscr;
    oal_uint8                                        uc_rate_num;

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    pst_bss_dscr = pst_roam_info->st_connect.pst_bss_dscr;

    if(OAL_PTR_NULL != pst_roam_info->pst_hmac_user)
    {
        oal_set_mac_addr(pst_roam_info->pst_hmac_user->st_user_base_info.auc_user_mac_addr,pst_bss_dscr->auc_bssid);
    }

    uc_rate_num = (pst_bss_dscr->uc_num_supp_rates < WLAN_MAX_SUPP_RATES) ? pst_bss_dscr->uc_num_supp_rates : WLAN_MAX_SUPP_RATES;
    oal_memcopy(pst_hmac_vap->auc_supp_rates,pst_bss_dscr->auc_supp_rates , uc_rate_num);
    mac_mib_set_SupportRateSetNums(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr->uc_num_supp_rates);

    if(mac_mib_get_ft_over_ds(&pst_hmac_vap->st_vap_base_info) && (OAL_TRUE == pst_hmac_vap->bit_11r_over_ds))
    {
        /* 配置join参数 */
        hmac_prepare_join_req_etc(&st_join_req, pst_bss_dscr);

        ul_ret = hmac_sta_update_join_req_params_etc(pst_hmac_vap, &st_join_req);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                           "{hmac_roam_connect_ft_reassoc_etc::hmac_sta_update_join_req_params_etc fail[%d].}", ul_ret);
            return ul_ret;
        }
    }
    /* 发送关联请求 */
    ul_ret = hmac_roam_send_reassoc_req(pst_roam_info);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_connect_ft_reassoc_etc::hmac_roam_send_assoc_req failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

