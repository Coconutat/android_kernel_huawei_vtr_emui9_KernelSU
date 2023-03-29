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

#ifndef  AT_IMSA_INTERFACE_H
#define  AT_IMSA_INTERFACE_H

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "vos.h"
#include "TafTypeDef.h"

#include "TafApsApi.h"
#include "AtMnInterface.h"
#include "MnCallApi.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

#define AT_IMSA_IMPU_MAX_LENGTH        (128)
#define AT_IMSA_IMPI_MAX_LENGTH        (128)
#define AT_IMSA_DOMAIN_MAX_LENGTH      (128)


/* equals IMSA_MAX_CALL_NUMBER_LENGTH */
#define AT_IMSA_CALL_ASCII_NUM_MAX_LENGTH     (40)

#define IMSA_PHONECONTEXT_MAX_LENGTH     (128)

#define IMSA_PUBLICEUSERID_MAX_LENGTH    (128)
#define AT_IMSA_IPV4_ADDR_LEN              (4)

#define AT_IMSA_IPV6_ADDR_LEN              (16)


#define IMSA_IMS_STRING_LENGTH           (128)
#define AT_IMSA_MAX_SMSPSI_LEN           (128)

#define IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN      (0x0100)

/* 当前ims注册失败网侧上报最大字符串长度为255 */
#define IMSA_AT_REG_FAIL_CAUSE_STR_MAX_LEN              (256)

#define AT_IMSA_USER_AGENT_STR_LEN          (16)

/*****************************************************************************
  2 枚举定义
*****************************************************************************/

enum AT_IMSA_MSG_TYPE_ENUM
{
    /* AT->IMSA */
    ID_AT_IMSA_CIREG_SET_REQ                = 0x0001,                           /* _H2ASN_MsgChoice AT_IMSA_CIREG_SET_REQ_STRU */
    ID_AT_IMSA_CIREG_QRY_REQ                = 0x0002,                           /* _H2ASN_MsgChoice AT_IMSA_CIREG_QRY_REQ_STRU */
    ID_AT_IMSA_CIREP_SET_REQ                = 0x0003,                           /* _H2ASN_MsgChoice AT_IMSA_CIREP_SET_REQ_STRU */
    ID_AT_IMSA_CIREP_QRY_REQ                = 0x0004,                           /* _H2ASN_MsgChoice AT_IMSA_CIREP_QRY_REQ_STRU */
    ID_AT_IMSA_VOLTEIMPU_QRY_REQ            = 0x0005,                           /* _H2ASN_MsgChoice AT_IMSA_VOLTEIMPU_QRY_REQ_STRU */



    ID_AT_IMSA_IMS_REG_DOMAIN_QRY_REQ       = 0x0006,                           /* _H2ASN_MsgChoice AT_IMSA_IMS_REG_DOMAIN_QRY_REQ_STRU */
    ID_AT_IMSA_IMS_CTRL_MSG                 = 0x0007,                           /* _H2ASN_MsgChoice AT_IMSA_IMS_CTRL_MSG_STRU */

    ID_AT_IMSA_CALL_ENCRYPT_SET_REQ         = 0x0008,                           /* _H2ASN_MsgChoice AT_IMSA_CALL_ENCRYPT_SET_REQ_STRU */

    ID_AT_IMSA_ROAMING_IMS_QRY_REQ          = 0x0009,                           /* _H2ASN_MsgChoice AT_IMSA_ROAMING_IMS_QRY_REQ_STRU */

    ID_AT_IMSA_PCSCF_SET_REQ                = 0x000A,                           /* _H2ASN_MsgChoice AT_IMSA_PCSCF_SET_REQ_STRU */
    ID_AT_IMSA_PCSCF_QRY_REQ                = 0x000B,                           /* _H2ASN_MsgChoice AT_IMSA_PCSCF_QRY_REQ_STRU */
    ID_AT_IMSA_DMDYN_SET_REQ                = 0x000C,                           /* _H2ASN_MsgChoice AT_IMSA_DMDYN_SET_REQ_STRU */
    ID_AT_IMSA_DMDYN_QRY_REQ                = 0x000D,                           /* _H2ASN_MsgChoice AT_IMSA_DMDYN_QRY_REQ_STRU */

    ID_AT_IMSA_IMSTIMER_SET_REQ             = 0x000E,                           /* _H2ASN_MsgChoice AT_IMSA_IMSTIMER_SET_REQ_STRU */
    ID_AT_IMSA_IMSTIMER_QRY_REQ             = 0x000F,                           /* _H2ASN_MsgChoice AT_IMSA_IMSTIMER_QRY_REQ_STRU */
    ID_AT_IMSA_SMSPSI_SET_REQ               = 0x0010,                           /* _H2ASN_MsgChoice AT_IMSA_SMSPSI_SET_REQ_STRU */
    ID_AT_IMSA_SMSPSI_QRY_REQ               = 0x0011,                           /* _H2ASN_MsgChoice AT_IMSA_SMSPSI_QRY_REQ_STRU */
    ID_AT_IMSA_DMUSER_QRY_REQ               = 0x0012,

    ID_AT_IMSA_NICKNAME_SET_REQ             = 0x0013,                           /* _H2ASN_MsgChoice AT_IMSA_NICKNAME_SET_REQ_STRU */
    ID_AT_IMSA_NICKNAME_QRY_REQ             = 0x0014,                           /* _H2ASN_MsgChoice AT_IMSA_NICKNAME_QRY_REQ_STRU */
    ID_AT_IMSA_BATTERYINFO_SET_REQ          = 0x0015,                           /* _H2ASN_MsgChoice AT_IMSA_BATTERY_INFO_SET_REQ_STRU */
    ID_AT_IMSA_VOLTEREG_NTF                 = 0x0016,                           /* _H2ASN_MsgChoice AT_IMSA_VOLTEREG_NTF_STRU */

    ID_AT_IMSA_CANCEL_ADD_VIDEO_REQ         = 0x0017,                           /* _H2ASN_MsgChoice AT_IMSA_CANCEL_ADD_VIDEO_REQ_STRU */

    ID_AT_IMSA_VOLTEIMPI_QRY_REQ            = 0x0018,                                               /* _H2ASN_MsgChoice AT_IMSA_VOLTEIMPI_QRY_REQ_STRU */
    ID_AT_IMSA_VOLTEDOMAIN_QRY_REQ          = 0x0019,                                             /* _H2ASN_MsgChoice AT_IMSA_VOLTEDOMAIN_QRY_REQ_STRU */
    ID_AT_IMSA_REGERR_REPORT_SET_REQ        = 0x001A,                           /* _H2ASN_MsgChoice AT_IMSA_REGERR_REPORT_SET_REQ_STRU*/
    ID_AT_IMSA_REGERR_REPORT_QRY_REQ        = 0x001B,                           /* _H2ASN_MsgChoice AT_IMSA_REGERR_REPORT_QRY_REQ_STRU*/

    ID_AT_IMSA_IMS_IP_CAP_SET_REQ           = 0x001C,
    ID_AT_IMSA_IMS_IP_CAP_QRY_REQ           = 0x001D,

    ID_AT_IMSA_USER_AGENT_CFG_SET_REQ      = 0x0023,

    /* IMSA->AT */
    ID_IMSA_AT_CIREG_SET_CNF                = 0x1001,                           /* _H2ASN_MsgChoice IMSA_AT_CIREG_SET_CNF_STRU */
    ID_IMSA_AT_CIREG_QRY_CNF                = 0x1002,                           /* _H2ASN_MsgChoice IMSA_AT_CIREG_QRY_CNF_STRU */
    ID_IMSA_AT_CIREP_SET_CNF                = 0x1003,                           /* _H2ASN_MsgChoice IMSA_AT_CIREP_SET_CNF_STRU */
    ID_IMSA_AT_CIREP_QRY_CNF                = 0x1004,                           /* _H2ASN_MsgChoice IMSA_AT_CIREP_QRY_CNF_STRU */

    ID_IMSA_AT_VOLTEIMPU_QRY_CNF            = 0x1005,                           /* _H2ASN_MsgChoice IMSA_AT_VOLTEIMPU_QRY_CNF_STRU */




    ID_IMSA_AT_CIREGU_IND                   = 0x1006,                           /* _H2ASN_MsgChoice IMSA_AT_CIREGU_IND_STRU */
    ID_IMSA_AT_CIREPH_IND                   = 0x1007,                           /* _H2ASN_MsgChoice IMSA_AT_CIREPH_IND_STRU */
    ID_IMSA_AT_CIREPI_IND                   = 0x1008,                           /* _H2ASN_MsgChoice IMSA_AT_CIREPI_IND_STRU */

    ID_IMSA_AT_VT_PDP_ACTIVATE_IND          = 0x1009,                           /* _H2ASN_MsgChoice IMSA_AT_VT_PDP_ACTIVATE_IND_STRU */
    ID_IMSA_AT_VT_PDP_DEACTIVATE_IND        = 0x100A,                           /* _H2ASN_MsgChoice IMSA_AT_VT_PDP_DEACTIVATE_IND_STRU */

    ID_IMSA_AT_MT_STATES_IND                = 0x100B,                           /* _H2ASN_MsgChoice IMSA_AT_MT_STATES_IND_STRU */

    ID_IMSA_AT_IMS_REG_DOMAIN_QRY_CNF       = 0x100C,                           /* _H2ASN_MsgChoice IMSA_AT_IMS_REG_DOMAIN_QRY_CNF_STRU */
    ID_IMSA_AT_IMS_CTRL_MSG                 = 0x100D,                           /* _H2ASN_MsgChoice IMSA_AT_IMS_CTRL_MSG_STRU */

    ID_IMSA_AT_CALL_ENCRYPT_SET_CNF         = 0x100E,                           /* _H2ASN_MsgChoice IMSA_AT_CALL_ENCRYPT_SET_CNF_STRU */

    ID_IMSA_AT_ROAMING_IMS_QRY_CNF          = 0x100F,                           /* _H2ASN_MsgChoice IMSA_AT_ROAMING_IMS_QRY_CNF_STRU */

    ID_IMSA_AT_IMS_RAT_HANDOVER_IND         = 0x1010,                           /* _H2ASN_MsgChoice IMSA_AT_IMS_RAT_HANDOVER_IND_STRU */
    ID_IMSA_AT_IMS_SRV_STATUS_UPDATE_IND    = 0x1011,                           /* _H2ASN_MsgChoice IMSA_AT_IMS_SRV_STATUS_UPDATE_IND_STRU */

    ID_IMSA_AT_PCSCF_SET_CNF                = 0x1012,                           /* _H2ASN_MsgChoice IMSA_AT_PCSCF_SET_CNF_STRU */
    ID_IMSA_AT_PCSCF_QRY_CNF                = 0x1013,                           /* _H2ASN_MsgChoice IMSA_AT_PCSCF_QRY_CNF_STRU */

    ID_IMSA_AT_DMDYN_SET_CNF                = 0x1014,                           /* _H2ASN_MsgChoice IMSA_AT_DMDYN_SET_CNF_STRU */
    ID_IMSA_AT_DMDYN_QRY_CNF                = 0x1015,                           /* _H2ASN_MsgChoice IMSA_AT_DMDYN_QRY_CNF_STRU */

    ID_IMSA_AT_DMCN_IND                     = 0x1016,                           /* _H2ASN_MsgChoice IMSA_AT_DMCN_IND_STRU */

    ID_IMSA_AT_IMSTIMER_SET_CNF             = 0x1017,                           /* _H2ASN_MsgChoice IMSA_AT_IMSTIMER_SET_CNF_STRU*/
    ID_IMSA_AT_IMSTIMER_QRY_CNF             = 0x1018,                           /* _H2ASN_MsgChoice IMSA_AT_IMSTIMER_QRY_CNF_STRU*/
    ID_IMSA_AT_SMSPSI_SET_CNF               = 0x1019,                           /* _H2ASN_MsgChoice IMSA_AT_SMSPSI_SET_CNF_STRU*/
    ID_IMSA_AT_SMSPSI_QRY_CNF               = 0x101A,                           /* _H2ASN_MsgChoice IMSA_AT_SMSPSI_SET_CNF_STRU*/
    ID_IMSA_AT_DMUSER_QRY_CNF               = 0x101B,

    ID_IMSA_AT_NICKNAME_SET_CNF             = 0x101C,                           /* _H2ASN_MsgChoice IMSA_AT_NICKNAME_SET_CNF_STRU */
    ID_IMSA_AT_NICKNAME_QRY_CNF             = 0x101D,                           /* _H2ASN_MsgChoice IMSA_AT_NICKNAME_QRY_CNF_STRU */
    ID_IMSA_AT_REGFAIL_IND                  = 0x101E,                           /* _H2ASN_MsgChoice IMSA_AT_REG_FAIL_IND_STRU */
    ID_IMSA_AT_BATTERYINFO_SET_CNF          = 0x101F,                           /* _H2ASN_MsgChoice IMSA_AT_BATTERY_INFO_SET_CNF_STRU */

    ID_IMSA_AT_CANCEL_ADD_VIDEO_CNF         = 0x1020,                           /* _H2ASN_MsgChoice IMSA_AT_CANCEL_ADD_VIDEO_CNF_STRU */

    ID_IMSA_AT_VOLTEIMPI_QRY_CNF            = 0x1021,                           /* _H2ASN_MsgChoice IMSA_AT_VOLTEIMPI_QRY_CNF_STRU */
    ID_IMSA_AT_VOLTEDOMAIN_QRY_CNF          = 0x1022,                           /* _H2ASN_MsgChoice IMSA_AT_VOLTEDOMAIN_QRY_CNF_STRU */
    ID_IMSA_AT_REGERR_REPORT_SET_CNF        = 0x1023,                           /* _H2ASN_MsgChoice IMSA_AT_REGERR_REPORT_SET_CNF_STRU*/
    ID_IMSA_AT_REGERR_REPORT_QRY_CNF        = 0x1024,                           /* _H2ASN_MsgChoice IMSA_AT_REGERR_REPORT_QRY_CNF_STRU*/
    ID_IMSA_AT_REGERR_REPORT_IND            = 0x1025,                           /* _H2ASN_MsgChoice IMSA_AT_REGERR_REPORT_IND_STRU */

    ID_IMSA_AT_IMS_IP_CAP_SET_CNF           = 0x1026,                           /* _H2ASN_MsgChoice IMSA_AT_IMS_UE_CAP_SET_CNF_STRU */
    ID_IMSA_AT_IMS_IP_CAP_QRY_CNF           = 0x1027,                           /* _H2ASN_MsgChoice IMSA_AT_IMS_UE_CAP_QRY_CNF_STRU */

    ID_IMSA_AT_EMC_PDN_ACTIVATE_IND         = 0x1028,                           /* _H2ASN_MsgChoice IMSA_AT_EMC_PDN_ACTIVATE_IND_STRU */
    ID_IMSA_AT_EMC_PDN_DEACTIVATE_IND       = 0x1029,                           /* _H2ASN_MsgChoice IMSA_AT_EMC_PDN_DEACTIVATE_IND_STRU */
    ID_IMSA_AT_CALL_ALT_SRV_IND             = 0x102A,                           /* _H2ASN_MsgChoice IMSA_AT_CALL_ALT_SRV_IND_STRU */

    ID_IMSA_AT_USER_AGENT_CFG_SET_CNF       = 0x1030,
    ID_AT_IMSA_MSG_BUTT
};
typedef  VOS_UINT32  AT_IMSA_MSG_TYPE_ENUM_UINT32;



enum AT_IMSA_IMS_REG_STATE_REPORT_ENUM
{
    AT_IMSA_IMS_REG_STATE_DISABLE_REPORT        = 0,
    AT_IMSA_IMS_REG_STATE_ENABLE_REPORT,
    AT_IMSA_IMS_REG_STATE_ENABLE_EXTENDED_REPROT,

    AT_IMSA_IMS_REG_STATE_REPROT_BUTT
};
typedef  VOS_UINT32  AT_IMSA_IMS_REG_STATE_REPORT_ENUM_UINT32;


enum AT_IMSA_CCWAI_MODE_ENUM
{
    AT_IMSA_CCWAI_MODE_DISABLE               = 0,
    AT_IMSA_CCWAI_MODE_ENABLE,

    AT_IMSA_CCWAI_MODE_BUTT
};
typedef  VOS_UINT8 AT_IMSA_CCWAI_MODE_ENUM_UINT8;



enum AT_IMSA_IMSVOPS_CAPABILITY_ENUM
{
    AT_IMSA_NW_NOT_SUPORT_IMSVOPS               = 0,
    AT_IMSA_NW_SUPORT_IMSVOPS,

    AT_IMSA_IMSVOPS_CAPABILITY_BUTT
};
typedef  VOS_UINT32  AT_IMSA_IMSVOPS_CAPABILITY_ENUM_UINT32;



enum AT_IMSA_CIREP_REPORT_ENUM
{
    AT_IMSA_CIREP_REPORT_DISENABLE      = 0,
    AT_IMSA_CIREP_REPORT_ENABLE,

    AT_IMSA_CIREP_REPORT_BUTT
};
typedef  VOS_UINT32  AT_IMSA_CIREP_REPORT_ENUM_UINT32;



enum AT_IMSA_SRVCC_HANDVOER_ENUM
{
    AT_IMSA_SRVCC_HANDOVER_STARTED                = 0,
    AT_IMSA_SRVCC_HANDOVER_SUCCESS,
    AT_IMSA_SRVCC_HANDOVER_CANCEL,
    AT_IMSA_SRVCC_HANDOVER_FAILURE,

    AT_IMSA_SRVCC_HANDOVER_BUTT
};
typedef  VOS_UINT32  AT_IMSA_SRVCC_HANDVOER_ENUM_UINT32;


enum AT_IMSA_BATTERY_STATUS_ENUM
{
    AT_IMSA_BATTERY_STATUS_NORMAL                 = 0,                          /* 正常 */
    AT_IMSA_BATTERY_STATUS_LOW,                                                 /* 低电 */
    AT_IMSA_BATTERY_STATUS_EXHAUST,                                             /* 电池耗尽 */

    AT_IMSA_BATTERY_STATUS_BUTT
};
typedef  VOS_UINT8  AT_IMSA_BATTERY_STATUS_ENUM_UINT8;


enum IMSA_AT_REG_FAIL_CODE_ENUM
{
    IMSA_AT_REG_FAIL_CODE_PERM                    = 1,

    IMSA_AT_REG_FAIL_CODE_BUTT
};
typedef  VOS_UINT8  IMSA_AT_REG_FAIL_CODE_ENUM_UINT8;


enum IMSA_AT_IMS_RAT_TYPE_ENUM
{
    IMSA_AT_IMS_RAT_TYPE_LTE            = 0x00,
    IMSA_AT_IMS_RAT_TYPE_WIFI           = 0x01,
    IMSA_AT_IMS_RAT_TYPE_BUTT
};
typedef VOS_UINT8 IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8;

enum AT_IMSA_ROAMING_IMS_SUPPORT_ENUM
{
    AT_IMSA_ROAMING_IMS_SUPPORT         = 1,
    AT_IMSA_ROAMING_IMS_NOT_SUPPORT     = 2,

    AT_IMSA_ROAMING_IMS_BUTT
};
typedef  VOS_UINT32  AT_IMSA_ROAMING_IMS_SUPPORT_ENUM_UINT32;


enum IMSA_AT_IMS_REG_DOMAIN_TYPE_ENUM
{
    IMSA_AT_IMS_REG_DOMAIN_TYPE_LTE     = 0x00,
    IMSA_AT_IMS_REG_DOMAIN_TYPE_WIFI    = 0x01,
    IMSA_AT_IMS_REG_DOMAIN_TYPE_UNKNOWN = 0x02,
    IMSA_AT_IMS_REG_DOMAIN_TYPE_BUTT
};
typedef VOS_UINT8 IMSA_AT_IMS_REG_DOMAIN_TYPE_ENUM_UINT8;



enum IMSA_AT_IMS_RAT_HO_STATUS_ENUM
{
    IMSA_AT_IMS_RAT_HO_STATUS_SUCCESS               = 0x00,     /* RAT handover was successful */
    IMSA_AT_IMS_RAT_HO_STATUS_FAILURE               = 0x01,     /* RAT handover was failed */
    IMSA_AT_IMS_RAT_HO_STATUS_NOT_TRIGGERED         = 0x02,     /* RAT handover could nut be triggered */
    IMSA_AT_IMS_RAT_HO_STATUS_BUTT
};
typedef VOS_UINT8 IMSA_AT_IMS_RAT_HO_STATUS_ENUM_UINT8;


enum IMSA_AT_IMS_SERVICE_STATUS_ENUM
{
    IMSA_AT_IMS_SERVICE_STATUS_NO_SERVICE           = 0x00,     /* IMS is no service */
    IMSA_AT_IMS_SERVICE_STATUS_LIMITED_SERVICE      = 0x01,     /* IMS is in limited service */
    IMSA_AT_IMS_SERVICE_STATUS_FULL_SERVICE         = 0x02,     /* IMS is in full service */
    IMSA_AT_IMS_SERVICE_STATUS_SOS_PENDING          = 0x08,     /* IMS is in sos pending */
    IMSA_AT_IMS_SERVICE_STATUS_BUTT
};
typedef VOS_UINT8 IMSA_AT_IMS_SERVICE_STATUS_ENUM_UINT8;



enum IMSA_AT_HO_CAUSE_ENUM
{
    IMSA_AT_HO_CAUSE_SUCCESS                    = 0,       /**< HO命令执行成功 */
    IMSA_AT_HO_CAUSE_FAIL_PARA_ERR              = 1,       /**< HO命令执行失败，由于参数错误 */
    IMSA_AT_HO_CAUSE_FAIL_CN_REJ                = 2,       /**< HO命令执行失败，由于被网侧拒绝 */
    IMSA_AT_HO_CAUSE_FAIL_TIMER_EXP             = 3,       /**< HO命令执行失败，由于定时器超时 */
    IMSA_AT_HO_CAUSE_FAIL_CONN_RELEASING        = 4,       /**< HO命令执行失败，由于正在释放连接 */
    IMSA_AT_HO_CAUSE_FAIL_PDP_ACTIVATE_LIMIT    = 5,       /**< HO命令执行失败，由于激活的承载数限制 */
    IMSA_AT_HO_CAUSE_FAIL_SAME_APN_OPERATING    = 6,       /**< HO命令执行失败，由于APS正在执行操作 */
    IMSA_AT_HO_CAUSE_FAIL_TEMP_FORBIDDEN        = 7,       /**< HO命令执行失败，收到临时被拒原因值或者网侧不响应 */
    IMSA_AT_HO_CAUSE_FAIL_PERM_FORBIDDEN        = 8,       /**< HO命令执行失败，收到永久被拒原因值 */
    IMSA_AT_HO_CAUSE_FAIL_WIFI_READY_IND_TIMEOUT    = 9,   /**< HO命令执行失败，由于WIFI下发送READY IND超时 */

    IMSA_AT_HO_CAUSE_FAIL_OHTERS                    = 11,    /**< HO命令执行失败，由于其他原因 */
	IMSA_AT_HO_CAUSE_FAIL_NO_DSDS_RESOURCE			= 12,    /**< HO命令执行失败，由于DSDS下申请无线资源失败 */

    IMSA_AT_HO_CAUSE_BUTT
};
typedef VOS_UINT32 IMSA_AT_HO_CAUSE_ENUM_UINT32;





enum IMSA_AT_PCSCF_SRC_ENUM
{
    IMSA_AT_PCSCF_SRC_DM_DEFAULT    = 0,    /* P-CSCF地址来源是DM默认配置 */
    IMSA_AT_PCSCF_SRC_DM            = 1,    /* P-CSCF地址来源是DM服务器 */

    IMSA_AT_PCSCF_SRC_BUTT
};
typedef VOS_UINT32 IMSA_AT_PCSCF_SRC_ENUM_UINT32;

enum AT_IMSA_VOICE_DOMAIN_ENUM
{
    AT_IMSA_VOICE_DOMAIN_CS_ONLY                = 0,  /* CS voice only */
    AT_IMSA_VOICE_DOMAIN_IMS_PS_ONLY            = 1,  /* IMS PS voice only */
    AT_IMSA_VOICE_DOMAIN_CS_PREFERRED           = 2,  /* CS voice preferred, IMS PS voice as secondary */
    AT_IMSA_VOICE_DOMAIN_IMS_PS_PREFERRED       = 3,  /* IMS PS voice preferred, CS voice as secondary  */

    AT_IMSA_VOICE_DOMAIN_BUTT
};
typedef VOS_UINT32 AT_IMSA_VOICE_DOMAIN_ENUM_UINT32;

enum IMSA_AT_REG_ERR_TYPE_ENUM
{
    IMSA_AT_REG_ERR_TYPE_PDN_FAIL       = 0x00,
    IMSA_AT_REG_ERR_TYPE_IMS_REG_FAIL   = 0x01,
    IMSA_AT_REG_ERR_TYPE_BUTT
};
typedef VOS_UINT8 IMSA_AT_REG_ERR_TYPE_ENUM_UINT8;

enum IMSA_AT_PDN_FAIL_CAUSE_ENUM
{
    IMSA_AT_PDN_FAIL_CAUSE_PARA_ERR                             = 1,
    /* IMSA_AT_PDN_FAIL_CAUSE_CN_REJ                               = 2,*/
    IMSA_AT_PDN_FAIL_CAUSE_TIMER_EXP                            = 3,
    IMSA_AT_PDN_FAIL_CAUSE_CONN_RELEASING                       = 4,
    IMSA_AT_PDN_FAIL_CAUSE_PDP_ACTIVATE_LIMIT                   = 5,
    IMSA_AT_PDN_FAIL_CAUSE_SAME_APN_OPERATING                   = 6,
    IMSA_AT_PDN_FAIL_CAUSE_TEMP_FORBIDDEN                       = 7,
    IMSA_AT_PDN_FAIL_CAUSE_PERM_FORBIDDEN                       = 8,
    IMSA_AT_PDN_FAIL_CAUSE_WIFI_READY_IND_TIMEOUT               = 9,
    IMSA_AT_PDN_FAIL_CAUSE_HO_NEW_PDN_SETUP_FAIL                = 10,
    IMSA_AT_PDN_FAIL_CAUSE_WIFI_FAIL_TEMP_FORBIDDEN             = 11,
    IMSA_AT_PDN_FAIL_CAUSE_WIFI_FAIL_PERM_FORBIDDEN             = 12,
    IMSA_AT_PDN_FAIL_CAUSE_WIFI_FAIL_TEMP_FORBIDDEN_WITH_TIMERLEN   = 13,
    IMSA_AT_PDN_FAIL_CAUSE_WIFI_FAIL_LINKLOST                   = 14,
    IMSA_AT_PDN_FAIL_CAUSE_OHTERS                               = 15,

    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_OPERATOR_DETERMINED_BARRING          = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 8),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_MBMS_BC_INSUFFICIENT                 = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 24),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_LLC_OR_SNDCP_FAILURE                 = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 25),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_INSUFFICIENT_RESOURCES               = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 26),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_MISSING_OR_UKNOWN_APN                = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 27),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_UNKNOWN_PDP_ADDR_OR_TYPE             = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 28),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_USER_AUTHENTICATION_FAIL             = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 29),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_ACTIVATION_REJECTED_BY_GGSN_SGW_OR_PGW         = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 30),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_ACTIVATION_REJECTED_UNSPECIFIED      = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 31),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SERVICE_OPTION_NOT_SUPPORTED         = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 32),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_REQUESTED_SERVICE_NOT_SUBSCRIBED     = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 33),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SERVICE_OPTION_TEMP_OUT_ORDER        = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 34),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_NSAPI_ALREADY_USED                   = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 35),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_REGULAR_DEACTIVATION                 = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 36),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_QOS_NOT_ACCEPTED                     = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 37),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_NETWORK_FAILURE                      = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 38),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_REACTIVATION_REQUESTED               = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 39),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_FEATURE_NOT_SUPPORT                  = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 40),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SEMANTIC_ERR_IN_TFT                  = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 41),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SYNTACTIC_ERR_IN_TFT                 = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 42),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_UKNOWN_PDP_CONTEXT                   = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 43),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SEMANTIC_ERR_IN_PACKET_FILTER        = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 44),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SYNCTACTIC_ERR_IN_PACKET_FILTER      = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 45),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_PDP_CONTEXT_WITHOUT_TFT_ACTIVATED    = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 46),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_MULTICAST_GROUP_MEMBERHHSHIP_TIMEOUT = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 47),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_REQUEST_REJECTED_BCM_VIOLATION       = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 48),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_LAST_PDN_DISCONN_NOT_ALLOWED         = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 49),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_PDP_TYPE_IPV4_ONLY_ALLOWED           = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 50),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_PDP_TYPE_IPV6_ONLY_ALLOWED           = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 51),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SINGLE_ADDR_BEARERS_ONLY_ALLOWED     = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 52),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_INFORMATION_NOT_RECEIVED             = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 53),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_PDN_CONNECTION_DOES_NOT_EXIST        = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 54),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SAME_APN_MULTI_PDN_CONNECTION_NOT_ALLOWED = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 55),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_COLLISION_WITH_NW_INITIATED_REQUEST  = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 56),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_UNSUPPORTED_QCI_VALUE                = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 59),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_BEARER_HANDLING_NOT_SUPPORTED        = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 60),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_INVALID_TI                           = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 81),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SEMANTICALLY_INCORRECT_MESSAGE       = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 95),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_INVALID_MANDATORY_INFO               = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 96),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_MSG_TYPE_NON_EXISTENT                = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 97),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_MSG_TYPE_NOT_COMPATIBLE              = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 98),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_IE_NON_EXISTENT                      = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 99),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_CONDITIONAL_IE_ERR                   = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 100),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_MSG_NOT_COMPATIBLE                   = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 101),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_PROTOCOL_ERR_UNSPECIFIED             = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 111),
    IMSA_AT_PDN_FAIL_CAUSE_SM_NW_APN_RESTRICTION_INCOMPATIBLE         = (IMSA_AT_PDN_FAIL_CAUSE_SM_NW_SECTION_BEGIN + 112),

    IMSA_AT_PDN_FAIL_CAUSE_BUTT
};
typedef VOS_UINT32 IMSA_AT_PDN_FAIL_CAUSE_ENUM_UINT32;

enum IMSA_AT_REG_FAIL_CAUSE_ENUM
{
    IMSA_AT_REG_FAIL_CAUSE_FAIL                = 1,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_REMOTE         = 2,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_TIMEOUT        = 3,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_TRANSPORT      = 4,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_SA             = 5,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_MT_DEREG       = 6,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_NO_ADDR_PAIR   = 7,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_LACK_PARAM     = 8,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_FORBIDDEN      = 9,
    IMSA_AT_REG_FAIL_CAUSE_FAIL_REREG_FAIL     = 10,

    /* IMS网侧拒绝原因值 300~699,直接填数字，不再一一列举  */

    IMSA_AT_REG_FAIL_CAUSE_BUTT                = 700,
};
typedef VOS_UINT32 IMSA_AT_REG_FAIL_CAUSE_ENUM_UINT32;


/*****************************************************************************
  3 类型定义
*****************************************************************************/

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT8                           aucContent[4];
} AT_IMSA_MSG_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;                           /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_CNF_MSG_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_CIREG_QRY_REQ_STRU;



typedef AT_IMSA_MSG_STRU AT_IMSA_CIREP_QRY_REQ_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_VOLTEIMPU_QRY_REQ_STRU;



typedef AT_IMSA_MSG_STRU AT_IMSA_VOLTEIMPI_QRY_REQ_STRU;



typedef AT_IMSA_MSG_STRU AT_IMSA_VOLTEDOMAIN_QRY_REQ_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    AT_IMSA_CCWAI_MODE_ENUM_UINT8       enMode;
    VOS_UINT32                          ulSrvClass;
} AT_IMSA_CCWAI_SET_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           ucReserved;
    VOS_UINT32                          ulResult;
} IMSA_AT_CCWAI_SET_CNF_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved1[1];
    VOS_UINT8                           ucEncrypt;                              /* 0:不加密，1:加密 */
    VOS_UINT8                           aucReserved2[3];
} AT_IMSA_CALL_ENCRYPT_SET_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           ucReserved;
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_CALL_ENCRYPT_SET_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    AT_IMSA_IMS_REG_STATE_REPORT_ENUM_UINT32    enCireg;
} AT_IMSA_CIREG_SET_REQ_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_CIREG_SET_CNF_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];

    VOS_UINT32                          bitOpExtInfo    : 1 ;                   /* +CIREG=2时,标志位置1 */
    VOS_UINT32                          bitOpSpare      : 31;

    AT_IMSA_IMS_REG_STATE_REPORT_ENUM_UINT32    enCireg;
    VOS_UINT32                          ulRegInfo;
    VOS_UINT32                          ulExtInfo;                              /* +CIREG=2时 ，扩展信息暂时回复0 */
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_CIREG_QRY_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;                             /* 主动上报时填0X3FFF */
    VOS_UINT8                           ucOpId;                                 /* 填0 */
    VOS_UINT8                           aucReserved[1];

    VOS_UINT32                          bitOpExtInfo    : 1 ;
    VOS_UINT32                          bitOpSpare      : 31;

    VOS_UINT32                          ulRegInfo;
    VOS_UINT32                          ulExtInfo;
} IMSA_AT_CIREGU_IND_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    AT_IMSA_CIREP_REPORT_ENUM_UINT32    enReport;
} AT_IMSA_CIREP_SET_REQ_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_CIREP_SET_CNF_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    AT_IMSA_CIREP_REPORT_ENUM_UINT32            enReport;
    AT_IMSA_IMSVOPS_CAPABILITY_ENUM_UINT32      enImsvops;
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_CIREP_QRY_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                             /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;               /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT32                          ulResult;              /* 成功返回VOS_OK，失败返回VOS_ERR */
    VOS_UINT32                          ulImpuLen;
    VOS_CHAR                            aucImpu[AT_IMSA_IMPU_MAX_LENGTH];
    VOS_UINT32                          ulImpuLenVirtual;
    VOS_CHAR                            aucImpuVirtual[AT_IMSA_IMPU_MAX_LENGTH];
} IMSA_AT_VOLTEIMPU_QRY_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                             /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;               /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT32                          ulResult;              /* 成功返回VOS_OK，失败返回VOS_ERR */
    VOS_UINT32                          ulImpiLen;
    VOS_CHAR                            aucImpi[AT_IMSA_IMPI_MAX_LENGTH];
} IMSA_AT_VOLTEIMPI_QRY_CNF_STRU;

typedef struct
{
    VOS_MSG_HEADER                                             /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;               /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT32                          ulResult;              /* 成功返回VOS_OK，失败返回VOS_ERR */
    VOS_UINT32                          ulDomainLen;
    VOS_CHAR                            aucDomain[AT_IMSA_DOMAIN_MAX_LENGTH];
} IMSA_AT_VOLTEDOMAIN_QRY_CNF_STRU;



typedef AT_IMSA_MSG_STRU AT_IMSA_ROAMING_IMS_QRY_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    VOS_UINT16                              usClientId;
    VOS_UINT8                               ucOpId;
    VOS_UINT8                               aucReserved[1];
    AT_IMSA_ROAMING_IMS_SUPPORT_ENUM_UINT32 enRoamingImsSupportFlag;
    VOS_UINT32                              ulResult;
} IMSA_AT_ROAMING_IMS_QRY_CNF_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;                             /* 主动上报时填0X3FFF */
    VOS_UINT8                           ucOpId;                                 /* 填0 */
    VOS_UINT8                           aucReserved[1];
    AT_IMSA_SRVCC_HANDVOER_ENUM_UINT32  enHandover;
} IMSA_AT_CIREPH_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;                             /* 主动上报时填0X3FFF */
    VOS_UINT8                           ucOpId;                                 /* 填0 */
    VOS_UINT8                           aucReserved[1];
    AT_IMSA_IMSVOPS_CAPABILITY_ENUM_UINT32      enImsvops;
} IMSA_AT_CIREPI_IND_STRU;



typedef struct
{
    VOS_MSG_HEADER                                             /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;               /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8     enRatType;                  /* 注册域 */
    TAF_PDP_ADDR_STRU                   stPdpAddr;
    TAF_PDP_DNS_STRU                    stIpv4Dns;
    TAF_PDP_IPV6_DNS_STRU               stIpv6Dns;
} IMSA_AT_VT_PDP_ACTIVATE_IND_STRU;



typedef struct
{
    VOS_MSG_HEADER                                             /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;               /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    TAF_PDP_TYPE_ENUM_UINT8             enPdpType;
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8     enRatType;             /* 注册域 */
    VOS_UINT8                           aucReserved[3];
} IMSA_AT_VT_PDP_DEACTIVATE_IND_STRU;

/*****************************************************************************
 结构名称  : IMSA_AT_EMC_PDN_ACTIVATE_IND_STRU
 结构说明  : ID_IMSA_AT_EMC_PDN_ACTIVATE_IND 消息结构
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                             /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;               /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           ucReaerved;
    TAF_PDP_ADDR_STRU                   stPdpAddr;
    TAF_PDP_DNS_STRU                    stIpv4Dns;
    TAF_PDP_IPV6_DNS_STRU               stIpv6Dns;
    VOS_UINT16                          usMtu;
    VOS_UINT16                          usReserved;
} IMSA_AT_EMC_PDN_ACTIVATE_IND_STRU;

/*****************************************************************************
 结构名称  : IMSA_AT_EMC_PDN_ACTIVATE_IND_STRU
 结构说明  : ID_IMSA_AT_EMC_PDN_DEACTIVATE_IND 消息结构
*****************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                             /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;               /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    TAF_PDP_TYPE_ENUM_UINT8             enPdpType;
} IMSA_AT_EMC_PDN_DEACTIVATE_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER                                          /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;            /* _H2ASN_Skip */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
    VOS_UINT32                          ulCauseCode;
    VOS_UINT8                           ucMtStatus;
    VOS_UINT8                           aucRsv[3];
    VOS_UINT8                           aucAsciiCallNum[AT_IMSA_CALL_ASCII_NUM_MAX_LENGTH];
} IMSA_AT_MT_STATES_IND_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_IMS_REG_DOMAIN_QRY_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    VOS_UINT16                              usClientId;
    VOS_UINT8                               ucOpId;
    IMSA_AT_IMS_REG_DOMAIN_TYPE_ENUM_UINT8  enImsRegDomain;
} IMSA_AT_IMS_REG_DOMAIN_QRY_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    VOS_UINT16                              usClientId;
    VOS_UINT8                               ucOpId;
    VOS_UINT8                               aucReserved[1];
    VOS_UINT32                              ulWifiMsgLen;
    VOS_UINT8                               aucWifiMsg[4];
} AT_IMSA_IMS_CTRL_MSG_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    VOS_UINT16                              usClientId;
    VOS_UINT8                               ucOpId;
    VOS_UINT8                               aucReserved[1];
    VOS_UINT32                              ulWifiMsgLen;
    VOS_UINT8                               aucWifiMsg[4];
} IMSA_AT_IMS_CTRL_MSG_STRU;



typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    VOS_UINT16                              usClientId;                         /* 主动上报时填0X3FFF */
    VOS_UINT8                               ucOpId;                             /* 填0 */
    IMSA_AT_IMS_RAT_HO_STATUS_ENUM_UINT8    enHoStatus;                         /* 切换状态 */
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8         enSrcRat;                           /* 原有IMS注册域 */
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8         enDstRat;                           /* 目标IMS注册域 */
    VOS_UINT8                               aucReserved[2];
    IMSA_AT_HO_CAUSE_ENUM_UINT32            enCause;                            /* 切换失败原因值 */
} IMSA_AT_IMS_RAT_HANDOVER_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    VOS_UINT16                              usClientId;                         /* 主动上报时填0X3FFF */
    VOS_UINT8                               ucOpId;                             /* 填0 */
    VOS_UINT8                               aucReserved[1];
    IMSA_AT_IMS_SERVICE_STATUS_ENUM_UINT8   enSmsSrvStatus;                     /* 短信的IMS服务状态 */
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8         enSmsSrvRat;                        /* 短信的IMS服务域 */
    IMSA_AT_IMS_SERVICE_STATUS_ENUM_UINT8   enVoIpSrvStatus;                    /* VoIP的IMS服务状态 */
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8         enVoIpSrvRat;                       /* VoIP的IMS服务域 */
    IMSA_AT_IMS_SERVICE_STATUS_ENUM_UINT8   enVtSrvStatus;                      /* VT的IMS服务状态 */
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8         enVtSrvRat;                         /* VT的IMS服务域 */
    IMSA_AT_IMS_SERVICE_STATUS_ENUM_UINT8   enVsSrvStatus;                      /* VS的IMS服务状态 */
    IMSA_AT_IMS_RAT_TYPE_ENUM_UINT8         enVsSrvRat;                         /* VS的IMS服务域 */
} IMSA_AT_IMS_SRV_STATUS_UPDATE_IND_STRU;

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
} IMSA_AT_DMCN_IND_STRU;


typedef struct
{
    VOS_UINT32                          bitOpAmrWbOctetAcigned       : 1;
    VOS_UINT32                          bitOpAmrWbBandWidthEfficient : 1;
    VOS_UINT32                          bitOpAmrOctetAcigned         : 1;
    VOS_UINT32                          bitOpAmrBandWidthEfficient   : 1;
    VOS_UINT32                          bitOpAmrWbMode               : 1;
    VOS_UINT32                          bitOpDtmfWb                  : 1;
    VOS_UINT32                          bitOpDtmfNb                  : 1;
    VOS_UINT32                          bitOpSpeechStart             : 1;
    VOS_UINT32                          bitOpSpeechEnd               : 1;
    VOS_UINT32                          bitOpVideoStart              : 1;
    VOS_UINT32                          bitOpVideoEnd                : 1;
    VOS_UINT32                          bitOpRetryBaseTime           : 1;
    VOS_UINT32                          bitOpRetryMaxTime            : 1;
    VOS_UINT32                          bitOpPhoneContext            : 1;
    VOS_UINT32                          bitOpPhoneContextImpu        : 1;
    VOS_UINT32                          bitOpSpare                   : 17;

    VOS_UINT32                          ulAmrWbOctetAcigned;
    VOS_UINT32                          ulAmrWbBandWidthEfficient;              /* 接口预留，IMSA不处理 */
    VOS_UINT32                          ulAmrOctetAcigned;
    VOS_UINT32                          ulAmrBandWidthEfficient;                /* 接口预留，IMSA不处理 */
    VOS_UINT32                          ulAmrWbMode;
    VOS_UINT32                          ulDtmfWb;                               /* 接口预留，IMSA不处理 */
    VOS_UINT32                          ulDtmfNb;                               /* 接口预留，IMSA不处理 */
    VOS_UINT32                          ulSpeechStart;
    VOS_UINT32                          ulSpeechEnd;
    VOS_UINT32                          ulVideoStart;
    VOS_UINT32                          ulVideoEnd;
    VOS_UINT32                          ulRetryBaseTime;
    VOS_UINT32                          ulRetryMaxTime;
    VOS_CHAR                            acPhoneContext[IMSA_PHONECONTEXT_MAX_LENGTH + 1];
    VOS_UINT8                           aucReserved1[3];
    VOS_CHAR                            acPhoneContextImpu[IMSA_PUBLICEUSERID_MAX_LENGTH + 1];
    VOS_UINT8                           aucReserved2[3];

} AT_IMSA_DMDYN_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    AT_IMSA_DMDYN_STRU                  stDmdyn;
} AT_IMSA_DMDYN_SET_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_DMDYN_SET_CNF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_DMDYN_QRY_REQ_STRU;

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    VOS_UINT32                          ulResult;
    AT_IMSA_DMDYN_STRU                  stDmdyn;
} IMSA_AT_DMDYN_QRY_CNF_STRU;


/*****************************************************************************
 结构名称: IMSA_PDP_IPV4_PCSCF_STRU
 结构说明: IPv4 P-CSCF地址结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitOpPrimPcscfAddr      : 1;
    VOS_UINT32                          bitOpSecPcscfAddr       : 1;
    VOS_UINT32                          bitOpThiPcscfAddr       : 1;
    VOS_UINT32                          bitOpPrimPcscfSipPort   : 1;
    VOS_UINT32                          bitOpSecPcscfSipPort    : 1;
    VOS_UINT32                          bitOpThiPcscfSipPort    : 1;
    VOS_UINT32                          bitOpSpare              : 26;

    VOS_UINT8                           aucPrimPcscfAddr[TAF_IPV4_ADDR_LEN];
    VOS_UINT8                           aucSecPcscfAddr[TAF_IPV4_ADDR_LEN];
    VOS_UINT8                           aucThiPcscfAddr[TAF_IPV4_ADDR_LEN];
    VOS_UINT32                          ulPrimPcscfSipPort;
    VOS_UINT32                          ulSecPcscfSipPort;
    VOS_UINT32                          ulThiPcscfSipPort;
} IMSA_AT_IPV4_PCSCF_STRU;

/*****************************************************************************
 结构名称: IMSA_PDP_IPV6_PCSCF_STRU
 结构说明: IPv6 P-CSCF地址结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitOpPrimPcscfAddr      : 1;
    VOS_UINT32                          bitOpSecPcscfAddr       : 1;
    VOS_UINT32                          bitOpThiPcscfAddr       : 1;
    VOS_UINT32                          bitOpPrimPcscfSipPort   : 1;
    VOS_UINT32                          bitOpSecPcscfSipPort    : 1;
    VOS_UINT32                          bitOpThiPcscfSipPort    : 1;
    VOS_UINT32                          bitOpSpare              : 26;

    VOS_UINT8                           aucPrimPcscfAddr[TAF_IPV6_ADDR_LEN];
    VOS_UINT8                           aucSecPcscfAddr[TAF_IPV6_ADDR_LEN];
    VOS_UINT8                           aucThiPcscfAddr[TAF_IPV6_ADDR_LEN];
    VOS_UINT32                          ulPrimPcscfSipPort;
    VOS_UINT32                          ulSecPcscfSipPort;
    VOS_UINT32                          ulThiPcscfSipPort;
} IMSA_AT_IPV6_PCSCF_STRU;

typedef struct
{
    IMSA_AT_PCSCF_SRC_ENUM_UINT32         enSrc;                                /* 区分P-CSCF地址来源， */
    IMSA_AT_IPV6_PCSCF_STRU               stIpv6Pcscf;
    IMSA_AT_IPV4_PCSCF_STRU               stIpv4Pcscf;
}IMSA_AT_PCSCF_STRU;

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    IMSA_AT_PCSCF_STRU                  stAtPcscf;
} AT_IMSA_PCSCF_SET_REQ_STRU;

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */
} IMSA_AT_PCSCF_SET_CNF_STRU;

typedef AT_IMSA_MSG_STRU AT_IMSA_PCSCF_QRY_REQ_STRU;

typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    VOS_UINT32                          ulResult;
    IMSA_AT_PCSCF_STRU                  stAtPcscf;
} IMSA_AT_PCSCF_QRY_CNF_STRU;


typedef struct
{
    VOS_UINT32                              bitOpTimerT1Value   : 1;
    VOS_UINT32                              bitOpTimerT2Value   : 1;
    VOS_UINT32                              bitOpTimerT4Value   : 1;
    VOS_UINT32                              bitOpTimerTAValue   : 1;
    VOS_UINT32                              bitOpTimerTBValue   : 1;
    VOS_UINT32                              bitOpTimerTCValue   : 1;
    VOS_UINT32                              bitOpTimerTDValue   : 1;
    VOS_UINT32                              bitOpTimerTEValue   : 1;
    VOS_UINT32                              bitOpTimerTFValue   : 1;
    VOS_UINT32                              bitOpTimerTGValue   : 1;
    VOS_UINT32                              bitOpTimerTHValue   : 1;
    VOS_UINT32                              bitOpTimerTIValue   : 1;
    VOS_UINT32                              bitOpTimerTJValue   : 1;
    VOS_UINT32                              bitOpTimerTKValue   : 1;
    VOS_UINT32                              bitOpSpare          : 18;

    VOS_UINT32                              ulTimerT1Value;
    VOS_UINT32                              ulTimerT2Value;
    VOS_UINT32                              ulTimerT4Value;
    VOS_UINT32                              ulTimerTAValue;
    VOS_UINT32                              ulTimerTBValue;
    VOS_UINT32                              ulTimerTCValue;
    VOS_UINT32                              ulTimerTDValue;
    VOS_UINT32                              ulTimerTEValue;
    VOS_UINT32                              ulTimerTFValue;
    VOS_UINT32                              ulTimerTGValue;
    VOS_UINT32                              ulTimerTHValue;
    VOS_UINT32                              ulTimerTIValue;
    VOS_UINT32                              ulTimerTJValue;
    VOS_UINT32                              ulTimerTKValue;

}IMSA_AT_IMS_TIMER_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                              ulMsgId;
    AT_APPCTRL_STRU                         stAppCtrl;
    IMSA_AT_IMS_TIMER_STRU                  stImsTimer;
}AT_IMSA_IMSTIMER_SET_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                              ulMsgId;
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;                   /* 成功返回 VOS_OK,失败返回 VOS_ERR*/
}IMSA_AT_IMSTIMER_SET_CNF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_IMSTIMER_QRY_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                              ulMsgId;
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;
    IMSA_AT_IMS_TIMER_STRU                  stImsTimer;
}IMSA_AT_IMSTIMER_QRY_CNF_STRU;


typedef struct
{
    VOS_CHAR                                acSmsPsi[AT_IMSA_MAX_SMSPSI_LEN + 1];
    VOS_UINT8                               aucReserved[3];
}IMSA_SMS_PSI_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                              ulMsgId;
    AT_APPCTRL_STRU                         stAppCtrl;
    IMSA_SMS_PSI_STRU                       stSmsPsi;
}AT_IMSA_SMSPSI_SET_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                              ulMsgId;
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;
}IMSA_AT_SMSPSI_SET_CNF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_SMSPSI_QRY_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                              ulMsgId;
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;
    IMSA_SMS_PSI_STRU                       stSmsPsi;

}IMSA_AT_SMSPSI_QRY_CNF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_DMUSER_QRY_REQ_STRU;


typedef struct
{
    VOS_CHAR                                acImpi[AT_IMSA_IMPI_MAX_LENGTH + 1];
    VOS_CHAR                                acImpu[AT_IMSA_IMPU_MAX_LENGTH + 1];
    VOS_CHAR                                acHomeNetWorkDomain[IMSA_IMS_STRING_LENGTH + 1];
    VOS_UINT8                               aucReserved[1];
    AT_IMSA_VOICE_DOMAIN_ENUM_UINT32        enVoiceDomain;
    VOS_UINT32                              ulIpsecEnable;
}IMSA_AT_DMUSER_STRU;


typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                              ulMsgId;
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;
    IMSA_AT_DMUSER_STRU                     stDmUser;
}IMSA_AT_DMUSER_QRY_CNF_STRU;



typedef struct
{
    VOS_UINT8                               ucNickNameLen;
    VOS_CHAR                                acNickName[MN_CALL_DISPLAY_NAME_STRING_SZ];
    VOS_UINT8                               aucReserved[2];
} IMSA_AT_NICKNAME_INFO_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
    IMSA_AT_NICKNAME_INFO_STRU              stNickName;
} AT_IMSA_NICKNAME_SET_REQ_STRU;


typedef IMSA_AT_CNF_MSG_STRU IMSA_AT_NICKNAME_SET_CNF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_NICKNAME_QRY_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;                           /* 成功返回VOS_OK，失败返回VOS_ERR */
    IMSA_AT_NICKNAME_INFO_STRU              stNickName;
} IMSA_AT_NICKNAME_QRY_CNF_STRU;


typedef struct
{
    AT_IMSA_BATTERY_STATUS_ENUM_UINT8       enBatteryStatus;
    VOS_UINT8                               aucReserved[3];
} AT_IMSA_BATTERY_STATUS_INFO_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
    AT_IMSA_BATTERY_STATUS_INFO_STRU        stBatteryStatusInfo;
} AT_IMSA_BATTERY_INFO_SET_REQ_STRU;


typedef IMSA_AT_CNF_MSG_STRU IMSA_AT_BATTERY_INFO_SET_CNF_STRU;


typedef struct
{
    IMSA_AT_REG_FAIL_CODE_ENUM_UINT8        enFailCode;
    VOS_UINT8                               aucReserved[3];
} IMSA_AT_REG_FAIL_INFO_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
    IMSA_AT_REG_FAIL_INFO_STRU              stRegFailInfo;
} IMSA_AT_REG_FAIL_IND_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_VOLTEREG_NTF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_CANCEL_ADD_VIDEO_REQ_STRU;


typedef IMSA_AT_CNF_MSG_STRU IMSA_AT_CANCEL_ADD_VIDEO_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    VOS_UINT8                           ucReportFlag;   /* VOS_TRUE:允许主动上报；VOS_FALSE:不允许上报 */
    VOS_UINT8                           aucReserv[3];
} AT_IMSA_REGERR_REPORT_SET_REQ_STRU;


typedef IMSA_AT_CNF_MSG_STRU IMSA_AT_REGERR_REPORT_SET_CNF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_REGERR_REPORT_QRY_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */

    AT_APPCTRL_STRU                     stAppCtrl;
    VOS_UINT32                          ulResult;                               /* 成功返回VOS_OK，失败返回VOS_ERR */

    VOS_UINT8                           ucReportFlag;
    VOS_UINT8                           aucReserved[7];
} IMSA_AT_REGERR_REPORT_QRY_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
    IMSA_AT_IMS_REG_DOMAIN_TYPE_ENUM_UINT8  enImsaRegDomain;
    IMSA_AT_REG_ERR_TYPE_ENUM_UINT8         enImsaRegErrType;
    VOS_UINT8                               aucRsv[2];
    IMSA_AT_PDN_FAIL_CAUSE_ENUM_UINT32      enImsaPdnFailCause;
    IMSA_AT_REG_FAIL_CAUSE_ENUM_UINT32      enImsaRegFailCause;
    VOS_CHAR                                acImsRegFailReasonCtx[IMSA_AT_REG_FAIL_CAUSE_STR_MAX_LEN];         /* 无字符串时全0 */
} IMSA_AT_REGERR_REPORT_IND_STRU;



typedef struct
{
    VOS_UINT32                              bitOpIpsecFlag       : 1;
    VOS_UINT32                              bitOpKeepAliveFlag   : 1;
    VOS_UINT32                              bitOpRev             : 30;
    VOS_UINT32                              ulIpsecFlag;                        /* VOS_TRUE:支持IPSEC；VOS_FALSE:不支持IPSEC */
    VOS_UINT32                              ulKeepAliveFlag;                    /* VOS_TRUE:支持KEEP ALIVE；VOS_FALSE:不支持KEEP ALIVE */
} AT_IMSA_IMS_IP_CAP_SET_INFO_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;
    AT_IMSA_IMS_IP_CAP_SET_INFO_STRU    stIpCapInfo;
} AT_IMSA_IMS_IP_CAP_SET_REQ_STRU;

typedef IMSA_AT_CNF_MSG_STRU IMSA_AT_IMS_IP_CAP_SET_CNF_STRU;


typedef AT_IMSA_MSG_STRU AT_IMSA_IMS_IP_CAP_QRY_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
    VOS_UINT32                              ulResult;
    VOS_UINT32                              ulIpsecFlag;
    VOS_UINT32                              ulKeepAliveFlag;
} IMSA_AT_IMS_IP_CAP_QRY_CNF_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                              ulMsgId;                            /* _H2ASN_Skip */
    AT_APPCTRL_STRU                         stAppCtrl;
} IMSA_AT_CALL_ALT_SRV_IND_STRU;


typedef struct
{
    VOS_UINT8                           aucPara1[AT_IMSA_USER_AGENT_STR_LEN + 1];
    VOS_UINT8                           aucPara2[AT_IMSA_USER_AGENT_STR_LEN + 1];
    VOS_UINT8                           aucPara3[AT_IMSA_USER_AGENT_STR_LEN + 1];
    VOS_UINT8                           aucPara4[AT_IMSA_USER_AGENT_STR_LEN + 1];
    VOS_UINT8                           aucPara5[AT_IMSA_USER_AGENT_STR_LEN + 1];
    VOS_UINT8                           aucPara6[AT_IMSA_USER_AGENT_STR_LEN + 1];
    VOS_UINT8                           aucReserved[2];
} AT_IMSA_USER_AGENT_CFG_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    VOS_UINT32                          ulMsgId;                                /* _H2ASN_Skip */
    AT_APPCTRL_STRU                     stAppCtrl;

    AT_IMSA_USER_AGENT_CFG_STRU         stUaCfgInfo;
} AT_IMSA_USER_AGENT_CFG_SET_REQ_STRU;


typedef IMSA_AT_CNF_MSG_STRU IMSA_AT_USER_AGENT_CFG_SET_CNF_STRU;


/*****************************************************************************
  4 宏定义
*****************************************************************************/


/*****************************************************************************
  5 全局变量声明
*****************************************************************************/


/*****************************************************************************
  6 接口函数声明
*****************************************************************************/

/*****************************************************************************
  7 OTHERS定义
*****************************************************************************/

/* ASN解析结构 */
typedef struct
{
    VOS_UINT32                          ulMsgId;                                /*_H2ASN_MsgChoice_Export AT_IMSA_MSG_TYPE_ENUM_UINT32 */
    VOS_UINT8                           aucMsg[4];
    /***************************************************************************
        _H2ASN_MsgChoice_When_Comment          AT_IMSA_MSG_TYPE_ENUM_UINT32
    ****************************************************************************/
}AT_IMSA_INTERFACE_MSG_DATA;
/*_H2ASN_Length UINT32*/

typedef struct
{
    VOS_MSG_HEADER
    AT_IMSA_INTERFACE_MSG_DATA           stMsgData;
} AtImsaInterface_MSG;


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


