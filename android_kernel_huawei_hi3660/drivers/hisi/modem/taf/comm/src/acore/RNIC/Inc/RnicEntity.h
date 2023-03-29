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
#ifndef __RNIC_ENTITY_H__
#define __RNIC_ENTITY_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "AtRnicInterface.h"
#include "PsCommonDef.h"
#include "ImmInterface.h"
#include "AdsDeviceInterface.h"
#include "RnicLinuxInterface.h"
#include "RnicCtx.h"
#if (defined(CONFIG_BALONG_SPE) && (VOS_LINUX == VOS_OS_VER))
#include "mdrv_spe_wport.h"
#endif
#include "RnicSndMsg.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/


#ifndef VOS_NTOHL                   /* 大小字节序转换*/
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

#if (defined(CONFIG_BALONG_SPE))
/* 设置SPE端口参数 */
/*lint -emacro({717}, RNIC_SET_SPE_PORT_ATTR)*/
#define RNIC_SET_SPE_PORT_ATTR(pstAttr,pstNetDev,ulTdNum,ulRdNum)\
            do\
            {\
                ((spe_wport_attr_t *)(pstAttr))->is_bypass_mode = 0;\
                ((spe_wport_attr_t *)(pstAttr))->is_ipf_port    = 1;\
                ((spe_wport_attr_t *)(pstAttr))->net_dev        = (pstNetDev);\
                ((spe_wport_attr_t *)(pstAttr))->td_depth       = (ulTdNum);\
                ((spe_wport_attr_t *)(pstAttr))->rd_depth       = (ulRdNum);\
            } while(0)
#endif

#define RNIC_BUILD_EXRABID(modemid, rabid)\
            ((VOS_UINT8)((((modemid) << 6) & 0xC0) | ((rabid) & 0x3F)))

#define RNIC_GET_MODEMID_FROM_EXRABID(ucExRabId) \
            (((VOS_UINT8)(ucExRabId) & (RNIC_RABID_TAKE_MODEM_1_MASK | RNIC_RABID_TAKE_MODEM_2_MASK)) >> 6)

#define RNIC_GET_RABID_FROM_EXRABID(ucExRabId) \
            ((VOS_UINT8)(ucExRabId) & RNIC_RABID_UNTAKE_MODEM_MASK)

#define RNIC_GET_RMNETID_FROM_EXPARAM(ulExParam)\
            ((VOS_UINT8)((ulExParam) & 0x000000FF))

#define RNIC_SPE_CACHE_HDR_SIZE         (RNIC_MAC_HDR_LEN + RNIC_IPV4_HDR_LEN)

#if (defined(CONFIG_BALONG_SPE))
#define RNIC_SPE_MEM_CB(pstImmZc)       ((RNIC_SPE_MEM_CB_STRU *)&((pstImmZc)->dma))

/*lint -emacro({717}, RNIC_SPE_MEM_MAP)*/
#define RNIC_SPE_MEM_MAP(pstImmZc, ulLen)\
            do\
            {\
                if (VOS_TRUE == RNIC_IsSpeMem(pstImmZc))\
                {\
                    RNIC_SpeMemMapRequset(pstImmZc, ulLen);\
                }\
            } while(0)

/*lint -emacro({717}, RNIC_SPE_MEM_UNMAP)*/
#define RNIC_SPE_MEM_UNMAP(pstImmZc, ulLen)\
            do\
            {\
                if (VOS_TRUE == RNIC_IsSpeMem(pstImmZc))\
                {\
                    RNIC_SpeMemUnmapRequset(pstImmZc, ulLen);\
                }\
            } while(0)
#else
#define RNIC_SPE_MEM_MAP(pstImmZc, ulLen)
#define RNIC_SPE_MEM_UNMAP(pstImmZc, ulLen)
#endif

/*******************************************************************************
  3 枚举定义
*******************************************************************************/


enum RNIC_RESULT_TYPE_ENUM
{
    RNIC_OK             = 0,                                                    /* 正常返回 */
    RNIC_NOTSTARTED     = 1,                                                    /* 未开始 */
    RNIC_INPROGRESS     = 2,                                                    /* 运行中 */
    RNIC_PERM           = 3,
    RNIC_NOENT          = 4,
    RNIC_IO             = 5,
    RNIC_NXIO           = 6,
    RNIC_NOMEM          = 7,                                                    /* 未申请到内存 */                                                    /* 未申请到内存 */
    RNIC_BUSY           = 8,                                                    /* RNIC网卡设备忙 */
    RNIC_NODEV          = 9,                                                    /* 无设备 */
    RNIC_INVAL          = 10,                                                   /* 非法设备 */
    RNIC_NOTSUPP        = 11,                                                   /* 操作不支持 */
    RNIC_TIMEDOUT       = 12,                                                   /* 超时 */
    RNIC_SUSPENDED      = 13,                                                   /* 挂起 */
    RNIC_UNKNOWN        = 14,                                                   /* 未知错误 */
    RNIC_TEST_FAILED    = 15,                                                   /* 测试失败 */
    RNIC_STATE          = 16,                                                   /* 状态错误 */
    RNIC_STALLED        = 17,                                                   /* 失速 */
    RNIC_PARAM          = 18,                                                   /* 参数错误 */
    RNIC_ABORTED        = 19,                                                   /* 请求取消 */
    RNIC_SHORT          = 20,                                                   /* 资源不足 */
    RNIC_EXPIRED        = 21,                                                   /* 溢出 */

    RNIC_ADDR_INVALID   = 22,                                                   /* 无法分配地址 */
    RNIC_OUT_RANGE      = 23,                                                   /* 不在有效范围内 */
    RNIC_PKT_TYPE_INVAL = 24,                                                   /* 无效ip类型 */
    RNIC_ADDMAC_FAIL    = 25,                                                   /* 添加mac头失败 */
    RNIC_RX_PKT_FAIL    = 26,                                                   /* 调用内核接口接收数据失败 */
    RNIC_ERROR          = 0xff,                                                 /* RNIC返回失败 */
    RNIC_BUTT
};
typedef VOS_INT32 RNIC_RESULT_TYPE_ENUM_INT32;

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
 结构名称  : RNIC_SPE_MEM_CB_STRU
 结构说明  : SPE MEM CB结构
*****************************************************************************/
typedef struct
{
    dma_addr_t                          ulDmaAddr;

} RNIC_SPE_MEM_CB_STRU;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
#if (FEATURE_ON == FEATURE_IMS)
VOS_VOID RNIC_ProcVoWifiULData(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
);
#endif
VOS_VOID RNIC_SendULDataInPdpActive(
    IMM_ZC_STRU                        *pstImmZc,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    VOS_UINT8                           ucRabId,
    ADS_PKT_TYPE_ENUM_UINT8             enIpType
);
VOS_VOID RNIC_SendULIpv4Data(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
);
VOS_VOID RNIC_SendULIpv6Data(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
);

VOS_UINT32 RNIC_SendDlData(
    RNIC_RMNET_ID_ENUM_UINT8            enRmNetId,
    IMM_ZC_STRU                        *pstData,
    ADS_PKT_TYPE_ENUM_UINT8             enPdpType
);

#if (FEATURE_ON == FEATURE_CL_INTERWORK)
VOS_UINT32 RNIC_RcvSdioDlData(
    VOS_UINT8                           ucPdnId,
    IMM_ZC_STRU                        *pstData
);
#endif

VOS_UINT32  RNIC_RcvAdsDlData(
    VOS_UINT8                           ucExRabid,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType,
    VOS_UINT32                          ucExParam
);

VOS_VOID RNIC_RcvInsideModemUlData(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
);

#if (FEATURE_ON == FEATURE_CL_INTERWORK)
VOS_VOID RNIC_RcvOutsideModemUlData(
    struct sk_buff                     *pstSkb,
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt
);
#endif

VOS_VOID RNIC_ProcessTxDataByModemType(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    struct sk_buff                     *pstSkb
);
#if (defined(CONFIG_BALONG_SPE))
VOS_UINT32 RNIC_IsSpeMem(IMM_ZC_STRU *pstImmZc);
dma_addr_t RNIC_GetMemDma(IMM_ZC_STRU *pstImmZc);
VOS_VOID RNIC_SpeMemMapRequset(
    IMM_ZC_STRU                        *pstImmZc,
    VOS_UINT32                          ulLen
);
VOS_VOID RNIC_SpeMemUnmapRequset(
    IMM_ZC_STRU                        *pstImmZc,
    VOS_UINT32                          ulLen
);
VOS_VOID RNIC_SpeReadCB(VOS_INT32 lPort, struct sk_buff *pstSkb);
VOS_UINT32 RNIC_SpeRxData(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
);
VOS_VOID RNIC_SpeInit(VOS_VOID);
#endif

VOS_INT32 RNIC_NetIfRx(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc
);
VOS_INT32 RNIC_NetIfRxEx(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc
);
VOS_UINT32 RNIC_EncapEthHead(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
);
VOS_UINT32 RNIC_NetRxData(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
);
VOS_UINT32 RNIC_NetRxDataEx(
    RNIC_SPEC_CTX_STRU                 *pstNetCntxt,
    IMM_ZC_STRU                        *pstImmZc,
    ADS_PKT_TYPE_ENUM_UINT8             enPktType
);

#if (FEATURE_ON == FEATURE_RNIC_NAPI_GRO)
VOS_VOID RNIC_NapiSchedule(VOS_UINT32 ulRmNetId);
VOS_INT32 RNIC_Poll(
    struct napi_struct                 *pstNapi,
    VOS_INT32                           lWeight
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

#endif
