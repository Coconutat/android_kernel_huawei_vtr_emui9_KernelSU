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

#ifndef __LNVCOMMON_H__
#define __LNVCOMMON_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
/* this file is included by drv, submit by zhouluojun, reviewed by hujianbo */
/* #pragma pack(4) */

/*****************************************************************************
  2 macro
*****************************************************************************/

/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
    PS NV值使用范围：0xD200 ~ 0xD38F
*****************************************************************************/
enum NV_ITEM_ID_ENUM
{
    EN_NV_ID_BEGIN                                  = 0,
    /*mod for LNAS NV migrate because of conflict with MBB product,begin 2017-02-23*/
    /*LNAS NV迁移背景:Boston NV归一化项目中给IMSA分配的区间[50501,53000]中的[50501,52000]这块区间和MBB产品线
      冲突，经过海思讨论将冲突区间让给MBB产品线，并给LNAS重新划分一块区间[31000,32500],所以这里将冲突区间的nvid
      减去4C2C调整到新区间中*/
    EN_NV_ID_LNAS_NEW_NV_INTERVAL_BEGIN             = 0x7918,/* LNAS NEW ALLOCATE NV INTERVAL BEGIN*/
    EN_NV_ID_IMS_BEGIN                              = 0x7919,/* IMS BEGIN */
    EN_NV_ID_IMS_CONTROL                            = 0x791A,/* IMS总开关 NV, for IMS support flag, 2016-10-11*/
    EN_NV_ID_IMS_SIP_TIMER                          = 0x791B,/* SIP定制器, Added by   for KT, 2016-12-05 */
    /* Added 2017-3-9 for emc category urn begin*/
    EN_NV_ID_IMS_EMC_CAT_URN_CONFIG                 = 0x791C,
    /* Added 2017-3-9 for emc category urn end*/
    EN_NV_ID_IMS_USER_AGENT_CONFIG                  = 0x791D,
    EN_NV_ID_IMS_END                                = 0x797C,/* IMS END */
    EN_NV_ID_LNAS_EMM_BEGIN                         = 0x797D,/* EMM BEGIN */
    EN_NV_ID_LNAS_EMM_HO_TAU_DELAY_CTRL_CONFIG      = 0x797E,/* TAU REQ delay*/
    /* add for separate special part from original NV , 2016-11-07, begin */
    EN_NV_ID_LNAS_AUSTRALIA_FLAG_CONFIG             = 0x797F,
    EN_NV_ID_LNAS_LTE_NO_SUBSCRIBE_CONFIG           = 0x7980,
    /* add for separate special part from original NV , 2016-11-07, end */
    /* Added for Boston_R13_CR_PHASEI 2016-10-17 begin */
    EN_NV_ID_NAS_T3402_CTRL_CONFIG                  = 0x7981,
    /* Added for Boston_R13_CR_PHASEI 2016-10-17 end */
    /* mod for Attach/Tau Rej#17#19 Keep Conn, 2016-11-25, Begin */
    EN_NV_ID_LNAS_ATTACH_TAU_REJ_NOT_RELEASE        = 0x7982,
    /* mod for Attach/Tau Rej#17#19 Keep Conn, 2016-11-25, End */
    /* Added for BOSTON_R13_CR_PHASEII,2016-12-05,Begin */
    EN_NV_ID_NAS_PLAIN_NAS_REJ_MSG_CTRL_CONFIG      = 0x7983,
    /* Added for BOSTON_R13_CR_PHASEII,2016-12-05,End */
    /* Added for load balance TAU 2016-12-20 start*/
    EN_NV_ID_LNAS_LOAD_BALANCE_TAU_CONTROL_CONFIG   = 0x7984,
    /* Added for load balance TAU 2016-12-20 end*/
    /* Added for MO_DETACH_REL,2017-01-05,Begin */
    EN_NV_ID_NAS_DETACH_ATTEMPT_CNT_CTRL_CONFIG     = 0x7985,/* detach过程中收到mmc释放后最大尝试次数*/
    /* Added for MO_DETACH_REL,2017-01-05,End */
    /* Added for MODETACH_ATTACH_COLLISION,2017-01-05,Begin */
    EN_NV_ID_NAS_STORE_MMC_DETACH_CTRL_CONFIG       = 0x7986,/* ATTACH过程中收到MMC DETACH是否缓存控制 */
    /* Added for MODETACH_ATTACH_COLLISION,2017-01-05,End */
    /* Added for BOSTON_R13_CR_PHASEIII 2017-01-16 begin */
    EN_NV_ID_NAS_ACDC_CTRL_CONFIG                   = 0x7987,
    /* Added for BOSTON_R13_CR_PHASEIII 2017-01-16 end */
 /* Added for Boston_R13_CR_PHASEIII, 2017-01-16, Begin */
    EN_NV_ID_NAS_BACKOFF_CTRL_CONFIG                = 0x7988,
    /* Added for Boston_R13_CR_PHASEIII, 2017-01-16, End */
    /* add for separate special part from original NV , 2017-03-07, begin */
    /* 解耦拆分新增的NV只在BOSTON版本上生效 */
    EN_NV_ID_LNAS_DSDS_OPTIMIZE_FLAG_CONFIG         = 0x7989,/* DSDS2.0优化控制开关 */
    EN_NV_ID_LNAS_SRLTE_FLAG_CONFIG                 = 0x798A,/* SRLTE控制开关 */
    EN_NV_ID_LNAS_TMO_IMSI_HPLMN_LIST_CONFIG        = 0x798B,/* TMO定制需求生效的IMSI HPLMN列表 */
    /* add for separate special part from original NV , 2017-03-07, end */

    EN_NV_ID_LNAS_MT_DETACH_WITH_OPTIMZIE_CONFIG    = 0x798f,
    /* Added for network not include eps_network_feature_support IE ,2017-08-25,begin */
    EN_NV_ID_NETWORK_FEATURE_VOPS_OPTIMIZE_CTRL     = 0x7999, /* 此NV用于解决沙特运营商ZAIN网络未携带eps network feature support信元时,非VOLTE用户无法进行IMS注册的问题 */
    /* Added for network not include eps_network_feature_support IE ,2017-08-25,end */
    EN_NV_ID_LNAS_EMM_END                           = 0x7BD4,/* EMM END */
    EN_NV_ID_LNAS_IMSA_BEGIN                        = 0x7BD5,/* IMSA BEGIN */
    EN_NV_ID_IMSA_DSDS_OPT_CONFIG                   = 0x7BD6,
    /* Added 2016-09-26 for sbm,begin */
    EN_NV_ID_SBM_FEATURE_CONFIG                     = 0x7BD7,
    /* Added 2016-09-26 for sbm,end */
    /* begin for emc pdn rej retry 2016-11-16 */
    EN_NV_ID_EMC_PDN_REJ_CONFIG                     = 0x7BD8,/* 紧急PDN建立被拒重试NV */
    /* end for emc pdn rej retry 2016-11-16 */
    EN_NV_ID_IMSA_REREG_CTRL                        = 0x7BD9,/* Added by   for Rogers_volte_II, 2016-11-19 */
    /* added for VISP Wakeup Compens Timer Issue, begin in 2016-12-20 */
    EN_NV_ID_VISP_WAKEUP_COMPENS_TIMER_CTRL         = 0x7BDA,
    /* added for VISP Wakeup Compens Timer Issue, end in 2016-12-20 */
    /* Added 2016-12-12 for KDDI,begin */
    EN_NV_ID_JAPAN_FEATURE_CONFIG                   = 0x7BDB,
    /* Added 2016-12-12 for KDDI,end */
    EN_NV_ID_REG_STRATEGY_CONFIG                    = 0x7BDC,/* 注册重试策略配置, Added by   for ChinaTelecom_Volte, 2017-01-20 */
    EN_NV_ID_CTCC_CUSTOM_CONFIG                     = 0x7BDD,/* 电信定制需求配置, Added by   for ChinaTelecom_Volte, 2017-01-20 */
    EN_NV_ID_NORMAL_PDN_REJ_CONFIG                  = 0x7BDE,/* PDN被拒立即重新发起激活的ESM原因值列表, Added by   for ChinaTelecom_Volte, 2017-01-20 */
    /* BOSTON VoWiFI Phase I Project,begin 2016-02-14 */
    EN_NV_ID_LTE_PDN_PERM_FORB_RETRY_CONFIG         = 0x7BDF,
    EN_NV_ID_VOWIFI_REMAIN_ACTIVE_CTRL_CONFIG       = 0x7BE0,
    /* BOSTON VoWiFI Phase I Project,end 2016-02-14 */
    /* add for separate special part from original NV , 2017-03-07, begin */
    /* 解耦拆分新增的NV只在BOSTON版本上生效 */
    EN_NV_ID_HIFI_CONTROL_CONFIG                    = 0x7BE1,
    EN_NV_ID_NO_CARD_EMC_CALL_SUPPORT_FLAG_CONFIG   = 0x7BE2,
    /* add for separate special part from original NV , 2017-03-07, end */
    EN_NV_ID_REG_FAIL_PDN_RETRY_CONFIG              = 0x7BE3,/* 注册失败后断开与重建PDN的控制NV */
    EN_NV_ID_EMC_TCALL_CTRL                         = 0x7BE5,/* 紧急TCALL定时器时长定制NV */
    /*  check PingPong Exist or not in L2W handover, begin in 2017-03-30 */
    EN_NV_ID_IMSA_PINGPONG_CONFIG                   = 0x7BE6, /* LTE/WiFi乒乓切换控制结构NV */
    /*  check PingPong Exist or not in L2W handover, end in 2017-03-30 */
    /* for Boston VoWIFI Phase II,begin 2017-04-08*/
    EN_NV_ID_IMSA_VOWIFI_MODEM_VERSION_CONFIG       = 0x7BEA,/* VoWIFI MODEM侧版本号配置 */
    /* for Boston VoWIFI Phase II,end 2017-04-08*/
    /* begin for EMC reg no respond 2017-06-23 */
    EN_NV_ID_EMC_REG_NO_RSP_TO_ANONYMOUS_CTRL       = 0x7BED,/* 紧急注册保护功能定制NV */
    /* end for EMC reg no respond 2017-06-23 */
    /* KDDI VOWIFI REQ#3 WIFI prefer 150s tried, begin in 2018-05-14 */
    EN_NV_ID_KDDI_WIFI_PDN_TRY_CONFIG               = 0x7BFE,
    /* KDDI VOWIFI REQ#3 WIFI prefer 150s tried, begin in 2018-05-14 */
    EN_NV_ID_LNAS_IMSA_END                          = 0x7E2C,/* IMSA END */
    EN_NV_ID_LNAS_AGPS_BEGIN                        = 0x7E2D,/* AGPS BEGIN */
    EN_NV_ID_LNAS_AGPS_END                          = 0x7EF3,/* AGPS END */
    EN_NV_ID_LNAS_NEW_NV_INTERVAL_END               = 0x7EF4,/* LNAS NEW ALLOCATE NV INTERVAL END*/
    /*mod for LNAS NV migrate because of conflict with MBB product,end 2017-02-23*/
    /*mod for LNAS NV migrate because of conflict with MBB product,begin 2017-02-23*/
    /***************************从0xCB27到0xCF08为LNAS的NV区间*********************************************/
    EN_NV_ID_LNAS_CSS_BEGIN                         = 0xCB27,/* CSS BEGIN */
    EN_NV_ID_CSS_CONFIG_CONTROL_PARA                = 0xCB34,
    EN_NV_ID_LNAS_CSS_END                           = 0xCBE8,/* CSS END */
    EN_NV_ID_LNAS_ERABM_ESM_BEGIN                   = 0xCBE9,/* ERABM ESM BEGIN */
    /* Added for IMS_PDN_STORE,2016-09-29,Begin */
    EN_NV_ID_LNAS_ESM_NDIS_CONN_DELAY_CTRL_CONFIG   = 0XCBEA,/* ndis conn delay定时器 单位ms */
    /* Added for IMS_PDN_STORE,2016-09-29,End */
    /* Added for MTU_REQUIRE,2016-11-12,Begin */
    EN_NV_ID_LNAS_ESM_IPV4_MTU_CTRL_CONFIG          = 0XCBEB,/*请求IPV4 MTU配置nv */
    /* Added for MTU_REQUIRE,2016-11-12,End */
    /* Added 2016-12-14 for KDDI,begin */
    EN_NV_ID_LNAS_ESM_CHANGE_TO_IMSAPN_CONFIG       = 0xCBEC, /* ATTACH时承载被拒后，是否要替换为IMS的APN */
    /* Added 2016-12-14 for KDDI,end */
    EN_NV_ID_LNAS_ERABM_ESM_END                     = 0xCCB0,/* ERABM ESM END */
    EN_NV_ID_LNAS_RSV_BEGIN                         = 0xCCB1,/* RSV BEGIN */
    EN_NV_ID_LNAS_RSV_END                           = 0xCF08,/* RSV END */
    /*mod for LNAS NV migrate because of conflict with MBB product,end 2017-02-23*/
    /***************************从0xCB27到0xCF08为LNAS的NV区间*********************************************/
    /* add for plmn search opt 6.0, 2016-08-04, End */
    EN_NV_ID_PS_BEGIN                               = 0xD200,/*Modem begin D000*/
    EN_NV_ID_LTEAPT_TOTAL_FLG                       = 0xD201,
    EN_NV_ID_IMS_SIP_CONFIG                         = 0xD202,/* modified 2013-12-23 VoLTE */
    EN_NV_ID_IMS_CONFIG                             = 0xD203,/* modified 2013-11-01 VoLTE */
    EN_NV_ID_UE_NET_CAPABILITY                      = 0xD204,
    EN_NV_ID_UE_CENTER                              = 0xD205,/* modified 2012-07-31 cs+ps1 */
    EN_NV_ID_NAS_RELEASE                            = 0xD206,
    EN_NV_ID_IMS_RAT_SUPPORT                        = 0xD207,/*modified 2013-06-27 VoLTE*/
    EN_NV_ID_SEC_CONTEXT                            = 0xD208,
    EN_NV_ID_IMSA_CONFIG                            = 0xD209,/*modified 2013-06-27 VoLTE*/
    EN_NV_ID_EPS_LOC                                = 0xD20a,
    EN_NV_ID_IMS_CAPABILITY                         = 0xD20b,/*modified 2013-06-27 VoLTE*/
    EN_NV_ID_SIP_PORT_CONFIG                        = 0xD20c,/*modified 2013-06-27 VoLTE*/
    EN_NV_ID_IMSI                                   = 0xD20d,
    EN_NV_ID_UE_VOICE_DOMAIN                        = 0xD20e,
    EN_NV_ID_IMS_VOIP_CONFIG                        = 0xD20f,/* modified 2013-12-23 VoLTE */
    EN_NV_ID_IMS_CODE_CONFIG                        = 0xD210,/* modified 2013-12-23 VoLTE */
    EN_NV_ID_IMS_SS_CONF_CONFIG                     = 0xD211,/* modified 2013-12-23 VoLTE */
    EN_NV_ID_ATTACH_BEARER_RE_ESTABLISH             = 0xD212,
    EN_NV_ID_IMS_SECURITY_CONFIG                    = 0xD213,/* modified 2013-12-23 VoLTE */
    EN_NV_ID_IMS_MEDIA_PARM_CONFIG                  = 0xD214,
    EN_NV_ID_PS_LOCALIP_CAP                         = 0xD215,/* modified 2014-09-18 R11 */
    EN_NV_ID_PS_COMM_BAND_CONFIG                    = 0xD216,/* modified 2015-05-14 css */
    EN_NV_ID_PS_RAT_RSSI_THRESHOLD                  = 0xD217,/* modified 2015-05-14 css */
    EN_NV_ID_LPP_PARA_CONFIG                        = 0xD218,/* modified 2015-8-5 fo lpp*/
    EN_NV_ID_LNAS_SWITCH_PARA                       = 0xD219,/* 2015-06-18 begin */
    EN_NV_ID_CONFIG_NWCAUSE                         = 0xD21a,/* modify 2014-02-15 for self-adaption NW cause*/
    EN_NV_ID_DAM_CONFIG_PARA                        = 0xD21b,/*   mod for AT&T program 2015-01-04 DTS begin */
    EN_NV_ID_LNAS_COMM_CONFIG_PARA                  = 0xD21c,/*   mod for AT&T program 2015-01-04 DTS begin */
    EN_NV_ID_PCSCF_DISCOVERY_POLICY                 = 0xD21d,/* add 2014-03-24 VoLTE */
    EN_NV_ID_BACKOFF_ALG_CONFIG                     = 0xD21e,/*   mod for AT&T program 2015-01-15 DTS begin */
    EN_NV_ID_LCS_COMMON_CONFIG                      = 0xD21f,/* modified 2015-10-13 fo lcs*/
    EN_NV_ID_CELL_MEAS_THREDHOLD                    = 0xD220, /*add */
    EN_NV_ID_GET_SIB_THRESHOLD                      = 0xD221,
    EN_NV_ID_UE_CAP_V9a0                            = 0xD222,/* R10 NV修改 begin */
    EN_NV_ID_UE_CAP_V1020                           = 0xD223,
    EN_NV_ID_UE_CAP_V1060                           = 0xD224,
    EN_NV_ID_UE_CAP_RF_PARA_V1060                   = 0xD225,/* R10 NV修改 end */
    EN_NV_ID_IRAT_REDIR_SWITCH                      = 0xD226,
    EN_NV_ID_LTE_SPAC_BAND_INFO                     = 0xD227,/* MTC add begin */
    EN_NV_ID_LTE_ABANDON_BAND_INFO                  = 0xD228,/* MTC add end */
    EN_NV_ID_LTE_MTC_NOTCH_FEATURE_INFO             = 0xD229,/* MTC NOTCH add by begin */
    EN_NV_ID_LTE_CUSTOM_MCC_INFO                    = 0xD22a,/*Delete FDD Band */
    EN_NV_ID_RRC_THRESHOLD                          = 0XD22b,/* adapt filter begin */
    EN_NV_ID_UE_CAPABILITY                          = 0xD22c,
    EN_NV_ID_SUPPORT_FREQ_BAND                      = 0xD22d,
    NV_ID_RRC_NV_CALIBRATION_BAND_LIST              = 0xD22e,
    EN_NV_ID_SUPPORT_SUPPORT_RESEL                  = 0xD22f,
    EN_NV_ID_UE_MAX_TX_POWER                        = 0xD230,
    EN_NV_ID_CGI_INFO                               = 0xD231,
    EN_NV_ID_RRC_LOCK_FREQ_SWITCH                   = 0xD232,
    EN_NV_ID_RRC_RESELECT_3DB_SWITCH                = 0xD233,
    EN_NV_ID_UE_RFRD_TYPE                           = 0xD234,
    EN_NV_ID_LTE_IRAT_TDS_CAPABILITY_INFO           = 0xD235,/* Irat TDS UE capability modify begin */
    EN_NV_ID_UE_EXTBAND_INFO                        = 0xD236,/*BEGIN  modify for B28全频段特性*/
    EN_NV_ID_EXTBAND_INFO_WITH_BANDWIDTH            = 0xD237,/*END  modify for B28全频段特性*/
    EN_NV_ID_DSDS_CFG_INFO                          = 0xD238,
    EN_NV_ID_FIX_MUTI_PLMNS_CAND_INFO               = 0xD239,
    EN_NV_ID_DYN_MUTI_PLMNS_CAND_INFO               = 0xD23A,
    EN_NV_ID_LATEST_CAND_FREQ_WITHPLMN_INFO         = 0xD23B,
    EN_NV_ID_IDC_PARA                               = 0xD23c,/* begin: add mbms for feature v700r500 */
    EN_NV_ID_OTDOA_ADDITIONAL_NCELL_PARA            = 0xD23d,/* begin: add for LPP */
    EN_NV_ID_LRRC_NV_CELL_SELECT_CTRL               = 0xD23E,
    EN_NV_ID_LRRC_NV_INTER_OPT_CTRL                 = 0xD23F,
    /*begin,add for B29 single dl ctrl */
    EN_NV_ID_LRRC_ONLY_DL_BAND_CTRL                 = 0xD241,
    /*end,add for B29 single dl ctrl */
    EN_NV_ID_RF_HW                                  = 0xD242,
    EN_NV_ID_L2_LUP_MEM_CTRL                        = 0xD243,
    /* add for cloud prefer frequency program, Begin */
    EN_NV_ID_PS_CLOUD_FREQ_STRATEGY                 = 0xD244,
    EN_NV_ID_PS_CLOUD_PREFER_FREQ_MCC1              = 0xD245,
    EN_NV_ID_PS_CLOUD_PREFER_FREQ_MCC2              = 0xD246,
    EN_NV_ID_PS_CLOUD_PREFER_FREQ_MCC3              = 0xD247,
    /* add for cloud prefer frequency program, End */
    /* begin add for Irat Resel RsrqTres */
    EN_NV_ID_GUTL_COMM_RESEL_THRES_CFG              = 0xD248,
    /* end add for Irat Resel RsrqTres */
    EN_NV_ID_M_FEATURE_CTRL                         = 0xD249,
    EN_NV_ID_PS_OM_FEATURE_CTRL                     = 0xD24A,
    /*begin,add for plmn forbidden band */
    EN_NV_ID_LRRC_PLMN_FORBIDDEN_BAND               = 0xD24B,
    /*end,add for plmn forbidden band */
    /* Begin UserPlan improve */
    EN_NV_ID_LRRC_USER_PLAN_IMPROVE_CTRL            = 0xD24C,
    /* End UserPlan improve */
    EN_NV_ID_LRRC_NV_CSG_CTRL_PARA                  = 0xD24E,/* add for CSG ctrl para */
    EN_NV_ID_LRRC_IMSA_VOWIFI_RPT_INFO              = 0xD24F,/* add for vowifi quality rpt,   */
    EN_NV_ID_USIM_BEGIN                             = 0xD250,
    EN_NV_ID_VOLTE_CARD_LOCK                        = 0xD251,
    EN_NV_ID_VOWIFI_PARA_CONFIG                     = 0xD252,
    EN_NV_ID_IMSA_COMM_PARA_CONFIG                  = 0xD253,
    EN_NV_ID_IMS_PARM_UE_CAPABILITY                 = 0xD254,
    EN_NV_ID_T3402_PLMN_CTRL                        = 0xD255, /* Added by   for DATA RETRY PHASEI, 2016-03-21 */
    EN_NV_ID_HPLMN_PERI_FILE                        = 0xD256,
    EN_NV_ID_SEC_CONTEXT_FILE                       = 0xD257,
    EN_NV_ID_CS_LOC_FILE                            = 0xD258,
    EN_NV_ID_PS_LOC_FILE                            = 0xD259,
    EN_NV_ID_EPS_LOC_FILE                           = 0xD25a,
    EN_NV_ID_IMSI_FILE                              = 0xD25b,
    EN_NV_ID_CS_CKIK_FILE                           = 0xD25c,
    EN_NV_ID_PS_KEY_FILE                            = 0xD25d,
    EN_NV_ID_KC_FILE                                = 0xD25e,
    EN_NV_ID_UE_NAS_SECURITY_ALGORITHMS             = 0xD25f,
    EN_NV_ID_USIM_VISION                            = 0xD260,
    EN_NV_ID_ACC_CLASSMASK_FILE                     = 0xD261,
    /* add for CSS Rat Recognize, 2016-06-12, Begin */
    EN_NV_ID_PS_CSS_RAT_RECOGNIZE                   = 0xD262,
    /* add for CSS Rat Recognize, 2016-06-12, End */
    EN_NV_ID_SEQ_INFO                               = 0xD263,
    EN_NV_ID_LRPLMNSI_FILE                          = 0xD264,
    EN_NV_ID_USIM_END                               = 0xD269,
    EN_NV_ID_CAND_CELL                              = 0xD26a,
    EN_NV_CSQ_RPT_INFO                              = 0xD26b,
    EN_NV_ID_TPS_SWITCH_PARA                        = 0xD26c,/* begin*/
    EN_NV_ID_IMS_STRING_SWITCH                      = 0xD26e,
    EN_NV_ID_IMSA_SMSPSI                            = 0xD26f,
    EN_NV_ID_SWITCH_PARA                            = 0xD275,
    EN_NV_ID_LTE_CONFIG                             = 0xD276,
    EN_NV_ID_LTE_GATE_WAY_IP                        = 0xD277,
    EN_NV_ID_SIB_SUPPORT                            = 0xD280,
    EN_NV_ID_GATE_CLOCK_SWITCH_CONFIG               = 0xD281,
    EN_NV_ID_POWER_CONFIG                           = 0xD282,
    EN_NV_ID_LTE_COMP_SWITCH                        = 0xD283,
    EN_NV_ID_UE_CAP_MEAS_PARA_BAND7                 = 0xD284,
    EN_NV_ID_UE_CAP_MEAS_PARA_BAND20                = 0xD285,
    EN_NV_ID_UE_CAP_MEAS_PARA_BAND38                = 0xD286,
    EN_NV_ID_UE_CAP_MEAS_PARA_BAND40                = 0xD287,
    EN_NV_ID_UE_CAP_MEAS_PARA_BAND41                = 0xD288,
    EN_NV_ID_RA_FAIL_REPORT_INFO                    = 0xD290,
    EN_NV_ID_TEST_PLMN_SET                          = 0xD291,/* 仪器测试时，搜网搜到非仪器的PLMN不上报，begin */
    EN_NV_ID_BAND1_CAND_INFO                        = 0xD2a0,/* mul Band */
    EN_NV_ID_BAND2_CAND_INFO                        = 0xD2a1,
    /*EN_NV_ID_BAND3_CAND_INFO                      = 0xD2a2,*/
    EN_NV_ID_BAND4_CAND_INFO                        = 0xD2a3,
    EN_NV_ID_BAND5_CAND_INFO                        = 0xD2a4,
    EN_NV_ID_BAND6_CAND_INFO                        = 0xD2a5,
    /*EN_NV_ID_BAND7_CAND_INFO                      = 0xD2a6,*/
    EN_NV_ID_BAND8_CAND_INFO                        = 0xD2a7,
    EN_NV_ID_BAND9_CAND_INFO                        = 0xD2a8,
    EN_NV_ID_BAND10_CAND_INFO                       = 0xD2a9,
    EN_NV_ID_BAND11_CAND_INFO                       = 0xD2aa,
    EN_NV_ID_BAND12_CAND_INFO                       = 0xD2ab,
    EN_NV_ID_BAND13_CAND_INFO                       = 0xD2ac,
    EN_NV_ID_BAND14_CAND_INFO                       = 0xD2ad,
    EN_NV_ID_BAND15_CAND_INFO                       = 0xD2ae,
    EN_NV_ID_BAND16_CAND_INFO                       = 0xD2af,
    EN_NV_ID_BAND17_CAND_INFO                       = 0xD2b0,
    EN_NV_ID_BAND18_CAND_INFO                       = 0xD2b1,
    EN_NV_ID_BAND19_CAND_INFO                       = 0xD2b2,
    /*EN_NV_ID_BAND20_CAND_INFO                     = 0xD2b3,*/
    EN_NV_ID_BAND21_CAND_INFO                       = 0xD2b4,
    EN_NV_ID_BAND22_CAND_INFO                       = 0xD2b5,
    EN_NV_ID_BAND23_CAND_INFO                       = 0xD2b6,
    EN_NV_ID_BAND24_CAND_INFO                       = 0xD2b7,
    EN_NV_ID_BAND25_CAND_INFO                       = 0xD2b8,
    EN_NV_ID_BAND26_CAND_INFO                       = 0xD2b9,
    EN_NV_ID_BAND27_CAND_INFO                       = 0xD2ba,
    EN_NV_ID_BAND28_CAND_INFO                       = 0xD2bb,
    EN_NV_ID_BAND29_CAND_INFO                       = 0xD2bc,
    EN_NV_ID_BAND30_CAND_INFO                       = 0xD2bd,
    EN_NV_ID_BAND31_CAND_INFO                       = 0xD2be,
    EN_NV_ID_BAND32_CAND_INFO                       = 0xD2bf,
    EN_NV_ID_BAND33_CAND_INFO                       = 0xD2c0,
    EN_NV_ID_BAND34_CAND_INFO                       = 0xD2c1,
    EN_NV_ID_BAND35_CAND_INFO                       = 0xD2c2,
    EN_NV_ID_BAND36_CAND_INFO                       = 0xD2c3,
    EN_NV_ID_BAND37_CAND_INFO                       = 0xD2c4,
    /*EN_NV_ID_BAND38_CAND_INFO                     = 0xD2c5,*/
    /*EN_NV_ID_BAND39_CAND_INFO                     = 0xD2c6,*/
    /*EN_NV_ID_BAND40_CAND_INFO                     = 0xD2c7,*/
    /*EN_NV_ID_BAND41_CAND_INFO                     = 0xD2c8,*/
    EN_NV_LWCLASH_RANGE_INFO                        = 0xD2c8,
    EN_NV_ID_BAND42_CAND_INFO                       = 0xD2c9,
    EN_NV_ID_BAND43_CAND_INFO                       = 0xD2ca,
    EN_NV_ID_BAND3_CAND_INFO                        = 0xD2d1,
    EN_NV_ID_BAND7_CAND_INFO                        = 0xD2d2,
    EN_NV_ID_BAND20_CAND_INFO                       = 0xD2d3,
    EN_NV_ID_BAND38_CAND_INFO                       = 0xD2d4,
    EN_NV_ID_BAND39_CAND_INFO                       = 0xD2d5,
    EN_NV_ID_BAND40_CAND_INFO                       = 0xD2d6,
    EN_NV_ID_BAND41_CAND_INFO                       = 0xD2d7,
    EN_NV_ID_BAND64_CAND_INFO                       = 0xD2d8,
    EN_NV_ID_FLOWCTRL_CONFIG                        = 0xD2d9,
    EN_NV_ID_BANDXX_CAND_INFO                       = 0xD2da,/*  -nv-bands-cfg-begin */
    EN_NV_ID_BANDNon2_CAND_INFO                     = 0xD2DB,/* 非标频段的候补信息begin */
    EN_NV_ID_BANDNon3_CAND_INFO                     = 0xD2DC,
    EN_NV_ID_BANDNon4_CAND_INFO                     = 0xD2DD,
    EN_NV_ID_BANDNon5_CAND_INFO                     = 0xD2DE,
    EN_NV_ID_BANDNon6_CAND_INFO                     = 0xD2DF,
    EN_NV_ID_BANDNon7_CAND_INFO                     = 0xD2E0,
    EN_NV_ID_BANDNon8_CAND_INFO                     = 0xD2E1,
    EN_NV_ID_BANDNon9_CAND_INFO                     = 0xD2E2,
    EN_NV_ID_BANDNon10_CAND_INFO                    = 0xD2E3,
    EN_NV_ID_BANDNon11_CAND_INFO                    = 0xD2E4,
    EN_NV_ID_BANDNon12_CAND_INFO                    = 0xD2E5,
    EN_NV_ID_BANDNon13_CAND_INFO                    = 0xD2E6,
    EN_NV_ID_BANDNon14_CAND_INFO                    = 0xD2E7,
    EN_NV_ID_BANDNon15_CAND_INFO                    = 0xD2E8,
    EN_NV_ID_BANDNon16_CAND_INFO                    = 0xD2E9,
    EN_NV_ID_BANDNon1_CAND_INFO                     = 0xD2EA,/* 非标频段的候补信息end */
    EN_NV_ID_BANDNon1_BAND_INFO                     = 0xD2ED,/* 非标频段的频段信息begin */
    EN_NV_ID_BANDNon2_BAND_INFO                     = 0xD2EE,
    EN_NV_ID_BANDNon3_BAND_INFO                     = 0xD2EF,
    EN_NV_ID_BANDNon4_BAND_INFO                     = 0xD2F0,
    EN_NV_ID_BANDNon5_BAND_INFO                     = 0xD2F1,
    EN_NV_ID_BANDNon6_BAND_INFO                     = 0xD2F2,
    EN_NV_ID_BANDNon7_BAND_INFO                     = 0xD2F3,
    EN_NV_ID_BANDNon8_BAND_INFO                     = 0xD2F4,
    EN_NV_ID_BANDNon9_BAND_INFO                     = 0xD2F5,
    EN_NV_ID_BANDNon10_BAND_INFO                    = 0xD2F6,
    EN_NV_ID_BANDNon11_BAND_INFO                    = 0xD2F7,
    EN_NV_ID_BANDNon12_BAND_INFO                    = 0xD2F8,
    EN_NV_ID_BANDNon13_BAND_INFO                    = 0xD2F9,
    EN_NV_ID_BANDNon14_BAND_INFO                    = 0xD2FA,
    EN_NV_ID_BANDNon15_BAND_INFO                    = 0xD2FB,
    EN_NV_ID_BANDNon16_BAND_INFO                    = 0xD2FC,/* 非标频段的频段信息end */
    EN_NV_ID_FLOWCTRL_LIMIT_CONFIG                  = 0xD2FD,
    EN_NV_ID_TDS_FREQ_CELL_LOCK_SWITCH              = 0xD2FE,/*  freq/cell lock begin*/
    EN_NV_ID_PS_END                                 = 0xD2ff,
    EN_NV_ID_TDS_UTRANCAPABILITY                    = 0xD300,
    EN_NV_ID_TDS_CERSSI_REPROT_PARA                 = 0xD301,
    EN_NV_ID_TDS_MAC_HSPA_CTRPARA                   = 0xD302,
    EN_NV_ID_TDS_RRC_LOCK_FREQ_SWITCH               = 0xD303,
    EN_NV_ID_TDS_SUPPORT_FREQ_BAND                  = 0xD304,
    EN_NV_ID_TDS_FLOWCTRPARA                        = 0xD305,
    EN_NV_ID_TDS_ENG_NULLTIMER_CONTROL_TESTFLAG_EQPOUT = 0xD306,
    EN_NV_ID_TDS_NET_SELECT_MODE                    = 0xD307,
    EN_NV_ID_TDS_PLMN_SELECT_MODE                   = 0xD308,
    EN_NV_ID_TDS_POWER_ON_ATTACH_MODE               = 0xD309,
    EN_NV_ID_TDS_MOBILESTATION_CLASS                = 0xD30a,
    EN_NV_ID_TDS_UTRAN_CFG_DSP                      = 0xD30b,
    EN_NV_ID_TDS_BA_LIST                            = 0xD30c,/* TRRC CHANGE FOR BA begin */
    EN_NV_ID_TDS_CUSTOMIZE_CS_SERVICE               = 0xD30d,
    EN_NV_ID_CTRL_PARA                              = 0xD30e,/*  -fast-dorm-cfg-3 */
    EN_NV_ID_TDS_ALFA_FILTER                        = 0xD30f,/* filter the serve cell begin*/
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND1        = 0xD310,/* CA NV begin,d310~d32f,d350~d37f,d390~d3bf */
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND2        = 0xD311,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND3        = 0xD312,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND4        = 0xD313,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND5        = 0xD314,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND6        = 0xD315,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND7        = 0xD316,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND8        = 0xD317,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND9        = 0xD318,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND10       = 0xD319,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND11       = 0xD31a,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND12       = 0xD31b,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND13       = 0xD31c,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND14       = 0xD31d,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND15       = 0xD31e,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND16       = 0xD31f,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND17       = 0xD320,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND18       = 0xD321,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND19       = 0xD322,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND20       = 0xD323,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND21       = 0xD324,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND22       = 0xD325,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND23       = 0xD326,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND24       = 0xD327,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND25       = 0xD328,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND26       = 0xD329,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND27       = 0xD32a,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND28       = 0xD32b,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND29       = 0xD32c,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND30       = 0xD32d,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND31       = 0xD32e,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND32       = 0xD32f,/* CA NV end,d310~d32f,d350~d37f,d390~d3bf */
    EN_NV_ID_TDS_AREA_LOST_TIMER                    = 0xD330,/* add for SGLTE Begin */
    EN_NV_ID_TDS_FEATHURE_PARA                      = 0xD331,
    EN_NV_ID_TDS_AREA_LOST_THSD                     = 0xD332,/* MTC AREA LOST Begin */
    EN_NV_ID_TDS_MTC_RESEL_PARA                     = 0xD333,/* MTC RESEL Begin */
    EN_NV_ID_TL_L2_PARA                             = 0xD334,
    EN_NV_ID_BSIC_FRAME_PARA                        = 0xD335,/*   modify for bsic_fream begin */
    EN_NV_ID_TDS_MEAS_IMPROVE_PARA                  = 0xD336,/*  modify begin */
    EN_NV_ID_CELL_SEARCH_TIME                       = 0xD338,/*begin:add for tds cell search optimize  */
    EN_NV_ID_CHR_ERROR_LOG_INFO                     = 0xD339,/*add by   for not to LTE begin*/
    EN_NV_ID_L2_CHR_CTRL_CONFIG                     = 0xD33a,/*Added by   for L2 CHR Ctrl Config*/
    EN_NV_ID_POWER_ON_LOG_SWITCH                    = 0xD33b,/* Added by   for 开机log功能 */

    /* begin:add for Boston Phase3  UE CAP  */
    EN_NV_ID_UL_CA_IDC_PARA                         = 0xD33c,
    EN_NV_ID_UE_CAP_V1180_EXT                       = 0xD33d,
    /* end:add for Boston Phase3  UE CAP  */

    EN_NV_ID_UE_CAP_V1090                           = 0xD340,/* R11 NV修改 begin */
    EN_NV_ID_UE_CAP_V1130                           = 0xD341,
    EN_NV_ID_UE_CAP_V1170                           = 0xD342,
    EN_NV_ID_UE_CAP_V9c0                            = 0xD343,
    EN_NV_ID_UE_CAP_V9d0                            = 0xD344,
    EN_NV_ID_UE_CAP_V9e0                            = 0xD345,
    EN_NV_ID_UE_CAP_V9h0                            = 0xD346,
    EN_NV_ID_UE_CAP_V10c0                           = 0xD347,
    EN_NV_ID_UE_CAP_V1180                           = 0xD348,
    EN_NV_ID_UE_CAP_V11A0                           = 0xD349,
    EN_NV_ID_UE_CAP_V1250                           = 0xD34a,
    EN_NV_ID_UE_CAP_V10f0                           = 0xD34b,/* R11 NV修改 end */
    /* Begin: 2016/1/4 cr develop */
    EN_NV_ID_UE_CAP_V10i0                           = 0xD34c,
    EN_NV_ID_UE_CAP_V11d0                           = 0xD34D,
    /* End: 2016/1/4 cr develop */
    EN_NV_ID_UE_CAP_MIN_CA_INFO                     = 0xD34e,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND33       = 0xD350,/* CA NV begin,d310~d32f,d350~d37f,d390~d3bf */
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND34       = 0xD351,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND35       = 0xD352,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND36       = 0xD353,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND37       = 0xD354,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND38       = 0xD355,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND39       = 0xD356,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND40       = 0xD357,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND41       = 0xD358,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND42       = 0xD359,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND43       = 0xD35a,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND44       = 0xD35b,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND45       = 0xD35c,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND46       = 0xD35d,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND47       = 0xD35e,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND48       = 0xD35f,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND49       = 0xD360,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND50       = 0xD361,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND51       = 0xD362,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND52       = 0xD363,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND53       = 0xD364,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND54       = 0xD365,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND55       = 0xD366,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND56       = 0xD367,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND57       = 0xD368,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND58       = 0xD369,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND59       = 0xD36a,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND60       = 0xD36b,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND61       = 0xD36c,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND62       = 0xD36d,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND63       = 0xD36e,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND64       = 0xD36f,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND65       = 0xD370,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND66       = 0xD371,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND67       = 0xD372,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND68       = 0xD373,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND69       = 0xD374,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND70       = 0xD375,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND71       = 0xD376,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND72       = 0xD377,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND73       = 0xD378,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND74       = 0xD379,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND75       = 0xD37a,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND76       = 0xD37b,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND77       = 0xD37c,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND78       = 0xD37d,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND79       = 0xD37e,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND80       = 0xD37f,/* CA NV end,d310~d32f,d350~d37f,d390~d3bf */
    EN_NV_ID_UE_CAP_MCC_CA_INFO_0                   = 0xD380,/* begin: add for MCC report CA,   */
    EN_NV_ID_UE_CAP_MCC_CA_INFO_1                   = 0xD381,
    EN_NV_ID_UE_CAP_MCC_CA_INFO_2                   = 0xD382,
    EN_NV_ID_UE_CAP_MCC_CA_INFO_3                   = 0xD383,
    EN_NV_ID_UE_CAP_MCC_CA_INFO_4                   = 0xD384,
    EN_NV_ID_UE_CAP_MCC_CA_INFO_5                   = 0xD385,
    EN_NV_ID_UE_CAP_MCC_CA_INFO_6                   = 0xD386,/* end: add for MCC report CA,   */
    EN_NV_ID_BAND44_CAND_INFO                       = 0xD387,/* begin: add for feature v700r500 ,  */
    EN_NV_ID_UE_CAP_MCC_BAND_INFO                   = 0xD388,/* Begin: add Report Ue Capability By MCC */
    EN_NV_ID_SBM_CUSTOM_DUAL_IMSI                   = 0xD389,/* Begin: 2015/5/6 sbm */
    EN_NV_ID_ACC_FAIL_BAR_INFO                      = 0xD38c,/*begin accfail bar byhanlei*/
    EN_NV_ID_BG_LTE_LIMIT_THREHHOLD                 = 0xD38d,
    EN_NV_ID_BG_LTE_LIMIT_THREHHOLD_TYPE            = 0xD38e,
    EN_NV_ID_DPDT_SWITCH_PARA                       = 0xD38f,/*Dpdt add by   begin*/
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND81       = 0xD390,/* CA NV begin,d310~d32f,d350~d37f,d390~d3bf */
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND82       = 0xD391,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND83       = 0xD392,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND84       = 0xD393,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND85       = 0xD394,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND86       = 0xD395,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND87       = 0xD396,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND88       = 0xD397,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND89       = 0xD398,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND90       = 0xD399,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND91       = 0xD39a,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND92       = 0xD39b,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND93       = 0xD39c,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND94       = 0xD39d,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND95       = 0xD39e,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND96       = 0xD39f,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND97       = 0xD3a0,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND98       = 0xD3a1,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND99       = 0xD3a2,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND100      = 0xD3a3,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND101      = 0xD3a4,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND102      = 0xD3a5,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND103      = 0xD3a6,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND104      = 0xD3a7,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND105      = 0xD3a8,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND106      = 0xD3a9,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND107      = 0xD3aa,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND108      = 0xD3ab,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND109      = 0xD3ac,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND110      = 0xD3ad,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND111      = 0xD3ae,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND112      = 0xD3af,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND113      = 0xD3b0,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND114      = 0xD3b1,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND115      = 0xD3b2,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND116      = 0xD3b3,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND117      = 0xD3b4,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND118      = 0xD3b5,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND119      = 0xD3b6,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND120      = 0xD3b7,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND121      = 0xD3b8,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND122      = 0xD3b9,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND123      = 0xD3ba,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND124      = 0xD3bb,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND125      = 0xD3bc,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND126      = 0xD3bd,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND127      = 0xD3be,
    EN_NV_ID_UE_CAP_V1020_RF_MEAS_PARA_BAND128      = 0xD3bf,/* CA NV end,d310~d32f,d350~d37f,d390~d3bf */
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM1          = 0xD5a0,/* 以下是DSP的NV范围，PSNV不够用，暂时借用之 */
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM2          = 0xD5a1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM3          = 0xD5a2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM4          = 0xD5a3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM5          = 0xD5a4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM6          = 0xD5a5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM7          = 0xD5a6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM8          = 0xD5a7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM9          = 0xD5a8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM10         = 0xD5a9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM11         = 0xD5aa,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM12         = 0xD5ab,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM13         = 0xD5ac,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM14         = 0xD5ad,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM15         = 0xD5ae,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM16         = 0xD5af,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM17         = 0xD5b0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM18         = 0xD5b1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM19         = 0xD5b2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM20         = 0xD5b3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM21         = 0xD5b4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM22         = 0xD5b5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM23         = 0xD5b6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM24         = 0xD5b7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM25         = 0xD5b8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM26         = 0xD5b9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM27         = 0xD5ba,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM28         = 0xD5bb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM29         = 0xD5bc,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM30         = 0xD5bd,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM31         = 0xD5be,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM32         = 0xD5bf,
     /* begin: add for LTE CSG,   */
    EN_NV_ID_CAMPED_CSG_CELL_INFO                   = 0xD5c0,
    EN_NV_ID_CAMPED_CSG_MACRO_NEIGH_CELL_INFO_PART1 = 0xD5c1,
    EN_NV_ID_CAMPED_CSG_MACRO_NEIGH_CELL_INFO_PART2 = 0xD5c2,
    /* end:   add for LTE CSG,  */

    /*Added for DATA RETRY PHASEII 2016-05-23 start*/
    EN_NV_ID_DATA_RETRY_EMM_PARA_CONFIG             = 0xD5c3,
    EN_NV_ID_DATA_RETRY_ESM_PARA_CONFIG             = 0xD5c4,
    /*Added for DATA RETRY PHASEII 2016-05-23 end*/

    /* Added  for DSDS OPTIMIZE MT DETACH BY TAU, 2016-06-16, Begin */
    EN_NV_ID_DSDS_MT_DETACH_TAU_CTRL                = 0xD5c5,
    /* Added  for DSDS OPTIMIZE MT DETACH BY TAU, 2016-06-16, End   */
    EN_NV_ID_TL_BIG_DATA_PARA                       = 0xD5c6,
    /* Added for DATA RETRY PHASEIII, 2016-6-23, begin */
    EN_NV_ID_ESM_T3396_CTRL_CONFIG                  = 0xD5c7,
    /* Added for DATA RETRY PHASEIII, 2016-6-23, end */

    /* Added for DATA RETRY PHASEIII 2016-06-21 start */
    EN_NV_ID_NAS_EAB_CONFIG                         = 0xD5c8,
    /* Added for DATA RETRY PHASEIII 2016-06-21 end */
    /* Added for DATA RETRY PHASEIV, 2016-07-25, begin */
    EN_NV_ID_EMM_T3346_CTRL_CONFIG                  = 0xD5c9,
    /* Added for DATA RETRY PHASEIV, 2016-07-25, end */

    /* Added for DATA RETRY PHASEIII 2016-06-21 start */
    EN_NV_ID_NAS_APN_SWITCH_CONFIG                  = 0xD5ca,
    /* Added for DATA RETRY PHASEIII 2016-06-21 end */

    /* begin:add for mmp */
    EN_NV_ID_RRC_MMP_CONFIG                         = 0xD5CB,
    /* end:add for mmp */

    EN_NV_ID_LTE_TX_UL_AMPR_NS05                    = 0xD759,
    EN_NV_ID_LTE_TX_CA_AMPR                         = 0xD760,
    EN_NV_ID_LTE_TX_CFR                             = 0xD761,

    /* add for supp comb v10i0 begin */
    EN_NV_ID_UE_CAP_V10i0_BAND_COMB_PARAM1          = 0xBB80,
    EN_NV_ID_UE_CAP_V10i0_BAND_COMB_PARAM2          = 0xBB81,
    EN_NV_ID_UE_CAP_V10i0_BAND_COMB_PARAM3          = 0xBB82,
    EN_NV_ID_UE_CAP_V10i0_BAND_COMB_PARAM4          = 0xBB83,
    EN_NV_ID_UE_CAP_V10i0_BAND_COMB_PARAM5          = 0xBB84,
    EN_NV_ID_UE_CAP_V10i0_BAND_COMB_PARAM6          = 0xBB85,

    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM1          = 0xBB86,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM2          = 0xBB87,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM3          = 0xBB88,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM4          = 0xBB89,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM5          = 0xBB8a,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM6          = 0xBB8b,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM7          = 0xBB8c,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM8          = 0xBB8d,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM9          = 0xBB8e,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM10         = 0xBB8f,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM11         = 0xBB90,
    EN_NV_ID_UE_CAP_V11d0_BAND_COMB_PARAM12         = 0xBB91,
    /* add for supp comb v10i0 end */
    EN_NV_ID_SUPP_BAND_COMB_V1250_NV_LIST           = 0xBB92,
    EN_NV_ID_UE_CAP_V1260                           = 0xBB93,
    EN_NV_ID_UE_CAP_V1270                           = 0xBB94,
    EN_NV_ID_UE_CAP_V1280                           = 0xBB95,
    EN_NV_ID_UE_CAP_V1310                           = 0xBB96,

    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM33         = 0xBB97,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM34         = 0xBB98,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM35         = 0xBB99,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM36         = 0xBB9a,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM37         = 0xBB9b,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM38         = 0xBB9c,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM39         = 0xBB9d,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM40         = 0xBB9e,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM41         = 0xBB9f,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM42         = 0xBBa0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM43         = 0xBBa1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM44         = 0xBBa2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM45         = 0xBBa3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM46         = 0xBBa4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM47         = 0xBBa5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM48         = 0xBBa6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM49         = 0xBBa7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM50         = 0xBBa8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM51         = 0xBBa9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM52         = 0xBBaa,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM53         = 0xBBab,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM54         = 0xBBac,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM55         = 0xBBad,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM56         = 0xBBae,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM57         = 0xBBaf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM58         = 0xBBb0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM59         = 0xBBb1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM60         = 0xBBb2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM61         = 0xBBb3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM62         = 0xBBb4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM63         = 0xBBb5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM64         = 0xBBb6,

    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM65         = 0xBBb7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM66         = 0xBBb8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM67         = 0xBBb9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM68         = 0xBBba,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM69         = 0xBBbb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM70         = 0xBBbc,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM71         = 0xBBbd,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM72         = 0xBBbe,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM73         = 0xBBbf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM74         = 0xBBc0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM75         = 0xBBc1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM76         = 0xBBc2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM77         = 0xBBc3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM78         = 0xBBc4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM79         = 0xBBc5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM80         = 0xBBc6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM81         = 0xBBc7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM82         = 0xBBc8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM83         = 0xBBc9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM84         = 0xBBca,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM85         = 0xBBcb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM86         = 0xBBcc,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM87         = 0xBBcd,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM88         = 0xBBce,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM89         = 0xBBcf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM90         = 0xBBd0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM91         = 0xBBd1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM92         = 0xBBd2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM93         = 0xBBd3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM94         = 0xBBd4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM95         = 0xBBd5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM96         = 0xBBd6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM97         = 0xBBd7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM98         = 0xBBd8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM99         = 0xBBd9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM100        = 0xBBda,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM101        = 0xBBdb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM102        = 0xBBdc,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM103        = 0xBBdd,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM104        = 0xBBde,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM105        = 0xBBdf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM106        = 0xBBe0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM107        = 0xBBe1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM108        = 0xBBe2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM109        = 0xBBe3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM110        = 0xBBe4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM111        = 0xBBe5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM112        = 0xBBe6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM113        = 0xBBe7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM114        = 0xBBe8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM115        = 0xBBe9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM116        = 0xBBea,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM117        = 0xBBeb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM118        = 0xBBec,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM119        = 0xBBed,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM120        = 0xBBee,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM121        = 0xBBef,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM122        = 0xBBf0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM123        = 0xBBf1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM124        = 0xBBf2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM125        = 0xBBf3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM126        = 0xBBf4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM127        = 0xBBf5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM128        = 0xBBf6,

    EN_NV_ID_UE_CAP_V1250_EXT                       = 0xBBf7,

    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM1          = 0xBBf8,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM2          = 0xBBf9,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM3          = 0xBBfa,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM4          = 0xBBfb,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM5          = 0xBBfc,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM6          = 0xBBfd,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM7          = 0xBBfe,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM8          = 0xBBff,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM9          = 0xBC00,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM10         = 0xBC01,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM11         = 0xBC02,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM12         = 0xBC03,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM13         = 0xBC04,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM14         = 0xBC05,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM15         = 0xBC06,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM16         = 0xBC07,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM17         = 0xBC08,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM18         = 0xBC09,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM19         = 0xBC0a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM20         = 0xBC0b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM21         = 0xBC0c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM22         = 0xBC0d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM23         = 0xBC0e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM24         = 0xBC0f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM25         = 0xBC10,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM26         = 0xBC11,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM27         = 0xBC12,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM28         = 0xBC13,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM29         = 0xBC14,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM30         = 0xBC15,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM31         = 0xBC16,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM32         = 0xBC17,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM33         = 0xBC18,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM34         = 0xBC19,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM35         = 0xBC1a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM36         = 0xBC1b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM37         = 0xBC1c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM38         = 0xBC1d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM39         = 0xBC1e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM40         = 0xBC1f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM41         = 0xBC20,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM42         = 0xBC21,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM43         = 0xBC22,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM44         = 0xBC23,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM45         = 0xBC24,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM46         = 0xBC25,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM47         = 0xBC26,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM48         = 0xBC27,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM49         = 0xBC28,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM50         = 0xBC29,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM51         = 0xBC2a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM52         = 0xBC2b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM53         = 0xBC2c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM54         = 0xBC2d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM55         = 0xBC2e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM56         = 0xBC2f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM57         = 0xBC30,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM58         = 0xBC31,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM59         = 0xBC32,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM60         = 0xBC33,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM61         = 0xBC34,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM62         = 0xBC35,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM63         = 0xBC36,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM64         = 0xBC37,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM65         = 0xBC38,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM66         = 0xBC39,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM67         = 0xBC3a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM68         = 0xBC3b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM69         = 0xBC3c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM70         = 0xBC3d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM71         = 0xBC3e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM72         = 0xBC3f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM73         = 0xBC40,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM74         = 0xBC41,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM75         = 0xBC42,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM76         = 0xBC43,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM77         = 0xBC44,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM78         = 0xBC45,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM79         = 0xBC46,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM80         = 0xBC47,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM81         = 0xBC48,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM82         = 0xBC49,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM83         = 0xBC4a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM84         = 0xBC4b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM85         = 0xBC4c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM86         = 0xBC4d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM87         = 0xBC4e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM88         = 0xBC4f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM89         = 0xBC50,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM90         = 0xBC51,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM91         = 0xBC52,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM92         = 0xBC53,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM93         = 0xBC54,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM94         = 0xBC55,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM95         = 0xBC56,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM96         = 0xBC57,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM97         = 0xBC58,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM98         = 0xBC59,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM99         = 0xBC5a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM100        = 0xBC5b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM101        = 0xBC5c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM102        = 0xBC5d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM103        = 0xBC5e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM104        = 0xBC5f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM105        = 0xBC60,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM106        = 0xBC61,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM107        = 0xBC62,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM108        = 0xBC63,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM109        = 0xBC64,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM110        = 0xBC65,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM111        = 0xBC66,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM112        = 0xBC67,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM113        = 0xBC68,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM114        = 0xBC69,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM115        = 0xBC6a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM116        = 0xBC6b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM117        = 0xBC6c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM118        = 0xBC6d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM119        = 0xBC6e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM120        = 0xBC6f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM121        = 0xBC70,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM122        = 0xBC71,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM123        = 0xBC72,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM124        = 0xBC73,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM125        = 0xBC74,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM126        = 0xBC75,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM127        = 0xBC76,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM128        = 0xBC77,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM129        = 0xBC78,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM130        = 0xBC79,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM131        = 0xBC7a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM132        = 0xBC7b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM133        = 0xBC7c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM134        = 0xBC7d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM135        = 0xBC7e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM136        = 0xBC7f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM137        = 0xBC80,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM138        = 0xBC81,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM139        = 0xBC82,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM140        = 0xBC83,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM141        = 0xBC84,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM142        = 0xBC85,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM143        = 0xBC86,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM144        = 0xBC87,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM145        = 0xBC88,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM146        = 0xBC89,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM147        = 0xBC8a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM148        = 0xBC8b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM149        = 0xBC8c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM150        = 0xBC8d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM151        = 0xBC8e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM152        = 0xBC8f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM153        = 0xBC90,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM154        = 0xBC91,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM155        = 0xBC92,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM156        = 0xBC93,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM157        = 0xBC94,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM158        = 0xBC95,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM159        = 0xBC96,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM160        = 0xBC97,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM161        = 0xBC98,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM162        = 0xBC99,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM163        = 0xBC9a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM164        = 0xBC9b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM165        = 0xBC9c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM166        = 0xBC9d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM167        = 0xBC9e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM168        = 0xBC9f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM169        = 0xBCa0,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM170        = 0xBCa1,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM171        = 0xBCa2,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM172        = 0xBCa3,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM173        = 0xBCa4,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM174        = 0xBCa5,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM175        = 0xBCa6,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM176        = 0xBCa7,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM177        = 0xBCa8,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM178        = 0xBCa9,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM179        = 0xBCaa,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM180        = 0xBCab,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM181        = 0xBCac,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM182        = 0xBCad,

    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM183        = 0xBCae,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM184        = 0xBCaf,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM185        = 0xBCb0,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM186        = 0xBCb1,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM187        = 0xBCb2,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM188        = 0xBCb3,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM189        = 0xBCb4,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM190        = 0xBCb5,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM191        = 0xBCb6,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM192        = 0xBCb7,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM193        = 0xBCb8,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM194        = 0xBCb9,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM195        = 0xBCba,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM196        = 0xBCbb,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM197        = 0xBCbc,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM198        = 0xBCbd,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM199        = 0xBCbe,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM200        = 0xBCbf,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM201        = 0xBCc0,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM202        = 0xBCc1,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM203        = 0xBCc2,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM204        = 0xBCc3,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM205        = 0xBCc4,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM206        = 0xBCc5,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM207        = 0xBCc6,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM208        = 0xBCc7,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM209        = 0xBCc8,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM210        = 0xBCc9,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM211        = 0xBCca,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM212        = 0xBCcb,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM213        = 0xBCcc,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM214        = 0xBCcd,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM215        = 0xBCce,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM216        = 0xBCcf,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM217        = 0xBCd0,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM218        = 0xBCd1,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM219        = 0xBCd2,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM220        = 0xBCd3,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM221        = 0xBCd4,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM222        = 0xBCd5,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM223        = 0xBCd6,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM224        = 0xBCd7,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM225        = 0xBCd8,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM226        = 0xBCd9,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM227        = 0xBCda,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM228        = 0xBCdb,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM229        = 0xBCdc,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM230        = 0xBCdd,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM231        = 0xBCde,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM232        = 0xBCdf,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM233        = 0xBCe0,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM234        = 0xBCe1,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM235        = 0xBCe2,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM236        = 0xBCe3,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM237        = 0xBCe4,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM238        = 0xBCe5,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM239        = 0xBCe6,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM240        = 0xBCe7,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM241        = 0xBCe8,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM242        = 0xBCe9,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM243        = 0xBCea,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM244        = 0xBCeb,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM245        = 0xBCec,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM246        = 0xBCed,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM247        = 0xBCee,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM248        = 0xBCef,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM249        = 0xBCf0,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM250        = 0xBCf1,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM251        = 0xBCf2,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM252        = 0xBCf3,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM253        = 0xBCf4,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM254        = 0xBCf5,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM255        = 0xBCf6,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM256        = 0xBCf7,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM257        = 0xBCf8,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM258        = 0xBCf9,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM259        = 0xBCfa,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM260        = 0xBCfb,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM261        = 0xBCfc,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM262        = 0xBCfd,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM263        = 0xBCfe,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM264        = 0xBCff,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM265        = 0xBD00,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM266        = 0xBD01,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM267        = 0xBD02,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM268        = 0xBD03,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM269        = 0xBD04,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM270        = 0xBD05,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM271        = 0xBD06,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM272        = 0xBD07,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM273        = 0xBD08,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM274        = 0xBD09,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM275        = 0xBD0a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM276        = 0xBD0b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM277        = 0xBD0c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM278        = 0xBD0d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM279        = 0xBD0e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM280        = 0xBD0f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM281        = 0xBD10,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM282        = 0xBD11,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM283        = 0xBD12,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM284        = 0xBD13,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM285        = 0xBD14,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM286        = 0xBD15,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM287        = 0xBD16,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM288        = 0xBD17,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM289        = 0xBD18,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM290        = 0xBD19,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM291        = 0xBD1a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM292        = 0xBD1b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM293        = 0xBD1c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM294        = 0xBD1d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM295        = 0xBD1f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM296        = 0xBD20,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM297        = 0xBD21,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM298        = 0xBD22,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM299        = 0xBD23,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM300        = 0xBD24,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM301        = 0xBD25,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM302        = 0xBD26,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM303        = 0xBD27,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM304        = 0xBD28,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM305        = 0xBD29,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM306        = 0xBD2a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM307        = 0xBD2b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM308        = 0xBD2c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM309        = 0xBD2d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM310        = 0xBD2e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM311        = 0xBD2f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM312        = 0xBD30,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM313        = 0xBD31,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM314        = 0xBD32,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM315        = 0xBD33,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM316        = 0xBD34,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM317        = 0xBD35,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM318        = 0xBD36,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM319        = 0xBD37,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM320        = 0xBD38,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM321        = 0xBD39,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM322        = 0xBD3a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM323        = 0xBD3b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM324        = 0xBD3c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM325        = 0xBD3d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM326        = 0xBD3e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM327        = 0xBD3f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM328        = 0xBD40,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM329        = 0xBD41,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM330        = 0xBD42,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM331        = 0xBD43,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM332        = 0xBD44,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM333        = 0xBD45,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM334        = 0xBD46,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM335        = 0xBD47,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM336        = 0xBD48,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM337        = 0xBD49,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM338        = 0xBD4a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM339        = 0xBD4b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM340        = 0xBD4c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM341        = 0xBD4d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM342        = 0xBD4e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM343        = 0xBD4f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM344        = 0xBD50,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM345        = 0xBD51,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM346        = 0xBD52,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM347        = 0xBD53,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM348        = 0xBD54,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM349        = 0xBD55,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM350        = 0xBD56,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM351        = 0xBD57,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM352        = 0xBD58,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM353        = 0xBD59,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM354        = 0xBD5a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM355        = 0xBD5b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM356        = 0xBD5c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM357        = 0xBD5d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM358        = 0xBD5e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM359        = 0xBD5f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM360        = 0xBD60,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM361        = 0xBD61,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM362        = 0xBD62,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM363        = 0xBD63,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM364        = 0xBD64,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM365        = 0xBD65,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM366        = 0xBD66,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM367        = 0xBD67,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM368        = 0xBD68,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM369        = 0xBD69,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM370        = 0xBD6a,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM371        = 0xBD6b,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM372        = 0xBD6c,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM373        = 0xBD6d,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM374        = 0xBD6e,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM375        = 0xBD6f,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM376        = 0xBD70,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM377        = 0xBD71,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM378        = 0xBD72,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM379        = 0xBD73,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM380        = 0xBD74,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM381        = 0xBD75,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM382        = 0xBD76,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM383        = 0xBD77,
    EN_NV_ID_UE_CAP_V1250_BAND_COMB_PARAM384        = 0xBD78,


    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM129        = 0xBD79,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM130        = 0xBD7a,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM131        = 0xBD7b,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM132        = 0xBD7c,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM133        = 0xBD7d,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM134        = 0xBD7e,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM135        = 0xBD7f,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM136        = 0xBD80,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM137        = 0xBD81,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM138        = 0xBD82,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM139        = 0xBD83,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM140        = 0xBD84,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM141        = 0xBD85,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM142        = 0xBD86,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM143        = 0xBD87,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM144        = 0xBD88,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM145        = 0xBD89,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM146        = 0xBD8a,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM147        = 0xBD8b,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM148        = 0xBD8c,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM149        = 0xBD8d,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM150        = 0xBD8e,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM151        = 0xBD8f,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM152        = 0xBD90,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM153        = 0xBD91,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM154        = 0xBD92,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM155        = 0xBD93,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM156        = 0xBD94,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM157        = 0xBD95,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM158        = 0xBD96,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM159        = 0xBD97,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM160        = 0xBD98,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM161        = 0xBD99,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM162        = 0xBD9a,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM163        = 0xBD9b,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM164        = 0xBD9c,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM165        = 0xBD9d,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM166        = 0xBD9e,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM167        = 0xBD9f,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM168        = 0xBDa0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM169        = 0xBDa1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM170        = 0xBDa2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM171        = 0xBDa3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM172        = 0xBDa4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM173        = 0xBDa5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM174        = 0xBDa6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM175        = 0xBDa7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM176        = 0xBDa8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM177        = 0xBDa9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM178        = 0xBDaa,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM179        = 0xBDab,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM180        = 0xBDac,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM181        = 0xBDad,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM182        = 0xBDae,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM183        = 0xBDaf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM184        = 0xBDb0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM185        = 0xBDb1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM186        = 0xBDb2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM187        = 0xBDb3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM188        = 0xBDb4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM189        = 0xBDb5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM190        = 0xBDb6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM191        = 0xBDb7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM192        = 0xBDb8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM193        = 0xBDb9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM194        = 0xBDba,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM195        = 0xBDbb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM196        = 0xBDbc,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM197        = 0xBDbd,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM198        = 0xBDbe,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM199        = 0xBDbf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM200        = 0xBDc0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM201        = 0xBDc1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM202        = 0xBDc2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM203        = 0xBDc3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM204        = 0xBDc4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM205        = 0xBDc5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM206        = 0xBDc6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM207        = 0xBDc7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM208        = 0xBDc8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM209        = 0xBDc9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM210        = 0xBDca,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM211        = 0xBDcb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM212        = 0xBDcc,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM213        = 0xBDcd,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM214        = 0xBDce,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM215        = 0xBDcf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM216        = 0xBDd0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM217        = 0xBDd1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM218        = 0xBDd2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM219        = 0xBDd3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM220        = 0xBDd4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM221        = 0xBDd5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM222        = 0xBDd6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM223        = 0xBDd7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM224        = 0xBDd8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM225        = 0xBDd9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM226        = 0xBDda,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM227        = 0xBDdb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM228        = 0xBDdc,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM229        = 0xBDdd,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM230        = 0xBDde,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM231        = 0xBDdf,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM232        = 0xBDe0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM233        = 0xBDe1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM234        = 0xBDe2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM235        = 0xBDe3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM236        = 0xBDe4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM237        = 0xBDe5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM238        = 0xBDe6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM239        = 0xBDe7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM240        = 0xBDe8,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM241        = 0xBDe9,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM242        = 0xBDea,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM243        = 0xBDeb,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM244        = 0xBDec,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM245        = 0xBDed,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM246        = 0xBDee,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM247        = 0xBDef,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM248        = 0xBDf0,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM249        = 0xBDf1,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM250        = 0xBDf2,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM251        = 0xBDf3,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM252        = 0xBDf4,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM253        = 0xBDf5,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM254        = 0xBDf6,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM255        = 0xBDf7,
    EN_NV_ID_UE_CAP_V1180_BAND_COMB_PARAM256        = 0xBDf8,

    /*begin: mod , for NV decoup*/
    EN_NV_ID_UE_CAP_RF_MEAS_PARAM_V1020             = 0xBDF9,
    EN_NV_ID_UE_CAP_FEATURE_GROUP_IND_R10           = 0xBDFA,
    EN_NV_ID_UE_CAP_FEATURE_GROUP_IND_R9            = 0xBDFB,
    EN_NV_ID_UE_CAP_FEATURE_GROUP_IND               = 0xBDFC,
    EN_NV_ID_UE_CAP_PDCP_PARA                       = 0xBDFD,
    EN_NV_ID_CMAS_ETWS_CTR                          = 0xBDFE,
    EN_NV_ID_CA_OPTIMIZE_CTR                        = 0xBDFF,
    EN_NV_ID_VOLTE_OPTIMIZE_CTR                     = 0xBE00,
    /*end: mod , for NV decoup*/
    /* begin boston dl cat 18 add */
    EN_NV_ID_UE_CAP_V1320                           = 0xBE01,
    EN_NV_ID_UE_CAP_V1330                           = 0xBE02,
    /* end boston dl cat 18 add */
    EN_NV_ID_VOLTE_END_EVAB1B2_CTR_INFO             = 0xBF9B
};
typedef unsigned long NV_ITEM_ID_ENUM_UINT32;

/*****************************************************************************
    PS NV值使用范围：0xD200 ~ 0xD38F
    0xD390 ~ 0xD3bf已用作CA
*****************************************************************************/


/* this file is included by drv, */
/*
#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif
*/



#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of PsNvInterface.h */

