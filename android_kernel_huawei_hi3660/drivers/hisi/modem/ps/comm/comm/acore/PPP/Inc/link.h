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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/link.h 330449 2018-03-05 07:26:05Z eadler $
 *
 */
#ifndef __PPP_LINK_H__
#define __PPP_LINK_H__

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

#include  "PPP/Inc/ppp_public.h"
#include  "PsTypeDef.h"
#include  "PPP/Inc/auth.h"
#include  "PPP/Inc/hdlc_interface.h"


#pragma pack(4)

#define    PHASE_DEAD              0    /* Link is dead */
#define    PHASE_ESTABLISH         1    /* Establishing link */
#define    PHASE_AUTHENTICATE      2    /* Being authenticated */
#define    PHASE_NETWORK           3    /* We're alive ! */
#define    PHASE_TERMINATE         4    /* Terminating link */

#define PHYSICAL_LINK    1
#define LOGICAL_LINK     2

#define PPP_HDLC_BY_SOFTWARE                            (0)         /* 使用软件实现PPP封装 解封装 */
#define PPP_HDLC_BY_HARDWARE                            (1)         /* 使用硬件实现PPP封装 解封装 */

extern struct  fsm_parent           parent;

#define PPP_CONFIG(PppId)           ((g_pstHdlcConfig + PppId) - 1)

extern void     link_PushPacket(struct link *, struct ppp_mbuf *, VOS_INT32 ulQueue, VOS_UINT16 usProto);
extern void     link_SequenceQueue(struct link *);
extern VOS_VOID link_Init(struct link *);
extern VOS_VOID PPP_InitSecureData(VOS_UINT8 ucPppId);

#pragma pack()

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


#endif /* end of Link.h */

