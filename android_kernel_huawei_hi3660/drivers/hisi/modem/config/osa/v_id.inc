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

/*****************************************************************************/
/*                                                                           */
/*                Copyright 1999 - 2003, Huawei Tech. Co., Ltd.              */
/*                           ALL RIGHTS RESERVED                             */
/*                                                                           */
/* FileNmae: \vos\include\vos\v_id.inc                                       */
/*                                                                           */
/* Author: Qin Peifeng                                                       */
/*                                                                           */
/* Version: 1.0                                                              */
/*                                                                           */
/* Date: 2000-04-21                                                          */
/*                                                                           */
/* Description:                                                              */
/*                                                                           */
/* Others:                                                                   */
/*                                                                           */
/* History:                                                                  */
/* 1. Date: 2000-04-21                                                       */
/*    Author: Qin Peifeng                                                    */
/*    Modification: Create this file                                         */
/*                                                                           */
/*****************************************************************************/

#ifndef _V_ID_INC
#define _V_ID_INC

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif /* __cpluscplus */
#endif /* __cpluscplus */

#include "product_config.h"

extern VOS_UINT32 MOD_RegFidPidRSP( VOS_UINT32 ulPID, VOS_UINT32 ulFID, VOS_INT priority, MODEM_ID_ENUM_UINT16 usModemId);

/*********************** INNER VARIABLES DEFINITION **************************/
/* PID与FID对应关系表 */
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
BEGIN_DECLARE_PID_BELONG_TO_FID()
    ON_DECLARE(MSP_PID_DIAG_AGENT ,             MSP_FID_DIAG_CCPU,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSP_PID_BBP_AGENT ,              MSP_FID_DIAG_CCPU,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_USIM,               I0_MAPS_USIMM_FID,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_MAPS_STK_PID,                 I0_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_MAPS_PIH_PID,                 I0_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_MAPS_PB_PID,                  I0_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_LL,                  I0_UEPS_FID_SL,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_SN,                  I0_UEPS_FID_SL,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_RLC,                I0_UEPS_FID_RM,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_MAC,                I0_UEPS_FID_RM,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_GAS,                 I0_UEPS_FID_RR,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_GRM,                 I0_UEPS_FID_GRM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_SHPA,                   WUEPS_FID_HPA,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_MM     ,               I0_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_MMC,                I0_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_MM,                 I0_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_GMM,                I0_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_NAS_COMM,              I0_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_SMT,                   WUEPS_FID_CM,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_CBS,                   WUEPS_FID_CM,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_CC,                 I0_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_SS,                 I0_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_SMS,                I0_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_TC,                 I0_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_RABM,               I0_WUEPS_FID_RABMSM,    VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_SM,                 I0_WUEPS_FID_RABMSM,    VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEAPP_PID_MUX,                  WUEPS_FID_TAF,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_ADMIN,              I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_TAF,                I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_VC,                 I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_DRV_AGENT,          I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_MTA,                 I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_XSMS,                I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_MMA,                I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_SAR,                   WUEPS_FID_SPY,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_SPY,                   WUEPS_FID_SPY,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_MONITOR,               WUEPS_FID_SPY,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_FLOWCTRL_C,             UEPS_FID_FLOWCTRL_C,    VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_MACDL,              I0_UEPS_FID_RM_DL,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_RLCDL,              I0_UEPS_FID_RM_DL,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_NOSIG,                 WUEPS_FID_NOSIG,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_CDS,                    UEPS_FID_CDS,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_NDCLIENT,               UEPS_FID_NDCLIENT,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_DHCP,                   UEPS_FID_NDCLIENT,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_CST,                    UEPS_FID_DC,            VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_DL,                  I0_UEPS_FID_DC,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(NVIM_WRITE_DATA_PID,             WUEPS_FID_NVIM_FLUSH,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_NVIM_FLUSH,            WUEPS_FID_NVIM_FLUSH,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_SLEEP,              I0_WUEPS_FID_SLEEP,     VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_AWAKE,              I0_WUEPS_FID_AWAKE,     VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_DSP_PID_APM,                  I0_DSP_FID_APM,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_UPA,                     DSP_FID_UPA,            VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_DSP_PID_WPHY,                 I0_DSP_FID_WPHY,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_DSP_PID_GPHY,                 I0_DSP_FID_GPHY,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_DSP_PID_SLEEP,                I0_DSP_FID_SLEEP,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_DSP_PID_WAKE,                 I0_DSP_FID_WAKE,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_DSP_PID_IDLE,                 I0_DSP_FID_IDLE,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_RCM,                     DSP_FID_RCM,            VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_STARTUP,                 DSP_FID_STARTUP,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_IPS_CCORE     ,         UEPS_FID_IPS_CCORE ,    VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_CSIMA,              I0_WUEPS_FID_CSIMA,     VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_CMMCA,                 WUEPS_FID_CMMCA,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CCPU_PID_CBT,                    CCPU_FID_CBT,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CCPU_PID_PAM_OM,                 CCPU_FID_PAM_OM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_AGING,                 CCPU_FID_PAM_OM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_REG,                   CCPU_FID_PAM_OM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_MTC,                    UEPS_FID_MTC,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_MSCC,                I0_UEPS_FID_MSCC,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(PS_PID_CSS,                      UEPS_FID_CSS,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_RRM,                    UEPS_FID_RRM,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_UEPS_PID_XPDS,                I0_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_BMC,                I0_UEPS_FID_PB,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_PDCP,               I0_UEPS_FID_PB,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_WRR,                I0_UEPS_FID_WRR,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_WCOM,               I0_UEPS_FID_WRR,        VOS_PRIORITY_NULL,    MODEM_ID_0)

#if ( FEATURE_MULTI_MODEM == FEATURE_ON )
    ON_DECLARE(I1_WUEPS_PID_USIM,               I1_MAPS_USIMM_FID,      VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_MAPS_STK_PID,                 I1_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_MAPS_PIH_PID,                 I1_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_MAPS_PB_PID,                  I1_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_SLEEP,              I1_WUEPS_FID_SLEEP,     VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_AWAKE,              I1_WUEPS_FID_AWAKE,     VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_GAS,                 I1_UEPS_FID_RR,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_MM,                 I1_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_MMC,                I1_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_GMM,                I1_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_NAS_COMM,              I1_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_CC,                 I1_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_SS,                 I1_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_TC,                 I1_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_SMS,                I1_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_RABM,               I1_WUEPS_FID_RABMSM,    VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_SM,                 I1_WUEPS_FID_RABMSM,    VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_ADMIN,              I1_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_TAF,                I1_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_VC,                 I1_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_MTA,                 I1_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_DRV_AGENT,          I1_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_XSMS,                I1_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_MMA,                I1_WUEPS_FID_TAF,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_LL,                  I1_UEPS_FID_SL,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_SN,                  I1_UEPS_FID_SL,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_DL,                  I1_UEPS_FID_DC,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_GRM,                 I1_UEPS_FID_GRM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_DSP_PID_APM,                  I1_DSP_FID_APM,         VOS_PRIORITY_NULL,    MODEM_ID_1)
#if ( FEATURE_MODEM1_SUPPORT_WCDMA == FEATURE_ON )
    ON_DECLARE(I1_DSP_PID_WPHY,                 I1_DSP_FID_WPHY,        VOS_PRIORITY_NULL,    MODEM_ID_1)
#endif
    ON_DECLARE(I1_DSP_PID_GPHY,                 I1_DSP_FID_GPHY,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_DSP_PID_SLEEP,                I1_DSP_FID_SLEEP,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_DSP_PID_WAKE,                 I1_DSP_FID_WAKE,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_DSP_PID_IDLE,                 I1_DSP_FID_IDLE,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_MSCC,                I1_UEPS_FID_MSCC,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_CSIMA,              I1_WUEPS_FID_CSIMA,     VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_UEPS_PID_XPDS,                I1_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_LCS ,               I1_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_1)
#if ( FEATURE_MODEM1_SUPPORT_WCDMA == FEATURE_ON )
    ON_DECLARE(I1_WUEPS_PID_WRR,                I1_UEPS_FID_WRR,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_WCOM,               I1_UEPS_FID_WRR,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_RLC,                I1_UEPS_FID_RM,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_MAC,                I1_UEPS_FID_RM,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_MACDL,              I1_UEPS_FID_RM_DL,      VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_RLCDL,              I1_UEPS_FID_RM_DL,      VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_BMC,                I1_UEPS_FID_PB,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_WUEPS_PID_PDCP,               I1_UEPS_FID_PB,         VOS_PRIORITY_NULL,    MODEM_ID_1)
#endif

#if ( FEATURE_MODEM1_SUPPORT_LTE == FEATURE_ON)
    ON_DECLARE(I1_MSP_L4_L4A_PID,               I1_MSP_L4_FID,          VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_MSP_LDSP_PID,                 I1_MSP_L4_FID,          VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_MSP_SYS_FTM_PID ,             I1_MSP_FTM_FID,         VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_ERRC   ,               I1_PS_FID_RRC  ,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_ERMM   ,               I1_PS_FID_RRC  ,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_ESM    ,               I1_PS_FID_CM   ,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_RABM   ,               I1_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_TC     ,               I1_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_IP     ,               I1_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_PTT    ,               I1_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_LPP    ,               I1_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_OM     ,               I1_PS_FID_OM   ,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_MAC_UL ,               I1_PS_FID_RM_UL,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_RLC_UL ,               I1_PS_FID_RM_UL,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_PDCP_UL,               I1_PS_FID_RM_UL,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_MAC_DL ,               I1_PS_FID_RM_DL,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_RLC_DL ,               I1_PS_FID_RM_DL,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_PDCP_DL,               I1_PS_FID_RM_DL,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_HPA    ,               I1_PS_FID_HPA  ,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_IMSA   ,               I1_PS_FID_IMSA ,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_IMSVA  ,               I1_PS_FID_IMSVA,        VOS_PRIORITY_NULL,    MODEM_ID_1)
    ON_DECLARE(I1_PS_PID_MM     ,               I1_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_1)
#endif

#if ( 3 == MULTI_MODEM_NUMBER )/* three modems */
    ON_DECLARE(I2_WUEPS_PID_USIM,               I2_MAPS_USIMM_FID,      VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_MAPS_STK_PID,                 I2_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_MAPS_PIH_PID,                 I2_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_MAPS_PB_PID,                  I2_MAPS_SI_FID,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_SLEEP,              I2_WUEPS_FID_SLEEP,     VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_AWAKE,              I2_WUEPS_FID_AWAKE,     VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_GAS,                 I2_UEPS_FID_RR,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_MM,                 I2_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_MMC,                I2_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_GMM,                I2_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_PS_PID_NAS_COMM,              I2_WUEPS_FID_MM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_CC,                 I2_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_SS,                 I2_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_TC,                 I2_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_SMS,                I2_WUEPS_FID_CM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_RABM,               I2_WUEPS_FID_RABMSM,    VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_SM,                 I2_WUEPS_FID_RABMSM,    VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_ADMIN,              I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_TAF,                I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_VC,                 I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_MTA,                 I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_DRV_AGENT,          I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_XSMS,                I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_MMA,                I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_LL,                  I2_UEPS_FID_SL,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_SN,                  I2_UEPS_FID_SL,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_DL,                  I2_UEPS_FID_DC,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_GRM,                 I2_UEPS_FID_GRM,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_DSP_PID_APM,                  I2_DSP_FID_APM,         VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_DSP_PID_GPHY,                 I2_DSP_FID_GPHY,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_DSP_PID_SLEEP,                I2_DSP_FID_SLEEP,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_DSP_PID_WAKE,                 I2_DSP_FID_WAKE,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_DSP_PID_IDLE,                 I2_DSP_FID_IDLE,        VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_MSCC,                I2_UEPS_FID_MSCC,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_CSIMA,              I2_WUEPS_FID_CSIMA,     VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_UEPS_PID_XPDS,                I2_WUEPS_FID_TAF,       VOS_PRIORITY_NULL,    MODEM_ID_2)
    ON_DECLARE(I2_WUEPS_PID_LCS ,               I2_PS_FID_CM,           VOS_PRIORITY_NULL,    MODEM_ID_2)
#endif

#endif

    ON_DECLARE(I0_MSP_L4_L4A_PID,               I0_MSP_L4_FID,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_MSP_LDSP_PID,                 I0_MSP_L4_FID,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_MSP_SYS_FTM_PID ,             I0_MSP_FTM_FID,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_ERRC   ,               I0_PS_FID_RRC  ,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_ERMM   ,               I0_PS_FID_RRC  ,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(TPS_PID_RRC     ,                PS_FID_RRC ,            VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_ESM    ,               I0_PS_FID_CM   ,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_RABM   ,               I0_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_TC     ,               I0_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_IP     ,               I0_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_PTT    ,               I0_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_LPP    ,               I0_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_WUEPS_PID_LCS ,               I0_PS_FID_CM ,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_OM     ,               I0_PS_FID_OM   ,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_MAC_UL ,               I0_PS_FID_RM_UL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_RLC_UL ,               I0_PS_FID_RM_UL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_PDCP_UL,               I0_PS_FID_RM_UL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_MAC_DL ,               I0_PS_FID_RM_DL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_RLC_DL ,               I0_PS_FID_RM_DL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_PDCP_DL,               I0_PS_FID_RM_DL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_HPA    ,               I0_PS_FID_HPA  ,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_IMSA   ,               I0_PS_FID_IMSA ,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(I0_PS_PID_IMSVA  ,               I0_PS_FID_IMSVA,        VOS_PRIORITY_NULL,    MODEM_ID_0)

#if (FEATURE_ON == FEATURE_EDA_SUPPORT)
    ON_DECLARE(WUEPS_PID_EDA  ,                 PS_FID_EDA,             VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

#if (!defined(CHIP_BB_HI6210))
    ON_DECLARE(TLPHY_PID_RTTAGENT,              TLPHY_FID_RTTAGENT,     VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

#if (FEATURE_ON == FEATURE_UE_MODE_TDS)
    ON_DECLARE(HL1_PID_SNDCMD,                  HL1_FID_SNDCMD,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(TPS_PID_RLC    ,                 TPS_FID_MAC  ,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(TPS_PID_MAC  ,                   TPS_FID_MAC ,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(TPS_PID_PDC     ,                TPS_FID_PDC ,           VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
    ON_DECLARE(UEPS_PID_1XCASM,                 UEPS_FID_CAS1X,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_1XCMEAS,                UEPS_FID_CAS1X,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_1XCSRCH,                UEPS_FID_CAS1X,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HALMP,                  UEPS_FID_CASHRPD,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HSP,                    UEPS_FID_CASHRPD,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HRUP,                   UEPS_FID_CASHRPD,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HSCP,                   UEPS_FID_CASHRPD,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_RLAC,                MSPS_FID_CTTF_1X_REV,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_RMAC,                MSPS_FID_CTTF_1X_REV,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_RRLP,                MSPS_FID_CTTF_1X_REV,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_RTESTSO,             MSPS_FID_CTTF_1X_REV,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_FLAC,                MSPS_FID_CTTF_1X_FWD,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_FMAC,                MSPS_FID_CTTF_1X_FWD,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_FRLP,                MSPS_FID_CTTF_1X_FWD,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_1X_FTESTSO,             MSPS_FID_CTTF_1X_FWD,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_RSLP,              CTTF_FID_HRPD_REV,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_RPA,               CTTF_FID_HRPD_REV,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_RSPS,              CTTF_FID_HRPD_REV,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_RMAC,              CTTF_FID_HRPD_REV,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_FSLP,              CTTF_FID_HRPD_FWD,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_FPA,               CTTF_FID_HRPD_FWD,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_FSPS,              CTTF_FID_HRPD_FWD,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_FMAC,              CTTF_FID_HRPD_FWD,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CTTF_PID_HRPD_SIG,               CTTF_FID_HRPD_SIG,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_PROCSTUB,                DSP_FID_UPHY_HPA,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(LINKH_PID_LINKH_ARM,             LINKH_FID_ARM,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_CSDR_1X_CM_AGENT,        LINKH_FID_ARM,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_CSDR_1X_SM_AGENT,        LINKH_FID_ARM,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_CSDR_HRPD_CM_AGENT,      LINKH_FID_ARM,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_CSDR_HRPD_SM_AGENT,      LINKH_FID_ARM,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_GSDR_HIGHPRI_AGENT,      LINKH_FID_ARM,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_1X_CM,                 CPROC_FID_1X,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_1X_SM,                 CPROC_FID_1X,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_1X_CTAS,               CPROC_FID_1X,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_1X_DSP,                CPROC_FID_1X,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_EVDO,                  CPROC_FID_HRPD,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_HRPD_CM,               CPROC_FID_HRPD,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_HRPD_SM,               CPROC_FID_HRPD,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(CPROC_PID_RM,                    CPROC_FID_RM,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_XREG,                   UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_XCC,                    UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_XSD,                    UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HSD,                    UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HSM,                    UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_EHSM,                   UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HLU,                    UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_HRM,                    UEPS_FID_CNAS,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_PPPC,                   MSPS_FID_PPPC,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(PID_UPHY_CHR_AGENT,              DSP_FID_UPHY_HPA,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_RTTSTUB,                 DSP_FID_RTTSTUB,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_RTTMATCH,                DSP_FID_RTTMATCH,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_RTTPREEMPT,              DSP_FID_RTTPREEMPT,     VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_RTTONLINE,               DSP_FID_RTTONLINE,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSPS_PID_MIP,                    MSPS_FID_PPPC,          VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif
#ifdef HPA_ITT
    ON_DECLARE(MAPS_STUB_RCVMSG_PID,            MAPS_STUB_FID,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MAPS_STUB_SENDMSG_PID,           MAPS_STUB_FID,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MAPS_STUB_GTRMSG_PROC_PID,       MAPS_STUB_FID,          VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif
#if (FEATURE_ON == FEATURE_BASTET)
    ON_DECLARE(UEPS_PID_BASTET,                 UEPS_FID_BASTET,        VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif
#if (VOS_WIN32 == VOS_OS_VER)
    ON_DECLARE(WUEPS_PID_ITTSTUB1,              WUEPS_FID_ITTSTUB,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_ITTSTUB2,              WUEPS_FID_ITTSTUB,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_WTTFTEST,              WUEPS_FID_ITTSTUB,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_OAMTEST,               WUEPS_FID_ITTSTUB,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_CBT,                    ACPU_FID_CBT,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PCSC,                   ACPU_FID_USIMM,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PB,                     ACPU_FID_USIMM,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PCVOICE,                ACPU_FID_PCVOICE,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PAM_OM,                 ACPU_FID_PAM_OM,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_TAFAGENT,               ACPU_FID_TAFAGENT,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_AT,                    WUEPS_FID_AT,           VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

END_DECLARE_PID_BELONG_TO_FID()

#endif

#if (OSA_CPU_ACPU == VOS_OSA_CPU)
BEGIN_DECLARE_PID_BELONG_TO_FID()

    ON_DECLARE(MSP_PID_DIAG_FW,                 MSP_FID_DIAG_ACPU,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(MSP_PID_DIAG_APP_AGENT,          MSP_FID_DIAG_ACPU,      VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(WUEPS_PID_AT,                    MSP_APP_DS_FID,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(PS_PID_APP_NDIS,                 PS_FID_APP_NDIS_PPP_DIPC,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(UEPS_PID_NDSERVER,               PS_FID_APP_NDIS_PPP_DIPC,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(PS_PID_APP_DIPC,                 PS_FID_APP_NDIS_PPP_DIPC,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(PS_PID_APP_MUX,                  PS_FID_APP_NDIS_PPP_DIPC,   VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_RNIC,                   ACPU_FID_RNIC,          VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_NFEXT,                  ACPU_FID_NFEXT,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_ADS_UL,                 ACPU_FID_ADS_UL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_ADS_DL,                 ACPU_FID_ADS_DL,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_CSD,                    ACPU_FID_CSD,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_TAFAGENT,               ACPU_FID_TAFAGENT,      VOS_PRIORITY_NULL,    MODEM_ID_0)

#if(FEATURE_ACPU_STAT == FEATURE_ON)
    ON_DECLARE(ACPU_PID_CPULOAD,                ACPU_FID_CPULOAD,       VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

#if(FEATURE_ACPU_FC_POINT_REG == FEATURE_ON)
    ON_DECLARE(UEPS_PID_FLOWCTRL_A,             UEPS_FID_FLOWCTRL_A,    VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

    ON_DECLARE(ACPU_PID_CBT,                    ACPU_FID_CBT,           VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PCSC,                   ACPU_FID_USIMM,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PB,                     ACPU_FID_USIMM,         VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PCVOICE,                ACPU_FID_PCVOICE,       VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(ACPU_PID_PAM_OM,                 ACPU_FID_PAM_OM,        VOS_PRIORITY_NULL,    MODEM_ID_0)

#if (FEATURE_VOICE_UP == FEATURE_ON)
    ON_DECLARE(DSP_PID_VOICE_RT,                ACPU_FID_MED_RT,        VOS_PRIORITY_NULL,    MODEM_ID_0)
    ON_DECLARE(DSP_PID_VOICE,                   ACPU_FID_MED,           VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

#if (FEATURE_ON == FEATURE_BASTET)
    ON_DECLARE(ACPU_PID_BASTET_COMM,            ACPU_FID_BASTET_COMM,   VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

#if(FEATURE_PPP == FEATURE_ON)
    ON_DECLARE(PS_PID_APP_PPP,                  ACPU_FID_PPP,           VOS_PRIORITY_NULL,    MODEM_ID_0)
#endif

END_DECLARE_PID_BELONG_TO_FID()
#endif

#endif
#ifdef __cplusplus
#if __cplusplus
}
#endif /* __cpluscplus */
#endif /* __cpluscplus */

/* _V_ID_INC */
