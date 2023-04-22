

#ifndef __HMAC_BLAKLIST_H__
#define __HMAC_BLAKLIST_H__

#if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE)

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "mac_vap.h"
#include "hmac_vap.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define CS_INVALID_AGING_TIME         0
#define CS_DEFAULT_AGING_TIME         3600
#define CS_DEFAULT_RESET_TIME         3600
#define CS_DEFAULT_THRESHOLD          100



/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/* 黑名单类型 */
typedef enum
{
    CS_BLACKLIST_TYPE_ADD,           /* 增加       */
    CS_BLACKLIST_TYPE_DEL,           /* 删除       */

    CS_BLACKLIST_TYPE_BUTT
}cs_blacklist_type_enum;
typedef oal_uint8 cs_blacklist_type_enum_uint8;




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
/* 自动黑名单配置参数 */
typedef struct
{
    oal_uint8                       uc_enabled;               /* 使能标志 0:未使能  1:使能 */
    oal_uint8                       auc_reserved[3];          /* 字节对齐                  */
    oal_uint32                      ul_threshold;             /* 门限                      */
    oal_uint32                      ul_reset_time;            /* 重置时间                  */
    oal_uint32                      ul_aging_time;            /* 老化时间                  */
} hmac_autoblacklist_cfg_stru;

/* 黑白名单配置 */
typedef struct
{
    oal_uint8                       uc_type;                  /* 配置类型    */
    oal_uint8                       uc_mode;                  /* 配置模式    */
    oal_uint8                       auc_sa[6];                /* mac地址     */
} hmac_blacklist_cfg_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 hmac_autoblacklist_enable_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_enabled);
extern oal_uint32 hmac_autoblacklist_set_aging_etc(mac_vap_stru *pst_mac_vap, oal_uint32 ul_aging_time);
extern oal_uint32 hmac_autoblacklist_set_threshold_etc(mac_vap_stru *pst_mac_vap, oal_uint32 ul_threshold);
extern oal_uint32 hmac_autoblacklist_set_reset_time_etc(mac_vap_stru *pst_mac_vap, oal_uint32 ul_reset_time);
extern oal_void hmac_autoblacklist_filter_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr);
extern oal_uint32 hmac_blacklist_set_mode_etc(mac_vap_stru *pst_mac_vap, oal_uint8 uc_mode);
extern oal_uint32 hmac_blacklist_get_mode(mac_vap_stru *pst_mac_vap, oal_uint8 *uc_mode);
extern oal_uint32 hmac_blacklist_add_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint32 ul_aging_time);
extern oal_uint32 hmac_blacklist_add_only_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr, oal_uint32 ul_aging_time);
extern oal_uint32 hmac_blacklist_del_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr);
extern oal_bool_enum_uint8 hmac_blacklist_filter_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr);
extern oal_void hmac_show_blacklist_info_etc(mac_vap_stru *pst_mac_vap);
extern oal_uint32 hmac_backlist_get_drop_etc(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr);
extern oal_uint8 hmac_backlist_get_list_num_etc(mac_vap_stru *pst_mac_vap);
extern oal_bool_enum_uint8 hmac_blacklist_get_assoc_ap(mac_vap_stru *pst_cur_vap, oal_uint8 *puc_mac_addr, mac_vap_stru **pst_assoc_vap);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif  /* #if (_PRE_WLAN_FEATURE_BLACKLIST_LEVEL != _PRE_WLAN_FEATURE_BLACKLIST_NONE) */

#endif /* end of hmac_blacklist.h */

