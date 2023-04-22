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

#ifndef __ATCMDMSGPROC_H__
#define __ATCMDMSGPROC_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include  "vos.h"
#include  "AtTypeDef.h"
#include  "AtCtx.h"
#include  "AcpuReset.h"
#include  "TafDrvAgent.h"
#include  "AtMtaInterface.h"
#include  "AtInternalMsg.h"
#if (FEATURE_ON == FEATURE_IMS)
#include  "AtImsaInterface.h"
#endif

#include  "TafAppMma.h"

#include "AtXpdsInterface.h"

#include "TafAppCall.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define         TAF_AT_INVALID_MCC          (0xffffffff)


/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/* Added by f62575 for AT Project, 2011/10/19, begin */
extern VOS_INT8                         g_acATE5DissdPwd[AT_DISSD_PWD_LEN+1];
/* Added by f62575 for AT Project, 2011/10/19, end */


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/*消息处理函数指针*/
typedef VOS_UINT32 (*pAtProcMsgFromDrvAgentFunc)(VOS_VOID *pMsg);

/*AT与MTA模块间消息处理函数指针*/
typedef VOS_UINT32 (*AT_MTA_MSG_PROC_FUNC)(VOS_VOID *pMsg);


/*AT与MMA模块间消息处理函数指针*/
typedef VOS_UINT32 (*AT_MMA_MSG_PROC_FUNC)(VOS_VOID *pMsg);

typedef VOS_UINT32 (*AT_XCALL_MSG_PROC_FUNC)(VOS_VOID *pMsg);

/*****************************************************************************
 结构名    : DRV_AGENT_MSG_PROC_STRU
 结构说明  : 消息与对应处理函数的结构
*****************************************************************************/
/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    DRV_AGENT_MSG_TYPE_ENUM_UINT32       ulMsgType;
    pAtProcMsgFromDrvAgentFunc           pProcMsgFunc;
}AT_PROC_MSG_FROM_DRV_AGENT_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*****************************************************************************
 Structure      : NAS_AT_OUTSIDE_RUNNING_CONTEXT_PART_ST
 Description    : PC回放工程，存储所有AT相关的全局变量，目前仅有短信相关全局变量
 Message origin :
 Note:
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucUsed;                                 /* 指示当前索引是否已被使用 */
    AT_USER_TYPE                        UserType;                               /* 指示当前用户类型 */
    AT_MODE_TYPE                        Mode;                                   /* 指示当前命令模式，只针对MUX和APP */
    AT_IND_MODE_TYPE                    IndMode;                                /* 指示当前命令模式，只针对MUX和APP */
    VOS_UINT16                          usClientId;                             /* 指示当前用户的 */
    MN_OPERATION_ID_T                   opId;                                   /* Operation ID, 标识本次操作             */
    VOS_UINT8                           aucReserved[1];
}NAS_AT_CLIENT_MANAGE_SIMPLE_STRU;

/*****************************************************************************
 Structure      : NAS_AT_SDT_AT_CLIENT_TABLE_STRU
 Description    : PC回放工程，所有AT相关的全局变量通过以下消息结构发送
 Message origin :
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    AT_INTER_MSG_ID_ENUM_UINT32             enMsgID;
    VOS_UINT8                               ucType;
    VOS_UINT8                               aucReserved[3];                     /* 在PACK(1)到PACK(4)调整中定义的保留字节 */
    NAS_AT_CLIENT_MANAGE_SIMPLE_STRU        gastAtClientTab[AT_MAX_CLIENT_NUM];
}NAS_AT_SDT_AT_CLIENT_TABLE_STRU;

typedef struct
{
    AT_CSCS_TYPE                        gucAtCscsType;
    AT_CSDH_TYPE                        gucAtCsdhType;
    MN_OPERATION_ID_T                   g_OpId;
    MN_MSG_CSMS_MSG_VERSION_ENUM_U8     g_enAtCsmsMsgVersion;
    AT_CNMI_TYPE_STRU                   gstAtCnmiType;
    AT_CMGF_MSG_FORMAT_ENUM_U8          g_enAtCmgfMsgFormat;
    VOS_UINT8                           aucReserved[3];                         /* 在PACK(1)到PACK(4)调整中定义的保留字节 */
    AT_CGSMS_SEND_DOMAIN_STRU           g_stAtCgsmsSendDomain;
    AT_CSCA_CSMP_INFO_STRU              g_stAtCscaCsmpInfo;
    AT_MSG_CPMS_STRU                    g_stAtCpmsInfo;
}NAS_AT_OUTSIDE_RUNNING_CONTEXT_PART_ST;

/*****************************************************************************
 Structure      : NAS_AT_SDT_AT_PART_ST
 Description    : PC回放工程，所有AT相关的全局变量通过以下消息结构发送
 Message origin :
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER
    AT_INTER_MSG_ID_ENUM_UINT32             enMsgID; /* 匹配AT_MSG_STRU消息中的ulMsgID  */
    VOS_UINT8                               ucType;  /* 之前是ucMsgIDs */
    VOS_UINT8                               aucReserved[3];                     /* 在PACK(1)到PACK(4)调整中定义的保留字节 */
    NAS_AT_OUTSIDE_RUNNING_CONTEXT_PART_ST  astOutsideCtx[MODEM_ID_BUTT];
}NAS_AT_SDT_AT_PART_ST;

/*****************************************************************************
 结构名    : AT_PROC_MSG_FROM_MTA_STRU
 结构说明  : AT与MTA消息与对应处理函数的结构
*****************************************************************************/
/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_MTA_MSG_TYPE_ENUM_UINT32         ulMsgType;
    AT_MTA_MSG_PROC_FUNC                pProcMsgFunc;
}AT_PROC_MSG_FROM_MTA_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*****************************************************************************
 结构名    : AT_PROC_MSG_FROM_MTA_STRU
 结构说明  : AT与MTA消息与对应处理函数的结构
*****************************************************************************/
/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    VOS_UINT32                          ulMsgName;
    AT_MMA_MSG_PROC_FUNC                pProcMsgFunc;
}AT_PROC_MSG_FROM_MMA_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/


/*****************************************************************************
 结构名    : AT_PROC_MSG_FROM_CALL_STRU
 结构说明  : AT与XCALL消息与对应处理函数的结构
*****************************************************************************/
/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    TAF_CCA_MSG_TYPE_ENUM_UINT32        ulMsgName;
    AT_XCALL_MSG_PROC_FUNC              pProcMsgFunc;
}AT_PROC_MSG_FROM_CALL_STRU;

/*****************************************************************************
 结构名    : AT_PROC_MSG_FROM_XPDS_STRU
 结构说明  : AT与XPDS消息与对应处理函数的结构
*****************************************************************************/
/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_XPDS_MSG_TYPE_ENUM_UINT32        ulMsgType;
    AT_MMA_MSG_PROC_FUNC                pProcMsgFunc;
}AT_PROC_MSG_FROM_XPDS_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
/* Modified by l60609 for DSDA Phase III, 2013-3-5, Begin */
VOS_UINT32 AT_FormatAtiCmdQryString(
    MODEM_ID_ENUM_UINT16                enModemId,
    DRV_AGENT_MSID_QRY_CNF_STRU         *pstDrvAgentMsidQryCnf
);
/* Modified by l60609 for DSDA Phase III, 2013-3-5, End */
VOS_UINT32 AT_RcvDrvAgentMsidQryCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentGasMntnCmdRsp(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentVertimeQryRsp(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentYjcxSetCnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentYjcxQryCnf(VOS_VOID *pMsg);

VOS_UINT32 At_RcvAtCcMsgStateQryCnfProc(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentHardwareQryRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentFullHardwareQryRsp(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentSetRxdivCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentQryRxdivCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentSetSimlockCnf(VOS_VOID *pMsg);

VOS_VOID At_QryEonsUcs2RspProc(
    VOS_UINT8                           ucIndex,
    VOS_UINT8                           OpId,
    VOS_UINT32                          ulResult,
    TAF_MMA_EONS_UCS2_PLMN_NAME_STRU   *stEonsUcs2PlmnName,
    TAF_MMA_EONS_UCS2_HNB_NAME_STRU    *stEonsUcs2HNBName
);
VOS_UINT32 AT_RcvMmaEonsUcs2Cnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMmaCmmSetCmdRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvAtMmaUsimStatusInd(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentSetNvRestoreCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentQryNvRestoreRstCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentNvRestoreManuDefaultRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_DeciDigit2Ascii(
    VOS_UINT8                           aucDeciDigit[],
    VOS_UINT32                          ulLength,
    VOS_UINT8                           aucAscii[]
);

VOS_VOID AT_ConvertImsiDigit2String(
    VOS_UINT8                          *pucImsi,
    VOS_UINT8                          *pucImsiString
);


/* Modified by l60609 for DSDA Phase III, 2013-3-5, Begin */
VOS_UINT32  AT_GetImeiValue(
    MODEM_ID_ENUM_UINT16                enModemId,
    VOS_UINT8 aucImei[TAF_PH_IMEI_LEN + 1]
);
/* Modified by l60609 for DSDA Phase III, 2013-3-5, End */
VOS_BOOL AT_IsSimLockPlmnInfoValid(VOS_VOID);

VOS_UINT32 AT_RcvDrvAgentSetGpioplRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentQryGpioplRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentSetDatalockRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentQryTbatvoltRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentQryVersionRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentQrySecuBootRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentSetFchanRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentQrySfeatureRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentQryProdtypeRsp(VOS_VOID * pMsg);


extern VOS_VOID At_CmdMsgDistr(AT_MSG_STRU *pstMsg);

extern VOS_VOID At_CovertMsInternalRxDivParaToUserSet(
    VOS_UINT16                          usCurBandSwitch,
    VOS_UINT32                         *pulUserDivBandsLow,
    VOS_UINT32                         *pulUserDivBandsHigh
);

extern VOS_UINT32 AT_RcvDrvAgentSetAdcRsp(VOS_VOID *pMsg);

/* Modified by f62575 for B050 Project, 2012-2-3, begin   */
extern VOS_BOOL AT_E5CheckRight(
    VOS_UINT8                           ucIndex,
    VOS_UINT8                          *pucData,
    VOS_UINT16                          usLen
);
/* Modified by f62575 for B050 Project, 2012-2-3, end */

/* Added by f62575 for SMALL IMAGE, 2012-1-3, Begin   */
VOS_UINT32 AT_RcvDrvAgentTseLrfSetRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentHkAdcGetRsp(VOS_VOID *pMsg);

/* Added by f62575 for SMALL IMAGE, 2012-1-3, End     */
VOS_UINT32 AT_RcvDrvAgentQryTbatRsp(VOS_VOID *pMsg);

#if (FEATURE_ON == FEATURE_SECURITY_SHELL)
VOS_UINT32 AT_RcvDrvAgentSetSpwordRsp(VOS_VOID *pMsg);
#endif
/* Added by f62575 for B050 Project, 2012-2-3, Begin   */
VOS_UINT32 AT_RcvDrvAgentSetSecuBootRsp(VOS_VOID *pMsg);
/* Added by f62575 for B050 Project, 2012-2-3, end   */

extern VOS_UINT32 AT_RcvMmaCipherInfoQueryCnf(VOS_VOID *pMsg);
extern VOS_UINT32 AT_RcvMmaLocInfoQueryCnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentNvBackupStatQryRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentNandBadBlockQryRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentNandDevInfoQryRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentChipTempQryRsp(VOS_VOID *pMsg);


/* Added by h59254 for SAR Project, 2012/04/24, begin */
VOS_UINT32 AT_RcvDrvAgentAntStateIndRsp(VOS_VOID *pMsg);
/* Added by h59254 for SAR Project, 2012/04/24, end */


VOS_VOID  AT_ReadSystemAppConfigNV(VOS_VOID);


VOS_UINT32 AT_RcvMmaOmMaintainInfoInd(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvDrvAgentSetMaxLockTmsRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentSetApSimstRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvDrvAgentHukSetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentFacAuthPubkeySetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentIdentifyStartSetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentIdentifyEndSetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentSimlockDataWriteSetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentPhoneSimlockInfoQryCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentSimlockDataReadQryCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentPhonePhynumSetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentPhonePhynumQryCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentPortctrlTmpSetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentPortAttribSetCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentPortAttribSetQryCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvDrvAgentOpwordSetCnf(VOS_VOID *pMsg);

extern VOS_UINT32 AT_RcvMtaCposSetCnf(VOS_VOID *pMsg);
extern VOS_UINT32 AT_RcvMtaCposrInd(VOS_VOID *pMsg);
extern VOS_UINT32 AT_RcvMtaXcposrRptInd(VOS_VOID *pMsg);
extern VOS_UINT32 AT_RcvMtaCgpsClockSetCnf(VOS_VOID *pMsg);
extern VOS_VOID At_ProcMtaMsg(AT_MTA_MSG_STRU *pMsg);

extern VOS_VOID AT_Rpt_NV_Read( VOS_VOID );

extern VOS_UINT32 AT_RcvMtaSimlockUnlockSetCnf( VOS_VOID *pMsg );

VOS_UINT32 AT_RcvMtaQryNmrCnf( VOS_VOID *pMsg );

VOS_UINT32 AT_RcvMtaWrrAutotestQryCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrCellinfoQryCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrMeanrptQryCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrCellSrhSetCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrCellSrhQryCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrFreqLockSetCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrFreqLockQryCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrRrcVersionSetCnf( VOS_VOID *pMsg );
VOS_UINT32 AT_RcvMtaWrrRrcVersionQryCnf( VOS_VOID *pMsg );

VOS_UINT32 AT_RcvMmaAcInfoQueryCnf(VOS_VOID *pstMsg);


/* Modified by l60609 for DSDA Phase III, 2013-2-26, Begin */
extern VOS_VOID AT_ReadWasCapabilityNV(VOS_VOID);
/* Modified by l60609 for DSDA Phase III, 2013-2-26, End */

VOS_UINT32 AT_RcvMtaBodySarSetCnf(VOS_VOID *pstMsg);

extern VOS_VOID AT_ReportResetCmd(AT_RESET_REPORT_CAUSE_ENUM_UINT32 enCause);
extern VOS_VOID AT_StopAllTimer(VOS_VOID);
extern VOS_VOID AT_ResetParseCtx(VOS_VOID);
extern VOS_VOID AT_ResetClientTab(VOS_VOID);
extern VOS_VOID AT_ResetOtherCtx(VOS_VOID);
extern VOS_UINT32 AT_RcvCcpuResetStartInd(
    VOS_VOID                           *pstMsg
);
extern VOS_UINT32 AT_RcvCcpuResetEndInd(
    VOS_VOID                           *pstMsg
);
extern VOS_UINT32 AT_RcvHifiResetStartInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvHifiResetEndInd(
    VOS_VOID                           *pstMsg
);

#if (VOS_WIN32 == VOS_OS_VER)
extern VOS_UINT32 At_PidInit(enum VOS_INIT_PHASE_DEFINE enPhase);
#endif

VOS_UINT32 AT_RcvMtaQryCurcCnf(VOS_VOID *pstMsg);
VOS_UINT32 AT_RcvMtaSetUnsolicitedRptCnf(VOS_VOID *pstMsg);
VOS_UINT32 AT_RcvMtaQryUnsolicitedRptCnf(VOS_VOID *pstMsg);
VOS_UINT32 AT_ProcMtaUnsolicitedRptQryCnf(
    VOS_UINT8                               ucIndex,
    VOS_VOID                               *pstMsg
);

VOS_UINT32 AT_RcvMmaCerssiInfoQueryCnf(VOS_VOID *pstMsg);

/*****************************************************************************
 函 数 名  : AT_RcvMtaImeiVerifyQryCnf
 功能描述  : 收到IMEI校验查询的处理
 输入参数  : pstMsg
 输出参数  : 无
 返 回 值  : VOS_UINT32

*****************************************************************************/
VOS_UINT32 AT_RcvMtaImeiVerifyQryCnf(VOS_VOID *pstMsg);
/*****************************************************************************
 函 数 名  : AT_RcvMtaCgsnQryCnf
 功能描述  : 收到UE信息上报的处理
 输入参数  : pstMsg
 输出参数  : 无
 返 回 值  : VOS_UINT32

*****************************************************************************/
VOS_UINT32 AT_RcvMtaCgsnQryCnf(VOS_VOID *pstMsg);


/* Added by f62575 for SS FDN&Call Control, 2013-05-06, begin */
VOS_UINT32 AT_RcvMmaCopnInfoQueryCnf(VOS_VOID *pMsg);
/* Added by f62575 for SS FDN&Call Control, 2013-05-06, end */

VOS_UINT32 AT_RcvMtaSetNCellMonitorCnf(VOS_VOID *pstMsg);
VOS_UINT32 AT_RcvMtaQryNCellMonitorCnf(VOS_VOID *pstMsg);
VOS_UINT32 AT_RcvMtaNCellMonitorInd(VOS_VOID *pstMsg);

VOS_UINT32 AT_RcvMmaSimInsertRsp(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMtaRefclkfreqSetCnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMtaRefclkfreqQryCnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMtaRefclkfreqInd(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMtaRficSsiRdQryCnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMtaHandleDectSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaHandleDectQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaPsTransferInd(VOS_VOID *pMsg);

#if (FEATURE_ON == FEATURE_DSDS)
VOS_UINT32 AT_RcvMtaPsProtectSetCnf(VOS_VOID *pMsg);
#endif
VOS_UINT32 AT_RcvMtaPhyInitCnf(VOS_VOID *pMsg);

VOS_VOID AT_RcvSwitchCmdModeMsg(VOS_UINT8 ucIndex);

VOS_VOID AT_RcvWaterLowMsg(VOS_UINT8 ucIndex);

VOS_UINT32 AT_RcvMtaEcidSetCnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMtaMipiInfoCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaMipiInfoInd(
    VOS_VOID                           *pMsg
);



VOS_UINT32 AT_RcvMmaSysCfgSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaPhoneModeSetCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaDetachCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaAttachCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMmaAttachStatusQryCnf(
    VOS_VOID                           *pstMsg
);

#if (FEATURE_ON == FEATURE_ECALL)
VOS_UINT32 AT_RcvVcMsgSetMsdCnfProc(
    MN_AT_IND_EVT_STRU                 *pstData
);

VOS_UINT32 AT_RcvVcMsgQryMsdCnfProc(
    MN_AT_IND_EVT_STRU                 *pstData
);

VOS_UINT32 AT_RcvVcMsgQryEcallCfgCnfProc(
    MN_AT_IND_EVT_STRU                 *pstData
);

#endif

VOS_UINT32 AT_RcvMmaCdmaCsqSetCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaCdmaCsqQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaCdmaCsqInd(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaCLModInd(
    VOS_VOID                           *pMsg
);


VOS_UINT32 AT_RcvMmaCFreqLockSetCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCFreqLockQueryCnf(
    VOS_VOID                           *pstMsg
);

extern VOS_UINT32 AT_RcvMmaCTimeInd(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMtaXpassInfoInd(
    VOS_VOID                           *pMsg
);

extern VOS_UINT32 AT_RcvMmaCFPlmnSetCnf(
    VOS_VOID                           *pMsg
);

extern VOS_UINT32 AT_RcvMmaCFPlmnQueryCnf(
    VOS_VOID                           *pstMsg
);

VOS_VOID AT_ReportQryPrefPlmnCmdPara(
    TAF_MMA_PREF_PLMN_QUERY_CNF_STRU   *pstCpolQryCnf,
    AT_MODEM_NET_CTX_STRU              *pstNetCtx,
    VOS_UINT16                         *pusLength,
    VOS_UINT32                          ucIndex,
    VOS_UINT32                          ulLoop
);

VOS_VOID AT_ReportQryPrefPlmnCmd(
    TAF_MMA_PREF_PLMN_QUERY_CNF_STRU   *pstCpolQryCnf,
    VOS_UINT32                         *pulValidPlmnNum,
    AT_MODEM_NET_CTX_STRU              *pstNetCtx,
    VOS_UINT16                         *pusLength,
    VOS_UINT32                          ucIndex
);

VOS_UINT32 AT_RcvMmaPrefPlmnSetCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaPrefPlmnQueryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaPrefPlmnTestCnf(
    VOS_VOID                           *pstMsg
);
extern AT_MTA_PERS_CATEGORY_ENUM_UINT8 AT_GetSimlockUnlockCategoryFromClck(VOS_VOID);

#if (FEATURE_ON == FEATURE_IMS)
VOS_UINT32 AT_RcvMmaImsSwitchSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaImsSwitchQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaVoiceDomainSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_VoiceDomainTransToOutputValue(
    TAF_MMA_VOICE_DOMAIN_ENUM_UINT32    enVoiceDoman,
    VOS_UINT32                         *pulValue
);

VOS_UINT32 AT_RcvMmaVoiceDomainQryCnf(
    VOS_VOID                           *pMsg
);
#endif

#if(FEATURE_ON == FEATURE_IMS)
VOS_UINT32 AT_RcvMmaImsDomainCfgSetCnf(
    VOS_VOID                            *pMsg
);

VOS_UINT32 AT_RcvMmaImsDomainCfgQryCnf(
    VOS_VOID                            *pMsg
);
VOS_UINT32 AT_RcvMmaRoamImsSupportSetCnf(
    VOS_VOID * pMsg
);
#endif



VOS_UINT32 AT_RcvMtaSetFemctrlCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaCerssiSetCnf(
    VOS_VOID                           *pstMsg
);

extern VOS_UINT32 AT_RcvMmaPlmnReselAutoSetCnf(
    VOS_VOID                           *pstMsg
);
extern VOS_UINT32 AT_RcvMmaPrefPlmnTypeSetCnf(
    VOS_VOID                           *pstMsg
);
extern VOS_UINT32 AT_RcvMmaPlmnSpecialSelSetCnf(
    VOS_VOID                           *pstMsg
);
extern VOS_UINT32 AT_RcvMmaPowerDownCnf(
    VOS_VOID                           *pstMsg
);
extern VOS_UINT32 AT_RcvMmaPlmnListAbortCnf(
    VOS_VOID                           *pstMsg
);

extern VOS_UINT32 AT_RcvMmaPhoneModeQryCnf(
    VOS_VOID                           *pMsg
);

/* Added by k902809 for Iteration 11, 2015-3-24, begin */
VOS_UINT32 AT_RcvMmaAcInfoChangeInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaEOPlmnSetCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaEOPlmnQryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaNetScanCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaUserSrvStateQryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32  AT_RcvMmaPwrOnAndRegTimeQryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32  AT_RcvMmaSpnQryCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32  AT_RcvMmaMMPlmnInfoQryCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32  AT_RcvMmaPlmnQryCnf(
    VOS_VOID                           *pstMsg
);

/* Added by k902809 for Iteration 11, Iteration 11 2015-3-24, end */
VOS_UINT32 AT_RcvTafMmaQuickStartSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvTafMmaAutoAttachSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvTafMmaSyscfgQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvTafMmaSyscfgTestCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvTafMmaQuickStartQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvTafMmaCsnrQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvTafMmaCsqQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvTafMmaCsqlvlQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaTimeChangeInd(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaModeChangeInd(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaPlmnChangeInd(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvTafMmaCrpnQryCnf(
    VOS_VOID                           *pMsg
);


VOS_UINT32 AT_RcvMmaCbcQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaHsQryCnf(
    VOS_VOID                           *pMsg
);


VOS_UINT32 AT_RcvMmaHdrCsqInd(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaHdrCsqSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaHdrCsqQryCnf(
    VOS_VOID                           *pMsg
);



VOS_UINT32 AT_RcvMmaAccessModeQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaCopsQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaRegStateQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaAutoAttachQryCnf(
    VOS_VOID                           *pMsg
);


VOS_UINT32 AT_RcvMmaSysInfoQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaAnqueryQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMtaCsnrQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMtaCsqlvlQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaEHplmnInfoQryCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMmaApHplmnQryCnf(
    VOS_UINT8                           ucIndex,
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaHplmnQryCnf(
    VOS_UINT8                           ucIndex,
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMmaSrvStatusInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaRssiInfoInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaRegRejInfoInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaRegStatusInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaPlmnSelectInfoInd(
    VOS_VOID                           *pstMsg
);


VOS_UINT32 AT_RcvMmaImsiRefreshInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMma1xChanSetCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMma1xChanQueryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCVerQueryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaStateQueryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCHverQueryCnf(
    VOS_VOID                           *pstMsg
);
#if (FEATURE_ON == FEATURE_LTE)
VOS_UINT32 AT_RcvMtaSetFrCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMtaSib16TimeUpdateInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMtaAccessStratumRelInd(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMtaRsInfoQryCnf(
    VOS_VOID                           *pstMsg
);
#endif


VOS_UINT32 AT_RcvMtaClearHistoryFreqCnf(
    VOS_VOID                           *pMsg
);



#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
VOS_UINT32 AT_RcvMmaCLocInfoQueryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCSidInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaQuitCallBackCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaSetCSidListCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaEmcCallBackNtf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMmaQryEmcCallBackCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaMeidSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaMeidQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaQryCurrSidNidCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaRoamingModeSwitchInd(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMmaCombinedModeSwitchInd(
    VOS_VOID                            *pstMsg
);

VOS_UINT32 AT_RcvMmaRatCombinedModeQryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaIccAppTypeSwitchInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCtRoamInfoCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCtRoamingInfoChgInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCtOosCountCnf(
    VOS_VOID                           *pstMsg
);


#if (FEATURE_ON == FEATURE_CHINA_TELECOM_VOICE_ENCRYPT)
VOS_UINT32 AT_RcvXcallEncryptCallCnf(
    VOS_VOID                                               *pstMsg
);
VOS_UINT32 AT_RcvXcallEncryptCallInd(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallEccRemoteCtrlInd(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallRemoteCtrlAnsCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallEccCapSetCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallEccCapQryCnf(
    VOS_VOID                           *pstMsg
);
#if (FEATURE_ON == FEATURE_CHINA_TELECOM_VOICE_ENCRYPT_TEST_MODE)
VOS_UINT32 AT_RcvXcallSetEccTestModeCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallQryEccTestModeCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallQryEccRandomCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallQryEccKmcCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallSetEccKmcCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvXcallEncryptedVoiceDataInd(
    VOS_VOID                           *pstMsg
);
#endif
#endif
VOS_UINT32 AT_RcvMmaClocinfoInd(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvXcallPrivacyModeSetCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvXcallPrivacyModeQryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvXcallPrivacyModeInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaCtRoamRptCfgSetCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMmaPrlIdQryCnf(
    VOS_VOID                           *pstMsg
);
#endif

VOS_UINT32 AT_RcvMmaDplmnSetCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaDplmnQryCnf(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvMmaBorderInfoSetCnf(
    VOS_VOID                           *pstMsg
);
VOS_UINT32 AT_RcvMmaBorderInfoQryCnf(
    VOS_VOID                           *pstMsg
);

extern VOS_UINT32 AT_RcvMmaSrchedPlmnInfoInd(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMtaTransModeQryCnf(VOS_VOID *pMsg);

VOS_UINT32 AT_RcvMtaUECenterQryCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvMtaUECenterSetCnf(VOS_VOID *pMsg);

extern VOS_UINT32  At_MipiRdCnfProc( HPA_AT_MIPI_RD_CNF_STRU    *pstMsg );
extern VOS_UINT32  At_MipiWrCnfProc( HPA_AT_MIPI_WR_CNF_STRU    *pstMsg );
extern VOS_UINT32  At_SsiWrCnfProc( HPA_AT_SSI_WR_CNF_STRU  *pstMsg );
extern VOS_UINT32  At_SsiRdCnfProc(HPA_AT_SSI_RD_CNF_STRU   *pstMsg );

extern VOS_UINT32  At_PdmCtrlCnfProc( HPA_AT_PDM_CTRL_CNF_STRU  *pstMsg );
VOS_UINT32 AT_RcvMmaInitLocInfoInd(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaPsEflociInfoQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaPsEflociInfoSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaEflociInfoQryCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaEflociInfoSetCnf(
    VOS_VOID                           *pMsg
);

#if (FEATURE_ON == FEATURE_PROBE_FREQLOCK)
VOS_UINT32 AT_RcvMtaSetM2MFreqLockCnf(VOS_VOID *pmsg);
VOS_UINT32 AT_RcvMtaQryM2MFreqLockCnf(VOS_VOID *pmsg);
#endif

VOS_UINT32 AT_RcvMtaSetXCposrCnf(
    VOS_VOID                                *pMsg
);

VOS_UINT32 AT_RcvMtaQryXcposrCnf(
    VOS_VOID                        *pMsg
);

VOS_UINT32 AT_RcvMtaSetXcposrRptCnf(
    VOS_VOID                        *pMsg
);

VOS_UINT32 AT_RcvMtaQryXcposrRptCnf(
    VOS_VOID                        *pMsg
);

VOS_UINT32 AT_RcvMtaSetSensorCnf(
    VOS_VOID                        *pMsg
);

VOS_UINT32 AT_RcvMtaSetScreenCnf(
    VOS_VOID                        *pMsg
);

VOS_UINT32 AT_RcvFratIgnitionQryCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvFratIgnitionSetCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaSetModemTimeCnf(
    VOS_VOID                            *pMsg
);

VOS_UINT32 AT_RcvMtaAfcClkUnlockCauseInd(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaSetBestFreqCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaBestFreqInfoInd(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaQryBestFreqCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaModemCapUpdateCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMmaPacspQryCnf(
    VOS_VOID                           *pMsg
);

#if (FEATURE_ON == FEATURE_LTE)
VOS_UINT32 AT_RcvMtaLteCategoryInfoInd(
    VOS_VOID                           *pMsg
);
#endif

#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of AtCmdMsgProc.h */

