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


#include <mdrv.h>
#include "diag_msgbsp.h"
#include "diag_debug.h"
#include "msp_diag_comm.h"
#include "diag_fs_proc.h"
#include "TafNvInterface.h"

#define    THIS_FILE_ID        MSP_FILE_ID_DIAG_MSGBSP_C
static VOS_BOOL g_ulPrivilegeRslt = FALSE;
VOS_UINT32 g_DiagHdsCmdNumC = 0;
VOS_UINT32 g_DiagHdsCmdListC[DIAG_HDS_CMD_MAX] = {0};
VOS_UINT32 g_DiagHdsCmdNumA = 0;
VOS_UINT32 g_DiagHdsCmdListA[DIAG_HDS_CMD_MAX] = {0};
DIAG_FRAME_INFO_STRU g_DiagNvAuthHead = {{0}};

VOS_UINT32 g_aulNvAuthIdList[] =
{
    en_NV_Item_IMEI,
    en_NV_Item_USB_Enum_Status,
    en_NV_Item_CustomizeSimLockPlmnInfo,
    en_NV_Item_CardlockStatus,
    en_NV_Item_CustomizeSimLockMaxTimes,
    en_NV_Item_CustomizeService,
    en_NV_Item_PRODUCT_ID,
    en_NV_Item_PREVENT_TEST_IMSI_REG,
    en_NV_Item_AT_SHELL_OPEN_FLAG,
};


/*lint -save -e433*/
VOS_UINT32 diag_NvRdProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 ret = ERR_MSP_SUCCESS;
    DIAG_CMD_NV_QRY_REQ_STRU* pstNVQryReq = NULL;
    DIAG_CMD_NV_QRY_CNF_STRU* pstNVQryCnf = NULL;
    VOS_UINT32 ulNVLen = 0, i = 0;
    VOS_UINT32 ulTotalSize = 0, ulOffset = 0;
    VOS_UINT8 *pData;
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead = NULL;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)(pstReq);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_BSP;

    pstNVQryReq = (DIAG_CMD_NV_QRY_REQ_STRU*)(pstDiagHead->aucData + sizeof(MSP_DIAG_DATA_REQ_STRU));
    if(0 == pstNVQryReq->ulCount)
    {
        diag_printf("[%s]:ulNVId=0x%x,ulNVLen=%d!\n",__FUNCTION__, pstNVQryReq->ulNVId[i], ulNVLen);
        return ERR_MSP_FAILURE;
    }
    
    for(i = 0; i < pstNVQryReq->ulCount; i++)
    {
        /*根据请求ID获取NV项长度*/
        ret = NV_GetLength(pstNVQryReq->ulNVId[i], &ulNVLen);
        if(ERR_MSP_SUCCESS != ret)
        {
            diag_printf("[%s]:ulNVId=0x%x,ulNVLen=%d!\n",__FUNCTION__, pstNVQryReq->ulNVId[i], ulNVLen);
            return ERR_MSP_FAILURE;
        }

        ulTotalSize += ulNVLen + sizeof(VOS_UINT32) + sizeof(VOS_UINT32); /* NV内容的长度加上(NVID和len各占用四字节) */
    }

    /* DIAG_CMD_NV_QRY_CNF_STRU的实际长度 */
    ulTotalSize += (sizeof(DIAG_CMD_NV_QRY_CNF_STRU) - sizeof(VOS_UINT32) - sizeof(VOS_UINT32));
    
    pstNVQryCnf = VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT, DYNAMIC_MEM_PT, ulTotalSize);
   

    if(NULL  == pstNVQryCnf)
    {
        diag_printf("[%s]:malloc error!\n",__FUNCTION__);
        return ERR_MSP_MALLOC_FAILUE;
    }

    pstNVQryCnf->ulModemid = pstNVQryReq->ulModemid;
    pstNVQryCnf->ulCount   = pstNVQryReq->ulCount;

    pData = (VOS_UINT8*)(&pstNVQryCnf->ulNVId);
    ulOffset = 0;
    
    for(i = 0; i < pstNVQryReq->ulCount; i++)
    {
        /*根据请求ID获取NV项长度*/
        ret = NV_GetLength(pstNVQryReq->ulNVId[i], &ulNVLen);
        if(ERR_MSP_SUCCESS != ret)
        {
            diag_printf("[%s]:ulNVId=0x%x,ulNVLen=%d!\n",__FUNCTION__, pstNVQryReq->ulNVId[i], ulNVLen);
            goto DIAG_ERROR;
        }

        *(VOS_UINT32*)(pData + ulOffset) = pstNVQryReq->ulNVId[i]; /* [false alarm]:屏蔽Fortify */
        ulOffset += sizeof(VOS_UINT32);

        *(VOS_UINT32*)(pData + ulOffset) = ulNVLen; /* [false alarm]:屏蔽Fortify */
        ulOffset += sizeof(VOS_UINT32);

        /*lint -save -e662 */   
        ret = NV_ReadEx(pstNVQryReq->ulModemid, pstNVQryReq->ulNVId[i], (pData + ulOffset), ulNVLen);
        /*lint -restore  */  
        if(ret != ERR_MSP_SUCCESS)
        {
            diag_printf("[%s]:NV READ ERR 0x%x,ulNVId=0x%x\n",__FUNCTION__, ret, pstNVQryReq->ulNVId[i]);
            goto DIAG_ERROR;
        }

        ulOffset += ulNVLen;
    }

    DIAG_MSG_COMMON_PROC(stDiagInfo, (*pstNVQryCnf), pstDiagHead);

    pstNVQryCnf->ulRc    = ret;

    ret = DIAG_MsgReport(&stDiagInfo, pstNVQryCnf, ulTotalSize);

    VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstNVQryCnf);

    return ret;

DIAG_ERROR:
    DIAG_MSG_COMMON_PROC(stDiagInfo, (*pstNVQryCnf), pstDiagHead);

    pstNVQryCnf->ulRc    = ERR_MSP_FAILURE;
    pstNVQryCnf->ulCount = 0;

    ret = DIAG_MsgReport(&stDiagInfo, pstNVQryCnf, sizeof(DIAG_CMD_NV_QRY_CNF_STRU));

    VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstNVQryCnf);

    return ret;
}
 /*lint -restore */


VOS_UINT32 diag_GetNvListProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 ret, ulNvNum, ulTotalLen;
    DIAG_CMD_NV_LIST_CNF_STRU* pstNVCnf = NULL;
    DIAG_CMD_NV_LIST_CNF_STRU stNVCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)(pstReq);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_BSP;

    ulNvNum = NV_GetNVIdListNum();
    if (0 == ulNvNum)
    {
        diag_printf("[%s]: ulNvNum=%d!\n",__FUNCTION__, ulNvNum);
        goto DIAG_ERROR;
    }

    /*APP_HEAD + Result + NV nums + NV ID/LEN lists*/
    ulTotalLen = sizeof(DIAG_CMD_NV_LIST_CNF_STRU) + (ulNvNum*sizeof(NV_LIST_INFO_STRU));

    pstNVCnf = (DIAG_CMD_NV_LIST_CNF_STRU*)VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT, DYNAMIC_MEM_PT, ulTotalLen);

    if (VOS_NULL_PTR == pstNVCnf)
    {
        diag_printf("[%s]:malloc error!\n",__FUNCTION__);
        goto DIAG_ERROR;
    }

    /*获取每个NV项的ID和长度*/
    ret = NV_GetNVIdList(pstNVCnf->astNvList);
    if (NV_OK != ret)
    {
        VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstNVCnf);
        goto DIAG_ERROR;
    }

    DIAG_MSG_COMMON_PROC(stDiagInfo, (*pstNVCnf), pstDiagHead);

    pstNVCnf->ulCount    = ulNvNum;
    pstNVCnf->ulRc       = ERR_MSP_SUCCESS;

    ret = DIAG_MsgReport(&stDiagInfo, pstNVCnf, ulTotalLen);

    VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstNVCnf);

    return ret;

DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stNVCnf, pstDiagHead);

    stNVCnf.ulCount      = 0;
    stNVCnf.ulRc         = ERR_MSP_FAILURE;

    ret = DIAG_MsgReport(&stDiagInfo, &stNVCnf, sizeof(stNVCnf));

    return ret;
}


VOS_UINT32 diag_GetNvResumListProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 i, ulTotalLen;
    VOS_UINT32 ret = 0;
    VOS_UINT32 ulNvNum = 0;
    VOS_UINT32 ulNvTotalNum = 0;
    VOS_UINT32 ulNvIndex = 0;
    DIAG_CMD_NV_RESUM_LIST_CNF_STRU * pstNvResumCnf;
    DIAG_CMD_NV_RESUM_LIST_CNF_STRU stNvResumCnf = {0};
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)((uintptr_t)pstReq);

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_BSP;
    
    /* get resum nv num */
    for(i=0; i<NV_ITEM_BUTT; i++)
    {
        ulNvTotalNum += NV_GetResumeNvIdNum(i);
    }
    
    ulTotalLen = (VOS_UINT32)sizeof(DIAG_CMD_NV_RESUM_LIST_CNF_STRU) + ulNvTotalNum*((VOS_UINT32)sizeof(VOS_UINT16));
        
    /* get nv list data */ 
    pstNvResumCnf = (DIAG_CMD_NV_RESUM_LIST_CNF_STRU *)VOS_MemAlloc(MSP_PID_DIAG_APP_AGENT, DYNAMIC_MEM_PT, ulTotalLen);
    if (VOS_NULL_PTR == pstNvResumCnf)
    {
        diag_printf("[%s]:malloc error!\n",__FUNCTION__);
        goto DIAG_ERROR;
    }

    for(i=0; i<NV_ITEM_BUTT; i++)
    {
        ulNvNum = NV_GetResumeNvIdNum(i);
        ret = NV_GetResumeNvIdList(i, (VOS_UCHAR *)&pstNvResumCnf->nvResumItem[ulNvIndex], ulNvNum);
        if(ret)
        {
            VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstNvResumCnf);
            diag_printf("[%s]:get nv resum list error!\n",__FUNCTION__);
            goto DIAG_ERROR;
        }
        ulNvIndex += ulNvNum;
    }

    DIAG_MSG_COMMON_PROC(stDiagInfo, (*pstNvResumCnf), (uintptr_t)pstDiagHead);

    pstNvResumCnf->ulCount    = ulNvTotalNum;
    pstNvResumCnf->ulRc       = ERR_MSP_SUCCESS;

    ret = DIAG_MsgReport(&stDiagInfo, pstNvResumCnf, ulTotalLen);

    VOS_MemFree(MSP_PID_DIAG_APP_AGENT, pstNvResumCnf);
    
    return ret;
    
DIAG_ERROR:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stNvResumCnf, (uintptr_t)pstDiagHead);
    stNvResumCnf.ulCount = 0;
    stNvResumCnf.ulRc = ERR_MSP_FAILURE;
    ret = DIAG_MsgReport(&stDiagInfo, &stNvResumCnf, (VOS_UINT32)sizeof(stNvResumCnf));

    return ret;
    
}


VOS_VOID diag_InitAuthVariable(VOS_VOID)
{
    IMEI_STRU stIMEI;
    VOS_UINT8 aucDefaultIMEI[] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

    /*假如IMEI为默认值，则不需要鉴权*/
    if (NV_OK == NV_Read(en_NV_Item_IMEI, (VOS_VOID*)&stIMEI, sizeof(stIMEI)))
    {
        if (0 == VOS_MemCmp((VOS_CHAR*)aucDefaultIMEI, &stIMEI, sizeof(stIMEI)))
        {
            g_ulPrivilegeRslt = TRUE;
        }
    }

    return;
}


VOS_UINT32 diag_IsAuthNv(VOS_UINT32 ulNvId)
{
    VOS_UINT32 i;

    for (i = 0; i < (sizeof(g_aulNvAuthIdList)/sizeof(VOS_UINT32)); i++)
    {
        if (ulNvId == g_aulNvAuthIdList[i])
        {
            if (TRUE == g_ulPrivilegeRslt)
            {
                return VOS_YES;
            }
            return VOS_NO;
        }
    }

    return VOS_YES;
}


VOS_UINT32 diag_NvWrProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 ret;
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_CMD_NV_WR_REQ_STRU* pstNVWRReq = NULL;
    DIAG_FRAME_INFO_STRU *pstDiagHead = NULL;
    DIAG_CMD_NV_WR_CNF_STRU stNVWRCnf = {0};
    VOS_UINT32 i = 0;
    VOS_UINT32 ulOffset = 0;
    VOS_UINT32 ulNvid, ulLen;
    VOS_UINT8 *pData;

    pstDiagHead = (DIAG_FRAME_INFO_STRU*)(pstReq);

    pstNVWRReq = (DIAG_CMD_NV_WR_REQ_STRU*)(pstDiagHead->aucData + sizeof(MSP_DIAG_DATA_REQ_STRU));

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_BSP;

    pData = (VOS_UINT8*)(&pstNVWRReq->ulNVId);

    for(i = 0; i < pstNVWRReq->ulCount; i++)
    {
        ulNvid    = *(VOS_UINT32*)(pData + ulOffset);
        ulOffset += sizeof(VOS_UINT32);

        ulLen     = *(VOS_UINT32*)(pData + ulOffset);
        ulOffset += sizeof(VOS_UINT32);
        
        if (VOS_YES != diag_IsAuthNv(ulNvid))
        {
            goto DIAG_ERROR2;
        }

        printk(KERN_ERR "NV Write ulNVId=0x%x\n", ulNvid);
      
        /*写入NV项*/
        /*lint -save -e662 */
        ret = NV_WriteEx(pstNVWRReq->ulModemid, ulNvid, (pData + ulOffset), ulLen);
        /*lint -restore  */  
        if(ret != ERR_MSP_SUCCESS)
        {
           printk(KERN_ERR "[%s]:NV Write ERR 0x%x,ulNVId=0x%x\n",__FUNCTION__, ret, ulNvid);
           goto DIAG_ERROR2;
        }
        /*将Nv项刷写到flash中*/
        ret = mdrv_nv_flush();
        if(ret)
        {
            printk(KERN_ERR "[%s]:flush nv ERR 0x%x,ulNVId=0x%x\n",__FUNCTION__, ret, ulNvid);
            goto DIAG_ERROR2;
        }
        ulOffset += ulLen;
    }

    DIAG_MSG_COMMON_PROC(stDiagInfo, stNVWRCnf, pstDiagHead);

    stNVWRCnf.ulModemid  = pstNVWRReq->ulModemid;
    stNVWRCnf.ulRc       = ret;

    ret = DIAG_MsgReport(&stDiagInfo, &stNVWRCnf, sizeof(stNVWRCnf));

    return ret;

DIAG_ERROR2:

    DIAG_MSG_COMMON_PROC(stDiagInfo, stNVWRCnf, pstDiagHead);

    stNVWRCnf.ulModemid  = pstNVWRReq->ulModemid;
    stNVWRCnf.ulRc       = ERR_MSP_FAILURE;

    ret = DIAG_MsgReport(&stDiagInfo, &stNVWRCnf, sizeof(stNVWRCnf));

    return ret;
}
/*lint -save -e826*/
VOS_VOID diag_AuthNvCfg(MsgBlock* pMsgBlock)
{
    DIAG_CMD_NV_AUTH_CNF_STRU  stAuthCnf = {0};
    MSP_DIAG_CNF_INFO_STRU     stDiagInfo = {0};
    MTA_DIAG_RSA_VERIFY_CNF_STRU *pstMsg = (MTA_DIAG_RSA_VERIFY_CNF_STRU *)pMsgBlock;

    if (TRUE == pstMsg->ulVerifyRslt)
    {
        g_ulPrivilegeRslt = pstMsg->ulVerifyRslt;
        stAuthCnf.ulRc = VOS_OK;
    }
    else if (FALSE == pstMsg->ulVerifyRslt)
    {
        g_ulPrivilegeRslt = pstMsg->ulVerifyRslt;
        stAuthCnf.ulRc = VOS_ERR;
    }
    else
    {
        diag_printf("[%s]:nv_auth result exception!\n",__FUNCTION__);
        stAuthCnf.ulRc = VOS_ERR;
    }

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_BSP;

    DIAG_MSG_COMMON_PROC(stDiagInfo, stAuthCnf, ((DIAG_FRAME_INFO_STRU *)(&g_DiagNvAuthHead)));

    if (VOS_OK != DIAG_MsgReport(&stDiagInfo, &stAuthCnf, (VOS_INT)sizeof(stAuthCnf)))
    {
        diag_printf("%s:diag nv auth msg report fail!\n", __FUNCTION__);
    }

}

VOS_UINT32 diag_NvAuthProc(VOS_UINT8* pstReq)
{
    VOS_UINT32 ulRet;
    MSP_DIAG_CNF_INFO_STRU     stDiagInfo = {0};
    DIAG_CMD_NV_AUTH_REQ_STRU *pstAuthReq;
    DIAG_CMD_NV_AUTH_CNF_STRU  stAuthCnf = {0};
    DIAG_MTA_RSA_VERIFY_REQ_STRU *pstNvAuth;
    DIAG_FRAME_INFO_STRU *pstNvHead = (DIAG_FRAME_INFO_STRU *)pstReq;

    pstAuthReq = (DIAG_CMD_NV_AUTH_REQ_STRU*)(pstNvHead->aucData + sizeof(MSP_DIAG_DATA_REQ_STRU));

    pstNvAuth = (DIAG_MTA_RSA_VERIFY_REQ_STRU *)VOS_AllocMsg(MSP_PID_DIAG_APP_AGENT,(VOS_INT)(sizeof(DIAG_MTA_RSA_VERIFY_REQ_STRU) - VOS_MSG_HEAD_LENGTH));
    if(pstNvAuth == NULL)
    {
        diag_printf("%s: alloc msg fail\n", __FUNCTION__);
        return ERR_MSP_FAILURE;
    }

    pstNvAuth->ulReceiverPid  = I0_UEPS_PID_MTA;
    pstNvAuth->ulMsgId        = ID_DIAG_MTA_RSA_VERIFY_REQ;
    (VOS_VOID)VOS_MemSet_s(pstNvAuth->aucSecString, MTA_RSA_CIPHERTEXT_LEN, 0, MTA_RSA_CIPHERTEXT_LEN);
    (VOS_VOID)VOS_MemCpy_s(pstNvAuth->aucSecString, sizeof(pstNvAuth->aucSecString),pstAuthReq->aucAuth, pstAuthReq->ulLen);

    if(ERR_MSP_SUCCESS != VOS_SendMsg(MSP_PID_DIAG_APP_AGENT, pstNvAuth))
    {
        diag_printf("%s: diag send msg to mta fail\n", __FUNCTION__);

        stAuthCnf.ulRc = VOS_ERR;

        stDiagInfo.ulMsgType = DIAG_MSG_TYPE_BSP;

        DIAG_MSG_COMMON_PROC(stDiagInfo, stAuthCnf, pstNvHead);

        ulRet = DIAG_MsgReport(&stDiagInfo, &stAuthCnf, (VOS_INT)sizeof(stAuthCnf));

        return ulRet;
    }

    else
    {
        VOS_MemCpy_s(&g_DiagNvAuthHead, sizeof(g_DiagNvAuthHead), pstNvHead, sizeof(DIAG_FRAME_INFO_STRU));
        return ERR_MSP_SUCCESS;
    }
}
/*lint -restore +e826*/
VOS_VOID diag_BspRecvCmdList(MsgBlock* pMsgBlock)
{
    DIAG_BSP_CMDLIST_STRU *pstMsg = (DIAG_BSP_CMDLIST_STRU *)pMsgBlock;

    g_DiagHdsCmdNumC = pstMsg->ulCmdNum;
    VOS_MemCpy_s(g_DiagHdsCmdListC, DIAG_HDS_CMD_MAX*sizeof(VOS_UINT32),  pstMsg->ulCmdList, g_DiagHdsCmdNumC*sizeof(VOS_UINT32));

    diag_printf("%s Rcv ccore cmd list, num: 0x%x\n",__FUNCTION__, g_DiagHdsCmdNumC);

    return;
}

VOS_VOID diag_BspCmdListInit(VOS_VOID)
{
    g_DiagHdsCmdNumA = DIAG_HDS_CMD_MAX;
    mdrv_hds_get_cmdlist(g_DiagHdsCmdListA, &g_DiagHdsCmdNumA);
}

VOS_BOOL diag_BspIsAcoreCmd(VOS_UINT32 ulCmdId)
{
    unsigned int i;

    if(!ulCmdId)
        return VOS_FALSE;

    for(i=0; i<g_DiagHdsCmdNumA; i++)
    {
        if(g_DiagHdsCmdListA[i] == ulCmdId)
        {
            return VOS_TRUE;
        }
    }

    return VOS_FALSE;
}


VOS_BOOL diag_BspIsCcoreCmd(VOS_UINT32 ulCmdId)
{
    unsigned int i;

    if(!ulCmdId)
        return VOS_FALSE;

    for(i=0; i<g_DiagHdsCmdNumC; i++)
    {
        if(g_DiagHdsCmdListC[i] == ulCmdId)
        {
            return VOS_TRUE;
        }
    }

    return VOS_FALSE;
}

VOS_UINT32 diag_BspMsgProc(DIAG_FRAME_INFO_STRU *pData)
{
    VOS_UINT32 ulRet = ERR_MSP_INVALID_PARAMETER ;
    MSP_DIAG_CNF_INFO_STRU stDiagInfo = {0};
    DIAG_BSP_COMM_CNF_STRU stCnf = {0};
    DIAG_FRAME_INFO_STRU *pstDiagHead = pData;
    DIAG_BSP_MSG_A_TRANS_C_STRU *pstInfo;
    VOS_UINT32 ulLen = 0;
    VOS_UINT32 acmd_flag = 0;
    VOS_UINT32 ccmd_flag = 0;

    if(DIAG_MSG_TYPE_BSP != pData->stID.pri4b)
    {
        diag_printf("%s Rcv Error Msg Id 0x%x\n",__FUNCTION__,pData->ulCmdId);
        return ulRet;
    }

    acmd_flag = diag_BspIsAcoreCmd(pData->ulCmdId);
    ccmd_flag = diag_BspIsCcoreCmd(pData->ulCmdId);

    if  ((VOS_TRUE == acmd_flag) && (VOS_FALSE == ccmd_flag))
    {
        ulRet = mdrv_hds_msg_proc((VOS_VOID *)pData);
        if(ulRet != 0)
        {
            diag_printf("acore msg_proc fail!\n",__FUNCTION__);
        }

        return ulRet;
    }
    else if ((VOS_FALSE == acmd_flag) && (VOS_TRUE == ccmd_flag))
    {
        /*通知ccore*/
        DIAG_MSG_BSP_ACORE_CFG_PROC(ulLen, pData, pstInfo, ulRet);
        return VOS_OK;
    }
    else if ((VOS_TRUE == acmd_flag) && (VOS_TRUE == ccmd_flag))
    {
        /*A核处理成功后通知ccore，A核处理不成功直接向工具回复*/
        ulRet = mdrv_hds_msg_proc((VOS_VOID*)pData);
        if(ulRet != 0)
        {
            diag_printf("comm_acore msg_proc fail!\n",__FUNCTION__);
            return ulRet;
        }

        DIAG_MSG_BSP_ACORE_CFG_PROC(ulLen, pData, pstInfo, ulRet);
        return VOS_OK;
    }
    else
    {
        //todo...
    }

    return ulRet;

DIAG_ERROR:

    stDiagInfo.ulMsgType = DIAG_MSG_TYPE_BSP;

    DIAG_MSG_COMMON_PROC(stDiagInfo, stCnf, pstDiagHead);

    stCnf.ulRet = ERR_MSP_FAILURE;

    ulRet = DIAG_MsgReport(&stDiagInfo, &stCnf, sizeof(stCnf));

    return ulRet;

}

/*****************************************************************************
 Function Name   : diag_BspMsgInit
 Description     : MSP dsp部分初始化
 Input           : None
 Output          : None
 Return          : None
 History         :

*****************************************************************************/
VOS_VOID diag_BspMsgInit(VOS_VOID)
{
    /*注册message消息回调*/
    DIAG_MsgProcReg(DIAG_MSG_TYPE_BSP,diag_BspMsgProc);
    mdrv_hds_cnf_register((hds_cnf_func)DIAG_MsgReport);
    diag_nvInit();
    diag_fsInit();
    diag_BspCmdListInit();
}

VOS_VOID diag_nvInit(VOS_VOID)
{
    mdrv_hds_cmd_register(DIAG_CMD_NV_WR, (bsp_hds_func)diag_NvWrProc);
    mdrv_hds_cmd_register(DIAG_CMD_NV_RD, (bsp_hds_func)diag_NvRdProc);
    mdrv_hds_cmd_register(DIAG_CMD_GET_NV_LIST, (bsp_hds_func)diag_GetNvListProc);
    mdrv_hds_cmd_register(DIAG_CMD_NV_AUTH, (bsp_hds_func)diag_NvAuthProc);
    mdrv_hds_cmd_register(DIAG_CMD_GET_NV_RESUM_LIST, (bsp_hds_func)diag_GetNvResumListProc);
}




