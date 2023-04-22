


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#ifdef _PRE_WLAN_FEATURE_WMMAC
#include "hmac_wmmac.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_encap_frame_sta.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_WMMAC_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/
OAL_STATIC oal_uint32  hmac_mgmt_tx_delts(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, mac_wmm_tspec_stru *pst_delts_args);


OAL_STATIC oal_uint16  hmac_mgmt_encap_addts_req(
                    hmac_vap_stru                      *pst_vap,
                    oal_uint8                          *puc_data,
                    mac_wmm_tspec_stru                 *pst_addts_args)
{
    oal_uint16 us_index = 0;

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

    /* Frame Control Field 中只需要设置Type/Subtype值，其他设置为0 */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* DA is address of STA requesting association */
    oal_set_mac_addr(puc_data + 4, pst_vap->st_vap_base_info.auc_bssid);

    /* SA的值为dot11MACAddress的值 */
    oal_set_mac_addr(puc_data + 10, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    oal_set_mac_addr(puc_data + 16, pst_vap->st_vap_base_info.auc_bssid);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*-----------------------------------------------------------------------*/
    /* |MAC HEADER| Category| Action| Dialog token| Status| Elements| FCS|   */
    /*-----------------------------------------------------------------------*/
    /* |24/30     |1        |1      |1            |1      |         |4   |   */
    /*-----------------------------------------------------------------------*/
    /*                                                                       */
    /*************************************************************************/

    /* 将索引指向frame body起始位置 */
    us_index = MAC_80211_FRAME_LEN;

    /* 设置Category */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_WMMAC_QOS;

    /* 设置Action */
    puc_data[us_index++] = MAC_WMMAC_ACTION_ADDTS_REQ;

    /* 设置Dialog Token */
    puc_data[us_index++] = pst_vap->uc_ts_dialog_token;

    /* Status 固定值为0 */
    puc_data[us_index++] = 0;

    /* 设置WMM AC参数 */
    us_index += mac_set_wmmac_ie_sta((oal_void *)pst_vap, puc_data + us_index, pst_addts_args);

    /* 返回的帧长度中不包括FCS */
    return us_index;
}

OAL_STATIC oal_uint32  hmac_mgmt_tx_addts_req_timeout(oal_void *p_arg)
{
    hmac_vap_stru                      *pst_vap = OAL_PTR_NULL;         /* vap指针 */
    hmac_user_stru                     *pst_hmac_user;
    mac_ts_stru                        *pst_ts_args;
    mac_wmm_tspec_stru                  st_addts_args;

    if (OAL_PTR_NULL == p_arg)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WMMAC, "{hmac_mgmt_tx_addts_req_timeout::p_arg null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ts_args = (mac_ts_stru *)p_arg;

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_ts_args->us_mac_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_ERROR_LOG0(0, OAM_SF_WMMAC, "{hmac_mgmt_tx_addts_req_timeout::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_ts_args->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_vap))
    {
        OAM_ERROR_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_addts_req_timeout::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 生成DELTS帧,只需要改变TSID */
    st_addts_args.ts_info.bit_tsid      = pst_ts_args->uc_tsid;
    st_addts_args.ts_info.bit_user_prio = pst_ts_args->uc_up;

    return hmac_mgmt_tx_delts(pst_vap, pst_hmac_user, &st_addts_args);
}


OAL_STATIC oal_uint32  hmac_mgmt_tx_addts_req(
                hmac_vap_stru                      *pst_hmac_vap,
                hmac_user_stru                     *pst_hmac_user,
                mac_wmm_tspec_stru                 *pst_addts_args)
{

    mac_vap_stru               *pst_mac_vap;
    mac_device_stru            *pst_device;
    oal_netbuf_stru            *pst_addts_req;
    oal_uint16                  us_frame_len;
    frw_event_mem_stru         *pst_event_mem;      /* 申请事件返回的内存指针 */
    oal_uint32                  ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_tx_ctl_stru            *pst_tx_ctl;
#endif
    frw_event_stru             *pst_hmac_to_dmac_ctx_event;
    dmac_tx_event_stru         *pst_tx_event;
    dmac_ctx_action_event_stru  st_wlan_ctx_action;
    oal_uint8                   uc_ac_num;


    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    /* 确定vap处于工作状态 */
    if (MAC_VAP_STATE_BUTT == pst_mac_vap->en_vap_state)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_addts_req:: vap has been down/del, vap_state[%d].}",
                         pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }

    /* 获取device结构 */
    pst_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_addba_req::pst_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请ADDTS_REQ管理帧内存 */
    pst_addts_req = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_addts_req)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_addts_req::pst_addts_req null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_addts_req, OAL_TRUE);
    OAL_MEMZERO(oal_netbuf_cb(pst_addts_req), OAL_NETBUF_CB_SIZE());

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_addts_req);
#endif

    OAL_NETBUF_PREV(pst_addts_req) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_addts_req) = OAL_PTR_NULL;

    pst_hmac_vap->uc_ts_dialog_token++;

    /* 调用封装管理帧接口 */
    us_frame_len = hmac_mgmt_encap_addts_req(pst_hmac_vap,oal_netbuf_data(pst_addts_req), pst_addts_args);

    OAL_MEMZERO((oal_uint8*)&st_wlan_ctx_action, OAL_SIZEOF(st_wlan_ctx_action));
    /* 赋值要传入Dmac的信息 */
    st_wlan_ctx_action.us_frame_len        = us_frame_len;
    st_wlan_ctx_action.uc_hdr_len          = MAC_80211_FRAME_LEN;
    st_wlan_ctx_action.en_action_category  = MAC_ACTION_CATEGORY_WMMAC_QOS;
    st_wlan_ctx_action.uc_action           = MAC_WMMAC_ACTION_ADDTS_REQ;
    st_wlan_ctx_action.us_user_idx         = pst_hmac_user->st_user_base_info.us_assoc_id;
    st_wlan_ctx_action.uc_tidno            = pst_addts_args->ts_info.bit_user_prio;
    st_wlan_ctx_action.uc_ts_dialog_token  = pst_hmac_vap->uc_ts_dialog_token;
    st_wlan_ctx_action.uc_initiator        = pst_addts_args->ts_info.bit_direction;
    st_wlan_ctx_action.uc_tsid             = pst_addts_args->ts_info.bit_tsid;

    oal_memcopy((oal_uint8 *)(oal_netbuf_data(pst_addts_req) + us_frame_len), (oal_uint8 *)&st_wlan_ctx_action, OAL_SIZEOF(dmac_ctx_action_event_stru));
    oal_netbuf_put(pst_addts_req, (us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru)));

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_tx_ctl->us_mpdu_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);
#endif

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_addts_req::pst_event_mem null.}");
        oal_netbuf_free(pst_addts_req);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_to_dmac_ctx_event = (frw_event_stru *)pst_event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(pst_hmac_to_dmac_ctx_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_ACTION,
                       OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    pst_tx_event = (dmac_tx_event_stru *)(pst_hmac_to_dmac_ctx_event->auc_event_data);
    pst_tx_event->pst_netbuf    = pst_addts_req;
    pst_tx_event->us_frame_len  = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_addts_req::frw_event_dispatch_event failed[%d].}", ul_ret);
        oal_netbuf_free(pst_addts_req);
        FRW_EVENT_FREE(pst_event_mem);
        return ul_ret;
    }

    FRW_EVENT_FREE(pst_event_mem);

    /* 更新对应的TS信息 */
    uc_ac_num = WLAN_WME_TID_TO_AC(pst_addts_args->ts_info.bit_user_prio);
    pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].en_ts_status        = MAC_TS_INPROGRESS;
    pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_ts_dialog_token  = pst_hmac_vap->uc_ts_dialog_token;
    pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_tsid             = pst_addts_args->ts_info.bit_tsid;
#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    /* 启动ADDTS REQ超时计时器 */
    FRW_TIMER_CREATE_TIMER(&pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].st_addts_timer,
                           hmac_mgmt_tx_addts_req_timeout,
                           WLAN_ADDTS_TIMEOUT,
                           &pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num],
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_device->ul_core_id);
#endif

     return OAL_SUCC;
}


OAL_STATIC oal_uint16  hmac_mgmt_encap_addts_rsp(hmac_vap_stru     *pst_vap,
                                                oal_uint8         *puc_data,
                                                oal_uint8         *puc_addts_req,
                                                oal_uint16         us_frame_len)
{
    oal_uint16      us_index  = 0;
    oal_uint8      *puc_sa    = OAL_PTR_NULL;


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
    mac_rx_get_sa((mac_ieee80211_frame_stru *)puc_addts_req, &puc_sa);

    /* 拷贝整个ADDTS REQ帧 */
    oal_memcopy(puc_data, puc_addts_req, us_frame_len);

    /* Frame Control Field 中只需要设置Type/Subtype值，其他设置为0 */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);

    /* DA is ADDTS Request frame's SA */
    oal_set_mac_addr(puc_data + 4, puc_sa);

    /* SA的值为dot11MACAddress的值 */
    oal_set_mac_addr(puc_data + 10, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
    oal_set_mac_addr(puc_data + 16, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*-----------------------------------------------------------------------*/
    /* |MAC HEADER| Category| Action| Dialog token| Status| Elements| FCS|   */
    /*-----------------------------------------------------------------------*/
    /* |24/30     |1        |1      |1            |1      |         |4   |   */
    /*-----------------------------------------------------------------------*/
    /*                                                                       */
    /*************************************************************************/

    /* 将索引指向frame body起始位置 */
    us_index = MAC_80211_FRAME_LEN;

    /* 设置Category */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_WMMAC_QOS;

    /* 设置Action */
    puc_data[us_index++] = MAC_WMMAC_ACTION_ADDTS_RSP;

    /* 设置Dialog Token */
    us_index++;

    puc_data[us_index++] = 0; /* Status 为Succ */

    /* 返回的帧长度中不包括FCS */
    return us_frame_len;
}

OAL_STATIC oal_uint16  hmac_mgmt_encap_delts(hmac_vap_stru    *pst_vap,
                                          oal_uint8        *puc_data,
                                          oal_uint8        *puc_addr,
                                          oal_uint8         uc_tsid)
{
    oal_uint16            us_index = 0;
    oal_uint16            us_ie_len;
    mac_wmm_tspec_stru    st_addts_args;


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

    /* Frame Control Field 中只需要设置Type/Subtype值，其他设置为0 */
    mac_hdr_set_frame_control(puc_data, WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_MGT | WLAN_FC0_SUBTYPE_ACTION);
    if (IS_STA(&(pst_vap->st_vap_base_info)))
    {
        /* DA is address of STA requesting association */
        oal_set_mac_addr(puc_data + 4, pst_vap->st_vap_base_info.auc_bssid);

        /* SA的值为dot11MACAddress的值 */
        oal_set_mac_addr(puc_data + 10, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
        oal_set_mac_addr(puc_data + 16, pst_vap->st_vap_base_info.auc_bssid);
    }
    else if (IS_AP(&(pst_vap->st_vap_base_info)))
    {
        /* DA is puc_addr, user's address */
        oal_set_mac_addr(puc_data + 4, puc_addr);

        /* SA的值为dot11MACAddress的值 */
        oal_set_mac_addr(puc_data + 10, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);
        oal_set_mac_addr(puc_data + 16, pst_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

    }

    /*************************************************************************/
    /*                Set the contents of the frame body                     */
    /*-----------------------------------------------------------------------*/
    /* |MAC HEADER| Category| Action| Dialog token| Status| Elements| FCS|   */
    /*-----------------------------------------------------------------------*/
    /* |24/30     |1        |1      |1            |1      |         |4   |   */
    /*-----------------------------------------------------------------------*/
    /*                                                                       */
    /*************************************************************************/

    /* 将索引指向frame body起始位置 */
    us_index = MAC_80211_FRAME_LEN;

    /* 设置Category */
    puc_data[us_index++] = MAC_ACTION_CATEGORY_WMMAC_QOS;

    /* 设置Action */
    puc_data[us_index++] = MAC_WMMAC_ACTION_DELTS;

    /* 设置Dialog Token */
    puc_data[us_index++] = 0;

    puc_data[us_index++] = 0; /* Status 固定值为0 */

    OAL_MEMZERO(&st_addts_args, OAL_SIZEOF(st_addts_args));
    st_addts_args.ts_info.bit_tsid = uc_tsid;

    /* 设置WMM AC参数，除了TSID，delts其他值均为0 */
    us_ie_len = mac_set_wmmac_ie_sta((oal_void *)pst_vap, puc_data + us_index, &st_addts_args);

    us_index += us_ie_len;

    /* 返回的帧长度中不包括FCS */
    return us_index;
}

OAL_STATIC oal_uint32  hmac_mgmt_tx_delts(
                       hmac_vap_stru                      *pst_hmac_vap,
                       hmac_user_stru                     *pst_hmac_user,
                       mac_wmm_tspec_stru                 *pst_delts_args)
{

    mac_vap_stru               *pst_mac_vap;
    oal_netbuf_stru            *pst_delts;
    oal_uint16                  us_frame_len;
    frw_event_mem_stru         *pst_event_mem;      /* 申请事件返回的内存指针 */
    oal_uint32                  ul_ret;
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    mac_tx_ctl_stru            *pst_tx_ctl;
#endif
    frw_event_stru             *pst_hmac_to_dmac_ctx_event;
    dmac_tx_event_stru         *pst_tx_event;
    dmac_ctx_action_event_stru  st_wlan_ctx_action;
    oal_uint8                   uc_ac_num = 0;

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);
    /* 确定vap处于工作状态 */
    if (MAC_VAP_STATE_BUTT == pst_mac_vap->en_vap_state)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_delts:: vap has been down/del, vap_state[%d].}",pst_mac_vap->en_vap_state);
        return OAL_FAIL;
    }
    /* 遍历所有AC，找到对应TSID的AC和UP */
    if (IS_STA(&pst_hmac_vap->st_vap_base_info))
    {
        for (uc_ac_num = 0; uc_ac_num < WLAN_WME_AC_BUTT; uc_ac_num++)
        {
            if(pst_delts_args->ts_info.bit_tsid == pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_tsid)
            {
                if (pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].en_ts_status == MAC_TS_NONE)
                {
                    return OAL_SUCC;
                }
                break;
            }
        }
        /* 如果找到了对应的TSID，则以对应的UP发送；如果没找到，delts默认以up7发送 */
        if(uc_ac_num < WLAN_WME_AC_BUTT)
        {
            pst_delts_args->ts_info.bit_user_prio = pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_up;
        }
        else
        {
            pst_delts_args->ts_info.bit_user_prio = 0x7;
        }
    }

    /* 申请DELTS管理帧内存 */
    pst_delts = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_delts)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_delts::pst_addts_req null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_delts, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_delts), OAL_NETBUF_CB_SIZE());

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_delts);
#endif

    OAL_NETBUF_PREV(pst_delts) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_delts) = OAL_PTR_NULL;

    /* 调用封装管理帧接口 */
    us_frame_len = hmac_mgmt_encap_delts(pst_hmac_vap,
                                         oal_netbuf_data(pst_delts),
                                         pst_hmac_user->st_user_base_info.auc_user_mac_addr,
                                         (oal_uint8)pst_delts_args->ts_info.bit_tsid);

    OAL_MEMZERO((oal_uint8*)&st_wlan_ctx_action, OAL_SIZEOF(st_wlan_ctx_action));

    /* 赋值要传入Dmac的信息 */
    st_wlan_ctx_action.us_frame_len        = us_frame_len;
    st_wlan_ctx_action.uc_hdr_len          = MAC_80211_FRAME_LEN;
    st_wlan_ctx_action.en_action_category  = MAC_ACTION_CATEGORY_WMMAC_QOS;
    st_wlan_ctx_action.uc_action           = MAC_WMMAC_ACTION_DELTS;
    st_wlan_ctx_action.us_user_idx         = pst_hmac_user->st_user_base_info.us_assoc_id;
    st_wlan_ctx_action.uc_tidno            = pst_delts_args->ts_info.bit_user_prio;
    st_wlan_ctx_action.uc_tsid             = pst_delts_args->ts_info.bit_tsid;

    oal_memcopy((oal_uint8 *)(oal_netbuf_data(pst_delts) + us_frame_len), (oal_uint8 *)&st_wlan_ctx_action, OAL_SIZEOF(dmac_ctx_action_event_stru));
    oal_netbuf_put(pst_delts, (us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru)));

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_tx_ctl->us_mpdu_len = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);
#endif

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_delts::pst_event_mem null.}");
        oal_netbuf_free(pst_delts);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_to_dmac_ctx_event = (frw_event_stru *)pst_event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(pst_hmac_to_dmac_ctx_event->st_event_hdr),
                       FRW_EVENT_TYPE_WLAN_CTX,
                       DMAC_WLAN_CTX_EVENT_SUB_TYPE_ACTION,
                       OAL_SIZEOF(dmac_tx_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_mac_vap->uc_chip_id,
                       pst_mac_vap->uc_device_id,
                       pst_mac_vap->uc_vap_id);

    pst_tx_event = (dmac_tx_event_stru *)(pst_hmac_to_dmac_ctx_event->auc_event_data);
    pst_tx_event->pst_netbuf    = pst_delts;
    pst_tx_event->us_frame_len  = us_frame_len + OAL_SIZEOF(dmac_ctx_action_event_stru);

    ul_ret = frw_event_dispatch_event(pst_event_mem);
    if (ul_ret != OAL_SUCC)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_tx_delts::frw_event_dispatch_event failed[%d].}", ul_ret);
        oal_netbuf_free(pst_delts);
        FRW_EVENT_FREE(pst_event_mem);
        return ul_ret;
    }

    FRW_EVENT_FREE(pst_event_mem);

    if (IS_STA(&pst_hmac_vap->st_vap_base_info)&& (uc_ac_num < WLAN_WME_AC_BUTT))
    {
        /* 更新对应的TS信息 */
        //OAL_MEMZERO(&(pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num]), OAL_SIZEOF(mac_ts_stru));
        pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].en_ts_status = MAC_TS_INIT;
        pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_tsid      = 0xFF;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_mgmt_rx_addts_rsp(
                hmac_vap_stru              *pst_hmac_vap,
                hmac_user_stru             *pst_hmac_user,
                oal_uint8                  *puc_payload)
{
    mac_vap_stru               *pst_mac_vap;
    oal_uint8                   uc_user_prio;
    oal_uint8                   uc_ac_num;
    oal_uint8                   uc_tsid;
    oal_uint8                   uc_dialog_token;
    mac_wmm_tspec_stru          st_tspec_args;
    mac_wmm_tspec_stru         *pst_tspec_info;
    mac_ts_stru                *pst_ts;


    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == puc_payload))
    {
        OAM_ERROR_LOG3(0, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::null param, %p %p %p.}", pst_hmac_vap, pst_hmac_user, puc_payload);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    /*************************************************************************/
    /*                ADDTS RSP frame body                     */
    /*-----------------------------------------------------------------------*/
    /* |MAC HEADER| Category| Action| Dialog token| Status| Elements| FCS|   */
    /*-----------------------------------------------------------------------*/
    /* |24/30     |1        |1      |1            |1      |         |4   |   */
    /*-----------------------------------------------------------------------*/
    /*                                                                       */
    /*************************************************************************/

    pst_tspec_info = (mac_wmm_tspec_stru *)(puc_payload + 12);
    uc_user_prio = pst_tspec_info->ts_info.bit_user_prio;

    /* 协议支持tid为0~15,02只支持tid0~7 */
    if(uc_user_prio >= WLAN_TID_MAX_NUM)
    {
        /* 对于tid > 7的resp直接忽略 */
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::addts rsp tsid[%d]} token[%d] status[%d]", uc_user_prio, puc_payload[2], puc_payload[3]);
        return OAL_SUCC;
    }
    uc_ac_num = WLAN_WME_TID_TO_AC(uc_user_prio);
    uc_tsid = pst_tspec_info->ts_info.bit_tsid;

    /* 获得之前保存的ts信息 */
    pst_ts   = &(pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num]);
    uc_dialog_token = puc_payload[2];

    if(pst_ts->uc_tsid != uc_tsid)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::addba rsp tsid[%d],old_tsid[%d]} ", uc_tsid, pst_ts->uc_tsid);
        return OAL_SUCC;
    }

    if(uc_dialog_token != pst_ts->uc_ts_dialog_token)
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::addts rspuc_dialog_token wrong.rsp dialog[%d], req dialog[%d]}",uc_dialog_token, pst_ts->uc_ts_dialog_token);
        return OAL_SUCC;
    }

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_ts->st_addts_timer);
#endif

    /* 如果对应TSID的TS未建立，则发送DELTS */
    if(MAC_TS_INIT == pst_ts->en_ts_status)
    {
         /* 发送DELTS帧 */
         st_tspec_args.ts_info.bit_tsid       = uc_tsid;
         st_tspec_args.ts_info.bit_user_prio  = uc_user_prio;
         st_tspec_args.ts_info.bit_acc_policy = 1;
         hmac_mgmt_tx_delts(pst_hmac_vap, pst_hmac_user, &st_tspec_args);
         OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::addts rsp is received when ba status is DMAC_TS_INIT.tsid[%d]}", uc_tsid);
         return OAL_SUCC;
    }

    /* 如果之前的TS已经存在，又收到addts rsp的拒绝建立，则保持原来的状态，不做处理 */
    if((MAC_TS_SUCCESS == pst_ts->en_ts_status) && (MAC_SUCCESSFUL_STATUSCODE != puc_payload[3]))
    {
        OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::addts rsp is received when ts status is already successful.tsid[%d], status[%d]}",uc_tsid, pst_ts->en_ts_status);
        return OAL_SUCC;
    }

    /* 根据ADDTS RSP信息，更新dmac TS状态 */
    if(MAC_SUCCESSFUL_STATUSCODE == puc_payload[3])
    {
        pst_ts->en_ts_status = MAC_TS_SUCCESS;
    }
    else
    {
        OAL_MEMZERO(pst_ts, OAL_SIZEOF(mac_ts_stru));
        pst_ts->en_ts_status = MAC_TS_INIT;
    }

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::medium_time[%d], tsid[%d], uc_up[%d], en_direction[%d].}",
                     pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].ul_medium_time,
                     pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_tsid,
                     pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_up,
                     pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].en_direction);

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_rsp::ts status[%d], dialog_token[%d], vap id[%d], ac num[%d].}",
                     pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].en_ts_status,
                     pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_ts_dialog_token,
                     pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num].uc_vap_id,
                     uc_ac_num);
    return OAL_SUCC;
}

oal_uint32  hmac_mgmt_rx_delts(
                hmac_vap_stru              *pst_hmac_vap,
                hmac_user_stru             *pst_hmac_user,
                oal_uint8                  *puc_payload)
{
    mac_vap_stru               *pst_mac_vap;
    oal_uint8                   uc_ac_num;
    oal_uint8                   uc_tsid;
    mac_wmm_tspec_stru         *pst_tspec_info;
    mac_ts_stru                *pst_ts;
    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_hmac_user) || (OAL_PTR_NULL == puc_payload))
    {
        OAM_ERROR_LOG3(0, OAM_SF_WMMAC, "{hmac_mgmt_rx_delts::null param, %p %p %p.}", pst_hmac_vap, pst_hmac_user, puc_payload);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &(pst_hmac_vap->st_vap_base_info);

    /*************************************************************************/
    /*                DELTS frame body                     */
    /*-----------------------------------------------------------------------*/
    /* |MAC HEADER| Category| Action| Dialog token| Status| Elements| FCS|   */
    /*-----------------------------------------------------------------------*/
    /* |24/30     |1        |1      |1            |1      |         |4   |   */
    /*-----------------------------------------------------------------------*/
    /*                                                                       */
    /*************************************************************************/

    pst_tspec_info = (mac_wmm_tspec_stru *)(puc_payload + 12);
    uc_tsid        = pst_tspec_info->ts_info.bit_tsid;

    /* 协议支持tid为0~15,02只支持tid0~7 */
    if(uc_tsid >= WLAN_TID_MAX_NUM)
    {
        /* 对于tid > 7的resp直接忽略 */
        OAM_WARNING_LOG3(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_delts::tsid[%d]} token[%d] status[%d]", uc_tsid, puc_payload[2], puc_payload[3]);
        return OAL_SUCC;
    }
    /* 遍历所有AC，找到对应的TSID，清空TS信息 */
    for(uc_ac_num = 0; uc_ac_num < WLAN_WME_AC_BUTT; uc_ac_num++)
    {
        /* 获得之前保存的ts信息 */
        pst_ts   = &(pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_num]);
        if(pst_ts->uc_tsid != uc_tsid)
        {
            OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_delts::tsid[%d],old_tsid[%d]} ", uc_tsid, pst_ts->uc_tsid);
            continue;
        }

        /* 清空已保存的TS信息 */
        if (pst_ts->st_addts_timer.en_is_registerd == OAL_TRUE)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_ts->st_addts_timer));
        }
        OAL_MEMZERO(pst_ts, OAL_SIZEOF(mac_ts_stru));
        if(1 == pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_qap_edac[uc_ac_num].en_dot11QAPEDCATableMandatory)
        {
            pst_ts->en_ts_status = MAC_TS_INIT;
        }
        else
        {
            pst_ts->en_ts_status = MAC_TS_NONE;
        }
        pst_ts->uc_tsid      = 0xFF;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_mgmt_rx_addts_req_frame(hmac_vap_stru *pst_hmac_vap, oal_netbuf_stru *pst_netbuf)
{
    mac_user_stru              *pst_mac_user;
    dmac_rx_ctl_stru           *pst_rx_ctrl;
    oal_uint16                  us_user_idx = 0;
    oal_uint8                   auc_sta_addr[WLAN_MAC_ADDR_LEN];
    oal_uint32                  ul_ret;

    oal_netbuf_stru            *pst_addts_rsp;
    mac_tx_ctl_stru            *pst_tx_ctl;
    oal_uint16                  us_frame_len;

    if ((OAL_PTR_NULL == pst_hmac_vap) || (OAL_PTR_NULL == pst_netbuf))
    {
        OAM_ERROR_LOG2(0, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_req_frame::pst_hmac_vap = [%x], pst_netbuf = [%x]!}\r\n",
                       pst_hmac_vap, pst_netbuf);

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_rx_ctrl = (dmac_rx_ctl_stru *)oal_netbuf_cb(pst_netbuf);

    mac_get_address2((oal_uint8 *)pst_rx_ctrl->st_rx_info.pul_mac_hdr_start_addr, auc_sta_addr);

    ul_ret = mac_vap_find_user_by_macaddr(&(pst_hmac_vap->st_vap_base_info), auc_sta_addr, &us_user_idx);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC,
                         "{hmac_mgmt_rx_addts_req_frame::mac_vap_find_user_by_macaddr failed[%d].}", ul_ret);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_user = (mac_user_stru *)mac_res_get_mac_user(us_user_idx);
    if (OAL_PTR_NULL == pst_mac_user)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_req_frame::pst_mac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 申请ADDTS_RSP管理帧内存 */
    pst_addts_rsp = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);
    if (OAL_PTR_NULL == pst_addts_rsp)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_req_frame::pst_addts_rsp null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAL_MEM_NETBUF_TRACE(pst_addts_rsp, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_addts_rsp), OAL_NETBUF_CB_SIZE());

    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_addts_rsp);

    OAL_NETBUF_PREV(pst_addts_rsp) = OAL_PTR_NULL;
    OAL_NETBUF_NEXT(pst_addts_rsp) = OAL_PTR_NULL;

    /* 调用封装管理帧接口 */
    us_frame_len = hmac_mgmt_encap_addts_rsp(pst_hmac_vap, oal_netbuf_data(pst_addts_rsp), oal_netbuf_data(pst_netbuf), pst_rx_ctrl->st_rx_info.us_frame_len);
    if (0 == us_frame_len)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_req_frame::encap_addts_rsp, but frame len is zero.}");
        oal_netbuf_free(pst_addts_rsp);
        return OAL_FAIL;
    }

    pst_tx_ctl->us_mpdu_len     = us_frame_len;               /* dmac发送需要的mpdu长度 */
    pst_tx_ctl->us_tx_user_idx  = us_user_idx;                /* 发送完成需要获取user结构体 */

    oal_netbuf_put(pst_addts_rsp, us_frame_len);

    /* Buffer this frame in the Memory Queue for transmission */
    ul_ret = hmac_tx_mgmt_send_event(&(pst_hmac_vap->st_vap_base_info), pst_addts_rsp, us_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_addts_rsp);
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_WMMAC, "{hmac_mgmt_rx_addts_req_frame::hmac_tx_mgmt_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }

    return OAL_SUCC;
}


oal_uint32  hmac_config_addts_req(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_wmm_tspec_stru_param_stru   *pst_addts_req;
    hmac_user_stru                      *pst_hmac_user;
    hmac_vap_stru                       *pst_hmac_vap;
    mac_wmm_tspec_stru                   st_addts_args;
    oal_uint8                            uc_ac = WLAN_WME_AC_BE;

    if (OAL_FALSE == g_en_wmmac_switch)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_addts_req::wmmac switch is false[%d].}", g_en_wmmac_switch);
        return OAL_SUCC;
    }

    pst_addts_req = (mac_cfg_wmm_tspec_stru_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_addts_req::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户对应的索引 */
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_addts_req::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*判断对应AC的ACM位，只有该AC的ACM为1时，才允许建立TS。 */
    uc_ac = WLAN_WME_TID_TO_AC(pst_addts_req->ts_info.bit_user_prio);
    if (!pst_hmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_qap_edac[uc_ac].en_dot11QAPEDCATableMandatory)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_addts_req::user prio[%d] is false.}", uc_ac);
        return OAL_SUCC;
    }

    st_addts_args.ts_info.bit_tsid       = pst_addts_req->ts_info.bit_tsid;
    st_addts_args.ts_info.bit_direction  = pst_addts_req->ts_info.bit_direction;
    st_addts_args.ts_info.bit_apsd       = pst_addts_req->ts_info.bit_apsd;
    st_addts_args.ts_info.bit_user_prio  = pst_addts_req->ts_info.bit_user_prio;
    st_addts_args.us_norminal_msdu_size  = pst_addts_req->us_norminal_msdu_size;
    st_addts_args.us_max_msdu_size       = pst_addts_req->us_max_msdu_size;
    st_addts_args.ul_min_data_rate       = pst_addts_req->ul_min_data_rate;
    st_addts_args.ul_mean_data_rate      = pst_addts_req->ul_mean_data_rate;
    st_addts_args.ul_peak_data_rate      = pst_addts_req->ul_peak_data_rate;
    st_addts_args.ul_min_phy_rate        = pst_addts_req->ul_min_phy_rate;
    st_addts_args.us_surplus_bw          = pst_addts_req->us_surplus_bw;

    /* 发送ADDTS REQ，建立TS */
    return hmac_mgmt_tx_addts_req(pst_hmac_vap, pst_hmac_user, &st_addts_args);
}


oal_uint32  hmac_config_delts(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    mac_cfg_wmm_tspec_stru_param_stru   *pst_delts;
    hmac_user_stru                      *pst_hmac_user;
    hmac_vap_stru                       *pst_hmac_vap;
    mac_wmm_tspec_stru                   st_delts_args;

    if (OAL_FALSE == g_en_wmmac_switch)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_delts::wmmac switch is false[%d].}", g_en_wmmac_switch);
        return OAL_SUCC;
    }

    pst_delts = (mac_cfg_wmm_tspec_stru_param_stru *)puc_param;

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_delts::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 获取用户对应的索引 */
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_delts::pst_hmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    st_delts_args.ts_info.bit_tsid = pst_delts->ts_info.bit_tsid;

    return hmac_mgmt_tx_delts(pst_hmac_vap, pst_hmac_user, &st_delts_args);
}


oal_uint32  hmac_config_wmmac_switch(mac_vap_stru *pst_mac_vap, oal_uint16 us_len, oal_uint8 *puc_param)
{
    oal_uint8                            uc_wmmac_switch;
    oal_uint32                           ul_ret;
    oal_uint8                            uc_ac_idx;
    hmac_user_stru                      *pst_hmac_user;

    uc_wmmac_switch = *(oal_uint8 *)puc_param;

    g_en_wmmac_switch = uc_wmmac_switch;
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_wmmac_switch::wmmac_switch is open/close = %d.vap mode = %d}",g_en_wmmac_switch,pst_mac_vap->en_vap_mode);
    /* TBD 后续功能需要时，需要增加关闭WMMAC开关时，删除所有VAP的所有用户的TS */

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_wmmac_switch::pst_hmac_user is deleted already.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    /* 关闭wmmac开关时，需要删除user下所有的TS信息 */
    if(!g_en_wmmac_switch)
    {
        for(uc_ac_idx = 0;uc_ac_idx < WLAN_WME_AC_BUTT; uc_ac_idx++)
        {
            if (pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_idx].st_addts_timer.en_is_registerd == OAL_TRUE)
            {
                FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_idx].st_addts_timer));
            }
            OAL_MEMZERO(&(pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_idx]), OAL_SIZEOF(mac_ts_stru));
            pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_idx].en_ts_status = MAC_TS_NONE;
            pst_hmac_user->st_user_base_info.st_ts_info[uc_ac_idx].uc_tsid = 0xFF;
        }

    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /***************************************************************************
    抛事件到DMAC层, 同步DMAC数据
    ***************************************************************************/
    ul_ret = hmac_config_send_event(pst_mac_vap, WLAN_CFGID_WMMAC_SWITCH, us_len, puc_param);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_wmmac_switch::hmac_config_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }
#endif

    return OAL_SUCC;
}


oal_uint32  hmac_tx_mgmt_reassoc_req(hmac_vap_stru *pst_hmac_vap_sta)
{
    oal_netbuf_stru            *pst_asoc_req_frame;
    mac_tx_ctl_stru            *pst_tx_ctl;
    hmac_user_stru             *pst_hmac_user_ap;
    oal_uint32                  ul_asoc_frame_len;
    oal_uint32                  ul_ret;
    oal_uint8                  *puc_curr_bssid;

    if (OAL_PTR_NULL == pst_hmac_vap_sta)
    {
        OAM_ERROR_LOG1(0, OAM_SF_ASSOC, "{hmac_tx_mgmt_reassoc_req,%p.}", pst_hmac_vap_sta);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hmac_vap_sta->bit_reassoc_flag = OAL_TRUE;

    pst_asoc_req_frame = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_MEM_NETBUF_SIZE2, OAL_NETBUF_PRIORITY_MID);

    if (OAL_PTR_NULL == pst_asoc_req_frame)
    {
        OAM_ERROR_LOG0(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_tx_mgmt_reassoc_req::pst_asoc_req_frame null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    OAL_MEM_NETBUF_TRACE(pst_asoc_req_frame, OAL_TRUE);

    OAL_MEMZERO(oal_netbuf_cb(pst_asoc_req_frame), OAL_NETBUF_CB_SIZE());

    /* 将mac header清零 */
    OAL_MEMZERO((oal_uint8 *)oal_netbuf_header(pst_asoc_req_frame), MAC_80211_FRAME_LEN);

    /* 组帧 (Re)Assoc_req_Frame */

    puc_curr_bssid = pst_hmac_vap_sta->st_vap_base_info.auc_bssid;

    ul_asoc_frame_len = hmac_mgmt_encap_asoc_req_sta(pst_hmac_vap_sta,(oal_uint8 *)(OAL_NETBUF_HEADER(pst_asoc_req_frame)), puc_curr_bssid);
    oal_netbuf_put(pst_asoc_req_frame, ul_asoc_frame_len);

    if (0 == ul_asoc_frame_len)
    {
        OAM_WARNING_LOG0(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_tx_mgmt_reassoc_req::hmac_mgmt_encap_asoc_req_sta null.}");
        oal_netbuf_free(pst_asoc_req_frame);

        return OAL_FAIL;
    }
    pst_hmac_user_ap = (hmac_user_stru *)mac_res_get_hmac_user((oal_uint16)pst_hmac_vap_sta->st_vap_base_info.uc_assoc_vap_id);
    if (OAL_PTR_NULL == pst_hmac_user_ap)
    {
        OAM_ERROR_LOG0(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{hmac_tx_mgmt_reassoc_req::pst_hmac_user_ap null.}");
        oal_netbuf_free(pst_asoc_req_frame);

        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_tx_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_asoc_req_frame);

    pst_tx_ctl->us_mpdu_len       = (oal_uint16)ul_asoc_frame_len;
    pst_tx_ctl->us_tx_user_idx    = pst_hmac_user_ap->st_user_base_info.us_assoc_id;

    /* 抛事件让DMAC将该帧发送 */
    ul_ret = hmac_tx_mgmt_send_event(&(pst_hmac_vap_sta->st_vap_base_info), pst_asoc_req_frame, (oal_uint16)ul_asoc_frame_len);
    if (OAL_SUCC != ul_ret)
    {
        oal_netbuf_free(pst_asoc_req_frame);

        OAM_WARNING_LOG1(pst_hmac_vap_sta->st_vap_base_info.uc_vap_id, OAM_SF_ASSOC, "{mac_tx_mgmt_reassoc_req::hmac_tx_mgmt_send_event failed[%d].}", ul_ret);
        return ul_ret;
    }
    return OAL_SUCC;

}


oal_uint32  hmac_config_reassoc_req(mac_vap_stru *pst_mac_vap)
{
    hmac_vap_stru                       *pst_hmac_vap;

    if (OAL_FALSE == g_en_wmmac_switch)
    {
        OAM_WARNING_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_reassoc_req::wmmac switch is false[%d].}", g_en_wmmac_switch);
        return OAL_SUCC;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_mac_vap->uc_vap_id);

    if (OAL_PTR_NULL == pst_hmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_WMMAC, "{hmac_config_reassoc_req::pst_hmac_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    hmac_tx_mgmt_reassoc_req(pst_hmac_vap);
    return OAL_SUCC;
}

#endif //_PRE_WLAN_FEATURE_WMMAC

#ifdef __cplusplus
#if __cplusplus
    }
#endif
#endif

