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



#include "diag_msghifi.h"
#include "diag_debug.h"
#include "diag_common.h"

#define    THIS_FILE_ID        MSP_FILE_ID_DIAG_MSGHIFI_C



VOS_UINT32 diag_HifiMsgProc(DIAG_FRAME_INFO_STRU* pstReq)
{
    VOS_UINT32 ulRet                    = ERR_MSP_SUCCESS;
    DIAG_COMM_CNF_STRU stHifiCnf        = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo   = {0};
    
    VOS_UINT32 ulLen;
    DIAG_HIFI_MSG_A_TRANS_C_STRU *pstInfo;

    ulLen = sizeof(DIAG_HIFI_MSG_A_TRANS_C_STRU) - VOS_MSG_HEAD_LENGTH + pstReq->ulMsgLen;
    pstInfo = (DIAG_HIFI_MSG_A_TRANS_C_STRU*)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT, ulLen);
    if(VOS_NULL == pstInfo)
    {
        ulRet = ERR_MSP_MALLOC_FAILUE;
        goto DIAG_ERROR;
    }
    
    pstInfo->ulReceiverPid = MSP_PID_DIAG_AGENT;
    pstInfo->ulSenderPid   = MSP_PID_DIAG_APP_AGENT;
    pstInfo->ulMsgId       = DIAG_MSG_HIFI_A_TRANS_C_REQ;
    
    ulLen = sizeof(DIAG_FRAME_INFO_STRU) + pstReq->ulMsgLen;
    (VOS_VOID)VOS_MemCpy_s(&pstInfo->stInfo, ulLen, pstReq, ulLen);
    
    ulRet = VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstInfo);
    if(ulRet)
    {
        goto DIAG_ERROR;
    }

    return ulRet;
    
DIAG_ERROR:
    

    stHifiCnf.ulRet  = ulRet;
    DIAG_MSG_COMMON_PROC(stDiagInfo, stHifiCnf, pstReq);

    stDiagInfo.ulMsgType = pstReq->stID.pri4b;

    /*组包回复*/
    ulRet = DIAG_MsgReport(&stDiagInfo, &stHifiCnf, sizeof(stHifiCnf));

    return ulRet;
}


/*****************************************************************************
 Function Name   : diag_HifiMsgInit
 Description     : MSG HIFI部分初始化
 Input           : None
 Output          : None
 Return          : None
 History         :

*****************************************************************************/
VOS_VOID diag_HifiMsgInit(VOS_VOID)
{
    /*注册message消息回调*/
    DIAG_MsgProcReg(DIAG_MSG_TYPE_AUDIO, diag_HifiMsgProc);
}



