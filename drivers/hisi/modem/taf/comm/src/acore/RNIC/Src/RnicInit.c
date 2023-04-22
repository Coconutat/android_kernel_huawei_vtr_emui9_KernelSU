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
#include "v_id.h"
#include "RnicProcMsg.h"
#include "v_typdef.h"
#include "RnicLog.h"
#include "RnicCtx.h"
#include "RnicInit.h"
#include "RnicEntity.h"


/*****************************************************************************
协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/

#define    THIS_FILE_ID        PS_FILE_ID_RNIC_INITS_C

/*****************************************************************************
2 全局变量定义
*****************************************************************************/

/*****************************************************************************
3 函数实现
*****************************************************************************/

VOS_INT RNIC_CCpuResetCallback(
    DRV_RESET_CB_MOMENT_E               enParam,
    VOS_INT                             iUserData
)
{
    RNIC_CCPU_RESET_IND_STRU           *pstMsg = VOS_NULL_PTR;

    /* 复位前 */
    if (MDRV_RESET_CB_BEFORE == enParam)
    {
        printk("\n RNIC_CCpuResetCallback before reset enter, %u \n", VOS_GetSlice());

        /* 构造消息 */
        pstMsg = (RNIC_CCPU_RESET_IND_STRU*)PS_ALLOC_MSG_WITH_HEADER_LEN(ACPU_PID_RNIC,
                                                                    sizeof(RNIC_CCPU_RESET_IND_STRU));
        if (VOS_NULL_PTR == pstMsg)
        {
            printk("\n RNIC_CCpuResetCallback before reset alloc msg fail, %u \n", VOS_GetSlice());
            return VOS_ERROR;
        }

        /* 填写消息头 */
        pstMsg->ulReceiverPid               = ACPU_PID_RNIC;
        pstMsg->enMsgId                     = ID_RNIC_CCPU_RESET_START_IND;

        /* 发消息 */
        if (VOS_OK != PS_SEND_MSG(ACPU_PID_RNIC, pstMsg))
        {
            printk("\n RNIC_CCpuResetCallback before reset send msg fail, %u \n", VOS_GetSlice());
            return VOS_ERROR;
        }

        /* 等待回复信号量初始为锁状态，等待消息处理完后信号量解锁。 */
        if (VOS_OK != VOS_SmP(RNIC_GetResetSem(), RNIC_RESET_TIMEOUT_LEN))
        {
            printk("\n RNIC_CCpuResetCallback before reset Vos_SmP fail, %u \n", VOS_GetSlice());
            RNIC_DBG_LOCK_BINARY_SEM_FAIL_NUM(1);

            return VOS_ERROR;
        }

        return VOS_OK;
    }
    /* 复位后 */
    else if (MDRV_RESET_CB_AFTER == enParam)
    {
        /* 记录复位的次数 */
        RNIC_DBG_SAVE_CCPU_RESET_SUCCESS_NUM(1);

        return VOS_OK;
    }
    else
    {
        return VOS_ERROR;
    }
}


VOS_UINT32 RNIC_PidInit (enum VOS_INIT_PHASE_DEFINE enPhase)
{
    RNIC_CTX_STRU                      *pstRnicCtx;

    pstRnicCtx                          = VOS_NULL_PTR;

    switch(enPhase)
    {
        case VOS_IP_LOAD_CONFIG:

            /* 初始化RNIC上下文信息 */
            pstRnicCtx = RNIC_GetRnicCtxAddr();
            RNIC_InitCtx(pstRnicCtx);

            /* 给低软注册回调函数，用于C核单独复位的处理 */
            mdrv_sysboot_register_reset_notify(NAS_RNIC_FUNC_PROC_NAME,
                                     RNIC_CCpuResetCallback,
                                     0,
                                     ACPU_RESET_PRIORITY_RNIC);


            break;

        case VOS_IP_FARMALLOC:
        case VOS_IP_INITIAL:
        case VOS_IP_ENROLLMENT:
        case VOS_IP_LOAD_DATA:
        case VOS_IP_FETCH_DATA:
        case VOS_IP_STARTUP:
        case VOS_IP_RIVAL:
        case VOS_IP_KICKOFF:
        case VOS_IP_STANDBY:
        case VOS_IP_BROADCAST_STATE:
        case VOS_IP_RESTART:
            break;
        default:
            break;
    }

    return VOS_OK;
}




VOS_UINT32 RNIC_FidInit (enum VOS_INIT_PHASE_DEFINE enPhase)
{
    VOS_UINT32                          ulRslt;

    switch(enPhase)
    {
        case VOS_IP_LOAD_CONFIG:

            /* 网卡模块注册PID */
            ulRslt = VOS_RegisterPIDInfo(ACPU_PID_RNIC,
                                (Init_Fun_Type)RNIC_PidInit,
                                (Msg_Fun_Type)RNIC_ProcMsg);
            if( VOS_OK != ulRslt )
            {
                RNIC_ERROR_LOG1(ACPU_PID_RNIC, "reg UEPS_PID_RNIC VOS_RegisterPIDInfo FAIL! ulRslt: %d\n", ulRslt);

                return VOS_ERR;
            }

            ulRslt = VOS_RegisterMsgTaskPrio(ACPU_FID_RNIC, VOS_PRIORITY_P6);

            if( VOS_OK != ulRslt )
            {
                return VOS_ERR;
            }

            break;

        case VOS_IP_FARMALLOC:
        case VOS_IP_INITIAL:
        case VOS_IP_ENROLLMENT:
        case VOS_IP_LOAD_DATA:
        case VOS_IP_FETCH_DATA:
        case VOS_IP_STARTUP:
        case VOS_IP_RIVAL:
        case VOS_IP_KICKOFF:
        case VOS_IP_STANDBY:
        case VOS_IP_BROADCAST_STATE:
        case VOS_IP_RESTART:
        case VOS_IP_BUTT:
            break;

        default:
            break;
    }

    return VOS_OK;
}    /* RNIC_FidInit */


