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
 * $FreeBSD: releng/11.2/usr.sbin/ppp/chap.c 330449 2018-03-05 07:26:05Z eadler $
 */

#include "PPP/Inc/link.h"
#include "PPP/Inc/ppp_fsm.h"

/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_CHAP_C


static const VOS_CHAR * const chapcodes[] = {
  "\?\?\?", "CHALLENGE", "RESPONSE", "SUCCESS", "FAILURE"
};
#define MAXCHAPCODE ((sizeof chapcodes / sizeof chapcodes[0]) - 1)


VOS_VOID ChapOutput(struct link *l, VOS_INT32 code, VOS_CHAR id,
       const VOS_UCHAR *ptr, VOS_UINT32 count, const VOS_CHAR *text)
{
  VOS_UINT32 len;    /* length of a CHAP frame */
  struct fsmheader lh;
  struct ppp_mbuf *bp;

  len = sizeof(struct fsmheader) + count;
  lh.code = (VOS_CHAR)code;
  lh.id = id;
  lh.length = (VOS_UINT16)VOS_HTONS(len);
  bp = ppp_m_get((VOS_INT32)len);

  if (VOS_NULL_PTR == bp)
  {
      PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING, "no mbuf\r\n");
      return;
  }

  PSACORE_MEM_CPY(PPP_MBUF_CTOP(bp), sizeof(struct fsmheader), &lh, sizeof(struct fsmheader));
  if ((count > 0) && (VOS_NULL_PTR != ptr))
  {
    PSACORE_MEM_CPY(PPP_MBUF_CTOP(bp) + sizeof(struct fsmheader), count, ptr, count);
  }

  if (VOS_NULL_PTR == text)
  {
    PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "Chap Output: code %d\r\n", code);
  }
  else
  {
    PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,"Chap Output: code %d with text\r\n", code);
  }

  /* now a CHAP frame is ready */
  link_PushPacket(l, bp, LINK_QUEUES(l) - 1, PROTO_CHAP);

  return;
}    /* ChapOutput */


static VOS_VOID chap_Cleanup(struct chap *chap)
{
    *chap->challenge.local = 0x0;
    *chap->challenge.peer = 0x0;

    chap->auth.retry = 0;
}    /* chap_Cleanup */


static VOS_VOID chap_Respond(struct link *l, const VOS_CHAR *name)
{
  VOS_UCHAR  aucResponseBody[1+MD5DIGESTSIZE+AUTHLEN];
  VOS_UINT32 len;    /* length of Response body */
  VOS_UINT32 ulHashValueLoop;

  /* Response body: */
  /*
   *  ------------------- --------------------- ----------
   * |   HASH-Size(1B)   |   HASH-Value(16B)   |   Name   |
   *  ------------------- --------------------- ----------
   */
  len = 1 + MD5DIGESTSIZE + VOS_StrLen((VOS_CHAR *)name);    /* BSD always thinks user name is not beyong AUTHLEN octets */

  aucResponseBody[0] = MD5DIGESTSIZE;    /* as CHAP only support MD5, MD5 hash value is 16 octets */

  /* in our product, when rx-ed Challenge from PC, just response hash value with zero */
  for (ulHashValueLoop = 1; ulHashValueLoop <= MD5DIGESTSIZE; ulHashValueLoop ++)
  {
      aucResponseBody[ulHashValueLoop] = 0x00;
  }

  if ((VOS_NULL_PTR != name) && ('\0' != *name))
  {
      PSACORE_MEM_CPY(&aucResponseBody[1+MD5DIGESTSIZE], VOS_StrLen((VOS_CHAR*)name), name, VOS_StrLen((VOS_CHAR*)name));
  }

  ChapOutput(l, CHAP_RESPONSE, (l->chap.auth.id), aucResponseBody, len, name);

  return;
}    /* chap_Respond */

/*
   if support mutil-thread, use child thread to read and write challenge/response to file,
   we do NOT need it, masked by liukai
*/


VOS_VOID ChapBufferChallengePacket(struct chap *chap, VOS_CHAR id,
             const VOS_UCHAR *ptr, VOS_UINT32 count)
{
    VOS_UINT32 len;    /* length of a CHAP frame */
    struct fsmheader lh;
    VOS_UINT8 *pucChallengeBuf;

    len = sizeof(struct fsmheader) + count;
    lh.code = CHAP_CHALLENGE;
    lh.id = id;
    lh.length = (VOS_UINT16)VOS_HTONS(len);

    pucChallengeBuf = chap->RecordData.BufChallenge;
    PSACORE_MEM_CPY(pucChallengeBuf, sizeof(struct fsmheader), &lh, sizeof(struct fsmheader));
    if (count > 0)
    {
        PSACORE_MEM_CPY(pucChallengeBuf + sizeof(struct fsmheader), count, ptr, count);
    }
    chap->RecordData.LenOfChallenge = (VOS_UINT16)len;

    return;
}    /* ChapBufferChallengePacket */


VOS_VOID chap_Challenge(struct link *l)
{
  struct chap *chap = &(l->chap);
  VOS_UINT32 len;
  VOS_UINT8 *cp;
  const VOS_CHAR acLocalChallenge[] = "HUAWEI_CHAP_SERVER";   /* we always use "HUAWEI_CHAP_SERVER" as Name of Challenge */

  /* Challenge body: */
  /*
   *  ------------------------ --------------------- ----------
   * |   Challenge-Size(1B)   |   Challenge-Value   |   Name   |
   *  ------------------------ --------------------- ----------
   */
  len = VOS_StrLen((VOS_CHAR *)acLocalChallenge);

  if (0x0 == *(chap->challenge.local)) {    /* as each time local[0] is 0x0, here is always true */
    cp = chap->challenge.local;
    /* 测试组建议challenge中随机字符串长度固定为16，和标杆一致 */
    *cp++ = (VOS_UINT8)(MD5DIGESTSIZE);

    /*
      *cp++ = (VOS_UINT8)(PS_RAND(CHAPCHALLENGELEN-MD5DIGESTSIZE) + MD5DIGESTSIZE);
      随机字串长度本为任意长度, 存放在local的第一个字节,为了防止对端只支持MD5而要求长度为16, 特意保证长度至少16字节
    */
    PPP_GetSecurityRand((VOS_UINT8)(MD5DIGESTSIZE), cp);
    cp += MD5DIGESTSIZE;

    /* use memcpy instead of strcpy, as "The Name should not be NUL or CR/LF terminated." in RFC1994 */
    PSACORE_MEM_CPY(cp, len, acLocalChallenge, len);
  }

  /* each time send challenge, record its packet */
  ChapBufferChallengePacket(chap, chap->auth.id, chap->challenge.local,
      1 + *(chap->challenge.local) + len);

  ChapOutput(l, CHAP_CHALLENGE, chap->auth.id, chap->challenge.local,
         1 + *(chap->challenge.local) + len, VOS_NULL_PTR);    /* 1: challenge length, *local: 随机字串长度, len: Name length */

  return;
}    /* chap_Challenge */


static VOS_VOID chap_Success(struct link *l)
{
  struct authinfo *authp = &(l->chap.auth);
  const VOS_CHAR *pcMsg = "Welcome!!";    /* follow BSD use "Welcome!!" as message */

  /* Success body: */
  /*
   *  -------------
   * |   Message   |
   *  -------------
   */

  ChapOutput(l, CHAP_SUCCESS, authp->id, (VOS_UCHAR *)pcMsg, VOS_StrLen((VOS_CHAR *)pcMsg), VOS_NULL_PTR);

  l->lcp.auth_ineed = 0;    /* after Authentication, clear flag to authenticate peer */

  if (0 == l->lcp.auth_iwait)    /* auth_iwait: 0, authentication to peer is not complete or no need to authentication,
                                               !0, authentication to peer is complete */
  {
    /*
     * Either I didn't need to authenticate, or I've already been
     * told that I got the answer right.
     */
    chap_ReInit(&(l->chap));
    if (PHASE_AUTHENTICATE == l->phase)
    {
        l->phase = PHASE_NETWORK;
        l->ipcp.fsm.state = ST_CLOSED;
        fsm_Open(&(l->ipcp.fsm));
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "goto ipcp stage!\r\n");
    }
  }

  return;
}    /* chap_Success */



VOS_VOID chap_Init(struct chap *chap)
{
  auth_Init(&(chap->auth), chap_Challenge, chap_Success, VOS_NULL_PTR);
  chap_Cleanup(chap);
  PSACORE_MEM_SET(&(chap->RecordData), sizeof(chap->RecordData), '\0', sizeof(chap->RecordData));

  return;
}    /* chap_Init */


VOS_VOID chap_ReInit(struct chap *chap)
{
  chap_Cleanup(chap);

  return;
}    /* chap_ReInit */


VOS_VOID ChapBufferResponsePacket(struct chap *chap, VOS_UCHAR ucHashSize,
             VOS_UCHAR aucHashValue[], VOS_INT32 lNameLen)
{
    VOS_UINT8 *pucResponseNextPos;    /* the start address to store response in buffer for next time */

    pucResponseNextPos = chap->RecordData.BufResponse;

    PSACORE_MEM_CPY(pucResponseNextPos, sizeof(struct fsmheader), &(chap->auth.in.hdr), sizeof(struct fsmheader));    /* record packet header */
    pucResponseNextPos += sizeof(struct fsmheader);

    *pucResponseNextPos = ucHashSize;
    pucResponseNextPos++;    /* hash-size always use one octet */

    if (ucHashSize != 0)    /* with hash-value */
    {
        PSACORE_MEM_CPY(pucResponseNextPos, ucHashSize, aucHashValue, ucHashSize);
        pucResponseNextPos += ucHashSize;
    }
    if (('\0' != *chap->auth.in.name) && (lNameLen > 0))    /* with name */
    {
        /*
            why do NOT use strcpy, as "The Name should not be NUL or CR/LF terminated."
            in RFC1994. However, rx-ed response packets of peer may use NUL as terminate,
            so use @lNameLen, not to use strlen to calculate itself.
        */
        PSACORE_MEM_CPY(pucResponseNextPos, (VOS_UINT32)lNameLen, chap->auth.in.name, (VOS_UINT32)lNameLen);
    }
    else
    {
        *pucResponseNextPos = '\0';
    }

    chap->RecordData.LenOfResponse = VOS_NTOHS(chap->auth.in.hdr.length);

    return ;
}    /* ChapBufferResponsePacket */


/*lint -e{668} 内部保证bp若为空VOS_NTOHS(chap->auth.in.hdr.length)必为0，走分支1，不会走到释放分支*/
PPP_ZC_STRU *chap_Input(struct link *l, PPP_ZC_STRU *pstMem)
{
  struct chap *chap;
  VOS_INT32 len;
  VOS_UCHAR alen;    /* answer length: challenge or response body length */
  struct ppp_mbuf *bp;
  VOS_UCHAR aucHashValue[MD5DIGESTSIZE];

  /* 初始化 */
  alen = 0;

  bp = ppp_m_get_from_ttfmem(pstMem);
  PPP_MemFree(pstMem);

  if (VOS_NULL_PTR == bp)
  {
    return VOS_NULL_PTR;
  }

  if (VOS_NULL_PTR == l) {
    PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING, "Chap Input: Not a physical link - dropped\r\n");
    ppp_m_freem(bp);
    return VOS_NULL_PTR;
  }

  if ((PHASE_NETWORK != l->phase) &&
      (PHASE_AUTHENTICATE != l->phase)) {
    PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "Unexpected Chap input - dropped\r\n");
    ppp_m_freem(bp);
    return VOS_NULL_PTR;
  }

  chap = &(l->chap);
  if ((VOS_NULL_PTR == (bp = auth_ReadHeader(&chap->auth, bp))) &&
      (0 == VOS_NTOHS(chap->auth.in.hdr.length)))
  {
    PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_WARNING, "Chap Input: Truncated header\r\n");
  }
  else if ((0 == chap->auth.in.hdr.code) || ((VOS_UINT8)(chap->auth.in.hdr.code) > MAXCHAPCODE))
  {
    PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                  "Chap Input: Bad CHAP code %d !\r\n", chap->auth.in.hdr.code);
  }
  else {
    len = ppp_m_length(bp);

    /* Identifier of rx-ed Response, Success, Fail should match Challenge tx-ed */
    if ((CHAP_CHALLENGE != chap->auth.in.hdr.code) &&
        (chap->auth.id != chap->auth.in.hdr.id)) {
      /* Wrong conversation dude ! */
      PPP_MNTN_LOG3(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,
              "Chap Input: code <1> dropped (got id <2> not equal to previous id <3>)\r\n",
              chap->auth.in.hdr.code, chap->auth.in.hdr.id, chap->auth.id);
      ppp_m_freem(bp);
      return VOS_NULL_PTR;
    }
    chap->auth.id = chap->auth.in.hdr.id;    /* We respond with this id */

    if (CHAP_CHALLENGE == chap->auth.in.hdr.code)    /* rx-ed challenge */
    {
        bp = ppp_mbuf_Read(bp, &alen, 1);    /* fetch length of peer's challenge */
        len -= (alen + 1);    /* after this step, len is length of peer's name */
        if (len < 0) {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: Truncated challenge (len %d, alen %d)!\r\n", len, alen);
          ppp_m_freem(bp);
          return VOS_NULL_PTR;
        }
        if (AUTHLEN < len)
        {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: name of challenge too long (len %d, alen %d)!\r\n", len, alen);
          ppp_m_freem(bp);
          return VOS_NULL_PTR;
        }
        if (CHAPCHALLENGELEN < alen)
        {
          PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: challenge too long (len %d)!\r\n", alen);
          ppp_m_freem(bp);
          return VOS_NULL_PTR;
        }

        *chap->challenge.peer = alen;
        bp = ppp_mbuf_Read(bp, chap->challenge.peer + 1, alen);    /* record peer's challenge */
        bp = auth_ReadName(&chap->auth, bp, len);    /* record peer's name */

        if (*chap->auth.in.name)    /* challenge with name */
        {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: challenge (len %d, alen %d) with name\r\n",
                  len, alen);
        }
        else    /* without name */
        {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: challenge (len %d, alen %d) without name\r\n",
                        len, alen);
        }

        chap_Respond(l, "HUAWEI_CHAP_CLIENT");    /* we always use "HUAWEI_CHAP_CLIENT" as Name of Response */
    }    /* end of rx-ed challenge */
    else if (CHAP_RESPONSE == chap->auth.in.hdr.code)    /* rx-ed response */
    {
        bp = ppp_mbuf_Read(bp, &alen, 1);    /* read HASH-Size */
        if (MD5DIGESTSIZE != alen)    /* as just support MD5, must be 16 octets */
        {
          PPP_MNTN_LOG1(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: Hash-Size %f is not correct !\r\n", alen);
          ppp_m_freem(bp);
          return VOS_NULL_PTR;
        }
        len -= (alen + 1);    /* after this step, len is length of Name Field */
        if (len < 0) {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: Truncated response (len %d, alen %d)!\r\n", len, alen);
          ppp_m_freem(bp);
          return VOS_NULL_PTR;
        }
        if (AUTHLEN < len)
        {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, LOG_LEVEL_WARNING,
                        "Chap Input: name of response too long (len %d, alen %d)!\r\n", len, alen);
          ppp_m_freem(bp);
          return VOS_NULL_PTR;
        }

        bp = ppp_mbuf_Read(bp, aucHashValue, MD5DIGESTSIZE);    /* cut HASH value */
        bp = auth_ReadName(&chap->auth, bp, len);

        if (*chap->auth.in.name)
        {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,"Chap Input: response (len <1>, alen <2>) with name\r\n",
                  len, alen);
        }
        else
        {
          PPP_MNTN_LOG2(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL,"Chap Input: response (len <1>, alen <2>) without name\r\n",
                  len, alen);
        }

        if (PHASE_AUTHENTICATE == l->phase)    /* 需要注意只备份在认证阶段中与challenge id匹配的response */
        {
            ChapBufferResponsePacket(chap, MD5DIGESTSIZE, aucHashValue, len);
        }

        chap_Success(l);

        /*
           Moves code to here as the last step of dealing with response by liukai,
           it should stop authentication timer after authentication pass or fail.
           Stops timer at first, a response frame format is not correct and discards it(way of BSD),
           UE has no chance to send challenge again
        */
        auth_StopTimer(&(chap->auth));
    }    /* end of rx-ed response */
    else if (CHAP_SUCCESS == chap->auth.in.hdr.code)    /* rx-ed success */
    {
        /* chap->auth.in.name is already set up at CHALLENGE time, need NOT to print again */
        if (0 < len)
        {
          PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "Chap Input: success with message\r\n");
        }
        else
        {
          PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "Chap Input: success without message\r\n");
        }

        if (PROTO_CHAP == l->lcp.auth_iwait) {
          l->lcp.auth_iwait = 0;
          if (0 == l->lcp.auth_ineed)    /* auth_ineed: 0, authentication by peer is not complete or no need to authentication,
                                                       !0, authentication by peer is complete */
          {
            /*
             * We've succeeded in our ``login''
             * If we're not expecting  the peer to authenticate (or he already
             * has), proceed to network phase.
             */
            chap_ReInit(&(l->chap));
            if (PHASE_AUTHENTICATE == l->phase)
            {
                l->phase = PHASE_NETWORK;
                l->ipcp.fsm.state = ST_CLOSED;
                fsm_Open(&(l->ipcp.fsm));
                PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "goto ipcp stage!\r\n");
            }
          }
        }
    }    /* end of rx-ed success */
    else    /* rx-ed fail */
    {
        /* chap->auth.in.name is already set up at CHALLENGE time, need NOT to print again */
        if (0 < len)
        {
          PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "Chap Input: fail with message\r\n");
        }
        else
        {
          PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "Chap Input: fail without message\r\n");
        }

        chap_Cleanup(&(l->chap));
        l->phase = PHASE_TERMINATE;
        fsm_Close(&(l->lcp.fsm));
        PPP_MNTN_LOG(PS_PID_APP_PPP, 0, PS_PRINT_NORMAL, "goto lcp stage!\r\n");
    }    /* end of rx-ed fail */
  }

  ppp_m_init_data(bp);

  ppp_m_freem(bp);
  return VOS_NULL_PTR;
}    /* chap_Input */


