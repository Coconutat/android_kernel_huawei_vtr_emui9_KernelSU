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

#ifndef __ADS_TCPIPTYPEDEF_H__
#define __ADS_TCPIPTYPEDEF_H__

/*****************************************************************************
  1 其他头文件包含
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

/* IP 数据包可维可测*/
#define ADS_IP_VERSION_V4               (4)                                     /* IPV4的版本号 */
#define ADS_IPV4_HDR_LEN                (20)                                    /* IPV4的头部长度 */
#define ADS_IPV4_PROTO_ICMP             (1)                                     /* IPV4的ICMP协议号 */
#define ADS_IPV4_ICMP_ECHO_REQUEST      (8)                                     /* IPV4的ICMP的TYPE ECHO REQ */
#define ADS_IPV4_ICMP_ECHO_REPLY        (0)                                     /* IPV4的ICMP的TYPE ECHO REPLY */

#define ADS_IP_VERSION_V6               (6)                                     /* IPV6的版本号 */
#define ADS_IPV6_HDR_LEN                (40)                                    /* IPV6的头部长度 */
#define ADS_IPV6_PROTO_ICMP             (58)                                    /* IPV6的ICMP协议号 */
#define ADS_IPV6_ICMP_ECHO_REQUEST      (128)                                   /* IPV6的ICMP的TYPE ECHO REQ */
#define ADS_IPV6_ICMP_ECHO_REPLY        (129)                                   /* IPV6的ICMP的TYPE ECHO REPLY */

#define ADS_IP_PROTO_TCP                (6)                                     /* TCP协议号 */
#define ADS_IP_PROTO_UDP                (17)                                    /* UDP协议号 */

#define ADS_IPV4_ADDR_LEN               (4)                                     /* IPV4地址长度 */
#define ADS_IPV6_ADDR_LEN               (16)                                    /* IPV6地址长度 */
#define ADS_IPV6_ADDR_HALF_LEN          (8)
#define ADS_IPV6_ADDR_QUARTER_LEN       (4)

#define ADS_GET_IP_VERSION(pucIpPkt)    ((pucIpPkt)[0] >> 4)                    /* 获取IP version */

/* 大小字节序转换*/
#ifndef VOS_NTOHL
#if VOS_BYTE_ORDER==VOS_BIG_ENDIAN
#define VOS_NTOHL(x)    (x)
#define VOS_HTONL(x)    (x)
#define VOS_NTOHS(x)    (x)
#define VOS_HTONS(x)    (x)
#else
#define VOS_NTOHL(x)    ((((x) & 0x000000ffU) << 24) | \
                         (((x) & 0x0000ff00U) <<  8) | \
                         (((x) & 0x00ff0000U) >>  8) | \
                         (((x) & 0xff000000U) >> 24))

#define VOS_HTONL(x)    ((((x) & 0x000000ffU) << 24) | \
                         (((x) & 0x0000ff00U) <<  8) | \
                         (((x) & 0x00ff0000U) >>  8) | \
                         (((x) & 0xff000000U) >> 24))

#define VOS_NTOHS(x)    ((((x) & 0x00ff) << 8) | \
                         (((x) & 0xff00) >> 8))

#define VOS_HTONS(x)    ((((x) & 0x00ff) << 8) | \
                         (((x) & 0xff00) >> 8))
#endif  /* _BYTE_ORDER==_LITTLE_ENDIAN */
#endif


/*****************************************************************************
  3 枚举定义
*****************************************************************************/


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
  7 UNION定义
*****************************************************************************/

#if (VOS_OS_VER != VOS_WIN32)
#pragma pack(1)
#else
#pragma pack(push, 1)
#endif


typedef union
{
    VOS_UINT8                           aucIpAddr[4];
    VOS_UINT32                          ulIpAddr;
} ADS_IPV4_ADDR_UN;


typedef union
{
    VOS_UINT8                           aucIpAddr[16];
    VOS_UINT16                          ausIpAddr[8];
    VOS_UINT32                          aulIpAddr[4];
} ADS_IPV6_ADDR_UN;

/*****************************************************************************
  8 STRUCT定义
*****************************************************************************/


typedef struct
{
    VOS_UINT8                           ucIpHdrLen          : 4;                /* header length */
    VOS_UINT8                           ucIpVersion         : 4;                /* version */
    VOS_UINT8                           ucTypeOfService;                        /* type of service */
    VOS_UINT16                          usTotalLen;                             /* total length */
    VOS_UINT16                          usIdentification;                       /* identification */
    VOS_UINT16                          usFlags_fo;                             /* flags and fragment offset field */
    VOS_UINT8                           ucTTL;                                  /* time to live*/
    VOS_UINT8                           ucProtocol;                             /* protocol */
    VOS_UINT16                          usCheckSum;                             /* checksum */
    ADS_IPV4_ADDR_UN                    unSrcAddr;                              /* source address */
    ADS_IPV4_ADDR_UN                    unDstAddr;                              /* dest address */
} ADS_IPV4_HDR_STRU;


typedef struct
{
    VOS_UINT8                           ucPriority          : 4;                /* Priority  */
    VOS_UINT8                           ucIpVersion         : 4;                /* ip version, to be 6 */
    VOS_UINT8                           aucFlowLabel[3];                        /* flow lable */
    VOS_UINT16                          usPayloadLen;                           /* not include ipv6 fixed hdr len 40bytes */
    VOS_UINT8                           ucNextHdr;                              /* for l4 protocol or ext hdr */
    VOS_UINT8                           ucHopLimit;
    ADS_IPV6_ADDR_UN                    unSrcAddr;
    ADS_IPV6_ADDR_UN                    unDstAddr;
} ADS_IPV6_HDR_STRU;


typedef struct
{
    VOS_UINT16                          usSrcPort;                              /* 源端口 */
    VOS_UINT16                          usDstPort;                              /* 目的端口 */
    VOS_UINT16                          usLen;                                  /* UDP包长度 */
    VOS_UINT16                          usChecksum;                             /* UDP校验和 */
} ADS_UDP_HDR_STRU;


typedef struct
{
    VOS_UINT16                          usSrcPort;
    VOS_UINT16                          usDstPort;
    VOS_UINT32                          ulSeqNum;
    VOS_UINT32                          ulAckNum;
    VOS_UINT16                          usReserved          : 4;
    VOS_UINT16                          usOffset            : 4;
    VOS_UINT16                          usFin               : 1;
    VOS_UINT16                          usSyn               : 1;
    VOS_UINT16                          usRst               : 1;
    VOS_UINT16                          usPsh               : 1;
    VOS_UINT16                          usAck               : 1;
    VOS_UINT16                          usUrg               : 1;
    VOS_UINT16                          usEce               : 1;
    VOS_UINT16                          usCwr               : 1;
    VOS_UINT16                          usWindowSize;
    VOS_UINT16                          usCheckSum;
    VOS_UINT16                          usUrgentPoint;
} ADS_TCP_HDR_STRU;


typedef struct
{
    VOS_UINT16                          usIdentifier;
    VOS_UINT16                          usSeqNum;

} ADS_ICMP_ECHO_HDR_STRU;


typedef struct
{
    VOS_UINT32                          ulUnUsed;

} ADS_ICMP_ERROR_HDR_STRU;


typedef struct
{
    VOS_UINT8                           ucType;
    VOS_UINT8                           ucCode;
    VOS_UINT16                          usCheckSum;

    union
    {
        ADS_ICMP_ECHO_HDR_STRU          stIcmpEcho;
        ADS_ICMP_ERROR_HDR_STRU         stIcmpError;
    }unIcmp;

} ADS_ICMP_HDR_STRU;

#if (VOS_OS_VER != VOS_WIN32)
#pragma pack()
#else
#pragma pack(pop)
#endif

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

#endif /* end of AdsTcpIpTypeDef.h */
