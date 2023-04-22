/*-
 * SPDX-License-Identifier: BSD-2-Clause-FreeBSD
 *
 * Copyright (c) 1997 Brian Somers <brian@Awfulhak.org>
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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/async.h 330449 2018-03-05 07:26:05Z eadler $
 */
#ifndef __PPP_ASYNC_H__
#define __PPP_ASYNC_H__

#include "vos.h"
#include "PPP/Inc/hdlc.h"
#include "PPP/Inc/ppp_public.h"
#include "PPP/Inc/ppp_mbuf.h"

#pragma pack(4)
#define HDLCSIZE    ((MAX_MRU*2)+6)


struct async {
  VOS_INT32 mode;
  VOS_INT32 length;
  VOS_CHAR hbuff[HDLCSIZE+2];    /* recv buffer */
  VOS_CHAR xbuff[HDLCSIZE+2];    /* xmit buffer */

  struct {
    VOS_CHAR EscMap[33];
    VOS_UINT8   aucReserved[7];
  } cfg;
};

struct lcp;
extern void async_Init(struct async *);
extern void async_Setup(struct async *);
extern void async_SetLinkParams(struct async *, VOS_UINT32, VOS_UINT32);
extern struct ppp_mbuf * async_TtfMemLayerPush(struct link *l, VOS_UINT8 *pHdr, VOS_UINT16 usHdrLen, PPP_ZC_STRU *bp,
                VOS_INT32 pri, VOS_UINT16 *proto);
extern struct ppp_mbuf * async_LayerPush(struct link *l, struct ppp_mbuf *bp,
                VOS_INT32 pri, VOS_UINT16 *proto);
extern PPP_ZC_STRU * async_Decode(struct async *async, VOS_CHAR c);


extern struct layer asynclayer;

#pragma pack()

#endif /* end of async.h */
