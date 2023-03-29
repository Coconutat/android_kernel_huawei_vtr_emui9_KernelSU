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

#include "vos.h"
#include "product_config.h"
#include "NVIM_Interface.h"
#include "omerrorlog.h"
#include "OmCommonPpm.h"
#include "AtAppVcomInterface.h"
#include "msp_diag_comm.h"
#include "diag_acore_common.h"
#include "diag_common.h"
#include "errlog_cfg.h"



#define    THIS_FILE_ID        PS_FILE_ID_OM_ERRORLOG_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

#define    ERRLOG_IDLE         0
#define    ERRLOG_BUSY         1

/* TODO:cjq */

/* 用于记录Error Log收到和发送给Ap侧消息 */
typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                          ulMsgName;
    VOS_UINT32                          ulMsgLen;
    VOS_UINT8                           aucData[0];
}OM_ERR_LOG_REQ_STRU;

/* 用于记录Error Log收到和发送给Ap侧消息 */
typedef struct
{
    VOS_UINT32                          ulFTMRcvNum;
    VOS_UINT32                          ulFTMRcvLen;
    VOS_UINT32                          ulErrLogRcvNum;
    VOS_UINT32                          ulErrLogRcvLen;

    VOS_UINT32                          ulFTMSendNum;
    VOS_UINT32                          ulFTMSendLen;
    VOS_UINT32                          ulErrLogSendNum;
    VOS_UINT32                          ulErrLogSendLen;
}OM_ERR_LOG_DEBUG_INFO;

/* OM收到AP需要在全局变量中记录内容 */
typedef struct
{
    ERR_LOG_ALARM_STRU                 *pstErrorLogModule;
    VOS_UINT32                         *pulFTMModule;
    VOS_UINT32                          ulErrLogReportSend;/* 记录Err Log需要上报组件 */
    VOS_UINT32                          ulFTMReportSend;   /* 记录FTM需要上报组件 */
    VOS_UINT32                          ulErrLogState;
    //VOS_UINT32                          *pstReceiveFlag;
    VOS_UINT32                          ulAlarmIdNum;
    VOS_UINT16                          usModemId;
    VOS_UINT16                          usFaultId;
    VOS_UINT8                           aucRsv[4];
}OM_APP_MSG_RECORD_STRU;

OM_APP_MSG_RECORD_STRU                  g_stOmAppMsgRecord; /* OM收到AP需要在全局变量中记录内容 */

      
struct semaphore                        g_stOmRxErrorLogBuffSem;

OM_VCOM_DEBUG_INFO                      g_stErrLogVcomDebugInfo   = {0};

OM_ERR_LOG_DEBUG_INFO                   g_stRcvUESendAP           = {0};

HTIMER                                  g_AcpuErrLogFullTmr       = VOS_NULL_PTR ;

VOS_SPINLOCK                            g_stVosErrLogSendSpinLock;  /* 自旋锁，用来作Err Log上报状态机的临界资源保护 */

HTIMER                                  g_AcpuCltInfoFullTmr      = VOS_NULL_PTR ;

/* 新增下发上报请求时的时间戳，用于故障上报结束时的消息 */
VOS_UINT64                              g_ulTriggerTime64Bit = 0;

/* 1表示超时后下次下发采集请求前，MTA上报的消息将不被OM处理 */
VOS_UINT32                              g_AcpuCtlInfoCnfNotNeedProcess = 0;
VOS_UINT8                               g_ucErrRptFlag[256] = {0};
#define                                 OM_MSG_RECEIVE_FLAG               (1) 
#define                                 OM_MSG_NO_RECEIVE_FLAG            (0) 
/* 工程模式主动上报涉及PID */
VOS_UINT32    g_aulModem0FTMDetail[OM_MAX_MODULE_ID]={
                         I0_WUEPS_PID_MMC,I0_WUEPS_PID_MM,I0_WUEPS_PID_GMM,MSP_PID_DIAG_AGENT,
                         0,               0,               0,              0,
                         0,               0,               0,              0,
                         WUEPS_PID_WRR,   0,               0,              DSP_PID_WPHY,
                         I0_DSP_PID_APM,  I0_DSP_PID_GPHY, 0,              0,
                         PS_PID_ERRC,     TPS_PID_RRC,     0,              0,
                         PS_PID_MM,       PS_PID_ESM,      0,              0,
                         0,               0,               0,              0};

VOS_UINT32    g_aulModem1FTMDetail[OM_MAX_MODULE_ID]={
                         I1_WUEPS_PID_MMC,I1_WUEPS_PID_MM,I1_WUEPS_PID_GMM,MSP_PID_DIAG_AGENT,
                         0,               0,              0,               0,
                         0,               0,              0,               0,
                         WUEPS_PID_WRR,   0,              0,               DSP_PID_WPHY,
                         I1_DSP_PID_APM,  I1_DSP_PID_GPHY,0,               0,
                         PS_PID_ERRC,     TPS_PID_RRC,    0,               0,
                         PS_PID_MM,       PS_PID_ESM,     0,               0,
                         0,               0,              0,               0};

VOS_UINT32    g_aulModem2FTMDetail[OM_MAX_MODULE_ID]={
                         I2_WUEPS_PID_MMC,I2_WUEPS_PID_MM,I2_WUEPS_PID_GMM,MSP_PID_DIAG_AGENT,
                         0,               0,              0,               0,
                         0,               0,              0,               0,
                         WUEPS_PID_WRR,   0,              0,               DSP_PID_WPHY,
                         I2_DSP_PID_APM,  I2_DSP_PID_GPHY,0,               0,
                         PS_PID_ERRC,     TPS_PID_RRC,    0,               0,
                         PS_PID_MM,       PS_PID_ESM,     0,               0,
                         0,               0,              0,               0};

/* 工程模式命令上报组件对应PID */
/* 数组中两个moduleID只是为了验证此功能增加，并没有实际用到 */
APP_OM_FTM_MSG_PID_STRU    g_astModem0FTMMsgModule[]={
                            {OM_ERR_LOG_MOUDLE_ID_IMS,  0},
                            {OM_ERR_LOG_MOUDLE_ID_IMSA, 0},
                           };

/* 数组中两个moduleID只是为了验证此功能增加，并没有实际用到 */
APP_OM_FTM_MSG_PID_STRU    g_astModem1FTMMsgModule[]={
                            {OM_ERR_LOG_MOUDLE_ID_IMS,  0},
                            {OM_ERR_LOG_MOUDLE_ID_IMSA, 0},
                           };

/* 数组中两个moduleID只是为了验证此功能增加，并没有实际用到 */
APP_OM_FTM_MSG_PID_STRU    g_astModem2FTMMsgModule[]={
                            {OM_ERR_LOG_MOUDLE_ID_IMS,  0},
                            {OM_ERR_LOG_MOUDLE_ID_IMSA, 0},
                           };

/*****************************************************************************
  3 外部引用声明
*****************************************************************************/

/*****************************************************************************
  4 函数实现
*****************************************************************************/

#define OM_ACPU_DEBUG_TRACE(pucData, ulDataLen, ulSwitch) \
        if(VOS_FALSE != (g_ulOmAcpuDbgFlag&ulSwitch)) \
        { \
            VOS_UINT32 ulOmDbgIndex; \
            (VOS_VOID)vos_printf("\n%s, Data Len: = %d\n", __FUNCTION__, ulDataLen); \
            for (ulOmDbgIndex = 0 ; ulOmDbgIndex < ulDataLen; ulOmDbgIndex++) \
            { \
                (VOS_VOID)vos_printf("%02x ", *((VOS_UINT8*)pucData + ulOmDbgIndex)); \
            } \
            (VOS_VOID)vos_printf("\r\n"); \
        }

#define OM_ACPU_CLT_REQ_MSGLEN 4*1024

VOS_VOID OM_AcpuRcvMsgFinish(VOS_VOID)
{
    OM_APP_REPORT_STATUS_STRU               stOmAppReportStatus={{0,},0,0,0,0,0,0};

    (VOS_VOID)VOS_MemSet_s(&stOmAppReportStatus, sizeof(stOmAppReportStatus), 0, (VOS_UINT32)(sizeof(OM_APP_REPORT_STATUS_STRU)));
    
    stOmAppReportStatus.stOmHeader.ulMsgType = OM_ERR_LOG_MSG_ERR_REPORT_END;
    stOmAppReportStatus.stOmHeader.ulMsgSN   = g_stErrLogVcomDebugInfo.ulVCOMSendSn++;
    stOmAppReportStatus.stOmHeader.ulMsgLen  = (sizeof(OM_APP_REPORT_STATUS_STRU) - sizeof(OM_ALARM_MSG_HEAD_STRU));
    stOmAppReportStatus.usModemID            = g_stOmAppMsgRecord.usModemId;
    stOmAppReportStatus.usFaultID            = g_stOmAppMsgRecord.usFaultId;
    stOmAppReportStatus.ulReportStatus       = g_stOmAppMsgRecord.ulErrLogReportSend;
    stOmAppReportStatus.ulTrigLowSlice       = (VOS_UINT32)(g_ulTriggerTime64Bit & 0xffffffff);
    stOmAppReportStatus.ulTrigHighSlice      = (VOS_UINT32)((g_ulTriggerTime64Bit >> 32) & 0xffffffff);

    OM_AcpuSendVComData((VOS_UINT8 *)&stOmAppReportStatus,(VOS_UINT32)sizeof(OM_APP_REPORT_STATUS_STRU));
    VOS_MemFree(MSP_PID_DIAG_APP_AGENT, g_stOmAppMsgRecord.pstErrorLogModule); 
    g_stOmAppMsgRecord.pstErrorLogModule = VOS_NULL;
    VOS_MemSet_s(g_ucErrRptFlag, sizeof(g_ucErrRptFlag),OM_MSG_RECEIVE_FLAG, 256*sizeof(VOS_UINT8));
    /* 防止消息再次下发不成功 */
    g_stOmAppMsgRecord.ulErrLogReportSend   = OM_AP_SEND_MSG_FINISH;
    g_stOmAppMsgRecord.ulErrLogState        = ERRLOG_IDLE;

    return ;
}


VOS_VOID OM_AcpuSendAppResult(VOS_UINT32 ulRest, VOS_UINT16  usModemId)
{
    OM_APP_RESULT_CNF_STRU               stOmAppResultCnf={{0,},0,0,0,0};

   
    (VOS_VOID)VOS_MemSet_s(&stOmAppResultCnf, sizeof(stOmAppResultCnf), 0, (VOS_UINT32)sizeof(OM_APP_RESULT_CNF_STRU));
   
    stOmAppResultCnf.stOmHeader.ulMsgType = OM_ERR_LOG_MSG_RESULT;
    stOmAppResultCnf.stOmHeader.ulMsgSN   = g_stErrLogVcomDebugInfo.ulVCOMSendSn++;
    stOmAppResultCnf.stOmHeader.ulMsgLen  = (sizeof(OM_APP_RESULT_CNF_STRU) - sizeof(OM_ALARM_MSG_HEAD_STRU));
    stOmAppResultCnf.usModemID            = usModemId;
    stOmAppResultCnf.ulStatus             = ulRest;

    OM_AcpuSendVComData((VOS_UINT8 *)&stOmAppResultCnf, (VOS_UINT32)sizeof(OM_APP_RESULT_CNF_STRU));

    return ;
}


VOS_INT OM_AcpuSwitchOnOffErrLog(APP_OM_CTRL_STATUS_STRU  *pstAppOmCtrlStatus)
{
    NV_ID_ERR_LOG_CTRL_INFO_STRU         stNvErrLogCtrlInfo;
    OM_ERROR_LOG_CTRL_IND_STRU          *pOmErrorLogCtrlInd;
    VOS_UINT32                           i;
    VOS_UINT32                          *pulSwitchOnOffModule;
    VOS_UINT32                           ulPidNum;
    /* 开关内容 */
    stNvErrLogCtrlInfo.ucAlmStatus = pstAppOmCtrlStatus->ucAlmStatus;
    stNvErrLogCtrlInfo.ucAlmLevel  = pstAppOmCtrlStatus->ucAlmLevel;
    
    if (VOS_OK !=NV_WriteEx(pstAppOmCtrlStatus->usModemID,
                            en_NV_Item_ErrLogCtrlInfo,
                            &stNvErrLogCtrlInfo,
                            (VOS_UINT32)sizeof(NV_ID_ERR_LOG_CTRL_INFO_STRU)))
    {
        /* 给Ap 回复消息 */
        
        (VOS_VOID)vos_printf("OM_AcpuSwitchOnOffErrLog: nv write fail!\r\n ");
        
        
        OM_AcpuSendAppResult(OM_APP_OMACPU_WRITE_NV_ERR, pstAppOmCtrlStatus->usModemID);
        return OM_APP_OMACPU_WRITE_NV_ERR;
    }
    
    pulSwitchOnOffModule=(VOS_UINT32*)VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT,DYNAMIC_MEM_PT,OM_PAM_LENGTH);
    if (VOS_NULL_PTR == pulSwitchOnOffModule)
    {
        OM_AcpuSendAppResult(OM_APP_PID_ALLOC_ERR, pstAppOmCtrlStatus->usModemID);
        return OM_APP_PID_ALLOC_ERR;
    }
    VOS_MemSet_s(pulSwitchOnOffModule, OM_PAM_LENGTH, 0,OM_PAM_LENGTH);
    if(VOS_OK!=ErrLog_GetErrlogPid(pstAppOmCtrlStatus->usModemID,pulSwitchOnOffModule,&ulPidNum))
    {
        /* 给Ap 回复消息 */
        (VOS_VOID)vos_printf("OM_AcpuSwitchOnOffErrLog: get pid fail!\r\n ");
        
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrNum++;
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrLen += (pstAppOmCtrlStatus->stOmHeader.ulMsgLen + sizeof(OM_ALARM_MSG_HEAD_STRU));
        OM_AcpuSendAppResult(OM_APP_GET_PID_ERR, pstAppOmCtrlStatus->usModemID);
        VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pulSwitchOnOffModule);
        return   OM_APP_GET_PID_ERR;
    }   
    
    for (i=0; i < ulPidNum; i++)
    {
            pOmErrorLogCtrlInd = (OM_ERROR_LOG_CTRL_IND_STRU*)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                                  (sizeof(OM_ERROR_LOG_CTRL_IND_STRU) - VOS_MSG_HEAD_LENGTH));
            if (VOS_NULL_PTR == pOmErrorLogCtrlInd)
            {
                OM_AcpuSendAppResult(OM_APP_OMACPU_ALLOC_MSG_ERR, pstAppOmCtrlStatus->usModemID);
                VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pulSwitchOnOffModule);
                return OM_APP_OMACPU_ALLOC_MSG_ERR;
            }

            pOmErrorLogCtrlInd->ulReceiverPid = pulSwitchOnOffModule[i];
            pOmErrorLogCtrlInd->ulMsgName     = ID_OM_ERR_LOG_CTRL_IND;
            pOmErrorLogCtrlInd->usModemID     = pstAppOmCtrlStatus->usModemID;
            pOmErrorLogCtrlInd->ucAlmStatus   = pstAppOmCtrlStatus->ucAlmStatus;
            pOmErrorLogCtrlInd->ucAlmLevel    = pstAppOmCtrlStatus->ucAlmLevel;
            (VOS_VOID)VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pOmErrorLogCtrlInd);
    }
    /* 给AP回复消息 */
    OM_AcpuSendAppResult(OM_APP_MSG_OK, pstAppOmCtrlStatus->usModemID);
    VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pulSwitchOnOffModule);
    return VOS_OK;
}


VOS_INT OM_AcpuSwitchOnOffFTM(APP_OM_CTRL_STATUS_STRU *pstAppOmCtrlStatus)
{
    NV_ID_FTM_DETAIL_STRU               stNvFTMDetail={0};
    OM_FTM_CTRL_IND_STRU               *pstOmFtmCtrlInd;
    VOS_UINT32                          ulRest;
    VOS_UINT32                          i;

    /* modem ID 检查*/
    if (MODEM_ID_0 == pstAppOmCtrlStatus->usModemID)
    {
        g_stOmAppMsgRecord.pulFTMModule = g_aulModem0FTMDetail;
    }
    else if (MODEM_ID_1 == pstAppOmCtrlStatus->usModemID)
    {
        g_stOmAppMsgRecord.pulFTMModule = g_aulModem1FTMDetail;
    }
    else if (2 == pstAppOmCtrlStatus->usModemID)
    {
        g_stOmAppMsgRecord.pulFTMModule = g_aulModem2FTMDetail;
    }
    else
    {
        
        (VOS_VOID)vos_printf("OM_AcpuSwitchOnOffFTM: modem id error!\r\n ");
        
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrNum++;
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrLen += (pstAppOmCtrlStatus->stOmHeader.ulMsgLen + sizeof(OM_ALARM_MSG_HEAD_STRU));
        OM_AcpuSendAppResult(OM_APP_SEND_MODEM_ID_ERR, pstAppOmCtrlStatus->usModemID);
        return OM_APP_SEND_MODEM_ID_ERR;
    }

    ulRest = NV_ReadEx(pstAppOmCtrlStatus->usModemID, en_NV_Item_FTMDetail,
                       (VOS_VOID*)&stNvFTMDetail, (VOS_UINT32)sizeof(NV_ID_FTM_DETAIL_STRU));
    if (VOS_OK != ulRest)
    {
       
        (VOS_VOID)vos_printf("OM_AcpuSwitchOnOffFTM: nv read faile!\r\n ");
        
        OM_AcpuSendAppResult(OM_APP_OMACPU_READ_NV_ERR, pstAppOmCtrlStatus->usModemID);
        return OM_APP_OMACPU_READ_NV_ERR;
    }

    g_stOmAppMsgRecord.ulFTMReportSend = stNvFTMDetail.ulFTMDetail;

    /* 因各组件任务优先级较高，先给AP回复消息。后便存在失败的场景，但可维可测，不以过度考虑 */
    OM_AcpuSendAppResult(OM_APP_MSG_OK, pstAppOmCtrlStatus->usModemID);

    /* 根据工程模式相关性，向对应PID发送消息 */
    for (i=0; i<OM_MAX_MODULE_ID; i++)
    {
        if ((0 !=(BIT_N(i) & g_stOmAppMsgRecord.ulFTMReportSend)) && (OM_PID_NULL != g_stOmAppMsgRecord.pulFTMModule[i]))
        {
            /* 给对应的PID发送消息 */
            pstOmFtmCtrlInd = (OM_FTM_CTRL_IND_STRU*)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                                  (sizeof(OM_FTM_CTRL_IND_STRU) - VOS_MSG_HEAD_LENGTH));
            if (VOS_NULL_PTR == pstOmFtmCtrlInd)
            {
                OM_AcpuSendAppResult(OM_APP_OMACPU_ALLOC_MSG_ERR, pstAppOmCtrlStatus->usModemID);
                return OM_APP_OMACPU_ALLOC_MSG_ERR;
            }

            pstOmFtmCtrlInd->ulReceiverPid = g_stOmAppMsgRecord.pulFTMModule[i];
            pstOmFtmCtrlInd->ulMsgName     = ID_OM_FTM_CTRL_IND;
            pstOmFtmCtrlInd->usModemID     = pstAppOmCtrlStatus->usModemID;
            pstOmFtmCtrlInd->ucActionFlag  = pstAppOmCtrlStatus->ucAlmStatus;

            (VOS_VOID)VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstOmFtmCtrlInd);
        }
    }

    return VOS_OK;
}


VOS_INT OM_AcpuSwitchOnOff(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    APP_OM_CTRL_STATUS_STRU              *pstAppOmCtrlStatus;

    pstAppOmCtrlStatus = (APP_OM_CTRL_STATUS_STRU*)pucData;

    if (OM_APP_SWITCH_MSG_ID_ERR_LOG == pstAppOmCtrlStatus->ulMsgModuleID)
    {
        /* Error log 开关 */
        return OM_AcpuSwitchOnOffErrLog(pstAppOmCtrlStatus);
    }
    else if (OM_APP_SWITCH_MSG_ID_FTM == pstAppOmCtrlStatus->ulMsgModuleID)
    {
        /* 工程模式开关 */
        return OM_AcpuSwitchOnOffFTM(pstAppOmCtrlStatus);
    }
    else
    {
        /* 异常信息上报 */
        (VOS_VOID)vos_printf("OM_AcpuSwitchOnOff: Msg Module id error!\r\n ");
        
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrNum++;
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrLen += ulLen;
        OM_AcpuSendAppResult(OM_APP_MSG_MODULE_ID_ERR, pstAppOmCtrlStatus->usModemID);
    }

    return OM_APP_MSG_MODULE_ID_ERR;
}



VOS_VOID OM_AcpuSendSkipPid(VOS_UINT32 index)
{
    if(g_stOmAppMsgRecord.ulErrLogReportSend > 0)
    {
        g_ucErrRptFlag[index] = OM_MSG_RECEIVE_FLAG;
        g_stOmAppMsgRecord.ulErrLogReportSend--;
    }
    
}


VOS_INT OM_AcpuRcvAppMsgCheck(APP_OM_REQ_ERR_LOG_STRU *pstAppOmReqErrLog)
{
    NV_ID_ERR_LOG_CTRL_INFO_STRU        stNvErrLogCtrlInfo={0};
    VOS_UINT32                          ulRest;
    
    /* 判断开关是否打开 */
    ulRest = NV_ReadEx(pstAppOmReqErrLog->usModemID,
                       en_NV_Item_ErrLogCtrlInfo,
                       (VOS_VOID*)&stNvErrLogCtrlInfo,
                       sizeof(NV_ID_ERR_LOG_CTRL_INFO_STRU));
    if (VOS_OK != ulRest)
    {
       
        (VOS_VOID)vos_printf("OM_AcpuRcvAppMsgCheck: nv read fail nv id is: %d!\r\n",en_NV_Item_ErrLogCtrlInfo);
       
        OM_AcpuSendAppResult(OM_APP_OMACPU_READ_NV_ERR, pstAppOmReqErrLog->usModemID);   
        return OM_APP_OMACPU_READ_NV_ERR;
    }

    /* 判断开关是否打开 */
    if (OM_APP_STATUS_CLOSE == stNvErrLogCtrlInfo.ucAlmStatus)
    {
        
        (VOS_VOID)vos_printf("OM_AcpuRcvAppMsgCheck: error log switch is close!\r\n");
        
        OM_AcpuSendAppResult(OM_APP_ERRLOG_SWITCH_CLOSE_ERR, pstAppOmReqErrLog->usModemID);
        return OM_APP_ERRLOG_SWITCH_CLOSE_ERR;
    }
    /* 如果没有上报完成 */
    if ((OM_AP_SEND_MSG_FINISH != g_stOmAppMsgRecord.ulErrLogReportSend)
        || (ERRLOG_IDLE != g_stOmAppMsgRecord.ulErrLogState))
    {
     
        (VOS_VOID)vos_printf("OM_AcpuRcvAppMsgCheck: error log report not end: %d!\r\n ", g_stOmAppMsgRecord.ulErrLogReportSend);
        
        OM_AcpuSendAppResult(OM_APP_REPORT_NOT_FINISH_ERR, pstAppOmReqErrLog->usModemID);    
        return OM_APP_REPORT_NOT_FINISH_ERR;
    }
    
    return VOS_OK;
}


VOS_INT OM_AcpuReportErrLogMsg(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    APP_OM_REQ_ERR_LOG_STRU            *pstAppOmReqErrLog;
    OM_ERR_LOG_REPORT_REQ_STRU         *pstOmErrLogReportReq;
    ERR_LOG_ALARM_STRU                 *pstErrologModule;
    VOS_UINT32                          ulSendPidCount = 0;
    VOS_UINT32                          ulAlarmidNum;
    VOS_INT                             lRest;
    VOS_UINT32                          i;
    pstAppOmReqErrLog = (APP_OM_REQ_ERR_LOG_STRU*)pucData;
    /* 收到AP消息检查 */
    lRest = OM_AcpuRcvAppMsgCheck(pstAppOmReqErrLog);
    if (VOS_OK != lRest)
    {
        return lRest;
    }
    else
    {
        g_stOmAppMsgRecord.ulErrLogState = ERRLOG_BUSY;
    }
    (void)vos_printf("OM_AcpuReportErrLogMsg: modem:0x%x,fauiltid:0x%x!\n",pstAppOmReqErrLog->usModemID,pstAppOmReqErrLog->usFaultId);

    pstErrologModule=(ERR_LOG_ALARM_STRU*)VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT,DYNAMIC_MEM_PT,OM_PAM_LENGTH);
    if (VOS_NULL_PTR == pstErrologModule)
    {
        OM_AcpuSendAppResult(OM_APP_PID_ALLOC_ERR, pstAppOmReqErrLog->usModemID);
        g_stOmAppMsgRecord.ulErrLogState = ERRLOG_IDLE;
        return OM_APP_PID_ALLOC_ERR;
    }
    VOS_MemSet_s(pstErrologModule, OM_PAM_LENGTH, 0, OM_PAM_LENGTH);
    if(VOS_OK!=ErrLog_GetPidAndAlarmId(pstAppOmReqErrLog->usModemID,pstAppOmReqErrLog->usFaultId,
                                     pstErrologModule,&ulAlarmidNum))
    {
        /* 给Ap 回复消息 */
        
        (VOS_VOID)vos_printf("OM_AcpuSwitchOnOffErrLog: get pid fail!\r\n ");
        
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrNum++;
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrLen += (pstAppOmReqErrLog->stOmHeader.ulMsgLen + sizeof(OM_ALARM_MSG_HEAD_STRU));
        OM_AcpuSendAppResult(OM_APP_GET_PID_ERR, pstAppOmReqErrLog->usModemID);
        VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstErrologModule);
 //       pstErrologModule = VOS_NULL;
        g_stOmAppMsgRecord.ulErrLogState = ERRLOG_IDLE;
        return  OM_APP_GET_PID_ERR;
    }
    VOS_MemSet_s(g_ucErrRptFlag, sizeof(g_ucErrRptFlag), 0, 256*sizeof(VOS_UINT8));
    g_stOmAppMsgRecord.ulAlarmIdNum      =ulAlarmidNum;
    g_stOmAppMsgRecord.ulErrLogReportSend=ulAlarmidNum;
    g_stOmAppMsgRecord.usFaultId         =pstAppOmReqErrLog->usFaultId;
    g_stOmAppMsgRecord.usModemId         =pstAppOmReqErrLog->usModemID;
    g_stOmAppMsgRecord.pstErrorLogModule =pstErrologModule;
    
    /* 因各组件任务优先级较高，先给AP回复消息。后便存在失败的场景，但可维可测，不以过度考虑 */
    OM_AcpuSendAppResult(OM_APP_MSG_OK, pstAppOmReqErrLog->usModemID);
    

    /* 根据告警相关性，向对应PID发送消息 */
    (void)vos_printf("OM_AcpuReportErrLogMsg: alarm num:0x%x!\n",g_stOmAppMsgRecord.ulAlarmIdNum);
    for (i=0; i<g_stOmAppMsgRecord.ulAlarmIdNum; i++)
    {
        /* if pid is invalid, skip it! */
        VOS_UINT32 RecvPid = g_stOmAppMsgRecord.pstErrorLogModule[i].ulPid;
        if(VOS_PID_AVAILABLE != VOS_CheckPidValidity(RecvPid))
        {
            OM_AcpuSendSkipPid(i);
            (void)vos_printf("OM_AcpuReportErrLogMsg: Index[%d]:Pid 0x%x Alarmid 0x%x is invalid, Skip it!\n",
                i, RecvPid, g_stOmAppMsgRecord.pstErrorLogModule[i].ulAlarmID);
            continue;
        }
        
        ulSendPidCount++;
        /* 给对应的PID发送消息 */
        pstOmErrLogReportReq  = (OM_ERR_LOG_REPORT_REQ_STRU*)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                                 (sizeof(OM_ERR_LOG_REPORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH));

        if (VOS_NULL_PTR == pstOmErrLogReportReq)
        {   
            VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstErrologModule);
            OM_AcpuSendAppResult(OM_APP_OMACPU_ALLOC_MSG_ERR, pstAppOmReqErrLog->usModemID);
            g_stOmAppMsgRecord.ulErrLogState = ERRLOG_IDLE;
            return OM_APP_OMACPU_ALLOC_MSG_ERR;
        }
        
        pstOmErrLogReportReq->ulReceiverPid = g_stOmAppMsgRecord.pstErrorLogModule[i].ulPid;
        pstOmErrLogReportReq->ulMsgName     = ID_OM_ERR_LOG_REPORT_REQ;
        pstOmErrLogReportReq->usModemID     = pstAppOmReqErrLog->usModemID;
        pstOmErrLogReportReq->ucFaultID     = (VOS_UINT8)pstAppOmReqErrLog->usFaultId;
        pstOmErrLogReportReq->ucAlarmID     = (VOS_UINT8)g_stOmAppMsgRecord.pstErrorLogModule[i].ulAlarmID;

        (VOS_VOID)VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstOmErrLogReportReq);
    }

    /* 假如没有告警相关性PID，直接给AP回复消息上报完毕 */
    if (0 == ulSendPidCount)
    {
        g_stOmAppMsgRecord.usModemId = pstAppOmReqErrLog->usModemID;
        (void)vos_printf("OM_AcpuReportErrLogMsg: no PID send\n");
        OM_AcpuRcvMsgFinish();
        return VOS_OK;
    }

    /* 起5s定时器 */
    g_AcpuErrLogFullTmr = VOS_NULL_PTR;
    if (VOS_OK != VOS_StartRelTimer(&g_AcpuErrLogFullTmr, MSP_PID_DIAG_APP_AGENT, OM_ERRLOG_TIMER_LENTH,
                                    DIAG_ERRORLOG_TIMER_NAME, DIAG_ERRORLOG_TIMER_PARA, VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5))
    {   
        
        (VOS_VOID)vos_printf("OM_AcpuReportErrLogMsg: StartDrxTimer fail\n");
       
        return OM_APP_ERRLOG_START_TIMER_ERR;
    }
    (void)vos_printf("OM_AcpuReportErrLogMsg: start timer success!\n");

    return VOS_OK;
}



VOS_INT OM_AcpuErrLogReqProc(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    OM_ERR_LOG_REQ_STRU     *pReqMsg;
    APP_OM_REQ_ERR_LOG_STRU *pReqErrLog = (APP_OM_REQ_ERR_LOG_STRU*)pucData;
    /* 给对应的PID发送消息 */
    pReqMsg  = (OM_ERR_LOG_REQ_STRU*)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                             (((sizeof(OM_ERR_LOG_REQ_STRU)) - VOS_MSG_HEAD_LENGTH) + ulLen));

    if (VOS_NULL_PTR == pReqMsg)
    {
        OM_AcpuSendAppResult(OM_APP_OMACPU_ALLOC_MSG_ERR, pReqErrLog->usModemID);
        (VOS_VOID)vos_printf("OM_AcpuErrLogReqProc: Alloc msg fail, modem:0x%x,faultid:0x%x!\n ",
                             pReqErrLog->usModemID,pReqErrLog->usFaultId);
        return OM_APP_OMACPU_ALLOC_MSG_ERR;
    }
    
    pReqMsg->ulReceiverPid = MSP_PID_DIAG_APP_AGENT;
    pReqMsg->ulMsgName     = ID_MSG_DIAG_CHR_REQ;
    pReqMsg->ulMsgLen      = ulLen;

    (void)VOS_MemCpy_s(pReqMsg->aucData, ulLen, pucData, ulLen);

    (void)VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pReqMsg);
    
    (void)vos_printf("OM_AcpuErrLogReqProc: Recv chr req, modem:0x%x,faultid:0x%x!\n ",
                         pReqErrLog->usModemID,pReqErrLog->usFaultId);
    return VOS_OK;
}


VOS_VOID OM_AcpuErrLogHook(VOS_UINT8 *pucData, VOS_UINT32 ulLen, VOS_UINT32 ulDateType)
{
    DIAG_TRANS_IND_STRU stTrans = {0};

    stTrans.ulModule    = DIAG_GEN_MODULE_EX(0, DIAG_MODE_LTE, DIAG_MSG_TYPE_MSP);
    stTrans.ulPid       = MSP_PID_DIAG_APP_AGENT;
    stTrans.ulMsgId     = OM_APP_ERRLOG_HOOK_IND | ulDateType;
    stTrans.ulLength    = ulLen;
    stTrans.pData       = pucData;

    DIAG_TransReport_Ex(&stTrans);

    return;
}


VOS_VOID OM_AcpuSendVComData(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    /* 调用注册函数，给Vcom发数据 */
    g_stErrLogVcomDebugInfo.ulVCOMSendNum++;
    g_stErrLogVcomDebugInfo.ulVCOMSendLen += ulLen;

    OM_ACPU_DEBUG_TRACE((VOS_UINT8*)pucData, ulLen, OM_ACPU_ERRLOG_SEND);
 

    /* 增加Trans勾包 */
    OM_AcpuErrLogHook(pucData, ulLen, OM_ERRLOG_SEND_MSG);

    if(VOS_OK != APP_VCOM_Send(APP_VCOM_DEV_INDEX_ERRLOG, pucData, ulLen))
    {
        g_stErrLogVcomDebugInfo.ulVCOMSendErrNum++;
        g_stErrLogVcomDebugInfo.ulVCOMSendErrLen += ulLen;

        
        (VOS_VOID)vos_printf("\r\n Info: OM_AcpuSendVComData: Nas Send Msg fail\n");
        

        return ;
    }
    return ;
}


VOS_VOID OM_AcpuSendFTMMsgOther(APP_OM_FTM_REQ_STRU *pstAppOmFtmReq, VOS_UINT32 ulLen, VOS_UINT32 ulPID)
{
    OM_FTM_REQUIRE_STRU                *pstOmFtmReq;

    if (VOS_NULL == ulPID)
    {
        return;
    }

    /* 工程模式命令上报通知对应的组件 */
    pstOmFtmReq  = (OM_FTM_REQUIRE_STRU*)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,
                                         (ulLen + sizeof(VOS_UINT32) + sizeof(VOS_UINT16)));

    if (VOS_NULL_PTR == pstOmFtmReq)
    {
        OM_AcpuSendAppResult(OM_APP_OMACPU_ALLOC_MSG_ERR, pstAppOmFtmReq->usModemID);
        
        (VOS_VOID)vos_printf("OM_AcpuSendFTMMsgOther:malloc msg fail\r\n");
        
        return ;
    }
    pstOmFtmReq->ulReceiverPid = ulPID;
    pstOmFtmReq->ulMsgName     = ID_OM_FTM_REQUIRE_IND;
    pstOmFtmReq->usModemID     = pstAppOmFtmReq->usModemID;
    
    (VOS_VOID)VOS_MemCpy_s((VOS_VOID*)pstOmFtmReq->aucContent, ulLen, (VOS_VOID*)pstAppOmFtmReq, ulLen);
    

    /* 提前回复消息 */
    OM_AcpuSendAppResult(OM_APP_MSG_OK, pstAppOmFtmReq->usModemID);

    (VOS_VOID)VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstOmFtmReq);

    return ;
}


VOS_INT OM_AcpuFTMMsgReq(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    APP_OM_FTM_REQ_STRU                *pstAppOmReqFtm;
    VOS_UINT32                          ulIndex;

    pstAppOmReqFtm = (APP_OM_FTM_REQ_STRU*)pucData;

    /* modem ID 检查*/
    if (MODEM_ID_0 == pstAppOmReqFtm->usModemID)
    {
        for (ulIndex=0; ulIndex<(sizeof(g_astModem0FTMMsgModule) / sizeof(g_astModem0FTMMsgModule[0])); ulIndex++)
        {
            if (pstAppOmReqFtm->ulMsgModuleID == g_astModem0FTMMsgModule[ulIndex].ulMsgModuleID)
            {
                OM_AcpuSendFTMMsgOther(pstAppOmReqFtm, ulLen, g_astModem0FTMMsgModule[ulIndex].ulPID);
                break;
            }
        }

        if ( (sizeof(g_astModem0FTMMsgModule) / sizeof(g_astModem0FTMMsgModule[0])) <= ulIndex )
        {
           
            (VOS_VOID)vos_printf("OM_AcpuFTMMsgReq:MsgModule ID is error %d\r\n", pstAppOmReqFtm->ulMsgModuleID);
            
            return OM_APP_MSG_MODULE_ID_ERR;
        }
    }

    else if (MODEM_ID_1 == pstAppOmReqFtm->usModemID)
    {
        for (ulIndex=0; ulIndex<(sizeof(g_astModem1FTMMsgModule) / sizeof(g_astModem1FTMMsgModule[0])); ulIndex++)
        {
            if (pstAppOmReqFtm->ulMsgModuleID == g_astModem1FTMMsgModule[ulIndex].ulMsgModuleID)
            {
                OM_AcpuSendFTMMsgOther(pstAppOmReqFtm, ulLen, g_astModem1FTMMsgModule[ulIndex].ulPID);
                break;
            }
        }

        if ( (sizeof(g_astModem1FTMMsgModule) / sizeof(g_astModem1FTMMsgModule[0])) <= ulIndex )
        {
            
            (VOS_VOID)vos_printf("OM_AcpuFTMMsgReq:MsgModule ID is error %d\r\n", pstAppOmReqFtm->ulMsgModuleID);
          
            return OM_APP_MSG_MODULE_ID_ERR;
        }
    }

    else if (2 == pstAppOmReqFtm->usModemID)
    {
        for (ulIndex=0; ulIndex<(sizeof(g_astModem2FTMMsgModule) / sizeof(g_astModem2FTMMsgModule[0])); ulIndex++)
        {
            if (pstAppOmReqFtm->ulMsgModuleID == g_astModem2FTMMsgModule[ulIndex].ulMsgModuleID)
            {
                OM_AcpuSendFTMMsgOther(pstAppOmReqFtm, ulLen, g_astModem2FTMMsgModule[ulIndex].ulPID);
                break;
            }
        }

        if ( (sizeof(g_astModem2FTMMsgModule) / sizeof(g_astModem2FTMMsgModule[0])) <= ulIndex )
        {
            
            (VOS_VOID)vos_printf("OM_AcpuFTMMsgReq:MsgModule ID is error %d\r\n", pstAppOmReqFtm->ulMsgModuleID);
            
            return OM_APP_MSG_MODULE_ID_ERR;
        }
    }
    else
    {
        
        (VOS_VOID)vos_printf("OM_AcpuFTMMsgReq: modem id error!\r\n ");
        
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrNum++;
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrLen += ulLen;
        OM_AcpuSendAppResult(OM_APP_SEND_MODEM_ID_ERR, pstAppOmReqFtm->usModemID);
        return OM_APP_SEND_MODEM_ID_ERR;
    }

    return OM_APP_MSG_OK;
}


VOS_UINT32 OM_AcpuRcvErrLogMsgCheck(OM_RCV_DATA_INFO_STRU *pstOmRcvDataInfo, VOS_UINT32 *pulSendPidCount)
{
    VOS_UINT32                            i;       
    /* Error Log 上报 */
    if (VOS_NULL_PTR == g_stOmAppMsgRecord.pstErrorLogModule)
    {
        
        (VOS_VOID)vos_printf("OM_AcpuRcvMsgCheck: not expect report Err log msg!\r\n ");
        
        return VOS_ERR;
    }     

    g_stRcvUESendAP.ulErrLogRcvNum++;
    g_stRcvUESendAP.ulErrLogRcvLen += pstOmRcvDataInfo->stOmHeader.ulMsgLen;
    
    /* 记录对应组件已上报消息 */
    for (i=0; i<g_stOmAppMsgRecord.ulAlarmIdNum; i++)
    {
        if ((g_stOmAppMsgRecord.pstErrorLogModule[i].ulPid == pstOmRcvDataInfo->ulSenderPid)
            && ((VOS_UINT8)g_stOmAppMsgRecord.pstErrorLogModule[i].ulAlarmID == pstOmRcvDataInfo->stOmHeader.ucAlarmId)
            && (OM_MSG_NO_RECEIVE_FLAG == g_ucErrRptFlag[i]))
        {   
            (*pulSendPidCount)++;
            g_ucErrRptFlag[i]=OM_MSG_RECEIVE_FLAG; 
            g_stRcvUESendAP.ulErrLogSendNum++;
            g_stRcvUESendAP.ulErrLogSendLen += pstOmRcvDataInfo->stOmHeader.ulMsgLen;
            
            g_stOmAppMsgRecord.ulErrLogReportSend--;

            break;
        }
    }
    

    return VOS_OK;
}


VOS_UINT32 OM_AcpuRcvFTMReportMsgCheck(OM_RCV_DATA_INFO_STRU *pstOmRcvDataInfo, VOS_UINT32 *pulSendPidCount)
{
    VOS_UINT32                          i;

    /* 工程模式主动上报 */
    if (VOS_NULL_PTR == g_stOmAppMsgRecord.pulFTMModule)
    {
        (VOS_VOID)vos_printf("OM_AcpuRcvMsgCheck: not expect report Ftm msg!\r\n ");
        
        return VOS_ERR;
    }

    /* 工程模式上报 */
    g_stRcvUESendAP.ulFTMRcvNum++;
    g_stRcvUESendAP.ulFTMRcvLen += pstOmRcvDataInfo->stOmHeader.ulMsgLen;
    for(i=0; i<OM_MAX_MODULE_ID; i++)
    {
        if ((g_stOmAppMsgRecord.pulFTMModule[i] == pstOmRcvDataInfo->ulSenderPid)
             && (0 !=(BIT_N(i) & g_stOmAppMsgRecord.ulFTMReportSend))
             && (OM_PID_NULL != g_stOmAppMsgRecord.pulFTMModule[i]))
        {
            (*pulSendPidCount)++;
            g_stRcvUESendAP.ulFTMSendNum++;
            g_stRcvUESendAP.ulFTMSendLen += pstOmRcvDataInfo->stOmHeader.ulMsgLen;

            break;
        }
    }


    return VOS_OK;
}


VOS_UINT32 OM_AcpuRcvFTMCnfMsgCheck(OM_RCV_DATA_INFO_STRU *pstOmRcvDataInfo, VOS_UINT32 *pulSendPidCount)
{
    OM_APP_FTM_CNF_STRU                *pstOmAppFtmCnf;
    VOS_UINT32                          ulIndex;

    /* 工程模式响应上报最小消息长度 */
    if ((4*sizeof(VOS_UINT32)) > pstOmRcvDataInfo->stOmHeader.ulMsgLen)
    {
       
        (VOS_VOID)vos_printf("OM_AcpuRcvFTMCnfMsgCheck:MsgModule ID is error %d\r\n", pstOmRcvDataInfo->stOmHeader.ulMsgLen);
        
        return OM_APP_MSG_LENGTH_ERR;
    }

    pstOmAppFtmCnf= (OM_APP_FTM_CNF_STRU*)(&pstOmRcvDataInfo->stOmHeader.usMsgType);

    if (MODEM_ID_0 == pstOmAppFtmCnf->usModemID)
    {
        for (ulIndex=0; ulIndex<(sizeof(g_astModem0FTMMsgModule) / sizeof(g_astModem0FTMMsgModule[0])); ulIndex++)
        {
            if ((g_astModem0FTMMsgModule[ulIndex].ulMsgModuleID == pstOmAppFtmCnf->ulMsgModuleID)
                && (g_astModem0FTMMsgModule[ulIndex].ulPID == pstOmRcvDataInfo->ulSenderPid))
            {
               (*pulSendPidCount)++;
                break;
            }
        }

        if ((sizeof(g_astModem0FTMMsgModule) / sizeof(g_astModem0FTMMsgModule[0])) <= ulIndex)
        {
            
            (VOS_VOID)vos_printf("OM_AcpuFTMMsgReq:MsgModule ID is error %d\r\n", pstOmAppFtmCnf->ulMsgModuleID);
           
            return OM_APP_MSG_MODULE_ID_ERR;
        }
    }
    else if (MODEM_ID_1 == pstOmAppFtmCnf->usModemID)
    {
        for (ulIndex=0; ulIndex<(sizeof(g_astModem1FTMMsgModule) / sizeof(g_astModem1FTMMsgModule[0])); ulIndex++)
        {
            if ((g_astModem1FTMMsgModule[ulIndex].ulMsgModuleID == pstOmAppFtmCnf->ulMsgModuleID)
                && (g_astModem1FTMMsgModule[ulIndex].ulPID == pstOmRcvDataInfo->ulSenderPid))
            {
               (*pulSendPidCount)++;
                break;
            }
        }

        if ((sizeof(g_astModem1FTMMsgModule) / sizeof(g_astModem1FTMMsgModule[0])) <= ulIndex)
        {
            
            (VOS_VOID)vos_printf("OM_AcpuFTMMsgReq:MsgModule ID is error %d\r\n", pstOmAppFtmCnf->ulMsgModuleID);
            
            return OM_APP_MSG_MODULE_ID_ERR;
        }
    }

    else if (2 == pstOmAppFtmCnf->usModemID)
    {
        for (ulIndex=0; ulIndex<(sizeof(g_astModem2FTMMsgModule) / sizeof(g_astModem2FTMMsgModule[0])); ulIndex++)
        {
            if ((g_astModem2FTMMsgModule[ulIndex].ulMsgModuleID == pstOmAppFtmCnf->ulMsgModuleID)
                && (g_astModem2FTMMsgModule[ulIndex].ulPID == pstOmRcvDataInfo->ulSenderPid))
            {
               (*pulSendPidCount)++;
                break;
            }
        }

        if ((sizeof(g_astModem2FTMMsgModule) / sizeof(g_astModem2FTMMsgModule[0])) <= ulIndex)
        {
            
            (VOS_VOID)vos_printf("OM_AcpuFTMMsgReq:MsgModule ID is error %d\r\n", pstOmAppFtmCnf->ulMsgModuleID);
            
            return OM_APP_MSG_MODULE_ID_ERR;
        }
    }
    else
    {
        return VOS_ERR;
    }

    return VOS_OK;

}

VOS_UINT32 OM_AcpuRcvMsgCheck(OM_RCV_DATA_INFO_STRU *pstOmRcvDataInfo)
{
    VOS_UINT32                          ulSendPidCount = 0;

    switch(pstOmRcvDataInfo->stOmHeader.usMsgType)
    {
        case OM_ERR_LOG_MSG_ERR_REPORT:
            /* Error Log 上报 */
            if(VOS_OK != OM_AcpuRcvErrLogMsgCheck(pstOmRcvDataInfo, &ulSendPidCount))
            {
                return VOS_ERR;
            }
            break;

        case OM_ERR_LOG_MSG_FTM_REPORT:
            /* 工程模式主动上报 */
            if(VOS_OK != OM_AcpuRcvFTMReportMsgCheck(pstOmRcvDataInfo, &ulSendPidCount))
            {
                return VOS_ERR;
            }
            break;

        case OM_ERR_LOG_MSG_FTM_CNF:
            /* 工程模式请求上报 */
            if(VOS_OK != OM_AcpuRcvFTMCnfMsgCheck(pstOmRcvDataInfo, &ulSendPidCount))
            {
                return VOS_ERR;
            }
            break;

        case OM_ERR_LOG_MSG_FAULT_REPORT:
            /* 平台检测故障主动上报 */
            ulSendPidCount++;
            break;

        case OM_ERR_LOG_MSG_ALARM_REPORT:
            /* 平台检测告警主动上报 */
            ulSendPidCount++;
            break;
        default:
            /* 异常 */
           
            (VOS_VOID)vos_printf("OM_AcpuRcvMsgCheck: Msg type error!\r\n ");
           
            return VOS_ERR;
    }

    /* 不是预期上报,丢弃消息 */
    if (OM_AP_NO_MSG_SEND == ulSendPidCount)
    {
        
        (VOS_VOID)vos_printf("OM_AcpuRcvMsgCheck: No msg Send!\r\n ");
        
        return VOS_ERR;
    }

    return VOS_OK;
}


VOS_VOID OM_AcpuErrLogInfoShow(VOS_VOID)
{
    
    (VOS_VOID)vos_printf("\r\nVCom Send sn is           %d", g_stErrLogVcomDebugInfo.ulVCOMSendSn);
    (VOS_VOID)vos_printf("\r\nVCom Send num is           %d", g_stErrLogVcomDebugInfo.ulVCOMSendNum);
    (VOS_VOID)vos_printf("\r\nVCom Send Len is           %d", g_stErrLogVcomDebugInfo.ulVCOMSendLen);

    (VOS_VOID)vos_printf("\r\nVCom Send Error num is         %d", g_stErrLogVcomDebugInfo.ulVCOMSendErrNum);
    (VOS_VOID)vos_printf("\r\nVCom Send Error Len is         %d\r\n", g_stErrLogVcomDebugInfo.ulVCOMSendErrLen);

    (VOS_VOID)vos_printf("\r\nVCom receive sn is           %d", g_stErrLogVcomDebugInfo.ulVCOMRcvSn);
    (VOS_VOID)vos_printf("\r\nVCom receive num is           %d", g_stErrLogVcomDebugInfo.ulVCOMRcvNum);
    (VOS_VOID)vos_printf("\r\nVCom receive Len is           %d", g_stErrLogVcomDebugInfo.ulVCOMRcvLen);

    (VOS_VOID)vos_printf("\r\nVCom receive Error num is         %d", g_stErrLogVcomDebugInfo.ulVCOMRcvErrNum);
    (VOS_VOID)vos_printf("\r\nVCom receive Error Len is         %d\r\n", g_stErrLogVcomDebugInfo.ulVCOMRcvErrLen);


    (VOS_VOID)vos_printf("\r\nOM Rcv Err log msg num is         %d", g_stRcvUESendAP.ulErrLogRcvNum);
    (VOS_VOID)vos_printf("\r\nOM Rcv Err log msg len is         %d", g_stRcvUESendAP.ulErrLogRcvLen);

    (VOS_VOID)vos_printf("\r\nOM Rcv FTM msg num is         %d", g_stRcvUESendAP.ulFTMRcvNum);
    (VOS_VOID)vos_printf("\r\nOM Rcv FTM msg len is         %d", g_stRcvUESendAP.ulFTMRcvLen);

    (VOS_VOID)vos_printf("\r\nOM Send Err log msg num is         %d", g_stRcvUESendAP.ulErrLogSendNum);
    (VOS_VOID)vos_printf("\r\nOM Send Err log msg len is         %d", g_stRcvUESendAP.ulErrLogSendLen);

    (VOS_VOID)vos_printf("\r\nOM Send FTM msg num is         %d", g_stRcvUESendAP.ulFTMSendNum);
    (VOS_VOID)vos_printf("\r\nOM Send FTM msg len is         %d\r\n", g_stRcvUESendAP.ulFTMSendLen);
    

    return;
}


VOS_VOID OM_AcpuRcvAPCtrl(VOS_UINT32 ultype,          VOS_UINT32 ulValuse,
                          VOS_UINT16 usModemID,       VOS_UINT8  ucAlmStatus,
                          VOS_UINT8  ucAlmLevel)
{
    APP_OM_CTRL_STATUS_STRU              stAppOmCtrlStatus;

    
    (VOS_VOID)VOS_MemSet_s(&stAppOmCtrlStatus, sizeof(stAppOmCtrlStatus), 0, (VOS_UINT32)sizeof(APP_OM_CTRL_STATUS_STRU));
    

    stAppOmCtrlStatus.stOmHeader.ulMsgType = ultype;
    stAppOmCtrlStatus.stOmHeader.ulMsgSN   = VOS_OK;
    stAppOmCtrlStatus.stOmHeader.ulMsgLen  = 2*sizeof(VOS_UINT32);
    stAppOmCtrlStatus.ulMsgModuleID        = ulValuse;
    stAppOmCtrlStatus.usModemID            = usModemID;
    stAppOmCtrlStatus.ucAlmStatus          = ucAlmStatus;
    stAppOmCtrlStatus.ucAlmLevel           = ucAlmLevel;

    
    (VOS_VOID)OM_AcpuReadVComData(APP_VCOM_DEV_INDEX_ERRLOG, (VOS_UINT8*)&stAppOmCtrlStatus, sizeof(APP_OM_CTRL_STATUS_STRU));
    
}



VOS_VOID OM_AcpuReportErrLog(VOS_UINT16 usModemID, VOS_UINT16 usFaultId)
{
    APP_OM_REQ_ERR_LOG_STRU              stAppOmCtrlStatus;

    
    (VOS_VOID)VOS_MemSet_s(&stAppOmCtrlStatus, sizeof(stAppOmCtrlStatus), 0, (VOS_UINT32)(sizeof(APP_OM_REQ_ERR_LOG_STRU)));
    

    stAppOmCtrlStatus.stOmHeader.ulMsgType = OM_ERR_LOG_MSG_SET_ERR_REPORT;
    stAppOmCtrlStatus.stOmHeader.ulMsgSN   = 2;
    stAppOmCtrlStatus.stOmHeader.ulMsgLen  = 2*sizeof(VOS_UINT32);
    stAppOmCtrlStatus.ulMsgModuleID        = 0x01;
    stAppOmCtrlStatus.usModemID            = usModemID;
    stAppOmCtrlStatus.usFaultId            = usFaultId;

    
    (VOS_VOID)OM_AcpuReadVComData(APP_VCOM_DEV_INDEX_ERRLOG, (VOS_UINT8*)&stAppOmCtrlStatus, sizeof(APP_OM_REQ_ERR_LOG_STRU));
    
}

/* 设置采集信息回复消息不需要处理标志 */
VOS_VOID OM_AcpuCltInfoCnfNotNeedProcessSetFlag(VOS_VOID)
{
    g_AcpuCtlInfoCnfNotNeedProcess = OM_CLTINFO_CNF_NOT_NEED_PROCESS;
}

/* 复位采集信息回复消息不需要处理标志 */
VOS_VOID OM_AcpuCltInfoCnfNotNeedProcessReSetFlag(VOS_VOID)
{
    g_AcpuCtlInfoCnfNotNeedProcess = OM_CLTINFO_CNF_NEED_PROCESS;
}
/* 获取采集信息回复消息不需要处理标志 */
VOS_UINT32 OM_AcpuCltInfoCnfGetNotNeedProcessFlag(VOS_VOID)
{
    return g_AcpuCtlInfoCnfNotNeedProcess;
}

/* 通过modemID获对应的MTA的pid ，当前只将请求发给MTA */
VOS_UINT32 OM_AcpuCltInfoGetMTAPid(VOS_UINT32 modemId)
{
    if (modemId == 0)
        return I0_UEPS_PID_MTA;
    else if (modemId == 1)
        return I1_UEPS_PID_MTA;
    else if (modemId == 2)
        return I2_UEPS_PID_MTA;
    else
        return OM_CLTINFO_INVALID_PID;
}


/* OM收到采集信息请求消息处理 */
VOS_INT OM_AcpuCltInfoReqMsgProc(VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    APP_OM_INFO_CLT_REQ_STRU *pOMCltInfo = NULL;
    OM_INFO_CLT_REPORT_REQ_STRU *pVOSCltInfo = NULL;
    VOS_UINT allocSize = 0;

    if(!pucData)
    {   
        vos_printf("OM_AcpuCltInfoReqMsgProc: pucData null!\n");
        return VOS_ERR;
    }
    if(ulLen < sizeof(APP_OM_INFO_CLT_REQ_STRU)-sizeof(pOMCltInfo->aucMsgContent))
    {
        vos_printf("OM_AcpuCltInfoReqMsgProc:ulLen too short!\n");
        return VOS_ERR;
    } 
    pOMCltInfo = (APP_OM_INFO_CLT_REQ_STRU *)pucData;
    if(ulLen - (sizeof(APP_OM_INFO_CLT_REQ_STRU)-sizeof(pOMCltInfo->aucMsgContent)) < pOMCltInfo->ulMsgLength )
    {
        vos_printf("OM_AcpuCltInfoReqMsgProc:ulLen too short!\n");
        return VOS_ERR;
    }
    allocSize = sizeof(OM_INFO_CLT_REPORT_REQ_STRU) - VOS_MSG_HEAD_LENGTH - sizeof(pOMCltInfo->aucMsgContent) + pOMCltInfo->ulMsgLength;
    pVOSCltInfo = (OM_INFO_CLT_REPORT_REQ_STRU *)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT, allocSize);

    if (!pVOSCltInfo) {
        vos_printf("OM_AcpuCltInfoReqMsgProc: null pointer pVOSCltInfo!\n");
        return VOS_ERR;
    }

    pVOSCltInfo->ulReceiverPid = OM_AcpuCltInfoGetMTAPid(pOMCltInfo->usModemID);

    if (pVOSCltInfo->ulReceiverPid == OM_CLTINFO_INVALID_PID) {
        vos_printf("OM_AcpuCltInfoReqMsgProc: OM_AcpuCltInfoGetMTAPid failed\n");
        VOS_FreeMsg(MSP_PID_DIAG_APP_AGENT, pVOSCltInfo);
        return OM_APP_SEND_MODEM_ID_ERR;
    }
    pVOSCltInfo->ulMsgName     = ID_OM_INFO_CLT_REPORT_REQ;
    pVOSCltInfo->usModemID     = pOMCltInfo->usModemID;
    pVOSCltInfo->enSceneType   = pOMCltInfo->usInfoID;
    pVOSCltInfo->ulMsgLen      = pOMCltInfo->ulMsgLength;
    VOS_MemCpy_s(pVOSCltInfo->aucContent, pVOSCltInfo->ulMsgLen, pOMCltInfo->aucMsgContent, pVOSCltInfo->ulMsgLen);

    (VOS_VOID)VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pVOSCltInfo);
    OM_AcpuCltInfoCnfNotNeedProcessReSetFlag();
    g_stOmAppMsgRecord.usModemId=pOMCltInfo->usModemID;

    /* 起5s定时器 */
    g_AcpuCltInfoFullTmr = VOS_NULL_PTR;
    if (VOS_OK != VOS_StartRelTimer(&g_AcpuCltInfoFullTmr, MSP_PID_DIAG_APP_AGENT, OM_CLTINFO_TIMER_LENTH,
                                DIAG_CLTINFO_TIMER_NAME, DIAG_CLTINFO_TIMER_PARA, VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5))
    {
        vos_printf("\r\n OM_AcpuCltInfoReqMsgProc: VOS_StartRelTimer fail\n");
        return OM_APP_ERRLOG_START_TIMER_ERR;
    }

    return VOS_OK;
}

/* 采集信息回复消息结束消息，msgType 0x11 */
VOS_VOID OM_AcpuRcvCltInfoFinish(VOS_VOID)
{
    OM_APP_REPORT_STATUS_STRU               stOmAppReportStatus = {};

    stOmAppReportStatus.stOmHeader.ulMsgType = OM_ERR_LOG_MSG_INFO_CLT_END;
    stOmAppReportStatus.stOmHeader.ulMsgSN   = g_stErrLogVcomDebugInfo.ulVCOMSendSn++;
    stOmAppReportStatus.stOmHeader.ulMsgLen  = (sizeof(OM_APP_REPORT_STATUS_STRU) - sizeof(OM_ALARM_MSG_HEAD_STRU));
    stOmAppReportStatus.usModemID            = g_stOmAppMsgRecord.usModemId;
    stOmAppReportStatus.ulReportStatus       = 0;
    stOmAppReportStatus.usFaultID            = g_stOmAppMsgRecord.usFaultId;
    stOmAppReportStatus.ulTrigLowSlice       = (VOS_UINT32)(g_ulTriggerTime64Bit & 0xffffffff);
    stOmAppReportStatus.ulTrigHighSlice      = (VOS_UINT32)((g_ulTriggerTime64Bit >> 32) & 0xffffffff);

    OM_AcpuSendVComData((VOS_UINT8 *)&stOmAppReportStatus, sizeof(OM_APP_REPORT_STATUS_STRU));

    return ;
}
 
/* 采集信息下发请求消息打桩函数，用于测试 */
VOS_VOID OM_AcpuReportCltInfo(VOS_UINT16 modemID, VOS_UINT16 infoId, VOS_UINT32 mcc, VOS_UINT32 mnc)
{
    APP_OM_INFO_CLT_REQ_STRU *cltInfoReq = NULL;
    OM_INFO_CLT_PLMN_ID_STRU *cltInfoPLMN = NULL;
    
    void *cltInfoalloc = NULL;
    VOS_UINT32 allocSize = sizeof(APP_OM_INFO_CLT_REQ_STRU) + 4; /* mcc/mnc 共8字节 */

    cltInfoalloc  = VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT, DYNAMIC_MEM_PT, allocSize);

    if (!cltInfoalloc){
        vos_printf("OM_AcpuReportCltInfo: failed to malloc\n");
        return;
    }
   
    (VOS_VOID)VOS_MemSet_s(cltInfoalloc, allocSize, 0, allocSize);
   
    cltInfoReq = (APP_OM_INFO_CLT_REQ_STRU *)cltInfoalloc;
    cltInfoReq->stOmHeader.ulMsgType = 0x10;
    cltInfoReq->stOmHeader.ulMsgSN   = 2;
    cltInfoReq->stOmHeader.ulMsgLen  = 4*sizeof(VOS_UINT32);

    cltInfoReq->usModemID            = modemID;
    cltInfoReq->usInfoID             = infoId;
    cltInfoReq->ulMsgLength          = 8;

    cltInfoPLMN = (OM_INFO_CLT_PLMN_ID_STRU *)(cltInfoReq->aucMsgContent);
    cltInfoPLMN->ulMcc = mcc;
    cltInfoPLMN->ulMnc = mnc;
    
    
    (VOS_VOID)OM_AcpuReadVComData(APP_VCOM_DEV_INDEX_ERRLOG, (VOS_UINT8*)cltInfoalloc,allocSize);
    (VOS_VOID)VOS_MemFree(MSP_PID_DIAG_APP_AGENT, cltInfoalloc);
 
    return;
}
 


VOS_VOID OM_AcpuErrLogReqMsgProc(MsgBlock* pMsg)
{
    OM_ERR_LOG_REQ_STRU *pReq = (OM_ERR_LOG_REQ_STRU*)pMsg;

    OM_AcpuReportErrLogMsg(pReq->aucData, pReq->ulMsgLen);

    return;
}


VOS_VOID OM_AcpuErrLogMsgProc(MsgBlock* pMsg)
{
    OM_RCV_DATA_INFO_STRU              *pstOmRcvDataInfo;

    pstOmRcvDataInfo = (OM_RCV_DATA_INFO_STRU*)pMsg;
    
    /* 判断消息ID是否为规定消息ID */
    if ((ID_OM_FTM_REPROT_IND != pstOmRcvDataInfo->ulMsgName)
         && (ID_OM_ERR_LOG_REPORT_CNF != pstOmRcvDataInfo->ulMsgName)
         && (ID_OM_FTM_REQUIRE_CNF != pstOmRcvDataInfo->ulMsgName)
         && (ID_OM_FAULT_ERR_LOG_IND != pstOmRcvDataInfo->ulMsgName)
         && (ID_OM_ALARM_ERR_LOG_IND != pstOmRcvDataInfo->ulMsgName))
    {
         
        (VOS_VOID)vos_printf("OM_AcpuErrLogMsgProc: not error Log msg.\n");
         
        return ;
    }
    /*  收到消息检查是否需要上报 */
    if (VOS_OK != OM_AcpuRcvMsgCheck(pstOmRcvDataInfo))
     {
         
        (VOS_VOID)vos_printf("OM_AcpuErrLogMsgProc: rcv msg error!\r\n ");
         
        return ;
    }

    pstOmRcvDataInfo->stOmHeader.ulMsgSN = g_stErrLogVcomDebugInfo.ulVCOMSendSn++;

    OM_AcpuSendVComData((VOS_UINT8 *)&pstOmRcvDataInfo->stOmHeader.usMsgType,
                        pstOmRcvDataInfo->stOmHeader.ulMsgLen + sizeof(OM_ERR_MSG_HEAD_STRU));

   
    (void)vos_printf("OM_AcpuErrLogMsgProc: rcv msg Send Success,Msgname 0x%x, Type 0x%x, Pid 0x%x, Cnt 0x%x\n",
        pstOmRcvDataInfo->ulMsgName,
        pstOmRcvDataInfo->stOmHeader.usMsgType,
        pstOmRcvDataInfo->ulSenderPid,
        g_stOmAppMsgRecord.ulErrLogReportSend);
     
    if ((OM_ERR_LOG_MSG_ERR_REPORT == pstOmRcvDataInfo->stOmHeader.usMsgType)
        && ( OM_AP_SEND_MSG_FINISH==g_stOmAppMsgRecord.ulErrLogReportSend))
    {
        /* 停定时器 */
         
        (void)VOS_StopRelTimer(&g_AcpuErrLogFullTmr);
        
        OM_AcpuRcvMsgFinish();
        (void)vos_printf("OM_AcpuErrLogMsgProc: Report all msg, Stop timer\n");
    }
     


    return ;
}
VOS_VOID OM_AcpuErrLogTimeoutShowPid(VOS_VOID)
{
    VOS_UINT32 i;
    
    if (VOS_NULL_PTR == g_stOmAppMsgRecord.pstErrorLogModule)
    {
        (void)vos_printf("OM_AcpuErrLogTimeoutShowPid: nothing to show!\n");
        return;
    }     

    /* 记录对应组件已上报消息 */
    for (i=0; i<g_stOmAppMsgRecord.ulAlarmIdNum; i++)
    {
        if (0 == g_ucErrRptFlag[i])
        {
            (void)vos_printf("OM_AcpuErrLogTimeoutShowPid: Pid 0x%x, AlarmId 0x%x!\n", 
                             g_stOmAppMsgRecord.pstErrorLogModule[i].ulPid,
                             g_stOmAppMsgRecord.pstErrorLogModule[i].ulAlarmID);
        }
    }
    return;
}


VOS_VOID OM_AcpuErrLogTimeoutProc(VOS_VOID)
{
    OM_AcpuErrLogTimeoutShowPid();
    OM_AcpuRcvMsgFinish();
    (void)vos_printf("OM_AcpuErrLogTimeoutProc: OM not receive all msg,timer is on!\n");

    return ;
}


VOS_INT OM_AcpuReadVComData(VOS_UINT8 ucDevIndex, VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    OM_ALARM_MSG_HEAD_STRU             *pstOmAlarmMsgHead;

    if (APP_VCOM_DEV_INDEX_ERRLOG != ucDevIndex)
    {
         
        (VOS_VOID)vos_printf("\r\n OM_AcpuReadVComData:PhyPort port is error: %d\n", ucDevIndex);
         
        OM_AcpuSendAppResult(OM_APP_VCOM_DEV_INDEX_ERR, MODEM_ID_BUTT);

        return VOS_ERR;
    }

    down(&g_stOmRxErrorLogBuffSem);

    g_stErrLogVcomDebugInfo.ulVCOMRcvNum++;
    g_stErrLogVcomDebugInfo.ulVCOMRcvLen += ulLen;

    if ((VOS_NULL_PTR == pucData) || (sizeof(APP_OM_CTRL_STATUS_STRU)> ulLen)||(ulLen > OM_ACPU_CLT_REQ_MSGLEN))
    {
       
        (VOS_VOID)vos_printf("\r\n OM_AcpuReadVComData:Send data is NULL or len is error:%d\n", ulLen);
         
        OM_AcpuSendAppResult(OM_APP_MSG_LENGTH_ERR, MODEM_ID_BUTT);
        up(&g_stOmRxErrorLogBuffSem);
        return VOS_ERR;
    }

    OM_ACPU_DEBUG_TRACE((VOS_UINT8*)pucData, ulLen, OM_ACPU_ERRLOG_RCV);

    /* 增加Trans勾包 */
    OM_AcpuErrLogHook(pucData, ulLen, OM_ERRLOG_RCV_MSG);

    /* 根据消息头判断命令类型 */
    pstOmAlarmMsgHead = (OM_ALARM_MSG_HEAD_STRU *)pucData;
    
    if ( pstOmAlarmMsgHead->ulMsgLen >OM_ACPU_CLT_REQ_MSGLEN - sizeof(OM_ALARM_MSG_HEAD_STRU))
    {
        (VOS_VOID)vos_printf("\r\n OM_AcpuReadVComData:msgthlen is err %d\n",pstOmAlarmMsgHead->ulMsgLen);
        OM_AcpuSendAppResult(OM_APP_VCOM_DEV_INDEX_ERR, MODEM_ID_BUTT);
        up(&g_stOmRxErrorLogBuffSem);
        return VOS_ERR;
        
    }
    g_stErrLogVcomDebugInfo.ulVCOMRcvSn = pstOmAlarmMsgHead->ulMsgSN;

    if (OM_ERR_LOG_MSG_ON_OFF == pstOmAlarmMsgHead->ulMsgType)
    {
        up(&g_stOmRxErrorLogBuffSem);
        return OM_AcpuSwitchOnOff(pucData, ulLen);
    }
    else if(OM_ERR_LOG_MSG_SET_ERR_REPORT == pstOmAlarmMsgHead->ulMsgType)
    {
        up(&g_stOmRxErrorLogBuffSem);
        return OM_AcpuErrLogReqProc(pucData, ulLen);
    }
    else if(OM_ERR_LOG_MSG_FTM_REQ == pstOmAlarmMsgHead->ulMsgType)
    {
        up(&g_stOmRxErrorLogBuffSem);
        return OM_AcpuFTMMsgReq(pucData, ulLen);
    }
    else if(OM_ERR_LOG_MSG_INFO_CLT_REQ == pstOmAlarmMsgHead->ulMsgType)
    {
        up(&g_stOmRxErrorLogBuffSem);
        return OM_AcpuCltInfoReqMsgProc(pucData, ulLen);
    }
    else
    {
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrNum++;
        g_stErrLogVcomDebugInfo.ulVCOMRcvErrLen += ulLen;
         
        (VOS_VOID)vos_printf("\r\n OM_AcpuReadVComData:Recv msg type is err\n");
         
        OM_AcpuSendAppResult(OM_APP_MSG_TYPE_ERR, MODEM_ID_BUTT);
    }

    up(&g_stOmRxErrorLogBuffSem);


    return VOS_ERR;
}



VOS_VOID GU_OamErrLogVComPortInit(VOS_VOID)
{
    VOS_SpinLockInit(&g_stVosErrLogSendSpinLock);

    /* 商用ERR LOG上报全局变量初始化 */
    g_stOmAppMsgRecord.ulErrLogReportSend = 0;/* 记录Err Log需要上报组件 */
    g_stOmAppMsgRecord.ulFTMReportSend    = 0;/* 记录FTM需要上报组件 */
    g_stOmAppMsgRecord.pstErrorLogModule  = VOS_NULL_PTR;
    g_stOmAppMsgRecord.pulFTMModule       = VOS_NULL_PTR;
    g_stOmAppMsgRecord.usModemId          = MODEM_ID_BUTT;
    g_stOmAppMsgRecord.ulErrLogState      = 0;

    sema_init(&g_stOmRxErrorLogBuffSem, 1);

    /* 注册收Vcom Error log函数给NAS */
     
    APP_VCOM_RegDataCallback(APP_VCOM_DEV_INDEX_ERRLOG, OM_AcpuReadVComData);
     
    return;
}

/* 采集信息回复消息处理函数 */
VOS_VOID OM_AcpuCltInfoCnfMsgProc(MsgBlock* pMsg)
{
    OM_INFO_CLT_REPORT_CNF_STRU *pCltInfoCnf = NULL;
    VOS_UINT16 sendSize = 0;

    pCltInfoCnf = (OM_INFO_CLT_REPORT_CNF_STRU *)pMsg;
    if ((!pCltInfoCnf) || (ID_OM_INFO_CLT_REPORT_CNF != pCltInfoCnf->ulMsgName))
    {
        return ;
    }

    pCltInfoCnf->ulMsgSN = g_stErrLogVcomDebugInfo.ulVCOMSendSn++;
    sendSize = pCltInfoCnf->ulRptLen + sizeof(OM_ALARM_MSG_HEAD_STRU);

    if (OM_AcpuCltInfoCnfGetNotNeedProcessFlag() == OM_CLTINFO_CNF_NEED_PROCESS)
    {
        OM_AcpuSendVComData((VOS_UINT8*)&pCltInfoCnf->ulMsgType, sendSize);

        if (OM_ERR_LOG_MSG_INFO_CLT_CNF == pCltInfoCnf->ulMsgType)
        {
            /* 停定时器 */
            VOS_StopRelTimer(&g_AcpuCltInfoFullTmr);
            OM_AcpuRcvCltInfoFinish();
        }
    }
    return;
}


