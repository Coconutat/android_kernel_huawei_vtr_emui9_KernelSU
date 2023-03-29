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

#ifndef __CDSADSINTERFACE_H__
#define __CDSADSINTERFACE_H__

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "vos.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)


/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* !!!!!!!!!!!暂时定义，最终的值由北京确定 */
#define ADS_CDS_MSG_HDR                (0x00)
#define CDS_ADS_MSG_HDR                (0x10)

#define CDS_ADS_ALL_RABID               (0xFF)


/*****************************************************************************
  3 枚举定义
*****************************************************************************/

enum CDS_ADS_MSG_ID_ENUM
{
    ID_CDS_ADS_STOP_SENDDATA_IND     = CDS_ADS_MSG_HDR + 0x01,                  /* CDS->ADS STOP SENDDATA IND */
    ID_CDS_ADS_STOP_SENDDATA_RSP     = ADS_CDS_MSG_HDR + 0x01,                  /* ADS->CDS STOP SENDDATA RSP */
    ID_CDS_ADS_START_SENDDATA_IND    = CDS_ADS_MSG_HDR + 0x02,                  /* CDS->ADS START SENDDATA IND */
    ID_CDS_ADS_START_SENDDATA_RSP    = ADS_CDS_MSG_HDR + 0x02,                  /* ADS->CDS START SENDDATA RSP */
    ID_CDS_ADS_CLEAR_DATA_IND        = CDS_ADS_MSG_HDR + 0x03,                  /* CDS->ADS CLEAR DATA IND */
    ID_CDS_ADS_CLEAR_DATA_RSP        = ADS_CDS_MSG_HDR + 0x03,                  /* ADS->CDS CLEAR DATA RSP */
    ID_CDS_ADS_IP_PACKET_IND         = CDS_ADS_MSG_HDR + 0x04,                  /* CDS->ADS IP PACKET IND */
    ID_ADS_CDS_ERR_IND               = ADS_CDS_MSG_HDR + 0x04,                  /* ADS->CDS IP PACKET DECODE ERR IND*/
    ID_CDS_ADS_MSG_ID_ENUM_BUTT
};
typedef VOS_UINT32  CDS_ADS_MSG_ID_ENUM_UINT32;



enum CDS_ADS_IP_PACKET_TYPE_ENUM
{
    CDS_ADS_IP_PACKET_TYPE_DHCP_SERVERV4   = 0x00,
    CDS_ADS_IP_PACKET_TYPE_ND_SERVERDHCPV6 = 0x01,
    CDS_ADS_IP_PACKET_TYPE_ICMPV6          = 0x02,
    CDS_ADS_IP_PACKET_TYPE_LINK_FE80       = 0x03,
    CDS_ADS_IP_PACKET_TYPE_LINK_FF         = 0x04,
    CDS_ADS_IP_PACKET_TYPE_BUTT
};
typedef VOS_UINT8 CDS_ADS_IP_PACKET_TYPE_ENUM_UINT8;


enum CDS_ADS_DL_IPF_BEARER_ID_ENUM
{
    CDS_ADS_DL_IPF_BEARER_ID_RSV0     = 0,                                      /* 0~4保留 */
    CDS_ADS_DL_IPF_BEARER_ID_EPSBID5  = 5,                                      /* 5~15 EPS Bearer ID*/
    CDS_ADS_DL_IPF_BEARER_ID_EPSBID15 = 15,
    CDS_ADS_DL_IPF_BEARER_ID_DHCPV4   = 16,                                     /* 下行DHCP */
    CDS_ADS_DL_IPF_BEARER_ID_DHCPV6   = 17,                                     /* 下行DHCPv6*/
    CDS_ADS_DL_IPF_BEARER_ID_ICMPV4   = 18,                                     /* 下行ICMP */
    CDS_ADS_DL_IPF_BEARER_ID_ICMPV6   = 19,                                     /* 下行ICMPv6 */
    CDS_ADS_DL_IPF_BEARER_ID_LL_FE80  = 20,
    CDS_ADS_DL_IPF_BEARER_ID_LL_FF    = 21,
    CDS_ADS_DL_IPF_BEARER_ID_INVALID  = 63                                      /* 不匹配任何Filter，0x3F*/
};
typedef VOS_UINT32 CDS_ADS_DL_IPF_BEARER_ID_ENUM_UINT32;


enum ADS_CDS_PKT_TYPE_ENUM
{
    ADS_CDS_IPF_PKT_TYPE_IP   = 0x00,                                           /* IP类型 */
    ADS_CDS_IPF_PKT_TYPE_PPP  = 0x01,                                           /* PPP类型 */
    ADS_CDS_IPF_PKT_TYPE_BUTT
};
typedef VOS_UINT8 ADS_CDS_IPF_PKT_TYPE_ENUM_UINT8;

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

typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* 消息ID */
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT8                           ucRabId;                                /* Rab Id*/
    VOS_UINT8                           aucReserved[1];

} CDS_ADS_STOP_SENDDATA_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* 消息ID */
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT8                           ucRabId;                                /* Rab Id*/
    VOS_UINT8                           aucReserved[1];
} CDS_ADS_STOP_SENDDATA_RSP_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* 消息ID */
    MODEM_ID_ENUM_UINT16                enModemId ;
    VOS_UINT8                           ucRabId;                                /* Rab Id*/
    VOS_UINT8                           aucReserved[1];
} CDS_ADS_START_SENDDATA_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* 消息ID */
    MODEM_ID_ENUM_UINT16                enModemId ;
    VOS_UINT8                           ucRabId;                                /* Rab Id*/
    VOS_UINT8                           aucReserved[1];
} CDS_ADS_START_SENDDATA_RSP_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* 消息ID */
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT8                           ucRabId;                                /* Rab Id*/
    VOS_UINT8                           aucRsv[1];                              /* 保留*/
} CDS_ADS_CLEAR_DATA_IND_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* 消息头 */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* 消息ID */
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT8                           ucRabId;                                /* Rab Id*/
    VOS_UINT8                           aucRsv[1];                              /* 保留*/
} CDS_ADS_CLEAR_DATA_RSP_STRU;


typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* _H2ASN_Skip */
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT8                           ucRabId;                                /* RAB标识，取值范围:[5,15] */
    CDS_ADS_IP_PACKET_TYPE_ENUM_UINT8   enIpPacketType;                         /* IP PACKET TYPE*/
    VOS_UINT16                          usLen;                                  /* Zc Len*/
    VOS_UINT8                           aucRsv[2];                              /* 保留*/
    VOS_UINT8                           aucData[4];                             /* 数据包 */
} CDS_ADS_DATA_IND_STRU;

/** ****************************************************************************
 * Name        : ADS_CDS_IP_PACKET_ERR_IND_STRU
 *
 * Description :
 *******************************************************************************/
typedef struct
{
    VOS_MSG_HEADER                                                              /* _H2ASN_Skip */
    CDS_ADS_MSG_ID_ENUM_UINT32          enMsgId;                                /* _H2ASN_Skip */
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT8                           ucRabId;                                /* RAB标识，取值范围:[5,15] */
    VOS_UINT8                           ucErrRate;                              /* 错包率，扩展备用，目前不处理*/
    VOS_UINT8                           aucRsv[4];                              /* 保留*/
}ADS_CDS_ERR_IND_STRU;

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

#endif

