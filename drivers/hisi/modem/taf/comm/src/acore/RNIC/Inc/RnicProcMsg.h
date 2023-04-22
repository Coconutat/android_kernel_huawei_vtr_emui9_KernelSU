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

#ifndef __RNIC_PROCMSG_H__
#define __RNIC_PROCMSG_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "AtRnicInterface.h"
#include "PsCommonDef.h"
#include "RnicLinuxInterface.h"
#include "RnicTimerMgmt.h"
#include "RnicConfigInterface.h"
#include "ImsaRnicInterface.h"

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


enum RNIC_DAIL_EVENT_TYPE_ENUM
{
    RNIC_DAIL_EVENT_UP                  = 0x0600,                               /*需要触发拨号*/
    RNIC_DAIL_EVENT_DOWN                        ,                               /*需要断开拨号 */

    RNIC_DAIL_EVENT_TYPE_BUTT
};
typedef VOS_UINT32 RNIC_DAIL_EVENT_TYPE_ENUM_UINT32;


enum RNIC_DIAL_MODE_ENUM
{
    RNIC_DIAL_MODE_MANUAL,                                                      /*Manual dial mode*/
    RNIC_DIAL_MODE_DEMAND_CONNECT,                                              /*Demand dial mode*/
    RNIC_DIAL_MODE_DEMAND_DISCONNECT,                                           /*Demand dial mode*/
    RNIC_DIAL_MODE_BUTT
};
typedef VOS_UINT32 RNIC_DIAL_MODE_ENUM_UINT32;


enum RNIC_DIAL_EVENT_REPORT_FLAG_ENUM
{
    RNIC_FORBID_EVENT_REPORT            = 0x0000,                               /*不给应用上报*/
    RNIC_ALLOW_EVENT_REPORT             = 0X0001,                               /*允许给应用上报*/

    RNIC_DIAL_EVENT_REPORT_FLAG_BUTT
};
typedef VOS_UINT32 RNIC_DIAL_EVENT_REPORT_FLAG_ENUM_UINT32;


enum RNIC_MSG_ID_ENUM
{
    ID_RNIC_RMNET_CONFIG_REQ            = 0x0000,                               /* _H2ASN_MsgChoice RNIC_RMNET_CONFIG_REQ_STRU */

    ID_RNIC_CCPU_RESET_START_IND        = 0x1001,                               /* _H2ASN_MsgChoice RNIC_CCPU_RESET_IND_STRU */
    ID_RNIC_CCPU_RESET_END_IND          = 0x1002,                               /* _H2ASN_MsgChoice RNIC_CCPU_RESET_IND_STRU */

    ID_RNIC_IMS_DATA_PROC_IND           = 0x1003,                               /* _H2ASN_MsgChoice RNIC_CCPU_RESET_IND_STRU */

    /* 可谓可测消息ID */
    ID_RNIC_MNTN_EVT_DIAL_CONNECT       = 0x6001,                               /* _H2ASN_MsgChoice RNIC_MNTN_DIAL_CONN_EVT_STRU */
    ID_RNIC_MNTN_EVT_DIAL_DISCONNECT    = 0x6002,                               /* _H2ASN_MsgChoice RNIC_MNTN_DIAL_DISCONN_EVT_STRU */

    /* 拨号模式信息 按需拨号*/
    ID_RNIC_MNTN_DIAL_MODE_INFO         = 0x6003,                               /* _H2ASN_MsgChoice RNIC_NOTIFY_MSG_STRU */

    /* 拨号时间信息 按需拨号*/
    ID_RNIC_MNTN_IDLE_TIMEOUT_INFO      = 0x6004,                               /* _H2ASN_MsgChoice RNIC_NOTIFY_MSG_STRU */

    /* 是否上报事件 按需拨号*/
    ID_RNIC_MNTN_EVENT_REPORT_INFO      = 0x6005,                               /* _H2ASN_MsgChoice RNIC_NOTIFY_MSG_STRU */

    /* 外部模块配置RNIC网卡的信息 */
    ID_RNIC_MNTN_RMNET_CONFIG_INFO      = 0x6006,                               /* _H2ASN_MsgChoice RNIC_RMNET_CONFIG_REQ_STRU */

    ID_RNIC_RNIC_MSG_ID_BUTT
};

typedef VOS_UINT32 RNIC_MSG_ID_ENUM_UINT32;


/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头定义
*****************************************************************************/


/*****************************************************************************
  6 消息定义
*****************************************************************************/

/* 定时器超时处理函数指针*/
typedef VOS_UINT32 (*RNIC_RCV_TI_EXPRIED_PROC_FUNC)(
    MsgBlock                           *pstMsg,
    VOS_UINT8                           ucRmNetId
);


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

typedef struct
{
    RNIC_DIAL_MODE_ENUM_UINT32                              enDialMode;         /* Dial模式 */
    VOS_UINT32                                              ulIdleTime;         /* 定时器长度，单位为秒 */
    RNIC_DIAL_EVENT_REPORT_FLAG_ENUM_UINT32                 enEventReportFlag;  /*是否给应用上报标识*/
    VOS_UINT32                                              ulReserved;
}RNIC_DIAL_MODE_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */        /*_H2ASN_Skip*/
    RNIC_MSG_ID_ENUM_UINT32             enMsgId;                                /*消息类型*/        /*_H2ASN_Skip*/
    RNIC_DIAL_MODE_STRU                 stDialInfo;                             /*按需拨号配置内容*/
}RNIC_NOTIFY_MSG_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */    /* _H2ASN_Skip */
    RNIC_MSG_ID_ENUM_UINT32             enMsgId;                                /* 消息ID */    /* _H2ASN_Skip */
} RNIC_MNTN_DIAL_CONN_EVT_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */    /* _H2ASN_Skip */
    RNIC_MSG_ID_ENUM_UINT32             enMsgId;                                /* 消息ID */    /* _H2ASN_Skip */
    VOS_UINT32                          ulPktNum;
    VOS_UINT32                          ulUsrExistFlg;
} RNIC_MNTN_DIAL_DISCONN_EVT_STRU;


typedef struct
{
    VOS_UINT32                          ulMsgId;
    RNIC_RCV_TI_EXPRIED_PROC_FUNC       pTiExpriedProcFunc;
}RNIC_RCV_TI_EXPRIED_PROC_STRU;

/* Modified by l60609 for L-C互操作项目, 2014-01-06, Begin */

typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */    /* _H2ASN_Skip */
    RNIC_MSG_ID_ENUM_UINT32             enMsgId;                                /* 消息ID */    /* _H2ASN_Skip */

    RNIC_MODEM_TYPE_ENUM_UINT8          enModemType;
    VOS_UINT8                           ucRmNetId;
    VOS_UINT16                          usModemId;
    VOS_UINT8                           ucPdnId;
    VOS_UINT8                           ucRabId;
    RNIC_RMNET_STATUS_ENUM_UINT8        enRmnetStatus;
    RNIC_IP_TYPE_ENUM_UINT8             enIpType;
}RNIC_RMNET_CONFIG_REQ_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */    /* _H2ASN_Skip */
    RNIC_MSG_ID_ENUM_UINT32             enMsgId;                                /* 消息ID */    /* _H2ASN_Skip */
}RNIC_CCPU_RESET_IND_STRU;
/* Modified by l60609 for L-C互操作项目, 2014-01-06, End */


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */    /* _H2ASN_Skip */
    RNIC_MSG_ID_ENUM_UINT32             enMsgId;                                /* 消息ID */    /* _H2ASN_Skip */
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId;
    VOS_UINT8                           aucReserved[3];
}RNIC_IMS_DATA_PROC_IND_STRU;

/*****************************************************************************
 结构名    : RNIC_IPFIXHDR_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IP固定头
*****************************************************************************/
typedef struct
{
    VOS_UINT8    ucIpHdrLen:4;                 /* header length */
    VOS_UINT8    ucIpVer:4;                    /* version */

    VOS_UINT8    ucServiceType;                /* type of service */
    VOS_UINT16   usTotalLen;                   /* total length */
    VOS_UINT16   usIdentification;             /* identification */
    VOS_UINT16   usOffset;                     /* fragment offset field */
    VOS_UINT8    ucTTL;                        /* time to live*/
    VOS_UINT8    ucProtocol;                   /* protocol */
    VOS_UINT16   usCheckSum;                   /* checksum */
    VOS_UINT32   ulSrcAddr;                    /* source address */
    VOS_UINT32   ulDestAddr;                   /* dest address */
}RNIC_IPFIXHDR_STRU;

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
VOS_UINT32 RNIC_SendDialEvent(
    VOS_UINT32                          ulDeviceId,
    VOS_UINT32                          ulEventId
);
VOS_UINT32 RNIC_RcvAtIpv4PdpActInd(
    MsgBlock                           *pstMsg
);
VOS_UINT32 RNIC_RcvAtIpv6PdpActInd(
    MsgBlock                           *pstMsg
);
VOS_UINT32 RNIC_RcvAtPdpDeactInd(
    MsgBlock                           *pstMsg
);
VOS_UINT32 RNIC_RcvAtDsflowInd(
    MsgBlock                           *pstMsg
);
VOS_UINT32  RNIC_RcvTiDsflowStatsExpired(
    MsgBlock                           *pstMsg,
    VOS_UINT8                           ucRmNetId
);
VOS_UINT32 RNIC_RcvTiDemandDialDisconnectExpired(
    MsgBlock                           *pstMsg,
    VOS_UINT8                           ucRmNetId
);
VOS_UINT32 RNIC_RcvTiDemandDialProtectExpired(
    MsgBlock                           *pstMsg,
    VOS_UINT8                           ucRmNetId
);
VOS_UINT32 RNIC_RcvTimerMsg(MsgBlock *pstMsg);
VOS_UINT32 RNIC_RcvAtMsg(MsgBlock *pstMsg);
VOS_UINT32 RNIC_ProcMsg (MsgBlock *pstMsg);
VOS_VOID RNIC_MNTN_TraceDialConnEvt(VOS_VOID);
VOS_VOID RNIC_MNTN_TraceDialDisconnEvt(
    VOS_UINT32                          ulPktNum,
    VOS_UINT32                          ulUsrExistFlg
);
VOS_UINT32 RNIC_RcvAtIpv4v6PdpActInd(
    MsgBlock                           *pstMsg
);
RNIC_TIMER_ID_ENUM_UINT16 RNIC_GetDsflowTimerIdByNetId(VOS_UINT8 ucRmNetId);
VOS_UINT32 RNIC_SaveNetIdByRabId(
    MODEM_ID_ENUM_UINT16                enModemId,
    VOS_UINT8                           ucRabId,
    VOS_UINT8                           ucRmNetId
);
VOS_UINT32 RNIC_BuildRabIdByModemId(
    MODEM_ID_ENUM_UINT16                enModemId,
    VOS_UINT8                           ucRabId,
    VOS_UINT8                          *pucRabId
);
VOS_UINT8 RNIC_GetNetIdByTimerId(VOS_UINT32 ulMsgId);

/* Added by L47619 for V3R3 Share-PDP Project, 2013-6-3, begin */
VOS_UINT32 RNIC_RcvAtPdnInfoCfgInd(
    MsgBlock                           *pstMsg
);
VOS_UINT32 RNIC_RcvAtPdnInfoRelInd(
    MsgBlock                           *pstMsg
);
/* Added by L47619 for V3R3 Share-PDP Project, 2013-6-3, end */

VOS_VOID RNIC_ProcInsideModemIpv4ActInd(
    RNIC_RMNET_CONFIG_REQ_STRU           *pstPdpStatusInd
);

VOS_VOID RNIC_ProcInsideModemIpv6ActInd(
    RNIC_RMNET_CONFIG_REQ_STRU           *pstPdpStatusInd
);

VOS_VOID RNIC_ProcRnicPdpActInd(
    RNIC_RMNET_CONFIG_REQ_STRU           *pstPdpStatusInd
);

#if (FEATURE_ON == FEATURE_CL_INTERWORK)
VOS_UINT32 RNIC_SaveNetIdByPdnId(
    VOS_UINT8                           ucPdnId,
    VOS_UINT8                           ucRmNetId
);

VOS_VOID RNIC_ProcOutsideModemIpv4ActInd(
    RNIC_RMNET_CONFIG_REQ_STRU           *pstPdpStatusInd
);

VOS_VOID RNIC_ProcOutsideModemIpv6ActInd(
    RNIC_RMNET_CONFIG_REQ_STRU           *pstPdpStatusInd
);

VOS_VOID RNIC_ProcRnicPdpHandoverInd(
    RNIC_RMNET_CONFIG_REQ_STRU           *pstPdpStatusInd
);
#endif

VOS_VOID RNIC_ProcRnicPdpDeactInd(
    RNIC_RMNET_CONFIG_REQ_STRU           *pstPdpStatusInd
);

VOS_UINT32 RNIC_RcvRnicRmnetConfigReq(
    MsgBlock                           *pstMsg
);

VOS_UINT32 RNIC_RcvRnicMsg(MsgBlock *pstMsg);
#if (FEATURE_ON == FEATURE_IMS)
VOS_UINT32 RNIC_RcvImsaMsg(MsgBlock *pstMsg);
VOS_UINT32 RNIC_RcvImsaPdnActInd(
    MsgBlock                           *pstMsg
);
VOS_UINT32 RNIC_RcvImsaPdnDeactInd(
    MsgBlock                           *pstMsg
);

VOS_UINT32 RNIC_RcvCdsMsg(MsgBlock *pstMsg);
VOS_VOID RNIC_RcvCdsImsDataInd(
    MsgBlock                           *pstMsg
);
#ifdef CONFIG_VOWIFI_NEW_FRW
VOS_UINT32 RNIC_ProcImsaPdnActInd_Wifi(
    IMSA_RNIC_PDN_INFO_CONFIG_STRU     *pstPdnInfo
);
VOS_UINT32 RNIC_ProcImsaPdnActInd_Lte(
    IMSA_RNIC_PDN_INFO_CONFIG_STRU     *pstPdnInfo
);
VOS_UINT32 RNIC_ProcImsaPdnDeactInd_Wifi(
    MODEM_ID_ENUM_UINT16                enModemId,
    IMSA_RNIC_PDN_EMC_IND_ENUM_UINT8    enEmcInd
);
VOS_UINT32 RNIC_ProcImsaPdnDeactInd_Lte(
    MODEM_ID_ENUM_UINT16                enModemId,
    IMSA_RNIC_PDN_EMC_IND_ENUM_UINT8    enEmcInd
);
VOS_UINT8 RNIC_GetImsRmnetId(
    IMSA_RNIC_IMS_RAT_TYPE_ENUM_UINT8   enRatType,
    MODEM_ID_ENUM_UINT16                enModemId,
    IMSA_RNIC_PDN_EMC_IND_ENUM_UINT8    enEmcInd
);
#else
VOS_UINT32 RNIC_ProcImsaPdnActInd_Wifi(
    IMSA_RNIC_PDN_ACT_IND_STRU         *pstRcvInd
);
VOS_UINT32 RNIC_ProcImsaPdnActInd_Lte(
    IMSA_RNIC_PDN_ACT_IND_STRU         *pstRcvInd
);
VOS_UINT32 RNIC_ProcImsaPdnDeactInd_Wifi(
    IMSA_RNIC_PDN_DEACT_IND_STRU       *pstRcvInd
);
VOS_UINT32 RNIC_ProcImsaPdnDeactInd_Lte(
    IMSA_RNIC_PDN_DEACT_IND_STRU       *pstRcvInd
);
#endif
#endif

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
VOS_VOID RNIC_RegNapiSchedualCallback(
    VOS_UINT8                           ucRabId,
    VOS_UINT8                           ucRmNetId
);
#endif

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

#endif /* end of RnicProcMsg.h */
