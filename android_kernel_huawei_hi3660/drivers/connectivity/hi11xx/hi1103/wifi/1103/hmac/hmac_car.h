
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#ifndef __HMAC_CAR_H__
#define __HMAC_CAR_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "hmac_device.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_CAR_H
/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define HMAC_CAR_CYCLE_MS               100         //CAR默认定时器超时时间ms
#define HMAC_CAR_WIFI_ETH               20          //上行需要减20个字节；wifi 34(26+8), eth 14(6+6+2); wifi-eth;用eth统计
#define HMAC_CAR_MULTICAST_PPS_NUM      0          //CAR组播默认pps的数目:0表示不使用pps限速，>0表示用pps限速的数值


/* 遍历VAP/USER操作封装 */
#define HMAC_DEVICE_FOREACH_VAP(_pst_vap, _pst_device, _uc_vap_index)   \
    for ((_uc_vap_index) = 0,   \
         (_pst_vap) = ((_pst_device)->uc_vap_num > 0)? ((mac_vap_stru *)mac_res_get_mac_vap((_pst_device)->auc_vap_id[0])) : OAL_PTR_NULL;   \
         (_uc_vap_index) < (_pst_device)->uc_vap_num;   \
         (_uc_vap_index) ++,                             \
         (_pst_vap) = ((_uc_vap_index) < (_pst_device)->uc_vap_num)? ((mac_vap_stru *)mac_res_get_mac_vap((_pst_device)->auc_vap_id[_uc_vap_index])) : OAL_PTR_NULL) \
         if (OAL_PTR_NULL !=(_pst_vap))

#define HMAC_VAP_FOREACH_USER(_pst_user, _pst_vap, _pst_list_pos)       \
             for ((_pst_list_pos) = (_pst_vap)->st_mac_user_list_head.pst_next,  \
                  (_pst_user) = OAL_DLIST_GET_ENTRY((_pst_list_pos), mac_user_stru, st_user_dlist);      \
                  (_pst_list_pos) != &((_pst_vap)->st_mac_user_list_head);                               \
                  (_pst_list_pos) = (_pst_list_pos)->pst_next,                                           \
                  (_pst_user) = OAL_DLIST_GET_ENTRY((_pst_list_pos), mac_user_stru, st_user_dlist))     \
                  if (OAL_PTR_NULL != (_pst_user))

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


/*****************************************************************************
  8 UNION定义
*****************************************************************************/

/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

extern oal_uint32  hmac_car_device_bw_limit(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param);

extern oal_uint32  hmac_car_vap_bw_limit(hmac_vap_stru *pst_hmac_vap, mac_cfg_car_stru *pst_car_cfg_param);
extern oal_uint32  hmac_car_user_bw_limit(hmac_vap_stru *pst_hmac_vap, mac_cfg_car_stru *pst_car_cfg_param);
extern oal_uint32  hmac_car_device_multicast(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param);
extern oal_uint32  hmac_car_device_timer_cycle_limit(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param);
extern oal_uint32  hmac_car_process(hmac_device_stru *pst_hmac_dev, hmac_vap_stru *pst_hmac_vap,
                                          hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_buf,
                                          hmac_car_up_down_type_enum_uint8 en_car_type);
extern oal_uint32  hmac_car_process_uplink(oal_uint16 us_ta_user_idx, hmac_vap_stru *pst_hmac_vap,
                                                  oal_netbuf_stru *pst_buf, hmac_car_up_down_type_enum_uint8 en_car_type);
extern oal_uint32  hmac_car_enable_switch(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param);
extern oal_uint32  hmac_car_show_info(hmac_device_stru *pst_hmac_dev);
extern oal_uint32  hmac_car_multicast_process(hmac_device_stru *pst_hmac_dev, oal_netbuf_stru *pst_buf);
extern oal_uint32  hmac_car_device_multicast_pps_num(hmac_device_stru *pst_hmac_dev, mac_cfg_car_stru *pst_car_cfg_param);
extern oal_uint32  hmac_car_init(hmac_device_stru *pst_hmac_dev);
extern oal_uint32  hmac_car_exit(hmac_device_stru *pst_hmac_dev);

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
