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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/hdlc.h 330449 2018-03-05 07:26:05Z eadler $
 */

#ifndef __PPP_HDLC_H__
#define __PPP_HDLC_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include "vos.h"
#if (FEATURE_ON == FEATURE_HARDWARE_HDLC_FUNC)
#include "PPP/Inc/hdlc_hardware_service.h"
#endif


/*
 *  Definition for Async HDLC
 */
#pragma pack(4)
#define HDLC_SYN 0x7e        /* SYNC character */
#define HDLC_ESC 0x7d        /* Escape character */
#define HDLC_XOR 0x20        /* Modifier value */

#define    HDLC_ADDR 0xff
#define    HDLC_UI      0x03
/*
 *  Definition for HDLC Frame Check Sequence
 */
#define INITFCS 0xffff        /* Initial value for FCS computation */
#define GOODFCS 0xf0b8        /* Good FCS value */

#define    DEF_MRU        1500
#define    MAX_MRU        2048
#define    MIN_MRU        296

#define    DEF_MTU        0    /* whatever peer says */
#define    MAX_MTU        2048
#define    MIN_MTU        296

/* 解封装单个输入报文最大长度为可申请内存块最大长度 */
#define PPP_HDLC_DEF_ONE_MAX_SIZE       (2048)

#define HDLC_MAKE_WORD(hi,low)    ((VOS_UINT16)(((VOS_UINT8)(low))|(((VOS_UINT16)((VOS_UINT8)(hi)))<<8)))
#define GET_BITS_FROM_BYTE(Byte, BitMask)  ((Byte) & (BitMask))

#define INSERT_BYTE_LAST(pstMem, pstData, usLen, byte) \
{ \
    pstData[usLen]     = pstData[usLen - 1]; \
    pstData[usLen - 1] = byte; \
    PPP_ZC_SET_DATA_LEN(pstMem, 1); \
}


struct ppp_mbuf;

struct hdlc {
  HTIMER                      hTimerHandle;                   /* LCP ECHO周期性定时器 */

  struct {
    VOS_INT32 badfcs;
    VOS_INT32 badaddr;
    VOS_INT32 badcommand;
    VOS_INT32 unknownproto;
  } laststats, stats;

  struct {
    struct lcp *owner;            /* parent LCP */
    VOS_INT32  method;                /* bit-mask for LQM_* from lqr.h */

    VOS_UINT32 ifOutUniPackets;        /* Packets sent by me */
    VOS_UINT32 ifOutOctets;             /* Octets sent by me */
    VOS_UINT32 ifInUniPackets;        /* Packets received from peer */
    VOS_UINT32 ifInDiscards;        /* Discards */
    VOS_UINT32 ifInErrors;        /* Errors */
    VOS_UINT32 ifInOctets;        /* Octets received from peer (unused) */

    struct {
      VOS_UINT32 InGoodOctets;      /* Good octets received from peer */
      VOS_UINT32 OutLQRs;           /* LQRs sent by me */
      VOS_UINT32 InLQRs;            /* LQRs received from peer */
      VOS_INT32  peer_timeout;      /* peers max lqr timeout */
      VOS_INT32  resent;            /* Resent last packet `resent' times */
    } lqr;

    struct {
      VOS_UINT32 seq_sent;        /* last echo sent */
      VOS_UINT32 seq_recv;        /* last echo received */
    } echo;
  } lqm;
};

extern void hdlc_Init(struct hdlc *, struct lcp *);
extern VOS_UINT16 hdlc_Fcs(VOS_CHAR *, VOS_UINT32);
extern PPP_ZC_STRU *hdlc_LayerPull(struct link *l, PPP_ZC_STRU *pstMem, VOS_UINT16 *proto);

#if (FEATURE_ON == FEATURE_HARDWARE_HDLC_FUNC)
#define     PPP_HDLC_CACHE_INVALID(data, len)                   dma_map_single(0, data, len, DMA_FROM_DEVICE)
#define     PPP_HDLC_CACHE_FLUSH(data, len)                     dma_map_single(0, data, len, DMA_TO_DEVICE)
#endif


#define    hdlc_WrapperOctets() (2)

extern struct layer hdlclayer;

#pragma pack()

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of hdlc.h */
