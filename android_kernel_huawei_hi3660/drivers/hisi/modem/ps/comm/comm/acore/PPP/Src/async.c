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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/async.c 330449 2018-03-05 07:26:05Z eadler $
 */


#include "PPP/Inc/ppp_public.h"
#include "PPP/Inc/ppp_mbuf.h"
#include "PPP/Inc/layer.h"
#include "PPP/Inc/ppp_fsm.h"
#include "PPP/Inc/hdlc.h"
#include "PPP/Inc/lcp.h"
#include "PPP/Inc/proto.h"
#include "PPP/Inc/async.h"
#include "PPP/Inc/throughput.h"
#include "PPP/Inc/auth.h"
#include "PPP/Inc/ipcp.h"
#include "PPP/Inc/pppid.h"
#include "PPP/Inc/link.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_ASYNC_C

#define MODE_HUNT 0x01
#define MODE_ESC  0x02

extern VOS_UINT32 PPP_SendPushedData(VOS_UINT16 usPppId, VOS_UINT8 * pucDataBuf, VOS_UINT16 usLen);

void
async_Init(struct async *async)
{
  async_Setup(async);

  PSACORE_MEM_SET(async->cfg.EscMap, sizeof async->cfg.EscMap, '\0', sizeof async->cfg.EscMap);
}

void
async_Setup(struct async *async)
{
  async->mode = MODE_HUNT;
  async->length = 0;
}

void
async_SetLinkParams(struct async *async, VOS_UINT32 mymap, VOS_UINT32 hismap)
{
}

/*
 * Encode into async HDLC byte code
 */
void
async_Encode(struct link *l, VOS_CHAR **cp, VOS_CHAR c, VOS_INT32 proto)
{
  VOS_CHAR *wp;

  VOS_UINT8 ch = (VOS_UINT8)c;

  wp = *cp;
  if ((ch < 0x20 && (proto == PROTO_LCP || (l->lcp.his_accmap & (1 << ch))))
      || (ch == HDLC_ESC) || (ch == HDLC_SYN)) {
    *wp++ = HDLC_ESC;
    ch ^= HDLC_XOR;
  }
  *wp++ = ch;
  *cp = wp;
}

extern VOS_UINT16 const fcstab[];


struct ppp_mbuf *
async_LayerPush(struct link *l, struct ppp_mbuf *bp,
                VOS_INT32 pri, VOS_UINT16 *proto)
{
  VOS_CHAR *cp, *sp, *ep;
  struct ppp_mbuf *wp;
  VOS_UINT32 cnt;
  VOS_UINT16 fcs;

  if (ppp_m_length(bp) > HDLCSIZE) {
    ppp_m_freem(bp);
    return VOS_NULL_PTR;
  }

  cp = l->async.xbuff;

  ep = cp + HDLCSIZE - 10;
  wp = bp;
  fcs = INITFCS;        /* Initial FCS is 0xffff */

  *cp++ = HDLC_SYN;

  /* culating FCS and stuffing in a ppp frame at the same time */
  while (wp) {
    sp = PPP_MBUF_CTOP(wp);
    for (cnt = wp->m_len; cnt > 0; cnt--) {
      fcs = (fcs >> 8) ^ fcstab[(fcs ^ *sp) & 0xff]; /* [false alarm]:移植开源代码 */
      async_Encode(l, &cp, *sp++, *proto);
      if (cp >= ep) {
        ppp_m_freem(bp);
        return VOS_NULL_PTR;
      }
    }
    wp = wp->m_next;
  }

  fcs = ~fcs;
  async_Encode(l, &cp, (fcs & 0xFF), *proto);    /* Low byte first (nothing like consistency) */
  async_Encode(l, &cp, (fcs >> 8), *proto);

  *cp++ = HDLC_SYN;

  cnt = cp - l->async.xbuff;
  ppp_m_freem(bp);

  PPP_SendPushedData((VOS_UINT16)PPP_LINK_TO_ID(l), (VOS_UINT8 *)(l->async.xbuff), (VOS_UINT16)cnt);

  return VOS_NULL_PTR;
}

struct ppp_mbuf *
async_TtfMemLayerPush(struct link *l, VOS_UINT8 *pHdr, VOS_UINT16 usHdrLen, PPP_ZC_STRU *bp,
                VOS_INT32 pri, VOS_UINT16 *proto)
{
    VOS_CHAR   *cp, *ep;
    VOS_UINT8  *sp;
    PPP_ZC_STRU *wp;
    VOS_INT32   cnt;
    VOS_UINT16  fcs;
    VOS_UINT16        usLen;

    cp = l->async.xbuff;

    ep = cp + HDLCSIZE - 10;
    fcs = INITFCS;        /* Initial FCS is 0xffff */

    *cp++ = HDLC_SYN;

    /* 先对PPP头部进行转义 */
    sp = pHdr;
    while (usHdrLen > 0)
    {
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ *sp) & 0xff];

        if ((*sp < 0x20 && (*proto == PROTO_LCP || (l->lcp.his_accmap & (1 << *sp))))
            || (*sp == HDLC_ESC)
            || (*sp == HDLC_SYN))
        {
            *cp++ = HDLC_ESC;
            *cp++ = *sp^HDLC_XOR;
        }
        else
        {
            *cp++   = *sp;
        }

        sp++;
        usHdrLen--;
    }


    wp = bp;
    /* culating FCS and stuffing in a ppp frame at the same time */
    sp    = PPP_ZC_GET_DATA_PTR(wp);
    usLen = PPP_ZC_GET_DATA_LEN(wp);

    for (cnt = usLen; cnt > 0; cnt--)
    {
        fcs = (fcs >> 8) ^ fcstab[(fcs ^ *sp) & 0xff];

        if ((*sp < 0x20 && (*proto == PROTO_LCP || (l->lcp.his_accmap & (1 << *sp))))
            || (*sp == HDLC_ESC)
            || (*sp == HDLC_SYN))
        {
            *cp++ = HDLC_ESC;
            *cp++ = *sp^HDLC_XOR;
        }
        else
        {
            *cp++   = *sp;
        }

        sp++;
    }

    fcs = ~fcs;
    async_Encode(l, &cp, (fcs & 0xFF), *proto);    /* Low byte first (nothing like consistency) */
    async_Encode(l, &cp, (fcs >> 8), *proto);

    *cp++ = HDLC_SYN;

    cnt = cp - l->async.xbuff;
    PPP_MemFree(bp);

    /* 把封装好的PPP帧发往串口 */
    PPP_SendPushedData((VOS_UINT16)PPP_LINK_TO_ID(l), (VOS_UINT8 *)(l->async.xbuff), (VOS_UINT16)cnt);

    return VOS_NULL_PTR;
}


struct ppp_mbuf * Ppp_asyncLayerPush(struct link *l, struct ppp_mbuf *bp, VOS_INT32 pri, VOS_UINT16 *proto)
{
    VOS_CHAR *cp, *sp, *ep;
    struct ppp_mbuf *wp;
    VOS_INT32 cnt;

    if (ppp_m_length(bp) > HDLCSIZE)
    {
        ppp_m_freem(bp);
        return VOS_NULL_PTR;
    }

    cp = l->async.xbuff;

    ep = cp + HDLCSIZE - 10;
    wp = bp;


    *cp++ = HDLC_SYN;
    while (wp)
    {
        sp = PPP_MBUF_CTOP(wp);
        for (cnt = wp->m_len; cnt > 0; cnt--)
        {
            async_Encode(l, &cp, *sp++, *proto); /* [false alarm]:移植开源代码 */
            if (cp >= ep)
            {
                ppp_m_freem(bp);
                return VOS_NULL_PTR;
            }
        }
        wp = wp->m_next;
    }
    *cp++ = HDLC_SYN;

    cnt = cp - l->async.xbuff;
    ppp_m_freem(bp);

    PPP_SendPushedData((VOS_UINT16)PPP_LINK_TO_ID(l), (VOS_UINT8 *)(l->async.xbuff), (VOS_UINT16)cnt);

    return VOS_NULL_PTR;
}

PPP_ZC_STRU * async_Decode(struct async *async, VOS_CHAR c)
{
    PPP_ZC_STRU   *pstMem;

    if ( (async->mode & MODE_HUNT) && (c != HDLC_SYN) )
    {
        return VOS_NULL_PTR;
    }

    switch (c)
    {
    case HDLC_SYN:
        async->mode &= ~MODE_HUNT;

        if (async->length)
        {
            /* packet is ready. */
            pstMem = PPP_MemCopyAlloc((VOS_UINT8 *)async->hbuff,
                                      (VOS_UINT16)(async->length),
                                      PPP_ZC_UL_RESERVE_LEN);

            async->length = 0;

            return pstMem;
        }
        break;

    case HDLC_ESC:
        if (!(async->mode & MODE_ESC))
        {
            async->mode |= MODE_ESC;
            break;
        }

    /* FALLTHROUGH */
    default:
        if (async->length >= HDLCSIZE)
        {
            /* packet is too large, discard it */
            PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, PS_PRINT_WARNING, "Packet too large (%d), discarding.\r\n",
                async->length);
            async->length = 0;
            async->mode = MODE_HUNT;
            break;
        }

        if (async->mode & MODE_ESC)
        {
            c ^= HDLC_XOR;
            async->mode &= ~MODE_ESC;
        }

        async->hbuff[async->length++] = c;
        break;
    }

    return VOS_NULL_PTR;
} /* async_Decode */


/*该函数已没有调用, 所以直接返回.....对于所有的Pull函数，其中的bp参数都不为空，但对于最底层的，该bp为空*/
PPP_ZC_STRU *
async_LayerPull(struct link *l, PPP_ZC_STRU *bp, VOS_UINT16 *proto)
{
  return VOS_NULL_PTR;
}

struct layer asynclayer =
  { LAYER_ASYNC, "async", async_LayerPush, async_LayerPull };


