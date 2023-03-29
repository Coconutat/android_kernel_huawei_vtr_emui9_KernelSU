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

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "RnicSndMsg.h"
#include "RnicLog.h"
#include "RnicCdsInterface.h"
#include "RnicDebug.h"

#include "NetMgrCtrlVcom.h"


#define    THIS_FILE_ID        PS_FILE_ID_RNIC_SND_MSG_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/

VOS_UINT32 RNIC_SendDialInfoMsg(
    RNIC_MSG_ID_ENUM_UINT32            enMsgId
)
{
    RNIC_NOTIFY_MSG_STRU               *pstDialInfo;
    RNIC_DIAL_MODE_STRU                *pstDialMode;


    /* 内存分配 */
    pstDialInfo = (RNIC_NOTIFY_MSG_STRU *)PS_ALLOC_MSG(ACPU_PID_RNIC,
                                                      sizeof(RNIC_NOTIFY_MSG_STRU) - VOS_MSG_HEAD_LENGTH);
    if (VOS_NULL_PTR == pstDialInfo)
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SendDialInfoMsg: Malloc failed!");
        return VOS_ERR;
    }

    pstDialMode                         = RNIC_GetDialModeAddr();

    /* 填充消息 */
    pstDialInfo->ulSenderCpuId          = VOS_LOCAL_CPUID;
    pstDialInfo->ulSenderPid            = ACPU_PID_RNIC;
    pstDialInfo->ulReceiverCpuId        = VOS_LOCAL_CPUID;
    pstDialInfo->ulReceiverPid          = ACPU_PID_RNIC;
    pstDialInfo->enMsgId                = enMsgId;

    TAF_MEM_CPY_S(&(pstDialInfo->stDialInfo), sizeof(pstDialInfo->stDialInfo), pstDialMode, sizeof(RNIC_DIAL_MODE_STRU));

    if (VOS_OK != PS_SEND_MSG(ACPU_PID_RNIC, pstDialInfo))
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SendDialInfoMsg: Send msg failed!");
        return VOS_ERR;
    }

    return VOS_OK;

}


VOS_UINT32 RNIC_SendCdsImsDataReq(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
)
{
    RNIC_CDS_IMS_DATA_REQ_STRU         *pstSndMsg = VOS_NULL_PTR;

    /* 内存分配 */
    pstSndMsg = (RNIC_CDS_IMS_DATA_REQ_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ACPU_PID_RNIC,
                                    sizeof(RNIC_CDS_IMS_DATA_REQ_STRU) - 4 + pstSkb->len);

    if (VOS_NULL_PTR == pstSndMsg)
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SendCdsImsDataReq: Malloc failed!");
        RNIC_DEV_ERR_PRINTK("RNIC_SendCdsImsDataReq: Malloc failed!");
        return VOS_ERR;
    }

    /* 填充消息头 */
    pstSndMsg->ulSenderCpuId            = VOS_LOCAL_CPUID;
    pstSndMsg->ulSenderPid              = ACPU_PID_RNIC;
    pstSndMsg->ulReceiverCpuId          = VOS_LOCAL_CPUID;
    pstSndMsg->ulReceiverPid            = UEPS_PID_CDS;
    pstSndMsg->ulMsgId                  = ID_RNIC_CDS_IMS_DATA_REQ;

    /* 填充消息 */
    pstSndMsg->usModemId                = pstNetCntxt->enModemId;
    pstSndMsg->usDataLen                = (VOS_UINT16)pstSkb->len;

    TAF_MEM_CPY_S(pstSndMsg->aucData, pstSkb->len, pstSkb->data, pstSkb->len);

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ACPU_PID_RNIC, pstSndMsg))
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SendCdsImsDataReq: Send msg failed!");
        RNIC_DEV_ERR_PRINTK("RNIC_SendCdsImsDataReq: Send msg failed!");
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_VOID RNIC_TrigImsDataProcEvent(RNIC_RMNET_ID_ENUM_UINT8 enRmNetId)
{
    RNIC_IMS_DATA_PROC_IND_STRU         *pstSndMsg = VOS_NULL_PTR;

    /* 内存分配 */
    pstSndMsg = (RNIC_IMS_DATA_PROC_IND_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(ACPU_PID_RNIC,
                                    sizeof(RNIC_IMS_DATA_PROC_IND_STRU));

    if (VOS_NULL_PTR == pstSndMsg)
    {
        RNIC_DEV_ERR_PRINTK("RNIC_TrigImsDataProcEvent: Malloc failed!");
        return;
    }

    /* 填充消息头 */
    pstSndMsg->ulSenderCpuId            = VOS_LOCAL_CPUID;
    pstSndMsg->ulSenderPid              = ACPU_PID_RNIC;
    pstSndMsg->ulReceiverCpuId          = VOS_LOCAL_CPUID;
    pstSndMsg->ulReceiverPid            = ACPU_PID_RNIC;
    pstSndMsg->enMsgId                  = ID_RNIC_IMS_DATA_PROC_IND;
    pstSndMsg->enRmNetId                = enRmNetId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ACPU_PID_RNIC, pstSndMsg))
    {
        RNIC_DEV_ERR_PRINTK("RNIC_TrigImsDataProcEvent: Send msg failed!");
    }

    return;
}


VOS_UINT32 RNIC_SndRnicRmnetConfigReq(
    RNIC_RMNET_CONFIG_STRU             *pstConfigInfo
)
{
    RNIC_RMNET_CONFIG_REQ_STRU         *pstSndMsg = VOS_NULL_PTR;

    /* 内存分配 */
    pstSndMsg = (RNIC_RMNET_CONFIG_REQ_STRU *)PS_ALLOC_MSG(ACPU_PID_RNIC,
                        sizeof(RNIC_RMNET_CONFIG_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstSndMsg)
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SndRnicRmnetConfigMsg: Malloc failed!");
        return VOS_ERR;
    }

    /* 填充消息头 */
    pstSndMsg->ulSenderCpuId            = VOS_LOCAL_CPUID;
    pstSndMsg->ulSenderPid              = ACPU_PID_RNIC;
    pstSndMsg->ulReceiverCpuId          = VOS_LOCAL_CPUID;
    pstSndMsg->ulReceiverPid            = ACPU_PID_RNIC;
    pstSndMsg->enMsgId                  = ID_RNIC_RMNET_CONFIG_REQ;

    /* 填充消息 */
    pstSndMsg->enModemType              = pstConfigInfo->enModemType;
    pstSndMsg->enRmnetStatus            = pstConfigInfo->enRmnetStatus;
    pstSndMsg->enIpType                 = pstConfigInfo->enIpType;
    pstSndMsg->ucPdnId                  = pstConfigInfo->ucPdnId;
    pstSndMsg->ucRabId                  = pstConfigInfo->ucRabId;
    pstSndMsg->ucRmNetId                = pstConfigInfo->ucRmNetId;
    pstSndMsg->usModemId                = pstConfigInfo->usModemId;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ACPU_PID_RNIC, pstSndMsg))
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SndRnicRmnetConfigMsg: Send msg failed!");
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_VOID RNIC_MNTN_SndRmnetConfigInfoMsg(
    RNIC_RMNET_CONFIG_STRU             *pstConfigInfo
)
{
    RNIC_RMNET_CONFIG_REQ_STRU          *pstSndMsg = VOS_NULL_PTR;

    /* 内存分配 */
    pstSndMsg = (RNIC_RMNET_CONFIG_REQ_STRU *)PS_ALLOC_MSG(ACPU_PID_RNIC,
                        sizeof(RNIC_RMNET_CONFIG_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstSndMsg)
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SndRnicRmnetConfigMsg: Malloc failed!");
        return;
    }

    /* 填充消息 */
    pstSndMsg->enModemType              = pstConfigInfo->enModemType;
    pstSndMsg->enRmnetStatus            = pstConfigInfo->enRmnetStatus;
    pstSndMsg->enIpType                 = pstConfigInfo->enIpType;
    pstSndMsg->ucPdnId                  = pstConfigInfo->ucPdnId;
    pstSndMsg->ucRabId                  = pstConfigInfo->ucRabId;
    pstSndMsg->ucRmNetId                = pstConfigInfo->ucRmNetId;
    pstSndMsg->usModemId                = pstConfigInfo->usModemId;

    pstSndMsg->ulSenderCpuId            = VOS_LOCAL_CPUID;
    pstSndMsg->ulSenderPid              = ACPU_PID_RNIC;
    pstSndMsg->ulReceiverCpuId          = VOS_LOCAL_CPUID;
    pstSndMsg->ulReceiverPid            = ACPU_PID_RNIC;
    pstSndMsg->enMsgId                  = ID_RNIC_MNTN_RMNET_CONFIG_INFO;

    /* 发送消息 */
    if (VOS_OK != PS_SEND_MSG(ACPU_PID_RNIC, pstSndMsg))
    {
        RNIC_ERROR_LOG(ACPU_PID_RNIC, "RNIC_SndRnicRmnetConfigMsg: Send msg failed!");
        return;
    }

    return;
}



VOS_VOID RNIC_FillNetManagerMsgPdnCfgInfo(
    NM_PDN_INFO_CONFIG_STRU            *pDestPdnInfo,
    IMSA_RNIC_PDN_INFO_CONFIG_STRU     *pSrcPdnInfo
)
{
    /* 对stRnicNmMsg中参数进行赋值 */
    pDestPdnInfo->bitOpIpv4PdnInfo     = pSrcPdnInfo->bitOpIpv4PdnInfo;
    pDestPdnInfo->bitOpIpv6PdnInfo     = pSrcPdnInfo->bitOpIpv6PdnInfo;
    pDestPdnInfo->bitOpSockPortInfo    = pSrcPdnInfo->bitOpSockPortInfo;

    pDestPdnInfo->enModemId    = pSrcPdnInfo->enModemId;
    pDestPdnInfo->enRatType    = pSrcPdnInfo->enRatType;

    /* 对ipv4的pdn进行赋值 */
    TAF_MEM_CPY_S(&(pDestPdnInfo->stIpv4PdnInfo),
                  sizeof(NM_IPV4_PDN_INFO_STRU),
                  &(pSrcPdnInfo->stIpv4PdnInfo),
                  sizeof(IMSA_RNIC_IPV4_PDN_INFO_STRU));

    TAF_MEM_CPY_S(&(pDestPdnInfo->stIpv6PdnInfo),
                  sizeof(NM_IPV6_PDN_INFO_STRU),
                  &(pSrcPdnInfo->stIpv6PdnInfo),
                  sizeof(IMSA_RNIC_IPV6_PDN_INFO_STRU));

    TAF_MEM_CPY_S(&(pDestPdnInfo->stSockPortInfo),
                  sizeof(NM_SOCK_PORT_INFO_STRU),
                  &(pSrcPdnInfo->stSockPortInfo),
                  sizeof(IMSA_RNIC_SOCK_PORT_INFO_STRU));

    return;
}


VOS_VOID RNIC_SndNetManagerPdpActInd(
    MsgBlock                           *pstMsg
)
{
    IMSA_RNIC_PDN_ACT_IND_STRU         *pstRcvInd;
    NM_MSG_STRU                         stRnicNmMsg;

    TAF_MEM_SET_S(&stRnicNmMsg, sizeof(NM_MSG_STRU), 0x00, sizeof(NM_MSG_STRU));

    pstRcvInd               = (IMSA_RNIC_PDN_ACT_IND_STRU *)pstMsg;
    stRnicNmMsg.enMsgId     = ID_NM_PDN_ACT_IND;
    stRnicNmMsg.ulMsgLen    = sizeof(NM_PDN_INFO_CONFIG_STRU);

    RNIC_FillNetManagerMsgPdnCfgInfo(&(stRnicNmMsg.unMsgInfo.stPdnCfgInfo), &(pstRcvInd->stPdnInfo));

    /* 调用虚拟设备提供的发送接口发送消息 */
    NM_CTRL_SendMsg(&stRnicNmMsg, sizeof(NM_MSG_STRU));

    return;
}


VOS_VOID RNIC_SndNetManagerPdpDeactInd(
    MsgBlock                           *pstMsg
)
{
    IMSA_RNIC_PDN_DEACT_IND_STRU       *pstRcvInd;
    NM_MSG_STRU                         stRnicNmMsg;

    pstRcvInd                           = (IMSA_RNIC_PDN_DEACT_IND_STRU *)pstMsg;
    TAF_MEM_SET_S(&stRnicNmMsg, sizeof(NM_MSG_STRU), 0x00, sizeof(NM_MSG_STRU));

    stRnicNmMsg.enMsgId     = ID_NM_PDN_DEACT_IND;
    stRnicNmMsg.ulMsgLen    = sizeof(NM_PDN_DEACT_IND_STRU);

    /* 对stRnicNmMsg中参数进行赋值 */
    stRnicNmMsg.unMsgInfo.stPdnDeactInd.enModemId   = pstRcvInd->enModemId;
    stRnicNmMsg.unMsgInfo.stPdnDeactInd.enRatType   = pstRcvInd->enRatType;

    /* 调用虚拟设备提供的发送接口发送消息 */
    NM_CTRL_SendMsg(&stRnicNmMsg, sizeof(NM_MSG_STRU));

    return;
}


VOS_VOID RNIC_SndNetManagerPdpModifyInd(
    MsgBlock                           *pstMsg
)
{
    IMSA_RNIC_PDN_MODIFY_IND_STRU      *pstRcvInd;
    NM_MSG_STRU                         stRnicNmMsg;

    TAF_MEM_SET_S(&stRnicNmMsg, sizeof(NM_MSG_STRU), 0x00, sizeof(NM_MSG_STRU));

    pstRcvInd               = (IMSA_RNIC_PDN_MODIFY_IND_STRU *)pstMsg;
    stRnicNmMsg.enMsgId     = ID_NM_PDN_MODIFY_IND;
    stRnicNmMsg.ulMsgLen    = sizeof(NM_PDN_INFO_CONFIG_STRU);

    RNIC_FillNetManagerMsgPdnCfgInfo(&(stRnicNmMsg.unMsgInfo.stPdnCfgInfo), &(pstRcvInd->stPdnInfo));

    /* 调用虚拟设备提供的发送接口发送消息 */
    NM_CTRL_SendMsg(&stRnicNmMsg, sizeof(NM_MSG_STRU));

    return;
}


VOS_VOID RNIC_SndNetManagerModemResetInd(VOS_VOID)
{
    NM_MSG_STRU                    stRnicNmMsg;

    TAF_MEM_SET_S(&stRnicNmMsg, sizeof(NM_MSG_STRU), 0x00, sizeof(NM_MSG_STRU));

    stRnicNmMsg.enMsgId     = ID_NM_MODEM_RESET_IND;
    stRnicNmMsg.ulMsgLen    = 0;

    /* 调用虚拟设备提供的发送接口发送消息 */
    NM_CTRL_SendMsg(&stRnicNmMsg, sizeof(NM_MSG_STRU));

    return;
}

