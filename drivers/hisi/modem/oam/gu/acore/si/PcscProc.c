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
#include "PcscProc.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID PS_FILE_ID_PCSC_APP_PROC_C

/* ACPU上维护卡状态的全局变量*/
USIMM_CARDAPP_SERVIC_ENUM_UINT32 g_enAcpuCardStatus = USIMM_CARDAPP_SERVIC_BUTT;

extern VOS_VOID OM_RecordInfoStart(VOS_EXC_DUMP_MEM_NUM_ENUM_UINT32 enNumber, VOS_UINT32 ulSendPid, VOS_UINT32 ulRcvPid, VOS_UINT32 ulMsgName);
extern VOS_VOID OM_RecordInfoEnd(VOS_EXC_DUMP_MEM_NUM_ENUM_UINT32 enNumber);


VOS_UINT32 PCSC_AcpuCmdReq(VOS_UINT32 ulCmdType, VOS_UINT8 *pucAPDU, VOS_UINT32 ulAPDULen)
{
    SI_PIH_PCSC_REQ_STRU     *pstMsg;

    if((ulAPDULen > 5)&&(pucAPDU[4] != ulAPDULen-5))
    {
        PS_LOG(ACPU_PID_PCSC, 0, PS_PRINT_ERROR, "PCSC_AcpuCmdReq: Data Len is Not Eq P3");
        return VOS_ERR;
    }

    pstMsg = (SI_PIH_PCSC_REQ_STRU *)VOS_AllocMsg(ACPU_PID_PCSC,
                        (sizeof(SI_PIH_PCSC_REQ_STRU) - VOS_MSG_HEAD_LENGTH) + ulAPDULen);
    if (VOS_NULL_PTR == pstMsg)
    {
        /* 打印错误 */
        PS_LOG(ACPU_PID_PCSC, 0, PS_PRINT_WARNING, "PCSC_AcpuCmdReq: VOS_AllocMsg is Failed");
        (VOS_VOID)vos_printf("PCSC_AcpuCmdReq: VOS_AllocMsg is Failed.\r\n");

        return VOS_ERR; /* 返回函数错误信息 */
    }

    pstMsg->stMsgHeader.ulReceiverPid = MAPS_PIH_PID;
    pstMsg->stMsgHeader.ulMsgName     = SI_PIH_PCSC_DATA_REQ;
    pstMsg->stMsgHeader.ulEventType   = ulCmdType;
    pstMsg->stMsgHeader.usClient      = 0xFFFF;
    pstMsg->ulCmdType                 = ulCmdType;
    pstMsg->ulCmdLen                  = ulAPDULen;

    if(ulAPDULen != 0)
    {
        PAM_MEM_CPY_S(pstMsg->aucAPDU,
                      ulAPDULen,
                      pucAPDU,
                      ulAPDULen);
    }

    if (VOS_OK != VOS_SendMsg(ACPU_PID_PCSC, pstMsg))
    {
        /*打印错误*/
        PS_LOG(ACPU_PID_PCSC, 0, PS_PRINT_WARNING, "PCSC_AcpuCmdReq: VOS_SendMsg is Failed.");
        (VOS_VOID)vos_printf("PCSC_AcpuCmdReq: VOS_SendMsg is Failed.");
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_INT PCSC_AcpuGetCardStatus(VOS_VOID)
{
    if (USIMM_CARDAPP_SERVIC_ABSENT == g_enAcpuCardStatus)
    {
        /*上报无卡状态*/
        return VOS_ERROR;
    }
    /*上报有卡状态*/
    return VOS_OK;
}



VOS_VOID PCSC_UpdateCardStatus(USIMM_CARDSTATUS_IND_STRU *pstMsg)
{
    if (USIMM_CARDAPP_SERVIC_BUTT == g_enAcpuCardStatus)
    {
        (VOS_VOID)vos_printf("Reg PCSC Func.\r\n");
    }

    /*更新本地卡状态的全局变量*/
    g_enAcpuCardStatus = pstMsg->stUsimSimInfo.enCardAppService;

    (VOS_VOID)vos_printf("Update Card Status: %d .\r\n", g_enAcpuCardStatus);


    return;
}


VOS_VOID  PCSC_AcpuMsgProc( MsgBlock *pMsg)
{
    SI_PIH_PCSC_CNF_STRU *pstPCSCMsg;

    pstPCSCMsg = (SI_PIH_PCSC_CNF_STRU*)pMsg;

    OM_RecordInfoStart(VOS_EXC_DUMP_MEM_NUM_1, pMsg->ulSenderPid, ACPU_PID_PCSC, *((VOS_UINT32*)pMsg->aucValue));

    switch(pstPCSCMsg->ulMsgName)
    {
        case SI_PIH_PCSC_DATA_CNF:
            break;
        case USIMM_CARDSTATUS_IND:
            PCSC_UpdateCardStatus((USIMM_CARDSTATUS_IND_STRU *)pMsg);
            break;
        default:
            PS_LOG(ACPU_PID_PCSC, 0, PS_PRINT_WARNING, "PCSC_AcpuMsgProc: unknow MsgType");
            break;
    }

    OM_RecordInfoEnd(VOS_EXC_DUMP_MEM_NUM_1);

    return;
}


VOS_UINT32 TestSendPcscCmd(VOS_UINT32 ulCmdType)
{
    VOS_UINT8 aucApduData[5];

    aucApduData[4] = '\0';

    return PCSC_AcpuCmdReq(ulCmdType,aucApduData,5);
}


