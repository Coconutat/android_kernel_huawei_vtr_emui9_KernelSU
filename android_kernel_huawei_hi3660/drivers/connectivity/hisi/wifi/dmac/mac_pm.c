


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_WLAN_FEATURE_PM) || defined(_PRE_WLAN_FEATURE_STA_PM)
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_spec.h"
#include "hal_ext_if.h"
#include "mac_resource.h"
#include "mac_device.h"
#include "mac_pm.h"
#ifdef _PRE_WLAN_FEATURE_SMP_SUPPORT
#include "dmac_vap.h"
#endif
#include "dmac_vap.h"
#include "dmac_mgmt_ap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_PM_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


mac_fsm_stru*  mac_fsm_create(oal_void*                 p_oshandle,         /*状态机owner的指针，对低功耗状态机，指向VAP结构*/
                                const oal_uint8          *p_name,             /*状态机的名字*/
                                oal_void                 *p_ctx,              /*状态机context*/
                                oal_uint8                 uc_init_state,      /*初始状态*/
                                const mac_fsm_state_info *p_state_info,       /*状态机实例指针*/
                                oal_uint8                 uc_num_states     /*本状态机的状态个数*/

)
{
    mac_fsm_stru   *pst_fsm = OAL_PTR_NULL;
    oal_uint32      ul_loop;


    if(MAC_FSM_MAX_STATES < uc_num_states)
    {
       OAM_ERROR_LOG1(0, OAM_SF_PWR, "{mac_fsm_create:state number [%d] too big. }",uc_num_states);
       return  OAL_PTR_NULL;
    }

    /*检查状态信息顺序是否和状态定义匹配*/
    for(ul_loop = 0;ul_loop < uc_num_states;ul_loop++)
    {
        if(p_state_info[ul_loop].state>=MAC_FSM_MAX_STATES||p_state_info[ul_loop].state!=ul_loop)
        {
            /* OAM日志中不能使用%s*/
            OAM_ERROR_LOG2(0, OAM_SF_PWR, "{FSM : entry %d has invalid state %d }",ul_loop,p_state_info[ul_loop].state);
            return OAL_PTR_NULL;
        }
    }

    pst_fsm =(mac_fsm_stru*)OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,OAL_SIZEOF(mac_fsm_stru),OAL_TRUE);
    if(OAL_PTR_NULL == pst_fsm)
    {
        OAM_ERROR_LOG1(0, OAM_SF_PWR, "{mac_fsm_create:malloc memory for fsm fail, size[%d]!", OAL_SIZEOF(mac_fsm_stru));
        return OAL_PTR_NULL;
    }

    OAL_MEMZERO(pst_fsm, OAL_SIZEOF(mac_fsm_stru));
    pst_fsm->uc_cur_state  = uc_init_state;
    pst_fsm->uc_prev_state = uc_init_state;
    pst_fsm->p_state_info  = p_state_info;
    pst_fsm->uc_num_states = uc_num_states;
    pst_fsm->p_oshandler   = p_oshandle;
    pst_fsm->p_ctx         = p_ctx;
    pst_fsm->us_last_event = MAC_FSM_EVENT_NONE;

    /* strncpy fsm name */
    ul_loop = 0;
    while((p_name[ul_loop] != '\0') && (ul_loop < MAC_FSM_MAX_NAME -1)) {
        pst_fsm->uc_name[ul_loop] = p_name[ul_loop];
        ul_loop++;
    }
    if (ul_loop < MAC_FSM_MAX_NAME) {
        pst_fsm->uc_name[ul_loop] = '\0';
    }

    /*启动状态机*/
   if(pst_fsm->p_state_info[pst_fsm->uc_cur_state].mac_fsm_entry)
   {
       pst_fsm->p_state_info[pst_fsm->uc_cur_state].mac_fsm_entry(pst_fsm->p_ctx);
   }

   return pst_fsm;

}


oal_void mac_fsm_destroy(mac_fsm_stru* p_fsm)
{
    OAL_MEM_FREE(p_fsm, OAL_TRUE);
    p_fsm = OAL_PTR_NULL;
    return;
}


oal_uint32 mac_fsm_trans_to_state(mac_fsm_stru* p_fsm,oal_uint8 uc_state)
{
    oal_uint8                    uc_cur_state = p_fsm->uc_cur_state;

    if ((uc_state == MAC_FSM_STATE_NONE) || (uc_state >= MAC_FSM_MAX_STATES)||(uc_state>=p_fsm->uc_num_states))
    {
        /* OAM日志中不能使用%s*/
         OAM_ERROR_LOG2(0, OAM_SF_PWR, "FSM:trans to state %d needs to be a valid state cur_state=%d",uc_state,uc_cur_state);
         return OAL_FAIL;
    }

    if(uc_state == uc_cur_state)
    {
        /* OAM日志中不能使用%s*/
       OAM_WARNING_LOG2(0, OAM_SF_PWR, "FSM :trans to state %d EQUAL to current state %d,nothing to do",uc_state,uc_cur_state);
       return OAL_SUCC;
    }

    /* OAM日志中不能使用%s*/
    OAM_INFO_LOG2(0, OAM_SF_PWR, "FSM: transition from %d => %d ",p_fsm->p_state_info[uc_cur_state].state,p_fsm->p_state_info[uc_state].state);

    /*调用前一状态的退出函数*/
    if(p_fsm->p_state_info[p_fsm->uc_cur_state].mac_fsm_exit)
    {
        p_fsm->p_state_info[p_fsm->uc_cur_state].mac_fsm_exit(p_fsm->p_ctx);
    }

    /*调用本状态的进入函数*/
    if(p_fsm->p_state_info[uc_state].mac_fsm_entry)
    {
       p_fsm->p_state_info[uc_state].mac_fsm_entry(p_fsm->p_ctx);
    }

    p_fsm->uc_prev_state = uc_cur_state;
    p_fsm->uc_cur_state  = uc_state;

    return OAL_SUCC;
}


oal_uint32 mac_fsm_event_dispatch(mac_fsm_stru* p_fsm ,oal_uint16 us_event,
                           oal_uint16 us_event_data_len, oal_void *p_event_data)
{
    oal_uint32 ul_event_handled = OAL_FALSE;

    if(p_fsm == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(0, OAM_SF_PWR, "mac_fsm_event_dispatch:p_fsm = OAL_PTR_NULL");
        return OAL_FAIL;
    }

    if((p_fsm->uc_cur_state!=MAC_FSM_STATE_NONE)&&(p_fsm->uc_cur_state<p_fsm->uc_num_states))
    {
        p_fsm->us_last_event = us_event;
        ul_event_handled = (*p_fsm->p_state_info[p_fsm->uc_cur_state].mac_fsm_event)(p_fsm->p_ctx, us_event, us_event_data_len, p_event_data);
    }
    if(OAL_FAIL == ul_event_handled)
    {
        /* OAM日志中不能使用%s*/
         OAM_ERROR_LOG2(0, OAM_SF_PWR, "FSM :mac_fsm_event_dispatch:event[%d] did not handled in state %d",
                        us_event,p_fsm->p_state_info[p_fsm->uc_cur_state].state);
         return OAL_FAIL;
    }

    return OAL_SUCC;
}

#ifdef _PRE_WLAN_FEATURE_PM

hal_lpm_state_enum_uint8  g_pm_hal_state_map[DEV_PWR_STATE_BUTT] =
{
    HAL_LPM_STATE_NORMAL_WORK,      /*DEV_PWR_STATE_WORK*/
    HAL_LPM_STATE_DEEP_SLEEP,       /*DEV_PWR_STATE_DEEP_SLEEP*/
    HAL_LPM_STATE_WOW,              /*DEV_PWR_STATE_WOW*/
    HAL_LPM_STATE_IDLE,             /*DEV_PWR_STATE_IDLE*/
    HAL_LPM_STATE_POWER_DOWN        /*DEV_PWR_STATE_OFF*/
};


mac_pm_arbiter_state_info g_pm_arbiter_state_info[DEV_PWR_STATE_BUTT] =
{
    {DEV_PWR_STATE_WORK,        "Work"},
    {DEV_PWR_STATE_DEEP_SLEEP,  "Deep sleep"},
    {DEV_PWR_STATE_WOW,         "Wow"},
    {DEV_PWR_STATE_IDLE,        "Idle"},
    {DEV_PWR_STATE_OFF,         "Poweroff"}
};


oal_uint32 mac_pm_arbiter_init(mac_device_stru* pst_device)
{
    mac_pm_arbiter_stru *pst_arbiter;

    pst_arbiter= OAL_MEM_ALLOC(OAL_MEM_POOL_ID_LOCAL,OAL_SIZEOF(mac_pm_arbiter_stru),OAL_TRUE);
    if(OAL_PTR_NULL == pst_arbiter)
    {
        OAM_ERROR_LOG0(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "hmac_pm_arbiter_init FAIL:out of memory");
        return OAL_FAIL;
    }
    OAL_MEMZERO(pst_arbiter, OAL_SIZEOF(mac_pm_arbiter_stru));
    pst_arbiter->pst_state_info = &g_pm_arbiter_state_info[0];

    pst_device->pst_pm_arbiter = (oal_void*)pst_arbiter;

    return OAL_SUCC;

}

oal_uint32 mac_pm_arbiter_destroy(mac_device_stru* pst_device)
{
    if(pst_device->pst_pm_arbiter)
    {
        OAL_MEM_FREE(pst_device->pst_pm_arbiter, OAL_TRUE);
        pst_device->pst_pm_arbiter = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_uint32 mac_pm_arbiter_alloc_id(mac_device_stru* pst_device, oal_uint8* pst_name,mac_pm_arbiter_type_enum en_arbiter_type)
{
    mac_pm_arbiter_stru *pst_pm_arbiter = (mac_pm_arbiter_stru*)(pst_device->pst_pm_arbiter);
    oal_uint32 i;

    if (pst_pm_arbiter == OAL_PTR_NULL)
    {
        OAM_ERROR_LOG0(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "hmac_pm_arbiter_alloc_id FAIL:mac device have no arbiter struct");
        return MAC_PWR_ARBITER_ID_INVALID;
    }

    if (en_arbiter_type <= MAC_PWR_ARBITER_TYPE_INVALID ||
            en_arbiter_type >= MAC_PWR_ARBITER_TYPE_BUTT)
    {
        return MAC_PWR_ARBITER_ID_INVALID;
    }

    /*从ul_id_bitmap中从低位开始遍历，找到一个为0的位即为未分配的ID*/
    for (i=0;i<MAC_PM_ARBITER_MAX_REQUESTORS;i++)
    {
        if (((1<<i) & pst_pm_arbiter->ul_id_bitmap) == 0)
        {
            pst_pm_arbiter->ul_id_bitmap |=  (1<<i);
            OAL_SPRINTF((char *)&pst_pm_arbiter->requestor[i].auc_id_name[0],
                    MAC_PM_ARBITER_MAX_REQ_NAME, "%s", pst_name);
            pst_pm_arbiter->requestor[i].en_arbiter_type = en_arbiter_type;
            pst_pm_arbiter->uc_requestor_num++;
            return i;
        }
    }

    return MAC_PWR_ARBITER_ID_INVALID;

}


oal_uint32 mac_pm_arbiter_free_id(mac_device_stru* pst_device, oal_uint32 ul_arbiter_id)
{
    mac_pm_arbiter_stru *pst_pm_arbiter = (mac_pm_arbiter_stru*)(pst_device->pst_pm_arbiter);
    oal_uint32            ul_loop;

    if(ul_arbiter_id >= MAC_PM_ARBITER_MAX_REQUESTORS)
    {
        OAM_ERROR_LOG2(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "hmac_pm_arbiter_free_id FAIL:invalid id %d,total %d",ul_arbiter_id,pst_pm_arbiter->uc_requestor_num);
        return OAL_FAIL;
    }
    pst_pm_arbiter->ul_id_bitmap &= ~(oal_uint32)(1<<ul_arbiter_id);
    for(ul_loop = 0;ul_loop<DEV_PWR_STATE_BUTT;ul_loop++)
    {
        pst_pm_arbiter->ul_state_bitmap[ul_loop]&= ~(oal_uint32)(1<<ul_arbiter_id);
    }
    pst_pm_arbiter->requestor[ul_arbiter_id].auc_id_name[0] = 0;
    pst_pm_arbiter->requestor[ul_arbiter_id].en_arbiter_type = MAC_PWR_ARBITER_TYPE_INVALID;
    pst_pm_arbiter->uc_requestor_num--;

    return OAL_SUCC;
}



oal_void mac_pm_arbiter_to_state(mac_device_stru *pst_device, mac_vap_stru *pst_mac_vap, oal_uint32 ul_arbiter_id,
                                            oal_uint8  uc_state_from, oal_uint8  uc_state_to)
{
    mac_pm_arbiter_stru         *pst_pm_arbiter =(mac_pm_arbiter_stru*)(pst_device->pst_pm_arbiter);
    oal_bool_enum_uint8          en_can_trans = OAL_TRUE;
    oal_uint32                   i;

    if(ul_arbiter_id >= MAC_PM_ARBITER_MAX_REQUESTORS)
    {
        OAM_ERROR_LOG2(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "mac_pm_arbiter_to_state FAIL:invalid id %d,total %d",ul_arbiter_id,pst_pm_arbiter->uc_requestor_num);
        return;
    }

    if((uc_state_from>=DEV_PWR_STATE_BUTT)||(uc_state_to>=DEV_PWR_STATE_BUTT)||(uc_state_from == uc_state_to))
    {
        OAM_ERROR_LOG2(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "mac_pm_arbiter_to_state FAIL:invalid state from %d to %d",uc_state_from,uc_state_to);
        return;
    }

    /*投票者本身必然发生了状态切换，清理原状态的bitmap，设置切换状态的bitmap*/
    pst_pm_arbiter->ul_state_bitmap[uc_state_from] &= ~(oal_uint32)(1<<ul_arbiter_id);

    pst_pm_arbiter->ul_state_bitmap[uc_state_to] |= (1<<ul_arbiter_id);

    /* OAM日志中不能使用%s*/
    OAM_INFO_LOG3(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "PM arbiter:%d vote to transmit from state %d to state %d",
                    ul_arbiter_id,uc_state_from,uc_state_to);

    /*工作状态，只要1票,device就必须要切换*/
    if(DEV_PWR_STATE_WORK == uc_state_to)
    {
        /* TBD:切回work时，防止多次设置硬件寄存器，需要在此过滤一下 */
        en_can_trans = OAL_TRUE;
    }
    else
    {
        /*如果所有成员都投票了，device进行状态切换*/
        for(i=0;i<pst_pm_arbiter->uc_requestor_num;i++)
        {
            if (pst_pm_arbiter->requestor[i].en_arbiter_type != MAC_PWR_ARBITER_TYPE_INVALID)
            {
                if (!((1<<i) & pst_pm_arbiter->ul_state_bitmap[uc_state_to]))
                {
                    en_can_trans = OAL_FALSE;
                    break;
                }
            }
        }
    }

    if(en_can_trans == OAL_TRUE)
    {
       pst_pm_arbiter->uc_prev_state = pst_pm_arbiter->uc_cur_state;

       /*操作hal层接口*/
       if(OAL_SUCC == mac_pm_set_hal_state(pst_device, pst_mac_vap, uc_state_to))
       {
           //OAM_INFO_LOG1(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "PM arbiter:set device to state %d",uc_state_to);

           pst_pm_arbiter->uc_cur_state  = uc_state_to;
       }

    }

    return;
}


oal_uint32  mac_pm_wow_prepare_probe_resp(dmac_vap_stru *pst_dmac_vap)
{
    oal_uint8       ast_dest_addr[WLAN_MAC_ADDR_LEN] = {0,0,0,0,0,0};


    if (OAL_PTR_NULL == pst_dmac_vap->pst_wow_probe_resp)
    {
        pst_dmac_vap->pst_wow_probe_resp = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
        if (OAL_PTR_NULL ==  pst_dmac_vap->pst_wow_probe_resp)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_test_lpm_wow_prepare_probe_resp::alloc mgmt buffer failed!}\r\n");
            return OAL_ERR_CODE_PTR_NULL;
        }

        OAL_NETBUF_PREV(pst_dmac_vap->pst_wow_probe_resp) = OAL_PTR_NULL;
        OAL_NETBUF_NEXT(pst_dmac_vap->pst_wow_probe_resp) = OAL_PTR_NULL;
    }

    /* 封装probe response帧 */
    pst_dmac_vap->us_wow_probe_resp_len = dmac_mgmt_encap_probe_response(pst_dmac_vap, pst_dmac_vap->pst_wow_probe_resp, &ast_dest_addr[0], OAL_FALSE);

    return OAL_SUCC;
}


oal_void  mac_pm_wow_release_probe_resp(dmac_vap_stru *pst_dmac_vap)
{

    if(OAL_PTR_NULL != pst_dmac_vap->pst_wow_probe_resp)
    {
        oal_netbuf_free(pst_dmac_vap->pst_wow_probe_resp);
        pst_dmac_vap->pst_wow_probe_resp    = OAL_PTR_NULL;
        pst_dmac_vap->us_wow_probe_resp_len = 0;
    }

    return ;
}


oal_uint32  mac_pm_wow_prepare_null_data(dmac_vap_stru *pst_dmac_vap)
{
    dmac_user_stru* pst_dmac_user;


    /*仅需要为STA模式的VAP准备null data帧，做keep alive*/
    if (WLAN_VAP_MODE_BSS_STA == pst_dmac_vap->st_vap_base_info.en_vap_mode)
    {
        pst_dmac_user = mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id);
        if (OAL_PTR_NULL == pst_dmac_user)
        {
            return OAL_FAIL;
        }

        if (OAL_PTR_NULL == pst_dmac_vap->pst_wow_null_data)
        {
            /* 申请net_buff */
            pst_dmac_vap->pst_wow_null_data = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_SHORT_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
            if (OAL_PTR_NULL == pst_dmac_vap->pst_wow_null_data)
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "{dmac_test_lpm_wow_prepare_null_data::alloc pst_net_buf fail!}\r\n");
                return OAL_ERR_CODE_ALLOC_MEM_FAIL;
            }

            OAL_NETBUF_PREV(pst_dmac_vap->pst_wow_null_data) = OAL_PTR_NULL;
            OAL_NETBUF_NEXT(pst_dmac_vap->pst_wow_null_data) = OAL_PTR_NULL;
        }

        /* 填写帧头,其中from ds为1，to ds为0，因此frame control的第二个字节为02 */
        mac_hdr_set_frame_control(oal_netbuf_header(pst_dmac_vap->pst_wow_null_data), (oal_uint16)(WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_NODATA) | 0x0200);

        /* 设置ADDR1为目的地址 */
        oal_set_mac_addr((oal_netbuf_header(pst_dmac_vap->pst_wow_null_data) + 4), pst_dmac_user->st_user_base_info.auc_user_mac_addr);

        /* 设置ADDR2为SA */
        oal_set_mac_addr((oal_netbuf_header(pst_dmac_vap->pst_wow_null_data) + 10),pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID);

        /* 设置ADDR3为BSSID */
        oal_set_mac_addr((oal_netbuf_header(pst_dmac_vap->pst_wow_null_data) + 16), pst_dmac_vap->st_vap_base_info.auc_bssid);
    }

    return OAL_SUCC;
}


oal_void  mac_pm_wow_release_null_data(dmac_vap_stru *pst_dmac_vap)
{
    if (OAL_PTR_NULL != pst_dmac_vap->pst_wow_null_data)
    {
        oal_netbuf_free(pst_dmac_vap->pst_wow_null_data);
        pst_dmac_vap->pst_wow_null_data = OAL_PTR_NULL;
    }
}


oal_uint32  mac_pm_set_wow_para(dmac_vap_stru *pst_dmac_vap, hal_wow_param_stru *pst_wow_para, mac_cfg_lpm_wow_en_stru st_wow_en)
{
    mac_device_stru                 *pst_mac_device;
    oal_uint8                       uc_vap_idx;
    oal_uint32                      ul_wow_set_bitmap = 0;

    pst_mac_device = mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        return OAL_FAIL;
    }

    OAL_MEMZERO(pst_wow_para, OAL_SIZEOF(hal_wow_param_stru));

    if (st_wow_en.uc_en)
    {
        ul_wow_set_bitmap|= HAL_WOW_PARA_EN;
        for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
        {

            pst_dmac_vap =  mac_res_get_dmac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
            if (OAL_PTR_NULL == pst_dmac_vap)
            {
                OAM_WARNING_LOG0(pst_mac_device->auc_vap_id[uc_vap_idx], OAM_SF_CFG, "{mac_pm_set_wow_para::pst_dmac_vap null.");
                return OAL_ERR_CODE_PTR_NULL;
            }
            mac_pm_wow_prepare_probe_resp(pst_dmac_vap);
            mac_pm_wow_prepare_null_data(pst_dmac_vap);
            if(0 == pst_dmac_vap->pst_hal_vap->uc_vap_id)
            {
              if(!pst_dmac_vap->pst_wow_probe_resp)
               {
                    continue;
               }
              pst_wow_para->ul_ap0_probe_resp_address = OAL_VIRT_TO_PHY_ADDR((oal_uint32 *)oal_netbuf_data(pst_dmac_vap->pst_wow_probe_resp));
              pst_wow_para->ul_ap0_probe_resp_len = pst_dmac_vap->us_wow_probe_resp_len;
              pst_wow_para->uc_ap0_probe_resp_phy = 0;    /* TBD 11b*/
              pst_wow_para->uc_ap0_probe_resp_rate = 0;   /* TBD 1M*/
              ul_wow_set_bitmap |= HAL_WOW_PARA_AP0_PROBE_RESP;
            }
            else if(1 == pst_dmac_vap->pst_hal_vap->uc_vap_id)
            {
              if(!pst_dmac_vap->pst_wow_probe_resp)
               {
                    continue;
               }
              pst_wow_para->ul_ap1_probe_resp_address = OAL_VIRT_TO_PHY_ADDR((oal_uint32 *)oal_netbuf_data(pst_dmac_vap->pst_wow_probe_resp));
              pst_wow_para->ul_ap1_probe_resp_len = pst_dmac_vap->us_wow_probe_resp_len;
              pst_wow_para->uc_ap1_probe_resp_phy = 0;    /* TBD 11b*/
              pst_wow_para->uc_ap1_probe_resp_rate = 0;   /* TBD 1M*/
              ul_wow_set_bitmap |= HAL_WOW_PARA_AP1_PROBE_RESP;
            }
            else if(4 == pst_dmac_vap->pst_hal_vap->uc_vap_id)
            {
               if(!pst_dmac_vap->pst_wow_null_data)
               {
                    continue;
               }
               pst_wow_para->ul_nulldata_address = OAL_VIRT_TO_PHY_ADDR((oal_uint32 *)oal_netbuf_data(pst_dmac_vap->pst_wow_null_data));
               pst_wow_para->uc_nulldata_rate = 0;
               pst_wow_para->uc_nulldata_phy_mode = 0;
               ul_wow_set_bitmap |= HAL_WOW_PARA_NULLDATA;

               pst_wow_para->ul_nulldata_interval = 1000; /*1s*/
            }
        }
        pst_wow_para->uc_nulldata_awake = st_wow_en.uc_null_wake;
        ul_wow_set_bitmap |= HAL_WOW_PARA_NULLDATA_AWAKE;

        pst_wow_para->ul_set_bitmap = ul_wow_set_bitmap;
    }
    else
    {
        ul_wow_set_bitmap = 0;
        for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
        {

            pst_dmac_vap =  mac_res_get_dmac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
            if (OAL_PTR_NULL == pst_dmac_vap)
            {
                OAM_WARNING_LOG0(pst_mac_device->auc_vap_id[uc_vap_idx], OAM_SF_CFG, "{mac_pm_set_wow_para::pst_dmac_vap null.}");
                return OAL_ERR_CODE_PTR_NULL;
            }
            mac_pm_wow_release_probe_resp(pst_dmac_vap);
            mac_pm_wow_release_null_data(pst_dmac_vap);
        }

        pst_wow_para->ul_set_bitmap = ul_wow_set_bitmap;
    }

    return OAL_SUCC;
}


oal_uint32 mac_pm_set_hal_state(mac_device_stru *pst_device, mac_vap_stru *pst_mac_vap, oal_uint8 uc_state_to)
{
    hal_lpm_state_param_stru     st_para;
    hal_wow_param_stru           st_wow_para;
    mac_cfg_lpm_wow_en_stru      st_wow_en;
    mac_pm_arbiter_stru         *pst_pm_arbiter =(mac_pm_arbiter_stru *)(pst_device->pst_pm_arbiter);
    mac_pm_handler_stru         *pst_ap_pm_handler = OAL_PTR_NULL;
    dmac_vap_stru               *pst_dmac_vap      = OAL_PTR_NULL;

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "{mac_pm_set_hal_state: pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ap_pm_handler = (mac_pm_handler_stru *)pst_dmac_vap->pst_pm_handler;
    OAL_MEMZERO(&st_para, OAL_SIZEOF(hal_lpm_state_param_stru));

    switch(uc_state_to)
    {
        case DEV_PWR_STATE_WORK:
            /*恢复接收通道和beacon interval配置*/
            st_para.bit_set_bcn_interval = OAL_TRUE;
        #ifdef _PRE_WLAN_FEATURE_DBAC
            if (mac_is_dbac_enabled(pst_device))
            {
                st_para.bit_set_bcn_interval = OAL_FALSE;
            }
        #endif
            st_para.ul_idle_bcn_interval = pst_device->ul_beacon_interval;
            /* 此时ap上层的状态已经转化过来，所以需要prev state判断 */
            if (DEV_PWR_STATE_WOW == pst_ap_pm_handler->p_mac_fsm->uc_prev_state)
            {
                st_wow_en.uc_en = 0;
                st_wow_en.uc_null_wake = 0;
                mac_pm_set_wow_para(pst_dmac_vap, &st_wow_para, st_wow_en);
            }
            break;
        case DEV_PWR_STATE_DEEP_SLEEP:
            st_para.bit_gpio_sleep_en = 0;
            st_para.bit_soft_sleep_en = 1;
            st_para.ul_sleep_time     = 100; /*100ms*/
            break;
        case DEV_PWR_STATE_WOW:
            st_wow_en.uc_en = 1;
            st_wow_en.uc_null_wake = 0;
            mac_pm_set_wow_para(pst_dmac_vap, &st_wow_para, st_wow_en);
            break;
        case DEV_PWR_STATE_IDLE:
            /*单通道接收,beacon调成AP_IDLE_BCN_INTERVAL*/
            st_para.bit_set_bcn_interval = OAL_TRUE;
        #ifdef _PRE_WLAN_FEATURE_DBAC
            if (mac_is_dbac_enabled(pst_device))
            {
                st_para.bit_set_bcn_interval = OAL_FALSE;
            }
        #endif
            st_para.ul_idle_bcn_interval = WLAN_BEACON_INTVAL_IDLE;
            break;
        case DEV_PWR_STATE_OFF:

            break;
        default:
        {
            return OAL_FAIL;
        }
    }

    pst_pm_arbiter->uc_prev_state = pst_pm_arbiter->uc_cur_state;
    pst_pm_arbiter->uc_cur_state  = uc_state_to;

    hal_set_lpm_state(pst_device->pst_device_stru,
                      g_pm_hal_state_map[pst_pm_arbiter->uc_prev_state],
                      g_pm_hal_state_map[uc_state_to],
                      &st_para,
                      &st_wow_para);

    return OAL_SUCC;
}
#endif
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

