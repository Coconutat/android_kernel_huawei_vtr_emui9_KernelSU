
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef    _PRE_WLAN_FEATURE_GREEN_AP

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include    "hal_witp_pa_reg_field.h"
#include    "hal_ext_if.h"
#include    "mac_device.h"
#include    "mac_vap.h"


#include    "dmac_tx_bss_comm.h"
#include    "dmac_vap.h"
#include    "dmac_device.h"
#include    "dmac_scan.h"
#include    "dmac_beacon.h"
#include    "dmac_mgmt_classifier.h"
#include    "dmac_config.h"
#include    "dmac_green_ap.h"
#include    "dmac_auto_adjust_freq.h"


#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/hrtimer.h>
#include <linux/time.h>           /* struct timespec    */
#endif


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_GREEN_AP_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)     /* 1102 */
OAL_STATIC oal_void dmac_green_ap_timer_isr(oal_void);
#else
OAL_STATIC enum hrtimer_restart dmac_green_ap_timer_isr(struct hrtimer *pst_hrtimer);
#endif

OAL_STATIC dmac_green_ap_mgr_stru g_ast_gap_mgr[MAC_RES_MAX_DEV_NUM];


oal_uint32  dmac_green_ap_setup_timer(dmac_green_ap_mgr_stru* pst_gap_mgr)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)     /* 1102 */
    Timer_DBAC_exit(GREEN_AP_STIMER_INDEX);
    Timer_DBAC_setup(GREEN_AP_STIMER_INDEX, dmac_green_ap_timer_isr);

    return OAL_SUCC;
#else
    hrtimer_init(&pst_gap_mgr->st_gap_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    pst_gap_mgr->st_gap_timer.function = dmac_green_ap_timer_isr;

    return OAL_SUCC;
#endif
}


oal_void dmac_green_ap_start_timer(dmac_green_ap_mgr_stru *pst_gap_mgr, oal_uint32 ul_duration)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)     /* 1102 */
    Timer_DBAC_start(GREEN_AP_STIMER_INDEX, ul_duration<<10);
#else
    hrtimer_start(&pst_gap_mgr->st_gap_timer,
                    ktime_set(ul_duration / 1000, (ul_duration % 1000) * 1000000),
                    HRTIMER_MODE_REL);
#endif
}


oal_void dmac_green_ap_stop_timer(dmac_green_ap_mgr_stru *pst_gap_mgr)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)     /* 1102 */
    Timer_DBAC_stop(GREEN_AP_STIMER_INDEX);
#else
    hrtimer_cancel(&pst_gap_mgr->st_gap_timer);
#endif
}


oal_void dmac_green_ap_release_timer(dmac_green_ap_mgr_stru *pst_gap_mgr)
{
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)     /* 1102 */
    Timer_DBAC_exit(GREEN_AP_STIMER_INDEX);
#endif
}


oal_uint8 dmac_green_ap_is_hw_queues_empty(mac_device_stru  *pst_device)
{
    hal_to_dmac_device_stru         *pst_hal_device;
    oal_uint8                        uc_queue_num;

    pst_hal_device = pst_device->pst_device_stru;

    /* 硬件发送队列 */
    for (uc_queue_num = 0; uc_queue_num < HAL_TX_QUEUE_BUTT; uc_queue_num++)
    {
        /*对应的硬件队列检查 */
        if (OAL_FALSE == (oal_dlist_is_empty(&(pst_hal_device->ast_tx_dscr_queue[uc_queue_num].st_header))))
        {
            return OAL_FALSE;
        }
    }

    return OAL_TRUE;
}


oal_uint8  dmac_green_ap_is_tid_queues_empty(dmac_vap_stru  *pst_dmac_vap)
{
    dmac_user_stru                   *pst_user;
    oal_uint8                        uc_tid_idx;

    /* TID队列 */
    pst_user = (dmac_user_stru *)mac_res_get_dmac_user((oal_uint16)(pst_dmac_vap->st_vap_base_info.uc_assoc_vap_id));

    if (OAL_PTR_NULL != pst_user)
    {
        if (OAL_FALSE == dmac_psm_is_tid_empty(pst_user))
        {
            return OAL_FALSE;
        }
        return OAL_TRUE;
    }
    else
    {
        return OAL_TRUE;
    }
}


oal_void  dmac_green_ap_pause_vap(mac_device_stru *pst_device,dmac_vap_stru *pst_dmac_vap,dmac_green_ap_mgr_stru *pst_gap_mgr)
{
    hal_one_packet_status_stru   st_status;
    mac_fcs_err_enum_uint8       en_fcs_req_ret;
    mac_fcs_mgr_stru            *pst_fcs_mgr;

    /*检查接收硬件发送队列和TID队列是否空*/
    if(OAL_FALSE == dmac_green_ap_is_hw_queues_empty(pst_device))
    {
        return;
    }

    if(OAL_FALSE == dmac_green_ap_is_tid_queues_empty(pst_dmac_vap))
    {
        return;
    }

    pst_fcs_mgr     = dmac_fcs_get_mgr_stru(pst_device);
    en_fcs_req_ret  = mac_fcs_request(pst_fcs_mgr, NULL, NULL);
    if (MAC_FCS_SUCCESS != en_fcs_req_ret)
    {
        OAM_WARNING_LOG1(0, OAM_SF_GREEN_AP, "{dmac_green_ap_pause_vap::mac_fcs_request failed, ret=%d}", (oal_uint32)en_fcs_req_ret);
        return;
    }

    dmac_vap_pause_tx(&pst_dmac_vap->st_vap_base_info);

    /* 发送one packet */
    mac_fcs_send_one_packet_start(pst_fcs_mgr, &pst_gap_mgr->st_one_packet_cfg, pst_device->pst_device_stru, &st_status, OAL_TRUE);
    hal_one_packet_stop(pst_device->pst_device_stru);
    mac_fcs_release(pst_fcs_mgr);

    hal_set_machw_tx_suspend(pst_device->pst_device_stru);
    hal_disable_machw_phy_and_pa(pst_device->pst_device_stru);

    hal_psm_rf_sleep(pst_device->pst_device_stru, OAL_TRUE);

    pst_gap_mgr->uc_state = DMAC_GREEN_AP_STATE_PAUSE;
    pst_gap_mgr->ul_pause_count++;
}


oal_void  dmac_green_ap_resume_vap(mac_device_stru *pst_device,dmac_vap_stru *pst_dmac_vap,dmac_green_ap_mgr_stru *pst_gap_mgr)
{
    if (DMAC_GREEN_AP_STATE_WORK == pst_gap_mgr->uc_state)
    {
        return;
    }

    hal_psm_rf_awake(pst_device->pst_device_stru, OAL_TRUE);

    hal_enable_machw_phy_and_pa(pst_device->pst_device_stru);
    hal_set_machw_tx_resume(pst_device->pst_device_stru);
    mac_vap_resume_tx(&(pst_dmac_vap->st_vap_base_info));
    hal_vap_beacon_resume(pst_dmac_vap->pst_hal_vap);

    pst_gap_mgr->uc_state = DMAC_GREEN_AP_STATE_WORK;
    pst_gap_mgr->ul_resume_count++;
}


oal_uint32  dmac_green_ap_timer_event_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru              *pst_event;
    dmac_green_ap_mgr_stru      *pst_gap_mgr;
    mac_device_stru             *pst_device;
    dmac_vap_stru               *pst_dmac_vap;

    pst_event   = (frw_event_stru *)pst_event_mem->puc_data;

    pst_device  = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if(OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_timer_event_handler:: pst_device NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_gap_mgr  = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;
    if (pst_gap_mgr->uc_state < DMAC_GREEN_AP_STATE_INITED)
    {
       return OAL_FAIL;
    }

    pst_dmac_vap = mac_res_get_dmac_vap(pst_gap_mgr->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_timer_event_handler:: pst_dmac_vap NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 奇数pause，偶数resume */
    if (pst_gap_mgr->uc_cur_slot & 0x01)
    {
         dmac_green_ap_pause_vap(pst_device, pst_dmac_vap, pst_gap_mgr);
    }
    else
    {
         dmac_green_ap_resume_vap(pst_device, pst_dmac_vap, pst_gap_mgr);
         dmac_tx_complete_schedule(pst_device->pst_device_stru, WLAN_WME_AC_BE);
    }

    pst_gap_mgr->ul_total_count++;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  dmac_green_ap_post_pause_event(dmac_green_ap_mgr_stru *pst_gap_mgr)
{
    frw_event_mem_stru      *pst_event_mem;
    frw_event_stru          *pst_event;

    pst_event_mem = FRW_EVENT_ALLOC(0);
    if (NULL == pst_event_mem)
    {
        OAM_WARNING_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_post_pause_event::alloc mem failed when post fcs event}");
        return  OAL_FAIL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       FRW_EVENT_TYPE_DMAC_MISC,
                       HAL_EVENT_DMAC_MISC_GREEN_AP,
                       OAL_SIZEOF(mac_fcs_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_0,
                       0,
                       0,
                       pst_gap_mgr->uc_vap_id); // no vap id


    if (OAL_SUCC != frw_event_dispatch_event(pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_post_pause_event::post fcs event error!}");
    }

    FRW_EVENT_FREE(pst_event_mem);

    return  OAL_SUCC;
}



#if defined(_PRE_PRODUCT_ID_HI110X_DEV)     /* 1102 */
OAL_STATIC oal_void dmac_green_ap_timer_isr(void)
#else
OAL_STATIC enum hrtimer_restart dmac_green_ap_timer_isr(struct hrtimer *pst_hrtimer)
#endif
{
    mac_device_stru         *pst_device;
    dmac_green_ap_mgr_stru  *pst_gap_mgr;
    oal_uint8                uc_time;
    oal_uint8                uc_offset;

#if defined(_PRE_PRODUCT_ID_HI110X_DEV)     /* 1102 */
    pst_device   = mac_res_get_dev(0);
    pst_gap_mgr  = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;
    dmac_green_ap_stop_timer(pst_gap_mgr);
#else
    pst_device = mac_res_get_dev((OAL_CONTAINER_OF(pst_hrtimer, dmac_green_ap_mgr_stru, st_gap_timer))->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_timer_isr:: pst_device NULL!}");
        return HRTIMER_NORESTART;
    }

    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;
#endif

    if (pst_gap_mgr->uc_state <= DMAC_GREEN_AP_STATE_INITED)
    {
        OAM_WARNING_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_timer_isr:: waiting for tbtt isr!}");
#if defined(_PRE_PRODUCT_ID_HI110X_DEV)
        return;
#else
        return HRTIMER_NORESTART;
#endif
    }

    pst_gap_mgr->uc_cur_slot++;

    dmac_green_ap_post_pause_event(pst_gap_mgr);

    /*最后一个时隙不启动timer，tbtt中断后启动*/
    if(pst_gap_mgr->uc_max_slot_cnt > pst_gap_mgr->uc_cur_slot)
    {
       uc_time   = (pst_gap_mgr->uc_cur_slot & 0x01) ? pst_gap_mgr->uc_pause_time : pst_gap_mgr->uc_work_time;
       uc_offset = (pst_gap_mgr->uc_cur_slot & 0x01) ? pst_gap_mgr->uc_resume_offset : 0;
       dmac_green_ap_start_timer(pst_gap_mgr, (uc_time - uc_offset));
    }
}


dmac_vap_stru*  dmac_green_ap_get_vap(mac_device_stru  *pst_device)
{
    dmac_vap_stru           *pst_dmac_vap = OAL_PTR_NULL;

    if (!pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_get_vap:: pst_device NULL!}");
        return OAL_PTR_NULL;
    }

    /* 只在单个aput下开启 */
    if(pst_device->uc_vap_num != 1)
    {
        OAM_ERROR_LOG1(0, OAM_SF_GREEN_AP, "{dmac_green_ap_get_vap::total vap[%d]}", pst_device->uc_vap_num);
        return OAL_PTR_NULL;
    }

    /*与DBAC功能互斥*/
    if(mac_is_dbac_running(pst_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_get_vap::mac_is_dbac_enabled}");
        return OAL_PTR_NULL;
    }

    pst_dmac_vap = mac_res_get_dmac_vap(pst_device->auc_vap_id[0]);
    if((pst_dmac_vap) && (WLAN_VAP_MODE_BSS_AP == pst_dmac_vap->st_vap_base_info.en_vap_mode))
    {
        return pst_dmac_vap;
    }

    return  OAL_PTR_NULL;
}


oal_void dmac_green_ap_tbtt_isr(oal_uint8 uc_hal_vap_id, oal_void  *p_arg)
{
    dmac_green_ap_mgr_stru     *pst_gap_mgr;
    hal_to_dmac_device_stru    *pst_hal_to_dmac_device = (hal_to_dmac_device_stru *)p_arg;
    mac_device_stru            *pst_mac_device;
    dmac_vap_stru              *pst_dmac_vap;
    oal_uint32                  ul_beacon_period;

    pst_mac_device  = mac_res_get_dev(pst_hal_to_dmac_device->uc_mac_device_id);
    if (OAL_UNLIKELY(NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_tbtt_isr::pst_mac_device == NULL}");
        return;
    }

    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_mac_device->pst_green_ap_mgr;
    if ((OAL_FALSE == pst_gap_mgr->uc_green_ap_enable)
       || (OAL_FALSE == pst_gap_mgr->en_green_ap_dyn_en))
    {
        return;
    }

    /* 防止未start就进入 */
    if (pst_gap_mgr->uc_state < DMAC_GREEN_AP_STATE_INITED)
    {
        return;
    }

    /* 每次tbtt中断获取vap，防止green ap启动时vap还未创建 */
    pst_dmac_vap = dmac_green_ap_get_vap(pst_mac_device);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_tbtt_isr:: dmac_green_ap_get_vap fail!}");
        return;
    }

    /* 初始化结构体有关变量 */
    if (DMAC_GREEN_AP_STATE_INITED == pst_gap_mgr->uc_state)
    {
        pst_gap_mgr->uc_vap_id     = pst_dmac_vap->st_vap_base_info.uc_vap_id;
        pst_gap_mgr->uc_hal_vap_id = pst_dmac_vap->pst_hal_vap->uc_vap_id;
        mac_fcs_prepare_one_packet_cfg(&pst_dmac_vap->st_vap_base_info, &pst_gap_mgr->st_one_packet_cfg, pst_gap_mgr->uc_pause_time);

        ul_beacon_period = pst_dmac_vap->st_vap_base_info.pst_mib_info->st_wlan_mib_sta_config.ul_dot11BeaconPeriod;
        if (0 != ul_beacon_period)
        {
            pst_gap_mgr->uc_max_slot_cnt = 2 * ul_beacon_period / (pst_gap_mgr->uc_work_time + pst_gap_mgr->uc_pause_time);
        }
        else
        {
            pst_gap_mgr->uc_max_slot_cnt = 2 * 100 / (pst_gap_mgr->uc_work_time + pst_gap_mgr->uc_pause_time);
        }

        /* 初始状态为work */
        pst_gap_mgr->uc_state = DMAC_GREEN_AP_STATE_WORK;
    }

    /*最后一个时隙不启动timer，tbtt中断后启动*/
    if ((pst_gap_mgr->uc_hal_vap_id == uc_hal_vap_id) && (DMAC_GREEN_AP_STATE_WORK == pst_gap_mgr->uc_state))
    {
       pst_gap_mgr->uc_cur_slot = 0;
       dmac_green_ap_start_timer(pst_gap_mgr, pst_gap_mgr->uc_work_time);
    }
    else if ((pst_gap_mgr->uc_hal_vap_id == uc_hal_vap_id) && (DMAC_GREEN_AP_STATE_PAUSE == pst_gap_mgr->uc_state))
    {
        /*异常情况，tbtt时刻，还处于pause状态,发生了不同步*/
        OAM_ERROR_LOG1(pst_gap_mgr->uc_vap_id, OAM_SF_GREEN_AP, "{dmac_green_ap_tbtt_isr::in pause state,cur_slot = %d}",pst_gap_mgr->uc_cur_slot);

        pst_gap_mgr->uc_cur_slot = 0;
        pst_dmac_vap = mac_res_get_dmac_vap(pst_gap_mgr->uc_vap_id);
        dmac_green_ap_resume_vap(pst_mac_device, pst_dmac_vap,pst_gap_mgr);
        dmac_green_ap_start_timer(pst_gap_mgr, pst_gap_mgr->uc_work_time);
    }
}


oal_void  dmac_green_ap_fcs_isr(oal_uint8 uc_vap_id, oal_void  *p_arg)
{
    mac_device_stru            *pst_mac_device;
    hal_to_dmac_device_stru    *pst_hal_to_dmac_device = (hal_to_dmac_device_stru *)p_arg;

    mac_fcs_verify_timestamp(MAC_FCS_STAGE_ONE_PKT_INTR);

    if (OAL_UNLIKELY(NULL == pst_hal_to_dmac_device))
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_GREEN_AP, "{dmac_green_ap_fcs_isr::in mac_fcs_isr p_arg=NULL}");
        return;
    }

    pst_mac_device  = mac_res_get_dev(pst_hal_to_dmac_device->uc_mac_device_id);
    if (OAL_UNLIKELY(NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(uc_vap_id, OAM_SF_GREEN_AP, "{dmac_green_ap_fcs_isr::pst_mac_device == NULL}");
        return;
    }

    pst_mac_device->st_fcs_mgr.en_fcs_done = OAL_TRUE;
}


oal_uint32  dmac_green_ap_init(mac_device_stru *pst_device)
{
    dmac_green_ap_mgr_stru  *pst_gap_mgr;

    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_init:: pst_device is NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_gap_mgr = &g_ast_gap_mgr[pst_device->uc_device_id];

    /* Green AP功能E5上默认打开 */
#if (_PRE_CONFIG_TARGET_PRODUCT == _PRE_TARGET_PRODUCT_TYPE_E5)
    pst_gap_mgr->uc_green_ap_enable  = OAL_TRUE;
#else
    pst_gap_mgr->uc_green_ap_enable  = OAL_FALSE;
#endif
    pst_gap_mgr->en_green_ap_dyn_en      = OAL_FALSE;
    pst_gap_mgr->en_green_ap_dyn_en_old  = OAL_FALSE;
    pst_gap_mgr->uc_green_ap_suspend     = OAL_FALSE;
    pst_gap_mgr->uc_state                = DMAC_GREEN_AP_STATE_NOT_INITED;
    pst_gap_mgr->uc_device_id            = pst_device->uc_device_id;
    pst_gap_mgr->uc_vap_id               = 0xff;
    pst_gap_mgr->uc_hal_vap_id           = 0xff;

    pst_gap_mgr->uc_work_time       = DMAC_DEFAULT_GAP_WORK_TIME;
    pst_gap_mgr->uc_pause_time      = DMAC_DEFAULT_GAP_PAUSE_TIME;
    pst_gap_mgr->uc_max_slot_cnt    = DMAC_DEFAULT_GAP_SLOT_CNT;
    pst_gap_mgr->uc_cur_slot        = 0;
    pst_gap_mgr->uc_resume_offset   = DMAC_DEFAULT_GAP_RESUME_OFFSET;

    pst_gap_mgr->ul_pause_count     = 0;
    pst_gap_mgr->ul_resume_count    = 0;
    pst_gap_mgr->ul_total_count     = 0;

    pst_device->pst_green_ap_mgr = pst_gap_mgr;

    hal_register_gap_isr_hook(pst_device->pst_device_stru, HAL_ISR_TYPE_TBTT, dmac_green_ap_tbtt_isr);
    hal_register_gap_isr_hook(pst_device->pst_device_stru, HAL_ISR_TYPE_ONE_PKT, dmac_green_ap_fcs_isr);

    dmac_green_ap_setup_timer(pst_gap_mgr);

    /* 注册并启动PPS统计 */
    dmac_set_auto_freq_pps_reuse();

    /* 自动打开green ap */
    //dmac_green_ap_start(pst_device->uc_device_id);

    return  OAL_SUCC;
}


oal_uint32  dmac_green_ap_exit(mac_device_stru *pst_device)
{
    dmac_green_ap_mgr_stru  *pst_gap_mgr;

    pst_gap_mgr = pst_device->pst_green_ap_mgr;

    hal_unregister_gap_isr_hook(pst_device->pst_device_stru, HAL_ISR_TYPE_TBTT);
    hal_unregister_gap_isr_hook(pst_device->pst_device_stru, HAL_ISR_TYPE_ONE_PKT);

    dmac_green_ap_release_timer(pst_gap_mgr);

    /* 去注册PPS统计 */
    dmac_set_auto_freq_pps_reuse_deinit();

    if (OAL_PTR_NULL != pst_device->pst_green_ap_mgr)
    {
        pst_device->pst_green_ap_mgr = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_uint32  dmac_green_ap_start(oal_uint8 uc_device_id)
{
    mac_device_stru         *pst_device;
    dmac_green_ap_mgr_stru  *pst_gap_mgr;

    pst_device  = mac_res_get_dev(uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_stop:: mac_res_get_dev NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;

    /* 未使能，返回 */
    if (OAL_FALSE == pst_gap_mgr->uc_green_ap_enable)
    {
        OAM_WARNING_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_start:: green ap is not enable!}");
        return OAL_FAIL;
    }

    /* 等待tbtt中断到来 */
    pst_gap_mgr->uc_state = DMAC_GREEN_AP_STATE_INITED;

    return OAL_SUCC;
}


oal_uint32  dmac_green_ap_stop(oal_uint8 uc_device_id)
{
    mac_device_stru         *pst_device;
    dmac_vap_stru           *pst_dmac_vap;
    dmac_green_ap_mgr_stru  *pst_gap_mgr;

    pst_device  = mac_res_get_dev((oal_uint32)uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_stop:: mac_res_get_dev NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;
    if(OAL_FALSE == pst_gap_mgr->uc_green_ap_enable)
    {
        OAM_WARNING_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_stop:: green ap not enabled!}");
        return OAL_SUCC;
    }

    pst_dmac_vap = mac_res_get_dmac_vap(pst_gap_mgr->uc_vap_id);
    if(OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_WARNING_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_stop:: pst_dmac_vap NULL!}");
        return OAL_FAIL;
    }

    dmac_green_ap_stop_timer(pst_gap_mgr);
    dmac_green_ap_release_timer(pst_gap_mgr);

    if (DMAC_GREEN_AP_STATE_PAUSE == pst_gap_mgr->uc_state)
    {
        OAM_ERROR_LOG1(pst_gap_mgr->uc_vap_id, OAM_SF_GREEN_AP, "{dmac_green_ap_tbtt_isr::in pause state,cur_slot = %d}",pst_gap_mgr->uc_cur_slot);

        dmac_green_ap_resume_vap(mac_res_get_dev(pst_dmac_vap->st_vap_base_info.uc_device_id),pst_dmac_vap,pst_gap_mgr);
    }

    pst_gap_mgr->uc_vap_id  = 0xff;
    pst_gap_mgr->uc_state   = DMAC_GREEN_AP_STATE_NOT_INITED;

    return OAL_SUCC;
}


oal_uint32  dmac_green_ap_suspend(mac_device_stru *pst_device)
{
    dmac_green_ap_mgr_stru  *pst_gap_mgr;

    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;

    if (OAL_PTR_NULL == pst_gap_mgr)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_resume:: pst_gap_mgr NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if(OAL_FALSE == pst_gap_mgr->uc_green_ap_enable)
    {
        return OAL_SUCC;
    }

    dmac_green_ap_stop(pst_device->uc_device_id);

    pst_gap_mgr->uc_green_ap_suspend = OAL_TRUE;

    return OAL_SUCC;
}


oal_uint32  dmac_green_ap_resume(mac_device_stru *pst_device)
{
    dmac_green_ap_mgr_stru  *pst_gap_mgr;

    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;

    if (OAL_PTR_NULL == pst_gap_mgr)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_resume:: pst_gap_mgr NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 防止wow唤醒,直接开启green ap */
    if (OAL_FALSE == pst_gap_mgr->en_green_ap_dyn_en)
    {
        return OAL_SUCC;
    }

    if(OAL_TRUE == pst_gap_mgr->uc_green_ap_suspend)
    {
        dmac_green_ap_start(pst_device->uc_device_id);
    }

    return OAL_SUCC;
}


oal_uint32  dmac_green_ap_dump_info(oal_uint8 uc_device_id)
{
    mac_device_stru         *pst_device;
    dmac_green_ap_mgr_stru  *pst_gap_mgr;

    pst_device = mac_res_get_dev(uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_GREEN_AP, "{dmac_green_ap_dump_info:: pst_device NULL!}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;

    OAL_IO_PRINT("Enable[%d]:Pause[%d],resume[%d],total[%d]\r\n", pst_gap_mgr->uc_green_ap_enable,
           pst_gap_mgr->ul_pause_count, pst_gap_mgr->ul_resume_count, pst_gap_mgr->ul_total_count);

    return OAL_SUCC;
}


oal_void  dmac_green_ap_pps_process(oal_uint32 ul_pps_rate)
{
    mac_device_stru         *pst_device;
    dmac_green_ap_mgr_stru  *pst_gap_mgr;
    oal_uint32               ul_thrpt_stat;

    pst_device  = mac_res_get_dev(0);
    pst_gap_mgr = (dmac_green_ap_mgr_stru *)pst_device->pst_green_ap_mgr;

    /* pps转换吞吐量(Mbps) */
    ul_thrpt_stat = DMAC_GAP_PPS_TO_THRPT_MBPS(ul_pps_rate);

    pst_gap_mgr->en_green_ap_dyn_en_old = pst_gap_mgr->en_green_ap_dyn_en;

    /* 吞吐量大于门限，关闭green ap */
    if (ul_thrpt_stat > DMAC_GAP_EN_THRPT_HIGH_TH)
    {
        pst_gap_mgr->en_green_ap_dyn_en = OAL_FALSE;
    }
    else
    {
        pst_gap_mgr->en_green_ap_dyn_en = OAL_TRUE;
    }

    if (pst_gap_mgr->en_green_ap_dyn_en_old != pst_gap_mgr->en_green_ap_dyn_en)
    {
        OAM_WARNING_LOG3(0, OAM_SF_GREEN_AP, "{dmac_green_ap_pps_process:: green ap en change from[%d] to [%d], throughput[%d]}",
                    pst_gap_mgr->en_green_ap_dyn_en_old, pst_gap_mgr->en_green_ap_dyn_en, ul_thrpt_stat);
    }
}

#endif /* _PRE_WLAN_FEATURE_GREEN_AP */

#ifdef  __cplusplus
#if     __cplusplus
    }
#endif
#endif
