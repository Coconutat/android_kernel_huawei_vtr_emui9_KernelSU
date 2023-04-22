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




/******************************************************************************
   1 头文件包含
******************************************************************************/
#include "PsTypeDef.h"
#include "TTFComm.h"
#include "TtfIpComm.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
/*lint -e767*/
#define    THIS_FILE_ID        PS_FILE_ID_TTF_IP_COMM_C
/*lint +e767*/


/******************************************************************************
   2 外部函数变量声明
******************************************************************************/

/******************************************************************************
   3 私有定义
******************************************************************************/


/******************************************************************************
   4 全局变量定义
******************************************************************************/


/******************************************************************************
   5 函数实现
******************************************************************************/
/*lint -save -e958 */


 VOS_UINT32 TTF_CheckIpDataByProtocalType
(
    VOS_UINT8                   *pucData,
    VOS_UINT16                  usMemUsedLen,
    VOS_UINT16                  usIpHeadLen,
    VOS_UINT16                  usIpTotalLen,
    IP_DATA_PROTOCOL_ENUM_UINT8 enDataProtocalType
)
{
    VOS_UINT16                  usTcpHeadLen;
    VOS_UINT16                  usUdpLen;

    if (usMemUsedLen < (usIpTotalLen) || (usIpTotalLen < usIpHeadLen))
    {
        /* 数据包长度，小于IP总长度直接退出 
		   usIpTotalLen 字段异常，小于IP包头，直接退出 */
        return PS_FAIL;
    }

    switch (enDataProtocalType)
    {
        case IP_DATA_PROTOCOL_TCP:
            {
                if (usMemUsedLen < (usIpHeadLen+TTF_TCP_HEAD_NORMAL_LEN))
                {
                    /* 数据包长度，不够容纳完整的TCP头， */
                    return PS_FAIL;
                }

                usTcpHeadLen = (pucData[usIpHeadLen + TCP_LEN_POS] & TCP_LEN_MASK) >> 2;

                if ( usMemUsedLen < (usTcpHeadLen + usIpHeadLen) )
                {
                    /* 数据包长度，不够容纳完整的TCP头， */
                    return PS_FAIL;
                }
            }
            break;

        case IP_DATA_PROTOCOL_UDP:
            {
                if ( usMemUsedLen < (usIpHeadLen + UDP_HEAD_LEN))
                {
                    /* 数据包长度，不够容纳完整的UDP头 */
                    return PS_FAIL;
                }

                /* 获取 UDP的总长度 */
                usUdpLen  = IP_GET_VAL_NTOH_U16(pucData, (usIpHeadLen+TTF_UDP_LEN_POS));
                if ( usUdpLen < UDP_HEAD_LEN)
                {
                    /* UdpLen 字段非法 */
                    return PS_FAIL;
                }
            }
            break;

        case IP_DATA_PROTOCOL_ICMPV4:
            {
                if (usMemUsedLen < (usIpHeadLen+ICMP_HEADER_LEN))
                {
                    /* 数据包长度，不够容纳完整的ICMPV4头 */
                    return PS_FAIL;
                }
            }
            break;

        case IP_DATA_PROTOCOL_ICMPV6:
            {
                if (usMemUsedLen < (usIpHeadLen+ICMP_HEADER_LEN))
                {
                    /* 数据包长度，不够容纳完整的ICMPV6头 */
                    return PS_FAIL;
                }
            }
            break;

        default:
            break;
    }


    return PS_SUCC;
}

IP_DATA_TYPE_ENUM_UINT8 TTF_ParseIpDataType
(
    VOS_UINT32                          ulPid,
    TTF_MEM_ST                         *pMemPt
)
{
    VOS_UINT16                                  usIpHeadLen;
    VOS_UINT16                                  usIpTotalLen;
    VOS_UINT16                                  usTcpHeadLen;
    IP_DATA_TYPE_ENUM_UINT8                     enDataType;
    IP_DATA_PROTOCOL_ENUM_UINT8                 enDataProtocalType;
    VOS_UINT16                                 *pusPort;
    VOS_UINT16                                 *pusFragmentOffset;
    VOS_UINT8                                   usTcpFlags;
    VOS_UINT8                                  *pData       = pMemPt->pData;

    /* 初始化设置为Null */
    enDataType = IP_DATA_TYPE_NULL;

    /* 内存至少有20字节，才能解析IP头的协议字段 ROTOCOL_POS(9), PROTOCOL_POS(6)*/
    if (pMemPt->usUsed <= IPV4_HEAD_NORMAL_LEN)
    {
        TTF_LOG(ulPid, DIAG_MODE_COMM, PS_PRINT_WARNING, "TTF_ParseIpDataType IPHeadLen is exception.");
        return IP_DATA_TYPE_BUTT;
    }

    if ( IPV4_VER_VAL == (pData[0] & IP_VER_MASK) )
    {
        usIpHeadLen         = (pData[0] & IP_HEADER_LEN_MASK) << 2;
        usIpTotalLen        = IP_GET_VAL_NTOH_U16(pData, IP_IPV4_DATA_LEN_POS);
        enDataProtocalType  = pData[PROTOCOL_POS];
    }
    else if( IPV6_VER_VAL == (pData[0] & IP_VER_MASK) )
    {
        usIpHeadLen         = IPV6_HEAD_NORMAL_LEN;
        usIpTotalLen        = IP_GET_VAL_NTOH_U16(pData, IP_IPV6_DATA_LEN_POS) + IPV6_HEAD_NORMAL_LEN;
        enDataProtocalType  = pData[PROTOCOL_POS_V6];
    }
    else
    {
        TTF_LOG(ulPid, DIAG_MODE_COMM, PS_PRINT_WARNING, "TTF_ParseIpDataType Protocol is Null.");
        return IP_DATA_TYPE_BUTT;
    }

    if(TTF_PS_DATA_PRIORITY_HIGH == pMemPt->ucDataPriority)
    {
        TTF_LOG(ulPid, DIAG_MODE_COMM, PS_PRINT_WARNING, "TTF_ParseIpDataType user high priority data.");
        return IP_DATA_TYPE_USER_HIGH;
    }

    /* 安全检查: 检查数据包大小是否能够容纳对应协议包头，不能容纳的异常包，就不用继续解析了*/
    if (PS_FAIL == TTF_CheckIpDataByProtocalType(pData, pMemPt->usUsed, usIpHeadLen, usIpTotalLen, enDataProtocalType))
    {
        TTF_LOG2(ulPid, DIAG_MODE_COMM, PS_PRINT_WARNING, "TTF_ParseIpDataType datalen<1> ProtocalType<2> is exception.",pMemPt->usUsed,enDataProtocalType);
        return IP_DATA_TYPE_BUTT;
    }

    switch (enDataProtocalType)
    {
        case IP_DATA_PROTOCOL_TCP:
            {
                enDataType   = IP_DATA_TYPE_TCP;

                usTcpHeadLen = (pData[usIpHeadLen + TCP_LEN_POS] & TCP_LEN_MASK) >> 2;

                /* SDU数据长度等于IP包头长度和TCP包头部长度之和，并且TCP包FLAG标志中含有ACK */
                if ( usIpTotalLen == (usTcpHeadLen + usIpHeadLen) )
                {
                    usTcpFlags = pData[usIpHeadLen + TCP_FLAG_POS] & 0x3F;

                    if (TCP_SYN_MASK == (TCP_SYN_MASK & usTcpFlags))
                    {
                        enDataType = IP_DATA_TYPE_TCP_SYN;
                        break;
                    }

                    if (TCP_ACK_MASK == (TCP_ACK_MASK & usTcpFlags))
                    {
                        enDataType = IP_DATA_TYPE_TCP_ACK;
                    }
                }
                else
                {
                    pusPort = (VOS_UINT16 *)&pData[usIpHeadLen + TCP_DST_PORT_POS];
                    if (FTP_DEF_SERVER_SIGNALLING_PORT == ntohs(*pusPort))
                    {
                        enDataType = IP_DATA_TYPE_FTP_SIGNALLING;
                    }
                }
            }
            break;

        case IP_DATA_PROTOCOL_UDP:
            {
                enDataType = IP_DATA_TYPE_UDP;

                pusPort = (VOS_UINT16 *)&pData[usIpHeadLen + UDP_DST_PORT_POS];

                if (DNS_DEF_SERVER_PORT == ntohs(*pusPort))
                {
                    enDataType = IP_DATA_TYPE_UDP_DNS;
                }
            }
            break;

        case IP_DATA_PROTOCOL_ICMPV4:
            {
                pusFragmentOffset = (VOS_UINT16 *)&pData[IPV4_HEAD_FRAGMENT_OFFSET_POS];

                /* 分段 */
                if (ntohs(*pusFragmentOffset) & IPV4_HEAD_FRAGMENT_OFFSET_MASK)
                {
                    break;
                }

                /* 获取ICMP报文的类型 */
                if ((ICMP_TYPE_REQUEST == pData[usIpHeadLen]) || (ICMP_TYPE_REPLY == pData[usIpHeadLen]))
                {
                    enDataType = IP_DATA_TYPE_ICMP;
                }
            }
            break;

        case IP_DATA_PROTOCOL_ICMPV6:
            {
                /* 获取ICMPV6报文的类型 */
                if ((ICMPV6_TYPE_REQUEST == pData[usIpHeadLen]) || (ICMPV6_TYPE_REPLY == pData[usIpHeadLen]))
                {
                    enDataType = IP_DATA_TYPE_ICMP;
                }
            }
            break;

        case IP_DATA_PROTOCOL_IPV6_FRAGMENT:
            break;

        default:
            break;
    }

    return enDataType;
} /* TTF_ParseIpDataType */


VOS_UINT16 TTF_GetIpDataTraceLen
(
    VOS_UINT32                          ulPid,
    VOS_UINT8                          *pData,
    VOS_UINT16                          usSduLen
)
{
    VOS_UINT16                                  usIpHeadLen;
    VOS_UINT16                                  usIpTotalLen;
    VOS_UINT16                                  usTcpHeadLen;
    IP_DATA_PROTOCOL_ENUM_UINT8                 enDataProtocalType;
    VOS_UINT16                                 *pusSourcePort;
    VOS_UINT16                                 *pusDestPort;
    VOS_UINT16                                  usIpDataTraceLen;

    /* 内存至少有20字节，才能解析IP头的协议字段 ROTOCOL_POS(9), PROTOCOL_POS(6)*/
    if (usSduLen <= IPV4_HEAD_NORMAL_LEN)
    {
        TTF_LOG(ulPid, DIAG_MODE_COMM, PS_PRINT_WARNING, "TTF_ParseIpDataType IPHeadLen is exception.");
        return 0;
    }

    if ( IPV4_VER_VAL == (pData[0] & IP_VER_MASK) )
    {
        usIpHeadLen         = (pData[0] & IP_HEADER_LEN_MASK) << 2;
        usIpTotalLen        = IP_GET_VAL_NTOH_U16(pData, IP_IPV4_DATA_LEN_POS);
        enDataProtocalType  = pData[PROTOCOL_POS];
    }
    else if( IPV6_VER_VAL == (pData[0] & IP_VER_MASK) )
    {
        usIpHeadLen         = IPV6_HEAD_NORMAL_LEN;
        usIpTotalLen        = IP_GET_VAL_NTOH_U16(pData, IP_IPV6_DATA_LEN_POS) + IPV6_HEAD_NORMAL_LEN;
        enDataProtocalType  = pData[PROTOCOL_POS_V6];
    }
    else
    {
        TTF_LOG(ulPid, DIAG_MODE_COMM, PS_PRINT_WARNING, "TTF_GetIpDataTraceLen Protocol is Null.");
        return 0;
    }

    /* 安全检查: 检查数据包大小是否能够容纳对应协议包头，不能容纳的异常包，就不用继续解析了*/
    if (PS_FAIL == TTF_CheckIpDataByProtocalType(pData, usSduLen, usIpHeadLen, usIpTotalLen, enDataProtocalType))
    {
        TTF_LOG2(ulPid, DIAG_MODE_COMM, PS_PRINT_WARNING, "TTF_GetIpDataTraceLen datalen<1> ProtocalType<2> is exception.",usSduLen,enDataProtocalType);
        return 0;
    }

    usIpDataTraceLen = usIpHeadLen;

    switch (enDataProtocalType)
    {
        case IP_DATA_PROTOCOL_TCP:
            {
                usTcpHeadLen    = (pData[usIpHeadLen + TCP_LEN_POS] & TCP_LEN_MASK) >> 2;

                /* SDU数据长度等于IP包头长度和TCP包头部长度之和，并且TCP包FLAG标志中含有ACK */
                if ( usIpTotalLen == (usTcpHeadLen + usIpHeadLen) )
                {
                    usIpDataTraceLen = usIpTotalLen;
                }
                else
                {
                    pusSourcePort   = (VOS_UINT16 *)&pData[usIpHeadLen];
                    pusDestPort     = (VOS_UINT16 *)&pData[usIpHeadLen + TCP_DST_PORT_POS];

                    /* FTP命令全部勾取，其它勾TCP头 */
                    if ((FTP_DEF_SERVER_SIGNALLING_PORT == ntohs(*pusSourcePort)) || (FTP_DEF_SERVER_SIGNALLING_PORT == ntohs(*pusDestPort)))
                    {
                        usIpDataTraceLen = usIpTotalLen;
                    }
                    else
                    {
                        usIpDataTraceLen = usIpHeadLen + usTcpHeadLen;
                    }
                }
            }
            break;

        case IP_DATA_PROTOCOL_UDP:
            {
                pusSourcePort   = (VOS_UINT16 *)&pData[usIpHeadLen];
                pusDestPort     = (VOS_UINT16 *)&pData[usIpHeadLen + UDP_DST_PORT_POS];

                /* DNS全部勾取，其它勾UDP头 */
                if ((DNS_DEF_SERVER_PORT == ntohs(*pusSourcePort)) || (DNS_DEF_SERVER_PORT == ntohs(*pusDestPort)))
                {
                    usIpDataTraceLen = usIpTotalLen;
                }
                else
                {
                    usIpDataTraceLen = usIpHeadLen + UDP_HEAD_LEN;
                }
            }
            break;

        case IP_DATA_PROTOCOL_ICMPV4:
            {
                usIpDataTraceLen = usIpHeadLen + ICMP_HEADER_LEN;
            }
            break;

        case IP_DATA_PROTOCOL_ICMPV6:
            {
                usIpDataTraceLen = usIpHeadLen + ICMP_HEADER_LEN;
            }
            break;

        default:
            break;
    }

    return usIpDataTraceLen;
}


/*lint -restore */



