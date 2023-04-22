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
#include "AdsDownLink.h"
#include "AdsDebug.h"
#include "mdrv.h"
#include "AcpuReset.h"
#include "AdsFilter.h"
#include "AdsDhcpInterface.h"
#include "AdsMntn.h"



/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID                 PS_FILE_ID_ADS_DOWNLINK_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
VOS_UINT32                              g_ulAdsDlDiscardPktFlag  = VOS_FALSE;   /* ADS下行直接丢包开关 */

/*****************************************************************************
  3 函数实现
*****************************************************************************/


VOS_INT ADS_DL_CCpuResetCallback(
    DRV_RESET_CB_MOMENT_E               enParam,
    VOS_INT                             lUserData
)
{
    ADS_CCPU_RESET_IND_STRU                *pstMsg = VOS_NULL_PTR;

    /* 参数为0表示复位前调用 */
    if (MDRV_RESET_CB_BEFORE == enParam)
    {
        ADS_TRACE_HIGH("before reset enter.\n");

        /* 防止IPF数据残留, 主动读取一次RD */
        ADS_DL_SndEvent(ADS_DL_EVENT_IPF_RD_INT);
        ADS_DBG_DL_CCORE_RESET_TRIG_EVENT(1);

        /* 构造消息 */
        pstMsg = (ADS_CCPU_RESET_IND_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(ACPU_PID_ADS_DL,
                                                                        sizeof(ADS_CCPU_RESET_IND_STRU));
        if (VOS_NULL_PTR == pstMsg)
        {
            ADS_TRACE_HIGH("before reset: alloc msg failed.\n");
            return VOS_ERROR;
        }

        /* 填写消息头 */
        pstMsg->ulReceiverPid               = ACPU_PID_ADS_DL;
        pstMsg->enMsgId                     = ID_ADS_CCPU_RESET_START_IND;

        /* 发消息 */
        if (VOS_OK != PS_SEND_MSG(ACPU_PID_ADS_DL, pstMsg))
        {
            ADS_TRACE_HIGH("before reset: send msg failed.\n");
            return VOS_ERROR;
        }

        /* 等待回复信号量初始为锁状态，等待消息处理完后信号量解锁。 */
        if (VOS_OK != VOS_SmP(ADS_GetDLResetSem(), ADS_RESET_TIMEOUT_LEN))
        {
            ADS_TRACE_HIGH("before reset VOS_SmP failed.\n");
            ADS_DBG_DL_RESET_LOCK_FAIL_NUM(1);
            return VOS_ERROR;
        }

        ADS_TRACE_HIGH("before reset succ.\n");
        return VOS_OK;
    }
    /* 复位后 */
    else if (MDRV_RESET_CB_AFTER == enParam)
    {
        ADS_TRACE_HIGH("after reset enter.\n");

        /* 构造消息 */
        pstMsg = (ADS_CCPU_RESET_IND_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(ACPU_PID_ADS_DL,
                                                                        sizeof(ADS_CCPU_RESET_IND_STRU));
        if (VOS_NULL_PTR == pstMsg)
        {
            ADS_TRACE_HIGH("after reset: alloc msg failed.\n");
            return VOS_ERROR;
        }

        /* 填写消息头 */
        pstMsg->ulReceiverPid               = ACPU_PID_ADS_DL;
        pstMsg->enMsgId                     = ID_ADS_CCPU_RESET_END_IND;

        /* 发消息 */
        if (VOS_OK != PS_SEND_MSG(ACPU_PID_ADS_DL, pstMsg))
        {
            ADS_TRACE_HIGH("after reset: send msg failed.\n");
            return VOS_ERROR;
        }

        ADS_TRACE_HIGH("after reset: succ.\n");
        ADS_DBG_DL_RESET_SUCC_NUM(1);
        return VOS_OK;
    }
    else
    {
        return VOS_ERROR;
    }
}


VOS_INT32 ADS_DL_IpfIntCB(VOS_VOID)
{
    if (ADS_IPF_MODE_INT == ADS_GET_IPF_MODE())
    {
        ADS_DL_WakeLock();
        ADS_DL_ProcIpfResult();
        ADS_DL_WakeUnLock();
    }
    else
    {
        ADS_DL_SndEvent(ADS_DL_EVENT_IPF_RD_INT);
    }
    ADS_DBG_DL_RCV_IPF_RD_INT_NUM(1);

    return VOS_OK;
}


VOS_INT32 ADS_DL_IpfAdqEmptyCB(IPF_ADQ_EMPTY_E eAdqEmpty)
{
    if (IPF_EMPTY_ADQ0 == eAdqEmpty)
    {
        ADS_DBG_DL_ADQ_RCV_AD0_EMPTY_INT_NUM(1);
    }
    else if (IPF_EMPTY_ADQ1 == eAdqEmpty)
    {
        ADS_DBG_DL_ADQ_RCV_AD1_EMPTY_INT_NUM(1);
    }
    else
    {
        ADS_DBG_DL_ADQ_RCV_AD0_EMPTY_INT_NUM(1);
        ADS_DBG_DL_ADQ_RCV_AD1_EMPTY_INT_NUM(1);
    }

    ADS_DL_SndEvent(ADS_DL_EVENT_IPF_ADQ_EMPTY_INT);
    ADS_DBG_DL_RCV_IPF_ADQ_EMPTY_INT_NUM(1);
    return VOS_OK;
}


VOS_VOID ADS_DL_RcvTiAdqEmptyExpired(
    VOS_UINT32                          ulParam,
    VOS_UINT32                          ulTimerName
)
{
    /* 触发下行ADQ空中断处理事件 */
    ADS_DL_SndEvent(ADS_DL_EVENT_IPF_ADQ_EMPTY_INT);
    ADS_DBG_DL_ADQ_EMPTY_TMR_TIMEOUT_NUM(1);
    return;
}


VOS_UINT32 ADS_DL_ConfigAdq(
    IPF_AD_TYPE_E                       enAdType,
    VOS_UINT                            ulIpfAdNum
)
{
    IPF_AD_DESC_S                      *pstAdDesc = VOS_NULL_PTR;
    IMM_ZC_STRU                        *pstImmZc  = VOS_NULL_PTR;
    VOS_UINT32                          ulPoolId;
    VOS_UINT32                          ulTmp;
    VOS_UINT32                          ulCnt;
    VOS_UINT32                          ulLen;
    VOS_INT32                           lRslt;

    if (IPF_AD_0 == enAdType)
    {
        ulPoolId = ADS_IPF_MEM_POOL_ID_AD0;
        ulLen    = ADS_IPF_AD0_MEM_BLK_SIZE;
    }
    else
    {
        ulPoolId = ADS_IPF_MEM_POOL_ID_AD1;
        ulLen    = ADS_IPF_AD1_MEM_BLK_SIZE;
    }

    for (ulCnt = 0; ulCnt < ulIpfAdNum; ulCnt++)
    {
        pstImmZc = ADS_IPF_AllocMem(ulPoolId, ulLen, IMM_MAC_HEADER_RES_LEN);
        if (VOS_NULL_PTR == pstImmZc)
        {
            ADS_DBG_DL_ADQ_ALLOC_MEM_FAIL_NUM(1);
            break;
        }

        ADS_DBG_DL_ADQ_ALLOC_MEM_SUCC_NUM(1);

        /* 填写AD描述符: OUTPUT0 ---> 目的地址; OUTPUT1 ---> SKBUFF */
        pstAdDesc = ADS_DL_GET_IPF_AD_DESC_PTR(enAdType, ulCnt);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
        pstAdDesc->u32OutPtr0 = (VOS_UINT32)virt_to_phys((VOS_VOID *)pstImmZc->data);
        pstAdDesc->u32OutPtr1 = (VOS_UINT32)virt_to_phys((VOS_VOID *)pstImmZc);
#else
        pstAdDesc->u32OutPtr0 = (VOS_UINT32)ADS_IPF_GetMemDma(pstImmZc);
        pstAdDesc->u32OutPtr1 = (VOS_UINT32)virt_to_phys((VOS_VOID *)pstImmZc);
#endif/* (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0) */
    }

    if (0 == ulCnt)
    {
        return 0;
    }

    /* 申请到AD才需要配置ADQ */
    lRslt = mdrv_ipf_config_dlad(enAdType, ulCnt, ADS_DL_GET_IPF_AD_DESC_PTR(enAdType, 0));
    if (IPF_SUCCESS != lRslt)
    {
        /* 配置失败，释放内存 */
        ulTmp = ulCnt;
        for (ulCnt = 0; ulCnt < ulTmp; ulCnt++)
        {
            pstAdDesc = ADS_DL_GET_IPF_AD_DESC_PTR(enAdType, ulCnt);
            pstImmZc  = (IMM_ZC_STRU *)phys_to_virt((unsigned long)pstAdDesc->u32OutPtr1);
            IMM_ZcFreeAny(pstImmZc);
            ADS_DBG_DL_ADQ_FREE_MEM_NUM(1);
        }

        ADS_DBG_DL_ADQ_CFG_IPF_FAIL_NUM(1);
        return 0;
    }

    /* 输出实际配置的AD数目 */
    ADS_DBG_DL_ADQ_CFG_AD_NUM(ulCnt);
    ADS_DBG_DL_ADQ_CFG_IPF_SUCC_NUM(1);
    return ulCnt;
}


VOS_VOID ADS_DL_AllocMemForAdq(VOS_VOID)
{
    VOS_INT32                           lRslt;
    VOS_UINT32                          ulIpfAd0Num;
    VOS_UINT32                          ulIpfAd1Num;
    VOS_UINT32                          ulActAd0Num;
    VOS_UINT32                          ulActAd1Num;

    ulIpfAd0Num = 0;
    ulIpfAd1Num = 0;
    ulActAd0Num = 0;
    ulActAd1Num = 0;

    /* 获取两个ADQ的空闲的AD个数 */
    lRslt = mdrv_ipf_get_dlad_num(&ulIpfAd0Num, &ulIpfAd1Num);
    if (IPF_SUCCESS != lRslt)
    {
        ADS_DBG_DL_ADQ_GET_FREE_AD_FAIL_NUM(1);
        return;
    }

    ADS_DBG_DL_ADQ_GET_FREE_AD_SUCC_NUM(1);

    /* 首先配置大内存的ADQ1 */
    if (0 != ulIpfAd1Num)
    {
        ulActAd1Num = ADS_DL_ConfigAdq(IPF_AD_1, ulIpfAd1Num);
        ADS_DBG_DL_ADQ_CFG_AD1_NUM(ulActAd1Num);
    }

    /* 再配置小内存的ADQ0 */
    if (0 != ulIpfAd0Num)
    {
        ulActAd0Num = ADS_DL_ConfigAdq(IPF_AD_0, ulIpfAd0Num);
        ADS_DBG_DL_ADQ_CFG_AD0_NUM(ulActAd0Num);
    }

    /* AD0为空或者AD1为空需要重新启动定时器 */
    if ( ((0 == ulActAd0Num) && (ADS_IPF_DLAD_START_TMR_THRESHOLD < ulIpfAd0Num))
      || ((0 == ulActAd1Num) && (ADS_IPF_DLAD_START_TMR_THRESHOLD < ulIpfAd1Num)) )
    {
        /* 如果两个ADQ任何一个空且申请不到内存，启定时器 */
        ADS_StartTimer(TI_ADS_DL_ADQ_EMPTY, TI_ADS_DL_ADQ_EMPTY_LEN);
    }

    return;
}


VOS_VOID ADS_DL_SendNdClientDataInd(IMM_ZC_STRU *pstImmZc)
{
    ADS_NDCLIENT_DATA_IND_STRU         *pstMsg  = VOS_NULL_PTR;
    VOS_UINT8                          *pucData = VOS_NULL_PTR;
    VOS_UINT32                          ulDataLen;
    VOS_UINT32                          ulResult;

    ulDataLen = IMM_ZcGetUsedLen(pstImmZc);

    /* 申请消息 */
    pstMsg = (ADS_NDCLIENT_DATA_IND_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                               ACPU_PID_ADS_DL,
                                               sizeof(ADS_NDCLIENT_DATA_IND_STRU) + ulDataLen - 2);
    if (VOS_NULL_PTR == pstMsg)
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendNdClientDataInd: pstMsg is NULL!");
        return;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(ADS_NDCLIENT_DATA_IND_STRU) + ulDataLen - 2 - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(ADS_NDCLIENT_DATA_IND_STRU) + ulDataLen - 2 - VOS_MSG_HEAD_LENGTH));

    /* 填写消息内容 */
    pstMsg->ulReceiverPid = UEPS_PID_NDCLIENT;
    pstMsg->enMsgId       = ID_ADS_NDCLIENT_DATA_IND;
    pstMsg->enModemId     = ADS_DL_GET_MODEMID_FROM_IMM(pstImmZc);
    pstMsg->ucRabId       = ADS_DL_GET_RABID_FROM_IMM(pstImmZc);;
    pstMsg->usLen         = (VOS_UINT16)ulDataLen;

    /* 拷贝数据内容 */
    pucData = IMM_ZcGetDataPtr(pstImmZc);
    TAF_MEM_CPY_S(pstMsg->aucData, ulDataLen, pucData, ulDataLen);

    /* 发送消息 */
    ulResult = PS_SEND_MSG(ACPU_PID_ADS_DL, pstMsg);
    if (VOS_OK != ulResult)
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendNdClientDataInd: Send msg fail!");
    }

    return;
}


VOS_VOID ADS_DL_SendDhcpClientDataInd(IMM_ZC_STRU *pstImmZc)
{
    ADS_DHCP_DATA_IND_STRU             *pstMsg  = VOS_NULL_PTR;
    VOS_UINT8                          *pucData = VOS_NULL_PTR;
    VOS_UINT32                          ulDataLen;
    VOS_UINT32                          ulResult;

    ulDataLen = IMM_ZcGetUsedLen(pstImmZc);

    /* 申请消息 */
    pstMsg = (ADS_DHCP_DATA_IND_STRU *)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                               ACPU_PID_ADS_DL,
                                               sizeof(ADS_DHCP_DATA_IND_STRU) + ulDataLen - 2);

    /* 内存申请失败，返回 */
    if( VOS_NULL_PTR == pstMsg )
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendDhcpClientDataInd: pstMsg is NULL!");
        return;
    }

    TAF_MEM_SET_S((VOS_INT8 *)pstMsg + VOS_MSG_HEAD_LENGTH,
               (VOS_SIZE_T)(sizeof(ADS_DHCP_DATA_IND_STRU) + ulDataLen - 2 - VOS_MSG_HEAD_LENGTH),
               0x00,
               (VOS_SIZE_T)(sizeof(ADS_DHCP_DATA_IND_STRU) + ulDataLen - 2 - VOS_MSG_HEAD_LENGTH));

    /* 填写消息内容 */
    pstMsg->ulReceiverPid = UEPS_PID_DHCP;
    pstMsg->enMsgId       = ID_ADS_DHCP_DATA_IND;
    pstMsg->enModemId     = ADS_DL_GET_MODEMID_FROM_IMM(pstImmZc);
    pstMsg->ucRabId       = ADS_DL_GET_RABID_FROM_IMM(pstImmZc);
    pstMsg->usLen         = (VOS_UINT16)ulDataLen;

    /* 拷贝数据 */
    pucData = IMM_ZcGetDataPtr(pstImmZc);
    TAF_MEM_CPY_S(pstMsg->aucData, ulDataLen, pucData, ulDataLen);

    /* 调用VOS发送原语 */
    ulResult = PS_SEND_MSG(ACPU_PID_ADS_DL, pstMsg);
    if (VOS_OK != ulResult)
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_SendDhcpClientDataInd: Send msg fail!");
    }

    return;
}


VOS_VOID ADS_DL_FreeIpfUsedAd0(VOS_VOID)
{
    VOS_UINT32                          i;
    VOS_UINT32                          ulAdNum;

    IPF_AD_DESC_S                      *pstAdDesc = VOS_NULL_PTR;

    ulAdNum = 0;

    pstAdDesc = (IPF_AD_DESC_S*)PS_MEM_ALLOC(ACPU_PID_ADS_DL,
                                             sizeof(IPF_AD_DESC_S) * IPF_DLAD0_DESC_SIZE);

    if ( VOS_NULL_PTR == pstAdDesc )
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_FreeIpfUsedAd0: pstAdDesc is null");
        return;
    }

    TAF_MEM_SET_S(pstAdDesc, (VOS_SIZE_T)(sizeof(IPF_AD_DESC_S) * IPF_DLAD0_DESC_SIZE), 0x00, (VOS_SIZE_T)(sizeof(IPF_AD_DESC_S) * IPF_DLAD0_DESC_SIZE));

    if (IPF_SUCCESS == mdrv_ipf_get_used_dlad(IPF_AD_0, (VOS_UINT32 *)&ulAdNum, pstAdDesc))
    {
        /* 释放ADQ0的内存 */
        for (i = 0; i < PS_MIN(ulAdNum, IPF_DLAD0_DESC_SIZE); i++)
        {
            IMM_ZcFreeAny((IMM_ZC_STRU *)phys_to_virt((unsigned long)pstAdDesc[i].u32OutPtr1));
        }
    }

    PS_MEM_FREE(ACPU_PID_ADS_DL, pstAdDesc);

    return;
}


VOS_VOID ADS_DL_FreeIpfUsedAd1(VOS_VOID)
{
    VOS_UINT32                          i;
    VOS_UINT32                          ulAdNum;

    IPF_AD_DESC_S                      *pstAdDesc = VOS_NULL_PTR;

    ulAdNum = 0;

    pstAdDesc = (IPF_AD_DESC_S*)PS_MEM_ALLOC(ACPU_PID_ADS_DL,
                                             sizeof(IPF_AD_DESC_S) * IPF_DLAD1_DESC_SIZE);

    if ( VOS_NULL_PTR == pstAdDesc )
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_FreeIpfUsedAd1: pstAdDesc is null");
        return;
    }

    TAF_MEM_SET_S(pstAdDesc, (VOS_SIZE_T)(sizeof(IPF_AD_DESC_S) * IPF_DLAD1_DESC_SIZE), 0x00, (VOS_SIZE_T)(sizeof(IPF_AD_DESC_S) * IPF_DLAD1_DESC_SIZE));

    if (IPF_SUCCESS == mdrv_ipf_get_used_dlad(IPF_AD_1, (VOS_UINT32 *)&ulAdNum, pstAdDesc))
    {
        /* 释放ADQ1的内存 */
        for (i = 0; i < PS_MIN(ulAdNum, IPF_DLAD1_DESC_SIZE); i++)
        {
            IMM_ZcFreeAny((IMM_ZC_STRU *)phys_to_virt((unsigned long)pstAdDesc[i].u32OutPtr1));
        }
    }

    PS_MEM_FREE(ACPU_PID_ADS_DL, pstAdDesc);

    return;
}


VOS_VOID ADS_DL_Xmit(
    VOS_UINT8                           ucInstance,
    VOS_UINT8                           ucRabId,
    IMM_ZC_STRU                        *pstImmZc
)
{
    RCV_DL_DATA_FUNC                    pRcvDlDataFunc    = VOS_NULL_PTR;
    IMM_ZC_STRU                        *pstLstImmZc       = VOS_NULL_PTR;
    VOS_UINT32                          ulExParam;
    VOS_UINT16                          usIpfResult;
    ADS_PKT_TYPE_ENUM_UINT8             enIpType;
    VOS_UINT8                           ucExRabId;

    /* 检查MODEMID */
    if (!ADS_IS_MODEMID_VALID(ucInstance))
    {
        if (VOS_NULL_PTR != pstImmZc)
        {
            IMM_ZcFreeAny(pstImmZc);
            ADS_DBG_DL_RMNET_MODEMID_ERR_NUM(1);
        }

        return;
    }

    /* 获取RABID对应下行回调函数指针 */
    if (ADS_IS_RABID_VALID(ucRabId))
    {
        pRcvDlDataFunc    = ADS_DL_GET_DATA_CALLBACK_FUNC(ucInstance, ucRabId);
        ulExParam         = ADS_DL_GET_DATA_EXPARAM(ucInstance, ucRabId);
    }
    else
    {
        if (VOS_NULL_PTR != pstImmZc)
        {
            IMM_ZcFreeAny(pstImmZc);
            ADS_DBG_DL_RMNET_RABID_ERR_NUM(1);
        }

        return;
    }

    /* 获取缓存的数据 */
    pstLstImmZc = ADS_DL_GET_LST_DATA_PTR(ucInstance, ucRabId);

    /* 发送下行数据 */
    if (VOS_NULL_PTR != pRcvDlDataFunc)
    {
        if (VOS_NULL_PTR != pstLstImmZc)
        {
            usIpfResult = ADS_DL_GET_IPF_RESULT_FORM_IMM(pstLstImmZc);
            enIpType    = ADS_DL_GET_IPTYPE_FROM_IPF_RESULT(usIpfResult);
            ucExRabId   = ADS_BUILD_EXRABID(ucInstance, ucRabId);

            (VOS_VOID)pRcvDlDataFunc(ucExRabId, pstLstImmZc, enIpType, ulExParam);


            ADS_DBG_DL_RMNET_TX_PKT_NUM(1);
        }

        ADS_DL_SET_LST_DATA_PTR(ucInstance, ucRabId, pstImmZc);
    }
    else
    {
        if (VOS_NULL_PTR != pstImmZc)
        {
            IMM_ZcFreeAny(pstImmZc);
            ADS_DBG_DL_RMNET_NO_FUNC_FREE_PKT_NUM(1);
        }

        if (VOS_NULL_PTR != pstLstImmZc)
        {
            IMM_ZcFreeAny(pstLstImmZc);
            ADS_DBG_DL_RMNET_NO_FUNC_FREE_PKT_NUM(1);
        }

        ADS_DL_SET_LST_DATA_PTR(ucInstance, ucRabId, VOS_NULL_PTR);
    }

    return;
}


VOS_VOID ADS_DL_ProcTxData(IMM_ZC_STRU *pstImmZc)
{
    VOS_UINT8                           ucInstance;
    VOS_UINT8                           ucRabId;

    if (VOS_NULL_PTR != pstImmZc)
    {
        ucInstance = ADS_DL_GET_MODEMID_FROM_IMM(pstImmZc);
        ucRabId    = ADS_DL_GET_RABID_FROM_IMM(pstImmZc);

        ADS_DL_Xmit(ucInstance, ucRabId, pstImmZc);
    }
    else
    {
        for (ucInstance = 0; ucInstance < ADS_INSTANCE_MAX_NUM; ucInstance++)
        {
            for (ucRabId = ADS_RAB_ID_MIN; ucRabId <= ADS_RAB_ID_MAX; ucRabId++)
            {
                ADS_DL_Xmit(ucInstance, ucRabId, VOS_NULL_PTR);
            }
        }
    }

    return;
}


IMM_ZC_STRU* ADS_DL_RdDescTransImmMem(const IPF_RD_DESC_S *pstRdDesc)
{
    IMM_ZC_STRU                        *pstImmZc = VOS_NULL_PTR;
    VOS_UINT32                          ulCacheLen;

    /* 将OUT指针转换为IMM内存指针 */
    pstImmZc = (IMM_ZC_STRU *)phys_to_virt((unsigned long)pstRdDesc->u32OutPtr);
    if (VOS_NULL_PTR == pstImmZc)
    {
        return VOS_NULL_PTR;
    }

    /* 统一刷CACHE */
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
    ulCacheLen = pstRdDesc->u16PktLen + IMM_MAC_HEADER_RES_LEN;
#else
    ulCacheLen = pstRdDesc->u16PktLen;
#endif
    ADS_IPF_DL_MEM_UNMAP(pstImmZc, ulCacheLen);

    /* 设置数据真实长度 */
    IMM_ZcPut(pstImmZc, pstRdDesc->u16PktLen);

    /* 保存描述符信息: u16Result/u16UsrField1 */
    ADS_DL_SAVE_RD_DESC_TO_IMM(pstImmZc, pstRdDesc);

    return pstImmZc;
}


VOS_VOID ADS_DL_ProcIpfFilterData(
    IMM_ZC_STRU                        *pstImmZc
)
{
    VOS_UINT32                          ulCacheLen;
    VOS_UINT16                          usIpfResult;

    /* 统一刷CACHE */
    usIpfResult = ADS_DL_GET_IPF_RESULT_FORM_IMM(pstImmZc);
    ulCacheLen  = IMM_ZcGetUsedLen(pstImmZc) + IMM_MAC_HEADER_RES_LEN;

    ADS_IPF_SPE_MEM_UNMAP(pstImmZc, ulCacheLen);

    /* 匹配下行过滤规则的数据
     * BearId 19: NDClient包，需要转发给NDClient
     * BearId 17: DHCPv6包，需要转发给DHCP
     * [0, 15]定义为非法数据包;
     * [16, 18, 20, 21]目前直接释放
     */
    if (CDS_ADS_DL_IPF_BEARER_ID_ICMPV6 == ADS_DL_GET_BEAREDID_FROM_IPF_RESULT(usIpfResult))
    {
        ADS_DL_SendNdClientDataInd(pstImmZc);
        ADS_DBG_DL_RDQ_RX_ND_PKT_NUM(1);
    }
    else if (CDS_ADS_DL_IPF_BEARER_ID_DHCPV6 == ADS_DL_GET_BEAREDID_FROM_IPF_RESULT(usIpfResult))
    {
        ADS_DL_SendDhcpClientDataInd(pstImmZc);
        ADS_DBG_DL_RDQ_RX_DHCP_PKT_NUM(1);
    }
    else
    {
        ADS_DBG_DL_RDQ_FILTER_ERR_PKT_NUM(1);
    }

    ADS_IPF_SPE_MEM_MAP(pstImmZc, ulCacheLen);
    IMM_ZcFreeAny(pstImmZc);

    return;

}


VOS_VOID ADS_DL_ProcIpfFilterQue(VOS_VOID)
{
    IMM_ZC_STRU                        *pstImmZc = VOS_NULL_PTR;

    for ( ; ; )
    {
        pstImmZc = IMM_ZcDequeueHead(ADS_GET_IPF_FILTER_QUE());
        if (VOS_NULL_PTR == pstImmZc)
        {
            break;
        }

        ADS_DL_ProcIpfFilterData(pstImmZc);
    }

    return;
}


VOS_VOID ADS_DL_ProcIpfResult(VOS_VOID)
{
    ADS_DL_IPF_RESULT_STRU             *pstIpfResult = VOS_NULL_PTR;
    IPF_RD_DESC_S                      *pstRdDesc = VOS_NULL_PTR;
    IMM_ZC_STRU                        *pstImmZc = VOS_NULL_PTR;
    VOS_UINT32                          ulRdNum = IPF_DLRD_DESC_SIZE;
    VOS_UINT32                          ulTxTimeout = 0;
    VOS_UINT32                          ulCnt;

    /*
    IPF_RD_DESC_S中u16Result含义
    [15]Reserve
    [14]bd_cd_noeqBD中len和CD的长度不等错误提示，0表示长度匹配，1表示长度不匹配
    [13]pkt_parse_err数据解析错误指示，0表示数据解析正常，1表示数据解析错误
    [12]bd_pkt_noeqBD中len和IP包头指示的len不等错误指示，0表示长度匹配，1表示长度不匹配
    [11]head_len_err IPV4长度错误指示信号，IPV6不检查长度，0表示头长度正确，1表示头长度错误
    [10]version_err版本号错误指示，0表示版本号是4或6，1表示版本号不是4或6
    [9]ip_type IP包类型，0表示IPV4，1表示IPV6
    [8]ff_type分片包第一个分片是否包含上层头指示，0表示分片包第一个分片包括上层头(IP包未分片时也为0)
     1表示分片包第一个分片包括上层头
    [7:6]pf_type IP包分片指示类型，00表示IP包未分片，01表示IP包分片，且为第一个分片，
       02表示分片，且为最后一个分片，03表示分片，且为中间分片
    [0:5]bear_id承载号，如果为0x3F代表所有过滤器不匹配
    */

    /*
    IPF_RD_DESC_S中user field域含义
    u16UsrField1: RabId
    u32UsrField2: 目的TTF_MEM_ST指针
    u32UsrField3: 源TTF_MEM_ST指针
    */

    /* 获取RD */
    pstRdDesc = ADS_DL_GET_IPF_RD_DESC_PTR(0);
    mdrv_ipf_get_dlrd(&ulRdNum, pstRdDesc);

    /* 获取的RD为0 */
    if (0 == ulRdNum)
    {
        /* 增加RD获取个数为0的统计个数 */
        ADS_DBG_DL_RDQ_GET_RD0_NUM(1);
        return;
    }


    /* 增加RD统计个数 */
    ADS_DBG_DL_RDQ_RX_RD_NUM(ulRdNum);

    /* 先配置AD，再处理RD */
    ADS_DL_AllocMemForAdq();

    for (ulCnt = 0; ulCnt < ulRdNum; ulCnt++)
    {
        pstRdDesc = ADS_DL_GET_IPF_RD_DESC_PTR(ulCnt);

        /* 转换为IMM内存 */
        pstImmZc = ADS_DL_RdDescTransImmMem(pstRdDesc);
        if (VOS_NULL_PTR == pstImmZc)
        {
            ADS_DBG_DL_RDQ_TRANS_MEM_FAIL_NUM(1);
            continue;
        }

        /* 统计下行周期性收到的数据字节数，用于流量查询 */
        ADS_RECV_DL_PERIOD_PKT_NUM(pstRdDesc->u16PktLen);

        /* 获取IPF RESULT */
        pstIpfResult = (ADS_DL_IPF_RESULT_STRU *)&(pstRdDesc->u16Result);

        ADS_MNTN_RecDLIpPktInfo(pstImmZc,
                                ADS_DL_GET_IPS_RSLT_USR_FIELD1_FROM_IMM(pstImmZc),
                                pstRdDesc->u32UsrField2,
                                pstRdDesc->u32UsrField3
                               );

        /* BearId 0x3F: 正常下行数据包需要转发给NDIS/PPP/RNIC*/
        if (CDS_ADS_DL_IPF_BEARER_ID_INVALID == pstIpfResult->usBearedId)
        {
            if (ADS_DL_IPF_RD_RSLT_IS_ERR_PKT(*((VOS_UINT16 *)pstIpfResult)))
            {
                ADS_DBG_DL_RDQ_RX_ERR_PKT_NUM(1);
            }

            ulTxTimeout = ADS_DL_TX_WAKE_LOCK_TMR_LEN;

            if (VOS_TRUE == g_ulAdsDlDiscardPktFlag)
            {
                IMM_ZcFreeAny(pstImmZc);
                continue;
            }

            ADS_DL_ProcTxData(pstImmZc);
            ADS_DBG_DL_RDQ_RX_NORM_PKT_NUM(1);
        }
        else
        {
            if (VOS_FALSE != VOS_CheckInterrupt())
            {
                /* 先入队列缓存 */
                IMM_ZcQueueTail(ADS_GET_IPF_FILTER_QUE(), pstImmZc);
            }
            else
            {
                ADS_DL_ProcIpfFilterData(pstImmZc);
            }
        }
    }

    /* 推送最后一个数据 */
    ADS_DL_ProcTxData(VOS_NULL_PTR);

    if (0 != IMM_ZcQueueLen(ADS_GET_IPF_FILTER_QUE()))
    {
        ADS_DL_SndEvent(ADS_DL_EVENT_IPF_FILTER_DATA_PROC);
    }

    ADS_MNTN_ReportDLPktInfo();

    ADS_DL_EnableTxWakeLockTimeout(ulTxTimeout);
    return;
}


VOS_UINT32 ADS_DL_IsFcAssemTuneNeeded(VOS_UINT32 ulRdNum)
{
    ADS_DL_FC_ASSEM_STRU               *pstFcAssemInfo;
    VOS_UINT32                          ulRslt = VOS_FALSE;

    /* 获取流控阈值参数 */
    pstFcAssemInfo = ADS_DL_GET_FC_ASSEM_INFO_PTR(ADS_INSTANCE_INDEX_0);

    if (0 != pstFcAssemInfo->ulEnableMask)
    {
        pstFcAssemInfo->ulRdCnt += ulRdNum;

        if (ADS_TIME_AFTER_EQ(ADS_GET_CURR_KERNEL_TIME(), (pstFcAssemInfo->ulTmrCnt + pstFcAssemInfo->ulExpireTmrLen)))
        {
            if (VOS_FALSE == pstFcAssemInfo->ulFcActiveFlg)
            {
                if (pstFcAssemInfo->ulRdCnt >= pstFcAssemInfo->ulRateUpLev)
                {
                    ADS_DBG_DL_FC_ACTIVATE_NUM(1);
                    pstFcAssemInfo->ulFcActiveFlg = VOS_TRUE;
                    ulRslt = VOS_TRUE;
                }
            }
            else
            {
                if (pstFcAssemInfo->ulRdCnt <= pstFcAssemInfo->ulRateDownLev)
                {
                    pstFcAssemInfo->ulFcActiveFlg = VOS_FALSE;
                }
            }

            ADS_DBG_DL_FC_TMR_TIMEOUT_NUM(1);
            pstFcAssemInfo->ulTmrCnt = ADS_GET_CURR_KERNEL_TIME();
            pstFcAssemInfo->ulRdCnt = 0;
        }
    }

    return ulRslt;
}


VOS_VOID ADS_DL_RegDrvAssemFunc(FC_ADS_DRV_ASSEM_STRU *pstDrvAssemParam)
{
    ADS_DL_FC_ASSEM_STRU               *pstFcAssemParamInfo;

    pstFcAssemParamInfo = ADS_DL_GET_FC_ASSEM_INFO_PTR(ADS_INSTANCE_INDEX_0);

    pstFcAssemParamInfo->ulEnableMask     = pstDrvAssemParam->ucEnableMask;
    pstFcAssemParamInfo->ulRateUpLev      = pstDrvAssemParam->ulDlRateUpLev;
    pstFcAssemParamInfo->ulRateDownLev    = pstDrvAssemParam->ulDlRateDownLev;
    pstFcAssemParamInfo->ulExpireTmrLen   = pstDrvAssemParam->ulExpireTmrLen;
    pstFcAssemParamInfo->pFcAssemTuneFunc = pstDrvAssemParam->pDrvAssemFunc;

    return;
}


VOS_UINT32 ADS_DL_RegDlDataCallback(
    VOS_UINT8                           ucExRabId,
    RCV_DL_DATA_FUNC                    pFunc,
    VOS_UINT32                          ulExParam
)
{
    ADS_DL_RAB_INFO_STRU               *pstDlRabInfo = VOS_NULL_PTR;
    VOS_UINT8                           ucInstance;
    VOS_UINT8                           ucRabId;

    /* 检查MODEMID */
    ucInstance = ADS_GET_MODEMID_FROM_EXRABID(ucExRabId);
    if (!ADS_IS_MODEMID_VALID(ucInstance))
    {
        ADS_ERROR_LOG1(ACPU_PID_ADS_DL,
            "ADS_DL_RegDlDataCallback: ModemId is invalid! <ModemId>", ucInstance);
        return VOS_ERR;
    }

    /* 检查RABID */
    ucRabId = ADS_GET_RABID_FROM_EXRABID(ucExRabId);
    if (!ADS_IS_RABID_VALID(ucRabId))
    {
        ADS_ERROR_LOG2(ACPU_PID_ADS_DL,
            "ADS_DL_RegDlDataCallback: RabId is invalid! <ModemId>,<RabId>", ucInstance, ucRabId);
        return VOS_ERR;
    }

    /* 保存下行数据回调参数 */
    pstDlRabInfo = ADS_DL_GET_RAB_INFO_PTR(ucInstance, ucRabId);
    pstDlRabInfo->ucRabId           = ucRabId;
    pstDlRabInfo->ulExParam         = ulExParam;
    pstDlRabInfo->pRcvDlDataFunc    = pFunc;

    return VOS_OK;
}


VOS_UINT32 ADS_DL_RegFilterDataCallback(
    VOS_UINT8                           ucRabId,
    ADS_FILTER_IP_ADDR_INFO_STRU       *pstFilterIpAddr,
    RCV_DL_DATA_FUNC                    pFunc
)
{
    ADS_DL_RAB_INFO_STRU               *pstDlRabInfo        = VOS_NULL_PTR;
    VOS_UINT8                           ucInstanceIndex;
    VOS_UINT8                           ucRealRabId;

    /* ucRabId的高2个bit表示modem id*/
    ucInstanceIndex = ADS_GET_MODEMID_FROM_EXRABID(ucRabId);
    ucRealRabId     = ADS_GET_RABID_FROM_EXRABID(ucRabId);

    /* RabId合法性检查 */
    if (!ADS_IS_RABID_VALID(ucRealRabId))
    {
        ADS_WARNING_LOG1(ACPU_PID_ADS_DL, "ADS_DL_RegFilterDataCallback: ucRabId is", ucRealRabId);
        return VOS_ERR;
    }

    pstDlRabInfo = ADS_DL_GET_RAB_INFO_PTR(ucInstanceIndex, ucRealRabId);

    /* 设置ADS下行数据过滤回调内容 */
    pstDlRabInfo->ucRabId               = ucRealRabId;
    pstDlRabInfo->pRcvDlFilterDataFunc  = pFunc;

    /* 保存过滤地址信息 */
    ADS_FILTER_SaveIPAddrInfo(pstFilterIpAddr);

    return VOS_OK;
}


VOS_UINT32 ADS_DL_DeregFilterDataCallback(VOS_UINT8 ucRabId)
{
    ADS_DL_RAB_INFO_STRU               *pstDlRabInfo        = VOS_NULL_PTR;
    VOS_UINT8                           ucInstanceIndex;
    VOS_UINT8                           ucRealRabId;

    /* ucRabId的高2个bit表示modem id*/
    ucInstanceIndex = ADS_GET_MODEMID_FROM_EXRABID(ucRabId);
    ucRealRabId     = ADS_GET_RABID_FROM_EXRABID(ucRabId);

    /* RabId合法性检查 */
    if (!ADS_IS_RABID_VALID(ucRealRabId))
    {
        ADS_WARNING_LOG1(ACPU_PID_ADS_DL, "ADS_DL_DeregFilterDataCallback: ucRabId is", ucRealRabId);
        return VOS_ERR;
    }

    pstDlRabInfo = ADS_DL_GET_RAB_INFO_PTR(ucInstanceIndex, ucRealRabId);

    /* 去注册ADS下行数据过滤回调内容 */
    pstDlRabInfo->pRcvDlFilterDataFunc  = VOS_NULL_PTR;

    /* 清除过滤信息 */
    ADS_FILTER_Reset();

    return VOS_OK;
}



VOS_UINT32 ADS_DL_RcvTafPdpStatusInd(MsgBlock *pMsg)
{
    ADS_PDP_STATUS_IND_STRU            *pstPdpStatusInd;
    ADS_DL_RAB_INFO_STRU               *pstDlRabInfo;
    VOS_UINT8                           ucInstanceIndex;
    ADS_CDS_IPF_PKT_TYPE_ENUM_UINT8     enPktType;

    pstPdpStatusInd = (ADS_PDP_STATUS_IND_STRU *)pMsg;

    enPktType       = ADS_CDS_IPF_PKT_TYPE_IP;
    ucInstanceIndex = (VOS_UINT8)(pstPdpStatusInd->enModemId);

    /* RabId合法性检查 */
    if (!ADS_IS_RABID_VALID(pstPdpStatusInd->ucRabId))
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_RcvTafPdpStatusInd: Rab Id is invalid");
        return VOS_ERR;
    }

    if (ADS_PDP_PPP == pstPdpStatusInd->enPdpType)
    {
        enPktType = ADS_CDS_IPF_PKT_TYPE_PPP;
    }

    pstDlRabInfo = ADS_DL_GET_RAB_INFO_PTR(ucInstanceIndex, pstPdpStatusInd->ucRabId);

    /* PDP激活 */
    if (ADS_PDP_STATUS_ACT == pstPdpStatusInd->enPdpStatus)
    {
        /* 设置ADS下行数据回调的RABID */
        pstDlRabInfo->ucRabId           = pstPdpStatusInd->ucRabId;
        pstDlRabInfo->enPktType         = enPktType;
    }
    /* PDP去激活  */
    else if (ADS_PDP_STATUS_DEACT == pstPdpStatusInd->enPdpStatus)
    {
        /* 清除ADS下行数据回调内容 */
        pstDlRabInfo->ucRabId           = ADS_RAB_ID_INVALID;
        pstDlRabInfo->enPktType         = ADS_CDS_IPF_PKT_TYPE_IP;
        pstDlRabInfo->ulExParam         = 0;

        if (ADS_CLEAN_RCV_CB_FUNC_TURE == pstPdpStatusInd->enCleanRcvCbFlag)
        {
            pstDlRabInfo->pRcvDlDataFunc    = VOS_NULL_PTR;
        }
        else
        {
            ADS_NORMAL_LOG(ACPU_PID_ADS_DL, "ADS_DL_RcvTafPdpStatusInd: Not clean ADS DL data call back func");
        }
    }
    else
    {
        ADS_ERROR_LOG(ACPU_PID_ADS_DL, "ADS_DL_RcvTafPdpStatusInd: Pdp Status is invalid");
    }

    return VOS_OK;
}


VOS_UINT32 ADS_DL_RcvCcpuResetStartInd(
    MsgBlock                           *pstMsg
)
{
    VOS_UINT8                           ucIndex;

    ADS_TRACE_HIGH("proc reset msg: enter.\n");

    /* 停止所有启动的定时器 */
    for (ucIndex = 0; ucIndex < ADS_MAX_TIMER_NUM; ucIndex++)
    {
        ADS_StopTimer(ACPU_PID_ADS_DL, ucIndex, ADS_TIMER_STOP_CAUSE_USER);
    }

    /* 重置下行上下文 */
    ADS_ResetDlCtx();

    /* 释放IPF的AD */
    ADS_DL_FreeIpfUsedAd1();
    ADS_DL_FreeIpfUsedAd0();

    /* 释放信号量，使得调用API任务继续运行 */
    VOS_SmV(ADS_GetDLResetSem());

    ADS_TRACE_HIGH("proc reset msg: leave.\n");
    return VOS_OK;
}


VOS_UINT32 ADS_DL_RcvCcpuResetEndInd(
    MsgBlock                           *pstMsg
)
{
    ADS_TRACE_HIGH("proc reset msg: enter.\n");

    /* 重置调试信息 */
    ADS_ResetDebugInfo();

    /* 复位IPF */
    mdrv_ipf_reinit_dlreg();

    /* 重新初始化ADQ */
    ADS_DL_AllocMemForAdq();

    ADS_TRACE_HIGH("proc reset msg: leave.\n");
    return VOS_OK;
}


VOS_UINT32 ADS_DL_RcvTafMsg(MsgBlock* pMsg)
{
    MSG_HEADER_STRU                    *pstMsg;

    pstMsg = (MSG_HEADER_STRU*)pMsg;

    switch(pstMsg->ulMsgName)
    {
        case ID_APS_ADS_PDP_STATUS_IND:
            ADS_DL_RcvTafPdpStatusInd(pMsg);
            break;

        default:
            break;
    }

    return VOS_OK;
}


VOS_UINT32 ADS_DL_RcvCdsMsg(MsgBlock *pMsg)
{
    /* 暂时没有要处理的消息，如果收到消息可能有错误 */
    return VOS_ERR;
}


VOS_UINT32 ADS_DL_RcvAdsDlMsg(MsgBlock *pMsg)
{
    MSG_HEADER_STRU                    *pstMsg;

    pstMsg = (MSG_HEADER_STRU*)pMsg;

    switch(pstMsg->ulMsgName)
    {
        case ID_ADS_CCPU_RESET_START_IND:
            ADS_DL_RcvCcpuResetStartInd(pMsg);
            break;

        case ID_ADS_CCPU_RESET_END_IND:
            ADS_DL_RcvCcpuResetEndInd(pMsg);
            break;

        default:
            ADS_NORMAL_LOG1(ACPU_PID_ADS_DL, "ADS_DL_RcvAdsDlMsg: rcv error msg id %d\r\n", pstMsg->ulMsgName);
            break;
    }

    return VOS_OK;
}


VOS_VOID ADS_DL_ProcMsg(MsgBlock* pMsg)
{
    if (VOS_NULL_PTR == pMsg)
    {
        return;
    }

    /* 消息的分发处理 */
    switch ( pMsg->ulSenderPid )
    {
        /* 来自APS的消息 */
        case I0_WUEPS_PID_TAF:
        case I1_WUEPS_PID_TAF:
        case I2_WUEPS_PID_TAF:
            ADS_DL_RcvTafMsg(pMsg);
            return;

        /* 来自CDS的消息 */
        case UEPS_PID_CDS:
            ADS_DL_RcvCdsMsg(pMsg);
            return;

        /* 来自ADS DL的消息 */
        case ACPU_PID_ADS_DL:
            ADS_DL_RcvAdsDlMsg(pMsg);
            return;

        default:
            return;
    }
}





