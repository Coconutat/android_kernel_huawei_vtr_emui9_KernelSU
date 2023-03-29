

#ifndef __HMAC_TX_DATA_H__
#define __HMAC_TX_DATA_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "mac_frame.h"
#include "dmac_ext_if.h"
#include "hmac_vap.h"
#include "hmac_user.h"
#include "hmac_main.h"
#include "hmac_mgmt_classifier.h"
#include "mac_resource.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_DATA_H


/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* 基本能力信息中关于是否是QOS的能力位 */
#define HMAC_CAP_INFO_QOS_MASK 0x0200

#define WLAN_TOS_TO_TID(_tos) (      \
        (((_tos) == 0) || ((_tos) == 3)) ? WLAN_TIDNO_BEST_EFFORT: \
        (((_tos) == 1) || ((_tos) == 2)) ? WLAN_TIDNO_BACKGROUND: \
        (((_tos) == 4) || ((_tos) == 5)) ? WLAN_TIDNO_VIDEO: \
        WLAN_TIDNO_VOICE)

#define WLAN_BA_CNT_INTERVAL 100

#if (_PRE_TARGET_PRODUCT_TYPE_ONT == _PRE_CONFIG_TARGET_PRODUCT)
#define HMAC_TXQUEUE_DROP_LIMIT_LOW  1400
#define HMAC_TXQUEUE_DROP_LIMIT_HIGH 1600
#else
#define HMAC_TXQUEUE_DROP_LIMIT_LOW  600
#define HMAC_TXQUEUE_DROP_LIMIT_HIGH 800
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
#define HMAC_TX_PKTS_STAT(_pkt)   (g_host_tx_pkts.ul_snd_pkts += (_pkt))
#endif

#define WLAN_AMPDU_THROUGHPUT_THRESHOLD_HIGH   300  /* 300Mb/s */
#define WLAN_AMPDU_THROUGHPUT_THRESHOLD_LOW    200  /* 200Mb/s */
#define WLAN_SMALL_AMSDU_THROUGHPUT_THRESHOLD_HIGH   300  /* 300Mb/s */
#define WLAN_SMALL_AMSDU_THROUGHPUT_THRESHOLD_LOW    200  /* 200Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH   90  /* 100Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW    30  /* 30Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_40M   300  /* 300Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_40M    150  /* 150Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_80M   550  /* 500Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_80M    450  /* 300Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_HIGH_160M  800  /* 500Mb/s */
#define WLAN_TCP_ACK_BUF_THROUGHPUT_THRESHOLD_LOW_160M   700  /* 300Mb/s */

#define WLAN_DYN_BYPASS_EXTLNA_THROUGHPUT_THRESHOLD_HIGH   100  /* 100Mb/s */
#define WLAN_DYN_BYPASS_EXTLNA_THROUGHPUT_THRESHOLD_LOW    50  /* 50Mb/s */
#define WLAN_SMALL_AMSDU_PPS_THRESHOLD_HIGH   25000  /* pps 25000 */
#define WLAN_SMALL_AMSDU_PPS_THRESHOLD_LOW    5000  /* pps 5000 */
#define WLAN_TCP_ACK_FILTER_THROUGHPUT_TH_HIGH  50
#define WLAN_TCP_ACK_FILTER_THROUGHPUT_TH_LOW   20

#define WLAN_AMPDU_HW_SWITCH_PERIOD     300 /* 1分钟 */
/*****************************************************************************
  3 枚举定义
*****************************************************************************/
typedef enum
{
    HMAC_TX_BSS_NOQOS = 0,
    HMAC_TX_BSS_QOS   = 1,

    HMAC_TX_BSS_QOS_BUTT
}hmac_tx_bss_qos_type_enum;

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
extern oal_uint8  g_uc_tx_ba_policy_select;
#endif
/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
typedef struct
{
    oal_uint32          ul_pkt_len;  /* HOST来帧量统计 */
    oal_uint32          ul_snd_pkts;  /* 驱动实际发送帧统计 */
    oal_uint32          ul_start_time; /* 均速统计开始时间 */
}hmac_tx_pkts_stat_stru;
extern hmac_tx_pkts_stat_stru g_host_tx_pkts;
#endif

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
typedef struct
{
    /* 定制化硬件聚合是否生效,默认为软件聚合 */
    oal_uint8    uc_ampdu_hw_en;
    /* 当前聚合是硬件聚合还是软件聚合 */
    oal_uint8    uc_ampdu_hw_enable;
    oal_uint16   us_remain_hw_cnt;
    oal_uint16   us_throughput_high;
    oal_uint16   us_throughput_low;
}hmac_tx_ampdu_hw_stru;
extern hmac_tx_ampdu_hw_stru g_st_ampdu_hw;
#endif

/*****************************************************************************
  8 UNION定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern oal_uint32  hmac_tx_encap_etc(hmac_vap_stru    *pst_vap,
                                            hmac_user_stru   *pst_user,
                                            oal_netbuf_stru  *pst_buf);
extern oal_uint32 hmac_tx_ucast_process_etc( hmac_vap_stru   *pst_vap,
                                                oal_netbuf_stru *pst_buf,
                                                hmac_user_stru  *pst_user,
                                                mac_tx_ctl_stru    *pst_tx_ctl);

extern oal_void  hmac_tx_ba_setup_etc(
                hmac_vap_stru          *pst_vap,
                hmac_user_stru         *pst_user,
                oal_uint8               uc_tidno);

extern  oal_void hmac_tx_ba_cnt_vary_etc(
                                hmac_vap_stru   *pst_hmac_vap,
                                hmac_user_stru  *pst_hmac_user,
                                oal_uint8        uc_tidno,
                                oal_netbuf_stru *pst_buf);

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)
extern  oal_uint8 hmac_tx_wmm_acm_etc(oal_bool_enum_uint8  en_wmm, hmac_vap_stru *pst_hmac_vap, oal_uint8 *puc_tid);
#endif /* defined(_PRE_PRODUCT_ID_HI110X_HOST) */
#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
extern oal_void  hmac_tx_ampdu_switch(oal_uint32 ul_tx_throughput_mbps);
#endif
extern oal_void hmac_rx_dyn_bypass_extlna_switch(oal_uint32 ul_tx_throughput_mbps, oal_uint32 ul_rx_throughput_mbps);

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU
extern oal_void hmac_tx_amsdu_ampdu_switch(oal_uint32 ul_tx_throughput_mbps);
#endif
#ifdef _PRE_WLAN_TCP_OPT
extern oal_void hmac_tcp_ack_filter_switch(oal_uint32 ul_rx_throughput_mbps);
#endif

extern oal_void hmac_tx_small_amsdu_switch(oal_uint32 ul_rx_throughput_mbps,oal_uint32 ul_pps);

#ifdef _PRE_WLAN_FEATURE_TCP_ACK_BUFFER
extern oal_void hmac_tx_tcp_ack_buf_switch(oal_uint32 ul_rx_throughput_mbps);
#endif
/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


OAL_STATIC OAL_INLINE oal_netbuf_stru *hmac_tx_get_next_mpdu(oal_netbuf_stru *pst_buf, oal_uint8 uc_netbuf_num)
{
    oal_netbuf_stru       *pst_next_buf = OAL_PTR_NULL;
    oal_uint32             ul_netbuf_index;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_buf))
    {
      //  OAM_ERROR_LOG0(0, OAM_SF_TX, "{hmac_tx_get_next_mpdu::pst_buf null}");
        return OAL_PTR_NULL;
    }

    pst_next_buf = pst_buf;
    for (ul_netbuf_index = 0; ul_netbuf_index < uc_netbuf_num; ul_netbuf_index++)
    {
        pst_next_buf = oal_netbuf_list_next(pst_next_buf);
    }

    return pst_next_buf;
}


OAL_STATIC OAL_INLINE oal_void hmac_tx_netbuf_list_enqueue(oal_netbuf_head_stru *pst_head, oal_netbuf_stru *pst_buf, oal_uint8 uc_netbuf_num)
{
    oal_uint32             ul_netbuf_index;
    oal_netbuf_stru       *pst_buf_next;

    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_head) || (OAL_PTR_NULL == pst_buf)))
    {
      //  OAM_ERROR_LOG2(0, OAM_SF_TX, "{hmac_tx_get_next_mpdu::input null %x %x}", pst_head, pst_buf);
        return;
    }

    for (ul_netbuf_index = 0; ul_netbuf_index < uc_netbuf_num; ul_netbuf_index++)
    {
        pst_buf_next = oal_netbuf_list_next(pst_buf);
        oal_netbuf_add_to_list_tail(pst_buf, pst_head);
        pst_buf = pst_buf_next;
    }
}



OAL_STATIC OAL_INLINE oal_void hmac_tx_get_addr(mac_ieee80211_qos_htc_frame_addr4_stru *pst_hdr,
                                                oal_uint8                               *puc_saddr,
                                                oal_uint8                               *puc_daddr)
{
    oal_uint8  uc_to_ds;
    oal_uint8  uc_from_ds;

    uc_to_ds   = mac_hdr_get_to_ds((oal_uint8 *)pst_hdr);
    uc_from_ds = mac_hdr_get_from_ds((oal_uint8 *)pst_hdr);

    if ((1 == uc_to_ds) && (0 == uc_from_ds))
    {
        /* to AP */
        oal_set_mac_addr(puc_saddr, pst_hdr->auc_address2);
        oal_set_mac_addr(puc_daddr, pst_hdr->auc_address3);
    }
    else if ((0 == uc_to_ds) && (0 == uc_from_ds))
    {
        /* IBSS */
        oal_set_mac_addr(puc_saddr, pst_hdr->auc_address2);
        oal_set_mac_addr(puc_daddr, pst_hdr->auc_address1);
    }
    else if ((1 == uc_to_ds) && (1 == uc_from_ds))
    {
        /* WDS */
        oal_set_mac_addr(puc_saddr, pst_hdr->auc_address4);
        oal_set_mac_addr(puc_daddr, pst_hdr->auc_address3);
    }
    else
    {
        /* From AP */
        oal_set_mac_addr(puc_saddr, pst_hdr->auc_address3);
        oal_set_mac_addr(puc_daddr, pst_hdr->auc_address1);
    }
}


OAL_STATIC OAL_INLINE oal_void hmac_tx_set_frame_ctrl(oal_uint32                               ul_qos,
                                                      mac_tx_ctl_stru                         *pst_tx_ctl,
                                                      mac_ieee80211_qos_htc_frame_addr4_stru  *pst_hdr_addr4)
{
    mac_ieee80211_qos_htc_frame_stru *pst_hdr = OAL_PTR_NULL;
    if (HMAC_TX_BSS_QOS == ul_qos)
    {
        /* 设置帧控制字段 */
        mac_hdr_set_frame_control((oal_uint8 *)pst_hdr_addr4, (WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_QOS));

        /* 更新帧头长度 */
        if (OAL_FALSE == MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl))
        {
            pst_hdr = (mac_ieee80211_qos_htc_frame_stru *)pst_hdr_addr4;
            /* 设置QOS控制字段 */
            pst_hdr->bit_qc_tid        = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);
            pst_hdr->bit_qc_eosp       = 0;
            pst_hdr->bit_qc_ack_polocy = MAC_GET_CB_ACK_POLACY(pst_tx_ctl);
            pst_hdr->bit_qc_amsdu      = MAC_GET_CB_IS_AMSDU(pst_tx_ctl);
            pst_hdr->qos_control.bit_qc_txop_limit = 0;
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_FRAME_LEN;
        }
        else
        {
            /* 设置QOS控制字段 */
            pst_hdr_addr4->bit_qc_tid        = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);
            pst_hdr_addr4->bit_qc_eosp       = 0;
            pst_hdr_addr4->bit_qc_ack_polocy = MAC_GET_CB_ACK_POLACY(pst_tx_ctl);
            pst_hdr_addr4->bit_qc_amsdu      = MAC_GET_CB_IS_AMSDU(pst_tx_ctl);
            pst_hdr_addr4->qos_control.qc_txop_limit = 0;
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_QOS_4ADDR_FRAME_LEN;
        }

        /* 由DMAC考虑是否需要HTC */
    }
    else
    {
        /* 设置帧控制字段 */
        mac_hdr_set_frame_control((oal_uint8 *)pst_hdr_addr4, WLAN_FC0_TYPE_DATA | WLAN_FC0_SUBTYPE_DATA);

        /* 非QOS数据帧帧控制字段设置 */
        if (MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl))
        {
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_4ADDR_FRAME_LEN;
        }
        else
        {
            MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl) = MAC_80211_FRAME_LEN;
        }
    }
}


OAL_STATIC OAL_INLINE oal_uint32 hmac_tx_set_addresses(
                hmac_vap_stru                           *pst_vap,
                hmac_user_stru                          *pst_user,
                mac_tx_ctl_stru                         *pst_tx_ctl,
                mac_ether_header_stru                   *pst_ether_hdr,
                mac_ieee80211_qos_htc_frame_addr4_stru  *pst_hdr)
{
    /* 分片号置成0，后续分片特性需要重新赋值 */
    pst_hdr->bit_frag_num    = 0;
    pst_hdr->bit_seq_num     = 0;

    if ((WLAN_VAP_MODE_BSS_AP == pst_vap->st_vap_base_info.en_vap_mode)
          && (!(MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl)))) /* From AP */
    {
        /* From DS标识位设置 */
        mac_hdr_set_from_ds((oal_uint8 *)pst_hdr, 1);

        /* to DS标识位设置 */
        mac_hdr_set_to_ds((oal_uint8 *)pst_hdr, 0);

        /* Set Address1 field in the WLAN Header with destination address */
        oal_set_mac_addr(pst_hdr->auc_address1, (oal_uint8*)pst_ether_hdr->auc_ether_dhost);

        /* Set Address2 field in the WLAN Header with the BSSID */
        oal_set_mac_addr(pst_hdr->auc_address2, pst_vap->st_vap_base_info.auc_bssid);

        if (MAC_GET_CB_IS_AMSDU(pst_tx_ctl)) /* AMSDU情况，地址3填写BSSID */
        {
            /* Set Address3 field in the WLAN Header with the BSSID */
            oal_set_mac_addr(pst_hdr->auc_address3, pst_vap->st_vap_base_info.auc_bssid);
        }
        else
        {
            /* Set Address3 field in the WLAN Header with the source address */
            oal_set_mac_addr(pst_hdr->auc_address3, (oal_uint8*)pst_ether_hdr->auc_ether_shost);
        }

    }
    else if ((WLAN_VAP_MODE_BSS_STA == pst_vap->st_vap_base_info.en_vap_mode)
          && (!(MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl))))
    {
        /* From DS标识位设置 */
        mac_hdr_set_from_ds((oal_uint8 *)pst_hdr, 0);

        /* to DS标识位设置 */
        mac_hdr_set_to_ds((oal_uint8 *)pst_hdr, 1);

        /* Set Address1 field in the WLAN Header with BSSID */
        oal_set_mac_addr(pst_hdr->auc_address1, pst_user->st_user_base_info.auc_user_mac_addr);

        if (pst_ether_hdr->us_ether_type == oal_byteorder_host_to_net_uint16(ETHER_LLTD_TYPE))
        {
            /* Set Address2 field in the WLAN Header with the source address */
            oal_set_mac_addr(pst_hdr->auc_address2, (oal_uint8*)pst_ether_hdr->auc_ether_shost);
        }
        else
        {
            /* Set Address2 field in the WLAN Header with the source address */
            oal_set_mac_addr(pst_hdr->auc_address2, mac_mib_get_StationID(&pst_vap->st_vap_base_info));
        }

        if (MAC_GET_CB_IS_AMSDU(pst_tx_ctl)) /* AMSDU情况，地址3填写BSSID */
        {
            /* Set Address3 field in the WLAN Header with the BSSID */
            oal_set_mac_addr(pst_hdr->auc_address3, pst_user->st_user_base_info.auc_user_mac_addr);
        }
        else
        {
            /* Set Address3 field in the WLAN Header with the destination address */
            oal_set_mac_addr(pst_hdr->auc_address3, (oal_uint8*)pst_ether_hdr->auc_ether_dhost);
        }
    }
    else if (MAC_GET_CB_IS_4ADDRESS(pst_tx_ctl))
    /* WDS */
    {
        /* TO DS标识位设置 */
        mac_hdr_set_to_ds((oal_uint8 *)pst_hdr, 1);

        /* From DS标识位设置 */
        mac_hdr_set_from_ds((oal_uint8 *)pst_hdr, 1);

        /* 地址1是 RA */
        oal_set_mac_addr(pst_hdr->auc_address1, pst_user->st_user_base_info.auc_user_mac_addr);

        /* 地址2是 TA (当前只有BSSID) */
        oal_set_mac_addr(pst_hdr->auc_address2, mac_mib_get_StationID(&pst_vap->st_vap_base_info));

        if (MAC_GET_CB_IS_AMSDU(pst_tx_ctl)) /* AMSDU情况，地址3和地址4填写BSSID */
        {
            if (WLAN_VAP_MODE_BSS_STA == pst_vap->st_vap_base_info.en_vap_mode)
            {
                /* 地址3是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address3, pst_user->st_user_base_info.auc_user_mac_addr);

                /* 地址4也是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address4, pst_user->st_user_base_info.auc_user_mac_addr);
            }
            else if (WLAN_VAP_MODE_BSS_AP == pst_vap->st_vap_base_info.en_vap_mode)
            {
                /* 地址3是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address3, mac_mib_get_StationID(&pst_vap->st_vap_base_info));

                /* 地址4也是 BSSID */
                oal_set_mac_addr(pst_hdr->auc_address4, mac_mib_get_StationID(&pst_vap->st_vap_base_info));
            }
        }
        else
        {
            /* 地址3是 DA */
            oal_set_mac_addr(pst_hdr->auc_address3, (oal_uint8*)pst_ether_hdr->auc_ether_dhost);

            /* 地址4是 SA */
            oal_set_mac_addr(pst_hdr->auc_address4, (oal_uint8*)pst_ether_hdr->auc_ether_shost);
        }

    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tid_ba_is_setup(hmac_user_stru *pst_hmac_user, oal_uint8 uc_tidno)
{
   if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_user) || uc_tidno >= WLAN_TID_MAX_NUM)
   {
       return OAL_FALSE;
   }
   return (DMAC_BA_COMPLETE == pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status) ? OAL_TRUE : OAL_FALSE;
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_vap_ba_is_setup(hmac_user_stru *pst_hmac_user)
{
    oal_uint8 uc_tidno;

   if (OAL_PTR_NULL == pst_hmac_user)
   {
       return OAL_FALSE;
   }

   for (uc_tidno = 0; uc_tidno < WLAN_TID_MAX_NUM; uc_tidno++)
   {
      if (DMAC_BA_COMPLETE == pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status)
      {
          return OAL_TRUE;
      }
   }

   return  OAL_FALSE;
}

OAL_STATIC OAL_INLINE oal_void hmac_tx_ba_del(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 uc_tidno)
{
    mac_action_mgmt_args_stru       st_action_args;   /* 用于填写ACTION帧的参数 */

    st_action_args.uc_category = MAC_ACTION_CATEGORY_BA;
    st_action_args.uc_action   = MAC_BA_ACTION_DELBA;
    st_action_args.ul_arg1     = uc_tidno;
    st_action_args.ul_arg2     = MAC_ORIGINATOR_DELBA;
    st_action_args.ul_arg3     = MAC_UNSPEC_REASON;
    st_action_args.puc_arg5    = pst_hmac_user->st_user_base_info.auc_user_mac_addr;
    hmac_mgmt_tx_action_etc(pst_hmac_vap,  pst_hmac_user, &st_action_args);

}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tid_need_ba_session(
                                    hmac_vap_stru   *pst_hmac_vap,
                                    hmac_user_stru  *pst_hmac_user,
                                    oal_uint8        uc_tidno,
                                    oal_netbuf_stru *pst_buf)
{
    hmac_tid_stru         *pst_hmac_tid_info;
    wlan_ciper_protocol_type_enum_uint8 en_cipher_type;
    oal_bool_enum_uint8    en_ampdu_support;
#ifndef _PRE_WLAN_FEATURE_AMPDU_VAP
    mac_device_stru       *pst_mac_device;
#endif

    /* 该tid下不允许建BA，配置命令需求 */
    if (OAL_FALSE == pst_hmac_user->ast_tid_info[uc_tidno].en_ba_handle_tx_enable)
    {
        return OAL_FALSE;
    }

    if(OAL_TRUE == hmac_tid_ba_is_setup(pst_hmac_user, uc_tidno))
    {
        if (OAL_TRUE == mac_mib_get_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info))
        {
            return OAL_FALSE;
        }

#ifdef _PRE_WLAN_FEATURE_AMPDU_TX_HW
        /* 判断需要删除BA,配置命令处可能已经删除BA,因此需要再次判断删除BA条件 */
        oal_spin_lock_bh(&pst_hmac_vap->st_ampdu_lock);
        if (OAL_FALSE == mac_mib_get_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info) &&
            (OAL_TRUE == hmac_tid_ba_is_setup(pst_hmac_user, uc_tidno)))
        {
            hmac_tx_ba_del(pst_hmac_vap, pst_hmac_user, uc_tidno);
        }

        oal_spin_unlock_bh(&pst_hmac_vap->st_ampdu_lock);
#else
        hmac_tx_ba_del(pst_hmac_vap, pst_hmac_user, uc_tidno);
#endif

    }

    /* 配置命令不允许建立聚合时返回 */
    if (OAL_FALSE == mac_mib_get_CfgAmpduTxAtive(&pst_hmac_vap->st_vap_base_info))
    {
        return OAL_FALSE;
    }

    /* 建立BA会话，需要判断VAP的AMPDU的支持情况，因为需要实现建立BA会话时，一定发AMPDU */
    if(OAL_FALSE == hmac_user_xht_support(pst_hmac_user))
    {
        if (DMAC_BA_INIT != pst_hmac_user->ast_tid_info[uc_tidno].st_ba_tx_info.en_ba_status)
        {
            hmac_tx_ba_del(pst_hmac_vap, pst_hmac_user, uc_tidno);
        }

        return OAL_FALSE;
    }

    /* wep/tkip不支持11n及以上协议 */
    en_cipher_type = pst_hmac_user->st_user_base_info.st_key_info.en_cipher_type;
    en_ampdu_support = ((WLAN_80211_CIPHER_SUITE_NO_ENCRYP == en_cipher_type) ||
                        (WLAN_80211_CIPHER_SUITE_CCMP == en_cipher_type) ||
                        (WLAN_80211_CIPHER_SUITE_CCMP_256 == en_cipher_type) ||
                        (WLAN_80211_CIPHER_SUITE_GCMP == en_cipher_type) ||
                        (WLAN_80211_CIPHER_SUITE_GCMP_256 == en_cipher_type)) ? OAL_TRUE : OAL_FALSE;
    if (OAL_FALSE == en_ampdu_support)
    {
        return OAL_FALSE;
    }

    if(WLAN_ADDBA_MODE_AUTO != mac_mib_get_AddBaMode(&pst_hmac_vap->st_vap_base_info))
    {
        return OAL_FALSE;
    }

    /* 针对VO业务, 根据VAP标志位确定是否建立BA会话 */
    if ((WLAN_WME_AC_VO == WLAN_WME_TID_TO_AC(uc_tidno)) && (OAL_FALSE == pst_hmac_vap->st_vap_base_info.bit_voice_aggr))
    {
        return OAL_FALSE;
    }

    /* 判断HMAC VAP的是否支持聚合 */
    if (!(mac_mib_get_TxAggregateActived(&pst_hmac_vap->st_vap_base_info) || (pst_hmac_vap->st_vap_base_info.st_cap_flag.bit_rifs_tx_on)))
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_tid_need_ba_session::en_tx_aggr_on of vap is off");
        return OAL_FALSE;
    }

#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
    if (mac_mib_get_TxBASessionNumber(&pst_hmac_vap->st_vap_base_info) >= WLAN_MAX_TX_BA)
    {
        OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_tid_need_ba_session::uc_tx_ba_session_num[%d] exceed spec", mac_mib_get_TxBASessionNumber(&pst_hmac_vap->st_vap_base_info));
        return OAL_FALSE;
    }
#else
    pst_mac_device = mac_res_get_dev_etc(pst_hmac_vap->st_vap_base_info.uc_device_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_mac_device))
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_tid_need_ba_session::pst_mac_dev null");
        return OAL_FALSE;
    }

    if (pst_mac_device->uc_tx_ba_session_num >= WLAN_MAX_TX_BA)
    {
        OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_tid_need_ba_session::uc_tx_ba_session_num[%d] exceed spec", pst_mac_device->uc_tx_ba_session_num);
        return OAL_FALSE;
    }
#endif

    /* 需要先发送5个单播帧，再进行BA会话的建立 */
    if ((OAL_TRUE == pst_hmac_user->st_user_base_info.st_cap_info.bit_qos) &&
        (pst_hmac_user->auc_ba_flag[uc_tidno] < DMAC_UCAST_FRAME_TX_COMP_TIMES))
    {
        OAM_INFO_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                      "{hmac_tid_need_ba_session::auc_ba_flag[%d]}", pst_hmac_user->auc_ba_flag[uc_tidno]);
        hmac_tx_ba_cnt_vary_etc(pst_hmac_vap, pst_hmac_user, uc_tidno, pst_buf);
        return OAL_FALSE;
    }
    /* 针对关闭WMM，非QOS帧处理 */
    else if(OAL_FALSE == pst_hmac_user->st_user_base_info.st_cap_info.bit_qos)
    {
        OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,"{UnQos Frame pass!!}");
        return OAL_FALSE;
    }

    pst_hmac_tid_info = &(pst_hmac_user->ast_tid_info[uc_tidno]);
    if ((pst_hmac_tid_info->st_ba_tx_info.en_ba_status == DMAC_BA_INIT)
     && (pst_hmac_tid_info->st_ba_tx_info.uc_addba_attemps < HMAC_ADDBA_EXCHANGE_ATTEMPTS))
    {
        pst_hmac_tid_info->st_ba_tx_info.en_ba_status = DMAC_BA_INPROGRESS;
        pst_hmac_tid_info->st_ba_tx_info.uc_addba_attemps++;
    }
    else
    {
        OAM_INFO_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                     "{hmac_tid_need_ba_session::addba_attemps[%d] of tid[%d] is COMPLETE}", pst_hmac_tid_info->st_ba_tx_info.uc_addba_attemps, uc_tidno);
        return OAL_FALSE;
    }

    return OAL_TRUE;
}


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hmac_tx_data.h */
