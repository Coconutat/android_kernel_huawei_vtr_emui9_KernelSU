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
  1 Include HeadFile
*****************************************************************************/
#include  "mdrv.h"
#include  "msp_errno.h"
#include  "msp_debug.h"
#include  "diag_common.h"
#include  "diag_debug.h"
#include  "OmCommonPpm.h"
#include  "diag_cfg.h"


/*****************************************************************************
  2 Declare the Global Variable
*****************************************************************************/

#define    THIS_FILE_ID        MSP_FILE_ID_MSP_DEBUG_C

/*****************************************************************************
  3 Function
*****************************************************************************/

/*****************************************************************************
 Function Name   : PTR : Process Trace Record (流程跟踪记录)
 Description     : 跟踪整个处理流程
*****************************************************************************/
DIAG_PTR_INFO_STRU g_stPtrInfo = {0};


VOS_VOID diag_PTR(DIAG_PTR_ID_ENUM enType)
{
    g_stPtrInfo.stPtr[g_stPtrInfo.ulCur].enStep = enType;
    g_stPtrInfo.stPtr[g_stPtrInfo.ulCur].ulTime = VOS_GetSlice();
    g_stPtrInfo.ulCur = (g_stPtrInfo.ulCur + 1) % DIAG_PTR_NUMBER;
}



VOS_VOID DIAG_DebugPTR(VOS_VOID)
{
    void *pFile;
    VOS_UINT32 ret;
    VOS_UINT32 ulValue;
    VOS_CHAR *DirPath = "/modem_log/DIAG";
    VOS_CHAR *FilePath = "/modem_log/DIAG/DIAG_PTR.bin";

    /* 如果DIAG目录不存在则先创建目录 */
    if (VOS_OK != mdrv_file_access(DirPath, 0))
    {
        if (VOS_OK != mdrv_file_mkdir(DirPath))
        {
            (VOS_VOID)vos_printf(" mdrv_file_mkdir /modem_log/DIAG failed.\n");
            return ;
        }
    }

    pFile = mdrv_file_open(FilePath, "wb+");
    if(pFile == 0)
    {
        (VOS_VOID)vos_printf(" mdrv_file_open failed.\n");

        return ;
    }

    ret = DIAG_DebugFileHeader(pFile);
    if(VOS_OK != ret)
    {
        (VOS_VOID)vos_printf(" DIAG_DebugFileHeader failed .\n");
        (VOS_VOID)mdrv_file_close(pFile);
        return ;
    }

    /* 打点信息长度 */
    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_stPtrInfo);
    ret = mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write sizeof g_stPtrInfo failed.\n");
    }

    /* 再写入打点信息 */
    ret = mdrv_file_write(&g_stPtrInfo, 1, sizeof(g_stPtrInfo), pFile);
    if(ret != sizeof(g_stPtrInfo))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write g_stPtrInfo failed.\n");
    }

    DIAG_DebugFileTail(pFile, FilePath);

    (VOS_VOID)mdrv_file_close(pFile);

    return ;
}


VOS_VOID DIAG_DebugShowPTR(VOS_UINT32 ulnum)
{
    VOS_UINT32 i, cur;
    VOS_UINT32 ulCurTime;
    DIAG_PTR_INFO_STRU *pPtrTmp;
    VOS_UINT32 ptrInfoSize = 0;

    VOS_CHAR *cName[EN_DIAG_PTR_PPM_PORTSEND+1] =
    {
        "begin",    "ppm_rcv",  "cpm_rcv",  "scm_soft", "scm_self", "scm_rcv",  "scm_rcv1", "scm_disp",
        "mspsvic1", "mspsvic2", "diagsvc1", "diagsvc2", "diagmsg1", "diagmsg2", "msgmsp1",  "msptrans",
        "msp_ps1",  "msp_ps2",  "connect",  "disconn",  "msg_rpt",  "svicpack", "snd_code", "scm_code",
        "code_dst", "scm_send", "cpm_send", "ppm_send"
    };

    ulCurTime = VOS_GetSlice();

    (VOS_VOID)vos_printf("current time %d ms.\n", (ulCurTime/33));

    for(i = 0; i < (EN_DIAG_PTR_PPM_PORTSEND+1); i++)
    {
        if(0 == (i % 8))
        {
            (VOS_VOID)vos_printf("\n");
        }
        (VOS_VOID)vos_printf("%02d %8s | ", i, cName[i]);
    }
    (VOS_VOID)vos_printf("\n");

    ptrInfoSize = sizeof(g_stPtrInfo);

    pPtrTmp = (DIAG_PTR_INFO_STRU *)VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT, DYNAMIC_MEM_PT, ptrInfoSize);
    if(VOS_NULL == pPtrTmp)
    {
        return;
    }

    (VOS_VOID)VOS_MemCpy_s(pPtrTmp, ptrInfoSize, &g_stPtrInfo, sizeof(g_stPtrInfo));

    cur = pPtrTmp->ulCur;

    for(i = 0; i < ulnum; i++)
    {
        if(0 != pPtrTmp->stPtr[cur].ulTime)
        {
            if(0 == (i % 20))
            {
                (VOS_VOID)vos_printf("\n");
                (VOS_VOID)VOS_TaskDelay(10);
            }

            (VOS_VOID)vos_printf("%02d %08d ms | ", pPtrTmp->stPtr[cur].enStep, (pPtrTmp->stPtr[cur].ulTime/33));
        }

        cur = (cur + 1) % DIAG_PTR_NUMBER;
    }
    (VOS_VOID)vos_printf("\n i = %d, over!\n", i);

    VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pPtrTmp);

    return ;
}

extern OM_ACPU_DEBUG_INFO g_stAcpuDebugInfo;

extern OM_CHANNLE_PORT_CFG_STRU g_stPortCfg;

extern OM_VCOM_DEBUG_INFO g_stVComDebugInfo[3];

extern VOS_UINT32 g_ulDiagCfgInfo;


VOS_VOID DIAG_DebugCommon(VOS_VOID)
{
    void *pFile;
    VOS_UINT32 ret;
    VOS_CHAR *DirPath = "/modem_log/DIAG";
    VOS_CHAR *FilePath = "/modem_log/DIAG/DIAG_DebugCommon.bin";
    VOS_UINT32 ulValue;
    VOS_UINT8 *pData;
    VOS_UINT32 ullen,offset;
    VOS_CHAR   aucInfo[DIAG_DEBUG_INFO_LEN];

    /* 前两个U32保存A/C核PID个数 */
    ullen =   sizeof(g_ulDiagCfgInfo)
            + sizeof(g_stPortCfg)
            + sizeof(VOS_UINT32) + sizeof(g_stAcpuDebugInfo)
            + sizeof(VOS_UINT32) + sizeof(g_stVComDebugInfo);

    pData = (VOS_UINT8 *)VOS_MemAlloc(DIAG_AGENT_PID, DYNAMIC_MEM_PT, ullen);
    if(VOS_NULL == pData)
    {
        return;
    }

    /* 如果DIAG目录不存在则先创建目录 */
    if (VOS_OK != mdrv_file_access(DirPath, 0))
    {
        if (VOS_OK != mdrv_file_mkdir(DirPath))
        {
            (VOS_VOID)vos_printf(" mdrv_file_mkdir /modem_log/DIAG failed.\n");
            VOS_MemFree(DIAG_AGENT_PID, pData);
            return ;
        }
    }

    pFile = mdrv_file_open(FilePath, "wb+");
    if(pFile == 0)
    {
        (VOS_VOID)vos_printf(" mdrv_file_open failed.\n");
        
        VOS_MemFree(DIAG_AGENT_PID, pData);

        return ;
    }

    ret = DIAG_DebugFileHeader(pFile);
    if(VOS_OK != ret)
    {
        (VOS_VOID)vos_printf(" DIAG_DebugFileHeader failed .\n");
        (VOS_VOID)mdrv_file_close(pFile);
            VOS_MemFree(DIAG_AGENT_PID, pData);
        return ;
    }

    (VOS_VOID)VOS_MemSet_s(aucInfo, sizeof(aucInfo), 0, DIAG_DEBUG_INFO_LEN);
    (VOS_VOID)VOS_MemCpy_s(aucInfo, (DIAG_DEBUG_INFO_LEN-1), "DIAG common info", VOS_StrLen("DIAG common info"));

    /* 通用信息 */
    ret = mdrv_file_write(aucInfo, 1, DIAG_DEBUG_INFO_LEN, pFile);
    if(ret != DIAG_DEBUG_INFO_LEN)
    {
        (VOS_VOID)vos_printf(" mdrv_file_write DIAG number info failed.\n");
    }

    offset  = 0;

    /* 当前DIAG的连接状态 */
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_ulDiagCfgInfo, sizeof(g_ulDiagCfgInfo));
    offset += sizeof(g_ulDiagCfgInfo);

    /* CPM记录的当前连接的通道 */
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_stPortCfg, sizeof(g_stPortCfg));
    offset += sizeof(g_stPortCfg);

    /* USB端口的相关可维可测信息 */
    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_stAcpuDebugInfo);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_stAcpuDebugInfo, sizeof(g_stAcpuDebugInfo));
    offset += sizeof(g_stAcpuDebugInfo);

    /* netlink端口的相关可维可测信息 */
    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_stVComDebugInfo);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_stVComDebugInfo, sizeof(g_stVComDebugInfo));
    offset += sizeof(g_stVComDebugInfo);

    ret = mdrv_file_write(pData, 1, offset, pFile);
    if(ret != offset)
    {
        (VOS_VOID)vos_printf(" mdrv_file_write pData failed.\n");
    }

    DIAG_DebugFileTail(pFile, FilePath);

    (VOS_VOID)mdrv_file_close(pFile);

    VOS_MemFree(DIAG_AGENT_PID, pData);

    return ;
}




VOS_UINT32 DIAG_DebugFileHeader(void *pFile)
{
    VOS_UINT32 ret;
    VOS_UINT32 ulValue;

    ret = (VOS_UINT32)mdrv_file_seek(pFile, 0, DRV_SEEK_SET);
    if(VOS_OK != ret)
    {
        (VOS_VOID)vos_printf(" mdrv_file_seek failed .\n");
        return ERR_MSP_FAILURE;
    }

    ulValue = DIAG_DEBUG_START;

    /* file start flag */
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write start flag failed.\n");
        return ERR_MSP_FAILURE;
    }

    ulValue = DIAG_DEBUG_VERSION;

    /* debug version */
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write debug version failed.\n");
        return ERR_MSP_FAILURE;
    }

    ulValue = 0;

    /* file size */
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write file size failed.\n");
        return ERR_MSP_FAILURE;
    }

    ulValue = VOS_GetSlice();

    /* 当前的slice */
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write ulTime failed.\n");
        return ERR_MSP_FAILURE;
    }

    return ERR_MSP_SUCCESS;
}



VOS_VOID DIAG_DebugFileTail(void *pFile, VOS_CHAR *FilePath)
{
    VOS_UINT32 ret;
    VOS_UINT32 ulValue;

    /* file end flag */
    ulValue = DIAG_DEBUG_END;
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write start flag failed.\n");
    }

}


extern VOS_UINT8 g_EventModuleCfg[DIAG_CFG_PID_NUM];

extern DIAG_CBT_INFO_TBL_STRU g_astCBTInfoTbl[EN_DIAG_DEBUG_INFO_MAX];



VOS_VOID diag_numberinfo(void *pFile)
{
    VOS_UINT32 ret;
    VOS_UINT32 ulValue;
    VOS_CHAR   aucInfo[DIAG_DEBUG_INFO_LEN];

    (VOS_VOID)VOS_MemSet_s(aucInfo, sizeof(aucInfo),0, DIAG_DEBUG_INFO_LEN);
    (VOS_VOID)VOS_MemCpy_s(aucInfo, (DIAG_DEBUG_INFO_LEN-1), "DIAG number info", VOS_StrLen("DIAG number info"));

    /* 上报次数信息 */
    ret = (VOS_UINT32)mdrv_file_write(aucInfo, 1, DIAG_DEBUG_INFO_LEN, pFile);
    if(ret != DIAG_DEBUG_INFO_LEN)
    {
        (VOS_VOID)vos_printf(" mdrv_file_write DIAG number info failed.\n");
    }

    /* 当前的slice */
    ulValue = VOS_GetSlice();
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write ulTime failed.\n");
    }

    /* 变量的size */
    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_astCBTInfoTbl);
    ret = (VOS_UINT32)mdrv_file_write(&ulValue, 1, sizeof(ulValue), pFile);
    if(ret != sizeof(ulValue))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write ulTime failed.\n");
    }

    /* 各上报次数统计量 */
    ret = (VOS_UINT32)mdrv_file_write(&g_astCBTInfoTbl[0], 1, sizeof(g_astCBTInfoTbl), pFile);
    if(ret != sizeof(g_astCBTInfoTbl))
    {
        (VOS_VOID)vos_printf(" mdrv_file_write g_astCBTInfoTbl failed.\n");
    }
}


VOS_VOID DIAG_DebugNoIndLog(VOS_VOID)
{
    void *pFile;
    VOS_UINT32 ret;
    VOS_CHAR *DirPath = "/modem_log/DIAG";
    VOS_CHAR *FilePath = "/modem_log/DIAG/DIAG_AcoreNoIndLog.bin";
    VOS_UINT32 ulValue;
    VOS_CHAR   aucInfo[DIAG_DEBUG_INFO_LEN];
    VOS_UINT8 *pData;
    VOS_UINT32 ullen,offset;

    /* 前两个U32保存A/C核PID个数 */
    ullen = (2 * sizeof(VOS_UINT32))
            + sizeof(VOS_UINT32) + sizeof(g_ALayerSrcModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_CLayerSrcModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_ALayerDstModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_CLayerDstModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_stMsgCfg)
            + sizeof(VOS_UINT32) + sizeof(g_EventModuleCfg)
            + sizeof(VOS_UINT32) + sizeof(g_PrintTotalCfg)
            + sizeof(VOS_UINT32) + sizeof(g_PrintModuleCfg);

    pData = VOS_MemAlloc(DIAG_AGENT_PID, DYNAMIC_MEM_PT, ullen);
    if(VOS_NULL == pData)
    {
        return;
    }

    /* 如果DIAG目录不存在则先创建目录 */
    if (VOS_OK != mdrv_file_access(DirPath, 0))
    {
        if (VOS_OK != mdrv_file_mkdir(DirPath))
        {
            (VOS_VOID)vos_printf(" mdrv_file_mkdir /modem_log/DIAG failed.\n");
            VOS_MemFree(DIAG_AGENT_PID, pData);
            return ;
        }
    }

    pFile = mdrv_file_open(FilePath, "wb+");
    if(pFile == 0)
    {
        (VOS_VOID)vos_printf(" mdrv_file_open failed.\n");
        
        VOS_MemFree(DIAG_AGENT_PID, pData);

        return ;
    }

    ret = DIAG_DebugFileHeader(pFile);
    if(VOS_OK != ret)
    {
        (VOS_VOID)vos_printf(" DIAG_DebugFileHeader failed .\n");
        (VOS_VOID)mdrv_file_close(pFile);
        
        VOS_MemFree(DIAG_AGENT_PID, pData);
        return ;
    }

    (VOS_VOID)VOS_MemSet_s(aucInfo, sizeof(aucInfo), 0, DIAG_DEBUG_INFO_LEN);
    (VOS_VOID)VOS_MemCpy_s(aucInfo, (DIAG_DEBUG_INFO_LEN-1), "DIAG config info", VOS_StrLen("DIAG config info"));

    /* 配置开关信息 */
    ret = (VOS_UINT32)mdrv_file_write(aucInfo, 1, DIAG_DEBUG_INFO_LEN, pFile);
    if(ret != DIAG_DEBUG_INFO_LEN)
    {
        (VOS_VOID)vos_printf(" mdrv_file_write DIAG config info failed.\n");
    }

    offset  = 0;

    /* A核PID个数 */
    ulValue = VOS_CPU_ID_1_PID_BUTT - VOS_PID_CPU_ID_1_DOPRAEND;
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    /* C核PID个数 */
    ulValue = VOS_CPU_ID_0_PID_BUTT - VOS_PID_CPU_ID_0_DOPRAEND;
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_ALayerSrcModuleCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_ALayerSrcModuleCfg[0], sizeof(g_ALayerSrcModuleCfg));
    offset += sizeof(g_ALayerSrcModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_CLayerSrcModuleCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_CLayerSrcModuleCfg[0], sizeof(g_CLayerSrcModuleCfg));
    offset += sizeof(g_CLayerSrcModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_ALayerDstModuleCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_ALayerDstModuleCfg[0], sizeof(g_ALayerDstModuleCfg));
    offset += sizeof(g_ALayerDstModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_CLayerDstModuleCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_CLayerDstModuleCfg[0], sizeof(g_CLayerDstModuleCfg));
    offset += sizeof(g_CLayerDstModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_stMsgCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_stMsgCfg, sizeof(g_stMsgCfg));
    offset += sizeof(g_stMsgCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_EventModuleCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_EventModuleCfg[0], sizeof(g_EventModuleCfg));
    offset += sizeof(g_EventModuleCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_PrintTotalCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_PrintTotalCfg, sizeof(g_PrintTotalCfg));
    offset += sizeof(g_PrintTotalCfg);

    ulValue = DIAG_DEBUG_SIZE_FLAG | sizeof(g_PrintModuleCfg);
    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &ulValue, sizeof(ulValue));
    offset += sizeof(ulValue);

    (VOS_VOID)VOS_MemCpy_s((pData + offset), (ullen - offset), &g_PrintModuleCfg[0], sizeof(g_PrintModuleCfg));
    offset += sizeof(g_PrintModuleCfg);

    ret = (VOS_UINT32)mdrv_file_write(pData, 1, offset, pFile);
    if(ret != offset)
    {
        (VOS_VOID)vos_printf(" mdrv_file_write pData failed.\n");
    }

    diag_numberinfo(pFile);

    /* 延迟5秒后再统计一次 */
    (VOS_VOID)VOS_TaskDelay(5000);

    diag_numberinfo(pFile);

    DIAG_DebugFileTail(pFile, FilePath);

    (VOS_VOID)mdrv_file_close(pFile);

    VOS_MemFree(DIAG_AGENT_PID, pData);

    return ;
}



