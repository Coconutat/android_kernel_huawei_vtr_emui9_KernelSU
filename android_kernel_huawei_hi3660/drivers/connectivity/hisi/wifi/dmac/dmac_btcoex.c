


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "mac_data.h"
#include "dmac_btcoex.h"
#include "dmac_alg_if.h"
#include "dmac_alg.h"
#include "dmac_auto_adjust_freq.h"
#include "dmac_device.h"
#include "dmac_resource.h"
#include "dmac_scan.h"
#include "hal_coex_reg.h"
#include "dmac_tx_bss_comm.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_BTCOEX_C

#define WLAN_TIDNO_COEX_QOSNULL     WLAN_TIDNO_ANT_TRAINING_HIGH_PRIO   /* 发送qos null复用智能天线高优先级训练帧 */
#define BTCOEX_RATE_THRESHOLD_MIN   0
#define BTCOEX_RATE_THRESHOLD_MAX   1
#define BTCOEX_RATE_THRESHOLD_BUTT  2


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
extern oal_uint32  dmac_send_sys_event(mac_vap_stru *pst_mac_vap,
                                                wlan_cfgid_enum_uint16 en_cfg_id,
                                                oal_uint16 us_len,
                                                oal_uint8 *puc_param);
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
extern oal_uint32 dmac_config_set_device_freq(mac_vap_stru *pst_mac_vap, oal_uint8 uc_len, oal_uint8 *puc_param);
#endif
extern oal_uint32 dmac_btcoex_ps_status_handler(frw_event_mem_stru *pst_event_mem);
OAL_STATIC OAL_INLINE oal_void dmac_btcoex_rx_average_rate_calculate (dmac_user_btcoex_rx_info_stru *pst_btcoex_wifi_rx_rate_info,
                                                                                oal_uint32 *pul_rx_rate, oal_uint16 *pus_rx_count);

oal_uint8 g_auc_rx_win_size[BTCOEX_RX_WINDOW_SIZE_GRADES][BTCOEX_RX_WINDOW_SIZE_INDEX_MAX] = {
    {2, 4, 8, 64}, {4, 8, 32, 64}
};
/****************************************
              音乐    电话    数传
    2G/20M      0       0       0
    5G/20M      0       0       0
    2G/40M      0       1       1
    5G/40M      0       0       1
    5G/80M      1       1       1
*****************************************/
oal_uint8 g_auc_rx_win_size_grage[WLAN_BAND_BUTT][WLAN_BW_CAP_BUTT][BTCOEX_BT_NUM_OF_ACTIVE_MODE] = {
    /* 2G */
    /* 20M */  /* 40M */
    /* 音乐, 电话, 数传 */
    {{0, 0, 0}, {0, 1, 1}, {1, 1, 1}, {1, 1, 1}},
    /* 5G */
    /* 20M */  /* 40M */  /* 80M */
    /* 音乐, 电话, 数传 */
    {{0, 0, 0}, {0, 0, 1}, {1, 1, 1}, {1, 1, 1}}
};
oal_uint16 g_aus_btcoex_rate_thresholds[WLAN_BAND_BUTT][WLAN_BW_CAP_BUTT][BTCOEX_RATE_THRESHOLD_BUTT] = {
    /* 2G */
    /* 20M */  /* 40M */
    {{23, 60}, {50, 130}, {0, 0},     {0, 0}},
    /* 5G */
    /* 20M */  /* 40M */  /* 80M */
    {{23, 80}, {50, 160}, {108, 340}, {0, 0}}
};

oal_uint32 g_rx_statistics_print = 0;

/*****************************************************************************
  3 函数实现
*****************************************************************************/


OAL_STATIC oal_void dmac_btcoex_encap_preempt_sta(oal_uint8 *puc_frame,
                                                               oal_uint8 *puc_da,
                                                               oal_uint8 *puc_sa,
                                                               coex_preempt_frame_enum_uint8 en_preempt_type)
{
    oal_uint32                        ul_qosnull_seq_num = 0;
    /* 填写帧头,其中from ds为1，to ds为0，ps=1 因此frame control的第二个字节为12 */
    if (BTCOEX_NULLDATA == en_preempt_type)
    {

        mac_ieee80211_frame_stru *pst_mac_header = (mac_ieee80211_frame_stru *)puc_frame;
        mac_null_data_encap(puc_frame,
                            WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_NODATA | 0x1100,
                            puc_da,
                            puc_sa);
        /* 设定seq num和frag */
        puc_frame[22] = 0;
        puc_frame[23] = 0;
        pst_mac_header->st_frame_control.bit_power_mgmt = 1;
    }
    else if (BTCOEX_QOSNULL == en_preempt_type)
    {
        dmac_btcoex_qosnull_frame_stru *pst_mac_header = (dmac_btcoex_qosnull_frame_stru *)puc_frame;
        mac_null_data_encap(puc_frame,
                            WLAN_PROTOCOL_VERSION | WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_QOS_NULL | 0x1100,
                            puc_da,
                            puc_sa);
        pst_mac_header->st_frame_control.bit_power_mgmt = 1;
        pst_mac_header->bit_qc_tid = WLAN_TIDNO_COEX_QOSNULL;
        pst_mac_header->bit_qc_eosp = 0;

        /* 设置seq的序列号 */
        hal_get_btcoex_abort_qos_null_seq_num(&ul_qosnull_seq_num);
        pst_mac_header->bit_sc_seq_num = (ul_qosnull_seq_num + 1);
        hal_set_btcoex_abort_qos_null_seq_num(pst_mac_header->bit_sc_seq_num);

        /*协议规定单播的QOS NULL DATA只允许normal ack 共存里面要设置0是对方会回ack */
        pst_mac_header->bit_qc_ack_polocy = WLAN_TX_NORMAL_ACK;

    }
    else
    {
    }

}


oal_void dmac_btcoex_init_preempt(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, coex_preempt_frame_enum_uint8 en_preempt_type)
{
    dmac_vap_stru *pst_dmac_vap;

    pst_dmac_vap = (dmac_vap_stru *)pst_mac_vap;
    /* 填写帧头*/
    dmac_btcoex_encap_preempt_sta(pst_dmac_vap->st_dmac_vap_btcoex.auc_null_qosnull_frame,
                                  pst_mac_user->auc_user_mac_addr,
                                  pst_mac_vap->pst_mib_info->st_wlan_mib_sta_config.auc_dot11StationID,
                                  en_preempt_type);

    hal_set_btcoex_abort_null_buff_addr((oal_uint32)pst_dmac_vap->st_dmac_vap_btcoex.auc_null_qosnull_frame);

    return;
}


OAL_STATIC OAL_INLINE oal_void dmac_btcoex_update_ba_size(mac_vap_stru *pst_mac_vap, dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba, hal_btcoex_btble_status_stru *pst_btble_status)
{
    bt_status_stru *pst_bt_status;
    ble_status_stru *pst_ble_status;
    oal_uint8 uc_bt_active_mode;
    oal_uint8 uc_bt_rx_win_size_grade;
    oal_uint8 uc_band;
    oal_uint8 uc_bandwidth;

    pst_bt_status = &(pst_btble_status->un_bt_status.st_bt_status);
    pst_ble_status = &(pst_btble_status->un_ble_status.st_ble_status);

    uc_band = pst_mac_vap->st_channel.en_band;
    mac_vap_get_bandwidth_cap(pst_mac_vap, &uc_bandwidth);
    if ((uc_band >= WLAN_BAND_BUTT) || (uc_bandwidth >= WLAN_BW_CAP_BUTT))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_ba_size::band %d, bandwidth %d exceed scale!}",
                         uc_band, uc_bandwidth);
        return;
    }
    if (pst_dmac_user_btcoex_delba->uc_ba_size_index >= BTCOEX_RX_WINDOW_SIZE_INDEX_MAX)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_ba_size::ba_size_index %d exceed scale!}",
                         pst_dmac_user_btcoex_delba->uc_ba_size_index);
        return;
    }

    if (pst_bt_status->bit_bt_sco)
    {
        uc_bt_active_mode = 1;
        /* 6slot 设备 */
        if (2 == pst_ble_status->bit_bt_6slot)
        {
            pst_dmac_user_btcoex_delba->uc_ba_size = 1;
            return;
        }
    }
    else if (pst_bt_status->bit_bt_a2dp)
    {
        uc_bt_active_mode = 0;
    }
    else if (pst_ble_status->bit_bt_transfer)
    {
        uc_bt_active_mode = 2;
    }
    else
    {
        uc_bt_active_mode = BTCOEX_BT_NUM_OF_ACTIVE_MODE;
    }

    /* BT没有业务, 聚合64 */
    if (uc_bt_active_mode >= BTCOEX_BT_NUM_OF_ACTIVE_MODE)
    {
        uc_bt_active_mode = 0;
        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
    }

    uc_bt_rx_win_size_grade = g_auc_rx_win_size_grage[uc_band][uc_bandwidth][uc_bt_active_mode];

    pst_dmac_user_btcoex_delba->uc_ba_size = g_auc_rx_win_size[uc_bt_rx_win_size_grade][pst_dmac_user_btcoex_delba->uc_ba_size_index];
}


oal_void dmac_btcoex_ps_stop_check_and_notify(oal_void)
{
    mac_device_stru         *pst_mac_device = OAL_PTR_NULL;
    hal_to_dmac_device_stru *pst_hal_device = OAL_PTR_NULL;
    mac_vap_stru            *pst_mac_vap    = OAL_PTR_NULL;
    oal_bool_enum_uint8      en_sco_status  = OAL_FALSE;
    oal_bool_enum_uint8      en_ps_stop     = OAL_FALSE;  /* 初始是打开ps */
    oal_uint8                uc_vap_idx;
    oal_uint8                uc_ap_num     = 0;
    oal_uint8                uc_sta_num    = 0;
    oal_uint8                uc_go_num     = 0;
    oal_uint8                uc_gc_num     = 0;

    pst_mac_device = mac_res_get_dev(0);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_stop_check_and_notify: mac device is null }");
        return;
    }
    pst_hal_device = pst_mac_device->pst_device_stru;
    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_btcoex_ps_stop_check_and_notify:pst_hal_device is null");
        return;
    }

    /* 1.电话场景 */
    hal_btcoex_get_bt_sco_status(pst_hal_device, &en_sco_status);
    if (OAL_TRUE == en_sco_status)
    {
        en_ps_stop = OAL_TRUE;
    }

    /* 2. dbac在运行,直接return */
    if ((OAL_TRUE == mac_is_dbac_running(pst_mac_device)))
    {
        en_ps_stop = OAL_TRUE;
    }

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }
        if (IS_P2P_GO(pst_mac_vap))
        {
            uc_go_num++;
        }
        else if (IS_P2P_CL(pst_mac_vap))
        {
            uc_gc_num++;
        }
        else if (IS_LEGACY_AP(pst_mac_vap))
        {
            uc_ap_num++;
        }
        else if (IS_LEGACY_STA(pst_mac_vap))
        {
            uc_sta_num++;
        }
    }
    if ((0 != uc_go_num) || (0 != uc_ap_num) || (0 != uc_sta_num && 0 != uc_gc_num))
    {
        en_ps_stop = OAL_TRUE;
    }

    /* 刷新ps能力 */
    GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_hal_device) = en_ps_stop;

    hal_set_btcoex_soc_gpreg1(en_ps_stop, BTCOEX_WIFI_STATUS_REG1_PS_STATE_MASK, BTCOEX_WIFI_STATUS_REG1_PS_STATE_OFFSET);  //ps禁止状态通知

    hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);

    OAM_WARNING_LOG_ALTER(0, OAM_SF_COEX,
        "{dmac_btcoex_ps_stop_check_and_notify::ap_num[%d]sta_num[%d]sco_status[%d]dbac_status[%d]ps_stop[%d]!}",
        5, uc_ap_num, uc_sta_num, en_sco_status,
        mac_is_dbac_running(pst_mac_device), GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_hal_device));
}


oal_void dmac_btcoex_ps_pause_check_and_notify(hal_to_dmac_device_stru *pst_hal_device)
{
    mac_device_stru      *pst_mac_device = OAL_PTR_NULL;
    mac_vap_stru         *pst_mac_vap    = OAL_PTR_NULL;
    oal_bool_enum_uint8   en_ps_pause     = OAL_FALSE;  /* 初始是不暂停ps */
    oal_uint8             uc_vap_idx;

    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_btcoex_ps_pause_check_and_notify:pst_hal_device is null");
        return;
    }
    pst_mac_device = mac_res_get_dev(pst_hal_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_pause_check_and_notify: mac device is null ptr. device id:%d}", pst_hal_device->uc_mac_device_id);
        return;
    }
    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        pst_mac_vap  = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == pst_mac_vap)
        {
            continue;
        }

        /* 1.处于漫游过程中需要暂停ps */
        if(MAC_VAP_STATE_ROAMING == pst_mac_vap->en_vap_state)
        {
            en_ps_pause = OAL_TRUE;
        }
    }

    /* 刷新ps能力 */
    GET_HAL_BTCOEX_SW_PREEMPT_PS_PAUSE(pst_hal_device) = en_ps_pause;

    OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_pause_check_and_notify::en_ps_pause[%d]!}",
        GET_HAL_BTCOEX_SW_PREEMPT_PS_PAUSE(pst_hal_device));
}


OAL_STATIC oal_uint32 dmac_bt_transfer_status_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru *pst_event;
    hal_btcoex_btble_status_stru *pst_btcoex_btble_status;
    mac_device_stru *pst_mac_device;
    mac_vap_stru *pst_mac_vap;
    hal_to_dmac_device_stru *pst_hal_device;
    dmac_user_stru *pst_dmac_user;
    dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_bt_transfer_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_bt_transfer_status_handler::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    dmac_btcoex_get_legacy_sta(pst_mac_device, &pst_mac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hal_device = pst_mac_device->pst_device_stru;
    pst_btcoex_btble_status = &(pst_hal_device->st_btcoex_btble_status);

    if (pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_transfer)
    {
        dmac_alg_cfg_btcoex_state_notify(pst_mac_device,BT_TRANSFER_ON);
    }
    else
    {
        dmac_alg_cfg_btcoex_state_notify(pst_mac_device,BT_TRANSFER_OFF);
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    /* 正常bt打开和关闭场景下，对聚合进行处理 */
    if ((pst_btcoex_btble_status->un_bt_status.st_bt_status.bit_bt_on) && (pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_transfer))
    {
        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
        pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
    }
    else
    {
        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
        pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
    }
    dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);
    OAM_WARNING_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_bt_transfer_status_handler::bt transfer status changed:%d, bar_size: %d}",
            pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_transfer,
            pst_dmac_user_btcoex_delba->uc_ba_size);

    dmac_btcoex_delba_trigger(pst_mac_vap, OAL_TRUE, pst_dmac_user_btcoex_delba->uc_ba_size);
    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_rx_rate_statistics_flag_callback(oal_void *p_arg)
{
    mac_device_stru *pst_mac_device;
    mac_vap_stru *pst_mac_vap;
    dmac_vap_stru *pst_dmac_vap;
    dmac_user_stru *pst_dmac_user;
    hal_btcoex_btble_status_stru *pst_btcoex_btble_status;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru *pst_dmac_user_btcoex_rx_info;

    pst_mac_vap = (mac_vap_stru *)p_arg;
    pst_dmac_vap = (dmac_vap_stru *)p_arg;
    if (OAL_FALSE == dmac_btcoex_check_legacy_sta(pst_mac_vap))
    {
        return OAL_SUCC;
    }
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_rx_rate_statistics_flag_callback::pst_device[id:%d] NULL}",
                    pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_rx_rate_statistics_flag_callback::pst_dmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_btcoex_btble_status = &(pst_mac_device->pst_device_stru->st_btcoex_btble_status);
    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_rx_info = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_rx_info);

    /* BT业务结束 */
    if (OAL_FALSE == pst_btcoex_btble_status->un_ble_status.st_ble_status.bit_bt_ba)
    {
        dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba;
        pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
        pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_flag = OAL_FALSE;
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_low_rate_timer));
        if (BTCOEX_RX_WINDOW_SIZE_INDEX_3 != pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index)
        {
            pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);
            dmac_btcoex_delba_trigger(pst_mac_vap, OAL_TRUE, pst_dmac_user_btcoex_delba->uc_ba_size);
        }
        OAL_MEMZERO(pst_dmac_user_btcoex_rx_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));
        return OAL_SUCC;
    }

    if (pst_dmac_user_btcoex_rx_info->us_rx_rate_stat_count < BTCOEX_RX_COUNT_LIMIT)
    {
        OAL_MEMZERO(pst_dmac_user_btcoex_rx_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));
        return OAL_SUCC;
    }

    pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_timeout = OAL_TRUE;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_sco_rx_rate_statistics_flag_callback(oal_void *p_arg)
{
    mac_device_stru *pst_mac_device;
    dmac_device_stru *pst_dmac_device;
    mac_vap_stru *pst_mac_vap;
    dmac_vap_stru *pst_dmac_vap;
    dmac_user_stru *pst_dmac_user;
    dmac_user_btcoex_sco_rx_rate_status_stru *pst_dmac_user_btcoex_sco_rx_rate_status;
    oal_uint32 ul_rx_rate = 0;
    oal_uint16 us_rx_count = 0;
    oal_uint8 uc_notify_bt = OAL_FALSE;
    oal_uint8 uc_notify_bt_value = 0;
    hal_btcoex_btble_status_stru *pst_btcoex_btble_status;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru *pst_btcoex_wifi_rx_rate_info;

    pst_mac_vap = (mac_vap_stru *)p_arg;
    pst_dmac_vap = (dmac_vap_stru *)p_arg;
    if (OAL_FALSE == dmac_btcoex_check_legacy_sta(pst_mac_vap))
    {
        return OAL_SUCC;
    }
    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_statistics_flag_callback::pst_device[id:%d] NULL}",
                    pst_mac_vap->uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_statistics_flag_callback::pst_dmac_user null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_device = dmac_res_get_mac_dev(pst_dmac_user->st_user_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_statistics_flag_callback::pst_dmac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_btcoex_btble_status = &(pst_mac_device->pst_device_stru->st_btcoex_btble_status);
    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_btcoex_wifi_rx_rate_info = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_sco_rx_info);

    /* BT业务结束 */
    if (!pst_btcoex_btble_status->un_bt_status.st_bt_status.bit_bt_sco)
    {
        if (!pst_btcoex_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp)
        {
            dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba;
            pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
            pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
            dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);
            dmac_btcoex_delba_trigger(pst_mac_vap, OAL_TRUE, pst_dmac_user_btcoex_delba->uc_ba_size);
        }
        pst_dmac_vap_btcoex_rx_statistics->uc_sco_rx_rate_statistics_flag = OAL_FALSE;
        OAL_MEMZERO(pst_btcoex_wifi_rx_rate_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));
        return OAL_SUCC;
    }

    /* 计算接收速率 */
    dmac_btcoex_rx_average_rate_calculate(pst_btcoex_wifi_rx_rate_info, &ul_rx_rate, &us_rx_count);

    /* 处理近场速率 */
    if (pst_dmac_device->st_dmac_alg_stat.en_dmac_device_distance_enum < DMAC_ALG_TPC_FAR_DISTANCE)
    {
        pst_dmac_user_btcoex_sco_rx_rate_status = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_sco_rx_rate_status);

        /* 根据不同速率确定不同的速率等级:高速、中速、低速、超低速 */
        if (0 == us_rx_count)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status)
            {
            case BTCOEX_RATE_STATE_M:
            case BTCOEX_RATE_STATE_L:
            case BTCOEX_RATE_STATE_SL:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_H;
                uc_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else if (us_rx_count <= 2)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status)
            {
            case BTCOEX_RATE_STATE_SL:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_L;
                uc_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else if (1 == ul_rx_rate)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status)
            {
            case BTCOEX_RATE_STATE_H:
            case BTCOEX_RATE_STATE_M:
            case BTCOEX_RATE_STATE_L:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_SL;
                uc_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else if (ul_rx_rate <= 11)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status)
            {
            case BTCOEX_RATE_STATE_H:
            case BTCOEX_RATE_STATE_M:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_L;
                uc_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else if (ul_rx_rate < 35)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status)
            {
            case BTCOEX_RATE_STATE_H:
            case BTCOEX_RATE_STATE_L:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_M;
                uc_notify_bt = OAL_TRUE;
                break;
            case BTCOEX_RATE_STATE_SL:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_L;
                uc_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        /* 速率大于50Mb/s，或接收包数超过500个，认为当前速率正常，告知BT当前Wifi处于高速状态 */
        else if (us_rx_count >= 250 || ul_rx_rate >= 50)
        {
            switch (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status)
            {
            case BTCOEX_RATE_STATE_M:
            case BTCOEX_RATE_STATE_L:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_H;
                uc_notify_bt = OAL_TRUE;
                break;
            case BTCOEX_RATE_STATE_SL:
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status = BTCOEX_RATE_STATE_L;
                uc_notify_bt = OAL_TRUE;
                break;
            default:
                break;
            }
        }
        else
        {}
        uc_notify_bt_value = pst_dmac_user_btcoex_sco_rx_rate_status->uc_status;

        /* 防止一直在超低速导致电话太卡，设置超时恢复成低速，本身状态不变，仅通知BT消息改变 */
        if (BTCOEX_RATE_STATE_SL == pst_dmac_user_btcoex_sco_rx_rate_status->uc_status)
        {
            pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time++;
            if (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time > (ALL_MID_PRIO_TIME + ALL_HIGH_PRIO_TIME))
            {
                pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time = 0;
                uc_notify_bt = OAL_TRUE;
                uc_notify_bt_value = BTCOEX_RATE_STATE_SL;
            }
            else if (pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time > ALL_MID_PRIO_TIME)
            {
                uc_notify_bt = OAL_TRUE;
                uc_notify_bt_value = BTCOEX_RATE_STATE_L;
            }
        }
        else
        {
            pst_dmac_user_btcoex_sco_rx_rate_status->uc_status_sl_time = 0;
        }

        if (OAL_TRUE == uc_notify_bt)
        {
            hal_set_btcoex_soc_gpreg1(uc_notify_bt_value, BIT4 | BIT5, 4);
            hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);
            OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_process::uc_notify_status: %d, uc_stay_status %d, uc_rate: %d, count %d.}",
                                uc_notify_bt_value, pst_dmac_user_btcoex_sco_rx_rate_status->uc_status, ul_rx_rate, us_rx_count);
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_a2dp_status_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    mac_vap_stru                       *pst_mac_vap;
    mac_device_stru                    *pst_mac_device;
    hal_to_dmac_device_stru            *pst_hal_device;
    dmac_vap_stru                      *pst_dmac_vap;
    dmac_user_stru                     *pst_dmac_user;
    hal_btcoex_btble_status_stru       *pst_btble_status;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_delba_stru        *pst_dmac_user_btcoex_delba;
    oal_uint8                           uc_need_delba;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_status_event_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_status_event_handler::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hal_device = pst_mac_device->pst_device_stru;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_status_event_handler::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    dmac_btcoex_get_legacy_sta(pst_mac_device, &pst_mac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap = (dmac_vap_stru *)pst_mac_vap;
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
    pst_btble_status = &(pst_hal_device->st_btcoex_btble_status);
    uc_need_delba = OAL_FALSE;

    /* 正常bt打开和关闭场景下，对聚合进行处理 */
    if (pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp)
    {
        if (BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index)
        {
            pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_1;
        }
        else
        {
            pst_dmac_user_btcoex_delba->uc_ba_size_index = pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index;
        }
        pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_flag = OAL_TRUE;
    }
    /* 只有电话音乐都没有的情况才进行恢复成64的聚合 */
    else if(!pst_btble_status->un_bt_status.st_bt_status.bit_bt_sco)
    {
        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
        pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_flag = OAL_FALSE;
    }
    /* 音乐结束的时候还有电话的场景，在这里不进行BA删建处理，由电话的流程控制 */
    else
    {
        return OAL_SUCC;
    }

    dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btble_status);
    OAM_WARNING_LOG3(0, OAM_SF_COEX, "{dmac_btcoex_status_event_handler::bt ba status changed:%d, bar_size: %d uc_need_delba:%d}",
            pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp,
            pst_dmac_user_btcoex_delba->uc_ba_size,uc_need_delba);
    dmac_btcoex_delba_trigger(pst_mac_vap, uc_need_delba, pst_dmac_user_btcoex_delba->uc_ba_size);

    pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_timeout = OAL_FALSE;
    FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer),
                               dmac_btcoex_rx_rate_statistics_flag_callback,
                               BTCOEX_RX_STATISTICS_TIME,
                               (void *)pst_mac_vap,
                               OAL_TRUE,
                               OAM_MODULE_ID_DMAC,
                               pst_mac_device->ul_core_id);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_page_scan_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru *pst_event;
    mac_vap_stru *pst_mac_vap;
    mac_device_stru *pst_mac_device;
    dmac_vap_stru *pst_dmac_vap;
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_page_scan_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_page_scan_handler::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    dmac_btcoex_get_legacy_sta(pst_mac_device, &pst_mac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap = (dmac_vap_stru *)pst_mac_vap;
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    hal_btcoex_update_ap_beacon_count(&(pst_dmac_vap_btcoex_occupied->ul_ap_beacon_count));
    pst_dmac_vap_btcoex_occupied->uc_ap_beacon_miss = 0;

    return OAL_SUCC;
}


OAL_STATIC oal_uint32 dmac_btcoex_sco_status_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru *pst_event;
    mac_vap_stru *pst_mac_vap;
    mac_device_stru *pst_mac_device;
    hal_to_dmac_device_stru *pst_hal_device;
    dmac_vap_stru *pst_dmac_vap;
    dmac_user_stru *pst_dmac_user;
    bt_status_stru *pst_bt_status;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba;
    hal_btcoex_btble_status_stru *pst_btble_status;
    oal_uint8 uc_need_delba = OAL_TRUE;
    oal_uint16 us_timeout_ms;
    oal_uint8 uc_periodic;
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    config_device_freq_h2d_stru st_device_freq;
    oal_uint8 uc_index;
    device_pps_freq_level_stru*    device_ba_pps_freq_level;
#endif
#endif

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = (frw_event_stru *)pst_event_mem->puc_data;
    pst_mac_device = mac_res_get_dev(pst_event->st_event_hdr.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_status_handler::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hal_device = pst_mac_device->pst_device_stru;
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hal_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_status_handler::pst_hal_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    dmac_btcoex_ps_stop_check_and_notify();
    dmac_btcoex_get_legacy_sta(pst_mac_device, &pst_mac_vap);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_vap))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_vap = (dmac_vap_stru *)pst_mac_vap;

    pst_btble_status = &(pst_hal_device->st_btcoex_btble_status);
    pst_bt_status = &(pst_btble_status->un_bt_status.st_bt_status);

#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    st_device_freq.uc_set_type = FREQ_SET_MODE;
    if (!pst_bt_status->bit_bt_sco)
    {
        /* 开启自动调频 */
        st_device_freq.uc_device_freq_enable = OAL_TRUE;
        device_ba_pps_freq_level = dmac_get_ba_pps_freq_level();
        for(uc_index = 0; uc_index < 4; uc_index++)
        {
            st_device_freq.st_device_data[uc_index].ul_speed_level = device_ba_pps_freq_level->ul_speed_level;
            st_device_freq.st_device_data[uc_index].ul_cpu_freq_level = device_ba_pps_freq_level->ul_cpu_freq_level;
            device_ba_pps_freq_level++;
        }
        dmac_config_set_device_freq(pst_mac_vap, 0, (oal_uint8 *)&st_device_freq);

    }
#endif
#endif
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_mac_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    /* 正常bt打开和关闭场景下，对聚合进行处理 */
    if (pst_bt_status->bit_bt_sco)
    {
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* 关闭自动调频 */
        st_device_freq.uc_device_freq_enable = OAL_FALSE;
        dmac_config_set_device_freq(pst_mac_vap, 0, (oal_uint8 *)&st_device_freq);
#endif
#endif
        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_1;
        if (pst_dmac_user_btcoex_delba->uc_ba_size_index == pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index)
        {
            uc_need_delba = OAL_FALSE;
        }

        pst_dmac_vap_btcoex_rx_statistics->uc_sco_rx_rate_statistics_flag = OAL_TRUE;
        us_timeout_ms = BTCOEX_SCO_CALCULATE_TIME;
        uc_periodic = OAL_TRUE;

    }
    /* 只有电话音乐都没有的情况才进行恢复成64的聚合 */
    else if(!pst_btble_status->un_bt_status.st_bt_status.bit_bt_a2dp)
    {
        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
        hal_set_btcoex_soc_gpreg1(0, BIT4 | BIT5, 4);
        uc_need_delba = OAL_FALSE;
        pst_dmac_vap_btcoex_rx_statistics->uc_sco_rx_rate_statistics_flag = OAL_FALSE;
        us_timeout_ms = BTCOEX_RX_STATISTICS_TIME;
        uc_periodic = OAL_FALSE;
    }
    /* 电话挂断后还有音乐的场景，不进行BA的删建处理，由音乐流程处理 */
    else
    {
        return OAL_SUCC;
    }

    FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_sco_statistics_timer),
                                       dmac_btcoex_sco_rx_rate_statistics_flag_callback,
                                       us_timeout_ms,
                                       (void *)pst_mac_vap,
                                       uc_periodic,
                                       OAM_MODULE_ID_DMAC,
                                       pst_mac_device->ul_core_id);

    dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btble_status);

    dmac_btcoex_delba_trigger(pst_mac_vap, uc_need_delba, pst_dmac_user_btcoex_delba->uc_ba_size);

    return OAL_SUCC;
}



oal_uint32 dmac_btcoex_register_dmac_misc_event(hal_dmac_misc_sub_type_enum en_event_type, oal_uint32 (*p_func)(frw_event_mem_stru *))
{
    if(en_event_type >= HAL_EVENT_DMAC_MISC_SUB_TYPE_BUTT || NULL == p_func)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_alg_register_dmac_misc_event fail");
        return  OAL_FAIL;
    }

    g_ast_dmac_misc_event_sub_table[en_event_type].p_func = p_func;

    return OAL_SUCC;
}


oal_uint32  dmac_btcoex_unregister_dmac_misc_event(hal_dmac_misc_sub_type_enum en_event_type)
{
    if(en_event_type >= HAL_EVENT_DMAC_MISC_SUB_TYPE_BUTT)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_alg_unregister_dmac_misc_event fail");
        return  OAL_FAIL;
    }

    g_ast_dmac_misc_event_sub_table[en_event_type].p_func = NULL;
    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_init(oal_void)
{
    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP, dmac_btcoex_a2dp_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_A2DP fail!}");
        return OAL_FAIL;
    }
    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER, dmac_bt_transfer_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_TRANSFER fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        return OAL_FAIL;
    }
    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN, dmac_btcoex_page_scan_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_PAGE_SCAN fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
        return OAL_FAIL;
    }
    if (OAL_SUCC != dmac_btcoex_register_dmac_misc_event(HAL_EVENT_DMAC_BT_SCO, dmac_btcoex_sco_status_handler))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_register_dmac_misc_event HAL_EVENT_DMAC_BT_SCO fail!}");
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
        dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN);
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_exit(oal_void)
{
    /* init coexit wifi bt param */
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_A2DP);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_TRANSFER);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_PAGE_SCAN);
    dmac_btcoex_unregister_dmac_misc_event(HAL_EVENT_DMAC_BT_SCO);
    return OAL_SUCC;
}


oal_void dmac_btcoex_wlan_priority_set(mac_vap_stru *pst_mac_vap, oal_uint8 uc_value, oal_uint8 uc_timeout_ms)
{
    dmac_vap_stru *pst_dmac_vap;
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied;
    oal_uint32 ul_now_ms;
    oal_uint8 uc_set = OAL_FALSE;

    pst_dmac_vap = (dmac_vap_stru *)pst_mac_vap;
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    /* 需要拉高prio */
    if (OAL_TRUE == uc_value)
    {
        /* 当前没有拉高prio并且可以拉高prio */
        if (OAL_FALSE == pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state)
        {

            /* 需要与上一次拉低时间间隔至少100ms */
            ul_now_ms = (oal_uint32)OAL_TIME_GET_STAMP_MS();
            if (ul_now_ms - pst_dmac_vap_btcoex_occupied->ul_timestamp > 50)
            {
                /* 启动priority定时器 */
                FRW_TIMER_CREATE_TIMER(&pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer,
                                           dmac_btcoex_wlan_priority_timeout_callback,
                                           uc_timeout_ms,
                                           (oal_void *)pst_dmac_vap,
                                           OAL_FALSE,
                                           OAM_MODULE_ID_DMAC,
                                           0);
                pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state = OAL_TRUE;

                uc_set = OAL_TRUE;
            }
            else
            {
            }
        }
    }
    /* 需要拉低prio */
    else
    {
        if (OAL_TRUE == pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state)
        {
            pst_dmac_vap_btcoex_occupied->uc_prio_occupied_state = OAL_FALSE;

            pst_dmac_vap_btcoex_occupied->ul_timestamp = (oal_uint32)OAL_TIME_GET_STAMP_MS();
        }
        uc_set = OAL_TRUE;
    }

    if (OAL_TRUE == uc_set)
    {
        /* uc_value == 1, 软件配置高优先级；uc_value == 0, 返回给硬件配置 */
        hal_set_btcoex_hw_rx_priority_dis((oal_uint8)(!uc_value));

        /* 设置软件配置优先级 */
        hal_set_btcoex_hw_priority_en((oal_uint8)(!uc_value));

        /* 软件设置优先级 */
        hal_set_btcoex_sw_priority_flag(uc_value);
    }

}


oal_uint32 dmac_btcoex_wlan_priority_timeout_callback(oal_void *p_arg)
{
    mac_vap_stru *pst_mac_vap = (mac_vap_stru *)p_arg;

    dmac_btcoex_wlan_priority_set(pst_mac_vap, 0, 0);
    return OAL_SUCC;
}


oal_void dmac_btcoex_change_state_syn(mac_vap_stru *pst_mac_vap)
{
    switch (pst_mac_vap->en_vap_state)
    {
    case MAC_VAP_STATE_STA_WAIT_SCAN:
    case MAC_VAP_STATE_STA_WAIT_AUTH_SEQ2:
    case MAC_VAP_STATE_STA_WAIT_AUTH_SEQ4:
    case MAC_VAP_STATE_STA_AUTH_COMP:
    case MAC_VAP_STATE_STA_WAIT_ASOC:
    case MAC_VAP_STATE_STA_OBSS_SCAN:
    case MAC_VAP_STATE_STA_BG_SCAN:
        hal_set_btcoex_hw_rx_priority_dis(0);
        break;
    default:
        hal_set_btcoex_hw_rx_priority_dis(1);
    }
}


oal_void dmac_btcoex_delba_trigger(mac_vap_stru *pst_mac_vap, oal_uint8 uc_need_delba,oal_uint8 uc_ba_size)
{
    dmac_to_hmac_btcoex_rx_delba_trigger_event_stru  st_dmac_to_hmac_btcoex_rx_delba;

    st_dmac_to_hmac_btcoex_rx_delba.uc_need_delba = uc_need_delba;
    st_dmac_to_hmac_btcoex_rx_delba.us_ba_size = uc_ba_size;
    st_dmac_to_hmac_btcoex_rx_delba.us_user_id = pst_mac_vap->uc_assoc_vap_id;
    dmac_send_sys_event(pst_mac_vap, WLAN_CFGID_BTCOEX_RX_DELBA_TRIGGER, OAL_SIZEOF(dmac_to_hmac_btcoex_rx_delba_trigger_event_stru), (oal_uint8 *)&st_dmac_to_hmac_btcoex_rx_delba);
}


oal_void dmac_btcoex_vap_up_handle(mac_vap_stru *pst_mac_vap)
{
    oal_uint8 uc_chan_idx;
    mac_device_stru *pst_mac_device;
    hal_to_dmac_device_stru *pst_hal_device;
    bt_status_stru *pst_bt_status;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_btcoex_vap_up_handle::pst_mac_device null.}");
        return;
    }

    pst_hal_device = pst_mac_device->pst_device_stru;

    if (OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_vap_up_handle::pst_hal_device null.}");
        return;
    }

    hal_set_btcoex_soc_gpreg0(pst_mac_vap->st_channel.en_band, BIT0, 0);   // 频段
    mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number, &uc_chan_idx);
    hal_set_btcoex_soc_gpreg0(uc_chan_idx, BIT5 | BIT4 | BIT3 | BIT2 | BIT1, 1);   // 信道
    hal_set_btcoex_soc_gpreg0(pst_mac_vap->st_channel.en_bandwidth, BIT8 | BIT7 | BIT6, 6);   // 带宽
    hal_set_btcoex_soc_gpreg0(OAL_TRUE, BIT13, 13);
    hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);

    hal_update_btcoex_btble_status(pst_hal_device);

    pst_bt_status = &(pst_hal_device->st_btcoex_btble_status.un_bt_status.st_bt_status);

    if (pst_bt_status->bit_bt_on)
    {
        hal_set_btcoex_sw_all_abort_ctrl(OAL_TRUE);
    }

    /* 状态上报BT: 信道，带宽，频率等*/
    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_CFG,
                 "{dmac_btcoex_vap_up_handle::VAP state->TRUE, channel state: band->%d, channel->%d, bandwidth->%d, bit status[%d]",
                 pst_mac_vap->st_channel.en_band,
                 pst_mac_vap->st_channel.uc_chan_number,
                 pst_mac_vap->st_channel.en_bandwidth,
                 pst_bt_status->bit_bt_on);
}


oal_void dmac_btcoex_vap_down_handle(mac_vap_stru *pst_mac_vap)
{
    mac_device_stru *pst_mac_device;

    pst_mac_device = mac_res_get_dev(pst_mac_vap->uc_device_id);

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_btcoex_vap_down_handle::pst_mac_device null.}");

        return;
    }

    if (0 == mac_device_calc_up_vap_num(pst_mac_device))
    {
        /* 状态上报BT */
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_CFG, "{dmac_btcoex_vap_down_handle::Notify BT cancel AFH.}");
        hal_set_btcoex_soc_gpreg1(OAL_FALSE, BIT3, 3);
        hal_set_btcoex_soc_gpreg0(OAL_FALSE, BIT13, 13);
        hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);
    }
}


oal_void dmac_btcoex_update_rx_rate_threshold (mac_vap_stru *pst_mac_vap, dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba)
{
    oal_uint8 uc_band;
    oal_uint8 uc_bandwidth;

    uc_band = pst_mac_vap->st_channel.en_band;
    mac_vap_get_bandwidth_cap(pst_mac_vap, &uc_bandwidth);

    if ((uc_band >= WLAN_BAND_BUTT) || (uc_bandwidth >= WLAN_BW_CAP_BUTT))
    {
        OAM_ERROR_LOG2(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_rx_rate_threshold::band %d, bandwidth %d exceed scale!}",
                         uc_band, uc_bandwidth);
        return;
    }
    pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min = g_aus_btcoex_rate_thresholds[uc_band][uc_bandwidth][BTCOEX_RATE_THRESHOLD_MIN];
    pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max = g_aus_btcoex_rate_thresholds[uc_band][uc_bandwidth][BTCOEX_RATE_THRESHOLD_MAX];

    OAM_WARNING_LOG4(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_update_rx_rate_threshold:: min: %d, max: %d, band %d, bandwidth %d.}",
                        pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min, pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max, uc_band, uc_bandwidth);

}


oal_uint32 dmac_config_btcoex_assoc_state_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user)
{
    dmac_vap_stru                *pst_dmac_vap;
    dmac_user_stru               *pst_dmac_user;
    dmac_vap_btcoex_stru         *pst_dmac_vap_btcoex;
    mac_device_stru              *pst_mac_device;
    hal_to_dmac_device_stru      *pst_hal_device;
    dmac_user_btcoex_delba_stru  *pst_dmac_user_btcoex_delba;
    hal_btcoex_btble_status_stru *pst_btcoex_btble_status;
    oal_uint8 uc_chan_idx;
    oal_uint8 uc_need_delba;
    ble_status_stru *pst_ble_status;
    bt_status_stru *pst_bt_status;
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    config_device_freq_h2d_stru   st_device_freq;
#endif
#endif

    hal_set_btcoex_soc_gpreg0(pst_mac_vap->st_channel.en_band, BIT0, 0);    // 频段
    mac_get_channel_idx_from_num(pst_mac_vap->st_channel.en_band, pst_mac_vap->st_channel.uc_chan_number, &uc_chan_idx);
    hal_set_btcoex_soc_gpreg0(uc_chan_idx, BIT5 | BIT4 | BIT3 | BIT2 | BIT1, 1);    // 信道
    hal_set_btcoex_soc_gpreg0(pst_mac_vap->st_channel.en_bandwidth, BIT8 | BIT7 | BIT6, 6); // 带宽
    hal_set_btcoex_soc_gpreg0(OAL_TRUE, BIT13, 13);
    hal_set_btcoex_soc_gpreg1(OAL_FALSE, BIT2, 2);

    pst_dmac_vap = (dmac_vap_stru *)pst_mac_vap;

    pst_dmac_vap_btcoex = &(pst_dmac_vap->st_dmac_vap_btcoex);

    pst_dmac_vap_btcoex->en_all_abort_preempt_type = BTCOEX_NOFRAME;
    dmac_btcoex_init_preempt(pst_mac_vap, pst_mac_user, pst_dmac_vap_btcoex->en_all_abort_preempt_type);

    if (OAL_FALSE == dmac_btcoex_check_legacy_sta(pst_mac_vap))
    {
        if (WLAN_LEGACY_VAP_MODE != pst_mac_vap->en_p2p_mode)
        {
            /* Notify Bt the P2P Connected state */
            hal_set_btcoex_soc_gpreg0(OAL_TRUE, BIT15, 15);
        }
        else
        {
            /* Notify Bt the AP-Mode connect */
            hal_set_btcoex_soc_gpreg1(OAL_TRUE, BIT3, 3);
        }
        hal_coex_sw_irq_set(BIT5);
        OAM_WARNING_LOG3(0, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::ba process skip! vap mode is %d, p2p mode is %d, vap_state: %d.}",
            pst_mac_vap->en_vap_mode, pst_mac_vap->en_p2p_mode, pst_mac_vap->en_vap_state);
        return OAL_SUCC;
    }

    /* 以上AP STA模式都适用，下面仅对STA */

    /* Notify Bt the Sta-Mode connected state */
    hal_set_btcoex_soc_gpreg1(OAL_TRUE, BIT3, 3);
    hal_coex_sw_irq_set(BIT5);

    pst_dmac_user = (dmac_user_stru *)pst_mac_user;

    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    dmac_btcoex_update_rx_rate_threshold(pst_mac_vap, pst_dmac_user_btcoex_delba);

    pst_mac_device = mac_res_get_dev((oal_uint32)pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::pst_mac_device is null.}");
        return OAL_FAIL;
    }

    pst_hal_device = pst_mac_device->pst_device_stru;

    pst_btcoex_btble_status = &(pst_hal_device->st_btcoex_btble_status);
#ifdef _PRE_WLAN_FEATURE_SMARTANT
    pst_hal_device->st_dual_antenna_check_status.bit_bt_on = pst_btcoex_btble_status->un_bt_status.st_bt_status.bit_bt_on;
#endif
    pst_ble_status = &(pst_btcoex_btble_status->un_ble_status.st_ble_status);
    pst_bt_status = &(pst_btcoex_btble_status->un_bt_status.st_bt_status);

    uc_need_delba = OAL_FALSE;
    pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;
    pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = BTCOEX_RX_WINDOW_SIZE_INDEX_3;

    if (0 == pst_ble_status->bit_bt_transfer && 0 == pst_ble_status->bit_bt_ba)
    {
        dmac_btcoex_delba_trigger(pst_mac_vap, OAL_FALSE, 0);
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::bt not working, ba size to default.}");
        return OAL_SUCC;
    }

    /* 电话场景下进行速率统计，周期目前为0.5秒 */
    if (pst_bt_status->bit_bt_sco)
    {
#ifdef _PRE_WLAN_FEATURE_AUTO_FREQ
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        /* 关闭自动调频 */
        st_device_freq.uc_set_type = FREQ_SET_MODE;
        st_device_freq.uc_device_freq_enable = OAL_FALSE;
        dmac_config_set_device_freq(pst_mac_vap, 0, (oal_uint8 *)&st_device_freq);
#endif
#endif

        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_1;
        pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.uc_sco_rx_rate_statistics_flag = OAL_TRUE;
        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_sco_statistics_timer),
                                   dmac_btcoex_sco_rx_rate_statistics_flag_callback,
                                   BTCOEX_SCO_CALCULATE_TIME,
                                   (void *)pst_mac_vap,
                                   OAL_TRUE,
                                   OAM_MODULE_ID_DMAC,
                                   pst_mac_device->ul_core_id);
    }
    else if (pst_bt_status->bit_bt_a2dp)
    {
        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
        pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.uc_rx_rate_statistics_flag = OAL_TRUE;
        pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.uc_rx_rate_statistics_timeout = OAL_FALSE;
        FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex->st_dmac_vap_btcoex_rx_statistics.bt_coex_statistics_timer),
                                   dmac_btcoex_rx_rate_statistics_flag_callback,
                                   BTCOEX_RX_STATISTICS_TIME,
                                   (void *)pst_mac_vap,
                                   OAL_TRUE,
                                   OAM_MODULE_ID_DMAC,
                                   pst_mac_device->ul_core_id);
    }
    else if (pst_ble_status->bit_bt_transfer)
    {
        dmac_alg_cfg_btcoex_state_notify(pst_mac_device, BT_TRANSFER_ON);

        pst_dmac_user_btcoex_delba->uc_ba_size_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
        pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = BTCOEX_RX_WINDOW_SIZE_INDEX_2;
    }

    dmac_btcoex_update_ba_size(pst_mac_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);
    OAM_WARNING_LOG4(0, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::bt ba status:%d, transfer status:%d, bar_size:%d, uc_need_delba:%d}",
                        pst_ble_status->bit_bt_ba,
                        pst_ble_status->bit_bt_transfer,
                        pst_dmac_user_btcoex_delba->uc_ba_size,
                        uc_need_delba);
    dmac_btcoex_delba_trigger(pst_mac_vap, uc_need_delba, pst_dmac_user_btcoex_delba->uc_ba_size);

    return OAL_SUCC;
}


oal_uint32 dmac_config_btcoex_disassoc_state_syn(mac_vap_stru *pst_mac_vap)
{
    dmac_vap_stru *pst_dmac_vap;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_vap_btcoex_occupied_stru *pst_dmac_vap_btcoex_occupied;

    if (OAL_FALSE == dmac_btcoex_check_legacy_sta(pst_mac_vap))
    {
        if (WLAN_LEGACY_VAP_MODE != pst_mac_vap->en_p2p_mode)
        {
            /* Notify Bt the P2P Disconnected state */
            hal_set_btcoex_soc_gpreg0(OAL_FALSE, BIT15, 15);
        }
        else
        {
            /* Notify Bt the AP-Mode vap disconnected */
            hal_set_btcoex_soc_gpreg1(OAL_FALSE, BIT3, 3);
        }
        hal_coex_sw_irq_set(BIT5);
        OAM_WARNING_LOG2(0, OAM_SF_COEX, "{dmac_config_btcoex_disassoc_state_syn::ba process skip! vap mode is %d, p2p mode is %d.}",
                pst_mac_vap->en_vap_mode, pst_mac_vap->en_p2p_mode);
        return OAL_SUCC;
    }
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_mac_vap->uc_vap_id);
    if (OAL_PTR_NULL == pst_dmac_vap)
    {
        OAM_ERROR_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_COEX, "{dmac_config_btcoex_assoc_state_syn::pst_dmac_vap is null.}");
        return OAL_FAIL;
    }
    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_vap_btcoex_occupied = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_occupied);

    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_low_rate_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_sco_statistics_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_occupied->bt_coex_occupied_timer));
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_dmac_vap_btcoex_occupied->bt_coex_priority_timer));

    /* Notify Bt the Station-VAP disconnected */
    hal_set_btcoex_soc_gpreg1(OAL_FALSE, BIT3, 3);
    hal_coex_sw_irq_set(BIT5);

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_void dmac_btcoex_rx_average_rate_calculate (dmac_user_btcoex_rx_info_stru *pst_btcoex_wifi_rx_rate_info,
                                                                                oal_uint32 *pul_rx_rate, oal_uint16 *pus_rx_count)
{
    if (0 != pst_btcoex_wifi_rx_rate_info->us_rx_rate_stat_count)
    {
        pst_btcoex_wifi_rx_rate_info->ull_rx_rate_mbps /= 1000;
        *pus_rx_count = pst_btcoex_wifi_rx_rate_info->us_rx_rate_stat_count;
        *pul_rx_rate = (oal_uint32)(pst_btcoex_wifi_rx_rate_info->ull_rx_rate_mbps / pst_btcoex_wifi_rx_rate_info->us_rx_rate_stat_count);
    }
    else
    {
        pst_btcoex_wifi_rx_rate_info->ull_rx_rate_mbps = 0;
        *pus_rx_count = 0;
        return;
    }
    OAL_MEMZERO(pst_btcoex_wifi_rx_rate_info, OAL_SIZEOF(dmac_user_btcoex_rx_info_stru));
}


OAL_STATIC oal_uint32 dmac_btcoex_low_rate_callback(oal_void *p_arg)
{
    mac_vap_stru *pst_vap = (mac_vap_stru *)p_arg;
    mac_device_stru *pst_mac_device;
    dmac_device_stru *pst_dmac_device;
    hal_to_dmac_device_stru *pst_hal_device;
    hal_btcoex_btble_status_stru *pst_btble_status;
    dmac_vap_stru *pst_dmac_vap = (dmac_vap_stru *)p_arg;
    dmac_user_stru *pst_dmac_user;
    dmac_user_btcoex_delba_stru *pst_dmac_user_btcoex_delba;
    oal_uint32 ul_rx_rate = 0;
    oal_uint16 us_rx_count = 0;
    oal_uint32 ul_rate_threshold_min;
    oal_uint32 ul_rate_threshold_max;
    dmac_user_btcoex_rx_info_stru *pst_btcoex_wifi_rx_rate_info;

    pst_mac_device = mac_res_get_dev(0);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DBAC, "{dmac_btcoex_low_rate_callback::pst_device[id:0] is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_ERROR_LOG0(0, OAM_SF_DBAC, "{dmac_btcoex_low_rate_callback::pst_dmac_user is NULL}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_dmac_device = dmac_res_get_mac_dev(pst_dmac_user->st_user_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_low_rate_callback::pst_dmac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_hal_device = pst_mac_device->pst_device_stru;
    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);
    pst_btble_status = &(pst_hal_device->st_btcoex_btble_status);
    ul_rate_threshold_min = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min;
    ul_rate_threshold_max = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max;
    pst_btcoex_wifi_rx_rate_info = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_rx_info);

    if (OAL_FALSE == pst_dmac_user_btcoex_delba->uc_get_addba_req_flag)
    {
        dmac_btcoex_rx_average_rate_calculate(pst_btcoex_wifi_rx_rate_info, &ul_rx_rate, &us_rx_count);

        if (pst_dmac_device->st_dmac_alg_stat.en_dmac_device_distance_enum < DMAC_ALG_TPC_FAR_DISTANCE)
        {
            if (((BTCOEX_RX_WINDOW_SIZE_DECREASE == pst_dmac_user_btcoex_delba->uc_ba_size_tendence)
                    && (ul_rx_rate < pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min))
                || ((BTCOEX_RX_WINDOW_SIZE_INCREASE == pst_dmac_user_btcoex_delba->uc_ba_size_tendence)
                    && (ul_rx_rate > pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max))
                || ((BTCOEX_RX_WINDOW_SIZE_DECREASE == pst_dmac_user_btcoex_delba->uc_ba_size_tendence)
                    && (BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index)
                    && (ul_rx_rate < (ul_rate_threshold_min + (ul_rate_threshold_max >> 1)))))
            {
                pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = pst_dmac_user_btcoex_delba->uc_ba_size_index;
                dmac_btcoex_update_ba_size(pst_vap, pst_dmac_user_btcoex_delba, pst_btble_status);
                dmac_btcoex_delba_trigger(pst_vap, OAL_TRUE, pst_dmac_user_btcoex_delba->uc_ba_size);
                OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_low_rate_callback::ba_size change to: %d, rate: %d.}", pst_dmac_user_btcoex_delba->uc_ba_size, ul_rx_rate);
            }
            else
            {
                /* 保持现有聚合不变 */
                pst_dmac_user_btcoex_delba->uc_ba_size_index = pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index;
                dmac_btcoex_update_ba_size(pst_vap, pst_dmac_user_btcoex_delba, pst_btble_status);
                OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_low_rate_callback::ba_size dont change: %d, rate: %d.}", pst_dmac_user_btcoex_delba->uc_ba_size, ul_rx_rate);
            }
        }
    }
    else
    {
        pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index = pst_dmac_user_btcoex_delba->uc_ba_size_index;
        OAM_WARNING_LOG0(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_low_rate_callback::receive addba req before delba.}");
    }

    FRW_TIMER_RESTART_TIMER(&(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics.bt_coex_statistics_timer),
                                BTCOEX_RX_STATISTICS_TIME, OAL_TRUE);

    return OAL_SUCC;
}


oal_void dmac_btcoex_bt_low_rate_process (mac_vap_stru *pst_vap,
                                                hal_to_dmac_device_stru *pst_hal_device)
{
    oal_uint32                          ul_rx_rate = 0;
    oal_uint16                          us_rx_count = 0;
    hal_btcoex_btble_status_stru       *pst_btcoex_btble_status;
    dmac_vap_stru                      *pst_dmac_vap;
    bt_status_stru                     *pst_bt_status;
    oal_uint32                          ul_rate_threshold_min;
    oal_uint32                          ul_rate_threshold_max;
    dmac_user_stru                     *pst_dmac_user;
    dmac_device_stru                   *pst_dmac_device;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru      *pst_dmac_user_btcoex_rx_info;
    dmac_user_btcoex_delba_stru        *pst_dmac_user_btcoex_delba;

    if (OAL_FALSE == dmac_btcoex_check_legacy_sta(pst_vap))
    {
        return;
    }
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_bt_low_rate_process::pst_dmac_vap null.}");
        return;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_bt_low_rate_process::pst_dmac_user null.}");
        return;
    }

    pst_dmac_device = dmac_res_get_mac_dev(pst_dmac_user->st_user_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_device))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_bt_low_rate_process::pst_dmac_device null.}");
        return;
    }


    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_rx_info = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_rx_info);
    pst_dmac_user_btcoex_delba = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_delba);

    /* 累加接收速率 */
    if (pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_flag)
    {
        if (0 != pst_hal_device->ul_rx_rate)
        {
            pst_dmac_user_btcoex_rx_info->ull_rx_rate_mbps += pst_hal_device->ul_rx_rate;
            pst_dmac_user_btcoex_rx_info->us_rx_rate_stat_count++;
        }
    }
    else
    {
        return;
    }

    /* 计算接收速率 */
    if (pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_timeout)
    {
        pst_dmac_vap_btcoex_rx_statistics->uc_rx_rate_statistics_timeout = OAL_FALSE;

        dmac_btcoex_rx_average_rate_calculate(pst_dmac_user_btcoex_rx_info, &ul_rx_rate, &us_rx_count);

        if (g_rx_statistics_print)
        {
            OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_bt_low_rate_process::rate: %d, coute: %d.}", ul_rx_rate, us_rx_count);
        }
    }
    else
    {
        return;
    }

    /* 处理近场速率 */
    if (pst_dmac_device->st_dmac_alg_stat.en_dmac_device_distance_enum < DMAC_ALG_TPC_FAR_DISTANCE)
    {
        pst_btcoex_btble_status = &(pst_hal_device->st_btcoex_btble_status);
        pst_bt_status = &(pst_btcoex_btble_status->un_bt_status.st_bt_status);
        /* 电话固定聚合 */
        if (pst_bt_status->bit_bt_sco)
        {
            return;
        }
        pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_HOLD;
        pst_dmac_user_btcoex_delba->uc_ba_size_index = pst_dmac_user_btcoex_delba->uc_ba_size_addba_rsp_index;
        ul_rate_threshold_min = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_min;
        ul_rate_threshold_max = pst_dmac_user_btcoex_delba->ul_rx_rate_threshold_max;

        /* 5G80M音乐固定2档 */
        if ((WLAN_BAND_5G == pst_vap->st_channel.en_band) 
            && (pst_vap->st_channel.en_bandwidth > WLAN_BAND_WIDTH_40MINUS)
            && (pst_bt_status->bit_bt_a2dp))
        {
            if ((BTCOEX_RX_WINDOW_SIZE_INDEX_3 == pst_dmac_user_btcoex_delba->uc_ba_size_index)
                    && (ul_rx_rate < (ul_rate_threshold_min + (ul_rate_threshold_max >> 1))))
            {
                pst_dmac_user_btcoex_delba->uc_ba_size_index--;
                pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
            }
            else if (pst_dmac_user_btcoex_delba->uc_ba_size_index < BTCOEX_RX_WINDOW_SIZE_INDEX_2)
            {
                pst_dmac_user_btcoex_delba->uc_ba_size_index++;
                pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
            }
        }
        else
        {
            switch (pst_dmac_user_btcoex_delba->uc_ba_size_index)
            {
                case BTCOEX_RX_WINDOW_SIZE_INDEX_3:
                    if (ul_rx_rate < (ul_rate_threshold_min + (ul_rate_threshold_max >> 1)))
                    {
                        pst_dmac_user_btcoex_delba->uc_ba_size_index--;
                        pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                    }
                    break;
                case BTCOEX_RX_WINDOW_SIZE_INDEX_2:
                    if (ul_rx_rate < ul_rate_threshold_min)
                    {
                        pst_dmac_user_btcoex_delba->uc_ba_size_index--;
                        pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                    }
                    break;
                case BTCOEX_RX_WINDOW_SIZE_INDEX_1:
                    if (ul_rx_rate < ul_rate_threshold_min)
                    {
                        pst_dmac_user_btcoex_delba->uc_ba_size_index--;
                        pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_DECREASE;
                    }
                    else if (ul_rx_rate > ul_rate_threshold_max)
                    {
                        pst_dmac_user_btcoex_delba->uc_ba_size_index++;
                        pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
                    }
                    break;
                case BTCOEX_RX_WINDOW_SIZE_INDEX_0:
                    if (ul_rx_rate > ul_rate_threshold_max)
                    {
                        pst_dmac_user_btcoex_delba->uc_ba_size_index++;
                        pst_dmac_user_btcoex_delba->uc_ba_size_tendence = BTCOEX_RX_WINDOW_SIZE_INCREASE;
                    }
                    break;
                default:
                    OAM_ERROR_LOG1(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_bt_low_rate_process::ba_size_index error: %d.}",
                                pst_dmac_user_btcoex_delba->uc_ba_size_index);
                    break;
            }
        }

        if (BTCOEX_RX_WINDOW_SIZE_HOLD != pst_dmac_user_btcoex_delba->uc_ba_size_tendence)
        {
            dmac_btcoex_update_ba_size(pst_vap, pst_dmac_user_btcoex_delba, pst_btcoex_btble_status);
            dmac_btcoex_delba_trigger(pst_vap, OAL_FALSE, pst_dmac_user_btcoex_delba->uc_ba_size);

            OAM_WARNING_LOG3(pst_vap->uc_vap_id, OAM_SF_COEX, "{dmac_btcoex_bt_low_rate_process::ba_size will change to: %d in 5s, rate: %d, count: %d.}",
                                pst_dmac_user_btcoex_delba->uc_ba_size, ul_rx_rate, us_rx_count);

            pst_dmac_user_btcoex_delba->uc_get_addba_req_flag = OAL_FALSE;

            FRW_TIMER_STOP_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_statistics_timer));
            FRW_TIMER_CREATE_TIMER(&(pst_dmac_vap_btcoex_rx_statistics->bt_coex_low_rate_timer),
                                           dmac_btcoex_low_rate_callback,
                                           BTCOEX_RX_LOW_RATE_TIME,
                                           (oal_void *)pst_vap,
                                           OAL_FALSE,
                                           OAM_MODULE_ID_DMAC,
                                           0);
        }
    }
}


oal_void dmac_btcoex_release_rx_prot(mac_vap_stru *pst_mac_vap, oal_uint8 uc_data_type)
{
    mac_device_stru *pst_mac_device;
    bt_status_stru              *pst_bt_status;

    pst_mac_device = mac_res_get_dev(0);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_release_rx_prot::pst_mac_devicenull.}");
        return;
    }
    pst_bt_status = &(pst_mac_device->pst_device_stru->st_btcoex_btble_status.un_bt_status.st_bt_status);
    if (!pst_bt_status->bit_bt_on || (uc_data_type == MAC_DATA_BUTT))
    {
        return;
    }
    /* 收到EAPOL帧以后，刷掉occupied_period以保证BT竞争到 */
    if ((uc_data_type == MAC_DATA_EAPOL) && !(pst_bt_status->bit_bt_sco))
    {
        hal_set_btcoex_occupied_period(0);
    }
    else if ((MAC_DATA_EAPOL == uc_data_type)
          || (MAC_DATA_DHCP == uc_data_type)
          || (MAC_DATA_ARP_RSP == uc_data_type))
    {
        dmac_btcoex_wlan_priority_set(pst_mac_vap, 0, 0);
    }
}


oal_void dmac_btcoex_tx_vip_frame(hal_to_dmac_device_stru *pst_hal_device, mac_vap_stru *pst_mac_vap, oal_dlist_head_stru *pst_tx_dscr_list_hdr)
{
    oal_dlist_head_stru    *pst_dscr_entry;
    hal_tx_dscr_stru       *pst_dscr_temp;
    oal_uint8               uc_data_type;
    oal_uint8               uc_coex_data_type;    // 通知BT发送数据类型,寄存器只有2bit
    bt_status_stru         *pst_bt_status;

    /* EAPOL帧，DHCP和ARP帧的共存保护 */
    pst_bt_status = &(pst_hal_device->st_btcoex_btble_status.un_bt_status.st_bt_status);
    if (!pst_bt_status->bit_bt_on
#ifdef _PRE_WLAN_FEATURE_LTECOEX
        && !pst_hal_device->ul_lte_coex_status
#endif
        )
    {
        return;
    }

    OAL_DLIST_SEARCH_FOR_EACH(pst_dscr_entry, pst_tx_dscr_list_hdr)
    {
        pst_dscr_temp = (hal_tx_dscr_stru *)OAL_DLIST_GET_ENTRY(pst_dscr_entry, hal_tx_dscr_stru, st_entry);
        uc_data_type = mac_get_data_type(pst_dscr_temp->pst_skb_start_addr);
        if (uc_data_type > MAC_DATA_EAPOL)
        {
            continue;
        }
        switch (uc_data_type)
        {
            case MAC_DATA_DHCP:
                uc_coex_data_type = 1;
                dmac_btcoex_wlan_priority_set(pst_mac_vap, 1, BTCOEX_PRIO_TIMEOUT_100MS);
                break;

            case MAC_DATA_ARP_REQ:
            case MAC_DATA_ARP_RSP:
                uc_coex_data_type = 2;
                dmac_btcoex_wlan_priority_set(pst_mac_vap, 1, BTCOEX_PRIO_TIMEOUT_60MS);
                break;

            case MAC_DATA_EAPOL:
                uc_coex_data_type = 3;
                /* 为提高p2p配对成功率，在非BT电话场景下，保护EAPOL帧收发 */
                if (!(pst_bt_status->bit_bt_sco))
                {
                    hal_set_btcoex_occupied_period(60000);//60ms
                }
                else
                {
                    /* 软件拉高priority */
                    dmac_btcoex_wlan_priority_set(pst_mac_vap, 1, BTCOEX_PRIO_TIMEOUT_100MS);
                }
                break;

            default:
                uc_coex_data_type = 0;
        }
        hal_set_btcoex_soc_gpreg0(uc_coex_data_type, BIT10 | BIT11 | BIT12, 10);   // 发送关键帧
        hal_coex_sw_irq_set(HAL_COEX_SW_IRQ_BT);
    }
}


oal_void dmac_btcoex_sco_rx_rate_process (mac_vap_stru *pst_vap,
                                                hal_to_dmac_device_stru *pst_hal_device)
{
    dmac_vap_stru *pst_dmac_vap;
    dmac_user_stru *pst_dmac_user;
    dmac_vap_btcoex_rx_statistics_stru *pst_dmac_vap_btcoex_rx_statistics;
    dmac_user_btcoex_rx_info_stru *pst_dmac_user_btcoex_sco_rx_info;


    if (OAL_FALSE == dmac_btcoex_check_legacy_sta(pst_vap))
    {
        return;
    }
    pst_dmac_vap = (dmac_vap_stru *)mac_res_get_dmac_vap(pst_vap->uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_process::pst_dmac_vap null.}");
        return;
    }

    pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_vap->uc_assoc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_dmac_user))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_sco_rx_rate_process::pst_dmac_user null.}");
        return;
    }
    pst_dmac_vap_btcoex_rx_statistics = &(pst_dmac_vap->st_dmac_vap_btcoex.st_dmac_vap_btcoex_rx_statistics);
    pst_dmac_user_btcoex_sco_rx_info = &(pst_dmac_user->st_dmac_user_btcoex_stru.st_dmac_user_btcoex_sco_rx_info);

    /* 累加接收速率 */
    if (pst_dmac_vap_btcoex_rx_statistics->uc_sco_rx_rate_statistics_flag)
    {
        if (0 != pst_hal_device->ul_rx_rate)
        {
            pst_dmac_user_btcoex_sco_rx_info->ull_rx_rate_mbps += pst_hal_device->ul_rx_rate;
            pst_dmac_user_btcoex_sco_rx_info->us_rx_rate_stat_count++;
        }
    }
    else
    {
        return;
    }

}


oal_void  dmac_btcoex_resume_wifi(mac_device_stru *pst_mac_device)
{
    oal_uint8 uc_up_vap_num = 0;
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_btcoex_resume_wifi:pst_mac_device is null");
        return;
    }
    uc_up_vap_num = mac_device_calc_up_vap_num(pst_mac_device);

    if (0 == uc_up_vap_num)
    {
        return;
    }
    if (mac_is_dbac_running(pst_mac_device))
    {
        /* dbac场景只需恢复dbac，由dbac自行切到工作信道 */
        dmac_alg_dbac_resume(pst_mac_device);
        return;
    }
    /* 将扫描虚假队列里的帧恢复到硬件队列中去,非DBAC场景下硬件队列的帧是被保持到扫描队列中去的 */
    dmac_tx_restore_tx_queue(pst_mac_device->pst_device_stru, HAL_TX_FAKE_QUEUE_BGSCAN_ID);

    /* 恢复home信道上被暂停的发送 */
    dmac_vap_resume_tx_by_chl(pst_mac_device, &(pst_mac_device->st_home_channel));
}


oal_uint32 dmac_btcoex_suspend_wifi(mac_device_stru *pst_mac_device)
{
    oal_uint8                    uc_up_vap_num = 0;
    mac_vap_stru                *pst_mac_vap1  = OAL_PTR_NULL;
    mac_vap_stru                *pst_mac_vap2  = OAL_PTR_NULL;

    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_btcoex_suspend_wifi:pst_mac_device is null");
        return OAL_ERR_CODE_PTR_NULL;
    }

    uc_up_vap_num = mac_device_calc_up_vap_num(pst_mac_device);

    if (0 == uc_up_vap_num)
    {
        /* 没有work的vap，表示现在处于扫描状态，前面已经被扫描abort了，都处于idle状态，可以直接做切换，不需要vap pause操作 */
        OAM_WARNING_LOG0(0,OAM_SF_COEX,"dmac_btcoex_suspend_wifi:vap num is 0");
    }
    else if (1 == uc_up_vap_num)
    {
        mac_device_find_up_vap(pst_mac_device,&pst_mac_vap1);
        if (OAL_PTR_NULL == pst_mac_vap1)
        {
            OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_btcoex_stop_wifi:pst_mac_vap1 is null");
            return OAL_ERR_CODE_PTR_NULL;
        }
        dmac_switch_same_channel_off(pst_mac_device,pst_mac_vap1,pst_mac_device->st_scan_params.us_scan_time);
    }
    else if (2 == uc_up_vap_num)
    {
        mac_device_find_2up_vap(pst_mac_device,&pst_mac_vap1,&pst_mac_vap2);
        if ((OAL_PTR_NULL == pst_mac_vap1) || (OAL_PTR_NULL == pst_mac_vap2))
        {
            OAM_ERROR_LOG2(0, OAM_SF_COEX, "dmac_btcoex_stop_wifi:pst_mac_vap1=%x,pst_mac_vap2=%x",pst_mac_vap1,pst_mac_vap2);
            return OAL_ERR_CODE_PTR_NULL;
        }
        dmac_switch_channel_off_enhanced_self_channel(pst_mac_device,pst_mac_vap1,pst_mac_vap2,pst_mac_device->st_scan_params.us_scan_time);
    }
    else
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_stop_wifi: cannot support 3 and more vaps!}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


oal_uint32 dmac_btcoex_pow_save_callback(oal_void *p_arg)
{
    hal_to_dmac_device_stru   *pst_h2d_device = OAL_PTR_NULL;
    mac_device_stru           *pst_mac_device = OAL_PTR_NULL;

    if(OAL_PTR_NULL == p_arg)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_btcoex_pow_save_callback:p_arg is null");
        return OAL_ERR_CODE_PTR_NULL;
    }
    pst_h2d_device = (hal_to_dmac_device_stru *)p_arg;
    pst_mac_device = mac_res_get_dev(pst_h2d_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::pst_mac_device null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 在scan状态时候，扫描不进入低功耗，此时不用担心低功耗，如果扫描来时 */
    if (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state)
    {
        switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
        {
            case HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN:
                 OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::scan begin,scan_mode=%d.}",pst_mac_device->st_scan_params.en_scan_mode);
                /* 如果扫描一开始被ps打断，此时ps来恢复继续扫描 */
                dmac_scan_begin(pst_mac_device);
                break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_WAIT:
                /* 如果扫描临时回home channel继续工作被打断，此时ps来恢复继续回home channel工作 */
                dmac_scan_switch_home_channel_work(pst_mac_device);
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::dmac_scan_switch_home_channel_work start.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_END:
                /* 如果扫描结束被打断，此时ps来恢复继续扫描 */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::dmac_scan_handle_switch_channel_back start.}");
                dmac_scan_end(pst_mac_device);
                break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
                /* abort状态一定是，在save状态时候scan baort，类似于ps=0要恢复ps=1的配置，因为scan abort提前resume了，此处不需要处理  */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::scan is already abort and resume.}");

                /* abort已经强制置为恢复，此次不保护，vap已经vap了不需要做操作 */
                /* 恢复发送和接收 */
                //dmac_vap_resume_tx_by_chl(pst_mac_device, pst_h2d_device, &(pst_h2d_device->st_wifi_channel_status));
                break;

            case HAL_BTCOEX_SW_POWSAVE_IDLE:
                /* 可能是状态1事件没有及时处理下半部，又来了状态0，此时处于扫描，恢复交给扫描来做即可 */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::HAL_BTCOEX_SW_POWSAVE_IDLE.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_WORK:
                /* 如果扫描期间来了1又来了0，此时恢复交给扫描自己恢复,ps=0不做处理 */
                break;

            default:
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::en_sw_preempt_type[%d] error.}",
                    GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
        }
    }
    else
    {
        switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
        {
            case HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN:
                if(WLAN_SCAN_MODE_BACKGROUND_CCA == pst_mac_device->st_scan_params.en_scan_mode)
                {
                    dmac_btcoex_resume_wifi(pst_mac_device);
                    dmac_scan_begin(pst_mac_device);
                }
                break;
            case HAL_BTCOEX_SW_POWSAVE_SCAN_END:
                if(WLAN_SCAN_MODE_BACKGROUND_CCA == pst_mac_device->st_scan_params.en_scan_mode)
                {
                    /* 如果扫描结束被打断，此时ps来恢复继续扫描 */
                    dmac_btcoex_resume_wifi(pst_mac_device);
                    dmac_scan_end(pst_mac_device);
                }
                break;
            case HAL_BTCOEX_SW_POWSAVE_PSM_END:
                /* 先执行了低功耗恢复，不要做操作 */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: powerdown already resume.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_IDLE:
                /* 可能是低功耗ps=1事件才开始执行，此时读取寄存器状态=0，当前是0状态，就按照0来处理即可，会连续来两个0的事件处理 */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: current is normal!.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
                /* 强制scan abort恢复了，此处不需要恢复 */
                OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: work state scan abort already resume.}");
                break;

            case HAL_BTCOEX_SW_POWSAVE_WORK:
                /* 恢复发送和接收 */
                if(HAL_BTCOEX_SW_POWSAVE_SUB_SCAN == GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device))
                {
                    GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;
                    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback:: work HAL_BTCOEX_SW_POWSAVE_SUB_SCAN.}");
                }
                else
                {
                    dmac_btcoex_resume_wifi(pst_mac_device);
                }
                break;

            default:
                OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::en_sw_preempt_type[%d] error.}",
                    GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
        }
    }

    if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_h2d_device))
    {
        /* 恢复硬件回cts */
        hal_enable_machw_cts_trans(pst_h2d_device);
    }
    /* preempt机制置为NONE形式 */
    GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_TIMEOUT;

    OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_pow_save_callback::time is up.}");

    return OAL_SUCC;
}



oal_void  dmac_btcoex_ps_timeout_update_time(hal_to_dmac_device_stru *pst_hal_device)
{
    hal_btcoex_ps_status_enum_uint8 en_ps_status = HAL_BTCOEX_PS_STATUE_BUTT;

    if(OAL_PTR_NULL == pst_hal_device)
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "dmac_btcoex_ps_timeout_update_time:pst_hal_device is null");
        return;
    }
    /* 获取当前ps业务状态 */
    hal_btcoex_get_ps_service_status(pst_hal_device, &en_ps_status);

    switch(en_ps_status)
    {
        case HAL_BTCOEX_PS_STATUE_ACL:
            pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms = BTCOEX_POWSAVE_TIMEOUT_LEVEL0;
            break;

        case HAL_BTCOEX_PS_STATUE_LDAC:
        case HAL_BTCOEX_PS_STATUE_LDAC_ACL:
        case HAL_BTCOEX_PS_STATUE_PAGE_INQ:
            pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms = BTCOEX_POWSAVE_TIMEOUT_LEVEL1;
            break;

        case HAL_BTCOEX_PS_STATUE_PAGE_ACL:
        case HAL_BTCOEX_PS_STATUE_LDAC_PAGE:
        case HAL_BTCOEX_PS_STATUE_TRIPLE:
            pst_hal_device->st_btcoex_sw_preempt.us_timeout_ms = BTCOEX_POWSAVE_TIMEOUT_LEVEL2;
            break;

        default:
            OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_timeout_update_time::en_ps_status[%d] error.}", en_ps_status);
    }
}



oal_uint32 dmac_btcoex_ps_status_handler(frw_event_mem_stru *pst_event_mem)
{
    frw_event_stru                     *pst_event;
    hal_to_dmac_device_stru            *pst_h2d_device;
    mac_device_stru                    *pst_mac_device;
    oal_bool_enum_uint8                 en_bt_acl_status;
    oal_uint32                          ul_ps_enqueue_time;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_event = frw_get_event_stru(pst_event_mem);

    hal_get_hal_to_dmac_device(pst_event->st_event_hdr.uc_chip_id, pst_event->st_event_hdr.uc_device_id, &pst_h2d_device);

    /* 记录事件入队时间 */
    ul_ps_enqueue_time = pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time;
    pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time = glbcnt_read_low32();
    if(pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time - ul_ps_enqueue_time > 20)
    {
        /* 中断上下半部执行时间差大于20 * 31.25us时，增加维测 */
        OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::ps start to end time beyond cnt[%d].}",
            pst_h2d_device->st_btcoex_sw_preempt.ul_ps_cur_time - ul_ps_enqueue_time);
    }

    /* 事件申请时在上半部处理时已经减为了0，这里要加回去，恢复成1 */
    oal_atomic_inc(&(pst_h2d_device->st_btcoex_sw_preempt.ul_ps_event_num));

    pst_mac_device = mac_res_get_dev(pst_h2d_device->uc_mac_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
       OAM_ERROR_LOG0(pst_h2d_device->uc_mac_device_id, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler: mac device is null ptr. device id:%d}");
       return OAL_ERR_CODE_PTR_NULL;
    }

    /* 1.软件ps未使能 */
    if (OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_ON(pst_h2d_device))
    {
        /* 未使能，直接返回 */
        return OAL_SUCC;
    }

    /* 2.业务ps stop检查 */
    if(OAL_TRUE == GET_HAL_BTCOEX_SW_PREEMPT_PS_STOP(pst_h2d_device))
    {
        return OAL_SUCC;
    }

    /* 3.业务ps pause检查 */
    if(OAL_TRUE == GET_HAL_BTCOEX_SW_PREEMPT_PS_PAUSE(pst_h2d_device))
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::ps need to pause.}");
        return OAL_SUCC;
    }

    /* 当前下半部事件，获取当前ps状态 */
    hal_btcoex_get_bt_acl_status(pst_h2d_device, &en_bt_acl_status);

    /* 因为低功耗造成连续两个相同的下半部来执行，尽量保证ps状态处理操作是交替执行，直接return, wifi下半部调度得慢，修改info打印
       上半部采用原子操作之后，这里基本不会再进来了 */
    if(en_bt_acl_status == pst_h2d_device->st_btcoex_sw_preempt.en_last_acl_status)
    {
        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::en_bt_acl_status is the same.}");
        return OAL_SUCC;
    }
    if (en_bt_acl_status)
    {
        /* preempt机制置为软件形式 */
        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_WORK;

        /* 在scan状态时候，CCA扫描的时候没有暂停发生队列，PS中断过来时需要对WiFi进行备份 */
        if (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state)
        {
            /* 在扫描执行过程中，不需要处理，扫描时对端已经处于节能状态并在tx pause状态，扫描结束时需要考虑是不是由ps来恢复，低功耗是200ms，
             即使扫描由ps来恢复，也来得及，ps等于0肯定在低功耗前来到或者恢复之后，不会被低功耗打断
            */
            GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_SCAN;
        }
        else
        {
            /* sta模式可能一开始还没关联上，处于非up状态，后续启动成up状态，ps=0时候恢复不生效，直接判断不处理即可 */

            /* 如果处于低功耗状态(主要是work状态下的awake子状态，收到ps中断，仍然对端在睡眠状态)，wifi不需要特殊处理，执行pause 不需要发ps帧，
            等低功耗自己来处理ps状态 */
            /* sta vap进行发送pause，并通知对端缓存数据暂停发送数据， 处于idle状态，空发一帧也没关系 */

            /* 1.先默认为active状态，能保证低功耗饿死时，中断ps=1也能唤醒 */
            GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;

            /* 2.刷新fcs业务类型 */
            if(OAL_TRUE == GET_BTCOEX_BT_LDAC_STATUS(pst_h2d_device))
            {
                pst_mac_device->st_fcs_mgr.en_fcs_service_type = HAL_FCS_SERVICE_TYPE_BTCOEX_LDAC;
            }
            else
            {
                pst_mac_device->st_fcs_mgr.en_fcs_service_type = HAL_FCS_SERVICE_TYPE_BTCOEX_NORMAL;
            }

            /* 3.停止wifi的业务 */
            dmac_btcoex_suspend_wifi(pst_mac_device);
        }

        /* 状态变迁,启动定时器完成ps操作，防止扫描置状态之后，出现wifi不醒来 */
        if(OAL_TRUE == pst_h2d_device->st_btcoex_powersave_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_h2d_device->st_btcoex_powersave_timer));
        }

        /* ps机制启动时，需要根据当前状态，刷新超时定时器时间 */
        dmac_btcoex_ps_timeout_update_time(pst_h2d_device);

        FRW_TIMER_CREATE_TIMER(&(pst_h2d_device->st_btcoex_powersave_timer),
                               dmac_btcoex_pow_save_callback,
                               pst_h2d_device->st_btcoex_sw_preempt.us_timeout_ms,
                               (oal_void *)pst_h2d_device,
                               OAL_FALSE,
                               OAM_MODULE_ID_DMAC,
                               pst_h2d_device->ul_core_id);
        
        if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_h2d_device))
        {
            /* 禁止硬件回cts */
            hal_disable_machw_cts_trans(pst_h2d_device);
        }
    }
    else
    {
        if(OAL_TRUE == pst_h2d_device->st_btcoex_powersave_timer.en_is_registerd)
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_h2d_device->st_btcoex_powersave_timer));
        }

        /* 在scan状态时候，扫描不进入低功耗，此时不用担心低功耗，如果扫描来时 */
        if (MAC_SCAN_STATE_RUNNING == pst_mac_device->en_curr_scan_state)
        {
            switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
            {
                case HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN:
                    /* 如果扫描一开始被ps打断，此时ps来恢复继续扫描 */
                    dmac_scan_begin(pst_mac_device);
                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_WAIT:
                    /* 如果扫描临时回home channel继续工作被打断，此时ps来恢复继续回home channel工作 */
                    dmac_scan_switch_home_channel_work(pst_mac_device);
                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_END:
                    /* 如果扫描结束被打断，此时ps来恢复继续扫描 */
                    dmac_scan_end(pst_mac_device);
                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
                    /* abort状态一定是，在save状态时候scan baort，类似于ps=0要恢复ps=1的配置，因为scan abort提前resume了，此处不需要处理  */
                    //OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::scan is already abort and resume.}");

                    /* abort已经强制置为恢复，此次不保护，vap已经vap了不需要做操作 */
                    /* 恢复发送和接收 */
                    //dmac_vap_resume_tx_by_chl(pst_mac_device, pst_h2d_device, &(pst_h2d_device->st_wifi_channel_status));
                    break;

                case HAL_BTCOEX_SW_POWSAVE_IDLE:
                    /* 可能是状态1事件没有及时处理下半部，又来了状态0，此时处于扫描，恢复交给扫描来做即可 */
                    break;

                case HAL_BTCOEX_SW_POWSAVE_WORK:
                    /* 如果扫描期间来了1又来了0，此时恢复交给扫描自己恢复,ps=0不做处理 */
                    break;

                case HAL_BTCOEX_SW_POWSAVE_TIMEOUT:
                    /* time is up， 或者低功耗已经提前恢复，属于正常 */
                    break;

                default:
                    OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::en_sw_preempt_type[%d] error.}",
                        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
            }
        }
        else
        {
            switch(GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device))
            {
                case HAL_BTCOEX_SW_POWSAVE_SCAN_BEGIN:
                    if(WLAN_SCAN_MODE_BACKGROUND_CCA == pst_mac_device->st_scan_params.en_scan_mode)
                    {
                        dmac_btcoex_resume_wifi(pst_mac_device);
                        dmac_scan_begin(pst_mac_device);
                    }
                    break;
                case HAL_BTCOEX_SW_POWSAVE_SCAN_END:
                    if(WLAN_SCAN_MODE_BACKGROUND_CCA == pst_mac_device->st_scan_params.en_scan_mode)
                    {
                        /* 如果扫描结束被打断，此时ps来恢复继续扫描 */
                        dmac_btcoex_resume_wifi(pst_mac_device);
                        dmac_scan_end(pst_mac_device);
                    }
                    break;
                case HAL_BTCOEX_SW_POWSAVE_PSM_END:
                    /* 先执行了低功耗恢复，不要做操作 */
                    break;

                case HAL_BTCOEX_SW_POWSAVE_IDLE:
                    /* 可能是低功耗ps=1事件才开始执行，此时读取寄存器状态=0，当前是0状态，就按照0来处理即可，会连续来两个0的事件处理 */
                    break;

                case HAL_BTCOEX_SW_POWSAVE_SCAN_ABORT:
                    /* 强制scan abort恢复了，此处不需要恢复 */
                    break;

                case HAL_BTCOEX_SW_POWSAVE_WORK:
                    /* 恢复发送和接收 */
                    if(HAL_BTCOEX_SW_POWSAVE_SUB_SCAN == GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device))
                    {
                        GET_HAL_BTCOEX_SW_PREEMPT_SUBTYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_SUB_ACTIVE;
                        OAM_WARNING_LOG0(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler:: work HAL_BTCOEX_SW_POWSAVE_SUB_SCAN.}");
                    }
                    else
                    {
                        dmac_btcoex_resume_wifi(pst_mac_device);
                    }
                    break;

                case HAL_BTCOEX_SW_POWSAVE_TIMEOUT:
                    /* time is up， 或者低功耗已经提前恢复，属于正常 */
                    break;
                default:
                    OAM_WARNING_LOG1(0, OAM_SF_COEX, "{dmac_btcoex_ps_status_handler::en_sw_preempt_type[%d] error.}",
                        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device));
            }
        }

        if(OAL_FALSE == HAL_BTCOEX_CHECK_SW_PREEMPT_REPLY_CTS_ON(pst_h2d_device))
        {
            /* 恢复硬件回cts */
            hal_enable_machw_cts_trans(pst_h2d_device);
        }
        /* preempt机制置为IDLE形式 */
        GET_HAL_BTCOEX_SW_PREEMPT_TYPE(pst_h2d_device) = HAL_BTCOEX_SW_POWSAVE_IDLE;
    }

    /* 软件记录上一次的acl状态 */
    pst_h2d_device->st_btcoex_sw_preempt.en_last_acl_status = en_bt_acl_status;

    return OAL_SUCC;
}

#endif /* end of _PRE_WLAN_FEATURE_COEXIST_BT */

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

