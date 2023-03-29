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

#ifndef _ATTAFAGENTINTERFACE_H_
#define _ATTAFAGENTINTERFACE_H_

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include  "vos.h"
#include  "TafAgentInterface.h"

#pragma pack(4)

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/



/*****************************************************************************
  3 函数实现
*****************************************************************************/

/*****************************************************************************
 函 数 名  : TAF_AGENT_FindCidForDial
 功能描述  : 通过同步机制获取用于拨号的CID
 输入参数  : pCid    - CID
 输出参数  : 无
 返 回 值  : VOS_OK  - 成功
             VOS_ERR - 失败
 调用函数  :
 被调函数  :
*****************************************************************************/
VOS_UINT32 TAF_AGENT_FindCidForDial(
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT8                          *pCid
);


/*****************************************************************************
 函 数 名  : TAF_AGENT_SetPdpCidPara
 功能描述  : 设置指定CID的参数
 输入参数  : usClientId    - 客户端ID
             pstPdpPriPara - PDP上下文参数
 输出参数  : 无
 返 回 值  : VOS_OK        - 成功
             VOS_ERR       - 失败
 调用函数  :
 被调函数  :
*****************************************************************************/
VOS_UINT32 TAF_AGENT_SetPdpCidPara(
    MN_CLIENT_ID_T                      usClientId,
    TAF_PDP_PRIM_CONTEXT_EXT_STRU      *pstPdpPrimContextExt
);


/*****************************************************************************
 函 数 名  : TAF_AGENT_GetPdpCidPara
 功能描述  : 查询指定CID的参数
 输入参数  : pstPdpPriPara - PDP上下文参数
             ucCid         - CID
 返 回 值  : VOS_OK        - 成功
             VOS_ERR       - 失败
 调用函数  :
 被调函数  :
*****************************************************************************/
VOS_UINT32 TAF_AGENT_GetPdpCidPara(
    TAF_PDP_PRIM_CONTEXT_STRU          *pstPdpPriPara,
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT8                           ucCid
);


/*****************************************************************************
 函 数 名  : TAF_AGENT_GetCallInfo
 功能描述  : 获取通话信息同步API
 输入参数  : usClientId
             pucNumOfCalls
             pstCallInfos
 输出参数  : 无
 返 回 值  : VOS_OK  - 成功
             VOS_ERR - 失败
 调用函数  :
 被调函数  :
*****************************************************************************/
VOS_UINT32 TAF_AGENT_GetCallInfoReq(
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT8                          *pucCallNum,
    TAFAGERNT_MN_CALL_INFO_STRU        *pstCallInfos
);


VOS_UINT32 TAF_AGENT_GetSysMode(
    VOS_UINT16                                 usClientId,
    TAF_AGENT_SYS_MODE_STRU                   *pstSysMode
);

extern VOS_VOID TAF_AGENT_ClearAllSem(VOS_VOID);

VOS_UINT32 TAF_AGENT_SetPdpCidQosPara(
    MN_CLIENT_ID_T                      usClientId,
    TAF_PS_PDP_QOS_SET_PARA_STRU       *pstQosPara
);

VOS_UINT32 TAF_AGENT_GetPdpCidQosPara(
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT8                           ucCid,
    TAF_PS_PDP_QOS_QUERY_PARA_STRU     *pstQosPara
);

VOS_UINT32 TAF_AGENT_GetAntState(
    VOS_UINT16                                 usClientId,
    VOS_UINT16                                *pusAntState
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

