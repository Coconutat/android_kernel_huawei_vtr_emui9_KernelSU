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
#ifndef _RNIC_SND_MSG_H_
#define _RNIC_SND_MSG_H_

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "RnicProcMsg.h"
#include "RnicConfigInterface.h"
#include "RnicCtx.h"

#ifdef CONFIG_VOWIFI_NEW_FRW
#include "NetMgrCtrlInterface.h"
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  1 全局变量定义
*****************************************************************************/

/*****************************************************************************
  2 宏定义
*****************************************************************************/


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
VOS_UINT32 RNIC_SendDialInfoMsg(
    RNIC_MSG_ID_ENUM_UINT32                enMsgId
);

#if (FEATURE_ON == FEATURE_IMS)
VOS_UINT32 RNIC_SendCdsImsDataReq(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
);

VOS_VOID RNIC_TrigImsDataProcEvent(RNIC_RMNET_ID_ENUM_UINT8 enRmNetId);

#endif

VOS_UINT32 RNIC_SndRnicRmnetConfigReq(
    RNIC_RMNET_CONFIG_STRU             *pstConfigInfo
);

VOS_VOID RNIC_MNTN_SndRmnetConfigInfoMsg(
    RNIC_RMNET_CONFIG_STRU             *pstConfigInfo
);
#ifdef CONFIG_VOWIFI_NEW_FRW
VOS_VOID RNIC_FillNetManagerMsgPdnCfgInfo(
    NM_PDN_INFO_CONFIG_STRU            *pDestPdnInfo,
    IMSA_RNIC_PDN_INFO_CONFIG_STRU     *pSrcPdnInfo
);
VOS_VOID RNIC_SndNetManagerPdpActInd(
    MsgBlock                           *pstMsg
);
VOS_VOID RNIC_SndNetManagerPdpDeactInd(
    MsgBlock                           *pstMsg
);
VOS_VOID RNIC_SndNetManagerPdpModifyInd(
    MsgBlock                           *pstMsg
);
VOS_VOID RNIC_SndNetManagerModemResetInd(VOS_VOID);
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

#endif /* end of RnicSndMsg.h */
