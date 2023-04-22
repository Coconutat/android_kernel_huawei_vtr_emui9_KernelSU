/*
* Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
* foss@huawei.com
*
* If distributed as part of the Linux kernel, the following license terms
* apply:
*
* * This program is free software; you can redistribute it and/or modify
* * it under the terms of the GNU General Public License version 2 and
* * only version 2 as published by the Free Software Foundation.
* *
* * This program is distributed in the hope that it will be useful,
* * but WITHOUT ANY WARRANTY; without even the implied warranty of
* * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* * GNU General Public License for more details.
* *
* * You should have received a copy of the GNU General Public License
* * along with this program; if not, write to the Free Software
* * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*
* Otherwise, the following license terms apply:
*
* * Redistribution and use in source and binary forms, with or without
* * modification, are permitted provided that the following conditions
* * are met:
* * 1) Redistributions of source code must retain the above copyright
* *    notice, this list of conditions and the following disclaimer.
* * 2) Redistributions in binary form must reproduce the above copyright
* *    notice, this list of conditions and the following disclaimer in the
* *    documentation and/or other materials provided with the distribution.
* * 3) Neither the name of Huawei nor the names of its contributors may
* *    be used to endorse or promote products derived from this software
* *    without specific prior written permission.
*
* * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/

/******************************************************************************
   1 头文件包含
******************************************************************************/
#include "v_typdef.h"
#include "ImmInterface.h"
#include "RnicProcMsg.h"
#include "RnicLog.h"
#include "RnicEntity.h"
#include "RnicDebug.h"
#include "RnicCtx.h"
#include "RnicConfigInterface.h"
#include "product_config.h"
#include "mdrv.h"
#include "PppRnicInterface.h"
#include "RnicSndMsg.h"
#include "BastetRnicInterface.h"


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/

#define THIS_FILE_ID PS_FILE_ID_RNIC_ENTITY_C

/******************************************************************************
   2 外部函数变量声明
******************************************************************************/

/******************************************************************************
   3 私有定义
******************************************************************************/

/******************************************************************************
   4 全局变量定义
*****************************************************************************/

/******************************************************************************
   5 函数实现
******************************************************************************/


VOS_VOID RNIC_SendULDataInPdpActive(
    IMM_ZC_STRU                        *pstImmZc,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    VOS_UINT8                           ucRabId,
    ADS_PKT_TYPE_ENUM_UINT8             enIpType
)
{
    RNIC_NETCARD_DEV_INFO_STRU         *pstPriv   = VOS_NULL_PTR;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;
    VOS_UINT32                          ulDataLen = 0;
    VOS_UINT8                           ucSendAdsRabId;



    pstPriv   = pstNetCntxt->pstPriv;
    enRmNetId = pstNetCntxt->enRmNetId;

    /* 根据Modem Id组装RabId */
    if (MODEM_ID_0 == pstNetCntxt->enModemId)
    {
        ucSendAdsRabId = ucRabId;
    }
    else if (MODEM_ID_1 == pstNetCntxt->enModemId)
    {
        ucSendAdsRabId = ucRabId | RNIC_RABID_TAKE_MODEM_1_MASK;
    }
    else if (MODEM_ID_2 == pstNetCntxt->enModemId)
    {
        ucSendAdsRabId = ucRabId | RNIC_RABID_TAKE_MODEM_2_MASK;
    }
    else
    {
        IMM_ZcFreeAny(pstImmZc);
        RNIC_DBG_MODEM_ID_UL_DISCARD_NUM(1, enRmNetId);
        pstPriv->stStats.tx_dropped++;
        return;
    }

    ulDataLen = pstImmZc->len;

    if (VOS_OK != ADS_UL_SendPacketEx(pstImmZc, enIpType, ucSendAdsRabId))
    {
        IMM_ZcFreeAny(pstImmZc);
        RNIC_DBG_SEND_UL_PKT_FAIL_NUM(1, enRmNetId);
        pstPriv->stStats.tx_dropped++;
        return;
    }

   RNIC_DBG_SEND_UL_PKT_NUM(1, enRmNetId);

    /* 统计网卡发送信息 */
    pstPriv->stStats.tx_packets++;
    pstPriv->stStats.tx_bytes += ulDataLen;

    /* 统计上行数据 */
    pstNetCntxt->stDsFlowStats.ulPeriodSendPktNum++;
    pstNetCntxt->stDsFlowStats.ulTotalSendFluxLow += ulDataLen;

    return;
}


VOS_VOID RNIC_ProcVoWifiULData(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
)
{

    RNIC_NETCARD_DEV_INFO_STRU         *pstPriv = VOS_NULL_PTR;

    /* 当IMS域为WIFI时，RMNET_IMS网卡出口的数据通过RNIC和CDS之间的核间消息传递 */
    if (VOS_OK == RNIC_SendCdsImsDataReq(pstSkb, pstNetCntxt))
    {
        RNIC_DBG_SEND_UL_PKT_NUM(1, pstNetCntxt->enRmNetId);

        /* 统计网卡发送信息 */
        pstPriv   = pstNetCntxt->pstPriv;

        pstPriv->stStats.tx_packets++;
        pstPriv->stStats.tx_bytes += pstSkb->len;

        /* 统计上行数据 */
        pstNetCntxt->stDsFlowStats.ulPeriodSendPktNum++;
        pstNetCntxt->stDsFlowStats.ulTotalSendFluxLow += pstSkb->len;
    }

    IMM_ZcFreeAny(pstSkb);

    return;
}


VOS_VOID RNIC_SendULIpv4Data(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
)
{
    IMM_ZC_STRU                        *pstImmZc = VOS_NULL_PTR;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;
    VOS_UINT8                           ucRabId;
    VOS_UINT32                          ulNonEmpty = VOS_FALSE;

    pstImmZc  = (IMM_ZC_STRU *)pstSkb;
    enRmNetId = pstNetCntxt->enRmNetId;

    /* 当IMS域为WIFI时，RMNET_IMS网卡出口的数据通过RNIC和CDS之间的核间消息传递 */
    if (RNIC_RMNET_R_IS_VALID(pstNetCntxt->enRmNetId))
    {
        if (0 == IMM_ZcQueueLen(&(pstNetCntxt->stPdpCtx.stImsQue)))
        {
            ulNonEmpty = VOS_TRUE;
        }

        IMM_ZcQueueTail(&(pstNetCntxt->stPdpCtx.stImsQue), pstSkb);

        if (VOS_TRUE == ulNonEmpty)
        {
            RNIC_TrigImsDataProcEvent(pstNetCntxt->enRmNetId);
        }

        return;
    }

    /* 获取网卡映射的RABID */
    ucRabId = RNIC_GET_SPEC_NET_IPV4_RABID(enRmNetId);
    if (RNIC_RAB_ID_INVALID == ucRabId)
    {
        IMM_ZcFreeAny((IMM_ZC_STRU *)pstSkb);
        RNIC_DBG_RAB_ID_ERR_NUM(1, enRmNetId);
        return;
    }

    /* PDP激活的情况下数据的处理 */
    RNIC_SendULDataInPdpActive(pstImmZc, pstNetCntxt, ucRabId, ADS_PKT_TYPE_IPV4);

    return;
}


VOS_VOID RNIC_SendULIpv6Data(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
)
{
    IMM_ZC_STRU                        *pstImmZc = VOS_NULL_PTR;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;
    VOS_UINT8                           ucRabId;
    VOS_UINT32                          ulNonEmpty = VOS_FALSE;

    pstImmZc  = (IMM_ZC_STRU *)pstSkb;
    enRmNetId = pstNetCntxt->enRmNetId;

    /* 当IMS域为WIFI时，RMNET_IMS网卡出口的数据通过RNIC和CDS之间的核间消息传递 */
    if (RNIC_RMNET_R_IS_VALID(pstNetCntxt->enRmNetId))
    {
        if (0 == IMM_ZcQueueLen(&(pstNetCntxt->stPdpCtx.stImsQue)))
        {
            ulNonEmpty = VOS_TRUE;
        }

        IMM_ZcQueueTail(&(pstNetCntxt->stPdpCtx.stImsQue), pstSkb);

        if (VOS_TRUE == ulNonEmpty)
        {
            RNIC_TrigImsDataProcEvent(enRmNetId);
        }

        return;
    }

    /* 获取网卡映射的RABID */
    ucRabId = RNIC_GET_SPEC_NET_IPV6_RABID(enRmNetId);
    if (RNIC_RAB_ID_INVALID == ucRabId)
    {
        IMM_ZcFreeAny((IMM_ZC_STRU *)pstSkb);
        RNIC_DBG_RAB_ID_ERR_NUM(1, enRmNetId);
        return;
    }

    /* PDP激活的情况下数据的处理 */
    RNIC_SendULDataInPdpActive(pstImmZc, pstNetCntxt, ucRabId, ADS_PKT_TYPE_IPV6);

    return;
}


VOS_UINT32 RNIC_ProcDemDial(
    struct sk_buff                     *pstSkb
)
{
    RNIC_DIAL_MODE_STRU                *pstDialMode;
    RNIC_TIMER_STATUS_ENUM_UINT8        enTiStatus;
    VOS_UINT32                          ulIpAddr;

    /* 获取IP地址 */
    ulIpAddr = *((VOS_UINT32 *)((pstSkb->data) + RNIC_IP_HEAD_DEST_ADDR_OFFSET));

    /*如果是广播包，则不发起按需拨号，直接过滤调*/
    if (RNIC_IPV4_BROADCAST_ADDR == ulIpAddr)
    {
        RNIC_DBG_UL_RECV_IPV4_BROADCAST_NUM(1, RNIC_RMNET_ID_0);
        return VOS_ERR;
    }

    /* 获取按需拨号的模式以及时长的地址 */
    pstDialMode = RNIC_GetDialModeAddr();

    /* 获取当前拨号保护定时器的状态 */
    enTiStatus  = RNIC_GetTimerStatus(TI_RNIC_DEMAND_DIAL_PROTECT);

    /*为了防止按需拨号上报太快，启动一个两秒定时器，*/
    if (RNIC_TIMER_STATUS_STOP == enTiStatus)
    {
        /* 通知应用进行拨号操作 */
        if (RNIC_ALLOW_EVENT_REPORT == pstDialMode->enEventReportFlag)
        {
            if (VOS_OK == RNIC_SendDialEvent(DEVICE_ID_WAN, RNIC_DAIL_EVENT_UP))
            {
                /* 启动拨号保护定时器  */
                RNIC_StartTimer(TI_RNIC_DEMAND_DIAL_PROTECT, TI_RNIC_DEMAND_DIAL_PROTECT_LEN);
                RNIC_DBG_SEND_APP_DIALUP_SUCC_NUM(1, RNIC_RMNET_ID_0);
                RNIC_NORMAL_LOG(ACPU_PID_RNIC, "RNIC_ProcDemDial: Send dial event succ.");
            }
            else
            {
                RNIC_DBG_SEND_APP_DIALUP_FAIL_NUM(1, RNIC_RMNET_ID_0);
                RNIC_WARNING_LOG(ACPU_PID_RNIC, "RNIC_ProcDemDial: Send dial event fail.");
            }

            RNIC_MNTN_TraceDialConnEvt();
        }
    }

    return VOS_OK;
}

/* Modified by l60609 for L-C互操作项目, 2014-1-14, begin */

VOS_VOID RNIC_RcvInsideModemUlData(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
)
{
    RNIC_NETCARD_DEV_INFO_STRU         *pstPriv = VOS_NULL_PTR;
    VOS_UINT16                          usEthType;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;

    pstPriv     = pstNetCntxt->pstPriv;
    enRmNetId   = pstNetCntxt->enRmNetId;

    /* 获取以太帧类型 */
    usEthType   = VOS_NTOHS(((RNIC_ETH_HEADER_STRU *)(pstSkb->data))->usEtherType);

    /* 流控检查 */
    if (RNIC_FLOW_CTRL_STATUS_START == RNIC_GET_FLOW_CTRL_STATUS(enRmNetId))
    {
        IMM_ZcFreeAny(pstSkb);
        RNIC_DBG_FLOW_CTRL_UL_DISCARD_NUM(1, enRmNetId);
        return;
    }

    /* 移除MAC头 */
    if (VOS_OK != IMM_ZcRemoveMacHead(pstSkb))
    {
        IMM_ZcFreeAny(pstSkb);
        RNIC_DBG_UL_RMV_MAC_HDR_FAIL_NUM(1, enRmNetId);
        pstPriv->stStats.tx_dropped++;
        return;
    }

    /* 只在网卡0上面才会触发按需拨号 */
    if ((RNIC_ETH_TYPE_IP == usEthType)
     && (RNIC_DIAL_MODE_DEMAND_DISCONNECT == RNIC_GET_DIAL_MODE())
     && (RNIC_PDP_REG_STATUS_DEACTIVE == RNIC_GET_SPEC_NET_IPV4_REG_STATE(enRmNetId))
     && (RNIC_RMNET_ID_0 == enRmNetId))
    {
        RNIC_SPE_MEM_UNMAP(pstSkb, RNIC_SPE_CACHE_HDR_SIZE);

        if (VOS_ERR == RNIC_ProcDemDial(pstSkb))
        {
            RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_StartXmit, the data is discared!");
        }

        RNIC_SPE_MEM_MAP(pstSkb, RNIC_SPE_CACHE_HDR_SIZE);

        IMM_ZcFreeAny(pstSkb);
        return;
    }

    /* IP报文类型判断 */
    switch(usEthType)
    {
        case RNIC_ETH_TYPE_IP:
            RNIC_SendULIpv4Data(pstSkb, pstNetCntxt);
            RNIC_DBG_RECV_UL_IPV4_PKT_NUM(1, enRmNetId);
            break;

        case RNIC_ETH_TYPE_IPV6:
            RNIC_SendULIpv6Data(pstSkb, pstNetCntxt);
            RNIC_DBG_RECV_UL_IPV6_PKT_NUM(1, enRmNetId);
            break;

        default:
            IMM_ZcFreeAny(pstSkb);
            RNIC_DBG_RECV_UL_ERR_PKT_NUM(1, enRmNetId);
            break;
    }

    return;
}
/* Modified by l60609 for L-C互操作项目, 2014-1-14, end */

/* Modified by l60609 for L-C互操作项目, 2014-01-06, Begin */


VOS_UINT32  RNIC_RcvAdsDlData(
    VOS_UINT8                           ucExRabid,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType,
    VOS_UINT32                          ulExParam
)
{
    VOS_UINT32                          ulRet;
    VOS_UINT8                           ucRmNetId;

    ucRmNetId = RNIC_GET_RMNETID_FROM_EXPARAM(ulExParam);

    ulRet = RNIC_SendDlData(ucRmNetId, pstImmZc, enPktType);

    return ulRet;
}


VOS_UINT32 RNIC_SendDlData(
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
)
{
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt = VOS_NULL_PTR;

    if (!RNIC_RMNET_IS_VALID(enRmNetId))
    {
        IMM_ZcFreeAny(pstImmZc);
        return RNIC_INVAL;
    }

    /* 获取网卡上下文 */
    pstNetCntxt = RNIC_GET_SPEC_NET_CTX(enRmNetId);


    return RNIC_NetRxDataEx(pstNetCntxt, pstImmZc, enPktType);
}
/* Modified by l60609 for L-C互操作项目, 2014-01-06, End */


unsigned int RNIC_StartFlowCtrl(unsigned char ucRmNetId)
{
    RNIC_SET_FLOW_CTRL_STATUS(RNIC_FLOW_CTRL_STATUS_START, ucRmNetId);
    return VOS_OK;
}


unsigned int RNIC_StopFlowCtrl(unsigned char ucRmNetId)
{
    RNIC_SET_FLOW_CTRL_STATUS(RNIC_FLOW_CTRL_STATUS_STOP, ucRmNetId);
    return VOS_OK;
}


unsigned long RNIC_ConfigRmnetStatus(
    RNIC_RMNET_CONFIG_STRU             *pstConfigInfo
)
{
    /*可维可测，输出配置信息*/
    RNIC_MNTN_SndRmnetConfigInfoMsg(pstConfigInfo);
    RNIC_DBG_CONFIGCHECK_ADD_TOTLA_NUM();

    /* 参数检查 */
    /* 内部modem需要检查 rab id是否异常 */
    if (RNIC_MODEM_TYPE_INSIDE == pstConfigInfo->enModemType)
    {
        if (!RNIC_RAB_ID_IS_VALID(pstConfigInfo->ucRabId))
        {
            RNIC_DBG_CONFIGCHECK_ADD_RABID_ERR_NUM();
            RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_ConfigRmnetStatus: invaild RAB id !");
            return VOS_ERR;
        }
    }
    /* 外部modem需要检查 pdn id是否异常 */
    else if (RNIC_MODEM_TYPE_OUTSIDE == pstConfigInfo->enModemType)
    {
        if (!RNIC_PDN_ID_IS_VALID(pstConfigInfo->ucPdnId))
        {
            RNIC_DBG_CONFIGCHECK_ADD_PDNID_ERR_NUM();
            RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_ConfigRmnetStatus: invaild Pdn id !");
            return VOS_ERR;
        }
    }
    /* 无效MODEM TYPE */
    else
    {
        RNIC_DBG_CONFIGCHECK_ADD_MODEMTYPE_ERR_NUM();
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_ConfigRmnetStatus: invaild modem type!");
        return VOS_ERR;
    }

    /* 网卡操作类型异常直接返回error */
    if (!RNIC_RMNET_STATUS_IS_VALID(pstConfigInfo->enRmnetStatus))
    {
        RNIC_DBG_CONFIGCHECK_ADD_RMNETSTATUS_ERR_NUM();
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_ConfigRmnetStatus: invaild Rmnet Status !");
        return VOS_ERR;
    }

    /* IP类型非法直接返回error */
    if (!RNIC_IP_TYPE_IS_VALID(pstConfigInfo->enIpType))
    {
        RNIC_DBG_CONFIGCHECK_ADD_IPTYPE_ERR_NUM();
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_ConfigRmnetStatus: invaild IP type !");
        return VOS_ERR;
    }

    /* 发送内部消息 */
    if (VOS_OK != RNIC_SndRnicRmnetConfigReq(pstConfigInfo))
    {
        RNIC_DBG_CONFIGCHECK_ADD_SND_ERR_NUM();
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_ConfigRmnetStatus: send pdp status ind fail !");
        return VOS_ERR;
    }

    RNIC_DBG_CONFIGCHECK_ADD_SUCC_NUM();
    return VOS_OK;
}


VOS_INT32 RNIC_CheckNetCardStatus(RNIC_SPEC_CTX_STRU *pstNetCntxt)
{
    RNIC_NETCARD_DEV_INFO_STRU         *pstPriv    = VOS_NULL_PTR;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId  = RNIC_RMNET_ID_BUTT;
    RNIC_RESULT_TYPE_ENUM_INT32         enRet      = RNIC_BUTT;

    pstPriv     = pstNetCntxt->pstPriv;
    enRmNetId   = pstNetCntxt->enRmNetId;

    /* 网卡设备检查 */
    if (VOS_NULL_PTR == pstPriv)
    {
        RNIC_DBG_NETCAED_DL_DISCARD_NUM(1, enRmNetId);
        return RNIC_INVAL;
    }

    /* 网卡未打开 */
    if (RNIC_NETCARD_STATUS_CLOSED == pstPriv->enStatus)
    {
        RNIC_DBG_DISCARD_DL_PKT_NUM(1, enRmNetId);
        pstPriv->stStats.rx_dropped++;
        return RNIC_OK;
    }

    return enRet;
}


VOS_INT32 RNIC_AddMacHead(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
)
{
    VOS_UINT8                          *pucAddData = VOS_NULL_PTR;
    RNIC_RESULT_TYPE_ENUM_INT32         enRet      = RNIC_BUTT;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId  = RNIC_RMNET_ID_BUTT;

    enRmNetId   = pstNetCntxt->enRmNetId;

    /* 数据长度超出有效值(不包含MAC头) */
    if ((pstImmZc->len) > RNIC_MAX_PACKET)
    {
        RNIC_DBG_RECV_DL_BIG_PKT_NUM(1, enRmNetId);
        pstNetCntxt->pstPriv->stStats.rx_errors++;
        pstNetCntxt->pstPriv->stStats.rx_length_errors++;
        return RNIC_OK;
    }

    /* 填充MAC帧头，调用ImmZc接口将MAC帧头填入ImmZc中 */
    if (ADS_PKT_TYPE_IPV4 == enPktType)
    {
        pucAddData = (VOS_UINT8*)&g_astRnicManageTbl[enRmNetId].stIpv4Ethhead;
        RNIC_DBG_RECV_DL_IPV4_PKT_NUM(1, enRmNetId);
    }
    else if (ADS_PKT_TYPE_IPV6 == enPktType)
    {
        pucAddData = (VOS_UINT8*)&g_astRnicManageTbl[enRmNetId].stIpv6Ethhead;
        RNIC_DBG_RECV_DL_IPV6_PKT_NUM(1, enRmNetId);
    }
    else   /* 数据包类型与承载支持类型不一致 */
    {
        RNIC_DBG_RECV_DL_ERR_PKT_NUM(1, enRmNetId);
        return RNIC_PKT_TYPE_INVAL;
    }

    if (VOS_OK != IMM_ZcAddMacHead(pstImmZc, pucAddData))
    {
        RNIC_DBG_ADD_DL_MACHEAD_FAIL_NUM(1, enRmNetId);
        return RNIC_ADDMAC_FAIL;
    }

    return enRet;
}


VOS_INT32 RNIC_NetIfRx(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc
)
{
    VOS_INT32                           lNetRxRet  = NET_RX_SUCCESS;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId  = RNIC_RMNET_ID_BUTT;

    enRmNetId   = pstNetCntxt->enRmNetId;

    if (VOS_FALSE == VOS_CheckInterrupt())
    {
        lNetRxRet = netif_rx_ni(pstImmZc);
    }
    else
    {
        lNetRxRet = netif_rx(pstImmZc);
    }

    if (NET_RX_SUCCESS != lNetRxRet)
    {
        RNIC_DBG_SEND_DL_PKT_FAIL_NUM(1, enRmNetId);
        pstNetCntxt->pstPriv->stStats.rx_dropped++;

        RNIC_ERROR_LOG1(ACPU_PID_RNIC, "RNIC_NetIfRx, netif_rx fail ret is !", lNetRxRet);

        return RNIC_RX_PKT_FAIL;
    }

    /* 增加下行发送数据统计 */
    RNIC_DBG_SEND_DL_PKT_NUM(1, enRmNetId);

    return RNIC_OK;
}


VOS_INT32 RNIC_NetIfRxEx(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc
)
{
    VOS_INT32                           lNetRxRet  = NET_RX_SUCCESS;
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId  = RNIC_RMNET_ID_BUTT;

    enRmNetId   = pstNetCntxt->enRmNetId;

    if (VOS_FALSE == VOS_CheckInterrupt())
    {
        lNetRxRet = netif_rx_ni(pstImmZc);
    }
    else
    {
        {
            lNetRxRet = netif_rx(pstImmZc);
        }
    }

    if (NET_RX_SUCCESS != lNetRxRet)
    {
        RNIC_DBG_SEND_DL_PKT_FAIL_NUM(1, enRmNetId);
        pstNetCntxt->pstPriv->stStats.rx_dropped++;
        return RNIC_RX_PKT_FAIL;
    }

    {
        /* 增加下行发送数据统计 */
        RNIC_DBG_SEND_DL_PKT_NUM(1, enRmNetId);
    }

    return RNIC_OK;
}


VOS_UINT32 RNIC_EncapEthHead(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
)
{
    RNIC_NETCARD_DEV_INFO_STRU         *pstPriv    = VOS_NULL_PTR;
    VOS_UINT32                          ulRet      = RNIC_BUTT;

    pstPriv     = pstNetCntxt->pstPriv;


    ulRet = (VOS_UINT32)RNIC_CheckNetCardStatus(pstNetCntxt);
    /* RNIC网卡设备异常 */
    if (RNIC_BUTT != ulRet)
    {
        IMM_ZcFreeAny(pstImmZc);

        RNIC_ERROR_LOG1(ACPU_PID_RNIC, "RNIC_EncapEthHead, RNIC_CheckNetCardStatus fail ret is !", ulRet);

        return ulRet;
    }

    ulRet = (VOS_UINT32)RNIC_AddMacHead(pstNetCntxt, pstImmZc, enPktType);
    /* 添加MAC头异常 */
    if (RNIC_BUTT != ulRet)
    {
        IMM_ZcFreeAny(pstImmZc);

        RNIC_ERROR_LOG1(ACPU_PID_RNIC, "RNIC_EncapEthHead, RNIC_AddMacHead fail ret is !", ulRet);

        return ulRet;
    }

    pstImmZc->protocol = eth_type_trans(pstImmZc, pstPriv->pstDev);

    return ulRet;
}


VOS_UINT32 RNIC_NetRxData(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
)
{
    VOS_UINT32                          ulRet = RNIC_BUTT;

    ulRet = RNIC_EncapEthHead(pstNetCntxt, pstImmZc, enPktType);
    /* 封装以太网头失败 */
    if (RNIC_BUTT != ulRet)
    {
        RNIC_ERROR_LOG1(ACPU_PID_RNIC, "RNIC_NetRxData, RNIC_EncapEthHead fail ret is !", ulRet);

        return ulRet;
    }

    /* 统计网卡接收数据信息 */
    pstNetCntxt->pstPriv->stStats.rx_packets++;
    pstNetCntxt->pstPriv->stStats.rx_bytes += pstImmZc->len;

    /* 统计收到的下行数据字节数，用于流量上报 */
    pstNetCntxt->stDsFlowStats.ulTotalRecvFluxLow += pstImmZc->len;

    return (VOS_UINT32)RNIC_NetIfRx(pstNetCntxt, pstImmZc);
}


VOS_UINT32 RNIC_NetRxDataEx(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
)
{
    VOS_UINT32                          ulRet = RNIC_BUTT;

    ulRet = RNIC_EncapEthHead(pstNetCntxt, pstImmZc, enPktType);
    /* 封装以太网头失败 */
    if (RNIC_BUTT != ulRet)
    {
        return ulRet;
    }

    /* 统计网卡接收数据信息 */
    pstNetCntxt->pstPriv->stStats.rx_packets++;
    pstNetCntxt->pstPriv->stStats.rx_bytes += pstImmZc->len;

    /* 统计收到的下行数据字节数，用于流量上报 */
    pstNetCntxt->stDsFlowStats.ulTotalRecvFluxLow += pstImmZc->len;

    return (VOS_UINT32)RNIC_NetIfRxEx(pstNetCntxt, pstImmZc);
}


VOS_VOID RNIC_ProcessTxDataByModemType(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    struct sk_buff                     *pstSkb
)
{
    RNIC_DBG_PRINT_UL_DATA(pstSkb);

    if (RNIC_MODEM_TYPE_INSIDE == pstNetCntxt->enModemType)
    {
        RNIC_RcvInsideModemUlData(pstSkb, pstNetCntxt);
    }
    else
    {
        IMM_ZcFreeAny(pstSkb);
    }

    return;
}



VOS_INT RNIC_BST_GetModemInfo(
    struct net_device                  *pstNetDev,
    BST_RNIC_MODEM_INFO_STRU           *pstModemInfo
)
{
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt = VOS_NULL_PTR;
    RNIC_NETCARD_DEV_INFO_STRU         *pstPriv     = VOS_NULL_PTR;

    if (VOS_NULL_PTR == pstNetDev)
    {
        RNIC_DEV_ERR_PRINTK("RNIC_BST_GetModemInfo: pstNetDev is null.");
        return VOS_ERROR;
    }

    if (VOS_NULL_PTR == pstModemInfo)
    {
        RNIC_DEV_ERR_PRINTK("RNIC_BST_GetModemInfo: pstModemInfo is null.");
        return VOS_ERROR;
    }

    pstPriv = (RNIC_NETCARD_DEV_INFO_STRU *)netdev_priv(pstNetDev);

    pstNetCntxt = RNIC_GetNetCntxtByRmNetId(pstPriv->enRmNetId);
    if (VOS_NULL_PTR == pstNetCntxt)
    {
        RNIC_DEV_ERR_PRINTK("RNIC_BST_GetModemInfo: enRmNetId is invalid.");
        return VOS_ERROR;
    }

    pstModemInfo->enIPv4State = (RNIC_PDP_REG_STATUS_DEACTIVE == pstNetCntxt->stPdpCtx.stIpv4PdpInfo.enRegStatus) ?
                                BST_RNIC_PDP_STATE_INACTIVE : BST_RNIC_PDP_STATE_ACTIVE;
    pstModemInfo->usModemId   = pstNetCntxt->enModemId;
    pstModemInfo->ucRabId     = pstNetCntxt->stPdpCtx.stIpv4PdpInfo.ucRabId;

    return VOS_OK;
}


