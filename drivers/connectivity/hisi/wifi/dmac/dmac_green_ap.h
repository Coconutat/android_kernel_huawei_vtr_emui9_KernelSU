
#ifndef    __DMAC_GREEN_AP_H__
#define    __DMAC_GREEN_AP_H__

#ifdef  __cplusplus
#if     __cplusplus
extern  "C" {
#endif
#endif

#ifdef    _PRE_WLAN_FEATURE_GREEN_AP
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include    "oal_types.h"
#include    "oal_hardware.h"
#include    "frw_ext_if.h"
#include    "mac_device.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_DMAC_GREEN_AP_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define  DMAC_DEFAULT_GAP_WORK_TIME     10
#define  DMAC_DEFAULT_GAP_PAUSE_TIME    10
#define  DMAC_DEFAULT_GAP_SLOT_CNT      2*100/(DMAC_DEFAULT_GAP_WORK_TIME + DMAC_DEFAULT_GAP_PAUSE_TIME)
#define  DMAC_DEFAULT_GAP_RESUME_OFFSET 1

#define  DMAC_GAP_EN_THRPT_HIGH_TH        50                        /* 吞吐量低于此门限，green ap才开启 */
#define  DMAC_GAP_PPS_TO_THRPT_MBPS(pps)  ((pps * 8 * 1460) >> 20)  /* pps转吞吐量(Mbps) */
/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    DMAC_GREEN_AP_STATE_NOT_INITED   = 0,
    DMAC_GREEN_AP_STATE_INITED       = 1,
    DMAC_GREEN_AP_STATE_WORK         = 2,
    DMAC_GREEN_AP_STATE_PAUSE        = 3,

    DMAC_GREEN_AP_STATE_BUTT
}dmac_green_ap_state_enum;
typedef oal_uint8 dmac_green_ap_state_enum_uint8;


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
typedef struct tag_dmac_green_ap_mgr_stru
{
    oal_uint8                      uc_green_ap_enable;       /* green ap使能 */
    oal_uint8                      uc_green_ap_suspend;      /* green ap是否suspend */
    oal_bool_enum_uint8            en_green_ap_dyn_en;       /* green ap 动态使能开关(吞吐量判断) */
    oal_bool_enum_uint8            en_green_ap_dyn_en_old;   /* 上个周期的green ap 动态使能结果 */
    dmac_green_ap_state_enum_uint8 uc_state;                 /* green ap状态 */
    oal_uint8                      resv1[3];

    oal_uint8 uc_device_id;         /* 指向对应的device id */
    oal_uint8 uc_hal_vap_id;
    oal_uint8 uc_vap_id;
    oal_uint8 uc_work_time;         /* 工作时隙内时间ms*/
    oal_uint8 uc_pause_time;        /* 暂停时隙内时间ms */
    oal_uint8 uc_max_slot_cnt;      /* beacon周期分为多少个时隙 = 2*beacon period/(work+pause) */
    oal_uint8 uc_cur_slot;          /* 当前时隙 */
    oal_uint8 uc_resume_offset;     /* resume需要提前的时间量，在保护时间到前提前resume恢复RF */

#if (!defined(_PRE_PRODUCT_ID_HI110X_DEV))
    struct hrtimer st_gap_timer;
#endif

    hal_one_packet_cfg_stru         st_one_packet_cfg;
    oal_uint8                       uc_one_packet_done;
    oal_uint8                       resv2[3];

    oal_uint32                      ul_pause_count;
    oal_uint32                      ul_resume_count;
    oal_uint32                      ul_total_count;
}dmac_green_ap_mgr_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  dmac_green_ap_init(mac_device_stru *pst_device);
extern oal_uint32  dmac_green_ap_exit(mac_device_stru *pst_device);
extern oal_uint32  dmac_green_ap_timer_event_handler(frw_event_mem_stru *pst_event_mem);
extern oal_uint32  dmac_green_ap_start(oal_uint8 uc_device_id);
extern oal_uint32  dmac_green_ap_stop(oal_uint8 uc_device_id);
extern oal_uint32  dmac_green_ap_suspend(mac_device_stru *pst_device);
extern oal_uint32  dmac_green_ap_resume(mac_device_stru *pst_device);
extern oal_uint32  dmac_green_ap_dump_info(oal_uint8 uc_device_id);
extern oal_void  dmac_green_ap_pps_process(oal_uint32 ul_pps_rate);

#endif /* _PRE_WLAN_FEATURE_GREEN_AP */

#ifdef  __cplusplus
#if     __cplusplus
    }
#endif
#endif

#endif
