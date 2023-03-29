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

#ifndef __ATCMDCAGPSPROC_H__
#define __ATCMDCAGPSPROC_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "AtCtx.h"
#include "AtParse.h"
#include "ATCmdProc.h"
#include "AtXpdsInterface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define AT_XPDS_AGPS_DATAUPLEN_MAX      (240)

#define AT_AGPS_EPH_INFO_MAX_SEG_NUM                        ( 5 )
#define AT_AGPS_EPH_INFO_FIRST_SEG_STR_LEN                  ( 8 )
#define AT_AGPS_EPH_INFO_NOT_FIRST_SEG_STR_LEN              ( 960 )

#define AT_AGPS_ALM_INFO_MAX_SEG_NUM                        ( 3 )
#define AT_AGPS_ALM_INFO_FIRST_SEG_STR_LEN                  ( 8 )
#define AT_AGPS_ALM_INFO_NOT_FIRST_SEG_STR_LEN              ( 896 )


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


typedef struct
{
    AT_XPDS_MSG_TYPE_ENUM_UINT32        enMsgType;
    VOS_UINT32                          ulReserved;
    VOS_CHAR                           *pcATCmd;
}AT_CAGPS_CMD_NAME_TLB_STRU;


typedef struct
{
    AT_XPDS_MSG_TYPE_ENUM_UINT32        enMsgType;
    AT_CMD_CURRENT_OPT_ENUM             enCmdOpt;
}AT_CAGPS_CMD_OPT_TLB_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

extern VOS_UINT32 At_SetAgpsDataCallStatus(
    VOS_UINT8                               ucIndex
);

extern VOS_UINT32 At_SetAgpsUpBindStatus(
    VOS_UINT8                               ucIndex
);

extern VOS_UINT32 At_SetAgpsForwardData(
    VOS_UINT8                               ucIndex
);

extern VOS_UINT32 AT_RcvXpdsAgpsDataCallReq(
    VOS_VOID                            *pstMsg
);

extern VOS_UINT32 AT_RcvXpdsAgpsServerBindReq(
    VOS_VOID                            *pstMsg
);

extern VOS_UINT32 AT_RcvXpdsAgpsReverseDataInd(
    VOS_VOID                            *pstMsg
);

extern VOS_UINT32 AT_CagpsSndXpdsReq(
    VOS_UINT8                           ucIndex,
    AT_XPDS_MSG_TYPE_ENUM_UINT32        enMsgType,
    VOS_UINT32                          ulMsgStructSize
);

extern VOS_UINT32 AT_SetCagpsCfgPosMode(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsStart(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsStop(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsCfgMpcAddr(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsCfgPdeAddr(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsQryRefloc(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsQryTime(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsPrmInfo(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 AT_SetCagpsReplyNiReq(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 At_SetCagpsPosInfo(
    VOS_UINT8                           ucIndex
);

extern VOS_CHAR* AT_SearchCagpsATCmd(
    AT_XPDS_MSG_TYPE_ENUM_UINT32        enMsgType
);

extern VOS_UINT32 AT_RcvXpdsCagpsCnf(
    VOS_VOID                           *pMsg
);

AT_CMD_CURRENT_OPT_ENUM AT_SearchCagpsATCmdOpt(
    AT_XPDS_MSG_TYPE_ENUM_UINT32        enMsgType
);

extern VOS_UINT32 AT_RcvXpdsCagpsRlstCnf(
    VOS_VOID                           *pMsg
);

VOS_UINT32 AT_RcvXpdsEphInfoInd(
    VOS_VOID                           *pstMsg
);

VOS_UINT32 AT_RcvXpdsAlmInfoInd(
    VOS_VOID                           *pstMsg
);

extern VOS_UINT32 At_SetCgpsControlStart(
    VOS_UINT8                           ucIndex
);

extern VOS_UINT32 At_SetCgpsControlStop(
    VOS_UINT8                           ucIndex
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

#endif
