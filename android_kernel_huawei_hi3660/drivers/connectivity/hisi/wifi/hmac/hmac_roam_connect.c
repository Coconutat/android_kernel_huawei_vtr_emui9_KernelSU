


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


oal_void hmac_roam_connect_fsm_init(oal_void)
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


oal_uint32 hmac_roam_connect_fsm_action(hmac_roam_info_stru *pst_roam_info, roam_connect_fsm_event_type_enum en_event, oal_void *p_param)
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

    if ((pst_roam_info->pst_hmac_vap->st_vap_base_info.en_vap_state != MAC_VAP_STATE_ROAMING) ||
       (pst_roam_info->en_main_state != en_main_state) ||
       (pst_roam_info->st_connect.en_state != en_connect_state))
    {
        OAM_WARNING_LOG3(0, OAM_SF_ROAM, "{hmac_roam_connect_check_state::unexpect vap_state[%d] main_state[%d] connect_state[%d]!}",
                      pst_roam_info->pst_hmac_vap->st_vap_base_info.en_vap_state, pst_roam_info->en_main_state, pst_roam_info->st_connect.en_state);
        return OAL_ERR_CODE_ROAM_INVALID_VAP_STATUS;
    }

    return OAL_SUCC;
}



oal_uint32 hmac_roam_connect_timeout(oal_void *p_arg)
{
    hmac_roam_info_stru *pst_roam_info;

    pst_roam_info = (hmac_roam_info_stru *)p_arg;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_timeout::p_arg is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_WARNING_LOG2(0, OAM_SF_ROAM, "{hmac_roam_connect_timeout::MAIN_STATE[%d] CONNECT_STATE[%d].}",
                     pst_roam_info->en_main_state, pst_roam_info->st_connect.en_state);

    return hmac_roam_connect_fsm_action(pst_roam_info, ROAM_CONNECT_FSM_EVENT_TIMEOUT, OAL_PTR_NULL);
}

OAL_STATIC oal_uint32  hmac_roam_connect_null_fn(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_null_fn .}");

    return OAL_SUCC;
}


OAL_STATIC oal_void hmac_roam_connect_start_timer(hmac_roam_info_stru *pst_roam_info, oal_uint32 ul_timeout)
{
    frw_timeout_stru    *pst_timer = &(pst_roam_info->st_connect.st_timer);

    OAM_INFO_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_start_timer [%d].}", ul_timeout);

    /* 启动认证超时定时器 */
    FRW_TIMER_CREATE_TIMER(pst_timer,
                           hmac_roam_connect_timeout,
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


oal_uint32 hmac_roam_connect_set_join_reg(mac_vap_stru *pst_mac_vap)
{
    frw_event_mem_stru               *pst_event_mem;
    frw_event_stru                   *pst_event;
    dmac_ctx_join_req_set_reg_stru   *pst_reg_params;

    /* 抛事件DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_SET_REG到DMAC */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_set_join_reg::pst_event_mem ALLOC FAIL, size = %d.}",
                       OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru));
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
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
    pst_reg_params->st_current_channel.uc_idx         = pst_mac_vap->st_channel.uc_idx;

    /* 设置dtim period信息 */
    pst_reg_params->us_beacon_period = (oal_uint16)pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.ul_dot11BeaconPeriod;

    /* 同步FortyMHzOperationImplemented */
    pst_reg_params->en_dot11FortyMHzOperationImplemented   = mac_mib_get_FortyMHzOperationImplemented(pst_mac_vap);

    /* 设置beacon filter关闭 */
    pst_reg_params->ul_beacon_filter = OAL_FALSE;

    /* 设置no frame filter打开 */
    pst_reg_params->ul_non_frame_filter = OAL_TRUE;

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}


oal_uint32 hmac_roam_connect_set_dtim_param(mac_vap_stru *pst_mac_vap, oal_uint8 uc_dtim_cnt, oal_uint8 uc_dtim_period)
{
    frw_event_mem_stru               *pst_event_mem;
    frw_event_stru                   *pst_event;
    dmac_ctx_set_dtim_tsf_reg_stru   *pst_set_dtim_tsf_reg_params;

    if (OAL_PTR_NULL == pst_mac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_set_dtim_param, pst_mac_vap = NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 抛事件 DMAC_WLAN_CTX_EVENT_SUB_TYPE_JOIN_DTIM_TSF_REG 到DMAC, 申请事件内存 */
    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_set_dtim_param::pst_event_mem ALLOC FAIL, size = %d.}",
                       OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 填写事件 */
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;

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
    frw_event_dispatch_event(pst_event_mem);
    FRW_EVENT_FREE(pst_event_mem);

    return OAL_SUCC;
}

OAL_STATIC oal_uint32  hmac_roam_connect_notify_wpas(hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_mac_hdr, oal_uint16 us_msg_len)
{
    hmac_roam_rsp_stru               st_roam_rsp;
    frw_event_mem_stru              *pst_event_mem;
    frw_event_stru                  *pst_event;
    oal_uint8                       *puc_mgmt_data;

    oal_memset(&st_roam_rsp, 0, OAL_SIZEOF(hmac_asoc_rsp_stru));

    /* 获取AP的mac地址 */
    mac_get_address3(puc_mac_hdr, st_roam_rsp.auc_bssid);

    /* 获取关联请求帧信息 */
    st_roam_rsp.puc_asoc_req_ie_buff = pst_hmac_vap->puc_asoc_req_ie_buff;
    st_roam_rsp.ul_asoc_req_ie_len   = pst_hmac_vap->ul_asoc_req_ie_len;

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hmac_roam_rsp_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_handle_asoc_rsp_sta::FRW_EVENT_ALLOC fail!}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /* 记录关联响应帧的部分内容，用于上报给内核 */
    if (us_msg_len < OAL_ASSOC_RSP_IE_OFFSET)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_handle_asoc_rsp_sta::us_msg_len is too short, %d.}",us_msg_len);
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    puc_mgmt_data = (oal_uint8*)oal_memalloc(us_msg_len - OAL_ASSOC_RSP_IE_OFFSET);
    if(OAL_PTR_NULL == puc_mgmt_data)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_handle_asoc_rsp_sta::pst_mgmt_data alloc null,size %d.}",(us_msg_len - OAL_ASSOC_RSP_IE_OFFSET));
        FRW_EVENT_FREE(pst_event_mem);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    st_roam_rsp.ul_asoc_rsp_ie_len   = us_msg_len - OAL_ASSOC_RSP_IE_OFFSET;
    oal_memcopy(puc_mgmt_data, (oal_uint8 *)(puc_mac_hdr + OAL_ASSOC_RSP_IE_OFFSET), st_roam_rsp.ul_asoc_rsp_ie_len);
    st_roam_rsp.puc_asoc_rsp_ie_buff = puc_mgmt_data;

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_HOST_CTX,
                       HMAC_HOST_CTX_EVENT_SUB_TYPE_ROAM_COMP_STA,
                       OAL_SIZEOF(hmac_asoc_rsp_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       pst_hmac_vap->st_vap_base_info.uc_chip_id,
                       pst_hmac_vap->st_vap_base_info.uc_device_id,
                       pst_hmac_vap->st_vap_base_info.uc_vap_id);
    oal_memcopy((oal_uint8 *)frw_get_event_payload(pst_event_mem), (oal_uint8 *)&st_roam_rsp, OAL_SIZEOF(hmac_roam_rsp_stru));

    /* 分发事件 */
    frw_event_dispatch_event(pst_event_mem);
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
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
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

    if (WLAN_FC0_SUBTYPE_AUTH == mac_get_frame_sub_type(puc_mac_hdr))
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
    frw_event_dispatch_event(pst_event_mem);
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

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_INIT);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_send_ft_req::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    if(pst_hmac_vap->bit_11r_enable != OAL_TRUE)
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

    puc_my_mac_addr     = pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID;
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

    mac_add_app_ie((oal_void *)&pst_hmac_vap->st_vap_base_info, puc_ft_buff, &us_app_ie_len, OAL_APP_FT_IE);
    us_ft_len      += us_app_ie_len;
    puc_ft_buff    += us_app_ie_len;

    oal_netbuf_put(pst_ft_frame, us_ft_len);

    /* 为填写发送描述符准备参数 */
    pst_tx_ctl                   = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_ft_frame);
    pst_tx_ctl->us_mpdu_len      = us_ft_len;
    pst_tx_ctl->us_tx_user_idx   = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_tx_ctl->uc_netbuf_num    = 1;

    /* 抛事件让dmac将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event(&pst_hmac_vap->st_vap_base_info, pst_ft_frame, us_ft_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_ft_frame);
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_send_ft_req::hmac_tx_mgmt_send_event failed[%d].}", ul_ret);
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
    hmac_rx_ctl_stru            *pst_rx_ctrl;
    oal_uint8                   *puc_mac_hdr;
    oal_uint8                   *puc_ft_frame_body;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_FT_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_ft_rsp::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap = pst_roam_info->pst_hmac_vap;
    if(pst_hmac_vap->bit_11r_enable != OAL_TRUE)
    {
        return OAL_SUCC;
    }

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;


    /* 只处理action帧 */
    if (WLAN_FC0_SUBTYPE_ACTION != mac_get_frame_sub_type(puc_mac_hdr))
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
    ul_ret = hmac_roam_ft_notify_wpas(pst_hmac_vap, puc_mac_hdr, pst_rx_ctrl->st_rx_info.us_frame_len);
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

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_INIT);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_start_join::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap = pst_roam_info->pst_hmac_vap;

    pst_bss_dscr = (mac_bss_dscr_stru *)p_param;

    oal_memcopy(pst_hmac_vap->auc_supp_rates,pst_bss_dscr->auc_supp_rates ,pst_bss_dscr->uc_num_supp_rates);
    pst_hmac_vap->uc_rs_nrates = pst_bss_dscr->uc_num_supp_rates;

    /* 配置join参数 */
    hmac_prepare_join_req(&st_join_req, pst_bss_dscr);

    ul_ret = hmac_sta_update_join_req_params(pst_hmac_vap, &st_join_req);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                       "{hmac_roam_start_join::hmac_sta_update_join_req_params fail[%d].}", ul_ret);
        return ul_ret;
    }

#if defined(_PRE_WLAN_FEATURE_WPA)
    if (DMAC_WPA_802_11I == pst_hmac_vap->uc_80211i_mode)
    {
        /* 设置 WPA Capability IE */
        mac_set_wpa_ie((oal_void *)&pst_hmac_vap->st_vap_base_info, st_app_ie.auc_ie, &uc_ie_len);
    }
#endif /* defined (_PRE_WLAN_FEATURE_WPA) */

#if defined(_PRE_WLAN_FEATURE_WPA2)
    if (DMAC_RSNA_802_11I == pst_hmac_vap->uc_80211i_mode)
    {
        /* 设置 RSN Capability IE */
        puc_pmkid = hmac_vap_get_pmksa(pst_hmac_vap, pst_bss_dscr->auc_bssid);
        mac_set_rsn_ie((oal_void *)&pst_hmac_vap->st_vap_base_info, puc_pmkid, st_app_ie.auc_ie, &uc_ie_len);
    }
#endif /* defiend (_PRE_WLAN_FEATURE_WPA2) */

    if (0 != uc_ie_len)
    {
        st_app_ie.en_app_ie_type = OAL_APP_REASSOC_REQ_IE;
        st_app_ie.ul_ie_len      = uc_ie_len;
        hmac_config_set_app_ie_to_vap(&pst_hmac_vap->st_vap_base_info, &st_app_ie, OAL_APP_REASSOC_REQ_IE);
    }
    else
    {
        mac_vap_clear_app_ie(&pst_hmac_vap->st_vap_base_info, OAL_APP_REASSOC_REQ_IE);
    }

    hmac_roam_connect_set_dtim_param(&pst_hmac_vap->st_vap_base_info, pst_bss_dscr->uc_dtim_cnt, pst_bss_dscr->uc_dtim_cnt);

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_WAIT_JOIN);

    ul_ret = hmac_roam_send_auth_seq1(pst_roam_info, p_param);
    if (OAL_SUCC != ul_ret)
    {
        hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_FAIL);

        /* 通知ROAM主状态机 */
        hmac_roam_connect_complete(pst_hmac_vap, OAL_FAIL);
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

    us_auth_len = hmac_mgmt_encap_auth_req(pst_hmac_vap, (oal_uint8 *)(OAL_NETBUF_HEADER(pst_auth_frame)));
    if (us_auth_len < OAL_AUTH_IE_OFFSET)
    {
        OAM_WARNING_LOG0(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_auth_seq1::hmac_mgmt_encap_auth_req failed.}");
        oal_netbuf_free(pst_auth_frame);
        return OAL_ERR_CODE_ROAM_FRAMER_LEN;
    }

    oal_netbuf_put(pst_auth_frame, us_auth_len);

    /* 为填写发送描述符准备参数 */
    pst_tx_ctl                   = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_auth_frame);
    pst_tx_ctl->us_mpdu_len      = us_auth_len;
    pst_tx_ctl->us_tx_user_idx   = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_tx_ctl->uc_netbuf_num    = 1;

    /* 抛事件让dmac将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event(&pst_hmac_vap->st_vap_base_info, pst_auth_frame, us_auth_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_auth_frame);
        OAM_ERROR_LOG1(uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_auth_seq1::hmac_tx_mgmt_send_event failed[%d].}", ul_ret);
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
    if (OAL_PTR_NULL == pst_assoc_req_frame)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::OAL_MEM_NETBUF_ALLOC fail.}");
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    OAL_MEM_NETBUF_TRACE(pst_assoc_req_frame, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_assoc_req_frame), OAL_NETBUF_CB_SIZE());

    /* 将mac header清零 */
    OAL_MEMZERO((oal_uint8 *)oal_netbuf_header(pst_assoc_req_frame), MAC_80211_FRAME_LEN);

    ul_assoc_len = hmac_mgmt_encap_asoc_req_sta(pst_hmac_vap,(oal_uint8 *)(OAL_NETBUF_HEADER(pst_assoc_req_frame)), pst_roam_info->st_old_bss.auc_bssid);

    oal_netbuf_put(pst_assoc_req_frame, ul_assoc_len);

    /* 帧长异常 */
    if (ul_assoc_len <= OAL_ASSOC_REQ_IE_OFFSET)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::unexpected assoc len[%d].}", ul_assoc_len);
        oal_netbuf_free(pst_assoc_req_frame);
        return OAL_FAIL;
    }

    if (OAL_PTR_NULL != pst_hmac_vap->puc_asoc_req_ie_buff)
    {
        OAL_MEM_FREE(pst_hmac_vap->puc_asoc_req_ie_buff, OAL_TRUE);
        pst_hmac_vap->puc_asoc_req_ie_buff = OAL_PTR_NULL;
        pst_hmac_vap->ul_asoc_req_ie_len   = 0;
    }

    /* 记录关联请求帧的部分内容，用于上报给内核 */
    pst_hmac_vap->puc_asoc_req_ie_buff = OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL, (oal_uint16)(ul_assoc_len - OAL_ASSOC_REQ_IE_OFFSET - 6), OAL_TRUE);
    if (OAL_PTR_NULL == pst_hmac_vap->puc_asoc_req_ie_buff)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::OAL_MEM_ALLOC len[%d] fail.}", ul_assoc_len);
        oal_netbuf_free(pst_assoc_req_frame);
        return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }
    pst_hmac_vap->ul_asoc_req_ie_len = ul_assoc_len - OAL_ASSOC_REQ_IE_OFFSET - 6;
    oal_memcopy(pst_hmac_vap->puc_asoc_req_ie_buff, OAL_NETBUF_HEADER(pst_assoc_req_frame) + OAL_ASSOC_REQ_IE_OFFSET + 6, pst_hmac_vap->ul_asoc_req_ie_len);

    /* 为填写发送描述符准备参数 */
    pst_tx_ctl                   = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_assoc_req_frame);
    pst_tx_ctl->us_mpdu_len      = (oal_uint16)ul_assoc_len;
    pst_tx_ctl->us_tx_user_idx   = pst_hmac_user->st_user_base_info.us_assoc_id;
    pst_tx_ctl->uc_netbuf_num    = 1;

    /* 抛事件让dmac将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event(&pst_hmac_vap->st_vap_base_info, pst_assoc_req_frame, (oal_uint16)ul_assoc_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_assoc_req_frame);
        OAL_MEM_FREE(pst_hmac_vap->puc_asoc_req_ie_buff, OAL_TRUE);
        pst_hmac_vap->puc_asoc_req_ie_buff = OAL_PTR_NULL;
        pst_hmac_vap->ul_asoc_req_ie_len   = 0;
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_ROAM, "{hmac_roam_send_reassoc_req::hmac_tx_mgmt_send_event failed[%d].}", ul_ret);
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
    hmac_rx_ctl_stru            *pst_rx_ctrl;
    oal_uint8                   *puc_mac_hdr;
    oal_uint8                    auc_bssid[6]   = {0};
    oal_uint16                   us_auth_status = MAC_UNSPEC_FAIL;
    oal_uint8                    uc_frame_sub_type;
    oal_uint16                   us_auth_seq_num;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_AUTH_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_auth_seq2::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;

    mac_get_address3(puc_mac_hdr, auc_bssid);
    if (oal_compare_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, auc_bssid))
    {
        return OAL_SUCC;
    }

    uc_frame_sub_type = mac_get_frame_sub_type(puc_mac_hdr);
    us_auth_seq_num = mac_get_auth_seq_num(puc_mac_hdr);
    us_auth_status = mac_get_auth_status(puc_mac_hdr);

    /* auth_seq2帧校验，错误帧不处理，在超时中统一处理 */
    if ((WLAN_FC0_SUBTYPE_AUTH != uc_frame_sub_type) ||
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
            ul_ret = hmac_roam_ft_notify_wpas(pst_hmac_vap, puc_mac_hdr, pst_rx_ctrl->st_rx_info.us_frame_len);
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
    hmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint8                       *puc_mac_hdr;
    oal_uint8                       *puc_payload;
    oal_uint16                       us_msg_len;
    mac_status_code_enum_uint16      en_asoc_status;
    oal_uint8                        auc_bss_addr[WLAN_MAC_ADDR_LEN];
    oal_uint8                        uc_frame_sub_type;

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_ASSOC_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_assoc_rsp::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
    puc_payload    = puc_mac_hdr + pst_rx_ctrl->st_rx_info.uc_mac_header_len;
    us_msg_len     = pst_rx_ctrl->st_rx_info.us_frame_len - pst_rx_ctrl->st_rx_info.uc_mac_header_len;

    /* mac地址校验 */
    mac_get_address3(puc_mac_hdr, auc_bss_addr);
    if (oal_compare_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, auc_bss_addr))
    {
        return OAL_SUCC;
    }

    /* assoc帧校验，错误帧处理 */
    uc_frame_sub_type = mac_get_frame_sub_type(puc_mac_hdr);
    en_asoc_status = mac_get_asoc_status(puc_payload);

    if ((WLAN_FC0_SUBTYPE_REASSOC_RSP   != uc_frame_sub_type)
         && (WLAN_FC0_SUBTYPE_ASSOC_RSP != uc_frame_sub_type))
    {
        return OAL_SUCC;
    }

    /* 关联响应帧长度校验 */
    if (pst_rx_ctrl->st_rx_info.us_frame_len <= OAL_ASSOC_RSP_IE_OFFSET)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                         "{hmac_roam_process_assoc_rsp::rsp ie length error, us_frame_len[%d].}", pst_rx_ctrl->st_rx_info.us_frame_len);
        return OAL_ERR_CODE_ROAM_FRAMER_LEN;
    }

    if(MAC_SUCCESSFUL_STATUSCODE != en_asoc_status)
    {
        return OAL_SUCC;
    }

    ul_ret = hmac_process_assoc_rsp(pst_hmac_vap, pst_hmac_user, puc_mac_hdr, puc_payload, us_msg_len);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                         "{hmac_roam_process_assoc_rsp::hmac_process_assoc_rsp failed[%d].}", ul_ret);
        return ul_ret;
    }

    /* user已经关联上，抛事件给DMAC，在DMAC层挂用户算法钩子 */
    hmac_user_add_notify_alg(&(pst_hmac_vap->st_vap_base_info), pst_hmac_user->st_user_base_info.us_assoc_id);

    /* 上报关联成功消息给APP */
    ul_ret = hmac_roam_connect_notify_wpas(pst_hmac_vap, puc_mac_hdr, pst_rx_ctrl->st_rx_info.us_frame_len);
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
           if (OAL_TRUE == pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_fast_bss_trans_cfg.en_dot11FastBSSTransitionActivated)
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
    hmac_rx_ctl_stru                *pst_rx_ctrl;
    oal_uint8                       *puc_mac_hdr;
    oal_uint8                       *puc_payload;
    oal_uint8                        auc_bss_addr[WLAN_MAC_ADDR_LEN];
    oal_uint8                        uc_frame_sub_type;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_HANDSHAKING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_process_action::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    pst_hmac_user = pst_roam_info->pst_hmac_user;

    pst_crx_event  = (dmac_wlan_crx_event_stru *)p_param;
    pst_rx_ctrl    = (hmac_rx_ctl_stru *)oal_netbuf_cb(pst_crx_event->pst_netbuf);
    puc_mac_hdr    = (oal_uint8 *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr;
    puc_payload    = puc_mac_hdr + pst_rx_ctrl->st_rx_info.uc_mac_header_len;

    /* mac地址校验 */
    mac_get_address3(puc_mac_hdr, auc_bss_addr);
    if (oal_compare_mac_addr(pst_hmac_user->st_user_base_info.auc_user_mac_addr, auc_bss_addr))
    {
        return OAL_SUCC;
    }

    uc_frame_sub_type = mac_get_frame_sub_type(puc_mac_hdr);
    if (WLAN_FC0_SUBTYPE_ACTION != uc_frame_sub_type)
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
                ul_ret = hmac_mgmt_rx_addba_req(pst_hmac_vap, pst_hmac_user, puc_payload);
                break;

            case MAC_BA_ACTION_ADDBA_RSP:
                ul_ret = hmac_mgmt_rx_addba_rsp(pst_hmac_vap, pst_hmac_user, puc_payload);
                break;

            case MAC_BA_ACTION_DELBA:
                ul_ret = hmac_mgmt_rx_delba(pst_hmac_vap, pst_hmac_user, puc_payload);
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

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_HANDSHAKING);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_connect_succ::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    hmac_roam_connect_change_state(pst_roam_info, ROAM_CONNECT_STATE_UP);

    /* 删除定时器 */
    hmac_roam_connect_del_timer(pst_roam_info);

    /* 通知ROAM主状态机 */
    hmac_roam_connect_complete(pst_roam_info->pst_hmac_vap, OAL_SUCC);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 hmac_roam_auth_timeout(hmac_roam_info_stru *pst_roam_info, oal_void *p_param)
{
    oal_uint32              ul_ret;
    hmac_vap_stru          *pst_hmac_vap;

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_AUTH_COMP);
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

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_ASSOC_COMP);
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

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_HANDSHAKING);
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

    ul_ret = hmac_roam_connect_check_state(pst_roam_info, ROAM_MAIN_STATE_CONNECTING, ROAM_CONNECT_STATE_WAIT_FT_COMP);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ROAM, "{hmac_roam_ft_timeout::check_state fail[%d]!}", ul_ret);
        return ul_ret;
    }

    pst_hmac_vap  = pst_roam_info->pst_hmac_vap;
    if(pst_hmac_vap->bit_11r_enable != OAL_TRUE)
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
    hmac_roam_alg_add_blacklist(pst_roam_info, pst_roam_info->st_connect.pst_bss_dscr->auc_bssid, ROAM_BLACKLIST_TYPE_REJECT_AP);
    OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                     "{hmac_roam_connect_fail::hmac_roam_alg_add_blacklist!}");

    /* 通知ROAM主状态机，BSS回退由主状态机完成 */

    if(ROAM_CONNECT_STATE_HANDSHAKING == connect_state)
    {
        hmac_roam_connect_complete(pst_hmac_vap, OAL_ERR_CODE_ROAM_HANDSHAKE_FAIL);
    }
    else
    {
        hmac_roam_connect_complete(pst_hmac_vap, OAL_FAIL);
    }

    return OAL_SUCC;
}

oal_uint32 hmac_roam_connect_start(hmac_vap_stru *pst_hmac_vap, mac_bss_dscr_stru *pst_bss_dscr)
{
    hmac_roam_info_stru                             *pst_roam_info;
#ifdef _PRE_WLAN_FEATURE_11R
    wlan_mib_Dot11FastBSSTransitionConfigEntry_stru *pst_wlan_mib_ft_cfg;
#endif //_PRE_WLAN_FEATURE_11R

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }


    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start::roam_info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    /* 漫游开关没有开时，不处理tbtt中断 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start::roam disabled!}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    pst_roam_info->st_connect.pst_bss_dscr = pst_bss_dscr;
    pst_roam_info->st_connect.uc_auth_num  = 0;
    pst_roam_info->st_connect.uc_assoc_num = 0;
    pst_roam_info->st_connect.uc_ft_num    = 0;

#ifdef _PRE_WLAN_FEATURE_11R
    if(OAL_TRUE == pst_hmac_vap->bit_11r_enable)
    {
        pst_wlan_mib_ft_cfg = &pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_fast_bss_trans_cfg;
        if ((OAL_TRUE == pst_wlan_mib_ft_cfg->en_dot11FastBSSTransitionActivated) &&
            (OAL_TRUE == pst_wlan_mib_ft_cfg->en_dot11FTOverDSActivated))
        {
            return hmac_roam_connect_fsm_action(pst_roam_info, ROAM_CONNECT_FSM_EVENT_FT_OVER_DS, (oal_void *)pst_bss_dscr);
        }
    }
#endif //_PRE_WLAN_FEATURE_11R

    return hmac_roam_connect_fsm_action(pst_roam_info, ROAM_CONNECT_FSM_EVENT_START, (oal_void *)pst_bss_dscr);

}


oal_uint32 hmac_roam_connect_stop(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru              *pst_roam_info;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start::vap null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }


    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (pst_roam_info == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_start::roam_info null!}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }
    pst_roam_info->st_connect.en_state = ROAM_CONNECT_STATE_INIT;
    return OAL_SUCC;
}


oal_void  hmac_roam_connect_rx_mgmt(hmac_vap_stru *pst_hmac_vap, dmac_wlan_crx_event_stru *pst_crx_event)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_uint32                        ul_ret;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_rx_mgmt::vap null!}");
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

    ul_ret = hmac_roam_connect_fsm_action(pst_roam_info, ROAM_CONNECT_FSM_EVENT_MGMT_RX, (oal_void *)pst_crx_event);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_rx_mgmt::MGMT_RX FAIL[%d]!}", ul_ret);
    }

    return;
}


oal_void hmac_roam_connect_key_done(hmac_vap_stru *pst_hmac_vap)
{
    hmac_roam_info_stru              *pst_roam_info;
    oal_uint32                        ul_ret;

    if (pst_hmac_vap == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_key_done::vap null!}");
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

    ul_ret = hmac_roam_connect_fsm_action(pst_roam_info, ROAM_CONNECT_FSM_EVENT_KEY_DONE, OAL_PTR_NULL);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_key_done::KEY_DONE FAIL[%d]!}", ul_ret);
    }
    OAM_WARNING_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_key_done::KEY_DONE !}");

    return;
}

#endif //_PRE_WLAN_FEATURE_ROAM
#ifdef _PRE_WLAN_FEATURE_11R

oal_uint32 hmac_roam_connect_ft_reassoc(hmac_vap_stru *pst_hmac_vap)
{
    wlan_mib_Dot11FastBSSTransitionConfigEntry_stru *pst_wlan_mib_ft_cfg;
    hmac_roam_info_stru                             *pst_roam_info;
    hmac_join_req_stru                               st_join_req;
    oal_uint32                                       ul_ret;
    mac_bss_dscr_stru                               *pst_bss_dscr;

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_ft_reassoc::param null.}");
        return OAL_ERR_CODE_ROAM_INVALID_VAP;
    }

    pst_roam_info = (hmac_roam_info_stru *)pst_hmac_vap->pul_roam_info;
    if (OAL_PTR_NULL == pst_roam_info)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ROAM, "{hmac_roam_connect_ft_reassoc::pul_roam_info null.}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 漫游开关没有开时，不处理 */
    if (0 == pst_roam_info->uc_enable)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM, "{hmac_roam_connect_ft_reassoc::roam disabled.}");
        return OAL_ERR_CODE_ROAM_DISABLED;
    }

    /* 主状态机为非CONNECTING状态，失败 */
    if (pst_roam_info->en_main_state != ROAM_MAIN_STATE_CONNECTING)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_connect_ft_reassoc::main state[%d] error.}", pst_roam_info->en_main_state);
        return OAL_ERR_CODE_ROAM_STATE_UNEXPECT;
    }

    /* CONNECT状态机为非WAIT_JOIN状态，失败 */
    if (pst_roam_info->st_connect.en_state != ROAM_CONNECT_STATE_WAIT_ASSOC_COMP)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_connect_ft_reassoc::connect state[%d] error.}", pst_roam_info->st_connect.en_state);
        return OAL_ERR_CODE_ROAM_STATE_UNEXPECT;
    }

    pst_bss_dscr = pst_roam_info->st_connect.pst_bss_dscr;
    if (OAL_PTR_NULL == pst_bss_dscr)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_connect_ft_reassoc::pst_bss_dscr is null.}");
        return OAL_ERR_CODE_ROAM_NO_VALID_BSS;
    }

    pst_wlan_mib_ft_cfg = &pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_fast_bss_trans_cfg;
    if ((OAL_TRUE == pst_wlan_mib_ft_cfg->en_dot11FastBSSTransitionActivated) &&
        (OAL_TRUE == pst_wlan_mib_ft_cfg->en_dot11FTOverDSActivated))
    {
        oal_memcopy(pst_hmac_vap->auc_supp_rates,pst_bss_dscr->auc_supp_rates ,pst_bss_dscr->uc_num_supp_rates);
        pst_hmac_vap->uc_rs_nrates = pst_bss_dscr->uc_num_supp_rates;

        /* 配置join参数 */
        hmac_prepare_join_req(&st_join_req, pst_bss_dscr);

        ul_ret = hmac_sta_update_join_req_params(pst_hmac_vap, &st_join_req);
        if (OAL_SUCC != ul_ret)
        {
            OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_SCAN,
                           "{hmac_roam_connect_ft_reassoc::hmac_sta_update_join_req_params fail[%d].}", ul_ret);
            return ul_ret;
        }
    }

    /* 发送关联请求 */
    ul_ret = hmac_roam_send_reassoc_req(pst_roam_info);
    if (OAL_SUCC != ul_ret)
    {
        OAM_ERROR_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ROAM,
                       "{hmac_roam_connect_ft_reassoc::hmac_roam_send_assoc_req failed[%d].}", ul_ret);
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

