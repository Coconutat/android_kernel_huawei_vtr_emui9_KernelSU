


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_WLAN_FEATURE_AP_PM)
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "wlan_spec.h"
#include "hal_ext_if.h"
#include "mac_resource.h"
#include "dmac_resource.h"

#include "mac_pm.h"
#include "dmac_vap.h"
#include "dmac_mgmt_ap.h"
#include "frw_timer.h"
#include "dmac_beacon.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_PM_C

extern oal_uint32  dmac_protection_update_mib_ap(mac_vap_stru *pst_mac_vap);

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

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
        OAM_WARNING_LOG2(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "mac_pm_arbiter_to_state FAIL:invalid state from %d to %d",uc_state_from,uc_state_to);
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
#ifdef _PRE_WLAN_FEATURE_1024QAM
    oal_uint8       uc_ie_len;
#endif

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

    /* 增加1024 QAM IE */
#ifdef _PRE_WLAN_FEATURE_1024QAM
    mac_set_1024qam_vendor_ie(pst_dmac_vap, mac_netbuf_get_payload(pst_dmac_vap->pst_wow_probe_resp) + pst_dmac_vap->us_wow_probe_resp_len - MAC_80211_FRAME_LEN, &uc_ie_len);
    pst_dmac_vap->us_wow_probe_resp_len += uc_ie_len;
#endif

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
        pst_dmac_user = mac_res_get_dmac_user(pst_dmac_vap->st_vap_base_info.us_assoc_vap_id);
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
        oal_set_mac_addr((oal_netbuf_header(pst_dmac_vap->pst_wow_null_data) + 10),mac_mib_get_StationID(&pst_dmac_vap->st_vap_base_info));

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
#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
    dmac_device_stru                *pst_dmac_device;
    hal_to_dmac_device_stru         *pst_hal_device;
#endif

    pst_mac_device = mac_res_get_dev_etc(pst_dmac_vap->st_vap_base_info.uc_device_id);
    if(OAL_PTR_NULL == pst_mac_device)
    {
        return OAL_FAIL;
    }

    OAL_MEMZERO(pst_wow_para, OAL_SIZEOF(hal_wow_param_stru));

#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
    pst_dmac_device = (dmac_device_stru *)dmac_res_get_mac_dev(pst_mac_device->uc_device_id);
    if(OAL_PTR_NULL == pst_dmac_device)
    {
        return OAL_FAIL;
    }
    pst_hal_device = pst_dmac_vap->pst_hal_device;
    if(OAL_PTR_NULL == pst_hal_device)
    {
        return OAL_FAIL;
    }
#endif

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
                pst_wow_para->ul_ap0_probe_resp_phy = 0;
                pst_wow_para->ul_ap0_probe_resp_rate = mac_fcs_get_prot_datarate(&pst_dmac_vap->st_vap_base_info);
                ul_wow_set_bitmap |= HAL_WOW_PARA_AP0_PROBE_RESP;
                /* 硬件没对vap模式判断,ap下进入wow后会1s发送mac地址全零的"null data" */
                pst_wow_para->ul_nulldata_interval = 0;
                ul_wow_set_bitmap |= HAL_WOW_PARA_NULLDATA_INTERVAL;
            }
            else if(1 == pst_dmac_vap->pst_hal_vap->uc_vap_id)
            {
                if(!pst_dmac_vap->pst_wow_probe_resp)
                {
                    continue;
                }
                pst_wow_para->ul_ap1_probe_resp_address = OAL_VIRT_TO_PHY_ADDR((oal_uint32 *)oal_netbuf_data(pst_dmac_vap->pst_wow_probe_resp));
                pst_wow_para->ul_ap1_probe_resp_len = pst_dmac_vap->us_wow_probe_resp_len;
                pst_wow_para->ul_ap1_probe_resp_phy = 0;
                pst_wow_para->ul_ap1_probe_resp_rate = mac_fcs_get_prot_datarate(&pst_dmac_vap->st_vap_base_info);
                ul_wow_set_bitmap |= HAL_WOW_PARA_AP1_PROBE_RESP;
                pst_wow_para->ul_nulldata_interval = 0;
                ul_wow_set_bitmap |= HAL_WOW_PARA_NULLDATA_INTERVAL;
            }
            else if(4 == pst_dmac_vap->pst_hal_vap->uc_vap_id)
            {
                pst_wow_para->ul_nulldata_rate = mac_fcs_get_prot_datarate(&pst_dmac_vap->st_vap_base_info);
                pst_wow_para->ul_nulldata_phy_mode = mac_fcs_get_prot_mode(&pst_dmac_vap->st_vap_base_info);
                ul_wow_set_bitmap |= HAL_WOW_PARA_NULLDATA;
                if (!pst_dmac_vap->pst_wow_null_data)
                {
                    pst_wow_para->ul_nulldata_address = 0;
                    pst_wow_para->ul_nulldata_interval = 0; /*1s*/
                    ul_wow_set_bitmap |= HAL_WOW_PARA_NULLDATA_INTERVAL;
                    continue;
                }
                pst_wow_para->ul_nulldata_address = OAL_VIRT_TO_PHY_ADDR((oal_uint32 *)oal_netbuf_data(pst_dmac_vap->pst_wow_null_data));
                pst_wow_para->ul_nulldata_interval = 1000; /*1s*/
                ul_wow_set_bitmap |= HAL_WOW_PARA_NULLDATA_INTERVAL;
            }
        }

#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
        hal_clr_fake_vap(pst_hal_device, pst_dmac_device->uc_fake_vap_id);
#endif
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
#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
    dmac_device_stru            *pst_dmac_device;
#endif
#ifdef _PRE_WLAN_FEATURE_DFR
    hal_to_dmac_device_stru     *pst_hal_device;
#endif

    pst_dmac_vap = mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "{mac_pm_set_hal_state: pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_ap_pm_handler = &pst_dmac_vap->st_pm_handler;
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
            if (DEV_PWR_STATE_WOW == pst_ap_pm_handler->st_oal_fsm.uc_prev_state)
            {
                st_wow_en.uc_en = 0;
                st_wow_en.uc_null_wake = 0;
                mac_pm_set_wow_para(pst_dmac_vap, &st_wow_para, st_wow_en);

                frw_timer_restart_etc();
#ifdef _PRE_WLAN_FEATURE_GREEN_AP
                dmac_green_ap_resume(pst_device);
#endif
            }
            /* 从深睡唤醒，重新启动定时器 */
            else if (DEV_PWR_STATE_DEEP_SLEEP== pst_ap_pm_handler->st_oal_fsm.uc_prev_state)
            {
                frw_timer_restart_etc();
            }
            break;
        case DEV_PWR_STATE_DEEP_SLEEP:
            st_para.bit_gpio_sleep_en = 1;
            st_para.bit_soft_sleep_en = 0;
            st_para.ul_sleep_time     = 100; /*100ms*/
#ifdef _PRE_WLAN_FEATURE_DFR
            pst_hal_device = pst_dmac_vap->pst_hal_device;
            FRW_TIMER_STOP_TIMER(&(pst_hal_device->st_dfr_tx_prot.st_tx_prot_timer));
#endif
            /* 停止所有定时器 */
            frw_timer_stop_etc();
            break;
        case DEV_PWR_STATE_WOW:
            st_wow_en.uc_en = 1;
            st_wow_en.uc_null_wake = 0;
            mac_pm_set_wow_para(pst_dmac_vap, &st_wow_para, st_wow_en);

#ifdef _PRE_WLAN_FEATURE_DFR
            pst_hal_device = pst_dmac_vap->pst_hal_device;
            FRW_TIMER_STOP_TIMER(&(pst_hal_device->st_dfr_tx_prot.st_tx_prot_timer));
#endif
            frw_timer_stop_etc();
#ifdef _PRE_WLAN_FEATURE_GREEN_AP
            dmac_green_ap_suspend(pst_device);
#endif
            /* 之前检测到有non ht的obss存在，在进WOW前清除掉，以免进WOW后还接力该设置 */
            if ((WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
                && (OAL_TRUE == pst_mac_vap->st_protection.bit_obss_non_ht_present))
            {
                pst_mac_vap->st_protection.bit_obss_non_ht_present = OAL_FALSE;
                pst_mac_vap->st_protection.uc_obss_non_ht_aging_cnt = 0;
                mac_mib_set_HtProtection(pst_mac_vap, WLAN_MIB_HT_NO_PROTECTION);

                dmac_encap_beacon(pst_dmac_vap, pst_dmac_vap->pauc_beacon_buffer[pst_dmac_vap->uc_beacon_idx], &(pst_dmac_vap->us_beacon_len));
                mac_pm_wow_prepare_probe_resp(pst_dmac_vap);

                OAM_WARNING_LOG0(pst_device->uc_cfg_vap_id, OAM_SF_PWR, "{mac_pm_set_hal_state: clear non ht obss setting}");
                dmac_protection_update_mib_ap(pst_mac_vap);
            }
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

    hal_set_lpm_state(pst_dmac_vap->pst_hal_device,
                  g_pm_hal_state_map[pst_pm_arbiter->uc_prev_state],
                  g_pm_hal_state_map[uc_state_to],
                  &st_para,
                  &st_wow_para);

#if (WLAN_MAX_NSS_NUM >= WLAN_DOUBLE_NSS)
    if (DEV_PWR_STATE_WORK == uc_state_to)
    {
        pst_dmac_device = (dmac_device_stru *)dmac_res_get_mac_dev(pst_device->uc_device_id);
        if(OAL_PTR_NULL == pst_dmac_device)
        {
            return OAL_FAIL;
        }
        hal_set_fake_vap(pst_dmac_vap->pst_hal_device, pst_dmac_device->uc_fake_vap_id);
    }
#endif

    return OAL_SUCC;
}
#endif
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

