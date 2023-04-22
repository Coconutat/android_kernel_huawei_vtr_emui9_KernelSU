/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1996 - 2001 Brian Somers <brian@Awfulhak.org>
 *          based on work by Toshiharu OHNO <tony-o@iij.ad.jp>
 *                           Internet Initiative Japan, Inc (IIJ)
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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/ipcp.c 330449 2018-03-05 07:26:05Z eadler $
 */


#include "PPP/Inc/ppp_public.h"
#include "PPP/Inc/layer.h"
#include "PPP/Inc/ppp_mbuf.h"
#include "PPP/Inc/hdlc.h"
#include "PPP/Inc/throughput.h"
#include "PPP/Inc/proto.h"
#include "PPP/Inc/ppp_fsm.h"
#include "PPP/Inc/lcp.h"
#include "PPP/Inc/async.h"
#include "PPP/Inc/auth.h"
#include "PPP/Inc/ipcp.h"
#include "PPP/Inc/pppid.h"
#include "PPP/Inc/link.h"
#include "PPP/Inc/ppp_input.h"
#include "TafNvInterface.h"

#undef REJECTED

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_IPCP_C


#define    REJECTED(p, x)    ((p)->peer_reject & (1<<(x)))
#define issep(ch) ((ch) == ' ' || (ch) == '\t')
#define isip(ch) (((ch) >= '0' && (ch) <= '9') || (ch) == '.')

extern VOS_UINT8  g_ucPppConfigWins;

struct compreq {
  VOS_UINT16 proto;
  VOS_CHAR slots;
  VOS_CHAR compcid;
};

VOS_INT32 IpcpLayerUp(struct fsm *);
void IpcpLayerDown(struct fsm *);
void IpcpLayerStart(struct fsm *);
void IpcpLayerFinish(struct fsm *);
void IpcpInitRestartCounter(struct fsm *, VOS_INT32);
void IpcpSendConfigReq(struct fsm *);
void IpcpSentTerminateReq(struct fsm *);
void IpcpSendTerminateAck(struct fsm *, VOS_CHAR);
void IpcpDecodeConfig(struct fsm *, VOS_CHAR *, VOS_CHAR *, VOS_INT32,
                             struct fsm_decode *);

static struct fsm_callbacks ipcp_Callbacks = {
    .LayerUp = IpcpLayerUp,
    .LayerDown = IpcpLayerDown,
    .LayerStart = IpcpLayerStart,
    .LayerFinish = IpcpLayerFinish,
    .InitRestartCounter = IpcpInitRestartCounter,
    .SendConfigReq = IpcpSendConfigReq,
    .SentTerminateReq = IpcpSentTerminateReq,
    .SendTerminateAck = IpcpSendTerminateAck,
    .DecodeConfig = IpcpDecodeConfig,
    .RecvResetReq = fsm_NullRecvResetReq,
    .RecvResetAck = fsm_NullRecvResetAck
};

const VOS_CHAR *
protoname(VOS_INT32 proto)
{
  static struct {
    VOS_INT32 id;
    const VOS_CHAR *txt;
  } cftypes[] = {
    /* Check out the latest ``Assigned numbers'' rfc (rfc1700.txt) */
    { 1, "IPADDRS" },        /* IP-Addresses */    /* deprecated */
    { 2, "COMPPROTO" },        /* IP-Compression-Protocol */
    { 3, "IPADDR" },        /* IP-Address */
    { 129, "PRIDNS" },        /* 129: Primary DNS Server Address */
    { 130, "PRINBNS" },        /* 130: Primary NBNS Server Address */
    { 131, "SECDNS" },        /* 131: Secondary DNS Server Address */
    { 132, "SECNBNS" }        /* 132: Secondary NBNS Server Address */
  };

  VOS_UINT32 f;

  for (f = 0; f < sizeof cftypes / sizeof *cftypes; f++)
    if (cftypes[f].id == proto)
      return cftypes[f].txt;

  return "unknow protocol";
}

void
ipcp_AddInOctets(struct ipcp *ipcp, VOS_INT32 n)
{
  throughput_addin(&ipcp->throughput, n);
}

void
ipcp_AddOutOctets(struct ipcp *ipcp, VOS_INT32 n)
{
  throughput_addout(&ipcp->throughput, n);
}

void
ipcp_Init(struct ipcp *ipcp, struct link *l,
          const struct fsm_parent *parents)
{
  static const VOS_CHAR * const timer_names[] =
    {"IPCP restart", "IPCP openmode", "IPCP stopped"};

  fsm_Init(&ipcp->fsm, "IPCP", PROTO_IPCP, 1, IPCP_MAXCODE, l, parents, &ipcp_Callbacks, timer_names);

  ipcp->cfg.HaveTriggerAddress = 0;
  ipcp->cfg.ns.dns[0].s_addr = INADDR_NONE;
  ipcp->cfg.ns.dns[1].s_addr = INADDR_NONE;
  ipcp->cfg.ns.dns_neg = 0;
  ipcp->cfg.ns.nbns[0].s_addr = INADDR_ANY;
  ipcp->cfg.ns.nbns[1].s_addr = INADDR_ANY;

  ipcp->cfg.fsm.timeout = DEF_FSMRETRY;
  ipcp->cfg.fsm.maxreq = DEF_FSMTRIES;
  ipcp->cfg.fsm.maxtrm = DEF_FSMTRIES;

  ipcp->hIpcpPendTimer = VOS_NULL_PTR;
  ipcp->pstIpcpPendFrame = VOS_NULL_PTR;

  throughput_init(&ipcp->throughput, SAMPLE_PERIOD);
  PSACORE_MEM_SET(ipcp->Queue, sizeof ipcp->Queue, '\0', sizeof ipcp->Queue);
  ipcp_Setup(ipcp, INADDR_NONE);
}

void
ipcp_Destroy(struct ipcp *ipcp)
{
  throughput_destroy(&ipcp->throughput);
}

void
ipcp_SetLink(struct ipcp *ipcp, struct link *l)
{
  ipcp->fsm.link = l;
}

void
ipcp_Setup(struct ipcp *ipcp, VOS_UINT32 mask)
{
  ipcp->fsm.open_mode = 0;

  ipcp->stage = IPCP_NOT_RECEIVE_REQ;
  ipcp->IpAddr_neg = 0;

  ipcp->PriDns_neg = 0;
  ipcp->SecDns_neg = 0;

  ipcp->PriNbns_neg = 0;
  ipcp->SecNbns_neg = 0;
  ipcp->IpAddrs_neg = 0;
  ipcp->CompressProto_neg = 0;

  ipcp->heis1172 = 0;
  ipcp->peer_req = 0;
  ipcp->peer_compproto = 0;

  ipcp->peer_reject = 0;
  ipcp->my_reject = 0;

  /* Copy startup values into ipcp->ns.dns */
  if (ipcp->cfg.ns.dns[0].s_addr != INADDR_NONE)
    PSACORE_MEM_CPY(ipcp->ns.dns, sizeof ipcp->ns.dns, ipcp->cfg.ns.dns, sizeof ipcp->ns.dns);
}

void
IpcpInitRestartCounter(struct fsm *fp, VOS_INT32 what)
{
  /* Set fsm timer load */
  struct ipcp *ipcp;

  if (VOS_NULL_PTR == fp)
  {
      return;
  }


  ipcp = fsm2ipcp(fp);

  switch (what) {
    case FSM_REQ_TIMER:
      if (VOS_NULL_PTR != ipcp)
      {
          fp->restart = ipcp->cfg.fsm.maxreq; /* [false alarm]:移植开源代码 */
      }
      break;
    case FSM_TRM_TIMER:
      if (VOS_NULL_PTR != ipcp)
      {
          fp->restart = ipcp->cfg.fsm.maxtrm; /* [false alarm]:移植开源代码 */
      }
      break;
    default:
      fp->restart = 1;
      break;
  }

  if( VOS_NULL_PTR !=(fp->timer) )
  {
      VOS_StopRelTimer(&(fp->timer));
  }

  if (VOS_OK !=
      VOS_StartRelTimer(&(fp->timer),PS_PID_APP_PPP,PPP_FSM_TIME_INTERVAL,
      TIMER_PPP_PHASE_MSG, (VOS_UINT32)PPP_LINK_TO_ID(fp->link), VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_0))
  {
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,"start reltimer error\r\n");
  }
}

void
IpcpSendConfigReq(struct fsm *fp)
{
  /*因为在3G文档里，不需要向对方发送config帧*/
  VOS_CHAR buff[MAX_FSM_OPT_LEN];

  fsm_Output(fp, CODE_CONFIGREQ, fp->reqid, buff, buff - buff,
             MB_IPCPOUT);
}

void
IpcpSentTerminateReq(struct fsm *fp)
{
  /* Term REQ just sent by FSM */
}

void
IpcpSendTerminateAck(struct fsm *fp, VOS_CHAR id)
{
  /* Send Term ACK please */
  fsm_Output(fp, CODE_TERMACK, id, VOS_NULL_PTR, 0, MB_IPCPOUT);
}

void
IpcpLayerStart(struct fsm *fp)
{
  /* We're about to start up ! */
  struct ipcp *ipcp;

  if (VOS_NULL_PTR == fp)
  {
      return;
  }

  ipcp = fsm2ipcp(fp);

  if (VOS_NULL_PTR == ipcp)
  {
      return;
  }

  PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp LayerStart");
  throughput_start(&ipcp->throughput, "IPCP throughput", /* [false alarm]:移植开源代码 */
                   /*Enabled(fp->bundle, OPT_THROUGHPUT)*/1);
  fp->more.reqs = fp->more.naks = fp->more.rejs = ipcp->cfg.fsm.maxreq * 3;
  ipcp->peer_req = 0;
}

void
IpcpLayerFinish(struct fsm *fp)
{
  /* We're now down */
  struct ipcp *ipcp = fsm2ipcp(fp);

  if (VOS_NULL_PTR == ipcp)
  {
      return;
  }

  PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp LayerFinish");
  throughput_stop(&ipcp->throughput); /* [false alarm]:移植开源代码 */
}
void
IpcpLayerDown(struct fsm *fp)
{
}

VOS_INT32
IpcpLayerUp(struct fsm *fp)
{
  /* We're now up */
  struct ipcp *ipcp;

  if (VOS_NULL_PTR == fp)
  {
      return VOS_ERR;
  }

  ipcp = fsm2ipcp(fp);

  if (VOS_NULL_PTR == ipcp)
  {
      return VOS_ERR;
  }

  fp->more.reqs = fp->more.naks = fp->more.rejs = ipcp->cfg.fsm.maxreq * 3; /* [false alarm]:移植开源代码 */
  return 1;
}

/*****************************************************************************
 Prototype      : StringCompare
 Description    : 对于两个字符串比较他们从头起的一段。

 Input          : ---两个字符串的首地址与比较的长度
 Output         : ---
 Return Value   : ---相等返回VOS_OK，否则返回VOS_ERR
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-12-31
    Author      : ---
    Modification: Created function
*****************************************************************************/
VOS_UINT32 StringCompare(VOS_CHAR* pString1,VOS_VOID* pString2,VOS_UINT16 len)
{
    VOS_INT32 i;
    VOS_CHAR* pString3 = (VOS_CHAR*)pString2;

    for(i=0;i<len;i++)
    {
            if(*(pString1+i) != *(pString3+i))
            return VOS_ERR;
    }
    return VOS_OK;
}

/*****************************************************************************
 Prototype      : DecodeAtIndication
 Description    : 解析从AT接收到的ACK与NAK报文。

 Input          : ---pIpcp:指向该报文所在的ipcp结构
                  ---pEchoBuffer:指向接收到的来自AT的ACK或NAK报文的首地址
                  ---BufferLen:指向接收到的来自AT的ACK或NAK报文的长度
 Output         : ---
 Return Value   : ---成功返回VOS_OK，否则返回VOS_ERR
 Calls          : ---
 Called By      : ---

 History        : ---
  1.Date        : 2005-12-31
    Author      : ---
    Modification: Created function
*****************************************************************************/
/*lint -e{64}  type mismatch*/
VOS_UINT32
DecodeAtIndication(struct ipcp* pIpcp,VOS_CHAR* pEchoBuffer,VOS_UINT16 BufferLen)
{
    struct fsm_opt *opt;
    VOS_UINT16 Len = 0;

    PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "decode at indication\r\n");

    while(Len < BufferLen)
    {
        opt = (struct fsm_opt *)(pEchoBuffer + Len);
        if (opt->hdr.len < sizeof(struct fsm_opt_hdr))
            {
                PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                              "Bad option length = %f\r\n", opt->hdr.len);
                return VOS_ERR;
            }

        switch (opt->hdr.id)
            {
                case TY_IPADDR:
                    PSACORE_MEM_CPY(&(pIpcp->peer_ip.s_addr), (opt->hdr.len-2), opt->data, (opt->hdr.len-2));
                    pIpcp->IpAddr_neg |= NEG_ACCEPTED;
                    break;

                case TY_COMPPROTO:
                    PSACORE_MEM_CPY(pIpcp->CompressProto, (opt->hdr.len-2), opt->data, (opt->hdr.len-2));
                    pIpcp->ComressProtoLen = (opt->hdr.len-2);
                    pIpcp->CompressProto_neg|= NEG_ACCEPTED;
                    break;

                case TY_IPADDRS:
                    break;

                case TY_PRIMARY_NBNS:
                    if (WINS_CONFIG_ENABLE == g_ucPppConfigWins)
                    {
                        PSACORE_MEM_CPY(&(pIpcp->PriNbnsAddr.s_addr), (opt->hdr.len-2), opt->data, (opt->hdr.len-2));
                        pIpcp->PriNbns_neg |= NEG_ACCEPTED;
                    }
                    break;

                case TY_SECONDARY_NBNS:
                    if (WINS_CONFIG_ENABLE == g_ucPppConfigWins)
                    {
                        PSACORE_MEM_CPY(&(pIpcp->SecNbnsAddr.s_addr), (opt->hdr.len-2), opt->data, (opt->hdr.len-2));
                        pIpcp->SecNbns_neg |= NEG_ACCEPTED;
                    }
                    break;

                case TY_PRIMARY_DNS:
                    PSACORE_MEM_CPY(&(pIpcp->PriDnsAddr.s_addr), (opt->hdr.len-2), opt->data, (opt->hdr.len-2));
                    pIpcp->PriDns_neg |= NEG_ACCEPTED;
                    break;

                case TY_SECONDARY_DNS:
                    PSACORE_MEM_CPY(&(pIpcp->SecDnsAddr.s_addr), (opt->hdr.len-2), opt->data, (opt->hdr.len-2));
                    pIpcp->SecDns_neg |= NEG_ACCEPTED;
                    break;

                default:
                    break;
            }
        Len += (VOS_UINT16)(opt->hdr.len);
    }

    return VOS_OK;
}

/*lint -e{64}  type mismatch*/
void
IpcpDecodeConfig(struct fsm *fp, VOS_CHAR *cp, VOS_CHAR *end, VOS_INT32 mode_type,
                 struct fsm_decode *dec)
{
  /* Deal with incoming PROTO_IPCP */
  struct ipcp *ipcp = fsm2ipcp(fp);
  VOS_INT32 gotdnsnak;
  struct ppp_in_addr ipaddr;
  struct fsm_opt *opt;

  if ( VOS_NULL_PTR == ipcp )
  {
      return;
  }

  gotdnsnak = 0;

  while (end - cp >= (VOS_INT32)sizeof(opt->hdr)) {
    if ((opt = fsm_readopt(&cp)) == VOS_NULL_PTR)
      break;

    switch (opt->hdr.id) {
    case TY_IPADDR:        /* RFC1332 */

      switch (mode_type) {
      case MODE_REQ:
        ipcp->peer_req = 1; /* [false alarm]:移植开源代码 */

        if (!IsAccepted(ipcp->IpAddr_neg))
        {
          PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:ip address rej!\r\n");
        }
        else
        {
            if(VOS_OK != StringCompare(opt->data,&(ipcp->peer_ip.s_addr),sizeof(struct ppp_in_addr)))
            {
                PSACORE_MEM_CPY(opt->data, sizeof(struct ppp_in_addr), &(ipcp->peer_ip.s_addr), sizeof(struct ppp_in_addr));
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:ip address nak!\r\n");
            }
            else
            {
                fsm_ack(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:ip address ack!\r\n");
            }
        }

        break;
      }
      break;

    case TY_COMPPROTO:
      switch (mode_type) {
      case MODE_REQ:
        if (!IsAccepted(ipcp->CompressProto_neg)) /* [false alarm]:移植开源代码 */
        {
          if (fp->link->ipcp.stage == IPCP_SUCCESS_FROM_GGSN)
          {
              fsm_rej(dec, opt);
              PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:CompressPro rej!\r\n");
          }
          else
          {
              fsm_nak(dec, opt);
              PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:CompressPro nak(no neg)!\r\n");
          }
        }
        else
        {
            if(VOS_OK != StringCompare(opt->data,ipcp->CompressProto,ipcp->ComressProtoLen))
            {
                PSACORE_MEM_CPY(opt->data, ipcp->ComressProtoLen, ipcp->CompressProto, ipcp->ComressProtoLen);
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:CompressPro nak!\r\n");
            }
            else
            {
                fsm_ack(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:CompressPro ack!\r\n");
            }
        }

        break;
      }

      break;



    case TY_IPADDRS:        /* RFC1172 */
      switch (mode_type) {
      case MODE_REQ:
        fsm_nak(dec, opt);
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:ipaddrs rej!\r\n");
        break;

      case MODE_NAK:
      case MODE_REJ:
        break;
      }
      break;

    case TY_PRIMARY_NBNS:    /* M$ NetBIOS nameserver hack (rfc1877) */

      switch (mode_type) {
      case MODE_REQ:
        if (!IsAccepted(ipcp->PriNbns_neg)) /* [false alarm]:移植开源代码 */
        {
          if (fp->link->ipcp.stage == IPCP_SUCCESS_FROM_GGSN)
          {
              fsm_rej(dec, opt);
              PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_nbns rej!\r\n");
          }
          else
          {
              fsm_nak(dec, opt);
              PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_nbns nak(no neg)!\r\n");
          }
        }
        else
        {
            if(VOS_OK != StringCompare(opt->data,&(ipcp->PriNbnsAddr.s_addr),sizeof(struct ppp_in_addr)))
            {
                PSACORE_MEM_CPY(opt->data,sizeof(struct ppp_in_addr), &(ipcp->PriNbnsAddr.s_addr),sizeof(struct ppp_in_addr));
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_nbns nak!\r\n");
            }
            else
            {
                fsm_ack(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_nbns ack!\r\n");
            }
        }

        break;

      case MODE_NAK:
      case MODE_REJ:
        break;
      }
      break;

    case TY_SECONDARY_NBNS:

      switch (mode_type) {
      case MODE_REQ:
        if (!IsAccepted(ipcp->SecNbns_neg)) /* [false alarm]:移植开源代码 */
        {
          if (fp->link->ipcp.stage == IPCP_SUCCESS_FROM_GGSN)
          {
              fsm_rej(dec, opt);
              PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_nbns rej!\r\n");
          }
          else
          {
              fsm_nak(dec, opt);
              PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_nbns nak(no neg)!\r\n");
          }
        }
        else
        {
            if(VOS_OK != StringCompare(opt->data,&(ipcp->SecNbnsAddr.s_addr),sizeof(struct ppp_in_addr)))
            {
                PSACORE_MEM_CPY(opt->data,sizeof(struct ppp_in_addr),&(ipcp->SecNbnsAddr.s_addr),sizeof(struct ppp_in_addr));
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_nbns nak!\r\n");
            }
            else
            {
                fsm_ack(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_nbns ack!\r\n");
            }
        }

        break;

      case MODE_NAK:
      case MODE_REJ:
        break;
      }
      break;

    case TY_PRIMARY_DNS:    /* primary dns */
      switch (mode_type) {
      case MODE_REQ:
        if (!IsAccepted(ipcp->PriDns_neg)) /* [false alarm]:移植开源代码 */
        {
            if (fp->link->ipcp.stage == IPCP_SUCCESS_FROM_GGSN)
            {
                fsm_rej(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_dns rej!\r\n");
            }
            else
            {
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_dns nak(no neg)!\r\n");
            }
        }
        else
        {
            if(VOS_OK != StringCompare(opt->data,&(ipcp->PriDnsAddr.s_addr),sizeof(struct ppp_in_addr)))
            {
                PSACORE_MEM_CPY(opt->data,sizeof(struct ppp_in_addr),&(ipcp->PriDnsAddr.s_addr),sizeof(struct ppp_in_addr));
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_dns nak!\r\n");
            }
            else
            {
                fsm_ack(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:pri_dns ack!\r\n");
            }
        }

        break;

      case MODE_NAK:
      case MODE_REJ:
        break;
      }
      break;

    case TY_SECONDARY_DNS:    /* primary dns */
      switch (mode_type) {
      case MODE_REQ:
        if (!IsAccepted(ipcp->SecDns_neg)) /* [false alarm]:移植开源代码 */
        {
            if (fp->link->ipcp.stage == IPCP_SUCCESS_FROM_GGSN)
            {
                fsm_rej(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_dns rej!\r\n");
            }
            else
            {
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_dns nak(no neg)!\r\n");
            }
        }
        else
        {
            if(VOS_OK != StringCompare(opt->data,&(ipcp->SecDnsAddr.s_addr),sizeof(struct ppp_in_addr)))
            {
                PSACORE_MEM_CPY(opt->data,sizeof(struct ppp_in_addr),&(ipcp->SecDnsAddr.s_addr),sizeof(struct ppp_in_addr));
                fsm_nak(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_dns nak!\r\n");
            }
            else
            {
                fsm_ack(dec, opt);
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "ipcp:sec_dns ack!\r\n");
            }
        }

        break;

      case MODE_NAK:
      case MODE_REJ:
        break;
      }
      break;

    default:
      if (mode_type != MODE_NOP) {
        ipcp->my_reject |= (1 << opt->hdr.id); /* [false alarm]:移植开源代码 */
        fsm_rej(dec, opt);
      }
      break;
    }
  }

  if (mode_type != MODE_NOP) {
    if (mode_type == MODE_REQ && !ipcp->peer_req) { /* [false alarm]:移植开源代码 */
      if (dec->rejend == dec->rej && dec->nakend == dec->nak) {
        /*
         * Pretend the peer has requested an IP.
         * We do this to ensure that we only send one NAK if the only
         * reason for the NAK is because the peer isn't sending a
         * TY_IPADDR REQ.  This stops us from repeatedly trying to tell
         * the peer that we have to have an IP address on their end.
         */
        ipcp->peer_req = 1;
      }
      ipaddr.s_addr = INADDR_ANY;
    }
    fsm_opt_normalise(dec);
  }

}

VOS_VOID Ppp_ProcConfigInfoInd(VOS_UINT16 usPppId)
{
    PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,"Ppp_ProcConfigInfoInd begain\r\n");

    /* 可维可测信息上报*/
    Ppp_EventMntnInfo(usPppId, PPP_CONFIG_INFO_PROC_NOTIFY);

    /* PPP ID非法*/
    if((usPppId < 1)
           ||(usPppId > PPP_MAX_ID_NUM))
    {
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,"PppId out of range!\r\n");
        return ;
    }

    /*定时器关闭*/
    if (VOS_NULL_PTR != (PPP_LINK(usPppId)->ipcp.hIpcpPendTimer))
    {
        PS_STOP_REL_TIMER(&(PPP_LINK(usPppId)->ipcp.hIpcpPendTimer));
        PPP_LINK(usPppId)->ipcp.hIpcpPendTimer = VOS_NULL_PTR;
    }


    /*处理待处理IPCP帧*/
    if (VOS_NULL_PTR != (PPP_LINK(usPppId)->ipcp.pstIpcpPendFrame))
    {
        if (PHASE_NETWORK != PPP_LINK(usPppId)->phase )
        {
            /* 如果当前不是IPCP阶段则直接丢弃待处理帧返回*/
            ppp_m_freem(PPP_LINK(usPppId)->ipcp.pstIpcpPendFrame);
        }
        else
        {
            /* 如果是IPCP阶段,则直接处理*/
            fsm_Input(&(PPP_LINK(usPppId)->ipcp.fsm), PPP_LINK(usPppId)->ipcp.pstIpcpPendFrame);
        }

        /*待处理IPCP帧置空*/
        PPP_LINK(usPppId)->ipcp.pstIpcpPendFrame = VOS_NULL_PTR;
    }

    return;
}

/*lint -e{508}  extern*/
extern PPP_ZC_STRU *
ipcp_Input(/*struct bundle *bundle, */struct link *l, PPP_ZC_STRU *pstMem)
{
    struct ppp_mbuf     *bp        = VOS_NULL_PTR;
    VOS_UINT32       ulRtn     = VOS_OK;
       VOS_UINT8         ucCode = 0;

    bp = ppp_m_get_from_ttfmem(pstMem);

    PPP_MemFree(pstMem);

    if (VOS_NULL_PTR == bp)
    {
        return VOS_NULL_PTR;
    }

    /* 如果当前还不是IPCP阶段则直接丢弃返回*/
    if (PHASE_NETWORK != l->phase )
    {
        if (l->phase < PHASE_NETWORK)
        {
            PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,
                          "Error: Unexpected IPCP in phase %d\r\n",(VOS_INT32)l->phase);
        }

        ppp_m_freem(bp);
        return VOS_NULL_PTR;
    }
    /*else if ( l->phase == PHASE_NETWORK) 后续是当前为IPCP阶段的处理*/


    /*如果不是IPCP Config Request则应该立即处理*/
    ppp_mbuf_View(bp, &ucCode, sizeof (ucCode));
    if (ucCode != CODE_CONFIGREQ)
    {
        fsm_Input(&(l->ipcp.fsm), bp);
        return VOS_NULL_PTR;
    }
    /*else if (ucCode == CODE_CONFIGREQ) 后续是IPCP Config Request的处理*/


    /*释放待处理帧,收到新的IPCP Config Request帧,则说明老的帧已经失效*/
    if (VOS_NULL_PTR  != l->ipcp.pstIpcpPendFrame)
    {
        ppp_m_freem(l->ipcp.pstIpcpPendFrame);
        l->ipcp.pstIpcpPendFrame = VOS_NULL_PTR;
    }


    /*网侧已经激活或还未开始激活,则直接处理并返回*/
    if ( IPCP_SUCCESS_FROM_GGSN == l->ipcp.stage
        || IPCP_NOT_RECEIVE_REQ  == l->ipcp.stage)
    {
        fsm_Input(&(l->ipcp.fsm), bp);
        return VOS_NULL_PTR;
    }

    /*网侧在等待激活,则起定时器,并保存待处理包*/
    if (VOS_NULL_PTR == l->ipcp.hIpcpPendTimer)
    {
        /*====================*/ /* 启动定时器 */
        ulRtn = VOS_StartRelTimer(&(l->ipcp.hIpcpPendTimer),  PS_PID_APP_PPP,
            PPP_IPCP_PENDING_TIMER_LEN,  TIMER_PDP_ACT_PENDING, (VOS_UINT32)PPP_LINK_TO_ID(l),  VOS_RELTIMER_NOLOOP,VOS_TIMER_PRECISION_0);
    }


    /*起定时器失败,则立即处理*/
    if (VOS_OK != ulRtn)
    {
        fsm_Input(&(l->ipcp.fsm), bp);
        return VOS_NULL_PTR;
    }


    l->ipcp.pstIpcpPendFrame = bp;

    return VOS_NULL_PTR;
}

VOS_UINT32
ipcp_QueueLen(struct ipcp *ipcp)
{
  struct ppp_mqueue *q;
  VOS_UINT32 result;

  result = 0;
  for (q = ipcp->Queue; q < ipcp->Queue + IPCP_QUEUES(ipcp); q++)
    result += q->len;

  return result;
}

VOS_INT32
ipcp_PushPacket(struct ipcp *ipcp, struct link *l)
{
  struct ppp_mqueue *queue;
  struct ppp_mbuf *bp;
  VOS_INT32 m_len;
  VOS_UINT32 secs = 0;

  if (ipcp->fsm.state != ST_OPENED)
    return 0;
  queue = ipcp->Queue + IPCP_QUEUES(ipcp) - 1;
  do {
    if (queue->top) {
      bp = ppp_m_dequeue(queue);
      bp = ppp_mbuf_Read(bp, &secs, sizeof secs);
      bp = ppp_m_pullup(bp);
      m_len = ppp_m_length(bp);

      link_PushPacket(l, bp, 0, PROTO_IPCP);
      ipcp_AddOutOctets(ipcp, m_len);
      return 1;
    }
  } while (queue-- != ipcp->Queue);

  return 0;
}


