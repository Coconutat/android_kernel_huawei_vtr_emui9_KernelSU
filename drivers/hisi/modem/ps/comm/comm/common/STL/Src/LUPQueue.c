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
  1 Include HeadFile
*****************************************************************************/
#include "LUPQueue.h"
#include "TTFComm.h"

#define    THIS_FILE_ID        PS_FILE_ID_L_TTFQUEUE_C

/*****************************************************************************
  2 Declare the Global Variable
*****************************************************************************/


/*****************************************************************************
  3 Function
*****************************************************************************/

VOS_UINT32 LUP_IsQueEmpty(const LUP_QUEUE_STRU *pstQue)
{
    OSA_ASSERT_RTN (VOS_NULL_PTR != pstQue, PS_TRUE);

    if (pstQue->ulHead == pstQue->ulTail)
    {
        return PS_TRUE;
    }

    return PS_FALSE;
}



VOS_UINT32 LUP_IsQueFull(const LUP_QUEUE_STRU *pstQue)
{
    OSA_ASSERT_RTN (VOS_NULL_PTR != pstQue, PS_TRUE);

    if (pstQue->ulHead == TTF_MOD_ADD(pstQue->ulTail, 1, pstQue->ulMaxNum))
    {
        return PS_TRUE;
    }

    return PS_FALSE;
}


VOS_UINT32 LUP_QueCnt(const LUP_QUEUE_STRU *pstQue)
{
    OSA_ASSERT_RTN (VOS_NULL_PTR != pstQue, 0);

    return TTF_MOD_SUB(pstQue->ulTail, pstQue->ulHead, pstQue->ulMaxNum);
}


VOS_UINT32 LUP_PeekQueHead(const LUP_QUEUE_STRU *pstQue, VOS_VOID **ppNode)
{
    VOS_UINT32 ulHead = 0;

    OSA_ASSERT_RTN (VOS_NULL_PTR != pstQue, PS_PTR_NULL);
    OSA_ASSERT_RTN (VOS_NULL_PTR != ppNode, PS_PARA_ERR);

    if (PS_TRUE == LUP_IsQueEmpty(pstQue))
    {
        return  PS_QUE_EMPTY;
    }

    ulHead  = TTF_MOD_ADD(pstQue->ulHead, 1, pstQue->ulMaxNum);
    *ppNode = pstQue->pBuff[ulHead];

    return PS_SUCC;
}


VOS_UINT32 LUP_EnQue(LUP_QUEUE_STRU *pstQue, VOS_VOID *pNode)
{
    VOS_UINT32          ulTail;

    OSA_ASSERT_RTN (VOS_NULL_PTR != pstQue, PS_PTR_NULL);
    OSA_ASSERT_RTN (VOS_NULL_PTR != pNode, PS_PTR_NULL);

    ulTail = TTF_MOD_ADD(pstQue->ulTail, 1, pstQue->ulMaxNum);
    if (pstQue->ulHead == ulTail)
    {
        return PS_QUE_FULL;
    }

    pstQue->pBuff[ulTail] = pNode;
    pstQue->ulTail = ulTail;

    return PS_SUCC;
}


VOS_UINT32 LUP_DeQue(LUP_QUEUE_STRU *pstQue, VOS_VOID **ppNode)
{
    OSA_ASSERT_RTN (VOS_NULL_PTR != pstQue, PS_PTR_NULL);
    OSA_ASSERT_RTN(VOS_NULL_PTR != ppNode, PS_PTR_NULL);

    if (pstQue->ulHead == pstQue->ulTail)
    {
        return  PS_QUE_EMPTY;
    }

    pstQue->ulHead  = TTF_MOD_ADD(pstQue->ulHead, 1, pstQue->ulMaxNum);
    *ppNode         = pstQue->pBuff[pstQue->ulHead];

    return PS_SUCC;
}


VOS_UINT32 LUP_BatchDeQue(LUP_QUEUE_STRU *pstQue, VOS_VOID **ppBuf,VOS_UINT32 *pulNum)
{
    VOS_UINT32    ulCnt;
    VOS_UINT32    ulLoop;

    OSA_ASSERT_RTN(VOS_NULL_PTR != pstQue, PS_PTR_NULL);
    OSA_ASSERT_RTN(VOS_NULL_PTR != ppBuf, PS_PTR_NULL);
    OSA_ASSERT_RTN(VOS_NULL_PTR != pulNum, PS_PTR_NULL);

    ulCnt = TTF_MOD_SUB(pstQue->ulTail, pstQue->ulHead, pstQue->ulMaxNum);

    if (0 == ulCnt)
    {
        return  PS_QUE_EMPTY;
    }

    for(ulLoop=0; ulLoop<ulCnt; ulLoop++)
    {
        pstQue->ulHead  = TTF_MOD_ADD(pstQue->ulHead, 1, pstQue->ulMaxNum);
        ppBuf[ulLoop]   = pstQue->pBuff[pstQue->ulHead];
    }

    *pulNum = ulCnt;

    return PS_SUCC;
}

/*lint -e715*/

VOS_UINT32 LUP_CreateQue(VOS_UINT32 ulPid, LUP_QUEUE_STRU **ppQue,
                                VOS_UINT32 ulMaxNodeNum)
{
    LUP_QUEUE_STRU  *pstQue     = VOS_NULL_PTR;
    VOS_UINT8       *pBuffer    = VOS_NULL_PTR;

    if (0 == ulMaxNodeNum)
    {
        return PS_PARA_ERR;
    }

    if (VOS_NULL_PTR == ppQue)
    {
        return PS_PTR_NULL;
    }

    pBuffer = (VOS_UINT8*)PS_MEM_ALLOC(ulPid, (ulMaxNodeNum + 1)* sizeof(VOS_VOID*));
    if (VOS_NULL_PTR == pBuffer)
    {
        return PS_MEM_ALLOC_FAIL;
    }

    pstQue = (LUP_QUEUE_STRU *)PS_MEM_ALLOC(ulPid, sizeof(LUP_QUEUE_STRU));
    if (VOS_NULL_PTR == pstQue)
    {
        (VOS_VOID)PS_MEM_FREE(ulPid, pBuffer);
        return PS_MEM_ALLOC_FAIL;
    }

    pstQue->ulHead      = ulMaxNodeNum;
    pstQue->ulTail      = ulMaxNodeNum;
    pstQue->ulMaxNum    = ulMaxNodeNum+1;
    /*lint -e826*/
    pstQue->pBuff       = (VOS_VOID **)pBuffer;
    /*lint +e826*/
    *ppQue              = pstQue;

    return  PS_SUCC;
}


VOS_UINT32 LUP_DestroyQue(VOS_UINT32 ulPid, LUP_QUEUE_STRU **ppQue)
{
    LUP_QUEUE_STRU     *pstQue;

    if (VOS_NULL_PTR == ppQue)
    {
        return PS_PTR_NULL;
    }
    pstQue = *ppQue;
    if (VOS_NULL_PTR == pstQue)
    {
        return PS_PARA_ERR;
    }

    /*lint -e516 -esym(516,*)*/
    PS_MEM_FREE(ulPid, pstQue->pBuff);
    pstQue->pBuff   = VOS_NULL_PTR;
    PS_MEM_FREE(ulPid, pstQue);
    /*lint -e516 +esym(516,*)*/
    *ppQue          = VOS_NULL_PTR;

    return PS_SUCC;
}
/*lint +e715*/


VOS_UINT32 LUP_EnQuetoHead(LUP_QUEUE_STRU *pstQue, VOS_VOID *pNode, VOS_VOID** ppTailNode)
{
    OSA_ASSERT_RTN (VOS_NULL_PTR != pstQue, PS_PTR_NULL);

    if(VOS_NULL_PTR == ppTailNode)
    {
        return PS_PTR_NULL;
    }
    if (PS_TRUE == LUP_IsQueFull(pstQue))
    {
        /*ÒÆ³ýÎ²½Úµã*/
        *ppTailNode = pstQue->pBuff[pstQue->ulTail];
        pstQue->ulTail = TTF_MOD_SUB(pstQue->ulTail, 1, pstQue->ulMaxNum);
    }
    else
    {
        *ppTailNode = VOS_NULL_PTR;
    }
    pstQue->pBuff[pstQue->ulHead] = pNode;
    pstQue->ulHead = TTF_MOD_SUB(pstQue->ulHead, 1, pstQue->ulMaxNum);
    return PS_SUCC;
}



