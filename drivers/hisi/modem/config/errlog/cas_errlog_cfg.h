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



#ifndef __CAS_ERRLOG_CFG_H__
#define __CAS_ERRLOG_CFG_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"

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

/*****************************************************************************
 枚举名    : CAS_1X_ERR_LOG_ALARM_ID_ENUM_UINT8
 协议表格  :
 ASN.1描述 :
 枚举说明  : CAS 1X需要上报的ErrLog类型定义, ALARM ID
             同一组件的ALARM ID不能相同，否则AP侧解析ERRLOG会出错
*****************************************************************************/
enum CAS_1X_ERR_LOG_ALARM_ENUM
{
    ID_CAS_1X_ERR_LOG_ALARM_ABNORMAL_REL                    = 0x01,            /* CAS_1X_OM_ERR_LOG_ABNORMAL_REL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_EST_CNF_FAIL                    = 0x02,            /* CAS_1X_OM_ERR_LOG_EST_CNF_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_SYS_DETERMIN                    = 0x03,            /* CAS_1X_OM_ERR_LOG_SYS_DETERMIN_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_HANDOFF_FAIL_REVERT             = 0x04,            /* CAS_1X_OM_ERR_LOG_HANDOFF_FAIL_REVERT_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_REJECT                          = 0x05,            /* CAS_1X_OM_ERR_LOG_REJECT_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_OTA_DECODE_FAIL                 = 0x06,            /* CAS_1X_OM_ERR_LOG_OTA_ENCODE_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_CALL_DROP                       = 0x07,            /* CAS_1X_OM_ERR_LOG_OHM_UPDATE_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_HANDOFF_SYNC_FAIL               = 0x08,            /* CAS_1X_OM_ERR_LOG_HANDOFF_SYNC_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_TCH_HANDOFF_FAIL                = 0x09,            /* CAS_1X_OM_ERR_LOG_TCH_HANDOFF_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_NEGOTIATION_FAIL                = 0x0a,            /* CAS_1X_OM_ERR_LOG_NEGOTIATION_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_MULTI_HANDOFF                   = 0x0b,            /* CAS_1X_OM_ERR_LOG_MULTI_HANDOFF_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_AUTHENTICATION_FAIL             = 0x0c,            /* CAS_1X_OM_ERR_LOG_AUTHENTICATION_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_OTA_ENCODE_FAIL                 = 0x0d,            /* CAS_1X_OM_ERR_LOG_OTA_ENCODE_FAIL_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_COMMON_INFO                     = 0x0e,            /* CAS_1X_OM_ERR_LOG_COMMON_INFO_STRU */
    ID_CAS_1X_ERR_LOG_ALARM_PSEUDO_CELL                     = 0x0f,            /* CAS_1X_OM_ERR_LOG_PSEUDO_CELL_STRU */

    ID_CAS_1X_ERR_LOG_ALARM_BUTT                            = 0x9F             /* 从0xA1开始是HRPD的ALARM ID */
};
typedef VOS_UINT8 CAS_1X_ERR_LOG_ALARM_ENUM_UINT8;

/*****************************************************************************
 枚举名    : CAS_HRPD_ERR_LOG_ALARM_ENUM_UINT8
 协议表格  :
 ASN.1描述 :
 枚举说明  : CAS HRPD需要上报的ErrLog类型定义, ALARM ID
             同一组件的ALARM ID不能相同，否则AP侧解析ERRLOG会出错；
             A0对应的AlarmId给AP主动上报时使用
*****************************************************************************/
enum CAS_HRPD_ERR_LOG_ALARM_ENUM
{
    ID_CAS_HRPD_ERR_LOG_REPORT_IND                          = 0xA0,            /* CAS_HRPD_OM_ERR_LOG_REPORT_STRU */
    ID_CAS_HRPD_ERR_LOG_CONN_OPEN_FAIL                      = 0xA1,            /* CAS_HRPD_OM_ERR_LOG_CONN_OPEN_FAIL_SRTU */
    ID_CAS_HRPD_ERR_LOG_SESSION_NEG_FAIL                    = 0xA2,            /* CAS_HRPD_OM_ERR_LOG_SESSION_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_NETWORK_LOST                        = 0xA3,            /* CAS_HRPD_OM_ERR_LOG_NETWORK_LOST_STRU */
    ID_CAS_HRPD_ERR_LOG_RESEL_TO_LTE_FAIL                   = 0xA4,            /* CAS_HRPD_OM_ERR_LOG_C2L_RESEL_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_L2C_RESEL_FAIL                      = 0xA5,            /* CAS_HRPD_OM_ERR_LOG_C2L_RESEL_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_L2C_REDIRECT_FAIL                   = 0xA6,            /* CAS_HRPD_OM_ERR_LOG_C2L_RESEL_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_CODEC_FAIL                          = 0xA7,            /* CAS_HRPD_OM_ERR_LOG_C2L_RESEL_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_NETWORK_SEARCH_FAIL                 = 0xA8,            /* CAS_HRPD_OM_ERR_LOG_C2L_RESEL_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_COMMON_INFO                         = 0xA9,            /* CAS_HRPD_OM_ERR_LOG_COMMON_INFO_STRU */
    ID_CAS_HRPD_ERR_LOG_TCA_RCV_FAIL                        = 0xAA,            /* CAS_HRPD_OM_ERR_LOG_TCA_RCV_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_C2L_BSR_FAIL                        = 0xAB,            /* CAS_HRPD_OM_ERR_LOG_C2L_BSR_FAIL_STRU */
    ID_CAS_HRPD_ERR_LOG_CL_MULTI_RESEL                      = 0xAC,            /* CAS_HRPD_OM_ERR_LOG_CL_MULTI_RESEL_STRU */
    ID_CAS_HRPD_ERR_LOG_PERSONALTY_COUNT_INVALID            = 0xAD,            /* CAS_HRPD_OM_ERR_LOG_PERSONALTY_COUNT_INVALID_STRU */

    ID_CAS_HRPD_ERR_LOG_BUTT                                = 0xFF
};
typedef VOS_UINT8 CAS_HRPD_ERR_LOG_ALARM_ENUM_UINT8;


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


#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of cas_errlog_cfg.h */


