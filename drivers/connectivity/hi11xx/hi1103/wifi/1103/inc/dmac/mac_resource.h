

#ifndef __MAC_RESOURCE_H__
#define __MAC_RESOURCE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_queue.h"
#include "mac_device.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_MAC_RESOURCE_H

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/* 最大dev数量 */
#define MAC_RES_MAX_DEV_NUM     WLAN_SERVICE_DEVICE_SUPPORT_MAX_NUM_SPEC
#define MAX_MAC_FCS_MEM_RES 2           /* CPU,MAC总线访问约束需要修改fcs发帧内存位置 */
#define MAX_MAC_BEACON_BUFF_NUM 2       /* CPU,MAC总线访问约束需要修改fcs发帧内存位置 */

/*board最大关联用户数 = 1个CHIP支持的最大关联用户数 * board上面的CHIP数目*/
#define MAC_RES_MAX_ASOC_USER_NUM    (WLAN_ASSOC_USER_MAX_NUM * WLAN_CHIP_MAX_NUM_PER_BOARD)
/*board最大组播用户数 = 整board支持的业务vap个数(每个业务vap一个组播用户)*/
#define MAC_RES_MAX_BCAST_USER_NUM   (WLAN_MULTI_USER_MAX_NUM_LIMIT)
/*board资源最大用户数 = 最大关联用户数 + 组播用户个数 */
#define MAC_RES_MAX_USER_LIMIT       (MAC_RES_MAX_ASOC_USER_NUM + MAC_RES_MAX_BCAST_USER_NUM)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
extern oal_uint16  g_us_max_asoc_user_etc;
extern oal_uint8   g_uc_max_active_user;


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/*
typedef struct
{
    mac_res_user_hash_stru      ast_user_hash_info[WLAN_ASSOC_USER_MAX_NUM];
    oal_queue_stru              st_queue;
    oal_uint32                  aul_idx[WLAN_ASSOC_USER_MAX_NUM];
    oal_uint8                   auc_user_cnt[WLAN_ASSOC_USER_MAX_NUM];

}mac_res_hash_stru;
*/
typedef struct
{
   oal_uint                 ul_user_idx_size;
}mac_res_user_idx_size_stru;

typedef struct
{
   oal_uint8                ul_user_cnt_size;
}mac_res_user_cnt_size_stru;

typedef struct
{
    mac_device_stru     ast_dev_info[MAC_RES_MAX_DEV_NUM];
    oal_queue_stru      st_queue;
    oal_uint            aul_idx[MAC_RES_MAX_DEV_NUM];
    oal_uint8           auc_user_cnt[MAC_RES_MAX_DEV_NUM];
    oal_uint8           auc_resv[2];
}mac_res_device_stru;

typedef struct
{
    oal_void               *past_vap_info[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];
    oal_queue_stru          st_queue;
    oal_uint                aul_idx[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];
    oal_uint16              us_hmac_priv_size;
    oal_uint8               auc_user_cnt[WLAN_VAP_SUPPORT_MAX_NUM_LIMIT];
    oal_uint8               auc_resv[2];
}mac_res_vap_stru;

typedef struct
{
    oal_void               *past_user_info[MAC_RES_MAX_USER_LIMIT];
    oal_queue_stru_16       st_queue;
    oal_uint                *pul_idx;
    oal_uint16              us_hmac_priv_size;
    oal_uint8               auc_resv[2];
    oal_uint8              *puc_user_cnt;

}mac_res_user_stru;


typedef struct
{
    mac_res_device_stru st_dev_res;
    mac_res_vap_stru    st_vap_res;
    mac_res_user_stru   st_user_res;
}mac_res_stru;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
extern mac_res_stru    g_st_mac_res_etc;
extern mac_res_stru  *g_pst_mac_res;

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  mac_res_alloc_dmac_dev_etc(oal_uint8    *puc_dev_idx);
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint32  mac_res_alloc_hmac_dev_etc(oal_uint8    *puc_dev_idx);
#else
extern oal_uint32  mac_res_alloc_hmac_dev_etc(oal_uint32    ul_dev_idx);
#endif
extern oal_uint32  mac_res_free_dev_etc(oal_uint32 ul_dev_idx);
extern oal_uint32  mac_res_free_mac_user_etc(oal_uint16 us_idx);
extern oal_uint32  mac_res_free_mac_vap_etc(oal_uint32 ul_idx);
extern oal_uint32  mac_res_init_etc(oal_void);
extern oal_uint32  mac_res_exit_etc(void);
extern mac_chip_stru  *mac_res_get_mac_chip(oal_uint32 ul_chip_idx);
oal_uint16  mac_chip_get_max_asoc_user(oal_uint8 uc_chip_id);
oal_uint8  mac_chip_get_max_active_user(oal_void);
oal_uint8  mac_chip_get_max_multi_user(oal_void);
oal_uint16  mac_board_get_max_user(oal_void);
#ifdef _PRE_WLAN_WEB_CMD_COMM
extern oal_void mac_res_set_hw_addr(oal_uint8 *puc_addr);
extern oal_void  mac_res_set_mac_bitmap(oal_uint8 val);
extern oal_void  mac_res_clear_mac_bitmap(oal_uint8 *puc_addr);
extern oal_int32 mac_res_get_mac_addr(oal_uint8 *puc_addr);
#endif



OAL_STATIC OAL_INLINE oal_uint32  mac_res_alloc_hmac_vap(oal_uint8 *puc_idx, oal_uint16 us_hmac_priv_size)
{
    oal_uint  ul_idx_temp;

    if (OAL_UNLIKELY(OAL_PTR_NULL == puc_idx))
    {
        return OAL_FAIL;
    }

    ul_idx_temp = (oal_uint)oal_queue_dequeue(&(g_pst_mac_res->st_vap_res.st_queue));

    /* 0为无效值 */
    if (0 == ul_idx_temp)
    {
        return OAL_FAIL;
    }

    *puc_idx = (oal_uint8)(ul_idx_temp - 1);

    g_pst_mac_res->st_vap_res.us_hmac_priv_size = us_hmac_priv_size;

    (g_pst_mac_res->st_vap_res.auc_user_cnt[ul_idx_temp - 1])++;

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_uint32  mac_res_alloc_dmac_vap(oal_uint8 uc_idx)
{
    if (OAL_UNLIKELY(uc_idx >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT))
    {
        return OAL_FAIL;
    }

    (g_pst_mac_res->st_vap_res.auc_user_cnt[uc_idx])++;

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_uint32  mac_res_alloc_dmac_user(oal_uint16 us_idx)
{
    if (OAL_UNLIKELY(us_idx >= MAC_RES_MAX_USER_LIMIT))
    {
        return OAL_FAIL;
    }

    /* 非ffload模式，用户自增在hmac侧已经执行 */
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    /* DMAC仅需要设置为有效即可 */
    (g_pst_mac_res->st_user_res.puc_user_cnt[us_idx])++;
#endif

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE mac_device_stru*  mac_res_get_dev_etc(oal_uint32 ul_dev_idx)
{
    if (OAL_UNLIKELY(ul_dev_idx >= MAC_RES_MAX_DEV_NUM))
    {
        return OAL_PTR_NULL;
    }

    return &(g_pst_mac_res->st_dev_res.ast_dev_info[ul_dev_idx]);
}


OAL_STATIC OAL_INLINE oal_void*  mac_res_get_hmac_vap(oal_uint32 ul_idx)
{
    if (OAL_UNLIKELY(ul_idx >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT))
    {
        return OAL_PTR_NULL;
    }

    return (oal_void *)(g_pst_mac_res->st_vap_res.past_vap_info[ul_idx]);
}


OAL_STATIC OAL_INLINE oal_void*  mac_res_get_mac_vap(oal_uint8 uc_idx)
{
    if (OAL_UNLIKELY(uc_idx >= WLAN_VAP_SUPPORT_MAX_NUM_LIMIT))
    {
        return OAL_PTR_NULL;
    }

    /* 这里返回偏移内存空间 */
    return (oal_void *)((oal_uint8 *)(g_pst_mac_res->st_vap_res.past_vap_info[uc_idx])
                        + g_pst_mac_res->st_vap_res.us_hmac_priv_size);

}


OAL_STATIC OAL_INLINE oal_void*  mac_res_get_dmac_vap(oal_uint8 uc_idx)
{
    return mac_res_get_mac_vap(uc_idx);
}


OAL_STATIC OAL_INLINE oal_uint32  mac_res_alloc_hmac_user(oal_uint16 *pus_idx, oal_uint16 us_hmac_priv_size)
{
    oal_uint  ul_idx_temp;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pus_idx))
    {
        return OAL_FAIL;
    }

    ul_idx_temp = (oal_uint)oal_queue_dequeue_16(&(g_pst_mac_res->st_user_res.st_queue));

    /* 0为无效值 */
    if (0 == ul_idx_temp)
    {
        return OAL_FAIL;
    }

    *pus_idx = (oal_uint16)(ul_idx_temp - 1);

    g_pst_mac_res->st_user_res.us_hmac_priv_size = us_hmac_priv_size;

    (g_pst_mac_res->st_user_res.puc_user_cnt[ul_idx_temp - 1])++;

    return OAL_SUCC;
}



OAL_STATIC OAL_INLINE oal_void*  _mac_res_get_hmac_user(oal_uint16 us_idx)
{
    if (OAL_UNLIKELY(us_idx >= MAC_RES_MAX_USER_LIMIT))
    {
        return OAL_PTR_NULL;
    }

    return (oal_void *)(g_pst_mac_res->st_user_res.past_user_info[us_idx]);
}


OAL_STATIC OAL_INLINE oal_void*  _mac_res_get_mac_user(oal_uint16 us_idx)
{
    if (OAL_UNLIKELY(us_idx >= MAC_RES_MAX_USER_LIMIT))
    {
        return OAL_PTR_NULL;
    }

    /* 这里偏移内存空间 */
    return (oal_void *)((oal_uint8 *)(g_pst_mac_res->st_user_res.past_user_info[us_idx])
                        + g_pst_mac_res->st_user_res.us_hmac_priv_size);
}

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of mac_resource.h */
