


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_tx_amsdu.h"
#include "hmac_tx_data.h"



#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_TX_AMSDU_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
mac_llc_snap_stru    g_st_mac_11c_snap_header = {
                            SNAP_LLC_LSAP,
                            SNAP_LLC_LSAP,
                            LLC_UI,
                            {
                                SNAP_RFC1042_ORGCODE_0,
                                SNAP_RFC1042_ORGCODE_1,
                                SNAP_RFC1042_ORGCODE_2,
                            },
                            0};
#ifdef _PRE_WLAN_FEATURE_DUAL_BAND_PERF_OPT
extern oal_bool_enum_uint8 g_en_2g_tx_amsdu;
#endif

OAL_STATIC oal_uint32  hmac_amsdu_tx_timeout_process(oal_void *p_arg);
OAL_STATIC oal_bool_enum_uint8 hmac_tx_amsdu_is_overflow(
                hmac_amsdu_stru    *pst_amsdu,
                mac_tx_ctl_stru    *pst_tx_ctl,
                oal_uint32          ul_frame_len,
                hmac_user_stru     *pst_user);
OAL_STATIC oal_uint32  hmac_amsdu_send(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, hmac_amsdu_stru *pst_amsdu);

/*****************************************************************************
  3 函数实现
*****************************************************************************/

OAL_STATIC oal_void hmac_amsdu_prepare_to_send(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, hmac_amsdu_stru *pst_amsdu)
{
    oal_uint32              ul_ret;

    /* 删除定时器 */
    FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_amsdu->st_amsdu_timer);
    OAM_INFO_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_prepare_to_send::amsdu size[%d],max masdu size[%d],msdu num[%d],max msdu num[%d].}",
                                pst_amsdu->us_amsdu_size,pst_amsdu->us_amsdu_maxsize, pst_amsdu->uc_msdu_num, pst_amsdu->uc_amsdu_maxnum);

    ul_ret = hmac_amsdu_send(pst_vap, pst_user, pst_amsdu);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU,"{hmac_amsdu_prepare_to_send::, amsdu send fails. erro[%d], short_pkt_num=%d.}", ul_ret, pst_amsdu->uc_msdu_num);
    }
}

#if defined(_PRE_PRODUCT_ID_HI110X_HOST)

OAL_STATIC OAL_INLINE oal_bool_enum_uint8 hmac_tx_amsdu_is_overflow(
                hmac_amsdu_stru    *pst_amsdu,
                mac_tx_ctl_stru    *pst_tx_ctl,
                oal_uint32          ul_frame_len,
                hmac_user_stru     *pst_user)
{
    mac_tx_ctl_stru     *pst_head_ctl;
    oal_netbuf_stru     *pst_head_buf;

    /* msdu链表中无msdu */
    pst_head_buf = oal_netbuf_peek(&pst_amsdu->st_msdu_head);
    if (OAL_PTR_NULL == pst_head_buf)
    {
        OAM_INFO_LOG0(1, OAM_SF_TX, "{hmac_tx_amsdu_is_overflow:: The first msdu.}");
        return OAL_FALSE;
    }

    pst_head_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_head_buf);
    /* amsdu不为空，并且amsdu中的子帧来源(lan或者wlan)与当前要封装的netbuf不同，则将amsdu发送出去，
       这样做是因为在发送完成中释放一个mpdu时，是根据第一个netbuf的cb中填写的事件类型来选择释放策略，
       如果一个mpdu中的netbuf来源不同，会造成内存泄漏 */
    if (MAC_GET_CB_EVENT_TYPE(pst_tx_ctl) != MAC_GET_CB_EVENT_TYPE(pst_head_ctl))
    {
        OAM_INFO_LOG2(1, OAM_SF_TX, "{hmac_tx_amsdu_is_overflow::en_event_type mismatched. %d %d.}",
                MAC_GET_CB_EVENT_TYPE(pst_tx_ctl), MAC_GET_CB_EVENT_TYPE(pst_head_ctl));
        return OAL_TRUE;
    }

    /* payload + padmax(3) 不能大于1568 */
    if (((pst_amsdu->us_amsdu_size + ul_frame_len + SNAP_LLC_FRAME_LEN + 3) > WLAN_LARGE_NETBUF_SIZE)
     || ((pst_amsdu->us_amsdu_size + ul_frame_len + SNAP_LLC_FRAME_LEN) > WLAN_AMSDU_FRAME_MAX_LEN))
    {

        OAM_INFO_LOG4(1, OAM_SF_TX, "{hmac_tx_amsdu_is_overflow::us_amsdu_size=%d us_amsdu_maxsize=%d framelen=%d uc_msdu_num=%d .}",
                               pst_amsdu->us_amsdu_size, pst_amsdu->us_amsdu_maxsize, ul_frame_len, pst_amsdu->uc_msdu_num);
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


OAL_STATIC oal_uint32  hmac_amsdu_send(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, hmac_amsdu_stru *pst_amsdu)
{
    frw_event_mem_stru *pst_amsdu_send_event_mem;
    frw_event_stru     *pst_amsdu_send_event;
    oal_uint32          ul_ret;
    mac_tx_ctl_stru    *pst_cb;
    oal_netbuf_stru    *pst_net_buf;
    dmac_tx_event_stru *pst_amsdu_event;

    /* 给dmac传送的amsdu相关的信息以及802.11头挂接 */
    pst_net_buf = oal_netbuf_delist(&(pst_amsdu->st_msdu_head));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_buf))
    {
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::pst_net_buf null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_net_buf);

    /* amsdu只聚合一个帧时，回退成非amsdu，统一encap接口 */
    if (1 == pst_amsdu->uc_msdu_num)
    {
        oal_memmove(OAL_NETBUF_DATA(pst_net_buf) + SNAP_LLC_FRAME_LEN, OAL_NETBUF_DATA(pst_net_buf), OAL_MAC_ADDR_LEN + OAL_MAC_ADDR_LEN);
        oal_netbuf_pull(pst_net_buf, SNAP_LLC_FRAME_LEN);

        MAC_GET_CB_IS_AMSDU(pst_cb) = OAL_FALSE;
        MAC_GET_CB_IS_FIRST_MSDU(pst_cb) = OAL_FALSE;
    }

    /* 把最后一个子帧的PAD去除 */
    oal_netbuf_trim(pst_net_buf, pst_amsdu->uc_last_pad_len);

    MAC_GET_CB_MPDU_LEN(pst_cb) = (oal_uint16)OAL_NETBUF_LEN(pst_net_buf);
    MAC_GET_CB_MPDU_NUM(pst_cb) = 1;

    /* 为整个amsdu封装802.11头 */
    ul_ret = hmac_tx_encap_etc(pst_vap, pst_user, pst_net_buf);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        oal_netbuf_free(pst_net_buf);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::hmac_tx_encap_etc failed[%d]}", ul_ret);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 抛事件 */
    pst_amsdu_send_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_amsdu_send_event_mem))
    {
        oal_netbuf_free(pst_net_buf);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::pst_amsdu_send_event_mem null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填事件头 */
    pst_amsdu_send_event = frw_get_event_stru(pst_amsdu_send_event_mem);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_amsdu_send_event))
    {
        oal_netbuf_free(pst_net_buf);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::pst_amsdu_send_event null}");
        FRW_EVENT_FREE(pst_amsdu_send_event_mem);
        return OAL_ERR_CODE_PTR_NULL;
    }

    FRW_EVENT_HDR_INIT(&(pst_amsdu_send_event->st_event_hdr),
                        FRW_EVENT_TYPE_HOST_DRX,
                        DMAC_TX_HOST_DRX,
                        OAL_SIZEOF(dmac_tx_event_stru),
                        FRW_EVENT_PIPELINE_STAGE_1,
                        pst_vap->st_vap_base_info.uc_chip_id,
                        pst_vap->st_vap_base_info.uc_device_id,
                        pst_vap->st_vap_base_info.uc_vap_id);

    pst_amsdu_event = (dmac_tx_event_stru *)(pst_amsdu_send_event->auc_event_data);
    pst_amsdu_event->pst_netbuf = pst_net_buf;

    ul_ret = frw_event_dispatch_event_etc(pst_amsdu_send_event_mem);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        oal_netbuf_free(pst_net_buf);
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "hmac_amsdu_send::frw_event_dispatch_event_etc fail[%d]", ul_ret);
    }

    /* 释放事件内存 */
    FRW_EVENT_FREE(pst_amsdu_send_event_mem);

    /* 清零amsdu结构体信息 */
    pst_amsdu->us_amsdu_size = 0;
    pst_amsdu->uc_msdu_num   = 0;

    return  ul_ret;
}


OAL_STATIC OAL_INLINE oal_uint32 hmac_amsdu_tx_encap_mpdu(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, hmac_amsdu_stru *pst_amsdu, oal_netbuf_stru *pst_buf)
{
    oal_uint32              ul_msdu_len;
    oal_uint32              ul_frame_len;
    oal_uint32              ul_tailroom;
    oal_uint16              us_msdu_offset;    /* 拷贝新msdu帧的偏移地址 */
    oal_netbuf_stru        *pst_dest_buf;
    mac_ether_header_stru  *pst_ether_head;    /* 以太网过来的skb的以太网头 */
    mac_llc_snap_stru      *pst_snap_head;     /* 为填写snap头的临时指针 */
    oal_uint8              *pst_msdu_payload;

    /* 协议栈来帧原始长 */
    ul_frame_len = oal_netbuf_get_len(pst_buf);
    /* 4字节对齐后的msdu帧的长度 */
    ul_msdu_len = OAL_ROUND_UP(ul_frame_len, 4);
    /* msdu帧长 */
    ul_msdu_len += SNAP_LLC_FRAME_LEN;

    pst_dest_buf = oal_netbuf_peek(&pst_amsdu->st_msdu_head);
    if (OAL_PTR_NULL == pst_dest_buf)
    {
        /* 链表中应该有netbuf */
        OAM_ERROR_LOG0(0,OAM_SF_AMSDU,"{hmac_amsdu_tx_encap_mpdu::oal_netbuf_peek return NULL}");
        return HMAC_TX_PASS;
    }

    /* 当期netbuf剩余空间少于msdu长 */
    ul_tailroom = oal_netbuf_tailroom(pst_dest_buf);
    if(ul_tailroom < ul_msdu_len)
    {
        OAM_ERROR_LOG3(0,OAM_SF_AMSDU,"{hmac_amsdu_tx_encap_mpdu::Notify1,tailroom[%d],msdu[%d],frame[%d]}",ul_tailroom, ul_msdu_len, ul_frame_len);
        /* 如果加上pad超出长度,尝试尾帧去掉pad */
        ul_msdu_len = ul_frame_len + SNAP_LLC_FRAME_LEN;
        if (ul_tailroom < ul_msdu_len)
        {
            hmac_amsdu_prepare_to_send(pst_vap, pst_user, pst_amsdu);
            return HMAC_TX_PASS;
        }
    }

    /* NEW MSDU OFFSET */
    us_msdu_offset = (oal_uint16)oal_netbuf_get_len(pst_dest_buf);
    OAM_INFO_LOG4(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_tx_encap_mpdu::frame len[%d], msdu len[%d], tailroom[%d] offset[%d].}",
                                  ul_frame_len, ul_msdu_len, ul_tailroom, us_msdu_offset);

    /* ETH HEAD + LLC + PAYLOAD */
    oal_netbuf_put(pst_dest_buf, ul_msdu_len);

    /* COPY ETH HEADER */
    pst_ether_head = (mac_ether_header_stru *)(oal_netbuf_data(pst_dest_buf) + us_msdu_offset);
    oal_memcopy((oal_uint8*)pst_ether_head, oal_netbuf_data(pst_buf), ETHER_HDR_LEN);

    /* FILL LLC HEADER */
    pst_snap_head = (mac_llc_snap_stru *)((oal_uint8*)pst_ether_head + ETHER_HDR_LEN);
    oal_memcopy((oal_uint8*)pst_snap_head, (oal_uint8*)&g_st_mac_11c_snap_header, SNAP_LLC_FRAME_LEN);

    /* change type & length */
    pst_snap_head->us_ether_type = pst_ether_head->us_ether_type;
    pst_ether_head->us_ether_type = oal_byteorder_host_to_net_uint16((oal_uint16)(ul_frame_len - ETHER_HDR_LEN + SNAP_LLC_FRAME_LEN));

    /* COPY MSDU PAYLOAD */
    pst_msdu_payload = (oal_uint8*)pst_snap_head + SNAP_LLC_FRAME_LEN;
    oal_memcopy(pst_msdu_payload, oal_netbuf_data(pst_buf) + ETHER_HDR_LEN, ul_frame_len - ETHER_HDR_LEN);

    /* 释放旧msdu */
    oal_netbuf_free(pst_buf);

    /* 更新amsdu信息 */
    pst_amsdu->uc_msdu_num++;
    pst_amsdu->us_amsdu_size += (oal_uint16)ul_msdu_len;
    pst_amsdu->uc_last_pad_len = (oal_uint8)(ul_msdu_len - SNAP_LLC_FRAME_LEN - ul_frame_len);

    ul_tailroom = oal_netbuf_tailroom(pst_dest_buf);
    /* 当前netbuf剩余空间较少 || 已经达到聚合最大帧数 */
    if ((ul_tailroom < HMAC_AMSDU_TX_MIN_LENGTH) || (pst_amsdu->uc_msdu_num >= pst_amsdu->uc_amsdu_maxnum))
    {
        hmac_amsdu_prepare_to_send(pst_vap, pst_user, pst_amsdu);
    }

    /* 由于最新的msdu skb已经被释放,不管当前amsdu是否缓存或发送成功/失败,都需要返回TX BUFF */
    return HMAC_TX_BUFF;

}


OAL_STATIC OAL_INLINE oal_uint32 hmac_amsdu_alloc_netbuf(hmac_amsdu_stru *pst_amsdu, oal_netbuf_stru *pst_buf)
{
    oal_netbuf_stru *pst_dest_buf;
    mac_tx_ctl_stru *pst_cb;

    pst_dest_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, WLAN_LARGE_NETBUF_SIZE, OAL_NETBUF_PRIORITY_MID);
    if(OAL_PTR_NULL == pst_dest_buf)
    {
        return OAL_FAIL;
    }

    /* 子帧链入amsdu尾部 */
    oal_netbuf_add_to_list_tail(pst_dest_buf, &pst_amsdu->st_msdu_head);

    oal_memcopy(oal_netbuf_cb(pst_dest_buf), oal_netbuf_cb(pst_buf), OAL_SIZEOF(mac_tx_ctl_stru));

    oal_netbuf_copy_queue_mapping(pst_dest_buf, pst_buf);

    pst_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_dest_buf);
    MAC_GET_CB_IS_FIRST_MSDU(pst_cb)    = OAL_TRUE;
    MAC_GET_CB_IS_AMSDU(pst_cb)  = OAL_TRUE;
    MAC_GET_CB_NETBUF_NUM(pst_cb)       = 1;

    return OAL_SUCC;
}


oal_uint32  hmac_amsdu_tx_process_etc(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf)
{
    oal_uint8           uc_tid_no;
    oal_uint32          ul_frame_len;
    oal_uint32          ul_ret;
    hmac_amsdu_stru    *pst_amsdu;
    mac_tx_ctl_stru    *pst_tx_ctl;

    pst_tx_ctl = (mac_tx_ctl_stru *)(oal_netbuf_cb(pst_buf));
    ul_frame_len = oal_netbuf_get_len(pst_buf);
    uc_tid_no    = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);
    pst_amsdu    = &(pst_user->ast_hmac_amsdu[uc_tid_no]);

    /* amsdu组帧溢出,将链表缓存帧发送并清空,新帧作为amsdu首帧入链表 */
    if (hmac_tx_amsdu_is_overflow(pst_amsdu, pst_tx_ctl, ul_frame_len, pst_user))
    {
        hmac_amsdu_prepare_to_send(pst_vap, pst_user, pst_amsdu);
    }

    if (0 == pst_amsdu->uc_msdu_num)
    {
        oal_netbuf_list_head_init(&pst_amsdu->st_msdu_head);
        /* 申请netbuf用于聚合amsdu */
        if(OAL_SUCC != hmac_amsdu_alloc_netbuf(pst_amsdu, pst_buf))
        {
            OAM_WARNING_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_tx_process_etc::failed to alloc netbuf.}");
            return HMAC_TX_PASS;
        }

        /* 启动定时器 */
        FRW_TIMER_CREATE_TIMER(&pst_amsdu->st_amsdu_timer,
                               hmac_amsdu_tx_timeout_process,
                               HMAC_AMSDU_LIFE_TIME,
                               pst_amsdu,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_vap->st_vap_base_info.ul_core_id);
    }

    /* 处理每一个msdu */
    ul_ret = hmac_amsdu_tx_encap_mpdu(pst_vap, pst_user, pst_amsdu, pst_buf);
    return ul_ret;
}

#elif(_PRE_PRODUCT_ID == _PRE_PRODUCT_ID_HI1151)


OAL_STATIC oal_uint32  hmac_amsdu_encap(hmac_amsdu_stru *pst_amsdu, oal_netbuf_stru *pst_buf, oal_uint32 ul_framelen)
{
    oal_uint32              ul_headroom;    /* 暂存skb剩余头部空间 */
    oal_uint32              ul_tailroom;    /* 暂存skb剩余尾部空间 */
    oal_uint8               uc_align;       /* 为4字节对齐尾部需要的偏移量 */
    mac_ether_header_stru   st_ether_head;  /* 暂存以太网过来的skb的以太网头 */
    mac_ether_header_stru  *pst_amsdu_head; /* 为填写amsdu子帧头的临时指针 */
    mac_llc_snap_stru      *pst_snap_head;  /* 为填写snap头的临时指针 */
    mac_tx_ctl_stru        *pst_cb;

    /* 入参检查 */
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_buf)
     || (OAL_PTR_NULL == pst_amsdu)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_AMPDU, "{hmac_amsdu_encap::input error}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 暂存数据剩余空间信息 */
    ul_headroom = oal_netbuf_headroom(pst_buf);
    ul_tailroom = oal_netbuf_tailroom(pst_buf);
    if (ul_framelen < oal_netbuf_get_len(pst_buf))
    {
        OAM_ERROR_LOG2(0, OAM_SF_SCAN, "{hmac_amsdu_encap::framelen[%d] < netbuflen[%d]!}", ul_framelen, oal_netbuf_get_len(pst_buf));
        return OAL_FAIL;
    }

    uc_align    = (oal_uint8)(ul_framelen - oal_netbuf_get_len(pst_buf));

    OAM_INFO_LOG3(0, OAM_SF_AMSDU, "{hmac_amsdu_encap::headroom[%d] tailroom[%d] offset[%d].}", ul_headroom, ul_tailroom, uc_align);

    /* 头部剩余空间不足需要扩展头部空间 */
    if (OAL_UNLIKELY(ul_headroom < SNAP_LLC_FRAME_LEN))
    {
        OAM_INFO_LOG1(0, OAM_SF_AMSDU, "{hmac_amsdu_encap::headroom[%d] need realloc.}", ul_headroom);
        pst_buf = oal_netbuf_realloc_headroom(pst_buf, (SNAP_LLC_FRAME_LEN - ul_headroom));
        if (OAL_PTR_NULL == pst_buf)
        {
            OAM_ERROR_LOG1(0, OAM_SF_AMSDU, "{hmac_amsdu_encap::headroom[%d] realloc fail.}", ul_headroom);
            return OAL_FAIL;
        }
    }

    /* 暂存以太网头的信息 */
    st_ether_head.us_ether_type = ((mac_ether_header_stru *)oal_netbuf_data(pst_buf))->us_ether_type;
    oal_set_mac_addr(st_ether_head.auc_ether_dhost, ((mac_ether_header_stru *)oal_netbuf_data(pst_buf))->auc_ether_dhost);
    oal_set_mac_addr(st_ether_head.auc_ether_shost, ((mac_ether_header_stru *)oal_netbuf_data(pst_buf))->auc_ether_shost);
    oal_set_mac_addr(pst_amsdu->auc_eth_da, ((mac_ether_header_stru *)oal_netbuf_data(pst_buf))->auc_ether_dhost);
    oal_set_mac_addr(pst_amsdu->auc_eth_sa, ((mac_ether_header_stru *)oal_netbuf_data(pst_buf))->auc_ether_shost);

    /* 填写snap头 */
    pst_snap_head = (mac_llc_snap_stru *)oal_netbuf_pull(pst_buf, ETHER_HDR_LEN - SNAP_LLC_FRAME_LEN);
    if (OAL_PTR_NULL == pst_snap_head)
    {
        OAM_ERROR_LOG0(0, OAM_SF_AMPDU, "{hmac_amsdu_encap::pst_snap_head null}");
        return OAL_FAIL;
    }

    pst_snap_head->uc_llc_dsap      = SNAP_LLC_LSAP;
    pst_snap_head->uc_llc_ssap      = SNAP_LLC_LSAP;
    pst_snap_head->uc_control       = LLC_UI;
    pst_snap_head->auc_org_code[0]  = SNAP_RFC1042_ORGCODE_0;
    pst_snap_head->auc_org_code[1]  = SNAP_RFC1042_ORGCODE_1;
    pst_snap_head->auc_org_code[2]  = SNAP_RFC1042_ORGCODE_2;
    pst_snap_head->us_ether_type    = st_ether_head.us_ether_type;

    /* 填写amsdu子帧头 */
    pst_amsdu_head = (mac_ether_header_stru *)oal_netbuf_push(pst_buf, ETHER_HDR_LEN);

    oal_set_mac_addr(pst_amsdu_head->auc_ether_dhost, st_ether_head.auc_ether_dhost);
    oal_set_mac_addr(pst_amsdu_head->auc_ether_shost, st_ether_head.auc_ether_shost);
    pst_amsdu_head->us_ether_type = oal_byteorder_host_to_net_uint16((oal_uint16)(oal_netbuf_get_len(pst_buf) - ETHER_HDR_LEN));

    /* 尾部空间不够字节对齐需扩展尾部空间 */
    if (OAL_UNLIKELY(ul_tailroom < uc_align))
    {
        OAM_INFO_LOG1(0, OAM_SF_AMSDU, "{hmac_amsdu_encap::tailroom[%d] need realloc.}", ul_tailroom);
        pst_buf = oal_netbuf_realloc_tailroom(pst_buf, uc_align - ul_tailroom);

        if (OAL_PTR_NULL == pst_buf)
        {
            OAM_ERROR_LOG1(0, OAM_SF_AMSDU, "{hmac_amsdu_encap::tailroom[%d] realloc fail.}", ul_tailroom);
            return OAL_FAIL;
        }
    }

    oal_netbuf_put(pst_buf, uc_align);

    pst_amsdu->uc_last_pad_len = uc_align;

    /* 子帧链入amsdu尾部 */
    oal_netbuf_add_to_list_tail(pst_buf, &pst_amsdu->st_msdu_head);

    /* 更新amsdu信息 */
    pst_amsdu->uc_msdu_num++;
    pst_amsdu->us_amsdu_size += (oal_uint16)oal_netbuf_get_len(pst_buf);

    pst_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_buf);

    /* 更新frame len */
    MAC_GET_CB_MPDU_LEN(pst_cb) = (oal_uint16)oal_netbuf_get_len(pst_buf);

    OAM_INFO_LOG2(0, OAM_SF_AMSDU, "{hmac_amsdu_encap::msdu_num[%d] amsdu_size[%d].}", pst_amsdu->uc_msdu_num, pst_amsdu->us_amsdu_size);

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_amsdu_send(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, hmac_amsdu_stru *pst_amsdu)
{
    frw_event_mem_stru *pst_amsdu_send_event_mem;
    frw_event_stru     *pst_amsdu_send_event;
    oal_uint32          ul_ret;
    mac_tx_ctl_stru    *pst_cb;
    oal_uint32          ul_index;
    oal_netbuf_stru    *pst_buf_temp;
    oal_netbuf_stru    *pst_net_buf;
    dmac_tx_event_stru *pst_amsdu_event;
#ifdef _PRE_DEBUG_MODE
    oal_uint16          us_buf_num = 0;
#endif

    /* 入参检查 */
    if (OAL_UNLIKELY((OAL_PTR_NULL == pst_vap) || (OAL_PTR_NULL == pst_user) || (OAL_PTR_NULL == pst_amsdu)))
    {
        OAM_ERROR_LOG3(0, OAM_SF_AMPDU, "{hmac_amsdu_send::input error %x %x %x}", pst_vap, pst_user, pst_amsdu);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 给dmac传送的amsdu相关的信息以及802.11头挂接 */
    pst_net_buf = oal_netbuf_peek(&(pst_amsdu->st_msdu_head));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_net_buf))
    {
        OAM_ERROR_LOG2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::pst_net_buf null, amsdu_num = %d, short_pkt_num = %d}", pst_amsdu->uc_msdu_num, pst_amsdu->uc_short_pkt_num);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_net_buf);
    MAC_GET_CB_IS_AMSDU(pst_cb)  = OAL_TRUE;
    MAC_GET_CB_MPDU_LEN(pst_cb)      = pst_amsdu->us_amsdu_size;    //首帧cb中存放本次聚合的总长度
    MAC_GET_CB_IS_FIRST_MSDU(pst_cb) = OAL_TRUE;
    MAC_GET_CB_NETBUF_NUM(pst_cb)    = pst_amsdu->uc_msdu_num;
    MAC_GET_CB_MPDU_NUM(pst_cb) = 1;

    /* amsdu只聚合一个帧时，回退成非amsdu，统一encap接口 */
    if (1 == pst_amsdu->uc_msdu_num)
    {
        oal_memmove(OAL_NETBUF_DATA(pst_net_buf) + SNAP_LLC_FRAME_LEN, OAL_NETBUF_DATA(pst_net_buf), OAL_MAC_ADDR_LEN + OAL_MAC_ADDR_LEN);
        oal_netbuf_pull(pst_net_buf, SNAP_LLC_FRAME_LEN);

        MAC_GET_CB_IS_AMSDU(pst_cb) = OAL_FALSE;
        MAC_GET_CB_MPDU_LEN(pst_cb) -= SNAP_LLC_FRAME_LEN;
    }

    /* 为整个amsdu封装802.11头 */
    ul_ret = hmac_tx_encap_etc(pst_vap, pst_user, pst_net_buf);
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        for (ul_index = 0; ul_index < pst_amsdu->uc_msdu_num; ul_index++)
        {
            pst_buf_temp = oal_netbuf_delist(&(pst_amsdu->st_msdu_head));

            oal_netbuf_free(pst_buf_temp);

            OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);
        }
        pst_amsdu->uc_msdu_num = 0;
        pst_amsdu->uc_short_pkt_num = 0;
        pst_amsdu->us_amsdu_size = 0;

        OAM_ERROR_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::hmac_tx_encap_etc failed[%d]}", ul_ret);
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 把最后一个子帧的PADDING去除 */
    //去除最后子帧neibuf 中padding长度
    pst_buf_temp = oal_netbuf_tail(&pst_amsdu->st_msdu_head);
    if (OAL_PTR_NULL == pst_buf_temp)
    {
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::pst_buf_temp null}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    oal_netbuf_trim(pst_buf_temp, pst_amsdu->uc_last_pad_len);
    //首帧cb中总长度需要减去padding长度
    MAC_GET_CB_MPDU_LEN(pst_cb) -= pst_amsdu->uc_last_pad_len;

    /* 链表头尾处理 */
    if(OAL_PTR_NULL != oal_netbuf_peek(&pst_amsdu->st_msdu_head))
    {
        OAL_NETBUF_PREV(oal_netbuf_peek(&pst_amsdu->st_msdu_head)) = OAL_PTR_NULL;
    }
    else
    {
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::oal_netbuf_peek return null}");
        return OAL_ERR_CODE_PTR_NULL;
    }
    if(OAL_PTR_NULL != oal_netbuf_tail(&pst_amsdu->st_msdu_head))
    {
        OAL_NETBUF_NEXT(oal_netbuf_tail(&pst_amsdu->st_msdu_head)) = OAL_PTR_NULL;
    }
    else
    {
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::oal_netbuf_tail return null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 抛事件 */
    pst_amsdu_send_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(dmac_tx_event_stru));

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_amsdu_send_event_mem))
    {
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "{hmac_amsdu_send::pst_amsdu_send_event_mem null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /* 填事件头 */
    pst_amsdu_send_event = frw_get_event_stru(pst_amsdu_send_event_mem);

    FRW_EVENT_HDR_INIT(&(pst_amsdu_send_event->st_event_hdr),
                        MAC_GET_CB_EVENT_TYPE(pst_cb),
                        MAC_GET_CB_EVENT_SUBTYPE(pst_cb),
                        OAL_SIZEOF(dmac_tx_event_stru),
                        FRW_EVENT_PIPELINE_STAGE_1,
                        pst_vap->st_vap_base_info.uc_chip_id,
                        pst_vap->st_vap_base_info.uc_device_id,
                        pst_vap->st_vap_base_info.uc_vap_id);

    pst_amsdu_send_event = frw_get_event_stru(pst_amsdu_send_event_mem);

    pst_amsdu_event = (dmac_tx_event_stru *)(pst_amsdu_send_event->auc_event_data);
    pst_amsdu_event->pst_netbuf = oal_netbuf_peek(&pst_amsdu->st_msdu_head);

    ul_ret = frw_event_dispatch_event_etc(pst_amsdu_send_event_mem);

    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        pst_cb = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_amsdu_event->pst_netbuf);
        OAL_MEM_FREE((oal_void *)MAC_GET_CB_FRAME_HEADER_ADDR(pst_cb), OAL_TRUE);
    #ifdef _PRE_DEBUG_MODE
        us_buf_num = hmac_free_netbuf_list_etc(pst_amsdu_event->pst_netbuf);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, us_buf_num);
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMPDU, "hmac_amsdu_send::frw_event_dispatch_event_etc fail[%d]", ul_ret);
    #else
        hmac_free_netbuf_list_etc(pst_amsdu_event->pst_netbuf);
    #endif
    }

    /* 清零amsdu结构体信息 */
    pst_amsdu->us_amsdu_size = 0;
    pst_amsdu->uc_msdu_num   = 0;
    oal_netbuf_list_head_init(&pst_amsdu->st_msdu_head);

    /* 清零amsdu短包计数    */
    pst_amsdu->uc_short_pkt_num = 0x00;

    /* 释放事件内存 */
    FRW_EVENT_FREE(pst_amsdu_send_event_mem);

    OAM_INFO_LOG0(0, OAM_SF_AMSDU, "{hmac_amsdu_send::amsdu send success.");

    return  ul_ret;
}


oal_void hmac_amsdu_update_cap(hmac_user_stru *pst_hmac_user_sta, hmac_amsdu_stru *pst_amsdu)
{
    mac_user_stru      *pst_mac_user;
    oal_uint16      us_amsdu_maxsize;

    pst_mac_user = &pst_hmac_user_sta->st_user_base_info;

    /* AMSDU最大聚合长度获取    */
    if ((WLAN_VHT_MODE == pst_hmac_user_sta->st_user_base_info.en_cur_protocol_mode)
    || (WLAN_VHT_ONLY_MODE == pst_hmac_user_sta->st_user_base_info.en_cur_protocol_mode))
    {
        /* VHT模式最大AMSDU最大长度更新 */
        us_amsdu_maxsize  = pst_mac_user->st_vht_hdl.us_max_mpdu_length;

#ifndef _PRE_WLAN_PRODUCT_1151V200
        if (WLAN_80211_CIPHER_SUITE_NO_ENCRYP == pst_mac_user->st_key_info.en_cipher_type)
        {
            /* 遗留问题:11ac聚合11454字节时性能会下降，所以限制open最大聚合长度为7935字节   */
            if(us_amsdu_maxsize > WLAN_AMSDU_FRAME_MAX_LEN_LONG)
            {
                us_amsdu_maxsize = WLAN_AMSDU_FRAME_MAX_LEN_LONG;
            }
        }
        else
        {
            /* 11ac聚合7935字节时，asus ac68网卡解密出问题会断流，所以限制加密最大聚合长度为3839字节   */
            if(us_amsdu_maxsize > WLAN_AMSDU_FRAME_MAX_LEN_SHORT)
            {
                us_amsdu_maxsize = WLAN_AMSDU_FRAME_MAX_LEN_SHORT;
            }
        }
#else
        //V200芯片已经修订此问题，加密模式下的AMSDU的长度限制预期可以放开，待验证!!!
        if(us_amsdu_maxsize > WLAN_AMSDU_FRAME_MAX_LEN_LONG)
        {
            us_amsdu_maxsize = WLAN_AMSDU_FRAME_MAX_LEN_LONG;
        }
#endif
    }
    else
    {
        /* HT模式最大AMSDU最大长度更新  */
        /* 11n受限于Delimeter MPDU长度限制，强制Amsdu最小长度聚合   */
        us_amsdu_maxsize = WLAN_AMSDU_FRAME_MAX_LEN_SHORT;
    }

    /* AMSUD最大长度和最大聚合个数配置  */
    pst_amsdu->us_amsdu_maxsize = us_amsdu_maxsize;
    if (1 == pst_mac_user->st_ht_hdl.uc_htc_support)
    {
        /* Account for HT-MAC Header, FCS & Security headers */
        pst_amsdu->us_amsdu_maxsize -= (30 + 4 + 16);
    }
    else
    {
        /* Account for QoS-MAC Header, FCS & Security headers */
        pst_amsdu->us_amsdu_maxsize -= (26 + 4 + 16);
    }

    if (1 == pst_hmac_user_sta->uc_is_wds)
    {
        /* Account for the 4th address in WDS-MAC Header */
        pst_amsdu->us_amsdu_maxsize -= (6);
    }

    pst_mac_user->us_amsdu_maxsize = pst_amsdu->us_amsdu_maxsize;
}


oal_uint32  hmac_amsdu_tx_process_etc(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf)
{
    oal_uint8           uc_tid_no;
    oal_uint32          ul_frame_len;
    hmac_amsdu_stru    *pst_amsdu;
    oal_uint32          ul_ret;         /* 所调用函数的返回值 */
    mac_tx_ctl_stru    *pst_tx_ctl;

    pst_tx_ctl = (mac_tx_ctl_stru *)(oal_netbuf_cb(pst_buf));

    ul_frame_len = oal_netbuf_get_len(pst_buf);

    uc_tid_no    = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);

    OAM_INFO_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_tx_process_etc::uc_tid_no=%d.}", uc_tid_no);

    /* 根据CB结构体中的TID信息，从用户结构体中获取amsdu结构体 */
    pst_amsdu    = &(pst_user->ast_hmac_amsdu[uc_tid_no]);

    MAC_GET_CB_IS_FIRST_MSDU(pst_tx_ctl) = OAL_FALSE;

    /* 根据能力位动态设定最大聚合长度   */
    hmac_amsdu_update_cap(pst_user, pst_amsdu);

    /* 获取到的amsdu中已有子帧，但新的子帧加入后该amsdu超过最大长度，则发送amsdu后封装新子帧并链入该amsdu */
    if (hmac_tx_amsdu_is_overflow(pst_amsdu, pst_tx_ctl, ul_frame_len, pst_user))
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_tx_process_etc::the length of amsdu is exceeded, so it is to be sent.}");

        /* 重启定时器 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_amsdu->st_amsdu_timer);

        ul_ret = hmac_amsdu_send(pst_vap, pst_user, pst_amsdu);

        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU,
                             "{hmac_amsdu_tx_process_etc::in amsdu notify, in the situation of length or number overflow, amsdu send fails. erro code is %d}", (oal_int32)ul_ret);

            return HMAC_TX_PASS;
        }
    }

    /* 小于500字节的短包AMSDU聚合不超过2个,否则会导致TX DATAFLOW BREAK高概率出现,流量极低   */
    if( pst_amsdu->uc_short_pkt_num >= HMAC_AMSDU_SHORT_PACKET_NUM )
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_tx_process_etc::the short packet num is overflow, so it is to be sent.}");

        /* 重启定时器 */
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_amsdu->st_amsdu_timer);

        ul_ret = hmac_amsdu_send(pst_vap, pst_user, pst_amsdu);
        if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
        {
            OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU,
                             "{hmac_amsdu_tx_process_etc::in amsdu notify, in the situation of short_pkt_num exceed, amsdu send fails. erro code is %d}", (oal_int32)ul_ret);
            return HMAC_TX_PASS;
        }

        /* 下一包MPDU为非AMSDU帧    */
        return HMAC_TX_PASS;
    }

    /* 若获取到的amsdu里为空则初始化amsdu中msdu链表头记录该amsdu诞生时间，封装子帧并链入该amsdu */
    if (0 == pst_amsdu->uc_msdu_num)
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_tx_process_etc::there is no msdu in the amsdu.}");

        oal_netbuf_list_head_init(&(pst_amsdu->st_msdu_head));

        /* 启动定时器 */
        FRW_TIMER_CREATE_TIMER(&pst_amsdu->st_amsdu_timer,
                               hmac_amsdu_tx_timeout_process,
                               HMAC_AMSDU_LIFE_TIME,
                               pst_amsdu,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_vap->st_vap_base_info.ul_core_id);

    }

    /* 4字节对齐后的帧的长度 */
    ul_frame_len = OAL_ROUND_UP(ul_frame_len, 4);

    /* 获取到的amsdu中已有子帧，但新的子帧加入后该amsdu没有超过最大长度，则封装新子帧并链入该amsdu */
    ul_ret = hmac_amsdu_encap(pst_amsdu, pst_buf, ul_frame_len);

    /* 小于500字节的短包个数统计    */
    if (OAL_UNLIKELY(OAL_SUCC != ul_ret))
    {
        OAM_WARNING_LOG1(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_tx_process_etc::in amsdu notify, amsdu encapsulation fails. erro code is %d.}", (oal_int32)ul_ret);
        OAM_STAT_VAP_INCR(pst_vap->st_vap_base_info.uc_vap_id, tx_abnormal_msdu_dropped, 1);

        return HMAC_TX_DROP_AMSDU_ENCAP_FAIL;
    }
    else	/* 只有返回成功才需要更新报文个数 */
    {
        if(OAL_TRUE == hmac_amsdu_is_short_pkt(ul_frame_len))
        {
            pst_amsdu->uc_short_pkt_num++;
        }
    }

    return HMAC_TX_BUFF;
}



OAL_STATIC  oal_bool_enum_uint8 hmac_tx_amsdu_is_overflow(
                hmac_amsdu_stru    *pst_amsdu,
                mac_tx_ctl_stru    *pst_tx_ctl,
                oal_uint32          ul_frame_len,
                hmac_user_stru     *pst_user)
{
    mac_tx_ctl_stru     *pst_head_ctl;
    oal_netbuf_stru     *pst_head_buf;

    if (0 == pst_amsdu->uc_msdu_num)
    {
        OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_amsdu_is_overflow::uc_msdu_num=0.}");
        return OAL_FALSE;
    }

    pst_head_buf = oal_netbuf_peek(&pst_amsdu->st_msdu_head);
    if (OAL_PTR_NULL == pst_head_buf)
    {
        OAM_INFO_LOG0(0, OAM_SF_TX, "{hmac_tx_amsdu_is_overflow::pst_head_buf null.}");
        return OAL_FALSE;
    }

    pst_head_ctl = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_head_buf);
    /* amsdu不为空，并且amsdu中的子帧来源(lan或者wlan)与当前要封装的netbuf不同，则将amsdu发送出去，
       这样做是因为在发送完成中释放一个mpdu时，是根据第一个netbuf的cb中填写的事件类型来选择释放策略，
       如果一个mpdu中的netbuf来源不同，会造成内存泄漏 */
    if (MAC_GET_CB_EVENT_TYPE(pst_tx_ctl) != MAC_GET_CB_EVENT_TYPE(pst_head_ctl))
    {
        OAM_INFO_LOG2(0, OAM_SF_TX, "{hmac_tx_amsdu_is_overflow::en_event_type mismatched. %d %d.}",
              MAC_GET_CB_EVENT_TYPE(pst_tx_ctl), MAC_GET_CB_EVENT_TYPE(pst_head_ctl));
        return OAL_TRUE;
    }

    OAM_INFO_LOG3(0, OAM_SF_TX, "{hmac_tx_amsdu_is_overflow::us_amsdu_size=%d uc_msdu_num=%d us_amsdu_maxsize=%d.}",
                  pst_amsdu->us_amsdu_size, pst_amsdu->uc_msdu_num, pst_amsdu->us_amsdu_maxsize);
    if (((pst_amsdu->us_amsdu_size + ul_frame_len + SNAP_LLC_FRAME_LEN) > pst_amsdu->us_amsdu_maxsize)
     || ((pst_amsdu->uc_msdu_num + 1) > pst_amsdu->uc_amsdu_maxnum))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}
#endif

#ifdef _PRE_WLAN_FEATURE_MULTI_NETBUF_AMSDU

oal_void hmac_tx_encap_large_skb_amsdu(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_user,oal_netbuf_stru *pst_buf,mac_tx_ctl_stru *pst_tx_ctl)
{
    mac_ether_header_stru                    *pst_ether_hdr_temp;
    mac_ether_header_stru                    *pst_ether_hdr;
    oal_uint8                                 uc_tid_no = WLAN_TIDNO_BUTT;
    oal_uint16                                us_mpdu_len;
    oal_uint16                                us_80211_frame_len;

    /* AMPDU+AMSDU功能未开启,由定制化门限决定，高于300Mbps时开启amsdu大包聚合 */
    if (OAL_FALSE == g_st_tx_large_amsdu.uc_cur_amsdu_enable)
    {
        return;
    }

    /* 针对关闭WMM，非QOS帧处理 */
    if(OAL_FALSE == pst_user->st_user_base_info.st_cap_info.bit_qos)
    {
        return;
    }

    /* VO、组播队列不开启AMPDU+AMSDU */
    uc_tid_no    = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);
    if (uc_tid_no >= WLAN_TIDNO_VOICE)
    {
        return;
    }

    /* 判断该tid是否支持AMPDU+AMSDU */
    if (OAL_FALSE == HMAC_USER_IS_AMSDU_SUPPORT(pst_user, uc_tid_no))
    {
        return;
    }

    /* 非长帧不进行AMPDU+AMSDU */
    us_mpdu_len = (oal_uint16)oal_netbuf_get_len(pst_buf);
    if ((us_mpdu_len < MAC_AMSDU_SKB_LEN_DOWN_LIMIT) || (us_mpdu_len > MAC_AMSDU_SKB_LEN_UP_LIMIT))
    {
        return;
    }

    /* 超出分片帧门限不进行AMPDU+AMSDU,计算时考虑需要新增的EHER HEAD LEN和字节对齐,MAC HEAD考虑最长帧头 */
    us_80211_frame_len = us_mpdu_len + SNAP_LLC_FRAME_LEN + 2 + MAC_80211_QOS_HTC_4ADDR_FRAME_LEN;
    if (us_80211_frame_len > mac_mib_get_FragmentationThreshold(&pst_hmac_vap->st_vap_base_info))
    {
        return;
    }

    /* 已经是小包AMSDU聚合 */
    if (OAL_TRUE == MAC_GET_CB_IS_AMSDU(pst_tx_ctl))
    {
        return;
    }

    /* ETHER HEAD头部空闲空间,4字节对齐;一般此条件均成立,放置于最后 */
    if (oal_netbuf_headroom(pst_buf) <  (SNAP_LLC_FRAME_LEN + 2))
    {
        return;
    }

    /* 80211 FRAME INCLUDE EHTER HEAD */
    MAC_GET_CB_HAS_EHTER_HEAD(pst_tx_ctl) = OAL_TRUE;

    pst_ether_hdr = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);

    /* 预留LLC HEAD长度 */
    oal_netbuf_push(pst_buf, SNAP_LLC_FRAME_LEN);
    pst_ether_hdr_temp = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
    /* 拷贝mac head */
    oal_memcopy((oal_uint8*)pst_ether_hdr_temp, (oal_uint8*)pst_ether_hdr, ETHER_HDR_LEN);
    /* 设置AMSDU帧长度 */
    pst_ether_hdr_temp->us_ether_type = oal_byteorder_host_to_net_uint16((oal_uint16)(us_mpdu_len - ETHER_HDR_LEN + SNAP_LLC_FRAME_LEN));

}
#endif


oal_uint32  hmac_amsdu_notify_etc(hmac_vap_stru *pst_vap, hmac_user_stru *pst_user, oal_netbuf_stru *pst_buf)
{
    oal_uint8           uc_tid_no;
    oal_uint32          ul_ret;         /* 所调用函数的返回值 */
    mac_tx_ctl_stru    *pst_tx_ctl;
    hmac_amsdu_stru    *pst_amsdu;

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_ip_header_stru     *pst_ip;
    mac_ether_header_stru  *pst_ether_header = (mac_ether_header_stru *)oal_netbuf_data(pst_buf);
#endif

    /* 获取cb中的tid信息 */
    pst_tx_ctl = (mac_tx_ctl_stru *)(oal_netbuf_cb(pst_buf));
    uc_tid_no    = MAC_GET_CB_WME_TID_TYPE(pst_tx_ctl);


    /* 针对关闭WMM，非QOS帧处理 */
    if(OAL_FALSE == pst_user->st_user_base_info.st_cap_info.bit_qos)
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_TX,"{hmac_amsdu_notify_etc::UnQos Frame pass!!}");
        return HMAC_TX_PASS;
    }
    /* 组播转单播数据不聚合 */
    if (pst_tx_ctl->bit_is_m2u_data)
    {
        return HMAC_TX_PASS;
    }

    #ifdef _PRE_WLAN_FEATURE_DUAL_BAND_PERF_OPT
    /* 5G存在业务时，不聚2g amsdu */
    if ((OAL_FALSE == g_en_2g_tx_amsdu) &&
       (WLAN_BAND_2G == pst_vap->st_vap_base_info.st_channel.en_band))
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id,OAM_SF_AMSDU,"{hmac_amsdu_notify_etc::2g tx amsdu forbidden due to 5G traffic}");
        return HMAC_TX_PASS;
    }
    #endif

    /* 检查amsdu开关是否打开,amsdu_tx_on 0/1; VAP 是否支持聚合 */
    if ((OAL_TRUE != mac_mib_get_CfgAmsduTxAtive(&pst_vap->st_vap_base_info))
       ||(OAL_TRUE != mac_mib_get_AmsduAggregateAtive(&pst_vap->st_vap_base_info)))
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_notify_etc::amsdu is unenable in amsdu notify}");
        return HMAC_TX_PASS;
    }

    /* 判断该tid是否在ampdu情况下支持amsdu的发送,ampdu_amsdu 0/1 */
    if (OAL_FALSE == HMAC_USER_IS_AMSDU_SUPPORT(pst_user, uc_tid_no))
    {
        OAM_INFO_LOG2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_notify_etc::AMPDU NOT SUPPORT AMSDU uc_tid_no=%d uc_amsdu_supported=%d}",
                      uc_tid_no, pst_user->uc_amsdu_supported);
        return HMAC_TX_PASS;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_HOST2NET_SHORT(ETHER_TYPE_IP) == pst_ether_header->us_ether_type)
    {
        pst_ip = (oal_ip_header_stru *)(pst_ether_header + 1);
        /* 1103 允许TCP ACK聚合 */
        #if (_PRE_PRODUCT_ID != _PRE_PRODUCT_ID_HI1103_HOST)
        /* tcp 关键帧不聚合 */
        if(MAC_TCP_PROTOCAL == pst_ip->uc_protocol)
        {
            if(OAL_TRUE == oal_netbuf_is_tcp_ack_etc(pst_ip))
            {
                return HMAC_TX_PASS;
            }
        }
        #endif
        /* 为了解决业务量小时ping包延迟的问题 */
        if(OAL_TRUE == oal_netbuf_is_icmp_etc(pst_ip))
        {
            return HMAC_TX_PASS;
        }
    }

#endif

    /* 检查用户是否是HT/VHT */
    if (OAL_FALSE == hmac_user_xht_support(pst_user))
    {
        OAM_INFO_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_notify_etc::user is not qos in amsdu notify}");
        return HMAC_TX_PASS;
    }

    if (OAL_UNLIKELY(uc_tid_no >= WLAN_TID_MAX_NUM))
    {
        OAM_ERROR_LOG0(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_notify_etc::invalid tid number obtained from the cb in asmdu notify function}");
        return HMAC_TX_PASS;
    }

    if (WLAN_TIDNO_VOICE == uc_tid_no)
    {
        OAM_INFO_LOG2(pst_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "{hmac_amsdu_notify_etc::VO TID NOT SUPPORT AMSDU uc_tid_no=%d uc_amsdu_supported=%d",
                                                                           uc_tid_no, pst_user->uc_amsdu_supported);
        return HMAC_TX_PASS;
    }

    pst_amsdu    = &(pst_user->ast_hmac_amsdu[uc_tid_no]);
    oal_spin_lock_bh(&pst_amsdu->st_amsdu_lock);

    /* 新来帧是大帧,需将缓存帧发完 */
    if(oal_netbuf_get_len(pst_buf) > WLAN_MSDU_MAX_LEN)
    {
        /* 防止乱序,应该先发送旧帧 */
        if(pst_amsdu->uc_msdu_num)
        {
            hmac_amsdu_prepare_to_send(pst_vap, pst_user, pst_amsdu);
        }
        oal_spin_unlock_bh(&pst_amsdu->st_amsdu_lock);
        return HMAC_TX_PASS;
    }

    ul_ret = hmac_amsdu_tx_process_etc(pst_vap, pst_user, pst_buf);
    oal_spin_unlock_bh(&pst_amsdu->st_amsdu_lock);
    return ul_ret;
}


OAL_STATIC oal_uint32  hmac_amsdu_tx_timeout_process(oal_void *p_arg)
{
    hmac_amsdu_stru         *pst_temp_amsdu;
    mac_tx_ctl_stru         *pst_cb;
    hmac_user_stru          *pst_user;
    oal_uint32               ul_ret;
    oal_netbuf_stru         *pst_netbuf;
    hmac_vap_stru           *pst_hmac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == p_arg))
    {
        OAM_ERROR_LOG0(0, OAM_SF_AMPDU, "{hmac_amsdu_tx_timeout_process::input null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_temp_amsdu = (hmac_amsdu_stru *)p_arg;

    oal_spin_lock_bh(&pst_temp_amsdu->st_amsdu_lock);

    /* 根据要发送的amsdu下第一个msdu子帧的cb字段的信息寻找对应用户结构体 */
    pst_netbuf = oal_netbuf_peek(&pst_temp_amsdu->st_msdu_head);
    if (OAL_PTR_NULL == pst_netbuf)
    {
        OAM_INFO_LOG1(0, OAM_SF_AMSDU, "hmac_amsdu_tx_timeout_process::pst_netbuf NULL. msdu_num[%d]", pst_temp_amsdu->uc_msdu_num);
        oal_spin_unlock_bh(&pst_temp_amsdu->st_amsdu_lock);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_cb          = (mac_tx_ctl_stru *)oal_netbuf_cb(pst_netbuf);
    pst_hmac_vap    = (hmac_vap_stru *)mac_res_get_hmac_vap(MAC_GET_CB_TX_VAP_INDEX(pst_cb));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        oal_spin_unlock_bh(&pst_temp_amsdu->st_amsdu_lock);
        OAM_ERROR_LOG0(0, OAM_SF_AMPDU, "{hmac_amsdu_tx_timeout_process::pst_hmac_vap null}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(MAC_GET_CB_TX_USER_IDX(pst_cb));
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_user))
    {
        oal_spin_unlock_bh(&pst_temp_amsdu->st_amsdu_lock);
        OAM_ERROR_LOG1(0, OAM_SF_AMPDU, "{hmac_amsdu_tx_timeout_process::pst_user[%d] null}", MAC_GET_CB_TX_USER_IDX(pst_cb));
        return OAL_ERR_CODE_PTR_NULL;
    }

    ul_ret = hmac_amsdu_send(pst_hmac_vap, pst_user, pst_temp_amsdu);
    if (OAL_SUCC != ul_ret)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "hmac_amsdu_tx_timeout_process::hmac_amsdu_send fail[%d]", ul_ret);
    }

    oal_spin_unlock_bh(&pst_temp_amsdu->st_amsdu_lock);
    OAM_INFO_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_AMSDU, "hmac_amsdu_tx_timeout_process::hmac_amsdu_send SUCC");

    return OAL_SUCC;
}


oal_void hmac_amsdu_init_user_etc(hmac_user_stru *pst_hmac_user_sta)
{
    oal_uint32           ul_amsdu_idx;
    hmac_amsdu_stru     *pst_amsdu;

    if(OAL_PTR_NULL == pst_hmac_user_sta)
    {
        OAM_ERROR_LOG0(0, OAM_SF_AMPDU, "{hmac_amsdu_init_user_etc::pst_hmac_user_sta null}");
        return;
    }

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
    pst_hmac_user_sta->us_amsdu_maxsize = WLAN_AMSDU_FRAME_MAX_LEN_LONG;
#endif

    pst_hmac_user_sta->uc_amsdu_supported = AMSDU_ENABLE_ALL_TID;

    /* 设置amsdu域 */
    for (ul_amsdu_idx = 0; ul_amsdu_idx < WLAN_TID_MAX_NUM; ul_amsdu_idx++)
    {
        pst_amsdu = &(pst_hmac_user_sta->ast_hmac_amsdu[ul_amsdu_idx]);

        oal_spin_lock_init(&pst_amsdu->st_amsdu_lock);
        oal_netbuf_list_head_init(&(pst_amsdu->st_msdu_head));
        pst_amsdu->us_amsdu_size    = 0;

        hmac_amsdu_set_maxnum(pst_amsdu, WLAN_AMSDU_MAX_NUM);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
        hmac_amsdu_set_maxsize(pst_amsdu, pst_hmac_user_sta, WLAN_AMSDU_FRAME_MAX_LEN_LONG);
#else
        pst_amsdu->uc_short_pkt_num = 0x00;
        pst_amsdu->uc_msdu_num      = 0;
#endif
    }
}



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

