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
#include "TafAppXsmsInterface.h"
#include "MnClient.h"
#include "TafNvInterface.h"
#include "NVIM_Interface.h"



#define THIS_FILE_ID                    PS_FILE_ID_TAF_XSMS_API_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数定义
*****************************************************************************/

VOS_UINT32 TAF_XSMS_GetReceiverPid(MN_CLIENT_ID_T  ClientId, VOS_UINT32 *pulReceiverPid)
{
    MODEM_ID_ENUM_UINT16    enModemID;

    /* 调用接口获取Modem ID */
    if (VOS_OK != AT_GetModemIdFromClient(ClientId, &enModemID))
    {
        return VOS_ERR;
    }

    if (MODEM_ID_1 == enModemID)
    {
        *pulReceiverPid = I1_UEPS_PID_XSMS;
    }
    else if (MODEM_ID_2 == enModemID)
    {
        *pulReceiverPid = I2_UEPS_PID_XSMS;
    }
    else
    {
        *pulReceiverPid = I0_UEPS_PID_XSMS;
    }

    return VOS_OK;
}



VOS_UINT32 TAF_XSMS_SendSmsReq(
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_XSMS_SEND_OPTION_ENUM_UINT8     enSndOption,
    VOS_UINT8                          *pucData)
{

    TAF_XSMS_SEND_MSG_REQ_STRU         *pstMsg;
    VOS_UINT32                          ulReceiverPid;


    if (VOS_OK != TAF_XSMS_GetReceiverPid(usClientId, &ulReceiverPid))
    {
        return VOS_ERR;
    }

    if (VOS_NULL_PTR == pucData)
    {
        return VOS_ERR;
    }

    pstMsg = (TAF_XSMS_SEND_MSG_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(TAF_XSMS_SEND_MSG_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_ERR;
    }

    pstMsg->ulMsgName     = TAF_XSMS_APP_MSG_TYPE_SEND_REQ;
    pstMsg->ulReceiverPid = ulReceiverPid;
    pstMsg->usClientId    = usClientId;
    pstMsg->ucOpId        = ucOpId;
    pstMsg->enSndOption   = enSndOption;

    TAF_MEM_CPY_S(&pstMsg->st1XSms, sizeof(pstMsg->st1XSms), pucData, sizeof(TAF_XSMS_MESSAGE_STRU));

    if (VOS_OK == VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        return VOS_OK;
    }
    else
    {
        return VOS_ERR;
    }
}


VOS_UINT32 TAF_XSMS_SetXsmsApMemFullReq(
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    VOS_UINT8                           ucApMemFullFlag)
{
    TAF_XSMS_APP_MSG_SET_AP_MEM_FULL_REQ_STRU               *pstMsg;
    VOS_UINT32                                               ulReceiverPid;

    if (VOS_OK != TAF_XSMS_GetReceiverPid(usClientId, &ulReceiverPid))
    {
        return VOS_ERR;
    }

    if (TAF_XSMS_AP_MEM_BUTT <= ucApMemFullFlag)
    {
        return VOS_ERR;
    }

    pstMsg = (TAF_XSMS_APP_MSG_SET_AP_MEM_FULL_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT,
              sizeof(TAF_XSMS_APP_MSG_SET_AP_MEM_FULL_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_ERR;
    }

    pstMsg->ulMsgName       = TAF_XSMS_APP_MSG_TYPE_UIM_MEM_SET_REQ;
    pstMsg->ulReceiverPid   = ulReceiverPid;
    pstMsg->usClientId      = usClientId;
    pstMsg->ucOpId          = ucOpId;
    pstMsg->enApMemFullFlag = ucApMemFullFlag;

    if (VOS_OK == VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        return VOS_OK;
    }
    else
    {
        return VOS_ERR;
    }
}



VOS_UINT32 TAF_XSMS_WriteSmsReq(
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    TAF_XSMS_STATUS_ENUM_UINT8          enStatus,
    VOS_UINT8                          *pucData)
{
    TAF_XSMS_WRITE_MSG_REQ_STRU        *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != TAF_XSMS_GetReceiverPid(usClientId, &ulReceiverPid))
    {
        return VOS_ERR;
    }

    if (VOS_NULL_PTR == pucData)
    {
        return VOS_ERR;
    }

    pstMsg = (TAF_XSMS_WRITE_MSG_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(TAF_XSMS_WRITE_MSG_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_ERR;
    }

    pstMsg->ulMsgName     = TAF_XSMS_APP_MSG_TYPE_WRITE_REQ;
    pstMsg->ulReceiverPid = ulReceiverPid;
    pstMsg->usClientId    = usClientId;
    pstMsg->ucOpId        = ucOpId;
    pstMsg->enSmsStatus   = enStatus;

    TAF_MEM_CPY_S(&pstMsg->st1XSms, sizeof(pstMsg->st1XSms), pucData, sizeof(TAF_XSMS_MESSAGE_STRU));

    if (VOS_OK == VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        return VOS_OK;
    }
    else
    {
        return VOS_ERR;
    }
}


VOS_UINT32 TAF_XSMS_DeleteSmsReq(
    MN_CLIENT_ID_T                      usClientId,
    MN_OPERATION_ID_T                   ucOpId,
    VOS_UINT8                           ucIndex)
{
    TAF_XSMS_DELETE_MSG_REQ_STRU       *pstMsg;
    VOS_UINT32                          ulReceiverPid;

    if (VOS_OK != TAF_XSMS_GetReceiverPid(usClientId, &ulReceiverPid))
    {
        return VOS_ERR;
    }

    pstMsg = (TAF_XSMS_DELETE_MSG_REQ_STRU *)VOS_AllocMsg(WUEPS_PID_AT, sizeof(TAF_XSMS_DELETE_MSG_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_ERR;
    }

    pstMsg->ulMsgName     = TAF_XSMS_APP_MSG_TYPE_DELETE_REQ;
    pstMsg->ulReceiverPid = ulReceiverPid;
    pstMsg->usClientId    = usClientId;
    pstMsg->ucOpId        = ucOpId;
    pstMsg->ucIndex       = ucIndex;

    if (VOS_OK == VOS_SendMsg(WUEPS_PID_AT, pstMsg))
    {
        return VOS_OK;
    }
    else
    {
        return VOS_ERR;
    }
}






