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

#ifndef __RRCNVINTERFACE_H__
#define __RRCNVINTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 Include Headfile
*****************************************************************************/
#include  "PsTypeDef.h"

#pragma pack(4)

/*****************************************************************************
  2 macro
*****************************************************************************/
	/* 非标准频段 begin */
#define LTE_COMM_NONSTANDARD_BAND_BEGIN                     101
#define LTE_COMM_NONSTANDARD_BAND_END                       116

#define LTE_COMM_NONSTANDARD_BAND_MAX_COUNT                 16
	/* 非标准频段 end */


/* reportCGI stub Begin */
/* CMM保存小区CGI信息列表结构 */
#define RRC_NV_USER_SET_CGI_MAX_NUM             (5)
#define RRC_NV_AUTO_SRCH_CGI_MAX_NUM            (16)
#define RRC_UU_MAX_NUM_OF_PLMN_ID_LIST_2        (5)
#define RRC_UU_MNC_MCC_MAX_LEN                  (3)
/* reportCGI stub end */

/* Begin: 2016/1/4 cr develop */
#define RRC_UU_MAX_NUM_OF_BAND_COMB_R11          (21)
#define RRC_UU_MAX_NUM_OF_BAND_COMB_R10          (20)
#define RRC_UU_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10 (4)
#define RRC_UU_MAX_NUM_OF_BANDWIDTH_CLASS_R10    (4)

/* BOSTON add beign */
#define RRC_UU_MAX_NUM_OF_OTHER_BAND_COMB_V10i0     (6)
#define RRC_UU_MAX_NUM_OF_OTHER_BAND_COMB_V11d0     (12)
/* BOSTON add end */

/* End: 2016/1/4 cr develop */

#define RRC_NV_CSEL_MAX_INTER_FREQ_COUNT        (8)
#define RRC_NV_EACH_BAND_FREQ_COUNT             (32)                            /* 每个band中nv存储的频点个数 */
#define RRC_MAX_NUM_OF_COUNT_IN_ONE_BAND        (4)
#define RRC_NV_CSEL_MAX_STORED_CELL_COUNT_PER_FREQ (4)
#define RRC_CSEL_MAX_FREQ_COUNT                 (128)                            /* csel本地保存的频点最大数 */
#define RRC_NV_EACH_BAND_BANDS_COUNT            (4)

#define RRC_NV_LWCLASH_RANGE_COUNT              (16)

/* UE能力相关宏定义 */
#define RRC_MAX_NUM_OF_BANDS                    (64)
#define RRC_MAX_NUM_OF_CDMA_BAND_CLASS          (32)
#define RRC_MAX_NUM_OF_FEATURE_GROUP            (32)
/* add for MFBI begin */
#define RRC_FGI_MULTIBAND_SUPPORT_IN_ARRAY      (30)
/* add for MFBI end */

/* add for ANR begin */
#define RRC_FGI_ANR_INTRA_SUPPORT_IN_ARRAY      (16)
#define RRC_FGI_ANR_INTER_SUPPORT_IN_ARRAY      (17)
#define RRC_FGI_ANR_IRAT_SUPPORT_IN_ARRAY       (18)
/* add for ANR end */

/*begin: add for feature v700r500*/
#define RRC_MAX_NUM_OF_INTER_BAND_TDD_CA_DIFF_CFG_R11  (2)
#define RRC_MAX_BYTE_LEN_OF_LATE_NON_CRITIC_EXT        (256)
#define RRC_MAX_REQ_BANDS_R11                          (64)
#define RRC_MAX_NUM_OF_SUPPORT_BAND_WIDTH_COMB_SET_R10 (4)
/*end: add for feature v700r500*/

/* begin: add for c50 ue cap,   */
#define RRC_MAX_SUPP_NAICS_2CRS_AP_R12                  (1)
#define RRC_MAX_BIT_LEN_OF_COMM_SUPPBANDS_PERBC_R12     (8)
/* boston modify begin */
#define RRC_MAX_SUPP_BAND_COMB_V1250                    (64)    /*协议值为128，数组过大，拆分为2个64*/
#define RRC_MAX_SUPP_BAND_COMB_ADD_V1250                (64)    /*协议值为256，数组过大，拆分为4个64*/
/* boston modify end */

#define RRC_MAX_SUPP_BANDLIST_EUTRA_V1250               (64)
#define RRC_MAX_NAICS_CAP_LIST_R12                      (8)
#define RRC_MAX_FREQ_BANDINDLIST_EUTRA_R12              (64)
#define RRC_MAX_SUPP_BANDINFO_LIST_R12                  (64)
/* end: add for c50 ue cap,   */

/* v7r2 LLD begin */
/*begin: modify for feature v700r500*/
#define RRC_MAX_NUM_OF_BAND_COMB_R10            (64)  /*协议值为128，数组过大，拆分为2个64*/
#define RRC_MAX_NUM_OF_BAND_COMB_R11            (64)  /*协议值为256，数组过大，拆分为4个64*/
/*end: modify for feature v700r500*/

#define RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10   (4)
#define RRC_MAX_NUM_OF_BANDWIDTH_CLASS_R10      (2)

#define RRC_MAX_NUM_OF_SUPPORT_BAND_COMB_EXT_R10 (128)

/* v7r2 LLD end */

#define RRC_REL8_ACCESS_STRATUM_REL             0x01
#define RRC_REL9_ACCESS_STRATUM_REL             0x02
/* v7r2 code begin */
#define RRC_REL10_ACCESS_STRATUM_REL            0x04
/* v7r2 code end */
/*begin: add for feature v700r500*/
#define RRC_REL11_ACCESS_STRATUM_REL            0x08
#define RRC_REL12_ACCESS_STRATUM_REL            0x10
/*end: add for feature v700r500*/

/* begin:add for Boston   */
#define RRC_REL13_ACCESS_STRATUM_REL            0x20
/* end:add for Boston   */

/* begin: add for MCC report CA,   */
#define RRC_NUM_OF_MCC_CA_NV                    (7)     /* mcc NV个数 */
#define RRC_MAX_NUM_OF_MCC                      (15)    /* 规格值100，NV超出大小，拆成7个NV */
#define RRC_MAX_NUM_OF_CA_PER_MCC               (32)
/* end: add for MCC report CA,   */

/* Begin: add for Report Ue Capability By MCC */
#define RRC_NUM_OF_MCC_BAND_NV                       (70)     /* mcc NV个数 */
#define LRRC_LTE_MAX_NUM_OF_BAND_PER_MCC             (16)
#define LRRC_UTRAN_MAX_NUM_OF_BAND_PER_MCC           (8)
/* End: add for Report Ue Capability By MCC */

/* Added by   for ue cap report, 2016-6-17, begin */
#define RRC_MIN_NUM_OF_CA                            (32)
#define RRC_MIN_NUM_OF_BAND                          (32)
/* Added by   for ue cap report, 2016-6-17, end */
/* boston add begin */
#define RRC_MAX_NUM_OF_SERV_CELL_R10                      (5)           /* 协议值:5 */

#define RRC_MAX_NUM_OF_SERV_CELL_R13                      (32)          /* 协议值:32 */
#define RRC_MAX_NUM_OF_NAICS_ENTRIES_R12                  (1)          /* 协议值:(8 + 7)/8 */
#define RRC_MAX_BIT_LEN_OF_BANDS                          (8)          /* 协议值:(64 + 7)/8 */
#define RRC_MAX_BIT_LEN_OF_WLAN_MAC_ADDRESS_R13           (6)           /* 协议值:6 */

#define RRC_MAX_NUM_OF_BAND_COMB_R13                      (64)     /*协议值为384，数组过大，拆分为6个64*/
#define RRC_MAX_BIT_LEN_OF_N_MAX_LIST                     (1)          /* 协议值:(7 + 7)/8 */
#define RRC_MAX_NUM_CSI_PROCESS_RELEASELIST_R11           (4)           /* 协议值:4 */

/* 规格裁剪 */
#define RRC_MAX_NUM_OF_WLAN_BANDS_R13                     (1)           /* 协议值:8 */
/* boston add end */
/*****************************************************************************
  3 Massage Declare
*****************************************************************************/


/*****************************************************************************
  4 Enum
*****************************************************************************/
/*****************************************************************************
 枚举名    : LRRC_LTE_MODE_TYPE_ENUM_UINT16
 协议表格  :
 ASN.1描述 :
 枚举说明  : LTE模式类型
*****************************************************************************/
enum LTE_COMM_BAND_MODE_TYPE_ENUM
{
    LTE_COMM_BAND_MODE_TYPE_FDD         = 0,            /* LTE FDD 制式 */
    LTE_COMM_BAND_MODE_TYPE_TDD         = 1,            /* LTE TDD 制式 */
    LTE_COMM_BAND_MODE_TYPE_BUTT
};
typedef VOS_UINT8 LTE_COMM_BAND_MODE_TYPE_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CATEGORY_ENUM_UINT8
 协议表格  :
 ASN.1描述 :
 枚举说明  : UE Category
*****************************************************************************/
enum RRC_UE_CATEGORY_ENUM
{
    RRC_UE_CATEGORY_1                   = 0,
    RRC_UE_CATEGORY_2                   = 1,
    RRC_UE_CATEGORY_3                   = 2,
    RRC_UE_CATEGORY_4                   = 3,
    RRC_UE_CATEGORY_5                   = 4,
    RRC_UE_CATEGORY_BUTT
};
typedef VOS_UINT8 RRC_UE_CATEGORY_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CATEGORY_EXT_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : UE Category Ext
*****************************************************************************/
enum RRC_UE_CATEGORY_EXT_ENUM
{
    RRC_UE_CATEGORY_6                   = 6,
    RRC_UE_CATEGORY_7                   = 7,
    RRC_UE_CATEGORY_8                   = 8,
    RRC_UE_CATEGORY_9                   = 9,
    RRC_UE_CATEGORY_10                  = 10,
    RRC_UE_CATEGORY_11                  = 11,
    RRC_UE_CATEGORY_12                  = 12,
    RRC_UE_CATEGORY_EXT_BUTT
};
typedef VOS_UINT8 RRC_UE_CATEGORY_EXT_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_ROH_CONTEXT_ENUM_UINT8
 协议表格  :
 ASN.1描述 :
 枚举说明  : maxNumberROHC-ContextSessions
*****************************************************************************/
enum RRC_UE_CAP_ROH_CONTEXT_ENUM
{
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS2    = 0,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS4    ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS8    ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS12   ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS16   ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS24   ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS32   ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS48   ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS64   ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS128  ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS256  ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS512  ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS1024 ,
    RRC_MAX_NUM_ROHC_CTX_SESSION_CS16384,
    RRC_MAX_NUM_ROHC_CTX_SESSION_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_ROH_CONTEXT_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_UTRA_FDD_BAND_ENUM_UINT8
 协议表格  :
 ASN.1描述 :
 枚举说明  : utra-FDD-Band
*****************************************************************************/
enum RRC_UE_CAP_UTRA_FDD_BAND_ENUM
{
    RRC_UE_CAP_UTRA_FDD_BAND_I          = 0,
    RRC_UE_CAP_UTRA_FDD_BAND_II         = 1,
    RRC_UE_CAP_UTRA_FDD_BAND_III        = 2,
    RRC_UE_CAP_UTRA_FDD_BAND_IV         = 3,
    RRC_UE_CAP_UTRA_FDD_BAND_V          = 4,
    RRC_UE_CAP_UTRA_FDD_BAND_VI         = 5,
    RRC_UE_CAP_UTRA_FDD_BAND_VII        = 6,
    RRC_UE_CAP_UTRA_FDD_BAND_VIII       = 7,
    RRC_UE_CAP_UTRA_FDD_BAND_IX         = 8,
    RRC_UE_CAP_UTRA_FDD_BAND_X          = 9,
    RRC_UE_CAP_UTRA_FDD_BAND_XI         = 10,
    RRC_UE_CAP_UTRA_FDD_BAND_XII        = 11,
    RRC_UE_CAP_UTRA_FDD_BAND_XIII       = 12,
    RRC_UE_CAP_UTRA_FDD_BAND_XIV        = 13,
    RRC_UE_CAP_UTRA_FDD_BAND_XV         = 14,
    RRC_UE_CAP_UTRA_FDD_BAND_XVI        = 15,
    RRC_UE_CAP_UTRA_FDD_BAND_XVII       = 16,
    RRC_UE_CAP_UTRA_FDD_BAND_XVIII      = 17,
    RRC_UE_CAP_UTRA_FDD_BAND_XIX        = 18,
    RRC_UE_CAP_UTRA_FDD_BAND_XX         = 19,
    RRC_UE_CAP_UTRA_FDD_BAND_XXI        = 20,
    RRC_UE_CAP_UTRA_FDD_BAND_XXII       = 21,
    RRC_UE_CAP_UTRA_FDD_BAND_XXIII      = 22,
    RRC_UE_CAP_UTRA_FDD_BAND_XXIV       = 23,
    RRC_UE_CAP_UTRA_FDD_BAND_XXV        = 24,
    RRC_UE_CAP_UTRA_FDD_BAND_XXVI       = 25,
    RRC_UE_CAP_UTRA_FDD_BAND_XXVII      = 26,
    RRC_UE_CAP_UTRA_FDD_BAND_XXVIII     = 27,
    RRC_UE_CAP_UTRA_FDD_BAND_XXIX       = 28,
    RRC_UE_CAP_UTRA_FDD_BAND_XXX        = 29,
    RRC_UE_CAP_UTRA_FDD_BAND_XXXI       = 30,
    RRC_UE_CAP_UTRA_FDD_BAND_XXXII      = 31,
    RRC_UE_CAP_UTRA_FDD_BAND_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_UTRA_FDD_BAND_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_UTRA_TDD128_BAND_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : utra-TDD128Band
*****************************************************************************/
enum RRC_UE_CAP_UTRA_TDD128_BAND_ENUM
{
    RRC_UE_CAP_UTRA_TDD128_BAND_A       = 0,
    RRC_UE_CAP_UTRA_TDD128_BAND_B       = 1,
    RRC_UE_CAP_UTRA_TDD128_BAND_C       = 2,
    RRC_UE_CAP_UTRA_TDD128_BAND_D       = 3,
    RRC_UE_CAP_UTRA_TDD128_BAND_E       = 4,
    RRC_UE_CAP_UTRA_TDD128_BAND_F       = 5,
    RRC_UE_CAP_UTRA_TDD128_BAND_G       = 6,
    RRC_UE_CAP_UTRA_TDD128_BAND_H       = 7,
    RRC_UE_CAP_UTRA_TDD128_BAND_I       = 8,
    RRC_UE_CAP_UTRA_TDD128_BAND_J       = 9,
    RRC_UE_CAP_UTRA_TDD128_BAND_K       = 10,
    RRC_UE_CAP_UTRA_TDD128_BAND_L       = 11,
    RRC_UE_CAP_UTRA_TDD128_BAND_M       = 12,
    RRC_UE_CAP_UTRA_TDD128_BAND_N       = 13,
    RRC_UE_CAP_UTRA_TDD128_BAND_O       = 14,
    RRC_UE_CAP_UTRA_TDD128_BAND_P       = 15,
    RRC_UE_CAP_UTRA_TDD128_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_UTRA_TDD128_BAND_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_UTRA_TDD384_BAND_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : utra-TDD384Band
*****************************************************************************/
enum RRC_UE_CAP_UTRA_TDD384_BAND_ENUM
{
    RRC_UE_CAP_UTRA_TDD384_BAND_A       = 0,
    RRC_UE_CAP_UTRA_TDD384_BAND_B       = 1,
    RRC_UE_CAP_UTRA_TDD384_BAND_C       = 2,
    RRC_UE_CAP_UTRA_TDD384_BAND_D       = 3,
    RRC_UE_CAP_UTRA_TDD384_BAND_E       = 4,
    RRC_UE_CAP_UTRA_TDD384_BAND_F       = 5,
    RRC_UE_CAP_UTRA_TDD384_BAND_G       = 6,
    RRC_UE_CAP_UTRA_TDD384_BAND_H       = 7,
    RRC_UE_CAP_UTRA_TDD384_BAND_I       = 8,
    RRC_UE_CAP_UTRA_TDD384_BAND_J       = 9,
    RRC_UE_CAP_UTRA_TDD384_BAND_K       = 10,
    RRC_UE_CAP_UTRA_TDD384_BAND_L       = 11,
    RRC_UE_CAP_UTRA_TDD384_BAND_M       = 12,
    RRC_UE_CAP_UTRA_TDD384_BAND_N       = 13,
    RRC_UE_CAP_UTRA_TDD384_BAND_O       = 14,
    RRC_UE_CAP_UTRA_TDD384_BAND_P       = 15,
    RRC_UE_CAP_UTRA_TDD384_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_UTRA_TDD384_BAND_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_UTRA_TDD768_BAND_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : utra-TDD768Band
*****************************************************************************/
enum RRC_UE_CAP_UTRA_TDD768_BAND_ENUM
{
    RRC_UE_CAP_UTRA_TDD768_BAND_A       = 0,
    RRC_UE_CAP_UTRA_TDD768_BAND_B       = 1,
    RRC_UE_CAP_UTRA_TDD768_BAND_C       = 2,
    RRC_UE_CAP_UTRA_TDD768_BAND_D       = 3,
    RRC_UE_CAP_UTRA_TDD768_BAND_E       = 4,
    RRC_UE_CAP_UTRA_TDD768_BAND_F       = 5,
    RRC_UE_CAP_UTRA_TDD768_BAND_G       = 6,
    RRC_UE_CAP_UTRA_TDD768_BAND_H       = 7,
    RRC_UE_CAP_UTRA_TDD768_BAND_I       = 8,
    RRC_UE_CAP_UTRA_TDD768_BAND_J       = 9,
    RRC_UE_CAP_UTRA_TDD768_BAND_K       = 10,
    RRC_UE_CAP_UTRA_TDD768_BAND_L       = 11,
    RRC_UE_CAP_UTRA_TDD768_BAND_M       = 12,
    RRC_UE_CAP_UTRA_TDD768_BAND_N       = 13,
    RRC_UE_CAP_UTRA_TDD768_BAND_O       = 14,
    RRC_UE_CAP_UTRA_TDD768_BAND_P       = 15,
    RRC_UE_CAP_UTRA_TDD768_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_UTRA_TDD768_BAND_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_GERAN_BAND_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : geran-Band
*****************************************************************************/
enum RRC_UE_CAP_GERAN_BAND_ENUM
{
    RRC_UE_CAP_GSM450_GERAN_BAND = 0,
    RRC_UE_CAP_GSM480_GERAN_BAND,
    RRC_UE_CAP_GSM710_GERAN_BAND,
    RRC_UE_CAP_GSM750_GERAN_BAND,
    RRC_UE_CAP_GSM810_GERAN_BAND,
    RRC_UE_CAP_GSM850_GERAN_BAND,
    RRC_UE_CAP_GSM900P_GERAN_BAND,
    RRC_UE_CAP_GSM900E_GERAN_BAND,
    RRC_UE_CAP_GSM900R_GERAN_BAND,
    RRC_UE_CAP_GSM1800_GERAN_BAND,
    RRC_UE_CAP_GSM1900_GERAN_BAND,
    RRC_UE_CAP_GERAN_BAND_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_GERAN_BAND_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_CDMA2000_HRPD_TX_CONFIG_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : cdma2000-HRPD-TxConfig
*****************************************************************************/
enum RRC_UE_CAP_CDMA2000_HRPD_TX_CONFIG_ENUM
{
    RRC_UE_CAP_CDMA2000_HRPD_TX_SINGLE  = 0,
    RRC_UE_CAP_CDMA2000_HRPD_TX_DUAL    = 1,
    RRC_UE_CAP_CDMA2000_HRPD_TX_BUTT
} ;
typedef VOS_UINT8 RRC_UE_CAP_CDMA2000_HRPD_TX_CONFIG_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_CDMA2000_HRPD_RX_CONFIG_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : cdma2000-HRPD-RxConfig
*****************************************************************************/
enum RRC_UE_CAP_CDMA2000_HRPD_RX_CONFIG_ENUM
{
    RRC_UE_CAP_CDMA2000_HRPD_RX_SINGLE  = 0,
    RRC_UE_CAP_CDMA2000_HRPD_RX_DUAL    = 1,
    RRC_UE_CAP_CDMA2000_HRPD_RX_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_CDMA2000_HRPD_RX_CONFIG_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_CDMA2000_1X_RTT_TX_CONFIG_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : cdma2000-1xRTT-TxConfig
*****************************************************************************/
enum RRC_UE_CAP_CDMA2000_1X_RTT_TX_CONFIG_ENUM
{
    RRC_UE_CAP_CDMA2000_1X_RTT_TX_SINGLE = 0,
    RRC_UE_CAP_CDMA2000_1X_RTT_TX_DUAL = 1,
    RRC_UE_CAP_CDMA2000_1X_RTT_TX_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_CDMA2000_1X_RTT_TX_CONFIG_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_CDMA2000_1X_RTT_RX_CONFIG_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : cdma2000-1xRTT-RxConfig
*****************************************************************************/
enum RRC_UE_CAP_CDMA2000_1X_RTT_RX_CONFIG_ENUM
{
    RRC_UE_CAP_CDMA2000_1X_RTT_RX_SINGLE = 0,
    RRC_UE_CAP_CDMA2000_1X_RTT_RX_DUAL   = 1,
    RRC_UE_CAP_CDMA2000_1X_RTT_RX_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_CDMA2000_1X_RTT_RX_CONFIG_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_CDMA2000_BANDCLASS_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : CDMA2000-Bandclass
*****************************************************************************/
enum RRC_UE_CAP_CDMA2000_BANDCLASS_ENUM
{
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC0   = 0,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC1   = 1,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC2   = 2,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC3   = 3,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC4   = 4,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC5   = 5,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC6   = 6,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC7   = 7,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC8   = 8,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC9   = 9,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC10  = 10,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC11  = 11,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC12  = 12,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC13  = 13,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC14  = 14,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC15  = 15,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC16  = 16,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BC17  = 17,
    RRC_UE_CAP_CDMA2000_BANDCLASS_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_CDMA2000_BANDCLASS_ENUM_UINT8;

/*****************************************************************************
 结构名    : RRC_NV_LOCK_INFO_ENUM
 协议表格  :
 ASN.1描述 :
 结构说明  : 锁定指示
 *****************************************************************************/
enum RRC_NV_LOCK_INFO_ENUM
{
    RRC_NV_LOCK_NONE           = 0,                                           /* 不锁定 */
    RRC_NV_LOCK_BAND,                                                         /* 锁定BAND */
    RRC_NV_LOCK_FREQ,                                                         /* 锁定频点 */
    RRC_NV_LOCK_CELL,                                                         /* 锁定小区 */
    RRC_NV_LOCK_BUTT
};
typedef VOS_UINT32  RRC_NV_LOCK_INFO_ENUM_UINT32;
/*****************************************************************************
 结构名    : RRC_NV_UE_RFRD_TYPE_ENUM
 协议表格  :
 ASN.1描述 :
 结构说明  : 射频类型指示
 *****************************************************************************/
enum RRC_NV_UE_RFRD_TYPE_ENUM
{
    RRC_NV_UE_RFRD_HI6360           = 0,
    RRC_NV_UE_RFRD_AD9361,
    RRC_NV_UE_RFRD_BUTT
};
typedef VOS_UINT8  RRC_NV_UE_RFRD_TYPE_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : 用来指示特性是否支持
*****************************************************************************/
enum RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM
{
    RRC_UE_CAP_FEATURE_SUPPORTED     = 0,
    RRC_UE_CAP_FEATURE_NOT_SUPPORTED = 1,
    RRC_UE_CAP_FEATURE_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_UE_CAP_DEVICE_TYPE_R9_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : 用来指示是否获取NW-based省电优化
*****************************************************************************/
enum RRC_UE_CAP_DEVICE_TYPE_R9_ENUM
{
    RRC_NO_BEN_FROM_BAT_CONSUMP_OPT   = 0,
    RRC_BEN_FROM_BAT_CONSUMP_OPT      = 1,
    RRC_BAT_CONSUMP_OPT_BUTT
};
typedef VOS_UINT8 RRC_UE_CAP_DEVICE_TYPE_R9_ENUM_UINT8;

/* v7r2 LLD begin */
/*****************************************************************************
 枚举名    : RRC_CA_BANDWIDTH_CLASS_R10_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  :
*****************************************************************************/
enum RRC_CA_BANDWIDTH_CLASS_R10_ENUM
{
    RRC_CA_BANDWIDTH_CLASS_A          = 0,
    RRC_CA_BANDWIDTH_CLASS_B,
    RRC_CA_BANDWIDTH_CLASS_C,
    RRC_CA_BANDWIDTH_CLASS_D,
    RRC_CA_BANDWIDTH_CLASS_E,
    RRC_CA_BANDWIDTH_CLASS_F
};
typedef VOS_UINT8 RRC_CA_BANDWIDTH_CLASS_R10_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_MIMO_CAP_UL_R10_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  :
*****************************************************************************/
enum RRC_MIMO_CAP_UL_R10_ENUM
{
    RRC_MIMO_CAP_UL_TWO_LAYERS        = 0,
    RRC_MIMO_CAP_UL_FOUR_LAYERS       = 1
};
typedef VOS_UINT8 RRC_MIMO_CAP_UL_R10_ENUM_UINT8;

/*****************************************************************************
 枚举名    : RRC_MIMO_CAP_DL_R10_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  :
*****************************************************************************/
enum RRC_MIMO_CAP_DL_R10_ENUM
{
    RRC_TWO_LAYERS_MIMO_CAP_DL_R10               = 0,
    RRC_FOUR_LAYERS_MIMO_CAP_DL_R10              = 1,
    RRC_EIGHT_LAYERS_MIMO_CAP_DL_R10             = 2
};
typedef VOS_UINT8 RRC_MIMO_CAP_DL_R10_ENUM_UINT8;

/*********************************************************************
枚举名    : RRC_PDCP_SN_EXT_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_PDCP_SN_EXT_R11                     = 0
}RRC_PDCP_SN_EXT_R11_ENUM;
typedef VOS_UINT8                 RRC_PDCP_SN_EXT_R11_ENUM_UINT8;
/*********************************************************************
枚举名    : RRC_SUPPORT_ROHC_CONTEXT_CONTINUE_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_SUPPORT_ROHC_CONTEXT_CONTINUE_R11   = 0
}RRC_SUPPORT_ROHC_CONTEXT_CONTINUE_R11_ENUM;
typedef VOS_UINT8                 RRC_SUPPORT_ROHC_CONTEXT_CONTINUE_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: crs-InterfHandl-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_CRS_INTERF_HANDL_R11                = 0
}RRC_CRS_INTERF_HANDL_R11_ENUM;
typedef VOS_UINT8                 RRC_CRS_INTERF_HANDL_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: ePDCCH-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_EPDCCH_R11                          = 0
}RRC_EPDCCH_R11_ENUM;
typedef VOS_UINT8                 RRC_EPDCCH_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: multiACK-CSI-Reporting-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_MULTI_ACK_CSI_RPRTING_R11           = 0
}RRC_MULTI_ACK_CSI_RPRTING_R11_ENUM;
typedef VOS_UINT8                 RRC_MULTI_ACK_CSI_RPRTING_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: ss-CCH-InterfHandl-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_SS_CCH_INTERF_HANDL_R11             = 0
}RRC_SS_CCH_INTERF_HANDL_R11_ENUM;
typedef VOS_UINT8                 RRC_SS_CCH_INTERF_HANDL_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: tdd-SpecialSubframe-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_TDD_SPECIAL_SUBFRAME_R11            = 0
}RRC_TDD_SPECIAL_SUBFRAME_R11_ENUM;
typedef VOS_UINT8                 RRC_TDD_SPECIAL_SUBFRAME_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: txDiv-PUCCH1b-ChSelect-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_TX_DIV_PUCCH1B_CH_SELECT_R11        = 0
}RRC_TX_DIV_PUCCH1B_CH_SELECT_R11_ENUM;
typedef VOS_UINT8                 RRC_TX_DIV_PUCCH1B_CH_SELECT_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: ul-CoMP-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_UL_COMP_R11                         = 0
}RRC_UL_COMP_R11_ENUM;
typedef VOS_UINT8                 RRC_UL_COMP_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: multipleTimingAdvance-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP                                     = 0
}RRC_MULTIPLE_TIMING_ADVANCE_R11_ENUM;
typedef VOS_UINT8                 RRC_MULTIPLE_TIMING_ADVANCE_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: simultaneousRx-Tx-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_SIMUL_RX_TX_R11                     = 0
}RRC_SIMUL_RX_TX_R11_ENUM;
typedef VOS_UINT8                 RRC_SIMUL_RX_TX_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: supportedCSI-Proc-r11
* 取值范围: ENUM(0..2)
*********************************************************************/
typedef enum
{
    RRC_N1_SUPP_CSI_PROC_R11                     = 0,
    RRC_N3_SUPP_CSI_PROC_R11                     = 1,
    RRC_N4_SUPP_CSI_PROC_R11                     = 2
}RRC_SUPP_CSI_PROC_R11_ENUM;
typedef VOS_UINT8                 RRC_SUPP_CSI_PROC_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: rsrqMeasWideband-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_RSRQ_MEAS_WIDEBAND_R11              = 0
}RRC_RSRQ_MEAS_WIDEBAND_R11_ENUM;
typedef VOS_UINT8                 RRC_RSRQ_MEAS_WIDEBAND_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: cdma2000-NW-Sharing-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_CDMA2000_NW_SHARING_R11             = 0
}RRC_CDMA2000_NW_SHARING_R11_ENUM;
typedef VOS_UINT8                 RRC_CDMA2000_NW_SHARING_R11_ENUM_UINT8;


/*********************************************************************
* ASN 文本: inDeviceCoexInd-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_IN_DEVICE_COEX_IND_R11              = 0
}RRC_IN_DEVICE_COEX_IND_R11_ENUM;
typedef VOS_UINT8                 RRC_IN_DEVICE_COEX_IND_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: powerPrefInd-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_PWR_PREF_IND_R11                    = 0
}RRC_PWR_PREF_IND_R11_ENUM;
typedef VOS_UINT8                 RRC_PWR_PREF_IND_R11_ENUM_UINT8;

/*********************************************************************
* ASN 文本: ue-Rx-TxTimeDiffMeasurements-r11
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_UE_RX_TX_TIME_DIFF_MEAS_R11         = 0
}RRC_UE_RX_TX_TIME_DIFF_MEAS_R11_ENUM;
typedef VOS_UINT8                 RRC_UE_RX_TX_TIME_DIFF_MEAS_R11_ENUM_UINT8;

/* v7r2 LLD end */

/*begin: add for feature v700r500 ,  */
/*********************************************************************
枚举名    : RRC_FREQ_BAND_RETRIEVAL_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_FREQ_BAND_RETRIEVAL_R11         = 0
}RRC_FREQ_BAND_RETRIEVAL_R11_ENUM;
typedef VOS_UINT8                 RRC_FREQ_BAND_RETRIEVAL_R11_ENUM_UINT8;

/*********************************************************************
枚举名    : RRC_SUPP_CSI_PROC_R11_BAND_PARAM_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_N1_SUPP_CSI_PROC_R11_BAND_PARAM_R11      = 0,
    RRC_N3_SUPP_CSI_PROC_R11_BAND_PARAM_R11      = 1,
    RRC_N4_SUPP_CSI_PROC_R11_BAND_PARAM_R11      = 2
}RRC_SUPP_CSI_PROC_R11_BAND_PARAM_R11_ENUM;
typedef VOS_UINT8                 RRC_SUPP_CSI_PROC_R11_BAND_PARAM_R11_ENUM_UINT8;

/*********************************************************************
枚举名    : RRC_MULTIPLE_TIMING_ADVANCE_R11_BAND_COMB_PARAM_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_MULTIPLE_TIMING_ADVANCE_R11_BAND_COMB_PARAM_R11    = 0
}RRC_MULTIPLE_TIMING_ADVANCE_R11_BAND_COMB_PARAM_R11_ENUM;
typedef VOS_UINT8                 RRC_MULTIPLE_TIMING_ADVANCE_R11_BAND_COMB_PARAM_R11_ENUM_UINT8;

/*********************************************************************
枚举名    : RRC_SIMUL_RX_TX_R11_BAND_COMB_PARAM_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_SIMUL_RX_TX_R11_BAND_COMB_PARAM_R11 = 0
}RRC_SIMUL_RX_TX_R11_BAND_COMB_PARAM_R11_ENUM;
typedef VOS_UINT8                 RRC_SIMUL_RX_TX_R11_BAND_COMB_PARAM_R11_ENUM_UINT8;

/*********************************************************************
枚举名    : RRC_MBMS_SCELL_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_MBMS_SCELL_R11                      = 0
}RRC_MBMS_SCELL_R11_ENUM;
typedef VOS_UINT8                 RRC_MBMS_SCELL_R11_ENUM_UINT8;

/*********************************************************************
枚举名    : RRC_MBMS_NON_SERVING_CELL_R11_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_MBMS_NON_SERVING_CELL_R11           = 0
}RRC_MBMS_NON_SERVING_CELL_R11_ENUM;
typedef VOS_UINT8                 RRC_MBMS_NON_SERVING_CELL_R11_ENUM_UINT8;

/*********************************************************************
枚举名    : RRC_EXTENDED_RLC_LI_FIELD_R12_ENUM
协议表格  :
ASN.1描述 :
枚举说明  :
*********************************************************************/
typedef enum
{
    RRC_SUPP_EXTENDED_RLC_LI_FIELD_R12           = 0
}RRC_EXTENDED_RLC_LI_FIELD_R12_ENUM;
typedef VOS_UINT8                 RRC_EXTENDED_RLC_LI_FIELD_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: e-HARQ-Pattern-FDD-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_E_HARQ_PATTERN_FDD_R12              = 0
}RRC_E_HARQ_PATTERN_FDD_R12_ENUM;
typedef VOS_UINT8                 RRC_E_HARQ_PATTERN_FDD_R12_ENUM_UINT8;
/*end: add for feature v700r500 ,  */

/* begin: add for c50 ue cap,   */
/*********************************************************************
* ASN 文本: enhanced-4TxCodebook-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_ENHNC_4TX_CODEBOOK_R12              = 0
}RRC_ENHNC_4TX_CODEBOOK_R12_ENUM;
typedef VOS_UINT8                 RRC_ENHNC_4TX_CODEBOOK_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: phy-TDD-ReConfig-TDDPCell-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_PHY_TDD_RE_CONFIG_TDD_PCELL_R12     = 0
}RRC_PHY_TDD_RE_CONFIG_TDD_PCELL_R12_ENUM;
typedef VOS_UINT8                 RRC_PHY_TDD_RE_CONFIG_TDD_PCELL_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: phy-TDD-ReConfig-FDDPCell-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_PHY_TDD_RE_CONFIG_FDD_PCELL_R12     = 0
}RRC_PHY_TDD_RE_CONFIG_FDD_PCELL_R12_ENUM;
typedef VOS_UINT8                 RRC_PHY_TDD_RE_CONFIG_FDD_PCELL_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: pusch-FeedbackMode-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_PUSCH_FEEDBACK_MODE_R12             = 0
}RRC_PUSCH_FEEDBACK_MODE_R12_ENUM;
typedef VOS_UINT8                 RRC_PUSCH_FEEDBACK_MODE_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: pusch-SRS-PowerControl-SubframeSet-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_PUSCH_SRS_PWR_CTRL_SUBFRAME_SET_R12 = 0
}RRC_PUSCH_SRS_PWR_CTRL_SUBFRAME_SET_R12_ENUM;
typedef VOS_UINT8                 RRC_PUSCH_SRS_PWR_CTRL_SUBFRAME_SET_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: csi-SubframeSet-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_CSI_SUBFRAME_SET_R12                = 0
}RRC_CSI_SUBFRAME_SET_R12_ENUM;
typedef VOS_UINT8                 RRC_CSI_SUBFRAME_SET_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: noResourceRestrictionForTTIBundling-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_NO_RSRC_RSTRIC_FOR_TTI_BUNDLING_R12 = 0
}RRC_NO_RSRC_RSTRIC_FOR_TTI_BUNDLING_R12_ENUM;
typedef VOS_UINT8                 RRC_NO_RSRC_RSTRIC_FOR_TTI_BUNDLING_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: discoverySignalsInDeactSCell-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_DISCOVERY_SIGNALS_IN_DEACT_SCELL_R12    = 0
}RRC_DISCOVERY_SIGNALS_IN_DEACT_SCELL_R12_ENUM;
typedef VOS_UINT8                 RRC_DISCOVERY_SIGNALS_IN_DEACT_SCELL_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: numberOfAggregatedPRB-r12
* 取值范围: ENUM(0..15)
*********************************************************************/
typedef enum
{
    RRC_N50_NUM_OF_AGGREGATED_PRB_R12            = 0,
    RRC_N75_NUM_OF_AGGREGATED_PRB_R12            = 1,
    RRC_N100_NUM_OF_AGGREGATED_PRB_R12           = 2,
    RRC_N125_NUM_OF_AGGREGATED_PRB_R12           = 3,
    RRC_N150_NUM_OF_AGGREGATED_PRB_R12           = 4,
    RRC_N175_NUM_OF_AGGREGATED_PRB_R12           = 5,
    RRC_N200_NUM_OF_AGGREGATED_PRB_R12           = 6,
    RRC_N225_NUM_OF_AGGREGATED_PRB_R12           = 7,
    RRC_N250_NUM_OF_AGGREGATED_PRB_R12           = 8,
    RRC_N275_NUM_OF_AGGREGATED_PRB_R12           = 9,
    RRC_N300_NUM_OF_AGGREGATED_PRB_R12           = 10,
    RRC_N350_NUM_OF_AGGREGATED_PRB_R12           = 11,
    RRC_N400_NUM_OF_AGGREGATED_PRB_R12           = 12,
    RRC_N450_NUM_OF_AGGREGATED_PRB_R12           = 13,
    RRC_N500_NUM_OF_AGGREGATED_PRB_R12           = 14,
    RRC_SPARE_NUM_OF_AGGREGATED_PRB_R12          = 15
}RRC_NUM_OF_AGGREGATED_PRB_R12_ENUM;
typedef VOS_UINT8                 RRC_NUM_OF_AGGREGATED_PRB_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: dl-256QAM-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_DL_256_QAM_R12                      = 0
}RRC_DL_256_QAM_R12_ENUM;
typedef VOS_UINT8                 RRC_DL_256_QAM_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: ul-64QAM-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_UL_64_QAM_R12                       = 0
}RRC_UL_64_QAM_R12_ENUM;
typedef VOS_UINT8                 RRC_UL_64_QAM_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: asynchronous-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_ASYNCHRONOUS_R12                    = 0
}RRC_ASYNCHRONOUS_R12_ENUM;
typedef VOS_UINT8                 RRC_ASYNCHRONOUS_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: freqBandPriorityAdjustment-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_FREQ_BAND_PRIOR_ADJ_R12             = 0
}RRC_FREQ_BAND_PRIOR_ADJ_R12_ENUM;
typedef VOS_UINT8                 RRC_FREQ_BAND_PRIOR_ADJ_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: loggedMBSFNMeasurements-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_LOGGED_MBSFN_MEAS_R12               = 0
}RRC_LOGGED_MBSFN_MEAS_R12_ENUM;
typedef VOS_UINT8                 RRC_LOGGED_MBSFN_MEAS_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: wlan-IW-RAN-Rules-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_WLAN_IW_RAN_RULES_R12               = 0
}RRC_WLAN_IW_RAN_RULES_R12_ENUM;
typedef VOS_UINT8                 RRC_WLAN_IW_RAN_RULES_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: wlan-IW-ANDSF-Policies-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_WLAN_IW_ANDSF_POLICIES_R12          = 0
}RRC_WLAN_IW_ANDSF_POLICIES_R12_ENUM;
typedef VOS_UINT8                 RRC_WLAN_IW_ANDSF_POLICIES_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: timerT312-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_TIMER_T312_R12                      = 0
}RRC_TIMER_T312_R12_ENUM;
typedef VOS_UINT8                 RRC_TIMER_T312_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: alternativeTimeToTrigger-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_ALTERNATIVE_TIME_TO_TRIG_R12        = 0
}RRC_ALTERNATIVE_TIME_TO_TRIG_R12_ENUM;
typedef VOS_UINT8                 RRC_ALTERNATIVE_TIME_TO_TRIG_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: incMonEUTRA-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_INC_MON_EUTRA_R12                   = 0
}RRC_INC_MON_EUTRA_R12_ENUM;
typedef VOS_UINT8                 RRC_INC_MON_EUTRA_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: incMonUTRA-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_INC_MON_UTRA_R12                    = 0
}RRC_INC_MON_UTRA_R12_ENUM;
typedef VOS_UINT8                 RRC_INC_MON_UTRA_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: extendedMaxMeasId-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_EXTENDED_MAX_MEAS_ID_R12            = 0
}RRC_EXTENDED_MAX_MEAS_ID_R12_ENUM;
typedef VOS_UINT8                 RRC_EXTENDED_MAX_MEAS_ID_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: extendedRSRQ-LowerRange-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_EXTENDED_RSRQ_LOWER_RANGE_R12       = 0
}RRC_EXTENDED_RSRQ_LOWER_RANGE_R12_ENUM;
typedef VOS_UINT8                 RRC_EXTENDED_RSRQ_LOWER_RANGE_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: rsrqOnAllSymbols-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_RSRQ_ON_ALL_SYMBOLS_R12             = 0
}RRC_RSRQ_ON_ALL_SYMBOLS_R12_ENUM;
typedef VOS_UINT8                 RRC_RSRQ_ON_ALL_SYMBOLS_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: crs-DiscoverySignalsMeas-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_CRS_DISCOVERY_SIGNALS_MEAS_R12      = 0
}RRC_CRS_DISCOVERY_SIGNALS_MEAS_R12_ENUM;
typedef VOS_UINT8                 RRC_CRS_DISCOVERY_SIGNALS_MEAS_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: csi-RS-DiscoverySignalsMeas-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_CSI_RS_DISCOVERY_SIGNALS_MEAS_R12   = 0
}RRC_CSI_RS_DISCOVERY_SIGNALS_MEAS_R12_ENUM;
typedef VOS_UINT8                 RRC_CSI_RS_DISCOVERY_SIGNALS_MEAS_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: drb-TypeSplit-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_DRB_TYPE_SPLIT_R12                  = 0
}RRC_DRB_TYPE_SPLIT_R12_ENUM;
typedef VOS_UINT8                 RRC_DRB_TYPE_SPLIT_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: drb-TypeSCG-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_DRB_TYPE_SCG_R12                    = 0
}RRC_DRB_TYPE_SCG_R12_ENUM;
typedef VOS_UINT8                 RRC_DRB_TYPE_SCG_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: mbms-AsyncDC-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_MBMS_ASYNC_DC_R12                   = 0
}RRC_MBMS_ASYNC_DC_R12_ENUM;
typedef VOS_UINT8                 RRC_MBMS_ASYNC_DC_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: logicalChannelSR-ProhibitTimer-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_LOGIC_CH_SR_PROHB_TIMER_R12         = 0
}RRC_LOGIC_CH_SR_PROHB_TIMER_R12_ENUM;
typedef VOS_UINT8                 RRC_LOGIC_CH_SR_PROHB_TIMER_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: longDRX-Command-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_LONG_DRX_CMD_R12                    = 0
}RRC_LONG_DRX_CMD_R12_ENUM;
typedef VOS_UINT8                 RRC_LONG_DRX_CMD_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: commSimultaneousTx-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_COMM_SIMUL_TX_R12                   = 0
}RRC_COMM_SIMUL_TX_R12_ENUM;
typedef VOS_UINT8                 RRC_COMM_SIMUL_TX_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: support-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_SUPPORT_R12                         = 0
}RRC_SUPPORT_R12_ENUM;
typedef VOS_UINT8                 RRC_SUPPORT_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: discScheduledResourceAlloc-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_DISC_SCHEDULED_RSRC_ALLOC_R12       = 0
}RRC_DISC_SCHEDULED_RSRC_ALLOC_R12_ENUM;
typedef VOS_UINT8                 RRC_DISC_SCHEDULED_RSRC_ALLOC_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: disc-UE-SelectedResourceAlloc-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_DISC_UE_SELECTED_RSRC_ALLOC_R12     = 0
}RRC_DISC_UE_SELECTED_RSRC_ALLOC_R12_ENUM;
typedef VOS_UINT8                 RRC_DISC_UE_SELECTED_RSRC_ALLOC_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: disc-SLSS-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_SUPP_DISC_SLSS_R12                       = 0
}RRC_DISC_SLSS_R12_ENUM;
typedef VOS_UINT8                 RRC_DISC_SLSS_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: discSupportedProc-r12
* 取值范围: ENUM(0..1)
*********************************************************************/
typedef enum
{
    RRC_N50_DISC_SUPP_PROC_R12                   = 0,
    RRC_N400_DISC_SUPP_PROC_R12                  = 1
}RRC_DISC_SUPP_PROC_R12_ENUM;
typedef VOS_UINT8                 RRC_DISC_SUPP_PROC_R12_ENUM_UINT8;

/* end: add for c50 ue cap,   */

/*****************************************************************************
   5 STRUCT
*****************************************************************************/
/*****************************************************************************
 结构名    : RRC_NV_CSEL_CAND_FREQ_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 关于某个候补频率的信息
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usFreqInfo;                             /* 频率频点 */
    VOS_UINT16                          usCellCount;                            /* 频率中的小区ID */
    VOS_UINT16                          ausCellIds[RRC_NV_CSEL_MAX_STORED_CELL_COUNT_PER_FREQ]; /* 邻小区列表 */
} RRC_NV_CSEL_CAND_FREQ_STRU;

/*****************************************************************************
 结构名    : RRC_NV_CSEL_FREQ_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 单个频带信息
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usFreqBegin;                            /* 起始频率 */
    VOS_UINT16                          usFreqEnd;                              /* 结束频率 */
} RRC_NV_CSEL_FREQ_BAND_INFO_STRU;
/* reportCGI stub begin */
/*****************************************************************************
 结构名    : RRC_UU_MCC_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;    /*(0, 3)*/
    VOS_UINT8                           aucItem[3];
}RRC_UU_MCC_STRU;

/*****************************************************************************
 结构名    : RRC_UU_MNC_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;    /*(0, 3)*/
    VOS_UINT8                           aucItem[3];
}RRC_UU_MNC_STRU;

/*****************************************************************************
 结构名    : RRC_UU_PLMN_ID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    RRC_UU_MCC_STRU                     Mcc;                          /* optional */
    RRC_UU_MNC_STRU                     Mnc;
}RRC_UU_PLMN_ID_STRU;

/*****************************************************************************
 结构名    : RRC_UU_CGI_EUTRA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    RRC_UU_PLMN_ID_STRU                 PlmnId;
    VOS_UINT32                          ulCellId;
}RRC_UU_CGI_EUTRA_STRU;

/*****************************************************************************
 结构名    : RRC_UU_PLMN_ID_LIST2_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 小区CGI信息结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;    /*(0, 1..RRC_UU_MAX_NUM_OF_PLMN_ID_LIST_2)*/
    RRC_UU_PLMN_ID_STRU                 aItem[RRC_UU_MAX_NUM_OF_PLMN_ID_LIST_2];
}RRC_UU_PLMN_ID_LIST2_STRU;

/*****************************************************************************
 结构名    : RRC_UU_CGI_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 小区CGI信息结构
*****************************************************************************/
typedef struct
{
    RRC_UU_CGI_EUTRA_STRU               CellGlobalId;
    VOS_UINT16                          usTrackingAreaCode;
    RRC_UU_PLMN_ID_LIST2_STRU           PlmnIdList;                   /* optional */
}RRC_UU_CGI_INFO_STRU;

/*****************************************************************************
 结构名    : RRC_CMM_CGI_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 小区CGI信息结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usPhyCellId;
    VOS_UINT16                          usFreq;
    RRC_UU_CGI_INFO_STRU                stCGI;
}RRC_NV_CGI_INFO_STRU;

/*****************************************************************************
 结构名    : RRC_CMM_USER_SET_CGI_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 用户手动设置的小区CGI信息列表结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;
    VOS_UINT8                           aucReserved[2];
    RRC_NV_CGI_INFO_STRU                astCgiInfo[RRC_NV_USER_SET_CGI_MAX_NUM];
}RRC_NV_USER_SET_CGI_LIST_STRU;

/*****************************************************************************
 结构名    : RRC_CMM_AUTO_SRCH_CGI_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE自动搜索得到的小区CGI信息列表结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;
    VOS_UINT8                           aucReserved[2];
    RRC_NV_CGI_INFO_STRU                astCgiInfo[RRC_NV_AUTO_SRCH_CGI_MAX_NUM];
}RRC_NV_AUTO_SRCH_CGI_LIST_STRU;

/*****************************************************************************
 结构名    : RRC_CMM_CELL_CGI_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 小区CGI信息列表结构
*****************************************************************************/
typedef struct
{
    RRC_NV_USER_SET_CGI_LIST_STRU      stUserSetCgiList;                       /* 用户手动设置的CGI信息列表 */
    RRC_NV_AUTO_SRCH_CGI_LIST_STRU     stAutoSrchCgiList;                      /* UE自动获取的CGI信息列表 */
}RRC_NV_CELL_CGI_LIST_STRU;
/* reportCGI stub end */

/*****************************************************************************
 结构名    : RRC_NV_SIB_SUPPORT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE支持的系统信息表结构
*****************************************************************************/
typedef struct
{
    /* MBMS　begin */
    VOS_UINT8   aucSibn[32];
    /* MBMS　end */
}RRC_NV_SIB_SUPPORT_STRU;

/* UE能力结构定义 */
/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_ROH_CPROFILES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中PDCP->ROHC参数
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8                  enProfile0x0001;
    PS_BOOL_ENUM_UINT8                  enProfile0x0002;
    PS_BOOL_ENUM_UINT8                  enProfile0x0003;
    PS_BOOL_ENUM_UINT8                  enProfile0x0004;
    PS_BOOL_ENUM_UINT8                  enProfile0x0006;
    PS_BOOL_ENUM_UINT8                  enProfile0x0101;
    PS_BOOL_ENUM_UINT8                  enProfile0x0102;
    PS_BOOL_ENUM_UINT8                  enProfile0x0103;
    PS_BOOL_ENUM_UINT8                  enProfile0x0104;
    VOS_UINT8                           aucReserved[3];
}RRC_UE_CAP_SUPP_ROH_CPROFILES_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_PDCP_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中PDCP参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[3];
    RRC_UE_CAP_ROH_CONTEXT_ENUM_UINT8   enMaxNumRohcContextSessions;
    RRC_UE_CAP_SUPP_ROH_CPROFILES_STRU  stPdcpSuppRohCprofiles;
} RRC_UE_CAP_PDCP_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_PDCP_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中PHY参数
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8                  enUeTxAntennaSelectionSupp;
    PS_BOOL_ENUM_UINT8                  enUeSpecRefSigsSupp;
    VOS_UINT8                           aucReserved[2];
} RRC_UE_CAP_PHY_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_EUTRA_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中RF->Band参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucEutraBand;
    PS_BOOL_ENUM_UINT8                  enHalfDuplex;
    VOS_UINT8                           aucReserved[2];
} RRC_UE_CAP_SUPP_EUTRA_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_RF_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中RF参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[2];
    VOS_UINT16                          usCnt;
    RRC_UE_CAP_SUPP_EUTRA_BAND_STRU     astSuppEutraBandList[RRC_MAX_NUM_OF_BANDS];
} RRC_UE_CAP_RF_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_INTER_FREQ_EUTRA_BAND_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中MEAS->EutraInterFreqBand参数
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8                  enInterFreqNeedForGaps;
    VOS_UINT8                           aucReserved[3];
} RRC_INTER_FREQ_EUTRA_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : RRC_INTER_FREQ_EUTRA_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中MEAS->InterFreqBand参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[2];
    VOS_UINT16                          usCnt;
    RRC_INTER_FREQ_EUTRA_BAND_LIST_STRU astEutraInterFreqBandList[RRC_MAX_NUM_OF_BANDS];
} RRC_INTER_FREQ_EUTRA_BAND_INFO_STRU;

/*****************************************************************************
 结构名    : RRC_INTER_RAT_BAND_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中MEAS->InterRatBand参数
*****************************************************************************/
typedef struct
{
    PS_BOOL_ENUM_UINT8                  enInterRatNeedForGaps;
    VOS_UINT8                           aucReserved[3];
} RRC_INTER_RAT_BAND_LIST_STRU;

/*****************************************************************************
 结构名    : RRC_INTER_RAT_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中MEAS->InterRatBand参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[2];
    VOS_UINT16                          usCnt;
    RRC_INTER_RAT_BAND_LIST_STRU        astInterRatBandList[RRC_MAX_NUM_OF_BANDS];
} RRC_INTER_RAT_BAND_INFO_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_MEAS_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中MEAS参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[2];
    VOS_UINT16                          usCnt;
    VOS_UINT32                          aulUeCapMeasIdx[RRC_MAX_NUM_OF_BANDS];
} RRC_UE_CAP_MEAS_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_UTRA_FDD_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedUTRA-FDD-BandListItem
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_UTRA_FDD_BAND_ENUM_UINT8 enUtraFddBand;
    VOS_UINT8                           aucReserved[3];
} RRC_UE_CAP_SUPP_UTRA_FDD_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_UTRA_FDD_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中INTER RAT->UTRA FDD参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[2];
    VOS_UINT16                          usCnt;
    RRC_UE_CAP_SUPP_UTRA_FDD_BAND_STRU  astSuppUtraFddBandList[RRC_MAX_NUM_OF_BANDS];
} RRC_IRAT_UTRA_FDD_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_UTRA_TDD128_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedUTRA-TDD128BandListItem
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_UTRA_TDD128_BAND_ENUM_UINT8        enUtraTdd128Band;
    VOS_UINT8                                     aucReserved[3];
} RRC_UE_CAP_SUPP_UTRA_TDD128_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_UTRA_TDD128_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedUTRA-TDD128BandList
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                     aucReserved[2];
    VOS_UINT16                                    usCnt;
    RRC_UE_CAP_SUPP_UTRA_TDD128_BAND_STRU         astSuppUtraTdd128BandList[RRC_MAX_NUM_OF_BANDS];
} RRC_IRAT_UTRA_TDD128_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_UTRA_TDD384_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedUTRA-TDD384BandListItem
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_UTRA_TDD384_BAND_ENUM_UINT8        enUtraTdd384Band;
    VOS_UINT8                                     aucReserved[3];
} RRC_UE_CAP_SUPP_UTRA_TDD384_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_UTRA_TDD384_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedUTRA-TDD384BandList
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                     aucReserved[2];
    VOS_UINT16                                    usCnt;
    RRC_UE_CAP_SUPP_UTRA_TDD384_BAND_STRU         astSuppUtraTdd384BandList[RRC_MAX_NUM_OF_BANDS];
} RRC_IRAT_UTRA_TDD384_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_UTRA_TDD768_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedUTRA-TDD768BandListItem
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_UTRA_TDD768_BAND_ENUM_UINT8        enUtraTdd768Band;
    VOS_UINT8                                     aucReserved[3];
} RRC_UE_CAP_SUPP_UTRA_TDD768_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_UTRA_TDD768_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedUTRA-TDD768BandList
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                     aucReserved[2];
    VOS_UINT16                                    usCnt;
    RRC_UE_CAP_SUPP_UTRA_TDD768_BAND_STRU         astSuppUtraTdd768BandList[RRC_MAX_NUM_OF_BANDS];
} RRC_IRAT_UTRA_TDD768_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_GERAN_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedGERAN-BandListItem
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_GERAN_BAND_ENUM_UINT8    enGeranBand;
    VOS_UINT8                           aucReserved[3];
} RRC_UE_CAP_SUPP_GERAN_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_GERAN_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-GERAN-Parameters
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucReserved;
    PS_BOOL_ENUM_UINT8                  enInterRatPsHoToGeran;
    VOS_UINT16                          usCnt;
    RRC_UE_CAP_SUPP_GERAN_BAND_STRU     astSuppGeranBandList[RRC_MAX_NUM_OF_BANDS];
} RRC_IRAT_GERAN_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_HRPD_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedHRPD-BandList
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucReserved[2];
    VOS_UINT16                          usCnt;
    RRC_UE_CAP_CDMA2000_BANDCLASS_ENUM_UINT8      aenCdma2000HrpdBand[RRC_MAX_NUM_OF_CDMA_BAND_CLASS];
} RRC_UE_CAP_SUPP_HRPD_BAND_INFO_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_CDMA2000_HRPD_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-CDMA2000-HRPD-Parameters
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                     aucReserved[2];
    RRC_UE_CAP_CDMA2000_HRPD_TX_CONFIG_ENUM_UINT8 enCdma2000HrpdTxCfg;
    RRC_UE_CAP_CDMA2000_HRPD_RX_CONFIG_ENUM_UINT8 enCdma2000HrpdRxCfg;
    RRC_UE_CAP_SUPP_HRPD_BAND_INFO_STRU           stSuppHrpdBandList;
} RRC_IRAT_CDMA2000_HRPD_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_SUPP_1X_RTT_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : Supported1xRTT-BandList
*****************************************************************************/
typedef struct
{
    VOS_UINT8                             aucReserved[2];
    VOS_UINT16                            usCnt;
    RRC_UE_CAP_CDMA2000_BANDCLASS_ENUM_UINT8      aenCdma20001xRttBand[RRC_MAX_NUM_OF_CDMA_BAND_CLASS];
} RRC_UE_CAP_SUPP_1X_RTT_BAND_INFO_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_CDMA2000_1X_RTT_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-CDMA2000-1xRTT-Parameters
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                           aucReserved[2];
    RRC_UE_CAP_CDMA2000_1X_RTT_TX_CONFIG_ENUM_UINT8     enCdma20001xRttTxCfg;
    RRC_UE_CAP_CDMA2000_1X_RTT_RX_CONFIG_ENUM_UINT8     enCdma20001xRttRxCfg;
    RRC_UE_CAP_SUPP_1X_RTT_BAND_INFO_STRU               stSupp1xRttBandList;
} RRC_IRAT_CDMA2000_1X_RTT_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_INTER_RAT_PARA_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中INTER RAT参数
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitCdma2000HrpdPresent     : 1;
    VOS_UINT32                          bitCdma20001xRttPresent    : 1;
    VOS_UINT32                          bitSpare                   : 30;

    RRC_IRAT_UTRA_FDD_PARA_STRU         stUtraFdd;
    RRC_IRAT_UTRA_TDD128_PARA_STRU      stUtraTdd128;
    RRC_IRAT_UTRA_TDD384_PARA_STRU      stUtraTdd384;
    RRC_IRAT_UTRA_TDD768_PARA_STRU      stUtraTdd768;
    RRC_IRAT_GERAN_PARA_STRU            stGeran;
    RRC_IRAT_CDMA2000_HRPD_PARA_STRU    stCdma2000Hrpd;
    RRC_IRAT_CDMA2000_1X_RTT_PARA_STRU  stCdma20001xRtt;
} RRC_UE_CAP_INTER_RAT_PARA_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_FEATURE_GROUP_IND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中特性组指示，aucFeatureInd[]: 1－支持，0－不支持，
             数组元素0指示Group1,元素1指示Group2,依次类推

*****************************************************************************/
typedef struct
{
    VOS_UINT8                     aucFeatureInd[RRC_MAX_NUM_OF_FEATURE_GROUP];
} RRC_UE_CAP_FEATURE_GROUP_IND_STRU;

/* v7r2 LLD begin */
/*****************************************************************************
 结构名    : RRC_UE_CAP_FEATURE_GROUP_IND_R9ADD_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中特性组指示，aucFeatureInd[]: 1－支持，0－不支持，
             数组元素0指示Group33,元素1指示Group64,依次类推

*****************************************************************************/
typedef struct
{
    VOS_UINT8                     aucFeatureInd[RRC_MAX_NUM_OF_FEATURE_GROUP];
} RRC_UE_CAP_FEATURE_GROUP_IND_R9ADD_STRU;
/* v7r2 LLD end */

/*****************************************************************************
 结构名    : RRC_PHY_LAYER_PARAM_V920_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : PhyLayerParameters-v920
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enEnhncDualLayerFddR9;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enEnhncDualLayerTddR9;
    VOS_UINT8                                     aucReserved[2];
}RRC_PHY_LAYER_PARAM_V920_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_PARAM_GERAN_V920_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-ParametersGERAN-v920
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enDtmR9;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enEReDirGeranR9;
    VOS_UINT8                                     aucReserved[2];
}RRC_IRAT_PARAM_GERAN_V920_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_PARAM_UTRA_V920_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-ParametersUTRA-v920
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enEReDirUtraR9;
    VOS_UINT8                                     aucReserved[3];
}RRC_IRAT_PARAM_UTRA_V920_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_PARAM_CDMA2000_1XRTT_V920_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-ParametersCDMA2000-1XRTT-v920
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enECsfb1xrttR9;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enECsfbConcPsMob1XrttR9;
    VOS_UINT8                                     aucReserved[2];
}RRC_IRAT_PARAM_CDMA2000_1XRTT_V920_STRU;

/*****************************************************************************
 结构名    : RRC_CSG_PROXIMITY_IND_PARAM_R9_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : CSG-ProximityIndicationParameters-r9
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enIntraFreqProximityIndR9;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enInterFreqProximityIndR9;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enUtranProximityIndR9;
    VOS_UINT8                                     ucReserved;
}RRC_CSG_PROXIMITY_IND_PARAM_R9_STRU;

/*****************************************************************************
 结构名    : RRC_NEIGH_CELL_SI_ACQ_PARAM_R9_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : NeighCellSI-AcquisitionParameters-r9
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enIntraFreqSiAcqForHoR9;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enInterFreqSiAcqForHoR9;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enUtranSiAcqForHoR9;
    VOS_UINT8                                     ucReserved;
}RRC_NEIGH_CELL_SI_ACQ_PARAM_R9_STRU;

/*****************************************************************************
 结构名    : RRC_SON_PARAM_R9_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SON-Parameters-r9
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8    enRachRprtR9;
    VOS_UINT8                                     aucReserved[3];
}RRC_SON_PARAM_R9_STRU;

/* v7r2 LLD begin */
/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_ADD_XDD_MODE_R9_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-CapabilityAddXDD-Mode-r9
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitPhyLayerParamR9Present                   : 1;
    VOS_UINT32    bitFeatureGroupIndicatorsR9Present          : 1;
    VOS_UINT32    bitFeatureGroupIndRel9AddR9Present          : 1;
    VOS_UINT32    bitInterRatParamGeranR9Present              : 1;
    VOS_UINT32    bitInterRatParamUtraR9Present               : 1;
    VOS_UINT32    bitInterRatParamCdma2000R9Present           : 1;
    VOS_UINT32    bitNeighCellSiAcqParamR9Present             : 1;
    VOS_UINT32    bitSpare                                    : 25;

    RRC_UE_CAP_PHY_PARA_STRU                    stPhyLayerParamR9;              /* optional */
    RRC_UE_CAP_FEATURE_GROUP_IND_STRU           stFeatureGroupIndicatorsR9;     /* optional */
    RRC_UE_CAP_FEATURE_GROUP_IND_R9ADD_STRU     stFeatureGroupIndRel9AddR9;     /* optional */
    RRC_IRAT_GERAN_PARA_STRU                    stInterRatParamGeranR9;         /* optional */
    RRC_IRAT_PARAM_UTRA_V920_STRU               stInterRatParamUtraR9;          /* optional */
    RRC_IRAT_PARAM_CDMA2000_1XRTT_V920_STRU     stInterRatParamCdma2000R9;      /* optional */
    RRC_NEIGH_CELL_SI_ACQ_PARAM_R9_STRU         stNeighCellSiAcqParamR9;        /* optional */
}RRC_UE_EUTRA_CAP_ADD_XDD_MODE_R9_STRU;

/*begin: add for feature v700r500*/
typedef VOS_UINT16           RRC_FREQ_BAND_IND_V9E0_UINT16;

/*****************************************************************************
 结构名    : RRC_OTDOA_POSITION_CAP_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : OTDOA-PositioningCapbilities-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitOtdoaUeAssistedR10Present           : 1;
    VOS_UINT32    bitInterFreqRstdMeasR10Present         : 1;
    VOS_UINT32    bitSpare                                 : 30;

    VOS_UINT8                                   aucReserved[2];
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enOtdoaUeAssistedR10;
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enInterFreqRstdMeasR10;         /* optional */
}RRC_OTDOA_POSITION_CAP_R10_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V10C0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v10c0-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitOtdoaPositionCapR10Present               : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_OTDOA_POSITION_CAP_R10_STRU       stOtdoaPositionCapR10;    /* optional */
}RRC_UE_EUTRA_CAP_V10C0_IES_STRU;

/*****************************************************************************
 结构名    : RRC_RF_PARAM_V10F0_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC_RF_PARAM_V10F0_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32     bitModifiedMprBehaviorR10Present           : 1;
    VOS_UINT32     bitSpare                                   : 31;

    VOS_UINT32     ulModifiedMprBehaviorR10;      /* optional */
}RRC_RF_PARAM_V10F0_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V10C0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v10c0-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32             bitRfParamV10f0Present                      : 1;
    VOS_UINT32             bitNonCritiExtPresent                       : 1;
    VOS_UINT32             bitSpare                                   : 30;

    RRC_RF_PARAM_V10F0_STRU  stRfParamV10f0;                 /* optional */
}RRC_UE_EUTRA_CAP_V10F0_IES_STRU;

/* Begin: 2016/1/4 cr develop */
/*********************************************************************
* ASN 文本: fourLayerTM3-TM4-r10
* 取值范围: ENUM(0..0)
*********************************************************************/
typedef enum
{
    RRC_UE_EUTRA_CAP_SUPP_FOUR_LAYER_TM3_TM4_R10              = 0
}RRC_UE_EUTRA_CAP_FOUR_LAYER_TM3_TM4_R10_ENUM;
typedef VOS_UINT8                 RRC_UE_EUTRA_CAP_TM3_TM4_R10_ENUM_UINT8;

/*********************************************************************
* ASN 文本 :CA-MIMO-ParametersDL-v10i0
*********************************************************************/
typedef struct
{
    VOS_UINT8    bitFourLayerTm3Tm4R10Present                : 1;
    VOS_UINT8    bitSpare                                    : 7;

    RRC_UE_EUTRA_CAP_TM3_TM4_R10_ENUM_UINT8    enFourLayerTm3Tm4R10;     /* optional */
    VOS_UINT8                                  aucReserved[2];
}RRC_UE_EUTRA_CAP_CA_MIMO_PARAM_DL_V10I0_STRU;

typedef struct
{
    VOS_UINT32                                        ucCnt;    /*(0, 1..UU_MAX_NUM_OF_BANDWIDTH_CLASS_R10)*/
    RRC_UE_EUTRA_CAP_CA_MIMO_PARAM_DL_V10I0_STRU      aItem[RRC_UU_MAX_NUM_OF_BANDWIDTH_CLASS_R10];
}RRC_UE_EUTRA_CAP_BAND_PARAM_DL_V10I0_STRU;

/*********************************************************************
* ASN 文本 :BandParameters-v10i0
*********************************************************************/
typedef struct
{
    RRC_UE_EUTRA_CAP_BAND_PARAM_DL_V10I0_STRU         BandParamDlV10i0;
}RRC_UE_EUTRA_CAP_BAND_PARAM_V10I0_STRU;

typedef struct
{
    VOS_UINT32                                 ucCnt;    /*(0, 1..UU_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10)*/
    RRC_UE_EUTRA_CAP_BAND_PARAM_V10I0_STRU    aItem[RRC_UU_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10];
}RRC_UE_EUTRA_CAP_BAND_PARAM_LIST_V10I0_STRU;

/*********************************************************************
* ASN 文本 :BandCombinationParameters-v10i0
*********************************************************************/
typedef struct
{
    RRC_UE_EUTRA_CAP_BAND_PARAM_LIST_V10I0_STRU       BandParamListV10i0;           /* optional */
}RRC_UE_EUTRA_CAP_BAND_COMB_PARAM_V10I0_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandCombination-v10i0
*********************************************************************/
typedef struct
{
    VOS_UINT32                                        ucCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R10)*/
    RRC_UE_EUTRA_CAP_BAND_COMB_PARAM_V10I0_STRU       aItem[RRC_UU_MAX_NUM_OF_BAND_COMB_R10];
}RRC_UE_EUTRA_CAP_SUPP_BAND_COMB_V10I0_STRU;

/*********************************************************************
* ASN 文本 :RF-Parameters-v10i0
*********************************************************************/
typedef struct
{

    RRC_UE_EUTRA_CAP_SUPP_BAND_COMB_V10I0_STRU        SuppBandCombV10i0;            /* optional */
}RRC_UE_EUTRA_RF_CAP_V10I0_IES_STRU;

typedef struct
{
    VOS_UINT32                                      bitRfParamV10f0Present                      : 1;
    VOS_UINT32                                      bitSpare                                    : 31;
    RRC_UE_EUTRA_RF_CAP_V10I0_IES_STRU              RfParamV10i0;                 /* optional */
}RRC_UE_EUTRA_CAP_V10I0_IES_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandCombinationAdd-v11d0
*********************************************************************/
typedef struct
{
    VOS_UINT32                                       usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R11)*/
    RRC_UE_EUTRA_CAP_BAND_COMB_PARAM_V10I0_STRU      aItem[RRC_UU_MAX_NUM_OF_BAND_COMB_R11];
}RRC_UE_EUTRA_SUPP_BAND_COMB_ADD_V11D0_STRU;

/*********************************************************************
* ASN 文本 :RF-Parameters-v11d0
*********************************************************************/
typedef struct
{
    RRC_UE_EUTRA_SUPP_BAND_COMB_ADD_V11D0_STRU    SuppBandCombAddV11d0;         /* optional */
}RRC_UE_EUTRA_RF_PARAM_V11D0_STRU;
/*********************************************************************
* ASN 文本 :UE-EUTRA-Capability-v11d0-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32                          bitRfParamV11d0Present          : 1;
    VOS_UINT32                          bitSpare                        : 31;
    RRC_UE_EUTRA_RF_PARAM_V11D0_STRU    RfParamV11d0;                 /* optional */
}RRC_UE_EUTRA_CAP_V11D0_IES_STRU;

/* End: 2016/1/4 cr develop */

/*****************************************************************************
 结构名    : RRC_IRAT_PARAM_UTRA_V9H0_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-ParametersUTRA-v9h0
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitMfbiUtraR9Present           : 1;
    VOS_UINT32    bitSpare                         : 31;

    VOS_UINT8                                   aucReserved[3];
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enMfbiUtraR9;            /* optional */
}RRC_IRAT_PARAM_UTRA_V9H0_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V9H0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v9h0-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitInterRatParamUtraV9h0Present             : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_IRAT_PARAM_UTRA_V9H0_STRU       stInterRatParamUtraV9h0;    /* optional */

    /*R11 NV begin*/
    //RRC_UE_EUTRA_CAP_V10C0_IES_STRU              stUeEutraCapV10c0;             /* optional */
    /*R11 NV end*/
}RRC_UE_EUTRA_CAP_V9H0_IES_STRU;

/*********************************************************************
结构名    : RRC_SUPP_BAND_EUTRA_V9E0_STRU
协议表格  :
ASN.1描述 :
结构说明  : bandEUTRA-v9e0
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitBandEutraV9e0Present                    : 1;
    VOS_UINT32    bitSpare                                   : 31;

    VOS_UINT8                            ucRev[2];
    RRC_FREQ_BAND_IND_V9E0_UINT16        usBandEutraV9e0;             /* optional */
}RRC_SUPP_BAND_EUTRA_V9E0_STRU;

/*********************************************************************
结构名    : RRC_SUPP_BAND_LIST_EUTRA_V9E0_STRU
协议表格  :
ASN.1描述 :
结构说明  : SupportedBandListEutra-v9e0
*********************************************************************/
typedef struct
{
    VOS_UINT8                            ucRev[2];
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BANDS)*/
    RRC_SUPP_BAND_EUTRA_V9E0_STRU        aItem[RRC_MAX_NUM_OF_BANDS];
}RRC_SUPP_BAND_LIST_EUTRA_V9E0_STRU;

/*****************************************************************************
 结构名    : RRC_RF_PARAM_V9E0_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-Parameters-v9e0
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitSuppBandListEutraV9e0Present           : 1;
    VOS_UINT32    bitSpare                                    : 31;

    RRC_SUPP_BAND_LIST_EUTRA_V9E0_STRU        stSuppBandListEutraV9e0;   /* optional */
}RRC_RF_PARAM_V9E0_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V9E0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v9e0-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitRfParamV9e0Present                       : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_RF_PARAM_V9E0_STRU       stRfParamV9e0;    /* optional */
    /*R11 NV begin*/
    //RRC_UE_EUTRA_CAP_V9H0_IES_STRU              stUeEutraCapV9h0;             /* optional */
    /*R11 NV end*/
}RRC_UE_EUTRA_CAP_V9E0_IES_STRU;

/*****************************************************************************
 结构名    : RRC_PHY_LAYER_PARAM_V9D0_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : PhyLayerParameters-v9d0
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitTm5FddR9Present              : 1;
    VOS_UINT32    bitTm5TddR9Present              : 1;
    VOS_UINT32    bitSpare                          : 30;

    VOS_UINT8                                   aucReserved[2];
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enTm5FddR9;         /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enTm5TddR9;         /* optional */
}RRC_PHY_LAYER_PARAM_V9D0_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V9D0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v9d0-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitPhyLayerParamV9d0Present                 : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_PHY_LAYER_PARAM_V9D0_STRU       stPhyLayerParamV9d0;        /* optional */
    /*R11 NV begin*/
    //RRC_UE_EUTRA_CAP_V9E0_IES_STRU              stUeEutraCapV9e0;             /* optional */
    /*R11 NV end*/
}RRC_UE_EUTRA_CAP_V9D0_IES_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_PARAM_UTRA_V9C0_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-ParametersUTRA-v9c0
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitVoiceOverPsHsUtraFddR9Present                 : 1;
    VOS_UINT32    bitVoiceOverPsHsUtraTdd128R9Present              : 1;
    VOS_UINT32    bitSrvccFromUtraFddToUtraFddR9Present            : 1;
    VOS_UINT32    bitSrvccFromUtraFddToGeranR9Present              : 1;
    VOS_UINT32    bitSrvccFromUtraTdd128ToUtraTdd128R9Present      : 1;
    VOS_UINT32    bitSrvccFromUtraTdd128ToGeranR9Present           : 1;
    VOS_UINT32    bitSpare                                           : 26;

    VOS_UINT8                                   aucReserved[2];
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enVoiceOverPsHsUtraFddR9;            /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enVoiceOverPsHsUtraTdd128R9;         /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enSrvccFromUtraFddToUtraFddR9;       /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enSrvccFromUtraFddToGeranR9;         /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enSrvccFromUtraTdd128ToUtraTdd128R9; /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enSrvccFromUtraTdd128ToGeranR9;      /* optional */
}RRC_IRAT_PARAM_UTRA_V9C0_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V9C0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v9c0-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitInterRatParamUtraV9c0Present             : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_IRAT_PARAM_UTRA_V9C0_STRU       stInterRatParamUtraV9c0;    /* optional */
    /*R11 NV begin*/
    //RRC_UE_EUTRA_CAP_V9D0_IES_STRU              stUeEutraCapV9d0;             /* optional */
    /*R11 NV end*/
}RRC_UE_EUTRA_CAP_V9C0_IES_STRU;

/*end: add for feature v700r500*/

/*begin: modify for feature v700r500*/
/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V9A0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v9a0-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitFeatureGroupIndRel9AddR9Present          : 1;
    VOS_UINT32    bitFddAddUeEutraCapabilitiesR9Present       : 1;
    VOS_UINT32    bitTddAddUeEutraCapabilitiesR9Present       : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 28;

    RRC_UE_CAP_FEATURE_GROUP_IND_R9ADD_STRU     stFeatureGroupIndRel9AddR9;     /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_R9_STRU       stFddAddUeEutraCapabilitiesR9;  /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_R9_STRU       stTddAddUeEutraCapabilitiesR9;  /* optional */
}RRC_UE_EUTRA_CAP_V9A0_IES_STRU;
/*end: modify for feature v700r500*/

/*****************************************************************************
 结构名    : RRC_NON_CONTI_UL_RA_WITHIN_CC_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : NonContiguousUL-RA-WithinCC-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                   aucReserved[3];
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enNonContiguousUlRaWithinCcInfoR10;    /* optional */
}RRC_NON_CONTI_UL_RA_WITHIN_CC_R10_STRU;

/*****************************************************************************
 结构名    : RRC_NON_CONT_UL_RA_WITHIN_CC_LIST_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : NonContiguousUL-RA-WithinCC-List-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                  usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BANDS)*/
    VOS_UINT8                                   aucReserved[2];
    RRC_NON_CONTI_UL_RA_WITHIN_CC_R10_STRU      astItem[RRC_MAX_NUM_OF_BANDS];
}RRC_NON_CONT_UL_RA_WITHIN_CC_LIST_R10_STRU;

/*****************************************************************************
 结构名    : RRC_PHY_LAYER_PARAM_V1020_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : PhyLayerParameters-v1020
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                       aucReserved[2];

    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8      enTwoAntennaPortsForPucchR10;         /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8      enTm9With8txFddR10;         /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8      enPmiDisablingR10;          /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8      enCrossCarrierSchdlR10;     /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8      enSimulPucchPuschR10;       /* optional */
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8      enMultiClusterPuschWithinCcR10;        /* optional */
    RRC_NON_CONT_UL_RA_WITHIN_CC_LIST_R10_STRU      stNonContiguousUlRaWithinCcListR10;    /* optional */
}RRC_PHY_LAYER_PARAM_V1020_STRU;

/*****************************************************************************
 结构名    : RRC_CA_MIMO_PARAM_UL_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : CA-MIMO-ParametersUL-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitSuppMimoCapUlR10Present                  : 1;
    VOS_UINT32    bitSpare                                    : 31;
    VOS_UINT8     aucReserved[2];

    RRC_CA_BANDWIDTH_CLASS_R10_ENUM_UINT8       enCaBwClassUlR10;
    RRC_MIMO_CAP_UL_R10_ENUM_UINT8              enSuppMimoCapUlR10;             /* optional */
}RRC_CA_MIMO_PARAM_UL_R10_STRU;

/*****************************************************************************
 结构名    : RRC_CA_MIMO_PARAM_DL_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : CA-MIMO-ParametersDL-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitSuppMimoCapDlR10Present                  : 1;
    VOS_UINT32    bitSpare                                    : 31;
    VOS_UINT8     aucReserved[2];

    RRC_CA_BANDWIDTH_CLASS_R10_ENUM_UINT8       enCaBwClassDlR10;
    RRC_MIMO_CAP_DL_R10_ENUM_UINT8              enSuppMimoCapDlR10;           /* optional */
}RRC_CA_MIMO_PARAM_DL_R10_STRU;

/*****************************************************************************
 结构名    : RRC_BAND_PARAM_UL_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandParametersUL-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                  usCnt;    /*(0, 1..UU_MAX_NUM_OF_BANDWIDTH_CLASS_R10)*/
    VOS_UINT8                                   aucReserved[2];
    RRC_CA_MIMO_PARAM_UL_R10_STRU               astItem[RRC_MAX_NUM_OF_BANDWIDTH_CLASS_R10];
}RRC_BAND_PARAM_UL_R10_STRU;

/*****************************************************************************
 结构名    : RRC_BAND_PARAM_DL_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandParametersDL-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                  usCnt;    /*(0, 1..UU_MAX_NUM_OF_BANDWIDTH_CLASS_R10)*/
    VOS_UINT8                                   aucReserved[2];
    RRC_CA_MIMO_PARAM_DL_R10_STRU               astItem[RRC_MAX_NUM_OF_BANDWIDTH_CLASS_R10];
}RRC_BAND_PARAM_DL_R10_STRU;

/*****************************************************************************
 结构名    : RRC_BAND_PARAM_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandParameters-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                   aucReserved[3];
    VOS_UINT8                                   ucBandEutraR10;                 /*(1..64)*/
    RRC_BAND_PARAM_UL_R10_STRU                  stBandParamUlR10;               /* optional */
    RRC_BAND_PARAM_DL_R10_STRU                  stBandParamDlR10;               /* optional */
}RRC_BAND_PARAM_R10_STRU;

/*****************************************************************************
 结构名    : RRC_BAND_COMB_PARAM_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandCombinationParameters-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                  usCnt;    /*(0, 1..UU_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10)*/
    VOS_UINT8                                   aucReserved[2];
    RRC_BAND_PARAM_R10_STRU                     astItem[RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10];
}RRC_BAND_COMB_PARAM_R10_STRU;

/*****************************************************************************
 结构名    : RRC_SUPP_BAND_COMB_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedBandCombination-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                  usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R10)*/
    VOS_UINT8                                   aucReserved[2];
    RRC_BAND_COMB_PARAM_R10_STRU                astItem[RRC_MAX_NUM_OF_BAND_COMB_R10];
}RRC_SUPP_BAND_COMB_R10_STRU;

/*****************************************************************************
 结构名    : RRC_RF_PARAM_V1020_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-Parameters-v1020
*****************************************************************************/
typedef struct
{
   RRC_SUPP_BAND_COMB_R10_STRU                  stSuppBandCombR10;
}RRC_RF_PARAM_V1020_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_MEAS_EUTRA_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中MEAS->Band参数
*****************************************************************************/
typedef struct
{
    RRC_INTER_FREQ_EUTRA_BAND_INFO_STRU stInterFreqEutraBandInfo;
    RRC_INTER_RAT_BAND_INFO_STRU        stInterRatBandInfo;
}RRC_UE_CAP_MEAS_EUTRA_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_BAND_COMB_LIST_EUTRA_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandCombinationListEUTRA-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                  usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R10)*/
    VOS_UINT8                                   aucReserved[2];
    RRC_UE_CAP_MEAS_EUTRA_BAND_STRU             astItem[RRC_MAX_NUM_OF_BAND_COMB_R10];
}RRC_BAND_COMB_LIST_EUTRA_R10_STRU;

/*****************************************************************************
 结构名    : RRC_MEAS_PARAM_V1020_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MeasParameters-v1020
*****************************************************************************/
typedef struct
{
   RRC_BAND_COMB_LIST_EUTRA_R10_STRU            stBandCombListEutraR10;
}RRC_MEAS_PARAM_V1020_STRU;

/*****************************************************************************
 结构名    : RRC_UE_BASED_NETW_PERF_MEAS_PARAM_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-BasedNetwPerfMeasParameters-r10
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitLoggedMeasIdleR10Present                 : 1;
    VOS_UINT32    bitStandaloneGnssLocR10Present              : 1;
    VOS_UINT32    bitSpare                                    : 30;
}RRC_UE_BASED_NETW_PERF_MEAS_PARAM_R10_STRU;

/*****************************************************************************
 结构名    : RRC_IRAT_PARAM_UTRA_TDD_V1020_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IRAT-ParametersUTRA-TDD-v1020
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_FEATURE_SUPPORT_FLAG_ENUM_UINT8  enEReDirUtraTddR10;
}RRC_IRAT_PARAM_UTRA_TDD_V1020_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_FEATURE_GROUP_IND_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE EUTRA能力中特性组指示，aucFeatureIndR10[]: 1－支持，0－不支持，
             数组元素0指示Group1,元素1指示Group2,依次类推

*****************************************************************************/
typedef struct
{
    VOS_UINT8                     aucFeatureIndR10[RRC_MAX_NUM_OF_FEATURE_GROUP];
} RRC_UE_CAP_FEATURE_GROUP_IND_R10_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1060_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-CapabilityAddXDD-Mode-v1060
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitPhyLayerParamV1060Present                : 1;
    VOS_UINT32    bitFeatureGroupIndRel10V1060Present         : 1;
    VOS_UINT32    bitInterRatParamCdma2000V1060Present        : 1;
    VOS_UINT32    bitInterRatParamUtraTddV1060Present         : 1;
    VOS_UINT32    bitSpare                                    : 28;

    RRC_PHY_LAYER_PARAM_V1020_STRU              stPhyLayerParamV1060;           /* optional */
    RRC_UE_CAP_FEATURE_GROUP_IND_R10_STRU       stFeatureGroupIndRel10V1060;    /* optional */
}RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1060_STRU;

/*****************************************************************************
 结构名    : RRC_SUPP_BANDWIDTH_COMB_SET_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-Parameters-v1060
*****************************************************************************/
typedef struct
{
    VOS_UINT16          usBitCnt;  /* 1..(8 * RRC_MAX_NUM_OF_SUPPORT_BAND_WIDTH_COMB_SET_R10) 有效bit位*/
    VOS_UINT8           aucReserved[2];
    VOS_UINT8           aucItem[RRC_MAX_NUM_OF_SUPPORT_BAND_WIDTH_COMB_SET_R10];
}RRC_SUPP_BANDWIDTH_COMB_SET_R10_STRU;

/*****************************************************************************
 结构名    : RRC_BAND_COMB_PARAM_EXT_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-Parameters-v1060
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitSuppBwCombSetR10Present                  : 1;
    VOS_UINT32    bitSpare                                    : 31;

    RRC_SUPP_BANDWIDTH_COMB_SET_R10_STRU stSuppBwCombSetR10;             /* optional */
}RRC_BAND_COMB_PARAM_EXT_R10_STRU;

/*****************************************************************************
 结构名    : RRC_SUPP_BAND_COMB_EXT_R10_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-Parameters-v1060
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;    /*(0, 1..UU_MAX_NUM_OF_SUPPORT_BAND_COMB_EXT_R10)*/
    VOS_UINT8                           ucReserved[2];
    RRC_BAND_COMB_PARAM_EXT_R10_STRU    aItem[RRC_MAX_NUM_OF_SUPPORT_BAND_COMB_EXT_R10];
}RRC_SUPP_BAND_COMB_EXT_R10_STRU;

/*****************************************************************************
 结构名    : RRC_RF_PARAM_V1060_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-Parameters-v1060
*****************************************************************************/
typedef struct
{
    RRC_SUPP_BAND_COMB_EXT_R10_STRU      stSuppBandCombExtR10;
}RRC_RF_PARAM_V1060_STRU;

/*****************************************************************************
 结构名    : RRC_RF_MEAS_PARAM_V1020_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-MEAS-Parameters-v1020
*****************************************************************************/
typedef struct
{
    VOS_UINT16                          usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R10)*/
    VOS_UINT8                           ucReserved[2];
    VOS_UINT32                          aulBandCombMeasGapParaIdx[RRC_MAX_NUM_OF_BAND_COMB_R10];
}RRC_RF_MEAS_PARAM_V1020_STRU;

typedef struct
{
    RRC_BAND_COMB_PARAM_R10_STRU        stBandCombParamR10;
    RRC_UE_CAP_MEAS_EUTRA_BAND_STRU     stUeCapMeasEutraBand;
}RRC_BAND_COMB_MEAS_GAP_PARAM_STRU;

/*begin: add for feature v700r500 ,  */
typedef     VOS_UINT16      RRC_FREQ_BAND_IND_R11_UINT16;

/*****************************************************************************
 结构名    : RRC_BAND_PARAM_R11_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandParameters-r11
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitSuppCsiProcR11Present                    : 1;
    VOS_UINT32    bitSpare                                    : 31;

    VOS_UINT8                                          ucRev[1];
    RRC_SUPP_CSI_PROC_R11_BAND_PARAM_R11_ENUM_UINT8    enSuppCsiProcR11;    /* optional */
    RRC_FREQ_BAND_IND_R11_UINT16                       usBandEutraR11;
    RRC_BAND_PARAM_UL_R10_STRU                         BandParamUlR11;      /* optional */
    RRC_BAND_PARAM_DL_R10_STRU                         BandParamDlR11;      /* optional */
}RRC_BAND_PARAM_R11_STRU;

/*****************************************************************************
 结构名    : RRC_BAND_PARAM_LIST_R11_BAND_COMB_PARAM_R11_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  :
*****************************************************************************/
typedef struct
{
    VOS_UINT8                            ucRev[2];
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10)*/
    RRC_BAND_PARAM_R11_STRU              aItem[RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10];
}RRC_BAND_PARAM_LIST_R11_BAND_COMB_PARAM_R11_STRU;

typedef    RRC_UE_CAP_MEAS_EUTRA_BAND_STRU    RRC_BAND_INFO_EUTRA_STRU;
/*****************************************************************************
 结构名    : RRC_BAND_COMB_PARAM_R11_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandCombinationParameters-r11
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitSuppBwCombSetR11Present                  : 1;
    VOS_UINT32    bitMultipleTimingAdvanceR11Present          : 1;
    VOS_UINT32    bitSimulRxTxR11Present                      : 1;
    VOS_UINT32    bitSpare                                    : 29;

    RRC_BAND_PARAM_LIST_R11_BAND_COMB_PARAM_R11_STRU                  BandParamListR11;
    RRC_SUPP_BANDWIDTH_COMB_SET_R10_STRU                              SuppBwCombSetR11;             /* optional */
    RRC_MULTIPLE_TIMING_ADVANCE_R11_BAND_COMB_PARAM_R11_ENUM_UINT8    enMultipleTimingAdvanceR11;    /* optional */
    RRC_SIMUL_RX_TX_R11_BAND_COMB_PARAM_R11_ENUM_UINT8                enSimulRxTxR11;    /* optional */
    VOS_UINT8                                                         ucRev[2];
    RRC_BAND_INFO_EUTRA_STRU                                          BandInfoEutraR11;
}RRC_BAND_COMB_PARAM_R11_STRU;

/*****************************************************************************
 结构名    : RRC_SUPP_BAND_COMB_ADD_R11_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : supportedBandCombinationAdd-r11
*****************************************************************************/
typedef struct
{
    VOS_UINT8     ucRev[2];
    VOS_UINT16    usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BAND_COMB_R11)*/
    VOS_UINT32    aulBandCombPamamR11[RRC_MAX_NUM_OF_BAND_COMB_R11];   /*RRC_BAND_COMB_PARAM_R11_STRU*/
}RRC_SUPP_BAND_COMB_ADD_R11_STRU;

/*****************************************************************************
 结构名    : RRC_REPORTED_FREQ_BANDS_R11_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : requestedBands-r11
*****************************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_REPORTED_FREQ_BANDS_R11)*/
    VOS_UINT8                            ucRev[2];
    RRC_FREQ_BAND_IND_R11_UINT16         ausItem[RRC_MAX_REQ_BANDS_R11];
}RRC_REQ_BANDS_R11_STRU;


/*****************************************************************************
 结构名    : RRC_RF_PARAM_V1180_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : rf-Parameters-v1180
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitFreqBandRetrievalR11Present              : 1;
    VOS_UINT32    bitSpare                                    : 31;

    VOS_UINT8                                 ucRev[3];
    RRC_FREQ_BAND_RETRIEVAL_R11_ENUM_UINT8    enFreqBandRetrievalR11;       /* optional */
    RRC_REQ_BANDS_R11_STRU                    ReqedBandsR11;                /* optional */
    RRC_SUPP_BAND_COMB_ADD_R11_STRU           SuppBandCombAddR11;           /* optional */
    RRC_SUPP_BAND_COMB_ADD_R11_STRU           SuppBandCombAddR11Ext1;       /* optional */
}RRC_RF_PARAM_V1180_STRU;

/* begin:add for Boston Phase3  UE CAP,   */
/*****************************************************************************
 结构名    : RRC_RF_PARAM_V1180_EXT_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : rf-Parameters-v1180
*****************************************************************************/
typedef struct
{
    RRC_SUPP_BAND_COMB_ADD_R11_STRU           SuppBandCombAddR11Ext2;       /* optional */
    RRC_SUPP_BAND_COMB_ADD_R11_STRU           SuppBandCombAddR11Ext3;       /* optional */
    RRC_SUPP_BAND_COMB_ADD_R11_STRU           SuppBandCombAddR11Ext4;       /* optional */
    RRC_SUPP_BAND_COMB_ADD_R11_STRU           SuppBandCombAddR11Ext5;       /* optional */
}RRC_SUPP_BAND_COMB_V1180_EXT_STRU;
/* end:add for Boston Phase3 UE CAP,   */

/*****************************************************************************
 结构名    : RRC_MBMS_PARAM_R11_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MBMS-Parameters-r11
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitMbmsScellR11Present                      : 1;
    VOS_UINT32    bitMbmsNonServingCellR11Present             : 1;
    VOS_UINT32    bitSpare                                    : 30;

    VOS_UINT8                                   ucRev[2];
    RRC_MBMS_SCELL_R11_ENUM_UINT8               enMbmsScellR11;               /* optional */
    RRC_MBMS_NON_SERVING_CELL_R11_ENUM_UINT8    enMbmsNonServingCellR11;    /* optional */
}RRC_MBMS_PARAM_R11_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1180_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-CapabilityAddXDD-Mode-v1180
*****************************************************************************/
typedef struct
{
    RRC_MBMS_PARAM_R11_STRU              MbmsParamR11;
}RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1180_STRU;

/* begin: add for c50 ue cap,   */
/*****************************************************************************
 结构名    : RRC_NAICS_CAP_ENTRY_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : NAICS-Capability-Entry-r12
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                   ucNumOfNaicsCapableCcR12;     /*(1..5)*/
    RRC_NUM_OF_AGGREGATED_PRB_R12_ENUM_UINT8    enNumOfAggregatedPrbR12;
    VOS_UINT8                                   aucRev[2];
}RRC_NAICS_CAP_ENTRY_R12_STRU;

/*****************************************************************************
 结构名    : RRC_NAICS_CAP_LIST_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : NAICS-Capability-List-r12
*****************************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NAICS_CAP_LIST_R12)*/
    VOS_UINT8                            aucRev[2];
    RRC_NAICS_CAP_ENTRY_R12_STRU         aItem[RRC_MAX_NAICS_CAP_LIST_R12];
}RRC_NAICS_CAP_LIST_R12_STRU;

typedef     VOS_UINT8         RRC_TDD_FDD_CA_PCELL_DUPLEX_R12_UINT8;
/*****************************************************************************
 结构名    : RRC_PHY_LAYER_PARAM_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v1250-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitEHarqPatternFddR12Present                : 1;
    VOS_UINT32    bitEnhnc4txCodebookR12Present               : 1;
    VOS_UINT32    bitTddFddCaPcellDuplexR12Present            : 1;
    VOS_UINT32    bitPhyTddReCfgTddPcellR12Present            : 1;
    VOS_UINT32    bitPhyTddReCfgFddPcellR12Present            : 1;
    VOS_UINT32    bitPuschFeedbackModeR12Present              : 1;
    VOS_UINT32    bitPuschSrsPwrCtrlSubframeSetR12Present     : 1;
    VOS_UINT32    bitCsiSubframeSetR12Present                 : 1;
    VOS_UINT32    bitNoRsrcRstricForTtiBundlingR12Present     : 1;
    VOS_UINT32    bitDiscoverySignalsInDeactScellR12Present   : 1;
    VOS_UINT32    bitSpare                                    : 22;

    RRC_E_HARQ_PATTERN_FDD_R12_ENUM_UINT8                   enEHarqPatternFddR12;               /* optional */
    RRC_ENHNC_4TX_CODEBOOK_R12_ENUM_UINT8                   enEnhnc4txCodebookR12;              /* optional */
    RRC_TDD_FDD_CA_PCELL_DUPLEX_R12_UINT8                   ucTddFddCaPcellDuplexR12;           /* optional */
    RRC_PHY_TDD_RE_CONFIG_TDD_PCELL_R12_ENUM_UINT8          enPhyTddReCfgTddPcellR12;           /* optional */
    RRC_PHY_TDD_RE_CONFIG_FDD_PCELL_R12_ENUM_UINT8          enPhyTddReCfgFddPcellR12;           /* optional */
    RRC_PUSCH_FEEDBACK_MODE_R12_ENUM_UINT8                  enPuschFeedbackModeR12;             /* optional */
    RRC_PUSCH_SRS_PWR_CTRL_SUBFRAME_SET_R12_ENUM_UINT8      enPuschSrsPwrCtrlSubframeSetR12;    /* optional */
    RRC_CSI_SUBFRAME_SET_R12_ENUM_UINT8                     enCsiSubframeSetR12;                /* optional */
    RRC_NO_RSRC_RSTRIC_FOR_TTI_BUNDLING_R12_ENUM_UINT8      enNoRsrcRstricForTtiBundlingR12;    /* optional */
    RRC_DISCOVERY_SIGNALS_IN_DEACT_SCELL_R12_ENUM_UINT8     enDiscoverySignalsInDeactScellR12;  /* optional */
    VOS_UINT8                                               aucRev[2];
    RRC_NAICS_CAP_LIST_R12_STRU                             NaicsCapListR12;                    /* optional */
}RRC_PHY_LAYER_PARAM_V1250_STRU;
/*****************************************************************************
 结构名    : RRC_SUPP_BAND_EUTRA_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedBandEUTRA-v1250
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitDl256QamR12Present                       : 1;
    VOS_UINT32    bitUl64QamR12Present                        : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_DL_256_QAM_R12_ENUM_UINT8        enDl256QamR12;                /* optional */
    RRC_UL_64_QAM_R12_ENUM_UINT8         enUl64QamR12;                 /* optional */
    VOS_UINT8                            aucRev[2];
}RRC_SUPP_BAND_EUTRA_V1250_STRU;

/*****************************************************************************
 结构名    : RRC_SUPP_BAND_LIST_EUTRA_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedBandListEUTRA-v1250
*****************************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_SUPP_BANDLIST_EUTRA_V1250)*/
    VOS_UINT8                            aucRev[2];
    RRC_SUPP_BAND_EUTRA_V1250_STRU       aItem[RRC_MAX_SUPP_BANDLIST_EUTRA_V1250];
}RRC_SUPP_BAND_LIST_EUTRA_V1250_STRU;

/*********************************************************************
* ASN 文本: threeEntries-r12
* 取值范围: BIT STRING (SIZE(3))
*********************************************************************/
typedef     VOS_UINT8         RRC_THREE_ENTRIES_R12_UINT8;

/*********************************************************************
* ASN 文本: fourEntries-r12
* 取值范围: BIT STRING (SIZE(7))
*********************************************************************/
typedef     VOS_UINT8         RRC_FOUR_ENTRIES_R12_UINT8;

/*********************************************************************
* ASN 文本: fiveEntries-r12
* 取值范围: BIT STRING (SIZE(15))
*********************************************************************/
typedef     VOS_UINT16        RRC_FIVE_ENTRIES_R12_UINT16;

/*****************************************************************************
 结构名    : RRC_SUPP_CELL_GROUPING_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : supportedCellGrouping-r12
*****************************************************************************/
typedef struct
{
    VOS_UINT8             ucChoice;
    VOS_UINT8             ucRev;
    //err code start
    #define           RRC_THREE_ENTRIES_R12_CHOSEN                      1
    #define           RRC_FOUR_ENTRIES_R12_CHOSEN                       2
    #define           RRC_FIVE_ENTRIES_R12_CHOSEN                       3
    //err code end

    union
    {
        RRC_THREE_ENTRIES_R12_UINT8      ucThreeEntriesR12;
        RRC_FOUR_ENTRIES_R12_UINT8       ucFourEntriesR12;
        RRC_FIVE_ENTRIES_R12_UINT16      usFiveEntriesR12;
    }u;
}RRC_SUPP_CELL_GROUPING_R12_STRU;

/*****************************************************************************
 结构名    : RRC_DC_SUPPORT_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : dc-Support-r12
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitAsynchronousR12Present                   : 1;
    VOS_UINT32    bitSuppCellGroupingR12Present               : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_ASYNCHRONOUS_R12_ENUM_UINT8      enAsynchronousR12;            /* optional */
    VOS_UINT8                            aucRev[3];
    RRC_SUPP_CELL_GROUPING_R12_STRU      SuppCellGroupingR12;          /* optional */
}RRC_DC_SUPPORT_R12_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_NAICS_2CRS_AP_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : supportedNAICS-2CRS-AP-r12
*********************************************************************/
typedef struct
{
    VOS_UINT16         usBitCnt;  /* 1..(8 * UU_MAX_SUPP_NAICS_2CRS_AP_R12) 有效bit位*/
    VOS_UINT8          aucRev[1];
    VOS_UINT8          aucItem[RRC_MAX_SUPP_NAICS_2CRS_AP_R12];
}RRC_SUPP_NAICS_2CRS_AP_R12_STRU;

/*********************************************************************
 结构名    : RRC_COMM_SUPP_BANDS_PER_BC_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : commSupportedBandsPerBC-r12
*********************************************************************/
typedef struct
{
    VOS_UINT16         usBitCnt;  /* 1..(8 * UU_MAX_BIT_LEN_OF_COMM_SUPPBANDS_PERBC_R12) 有效bit位*/
    VOS_UINT8          aucRev[2];
    VOS_UINT8          aucItem[RRC_MAX_BIT_LEN_OF_COMM_SUPPBANDS_PERBC_R12];
}RRC_COMM_SUPP_BANDS_PER_BC_R12_STRU;

/*********************************************************************
 结构名    : RRC_BAND_COMB_PARAM_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : BandCombinationParameters-v1250
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitDcSupportR12Present                      : 1;
    VOS_UINT32    bitSuppNaics2crsApR12Present                : 1;
    VOS_UINT32    bitCommSuppBandsPerBcR12Present             : 1;
    VOS_UINT32    bitSpare                                    : 29;

    RRC_DC_SUPPORT_R12_STRU              DcSupportR12;                 /* optional */
    RRC_SUPP_NAICS_2CRS_AP_R12_STRU      SuppNaics2crsApR12;           /* optional */
    RRC_COMM_SUPP_BANDS_PER_BC_R12_STRU  CommSuppBandsPerBcR12;        /* optional */
}RRC_BAND_COMB_PARAM_V1250_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedBandCombination-v1250
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_SUPP_BAND_COMB_V1250)*/
    VOS_UINT8                            aucRev[2];
    RRC_BAND_COMB_PARAM_V1250_STRU       aItem[RRC_MAX_SUPP_BAND_COMB_V1250];
}RRC_SUPP_BAND_COMB_V1250_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_ADD_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedBandCombinationAdd-v1250
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_SUPP_BAND_COMB_ADD_V1250)*/
    VOS_UINT8                            aucRev[2];
    RRC_BAND_COMB_PARAM_V1250_STRU       aItem[RRC_MAX_SUPP_BAND_COMB_ADD_V1250];
}RRC_SUPP_BAND_COMB_ADD_V1250_STRU;

/* boston modify begin */
/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_V1250_NVID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储RRC_SUPP_BAND_COMB_V1250_STRU结构对应的NVID
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_SUPP_BAND_COMB_V1250)*/
    VOS_UINT8                            aucRev[2];
    VOS_UINT32                           aulSuppBandCombV1250ParaIdx[RRC_MAX_SUPP_BAND_COMB_V1250];
}RRC_SUPP_BAND_COMB_V1250_NVID_STRU;
/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_ADD_V1250_NVID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储RRC_SUPP_BAND_COMB_ADD_V1250_STRU结构对应的NVID
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_SUPP_BAND_COMB_ADD_V1250)*/
    VOS_UINT8                            aucRev[2];
    VOS_UINT32                           aulSuppBandCombAddV1250ParaIdx[RRC_MAX_SUPP_BAND_COMB_ADD_V1250];
}RRC_SUPP_BAND_COMB_ADD_V1250_NVID_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_ADD_V1250_NVID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储RF_PARAM_V1250中SUPP_BAND_COMB_V1250的NVID列表
*********************************************************************/
typedef struct
{
    RRC_SUPP_BAND_COMB_V1250_NVID_STRU          stRrcSuppCombV1250NvIdList;
    RRC_SUPP_BAND_COMB_V1250_NVID_STRU          stRrcSuppCombV1250NvIdListExt;
    RRC_SUPP_BAND_COMB_ADD_V1250_NVID_STRU      stRrcSuppCombAddV1250NvIdList;
    RRC_SUPP_BAND_COMB_ADD_V1250_NVID_STRU      stRrcSuppCombAddV1250NvIdListExt;
    RRC_SUPP_BAND_COMB_ADD_V1250_NVID_STRU      stRrcSuppCombAddV1250NvIdListExt1;
    RRC_SUPP_BAND_COMB_ADD_V1250_NVID_STRU      stRrcSuppCombAddV1250NvIdListExt2;
}RRC_RF_PARAM_V1250_SUPP_BAND_COMB_V1250_NVID_LIST_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_V1250_PER_NV_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储每个NV对应的SUPP_BAND_COMB_V1250
*********************************************************************/
typedef struct
{
    RRC_BAND_COMB_PARAM_V1250_STRU       stRrcBandCombPrarmV1250;
}RRC_SUPP_BAND_COMB_V1250_PER_NV_STRU;
/* boston modify end */
/*********************************************************************
 结构名    : RRC_RF_PARAM_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RF-Parameters-v1250
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitFreqBandPriorAdjR12Present               : 1;
    VOS_UINT32    bitSpare                                    : 31;

    RRC_SUPP_BAND_LIST_EUTRA_V1250_STRU         SuppBandListEutraV1250;       /* optional */
    //RRC_SUPP_BAND_COMB_V1250_STRU               SuppBandCombV1250;            /* optional */
    //RRC_SUPP_BAND_COMB_ADD_V1250_STRU           SuppBandCombAddV1250;         /* optional */
    RRC_FREQ_BAND_PRIOR_ADJ_R12_ENUM_UINT8      enFreqBandPriorAdjR12;        /* optional */
    VOS_UINT8                                   aucRev[3];
}RRC_RF_PARAM_V1250_STRU;

/*********************************************************************
 结构名    : RRC_UE_BASED_NETW_PERF_MEAS_PARAM_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-BasedNetwPerfMeasParameters-v1250
*********************************************************************/
typedef struct
{
    RRC_LOGGED_MBSFN_MEAS_R12_ENUM_UINT8    enLoggedMbsfnMeasR12;
    VOS_UINT8                               aucRev[3];
}RRC_UE_BASED_NETW_PERF_MEAS_PARAM_V1250_STRU;

/*********************************************************************
 结构名    : RRC_WLAN_IW_PARAM_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : WLAN-IW-Parameters-r12
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitWlanIwRanRulesR12Present                 : 1;
    VOS_UINT32    bitWlanIwAndsfPoliciesR12Present            : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_WLAN_IW_RAN_RULES_R12_ENUM_UINT8         enWlanIwRanRulesR12;          /* optional */
    RRC_WLAN_IW_ANDSF_POLICIES_R12_ENUM_UINT8    enWlanIwAndsfPoliciesR12;     /* optional */
    VOS_UINT8                                    aucRev[2];
}RRC_WLAN_IW_PARAM_R12_STRU;

/*********************************************************************
 结构名    : RRC_MEAS_PARAM_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MeasParameters-v1250
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitTimerT312R12Present                      : 1;
    VOS_UINT32    bitAlternativeTimeToTrigR12Present          : 1;
    VOS_UINT32    bitIncMonEutraR12Present                    : 1;
    VOS_UINT32    bitIncMonUtraR12Present                     : 1;
    VOS_UINT32    bitExtendedMaxMeasIdR12Present              : 1;
    VOS_UINT32    bitExtendedRsrqLowerRangeR12Present         : 1;
    VOS_UINT32    bitRsrqOnAllSymbolsR12Present               : 1;
    VOS_UINT32    bitCrsDiscoverySignalsMeasR12Present        : 1;
    VOS_UINT32    bitCsiRsDiscoverySignalsMeasR12Present      : 1;
    VOS_UINT32    bitSpare                                    : 23;

    RRC_TIMER_T312_R12_ENUM_UINT8                       enTimerT312R12;                 /* optional */
    RRC_ALTERNATIVE_TIME_TO_TRIG_R12_ENUM_UINT8         enAlternativeTimeToTrigR12;     /* optional */
    RRC_INC_MON_EUTRA_R12_ENUM_UINT8                    enIncMonEutraR12;               /* optional */
    RRC_INC_MON_UTRA_R12_ENUM_UINT8                     enIncMonUtraR12;                /* optional */
    RRC_EXTENDED_MAX_MEAS_ID_R12_ENUM_UINT8             enExtendedMaxMeasIdR12;         /* optional */
    RRC_EXTENDED_RSRQ_LOWER_RANGE_R12_ENUM_UINT8        enExtendedRsrqLowerRangeR12;    /* optional */
    RRC_RSRQ_ON_ALL_SYMBOLS_R12_ENUM_UINT8              enRsrqOnAllSymbolsR12;          /* optional */
    RRC_CRS_DISCOVERY_SIGNALS_MEAS_R12_ENUM_UINT8       enCrsDiscoverySignalsMeasR12;   /* optional */
    RRC_CSI_RS_DISCOVERY_SIGNALS_MEAS_R12_ENUM_UINT8    enCsiRsDiscoverySignalsMeasR12; /* optional */
    VOS_UINT8                                           aucRev[3];
}RRC_MEAS_PARAM_V1250_STRU;

/*********************************************************************
 结构名    : RRC_DC_PARAM_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : DC-Parameters-r12
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitDrbTypeSplitR12Present                   : 1;
    VOS_UINT32    bitDrbTypeScgR12Present                     : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_DRB_TYPE_SPLIT_R12_ENUM_UINT8    enDrbTypeSplitR12;            /* optional */
    RRC_DRB_TYPE_SCG_R12_ENUM_UINT8      enDrbTypeScgR12;              /* optional */
    VOS_UINT8                            aucRev[2];
}RRC_DC_PARAM_R12_STRU;

/*********************************************************************
 结构名    : RRC_MBMS_PARAM_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MBMS-Parameters-v1250
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitMbmsAsyncDcR12Present                    : 1;
    VOS_UINT32    bitSpare                                    : 31;

    RRC_MBMS_ASYNC_DC_R12_ENUM_UINT8     enMbmsAsyncDcR12;             /* optional */
    VOS_UINT8                            aucRev[3];
}RRC_MBMS_PARAM_V1250_STRU;

/*********************************************************************
 结构名    : RRC_MAC_PARAM_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : MAC-Parameters-r12
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitLogicChSrProhbTimerR12Present            : 1;
    VOS_UINT32    bitLongDrxCmdR12Present                     : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_LOGIC_CH_SR_PROHB_TIMER_R12_ENUM_UINT8    enLogicChSrProhbTimerR12;    /* optional */
    RRC_LONG_DRX_CMD_R12_ENUM_UINT8               enLongDrxCmdR12;              /* optional */
    VOS_UINT8                                     aucRev[2];
}RRC_MAC_PARAM_R12_STRU;

/*********************************************************************
 结构名    : RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1250_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-CapabilityAddXDD-Mode-v1250
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitPhyLayerParamV1250Present                : 1;
    VOS_UINT32    bitMeasParamV1250Present                    : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_PHY_LAYER_PARAM_V1250_STRU       PhyLayerParamV1250;           /* optional */
    RRC_MEAS_PARAM_V1250_STRU            MeasParamV1250;               /* optional */
}RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1250_STRU;

/*********************************************************************
 结构名    : RRC_FREQ_BAND_IND_LIST_EUTRA_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : FreqBandIndicatorListEUTRA-r12
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_FREQ_BANDINDLIST_EUTRA_R12)*/
    VOS_UINT8                            aucRev[2];
    RRC_FREQ_BAND_IND_R11_UINT16         ausItem[RRC_MAX_FREQ_BANDINDLIST_EUTRA_R12];
}RRC_FREQ_BAND_IND_LIST_EUTRA_R12_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_INFO_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedBandInfo-r12
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitSupportR12Present                        : 1;
    VOS_UINT32    bitSpare                                    : 31;

    RRC_SUPPORT_R12_ENUM_UINT8           enSupportR12;                 /* optional */
    VOS_UINT8                            aucRev[3];
}RRC_SUPP_BAND_INFO_R12_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_INFO_LIST_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : SupportedBandInfoList-r12
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_SUPP_BANDINFO_LIST_R12)*/
    VOS_UINT8                            aucRev[2];
    RRC_SUPP_BAND_INFO_R12_STRU          aItem[RRC_MAX_SUPP_BANDINFO_LIST_R12];
}RRC_SUPP_BAND_INFO_LIST_R12_STRU;

/*********************************************************************
* ASN 文本 :SL-Parameters-r12
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitCommSimulTxR12Present                    : 1;
    VOS_UINT32    bitDiscScheduledRsrcAllocR12Present         : 1;
    VOS_UINT32    bitDiscUeSelectedRsrcAllocR12Present        : 1;
    VOS_UINT32    bitDiscSlssR12Present                       : 1;
    VOS_UINT32    bitDiscSuppProcR12Present                   : 1;
    VOS_UINT32    bitSpare                                    : 27;

    RRC_COMM_SIMUL_TX_R12_ENUM_UINT8                    enCommSimulTxR12;               /* optional */
    VOS_UINT8                                           aucRev[3];
    RRC_FREQ_BAND_IND_LIST_EUTRA_R12_STRU               CommSuppBandsR12;               /* optional */
    RRC_SUPP_BAND_INFO_LIST_R12_STRU                    DiscSuppBandsR12;               /* optional */
    RRC_DISC_SCHEDULED_RSRC_ALLOC_R12_ENUM_UINT8        enDiscScheduledRsrcAllocR12;    /* optional */
    RRC_DISC_UE_SELECTED_RSRC_ALLOC_R12_ENUM_UINT8      enDiscUeSelectedRsrcAllocR12;   /* optional */
    RRC_DISC_SLSS_R12_ENUM_UINT8                        enDiscSlssR12;                  /* optional */
    RRC_DISC_SUPP_PROC_R12_ENUM_UINT8                   enDiscSuppProcR12;              /* optional */
}RRC_SL_PARAM_R12_STRU;

/* end: add for c50 ue cap,   */

/*****************************************************************************
 结构名    : RRC_RLC_PARAM_R12_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC_RLC_PARAM_R12_STRU
*****************************************************************************/
typedef struct
{
    RRC_EXTENDED_RLC_LI_FIELD_R12_ENUM_UINT8    enExtendedRlcLiFieldR12;
    VOS_UINT8                                   aucRev[3];
}RRC_RLC_PARAM_R12_STRU;

/* begin: add for c50 ue cap,   */
/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1250_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC_UE_EUTRA_CAP_V1250_IES_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitPhyLayerParamV1250Present                : 1;
    VOS_UINT32    bitRfParamV1250Present                      : 1;
    VOS_UINT32    bitRlcParamR12Present                       : 1;
    VOS_UINT32    bitUeBasedNetwPerfMeasParamV1250Present     : 1;
    VOS_UINT32    bitUeCatgDlR12Present                       : 1;
    VOS_UINT32    bitUeCatgUlR12Present                       : 1;
    VOS_UINT32    bitWlanIwParamR12Present                    : 1;
    VOS_UINT32    bitMeasParamV1250Present                    : 1;
    VOS_UINT32    bitDcParamR12Present                        : 1;
    VOS_UINT32    bitMbmsParamV1250Present                    : 1;
    VOS_UINT32    bitMacParamR12Present                       : 1;
    VOS_UINT32    bitFddAddUeEutraCapabilitiesV1250Present    : 1;
    VOS_UINT32    bitTddAddUeEutraCapabilitiesV1250Present    : 1;
    VOS_UINT32    bitSlParamR12Present                        : 1;
    VOS_UINT32    bitSpare                                    : 18;

    RRC_PHY_LAYER_PARAM_V1250_STRU                  PhyLayerParamV1250;             /* optional */
    RRC_RF_PARAM_V1250_STRU                         RfParamV1250;                   /* optional */
    RRC_RLC_PARAM_R12_STRU                          RlcParamR12;                    /* optional */
    RRC_UE_BASED_NETW_PERF_MEAS_PARAM_V1250_STRU    UeBasedNetwPerfMeasParamV1250;  /* optional */
    VOS_UINT8                                       ucUeCatgDlR12;                  /* optional */    /*(0..14)*/
    VOS_UINT8                                       ucUeCatgUlR12;                  /* optional */    /*(0..13)*/
    VOS_UINT8                                       aucRev[2];

}RRC_UE_EUTRA_CAP_V1250_IES_STRU;
/* end: add for c50 ue cap,   */
/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1250_EXT_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC_UE_EUTRA_CAP_V1250_EXT_IES_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitNonCritiExtPresent             : 1;
    VOS_UINT32    bitSpare                          : 31;
    /* boston add begin */
    RRC_WLAN_IW_PARAM_R12_STRU                      WlanIwParamR12;                 /* optional */
    RRC_MEAS_PARAM_V1250_STRU                       MeasParamV1250;                 /* optional */
    RRC_DC_PARAM_R12_STRU                           DcParamR12;                     /* optional */
    RRC_MBMS_PARAM_V1250_STRU                       MbmsParamV1250;                 /* optional */
    RRC_MAC_PARAM_R12_STRU                          MacParamR12;                    /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1250_STRU        FddAddUeEutraCapabilitiesV1250; /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1250_STRU        TddAddUeEutraCapabilitiesV1250; /* optional */
    RRC_SL_PARAM_R12_STRU                           SlParamR12;                     /* optional */
    /* boston add end */

}RRC_UE_EUTRA_CAP_V1250_EXT_IES_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1180_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v1180-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitRfParamV1180Present                      : 1;
    VOS_UINT32    bitMbmsParamR11Present                      : 1;
    VOS_UINT32    bitFddAddUeEutraCapabilitiesV1180Present    : 1;
    VOS_UINT32    bitTddAddUeEutraCapabilitiesV1180Present    : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 27;

    RRC_RF_PARAM_V1180_STRU                     RfParamV1180;                 /* optional */
    RRC_MBMS_PARAM_R11_STRU                     MbmsParamR11;                 /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1180_STRU    FddAddUeEutraCapabilitiesV1180;    /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1180_STRU    TddAddUeEutraCapabilitiesV1180;    /* optional */
    /*RRC_UE_EUTRA_CAP_V11A0_IES_STRU           NonCritiExt; */                  /* optional */
}RRC_UE_EUTRA_CAP_V1180_IES_STRU;

/*****************************************************************************
 结构名    : RRC_MEAS_PARAM_V11A0_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC_MEAS_PARAM_V11A0_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitBenefitsFromInterruptionR11Present       : 1;
    VOS_UINT32    bitSpare                                    : 31;

    VOS_UINT8     enBenefitsFromInterruptionR11;    /* optional */
    VOS_UINT8     aucRev[3];
}RRC_MEAS_PARAM_V11A0_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V11A0_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : RRC_UE_EUTRA_CAP_V11A0_IES_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitUeCatgV11a0Present                       : 1;
    VOS_UINT32    bitMeasParamV11a0Present                    : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 29;

    VOS_UINT8                            ucUeCatgV11a0;                /* optional */    /*(11..12)*/
    VOS_UINT8                            aucRev[3];
    RRC_MEAS_PARAM_V11A0_STRU            MeasParamV11a0;               /* optional */
    /*RRC_UE_EUTRA_CAP_V1250_IES_STRU    NonCritiExt; */                 /* optional */
}RRC_UE_EUTRA_CAP_V11A0_IES_STRU;

/*****************************************************************************
 结构名    : RRC_INTER_BAND_TDD_CA_DIFF_CFG_R11_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : interBandTDD-CA-WithDifferentConfig-r11  BIT STRING(SIZE (2))
*****************************************************************************/
typedef struct
{
    VOS_UINT8     aucInterBandTddCaDiffCfgR11[RRC_MAX_NUM_OF_INTER_BAND_TDD_CA_DIFF_CFG_R11];
    VOS_UINT8     ucRev[2];
} RRC_INTER_BAND_TDD_CA_DIFF_CFG_R11_STRU;

/*****************************************************************************
 结构名    : RRC_PHY_LAYER_PARAM_V1170_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : phyLayerParameters-v1170
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitInterBandTddCaDiffCfgR11Present        : 1;
    VOS_UINT32    bitSpare                                  : 31;

    RRC_INTER_BAND_TDD_CA_DIFF_CFG_R11_STRU    stInterBandTddCaDiffCfgR11;     /* optional */
}RRC_PHY_LAYER_PARAM_V1170_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1170_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v1170-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitPhyLayerParamV1170Present                : 1;
    VOS_UINT32    bitucUeCategoryV1170Present                 : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 29;

    RRC_PHY_LAYER_PARAM_V1170_STRU       stPhyLayerParamV1170;           /* optional */
    VOS_UINT8                            ucRev[3];
    VOS_UINT8                            ucUeCategoryV1170;     /*(9..10)   optional */
    /*R12 nv begin*/
    //RRC_UE_EUTRA_CAP_V1180_IES_STRU    stUeEutraCapV1180;
    /*R12 nv end*/
}RRC_UE_EUTRA_CAP_V1170_IES_STRU;
/*end: add for feature v700r500 ,  */

/*********************************************************************
* ASN 文本 :PhyLayerParameters-v1130
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitCrsInterfHandlR11Present                 : 1;
    VOS_UINT32    bitEpdcchR11Present                         : 1;
    VOS_UINT32    bitMultiAckCsiRprtingR11Present             : 1;
    VOS_UINT32    bitSsCchInterfHandlR11Present               : 1;
    VOS_UINT32    bitTddSpecialSubframeR11Present             : 1;
    VOS_UINT32    bitTxDivPucch1bChSelectR11Present           : 1;
    VOS_UINT32    bitUlCompR11Present                         : 1;
    VOS_UINT32    bitSpare                                    : 25;

    VOS_UINT8                            ucRev;
    RRC_CRS_INTERF_HANDL_R11_ENUM_UINT8  enCrsInterfHandlR11;          /* optional */
    RRC_EPDCCH_R11_ENUM_UINT8            enEpdcchR11;                  /* optional */
    RRC_MULTI_ACK_CSI_RPRTING_R11_ENUM_UINT8    enMultiAckCsiRprtingR11;    /* optional */
    RRC_SS_CCH_INTERF_HANDL_R11_ENUM_UINT8    enSsCchInterfHandlR11;   /* optional */
    RRC_TDD_SPECIAL_SUBFRAME_R11_ENUM_UINT8    enTddSpecialSubframeR11;    /* optional */
    RRC_TX_DIV_PUCCH1B_CH_SELECT_R11_ENUM_UINT8    enTxDivPucch1bChSelectR11;    /* optional */
    RRC_UL_COMP_R11_ENUM_UINT8           enUlCompR11;                  /* optional */
}RRC_PHY_LAYER_PARAM_V1130_STRU;

/*********************************************************************
结构名    : RRC_PDCP_PARAM_V1130_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_PDCP_PARAM_V1130_STRU-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitPdcpSnExtR11Present                      : 1;
    VOS_UINT32    bitSupportRohcContextContinueR11Present     : 1;
    VOS_UINT32    bitSpare                                    : 30;

    VOS_UINT8     ucRev[2];
    RRC_PDCP_SN_EXT_R11_ENUM_UINT8       enPdcpSnExtR11;               /* optional */
    RRC_SUPPORT_ROHC_CONTEXT_CONTINUE_R11_ENUM_UINT8    enSupportRohcContextContinueR11;    /* optional */
}RRC_PDCP_PARAM_V1130_STRU;

/*********************************************************************
* ASN 文本 :BandParameters-v1130
*********************************************************************/
typedef struct
{
    VOS_UINT8                            ucRev[3];
    RRC_SUPP_CSI_PROC_R11_ENUM_UINT8     enSuppCsiProcR11;
}RRC_BAND_PARAM_V1130_STRU;

typedef struct
{
    VOS_UINT8                            ucRev[2];
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10)*/
    RRC_BAND_PARAM_V1130_STRU            aItem[RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10];
}RRC_BAND_PARAM_LIST_R11_STRU;

/*********************************************************************
* ASN 文本 :BandCombinationParameters-v1130
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitMultipleTimingAdvanceR11Present          : 1;
    VOS_UINT32    bitSimulRxTxR11Present                      : 1;
    VOS_UINT32    bitSpare                                    : 30;

    VOS_UINT8                            ucRev[2];
    RRC_MULTIPLE_TIMING_ADVANCE_R11_ENUM_UINT8    enMultipleTimingAdvanceR11;    /* optional */
    RRC_SIMUL_RX_TX_R11_ENUM_UINT8       enSimulRxTxR11;               /* optional */
    RRC_BAND_PARAM_LIST_R11_STRU         BandParamListR11;             /* optional */
}RRC_BAND_COMB_PARAM_V1130_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandCombination-v1130
*********************************************************************/
typedef struct
{
    VOS_UINT8                            ucRev[2];
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R10)*/
    RRC_BAND_COMB_PARAM_V1130_STRU       aItem[RRC_MAX_NUM_OF_BAND_COMB_R10];
}RRC_SUPP_BAND_COMB_V1130_STRU;

/*********************************************************************
结构名    : RRC_RF_PARAM_V1130_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_RF_PARAM_V1130_STRU-IEs
*********************************************************************/
typedef struct
{
    RRC_SUPP_BAND_COMB_V1130_STRU        SuppBandCombV1130;            /* optional */
}RRC_RF_PARAM_V1130_STRU;

/*********************************************************************
结构名    : RRC_MEAS_PARAM_V1130_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_MEAS_PARAM_V1130_STRU-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitRsrqMeasWidebandR11Present               : 1;
    VOS_UINT32    bitSpare                                    : 31;

    VOS_UINT8                            ucRev[3];
    RRC_RSRQ_MEAS_WIDEBAND_R11_ENUM_UINT8    enRsrqMeasWidebandR11;    /* optional */
}RRC_MEAS_PARAM_V1130_STRU;


/*********************************************************************
结构名    : RRC_IRAT_PARAM_CDMA2000_V1130_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_IRAT_PARAM_CDMA2000_V1130_STRU-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitCdma2000NwSharingR11Present              : 1;
    VOS_UINT32    bitSpare                                    : 31;

    VOS_UINT8                            ucRev[3];
    RRC_CDMA2000_NW_SHARING_R11_ENUM_UINT8    enCdma2000NwSharingR11;  /* optional */
}RRC_IRAT_PARAM_CDMA2000_V1130_STRU;



/*********************************************************************
结构名    : RRC_OTHER_PARAM_R11_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_OTHER_PARAM_R11_STRU-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitInDeviceCoexIndR11Present                : 1;
    VOS_UINT32    bitPwrPrefIndR11Present                     : 1;
    VOS_UINT32    bitUeRxTxTimeDiffMeasR11Present             : 1;
    VOS_UINT32    bitSpare                                    : 29;

    VOS_UINT8                            ucRev;
    RRC_IN_DEVICE_COEX_IND_R11_ENUM_UINT8    enInDeviceCoexIndR11;     /* optional */
    RRC_PWR_PREF_IND_R11_ENUM_UINT8      enPwrPrefIndR11;              /* optional */
    RRC_UE_RX_TX_TIME_DIFF_MEAS_R11_ENUM_UINT8    enUeRxTxTimeDiffMeasR11;    /* optional */
}RRC_OTHER_PARAM_R11_STRU;

/*begin: add for feature v700r500*/
/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1130_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-CapabilityAddXDD-Mode-v1130
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitstPhyLayerParamV1130Present              : 1;
    VOS_UINT32    bitstMeasParamV1130Present                  : 1;
    VOS_UINT32    bitstOtherParamR11Present                   : 1;
    VOS_UINT32    bitSpare                                    : 29;

    RRC_PHY_LAYER_PARAM_V1130_STRU              stPhyLayerParamV1130;       /* optional */
    RRC_MEAS_PARAM_V1130_STRU                   stMeasParamV1130;           /* optional */
    RRC_OTHER_PARAM_R11_STRU                    stOtherParamR11;            /* optional */
}RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1130_STRU;
/*end: add for feature v700r500*/

/*begin: modify for feature v700r500*/
/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1130_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v1130-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitPhyLayerParamV1130Present                : 1;
    VOS_UINT32    bitFddAddUeEutraCapabilitiesV1130Present    : 1;
    VOS_UINT32    bitTddAddUeEutraCapabilitiesV1130Present    : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 28;

    RRC_PDCP_PARAM_V1130_STRU            PdcpParamV1130;
    RRC_PHY_LAYER_PARAM_V1130_STRU       PhyLayerParamV1130;           /* optional */
    RRC_RF_PARAM_V1130_STRU              RfParamV1130;
    RRC_MEAS_PARAM_V1130_STRU            MeasParamV1130;
    RRC_IRAT_PARAM_CDMA2000_V1130_STRU   InterRatParamCdma2000V1130;
    RRC_OTHER_PARAM_R11_STRU             OtherParamR11;
    /*R11 NV begin*/
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1130_STRU    stFddAddUeEutraCapabilitiesV1130;    /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1130_STRU    stTddAddUeEutraCapabilitiesV1130;    /* optional */
    //RRC_UE_EUTRA_CAP_V1170_IES_STRU           stUeEutraCapV1170;                   /* optional */
    /*R11 NV end*/
}RRC_UE_EUTRA_CAP_V1130_IES_STRU;
/*end: modify for feature v700r500*/

/*********************************************************************
结构名    : RRC_BAND_PARAM_V1090_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_BAND_PARAM_V1090_STRU-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitBandEutraV1090Present                    : 1;
    VOS_UINT32    bitSpare                                    : 31;

    VOS_UINT8                            ucRev[2];
    RRC_FREQ_BAND_IND_V9E0_UINT16        usBandEutraV1090;             /* optional */
}RRC_BAND_PARAM_V1090_STRU;

/*********************************************************************
结构名    : RRC_BAND_COMB_PARAM_V1090_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_BAND_COMB_PARAM_V1090_STRU-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT8                            ucRev[2];
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10)*/
    RRC_BAND_PARAM_V1090_STRU            aItem[RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10];
}RRC_BAND_COMB_PARAM_V1090_STRU;

/*********************************************************************
结构名    : RRC_SUPP_BAND_COMB_V1090_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_SUPP_BAND_COMB_V1090_STRU-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT8                            ucRev[2];
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BAND_COMB_R10)*/
    RRC_BAND_COMB_PARAM_V1090_STRU       aItem[RRC_MAX_NUM_OF_BAND_COMB_R10];
}RRC_SUPP_BAND_COMB_V1090_STRU;

/*********************************************************************
结构名    : RRC_RF_PARAM_V1090_STRU
协议表格  :
ASN.1描述 :
结构说明  : RRC_RF_PARAM_V1090_STRU-IEs
*********************************************************************/
typedef struct
{
    RRC_SUPP_BAND_COMB_V1090_STRU        SuppBandCombV1090;            /* optional */
}RRC_RF_PARAM_V1090_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1090_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v1090-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitRfParamV1090Present                      : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_RF_PARAM_V1090_STRU              RfParamV1090;                 /* optional */

}RRC_UE_EUTRA_CAP_V1090_IES_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1060_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v1060-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitFddAddUeEutraCapabilitiesV1060Present    : 1;
    VOS_UINT32    bitTddAddUeEutraCapabilitiesV1060Present    : 1;
    VOS_UINT32    bitRfParamV1060Present                      : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 28;

    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1060_STRU    stFddAddUeEutraCapabilitiesV1060;    /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1060_STRU    stTddAddUeEutraCapabilitiesV1060;    /* optional */
    /* R10 NV修改 begin*/
    //RRC_RF_PARAM_V1060_STRU                     stRfParamV1060;                 /* optional */
    /* R10 NV修改 end*/
}RRC_UE_EUTRA_CAP_V1060_IES_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1020_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v1020-IEs
*****************************************************************************/
typedef struct
{
    /* R10 NV 修改 begin */
    VOS_UINT32    bitUeCatgV1020Present                       : 1;
    VOS_UINT32    bitPhyLayerParamV1020Present                : 1;
    //VOS_UINT32    bitRfParamV1020Present                      : 1;
    //VOS_UINT32    bitMeasParamV1020Present                    : 1;
    VOS_UINT32    bitRfMeasParamV1020Present                  : 1;
    VOS_UINT32    bitFeatureGroupIndRel10R10Present           : 1;
    VOS_UINT32    bitInterRatParamCdma2000V1020Present        : 1;
    VOS_UINT32    bitUeBasedNetwPerfMeasParamR10Present       : 1;
    VOS_UINT32    bitInterRatParamUtraTddV1020Present         : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 24;
    VOS_UINT8     aucReserved[3];

    VOS_UINT8                                   ucUeCatgV1020;                  /* optional */    /*(6..8)*/
    RRC_PHY_LAYER_PARAM_V1020_STRU              stPhyLayerParamV1020;           /* optional */

    //RRC_RF_PARAM_V1020_STRU                   stRfParamV1020;                 /* optional */
    //RRC_MEAS_PARAM_V1020_STRU                 stMeasParamV1020;               /* optional */
    RRC_UE_CAP_FEATURE_GROUP_IND_R10_STRU       stFeatureGroupIndRel10R10;      /* optional */
    RRC_UE_BASED_NETW_PERF_MEAS_PARAM_R10_STRU  stUeBasedNetwPerfMeasParamR10;  /* optional */
    //RRC_UE_EUTRA_CAP_V1060_IES_STRU           stNonCritiExt;
    RRC_RF_MEAS_PARAM_V1020_STRU                stRfMeasParamV1020;
    RRC_RF_MEAS_PARAM_V1020_STRU                stRfMeasParamV1020Ext;
    /* optional */
    /* R10 NV 修改 end */

}RRC_UE_EUTRA_CAP_V1020_IES_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V940_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v940-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitUeEutraCapV9a0Present                    : 1;
    VOS_UINT32    bitUeEutraCapV1020Present                   : 1;
    VOS_UINT32    bitSpare                                    : 30;

    /*R10 NV begin*/
    //RRC_UE_EUTRA_CAP_V9A0_IES_STRU              stUeEutraCapV9a0;               /* optional */
    //RRC_UE_EUTRA_CAP_V1020_IES_STRU             stUeEutraCapV1020;              /* optional */
    /*R10 NV end*/
}RRC_UE_EUTRA_CAP_V940_IES_STRU;
/* v7r2 LLD end */

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V920_IES_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : UE-EUTRA-Capability-v920-IEs
*****************************************************************************/
typedef struct
{
    VOS_UINT32    bitInterRatParamUtraV920Present             : 1;
    VOS_UINT32    bitInterRatParamCdma2000V920Present         : 1;
    /* v7r2 LLD begin */
    VOS_UINT32    bitUeEutraCapV940Present                    : 1;
    VOS_UINT32    bitSpare                                    : 29;
    /* v7r2 LLD end */

    RRC_PHY_LAYER_PARAM_V920_STRU               stPhyLayerParamV920;
    RRC_IRAT_PARAM_GERAN_V920_STRU              stInterRatParamGeranV920;
    RRC_IRAT_PARAM_UTRA_V920_STRU               stInterRatParamUtraV920;        /* optional */
    RRC_IRAT_PARAM_CDMA2000_1XRTT_V920_STRU     stInterRatParamCdma2000V920;    /* optional */
    RRC_UE_CAP_DEVICE_TYPE_R9_ENUM_UINT8        enDeviceTypeR9;
    VOS_UINT8                                   aucReserved[3];
    RRC_CSG_PROXIMITY_IND_PARAM_R9_STRU         stCsgProximityIndParamR9;
    RRC_NEIGH_CELL_SI_ACQ_PARAM_R9_STRU         stNeighCellSiAcqParamR9;
    RRC_SON_PARAM_R9_STRU                       stSonParamR9;
    /* v7r2 LLD begin */
    RRC_UE_EUTRA_CAP_V940_IES_STRU              stUeEutraCapV940;               /* optional */
    /* v7r2 LLD end */
}RRC_UE_EUTRA_CAP_V920_IES_STRU;

/* add by   for NorStandard begin */
/*****************************************************************************
 结构名    : LTE_NON_STANDARD_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 非标频段的频点信息相关的数据结构,与36101 talbe 5.7.3-1对应

            RealFreq = usFLow + (RealArfcn - usRangOfNLow),
            其中 RealFreq 真实的频点信息；usRangOfNLow、usFLow从NV项中获取，具体含义
            见数据结构中的说明; RealArfcn实际的arfcn。
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                usFLow;                           /* 起始频点，单位是100khz */
    VOS_UINT16                                usFHigh;                          /* 结束频点，单位是100khz */
    VOS_UINT16                                usRangOfNLow;                     /* Nlow,与Noffset一致 */
    VOS_UINT16                                usRangOfNHigh;                    /* Nhigh*/
}LTE_COMM_NON_STANDARD_FREQ_INFO_STRU;

/*****************************************************************************
 结构名    : LTE_NON_STANDARD_POWER_CLASS_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 非标频段power class相关的数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT16                                 usUEPowerClass;
    VOS_INT16                                  sPowerToleranceHigh;    /* 单位 0.1dBm */
    VOS_INT16                                  sPowerToleranceLow;     /* 单位 0.1dBm */
    VOS_UINT8                                  aucReservered[2];
}LTE_COMM_NON_STANDARD_POWER_CLASS_INFO_STRU;

/*****************************************************************************
 结构名    : LTE_NON_STANDARD_BAND_TEST_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 非标频段的测试信息相关的数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                   ucBandWidth;                       /* 带宽，单位0.1MHz，可选值14;30;50;150;200 */
    VOS_UINT8                                   aucReservered[3];
    VOS_UINT16                                  usTestFreqs[3];                    /* 测试低、中、高频点，单位0.1MHz */
    VOS_INT16					                sRxRefSensPower;                   /* 下行测试灵敏度功率，单位0.1dBm */
    VOS_UINT32                                  aulReservered[16];
}LTE_NON_STANDARD_BAND_TEST_INFO_STRU;

/*****************************************************************************
 结构名    : LTE_NON_STANDARD_BAND_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 非标频段的频点信息相关的数据结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                                   ucBandID;                       /* band iD */
    LTE_COMM_BAND_MODE_TYPE_ENUM_UINT8          enBandMode;
    VOS_INT16                                   sTxRxFreqSepara;
    LTE_COMM_NON_STANDARD_FREQ_INFO_STRU        stUlFreqInfo;
    LTE_COMM_NON_STANDARD_FREQ_INFO_STRU        stDlFreqInfo;
    LTE_COMM_NON_STANDARD_POWER_CLASS_INFO_STRU stUePowerClassInfo;
    LTE_NON_STANDARD_BAND_TEST_INFO_STRU		stTestInfo[6];                 /*1.4MHz；3MHz；5MHz,10MHz,15MHz；20MHz带宽的测试参数 */
    VOS_UINT32                                  aulReservered[16];
}LTE_COMM_NON_STANDARD_BAND_COMM_STRU;

/* add by   for NorStandard end */

/* begin: add for MCC report CA,   */
/*****************************************************************************
 结构名    : RRC_UE_CAP_MCC_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 每个MCC对应的CA组合结构 132Byte
*****************************************************************************/
typedef struct
{
    VOS_UINT32              ulMcc;
    VOS_UINT32              aulMccSupportCA[RRC_MAX_NUM_OF_CA_PER_MCC];
}RRC_UE_CAP_MCC_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_MCC_CA_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 根据MCC上报CA组合结构
*****************************************************************************/
typedef struct
{
    RRC_UE_CAP_MCC_STRU     astUeCapMcc[RRC_MAX_NUM_OF_MCC];
}RRC_UE_CAP_MCC_CA_INFO_STRU;
/* end: add for MCC report CA,   */

/* Added by   for ue cap report, 2016-6-17, begin */
/*****************************************************************************
 结构名    : RRC_UE_CAP_MIN_CA_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 根据上报默认最小CA组合结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8     ucPrefBandSwitch;
    VOS_UINT8     ucMinCaSwitch;
    VOS_UINT8     ucMaxByteMinCaSwitch;
    VOS_UINT8     aucReservered[3];

    VOS_UINT8     ucGTLMinBandNum;
    VOS_UINT8     ucGULMinBandNum;

    VOS_UINT8     aucGTLMinSupportBand[RRC_MIN_NUM_OF_BAND];
    VOS_UINT8     aucGULMinSupportBand[RRC_MIN_NUM_OF_BAND];
    VOS_UINT32    aulGTLMinSupportCA[RRC_MIN_NUM_OF_CA];
    VOS_UINT32    aulGULMinSupportCA[RRC_MIN_NUM_OF_CA];
}RRC_UE_CAP_MIN_CA_INFO_STRU;
/* Added by   for ue cap report, 2016-6-17, end */

/* Begin: add for Report Ue Capability By MCC */
/*****************************************************************************
 结构名    : RRC_UE_CAP_MCC_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 每个MCC对应的Band组合结构 132Byte
*****************************************************************************/
typedef struct
{
    VOS_UINT16              usMcc;
    VOS_UINT8               ucLteSupportBandNum;                                /* 指示该MCC下支持的 Lte Band个数 */
    VOS_UINT8               ucUtranSupportBandNum;                              /* 指示该MCC下支持的 Utran Band个数 */
    VOS_UINT8               aucMccLteSupportBand[LRRC_LTE_MAX_NUM_OF_BAND_PER_MCC];
    VOS_UINT8               aucMccUtranSupportBand[LRRC_UTRAN_MAX_NUM_OF_BAND_PER_MCC];
}RRC_UE_CAP_MCC_BAND_STRU;

/*****************************************************************************
 结构名    : RRC_UE_CAP_MCC_BAND_INFO_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 根据MCC上报Band结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                   ulMccNum;                                      /* 指示多少组MCC有效 */
    RRC_UE_CAP_MCC_BAND_STRU     astUeCapMccBand[RRC_NUM_OF_MCC_BAND_NV];
}RRC_UE_CAP_MCC_BAND_INFO_STRU;

/* End: add for Report Ue Capability By MCC */

/* boston add begin */
/* V10j0 */
/*********************************************************************
* ASN 文本: multiNS-Pmax-r10
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_MULTI_NS_PMAX_R10_ENUM
{
    RRC_SUPP_MULTI_NS_PMAX_R10                   = 0
};
typedef VOS_UINT8 RRC_MULTI_NS_PMAX_R10_ENUM_UINT8;

/*********************************************************************
* ASN 文本 :RF-Parameters-v10j0
*********************************************************************/
typedef struct
{
    VOS_UINT32          bitMultiNsPmaxR10Present           : 1;
    VOS_UINT32          bitSpare                           : 31;

    RRC_MULTI_NS_PMAX_R10_ENUM_UINT8     enMultiNsPmaxR10;      /* optional */
}RRC_RF_PARAM_V10J0_STRU;

/*********************************************************************
* ASN 文本 :UE-EUTRA-Capability-v10j0-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitRfParamV10j0Present       : 1;
    VOS_UINT32              bitSpare                     : 31;

    RRC_RF_PARAM_V10J0_STRU              RfParamV10j0;      /* optional */
}RRC_UE_EUTRA_CAP_V10J0_IES_STRU;

/* V1260 */
/*****************************************************************************
 枚举名    : RRC_UE_CATEGORY_DL_V1260_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : UE Category Dl V1260
*****************************************************************************/
enum RRC_UE_CATEGORY_DL_V1260_ENUM
{
    RRC_UE_CATEGORY_15                  = 15,
    RRC_UE_CATEGORY_16                  = 16,
    RRC_UE_CATEGORY_V1260_BUTT
};
typedef VOS_UINT8 RRC_UE_CATEGORY_DL_V1260_ENUM_UINT8;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1260_IES_STRU
 协议表格  :
 ASN.1描述 : UU_UE_EUTRA_CAP_V1260_IES_STRU
 结构说明  : RRC_UE_EUTRA_CAP_V1260_IES_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32              bitUeCatgDlV1260Present          : 1;
    VOS_UINT32              bitNonCritiExtPresent            : 1;
    VOS_UINT32              bitSpare                         : 30;

    RRC_UE_CATEGORY_DL_V1260_ENUM_UINT8        enUeCatgDlV1260;    /* optional */  /*(15..16)*/
    VOS_UINT8                                  aucRsv[3];
    /* UU_UE_EUTRA_CAP_V1270_IES_STRU      NonCritiExt; optional */
}RRC_UE_EUTRA_CAP_V1260_IES_STRU;

/* V1270 */
/*********************************************************************
* ASN 文本: fourLayerTM3-TM4-perCC-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_FOUR_LAYER_TM3_TM4_PER_CC_R12_ENUM
{
    RRC_SUPP_FOUR_LAYER_TM3_TM4_PER_CC_R12       = 0
};
typedef VOS_UINT8 RRC_FOUR_LAYER_TM3_TM4_PER_CC_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本: supportedCSI-Proc-r12
* 取值范围: ENUM(0..2)
*********************************************************************/
enum RRC_SUPP_CSI_PROC_R12_ENUM
{
    RRC_N1_SUPP_CSI_PROC_R12                     = 0,
    RRC_N3_SUPP_CSI_PROC_R12                     = 1,
    RRC_N4_SUPP_CSI_PROC_R12                     = 2
};
typedef VOS_UINT8 RRC_SUPP_CSI_PROC_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本 :IntraBandContiguousCC-Info-r12
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitFourLayerTm3Tm4PerCcR12Present           : 1;
    VOS_UINT32              bitSuppMimoCapDlR12Present                  : 1;
    VOS_UINT32              bitSuppCsiProcR12Present                    : 1;
    VOS_UINT32              bitSpare                                    : 29;

    RRC_MIMO_CAP_DL_R10_ENUM_UINT8                  enSuppMimoCapDlR12;           /* optional */
    RRC_SUPP_CSI_PROC_R12_ENUM_UINT8                enSuppCsiProcR12;             /* optional */
    VOS_UINT8                                       aucRev[2];
}RRC_INTRA_BAND_CONTIGUOUS_CC_INFO_R12_STRU;

typedef struct
{
    VOS_UINT16                                    usCnt;    /*(0, 1..UU_MAX_NUM_OF_SERV_CELL_R10)*/
    VOS_UINT8                                     aucRev[2];
    RRC_INTRA_BAND_CONTIGUOUS_CC_INFO_R12_STRU    aItem[RRC_MAX_NUM_OF_SERV_CELL_R10];
}RRC_INTRA_BAND_CONTIGUOUS_CC_INFO_LIST_R12_STRU;

/*********************************************************************
* ASN 文本 :CA-MIMO-ParametersDL-v1270
*********************************************************************/
typedef struct
{
    RRC_INTRA_BAND_CONTIGUOUS_CC_INFO_LIST_R12_STRU    IntraBandContiguousCcInfoListR12;
}RRC_CA_MIMO_PARAM_DL_V1270_STRU;

typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NUM_OF_BANDWIDTH_CLASS_R10)*/
    VOS_UINT8                            aucRev[2];
    RRC_CA_MIMO_PARAM_DL_V1270_STRU      aItem[RRC_MAX_NUM_OF_BANDWIDTH_CLASS_R10];
}RRC_BAND_PARAM_DL_V1270_STRU;

/*********************************************************************
* ASN 文本 :BandParameters-v1270
*********************************************************************/
typedef struct
{
    RRC_BAND_PARAM_DL_V1270_STRU         BandParamDlV1270;
}RRC_BAND_PARAM_V1270_STRU;

typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10)*/
    VOS_UINT8                            aucRev[2];
    RRC_BAND_PARAM_V1270_STRU            aItem[RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10];
}RRC_BAND_PARAM_LIST_V1270_STRU;

/*********************************************************************
* ASN 文本 :BandCombinationParameters-v1270
*********************************************************************/
typedef struct
{
    RRC_BAND_PARAM_LIST_V1270_STRU       BandParamListV1270;           /* optional */
}RRC_BAND_COMB_PARAM_V1270_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandCombination-v1270
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R10)*/
    VOS_UINT8                            aucRev[2];
    RRC_BAND_COMB_PARAM_V1270_STRU       aItem[RRC_MAX_NUM_OF_BAND_COMB_R10];
}RRC_SUPP_BAND_COMB_V1270_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandCombinationAdd-v1270
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R11)*/
    VOS_UINT8                            aucRev[2];
    RRC_BAND_COMB_PARAM_V1270_STRU       aItem[RRC_MAX_NUM_OF_BAND_COMB_R11];
}RRC_SUPP_BAND_COMB_ADD_V1270_STRU;
/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_V1270_NVID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储RRC_SUPP_BAND_COMB_V1270_STRU结构对应的NVID
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BAND_COMB_R10)*/
    VOS_UINT8                            aucRev[2];
    VOS_UINT32                           aulSuppBandCombV1270ParaIdx[RRC_MAX_NUM_OF_BAND_COMB_R10];
}RRC_SUPP_BAND_COMB_V1270_NVID_STRU;
/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_ADD_V1270_NVID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储RRC_SUPP_BAND_COMB_ADD_V1270_STRU结构对应的NVID
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BAND_COMB_R11)*/
    VOS_UINT8                            aucRev[2];
    VOS_UINT32                           aulSuppBandCombAddV1270ParaIdx[RRC_MAX_NUM_OF_BAND_COMB_R11];
}RRC_SUPP_BAND_COMB_ADD_V1270_NVID_STRU;

typedef struct
{
    RRC_BAND_COMB_PARAM_V1270_STRU       stRrcBandCombPrarmV1270;
}RRC_SUPP_BAND_COMB_V1270_PER_NV_STRU;

/*********************************************************************
* ASN 文本 :RF-Parameters-v1270
*********************************************************************/
typedef struct
{
    /* RRC_SUPP_BAND_COMB_V1270_STRU            SuppBandCombV1270; optional */
    /* RRC_SUPP_BAND_COMB_ADD_V1270_STRU        SuppBandCombAddV1270; optional */
    RRC_SUPP_BAND_COMB_V1270_NVID_STRU          stRrcSuppCombV1270NvIdList;
    RRC_SUPP_BAND_COMB_V1270_NVID_STRU          stRrcSuppCombV1270NvIdListExt;
    RRC_SUPP_BAND_COMB_ADD_V1270_NVID_STRU      stRrcSuppCombAddV1270NvIdList;
    RRC_SUPP_BAND_COMB_ADD_V1270_NVID_STRU      stRrcSuppCombAddV1270NvIdListExt;
    RRC_SUPP_BAND_COMB_ADD_V1270_NVID_STRU      stRrcSuppCombAddV1270NvIdListExt1;
    RRC_SUPP_BAND_COMB_ADD_V1270_NVID_STRU      stRrcSuppCombAddV1270NvIdListExt2;
}RRC_RF_PARAM_V1270_STRU;

/*****************************************************************************
 结构名    : RRC_UE_EUTRA_CAP_V1270_IES_STRU
 协议表格  :
 ASN.1描述 : UU_UE_EUTRA_CAP_V1270_IES_STRU
 结构说明  : RRC_UE_EUTRA_CAP_V1270_IES_STRU
*****************************************************************************/
typedef struct
{
    VOS_UINT32              bitRfParamV1270Present                      : 1;
    VOS_UINT32              bitNonCritiExtPresent                       : 1;
    VOS_UINT32              bitSpare                                    : 30;

    RRC_RF_PARAM_V1270_STRU              RfParamV1270;                 /* optional */
    /* UU_UE_EUTRA_CAP_V1280_IES_STRU      NonCritiExt;  optional */
}RRC_UE_EUTRA_CAP_V1270_IES_STRU;

/* V1280 */
/*********************************************************************
* ASN 文本: alternativeTBS-Indices-r12
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_ALTERNATIVE_TBS_INDICES_R12_ENUM
{
    RRC_SUPP_ALTERNATIVE_TBS_INDICES_R12         = 0
};
typedef VOS_UINT8 RRC_ALTERNATIVE_TBS_INDICES_R12_ENUM_UINT8;

/*********************************************************************
* ASN 文本 :PhyLayerParameters-v1280
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitAlternativeTbsIndicesR12Present          : 1;
    VOS_UINT32              bitSpare                                    : 31;

    /* alternativeTBS-Indices-r12 ENUM(0..0) optional */
}RRC_PHY_LAYER_PARAM_V1280_STRU;

/*********************************************************************
* ASN 文本 :UE-EUTRA-Capability-v1280-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitPhyLayerParamV1280Present                : 1;
    VOS_UINT32              bitNonCritiExtPresent                       : 1;
    VOS_UINT32              bitSpare                                    : 30;

    RRC_PHY_LAYER_PARAM_V1280_STRU       PhyLayerParamV1280;           /* optional */
    /* UU_UE_EUTRA_CAP_V1310_IES_STRU      NonCritiExt;  optional */
}RRC_UE_EUTRA_CAP_V1280_IES_STRU;


/* V1310 */
/*********************************************************************
* ASN 文本: ue-CategoryDL-v1310
* 取值范围: ENUM(0..1)
*********************************************************************/
enum RRC_UE_CATG_DL_V1310_ENUM
{
    RRC_N17                                      = 0,
    RRC_M1                                       = 1
};
typedef VOS_UINT8 RRC_UE_CATG_DL_V1310_ENUM_UINT8;

/*********************************************************************
* ASN 文本: ue-CategoryUL-v1310
* 取值范围: ENUM(0..1)
*********************************************************************/
enum RRC_UE_CATG_UL_V1310_ENUM
{
    RRC_N14_UE_CATG_UL_V1310                     = 0,
    RRC_M1_UE_CATG_UL_V1310                      = 1
};
typedef VOS_UINT8 RRC_UE_CATG_UL_V1310_ENUM_UINT8;

/*********************************************************************
* ASN 文本 :PDCP-Parameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitPdcpSnExt18bitsR13Present                : 1;
    VOS_UINT32    bitSpare                                    : 31;

    /* pdcp-SN-Extension-18bits-r13 ENUM(0..0) optional */
}RRC_PDCP_PARAM_V1310_STRU;

/*********************************************************************
* ASN 文本 :RLC-Parameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitExtendedRlcSnSoFieldR13Present           : 1;
    VOS_UINT32    bitSpare                                    : 31;

    /* extendedRLC-SN-SO-Field-r13 ENUM(0..0) optional */
}RRC_RLC_PARAM_V1310_STRU;


/*********************************************************************
* ASN 文本 :MAC-Parameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32      bitExtendedMacLenFieldR13Present            : 1;
    VOS_UINT32      bitExtendedLongDrxR13Present                : 1;
    VOS_UINT32      bitSpare                                    : 30;
}RRC_MAC_PARAM_V1310_STRU;

typedef struct
{
    VOS_UINT32    bitReducedIntNonContCombReqR13Present       : 1;
    VOS_UINT32    bitReqedCcsDlR13Present                     : 1;
    VOS_UINT32    bitReqedCcsUlR13Present                     : 1;
    VOS_UINT32    bitSkipFallbackCombReqR13Present            : 1;
    VOS_UINT32    bitSpare                                    : 28;

    VOS_UINT8                         ucReqedCcsDlR13;              /* optional */    /*(2..32)*/
    VOS_UINT8                         ucReqedCcsUlR13;              /* optional */    /*(2..32)*/
    VOS_UINT8                         aucRsv[2];
}RRC_ENB_REQ_PARAM_R13_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandEUTRA-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitPwrClass20dBmR13Present                  : 1;
    VOS_UINT32    bitSpare                                    : 31;
}RRC_SUPP_BAND_EUTRA_V1310_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandListEUTRA-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BANDS)*/
    VOS_UINT8                            aucRev[2];
    RRC_SUPP_BAND_EUTRA_V1310_STRU       aItem[RRC_MAX_NUM_OF_BANDS];
}RRC_SUPP_BAND_LIST_EUTRA_V1310_STRU;

/*********************************************************************
* ASN 文本: differentFallbackSupported-r13
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_DIFF_FALLBACK_SUPP_R13_ENUM
{
    RRC_TRUE_DIFF_FALLBACK_SUPP_R13              = 0
};
typedef VOS_UINT8 RRC_DIFF_FALLBACK_SUPP_R13_ENUM_UINT8;

/*********************************************************************
* ASN 文本: fourLayerTM3-TM4-r13
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_FOUR_LAYER_TM3_TM4_R13_ENUM
{
    RRC_SUPP_FOUR_LAYER_TM3_TM4_R13              = 0
};
typedef VOS_UINT8 RRC_FOUR_LAYER_TM3_TM4_R13_ENUM_UINT8;

typedef struct
{
    VOS_UINT16              usCnt;    /*(0, 1..RRC_MAX_NUM_OF_SERV_CELL_R13)*/
    VOS_UINT8               aucRev[2];
    RRC_INTRA_BAND_CONTIGUOUS_CC_INFO_R12_STRU    aItem[RRC_MAX_NUM_OF_SERV_CELL_R13];
}RRC_INTRA_BAND_CONTIGUOUS_CC_INFO_LIST_R13_STRU;

/*********************************************************************
* ASN 文本 :BandParametersDL-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitSuppMimoCapDlR13Present                  : 1;
    VOS_UINT32    bitFourLayerTm3Tm4R13Present                : 1;
    VOS_UINT32    bitSpare                                    : 30;

    RRC_CA_BANDWIDTH_CLASS_R10_ENUM_UINT8    enCaBwClassDlR13;
    RRC_MIMO_CAP_DL_R10_ENUM_UINT8       enSuppMimoCapDlR13;           /* optional */
    RRC_FOUR_LAYER_TM3_TM4_R13_ENUM_UINT8    enFourLayerTm3Tm4R13;     /* optional */
    RRC_INTRA_BAND_CONTIGUOUS_CC_INFO_LIST_R13_STRU    IntraBandContiguousCcInfoListR13;
}RRC_BAND_PARAM_DL_R13_STRU;

/*********************************************************************
* ASN 文本: supportedCSI-Proc-r13
* 取值范围: ENUM(0..2)
*********************************************************************/
enum RRC_SUPP_CSI_PROC_R13_ENUM
{
    RRC_N1_SUPP_CSI_PROC_R13                     = 0,
    RRC_N3_SUPP_CSI_PROC_R13                     = 1,
    RRC_N4_SUPP_CSI_PROC_R13                     = 2
};
typedef VOS_UINT8 RRC_SUPP_CSI_PROC_R13_ENUM_UINT8;

/*********************************************************************
* ASN 文本 :BandParameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitBandParamUlR13Present                    : 1;
    VOS_UINT32    bitBandParamDlR13Present                    : 1;
    VOS_UINT32    bitSuppCsiProcR13Present                    : 1;
    VOS_UINT32    bitSpare                                    : 29;

    RRC_FREQ_BAND_IND_R11_UINT16         usBandEutraR13;
    VOS_UINT8                            aucRev[2];
    RRC_CA_MIMO_PARAM_UL_R10_STRU        BandParamUlR13;               /* optional */
    RRC_BAND_PARAM_DL_R13_STRU           BandParamDlR13;               /* optional */
    RRC_SUPP_CSI_PROC_R13_ENUM_UINT8     enSuppCsiProcR13;             /* optional */
}RRC_BAND_PARAM_R13_STRU;

typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10)*/
    VOS_UINT8                            aucRev[2];
    RRC_BAND_PARAM_R13_STRU              aItem[RRC_MAX_NUM_OF_SIMULTANEOUS_BANDS_R10];
}RRC_BAND_PARAM_LIST_R13_STRU;

/*********************************************************************
* ASN 文本: multipleTimingAdvance-r13
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_MULTIPLE_TIMING_ADVANCE_R13_ENUM
{
    RRC_MULTIPLE_TIMING_ADVANCE_R13               = 0
};
typedef VOS_UINT8 RRC_MULTIPLE_TIMING_ADVANCE_R13_ENUM_UINT8;

/*********************************************************************
* ASN 文本: simultaneousRx-Tx-r13
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_SIMUL_RX_TX_R13_ENUM
{
    RRC_SUPP_SIMUL_RX_TX_R13                     = 0
};
typedef VOS_UINT8 RRC_SIMUL_RX_TX_R13_ENUM_UINT8;

/*********************************************************************
* ASN 文本: asynchronous-r13
* 取值范围: ENUM(0..0)
*********************************************************************/
enum RRC_ASYNCHRONOUS_R13_ENUM
{
    RRC_SUPP_ASYNCHRONOUS_R13                    = 0
};
typedef VOS_UINT8 RRC_ASYNCHRONOUS_R13_ENUM_UINT8;


typedef struct
{
    VOS_UINT8             ucChoice;
    /* err code start */
    #define           RRC_THREE_ENTRIES_R13_CHOSEN                      1
    #define           RRC_FOUR_ENTRIES_R13_CHOSEN                       2
    #define           RRC_FIVE_ENTRIES_R13_CHOSEN                       3
    /* err code end */

    union
    {
        VOS_UINT8      ucThreeEntriesR13;
        VOS_UINT8       ucFourEntriesR13;
        VOS_UINT16      usFiveEntriesR13;
    }u;
}RRC_SUPP_CELL_GROUPING_R13_STRU;

typedef struct
{
    VOS_UINT32          bitAsynchronousR13Present           : 1;
    VOS_UINT32          bitSuppCellGroupingR13Present       : 1;
    VOS_UINT32          bitSpare                            : 30;

    RRC_ASYNCHRONOUS_R13_ENUM_UINT8      enAsynchronousR13;            /* optional */
    RRC_SUPP_CELL_GROUPING_R13_STRU      SuppCellGroupingR13;          /* optional */
}RRC_DC_SUPPORT_R13_STRU;

/*********************************************************************
* ASN 文本: supportedNAICS-2CRS-AP-r13
* 取值范围: BIT STRING (SIZE(1..8))
*********************************************************************/
typedef     struct
{
    VOS_UINT16          usBitCnt;  /* 1..(8 * RRC_MAX_NUM_OF_NAICS_ENTRIES_R12) 有效bit位*/
    VOS_UINT8           aucRev[2];
    VOS_UINT8           aucItem[RRC_MAX_NUM_OF_NAICS_ENTRIES_R12];
}RRC_SUPP_NAICS_2CRS_AP_R13_STRU;

/*********************************************************************
* ASN 文本: commSupportedBandsPerBC-r13
* 取值范围: BIT STRING (SIZE(1..64))
*********************************************************************/
typedef struct
{
    VOS_UINT16         usBitCnt;  /* 1..(8 * RRC_MAX_BIT_LEN_OF_BANDS) 有效bit位*/
    VOS_UINT8          aucRev[2];
    VOS_UINT8          aucItem[RRC_MAX_BIT_LEN_OF_BANDS];
}RRC_COMM_SUPP_BANDS_PER_BC_R13_STRU;

/*********************************************************************
* ASN 文本 :BandCombinationParameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32          bitDiffFallbackSuppR13Present               : 1;
    VOS_UINT32          bitSuppBwCombSetR13Present                  : 1;
    VOS_UINT32          bitMultipleTimingAdvanceR13Present          : 1;
    VOS_UINT32          bitSimulRxTxR13Present                      : 1;
    VOS_UINT32          bitDcSupportR13Present                      : 1;
    VOS_UINT32          bitSuppNaics2crsApR13Present                : 1;
    VOS_UINT32          bitCommSuppBandsPerBcR13Present             : 1;
    VOS_UINT32          bitSpare                                    : 25;

    RRC_DIFF_FALLBACK_SUPP_R13_ENUM_UINT8    enDiffFallbackSuppR13;    /* optional */
    RRC_MULTIPLE_TIMING_ADVANCE_R13_ENUM_UINT8    enMultipleTimingAdvanceR13;    /* optional */
    RRC_SIMUL_RX_TX_R13_ENUM_UINT8           enSimulRxTxR13;               /* optional */
    VOS_UINT8                                aucRsv[1];
    RRC_BAND_PARAM_LIST_R13_STRU             BandParamListR13;
    RRC_SUPP_BANDWIDTH_COMB_SET_R10_STRU     SuppBwCombSetR13;             /* optional */
    RRC_BAND_INFO_EUTRA_STRU                 BandInfoEutraR13;
    RRC_DC_SUPPORT_R13_STRU                  DcSupportR13;                 /* optional */
    RRC_SUPP_NAICS_2CRS_AP_R13_STRU          SuppNaics2crsApR13;           /* optional */
    RRC_COMM_SUPP_BANDS_PER_BC_R13_STRU      CommSuppBandsPerBcR13;        /* optional */
}RRC_BAND_COMB_PARAM_R13_STRU;

/*********************************************************************
* ASN 文本 :SupportedBandCombinationReduced-r13
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..UU_MAX_NUM_OF_BAND_COMB_R13)*/
    VOS_UINT8                            aucRev[2];
    RRC_BAND_COMB_PARAM_R13_STRU         aItem[RRC_MAX_NUM_OF_BAND_COMB_R13];
}RRC_SUPP_BAND_COMB_REDUCED_R13_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储RRC_SUPP_BAND_COMB_REDUCED_R13_STRU结构对应的NVID
*********************************************************************/
typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_BAND_COMB_R13)*/
    VOS_UINT8                            aucRev[2];
    VOS_UINT32                           aulSuppBandCombReducedR13ParaIdx[RRC_MAX_NUM_OF_BAND_COMB_R13];
}RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU;


/***************************************************************************
 结构名    : RRC_RF_PARAM_V1310_SUPP_BAND_COMB_REDUCED_R13_NVID_LIST_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储RF_PARAM_V1310中SUPP_BAND_COMB_REDUCED_R13的NVID列表
***************************************************************************/
typedef struct
{
    RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU          stRrcSuppCombReducedR13NvIdList;
    RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU          stRrcSuppCombReducedR13NvIdListExt;
    RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU          stRrcSuppCombReducedR13NvIdListExt1;
    RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU          stRrcSuppCombReducedR13NvIdListExt2;
    RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU          stRrcSuppCombReducedR13NvIdListExt3;
    RRC_SUPP_BAND_COMB_REDUCED_R13_NVID_STRU          stRrcSuppCombReducedR13NvIdListExt4;
}RRC_RF_PARAM_V1310_SUPP_BAND_COMB_REDUCED_R13_NVID_LIST_STRU;

/*********************************************************************
 结构名    : RRC_SUPP_BAND_COMB_REDUCED_R13_PER_NV_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : 存储每个NV对应的SUPP_BAND_COMB_REDUCED_R13
*********************************************************************/
typedef struct
{
    RRC_BAND_COMB_PARAM_R13_STRU       stRrcBandCombPrarmR13;
}RRC_SUPP_BAND_COMB_REDUCED_R13_PER_NV_STRU;


/*********************************************************************
* ASN 文本 :RF-Parameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32      bitEnbReqParamR13Present                    : 1;
    VOS_UINT32      bitMaxCcsRetrievalR13Present                : 1;
    VOS_UINT32      bitSkipFallbackCombinationsR13Present       : 1;
    VOS_UINT32      bitReducedIntNonContCombR13Present          : 1;
    VOS_UINT32      bitSpare                                    : 28;

    RRC_ENB_REQ_PARAM_R13_STRU           EnbReqParamR13;               /* optional */
    RRC_SUPP_BAND_LIST_EUTRA_V1310_STRU  SuppBandListEutraV1310;       /* optional */
    /* RRC_SUPP_BAND_COMB_REDUCED_R13_STRU  SuppBandCombReducedR13;  optional */
    /* 存储SUPP_BAND_COMB_REDUCED_R13的nv列表对应的NV */
    VOS_UINT32                           ulNvIdOfSuppBandCombReducedR13NvList;    /* 由于共有384个NV，导致结构大小超过2048，因此用一个NV来储存NV列表  */
    /* RRC_SUPP_BAND_COMB_REDUCED_R13_STRU  SuppBandCombReducedR13;  optional */
}RRC_RF_PARAM_V1310_STRU;


/*********************************************************************
* ASN 文本 :MeasParameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitRsSinrMeasR13Present                     : 1;
    VOS_UINT32    bitWhiteCellListR13Present                  : 1;
    VOS_UINT32    bitExtendedMaxObjIdR13Present               : 1;
    VOS_UINT32    bitUlPdcpDelayR13Present                    : 1;
    VOS_UINT32    bitExtendedFreqPrioritiesR13Present         : 1;
    VOS_UINT32    bitMultiBandInfoRprtR13Present              : 1;
    VOS_UINT32    bitRssiAndChOccupancyRprtingR13Present      : 1;
    VOS_UINT32    bitSpare                                    : 25;
}RRC_MEAS_PARAM_V1310_STRU;


/*********************************************************************
* ASN 文本 :DC-Parameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitPdcpTransfSplitUlR13Present              : 1;
    VOS_UINT32    bitUeSstdMeasR13Present                     : 1;
    VOS_UINT32    bitSpare                                    : 30;
}RRC_DC_PARAM_V1310_STRU;

/*********************************************************************
* ASN 文本 :SL-Parameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitDiscSysInfoRprtingR13Present             : 1;
    VOS_UINT32    bitCommMultipleTxR13Present                 : 1;
    VOS_UINT32    bitDiscInterFreqTxR13Present                : 1;
    VOS_UINT32    bitDiscPerioSlssR13Present                  : 1;
    VOS_UINT32    bitSpare                                    : 28;
}RRC_SL_PARAM_V1310_STRU;

/*********************************************************************
* ASN 文本 :SCPTM-Parameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitScptmParallelReceptionR13Present         : 1;
    VOS_UINT32    bitScptmScellR13Present                     : 1;
    VOS_UINT32    bitScptmNonServingCellR13Present            : 1;
    VOS_UINT32    bitScptmAsyncDcR13Present                   : 1;
    VOS_UINT32    bitSpare                                    : 28;
}RRC_SCPTM_PARAM_R13_STRU;

/*********************************************************************
* ASN 文本 :MTC-Parameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32          bitCeModeaR13Present          : 1;
    VOS_UINT32          bitCeModebR13Present          : 1;
    VOS_UINT32          bitSpare                      : 30;
}RRC_MTC_PARAM_R13_STRU;

/*********************************************************************
* ASN 文本: WLAN-BandIndicator-r13
* 取值范围: ENUM(0..7)
*********************************************************************/
enum RRC_WLAN_BAND_IND_R13_ENUM
{
    RRC_BAND2DOT4_WLAN_BAND_IND_R13              = 0,
    RRC_BAND5_WLAN_BAND_IND_R13                  = 1,
    RRC_SPARE6_WLAN_BAND_IND_R13                 = 2,
    RRC_SPARE5_WLAN_BAND_IND_R13                 = 3,
    RRC_SPARE4_WLAN_BAND_IND_R13                 = 4,
    RRC_SPARE3_WLAN_BAND_IND_R13                 = 5,
    RRC_SPARE2_WLAN_BAND_IND_R13                 = 6,
    RRC_SPARE1_WLAN_BAND_IND_R13                 = 7
};
typedef VOS_UINT8 RRC_WLAN_BAND_IND_R13_ENUM_UINT8;


typedef struct
{
    VOS_UINT16                           usCnt;    /*(0, 1..RRC_MAX_NUM_OF_WLAN_BANDS_R13)*/
    VOS_UINT8                            aucRev[1];
    RRC_WLAN_BAND_IND_R13_ENUM_UINT8     aenItem[RRC_MAX_NUM_OF_WLAN_BANDS_R13];
}RRC_SUPP_BAND_LIST_WLAN_R13_STRU;

/*********************************************************************
* ASN 文本 :IRAT-ParametersWLAN-r13
*********************************************************************/
typedef struct
{
    RRC_SUPP_BAND_LIST_WLAN_R13_STRU     SuppBandListWlanR13;          /* optional */
}RRC_IRAT_PARAM_WLAN_R13_STRU;

/*********************************************************************
* ASN 文本 :LAA-Parameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitCrossCarrierSchdlLaaDlR13Present         : 1;
    VOS_UINT32    bitCsiRsDrsRrmMeasLaaR13Present             : 1;
    VOS_UINT32    bitDllaaR13Present                          : 1;
    VOS_UINT32    bitEndingDwPtsR13Present                    : 1;
    VOS_UINT32    bitSecondSlotStartingPosR13Present          : 1;
    VOS_UINT32    bitTm9LaaR13Present                         : 1;
    VOS_UINT32    bitTm10LaaR13Present                        : 1;
    VOS_UINT32    bitSpare                                    : 25;

}RRC_LAA_PARAM_R13_STRU;

/*********************************************************************
* ASN 文本: wlan-MAC-Address-r13
* 取值范围: OCTET STRING (SIZE(6))
*********************************************************************/
typedef struct
{
    VOS_UINT16          usLen;/*(6..RRC_MAX_BIT_LEN_OF_WLAN_MAC_ADDRESS_R13)*/
    VOS_UINT8           aucItem[RRC_MAX_BIT_LEN_OF_WLAN_MAC_ADDRESS_R13];
}RRC_WLAN_MAC_ADDR_R13_STRU;

/*********************************************************************
* ASN 文本 :LWA-Parameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32          bitLwaR13Present                : 1;
    VOS_UINT32          bitLwaSplitBearerR13Present     : 1;
    VOS_UINT32          bitWlanMacAddrR13Present        : 1;
    VOS_UINT32          bitLwaBufSizeR13Present         : 1;
    VOS_UINT32          bitSpare                        : 28;

    RRC_WLAN_MAC_ADDR_R13_STRU           WlanMacAddrR13;               /* optional */
}RRC_LWA_PARAM_R13_STRU;

/*********************************************************************
* ASN 文本 :WLAN-IW-Parameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitRclwiR13Present          : 1;
    VOS_UINT32              bitSpare                    : 31;

    /* rclwi-r13 ENUM(0..0) optional */
}RRC_WLAN_IW_PARAM_V1310_STRU;

/*********************************************************************
* ASN 文本 :LWIP-Parameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitLwipR13Present           : 1;
    VOS_UINT32              bitSpare                    : 31;

    /* lwip-r13 ENUM(0..0) optional */
}RRC_LWIP_PARAM_R13_STRU;

typedef struct
{
    VOS_UINT32              bitMaxNumDecodingR13Present                 : 1;
    VOS_UINT32              bitPdcchCandidateReductionsR13Present       : 1;
    VOS_UINT32              bitSkipMonitoringDciFormat01aR13Present     : 1;
    VOS_UINT32              bitSpare                                    : 29;

    VOS_UINT8                         ucMaxNumDecodingR13;          /* optional (1..32)*/
    VOS_UINT8                         aucRev[3];
}RRC_SUPP_BLIND_DECODING_R13_STRU;

/*********************************************************************
* ASN 文本 :PhyLayerParameters-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32          bitAperiodicCsiRprtingR13Present            : 1;
    VOS_UINT32          bitCodebookHarqAckR13Present                : 1;
    VOS_UINT32          bitCrossCarrierSchdlB5cR13Present           : 1;
    VOS_UINT32          bitFddHarqTimingTddR13Present               : 1;
    VOS_UINT32          bitMaxNumUpdatedCsiProcR13Present           : 1;
    VOS_UINT32          bitPucchFormat4R13Present                   : 1;
    VOS_UINT32          bitPucchFormat5R13Present                   : 1;
    VOS_UINT32          bitPucchScellR13Present                     : 1;
    VOS_UINT32          bitSpatialBundlingHarqAckR13Present         : 1;
    VOS_UINT32          bitSuppBlindDecodingR13Present              : 1;
    VOS_UINT32          bitUciPuschExtR13Present                    : 1;
    VOS_UINT32          bitCrsInterfMitigationTm10R13Present        : 1;
    VOS_UINT32          bitPdschCollisionHandlR13Present            : 1;
    VOS_UINT32          bitSpare                                    : 19;

    VOS_UINT8                                       ucAperiodicCsiRprtingR13;     /* optional */
    VOS_UINT8                                       ucCodebookHarqAckR13;         /* optional */
    VOS_UINT8                                       ucMaxNumUpdatedCsiProcR13;    /* optional (5..32)*/
    VOS_UINT8                                       aucRev[1];
    RRC_SUPP_BLIND_DECODING_R13_STRU                SuppBlindDecodingR13;         /* optional */
}RRC_PHY_LAYER_PARAM_V1310_STRU;

/*********************************************************************
* ASN 文本 :UE-EUTRA-CapabilityAddXDD-Mode-v1310
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitPhyLayerParamV1310Present                : 1;
    VOS_UINT32              bitSpare                                    : 31;

    RRC_PHY_LAYER_PARAM_V1310_STRU       PhyLayerParamV1310;           /* optional */
}RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1310_STRU;

/* begin boston dl cat 18 add */
/*********************************************************************
* ASN 文本 :PhyLayerParameters-v1330
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitCchInterfMitigationRefRecTypeaR13Present : 1;
    VOS_UINT32    bitCchInterfMitigationRefRecTypebR13Present : 1;
    VOS_UINT32    bitCchInterfMitigationMaxNumCcsR13Present   : 1;
    VOS_UINT32    bitCrsInterfMitigationTm1toTm9R13Present    : 1;
    VOS_UINT32    bitSpare                                    : 28;

    VOS_UINT8               ucCchInterfMitigationMaxNumCcsR13;    /* optional */    /*(1..32)*/
    VOS_UINT8               ucCrsInterfMitigationTm1toTm9R13;    /* optional */    /*(1..32)*/
    VOS_UINT8               aucRsv[2];
}RRC_PHY_LAYER_PARAM_V1330_STRU;

/*********************************************************************
* ASN 文本 :UE-EUTRA-Capability-v1330-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitUeCatgDlV1330Present                     : 1;
    VOS_UINT32    bitPhyLayerParamV1330Present                : 1;
    VOS_UINT32    bitUeCeNeedUlGapsR13Present                 : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 28;

    VOS_UINT8              ucUeCatgDlV1330;    /* optional */    /*(18..19)*/
    VOS_UINT8              aucRsv[3];
    RRC_PHY_LAYER_PARAM_V1330_STRU       PhyLayerParamV1330;           /* optional */
    /* UU_UE_EUTRA_CAP_V1340_IES_STRU      NonCritiExt;      optional */
}RRC_UE_EUTRA_CAP_V1330_IES_STRU;

/*********************************************************************
* ASN 文本 :MIMO-NonPrecodedCapabilities-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitConfig1R13Present                        : 1;
    VOS_UINT32    bitConfig2R13Present                        : 1;
    VOS_UINT32    bitConfig3R13Present                        : 1;
    VOS_UINT32    bitConfig4R13Present                        : 1;
    VOS_UINT32    bitSpare                                    : 28;
}RRC_MIMO_NON_PRECODED_CAPABILITIES_R13_STRU;

/*********************************************************************
* ASN 文本 : n-MaxList-r13
*********************************************************************/
typedef     struct
{
    VOS_UINT16         usBitCnt;  /* 1..(8 * UU_MAX_BIT_LEN_OF_N_MAX_LIST) ???bit? */
    VOS_UINT8          aucItem[RRC_MAX_BIT_LEN_OF_N_MAX_LIST];
    VOS_UINT8          aucRsv[1];
}RRC_N_MAX_LIST_R13_STRU;

/*********************************************************************
* ASN 文本 :MIMO-BeamformedCapabilities-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitNMaxListR13Present                       : 1;
    VOS_UINT32    bitSpare                                    : 31;

    VOS_UINT8               ucKMaxR13;      /* (1..8) */
    VOS_UINT8               aucRsv[3];
    RRC_N_MAX_LIST_R13_STRU              NMaxListR13;         /* optional */
}RRC_MIMO_BEAMFORMED_CAPABILITIES_R13_STRU;

/*********************************************************************
* ASN 文本 :MIMO-BeamformedCapabilityList-r13
*********************************************************************/
typedef struct
{
    VOS_UINT16                         usCnt;    /*(0, 1..UU_MAX_NUM_CSI_PROCESS_RELEASELIST_R11)*/
    VOS_UINT8                          aucRsv[2];
    RRC_MIMO_BEAMFORMED_CAPABILITIES_R13_STRU    aItem[RRC_MAX_NUM_CSI_PROCESS_RELEASELIST_R11];
}RRC_MIMO_BEAMFORMED_CAP_LIST_R13_STRU;

/*********************************************************************
* ASN 文本 :MIMO-UE-BeamformedCapabilities-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitAltCodebookR13Present                    : 1;
    VOS_UINT32    bitSpare                                    : 31;

    RRC_MIMO_BEAMFORMED_CAP_LIST_R13_STRU    MimoBeamformedCapabilitiesR13;
}RRC_MIMO_UE_BEAMFORMED_CAPABILITIES_R13_STRU;

/*********************************************************************
* ASN 文本 :MIMO-UE-ParametersPerTM-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitNonPrecodedR13Present                    : 1;
    VOS_UINT32    bitBeamformedR13Present                     : 1;
    VOS_UINT32    bitChMeasRstricR13Present                   : 1;
    VOS_UINT32    bitDmrsEnhancementsR13Present               : 1;
    VOS_UINT32    bitCsiRsEnhancementsTddR13Present           : 1;
    VOS_UINT32    bitSpare                                    : 27;

    RRC_MIMO_NON_PRECODED_CAPABILITIES_R13_STRU    NonPrecodedR13;     /* optional */
    RRC_MIMO_UE_BEAMFORMED_CAPABILITIES_R13_STRU   BeamformedR13;     /* optional */
}RRC_MIMO_UE_PARAM_PER_TM_R13_STRU;

/*********************************************************************
* ASN 文本 :MIMO-UE-Parameters-r13
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitParamsTm9R13Present                      : 1;
    VOS_UINT32    bitParamsTm10R13Present                     : 1;
    VOS_UINT32    bitSrsEnhancementsTddR13Present             : 1;
    VOS_UINT32    bitSrsEnhancementsR13Present                : 1;
    VOS_UINT32    bitInterferenceMeasRstricR13Present         : 1;
    VOS_UINT32    bitSpare                                    : 27;

    RRC_MIMO_UE_PARAM_PER_TM_R13_STRU    ParamsTm9R13;                 /* optional */
    RRC_MIMO_UE_PARAM_PER_TM_R13_STRU    ParamsTm10R13;                /* optional */
}RRC_MIMO_UE_PARAM_R13_STRU;

/*********************************************************************
* ASN 文本 :PhyLayerParameters-v1320
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitMimoUeParamR13Present                    : 1;
    VOS_UINT32    bitSpare                                    : 31;

    RRC_MIMO_UE_PARAM_R13_STRU           MimoUeParamR13;               /* optional */
}RRC_PHY_LAYER_PARAM_V1320_STRU;

/*********************************************************************
* ASN 文本 :CE-Parameters-v1320
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitIntraFreqA3CeModeaR13Present             : 1;
    VOS_UINT32    bitIntraFreqA3CeModebR13Present             : 1;
    VOS_UINT32    bitIntraFreqHoCeModeaR13Present             : 1;
    VOS_UINT32    bitIntraFreqHoCeModebR13Present             : 1;
    VOS_UINT32    bitSpare                                    : 28;
}RRC_CE_PARAM_V1320_STRU;

/*********************************************************************
* ASN 文本 :UE-EUTRA-Capability-v1320-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32    bitCeParamV1320Present                      : 1;
    VOS_UINT32    bitPhyLayerParamV1320Present                : 1;
    VOS_UINT32    bitRfParamV1320Present                      : 1;
    VOS_UINT32    bitFddAddUeEutraCapabilitiesV1320Present    : 1;
    VOS_UINT32    bitTddAddUeEutraCapabilitiesV1320Present    : 1;
    VOS_UINT32    bitNonCritiExtPresent                       : 1;
    VOS_UINT32    bitSpare                                    : 26;

    RRC_CE_PARAM_V1320_STRU              CeParamV1320;        /* optional */
    RRC_PHY_LAYER_PARAM_V1320_STRU       PhyLayerParamV1320;           /* optional */
    /* RRC_UE_EUTRA_CAP_V1330_IES_STRU      NonCritiExt; optional */
}RRC_UE_EUTRA_CAP_V1320_IES_STRU;
/* end boston dl cat 18 add */

/*********************************************************************
* ASN 文本 :UE-EUTRA-Capability-v1310-IEs
*********************************************************************/
typedef struct
{
    VOS_UINT32              bitUeCatgDlV1310Present                     : 1;
    VOS_UINT32              bitUeCatgUlV1310Present                     : 1;
    VOS_UINT32              bitMacParamV1310Present                     : 1;
    VOS_UINT32              bitPhyLayerParamV1310Present                : 1;
    VOS_UINT32              bitRfParamV1310Present                      : 1;
    VOS_UINT32              bitMeasParamV1310Present                    : 1;
    VOS_UINT32              bitDcParamV1310Present                      : 1;
    VOS_UINT32              bitSlParamV1310Present                      : 1;
    VOS_UINT32              bitScptmParamR13Present                     : 1;
    VOS_UINT32              bitMtcParamR13Present                       : 1;
    VOS_UINT32              bitLaaParamR13Present                       : 1;
    VOS_UINT32              bitLwaParamR13Present                       : 1;
    VOS_UINT32              bitFddAddUeEutraCapabilitiesV1310Present    : 1;
    VOS_UINT32              bitTddAddUeEutraCapabilitiesV1310Present    : 1;
    VOS_UINT32              bitNonCritiExtPresent                       : 1;
    VOS_UINT32              bitSpare                                    : 17;

    RRC_UE_CATG_DL_V1310_ENUM_UINT8      enUeCatgDlV1310;              /* optional */
    RRC_UE_CATG_UL_V1310_ENUM_UINT8      enUeCatgUlV1310;              /* optional */
    VOS_UINT8                            aucRev[2];
    RRC_PDCP_PARAM_V1310_STRU            PdcpParamV1310;
    RRC_RLC_PARAM_V1310_STRU             RlcParamV1310;
    RRC_MAC_PARAM_V1310_STRU             MacParamV1310;                /* optional */
    RRC_PHY_LAYER_PARAM_V1310_STRU       PhyLayerParamV1310;           /* optional */
    RRC_RF_PARAM_V1310_STRU              RfParamV1310;                 /* optional */
    RRC_MEAS_PARAM_V1310_STRU            MeasParamV1310;               /* optional */
    RRC_DC_PARAM_V1310_STRU              DcParamV1310;                 /* optional */
    RRC_SL_PARAM_V1310_STRU              SlParamV1310;                 /* optional */
    RRC_SCPTM_PARAM_R13_STRU             ScptmParamR13;                /* optional */
    RRC_MTC_PARAM_R13_STRU               MtcParamR13;                  /* optional */
    RRC_IRAT_PARAM_WLAN_R13_STRU         InterRatParamWlanR13;
    RRC_LAA_PARAM_R13_STRU               LaaParamR13;                  /* optional */
    RRC_LWA_PARAM_R13_STRU               LwaParamR13;                  /* optional */
    RRC_WLAN_IW_PARAM_V1310_STRU         WlanIwParamV1310;
    RRC_LWIP_PARAM_R13_STRU              LwipParamR13;
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1310_STRU    FddAddUeEutraCapabilitiesV1310;    /* optional */
    RRC_UE_EUTRA_CAP_ADD_XDD_MODE_V1310_STRU    TddAddUeEutraCapabilitiesV1310;    /* optional */
    /* RRC_UE_EUTRA_CAP_V1320_IES_STRU      NonCritiExt;              optional */
}RRC_UE_EUTRA_CAP_V1310_IES_STRU;
/* boston add end */


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

#endif /* end of RrcNvInterface.h */
