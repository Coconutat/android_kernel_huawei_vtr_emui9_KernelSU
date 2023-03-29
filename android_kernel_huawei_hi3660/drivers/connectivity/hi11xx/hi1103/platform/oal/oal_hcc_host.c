


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#define HI11XX_LOG_MODULE_NAME "[HCC]"
#define HI11XX_LOG_MODULE_NAME_VAR hcc_loglevel
#define HISI_LOG_TAG "[HCC]"
#include "oal_util.h"
#include "oal_sdio_host_if.h"
#include "oal_pcie_host.h"
#include "oal_pcie_linux.h"
#include "oal_hcc_host_if.h"
#include "oal_profiling.h"
#include "oam_ext_if.h"

#ifdef CONFIG_MMC
#include "plat_pm_wlan.h"
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
#include <linux/sched.h>
#include <linux/kernel.h>
#include <linux/jiffies.h>
#include <linux/wait.h>
#include <linux/workqueue.h>
#include <linux/kthread.h>
#endif

#undef  THIS_FILE_ID
#define THIS_FILE_ID OAM_FILE_ID_OAL_HCC_HOST_C


#define HCC_TX_FLOW_HI_LEVEL    (512)
#define HCC_TX_FLOW_LO_LEVEL    (128)

#define HCC_TRANS_THREAD_POLICY    SCHED_FIFO
#define HCC_TRANS_THERAD_PRIORITY       (10)
#define HCC_TRANS_THERAD_NICE           (-10)
OAL_STATIC struct hcc_handler* hcc_tc = NULL;

OAL_STATIC oal_uint32 g_hcc_tx_max_buf_len = 4096;

oal_uint32 g_ul_tcp_ack_wait_sche_cnt_etc = 1;


#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
struct custom_process_func_handler g_pst_custom_process_func_etc;
EXPORT_SYMBOL_GPL(g_pst_custom_process_func_etc);//lint !e132 !e745 !e578
#endif

/*use lint -e16 to mask the error19! */
oal_uint32 g_pm_wifi_rxtx_count = 0; //pm收发包统计变量
/*lint -e19 */
oal_module_symbol(g_pm_wifi_rxtx_count);
/*lint +e19 */

//oal_uint32 hcc_assemble_count_etc = HISDIO_HOST2DEV_SCATT_MAX;
oal_uint32 hcc_assemble_count_etc = 8;
/*lint -e19 */
oal_module_symbol(hcc_assemble_count_etc);
/*lint +e19 */
/*1 means hcc rx data process in hcc thread,
  0 means process in sdio thread*/
oal_uint32 hcc_rx_thread_enable_etc = 1;
oal_uint32 hcc_credit_bottom_value_etc=2;
oal_uint32 hcc_flowctrl_detect_panic_etc=0;
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
module_param(g_hcc_tx_max_buf_len, uint, S_IRUGO|S_IWUSR);
module_param(hcc_assemble_count_etc, uint, S_IRUGO|S_IWUSR);
module_param(hcc_rx_thread_enable_etc, uint, S_IRUGO|S_IWUSR);
module_param(hcc_credit_bottom_value_etc, uint, S_IRUGO|S_IWUSR);
module_param(hcc_flowctrl_detect_panic_etc, uint, S_IRUGO|S_IWUSR);
#endif
oal_int32 hcc_send_rx_queue_etc(struct hcc_handler *hcc, hcc_queue_type type);
oal_uint32 hcc_queues_flow_ctrl_len_etc(struct hcc_handler* hcc, hcc_chan_type dir);
OAL_STATIC  oal_void hcc_dev_flowctr_timer_del(struct hcc_handler *hcc);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
oal_bool_enum_uint8 hcc_flowctl_get_device_mode_etc(struct hcc_handler * hcc);
oal_void hcc_tx_network_start_subq_etc(struct hcc_handler *hcc, oal_uint16 us_queue_idx);
oal_void hcc_tx_network_stop_subq_etc(struct hcc_handler *hcc, oal_uint16 us_queue_idx);
#endif

oal_int32 hcc_set_hi11xx_loglevel(oal_int32 loglevel)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_int32 ret = HI11XX_LOG_MODULE_NAME_VAR;
    HI11XX_LOG_MODULE_NAME_VAR = loglevel;
    return ret;
#else
    return 0;
#endif
}

oal_int32 hcc_set_all_loglevel(oal_int32 loglevel)
{
    oal_int32 ret = hcc_set_hi11xx_loglevel(loglevel);
#ifdef _PRE_PLAT_FEATURE_HI110X_PCIE
    oal_pcie_set_loglevel(loglevel);
    oal_pcie_set_hi11xx_loglevel(loglevel);
#endif
    return ret;
}

oal_uint32 hcc_get_max_buf_len(oal_void)
{
    return g_hcc_tx_max_buf_len;
}

/*This is not good*/
struct hcc_handler* hcc_get_110x_handler(oal_void)
{
    hcc_bus_dev *pst_bus_dev = hcc_get_bus_dev(HCC_CHIP_110X_DEV);
    if(OAL_UNLIKELY(NULL == pst_bus_dev))
    {
        return NULL;
    }

    return pst_bus_dev->hcc;
}
/*lint -e19 */
oal_module_symbol(hcc_get_110x_handler);
/*lint +e19 */

OAL_STATIC oal_void hcc_tx_netbuf_destory(struct hcc_handler* hcc)
{
    if(OAL_WARN_ON(NULL == hcc))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: hcc is null",__FUNCTION__);
         return;
    };

    oal_wake_unlock(&hcc->tx_wake_lock);
}


#ifdef _PRE_PLAT_FEATURE_CUSTOMIZE
struct custom_process_func_handler* oal_get_custom_process_func_etc(oal_void)
{
    return &g_pst_custom_process_func_etc;
}
/*lint -e19 */
oal_module_symbol(oal_get_custom_process_func_etc);
/*lint +e19 */
#endif

OAL_STATIC  oal_netbuf_stru* hcc_tx_assem_descr_get(struct hcc_handler *hcc)
{
    return  oal_netbuf_delist(&hcc->tx_descr_info.tx_assem_descr_hdr);
}

OAL_STATIC  oal_void hcc_tx_assem_descr_put(struct hcc_handler *hcc, oal_netbuf_stru* netbuf)
{
    oal_netbuf_list_tail(&hcc->tx_descr_info.tx_assem_descr_hdr, netbuf);
}

oal_void hcc_clear_next_assem_descr_etc(struct hcc_handler *hcc,
                                                                 oal_netbuf_stru* descr_netbuf)
{
    OAL_REFERENCE(hcc);
    /*Just need clear the first bytes*/
    oal_memset(OAL_NETBUF_DATA(descr_netbuf),0,1);
}

#ifdef CONFIG_MMC
oal_uint32 hcc_get_large_pkt_free_cnt_etc(struct hcc_handler* hcc)
{
    return oal_sdio_get_large_pkt_free_cnt_etc(oal_get_sdio_default_handler());
}
#endif

/*align_size must be power of 2*/
OAL_STATIC  oal_netbuf_stru*  hcc_netbuf_len_align(oal_netbuf_stru* netbuf,
                                                          oal_uint32 align_size)
{
    oal_int32 ret = OAL_SUCC;
    oal_uint32 len_algin, tail_room_len;
    oal_uint32 len = OAL_NETBUF_LEN(netbuf);
    if(OAL_IS_ALIGNED(len,align_size))
    {
        return netbuf;
    }
    /*align the netbuf*/
    len_algin = OAL_ROUND_UP(len,align_size);

    if(OAL_UNLIKELY(len_algin < len))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s error: len_algin:%d < len:%d",__FUNCTION__,len_algin,len);
        return NULL;
    };

    tail_room_len = len_algin - len;

    if(OAL_UNLIKELY(tail_room_len > oal_netbuf_tailroom(netbuf)))
    {
        /*tailroom not enough*/
        ret = oal_netbuf_expand_head(netbuf, 0, (oal_int32)tail_room_len, GFP_KERNEL);
        if(OAL_WARN_ON(OAL_SUCC != ret))
        {
            oal_print_hi11xx_log(HI11XX_LOG_WARN, "alloc head room failed,expand tail len=%d", tail_room_len);
            DECLARE_DFT_TRACE_KEY_INFO("netbuf align expand head fail", OAL_DFT_TRACE_FAIL);
            return NULL;
        }
    }

    oal_netbuf_put(netbuf, tail_room_len);

    if(OAL_UNLIKELY(!OAL_IS_ALIGNED(OAL_NETBUF_LEN(netbuf),align_size)))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s error: netbuf len :%d not align %d",__FUNCTION__,OAL_NETBUF_LEN(netbuf),align_size);
        return NULL;
    };

    return netbuf;

}

OAL_STATIC  oal_void hcc_build_next_assem_descr(struct hcc_handler *hcc,
                                                                 hcc_queue_type  type,
                                                                 oal_netbuf_head_stru *head,
                                                                 oal_netbuf_head_stru* next_assembled_head,
                                                                 oal_netbuf_stru* descr_netbuf,
                                                                 oal_uint32 remain_len)
{
    oal_int32 i = 0;
    oal_int32 len;
    oal_uint8 * buf;
    oal_netbuf_stru* netbuf, *netbuf_t;
    oal_uint32 assemble_max_count, queue_len, current_trans_len;


    buf = (oal_uint8*)OAL_NETBUF_DATA(descr_netbuf);
    len = (oal_int32)OAL_NETBUF_LEN(descr_netbuf);

    if(OAL_UNLIKELY(!oal_netbuf_list_empty(next_assembled_head)))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s error: next_assembled_head is empty",__FUNCTION__);
        return ;
    };

    assemble_max_count = OAL_MAX(1,hcc_assemble_count_etc);
    queue_len = oal_netbuf_list_len(head);
    current_trans_len = OAL_MIN(queue_len, assemble_max_count);
    current_trans_len = OAL_MIN(current_trans_len, remain_len);

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "build next descr, queue:[remain_len:%u][len:%u][trans_len:%u][max_assem_len:%u]",
                             remain_len,queue_len,current_trans_len,assemble_max_count);

    buf[0] = 0;

    if(0 == current_trans_len)
    {
        return;
    }

    for(;;)
    {
        /*move the netbuf from head queue to prepare-send queue, head->tail*/
        netbuf = oal_netbuf_delist(head);
        if(NULL == netbuf)

        {
            oal_print_hi11xx_log(HI11XX_LOG_WARN, "why? this should never happaned! assem list len:%d",
                        oal_netbuf_list_len(next_assembled_head));
            DECLARE_DFT_TRACE_KEY_INFO("buid assem error", OAL_DFT_TRACE_FAIL);
            break;
        }

        /*align the buff len to 32B*/
        netbuf_t  = hcc_netbuf_len_align(netbuf, HISDIO_H2D_SCATT_BUFFLEN_ALIGN);
        if(NULL == netbuf_t)
        {
            /*return to the list*/
            oal_netbuf_addlist(head, netbuf);
            break;
        }

        current_trans_len--;

        oal_netbuf_list_tail(next_assembled_head, netbuf_t);
        if(OAL_UNLIKELY(i >= len))
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc tx scatt num :%d over buff len:%d,assem count:%u",i,len, hcc_assemble_count_etc);
            break;
        }

        buf[i++] = (oal_uint8)(OAL_NETBUF_LEN(netbuf) >> HISDIO_H2D_SCATT_BUFFLEN_ALIGN_BITS);
        if(0 == current_trans_len)
        {
            /*send empty*/
            if(i != len)
            {
                buf[i] = 0;
            }
            break;
        }

    }

    //oal_print_hex_dump((oal_uint8*)buf, HISDIO_HOST2DEV_SCATT_SIZE, 2, "h2d assem info");

    if(OAL_LIKELY(!oal_netbuf_list_empty(next_assembled_head)))
        hcc->hcc_transer_info.tx_assem_info.assembled_queue_type = type;

}


oal_int32 hcc_tx_param_check_etc(struct hcc_handler* hcc,
                                   struct hcc_transfer_param* param)
{
    if(OAL_UNLIKELY(param->extend_len > hcc->hdr_rever_max_len))
    {
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        WARN(1,"invaild reserved len %u ,never should over %zu\n",
            param->extend_len, HCC_HDR_RESERVED_MAX_LEN);
#endif
        /*can't free buf here*/
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(param->main_type >= HCC_ACTION_TYPE_BUTT))
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "wrong main type:%d", param->main_type);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(param->fc_flag & (~HCC_FC_ALL)))
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "wrong fc_flag:%d", param->fc_flag);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(param->queue_id >= HCC_QUEUE_COUNT))
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "wrong queue_id:%d", param->queue_id);
        return -OAL_EINVAL;
    }
    return OAL_SUCC;
}

oal_void hcc_tx_network_stopall_queues_etc(struct hcc_handler *hcc)
{
    if(OAL_WARN_ON(NULL == hcc))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: hcc is null",__FUNCTION__);
         return;
    };
    if(OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.net_stopall))
    {
        //oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
        hcc->hcc_transer_info.tx_flow_ctrl.net_stopall();
        //OAL_IO_PRINT("stop hcc_tx_network_stopall_queues_etc\n");
        //oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
    }
}

oal_void hcc_tx_network_startall_queues_etc(struct hcc_handler *hcc)
{
    if(OAL_WARN_ON(NULL == hcc))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: hcc is null",__FUNCTION__);
         return;
    };

    if(OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.net_startall))
    {
        //oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
        hcc->hcc_transer_info.tx_flow_ctrl.net_startall();
        //OAL_IO_PRINT("start hcc_tx_network_startall_queues_etc\n");
        //oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
    }
}


oal_void hcc_tx_flow_ctrl_cb_register_etc(flowctrl_cb stopall, flowctrl_cb startall)
{
    if(NULL == hcc_tc)
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[ERROR]hcc_tx_flow_ctrl_cb_register_etc failed! hcc_tc is NULL");
        return;
    }
    hcc_tc->hcc_transer_info.tx_flow_ctrl.net_stopall = stopall;
    hcc_tc->hcc_transer_info.tx_flow_ctrl.net_startall = startall;
}

/*lint -e19 */
oal_module_symbol(hcc_tx_flow_ctrl_cb_register_etc);
/*lint +e19 */


oal_void hcc_tx_wlan_queue_map_set_etc(struct hcc_handler* hcc,hcc_queue_type hcc_queue_id,wlan_net_queue_type wlan_queue_id)
{
    hcc_trans_queue *pst_hcc_queue;
    if(OAL_WARN_ON(NULL == hcc))
    {
        return;
    }

    if(OAL_WARN_ON(hcc_queue_id >= HCC_QUEUE_COUNT || wlan_queue_id >= WLAN_NET_QUEUE_BUTT))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "invaild param,hcc id:%d,wlan id:%d",(oal_int32)hcc_queue_id, (oal_int32)wlan_queue_id);
        return;
    }
    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[hcc_queue_id];
    pst_hcc_queue->wlan_queue_id = wlan_queue_id;
}

/*lint -e19 */
oal_module_symbol(hcc_tx_wlan_queue_map_set_etc);
/*lint +e19 */

oal_void hcc_msg_slave_thruput_bypass_etc(oal_void)
{
    struct hcc_handler* hcc = hcc_get_110x_handler();
    oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_msg_slave_thruput_bypass_etc.");

    hcc_bus_send_message(HCC_TO_BUS(hcc), H2D_MSG_HCC_SLAVE_THRUPUT_BYPASS);
}
#ifndef _PRE_PC_LINT
oal_module_symbol(hcc_msg_slave_thruput_bypass_etc);
#endif

OAL_STATIC oal_uint32 hcc_check_header_vaild(struct hcc_header * hdr)
{
    if(OAL_UNLIKELY(hdr->main_type >= HCC_ACTION_TYPE_BUTT)
       || (HCC_HDR_LEN + hdr->pad_hdr + hdr->pad_payload > HCC_HDR_TOTAL_LEN))
    {
        DECLARE_DFT_TRACE_KEY_INFO("hcc_check_header_vaild_fail",OAL_DFT_TRACE_FAIL);
        return OAL_FALSE;
    }
    return OAL_TRUE;
}


oal_int32 hcc_tx_etc(struct hcc_handler* hcc, oal_netbuf_stru* netbuf,
                   struct hcc_transfer_param* param)
{
    oal_uint32 queue_id = DATA_LO_QUEUE;
    oal_int32 ret = OAL_SUCC;
    oal_uint  pad_payload, headroom,
              payload_len, payload_addr,
              pad_hdr;
    oal_uint8* old_data_addr;
    oal_int32 headroom_add = 0;
    struct hcc_header *hdr;
    hcc_trans_queue *pst_hcc_queue;
    struct hcc_tx_cb_stru* pst_cb_stru;

    if(OAL_WARN_ON(NULL == hcc))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "HCC IS null !%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(NULL == netbuf))
    {
        return -OAL_EINVAL;
    }

    if(OAL_WARN_ON(NULL == param))
    {
        return -OAL_EINVAL;
    }

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_UNLIKELY(HCC_ON != oal_atomic_read(&hcc->state)))
    {
        if(HCC_OFF == oal_atomic_read(&hcc->state))
        {
            return -OAL_EBUSY;
        }
        else if(HCC_EXCEPTION == oal_atomic_read(&hcc->state))
        {
            return -OAL_EIO;
        }
        else
        {
            oal_print_hi11xx_log(HI11XX_LOG_INFO, "invaild hcc state:%d", oal_atomic_read(&hcc->state));
            return -OAL_EINVAL;
        }
    }
#endif

    if(OAL_UNLIKELY(OAL_SUCC != hcc_tx_param_check_etc(hcc, param)))
    {
        return -OAL_EINVAL;
    }

    /*lint -e522*/
    OAL_WARN_ON(0 == OAL_NETBUF_LEN(netbuf));
    /*lint +e522*/

    queue_id = param->queue_id;
    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[queue_id];

    if(OAL_WARN_ON((param->fc_flag & HCC_FC_WAIT) && oal_in_interrupt()))
    {
        /*when in interrupt,can't sched!*/
        param->fc_flag &= ~HCC_FC_WAIT;
    }

    if(param->fc_flag)
    {
        /*flow control process*/
        /*Block if fc*/
        if(HCC_FC_WAIT & param->fc_flag)
        {
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
            /*lint -e730*/
            ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hcc->hcc_transer_info.tx_flow_ctrl.wait_queue,
                                            (oal_netbuf_list_len(&pst_hcc_queue->data_queue)
                                            <  pst_hcc_queue->flow_ctrl.low_waterline),
                                            60*OAL_TIME_HZ);

#else
            /*lint -e730*/
            ret = OAL_WAIT_EVENT_INTERRUPTIBLE_TIMEOUT(hcc->hcc_transer_info.tx_flow_ctrl.wait_queue,
                                            hcc_queues_flow_ctrl_len_etc(hcc, HCC_TX) < HCC_TX_FLOW_LO_LEVEL,
                                            60*OAL_TIME_HZ);
#endif
            if(0 == ret)
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "[WARN]hcc flow control wait event timeout! too much time locked");
                DECLARE_DFT_TRACE_KEY_INFO("hcc flow control wait timeout", OAL_DFT_TRACE_FAIL);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
                hcc_print_current_trans_info(0);
#endif
            } else if(-ERESTARTSYS == ret)
            {
                oal_print_hi11xx_log(HI11XX_LOG_WARN, "wifi task was interrupted by a signal");
                return -OAL_EFAIL;
            }
        }

        /*control net layer if fc*/
        if(HCC_FC_NET & param->fc_flag)
        {
            /*net layer??*/
            oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
            if((oal_netbuf_list_len(&pst_hcc_queue->data_queue) > pst_hcc_queue->flow_ctrl.high_waterline)
                && (OAL_FALSE == pst_hcc_queue->flow_ctrl.is_stopped))
            {
                //OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
                //OAM_INFO_LOG3(0, OAM_SF_TX, "{hcc_tx_etc: stop_netdev_queue, queue_len[%d] = %d}, thread = %d", queue_id,
                //                    oal_netbuf_list_len(&pst_hcc_queue->data_queue), pst_hcc_queue->flow_ctrl.high_waterline);
                hcc_tx_network_stop_subq_etc(hcc, (oal_uint16)pst_hcc_queue->wlan_queue_id);
                pst_hcc_queue->flow_ctrl.is_stopped = OAL_TRUE;
            }

#else
            if(hcc_queues_flow_ctrl_len_etc(hcc, HCC_TX) > HCC_TX_FLOW_HI_LEVEL)
            {
                hcc_tx_network_stopall_queues_etc(hcc);
            }
#endif
            oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
        }

        /*control net layer if fc*/
        if(HCC_FC_DROP & param->fc_flag)
        {
            /*10 netbufs to buff*/
            if(oal_netbuf_list_len(&pst_hcc_queue->data_queue) > pst_hcc_queue->flow_ctrl.high_waterline + 100)
            {
                pst_hcc_queue->loss_pkts++;
                /*drop the netbuf*/
                oal_netbuf_free(netbuf);
                return OAL_SUCC;
            }
        }
    }

    if(OAL_GET_THRUPUT_BYPASS_ENABLE(OAL_TX_SDIO_HOST_BYPASS))
    {
        oal_netbuf_free(netbuf);
        return OAL_SUCC;
    }

    /*64B + 4B = head, tail =512*/
    /* only payload data*/
    payload_len = OAL_NETBUF_LEN(netbuf) - param->extend_len;

    old_data_addr = OAL_NETBUF_DATA(netbuf);
    payload_addr = (oal_uint)OAL_NETBUF_DATA(netbuf) + param->extend_len;

    /*if pad not 0, we must copy the extend data*/
    pad_payload = payload_addr - OAL_ROUND_DOWN(payload_addr, 4);
    /* should be 1 byte */
    pad_hdr = HCC_HDR_RESERVED_MAX_LEN - param->extend_len;

#if 0
    /*Tx support 0 data len*/
    if(OAL_WARN_ON(0 == payload_len))
    {
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "main:%d, sub:%d", param->main_type, param->sub_type);
        return -OAL_EINVAL;
    }
#endif

    headroom = HCC_HDR_LEN + pad_hdr + pad_payload;

    if(headroom > oal_netbuf_headroom(netbuf))
    {
        headroom_add = (oal_int32) (headroom - oal_netbuf_headroom(netbuf));
    }

    if(headroom_add)
    {
        /*relloc the netbuf*/
        ret = oal_netbuf_expand_head(netbuf, headroom_add, 0, GFP_ATOMIC);
        if(OAL_UNLIKELY(OAL_SUCC != ret))
        {
            oal_print_hi11xx_log(HI11XX_LOG_WARN, "alloc head room failed,netbuf len is %d,expand len:%d\n",
                            OAL_NETBUF_LEN(netbuf), headroom_add);
            return -OAL_EFAIL;
        }
        old_data_addr = OAL_NETBUF_DATA(netbuf);
    }

    hdr = (struct hcc_header *)oal_netbuf_push(netbuf, headroom);

    if(OAL_UNLIKELY(!OAL_IS_ALIGNED((oal_uint)hdr, 4)))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s error: hdr:%ld not aligned len:%d",__FUNCTION__,(oal_uint)hdr,4);
        return  -OAL_EFAIL;
    };

    if(pad_payload > 0)
    {
        /*algin the extend data*/
        oal_memmove(old_data_addr - pad_payload, old_data_addr, param->extend_len);
    }

    /*fill the hcc header*/
    hdr->pad_payload = (oal_uint16)pad_payload;
    hdr->pay_len = (oal_uint16)payload_len;
    hdr->seq = ((oal_uint32)(oal_atomic_inc_return(&hcc->tx_seq))) & 0xFF;;
    hdr->main_type = (oal_uint8) param->main_type;
    hdr->sub_type = (oal_uint8) param->sub_type;
    hdr->pad_hdr = (oal_uint8)pad_hdr;
    hdr->more = 0;
    hdr->option = 0;

    if(OAL_UNLIKELY(hdr->pad_hdr + HCC_HDR_LEN  > HCC_HDR_TOTAL_LEN))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s error: hdr->pad_hdr + HCC_HDR_LEN:%ld > len:%d",__FUNCTION__,hdr->pad_hdr + HCC_HDR_LEN ,HCC_HDR_TOTAL_LEN);
        return  -OAL_EFAIL;
    };

    if(OAL_WARN_ON(hdr->pay_len > g_hcc_tx_max_buf_len))
    {
        /*pay_len超过DEVICE 最大内存长度*/
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[ERROR]main:%d, sub:%d,pay len:%d,netbuf len:%d, extend len:%d,pad_payload:%d,max len:%u",
                        hdr->main_type,
                        hdr->sub_type,
                        hdr->pay_len,
                        hdr->pay_len + param->extend_len,
                        param->extend_len,
                        hdr->pad_payload,
                        g_hcc_tx_max_buf_len);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_tx_check_param_fail",OAL_DFT_TRACE_FAIL);
        return -OAL_EINVAL;
    }
    else
    {
        /*当长度 在1544 + [1~3] 之内， 牺牲性能,将payload 内存前移 1~3B，节省DEVICE内存*/
        if(hdr->pad_payload + hdr->pay_len > g_hcc_tx_max_buf_len)
        {
            oal_uint8* pst_dst =  (oal_uint8*)hdr + HCC_HDR_TOTAL_LEN;
            oal_uint8* pst_src = pst_dst + hdr->pad_payload;
            oal_memmove((oal_void*)pst_dst, (const oal_void *)pst_src, hdr->pay_len);
            oal_netbuf_trim(netbuf, hdr->pad_payload);
            hdr->pad_payload = 0;/*after memmove,the pad is 0*/
            DECLARE_DFT_TRACE_KEY_INFO("hcc_tx_check_memmove",OAL_DFT_TRACE_SUCC);
        }
    }

    if(OAL_WARN_ON(OAL_ROUND_UP(OAL_NETBUF_LEN(netbuf),HISDIO_H2D_SCATT_BUFFLEN_ALIGN)
                    > g_hcc_tx_max_buf_len + HCC_HDR_TOTAL_LEN))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "[E]Fatal error,netbuf's len:%d over the payload:%d",
                        OAL_ROUND_UP(OAL_NETBUF_LEN(netbuf),HISDIO_H2D_SCATT_BUFFLEN_ALIGN),
                        g_hcc_tx_max_buf_len + HCC_HDR_TOTAL_LEN);
        DECLARE_DFT_TRACE_KEY_INFO("hcc_tx_check_algin_fail",OAL_DFT_TRACE_FAIL);
        return -OAL_EINVAL;
    }

#ifdef CONFIG_HCC_DEBUG
    oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "hcc_tx_etc into queue:%d, main:%d, sub:%d",
                  queue_id, param->main_type, param->sub_type);
    oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "hcc tx pkt seq:%d", hdr->seq);
    oal_print_hex_dump(OAL_NETBUF_DATA(netbuf), HCC_HDR_TOTAL_LEN, 8, "hcc hdr");
    oal_print_hex_dump(OAL_NETBUF_DATA(netbuf)+HCC_HDR_TOTAL_LEN+hdr->pad_payload,
                            hdr->pay_len, 8, "hcc payload");
#endif

    /*android wakelock,one netbuf one lock*/
    oal_wake_lock(&hcc->tx_wake_lock);

    pst_cb_stru = (struct hcc_tx_cb_stru*)OAL_NETBUF_CB(netbuf);
    pst_cb_stru->destory = hcc_tx_netbuf_destory;
    pst_cb_stru->magic = HCC_TX_WAKELOCK_MAGIC;
    pst_cb_stru->qtype = queue_id;

    oal_netbuf_list_tail(&pst_hcc_queue->data_queue, netbuf);

    hcc_sched_transfer(hcc);

    return OAL_SUCC;
}
/*lint -e19 */
oal_module_symbol(hcc_tx_etc);
/*lint +e19 */


OAL_STATIC  oal_void hcc_transfer_rx_handler(struct hcc_handler* hcc,oal_netbuf_stru* netbuf)
{
    /*get the rx buf and enqueue*/

    oal_netbuf_list_tail(&hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].data_queue,
                    netbuf);
    if(0 == hcc_rx_thread_enable_etc)
        hcc_send_rx_queue_etc(hcc,DATA_LO_QUEUE);
}


OAL_STATIC  oal_void hcc_transfer_rx_handler_replace(struct hcc_handler* hcc,oal_netbuf_stru* pst_netbuf_new)
{
    oal_netbuf_stru* pst_netbuf_old;
    hcc_trans_queue *pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE];

    /*remove from head*/
    pst_netbuf_old = oal_netbuf_delist(&pst_hcc_queue->data_queue);
    if(OAL_LIKELY(NULL != pst_netbuf_old))
    {
        pst_hcc_queue->loss_pkts++;
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "hcc_rx loss pkts :%u", pst_hcc_queue->loss_pkts);
        oal_netbuf_free(pst_netbuf_old);
    }

    /*get the rx buf and enqueue*/
    oal_netbuf_list_tail(&pst_hcc_queue->data_queue,
                    pst_netbuf_new);

    if(0 == hcc_rx_thread_enable_etc)
        hcc_send_rx_queue_etc(hcc,DATA_LO_QUEUE);
}


oal_int32 hcc_bus_rx_handler(oal_void* data)
{
    oal_int32 ret = OAL_SUCC;
    oal_uint32 scatt_count;
    oal_netbuf_stru* netbuf;
    oal_netbuf_head_stru    head;
    struct hcc_handler* hcc = (struct hcc_handler*)data;
    struct hcc_header * pst_hcc_head;


    if(OAL_WARN_ON(!hcc))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "hcc is null:%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    };

    oal_netbuf_head_init(&head);

    /*调用bus底层的实体处理接口获取netbuf list*/
    ret = hcc_bus_rx_netbuf_list(HCC_TO_BUS(hcc), &head);
    if(OAL_UNLIKELY(OAL_SUCC != ret))
    {
        return ret;
    }

    scatt_count = oal_netbuf_list_len(&head);

    if(OAL_UNLIKELY(scatt_count >= HCC_RX_ASSEM_INFO_MAX_NUM))
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "WARN:why so much scatt buffs[%u]?", scatt_count);
        scatt_count = 0;
    }

    hcc->hcc_transer_info.rx_assem_info.info[scatt_count]++;

    for(;;)
    {
        netbuf = oal_netbuf_delist(&head);
        if(NULL == netbuf)
            break;

        /*RX 流控，当接收来不及处理时丢掉最旧的数据包,SDIO不去读DEVICE侧会堵住*/

        pst_hcc_head = (struct hcc_header *)OAL_NETBUF_DATA(netbuf);
        if(OAL_UNLIKELY(OAL_TRUE != hcc_check_header_vaild(pst_hcc_head)))
        {
            oal_print_hex_dump((oal_uint8 * )pst_hcc_head, HCC_HDR_TOTAL_LEN, 16, "invaild hcc header: ");
            hcc_bus_try_to_dump_device_mem(hcc_get_current_110x_bus(), OAL_FALSE);
        }

        if(OAL_TRUE == hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].flow_ctrl.enable)
        {
            if(oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].data_queue)
               > hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].flow_ctrl.high_waterline)
            {
                hcc_transfer_rx_handler_replace(hcc, netbuf);
            }
            else
            {
                hcc_transfer_rx_handler(hcc, netbuf);
            }
        }
        else
        {
            hcc_transfer_rx_handler(hcc, netbuf);
        }
    }

    /*sched hcc thread*/
    if(1 == hcc_rx_thread_enable_etc)
        hcc_sched_transfer(hcc);
#ifdef CONFIG_HCC_DEBUG
    //oal_msleep(500);
#endif
    return OAL_SUCC;
}

oal_void hcc_rx_submit(struct hcc_handler* hcc, oal_netbuf_stru* pst_netbuf)
{
    oal_netbuf_list_tail(&hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[DATA_LO_QUEUE].data_queue,
                pst_netbuf);
}

oal_int32 sdio_credit_info_update_handler_etc(oal_void* data)
{
#if 0
    struct hcc_handler* hcc = (struct hcc_handler*)data;
    if(hcc_get_large_pkt_free_cnt_etc(hcc) > CONFIG_CREDIT_MSG_FLOW_WATER_LINE)
        hcc_sched_transfer(hcc);
#endif
    return OAL_SUCC;
}

OAL_STATIC OAL_INLINE hcc_netbuf_queue_type hcc_queue_map_to_netbuf_queue(hcc_queue_type qtype)
{
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    if(CTRL_QUEUE == qtype || DATA_HI_QUEUE == qtype)
#else
    if(DATA_HI_QUEUE == qtype)
#endif
        return HCC_NETBUF_HIGH_QUEUE;

    return HCC_NETBUF_NORMAL_QUEUE;
}

OAL_STATIC oal_int32 _queues_not_flowctrl_len_check(struct hcc_handler* hcc,
                                                              hcc_chan_type dir)
{
    oal_int32 i;
    for(i = 0; i < HCC_QUEUE_COUNT;i++)
    {
        if(OAL_TRUE != hcc->hcc_transer_info.hcc_queues[dir].queues[i].flow_ctrl.enable &&
          HCC_FLOWCTRL_CREDIT != hcc->hcc_transer_info.hcc_queues[dir].queues[i].flow_ctrl.flow_type)
        {
            if (oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[dir].queues[i].data_queue))
            {
                    //TODO:
                    if(OAL_UNLIKELY((NULL == HCC_TO_DEV(hcc)) || (NULL == HCC_TO_BUS(hcc))))
                    {
                        oal_print_hi11xx_log(HI11XX_LOG_WARN, "hcc'dev is %p, hcc'dev bus is %p", HCC_TO_DEV(hcc), HCC_TO_DEV(hcc) ? HCC_TO_BUS(hcc) : NULL);
                        //return OAL_FALSE;
                    }
                    else
                    {
                        if(OAL_TRUE == hcc_bus_check_tx_condition(HCC_TO_BUS(hcc), hcc_queue_map_to_netbuf_queue((hcc_queue_type)i)))
                        {
                            /*发送通道畅通*/
                            oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "sdio tx cond true");
                            return OAL_TRUE;
                        }
                    }
            }
        }
    }

    return OAL_FALSE;
}

/*TBD:This is not good,
  PCIE 这层没有反压流控，靠中断反压*/
OAL_STATIC oal_int32 _queues_pcie_len_check(struct hcc_handler* hcc,
                                                              hcc_chan_type dir)
{
    oal_int32 i;
    for(i = 0; i < HCC_QUEUE_COUNT;i++)
    {
        if (oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[dir].queues[i].data_queue))
        {
                //TODO:
                if(OAL_UNLIKELY((NULL == HCC_TO_DEV(hcc)) || (NULL == HCC_TO_BUS(hcc))))
                {
                    oal_print_hi11xx_log(HI11XX_LOG_WARN, "hcc'dev is %p, hcc'dev bus is %p", HCC_TO_DEV(hcc), HCC_TO_DEV(hcc) ? HCC_TO_BUS(hcc) : NULL);
                    //return OAL_FALSE;
                }
                else
                {
                    if(OAL_TRUE == hcc_bus_check_tx_condition(HCC_TO_BUS(hcc), hcc_queue_map_to_netbuf_queue((hcc_queue_type)i)))
                    {
                        /*发送通道畅通*/
                        oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "pcie check tx cond true");
                        return OAL_TRUE;
                    }
                }
        }
    }

    return OAL_FALSE;
}

/*wether the hcc flow ctrl queues have data.*/
OAL_STATIC oal_int32 _queues_flow_ctrl_len_check(struct hcc_handler* hcc, hcc_chan_type dir)
{
    oal_int32 i ;
    for(i = 0; i < HCC_QUEUE_COUNT;i++)
    {
        if(OAL_TRUE == hcc->hcc_transer_info.hcc_queues[dir].queues[i].flow_ctrl.enable)
        {
            if(HCC_FLOWCTRL_SDIO == hcc->hcc_transer_info.hcc_queues[dir].queues[i].flow_ctrl.flow_type)
            {
                if(oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[dir].queues[i].data_queue)
                    && (D2H_MSG_FLOWCTRL_ON == hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag))
                {
                        oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "sdio fc true");
                        return 1;
                }
            }
        }
        else
        {
            if(HCC_FLOWCTRL_CREDIT == hcc->hcc_transer_info.hcc_queues[dir].queues[i].flow_ctrl.flow_type)
            {
                /*credit flowctrl*/
                if (oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[dir].queues[i].data_queue)
                    && (hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt > hcc_credit_bottom_value_etc))
                {
                        oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "sdio fcr true");
                        return 1;
                }
            }
        }
    }

    return 0;
}

oal_uint32 hcc_queues_flow_ctrl_len_etc(struct hcc_handler* hcc, hcc_chan_type dir)
{
    oal_int32 i ;
    oal_uint32 total = 0;
    for(i = 0; i < HCC_QUEUE_COUNT;i++)
    {
        if(OAL_TRUE == hcc->hcc_transer_info.hcc_queues[dir].queues[i].flow_ctrl.enable)
        {
            total += oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[dir].queues[i].data_queue);
        }
    }
    return total;
}

/*wether the hcc queue have data.*/
OAL_STATIC oal_int32 _queues_len_check(struct hcc_handler* hcc, hcc_chan_type dir)
{
    oal_int32 i ;
    for(i = 0; i < HCC_QUEUE_COUNT;i++)
    {
        if(oal_netbuf_list_len(&hcc->hcc_transer_info.hcc_queues[dir].queues[i].data_queue))
        {
                oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "d:%d, qid:%d had pkts", dir, i);
                return OAL_TRUE;
        }
    }
    return OAL_FALSE;
}


OAL_STATIC  oal_int32  hcc_thread_wait_event_cond_check(struct hcc_handler* hcc)
{
    oal_int32 ret;
    if(OAL_WARN_ON(NULL == hcc))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: hcc is null",__FUNCTION__);
         return OAL_FALSE;
    };
    /*please first check the condition
      which may be ok likely to reduce the cpu mips*/
    if(HCC_BUS_PCIE == hcc->bus_dev->cur_bus->bus_type)
    {
        ret = ((_queues_len_check(hcc, HCC_RX))
                   ||(_queues_pcie_len_check(hcc, HCC_TX)) || (hcc_bus_pending_signal_check(hcc->bus_dev->cur_bus))
                   ||(hcc->p_hmac_tcp_ack_need_schedule_func && OAL_TRUE == hcc->p_hmac_tcp_ack_need_schedule_func()));
    }
    else if(HCC_BUS_SDIO == hcc->bus_dev->cur_bus->bus_type)
    {
        ret = (_queues_flow_ctrl_len_check(hcc, HCC_TX)
                   ||(_queues_len_check(hcc, HCC_RX))
                   ||(_queues_not_flowctrl_len_check(hcc, HCC_TX))
                   ||(hcc->p_hmac_tcp_ack_need_schedule_func && OAL_TRUE == hcc->p_hmac_tcp_ack_need_schedule_func()));
    }
    else
    {
        ret = OAL_FALSE;
    }

#ifdef _PRE_CONFIG_WLAN_THRANS_THREAD_DEBUG
    if(OAL_TRUE == ret)
        hcc->hcc_transer_info.thread_stat.wait_event_run_count++;
    if(OAL_FALSE == ret)
        hcc->hcc_transer_info.thread_stat.wait_event_block_count++;
#endif
    oal_print_hi11xx_log(HI11XX_LOG_VERBOSE, "wait_cond:%d", ret);
    return ret;
}

OAL_STATIC oal_int32 hcc_send_single_descr(struct hcc_handler *hcc, oal_netbuf_stru* netbuf)
{
    oal_netbuf_head_stru head_send;
    OAL_REFERENCE(hcc);

    if(OAL_WARN_ON(NULL == netbuf))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "netbuf is null!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    oal_netbuf_list_head_init(&head_send);
    oal_netbuf_list_tail(&head_send, netbuf);

    return hcc_bus_tx_netbuf_list(HCC_TO_BUS(hcc), &head_send, HCC_NETBUF_NORMAL_QUEUE);
}

oal_int32 hcc_send_descr_control_data_etc(struct hcc_handler *hcc, hcc_descr_type descr_type, oal_void* data, oal_uint32 ul_size)
{
    oal_int32 ret = OAL_SUCC;
    oal_netbuf_stru* netbuf = NULL;
    struct hcc_descr_header* dscr_hdr;
    netbuf = hcc_tx_assem_descr_get(hcc);
    if(OAL_WARN_ON(NULL == netbuf))
    {
        oal_print_hi11xx_log(HI11XX_LOG_ERR, "No descr mem!");
        return -OAL_ENOMEM;
    }

    dscr_hdr = (struct hcc_descr_header*)OAL_NETBUF_DATA(netbuf);
    dscr_hdr->descr_type = descr_type;

    if(ul_size)
    {
        if(OAL_WARN_ON(NULL == data))
        {
            hcc_tx_assem_descr_put(hcc,netbuf);
            return -OAL_EINVAL;
        }
        if(OAL_WARN_ON(ul_size + OAL_SIZEOF(struct hcc_descr_header) > OAL_NETBUF_LEN(netbuf)))
        {
            OAL_IO_PRINT("invaild request size:%u,max size:%u\r\n",
                            (oal_uint32)(ul_size + OAL_SIZEOF(struct hcc_descr_header)),(oal_uint32)OAL_NETBUF_LEN(netbuf));
            hcc_tx_assem_descr_put(hcc,netbuf);
            return -OAL_EINVAL;
        }

        /*lint -e124*/
        oal_memcopy((oal_void *)((oal_uint8 *)OAL_NETBUF_DATA(netbuf) + OAL_SIZEOF(struct hcc_descr_header)),data,ul_size);
    }


    ret = hcc_send_single_descr(hcc,netbuf);


    hcc_tx_assem_descr_put(hcc,netbuf);
    return ret;
}


oal_int32 hcc_tx_netbuf_queue_switch_etc(struct hcc_handler *hcc, hcc_netbuf_queue_type queue_type)
{
    return hcc_send_descr_control_data_etc(hcc, HCC_NETBUF_QUEUE_SWITCH,&queue_type,OAL_SIZEOF(queue_type));
}

oal_int32 hcc_tx_netbuf_test_and_switch_high_pri_queue_etc(struct hcc_handler *hcc, hcc_netbuf_queue_type pool_type)
{
    oal_int32 ret = OAL_EFAIL;
    if(HCC_NETBUF_HIGH_QUEUE == pool_type)
    {
        ret = hcc_tx_netbuf_queue_switch_etc(hcc, HCC_NETBUF_HIGH_QUEUE);
    }
    return ret;
}

oal_int32 hcc_tx_netbuf_restore_normal_pri_queue_etc(struct hcc_handler *hcc, hcc_netbuf_queue_type pool_type)
{
    oal_int32 ret = OAL_EFAIL;
    if(HCC_NETBUF_HIGH_QUEUE == pool_type)
    {
        ret = hcc_tx_netbuf_queue_switch_etc(hcc, HCC_NETBUF_NORMAL_QUEUE);
    }
    return ret;
}

/*归还待发送队列*/
oal_void hcc_restore_tx_netbuf(struct hcc_handler *hcc, oal_netbuf_stru *pst_netbuf)
{
    struct hcc_tx_cb_stru* pst_cb_stru;
    hcc_trans_queue* pst_hcc_queue ;

    pst_cb_stru = (struct hcc_tx_cb_stru*)OAL_NETBUF_CB(pst_netbuf);

    if(OAL_UNLIKELY(HCC_TX_WAKELOCK_MAGIC != pst_cb_stru->magic || pst_cb_stru->qtype >= HCC_QUEUE_COUNT))
    {
#ifdef CONFIG_PRINTK
    	printk(KERN_EMERG "BUG: tx netbuf:%p on CPU#%d,magic:%08x should be %08x, qtype:%u\n", pst_cb_stru,
    	                    raw_smp_processor_id(),pst_cb_stru->magic,HCC_TX_WAKELOCK_MAGIC, pst_cb_stru->qtype);
        print_hex_dump(KERN_ERR, "tx_netbuf_magic", DUMP_PREFIX_ADDRESS, 16, 1,
	       (oal_uint8 *)pst_netbuf, sizeof(oal_netbuf_stru), true);
        printk(KERN_ERR"\n");
#endif
        //OAL_WARN_ON(1);
        DECLARE_DFT_TRACE_KEY_INFO("tx_restore_wakelock_crash", OAL_DFT_TRACE_EXCEP);
        return;
    }

    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[pst_cb_stru->qtype];

    oal_netbuf_addlist(&pst_hcc_queue->data_queue, pst_netbuf);
}

oal_void  hcc_restore_assemble_netbuf_list(struct hcc_handler *hcc)
{
    hcc_queue_type type;
    oal_netbuf_head_stru *assembled_head;

    type = hcc->hcc_transer_info.tx_assem_info.assembled_queue_type;
    assembled_head = &hcc->hcc_transer_info.tx_assem_info.assembled_head;

    if(type >= HCC_QUEUE_COUNT)
    {
        type = DATA_LO_QUEUE;
    }

    if(!oal_netbuf_list_empty(assembled_head))
        oal_netbuf_splice_sync(&hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type].data_queue,
                                assembled_head);
}

OAL_STATIC  oal_int32 hcc_send_assemble_reset(struct hcc_handler *hcc)
{
    oal_int32 ret = OAL_SUCC;

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_reset_count++;

    /*当只发送一个聚合描述符包，并且聚合个数为0描述通知Device 重置聚合信息*/
    ret = hcc_send_descr_control_data_etc(hcc, HCC_DESCR_ASSEM_RESET,NULL,0);

    hcc_restore_assemble_netbuf_list(hcc);

    return ret;
}

OAL_STATIC oal_int32 hcc_send_data_packet(struct hcc_handler* hcc,
                                                oal_netbuf_head_stru *head,
                                                hcc_queue_type   type,
                                                oal_netbuf_head_stru* next_assembled_head,
                                                hcc_send_mode mode,
                                                oal_uint32* remain_len)
{
    oal_uint8* buf = NULL;
    oal_uint32 total_send;
    oal_int32               ret = OAL_SUCC;
    oal_netbuf_head_stru head_send;
    oal_netbuf_stru*  netbuf, * descr_netbuf,*netbuf_t;
    oal_uint32* info;
    hcc_trans_queue *pst_hcc_queue;
    oal_uint8   uc_credit;

    if(0 == *remain_len)
    {
        return OAL_SUCC;
    }

    oal_print_hi11xx_log(HI11XX_LOG_DBG, "send queue:%d, mode:%d,next assem len:%d",
                                        type, mode,
                                        oal_netbuf_list_len(next_assembled_head));

    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type];

    if(HCC_FLOWCTRL_CREDIT== pst_hcc_queue->flow_ctrl.flow_type)
    {
        /*credit flowctrl*/
        uc_credit    = hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt;

        /* 高优先级流控: credit值为0时不发送 */
        if (!(uc_credit > hcc_credit_bottom_value_etc))
        {
            return OAL_SUCC;
        }
    }

    descr_netbuf = hcc_tx_assem_descr_get(hcc);
    if(NULL == descr_netbuf)
    {
        ret = -OAL_ENOMEM;
        /*lint -e801*/
        goto failed_get_assem_descr;
    }

    info = hcc->hcc_transer_info.tx_assem_info.info;

    oal_netbuf_list_head_init(&head_send);

    if(oal_netbuf_list_empty(next_assembled_head))
    {
        /*single send*/
        netbuf = oal_netbuf_delist(head);
        if (NULL == netbuf)
        {
            OAL_IO_PRINT("netbuf is NULL [len:%d]\n", oal_netbuf_list_len(head));
            ret = OAL_SUCC;
            /*lint -e801*/
            goto failed_get_sig_buff;
        }

        netbuf_t = hcc_netbuf_len_align(netbuf, HISDIO_H2D_SCATT_BUFFLEN_ALIGN);
        if(OAL_UNLIKELY(NULL == netbuf_t))
        {
            /*return to the list*/
            oal_netbuf_addlist(head, netbuf);
            /*lint -e801*/
            goto failed_align_netbuf;
        }

        oal_netbuf_list_tail(&head_send, netbuf);
        info[0]++;
    }
    else
    {
        oal_uint32 assemble_len = oal_netbuf_list_len(next_assembled_head);
        if(OAL_UNLIKELY(assemble_len > HISDIO_HOST2DEV_SCATT_SIZE))
        {
            oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s error: assemble_len:%d > HISDIO_HOST2DEV_SCATT_SIZE:%d",__FUNCTION__,assemble_len,HISDIO_HOST2DEV_SCATT_SIZE);
        }
        else
        {
            info[assemble_len]++;
        }
        /*move the assem list to send queue*/
        oal_netbuf_splice_init(next_assembled_head,&head_send);
    }

    total_send = oal_netbuf_list_len(&head_send);

    if(*remain_len >= total_send)
        *remain_len -= total_send;
    else
        *remain_len = 0;

    if(OAL_WARN_ON(!oal_netbuf_list_empty(next_assembled_head)))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: next_assembled_head is empty",__FUNCTION__);
    };

    if(hcc_assem_send == mode)
        hcc_build_next_assem_descr(hcc, type, head,next_assembled_head,descr_netbuf, *remain_len);
    else
    {
        buf = OAL_NETBUF_DATA(descr_netbuf);
        *((oal_uint32*)buf) = 0;
    }


    /*add the assem descr buf*/
    oal_netbuf_addlist(&head_send, descr_netbuf);

    ret = hcc_bus_tx_netbuf_list(HCC_TO_BUS(hcc), &head_send, hcc_queue_map_to_netbuf_queue(type));

   g_pm_wifi_rxtx_count += total_send; //发送包统计 for pm



    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type].total_pkts += total_send;

    /* 高优先级流控: 更新credit值 */
    if(HCC_FLOWCTRL_CREDIT== pst_hcc_queue->flow_ctrl.flow_type)
    {
        oal_spin_lock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));

        uc_credit = hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt;
        uc_credit = (uc_credit > total_send) ? (oal_uint8)(uc_credit - total_send) : 0;
        hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt = uc_credit;

        oal_spin_unlock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));
    }

    descr_netbuf = oal_netbuf_delist(&head_send);
    if (OAL_PTR_NULL == descr_netbuf)
    {
        OAM_ERROR_LOG0(0,OAM_SF_ANY,"hcc_send_data_packet::oal_netbuf_delist fail.descr_netbuf is NULL.");
        ret = OAL_SUCC;
        goto failed_get_sig_buff;
    }

    hcc_tx_assem_descr_put(hcc, descr_netbuf);

    /*free the sent netbuf,release the wakelock*/
    hcc_tx_netbuf_list_free(&head_send);

    return ret;
failed_align_netbuf:
    hcc_tx_assem_descr_put(hcc, descr_netbuf);
failed_get_sig_buff:
failed_get_assem_descr:
    return ret;
}

oal_void hcc_clear_all_queues_etc(struct hcc_handler * hcc, oal_int32 is_need_lock)
{
    oal_int32 i;
    oal_netbuf_head_stru* pst_head;

    if(OAL_WARN_ON(NULL == hcc))
    {
        OAL_IO_PRINT("[E]hcc is null\n");
        return;
    }

    if(OAL_TRUE == is_need_lock)
    {
        hcc_tx_transfer_lock(hcc);
    }

    /*Restore assem queues*/
    hcc_restore_assemble_netbuf_list(hcc);

    /*Clear all tx queues*/
    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        oal_uint32 ul_list_len;
        pst_head = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].data_queue;
        ul_list_len = oal_netbuf_list_len(pst_head);
        hcc_tx_netbuf_list_free(pst_head);
        if(ul_list_len)
        {
            OAL_IO_PRINT("Clear queue:%d,total %u hcc tx pkts!\n", i, ul_list_len);
        }
    }

    if(OAL_TRUE == is_need_lock)
    {
        hcc_tx_transfer_unlock(hcc);
    }

    /*Clear all rx queues*/
    if(OAL_TRUE == is_need_lock)
    {
        hcc_rx_transfer_lock(hcc);
    }

    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        oal_uint32 ul_list_len;
        pst_head = &hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].data_queue;
        ul_list_len = oal_netbuf_list_len(pst_head);
        oal_netbuf_queue_purge(pst_head);
        if(ul_list_len)
        {
            OAL_IO_PRINT("Clear queue:%d,total %u hcc rx pkts!\n", i, ul_list_len);
        }
    }

    if(OAL_TRUE == is_need_lock)
    {
        hcc_rx_transfer_unlock(hcc);
    }
}

oal_void hcc_change_state_etc(struct hcc_handler * hcc, oal_int32 state)
{
    oal_int32  old_state, new_state;
    if(OAL_WARN_ON(NULL == hcc))
    {
        return;
    }

    old_state = oal_atomic_read(&hcc->state);

    oal_atomic_set(&hcc->state, state);

    new_state = oal_atomic_read(&hcc->state);

    if(old_state != new_state)
    {
        OAL_IO_PRINT("hcc state %s[%d]=>%s[%d]\n",
                     HCC_ON == old_state ? "on ":"off",
                     old_state,
                     HCC_ON == new_state ? "on ":"off",
                     new_state);
    }
}

oal_void hcc_change_state_exception_etc(oal_void)
{
    hcc_change_state_etc(hcc_get_110x_handler(), HCC_EXCEPTION);
}


oal_void hcc_enable_etc(struct hcc_handler * hcc, oal_int32 is_need_lock)
{
    oal_int32 i;
    if(OAL_WARN_ON(NULL == hcc))
    {
        return;
    }

    if(OAL_TRUE == is_need_lock)
    {
        hcc_transfer_lock(hcc);
    }

    hcc_clear_all_queues_etc(hcc, OAL_FALSE);

    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.is_stopped = OAL_FALSE;
        oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
    }

    hcc_change_state_etc(hcc, HCC_ON);

    if(OAL_TRUE == is_need_lock)
    {
        hcc_transfer_unlock(hcc);
    }

}


oal_void hcc_disable_etc(struct hcc_handler * hcc, oal_int32 is_need_lock)
{
    if(OAL_UNLIKELY(NULL == hcc))
    {
        oal_print_hi11xx_log(HI11XX_LOG_WARN, "hcc is null");
        return;
    }

    hcc_change_state_etc(hcc, HCC_OFF);

    /*disable flow ctrl detect timer*/
    hcc_dev_flowctr_timer_del(hcc);
    oal_cancel_delayed_work_sync(&hcc->hcc_transer_info.tx_flow_ctrl.worker);

    if(OAL_TRUE == is_need_lock)
    {
        hcc_transfer_lock(hcc);
    }

    hcc_clear_all_queues_etc(hcc, OAL_FALSE);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    atomic_set(&hcc->tx_seq, 0);
#endif
    if(OAL_TRUE == is_need_lock)
    {
        hcc_transfer_unlock(hcc);
    }
    /*must wake up tx thread after clear the hcc queues*/
    OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);

}


int hcc_send_tx_queue_etc(struct hcc_handler *hcc, hcc_queue_type type)
{
    oal_int32       count = 0;
    oal_netbuf_head_stru *head;
    hcc_trans_queue *pst_hcc_queue;
    hcc_netbuf_queue_type qtype = HCC_NETBUF_NORMAL_QUEUE;

    if(OAL_UNLIKELY(NULL == hcc))
    {
         OAL_IO_PRINT("%s error: hcc is null",__FUNCTION__);
         return 0;
    };

    if(OAL_UNLIKELY(type >= HCC_QUEUE_COUNT))
    {
        OAL_IO_PRINT("unkown hcc queue type %d\n", type);
        return count;
    }

    pst_hcc_queue = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[type];
    head = &pst_hcc_queue->data_queue;

    /*TBD:PCIE 内存在 PCIE驱动侧释放， SDIO在HCC中发送完成后释放，
    后续归一,SDIO聚合需要下移到SDIO驱动,聚合独立一个模块*/
    hcc_tx_transfer_lock(hcc);
#ifdef WIN32
    if(0)
#else
    if(HCC_BUS_PCIE == hcc->bus_dev->cur_bus->bus_type)
#endif
    {

        /*参考 hcc_send_data_packet*/
        if(oal_netbuf_list_empty(head))
        {
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "queue type %d is empty\n", type);
            hcc_tx_transfer_unlock(hcc);
            return count;
        }

        qtype = hcc_queue_map_to_netbuf_queue(type);
        //count += oal_pcie_send_netbuf_list(oal_get_default_pcie_handler(), head, qtype);
        count += hcc_bus_tx_netbuf_list(HCC_TO_BUS(hcc), head, qtype);
        hcc->hcc_transer_info.tx_assem_info.pkt_cnt += count;
        g_pm_wifi_rxtx_count += (oal_uint32)count; /*delete p_auto_freq_count_func,add this*/

        oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
            if((oal_netbuf_list_len(&pst_hcc_queue->data_queue) < pst_hcc_queue->flow_ctrl.low_waterline))
            {
                if(OAL_TRUE == pst_hcc_queue->flow_ctrl.is_stopped)
                {
                    pst_hcc_queue->flow_ctrl.is_stopped = OAL_FALSE;

                    //OAM_INFO_LOG3(0, OAM_SF_TX, "{hcc_tx_etc: start_netdev_queue, queue_len[%d] = %d}, threshold = %d", type,
                    //        oal_netbuf_list_len(&pst_hcc_queue->data_queue), pst_hcc_queue->flow_ctrl.low_waterline);
                    hcc_tx_network_start_subq_etc(hcc, (oal_uint16)pst_hcc_queue->wlan_queue_id);
                }
                OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
            }

#else
            if(hcc_queues_flow_ctrl_len_etc(hcc, HCC_TX) < HCC_TX_FLOW_LO_LEVEL)
            {
                OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
                hcc_tx_network_startall_queues_etc(hcc);
            }
#endif
        oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
        hcc_tx_transfer_unlock(hcc);
        return count;
    }
    else/*SDIO & USB*/
    {
        oal_uint8  uc_credit;
        oal_int32       ret = 0;
        oal_uint32  queue_len, remain_len, remain_len_t;
        oal_netbuf_head_stru *next_assembled_head;
        hcc_send_mode send_mode;
        hcc_netbuf_queue_type pool_type;
        oal_uint32 ul_pool_type_flag = OAL_FALSE;

        if(oal_netbuf_list_empty(head))
        {
            oal_print_hi11xx_log(HI11XX_LOG_DBG, "queue type %d is empty", type);
            hcc_tx_transfer_unlock(hcc);
            return count;
        }

        queue_len = oal_netbuf_list_len(head);
        remain_len = queue_len;

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
        oal_print_hi11xx_log(HI11XX_LOG_DBG, "before_update: hcc_send_tx_queue_etc: queue_type = %d, burst_limit = %d, remain_len = %d",
            type, pst_hcc_queue->burst_limit, remain_len);
        remain_len = OAL_MIN(pst_hcc_queue->burst_limit, remain_len);
#endif

        remain_len_t = remain_len;
        next_assembled_head = &hcc->hcc_transer_info.tx_assem_info.assembled_head;

        if(OAL_UNLIKELY(!oal_netbuf_list_empty(next_assembled_head)))
        {
            /*First enter the queue, should single send*/
            OAL_IO_PRINT("[WARN]reset assemble package![queue type:%d,len:%d]\n",
                                (oal_int32)type, oal_netbuf_list_len(next_assembled_head));
            if(OAL_SUCC != hcc_send_assemble_reset(hcc))
            {
                hcc_tx_transfer_unlock(hcc);
                /*send one pkt*/
                return 1;
            }
        }

        send_mode = pst_hcc_queue->send_mode;
        pool_type = (hcc_netbuf_queue_type)pst_hcc_queue->netbuf_pool_type;

        for(;;)
        {
            if(0 == remain_len)
            {
                break;
            }

#ifdef CONFIG_SDIO_MSG_FLOWCTRL
            if(OAL_TRUE == pst_hcc_queue->flow_ctrl.enable)
            {
                if(HCC_FLOWCTRL_SDIO == pst_hcc_queue->flow_ctrl.flow_type)
                {
                    /*flow ctrl*/
                    if(D2H_MSG_FLOWCTRL_ON != hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag)  {
                        oal_print_hi11xx_log(HI11XX_LOG_DBG, "[WARNING]can't send data, flow off");
                        if(!oal_netbuf_list_empty(&hcc->hcc_transer_info.tx_assem_info.assembled_head))
                        {
                            if(OAL_SUCC != hcc_send_assemble_reset(hcc))
                            {
                                hcc_tx_transfer_unlock(hcc);
                                count++;
                                /*send one pkt*/
                                return count;
                            }
                        }
                        hcc_tx_transfer_unlock(hcc);
                        return count;
                    }
                }
            }
#endif
            if(HCC_FLOWCTRL_CREDIT== pst_hcc_queue->flow_ctrl.flow_type)
            {
                uc_credit    = hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt;

                /*高优先级如果没有内存，直接返回规避死循环问题。*/
                if (!(uc_credit > hcc_credit_bottom_value_etc))
                {
                    if(ul_pool_type_flag == OAL_TRUE)
                    {
                        /*恢复成普通优先级*/
                        hcc_tx_netbuf_restore_normal_pri_queue_etc(hcc,pool_type);
                    }
                    hcc_tx_transfer_unlock(hcc);
                    return OAL_SUCC;
                }

                if(ul_pool_type_flag == OAL_FALSE)
                {
                    if(OAL_SUCC == hcc_tx_netbuf_test_and_switch_high_pri_queue_etc(hcc,pool_type))
                    {
                        ul_pool_type_flag = OAL_TRUE;
                    }
                }
            }

            ret = hcc_send_data_packet(hcc, head, type, next_assembled_head, send_mode, &remain_len);

            if(OAL_UNLIKELY(remain_len > remain_len_t))
            {
                oal_print_hi11xx_log(HI11XX_LOG_ERR, "%s error: remain_len:%d > remain_len_t:%d",__FUNCTION__,remain_len,remain_len_t);
                hcc_tx_transfer_unlock(hcc);
                return OAL_FAIL;
            };

            if(OAL_LIKELY(OAL_SUCC == ret))
            {
                count += (oal_int32)(remain_len_t - remain_len);
                hcc->hcc_transer_info.tx_assem_info.pkt_cnt += (remain_len_t - remain_len);
            }

#ifdef CONFIG_SDIO_MSG_FLOWCTRL
            oal_spin_lock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
            if((oal_netbuf_list_len(&pst_hcc_queue->data_queue) < pst_hcc_queue->flow_ctrl.low_waterline))
            {
                if(OAL_TRUE == pst_hcc_queue->flow_ctrl.is_stopped)
                {
                    pst_hcc_queue->flow_ctrl.is_stopped = OAL_FALSE;

                    //OAM_INFO_LOG3(0, OAM_SF_TX, "{hcc_tx_etc: start_netdev_queue, queue_len[%d] = %d}, threshold = %d", type,
                    //        oal_netbuf_list_len(&pst_hcc_queue->data_queue), pst_hcc_queue->flow_ctrl.low_waterline);
                    hcc_tx_network_start_subq_etc(hcc, (oal_uint16)pst_hcc_queue->wlan_queue_id);
                }
                OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
            }

#else
            if(hcc_queues_flow_ctrl_len_etc(hcc, HCC_TX) < HCC_TX_FLOW_LO_LEVEL)
            {
                OAL_WAIT_QUEUE_WAKE_UP_INTERRUPT(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);
                hcc_tx_network_startall_queues_etc(hcc);
            }
#endif
            oal_spin_unlock_bh(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
#endif
        }

        if(HCC_FLOWCTRL_CREDIT== pst_hcc_queue->flow_ctrl.flow_type)
        {
            if(ul_pool_type_flag == OAL_TRUE)
            {
                hcc_tx_netbuf_restore_normal_pri_queue_etc(hcc,pool_type);
            }
        }

        hcc_tx_transfer_unlock(hcc);
        return count;
    }

}


oal_int32 hcc_rx_register_etc(struct hcc_handler *hcc, oal_uint8 mtype, hcc_rx_post_do post_do, hcc_rx_pre_do pre_do)
{
    hcc_rx_action    *rx_action;

	if(OAL_WARN_ON(NULL == hcc))
	{
		return -OAL_EINVAL;
	}

    rx_action = &(hcc->hcc_transer_info.rx_action_info.action[mtype]);

    if(OAL_WARN_ON(NULL != rx_action->post_do))
    {
#ifdef CONFIG_PRINTK
        OAL_IO_PRINT("repeat regist hcc action :%d,it's had %pF\n",mtype,rx_action->post_do);
#else
		OAL_IO_PRINT("repeat regist hcc action :%d,it's had %p\n",mtype,rx_action->post_do);
#endif
        return -OAL_EBUSY;
    }
    /*此处暂时不加互斥锁，由流程保证。*/
    rx_action->post_do = post_do;
    rx_action->pre_do = pre_do;

    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(hcc_rx_register_etc);
/*lint +e19*/

oal_int32 hcc_rx_unregister_etc(struct hcc_handler *hcc, oal_uint8 mtype)
{
    hcc_rx_action    *rx_action;

    if (OAL_WARN_ON((NULL == hcc) || (mtype >= HCC_ACTION_TYPE_BUTT)))
    {
        OAL_IO_PRINT("Invalid params:hcc = %p, main type = %u\n", hcc, mtype);
        return -OAL_EFAIL;
    }

    rx_action = &hcc->hcc_transer_info.rx_action_info.action[mtype];

    oal_memset((oal_void*)rx_action,0,OAL_SIZEOF(hcc_rx_action));

    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(hcc_rx_unregister_etc);
/*lint +e19*/

OAL_STATIC oal_uint32 hcc_check_rx_netbuf_vaild(struct hcc_header *hdr, oal_int32 netbuf_len)
{
    if(OAL_UNLIKELY(OAL_TRUE != hcc_check_header_vaild(hdr)))
        return OAL_FALSE;

    if(OAL_UNLIKELY(hdr->pad_payload + HCC_HDR_TOTAL_LEN +  hdr->pay_len > netbuf_len))
        return OAL_FALSE;

    return OAL_TRUE;
}


OAL_STATIC  oal_int32 hcc_rx(struct hcc_handler *hcc, oal_netbuf_stru* netbuf)
{
    struct hcc_header * hdr;
    oal_int32               netbuf_len;
    oal_int32               extend_len;

    if(OAL_UNLIKELY(!hcc))
    {
        OAL_IO_PRINT("hcc is null r failed!%s\n",__FUNCTION__);
        return -OAL_EINVAL;
    }

    hdr = (struct hcc_header *)OAL_NETBUF_DATA(netbuf);
    netbuf_len = (oal_int32) OAL_NETBUF_LEN(netbuf);

    if(OAL_UNLIKELY(OAL_TRUE != hcc_check_rx_netbuf_vaild(hdr, netbuf_len)))
    {
        oal_print_hex_dump((oal_uint8 * )hdr, (oal_int32)OAL_NETBUF_LEN(netbuf), 16, "invaild hcc_rx header: ");
		if(HCC_TO_DEV(hcc))
		    hcc_bus_try_to_dump_device_mem(HCC_TO_BUS(hcc), OAL_FALSE);
        return -OAL_EFAIL;
    }


    extend_len = HCC_HDR_TOTAL_LEN - HCC_HDR_LEN - hdr->pad_hdr;

    if(hdr->pad_payload)
    {
        /*memmove*/
        oal_uint8* extend_addr = (oal_uint8*)hdr + HCC_HDR_LEN + hdr->pad_hdr;
        oal_memmove(extend_addr + hdr->pad_payload,extend_addr,(oal_uint32) extend_len);
    }

    if(HCC_HDR_LEN + hdr->pad_hdr + hdr->pad_payload > OAL_NETBUF_LEN(netbuf))
    {
        /*add check detail*/
        OAL_IO_PRINT("[Error]wrong pad_hdr,hcc hdr too long,len:%d!\n",(oal_int32)OAL_NETBUF_LEN(netbuf));
        oal_print_hex_dump((oal_uint8*)OAL_NETBUF_DATA(netbuf), (oal_int32)OAL_NETBUF_LEN(netbuf), 32, "rx wrong data: ");
        /*print the data buff here!*/
        return -OAL_EINVAL;
    }

#ifdef CONFIG_HCC_DEBUG
    OAL_IO_PRINT("[WIFI]hcc rx pkt seq:%d\n", hdr->seq);
    oal_print_hex_dump((oal_uint8*)hdr, HCC_HDR_TOTAL_LEN, 8, "hcc hdr:");
    oal_print_hex_dump(OAL_NETBUF_DATA(netbuf)+HCC_HDR_TOTAL_LEN+hdr->pad_payload,
                            hdr->pay_len, 8, "hcc payload:");
#endif

    oal_netbuf_pull(netbuf, HCC_HDR_LEN + hdr->pad_hdr + hdr->pad_payload);

    /*传出去的netbuf len 包含extend_len长度!*/
    oal_netbuf_trim(netbuf, OAL_NETBUF_LEN(netbuf) - hdr->pay_len - (oal_uint32)extend_len);

    OAL_NETBUF_NEXT(netbuf) = NULL;
    OAL_NETBUF_PREV(netbuf) = NULL;

    return OAL_SUCC;
}

oal_int32 hcc_send_rx_queue_etc(struct hcc_handler *hcc, hcc_queue_type type)
{
    oal_int32 count = 0;
    oal_int32 pre_ret = OAL_SUCC;
    oal_uint8* pst_pre_context = NULL;
    oal_netbuf_head_stru* netbuf_hdr;
    hcc_rx_action    *rx_action;
    oal_netbuf_stru *pst_netbuf;
    hcc_netbuf_stru  st_hcc_netbuf;
    struct hcc_header * pst_hcc_head;

    if(OAL_WARN_ON(type >= HCC_QUEUE_COUNT))
        return -OAL_EINVAL ;

    netbuf_hdr = &hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[type].data_queue;

    /* 依次处理队列中每个netbuf */
    for(;;)
    {

        pst_netbuf = oal_netbuf_delist(netbuf_hdr);
        if(NULL == pst_netbuf)
        {
            break;
        }

        pst_hcc_head = (struct hcc_header *)OAL_NETBUF_DATA(pst_netbuf);

        if(OAL_UNLIKELY(OAL_TRUE != hcc_check_header_vaild(pst_hcc_head)))
        {
            oal_print_hex_dump((oal_uint8 * )pst_hcc_head, HCC_HDR_TOTAL_LEN, 16, "invaild hcc header: ");
            DECLARE_DFT_TRACE_KEY_INFO("hcc_rx_hdr_err", OAL_DFT_TRACE_EXCEP);
            OAM_ERROR_LOG0(0, OAM_SF_ANY, "invaild hcc rx head");
            count++;
            oal_netbuf_free(pst_netbuf);
            return count;
        }

        rx_action = &hcc->hcc_transer_info.rx_action_info.action[pst_hcc_head->main_type];

        pre_ret = OAL_SUCC;

        /*prepare*/
        if(rx_action->pre_do)
        {
            st_hcc_netbuf.pst_netbuf = pst_netbuf;
            st_hcc_netbuf.len = (oal_int32)OAL_NETBUF_LEN(pst_netbuf);
            pre_ret = rx_action->pre_do(hcc, pst_hcc_head->sub_type,&st_hcc_netbuf,&pst_pre_context);
        }

        if(OAL_SUCC == pre_ret)
        {
            if(OAL_LIKELY(OAL_SUCC == hcc_rx(hcc, pst_netbuf)))
            {
                if(OAL_LIKELY(NULL != rx_action->post_do))
                {
                    st_hcc_netbuf.pst_netbuf = pst_netbuf;
                    st_hcc_netbuf.len = (oal_int32)OAL_NETBUF_LEN(pst_netbuf);
                    //hcc_netbuf_stru.len = (oal_int32)pst_hcc_head->pay_len;
                    rx_action->post_do(hcc, pst_hcc_head->sub_type,&st_hcc_netbuf, pst_pre_context);
                }
                else
                {
                    OAM_ERROR_LOG2(0, OAM_SF_ANY,"hcc mtype:%d,stype:%d did't register cb function!",
                                pst_hcc_head->main_type,
                                pst_hcc_head->sub_type);
                    oal_print_hex_dump((oal_uint8*)pst_hcc_head, HCC_HDR_TOTAL_LEN, 32, "hcc invaild header: ");
                    oal_print_hex_dump(OAL_NETBUF_DATA(pst_netbuf),
                                       (oal_int32)OAL_NETBUF_LEN(pst_netbuf),32,"hcc invaild header(payload): ");
                    oal_netbuf_free(pst_netbuf);
                }
            }
            else
            {
                oal_netbuf_free(pst_netbuf);
            }
            hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[type].total_pkts++;
            count++;
        }
        else
        {
            /*simple process, when pre do failed,
            keep the netbuf in list,
            and skip the loop*/
            oal_netbuf_addlist(netbuf_hdr,pst_netbuf);
            break;
        }
    }

    hcc->hcc_transer_info.rx_assem_info.pkt_cnt += count;

    return count;
}

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
oal_void hcc_tx_network_start_subq_etc(struct hcc_handler *hcc, oal_uint16 us_queue_idx)
{

    if(OAL_WARN_ON(!hcc))
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return;
    }

    if(OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.start_subq))
    {

        hcc->hcc_transer_info.tx_flow_ctrl.start_subq(us_queue_idx);
    }
}

oal_void hcc_tx_network_stop_subq_etc(struct hcc_handler *hcc, oal_uint16 us_queue_idx)
{
    if(OAL_WARN_ON(!hcc))
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return;
    }

    if(OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.stop_subq))
    {
        hcc->hcc_transer_info.tx_flow_ctrl.stop_subq(us_queue_idx);
    }
}


oal_void hcc_flowctl_operate_subq_register_etc(hcc_flowctl_start_subq start_subq, hcc_flowctl_stop_subq stop_subq)
{
    if(NULL == hcc_tc)
    {
        OAL_IO_PRINT("[ERROR]hcc_flowctl_operate_subq_register_etc failed! hcc_tc is NULL\n");
        return;
    }
    hcc_tc->hcc_transer_info.tx_flow_ctrl.start_subq = start_subq;
    hcc_tc->hcc_transer_info.tx_flow_ctrl.stop_subq = stop_subq;
    return;
}


oal_void   hcc_host_set_flowctl_param_etc(oal_uint8 uc_queue_type, oal_uint16 us_burst_limit, oal_uint16 us_low_waterline, oal_uint16 us_high_waterline)
{
    if (uc_queue_type >= HCC_QUEUE_COUNT)
    {
        OAL_IO_PRINT("CONFIG_ERROR: hcc_host_set_flowctl_param_etc: uc_queue_type = %d\r\n", uc_queue_type);
        return;
    }
    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].burst_limit = us_burst_limit;
    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.low_waterline = us_low_waterline;
    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.high_waterline = us_high_waterline;

    OAL_IO_PRINT("hcc_host_set_flowctl_param_etc, queue[%d]: burst limit = %d, low_waterline = %d, high_waterline =%d\r\n",
                    uc_queue_type, us_burst_limit, us_low_waterline, us_high_waterline);
    return;
}

oal_void    hcc_host_get_flowctl_param_etc(oal_uint8 uc_queue_type, oal_uint16 *pus_burst_limit, oal_uint16 *pus_low_waterline, oal_uint16 *pus_high_waterline)
{
    if (uc_queue_type >= HCC_QUEUE_COUNT)
    {
        OAL_IO_PRINT("CONFIG_ERROR: hcc_host_get_flowctl_param_etc: uc_queue_type = %d\r\n", uc_queue_type);
        return;
    }

    *pus_burst_limit = hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].burst_limit;
    *pus_low_waterline = hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.low_waterline;
    *pus_high_waterline = hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[uc_queue_type].flow_ctrl.high_waterline;
    return;
}

oal_void   hcc_host_get_flowctl_stat_etc(oal_void)
{
    oal_uint16 us_queue_idx;

    /* 输出各个队列的状态信息 */
    for(us_queue_idx = 0; us_queue_idx < HCC_QUEUE_COUNT; us_queue_idx++)
    {
        OAL_IO_PRINT("Q[%d]:bst_lmt[%d],low_wl[%d],high_wl[%d]\r\n",
                    us_queue_idx,
                    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].burst_limit,
                    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.low_waterline,
                    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.high_waterline);

        OAL_IO_PRINT("      wlan_q[%d],stoped?[%d],q_len[%d],total_pkt[%d],loss_pkt[%d]\r\n",
                    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].wlan_queue_id,
                    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].flow_ctrl.is_stopped,
                    oal_netbuf_list_len(&hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].data_queue),
                    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].total_pkts,
                    hcc_tc->hcc_transer_info.hcc_queues[HCC_TX].queues[us_queue_idx].loss_pkts);
    }

    return;

}

oal_bool_enum_uint8 hcc_flowctl_get_device_mode_etc(struct hcc_handler *hcc)
{
    if(OAL_WARN_ON(!hcc))
    {
        OAL_IO_PRINT("buf is null r failed!%s\n",__FUNCTION__);
        return OAL_FALSE;
    }

    if(OAL_LIKELY(hcc->hcc_transer_info.tx_flow_ctrl.get_mode))
    {
        return hcc->hcc_transer_info.tx_flow_ctrl.get_mode();
    }

    return OAL_FALSE;
}

oal_void hcc_flowctl_get_device_mode_register_etc(hcc_flowctl_get_mode get_mode)
{
    if (OAL_PTR_NULL == hcc_tc)
    {
        OAL_IO_PRINT("[ERROR]hcc_flowctl_get_device_mode_register_etc failed! hcc_tc is NULL\n");
        return;
    }
    hcc_tc->hcc_transer_info.tx_flow_ctrl.get_mode = get_mode;
}

oal_void    hcc_host_update_vi_flowctl_param_etc(oal_uint32 be_cwmin, oal_uint32 vi_cwmin)
{
    oal_uint16  us_burst_limit;
    oal_uint16  us_low_waterline;
    oal_uint16  us_high_waterline;

    /* 如果vi与be的edca参数设置为一致，则更新VI的拥塞控制参数 */
    if (be_cwmin == vi_cwmin)
    {
        hcc_host_get_flowctl_param_etc(DATA_UDP_BE_QUEUE, &us_burst_limit, &us_low_waterline, &us_high_waterline);
        hcc_host_set_flowctl_param_etc(DATA_UDP_VI_QUEUE, us_burst_limit, us_low_waterline, us_high_waterline);
    }
    else //否则设置vi的拥塞控制参数为默认值
    {
        hcc_host_set_flowctl_param_etc(DATA_UDP_VI_QUEUE, 40, 40, 80);
    }
}

/*lint -e19 */
oal_module_symbol(hcc_flowctl_get_device_mode_register_etc);
oal_module_symbol(hcc_flowctl_operate_subq_register_etc);
oal_module_symbol(hcc_host_update_vi_flowctl_param_etc);

/*lint +e19 */

#else


OAL_STATIC  oal_int32 hcc_process_tx_queues(struct hcc_handler *hcc)
{
    oal_int32 i;
    oal_int32 ret = 0;
    //oal_netbuf_head_stru* head;
    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        //head = &hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].data_queue;
        /*send tx queue*/
        ret += hcc_send_tx_queue_etc(hcc, (hcc_queue_type)i);
    }
    return ret;
}


OAL_STATIC  oal_int32 hcc_process_rx_queues(struct hcc_handler *hcc)
{
    oal_int32 i;

    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        hcc_send_rx_queue_etc(hcc,(hcc_queue_type)i);
    }
    return OAL_SUCC;
}

#endif


oal_int32 hcc_thread_process_etc(struct hcc_handler *hcc)
{
    oal_int32 ret = 0;
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    oal_bool_enum_uint8      en_device_is_sta = OAL_FALSE;

    en_device_is_sta = hcc_flowctl_get_device_mode_etc(hcc);
    if (OAL_TRUE == en_device_is_sta)
    {
        /*Tx Tcp Data queue > Rx Tcp Ack
          Rx Tcp Data > Tx Tcp Ack
          Tx Tcp Data queue > Rx Tcp Data queue*/
        ret += hcc_send_rx_queue_etc(hcc, CTRL_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, CTRL_QUEUE);

        ret += hcc_send_rx_queue_etc(hcc, DATA_HI_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_HI_QUEUE);

        /* 下行TCP优先 */
        ret += hcc_send_rx_queue_etc(hcc, DATA_TCP_DATA_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_TCP_ACK_QUEUE);

        ret += hcc_send_tx_queue_etc(hcc, DATA_TCP_DATA_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_TCP_ACK_QUEUE);

        /*Tx Lo < Rx Lo*/
        ret += hcc_send_rx_queue_etc(hcc, DATA_LO_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_LO_QUEUE);

        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_VO_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_VO_QUEUE);

        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_VI_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_VI_QUEUE);

        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_BE_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_BE_QUEUE);

        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_BK_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_BK_QUEUE);
    }
    else
    {
        /*Tx Tcp Data queue > Rx Tcp Ack
          Rx Tcp Data > Tx Tcp Ack
          Tx Tcp Data queue < Rx Tcp Data queue*/
        ret += hcc_send_tx_queue_etc(hcc, CTRL_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, CTRL_QUEUE);

        ret += hcc_send_tx_queue_etc(hcc, DATA_HI_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_HI_QUEUE);

        ret += hcc_send_tx_queue_etc(hcc, DATA_TCP_DATA_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_TCP_ACK_QUEUE);

        ret += hcc_send_rx_queue_etc(hcc, DATA_TCP_DATA_QUEUE);
        ret += hcc_send_tx_queue_etc(hcc, DATA_TCP_ACK_QUEUE);

        /*Tx Lo < Rx Lo*/
        ret += hcc_send_tx_queue_etc(hcc, DATA_LO_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_LO_QUEUE);

        /* udp业务 */
        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_VO_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_VO_QUEUE);

        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_VI_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_VI_QUEUE);

        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_BE_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_BE_QUEUE);

        ret += hcc_send_tx_queue_etc(hcc, DATA_UDP_BK_QUEUE);
        ret += hcc_send_rx_queue_etc(hcc, DATA_UDP_BK_QUEUE);
    }
#else
    hcc_process_rx_queues(hcc);
    hcc_process_tx_queues(hcc);
#endif

    return ret;
}

oal_void hcc_set_tcpack_cnt_etc(oal_uint32 ul_val)
{
    g_ul_tcp_ack_wait_sche_cnt_etc = ul_val;
    return;
}

oal_int32 hcc_transfer_thread_etc(oal_void *data)
{
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    oal_int32 count;
    oal_int32 ret = 0;
    struct hcc_handler* hcc;
    struct sched_param       param;
#ifdef _PRE_WLAN_TCP_OPT
    OAL_STATIC oal_uint8 ack_loop_count = 0;
#endif

    hcc = (struct hcc_handler *)data;

    if(OAL_WARN_ON(NULL == hcc))
    {
         OAL_IO_PRINT("%s error: hcc is null",__FUNCTION__);
         return OAL_FAIL;
    };

    OAL_IO_PRINT("hisi wifi hcc transfer thread enter\n");
    param.sched_priority = 1;
    oal_set_thread_property_etc(current,
                            SCHED_FIFO,
                            &param,
                            HCC_TRANS_THERAD_NICE);


    allow_signal(SIGTERM);

    for (;;)
    {
        count = 0;
        if (OAL_UNLIKELY(kthread_should_stop()))
        {
            OAL_IO_PRINT("hisi wifi hcc transfer thread leave\n");
            break;
        }

        if(OAL_UNLIKELY((NULL == hcc->bus_dev) || (NULL == hcc->bus_dev->cur_bus)))
        {
            OAL_IO_PRINT("bus_dev or cur_bus is null\n");
            msleep(1000);
            continue;
        }

        ret = OAL_WAIT_EVENT_INTERRUPTIBLE(hcc->hcc_transer_info.hcc_transfer_wq,
                                        (OAL_TRUE == hcc_thread_wait_event_cond_check(hcc)));
        if(OAL_UNLIKELY(-ERESTARTSYS == ret))
        {
            OAL_IO_PRINT("wifi task %s was interrupted by a signal\n", oal_get_current_task_name());
            break;
        }
#ifdef _PRE_WLAN_TCP_OPT
        if(OAL_PTR_NULL != hcc->p_hmac_tcp_ack_process_func)
        {
            ack_loop_count++;
            if(ack_loop_count >= g_ul_tcp_ack_wait_sche_cnt_etc)
            {
                ack_loop_count = 0;
                hcc->p_hmac_tcp_ack_process_func();
            }
        }
#endif
        count |= hcc_bus_pending_signal_process(hcc->bus_dev->cur_bus);

        count |= hcc_thread_process_etc(hcc);

#ifdef _PRE_CONFIG_WLAN_THRANS_THREAD_DEBUG
        if(count)
        {
            hcc->hcc_transer_info.thread_stat.loop_have_data_count++;
        }
        else
        {
            hcc->hcc_transer_info.thread_stat.loop_no_data_count++;
        }
#else
        OAL_REFERENCE(count);
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
        if(!count)
        {
            /*空转*/
            cpu_relax();
        }
#endif
    }
#else
    OAL_IO_PRINT("hisi wifi hcc transfer thread done!%p\n", data);
#endif
    OAL_IO_PRINT("hisi wifi hcc transfer thread done!\n");
    return OAL_SUCC;
}
/*lint -e19*/
oal_module_symbol(hcc_thread_process_etc);
oal_module_symbol(hcc_transfer_thread_etc);
oal_module_symbol(g_ul_tcp_ack_wait_sche_cnt_etc);
/*lint +e19*/

OAL_STATIC  oal_void hcc_dev_flowctr_timer_del(struct hcc_handler *hcc)
{
    if(oal_in_interrupt())
    {
        oal_timer_delete(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer);
    }
    else
    {
        oal_timer_delete_sync(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer);
    }
}

oal_void hcc_dev_flowctrl_on_etc(struct hcc_handler *hcc, oal_uint8 need_notify_dev)
{
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_on_count++;
    oal_print_hi11xx_log(HI11XX_LOG_DBG, "start tranferring to device");

    if(D2H_MSG_FLOWCTRL_OFF == hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag)
    {
        hcc_dev_flowctr_timer_del(hcc);
        hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag = D2H_MSG_FLOWCTRL_ON;
        hcc_sched_transfer(hcc);
    }

    if(need_notify_dev)
    {
        oal_print_hi11xx_log(HI11XX_LOG_INFO, "Host turn on dev flow ctrl...");
        hcc_bus_send_message(HCC_TO_BUS(hcc), H2D_MSG_FLOWCTRL_ON);
    }

}

oal_void hcc_dev_flowctrl_off_etc(struct hcc_handler *hcc)
{
    if(D2H_MSG_FLOWCTRL_ON == hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag)
    {
        oal_timer_start(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer,
                        hcc->hcc_transer_info.tx_flow_ctrl.timeout);
    }

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag = D2H_MSG_FLOWCTRL_OFF;
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_off_count++;
    oal_print_hi11xx_log(HI11XX_LOG_DBG, "stop tranferring to device");
}

oal_void hcc_transfer_queues_init_etc(struct hcc_handler *hcc)
{
    oal_int32 i,j;
    for(i = 0; i < HCC_DIR_COUNT; i++)
    {
        for(j = 0; j < HCC_QUEUE_COUNT; j++)
        {
            oal_netbuf_head_init(&hcc->hcc_transer_info.hcc_queues[i].queues[j].data_queue);
        }
    }
}

oal_int32 hcc_tx_assem_descr_init_etc(struct hcc_handler *hcc)
{
    oal_int32 i;
    oal_int32 ret = OAL_SUCC;
    oal_netbuf_stru * netbuf = NULL;

    oal_netbuf_head_init(&hcc->tx_descr_info.tx_assem_descr_hdr);

    /*assem descr ping-pong buff, 2 should be ok*/
    hcc->tx_descr_info.descr_num = 2;

    for(i = 0; i < hcc->tx_descr_info.descr_num; i++)
    {
        netbuf = oal_netbuf_alloc(HISDIO_HOST2DEV_SCATT_SIZE, 0, 0);
        if(NULL == netbuf)
        {
            /*lint -e801*/
            goto failed_netbuf_alloc;
        }

        oal_netbuf_put(netbuf, HISDIO_HOST2DEV_SCATT_SIZE);
        oal_memset(OAL_NETBUF_DATA(netbuf),0,OAL_NETBUF_LEN(netbuf));
        oal_netbuf_list_tail(&hcc->tx_descr_info.tx_assem_descr_hdr, netbuf);
        if (OAL_WARN_ON(!OAL_IS_ALIGNED(((oal_uint)OAL_NETBUF_DATA(netbuf)),4)))
        {
                OAM_WARNING_LOG1(0, OAM_SF_ANY, "{(oal_uint)OAL_NETBUF_DATA(netbuf):%d not align 4}",(oal_uint)OAL_NETBUF_DATA(netbuf));
        }
    }

    OAL_BUILD_BUG_ON(HISDIO_HOST2DEV_SCATT_SIZE < 4);

    return ret;
failed_netbuf_alloc:
    oal_netbuf_list_purge(&hcc->tx_descr_info.tx_assem_descr_hdr);
    return -OAL_ENOMEM;

}

oal_void hcc_tx_assem_descr_exit_etc(struct hcc_handler *hcc)
{
    oal_netbuf_list_purge(&hcc->tx_descr_info.tx_assem_descr_hdr);
}

oal_void hcc_tx_assem_info_reset_etc(struct hcc_handler *hcc)
{
    oal_memset(hcc->hcc_transer_info.tx_assem_info.info, 0, OAL_SIZEOF(hcc->hcc_transer_info.tx_assem_info.info));
}

oal_void oal_sdio_rx_assem_info_reset(struct hcc_handler *hcc)
{
    oal_memset(hcc->hcc_transer_info.rx_assem_info.info, 0, OAL_SIZEOF(hcc->hcc_transer_info.rx_assem_info.info));
}

oal_void hcc_assem_info_init_etc(struct hcc_handler *hcc)
{
    hcc->hcc_transer_info.tx_assem_info.assemble_max_count = hcc_assemble_count_etc;
    hcc_tx_assem_info_reset_etc(hcc);
    oal_sdio_rx_assem_info_reset(hcc);
    oal_netbuf_list_head_init(&hcc->hcc_transer_info.tx_assem_info.assembled_head);
}

oal_void hcc_trans_limit_parm_init_etc(struct hcc_handler  *hcc)
{
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
    oal_int32 i;
    hcc_trans_queues *pst_hcc_tx_queues = &hcc->hcc_transer_info.hcc_queues[HCC_TX];

    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].burst_limit = (oal_uint32)HCC_FLUSH_ALL;
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].burst_limit = (oal_uint32)HCC_FLUSH_ALL;
    }

    pst_hcc_tx_queues->queues[CTRL_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_HI_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_LO_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_TCP_DATA_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_TCP_ACK_QUEUE].burst_limit = 256;
    pst_hcc_tx_queues->queues[DATA_UDP_BK_QUEUE].burst_limit = 10;
    pst_hcc_tx_queues->queues[DATA_UDP_BE_QUEUE].burst_limit = 20;
    pst_hcc_tx_queues->queues[DATA_UDP_VI_QUEUE].burst_limit = 40;
    pst_hcc_tx_queues->queues[DATA_UDP_VO_QUEUE].burst_limit = 60;
#endif

}

oal_void hcc_trans_send_mode_init_etc(struct hcc_handler  *hcc)
{
    oal_int32 i;

    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].send_mode = hcc_assem_send;
    }
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].send_mode = hcc_single_send;
}

OAL_STATIC OAL_VOLATILE oal_int32 g_flowctrl_info_flag = 0;
OAL_STATIC oal_void hcc_dev_flow_ctrl_timeout_isr(oal_uint arg)
{
    struct hcc_handler *hcc = (struct hcc_handler *)arg;
    if(NULL == hcc)
    {
        OAL_IO_PRINT("hcc is null\n");
        return;
    }
    /*flowctrl lock too much time.*/
    DECLARE_DFT_TRACE_KEY_INFO("hcc_flow_lock_timeout",OAL_DFT_TRACE_EXCEP);
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{SDIO flow ctrl had off for %u ms, it's a long time}",
                    (oal_uint32)hcc->hcc_transer_info.tx_flow_ctrl.timeout);
    g_flowctrl_info_flag = 0;

    /*If work is idle,queue a new work.*/
    if(0 == oal_work_is_busy(&hcc->hcc_transer_info.tx_flow_ctrl.worker.work))
    {
        oal_queue_delayed_system_work(&hcc->hcc_transer_info.tx_flow_ctrl.worker,0);
    }
}

oal_void hcc_flowctrl_deadlock_detect_worker_etc(oal_work_stru *pst_flow_work)
{
    struct hcc_handler  *hcc = hcc_get_110x_handler();
    if(NULL == hcc)
    {
        OAL_IO_PRINT("hcc_flowctrl_deadlock_detect_worker_etc hcc is null\n");
        return;
    }
    OAM_WARNING_LOG1(0, OAM_SF_ANY, "{hcc_flowctrl_deadlock_detect_worker_etc action,%d}",
                    g_flowctrl_info_flag);
    if( 0 == g_flowctrl_info_flag)
    {
        g_flowctrl_info_flag = 1 ;
        OAL_SMP_MB();
        //oal_sdio_send_msg_etc(hcc->hi_sdio, H2D_MSG_DEVICE_INFO_DUMP);
        /*queue system_wq delay work,and send other message 20ms later.*/
        /* print device mem */
        hcc_print_device_mem_info_etc();
        oal_queue_delayed_system_work(&hcc->hcc_transer_info.tx_flow_ctrl.worker,OAL_MSECS_TO_JIFFIES(20));
    }
    else if( 1 == g_flowctrl_info_flag)
    {
        //oal_sdio_send_msg_etc(hcc->hi_sdio, H2D_MSG_DEVICE_MEM_DUMP);
        /* print device mem */
        if(hcc_flowctrl_detect_panic_etc)
        {
            hcc_trigger_device_panic_etc();
        }
        else
        {
            hcc_print_device_mem_info_etc();
        }
    }

    return;
}

oal_void hcc_trans_flow_ctrl_info_reset(struct hcc_handler  *hcc)
{
    /*disable flow ctrl detect timer*/
    hcc_dev_flowctr_timer_del(hcc);
    oal_cancel_delayed_work_sync(&hcc->hcc_transer_info.tx_flow_ctrl.worker);

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag = D2H_MSG_FLOWCTRL_OFF;
    hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt = 0;/*默认不允许发送等待wcpu更新credit*/
}

oal_void hcc_trans_flow_ctrl_info_init_etc(struct hcc_handler  *hcc)
{
    oal_int32 i;
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_flag = D2H_MSG_FLOWCTRL_ON;
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_off_count = 0;
    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_on_count = 0;
    oal_spin_lock_init(&hcc->hcc_transer_info.tx_flow_ctrl.lock);
    hcc->hcc_transer_info.tx_flow_ctrl.timeout = 20*1000;
    OAL_INIT_DELAYED_WORK(&hcc->hcc_transer_info.tx_flow_ctrl.worker,hcc_flowctrl_deadlock_detect_worker_etc);
    oal_timer_init(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer,
                    hcc->hcc_transer_info.tx_flow_ctrl.timeout,
                    hcc_dev_flow_ctrl_timeout_isr,
                    (oal_uint)hcc);

    hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt = HCC_FLOW_HIGH_PRI_BUFF_CNT;
    oal_spin_lock_init(&hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock);

    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.enable = OAL_TRUE;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.flow_type = HCC_FLOWCTRL_SDIO;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.is_stopped = OAL_FALSE;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.low_waterline= 512;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.high_waterline= 1024;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].netbuf_pool_type = HCC_NETBUF_NORMAL_QUEUE;
    }

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.flow_type = HCC_FLOWCTRL_CREDIT;

    for(i = 0; i < HCC_QUEUE_COUNT; i++)
    {
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].flow_ctrl.enable = OAL_TRUE;
        hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[i].flow_ctrl.is_stopped = OAL_FALSE;
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].flow_ctrl.low_waterline= 128;
        hcc->hcc_transer_info.hcc_queues[HCC_RX].queues[i].flow_ctrl.high_waterline= 512;
    }

    /*DEVICE 没有给高优先级预留内存，所有队列都需要流控。*/
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.enable = OAL_FALSE;
#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[CTRL_QUEUE].flow_ctrl.low_waterline= 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[CTRL_QUEUE].flow_ctrl.high_waterline= 256;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[CTRL_QUEUE].netbuf_pool_type = HCC_NETBUF_HIGH_QUEUE;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.low_waterline= 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].flow_ctrl.high_waterline= 256;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_HI_QUEUE].netbuf_pool_type = HCC_NETBUF_HIGH_QUEUE;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_LO_QUEUE].flow_ctrl.low_waterline= 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_LO_QUEUE].flow_ctrl.high_waterline= 256;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_DATA_QUEUE].flow_ctrl.low_waterline= 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_DATA_QUEUE].flow_ctrl.high_waterline= 256;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_ACK_QUEUE].flow_ctrl.low_waterline= 128;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_TCP_ACK_QUEUE].flow_ctrl.high_waterline= 256;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BK_QUEUE].flow_ctrl.low_waterline= 10;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BK_QUEUE].flow_ctrl.high_waterline= 20;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BE_QUEUE].flow_ctrl.low_waterline= 20;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_BE_QUEUE].flow_ctrl.high_waterline= 40;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VI_QUEUE].flow_ctrl.low_waterline= 40;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VI_QUEUE].flow_ctrl.high_waterline= 80;

    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VO_QUEUE].flow_ctrl.low_waterline= 60;
    hcc->hcc_transer_info.hcc_queues[HCC_TX].queues[DATA_UDP_VO_QUEUE].flow_ctrl.high_waterline= 120;
#endif

}
oal_int32 hcc_flow_on_callback_etc(oal_void* data)
{
    hcc_dev_flowctrl_on_etc((struct hcc_handler *)data, 0);
    return OAL_SUCC;
}

oal_int32 hcc_flow_off_callback_etc(oal_void* data)
{
    hcc_dev_flowctrl_off_etc((struct hcc_handler *)data);
    return OAL_SUCC;
}


oal_int32  hcc_credit_update_callback_etc(oal_void* data)
{
    oal_uint8           uc_large_cnt;
    struct hcc_handler *hcc     = (struct hcc_handler *)data;
    struct oal_sdio    *hi_sdio = oal_get_sdio_default_handler();

    if(OAL_WARN_ON(!hi_sdio))
    {
        OAL_IO_PRINT("hcc_credit_update_callback_etc set fail: hi_sdio is null!\n");
        return OAL_FAIL;
    }

    uc_large_cnt = HISDIO_LARGE_PKT_GET(hi_sdio->sdio_extend->credit_info);

    oal_spin_lock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));

    hcc->hcc_transer_info.tx_flow_ctrl.uc_hipriority_cnt = uc_large_cnt;

    oal_spin_unlock(&(hcc->hcc_transer_info.tx_flow_ctrl.st_hipri_lock));

    hcc->hcc_transer_info.tx_flow_ctrl.flowctrl_hipri_update_count++;

    hcc_sched_transfer(hcc);

    return OAL_SUCC;
}

oal_int32  hcc_high_pkts_loss_callback_etc(oal_void* data)
{
    OAL_REFERENCE(data);
    DECLARE_DFT_TRACE_KEY_INFO("sdio_high_pkts_loss", OAL_DFT_TRACE_EXCEP);
    return OAL_SUCC;
}


struct hcc_handler* hcc_module_init_etc(hcc_bus_dev* pst_bus_dev)
{
    oal_uint32 ul_tx_max_len;
    struct hcc_handler* hcc = NULL;

    OAL_BUILD_BUG_ON(HCC_HDR_LEN > HCC_HDR_TOTAL_LEN);

    /*main_type:4 只能表示16种类型*/
    OAL_BUILD_BUG_ON(HCC_ACTION_TYPE_BUTT > 15);

    if(OAL_WARN_ON(NULL == pst_bus_dev))
    {
         oal_print_hi11xx_log(HI11XX_LOG_ERR,"%s error: pst_bus_dev is null",__FUNCTION__);
         return NULL;
    }

    /*1544-the max netbuf len of device*/
    /*lint -e778*/
    g_hcc_tx_max_buf_len = OAL_ROUND_DOWN(HSDIO_HOST2DEV_PKTS_MAX_LEN,HISDIO_H2D_SCATT_BUFFLEN_ALIGN);
    /*one byte,2^8=256*/
    ul_tx_max_len = (1<<(8+HISDIO_H2D_SCATT_BUFFLEN_ALIGN_BITS));
    g_hcc_tx_max_buf_len = OAL_MIN(g_hcc_tx_max_buf_len,ul_tx_max_len);
    /*lint +e778*/

    hcc = (struct hcc_handler*)oal_memalloc(OAL_SIZEOF(struct hcc_handler));
    if(!hcc)
    {
        OAL_IO_PRINT("hcc mem alloc  failed!\n");
        return NULL;
    }

    oal_memset((oal_void*)hcc, 0, OAL_SIZEOF(struct hcc_handler));

    hcc->bus_dev = pst_bus_dev;

    hcc->hdr_rever_max_len = HCC_HDR_RESERVED_MAX_LEN;

    /*disable hcc default*/
    oal_atomic_set(&hcc->state, HCC_OFF);

    OAL_WAIT_QUEUE_INIT_HEAD(&hcc->hcc_transer_info.hcc_transfer_wq);    /*queues init*/
    hcc_transfer_queues_init_etc(hcc);

    hcc_trans_flow_ctrl_info_init_etc(hcc);

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mutex_init(&hcc->tx_transfer_lock);
#endif

    OAL_WAIT_QUEUE_INIT_HEAD(&hcc->hcc_transer_info.tx_flow_ctrl.wait_queue);

#if !(defined(_PRE_WLAN_TCP_OPT) || defined(WIN32))
    hcc->hcc_transer_info.hcc_transfer_thread_etc = oal_thread_create_etc(hcc_transfer_thread_etc,
                                        hcc,
                                        NULL,
                                        "hisi_hcc",
                                        HCC_TRANS_THREAD_POLICY,
                                        HCC_TRANS_THERAD_PRIORITY,
                                        -1);
    if(!hcc->hcc_transer_info.hcc_transfer_thread_etc)
    {
		OAL_IO_PRINT("hcc thread create failed!\n");
        goto failed_create_hcc_thread;
    }
#endif

#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_SUCC !=hcc_transfer_rx_register(hcc,(oal_void*)hcc, hcc_bus_rx_handler))
    {
		OAL_IO_PRINT("sdio rx transfer callback register failed!\n");
        goto failed_rx_cb_reg;
    }
#endif

    hcc_assem_info_init_etc(hcc);
    hcc_trans_limit_parm_init_etc(hcc);
    hcc_trans_send_mode_init_etc(hcc);

    OAL_BUILD_BUG_ON(!OAL_IS_ALIGNED(HISDIO_HOST2DEV_SCATT_SIZE, HISDIO_H2D_SCATT_BUFFLEN_ALIGN));

    if(OAL_SUCC != hcc_tx_assem_descr_init_etc(hcc))
    {
        OAL_IO_PRINT("hcc tx assem descrt alloc failed!\n");
        goto failed_tx_assem_descr_alloc;
    }
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    if(OAL_SUCC != hcc_message_register_etc(hcc, D2H_MSG_FLOWCTRL_ON,hcc_flow_on_callback_etc,hcc))
    {
        OAL_IO_PRINT("register flow ctrl on failed!\n");
        goto failed_reg_flowon_msg;
    }

    if(OAL_SUCC != hcc_message_register_etc(hcc, D2H_MSG_FLOWCTRL_OFF,hcc_flow_off_callback_etc,hcc))
    {
        OAL_IO_PRINT("register flow ctrl off failed!\n");
        goto failed_reg_flowoff_msg;
    }

    hcc_message_register_etc(hcc, D2H_MSG_CREDIT_UPDATE, hcc_credit_update_callback_etc, hcc);

    hcc_message_register_etc(hcc, D2H_MSG_HIGH_PKT_LOSS, hcc_high_pkts_loss_callback_etc, hcc);
#endif
    if(OAL_SUCC != hcc_test_init_module_etc(hcc))
    {
        OAL_IO_PRINT("register flow ctrl off failed!\n");
        goto failed_hcc_test_init;
    }

    oal_wake_lock_init(&hcc->tx_wake_lock,"hcc_tx_etc");

    OAL_IO_PRINT("hcc_module_init_etc dev id:%d succ\n", pst_bus_dev->dev_id);

    /*hcc_tc is used to test!*/
    hcc_tc = hcc;

    return hcc;
failed_hcc_test_init:
    hcc_message_unregister_etc(hcc,D2H_MSG_FLOWCTRL_OFF);
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
failed_reg_flowoff_msg:
    hcc_message_unregister_etc(hcc,D2H_MSG_FLOWCTRL_ON);
failed_reg_flowon_msg:
#endif
    hcc_tx_assem_descr_exit_etc(hcc);
failed_tx_assem_descr_alloc:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    hcc_bus_transfer_rx_unregister(HCC_TO_BUS(hcc));
failed_rx_cb_reg:
#endif
#if !defined(_PRE_WLAN_TCP_OPT) || defined(WIN32)
    oal_thread_stop_etc(hcc->hcc_transer_info.hcc_transfer_thread_etc, NULL);
    hcc->hcc_transer_info.hcc_transfer_thread_etc = NULL;
#endif
#if !(defined(_PRE_WLAN_TCP_OPT) || defined(WIN32))
failed_create_hcc_thread:
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mutex_destroy(&hcc->tx_transfer_lock);
#endif
#endif
    oal_free(hcc);
    /*lint +e801*/

    return NULL;
}


oal_void hcc_module_exit_etc(struct hcc_handler* hcc)
{
    oal_wake_lock_exit(&hcc->tx_wake_lock);
    hcc_test_exit_module_etc(hcc);
    hcc_message_unregister_etc(hcc,D2H_MSG_FLOWCTRL_OFF);
    hcc_message_unregister_etc(hcc,D2H_MSG_FLOWCTRL_ON);
    hcc_tx_assem_descr_exit_etc(hcc);
    //hcc_rx_unregister_etc(hcc);
    hcc_bus_transfer_rx_unregister(HCC_TO_BUS(hcc));
#if !defined(_PRE_WLAN_TCP_OPT) || defined(WIN32)
    oal_thread_stop_etc(hcc->hcc_transer_info.hcc_transfer_thread_etc, NULL);
    hcc->hcc_transer_info.hcc_transfer_thread_etc = NULL;
#endif
#if (_PRE_OS_VERSION_LINUX == _PRE_OS_VERSION)
    mutex_destroy(&hcc->tx_transfer_lock);
#endif

    oal_timer_delete_sync(&hcc->hcc_transer_info.tx_flow_ctrl.flow_timer);
    oal_cancel_delayed_work_sync(&hcc->hcc_transer_info.tx_flow_ctrl.worker);

    oal_free(hcc);
    hcc_tc = NULL;
}

#ifdef CONFIG_PRINTK
oal_void hcc_sched_transfer_test_etc(oal_void)
{
    hcc_sched_transfer(hcc_tc);
}

oal_void hcc_dev_flowctrl_on_test_etc(oal_void)
{
    if(NULL != hcc_tc)
        hcc_dev_flowctrl_on_etc(hcc_tc,1);
}
#endif
#ifdef CONFIG_MMC
oal_void hcc_device_info_dump_etc(oal_void)
{
    struct hcc_handler* hcc = hcc_get_110x_handler();
    OAL_IO_PRINT("hcc_device_info_dump_etc\n");
    hcc_bus_send_message(HCC_TO_BUS(hcc), H2D_MSG_DEVICE_INFO_DUMP);
}

oal_void hcc_device_mem_dump_etc(oal_void)
{
    struct hcc_handler* hcc = hcc_get_110x_handler();
    OAL_IO_PRINT("hcc_device_mem_dump_etc\n");
    hcc_bus_send_message(HCC_TO_BUS(hcc), H2D_MSG_DEVICE_MEM_DUMP);
}
#endif
/*lint -e19 */

oal_void hcc_trigger_device_panic_etc(oal_void)
{
#ifdef CONFIG_MMC
    struct hcc_handler* hcc = hcc_get_110x_handler();
    wlan_pm_disable_etc();
    hcc_bus_send_message(HCC_TO_BUS(hcc), H2D_MSG_TEST);
    wlan_pm_enable_etc();
#endif
}
oal_module_symbol(hcc_trigger_device_panic_etc);

oal_void hcc_print_device_mem_info_etc(oal_void)
{
#ifdef CONFIG_MMC
    struct hcc_handler* hcc = hcc_get_110x_handler();

    wlan_pm_disable_etc();
    hcc_bus_send_message(HCC_TO_BUS(hcc), H2D_MSG_DEVICE_MEM_INFO);
    wlan_pm_enable_etc();
#endif
}
oal_module_symbol(hcc_print_device_mem_info_etc);

#ifdef _PRE_WLAN_FEATURE_OFFLOAD_FLOWCTL
oal_module_symbol(hcc_host_set_flowctl_param_etc);
oal_module_symbol(hcc_host_get_flowctl_param_etc);
oal_module_symbol(hcc_host_get_flowctl_stat_etc);
#endif



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif
