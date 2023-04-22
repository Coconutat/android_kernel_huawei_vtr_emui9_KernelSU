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

#ifndef __ATCMDSIMROC_H__
#define __ATCMDSIMPROC_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "AtCtx.h"
#include "AtParse.h"
#include "ATCmdProc.h"
#include "TafNvInterface.h"
#include "siapppih.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

enum AT_CARDAPP_ENUM
{
    AT_PREFER_APP_CDMA          = 0x00000000,
    AT_PREFER_APP_GUTL          = 0x00000001,
    AT_PREFER_APP_BUTT          = 0x00000002,
};
typedef TAF_UINT32 AT_CARDAPP_ENUM_UINT32;

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
extern VOS_UINT32 At_Base16Decode(VOS_CHAR *pcData, VOS_UINT32 ulDataLen, VOS_UINT8* pucDst);


extern VOS_UINT32 At_SetHvsstPara(
    VOS_UINT8                           ucIndex
);
extern VOS_UINT32 At_QryHvsstPara(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT16 At_HvsstQueryCnf(
    VOS_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent);

extern VOS_UINT32 At_SetSciChgPara(
    VOS_UINT8                           ucIndex
);
extern VOS_UINT32 At_QrySciChgPara(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT16 At_SciCfgQueryCnf(
    VOS_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent);

extern VOS_UINT16 AT_UiccAuthCnf(TAF_UINT8 ucIndex,SI_PIH_EVENT_INFO_STRU *pstEvent);
extern VOS_UINT16 AT_UiccAccessFileCnf(TAF_UINT8 ucIndex,SI_PIH_EVENT_INFO_STRU *pstEvent);

extern VOS_UINT32 At_SetCrlaPara(VOS_UINT8 ucIndex);
extern VOS_UINT32 At_QryCardSession(VOS_UINT8 ucIndex);
extern TAF_UINT32 At_QryCardVoltagePara(TAF_UINT8 ucIndex);
extern TAF_UINT32 At_SetPrivateCglaPara(TAF_UINT8 ucIndex);
extern TAF_UINT16 At_PrintPrivateCglaResult(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent);
extern TAF_UINT32 At_SetPrfAppPara(TAF_UINT8 ucIndex);
extern TAF_UINT32 At_QryPrfAppPara(TAF_UINT8 ucIndex);
extern TAF_UINT32 At_TestPrfAppPara(TAF_UINT8 ucIndex);

extern TAF_UINT32 At_SetUiccPrfAppPara(TAF_UINT8 ucIndex);
extern TAF_UINT32 At_QryUiccPrfAppPara(TAF_UINT8 ucIndex);
extern TAF_UINT32 At_TestUiccPrfAppPara(TAF_UINT8 ucIndex);

extern TAF_UINT32 At_SetCCimiPara(TAF_UINT8 ucIndex);

extern TAF_UINT16 At_CardErrorInfoInd(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent);

extern TAF_UINT16 At_CardIccidInfoInd(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent
);
#if ((FEATURE_ON == FEATURE_VSIM)&&(FEATURE_ON == FEATURE_VSIM_ICC_SEC_CHANNEL))
extern VOS_UINT16 At_Hex2Base16(VOS_UINT32 MaxLength,VOS_CHAR *headaddr,VOS_CHAR *pucDst,VOS_UINT8 *pucSrc,VOS_UINT16 usSrcLen);

extern VOS_UINT32 At_QryHvCheckCardPara(
    VOS_UINT8                           ucIndex
);

VOS_UINT32 At_QryIccVsimVer(
    VOS_UINT8                           ucIndex
);

#endif

TAF_UINT16 At_SimHotPlugStatusInd(
    TAF_UINT8                           ucIndex,
    SI_PIH_EVENT_INFO_STRU             *pstEvent
);

TAF_UINT16 At_SWCheckStatusInd(
    SI_PIH_EVENT_INFO_STRU             *pstEvent
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

#endif /* end of AtCmdSimProc.h */
