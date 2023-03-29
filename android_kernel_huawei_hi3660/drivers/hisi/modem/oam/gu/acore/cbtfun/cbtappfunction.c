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
#include "NVIM_Interface.h"
#include "TafNvInterface.h"
#include "omprivate.h"
#include "cbtapprl.h"
#include "diag_api.h"
#include "CbtPpm.h"
#include "mdrv.h"




#define    THIS_FILE_ID        PS_FILE_ID_APP_CBT_FUNC_C


extern VOS_UINT32                       g_ulCbtMsgSN;
extern CBT_RCV_CHAN_CTRL_INFO_STRU      g_stAcpuCbtCtrlInfo;

extern VOS_UINT32 OM_IsAcpuAuthNv(VOS_UINT16 usNvId);

extern VOS_UINT32 CBTSCM_SoftDecodeReqRcvTaskInit(VOS_VOID);


VOS_UINT32 CBT_AcpuReadNv(CBT_UNIFORM_MSG_STRU *pstAppToCbtMsg, VOS_UINT16 usReturnPrimId)
{
    CBT_UNIFORM_MSG_STRU               *pstCbtToAppMsg;
    CBT_READ_NV_REQ_STRU               *pstAppToCbtReadNv;
    CBT_READ_NV_IND_STRU               *pstCbtToAppReadNv;
    CBT_READ_NV_CNF_STRU                stCbtReadNvCnf;
    VOS_UINT16                         *pusCbtToAppPara;
    VOS_UINT32                          ulCount;
    VOS_UINT32                          ulIndex;
    VOS_UINT32                          ulTotalSize = 0;
    VOS_UINT32                          ulResult;
    VOS_UINT16                          usNvId;
    VOS_UINT32                          ulNvLen;

    pstAppToCbtReadNv = (CBT_READ_NV_REQ_STRU *)(pstAppToCbtMsg->aucPara);
    ulCount = pstAppToCbtReadNv->ulCount;

    /*Get the total length of all NV items.*/
    for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
    {
        usNvId = pstAppToCbtReadNv->ausNvItemId[ulIndex];
        ulResult = NV_GetLength(usNvId, &ulNvLen);
        if (VOS_OK != ulResult)
        {
            stCbtReadNvCnf.ulErrorCode = ulResult;
            stCbtReadNvCnf.ulErrNvId   = (VOS_UINT32)usNvId;
            stCbtReadNvCnf.ulCount     = ulCount;
            stCbtReadNvCnf.ulMsgLength = CBT_READ_NV_HEAD_SIZE;
            CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, usReturnPrimId, (CBT_UNIFORM_MSG_STRU *)&stCbtReadNvCnf);

            return ulResult;
        }
        ulTotalSize += ulNvLen;
    }

    /*Allocate the memory space.*/
    ulTotalSize += CBT_MSG_HEAD_EX_LENGTH + CBT_READ_NV_HEAD_SIZE + (ulCount*CBT_NV_ITEM_SIZE);
    pstCbtToAppMsg = (CBT_UNIFORM_MSG_STRU *)VOS_AssistantMemAlloc(ACPU_PID_CBT,
                                                   DYNAMIC_MEM_PT, ulTotalSize);
    if (VOS_NULL_PTR == pstCbtToAppMsg)
    {
        CBT_AcpuSendResultChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, usReturnPrimId, VOS_ERR);

        return VOS_ERR;
    }
    /*Assign the return value and the count to struct's fields.*/
    pstCbtToAppReadNv = (CBT_READ_NV_IND_STRU *)(pstCbtToAppMsg->aucPara);
    pstCbtToAppReadNv->ulErrorCode = VOS_OK;
    pstCbtToAppReadNv->ulErrNvId   = 0;
    pstCbtToAppReadNv->ulCount     = ulCount;

    pusCbtToAppPara = (VOS_UINT16*)(pstCbtToAppReadNv->ausNVItemData);
    /*Read the NV content by the NV Id.*/
    for(ulIndex = 0; ulIndex < ulCount; ulIndex++)
    {
        usNvId = pstAppToCbtReadNv->ausNvItemId[ulIndex];
        ulResult = NV_GetLength(usNvId, &ulNvLen);
        if (VOS_OK != ulResult)
        {
            stCbtReadNvCnf.ulErrorCode = ulResult;
            stCbtReadNvCnf.ulErrNvId   = (VOS_UINT32)usNvId;
            stCbtReadNvCnf.ulCount     = ulCount;
            stCbtReadNvCnf.ulMsgLength = CBT_READ_NV_HEAD_SIZE;
            CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, usReturnPrimId, (CBT_UNIFORM_MSG_STRU *)&stCbtReadNvCnf);
            (VOS_VOID)VOS_MemFree(ACPU_PID_CBT, pstCbtToAppMsg);

            return ulResult;
        }

        *pusCbtToAppPara = usNvId;
        pusCbtToAppPara++;
        *pusCbtToAppPara = (VOS_UINT16)ulNvLen;
        pusCbtToAppPara++;

        ulResult = NV_ReadEx(pstAppToCbtMsg->stMsgHeader.stModemSsid.ucModem,
                            usNvId, pusCbtToAppPara, ulNvLen);
        if (NV_OK != ulResult)
        {
            stCbtReadNvCnf.ulErrorCode    = ulResult;
            stCbtReadNvCnf.ulErrNvId      = (VOS_UINT32)usNvId;
            stCbtReadNvCnf.ulCount        = ulCount;
            stCbtReadNvCnf.ulMsgLength    = CBT_READ_NV_HEAD_SIZE;
            CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, usReturnPrimId, (CBT_UNIFORM_MSG_STRU *)&stCbtReadNvCnf);
            (VOS_VOID)VOS_MemFree(ACPU_PID_CBT, pstCbtToAppMsg);

            return ulResult;
        }
        pusCbtToAppPara += ulNvLen/sizeof(VOS_UINT16);
    }

    pstCbtToAppMsg->ulMsgLength = ulTotalSize - CBT_MSG_HEAD_EX_LENGTH;
    CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, usReturnPrimId, pstCbtToAppMsg);
    (VOS_VOID)VOS_MemFree(ACPU_PID_CBT, pstCbtToAppMsg);

    return VOS_OK;
}


VOS_UINT32 CBT_AcpuWriteNv(CBT_UNIFORM_MSG_STRU *pstAppToCbtMsg, VOS_UINT16 usReturnPrimId)
{
    CBT_WRITE_NV_REQ_STRU   *pstAppCbtWriteNv;
    CBT_WRITE_NV_CNF_STRU    stCbtWriteNvCnf;
    VOS_UINT16              *pusAppToCbtPara;
    VOS_UINT32               ulCount;
    VOS_UINT32               ulIndex;
    VOS_UINT16               usNvId;
    VOS_UINT16               usNvLen;
    VOS_UINT32               ulResult;

    pstAppCbtWriteNv = (CBT_WRITE_NV_REQ_STRU *)(pstAppToCbtMsg->aucPara);
    /*Get the number of all NV Id.*/
    ulCount = pstAppCbtWriteNv->ulCount;

    pusAppToCbtPara = (VOS_UINT16*)(pstAppCbtWriteNv->ausNvItemData);
    /*Write the NV content by NV Id.*/
    for (ulIndex = 0; ulIndex < ulCount; ulIndex++)
    {
        usNvId  = *pusAppToCbtPara;
        pusAppToCbtPara++;
        usNvLen = *pusAppToCbtPara;
        pusAppToCbtPara++;

        ulResult = NV_WriteEx(pstAppToCbtMsg->stMsgHeader.stModemSsid.ucModem,
                                usNvId, pusAppToCbtPara, (VOS_UINT32)usNvLen);
        if(NV_OK != ulResult)
        {
            stCbtWriteNvCnf.ulErrorCode    = ulResult;
            stCbtWriteNvCnf.ulErrNvId      = (VOS_UINT16)usNvId;
            stCbtWriteNvCnf.ulMsgLength    = CBT_WRITE_NV_HEAD_SIZE;
            CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, \
                                usReturnPrimId, (CBT_UNIFORM_MSG_STRU *)&stCbtWriteNvCnf);

            return VOS_ERR;
        }
        /*由于返回的usNvLen以byte为单位，所以需要除以指针指向类型的大小*/
        pusAppToCbtPara += (usNvLen/sizeof(VOS_UINT16));
    }

    ulResult = NV_Flush();
    if(NV_OK != ulResult)
    {
        stCbtWriteNvCnf.ulErrorCode    = ulResult;
        stCbtWriteNvCnf.ulErrNvId      = (VOS_UINT16)0xFFFF;
        stCbtWriteNvCnf.ulMsgLength    = CBT_WRITE_NV_HEAD_SIZE;
        CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, \
                            usReturnPrimId, (CBT_UNIFORM_MSG_STRU *)&stCbtWriteNvCnf);

        return VOS_ERR;
    }

    stCbtWriteNvCnf.ulErrorCode    = NV_OK;
    stCbtWriteNvCnf.ulErrNvId      = 0;
    stCbtWriteNvCnf.ulMsgLength    = CBT_WRITE_NV_HEAD_SIZE;
    CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, \
                        usReturnPrimId, (CBT_UNIFORM_MSG_STRU *)&stCbtWriteNvCnf);

    return VOS_OK;
}


VOS_UINT32 CBT_AcpuEstablishProc(CBT_UNIFORM_MSG_STRU * pstAppToCbtMsg)
{
    CBT_UNIFORM_MSG_STRU       *pstCbtToAppMsg;
    CBT_ESTABLISH_IND_STRU     *pstCbtToAppEstablish;
    const MODEM_VER_INFO_S     *pstVerInfo = VOS_NULL_PTR;

    MsgBlock                   *pMsg;
    VOS_UINT32                  ulTotalSize = 0;

    /* 先断开链路 */
    g_stAcpuCbtCtrlInfo.ulCbtSwitchOnOff = CBT_STATE_IDLE;
    g_ulCbtMsgSN                         = 0;

    /*Allocate the memory space.*/
    ulTotalSize = CBT_MSG_HEAD_EX_LENGTH + CBT_IND_RESULT_SIZE + CBT_EST_IND_CHIP_ID_SIZE + CBT_EST_IND_RSV_LEN;
    pstCbtToAppMsg = (CBT_UNIFORM_MSG_STRU *)VOS_AssistantMemAlloc(ACPU_PID_CBT,
                                                   DYNAMIC_MEM_PT, ulTotalSize);
    if (VOS_NULL_PTR == pstCbtToAppMsg)
    {
        CBT_AcpuSendResultChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, OM_APP_ESTABLISH_CNF, VOS_ERR);
        return VOS_ERR;
    }

    pstCbtToAppEstablish = (CBT_ESTABLISH_IND_STRU *)pstCbtToAppMsg->aucPara;
    /* 以兼容校准工具，建链成功回复状态字0x02 */
    pstCbtToAppEstablish->ulResult = 0x0002;

    /* 调用底软接口获取芯片类型 */
    pstVerInfo = mdrv_ver_get_info();
    if(VOS_NULL_PTR == pstVerInfo)
    {
        CBT_AcpuSendResultChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, OM_APP_ESTABLISH_CNF, VOS_ERR);

        if (VOS_OK != VOS_MemFree(ACPU_PID_CBT, pstCbtToAppMsg))
        {
        }

        return VOS_ERR;
    }

    pstCbtToAppEstablish->ulChipId = (VOS_UINT32)(pstVerInfo->stproductinfo.chip_id);

    /*lint -e419 -e416*/
    PAM_MEM_SET_S(pstCbtToAppEstablish->ausReserve, CBT_EST_IND_RSV_LEN, 0, CBT_EST_IND_RSV_LEN);    /*预留字段，暂时赋值为全0*/
    /*lint +e419 +e416*/

    pstCbtToAppMsg->ulMsgLength = ulTotalSize - CBT_MSG_HEAD_EX_LENGTH;
    /* 给工具回复建链成功状态 */
    CBT_AcpuSendContentChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, OM_APP_ESTABLISH_CNF, pstCbtToAppMsg);

    if (VOS_OK != VOS_MemFree(ACPU_PID_CBT, pstCbtToAppMsg))
    {
    }

    /* 激活链路 */
    g_stAcpuCbtCtrlInfo.ulCbtSwitchOnOff = CBT_STATE_ACTIVE;

    /* 通知CCPU链路状态 */
    pMsg = (MsgBlock *)VOS_AllocMsg(PC_PID_TOOL, sizeof(CBT_UNIFORM_MSG_STRU));

    if (VOS_NULL_PTR == pMsg)
    {
        return VOS_ERR;
    }

    pMsg->ulReceiverPid      = CCPU_PID_CBT;

    /*lint -e419 -e416  */
    PAM_MEM_CPY_S((VOS_VOID *)(pMsg->aucValue),
                  (pstAppToCbtMsg->ulMsgLength + CBT_MSG_HEAD_EX_LENGTH),
                  (VOS_VOID *)pstAppToCbtMsg,
                  (pstAppToCbtMsg->ulMsgLength + CBT_MSG_HEAD_EX_LENGTH));
    /*lint +e419 +e416  */

    if (VOS_OK != VOS_SendMsg(PC_PID_TOOL, pMsg))
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_UINT32 CBT_AcpuReleaseProc(CBT_UNIFORM_MSG_STRU * pstAppToCbtMsg)
{
    CBT_UNIFORM_MSG_WITH_HEADER_STRU    *pSendMsg;

    g_stAcpuCbtCtrlInfo.ulCbtSwitchOnOff       = CBT_STATE_IDLE;

    pSendMsg = (CBT_UNIFORM_MSG_WITH_HEADER_STRU *)VOS_AllocMsg(PC_PID_TOOL, (sizeof(CBT_UNIFORM_MSG_WITH_HEADER_STRU) - VOS_MSG_HEAD_LENGTH));

    if (VOS_NULL_PTR == pSendMsg)
    {
        return VOS_ERR;
    }

    pSendMsg->ulReceiverPid      = CCPU_PID_CBT;

    /*lint -e419 -e416*/
    PAM_MEM_CPY_S((VOS_VOID *)(&pSendMsg->stPcMsgData),
                  sizeof(CBT_UNIFORM_MSG_STRU),
                  (VOS_VOID *)pstAppToCbtMsg,
                  sizeof(CBT_UNIFORM_MSG_STRU));
    /*lint +e419 +e416*/

    if (VOS_OK != VOS_SendMsg(PC_PID_TOOL, pSendMsg))
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


 VOS_UINT32 CBT_AcpuNvBackUp(CBT_UNIFORM_MSG_STRU * pstAppToCbtMsg)
{
    if(VOS_OK != mdrv_nv_backup())
    {
        CBT_AcpuSendResultChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, OM_APP_NV_BACKUP_CNF, VOS_ERR);
        return VOS_ERR;
    }
    CBT_AcpuSendResultChannel(pstAppToCbtMsg->stMsgHeader.stModemSsid, pstAppToCbtMsg->stCompMode, OM_APP_NV_BACKUP_CNF, VOS_OK);
    return VOS_OK;
}


VOS_VOID CBT_AppMsgProc(MsgBlock* pMsg)
{
    CBT_CTOA_MSG_STRU           *pstCbtMsg;
    CBT_UNIFORM_MSG_WITH_HEADER_STRU
                                *pstRcvMsg;
    CBT_UNIFORM_MSG_STRU        *pstPcToCbtMsg;
    VOS_UINT16                   usMsgId;

    DIAG_TraceReport((VOS_VOID *)pMsg);

    if (PC_PID_TOOL == pMsg->ulSenderPid)
    {
        pstRcvMsg                       = (CBT_UNIFORM_MSG_WITH_HEADER_STRU *)pMsg;
        pstPcToCbtMsg                   = (CBT_UNIFORM_MSG_STRU *)&(pstRcvMsg->stPcMsgData);
        usMsgId                         = pstPcToCbtMsg->usMsgId;

        switch (usMsgId)
        {
            case APP_OM_ESTABLISH_REQ:
                (VOS_VOID)CBT_AcpuEstablishProc(pstPcToCbtMsg);
                break;

            case APP_OM_RELEASE_REQ:
                (VOS_VOID)CBT_AcpuReleaseProc(pstPcToCbtMsg);
                break;

            case APP_OM_READ_NV_REQ:
                (VOS_VOID)CBT_AcpuReadNv(pstPcToCbtMsg, OM_APP_READ_NV_IND);
                break;

            case APP_OM_WRITE_NV_REQ:
                (VOS_VOID)CBT_AcpuWriteNv(pstPcToCbtMsg, OM_APP_WRITE_NV_CNF);
                break;

            case APP_OM_NV_BACKUP_REQ:
                (VOS_VOID)CBT_AcpuNvBackUp(pstPcToCbtMsg);
                break;

            default:
                CBT_AcpuSendResultChannel(pstPcToCbtMsg->stMsgHeader.stModemSsid, pstPcToCbtMsg->stCompMode, usMsgId, VOS_ERR);
                break;
        }
    }
    else if (CCPU_PID_CBT == pMsg->ulSenderPid)
    {
        pstCbtMsg = (CBT_CTOA_MSG_STRU *)pMsg;

        if (OM_CBT_SEND_DATA_REQ == pstCbtMsg->usPrimId)/* C核发过来的校准数据 */
        {
            (VOS_VOID)CBT_AcpuSendData((CBT_UNIFORM_MSG_STRU *)(pstCbtMsg->aucData), pstCbtMsg->usLen);
            return;
        }
    }
    else
    {
    }

    return;
}


VOS_UINT32 CBT_AppPidInit(enum VOS_INIT_PHASE_DEFINE ip)
{
    switch( ip )
    {
        case VOS_IP_RESTART:
            CBTPPM_OamCbtPortInit();
            break;

        default:
            break;
    }

    return VOS_OK;
}


VOS_UINT32 CBTAppFidInit(enum VOS_INIT_PHASE_DEFINE ip)
{
    VOS_UINT32 ulRslt;

    switch( ip )
    {
        case VOS_IP_LOAD_CONFIG:
        {

            ulRslt = VOS_RegisterPIDInfo(ACPU_PID_CBT,
                                        (Init_Fun_Type)CBT_AppPidInit,
                                        (Msg_Fun_Type)CBT_AppMsgProc);
            if( VOS_OK != ulRslt )
            {
                return VOS_ERR;
            }

            ulRslt = VOS_RegisterMsgTaskPrio(ACPU_FID_CBT, VOS_PRIORITY_M2);
            if( VOS_OK != ulRslt )
            {
                return VOS_ERR;
            }

            if (VOS_OK != CBT_AcpuInit())
            {
                return VOS_ERR;
            }

            /* CBT自处理任务创建 */
            if (VOS_OK != CBTSCM_SoftDecodeReqRcvTaskInit())
            {
                return VOS_ERR;
            }

            if (VOS_OK != CBTPPM_SocketTaskInit())
            {
                return VOS_ERR;
            }

            break;
        }

        default:
            break;
    }
    return VOS_OK;
}


