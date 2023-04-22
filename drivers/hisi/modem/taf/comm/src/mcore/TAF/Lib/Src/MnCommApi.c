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
#include "MnCommApi.h"
#include "AtMnInterface.h"



#define    THIS_FILE_ID        PS_FILE_ID_MN_COMM_API_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


/*****************************************************************************
  3 函数实现
*****************************************************************************/
extern VOS_UINT32 AT_GetDestPid(
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT32                          ulRcvPid
);



VOS_VOID MN_GetAppReqMsgLen(
    VOS_UINT32 ulParaLen,
    VOS_UINT32 *pulMsgLen
)
{
    VOS_UINT32 ulMsgLen;

    if (VOS_NULL_PTR == pulMsgLen)
    {
        return;
    }

    if (ulParaLen <= 4)
    {
        ulMsgLen = sizeof(MN_APP_REQ_MSG_STRU);
    }
    else
    {
        ulMsgLen = (sizeof(MN_APP_REQ_MSG_STRU) + ulParaLen) - 4;
    }

    *pulMsgLen = ulMsgLen;

    return;
}


VOS_VOID  MN_FillAppReqMsgHeader(
    MN_APP_REQ_MSG_STRU                 *pMsg,
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    VOS_UINT16                          usMsgType,
    VOS_UINT32                          ulReceiverPid
 )
{
    MN_APP_REQ_MSG_STRU *pAppMsgHeader;

    pAppMsgHeader                       = pMsg;
    pAppMsgHeader->clientId             = ClientId;
    pAppMsgHeader->opId                 = OpId;
    pAppMsgHeader->usMsgName            = usMsgType;
    /* pAppMsgHeader->ulSenderCpuId        = VOS_LOCAL_CPUID; */
    /* pAppMsgHeader->ulReceiverCpuId      = VOS_LOCAL_CPUID; */
    /* pAppMsgHeader->ulSenderPid          = WUEPS_PID_AT; */
    pAppMsgHeader->ulReceiverPid        = AT_GetDestPid(ClientId, ulReceiverPid);

    return;
}


VOS_VOID  MN_FillAppReqMsgPara(
    VOS_VOID                            *pSndMsgPara,
    VOS_VOID                            *pPara,
    VOS_UINT32                          ulLen
)
{
    VOS_UINT32                          ulMsgLen;
    if (VOS_NULL_PTR == pSndMsgPara)
    {
        return;
    }

    if (ulLen > 0)
    {
        MN_GetAppReqMsgLen(ulLen, &ulMsgLen);
        TAF_MEM_CPY_S(pSndMsgPara, ulMsgLen - (sizeof(MN_APP_REQ_MSG_STRU) - 4), pPara, ulLen);
    }

    return;
}



VOS_UINT32  MN_FillAndSndAppReqMsg(
    MN_CLIENT_ID_T                      ClientId,
    MN_OPERATION_ID_T                   OpId,
    VOS_UINT16                          usMsgType,
    VOS_VOID                            *pPara,
    VOS_UINT32                          ulParaLen,
    VOS_UINT32                          ulReceiverPid
)
{
    VOS_UINT8                          *pMsg;
    VOS_UINT32                          ulRet;
    VOS_UINT32                          ulMsgLen;
    VOS_UINT8                          *pMsgPara;

    if ((VOS_NULL_PTR == pPara) && (0 < ulParaLen))
    {
        return TAF_FAILURE;
    }

    pMsgPara = (VOS_UINT8 *)pPara;

    /* 获取消息长度 */
    MN_GetAppReqMsgLen( ulParaLen, &ulMsgLen);

    /* 申请消息 */
    pMsg = (VOS_UINT8 *)PS_ALLOC_MSG(WUEPS_PID_AT, ulMsgLen - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pMsg)
    {
        return TAF_FAILURE;
    }

    TAF_MEM_SET_S( (pMsg + VOS_MSG_HEAD_LENGTH), (ulMsgLen - VOS_MSG_HEAD_LENGTH), 0x00, (ulMsgLen - VOS_MSG_HEAD_LENGTH));

    /* 填充消息头 */
    MN_FillAppReqMsgHeader((MN_APP_REQ_MSG_STRU *)pMsg, ClientId, OpId, usMsgType, ulReceiverPid );

    /* 填充消息参数 */
    MN_FillAppReqMsgPara( &pMsg[sizeof(MN_APP_REQ_MSG_STRU) - 4], pMsgPara, ulParaLen );

    ulRet = PS_SEND_MSG(WUEPS_PID_AT, pMsg);

    if (VOS_OK != ulRet)
    {
        return TAF_FAILURE;
    }

    return TAF_SUCCESS;
}




