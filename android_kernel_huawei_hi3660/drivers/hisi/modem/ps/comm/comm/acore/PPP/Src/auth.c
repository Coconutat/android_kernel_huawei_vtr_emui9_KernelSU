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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/auth.c 330449 2018-03-05 07:26:05Z eadler $
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

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_AUTH_C


const VOS_CHAR *
Auth2Nam(VOS_UINT16 auth, VOS_CHAR chap_type)
{
  static char chap[16];

  switch (auth) {
  case PROTO_PAP:
    return "PAP";

  case PROTO_CHAP:
    /* CHAP05(Challenge Handshake Authentication proto)
       CHAP80(Microsoft (NT/LANMan) CHAP)
       CHAP81(Microsoft CHAP v2)
    */
    (VOS_VOID)VOS_nsprintf_s(chap, (VOS_SIZE_T)(sizeof(chap)), (VOS_SIZE_T)(sizeof(chap) - 1), "CHAP 0x%02x", chap_type);
    return chap;

  case 0:
    return "None";
  }

  return "Unknown";
}


VOS_VOID AuthTimeout(struct link *l)
{
  VOS_UINT32 ulTimerValue;

  struct authinfo *authp = &(l->chap.auth);

  if (--authp->retry > 0) {    /* can still try */
      authp->id++;
      if (VOS_NULL_PTR != authp->fn.req)    /* add protection of function pointer */
      {
          (*authp->fn.req)(l);
          ulTimerValue = PPP_FSM_TIME_INTERVAL;
          if (authp->cfg.fsm.timeout > 0)
          {
              ulTimerValue = PS_MIN(authp->cfg.fsm.timeout*1000, PPP_FSM_TIME_INTERVAL);
          }
          VOS_StartRelTimer(&(authp->hAuthTimer), PS_PID_APP_PPP,
              ulTimerValue, TIMER_PPP_PHASE_MSG, (VOS_UINT32)PPP_LINK_TO_ID(l),
              VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_0);
      }
  } else {
      PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,
          "Auth: No response from server try max times. Goto lcp stage!\r\n");
      chap_ReInit(&(l->chap));
      l->phase = PHASE_TERMINATE;
      fsm_Close(&(l->lcp.fsm));
  }
}    /* AuthTimeout */

void
auth_Init(struct authinfo *authp, auth_func req,
          auth_func success, auth_func failure)
{
  PSACORE_MEM_SET(authp, sizeof(struct authinfo), '\0', sizeof(struct authinfo));
  authp->cfg.fsm.timeout = DEF_FSMRETRY;
  authp->cfg.fsm.maxreq = DEF_FSMAUTHTRIES;
  authp->cfg.fsm.maxtrm = 0;    /* not used */
  authp->fn.req = req;
  authp->fn.success = success;
  authp->fn.failure = failure;
  authp->hAuthTimer = VOS_NULL;
/*  authp->physical = p;*/
}


VOS_VOID auth_StartReq(struct link *l, struct authinfo *authp)
{
  VOS_UINT32        ulTimerValue;

  PPP_MNTN_LOG(PS_PID_APP_PPP, DIAG_MODE_COMM, PS_PRINT_NORMAL, "auth_StartReq\r\n");

  authp->retry  = (VOS_INT32)(authp->cfg.fsm.maxreq);
  authp->id     = 1;

  if (VOS_NULL_PTR != authp->fn.req)    /* add protection of function pointer */
  {
    auth_Req(authp,l);

    /* 防止在上一次认证定时器没有超时前, 发生LCP Layer Up, 再次需要认证 */
    if (VOS_NULL != authp->hAuthTimer)
    {
        PS_STOP_REL_TIMER(&(authp->hAuthTimer));
    }

    ulTimerValue = PPP_FSM_TIME_INTERVAL;
    if (authp->cfg.fsm.timeout > 0)
    {
        ulTimerValue = PS_MIN(authp->cfg.fsm.timeout*1000, PPP_FSM_TIME_INTERVAL);
    }
    VOS_StartRelTimer(&(authp->hAuthTimer), PS_PID_APP_PPP,
            ulTimerValue, TIMER_PPP_PHASE_MSG, (VOS_UINT32)PPP_LINK_TO_ID(l),
            VOS_RELTIMER_NOLOOP, VOS_TIMER_PRECISION_0);
  }
}    /* auth_StartReq */


VOS_VOID auth_StopTimer(struct authinfo *authp)
{
    if (VOS_NULL_PTR != authp->hAuthTimer)
    {
        PS_STOP_REL_TIMER(&(authp->hAuthTimer));
    }
}

/*lint -e{572} 0的移位*/
struct ppp_mbuf *
auth_ReadHeader(struct authinfo *authp, struct ppp_mbuf *bp)
{
  VOS_UINT32 len;

  len = ppp_m_length(bp);
  if (len >= sizeof authp->in.hdr) {
    bp = ppp_mbuf_Read(bp, (VOS_CHAR *)&authp->in.hdr, sizeof authp->in.hdr);
    if (len >= (VOS_UINT32)VOS_NTOHS(authp->in.hdr.length))
      return bp;
    authp->in.hdr.length = VOS_HTONS(0);
    PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_WARNING, "auth_ReadHeader: Short packet (%d > %d): !",
               VOS_NTOHS(authp->in.hdr.length), len);
  } else {
    authp->in.hdr.length = VOS_HTONS(0);
    PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_WARNING, "auth_ReadHeader: Short packet header (%d > %d) !",
               (VOS_INT32)(sizeof authp->in.hdr), len);
  }

  ppp_m_freem(bp);
  return VOS_NULL_PTR;
}

struct ppp_mbuf *
auth_ReadName(struct authinfo *authp, struct ppp_mbuf *bp, VOS_UINT32 len)
{
  if (len > sizeof authp->in.name - 1)
  {
    PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                  "auth_ReadName: Name too long (%d) !\r\n", len);
  }
  else
  {
    VOS_UINT32 mlen = ppp_m_length(bp);

    if (len > mlen)
    {
      PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                    "auth_ReadName: Short packet (%d > %d) !\r\n",
                    len, mlen);
    }
    else
    {
      bp = ppp_mbuf_Read(bp, (VOS_CHAR *)authp->in.name, len);
      authp->in.name[len] = '\0';
      return bp;
    }
  }

  *authp->in.name = '\0';
  ppp_m_freem(bp);
  return VOS_NULL_PTR;
}


