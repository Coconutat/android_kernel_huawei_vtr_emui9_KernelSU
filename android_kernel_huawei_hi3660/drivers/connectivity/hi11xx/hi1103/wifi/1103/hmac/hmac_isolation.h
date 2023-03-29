

#ifndef __HMAC_ISOLATION_H__
#define __HMAC_ISOLATION_H__

#ifdef _PRE_WLAN_FEATURE_ISOLATION

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

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/* 隔离模式 mode*/
typedef enum
{
    CS_ISOLATION_MODE_BROADCAST=0x01,        /* 广播     */
    CS_ISOLATION_MODE_MULTICAST=0x02,        /* 组播     */
    CS_ISOLATION_MODE_UNICAST=0x04,          /* 单播     */
}cs_isolation_mode_enum;

/* 隔离类型 */
typedef enum
{
    CS_ISOLATION_TYPE_NONE,             /* 关闭隔离        */
    CS_ISOLATION_TYPE_MULTI_BSS,        /* 多BSS隔离       */
    CS_ISOLATION_TYPE_SINGLE_BSS,       /* 单BSS隔离       */

    CS_ISOLATION_TYPE_BUTT
}cs_isolation_type_enum;
typedef oal_uint8 cs_isolation_type_enum_uint8;

/* 隔离forwording方式 */
typedef enum
{
    CS_ISOLATION_FORWORD_NONE,         /* 隔离forword为不丢弃       */
    CS_ISOLATION_FORWORD_TOLAN,        /* 隔离forword为发送到lan  */
    CS_ISOLATION_FORWORD_DROP,         /* 隔离forword为丢弃       */

    CS_ISOLATION_FORWORD_BUTT
}cs_isolation_forward_enum;
typedef oal_uint8 cs_isolation_forward_enum_uint8;

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
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32 hmac_isolation_set_mode(mac_vap_stru *pst_mac_vap, oal_uint8 uc_mode);
extern oal_uint32 hmac_isolation_set_type(mac_vap_stru *pst_mac_vap, oal_uint8 uc_bss_type, oal_uint8 uc_isolation_type);
extern oal_uint32 hmac_isolation_set_forward(mac_vap_stru *pst_mac_vap, oal_uint8 uc_forward);
extern cs_isolation_forward_enum hmac_isolation_filter(mac_vap_stru *pst_mac_vap, oal_uint8 *puc_mac_addr);

extern oal_uint32 hmac_isolation_clear_counter(mac_vap_stru *pst_mac_vap);
extern oal_void hmac_show_isolation_info(mac_vap_stru *pst_mac_vap);

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif  /* #ifdef _PRE_WLAN_FEATURE_ISOLATION */

#endif /* end of hmac_isolation.h */

