

#ifndef __DMAC_BTCOEX_H__
#define __DMAC_BTCOEX_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_BTCOEX

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "hal_ext_if.h"
#include "mac_resource.h"
#include "mac_board.h"
#include "mac_frame.h"
#include "dmac_ext_if.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_BTCOEX_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/

#define BTCOEX_RELEASE_TIMEOUT              (1000)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
typedef struct
{
    mac_header_frame_control_stru   st_frame_control;
    oal_uint16                      bit_duration_value      : 15,           /* duration/id */
                                    bit_duration_flag       : 1;
    oal_uint8                       auc_address1[WLAN_MAC_ADDR_LEN];
    oal_uint8                       auc_address2[WLAN_MAC_ADDR_LEN];
    oal_uint8                       auc_address3[WLAN_MAC_ADDR_LEN];
    oal_uint16                      bit_sc_frag_num     : 4,                /* sequence control */
                                    bit_sc_seq_num      : 12;
    oal_uint8                       bit_qc_tid          : 4,
                                    bit_qc_eosp         : 1,
                                    bit_qc_ack_polocy   : 2,
                                    bit_qc_amsdu        : 1;

} dmac_btcoex_qosnull_frame_stru;

typedef struct
{
    frw_timeout_stru bt_coex_low_rate_timer;
    frw_timeout_stru bt_coex_statistics_timer;
    frw_timeout_stru bt_coex_sco_statistics_timer;
    oal_uint8 uc_rx_rate_statistics_flag;
    oal_uint8 uc_rx_rate_statistics_timeout;
    oal_uint8 uc_sco_rx_rate_statistics_flag;
    oal_uint8 uc_resv[1];
} dmac_vap_btcoex_rx_statistics_stru;

typedef struct
{
    frw_timeout_stru bt_coex_priority_timer;                 /* 读取寄存器周期定时器 */
    frw_timeout_stru bt_coex_occupied_timer;                 /* 周期拉高occupied信号线，保证WiFi不被BT抢占 */
    oal_uint32 ul_ap_beacon_count;
    oal_uint32 ul_timestamp;
    oal_uint8 uc_ap_beacon_miss;
    oal_uint8 uc_occupied_times;
    oal_uint8 uc_prio_occupied_state;
    oal_uint8 uc_linkloss_occupied_times;
    oal_uint8 uc_linkloss_index;
    oal_uint8 auc_resv[3];
} dmac_vap_btcoex_occupied_stru;

typedef struct
{
    dmac_vap_btcoex_rx_statistics_stru st_dmac_vap_btcoex_rx_statistics;
    dmac_vap_btcoex_occupied_stru st_dmac_vap_btcoex_occupied;

    oal_uint8 auc_null_qosnull_frame[BTCOEX_MAC_HDR];
    coex_preempt_frame_enum_uint8  en_all_abort_preempt_type;
    oal_uint8 auc_resv[3];
} dmac_vap_btcoex_stru;

typedef struct
{
    oal_uint32 ul_rx_rate_threshold_min;
    oal_uint32 ul_rx_rate_threshold_max;
    oal_uint8 uc_get_addba_req_flag;
    oal_uint8 uc_ba_size_index;
    oal_uint8 uc_ba_size_addba_rsp_index;
    oal_uint8 uc_ba_size;
    oal_uint8 uc_ba_size_tendence;
    oal_uint8 auc_resv[3];
} dmac_user_btcoex_delba_stru;

typedef struct
{
    oal_uint16 us_rx_rate_stat_count;
    oal_uint64 ull_rx_rate_mbps;
} dmac_user_btcoex_rx_info_stru;

typedef struct
{
    oal_uint8 uc_status;
    oal_uint8 uc_status_sl_time;
	oal_uint8 auc_resv[2];
} dmac_user_btcoex_sco_rx_rate_status_stru;

typedef struct
{
    dmac_user_btcoex_delba_stru st_dmac_user_btcoex_delba;
    dmac_user_btcoex_rx_info_stru st_dmac_user_btcoex_rx_info;
    dmac_user_btcoex_rx_info_stru st_dmac_user_btcoex_sco_rx_info;
    dmac_user_btcoex_sco_rx_rate_status_stru st_dmac_user_btcoex_sco_rx_rate_status;
} dmac_user_btcoex_stru;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
OAL_INLINE OAL_STATIC oal_uint32 dmac_btcoex_check_legacy_sta(mac_vap_stru *pst_mac_vap)
{
    if ((WLAN_VAP_MODE_BSS_STA == pst_mac_vap->en_vap_mode)
        && (WLAN_LEGACY_VAP_MODE == pst_mac_vap->en_p2p_mode))
    {
        return OAL_TRUE;
    }
    else
    {
        return OAL_FALSE;
    }
}

OAL_INLINE OAL_STATIC oal_void dmac_btcoex_get_legacy_sta(mac_device_stru *pst_mac_device,
                                                                        mac_vap_stru **ppst_mac_vap)
{
    oal_uint8 uc_vap_idx;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        *ppst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == (*ppst_mac_vap))
        {
            continue;
        }

        if (OAL_TRUE == dmac_btcoex_check_legacy_sta(*ppst_mac_vap))
        {
            return;
        }
    }
    *ppst_mac_vap = OAL_PTR_NULL;
}

OAL_INLINE OAL_STATIC oal_uint32 dmac_btcoex_check_legacy_ap(mac_vap_stru *pst_mac_vap)
{
    if ((WLAN_VAP_MODE_BSS_AP == pst_mac_vap->en_vap_mode)
        && (WLAN_LEGACY_VAP_MODE == pst_mac_vap->en_p2p_mode))
    {
        return OAL_TRUE;
    }
    else
    {
        return OAL_FALSE;
    }
}

OAL_INLINE OAL_STATIC oal_void dmac_btcoex_get_legacy_ap(mac_device_stru *pst_mac_device,
                                                                        mac_vap_stru **ppst_mac_vap)
{
    oal_uint8 uc_vap_idx;

    for (uc_vap_idx = 0; uc_vap_idx < pst_mac_device->uc_vap_num; uc_vap_idx++)
    {
        *ppst_mac_vap = (mac_vap_stru *)mac_res_get_mac_vap(pst_mac_device->auc_vap_id[uc_vap_idx]);
        if (OAL_PTR_NULL == (*ppst_mac_vap))
        {
            continue;
        }

        if (OAL_TRUE == dmac_btcoex_check_legacy_ap(*ppst_mac_vap))
        {
            return;
        }
    }
    *ppst_mac_vap = OAL_PTR_NULL;
}

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 dmac_btcoex_init(oal_void);
extern oal_uint32 dmac_btcoex_exit(oal_void);
extern oal_void dmac_btcoex_init_preempt(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user, coex_preempt_frame_enum_uint8 en_preempt_type);
extern oal_void dmac_btcoex_wlan_priority_set(mac_vap_stru *pst_mac_vap, oal_uint8 uc_value, oal_uint8 uc_timeout_ms);
extern oal_uint32 dmac_btcoex_wlan_priority_timeout_callback(oal_void *p_arg);
extern oal_void dmac_btcoex_change_state_syn(mac_vap_stru *pst_mac_vap);
extern oal_void dmac_btcoex_vap_up_handle(mac_vap_stru *pst_mac_vap);
extern oal_void dmac_btcoex_vap_down_handle(mac_vap_stru *pst_mac_vap);
extern oal_uint32 dmac_config_btcoex_assoc_state_syn(mac_vap_stru *pst_mac_vap, mac_user_stru *pst_mac_user);
extern oal_uint32 dmac_config_btcoex_disassoc_state_syn(mac_vap_stru *pst_mac_vap);
extern oal_void dmac_btcoex_delba_trigger(mac_vap_stru *pst_mac_vap, oal_uint8 uc_bt_on,oal_uint8 uc_ba_size);
extern oal_void dmac_btcoex_bt_low_rate_process (mac_vap_stru *pst_vap, hal_to_dmac_device_stru *pst_hal_device);
extern oal_void dmac_btcoex_release_rx_prot(mac_vap_stru *pst_mac_vap, oal_uint8 uc_data_type);
extern oal_void dmac_btcoex_tx_vip_frame(hal_to_dmac_device_stru *pst_hal_device, mac_vap_stru *pst_mac_vap, oal_dlist_head_stru *pst_tx_dscr_list_hdr);
extern oal_void dmac_btcoex_sco_rx_rate_process (mac_vap_stru *pst_vap, hal_to_dmac_device_stru *pst_hal_device);
extern oal_void dmac_btcoex_ps_stop_check_and_notify(oal_void);
extern oal_void dmac_btcoex_ps_pause_check_and_notify(hal_to_dmac_device_stru *pst_hal_device);
extern oal_uint32 dmac_btcoex_ps_status_handler(frw_event_mem_stru *pst_event_mem);
#endif /* #ifdef _PRE_WLAN_FEATURE_COEXIST_BT */

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of dmac_data_acq.h */
