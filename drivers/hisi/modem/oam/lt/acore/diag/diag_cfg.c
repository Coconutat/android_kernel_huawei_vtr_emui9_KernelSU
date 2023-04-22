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
#include "diag_common.h"
#include "diag_cfg.h"
#include "diag_msgmsp.h"
#include "diag_msgphy.h"
#include "diag_api.h"
#include "diag_debug.h"
#include "msp_errno.h"
#include "LPsNvInterface.h"
#include "mdrv.h"
#include "msp_nv_id.h"
#include "SCMProc.h"
#include "soc_socp_adapter.h"



#define    THIS_FILE_ID        MSP_FILE_ID_DIAG_CFG_C

/*****************************************************************************
  2 Declare the Global Variable
*****************************************************************************/
#define DIAG_CFG_PRINT_MODULE_NUM              (44)

VOS_UINT32 g_ulDiagCfgInfo = 0;

VOS_UINT8 g_ALayerSrcModuleCfg[VOS_CPU_ID_1_PID_BUTT - VOS_PID_CPU_ID_1_DOPRAEND] = {0};
VOS_UINT8 g_CLayerSrcModuleCfg[VOS_CPU_ID_0_PID_BUTT - VOS_PID_CPU_ID_0_DOPRAEND] = {0};

VOS_UINT8 g_ALayerDstModuleCfg[VOS_CPU_ID_1_PID_BUTT - VOS_PID_CPU_ID_1_DOPRAEND] = {0};
VOS_UINT8 g_CLayerDstModuleCfg[VOS_CPU_ID_0_PID_BUTT - VOS_PID_CPU_ID_0_DOPRAEND] = {0};

VOS_UINT8 g_EventModuleCfg[DIAG_CFG_PID_NUM] = {0};

VOS_UINT8 g_PrintModuleCfg[DIAG_CFG_PID_NUM] = {0};
VOS_UINT32 g_PrintTotalCfg = DIAG_CFG_PRINT_TOTAL_MODULE_SWT_NOT_USE;
DIAG_CFG_LOG_CAT_CFG_STRU g_stMsgCfg = {0};

VOS_UINT32 g_ulDiagDfsCtrl = 0;

HTIMER          g_DebugTimer;

/*****************************************************************************
  3 Function
*****************************************************************************/


VOS_VOID diag_CfgResetAllSwt(VOS_VOID)
{
    VOS_ULONG    ulLockLevel;

    /* 规避老的hids在建链时候下发disconnect命令，不能将开机log配置清除 */
    if(DIAG_IS_POLOG_ON)
    {
        g_ulDiagCfgInfo = DIAG_CFG_INIT | DIAG_CFG_POWERONLOG;
        diag_printf("diag_CfgResetAllSwt, keep init&poweronlog flag.\n");
    }
    else
    {
        g_ulDiagCfgInfo = DIAG_CFG_INIT;
    }

    /*清空层间开关状态*/
    (VOS_VOID)VOS_MemSet_s(g_ALayerSrcModuleCfg, (VOS_UINT32)sizeof(g_ALayerSrcModuleCfg), 0, (VOS_UINT32)sizeof(g_ALayerSrcModuleCfg));
    (VOS_VOID)VOS_MemSet_s(g_CLayerSrcModuleCfg, (VOS_UINT32)sizeof(g_CLayerSrcModuleCfg), 0, (VOS_UINT32)sizeof(g_CLayerSrcModuleCfg));
    (VOS_VOID)VOS_MemSet_s(g_ALayerDstModuleCfg, (VOS_UINT32)sizeof(g_ALayerDstModuleCfg), 0, (VOS_UINT32)sizeof(g_ALayerDstModuleCfg));
    (VOS_VOID)VOS_MemSet_s(g_CLayerDstModuleCfg, (VOS_UINT32)sizeof(g_CLayerDstModuleCfg), 0, (VOS_UINT32)sizeof(g_CLayerDstModuleCfg));

    /* 为兼容原TL任务的EVENT开关机制，默认把所有EVENT子开关都设置为打开 */
    (VOS_VOID)VOS_MemSet_s(g_EventModuleCfg, (VOS_UINT32)sizeof(g_EventModuleCfg), 0x1, (VOS_UINT32)sizeof(g_EventModuleCfg));

    /*清空打印开关状态*/
    (VOS_VOID)VOS_MemSet_s(g_PrintModuleCfg, (VOS_UINT32)sizeof(g_PrintModuleCfg), 0, (VOS_UINT32)sizeof(g_PrintModuleCfg));

    /*清空打印总开关状态*/
    g_PrintTotalCfg = DIAG_CFG_PRINT_TOTAL_MODULE_SWT_NOT_USE;

    /*清空消息过滤开关状态*/
    (VOS_VOID)VOS_MemSet_s(&g_stMsgCfg, (VOS_UINT32)sizeof(DIAG_CFG_LOG_CAT_CFG_STRU), 0, (VOS_UINT32)sizeof(DIAG_CFG_LOG_CAT_CFG_STRU));

    VOS_SpinLockIntLock(&g_DiagLogPktNum.ulPrintLock, ulLockLevel);
    g_DiagLogPktNum.ulPrintNum = 0;
    VOS_SpinUnlockIntUnlock(&g_DiagLogPktNum.ulPrintLock, ulLockLevel);

    VOS_SpinLockIntLock(&g_DiagLogPktNum.ulTransLock, ulLockLevel);
    g_DiagLogPktNum.ulTransNum = 0;
    VOS_SpinUnlockIntUnlock(&g_DiagLogPktNum.ulTransLock, ulLockLevel);

    VOS_SpinLockIntLock(&g_DiagLogPktNum.ulEventLock, ulLockLevel);
    g_DiagLogPktNum.ulEventNum = 0;
    VOS_SpinUnlockIntUnlock(&g_DiagLogPktNum.ulEventLock, ulLockLevel);


    VOS_SpinLockIntLock(&g_DiagLogPktNum.ulAirLock, ulLockLevel);
    g_DiagLogPktNum.ulAirNum = 0;
    VOS_SpinUnlockIntUnlock(&g_DiagLogPktNum.ulAirLock, ulLockLevel);

    VOS_SpinLockIntLock(&g_DiagLogPktNum.ulVoLTELock, ulLockLevel);
    g_DiagLogPktNum.ulVoLTENum = 0;
    VOS_SpinUnlockIntUnlock(&g_DiagLogPktNum.ulVoLTELock, ulLockLevel);

    VOS_SpinLockIntLock(&g_DiagLogPktNum.ulLayerLock, ulLockLevel);
    g_DiagLogPktNum.ulLayerNum = 0;
    VOS_SpinUnlockIntUnlock(&g_DiagLogPktNum.ulLayerLock, ulLockLevel);

    VOS_SpinLockIntLock(&g_DiagLogPktNum.ulUserLock, ulLockLevel);
    g_DiagLogPktNum.ulUserNum = 0;
    VOS_SpinUnlockIntUnlock(&g_DiagLogPktNum.ulUserLock, ulLockLevel);

    return;
}


VOS_UINT32 diag_CfgSetGlobalBitValue(VOS_UINT32* pstDiagGlobal,ENUM_DIAG_CFG_BIT_U32 enBit,ENUM_DIAG_CFG_SWT_U8 enSwtich)
{
    /*设置为open 1时，需要使用|才能置该bit 为1*/
    if(DIAG_CFG_SWT_OPEN == enSwtich)
    {
        *pstDiagGlobal |=  ((VOS_UINT)1 << enBit);
    }
    /*设置为close 0时，需要使用&才能置该bit 为0*/
    else if(DIAG_CFG_SWT_CLOSE == enSwtich)
    {
        *pstDiagGlobal &= ~((VOS_UINT)1 << enBit);
    }
    else
    {
        return ERR_MSP_INVALID_PARAMETER;
    }
    return ERR_MSP_SUCCESS;
}


VOS_UINT32 diag_AirCfgProc (VOS_UINT8* pstReq)
{
    VOS_UINT32 ret;
    DIAG_CMD_LOG_CAT_AIR_REQ_STRU* pstAirSwtReq = NULL;
    DIAG_CMD_LOG_CAT_AIR_CNF_STRU stAirSwtCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead;
    ENUM_DIAG_CFG_SWT_U8 enLSwitch;

    VOS_UINT32 ulLen;
    DIAG_MSG_A_TRANS_C_STRU *pstInfo;

    /* 开机log上报特性中，在工具下发AirCfg配置时，才打开启动缓存的log上报 */
    if(DIAG_IS_POLOG_ON)
    {
        /* 收到空口开关恢复为正常的开关控制上报 */
        g_ulDiagCfgInfo &= (~DIAG_CFG_POWERONLOG);


        /* 通过空口开关来作为开机log上报的触发命令 */
        SCM_RegCoderDestIndChan();

        /* 手动触发log上报，并readdone操作后去除中断屏蔽 */
        SCM_CoderDestReadCB(SOCP_CODER_DST_OM_IND);
    }

    pstDiagHead  = (DIAG_FRAME_INFO_STRU*)(pstReq);
    pstAirSwtReq = (DIAG_CMD_LOG_CAT_AIR_REQ_STRU*)(pstReq + DIAG_MESSAGE_DATA_HEADER_LEN);

    /*设置LT空口开关值*/
    enLSwitch = DIAG_GET_CFG_SWT(pstAirSwtReq->ulSwitch);
    ret = diag_CfgSetGlobalBitValue(&g_ulDiagCfgInfo,DIAG_CFG_LT_AIR_BIT,enLSwitch);
    if(ERR_MSP_SUCCESS == ret)
    {

        DIAG_MSG_ACORE_CFG_PROC(ulLen, pstDiagHead, pstInfo, ret);
        return ret;
    }

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stAirSwtCnf, pstDiagHead);

    stDiagInfo.ulMsgType    = DIAG_MSG_TYPE_MSP;

    stAirSwtCnf.ulRc = ERR_HIDS_CORE_ERROR;

    ret = DIAG_MsgReport(&stDiagInfo, &stAirSwtCnf, (VOS_UINT32)sizeof(stAirSwtCnf));

    return ret;
}



VOS_UINT32 diag_CfgSetLayerSwt(DIAG_CMD_LOG_CAT_LAYER_REQ_STRU* pstLayerReq, VOS_UINT32 ulCfgSize)
{

    VOS_UINT32 j;
    VOS_UINT32 ulOffset = 0;
    ENUM_DIAG_CFG_SWT_U8 enSwitch = 0;

    if((0 == ulCfgSize)||(0 !=ulCfgSize %sizeof(DIAG_CMD_LOG_CAT_LAYER_REQ_STRU)))
    {
        return  ERR_MSP_INVALID_PARAMETER;
    }

    /* 遍历某Category的开关配置项列表，查找对应的配置项进行设置*/
    for(j = 0 ; j< ulCfgSize /sizeof(DIAG_CMD_LOG_CAT_LAYER_REQ_STRU);j++)
    {
        enSwitch = DIAG_GET_CFG_SWT((pstLayerReq + j)->ulSwitch);

        if(DIAG_CFG_LAYER_MODULE_IS_ACORE((pstLayerReq + j)->ulModuleId))
        {
            ulOffset = DIAG_CFG_LAYER_MODULE_ACORE_OFFSET((pstLayerReq + j)->ulModuleId);

            if( DIAG_CMD_LAYER_MOD_SRC == (pstLayerReq + j)->ulIsDestModule)
            {
                g_ALayerSrcModuleCfg[ulOffset] = (VOS_UINT8)enSwitch;
            }
            else
            {
                g_ALayerDstModuleCfg[ulOffset] = (VOS_UINT8)enSwitch;
            }
        }
        else if(DIAG_CFG_LAYER_MODULE_IS_CCORE((pstLayerReq + j)->ulModuleId ))
        {
            ulOffset = DIAG_CFG_LAYER_MODULE_CCORE_OFFSET((pstLayerReq + j)->ulModuleId);

            if( DIAG_CMD_LAYER_MOD_SRC == (pstLayerReq + j)->ulIsDestModule)
            {
                g_CLayerSrcModuleCfg[ulOffset] = (VOS_UINT8)enSwitch;
            }
            else
            {
                g_CLayerDstModuleCfg[ulOffset] = (VOS_UINT8)enSwitch;
            }
        }
    }

    return ERR_MSP_SUCCESS;
}



VOS_UINT32 diag_LayerCfgProc (VOS_UINT8* pstReq)
{
    VOS_UINT32 ret;
    DIAG_FRAME_INFO_STRU *pstDiagHead = NULL;
    DIAG_CMD_LOG_CAT_LAYER_REQ_STRU* pstLayerSwtReq = NULL;
    DIAG_CMD_LOG_CAT_LAYER_CNF_STRU stLayerSwtCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};

    VOS_UINT32 ulLen;
    DIAG_MSG_A_TRANS_C_STRU *pstInfo;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)(pstReq);

    pstLayerSwtReq = (DIAG_CMD_LOG_CAT_LAYER_REQ_STRU*)(pstReq + DIAG_MESSAGE_DATA_HEADER_LEN);

    /*设置层间模块开关到全局变量中*/
    ret = diag_CfgSetLayerSwt(pstLayerSwtReq, (VOS_UINT32)(pstDiagHead->ulMsgLen - sizeof(MSP_DIAG_DATA_REQ_STRU)));
    if(ERR_MSP_SUCCESS == ret)
    {
        DIAG_MSG_ACORE_CFG_PROC(ulLen, pstDiagHead, pstInfo, ret);
        return ret;

    }

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stLayerSwtCnf, pstDiagHead);

    stDiagInfo.ulMsgType    = DIAG_MSG_TYPE_MSP;

    stLayerSwtCnf.ulRc = ERR_HIDS_CORE_ERROR;

    ret = DIAG_MsgReport(&stDiagInfo, &stLayerSwtCnf, (VOS_UINT32)sizeof(stLayerSwtCnf));

    return ret;
}



VOS_UINT32 diag_EventCfgProc(VOS_UINT8* pstReq)
{
    DIAG_CMD_LOG_CAT_EVENT_REQ_STRU* pstEvtSwtReq = NULL;
    DIAG_CMD_LOG_CAT_EVENT_CNF_STRU stEvtSwtCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead = NULL;
    VOS_UINT32 ret;
    VOS_UINT32 i, pid;
    ENUM_DIAG_CFG_SWT_U8 enSwitch;

    VOS_UINT32 ulLen;
    DIAG_MSG_A_TRANS_C_STRU *pstInfo;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)pstReq;

    pstEvtSwtReq = (DIAG_CMD_LOG_CAT_EVENT_REQ_STRU*)(pstReq + DIAG_MESSAGE_DATA_HEADER_LEN);

    enSwitch = (ENUM_DIAG_CFG_SWT_U8)pstEvtSwtReq->ulSwt;

    /* 打开和关闭都是配置固定的模块，只有总开关是打开时才配置到各模块 */
    if(enSwitch)
    {
        for(i = 0; i < pstEvtSwtReq->ulCount; i++)
        {
            pid = DIAG_EVENT_MODID(pstEvtSwtReq->aulModuleId[i]);
            if(DIAG_CFG_MODULE_IS_INVALID(pid))
            {
                continue;
            }

            g_EventModuleCfg[pid - VOS_PID_DOPRAEND] = DIAG_EVENT_SWT(pstEvtSwtReq->aulModuleId[i]);
        }
    }

    ret = diag_CfgSetGlobalBitValue(&g_ulDiagCfgInfo, DIAG_CFG_EVENT_BIT, enSwitch);
    if(ERR_MSP_SUCCESS == ret)
    {
        DIAG_MSG_ACORE_CFG_PROC(ulLen, pstDiagHead, pstInfo, ret);
        return ret;

    }

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stEvtSwtCnf, pstDiagHead);

    stDiagInfo.ulMsgType    = DIAG_MSG_TYPE_MSP;

    stEvtSwtCnf.ulRc = ERR_HIDS_CORE_ERROR;

    ret = DIAG_MsgReport(&stDiagInfo, &stEvtSwtCnf, sizeof(stEvtSwtCnf));

    return ret;
}


VOS_UINT32 diag_CfgSetMsgSwt(DIAG_CMD_LOG_CAT_CFG_REQ_STRU *pstCatCfgReq,VOS_UINT32 ulCfgSize)
{
    VOS_UINT32 i = 0,j;
    ENUM_DIAG_CFG_SWT_U8 enSwitch =0;
    VOS_UINT32 ulRst = ERR_MSP_INVALID_PARAMETER;
    DIAG_CFG_LOG_CAT_MSG_CFG_STRU *pstItemCfg =NULL;

    /*参数检查*/
    if((0 == ulCfgSize)||(0 !=ulCfgSize % sizeof(DIAG_CMD_LOG_CAT_CFG_REQ_STRU)))
    {
        DIAG_DEBUG_SDM_FUN(EN_DIAG_DEBUG_MSG_ERR, ulCfgSize, 0, 0);
        return ERR_MSP_DIAG_CMD_SIZE_INVALID;
    }

    for(j = 0 ; j< ulCfgSize /sizeof(DIAG_CMD_LOG_CAT_CFG_REQ_STRU);j++)
    {

        /*仅支持层间消息CATEGORY过滤*/
        if(DIAG_CMD_LOG_CATETORY_LAYER_ID != (pstCatCfgReq + j)->ulCategory)
        {
            DIAG_DEBUG_SDM_FUN(EN_DIAG_DEBUG_MSG_ERR, (pstCatCfgReq + j)->ulCategory, 0, 1);
            return ERR_MSP_NOT_SUPPORT;
        }
    }


    /* 遍历某Category的开关配置项列表，查找对应的配置项进行设置*/   /* [false alarm]:屏蔽Fortify */
    /* coverity[unreachable] */
    for(j = 0 ; j< ulCfgSize /sizeof(DIAG_CMD_LOG_CAT_CFG_REQ_STRU);j++)
    {
        enSwitch = DIAG_GET_CFG_SWT((pstCatCfgReq + j)->ulSwitch);

        for(i = 0; i < g_stMsgCfg.ulCfgCnt; i++)
        {
            pstItemCfg = (g_stMsgCfg.astMsgCfgList + i);

            if((pstCatCfgReq + j)->ulId == pstItemCfg->ulId)
            {
                pstItemCfg->ulSwt = enSwitch;
                ulRst = ERR_MSP_SUCCESS;
                break;
            }
        }
        if(i >= g_stMsgCfg.ulCfgCnt)
        {
            /*目前仅一次支持DIAG_CFG_CAT_CFG_NUM个消息过滤*/
            if((g_stMsgCfg.ulCfgCnt < DIAG_CFG_CAT_CFG_NUM))
            {
                pstItemCfg = g_stMsgCfg.astMsgCfgList + g_stMsgCfg.ulCfgCnt;

                pstItemCfg->ulId  = (pstCatCfgReq + j)->ulId;
                pstItemCfg->ulSwt = enSwitch;

                g_stMsgCfg.ulCfgCnt++;
                ulRst = ERR_MSP_SUCCESS;
            }
            else
            {
                DIAG_DEBUG_SDM_FUN(EN_DIAG_DEBUG_MSG_ERR, g_stMsgCfg.ulCfgCnt, 0, 3);
                return ERR_MSP_INVALID_PARAMETER;
            }
        }
    }

    return ulRst;

}


VOS_UINT32 diag_MsgCfgProc(VOS_UINT8* pstReq)
{
    DIAG_CMD_LOG_CAT_CFG_CNF_STRU stCatSwtCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_CMD_LOG_CAT_CFG_REQ_STRU* pstCatCfgReq = NULL;
    VOS_UINT32 ret;
    DIAG_FRAME_INFO_STRU *pstDiagHead = NULL;

    VOS_UINT32 ulLen;
    DIAG_MSG_A_TRANS_C_STRU *pstInfo;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)(pstReq);

    pstCatCfgReq = (DIAG_CMD_LOG_CAT_CFG_REQ_STRU*)(pstReq + DIAG_MESSAGE_DATA_HEADER_LEN);

    /*设置消息过滤开关到全局变量中*/
    ret = diag_CfgSetMsgSwt(pstCatCfgReq, (VOS_UINT32)(pstDiagHead->ulMsgLen - sizeof(MSP_DIAG_DATA_REQ_STRU)));
    if(ERR_MSP_SUCCESS == ret)
    {
        DIAG_MSG_ACORE_CFG_PROC(ulLen, pstDiagHead, pstInfo, ret);
        return ret;

    }

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stCatSwtCnf, pstDiagHead);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_MSP;

    stCatSwtCnf.ulRc = ERR_HIDS_CORE_ERROR;

    ret = DIAG_MsgReport(&stDiagInfo, &stCatSwtCnf, (VOS_UINT32)sizeof(stCatSwtCnf));

    return ret;

}



VOS_UINT32 diag_CfgSetPrintSwt(DIAG_CMD_LOG_CAT_PRINT_REQ_STRU* pstPrintReq, VOS_UINT32 ulCfgSize)
{
    VOS_UINT32 j = 0;
    VOS_UINT8 ucLevelFilter = 0;


    if ((0 == ulCfgSize)||(0 != ulCfgSize % sizeof(DIAG_CMD_LOG_CAT_PRINT_REQ_STRU)))
    {
        return ERR_MSP_INVALID_PARAMETER;
    }

    /* 工具的LEVEL值转换成MSP本地存储的LEVEL值 */
    /*
        TOOL        <->     MSP     <->     PS
        0x40000000  <->     0x40    <->     1 (ERROR);
        0x20000000  <->     0x20    <->     2 (WARNING);
        0x10000000  <->     0x10    <->     3 (NORMAL);
        0x08000000  <->     0x08    <->     4 (INFO)
    */

    if(DIAG_CFG_PRINT_TOTAL_MODULE == pstPrintReq->ulModuleId)
    {
        /*设置PRINT时首先重置所有模块设置*/
         (VOS_VOID)VOS_MemSet_s(g_PrintModuleCfg,(VOS_UINT32)sizeof(g_PrintModuleCfg),0,sizeof(g_PrintModuleCfg));

        /*设置打印总开关*/
        ucLevelFilter = DIAG_GET_PRINT_CFG_SWT(pstPrintReq->ulLevelFilter);
        g_PrintTotalCfg = ucLevelFilter;
    }
    else
    {
        /* 重置PRINT总开关0xFF模块*/
        g_PrintTotalCfg = DIAG_CFG_PRINT_TOTAL_MODULE_SWT_NOT_USE;

        /* 遍历某Category的开关配置项列表，查找对应的配置项进行设置*/
        for(j = 0 ; j< ulCfgSize /sizeof(DIAG_CMD_LOG_CAT_PRINT_REQ_STRU);j++)
        {
            if(DIAG_CFG_MODULE_IS_INVALID((VOS_INT32)((pstPrintReq + j)->ulModuleId )))
            {
                /* TODO:做记录 */
                //ret = ERR_MSP_INVALID_PARAMETER;
                continue;
            }

            ucLevelFilter = DIAG_GET_PRINT_CFG_SWT((pstPrintReq +j)->ulLevelFilter);

            g_PrintModuleCfg[(pstPrintReq +j)->ulModuleId - VOS_PID_DOPRAEND] = ucLevelFilter;
        }
    }
    return ERR_MSP_SUCCESS;
}



VOS_UINT32 diag_PrintCfgProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 ret;
    DIAG_FRAME_INFO_STRU *pstDiagHead = NULL;
    DIAG_CMD_LOG_CAT_PRINT_REQ_STRU* pstPrintSwtReq = NULL;
    DIAG_CMD_LOG_CAT_PRINT_CNF_STRU stPrintSwtCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};

    VOS_UINT32 ulLen;
    DIAG_MSG_A_TRANS_C_STRU *pstInfo;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)(pstReq);

    pstPrintSwtReq = (DIAG_CMD_LOG_CAT_PRINT_REQ_STRU*)(pstReq + DIAG_MESSAGE_DATA_HEADER_LEN);

    /*设置打印开关到全局变量中*/
    ret = diag_CfgSetPrintSwt(pstPrintSwtReq, (VOS_UINT32)(pstDiagHead->ulMsgLen - sizeof(MSP_DIAG_DATA_REQ_STRU)));
    if(ERR_MSP_SUCCESS == ret)
    {
        DIAG_MSG_ACORE_CFG_PROC(ulLen, pstDiagHead, pstInfo, ret);
        return ret;

    }

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stPrintSwtCnf, pstDiagHead);

    stDiagInfo.ulMsgType    = DIAG_MSG_TYPE_MSP;

    stPrintSwtCnf.ulRc = ERR_HIDS_CORE_ERROR;

    ret = DIAG_MsgReport(&stDiagInfo, &stPrintSwtCnf, (VOS_UINT32)sizeof(stPrintSwtCnf));

    return ret;
}




VOS_UINT64 diag_GetFrameTime(VOS_VOID)
{
    VOS_UINT32 ultimelow = 0;
    VOS_UINT32 ultimehigh= 0;
    VOS_UINT64 ulFrameCount = 0;
    VOS_INT32 ret;

    ret = mdrv_timer_get_accuracy_timestamp(&ultimehigh, &ultimelow);
    if(ERR_MSP_SUCCESS != ret)
    {
       ulFrameCount = 0;
    }
    else
    {
        ulFrameCount = ((VOS_UINT64)ultimehigh << 32) | ((VOS_UINT64)ultimelow);
    }
    return ulFrameCount;
}

/*****************************************************************************
 Function Name   : diag_GetTimeStampInitValue
 Description     : 该函数处理hidis获取单板中TL和Gu的时间戳初始值请求
 Input           : pstReq 待处理数据
 Output          : None
 Return          : VOS_UINT32

    2.c64416         2014-11-18  适配新的诊断架构
*****************************************************************************/
VOS_UINT32 diag_GetTimeStampInitValue(VOS_UINT8* pstReq)
{
    VOS_UINT ret = ERR_MSP_SUCCESS;
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_TIMESTAMP_CNF_STRU timestampCnf = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead = NULL;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)(pstReq);

    DIAG_MSG_COMMON_PROC(stDiagInfo, timestampCnf, pstDiagHead);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_MSP;

    timestampCnf.ulGuTimeStampInitValue = mdrv_timer_get_normal_timestamp();
    timestampCnf.ulTLTimeStampInitValue = diag_GetFrameTime();
    timestampCnf.ulErrcode              = ret;

    /*组包给FW回复*/
    ret = DIAG_MsgReport(&stDiagInfo, &timestampCnf, (VOS_UINT32)sizeof(timestampCnf));

    return (VOS_UINT32)ret;
}


#define DIAG_NV_IMEI_LEN                             15

VOS_UINT32 diag_GetImei(VOS_CHAR szimei [ 16 ])
{
    VOS_UINT32  ret;
    VOS_UINT32  uslen;
    VOS_UINT32  subscript = 0;
    VOS_CHAR   checkdata = 0;
    VOS_CHAR   auctemp[DIAG_NV_IMEI_LEN+1] = {0};

    uslen = DIAG_NV_IMEI_LEN+1;

    ret = NV_Read(0, auctemp, uslen);

    if(ret != 0)
    {
        return ret;
    }
    else
    {
        for (subscript = 0; subscript < (DIAG_NV_IMEI_LEN - 1); subscript += 2)
        {
            checkdata += (VOS_CHAR)(((auctemp[subscript])
                           +((auctemp[subscript + sizeof(VOS_CHAR)] * 2) / 10))
                           +((auctemp[subscript + sizeof(VOS_CHAR)] * 2) % 10));
        }
        checkdata = (10 - (checkdata%10)) % 10;

        for (subscript = 0; subscript < uslen; subscript++)
        {
            *(szimei + subscript) = *(auctemp + subscript) + 0x30; /*字符转换*/
        }

        szimei[DIAG_NV_IMEI_LEN - 1] = checkdata + 0x30;
        szimei[DIAG_NV_IMEI_LEN] = 0;
    }

    return 0;
}



VOS_VOID diag_GetModemInfo(DIAG_FRAME_INFO_STRU *pstDiagHead)
{
    VOS_UINT32 ulCnfRst;
    DIAG_CMD_HOST_CONNECT_CNF_STRU stCnf    = {0};
    const MODEM_VER_INFO_S* pstVerInfo;
    MSP_DIAG_CNF_INFO_STRU stDiagInfo       = {0};
    DIAG_MSG_REPORT_HEAD_STRU stDiagHead    = {0};

    /*处理结果*/
    stCnf.ulAuid = ((MSP_DIAG_DATA_REQ_STRU*)(pstDiagHead->aucData))->ulAuid;
    stCnf.ulSn   = ((MSP_DIAG_DATA_REQ_STRU*)(pstDiagHead->aucData))->ulSn;

    (VOS_VOID)VOS_MemSet_s(&(stCnf.stBuildVersion), (VOS_UINT32)sizeof(DIAG_CMD_UE_BUILD_VER_STRU), 0, (VOS_UINT32)sizeof(DIAG_CMD_UE_BUILD_VER_STRU));

    /*获取版本信息*/
    pstVerInfo = mdrv_ver_get_info();
	if(pstVerInfo!=NULL)
	{
        stCnf.stBuildVersion.usVVerNo        = pstVerInfo->stswverinfo.ulVVerNO;
        stCnf.stBuildVersion.usRVerNo        = pstVerInfo->stswverinfo.ulRVerNO;
        stCnf.stBuildVersion.usCVerNo        = pstVerInfo->stswverinfo.ulCVerNO;
        stCnf.stBuildVersion.usBVerNo        = pstVerInfo->stswverinfo.ulBVerNO;
        stCnf.stBuildVersion.usSpcNo         = pstVerInfo->stswverinfo.ulSpcNO;
        stCnf.stBuildVersion.usHardwareVerNo = pstVerInfo->stswverinfo.ulCustomNOv;
        stCnf.stBuildVersion.ulProductNo     = pstVerInfo->stswverinfo.ulProductNo;

        /*获取数采基地址*/
        stCnf.ulChipBaseAddr = (VOS_UINT32)pstVerInfo->stproductinfo.echiptype;
	}

    /*获取IMEI号*/
    (VOS_VOID)diag_GetImei(stCnf.szImei);

    /*获取软件版本号*/
    (VOS_VOID)VOS_MemSet_s(&stCnf.stUeSoftVersion, (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU), 0, (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU));

    /*路测信息获取*/
    (VOS_VOID)NV_Read(EN_NV_ID_AGENT_FLAG, &(stCnf.stAgentFlag), (VOS_UINT32)sizeof(NV_ITEM_AGENT_FLAG_STRU));

    stCnf.diag_cfg.UintValue = 0;

    /* 010: OM通道融合的版本 */
    /* 110: OM融合GU未融合的版本 */
    /* 100: OM完全融合的版本 */
    stCnf.diag_cfg.CtrlFlag.ulDrxControlFlag    = 0; /*和HIDS确认此处不再使用,打桩处理即可*/
    stCnf.diag_cfg.CtrlFlag.ulPortFlag          = 0;
    stCnf.diag_cfg.CtrlFlag.ulOmUnifyFlag       = 1;

	stCnf.ulLpdMode                             = 0x5a5a5a5a;
    stCnf.ulRc                                  = ERR_MSP_SUCCESS;

    (VOS_VOID)VOS_MemCpy_s(stCnf.szProduct, (VOS_UINT32)sizeof(PRODUCT_FULL_VERSION_STR), PRODUCT_FULL_VERSION_STR, (VOS_UINT32)sizeof(PRODUCT_FULL_VERSION_STR));

    DIAG_MSG_COMMON_PROC(stDiagInfo, stCnf, pstDiagHead);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_MSP;

    stDiagHead.u.stID.pri4b     = (stDiagInfo.ulMsgType & 0xf);
    stDiagHead.u.stID.mode4b    = (stDiagInfo.ulMode & 0xf);
    stDiagHead.u.stID.sec5b     = (stDiagInfo.ulSubType & 0x1f);
    stDiagHead.u.stID.cmdid19b  = (stDiagInfo.ulMsgId & 0x7ffff);
    stDiagHead.ulSsid           = stDiagInfo.ulSSId;
    stDiagHead.ulModemId        = stDiagInfo.ulModemid;
    stDiagHead.ulDirection      = stDiagInfo.ulDirection;
    stDiagHead.ulMsgTransId     = stDiagInfo.ulTransId;
    stDiagHead.ulChanId         = SCM_CODER_SRC_LOM_CNF;
    stDiagHead.ulDataSize       = sizeof(stCnf);
    stDiagHead.pData            = &stCnf;

    ulCnfRst = diag_ServicePackData(&stDiagHead);

    if(ERR_MSP_SUCCESS != ulCnfRst)
    {
        diag_printf("diag_GetModemInfo failed.\n");
    }
    else
    {
        diag_printf("diag_GetModemInfo success.\n");
    }

    return;
}


VOS_UINT32 diag_ConnProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 ret;
    VOS_UINT32 ulCnfRst = ERR_MSP_UNAVAILABLE;
    DIAG_MSG_MSP_CONN_STRU *pstConn;
    DIAG_CMD_HOST_CONNECT_CNF_STRU stCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    const MODEM_VER_INFO_S* pstVerInfo = VOS_NULL;
    DIAG_CMD_REPLAY_SET_REQ_STRU stReplay={0};
    DIAG_FRAME_INFO_STRU *pstDiagHead;
    DIAG_CMD_GET_MDM_INFO_REQ_STRU *pstInfo = VOS_NULL;

    diag_PTR(EN_DIAG_PTR_MSGMSP_CONN_IN);

    pstDiagHead = (DIAG_FRAME_INFO_STRU *)pstReq;

    /* 新增获取modem信息的命令用于工具查询单板信息 */
    if(sizeof(DIAG_CMD_GET_MDM_INFO_REQ_STRU) == pstDiagHead->ulMsgLen)
    {
        pstInfo = (DIAG_CMD_GET_MDM_INFO_REQ_STRU *)pstDiagHead->aucData;
        if((VOS_NULL != pstInfo) && (DIAG_GET_MODEM_INFO == pstInfo->ulInfo))
        {
            diag_GetModemInfo(pstDiagHead);
            return ERR_MSP_SUCCESS;
        }
    }

    diag_printf("Receive tool connect cmd!\n");

    pstConn = (DIAG_MSG_MSP_CONN_STRU *)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT, (VOS_UINT32)(sizeof(DIAG_MSG_MSP_CONN_STRU)-VOS_MSG_HEAD_LENGTH));
    if(VOS_NULL == pstConn)
    {
        goto DIAG_ERROR;
    }

    /*设置连接状态开关值*/
    ulCnfRst = diag_CfgSetGlobalBitValue(&g_ulDiagCfgInfo,DIAG_CFG_CONN_BIT,DIAG_CFG_SWT_OPEN);
    if(ulCnfRst)
    {
        diag_printf("Open DIAG_CFG_CONN_BIT failed.\n");
        VOS_FreeMsg(MSP_PID_DIAG_APP_AGENT, pstConn);
        goto DIAG_ERROR;
    }

    /* 关闭SOCP模块的自动降频 */
    mdrv_socp_disalbe_dfs();

    (VOS_VOID)VOS_MemSet_s(&(pstConn->stConnInfo.stBuildVersion), (VOS_UINT32)sizeof(DIAG_CMD_UE_BUILD_VER_STRU), 0, (VOS_UINT32)sizeof(DIAG_CMD_UE_BUILD_VER_STRU));

    /*获取版本信息*/
    pstVerInfo = mdrv_ver_get_info();
    if(pstVerInfo!=NULL)
    {
        pstConn->stConnInfo.stBuildVersion.usVVerNo        = pstVerInfo->stswverinfo.ulVVerNO;
        pstConn->stConnInfo.stBuildVersion.usRVerNo        = pstVerInfo->stswverinfo.ulRVerNO;
        pstConn->stConnInfo.stBuildVersion.usCVerNo        = pstVerInfo->stswverinfo.ulCVerNO;
        pstConn->stConnInfo.stBuildVersion.usBVerNo        = pstVerInfo->stswverinfo.ulBVerNO;
        pstConn->stConnInfo.stBuildVersion.usSpcNo         = pstVerInfo->stswverinfo.ulSpcNO;
        pstConn->stConnInfo.stBuildVersion.usHardwareVerNo = pstVerInfo->stswverinfo.ulCustomNOv;
        pstConn->stConnInfo.stBuildVersion.ulProductNo     = pstVerInfo->stswverinfo.ulProductNo;

        /*获取数采基地址*/
        pstConn->stConnInfo.ulChipBaseAddr = (VOS_UINT32)pstVerInfo->stproductinfo.echiptype;
    }

    /*获取IMEI号*/
    (VOS_VOID)diag_GetImei(pstConn->stConnInfo.szImei);

    /*获取软件版本号*/
    (VOS_VOID)VOS_MemSet_s(&pstConn->stConnInfo.stUeSoftVersion, (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU), 0, (VOS_UINT32)sizeof(DIAG_CMD_UE_SOFT_VERSION_STRU));

    /*路测信息获取*/
    (VOS_VOID)NV_Read(EN_NV_ID_AGENT_FLAG,&(pstConn->stConnInfo.stAgentFlag), (VOS_UINT32)sizeof(NV_ITEM_AGENT_FLAG_STRU));

    pstConn->stConnInfo.diag_cfg.UintValue = 0;

    /* 010: OM通道融合的版本 */
    /* 110: OM融合GU未融合的版本 */
    /* 100: OM完全融合的版本 */
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulDrxControlFlag = 0; /*和HIDS确认此处不再使用,打桩处理即可*/
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulPortFlag = 0;
    pstConn->stConnInfo.diag_cfg.CtrlFlag.ulOmUnifyFlag = 1;

    pstConn->stConnInfo.ulLpdMode = 0x5a5a5a5a;

    (VOS_VOID)VOS_MemCpy_s(pstConn->stConnInfo.szProduct, (VOS_UINT32)sizeof(PRODUCT_FULL_VERSION_STR), PRODUCT_FULL_VERSION_STR, (VOS_UINT32)sizeof(PRODUCT_FULL_VERSION_STR));

    ulCnfRst = diag_SendMsg(MSP_PID_DIAG_APP_AGENT,PS_PID_MM,ID_MSG_DIAG_CMD_REPLAY_TO_PS,(VOS_UINT8*)&stReplay,\
                (VOS_UINT32)sizeof(DIAG_CMD_REPLAY_SET_REQ_STRU));
    if(ulCnfRst)
    {
        VOS_FreeMsg(MSP_PID_DIAG_APP_AGENT, pstConn);
        goto DIAG_ERROR;
    }

    /*处理结果*/
    pstConn->stConnInfo.ulAuid = ((MSP_DIAG_DATA_REQ_STRU*)(pstDiagHead->aucData))->ulAuid;
    pstConn->stConnInfo.ulSn   = ((MSP_DIAG_DATA_REQ_STRU*)(pstDiagHead->aucData))->ulSn;
    pstConn->stConnInfo.ulRc   = ERR_MSP_SUCCESS;

    pstConn->ulReceiverPid = MSP_PID_DIAG_AGENT;
    pstConn->ulSenderPid   = MSP_PID_DIAG_APP_AGENT;
    pstConn->ulCmdId       = pstDiagHead->ulCmdId;
    pstConn->ulMsgId       = DIAG_MSG_MSP_CONN_REQ;

    diag_AgentVoteToSocp(SOCP_VOTE_FOR_WAKE);

    ulCnfRst = VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstConn);
    if(ERR_MSP_SUCCESS == ulCnfRst)
    {
        /* 启动定时器上报可维可测信息给工具定位丢包问题 */
        ret = VOS_StartRelTimer(&g_DebugTimer, MSP_PID_DIAG_APP_AGENT, DIAG_DEBUG_TIMER_LEN, DIAG_DEBUG_TIMER_NAME, \
                                DIAG_DEBUG_TIMER_PARA, VOS_RELTIMER_NOLOOP, VOS_TIMER_NO_PRECISION);
        if(ret != ERR_MSP_SUCCESS)
        {
            diag_printf("VOS_StartRelTimer fail [%s]\n", __FUNCTION__);
        }


        mdrv_hds_printlog_conn();

        mdrv_hds_translog_conn();

        diag_printf("Diag send ConnInfo to Modem success.\n");

        return ulCnfRst;
    }

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stCnf, pstDiagHead);

    stDiagInfo.ulMsgType    = DIAG_MSG_TYPE_MSP;

    stCnf.ulRc   = ERR_HIDS_CORE_ERROR;

    ulCnfRst = DIAG_MsgReport(&stDiagInfo, &stCnf, (VOS_UINT32)sizeof(stCnf));

    diag_printf("diag connect failed.\n");

    return ulCnfRst;
}



VOS_UINT32 diag_SetChanDisconn(MsgBlock* pMsgBlock)
{
    if(!DIAG_IS_CONN_ON)
    {
        return 0;
    }
    else
    {
        diag_CfgResetAllSwt();

        mdrv_om_set_hsoflag(0);

        /*将状态发送给C核*/
        diag_SendMsg(MSP_PID_DIAG_APP_AGENT,MSP_PID_DIAG_AGENT,ID_MSG_DIAG_HSO_DISCONN_IND, VOS_NULL, 0);
        diag_AgentVoteToSocp(SOCP_VOTE_FOR_SLEEP);
    }

    return 0;
}



VOS_UINT32 diag_DisConnProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 ret;
    DIAG_CMD_HOST_DISCONNECT_CNF_STRU stCnfDisConn = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead;

    VOS_UINT32 ulLen;
    DIAG_MSG_A_TRANS_C_STRU *pstInfo;

    diag_PTR(EN_DIAG_PTR_MSGMSP_DISCONN_IN);

    diag_printf("Receive tool disconnect cmd!\n");

    pstDiagHead = (DIAG_FRAME_INFO_STRU *)pstReq;

    /*重置所有开关状态为未打开*/
    diag_CfgResetAllSwt();

    /* 删除定时器 */
    (VOS_VOID)VOS_StopRelTimer(&g_DebugTimer);


    DIAG_MSG_ACORE_CFG_PROC(ulLen, pstDiagHead, pstInfo, ret);

    /* 打开SOCP模块的自动降频 */
    mdrv_socp_enalbe_dfs();

    /*解码投睡眠票由agent代投*/
    diag_AgentVoteToSocp(SOCP_VOTE_FOR_SLEEP);
    return ret;

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stCnfDisConn, pstDiagHead);

    stDiagInfo.ulMsgType    = DIAG_MSG_TYPE_MSP;

    stCnfDisConn.ulRc   = ERR_HIDS_CORE_ERROR;

    ret = DIAG_MsgReport(&stDiagInfo, &stCnfDisConn, (VOS_UINT32)sizeof(stCnfDisConn));

    diag_AgentVoteToSocp(SOCP_VOTE_FOR_SLEEP);

    diag_printf("diag disconnect failed.\n");

    return ret;
}




