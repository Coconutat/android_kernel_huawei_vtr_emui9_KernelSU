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


#ifndef __SI_STK_H__
#define __SI_STK_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"

#if (OSA_CPU_CCPU == VOS_OSA_CPU)
#include "pamom.h"
#include "UsimmApi.h"
#include "NasStkInterface.h"
#endif

#if (OSA_CPU_ACPU == VOS_OSA_CPU)
#include "pamappom.h"
#endif

#include "msp_nvim.h"
#include "TafOamInterface.h"
#include "omnvinterface.h"
#include "NasNvInterface.h"
#include "TafNvInterface.h"
#include "AtOamInterface.h"
#include "MnErrorCode.h"
#include "Taf_MmiStrParse.h"
#include "TafNvInterface.h"
#include "msp_diag_comm.h"
#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
#include "TafXsmsStkInterface.h"
#include "TafXsmsDecode.h"
#endif
#endif

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)


/*****************************************************************************
  2 宏定义
*****************************************************************************/

typedef VOS_UINT32 (*TLV2DS)(VOS_UINT8 *pDes,VOS_UINT8 *pSrc);

typedef VOS_UINT32 (*DS2TLV)(VOS_UINT8 Tag,VOS_UINT8 *pDes,VOS_UINT8 *pSrc,VOS_UINT8 len);

#define STK_GEN_LOG_MODULE(Level)       (DIAG_GEN_LOG_MODULE(VOS_GetModemIDFromPid(MAPS_STK_PID), DIAG_MODE_COMM, Level))

#define STK_INFO_LOG(string)            (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_NORMAL),MAPS_STK_PID, __FILE__, __LINE__, "NORMAL:%s", string)
#define STK_NORMAL_LOG(string)          (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_NORMAL),MAPS_STK_PID, __FILE__, __LINE__, "NORMAL:%s", string)
#define STK_WARNING_LOG(string)         (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_WARNING),MAPS_STK_PID, __FILE__, __LINE__, "WARNING:%s", string)
#define STK_ERROR_LOG(string)           (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_ERROR),MAPS_STK_PID, __FILE__, __LINE__, "ERROR:%s", string)

#define STK_INFO_LOG1(string, para1)    (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_NORMAL),MAPS_STK_PID, __FILE__, __LINE__, "NORMAL:%s,%d", string, para1)
#define STK_NORMAL_LOG1(string, para1)  (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_NORMAL),MAPS_STK_PID, __FILE__, __LINE__, "NORMAL:%s,%d", string, para1)
#define STK_WARNING_LOG1(string, para1) (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_WARNING),MAPS_STK_PID, __FILE__, __LINE__, "WARNING:%s,%d", string, para1)
#define STK_ERROR_LOG1(string, para1)   (VOS_VOID)DIAG_LogReport(STK_GEN_LOG_MODULE(PS_LOG_LEVEL_ERROR),MAPS_STK_PID, __FILE__, __LINE__, "ERROR:%s,%d", string, para1)

#define SI_SAT_OFFSET(A,b)              ((VOS_UINT16)(&(((A*)0)->b)))

#define SI_STK_GET_MCC(NUM)             (100 * ((NUM  % 1000 / 100 + 9) % 10) \
                                        + 10* ((NUM % 100 / 10 + 9) % 10) \
                                        + (NUM % 10 + 9) %10)

#define SI_STK_GET_MNC(NUM)             (10* ((NUM % 100 / 10 + 9) % 10) \
                                        + (NUM % 10 + 9) %10)

#define SI_TAGNOTFOUND                          (0xFFFFFFFF)

#define STK_DATA_TAG_MASK                       (0x80) /*按照协议,Data Tag的最高bit可以是1*/

#define STK_POROTECT_TIME_LEN                   (20)  /*默认20秒超时*/

#define STK_IND_TIME_LEN                        (5000)

#define STK_DURATION_MIN                        (0x00)
#define STK_DURATION_SEC                        (0x01)
#define STK_DURATION_TENTH_SEC                  (0x02)

#define STK_TIME_MANAGEMENT_START               (0x00)
#define STK_TIME_MANAGEMENT_DEACTIVE            (0x01)
#define STK_TIME_MANAGEMENT_GET_VALUE           (0x02)

#define STK_TIMER_MAX_NUM                       (9)

/*返回的TR数据长度，按照结果只有一个字节计算*/
#define STK_TERMINAL_RSP_LEN                    (12)

#define STK_LOCAL_INFO_2G                       (9)

#define STK_ENVELOPE_MAX_LEN                    (256)

#define STK_SMS_MAX_MSG_SEGMENT_NUM             (2)

#define STK_TIMER_SECOND_VALUE_MAX              (60)
#define STK_TIMER_MINUTE_VALUE_MAX              (60)
#define STK_TIMER_HOUR_VALUE_MAX                (17)

#define STK_PROVIDE_LOCAL_INFO_NAA              (0x00)
#define STK_PROVIDE_LOCAL_INFO_IMEI             (0x01)
#define STK_PROVIDE_LOCAL_INFO_NMR              (0x02)
#define STK_PROVIDE_LOCAL_INFO_TIME             (0x03)
#define STK_PROVIDE_LOCAL_INFO_LANGUAGE         (0x04)
#define STK_PROVIDE_LOCAL_INFO_RES_GSM          (0x05)
#define STK_PROVIDE_LOCAL_INFO_ACCESS_TECH      (0x06)
#define STK_PROVIDE_LOCAL_INFO_ESN              (0x07)
#define STK_PROVIDE_LOCAL_INFO_IMEISV           (0x08)
#define STK_PROVIDE_LOCAL_INFO_SEARCH_MODE      (0x09)
#define STK_PROVIDE_LOCAL_INFO_BATTERY_STATE    (0x0A)
#define STK_PROVIDE_LOCAL_INFO_MEID             (0x0B)
#define STK_PROVIDE_LOCAL_INFO_RES_3GPP         (0x0C)

#define STK_IMEI_LEN                            (14)
#define STK_IMEISV_LEN                          (16)
#define STK_IMEI_TYPE                           (0x0A)
#define STK_IMEISV_TYPE                         (0x03)

#define STK_SEND_SMS_OP_ID                      (0x00)
#define STK_SEND_SS_OP_ID                       (0x01)
#define STK_SEND_USSD_OP_ID                     (0x02)
#define STK_SETUP_CALL_OP_ID                    (0x03)
#define STK_SETUP_CALL_DTMF_OP_ID               (0x04)
#define STK_SEND_DTMF_OP_ID                     (0x05)

#define STK_NV_ENABLED                          (0x0101)

/*profile 特别bit*/
#define SI_STK_LOCALINFO_NETINFO                (30)
#define SI_STK_LOCALINFO_NMR                    (31)
#define SI_STK_GET_READER_STATUS                (51)
#define SI_STK_GET_READER_ID                    (52)
#define SI_STK_TIMER_MM_START	                (56)
#define SI_STK_TIMER_MM_STOP                    (57)
#define SI_STK_LOCALINFO_TIME                   (58)
#define SI_STK_CALL_CONTROL                     (63)
#define SI_STK_LOCALINFO_LANGUAGE               (67)
#define SI_STK_LOCALINFO_TA                     (68)
#define SI_STK_LOCALINFO_ACCESS                 (71)

#define STK_OPID_VALUE                          STK_SEND_SMS_OP_ID

/* NAS支持最大呼叫数 */
#define STK_CALL_MAX_NUM                        (7)

/* 定义parse字符 */
#define STK_PAUSE_CHAR                          (0x0c)

/* 挂断所有呼叫的ID */
#define STK_ALL_CALL_ID                         (0)

/* DURATION定时器默认时长，等待NAS回复CONNECT消息 */
#define STK_SETUP_CALL_DURATION_DEFAULT         (60)

/* 发送DTMF时暂停的时间长度，单位为MS */
#define STK_SEND_DTMF_PAUSE_TIME                (3000)

/* STK重试呼叫的最大次数 */
#define STK_RECALL_TIME_MAX                     (3)

#define STK_BURSTDTMF_LEN                       (150)


#define STK_TIMER_MANAGEMENT_TIMER_START(TimerId,Length)    VOS_StopRelTimer(&gstSTKTimer[TimerId].stTimer);\
                                                            (VOS_VOID)VOS_StartRelTimer(&gstSTKTimer[TimerId].stTimer, MAPS_STK_PID,\
                                                                            Length, TimerId, 0, VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5)

#define STK_TIMER_MANAGEMENT_TIMER_STOP(TimerId)            VOS_StopRelTimer(&gstSTKTimer[TimerId].stTimer)


#define STK_PROTECT_TIMER_START(ulLength)                   VOS_StartRelTimer(&gstSTKProtectTimer, MAPS_STK_PID,\
                                                                            ulLength*1000, STK_PROTECT_TIMER_NAME, 0,VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5)

#define STK_PROTECT_TIMER_STOP                              VOS_StopRelTimer(&gstSTKProtectTimer)

#define STK_NMR_TIMER_START(ulLength)                       VOS_StartRelTimer(&gstSTKNMRTimer, MAPS_STK_PID,\
                                                                            ulLength*1000, STK_NMR_TIMER_NAME, 0, VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5)

#define STK_NMR_TIMER_STOP                                  VOS_StopRelTimer(&gstSTKNMRTimer)

#define STK_GETTA_TIMER_START                               VOS_StartRelTimer(&gstSTKGetTATimer, MAPS_STK_PID,\
                                                                            5000, STK_GETTA_TIMER_NAME, 0,VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5)

#define STK_GETTA_TIMER_STOP                                VOS_StopRelTimer(&gstSTKGetTATimer)

#define STK_IND_TIMER_START(ulLength)                       VOS_StartRelTimer(&gstSTKINDTimer, MAPS_STK_PID,\
                                                                            ulLength, STK_IND_TIMER_NAME, 0,VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5)

#define STK_IND_TIMER_STOP                                  VOS_StopRelTimer(&gstSTKINDTimer)

#define STK_REFRESH_CNF_TIMER_START(ulLength)               VOS_StartRelTimer(&gstSTKRefreshCnfTimer, MAPS_STK_PID,\
                                                                            ulLength, STK_REFRESH_TIMER_NAME, 0,VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5)

#define STK_REFRESH_CNF_TIMER_STOP                          VOS_StopRelTimer(&gstSTKRefreshCnfTimer)

/* Added by h59254 for V7R1C50 setup call 20120920 begin */
#define STK_SETUP_CALL_DURATION_TIMER_START(ulLength)       VOS_StartRelTimer(&gstSTKSetupCallDurationTimer, MAPS_STK_PID,\
                                                                            ulLength, STK_SETUP_CALL_DURATION_TIMER_NAME, 0,VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_10)

#define STK_SETUP_CALL_DURATION_TIMER_STOP                  VOS_StopRelTimer(&gstSTKSetupCallDurationTimer)

#define STK_DTMF_PAUSE_TIMER_START(ulLength)                VOS_StartRelTimer(&gstSTKDtmfPauseTimer, MAPS_STK_PID,\
                                                                            ulLength, STK_DTMF_PAUSE_TIMER_NAME, 0,VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_10)

/* Added by h59254 for V7R1C50 setup call 20120920 end */
#define STK_RESEND_TIMER_START(TimerId, TimerName, Length)  (VOS_VOID)VOS_StopRelTimer(TimerId);\
                                                            (VOS_VOID)VOS_StartRelTimer(TimerId, MAPS_STK_PID,\
                                                                            Length*1000, TimerName, 0, VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_5)

#define STK_RESEND_TIMER_STOP(TimerId)            VOS_StopRelTimer(TimerId)



#define STK_BCD2HEX(x) ((VOS_UINT8)((VOS_UINT8)(((x) & 0xF0)>>4) + (VOS_UINT8)(((x) & 0x0F)*0x0A)))

#define STK_HEX2BCD(x) (((x) < 10)?(VOS_UINT8)((x)<<4):((VOS_UINT8)((VOS_UINT8)(((x)/10) & 0x0F) + (VOS_UINT8)(((x)%10)<<4))))

#define STK_ARRAYSIZE(array)                (sizeof(array)/sizeof(array[0]))

/* 检查TAF回复的PS_USIM_ENVELOPE_CNF_STRU消息中，ucDataType的合法性 */
#define STK_IS_TAF_ENVELOPE_CNF_DATATYPE_VALID(ucDataType)   ( (SI_STK_ENVELOPE_PPDOWN == ucDataType)   \
                                                             ||(SI_STK_ENVELOPE_CBDOWN == ucDataType)   \
                                                             ||(SI_STK_ENVELOPE_CALLCRTL == ucDataType) \
                                                             ||(SI_STK_ENVELOPE_SMSCRTL == ucDataType)  \
                                                             ||(SI_STK_ENVELOPE_USSDDOWN == ucDataType)  )

/*******************************************************************************
  3 枚举定义
*******************************************************************************/
enum SI_STK_TIMERNAME
{
    STK_PROTECT_TIMER_NAME                  = 0,
    STK_TIMER_MANAGEMENT_TIMER1             = 1,
    STK_TIMER_MANAGEMENT_TIMER2             = 2,
    STK_TIMER_MANAGEMENT_TIMER3             = 3,
    STK_TIMER_MANAGEMENT_TIMER4             = 4,
    STK_TIMER_MANAGEMENT_TIMER5             = 5,
    STK_TIMER_MANAGEMENT_TIMER6             = 6,
    STK_TIMER_MANAGEMENT_TIMER7             = 7,
    STK_TIMER_MANAGEMENT_TIMER8             = 8,
    STK_IND_TIMER_NAME                      = 9,
    STK_REFRESH_TIMER_NAME                  = 10,
    STK_GETTA_TIMER_NAME                    = 11,
    STK_SETUP_CALL_DURATION_TIMER_NAME      = 12,
    STK_DTMF_PAUSE_TIMER_NAME               = 13,
    STK_NMR_TIMER_NAME                      = 14,
    STK_LOCIEVENT_TIMER_NAME                = 15,
    STK_ACCEVENT_TIMER_NAME                 = 16,
    SI_STK_TIMERNAME_BUTT
};
typedef VOS_UINT32 SI_STK_TIMERNAME_UINT32;

enum SI_STK_CMD_PROC_STATUS
{
    SI_STK_NORMAL_CNF                = 0,   /*正常回复*/
    SI_STK_WAITING_CNF               = 1,   /*等待回复*/
    SI_STK_TIMEOUT_CNF               = 2,   /*超时回复*/
    SI_STK_BUTT
};
typedef VOS_UINT32 SI_STK_CMD_PROC_STATUS_UINT32;

enum SI_STK_TAG_TYPE
{
    SI_TAG_TYPE_NULL                = 0,   /*数据结构不确定*/
    SI_TAG_TYPE_LV                  = 1,   /*LV 数据结构*/
    SI_TAG_TYPE_TLV                 = 2,   /*TLV数据结构*/
    SI_TAG_TYPE_V                   = 3,   /*V 数据结构*/
    SI_TAG_BUTT
};
typedef VOS_UINT8 SI_STK_TAG_TYPE_UINT8;

enum SI_DECODE_RESULT
{
    SI_STK_OK                       = COMMAND_PERFORMED_SUCCESSFULLY,
    SI_STK_CMDEND                   = PROACTIVE_UICC_SESSION_TERMINATED_BY_THE_USER,
    SI_STK_HANDLEERROR              = TERMINAL_CURRENTLY_UNABLE_TO_PROCESS_COMMAND,
    SI_STK_NOTSUPPORT               = COMMAND_BEYOND_TERMINALS_CAPABILITIES,
    SI_STK_TYPE_ERROR               = COMMAND_TYPE_NOT_UNDERSTOOD_BY_TERMINAL,
    SI_STK_DATA_ERROR               = COMMAND_DATA_NOT_UNDERSTOOD_BY_TERMINAL,
    SI_STK_OK_WAITRP                = 0x40,
    SI_STK_NORP                     = 0x41,
    SI_DECODE_RESULT_BUTT
};

enum SI_CODEDATA_TYPE
{
    SI_CODE_TR_DATA                 = 0x00,
    SI_CODE_EVENT_DATA              = 0x01,
    SI_CODE_ENVELOPE_DATA           = 0x02,
    SI_CODE_DATA_BUTT
};
typedef VOS_UINT32 SI_CODEDATA_TYPE_UINT32;

enum SI_STK_DATA_TAG
{
    COMMAND_DETAILS_TAG             = 0x01,
    DEVICE_IDENTITY_TAG             = 0x02,
    RESULT_TAG                      = 0x03,
    DURATION_TAG                    = 0x04,
    ALPHA_IDENTIFIER_TAG            = 0x05,
    ADDRESS_TAG                     = 0x06,
    CAP_CFG_PARA_TAG                = 0x07,
    SUBADDRESS_TAG                  = 0x08,
    SS_STRING_TAG                   = 0x09,
    USSD_STRING_TAG                 = 0x0A,
    SMS_TPDU_TAG                    = 0x0B,
    CELL_BROADCAST_PAGE_TAG         = 0x0C,
    TEXT_STRING_TAG                 = 0x0D,
    TONE_TAG                        = 0x0E,
    ITEM_TAG                        = 0x0F,
    ITEM_IDENTIFIER_TAG             = 0x10,
    RESPONSE_LENGTH_TAG             = 0x11,
    FILE_LIST_TAG                   = 0x12,
    LOCATION_INFORMATION_TAG        = 0x13,
    IMEI_TAG                        = 0x14,
    HELP_REQUEST_TAG                = 0x15,
    NET_MEASUREMENT_RESULTS_TAG     = 0x16,
    DEFAULT_TEXT_TAG                = 0x17,
    MS_NEXTACTION_INDICATOR_TAG     = 0x18,
    EVENT_LIST_TAG                  = 0x19,
    CAUSE_TAG                       = 0x1A,
    LOCATION_STATUS_TAG             = 0x1B,
    TRANSACTION_IDENTIFIER_TAG      = 0x1C,
    BCCH_CHANNEL_LIST_TAG           = 0x1D,
    ICON_IDENTIFIER_TAG             = 0x1E,
    ITEM_ICONID_LIST_TAG            = 0x1F,
    CARD_READER_STATUS_TAG          = 0x20,
    CARD_ATR_TAG                    = 0x21,
    C_APDU_TAG                      = 0x22,
    R_APDU_TAG                      = 0x23,
    TIMER_IDENTIFIER_TAG            = 0x24,
    TIMER_VALUE_TAG                 = 0x25,
    DATETIME_AND_TIMEZONE_TAG       = 0x26,
    CALL_CONTROL_REQACTION_TAG      = 0x27,
    AT_COMMAND_TAG                  = 0x28,
    AT_RESPONSE_TAG                 = 0x29,
    BC_REPEAT_INDICATOR_TAG         = 0x2A,
    IMMEDIATE_RESPONSE_TAG          = 0x2B,
    DTMF_STRING_TAG                 = 0x2C,
    LANGUAGE_TAG                    = 0x2D,
    TIMING_ADVANCE_TAG              = 0x2E,
    AID_TAG                         = 0x2F,
    BROWSER_IDENTITY_TAG            = 0x30,
    URL_TAG                         = 0x31,
    BEARER_TAG                      = 0x32,
    PROVISIONING_REFFILE_TAG        = 0x33,
    BROWSER_TERMINATION_CAUSE_TAG   = 0x34,
    BEARER_DESCRIPTION_TAG          = 0x35,
    CHANNEL_DATA_TAG                = 0x36,
    CHANNEL_DATA_LENGTH_TAG         = 0x37,
    CHANNEL_STATUS_TAG              = 0x38,
    BUFFER_SIZE_TAG                 = 0x39,
    CARD_READER_IDENTIFIER_TAG      = 0x3A,
    RFU_3B                          = 0x3B,
    UICC_TERMINAL_TRAN_LEVEL_TAG    = 0x3C,
    RFU_3D                          = 0x3D,
    OTHER_ADDR_TAG                  = 0x3E,
    ACCESS_TECHNOLOGY_TAG           = 0x3F,
    DISPLAY_PARAMETERS_TAG          = 0x40,
    SERVICE_RECORD_TAG              = 0x41,
    DEVICE_FILTER_TAG               = 0x42,
    SERVICE_SEARCH_TAG              = 0x43,
    ATTRIBUTE_INFORMATION_TAG       = 0x44,
    SERVICE_AVAILABILITY_TAG        = 0x45,
    ESN_TAG                         = 0x46,
    NETWORK_ACCESS_NAME_TAG         = 0x47,
    CDMA_SMS_TPDU                   = 0x48,
    REMOTE_ENTITY_ADDRESS_TAG       = 0x49,
    I_WLAN_ID_TAG                   = 0x4A,
    I_WLAN_ACCESS_STATUS_TAG        = 0x4B,
    RFU_4C                          = 0x4C,
    RFU_4D                          = 0x4D,
    RFU_4E                          = 0x4E,
    RFU_4F                          = 0x4F,
    TEXT_ATTRIBUTE_TAG              = 0x50,
    ITEM_TEXT_ATTRIBUTE_LIST_TAG    = 0x51,
    PDP_CONTEXT_ACTIVATION_PAR_TAG  = 0x52,
    RFU_53                          = 0x53,
    RFU_54                          = 0x54,
    CSG_CELL_SELEC_STATUS_TAG       = 0x55,
    CSG_ID_TAG                      = 0x56,
    HNB_NAME_TAG                    = 0x57,
    RFU_58                          = 0x58,
    RFU_59                          = 0x59,
    RFU_5A                          = 0x5A,
    RFU_5B                          = 0x5B,
    RFU_5C                          = 0x5C,
    RFU_5D                          = 0x5D,
    RFU_5E                          = 0x5E,
    RFU_5F                          = 0x5F,
    RFU_60                          = 0x60,
    CDMA_ECC_TAG                    = 0x61,
    IMEISV_TAG                      = 0x62,
    BATTERY_STATE_TAG               = 0x63,
    BROWSING_STATUS_TAG             = 0x64,
    NETWORK_SEARCH_MODE_TAG         = 0x65,
    FRAME_LAYOUT_TAG                = 0x66,
    FRAMES_INFORMATION_TAG          = 0x67,
    FRAME_IDENTIFIER_TAG            = 0x68,
    UTRAN_MEASUREMENT_TAG           = 0x69,
    MMS_REFERENCE_TAG               = 0x6A,
    MMS_IDENTIFIER_TAG              = 0x6B,
    MMS_TRANSFER_STATUS_TAG         = 0x6C,
    MEID_TAG                        = 0x6D,
    MMS_CONTENT_ID_TAG              = 0x6E,
    MMS_NOTIFICATION_TAG            = 0x6F,
    LAST_ENVELOPE_TAG               = 0x70,
    RFU_62                          = 0x71,
    PLMNWACT_LIST_TAG               = 0x72,
    ROUTING_AREA_INFO_TAG           = 0x73,
    ATTACH_TYPE_TAG                 = 0x74,
    REJETION_CAUSE_CODE_TAG         = 0x75,
    GEOGRAPH_LOCAL_PARA_TAG         = 0x76,
    GAD_SHAPES_TAG                  = 0x77,
    NMEA_SENTENCE_TAG               = 0x78,
    PLMN_LIST_TAG                   = 0x79,
    RFU_7A                          = 0x7A,
    RFU_7B                          = 0x7B,
    EPSPDN_ACTIVE_PARA_TAG          = 0x7C,
    TRACKING_AREA_ID_TAG            = 0x7D,
    CSG_ID_LIST_TAG                 = 0x7E,
    SI_STK_TAG_BUTT
};

enum SI_STK_LOCK_ENUM
{
    SI_STK_PROUNLOCK                = 0x00,
    SI_STK_PROLOCK                  = 0x01,
    SI_STK_LOCK_BUTT
};
typedef VOS_UINT32 SI_STK_LOCK_ENUM_UINT32;

enum SI_STK_TIMERSTATE_ENUM
{
    SI_STK_TIMERSTOP                = 0x00,
    SI_STK_TIMERRUN                 = 0x01,
    SI_STK_TIMERBUTT
};
typedef VOS_UINT32 SI_STK_TIMERSTATE_ENUM_UINT32;

enum SI_STK_STR_CHANGEMODE_ENUM
{
    SI_STK_STR_CHANGEBIG            = 0x00,
    SI_STK_STR_CHANGESMALL          = 0x01,
    SI_STK_STR_CHANGE_BUTT
};
typedef VOS_UINT32 SI_STK_STR_CHANGEMODE_ENUM_UINT32;


/* 沃达丰Dual IMSI 切换定制需求 */
enum SI_STK_IMSICHG_STATE_ENUM
{
    SI_STK_IMSICHG_NULL             = 0x00,
    SI_STK_IMSICHG_SELECT_ITEM      = 0x01,
    SI_STK_IMSICHG_REFRESH          = 0x02,
    SI_STK_IMSICHG_STATE_BUTT
};
typedef VOS_UINT32 SI_STK_IMSICHG_STATE_ENUM_UINT32;

/*****************************************************************************
 枚举名    : SI_STK_DTMF_SEND_ENUM_UINT32
 枚举说明  : STK模块记录的DTMF SEND状态
*****************************************************************************/
enum SI_STK_DTMF_SEND_ENUM
{
    SI_STK_DTMF_SEND_OK             = 0,    /* 当前发送数据成功 */
    SI_STK_DTMF_SEND_ERR            = 1,    /* 当前发送数据失败 */
    SI_STK_DTMF_SEND_FINISH         = 2,    /* 当前发送数据完成 */
    SI_STK_DTMF_SEND_REJECT         = 3,    /* 当前发送数据被拒 */
    SI_STK_DTMF_SEND_BUTT,
};
typedef VOS_UINT32 SI_STK_DTMF_SEND_ENUM_UINT32;

/*****************************************************************************
 枚举名    : SI_STK_CALL_STATUS_ENUM
 枚举说明  : STK模块记录的当前呼叫状态
*****************************************************************************/
enum SI_STK_CALL_ID_STATUS_ENUM
{
    SI_STK_CALL_ID_STATUS_IDLE          =   0x5A,           /* 当前处于空闲状态 */
    SI_STK_CALL_ID_STATUS_BUSY          =   0xA5,           /* 当前处于呼叫状态(包括DTMF发送过程中) */
    SI_STK_CALL_ID_STATUS_BUTT,
};
typedef VOS_UINT8 SI_STK_CALL_ID_STATUS_ENUM_UINT8;

/*****************************************************************************
 枚举名    : SI_STK_CALL_STATUS_ENUM
 枚举说明  : STK模块记录的当前呼叫状态
*****************************************************************************/
enum SI_STK_CALL_STATUS_ENUM
{
    SI_STK_CALL_STATUS_WAIT_IDLE        =   0,              /* 当前处于空闲状态,当前无SETUP CALL主动命令，不处理NAS消息 */
    SI_STK_CALL_STATUS_WAIT_CONNECT     =   1,              /* 当前处于等待CONNECT消息 */
    SI_STK_CALL_STATUS_WAIT_DISCONNECT  =   2,              /* 当前处于等待DISCONNECT消息 */
    SI_STK_CALL_STATUS_WAIT_HOLD        =   3,              /* 当前处于HOLD消息 */
    SI_STK_CALL_STATUS_SEND_DTMF        =   4,              /* 当前处于发送DTMF过程中 */
    SI_STK_CALL_STATUS_WAIT_DURTIMEROUT = 	5,              /* 当前处于等待Duration超时过程中 */
    SI_STK_CALL_STATUS_BUTT,
};
typedef VOS_UINT32 SI_STK_CALL_STATUS_ENUM_UINT32;

/*****************************************************************************
 枚举名    : SI_STK_SETUP_CALL_COMMAND_QUALIFIER
 枚举说明  : SETUP CALL主动命令的CQ值
*****************************************************************************/
enum SI_STK_SETUP_CALL_COMMAND_QUALIFIER_ENUM
{
    SI_STK_SETUP_CALL_ONLY_NOT_BUSY_ON_ANOTHER_CALL         =   0,
    SI_STK_SETUP_CALL_ONLY_NOT_BUSY_ON_ANOTHER_CALL_REDIAL  =   1,
    SI_STK_SETUP_CALL_PUTTING_ALL_OTHER_CALL_ON_HOLD        =   2,
    SI_STK_SETUP_CALL_PUTTING_ALL_OTHER_CALL_ON_HOLD_REDIAL =   3,
    SI_STK_SETUP_CALL_DISCONNECT_ALL_OTHER_CALL             =   4,
    SI_STK_SETUP_CALL_DISCONNECT_ALL_OTHER_CALL_REDIAL      =   5,
    SI_STK_SETUP_CALL_BUTT,
};
typedef VOS_UINT8 SI_STK_SETUP_CALL_COMMAND_QUALIFIER_ENUM_UINT8;


enum SI_STK_SEARCH_MODE_ENUM
{
    SI_STK_SEARCH_MODE_AUTO         = TAF_PH_PLMN_SEL_MODE_AUTO,
    SI_STK_SEARCH_MODE_MANUAL       = TAF_PH_PLMN_SEL_MODE_MANUAL,
    SI_STK_SEARCH_MODE_BUTT
};


enum SI_STK_HOOK_MSG_NAME_ENUM
{
     STK_TAF_CS_SERVICE_STATUS          = 0xFF00,    /* CS服务状态勾包 */
     STK_TAF_CBS_DCS                    = 0xFF01,    /* USSD DCS编码钩包 */

};
typedef VOS_UINT32 SI_STK_HOOK_MSG_NAME_ENUM_UINT32;


enum SI_STK_TEXT_STRING_DCS_ENUM
{
     SI_STK_TEXT_STRING_DCS_7BIT        = 0x00,
     SI_STK_TEXT_STRING_DCS_8BIT        = 0x04,
     SI_STK_TEXT_STRING_DCS_UCS         = 0x08,
     SI_STK_TEXT_STRING_DCS_BUTT
};
typedef VOS_UINT8 SI_STK_TEXT_STRING_DCS_ENUM_UINT8;

/*C+L新增，电信要求Event需要能够重发，仅为此项目专用，不能扩展 */
enum SI_STK_ENVELOPE_RESENT_FLAG
{
    SI_STK_ENVELOPE_SENT_ONETIME        = 0,
    SI_STK_ENVELOPE_LOCI_RESEND         = 1,    /*本地事件需要重发*/
    SI_STK_ENVELOPE_ACC_RESEND          = 2,    /*接入技术改变需要重发*/
    SI_STK_ENVELOPE_RESENT_BUTT
};
typedef VOS_UINT32 SI_STK_ENVELOPE_RESENT_FLAG_UINT32;

enum SI_STK_CALLSTATE_FLAG
{
    SI_STK_CALLSTATE_OFF                = 0x0,
    SI_STK_CALLSTATE_ON                 = 0xAA,
    SI_STK_CALLSTATE_BUTT
};
typedef VOS_UINT8 SI_STK_CALLSTATE_FLAG_UINT8;

/*****************************************************************************
  4 单一数据结构定义
*****************************************************************************/
typedef struct
{
    VOS_UINT32                      ulLen;
    VOS_UINT8                       *pValue;
}SI_SAT_LV_STRU;

typedef struct
{
    VOS_UINT8                       ucTag;
    VOS_UINT8                       ucLen;
    VOS_UINT8                       aucRsv[2];
    VOS_UINT8                       *pValue;
}SI_SAT_TLV_STRU;

typedef struct
{
    VOS_UINT32                      ulOpInfo;
    VOS_UINT8                       aucDecode[500];
}SI_SAT_COMMDATA_STRU;

typedef struct
{
    VOS_UINT32                      ulSenderPID;
    SI_STK_REQMSG_ENUM_UINT32       enSTKEventType;
    SI_STK_LOCK_ENUM_UINT32         enSTKLock;
    VOS_UINT16                      usClientID;
    VOS_UINT8                       ucOpID;
    VOS_UINT8                       ucReserved;
}SI_STK_REQ_UINT_STRU;

typedef struct
{
    VOS_UINT8                       ucUsatTag;
    VOS_UINT8                       aucReserved[3];
    SI_STK_COMMAND_DETAILS_STRU     CmdDetail;
    SI_STK_DEVICE_IDENTITIES_STRU   CmdDevice;
    SI_STK_SET_UP_MENU_STRU         stSetUpMenu;
}SI_SAT_SetUpMenu_DATA_STRU;

typedef struct
{
    VOS_UINT8                       ucOpId;
    VOS_UINT8                       ucCallId;
    VOS_UINT8                       aucRsv[2];
    VOS_UINT32                      ulDtmfLen;
    VOS_UINT32                      ulCurDtmfPtr;
    SI_CHAR                         acDtfm[512];
}SI_SAT_DTMF_DATA_STRU;

typedef struct
{
    VOS_UINT16                       usCmdType;
    VOS_UINT16                       usBitNum;
}SI_STK_CMD_PROFILE_CFG_STRU;

typedef struct
{
    VOS_UINT8                       ucDataTag;  /*用于存放Tag的值*/
    SI_STK_TAG_TYPE_UINT8           ucTagType;  /*用于指示Tag的编解码类型*/
    VOS_UINT8                       ucSTRULen;  /*用于表明数据结构的大小*/
    VOS_UINT8                       ucValueLen; /*用于指明当前V型数据实际大小*/
}SI_STK_DATADECODE_STRU;

typedef struct
{
    VOS_UINT32                      ulCmdType;   /*存放STK命令的类型*/
    VOS_UINT32                      ulTagLen;    /*存放编解码的列表长度*/
    VOS_UINT8                       *pucTagList; /*存放编解码列表地址*/
}SI_STK_TAGLIST_STRU;

/*****************************************************************************
 结构名    : SI_STK_CALL_STATUS_STRU
 结构说明  : STK模块呼叫状态表结构
*****************************************************************************/
typedef struct
{
    MN_CALL_ID_T                        callId;         /* 呼叫ID */
    SI_STK_CALL_ID_STATUS_ENUM_UINT8    enStatus;       /* 呼叫状态 */
    TAF_CALL_VOICE_DOMAIN_ENUM_UINT8    enVoiceDomain;  /* VOICE DOMAIN */
    VOS_UINT8                           ucRsv;
}SI_STK_CALL_STATUS_STRU;

/*****************************************************************************
 结构名    : SI_STK_SETUP_CALL_DTMF_INFO_STRU
 结构说明  : STK模块SETUP CALL中DTMF控制结构
*****************************************************************************/
typedef struct
{
    VOS_UINT8                          *pucDtmfCharacter;           /* DTMF字符串 */
    VOS_UINT32                          ulDtmfCharacterRemain;      /* 待发送的DTMF字符串 */
    VOS_UINT32                          ulDtmfCharacterCurrSnd;     /* 当前发送DTMF字符 */
}SI_STK_SETUP_CALL_DTMF_INFO_STRU;

/*****************************************************************************
 结构名    : SI_STK_SETUP_CALL_CTRL_STRU
 结构说明  : STK模块呼叫控制结构
*****************************************************************************/
typedef struct
{
    SI_STK_CALL_STATUS_ENUM_UINT32      enCallStatus;                                       /* SETUP CALL主动命令处理状态 */
    SI_STK_CALL_STATUS_STRU             astStkCallStatusTable[STK_CALL_MAX_NUM];            /* STK模块呼叫状态表 */
    MN_CALL_ID_T                        ucSetupCallId;                                      /* SETUP CALL发起呼叫的ID，由NAS分配 */
    VOS_UINT8                           ucRecallTime;                                       /* 重新呼叫的次数 */
}SI_STK_SETUP_CALL_CTRL_STRU;

/*****************************************************************************
 结构名    : SI_STK_CBP_SETUP_EVENT_LIST_STRU
 结构说明  : L+C共SIM卡功能保存EVENT LIST
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          ulProactiveCmdLen;
    VOS_UINT8                          *pucCmd;
}SI_STK_CBP_SETUP_EVENT_LIST_STRU;

typedef VOS_UINT32 (*pfSATCmdDecode)(VOS_UINT8  *pucCmd, VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU* pstResult);

typedef VOS_UINT32 (*pfSATCmdPro)(SI_STK_DATA_INFO_STRU *pEvent);

typedef struct
{
    SI_STK_CMD_TYPE                 ulCmdType;  /*处理消息类型*/
    pfSATCmdDecode                  pfDecodeFun;/*对应解码函数*/
    pfSATCmdPro                     pfProcFun;  /*对应处理函数*/
}SI_STK_CMDHANDLE_LIST_STRU;

typedef struct
{
    SI_STK_TIMERSTATE_ENUM_UINT32   enTimerState;
    HTIMER                          stTimer;
    VOS_UINT8                       ucHour;
    VOS_UINT8                       ucMin;
    VOS_UINT8                       ucSec;
    VOS_UINT8                       Reserve;
}SI_STK_TIMER_STRU;


typedef struct
{
    SI_STK_COMMAND_DETAILS_STRU     stCmdDetail;
    SI_STK_DEVICE_IDENTITIES_STRU   CmdDevice;
}SI_STK_PROACTIVE_CMD_INFO_STRU;

typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                      MsgName;
    SI_CLIENT_ID_T                  ClientId;
    VOS_UINT8                       OpId;
    VOS_UINT8                       Rsv;
    SI_STK_CMD_TYPE                 SatType;
    VOS_UINT32                      Datalen;
    VOS_UINT8                       Data[8];
}SI_STK_REQ_STRU;

typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                      ulMsgName;
    VOS_UINT8                       aucPWD[4];
}SI_STK_SS_PWD_REQ_STRU;

/* 沃达丰Dual IMSI 切换定制需求 */
typedef struct
{
    SI_STK_IMSICHG_STATE_ENUM_UINT32 enIMSIChgState;   /* 沃达丰IMSI切换定制需求状态记录 */
    VOS_UINT32                       ulCurImsiSign;    /* 取值0,1，与两个IMSI相对应 */
    VOS_UINT8                        aucOldImsi[9];    /* 切换前的IMSI */
    VOS_UINT8                        ucOldItemId;      /* 上一次选择的ITEM ID */
    VOS_UINT8                        ucTryFlag;        /* 记录是否已尝试过一次切换 */
    VOS_UINT8                        ucCycleFlag;      /* 标记是否在状态机中轮转 */
}SI_STK_IMSICHG_CTRL_STRU;

typedef  VOS_UINT32 (*STKIMSIPROC)(PS_SI_MSG_STRU *pMsg);

/* IMSI 切换定制需求状态机结构 */
typedef struct
{
    SI_STK_IMSICHG_STATE_ENUM_UINT32 enIMSIChgState;   /* 沃达丰IMSI切换定制需求状态记录 */
    STKIMSIPROC                      pIMSIChgProc;     /* 沃达丰IMSI切换定制需求对应状态处理函数 */
}SI_STK_IMSICHG_PROC_STRU;

/* 缓存下发的Envelope命令 */
typedef struct
{
    VOS_UINT32                       ulCmdLen;        /* Envelope命令长度 */
    VOS_UINT8                        aucCmdData[SI_STK_DATA_MAX_LEN]; /* Envelope命令内容 */
}SI_STK_ENVELOPE_CMD_STRU;

typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                      ulMsgName;
    VOS_UINT8                       aucData[4];
}SI_STK_ATCNF_MSG_STRU;

typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                      ulMsgName;
    VOS_UINT16                       usClientId;
    VOS_UINT8                       ucOpId;
    VOS_UINT8                       ucValue;
    VOS_UINT8                       aucData[4];
}SI_STK_TAF_SENDMSG_STRU;

typedef struct
{
    VOS_MSG_HEADER
    VOS_UINT32                      ulMsgId;
    MN_MSG_EVENT_ENUM_U32           enEventType;
    VOS_UINT8                       aucEvent[4];
}SI_STK_TAFCNF_MSG_STRU;

/* 内部事件记录结构声明*/
typedef struct
{
    VOS_UINT32                      OP_MTCall:1;
    VOS_UINT32                      OP_CallConnect:1;
    VOS_UINT32                      OP_CallDisconnet:1;
    VOS_UINT32                      OP_LociStatus:1;
    VOS_UINT32                      OP_AccChange:1;
    VOS_UINT32                      OP_NetSearchChg:1;
    VOS_UINT32                      OP_NetRej:1;
    VOS_UINT32                      OP_Reserved:25;
}SI_STK_EVENT_STATE_STRU;

typedef struct
{
    MSG_HEADER_STRU                  MsgHeader;
    VOS_UINT32                       ulCsServiceStatus;
}STK_CS_SERVICE_STRU;


typedef struct
{
    VOS_UINT8                           ucInputDcs;
    VOS_UINT8                           ucRsv;
    VOS_UINT16                          usStringLen;
    MN_MSG_CBDCS_CODE_STRU              stCbDcs;
    VOS_UINT8                           aucString[4];
}STK_CBS_DCS_STRU;
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
typedef struct
{
    NAS_STK_UPDATE_TYPE_ENUM_UINT8          enOrigType;     /* MM子层传递给STK的原始type */
    SI_STK_UPDATE_ATTACH_TYPE_ENUM_UINT8    enDestType;     /* 根据31111 8.92  Update/Attach Type，原始type所映射到的目的type */
    VOS_UINT16                              usRsv;
}STK_UPDATA_ATTACH_TYPE_TRANSFER_STRU;
#endif

typedef struct
{
    HTIMER                                  stSTKResendTimer;
    VOS_UINT32                              ulSTKResendCounter;
    VOS_UINT32                              ulDataLen;
    VOS_UINT8                               aucData[STK_ENVELOPE_MAX_LEN];
}STK_EVENTDATA_RESEND_STRU;

/*****************************************************************************
 结构名    : SI_STK_CARDINFO_STRU
 结构说明  : STK模块保存USIM卡信息数据结构
*****************************************************************************/
typedef struct
{
    USIMM_PHYCARD_TYPE_ENUM_UINT32      enPhyType;
    USIMM_CARD_TYPE_ENUM_UINT32         enCardType;
    USIMM_CARDAPP_SERVIC_ENUM_UINT32    enSvcStatus;
}SI_STK_CARDINFO_STRU;

typedef struct
{
    VOS_UINT32                          ulReqFlag;
    VOS_UINT32                          ulDataLen;
    VOS_UINT8                           aucData[SI_STK_DATA_MAX_LEN];
}SI_STK_FLAGLVDATA_STRU;

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
typedef struct
{
    VOS_UINT32                          ulIsPressNeed;
    VOS_UINT32                          ulSmsNum;
    VOS_UINT16                          usUserDataStartByte;
    VOS_UINT16                          usUserDataLen;
    TAF_XSMS_BD_MSG_USER_DATA_STRU      stUserData;
}SI_STK_CDMA_SMS_CTRL_PARA_STRU;
#endif
#endif

typedef struct
{
    SI_STK_CALLSTATE_FLAG_UINT8         enCSCallState;
    SI_STK_CALLSTATE_FLAG_UINT8         enPSCallState;
    VOS_UINT16                          usRsv;
}SI_STK_CALLSTATE_STRU;

typedef struct
{
    VOS_UINT32                          Mcc;
    VOS_UINT32                          Mnc;
}STK_PLMN_ID_STRU;

/*****************************************************************************
 结构名    : SI_STK_SYS_INFO_STRU
 结构说明  : STK模块维护的系统信息字段结构
*****************************************************************************/
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
typedef struct
{
    SI_STK_ACCESS_TECH_ENUM_UINT8       enCurRat;       /* 接入技术 */
    SI_STK_SEARCH_MODE_ENUM_UINT8       enSrchMode;     /* 搜网模式 */
    NAS_STK_UTRAN_MODE_ENUM_UINT8       enUtranMode;    /* TDD或FDD模式 */
    VOS_UINT8                           ucRsv;
    VOS_UINT32                          ulCellId;       /* 小区ID */
    STK_PLMN_ID_STRU                    stCurPlmnId;    /* PLMN */
    VOS_UINT16                          usLac;
}SI_STK_SYS_INFO_STRU;
#endif
/*****************************************************************************
 结构名    : SI_STK_NAS_INFO_STRU
 结构说明  : STK模块维护的NAS信息字段结构
*****************************************************************************/
#if (OSA_CPU_CCPU == VOS_OSA_CPU)

typedef struct
{
    VOS_UINT32                          ulEventMsgFlag;
    /* 使用STK自定义的服务状态，不使用NAS消息中的值，在更新时需要转换 */
    SI_STK_SERVICE_STATUS_ENUM_UINT32   enServiceStatus;
    SI_STK_SERVICE_STATUS_ENUM_UINT32   enCsServiceStatus;
    SI_STK_SYS_INFO_STRU                stSysInfo;
}SI_STK_NAS_INFO_STRU;

typedef struct
{
    /* 使用STK自定义的服务状态，不使用NAS消息中的值，在更新时需要转换 */
    SI_STK_SERVICE_STATUS_ENUM_UINT32   enServiceStatus;
    MMA_STK_1X_SYS_INFO_STRU            stSysInfo;
}SI_STK_CDMANAS_SYSINFO_STRU;
#endif


typedef VOS_VOID (*PFSISTKPIDMSGPROC)(PS_SI_MSG_STRU *pMsg);


typedef struct
{
    VOS_UINT32                          ulMsgPid;
    PFSISTKPIDMSGPROC                   pProcFunc;        /* 处理函数 */
}SI_STK_PIDMSGPROC_FUNC;


typedef VOS_VOID (*PFSISTKCSCALLBACKPROC)(MN_CALL_EVENT_ENUM_U32 enEventType, MN_CALL_INFO_STRU *pstEvent);


typedef struct
{
    MN_CALL_EVENT_ENUM_U32              enEventType;
    PFSISTKCSCALLBACKPROC               pProcFunc;        /* 处理函数 */
}SI_STK_CSCALLBACKPROC_FUNC;


typedef VOS_UINT32 (*PFSISTKPROVIDELOCALPROC)(SI_STK_DATA_INFO_STRU *pCmdData);


typedef struct
{
    VOS_UINT32                          ucCommandQua;
    PFSISTKPROVIDELOCALPROC             pProcFunc;        /* 处理函数 */
}SI_STK_PROVIDELOCALPROC_FUNC;

/*****************************************************************************
  6 全局变量声明
*****************************************************************************/
extern VOS_UINT8                        g_ucSendSSType;

extern VOS_UINT8                        g_ucSendMsgNum;

extern VOS_UINT8                        g_ucReceCBNum;

extern SI_STK_DATA_INFO_STRU            gstUsatCmdDecode;

extern HTIMER                           gstSTKProtectTimer;

extern HTIMER                           gstSTKGetTATimer;

extern HTIMER                           gstSTKINDTimer;

extern HTIMER                           gstSTKRefreshCnfTimer;

/* Added by h59254 for V7R1C50 setup call 20120920 begin */
extern HTIMER                           gstSTKSetupCallDurationTimer;

extern HTIMER                           gstSTKDtmfPauseTimer;
/* Added by h59254 for V7R1C50 setup call 20120920 end */

extern HTIMER                           gstSTKNMRTimer;

extern VOS_UINT8                        gucSTKRefreshQua;

extern USIMM_STK_CFG_STRU               g_stSTKProfileContent;

extern SI_STK_REQ_UINT_STRU             gstSTKCtrlStru;

extern SI_STK_TIMER_STRU                gstSTKTimer[STK_TIMER_MAX_NUM];

extern SI_SAT_SetUpMenu_DATA_STRU       gstSetUpMenuData;

extern SI_STK_IMSICHG_CTRL_STRU         gstSTKIMSIChgCtrl;/* IMSI切换控制变量 */

extern SI_STK_IMSICHG_MATCH_STRU        gstSTKIMSIMatch; /* NV项读出的IMSI切换匹配码流 */

extern SI_STK_ENVELOPE_CMD_STRU         gstSTKEnvelopeCmd; /* 缓存Envelope命令 */

extern VOS_SEM                          gulSTKApiSmId;

extern SI_STK_TAGLIST_STRU              gastSTKDecodeList[24];

extern SI_STK_TAGLIST_STRU              gastRespCodeList[8];

extern SI_STK_TAGLIST_STRU              gastEventCodeList[16];

extern SI_STK_TAGLIST_STRU              gastEnvelopeList[7];

extern SI_STK_TAGLIST_STRU              gastEnvelopeDecodeList[2];

extern SI_STK_CMD_PROFILE_CFG_STRU      gastSTKCmdProfileCfg[31];

extern SI_STK_CMD_PROFILE_CFG_STRU      gastSTKEventProfileCfg[15];

extern VOS_UINT16                       gusSTKSMSIndEnable;

extern VOS_UINT8                        *g_pucSTKSndCodeAddr;

extern SI_STK_CMD_PROC_STATUS_UINT32    g_enTACmdStatus;

extern SI_STK_SETUP_CALL_CTRL_STRU      g_stStkSetupCallCtrl;

extern SI_SAT_DTMF_DATA_STRU            g_stSTKDtmfData;

extern VOS_UINT8                        g_ucCsinCurcRptCfg;

extern SI_STK_EVENT_STATE_STRU          g_stSTKEventState;

extern MODEM_ID_ENUM_UINT16             g_enSTKCurCSModem;
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
extern SI_STK_NAS_INFO_STRU             g_stStkSysInfo;
#endif
extern SI_STK_CBP_SETUP_EVENT_LIST_STRU g_stCbpEventList;

extern SI_STK_CARDINFO_STRU             g_stStkCardInfo;

extern VOS_BOOL                         g_bSTKAttResetFlag;

extern SI_STK_CALLSTATE_STRU            g_stSTKCallState;

#if (VOS_WIN32 == VOS_OS_VER) /* UT工程编译不过,通过编译宏区分 */
extern MN_MSG_RAW_TS_DATA_STRU            *f_pstStkMsgRawTsDataInfo;
extern MN_MSG_SEND_PARM_STRU              *f_pstSendDirectParm;
#endif

extern SI_STK_CMD_PROC_STATUS_UINT32       g_enNMRCmdStatus;

extern TAF_NV_LC_CTRL_PARA_STRU            g_stLCEnableCfg;

extern STK_EVENTDATA_RESEND_STRU           g_astEventResend[2];

extern STK_FEATURE_CFG_STRU                g_stSTKFeatureCnf;

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
extern VOS_UINT8                           g_ucIsXsmsInBuffFlag;   /* FALSE: NO IN BUFF
                                                           TRUR : ONE IN BUFF*/
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
extern TAF_XSMS_MESSAGE_STRU              *g_pstStkSendXsmsBuff;

extern TAF_XSMS_MESSAGE_STRU              *g_pstStkRcvXsmsBuff;

extern SI_STK_CDMANAS_SYSINFO_STRU         g_stStkCdmaSysInfo;
#endif

extern VOS_UINT8                           g_ucIsRatCdma;
#endif

/*****************************************************************************
  6 函数声明
*****************************************************************************/
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
/* STKComm.c */
extern VOS_UINT32 SI_STK_StrStr(VOS_UINT8 *pucStr1, VOS_UINT8 *pucStr2 , VOS_UINT32 ulStrlen1, VOS_UINT32 ulStrlen2);

extern VOS_UINT32 SI_STKFindTag(VOS_UINT8 ucTag, VOS_UINT8 *pucData, VOS_UINT32 ulDataLen,VOS_UINT32 ulFindNum);

extern VOS_VOID SI_STKDecodeTagList(VOS_UINT8 *pucCmdData,VOS_UINT8 *pucTagList,VOS_UINT32 ulListLen, SI_SAT_COMMDATA_STRU *pstDec);

extern VOS_VOID SI_STK_InitEnvelope(VOS_UINT8 *pCmdData, SI_STK_DEVICE_IDENTITIES_STRU *pstDiInfo, VOS_UINT8 ucCmdType);

extern VOS_UINT32 SI_STKCommCodeData(VOS_UINT8 *pucCmdData,SI_CODEDATA_TYPE_UINT32 enDataType, VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU *pstSrc);

extern VOS_UINT32 SI_STKTagDataFree(VOS_UINT8 ucTag, VOS_VOID *pData);

extern VOS_VOID SI_STK_SndProactiveCmd2CBP(VOS_UINT32 ulCmdLen, VOS_UINT8 *pucCmd);

extern VOS_UINT32 SI_STKCommDecodeData(VOS_UINT8 *pucCmdData,VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU *pstDec);

extern VOS_UINT32 SI_STK_SendTerminalResponseReqMsg(VOS_UINT8 ucLen,VOS_UINT8 *pucRsp, USIMM_TR_PARA_ENUM_32 ulPara);

extern VOS_UINT32 SI_STK_SendResetReqMsg(USIMM_STK_COMMAND_DETAILS_STRU  *pstCMDDetail,  USIMM_RESET_INFO_STRU *pstRstInfo);

extern VOS_VOID SI_STKSimpleResponseData(SI_STK_COMMAND_DETAILS_STRU *pstCMDInfo, VOS_UINT32 ulDataLen, VOS_UINT8 *pucData);

extern SI_VOID SI_STK_EventDownload(VOS_VOID *pEventData, VOS_UINT32 ulEventLen, SI_STK_EVENT_TYPE ulEventDownTag, VOS_UINT32 ucSDId);

extern VOS_VOID SI_STKSimpleResponse(SI_STK_COMMAND_DETAILS_STRU *pstCMDInfo, VOS_UINT8 ucResult);

extern VOS_VOID SI_STKCommDataFree(VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU *pstData);

extern VOS_UINT32 SI_STKDecodeTagData(VOS_UINT8 *pucCmdData, VOS_UINT8 ucTag, VOS_VOID *pDec);

extern VOS_UINT32 SI_STK_CheckProfileCfg(VOS_UINT32 ulCmdType, SI_STK_CMD_PROFILE_CFG_STRU *pstCfgTbl, VOS_UINT32 ulTblSize);

extern VOS_VOID SI_STK_SaveEnvelopeCmd(VOS_UINT32 ulCmdLen, VOS_UINT8 *pucCmdData);

extern VOS_UINT32 SI_STK_SendEnvelopeReqMsg(VOS_UINT32 ulSendPara, USIMM_CARDAPP_ENUM_UINT32 enAppType, VOS_UINT8 ucLen, VOS_UINT8 *pucEnvelope);

extern VOS_VOID SI_STK_ClearEnvelopeCmd(VOS_VOID);

extern VOS_UINT32 SI_STKCheckCardState(VOS_VOID);

extern VOS_UINT32 SI_STKErrorProc(VOS_VOID);

extern VOS_VOID SI_STK_InitTResponse(VOS_UINT8 *pTRdata, SI_STK_COMMAND_DETAILS_STRU *pstCMDInfo, SI_STK_RESULT_STRU *pstResult);

extern VOS_VOID SI_STKSendTRWithData(SI_STK_COMMAND_DETAILS_STRU *pstCMDInfo, VOS_UINT32 ulResultLen, VOS_UINT8 *pucResult, VOS_UINT32 ulDataLen, VOS_UINT8 *pucData);

extern VOS_VOID SI_STK_SsStr2ASCII(const VOS_UINT8 *pucSrc, VOS_UINT8 *pucDest, VOS_UINT32 ulLength);

extern VOS_VOID SI_STK_Num2BCD(const VOS_UINT8 *pucSrc, VOS_UINT8 *pucDest, VOS_UINT32 ulLength);

extern VOS_UINT32 SI_STKGetBitFromBuf(VOS_UINT8 *pucDataBuf, VOS_UINT32 ulBitNo,VOS_UINT32 ulBufLen);

extern VOS_UINT32 SI_STK_SendStatusReqMsg(VOS_VOID);

/* STKGobal.c */
extern VOS_VOID STK_UpdateCsSvcStatus(NAS_STK_SERVICE_STATUS_ENUM_UINT8 enCsServiceStatus);

extern VOS_VOID STK_GetSysInfo(SI_STK_SYS_INFO_STRU *pstSysInfo);

extern VOS_UINT32 STK_GetSvcStatus(VOS_VOID);

extern VOS_UINT8 STK_GetCurRat(VOS_VOID);

extern VOS_UINT8 STK_GetSearchMode(VOS_VOID);

extern VOS_VOID STK_UpdateSvcStatus(NAS_STK_SERVICE_STATUS_ENUM_UINT8 enServiceStatus);

extern VOS_VOID STK_UpdateCurRat(TAF_MMA_RAT_TYPE_ENUM_UINT8 ucRat);

extern VOS_VOID STK_UpdateSysInfo(NAS_STK_SYS_INFO_STRU *pstSysInfo);

extern VOS_UINT32 STK_GetCsSvcStatus(VOS_VOID);

extern VOS_VOID STK_ResetGlobalValue(VOS_VOID);

extern VOS_VOID STK_ProfileInit(USIMM_STK_CFG_STRU *pstSTKProfileContent, USIMM_PHYCARD_TYPE_ENUM_UINT32 enPhyType);

extern VOS_VOID STK_GetProfileInfo(VOS_UINT8 **ppucProfile, VOS_UINT8 *pucProfileLen);

extern VOS_VOID STK_SetCallCleanGobal(VOS_VOID);

extern VOS_VOID STK_UpdateSearchMode(VOS_UINT8 ucSearchMode);

extern VOS_VOID STK_InitGobal(VOS_VOID);

extern VOS_VOID STK_ClearPauseCharGobal(VOS_VOID);

extern VOS_VOID STK_CallStateMachineSet(SI_STK_CALL_STATUS_ENUM_UINT32 enCallStatus);

/* STKPro.c */
extern VOS_UINT32 SI_STK_RefreshProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STKNoNeedDecode(VOS_UINT8* pucCmdData, VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU *pstResult);

extern VOS_UINT32 SI_STK_MoreTimeProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_PollIntervalProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_PollingOFFProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SetUpEeventListSpecialProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STKSetUpCall_Decode(VOS_UINT8* pucCmdData, VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU*pstResult);

extern VOS_UINT32 SI_STK_SetCallSpecialProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SendSSProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SendUSSDProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SendSMSProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SendDTMFProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_LaunchBrowser_Decode(VOS_UINT8* pucCmdData, VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU *pstResult);

extern VOS_UINT32 SI_STKDataIndCallback(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_DisplayTextProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SelectItem_Decode(VOS_UINT8* pucCmdData, VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU *pstResult);

extern VOS_BOOL SI_STK_CheckSupportAP(VOS_VOID);

extern VOS_UINT32 SI_STK_SetUpMenu_Decode(VOS_UINT8* pucCmdData, VOS_UINT32 ulCmdType, SI_SAT_COMMDATA_STRU*pstResult);

extern VOS_UINT32 SI_STK_SetUpMenuProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_ProvideLocIProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_TimerManageProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SetUpIdleTextProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STKSetFrame_Decode(VOS_UINT8* pucCmdData, VOS_UINT32 ulCmdType,SI_SAT_COMMDATA_STRU*pstResult);

extern VOS_UINT32 SI_STKCommDecode(VOS_UINT8* pucCmdData, VOS_UINT32 ulCmdType,SI_SAT_COMMDATA_STRU *pstResult);

extern VOS_BOOL SI_STK_CheckCardStatus(VOS_VOID);

extern VOS_VOID SI_STK_CbpTrCnfMsgProc(PS_SI_MSG_STRU *pMsg);

extern VOS_VOID SI_STK_TRCnfMsgProc(PS_SI_MSG_STRU *pMsg);

extern VOS_VOID SI_STK_CbpTRCnfProc(VOS_UINT32 ulErrorCode, VOS_UINT8 ucSW1, VOS_UINT8 ucSW2);

extern VOS_VOID SI_STK_RcvSmsEnvelopeCnfDispatchHandle(
    USIMM_STKENVELOPE_CNF_STRU             *pstUsimmMsg
);

extern VOS_BOOL SI_STK_IsProactiveCmdNeedSnd2Csima(VOS_VOID);

extern VOS_UINT32 SI_STK_SetUpEeventListProc(SI_STK_DATA_INFO_STRU *pCmdData);

/* STKSpecialProc.c */
extern VOS_VOID SI_STK_LocationStatusEventDownload(VOS_VOID);

extern VOS_BOOL STK_IsCallInService(VOS_VOID);

extern VOS_BOOL STK_IsCPBCsimRefreshCmd(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_ProvideLocalInfo_NAA(VOS_UINT8 *pTrData, SI_STK_NAS_INFO_STRU *pstMmaInfo);

extern VOS_VOID SI_STK_TransferUpdateAttachType(VOS_UINT8 *pucType, NAS_STK_UPDATE_TYPE_ENUM_UINT8 ucNasType);

extern VOS_UINT32 MN_MSG_DecodeCbsDcs(
    VOS_UINT8                           ucDcs,
    VOS_UINT8                          *pucContent,
    VOS_UINT32                          ulContentLength,
    MN_MSG_CBDCS_CODE_STRU             *pstDcsInfo
);

extern VOS_VOID SI_STK_SendChangePollTimerLenMsg(
    VOS_UINT32                          ulTimerLen
);

extern VOS_UINT32 SI_STK_SendSSSpecialProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SendDTMFSpecialProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SendSMSSpecialProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_SendUSSDSpecialProc(SI_STK_DATA_INFO_STRU *pCmdData);

extern VOS_UINT32 SI_STK_GetRecTAFPID(VOS_UINT32 *pulTAFPid);

extern VOS_VOID SI_STK_EventResendClean(SI_STK_ENVELOPE_RESENT_FLAG_UINT32 enDataType);

extern VOS_UINT32 SI_STK_SVLTECheckMsgPID(PS_SI_MSG_STRU *pMsg, VOS_UINT32 ulPid);

extern VOS_VOID SI_STK_EventResendSave(SI_STK_ENVELOPE_RESENT_FLAG_UINT32 enDataType, VOS_UINT32 ulEventLen, VOS_UINT8 *pucData);

extern VOS_UINT32 SI_STK_DisconnectStkCall(VOS_VOID);

extern VOS_UINT32  SI_STK_SendStartDtmfMsg(VOS_CHAR cDtmf, VOS_UINT8 ucOpId, VOS_UINT8 ucCallId);

extern VOS_VOID SI_STK_ResendTimerMsgHandle(REL_TIMER_MSG *pstMsg);

extern VOS_VOID SI_STK_IMSIChgBegin(SI_STK_REQ_STRU *STKReqMsg);

extern VOS_VOID SI_STK_SetUpCallAllow(SI_STK_SETUPCALLCONFIRM_ENUM_UINT32 enAction);

extern VOS_VOID SI_STK_NetworkRejectionEventDownload(NAS_STK_NETWORK_REJECTION_EVENT_STRU *pstMsg);

extern VOS_UINT32 SI_STK_SVLTECheckCsDomainAndMsgPID(PS_SI_MSG_STRU *pMsg, VOS_UINT32 ulExpectPid);

extern VOS_VOID SI_STK_UpdateLocationInfo(NAS_STK_LOC_STATUS_EVENT_INFO_STRU   *pstLocStatusEvent);

extern VOS_VOID SI_STK_LociInfoIndMsgProc(NAS_STK_LOCATION_INFO_IND_STRU *pstLociInfo);

extern VOS_VOID SI_STK_NetworkSearchModeChangeEventDownload(VOS_VOID);

extern VOS_VOID SI_STK_TAFMsgProc(PS_SI_MSG_STRU *pMsg);

extern VOS_VOID SI_STK_IMSIChgProc(PS_SI_MSG_STRU *pMsg);

extern VOS_VOID SI_STK_UssdDcsHook(VOS_UINT8 ucInputDcs, VOS_UINT8 *pUssdString, VOS_UINT16 usStringLen, MN_MSG_CBDCS_CODE_STRU *pstCbDcs);

extern VOS_UINT32  TAF_STD_UnPack7Bit(
    const VOS_UINT8                     *pucOrgChar,
    VOS_UINT32                          ulLen,
    VOS_UINT8                           ucFillBit,
    VOS_UINT8                           *pucUnPackedChar
);

extern VOS_UINT32  TAF_STD_Pack7Bit(
    const VOS_UINT8                     *pucOrgChar,
    VOS_UINT32                          ulLen,
    VOS_UINT8                           ucFillBit,
    VOS_UINT8                           *pucPackedChar,
    VOS_UINT32                          *pulLen
);

extern VOS_VOID SI_STK_SetUpCallStopDurationTimer(VOS_UINT32 ulFlg);

extern SI_STK_DTMF_SEND_ENUM_UINT32  SI_STK_SendStopDtmfMsg(VOS_UINT8 ucopID);

extern SI_STK_DTMF_SEND_ENUM_UINT32  SI_STK_SendDtmfToTAF(VOS_UINT8 ucOpId, VOS_UINT8 ucCallId);

extern VOS_VOID SI_STK_SetupCallStatusTable(MN_CALL_EVENT_ENUM_U32 enEventType, MN_CALL_INFO_STRU *pstEvent);

extern VOS_VOID SI_STK_SuspendCnfMsgProc(MN_CALL_INFO_STRU *pstEvent);

extern VOS_VOID SI_STK_ATTResetReSend(VOS_VOID);

extern VOS_UINT32 SI_STK_SetUpCallOnlyNotBusyOnCall(VOS_VOID);

extern VOS_VOID SI_STK_BcdStrToAscii(VOS_UINT8 ucBcdNumLen,VOS_UINT8 *pucBcdNum,VOS_UINT8 *pucAsciiNum,VOS_UINT32 *pulLen,SI_STK_STR_CHANGEMODE_ENUM_UINT32 enChangeMode);

extern VOS_UINT32 SI_STK_EncodeUssdString(SI_STK_USSD_STRING_STRU *pstSrcUssdString, TAF_SS_USSD_STRING_STRU  *pstDestUssdString);

extern VOS_VOID SI_STK_SetUpCallStartDurationTimer(VOS_UINT32 ulFlg, SI_STK_DURATION_STRU *pstDuration);

extern VOS_UINT32 SI_STK_SetUpCall(VOS_BOOL bNeedDurTimer);

/* STKAtPrintf.c */
extern VOS_VOID At_STKCallBackFunc(SI_STK_EVENT_INFO_STRU *pstEvent);
#endif

#if (FEATURE_ON == FEATURE_UE_MODE_CDMA)
#if (OSA_CPU_CCPU == VOS_OSA_CPU)
extern VOS_VOID SI_STK_CdmaLocStatusEventDownload(VOS_VOID);

extern VOS_UINT32 SI_STKGetCdmaEcc(
    SI_STK_ADDRESS_STRU                 *pstAddr
);

extern VOS_VOID SI_STK_RcvXsmsEnvelopeHandle(USIMM_STKENVELOPE_CNF_STRU *pstMsg);

extern VOS_VOID SI_STK_XsmsRcvSendSmsCnf(
    PS_SI_MSG_STRU                      *pMsg
);

extern VOS_VOID SI_STK_XsmsRcvSendSmsRsltInd(
    PS_SI_MSG_STRU                      *pMsg
);

extern VOS_VOID SI_STK_XsmsRcvSmsPPDownLoadReq(
    PS_SI_MSG_STRU                      *pMsg
);

extern VOS_VOID SI_STK_InitXsmsGlobal(VOS_VOID);

extern VOS_VOID SI_STK_SendXsmsReq(
    TAF_XSMS_MESSAGE_STRU               *pstMessage,
    VOS_UINT16                           usIsUserAckMsg
);

extern VOS_VOID SI_STK_RcvXsmsEnvelopeHandle(USIMM_STKENVELOPE_CNF_STRU *pstMsg);

extern VOS_VOID SI_STK_SendXsmsPPDownLoadCnf(
    VOS_UINT32                           ulEnvelopeRslt,
    VOS_UINT32                           ulRspCode,
    VOS_UINT32                           ulDataLen,
    VOS_UINT8                           *pucData
);

extern VOS_VOID SI_STK_ServiceInfoInd(
    MMA_STK_SERVICE_INFO_IND_STRU       *pstMsg
);

extern VOS_BOOL SI_STK_CheckCdmaCallID(VOS_VOID);

extern SI_STK_DTMF_SEND_ENUM_UINT32 SI_STK_SendXDtmfToTAF(
    VOS_UINT8                           ucOpID,
    VOS_UINT8                           ucCallID
);

extern VOS_VOID SI_STK_SendBurstDTMFCnfProc(
    TAF_CALL_EVT_SEND_BURST_DTMF_CNF_STRU   *pstEvent
);

extern VOS_VOID SI_STK_SendBurstDTMFResultProc(
    TAF_CALL_EVT_SEND_BURST_DTMF_RSLT_STRU  *pstEvent
);

extern VOS_UINT32 SI_STK_SendCdmaSMSHandle(
    SI_STK_SEND_SHORT_MESSAGE_STRU      *pstSendSMS,
    SI_STK_COMMAND_DETAILS_STRU         *pstCmdDetail
);

extern VOS_UINT32 SI_STK_CdmaProvideLocalInfo(
    VOS_UINT8                           *pTrData
);

extern VOS_UINT32 SI_STK_ProvideESN(
    VOS_UINT8                           *pTrData
);

extern VOS_UINT32 SI_STK_ProvideMEID(
    VOS_UINT8                           *pTrData
);

extern VOS_UINT32  SI_STK_CheckCdmaSmsPara(
    TAF_XSMS_TRANSPORT_MESSAGE_STRU     *pstTLMsg,
    SI_STK_COMMAND_DETAILS_STRU         *pstCmdDetail,
    SI_STK_CDMA_SMS_CTRL_PARA_STRU      *pstCdmaSmsPara
);
#endif
#endif

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




