


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)
/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "oal_profiling.h"
#include "hmac_hcc_adapt.h"
#include "mac_resource.h"
#include "oal_hcc_host_if.h"
#include "frw_event_main.h"
#include "hmac_vap.h"

#if (_PRE_MULTI_CORE_MODE_OFFLOAD_DMAC == _PRE_MULTI_CORE_MODE)&&(_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include "plat_pm_wlan.h"
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_HMAC_HCC_ADAPT_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
OAL_STATIC oal_uint8  g_hcc_sched_stat[FRW_EVENT_TYPE_BUTT];
OAL_STATIC oal_uint8  g_hcc_flowctrl_stat[FRW_EVENT_TYPE_BUTT];
OAL_STATIC oal_uint32  g_hcc_sched_event_pkts[FRW_EVENT_TYPE_BUTT]={0};
OAL_STATIC oal_uint8  g_wlan_queue_to_dmac_queue[WLAN_NET_QUEUE_BUTT];

extern oal_uint32 hmac_hcc_tx_netbuf_etc(frw_event_mem_stru * pst_hcc_event_mem,
                                    oal_netbuf_stru *pst_netbuf,oal_uint32 ul_hdr_len,
                                    oal_uint32 fc_type,
                                    oal_uint32 queue_id);
oal_uint32 hmac_hcc_tx_netbuf_auto_etc(frw_event_mem_stru * pst_hcc_event_mem,
                                    oal_netbuf_stru *pst_netbuf,oal_uint32 ul_hdr_len);
extern oal_uint32 hmac_hcc_tx_data_etc(frw_event_mem_stru * pst_hcc_event_mem, oal_netbuf_stru *pst_netbuf);

/*****************************************************************************
  3 函数实现
*****************************************************************************/
oal_void hmac_tx_net_queue_map_init_etc(oal_void)
{
    oal_memset(g_wlan_queue_to_dmac_queue,DATA_LO_QUEUE,OAL_SIZEOF(g_wlan_queue_to_dmac_queue));
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    g_wlan_queue_to_dmac_queue[WLAN_HI_QUEUE] = DATA_HI_QUEUE;
    g_wlan_queue_to_dmac_queue[WLAN_NORMAL_QUEUE] = DATA_LO_QUEUE;
    g_wlan_queue_to_dmac_queue[WLAN_TCP_DATA_QUEUE] = DATA_TCP_DATA_QUEUE;
    g_wlan_queue_to_dmac_queue[WLAN_TCP_ACK_QUEUE] = DATA_TCP_ACK_QUEUE;
    g_wlan_queue_to_dmac_queue[WLAN_UDP_BK_QUEUE] = DATA_UDP_BK_QUEUE;
    g_wlan_queue_to_dmac_queue[WLAN_UDP_BE_QUEUE] = DATA_UDP_BE_QUEUE;
    g_wlan_queue_to_dmac_queue[WLAN_UDP_VI_QUEUE] = DATA_UDP_VI_QUEUE;
    g_wlan_queue_to_dmac_queue[WLAN_UDP_VO_QUEUE] = DATA_UDP_VO_QUEUE;


    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_HI_QUEUE,WLAN_HI_QUEUE);
    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_LO_QUEUE,WLAN_NORMAL_QUEUE);
    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_TCP_DATA_QUEUE,WLAN_TCP_DATA_QUEUE);
    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_TCP_ACK_QUEUE,WLAN_TCP_ACK_QUEUE);
    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_UDP_BK_QUEUE,WLAN_UDP_BK_QUEUE);
    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_UDP_BE_QUEUE,WLAN_UDP_BE_QUEUE);
    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_UDP_VI_QUEUE,WLAN_UDP_VI_QUEUE);
    hcc_tx_wlan_queue_map_set_etc(hcc_get_110x_handler(),DATA_UDP_VO_QUEUE,WLAN_UDP_VO_QUEUE);
#endif

}

#ifdef _PRE_CONFIG_CONN_HISI_SYSFS_SUPPORT
oal_int32 hmac_tx_event_pkts_info_print_etc(oal_void* data, char* buf, oal_int32 buf_len)
{
    int i;
    oal_int32 ret = 0;
    oal_uint64 total = 0;
    struct hcc_handler* hcc = hcc_get_110x_handler();
    if(NULL == hcc)
        return ret;

    ret +=  OAL_SPRINTF(buf + ret , buf_len - ret,"tx_event_pkts_info_show\n");
    for(i = 0; i < FRW_EVENT_TYPE_BUTT; i++)
    {
        if(g_hcc_sched_event_pkts[i])
            ret +=  OAL_SPRINTF(buf + ret , buf_len - ret,"event:%d, pkts:%10u\n", i,g_hcc_sched_event_pkts[i]);
        total += g_hcc_sched_event_pkts[i];
    }

    if(total)
        ret +=  OAL_SPRINTF(buf + ret , buf_len - ret,"total:%llu\n",total);
    return ret;
}
#endif

#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
OAL_STATIC DECLARE_WIFI_PANIC_STRU(hmac_panic_hcc_adapt,hmac_tx_event_pkts_info_print_etc);
#endif

oal_void hmac_tx_sched_info_init_etc(oal_void)
{
    oal_memset(g_hcc_sched_stat,DATA_LO_QUEUE,OAL_SIZEOF(g_hcc_sched_stat));

    /*set the event sched PRI, TBD*/
    g_hcc_sched_stat[FRW_EVENT_TYPE_HIGH_PRIO] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_HOST_CRX] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_HOST_DRX] = DATA_LO_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_HOST_CTX] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_HOST_SDT_REG] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_WLAN_CRX] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_WLAN_DRX] = DATA_LO_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_WLAN_CTX] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_WLAN_TX_COMP] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_TBTT] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_TIMEOUT] = DATA_HI_QUEUE;
    g_hcc_sched_stat[FRW_EVENT_TYPE_DMAC_MISC] = DATA_HI_QUEUE;

    oal_memset(g_hcc_flowctrl_stat,HCC_FC_NONE,OAL_SIZEOF(g_hcc_flowctrl_stat));
#if 0
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HIGH_PRIO] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HOST_CRX] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HOST_DRX] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HOST_CTX] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HOST_SDT_REG] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_WLAN_CRX] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_WLAN_DRX] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_WLAN_CTX] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_WLAN_TX_COMP] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_TBTT] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_TIMEOUT] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HMAC_MISC] = HCC_FC_NONE;
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_DMAC_MISC] = HCC_FC_NONE;
#endif
    /*来自HOST的事件，如果从Kernel Net过来选择网络层流控+丢包的方式，
    如果是Wlan To Wlan 的方式，直接丢包!*/
    g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HOST_DRX] = HCC_FC_DROP|HCC_FC_NET;
    //g_hcc_flowctrl_stat[FRW_EVENT_TYPE_HOST_DRX] = HCC_FC_DROP;

    //g_hcc_flowctrl_stat[FRW_EVENT_TYPE_WLAN_DTX] = HCC_FC_NET;
}

OAL_STATIC OAL_INLINE oal_void hmac_hcc_adapt_extend_hdr_init(frw_event_hdr_stru *pst_event_hdr,oal_netbuf_stru *pst_netbuf)
{
    struct frw_hcc_extend_hdr* pst_hdr;

    pst_hdr = (struct frw_hcc_extend_hdr*)OAL_NETBUF_DATA(pst_netbuf);
    pst_hdr->en_nest_type = pst_event_hdr->en_type;
    pst_hdr->uc_nest_sub_type = pst_event_hdr->uc_sub_type;
    pst_hdr->device_id = pst_event_hdr->uc_device_id;
    pst_hdr->chip_id = pst_event_hdr->uc_chip_id;
    pst_hdr->vap_id = pst_event_hdr->uc_vap_id;
}

#if 0

oal_void get_mac_rx_ctl(mac_rx_ctl_stru  *pst_mac_rx_ctl, mac_rx_ctl_cut_stru  *pst_mac_rx_cut_ctl)
{
    pst_mac_rx_ctl->bit_amsdu_enable    = pst_mac_rx_cut_ctl->bit_amsdu_enable;
    pst_mac_rx_ctl->bit_buff_nums       = pst_mac_rx_cut_ctl->bit_buff_nums;
    //pst_mac_rx_ctl->us_da_user_idx      = pst_mac_rx_cut_ctl->bit_da_user_idx;
    pst_mac_rx_ctl->bit_is_first_buffer = pst_mac_rx_cut_ctl->bit_is_first_buffer;
    pst_mac_rx_ctl->bit_is_fragmented   = pst_mac_rx_cut_ctl->bit_is_fragmented;
    pst_mac_rx_ctl->uc_mac_header_len   = pst_mac_rx_cut_ctl->bit_mac_header_len;
    pst_mac_rx_ctl->us_ta_user_idx      = pst_mac_rx_cut_ctl->bit_ta_user_idx;
    pst_mac_rx_ctl->bit_vap_id          = pst_mac_rx_cut_ctl->bit_vap_id;
    pst_mac_rx_ctl->uc_msdu_in_buffer   = pst_mac_rx_cut_ctl->uc_msdu_in_buffer;
    pst_mac_rx_ctl->us_frame_len        = pst_mac_rx_cut_ctl->us_frame_len;
    pst_mac_rx_ctl->uc_mac_vap_id       = pst_mac_rx_cut_ctl->uc_mac_vap_id;
    pst_mac_rx_ctl->uc_channel_number   = pst_mac_rx_cut_ctl->uc_channel_number;
    pst_mac_rx_ctl->bit_is_beacon       = pst_mac_rx_cut_ctl->bit_is_beacon;
    pst_mac_rx_ctl->bit_is_last_buffer  = pst_mac_rx_cut_ctl->bit_is_last_buffer;
}
#endif


oal_uint32 check_headroom_add_length_etc(mac_tx_ctl_stru *pst_tx_ctl, frw_event_type_enum_uint8 en_nest_type, oal_uint8 uc_nest_sub_type, oal_uint8 uc_cb_length)
{
    oal_uint32 ul_headroom_add;

    if (1 == MAC_GET_CB_80211_MAC_HEAD_TYPE(pst_tx_ctl))
    {
        /*case 1: data from net, mac head is maintence in netbuff*/
         /*lint -e778*/
        ul_headroom_add = uc_cb_length + MAC_80211_QOS_HTC_4ADDR_FRAME_LEN;//结构体肯定大于4
         /*lint +e778*/
    }
    else if ((FRW_EVENT_TYPE_WLAN_CTX == en_nest_type) && (DMAC_WLAN_CTX_EVENT_SUB_TYPE_MGMT == uc_nest_sub_type))
    {
        /*case 2: mgmt frame, mac header is maintence in payload part*/
        ul_headroom_add = uc_cb_length + (MAX_MAC_HEAD_LEN - MAC_80211_FRAME_LEN);
    }
    else
    {
        /*case 3: data from net, mac head not maintence in netbuff*/
        /*case 4: netbuff alloced in adapt layer */
        ul_headroom_add = uc_cb_length + MAX_MAC_HEAD_LEN;
    }

    return ul_headroom_add;
}


oal_void hmac_adjust_netbuf_data_etc(oal_netbuf_stru *pst_netbuf, mac_tx_ctl_stru *pst_tx_ctrl, frw_event_type_enum_uint8  en_nest_type, oal_uint8  uc_nest_sub_type)
{
    oal_uint8                       *puc_data_hdr;
    oal_uint8                        uc_cb_length;

    /*在进入HCC之前，将CB字段和Mac头连续存放至payload之前*/
    puc_data_hdr      = OAL_NETBUF_DATA(pst_netbuf);

    uc_cb_length = OAL_SIZEOF(mac_tx_ctl_stru) - OAL_SIZEOF(mac_tx_expand_cb_stru);

    if ((FRW_EVENT_TYPE_WLAN_CTX == en_nest_type) && (DMAC_WLAN_CTX_EVENT_SUB_TYPE_MGMT == uc_nest_sub_type))
    {
        /*case 1: mgmt frame, mac header is maintence in payload part*/
        oal_memcopy(puc_data_hdr, (oal_uint8 *)pst_tx_ctrl, uc_cb_length);
        /*copy mac hdr*/
        oal_memmove(puc_data_hdr + uc_cb_length,
                    puc_data_hdr + uc_cb_length + (MAX_MAC_HEAD_LEN - MAC_80211_FRAME_LEN),
                    MAC_80211_FRAME_LEN);
    }
    else if (OAL_PTR_NULL != MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl))
    {
        /*case 2: data from net, mac head not maintence in netbuff*/
        /*case 3: netbuff alloced in adapt layer */
        oal_memcopy(puc_data_hdr, (oal_uint8 *)pst_tx_ctrl, uc_cb_length);

        /* 帧头和帧体不连续，帧头重新申请了事件内存，此处需要释放 */
        if (0 == MAC_GET_CB_80211_MAC_HEAD_TYPE(pst_tx_ctrl))
        {
            oal_memmove(puc_data_hdr + uc_cb_length, (oal_uint8 *)MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl), MAC_GET_CB_FRAME_HEADER_LENGTH(pst_tx_ctrl));
            OAL_MEM_FREE((oal_uint8 *)MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl), OAL_TRUE);
        }
    }
}


oal_uint32 hmac_hcc_tx_netbuf_auto_etc(frw_event_mem_stru * pst_hcc_event_mem,
                                    oal_netbuf_stru *pst_netbuf,oal_uint32 ul_hdr_len)
{
    oal_uint32      fc_type,queue_id;
    frw_event_hdr_stru              *pst_event_hdr;
    frw_event_type_enum_uint8        en_type;
    pst_event_hdr           = frw_get_event_hdr(pst_hcc_event_mem);
    en_type                 = pst_event_hdr->en_type;

    if(OAL_WARN_ON(en_type >= FRW_EVENT_TYPE_BUTT))
    {
        oal_netbuf_free(pst_netbuf);
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_hcc_tx_netbuf_auto_etc::FRW_EVENT_TYPE[%d] over limit!}",en_type);
        return OAL_FAIL;
    }

    queue_id = g_hcc_sched_stat[en_type];
    fc_type = g_hcc_flowctrl_stat[en_type];

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    /* 对于从以太网报文获取其队列号 */
    if (FRW_EVENT_TYPE_HOST_DRX == en_type)
    {
        queue_id = oal_skb_get_queue_mapping(pst_netbuf);
        if(OAL_WARN_ON(queue_id >= WLAN_NET_QUEUE_BUTT))
        {
            queue_id = DATA_LO_QUEUE;
        }
        else
        {
            queue_id = g_wlan_queue_to_dmac_queue[queue_id];
        }
    }
#endif

    return hmac_hcc_tx_netbuf_etc(pst_hcc_event_mem,pst_netbuf,ul_hdr_len,fc_type,queue_id);
}


oal_uint32 hmac_hcc_tx_netbuf_etc(frw_event_mem_stru * pst_hcc_event_mem,
                                    oal_netbuf_stru *pst_netbuf,oal_uint32 ul_hdr_len,
                                    oal_uint32 fc_type,
                                    oal_uint32 queue_id)
{
    frw_event_hdr_stru             *pst_event_hdr = OAL_PTR_NULL;
    oal_uint32                       ret = OAL_SUCC;
    oal_uint32 ul_netbuf_old_addr;
    oal_uint32 ul_netbuf_new_addr;
    oal_uint32 ul_addr_offset;
    oal_uint32 ul_hcc_head_len = 0;
    oal_uint32 ul_hcc_head_pad = 0;
    oal_int32  ul_head_room = 0;

    DECLARE_HCC_TX_PARAM_INITIALIZER(st_hcc_transfer_param,
                                     HCC_ACTION_TYPE_WIFI,
                                     0,
                                     ul_hdr_len + OAL_SIZEOF(struct frw_hcc_extend_hdr),
                                     fc_type,
                                     queue_id);

    if (OAL_UNLIKELY(NULL == pst_netbuf))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_tx_data_etc:: pst_netbuf is null}");
        return -OAL_EINVAL;
    }

    ul_hcc_head_len = HCC_HDR_LEN + OAL_SIZEOF(struct frw_hcc_extend_hdr);

    /* 一次性扩展HCC适配层9字节 */
    if (oal_netbuf_headroom(pst_netbuf) > 0)
    {
        /* 保证cb 4bytes对齐 */
        oal_netbuf_push(pst_netbuf,1);

        ul_hcc_head_pad = ul_hcc_head_len;

    }
    else
    {
        ul_hcc_head_pad = ul_hcc_head_len + 1;
    }

    ul_head_room = (oal_int32)oal_netbuf_headroom(pst_netbuf);
    //OAM_ERROR_LOG3(0, OAM_SF_ANY, "{hmac_hcc_tx_netbuf_etc::ul_hcc_head_len[%d],ul_headroom_add[%d],expand data[%x]}",ul_hcc_head_len,ul_headroom_add,OAL_NETBUF_DATA(pst_netbuf));

    if(ul_hcc_head_pad > ul_head_room)
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hmac_hcc_tx_netbuf_etc expand head done![%d]}",ul_head_room);
        ret = (oal_uint32)oal_netbuf_expand_head(pst_netbuf, (oal_int32) ul_hcc_head_pad - ul_head_room, 0, GFP_ATOMIC);
        if(OAL_WARN_ON(OAL_SUCC != ret))
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_tx_data_etc:: alloc head room failed.}");
            oal_netbuf_free(pst_netbuf);
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }
    }

    if (ul_hcc_head_pad == ul_hcc_head_len)
    {
        /* 恢复至CB首地址 */
        oal_netbuf_pull(pst_netbuf,1);
    }

    ul_netbuf_old_addr = (oal_uint)(OAL_NETBUF_DATA(pst_netbuf) + ul_hdr_len);
    ul_netbuf_new_addr = OAL_ROUND_DOWN(ul_netbuf_old_addr, 4);
    ul_addr_offset = ul_netbuf_old_addr - ul_netbuf_new_addr;

    //OAM_ERROR_LOG3(0, OAM_SF_ANY, "{hmac_hcc_tx_netbuf_etc::expand data[%x],old[%x]new[%x]}",OAL_NETBUF_DATA(pst_netbuf),ul_netbuf_old_addr,ul_netbuf_new_addr);

    /* 未对齐时在host侧做数据搬移，此处牺牲host，解放device */
    if (ul_addr_offset)
    {
        ul_head_room = (oal_int32)oal_netbuf_headroom(pst_netbuf);
        //OAM_WARNING_LOG2(0, OAM_SF_ANY, "{hmac_hcc_tx_netbuf_etc 4 bytes offset[%d],headroom[%d]}",ul_addr_offset,ul_head_room);
        if(ul_addr_offset > ul_head_room)
        {
            ret = oal_netbuf_expand_head(pst_netbuf, ul_addr_offset - ul_head_room, 0, GFP_ATOMIC);
            if(OAL_WARN_ON(OAL_SUCC != ret))
            {
                OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_tx_data_etc:: alloc head room failed.}");
                oal_netbuf_free(pst_netbuf);
                return OAL_ERR_CODE_ALLOC_MEM_FAIL;
            }
        }

        oal_memmove((oal_uint8*)OAL_NETBUF_DATA(pst_netbuf) - ul_addr_offset, (oal_uint8*)OAL_NETBUF_DATA(pst_netbuf), OAL_NETBUF_LEN(pst_netbuf));
        oal_netbuf_push(pst_netbuf, ul_addr_offset);
        oal_netbuf_trim(pst_netbuf, ul_addr_offset);
    }

    /*add frw hcc extend area*/
    oal_netbuf_push(pst_netbuf, OAL_SIZEOF(struct frw_hcc_extend_hdr));

    pst_event_hdr = frw_get_event_hdr(pst_hcc_event_mem);
    hmac_hcc_adapt_extend_hdr_init(pst_event_hdr, pst_netbuf);

    //expand 14B后性能下降40%,待确认!
    //oal_netbuf_expand_head(pst_netbuf, 4, 0, GFP_ATOMIC);
#ifdef CONFIG_PRINTK
    ret = (oal_uint32)hcc_tx_etc(hcc_get_110x_handler(), pst_netbuf, &st_hcc_transfer_param);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        /*hcc 关闭时下发了命令,报警需要清理*/
        if(OAL_UNLIKELY(-OAL_EBUSY == ret))
        {
            if(oal_print_rate_limit(30*PRINT_RATE_SECOND))
            {
                OAL_WARN_ON(1);
            }
            OAL_IO_PRINT("[E]hmac_tx event[%u:%u] drop!\n", pst_event_hdr->en_type, pst_event_hdr->uc_sub_type);
            ret = OAL_SUCC;
            DECLARE_DFT_TRACE_KEY_INFO("hcc_is_busy",OAL_DFT_TRACE_OTHER);
        }

        if(-OAL_EIO == ret)
        {
            /*hcc exception, drop the pkts*/
            ret = OAL_SUCC;
        }

        oal_netbuf_free(pst_netbuf);
    }
    else
    {
        //frw_event_hdr_stru              *pst_event_hdr = frw_get_event_hdr(pst_hcc_event_mem);
        if(OAL_LIKELY(pst_event_hdr->en_type < FRW_EVENT_TYPE_BUTT))
            g_hcc_sched_event_pkts[pst_event_hdr->en_type]++;
    }
    OAL_MIPS_TX_STATISTIC(HOST_PROFILING_FUNC_HCC_TX);

    return ret;
#else
    /*UT Failed! Should remove this macro when DMT!*/
    return ret;
#endif
}


oal_uint32 hmac_hcc_tx_data_etc(frw_event_mem_stru * pst_hcc_event_mem, oal_netbuf_stru *pst_netbuf)
{
    frw_event_hdr_stru              *pst_event_hdr;
    frw_event_type_enum_uint8        en_type;
    oal_uint8                        uc_sub_type;
    mac_tx_ctl_stru                *pst_tx_ctrl;
    oal_uint32                      ul_headroom_add;
    oal_int32                       ret = OAL_SUCC;
    oal_uint8                       auc_macheader[MAC_80211_QOS_HTC_4ADDR_FRAME_LEN] = {0};
    oal_uint8                       uc_cb_length;

    /*提取嵌套的业务事件类型*/
    pst_event_hdr           = frw_get_event_hdr(pst_hcc_event_mem);

    en_type                 = pst_event_hdr->en_type;
    uc_sub_type             = pst_event_hdr->uc_sub_type;

    if (OAL_UNLIKELY(NULL == pst_netbuf))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_tx_data_etc:: pst_netbuf is null}");
        return OAL_FAIL;
    }

    pst_tx_ctrl  = (mac_tx_ctl_stru *)OAL_NETBUF_CB(pst_netbuf);
    if (OAL_WARN_ON(MAC_GET_CB_IS_4ADDRESS(pst_tx_ctrl)))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_tx_data_etc:: use 4 address.}");
        oal_netbuf_free(pst_netbuf);
        return OAL_FAIL;
    }

    uc_cb_length = OAL_SIZEOF(mac_tx_ctl_stru) - OAL_SIZEOF(mac_tx_expand_cb_stru);

    ul_headroom_add = check_headroom_add_length_etc(pst_tx_ctrl, en_type, uc_sub_type, uc_cb_length);

    if(ul_headroom_add > oal_netbuf_headroom(pst_netbuf))
    {
        if (1 == MAC_GET_CB_80211_MAC_HEAD_TYPE(pst_tx_ctrl))
        {
            oal_memcopy(auc_macheader, (oal_uint8 *)MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl), MAX_MAC_HEAD_LEN);
        }

        ret = oal_netbuf_expand_head(pst_netbuf,
                                    (oal_int32)ul_headroom_add - (oal_int32)oal_netbuf_headroom(pst_netbuf),
                                    0, GFP_ATOMIC);
        if(OAL_WARN_ON(OAL_SUCC != ret))
        {
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_tx_data_etc:: alloc head room failed.}");
            oal_netbuf_free(pst_netbuf);
            return OAL_ERR_CODE_ALLOC_MEM_FAIL;
        }

        if (1 == MAC_GET_CB_80211_MAC_HEAD_TYPE(pst_tx_ctrl))
        {
            oal_memcopy(OAL_NETBUF_DATA(pst_netbuf), auc_macheader, MAX_MAC_HEAD_LEN);
            MAC_GET_CB_FRAME_HEADER_ADDR(pst_tx_ctrl) = (mac_ieee80211_frame_stru *)OAL_NETBUF_DATA(pst_netbuf);
        }
    }

    /*修改netbuff的data指针和len*/
    oal_netbuf_push(pst_netbuf, ul_headroom_add);
    hmac_adjust_netbuf_data_etc(pst_netbuf, pst_tx_ctrl,en_type, uc_sub_type);

    OAL_MIPS_TX_STATISTIC(HOST_PROFILING_FUNC_HCC_TX_DATA);
    /*netbuf不管成功与否都由发送函数释放!*/
    hmac_hcc_tx_netbuf_auto_etc(pst_hcc_event_mem,pst_netbuf, uc_cb_length + MAX_MAC_HEAD_LEN);
    return OAL_SUCC;
}

oal_uint32 hmac_hcc_tx_netbuf_adapt_etc(frw_event_mem_stru * pst_hcc_event_mem,
                                    oal_netbuf_stru *pst_netbuf)
{
    return hmac_hcc_tx_netbuf_auto_etc(pst_hcc_event_mem,pst_netbuf,0);
}

oal_uint32 hmac_hcc_tx_event_buf_to_netbuf_etc(frw_event_mem_stru   *pst_event_mem,
                                                         oal_uint8 *           pst_buf,
                                                         oal_uint32            payload_size)
{
    oal_netbuf_stru                 *pst_netbuf;
    /*申请netbuf存放事件payload*/
    pst_netbuf = hcc_netbuf_alloc(payload_size);
    if (OAL_WARN_ON(NULL == pst_netbuf))
    {
       OAL_IO_PRINT("hmac_hcc_tx_event_buf_to_netbuf_etc alloc netbuf failed!\n");
       return OAL_ERR_CODE_ALLOC_MEM_FAIL;
    }

    /*将结构体拷贝到netbuff数据区*/
    //OAL_MEMZERO(oal_netbuf_cb(pst_netbuf), OAL_TX_CB_LEN);
    oal_netbuf_put(pst_netbuf, payload_size);
    oal_memcopy((oal_uint8 *)(OAL_NETBUF_DATA(pst_netbuf)), (oal_uint8 *)pst_buf, payload_size);

    return hmac_hcc_tx_netbuf_adapt_etc(pst_event_mem,pst_netbuf);
}


oal_uint32 hmac_hcc_tx_event_payload_to_netbuf_etc(frw_event_mem_stru   *pst_event_mem,
                                                         oal_uint32            payload_size)
{
    oal_uint8          *pst_event_payload;

    if(OAL_WARN_ON(NULL == pst_event_mem))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_hcc_tx_event_payload_to_netbuf_etc:pst_event_mem null!");
        return OAL_ERR_CODE_PTR_NULL;
    }

    /*取业务事件信息*/
    pst_event_payload    = frw_get_event_payload(pst_event_mem);
    return hmac_hcc_tx_event_buf_to_netbuf_etc(pst_event_mem,pst_event_payload,payload_size);
}


oal_uint32 hmac_hcc_rx_event_comm_adapt_etc(frw_event_mem_stru *pst_hcc_event_mem)
{
    oal_uint8                       bit_mac_header_len;
    frw_event_hdr_stru              *pst_event_hdr;
    hcc_event_stru                  *pst_hcc_event_payload;

    mac_rx_ctl_stru                 *pst_rx_ctrl;
    oal_uint8                       *puc_hcc_extend_hdr;

    /*step1 提取嵌套的业务事件类型*/
    pst_event_hdr           = frw_get_event_hdr(pst_hcc_event_mem);
    pst_hcc_event_payload   = (hcc_event_stru*)frw_get_event_payload(pst_hcc_event_mem);


    /*完成从51Mac rx ctl 到02 Mac rx ctl的拷贝,
    传到此处,pad_payload已经是0*/

    /* hcc protocol header
    |-------hcc total(64B)-----|-----------package mem--------------|
    |hcc hdr|pad hdr|hcc extend|pad_payload|--------payload---------|*/

    if(OAL_WARN_ON(NULL == pst_hcc_event_payload->pst_netbuf))
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "hmac_hcc_rx_event_comm_adapt_etc:did't found netbuf!");
        return OAL_FAIL;
    }

    puc_hcc_extend_hdr  = OAL_NETBUF_DATA((oal_netbuf_stru *)pst_hcc_event_payload->pst_netbuf);
    bit_mac_header_len = ((mac_rx_ctl_stru *)puc_hcc_extend_hdr)->uc_mac_header_len;
    if (bit_mac_header_len)
    {
        if(bit_mac_header_len > MAX_MAC_HEAD_LEN)
        {
            OAM_ERROR_LOG3(pst_event_hdr->uc_vap_id, OAM_SF_ANY, "invaild mac header len:%d,main:%d,sub:%d",
                            bit_mac_header_len,pst_event_hdr->en_type, pst_event_hdr->uc_sub_type);
            oal_print_hex_dump(puc_hcc_extend_hdr, (oal_int32)OAL_NETBUF_LEN((oal_netbuf_stru *)pst_hcc_event_payload->pst_netbuf), 32, "invaild mac header len");
            return OAL_FAIL;
        }

        pst_rx_ctrl  = (mac_rx_ctl_stru *)OAL_NETBUF_CB((oal_netbuf_stru *)pst_hcc_event_payload->pst_netbuf);
        //get_mac_rx_ctl(pst_rx_ctrl, (mac_rx_ctl_cut_stru *)puc_hcc_extend_hdr) ;
        oal_memcopy(pst_rx_ctrl, puc_hcc_extend_hdr, OAL_SIZEOF(mac_rx_ctl_stru));

        /*需要修改pst_rx_ctrl中所有指针*/
        MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl) = (oal_uint32 *)(puc_hcc_extend_hdr + OAL_MAX_CB_LEN + MAX_MAC_HEAD_LEN - pst_rx_ctrl->uc_mac_header_len);

        /* 将mac header的内容向高地址偏移8个字节拷贝，使得mac header和payload的内容连续 */
        oal_memmove((oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl),
                    (oal_uint8 *)((oal_uint8 *)MAC_GET_RX_CB_MAC_HEADER_ADDR(pst_rx_ctrl) - (MAX_MAC_HEAD_LEN - pst_rx_ctrl->uc_mac_header_len)),
                    pst_rx_ctrl->uc_mac_header_len);

        /*将netbuff data指针移到payload位置*/
        oal_netbuf_pull(pst_hcc_event_payload->pst_netbuf, OAL_MAX_CB_LEN + (MAX_MAC_HEAD_LEN - pst_rx_ctrl->uc_mac_header_len));

        //OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_rx_event_handler::cut short}");
    }
    else
    {
        oal_netbuf_pull(pst_hcc_event_payload->pst_netbuf, (OAL_MAX_CB_LEN + MAX_MAC_HEAD_LEN));

        //OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_hcc_rx_event_handler::cut short}");
    }


    return OAL_SUCC;
}


frw_event_mem_stru * hmac_hcc_expand_rx_adpat_event_etc(frw_event_mem_stru *pst_hcc_event_mem, oal_uint32 event_size)
{
    frw_event_hdr_stru             *pst_hcc_event_hdr;
    hcc_event_stru                 *pst_hcc_event_payload;
    oal_netbuf_stru                *pst_hcc_netbuf;
    frw_event_type_enum_uint8       en_type;
    oal_uint8                       uc_sub_type;
    oal_uint8                       uc_chip_id;
    oal_uint8                       uc_device_id;
    oal_uint8                       uc_vap_id;
    frw_event_mem_stru             *pst_event_mem;              /* 业务事件相关信息 */

   /* 提取HCC事件信息 */
    pst_hcc_event_hdr       = frw_get_event_hdr(pst_hcc_event_mem);
    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);
    pst_hcc_netbuf          = pst_hcc_event_payload->pst_netbuf;
    en_type                 = pst_hcc_event_hdr->en_type;
    uc_sub_type             = pst_hcc_event_hdr->uc_sub_type;
    uc_chip_id              = pst_hcc_event_hdr->uc_chip_id;
    uc_device_id            = pst_hcc_event_hdr->uc_device_id;
    uc_vap_id               = pst_hcc_event_hdr->uc_vap_id;

    /* 申请业务事件 */
    pst_event_mem = FRW_EVENT_ALLOC((oal_uint16)event_size);
    if (OAL_WARN_ON(OAL_PTR_NULL == pst_event_mem))
    {
        OAM_WARNING_LOG1(0,OAM_SF_ANY,"hmac_hcc_rx_netbuf_convert_to_event_etc  alloc event failed,event len:%d",event_size);
        /* 释放hcc事件中申请的netbuf内存 */
        oal_netbuf_free(pst_hcc_netbuf);
        return OAL_PTR_NULL;
    }

    /* 填业务事件头*/
    FRW_EVENT_HDR_INIT(frw_get_event_hdr(pst_event_mem),
                       en_type,
                       uc_sub_type,
                       (oal_uint16)event_size,
                       FRW_EVENT_PIPELINE_STAGE_1,
                       uc_chip_id,
                       uc_device_id,
                       uc_vap_id);

    return pst_event_mem;
}


frw_event_mem_stru * hmac_hcc_rx_netbuf_convert_to_event_etc(frw_event_mem_stru *pst_hcc_event_mem, oal_uint32 revert_size)
{
    //frw_event_hdr_stru             *pst_hcc_event_hdr;
    hcc_event_stru                 *pst_hcc_event_payload;
    oal_netbuf_stru                *pst_hcc_netbuf;
    //frw_event_type_enum_uint8       en_type;
    //oal_uint8                       uc_sub_type;
    frw_event_mem_stru             *pst_event_mem;              /* 业务事件相关信息 */

    if(OAL_WARN_ON(NULL == pst_hcc_event_mem))
    {
        return NULL;
    }

    /*filter the extend buf*/
    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    //pst_hcc_event_hdr       = frw_get_event_hdr(pst_hcc_event_mem);
    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);
    pst_hcc_netbuf          = pst_hcc_event_payload->pst_netbuf;
    //en_type                 = pst_hcc_event_hdr->en_type;
    //uc_sub_type             = pst_hcc_event_hdr->uc_sub_type;

    if(OAL_WARN_ON(NULL == pst_hcc_netbuf))
    {
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"Fatal Error,payload did't contain any netbuf!");
        return OAL_PTR_NULL;
    }

    if(revert_size > OAL_NETBUF_LEN(pst_hcc_netbuf))
    {
        revert_size = OAL_NETBUF_LEN(pst_hcc_netbuf);
    }

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem,revert_size);
    if (OAL_PTR_NULL == pst_event_mem)
    {
        return OAL_PTR_NULL;
    }

    if(revert_size)
        oal_memcopy((oal_uint8 *)frw_get_event_payload(pst_event_mem),
                (oal_uint8 *)OAL_NETBUF_DATA(pst_hcc_netbuf), revert_size);

    /* 释放hcc事件中申请的netbuf内存 */
    oal_netbuf_free(pst_hcc_netbuf);

    return pst_event_mem;
}


frw_event_mem_stru * hmac_hcc_rx_convert_netbuf_to_event_default_etc(frw_event_mem_stru *pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;

    if(OAL_WARN_ON(OAL_PTR_NULL == pst_hcc_event_mem))
    {
        return OAL_PTR_NULL;
    }

    pst_hcc_event_payload = (hcc_event_stru*)frw_get_event_payload(pst_hcc_event_mem);
    return hmac_hcc_rx_netbuf_convert_to_event_etc(pst_hcc_event_mem,pst_hcc_event_payload->ul_buf_len);
}


frw_event_mem_stru * hmac_hcc_test_rx_adapt_etc(frw_event_mem_stru * pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;

    frw_event_mem_stru              *pst_event_mem;
    hcc_event_stru                  *pst_hcc_rx_event;

    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);

    /*filter the extend buf*/
    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem,OAL_SIZEOF(hcc_event_stru));
    if (NULL == pst_event_mem)
    {
        return NULL;
    }

    /*填业务事件信息*/
    pst_hcc_rx_event                 = (hcc_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_hcc_rx_event->pst_netbuf     = pst_hcc_event_payload->pst_netbuf;
    pst_hcc_rx_event->ul_buf_len     = (oal_uint32)OAL_NETBUF_LEN((oal_netbuf_stru*)pst_hcc_event_payload->pst_netbuf);

    return pst_event_mem;
}

frw_event_mem_stru * hmac_rx_convert_netbuf_to_netbuf_default_etc(frw_event_mem_stru * pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;

    frw_event_mem_stru              *pst_event_mem;

    dmac_tx_event_stru              *pst_ctx_event;

    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);

    /*filter the extend buf*/
    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem,OAL_SIZEOF(dmac_tx_event_stru));
    if (NULL == pst_event_mem)
    {
        return NULL;
    }

    pst_ctx_event               = (dmac_tx_event_stru *)frw_get_event_payload(pst_event_mem);

    pst_ctx_event->pst_netbuf   = pst_hcc_event_payload->pst_netbuf;
    pst_ctx_event->us_frame_len = (oal_uint16)OAL_NETBUF_LEN((oal_netbuf_stru*)pst_hcc_event_payload->pst_netbuf);

    OAM_INFO_LOG2(0, OAM_SF_ANY, "{hmac_rx_convert_netbuf_to_netbuf_default_etc::netbuf = %p, frame len = %d.}",
                  pst_ctx_event->pst_netbuf, pst_ctx_event->us_frame_len);

    return pst_event_mem;
}


frw_event_mem_stru * hmac_rx_process_data_rx_adapt(frw_event_mem_stru * pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;

    frw_event_mem_stru              *pst_event_mem;
    dmac_wlan_drx_event_stru        *pst_wlan_rx_event;

    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_DATA_ADAPT);

    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);

    /*filter the extend buf*/
    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem,OAL_SIZEOF(dmac_wlan_drx_event_stru));
    if (NULL == pst_event_mem)
    {
        return NULL;
    }

    /*填业务事件信息*/
    pst_wlan_rx_event                 = (dmac_wlan_drx_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_wlan_rx_event->pst_netbuf     = pst_hcc_event_payload->pst_netbuf;
    pst_wlan_rx_event->us_netbuf_num  = 1;//目前不支持通过SDIO后组链，默认都是单帧


    return pst_event_mem;
}

frw_event_mem_stru *  hmac_rx_process_mgmt_event_rx_adapt_etc(frw_event_mem_stru * pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;

    frw_event_mem_stru              *pst_event_mem;
    dmac_wlan_crx_event_stru        *pst_crx_event;

    /*取HCC事件信息*/
    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);

    /*filter the extend buf*/
    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem,OAL_SIZEOF(dmac_wlan_crx_event_stru));
    if (NULL == pst_event_mem)
    {
        return NULL;
    }

    /*填业务事件信息*/
    pst_crx_event                     = (dmac_wlan_crx_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_crx_event->pst_netbuf         = pst_hcc_event_payload->pst_netbuf;

    return pst_event_mem;

}

#ifdef _PRE_WLAN_FEATRUE_FLOWCTL


frw_event_mem_stru* hmac_alg_flowctl_backp_rx_adapt(frw_event_mem_stru * pst_hcc_event_mem)
{
    frw_event_stru                  *pst_hcc_event;
    hcc_event_stru                  *pst_hcc_event_payload;
    frw_event_hdr_stru              *pst_hcc_event_hdr;

    oal_uint8                        uc_chip_id;
    oal_uint8                        uc_device_id;
    oal_uint8                        uc_vap_id;

    frw_event_mem_stru              *pst_event_mem;
    frw_event_stru                  *pst_event;

    if (OAL_PTR_NULL == pst_hcc_event_mem)
    {
        return OAL_PTR_NULL;
    }

    /*step1 取HCC事件头*/
    pst_hcc_event           = frw_get_event_stru(pst_hcc_event_mem);
    pst_hcc_event_hdr       = &(pst_hcc_event->st_event_hdr);
    uc_chip_id              = pst_hcc_event_hdr->uc_chip_id;
    uc_device_id            = pst_hcc_event_hdr->uc_device_id;
    uc_vap_id               = pst_hcc_event_hdr->uc_vap_id;

    /*step2 取HCC事件信息*/
    pst_hcc_event_payload   = (hcc_event_stru *)pst_hcc_event->auc_event_data;


    /*step3 申请业务事件*/
    pst_event_mem = FRW_EVENT_ALLOC((oal_uint16)pst_hcc_event_payload->ul_buf_len);
    if (OAL_PTR_NULL == pst_event_mem)
    {
        oal_netbuf_free(pst_hcc_event_payload->pst_netbuf);
        return OAL_PTR_NULL;
    }

    pst_event =  frw_get_event_stru(pst_event_mem);

    /*step4 填业务事件头*/
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                   pst_hcc_event_payload->en_nest_type,
                   pst_hcc_event_payload->uc_nest_sub_type,
                   (oal_uint16)pst_hcc_event_payload->ul_buf_len,
                   FRW_EVENT_PIPELINE_STAGE_1,
                   uc_chip_id,
                   uc_device_id,
                   uc_vap_id);

    /*step5 填HCC事件信息*/
    oal_memcopy(pst_event->auc_event_data, (oal_uint8 *)(OAL_NETBUF_DATA((oal_netbuf_stru *)pst_hcc_event_payload->pst_netbuf)), pst_hcc_event_payload->ul_buf_len);

    oal_netbuf_free(pst_hcc_event_payload->pst_netbuf);

    return pst_event_mem;
}


#endif


frw_event_mem_stru *hmac_cali2hmac_misc_event_rx_adapt_etc(frw_event_mem_stru * pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;
    frw_event_mem_stru              *pst_event_mem;
    hal_cali_hal2hmac_event_stru    *pst_cali_save_event;

    //OAL_IO_PRINT("hmac_cali2hmac_misc_event_rx_adapt_etc start\r\n");

    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_DATA_ADAPT);

    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);

    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem, OAL_SIZEOF(hal_cali_hal2hmac_event_stru));
    if (NULL == pst_event_mem)
    {
        OAL_IO_PRINT("cali_hmac_rx_adapt_fail\r\n");
        return NULL;
    }

    /*填业务事件信息*/
    pst_cali_save_event                 = (hal_cali_hal2hmac_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_cali_save_event->pst_netbuf     = pst_hcc_event_payload->pst_netbuf;
    pst_cali_save_event->us_netbuf_num  = 1;//目前不支持通过SDIO后组链，默认都是单帧

    return pst_event_mem;
}

#ifdef _PRE_WLAN_ONLINE_DPD

frw_event_mem_stru * hmac_dpd_rx_adapt(frw_event_mem_stru * pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;

    frw_event_mem_stru              *pst_event_mem;
    hal_cali_hal2hmac_event_stru    *pst_cali_save_event;

    //OAL_IO_PRINT("hmac_cali2hmac_misc_event_rx_adapt_etc start\r\n");

    OAL_MIPS_RX_STATISTIC(HMAC_PROFILING_FUNC_RX_DATA_ADAPT);

    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);

    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem,OAL_SIZEOF(hal_cali_hal2hmac_event_stru));
    if (NULL == pst_event_mem)
    {
        OAL_IO_PRINT("cali_hmac_rx_adapt_fail\r\n");
        return NULL;
    }

    /*填业务事件信息*/
    pst_cali_save_event                 = (hal_cali_hal2hmac_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_cali_save_event->pst_netbuf     = pst_hcc_event_payload->pst_netbuf;
    pst_cali_save_event->us_netbuf_num  = 1;//目前不支持通过SDIO后组链，默认都是单帧

    return pst_event_mem;
}
#endif

#ifdef _PRE_WLAN_FEATURE_APF

frw_event_mem_stru * hmac_apf_program_report_rx_adapt(frw_event_mem_stru * pst_hcc_event_mem)
{
    hcc_event_stru                  *pst_hcc_event_payload;
    frw_event_mem_stru              *pst_event_mem;
    dmac_apf_report_event_stru      *pst_report_event;

    pst_hcc_event_payload   = (hcc_event_stru *)frw_get_event_payload(pst_hcc_event_mem);

    /*filter the extend buf*/
    hmac_hcc_rx_event_comm_adapt_etc(pst_hcc_event_mem);

    pst_event_mem = hmac_hcc_expand_rx_adpat_event_etc(pst_hcc_event_mem,OAL_SIZEOF(dmac_apf_report_event_stru));
    if (NULL == pst_event_mem)
    {
       return NULL;
    }

    /*填业务事件信息*/
    pst_report_event               = (dmac_apf_report_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_report_event->p_program    = pst_hcc_event_payload->pst_netbuf;

    return pst_event_mem;
}
#endif

oal_uint32 hmac_proc_add_user_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_add_user_stru));
}




oal_uint32 hmac_proc_del_user_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_del_user_stru));
}


/*lint -e413*/
oal_uint32 hmac_proc_config_syn_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    hmac_to_dmac_cfg_msg_stru       *pst_syn_cfg_payload;
    pst_syn_cfg_payload    = (hmac_to_dmac_cfg_msg_stru *)frw_get_event_payload(pst_event_mem);

    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem,
                                              (pst_syn_cfg_payload->us_len + (oal_uint32)OAL_OFFSET_OF(hmac_to_dmac_cfg_msg_stru,auc_msg_body)));
}
/*lint +e413*/



/*lint -e413*/
oal_uint32 hmac_proc_config_syn_alg_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    hmac_to_dmac_cfg_msg_stru       *pst_syn_cfg_payload;
    pst_syn_cfg_payload    = (hmac_to_dmac_cfg_msg_stru *)frw_get_event_payload(pst_event_mem);

    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem,
                                              (pst_syn_cfg_payload->us_len + (oal_uint32)OAL_OFFSET_OF(hmac_to_dmac_cfg_msg_stru,auc_msg_body)));
}
/*lint +e413*/


oal_uint32 hmac_sdt_recv_reg_cmd_tx_adapt(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, (oal_uint32)((frw_get_event_stru(pst_event_mem))->st_event_hdr.us_length));
}

#if defined(_PRE_WLAN_FEATURE_DATA_SAMPLE) || defined(_PRE_WLAN_FEATURE_PSD_ANALYSIS) || defined(_PRE_WLAN_RF_AUTOCALI)

oal_uint32 hmac_sdt_recv_sample_cmd_tx_adapt(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, (oal_uint32)((frw_get_event_stru(pst_event_mem))->st_event_hdr.us_length) - OAL_SIZEOF(frw_event_hdr_stru));
}
#endif



oal_uint32 hmac_proc_tx_host_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    oal_netbuf_stru                 *pst_current_netbuf;
    oal_netbuf_stru                 *pst_current_netbuf_tmp = NULL;
    dmac_tx_event_stru              *pst_dmac_tx_event_payload;

    /*取业务事件信息*/
    pst_dmac_tx_event_payload = (dmac_tx_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_current_netbuf        = pst_dmac_tx_event_payload->pst_netbuf;

    while(OAL_PTR_NULL != pst_current_netbuf)
    {
        /*必须在netbuf抛出之前指向下一个netbuf，防止frw_event_dispatch_event_etc 中重置 netbuf->next */
        pst_current_netbuf_tmp = pst_current_netbuf;
        pst_current_netbuf = OAL_NETBUF_NEXT(pst_current_netbuf);

        /*netbuf 失败由被调函数释放!*/
        OAL_MIPS_TX_STATISTIC(HOST_PROFILING_FUNC_HCC_TX_ADAPT);
        hmac_hcc_tx_data_etc(pst_event_mem, pst_current_netbuf_tmp);

    }
    return OAL_SUCC;
}


oal_uint32 hmac_proc_set_edca_param_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));
}

#ifdef _PRE_WLAN_FEATURE_11AX

oal_uint32 hmac_proc_set_mu_edca_param_tx_adapt(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_sta_asoc_set_edca_reg_stru));
}
#endif


oal_uint32 hmac_scan_proc_scan_req_event_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    mac_scan_req_stru          *pst_h2d_scan_req_params;        /* 下发的扫描参数 */

    if(OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
    {
        return OAL_ERR_CODE_PTR_NULL;
    }

    OAM_INFO_LOG0(0, OAM_SF_ANY, "{hmac_scan_proc_scan_req_event_tx_adapt_etc:: scan req, enter into tx adapt.}");
    pst_h2d_scan_req_params = (mac_scan_req_stru *)frw_get_event_payload(pst_event_mem);

    return hmac_hcc_tx_event_buf_to_netbuf_etc(pst_event_mem, (oal_uint8*)pst_h2d_scan_req_params, OAL_SIZEOF(mac_scan_req_stru));
}


oal_uint32 hmac_send_event_netbuf_tx_adapt(frw_event_mem_stru *pst_event_mem)
{
    dmac_tx_event_stru          *pst_dmac_tx_event;

    if(OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_CALIBRATE, "{hmac_send_event_netbuf_tx_adapt::pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_tx_event = (dmac_tx_event_stru *)frw_get_event_payload(pst_event_mem);
    return hmac_hcc_tx_event_buf_to_netbuf_etc(pst_event_mem, (oal_uint8*)OAL_NETBUF_DATA(pst_dmac_tx_event->pst_netbuf), pst_dmac_tx_event->us_frame_len);
}
#ifdef _PRE_WLAN_FEATURE_IP_FILTER

oal_uint32 hmac_config_update_ip_filter_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    dmac_tx_event_stru          *pst_dmac_tx_event;

    if(OAL_PTR_NULL == pst_event_mem)
    {
        OAM_ERROR_LOG0(0, OAM_SF_ANY, "{hmac_config_update_ip_filter_tx_adapt_etc:: pst_event_mem null.}");
        return OAL_ERR_CODE_PTR_NULL;
    }

    pst_dmac_tx_event = (dmac_tx_event_stru *)frw_get_event_payload(pst_event_mem);
    return hmac_hcc_tx_event_buf_to_netbuf_etc(pst_event_mem, (oal_uint8*)OAL_NETBUF_DATA(pst_dmac_tx_event->pst_netbuf), pst_dmac_tx_event->us_frame_len);

}

#endif //_PRE_WLAN_FEATURE_IP_FILTER


oal_uint32 hmac_scan_proc_sched_scan_req_event_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    mac_pno_scan_stru   *pst_h2d_pno_scan_req_params;     /* 下发PNO调度扫描请求 */

	if(OAL_UNLIKELY(OAL_PTR_NULL == pst_event_mem))
	{
	    return OAL_ERR_CODE_PTR_NULL;
	}

    pst_h2d_pno_scan_req_params = (mac_pno_scan_stru *)(*(oal_uint *)frw_get_event_payload(pst_event_mem));

    return hmac_hcc_tx_event_buf_to_netbuf_etc(pst_event_mem, (oal_uint8 *)pst_h2d_pno_scan_req_params, OAL_SIZEOF(mac_pno_scan_stru));
}


oal_uint32 hmac_mgmt_update_user_qos_table_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_asoc_set_reg_stru));
}


oal_uint32 hmac_proc_join_set_reg_event_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{hmac_proc_join_set_reg_event_tx_adapt_etc::tx adapt.}");
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_join_req_set_reg_stru));
}



oal_uint32 hmac_proc_join_set_dtim_reg_event_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{hmac_proc_join_set_dtim_reg_event_tx_adapt_etc::tx adapt.}");
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_set_dtim_tsf_reg_stru));
}


oal_uint32 hmac_hcc_tx_convert_event_to_netbuf_uint32_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(oal_uint32));
}


oal_uint32 hmac_hcc_tx_convert_event_to_netbuf_uint16_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(oal_uint16));
}


oal_uint32 hmac_hcc_tx_convert_event_to_netbuf_uint8_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(oal_uint8));
}


oal_uint32 hmac_user_add_notify_alg_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    OAM_INFO_LOG0(0, OAM_SF_ANY, "{hmac_user_add_notify_alg_tx_adapt_etc::tx adapt.}");
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_add_user_stru));
}


oal_uint32 hmac_proc_rx_process_sync_event_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_ctx_action_event_stru));
}

oal_uint32 hmac_chan_select_channel_mac_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_set_chan_stru));
}


oal_uint32 hmac_chan_initiate_switch_to_new_channel_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(dmac_set_ch_switch_info_stru));
}

#ifdef _PRE_WLAN_FEATURE_EDCA_OPT_AP

oal_uint32 hmac_edca_opt_stat_event_tx_adapt_etc(frw_event_mem_stru *pst_event_mem)
{
    return hmac_hcc_tx_event_payload_to_netbuf_etc(pst_event_mem, OAL_SIZEOF(oal_uint8) * 16);
}

#endif

oal_int32 hmac_rx_extend_hdr_vaild_check_etc(struct frw_hcc_extend_hdr* pst_extend_hdr)
{
    if(OAL_UNLIKELY(pst_extend_hdr->en_nest_type >= FRW_EVENT_TYPE_BUTT))
    {
        return OAL_FALSE;
    }
#if 0
    if(OAL_UNLIKELY(pst_extend_hdr->vap_id > WLAN_VAP_SUPPORT_MAX_NUM_LIMIT))
    {
        return OAL_FALSE;
    }
#endif
    return OAL_TRUE;
}

/*
oal_int32 dmac_rx_wifi_pre_action_function(oal_uint8 stype, hcc_netbuf_stru* pst_hcc_netbuf,
                                                  oal_uint8  **pre_do_context)
{
    return OAL_SUCC;
}
*/

oal_int32 hmac_rx_wifi_post_action_function_etc(struct hcc_handler* hcc, oal_uint8 stype,
                                             hcc_netbuf_stru* pst_hcc_netbuf, oal_uint8 *pst_context)
{
    oal_int32 ret = OAL_SUCC;
    hmac_vap_stru             *pst_hmac_vap;
    struct frw_hcc_extend_hdr* pst_extend_hdr;

    frw_event_mem_stru   *pst_event_mem;      /*event mem */
    frw_event_stru       *pst_event;
    hcc_event_stru       *pst_event_payload;
    mac_rx_ctl_stru      *pst_rx_ctl;
    oal_uint8            *puc_hcc_extend_hdr;

    OAL_REFERENCE(hcc);

    pst_extend_hdr = (struct frw_hcc_extend_hdr*)OAL_NETBUF_DATA(pst_hcc_netbuf->pst_netbuf);
    if(OAL_TRUE != hmac_rx_extend_hdr_vaild_check_etc(pst_extend_hdr))
    {
        oal_print_hex_dump(OAL_NETBUF_DATA(pst_hcc_netbuf->pst_netbuf), (oal_int32)OAL_NETBUF_LEN(pst_hcc_netbuf->pst_netbuf),
                           32, "invaild frw extend hdr: ");
        return -OAL_EINVAL;
    }

    pst_hmac_vap = (hmac_vap_stru *)mac_res_get_hmac_vap(pst_extend_hdr->vap_id);

    frw_event_task_lock();
    if(OAL_UNLIKELY(NULL == pst_hmac_vap))
    {
        OAM_WARNING_LOG1(0, OAM_SF_ANY, "hmac rx adapt ignored,pst vap is null, vap id:%u", pst_extend_hdr->vap_id);
        frw_event_task_unlock();
        oal_netbuf_free(pst_hcc_netbuf->pst_netbuf);
        return -OAL_EINVAL;
    }

    if(OAL_UNLIKELY(MAC_VAP_VAILD != pst_hmac_vap->st_vap_base_info.uc_init_flag))
    {
        if(0 == pst_extend_hdr->vap_id)
        {
            /*配置VAP不过滤*/
        }
        else
        {
            OAM_WARNING_LOG2(pst_extend_hdr->vap_id, OAM_SF_ANY, "hmac rx adapt ignored,main:%u,sub:%u", pst_extend_hdr->en_nest_type,pst_extend_hdr->uc_nest_sub_type);
            frw_event_task_unlock();
            oal_netbuf_free(pst_hcc_netbuf->pst_netbuf);
            return -OAL_ENOMEM;
        }
    }
    frw_event_task_unlock();

    pst_event_mem = FRW_EVENT_ALLOC(OAL_SIZEOF(hcc_event_stru));
    if (NULL == pst_event_mem)
    {
        OAL_IO_PRINT("[WARN]event mem alloc failed\n");
        return -OAL_ENOMEM;
    }

    /*trim the frw hcc extend header*/
    oal_netbuf_pull(pst_hcc_netbuf->pst_netbuf, OAL_SIZEOF(struct frw_hcc_extend_hdr));

    /*event hdr point*/
    pst_event = frw_get_event_stru(pst_event_mem);

    /*fill event hdr*/
    FRW_EVENT_HDR_INIT(&(pst_event->st_event_hdr),
                       pst_extend_hdr->en_nest_type,
                       pst_extend_hdr->uc_nest_sub_type,
                       OAL_SIZEOF(hcc_event_stru),
                       FRW_EVENT_PIPELINE_STAGE_1,
                       pst_extend_hdr->chip_id,
                       pst_extend_hdr->device_id,
                       pst_extend_hdr->vap_id);


    pst_event_payload = (hcc_event_stru *)frw_get_event_payload(pst_event_mem);
    pst_event_payload->pst_netbuf = pst_hcc_netbuf->pst_netbuf;
    pst_event_payload->ul_buf_len = OAL_NETBUF_LEN(pst_hcc_netbuf->pst_netbuf);
    //pst_event_payload->ul_buf_len = pst_hcc_netbuf->len;

    puc_hcc_extend_hdr  = OAL_NETBUF_DATA((oal_netbuf_stru *)pst_event_payload->pst_netbuf);
    pst_rx_ctl    = (mac_rx_ctl_stru *)puc_hcc_extend_hdr;

    if(!(pst_rx_ctl->bit_is_beacon))
    {
        g_pm_wifi_rxtx_count++; ////收包统计 for pm
    }

#ifdef _PRE_WLAN_WAKEUP_SRC_PARSE
   if((OAL_TRUE==wlan_pm_wkup_src_debug_get())&&((FRW_EVENT_TYPE_WLAN_DRX!=pst_extend_hdr->en_nest_type)))
   {
        OAL_IO_PRINT("wifi_wake_src:event[%d],subtype[%d]!\n",pst_extend_hdr->en_nest_type, pst_extend_hdr->uc_nest_sub_type);

        /* 管理帧事件，开关在管理帧处理流程中打印具体的管理帧类型后关闭 */
        if(!((FRW_EVENT_TYPE_WLAN_CRX==pst_extend_hdr->en_nest_type)&&(DMAC_WLAN_CRX_EVENT_SUB_TYPE_RX==pst_extend_hdr->uc_nest_sub_type)))
        {
            wlan_pm_wkup_src_debug_set(OAL_FALSE);
        }

   }
#endif

    frw_event_task_lock();
    ret = (oal_int32)frw_event_dispatch_event_etc(pst_event_mem);
    frw_event_task_unlock();
    if(OAL_WARN_ON(OAL_SUCC != ret))
    {
        /*如果事件入队失败，内存失败由该函数释放，直接调用的由rx adapt函数释放!*/
        OAL_IO_PRINT("[WARN]hcc rx post event failed!!!ret=%u,main:%d,sub:%d\n",
                    ret,
                    pst_extend_hdr->en_nest_type,
                    pst_extend_hdr->uc_nest_sub_type);
    }
    FRW_EVENT_FREE(pst_event_mem);

    return ret;
}

oal_int32 hmac_hcc_adapt_init_etc(oal_void)
{
    hmac_tx_net_queue_map_init_etc();
    hmac_tx_sched_info_init_etc();
    hcc_rx_register_etc(hcc_get_110x_handler(), HCC_ACTION_TYPE_WIFI, hmac_rx_wifi_post_action_function_etc, NULL);
#ifdef _PRE_CONFIG_HISI_PANIC_DUMP_SUPPORT
    hwifi_panic_log_register_etc(&hmac_panic_hcc_adapt,NULL);
#endif
    return OAL_SUCC;
}

#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


