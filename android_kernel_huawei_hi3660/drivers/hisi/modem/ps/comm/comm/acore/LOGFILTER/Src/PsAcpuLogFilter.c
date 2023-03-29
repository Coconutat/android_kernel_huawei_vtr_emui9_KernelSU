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
#include "PsAcpuLogFilter.h"
#include "PsLogFilterComm.h"
#include "msp_diag_comm.h"


#define    THIS_FILE_ID        PS_FILE_ID_ACPU_LOG_FILTER_C

/******************************************************************************
   2 外部函数变量声明
******************************************************************************/

/******************************************************************************
   3 私有定义
******************************************************************************/


/******************************************************************************
   4 全局变量定义
******************************************************************************/


PS_OM_ACPU_LAYER_MSG_FILTER_CTRL_STRU        g_stAcpuLayerMsgFilterCtrl =
    {0, {VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR}};
PS_OM_ACPU_LAYER_MSG_MATCH_CTRL_STRU         g_stAcpuLayerMsgMatchCtrl  =
    {0, {VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR}};

/******************************************************************************
   5 函数实现
******************************************************************************/



VOS_VOID* PS_OM_LayerMsgMatch_Acpu
(
    VOS_VOID                           *pMsg
)
{
    VOS_VOID                           *pResult;

    pResult = PS_OM_LayerMsgCommMatch(pMsg,
                        g_stAcpuLayerMsgMatchCtrl.ulRegCnt,
                        g_stAcpuLayerMsgMatchCtrl.apfuncMatchEntry, 
                        PS_OM_ACPU_LAYER_MSG_MATCH_ITEM_MAX_CNT);
    return pResult;
}


VOS_UINT32 PS_OM_LayerMsgFilter_Acpu
(
    const VOS_VOID                     *pMsg
)
{  
    VOS_UINT32                          ulResult;
 
    ulResult = PS_OM_LayerMsgCommFilter(pMsg, 
                    g_stAcpuLayerMsgFilterCtrl.ulRegCnt, 
                    g_stAcpuLayerMsgFilterCtrl.apfuncFilterEntry, 
                    PS_OM_ACPU_LAYER_MSG_FILTER_ITEM_MAX_CNT);
    return ulResult;    
}

/* 防止PC工程编译过程中函数多重定义 */

VOS_VOID PS_OM_LayerMsgMatchInit(VOS_VOID)
{
    (VOS_VOID)DIAG_TraceMatchFuncReg(PS_OM_LayerMsgMatch_Acpu);

    return;
}


VOS_UINT32 PS_OM_LayerMsgMatchFuncReg
(
    PS_OM_LAYER_MSG_MATCH_PFUNC         pFunc
)
{
    VOS_UINT32                          ulResult;
          
    ulResult = PS_OM_LayerMsgMatchFuncCommReg(pFunc,
                    &(g_stAcpuLayerMsgMatchCtrl.ulRegCnt),
                    g_stAcpuLayerMsgMatchCtrl.apfuncMatchEntry,
                    PS_OM_ACPU_LAYER_MSG_MATCH_ITEM_MAX_CNT);

    return ulResult;
}


VOS_VOID PS_OM_LayerMsgFilterInit(VOS_VOID)
{
   (VOS_VOID)DIAG_TraceFilterFuncReg(PS_OM_LayerMsgFilter_Acpu);

    return;
}


VOS_UINT32 PS_OM_LayerMsgFilterFuncReg
(
    PS_OM_LAYER_MSG_FILTER_PFUNC        pFunc
)
{
    VOS_UINT32                          ulResult;
    
    ulResult = PS_OM_LayerMsgFilterFuncCommReg(pFunc,
                    &(g_stAcpuLayerMsgFilterCtrl.ulRegCnt),
                    g_stAcpuLayerMsgFilterCtrl.apfuncFilterEntry,
                    PS_OM_ACPU_LAYER_MSG_FILTER_ITEM_MAX_CNT);
    return ulResult;
}


