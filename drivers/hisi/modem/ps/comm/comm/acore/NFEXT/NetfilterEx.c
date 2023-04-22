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
#include "product_config.h"


#include "NetfilterEx.h"

#include "linux/inet.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/

#define THIS_FILE_ID PS_FILE_ID_ACPU_NFEX_C


/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define             PACKAGE_HEAD_LEN        (80)        /*设定的截取包头的长度*/
#define             OM_SOCK_PORT_NUM        (3000)      /*与OM的宏SOCK_PORT_NUM保持一致*/
/*****************************************************************************
  3 外部函数变量声明
*****************************************************************************/

extern NF_EXT_ENTITY_STRU           g_stExEntity;
extern NF_EXT_FLOW_CTRL_ENTITY      g_stExFlowCtrlEntity;

/******************************************************************************
  4 函数实现
******************************************************************************/

NF_EXT_FLAG_OM_DATA_ENUM_U32 NFExt_IsOmData(struct sk_buff *skb)
{
    struct iphdr        *ipHeader;
    __be32               srcIp;
    __be32               destIp;
    struct tcphdr       *tcpHeader;
    __be16               srcPort;
    __be16               destPort;
    __be32               ulOmSocketIp;

    ipHeader        = (struct iphdr *)(skb_network_header(skb));

    /*如果不是TCP报文则直接返回*/
    if ( NF_EXT_RPO_TCP != ipHeader->protocol )
    {
        return NF_EXT_FLAG_NOT_OM_DATA;
    }

    /* 传输层的数据在ip层之后 */
    tcpHeader       = (struct tcphdr *)(skb_network_header(skb) + sizeof(struct iphdr));

    srcIp           = ipHeader->saddr;
    destIp          = ipHeader->daddr;
    srcPort         = ntohs(tcpHeader->source);
    destPort        = ntohs(tcpHeader->dest);
    ulOmSocketIp    = g_stExEntity.ulOmIp;

    if ( ((ulOmSocketIp == srcIp) && (OM_SOCK_PORT_NUM == srcPort))
      || ((ulOmSocketIp == destIp) && (OM_SOCK_PORT_NUM == destPort)) )
    {
        return NF_EXT_FLAG_OM_DATA;
    }

    return NF_EXT_FLAG_NOT_OM_DATA;
}


VOS_VOID NFExt_BrDataExport( struct sk_buff *skb,
                                const struct net_device *device_in,
                                const struct net_device *device_out,
                                TTF_MNTN_MSG_TYPE_ENUM_UINT16 enType)
{
    VOS_UINT8                       *pucData;
    VOS_UINT32                       ulHookDataLen;

    /* skb->data指向数据包的IP头部，上移14个字节令 pucData指向数据包的mac头部 */
    pucData             = skb->data - MAC_HEADER_LENGTH;
    ulHookDataLen       = ((skb->len > NF_EXT_MAX_IP_SIZE) ? NF_EXT_MAX_IP_SIZE : skb->len) + MAC_HEADER_LENGTH;

    IPS_MNTN_BridgePktInfoCB((const VOS_UINT8 *)device_in->name, (const VOS_UINT8 *)device_out->name, pucData, (VOS_UINT16)ulHookDataLen, enType);
}


VOS_VOID NFExt_ArpDataExport( struct sk_buff *skb,
                                    const struct net_device *device,
                                    TTF_MNTN_MSG_TYPE_ENUM_UINT16 enType)
{
    VOS_UINT8                       *pucData;
    VOS_UINT32                       ulHookDataLen;

    pucData             = skb->data;
    ulHookDataLen       = skb->len;

    IPS_MNTN_CtrlPktInfoCB((const VOS_UINT8 *)device->name, (const VOS_UINT8 *)pucData, (VOS_UINT16)ulHookDataLen, enType);
}


VOS_VOID NFExt_IpDataExport( struct sk_buff *skb,
                         const struct net_device *device,
                         TTF_MNTN_MSG_TYPE_ENUM_UINT16 enType)
{
    VOS_UINT8                       *pucData;
    VOS_UINT32                       ulHookDataLen;

    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return;
    }

    /* skb->data指向数据包的IP头部，上移14个字节令 pucData指向数据包的mac头部 */
    pucData           = skb->data - MAC_HEADER_LENGTH;
    ulHookDataLen     = ((skb->len > NF_EXT_MAX_IP_SIZE) ? NF_EXT_MAX_IP_SIZE : skb->len) + MAC_HEADER_LENGTH;

    IPS_MNTN_PktInfoCB((const VOS_UINT8 *)device->name, (const VOS_UINT8 *)pucData, (VOS_UINT16)ulHookDataLen, enType);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 1, 0))


unsigned int NFExt_BrPreRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    /* 判断是否OM的数据 */
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, in, out, ID_IPS_TRACE_BRIDGE_PRE_ROUTING_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrPostRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, in, out, ID_IPS_TRACE_BRIDGE_POST_ROUTING_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrLocalInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, in, out, ID_IPS_TRACE_BRIDGE_LOCAL_IN_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrLocalOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, in, out, ID_IPS_TRACE_BRIDGE_LOCAL_OUT_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrForwardHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_BrDataExport(skb, in, out, ID_IPS_TRACE_BRIDGE_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_ArpInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_ArpDataExport(skb, in, ID_IPS_TRACE_RECV_ARP_PKT);

    return NF_ACCEPT;
}



unsigned int NFExt_ArpOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{

    NFExt_ArpDataExport(skb, out, ID_IPS_TRACE_SEND_ARP_PKT);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4PreRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, in, ID_IPS_TRACE_INPUT_DATA_INFO);

    return NF_ACCEPT;
}



unsigned int NFExt_Ip4PostRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, out, ID_IPS_TRACE_OUTPUT_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4LocalInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{

    NFExt_IpDataExport(skb, in, ID_IPS_TRACE_IP4_LOCAL_IN_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4LocalOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, out, ID_IPS_TRACE_IP4_LOCAL_OUT_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4ForwardHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, in, ID_IPS_TRACE_IP4_FORWARD_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6PreRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, in, ID_IPS_TRACE_INPUT_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6PostRoutingHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, out, ID_IPS_TRACE_OUTPUT_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6LocalInHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, in, ID_IPS_TRACE_IP6_LOCAL_IN_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6LocalOutHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, out, ID_IPS_TRACE_IP6_LOCAL_OUT_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6ForwardHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NFExt_IpDataExport(skb, in, ID_IPS_TRACE_IP6_FORWARD_INFO);

    return NF_ACCEPT;
}

/*****************************************************************************
                        流控功能
*****************************************************************************/



unsigned int NFExt_BrForwardFlowCtrlHook(unsigned int hooknum,
                            struct sk_buff *skb,
                            const struct net_device *in,
                            const struct net_device *out,
                            int (*okfn)(struct sk_buff *))
{
    NF_EXT_STATS_INC(1, NF_EXT_STATS_BR_FC_ENTER);

    /* 网桥转发统计 */
    NF_EXT_TX_BYTES_INC(skb->len, NF_EXT_TX_BYTES_CNT_BR);


    /* 当前在网桥forward流控状态，直接丢包 */
    if (NF_EXT_BR_FORWARD_FLOW_CTRL_MASK == (g_stExFlowCtrlEntity.ulFlowCtrlMsk & NF_EXT_BR_FORWARD_FLOW_CTRL_MASK))
    {
        NF_EXT_STATS_INC(1, NF_EXT_STATS_BR_FC_DROP);
        return NF_DROP;
    }

    return NF_ACCEPT;
}

#else

unsigned int NFExt_BrPreRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    /* 判断是否OM的数据 */
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, state->in, state->out, ID_IPS_TRACE_BRIDGE_PRE_ROUTING_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrPostRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, state->in, state->out, ID_IPS_TRACE_BRIDGE_POST_ROUTING_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrLocalInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, state->in, state->out, ID_IPS_TRACE_BRIDGE_LOCAL_IN_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrLocalOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    if ( NF_EXT_FLAG_OM_DATA == NFExt_IsOmData(skb) )
    {
        return NF_ACCEPT;
    }

    NFExt_BrDataExport(skb, state->in, state->out, ID_IPS_TRACE_BRIDGE_LOCAL_OUT_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_BrForwardHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_BrDataExport(skb, state->in, state->out, ID_IPS_TRACE_BRIDGE_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_ArpInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_ArpDataExport(skb, state->in, ID_IPS_TRACE_RECV_ARP_PKT);

    return NF_ACCEPT;
}



unsigned int NFExt_ArpOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{

    NFExt_ArpDataExport(skb, state->out, ID_IPS_TRACE_SEND_ARP_PKT);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4PreRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->in, ID_IPS_TRACE_INPUT_DATA_INFO);

    return NF_ACCEPT;
}



unsigned int NFExt_Ip4PostRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->out, ID_IPS_TRACE_OUTPUT_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4LocalInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{

    NFExt_IpDataExport(skb, state->in, ID_IPS_TRACE_IP4_LOCAL_IN_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4LocalOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->out, ID_IPS_TRACE_IP4_LOCAL_OUT_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip4ForwardHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->in, ID_IPS_TRACE_IP4_FORWARD_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6PreRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->in, ID_IPS_TRACE_INPUT_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6PostRoutingHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->out, ID_IPS_TRACE_OUTPUT_DATA_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6LocalInHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->in, ID_IPS_TRACE_IP6_LOCAL_IN_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6LocalOutHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->out, ID_IPS_TRACE_IP6_LOCAL_OUT_INFO);

    return NF_ACCEPT;
}


unsigned int NFExt_Ip6ForwardHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NFExt_IpDataExport(skb, state->in, ID_IPS_TRACE_IP6_FORWARD_INFO);

    return NF_ACCEPT;
}

/*****************************************************************************
                        流控功能
*****************************************************************************/

unsigned int NFExt_BrForwardFlowCtrlHook(const struct nf_hook_ops *ops,
                                    struct sk_buff *skb,
                                    const struct nf_hook_state *state)
{
    NF_EXT_STATS_INC(1, NF_EXT_STATS_BR_FC_ENTER);

    /* 网桥转发统计 */
    NF_EXT_TX_BYTES_INC(skb->len, NF_EXT_TX_BYTES_CNT_BR);


    /* 当前在网桥forward流控状态，直接丢包 */
    if (NF_EXT_BR_FORWARD_FLOW_CTRL_MASK == (g_stExFlowCtrlEntity.ulFlowCtrlMsk & NF_EXT_BR_FORWARD_FLOW_CTRL_MASK))
    {
        NF_EXT_STATS_INC(1, NF_EXT_STATS_BR_FC_DROP);
        return NF_DROP;
    }

    return NF_ACCEPT;
}

#endif      /* #if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 1, 0)) */

