

#ifndef __MAC_PM_H__
#define __MAC_PM_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if defined(_PRE_WLAN_FEATURE_AP_PM) || defined(_PRE_WLAN_FEATURE_STA_PM)
/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "mac_vap.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_PM_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define MAC_PM_ARBITER_MAX_REQUESTORS   16      /*最大参与仲裁者数目*/
#define MAC_PM_ARBITER_MAX_REQ_NAME     16      /*请求仲裁者名字最大长度*/
#define MAC_PWR_ARBITER_ID_INVALID      255     /* invalid arbiter id */

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum {
    MAC_PWR_ARBITER_TYPE_INVALID = 0,  /*非法类型*/
    MAC_PWR_ARBITER_TYPE_AP,           /*AP类型*/
    MAC_PWR_ARBITER_TYPE_STA,          /*STA类型*/
    MAC_PWR_ARBITER_TYPE_P2P,          /*P2P类型*/

    MAC_PWR_ARBITER_TYPE_BUTT
} mac_pm_arbiter_type_enum;

/*device的状态枚举，VAP的节能状态到device的节能状态要做映射，做到AP类型vap，sta类型VAP
P2P类型vap的状态机可自行定义，最后通过仲裁归一到device的状态*/
typedef enum {
    DEV_PWR_STATE_WORK = 0,         /*工作状态*/
    DEV_PWR_STATE_DEEP_SLEEP,      /*深睡状态*/
    DEV_PWR_STATE_WOW,             /*WOW状态*/
    DEV_PWR_STATE_IDLE,            /*idle状态，无用户关联*/
    DEV_PWR_STATE_OFF,             /*下电状态*/

    DEV_PWR_STATE_BUTT             /*最大状态*/
} device_pwr_state_enum;

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
typedef struct _mac_pwr_event_info{
    oal_uint32           ul_event_id;           /*event枚举值,每状态机自己定义*/
    const oal_int8      *auc_event_name;         /*event命名*/
}mac_pwr_event_info;

struct mac_pm_arbiter_requestor_info {
    oal_uint8                   auc_id_name[MAC_PM_ARBITER_MAX_REQ_NAME]; /* names of the requestors */
    mac_pm_arbiter_type_enum    en_arbiter_type;
} ;

typedef struct _mac_pm_arbiter_state_info{
    oal_uint32               ul_state;               /*state枚举值,每状态机自己定义*/
    const oal_int8          *auc_state_name;         /*state命名*/
}mac_pm_arbiter_state_info;

typedef struct _mac_pm_arbiter_info {
    oal_uint32   ul_id_bitmap;                          /*分配的requestor id bitmaps */
    oal_uint32   ul_state_bitmap[DEV_PWR_STATE_BUTT];   /*每个状态对应一个bitmap*/
    oal_uint8    uc_cur_state;                          /*当前device的低功耗状态*/
    oal_uint8    uc_prev_state;                         /*前一device的低功耗状态*/
    oal_uint8    uc_requestor_num;                      /*当前请求仲裁者的数目*/
    oal_uint8    uc_rsv;
    mac_pm_arbiter_state_info   *pst_state_info;
    struct mac_pm_arbiter_requestor_info requestor[MAC_PM_ARBITER_MAX_REQUESTORS];  /*投票者的信息，维测用*/
}mac_pm_arbiter_stru;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 mac_pm_arbiter_init(mac_device_stru* pst_device);
extern oal_uint32 mac_pm_arbiter_destroy(mac_device_stru* pst_device);
extern oal_uint32 mac_pm_arbiter_alloc_id(mac_device_stru* pst_device, oal_uint8* pst_name,mac_pm_arbiter_type_enum en_arbiter_type);
extern oal_uint32 mac_pm_arbiter_free_id(mac_device_stru* pst_device, oal_uint32 ul_arbiter_id);
extern oal_void mac_pm_arbiter_to_state(mac_device_stru *pst_device, mac_vap_stru *pst_mac_vap, oal_uint32 ul_arbiter_id,
                                            oal_uint8  uc_state_from, oal_uint8  uc_state_to);
extern oal_uint32 mac_pm_set_hal_state(mac_device_stru *pst_device,mac_vap_stru *pst_mac_vap,oal_uint8 uc_state_to);


#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_fsm.h */
