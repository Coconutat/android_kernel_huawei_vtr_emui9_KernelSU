

#ifndef __HMAC_11V_H__
#define __HMAC_11V_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "oal_ext_if.h"
#include "oal_mem.h"
#include "mac_vap.h"


#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_11V_H

#ifdef _PRE_WLAN_FEATURE_11V_ENABLE

/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* 目前管理帧申请内存800字节 帧自带最大长度19(frame boady)+50(url)+ N*(15(neighbor)+3(sub)+12(sub))  不存在超出 修改数量限制时须注意 */
#define HMAC_MAX_BSS_NEIGHBOR_LIST  20          /* BSS Transition 帧中允许发送的最大候选AP列表集数量 */

// 11v等待帧返回的超时时间
#define HMAC_11V_WAIT_STATUS_TIMEOUT        5000         // 5000ms
#define HMAC_11V_MAX_URL_LENGTH             50          /* 携带URL字符的最大长度限制为50 */
#define HMAC_11V_TERMINATION_TSF_LENGTH     8           /* termination_tsf时间域字节长度 */
#define HMAC_11V_QUERY_FRAME_BODY_FIX       4           /* query帧帧体固定头长度 */
#define HMAC_11V_REQUEST_FRAME_BODY_FIX     7           /* query帧帧体固定头长度 */
#define HMAC_11V_RESPONSE_FRAME_BODY_FIX    5           /* response帧帧体固定头长度 */
#define HMAC_11V_PERFERMANCE_ELEMENT_LEN    1           /* perfermance ie length */
#define HMAC_11V_TERMINATION_ELEMENT_LEN    10          /* termination ie length */
#define HMAC_11V_TOKEN_MAX_VALUE            255         /* 帧发送信令的最大值 */
#define HMAC_11V_SUBELEMENT_ID_RESV         0           /* SUBELEMENT预留 ID*/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/* IEEE Std 802.11-2012 - Table 8-253 */
enum bss_trans_mgmt_status_code {
	WNM_BSS_TM_ACCEPT = 0,
	WNM_BSS_TM_REJECT_UNSPECIFIED = 1,
	WNM_BSS_TM_REJECT_INSUFFICIENT_BEACON = 2,
	WNM_BSS_TM_REJECT_INSUFFICIENT_CAPABITY = 3,
	WNM_BSS_TM_REJECT_UNDESIRED = 4,
	WNM_BSS_TM_REJECT_DELAY_REQUEST = 5,
	WNM_BSS_TM_REJECT_STA_CANDIDATE_LIST_PROVIDED = 6,
	WNM_BSS_TM_REJECT_NO_SUITABLE_CANDIDATES = 7,
	WNM_BSS_TM_REJECT_LEAVING_ESS = 8
};

/*****************************************************************************
  Neighbor Report Element子信息元素(Subelement)的 ID
  820.11-2012协议583页，Table 8-115—SubElement IDs
*****************************************************************************/
typedef enum
{
    HMAC_NEIGH_SUB_ID_RESERVED              = 0,
    HMAC_NEIGH_SUB_ID_TFS_INFO,
    HMAC_NEIGH_SUB_ID_COND_COUNTRY,
    HMAC_NEIGH_SUB_ID_BSS_CANDIDATE_PERF,
    HMAC_NEIGH_SUB_ID_TERM_DURATION,

    HMAC_NEIGH_SUB_ID_BUTT
}hmac_neighbor_sub_eid_enum;
typedef oal_uint8 hmac_neighbor_sub_eid_enum_uint8;


/*****************************************************************************
  4 STRUCT定义
*****************************************************************************/
/* Subelement BSS Transition Termination Duration */
struct hmac_bss_term_duration
{
    oal_uint8       uc_sub_ie_id;                                               /* subelement ID，将ID置成0 表示不存在该元素 */
    oal_uint8       auc_termination_tsf[HMAC_11V_TERMINATION_TSF_LENGTH];       /* BSS终止时间: TSF */
    oal_uint16      us_duration_min;                                            /* BSS消失时间 time: 分钟 */
    oal_uint8       uc_resv;                                                    /* 四字节对齐用 */
}__OAL_DECLARE_PACKED;
typedef struct hmac_bss_term_duration  hmac_bss_term_duration_stru;

/* 候选BSS列表集的Neighbor Report IE结构体 由于只需要用到subelement 3 4 故定义两个subelement */
struct hmac_neighbor_bss_info
{
    oal_uint8       auc_mac_addr[WLAN_MAC_ADDR_LEN];                      /* BSSID MAC地址 */
    oal_uint8       uc_opt_class;                                         /* Operation Class */
    oal_uint8       uc_chl_num;                                           /* Channel number */
    oal_uint8       uc_phy_type;                                          /* PHY type */
    oal_uint8       uc_candidate_perf;                                    /* perference data BSSID偏好值 */
    oal_uint16      us_resv;                                              /* 四字节对齐 */
    oal_bssid_infomation_stru   st_bssid_info;                       /* BSSID information */
    hmac_bss_term_duration_stru st_term_duration;                   /* 子元素Termination duration */
}__OAL_DECLARE_PACKED;
typedef struct hmac_neighbor_bss_info  hmac_neighbor_bss_info_stru;

struct hmac_bsst_req_mode
{
    oal_uint8   bit_candidate_list_include:1,                   /* 是否包含邻居AP列表 */
                bit_abridged:1,                                 /* 1:表示没有包含在邻居列表的AP优先级设置为0 */
                bit_bss_disassoc_imminent:1,                    /* 是否即将断开STA 置0 帧的时间域全为FF FF */
                bit_termination_include:1,                      /* BSS终结时间  置0则帧体中不包含BSS Termination Duration字段 */
                bit_ess_disassoc_imminent:1,                    /* EES终止时间 */
                bit_rev:3;
}__OAL_DECLARE_PACKED;
typedef struct hmac_bsst_req_mode  hmac_bsst_req_mode_stru;

/* bss transition request帧体信息结构体 */
struct hmac_bsst_req_info
{
    oal_uint8               uc_validity_interval;                              /* 邻居列表有效时间 TBTTs */
    oal_uint16              us_disassoc_time;                                  /* AP去关联STA时间 TBTTs */
    oal_uint8               *puc_session_url;                                  /* 要求传入字符串 最大限制为100个字符 */
    hmac_bsst_req_mode_stru st_request_mode;                                   /* request mode */
    oal_uint8               uc_bss_list_num;
    oal_uint8               uc_resv;                                           /* 4字节对齐 */
    hmac_neighbor_bss_info_stru *pst_neighbor_bss_list;                        /* bss list的数量 最大不允许超过50个 */
    hmac_bss_term_duration_stru st_term_duration;                              /* 子元素Termination duration */
}__OAL_DECLARE_PACKED;
typedef struct hmac_bsst_req_info  hmac_bsst_req_info_stru;

/* bss transition response帧体信息结构体 */
struct hmac_bsst_rsp_info
{
    oal_uint8       uc_status_code;                         /* 状态码 接收或者拒绝 */
    oal_uint8       uc_termination_delay;                   /* 要求AP延后终止时间:分钟 */
    oal_uint8       auc_target_bss_addr[WLAN_MAC_ADDR_LEN]; /* 接收切换的目标BSSID */
    oal_uint8       uc_chl_num;                             /* Channel number */
    oal_uint8       uc_bss_list_num;                        /* bss list的数量最大限制为50个 */
    oal_int8        c_rssi;                                 /* 当前关联AP的RSSI，非Candiate AP的RSSI */
    oal_uint8       us_resv;                                /* 四字节对齐 */
    hmac_neighbor_bss_info_stru *pst_neighbor_bss_list;
}__OAL_DECLARE_PACKED;
typedef struct hmac_bsst_rsp_info  hmac_bsst_rsp_info_stru;

/*****************************************************************************
  5 全局变量声明
*****************************************************************************/


/*****************************************************************************
  6 消息头定义
*****************************************************************************/


/*****************************************************************************
  7 消息定义
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
/* 将Neighbor Report IE结构体从帧数据中解析出来 */
extern hmac_neighbor_bss_info_stru * hmac_get_neighbor_ie(oal_uint8 *puc_data, oal_uint16 us_len, oal_uint8 *pst_bss_num);
/* STA 处理AP发送的bss transition request frame */
extern oal_uint32  hmac_rx_bsst_req_action(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf);
/* STA 封装bss transition response frame */
extern oal_uint16  hmac_encap_bsst_rsp_action(hmac_vap_stru *pst_hmac_vap,
                                              hmac_user_stru *pst_hmac_user,
                                              hmac_bsst_rsp_info_stru *pst_bsst_rsp_info,
                                              oal_netbuf_stru *pst_buffer);
/* STA 发送bss transition response frame */
//extern oal_uint32  hmac_tx_bsst_rsp_action(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, hmac_bsst_rsp_info_stru *pst_bsst_rsp_info);
extern oal_uint32 hmac_tx_bsst_rsp_action(void *pst_void1, void *pst_void2, void *pst_void3);

extern oal_uint32 hmac_11v_roam_scan_check(hmac_vap_stru *pst_hmac_vap);

#endif /*_PRE_WLAN_FEATURE_11V_ENABLE*/

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_11v.h */
