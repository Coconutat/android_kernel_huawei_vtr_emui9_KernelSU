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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/chap.h 330449 2018-03-05 07:26:05Z eadler $
 */

#ifndef __PPP_CHAP_H__
#define __PPP_CHAP_H__

#include "PPP/Inc/auth.h"

#pragma pack(4)

#define    CHAP_CHALLENGE    (1)
#define    CHAP_RESPONSE     (2)
#define    CHAP_SUCCESS      (3)
#define    CHAP_FAILURE      (4)

struct chap {
  struct authinfo auth;    /* common structure of authentication */
  struct {
    /*
     * format of local:
     *  --------------------------------- ------------------- --------
     * |  length of local challenge(1B)  |  local challenge  |  name  |
     *  --------------------------------- ------------------- --------
     */
    VOS_UINT8 local[1+CHAPCHALLENGELEN + AUTHLEN];    /* I invented this one */
    /*
     * format of peer:
     *  -------------------------------- ------------------
     * |  length of peer challenge(1B)  |  peer challenge  |
     *  -------------------------------- ------------------
     in fact, peer just need (1+CHAPCHALLENGELEN) bytes,
     but in BSD, peer is (CHAPCHALLENGELEN + AUTHLEN) bytes
     right now, we assume peer need (1+CHAPCHALLENGELEN + AUTHLEN) bytes
     */
    VOS_UINT8 peer[1+CHAPCHALLENGELEN + AUTHLEN];    /* Peer gave us this one */
  } challenge;    /* used in Authentication phase follow CHAP, RFC1994 */
  struct
  {
    /*
        这个结构是为了保留发送到GGSN完整的config req报文而设的,
        把发送的challenge报文和接收到的response报文发给AT,
        需要注意这部分内容需要提交给TAF使用, 根据PPP与TAF的接口设计,
        BufChallenge和BufResponse的长度都不能超过253Bytes
    */
    VOS_UINT16 LenOfChallenge;
    VOS_UINT16 LenOfResponse;
    VOS_UINT8  BufChallenge[1+1+2+1+CHAPCHALLENGELEN + AUTHLEN];    /* code(1B)+id(1B)+length(2B)+challenge_size(1B)+challenge+name */
    VOS_UINT8  BufResponse[1+1+2+1+PASSWORDLEN + AUTHLEN];    /* code(1B)+id(1B)+length(2B)+response_size(1B)+response+name */
  } RecordData;
};

#define auth2chap(a) \
  ((struct chap *)((VOS_INT8 *)a - (VOS_UINT_PTR)&((struct chap *)0)->auth))

void chap_Init(struct chap *);
void chap_ReInit(struct chap *);
PPP_ZC_STRU *chap_Input(struct link *, PPP_ZC_STRU *);

#pragma pack()


#endif    /* end of chap.h */


