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

#ifndef __ADSDEVINTERFACE_H__
#define __ADSDEVINTERFACE_H__

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "ImmInterface.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)


/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* 定义IPV6 MTU最大长度值 */
#define ADS_MTU_LEN_MAX                 (1500)

/* !!!!!!!!!!!暂时定义，最终的值由北京确定 */
#define ADS_NDIS_MSG_HDR                (0x00)
#define NDIS_ADS_MSG_HDR                (0x00)


/*****************************************************************************
  3 枚举定义
*****************************************************************************/

enum ADS_PKT_TYPE_ENUM
{
    ADS_PKT_TYPE_IPV4 = 0x00,                                                   /* IPV4 */
    ADS_PKT_TYPE_IPV6 = 0x01,                                                   /* IPV6 */
    ADS_PKT_TYPE_BUTT
};
typedef VOS_UINT8 ADS_PKT_TYPE_ENUM_UINT8;


enum ADS_NDIS_IP_PACKET_TYPE_ENUM
{
    ADS_NDIS_IP_PACKET_TYPE_DHCPV4      = 0x00,
    ADS_NDIS_IP_PACKET_TYPE_DHCPV6      = 0x01,
    ADS_NDIS_IP_PACKET_TYPE_ICMPV6      = 0x02,
    ADS_NDIS_IP_PACKET_TYPE_LINK_FE80   = 0x03,
    ADS_NDIS_IP_PACKET_TYPE_LINK_FF     = 0x04,
    ADS_NDIS_IP_PACKET_TYPE_BUTT
};
typedef VOS_UINT8 ADS_NDIS_IP_PACKET_TYPE_ENUM_UINT8;


enum ADS_NDIS_MSG_ID_ENUM
{
    ID_ADS_NDIS_DATA_IND               = ADS_NDIS_MSG_HDR + 0x00,               /* ADS->CDS IP PACKET IND */
    ID_ADS_NDIS_MSG_ID_ENUM_BUTT
};
typedef VOS_UINT32  ADS_NDIS_MSG_ID_ENUM_UINT32;


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
    VOS_MSG_HEADER                                                              /*_H2ASN_Skip*/
    ADS_NDIS_MSG_ID_ENUM_UINT32         enMsgId;                                /*_H2ASN_Skip*/
    MODEM_ID_ENUM_UINT16                enModemId;
    VOS_UINT8                           ucRabId;                                /* RAB标识，取值范围:[5,15] */
    ADS_NDIS_IP_PACKET_TYPE_ENUM_UINT8  enIpPacketType;
    VOS_UINT8                           aucReserved[4];
    IMM_ZC_STRU                        *pstSkBuff;                              /* 数据包结构指针 */
} ADS_NDIS_DATA_IND_STRU;


typedef struct
{
    VOS_UINT32                          bitOpIpv4Addr       : 1;
    VOS_UINT32                          bitOpIpv6Addr       : 1;
    VOS_UINT32                          bitOpSpare          : 30;

    VOS_UINT8                           aucIpv4Addr[4];
    VOS_UINT8                           aucIpv6Addr[16];
} ADS_FILTER_IP_ADDR_INFO_STRU;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/
/*****************************************************************************
 函 数 名  : ADS_UL_SendPacket
 功能描述  : ADS上行为上层模块提供的数据发送函数，本接口不释放内存。
            上层模块根据返回值判断是否需要释放内存
 输入参数  : pstImmZc  --- IMM数据
             ucExRabId --- RABID [5, 15]
             (*****ucExRabId为扩展的RabId，高2bit表示ModemId，低6bit表示RabId*****)
 输出参数  : 无
 返 回 值  : VOS_UINT32
*****************************************************************************/

VOS_UINT32 ADS_UL_SendPacket(
    IMM_ZC_STRU                        *pstImmZc,
    VOS_UINT8                           ucExRabId
);

typedef VOS_UINT32 (*RCV_DL_DATA_FUNC)(VOS_UINT8 ucExRabId, IMM_ZC_STRU *pData, ADS_PKT_TYPE_ENUM_UINT8 enPktType, VOS_UINT32 ulExParam);

/*****************************************************************************
 函 数 名  : ADS_DL_RegDlDataCallback
 功能描述  : ADS下行数据处理模块为上层模块提供的注册下行数据接收函数接口
 输入参数  : ucExRabId --- 扩展承载号
             (*****ucExRabId为扩展的RabId，高2bit表示ModemId，低6bit表示RabId*****)
             pFunc     --- 数据接收回调函数指针
             ulExParam --- 扩展参数
 输出参数  : 无
 返 回 值  : VOS_OK/VOS_ERR
*****************************************************************************/
VOS_UINT32 ADS_DL_RegDlDataCallback(
    VOS_UINT8                           ucExRabId,
    RCV_DL_DATA_FUNC                    pFunc,
    VOS_UINT32                          ulExParam
);

/*****************************************************************************
 函 数 名  : ADS_DL_RegFilterDataCallback
 功能描述  : 注册下行数据过滤回调扩展
 输入参数  : ucRabId         - RABID [5, 15]
             pstFilterIpAddr - IP地址信息
             pFunc           - 函数指针
 输出参数  : 无
 返 回 值  : VOS_OK          - 注册成功
             VOS_ERR         - 注册失败
*****************************************************************************/
VOS_UINT32 ADS_DL_RegFilterDataCallback(
    VOS_UINT8                           ucRabId,
    ADS_FILTER_IP_ADDR_INFO_STRU       *pstFilterIpAddr,
    RCV_DL_DATA_FUNC                    pFunc
);

/*****************************************************************************
 函 数 名  : ADS_DL_DeregFilterDataCallback
 功能描述  : 去注册下行数据过滤回调扩展
 输入参数  : ucRabId - RABID [5, 15]
 输出参数  : 无
 返 回 值  : VOS_OK  - 去注册成功
             VOS_ERR - 去注册失败
*****************************************************************************/
VOS_UINT32 ADS_DL_DeregFilterDataCallback(VOS_UINT8 ucRabId);

/*****************************************************************************
 函 数 名  : ADS_UL_SendPacketEx
 功能描述  : 上行发送数据扩展接口, 使用该接口发送的数据在ADS记录其信息, 用于
             下行数据过滤匹配, 该接口必须和ADS_DL_RegFilterDataCallback配合
             使用, 只有使用ADS_DL_RegFilterDataCallback注册过下行过滤回调后,
             下行数据才需要根据使用该接口记录的信息进行过滤
 输入参数  : pstImmZc  --- IMM数据
             enIpType  --- IP类型
             ucExRabId --- RABID [5, 15]
             (*****ucExRabId为扩展的RabId，高2bit表示ModemId，低6bit表示RabId*****)
 输出参数  : 无
 返 回 值  : VOS_OK/VOS_ERR
*****************************************************************************/
VOS_UINT32 ADS_UL_SendPacketEx(
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enIpType,
    VOS_UINT8                           ucExRabId
);

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
typedef VOS_VOID (*RCV_RD_LAST_DATA_FUNC)(VOS_UINT32 ulRmNetId);

/*********************************************************************************
 函 数 名  : ADS_DL_RegRdLastDataCallback
 功能描述  : ADS下行数据处理模块为上层模块提供的注册下行RD最后一个数据处理回调函数
 输入参数  : ucExRabId --- 扩展承载号
             (*****ucExRabId为扩展的RabId，高2bit表示ModemId，低6bit表示RabId*****)
             pFunc     --- 数据接收回调函数指针
             ulExParam --- 扩展参数
 输出参数  : 无
 返 回 值  : VOS_OK/VOS_ERR
*********************************************************************************/
VOS_UINT32 ADS_DL_RegRdLastDataCallback(
    VOS_UINT8                           ucExRabId,
    RCV_RD_LAST_DATA_FUNC               pFunc,
    VOS_UINT32                          ulExParam
);
#endif

#if (defined(CONFIG_BALONG_SPE))
/*****************************************************************************
 函 数 名  : ADS_IPF_SpeIntWakeupADS
 功能描述  : SPE中断触发事件, 唤醒ADS任务处理上行数据
 输入参数  : VOS_VOID
 输出参数  : 无
 返 回 值  : VOS_VOID
*****************************************************************************/
VOS_VOID ADS_IPF_SpeIntWakeupADS(VOS_VOID);

/*****************************************************************************
 函 数 名  : ADS_IPF_RecycleMem
 功能描述  : 回收内存
 输入参数  : pstImmZc --- imm memory
 输出参数  : 无
 返 回 值  : VOS_OK/VOS_ERROR
*****************************************************************************/
VOS_INT ADS_IPF_RecycleMem(IMM_ZC_STRU *pstImmZc);

/*****************************************************************************
 函 数 名  : ADS_IPF_RegSpeWPort
 功能描述  : 注册SPE端口
 输入参数  : lPort --- 端口
 输出参数  : 无
 返 回 值  : VOS_VOID
*****************************************************************************/
VOS_VOID ADS_IPF_RegSpeWPort(VOS_INT32 lPort);

/*****************************************************************************
 函 数 名  : ADS_IPF_GetSpeWPortTdDepth
 功能描述  : 获取SPE WPORT端口的TD深度配置
 输入参数  : VOS_VOID
 输出参数  : 无
 返 回 值  : 0    --- 使用SPE的默认配置
             其他 --- 根据ADS预申请内存块数量计算的深度
*****************************************************************************/
VOS_UINT32 ADS_IPF_GetSpeWPortTdDepth(VOS_VOID);
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

#endif

