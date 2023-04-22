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



#ifndef __OMBUFMNGR_H__
#define __OMBUFMNGR_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"


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
#define OM_RECORD_MAX_NUM  (100)


#define OM_ALLOC_RECORD(pData, lLen) \
    { \
        g_stOmBufRecord.astAllocItem[g_stOmBufRecord.ulAllocNum].pucData    = pData; \
        g_stOmBufRecord.astAllocItem[g_stOmBufRecord.ulAllocNum].lLen       = lLen; \
        g_stOmBufRecord.astAllocItem[g_stOmBufRecord.ulAllocNum].ulSlice    = VOS_GetSlice(); \
        \
        g_stOmBufRecord.ulAllocNum++; \
        /* 达到末尾需要翻转*/ \
        if (OM_RECORD_MAX_NUM == g_stOmBufRecord.ulAllocNum) \
        { \
            g_stOmBufRecord.ulAllocNum = 0; \
        } \
    } \

#define OM_RLS_RECORD(pData, lLen) \
    { \
        g_stOmBufRecord.astRlsItem[g_stOmBufRecord.ulRlsNum].pucData    = pData; \
        g_stOmBufRecord.astRlsItem[g_stOmBufRecord.ulRlsNum].lLen       = lLen; \
        g_stOmBufRecord.astRlsItem[g_stOmBufRecord.ulRlsNum].ulSlice    = VOS_GetSlice(); \
        \
        g_stOmBufRecord.ulRlsNum++; \
        /* 达到末尾需要翻转*/ \
        if (OM_RECORD_MAX_NUM == g_stOmBufRecord.ulRlsNum) \
        { \
            g_stOmBufRecord.ulRlsNum = 0; \
        } \
    } \

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
 结构名    : OM_BUF_CTRL_STRU
 结构说明  : 包含OM缓冲区管理控制信息
*****************************************************************************/
typedef struct
{
    VOS_INT32                           lAlloc;         /* 指向分配内存的偏移地址 */
    VOS_INT32                           lRelease;       /* 指向释放内存的偏移地址 */
    VOS_INT32                           lPadding;       /* 保存填充内存的大小 */
    VOS_INT32                           lBufSize;       /* 保存缓冲区总大小 */
    VOS_UINT8                          *pucBuf;         /* 指向缓冲区虚拟首地址 */
    VOS_UINT8                          *pucRealBuf;     /* 指向缓冲区实首地址 */
}OM_BUF_CTRL_STRU;


typedef struct
{
    VOS_UINT8                          *pucData;
    VOS_INT32                           lLen;
    VOS_UINT32                          ulSlice;
}OM_DATA_ITEM_STRU;

typedef struct
{
    VOS_UINT32                          ulSocpBug;
    VOS_UINT32                          ulAllocNum;
    VOS_UINT32                          ulRlsNum;
    OM_DATA_ITEM_STRU                   astAllocItem[OM_RECORD_MAX_NUM];
    OM_DATA_ITEM_STRU                   astRlsItem[OM_RECORD_MAX_NUM];
}OM_BUF_RECORD_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/
/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
extern OM_BUF_RECORD_STRU                      g_stOmBufRecord;

/*****************************************************************************
  10 函数声明
*****************************************************************************/

extern VOS_UINT32 OM_GetSlice(VOS_VOID);

/*****************************************************************************
 函 数 名  : OM_CreateTraceBuffer
 功能描述  : 创建OM缓冲区
 输入参数  : pstBufCtrl: 指向缓冲区控制块指针
             lBufSize:   分配缓冲区大小

 输出参数  : 无

 返 回 值  : VOS_OK/VOS_ERR
*****************************************************************************/
VOS_UINT32 OM_CreateTraceBuffer(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_INT32 lBufSize);

/*****************************************************************************
 函 数 名  : OM_AllocTraceMem
 功能描述  : 从缓冲区中分配连续的物理内存.
 输入参数  : pstBufCtrl: 指向缓冲区控制块指针
             lLen:       申请内存大小

 输出参数  : 无

 返 回 值  : VOS_NULL_PTR 分配失败
             VOS_VOID*    分配内存的指针
*****************************************************************************/
VOS_VOID* OM_AllocTraceMem(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_INT32 lLen);

/*****************************************************************************
 函 数 名  : OM_ReleaseTraceMem
 功能描述  : 释放从缓冲区中分配连续的物理内存.
 输入参数  : pstBufCtrl: 指向缓冲区控制块指针
             pucAddr:    指向释放内存的指针
             lLen:      释放缓冲区大小

 输出参数  : 无

 返 回 值  : VOS_OK/VOS_ERR
*****************************************************************************/
VOS_UINT32 OM_ReleaseTraceMem(OM_BUF_CTRL_STRU *pstBufCtrl,
                                        VOS_VOID *pAddr, VOS_INT32 lLen);

/*****************************************************************************
 函 数 名  : OM_TraceMemFreeBytes
 功能描述  : 获得缓冲区中使用内存大小.
 输入参数  : pstBufCtrl: 指向缓冲区控制块指针

 输出参数  : 无

 返 回 值  : VOS_INT32 使用内存大小
*****************************************************************************/
VOS_INT32 OM_TraceMemNBytes(OM_BUF_CTRL_STRU *pstBufCtrl);

/*****************************************************************************
 函 数 名  : OM_AddrVirtToReal
 功能描述  : 将虚地址转换为实地址.
 输入参数  : pstBufCtrl: 指向缓冲区控制块指针
             pucVirtAddr:待转换虚地址

 输出参数  : 无

 返 回 值  : VOS_NULL_PTR 转换失败
             VOS_VOID*    指向转换后实地址
*****************************************************************************/
VOS_VOID* OM_AddrVirtToReal(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_UINT8 *pucVirtAddr);

/*****************************************************************************
 函 数 名  : OM_AddrRealToVirt
 功能描述  : 将实地址转换为虚地址.
 输入参数  : pstBufCtrl: 指向缓冲区控制块指针
             pucRealAddr:待转换实地址

 输出参数  : 无

 返 回 值  : VOS_NULL_PTR 转换失败
             VOS_VOID*    指向转换后虚地址
*****************************************************************************/
VOS_VOID* OM_AddrRealToVirt(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_UINT8 *pucRealAddr);

#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
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
