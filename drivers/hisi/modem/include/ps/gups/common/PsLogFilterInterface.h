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

#ifndef __PSLOGFILTERINTERFACE_H__
#define __PSLOGFILTERINTERFACE_H__


/******************************************************************************
  1 其他头文件包含
******************************************************************************/
#include "vos.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)


/******************************************************************************
  2 宏定义
******************************************************************************/

/******************************************************************************
  3 枚举定义
******************************************************************************/


/******************************************************************************
  4 全局变量声明
******************************************************************************/


/******************************************************************************
  5 消息头定义
******************************************************************************/


/******************************************************************************
  6 消息定义
******************************************************************************/


/******************************************************************************
  7 STRUCT定义
******************************************************************************/


/*****************************************************************************
 结构名    : PS_OM_LAYER_MSG_MATCH_PFUNC
 协议表格  :
 ASN.1描述 :
 结构说明  : MsgMatch回调函数指针
             此回调函数规则:
             1.如果注册回调内部没有对消息进行处理，则需要将入参指针返回，否则
               本模块不知道是否需要将此消息传递给下一个注册回调进行处理
             2.如果注册回调内部对消息进行了处理，则返回值能够实现两个功能:
               ①返回VOS_NULL，则将此消息进行完全过滤，不会再勾取出来
               ②返回与入参指针不同的另一个指针，则勾取的消息将会使用返回的指
                 针内容替换原消息的内容。另本模块不负责对替换的内存进行释放，
                 替换原消息使用的内存请各模块自行管理。
*****************************************************************************/
typedef VOS_VOID * (*PS_OM_LAYER_MSG_MATCH_PFUNC)(MsgBlock *pMsg);

/*****************************************************************************
 结构名    : PS_OM_CCPU_LAYER_MSG_FILTER_PFUNC
 协议表格  :
 ASN.1描述 :
 结构说明  : MsgFilter回调函数指针
 此回调函数规则:
             1.如果注册回调内部没有对消息进行处理，返回VOS_FALSE，否则
               本模块不知道是否需要将此消息传递给下一个注册回调进行处理
             2.如果注册回调内部对消息进行了处理，返回VOS_TRUE表示该消息
               需要进行过滤。
*****************************************************************************/
typedef VOS_UINT32 (*PS_OM_LAYER_MSG_FILTER_PFUNC)(const VOS_VOID *pMsg);


/******************************************************************************
  8 UNION定义
******************************************************************************/


/******************************************************************************
  9 OTHERS定义
******************************************************************************/


VOS_VOID PS_OM_LayerMsgMatchInit(VOS_VOID);


VOS_UINT32 PS_OM_LayerMsgMatchFuncReg
(
    PS_OM_LAYER_MSG_MATCH_PFUNC         pFunc
);


VOS_VOID PS_OM_LayerMsgFilterInit(VOS_VOID);


VOS_UINT32 PS_OM_LayerMsgFilterFuncReg
(
    PS_OM_LAYER_MSG_FILTER_PFUNC         pFunc
);



#pragma pack()


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* PsLogFilterInterface.h */


