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
  1 头文件包含
*****************************************************************************/
#include "IMMmemZC.h"
#include "v_id.h"
#include "TTFUtil.h"



/*****************************************************************************
  2 全局变量定义
*****************************************************************************/
/*lint -e767*/
#define    THIS_FILE_ID                 PS_FILE_ID_IMM_ZC_C
/*lint +e767*/

/******************************************************************************
   2 函数实现
******************************************************************************/

IMM_ZC_STRU* IMM_ZcStaticAlloc_Debug(unsigned short usFileID, unsigned short usLineNum, unsigned int ulLen)
{
    IMM_ZC_STRU *pstAlloc = VOS_NULL_PTR;

    /* 智能机形态上, 申请skb系统内存 */
    pstAlloc = (IMM_ZC_STRU *)IMM_ZcLargeMemAlloc(ulLen);

    return pstAlloc;
}


IMM_ZC_STRU* IMM_ZcStaticCopy_Debug(VOS_UINT16 usFileID, VOS_UINT16 usLineNum, IMM_ZC_STRU* pstImmZc)
{
    return NULL;
}


void IMM_ZcHeadFree(IMM_ZC_STRU* pstImmZc)
{
    return;
}


IMM_MEM_STRU *IMM_ZcMapToImmMem_Debug(unsigned short usFileID,
        unsigned short usLineNum, IMM_ZC_STRU *pstImmZc)
{
    return NULL;
} /* IMM_ZcMapToImmMem */



unsigned int IMM_ZcAddMacHead (IMM_ZC_STRU *pstImmZc, const unsigned char* pucAddData)
{
    unsigned char                      *pucDestAddr;

    if ( NULL == pstImmZc )
    {
        vos_printf("IMM_ZcAddMacHead pstImmZc ptr is null! \n");
        return VOS_ERR;
    }

    if ( NULL == pucAddData )
    {
        vos_printf("IMM_ZcAddMacHead pucData ptr is null! \n");
        return VOS_ERR;
    }


    if( IMM_MAC_HEADER_RES_LEN > (pstImmZc->data - pstImmZc->head) )
    {
        vos_printf("IMM_ZcAddMacHead invalid data Len! data = %p, head = %p \n",
                    pstImmZc->data, pstImmZc->head);

        return VOS_ERR;
    }

    pucDestAddr = IMM_ZcPush(pstImmZc, IMM_MAC_HEADER_RES_LEN);
    PSACORE_MEM_CPY(pucDestAddr, IMM_MAC_HEADER_RES_LEN, pucAddData, IMM_MAC_HEADER_RES_LEN);

    return VOS_OK;
}



unsigned int IMM_ZcRemoveMacHead(IMM_ZC_STRU *pstImmZc)
{
    if ( NULL == pstImmZc )
    {
        vos_printf("IMM_ZcRemoveMacHead pstImmZc ptr is null! \n");
        return VOS_ERR;
    }

    if ( IMM_MAC_HEADER_RES_LEN > pstImmZc->len )
    {
        vos_printf("IMM_ZcRemoveMacHead invalid data Len! tail = %p, data = %p, len = %d \n",
                    skb_tail_pointer(pstImmZc), pstImmZc->data, pstImmZc->len);

        return VOS_ERR;
    }

    IMM_ZcPull(pstImmZc, IMM_MAC_HEADER_RES_LEN);

    return VOS_OK;
}



unsigned char* IMM_ZcPush_Debug(unsigned short usFileID, unsigned short usLineNum,
            IMM_ZC_STRU *pstImmZc, unsigned int ulLen)
{
    unsigned char* pucRet = NULL;

    pucRet = skb_push((pstImmZc), (ulLen));

    return pucRet;
}



unsigned char* IMM_ZcPull_Debug(unsigned short usFileID, unsigned short usLineNum,
            IMM_ZC_STRU *pstImmZc, unsigned int ulLen)
{
    unsigned char* pucRet = NULL;

    pucRet = skb_pull(pstImmZc, ulLen);

    return pucRet;
}



unsigned char* IMM_ZcPut_Debug(unsigned short usFileID, unsigned short usLineNum,
            IMM_ZC_STRU *pstImmZc, unsigned int ulLen)
{
    unsigned char* pucRet = NULL;

    pucRet = skb_put(pstImmZc, ulLen);

    return pucRet;
}



void IMM_ZcReserve_Debug(unsigned short usFileID, unsigned short usLineNum,
            IMM_ZC_STRU *pstImmZc, unsigned int ulLen)
{
    skb_reserve(pstImmZc, (int)ulLen);

    return;
}



unsigned short IMM_ZcGetUserApp(IMM_ZC_STRU *pstImmZc)
{
    return IMM_PRIV_CB(pstImmZc)->usApp;
}



void IMM_ZcSetUserApp (IMM_ZC_STRU *pstImmZc, unsigned short usApp)
{
    if ( NULL == pstImmZc )
    {
        return;
    }

    IMM_PRIV_CB(pstImmZc)->usApp = usApp;

    return;
}



IMM_ZC_STRU * IMM_ZcDataTransformImmZc_Debug(unsigned short usFileID,
        unsigned short usLineNum, const unsigned char *pucData, unsigned int ulLen, void *pstTtfMem)
{
    return NULL;
}/* IMM_ZcDataTransformImmZc_Debug */



