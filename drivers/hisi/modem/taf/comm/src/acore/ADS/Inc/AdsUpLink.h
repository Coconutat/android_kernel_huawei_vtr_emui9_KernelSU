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

#ifndef __ADSUPLINK_H__
#define __ADSUPLINK_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "AdsIntraMsg.h"
#include "AdsCtx.h"
#include "product_config.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/

/* 构造BD的user field 1,第二个字节为Modem id,第一个字节的高4位为PktTpye,第一个字节的低4位为RabId */
#define ADS_UL_BUILD_BD_USER_FIELD_1(Instance, RabId) \
        (((((VOS_UINT16)Instance) << 8) & 0xFF00) | ((ADS_UL_GET_QUEUE_PKT_TYPE(Instance, RabId) << 4) & 0xF0) | (RabId & 0x0F))

/* 过滤器组号，Modem0用0，MODEM1用1与实例号相同 */
#define ADS_UL_GET_BD_FC_HEAD(Instance)         (Instance)

/* 构造属性信息:
           bit0:   int_en  中断使能
           bit2:1  mode    模式控制
           bit3    rsv
           bit6:4  fc_head 过滤器组号 mfc_en控制域为1时，有效
           bit15:7 rsv
   Boston对应的结构:
           bit0:   int_en  中断使能
           bit2:1  mode    模式控制
           bit4:3  rsv
           bit8:5  fc_head 过滤器组号 mfc_en控制域为1时，有效
           bit15:9 rsv
*/
#ifdef CONFIG_NEW_PLATFORM
#define ADS_UL_BUILD_BD_ATTRIBUTE(Flag, Mode, FcHead) ((Flag & 0x000F) | (Mode << 1 & 0x0006) | (FcHead << 5 &0x01E0))
#else
#define ADS_UL_BUILD_BD_ATTRIBUTE(Flag, Mode, FcHead) ((Flag & 0x000F) | (Mode << 1 & 0x0006) | (FcHead << 4 &0x0070))
#endif

#define ADS_UL_SET_BD_ATTR_INT_FLAG(usAttr)            ((usAttr) = (usAttr) | 0x1)

#define ADS_UL_IPF_1XHRPD                   (IPF_1XHRPD_ULFC)

/*lint -emacro({717}, ADS_UL_SAVE_MODEMID_PKTTYEP_RABID_TO_IMM)*/
#define ADS_UL_SAVE_MODEMID_PKTTYEP_RABID_TO_IMM(pstImmZc, usModemId, ucPktType, ucRabId)\
            do\
            {\
                ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0] = (usModemId);\
                ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0] = ((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) << 8) | (ucPktType);\
                ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0] = ((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) << 8) | (ucRabId);\
            } while(0)

/*lint -emacro({717}, ADS_UL_SAVE_SLICE_TO_IMM)*/
#define ADS_UL_SAVE_SLICE_TO_IMM(pstImmZc, ulSlice)\
            do\
            {\
                ADS_IMM_MEM_CB(pstImmZc)->aulPriv[1] = (ulSlice);\
            } while(0)

/* 从IMM中获取ModemId */
#define ADS_UL_GET_MODEMID_FROM_IMM(pstImmZc)   ((VOS_UINT16)(((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) & 0xFFFF0000) >> 16))

/* 从IMM中获取PktType */
#define ADS_UL_GET_PKTTYPE_FROM_IMM(pstImmZc)   ((VOS_UINT8)(((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) & 0x0000FF00) >> 8))

/* 从IMM中获取RabId */
#define ADS_UL_GET_RABIID_FROM_IMM(pstImmZc)    ((VOS_UINT8)((ADS_IMM_MEM_CB(pstImmZc)->aulPriv[0]) & 0x000000FF))

/* 从IMM中获取Slice */
#define ADS_UL_GET_SLICE_FROM_IMM(pstImmZc)     ((VOS_UINT32)ADS_IMM_MEM_CB(pstImmZc)->aulPriv[1])

/* 上行内存cache flush (map) */
/*lint -emacro({717}, ADS_IPF_UL_MEM_MAP)*/
#define ADS_IPF_UL_MEM_MAP(pstImmZc, ulLen)\
            do\
            {\
                if (VOS_FALSE == ADS_IPF_IsSpeMem(pstImmZc))\
                {\
                    ADS_IPF_MemMapRequset(pstImmZc, ulLen, 1);\
                }\
            } while(0)

/* 上行内存cache flush (unmap) */
/*lint -emacro({717}, ADS_IPF_UL_MEM_UNMAP)*/
#define ADS_IPF_UL_MEM_UNMAP(pstImmZc, ulLen)\
            do\
            {\
                if (VOS_FALSE == ADS_IPF_IsSpeMem(pstImmZc))\
                {\
                    ADS_IPF_MemUnmapRequset(pstImmZc, ulLen, 1);\
                }\
            } while(0)


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
  7 STRUCT定义
*****************************************************************************/

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/

VOS_VOID ADS_UL_ConfigBD(VOS_UINT32 ulBdNum);
IMM_ZC_STRU* ADS_UL_GetInstanceNextQueueNode(
    VOS_UINT8                           ucInstanceIndex,
    VOS_UINT8                          *pucRabId,
    VOS_UINT8                          *puc1XorHrpdUlIpfFlag
);
IMM_ZC_STRU* ADS_UL_GetNextQueueNode(
    VOS_UINT8                          *pucRabId,
    VOS_UINT8                          *pucInstanceIndex,
    VOS_UINT8                          *puc1XorHrpdUlIpfFlag
);
VOS_VOID ADS_UL_ProcLinkData(VOS_VOID);
VOS_VOID ADS_UL_ProcMsg(MsgBlock* pMsg);
VOS_UINT32 ADS_UL_RcvTafMsg(MsgBlock *pMsg);
VOS_UINT32 ADS_UL_RcvTafPdpStatusInd(MsgBlock *pMsg);
VOS_UINT32 ADS_UL_ProcPdpStatusInd(
    ADS_PDP_STATUS_IND_STRU            *pstStatusInd
);
VOS_UINT32 ADS_UL_RcvCdsIpPacketMsg(MsgBlock *pMsg);
VOS_UINT32 ADS_UL_RcvCdsMsg(MsgBlock *pMsg);
VOS_UINT32 ADS_UL_RcvTimerMsg(MsgBlock *pMsg);
VOS_VOID ADS_UL_SaveIpfSrcMem(const ADS_IPF_BD_BUFF_STRU *pstIpfUlBdBuff, VOS_UINT32 ulSaveNum);
VOS_VOID ADS_UL_FreeIpfSrcMem(VOS_VOID);
VOS_VOID ADS_UL_ClearIpfSrcMem(VOS_VOID);
VOS_UINT32 ADS_UL_RcvCcpuResetStartInd(
    MsgBlock                           *pstMsg
);
VOS_VOID ADS_UL_RcvTiDsFlowStatsExpired(
    VOS_UINT32                          ulTimerName,
    VOS_UINT32                          ulParam
);
VOS_VOID ADS_UL_StartDsFlowStats(
    VOS_UINT8                           ucInstance,
    VOS_UINT8                           ucRabId
);
VOS_VOID ADS_UL_StopDsFlowStats(
    VOS_UINT8                           ucInstance,
    VOS_UINT8                           ucRabId
);
VOS_VOID ADS_UL_RcvTiDataStatExpired(
    VOS_UINT32                          ulTimerName,
    VOS_UINT32                          ulParam
);
VOS_INT ADS_UL_CCpuResetCallback(
    DRV_RESET_CB_MOMENT_E               enParam,
    VOS_INT                             iUserData
);
VOS_VOID ADS_UL_RcvTiSendExpired(
    VOS_UINT32                          ulParam,
    VOS_UINT32                          ulTimerName
);


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

#endif /* end of AdsUlProcData.h */
