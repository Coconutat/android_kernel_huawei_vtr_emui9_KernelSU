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

#ifndef __NAS_ERRLOG_CFG_H__
#define __NAS_ERRLOG_CFG_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "product_config.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


enum NAS_ERR_LOG_ALM_ID_ENUM
{
    NAS_ERR_LOG_ALM_CS_REG_FAIL                             = 0x01,             /* CS域注册失败 */
    NAS_ERR_LOG_ALM_PS_REG_FAIL                             = 0x02,             /* PS域注册失败 */
    NAS_ERR_LOG_ALM_OOS_NW_SRCH_PROC_INFO                   = 0x03,             /* 出服务区的搜索过程信息 */
    NAS_ERR_LOG_ALM_CS_CALL_FAIL                            = 0x04,             /* CS呼叫失败及异常挂断 */
    NAS_ERR_LOG_ALM_PS_CALL_FAIL                            = 0x05,             /* PS呼叫失败及异常挂断 */
    NAS_ERR_LOG_ALM_SMS_FAIL                                = 0x06,             /* 短信失败 */
    NAS_ERR_LOG_ALM_VC_OPT_FAIL                             = 0x07,             /* VC操作失败 */
    NAS_ERR_LOG_ALM_CS_PAGING_FAIL                          = 0x08,             /* CS PAGING fail */
    NAS_ERR_LOG_ALM_CS_MT_CALL_FAIL                         = 0x09,             /* CS MT fail */
    NAS_ERR_LOG_ALM_CSFB_MT_CALL_FAIL                       = 0x0a,             /* CSFB MT fail */
    NAS_ERR_LOG_ALM_MNTN                                    = 0x0b,             /* 故障告警的可维可测 */
    NAS_ERR_LOG_ALM_NW_DETACH_IND                           = 0x0c,             /* 网络发起的DETACH指示 */

    NAS_ERR_LOG_ALM_PS_SRV_REG_FAIL                         = 0x0d,             /* PS SERVICE被拒 */
    NAS_ERR_LOG_ALM_CM_SRV_REJ_IND                          = 0x0e,             /* CS SERVICE拒绝 */
    NAS_ERR_LOG_ALM_MO_DETACH_IND                           = 0x0f,             /* 本地发起的DETACH */
    NAS_ERR_LOG_ALM_RAT_FREQUENTLY_SWITCH                   = 0x10,             /* 4G与2/3G频繁切换 */

    NAS_ERR_LOG_ALM_OOS_BEGIN                               = 0x11,             /* 无服务开始 */
    NAS_ERR_LOG_ALM_OOS_END                                 = 0x12,             /* 无服务结束 */

    NAS_ERR_LOG_ALM_PROCESS_INFO_IND                        = 0x13,             /* 过程信息指示 */

    NAS_ERR_LOG_ALM_FFT_SRCH_MCC_INFO                       = 0x14,             /* FFT搜索MCC的结果 */

    NAS_ERR_LOG_ALM_GET_CARD_IMSI_FAIL                      = 0x15,             /* 获取卡的IMSI时失败 */

    NAS_ERR_LOG_ALM_SRVCC_FAIL_INFO                         = 0x16,             /* FFT搜索MCC的结果 */

    NAS_ERR_LOG_ALM_LTE_LOST                                = 0x17,             /* 主动上报长时间不回LTE的事件 */
    NAS_ERR_LOG_ALM_LTE_LOST_DETAIL_INFO                    = 0x18,             /* 写缓冲区里的不回4G的记录的详细信息 */

    NAS_ERR_LOG_ALM_MM_CSFB_INFO                            = 0x19,             /* MM的CSFB信息 */
    NAS_ERR_LOG_ALM_PROCESS_INFO_MM_RCV_CALL_SRV            = 0x1A,             /* MM接收到呼叫业务的过程信息 */
    NAS_ERR_LOG_ALM_PROCESS_INFO_MM_AUTH                    = 0x1B,             /* MM鉴权的过程信息 */
    NAS_ERR_LOG_ALM_PROCESS_INFO_CC_ALERTING                = 0x1C,             /* CC ALERTING的过程信息 */
    NAS_ERR_LOG_ALM_PROCESS_INFO_CC_PROGRESS_IND            = 0x1D,             /* Progress Indicator的过程信息 */
    NAS_ERR_LOG_ALM_CC_ABNORNAL_HANGUP                      = 0x1E,             /* CC的异常挂断 */
    NAS_ERR_LOG_ALM_CC_REEST                                = 0x1F,             /* CC业务重建 */
    NAS_ERR_LOG_ALM_TAF_CALL_INFO_STATS                     = 0x20,             /* TAF CALL的呼叫统计信息 */

    NAS_ERR_LOG_ALM_MM_PAGING_RSP_RETRY_EST_SUCC            = 0x21,             /* 被叫寻呼响应重发建链成功信息 */

    NAS_ERR_LOG_ALM_INTER_SYSTIM_RESUME_TIMER_ABNORMAL      = 0x22,             /* 异系统resume定时器句柄空 */

    NAS_ERR_LOG_ALM_MM_PAGING_CACHE_RETRY_SUCC              = 0x23,             /* 被叫寻呼缓存发成功信息 */

    NAS_ERR_LOG_ALM_CHECK_SMS_STATUS_TIMEOUT                = 0x24,             /* TAF获取短信状态列表超时 */

    NAS_ERR_LOG_ALM_OOS_STATS_INFO                          = 0x25,           /* 降功耗的丢网CHR的统计丢网信息 */

    NAS_ERR_LOG_ALM_CC_DECODE_FAIL                          = 0x26,             /* CC解码失败 */

    NAS_ERR_LOG_ALM_CC_TIMER_INFO                           = 0x27,             /* 定时器时长上报 */
    NAS_ERR_LOG_ALM_MM_MT_S12_DISCARD_SYSINFO_INFO          = 0x28,             /* 被叫在S12时忽略系统消息的场景记录 */

    NAS_ERR_LOG_ALM_CC_INCOMPATIBLE_INFO                    = 0x29,             /* 冲突场景上报 */

    NAS_ERR_LOG_ALM_1X_COMMON_INFO                          = 0x40,              /*1x公共信息*/
    NAS_ERR_LOG_ALM_1X_OOS_REPORT                           = 0x41,              /*1x搜网失败上报*/
    NAS_ERR_LOG_ALM_1X_OOS_RECOVERY                         = 0x42,              /*1x搜网失败恢复上报*/
    NAS_ERR_LOG_ALM_1X_OOS_SEARCH                           = 0x43,              /*1x搜网失败搜网过程*/
    NAS_ERR_LOG_ALM_1X_CS_MO_CALL_EXCEPTION                 = 0x44,              /*1x CS主叫失败*/
    NAS_ERR_LOG_ALM_1X_MT_SERVICE_EXCEPTION                 = 0x45,              /*1x被叫失败*/
    NAS_ERR_LOG_ALM_1X_CS_CONVERSATION_EXCEPTION            = 0x46,              /*1x CS掉话*/
    NAS_ERR_LOG_ALM_1X_CODEC_FAIL                           = 0x47,              /* 1X空口消息编解码失败 */
    NAS_ERR_LOG_ALM_1X_QPCH_SUPPORT                         = 0x48,             /* 1x QPCH能力支持信息 */

    NAS_ERR_LOG_ALM_1X_OOS_PROCESS_INFO                     = 0x49,             /* CL丢网频繁上报特性:1x 丢网恢复信息 */

    NAS_ERR_LOG_ALM_1X_MT_REEST_INFO                        = 0x4a,             /* 1x被叫重试信息 */

    NAS_ERR_LOG_ALM_HRPD_COMMON_INFO                        = 0x80,              /*Hrpd公共信息*/
    NAS_ERR_LOG_ALM_CDMA_PS_CALL_FAIL                       = 0x81,              /*Hrpd PS 呼叫失败*/
    NAS_ERR_LOG_ALM_CDMA_PS_DISC_EXCEPTION                  = 0x82,              /*Hrpd PS 断链*/
    NAS_ERR_LOG_ALM_HRPD_UATI_FAIL                          = 0x83,              /*Uati失败*/
    NAS_ERR_LOG_ALM_HRPD_SESSION_FAIL                       = 0x84,              /*Session失败*/
    NAS_ERR_LOG_ALM_HRPD_SESSION_EXCEP_DEACT                = 0x85,              /*Session异常去激活*/
    NAS_ERR_LOG_ALM_CL_OOS_SEARCH                           = 0x86,              /*CL多模搜网搜索过程*/
    NAS_ERR_LOG_ALM_HRPD_OR_LTE_OOS_REPORT                  = 0x87,             /* CL模式下HRPD或LTE丢网上报 */
    NAS_ERR_LOG_ALM_HRPD_OR_LTE_OOS_RECOVERY                = 0x88,              /* CL模式下HRPD或LTE丢网恢复上报 */
    NAS_ERR_LOG_ALM_HRPD_OOS_SEARCH                         = 0x89,              /* HRPD搜网失败搜网过程 */
    NAS_ERR_LOG_ALM_XREG_FAIL_INFO                          = 0x8A,
    NAS_ERR_LOG_ALM_XSMS_SND_FAIL                           = 0x8B,
    NAS_ERR_LOG_ALM_XSMS_RCV_FAIL                           = 0x8C,
    NAS_ERR_LOG_ALM_CL_LTE_COMMON_INFO                      = 0x8D,           /* CL模式下，LTE公共信息 */

    NAS_ERR_LOG_ALM_TIMER_ERR                               = 0x8E,           /* 定时器异常 */

    NAS_ERR_LOG_ALM_CL_LEAVE_LTE_REPORT                     = 0X8F,              /* 长时间不回LTE主动上报 */
    NAS_ERR_LOG_ALM_CL_LEAVE_LTE_INFO                       = 0X90,              /* 长时间不回LTE信息 */
    NAS_ERR_LOG_ALM_HRPD_CODEC_FAIL                         = 0x91,            /* HRPD空口消息编解码失败 */

    NAS_ERR_LOG_ALM_HRPD_SESSION_CLOSE_PROT_NEG_ERR         = 0x92,            /* HRPD SessionClose原因值为PROT_NEG_ERR的信息 */
    NAS_ERR_LOG_ALM_QUICK_BACK_TO_4G_STATISTIC_INFO         = 0x93,            /* APS触发，3G下快回4G信息统计 */

    NAS_ERR_LOG_ALM_VOLTE_IMS_1X_SWITCH_INFO                = 0x94,             /* 电信VOLTE 1x和ims乒乓切换异常CHR信息 */
    NAS_ERR_LOG_ALM_CSCALL_IMS_FAIL_1X_REDIAL_INFO          = 0x95,             /* 电信VOLTE IMS呼叫失败后换域重播的信息 */

    NAS_ERR_LOG_ALM_DO_LTE_OOS_PROCESS_INFO                 = 0x96,             /* CL丢网频繁上报特性:DO_LTE 丢网恢复信息 */

    NAS_ERR_LOG_MODEM_RESTART_ABORT_SERVICE                 = 0x97,               /* Modem异常重启打断业务，上报chr信息 */

    NAS_ERR_LOG_ALM_ID_BUTT
};
typedef VOS_UINT16  NAS_ERR_LOG_ALM_ID_ENUM_U16;

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

#endif
