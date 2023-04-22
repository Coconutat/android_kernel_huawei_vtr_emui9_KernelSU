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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/auth.h 330449 2018-03-05 07:26:05Z eadler $
 */

#ifndef __PPP_AUTH_H__
#define __PPP_AUTH_H__

#include "PppInterface.h"
#include "PPP/Inc/ppp_fsm.h"

#pragma pack(4)

#ifndef AUTHLEN
#define AUTHLEN  (100)    /* Size of authname/authkey(porting from BSD, not alter) */
#endif
#ifndef PASSWORDLEN
#define PASSWORDLEN (100)    /* Size of authname/authkey */
#endif
#ifndef CHAPCHALLENGELEN
#define CHAPCHALLENGELEN (48)    /* Maximum chap challenge(porting from BSD, not alter) */
#endif
#ifndef MD5DIGESTSIZE
#define MD5DIGESTSIZE (16)    /* MD5 (Message-Digest) hash size */
#endif

struct authinfo;
typedef void (*auth_func)(struct link *);


struct authinfo {
  struct {
    auth_func req;
    auth_func success;
    auth_func failure;
  } fn;

  HTIMER        hAuthTimer;

  struct {
    struct fsmheader    hdr;
    VOS_CHAR            name[AUTHLEN];
  } in;

  VOS_INT32     retry;

  VOS_CHAR  id;
  VOS_UINT8 aucReserved1[3];

  struct {
    struct fsm_retry fsm;    /* How often/frequently to resend requests */
  } cfg;
};

#define auth_Req(a,l)       (*(a)->fn.req)(l)
#define auth_Failure(a,l)   (*(a)->fn.failure)(l)
#define auth_Success(a,l)   (*(a)->fn.success)(l)

extern const VOS_CHAR *Auth2Nam(VOS_UINT16, VOS_CHAR);
extern void auth_Init(struct authinfo *, auth_func, auth_func, auth_func);
extern void auth_StopTimer(struct authinfo *);
extern void auth_StartReq(struct link *, struct authinfo *);
extern void AuthTimeout(struct link *);
extern struct ppp_mbuf *auth_ReadHeader(struct authinfo *, struct ppp_mbuf *);
extern struct ppp_mbuf *auth_ReadName(struct authinfo *, struct ppp_mbuf *, VOS_UINT32);

#pragma pack()

#endif /* end of auth.h */
