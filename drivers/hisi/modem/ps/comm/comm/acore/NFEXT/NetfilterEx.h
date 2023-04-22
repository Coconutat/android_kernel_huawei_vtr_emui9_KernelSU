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

#ifndef _NETFILTEREX_H_
#define _NETFILTEREX_H_

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "v_id.h"
#include "vos.h"
#include "SysNvId.h"
#include "NVIM_Interface.h"
#include "NetWInterface.h"
#include "PsTypeDef.h"
#include "IpsMntn.h"
#include "omringbuffer.h"
#include "PsCommonDef.h"
#include "LinuxInterface.h"
#include "TtfNvInterface.h"

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#ifndef __UT_CENTER__
#define NF_EXT_MEM_ALLOC(pid, size)                  kmalloc(size, GFP_ATOMIC)
#define NF_EXT_MEM_FREE(pid, p)                      kfree(p);
#else
#define NF_EXT_MEM_ALLOC(pid, size)                  PS_MEM_ALLOC(pid, size)
#define NF_EXT_MEM_FREE(pid, p)                      PS_MEM_FREE(pid, p);
#endif

#if ((SC_CTRL_MOD_P532 == SC_CTRL_MOD) || (SC_CTRL_MOD_6932_SFT == SC_CTRL_MOD))
#define NF_EXT_RING_BUF_SIZE                        (2*1024 - 1)  /*环形buff的大小*/
#else
#define NF_EXT_RING_BUF_SIZE                        (8*1024 - 1)  /*环形buff的大小*/
#endif

#define NF_ONCE_DEAL_MAX_CNT                        (200)

#define DBG_ON                                      (1)
#define DBG_OFF                                     (0)

#define NF_EXT_DBG                                  DBG_ON

#if (FEATURE_ON == FEATURE_NFEXT)
#define NF_EXT_DEF_BR_ARP_HOOK_ON_MASK              (g_stExHookMask.ulBrArpHookValue)

#define NF_EXT_DEF_PRE_ROUTING_HOOK_ON_MASK         (g_stExHookMask.ulInHookValue)

#define NF_EXT_DEF_POST_ROUTING_HOOK_ON_MASK        (g_stExHookMask.ulOutHookValue)

#define NF_EXT_DEF_LOCAL_HOOK_ON_MASK               (g_stExHookMask.ulLocalHookValue)

/* 网络协议栈流控HOOK MASK开关 */
#define NF_EXT_DEF_FLOW_CTRL_HOOK_ON_MASK           (g_stExHookMask.ulFlowCtrlHookValue)


#define NF_EXT_RPO_TCP                              (0x6)   /*TCP协议类型标志*/
#define MAC_HEADER_LENGTH                           (14)

#define NF_EXT_MAX_IP_SIZE                          (65535)

#define NF_EXT_BR_FORWARD_FLOW_CTRL_MASK            (1U)

#define NF_EXT_BR_NAME                              "br0"

#define NF_EXT_TX_BYTES_INC(a, b)                   (g_stExFlowCtrlEntity.aulTxBytesCnt[b] += a)

#define NF_EXT_GET_MASK_FROM_INDEX(ucIdx)           (1 << (ucIdx))

#endif

/*******************************************************************************
  3 枚举定义
*******************************************************************************/
enum NF_EXT_FLAG_OM_DATA_ENUM
{
    NF_EXT_FLAG_OM_DATA                = 0,                    /* OM消息标志 */
    NF_EXT_FLAG_NOT_OM_DATA            = 1,                    /* 非OM消息标志 */

    NF_EXT_FLAG_OM_DATA_BUTT
};
typedef int NF_EXT_FLAG_OM_DATA_ENUM_U32;

enum NF_EXT_FLAG_BLOCK_ENUM
{
    NF_EXT_FLAG_BLOCKED                = 0,                    /* 阻塞标志 */
    NF_EXT_FLAG_UNBLOCKED              = 1,                    /* 非阻塞标志 */

    NF_EXT_FLAG_BUTT
};

enum NF_EXT_TX_BYTES_CNT_ENUM
{
    NF_EXT_TX_BYTES_CNT_BR             = 0,                    /* 统计类型 */
    NF_EXT_TX_BYTES_CNT_BUTT
};

#if(NF_EXT_DBG == DBG_ON)
enum NF_EXT_STATS_ENUM
{
    NF_EXT_STATS_BR_FC_DROP            = 0,
    NF_EXT_STATS_BR_FC_ENTER,
    NF_EXT_STATS_BUF_FULL_DROP,
    NF_EXT_STATS_PUT_BUF_FAIL,
    NF_EXT_STATS_ALLOC_MEM_FAIL,
    NF_EXT_STATS_GET_BUF_FAIL,

    NF_EXT_STATS_BUT
};
#endif

/*****************************************************************************
 枚举名    : NF_EXT_HOOK_ON_MASK_IDX_ENUM_UINT8
 协议表格  :
 ASN.1描述 :
 枚举说明  : netfilter钩子函数掩码
*****************************************************************************/
enum NF_EXT_HOOK_ON_MASK_IDX_ENUM
{
    NF_EXT_BR_PRE_ROUTING_HOOK_ON_MASK_IDX          = 0x00,
    NF_EXT_BR_POST_ROUTING_HOOK_ON_MASK_IDX         = 0x01,
    NF_EXT_BR_FORWARD_HOOK_ON_MASK_IDX              = 0x02,
    NF_EXT_BR_LOCAL_IN_HOOK_ON_MASK_IDX             = 0x03,
    NF_EXT_BR_LOCAL_OUT_HOOK_ON_MASK_IDX            = 0x04,
    NF_EXT_ARP_LOCAL_IN_ON_MASK_IDX                 = 0x05,
    NF_EXT_ARP_LOCAL_OUT_ON_MASK_IDX                = 0x06,
    NF_EXT_IP4_PRE_ROUTING_HOOK_ON_MASK_IDX         = 0x07,
    NF_EXT_IP4_POST_ROUTING_HOOK_ON_MASK_IDX        = 0x08,
    NF_EXT_IP4_LOCAL_IN_HOOK_ON_MASK_IDX            = 0x09,
    NF_EXT_IP4_LOCAL_OUT_HOOK_ON_MASK_IDX           = 0x0A,
    NF_EXT_IP4_FORWARD_HOOK_ON_MASK_IDX             = 0x0B,
    NF_EXT_IP6_PRE_ROUTING_HOOK_ON_MASK_IDX         = 0x0C,
    NF_EXT_IP6_POST_ROUTING_HOOK_ON_MASK_IDX        = 0x0D,
    NF_EXT_IP6_LOCAL_IN_HOOK_ON_MASK_IDX            = 0x0E,
    NF_EXT_IP6_LOCAL_OUT_HOOK_ON_MASK_IDX           = 0x0F,
    NF_EXT_IP6_FORWARD_HOOK_ON_MASK_IDX             = 0x10,
    NF_EXT_BR_FORWARD_FLOW_CTRL_HOOK_ON_MASK_IDX    = 0x11,
    NF_EXT_HOOK_ON_MASK_IDX_ENUM_BUTT
};
typedef  VOS_UINT8 NF_EXT_HOOK_ON_MASK_IDX_ENUM_UINT8;

/*****************************************************************************
 枚举名    : NF_EXT_HOOK_ON_MASK_PRIORITY_ENUM
 协议表格  :
 ASN.1描述 :
 枚举说明  : netfilter钩子函数掩码优先级(规避MIXED_ENUMS新增)
*****************************************************************************/
enum NF_EXT_HOOK_ON_MASK_PRIORITY_ENUM
{
    NF_EXT_BR_PRI_FILTER_OTHER      = NF_BR_PRI_FILTER_OTHER,
    NF_EXT_BR_PRI_FILTER_BRIDGED    = NF_BR_PRI_FILTER_BRIDGED,
    NF_EXT_IP_PRI_CONNTRACK         = NF_IP_PRI_CONNTRACK,
    NF_EXT_IP_PRI_MANGLE            = NF_IP_PRI_MANGLE,
    NF_EXT_IP_PRI_SELINUX_LAST      = NF_IP_PRI_SELINUX_LAST,

    NF_EXT_HOOK_ON_MASK_PRIORITY_ENUM_BUTT
};


/*****************************************************************************
  4 结构定义
*****************************************************************************/

typedef struct
{
    VOS_UINT32                      ulIsBlkflag;               /* 阻塞条件 */
    VOS_UINT32                      ulCurHookOnMask;           /* 当前Hook掩码 */
    VOS_UINT32                      ulIsDeviceOpen ;           /* 设备是否开启的标志 */
    VOS_UINT32                      ulOmIp;
    OM_RING_ID                      pRingBufferId;             /* 环形buff*/
    VOS_SPINLOCK                    stLockTxTask;              /* 自旋锁，用于环形buff操作的互斥保护 */
    VOS_UINT8                       aucRsv2[4];

    struct cpumask                  orig_mask;
    struct cpumask                  curr_mask;
}NF_EXT_ENTITY_STRU;

typedef struct
{
    VOS_UINT8   *pData;
}NF_EXT_DATA_RING_BUF_STRU;

#if (FEATURE_ON == FEATURE_NFEXT)
/*********************************************
 结构体名 :NF_EXT_MASK_OPS_STRU
 协议表格 :无
 结构体说明 :勾子开关掩码映射表结构体
*********************************************/
typedef struct
{
    u_int32_t                       ulHookMask;
    VOS_UINT8                       aucRsv[4];
    struct nf_hook_ops              stNfExtOps;
} NF_EXT_MASK_OPS_STRU;

typedef struct
{
    VOS_UINT32         ulFlowCtrlMsk;
    VOS_UINT32         aulTxBytesCnt[NF_EXT_TX_BYTES_CNT_BUTT];
    struct net_device *pstBrDev;
}NF_EXT_FLOW_CTRL_ENTITY;

typedef struct
{
    VOS_UINT32          ulBrArpHookValue;       /* 网桥和ARP钩子函数对应的掩码 */
    VOS_UINT32          ulInHookValue;          /* IP层PRE_ROUTING钩子函数对应的掩码 */
    VOS_UINT32          ulOutHookValue;         /* IP层POST_ROUTING钩子函数对应的掩码 */
    VOS_UINT32          ulFlowCtrlHookValue;    /* 网桥流控钩子函数所对应的掩码 */
    VOS_UINT32          ulLocalHookValue;       /* IP层LOCAL钩子函数对应的掩码  */
    VOS_UINT32          ulRsv;
}NF_EXT_HOOK_MASK_NV_STRU;
#endif

#if(NF_EXT_DBG == DBG_ON)
typedef struct
{
    VOS_UINT32  aulStats[NF_EXT_STATS_BUT];
}NF_EXT_STATS_STRU;
#endif

/*****************************************************************************
  5 全局变量声明
*****************************************************************************/
#if(NF_EXT_DBG == DBG_ON)
extern NF_EXT_STATS_STRU g_stNfExtStats;

#define NF_EXT_STATS_INC(a, b)                      (g_stNfExtStats.aulStats[b] += a)
#else
#define NF_EXT_STATS_INC(a, b)                      do{}while(0)
#endif

#define NFEXT_DATA_PROC_NOTIFY                      (0x0001)

/*****************************************************************************
  6 函数声明
*****************************************************************************/
#if (FEATURE_ON == FEATURE_NFEXT)
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0))
extern unsigned int NFExt_BrPreRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_BrPostRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);


extern unsigned int NFExt_BrLocalInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_BrLocalOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_BrForwardHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_ArpInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_ArpOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip4PreRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip4PostRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip4LocalInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip4LocalOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip4ForwardHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip6PreRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip6PostRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip6LocalInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip6LocalOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_Ip6ForwardHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

extern unsigned int NFExt_BrForwardFlowCtrlHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state);

#else
extern unsigned int NFExt_BrPreRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_BrPostRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));


extern unsigned int NFExt_BrLocalInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_BrLocalOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_BrForwardHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_ArpInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_ArpOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip4PreRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip4PostRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip4LocalInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip4LocalOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip4ForwardHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip6PreRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip6PostRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip6LocalInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip6LocalOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_Ip6ForwardHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));

extern unsigned int NFExt_BrForwardFlowCtrlHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *));
#endif

extern NF_EXT_FLAG_OM_DATA_ENUM_U32 NFExt_IsOmData(struct sk_buff *skb);
extern VOS_VOID NFExt_BrDataExport( struct sk_buff *skb,
                                const struct net_device *device_in,
                                const struct net_device *device_out,
                                TTF_MNTN_MSG_TYPE_ENUM_UINT16 enType);
extern VOS_VOID NFExt_ArpDataExport( struct sk_buff *skb,
                                const struct net_device *device,
                                TTF_MNTN_MSG_TYPE_ENUM_UINT16 enType);
extern VOS_VOID NFExt_IpDataExport( struct sk_buff *skb,
                         const struct net_device *device,
                         TTF_MNTN_MSG_TYPE_ENUM_UINT16 enType);

extern VOS_VOID  NFExt_UnregHooks(VOS_UINT32 ulMask);
extern VOS_INT  NFExt_RegHooks(VOS_UINT32 ulMask);
extern VOS_UINT32  NFExt_Get1stInetIpv4Addr(struct net_device *pstDev);
extern PS_BOOL_ENUM_UINT8 NFExt_ConfigEffective(IPS_MNTN_TRACE_CONFIG_REQ_STRU *pRcvMsg);

extern VOS_VOID  NFExt_BrSetFlowCtrl(VOS_VOID);
extern VOS_VOID  NFExt_BrStopFlowCtrl(VOS_VOID);
extern VOS_UINT32 NFExt_GetBrBytesCnt(VOS_VOID);


#endif

extern VOS_UINT32 NFExt_AddDataToRingBuf(NF_EXT_DATA_RING_BUF_STRU *pstMsg);
extern VOS_VOID NFExt_FlushRingBuffer(OM_RING_ID rngId);

extern VOS_VOID NFExt_BindToCpu(VOS_VOID);
extern VOS_VOID NFExt_RcvOmMsg(VOS_VOID *pMsg);
extern VOS_VOID NFExt_ProcDataNotify(VOS_VOID);
extern VOS_VOID NFExt_MsgProc( struct MsgCB * pMsg );
extern VOS_UINT32 NFExt_PidInit( enum VOS_INIT_PHASE_DEFINE ip );
extern VOS_VOID NFExt_FidTask(VOS_VOID);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif
