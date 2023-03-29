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
#include "PsCommonDef.h"

#include "TafMtcApi.h"





/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_TAF_MTC_API_C


/*****************************************************************************
   2 全局变量定义
*****************************************************************************/
    extern VOS_UINT32 AT_GetDestPid(
        MN_CLIENT_ID_T                      usClientId,
        VOS_UINT32                          ulRcvPid
    );

/*****************************************************************************
   3 函数实现
*****************************************************************************/

VOS_UINT32 MTC_SndMsg(
    VOS_UINT32                          ulTaskId,
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pData,
    VOS_UINT32                          ulLength
)
{
    TAF_PS_MSG_STRU                    *pstMsg   = VOS_NULL_PTR;
    VOS_UINT32                          ulResult;

    ulResult = VOS_OK;

    /* 构造消息 */
    pstMsg = (TAF_PS_MSG_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(
                                UEPS_PID_MTC,
                                sizeof(MSG_HEADER_STRU) + ulLength);
    if (VOS_NULL_PTR == pstMsg)
    {
        return VOS_ERR;
    }

    pstMsg->stHeader.ulReceiverPid      = UEPS_PID_MTC;
    pstMsg->stHeader.ulMsgName          = ulMsgId;

    /* 填写消息内容 */
    TAF_MEM_CPY_S(pstMsg->aucContent, ulLength, pData, ulLength);

    /* 发送消息 */
    ulResult = PS_SEND_MSG(UEPS_PID_MTC, pstMsg);
    if (VOS_OK != ulResult)
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT32 MTC_SetCdmaServiceConnStateInfo(
    TAF_CTRL_STRU                      *pstCtrl,
    TAF_MTC_CDMA_STATE_INFO_STRU       *pstCdmaState
)
{
    TAF_MTC_CDMA_STATE_IND_STRU         stSetCdmaConnSt;
    VOS_UINT32                          ulResult;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stSetCdmaConnSt, sizeof(stSetCdmaConnSt), 0x00, sizeof(TAF_MTC_CDMA_STATE_IND_STRU));

    /* 构造ID_MSG_MTC_CDMA_CONN_STATUS消息 */
    TAF_API_CTRL_HEADER(&stSetCdmaConnSt.stCtrl, pstCtrl->ulModuleId,
                        pstCtrl->usClientId, pstCtrl->ucOpId);

    TAF_MEM_CPY_S(&stSetCdmaConnSt.stCdmaState, sizeof(stSetCdmaConnSt.stCdmaState), pstCdmaState, sizeof(TAF_MTC_CDMA_STATE_INFO_STRU));

    /* 发送消息 */
    ulResult = MTC_SndMsg(UEPS_PID_MTC,
                         ID_MSG_MTC_CDMA_CONN_STATE_IND,
                         &stSetCdmaConnSt,
                         sizeof(TAF_MTC_CDMA_STATE_IND_STRU));

    return ulResult;
}


VOS_UINT32 MTC_SetModemServiceConnState(
    TAF_CTRL_STRU                      *pstCtrl,
    TAF_MTC_SRV_CONN_STATE_INFO_STRU   *pstModemConnSt
)
{
    TAF_MTC_MODEM_CONN_STATUS_IND_STRU  stModemConnSt;
    VOS_UINT32                          ulResult;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stModemConnSt, sizeof(stModemConnSt), 0x00, sizeof(TAF_MTC_MODEM_CONN_STATUS_IND_STRU));

    /* 构造ID_MSG_MTC_CDMA_CONN_STATUS消息 */
    TAF_API_CTRL_HEADER(&(stModemConnSt.stCtrl), pstCtrl->ulModuleId, pstCtrl->usClientId, pstCtrl->ucOpId);

    TAF_MEM_CPY_S(&stModemConnSt.stModemConnStateInfo, sizeof(stModemConnSt.stModemConnStateInfo), pstModemConnSt, sizeof(TAF_MTC_SRV_CONN_STATE_INFO_STRU));

    /* 发送消息 */
    ulResult = MTC_SndMsg(UEPS_PID_MTC,
                         ID_MSG_MTC_MODEM_SERVICE_CONN_STATE_IND,
                         &stModemConnSt,
                         sizeof(TAF_MTC_MODEM_CONN_STATUS_IND_STRU));

    return ulResult;
}


VOS_UINT32 MTC_SetModemUsimmState(
    TAF_CTRL_STRU                           *pstCtrl,
    TAF_MTC_USIMM_CARD_SERVIC_ENUM_UINT16    enUsimState,
    TAF_MTC_USIMM_CARD_SERVIC_ENUM_UINT16    enCsimState
)
{
    TAF_MTC_USIMM_STATUS_IND_STRU       stUsimmState;
    VOS_UINT32                          ulResult;

    /* 初始化 */
    ulResult = VOS_OK;
    TAF_MEM_SET_S(&stUsimmState, sizeof(stUsimmState), 0x00, sizeof(TAF_MTC_USIMM_STATUS_IND_STRU));

    /* 构造ID_MSG_MTC_CDMA_CONN_STATUS消息 */
    TAF_API_CTRL_HEADER(&(stUsimmState.stCtrl), pstCtrl->ulModuleId, pstCtrl->usClientId, pstCtrl->ucOpId);

    stUsimmState.enUsimState = enUsimState;
    stUsimmState.enCsimState = enCsimState;

    /* 发送消息 */
    ulResult = MTC_SndMsg(UEPS_PID_MTC,
                          ID_MSG_MTC_USIMM_STATE_IND,
                          &stUsimmState,
                          sizeof(TAF_MTC_USIMM_STATUS_IND_STRU));

    return ulResult;
}


VOS_VOID MTC_SetBeginSessionInfo(
    TAF_CTRL_STRU                      *pstCtrl,
    MTC_SESSION_TYPE_ENUM_UINT8         enSessionType
)
{
    MTC_BEGIN_SESSION_IND_STRU          stSessionInfo;

    /* 初始化 */
    TAF_MEM_SET_S(&stSessionInfo, sizeof(stSessionInfo), 0x00, sizeof(MTC_BEGIN_SESSION_IND_STRU));

    /* 构造ID_MSG_MTC_BEGIN_SESSION_IND消息 */
    TAF_API_CTRL_HEADER(&(stSessionInfo.stCtrl), pstCtrl->ulModuleId, pstCtrl->usClientId, pstCtrl->ucOpId);

    stSessionInfo.enSessionType = enSessionType;

    /* 发送消息 */
    MTC_SndMsg(UEPS_PID_MTC,
               ID_MSG_MTC_BEGIN_SESSION_IND,
               &stSessionInfo,
               sizeof(MTC_BEGIN_SESSION_IND_STRU));

    return;
}


VOS_VOID MTC_SetEndSessionInfo(
    TAF_CTRL_STRU                      *pstCtrl,
    MTC_SESSION_TYPE_ENUM_UINT8         enSessionType
)
{
    MTC_END_SESSION_IND_STRU            stEndSessionInfo;

    /* 初始化 */
    TAF_MEM_SET_S(&stEndSessionInfo, sizeof(stEndSessionInfo), 0x00, sizeof(MTC_END_SESSION_IND_STRU));

    /* 构造ID_MSG_MTC_END_SESSION_IND消息 */
    TAF_API_CTRL_HEADER(&(stEndSessionInfo.stCtrl), pstCtrl->ulModuleId, pstCtrl->usClientId, pstCtrl->ucOpId);

    stEndSessionInfo.enSessionType  = enSessionType;
    stEndSessionInfo.enCsRelAll     = VOS_FALSE;
    stEndSessionInfo.enPsRelAll     = VOS_FALSE;

    /* 发送消息 */
    MTC_SndMsg(UEPS_PID_MTC,
               ID_MSG_MTC_END_SESSION_IND,
               &stEndSessionInfo,
               sizeof(MTC_BEGIN_SESSION_IND_STRU));

    return;
}


VOS_VOID MTC_SetPowerSaveInfo(
    VOS_UINT32                          ulSndPid,
    TAF_MTC_POWER_SAVE_ENUM_UINT8       enPowerSaveStatus
)
{
    MTC_POWER_SAVE_IND_STRU             stPowerSaveInfo;

    /* 初始化 */
    TAF_MEM_SET_S(&stPowerSaveInfo, sizeof(stPowerSaveInfo), 0x00, sizeof(MTC_POWER_SAVE_IND_STRU));

    /* 构造ID_MSG_MTC_POWER_SAVE_IND消息 */
    stPowerSaveInfo.stCtrl.ulModuleId = ulSndPid;
    stPowerSaveInfo.enPowerSaveStatus = enPowerSaveStatus;

    /* 发送消息 */
    MTC_SndMsg(UEPS_PID_MTC,
               ID_MSG_MTC_POWER_SAVE_IND,
               &stPowerSaveInfo,
               sizeof(MTC_POWER_SAVE_IND_STRU));

    return;
}


VOS_VOID MTC_SetRatModeInfo(
    TAF_CTRL_STRU                      *pstCtrl,
    MTC_RATMODE_ENUM_UINT8              enRatMode
)
{
    MTC_RAT_MODE_IND_STRU               stRatModeInfo;

    /* 初始化 */
    TAF_MEM_SET_S(&stRatModeInfo, sizeof(stRatModeInfo), 0x00, sizeof(MTC_RAT_MODE_IND_STRU));

    /* 构造ID_MSG_MTC_RAT_MODE_IND消息 */
    TAF_API_CTRL_HEADER(&(stRatModeInfo.stCtrl), pstCtrl->ulModuleId, pstCtrl->usClientId, pstCtrl->ucOpId);

    stRatModeInfo.enRatMode = enRatMode;

    /* 发送消息 */
    MTC_SndMsg(UEPS_PID_MTC,
               ID_MSG_MTC_RAT_MODE_IND,
               &stRatModeInfo,
               (VOS_UINT32)sizeof(MTC_RAT_MODE_IND_STRU));

    return;
}





