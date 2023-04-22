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
**************************************************************************** */
#include "SCMProc.h"
#include "SCMSoftDecode.h"
#include "OmHdlcInterface.h"
#include "ombufmngr.h"


#define    THIS_FILE_ID        PS_FILE_ID_SCM_SOFT_DECODE_C

/* ****************************************************************************
  2 全局变量定义
**************************************************************************** */
/* 自旋锁，用来作OM数据接收的临界资源保护 */
VOS_SPINLOCK             g_stScmSoftDecodeDataRcvSpinLock;

/* HDLC控制结构 */
OM_HDLC_STRU             g_stScmHdlcSoftDecodeEntity;

/* SCM数据接收数据缓冲区 */
VOS_CHAR                 g_aucSCMDataRcvBuffer[SCM_DATA_RCV_PKT_SIZE];

/* SCM数据接收任务控制结构 */
SCM_DATA_RCV_CTRL_STRU   g_stSCMDataRcvTaskCtrlInfo;

SCM_SOFTDECODE_INFO_STRU   g_stScmSoftDecodeInfo;

/*****************************************************************************
  3 外部引用声明
*****************************************************************************/

VOS_UINT32 SCM_SoftDecodeAcpuRcvData(
    OM_HDLC_STRU                       *pstHdlcCtrl,
    VOS_UINT8                          *pucData,
    VOS_UINT32                          ulLen);


VOS_UINT32 SCM_SoftDecodeCfgHdlcInit(OM_HDLC_STRU *pstHdlc);

/*****************************************************************************
  4 函数实现
*****************************************************************************/


VOS_UINT32 SCM_SoftDecodeCfgDataRcv(VOS_UINT8 *pucBuffer, VOS_UINT32 ulLen)
{
    VOS_UINT32                          ulRstl;
    VOS_ULONG                           ulLockLevel;

    VOS_SpinLockIntLock(&g_stScmSoftDecodeDataRcvSpinLock, ulLockLevel);

    ulRstl = SCM_SoftDecodeDataRcv(pucBuffer, ulLen);

    VOS_SpinUnlockIntUnlock(&g_stScmSoftDecodeDataRcvSpinLock, ulLockLevel);

    return ulRstl;
}


VOS_UINT32 SCM_SoftDecodeDataRcv(VOS_UINT8 *pucBuffer, VOS_UINT32 ulLen)
{
    VOS_INT32                           sRet;

    diag_PTR(EN_DIAG_PTR_SCM_SOFTDECODE);

    if (ulLen > (VOS_UINT32)OM_RingBufferFreeBytes(g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId))
    {
        g_stScmSoftDecodeInfo.stRbInfo.ulBufferNotEnough++;
        diag_PTR(EN_DIAG_PTR_SCM_ERR1);

        return VOS_ERR;
    }

    sRet = OM_RingBufferPut(g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId,
                            (VOS_CHAR *)pucBuffer,
                            (VOS_INT)ulLen);

    if (ulLen == (VOS_UINT32)sRet)
    {
        if (VOS_OK == VOS_SmV(g_stSCMDataRcvTaskCtrlInfo.SmID))
        {
            g_stScmSoftDecodeInfo.stPutInfo.ulDataLen += sRet;
            g_stScmSoftDecodeInfo.stPutInfo.ulNum++;

            return VOS_OK;
        }
    }

    g_stScmSoftDecodeInfo.stRbInfo.ulRingBufferPutErr++;
    diag_PTR(EN_DIAG_PTR_SCM_ERR2);

    return VOS_ERR;
}


VOS_VOID SCM_RcvDataDispatch(
    OM_HDLC_STRU                       *pstHdlcCtrl,
    VOS_UINT8                           ucDataType)
{
    /* TL数据 */
    if (SCM_DATA_TYPE_TL == ucDataType)
    {
        if (VOS_NULL_PTR != g_astSCMDecoderCbFunc[SOCP_DECODER_DST_CB_TL_OM])
        {
            diag_PTR(EN_DIAG_PTR_SCM_DISPATCH);

            /* TL不需要DATATYPE字段，回调时删除 */
            g_astSCMDecoderCbFunc[SOCP_DECODER_DST_CB_TL_OM](SOCP_DECODER_DST_LOM,
                                                    pstHdlcCtrl->pucDecapBuff + sizeof(SOCP_DATA_TYPE_ENUM_UIN8),
                                                    pstHdlcCtrl->ulInfoLen - sizeof(SOCP_DATA_TYPE_ENUM_UIN8),
                                                    VOS_NULL_PTR,
                                                    VOS_NULL);
        }

        return;
    }

    return;
}


VOS_UINT32 SCM_SoftDecodeAcpuRcvData(
    OM_HDLC_STRU                       *pstHdlcCtrl,
    VOS_UINT8                          *pucData,
    VOS_UINT32                          ulLen)
{
    VOS_UINT32                          i;
    VOS_UINT32                          ulResult;
    VOS_UINT8                           ucGutlType;
    VOS_UINT8                           ucChar;

    ulResult = VOS_ERR;

    for( i = 0; i < ulLen; i++ )
    {
        ucChar = (VOS_UINT8)pucData[i];

        ulResult = Om_HdlcDecap(pstHdlcCtrl, ucChar);

        if ( HDLC_SUCC == ulResult )
        {
            g_stScmSoftDecodeInfo.stHdlcDecapData.ulDataLen += pstHdlcCtrl->ulInfoLen;
            g_stScmSoftDecodeInfo.stHdlcDecapData.ulNum++;

            ucGutlType = pstHdlcCtrl->pucDecapBuff[0];

            diag_PTR(EN_DIAG_PTR_SCM_RCVDATA_SUCCESS);

            SCM_RcvDataDispatch(pstHdlcCtrl, ucGutlType);
        }
        else if (HDLC_NOT_HDLC_FRAME == ulResult)
        {
            /*不是完整分帧,继续HDLC解封装*/
        }
        else
        {
            g_stScmSoftDecodeInfo.ulFrameDecapErr++;
        }
    }

    return VOS_OK;
}


VOS_UINT32 SCM_SoftDecodeCfgHdlcInit(OM_HDLC_STRU *pstHdlc)
{
    /* 申请用于HDLC解封装的缓存 */
    pstHdlc->pucDecapBuff    = (VOS_UINT8 *)VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT, STATIC_MEM_PT, SCM_DATA_RCV_PKT_SIZE);

    if (VOS_NULL_PTR == pstHdlc->pucDecapBuff)
    {
        (VOS_VOID)vos_printf("SCM_SoftDecodeCfgHdlcInit: Alloc Decapsulate buffer fail!.\n");

        return VOS_ERR;
    }

    /* HDLC解封装缓存长度赋值 */
    pstHdlc->ulDecapBuffSize = SCM_DATA_RCV_PKT_SIZE;

    /* 初始化HDLC解封装控制上下文 */
    Om_HdlcInit(pstHdlc);

    return VOS_OK;
}


VOS_VOID SCM_SoftDecodeCfgRcvSelfTask(VOS_VOID)
{
    VOS_INT32                           sRet;
    VOS_INT32                           lLen;
    VOS_INT32                           lRemainlen;
    VOS_INT32                           lReadLen;
    VOS_UINT32                          ulPktNum;
    VOS_UINT32                          i;
    VOS_ULONG                           ulLockLevel;

    for (;;)
    {
        if (VOS_OK != VOS_SmP(g_stSCMDataRcvTaskCtrlInfo.SmID, 0))
        {
            continue;
        }

        diag_PTR(EN_DIAG_PTR_SCM_SELFTASK);

        lLen = OM_RingBufferNBytes(g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId);

        if (lLen <= 0)
        {
            continue;
        }

        ulPktNum = (VOS_UINT32)((lLen + SCM_DATA_RCV_PKT_SIZE - 1) / SCM_DATA_RCV_PKT_SIZE);

        lRemainlen = lLen;

        for (i = 0; i < ulPktNum; i++)
        {
            if (SCM_DATA_RCV_PKT_SIZE < lRemainlen)
            {
                lReadLen = SCM_DATA_RCV_PKT_SIZE;

                sRet = OM_RingBufferGet(g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId,
                                        g_stSCMDataRcvTaskCtrlInfo.pucBuffer,
                                        SCM_DATA_RCV_PKT_SIZE);
            }
            else
            {
                lReadLen = lRemainlen;

                sRet = OM_RingBufferGet(g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId,
                                        g_stSCMDataRcvTaskCtrlInfo.pucBuffer,
                                        lRemainlen);
            }

            if (sRet != lReadLen)
            {
                VOS_SpinLockIntLock(&g_stScmSoftDecodeDataRcvSpinLock, ulLockLevel);

                OM_RingBufferFlush(g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId);

                VOS_SpinUnlockIntUnlock(&g_stScmSoftDecodeDataRcvSpinLock, ulLockLevel);

                g_stScmSoftDecodeInfo.stRbInfo.ulRingBufferFlush++;
                diag_PTR(EN_DIAG_PTR_SCM_ERR3);

                continue;
            }

            lRemainlen -= lReadLen;

            g_stScmSoftDecodeInfo.stGetInfo.ulDataLen += lReadLen;

            diag_PTR(EN_DIAG_PTR_SCM_RCVDATA);

            /* 调用HDLC解封装函数 */
            if (VOS_OK != SCM_SoftDecodeAcpuRcvData(&g_stScmHdlcSoftDecodeEntity,
                                                    (VOS_UINT8 *)g_stSCMDataRcvTaskCtrlInfo.pucBuffer,
                                                    (VOS_UINT32)lReadLen))
            {
                (VOS_VOID)vos_printf("SCM_SoftDecodeCfgRcvSelfTask: SCM_SoftDecodeAcpuRcvData Fail.\n");
            }
        }

    
    /* coverity[loop_bottom] */
    }
}



VOS_UINT32 SCM_SoftDecodeCfgRcvTaskInit(VOS_VOID)
{
    VOS_UINT32                              ulRslt;

    if (VOS_OK != VOS_SmCCreate("OMCF", 0, VOS_SEMA4_FIFO, &(g_stSCMDataRcvTaskCtrlInfo.SmID)))
    {
        (VOS_VOID)vos_printf("SCM_SoftDecodeCfgRcvTaskInit: Error, OMCFG semCCreate Fail.\n");

        g_stScmSoftDecodeInfo.stRbInfo.ulSemCreatErr++;

        return VOS_ERR;
    }

    /* 注册OM配置数据接收自处理任务 */
    ulRslt = VOS_RegisterSelfTaskPrio(MSP_FID_DIAG_ACPU,
                                      (VOS_TASK_ENTRY_TYPE)SCM_SoftDecodeCfgRcvSelfTask,
                                      SCM_DATA_RCV_SELFTASK_PRIO,
                                      8096);
    if ( VOS_NULL_BYTE == ulRslt )
    {
        vos_printf("SCM_SoftDecodeCfgRcvTaskInit: VOS_RegisterSelfTaskPrio Fail.\n");
        return VOS_ERR;
    }

    (VOS_VOID)VOS_MemSet_s(&g_stScmSoftDecodeInfo, sizeof(g_stScmSoftDecodeInfo),0, sizeof(SCM_SOFTDECODE_INFO_STRU));

    if (VOS_OK != SCM_SoftDecodeCfgHdlcInit(&g_stScmHdlcSoftDecodeEntity))
    {
        (VOS_VOID)vos_printf("SCM_SoftDecodeCfgRcvTaskInit: Error, HDLC Init Fail.\n");

        g_stScmSoftDecodeInfo.ulHdlcInitErr++;

        return VOS_ERR;
    }

    g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId = OM_RingBufferCreate(SCM_DATA_RCV_BUFFER_SIZE);

    if (VOS_NULL_PTR == g_stSCMDataRcvTaskCtrlInfo.rngOmRbufId)
    {
        (VOS_VOID)vos_printf("SCM_SoftDecodeCfgRcvTaskInit: Error, Creat OMCFG ringBuffer Fail.\n");

        g_stScmSoftDecodeInfo.stRbInfo.ulRingBufferCreatErr++;

        (VOS_VOID)VOS_MemFree(MSP_PID_DIAG_APP_AGENT, g_stScmHdlcSoftDecodeEntity.pucDecapBuff);

        return VOS_ERR;
    }

    g_stSCMDataRcvTaskCtrlInfo.pucBuffer = &g_aucSCMDataRcvBuffer[0];

    VOS_SpinLockInit(&g_stScmSoftDecodeDataRcvSpinLock);

    return VOS_OK;
}

VOS_VOID SCM_SoftDecodeInfoShow(VOS_VOID)
{
    (VOS_VOID)vos_printf("\r\nSCM_SoftDecodeInfoShow:\r\n");

    (VOS_VOID)vos_printf("\r\nSem Creat Error %d:\r\n",                   g_stScmSoftDecodeInfo.stRbInfo.ulSemCreatErr);
    (VOS_VOID)vos_printf("\r\nSem Give Error %d:\r\n",                    g_stScmSoftDecodeInfo.stRbInfo.ulSemGiveErr);
    (VOS_VOID)vos_printf("\r\nRing Buffer Creat Error %d:\r\n",           g_stScmSoftDecodeInfo.stRbInfo.ulRingBufferCreatErr);
    (VOS_VOID)vos_printf("\r\nTask Id Error %d:\r\n",                     g_stScmSoftDecodeInfo.stRbInfo.ulTaskIdErr);
    (VOS_VOID)vos_printf("\r\nRing Buffer not Enough %d:\r\n",            g_stScmSoftDecodeInfo.stRbInfo.ulBufferNotEnough);
    (VOS_VOID)vos_printf("\r\nRing Buffer Flush %d:\r\n",                 g_stScmSoftDecodeInfo.stRbInfo.ulRingBufferFlush);
    (VOS_VOID)vos_printf("\r\nRing Buffer Put Error %d:\r\n",             g_stScmSoftDecodeInfo.stRbInfo.ulRingBufferPutErr);

    (VOS_VOID)vos_printf("\r\nRing Buffer Put Success Times %d:\r\n",     g_stScmSoftDecodeInfo.stPutInfo.ulNum);
    (VOS_VOID)vos_printf("\r\nRing Buffer Put Success Bytes %d:\r\n",     g_stScmSoftDecodeInfo.stPutInfo.ulDataLen);

    (VOS_VOID)vos_printf("\r\nRing Buffer Get Success Times %d:\r\n",     g_stScmSoftDecodeInfo.stGetInfo.ulNum);
    (VOS_VOID)vos_printf("\r\nRing Buffer Get Success Bytes %d:\r\n",     g_stScmSoftDecodeInfo.stGetInfo.ulDataLen);

    (VOS_VOID)vos_printf("\r\nHDLC Decode Success Times %d:\r\n",         g_stScmSoftDecodeInfo.stHdlcDecapData.ulNum);
    (VOS_VOID)vos_printf("\r\nHDLC Decode Success Bytes %d:\r\n",         g_stScmSoftDecodeInfo.stHdlcDecapData.ulDataLen);

    (VOS_VOID)vos_printf("\r\nHDLC Decode Error Times %d:\r\n",           g_stScmSoftDecodeInfo.ulFrameDecapErr);

    (VOS_VOID)vos_printf("\r\nHDLC Init Error Times %d:\r\n",             g_stScmSoftDecodeInfo.ulHdlcInitErr);

    (VOS_VOID)vos_printf("\r\nHDLC Decode Data Type Error Times %d:\r\n", g_stScmSoftDecodeInfo.ulDataTypeErr);

    (VOS_VOID)vos_printf("\r\nCPM Reg Logic Rcv Func Success Times %d:\r\n", g_stScmSoftDecodeInfo.ulCpmRegLogicRcvSuc);
}






