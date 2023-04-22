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



/*****************************************************************************
  1 头文件
*****************************************************************************/
#include "ombufmngr.h"
#include "omprivate.h"



#define    THIS_FILE_ID        PS_FILE_ID_OM_BUF_MMNGR_C

/*****************************************************************************
  2 全局变量声明
*****************************************************************************/
OM_BUF_RECORD_STRU                      g_stOmBufRecord;

/*****************************************************************************
  3 函数申明
*****************************************************************************/


VOS_UINT32 OM_CreateTraceBuffer(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_INT32 lBufSize)
{
    VOS_UINT_PTR ulRealAddr;

    /* 参数检测 */
    if ((VOS_NULL_PTR == pstBufCtrl) || (0 == lBufSize))
    {
        return VOS_ERR;
    }

    pstBufCtrl->lAlloc   = 0;
    pstBufCtrl->lRelease = 0;
    pstBufCtrl->lPadding = 0;
    pstBufCtrl->lBufSize = lBufSize + 1;

    /*申请uncache的动态内存区*/
    pstBufCtrl->pucBuf = (VOS_UINT8*)VOS_UnCacheMemAlloc((VOS_UINT32)pstBufCtrl->lBufSize, &ulRealAddr);

    /* 分配内存失败 */
    if (VOS_NULL_PTR == pstBufCtrl->pucBuf)
    {
        return VOS_ERR;
    }

    /* 保存buf实地址 */
    pstBufCtrl->pucRealBuf = (VOS_UINT8 *)ulRealAddr;

    PAM_MEM_SET_S(&g_stOmBufRecord, sizeof(g_stOmBufRecord), 0, sizeof(g_stOmBufRecord));

    return VOS_OK;
}


VOS_VOID* OM_AllocTraceMem(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_INT32 lLen)
{
    VOS_INT32                           lFreeSize;
    VOS_UINT8                          *pucAddr;
    VOS_INT32                           lTmpAlloc;
    VOS_INT32                           lTmpPadding;

    /* 输入参数检测 */
    if ((VOS_NULL_PTR == pstBufCtrl)
        || (lLen >= pstBufCtrl->lBufSize)
        || (0 == lLen))
    {
        OM_ALLOC_RECORD(VOS_NULL_PTR, lLen);

        return VOS_NULL_PTR;
    }

    lTmpAlloc       = pstBufCtrl->lAlloc;
    lTmpPadding     = pstBufCtrl->lPadding;

    /* 没有翻转 */
    if (pstBufCtrl->lAlloc >= pstBufCtrl->lRelease)
    {
        /* 获取剩余内存大小 */
        if (0 == pstBufCtrl->lRelease)
        {
            lFreeSize = (pstBufCtrl->lBufSize - pstBufCtrl->lAlloc) - 1;
        }
        else
        {
            lFreeSize = pstBufCtrl->lBufSize - pstBufCtrl->lAlloc;
        }

        /* 满足用户申请内存大小则返回 */
        if (lFreeSize >= lLen)
        {
            pucAddr = pstBufCtrl->pucBuf + pstBufCtrl->lAlloc;
            pstBufCtrl->lAlloc += lLen;

            OM_ALLOC_RECORD(pucAddr, lLen);

            return pucAddr;
        }

        lTmpPadding  = lFreeSize;
        lTmpAlloc    = 0;
    }

    /* 翻转，获取剩余内存大小 */
    lFreeSize = (pstBufCtrl->lRelease - lTmpAlloc) - 1;

    if (lFreeSize >= lLen)
    {
        pucAddr = pstBufCtrl->pucBuf + lTmpAlloc;

        pstBufCtrl->lAlloc    = lTmpAlloc + lLen;
        pstBufCtrl->lPadding  = lTmpPadding;

        OM_ALLOC_RECORD(pucAddr, lLen);

        return pucAddr;
    }

    OM_ALLOC_RECORD(VOS_NULL_PTR, lLen);

    /* 否则分配失败，返回空指针 */
    return VOS_NULL_PTR;
}


VOS_UINT32 OM_ReleaseTraceMem(OM_BUF_CTRL_STRU *pstBufCtrl,
                                        VOS_VOID *pAddr, VOS_INT32 lLen)
{
    VOS_INT32                           lUsedSize;
    VOS_INT32                           lTmpRls;

    OM_RLS_RECORD(pAddr, lLen);

    /* 输入参数检测 */
    if ((VOS_NULL_PTR == pstBufCtrl) || (lLen >= pstBufCtrl->lBufSize))
    {
        return VOS_ERR;
    }

    /* 释放内存地址进行检测 */
    if ((pAddr != (pstBufCtrl->pucBuf + pstBufCtrl->lRelease))
        && (pAddr != pstBufCtrl->pucBuf))
    {
        g_stOmBufRecord.ulSocpBug++;
        /*return VOS_ERR;*/
    }

    /* 未翻转 */
    if (pstBufCtrl->lAlloc >= pstBufCtrl->lRelease)
    {
        lUsedSize = pstBufCtrl->lAlloc - pstBufCtrl->lRelease;

        /* 长度错误 */
        if (lUsedSize < lLen)
        {
            return VOS_ERR;
        }

        pstBufCtrl->lRelease = (VOS_UINT8 *)pAddr + lLen - pstBufCtrl->pucBuf;

        return VOS_OK;
    }

    if (((VOS_UINT8 *)pAddr - pstBufCtrl->pucBuf + lLen) > pstBufCtrl->lBufSize)
    {
        lTmpRls = (((VOS_UINT8 *)pAddr - pstBufCtrl->pucBuf + lLen) + pstBufCtrl->lPadding) % pstBufCtrl->lBufSize;
    }
    else
    {
        lTmpRls = ((VOS_UINT8 *)pAddr - pstBufCtrl->pucBuf + lLen) % pstBufCtrl->lBufSize;
    }

    /* 输入ulLen不正确 */
    if ((lTmpRls > pstBufCtrl->lAlloc) && (lTmpRls < pstBufCtrl->lRelease))
    {
        return VOS_ERR;
    }

    /* 如果发生跳转则将Padding值归0 */
    if (lTmpRls <= pstBufCtrl->lAlloc)
    {
        pstBufCtrl->lPadding = 0;
    }

    pstBufCtrl->lRelease = lTmpRls;

    return VOS_OK;
}


VOS_INT32 OM_TraceMemNBytes(OM_BUF_CTRL_STRU *pstBufCtrl)
{
    VOS_INT32 lUsedBytes;

    /* 输入参数检测 */
    if (VOS_NULL_PTR == pstBufCtrl)
    {
        return 0;
    }

    lUsedBytes = pstBufCtrl->lAlloc - pstBufCtrl->lRelease;

    /* 指针有翻转 */
    if (lUsedBytes < 0)
    {
        lUsedBytes += pstBufCtrl->lBufSize;
    }

    return lUsedBytes;
}


VOS_VOID* OM_AddrVirtToReal(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_UINT8 *pucVirtAddr)
{
    return (VOS_VOID *)VOS_UncacheMemVirtToPhy(pucVirtAddr, pstBufCtrl->pucRealBuf, pstBufCtrl->pucBuf, (VOS_UINT32)pstBufCtrl->lBufSize);
}


VOS_VOID* OM_AddrRealToVirt(OM_BUF_CTRL_STRU *pstBufCtrl, VOS_UINT8 *pucRealAddr)
{
    return (VOS_VOID*)VOS_UncacheMemPhyToVirt(pucRealAddr, pstBufCtrl->pucRealBuf, pstBufCtrl->pucBuf, (VOS_UINT32)pstBufCtrl->lBufSize);
}


VOS_VOID OM_BufShow(VOS_VOID)
{
    VOS_UINT32                          ulIndex;

    (VOS_VOID)vos_printf("\r\n OM ALLOC DATA SOCP Bug: %d.\r\n",g_stOmBufRecord.ulSocpBug);

    (VOS_VOID)vos_printf("\r\n OM ALLOC DATA LIST: \r\n");

    for (ulIndex = g_stOmBufRecord.ulAllocNum; ulIndex < OM_RECORD_MAX_NUM; ulIndex++)
    {
        (VOS_VOID)vos_printf("Slice: 0x%x, Addr: 0x%p, Len: 0x%x.\r\n ",
                    g_stOmBufRecord.astAllocItem[ulIndex].ulSlice,
                    g_stOmBufRecord.astAllocItem[ulIndex].pucData,
                    g_stOmBufRecord.astAllocItem[ulIndex].lLen);
    }

    for (ulIndex = 0; ulIndex < g_stOmBufRecord.ulAllocNum; ulIndex++)
    {
        (VOS_VOID)vos_printf("Slice: 0x%x, Addr: 0x%p, Len: 0x%x.\r\n ",
                    g_stOmBufRecord.astAllocItem[ulIndex].ulSlice,
                    g_stOmBufRecord.astAllocItem[ulIndex].pucData,
                    g_stOmBufRecord.astAllocItem[ulIndex].lLen);
    }

    (VOS_VOID)vos_printf("\r\n OM RELEASE DATA LIST: \r\n");

    for (ulIndex = g_stOmBufRecord.ulRlsNum; ulIndex < OM_RECORD_MAX_NUM; ulIndex++)
    {
        (VOS_VOID)vos_printf("Slice: 0x%x, Addr: 0x%p, Len: 0x%x.\r\n ",
                    g_stOmBufRecord.astRlsItem[ulIndex].ulSlice,
                    g_stOmBufRecord.astRlsItem[ulIndex].pucData,
                    g_stOmBufRecord.astRlsItem[ulIndex].lLen);
    }

    for (ulIndex = 0; ulIndex < g_stOmBufRecord.ulRlsNum; ulIndex++)
    {
        (VOS_VOID)vos_printf("Slice: 0x%x, Addr: 0x%p, Len: 0x%x.\r\n ",
                    g_stOmBufRecord.astRlsItem[ulIndex].ulSlice,
                    g_stOmBufRecord.astRlsItem[ulIndex].pucData,
                    g_stOmBufRecord.astRlsItem[ulIndex].lLen);
    }

    return;
}


