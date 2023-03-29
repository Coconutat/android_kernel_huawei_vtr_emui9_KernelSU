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




#include <mdrv.h>
#include "diag_msgapplog.h"
#include "diag_debug.h"
#include "msp_diag_comm.h"


#define    THIS_FILE_ID        MSP_FILE_ID_DIAG_MSGAPPLOG_C

DIAG_APPLOG_PROC_FUN_STRU g_DiagAppLogFunc[] = {
  {diag_AppLogDataConfig       ,DIAG_CMD_APPLOG_CONFIG   ,0}};

 VOS_UINT32 diag_MspMsgProc(DIAG_FRAME_INFO_STRU *pData);
  
/*****************************************************************************
 Function Name   : diag_AppLogMsgProc
 Description     : APPLOG  消息处理
 Input           : None
 Output          : None
 Return          : None
 History         :

*****************************************************************************/
VOS_UINT32 diag_AppLogDataConfig(VOS_UINT8* pstReq)
{
    VOS_UINT32 ulRet = ERR_MSP_SUCCESS;
    DIAG_FRAME_INFO_STRU *pData;
    DIAG_APPLOG_CFG_REQ  *plogcfg;
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_APPLOG_CFG_CNF stAppLog = {0};
   
    pData = (DIAG_FRAME_INFO_STRU *)pstReq;
    plogcfg=(DIAG_APPLOG_CFG_REQ *)(pData->aucData+APPLOG_DATA_OFFSET);
   
    DIAG_MSG_COMMON_PROC(stDiagInfo, stAppLog, pData);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_APP;

    stAppLog.ulRc  = mdrv_applog_cfg(plogcfg->enable,plogcfg->level);

    /*组包给FW回复*/
    ulRet = DIAG_MsgReport(&stDiagInfo, &stAppLog, sizeof(stAppLog));
    if(ulRet)
    {
       diag_printf("Rcv  Msg Id fail!\n");          
    }
    return ulRet;
}
/*****************************************************************************
 Function Name   : diag_AppLogMsgProc
 Description     : APPLOG处理消息处理
 Input           : None
 Output          : None
 Return          : None
 History         :

*****************************************************************************/
VOS_UINT32 diag_AppLogMsgProc(DIAG_FRAME_INFO_STRU *pData)
{
    VOS_UINT32 ulRet = ERR_MSP_INVALID_PARAMETER ;
    VOS_UINT32 i = 0;
    if(DIAG_MSG_TYPE_APP != pData->stID.pri4b)
    {
        diag_printf("%s Rcv Error Msg Id 0x%x\n",__FUNCTION__,pData->ulCmdId);
        return ulRet;
    }

    for(i = 0; i< sizeof(g_DiagAppLogFunc)/sizeof(g_DiagAppLogFunc[0]);i++)
    {
        if(g_DiagAppLogFunc[i].ulCmdId == pData->ulCmdId)
        {
            g_DiagAppLogFunc[i].ulReserve ++;
            ulRet = g_DiagAppLogFunc[i].pFunc((VOS_UINT8*)pData);
            return ulRet;
        }
    }

    return ulRet;
}

/*****************************************************************************
 Function Name   : diag_AppLogMsgInit
 Description     : APP LOG初始化
 Input           : None
 Output          : None
 Return          : None
 History         :

*****************************************************************************/
VOS_VOID diag_AppLogMsgInit(VOS_VOID)
{
    /*注册message消息回调*/
    DIAG_MsgProcReg(DIAG_MSG_TYPE_APP,diag_AppLogMsgProc);
}




