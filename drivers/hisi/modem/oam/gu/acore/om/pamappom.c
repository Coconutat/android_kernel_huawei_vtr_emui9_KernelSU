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


#include "pamappom.h"
#include "omprivate.h"
#include "NVIM_Interface.h"
#include "TafNvInterface.h"
#include "CbtPpm.h"
#include "DrvNvInterface.h"
#include "NasNvInterface.h"
#include "msp_nvim.h"
#include "AtOamInterface.h"
#include "PsLogFilterInterface.h"


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID PS_FILE_ID_PAM_APP_OM_C


/* 记录收到消息信息的buffer及当前长度 */
OM_RECORD_BUF_STRU                      g_astAcpuRecordInfo[VOS_EXC_DUMP_MEM_NUM_BUTT];

VOS_UINT32                              g_ulAcpuOmFilterFlag;


VOS_VOID OM_RecordInfoEnd(VOS_EXC_DUMP_MEM_NUM_ENUM_UINT32 enNumber)
{
    VOS_UINT32 *pulBuf;

    if(VOS_EXC_DUMP_MEM_NUM_BUTT <= enNumber)
    {
        return;
    }

    if(VOS_NULL_PTR == g_astAcpuRecordInfo[enNumber].pucBuf)
    {
        return;
    }

    if(0 == g_astAcpuRecordInfo[enNumber].ulLen)
    {
        return;
    }

    /* 在start中已经变更了记录endslice的长度，因此此处回退四个字节填写endslice的值 */
    pulBuf = (VOS_UINT32*)(g_astAcpuRecordInfo[enNumber].pucBuf + g_astAcpuRecordInfo[enNumber].ulLen - sizeof(VOS_UINT32));

    *pulBuf = VOS_GetSlice();

    return;
}


VOS_VOID OM_RecordInfoStart(VOS_EXC_DUMP_MEM_NUM_ENUM_UINT32 enNumber, VOS_UINT32 ulSendPid, VOS_UINT32 ulRcvPid, VOS_UINT32 ulMsgName)
{
    OM_RECORD_INFO_STRU     stRecordInfo;

    if(VOS_EXC_DUMP_MEM_NUM_BUTT <= enNumber)
    {
       return;
    }

    if(VOS_NULL_PTR == g_astAcpuRecordInfo[enNumber].pucBuf)
    {
       return;
    }

    g_astAcpuRecordInfo[enNumber].ulLen %= VOS_TASK_DUMP_INFO_SIZE;

    stRecordInfo.usSendPid      = (VOS_UINT16)ulSendPid;
    stRecordInfo.usRcvPid       = (VOS_UINT16)ulRcvPid;
    stRecordInfo.ulMsgName      = ulMsgName;
    stRecordInfo.ulSliceStart   = VOS_GetSlice();
    stRecordInfo.ulSliceEnd     = 0;

    PAM_MEM_CPY_S((g_astAcpuRecordInfo[enNumber].pucBuf + g_astAcpuRecordInfo[enNumber].ulLen),
                  sizeof(OM_RECORD_INFO_STRU),
                  &stRecordInfo,
                  sizeof(OM_RECORD_INFO_STRU));

    g_astAcpuRecordInfo[enNumber].ulLen += (VOS_UINT32)sizeof(OM_RECORD_INFO_STRU);

    return;
}


VOS_VOID OM_RecordMemInit(VOS_VOID)
{
   VOS_UINT32 i;

   PAM_MEM_SET_S(g_astAcpuRecordInfo,
                 sizeof(g_astAcpuRecordInfo),
                 0,
                 sizeof(g_astAcpuRecordInfo));

   /* 分配每个模块记录可谓可测信息的空间 */
   for(i = 0; i < VOS_EXC_DUMP_MEM_NUM_BUTT; i++)
   {
      g_astAcpuRecordInfo[i].pucBuf = (VOS_UINT8*)VOS_ExcDumpMemAlloc(i);

      if(VOS_NULL_PTR == g_astAcpuRecordInfo[i].pucBuf)
      {
          return;
      }
   }

   return;
}

VOS_VOID PAMOM_AcpuTimerMsgProc(MsgBlock* pMsg)
{
    return;
}

VOS_VOID PAMOM_QuereyPidInfo(VOS_VOID)
{
    PAM_VOS_QUEREY_PID_INFO_REQ_STRU    *pstMsg;

    pstMsg = (PAM_VOS_QUEREY_PID_INFO_REQ_STRU *)VOS_AllocMsg(ACPU_PID_PAM_OM,
                            sizeof(PAM_VOS_QUEREY_PID_INFO_REQ_STRU) - VOS_MSG_HEAD_LENGTH);

    /* 分配消息失败 */
    if (VOS_NULL_PTR == pstMsg)
    {
        return;
    }

    pstMsg->ulReceiverPid  = CCPU_PID_PAM_OM;
    pstMsg->usPrimId       = PAM_VOS_QUEREY_PID_INFO_REQ;

    (VOS_VOID)VOS_SendMsg(ACPU_PID_PAM_OM, pstMsg);

    return;
}


VOS_VOID PAMOM_QuereyPidInfoMsgProc(MsgBlock* pMsg)
{
    PAM_VOS_QUEREY_PID_INFO_REQ_STRU    *pstMsg;
    PAM_VOS_QUEREY_PID_INFO_CNF_STRU    *pstCnfMsg;
    VOS_UINT32                           ulLen;

    pstMsg = (PAM_VOS_QUEREY_PID_INFO_REQ_STRU *)pMsg;

    if (PAM_VOS_QUEREY_PID_INFO_REQ == pstMsg->usPrimId)
    {
        ulLen = VOS_QueryPidInfoBufSize();

        pstCnfMsg = (PAM_VOS_QUEREY_PID_INFO_CNF_STRU *)VOS_AllocMsg(ACPU_PID_PAM_OM,
                            sizeof(PAM_VOS_QUEREY_PID_INFO_CNF_STRU) - VOS_MSG_HEAD_LENGTH + ulLen);

        /* 分配消息失败 */
        if (VOS_NULL_PTR == pstCnfMsg)
        {
            return;
        }

        pstCnfMsg->ulReceiverPid  = CCPU_PID_PAM_OM;
        pstCnfMsg->usPrimId       = PAM_VOS_QUEREY_PID_INFO_CNF;
        pstCnfMsg->usLen          = (VOS_UINT16)ulLen;
        VOS_QueryPidInfo((VOS_VOID *)pstCnfMsg->aucValue);

        (VOS_VOID)VOS_SendMsg(ACPU_PID_PAM_OM, pstCnfMsg);
    }
    else if (PAM_VOS_QUEREY_PID_INFO_CNF == pstMsg->usPrimId)
    {
        pstCnfMsg = (PAM_VOS_QUEREY_PID_INFO_CNF_STRU *)pMsg;

        VOS_SetPidInfo((VOS_VOID *)(pstCnfMsg->aucValue), pstCnfMsg->usLen);
    }
    else
    {
        /* blank */
    }

    return;
}


VOS_UINT32 PAM_OM_AcpuPihToAtMsgFilter(
    const VOS_VOID                      *pMsg
)
{
    MN_APP_PIH_AT_CNF_STRU             *pstEventCnf;

    pstEventCnf = (MN_APP_PIH_AT_CNF_STRU *)pMsg;

    if (PIH_AT_EVENT_CNF != pstEventCnf->ulMsgId)
    {
        return VOS_FALSE;
    }

    switch (pstEventCnf->stPIHAtEvent.EventType)
    {
        case SI_PIH_EVENT_CGLA_SET_CNF:
        case SI_PIH_EVENT_CRSM_SET_CNF:
        case SI_PIH_EVENT_CRLA_SET_CNF:
        case SI_PIH_EVENT_CIMI_QRY_CNF:
        case SI_PIH_EVENT_CCIMI_QRY_CNF:
        case SI_PIH_EVENT_SIM_ICCID_IND:
        case SI_PIH_EVENT_GENERIC_ACCESS_CNF:
        case SI_PIH_EVENT_ISDB_ACCESS_CNF:
        case SI_PIH_EVENT_PRIVATECGLA_SET_CNF:
        case SI_PIH_EVENT_URSM_CNF:
            OM_NORMAL_LOG1("PAM_OM_AcpuAtToPihMsgFilter: The filter EventType is :", pstEventCnf->stPIHAtEvent.EventType);

            return VOS_TRUE;

        default:
            return VOS_FALSE;
    }
}


VOS_UINT32 PAM_OM_AcpuAtToPihMsgFilter(
    const VOS_VOID                      *pMsg
)
{
    MSG_HEADER_STRU                     *pstMsg;

    pstMsg = (MSG_HEADER_STRU *)pMsg;

    switch (pstMsg->ulMsgName)
    {
        case OM_SI_PIH_CRSM_SET_REQ:
        case OM_SI_PIH_CRLA_SET_REQ:
        case OM_SI_PIH_CGLA_SET_REQ:
        case OM_SI_PIH_GACCESS_REQ:
        case OM_SI_PIH_ISDB_ACCESS_REQ:
        case OM_SI_PIH_PRIVATECGLA_SET_REQ:
        case OM_SI_PIH_URSM_REQ:
            OM_NORMAL_LOG1("PAM_OM_AcpuAtToPihMsgFilter: The filter ulMsgName is :", pstMsg->ulMsgName);

            return VOS_TRUE;

        default:
            break;
    }

    return VOS_FALSE;
}


VOS_UINT32 PAM_OM_LayerMsgFilter(
    const VOS_VOID                      *pMsg
)
{
    OM_FILTER_MSG_HEAD_STRU             *pstMsgHead;

    if (VOS_FALSE == g_ulAcpuOmFilterFlag)
    {
        return VOS_FALSE;
    }

    pstMsgHead = (OM_FILTER_MSG_HEAD_STRU*)pMsg;

    /* PB相关的消息全部过滤 */
    if ((I0_MAPS_PB_PID == pstMsgHead->ulSenderPid)
     || (I1_MAPS_PB_PID == pstMsgHead->ulSenderPid)
     || (I2_MAPS_PB_PID == pstMsgHead->ulSenderPid)
     || (I0_MAPS_PB_PID == pstMsgHead->ulReceiverPid)
     || (I1_MAPS_PB_PID == pstMsgHead->ulReceiverPid)
     || (I2_MAPS_PB_PID == pstMsgHead->ulReceiverPid)
     || (ACPU_PID_PB    == pstMsgHead->ulReceiverPid))
    {
        return VOS_TRUE;
    }

     /* PIH 消息过滤 */
    if ( (I0_MAPS_PIH_PID == pstMsgHead->ulSenderPid)
      || (I1_MAPS_PIH_PID == pstMsgHead->ulSenderPid)
      || (I2_MAPS_PIH_PID == pstMsgHead->ulSenderPid))
    {
        return PAM_OM_AcpuPihToAtMsgFilter(pMsg);
    }

    if (WUEPS_PID_AT == pstMsgHead->ulSenderPid)
    {
        return PAM_OM_AcpuAtToPihMsgFilter(pMsg);
    }

    return VOS_FALSE;
}


 VOS_VOID PAMOM_AcpuCcpuPamMsgProc(MsgBlock* pMsg)
 {
    VOS_UINT16                          usPrimId;

    usPrimId = *(VOS_UINT16 *)(pMsg->aucValue);

    if (PAM_VOS_QUEREY_PID_INFO_REQ == usPrimId)
    {
        PAMOM_QuereyPidInfoMsgProc(pMsg);
    }
    else if (PAM_VOS_QUEREY_PID_INFO_CNF == usPrimId)
    {
        PAMOM_QuereyPidInfoMsgProc(pMsg);
    }
    else
    {

    }

    return;
}


VOS_VOID PAMOM_AppMsgProc(MsgBlock* pMsg)
{
    if (VOS_PID_TIMER == pMsg->ulSenderPid)
    {
        PAMOM_AcpuTimerMsgProc(pMsg);
    }
    else if (CCPU_PID_PAM_OM == pMsg->ulSenderPid)
    {
        PAMOM_AcpuCcpuPamMsgProc(pMsg);
    }
    else
    {
        /* blank */
    }

    return;
}


VOS_UINT32 PAMOM_AcpuInit(VOS_VOID)
{
    NAS_NV_PRIVACY_FILTER_CFG_STRU      stPrivacyFilterCfg  = {0};

    PS_OM_LayerMsgFilterFuncReg(PAM_OM_LayerMsgFilter);

    PAMOM_QuereyPidInfo();


    if (NV_OK != NV_ReadEx(MODEM_ID_0,
                           en_NV_Item_Privacy_Log_Filter_Cfg,
                           &stPrivacyFilterCfg,
                           sizeof(NAS_NV_PRIVACY_FILTER_CFG_STRU)))
    {
        stPrivacyFilterCfg.ucFilterEnableFlg = VOS_TRUE;
    }

    if (VOS_FALSE != stPrivacyFilterCfg.ucFilterEnableFlg)
    {
        stPrivacyFilterCfg.ucFilterEnableFlg = VOS_TRUE;
    }

    g_ulAcpuOmFilterFlag = stPrivacyFilterCfg.ucFilterEnableFlg;

    return VOS_OK;
}


VOS_UINT32 PAMOM_AppPidInit(enum VOS_INIT_PHASE_DEFINE ip)
{
    switch( ip )
    {
        case VOS_IP_LOAD_CONFIG:
            return PAMOM_AcpuInit();

        default:
            break;
    }

    return VOS_OK;
}


VOS_UINT32 PAMOM_APP_FID_Init(enum VOS_INIT_PHASE_DEFINE ip)
{
    VOS_UINT32                          ulRslt;

    switch( ip )
    {
        case VOS_IP_LOAD_CONFIG:
        {
            ulRslt = VOS_RegisterPIDInfo(ACPU_PID_PAM_OM,
                                        (Init_Fun_Type)PAMOM_AppPidInit,
                                        (Msg_Fun_Type)PAMOM_AppMsgProc);
            if( VOS_OK != ulRslt )
            {
                return VOS_ERR;
            }

            ulRslt = VOS_RegisterMsgTaskPrio(ACPU_FID_PAM_OM, VOS_PRIORITY_M2);
            if( VOS_OK != ulRslt )
            {
                return VOS_ERR;
            }

            /* 如目录不存在则创建 */
            if (VOS_OK != mdrv_file_access(PAM_LOG_PARTH, PAM_FILE_EXIST))
            {
                (VOS_VOID)mdrv_file_mkdir(PAM_LOG_PARTH);
            }

            break;
        }

        default:
            break;
    }
    return VOS_OK;
}


VOS_VOID OM_OSAEvent(VOS_VOID *pData, VOS_UINT32 ulLength)
{
    DIAG_EVENT_IND_STRU                 stEventInd;

    stEventInd.ulModule = DIAG_GEN_MODULE(DIAG_MODEM_0, DIAG_MODE_COMM);
    stEventInd.ulPid    = ACPU_PID_PAM_OM;
    stEventInd.ulEventId= OAM_EVENT_TIMER;
    stEventInd.ulLength = ulLength;
    stEventInd.pData    = pData;

    (VOS_VOID)DIAG_EventReport(&stEventInd);

    return;
}


/* AT<->AT的屏蔽处理，移到GuNasLogFilter.c */


