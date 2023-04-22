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


#ifndef __IMM_MEM_PS_H__
#define __IMM_MEM_PS_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "product_config.h"
#if (VOS_LINUX == VOS_OS_VER)
#include <linux/skbuff.h>
#else
#include "skbuff.h"
#endif


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif




#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define FEATURE_IMM_MEM_DEBUG           (FEATURE_TTF_MEM_DEBUG)


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*****************************************************************************
 枚举名    : IMM_BOOL_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : IMM统一布尔类型枚举定义
*****************************************************************************/
enum IMM_BOOL_ENUM
{
    IMM_FALSE                            = 0,
    IMM_TRUE                             = 1,
    IMM_BOOL_BUTT
};
typedef unsigned char   IMM_BOOL_ENUM_UINT8;

/*****************************************************************************
 枚举名    : IMM_RSLT_CODE_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : IMM 统一返回值枚举定义
*****************************************************************************/
enum IMM_RSLT_CODE_ENUM
{
    IMM_SUCC                             = 0,
    IMM_FAIL                             = 1,
};
typedef unsigned int   IMM_RSLT_CODE_ENUM_UINT32;


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

enum MEM_BLK_STATE_ENUM
{
    MEM_BLK_STATE_FREE,       /*该内存未申请过或已释放*/
    MEM_BLK_STATE_ALLOC,      /*该内存已申请*/
    MEM_BLK_STATE_BUTT
};
typedef unsigned int MEM_BLK_STATE_ENUM_UINT32;



typedef struct
{
    MEM_BLK_STATE_ENUM_UINT32           enMemStateFlag;
    unsigned int                        ulAllocTick;        /* CPU tick when alloc or free */
    unsigned short                      usAllocFileID;      /* File ID when alloc or free */
    unsigned short                      usAllocLineNum;     /* File Line when alloc or free */
    unsigned short                      usTraceFileID;      /* File ID when traced */
    unsigned short                      usTraceLineNum;     /* File Line when traced */
    unsigned int                        ulTraceTick;        /* CPU tick when traced */
} IMM_BLK_MEM_DEBUG_STRU;



enum IMM_MEM_POOL_ID_ENUM
{
    IMM_MEM_POOL_ID_SHARE = 0,              /* A核共享内存池ID */
    IMM_MEM_POOL_ID_EXT,                    /* A核外部内存池ID */

    IMM_MEM_POOL_ID_BUTT
};
typedef unsigned char IMM_MEM_POOL_ID_ENUM_UINT8;



typedef struct
{
    IMM_MEM_POOL_ID_ENUM_UINT8          enPoolId;           /* 本内存块是是属于哪个内存池 */
    unsigned char                       ucClusterId;        /* 本内存是属于哪一个级别 */
    unsigned short                      usMagicNum;         /* 内存控制块魔术字 */
#if(FEATURE_ON == FEATURE_IMM_MEM_DEBUG)
    IMM_BLK_MEM_DEBUG_STRU              stDbgInfo;         /* 内存控制块的DEBUG信息存储块 */
#endif
    unsigned char                      *pstMemBlk;          /* 存放数据的指针，物理上指向结构体的连续内存 */
}IMM_MEM_STRU;


/* 为了屏蔽不同版本的差异, 将IMM_ZC_HEAD_STRU/IMM_ZC_STRU放到头文件中 */
typedef struct sk_buff_head IMM_ZC_HEAD_STRU;

typedef struct sk_buff IMM_ZC_STRU;

typedef void (*IMM_MEM_EVENT_CALLBACK)(unsigned int ulMaxClusterFreeCnt);


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

#pragma pack(0)



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of IMM.h */

