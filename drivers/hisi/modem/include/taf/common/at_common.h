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

#ifndef __AT_COMMON_H__
#define __AT_COMMON_H__

#include "vos.h"


#ifdef __cplusplus
extern "C"
{
#endif

extern VOS_VOID* At_HeapAllocD(VOS_UINT32 ulSize);
extern VOS_VOID At_HeapFreeD(VOS_VOID *pAddr);

#define AT_MALLOC(ulSize)          At_HeapAllocD(ulSize)
#define AT_FREE(pAddr)             At_HeapFreeD(pAddr)


#define AT_DISABLE_CSIM                       (0)

typedef struct
{
    VOS_UINT32 ulEnableCSIM;
}NVIM_ENALBE_CSIM_STRU;


typedef struct
{
    VOS_UINT16      ucIndex;        /* AT通道ID */
    VOS_UINT16      aucRsv;
    VOS_UINT32      ulMsgId;        /* 原语ID */
    VOS_UINT16      aucData[4];     /* 原语内容 */
}TDS_AT_PS_MSG_INFO_STRU;


/*****************************************************************************
 Prototype       : TDS_PsAppSendMsg
 Description     : PS调用APP的接口，用于从DSP接收TDS维修原语(AT实现，PS调用)
 Input           : VOS_UINT32 ulLen 消息长度(数据长度不大于48字节)
                   TDS_AT_PS_MSG_INFO_STRU * pstTdsAppPsMsg 消息内容
 Output          : None.
 Return Value    : 成功:0,失败:其他值
 History         : ---
 *****************************************************************************/
extern VOS_UINT32 TDS_PsAtSendMsg(VOS_UINT32 ulLen, TDS_AT_PS_MSG_INFO_STRU * pstTdsAppPsMsg);
#ifdef __cplusplus
}
#endif


#endif /*__MSP_AT_H__*/

