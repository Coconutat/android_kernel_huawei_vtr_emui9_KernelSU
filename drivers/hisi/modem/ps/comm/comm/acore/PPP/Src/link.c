/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1998 Brian Somers <brian@Awfulhak.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD: releng/11.2/usr.sbin/ppp/link.c 330449 2018-03-05 07:26:05Z eadler $
 *
 */

/******************************************************************************
   头文件包含
******************************************************************************/
#include "PPP/Inc/ppp_public.h"
#include "PPP/Inc/layer.h"
#include "PPP/Inc/ppp_mbuf.h"
#include "PPP/Inc/hdlc.h"
#include "PPP/Inc/acf.h"
#include "PPP/Inc/throughput.h"
#include "PPP/Inc/proto.h"
#include "PPP/Inc/ppp_fsm.h"
#include "PPP/Inc/lcp.h"
#include "PPP/Inc/async.h"
#include "PPP/Inc/auth.h"
#include "PPP/Inc/ipcp.h"
#include "PPP/Inc/pppid.h"
#include "PPP/Inc/link.h"
#include "PPP/Inc/pap.h"
#include "PPP/Inc/ppp_init.h"
#include "PPP/Inc/ppp_input.h"
#include "AdsDeviceInterface.h"


/*****************************************************************************
   1 协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
/*lint -e767  原因简述: 打点日志文件宏ID定义 */
#define    THIS_FILE_ID        PS_FILE_ID_LINK_C
/*lint +e767 */


/******************************************************************************
   2 外部函数变量声明
******************************************************************************/
extern PPP_ZC_STRU *ipv4_Input(struct link *l, PPP_ZC_STRU *bp);

void ParentLayerStart (void *p, struct fsm *fsm);
void ParentLayerUp (void *p, struct fsm *fsm);
void ParentLayerDown (void *p, struct fsm *fsm);
void ParentLayerFinish (void *p, struct fsm *fsm);

/*****************************************************************************
   3 私有定义
*****************************************************************************/
#define PROTO_IN  1                       /* third arg to link_ProtocolRecord */
#define PROTO_OUT 2


/*****************************************************************************
   4 全局变量定义
*****************************************************************************/
struct link*            pgPppLink = VOS_NULL_PTR;

PPP_HDLC_CONFIG_STRU    g_astHdlcConfig[PPP_MAX_ID_NUM] = {
    [0] = {
            .pFunProcData = VOS_NULL_PTR,
            .pFunProcProtocolPacket = VOS_NULL_PTR,
            .pFunDisable = VOS_NULL_PTR,
            .pFunProcAsFrmData = VOS_NULL_PTR
    }
};

PPP_HDLC_CONFIG_STRU    *g_pstHdlcConfig = &g_astHdlcConfig[0];

/*lint -e958 -e830 -e528*/

static const struct {
  VOS_UINT16 proto;
  PPP_ZC_STRU *(*fn)(/*struct bundle *, */struct link *, PPP_ZC_STRU *);
} despatcher[] = {
  { PROTO_IP, ipv4_Input },
#ifndef NI_WITHSCOPEID
#define NI_WITHSCOPEID 0
#endif
  /*{ PROTO_IPV6,ipv6_Input },*/
  /*{ PROTO_MP, mp_Input },*/
  { PROTO_LCP,lcp_Input},
  { PROTO_IPCP, ipcp_Input},
#ifndef NI_WITHSCOPEID
#define NI_WITHSCOPEID 0
#endif
  /*{ PROTO_IPV6CP, ipv6cp_Input },*/
  { PROTO_PAP, pap_Input},
  { PROTO_CHAP, chap_Input }
  /*{ PROTO_CCP, ccp_Input },*/
  /*{ PROTO_LQR, lqr_Input },*/
  /*{ PROTO_CBCP, cbcp_Input }*/
};
/*lint +e958 +e830 -e528*/

#define DSIZE (sizeof despatcher / sizeof despatcher[0])

struct  fsm_parent  parent = {
             ParentLayerStart,
             ParentLayerUp,
             ParentLayerDown,
             ParentLayerFinish,
             VOS_NULL
             };

/******************************************************************************
   5 函数实现
******************************************************************************/
void link_SequenceQueue(struct link *l)
{
  struct ppp_mqueue *queue, *highest;

  PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "link_SequenceQueue\r\n");

  highest = LINK_HIGHQ(l);
  for (queue = l->Queue; queue < highest; queue++)
  {
    while (queue->len)
    {
        ppp_m_enqueue(highest, ppp_m_dequeue(queue));
    }
  }
}


VOS_INT32 link_Stack(struct link *l, struct layer *layer)
{
  if (l->nlayers == sizeof l->layer / sizeof l->layer[0]) {
    PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
        "Oops, cannot stack a layer\r\n");
    return 0;
  }
  l->layer[l->nlayers++] = layer;
  return 1;
}

void link_EmptyStack(struct link *l)
{
  l->nlayers = 0;
}

/*****************************************************************************
 Prototype      : ipv4_Input
 Description    : 对于TE发送来的数据报文，如果对应的link中是处于网络阶段并且状
                  态为open，转发此报文到GGSN。

 Input          : ---
 Output         : ---返回指向mbuf的指针
 Return Value   : ---
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-11-18
    Author      : ---
    Modification: Created function
*****************************************************************************/
PPP_ZC_STRU *ipv4_Input(/*struct bundle *bundle, */struct link *l, PPP_ZC_STRU *bp)
{
    if(l->phase == PHASE_NETWORK
        &&l->ipcp.fsm.state == ST_OPENED)
    {
        /*将上行数据发往协议栈*/
        PPP_SendPulledData((VOS_UINT16)PPP_LINK_TO_ID(l), bp);
    }
    else
    {
        PPP_MemFree(bp);
    }

    return VOS_NULL_PTR;
}

void ParentLayerStart (void *p, struct fsm *fsm)         /* tls */
{
    return;
}

void ParentLayerUp (void *p, struct fsm *fsm)            /* tlu */
{
    return;
}

void ParentLayerDown (void *p, struct fsm *fsm)          /* tld */
{
    return;
}

void ParentLayerFinish (void *p, struct fsm *fsm)        /* tlf */
{
    return;
}


VOS_VOID PPP_InitSecureData(VOS_UINT8 ucPppId)
{
    struct link                        *pstPppLink;


    if ((0 == ucPppId) || (ucPppId > PPP_MAX_ID_NUM))
    {
        return;
    }

    pstPppLink = PPP_LINK(ucPppId);

    /* 清除chap鉴权保存的信息 */
    PSACORE_MEM_SET(&(pstPppLink->chap.challenge), (VOS_SIZE_T)sizeof(pstPppLink->chap.challenge),
        0, (VOS_SIZE_T)sizeof(pstPppLink->chap.challenge));
    PSACORE_MEM_SET(&(pstPppLink->chap.RecordData), (VOS_SIZE_T)sizeof(pstPppLink->chap.RecordData),
        0, (VOS_SIZE_T)sizeof(pstPppLink->chap.RecordData));

    /* 清除pap鉴权保存的信息 */
    PSACORE_MEM_SET(&(pstPppLink->pap.RecordData), (VOS_SIZE_T)sizeof(pstPppLink->pap.RecordData),
        0, (VOS_SIZE_T)sizeof(pstPppLink->pap.RecordData));

    /* 清除鉴权保存的用户名密码信息 */
    PSACORE_MEM_SET(&(pstPppLink->auth), (VOS_SIZE_T)sizeof(pstPppLink->auth),
        0, (VOS_SIZE_T)sizeof(pstPppLink->auth));

    return;
}

VOS_VOID link_Init(struct link *l)
{
    l->phase    = PHASE_DEAD;
    l->type     = PHYSICAL_LINK;
    l->name     = "Ppp";
    l->len      = sizeof(*l);

    /* The sample period is fixed - see physical2iov() & iov2physical() */
    throughput_init(&l->stats.total, SAMPLE_PERIOD);

    l->stats.gather = 1;
    l->DropedPacketFromGgsn = 0;

    PSACORE_MEM_SET(l->Queue, sizeof l->Queue, '\0', sizeof l->Queue);
    PSACORE_MEM_SET(l->proto_in, sizeof l->proto_in, '\0', sizeof l->proto_in);
    PSACORE_MEM_SET(l->proto_out, sizeof l->proto_out, '\0', sizeof l->proto_out);
    link_EmptyStack(l);

    /*依次压入PPP协议的各个处理层*/
    link_Stack(l, &asynclayer);
    link_Stack(l, &hdlclayer);
    link_Stack(l, &acflayer);

    link_Stack(l, &protolayer);

    PSACORE_MEM_SET(l->auth.name, sizeof(l->auth.name), '\0', sizeof(l->auth.name));
    PSACORE_MEM_SET(l->auth.key, sizeof(l->auth.key), '\0', sizeof(l->auth.key));

    async_Init(&(l->async));
    hdlc_Init(&(l->hdlc),&(l->lcp));

    /*参数const struct fsm_parent * parent不能为空*/
    lcp_Init(&(l->lcp), l, &parent);
    ipcp_Init(&(l->ipcp), l, &parent);
    pap_Init(&(l->pap));
    chap_Init(&(l->chap));
}

void link_PushPacket(struct link *l, struct ppp_mbuf *bp, VOS_INT32 pri, VOS_UINT16 proto)
{
    PPP_ID                  usPppId     = (PPP_ID)(PPP_LINK_TO_ID(l));
    PPP_HDLC_CONFIG_STRU   *pstHdlcConfig;

    /* 把PPP协商包作为可维可测信息,IP包不做可维可测维护*/
    if (PROTO_IP != proto)
    {
        Ppp_MBufFrameMntnInfo(bp, proto, PPP_SEND_OUT_PROTOCOL_FRAME);
    }

    pstHdlcConfig = PPP_CONFIG(usPppId);

    if (VOS_NULL_PTR != pstHdlcConfig->pFunProcProtocolPacket)
    {
        pstHdlcConfig->pFunProcProtocolPacket(l, bp, pri, proto);
    }
    else
    {
        ppp_m_freem(bp);
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING,
                     "PPP, link_PushPacket, WARNING, pFunProcProtocolPacket is NULL!\r\n");
    }

    return;
}


VOS_VOID PPP_HDLC_ProcIpModeUlData
(
    struct link *pstLink,
    PPP_ZC_STRU *pstMem,
    VOS_UINT16  usProto
)
{
    VOS_UINT32          f;
    struct ppp_mbuf    *bp;

    /* 把PPP协商包作为可维可测信息,IP包不做可维可测维护*/
    if (PROTO_IP != usProto)
    {
        Ppp_TtfMemFrameMntnInfo(pstMem, usProto, PPP_RECV_IN_PROTOCOL_FRAME);
    }

    for (f = 0; f < DSIZE; f++)
    {
        if (despatcher[f].proto == usProto)
        {
            pstMem = (*despatcher[f].fn)(pstLink, pstMem);
            break;
        }
    }

    /* 如果是不支持的协议，向对端发送REJ帧 */
    if (VOS_NULL_PTR != pstMem)
    {
        bp = ppp_m_get_from_ttfmem(pstMem);

        PPP_MemFree(pstMem);

        if (VOS_NULL_PTR == bp)
        {
            return;
        }

        /*    struct physical *p = link2physical(l);

        log_Printf(LogPHASE, "%s protocol 0x%04x (%s)\n",
        f == DSIZE ? "Unknown" : "Unexpected", proto,
        hdlc_Protocol2Nam(proto));*/

        bp = ppp_m_pullup(proto_Prepend(bp, usProto, 0, 0));

        if (VOS_NULL_PTR == bp)
        {
            return;
        }
        lcp_SendProtoRej(&pstLink->lcp, PPP_MBUF_CTOP(bp), bp->m_len);
        if (pstLink)
        {
            pstLink->hdlc.lqm.ifInDiscards++;
            pstLink->hdlc.stats.unknownproto++;
        }
        ppp_m_freem(bp);
    }

    return;
}


VOS_VOID PPP_HDLC_ProcPppModeUlData
(
    PPP_ID      usPppId,
    PPP_ZC_STRU *pstMem
)
{
    PPP_SendPulledData(usPppId, pstMem);
    return;
}


VOS_VOID PPP_HDLC_ProcDlData(VOS_UINT16 usPppId, PPP_ZC_STRU *pstMem)
{
    /* 调用AT模块下行数据接收接口 */
    AT_SendZcDataToModem(usPppId, pstMem);

    return;
}



VOS_UINT32 PPP_SendPulledData(VOS_UINT16 usPppId,  PPP_ZC_STRU *pstImmZc)
{
    VOS_UINT8                          ucRabId = PPP_INVALID_RABID;
    VOS_UINT32                         ulResult;


    /* 通过usPppId，寻找到usRabId */
    if ( !PPP_PPPID_TO_RAB(usPppId, &ucRabId) )
    {
        g_PppDataQCtrl.stStat.ulUplinkDropCnt++;
        PPP_MemFree(pstImmZc);
        PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,
                      "PPP, PPP_PushPacketEvent, WARNING, Can not get PPP Id %d, RabId %d",
                      usPppId, ucRabId);

        return PS_FAIL;
    }


    /* 数据发送给ADS，如果失败则释放内存 */
    ulResult = ADS_UL_SendPacket(pstImmZc, ucRabId);

    if ( VOS_OK != ulResult )
    {
        g_PppDataQCtrl.stStat.ulUplinkDropCnt++;
        PPP_MemFree(pstImmZc);

        return PS_FAIL;
    }

    g_PppDataQCtrl.stStat.ulUplinkSndDataCnt++;

    return PS_SUCC;
}

/*lint -e{429}*/

VOS_UINT32 PPP_SendPushedData(VOS_UINT16 usPppId, VOS_UINT8 *pucDataBuf, VOS_UINT16 usLen)
{
    PPP_ZC_STRU                        *pstMem;
    VOS_UINT16                          usRemainLen = usLen;
    VOS_UINT8                          *pucRemainDataBuf = pucDataBuf;


    while ( 0 < usRemainLen)
    {
        /* 零拷贝内存有最大限制，超限分多次发送 */
        if ( PPP_ZC_MAX_DATA_LEN < usRemainLen)
        {
            pstMem       = PPP_MemCopyAlloc(pucRemainDataBuf, PPP_ZC_MAX_DATA_LEN, PPP_ZC_DL_RESERVE_LEN);
            usRemainLen -= PPP_ZC_MAX_DATA_LEN;
            pucRemainDataBuf += PPP_ZC_MAX_DATA_LEN;
        }
        else
        {
            pstMem       = PPP_MemCopyAlloc(pucRemainDataBuf, usRemainLen, PPP_ZC_DL_RESERVE_LEN);
            usRemainLen  = 0;
        }

        if ( VOS_NULL_PTR == pstMem )
        {
            return PS_FAIL;
        }

        PPP_HDLC_ProcDlData(usPppId, pstMem);

        g_PppDataQCtrl.stStat.ulDownlinkSndDataCnt++;
    }

    return PS_SUCC;
}




