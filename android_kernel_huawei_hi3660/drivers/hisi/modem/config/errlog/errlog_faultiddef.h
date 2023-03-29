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

 
#ifndef __ERRLOG_CFG_FAULT_ID_DEF_H__
#define __ERRLOG_CFG_FAULT_ID_DEF_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "product_config.h"

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


enum ERR_LOG_FAULT_ID_ENUM
{
    /* TODO: python search flag satrt */
    FAULT_ID_GU_ERR_LOG_REPT                = 0X00, /* GU ErrLog 上报相关 */

    FAULT_ID_1X_OOS                         = 0XA1, /* 1x掉网 */
    FAULT_ID_1X_MO_CALL_FAIL                = 0XA2, /* 1x主叫失败 */
    FAULT_ID_1X_MT_CALL_FAIL                = 0XA3, /* 1x被叫失败 */
    FAULT_ID_1X_CALL_EXCEPTION              = 0XA4, /* 1x掉话 */
    FAULT_ID_1X_REG_FAIL                    = 0XA5, /* 注册失败 */
    FAULT_ID_1X_SHORT_MSG_SEND_FAIL         = 0XA6, /* 短息发送失败 */
    FAULT_ID_1X_SHORT_MSG_RCV_FAIL          = 0XA7, /* 短息接收失败 */
    FAULT_ID_1X_OOS_RECOVERY                = 0XA8, /* 1x掉网恢复 */
    FAULT_ID_1X_QPCH_CHANGE                 = 0xA9, /* QPCH能力指示 */
    FAULT_ID_1X_VOICE_QUALITY_BAD           = 0XB1, /* 语音质量差 */
    FAULT_ID_AP_GET_PHY_COUNT_DATA_REQ      = 0XB2, /* AP定时查询触发物理层主动上报 */
    FAULT_ID_1X_COUNT_DATA_REPORT           = 0XB3, /* CSDR 1X主动上报统计数据 */
    FAULT_ID_HRPD_OR_LTE_OOS                = 0XC1, /* CL模式下HRPD或LTE丢网*/
    FAULT_ID_HRPD_PS_CALL_EXCEPTION         = 0XC2, /* PS呼叫失败 */
    FAULT_ID_HRPD_PS_SESSION_EXCEPTION      = 0XC3, /* PS会话异常 */
    FAULT_ID_HRPD_PS_DISC_EXCEPTION         = 0XC4, /* PS断链 */
    FAULT_ID_CL_OOS_SEARCH                  = 0XC5, /* CL多模搜网过程 */
    FAULT_ID_HRPD_OR_LTE_OOS_RECOVERY       = 0XC6, /* CL模式下HRPD或LTE掉网恢复 */
    FAULT_ID_HRPD_EXCEPTION_COLLECTION      = 0XC7, /* 通用异常收集 */

    FAULT_ID_CL_LEAVE_LTE                   = 0xC8, /* CL模式下长时间不回4G */

    FAULT_ID_VOLTE_IMS_1X_SWITCH            = 0xCB, /* 电信VOLTE下， ims<->1x乒乓切换主动上报 */

    FAULT_ID_MODEM_RESTART_ABORT_SERVICE      = 0xCC,  /* modem重启导致业务释放主动上报，具体FAULT_ID需要与AP确认 */

    /* TODO: python search flag end */
    FAULT_ID_ERR_LOG_ENUM_BUT
};


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


#if (VOS_OS_VER == VOS_WIN32)
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
