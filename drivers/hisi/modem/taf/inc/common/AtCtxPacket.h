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

#ifndef __ATCTXPACKET_H__
#define __ATCTXPACKET_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "v_id.h"
#include "AtTypeDef.h"
#include "MnClient.h"
#include "TafApsApi.h"
#include "TafNvInterface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define AT_IPV6_CAPABILITY_IPV4_ONLY            (1)
#define AT_IPV6_CAPABILITY_IPV6_ONLY            (2)
#define AT_IPV6_CAPABILITY_IPV4V6_OVER_ONE_PDP  (4)
#define AT_IPV6_CAPABILITY_IPV4V6_OVER_TWO_PDP  (8)

/* PS域呼叫最大个数 */
#define AT_PS_MAX_CALL_NUM              (3)


#define AT_PS_RABID_OFFSET              (5)                 /* RABID偏移 */
#define AT_PS_RABID_MAX_NUM             (11)                /* RABID数量 */
#define AT_PS_MIN_RABID                 (5)                 /* RABID最小值 */
#define AT_PS_MAX_RABID                 (15)                /* RABID最大值 */
#define AT_PS_INVALID_RABID             (0xFF)              /* RABID无效值 */
#define AT_PS_RABID_MODEM_1_MASK        (0x40)

#define IPV6_ADDRESS_TEST_MODE_ENABLE                    (0x55aa55aa)           /* 0x55aa55aa值IPV6地址为测试模式 */

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
enum AT_PDP_STATE_ENUM
{
    AT_PDP_STATE_IDLE                = 0,
    AT_PDP_STATE_ACTED               = 1,
    AT_PDP_STATE_ACTING              = 2,
    AT_PDP_STATE_DEACTING            = 3,
    AT_PDP_STATE_BUTT
};
typedef VOS_UINT8 AT_PDP_STATE_ENUM_U8;

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
 结构名称   : AT_DIAL_PARAM_STRU
 协议表格   :
 ASN.1 描述 :
 结构说明   : 保存用户拨号参数
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           ucCid;
    TAF_PDP_TYPE_ENUM_UINT8             enPdpType;
    VOS_UINT8                           ucPdpTypeValidFlag;

    VOS_UINT8                           ucAPNLen;                               /*APN长度*/
    VOS_UINT8                           aucAPN[TAF_MAX_APN_LEN + 1];             /*指向APN指针*/

    VOS_UINT16                          usUsernameLen;                          /*username长度*/
    VOS_UINT16                          usRsv3;
    VOS_UINT8                           aucUsername[TAF_MAX_GW_AUTH_USERNAME_LEN];  /*指向username指针*/
    VOS_UINT8                           ucRsv4;

    VOS_UINT16                          usPasswordLen;                          /*password长度*/
    VOS_UINT16                          usRsv5;
    VOS_UINT8                           aucPassword[TAF_MAX_GW_AUTH_PASSWORD_LEN];  /*指向password指针*/
    VOS_UINT8                           ucRsv6;

    VOS_UINT16                          usAuthType;
    VOS_UINT8                           ucBitRatType;
    VOS_UINT8                           aucRsv7[1];                             /*填充位*/

    VOS_UINT32                          ulIPv4ValidFlag;
    VOS_UINT8                           aucIPv4Addr[TAF_MAX_IPV4_ADDR_STR_LEN];

    VOS_UINT32                          ulPrimIPv4DNSValidFlag;
    VOS_UINT8                           aucPrimIPv4DNSAddr[TAF_MAX_IPV4_ADDR_STR_LEN];

    VOS_UINT32                          ulSndIPv4DNSValidFlag;
    VOS_UINT8                           aucSndIPv4DNSAddr[TAF_MAX_IPV4_ADDR_STR_LEN];

    VOS_UINT32                          ulPrimIPv4WINNSValidFlag;
    VOS_UINT8                           aucPrimIPv4WINNSAddr[TAF_MAX_IPV4_ADDR_STR_LEN];

    VOS_UINT32                          ulSndIPv4WINNSValidFlag;
    VOS_UINT8                           aucSndIPv4WINNSAddr[TAF_MAX_IPV4_ADDR_STR_LEN];

    VOS_UINT32                          ulPrimIPv6DNSValidFlag;
    VOS_UINT8                           aucPrimIPv6DNSAddr[TAF_IPV6_ADDR_LEN];

    VOS_UINT32                          ulSndIPv6DNSValidFlag;
    VOS_UINT8                           aucSndIPv6DNSAddr[TAF_IPV6_ADDR_LEN];

    VOS_UINT32                          ulPrimIPv6WINNSValidFlag;
    VOS_UINT8                           aucPrimIPv6WINNSAddr[TAF_IPV6_ADDR_LEN];

    VOS_UINT32                          ulSndIPv6WINNSValidFlag;
    VOS_UINT8                           aucSndIPv6WINNSAddr[TAF_IPV6_ADDR_LEN];

}AT_DIAL_PARAM_STRU;



typedef struct
{
    VOS_UINT8                           ucIpv4PcscfAddrNum;                     /* IPV4的P-CSCF地址个数，有效范围[0,8] */
    VOS_UINT8                           aucRsv[3];                              /* 保留 */

    VOS_UINT32                          aulIpv4PcscfAddrList[TAF_PCSCF_ADDR_MAX_NUM];
} AT_IPV4_PCSCF_LIST_STRU;


typedef struct
{
    VOS_UINT8                           aucPcscfAddr[TAF_IPV6_ADDR_LEN];
} AT_PDP_IPV6_PCSCF_STRU;


typedef struct
{
    VOS_UINT8                           ucIpv6PcscfAddrNum;                     /* IPV6的P-CSCF地址个数，有效范围[0,8] */
    VOS_UINT8                           aucRsv[3];                              /* 保留 */

    AT_PDP_IPV6_PCSCF_STRU              astIpv6PcscfAddrList[TAF_PCSCF_ADDR_MAX_NUM];
} AT_IPV6_PCSCF_LIST_STRU;

/*****************************************************************************
 结构名    : AT_IPV6_DHCP_PARAM_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IPV6类型的PDP激活参数指结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitOpIpv6PriDns   : 1;
    VOS_UINT32                          bitOpIpv6SecDns   : 1;
    VOS_UINT32                          bitOpIpv6Spare    : 30;

    VOS_UINT8                           ucRabId;                                /* RAB标识，取值范围:[5,15] */
    VOS_UINT8                           aucRsv[3];
    VOS_UINT8                           aucIpv6Addr[TAF_IPV6_ADDR_LEN];      /* 从 PDP上下文带来的IPV6地址长度，不包括":" */

    VOS_UINT8                           aucIpv6PrimDNS[TAF_IPV6_ADDR_LEN];   /* 从 PDP上下文带来的IPV6主DNS长度，不包括":" */
    VOS_UINT8                           aucIpv6SecDNS[TAF_IPV6_ADDR_LEN];    /* 从 PDP上下文带来的IPV6副DNS长度，不包括":" */

    AT_IPV6_PCSCF_LIST_STRU             stIpv6PcscfList;
}AT_IPV6_DHCP_PARAM_STRU;

/*****************************************************************************
 结构名    : AT_IPV4_DHCP_PARAM_STRU
 协议表格  :
 ASN.1描述 :
 结构说明  : IPV4类型的PDP激活参数指结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitOpIpv4PriDns   : 1;
    VOS_UINT32                          bitOpIpv4SecDns   : 1;
    VOS_UINT32                          bitOpIpv4PriWINNS : 1;
    VOS_UINT32                          bitOpIpv4SecWINNS : 1;
    VOS_UINT32                          bitOpSpare        : 28;

    VOS_UINT8                           ucRabId;                                /* RAB标识，取值范围:[5,15] */
    VOS_UINT8                           aucRsv1[3];

    VOS_UINT32                          ulIpv4Addr;                             /* IPV4的IP地址，主机序 */
    VOS_UINT32                          ulIpv4NetMask;                          /* IPV4的掩码，主机序 */
    VOS_UINT32                          ulIpv4GateWay;                          /* IPV4的网关地址，主机序 */
    VOS_UINT32                          ulIpv4PrimDNS;                          /* IPV4的主DNS，主机序 */
    VOS_UINT32                          ulIpv4SecDNS;                           /* IPV4的主DNS，主机序 */
    VOS_UINT32                          ulIpv4PrimWINNS;                        /* IPV4的主WINNS，主机序 */
    VOS_UINT32                          ulIpv4SecWINNS;                         /* IPV4的副WINNS，主机序 */
    AT_IPV4_PCSCF_LIST_STRU             stIpv4PcscfList;                        /* 8组Pcscf */
}AT_IPV4_DHCP_PARAM_STRU;



typedef struct
{
    AT_CLIENT_TAB_INDEX_UINT8           enPortIndex;
    AT_CLIENT_TAB_INDEX_UINT8           enUserIndex;
    AT_USER_TYPE                        ucUsrType;
    VOS_UINT8                           ucUsrCid;
} AT_PS_USER_INFO_STRU;

/*****************************************************************************
 结构名称   : AT_IPV6_RA_INFO_STRU
 协议表格   :
 ASN.1 描述 :
 结构说明   : RA消息中相关参数结构体
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitOpLanAddr            : 1;
    VOS_UINT32                          bitOpPrefixAddr         : 1;
    VOS_UINT32                          bitOpMtuSize            : 1;
    VOS_UINT32                          bitOpPreferredLifetime  : 1;
    VOS_UINT32                          bitOpValidLifetime      : 1;
    VOS_UINT32                          bitOpSpare              : 27;


    VOS_UINT8                           aucLanAddr[TAF_IPV6_ADDR_LEN];       /* IPv6 路由器LAN端口地址 */
    VOS_UINT8                           aucPrefixAddr[TAF_IPV6_ADDR_LEN];    /* IPv6前缀 */
    VOS_UINT32                          ulPrefixBitLen;                         /* IPv6前缀长度 */
    VOS_UINT32                          ulMtuSize;                              /* RA消息中广播的IPv6的MTU的取值 */
    VOS_UINT32                          ulPreferredLifetime;                    /* IPv6前缀的Preferred lifetime */
    VOS_UINT32                          ulValidLifetime;                        /* IPv6前缀的Valid lifetime */
} AT_IPV6_RA_INFO_STRU;


/*消息处理函数指针*/
typedef VOS_VOID (*AT_PS_RPT_CONN_RSLT_FUNC)(\
    VOS_UINT8                           ucCid, \
    VOS_UINT8                           ucPortIndex, \
    TAF_PDP_TYPE_ENUM_UINT8             enPdpType);


/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_USER_TYPE                        ucUsrType;
    AT_PS_RPT_CONN_RSLT_FUNC            pRptConnRsltFunc;
}AT_PS_REPORT_CONN_RESULT_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*消息处理函数指针*/
typedef VOS_VOID (*AT_PS_RPT_END_RSLT_FUNC)(\
    VOS_UINT8                           ucCid, \
    VOS_UINT8                           ucPortIndex, \
    TAF_PDP_TYPE_ENUM_UINT8             enPdpType, \
    TAF_PS_CAUSE_ENUM_UINT32            enCause);


/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_USER_TYPE                        ucUsrType;
    AT_PS_RPT_END_RSLT_FUNC             pRptEndRsltFunc;
}AT_PS_REPORT_END_RESULT_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*消息处理函数指针*/
typedef VOS_VOID (*AT_PS_REG_FC_POINT_FUNC)(\
    VOS_UINT8                           ucCid, \
    TAF_PS_CALL_PDP_ACTIVATE_CNF_STRU  *pstEvent);


/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_USER_TYPE                        ucUsrType;
    AT_PS_REG_FC_POINT_FUNC             pRegFcPoint;
}AT_PS_REG_FC_POINT_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*消息处理函数指针*/
typedef VOS_VOID (*AT_PS_DEREG_FC_POINT_FUNC)(\
    VOS_UINT8                           ucCid, \
    TAF_PS_CALL_PDP_DEACTIVATE_CNF_STRU *pstEvent);


/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_USER_TYPE                        ucUsrType;
    AT_PS_DEREG_FC_POINT_FUNC           pDeRegFcPoint;
}AT_PS_DEREG_FC_POINT_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*消息处理函数指针*/
typedef VOS_VOID (*AT_PS_SND_PDP_ACT_IND_FUNC)(\
    VOS_UINT8                           ucCid, \
    TAF_PS_CALL_PDP_ACTIVATE_CNF_STRU  *pstEvent, \
    TAF_PDP_TYPE_ENUM_UINT8             enPdpType);


/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_USER_TYPE                        ucUsrType;
    AT_PS_SND_PDP_ACT_IND_FUNC          pSndPdpActInd;
}AT_PS_SND_PDP_ACT_IND_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/

/*消息处理函数指针*/
typedef VOS_VOID (*AT_PS_SND_PDP_DEACT_IND_FUNC)(\
    VOS_UINT8                           ucCid, \
    TAF_PS_CALL_PDP_DEACTIVATE_CNF_STRU *pstEvent, \
    TAF_PDP_TYPE_ENUM_UINT8             enPdpType);


/*lint -e958 -e959 修改人:l60609;原因:64bit*/
typedef struct
{
    AT_USER_TYPE                        ucUsrType;
    AT_PS_SND_PDP_DEACT_IND_FUNC        pSndPdpDeActInd;
}AT_PS_SND_PDP_DEACT_IND_STRU;
/*lint +e958 +e959 修改人:l60609;原因:64bit*/


typedef struct
{
    VOS_UINT32                          ulUsed;                                 /* 指定CID是否已经通过CHDATA配置了数传通道，VOS_TRUE:已经配置；VOS_FALSE:未配置 */
    VOS_UINT32                          ulRmNetId;                              /* 数据通道ID
                                                                                   HSIC通道 :UDI_ACM_HSIC_ACM1_ID，UDI_ACM_HSIC_ACM3_ID和UDI_ACM_HSIC_ACM5_ID，如果未配置则为无效值UDI_INVAL_DEV_ID
                                                                                   VCOM通道 :RNIC_RMNET_ID_0 ~ RNIC_RMNET_ID_4
                                                                                   */
    VOS_UINT32                          ulRmNetActFlg;                          /* 指定CID是否已经PDP激活，VOS_TRUE:已经激活；VOS_FALSE:未激活 */
}AT_PS_DATA_CHANL_CFG_STRU;

#if (FEATURE_ON == FEATURE_IPV6)


typedef struct
{
    VOS_UINT32                          ulCauseNum;
    VOS_UINT8                           aucReserved[4];
    TAF_PS_CAUSE_ENUM_UINT32            aenPsCause[TAF_NV_IPV6_FALLBACK_EXT_CAUSE_MAX_NUM];
} AT_PS_IPV6_BACKPROC_EXT_CAUSE_STRU;
#endif


typedef struct
{
    VOS_UINT32                          ulUsedFlg;          /* 呼叫实体分配标志 */
    TAF_PDP_TYPE_ENUM_UINT8             enCurrPdpType;      /* 当前呼叫类型 */
    VOS_UINT8                           aucRsv1[3];         /* 保留位 */
    AT_PS_USER_INFO_STRU                stUserInfo;         /* 呼叫实体用户信息 */
    AT_DIAL_PARAM_STRU                  stUsrDialParam;     /* 呼叫实体拨号参数 */

    VOS_UINT8                           ucIpv4Cid;          /* IPv4 CID */
    AT_PDP_STATE_ENUM_U8                enIpv4State;        /* IPv4 状态 */
    VOS_UINT8                           aucRsv2[2];         /* 保留位 */
    AT_IPV4_DHCP_PARAM_STRU             stIpv4DhcpInfo;     /* IPv4 DHCP信息 */

#if (FEATURE_ON == FEATURE_IPV6)
    VOS_UINT8                           ucIpv6Cid;          /* IPv6 CID */
    AT_PDP_STATE_ENUM_U8                enIpv6State;        /* IPv6 状态 */
    VOS_UINT8                           aucRsv3[2];         /* 保留位 */
    AT_IPV6_RA_INFO_STRU                stIpv6RaInfo;       /* IPv6 路由公告信息 */
    AT_IPV6_DHCP_PARAM_STRU             stIpv6DhcpInfo;     /* IPv6 DHCP信息 */
#endif

} AT_PS_CALL_ENTITY_STRU;

/*****************************************************************************
 结构名称  : AT_IMS_EMC_IPV4_PDN_INFO_STRU
 结构说明  : IMS EMC IPV4 PDN信息
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucIpAddr[TAF_IPV4_ADDR_LEN];
    VOS_UINT8                           aucDnsPrimAddr[TAF_IPV4_ADDR_LEN];
    VOS_UINT8                           aucDnsSecAddr[TAF_IPV4_ADDR_LEN];
    VOS_UINT16                          usMtu;
    VOS_UINT16                          usReserved;

} AT_IMS_EMC_IPV4_PDN_INFO_STRU;

/*****************************************************************************
 结构名称  : AT_IMS_EMC_IPV6_PDN_INFO_STRU
 结构说明  : IMS EMC IPV6 PDN信息
*****************************************************************************/
typedef struct
{
    VOS_UINT8                           aucIpAddr[TAF_IPV6_ADDR_LEN];
    VOS_UINT8                           aucDnsPrimAddr[TAF_IPV6_ADDR_LEN];
    VOS_UINT8                           aucDnsSecAddr[TAF_IPV6_ADDR_LEN];
    VOS_UINT16                          usMtu;
    VOS_UINT16                          usReserved;

} AT_IMS_EMC_IPV6_PDN_INFO_STRU;

/*****************************************************************************
 结构名称  : AT_IMS_EMC_RDP_STRU
 结构说明  : IMS EMC 动态参数结构
*****************************************************************************/
typedef struct
{
    VOS_UINT32                          bitOpIPv4PdnInfo    : 1;
    VOS_UINT32                          bitOpIPv6PdnInfo    : 1;
    VOS_UINT32                          bitOpSpare          : 30;

    AT_IMS_EMC_IPV4_PDN_INFO_STRU       stIPv4PdnInfo;
    AT_IMS_EMC_IPV6_PDN_INFO_STRU       stIPv6PdnInfo;

} AT_IMS_EMC_RDP_STRU;


typedef struct
{
#if (FEATURE_ON == FEATURE_IPV6)
    VOS_UINT8                           ucIpv6Capability;
    VOS_UINT8                           aucReserved1[3];
    VOS_UINT32                          ulIpv6AddrTestModeCfg;

    /* 保存用户定制的用于回退处理的PS域原因值 */
    AT_PS_IPV6_BACKPROC_EXT_CAUSE_STRU  stIpv6BackProcExtCauseTbl;
#endif

    VOS_UINT8                           ucSharePdpFlag;
    VOS_UINT8                           aucReserved2[6];
    VOS_UINT8                           ucRedialForNoCauseFlag;

    VOS_INT32                           lSpePort;
    VOS_UINT32                          ulIpfPortFlg;

}AT_COMM_PS_CTX_STRU;


typedef struct
{
    /* 保存和CID关联的PS域呼叫实体的索引 */
    VOS_UINT8                           aucCidToIndexTbl[TAF_MAX_CID + 1];

    /* PS域呼叫实体 */
    AT_PS_CALL_ENTITY_STRU              astCallEntity[AT_PS_MAX_CALL_NUM];

    /* CID与数传通道的对应关系 */
    AT_PS_DATA_CHANL_CFG_STRU           astChannelCfg[TAF_MAX_CID + 1];

    /* PS域呼叫错误码 */
    TAF_PS_CAUSE_ENUM_UINT32            enPsErrCause;
    /* IP地址与RABID的映射表, IP地址为主机序 */
    VOS_UINT32                          aulIpAddrRabIdMap[AT_PS_RABID_MAX_NUM];

#if (FEATURE_ON == FEATURE_IMS)
    /* IMS EMC PDN 动态信息 */
    AT_IMS_EMC_RDP_STRU                 stImsEmcRdp;
#endif

} AT_MODEM_PS_CTX_STRU;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
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

#endif /* end of AtCtxPacket.h */
