


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "hmac_frag.h"
#include "hmac_11i.h"



#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_FRAG_C
#if 0
OAL_STATIC oal_void  hmac_defrag_check_seq_addr(hmac_user_stru *pst_hmac_user,
                                                        mac_ieee80211_frame_stru *pst_last_hdr,
                                                        mac_ieee80211_frame_stru *pst_mac_hdr,
                                                        oal_uint16 us_rx_seq,
                                                        oal_uint8 uc_frag_num);
#endif
/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/


oal_uint32 hmac_frag_process(hmac_vap_stru *pst_hmac_vap,
                                   oal_netbuf_stru *pst_netbuf_original,
                                   mac_tx_ctl_stru *pst_tx_ctl,
                                   oal_uint32 ul_cip_hdrsize,
                                   oal_uint32 ul_max_tx_unit)
{
    mac_ieee80211_frame_stru *pst_mac_header;
    mac_ieee80211_frame_stru *pst_frag_header;
    oal_netbuf_stru          *pst_netbuf;
    oal_netbuf_stru          *pst_netbuf_prev;
    oal_uint32                ul_total_hdrsize;
    oal_uint32                ul_frag_num;
    oal_uint32                ul_frag_size;
    oal_int32                 l_remainder;
    oal_uint32                ul_payload  = 0;
    oal_uint32                ul_offset;
    mac_tx_ctl_stru          *pst_tx_ctl_copy;
    oal_uint32                ul_mac_hdr_size;

    ul_mac_hdr_size = MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctl);
    pst_mac_header = MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctl);
    pst_mac_header->st_frame_control.bit_more_frag = OAL_TRUE;
    ul_total_hdrsize = ul_mac_hdr_size + ul_cip_hdrsize;
    ul_frag_num = 1;
    /* 加密字节数包含在分片门限中，预留加密字节长度，由硬件填写加密头 */

    ul_offset       = ul_max_tx_unit - ul_cip_hdrsize - ul_mac_hdr_size;
    l_remainder    = (oal_int32)(OAL_NETBUF_LEN(pst_netbuf_original) - ul_offset);
    pst_netbuf_prev = pst_netbuf_original;

    do
    {
        ul_frag_size = ul_total_hdrsize + (oal_uint32)l_remainder;

        /* 判断是否还有更多的分片 */
        if (ul_frag_size > ul_max_tx_unit)
        {
            ul_frag_size = ul_max_tx_unit;
        }

        /* 防止反复扩充头部空间预留32字节 */
        pst_netbuf = oal_netbuf_alloc(ul_frag_size + MAC_80211_QOS_HTC_4ADDR_FRAME_LEN + 32, MAC_80211_QOS_HTC_4ADDR_FRAME_LEN + 32, 4);
        if (OAL_PTR_NULL == pst_netbuf)
        {
            /* 在外部释放之前申请的报文 */
            OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_frag_process::pst_netbuf null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        pst_tx_ctl_copy = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_netbuf);
        /* 拷贝cb字段 */
        oal_memcopy(pst_tx_ctl_copy, pst_tx_ctl, MAC_TX_CTL_SIZE);
        oal_netbuf_copy_queue_mapping(pst_netbuf, pst_netbuf_original);

        /*netbuf的headroom大于802.11 mac头长度*/
        pst_frag_header = (mac_ieee80211_frame_stru *)(OAL_NETBUF_PAYLOAD(pst_netbuf)- MAC_80211_QOS_HTC_4ADDR_FRAME_LEN);
        MAC_GET_CB_80211_MAC_HEAD_TYPE(pst_tx_ctl_copy) = 1;  /*指示mac头部在skb中*/

        /* 拷贝帧头内容 */
        oal_memcopy(pst_frag_header, pst_mac_header, ul_mac_hdr_size);
        /* 赋值分片号 */
        pst_frag_header->bit_frag_num = ul_frag_num;
        ul_frag_num++;
        /* 计算分片报文帧体长度 */
        ul_payload = ul_frag_size - ul_total_hdrsize;

        oal_netbuf_copydata(pst_netbuf_original, ul_offset, OAL_NETBUF_PAYLOAD(pst_netbuf), ul_payload);

        oal_netbuf_set_len(pst_netbuf, ul_payload);

        MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctl_copy) = pst_frag_header;
        MAC_GET_CB_MPDU_LEN(pst_tx_ctl_copy)      = (oal_uint16)ul_payload;
        OAL_NETBUF_NEXT(pst_netbuf_prev)          = pst_netbuf;
        pst_netbuf_prev                           = pst_netbuf;

        /* 计算下一个分片报文的长度和偏移 */
        l_remainder    -= (oal_int32)ul_payload;
        ul_offset       += ul_payload;
    }while(l_remainder > 0);

    pst_frag_header->st_frame_control.bit_more_frag = OAL_FALSE;
    OAL_NETBUF_NEXT(pst_netbuf) = OAL_PTR_NULL;

    /* 原始报文作为分片报文的第一个 */
    oal_netbuf_trim(pst_netbuf_original, OAL_NETBUF_LEN(pst_netbuf_original) - (ul_max_tx_unit - ul_cip_hdrsize - ul_mac_hdr_size));
    MAC_GET_CB_MPDU_LEN(pst_tx_ctl) = (oal_uint16)(OAL_NETBUF_LEN(pst_netbuf_original));


    return OAL_SUCC;
}


oal_uint32  hmac_defrag_timeout_fn_etc(oal_void *p_arg)
{
    hmac_user_stru  *pst_hmac_user;
    oal_netbuf_stru *pst_netbuf = OAL_PTR_NULL;
    pst_hmac_user = (hmac_user_stru *)p_arg;


    /* 超时后释放正在重组的分片报文 */
    if (pst_hmac_user->pst_defrag_netbuf)
    {
        pst_netbuf = pst_hmac_user->pst_defrag_netbuf;

        OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_FALSE);
        oal_netbuf_free(pst_netbuf);
        pst_hmac_user->pst_defrag_netbuf = OAL_PTR_NULL;
    }

    return OAL_SUCC;
}


oal_netbuf_stru* hmac_defrag_process_etc(hmac_user_stru *pst_hmac_user, oal_netbuf_stru *pst_netbuf, oal_uint32 ul_hrdsize)
{
    mac_ieee80211_frame_stru *pst_mac_hdr   = OAL_PTR_NULL;
    mac_ieee80211_frame_stru *pst_last_hdr  = OAL_PTR_NULL;
    oal_uint16                us_rx_seq     = 0;
    oal_uint16                us_last_seq   = 0;
    oal_uint8                 uc_frag_num   = 0;
    oal_uint8                 uc_last_frag_num  = 0;
    oal_bool_enum_uint8       en_more_frag      = OAL_FALSE;
    oal_netbuf_stru          *pst_new_buf       = OAL_PTR_NULL;
    mac_rx_ctl_stru          *pst_rx_ctl;
    oal_uint32                ul_ret;
    oal_uint8                 uc_device_id;
    mac_device_stru          *pst_mac_device;

    if (0 == ul_hrdsize)
    {
        oal_netbuf_free(pst_netbuf);
        OAM_WARNING_LOG0(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_defrag_process_etc::mac head len is 0.}");
        return OAL_PTR_NULL;
    }

    pst_mac_hdr = (mac_ieee80211_frame_stru *)oal_netbuf_data(pst_netbuf);
    us_rx_seq   = pst_mac_hdr->bit_seq_num;
    uc_frag_num = (oal_uint8)pst_mac_hdr->bit_frag_num;
    en_more_frag = (oal_bool_enum_uint8)pst_mac_hdr->st_frame_control.bit_more_frag;

    /* 如果没有什么可以去分片的则直接返回 */
    if (!en_more_frag && 0 == uc_frag_num && OAL_PTR_NULL == pst_hmac_user->pst_defrag_netbuf)
    {
        return pst_netbuf;
    }

    OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_FALSE);

    /* 首先检查到来的分片报文是不是属于正在重组的分片报文 */
    if (OAL_PTR_NULL != pst_hmac_user->pst_defrag_netbuf)
    {
        frw_timer_restart_timer_etc(&pst_hmac_user->st_defrag_timer, HMAC_FRAG_TIMEOUT, OAL_FALSE);

        pst_last_hdr = (mac_ieee80211_frame_stru *)oal_netbuf_data(pst_hmac_user->pst_defrag_netbuf);

        us_last_seq  = pst_last_hdr->bit_seq_num;
        uc_last_frag_num = (oal_uint8)pst_last_hdr->bit_frag_num;

        /* 如果地址不匹配，序列号不匹配，分片号不匹配则释放现在正在重组的报文 */
        if (us_rx_seq != us_last_seq ||
            uc_frag_num != (uc_last_frag_num + 1) ||
            oal_compare_mac_addr(pst_last_hdr->auc_address1, pst_mac_hdr->auc_address1) ||
            oal_compare_mac_addr(pst_last_hdr->auc_address2, pst_mac_hdr->auc_address2))
        {
            FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_user->st_defrag_timer);
            oal_netbuf_free(pst_hmac_user->pst_defrag_netbuf);
            pst_hmac_user->pst_defrag_netbuf = OAL_PTR_NULL;
        }
    }

    /* 判断到来的分片报文是否是第一个分片 */
    if (OAL_PTR_NULL == pst_hmac_user->pst_defrag_netbuf)
    {
        /* 没有分片,直接返回 */
        if (0 == en_more_frag && 0 == uc_frag_num)
        {
            return pst_netbuf;
        }

        /* 首片分片的分片号不为0则释放 */
        if (uc_frag_num != 0)
        {
            oal_netbuf_free(pst_netbuf);
            OAM_STAT_VAP_INCR(pst_hmac_user->st_user_base_info.uc_vap_id, rx_defrag_process_dropped, 1);
            OAM_INFO_LOG3(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY,
                        "{hmac_defrag_process_etc::The first frag_num is not Zero(%d), seq_num[%d], mor_frag[%d].}",
                           uc_frag_num, us_rx_seq, en_more_frag);

            return OAL_PTR_NULL;
        }


        uc_device_id   = pst_hmac_user->st_user_base_info.uc_device_id;
        pst_mac_device = mac_res_get_dev_etc((oal_uint32)uc_device_id);
        if (OAL_PTR_NULL == pst_mac_device)
        {
            OAM_ERROR_LOG4(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY,
                        "{hmac_defrag_process_etc::user index[%d] user mac:XX:XX:XX:%02X:%02X:%02X}",
                        pst_hmac_user->st_user_base_info.us_assoc_id,
                        pst_hmac_user->st_user_base_info.auc_user_mac_addr[3],
                        pst_hmac_user->st_user_base_info.auc_user_mac_addr[4],
                        pst_hmac_user->st_user_base_info.auc_user_mac_addr[5]);
            /* user异常，丢弃报文 */
            oal_netbuf_free(pst_netbuf);
            OAM_STAT_VAP_INCR(pst_hmac_user->st_user_base_info.uc_vap_id, rx_defrag_process_dropped, 1);
            return OAL_PTR_NULL;
        }

        /* 启动超时定时器，超时释放重组报文 */
        FRW_TIMER_CREATE_TIMER(&pst_hmac_user->st_defrag_timer,
                                hmac_defrag_timeout_fn_etc,
                                HMAC_FRAG_TIMEOUT,
                                pst_hmac_user,
                                OAL_FALSE,
                                OAM_MODULE_ID_HMAC,
                                pst_mac_device->ul_core_id);

        /* 内存池netbuf只有1600 可能不够，参照A公司申请2500操作系统原生态报文 */
        pst_new_buf = OAL_MEM_NETBUF_ALLOC(OAL_NORMAL_NETBUF, HMAC_MAX_FRAG_SIZE, OAL_NETBUF_PRIORITY_MID);
        if (OAL_PTR_NULL == pst_new_buf)
        {
            OAM_ERROR_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_defrag_process_etc::Alloc new_buf null,size[%d].}", HMAC_MAX_FRAG_SIZE);
            return OAL_PTR_NULL;
        }
        OAL_MEM_NETBUF_TRACE(pst_new_buf, OAL_FALSE);

        /* 将分片报文拷贝到新申请的报文中并挂接到用户结构体下，释放原有的报文 */
        oal_netbuf_init(pst_new_buf, OAL_NETBUF_LEN(pst_netbuf));
        oal_netbuf_copydata(pst_netbuf, 0, oal_netbuf_data(pst_new_buf), OAL_NETBUF_LEN(pst_netbuf));
        oal_memcopy(OAL_NETBUF_CB(pst_new_buf), OAL_NETBUF_CB(pst_netbuf), MAC_TX_CTL_SIZE);
        pst_rx_ctl = (mac_rx_ctl_stru *)OAL_NETBUF_CB(pst_new_buf);
        MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctl) = (oal_uint32 *)OAL_NETBUF_HEADER(pst_new_buf);
        pst_hmac_user->pst_defrag_netbuf = pst_new_buf;

        oal_netbuf_free(pst_netbuf);
    }
    else
    {
        /* 此分片是期望的到来的分片，重启定时器，并进行重组 */
        frw_timer_restart_timer_etc(&pst_hmac_user->st_defrag_timer, HMAC_FRAG_TIMEOUT, OAL_FALSE);

        pst_last_hdr = (mac_ieee80211_frame_stru *)oal_netbuf_data(pst_hmac_user->pst_defrag_netbuf);

        /* 记录最新分片报文的分片号 */
        pst_last_hdr->bit_seq_num   = pst_mac_hdr->bit_seq_num;/* [false alarm]:pst_mac_hdr不可能为空，这是队列中的元素，队列不可能为空*/
        pst_last_hdr->bit_frag_num  = pst_mac_hdr->bit_frag_num;

        oal_netbuf_pull(pst_netbuf, ul_hrdsize);

        /*此接口内会调用dev_kfree_skb*/
        oal_netbuf_concat(pst_hmac_user->pst_defrag_netbuf, pst_netbuf);
    }

    /* 判断是否重组完毕，存在更多报文返回空指针，重组完毕返回组好的报文 */
    if (en_more_frag)
    {
        pst_netbuf = OAL_PTR_NULL;
    }
    else
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&pst_hmac_user->st_defrag_timer);

        pst_netbuf = pst_hmac_user->pst_defrag_netbuf;

        pst_hmac_user->pst_defrag_netbuf = OAL_PTR_NULL;

        /* 对重组好的报文进行mic检查 */
        ul_ret = hmac_de_mic_etc(pst_hmac_user, pst_netbuf);
        if (OAL_SUCC != ul_ret)
        {
            oal_netbuf_free(pst_netbuf);
            OAM_STAT_VAP_INCR(pst_hmac_user->st_user_base_info.uc_vap_id, rx_de_mic_fail_dropped, 1);
            OAM_WARNING_LOG1(pst_hmac_user->st_user_base_info.uc_vap_id, OAM_SF_ANY, "{hmac_defrag_process_etc::hmac_de_mic_etc failed[%d].}", ul_ret);
            return OAL_PTR_NULL;
        }

        pst_last_hdr = (mac_ieee80211_frame_stru *)oal_netbuf_data(pst_netbuf);
        pst_last_hdr->bit_frag_num = 0;

    }

    return pst_netbuf;
}







#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

