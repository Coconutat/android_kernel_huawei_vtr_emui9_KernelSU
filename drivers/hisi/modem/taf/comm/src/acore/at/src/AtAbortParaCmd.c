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
#include "AtParse.h"
#include "ATCmdProc.h"


/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_AT_ABORTPARACMD_C

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

/*****************************************************************************
  3 函数实现
*****************************************************************************/

VOS_UINT32 AT_AbortCsgIdSearchPara(
    VOS_UINT8                           ucIndex
)
{
    /* 当前只能处理列表搜的打断, 其它命令则不进行打断操作 */
    if ( AT_CMD_CSG_LIST_SEARCH == gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        /* AT向MMA发送打断列表搜的请求 */
        if (VOS_TRUE == TAF_MMA_AbortCsgListSearchReq(WUEPS_PID_AT, gastAtClientTab[ucIndex].usClientId, 0))
        {
            /* 停止csg列表搜AT的保护定时器 */
            AT_StopRelTimer(ucIndex, &gastAtClientTab[ucIndex].hTimer);

            /* 更新当前操作类型 */
            gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_ABORT_CSG_LIST_SEARCH;

            return AT_WAIT_ASYNC_RETURN;
        }

    }

    return AT_FAILURE;
}



VOS_UINT32 At_AbortCopsPara(
    VOS_UINT8                           ucIndex
)
{
    /* 当前只能处理列表搜的打断, 其它命令则不进行打断操作 */
    if ( AT_CMD_COPS_TEST == gastAtClientTab[ucIndex].CmdCurrentOpt )
    {
        /* AT向MMA发送打断列表搜的请求 */
        if (VOS_TRUE == TAF_MMA_AbortPlmnListReq(WUEPS_PID_AT, gastAtClientTab[ucIndex].usClientId, 0))
        {
            /* 停止列表搜AT的保护定时器 */
            AT_StopRelTimer(ucIndex, &gastAtClientTab[ucIndex].hTimer);

            /* 更新当前操作类型 */
            gastAtClientTab[ucIndex].CmdCurrentOpt = AT_CMD_COPS_ABORT_PLMN_LIST;

            return AT_WAIT_ASYNC_RETURN;
        }
        else
        {
            return AT_FAILURE;
        }
    }
    else
    {
        return AT_FAILURE;
    }

}



