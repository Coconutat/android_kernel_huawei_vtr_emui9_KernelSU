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

#ifndef __ATCMDFTMPROC_H__
#define __ATCMDFTMPROC_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "AtCtx.h"
#include "AtParse.h"

#include "AtMtaInterface.h"
#include "TafDrvAgent.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* LOG3.5的端口定义: USB */
#define AT_LOG_PORT_USB                 (0)

/* LOG3.5的端口定义: VCOM */
#define AT_LOG_PORT_VCOM                (1)

#define AT_JAM_DETECT_DEFAULT_METHOD                        (2)
#define AT_JAM_DETECT_DEFAULT_THRESHOLD                     (10)
#define AT_JAM_DETECT_DEFAULT_FREQ_NUM                      (30)

#define AT_NETMON_PLMN_STRING_MAX_LENGTH                    (10)
#define AT_NETMON_GSM_RX_QUALITY_INVALID_VALUE              (99)
#define AT_NETMON_GSM_RSSI_INVALID_VALUE                    (-500)
#define AT_NETMON_UTRAN_FDD_RSCP_INVALID_VALUE              (0)
#define AT_NETMON_UTRAN_FDD_RSSI_INVALID_VALUE              (0)
#define AT_NETMON_UTRAN_FDD_ECN0_INVALID_VALUE              (0)


/*****************************************************************************
  3 枚举定义
*****************************************************************************/


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

VOS_UINT32 At_SetLogPortPara(VOS_UINT8 ucIndex);

VOS_UINT32 At_QryLogPortPara(VOS_UINT8 ucIndex);

VOS_UINT32 At_SetDpdtTestFlagPara(TAF_UINT8 ucIndex);
VOS_UINT32 At_SetDpdtPara(TAF_UINT8 ucIndex);
VOS_UINT32 At_SetQryDpdtPara(TAF_UINT8 ucIndex);
VOS_UINT32 AT_RcvMtaSetDpdtTestFlagCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvMtaSetDpdtValueCnf(VOS_VOID *pMsg);
VOS_UINT32 AT_RcvMtaQryDpdtValueCnf(VOS_VOID *pMsg);



VOS_UINT32 At_SetJamDetectPara(VOS_UINT8 ucIndex);
VOS_UINT32 At_QryJamDetectPara(VOS_UINT8 ucIndex);
VOS_UINT32 AT_RcvMtaSetJamDetectCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMtaJamDetectInd(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_SetRatFreqLock(VOS_UINT8 ucIndex);

VOS_UINT32 AT_QryRatFreqLock(VOS_UINT8 ucIndex);

VOS_UINT32 AT_RcvMtaSetRatFreqLockCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvMtaSetGFreqLockCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_SetGFreqLock(VOS_UINT8 ucIndex);


VOS_UINT32 At_SetNetMonSCellPara(
    VOS_UINT8                           ucIndex
);
VOS_UINT32 At_SetNetMonNCellPara(
    VOS_UINT8                           ucIndex
);
VOS_UINT32 AT_RcvMtaSetNetMonSCellCnf(
    VOS_VOID                           *pMsg
);
VOS_UINT32 AT_RcvMtaSetNetMonNCellCnf(
    VOS_VOID                           *pMsg
);
VOS_VOID AT_NetMonFmtGsmSCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstSCellInfo,
    VOS_UINT16                         *pusLength
);
VOS_VOID AT_NetMonFmtUtranFddSCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstSCellInfo,
    VOS_UINT16                         *pusLength
);
VOS_VOID AT_NetMonFmtGsmNCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstNCellInfo,
    VOS_UINT16                          usInLen,
    VOS_UINT16                         *pusOutLen
);
VOS_VOID AT_NetMonFmtUtranFddNCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstNCellInfo,
    VOS_UINT16                          usInLen,
    VOS_UINT16                         *pusOutLen
);
VOS_VOID AT_NetMonFmtPlmnId(
    VOS_UINT32                          ulMcc,
    VOS_UINT32                          ulMnc,
    VOS_CHAR                           *pstrPlmn,
    VOS_UINT16                         *pusLength
);

#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
VOS_VOID AT_NetMonFmtUtranTddSCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstSCellInfo,
    VOS_UINT16                         *pusLength
);
VOS_VOID AT_NetMonFmtUtranTddNCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstNCellInfo,
    VOS_UINT16                          usInLen,
    VOS_UINT16                         *pusOutLen
);
#endif

#if (FEATURE_ON == FEATURE_LTE)
VOS_VOID AT_NetMonFmtEutranSCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstSCellInfo,
    VOS_UINT16                         *pusLength
);
VOS_VOID AT_NetMonFmtEutranNCellData(
    MTA_AT_NETMON_CELL_INFO_STRU       *pstNCellInfo,
    VOS_UINT16 usInLen,VOS_UINT16      *pusOutLen
);
VOS_VOID AT_FmtTimeStru(
    MTA_AT_TIME_STRU                   *pstTimeStru,
    VOS_UINT16                         *pusOutLen,
    VOS_UINT16                          usInLen
);
VOS_VOID AT_FormatRsrp(
    MTA_AT_RS_INFO_QRY_CNF_STRU        *pstRsInfoQryCnf
);
VOS_VOID AT_FormatRsrq(
    MTA_AT_RS_INFO_QRY_CNF_STRU        *pstRsInfoQryCnf
);
#endif
VOS_VOID AT_FormatGasAtCmdRslt(
    GAS_AT_RSLT_STRU                   *pstAtCmdRslt
);

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

#endif /* end of AtCmdFtmProc.h */
