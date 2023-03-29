/*
* Copyright (C) Huawei Technologies Co., Ltd. 2012-2015. All rights reserved.
* foss@huawei.com
*
* If distributed as part of the Linux kernel, the following license terms
* apply:
*
* * This program is free software; you can redistribute it and/or modify
* * it under the terms of the GNU General Public License version 2 and
* * only version 2 as published by the Free Software Foundation.
* *
* * This program is distributed in the hope that it will be useful,
* * but WITHOUT ANY WARRANTY; without even the implied warranty of
* * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* * GNU General Public License for more details.
* *
* * You should have received a copy of the GNU General Public License
* * along with this program; if not, write to the Free Software
* * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA
*
* Otherwise, the following license terms apply:
*
* * Redistribution and use in source and binary forms, with or without
* * modification, are permitted provided that the following conditions
* * are met:
* * 1) Redistributions of source code must retain the above copyright
* *    notice, this list of conditions and the following disclaimer.
* * 2) Redistributions in binary form must reproduce the above copyright
* *    notice, this list of conditions and the following disclaimer in the
* *    documentation and/or other materials provided with the distribution.
* * 3) Neither the name of Huawei nor the names of its contributors may
* *    be used to endorse or promote products derived from this software
* *    without specific prior written permission.
*
* * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*
*/

/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "AdsDebug.h"
#include "AdsMntn.h"



/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID                 PS_FILE_ID_ADS_MNTN_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/


ADS_MNTN_UL_IP_PKT_REC_STRU                g_stAdsUlPktRecInfo = {0};
ADS_MNTN_DL_IP_PKT_REC_STRU                g_stAdsDlPktRecInfo = {0};

/*****************************************************************************
  3 函数实现
*****************************************************************************/


VOS_VOID ADS_MNTN_TransReport(
    VOS_UINT32                          ulMsgId,
    VOS_VOID                           *pData,
    VOS_UINT32                          ulLen
)
{
    DIAG_TRANS_IND_STRU                 stDiagTransInd;

    stDiagTransInd.ulModule  = ADS_MNTN_COMM_MOUDLE_ID;
    stDiagTransInd.ulPid     = ACPU_PID_ADS_UL;
    stDiagTransInd.ulMsgId   = ulMsgId;
    stDiagTransInd.ulReserve = 0;
    stDiagTransInd.ulLength  = ulLen;
    stDiagTransInd.pData     = pData;

    (VOS_VOID)DIAG_TransReport(&stDiagTransInd);
    return;
}


VOS_VOID ADS_MNTN_SndULIpfProcStatsInd(VOS_VOID)
{
    ADS_MNTN_UL_IPF_PROC_STATS_STRU     stStats;

    stStats.stCommHeader.ucVer          = 100;
    stStats.stCommHeader.ucReserved     = 0;
    stStats.stCommHeader.usReserved0    = 0;
    stStats.stCommHeader.usReserved1    = 0;
    stStats.stCommHeader.usReserved2    = 0;

    stStats.ulULQueNonEmptyTrigEvent    = g_stAdsStats.stUlComStatsInfo.ulULQueNonEmptyTrigEvent;
    stStats.ulULQueFullTrigEvent        = g_stAdsStats.stUlComStatsInfo.ulULQueFullTrigEvent;
    stStats.ulULQueHitThresTrigEvent    = g_stAdsStats.stUlComStatsInfo.ulULQueHitThresTrigEvent;
    stStats.ulULTmrHitThresTrigEvent    = g_stAdsStats.stUlComStatsInfo.ulULTmrHitThresTrigEvent;
    stStats.ulUL10MsTmrTrigEvent        = g_stAdsStats.stUlComStatsInfo.ulUL10MsTmrTrigEvent;
    stStats.ulULSpeIntTrigEvent         = g_stAdsStats.stUlComStatsInfo.ulULSpeIntTrigEvent;
    stStats.ulULProcEventNum            = g_stAdsStats.stUlComStatsInfo.ulULProcEventNum;

    stStats.ulULBdqCfgIpfHaveNoBd       = g_stAdsStats.stUlComStatsInfo.ulULBdqCfgIpfHaveNoBd;
    stStats.ulULBdqCfgBdSuccNum         = g_stAdsStats.stUlComStatsInfo.ulULBdqCfgBdSuccNum;
    stStats.ulULBdqCfgBdFailNum         = g_stAdsStats.stUlComStatsInfo.ulULBdqCfgBdFailNum;
    stStats.ulULBdqCfgIpfSuccNum        = g_stAdsStats.stUlComStatsInfo.ulULBdqCfgIpfSuccNum;
    stStats.ulULBdqCfgIpfFailNum        = g_stAdsStats.stUlComStatsInfo.ulULBdqCfgIpfFailNum;
    stStats.ulULBdqSaveSrcMemNum        = g_stAdsStats.stUlComStatsInfo.ulULBdqSaveSrcMemNum;
    stStats.ulULBdqFreeSrcMemNum        = g_stAdsStats.stUlComStatsInfo.ulULBdqFreeSrcMemNum;
    stStats.ulULBdqFreeSrcMemErr        = g_stAdsStats.stUlComStatsInfo.ulULBdqFreeSrcMemErr;

    stStats.ulULBuffThresholdCurrent    = g_stAdsCtx.stAdsIpfCtx.ulThredHoldNum;

    stStats.ulULBuffThreshold1          = g_stAdsCtx.stAdsIpfCtx.stUlAssemParmInfo.stThresholdLevel.ulThreshold1;
    stStats.ulULBuffThreshold2          = g_stAdsCtx.stAdsIpfCtx.stUlAssemParmInfo.stThresholdLevel.ulThreshold2;
    stStats.ulULBuffThreshold3          = g_stAdsCtx.stAdsIpfCtx.stUlAssemParmInfo.stThresholdLevel.ulThreshold3;
    stStats.ulULBuffThreshold4          = g_stAdsCtx.stAdsIpfCtx.stUlAssemParmInfo.stThresholdLevel.ulThreshold4;

    stStats.ulULWmLevel1HitNum          = g_stAdsStats.stUlComStatsInfo.ulULWmLevel1HitNum;
    stStats.ulULWmLevel2HitNum          = g_stAdsStats.stUlComStatsInfo.ulULWmLevel2HitNum;
    stStats.ulULWmLevel3HitNum          = g_stAdsStats.stUlComStatsInfo.ulULWmLevel3HitNum;
    stStats.ulULWmLevel4HitNum          = g_stAdsStats.stUlComStatsInfo.ulULWmLevel4HitNum;

    ADS_MNTN_TransReport(ID_DIAG_ADS_UL_IPF_PROC_STATS_IND,
                        (VOS_VOID *)&stStats,
                        sizeof(ADS_MNTN_UL_IPF_PROC_STATS_STRU));

    return;
}


VOS_VOID ADS_MNTN_SndDLIpfProcStatsInd(VOS_VOID)
{
    ADS_MNTN_DL_IPF_PROC_STATS_STRU     stStats;

    stStats.stCommHeader.ucVer          = 100;
    stStats.stCommHeader.ucReserved     = 0;
    stStats.stCommHeader.usReserved0    = 0;
    stStats.stCommHeader.usReserved1    = 0;
    stStats.stCommHeader.usReserved2    = 0;

    stStats.ulDLRcvIpfRdIntNum          = g_stAdsStats.stDlComStatsInfo.ulDLRcvIpfRdIntNum;
    stStats.ulDLProcIpfRdEventNum       = g_stAdsStats.stDlComStatsInfo.ulDLProcIpfRdEventNum;
    stStats.ulDLRcvIpfAdqEmptyIntNum    = g_stAdsStats.stDlComStatsInfo.ulDLRcvIpfAdqEmptyIntNum;
    stStats.ulDLProcIpfAdEventNum       = g_stAdsStats.stDlComStatsInfo.ulDLProcIpfAdEventNum;
    stStats.ulDLCCoreResetTrigEvent     = g_stAdsStats.stDlComStatsInfo.ulDLCCoreResetTrigEvent;

    stStats.ulDLRdqRxRdNum              = g_stAdsStats.stDlComStatsInfo.ulDLRdqRxRdNum;
    stStats.ulDLRdqGetRd0Num            = g_stAdsStats.stDlComStatsInfo.ulDLRdqGetRd0Num;
    stStats.ulDLRdqTransMemFailNum      = g_stAdsStats.stDlComStatsInfo.ulDLRdqTransMemFailNum;
    stStats.ulDLRdqRxNormPktNum         = g_stAdsStats.stDlComStatsInfo.ulDLRdqRxNormPktNum;
    stStats.ulDLRdqRxNdPktNum           = g_stAdsStats.stDlComStatsInfo.ulDLRdqRxNdPktNum;
    stStats.ulDLRdqRxDhcpPktNum         = g_stAdsStats.stDlComStatsInfo.ulDLRdqRxDhcpPktNum;
    stStats.ulDLRdqRxErrPktNum          = g_stAdsStats.stDlComStatsInfo.ulDLRdqRxErrPktNum;
    stStats.ulDLRdqFilterErrNum         = g_stAdsStats.stDlComStatsInfo.ulDLRdqFilterErrNum;

    stStats.ulDLAdqAllocSysMemSuccNum   = g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocSysMemSuccNum;
    stStats.ulDLAdqAllocSysMemFailNum   = g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocSysMemFailNum;
    stStats.ulDLAdqAllocMemSuccNum      = g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocMemSuccNum;
    stStats.ulDLAdqAllocMemFailNum      = g_stAdsStats.stDlComStatsInfo.ulDLAdqAllocMemFailNum;
    stStats.ulDLAdqFreeMemNum           = g_stAdsStats.stDlComStatsInfo.ulDLAdqFreeMemNum;
    stStats.ulDLAdqRecycleMemSuccNum    = g_stAdsStats.stDlComStatsInfo.ulDLAdqRecycleMemSuccNum;
    stStats.ulDLAdqRecycleMemFailNum    = g_stAdsStats.stDlComStatsInfo.ulDLAdqRecycleMemFailNum;
    stStats.ulDLAdqGetFreeAdSuccNum     = g_stAdsStats.stDlComStatsInfo.ulDLAdqGetFreeAdSuccNum;
    stStats.ulDLAdqGetFreeAdFailNum     = g_stAdsStats.stDlComStatsInfo.ulDLAdqGetFreeAdFailNum;
    stStats.ulDLAdqCfgAdNum             = g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgAdNum;
    stStats.ulDLAdqCfgAd0Num            = g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgAd0Num;
    stStats.ulDLAdqCfgAd1Num            = g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgAd1Num;
    stStats.ulDLAdqCfgIpfSuccNum        = g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgIpfSuccNum;
    stStats.ulDLAdqCfgIpfFailNum        = g_stAdsStats.stDlComStatsInfo.ulDLAdqCfgIpfFailNum;
    stStats.ulDLAdqStartEmptyTmrNum     = g_stAdsStats.stDlComStatsInfo.ulDLAdqStartEmptyTmrNum;
    stStats.ulDLAdqEmptyTmrTimeoutNum   = g_stAdsStats.stDlComStatsInfo.ulDLAdqEmptyTmrTimeoutNum;
    stStats.ulDLAdqRcvAd0EmptyIntNum    = g_stAdsStats.stDlComStatsInfo.ulDLAdqRcvAd0EmptyIntNum;
    stStats.ulDLAdqRcvAd1EmptyIntNum    = g_stAdsStats.stDlComStatsInfo.ulDLAdqRcvAd1EmptyIntNum;

    ADS_MNTN_TransReport(ID_DIAG_ADS_DL_IPF_PROC_STATS_IND,
                        (VOS_VOID *)&stStats,
                        sizeof(ADS_MNTN_DL_IPF_PROC_STATS_STRU));

    return;
}


VOS_VOID ADS_MNTN_SndULPktProcStatsInd(VOS_VOID)
{
    ADS_MNTN_UL_PKT_PROC_STATS_STRU     stStats;

    stStats.stCommHeader.ucVer          = 100;
    stStats.stCommHeader.ucReserved     = 0;
    stStats.stCommHeader.usReserved0    = 0;
    stStats.stCommHeader.usReserved1    = 0;
    stStats.stCommHeader.usReserved2    = 0;

    stStats.ulULRmnetRxPktNum           = g_stAdsStats.stUlComStatsInfo.ulULRmnetRxPktNum;
    stStats.ulULRmnetModemIdErrNum      = g_stAdsStats.stUlComStatsInfo.ulULRmnetModemIdErrNum;
    stStats.ulULRmnetRabIdErrNum        = g_stAdsStats.stUlComStatsInfo.ulULRmnetRabIdErrNum;
    stStats.ulULRmnetEnQueSuccNum       = g_stAdsStats.stUlComStatsInfo.ulULRmnetEnQueSuccNum;
    stStats.ulULRmnetEnQueFailNum       = g_stAdsStats.stUlComStatsInfo.ulULRmnetEnQueFailNum;
    stStats.ulULPktEnQueSuccNum         = g_stAdsStats.stUlComStatsInfo.ulULPktEnQueSuccNum;
    stStats.ulULPktEnQueFailNum         = g_stAdsStats.stUlComStatsInfo.ulULPktEnQueFailNum;

    ADS_MNTN_TransReport(ID_DIAG_ADS_UL_PKT_PROC_STATS_IND,
                         (VOS_VOID *)&stStats,
                         sizeof(ADS_MNTN_UL_PKT_PROC_STATS_STRU));

    return;
}


VOS_VOID ADS_MNTN_SndDLPktProcStatsInd(VOS_VOID)
{
    ADS_MNTN_DL_PKT_PROC_STATS_STRU     stStats;

    stStats.stCommHeader.ucVer          = 100;
    stStats.stCommHeader.ucReserved     = 0;
    stStats.stCommHeader.usReserved0    = 0;
    stStats.stCommHeader.usReserved1    = 0;
    stStats.stCommHeader.usReserved2    = 0;

    stStats.ulDLRmnetTxPktNum           = g_stAdsStats.stDlComStatsInfo.ulDLRmnetTxPktNum;
    stStats.ulDLRmnetNoFuncFreePktNum   = g_stAdsStats.stDlComStatsInfo.ulDLRmnetNoFuncFreePktNum;
    stStats.ulDLRmnetRabIdErrNum        = g_stAdsStats.stDlComStatsInfo.ulDLRmnetRabIdErrNum;

    ADS_MNTN_TransReport(ID_DIAG_ADS_DL_PKT_PROC_STATS_IND,
                        (VOS_VOID *)&stStats,
                        sizeof(ADS_MNTN_DL_PKT_PROC_STATS_STRU));

    return;
}


VOS_VOID ADS_MNTN_SndThroughputStatsInd(VOS_VOID)
{
    ADS_MNTN_THROUGHPUT_STATS_STRU      stStats;

    stStats.stCommHeader.ucVer          = 100;
    stStats.stCommHeader.ucReserved     = 0;
    stStats.stCommHeader.usReserved0    = 0;
    stStats.stCommHeader.usReserved1    = 0;
    stStats.stCommHeader.usReserved2    = 0;

    stStats.ulULDataRate                = g_stAdsCtx.stDsFlowStatsCtx.stULDataStats.ulULCurDataRate << 3;
    stStats.ulDLDataRate                = g_stAdsCtx.stDsFlowStatsCtx.stDLDataStats.ulDLCurDataRate << 3;

    ADS_MNTN_TransReport(ID_DIAG_ADS_THROUGHPUT_STATS_IND,
                        (VOS_VOID *)&stStats,
                        sizeof(ADS_MNTN_THROUGHPUT_STATS_STRU));

    return;
}


VOS_VOID ADS_MNTN_HookIpPkt(
    IMM_ZC_STRU                        *pstImmZc,
    VOS_UINT16                          usMsgType,
    VOS_UINT32                          ulParam1,
    VOS_UINT32                          ulParam2,
    VOS_UINT32                          ulParam3,
    VOS_UINT32                          ulParam4
)
{
    VOS_UINT32                          ulDataLen;
    VOS_UINT32                          ulCacheLen;

    if (VOS_FALSE == IPS_MNTN_GetIPInfoCfg(usMsgType))
    {
        return;
    }

    ulDataLen  = IMM_ZcGetUsedLen(pstImmZc);
    ulCacheLen = (ulDataLen < ADS_MNTN_HOOK_PKT_MAX_SIZE) ?
                    (IMM_MAC_HEADER_RES_LEN + ulDataLen) :
                    (IMM_MAC_HEADER_RES_LEN + ADS_MNTN_HOOK_PKT_MAX_SIZE);

    ADS_IPF_SPE_MEM_UNMAP(pstImmZc, ulCacheLen);

    IPS_MNTN_TraceIpInfo(pstImmZc, usMsgType,
                         ulParam1, ulParam2, ulParam3, ulParam4);

    ADS_IPF_SPE_MEM_MAP(pstImmZc, ulCacheLen);
    return;
}


VOS_VOID ADS_MNTN_ReportAllStatsInfo(VOS_VOID)
{
    ADS_MNTN_SndULIpfProcStatsInd();
    ADS_MNTN_SndDLIpfProcStatsInd();
    ADS_MNTN_SndULPktProcStatsInd();
    ADS_MNTN_SndDLPktProcStatsInd();
    ADS_MNTN_SndThroughputStatsInd();

    return;
}



VOS_VOID ADS_MNTN_ReportULPktInfo(VOS_VOID)
{
    ADS_MNTN_UL_IP_PKT_REC_STRU        *pstRecStru   = VOS_NULL_PTR;
    VOS_UINT16                          usReportSize = 0;

    pstRecStru = ADS_MNTN_UL_RKT_REC_INFO_ARRAY;

    /*没有数据不上报*/
    if (0 == pstRecStru->ulRptNum)
    {
        return;
    }

    usReportSize = (VOS_UINT16)(sizeof(ADS_MNTN_IP_PKT_INFO_STRU) * (pstRecStru->ulRptNum) + (sizeof(VOS_UINT8) * 4) + sizeof(VOS_UINT32));

    pstRecStru->ucVer = 100;

    ADS_MNTN_TransReport(ID_DIAG_ADS_UL_PKT_INFO_STATS_IND,
                        (VOS_VOID *)pstRecStru,
                         usReportSize);

    pstRecStru->ulRptNum = 0;

    return;
}


VOS_VOID ADS_MNTN_ReportDLPktInfo(VOS_VOID)
{
    ADS_MNTN_DL_IP_PKT_REC_STRU        *pstRecStru   = VOS_NULL_PTR;
    VOS_UINT16                          usReportSize = 0;

    pstRecStru = ADS_MNTN_DL_RKT_REC_INFO_ARRAY;

    /*没有数据不上报*/
    if (0 == pstRecStru->ulRptNum)
    {
        return;
    }

    usReportSize = (VOS_UINT16)(sizeof(ADS_MNTN_IP_PKT_INFO_STRU) * (pstRecStru->ulRptNum) + (sizeof(VOS_UINT8) * 4) + sizeof(VOS_UINT32));

    pstRecStru->ucVer = 100;

    ADS_MNTN_TransReport(ID_DIAG_ADS_DL_PKT_INFO_STATS_IND,
                        (VOS_VOID *)pstRecStru,
                         usReportSize);

    pstRecStru->ulRptNum = 0;

    return;
}


VOS_VOID ADS_MNTN_DecodeL4PktInfo(
    VOS_UINT8                          *pucIpPkt,
    ADS_MNTN_IP_PKT_INFO_STRU          *pstPktInfo
)
{
    ADS_TCP_HDR_STRU                  *pstTcpHdr  = VOS_NULL_PTR;
    ADS_UDP_HDR_STRU                  *pstUdpHdr  = VOS_NULL_PTR;
    ADS_ICMP_HDR_STRU                 *pstIcmpHdr = VOS_NULL_PTR;

    switch (pstPktInfo->ucL4Proto)
    {
        case ADS_IP_PROTO_TCP:
            pstTcpHdr                   = (ADS_TCP_HDR_STRU *)pucIpPkt;
            pstPktInfo->usSrcPort       = VOS_NTOHS(pstTcpHdr->usSrcPort);
            pstPktInfo->usDstPort       = VOS_NTOHS(pstTcpHdr->usDstPort);
            pstPktInfo->ulL4Id          = VOS_NTOHL(pstTcpHdr->ulSeqNum);
            pstPktInfo->ulTcpAckSeq     = VOS_NTOHL(pstTcpHdr->ulAckNum);
            break;

        case ADS_IP_PROTO_UDP:
            pstUdpHdr                   = (ADS_UDP_HDR_STRU *)pucIpPkt;
            pstPktInfo->usSrcPort       = VOS_NTOHS(pstUdpHdr->usSrcPort);
            pstPktInfo->usDstPort       = VOS_NTOHS(pstUdpHdr->usDstPort);
            break;

        case ADS_IPV4_PROTO_ICMP:
        case ADS_IPV6_PROTO_ICMP:
            pstIcmpHdr                  = (ADS_ICMP_HDR_STRU *)pucIpPkt;
            pstPktInfo->ucIcmpType      = pstIcmpHdr->ucType;
            if ((ADS_IPV4_ICMP_ECHO_REQUEST == pstPktInfo->ucIcmpType)
              ||(ADS_IPV4_ICMP_ECHO_REPLY   == pstPktInfo->ucIcmpType)
              ||(ADS_IPV6_ICMP_ECHO_REQUEST == pstPktInfo->ucIcmpType)
              ||(ADS_IPV6_ICMP_ECHO_REPLY   == pstPktInfo->ucIcmpType))
            {
                pstPktInfo->ulL4Id      = VOS_NTOHS(pstIcmpHdr->unIcmp.stIcmpEcho.usIdentifier);
                pstPktInfo->ulTcpAckSeq = VOS_NTOHS(pstIcmpHdr->unIcmp.stIcmpEcho.usSeqNum);
            }
            break;

        default:
            pstPktInfo->ulL4Id          = VOS_NTOHL(((VOS_UINT32 *)pucIpPkt)[0]);
            pstPktInfo->ulTcpAckSeq     = VOS_NTOHL(((VOS_UINT32 *)pucIpPkt)[1]);
            break;
    }

    return;
}


VOS_VOID ADS_MNTN_DecodeIpv6PktInfo(
    VOS_UINT8                          *pucIpPkt,
    ADS_MNTN_IP_PKT_INFO_STRU          *pstPktInfo
)
{
    ADS_IPV6_HDR_STRU                 *pstIPv6Hdr = VOS_NULL_PTR;

    pstIPv6Hdr             = (ADS_IPV6_HDR_STRU *)pucIpPkt;
    pstPktInfo->ucIpVer    = ADS_IP_VERSION_V6;
    pstPktInfo->ucL4Proto  = pstIPv6Hdr->ucNextHdr;
    pstPktInfo->usIp4Id    = 0;
    pstPktInfo->usDataLen  = VOS_NTOHS(pstIPv6Hdr->usPayloadLen) + ADS_IPV6_HDR_LEN;

    pucIpPkt              += ADS_IPV6_HDR_LEN;
    ADS_MNTN_DecodeL4PktInfo(pucIpPkt, pstPktInfo);

    return;
}


VOS_VOID ADS_MNTN_DecodeIpv4PktInfo(
    VOS_UINT8                          *pucIpPkt,
    ADS_MNTN_IP_PKT_INFO_STRU          *pstPktInfo
)
{
    ADS_IPV4_HDR_STRU                 *pstIPv4Hdr = VOS_NULL_PTR;

    pstIPv4Hdr             = (ADS_IPV4_HDR_STRU *)pucIpPkt;
    pstPktInfo->ucIpVer    = ADS_IP_VERSION_V4;
    pstPktInfo->ucL4Proto  = pstIPv4Hdr->ucProtocol;
    pstPktInfo->usIp4Id    = VOS_NTOHS(pstIPv4Hdr->usIdentification);
    pstPktInfo->usDataLen  = VOS_NTOHS(pstIPv4Hdr->usTotalLen);

    pucIpPkt              += ADS_IPV4_HDR_LEN;
    ADS_MNTN_DecodeL4PktInfo(pucIpPkt, pstPktInfo);

    return;
}


VOS_VOID ADS_MNTN_DecodeIpPktInfo(
    VOS_UINT8                          *pucIpPkt,
    ADS_MNTN_IP_PKT_INFO_STRU          *pstPktInfo
)
{
    if (ADS_IP_VERSION_V4 == ADS_GET_IP_VERSION(pucIpPkt))
    {
        ADS_MNTN_DecodeIpv4PktInfo(pucIpPkt, pstPktInfo);
    }
    else
    {
        ADS_MNTN_DecodeIpv6PktInfo(pucIpPkt, pstPktInfo);
    }

    return;
}


VOS_VOID ADS_MNTN_RecULIpPktInfo(
    IMM_ZC_STRU                        *pstImmZc,
    VOS_UINT32                          ulParam1,
    VOS_UINT32                          ulParam2,
    VOS_UINT32                          ulParam3
)
{
    ADS_MNTN_UL_IP_PKT_REC_STRU        *pstRecStru = VOS_NULL_PTR;
    ADS_MNTN_IP_PKT_INFO_STRU          *pstPktInfo = VOS_NULL_PTR;
    VOS_UINT32                          ulRet;
    VOS_UINT32                          ulDataLen;
    VOS_UINT32                          ulCacheLen;

    /* HIDS未连接 */
    if (VOS_NO == DIAG_GetConnState())
    {
        return;
    }

    ulRet = IPS_MNTN_GetIPInfoCfg(ID_IPS_TRACE_ADS_UL);
    if (PS_FALSE == ulRet)
    {
        /*不捕获该报文*/
        return;
    }

    pstRecStru = ADS_MNTN_UL_RKT_REC_INFO_ARRAY;

    if (pstRecStru->ulRptNum >= ADS_MNTN_REC_UL_PKT_MAX_NUM)
    {
        return;
    }

    pstPktInfo           = &(pstRecStru->astIpPktRecInfo[pstRecStru->ulRptNum]);
    pstPktInfo->ulParam1 = ulParam1;
    pstPktInfo->ulParam2 = ulParam2;
    pstPktInfo->ulParam3 = ulParam3;

    ulDataLen  = IMM_ZcGetUsedLen(pstImmZc);
    ulCacheLen = (ulDataLen < ADS_MNTN_HOOK_PKT_MAX_SIZE) ?
                    (IMM_MAC_HEADER_RES_LEN + ulDataLen) :
                    (IMM_MAC_HEADER_RES_LEN + ADS_MNTN_HOOK_PKT_MAX_SIZE);

    ADS_IPF_SPE_MEM_UNMAP(pstImmZc, ulCacheLen);
    ADS_MNTN_DecodeIpPktInfo(IMM_ZcGetDataPtr(pstImmZc), pstPktInfo);
    ADS_IPF_SPE_MEM_MAP(pstImmZc, ulCacheLen);

    (pstRecStru->ulRptNum)++;
    if (pstRecStru->ulRptNum >= ADS_MNTN_REC_UL_PKT_MAX_NUM)
    {
        ADS_MNTN_ReportULPktInfo();
        pstRecStru->ulRptNum = 0;
    }

    return;
}


VOS_VOID ADS_MNTN_RecDLIpPktInfo(
    IMM_ZC_STRU                        *pstImmZc,
    VOS_UINT32                          ulParam1,
    VOS_UINT32                          ulParam2,
    VOS_UINT32                          ulParam3
)
{
    ADS_MNTN_DL_IP_PKT_REC_STRU        *pstRecStru = VOS_NULL_PTR;
    ADS_MNTN_IP_PKT_INFO_STRU          *pstPktInfo = VOS_NULL_PTR;
    VOS_UINT32                          ulRet;
    VOS_UINT32                          ulDataLen;
    VOS_UINT32                          ulCacheLen;

    /* HIDS未连接 */
    if (VOS_NO == DIAG_GetConnState())
    {
        return;
    }

    ulRet = IPS_MNTN_GetIPInfoCfg(ID_IPS_TRACE_ADS_DL);
    if(PS_FALSE == ulRet)
    {
        /*不捕获该报文*/
        return;
    }

    pstRecStru = ADS_MNTN_DL_RKT_REC_INFO_ARRAY;

    if (pstRecStru->ulRptNum >= ADS_MNTN_REC_DL_PKT_MAX_NUM)
    {
        return;
    }

    pstPktInfo           = &(pstRecStru->astIpPktRecInfo[pstRecStru->ulRptNum]);
    pstPktInfo->ulParam1 = ulParam1;
    pstPktInfo->ulParam2 = ulParam2;
    pstPktInfo->ulParam3 = ulParam3;

    ulDataLen  = IMM_ZcGetUsedLen(pstImmZc);
    ulCacheLen = (ulDataLen < ADS_MNTN_HOOK_PKT_MAX_SIZE) ?
                    (IMM_MAC_HEADER_RES_LEN + ulDataLen) :
                    (IMM_MAC_HEADER_RES_LEN + ADS_MNTN_HOOK_PKT_MAX_SIZE);

    ADS_IPF_SPE_MEM_UNMAP(pstImmZc, ulCacheLen);
    ADS_MNTN_DecodeIpPktInfo(IMM_ZcGetDataPtr(pstImmZc), pstPktInfo);
    ADS_IPF_SPE_MEM_MAP(pstImmZc, ulCacheLen);

    (pstRecStru->ulRptNum)++;
    if (pstRecStru->ulRptNum >= ADS_MNTN_REC_DL_PKT_MAX_NUM)
    {
        ADS_MNTN_ReportDLPktInfo();
        pstRecStru->ulRptNum = 0;
    }

    return;
}


