


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "wlan_spec.h"
#include "mac_vap.h"
#include "hmac_blockack.h"
#include "hmac_main.h"
#include "hmac_rx_data.h"
#include "hmac_mgmt_bss_comm.h"
#include "hmac_user.h"
#include  "hmac_auto_adjust_freq.h"

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_BLOCKACK_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*****************************************************************************
  3 函数实现
*****************************************************************************/

oal_void hmac_reorder_ba_timer_start_etc(hmac_vap_stru *pst_hmac_vap, hmac_user_stru *pst_hmac_user, oal_uint8 uc_tid)
{
    mac_vap_stru               *pst_mac_vap;
    hmac_ba_rx_stru            *pst_ba_rx_stru;
    mac_device_stru            *pst_device;
    oal_uint16                  us_timeout;

    /* 如果超时定时器已经被注册则返回 */
    if (OAL_TRUE == pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer.en_is_registerd)
    {
        return;
    }

    pst_mac_vap = &pst_hmac_vap->st_vap_base_info;

    pst_device = mac_res_get_dev_etc(pst_mac_vap->uc_device_id);
    if (OAL_PTR_NULL == pst_device)
    {
        OAM_ERROR_LOG1(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_reorder_ba_timer_start_etc::pst_device[%d] null.}", pst_mac_vap->uc_device_id);
        return ;
    }

    /* 业务量较小时,使用小周期的重排序定时器,保证及时上报至协议栈;
       业务量较大时,使用大周期的重排序定时器,保证尽量不丢包*/
    if (OAL_FALSE == hmac_wifi_rx_is_busy())
    {
        us_timeout = pst_hmac_vap->us_rx_timeout_min[WLAN_WME_TID_TO_AC(uc_tid)];
    }
    else
    {
        us_timeout = pst_hmac_vap->us_rx_timeout[WLAN_WME_TID_TO_AC(uc_tid)];
    }

    pst_ba_rx_stru = pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info;

    oal_spin_lock(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer_lock));

    FRW_TIMER_CREATE_TIMER(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer),
                           hmac_ba_timeout_fn_etc,
                           us_timeout,
                           &(pst_ba_rx_stru->st_alarm_data),
                           OAL_FALSE,
                           OAM_MODULE_ID_HMAC,
                           pst_device->ul_core_id);

    oal_spin_unlock(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer_lock));

}


OAL_STATIC hmac_rx_buf_stru* hmac_ba_buffer_frame_in_reorder(hmac_ba_rx_stru* pst_ba_rx_hdl, oal_uint16 us_seq_num, mac_rx_ctl_stru *pst_cb_ctrl)
{
    oal_uint16          us_buf_index;
    hmac_rx_buf_stru*   pst_rx_buf;

    us_buf_index = (us_seq_num & (WLAN_AMPDU_RX_BUFFER_SIZE - 1));

    pst_rx_buf = &(pst_ba_rx_hdl->ast_re_order_list[us_buf_index]);

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC != _PRE_MULTI_CORE_MODE)
    if (1 == pst_rx_buf->in_use)
    {
        hmac_rx_free_netbuf_list_etc(&pst_rx_buf->st_netbuf_head, pst_rx_buf->uc_num_buf);
        OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_ba_buffer_frame_in_reorder::slot already used, seq[%d].}", us_seq_num);
    }
    else
    {
        pst_ba_rx_hdl->uc_mpdu_cnt++;
    }

    pst_rx_buf->uc_num_buf   = pst_cb_ctrl->bit_buff_nums;  //标识该MPDU占用的netbuff个数，一般用于AMSDU

    pst_rx_buf->in_use = 1;
#else
    if (1 == pst_rx_buf->in_use)
    {
        hmac_rx_free_netbuf_list_etc(&pst_rx_buf->st_netbuf_head, pst_rx_buf->uc_num_buf);
        pst_ba_rx_hdl->uc_mpdu_cnt--;
        pst_rx_buf->in_use = 0;
        pst_rx_buf->uc_num_buf = 0;
        OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_ba_buffer_frame_in_reorder::slot already used, seq[%d].}", us_seq_num);
    }

    if(OAL_TRUE == pst_cb_ctrl->bit_amsdu_enable)
    {
        if(OAL_TRUE == pst_cb_ctrl->bit_is_first_buffer)
        {
            if(oal_netbuf_list_len(&pst_rx_buf->st_netbuf_head) != 0)
            {
                hmac_rx_free_netbuf_list_etc(&pst_rx_buf->st_netbuf_head, oal_netbuf_list_len(&pst_rx_buf->st_netbuf_head));
                OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_ba_buffer_frame_in_reorder::seq[%d] amsdu first buffer, need clear st_netbuf_head first}", us_seq_num);
            }
            pst_rx_buf->uc_num_buf = 0;
        }

        /* offload下,amsdu帧拆成单帧分别上报 */
        pst_rx_buf->uc_num_buf += pst_cb_ctrl->bit_buff_nums;

        /* 遇到最后一个amsdu buffer 才标记in use 为 1 */
        if(OAL_TRUE == pst_cb_ctrl->bit_is_last_buffer)
        {
            //OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_ba_buffer_frame_in_reorder::amsdu total [%d] netbuf num}", pst_rx_buf->uc_num_buf);
            pst_ba_rx_hdl->uc_mpdu_cnt++;
            pst_rx_buf->in_use = 1;
        }
        else
        {
            //OAM_INFO_LOG1(0, OAM_SF_BA, "{hmac_ba_buffer_frame_in_reorder::partial amsdu [%d] netbuf num}", pst_rx_buf->uc_num_buf);
            pst_rx_buf->in_use = 0;
        }
    }
    else
    {
        pst_rx_buf->uc_num_buf   = pst_cb_ctrl->bit_buff_nums;
        pst_ba_rx_hdl->uc_mpdu_cnt++;
        pst_rx_buf->in_use = 1;
    }
#endif
    return pst_rx_buf;
}


OAL_STATIC oal_uint32  hmac_ba_send_frames_with_gap(hmac_ba_rx_stru *pst_ba_rx_hdl, oal_netbuf_head_stru *pst_netbuf_header, oal_uint16 us_last_seqnum, mac_vap_stru *pst_vap)
{
    oal_uint8            uc_num_frms  = 0;
    oal_uint16           us_seq_num;
    hmac_rx_buf_stru    *pst_rx_buf  = OAL_PTR_NULL;
    oal_uint8            uc_loop_index;
    oal_netbuf_stru     *pst_netbuf;

    us_seq_num   = pst_ba_rx_hdl->us_baw_start;

    OAM_INFO_LOG1(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_send_frames_with_gap::to seq[%d].}", us_last_seqnum);

    while(us_seq_num != us_last_seqnum)
    {
        if((pst_rx_buf = hmac_remove_frame_from_reorder_q(pst_ba_rx_hdl, us_seq_num)) != OAL_PTR_NULL)
        {
            pst_ba_rx_hdl->uc_mpdu_cnt--;
            pst_netbuf = oal_netbuf_peek(&pst_rx_buf->st_netbuf_head);
            if (OAL_UNLIKELY(pst_netbuf == OAL_PTR_NULL))
            {
                OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_send_frames_with_gap::gap[%d].\r\n}", us_seq_num);

                us_seq_num = DMAC_BA_SEQNO_ADD(us_seq_num, 1);
                pst_rx_buf->uc_num_buf = 0;

                continue;
            }

            for (uc_loop_index = 0; uc_loop_index < pst_rx_buf->uc_num_buf; uc_loop_index++)
            {
                pst_netbuf = oal_netbuf_delist(&pst_rx_buf->st_netbuf_head);
                OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_FALSE);
                if (OAL_PTR_NULL != pst_netbuf)
                {
                    oal_netbuf_add_to_list_tail(pst_netbuf, pst_netbuf_header);
                }
            }
            pst_rx_buf->uc_num_buf = 0;
            uc_num_frms++;
        }

        us_seq_num = DMAC_BA_SEQNO_ADD(us_seq_num, 1);
    }

    if (0 != pst_ba_rx_hdl->uc_mpdu_cnt)
    {
        OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_send_frames_with_gap::uc_mpdu_cnt=%d.}", pst_ba_rx_hdl->uc_mpdu_cnt);
    }

    return uc_num_frms;
}


OAL_STATIC oal_uint16  hmac_ba_send_frames_in_order(hmac_ba_rx_stru *pst_ba_rx_hdl, oal_netbuf_head_stru *pst_netbuf_header, mac_vap_stru *pst_vap)
{
    oal_uint16          us_seq_num;
    hmac_rx_buf_stru   *pst_rx_buf  = OAL_PTR_NULL;
    oal_uint8           uc_loop_index;
    oal_netbuf_stru    *pst_netbuf;

    us_seq_num   = pst_ba_rx_hdl->us_baw_start;
    while((pst_rx_buf = hmac_remove_frame_from_reorder_q(pst_ba_rx_hdl, us_seq_num)) != OAL_PTR_NULL)
    {
        pst_ba_rx_hdl->uc_mpdu_cnt--;
        us_seq_num = HMAC_BA_SEQNO_ADD(us_seq_num, 1);
        pst_netbuf = oal_netbuf_peek(&pst_rx_buf->st_netbuf_head);
        if (OAL_PTR_NULL == pst_netbuf)
        {
            OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_send_frames_in_order::[%d] slot error.}", us_seq_num);
            pst_rx_buf->uc_num_buf = 0;
            continue;
        }

        for (uc_loop_index = 0; uc_loop_index < pst_rx_buf->uc_num_buf; uc_loop_index++)
        {
            pst_netbuf = oal_netbuf_delist(&pst_rx_buf->st_netbuf_head);
            OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_FALSE);
            if (OAL_PTR_NULL != pst_netbuf)
            {
                oal_netbuf_add_to_list_tail(pst_netbuf, pst_netbuf_header);
            }
        }

        pst_rx_buf->uc_num_buf = 0;
    }

    return us_seq_num;
}


OAL_STATIC OAL_INLINE oal_void  hmac_ba_buffer_rx_frame(hmac_ba_rx_stru *pst_ba_rx_hdl,
                                                                   mac_rx_ctl_stru *pst_cb_ctrl,
                                                                   oal_netbuf_head_stru *pst_netbuf_header,
                                                                   oal_uint16 us_seq_num)
{
    hmac_rx_buf_stru   *pst_rx_netbuf = OAL_PTR_NULL;
    oal_netbuf_stru    *pst_netbuf;
    oal_uint8           uc_netbuf_index;
#ifdef _PRE_DEBUG_MODE
    oal_uint32          ul_netbuf_num;
#endif

    /* Get the pointer to the buffered packet */
    pst_rx_netbuf = hmac_ba_buffer_frame_in_reorder(pst_ba_rx_hdl, us_seq_num, pst_cb_ctrl);

    /* Update the buffered receive packet details */
    pst_rx_netbuf->us_seq_num   = us_seq_num;

    pst_rx_netbuf->ul_rx_time   = (oal_uint32)OAL_TIME_GET_STAMP_MS();

#ifdef _PRE_DEBUG_MODE
    ul_netbuf_num = oal_netbuf_get_buf_num(&pst_rx_netbuf->st_netbuf_head);
    if (0 != ul_netbuf_num)
    {
        OAM_INFO_LOG1(pst_cb_ctrl->uc_mac_vap_id, OAM_SF_BA, "{hmac_ba_buffer_rx_frame::%d netbuf miss here.}", ul_netbuf_num);
    }

    if (1 != pst_rx_netbuf->uc_num_buf)
    {
        OAM_INFO_LOG1(pst_cb_ctrl->uc_mac_vap_id, OAM_SF_BA, "{hmac_ba_buffer_rx_frame:find amsdu netbuff cnt %d.}", pst_rx_netbuf->uc_num_buf);
    }
#endif

    /* all buffers of this frame must be deleted from the buf list */
    for (uc_netbuf_index = pst_cb_ctrl->bit_buff_nums; uc_netbuf_index > 0; uc_netbuf_index--)
    {
        pst_netbuf = oal_netbuf_delist(pst_netbuf_header);

        OAL_MEM_NETBUF_TRACE(pst_netbuf, OAL_TRUE);
        if (OAL_UNLIKELY(OAL_PTR_NULL != pst_netbuf))
        {
            /* enqueue reorder queue */
            oal_netbuf_add_to_list_tail(pst_netbuf, &pst_rx_netbuf->st_netbuf_head);
        }
        else
        {
            OAM_ERROR_LOG0(pst_cb_ctrl->uc_mac_vap_id, OAM_SF_BA, "{hmac_ba_buffer_rx_frame:netbuff error in amsdu.}");
        }
    }

    if (oal_netbuf_list_len(&pst_rx_netbuf->st_netbuf_head) != pst_rx_netbuf->uc_num_buf)
    {
        OAM_WARNING_LOG2(pst_cb_ctrl->uc_mac_vap_id, OAM_SF_BA, "{hmac_ba_buffer_rx_frame: list_len=%d numbuf=%d}",
            oal_netbuf_list_len(&pst_rx_netbuf->st_netbuf_head),pst_rx_netbuf->uc_num_buf);
        pst_rx_netbuf->uc_num_buf = oal_netbuf_list_len(&pst_rx_netbuf->st_netbuf_head);
    }


}


OAL_STATIC OAL_INLINE oal_void  hmac_ba_reorder_rx_data(hmac_ba_rx_stru        *pst_ba_rx_hdl,
                                                        oal_netbuf_head_stru   *pst_netbuf_header,
                                                        mac_vap_stru           *pst_vap,
                                                        oal_uint16              us_seq_num)
{
    oal_uint8 uc_seqnum_pos;
    oal_uint16 us_temp_winstart;

    uc_seqnum_pos = hmac_ba_seqno_bound_chk(pst_ba_rx_hdl->us_baw_start, pst_ba_rx_hdl->us_baw_end, us_seq_num);

    if(DMAC_BA_BETWEEN_SEQLO_SEQHI == uc_seqnum_pos)
    {
        pst_ba_rx_hdl->us_baw_start = hmac_ba_send_frames_in_order(pst_ba_rx_hdl, pst_netbuf_header, pst_vap);
        pst_ba_rx_hdl->us_baw_end   = DMAC_BA_SEQNO_ADD(pst_ba_rx_hdl->us_baw_start, (pst_ba_rx_hdl->us_baw_size - 1));
    }
    else if(DMAC_BA_GREATER_THAN_SEQHI == uc_seqnum_pos)
    {
        us_temp_winstart = HMAC_BA_SEQNO_SUB(us_seq_num, (pst_ba_rx_hdl->us_baw_size - 1));

        hmac_ba_send_frames_with_gap(pst_ba_rx_hdl, pst_netbuf_header, us_temp_winstart, pst_vap);
        pst_ba_rx_hdl->us_baw_start = us_temp_winstart;
        pst_ba_rx_hdl->us_baw_start = hmac_ba_send_frames_in_order(pst_ba_rx_hdl, pst_netbuf_header, pst_vap);
        pst_ba_rx_hdl->us_baw_end   = HMAC_BA_SEQNO_ADD(pst_ba_rx_hdl->us_baw_start, (pst_ba_rx_hdl->us_baw_size - 1));
    }
    else
    {
        OAM_INFO_LOG3(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_reorder_rx_data::else branch seqno[%d] ws[%d] we[%d].}",
                      us_seq_num, pst_ba_rx_hdl->us_baw_start, pst_ba_rx_hdl->us_baw_end);
    }
}


OAL_STATIC oal_void  hmac_ba_flush_reorder_q(hmac_ba_rx_stru *pst_rx_ba)
{
    hmac_rx_buf_stru   *pst_rx_buf = OAL_PTR_NULL;
    oal_uint16          us_index;

    for (us_index = 0; us_index < WLAN_AMPDU_RX_BUFFER_SIZE; us_index++)
    {
        pst_rx_buf = &(pst_rx_ba->ast_re_order_list[us_index]);

        if (OAL_TRUE == pst_rx_buf->in_use)
        {
            hmac_rx_free_netbuf_list_etc(&pst_rx_buf->st_netbuf_head, pst_rx_buf->uc_num_buf);

            pst_rx_buf->in_use = OAL_FALSE;
            pst_rx_buf->uc_num_buf = 0;
            pst_rx_ba->uc_mpdu_cnt--;
        }
    }

    if (0 != pst_rx_ba->uc_mpdu_cnt)
    {
        OAM_WARNING_LOG1(0, OAM_SF_BA, "{hmac_ba_flush_reorder_q:: %d mpdu cnt left.}", pst_rx_ba->uc_mpdu_cnt);
    }
}

OAL_STATIC OAL_INLINE oal_uint32  hmac_ba_check_rx_aggr(mac_vap_stru               *pst_vap,
                                             mac_ieee80211_frame_stru   *pst_frame_hdr)
{
    /* 该vap是否是ht */
    if (OAL_FALSE == mac_mib_get_HighThroughputOptionImplemented(pst_vap))
    {
        OAM_INFO_LOG0(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_check_rx_aggr::ht not supported by this vap.}");
        return OAL_FAIL;
    }

    /* 判断该帧是不是qos帧 */
    if ((WLAN_FC0_SUBTYPE_QOS | WLAN_FC0_TYPE_DATA) != ((oal_uint8 *)pst_frame_hdr)[0])
    {
        OAM_INFO_LOG0(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_check_rx_aggr::not qos data.}");
        return OAL_FAIL;
    }

    return OAL_SUCC;
}


OAL_STATIC OAL_INLINE oal_bool_enum_uint8  hmac_ba_need_update_hw_baw(hmac_ba_rx_stru *pst_ba_rx_hdl, oal_uint16 us_seq_num)
{
    if ((OAL_TRUE == hmac_ba_seqno_lt(us_seq_num, pst_ba_rx_hdl->us_baw_start))
     && (OAL_FALSE == hmac_ba_rx_seqno_lt(us_seq_num, pst_ba_rx_hdl->us_baw_start)))
    {
        return OAL_TRUE;
    }

    return OAL_FALSE;
}


oal_uint32  hmac_ba_filter_serv_etc(
                hmac_user_stru             *pst_hmac_user,
                mac_rx_ctl_stru            *pst_cb_ctrl,
                oal_netbuf_head_stru       *pst_netbuf_header,
                oal_bool_enum_uint8        *pen_is_ba_buf)
{
    hmac_ba_rx_stru        *pst_ba_rx_hdl;
    oal_uint16              us_seq_num;
    oal_bool_enum_uint8     en_is_4addr;
    oal_uint8               uc_is_tods;
    oal_uint8               uc_is_from_ds;
    oal_uint8               uc_tid;
    oal_uint16              us_baw_start_temp;
    oal_uint32              ul_ret;
    mac_ieee80211_frame_stru   *pst_frame_hdr;
    hmac_vap_stru               *pst_hmac_vap;
    mac_vap_stru               *pst_mac_vap;

    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_netbuf_header ||
        OAL_PTR_NULL == pst_cb_ctrl ||
        OAL_PTR_NULL == pen_is_ba_buf))
    {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_ba_filter_serv_etc::param null.}");

        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_frame_hdr = (mac_ieee80211_frame_stru *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_cb_ctrl);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_frame_hdr))
    {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_ba_filter_serv_etc::pst_frame_hdr null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_vap = (hmac_vap_stru*)mac_res_get_hmac_vap(pst_cb_ctrl->uc_mac_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_ba_filter_serv_etc::pst_vap null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_vap = &pst_hmac_vap->st_vap_base_info;

    ul_ret = hmac_ba_check_rx_aggr(pst_mac_vap, pst_frame_hdr);
    if (OAL_SUCC != ul_ret)
    {
        return OAL_SUCC;
    }

    /* 考虑四地址情况获取报文的tid */
    uc_is_tods    = mac_hdr_get_to_ds((oal_uint8 *)pst_frame_hdr);
    uc_is_from_ds = mac_hdr_get_from_ds((oal_uint8 *)pst_frame_hdr);
    en_is_4addr   = uc_is_tods && uc_is_from_ds;
    uc_tid        = mac_get_tid_value((oal_uint8 *)pst_frame_hdr, en_is_4addr);

    pst_ba_rx_hdl = pst_hmac_user->ast_tid_info[uc_tid].pst_ba_rx_info;
    if (OAL_PTR_NULL == pst_ba_rx_hdl)
    {
        return OAL_SUCC;
    }
    if (DMAC_BA_COMPLETE != pst_ba_rx_hdl->en_ba_status)
    {
        OAM_WARNING_LOG1(pst_cb_ctrl->uc_mac_vap_id, OAM_SF_BA, "{hmac_ba_filter_serv_etc::ba_status = %d.", pst_ba_rx_hdl->en_ba_status);
        return OAL_SUCC;
    }

    /* 暂时保存BA窗口的序列号，用于鉴别是否有帧上报 */
    us_baw_start_temp = pst_ba_rx_hdl->us_baw_start;

    us_seq_num = mac_get_seq_num((oal_uint8 *)pst_frame_hdr);

    if (OAL_TRUE == (oal_bool_enum_uint8)pst_frame_hdr->st_frame_control.bit_more_frag)
    {
        OAM_WARNING_LOG1(pst_cb_ctrl->uc_mac_vap_id, OAM_SF_BA, "{hmac_ba_filter_serv_etc::We get a frag_frame[seq_num=%d] When BA_session is set UP!", us_seq_num);
        return OAL_SUCC;
    }

    /* duplicate frame判断 */
    if (OAL_TRUE == hmac_ba_rx_seqno_lt(us_seq_num, pst_ba_rx_hdl->us_baw_start))
    {
        /* 上次非定时器上报，直接删除duplicate frame帧，否则，直接上报 */
        if (OAL_FALSE == pst_ba_rx_hdl->en_timer_triggered)
        {
            /* 确实已经收到该帧 */
            if (hmac_ba_isset(pst_ba_rx_hdl, us_seq_num))
            {
                //OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_filter_serv_etc::duplicate frame,us_seq_num=%d baw_start=%d.",
                //                us_seq_num, pst_ba_rx_hdl->us_baw_start);

                HMAC_USER_STATS_PKT_INCR(pst_hmac_user->ul_rx_pkt_drop, 1);
                return OAL_FAIL;
            }
        }

        return OAL_SUCC;
    }
    /* restart ba timer */
    //frw_timer_restart_timer_etc(&pst_ba_rx_hdl->st_ba_timer, pst_ba_rx_hdl->st_ba_timer.us_timeout, OAL_TRUE);
    if (OAL_TRUE == hmac_ba_seqno_lt(pst_ba_rx_hdl->us_baw_tail, us_seq_num))
    {
        pst_ba_rx_hdl->us_baw_tail = us_seq_num;
    }

    /* 接收到的帧的序列号等于BAW_START，并且缓存队列帧个数为0，则直接上报给HMAC */
    if ((us_seq_num == pst_ba_rx_hdl->us_baw_start) && (0 == pst_ba_rx_hdl->uc_mpdu_cnt)
         #if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
         /* offload 下amsdu帧由于可能多个buffer组成，一律走重排序 */
         && (OAL_FALSE == pst_cb_ctrl->bit_amsdu_enable)
         #endif
        )
    {
        //OAM_TID_AMPDU_STATS_INCR(pst_tid_queue->pst_tid_ampdu_stat->ul_ba_recipient_direct_up_count, 1);

        pst_ba_rx_hdl->us_baw_start = DMAC_BA_SEQNO_ADD(pst_ba_rx_hdl->us_baw_start, 1);
        pst_ba_rx_hdl->us_baw_end  = DMAC_BA_SEQNO_ADD(pst_ba_rx_hdl->us_baw_end, 1);

        //OAM_WARNING_LOG1(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_filter_serv_etc::new packet need not be buffered, ws[%d].}\r\n", pst_ba_rx_hdl->us_baw_start);
        //OAL_IO_PRINT("{hmac_ba_filter_serv_etc::new packet need not be buffered, ws[%d].}\r\n", pst_ba_rx_hdl->us_baw_start);
    }
    else
    {
        //OAM_TID_AMPDU_STATS_INCR(pst_tid_queue->pst_tid_ampdu_stat->ul_ba_recipient_buffer_frame_count, 1);
        //OAM_WARNING_LOG2(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_filter_serv_etc::new packet need buffered, ws[%d],sq[%d].}\r\n", pst_ba_rx_hdl->us_baw_start, us_seq_num);
        //OAL_IO_PRINT("{hmac_ba_filter_serv_etc::new packet need buffered, ws[%d],sq[%d].}\r\n", pst_ba_rx_hdl->us_baw_start, us_seq_num);

        /* Buffer the new MSDU */
        *pen_is_ba_buf = OAL_TRUE;

        /* buffer frame to reorder */
        hmac_ba_buffer_rx_frame(pst_ba_rx_hdl, pst_cb_ctrl, pst_netbuf_header, us_seq_num);

        /* put the reordered netbufs to the end of the list */
        hmac_ba_reorder_rx_data(pst_ba_rx_hdl, pst_netbuf_header, pst_mac_vap, us_seq_num);

        /* Check for Sync loss and flush the reorder queue when one is detected */
        if((pst_ba_rx_hdl->us_baw_tail == DMAC_BA_SEQNO_SUB(pst_ba_rx_hdl->us_baw_start, 1)) &&
            (pst_ba_rx_hdl->uc_mpdu_cnt > 0))
        {
            //OAM_TID_AMPDU_STATS_INCR(pst_tid_queue->pst_tid_ampdu_stat->ul_ba_recipient_sync_loss_count, 1);

            OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_filter_serv_etc::Sync loss and flush the reorder queue.}");
            hmac_ba_flush_reorder_q(pst_ba_rx_hdl);
        }

        /* 重排序队列刷新后,如果队列中有帧那么启动定时器 */
        if (pst_ba_rx_hdl->uc_mpdu_cnt > 0)
        {
            hmac_reorder_ba_timer_start_etc(pst_hmac_vap, pst_hmac_user, uc_tid);
        }
    }

#if 0 /* 函数hmac_ba_need_update_hw_baw逻辑有误，且并未根据它的返回做任何实质性的操作，应是上移到hmac后的残留代码 */
    if (OAL_TRUE == hmac_ba_need_update_hw_baw(pst_ba_rx_hdl, us_seq_num))
    {
        OAM_WARNING_LOG0(pst_mac_vap->uc_vap_id, OAM_SF_BA, "{hmac_ba_filter_serv_etc::need to check mac ba ssn.}");
    }
#endif
    if (us_baw_start_temp != pst_ba_rx_hdl->us_baw_start)
    {
        pst_ba_rx_hdl->en_timer_triggered = OAL_FALSE;
    }

    return OAL_SUCC;
}


oal_void hmac_reorder_ba_rx_buffer_bar_etc(hmac_ba_rx_stru *pst_rx_ba, oal_uint16 us_start_seq_num,  mac_vap_stru *pst_vap)
{
    oal_netbuf_head_stru    st_netbuf_head;
    oal_uint8               uc_seqnum_pos;

    if (OAL_PTR_NULL == pst_rx_ba)
    {
        OAM_WARNING_LOG0(0, OAM_SF_BA, "{hmac_reorder_ba_rx_buffer_bar_etc::receive a bar, but ba session doesnot set up.}");
        return;
    }

    /* 针对 BAR 的SSN和窗口的start_num相等时，不需要移窗 */
    if(pst_rx_ba->us_baw_start == us_start_seq_num)
    {
        OAM_INFO_LOG0(0, OAM_SF_BA, "{hmac_reorder_ba_rx_buffer_bar_etc::seq is equal to start num.}");
        return;
    }

    oal_netbuf_list_head_init(&st_netbuf_head);

    uc_seqnum_pos = hmac_ba_seqno_bound_chk(pst_rx_ba->us_baw_start, pst_rx_ba->us_baw_end, us_start_seq_num);
    /* 针对BAR的的SSN在窗口内才移窗 */
    if (DMAC_BA_BETWEEN_SEQLO_SEQHI == uc_seqnum_pos)
    {
        hmac_ba_send_frames_with_gap(pst_rx_ba, &st_netbuf_head, us_start_seq_num, pst_vap);
        pst_rx_ba->us_baw_start = us_start_seq_num;
        pst_rx_ba->us_baw_start = hmac_ba_send_frames_in_order(pst_rx_ba, &st_netbuf_head, pst_vap);
        pst_rx_ba->us_baw_end   = HMAC_BA_SEQNO_ADD(pst_rx_ba->us_baw_start, (pst_rx_ba->us_baw_size - 1));

        OAM_INFO_LOG3(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_reorder_ba_rx_buffer_bar_etc::receive a bar, us_baw_start=%d us_baw_end=%d. us_seq_num=%d.}",
          pst_rx_ba->us_baw_start, pst_rx_ba->us_baw_end, us_start_seq_num);

        hmac_rx_lan_frame_etc(&st_netbuf_head);
    }
    else if (DMAC_BA_GREATER_THAN_SEQHI == uc_seqnum_pos)
    {
        /* 异常 */
        OAM_WARNING_LOG3(pst_vap->uc_vap_id, OAM_SF_BA, "{hmac_reorder_ba_rx_buffer_bar_etc::receive a bar and ssn is out of winsize, us_baw_start=%d us_baw_end=%d, us_seq_num=%d.}",
          pst_rx_ba->us_baw_start, pst_rx_ba->us_baw_end, us_start_seq_num);
    }
}


OAL_STATIC oal_uint32  hmac_ba_rx_prepare_bufflist(hmac_vap_stru *pst_hmac_vap, hmac_rx_buf_stru *pst_rx_buf, oal_netbuf_head_stru *pst_netbuf_head)
{
    oal_netbuf_stru     *pst_netbuf;
    oal_uint8            uc_loop_index;

    pst_netbuf = oal_netbuf_peek(&pst_rx_buf->st_netbuf_head);
    if (pst_netbuf == OAL_PTR_NULL)
    {
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_ba_rx_prepare_bufflist::pst_netbuf null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    for (uc_loop_index = 0; uc_loop_index < pst_rx_buf->uc_num_buf; uc_loop_index++)
    {
        pst_netbuf = oal_netbuf_delist(&pst_rx_buf->st_netbuf_head);
        if (OAL_PTR_NULL != pst_netbuf)
        {
            oal_netbuf_add_to_list_tail(pst_netbuf, pst_netbuf_head);
            //OAL_IO_PRINT("hmac_ba_rx_prepare_bufflist: out 0x%x", pst_netbuf);
        }
        else
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_ba_rx_prepare_bufflist::uc_num_buf in reorder list is error.}");
        }
    }

    return OAL_SUCC;
}


OAL_STATIC oal_uint32  hmac_ba_send_reorder_timeout(hmac_ba_rx_stru *pst_rx_ba, hmac_vap_stru *pst_hmac_vap, hmac_ba_alarm_stru *pst_alarm_data,
                                                    oal_uint16 *pus_timeout)
{
    oal_uint32                  ul_time_diff;
    oal_uint32                  ul_rx_timeout;
    oal_netbuf_head_stru        st_netbuf_head;
    oal_uint16                  us_baw_head;
    oal_uint16                  us_baw_start;   /* 保存最初的窗口起始序列号 */
    hmac_rx_buf_stru           *pst_rx_buf;
    oal_uint8                   uc_buff_count = 0;
    oal_uint32                  ul_ret;
    oal_uint16                  us_baw_end;


    oal_netbuf_list_head_init(&st_netbuf_head);
    us_baw_head     = pst_rx_ba->us_baw_start;
    us_baw_start    = pst_rx_ba->us_baw_start;
    us_baw_end      = HMAC_BA_SEQNO_ADD(pst_rx_ba->us_baw_tail, 1);
    ul_rx_timeout   = (oal_uint32)(*pus_timeout);

    /* OAM_INFO_LOG2(0, OAM_SF_BA, "{hmac_ba_send_reorder_timeout::us_baw_head=%d us_baw_end=%d.}",
                  us_baw_head, us_baw_end); */
    oal_spin_lock(&pst_rx_ba->st_ba_lock);

    while (us_baw_head != us_baw_end)
    {
        pst_rx_buf = hmac_get_frame_from_reorder_q(pst_rx_ba, us_baw_head);
        if (OAL_PTR_NULL == pst_rx_buf)
        {
            uc_buff_count++;
            us_baw_head = HMAC_BA_SEQNO_ADD(us_baw_head, 1);
            continue;
        }

        /* 如果冲排序队列中的帧滞留时间小于定时器超时时间,那么暂时不强制flush */
        ul_time_diff = (oal_uint32)OAL_TIME_GET_STAMP_MS() - pst_rx_buf->ul_rx_time;
        if (ul_time_diff < ul_rx_timeout)
        {
            /* frw_timer_restart_timer_etc(&pst_rx_ba->st_ba_timer, (oal_uint16)(ul_rx_timeout - ul_time_diff), OAL_TRUE); */
            *pus_timeout = (oal_uint16)(ul_rx_timeout - ul_time_diff);
            break;
        }

        pst_rx_ba->uc_mpdu_cnt--;
        pst_rx_buf->in_use = 0;

        ul_ret = hmac_ba_rx_prepare_bufflist(pst_hmac_vap, pst_rx_buf, &st_netbuf_head);
        if (ul_ret != OAL_SUCC)
        {
            uc_buff_count++;
            us_baw_head = HMAC_BA_SEQNO_ADD(us_baw_head, 1);
            continue;
        }

        uc_buff_count++;
        us_baw_head = HMAC_BA_SEQNO_ADD(us_baw_head, 1);
        pst_rx_ba->us_baw_start = HMAC_BA_SEQNO_ADD(pst_rx_ba->us_baw_start, uc_buff_count);
        pst_rx_ba->us_baw_end   = HMAC_BA_SEQNO_ADD(pst_rx_ba->us_baw_start, (pst_rx_ba->us_baw_size - 1));

        uc_buff_count = 0;
    }

    oal_spin_unlock(&pst_rx_ba->st_ba_lock);

    /* 判断本次定时器超时是否有帧上报 */
    if (us_baw_start != pst_rx_ba->us_baw_start)
    {
        //hmac_ba_update_rx_baw(pst_rx_ba, us_baw_start);
        pst_rx_ba->en_timer_triggered = OAL_TRUE;

        //OAL_IO_PRINT("hmac_ba_send_reorder_timeout: old seq %d, new seq %d\r\n", us_baw_start, pst_rx_ba->us_baw_start);
    }

    hmac_rx_lan_frame_etc(&st_netbuf_head);

    return OAL_SUCC;
}


oal_uint32  hmac_ba_timeout_fn_etc(oal_void *p_arg)
{
    hmac_ba_rx_stru                    *pst_rx_ba;
    hmac_vap_stru                      *pst_vap;
    hmac_user_stru                     *pst_hmac_user;
    hmac_ba_alarm_stru                 *pst_alarm_data;
    mac_delba_initiator_enum_uint8      en_direction;
    oal_uint8                           uc_tid = 0;
    mac_device_stru                    *pst_mac_device;
    oal_uint16                          us_timeout= 0;

    pst_alarm_data = (hmac_ba_alarm_stru *)p_arg;

    en_direction = pst_alarm_data->en_direction;

    uc_tid = pst_alarm_data->uc_tid;
    if (uc_tid >= WLAN_TID_MAX_NUM)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BA, "{hmac_ba_timeout_fn_etc::tid %d overflow.}", uc_tid);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_alarm_data->uc_vap_id);
    if (OAL_PTR_NULL == pst_vap)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BA, "{hmac_ba_timeout_fn_etc::pst_vap null. vap id %d.}", pst_alarm_data->uc_vap_id);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc(pst_alarm_data->us_mac_user_idx);
    if (OAL_PTR_NULL == pst_hmac_user)
    {
        OAM_WARNING_LOG1(0, OAM_SF_BA, "{hmac_ba_timeout_fn_etc::pst_hmac_user null. user idx %d.}", pst_alarm_data->us_mac_user_idx);
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_mac_device = mac_res_get_dev_etc(pst_vap->st_vap_base_info.uc_device_id);
    if (OAL_PTR_NULL == pst_mac_device)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BA, "{hmac_ba_timeout_fn_etc::pst_mac_device null. uc_device_id id %d.}", pst_vap->st_vap_base_info.uc_device_id);
        return OAL_ERR_CODE_PTR_NULL;
    }


//    if (pst_mac_device->ul_core_id >= 1)
    if (pst_mac_device->ul_core_id >= WLAN_FRW_MAX_NUM_CORES)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BA, "{hmac_ba_timeout_fn_etc::core id %d overflow.}", pst_mac_device->ul_core_id);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    if (MAC_RECIPIENT_DELBA == en_direction)
    {
        pst_rx_ba = (hmac_ba_rx_stru *)pst_alarm_data->pst_ba;

        if (OAL_PTR_NULL == pst_rx_ba)
        {
            OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_ba_timeout_fn_etc::pst_rx_ba is null.}");
            return OAL_ERR_CODE_PTR_NULL;
        }

        /* 接收业务量较少时只能靠超时定时器冲刷重排序队列,为改善游戏帧延时,需要将超时时间设小 */
        if (OAL_FALSE == hmac_wifi_rx_is_busy())
        {
            us_timeout = pst_vap->us_rx_timeout_min[WLAN_WME_TID_TO_AC(uc_tid)];
        }
        else
        {
            us_timeout = pst_vap->us_rx_timeout[WLAN_WME_TID_TO_AC(uc_tid)];
        }

        if (pst_rx_ba->uc_mpdu_cnt > 0)
        {
            //OAL_IO_PRINT("{hmac_ba_timeout_fn_etc::us_ba_timeout=%d uc_mpdu_cnt=%d.\r\n}", pst_rx_ba->us_ba_timeout, pst_rx_ba->uc_mpdu_cnt);
            hmac_ba_send_reorder_timeout(pst_rx_ba, pst_vap, pst_alarm_data, &us_timeout);

            //pst_alarm_data->us_timeout_times = 0;
        }
#if 0
        else
        {
            /* frw_timer_restart_timer_etc(&pst_rx_ba->st_ba_timer, pst_vap->us_rx_timeout[WLAN_WME_TID_TO_AC(pst_alarm_data->uc_tid)], OAL_TRUE); */
            pst_alarm_data->us_timeout_times++;
        }
#endif

        /* 若重排序队列刷新后,依然有缓存帧则需要重启定时器;
           若重排序队列无帧则为了节省功耗不启动定时器,在有帧入队时重启 */
        if (pst_rx_ba->uc_mpdu_cnt > 0)
        {
            oal_spin_lock(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer_lock));
            /* 此处不需要判断定时器是否已经启动,如果未启动则启动定时器;
               如果此定时器已经启动*/
            //if (OAL_FALSE == pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer.en_is_registerd)
            FRW_TIMER_CREATE_TIMER(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer),
                                   hmac_ba_timeout_fn_etc,
                                   us_timeout,
                                   pst_alarm_data,
                                   OAL_FALSE,
                                   OAM_MODULE_ID_HMAC,
                                   pst_mac_device->ul_core_id);

            oal_spin_unlock(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer_lock));
        }

#if 0  /*变量us_timeout_times没有使用，不再需要赋值*/
        if (pst_alarm_data->us_timeout_times == pst_vap->us_del_timeout && pst_vap->us_del_timeout != 0)
        {
            //pst_dmac_user = (dmac_user_stru *)mac_res_get_dmac_user(pst_alarm_data->us_mac_user_idx);
            //uc_tid        = pst_alarm_data->uc_tid;
            //dmac_mgmt_delba(pst_vap, pst_dmac_user, uc_tid, en_direction, MAC_QSTA_TIMEOUT);
            pst_alarm_data->us_timeout_times = 0;
        }
#endif
    }
    else
    {
        /* tx ba不删除 */
        FRW_TIMER_CREATE_TIMER(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer),
                               hmac_ba_timeout_fn_etc,
                               pst_vap->us_rx_timeout[WLAN_WME_TID_TO_AC(uc_tid)],
                               pst_alarm_data,
                               OAL_FALSE,
                               OAM_MODULE_ID_HMAC,
                               pst_mac_device->ul_core_id);
    }

    return OAL_SUCC;
}


oal_uint32  hmac_ba_reset_rx_handle_etc(mac_device_stru *pst_mac_device, hmac_ba_rx_stru **ppst_rx_ba, oal_uint8 uc_tid, oal_bool_enum_uint8 en_is_aging)
{
    hmac_vap_stru    *pst_hmac_vap;
    hmac_user_stru   *pst_hmac_user;
    mac_chip_stru    *pst_mac_chip;
    oal_bool_enum     en_need_del_lut = OAL_TRUE;

    if (OAL_UNLIKELY((OAL_PTR_NULL == *ppst_rx_ba) || (OAL_TRUE != (*ppst_rx_ba)->en_is_ba)))
    {
        OAM_WARNING_LOG0(0, OAM_SF_BA, "{hmac_ba_reset_rx_handle_etc::rx ba not set yet.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    if (uc_tid >= WLAN_TID_MAX_NUM)
    {
        OAM_ERROR_LOG1(0, OAM_SF_BA, "{hmac_ba_reset_rx_handle_etc::tid %d overflow.}", uc_tid);
        return OAL_ERR_CODE_ARRAY_OVERFLOW;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap((*ppst_rx_ba)->st_alarm_data.uc_vap_id);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_vap))
    {
        OAM_ERROR_LOG0(0, OAM_SF_BA, "{hmac_ba_reset_rx_handle_etc::pst_hmac_vap is null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*Step1: disable the flag of ba session*/
    (*ppst_rx_ba)->en_is_ba = OAL_FALSE;

    /*Step2: flush reorder q*/
    hmac_ba_flush_reorder_q(*ppst_rx_ba);

    if (MAC_INVALID_RX_BA_LUT_INDEX == (*ppst_rx_ba)->uc_lut_index)
    {
        en_need_del_lut = OAL_FALSE;
        OAM_WARNING_LOG1(0, OAM_SF_BA, "{hmac_ba_reset_rx_handle_etc::no need to del lut index, lut index[%d]}\n", (*ppst_rx_ba)->uc_lut_index);
    }

    /*Step3: if lut index is valid, del lut index alloc before*/
    if ((MAC_BA_POLICY_IMMEDIATE == (*ppst_rx_ba)->uc_ba_policy) && (OAL_TRUE == en_need_del_lut))
    {
        pst_mac_chip = hmac_res_get_mac_chip(pst_mac_device->uc_chip_id);
        if(OAL_PTR_NULL == pst_mac_chip)
        {
            return OAL_ERR_CODE_PTR_NULL;
        }
        hmac_ba_del_lut_index(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_table, (*ppst_rx_ba)->uc_lut_index);
    }

    /*Step4: dec the ba session cnt maitence in device struc*/
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
    hmac_rx_ba_session_decr_etc(pst_hmac_vap, uc_tid);
#else
    hmac_rx_ba_session_decr_etc(pst_mac_device, uc_tid);
#endif
    pst_hmac_user = (hmac_user_stru *)mac_res_get_hmac_user_etc((*ppst_rx_ba)->st_alarm_data.us_mac_user_idx);
    if (OAL_UNLIKELY(OAL_PTR_NULL == pst_hmac_user))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    oal_spin_lock(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer_lock));
    /*Step5: Del Timer*/
    if (pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer.en_is_registerd == OAL_TRUE)
    {
        FRW_TIMER_IMMEDIATE_DESTROY_TIMER(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer));
    }
    oal_spin_unlock(&(pst_hmac_user->ast_tid_info[uc_tid].st_ba_timer_lock));

    /*Step6: Free rx handle */
    OAL_MEM_FREE(*ppst_rx_ba, OAL_TRUE);
    *ppst_rx_ba = OAL_PTR_NULL;

    return OAL_SUCC;
}


oal_uint8  hmac_mgmt_check_set_rx_ba_ok_etc(
                hmac_vap_stru     *pst_hmac_vap,
                hmac_user_stru    *pst_hmac_user,
                hmac_ba_rx_stru   *pst_ba_rx_info,
                mac_device_stru   *pst_device,
                hmac_tid_stru     *pst_tid_info)
{
    mac_chip_stru *pst_mac_chip;
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    oal_uint8     uc_max_rx_ba_size;
#endif

    pst_ba_rx_info->uc_lut_index = MAC_INVALID_RX_BA_LUT_INDEX;

    /* 立即块确认判断 */
    if (MAC_BA_POLICY_IMMEDIATE == pst_ba_rx_info->uc_ba_policy)
    {
        if (OAL_FALSE == mac_mib_get_dot11ImmediateBlockAckOptionImplemented(&pst_hmac_vap->st_vap_base_info))
        {
            /* 不支持立即块确认 */
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_check_set_rx_ba_ok_etc::not support immediate Block Ack.}");
            return MAC_INVALID_REQ_PARAMS;
        }
        else
        {
            if (pst_ba_rx_info->en_back_var != MAC_BACK_COMPRESSED)
            {
                /* 不支持非压缩块确认 */
                OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_check_set_rx_ba_ok_etc::not support non-Compressed Block Ack.}");
                return MAC_REQ_DECLINED;
            }
        }
    }
    else if (MAC_BA_POLICY_DELAYED == pst_ba_rx_info->uc_ba_policy)
    {
        /* 延迟块确认不支持 */
        OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_check_set_rx_ba_ok_etc::not support delayed Block Ack.}");
        return MAC_INVALID_REQ_PARAMS;
    }
    pst_mac_chip = hmac_res_get_mac_chip(pst_device->uc_chip_id);
    if (OAL_PTR_NULL == pst_mac_chip)
    {
        return MAC_REQ_DECLINED;
    }
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
    if (mac_mib_get_RxBASessionNumber(&pst_hmac_vap->st_vap_base_info) > WLAN_MAX_RX_BA)
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_check_set_rx_ba_ok_etc::uc_rx_ba_session_num[%d] is up to max.}\r\n",
                         mac_mib_get_RxBASessionNumber(&pst_hmac_vap->st_vap_base_info));
        return MAC_REQ_DECLINED;
    }
#else
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    /* 允许创建实际硬件能力以上的BA Session  最多HAL_MAX_RX_BA_LUT_SIZE个session */
    if(pst_mac_chip->en_waveapp_32plus_user_enable)
    {
        uc_max_rx_ba_size = HAL_MAX_RX_BA_LUT_SIZE;
    }
    else
    {
        uc_max_rx_ba_size = WLAN_MAX_RX_BA;
    }
    if (pst_device->uc_rx_ba_session_num > uc_max_rx_ba_size)
#else
    if (pst_device->uc_rx_ba_session_num > WLAN_MAX_RX_BA)
#endif
    {
        OAM_WARNING_LOG1(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_check_set_rx_ba_ok_etc::uc_rx_ba_session_num[%d] is up to max.}\r\n",
                         pst_device->uc_rx_ba_session_num);
        return MAC_REQ_DECLINED;
    }
#endif

    /* 获取BA LUT INDEX */
    pst_ba_rx_info->uc_lut_index = hmac_ba_get_lut_index(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_table, 0, HAL_MAX_RX_BA_LUT_SIZE);
#ifdef _PRE_WLAN_FEATURE_RX_AGGR_EXTEND
    oal_reset_lut_index_status(pst_mac_chip->st_lut_table.auc_rx_ba_lut_idx_status_table, HAL_MAX_RX_BA_LUT_SIZE, pst_ba_rx_info->uc_lut_index);
#endif
        /* LUT index表已满 */
    if (MAC_INVALID_RX_BA_LUT_INDEX == pst_ba_rx_info->uc_lut_index)
    {
        OAM_ERROR_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_check_set_rx_ba_ok_etc::ba lut index table full.");
        return MAC_REQ_DECLINED;
    }

#ifdef _PRE_WLAN_FEATURE_BTCOEX
    /* 共存黑名单用户，不建立聚合，直到对应业务将标记清除 */
    if(OAL_TRUE == MAC_BTCOEX_CHECK_VALID_STA(&(pst_hmac_vap->st_vap_base_info)))
    {
        if(OAL_FALSE == pst_hmac_user->st_hmac_user_btcoex.st_hmac_btcoex_addba_req.en_ba_handle_allow)
        {
            OAM_WARNING_LOG0(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA, "{hmac_mgmt_check_set_rx_ba_ok_etc::btcoex blacklist user, not addba!");
            return MAC_REQ_DECLINED;
        }
    }
#endif

    /* 该tid下不允许建BA，配置命令需求 */
    if (OAL_FALSE == pst_tid_info->en_ba_handle_rx_enable)
    {
        OAM_WARNING_LOG2(pst_hmac_vap->st_vap_base_info.uc_vap_id, OAM_SF_BA,
                        "{hmac_mgmt_check_set_rx_ba_ok_etc::uc_tid_no[%d] user_idx[%d] is not available.}",
                         pst_tid_info->uc_tid_no, pst_tid_info->us_hmac_user_idx);
        return MAC_REQ_DECLINED;
    }

    return MAC_SUCCESSFUL_STATUSCODE;
}


oal_void  hmac_up_rx_bar_etc(hmac_vap_stru *pst_hmac_vap, dmac_rx_ctl_stru *pst_rx_ctl, oal_netbuf_stru *pst_netbuf)
{
    oal_uint8                 *puc_payload;
    mac_ieee80211_frame_stru  *pst_frame_hdr;
    oal_uint8                 *puc_sa_addr;
    oal_uint8                  uc_tidno;
    hmac_user_stru            *pst_ta_user;
    oal_uint16                 us_start_seqnum;
    hmac_ba_rx_stru           *pst_ba_rx_info;

    pst_frame_hdr = (mac_ieee80211_frame_stru  *)mac_get_rx_cb_mac_hdr(&(pst_rx_ctl->st_rx_info));
    puc_sa_addr = pst_frame_hdr->auc_address2;

    /*  获取用户指针 */
    pst_ta_user = mac_vap_get_hmac_user_by_addr_etc(&(pst_hmac_vap->st_vap_base_info), puc_sa_addr);
    if (OAL_PTR_NULL == pst_ta_user)
    {
        OAM_WARNING_LOG0(0, OAM_SF_ANY, "{hmac_up_rx_bar_etc::pst_ta_user  is null.}");
        return;
    }

    /* 获取帧头和payload指针*/
    puc_payload = MAC_GET_RX_PAYLOAD_ADDR(&(pst_rx_ctl->st_rx_info), pst_netbuf);

    /*************************************************************************/
    /*                     BlockAck Request Frame Format                     */
    /* --------------------------------------------------------------------  */
    /* |Frame Control|Duration|DA|SA|BAR Control|BlockAck Starting    |FCS|  */
    /* |             |        |  |  |           |Sequence number      |   |  */
    /* --------------------------------------------------------------------  */
    /* | 2           |2       |6 |6 |2          |2                    |4  |  */
    /* --------------------------------------------------------------------  */
    /*                                                                       */
    /*************************************************************************/

    uc_tidno        = (puc_payload[1] & 0xF0) >> 4;
    if (uc_tidno >= WLAN_TID_MAX_NUM)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_up_rx_bar_etc::uc_tidno[%d] invalid.}", uc_tidno);
        return;
    }
    us_start_seqnum = mac_get_bar_start_seq_num(puc_payload);
    pst_ba_rx_info  = pst_ta_user->ast_tid_info[uc_tidno].pst_ba_rx_info;

    hmac_reorder_ba_rx_buffer_bar_etc(pst_ba_rx_info, us_start_seqnum, &(pst_hmac_vap->st_vap_base_info));
}
#ifdef _PRE_WLAN_FEATURE_AMPDU_VAP
oal_bool_enum_uint8 hmac_is_device_ba_setup_etc(void)
{
    oal_uint8 uc_vap_id;
    hmac_vap_stru *pst_hmac_vap;

    for (uc_vap_id = 0; uc_vap_id < WLAN_VAP_SUPPORT_MAX_NUM_LIMIT; uc_vap_id++)
    {
        pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(uc_vap_id);
        if (OAL_PTR_NULL == pst_hmac_vap)
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_is_device_ba_setup_etc pst_mac_vap is null.}");
            continue;
        }
        if ((MAC_VAP_STATE_UP != pst_hmac_vap->st_vap_base_info.en_vap_state) &&
            (MAC_VAP_STATE_PAUSE != pst_hmac_vap->st_vap_base_info.en_vap_state))
        {
            continue;
        }
        if ((mac_mib_get_TxBASessionNumber(&pst_hmac_vap->st_vap_base_info) != 0) ||
            (mac_mib_get_RxBASessionNumber(&pst_hmac_vap->st_vap_base_info) != 0))
        {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}
#else
oal_bool_enum_uint8 hmac_is_device_ba_setup_etc(void)
{
    oal_uint32           uc_device_max;
    oal_uint8            uc_device;
    mac_device_stru     *pst_mac_device;
    mac_chip_stru       *pst_mac_chip;

    pst_mac_chip = hmac_res_get_mac_chip(0);

    /* OAL接口获取支持device个数 */
    uc_device_max = oal_chip_get_device_num_etc(pst_mac_chip->ul_chip_ver);
    if(uc_device_max > WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP)
    {
        OAM_ERROR_LOG2(0, OAM_SF_ANY, "{hmac_is_device_ba_setup_etc uc_device_max is %d,more than %d.}", uc_device_max, WLAN_SERVICE_DEVICE_MAX_NUM_PER_CHIP);
        return OAL_FALSE;
    }

    for (uc_device = 0; uc_device < uc_device_max; uc_device++)
    {
        pst_mac_device = mac_res_get_dev_etc(pst_mac_chip->auc_device_id[uc_device]);
        if (OAL_PTR_NULL == pst_mac_device)
        {
            continue;
        }

        if ((pst_mac_device->uc_tx_ba_session_num != 0) ||
            (pst_mac_device->uc_rx_ba_session_num != 0))
        {
            return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

