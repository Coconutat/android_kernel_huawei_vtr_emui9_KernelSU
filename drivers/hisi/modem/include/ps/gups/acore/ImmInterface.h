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


#ifndef __IMMINTERFACE_H__
#define __IMMINTERFACE_H__


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "product_config.h"
#include "IMMmem_PS.h"

#if (defined(CONFIG_BALONG_SPE) && (VOS_LINUX == VOS_OS_VER))
#include "mdrv_spe_wport.h"
#endif /* CONFIG_BALONG_SPE */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)


/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define IMM_MAX_ETH_FRAME_LEN           (1536)
#define IMM_MAC_HEADER_RES_LEN          (14)
#define IMM_INVALID_VALUE               (0xFFFFFFFF)


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



extern IMM_ZC_STRU* IMM_ZcStaticAlloc_Debug(unsigned short usFileID, unsigned short usLineNums,
            unsigned int ulLen);

#define    IMM_ZcStaticAlloc(ulLen)\
    IMM_ZcStaticAlloc_Debug(THIS_FILE_ID, __LINE__, (ulLen))




#define IMM_ZcLargeMemAlloc(ulLen)  dev_alloc_skb((ulLen))


extern IMM_ZC_STRU * IMM_ZcDataTransformImmZc_Debug(unsigned short usFileID,
            unsigned short usLineNum, const unsigned char *pucData, unsigned int ulLen, void *pstTtfMem);


#define IMM_DataTransformImmZc(pucData, ulLen, pstTtfMem)\
    IMM_ZcDataTransformImmZc_Debug(THIS_FILE_ID, __LINE__, (pucData), (ulLen), (pstTtfMem))



extern  IMM_ZC_STRU* IMM_ZcStaticCopy_Debug(VOS_UINT16 usFileID, VOS_UINT16 usLineNums, IMM_ZC_STRU* pstImmZc);

#define IMM_ZcStaticCopy(pstImmZc)\
    IMM_ZcStaticCopy_Debug(THIS_FILE_ID, __LINE__, (pstImmZc))


#if (defined(CONFIG_BALONG_SPE) && (VOS_LINUX == VOS_OS_VER))
#define IMM_ZcFree( pstImmZc )              mdrv_spe_wport_recycle((pstImmZc))
#else
#define IMM_ZcFree( pstImmZc )              kfree_skb((pstImmZc))
#endif


#if (defined(CONFIG_BALONG_SPE) && (VOS_LINUX == VOS_OS_VER))
#define IMM_ZcFreeAny( pstImmZc )              mdrv_spe_wport_recycle((pstImmZc))
#else
#define IMM_ZcFreeAny( pstImmZc )              dev_kfree_skb_any((pstImmZc))
#endif


extern void IMM_ZcHeadFree(IMM_ZC_STRU* pstImmZc);


extern VOS_VOID IMM_RbRemoteFreeMem(VOS_VOID *pucAddr);

#define IMM_RemoteFreeTtfMem(pucAddr)     IMM_RbRemoteFreeMem((pucAddr))



extern unsigned int IMM_ZcAddMacHead (IMM_ZC_STRU *pstImmZc, const unsigned char * pucAddData);


extern unsigned int IMM_ZcRemoveMacHead (IMM_ZC_STRU *pstImmZc);


extern IMM_MEM_STRU *IMM_ZcMapToImmMem_Debug(unsigned short usFileID,
            unsigned short usLineNum, IMM_ZC_STRU *pstImmZc);


#define IMM_ZcMapToImmMem(pstImmZc)\
    IMM_ZcMapToImmMem_Debug(THIS_FILE_ID, __LINE__, (pstImmZc))



#define IMM_ZcPush(pstImmZc,ulLen)          skb_push((pstImmZc),(ulLen))



#define IMM_ZcPull(pstImmZc,ulLen)          skb_pull((pstImmZc),(ulLen))



#define IMM_ZcPut(pstImmZc,ulLen)           skb_put((pstImmZc),(ulLen))



#define IMM_ZcReserve(pstImmZc,ulLen)       skb_reserve((pstImmZc),(int)(ulLen))



#define IMM_ZcResetTailPointer(pstImmZc)    skb_reset_tail_pointer((pstImmZc))



#define IMM_ZcHeadRoom(pstImmZc)            skb_headroom((pstImmZc))



#define IMM_ZcTailRoom(pstImmZc)            skb_tailroom((pstImmZc))



#define IMM_ZcGetDataPtr(pstImmZc)          ((pstImmZc)->data)



#define IMM_ZcGetUsedLen(pstImmZc)          ((pstImmZc)->len)



extern unsigned short IMM_ZcGetUserApp(IMM_ZC_STRU *pstImmZc);



extern void IMM_ZcSetUserApp (IMM_ZC_STRU *pstImmZc, unsigned short usApp);



#define IMM_ZcQueueHeadInit(pstList)        skb_queue_head_init((pstList))



#define IMM_ZcQueueHead(pstList,pstNew)     skb_queue_head((pstList),(pstNew))



#define IMM_ZcQueueTail(pstList,pstNew)     skb_queue_tail((pstList),(pstNew))



#define IMM_ZcDequeueHead(pstList)          skb_dequeue((pstList))



#define IMM_ZcDequeueTail(pstList)          skb_dequeue_tail((pstList))



#define IMM_ZcQueueLen(pstList)             skb_queue_len((pstList))



#define IMM_ZcQueuePeek(pstList)            skb_peek((pstList))



#define IMM_ZcQueuePeekTail(pstList)        skb_peek_tail((pstList))



VOS_VOID IMM_MntnAcpuCheckPoolLeak( VOS_VOID );


#pragma pack()




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of ImmInterface.h */

