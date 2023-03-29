/*
* Copyright (C) Huawei Technologies Co., Ltd. 2012-2018. All rights reserved.
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

#ifndef __LPSNVINTERFACE_H__
#define __LPSNVINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include  "vos.h"
#include  "PsTypeDef.h"
#include  "LNvCommon.h"
#include  "RrcNvInterface.h"
#include  "LNasNvInterface.h"
#include  "msp_nvim.h"
#if (VOS_OS_VER != VOS_WIN32)
#include  "msp.h"
#endif
#pragma pack(4)

/*****************************************************************************
  2 macro
*****************************************************************************/
/* NV操作函数 */
/*
ERR_MSP_NO_INITILIZATION   3              模块初始化失败，无法执行接口操作
ERR_MSP_NO_INITILIZATION   502            参数错误
ERR_MSP_NV_NOT_SUPPORT_ID  520            该ID不存在
ERR_MSP_NV_ERROR_READ      521            读flash失败
ERR_MSP_NV_ITEM_IS_EMPTY   1100           该项数据长度为零
*/
#if (VOS_OS_VER != VOS_WIN32)
#define LPs_NvimItem_Read( ulId, pItemData, usItemDataLen ) \
        NVM_Read( ulId, (VOS_VOID *)pItemData, usItemDataLen)

/*
ERR_MSP_NO_INITILIZATION             3              模块初始化失败，无法执行接口操作
ERR_MSP_NO_INITILIZATION             502            参数错误
ERR_MSP_NV_NOT_SUPPORT_ID            520            该ID不存在（按照要求该接口中不支持添加ID的操作）
ERR_MSP_NV_NOT_SUPPORT_LENTH         522            长度不支持
ERR_MSP_NV_ERROR_WRITE               524            写flash失败
ERR_MSP_NV_FLASH_FULL                1103           flash空间被写满
ERR_MSP_NVIM_NOT_SUPPORT_WRITE       2007           系统模式不允许修改该项
*/
#define LPs_NvimItem_Write( ulId, pItemData, usItemDataLen)\
       NVM_Write( ulId, (VOS_VOID *)pItemData, usItemDataLen)

#define LPs_NvimItem_ReadEx(ulModmeId, ulId, pItemData, usItemDataLen ) \
        NV_ReadEx(ulModmeId, ulId, (VOS_VOID *)pItemData, usItemDataLen)
#define LPs_NvimItem_WriteEx(ulModmeId, ulId, pItemData, usItemDataLen)\
       NV_WriteEx(ulModmeId, ulId, (VOS_VOID *)pItemData, usItemDataLen)
#else
#define LPs_NvimItem_Read( ulId, pItemData, usItemDataLen ) \
        NVM_Read_Stub( ulId, (VOS_VOID *)pItemData, usItemDataLen)

#define LPs_NvimItem_Write( ulId, pItemData, usItemDataLen)\
       NVM_Write_Stub( ulId, (VOS_VOID *)pItemData, usItemDataLen)

#define LPs_NvimItem_ReadEx(ulModmeId, ulId, pItemData, usItemDataLen ) \
        NVM_Read_Stub(ulId, (VOS_VOID *)pItemData, usItemDataLen)
#define LPs_NvimItem_WriteEx(ulModmeId, ulId, pItemData, usItemDataLen)\
       NVM_Write_Stub(ulId, (VOS_VOID *)pItemData, usItemDataLen)
#endif

#define LPs_FactNvim_Write(ulId, pItemData, usDataLen)\
        NVIM_WriteFactoryForce(ulId, (VOS_VOID *)pItemData, usDataLen)

/* GCF开关组,位域功能 */
#define LPS_NV_GET_TAU_CIPHERED_BIT         ( 0x00000001 )
#define LPS_NV_LOCK_BAND_FREQ_CELL_IN_TEST_MODE_FUNC_BIT     (0x00000001)

/*  TdsDataInfoNotify Begin*/
#define LPS_NV_DL_CTRL_PDU_NOTIFY_BIT          (0x00000002)
/*  TdsDataInfoNotify End*/

/*added 20130320 begin*/
#define LPS_NV_SEARCHING_OPTIMIZE_FUN_BIT    (0x00000004)
/*added 20130320 end*/

#define LPS_NV_GET_MSG4_OLDCR_BIT           ( 0x00000002 )

/* GCF是否支持24.301 Table D.1.1中建链原因设置，位域开关 */
#define LPS_NV_GET_EST_CAUSE_BIT             ( 0x00000004 )

/* 指示安全功能是否支持测试模式,位域功能 */
#define LPS_NV_GET_SUPPORT_SMC_TEST_MODE_BIT  ( 0x00000008 )

/* GCF打桩通过CDRX相关用例，实际场景为了降功耗不打开该开关，位域开关 */
#define LPS_NV_GET_CDRX_L2_TIMER_BIT         ( 0x00000010 )

/* PS功能开关组,位域功能 */
#define LPS_NV_GET_SUPPORT_BAND64_BIT       ( 0x00000001 )

/* PS功能开关组,删除SDF是否携带Qos 位域 */
/* 删除SDF时是否携带Qos信息功能开关，0为关闭，1为打开。默认为关闭 */
#define LPS_NV_GET_DEL_SDF_WITH_QOS_BIT       ( 0x00000002 )

/* 是否进行连接态收到本ue的业务寻呼后进行释放, 缺省0 表示进行释放,  4表示不进行释放 */
#define LPS_NV_GET_CONN_RCV_PAGING_TO_RELASE_BIT       ( 0x00000004 )


/* PS功能开关组,是否支持软件KDF算法位域 */
#define LPS_NV_GET_HARD_KDF_BIT         ( 0x00000010 )
#define LPS_NV_GET_HARD_KDF_HYBRI_BIT   ( 0x00000020 )
#define LPS_NV_GET_ULDRB_DISCARD_BIT    ( 0x00000040 )

/* 信令面时延优化开关， 0x00000080表示打开，0表示关闭 */
#define PS_NV_TIME_DELAY_SWITCH_BIT     ( 0x00000080 )

/* 信令面时延优化开关， 0x00000100表示打开，0表示关闭 */
#define PS_NV_TIME_DELAY_HOOK_SWITCH_BIT     ( 0x00000100 )

/* MBMS　开关 */
#define LPS_NV_GET_MBMS_SUPPORT_BIT         ( 0x00000200 )

/* 软银Bandwidth需求开关 */
#define LPS_NV_GET_BANDWIDTH_SUPPORT_BIT         ( 0x00000400 )

/* modify freq all begin */
#define LPS_NV_GET_FREQ_IN_ALL_BANDWITH_SUPPORT_BIT ( 0x00000800 )
/* modify freq all end */
/* 重建开关 */
#define LPS_NV_GET_REST_STUB_SUPPORT_BIT         ( 0x00001000 )

/* DCM定制需求 GU到L的切换不判断被禁TA开关 */
#define LPS_NV_GET_GU2L_HO_IGNORE_FORBID_TA_BIT     ( 0x00002000 )

/* -ETWS-Begin */
#define LPS_NV_ETWS_SUPPORT_BIT         ( 0x00004000 )
#define LPS_NV_CMAS_SUPPORT_BIT         ( 0x00008000 )
/* -ETWS-End */
/* -SYSCFG-SAVENV-Begin */
#define LPS_NV_GET_JP_UBand1_Record_SUPPORT_BIT         ( 0x00010000 )
/* -SYSCFG-SAVENV-End */

/* L重建UTRA-FDD搜网开关 */
#define LPS_NV_GET_L_REEST_CELL_SRCH_UTRA_FDD_BIT         ( 0x00020000 )

/* L重建GSM搜网开关 */
#define LPS_NV_GET_L_REEST_CELL_SRCH_GSM_BIT         ( 0x00040000 )

/*add for cell bar 2012-5-29 begin*/
/* 小区搜索到小区，但是无法接收或者部分接收到系统信息时,用于判断否需要bar小区 */
#define LPS_NV_GET_CELL_SRCH_CELL_BAR_BIT         ( 0x00080000 )
/*add for cell bar 2012-5-29 end*/

/* -nccpermit */
#define LPS_NV_NCC_PERMIT_BIT         ( 0x00100000 )

/* bgs begin */
/* L背景搜功能是否支持开关 */
#define LPS_NV_GET_L_BGS_SUPPORT_BIT                 ( 0x00200000 )
/* bgs end */

/* tcp并发*/
#define LPS_NV_TCP_OPT_BIT            ( 0x00800000 )


/* -reest-measGap*/
#define LPS_NV_GET_REEST_MEAS_GAP_SETUP_BIT            ( 0x00400000 )

/* -SoftBank-定制标志:true,set Band41 range */
#define LPS_NV_SOFT_BANK_FLAG_BIT            ( 0x01000000 )

/* -nccpermit-2 */
#define LPS_NV_NO_IDLE_NCC_PERMIT_BIT        ( 0x02000000 )

/* -dcom-resel-cfg */
#define LPS_NV_JP_DCOM_USE_R8_BIT        ( 0x04000000 )

/* -dcom-resel-cfg */
#define LPS_NV_JP_DCOM_CON_TO_IDLE_BIT        ( 0x08000000 )

/* -dcom-resel-cfg */
#define LPS_NV_JP_DCOM_REL_OFFSET_BIT        ( 0x10000000 )
#define LPS_NV_JP_DCOM_REL_OFFSET_DEBUG_BIT  ( 0x20000000 )
/* -fast-dorm-cfg */
#define LPS_NV_JP_DCOM_FAST_DORM_BIT  ( 0x40000000 )

/* add for RRM 5.2.1 test begin */
#define LPS_NV_RRM_LTE_521_BIT  ( 0x80000000 )
/* add for RRM 5.2.1 test end */

/* -CMAS-Begin */
#define LPS_NV_CMAS_REPT_BEFORE_CAMP_SUCC_BIT    (0x00000008 )
/* -CMAS-End */

/* v7r2 ca begin */
/* CA特性是否打开，1为打开，0为关闭 */
#define LPS_NV_CA_SUPPORT_BIT  ( 0x00000001 )
/* v7r2 ca end */

/* add for LTE NetScan begin */
/* stPsFunFlag02，数组中为1 */
/* NetScan特性是否打开，1为打开，0为关闭 */
#define LPS_NV_NETSCAN_SUPPORT_BIT    (0x00000002)
/* add for LTE NetScan end */

/* add for MFBI begin */
#define LPS_NV_LRRC_MFBI_CAMPON_WIDER_BAND_FLAG_BIT    (0x00000004)
/* add for MFBI end */

#define LPS_NV_LRRC_SUPPORT_EIA0_BIT (0x00000010)

/* lwurc 频段限制指示开关 */
#define LPS_NV_LWURC_BAND_LIMIT_FLG_BIT            (0x00000020)

/* 启动背景搜，但是只搜索L单模，搜索完毕后回复LMM搜索结果，不再搜索 GU 模 */
#define LPS_NV_GET_L_BGS_L_SUPPORT_BIT            (0x00000040)

#define LPS_NV_SND_ETWS_IMIDIEATELY_BIT           (0x00000080)                  /* 0x80表示为立即发送；0x00表示收齐后再发送 */

/*added for L2 OM report 2014-03-11*/
#define LPS_NV_L2_TLEVENT_REPORT_BIT           (0x00000100)                  /* TL L2事件上报*/
#define LPS_NV_RRC_LIST_SEARCH_TWICE_BIT           (0x00000400)                  /* LTE 控制LIST搜网先验频点搜索两次开关*/
#define LPS_NV_RRC_BACKTOLTE_REDRECTFAIL_BIT        (0x00000800)

/* PS功能开关组,是否支持安全激活场景下处理明文拒绝NAS消息标识 */
#define LPS_NV_GET_NAS_PLAIN_REJ_MSG_FLAG_BIT  (0x00001000)

/*begin:added for CA Cell Info Report 2014/06/10*/
#define LPS_NV_L2_CACELLINFO_REPORT_BIT          (0x00002000)                  /*CA SCELL状态信息上报*/
/*end:added for CA Cell Info Report 2014/06/10*/

/* add for CSFB return to L TAU type2 2014-07-24 begin */
/* PS功能开关组,规避兰州CSFB中兴核心网BUG的NV开关标识 */
#define LPS_NV_GET_NAS_CSFB_TAU_TYPE2_FLAG_BIT  (0x00004000)
/* add for CSFB return to L TAU type2 2014-07-24 end */

/* 2015-06-04 begin */
#define LPS_NV_GET_INTRA_HO_IGNORE_FORBID_TA_BIT    (0x00008000)                /* 本系统切换不判断被禁TA开关 */
/* 2015-06-04 end */

/* 2015-06-04 begin */
#define LPS_NV_GET_HO_RETRAN_ESR_MSG_FLAG_BIT     (0x00010000)                  /* ESR过程中发生切换,未发起TAU流程,重发ESR消息控制开关 */
/* 2015-06-04 end */
/* begin: add for csfb optimize */
#define LPS_NV_GET_CSFB_OPTIMIZE_FLAG_BIT           (0x00020000)                /*csfb场景收不到网侧release场景优化开关*/
/* end: add for csfb optimize */

/* begin:add for CHR psfunflag02 bitflag19 */
#define LPS_NV_LRRC_CHR_FEATURE_ON_FLAG_BIT    (0x00040000)
/* end:add for CHR */

/* begin: psfunflag02 bitflag20 */
#define LPS_NV_LRRC_DYNAMIC_UE_CAP                (0x00080000)
/* end */

/* begin: add for ignore CA recfg, 2017-1-13 */
/* UE不支持CA时,如果仪表强配CA,UE是否回复RECFG_CMP开关; psfunflag02 bitflag21; 0:不回复CMP; 1:回复CMP */
#define LPS_NV_LRRC_REPLY_RECFG_CMP_FLAG          (0x00100000)
/* end: add for ignore CA recfg, 2017-1-13 */

/* begin: add for cmas taiwan, 2016-12-27 */
#define LPS_NV_LRRC_CMAS_TEMPORARY_PROJECT_FLAG          (0x00200000)
/* end: add for cmas taiwan, 2016-12-27 */

/*add 低功耗模块增加 ADRX SSC NV开关 stPsFunFlag03 第0位 第1位*/
#define LPS_NV_GET_SSC_SUPPORT_BIT                (0x00000001)
#define LPS_NV_GET_ADRX_SUPPORT_BIT               (0x00000002)
/* rrc release modify begin */
#define LPS_NV_GET_NOT_TRY_FAKE_CELL_FLAG_BIT     (0x00000004)
/* rrc release modify end */
#define LPS_NV_GET_DEL_MOBILE_SUPPORT_BIT         (0x00000008)

/* add for P-TMSI set active flag begin */
#define LPS_NV_GET_PTMSI_TAU_ACTIVE_FLAG_SUPPORT_BIT (0x00000020)
/* add for P-TMSI set active flag end */

#define LPS_NV_GET_SBM_MFBI_CONN_MEAS_FLAG_SUPPORT_BIT (0x00000040)

/* begin: add for fake conn mdt location info for gcf test */
#define LPS_NV_GET_FAKE_MDT_LOCINFO_FLAG_BIT    (0x00000080)
/* end: add for fake conn mdt location info for gcf test */

#define LPS_NV_GET_CMAS_RCV_CMP_FLAG_BIT    (0x00000100)

#define LPS_NV_GU2L_NO_BAR_BIT    (0x00000200)

/* Begin: add DCM定制,切换过程中HO_CNF响应失败,等到T304超时后出发重建 */
/* stPsFunFlag03 bitFlag12_11 */
#define LPS_NV_DCM_HO_FAIL_WAIT_T304_REEST_BIT    (0x00000800)
/* End: add DCM定制,切换过程中HO_CNF响应失败,等到T304超时后出发重建 */


/*TDS DSP 自检地址*/
#define ADDR_SELF_ADJUST_PARA       (0x1300F840)

/* for 美国项目仪器测试 NV控制*/
#define LPS_NV_GET_PDCP_CNTCHK_BIT    (0x00000010)


/* preamble modify begin */
#define LPS_NV_MAC_HIGHSPEED_CELL_RA_OPTIMIZE_BIT   (0x00000020)
#define LPS_NV_MAC_RA_PBL_LESS2_OPTIMIZE_BIT   (0x00000040)
/* preamble modify end */


/* GCF开关组 */
#define  LPS_NV_GET_GCF_FLAG(ucIndex, ulFlag) \
         (g_aulGcfFlg[ucIndex]&(ulFlag))

/* PS功能开关组 */
#define  LPS_NV_GET_PS_FUN_FLAG(ucIndex, ulFlag) \
         (g_aulPsFunFlg[ucIndex]&(ulFlag))

#define  LPS_NV_SUPPORT_BAND64                1     /* 支持Band64频段 */
#define  LPS_NV_NOT_SUPPORT_BAND64            0     /* 不支持Band64频段 */
#define LPS_NV_CONN_RCV_PAGING_RELASE         0     /* 收到业务寻呼进行释放 */
#define LPS_NV_CONN_RCV_PAGING_NO_RELASE  LPS_NV_GET_CONN_RCV_PAGING_TO_RELASE_BIT   /* 收到业务寻呼不进行释放 */

#define LPS_NV_GET_HARD_KDF_OFF        0                        /* 不支持硬件KDF算法 */
#define LPS_NV_GET_HARD_KDF_ON         LPS_NV_GET_HARD_KDF_BIT  /* 支持硬件KDF算法 */


#define LPS_NV_GET_HARD_KDF_HYBRI_OFF        0                              /* 不使用两次混合计算的KDF算法 */
#define LPS_NV_GET_HARD_KDF_HYBRI_ON         LPS_NV_GET_HARD_KDF_HYBRI_BIT  /* 使用两次混合计算的KDF算法 */


/* PS功能开关宏定义 */
#define LPS_FUNC_SW_ON                       (1)
#define LPS_FUNC_SW_OFF                      (0)

/*打印全局变量的最大个数*/
#define MAX_GLOBAL_PRANT_NUM   5

/* TPS 开关组 */
#define  TPS_NV_GET_FLAG(uVlaue, ulFlag) \
         (uVlaue&(ulFlag))
#define TPS_NV_OP_HL1_SCHEDULE_IND_FLITER_BIT            ( 0x00000001 )
#define TPS_NV_OP_T_RRC_trafficMeasPeriod_FLITER_BIT            ( 0x00000002 )
#define TPS_NV_Print_FLITER_BIT            ( 0x00000004 )
/*CELLUPDATE MODEFIED BEGIN*/
#define TPS_NV_OP_Cellupdate_stub_BIT            ( 0x00000001 )
/*CELLUPDATE MODEFIED END*/
/*begin ADRX function  add 20140825*/
#define TPS_NV_OP_ADRX_GET_BIT            ( 0x00000100 )
/* 强制ADRX */
#define TPS_NV_OP_FORCE_ADRX_GET_BIT      ( 0x00001000 )

/* Cell-Reselection optimazation in CELL-FACH when Srx/SNR are lower than the thresholds*/
#define TPS_NV_CELL_RESEL_FOR_CELLFACH_GET_BIT      ( 0x00002000 )

/* RRC Connection Request failure with V300>N300 */
#define TPS_NV_RRCCONNECTIONREQ_FAILUREOPT_BIT      ( 0x00200000 )

/*end ADRX function add 20140825*/

/* DMO begin*/
/*bitFlag10*/
#define  TPS_NV_OP_DMO_Message_BIT           ( 0x00000200)
/* DMO end*/

#define TPS_NV_SMC_STUB_BIT                 ( 0x00000400 )

#define TPS_NV_CS_TL_MEAS_EVA_CLOSE_BIT                 ( 0x00000800 )

#define TPS_NV_OP_NEWKEY_GET_BIT            ( 0x00000008 )
/*begin:add for tds cell search optimize */
#define TPS_NV_CELL_SEARCH_OPT_BIT                 ( 0x00010000 )
#define TPS_NV_CELL_SEARCH_PRO_BIT                 ( 0x00020000 )
/*end:add for tds cell search optimize */
/* add for L2Tredir cell search optimize begin*/
#define TPS_NV_L2T_REDIR_OPT_BIT                 ( 0x00004000 )
/* add for L2Tredir cell search optimize end*/
/*begin css band scan sweep optimize */
#define TPS_NV_CSS_BAND_SCAN_OPT_BIT              ( 0x00040000 )
/*end css band scan sweep optimize */

#define TPS_NV_OP_TCP_ACK_DIS_GET_BIT       ( 0x00000010 )


/*DETECT CELL MODIFIY BEGIN*/
#define TPS_NV_OP_DETECT_CELL_GET_BIT       (0x00000020)
/*DETECT CELL MODIFIY END*/

#define TPS_NV_OP_MEASCONTROL_GET_BIT       (0x00000040)

#define TPS_NV_OP_RlcUmDataInd_GET_BIT       (0x00000080)


/* 软调信息添加 BEGIN*/
#define TPS_NV_OP_Debug_Info_BIT            ( 0x00000002 )

#define TPS_NV_OP_Debug_State_BIT            ( 0x00000004 )
/* 软调信息添加 END*/


#define TPS_NV_GCF_CS_LOOP_BIT            ( 0x00000001 )

#define TPS_NV_GCF_DRXEXTRA_CONTROL_BIT            ( 0x00000002 )

/*准FR特性begin*/
#define TPS_NV_OP_GSM_FR_BIT           ( 0x00000004 )
/*准FR特性end*/
/*TG 重选优化begin*/
#define TPS_NV_GCF_SIRAT_BIT           ( 0x00000008 )
#define TPS_NV_GCF_QRXLEVMIN_BIT        (0x00000010)
#define TPS_NV_GCF_S_UNSATISFIED_BIT    (0x00000020)
#define TPS_NV_T2G_IRAT_REPORT_BIT      (0x00000040)
/*TG 重选优化end*/

#define TPS_NV_GCF_RRM_STUB_BIT         (0x00000080)

/* add for AutoFR Enhance   Begin */
/* TL FR特性开关 */
#define TPS_NV_FR_FEATURE_BIT           (0x00080000)
/* PS信令连接存在时FR开关 */
#define TPS_NV_PS_SIG_CONN_EXIST_FR_BIT (0x00100000)
/* add for AutoFR Enhance   End */

/* TG 3A Estimate Optimization  2016.1.28 Begin */
#define TPS_NV_TG_3A_ESTIMATE_OPTIMIZATION_BIT (0x00800000)
/* TG 3A Estimate Optimization  2016.1.28 End */
/* add for modify debug meas info begin*/
/*控制IDL态下DBG消息是否打开NV*/
#define TPS_NV_DBG_IDL_MEAS_INFO_BIT           (0x01000000 )
/*控制连接态下DBG消息是否打开NV*/
#define TPS_NV_DBG_CONN_MEAS_INFO_BIT           (0x02000000 )
/* add for modify debug meas info end*/

/* Ac All Barred Optimize Begin */
#define TPS_NV_AC_ALL_BARRED_OPTIMIZE_BIT      (0x04000000 )
/* Ac All Barred Optimize End */

/* LTE PS Transfer begin */
#define LRRC_MTC_THRESHOLD_MIN_VALUE      (0)
#define LRRC_MTC_THRESHOLD_MAX_VALUE      (31 * 2 * 8)
/* LTE PS Transfer end */
/* taiwan modify begin */
#define LRRC_IRAT_REDIR_NOT_BANDSCAN_BIT (0x00000008)
/* taiwan modify end */

/* 测试网侧发送Paging情况,不处理本UE Paging  begin*/
#define LRRC_IDLE_DISCARD_PAGING_BIT     (0x00000001)
/* 测试网侧发送Paging情况,不处理本UE Paging  end*/

/* high rat 搜网，中国按预置频段搜*/
#define LRRC_NV_HIGHRAT_SEARCH_BIT     (0x00000004)
/* high rat 搜网，中国按预置频段搜*/

#define TPS_NV_OP_OCCUPY_GET_BIT            ( 0x00000001 )
#define TPS_NV_OP_SIGNAL_REL_GET_BIT        ( 0x00000002 )

/* 测试网侧发送Paging情况,不处理本UE Paging  begin*/
#define TPS_NV_OP_DISCARD_PAGING_GET_BIT    ( 0x00000004 )
/* 测试网侧发送Paging情况,不处理本UE Paging  end*/

/* 上报cgi-info后相关MeasObj的cellForWhichToReportCGI是否需要保留开关   begin */
#define LRRC_NV_ENDELCELLFORCGIFlG_BIT    ( 0x00040000 )
/* 上报cgi-info后相关MeasObj的cellForWhichToReportCGI是否需要保留开关   end */

#define LRRC_MAX_NUM_IDC_SUBFRAME_PATTERNLIST           8
#define LRRC_MAX_NUM_SUBFRAME_CONFIG0_R11               9
#define LRRC_MAX_NUM_SUBFRAME_CONFIG6_R11               8

#define LRRC_ACCFAIL_MAX_BAR_STEP                       5


#define LRRC_SINGLE_CAND_PLMN_MAX_BAND                  8
#define LRRC_SINGLE_CAND_PLMN_MAX_FRQNUM               16
#define LRRC_CAND_PLMNFREQ_MAX_PLMNNUM                 20
#define LRRC_LATEST_MAX_CAND_FREQ_NUM                  12

/*某个PLMN先验总个数，包括测量配置中*/
#define LRRC_MAX_PLMN_CANDARFCN_NUM                    48

#define LUP_MEM_MAX_POOL_NUM                (5)
#define LUP_MEM_MAX_CLUSTER_NUM             (8)

/* begin: add for vowifi quality rpt */
#define LRRC_IMSA_THRES_WLAN_LOW_P             (-105)
#define LRRC_IMSA_THRES_WLAN_LOW_Q             (-20)
#define LRRC_IMSA_THRES_WLAN_HIGH_P            (-80)
#define LRRC_IMSA_THRES_WLAN_HIGH_Q            (-5)
#define LRRC_IMSA_TIME_STEERING_WLAN           (3000)
#define LRRC_IMSA_THRES_WLAN_DELTA_P           (5)
#define LRRC_IMSA_THRES_WLAN_INTERVAL_RPT_TIME           (10000)
/* end: add for vowifi quality rpt */
#define LRRC_CSG_HISTORY_CAMPED_CELL_NUM               50
#define LRRC_CSG_NEIGH_MACRO_CELL_NUM                  8
#define LRRC_CSG_HISTORY_CAMPED_CELL_PART_NUM          25

#define LRRC_SINGLE_DL_BAND_GROUP_NUM       (8)
#define LRRC_SINGLE_DL_BAND_CTRL_BIT             (0x00000001)

#define LRRC_RESTRICTED_BAND_ARRAY_NUM      (8)
#define LRRC_MAX_NUM_IDC_UL_CA_PARALIST      8

/* 仪器测试时，搜网搜到非仪器的PLMN不上报，begin */
#define LRRC_TEST_PLMN_MAX_NUM               (32)
/* 仪器测试时，搜网搜到非仪器的PLMN不上报，end */

/*begin,add for plmn forbidden band,  */
#define LRRC_FORBIDDEN_BAND_PLMN_MAX_NUM           (16)
#define LRRC_SINGLE_PLMN_FORBIDDEN_BAND_MAX_NUM    (8)
/*end,add for plmn forbidden band,  */

/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
*****************************************************************************/
/*开关枚举*/
enum RRC_NV_SWITCH_ENUM
{
    RRC_SWITCH_OFF = 0,
    RRC_SWITCH_ON  = 1
};

typedef VOS_UINT32   RRC_NV_SWITCH_ENUM_UINT32;

/* sib定时器选择开关枚举*/
enum RRC_NV_TIMER_CHOICE_ENUM
{
    RRC_TIMER_CHOICE_DEFAULT = 0,           /* 默认选择 */
    RRC_TIMER_CHOICE_DT  = 1,               /* 德电需求选择 */
    RRC_TIMER_CHOICE_BUTT
};
typedef VOS_UINT32   RRC_NV_TIMER_CHOICE_ENUM_UINT32;

enum RRC_PHY_LTE_RF_HW_ENUM
{
    RRC_PHY_LTE_RF_HW_BAND7_38_40         = 0,  /* Band7/38/40多频带扣板*/
    RRC_PHY_LTE_RF_HW_BAND20,                   /* Band20单频带扣板*/
    RRC_PHY_LTE_RF_HW_USB_STICK_VB,             /* USB STICK的vb硬件单板 */
    RRC_PHY_LTE_RF_HW_SOFTBANK_BAND64,          /* 软银使用的Band64扣板 */
    RRC_PHY_LTE_RF_HW_USB_STICK_VC,             /* USB STICK的vC硬件单板 */
    RRC_PHY_LTE_RF_HW_USB_STICK_VD,             /* USB STICK的vD硬件单板 */
    RRC_PHY_LTE_RF_HW_USB_STICK_VE,             /* USB STICK的vE硬件单板 */
    RRC_PHY_LTE_RF_HW_USB_STICK_VF,             /* USB STICK的vF硬件单板 */
    RRC_PHY_LTE_RF_HW_USB_STICK_VG,             /* USB STICK的vG硬件单板 */
    RRC_PHY_LTE_RF_HW_USB_STICK_VH,             /* USB STICK的vH硬件单板 */
    RRC_PHY_LTE_RF_HW_BUTT
};
typedef VOS_UINT8 RRC_PHY_LTE_RF_HW_ENUM_UINT8;
/*NV项枚举*/
/* -PLMN_EXACTLY_COMPARE_FLG */
#ifdef PS_ITT_PC_TEST2
#define en_NV_Item_PLMN_EXACTLY_COMPARE_FLG   (9106)
#endif

/*****************************************************************************
 结构名    : LPS_NV_FLOW_CONTROL
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈流控结构体
*****************************************************************************/
typedef struct
{
    /* 流控总开关 */
    VOS_UINT32                          ulFlowCtrlFlag;
    /*ulFcInitFlag流控初始化选择 0-由RRC动态选择，1-代表由NV项进行控制*/
    VOS_UINT32                          ulFcInitFlag;
    VOS_UINT32                          ulFcPdcpTarget;
    VOS_UINT32                          ulTargetIPMax;
    VOS_UINT32                          ulSendSduMax;
    VOS_UINT32                          ulDlSduMax;
    VOS_UINT32                          ulULIpMax;
    VOS_UINT32                          ulMeasGap;
    VOS_UINT32                          ulSduGap;
}LPS_NV_FLOW_CONTROL;

/*****************************************************************************
 结构名    : LPS_NV_GLOBAL_PRINT
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈打印全局变量结构体
*****************************************************************************/
typedef struct
{
    /* 打印总开关 0:关闭打印全局变量，1: 打印一个，2:打印两个，依次 3 4 5 */
    VOS_UINT32                          ulGlobalPrintFlag;
    VOS_UINT32                          aulPara[MAX_GLOBAL_PRANT_NUM];
}LPS_NV_GLOBAL_PRINT_STRU;

/*****************************************************************************
 结构名    : LPS_NV_MEAS_THRESHOLD_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_UINT8 ucSynsSwitch;     /*是否启动测量值掉底发起重建流程 */
    VOS_UINT8 ucRsrpLowCount;   /* PHY上报的测量值小于sRsrpHoldThreshold达到两次。发起重建流程 */
    VOS_INT16 sRsrpHoldThreshold;  /* PHY上报的测量值小于某值 ucRsrpLowCount++*/
}LPS_NV_MEAS_THRESHOLD_STRU;


/* 2655 begin */
 /*****************************************************************************
 结构名    : LPS_NV_LONG_BIT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈的和协议功能相关的开关的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32  bitFlag01                   :1;
    VOS_UINT32  bitFlag02                   :1;
    VOS_UINT32  bitFlag03                   :1;
    VOS_UINT32  bitFlag04                   :1;
    VOS_UINT32  bitFlag05                   :1;
    VOS_UINT32  bitFlag06                   :1;
    VOS_UINT32  bitFlag07                   :1;
    VOS_UINT32  bitFlag08                   :1;
    VOS_UINT32  bitFlag09                   :1;
    VOS_UINT32  bitFlag10                   :1;
    VOS_UINT32  bitFlag11                   :1;
    VOS_UINT32  bitFlag12                   :1;
    VOS_UINT32  bitFlag13                   :1;
    VOS_UINT32  bitFlag14                   :1;
    VOS_UINT32  bitFlag15                   :1;
    VOS_UINT32  bitFlag16                   :1;
    VOS_UINT32  bitFlag17                   :1;
    VOS_UINT32  bitFlag18                   :1;
    VOS_UINT32  bitFlag19                   :1;
    VOS_UINT32  bitFlag20                   :1;
    VOS_UINT32  bitFlag21                   :1;
    VOS_UINT32  bitFlag22                   :1;
    VOS_UINT32  bitFlag23                   :1;
    VOS_UINT32  bitFlag24                   :1;
    VOS_UINT32  bitFlag25                   :1;
    VOS_UINT32  bitFlag26                   :1;
    VOS_UINT32  bitFlag27                   :1;
    VOS_UINT32  bitFlag28                   :1;
    VOS_UINT32  bitFlag29                   :1;
    VOS_UINT32  bitFlag30                   :1;
    VOS_UINT32  bitFlag31                   :1;
    VOS_UINT32  bitFlag32                   :1;
}LPS_NV_LONG_BIT_STRU;
/*****************************************************************************
 结构名    : LPS_NV_TL_L2_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : L2功能相关的开关的结构体
*****************************************************************************/
typedef struct
{
    /* 配置RLC 上行组包个数限制 ，NV默认为45*/
    VOS_UINT32                          ulRlcUlSduLimit;

    LPS_NV_LONG_BIT_STRU                stL2Flag;

    /************************stL2Flag各个BIT含义***************************
     bitFlag01: 设置抓取L2 MAC PDU中的参数 g_ulIfForwardToPc, 默认为关
     bitFlag02: 设置抓取L2 MAC PDU中的参数 g_ulCapPusch, 默认为关
     bitFlag03: 设置抓取L2 MAC PDU中的参数 g_ulCapPdsch, 默认为关
     bitFlag04: 设置抓取L2 MAC PDU中的参数 g_ulCapPdcp, 默认为关
     bitFlag05: 设置抓取L2 MAC PDU中的参数 g_ulTcpDebugFlg, 默认为关
     bitFlag06: 设置抓取L2 MAC PDU中的参数 g_ulTcpDlFlg, 默认为关
     bitFlag07: 设置抓取L2 MAC PDU中的参数 g_g_ulTcpUlFlg, 默认为关
     bitFlag08: 设置抓取L2 MAC PDU中的参数 g_ulTcpCtrlPduFlg, 默认为关
     bitFlag09: 设置RLC POLLING 参数能力的开关g_ulPollingCfgSwitch,默认为关
     bitFlag10:
     bitFlag11:
     bitFlag12:
     bitFlag13:
     bitFlag14:
     bitFlag15:
     bitFlag16:
     bitFlag17:
     bitFlag18:
     bitFlag19:
     bitFlag20:
     bitFlag21:
     bitFlag22:
     bitFlag23:
     bitFlag24:
     bitFlag25:
     bitFlag26:
     bitFlag27:
     bitFlag28:
     bitFlag29:
     bitFlag30:
     bitFlag31:
     bitFlag32:
    ****************************************************************************/

    LPS_NV_LONG_BIT_STRU                stL2Flag1;

    /*********预留位使用规则************************
    Rev[0]为规避罗德仪器不响应IPV6前缀问题，使用NV控制的全局变量，IPV6前缀默认UE只发1次
    Rev[1]设置ROHC压缩模式
    Rev[2]按照华为基站配置参数pollingPdu,默认32，单位个，防止参数太大状态报告不及时，上行内存不释放引起的流量问题
    Rev[3]按照华为基站配置参数pollingByte，默认为25000，单位字节
    **********************************/
    VOS_UINT32                          ulRev[32];
}LPS_NV_TL_L2_PARA_STRU;
/*****************************************************************************
 结构名    : LPS_NV_TL_L2_CHR_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : L2 CHR控制参数结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulRptPeriod;
    VOS_UINT32                          ulReestRptPeriod;
    VOS_UINT32                          ulReestRptThres;
    VOS_UINT32                          ulPdcpUlLossRptPeriod;
    VOS_UINT32                          ulPdcpUlLossRptThres;
    VOS_UINT32                          ulPdcpUlBuffTimeRptThres;
    VOS_UINT32                          ulPdcpDlLossRptPeriod;
    VOS_UINT32                          ulPdcpDlLossRptThres;
    VOS_UINT32                          ulMacDlTbCheckErrRptPeriod;
    VOS_UINT32                          ulMacDlTbCheckErrRptThres;
    VOS_UINT32                          ulPdcpSigPasRptPeriod;
    VOS_UINT32                          ulImsVoiceStopRptPeriod;
    VOS_UINT32                          ulImsVoiceStopRptThres;
    VOS_UINT32                          aulResv[24];
}LPS_NV_TL_L2_CHR_PARA_STRU;

/*****************************************************************************
 结构名    : LPS_SWITCH_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈的和协议功能相关的开关的结构体
*****************************************************************************/
typedef struct
{
    /* 配置TA Timer超时SRS和PUCCH释放开关 0:关闭，1:打开，默认关闭 */
    VOS_UINT32                          ulCloseTaTimer;

    /* 配置SR触发方开关 0:关闭  1:打开，默认关闭 */
    VOS_UINT32                          ulSrTrigFlag;

    /* 配置SR触发随机接入开关 0:关闭，1:打开，默认关闭 */
    VOS_UINT32                          ulCloseSrRandFlag;

    /*同失步开关，0为关闭，1为打开。默认为关闭，设置为打开*/
    /*VOS_UINT32                          ulSyncCtrlFlag;*/

    /*连接态测量开关，0为关闭，1为打开。默认为关闭，设置为打开*/
    VOS_UINT32                          ulConnMeasFlg;

    /*设置测量IND消息频率，参数表示上报间隔: 取值方法要大于等于1*/
    VOS_UINT32                          ulMeasReportMaxNum;

    /*0:关闭抓包功能 1:打开抓包功能 默认是打开*/
    VOS_UINT32                          ulIfForwardToPc;

    /*打开安全加密开关，仅仅需要时设置该标志*/
    VOS_UINT32                          ulSmcControl;

    /*规避eNB的TDD的接入问题*/
    /*VOS_UINT32                          ulMsg4OnlyPadding;*/

    /*判断是否BAR小区开关*/
    VOS_UINT32                          ulCloseBarCell;

    /*DRX功能开关 0:关闭  1:打开*/
    VOS_UINT32                          ulDrxControlFlag;

   /*外场测试桩代码开关 0:关闭  1:打开.默认为关闭*/
    VOS_UINT32                          ulFieldTestSwitch;

    /* PCO功能开关，0为关闭，1为打开。默认为打开 */
    VOS_UINT32                          ulPcoFlag;

    /*流控结构*/
    LPS_NV_FLOW_CONTROL                  stPsNvFlowCtl;

    /************************stPsGcfFlag01各个BIT含义***************************
     bitFlag01:表示GCF测试场景TAU消息是否加密，平台缺省值为1。
               0: 加密；1:不加密
     bitFlag02:表示GCF测试场景msg4解析开关，平台缺省值为1。
               0: 表示按协议实现; 1:表示按规避方案实现
     bitFlag03:表示GCF测试场景是否支持24.301 Table D.1.1中建链原因设置，平台缺省值为1。
               0: 支持; 1:不支持
     bitFlag04:表示GCF测试场景指示安全功能是否支持测试模式，测试模式支持空完整性算法，平台缺省值为1。
               0: 支持; 1:不支持
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stPsGcfFlag01;

    /************************stPsGcfFlag02各个BIT含义***************************
     stPsGcfFlag01各个BIT含义:
     bitFlag01:
     bitFlag02:
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stPsGcfFlag02;

    /************************stPsFunFlag01各个BIT含义***************************
     bitFlag01: Band64是否支持，缺省值:0; 0:不支持;1:支持
     bitFlag02: 删除SDF时是否携带Qos信息,缺省值:0; 0:携带;1:不携带
     bitFlag03: 是否进行连接态收到本ue的业务寻呼后进行释放,缺省值:0; 0:释放;1:不释放
     bitFlag04: 校验安全功能开关,缺省值:0; 0:打开;1:关闭
     bitFlag05:
     bitFlag06:
     bitFlag07: 拨号功能合入之前，装备测试时上行DRB数据在PDCP入口丢掉,0为关闭，1为打开。默认为关闭
     bitFlag08: Print时延优化开关，缺省值:0; 0:关;1:开;但是RRM测试时时自动生效的
     bitFlag09: 层间消息时延优化开关，缺省值:0; 0:关;1:开;
     bitFlag10: MBMS的支持开关，缺省值:0; 0:关;1:开;
     bitFlag11: SBM定制需求开关，特定band支持特定的带宽.缺省值:0; 0:关;1:开;
     bitFlag12: 检查整个小区的带宽是都在协议频段内，缺省值:1; 0:关;1:开;
     bitFlag13: 测试部需求,模拟切换失败用例，缺省值:1; 0:关;1:开;
     bitFlag14: DCM定制需求 GU到L的切换NAS不判断被禁TA;缺省值:0; 0:关;1:开;
     bitFlag15: 是否支持ETWS功能;缺省值:1; 0:关;1:开;
     bitFlag16: 是否支持CMAS功能;缺省值:1; 0:关;1:开;
     bitFlag17:
     bitFlag18: L重建时是否搜索UTRAN;缺省值:1; 0:关;1:开;
     bitFlag19: L重建时是否搜索GSM;缺省值:1; 0:关;1:开;
     bitFlag20: 部分接收到系统信息时,如果小区RSRP门限小于设定值，用于判断否需要bar小区，;缺省值:0; 0:关;1:开;
     bitFlag21: NCC从左向右还是从右向左，目前按照协议实现，缺省值:0; 0表示按照协议，1表示不按照协议
     bitFlag22: 是否支持背景搜索,缺省值:0;1:关，0:开；
     bitFlag23: 重建时需要根据GAP是否有配置重新配置一下物理层，缺省值:1; 1:开，0:关
     bitFlag24:
     bitFlag25:
     bitFlag26: IDLE下重选前不判断NCC，缺省值:0; 0表示需要判断；1表示不判断。
     bitFlag27: V7R1日本DCOM需求，缺省值:0; 0:关;1:开;
     bitFlag28: V7R1日本DCOM需求，缺省值:0; 0:关;1:开;
     bitFlag29: V7R1日本DCOM需求，缺省值:0; 0:关;1:开;
     bitFlag30: V7R1日本DCOM需求，缺省值:0; 0:关;1:开;
     bitFlag31: V7R1日本DCOM需求，缺省值:0; 0:关;1:开;
     bitFlag32: RRM测试时，缺省关闭打印是否生效，缺省值:0; 1:不生效；0:生效。
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stPsFunFlag01;

    /************************stPsFunFlag02各个BIT含义***************************
     bitFlag01: CA特性开关:缺省值:0; 0:开;1:关:;
     bitFlag02:
     bitFlag03:
     bitFlag04: CMAS在驻留之前还是驻留之后上报，缺省值:0;0: 驻留之后上报; 1:驻留前后都可以上报
     bitFlag05:
     bitFlag06:
     bitFlag07: 背景搜，只搜L单模.不搜GU模开关.缺省值:0; 0:关;1:开;
     bitFlag08: ETWS发生时机; 1:立即发送，0:收齐后再发送
     bitFlag09: TL L2关键事件上报-缺省值:0;0:关;1:开
     bitFlag10: LTE L2 数传信息上报-缺省值:0;0:关;1:开
     bitFlag11: LIST搜网先验频点是否搜索2次。0:关;1:开
     bitFlag12: 网络指定模式重定向结束后是否继续发起其他模式的重定向流程; 0:发起网络指定模式的重定向之后继续发起其他模式的重定向;1:只发起网络指定模式的重定向
     bitFlag13: 是否支持安全上下文激活时处理明文的NAS拒绝消息。缺省值: 0; 0: 不支持 1: 支持
     bitFlag14: LTE CA小区状态信息上报-缺省值:0;0:关;1:开
     bitFlag15: 为解决网络BUG导致的CSFB流程FR回到L时概率收不到paging的问题，设置的接入层发起的异系统变换到L下发起TAU类型总是type2类型开关。缺省值0.0: 关闭 1:打开
     bitFlag16: LTE模内切换NAS忽略被禁TA,缺省值:0; 0:关; 1:开
     bitFlag17: ESR过程中发生切换,未发起TAU流程,重发ESR消息控制开关,缺省值:0; 0:不重传 1:重传
     bitFlag18: csfb场景收不到网侧release场景优化开关，缺省值:0;0:关;1:开
     bitFlag19: LRRC检测异常主动上报CHR功能开关,缺省值:1; 0:关闭;1:打开
     bitFlag20: 单双卡切换UE能力动态获取开关,0:关;1:开;chicago平台默认开,其他平台默认关闭
     bitFlag21: UE不支持CA时,如果仪表强配CA,UE是否回复RECFG_CMP开关;缺省值:0;  0:不回复CMP; 1:回复CMP
     bitFlag22: 台湾CMAS临时方案开关; 缺省值:0;  0:关闭; 1:打开;
     bitFlag23:
     bitFlag24:
     bitFlag25:
     bitFlag26:
     bitFlag27:
     bitFlag28:
     bitFlag29:
     bitFlag30:
     bitFlag31:
     bitFlag32:
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stPsFunFlag02;

    
    LPS_NV_LONG_BIT_STRU                 stPsFunFlag03;

   /************************stPsFunFlag04各个BIT含义***************************
     bitFlag01: 检测LTE网侧寻呼情况 若打开,IDLE态不处理本UE Paging。默认为0.
                0:功能关闭  1:功能打开
     bitFlag02:
     bitFlag03:
     bitFlag04:
     bitFlag05:
     bitFlag06: 控制L2 MAC随机接入时对于高速小区是否启动小区半径外接入优化，1表示启动,默认关闭，由产品线定制
     bitFlag07: 控制L2 MAC随机接入时对于Preamble差2是否启动小区半径外接入优化,1表示启动，默认关闭，由产品线定制
     bitFlag08:
     bitFlag09:
     bitFlag10:
     bitFlag11:
     bitFlag12:
     bitFlag13:
     bitFlag14:
     bitFlag15:
     bitFlag16:
     bitFlag17:
     bitFlag18:
     bitFlag19:
     bitFlag20:
     bitFlag21:
     bitFlag22:
     bitFlag23:
     bitFlag24:
     bitFlag25:
     bitFlag26:
     bitFlag27:
     bitFlag28:
     bitFlag29:
     bitFlag30:
     bitFlag31:
     bitFlag32:
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stPsFunFlag04;

    LPS_NV_GLOBAL_PRINT_STRU             stPsNvGlobalPrint;
}LPS_SWITCH_PARA_STRU;

/* Tps相关开关控制*/
/*****************************************************************************
 结构名    : TPS_SWITCH_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈的和协议功能相关的开关的结构体
*****************************************************************************/
typedef struct
{
    /************************stTpsLayFliterFlag01各个BIT含义********************
     bitFlag01:层间消息OP_HL1_SCHEDULE_IND过滤开关。平台缺省值为0
               0:过滤 1:不过滤
     bitFlag02:层间消息OP_T_RRC_trafficMeasPeriod过滤开关。平台缺省值为0
               0:过滤 1:不过滤
     bitFlag03:部分动态打印关闭，一般是数组、循环等次要信息关闭。平台缺省值为0
               0:关闭打印 1:不关闭打印
     bitFlag04:
     bitFlag05:
     bitFlag06:
     bitFlag07:
     bitFlag08:
     bitFlag09:
     bitFlag10:
     bitFlag11:
     bitFlag12:
     bitFlag13:
     bitFlag14:
     bitFlag15:
     bitFlag16:
     bitFlag17:
     bitFlag18:
     bitFlag19:
     bitFlag20:
     bitFlag21:
     bitFlag22:
     bitFlag23:
     bitFlag24:
     bitFlag25:
     bitFlag26:
     bitFlag27:
     bitFlag28:
     bitFlag29:
     bitFlag30:
     bitFlag31:
     bitFlag32:

    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stTpsLayFliterFlag01;

    /************************stTpsLayFliterFlag01各个BIT含义********************
     bitFlag01:
     bitFlag02:
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stTpsLayFliterFlag02;

    /************************stTPsGCFFlag01各个BIT含义**************************
     bitFlag01: GCF是否打开cs loop 功能。
                平台缺省值为0
                0:关闭CsLoop功能  1:打开功能
                此功能已经自动识别，无需打开。

     bitFlag02: TDS快速搜网功能,非标功能。功能打开时TRRC不满足S准则，直接发起搜网流程。
                平台缺省值为0
                0:功能关闭 1:功能打开

     bitFlag03: 准FR特性开关
                平台缺省值为1
                0:功能关闭 1:功能打开

     bitFlag04: TG互操作，sOfIratsearch门限优化，已不使用。
                平台缺省值为0
                0:关闭功能  1:打开功能

     bitFlag05: TG互操作，QrxLev门限优化，已不使用。
                平台缺省值为0
                0:关闭功能  1:打开功能

     bitFlag06: 当T的S值小于0时，如果GSM有S值大于0的小区则直接重选，已不使用。
                平台缺省值为0
                0:关闭功能  1:打开功能

     bitFlag07: 重选优化,非标功能。如果是能量在这个范围内，把能量降6db，保证重选到tds时，t能量足够强
                平台缺省值为0
                0:关闭功能  1:打开功能

     bitFlag08: RRM桩函数，只有当此功能打开，RRM测试才能通过。
                平台缺省值为0
                0:功能关闭 1:功能打开

     bitFlag09:
     bitFlag10:
     bitFlag11:
     bitFlag12:
     bitFlag13:
     bitFlag14:
     bitFlag15:
     bitFlag16:
     bitFlag17:
     bitFlag18:
     bitFlag19:
     bitFlag20:
     bitFlag21:
     bitFlag22:
     bitFlag23:
     bitFlag24:
     bitFlag25:
     bitFlag26:
     bitFlag27:
     bitFlag28:
     bitFlag29:
     bitFlag30:
     bitFlag31:
     bitFlag32:
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stTPsGCFFlag01;

    /************************stTPsGCFFlag02各个BIT含义**************************
     bitFlag01:
     bitFlag02:
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stTPsGCFFlag02;

    /************************stTPsFunFlag01各个BIT含义**************************
     bitFlag01: CellUpdate流程优化，在T313超时，如果只有PS域时不发起CellUpdate直接,进入IDL状态
                平台缺省值为1。
                0:功能打开 1:功能关闭

     bitFlag02: debug信息处理，对相关状态下的小区信息、状态等数据进行保存，
                平台缺省值为0.
                0:功能打开 1:功能关闭

     bitFlag03: debug信息上报功能，将Debug信息以层间形式上报。
                平台缺省值为0。
                0:功能打开 1:功能关闭

     bitFlag04: SMC使用同步函数接口的参数
                平台缺省值为0
                0:功能关闭 1:功能打开

     bitFlag05: TCP ACK丢弃功能。
                平台缺省值为0
                0:功能关闭 1:功能打开

     bitFlag06: DetectCell功能。
                平台缺省值为0
                0:功能关闭 1:功能打开

     bitFlag07: 规避网络的无效MeasCtrl，在特定网络环境下使用
                平台缺省值为0
                0:功能关闭 1:功能打开

     bitFlag08: CCCH消息上报，屏蔽掉不属于UE的空口
                平台缺省值为1
                0:功能关闭 1:功能打开

     bitFlag09:ADRX开启功能 ,在有寻呼重发的小区内，有效的延长的DRX唤醒周期，可以有效降低待机功耗
               平台层缺省值为0
               0:功能关闭 1:功能打开

     bitFlag10:DMO message function
               0:功能关闭 1:功能打开

     bitFlag11:郑州洛阳掉网SMC验证失败优化开关，功能开启则SMC验证失败后通知GUNAS删除本地安全上下文
               平台层缺省值为0
               0:功能关闭 1:功能打开

     bitFlag12:CS连接态下T对L测量的评估测报是否上报的优化开关
               平台层缺省值为1
               0:功能关闭 1:功能打开

     bitFlag13:ADRX强制开启功能 ,强制延长的DRX唤醒周期，可以有效降低待机功耗
               平台层缺省值为0
               0:功能关闭 1:功能打开
     bitFlag14:CS FACH重选优化开关，开启时如果服务小区能量低于门限值则重选，高于门限不重选
              平台缺省值为0
              0:功能关闭 1:功能打开
     bitFlag15:L2T重定向搜网优化开关，开启时用测量到的小区作为先验信息
               平台缺省值为0
               0:功能关闭 1:功能打开
     bitFlag16:
     bitFlag17:搜网优化MAC定时器优化开关，开启时MAC长时间不给TRRC上报消息，直接回复搜网失败
              平台缺省值为0
              0:功能关闭 1:功能打开
     bitFlag18:搜网优化先验信息优化开关，开启时用测量到的小区作为先验信息
               平台缺省值为0
               0:功能关闭 1:功能打开
     bitFlag19:利用CSSBANDSCAN，优化扫频开关
               0:功能关闭 1:功能打开
     bitFlag20:FR特性开关
               平台缺省值为1
               0:功能关闭 1:功能打开
     bitFlag21:PS域链路存在时FR开关
               平台缺省值为1
               0:功能关闭 1:功能打开
     bitFlag22:RRC connection request失败优化开关，当V300>N300时,Bar当前小区开关
               平台缺省值为1
               0:功能关闭 1:功能打开
     bitFlag23:
     bitFlag24:TG 3A测量报告上报优化开关
               平台缺省值为1
               0:功能关闭 1:功能打开
     bitFlag25:Idl态新增DEBUG消息打印开关
               平台缺省为1
               0:功能关闭 1:功能打开
     bitFlag26:Conn态新增DEBUG消息打印开关
               平台缺省为1
               0:功能关闭 1:功能打开
     bitFlag27:Ac都被Bar优化功能开关
               平台缺省为1
               0:功能关闭 1:功能打开
     bitFlag28:
     bitFlag29:
     bitFlag30:
     bitFlag31:
     bitFlag32:

    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stTPsFunFlag01;

    /************************stTPsFunFlag02各个BIT含义**************************
     bitFlag01: TDS DSDS 控制是否在DCH_CONFIG设置不抢.默认为0
                0:功能打开  1:功能关闭
     bitFlag02: TDS DSDS 控制是否有500ms发起SignalRel。默认为0.
                0:功能打开  1:功能关闭
     bitFlag03:检测TDS网侧寻呼情况 若打开,IDLE态不处理本UE Paging。默认为0.
                0:功能关闭  1:功能打开
    ****************************************************************************/
    LPS_NV_LONG_BIT_STRU                 stTPsFunFlag02;
}TPS_SWITCH_PARA_STRU;

/* end Tps相关开关控制*/
/*****************************************************************************
 结构名    : TDS_CERSSI_REPORT_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈的和协议功能相关的开关的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32                           ulThreshold;
    VOS_UINT32                           ulInterval;
    VOS_UINT8                            aucRsv[4];
}TDS_CERSSI_REPORT_PARA_STRU;
/*****************************************************************************
 结构名    : LPS_LTE_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈的和LTE相关的配置
*****************************************************************************/
typedef struct
{
    /* LTE天线个数 */
    VOS_UINT32                          ulPsCfgDspAntCnt;

    /*VOS_UINT32                          ulPsCfgDspRatMod;*/
    /*频带号，取值范围从1至40，分别对应36.101的band取值,默认为20*/
    VOS_UINT8                          ucEutraBand;

    VOS_UINT8                          aucRsv[3];

    /* 上下行子帧配比，默认为1 */
    VOS_UINT32                          ulCfgSubFrameAssign;

    /* 特殊子帧配置，默认为7 */
    VOS_UINT32                          ulSubFramePattern;
}LPS_LTE_CONFIG_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CAND_CELL_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 候补小区NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usFreqCount;                            /* 有效的频率数目 */
    VOS_UINT8                           aucReserved1[2];
    RRC_NV_CSEL_CAND_FREQ_STRU          ausCandFreqs[RRC_NV_EACH_BAND_FREQ_COUNT];/* 先验频率列表 */
}LRRC_NV_CAND_CELL_LIST_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_SUPPORT_BAND_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE支持频段NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usBandCount;                            /* 有效的频带数目 */
    VOS_UINT8                           aucReserved1[2];
    RRC_NV_CSEL_FREQ_BAND_INFO_STRU     astCandBands[RRC_NV_EACH_BAND_BANDS_COUNT];
}LRRC_NV_SUPPORT_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : RRC_NV_EACH_BAND_IND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 每个Band下的候补信息
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucBandInd;                              /* 频段指示 */
    PS_BOOL_ENUM_UINT8                  enFirstSearchFlg;                       /* 频点或频段搜索阶段，优先搜索这个频段 */
    VOS_UINT8                           aucReserved1[2];
    LRRC_NV_CAND_CELL_LIST_STRU         stCandCellInfo;                         /* Band内候补小区信息 */
    LRRC_NV_SUPPORT_BAND_LIST_STRU      stCandBandsInfo;                        /* Band内候补频段信息 */
}LRRC_NV_EACH_BAND_CAND_INFO_STRU;


/*****************************************************************************
 结构名    : LRRC_NV_CALIBRATION_BAND_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE需要校准的band信息
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[2];
    VOS_UINT16                          usCnt;
    VOS_UINT8                           aucEutraBand[RRC_MAX_NUM_OF_BANDS];
}LRRC_NV_CALIBRATION_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : LRRC_UE_CAP_MEAS_EUTRA_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中MEAS->Band参数
*****************************************************************************/
typedef struct
{
    RRC_INTER_FREQ_EUTRA_BAND_INFO_STRU stInterFreqEutraBandInfo;
    RRC_INTER_RAT_BAND_INFO_STRU        stInterRatBandInfo;
}LRRC_UE_CAP_MEAS_EUTRA_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力信息
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitFeatureGroupIndsPresent    : 1;
    VOS_UINT32                          bitUeCapV920IesPresent        : 1;
    VOS_UINT32                          bitSpare                      : 30;
    VOS_UINT8                           aucReserved[2];
    /* 用来设置UE接入层版本，1：支持，0：不支持，从低到高位分别用来指示Rel8,Rel9,.... */
    VOS_UINT8                           ucAsRelBitmap;
    RRC_UE_CATEGORY_ENUM_UINT8          enUeCatg;
    RRC_UE_CAP_PDCP_PARA_STRU           stPdcpPara;
    RRC_UE_CAP_PHY_PARA_STRU            stPhyPara;
    RRC_UE_CAP_RF_PARA_STRU             stRfPara;
    RRC_UE_CAP_MEAS_PARA_STRU           stMeasPara;
    RRC_UE_CAP_FEATURE_GROUP_IND_STRU   stFeatrueGroupInds;
    RRC_UE_CAP_INTER_RAT_PARA_STRU      stInterRatPara;
    RRC_UE_EUTRA_CAP_V920_IES_STRU      stUeCapV920Ies;
} LRRC_NV_UE_EUTRA_CAP_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_RRC_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC控制开关结构
             重选3db控制开关,BAR小区后最大搜索次数,SIB接收定时器选择开关
*****************************************************************************/
typedef struct
{
    RRC_NV_SWITCH_ENUM_UINT32            en3dbCtlFlg;           /* 默认值为关闭 0 */
    VOS_UINT32                           ulMaxSeachCntOfBarFrq;
    RRC_NV_TIMER_CHOICE_ENUM_UINT32      enSibTimerChoiceFlg;   /* 默认为0 */
    VOS_UINT32                           ulReserved;            /* 预留1位 */
}LRRC_NV_RRC_CTRL_STRU;

enum LRRC_NV_LOCK_MODE
{
    LRRC_NV_LOCK_MODE_RoadTest                    = 0,          /*路测锁频，该锁频模式下,对List搜有限制*/
    LRRC_NV_LOCK_MODE_POS                         = 1,          /*POS机、抄表锁频，该锁频模式下，要求对List搜无限制*/
    LRRC_NV_LOCK_MODE_BUTT
};
typedef VOS_UINT8   LRRC_NV_LOCK_MODE_UINT8;

/*****************************************************************************
 结构名    : LRRC_NV_LOCK_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :NV项锁信息数据结构
*****************************************************************************/
typedef struct
{
    RRC_NV_LOCK_INFO_ENUM_UINT32         enLockInd;
    /*begin: add by  */
    PS_BOOL_ENUM_UINT8                   ucActiveFlag;                   /*有效标志位，用于下电丢失,PS_TRUE下电有效，否则无效*/
    LRRC_NV_LOCK_MODE_UINT8              ucLockMode;                     /*锁频模式,*/
    /*end: add by  有效标志位，用于下电丢失,PS_TRUE下电有效，否则无效*/
    VOS_UINT8                            ucBandInd;                      /* 频段指示 */
    VOS_UINT8                            ucForbidReselAndHO;             /* 锁频过程中是否允许切换和重选 */
    VOS_UINT16                           usCellId;                       /* 小区ID */
    VOS_UINT16                           usFreqPoint;                    /* 频率 */
    RRC_NV_CSEL_FREQ_BAND_INFO_STRU      stLockedBand;                   /* 锁定Band信息 */
} LRRC_NV_LOCK_INFO_STRU;

/* modify begin */

/*****************************************************************************
 结构名    : LRRC_NV_CSQ_RPT_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 能量上报的周期和门限
*****************************************************************************/
typedef struct
{
    VOS_UINT32                       ulRssiReportLen;
    VOS_UINT32                       ulDetaRssi;    /* 能量上报门限 */
    VOS_UINT16                       usMs;          /* 上报时间间隔 */
    VOS_UINT16                       usDetaSINR;    /* 信噪比门限 (0,50)db*/
} LRRC_NV_CSQ_RPT_INFO_STRU;

/* modify end */

/*****************************************************************************
 结构名    : LRRC_NV_LWCLASH_RANGE_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 冲突范围的上限和下限
*****************************************************************************/
typedef struct
{
    VOS_UINT16                        usFreqBegin;        /*范围下限频点 单位:100Khz*/
    VOS_UINT16                        usFreqEnd;          /*范围上限频点 单位:100Khz*/
} LRRC_NV_LWCLASH_RANGE_INFO_STRU;
/*****************************************************************************
 结构名    : LRRC_NV_LWCLASH_RANGE_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 冲突范围的上限和下限
*****************************************************************************/
typedef struct
{
    VOS_UINT16                        ulnum;                /*范围下限频点 单位:100Khz*/
    VOS_UINT8                         ucRev[2];
    LRRC_NV_LWCLASH_RANGE_INFO_STRU   stBandRng[RRC_NV_LWCLASH_RANGE_COUNT]; /*范围上限频点 单位:100Khz*/
} LRRC_NV_LWCLASH_RANGE_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_UE_RFRD_TYPE_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :NV项射频类型结构
*****************************************************************************/
typedef struct
{
    RRC_NV_UE_RFRD_TYPE_ENUM_UINT8       enUeRfrdType;              /* 射频类型 */
    VOS_UINT8                            aucReserved[3];

} LRRC_NV_UE_RFRD_TYPE_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_RF_HW_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :9361RF配置
*****************************************************************************/
typedef struct
{
    RRC_PHY_LTE_RF_HW_ENUM_UINT8       enLteRfHw;
    VOS_UINT8                          aucRsv[3];
} LRRC_NV_RF_HW_STRU;

/*****************************************************************************
 结构名    : LTE_COMPENSATE_SWITCH_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 补偿开关, 从NV读出配置DSP
*****************************************************************************/
typedef struct
{
    VOS_UINT16 usFreqCmpnstSwitch;      /*0：关掉频率补偿 1:打开频率补偿*/
    VOS_UINT16 usTempCmpnstSwitch;      /*0：关掉温度补偿  1:打开温度补偿*/
    VOS_UINT16 usTxPwrCmpnstSwitch;     /*0：关掉发射功率补偿 1:打开发射补偿 。当前此开关预留*/
    VOS_UINT16 usReserved;
}LTE_COMPENSATE_SWITCH_STRU;

/*****************************************************************************
 结构名    : LUPLINK_POWER_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 功控配置, 从NV读出配置DSP
*****************************************************************************/
typedef struct
{
    VOS_UINT16 usPuschPwrCtrlSwitch;
    VOS_UINT16 usPuschClosedLoopSwitch;
    VOS_UINT16 usPucchPwrCtrlSwitch;
    VOS_UINT16 usPucchClosedLoopSwitch;
    VOS_UINT16 usSrsPwrCtrlSwitch;
    VOS_UINT16 usPrachPwrCtrlSwitch;

    VOS_INT16  sPuschTxPower;
    VOS_INT16  sPucchTxPower;
    VOS_INT16  sSrsTxPower;
    VOS_INT16  sPrachTxPower;
}LUPLINK_POWER_CONFIG_STRU;

/* LTE PS Transfer begin */
/* MTC AREA LOST Begins */
#if(FEATURE_MULTI_MODEM == FEATURE_ON)
typedef struct
{
    PS_BOOL_ENUM_UINT8      enValidFlag;  /*门限是否有效  */
    VOS_UINT8               ucLrrcMTCLostThreshold; 	/* LTE的PS迁移门限,0~31 */
    VOS_UINT8               ucLrrcMTCAvailableDValue;    /* LTE的PS回迁门限与迁移门限差值,0~(31-ucLrrcMTCLostThresh) */
    VOS_UINT8               ucMTCLostTimerLen;    /* LTE->CDMA的PS迁移过程的定时器长度,取值单位:s */
    VOS_INT16               sRsv; /*保留*/
    VOS_UINT16              usMTCAvailableTimerLen; /* CDMA->LTE的PS迁移过程的定时器长度,取值单位:s */
}LTE_MTC_AREA_LOST_THRESHOLD_STRU;
#endif
/* MTC AREA LOST End */
/* LTE PS Transfer end */


/* -PLMN_EXACTLY_COMPARE_FLG */
#ifdef PS_ITT_PC_TEST2
typedef struct
{
    VOS_UINT8   ucPlmnExactlyCompareFlag;
    VOS_UINT8   aucRsv[3];
}NVIM_PLMN_EXACTLY_COMPARE_FLAG_STRU;
#endif
/* TCXo */


/*****************************************************************************
 结构名    : LTE_GATE_CLOCK_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :时钟门控
*****************************************************************************/
typedef struct
{
    VOS_UINT16 usGateClockEnableFlag;       /* 0:invalid, 1:Valid, 默认 0 */
    VOS_UINT16 usRsvd;
} LTE_GATE_CLOCK_CONFIG_STRU;

/*****************************************************************************
 结构名    : FC_LTE_CPU_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 定义LTE CPU流控配置结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16          usFirstDelay;           /* 延迟进入流控状态，连续几个周期高于阈值才进行流控 */
    VOS_UINT16          usUlDownRate;           /* 上行降速步长百分比 */
    VOS_UINT16          usUlUpRate;             /* 上行升速步长百分比 */
    VOS_UINT16          usUlMinThr;             /* 上行降速下限 kbps */
    VOS_UINT16          usHqDownRate;           /* HARQ降速步长百分比 */
    VOS_UINT16          usHqUpRate;             /* HARQ升速步长百分比 */
    VOS_UINT16          usHqMaxDiscardRate;     /* HARQ丢包上限 */
    VOS_UINT16          usDowngradeThres;       /* 降速阈值，CPU占用率高于该值时，开始降速 */
    VOS_UINT16          usUpgradeThres;         /* 升速阈值，CPU占用率低于该值时，开始升速 */
	VOS_UINT16          usResv;
} FC_LTE_CPU_CONFIG_STRU;

/*****************************************************************************
 结构名    : FC_LTE_TEMPERATURE_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 定义LTE 温控配置结构
*****************************************************************************/
#define FC_UL_THROUGHPUT_THRES_CNT_NV          (12)
typedef struct
{
    VOS_UINT8           ucInitialPos;           /* 降速开始位置0~11, 默认4 */
    VOS_UINT8           ucZeroSupport;          /* 是否支持速率降到0,0表示不支持，默认0 */
    VOS_UINT8           ucResv[2];              /* 保留位 */
    VOS_UINT32          ulTemperSteps[FC_UL_THROUGHPUT_THRES_CNT_NV];      /* 温控速率档位kbps，初值[50000, 40000, 30000, 25000, 20000, 15000, 10000, 5000, 2000, 0, 0, 0] */
    VOS_UINT32          ulMinBsrThr;            /* BSR能够控制的最小速率kbps，默认120 */
} FC_LTE_TEMPERATURE_CONFIG_STRU;

/*****************************************************************************
 结构名    : FLOWCTRL_LTE_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 定义LTE流控配置结构
*****************************************************************************/
typedef struct
{
    FC_LTE_TEMPERATURE_CONFIG_STRU stFcTemperatureConfig;
    FC_LTE_CPU_CONFIG_STRU  stFcCpuConfig;
} FLOWCTRL_LTE_CONFIG_STRU;


/* -SYSCFG-SAVENV-Begin */
#define NV_ITEM_EUTRA_MAX_SUPPORT_BANDS_NUM         (64)
#define NV_MAX_FDD_FREQ_BANDS_NUM                 (32)

/*
NVIM_UE_SUPPORT_FREQ_BAND_STRU结构说明:
usWcdmaBand和usGsmBand用Bit位表示用户设置的频段，bit1代表频段I,bit2代表频段II,
依次类推,比特位为1,表示支持该频段.下表是比特位和频段对应关系:
-------------------------------------------------------------------------------
        bit8       bit7      bit6     bit5    bit4     bit3      bit2     bit1
-------------------------------------------------------------------------------
WCDMA   900(VIII)  2600(VII) 800(VI)  850(V)  1700(IV) 1800(III) 1900(II) 2100(I) oct1
        spare      spare     spare    spare   spare    spare     spare   J1700(IX)oct2
-------------------------------------------------------------------------------
GSM频段 1900(VIII) 1800(VII) E900(VI) R900(V) P900(IV) 850(III)  480(II)  450(I)  oct3
        spare      spare     spare    spare   spare    spare     spare    700(IX) oct4
-------------------------------------------------------------------------------
aucUeSupportWcdmaBand和aucUeSupportGsmBand用数组表示UE支持的频段,并以存储顺序的
先后表示频段优先顺序,用0xff表示无效.

例如:
oct1-oct4分别是：0x03,0x00,0x7B,0x00
   则代表用户设置频段为：W：WCDMA-I-2100, WCDMA-II-1900
                         G：850(III),P900(IV),R900(V),E900(VI),1800(VII)
oct5-oct16分别是:2,5,1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff
   则代表UE支持W频段I,II,V,优先顺序是:II,V,I.
oct17-oct28分别是:4,5,8,7,6,3,0xff,0xff,0xff,0xff,0xff,0xff
   则代表UE支持G频段III,IV,V,VI,VII,VIII,优先顺序是:IV,V,VIII,VII,VI,III.
*/
typedef struct
{
    VOS_UINT32                  ulWcdmaBand;
    VOS_UINT16                  usGsmBand;
    VOS_UINT8                   aucReserve[2];
    VOS_UINT8                   aucUeSupportWcdmaBand[NV_MAX_FDD_FREQ_BANDS_NUM];
    VOS_UINT8                   aucUeSupportGsmBand[NV_MAX_FDD_FREQ_BANDS_NUM];
}NV_UE_SUPPORT_FREQ_BAND_STRU;

/*****************************************************************************
 结构名    : NV_UE_EUTRA_FREQ_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LTE支持的一个频段的频点信息结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucBandNo;                               /* band号，值为(1-64) */
    VOS_UINT8                           ucDuplexModeFlg;                        /* 双工模式指示，1为半双工，0为全双工，默认为0 */
    VOS_UINT8                           aucReserve[2];
}NV_UE_EUTRA_FREQ_BAND_INFO_STRU;
/*******************************************************************************
 结构名    : NV_UE_EUTRA_SUPPORT_FREQ_BAND_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LTE支持的能力结构,由LTE,WAS GAS共同读取，包括FDD,TDD,以及LTE支持的频段信息

LTE每个BAND的起始频点号
1  0     – 599
2  600   -  1199
3  1200  – 1949
4  1950  – 2399
5  2400  – 2649
6  2650  – 2749
7  2750  – 3449
83 450   – 3799
93 800   – 4149
10 4150  – 4749
11 4750  – 4949
12 5010  -  5179
13 5180  – 5279
14 5280  – 5379
...
17 5730  – 5849
18 5850  – 5999
19 6000  – 6149
20 6150  – 6449
21 6450  – 6599
...
33 36000 – 36199
34 36200 – 36349
35 36350 – 36949
36 36950 – 37549
37 37550 – 37749
38 37750 – 38249
39 38250 -  38649
40 38650 -  39649
64 25450 -  25749
*******************************************************************************/
typedef struct
{
    VOS_UINT16                              usBandCnt;                          /* LTE band个数  默认值4 */
    VOS_UINT8                               aucReserved1[2];                    /* 保留字节，以备用 */

    /* LTE支持的band个数，最大为64 ，默认支持7，20，38 40，即
        stCandBands[0].ucBandNo  =  7
        astCandBands[1].ucBandNo = 20
        astCandBands[2].ucBandNo = 38
        astCandBands[3].ucBandNo = 40  */
    NV_UE_EUTRA_FREQ_BAND_INFO_STRU       astCandBands[NV_ITEM_EUTRA_MAX_SUPPORT_BANDS_NUM];
}NV_UE_EUTRA_SUPPORT_FREQ_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : NV_GUL_SUPPORT_FREQ_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 记录SYSCFG配置GUL的有效的band结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                       ucValidFlag;
    VOS_UINT8                                       aucReserved1[3];
    NV_UE_SUPPORT_FREQ_BAND_STRU                  stGuCfgBand;
    NV_UE_EUTRA_SUPPORT_FREQ_BAND_LIST_STRU       stLteCfgBand;
}NV_GUL_SUPPORT_FREQ_BAND_STRU;
/* -SYSCFG-SAVENV-End */
/* -nv-bands-cfg-begin */
#define NV_ITEM_EUTRA_MAX_BANDXX_CAND_BANDS_NUM    (12)
#define NV_ITEM_EUTRA_MAX_CUSTOM_BAND_WIDTH_NUM    (6)
/*BEGIN  Fro LTE Band Customize*/
/* begin: modify for macro  redundance,  */
#define NV_LTE_NV_MAX_SUPPORT_BAND                 (47)
/* end:   modify for macro  redundance,  */
#define NV_BAND_IND_64                             (64)
enum RRC_NV_D2DA_BANDXX_FUNC_FLAG_ENUM
{
    RRC_BANDXX_FUNC_INVALIED = 0,
    RRC_BANDXX_FUNC_JAPAN_CUSTOMER,
    RRC_BANDXX_FUNC_CUSTOMIZE_BAND_RANG
};
typedef VOS_UINT16 RRC_NV_D2DA_BANDXX_FUNC_FLAG_ENUM_UINT16;
/*END  Fro LTE Band Customize*/

/*****************************************************************************
 结构名    : LRRC_NV_CUSTOM_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : band定制信息
*****************************************************************************/
typedef struct
{
    VOS_UINT8                         ucBandWidth;/* 0-5分别对应:1.4m、3m、5m、10m、15m、20m */
    VOS_UINT8                         aucReserved1[3];
    RRC_NV_CSEL_FREQ_BAND_INFO_STRU   stCandBands;
}LRRC_NV_CUSTOM_BAND_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_BANDXX_CAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : band定制信息列表
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                       ucBandInds;
    VOS_UINT8                                       ucCustomBandInfoNum;
    VOS_UINT8                                       aucReserved1[2];
    LRRC_NV_CUSTOM_BAND_INFO_STRU                   astCustomBandInfoList[NV_ITEM_EUTRA_MAX_CUSTOM_BAND_WIDTH_NUM];
}LRRC_NV_BANDXX_CAND_INFO_STRU;

/*****************************************************************************
 结构名    : NV_ID_BANDXX_CAND_INFO_GROUP_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 特殊band定制结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                       ucValidFlag;
    VOS_UINT8                                       ucCandBandNum;
    VOS_UINT8                                       aucReserved1[2];
    LRRC_NV_BANDXX_CAND_INFO_STRU                   astCandBandInfoList[NV_ITEM_EUTRA_MAX_BANDXX_CAND_BANDS_NUM];
}NV_ID_BANDXX_CAND_INFO_GROUP_STRU;
/* -nv-bands-cfg-end */

typedef struct
{
    VOS_UINT8                                       ucRcvSibRsrqFlg; /* 使用q值判断是否收sib标记 */
    VOS_UINT8                                       aucReserved1;
    VOS_UINT8                                       aucReserved2;
    VOS_UINT8                                       aucReserved3;
    VOS_UINT16                                      usGetGeoMinRsrpThreshold;
    VOS_UINT16                                      usMinRsrqRcvSibThreshold; /* Q值门限*/
    VOS_UINT16                                      usHighRsrpByRsrqRcvSib; /* Q值门限生效时P值上限 */
    VOS_UINT16                                      usLowRsrpByRsrqRcvSib;  /* Q值门限生效时P值下限 */

}LRRC_NV_CSEL_GET_SIB_THRESHOLD_STRU;
/* BEGIN DTS_DCM_IDLE_RESELECTION_CFG_2  2012-12-08 Modify*/
typedef struct
{
    PS_BOOL_ENUM_UINT8    ucOffSetEnableFlg; /* offset NV使能标记*/
    PS_BOOL_ENUM_UINT8    enR8BasedCellReselFlg;  /* 是否是基于R8的重选准则*/
    VOS_UINT8              aucReservel[2];         /* 保留位*/
    VOS_UINT16             usWRscpThresh;        /* W服务小区的RSCP门限，大于此门限才能使用offset，取绝对值*/
    VOS_UINT16             usWEcnoThresh;        /* W服务小区的ECNO门限，大于此门限才能使用offset，取绝对值*/
    VOS_UINT16             usEutranCellRsrpOffset;  /* EUTRAN小区的RSRP OFFSET*/
    VOS_UINT16             usEutranCellRsrqOffset;  /* EUTRAN小区的RSRQ OFFSET*/
    VOS_UINT16             usLteServRsrpOffSet;
    VOS_UINT16             usLteServRsrqOffSet;
    VOS_UINT16             usUtranRscpOffSet;
    VOS_UINT16             usUtranEcnoOffSet;
}NV_ID_APP_LRRC_RESEL_OFFSET_CFG_STRU;
/* END DTS_DCM_IDLE_RESELECTION_CFG_2  2012-12-08 Modify*/

/* -fast-dorm-cfg-3 */
#define LRRC_NV_CTRL_PARA_U8_MAX_LEN                   (16)
#define LRRC_NV_CTRL_PARA_U16_MAX_LEN                  (8)
#define LRRC_NV_CTRL_PARA_U32_MAX_LEN                  (8)

/* -fast-dorm-cfg-3 */
#define LRRC_CTRL_PARA_FAST_DORM_DELAY_BIT             (16)
#define LRRC_GET_CTRL_PARA_U8_INDEX( usIndex )         ( (usIndex))
#define LRRC_GET_CTRL_PARA_U16_INDEX( usIndex )        ( (usIndex)-LRRC_NV_CTRL_PARA_U8_MAX_LEN)
#define LRRC_GET_CTRL_PARA_U32_INDEX( usIndex )        ( (usIndex)-LRRC_NV_CTRL_PARA_U8_MAX_LEN - LRRC_NV_CTRL_PARA_U16_MAX_LEN )
#define LRRC_CTRL_PARA_IS_BIT_PRESENT( ulBitMap, ulBit )         ( (ulBitMap)&( 1<< (ulBit) ) )

typedef struct
{
    VOS_UINT32          bitValidMap ; /*BitMap(high--->low) U32,U16,U8,U8 */
    VOS_UINT8           astucCtrlPara[LRRC_NV_CTRL_PARA_U8_MAX_LEN];
    VOS_UINT16          astusCtrlPara[LRRC_NV_CTRL_PARA_U16_MAX_LEN];
    VOS_UINT32          astulCtrlPara[LRRC_NV_CTRL_PARA_U32_MAX_LEN];
}NV_ID_CTRL_PARA_STRU;

/* CMCC-BEGIN */
/*****************************************************************************
 结构名    : LTE_IRAT_REDIR_SWITCH_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 异系统重定向，CMCC G2L重定向4s需求控制开关和G2L自主重定向2s需求控制开关
            沙特 W2L重定向自主重定向2s需求控制开关数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                   ucG2LRedirFlag;                 /* G2L重定向4s需求控制开关 */
    VOS_UINT8                                   ucG2LAutoRedirFlag;             /* G2L自主重定向2s需求控制开关 */
    VOS_UINT8                                   ucW2LAutoRedirFlag;             /* W2L自主重定向2s需求控制开关 */
    VOS_UINT8                                   aucReservd1[5];
    VOS_UINT16                                  usG2LRedirTimeLen;              /* G2L重定向时长，默认值为4s，单位为ms */
    VOS_UINT16                                  usG2LAutoRedirTimeLen;          /* G2L自主重定向时长，默认值为2s，单位为ms */
    VOS_UINT16                                  usW2LAutoRedirTimeLen;          /* W2L自主重定向时长，默认值为2s，单位为ms */
    VOS_UINT8                                   aucReservd2[6];
}LTE_IRAT_REDIR_SWITCH_STRU;
/* CMCC-END */
/*****************************************************************************
 结构名    : FC_LTE_TEMPERATURE_LIMIT_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 定义LTE 温控配置结构
*****************************************************************************/
#define FC_UL_THROUGHPUT_MIN_COUNT_NV          (0x03)
typedef struct
{
    VOS_UINT32          ulZeroSupported;
    VOS_UINT32          ulMinThroughput[FC_UL_THROUGHPUT_MIN_COUNT_NV];
} FC_LTE_TEMPERATURE_LIMIT_CONFIG_STRU;

/*****************************************************************************
 结构名    : LTE_APT_TOTAL_SWITCH
 协议表格  :
 ASN.1描述 :
 结构说明  : LTE NV总开关
*****************************************************************************/
typedef struct
{
    VOS_UINT32          ulLteAptTotalSwitch;
    VOS_UINT32          ulRsv;
}LTE_APT_COMMONCTRL_STRU;

/* MTC add  begin */
#define     LTE_MTC_ABAN_BAND_NUM_MAX   (8)
#define     LTE_MTC_ABAN_PLMN_NUM_MAX   (8)
#define     LTE_MTC_SPAC_BAND_NUM_MAX   (8)

typedef struct
{
    VOS_UINT32                          ulMcc;              /* MCC,3 bytes      */
    VOS_UINT32                          ulMnc;              /* MNC,2 or 3 bytes */
} LRRC_PLMN_ID_STRU;

/* 扫频排除 band NV信息 */
typedef struct
{
    LRRC_PLMN_ID_STRU                 stPlmnID;
    VOS_UINT32                       ulBandNum;                                    /* 配置的Band个数 */
    VOS_UINT8                        ucBand[LTE_MTC_ABAN_BAND_NUM_MAX];            /* 记录不需要扫频的Band */
} LTE_MTC_PLMN_ABAN_BAND_LIST;

/*Delete FDD Band */
typedef struct
{
    VOS_UINT8                     ucSwitchFlag;                                   /* 功能总控开关,0 close; 1 open */
    VOS_UINT8                     ucReserved;
    VOS_UINT16                    usMcc;                                          /* 默认填写 460  */
}LTE_CUSTOM_MCC_INFO_STRU;

extern  LTE_CUSTOM_MCC_INFO_STRU g_stLRrcDelFddBand;
/*Delete FDD Band */

typedef struct
{
    VOS_UINT32                        ulPlmnNum;
    LTE_MTC_PLMN_ABAN_BAND_LIST       astAbandPlmnList[LTE_MTC_ABAN_PLMN_NUM_MAX]; /* Plmn和BandList */
} LTE_MTC_ABANDON_EUTRA_BAND_SCAN_STRU;


/* 扫频特殊band 设置信息 */
typedef struct
{
     VOS_UINT8                      ucBand;                                        /* 指示Band信息 */
     VOS_UINT8                      aucRsv[3];                                     /*保留位*/
     VOS_UINT16                     usFreqBegin;
     VOS_UINT16                     usFreqEnd;
}LTE_MTC_SPAC_BAND_CONFIG_LIST;

typedef struct
{
     VOS_UINT32                     ulBandNum;
     VOS_UINT32                     ulSpacBandSwitch;                               /* 0 close; 1 open*/
     LTE_MTC_SPAC_BAND_CONFIG_LIST  astSpacBandList[LTE_MTC_SPAC_BAND_NUM_MAX];     /* 记录需要扫频特殊配置的Band */
} LTE_MTC_SPAC_BAND_SCAN_CONFIG_STRU;
extern LTE_MTC_ABANDON_EUTRA_BAND_SCAN_STRU g_stAbandEutraBandInfo;
extern LTE_MTC_SPAC_BAND_SCAN_CONFIG_STRU   g_stSpacBandInfo;
/* MTC add  end */

/* MTC NOTCH add begin  */
/*****************************************************************************
 结构名    : LTE_MTC_NOTCH_FEATURE_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : aucRcve[0]用于建链时缓存的S值判断门限
*****************************************************************************/
typedef struct
{
     VOS_UINT16                     usSwitchFlag;                                   /* 功能总控开关 */
     VOS_UINT8                      aucRcve[2];                                     /* 保留位 */
} LTE_MTC_NOTCH_FEATURE_STRU;

/* MTC NOTCH add end  */
/*****************************************************************************
 结构名    : LTE_RRC_THRESHOLD_STRU
 协议表格  :
 ASN.1描述 :
        ulCdrxPeriod:使用Balong基线值
        lGeranBasiclag: 使用Balong基线值
        lGeranBasiStartThres:使用Balong基线值
        ulCdrxFilterSwitch:使用Balong基线值
        ulAbandinFristItraMeasFlg:丢弃第一次异系统测量结果，0:不丢弃。1:丢弃。Balong基线默认值:0.
        ulTdsT315Len:  T315定时器长度，单位秒。Balong基线默认值为0，产线需要自行设置，建议值 5.
        ulHrpdIdleMeasIndInterTime:hrpd上报空闲态测量Ind的时间间隔，单位毫秒.Balong基线默认值:200.
        ulHrpdConnMeasIndInterTime:hrpd上报连接态测量Ind的时间间隔，单位毫秒.Balong基线默认值:200.
        ultimerLrrcWaitCdmaCnfLen:等待hrpd重选、重定向结果IND定时器时长,单位毫秒.Balong基线默认值:30000.
        ulTReselectUtran: 自定义3G重选定时器长度.单位:秒。Balong基线默认值为0，产线需要自行设置，建议值 60.
        ulTReselectGsm: 自定义2G重选定时器长度.单位:秒。Balong基线默认值为0，产线需要自行设置，建议值 50.
 结构说明  : LTE RRC 阈值
*****************************************************************************/
/* sync from kitkat begin */
/* begin:Abandon Frist Itra Meas Result */
typedef struct
{
    VOS_UINT32          ulCdrxPeriod;
    VOS_INT32           lGeranBasiclag;
    VOS_INT32           lGeranBasiStartThres;
    VOS_UINT32          ulCdrxFilterSwitch;
    VOS_UINT32          ulAbandinFristItraMeasFlg;
    VOS_UINT32          ulTdsT315Len;
    VOS_UINT32          ulHrpdIdleMeasIndInterTime;
    VOS_UINT32          ulHrpdConnMeasIndInterTime;
    VOS_UINT32          ultimerLrrcWaitCdmaCnfLen;
    VOS_UINT32          ulTReselectUtran;
    VOS_UINT32          ulTReselectGsm;
    /* mod for FreqSearchEnhance begin */
    VOS_UINT32          ulFreqSearchEnhanceFlag;
    VOS_UINT32          ulFreqSearchEnhanceSrchCnt;
    VOS_UINT32          ulFreqSearchEnhanceSrchThres;
    /* mod for FreqSearchEnhance end */
    VOS_UINT32          ulTReselectCdma;
    /* GU高优先级RAT背景搜L，LTE搜网驻留IND的门限，单位-1/8 dB*/
    VOS_UINT32          ulBgsL2GuSearchIndThreshFlag;
    VOS_INT32           lBgsL2GuSearchIndThresh;
    /* GU高优先级RAT背景搜L，LTE搜网驻留IND的门限，单位-1/8 dB*/
    VOS_UINT32          ulCampFailRetryNum;
    VOS_INT32           lDelfaultqQualMin;/*SIB1中不含qQualMin时，使用一个默认值判断是否起同频测量*/
    /* mod for Balong CL begin */
    VOS_UINT32          ulWaitCasReselCnfLen;    /* 自研CL LTE等待CDMA重选CNF定时器时长 */
    VOS_UINT32          ulWaitCasRedirectCnfLen;    /* 自研CL LTE等待CDMA重定向CNF定时器时长 */
    /* mod for Balong CL end */
    /* begin: add for AC Bar Ctrl */
    VOS_UINT32          ulAcBarStudyTimeLen;           /*acbar的学习时长，单位s*/
    VOS_UINT32          ulAcBarNormalBarTimeLen;           /*学习时长内达到最大次数之后首次被BAR之后在该时间段内按正常acbar处理，单位s*/
    VOS_UINT32          ulAcNotBarNumforMoSignal;      /*ulAcBarStudyTimeLen长时间内，前ulAcNotBarNumforMoSignal次建链不Bar*/
    VOS_UINT32          ulAcNotBarNumforMoData;        /*ulAcBarStudyTimeLen长时间内，前ulAcNotBarNumforMoData次建链不Bar*/
    VOS_UINT32          ulAcNotBarNumforMoCsfb;        /*ulAcBarStudyTimeLen长时间内，前ulAcNotBarNumforMoCsfb次建链不Bar*/
    /* end: add for AC Bar Ctrl */
    /* Begin: 20150801 */
    VOS_UINT32          ulFrRecvSibRsrpFlag;   /* 设置GU FR 回LTE是否判断rsrp值标识 */
    VOS_UINT32          ulFrRecvSibRsrqFlag;   /* 设置GU FR 回LTE是否判断rsrp值标识*/
    VOS_UINT32          ulFrRecvSibRsrq;       /* 设置GU FR 回LTE是否判断rsrp值,设置范围【12 - 40】,代码自动换成负值 */
    VOS_UINT32          ulFrRecvSibRsrp;       /* 设置GU FR 回LTE是否判断rsrp值,设置范围【100 - 140】,代码自动换成负值 */
    VOS_UINT32          ulStopIratRedirTimerFlg;  /*自主FR停止定时器时机Flg */
    /* End: 20150801 */
    VOS_INT32           lRsrqMinforResel;

}LTE_RRC_THRESHOLD_STRU;
extern VOS_UINT32   g_ulAbandinFristItraMeasFlg;
/* end:Abandon Frist Itra Meas Result */
/* sync from kitkat end */

extern VOS_UINT8    g_ucTReselectUtran;
extern VOS_UINT8    g_ucTReselectGsm;
/* begin: add for cdma */
extern VOS_UINT8    g_ucTReselectCdma;
/* end: add for cdma */
extern VOS_UINT16    g_usNoRptSecondCellThres;

/* Irat TDS UE capability modify  begin */
#define         MAX_IRAT_TDS_UE_CAPABILITY_LENGHT           (100)
typedef struct
{
     VOS_UINT16                     usSwitchFlag;                                   /* 功能总控开关: 默认打开使用NV配置的ue能力码流，特殊情况下使用自己代码里的*/
     VOS_UINT16                     usLength;                                       /* UE能力码流长度 */
     VOS_UINT8                      aucCapInfo[MAX_IRAT_TDS_UE_CAPABILITY_LENGHT];                                /* UE能力码流 */
} LTE_IRAT_TDS_UE_CAPABILITY_STRU;
/* Irat TDS UE capability modify  end */
/*BEGIN modify for B28全频段特性*/


/*****************************************************************************
 结构名    : LRRC_NV_EXT_BAND_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 在不同带宽下的分段频段NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usBandInd;      /* 基本的BAND 号*/
    VOS_UINT16                          usScellBandInd;      /* Scell的BAND 号*/

	/* Band内候补带宽频段信息 */
    LRRC_NV_SUPPORT_BAND_LIST_STRU      astBandInfoWithBandWidth[6];/*RRC_BAND_WIDTH_BUTT*/
}LRRC_NV_EXT_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : LTE_RRC_EXTBANDINFO_WITH_BANDWIDTH_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE支持的频段在不同带宽下的分段频段NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8          ucActive;
    VOS_UINT8          ucScellEnable;
    VOS_UINT16         usBandCount;
    LRRC_NV_EXT_BAND_LIST_STRU          stCandBandsInfo[8]; /*支持最多8个频定制分频*/
}LTE_RRC_EXTBANDINFO_WITH_BANDWIDTH_STRU;

/*END modify for B28全频段特性*/
/*****************************************************************************
 结构名    : LRRC_NV_DSDS_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : DSDS特性配置NV
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usRsv0;
    VOS_UINT16                          usDsdsPagingTimeLen;
    VOS_UINT16                          usDsdsShrtOccpRfRcverTimeLen;
    VOS_UINT16                          usRsv;
    VOS_UINT32                          ulFrRelDelaytimer;
    VOS_UINT32                          ulRsv1;
    VOS_UINT32                          ulRsv2;
    VOS_UINT32                          ulRsv3;
}LRRC_NV_DSDS_CONFIG_STRU;
/* mod for AT&T program 2015-01-04 DTS begin */
/*****************************************************************************
 结构名    : LTE_RRC_DAM_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LRRC的DAM参数
*****************************************************************************/
typedef struct
{
    VOS_UINT32                           ulDamCRRCMaxCnt;           /* LRRC DAM功能CRRC最大值.有效范围1-60,默认值为5 */
    VOS_UINT32                           ulDamCellBarTimerLenth;    /* LRRC DAM功能小区被bar定时器最大值.有效范围1-60(分钟),默认值为6 */
}LRRC_DAM_PARA_STRU;

/*****************************************************************************
结构名称    :PS_NV_DAM_CONFIG_PARA_STRU
使用说明    :DAM需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                           bitOpLrrcDamSwitch:1;
    VOS_UINT32                           bitOpRsv          :31;
    VOS_UINT32                           ulCService;         /* 有效范围1-60，默认值5   */
    LRRC_DAM_PARA_STRU                   stLrrcDamPara;      /* LRRC的DAM相关参数       */
    NAS_DAM_CONFIG_PLMN_PARA             stDamConfigPlmnPara;/* DAM特性控制列表         */

    /* enable lte后是否触发搜网，0不触发，1触发 */
    VOS_UINT8                            ucEnableLteTrigPlmnSearchFlag;

    /* 在DAM网络 disbale LTE后是否将DAM的LTE网络加入禁止接入技术网络列表中，TRUE需要加入，FALSE不需要加入 */
    VOS_UINT8                            ucAddDamPlmnInDisablePlmnWithRatListFlag;

    /* Modified by for DAM , 2016-04-29, Begin */
    VOS_UINT8                            ucDamFlag[2];       /* DAM特性开关，1开启，0关闭，只使用[0]位置 */
    /* Modified by for DAM , 2016-04-29, End */

}PS_NV_DAM_CONFIG_PARA_STRU;
/* mod for AT&T program 2015-01-04 DTS end */

/* begin: add mbms for feature v700r500 , */
/*****************************************************************************
结构名称    :LRRC_DRX_ASSIST_INFO_R11_STRU
使用说明    :IDC需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           enDrxOffsetR11Flg;                      /*指示是否存在，PS_FALSE:不存在，PS_TRUE:存在*/
    VOS_UINT8                           enDrxCycleLenR11;
    VOS_UINT8                           ucDrxOffsetR11;                         /* optional */    /*(0..255)*/
    VOS_UINT8                           enDrxActTimeR11;
}LRRC_DRX_ASSIST_INFO_R11_STRU;

/*****************************************************************************
结构名称    :LRRC_IDC_SUBFRAME_PATTERN_R11_STRU
使用说明    :IDC需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucChoice;                               /*1:ucSubframePatternFddR11,2:Tdd SubframeCfg0R11,3:usSubframeCfg15R11,4:SubframeCfg6R11*/

#define SUBFRAME_PATTERN_FDD_CFG        1
#define SUBFRAME_PATTERN_TDD_CFG_0      2
#define SUBFRAME_PATTERN_TDD_CFG_1_5    3
#define SUBFRAME_PATTERN_TDD_CFG_6      4

    VOS_UINT8                           ucSubframePatternFddCfgR11;
    VOS_UINT8                           aucSubframeTddCfg0R11[LRRC_MAX_NUM_SUBFRAME_CONFIG0_R11];
    VOS_UINT8                           usRsv;
    VOS_UINT16                          usSubframeTddCfg15R11;
    VOS_UINT8                           ausRsv1[2];
    VOS_UINT8                           aucSubframeTddCfg6R11[LRRC_MAX_NUM_SUBFRAME_CONFIG6_R11];
}LRRC_IDC_SUBFRAME_PATTERN_R11_STRU;

/*****************************************************************************
结构名称    :LRRC_IDC_SUBFRAME_PATTERN_LIST_R11_STRU
使用说明    :IDC需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;    /*(0, 1..UU_MAX_NUM_IDC_SUBFRAME_PATTERNLIST_R11)*/
    VOS_UINT16                          usRsv;
    LRRC_IDC_SUBFRAME_PATTERN_R11_STRU  aItem[LRRC_MAX_NUM_IDC_SUBFRAME_PATTERNLIST];
}LRRC_IDC_SUBFRAME_PATTERN_LIST_R11_STRU;

/*****************************************************************************
结构名称    :LRRC_TDM_ASSIST_INFO_R11_STRU
使用说明    :IDC需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                 ucChoice;                         /*1:stDrxAssistInfoR11,2:stIdcSubframePatternListR11*/
    VOS_UINT8                                 aucReserved[3];

    LRRC_DRX_ASSIST_INFO_R11_STRU             stDrxAssistInfoR11;
    LRRC_IDC_SUBFRAME_PATTERN_LIST_R11_STRU   stIdcSubframePatternListR11;
}LRRC_TDM_ASSIST_INFO_R11_STRU;

/*****************************************************************************
结构名称    :PS_NV_IDC_CONFIG_PARA_STRU
使用说明    :IDC需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                                bitTdmAssistInfoR11Present :1;
    VOS_UINT32                                bitSpare                   :31;

    VOS_UINT8                                 ucInterferenceDirR11;             /*0:EUTRA,1:OTHER,2:BOTH*/
    VOS_UINT8                                 ucRsv[3];
    LRRC_TDM_ASSIST_INFO_R11_STRU             stTdmAssistInfoR11;
}PS_NV_IDC_CONFIG_PARA_STRU;
/* end:   add mbms for feature v700r500 , */

/* begin: add for Boston Phase3 ,  */

typedef struct
{
    VOS_UINT32    bitGpsR11Present                            : 1;
    VOS_UINT32    bitGlonassR11Present                        : 1;
    VOS_UINT32    bitBdsR11Present                            : 1;
    VOS_UINT32    bitGalileoR11Present                        : 1;
    VOS_UINT32    bitWlanR11Present                           : 1;
    VOS_UINT32    bitBluetoothR11Present                      : 1;
    VOS_UINT32    bitSpare                                    : 26;

    VOS_UINT16    usTxBegin;
    VOS_UINT16    usTxEnd;
    VOS_UINT16    usRxBegin;
    VOS_UINT16    usRxEnd;
} LRRC_IDC_UL_CA_PARA_STRU;


typedef struct
{
    VOS_UINT16                          usCnt;
    VOS_UINT16                          usRsv;
    LRRC_IDC_UL_CA_PARA_STRU            aItem[LRRC_MAX_NUM_IDC_UL_CA_PARALIST];
} PS_NV_IDC_UL_CA_PARA_LIST_STRU;

/* end: add for Boston Phase3 ,  */

/*****************************************************************************
结构名称    :PS_NV_OTDOA_ADDITIONAL_NCELL_INFO_STRU
使用说明    :otdoa需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                                bitAddOtdoaNeighbourCellInfoListFlag :1;
    VOS_UINT32                                bitUeRxTxTimeDiffMeasPresent         :1;
    VOS_UINT32                                bitSpare                             :30;

}PS_NV_OTDOA_ADDITIONAL_NCELL_INFO_STRU;

/*****************************************************************************
结构名称    :LRRC_ACCFAILBAR_CONFIG_STRU
使用说明    :无法接入的坏小区bar特性NV控制结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16   usAccFailEnableFlg;       /*是否使能此项功能*/
    VOS_UINT16   usOrigAccFailTimeLimit;   /*连续发生随机接入失败的统计时限，单位为s*/
    VOS_UINT16   usOrigAccFailCntLimit;    /*连续发生随机接入失败的次数门限*/
    /*也即在usOrigAccFailTimeLimit内发生usOrigAccFailCntLimit及以上的接入失败，Bar掉小区*/

    VOS_UINT16   usRsv1;


    VOS_UINT16   usBarStepCnt;             /*渐进式分成几步延长Bar时限，最大不能超过LRRC_ACCFAIL_MAX_BAR_STEP*/
    VOS_UINT16   ausBarTimeLen[LRRC_ACCFAIL_MAX_BAR_STEP]; /*单位为s*/

    VOS_UINT16   usQImproveRelBarTrhesh;   /*Q值变好门限，可触发解Bar*/
    VOS_UINT16   usPImproveRelBarTrhesh;   /*P值变好门限，可触发解Bar*/
    VOS_UINT16   usTimeLenBetweenBarStep;  /*渐进Bar之间的有效时长，即大于该时长归0*/

    VOS_UINT16   usRsv2;
    VOS_UINT16   usRsv3;
    VOS_UINT16   usRsv4;
    VOS_UINT32   ulRsv;
}LRRC_NV_ACCFAILBAR_CONFIG_STRU;
/* Begin: 2015/5/6 sbm */
/*****************************************************************************
 结构名    : LRRC_SBM_CUSTOM_DUAL_IMSI_STRU_NEW
 结构说明  : 软银双imsi开关和MCC信息
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucSbmCustomSrchFlg; /*  SBm 搜网 开关，LTE模使用，用于控制双IMSI方案和搜网方案*/
    VOS_UINT8                           aucReserve[3];          /* 保留位 */
    VOS_UINT32                          aulSbmSupBandInJapan[2];         /* 软银在日本国内支持的band，如果是全1则认为支持的band不做限制，支持UE支持的所有band */
    VOS_UINT32                          aulEmSupBandInJapan[2];          /* EM在日本国内支持的band，如果是全1则认为支持的band不做限制，支持UE支持的所有band */
    VOS_UINT32                          aulOtherOperateSupBandInJapan[2];/* 其他运营商在日本国内支持的band，如果是全1则认为支持的band不做限制，支持UE支持的所有band */
    VOS_UINT32                          aulSupBandOutJapan[2];           /* 日本国外支持的band,如果全1则认为支持的band不做限制，支持UE支持的所有band */
} LRRC_SBM_CUSTOM_DUAL_IMSI_STRU_NEW;
/* End: 2015/5/6 sbm */

/*****************************************************************************
 枚举名    : LRRC_NV_PLMN_CAND_FREQ_PRIOR_ENUM_UINT8
 协议表格  :
 ASN.1描述 :
 枚举说明  : TDD优先还是FDD优先
*****************************************************************************/
typedef enum
{
    LRRC_CELL_SEARCH_PRIOR_NO_PRIOR   = 0,
    LRRC_CELL_SEARCH_PRIOR_TDD_PRIOR  = 1,
    LRRC_CELL_SEARCH_PRIOR_FDD_PRIOR  = 2,
} LRRC_NV_PLMN_CAND_FREQ_PRIOR_ENUM;
typedef VOS_UINT8   LRRC_NV_PLMN_CAND_FREQ_PRIOR_ENUM_UINT8;

/*****************************************************************************
 枚举名    : LRRC_PLMN_CAND_INFO_TYPE_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : 动静态表类型
*****************************************************************************/
typedef enum
{
    LRRC_PLMN_CAND_INFO_FIXTBL_TYPE        = 0,
    LRRC_PLMN_CAND_INFO_DYNTBL_TYPE        = 1,
    LRRC_PLMN_CAND_INFO_TYPE_BUTT          = 2,
} LRRC_PLMN_CAND_INFO_TYPE_ENUM;
typedef VOS_UINT8   LRRC_PLMN_CAND_INFO_TYPE_ENUM_UINT8;

/*****************************************************************************
 结构名    : LRRC_NV_EACH_PLMN_CAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 单个PLMN候补信息
*****************************************************************************/
typedef struct
{
    LRRC_LNAS_PLMN_ID_STRU  stNasFormatPlmn;
    VOS_UINT16              ausPlmnBand[LRRC_SINGLE_CAND_PLMN_MAX_BAND];
    VOS_UINT32              aulEarfcn[LRRC_SINGLE_CAND_PLMN_MAX_FRQNUM];

    VOS_UINT32              ucBitSwitch              :1;/* plmn定制开关，默认值0。      值0，关闭；值1，打开*/
    VOS_UINT32              ucBitW2LSrch             :3;/* L从模，W2L搜网，默认值1。    值0，只搜历史频点+预置频点；值1，搜预置频段*/
    VOS_UINT32              ucBitRedirSrch           :3;/* 重定向搜网，默认值0。        值0，只搜历史频点+预置频点；值1，搜预置频段*/
    VOS_UINT32              ucBitReestSrch           :3;/* 重建立搜网，默认值1。        值0，只搜历史频点+预置频点；值1，搜预置频段*/
    VOS_UINT32              ucBitOosSrch             :3;/* 出服务区上报前搜网，默认值0。值0，只搜历史频点+预置频点；值1，搜预置频段，值2，搜全频段；值6，不搜网直接报出服务区*/
    VOS_UINT32              ucBitAsSrch              :3;/* 其他AS搜网，默认值0。        值0，只搜历史频点+预置频点；值1，搜预置频段；conn->idle*/
    VOS_UINT32              ucBitAddMFreq            :1;/* 针对单个运营商定制,扫频是否添加测量频点到先验列表,0,添加；1,不添加，默认0*/
    VOS_UINT32              ucBitRsv4                :1;
    VOS_UINT32              ucBitRsv5                :1;
    VOS_UINT32              ucBitRsv6                :1;
    VOS_UINT32              ucBitRsv7                :1;
    VOS_UINT32              ucBitRsv8                :1;
    VOS_UINT32              ucBitRsv9                :1;
    VOS_UINT32              ucBitRsv10               :1;
    VOS_UINT32              ucBitRsv11               :2;
    VOS_UINT32              usBitRsv0                :6;

    VOS_INT16               sThresh0;                    /*预留的门限*/
    VOS_INT16               sThresh1;                    /*预留的门限*/
}LRRC_NV_SINGLE_PLMN_CAND_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CAND_FREQ_INFO_WITHPLMN
 协议表格  :
 ASN.1描述 :
 结构说明  : 单个PLMN候补信息
*****************************************************************************/
typedef struct
{
    LRRC_LNAS_PLMN_ID_STRU  stNasFormatPlmn;
    VOS_UINT16              usBand;
    VOS_UINT8               ucRsv0;
    VOS_UINT8               ucRsv1;
    VOS_UINT16              usRsv;
    VOS_UINT16              usRsv1;
    VOS_UINT32              ulEarfcn;
}LRRC_NV_CAND_FREQ_INFO_WITHPLMN_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_FIX_MUTI_PLMNS_CAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 固定占用的PLMN的先验信息，大T规划明确的运营商信息
*****************************************************************************/
typedef struct
{
    VOS_UINT16              ulValidNum;
    VOS_UINT8               ucCandInfoForEplmn;      /*先验信息是否考虑Eplmn列表*/
    VOS_UINT8               ucRsv1;
    VOS_UINT16              sRsv0;
    VOS_UINT16              sRsv1;

    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;

    VOS_UINT32              ucBitW2LSrch                  :3;/* L从模，W2L搜网，默认值1。    值0，只搜历史频点+预置频点；值1，搜预置频段*/
    VOS_UINT32              ucBitRedirSrch                :3;/* 重定向搜网，默认值0。        值0，只搜历史频点+预置频点；值1，搜预置频段*/
    VOS_UINT32              ucBitReestSrch                :3;/* 重建立搜网，默认值1。        值0，只搜历史频点+预置频点；值1，搜预置频段*/
    VOS_UINT32              ucBitAsSrch                   :3;/* 其他AS搜网，默认值0。        值0，只搜历史频点+预置频点；值1，搜预置频段；conn->idle*/
    VOS_UINT32              ucBitOosSrch                  :3;/* 出服务区上报前搜网，默认值0。 值0，只搜历史频点+预置频点；值1，搜预置频段，值2，搜全频段；值6，不搜网直接报出服务区*/
    VOS_UINT32              ucBitRsv5                     :1;
    VOS_UINT32              ucBitRsv6                     :1;
    VOS_UINT32              ucBitRsv7                     :1;
    VOS_UINT32              ucBitRsv8                     :1;
    VOS_UINT32              ucBitRsv9                     :2;
    VOS_UINT32              ucBitRsv10                    :3;
    VOS_UINT32              ucBitRsv11                    :4;
    VOS_UINT32              usBitRsv0                     :4;

    VOS_UINT32              ulRsv;

    LRRC_NV_SINGLE_PLMN_CAND_INFO_STRU  astPlmnCand[LRRC_CAND_PLMNFREQ_MAX_PLMNNUM];
}LRRC_NV_FIX_MUTI_PLMNS_CAND_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_DYNAMIC_MUTI_PLMNS_CAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 动态的PLMN的先验信息
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucRsv0;
    VOS_UINT8               ucRsv1;
    VOS_UINT16              usRsv0;
    VOS_UINT16              sRsv0;
    VOS_UINT16              sRsv1;

    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;

    VOS_UINT32              ucBitRsv1                :1;  /* 接入层发起的搜索,OOS等是否进行 band搜索*/
    VOS_UINT32              ucBitRsv2                :1;  /* 接入层发起的搜索,OOS等如果扫频是否只扫已知频段*/
    VOS_UINT32              ucBitRsv3                :1;  /* 重建立不扫频，只搜历史*/
    VOS_UINT32              ucBitRsv                 :1;  /* 重建立扫频只扫已知频段*/
    VOS_UINT32              ucBitRsv4                :1;
    VOS_UINT32              ucBitRsv5                :1;
    VOS_UINT32              ucBitRsv6                :1;
    VOS_UINT32              ucBitRsv7                :1;
    VOS_UINT32              ucBitRsv8                :1;
    VOS_UINT32              ucBitRsv9                :1;
    VOS_UINT32              ucBitRsv10               :4;
    VOS_UINT32              ucBitRsv11               :4;
    VOS_UINT32              usBitRsv0                :10;

    VOS_UINT32              ulRsv;

    LRRC_NV_SINGLE_PLMN_CAND_INFO_STRU      astPlmnCand[LRRC_CAND_PLMNFREQ_MAX_PLMNNUM];
}LRRC_NV_DYNAMIC_MUTI_PLMNS_CAND_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_LATEST_CAND_FREQ_INFO_WITHPLMN_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 最近保存的频点，这些频点的PLMN有可能是不确定的，来自:
             1.系统消息中的异频频点
             2.本系统测量配置
             3.异系统测量
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucAddMeasFreq; /* 针对所有运营商,扫频是否添加测量频点到先验列表,1,不添加；0,添加，默认0*/
    VOS_UINT8               ucRsv1;
    VOS_UINT16              usRsv1;

    VOS_UINT16              sRsv0;
    VOS_UINT16              sRsv1;

    VOS_UINT32              ulRsv;

    LRRC_NV_SINGLE_PLMN_CAND_INFO_STRU    stLatestServPlmnCandFreq; /*最后驻留的服务区的信息*/
    LRRC_NV_CAND_FREQ_INFO_WITHPLMN_STRU  astLastestCandFreq[LRRC_LATEST_MAX_CAND_FREQ_NUM];    /*异频异系统配置中最近的PLMN尚不能完全确定的频点*/
}LRRC_NV_LATEST_CAND_FREQ_INFO_WITHPLMN_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CELL_GLOBAL_ID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : EUTRA CELL GLOBAL CELL ID
*****************************************************************************/
typedef struct
{
    LRRC_LNAS_PLMN_ID_STRU              stPlmnId;                    /* SIB1中广播的PLMN列表中第一个PLMN */
    VOS_UINT32                          ulCellIdentity;
} LRRC_NV_CELL_GLOBAL_ID_STRU;

/*Begin: add by CSG */
/*****************************************************************************
 结构名    : LRRC_NV_CSG_NEIGH_MACRO_CELL_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存放CSG小区的邻近宏小区ECGI
*****************************************************************************/
typedef struct
{
    LRRC_NV_CELL_GLOBAL_ID_STRU        astCellGlobalId[LRRC_CSG_NEIGH_MACRO_CELL_NUM];
}LRRC_NV_CSG_NEIGH_MACRO_CELL_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CSG_CAMPED_CELL_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存放最近驻留过的CSG小区
*****************************************************************************/
typedef struct
{
    VOS_UINT16                  usPhyCellId;
    VOS_UINT16                  usBandInd;
    VOS_UINT32                  ulFreqInfo;
    VOS_UINT32                  ulCsgId;

    LRRC_LNAS_PLMN_ID_STRU      stCampedPlmn;        /* 写代码注意，此PLMN可能与下面CGI中的PLMN不是同一个PLMN */
    LRRC_NV_CELL_GLOBAL_ID_STRU stCellGlobalId;      /* 驻留小区的CGI信息 */

    VOS_UINT32                  ulNeighMacroCellNum;    /* CSG小区相邻的CSG宏小区数目 */
}LRRC_NV_CSG_CAMPED_CELL_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CSG_HISTORY_CAMPED_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : CSG历史信息，保存UE成功驻留的小区PLMN信息
*****************************************************************************/
typedef struct
{
    VOS_UINT32                         ulCsgCampedCellNum;    /* 成功驻留的小区数目 */
    /* UE驻留的CSG小区信息 索引值越小表示该条目越新*/
    LRRC_NV_CSG_CAMPED_CELL_INFO_STRU  astCsgCampedCellInfo[LRRC_CSG_HISTORY_CAMPED_CELL_NUM];
}LRRC_NV_CSG_HISTORY_CAMPED_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CSG_NEIGH_MACRO_CELL_PRAT_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LRRC_NV_CSG_NEIGH_MACRO_CELL_PRAT_INFO_STRU
*****************************************************************************/
typedef struct
{
    LRRC_NV_CSG_NEIGH_MACRO_CELL_INFO_STRU       astNeighborMacroCell[LRRC_CSG_HISTORY_CAMPED_CELL_PART_NUM];
}LRRC_NV_CSG_NEIGH_MACRO_CELL_PRAT_INFO_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CSG_HISTORY_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存放CSG相关的先验信息:驻留过的CSG LTE小区以及其邻近的LTE宏小区
*****************************************************************************/
typedef struct
{
    /* 驻留过的CSG小区 */
    LRRC_NV_CSG_HISTORY_CAMPED_INFO_STRU         stCampedCsgCell;
    /* CSG小区的邻近宏小区 索引值越小表示该条目越新 */
    LRRC_NV_CSG_NEIGH_MACRO_CELL_INFO_STRU       astNeighborMacroCell[LRRC_CSG_HISTORY_CAMPED_CELL_NUM];
}LRRC_NV_CSG_HISTORY_INFO_STRU;
/*End: add by CSG */

/*****************************************************************************
 结构名    : RRC_CSEL_MOBILITY_PARAM_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 小区的移动性参数
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usHighTReselectionSf;                   /* 量化后进入高移动性状态后重选定时器的尺度因子 */
    VOS_UINT16                          usMediumTReselectionSf;                 /* 量化后进入中移动性状态后重选定时器的尺度因子 */
    VOS_INT16                           sHighQHystSf;                           /* 进入高移动性状态后QHyst的尺度因子 */
    VOS_INT16                           sMediumQHystSf;                         /* 进入中移动性状态后QHyst的尺度因子 */
} RRC_CSEL_MOBILITY_PARAM_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CSG_ASF_PARAMETERS_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存放CSG ASF相关的参数
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8                  enEnableIdleAsf;                        /* LTE IDLE ASF使能开关 */
    PS_BOOL_ENUM_UINT8                  enAvoidCsgPingPongResel;                /* 防CSG乒乓重选开关 */
    VOS_UINT16                          usMeasBandWidth;                        /* 测量带宽 */

    PS_BOOL_ENUM_UINT8                  enPresenceAntennaPort1;                 /* 天线信息 */
    VOS_UINT8                           ucNeighCellCfg;
    VOS_INT16                           sQOffset;                               /* QHyst */

    VOS_UINT16                          usTReselection;                         /* 同频小区重选定时器 ms */
    VOS_INT16                           sQRxLevMin;

    VOS_INT16                           sPMax;
    VOS_INT16                           sPCompensation;                         /* 根据 P-Max 和 UE Max Tx power 推算出来的 PCompensation */

    RRC_CSEL_MOBILITY_PARAM_STRU        stMobilityParam;                        /* 同频移动性参数 */

    VOS_INT16                           sQqualMin;                              /* S准则RSRQ评估参数 */
    /*  非标功能:
            1、重选评估时，如果ASF小区与同频下最好小区的质量差值小于usIdleAsfMaxDiffToBest，判定为满足重选要求
            usIdleAsf_MaxDifferToBest为0，相当于关闭此处理
            2、按高优先级评估异频ASF小区时
    */
    VOS_UINT16                          usIdleAsfMaxDiffToBest;

    VOS_INT16                           sRsrqQualityThre;                       /* ASF目标小区最低的LTE RSRQ质量门限要求 */
    VOS_UINT16                          usBaseLengthOfIdleAsfTimer;             /* IdleAsfTimer的基本时长 */

    VOS_UINT16                          usStepSizeOfIdleAsfTimer;               /* IdleAsfTimer的调整步长 */
    /* IdleAsfTimer最长时间为(usBaseLengthOfIdleAsfTimer + usStepSizeOfIdleAsfTimer *  ucMaxStepOfIdleAsfTimer)秒 */
    VOS_UINT8                           ucMaxStepOfIdleAsfTimer;                /* IdleAsfTimer时长的最大调整步数 */
    /* CSG附加测量定时器时长为(N * 寻呼周期) */
    VOS_UINT8                           ucNForCsgAddMeasureTimer;               /* CSG附加测量定时器用的参数N */
    VOS_UINT16                          usConnAsfTimerLen;                  /* 连接态自主搜定时器时长，单位秒，大于等于5 */
    /* 连接态自主搜异频测量开关,1表示打开执行异频自主频点测量,关闭则依赖网侧异频obj配置测量 */
    PS_BOOL_ENUM_UINT8                  enEnableConnAsfInterMeas;
    /* AT&T定制需求
        · In the case of a CSG Aware UICC:
            o ASF shall not be triggered if the EF-Operator CSG Lists is empty i.e. does not contain any CSG IDs
            o ASF shall not be triggered when the UE is registered on a PLMN that is not listed as a “PLMN” in the
                CSG Information TLV object of the EF-Operator CSG Lists
            o ASF shall only be triggered when the UE is registered on a PLMN that is listed as a “PLMN” in the
                CSG Information TLV object of the EF-Operator CSG Lists
        · In the case of a CSG Not Aware UICC:
            o ASF shall not be triggered if the UE based allowed CSG List is empty i.e. does not contain any CSG IDs
            o ASF shall not be triggered when the UE is registered on a PLMN that is not a PLMN of a fingerprinted CSG ID
            o ASF shall only be triggered when the UE is registered on a PLMN that is a PLMN of a fingerprinted CSD ID
    */
    PS_BOOL_ENUM_UINT8                  enAttAsfPlmnLimit;                      /* AT&T ASF定制开关 */
}LRRC_NV_CSG_ASF_PARAMETERS_STRU;


/*****************************************************************************
 结构名    : TLRRC_NV_CTRL_SUBFUNC_COMM_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 子功能公共控制结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucSwitch;
    VOS_UINT8               ucRsv1;
    VOS_UINT16              usRsv0;
    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;
    VOS_UINT16              usRsv5;
    VOS_UINT16              usRsv6;
    VOS_UINT32              ulRsv0;
}TLRRC_NV_CTRL_SUBFUNC_COMM_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_RESTRICTED_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 受限Band NV配置
*****************************************************************************/
typedef struct
{
    VOS_UINT32              aulBand[LRRC_RESTRICTED_BAND_ARRAY_NUM];
}LRRC_NV_RESTRICTED_BAND_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CELL_SELECT_RSSI_SORT_CTRL
 协议表格  :
 ASN.1描述 :
 结构说明  : 小区选择、小区搜索、驻留NV控制结够
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucValidFlg;
    VOS_UINT8               ucEnable;
    VOS_INT16               sRssiHighPriThre;
    VOS_INT16               sRssiHighPriDiffThre;
    VOS_UINT16              usSortFreqMaxNum;
    VOS_UINT16              usRedirectSortNum;
    VOS_UINT16              usPssRatioThres;
    VOS_INT16               sHighPssRatioRssiThre;  /* PSS RATIO最佳的小区RSSI只要高于这个值,就优选 */
    VOS_UINT16              usHighPssRatioRssiDiffThres;
    VOS_INT32               sHighPssRatioLowRssiThre;
    VOS_UINT16              usNoNoisePssRatioThres; /* 无噪场景PssRatio阈值 */
    VOS_UINT16              usNoNoisePssRatioDiffThres;/* 无噪场景，最大PssRatio与最大Rssi频点的PssRatio差值阈值 */
    VOS_INT16               sFddSrchFristRssiThre;/*联合排序后，电信开机指定搜首先搜FDD频点所对应的rssi门限*/
    VOS_UINT16              sFddSrchFristPssRatioThre;/*联合排序后，电信开机指定搜首先搜FDD频点所对应的PssRatio门限*/
    VOS_UINT32              ulRsv0;
    VOS_UINT32              ulRsv1;
}LRRC_NV_CELL_SELECT_RSSI_SORT_CTRL;

/*****************************************************************************
 结构名    : LRRC_NV_CELL_SELECT_RSSI_SORT_CTRL
 协议表格  :
 ASN.1描述 :
 结构说明  : CSG特性搜网参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucValidFlg;
    VOS_UINT8               ucRsv0;
    VOS_INT16               sRsv0;
    VOS_INT16               sRsv1;
    VOS_UINT16              usCsgListNeighNum;    /* CSG列表搜搜次强次数 */
    VOS_UINT16              usCsgSpecNeighNum;    /* CSG指定搜次强次数 */
    VOS_UINT16              usRsv0;
    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;
    VOS_UINT32              ulRsv3;
}LRRC_NV_CSG_FEATURE_CTRL;

/*****************************************************************************
 结构名    : LRRC_NV_INTER_OPT_FREQENTLY_CTRL
 协议表格  :
 ASN.1描述 :
 结构说明  : 频繁互操作抑制
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucTotalSwitch;                   /* 总开关 */
    VOS_UINT8               ucComplicatedIntOptSwich;        /* 复杂频繁互操作是否抑制 */
    VOS_INT16               sRsrpHighQualityThre;            /* LTE RSRP高质量门限 */
    VOS_INT16               sRsrqHighQualityThre;            /* LTE RSRQ高质量门限 */
    VOS_INT16               sGutHighQualityThre;             /* G RSSI/WT RSCP高质量门限 */
    VOS_INT16               sEneryDiffThre;                 /* 正值，RSRP/RSSI/RSCP差值门限，高于此值认为明显变化 */
    VOS_INT16               sRsrqDiffThre;                  /* 正值，RSRQ差值门限，高于此值认为明显变化 */
    VOS_UINT16              usBarOptTimerLen;                /* 抑制时长 */
    VOS_UINT16              usEvalueTimerLen;                /* 评估时长，超出此时长，认为互操作间发生时间够长，不属于频繁 */
    VOS_UINT16              usInvalidTimerLenTwoOpt;         /* 两次互操作间评估时长，超出此时长，认为互操作间发生时间够长，不属于频繁 */
    VOS_UINT16              usTwoCellOptTimerLen;            /* 在usTwoCellOptTimerLen间发生4次两两互操作，认为发生简单乒乓 */
    VOS_UINT16              usIratTwoCellOptTimerLen;        /* 在usIratTwoCellOptTimerLen间发生3次异系统两两互操作，认为发生简单乒乓 */
    VOS_UINT16              enReslParaAdjustToCtlRselFreq;   /* 是否调整重选参数 */
    VOS_UINT16              enReslParaAdjustHighSpeed;       /* 高铁是否同样进行重选参数调整 */
    VOS_INT16               sIntraReselMinDiff;             /* 同频重选时，邻区相对主区最小能量差值，缺省2db */
    VOS_UINT16              usReselTmrMin;                   /* ReselTmr最小值,ms */
    VOS_UINT16              ulRsv0;
    VOS_UINT32              ulRsv1;
    VOS_UINT16              ulRsv2;
    VOS_UINT16              ulRsv3;
}LRRC_NV_INTER_OPT_FREQENTLY_CTRL;

/*begin,add for CSFB B1 REPORT,  */
/*****************************************************************************
 结构名    : TLRRC_NV_LOC_B1_SWITCH_AND_THRETH
 协议表格  :
 ASN.1描述 :
 结构说明  : CSFB场景B1的私有门限
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucCsfbB1Switch;
    VOS_UINT8               ucRsv1;
    VOS_INT16               sB1UtraRscp;         /* B1事件私有的UTRA的RSCP最低上报门限，单位1dB*/
    VOS_INT16               sB1UtraEcn0;         /* B1事件私有的UTRA的Ecn0最低上报门限，单位0.5dB*/
    VOS_INT16               sB1GeranRssi;        /* B1事件私有的GERAN的RSSI最低上报门限，单位1dB*/
    VOS_UINT16              usRsv0;
    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT32              ulRsv0;
}LRRC_NV_PRIVATE_B1_SWITCH_AND_THRETH_STRU;
/*end,add for CSFB B1 REPORT,  */

/* begin,  ,add for VOLTE B2 */
/*****************************************************************************
 结构名    : LRRC_NV_VOLTE_A2B2_SWITCH_AND_THRETH_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : VOLTE场景A2和B2的私有门限
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucVolteA2Switch;     /* VOLTE场景使用私有A2门限的开关 */
    VOS_UINT8               ucVolteB2Switch;     /* VOLTE场景使用私有B2门限的开关 */
    VOS_INT16               sA2RsrpThres;        /* A2 RSRP门限, 单位1dB*/
    VOS_INT16               sA2RsrqThres;        /* A2 RSRQ门限, 单位1dB*/
    VOS_INT16               sB2RsrpThres;        /* B2 RSRP门限, 单位1dB*/
    VOS_INT16               sB2RsrqThres;        /* B2 RSRQ门限, 单位1dB*/
    VOS_INT16               sB2UtraRscp;         /* B2事件私有的UTRA的RSCP门限，单位1dB*/
    VOS_INT16               sB2UtraEcn0;         /* B2事件私有的UTRA的Ecn0门限，单位0.5dB*/
    VOS_INT16               sB2GeranRssi;        /* B2事件私有的GERAN的RSSI门限，单位1dB*/
    VOS_UINT32              ulRsv0;
}LRRC_NV_VOLTE_A2B2_SWITCH_AND_THRETH_STRU;
/* end,  ,add for VOLTE B2 */

/* begin,  ,add for VOLTE */
/*****************************************************************************
 结构名    : LRRC_NV_VOLTE_OR_CSFB_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 发起VOLTE还是CSFB的控制开关
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucTotalFeatureSwitch;     /* 整个特性是否生效开关 */
    VOS_UINT8               ucUSASwitch;              /* 美国特性打开，则不判断邻区，只要服务小区低于门限，则发起CSFB;需要ucTotalFeatureSwitch打开 */
    VOS_INT16               sRsrpThres;               /* RSRP门限, 单位1dB*/
    VOS_INT16               sRsrqThres;               /* RSRQ门限, 单位1dB*/
    VOS_INT16               sUtraRscp;                /* UTRA的RSCP门限，单位1dB*/
    VOS_INT16               sUtraEcn0;                /* UTRA的Ecn0门限，单位0.5dB*/
    VOS_INT16               sGeranRssi;               /* GERAN的RSSI门限，单位1dB*/
    VOS_INT16               usRsv0;
    VOS_INT16               usRsv1;
    VOS_UINT32              ulRsv0;
}LRRC_NV_VOLTE_OR_CSFB_CTRL_STRU;
/* end,  ,add for VOLTE */

/*****************************************************************************
 结构名    : LRRC_NV_VOLTE_B2_PRA2_SWITCH_AND_THRETH_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 子功能公共控制结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucVolteB2Pra2Switch;
    VOS_UINT8               ucRsv1;
    VOS_INT16               sB2UtraMinRscp;      /* B2事件私有的UTRA的RSCP最低门限，单位1dB*/
    VOS_INT16               sB2UtraMinEcn0;      /* B2事件私有的UTRA的Ecn0最低门限，单位0.5dB*/
    VOS_INT16               sB2GeranMinRssi;     /* B2事件私有的GERAN的RSSI最低门限，单位1dB*/
    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;
    VOS_UINT32              ulRsv5;
}LRRC_NV_VOLTE_B2_PRA2_SWITCH_AND_THRETH_STRU;

/* Volte Evs重协商特性,begin */
/*****************************************************************************
 结构名    : LRRC_NV_VOLTE_EVS_PARA_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 子功能公共控制结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucVolteEvsCtrlSwitch; /* EVS冲协商特性LRRC功能生效控制开关 */
    VOS_UINT8               ucRsv1;
    VOS_INT16               sVolteEvsCtrlRsrp;      /* 起呼时IMS判断是否采用EVS编码的RSRP门限，单位1dB*/
    VOS_INT16               sVolteEvsCtrlRsrq;      /* 起呼时IMS判断是否采用EVS编码的RSRP门限，单位1dB*/
    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;
    VOS_UINT16              usRsv5;
    VOS_UINT32              ulRsv0;
}LRRC_NV_VOLTE_EVS_PARA_CTRL_STRU;
/* Volte Evs重协商特性,end */


/*****************************************************************************
 结构名    : LRRC_NV_ID_M_FEATURE_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 美国特性的控制开关，用于产品线定制
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8      ucConnPagingSwitch; /* 美国特性，Verizon定制需求: 连接态收paging特性开关 */
    VOS_UINT8               ucDlSupervisonFailSwitch; /* 美国特性，Verizon定制需求: downlink supervision failure特性开关 */
    VOS_UINT8               ucSib16TimeUpdateSwitch;  /* 美国特性，Verizon定制需求: SIB16获取进行时间更新开关 */
    /* begin, No sim not camp at cell which not set ims-emergency-suppot,add by   */
    VOS_UINT8               ucNoUsimCheckImsEmergSuppSwitch; /* 美国特性，无卡状态不在不支持IMS紧急呼小区驻留开关 */
    /* end, No sim not camp at cell which not set ims-emergency-suppot,add by   */
    VOS_UINT8               ucSwitch4;
    VOS_UINT8               ucSwitch5;
    VOS_UINT8               ucSwitch6;
    VOS_UINT8               ucSwitch7;

    VOS_INT16               sUSADeltaRsrp;          /* 美国特性，AT&T定制需求: RSRP测量偏高，测量IND需要增加一个Delta值，默认值使用0 */
    VOS_UINT16              usPagingValidTime;      /* 美国特性，Verizon定制需求: 连接态不处理paging时延，默认为5s,范围为0-10s */
    VOS_UINT16              ucDlSupervisonFailTime; /* 美国特性，Verizon定制需求: downlink supervision failure特性的时间长度，默认5s，范围0-10s */
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;
    VOS_UINT16              usRsv5;
    VOS_UINT16              usRsv6;
    VOS_UINT16              usRsv7;
    VOS_UINT16              usRsv8;
    VOS_UINT16              usRsv9;
    VOS_UINT16              usRsv10;
    VOS_UINT16              usRsv11;
    VOS_UINT16              usRsv12;
    VOS_UINT16              usRsv13;
    VOS_UINT16              usRsv14;
    VOS_UINT16              usRsv15;

    VOS_UINT32              ulRsv0;
    VOS_UINT32              ulRsv1;
    LRRC_NV_RESTRICTED_BAND_STRU        stRestrictedBand;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv1;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv2;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv3;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv4;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv5;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv6;
}LRRC_NV_ID_M_FEATURE_CTRL_STRU;

/*****************************************************************************
 结构名    : LPS_NV_OM_VOLTE_FAULT_CTRL_CTRL
 协议表格  :
 ASN.1描述 :
 结构说明  : Volte自诊断控制
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucVtTotalSwitch;           /* 自诊断总开关 */
    VOS_UINT8               ucVtChrRtpSwitch;          /* CHR上报开关 */
    VOS_UINT8               ucDelayAnTime;             /* 延迟分析时间，即问题时间点延后多少时间作为问题段 */
    VOS_UINT8               ucPreDelayAnTime;          /* 前置延迟分析时间，即问题时间前多少时间作为问题，单位s */
    VOS_UINT16              usConPktLostThrUnitTime;   /* 单位时间连续丢包门限，低于此门限不认为是问题 */
    VOS_UINT16              usAccuPktLostThrUnitTime;  /* 单位时间连续丢包门限，低于此门限不认为是问题*/
    VOS_UINT16              usJitterThrUnitTime;       /* 抖动门限，低于此门限不认为是问题*/
    VOS_UINT16              usL2TrigPktLostThr;        /* L2触发的丢包门限 */
    VOS_UINT16              usL2TrigJitterThr;         /* L2触发的抖动门限 */
    VOS_INT16               sLowRsrqThre;              /* 弱信号Rsrq门限 */
    VOS_INT16               sLowRsrpThre;              /* 弱信号Rsrp门限 */
    VOS_UINT16              usChrRptInterval;          /* CHR上报间隔控制，单位小时 */
    VOS_UINT16              usGenStatRptInterval;      /* 一般性统计CHR上报间隔 */
    VOS_UINT16              usGenStatCallLen;          /* 通话时长大于多少上报统计，分钟 */
    VOS_UINT16              usGenStatFaultNum;         /* 发生多少次异常  */
    VOS_UINT16              usRsv7;
    VOS_UINT32              ulRsv0;
}LPS_NV_OM_VOLTE_FAULT_CTRL_CTRL;

/*****************************************************************************
 结构名    : LPS_NV_OM_FEATURE_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议OM、自诊断、CHR的开关、控制参数等
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucTFreqCollection; /* TDS频点收集Chr开关 */
    VOS_UINT8               ucSwitch1;
    VOS_UINT8               ucSwitch2;
    VOS_UINT8               ucSwitch3;
    VOS_UINT8               ucSwitch4;
    VOS_UINT8               ucSwitch5;
    VOS_UINT8               ucSwitch6;
    VOS_UINT8               ucSwitch7;

    VOS_UINT16              usRsv0;
    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;
    VOS_UINT16              usRsv5;
    VOS_UINT16              usRsv6;
    VOS_UINT16              usRsv7;
    VOS_UINT16              usRsv8;
    VOS_UINT16              usRsv9;
    VOS_UINT16              usRsv10;
    VOS_UINT16              usRsv11;
    VOS_UINT16              usRsv12;
    VOS_UINT16              usRsv13;
    VOS_UINT16              usRsv14;
    VOS_UINT16              usRsv15;

    VOS_UINT32              ulRsv0;
    VOS_UINT32              ulRsv1;

    LPS_NV_OM_VOLTE_FAULT_CTRL_CTRL     stVolteFaultInd;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv1;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv2;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv3;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv4;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv5;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv6;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv7;
}LPS_NV_OM_FEATURE_CTRL_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CELL_SELECT_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 小区选择、小区搜索、驻留NV控制结够
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucRemoveBarredSwitch;         /* 广播接收超时，小区被bar，能量变好，小区解bar功能开关*/
    VOS_UINT8               ucRemoveDupFreqAndBandSwitch; /* 删除重复频点频段功能开关*/
    VOS_UINT8               ucSwitch2;
    VOS_UINT8               ucSwitch3;
    VOS_UINT8               ucSwitch4;
    VOS_UINT8               ucSwitch5;
    VOS_UINT8               ucSwitch6;
    VOS_UINT8               ucSwitch7;

    VOS_INT16               sBarredDeltaRsrp;             /* 搜网小区RSRP比被bar时小区好sDeltaRsrp个DB时解bar*/
    VOS_INT16               sBarredDeltaRsrq;             /* 搜网小区RSRQ比被bar时小区好sDeltaRsrq个DB时解bar*/
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;
    VOS_UINT16              usRsv5;
    VOS_UINT16              usRsv6;
    VOS_UINT16              usRsv7;
    VOS_UINT16              usRsv8;
    VOS_UINT16              usRsv9;
    VOS_UINT16              usRsv10;
    VOS_UINT16              usRsv11;
    VOS_UINT16              usRsv12;
    VOS_UINT16              usRsv13;
    VOS_UINT16              usRsv14;
    VOS_UINT16              usRsv15;

    VOS_UINT32              ulRsv0;
    VOS_UINT32              ulRsv1;

    LRRC_NV_CELL_SELECT_RSSI_SORT_CTRL  stRssiSortCtl;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv1;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv2;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv3;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv4;
}LRRC_NV_CELL_SELECT_CTRL_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_CSG_CTRL_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : CSG 驻留NV控制、ASF 控制、CSG次强搜索控制结构
*****************************************************************************/
typedef struct
{
    LRRC_NV_CSG_FEATURE_CTRL            stCsgFeatureCtrl;
    LRRC_NV_CSG_ASF_PARAMETERS_STRU     stCsgASfParameters;
}LRRC_NV_CSG_CTRL_PARA_STRU;

/*begin: mod by  , for NV decoup*/
/*********************************************************************
 结构名    : LRRC_NV_CMAS_ETWS_CTR_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LRRC_NV_CMAS_ETWS_CTR_STRU控制结构
*********************************************************************/
typedef struct
{
    VOS_UINT8  ucEtwsSupportSwitch;  /* ETWS支持开关，0不支持，1支持 */
    VOS_UINT8  ucCmasSupportSwitch;  /* CMAS支持开关，0不支持，1支持 */
    VOS_UINT8  ucCmasReptCampSucessSwitch;     /* 0:CMAS在驻留后上报，1，驻留前后都可上报 */
    VOS_UINT8  ucSndEtwsImidiSwitch;     /* ETWS发生时机，0收齐后再上报，1立即发送 */
    VOS_UINT8  ucGetCmasRcvCmpFlg;     /*CMAS是否设置接收系统消息接收完成的标识，0：设置标识；1：不设置标识 */
    VOS_UINT8  aucReserved[3];
}LRRC_NV_CMAS_ETWS_CTR_STRU;

/*********************************************************************
 结构名    : LRRC_NV_CA_OPTIMIZE_CTR_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : CA优化控制结构
*********************************************************************/
typedef struct
{
    VOS_UINT8  ucCaSupportSwitch;   /* CA支持开关，0不支持，1支持 */
    VOS_UINT8  ucRplyRecfgCmpFlag;  /* UE不支持CA时，如果仪表强配，是否回复CPM0：不回复CMP;1：回复CMP */
    VOS_UINT8  aucReserved[2];
}LRRC_NV_CA_OPTIMIZE_CTR_STRU;

/*********************************************************************
 结构名    : LRRC_NV_VOLTE_OPTIMIZE_CTR_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : VOLTE优化控制结构
*********************************************************************/
typedef struct
{
    VOS_UINT8               ucVolteDelayB2MeasRptWhenMO;   /* Volte主叫延迟报B2测量报告以避免发生bsrvcc */
    VOS_UINT8               ucVolteDelayB2MeasRptWhenMT;   /* Volte被叫延迟报B2测量报告以避免发生bsrvcc */
    VOS_UINT8               ucVolteNoTdsConnB2MeasSwitch;  /* 中移动Volte不启动Tds测量优化开关 */
    VOS_UINT8               ucReserved;

    LRRC_NV_VOLTE_A2B2_SWITCH_AND_THRETH_STRU       stVolteA2B2;
    LRRC_NV_VOLTE_OR_CSFB_CTRL_STRU                 stVolteOrCsfbCtrl;
    LRRC_NV_VOLTE_B2_PRA2_SWITCH_AND_THRETH_STRU    stVolteB2Pra2Para;
}LRRC_NV_VOLTE_OPTIMIZE_CTR_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_INTER_OPT_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 互操作控制结构NV，包括本系统，异系统
*****************************************************************************/
typedef struct
{
    VOS_UINT8               ucSCellReselRsrqSwitch;    /* LTE模内重选，对服务小区RSRQ限制开关 */
    /* add for Conn Meas Filter Optimize begin */
    VOS_UINT8               ucConnMeasFilterOptimizeSwitch;
    /* add for Conn Meas Filter Optimize begin */
    VOS_UINT8               ucCsfbT304ExpCcoRemain;    /* CSFB流程中，T304超时，不停止CCO开关 */
    VOS_UINT8               ucVolteDelayB2MeasRptWhenMO;   /* Volte主叫延迟报B2测量报告以避免发生bsrvcc */
    VOS_UINT8               ucVolteDelayB2MeasRptWhenMT;   /* Volte被叫延迟报B2测量报告以避免发生bsrvcc */
    /* Do not start tds connected B2 meas when volte is running, Begin */
    VOS_UINT8               ucVolteNoTdsConnB2MeasSwitch;  /* 中移动Volte不启动Tds测量优化开关 */
    /* Do not start tds connected B2 meas when volte is running, End */
    /* set LTE Default resel prioty when no resel priory in sib5,begin */
    VOS_UINT8               ucSetLteReselPriotySwitch;
    /* set LTE Default resel prioty when no resel priory in sib5,end */
    VOS_UINT8               ucSwitch3;

    VOS_INT16               sSCellReselRsrqTres;       /* LTE模内重选，服务小区RSRQ限制绝对门限*/
    VOS_INT16               sReselDeltaRsrq;          /* LTE模内重选，服务小区RSRQ低于绝对门限，并且邻区比服务小区好usReselDeltaRsrq个DB，并且邻区RSRP满足门限，重选*/
    VOS_INT16               sReselRsrpTres;            /* LTE模内重选，邻区满足sReselDeltaRsrq的差值门限要求时，RSRP需要高于sReselRsrpTres */
    VOS_UINT16              usBgsClSearchThresSwitch;     /* 自研CL模式下，C背景搜L，小区搜索门限开关 */
    VOS_INT16               sBgsClSearchThres;         /* 自研CL模式下，C背景搜L，小区搜索门限 */
    VOS_UINT16              usCellSelThresSwitch;         /* 为防止网络配置的驻留门限过低，定制私有门限开关 */
    VOS_INT16               sCellSelQrxlevMin;         /* ucCellSelThresSwitch打开时有效，为防止网络配置的驻留门限过低，定制私有QrxlevMin值,单位1dB */
    VOS_INT16               sCellSelqQualMin;         /* ucCellSelThresSwitch打开时有效，为防止网络配置的驻留门限过低，定制私有qQualMin值,单位1dB */
    /* begin, mod for conn meas and A3 timer */
    VOS_UINT16              usConnMeasCtrlSwitch;         /* 连接态弱RSRQ测量控制开关 */
    VOS_INT16               sConnMeasRsrq;                /* 连接态弱RSRQ门限: 1、不考虑sMeasure起同频测量 2、缩短A3评估时延的RSRQ门限 */
    VOS_UINT16              usEverntA3TrigerTime;         /* 缩短的A3评估时间: 单位ms  */
    /* end, mod for conn meas and A3 timer */
    VOS_UINT16              usNoRptSecondCellThres;
    VOS_INT16               sLowPriMeasSrvRsrqThres;   /* 起等级优先级测量的服务小区q值门限，单位1/8dB,控制开关，非标优化 */

    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;


    VOS_UINT32              ulRsv0;
    VOS_UINT32              ulRsv1;

    LRRC_NV_INTER_OPT_FREQENTLY_CTRL    stOptFreqentlyCtl;
    LRRC_NV_PRIVATE_B1_SWITCH_AND_THRETH_STRU    stPriB1;
    LRRC_NV_VOLTE_A2B2_SWITCH_AND_THRETH_STRU    stVolteA2B2;
    LRRC_NV_VOLTE_OR_CSFB_CTRL_STRU     stVolteOrCsfbCtrl;
    LRRC_NV_VOLTE_B2_PRA2_SWITCH_AND_THRETH_STRU     stVolteB2Pra2Para;
    /* Volte Evs重协商特性,begin */
    LRRC_NV_VOLTE_EVS_PARA_CTRL_STRU     stVolteEvsParaCtrl;
    /* Volte Evs重协商特性,end */
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv1;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv2;
    //TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv3;
}LRRC_NV_INTER_OPT_CTRL_STRU;

/*****************************************************************************
 结构名    : LUP_MEM_POOL_CFG_NV_STRU
 结构说明  : LUP_MEM_POOL_CFG_NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                      ucClusterCnt;
    VOS_UINT8                      aucRsv[3];
    VOS_UINT16                     ausBlkSize[LUP_MEM_MAX_CLUSTER_NUM];         /* 本级数的大小 */
    VOS_UINT16                     ausBlkCnt[LUP_MEM_MAX_CLUSTER_NUM];          /* 本级个数 */
}LUP_MEM_POOL_CFG_NV_STRU;

/*****************************************************************************
 结构名    : TTF_MEM_SOLUTION_CFG_NV_STRU
 结构说明  : TTF_MEM_SOLUTION_CFG对应的NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucPoolCnt;
    VOS_UINT8                           aucReserve[3];
    LUP_MEM_POOL_CFG_NV_STRU            astLupMemPoolCfgInfo[LUP_MEM_MAX_POOL_NUM];
}LUP_MEM_CFG_NV_STRU;

/* begin: mod for vowifi cellerprefer */
/*****************************************************************************
 结构名    : LRRC_NV_VOWIFI_TREES_STRU
 协议表格  : 1、P值小于门限的lowp或者Q值小于门限的lowq，质量为BAD；
             2、P值大于门限的highp并且Q值大于门限的highq，质量为GOOD;
             3、其他情况为NORMAL;
 ASN.1描述 :
 结构说明  : vowifi 能量上报门限
*****************************************************************************/
typedef struct
{
    VOS_INT16               sThresWlanLowP;
    VOS_INT16               sThresWlanLowQ;
    VOS_INT16               sThresWlanHighP;
    VOS_INT16               sThresWlanHighQ;
    VOS_UINT32              ulTSteeringWLAN;
    VOS_INT16               sThresWlanDeltaP;
    VOS_INT16               sStableIntervalRptLen; /*迟滞上报，在稳定区间上触发上报时间*/
}LRRC_VOWIFI_THRES_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_VOWIFI_TREES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : vowifi 能量上报门限，连接态和idle态使用不用的门限
*****************************************************************************/
typedef struct
{
    LRRC_VOWIFI_THRES_STRU  stVowifiThresConn;
    LRRC_VOWIFI_THRES_STRU  stVowifiThresIdle;
}LRRC_NV_VOWIFI_THRES_STRU;
/* end: mod for vowifi cellerprefer */

/*begin,add for B29 single dl ctrl,  */
/*****************************************************************************
 结构名    : LRRC_NV_ONLY_DL_BAND_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : Band 单下行功能相关的开关的结构体
*****************************************************************************/
typedef struct
{
    /* 每组对应32个band，每一位对应band值 = (组数-1)*32 + 标志位 */
    LPS_NV_LONG_BIT_STRU                stBandGroupFlag[LRRC_SINGLE_DL_BAND_GROUP_NUM];
}LRRC_NV_ONLY_DL_BAND_CTRL_STRU;
/*end,add for B29 single dl ctrl,  */


/* begin,  ,add for Irat Resel RsrqTres */
/*****************************************************************************
 结构名    : GUTL_COMM_RESEL_THRES_CFG_NV_STRU
 结构说明  : GUTL_COMM_RESEL_THRES_CFG对应的NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucFeatureSwitch;                        /* 功能开关*/
    VOS_UINT8                           ucHigPrioRselSwitch;                    /* L本系统高优先级重选优化功能开关 */
    VOS_INT16                           sRsrqThres;                             /* L的服务小区Rsrq私有门限，单位1dB */
    VOS_INT16                           sDeltaRsrq;                             /* 异系统重选回L的RSRQ约束的迟滞门限，单位1dB */
    VOS_INT16                           sUtraRscp;                              /* 低于此值时，utra重选回L按原流程处理，单位1dB */
    VOS_INT16                           sUtraEcn0;                              /* 低于此值时，utra重选回L按原流程处理，单位1dB */
    VOS_INT16                           sGeranRssi;                             /* 低于此值时，geran重选回L按原流程处理，单位1dB */
    VOS_INT16                           sRsrpThres;                             /* L本系统高优先级邻区Rsrp私有门限，单位1dB */
    VOS_UINT16                          usDeltaRsrp;                            /* L本系统高优先级邻区与服务区Rsrp绝对差值门限，单位1dB */
    VOS_UINT16                          usReserve1;
    VOS_UINT16                          usReserve2;
    VOS_UINT16                          usReserve3;
    VOS_UINT16                          usReserve4;
}GUTL_COMM_RESEL_THRES_CFG_NV_STRU;
/* end,  ,add for Irat Resel RsrqTres */

/*****************************************************************************
结构名    : TL_CHR_BIG_DATA_NV_PARA_STRU
协议表格  :
ASN.1描述 :
结构说明  : 大数据统计CHR 信号强度门限
*****************************************************************************/
typedef struct
{
    VOS_INT16           sLteLowRsrpThres;              /* 大数据统计CHR LTE弱信号门限 */
    VOS_INT16           sLteHigRsrpThres;              /* 大数据统计CHR LTE强信号门限 */
    VOS_INT8            cTdsLowRscpThres;              /* 大数据统计CHR TDS弱信号门限 */
    VOS_INT8            cTdsHigRscpThres;              /* 大数据统计CHR TDS强信号门限 */
    VOS_UINT8           ucResv[2];
}TL_CHR_BIG_DATA_NV_PARA_STRU;

/* begin:add for mmp */
/*****************************************************************************
结构名    : TL_CHR_BIG_DATA_NV_PARA_STRU
协议表格  :
ASN.1描述 :
结构说明  : EN_NV_ID_RRC_MMP_CONFIG LRRC多modem共享优化配置参数
*****************************************************************************/
typedef struct
{
    VOS_UINT32              bitMmpPagingShareEnable : 1;         /* 多modem paging共享开关 */
    VOS_UINT32              bitMmpCandFreqShareEnable : 1;       /* 多modem 先验信息共享开关 */
    VOS_UINT32              bitMmpBsicShareEnable : 1;           /* 多modem Bsic信息共享开关 */
    VOS_UINT32              bitMmpSibShareEnable : 1;            /* 多modem 系统消息共享开关 */
    VOS_UINT32              bitMmpMeasureShareEnbale : 1;        /* 多modem 测量共享开关 */
    VOS_UINT32              bitMmpReselShareEnbale : 1;          /* 多modem 重选共享开关 */
    VOS_UINT32              bitRsv1 : 1;
    VOS_UINT32              bitRsv2 : 1;
    VOS_UINT32              bitRsv3 : 1;
    VOS_UINT32              bitRsv4 : 1;
    VOS_UINT32              bitRsv5 : 1;
    VOS_UINT32              bitRsv6 : 1;
    VOS_UINT32              bitRsv7 : 1;
    VOS_UINT32              bitRsv8 : 1;
    VOS_UINT32              bitRsv9 : 1;
    VOS_UINT32              bitRsv10 : 1;
    VOS_UINT32              bitRsv11 : 1;
    VOS_UINT32              bitRsv12 : 1;
    VOS_UINT32              bitRsv13 : 1;
    VOS_UINT32              bitRsv14 : 1;
    VOS_UINT32              bitRsv15 : 1;
    VOS_UINT32              bitRsv16 : 1;
    VOS_UINT32              bitRsv17 : 1;
    VOS_UINT32              bitRsv18 : 1;
    VOS_UINT32              bitRsv19 : 1;
    VOS_UINT32              bitRsv20 : 1;
    VOS_UINT32              bitRsv21 : 1;
    VOS_UINT32              bitRsv22 : 1;
    VOS_UINT32              bitRsv23 : 1;
    VOS_UINT32              bitRsv24 : 1;
    VOS_UINT32              bitRsv25 : 1;
    VOS_UINT32              bitRsv26 : 1;

    /* 先验频点共享优化参数 */
    VOS_INT16               sMmpLowRsrpThresh;                   /* 低质量先验频点门限 */
    VOS_INT16               sMmpHighRsrpThresh;                  /* 高质量先验频点门限 */
    VOS_INT16               sMmpRsrpOffset;                      /* 低优先级频点高于高质量频点offset以上时，且高质量频点不过sMmpHighRsrpThresh时，先搜低优先级频点 */
    PS_BOOL_ENUM_UINT8      enCandFreqShareInChina;              /* 先验频点共享优化仅在国内生效 */
    VOS_UINT8               ucMmpMaxCandFreqNum;                 /* MMP先搜频点的最大个数，最小为1，1表示仅先搜另一modem主频点 */
}RRC_MMP_CONFIG_STRU;
/* end:add for mmp */

/*****************************************************************************
 结构名    : 开机log功能是否打开

 结构说明  : cPowerOnlogC和cPowerOnlogA都为1，此功能才生效
*****************************************************************************/
typedef struct
{
    /* C核的开机log是否打开*/
    VOS_INT8                            cPowerOnlogC;

    /* AP的开机log是否打开,默认为0。在A核linux内核初始化时根据SOCP的50M是否存在修改此NV的值:存在，此值会被修改为1；否则，保持0不变 */
    VOS_INT8                            cPowerOnlogA;
    VOS_INT8                            cSpare1;
    VOS_INT8                            cSpare2;
}NV_POWER_ON_LOG_SWITCH_STRU;

/* 仪器测试时，搜网搜到非仪器的PLMN不上报，begin */
/*****************************************************************************
 结构名    : LRRC_NV_TEST_PLMN_SET_STRU

 结构说明  : PLMN搜网阶段，可指定的测试PLMN
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8                        enTestPlmnSetFlag;
    VOS_UINT8                                 ucPlmnNum;
    LRRC_LNAS_PLMN_ID_STRU                    stPlmnId[LRRC_TEST_PLMN_MAX_NUM];
}LRRC_NV_TEST_PLMN_SET_STRU;
/* 仪器测试时，搜网搜到非仪器的PLMN不上报，end */

/*****************************************************************************
 结构名    : LRRC_NV_RA_FAIL_REPORT_STRU

 结构说明  : LRRC  随机接入失败，上报，CHR
*****************************************************************************/
typedef struct
{
    RRC_NV_SWITCH_ENUM_UINT32                 enReportSwitch;           /* 随机接入失败上报开关 */
    VOS_INT16                                 sRsrpThreshold;           /* RSRP值,门限值:默认为  -100db */
    VOS_INT16                                 sRsrqThreshold;           /* RSRq值,门限值:默认为  -13db */
    VOS_UINT32                                ulSilencePeriod;          /* 最短时间间隔 30min */
}LRRC_NV_RA_FAIL_REPORT_STRU;

/*begin,add for plmn forbidden band,  */
/*****************************************************************************
 结构名    : LRRC_SINGLE_PLMN_FORBIDDEN_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 单个PLMN禁止Band
*****************************************************************************/
typedef struct
{
    LRRC_LNAS_PLMN_ID_STRU  stNasFormatPlmn;
    VOS_UINT16              ausPlmnBand[LRRC_SINGLE_PLMN_FORBIDDEN_BAND_MAX_NUM];
}LRRC_SINGLE_PLMN_FORBIDDEN_BAND_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_PLMN_FORBIDDEN_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : PLMN禁止Band
*****************************************************************************/
typedef struct
{
    VOS_UINT32                            ulPlmnNum;
    LRRC_SINGLE_PLMN_FORBIDDEN_BAND_STRU  astPlmn[LRRC_FORBIDDEN_BAND_PLMN_MAX_NUM];
}LRRC_NV_PLMN_FORBIDDEN_BAND_STRU;
/*end,add for plmn forbidden band,  */
/* Begin UserPlan improve by   */
/*****************************************************************************
 结构名    : LRRC_NV_USER_PLANE_IMPROVE_CTRL_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 数据业务用户体验优化NV结构
 *****************************************************************************/
typedef struct
{
    VOS_UINT8               ucConnRelProtectSwtich;         /* 空口释放未解到优化开关 */
    VOS_UINT8               ucSRProtectSwtich;              /* 网络不配SR保护开关 */
    VOS_UINT8               ucReestRecfgProtectSwtich;      /* 保护重建后网络不发重配开关 */
    VOS_UINT8               ucUeCapProtectSwtich;           /* UE 能力兼容特性开关 */
    VOS_UINT8               ucPdcpDisPduProtectSwtich;      /* PDCP连续窗外丢包保护开关*/
    VOS_UINT8               ucPdcpPacketErrProtectSwtich;   /* PDCP连续解密出错保护开关 */
    VOS_UINT8               ucUserPlaneImproveChrSwtich;    /* 数据业务用户体验CHR开关 */
    VOS_UINT8               ucSwitch7;
    VOS_UINT8               ucSwitch8;
    VOS_UINT8               ucSwitch9;
    VOS_UINT8               ucSwitch10;
    VOS_UINT8               ucSwitch11;
    VOS_UINT16              usDecreaA2swtich;
    VOS_INT16               sDecreaA2offset;
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
    VOS_UINT16              usRsv4;
    VOS_UINT16              usRsv5;
    VOS_UINT16              usRsv6;
    VOS_UINT16              usRsv7;
    VOS_UINT16              usRsv8;
    VOS_UINT16              usRsv9;
    VOS_UINT16              usRsv10;
    VOS_UINT16              usRsv11;
    VOS_UINT16              usRsv12;
    VOS_UINT16              usRsv13;
    VOS_UINT16              usRsv14;
    VOS_UINT16              usRsv15;

    VOS_UINT32              ulConnRelProtectTimerLen;    /* 空口释放保护定时器时长单位ms */
    VOS_UINT32              ulRaSussCountThre;           /* 随机接入成功次数门限 */
    VOS_UINT32              ulRecfgProtectTimerLen;      /* 等待重配定时器时长单位ms */
    VOS_UINT32              ulCapRelDetectCountThre;     /* 检测UE能力上报异常次数门限 */
    VOS_UINT32              ulRsv4;
    VOS_UINT32              ulRsv5;
    VOS_UINT32              ulRsv6;
    VOS_UINT32              ulRsv7;

    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv0;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv1;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv2;
    TLRRC_NV_CTRL_SUBFUNC_COMM_STRU     stRsv3;
}LRRC_NV_USER_PLANE_IMPROVE_CTRL_STRU;
/* End UserPlan improve by   */

/*begin: mod by  , for NV decoup*/
/*****************************************************************************
 结构名    : LRRC_NV_RF_MEAS_PARAM_V1020
 协议表格  :
 ASN.1描述 : NV53795拆分出NV48633 对应结构
 结构说明  : LRRC_NV_RF_MEAS_PARAM_V1020_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitRfMeasParamV1020Present                  : 1;
    VOS_UINT32    bitSpare                                    : 31;
    RRC_RF_MEAS_PARAM_V1020_STRU                stRfMeasParamV1020;
    RRC_RF_MEAS_PARAM_V1020_STRU                stRfMeasParamV1020Ext;
}LRRC_NV_RF_MEAS_PARAM_V1020_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_UE_CAP_FEATURE_GROUP_IND_R10
 协议表格  :
 ASN.1描述 : NV53795拆分出NV48634对应结构
 结构说明  : LRRC_NV_UE_CAP_FEATURE_GROUP_IND_R10_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitFeatureGroupIndRel10R10Present           : 1;
    VOS_UINT32    bitSpare                                    : 31;
    RRC_UE_CAP_FEATURE_GROUP_IND_R10_STRU       stFeatureGroupIndRel10R10;
}LRRC_NV_UE_CAP_FEATURE_GROUP_IND_R10_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_UE_CAP_FEATURE_GROUP_IND_R9
 协议表格  :
 ASN.1描述 : NV53794拆分出NV48635 对应结构
 结构说明  : LRRC_NV_UE_CAP_FEATURE_GROUP_IND_R9_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitFeatureGroupIndRel9AddR9Present          : 1;
    VOS_UINT32    bitFddFeatureGroupIndR9Present              : 1;
    VOS_UINT32    bitFddFeatureGroupIndRel9AddR9Present       : 1;
    VOS_UINT32    bitTddFeatureGroupIndR9Present              : 1;
    VOS_UINT32    bitTddFeatureGroupIndRel9AddR9Present       : 1;
    VOS_UINT32    bitSpare                                    : 27;

    RRC_UE_CAP_FEATURE_GROUP_IND_R9ADD_STRU       stFeatureGroupIndRel9AddR9;
    RRC_UE_CAP_FEATURE_GROUP_IND_STRU             stFddFeatureGroupIndR9;
    RRC_UE_CAP_FEATURE_GROUP_IND_R9ADD_STRU       stFddFeatureGroupIndRel9AddR9;
    RRC_UE_CAP_FEATURE_GROUP_IND_STRU             stTddFeatureGroupIndR9;
    RRC_UE_CAP_FEATURE_GROUP_IND_R9ADD_STRU       stTddFeatureGroupIndRel9AddR9;
}LRRC_NV_UE_CAP_FEATURE_GROUP_IND_R9_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_UE_CAP_FEATURE_GROUP_IND
 协议表格  :
 ASN.1描述 : NV53804拆分出NV48636 对应结构
 结构说明  : LRRC_NV_UE_CAP_FEATURE_GROUP_IND_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitFeatureGroupIndsPresent    : 1;
    VOS_UINT32                          bitSpare                      : 31;

    RRC_UE_CAP_FEATURE_GROUP_IND_STRU   stFeatrueGroupInds;
}LRRC_NV_UE_CAP_FEATURE_GROUP_IND_STRU;

/*****************************************************************************
 结构名    : LRRC_NV_UE_CAP_PDCP_PARA
 协议表格  :
 ASN.1描述 : NV53804拆分出NV48637 对应结构
 结构说明  : LRRC_NV_UE_CAP_PDCP_PARA_STRU
*****************************************************************************/
typedef RRC_UE_CAP_PDCP_PARA_STRU  LRRC_NV_UE_CAP_PDCP_PARA_STRU;

/*end: mod by  , for NV decoup*/

/*****************************************************************************
 结构名    : LRRC_NV_VOLTE_END_EVAB1B2_CTR_INFO
 协议表格  :
 ASN.1描述 :
 结构说明  : VOLTE结束后对评估B1B2事件启动时刻进行控制NV结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32              ulVolteEndEvaB1B2CtrTimerLen;/* VOLTE结束后对评估B1B2事件启动延长时间 */
    VOS_UINT8               ucRsv1;
    VOS_UINT8               ucRsv2;
    VOS_UINT16              usRsv1;
    VOS_UINT16              usRsv2;
    VOS_UINT16              usRsv3;
}LRRC_NV_VOLTE_END_EVAB1B2_CTR_INFO_STRU;


/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/
#if (VOS_OS_VER != VOS_WIN32)
/*boston modify begin */
/*关闭TA Timer的开关，0:开关关闭，即TA Timer生效 1:开关打开，即TA Timer不生效*/
extern VOS_UINT32  g_aulCloseTaTimer[LPS_MAX_MODEM_NUM];

/*关闭SR触发随机接入的开关，0:开关关闭，即SR触发随机接入生效 1:开关打开，即SR触发随机接入不生效*/
extern VOS_UINT32  g_aulCloseSrRandFlag[LPS_MAX_MODEM_NUM];

/*关闭SR触发机制开关，0:开关关闭，即SR不以规整BSR触发为前提 1:开关打开，即SR以规整BSR触发为前提*/
extern VOS_UINT32  g_aulSrTrigFlag[LPS_MAX_MODEM_NUM];
/*boston modify end */

/*设置是否将板间消息转发一份至PC以解析分析*/
extern VOS_UINT32  g_ulIfForwardToPc;

/* 加密的开关:0-打开加密，1-关闭加密*/
extern VOS_UINT32  g_ulSmcControl;

/* DCM定制需求，GU到L的切换不考虑被禁TA的开关 */
extern VOS_UINT32  g_ulHoIgnoreForbidTaFlag;
/* 定制需求，模内切换不考虑被禁TA的开关 1:打开 0:关闭 默认关闭 */
extern VOS_UINT32    g_ulIntraHoIgnoreForbidTaFlag;

/*外场测试桩代码开关 0:关闭  1:打开.默认为关闭*/
/*boston modify begin */
extern VOS_UINT32 g_aulFieldTestSwitch[LPS_MAX_MODEM_NUM];
/*boston modify end */

/*规避eNB的TDD的接入问题*/

/*判断是否BAR小区*/
extern VOS_UINT32 g_ulCloseBarCell;

/* DRX Control Flag  0: Close 1: Open */
extern VOS_UINT32 g_ulDrxControlFlag;

/* 上下行子帧配比，默认为1 */
extern VOS_UINT32 g_ulCfgSubFrameAssign;

/* 特殊子帧配置，默认为7 */
extern VOS_UINT32 g_ulSubFramePattern;

/* 天线个数 */
extern VOS_UINT32 g_ulPsCfgDspAntCnt;


/* PCO功能开关，0为关闭，1为打开。默认为打开 */
extern VOS_UINT32 g_ulPcoFlag;

/* 删除SDF时是否携带Qos信息功能开关，0为关闭，1为打开。默认为关闭 */
extern VOS_UINT32 g_ulDelSdfWithQosFlag;

/* 无用全局变量删除, 2016-4-19*/

/* TAU消息加密开关，0为加密，1为不加密。默认为不加密 */
extern VOS_UINT32 g_ulTauCipheredFlag;

/*流控算法开关*/
extern VOS_UINT32  g_ulFlowCtrlFlag;

/*流控调整CPU剩余目标值*/
extern VOS_UINT32  g_ulFcPdcpTarget;

/*漏桶算法的最大值*/
extern VOS_UINT32   g_ulTargetIPMax;
extern VOS_UINT32   g_ulSendSduMax;

/*定义漏桶算法阈值*/
extern VOS_UINT32  g_ulDlSduMax;
extern VOS_UINT32  g_ulULIpMax;

/*漏桶算法的测量周期，单位ms*/
extern VOS_UINT32  g_ulMeasGap;
extern VOS_UINT32  g_ulSduGap;
/*流控初始化选择*/
extern VOS_UINT32  g_ulFcInitFlag ;

/*用于对Msg4 6种头格式的CR修改前的解析处理，1表示可以解析所有相关的Msg4头，
表示只能解析最新协议规定的6种格式，默认是0*/
/*boston modify begin */
extern VOS_UINT32  g_aulMsg4ForOldCrFlag[LPS_MAX_MODEM_NUM];
/*boston modify end */

/* GCF是否支持24.301 Table D.1.1中建链原因设置 0x00000004,支持，0不支持 */
extern VOS_UINT32 g_ulGcfEstCauseFlag;

/* SMC功能是否支持测试模式，缺省支持，取值为0x00000008 */
extern VOS_UINT32  g_ulSmcFuncTestMode;

/* GCF打桩通过CDRX相关用例，实际场景为了降功耗不打开该开关，位域开关,0表示默认不打开对应外场测试
1表示打开对应GCF测试，默认值是0
extern VOS_UINT32  g_ulGcfConnDrxStubFlag;*/

/* Added usim API clear 2016-09-08 begin */
/* LNAS存储的是否为测试卡，VOS_TRUE:是测试卡；VOS_FALSE:非测试卡*/
extern VOS_UINT8  g_ucTestCardFlag;
/* Added usim API clear 2016-09-08 end */

#endif
/* 产品选择开关 */
extern VOS_UINT32  g_ulPsSupportBand64Flg;
extern VOS_UINT32  g_ulPsPagingConnRelFlg;
extern VOS_UINT32  g_ulPsUlDrbDiscardFlag;
extern VOS_UINT32  g_ulMbmsSupportSwitch;
extern VOS_UINT32  g_ulPsBandwidthSupportFlg;
/* modify freq all begin */
extern VOS_UINT32  g_ulPsFreqInAllBandWidthFlg;

extern VOS_UINT32  g_ulHoFailFlag;
extern VOS_UINT16  g_usHoFailT304;
/* -SYSCFG-SAVENV-Begin */
extern VOS_UINT32  gulJpSbmDualImsiSwitch;
extern VOS_UINT32  gulLteDelWcdmaBandInJapan;           /*NV配置中，需要删除的*/
/* Begin: 2015/5/6 sbm */
extern VOS_UINT8       g_ucSbmCustomSrchFlg;
extern VOS_UINT32      g_ulSbmSupBandInJapan;         /* 软银在日本国内支持的band，如果是全1则认为支持的band不做限制，支持UE支持的所有band */
extern VOS_UINT32      g_ulEmSupBandInJapan;          /* EM在日本国内支持的band，如果是全1则认为支持的band不做限制，支持UE支持的所有band */
extern VOS_UINT32      g_ulOtherOperateSupBandInJapan;/* 其他运营商在日本国内支持的band，如果是全1则认为支持的band不做限制，支持UE支持的所有band */
extern VOS_UINT32      g_ulSupBandOutJapan;
/* Begin: 2015/5/6 sbm */

/*Begin: for sbm delete wcdma band9  */
extern VOS_UINT8       gucJpUband1MncCnt;
extern VOS_UINT8       gaucJpUband1Mnc[3];
extern VOS_UINT8       gaucJpUband1Mcc[3];
extern VOS_UINT8       gaucJpUband2Mcc[3];
/*End: for sbm delete wcdma band9  */
/* -SYSCFG-SAVENV-End */
extern VOS_UINT32  g_ulLReestIratCellSrchFlg;
/* -nccpermit */
extern VOS_UINT32 g_ulNccPermitFlag;
extern VOS_UINT32  g_ulLBgsSupportFlg;
/* -reest-measGap*/
extern VOS_UINT32  g_ulLBgsLSupportFlg;

extern VOS_UINT32 g_ulLTEReestMeasGapSetupFlag;
/* BEGIN   2012-12-08 Add, 控制屏幕关闭后发送AT命令，通知LTE进入IDLE的功能*/
extern VOS_UINT32 g_ulLTEConnToIdleFlag;
/* END  2012-12-08 Add, 控制屏幕关闭后发送AT命令，通知LTE进入IDLE的功能*/


/* -PLMN_EXACTLY_COMPARE_FLG */
extern VOS_UINT32 g_ulPlmnExactCmpFlg;
/* -SoftBank-定制标志:true,set Band41 range */
extern VOS_UINT32 gulSoftBankFlag ;
/* -nccpermit-2 */
extern VOS_UINT32 g_ulNoIdleNccPermitFlag;

/* -dcom-resel-cfg */
extern VOS_UINT32 g_ulLteJpDcomUseR8Flag;
extern VOS_UINT32 g_ulLteJpDccomRelOffSetFlag;
extern VOS_UINT32 g_ulLteUtranEvaFlag;
/* -fast-dorm-cfg */
extern VOS_UINT32 g_ulLteJpFastdormFlag;
/* -fast-dorm-cfg-3 */
extern VOS_UINT32 g_usLteJpFastdormDelayValue;

/*  TdsDataInfoNotify Begin*/
extern VOS_UINT32 g_ulDlCtrlPduNotifyFlag;
/*  TdsDataInfoNotify End*/

/* -CMAS-Begin */
/* >0: cmas report after rrc auth(after camp in fact); 0:cmas report even when cell searching*/
extern VOS_UINT32                              gul_CmasRptBeforeAuthSwitch;
/* -CMAS-End */

/*add 低功耗模块增加 ADRX SSC NV开关 stPsFunFlag03 第0位 第1位*/
extern VOS_UINT32 g_ulSscSupportFlag;
extern VOS_UINT32 g_ulAdrxSupportFlag;
/* taiwan modify begin */
extern VOS_UINT32 g_ulRedirNotBandScanFlg;
/* taiwan modify end */

/* 测试网侧发送Paging情况,不处理本UE Paging  begin*/
extern PS_BOOL_ENUM_UINT8  g_enPsIdleDiscardPagingFlag;
/* 测试网侧发送Paging情况,不处理本UE Paging  end*/

/* ESR流程发生切换,未发起TAU,重传ESR消息控制开关:规避小区切换后网侧不下发REL问题 */
extern VOS_UINT32  g_ulNasEmmHoRetransEsrFlag;
/* rrc release modify begin */
extern VOS_UINT32 g_ulNotTryFakeCellFlg;
/* rrc release modify end */

extern VOS_UINT32                      g_ulDynmicUeCapFlg;

/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/


/*****************************************************************************
  9 OTHERS
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

#endif /* end of PsNvInterface.h */

