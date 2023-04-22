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

#ifndef __LNASNVINTERFACE_H__
#define __LNASNVINTERFACE_H__

/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include  "AppNasComm.h"


/*****************************************************************************
  1.1 Cplusplus Announce
*****************************************************************************/
#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  #pragma pack(*)    设置字节对齐方式
*****************************************************************************/
#if (VOS_OS_VER != VOS_WIN32)
#pragma pack(4)
#else
#pragma pack(push, 4)
#endif

/*****************************************************************************
  2 Macro
*****************************************************************************/


/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
*****************************************************************************/

/*Added for DATA RETRY PHASEII 2016-05-23 start*/
#define LNAS_NV_DATA_RETRY_EMM_FAIL_CAUSE_EVENT_CNT     (9)
#define LNAS_NV_DATA_RETRY_EMM_FAIL_CAUSE_NUM           (6)

/*Added for DATA RETRY PHASEII 2016-05-23 end*/

/*Added for DATA RETRY PHASEII 2016-05-24 start*/
#define LNAS_NV_MAX_APN_CONTEXT_NUM               6
#define LNAS_NV_MAX_APN_LEN                       99

/*Added for DATA RETRY PHASEII 2016-05-24 end*/
/* mod for Attach/Tau Rej#17#19 Keep Conn, 2016-11-25, Begin */
#define LNAS_NV_ATTACH_REJ_NOT_REL_MAX_CAUSE_NUM  10
#define LNAS_NV_TAU_REJ_NOT_REL_MAX_CAUSE_NUM     10
/* mod for Attach/Tau Rej#17#19 Keep Conn, 2016-11-25, End */
/* Added for BOSTON_R13_CR_PHASEIII 2017-01-16 begin */
#define LNAS_NV_ACDC_APP_MAX_NUM                  4
#define LNAS_NV_ACDC_OSID_LEN                (16)
#define LNAS_NV_ACDC_MAX_APPID_LEN           (128)
/* Added for BOSTON_R13_CR_PHASEIII 2017-01-16 end */

/*Added for MT-DETACH issue 2017-04-13 start*/
#define LNAS_NV_MT_DETACH_OPT_OTHER_CAUSE_CNT (32)
/*Added for MT-DETACH issue 2017-04-13 end*/
/************************stNasFunFlag02 Begin***************************/

/*****************************************************************************
  5 STRUCT
*****************************************************************************/
/*****************************************************************************
结构名    :RRC_PLMN_ID_STRU中
协议表格  :
ASN.1描述 :
结构说明  :
    MCC, Mobile country code (aucPlmnId[0], aucPlmnId[1] bits 1 to 4)
    MNC, Mobile network code (aucPlmnId[2], aucPlmnId[1] bits 5 to 8).

    The coding of this field is the responsibility of each administration but BCD
    coding shall be used. The MNC shall consist of 2 or 3 digits. For PCS 1900 for NA,
    Federal regulation mandates that a 3-digit MNC shall be used. However a network
    operator may decide to use only two digits in the MNC over the radio interface.
    In this case, bits 5 to 8 of octet 4 shall be coded as "1111". Mobile equipment
    shall accept MNC coded in such a way.

    ---------------------------------------------------------------------------
                 ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
    ---------------------------------------------------------------------------
    aucPlmnId[0] ||    MCC digit 2            |           MCC digit 1
    ---------------------------------------------------------------------------
    aucPlmnId[1] ||    MNC digit 3            |           MCC digit 3
    ---------------------------------------------------------------------------
    aucPlmnId[2] ||    MNC digit 2            |           MNC digit 1
    ---------------------------------------------------------------------------

    AT命令：
    at+cops=1,2,"mcc digit 3, mcc digit 2, mcc digit 1, mnc digit 3, mnc digit 2, mnc digit 1",2 :

    e.g.
    at+cops=1,2,"789456",2 :
    --------------------------------------------------------------------------------
    (mcc digit 3)|(mcc digit 2)|(mcc digit 1)|(mnc digit 3)|(mnc digit 2)|(mnc digit 1)
    --------------------------------------------------------------------------------
       7         |     8       |      9      |     4       |      5      |     6
    --------------------------------------------------------------------------------

    在aucPlmnId[3]中的存放格式:
    ---------------------------------------------------------------------------
                 ||(BIT8)|(BIT7)|(BIT6)|(BIT5)|(BIT4)|(BIT3)|(BIT2)|(BIT1)
    ---------------------------------------------------------------------------
    aucPlmnId[0] ||    MCC digit 2 = 8        |           MCC digit 1 = 9
    ---------------------------------------------------------------------------
    aucPlmnId[1] ||    MNC digit 3 = 4        |           MCC digit 3 = 7
    ---------------------------------------------------------------------------
    aucPlmnId[2] ||    MNC digit 2 = 6        |           MNC digit 1 = 5
    ---------------------------------------------------------------------------
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucPlmnId[3];
    VOS_UINT8                           ucReserved;
}LRRC_LNAS_PLMN_ID_STRU;


typedef struct
{
    VOS_UINT32                          bitOpUeNetCap   :1;
    VOS_UINT32                          bitRsv          :31;

    NAS_MM_UE_NET_CAP_STRU              stUeNetCap;
}LNAS_LMM_NV_UE_NET_CAP_STRU;



typedef struct
{
    VOS_UINT32                          bitOpImsi     :1;
    VOS_UINT32                          bitOpRsv      :31;

    VOS_UINT8                           aucImsi[NAS_MM_MAX_UEID_BUF_SIZE];
    VOS_UINT8                           aucReserved1[2];
}LNAS_LMM_NV_IMSI_STRU;

/* LEQUIP_NV使用 */
typedef struct
{
    VOS_UINT32                          bitOpImei     :1;
    VOS_UINT32                          bitOpRsv      :31;

    VOS_UINT8                           aucImei[NAS_MM_MAX_UEID_BUF_SIZE];
    VOS_UINT8                           aucReserved1[2];
}LNAS_LMM_NV_IMEI_STRU;

typedef struct
{
    VOS_UINT32                          bitOpGuti     :1; /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpTai      :1;
    VOS_UINT32                          bitOpUpState  :1;
    VOS_UINT32                          bitOpRsv      :29;

    NAS_MM_GUTI_STRU                    stGuti;
    NAS_MM_TA_STRU                      stLastRegTai;
    NAS_MM_UPDATE_STATE_ENUM_UINT32     enUpdateState;
}LNAS_LMM_NV_EPS_LOC_STRU;


typedef struct
{
    VOS_UINT32                          bitOpMsClassMark :1; /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv      :31;
    NAS_MM_MS_CLASSMARK_STRU            stMsClassMark;
}LNAS_LMM_NV_MS_CLASSMARK_STRU;


typedef struct
{
    VOS_UINT32                          bitOpAccClassMark :1; /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv      :31;
    VOS_UINT16                          usAccClassMark;
    VOS_UINT8                           aucReserved1[2];
}LNAS_LMM_NV_ACC_CLASSMARK_STRU;


typedef struct
{
    VOS_UINT32                          bitOpEpsSec   :1;       /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv      :31;

    VOS_UINT8                           ucKSIsgsn;
    VOS_UINT8                           ucKSIasme;
    VOS_UINT8                           ucSecuAlg;
    VOS_UINT8                           ucRsv;
    VOS_UINT8                           aucKasme[NAS_MM_AUTH_KEY_ASME_LEN];
    VOS_UINT32                          ulUlNasCount;
    VOS_UINT32                          ulDlNasCount;
}LNAS_LMM_NV_EPS_SEC_CONTEXT_STRU;
/* modified 2012-07-31 cs+ps1 begin */
typedef struct
{
    VOS_UINT32                          bitOpUeCenter       :1;       /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv            :31;

    LNAS_LMM_UE_CENTER_ENUM_UINT32      enUeCenter;
}LNAS_LMM_NV_UE_CENTER_STRU;
/* modified 2012-07-31 cs+ps1 end */


typedef struct
{
    VOS_UINT32                          bitOpVoicDomain     :1;       /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv            :31;
    NAS_LMM_VOICE_DOMAIN_ENUM_UINT32    enVoicDomain;
}LNAS_LMM_NV_VOICE_DOMAIN_STRU;

/*R10 modify begin for */
typedef struct
{
    VOS_UINT32                          bitOpReleaseName    :1;       /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv            :31;

    VOS_UINT32                          ulReleaseName;

}LNAS_LMM_NV_NAS_RELEASE_STRU;
/*R10 modify end for */

/* begin for r11 2014-09-18 */
typedef struct
{
    VOS_UINT32                          bitOpLocalIpCap     :1;       /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv            :31;

    VOS_UINT32                          ulLocalIpCap;

}LNAS_LMM_NV_NAS_LOCALIP_CAP_STRU;

/* end for r11 2014-09-18 */

/*self-adaption NW cause modify begin for */
typedef LNAS_LMM_CONFIG_NWCAUSE_STRU        LNAS_LMM_NV_CONFIG_NWCAUSE_STRU;

/*self-adaption NW cause modify end for */


/* mod for AT&T program 2015-01-15 DTS begin */
/* Modified for GU_BACK_OFF,2016-04-07,Begin */
/*****************************************************************************
结构名称    :LNAS_ESM_NV_BACKOFF_CONFIG_PARA_STRU
使用说明    :DAM需求的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                           bitOpBackOffAlg   :1; /* Back-off算法开关     */
    VOS_UINT32                           bitOpBackOffFx    :1; /* NV中Fx值是否有效     */
    VOS_UINT32                           bitOpShareEntityFlag :1;  /* 是否公用一个实体；0 否；1 是 */
    VOS_UINT32                           bitOpRsv          :29;

    NAS_BACKOFF_RAT_SUPPORT_ENUM_UINT32  enBackOffRatSupport;  /* LTE和GU算法生效枚举  */
    NAS_CONFIG_BACKOFF_FX_PARA_STRU      stBackOffFx;          /* Fx参数               */
    NAS_CONFIG_PDP_PERM_CAUSE_STRU       stPdpPermCause;       /* 永久拒绝原因值列表   */
    NAS_CONFIG_PDP_TEMP_CAUSE_STRU       stPdpTempCause;       /* 临时拒绝原因值列表   */
}NAS_BACKOFF_NV_BACKOFF_CONFIG_PARA_STRU;
/* Modified for GU_BACK_OFF,2016-04-07,End */
/* mod for AT&T program 2015-01-15 DTS end */

/*Add for 4G No Account 2015-3-25 DTS start*/
/*****************************************************************************
结构名称    :LNAS_LMM_NV_LTE_NO_ACCOUNT_CONFIG_STRU
使用说明    :针对LTE未开户问题特性NV
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucLteNoSubscribeVplmnSwitch;    /*4G未开户特性开关, VPLMN开关，1: 打开， 0: 关闭*/
    VOS_UINT8                           ucLteNoSubscribeHplmnSwitch;    /*4G未开户特性开关，HPLMN开关, 1: 打开， 0: 关闭*/
    VOS_UINT8                           ucDiscardExtendedEmmCauseFlag;  /*是否忽略网络在attach rej和tau rej中的附加原因值，
                                                                          0:不忽略,1:忽略，该项不受测试卡控制*/
    VOS_UINT8                           ucRsv2;                         /*保留位*/
    VOS_UINT32                          ulPublishmentTimerVplmnLen;     /*惩罚定时器时长VPLMN*/
    VOS_UINT32                          ulPublishmentTimerHplmnLen;     /*惩罚定时器时长HPLMN*/
}LNAS_LMM_NV_LTE_NO_SUBSCRIBE_CONFIG_STRU;
/*Add for 4G No Account 2015-3-25 DTS end*/

/* mod for Attach/Tau Rej#17#19 Keep Conn, 2016-11-25, Begin */
/*****************************************************************************
结构名称    :LNAS_LMM_NV_ATTACHTAU_REJ1719_NOT_REL_STRU
使用说明    :DOCOMO需求ATTACH/TAU被17/19拒绝后不释放链路
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucSwitch;    /*特性开关,1: 打开,即不释放链路 0: 关闭*/
    VOS_UINT8                           ucAttachCauseNum;
    VOS_UINT8                           aucAttachCause[LNAS_NV_ATTACH_REJ_NOT_REL_MAX_CAUSE_NUM];
    VOS_UINT8                           ucTauCauseNum;
    VOS_UINT8                           aucTauCause[LNAS_NV_TAU_REJ_NOT_REL_MAX_CAUSE_NUM];
    VOS_UINT8                           ucRsv;
}LNAS_LMM_NV_ATTACHTAU_REJ_NOT_REL_STRU;
/* mod for Attach/Tau Rej#17#19 Keep Conn, 2016-11-25, End */

/* Added for load balance TAU 2016-12-20 start*/
/*****************************************************************************
结构名称    :LNAS_LMM_NV_LOAD_BALANCE_TAU_CONTROL_CONFIG_STRU
使用说明    :网侧频繁下发释放携带load balance导致UE频繁发TAU调节控制
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucSwitch;                   /*特性开关,1: 打开,即打开当前频繁发TAU的调节控制 0: 关闭*/
    VOS_UINT8                           aucRsv[3];
    VOS_UINT32                          ulThrotCtlTimeLen;          /*调节控制的时长，单位: 毫秒*/
}LNAS_LMM_NV_LOAD_BALANCE_TAU_CONTROL_CONFIG_STRU;
/* Added for load balance TAU 2016-12-20 end*/

/* mod for AT&T program 2015-01-04 DTS begin */
/*****************************************************************************
结构名称    :LNAS_LMM_NV_COMM_CONFIG_PARA_STRU
使用说明    :LNAS关于LMM定制需求的公共NV数据结构，后续LMM相关的公共定制需求NV均在
             此NV中进行增加，便于维护和控制
*****************************************************************************/
typedef struct
{
    /* Added for DATA RETRY PHASEIV, 2016-7-25, begin */
    VOS_UINT8                            ucMaxRej19AtmptCnt; /*  0: 按协议流程直接设置为5 ; 其余值为最大尝试次数*/
    /* Added for DATA RETRY PHASEIV, 2016-7-25, end */
    VOS_UINT8                            ucRsv0[3];           /* 保留，留以后其他需求启用 */
    LNAS_LMM_NV_LTE_NO_SUBSCRIBE_CONFIG_STRU    stLteNoSubscribeConfig; /* 4G未开户所需要NV*/
    VOS_UINT32                           ulRsv1[13];          /* 保留，留以后其他需求启用 */
/* mod for AT&T program phaseIII 2015-03-15 DTS begin */
    NAS_TMO_IMSI_HPLMN_LIST              stTmoImsiHplmnList;  /* TMO定制需求生效的IMSI PLMN列表 */
/* mod for AT&T program phaseIII 2015-03-15 DTS end */
    VOS_UINT8                            ucRsv2[28];          /* 保留，留以后其他需求启用 */
}LNAS_LMM_NV_COMM_CONFIG_PARA_STRU;

/* mod for AT&T program 2015-01-04 DTS end */

typedef struct
{
    VOS_UINT32                          bitOpAttachBearerReest   :1;   /* 1: VALID; 0: INVALID*/
    VOS_UINT32                          bitOpRsv                 :31;
    VOS_UINT32                          ulReestTimeLen;
}LNAS_ESM_NV_ATTACH_BEARER_REEST_STRU;
/* 2015-5-27 begin */
/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_1
 协议表格  :
 ASN.1描述 :
 结构说明  : 协议栈的和协议功能相关的开关的结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32  bitOpKeyInfoFlag                       :1;/* Lnas可维可测关键信息上报控制开关,默认值:1 */
    VOS_UINT32  bitOpKeyEventFlag                      :1;/* Lnas可维可测关键事件上报控制开关,默认值:1 */
    VOS_UINT32  bitOpApiFlag                           :1;/* Lnas可维可测API信息上报控制开关,默认值:1 */
    VOS_UINT32  bitOpSuccRatioFlag                     :1;/* Lnas可维可测成功率信息上报控制开关,默认值:1 */
    VOS_UINT32  bitOpDelayFlag                         :1;/* Lnas可维可测时延信息上报控制开关,默认值:1 */
    VOS_UINT32  bitOpEsrRej39OptimizeFlag              :1;/* Lnas Esr Rej #39被拒优化控制开关,默认值:0 */
    VOS_UINT32  bitOpNasAustraliaFlag                  :1;/* Lnas 澳电定制需求开关,默认值:0 */
    VOS_UINT32  bitOpImsiAttachWithInvalidTinFlag      :1;/* Lnas DSDS2.0开关 */
    VOS_UINT32  bitOpDsdsOptimizeFlag                  :1;/* Lnas SRLTE开关 */
    VOS_UINT32  bitOpNasSrlteFlag                      :1;/* srlte控制开关 */
    VOS_UINT32  bitOpNasT3402DefaultFlag               :1;/* T3402默认定时器控制开关 */
    VOS_UINT32  bitOpThrotAlgSwitchFlag                :1;/* Lnas DATA RETRY特性调节算法开关,默认值:0 */
    VOS_UINT32  bitOpUiccResetClearFlag                :1;/* uicc reset时清除GUTI,LVR TAI，设置EU2的开关 */
    VOS_UINT32  bitOpDataRetryCtrlFlag                 :1;/* Lnas data retry特性开关 */
    VOS_UINT32  bitOpTauRej17OneMoreAttachOptimFlag    :1;/* LNAS TAU#17优化方案控制开关，默认值:0 */
    VOS_UINT32  bitOpIncreaseFreqFlag                  :1;/* LNAS EMM流程发起的时候是否打开提频特性(目前仅涉及ATTACH流程)，默认值:0*/
    /* Mod for DSDS CSFB_FR_DELAY CHR, 2016-08-06, begin */
    VOS_UINT32  bitOpCsfbFrChr                         :1;/* LNAS CSFB FR DEALY CHR上报控制开关，默认值:0 */
    /* Mod for DSDS CSFB_FR_DELAY CHR, 2016-08-06, end */
    /* Added for Boston_R13_CR_PHASEI, 2016-10-18, begin */
    VOS_UINT32  bitOpServiceCounterFlag                :1;/* Lnas Service Counter开关,默认值:0 */
    /* Added for Boston_R13_CR_PHASEI, 2016-10-18, begin */
    /* Added for Boston_R13_CR_PHASEII 2016-12-06 begin */
    VOS_UINT32  bitOpAttachWithImsiFlag                 :1;/* Lnas Attach with IMSI特性开关,默认值:0 */
    /* Added for Boston_R13_CR_PHASEII 2016-12-06 end */
    /* Added for BOSTON_R13_CR_PHASEIII 2017-01-16 begin */
    VOS_UINT32  bitOpAcdcFlag                           :1;/* ACDC特性开关,默认值:0 */
    /* Added for BOSTON_R13_CR_PHASEIII 2017-01-16 end */
    /* Added for Boston_R13_CR_PHASEIII, 2017-01-12, begin */
    VOS_UINT32  bitOpAttachCause19WithoutEmmTimerFlag   :1;/*换APN注册时，停止T3411/T3402的特性开关，默认值关闭:0*/
    /* Added for Boston_R13_CR_PHASEIII, 2017-01-12, end */
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
}LNAS_FUN_FLAG_NV_BIT_STRU_1;

/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_2
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
}LNAS_FUN_FLAG_NV_BIT_STRU_2;

/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_3
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
}LNAS_FUN_FLAG_NV_BIT_STRU_3;

/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_4
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
}LNAS_FUN_FLAG_NV_BIT_STRU_4;

/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_5
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
}LNAS_FUN_FLAG_NV_BIT_STRU_5;

/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_6
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
}LNAS_FUN_FLAG_NV_BIT_STRU_6;

/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_7
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
}LNAS_FUN_FLAG_NV_BIT_STRU_7;

/*****************************************************************************
 结构名    : LNAS_FUN_FLAG_NV_BIT_STRU_8
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
}LNAS_FUN_FLAG_NV_BIT_STRU_8;

/*****************************************************************************
结构名称    :LNAS_LMM_NV_GRADUAL_FORBIDDEN_PARA_STRU
使用说明    :LNAS渐进Forbidden控制NV结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucGradualForbFlag;           /* 特性控制开关, 0: 关; 1: 开 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT16                          usGradualForbTimerFirstLen;  /* 第一次被#15拒, 惩罚定时器时长, 单位分钟 */
    VOS_UINT16                          usGradualForbTimerSecondLen; /* 第二次被#15拒, 惩罚定时器时长, 单位分钟 */
    VOS_UINT32                          ulGradualForbAgingTimerLen;  /* 老化定时器时长, 单位分钟 */
    VOS_UINT16                          usRsv1;
    VOS_UINT16                          usRsv2;
    VOS_UINT16                          usRsv3;
    VOS_UINT16                          usRsv4;
    VOS_UINT32                          ulRsv1;
    VOS_UINT32                          ulRsv2;
    VOS_UINT32                          ulRsv3;
    VOS_UINT32                          ulRsv4;
}LNAS_LMM_NV_GRADUAL_FORBIDDEN_PARA_STRU;

/*****************************************************************************
 结构名    : LNAS_SWITCH_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : LNAS控制开关的结构体(新使用BIT位,必须填写相应说明)
*****************************************************************************/
typedef struct
{
    /*bit位，用于LNAS控制开关*/
    LNAS_FUN_FLAG_NV_BIT_STRU_1                 stNasFunFlag01;
    LNAS_FUN_FLAG_NV_BIT_STRU_2                 stNasFunFlag02;
    LNAS_FUN_FLAG_NV_BIT_STRU_3                 stNasFunFlag03;
    LNAS_FUN_FLAG_NV_BIT_STRU_4                 stNasFunFlag04;
    LNAS_FUN_FLAG_NV_BIT_STRU_5                 stNasFunFlag05;
    LNAS_FUN_FLAG_NV_BIT_STRU_6                 stNasFunFlag06;
    LNAS_FUN_FLAG_NV_BIT_STRU_7                 stNasFunFlag07;
    LNAS_FUN_FLAG_NV_BIT_STRU_8                 stNasFunFlag08;

    LNAS_LMM_NV_GRADUAL_FORBIDDEN_PARA_STRU stGradualForbPara;

    /* 控制开关请使用上面Bit位, 以下为保留位 */
    /*  tau #17 2016-05-20 begin */
    VOS_UINT8                               ucTauRej17MaxTimes;    /* TAU被17拒绝的次数，在这个次数后转换成10启动attach，取值范围1-5 */
    /*  tau #17 2016-05-20 end */
    /* temp_forbidden_TA , 2016-06-17 begin */
    VOS_UINT8                               uc3402TempForbiddenTAFlag;
    /* 0: 优化关闭  */
    /* 1: HPLMN开启 */
    /* 2: VPLMN开启 */
    /* 3: 优化开启  */
    /* temp_forbidden_TA , 2016-06-17 end */
    VOS_UINT8                               ucRsv3;
    VOS_UINT8                               ucRsv4;
    VOS_UINT8                               ucRsv5;
    VOS_UINT8                               ucRsv6;
    VOS_UINT8                               ucRsv7;
    VOS_UINT8                               ucRsv8;
    VOS_UINT8                               ucRsv9;
    VOS_UINT8                               ucRsv10;
    VOS_UINT8                               ucRsv11;
    VOS_UINT8                               ucRsv12;
    VOS_UINT8                               ucRsv13;
    VOS_UINT8                               ucRsv14;
    VOS_UINT8                               ucRsv15;
    VOS_UINT8                               ucRsv16;
    VOS_UINT8                               ucRsv17;
    VOS_UINT8                               ucRsv18;
    VOS_UINT8                               ucRsv19;
    VOS_UINT8                               ucRsv20;
    VOS_UINT8                               ucRsv21;
    VOS_UINT8                               ucRsv22;
    VOS_UINT8                               ucRsv23;
    VOS_UINT8                               ucRsv24;
    VOS_UINT8                               ucRsv25;
    VOS_UINT8                               ucRsv26;
    VOS_UINT8                               ucRsv27;
    VOS_UINT8                               ucRsv28;
    VOS_UINT8                               ucRsv29;
    VOS_UINT8                               ucRsv30;
    VOS_UINT8                               ucRsv31;
    VOS_UINT8                               ucRsv32;
    VOS_UINT8                               ucRsv33;
    VOS_UINT8                               ucRsv34;
    VOS_UINT8                               ucRsv35;
    VOS_UINT8                               ucRsv36;
    VOS_UINT8                               ucRsv37;
    VOS_UINT8                               ucRsv38;
    VOS_UINT8                               ucRsv39;
    VOS_UINT8                               ucRsv40;
    VOS_UINT8                               ucRsv41;
    VOS_UINT8                               ucRsv42;
    VOS_UINT8                               ucRsv43;
    VOS_UINT8                               ucRsv44;
    VOS_UINT8                               ucRsv45;
    VOS_UINT8                               ucRsv46;
    VOS_UINT8                               ucRsv47;
    VOS_UINT8                               ucRsv48;
    VOS_UINT8                               ucRsv49;
    VOS_UINT8                               ucRsv50;
    VOS_UINT8                               ucRsv51;
    VOS_UINT8                               ucRsv52;
    VOS_UINT8                               ucRsv53;
    VOS_UINT8                               ucRsv54;
    VOS_UINT8                               ucRsv55;
    VOS_UINT8                               ucRsv56;
    VOS_UINT8                               ucRsv57;
    VOS_UINT8                               ucRsv58;
    VOS_UINT8                               ucRsv59;
    VOS_UINT8                               ucRsv60;
    VOS_UINT8                               ucRsv61;
    VOS_UINT8                               ucRsv62;
    VOS_UINT8                               ucRsv63;
    VOS_UINT8                               ucRsv64;
    /* temp_forbidden_TA , 2016-06-17 begin */
    VOS_UINT16                              usTempForbTimerLen; /*单位为分钟*/
    /* temp_forbidden_TA , 2016-06-17 end */
    VOS_UINT16                              usRsv2;
    VOS_UINT16                              usRsv3;
    VOS_UINT16                              usRsv4;
    VOS_UINT16                              usRsv5;
    VOS_UINT16                              usRsv6;
    VOS_UINT16                              usRsv7;
    VOS_UINT16                              usRsv8;
    VOS_UINT16                              usRsv9;
    VOS_UINT16                              usRsv10;
    VOS_UINT16                              usRsv11;
    VOS_UINT16                              usRsv12;
    VOS_UINT16                              usRsv13;
    VOS_UINT16                              usRsv14;
    VOS_UINT16                              usRsv15;
    VOS_UINT16                              usRsv16;
    VOS_UINT16                              usRsv17;
    VOS_UINT16                              usRsv18;
    VOS_UINT16                              usRsv19;
    VOS_UINT16                              usRsv20;
    VOS_UINT16                              usRsv21;
    VOS_UINT16                              usRsv22;
    VOS_UINT16                              usRsv23;
    VOS_UINT16                              usRsv24;
    VOS_UINT16                              usRsv25;
    VOS_UINT16                              usRsv26;
    VOS_UINT16                              usRsv27;
    VOS_UINT16                              usRsv28;
    VOS_UINT16                              usRsv29;
    VOS_UINT16                              usRsv30;
    VOS_UINT16                              usRsv31;
    VOS_UINT16                              usRsv32;
    VOS_UINT16                              usRsv33;
    VOS_UINT16                              usRsv34;
    VOS_UINT16                              usRsv35;
    VOS_UINT16                              usRsv36;
    VOS_UINT16                              usRsv37;
    VOS_UINT16                              usRsv38;
    VOS_UINT16                              usRsv39;
    VOS_UINT16                              usRsv40;
    VOS_UINT16                              usRsv41;
    VOS_UINT16                              usRsv42;
    VOS_UINT16                              usRsv43;
    VOS_UINT16                              usRsv44;
    VOS_UINT16                              usRsv45;
    VOS_UINT16                              usRsv46;
    VOS_UINT16                              usRsv47;
    VOS_UINT16                              usRsv48;
    VOS_UINT16                              usRsv49;
    VOS_UINT16                              usRsv50;
    VOS_UINT16                              usRsv51;
    VOS_UINT16                              usRsv52;
    VOS_UINT16                              usRsv53;
    VOS_UINT16                              usRsv54;
    VOS_UINT16                              usRsv55;
    VOS_UINT16                              usRsv56;
    VOS_UINT16                              usRsv57;
    VOS_UINT16                              usRsv58;
    VOS_UINT16                              usRsv59;
    VOS_UINT16                              usRsv60;
    VOS_UINT16                              usRsv61;
    VOS_UINT16                              usRsv62;
    VOS_UINT16                              usRsv63;
    VOS_UINT16                              usRsv64;

    /*Add for attach Vote 2016-09-05 start*/
    VOS_UINT32                              ulCcpuIncreaseFreqValue; /*Ccpu提频的值,单位KHZ*/
    VOS_UINT32                              ulDdrIncreaseFreqValue;  /*Ddr提频的值,单位KHZ*/
    /*Add for attach Vote 2016-09-05 end*/

    VOS_UINT32                              ulRsv3;
    VOS_UINT32                              ulRsv4;
    VOS_UINT32                              ulRsv5;
    VOS_UINT32                              ulRsv6;
    VOS_UINT32                              ulRsv7;
    VOS_UINT32                              ulRsv8;
    VOS_UINT32                              ulRsv9;
    VOS_UINT32                              ulRsv10;
    VOS_UINT32                              ulRsv11;
    VOS_UINT32                              ulRsv12;
    VOS_UINT32                              ulRsv13;
    VOS_UINT32                              ulRsv14;
    VOS_UINT32                              ulRsv15;
    VOS_UINT32                              ulRsv16;
    VOS_UINT32                              ulRsv17;
    VOS_UINT32                              ulRsv18;
    VOS_UINT32                              ulRsv19;
    VOS_UINT32                              ulRsv20;
    VOS_UINT32                              ulRsv21;
    VOS_UINT32                              ulRsv22;
    VOS_UINT32                              ulRsv23;
    VOS_UINT32                              ulRsv24;
    VOS_UINT32                              ulRsv25;
    VOS_UINT32                              ulRsv26;
    VOS_UINT32                              ulRsv27;
    VOS_UINT32                              ulRsv28;
    VOS_UINT32                              ulRsv29;
    VOS_UINT32                              ulRsv30;
    VOS_UINT32                              ulRsv31;
    VOS_UINT32                              ulRsv32;
}LNAS_NV_SWITCH_PARA_STRU;
/* 2015-5-27 end */
typedef struct
{
    VOS_UINT32                          bitOpPeriodicRptTimes:1;                /* 周期上报次数控制特性开关 */
    VOS_UINT32                          bitOp2:1;
    VOS_UINT32                          bitOp3:1;
    VOS_UINT32                          bitOp4:1;
    VOS_UINT32                          bitOp5:1;
    VOS_UINT32                          bitOp6:1;
    VOS_UINT32                          bitOp7:1;
    VOS_UINT32                          bitOp8:1;
    VOS_UINT32                          bitOp9:1;
    VOS_UINT32                          bitOp10:1;
    VOS_UINT32                          bitOp11:1;
    VOS_UINT32                          bitOp12:1;
    VOS_UINT32                          bitOp13:1;
    VOS_UINT32                          bitOp14:1;
    VOS_UINT32                          bitOp15:1;
    VOS_UINT32                          bitOp16:1;
    VOS_UINT32                          bitOp17:1;
    VOS_UINT32                          bitOp18:1;
    VOS_UINT32                          bitOp19:1;
    VOS_UINT32                          bitOp20:1;
    VOS_UINT32                          bitOp21:1;
    VOS_UINT32                          bitOp22:1;
    VOS_UINT32                          bitOp23:1;
    VOS_UINT32                          bitOp24:1;
    VOS_UINT32                          bitOp25:1;
    VOS_UINT32                          bitOp26:1;
    VOS_UINT32                          bitOp27:1;
    VOS_UINT32                          bitOp28:1;
    VOS_UINT32                          bitOp29:1;
    VOS_UINT32                          bitOp30:1;
    VOS_UINT32                          bitOp31:1;
    VOS_UINT32                          bitOp32:1;
}LNAS_LCS_NV_FEATURE_BIT_STRU;

/*Add for Lcs 2015-10-13 start*/
typedef struct
{
    LNAS_LCS_NV_FEATURE_BIT_STRU        stLcsFeatureBit;

    VOS_UINT8                           ucMaxRetryTimes;                        /* 最大重新尝试次数 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
    VOS_UINT8                           ucRsv4;
    VOS_UINT8                           ucRsv5;
    VOS_UINT8                           ucRsv6;
    VOS_UINT8                           ucRsv7;
    VOS_UINT8                           ucRsv8;
    VOS_UINT8                           ucRsv9;
    VOS_UINT8                           ucRsv10;
    VOS_UINT8                           ucRsv11;
    VOS_UINT8                           ucRsv12;
    VOS_UINT8                           ucRsv13;
    VOS_UINT8                           ucRsv14;
    VOS_UINT8                           ucRsv15;
    VOS_UINT8                           ucRsv16;
    VOS_UINT8                           ucRsv17;
    VOS_UINT8                           ucRsv18;
    VOS_UINT8                           ucRsv19;
    VOS_UINT8                           ucRsv20;
    VOS_UINT8                           ucRsv21;
    VOS_UINT8                           ucRsv22;
    VOS_UINT8                           ucRsv23;
    VOS_UINT16                          usRetryTimerLen;                        /* 重新尝试定时器时长, 超时后会重新建链 */
    VOS_UINT16                          usPeriodicRptTimes;                     /* MO-LR为周期上报时,设置周期上报次数 */
    VOS_UINT16                          usRsv2;
    VOS_UINT16                          usRsv3;
    VOS_UINT16                          usRsv4;
    VOS_UINT16                          usRsv5;
    VOS_UINT16                          usRsv6;
    VOS_UINT16                          usRsv7;
    VOS_UINT16                          usRsv8;
    VOS_UINT16                          usRsv9;
    VOS_UINT16                          usRsv10;
    VOS_UINT16                          usRsv11;
    VOS_UINT16                          usRsv12;
    VOS_UINT16                          usRsv13;
    VOS_UINT16                          usRsv14;
    VOS_UINT16                          usRsv15;
    VOS_UINT32                          ulRsv0;
    VOS_UINT32                          ulRsv1;
    VOS_UINT32                          ulRsv2;
    VOS_UINT32                          ulRsv3;
    VOS_UINT32                          ulRsv4;
    VOS_UINT32                          ulRsv5;
    VOS_UINT32                          ulRsv6;
    VOS_UINT32                          ulRsv7;
    VOS_UINT32                          ulRsv8;
    VOS_UINT32                          ulRsv9;
    VOS_UINT32                          ulRsv10;
    VOS_UINT32                          ulRsv11;
    VOS_UINT32                          ulRsv12;
    VOS_UINT32                          ulRsv13;
    VOS_UINT32                          ulRsv14;
    VOS_UINT32                          ulRsv15;
}LNAS_LCS_NV_COMMON_CONFIG_STRU;
/*Add for Lcs 2015-10-13 end*/

/* Added for Boston_R13_CR_PHASEI 2016-10-17 begin */
/*****************************************************************************
结构名称    :LNAS_LMM_NV_T3402_INFO_STRU
使用说明    :LNAS关于T3402定时器的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucIsT3402DefaultValue;                  /* 3402定时器是否为默认时长，0否，1是 */
    VOS_UINT8                           ucRsv[3];                               /* 保留 */
    VOS_UINT32                          ul3402Len;                              /* 3402定时器时长，单位:秒 */
    NAS_MM_PLMN_LIST_STRU               stT3402EPlmnList;                       /* 3402定时器时长生效的EPLMNLIST */
}LNAS_LMM_NV_T3402_INFO_STRU;

/* Added for Boston_R13_CR_PHASEI 2016-10-17 end */

/* Added by for DATA RETRY PHASEI, 2016-03-21, Begin */
/*****************************************************************************
结构名称    :LNAS_LMM_NV_T3402_CTRL_STRU
使用说明    :LNAS关于DATA RETRY特性T3402定时器针对PLMN列表维护的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucT3402PlmnCtrlSwitch;                  /* T3402针对PLMN有效开关，0关闭，1打开 */
    VOS_UINT8                           ucT3402RmLenStore;                      /* T3402剩余时长是否需要保存，0否，1是 */
    VOS_UINT8                           ucRsv1;                                 /* 保留 */
    VOS_UINT8                           ucT3402PlmnNum;                         /* astT3402PlmnList中有效单元个数 */
    NAS_PLMN_T3402_STRU                 astT3402PlmnList[NAS_MM_MAX_T3402_PLMN_NUM];
    VOS_UINT32                          ulT3402DefaultValue;                    /* 3402定时器默认时长，单位:秒 */

}LNAS_LMM_NV_T3402_CTRL_STRU;
/* Added by for DATA RETRY PHASEI, 2016-03-21, End */

/*Added for DATA RETRY PHASEII 2016-05-23 start*/
typedef struct
{
    VOS_UINT32                                  ulRemainLogTime;    /*当前事件关机时候还剩余的时间,单位为秒*/
    VOS_UINT16                                  usEventCnt;         /*当前原因值事件的计数*/
    VOS_UINT8                                   ucRsv1;
    VOS_UINT8                                   ucRsv2;
    NAS_EMM_PLMN_ID_STRU                        stPlmnId;           /*当前PLMN ID*/
    NAS_EMM_TAC_STRU                            stTac;              /*当前Tac*/
}LNAS_EMM_DATA_RETRY_FAIL_EVENT_STRU;

typedef struct
{
    VOS_UINT8                                   ucEmmcause;         /*原因值ID*/
    VOS_UINT8                                   ucRsv1;
    VOS_UINT8                                   ucRsv2;
    VOS_UINT8                                   ucRsv3;
    VOS_UINT32                                  ulEmmFailEventCnt;  /*表示当前原因值列表中存储的有效个数*/
    LNAS_EMM_DATA_RETRY_FAIL_EVENT_STRU         astEmmFailEvent[LNAS_NV_DATA_RETRY_EMM_FAIL_CAUSE_EVENT_CNT];  /*具体原因值事件列表的list*/
}LNAS_EMM_DATA_RETRY_FAIL_EVENT_LIST_STRU;

typedef struct
{
    VOS_UINT32                                  aulMaxLogTime[LNAS_NV_DATA_RETRY_EMM_FAIL_CAUSE_NUM];       /*表示事件存储的最大时间*/
    VOS_UINT8                                   aucMaxEventCounter[LNAS_NV_DATA_RETRY_EMM_FAIL_CAUSE_NUM];  /*表示事件在ulMaxLogTime时间内尝试的最大次数*/
    VOS_UINT8                                   ucRsv1;
    VOS_UINT8                                   ucRsv2;
	LNAS_EMM_DATA_RETRY_FAIL_EVENT_LIST_STRU    astEventCause[LNAS_NV_DATA_RETRY_EMM_FAIL_CAUSE_NUM];
}LNAS_EMM_DATA_RETRY_NV_PARA_CONFIG_STRU;


typedef struct
{
    VOS_UINT8                           ucApnLen;
    VOS_UINT8                           aucApnName[LNAS_NV_MAX_APN_LEN];
}LNAS_APN_INFO_STRU;

/* Added for DATA RETRY PHASEIII, 2016-6-23, begin */
/*****************************************************************************
结构名称    :LNAS_APN_PRIO_INFO_STRU
使用说明    :
*****************************************************************************/
typedef struct
{
    LNAS_APN_INFO_STRU                  stApnInfo;                              /* APN 信息 */
    NAS_ESM_BEARER_PRIO_ENUM_UINT32     ulBearPrio;                             /* 承载优先级*/
}LNAS_APN_PRIO_INFO_STRU;
/* Added for DATA RETRY PHASEIII, 2016-6-23, end */

typedef struct
{
    LNAS_APN_INFO_STRU                  stApnInfo;                          /* APN                */
    VOS_UINT32                          ulRemainTimerLen;                   /*当前APN关机时候调节定时器还剩余的时间*/
}LNAS_ESM_APN_THROT_REMAIN_TIME_INFO_STRU;

typedef struct
{
    VOS_UINT32                                  ulWaitTime;            /* 在PDN去连接之后，在waittime之后才能发起PDN建立流程,单位:s*/
    VOS_UINT32                                  ulPdnMaxConnTime;      /* 最大连接时间，标识每一个PDN第一次发起以来经过的最大时间，单位:s*/
    VOS_UINT32                                  ulPdnMaxConnCount;     /* 最大连接次数，在最大连接时间，允许发起PDN的最大连接次数*/
    VOS_UINT8                                   ucApnTotalNum;         /* 正在使用的APN上下文总数 */
    VOS_UINT8                                   aucRsv[3];              /* 保留 */

    /* PDN调节定时器启动期间关机，维护剩余时间列表，连续存储*/
    LNAS_ESM_APN_THROT_REMAIN_TIME_INFO_STRU    astApnThrotRemainTimeInfo[LNAS_NV_MAX_APN_CONTEXT_NUM];
}LNAS_ESM_DATA_RETRY_NV_PARA_CONFIG_STRU;

/*Added for DATA RETRY PHASEII 2016-05-23 end*/

/* Added for DSDS OPTIMIZE MT DETACH BY TAU, 2016-06-16, Begin */
/*****************************************************************************
结构名称    :LNAS_LMM_NV_MT_DETACH_TAU_CTRL_STRU
使用说明    :LNAS关于产品线定制的周期性收到网络层DETACH，因DSDS可能无法收到
             网侧DETACH,导致UE长时间被叫不通问题的定制NV数据结构.

             (1)当ucPlmnNum = 1,且列表中该PLMN设为全F,则特性适用所有PLMN;
             (2)其他情况,特性只适用于驻留在此列表中的PLMN。
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucActiveFlag;                           /* 特性开关, 1:特性开启，0:打特性关闭 */
    VOS_UINT8                           ucRsv0;                                 /* 保留，留以后其他需求启用 */
    VOS_UINT8                           ucRsv1;                                 /* 保留，留以后其他需求启用 */
    VOS_UINT8                           ucPlmnNum;                              /* 特性适用的PLMN个数  */
    NAS_MM_PLMN_ID_STRU                 astPlmnId[NAS_MT_DETACH_TAU_PLMN_MAX_NUM];/* 特性适用的具体PLMN列表。 */
}LNAS_LMM_NV_MT_DETACH_TAU_CTRL_STRU;
/* Added for DSDS OPTIMIZE MT DETACH BY TAU, 2016-06-16, End */

/* Added for network not include eps_network_feature_support IE  ,2017-08-25,begin */
/*****************************************************************************
结构名称    :LNAS_LMM_NV_NETWORK_FEATURE_VOPS_OPTIMIZE_CTRL_STRU
使用说明    :此NV用于解决沙特运营商zain网络未在ATTACH_ACP中携带eps_network_feature_support
             可选信元时,VOLTE无法进行IMS注册的问题.
             网络未携带eps network feature support信元时,正常按照协议的处理是把信元相关
             项都置为不支持;此NV开关打开时,为了解决IMS无法注册问题,需要把此信元的VOPS能力项
             设为支持.
             此NV由终端定制是否打开。
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucNetVopsOptimizeCtrl;                  /* 特性开关, 1:特性开启，0:特性关闭. 默认关闭 */
    VOS_UINT8                           ucRsv0;                                 /* 保留，留以后其他需求启用 */
    VOS_UINT8                           ucRsv1;                                 /* 保留，留以后其他需求启用 */
    VOS_UINT8                           ucRsv2;                                 /* 保留，留以后其他需求启用 */
}LNAS_LMM_NV_NETWORK_FEATURE_VOPS_OPTIMIZE_CTRL_STRU;
/* Added for network not include eps_network_feature_support IE  ,2017-08-25,end */

/* Added for DATA RETRY PHASEIII, 2016-6-23, begin */
typedef struct
{
    LNAS_APN_PRIO_INFO_STRU             stApnAndPrioInfo;                       /* APN and Prio INFO */
    VOS_UINT32                          ulT3396RemainLenForNonCustom;           /* 单位为秒 */
    VOS_UINT8                           ucPlmnNum;                              /* PLMN个数 */
    VOS_UINT8                           aucRsv[3];
    NAS_PLMN_T3396_STRU                 astT3396PlmnList[NAS_MM_MAX_T3396_PLMN_NUM];
}LNAS_ESM_T3396_APN_PLMN_INFO_STRU;
/*****************************************************************************
结构名称    :LNAS_ESM_NV_T3396_CTRL_CONFIG_STRU
使用说明    :LNAS关于DATA RETRY特性T3396定时器针对PLMN/APN列表维护的NV数据结构
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucT3396CtrlSwitch;                      /* T3396特性开关，0关闭，1打开 */
    VOS_UINT8                           ucIsPlmnCustom;                         /* PLMN定制标识，0非定制，1定制 */
    VOS_UINT8                           ucApnTotalNum;                          /* 正在使用的APN上下文总数 */
    VOS_UINT8                           ucRsv;                                  /* 保留 */

    LNAS_ESM_T3396_APN_PLMN_INFO_STRU    astT3396ApnPlmnInfo[LNAS_NV_MAX_APN_CONTEXT_NUM];

}LNAS_ESM_NV_T3396_CTRL_CONFIG_STRU;
/* Added for DATA RETRY PHASEIII, 2016-6-23, end */

/*Added for DATA RETRY PHASEIII 2016-06-21 start*/
/*****************************************************************************
结构名称    :LNAS_LMM_NV_EAB_CONFIG_STRU
使用说明    :LNAS关于EAB特性的NV数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucUeCapacityLowPri;             /* UE能力，是否支持信令低优先级， 0不支持，1支持 */
    VOS_UINT8                           ucUeCapacityEab;                /* UE能力，是否支持EAB， 0不支持，1支持 */
    VOS_UINT8                           ucEabFlag;                      /* EAB特性开关，0不支持，1支持 */
    VOS_UINT8                           ucSigLowPriFlag;                /* NAS Signalling Priority 标志位，0非低优先级，1低优先级 */
    VOS_UINT8                           ucOverrideSigLowPriFlag;        /* NAS Signalling Priority 重写标志，0不可重写，1可重写 */
    VOS_UINT8                           ucOverrideEabFlag;              /* EAB开关重写标志，0不可重写，1可重写 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
}LNAS_LMM_NV_EAB_CONFIG_STRU;

/*Added for DATA RETRY PHASEIII 2016-06-21 end*/

/* Added  for DATA RETRY PHASEIV, 2016-07-25, begin */
/*****************************************************************************
结构名称    :LNAS_EMM_NV_T3346_CTRL_CONFIG_STRU
使用说明    :LNAS T3346定时器的NV数据结构
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucT3346CtrlSwitch;                      /* T3346特性开关，0关闭，1打开 */
    VOS_UINT8                           ucNasSigPrio;                           /* NAS Signalling Priority ，0非低优先级，1低优先级 */
    VOS_UINT8                           ucRsv1;                                 /* 保留 */
    VOS_UINT8                           ucRsv2;                                 /* 保留 */

    VOS_UINT32                          ulT3346RemainLen;                       /* 单位为秒 */
    NAS_MM_PLMN_ID_STRU                 stPlmnId;                               /* PLMN */
    NAS_MM_PLMN_LIST_STRU               stEPlmnList;                            /* equivalent PLMN list */

}LNAS_EMM_NV_T3346_CTRL_CONFIG_STRU;
/* Added  for DATA RETRY PHASEIV, 2016-07-25, end */

/* Added for DATA RETRY PHASEIV, 2016-7-25, begin */
/*****************************************************************************
结构名称    :LNAS_ESM_NV_APN_SWITCH_CTRL_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucApnSwitchForVZW;          /*APN SWTICH 功能开关: 1 开启；0 关闭*/
    VOS_UINT8                           ucApnSwitchForNonVZW;      /*APN SWTICH 功能开关: 1 开启；0 关闭*/
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;

}LNAS_ESM_NV_APN_SWITCH_CTRL_CONFIG_STRU;
/* Added for DATA RETRY PHASEIV, 2016-7-25, end */
/* Added  for Boston_R13_CR_PHASEIII, 2017-01-16, Begin */
/*****************************************************************************
结构名称    :LNAS_ESM_NV_BACKOFF_CTRL_CONFIG_STRU
使用说明    :LNAS关于back-off的NV数据结构
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucBackOffCtrlSwitch;                    /* back-off特性开关，0关闭，1打开 */
    VOS_UINT8                           aucRsv[3];                              /* 保留 */

}LNAS_ESM_NV_BACKOFF_CTRL_CONFIG_STRU;
/* Added  for Boston_R13_CR_PHASEIII, 2017-01-16, End */

/* Added for ,2016-10-28,Begin */
/*****************************************************************************
结构名称    :LNAS_EMM_NV_HO_TAU_DELAY_CTRL_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucSwitchFlag;              /*SWTICH 功能开关: 1 开启；0 关闭,默认开启*/
    VOS_UINT8                           aucRsv[3];

    VOS_UINT32                          ulHoTauDelayTimeLen;      /*HO TAU DELAY 定时器时长 ms */
}LNAS_EMM_NV_HO_TAU_DELAY_CTRL_CONFIG_STRU;

/*****************************************************************************
结构名称    :LNAS_ESM_NV_NDIS_CONN_DELAY_CTRL_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucSwitchFlag;              /* SWTICH 功能开关: 1 开启；0 关闭,默认开启*/
    VOS_UINT8                           aucRsv[3];

    VOS_UINT32                          ulNdisConnDelayTimeLen;      /*NDIS CONN DELAY 定时器时长 ms */
}LNAS_ESM_NV_NDIS_CONN_DELAY_CTRL_CONFIG_STRU;

/* Added for ,2016-10-28,End */

/* add for separate special part from original NV  , 2016-11-07, begin */
/*****************************************************************************
 结构名    : LNAS_AUSTRALIA_FLAG_CONFIG_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 澳电定制的控制开关结构体(新使用BIT位,必须填写相应说明)
*****************************************************************************/
typedef struct
{
    VOS_UINT32      ulAustraliaFlag;
    VOS_UINT32      ulRsv;
}LNAS_LMM_NV_AUSTRALIA_FLAG_CONFIG_STRU;
/* add for separate special part from original NV  , 2016-11-07, end */
/* Added for MTU_REQUIRE,2016-11-12,Begin */
/*****************************************************************************
结构名称    :LNAS_ESM_NV_NDIS_CONN_DELAY_CTRL_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucIpv4MtuForImsApnFlag;             /* 针对IMS APN是否请求ipv4 mtu: 1 请求；0 不请求,默认请求*/
    VOS_UINT8                           ucIpv4MtuForOtherApnFlag;           /* 针对其他 APN是否请求ipv4 mtu: 1 请求；0 不请求,默认不请求*/
    VOS_UINT8                           aucRsv[2];
}LNAS_ESM_NV_IPV4_MTU_CTRL_CONFIG_STRU;
/* Added for MTU_REQUIRE,2016-11-12,End */
/* Added for BOSTON_R13_CR_PHASEII,2016-12-05,Begin */
/*****************************************************************************
结构名称    :LNAS_ESM_NV_NDIS_CONN_DELAY_CTRL_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucSwitchFlag;                       /* 特性是否开启: 1 开启；0不开启*/
    VOS_UINT8                           aucRsv[3];
    VOS_UINT8                           ucUsimInvalidForGprsMaxValue;       /* GRPS业务卡无效计数最大值 */
    VOS_UINT8                           ucUsimInvalidForNonGprsMaxValue;    /* 非GRPS业务卡无效计数最大值 */
    VOS_UINT8                           ucPlmnSpecificAttemptMaxValue;      /* 特定PLMN尝试计数最大值 */
    VOS_UINT8                           ucPlmnSpecificPsAttemptMaxValue;    /* 特定PLMN PS尝试计数最大值 */
}LNAS_EMM_NV_PLAIN_NAS_REJ_MSG_CTRL_CONFIG_STRU;
/* Added for BOSTON_R13_CR_PHASEII,2016-12-05,End */
/* Added for MO_DETACH_REL,2017-01-05,Begin */
/*****************************************************************************
结构名称    :LNAS_ESM_NV_NDIS_CONN_DELAY_CTRL_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucSwitchFlag;                       /* 特性是否开启: 1 开启；0不开启*/
    VOS_UINT8                           ucMaxDetachAttemptCnt;              /* detach最大尝试次数 */
    VOS_UINT8                           aucRsv[2];

}LNAS_EMM_NV_DETACH_ATTEMPT_CNT_CTRL_CONFIG_STRU;
/* Added for MO_DETACH_REL,2017-01-05,End */



/* Added by 2016-12-14 for KDDI,begin */
/*****************************************************************************
结构名称    :LNAS_ESM_NV_CHANGE_TO_IMSAPN_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucChangeToImsApnFlag;             /* ATTACH被拒后，是否要替换为IMS APN，VOS_TRUE:需要；VOS_fALSE_不需要*/
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
}LNAS_ESM_NV_CHANGE_TO_IMSAPN_CONFIG_STRU;
/* Added by 2016-12-14 for KDDI,end */
/* Added for MODETACH_ATTACH_COLLISION,2017-01-05,Begin */
/*****************************************************************************
结构名称    :LNAS_EMM_NV_STORE_MMC_DETACH_CTRL_CONFIG_STRU
使用说明    :
*****************************************************************************/

typedef struct
{
    VOS_UINT8                           ucSwitchFlag;                       /* 特性是否开启: 1 开启；0不开启*/
    VOS_UINT8                           aucRsv[3];

}LNAS_EMM_NV_STORE_MMC_DETACH_CTRL_CONFIG_STRU;


/* Added for MODETACH_ATTACH_COLLISION,2017-01-05,End */

/*Added for BOSTON_R13_CR_PHASEIII 2017-01-16 begin */
/*****************************************************************************
结构名称    :LNAS_LMM_NV_ACDC_APP_STRU
使用说明    :LNAS关于ACDC特性APP-ACDCcategory的数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                               aucOsId[LNAS_NV_ACDC_OSID_LEN];        /* OSID */
    VOS_UINT8                                               aucAppId[LNAS_NV_ACDC_MAX_APPID_LEN];      /* APPID */
    VOS_UINT8                                               ucAcdcCategory;     /* APPID */
    VOS_UINT8                                               ucRsv[3];           /* 保留 */
}LNAS_LMM_NV_ACDC_APP_STRU;

/*Added for MT-DETACH issue 2017-04-13 start*/
/*****************************************************************************
结构名称    :NAS_EMM_MT_DETACH_OPTIMIZE_STRU
使用说明    :MT-DETACH优化的结构
*****************************************************************************/
typedef struct
{
    /*
    表示MT-detach re-attach-not-required 携带原因值，协议明确规定了如下的原因值:
    2、3、6、7、8、11、12、13、14、15、25 这些原因值协议有明确规定了处理原则，除开
    之外的其他原因值，统一当成other原因值处理了，本次优化针对other原因值。
    */
    VOS_UINT8                           aucMtDetachOptForGivenCauseList[LNAS_NV_MT_DETACH_OPT_OTHER_CAUSE_CNT]; /*MT-DETACH优化的原因值列表*/
    VOS_UINT16                          usRsv1;
    VOS_UINT16                          usRsv2;
    VOS_UINT16                          usRsv3;
    VOS_UINT16                          usRsv4;
    VOS_UINT32                          ulRsv1;
    VOS_UINT32                          u1Rsv2;
    VOS_UINT32                          ulRsv3;
}LNAS_LMM_NV_MT_DETACH_WITH_OPTIMZIE_CONFIG_STRU;
/*Added for MT-DETACH issue 2017-04-13 end*/


/*****************************************************************************
结构名称    :LNAS_LMM_NV_ACDC_APP_LIST_STRU
使用说明    :LNAS关于ACDC特性APP列表的数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulAppNum;                               /* APP列表个数 */
    LNAS_LMM_NV_ACDC_APP_STRU           astAcdcAppList[LNAS_NV_ACDC_APP_MAX_NUM]; /* APP列表 */
}LNAS_EMM_NV_ACDC_CONFIG_STRU;
/*Added for BOSTON_R13_CR_PHASEIII 2017-01-16 end */

/* add for separate special part from original NV  , 2017-03-07, begin */
/*****************************************************************************
结构名    : LNAS_LMM_NV_DSDS_OPTIMIZE_FLAG_CONFIG_STRU
协议表格  :
ASN.1描述 :
结构说明  : DSDS2.0优化的控制开关结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucDsdsOptimizeFlag;                 /* 特性是否开启: 1:开启；0:关闭 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
}LNAS_LMM_NV_DSDS_OPTIMIZE_FLAG_CONFIG_STRU;

/*****************************************************************************
结构名    : LNAS_LMM_NV_SRLTE_FLAG_CONFIG_STRU
协议表格  :
ASN.1描述 :
结构说明  : SRLTE的控制开关结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucSrlteFlag;                       /* 特性是否开启: 1:开启；0:关闭 */
    VOS_UINT8                           ucRsv1;
    VOS_UINT8                           ucRsv2;
    VOS_UINT8                           ucRsv3;
}LNAS_LMM_NV_SRLTE_FLAG_CONFIG_STRU;
/* add for separate special part from original NV  , 2017-03-07, end */

/*****************************************************************************
  6 UNION
*****************************************************************************/


/*****************************************************************************
  7 Extern Global Variable
*****************************************************************************/


/*****************************************************************************
  8 Fuction Extern
*****************************************************************************/

/*****************************************************************************
  9 OTHERS
*****************************************************************************/










#if (VOS_OS_VER != VOS_WIN32)
#pragma pack()
#else
#pragma pack(pop)
#endif




#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of MmcEmmInterface.h */




