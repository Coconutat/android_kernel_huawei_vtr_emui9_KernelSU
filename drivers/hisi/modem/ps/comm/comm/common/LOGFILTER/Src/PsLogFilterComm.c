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

/******************************************************************************
   1 头文件包含
******************************************************************************/
#include "PsLogFilterInterface.h"
#include "PsLib.h"


#define    THIS_FILE_ID        PS_FILE_ID_LOG_FILTER_COMM_C

/******************************************************************************
   2 外部函数变量声明
******************************************************************************/

/******************************************************************************
   3 私有定义
******************************************************************************/


/******************************************************************************
   4 全局变量定义
******************************************************************************/


/******************************************************************************
   5 函数实现
******************************************************************************/



VOS_UINT32 PS_OM_LayerMsgMatchFuncCommReg
(
    PS_OM_LAYER_MSG_MATCH_PFUNC         pFunc,
    VOS_UINT32                         *pulRegCnt,
    PS_OM_LAYER_MSG_MATCH_PFUNC        *apfuncMatchEntry,
    VOS_UINT32                          ulMsgMatchItemMaxCnt
)
{
    VOS_UINT32                          ulIndex;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulMaxLoop;

    if ((VOS_NULL_PTR == pFunc)
        || (VOS_NULL_PTR == pulRegCnt)
        || (VOS_NULL_PTR == apfuncMatchEntry))
    {
        return VOS_ERR;
    }

    ulIndex     = *pulRegCnt;
    ulMaxLoop   = PS_MIN(ulIndex, ulMsgMatchItemMaxCnt);

    for (ulLoop = 0; ulLoop < ulMaxLoop; ulLoop++)
    {
        if (apfuncMatchEntry[ulLoop] == pFunc)
        {
            return VOS_OK;
        }
    }

    if (ulMsgMatchItemMaxCnt > ulIndex)
    {
       *pulRegCnt   += 1;
        apfuncMatchEntry[ulIndex] = pFunc;

        return VOS_OK;
    }

    return VOS_ERR;
}



VOS_VOID* PS_OM_LayerMsgCommMatch
(
    VOS_VOID                           *pMsg,
    VOS_UINT32                          ulRegCnt,
    PS_OM_LAYER_MSG_MATCH_PFUNC        *apfuncMatchEntry,
    VOS_UINT32                          ulMsgMatchItemMaxCnt
)
{
    VOS_UINT32                          ulIndex;
    VOS_UINT32                          ulEntryCnt;
    VOS_VOID                           *pResult    = pMsg;

    ulEntryCnt      = PS_MIN(ulRegCnt, ulMsgMatchItemMaxCnt);

    for (ulIndex = 0; ulIndex < ulEntryCnt; ulIndex++)
    {
        if (VOS_NULL != apfuncMatchEntry[ulIndex])
        {
            pResult = apfuncMatchEntry[ulIndex]((MsgBlock*)pMsg);
            if (pMsg != pResult)
            {
                break;
            }
        }
    }

    return pResult;
}



VOS_UINT32 PS_OM_LayerMsgFilterFuncCommReg
(
    PS_OM_LAYER_MSG_FILTER_PFUNC         pFunc,
    VOS_UINT32                          *pulRegCnt,
    PS_OM_LAYER_MSG_FILTER_PFUNC        *apfuncFilterEntry,
    VOS_UINT32                           ulMsgFilterItemMaxCnt
)
{
    VOS_UINT32                          ulIndex;
    VOS_UINT32                          ulLoop;
    VOS_UINT32                          ulMaxLoop;

    if ((VOS_NULL_PTR == pFunc)
        || (VOS_NULL_PTR == pulRegCnt)
        || (VOS_NULL_PTR == apfuncFilterEntry))
    {
        return VOS_ERR;
    }

    ulIndex     = *pulRegCnt;
    ulMaxLoop   = PS_MIN(ulIndex, ulMsgFilterItemMaxCnt);

    for (ulLoop = 0; ulLoop < ulMaxLoop; ulLoop++)
    {
        if (apfuncFilterEntry[ulLoop] == pFunc)
        {
            return VOS_OK;
        }
    }

    if (ulIndex < ulMsgFilterItemMaxCnt)
    {
        *pulRegCnt   += 1;
        apfuncFilterEntry[ulIndex] = pFunc;

        return VOS_OK;
    }

    return VOS_ERR;
}


VOS_UINT32 PS_OM_LayerMsgCommFilter
(
    const VOS_VOID                     *pMsg,
    VOS_UINT32                          ulRegCnt,
    PS_OM_LAYER_MSG_FILTER_PFUNC       *apfuncFilterEntry,
    VOS_UINT32                          ulMsgFilterItemMaxCnt
)
{
    VOS_UINT32                          ulIndex;
    VOS_UINT32                          ulEntryCnt;
    VOS_UINT32                          ulResult    = VOS_FALSE;

    ulEntryCnt      = PS_MIN(ulRegCnt, ulMsgFilterItemMaxCnt);

    for (ulIndex = 0; ulIndex < ulEntryCnt; ulIndex++)
    {
        if (VOS_NULL != apfuncFilterEntry[ulIndex])
        {
            ulResult = apfuncFilterEntry[ulIndex](pMsg);
            if (VOS_FALSE != ulResult)
            {
                break;
            }
        }
    }

    return ulResult;

}


