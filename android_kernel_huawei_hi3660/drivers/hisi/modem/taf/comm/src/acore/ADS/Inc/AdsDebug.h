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

#ifndef __ADSTEST_H__
#define __ADSTEST_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/

#include "vos.h"
#include "product_config.h"
#include "PsCommonDef.h"
#include "AdsIntraMsg.h"
#include "AdsCtx.h"
#include "msp_diag_comm.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/

#define ADS_FLOW_DL_DEFAULT_RPT_THRESHOLD   (5000000)
#define ADS_FLOW_UL_DEFAULT_RPT_THRESHOLD   (500000)

#define ADS_LOG_BUFF_LEN 	                (256)

#if defined(_lint)
#define ADS_DBG_PRINT(lvl, fmt, ...)\
            PS_PRINTF(fmt, ##__VA_ARGS__)
#else
#define ADS_DBG_PRINT(lvl, fmt, ...)\
            ADS_LogPrintf(lvl, "[slice:%u][%s] "fmt, VOS_GetSlice(), __FUNCTION__, ##__VA_ARGS__)
#endif

#define ADS_TRACE_LOW(...)\
            ADS_DBG_PRINT(ADS_LOG_LEVEL_INFO, __VA_ARGS__)

#define ADS_TRACE_MED(...)\
            ADS_DBG_PRINT(ADS_LOG_LEVEL_NOTICE, __VA_ARGS__)

#define ADS_TRACE_HIGH(...)\
            ADS_DBG_PRINT(ADS_LOG_LEVEL_ERROR, __VA_ARGS__)


/* 上行IPF中断事件统计 */
#define ADS_DBG_UL_QUE_NON_EMPTY_TRIG_EVENT(n)      (g_stAdsStats.stUlComStatsInfo.ulULQueNonEmptyTrigEvent += (n))
#define ADS_DBG_UL_QUE_HIT_THRES_TRIG_EVENT(n)      (g_stAdsStats.stUlComStatsInfo.ulULQueHitThresTrigEvent += (n))
#define ADS_DBG_UL_TMR_HIT_THRES_TRIG_EVENT(n)      (g_stAdsStats.stUlComStatsInfo.ulULTmrHitThresTrigEvent += (n))
#define ADS_DBG_UL_10MS_TMR_TRIG_EVENT(n)           (g_stAdsStats.stUlComStatsInfo.ulUL10MsTmrTrigEvent += (n))
#define ADS_DBG_UL_SPE_INT_TRIG_EVENT(n)            (g_stAdsStats.stUlComStatsInfo.ulULSpeIntTrigEvent += (n))
#define ADS_DBG_UL_PROC_EVENT_NUM(n)                (g_stAdsStats.stUlComStatsInfo.ulULProcEventNum += (n))

/* 上行数据统计 */
#define ADS_DBG_UL_RMNET_RX_PKT_NUM(n)              (g_stAdsStats.stUlComStatsInfo.ulULRmnetRxPktNum += (n))
#define ADS_DBG_UL_RMNET_MODEMID_ERR_NUM(n)         (g_stAdsStats.stUlComStatsInfo.ulULRmnetModemIdErrNum += (n))
#define ADS_DBG_UL_RMNET_RABID_NUM(n)               (g_stAdsStats.stUlComStatsInfo.ulULRmnetRabIdErrNum += (n))
#define ADS_DBG_UL_RMNET_ENQUE_SUCC_NUM(n)          (g_stAdsStats.stUlComStatsInfo.ulULRmnetEnQueSuccNum += (n))
#define ADS_DBG_UL_RMNET_ENQUE_FAIL_NUM(n)          (g_stAdsStats.stUlComStatsInfo.ulULRmnetEnQueFailNum += (n))
#define ADS_DBG_UL_PKT_ENQUE_SUCC_NUM(n)            (g_stAdsStats.stUlComStatsInfo.ulULPktEnQueSuccNum += (n))
#define ADS_DBG_UL_PKT_ENQUE_FAIL_NUM(n)            (g_stAdsStats.stUlComStatsInfo.ulULPktEnQueFailNum += (n))

/* 上行BD统计 */
#define ADS_DBG_UL_BDQ_CFG_IPF_HAVE_NO_BD_NUM(n)    (g_stAdsStats.stUlComStatsInfo.ulULBdqCfgIpfHaveNoBd += (n))
#define ADS_DBG_UL_BDQ_CFG_BD_SUCC_NUM(n)           (g_stAdsStats.stUlComStatsInfo.ulULBdqCfgBdSuccNum += (n))
#define ADS_DBG_UL_BDQ_CFG_BD_FAIL_NUM(n)           (g_stAdsStats.stUlComStatsInfo.ulULBdqCfgBdFailNum += (n))
#define ADS_DBG_UL_BDQ_CFG_IPF_SUCC_NUM(n)          (g_stAdsStats.stUlComStatsInfo.ulULBdqCfgIpfSuccNum += (n))
#define ADS_DBG_UL_BDQ_CFG_IPF_FAIL_NUM(n)          (g_stAdsStats.stUlComStatsInfo.ulULBdqCfgIpfFailNum += (n))
#define ADS_DBG_UL_BDQ_SAVE_SRC_MEM_NUM(n)          (g_stAdsStats.stUlComStatsInfo.ulULBdqSaveSrcMemNum += (n))
#define ADS_DBG_UL_BDQ_FREE_SRC_MEM_NUM(n)          (g_stAdsStats.stUlComStatsInfo.ulULBdqFreeSrcMemNum += (n))
#define ADS_DBG_UL_BDQ_FREE_SRC_MEM_ERR(n)          (g_stAdsStats.stUlComStatsInfo.ulULBdqFreeSrcMemErr += (n))

/* 上行组包统计 */
#define ADS_DBG_UL_WM_LEVEL_1_HIT_NUM(n)            (g_stAdsStats.stUlComStatsInfo.ulULWmLevel1HitNum += (n))
#define ADS_DBG_UL_WM_LEVEL_2_HIT_NUM(n)            (g_stAdsStats.stUlComStatsInfo.ulULWmLevel2HitNum += (n))
#define ADS_DBG_UL_WM_LEVEL_3_HIT_NUM(n)            (g_stAdsStats.stUlComStatsInfo.ulULWmLevel3HitNum += (n))
#define ADS_DBG_UL_WM_LEVEL_4_HIT_NUM(n)            (g_stAdsStats.stUlComStatsInfo.ulULWmLevel4HitNum += (n))

/* 上行复位统计 */
#define ADS_DBG_UL_RESET_CREATE_SEM_FAIL_NUM(n)     (g_stAdsStats.stResetStatsInfo.ulULResetCreateSemFailNum += (n))
#define ADS_DBG_UL_RESET_LOCK_FAIL_NUM(n)           (g_stAdsStats.stResetStatsInfo.ulULResetLockFailNum += (n))
#define ADS_DBG_UL_RESET_SUCC_NUM(n)                (g_stAdsStats.stResetStatsInfo.ulULResetSuccNum += (n))

/* 下行IPF中断事件统计 */
#define ADS_DBG_DL_RCV_IPF_RD_INT_NUM(n)            (g_stAdsStats.stDlComStatsInfo.ulDLRcvIpfRdIntNum += (n))
#define ADS_DBG_DL_CCORE_RESET_TRIG_EVENT(n)        (g_stAdsStats.stDlComStatsInfo.ulDLCCoreResetTrigEvent += (n))
#define ADS_DBG_DL_PROC_IPF_RD_EVENT_NUM(n)         (g_stAdsStats.stDlComStatsInfo.ulDLProcIpfRdEventNum += (n))
#define ADS_DBG_DL_RCV_IPF_ADQ_EMPTY_INT_NUM(n)     (g_stAdsStats.stDlComStatsInfo.ulDLRcvIpfAdqEmptyIntNum += (n))
#define ADS_DBG_DL_RECYCLE_MEM_TRIG_EVENT(n)        (g_stAdsStats.stDlComStatsInfo.ulDLRecycleMemTrigEvent += (n))
#define ADS_DBG_DL_PROC_IPF_AD_EVENT_NUM(n)         (g_stAdsStats.stDlComStatsInfo.ulDLProcIpfAdEventNum += (n))

/* 下行RD统计 */
#define ADS_DBG_DL_RDQ_RX_RD_NUM(n)                 (g_stAdsStats.stDlComStatsInfo.ulDLRdqRxRdNum += (n))
#define ADS_DBG_DL_RDQ_GET_RD0_NUM(n)               (g_stAdsStats.stDlComStatsInfo.ulDLRdqGetRd0Num += (n))
#define ADS_DBG_DL_RDQ_TRANS_MEM_FAIL_NUM(n)        (g_stAdsStats.stDlComStatsInfo.ulDLRdqTransMemFailNum += (n))
#define ADS_DBG_DL_RDQ_RX_NORM_PKT_NUM(n)           (g_stAdsStats.stDlComStatsInfo.ulDLRdqRxNormPktNum += (n))
#define ADS_DBG_DL_RDQ_RX_ND_PKT_NUM(n)             (g_stAdsStats.stDlComStatsInfo.ulDLRdqRxNdPktNum += (n))
#define ADS_DBG_DL_RDQ_RX_DHCP_PKT_NUM(n)           (g_stAdsStats.stDlComStatsInfo.ulDLRdqRxDhcpPktNum += (n))
#define ADS_DBG_DL_RDQ_RX_ERR_PKT_NUM(n)            (g_stAdsStats.stDlComStatsInfo.ulDLRdqRxErrPktNum += (n))
#define ADS_DBG_DL_RDQ_FILTER_ERR_PKT_NUM(n)        (g_stAdsStats.stDlComStatsInfo.ulDLRdqFilterErrNum += (n))

/* 下行数据统计*/
#define ADS_DBG_DL_RMNET_TX_PKT_NUM(n)              (g_stAdsStats.stDlComStatsInfo.ulDLRmnetTxPktNum += (n))
#define ADS_DBG_DL_RMNET_MODEMID_ERR_NUM(n)         (g_stAdsStats.stDlComStatsInfo.ulDLRmnetModemIdErrNum += (n))
#define ADS_DBG_DL_RMNET_RABID_ERR_NUM(n)           (g_stAdsStats.stDlComStatsInfo.ulDLRmnetRabIdErrNum += (n))
#define ADS_DBG_DL_RMNET_NO_FUNC_FREE_PKT_NUM(n)    (g_stAdsStats.stDlComStatsInfo.ulDLRmnetNoFuncFreePktNum += (n))

/* 下行AD统计  */
#define ADS_DBG_DL_ADQ_ALLOC_SYS_MEM_SUCC_NUM(n)    (g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocSysMemSuccNum += (n))
#define ADS_DBG_DL_ADQ_ALLOC_SYS_MEM_FAIL_NUM(n)    (g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocSysMemFailNum += (n))
#define ADS_DBG_DL_ADQ_ALLOC_MEM_SUCC_NUM(n)        (g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocMemSuccNum += (n))
#define ADS_DBG_DL_ADQ_ALLOC_MEM_FAIL_NUM(n)        (g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocMemFailNum += (n))
#define ADS_DBG_DL_ADQ_FREE_MEM_NUM(n)              (g_stAdsStats.stDlComStatsInfo.ulDLAdqFreeMemNum  += (n))
#define ADS_DBG_DL_ADQ_RECYCLE_MEM_SUCC_NUM(n)      (g_stAdsStats.stDlComStatsInfo.ulDLAdqRecycleMemSuccNum += (n))
#define ADS_DBG_DL_ADQ_RECYCLE_MEM_FAIL_NUM(n)      (g_stAdsStats.stDlComStatsInfo.ulDLAdqRecycleMemFailNum += (n))
#define ADS_DBG_DL_ADQ_GET_FREE_AD_SUCC_NUM(n)      (g_stAdsStats.stDlComStatsInfo.ulDLAdqGetFreeAdSuccNum += (n))
#define ADS_DBG_DL_ADQ_GET_FREE_AD_FAIL_NUM(n)      (g_stAdsStats.stDlComStatsInfo.ulDLAdqGetFreeAdFailNum += (n))
#define ADS_DBG_DL_ADQ_CFG_AD_NUM(n)                (g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgAdNum += (n))
#define ADS_DBG_DL_ADQ_CFG_AD0_NUM(n)               (g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgAd0Num += (n))
#define ADS_DBG_DL_ADQ_CFG_AD1_NUM(n)               (g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgAd1Num += (n))
#define ADS_DBG_DL_ADQ_CFG_IPF_SUCC_NUM(n)          (g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgIpfSuccNum += (n))
#define ADS_DBG_DL_ADQ_CFG_IPF_FAIL_NUM(n)          (g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgIpfFailNum += (n))
#define ADS_DBG_DL_ADQ_START_EMPTY_TMR_NUM(n)       (g_stAdsStats.stDlComStatsInfo.ulDLAdqStartEmptyTmrNum += (n))
#define ADS_DBG_DL_ADQ_EMPTY_TMR_TIMEOUT_NUM(n)     (g_stAdsStats.stDlComStatsInfo.ulDLAdqEmptyTmrTimeoutNum += (n))
#define ADS_DBG_DL_ADQ_RCV_AD0_EMPTY_INT_NUM(n)     (g_stAdsStats.stDlComStatsInfo.ulDLAdqRcvAd0EmptyIntNum += (n))
#define ADS_DBG_DL_ADQ_RCV_AD1_EMPTY_INT_NUM(n)     (g_stAdsStats.stDlComStatsInfo.ulDLAdqRcvAd1EmptyIntNum += (n))

/* 下行流控统计 */
#define ADS_DBG_DL_FC_ACTIVATE_NUM(n)               (g_stAdsStats.stDlComStatsInfo.ulDLFcActivateNum += (n))
#define ADS_DBG_DL_FC_TMR_TIMEOUT_NUM(n)            (g_stAdsStats.stDlComStatsInfo.ulDLFcTmrTimeoutNum += (n))

/* 下行复位统计 */
#define ADS_DBG_DL_RESET_CREATE_SEM_FAIL_NUM(n)     (g_stAdsStats.stResetStatsInfo.ulDLResetCreateSemFailNum += (n))
#define ADS_DBG_DL_RESET_LOCK_FAIL_NUM(n)           (g_stAdsStats.stResetStatsInfo.ulDLResetLockFailNum += (n))
#define ADS_DBG_DL_RESET_SUCC_NUM(n)                (g_stAdsStats.stResetStatsInfo.ulDLResetSuccNum += (n))


/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*****************************************************************************
 枚举名    : ADS_FLOW_DEBUG_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : ADS流量上报Debug开关
*****************************************************************************/
enum ADS_FLOW_DEBUG_ENUM
{
    ADS_FLOW_DEBUG_OFF                  = 0,
    ADS_FLOW_DEBUG_DL_ON                = 1,
    ADS_FLOW_DEBUG_UL_ON                = 2,
    ADS_FLOW_DEBUG_ALL_ON               = 3,
    ADS_FLOW_DEBUG_BUTT
};

/*****************************************************************************
 枚举名称  : ADS_LOG_LEVEL_ENUM
 枚举说明  : ADS打印LOG等级
*****************************************************************************/
enum ADS_LOG_LEVEL_ENUM
{
    ADS_LOG_LEVEL_DEBUG                 = 0,                /* 0x0: debug-level                      */
    ADS_LOG_LEVEL_INFO,                                     /* 0x1: informational                    */
    ADS_LOG_LEVEL_NOTICE,                                   /* 0x2: normal but significant condition */
    ADS_LOG_LEVEL_WARNING,                                  /* 0x3: warning conditions               */
    ADS_LOG_LEVEL_ERROR,                                    /* 0x4: error conditions                 */
    ADS_LOG_LEVEL_FATAL,                                    /* 0x5: fatal error condition            */

    ADS_LOG_LEVEL_BUTT
};
typedef VOS_UINT32 ADS_LOG_LEVEL_ENUM_UINT32;


/*****************************************************************************
  4 消息头定义
*****************************************************************************/


/*****************************************************************************
  5 消息定义
*****************************************************************************/


/*****************************************************************************
  6 STRUCT定义
*****************************************************************************/


typedef struct
{
    /* 上行IPF事件统计 */
    VOS_UINT32                          ulULQueNonEmptyTrigEvent;
    VOS_UINT32                          ulULQueFullTrigEvent;
    VOS_UINT32                          ulULQueHitThresTrigEvent;
    VOS_UINT32                          ulULTmrHitThresTrigEvent;
    VOS_UINT32                          ulUL10MsTmrTrigEvent;
    VOS_UINT32                          ulULSpeIntTrigEvent;
    VOS_UINT32                          ulULProcEventNum;

    /* 上行数据统计 */
    VOS_UINT32                          ulULRmnetRxPktNum;
    VOS_UINT32                          ulULRmnetModemIdErrNum;
    VOS_UINT32                          ulULRmnetRabIdErrNum;
    VOS_UINT32                          ulULRmnetEnQueSuccNum;
    VOS_UINT32                          ulULRmnetEnQueFailNum;
    VOS_UINT32                          ulULPktEnQueSuccNum;
    VOS_UINT32                          ulULPktEnQueFailNum;

    /* 上行BD统计 */
    VOS_UINT32                          ulULBdqCfgIpfHaveNoBd;
    VOS_UINT32                          ulULBdqCfgBdSuccNum;
    VOS_UINT32                          ulULBdqCfgBdFailNum;
    VOS_UINT32                          ulULBdqCfgIpfSuccNum;
    VOS_UINT32                          ulULBdqCfgIpfFailNum;
    VOS_UINT32                          ulULBdqSaveSrcMemNum;
    VOS_UINT32                          ulULBdqFreeSrcMemNum;
    VOS_UINT32                          ulULBdqFreeSrcMemErr;

    /* 上行组包统计 */
    VOS_UINT32                          ulULWmLevel1HitNum;
    VOS_UINT32                          ulULWmLevel2HitNum;
    VOS_UINT32                          ulULWmLevel3HitNum;
    VOS_UINT32                          ulULWmLevel4HitNum;

    /* 上行流量统计 */
    VOS_UINT32                          ulULFlowDebugFlag;
    VOS_UINT32                          ulULFlowRptThreshold;
    VOS_UINT32                          ulULFlowInfo;
    VOS_UINT32                          ulULStartSlice;
    VOS_UINT32                          ulULEndSlice;

}ADS_UL_COM_STATS_INFO_STRU;


typedef struct
{
    /* 下行IPF事件统计 */
    VOS_UINT32                          ulDLRcvIpfRdIntNum;
    VOS_UINT32                          ulDLCCoreResetTrigEvent;
    VOS_UINT32                          ulDLProcIpfRdEventNum;
    VOS_UINT32                          ulDLRcvIpfAdqEmptyIntNum;
    VOS_UINT32                          ulDLRecycleMemTrigEvent;
    VOS_UINT32                          ulDLProcIpfAdEventNum;

    /* 下行RD统计 */
    VOS_UINT32                          ulDLRdqRxRdNum;
    VOS_UINT32                          ulDLRdqGetRd0Num;
    VOS_UINT32                          ulDLRdqTransMemFailNum;
    VOS_UINT32                          ulDLRdqRxNormPktNum;
    VOS_UINT32                          ulDLRdqRxNdPktNum;
    VOS_UINT32                          ulDLRdqRxDhcpPktNum;
    VOS_UINT32                          ulDLRdqRxErrPktNum;
    VOS_UINT32                          ulDLRdqFilterErrNum;

    /* 下行数据统计 */
    VOS_UINT32                          ulDLRmnetTxPktNum;
    VOS_UINT32                          ulDLRmnetModemIdErrNum;
    VOS_UINT32                          ulDLRmnetRabIdErrNum;
    VOS_UINT32                          ulDLRmnetNoFuncFreePktNum;

    /* 下行AD统计 */
    VOS_UINT32                          ulDLAdqAllocSysMemSuccNum;
    VOS_UINT32                          ulDLAdqAllocSysMemFailNum;
    VOS_UINT32                          ulDLAdqAllocMemSuccNum;
    VOS_UINT32                          ulDLAdqAllocMemFailNum;
    VOS_UINT32                          ulDLAdqFreeMemNum;
    VOS_UINT32                          ulDLAdqRecycleMemSuccNum;
    VOS_UINT32                          ulDLAdqRecycleMemFailNum;
    VOS_UINT32                          ulDLAdqGetFreeAdSuccNum;
    VOS_UINT32                          ulDLAdqGetFreeAdFailNum;
    VOS_UINT32                          ulDLAdqCfgAdNum;
    VOS_UINT32                          ulDLAdqCfgAd0Num;
    VOS_UINT32                          ulDLAdqCfgAd1Num;
    VOS_UINT32                          ulDLAdqCfgIpfSuccNum;
    VOS_UINT32                          ulDLAdqCfgIpfFailNum;
    VOS_UINT32                          ulDLAdqStartEmptyTmrNum;
    VOS_UINT32                          ulDLAdqEmptyTmrTimeoutNum;
    VOS_UINT32                          ulDLAdqRcvAd0EmptyIntNum;
    VOS_UINT32                          ulDLAdqRcvAd1EmptyIntNum;

    /* 下行流控统计 */
    VOS_UINT32                          ulDLFcActivateNum;
    VOS_UINT32                          ulDLFcTmrTimeoutNum;

    /* 下行流量统计 */
    VOS_UINT32                          ulDLFlowDebugFlag;
    VOS_UINT32                          ulDLFlowRptThreshold;
    VOS_UINT32                          ulDLFlowInfo;
    VOS_UINT32                          ulDLStartSlice;
    VOS_UINT32                          ulDLEndSlice;

}ADS_DL_COM_STATS_INFO_STRU;


typedef struct
{
    /* 上行复位统计 */
    VOS_UINT32                          ulULResetCreateSemFailNum;
    VOS_UINT32                          ulULResetLockFailNum;
    VOS_UINT32                          ulULResetSuccNum;

    /* 下行复位统计 */
    VOS_UINT32                          ulDLResetCreateSemFailNum;
    VOS_UINT32                          ulDLResetLockFailNum;
    VOS_UINT32                          ulDLResetSuccNum;
} ADS_RESET_STATS_INFO_STRU;


typedef struct
{
    ADS_UL_COM_STATS_INFO_STRU          stUlComStatsInfo;
    ADS_DL_COM_STATS_INFO_STRU          stDlComStatsInfo;
    ADS_RESET_STATS_INFO_STRU           stResetStatsInfo;

} ADS_STATS_INFO_STRU;


/*****************************************************************************
  8 全局变量声明
*****************************************************************************/

extern ADS_STATS_INFO_STRU              g_stAdsStats;
extern VOS_UINT32                       g_ulAdsDlDiscardPktFlag;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

VOS_VOID ADS_ResetDebugInfo(VOS_VOID);
VOS_VOID ADS_DLFlowAdd(VOS_UINT32 ulSduLen);
VOS_VOID ADS_ULFlowAdd(VOS_UINT32 ulSduLen);
VOS_VOID ADS_SetFlowDebugFlag(VOS_UINT32  ulFlowDebugFlag);
VOS_VOID ADS_SetFlowDLRptThreshold(VOS_UINT32  ulFlowDLRptThreshold);
VOS_VOID ADS_SetFlowULRptThreshold(VOS_UINT32  ulFlowULRptThreshold);
VOS_VOID ADS_LogPrintf(ADS_LOG_LEVEL_ENUM_UINT32 enLevel, VOS_CHAR *pcFmt, ...);
VOS_VOID ADS_SetDlDiscardPktFlag(VOS_UINT32 ulValue);


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

#endif /* end of AdsDebug.h */
