

#ifndef __HMAC_PROXYSTA_H__
#define __HMAC_PROXYSTA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#ifdef _PRE_WLAN_FEATURE_PROXYSTA
#include "oal_ext_if.h"
#include "frw_ext_if.h"
#include "mac_vap.h"
#include "hmac_mgmt_sta.h"

#define VAMP_MAX_RSP_SIZE   64

/*
 * driver向vamp上报的消息类型
 */
typedef enum
{
    VAMP_RESP_TYPE_FDB_CHANGE,
}vamp_resp_enum;

typedef oal_uint8 vamp_resp_enum_uint8;

/*
 * driver向vamp上报的消息结构体
 */
typedef struct
{
    vamp_resp_enum_uint8    en_type;    // 类型
    oal_uint8               uc_size;    // 长度，不包括type与size
    oal_uint8               auc_payload[VAMP_MAX_RSP_SIZE-2];   // payload
}__OAL_DECLARE_PACKED vamp_nlc_msg_stru;

extern oal_uint32  hmac_psta_mgr_init(oal_void);
extern oal_uint32  hmac_psta_mgr_exit(oal_void);

extern oal_uint32  hmac_psta_init_vap(hmac_vap_stru *pst_hmac_vap, mac_cfg_add_vap_param_stru *pst_param);
extern oal_uint32  hmac_psta_del_vap(hmac_vap_stru *pst_mac_vap);
extern oal_uint32  hmac_psta_add_vap(hmac_vap_stru *pst_mac_vap);

extern oal_uint32  hmac_psta_tx_process(oal_netbuf_stru *pst_buf, mac_vap_stru **ppst_vap);
extern oal_uint32  hmac_psta_rx_process(oal_netbuf_stru *pst_buf, hmac_vap_stru *pst_hmac_vap);

extern oal_uint32  hmac_psta_rx_mat(oal_netbuf_stru *pst_buf, hmac_vap_stru *pst_hmac_vap);
extern oal_uint32  hmac_psta_tx_mat(oal_netbuf_stru *pst_buf, hmac_vap_stru *pst_hmac_vap);

extern oal_uint32  hmac_psta_proc_wait_join(hmac_vap_stru *pst_hmac_sta, hmac_join_req_stru *pst_join_req);
extern oal_uint32  hmac_psta_proc_join_result(hmac_vap_stru *pst_hmac_sta, oal_bool_enum_uint8 en_succ);

extern hmac_vap_stru *hmac_psta_get_msta(hmac_vap_stru *pst_hmac_vap);
extern hmac_psta_rep_stru *hmac_psta_get_rep(hmac_vap_stru *pst_hmac_vap);
#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
#endif
#endif /* end of hmac_proxysta.h */
