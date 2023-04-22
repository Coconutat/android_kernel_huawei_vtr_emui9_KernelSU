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
#include "diag_mem.h"
#include "diag_debug.h"
#include "SCMProc.h"
#include "soc_socp_adapter.h"
#include "cpm.h"
#include "LNvCommon.h"
#include "LPsNvInterface.h"
#include "SCMSoftDecode.h"
#include "OmCommonPpm.h"


#define    THIS_FILE_ID        PS_FILE_ID_SCM_PROC_C

/* ****************************************************************************
  2 全局变量定义
**************************************************************************** */
extern SCM_SOFTDECODE_INFO_STRU   g_stScmSoftDecodeInfo;
extern OM_ACPU_DEBUG_INFO         g_stAcpuDebugInfo;



/* 解码目的通道回调函数 */
SCM_DECODERDESTFUCN         g_astSCMDecoderCbFunc[SOCP_DECODER_DST_CB_BUTT]={VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR};

SCM_CODER_SRC_CFG_STRU      g_astSCMCoderSrcCfg[SCM_CODER_SRC_NUM]=
{
    {SCM_CHANNEL_UNINIT, SOCP_CODER_SRC_LOM_CNF1, SOCP_CODER_DST_OM_CNF,   SOCP_DATA_TYPE_0, SOCP_ENCSRC_CHNMODE_CTSPACKET, SOCP_CHAN_PRIORITY_2, SCM_CODER_SRC_BDSIZE, SCM_CODER_SRC_RDSIZE, VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR},
    {SCM_CHANNEL_UNINIT, SOCP_CODER_SRC_LOM_IND1, SOCP_CODER_DST_OM_IND,   SOCP_DATA_TYPE_0, SOCP_ENCSRC_CHNMODE_CTSPACKET, SOCP_CHAN_PRIORITY_2, SCM_CODER_SRC_BDSIZE, SCM_CODER_SRC_RDSIZE, VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR}
};

SCM_CODER_DEST_CFG_STRU     g_astSCMCoderDstCfg[SCM_CODER_DST_NUM]=
{
    {SCM_CHANNEL_UNINIT, SOCP_CODER_DST_OM_CNF, SCM_CODER_DST_CNF_SIZE, SCM_CODER_DST_THRESHOLD,  VOS_NULL_PTR, VOS_NULL_PTR,  VOS_NULL_PTR},
    {SCM_CHANNEL_UNINIT, SOCP_CODER_DST_OM_IND, SCM_CODER_DST_IND_SIZE, SCM_CODER_DST_THRESHOLD,  VOS_NULL_PTR, VOS_NULL_PTR,  VOS_NULL_PTR},
};

SCM_DECODER_SRC_CFG_STRU    g_astSCMDecoderSrcCfg[SCM_DECODER_SRC_NUM]=
{
    {SCM_CHANNEL_UNINIT, SOCP_DECODER_SRC_LOM, SCM_DECODER_SRC_SIZE, VOS_NULL, VOS_NULL_PTR, VOS_NULL_PTR}
};

SCM_DECODER_DEST_CFG_STRU   g_astSCMDecoderDstCfg[SCM_DECODER_DST_NUM]=
{
    {SCM_CHANNEL_UNINIT, SOCP_DECODER_DST_LOM, SOCP_DECODER_SRC_LOM, SOCP_DATA_TYPE_0, SCM_DECODER_DST_SIZE, SCM_DECDOER_DST_THRESHOLD, VOS_NULL_PTR, VOS_NULL_PTR, VOS_NULL_PTR}
};



SCM_INFODATA_STRU           g_stSCMInfoData;    /* 用于保存log信息 */


/* 自旋锁，用来作编码源buff的临界资源保护 */
VOS_SPINLOCK             g_stScmGetCoderSrcBuffSpinLock;


SCM_CODER_SRC_DEBUG_STRU g_astScmCoderSrcDebugInfo[SCM_CODER_SRC_NUM] = {{0}};


/*****************************************************************************
  3 外部引用声明
*****************************************************************************/

/*****************************************************************************
  4 函数实现
*****************************************************************************/


VOS_UINT32  SCM_FindChannelCfg(VOS_UINT32           ulChannelID,
                                    VOS_UINT32              ulChMax,
                                    SCM_CHANNEL_CFG_HEAD    *pstCfg,
                                    VOS_UINT32               ulStruLen,
                                    VOS_UINT32              *pulNum)
{
    VOS_UINT32                          i;
    SCM_CHANNEL_CFG_HEAD               *pstTmpCfg = pstCfg;

    for(i=0; i<ulChMax; i++)
    {
        if((pstTmpCfg->ulChannelID == ulChannelID)
            &&(SCM_CHANNEL_INIT_SUCC == pstTmpCfg->enInitState))
        {
            *pulNum = i;

            return VOS_OK;/* 返回成功 */
        }

        pstTmpCfg = (SCM_CHANNEL_CFG_HEAD *)((VOS_UINT8 *)pstTmpCfg + ulStruLen);
    }

    return VOS_ERR;/* 返回失败 */
}


VOS_UINT32 SCM_CoderSrcChannelCfg(SCM_CODER_SRC_CFG_STRU *pstCfg)
{
    SOCP_CODER_SRC_CHAN_S               stChannle;          /* 当前通道的属性信息 */

    stChannle.u32DestChanID = pstCfg->enDstCHID;            /*  目标通道ID */
    stChannle.eDataType     = pstCfg->enDataType;           /*  数据类型，指明数据封装协议，用于复用多平台 */
    stChannle.eMode         = pstCfg->enCHMode;             /*  通道数据模式 */
    stChannle.ePriority     = pstCfg->enCHLevel;            /*  通道优先级 */
    stChannle.u32BypassEn   = SOCP_HDLC_ENABLE;             /*  通道bypass使能 */
    stChannle.eDataTypeEn   = SOCP_DATA_TYPE_EN;            /*  数据类型使能位 */
    stChannle.eDebugEn      = SOCP_ENC_DEBUG_DIS;           /*  调试位使能 */

    stChannle.sCoderSetSrcBuf.pucInputStart  = pstCfg->pucSrcPHY;                             /*  输入通道起始地址 */
    stChannle.sCoderSetSrcBuf.pucInputEnd    = (pstCfg->pucSrcPHY + pstCfg->ulSrcBufLen)-1;   /*  输入通道结束地址 */
    stChannle.sCoderSetSrcBuf.pucRDStart     = pstCfg->pucRDPHY;                              /* RD buffer起始地址 */
    stChannle.sCoderSetSrcBuf.pucRDEnd       = (pstCfg->pucRDPHY + pstCfg->ulRDBufLen)-1;     /*  RD buffer结束地址 */
    stChannle.sCoderSetSrcBuf.u32RDThreshold = SCM_CODER_SRC_RD_THRESHOLD;                    /* RD buffer数据上报阈值 */

    if (VOS_OK != mdrv_socp_corder_set_src_chan(pstCfg->enChannelID, &stChannle))
    {
        SCM_CODER_SRC_ERR("SCM_CoderSrcChannelCfg: Search Channel ID Error", pstCfg->enChannelID, 0);/* 打印失败 */

        return VOS_ERR;/* 返回错误 */
    }

    pstCfg->enInitState = SCM_CHANNEL_INIT_SUCC; /* 记录通道初始化配置错误 */

    return VOS_OK;/* 返回成功 */
}


VOS_UINT32 SCM_CoderSrcChannelInit(VOS_VOID)
{
    VOS_UINT32                          i;

    VOS_SpinLockInit(&g_stScmGetCoderSrcBuffSpinLock);

    for (i = 0; i < SCM_CODER_SRC_NUM; i++)
    {
        /* TODO: CJQ */
        if(g_astSCMCoderSrcCfg[i].enChannelID == SOCP_CODER_SRC_LOM_IND3)
        {
            g_astSCMCoderSrcCfg[i].enInitState = SCM_CHANNEL_CFG_FAIL;
            continue;
        }

        if (VOS_OK != SCM_CoderSrcChannelCfg(&g_astSCMCoderSrcCfg[i]))
        {
            g_astSCMCoderSrcCfg[i].enInitState = SCM_CHANNEL_CFG_FAIL;  /* 记录通道初始化配置错误 */

            return VOS_ERR;/* 返回失败 */
        }

        if(VOS_OK != mdrv_socp_start(g_astSCMCoderSrcCfg[i].enChannelID))
        {
            g_astSCMCoderSrcCfg[i].enInitState = SCM_CHANNEL_START_FAIL;  /* 记录通道开启配置错误 */

            return VOS_ERR;/* 返回失败 */
        }

        g_astSCMCoderSrcCfg[i].enInitState = SCM_CHANNEL_INIT_SUCC;     /* 记录通道初始化配置错误 */
    }

    return VOS_OK;/* 返回成功 */
}


VOS_UINT32 SCM_RlsSrcRDAll(SOCP_CODER_SRC_ENUM_U32 enChanlID, VOS_UINT_PTR *pDataPhyAddr, VOS_UINT32 *pulDataLen)
{
    SOCP_BUFFER_RW_STRU                 stSrcChanRD;
    SOCP_RD_DATA_STRU                  *pstRDData;
    VOS_UINT32                          ulRDNum;
    VOS_UINT32                          ulRDTotalNum= 0;
    VOS_UINT32                          ulTotalLen  = 0;
    VOS_UINT32                          ulFirstAddr = 0;
    VOS_UINT32                          ulCfgNum;
    VOS_UINT32                          i;
    SCM_CHANNEL_ENUM_U32                enLteMark   = SCM_CHANNEL_BUTT;/*标志通道类型*/
    VOS_UINT32                          ulen = 0;

    if((SOCP_CODER_SRC_LOM_CNF1 == enChanlID)||(SOCP_CODER_SRC_LOM_CNF2 == enChanlID)
        ||(SOCP_CODER_SRC_LOM_IND1 == enChanlID)||(SOCP_CODER_SRC_LOM_IND2 == enChanlID)
        ||(SOCP_CODER_SRC_LOM_IND3 == enChanlID))
    {
          enLteMark = SCM_LTE_CHANNEL;
    }
    (VOS_VOID)VOS_MemSet_s(&stSrcChanRD, (VOS_UINT32)sizeof(stSrcChanRD), 0, sizeof(stSrcChanRD));

    if ( VOS_OK != SCM_FindChannelCfg(enChanlID,
                                        SCM_CODER_SRC_NUM,
                                        (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderSrcCfg,
                                        sizeof(SCM_CODER_SRC_CFG_STRU),
                                        &ulCfgNum))/* 判断通道参数 */
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDAll: Find Channel Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    SCM_CODER_SRC_LOG("SCM_RlsSrcRDAll: Release Channel Data", enChanlID, 0);

    if (VOS_OK != mdrv_socp_get_rd_buffer(enChanlID, &stSrcChanRD))
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDAll: Get RD Info Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    if (VOS_NULL_PTR == stSrcChanRD.pBuffer)        /* 参数错误 */
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDAll: RD Info is Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    if ((0 == stSrcChanRD.u32Size) && (0 == stSrcChanRD.u32RbSize)) /* 无数据需要释放 */
    {
        *pDataPhyAddr   = 0;
        *pulDataLen     = 0;

        return VOS_OK;/* 返回成功 */
    }

    /* RD个数获取非法 */
    if(((stSrcChanRD.u32Size + stSrcChanRD.u32RbSize) / sizeof(SOCP_RD_DATA_STRU)) > SCM_CODE_SRC_RD_NUM)
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: Get RD Data Error", enChanlID, 0);/* 记录Log */
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: Get RD PTR Error", stSrcChanRD.u32Size, stSrcChanRD.u32RbSize);/* 记录Log */

        return VOS_ERR;/* 返回失败 */
    }

    /* 计算RD个数 */
    ulRDNum = stSrcChanRD.u32Size / sizeof(SOCP_RD_DATA_STRU);

    if (0 != ulRDNum)
    {
        stSrcChanRD.pBuffer = (VOS_CHAR*)VOS_UncacheMemPhyToVirt((VOS_UINT8*)stSrcChanRD.pBuffer,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDPHY,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDBuf,
                                    g_astSCMCoderSrcCfg[ulCfgNum].ulRDBufLen);

        ulRDTotalNum = ulRDNum;

        /* 获取RD数据的地址和长度 */
        pstRDData   = (SOCP_RD_DATA_STRU*)stSrcChanRD.pBuffer;

        /* 记录当前第一个释放的数据地址,后面需要返回给上层 */
        ulFirstAddr = pstRDData->ulDataAddr;

        for (i = 0; i < ulRDNum; i++)
        {
            ulen    = 0;

            /* 累计RD数据长度 */
            if(SCM_LTE_CHANNEL == enLteMark)
            {
                ulen = ALIGN_DDR_WITH_4BYTE(pstRDData->usMsgLen);
            }
            else
            {
                ulen = pstRDData->usMsgLen;
            }
            ulTotalLen += ulen;
            pstRDData++;
        }
    }

    /* 计算回卷RD个数 */
    ulRDNum = stSrcChanRD.u32RbSize / sizeof(SOCP_RD_DATA_STRU);

    if (0 != ulRDNum)
    {
        stSrcChanRD.pRbBuffer = (VOS_CHAR*)VOS_UncacheMemPhyToVirt((VOS_UINT8*)stSrcChanRD.pRbBuffer,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDPHY,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDBuf,
                                    g_astSCMCoderSrcCfg[ulCfgNum].ulRDBufLen);

        ulRDTotalNum += ulRDNum;

        /* 获取RD数据回卷的地址和长度 */
        pstRDData   = (SOCP_RD_DATA_STRU*)stSrcChanRD.pRbBuffer;

        if (0 == ulFirstAddr)/* 记录当前第一个释放的数据地址,后面需要返回给上层 */
        {
            ulFirstAddr = pstRDData->ulDataAddr;
        }

        for (i = 0; i < ulRDNum; i++)
        {
            ulen    = 0;

            /* 累计RD数据长度 */
            if(SCM_LTE_CHANNEL == enLteMark)
            {
                ulen = ALIGN_DDR_WITH_4BYTE((pstRDData->usMsgLen));
            }
            else
            {
                ulen = pstRDData->usMsgLen;
            }
            ulTotalLen += ulen;

            pstRDData++;
        }
    }

    if (VOS_OK != mdrv_socp_read_rd_done(enChanlID, (stSrcChanRD.u32Size+stSrcChanRD.u32RbSize)))
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDAll: Write RD Done is Error", enChanlID, (stSrcChanRD.u32Size+stSrcChanRD.u32RbSize));/* 记录Log */
        return VOS_ERR;/* 返回错误 */
    }

    /* 获取的RD最大值记录到全局变量中 */
    if (ulRDTotalNum > g_stSCMInfoData.aulRDUsedMax[ulCfgNum])
    {
        g_stSCMInfoData.aulRDUsedMax[ulCfgNum] = ulRDTotalNum;
    }

    *pDataPhyAddr   = ulFirstAddr;

    *pulDataLen     = ulTotalLen;

    return VOS_OK;/* 返回结果OK */
}


VOS_UINT32 SCM_RlsSrcRDFirst(SOCP_CODER_SRC_ENUM_U32 enChanlID,
                                    VOS_UINT_PTR *pDataPhyAddr,
                                    VOS_UINT32   *pulDataLen)
{
    SOCP_BUFFER_RW_STRU                 stSrcChanRD;
    SOCP_RD_DATA_STRU                  *pstRDData;
    VOS_UINT32                          ulRDTotalNum = 0;
    VOS_UINT32                          ulCfgNum;
    VOS_UINT32                          ulResult;

    (VOS_VOID)VOS_MemSet_s(&stSrcChanRD, (VOS_UINT32)sizeof(stSrcChanRD), 0, sizeof(stSrcChanRD));

    if (VOS_OK != SCM_FindChannelCfg(enChanlID, SCM_CODER_SRC_NUM,
                                (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderSrcCfg,
                                sizeof(SCM_CODER_SRC_CFG_STRU),
                                &ulCfgNum))/* 判断通道参数 */
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: Find Channel Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    SCM_CODER_SRC_LOG("SCM_RlsSrcRDFirst: Release Channel Data", enChanlID, 0);

    if (VOS_OK != mdrv_socp_get_rd_buffer(enChanlID, &stSrcChanRD))
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: Get RD Buffer Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    if (VOS_NULL_PTR == stSrcChanRD.pBuffer)        /* 参数错误 */
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: RD Buffer is  Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    /* RD个数获取非法 */
    if(((stSrcChanRD.u32Size + stSrcChanRD.u32RbSize) / sizeof(SOCP_RD_DATA_STRU)) > SCM_CODE_SRC_RD_NUM)
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: Get RD Data Error", enChanlID, 0);/* 记录Log */
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: Get RD PTR Error", stSrcChanRD.u32Size, stSrcChanRD.u32RbSize);/* 记录Log */

        return VOS_ERR;/* 返回失败 */
    }

    if (0 != stSrcChanRD.u32Size)      /* RD有数据 */
    {
        stSrcChanRD.pBuffer = (VOS_CHAR*)VOS_UncacheMemPhyToVirt((VOS_UINT8*)stSrcChanRD.pBuffer,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDPHY,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDBuf,
                                    g_astSCMCoderSrcCfg[ulCfgNum].ulRDBufLen);

        /* 获取RD第一包数据首地址和长度 */
        pstRDData = (SOCP_RD_DATA_STRU *)stSrcChanRD.pBuffer;

        *pDataPhyAddr   = (VOS_UINT_PTR)(pstRDData->ulDataAddr);
        *pulDataLen     = (VOS_UINT32)pstRDData->usMsgLen;

        ulResult = (VOS_UINT32)mdrv_socp_read_rd_done(enChanlID, (VOS_UINT32)sizeof(SOCP_RD_DATA_STRU));  /* 释放一包数据 */
    }
    else if (0 != stSrcChanRD.u32RbSize)/* RD回卷有数据 */
    {
        stSrcChanRD.pRbBuffer = (VOS_CHAR*)VOS_UncacheMemPhyToVirt((VOS_UINT8*)stSrcChanRD.pRbBuffer,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDPHY,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucRDBuf,
                                    g_astSCMCoderSrcCfg[ulCfgNum].ulRDBufLen);

        /* 获取回卷RD第一包数据首地址和长度 */
        pstRDData = (SOCP_RD_DATA_STRU *)stSrcChanRD.pRbBuffer;

        *pDataPhyAddr   = (VOS_UINT_PTR)(pstRDData->ulDataAddr);
        *pulDataLen     = (VOS_UINT32)pstRDData->usMsgLen;

        ulResult = (VOS_UINT32)mdrv_socp_read_rd_done(enChanlID, (VOS_UINT32)sizeof(SOCP_RD_DATA_STRU));  /* 释放一包数据 */
    }
    else         /* 无数据需要释放 */
    {
        *pDataPhyAddr   = 0;
        *pulDataLen     = 0;
        ulResult        = VOS_OK;
    }

    if (VOS_OK != ulResult)
    {
        SCM_CODER_SRC_ERR("SCM_RlsSrcRDFirst: Read RD Done is  Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回错误 */
    }

    /* 计算RD总大小，记录全局变量中 */
    ulRDTotalNum = (stSrcChanRD.u32Size + stSrcChanRD.u32RbSize) / sizeof(SOCP_RD_DATA_STRU);

    if (ulRDTotalNum > g_stSCMInfoData.aulRDUsedMax[ulCfgNum])
    {
        g_stSCMInfoData.aulRDUsedMax[ulCfgNum] = ulRDTotalNum;
    }

    return VOS_OK;/* 返回成功 */
}



VOS_UINT32 SCM_GetBDFreeNum(SOCP_CODER_SRC_ENUM_U32 enChanlID, VOS_UINT32 *pulBDNum)
{
    SOCP_BUFFER_RW_STRU                 stRwBuf;
    VOS_UINT32                          ulBDNum;
    VOS_UINT32                          ulBDTotal;
    VOS_UINT32                          ulCfgNum;

    /* 判断指针的正确 */
    if (VOS_NULL_PTR == pulBDNum)
    {
        return VOS_ERR;
    }

    if (VOS_OK != SCM_FindChannelCfg(enChanlID, SCM_CODER_SRC_NUM,
                                (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderSrcCfg, sizeof(SCM_CODER_SRC_CFG_STRU), &ulCfgNum))/* 判断通道参数 */
    {
        SCM_CODER_SRC_ERR("SCM_GetBDFreeNum: Find Channel Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    SCM_CODER_SRC_LOG("SCM_GetBDFreeNum: Get BD Number", enChanlID, 0);

    if (VOS_OK != mdrv_socp_get_write_buff(enChanlID, &stRwBuf))
    {
        SCM_CODER_SRC_ERR("SCM_GetBDFreeNum: Get Write Buffer Error", enChanlID, 0);/* 记录Log */
        return VOS_ERR;/* 返回失败 */
    }

    /* 计算BD的值 */
    ulBDNum = (stRwBuf.u32Size + stRwBuf.u32RbSize) / sizeof(SOCP_BD_DATA_STRU);

    /* BD个数获取非法 */
    if(ulBDNum > SCM_CODE_SRC_BD_NUM)
    {
        SCM_CODER_SRC_ERR("SCM_GetBDFreeNum: Get BD Data Error", enChanlID, 0);/* 记录Log */
        SCM_CODER_SRC_ERR("SCM_GetBDFreeNum: Get BD PTR Error", stRwBuf.u32Size, stRwBuf.u32RbSize);/* 记录Log */

        return VOS_ERR;/* 返回失败 */
    }

    /* 至少要保留一个空闲BD，保证通道不会被写满而异常 */
    if (ulBDNum <= 1)
    {
        *pulBDNum = 0;
    }
    else
    {
        *pulBDNum = (ulBDNum - 1);
    }

    /* 计算通道全部BD的个数 */
    ulBDTotal = g_astSCMCoderSrcCfg[ulCfgNum].ulSrcBufLen / sizeof(SOCP_BD_DATA_STRU);

    /* 和全局变量中比较记录最大值 */
    if ((ulBDTotal- ulBDNum) > g_stSCMInfoData.aulBDUsedMax[ulCfgNum])
    {
        g_stSCMInfoData.aulBDUsedMax[ulCfgNum] = (ulBDTotal- ulBDNum);
    }

    return VOS_OK;
}



VOS_VOID SCM_CoderSrcMemcpy(SOCP_CODER_SRC_ENUM_U32 enChanlID, SCM_CODER_SRC_MEMCPY_STRU *pInfo, SOCP_BUFFER_RW_STRU *pstSocpBuf)
{
    VOS_VOID    *pDst;
    VOS_UINT32  ulCfgNum;

    if (VOS_OK != SCM_FindChannelCfg(enChanlID, SCM_CODER_SRC_NUM,
                                     (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderSrcCfg,
                                     sizeof(SCM_CODER_SRC_CFG_STRU), &ulCfgNum))/* 判断通道参数 */
    {
        return ;
    }

    /* 本函数的拷贝处理流程不检查越界情况，由调用的地方保证 */
    if(pInfo->uloffset < pstSocpBuf->u32Size)
    {
        if((pInfo->uloffset + pInfo->ulLen) <= pstSocpBuf->u32Size)
        {
            (VOS_VOID)VOS_MemCpy_s(((VOS_UINT8*)pInfo->pHeader + pInfo->uloffset), pInfo->ulLen, pInfo->pSrc, pInfo->ulLen);          
            VOS_FlushCpuWriteBuf();
        }
        else
        {
            (VOS_VOID)VOS_MemCpy_s(((VOS_UINT8*)pInfo->pHeader + pInfo->uloffset), (pstSocpBuf->u32Size - pInfo->uloffset), pInfo->pSrc, (pstSocpBuf->u32Size - pInfo->uloffset));
            VOS_FlushCpuWriteBuf();
            
            pDst = g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf;

            (VOS_VOID)VOS_MemCpy_s(pDst,
                (pInfo->uloffset + pInfo->ulLen - pstSocpBuf->u32Size),
                ((VOS_UINT8*)pInfo->pSrc + (pstSocpBuf->u32Size - pInfo->uloffset)),
                (pInfo->uloffset + pInfo->ulLen - pstSocpBuf->u32Size));           
            VOS_FlushCpuWriteBuf();
            
        }
    }
    else
    {
        pDst = g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf;

        pDst = (VOS_UINT8*)pDst + (pInfo->uloffset - pstSocpBuf->u32Size);

        (VOS_VOID)VOS_MemCpy_s(pDst, pInfo->ulLen, pInfo->pSrc, pInfo->ulLen);
        VOS_FlushCpuWriteBuf();
        
    }
}



VOS_UINT32 SCM_GetCoderChnSrcBuff(SOCP_CODER_SRC_ENUM_U32 enChanlID,
                                            VOS_UINT32 ulDataLen,
                                            SCM_CODER_SRC_PACKET_HEADER_STRU** pstCoderHeader,
                                            SOCP_BUFFER_RW_STRU *pstSocpBuf)
{
    SOCP_BUFFER_RW_STRU                 stRwBuf;
    VOS_UINT32                          ulCfgNum;
    SCM_CODER_SRC_PACKET_HEADER_STRU    *pstBuff;
    VOS_UINT32                          *pstBuftmp;
    VOS_UINT32                          ulTrueLen;

    /* 判断数据不能大于4K */
    if ((0 == ulDataLen) || (ulDataLen > SCM_CODER_SRC_MAX_LEN))
    {
        (VOS_VOID)vos_printf("%s : ulDataLen %d.\n", __FUNCTION__, ulDataLen);
        return VOS_ERR;
    }

    if (VOS_OK != SCM_FindChannelCfg(enChanlID, SCM_CODER_SRC_NUM,
                                     (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderSrcCfg,
                                     sizeof(SCM_CODER_SRC_CFG_STRU), &ulCfgNum))/* 判断通道参数 */
    {
        return VOS_ERR;/* 返回失败 */
    }

    if(SOCP_ENCSRC_CHNMODE_LIST == g_astSCMCoderSrcCfg[ulCfgNum].enCHMode)
    {
        return VOS_ERR;/* 返回失败 */
    }

    if(VOS_OK != mdrv_socp_get_write_buff(enChanlID, &stRwBuf))
    {
        g_astScmCoderSrcDebugInfo[ulCfgNum].ulGetWriteBufErr ++;
        return VOS_ERR;/* 返回失败 */
    }

    ulTrueLen = ALIGN_DDR_WITH_8BYTE(ulDataLen);
    if((stRwBuf.u32Size + stRwBuf.u32RbSize) >= (ulTrueLen + SCM_HISI_HEADER_LENGTH))
    {
        /*需要返回虚拟地址给上层*/
        pstBuff = (SCM_CODER_SRC_PACKET_HEADER_STRU*)VOS_UncacheMemPhyToVirt((VOS_UINT8*)stRwBuf.pBuffer,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucSrcPHY,
                                    g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf,
                                    g_astSCMCoderSrcCfg[ulCfgNum].ulSrcBufLen);

        if(stRwBuf.u32Size >= SCM_HISI_HEADER_LENGTH)
        {
            pstBuff->ulHisiMagic = SCM_HISI_HEADER_MAGIC;
            pstBuff->ulDataLen   = ulDataLen;
        }
        else if(stRwBuf.u32Size >= 4)
        {
            pstBuff->ulHisiMagic = SCM_HISI_HEADER_MAGIC;

            pstBuftmp = (VOS_UINT32*)g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf;
            *pstBuftmp      = ulDataLen;
        }
        else    /* TODO: 应该没有stRwBuf.u32Size为0的场景 */
        {
            pstBuftmp = (VOS_UINT32*)g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf;

            *(pstBuftmp++)  = SCM_HISI_HEADER_MAGIC;
            *pstBuftmp      = ulDataLen;
        }

        *pstCoderHeader     = pstBuff;

        (VOS_VOID)VOS_MemCpy_s(pstSocpBuf, (VOS_UINT32)sizeof(stRwBuf), &stRwBuf, sizeof(stRwBuf));

        return VOS_OK;
    }
    else
    {
        g_astScmCoderSrcDebugInfo[ulCfgNum].ulGetCoherentBuffErr++;
        return VOS_ERR;
    }

}


VOS_UINT32 SCM_SendCoderSrc(SOCP_CODER_SRC_ENUM_U32 enChanlID, VOS_UINT8 *pucSendDataAddr, VOS_UINT32 ulSendLen)
{
    SOCP_BUFFER_RW_STRU                 stRwBuf;
    VOS_UINT32                          ulBDNum;
    VOS_UINT32                          ulCfgNum;
    SOCP_BD_DATA_STRU                   stBDData;
    SCM_CODER_SRC_PACKET_HEADER_STRU*   pstCoderHeader;

    /* 判断数据指针和长度的正确，长度不能大于4K */
    if ((VOS_NULL_PTR == pucSendDataAddr)
        ||(0 == ulSendLen)
        /*||(SCM_CODER_SRC_MAX_LEN < ulSendLen)*/)
    {
        return VOS_ERR;
    }

    if (VOS_OK != SCM_FindChannelCfg(enChanlID, SCM_CODER_SRC_NUM,
                                     (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderSrcCfg,
                                     sizeof(SCM_CODER_SRC_CFG_STRU), &ulCfgNum))/* 判断通道参数 */
    {
        return VOS_ERR;/* 返回失败 */
    }

    if (VOS_OK != mdrv_socp_get_write_buff(g_astSCMCoderSrcCfg[ulCfgNum].enChannelID, &stRwBuf))
    {
        g_astScmCoderSrcDebugInfo[ulCfgNum].ulGetWriteBufErr ++;
        return VOS_ERR;/* 返回失败 */
    }


    if(SOCP_ENCSRC_CHNMODE_LIST == g_astSCMCoderSrcCfg[ulCfgNum].enCHMode)
    {
        /* 计算空闲BD的值 */
        ulBDNum = (stRwBuf.u32Size + stRwBuf.u32RbSize) / sizeof(SOCP_BD_DATA_STRU);

        /* 判断是否还有空间 */
        if (1 >= ulBDNum)
        {
            return VOS_ERR;
        }

        stRwBuf.pBuffer = (VOS_CHAR*)VOS_UncacheMemPhyToVirt((VOS_UINT8*)stRwBuf.pBuffer,
                                        g_astSCMCoderSrcCfg[ulCfgNum].pucSrcPHY,
                                        g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf,
                                        g_astSCMCoderSrcCfg[ulCfgNum].ulSrcBufLen);

        stBDData.ulDataAddr = (VOS_UINT32)((VOS_UINT_PTR)pucSendDataAddr);
        stBDData.usMsgLen   = (VOS_UINT16)ulSendLen;
        stBDData.enDataType = SOCP_BD_DATA;

        (VOS_VOID)VOS_MemCpy_s(stRwBuf.pBuffer, (VOS_UINT32)sizeof(stBDData), &stBDData, sizeof(stBDData));    /* 复制数据到指定的地址 */
        VOS_FlushCpuWriteBuf();

        if (VOS_OK != mdrv_socp_write_done(enChanlID, sizeof(stBDData)))   /* 当前数据写入完毕 */
        {
            SCM_CODER_SRC_ERR("SCM_SendCoderSrc: Write Buffer is Error", enChanlID, 0);/* 记录Log */
            return VOS_ERR;/* 返回失败 */
        }
    }
    else if(SOCP_ENCSRC_CHNMODE_CTSPACKET == g_astSCMCoderSrcCfg[ulCfgNum].enCHMode)
    {
        if(stRwBuf.u32Size < SCM_HISI_HEADER_LENGTH)
        {
            g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendFirstNotEnough ++;
            return VOS_ERR;
        }

        stRwBuf.pBuffer = (VOS_CHAR*)VOS_UncacheMemPhyToVirt((VOS_UINT8*)stRwBuf.pBuffer,
                                        g_astSCMCoderSrcCfg[ulCfgNum].pucSrcPHY,
                                        g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf,
                                        g_astSCMCoderSrcCfg[ulCfgNum].ulSrcBufLen);
        if(stRwBuf.pBuffer != (VOS_CHAR*)pucSendDataAddr)
        {
            g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendAddrErr++;
            return VOS_ERR;
        }

        pstCoderHeader = (SCM_CODER_SRC_PACKET_HEADER_STRU*)pucSendDataAddr;
        if((pstCoderHeader->ulDataLen != ulSendLen)||(pstCoderHeader->ulHisiMagic != SCM_HISI_HEADER_MAGIC))
        {
            g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendHeaderErr++;
            return VOS_ERR;
        }
        VOS_FlushCpuWriteBuf();
        /*第一段连续空间不足HISI包头长度*/
        ulSendLen = ALIGN_DDR_WITH_8BYTE(ulSendLen);
        if(VOS_OK != mdrv_socp_write_done(enChanlID, (ulSendLen + SCM_HISI_HEADER_LENGTH)))
        {
            g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendWriteDoneErr ++;
            return VOS_ERR;
        }
        g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendDataLen += ulSendLen;
        g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendPacketNum ++;
    }
    else
    {
        return VOS_ERR;
    }
    return VOS_OK;
}


VOS_UINT32 SCM_CoderDstChanMemAlloc(VOS_VOID)
{
    VOS_UINT32                          i = 0;
    VOS_UINT_PTR                        ulPHYAddr;

    for (i = 0; i < SCM_CODER_DST_NUM; i++)
    {
        /* 申请编码目的空间 */
        g_astSCMCoderDstCfg[i].pucBuf = (VOS_UINT8*)VOS_UnCacheMemAlloc(g_astSCMCoderDstCfg[i].ulBufLen, &ulPHYAddr);

        /* 申请空间错误 */
        if (VOS_NULL_PTR == g_astSCMCoderDstCfg[i].pucBuf)
        {
            /* 记录通道初始化标记为内存申请异常 */
            g_astSCMCoderDstCfg[i].enInitState = SCM_CHANNEL_MEM_FAIL;

            return VOS_ERR;/* 返回错误 */
        }

        g_astSCMCoderDstCfg[i].pucBufPHY = (VOS_UINT8*)ulPHYAddr;
    }

    return VOS_OK;
}


VOS_UINT32 SCM_CoderDstChanMemInit(VOS_VOID)
{
    VOS_UINT_PTR                        ulPHYAddr;
    VOS_UINT32                          ulBufLen;
    SOCP_ENC_DST_BUF_LOG_CFG_STRU       stLogCfg;

    VOS_MemSet_s(&stLogCfg,(VOS_UINT32)sizeof(stLogCfg),0,sizeof(stLogCfg));
    if(VOS_OK != mdrv_socp_get_sd_logcfg(&stLogCfg))
    {
        vos_printf("!!!! No code dest channel config from SOCP.\n");

        return SCM_CoderDstChanMemAlloc();
    }

    /* 未打开log buffer下的处理 */
    if ((SOCP_DST_CHAN_NOT_CFG == stLogCfg.logOnFlag)
        || (VOS_NULL == stLogCfg.ulPhyBufferAddr))
    {
        vos_printf("!!!! No code dest channel config from SOCP, flag %d, addr 0x%x.\n",
            stLogCfg.logOnFlag, stLogCfg.ulPhyBufferAddr);

        return SCM_CoderDstChanMemAlloc();
    }

    /* IND通道需要做延迟写入，BUFFER大小50M(默认值)，水线设置为水线设置为75%，内存在初始化已经申请过 */
    g_astSCMCoderDstCfg[SCM_CODER_DST_IND_CHANNEL].pucBufPHY    = (VOS_UINT8*)(stLogCfg.ulPhyBufferAddr);
    g_astSCMCoderDstCfg[SCM_CODER_DST_IND_CHANNEL].ulBufLen     = stLogCfg.BufferSize;
    g_astSCMCoderDstCfg[SCM_CODER_DST_IND_CHANNEL].pucBuf       = stLogCfg.pVirBuffer;
    /* 因为旧版本的SOCP的单位是KB新的版本是B,为了兼容SOCP代码中乘了1024,所以这里需要除以1024 */
    g_astSCMCoderDstCfg[SCM_CODER_DST_IND_CHANNEL].ulThreshold  = 0x4;

    /* CNF通道不需要做延迟写入 */
    ulBufLen = g_astSCMCoderDstCfg[SCM_CODER_DST_CNF_CHANNEL].ulBufLen;

    g_astSCMCoderDstCfg[SCM_CODER_DST_CNF_CHANNEL].pucBuf = (VOS_UINT8*)VOS_UnCacheMemAlloc(ulBufLen, &ulPHYAddr);

    /* 申请空间错误 */
    if (VOS_NULL_PTR == g_astSCMCoderDstCfg[SCM_CODER_DST_CNF_CHANNEL].pucBuf)
    {
        /* 记录通道初始化标记为内存申请异常 */
        g_astSCMCoderDstCfg[SCM_CODER_DST_CNF_CHANNEL].enInitState = SCM_CHANNEL_MEM_FAIL;

        return VOS_ERR;
    }

    g_astSCMCoderDstCfg[SCM_CODER_DST_CNF_CHANNEL].pucBufPHY = (VOS_UINT8*)ulPHYAddr;

    return VOS_OK;
}


VOS_UINT32 SCM_RlsDestBuf(VOS_UINT32 ulChanlID, VOS_UINT32 ulReadSize)
{
    VOS_UINT32                          ulDataLen;
    SOCP_BUFFER_RW_STRU                 stBuffer;

    if(0 == ulReadSize) /*释放通道所有数据*/
    {
        if (VOS_OK != BSP_SOCP_GetReadBuff(ulChanlID, &stBuffer))
        {
            SCM_CODER_DST_ERR("SCM_RlsDestBuf: Get Read Buffer is Error", ulChanlID, 0);/* 记录Log */
            return VOS_ERR;
        }

        ulDataLen = stBuffer.u32Size + stBuffer.u32RbSize;

        if (SOCP_CODER_DST_OM_IND == ulChanlID)
        {
            diag_ReportLost(EN_DIAG_LOST_CPMCB);
        }

        SCM_CODER_DST_LOG("SCM_RlsDestBuf: Relese All Data", ulChanlID, ulDataLen);
    }
    else
    {
        /* 记录调用时间 */
        SCM_CODER_DST_LOG("SCM_RlsDestBuf: Relese Read Data", ulChanlID, ulReadSize);

        ulDataLen = ulReadSize;
    }

    if (VOS_OK != BSP_SOCP_ReadDataDone(ulChanlID, ulDataLen))
    {
        SCM_CODER_DST_ERR("SCM_RlsDestBuf: Read Data Done is Error", ulChanlID, ulDataLen);/* 记录Log */

        return VOS_ERR;
    }
    OM_ACPU_DEBUG_CHANNEL_TRACE(ulChanlID, NULL, ulDataLen, OM_ACPU_READ_DONE, OM_ACPU_DATA);
    return VOS_OK;
}


VOS_VOID SCM_CoderDestReadCB(VOS_UINT32 ulDstChID)
{
    VOS_UINT32                          ulChType;
    VOS_UINT32                          ulCfgNum;
    SOCP_BUFFER_RW_STRU                 stBuffer;
    VOS_UINT32                          ulTimerIn;
    VOS_UINT32                          ulTimerOut;
    VOS_UINT_PTR                        ulVirtAddr;

    if(SOCP_CODER_DST_OM_CNF == ulDstChID)
    {
        diag_PTR(EN_DIAG_PTR_SCM_CODER_DST_CB);
    }

    ulChType = SOCP_REAL_CHAN_TYPE(ulDstChID);

    if (SOCP_CODER_DEST_CHAN != ulChType)
    {
        SCM_CODER_DST_ERR("SCM_CoderDestReadCB: Channel Type is Error", ulDstChID, ulChType);/* 记录Log */
        return;
    }

    if (VOS_OK != BSP_SOCP_GetReadBuff(ulDstChID, &stBuffer))
    {
        SCM_CODER_DST_ERR("SCM_CoderDestReadCB: Get Read Buffer is Error", ulDstChID, 0);/* 记录Log */
        return;
    }

    if (VOS_OK != SCM_FindChannelCfg(ulDstChID, SCM_CODER_DST_NUM,
                                (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderDstCfg, sizeof(SCM_CODER_DEST_CFG_STRU), &ulCfgNum))
    {
        BSP_SOCP_ReadDataDone(ulDstChID, stBuffer.u32Size + stBuffer.u32RbSize);  /* 清空数据 */

        if(SOCP_CODER_DST_OM_IND == ulDstChID)
        {
            diag_ReportLost(EN_DIAG_LOST_BRANCH);
        }

        SCM_CODER_DST_ERR("SCM_CoderDestReadCB: Find Channel is Error", ulDstChID, 0);/* 记录Log */

        return;
    }

     /* 开机log功能，IND通道上报函数为空，使log缓存在本地 */
    if(VOS_NULL_PTR == g_astSCMCoderDstCfg[ulCfgNum].pfunc)
    {
        return;
    }

    if((0 == (stBuffer.u32Size + stBuffer.u32RbSize))||(VOS_NULL_PTR == stBuffer.pBuffer))
    {
        BSP_SOCP_ReadDataDone(ulDstChID, stBuffer.u32Size + stBuffer.u32RbSize);  /* 清空数据 */

        if(SOCP_CODER_DST_OM_IND == ulDstChID)
        {
            diag_ReportLost(EN_DIAG_LOST_BRANCH);
        }

        SCM_CODER_DST_ERR("SCM_CoderDestReadCB: Get RD error ", ulDstChID,0);/* 记录Log */
        return;
    }

    if(0 == stBuffer.u32Size)
    {
        return;
    }

    /* 发送数据 */
    ulVirtAddr = VOS_UncacheMemPhyToVirt((VOS_UINT8 *)stBuffer.pBuffer,
                                g_astSCMCoderDstCfg[ulCfgNum].pucBufPHY,
                                g_astSCMCoderDstCfg[ulCfgNum].pucBuf,
                                g_astSCMCoderDstCfg[ulCfgNum].ulBufLen);
    if(VOS_NULL_PTR == ulVirtAddr)
    {
        BSP_SOCP_ReadDataDone(ulDstChID, stBuffer.u32Size + stBuffer.u32RbSize);  /* 清空数据 */

        if(SOCP_CODER_DST_OM_IND == ulDstChID)
        {
            diag_ReportLost(EN_DIAG_LOST_BRANCH);
        }

        SCM_CODER_DST_ERR("SCM_CoderDestReadCB:  stBuffer.pBuffer == VOS_NULL", ulDstChID, 0);/* 记录Log */
        return;
    }
    ulTimerIn = mdrv_timer_get_normal_timestamp();

    g_astSCMCoderDstCfg[ulCfgNum].pfunc(ulDstChID, (VOS_UINT8*)ulVirtAddr, (VOS_UINT8*)stBuffer.pBuffer,(VOS_UINT32)stBuffer.u32Size);
    ulTimerOut = mdrv_timer_get_normal_timestamp();
    /* 记录回调函数的执行时间 */
    SCM_CODER_DST_LOG("SCM_CoderDestReadCB: Call channel Func Proc time", ulDstChID, (ulTimerIn-ulTimerOut));

    return;
}

/* ****************************************************************************
 函 数 名  : SCM_CoderDestEventCB
 功能描述  : 处理编码目的通道的Event事件回调
 输入参数  : ulDstChID 目的通道ID
 输出参数  : 无
 返 回 值  : 无
 调用函数  :
 被调函数  :
 修改历史  :

**************************************************************************** */
VOS_VOID SCM_CoderDestEventCB(VOS_UINT32 ulDstChID,SOCP_EVENT_ENUM_UIN32 u32Event, VOS_UINT32 u32Param)
{
    
    if(SOCP_EVENT_OUTBUFFER_OVERFLOW == u32Event)
    {
        diag_ReportLost(EN_DIAG_LOST_OVERFLOW);
    }
    else
    {
        /* do nothing */
    }
}


VOS_UINT32 SCM_CoderDstChannelInit(VOS_VOID)
{
    VOS_UINT32                          i;
    SOCP_CODER_DEST_CHAN_S              stChannel;

    for (i = 0; i < SCM_CODER_DST_NUM; i++)
    {
        if (0 == i)
        {
            stChannel.u32EncDstThrh = SCM_CODER_DST_GTHRESHOLD;
        }
        else
        {
            /* 扩大编码目的通道1阈值门限 */
            stChannel.u32EncDstThrh = 2 * SCM_CODER_DST_GTHRESHOLD;
        }

        stChannel.sCoderSetDstBuf.pucOutputStart    = g_astSCMCoderDstCfg[i].pucBufPHY;

        stChannel.sCoderSetDstBuf.pucOutputEnd
            = (g_astSCMCoderDstCfg[i].pucBufPHY + g_astSCMCoderDstCfg[i].ulBufLen)-1;

        stChannel.sCoderSetDstBuf.u32Threshold      = g_astSCMCoderDstCfg[i].ulThreshold;

        if (VOS_OK != mdrv_socp_coder_set_dest_chan_attr(g_astSCMCoderDstCfg[i].enChannelID, &stChannel))
        {
            g_astSCMCoderDstCfg[i].enInitState = SCM_CHANNEL_CFG_FAIL;  /* 记录通道初始化配置错误 */

            return VOS_ERR;/* 返回失败 */
        }

        g_astSCMCoderDstCfg[i].enInitState = SCM_CHANNEL_INIT_SUCC;     /* 记录通道初始化配置错误 */

        BSP_SOCP_RegisterReadCB(g_astSCMCoderDstCfg[i].enChannelID, (socp_read_cb)SCM_CoderDestReadCB);

        DRV_SOCP_REGISTER_EVENT_CB(g_astSCMCoderDstCfg[i].enChannelID, (socp_event_cb)SCM_CoderDestEventCB);
    }

    return VOS_OK;
}




VOS_UINT32 SCM_RegCoderDestProc(SOCP_CODER_DST_ENUM_U32 enChanlID, SCM_CODERDESTFUCN func)
{
    VOS_UINT32                          ulCgfNum;

    if (VOS_OK != SCM_FindChannelCfg(enChanlID,
                                SCM_CODER_DST_NUM,
                                (SCM_CHANNEL_CFG_HEAD *)g_astSCMCoderDstCfg,
                                sizeof(SCM_CODER_DEST_CFG_STRU),
                                &ulCgfNum))
    {
        return VOS_ERR;/* 返回失败 */
    }

    g_astSCMCoderDstCfg[ulCgfNum].pfunc = func;

    return VOS_OK;/* 返回成功 */
}



VOS_UINT32 SCM_RegDecoderDestProc(SOCP_DECODER_DST_ENUM_U32 enChanlID, SCM_DECODERDESTFUCN func)
{
    VOS_UINT32                          ulOffset;

    if (enChanlID >= SOCP_DECODER_DST_BUTT)
    {
        return VOS_ERR;
    }

    if (VOS_NULL_PTR == func)
    {
        return VOS_ERR;
    }

    if (SOCP_DECODER_DST_LOM == enChanlID)
    {
        ulOffset = SOCP_DECODER_DST_CB_TL_OM;
    }
    else if (SOCP_DECODER_DST_GUOM == enChanlID)
    {
        ulOffset = SOCP_DECODER_DST_CB_GU_OM;
    }
    else if (SOCP_DECODER_CBT == enChanlID)
    {
        ulOffset = SOCP_DECODER_DST_CB_GU_CBT;
    }
    else
    {
        return VOS_ERR;
    }

    g_astSCMDecoderCbFunc[ulOffset] = func;

    return VOS_OK;
}

VOS_UINT32 SCM_CoderDstCompressCfg(VOS_BOOL enable)
{
    if (enable)
    {
        mdrv_socp_compress_enable(SOCP_CODER_DST_OM_IND);
    }
    else
    {
        mdrv_socp_compress_disable(SOCP_CODER_DST_OM_IND);
    }
    return VOS_OK;
}



VOS_UINT32 SCM_CreateSrcBuffer(VOS_UINT8 **pBufVir, VOS_UINT8 **pBufPhy, VOS_UINT32 ulLen)
{
    VOS_UINT_PTR ulRealAddr;

    /*申请uncache的动态内存区*/
    *pBufVir = (VOS_UINT8*)VOS_UnCacheMemAlloc(ulLen, &ulRealAddr);

    /* 分配内存失败 */
    if (VOS_NULL_PTR == *pBufVir)
    {
        return VOS_ERR;
    }

    /* 保存buf实地址 */
    *pBufPhy = (VOS_UINT8*)ulRealAddr;

    return VOS_OK;
}



VOS_UINT32 SCM_CoderSrcChanMemInit(VOS_VOID)
{
    VOS_UINT32                           i;
    VOS_UINT32                          ulRet;

    for (i=0; i<SCM_CODER_SRC_NUM; i++)
    {
        if(g_astSCMCoderSrcCfg[i].enChannelID == SCM_CODER_SRC_LOM_CNF)
        {
            ulRet = SCM_CreateSrcBuffer(&g_astSCMCoderSrcCfg[i].pucSrcBuf,
                                        &g_astSCMCoderSrcCfg[i].pucSrcPHY,
                                        SCM_CODER_SRC_CNF_BUFFER_SIZE);
            if(VOS_OK != ulRet)
            {
                return VOS_ERR;
            }
            g_astSCMCoderSrcCfg[i].ulSrcBufLen  = SCM_CODER_SRC_CNF_BUFFER_SIZE;
        }
        else if(g_astSCMCoderSrcCfg[i].enChannelID == SCM_CODER_SRC_LOM_IND)
        {
            ulRet = SCM_CreateSrcBuffer(&g_astSCMCoderSrcCfg[i].pucSrcBuf,
                                        &g_astSCMCoderSrcCfg[i].pucSrcPHY,
                                        SCM_CODER_SRC_IND_BUFFER_SIZE);
            if(VOS_OK != ulRet)
            {
                return VOS_ERR;
            }
            g_astSCMCoderSrcCfg[i].ulSrcBufLen  = SCM_CODER_SRC_IND_BUFFER_SIZE;

        }
        else
        {
            /* TODO: CJQ */
            /* SOCP_CODER_SRC_LOM_IND3 通道暂未使用，暂不申请内存 */
        }
    }

    return VOS_OK;

}


VOS_UINT32 SCM_ChannelMemInit(VOS_VOID)
{
    VOS_UINT32  ulRet;
    NV_POWER_ON_LOG_SWITCH_STRU     stPowerOnLog;
    SOCP_ENC_DST_BUF_LOG_CFG_STRU   stLogCfg;

    if(VOS_OK != SCM_CoderSrcChanMemInit())
    {
        return VOS_ERR;
    }
    
    VOS_MemSet_s(&stLogCfg,(VOS_UINT32)sizeof(stLogCfg),0,sizeof(stLogCfg));
    if(VOS_OK == mdrv_socp_get_sd_logcfg(&stLogCfg))
    {
        ulRet = NV_Read(EN_NV_ID_POWER_ON_LOG_SWITCH, (u8*)&stPowerOnLog, sizeof(stPowerOnLog));
        
        if(VOS_OK != ulRet)
        {
            (VOS_VOID)vos_printf("Read nv 0x%x fail.\n", EN_NV_ID_POWER_ON_LOG_SWITCH);
        }
        else
        {
            stPowerOnLog.cPowerOnlogA = (stLogCfg.BufferSize < 50*1024*1024) ? 0 : 1;
            if(VOS_OK != NV_Write(EN_NV_ID_POWER_ON_LOG_SWITCH, (u8*)&stPowerOnLog, sizeof(stPowerOnLog)))
            {
                (VOS_VOID)vos_printf("Write nv 0x%x fail.\n", EN_NV_ID_POWER_ON_LOG_SWITCH);
            }
            else
            {
                (VOS_VOID)vos_printf("Write power on log nv 0x%x success.\n", EN_NV_ID_POWER_ON_LOG_SWITCH);
            }
        }
    }

    return VOS_OK;
}



VOS_UINT32 SCM_ChannelInit(VOS_VOID)
{
    VOS_MemSet_s(&g_stSCMInfoData, (VOS_UINT32)sizeof(g_stSCMInfoData), 0, sizeof(g_stSCMInfoData));

    if (VOS_OK != SCM_CoderSrcChannelInit()) /* 编码源通道初始化 */
    {
        return VOS_ERR;/* 返回错误 */
    }

    return VOS_OK;
}


VOS_VOID SCM_ChannelInfoShow(VOS_VOID)
{
    VOS_UINT32      i;

    for(i=0; i<SCM_CODER_SRC_NUM; i++)
    {
        vos_printf("\r\n The Channle 0x%x info is :", g_astSCMCoderSrcCfg[i].enChannelID);

        vos_printf("\r\n The Max BD num is %d", g_stSCMInfoData.aulBDUsedMax[i]);

        vos_printf("\r\n The Max rD num is %d \r\n", g_stSCMInfoData.aulRDUsedMax[i]);
    }

    return;
}



VOS_VOID SCM_CoderSrcCHShow(VOS_UINT32 ulCfgNum)
{
    vos_printf("\r\n CH id         is 0x%x", g_astSCMCoderSrcCfg[ulCfgNum].enChannelID);
    vos_printf("\r\n CH init state is   %d", g_astSCMCoderSrcCfg[ulCfgNum].enInitState);
    vos_printf("\r\n CH type       is   %d", g_astSCMCoderSrcCfg[ulCfgNum].enCHMode);
    vos_printf("\r\n CH Dst        is 0x%x", g_astSCMCoderSrcCfg[ulCfgNum].enDstCHID);
    vos_printf("\r\n CH data type  is   %d", g_astSCMCoderSrcCfg[ulCfgNum].enDataType);
    vos_printf("\r\n CH Level      is   %d", g_astSCMCoderSrcCfg[ulCfgNum].enCHLevel);
    vos_printf("\r\n CH BD VirtAddris 0x%p", g_astSCMCoderSrcCfg[ulCfgNum].pucSrcBuf);
    vos_printf("\r\n CH BD PHYAddr is 0x%p", g_astSCMCoderSrcCfg[ulCfgNum].pucSrcPHY);
    vos_printf("\r\n CH BD BufLen  is 0x%x", g_astSCMCoderSrcCfg[ulCfgNum].ulSrcBufLen);
    vos_printf("\r\n CH RD VirtAddris 0x%p", g_astSCMCoderSrcCfg[ulCfgNum].pucRDBuf);
    vos_printf("\r\n CH RD PHYAddr is 0x%p", g_astSCMCoderSrcCfg[ulCfgNum].pucRDPHY);
    vos_printf("\r\n CH RD BufLen  is 0x%x", g_astSCMCoderSrcCfg[ulCfgNum].ulRDBufLen);

    vos_printf("\r\n CH SEND data   LENGTH          0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendDataLen);
    vos_printf("\r\n CH SEND packet NUM             0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendPacketNum);
    vos_printf("\r\n CH SEND addr   ERR             0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendAddrErr);
    vos_printf("\r\n CH SEND head   ERR             0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendHeaderErr);
    vos_printf("\r\n CH SEND write  ERR             0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendWriteDoneErr);
    vos_printf("\r\n CH SEND first buff not enough  0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulSendFirstNotEnough);

    vos_printf("\r\n CH SEND get buffer ERR         0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulGetCoderBuffErr);
    vos_printf("\r\n CH SEND get coherent buff ERR  0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulGetCoherentBuffErr);

    vos_printf("\r\n CH SEND fill max buff  len     0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulFillFirstBuffMax);
    vos_printf("\r\n CH SEND fill num               0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulFillFirstBuffNum);
    vos_printf("\r\n CH SEND after fill not enough  0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulAfterFillNotEnough);
    vos_printf("\r\n CH SEND fill not enough        0x%x", g_astScmCoderSrcDebugInfo[ulCfgNum].ulFillNotEnough);


    return;
}



VOS_VOID SCM_CoderDstCHShow(VOS_UINT32 ulCfgNum)
{
    vos_printf("\r\n CH id         is 0x%x", g_astSCMCoderDstCfg[ulCfgNum].enChannelID);
    vos_printf("\r\n CH init state is   %d", g_astSCMCoderDstCfg[ulCfgNum].enInitState);
    vos_printf("\r\n CH BD VirtAddris 0x%x", g_astSCMCoderDstCfg[ulCfgNum].pucBuf);
    vos_printf("\r\n CH BD PHYAddr is 0x%x", g_astSCMCoderDstCfg[ulCfgNum].pucBufPHY);
    vos_printf("\r\n CH BD BufLen  is 0x%x", g_astSCMCoderDstCfg[ulCfgNum].ulBufLen);
    vos_printf("\r\n CH threshold  is 0x%x", g_astSCMCoderDstCfg[ulCfgNum].ulThreshold);
    vos_printf("\r\n CH CB func    is 0x%x", g_astSCMCoderDstCfg[ulCfgNum].pfunc);

    return;
}


SCM_INFODATA_STRU* SCM_GetDebugLogInfo(VOS_VOID)
{
    return &g_stSCMInfoData;
}


VOS_UINT32 SCM_GetDebugLogInfoLen(VOS_VOID)
{
    return (VOS_UINT32)sizeof(g_stSCMInfoData);
}



VOS_VOID SCM_SocpSendDataToUDISucc(
    SOCP_CODER_DST_ENUM_U32             enChanID,
    CPM_PHY_PORT_ENUM_UINT32            enPhyport,
    OM_SOCP_CHANNEL_DEBUG_INFO         *pstDebugInfo,
    VOS_UINT32                         *pulSendDataLen
)
{
    if ((SOCP_CODER_DST_OM_CNF == enChanID) && (CPM_CFG_PORT == enPhyport))
    {
        if ((0 == g_stUsbCfgPseudoSync.ulLen) || (*pulSendDataLen != g_stUsbCfgPseudoSync.ulLen))
        {
            pstDebugInfo->ulUSBSendCBAbnormalNum++;
            pstDebugInfo->ulUSBSendCBAbnormalLen += *pulSendDataLen;
        }

        *pulSendDataLen = g_stUsbCfgPseudoSync.ulLen;
    }
    else if ((SOCP_CODER_DST_OM_IND == enChanID) && (CPM_IND_PORT == enPhyport))
    {
        if ((0 == g_stUsbIndPseudoSync.ulLen) || (*pulSendDataLen != g_stUsbIndPseudoSync.ulLen))
        {
            pstDebugInfo->ulUSBSendCBAbnormalNum++;
            pstDebugInfo->ulUSBSendCBAbnormalLen += *pulSendDataLen;
        }

        *pulSendDataLen = g_stUsbIndPseudoSync.ulLen;
    }
    else
    {
        ;
    }

    return;
}


VOS_VOID SCM_SocpSendDataToUDI(SOCP_CODER_DST_ENUM_U32 enChanID, VOS_UINT8 *pucVirData, VOS_UINT8 *pucPHYData, VOS_UINT32 ulDataLen)
{
    VOS_UINT32                  ulResult;
    VOS_UINT32                  ulRet = VOS_ERR;
    CPM_PHY_PORT_ENUM_UINT32    enPhyport;
    VOS_UINT32                  ulSendDataLen;
    VOS_BOOL                    bUsbSendSucFlag = VOS_FALSE;
    VOS_BOOL                    bUsbSendFlag = VOS_FALSE;
    OM_SOCP_CHANNEL_DEBUG_INFO  *pstDebugInfo = VOS_NULL_PTR;
    CPM_LOGIC_PORT_ENUM_UINT32  enLogicPort;

    if(SOCP_CODER_DST_OM_CNF == enChanID)
    {
        diag_PTR(EN_DIAG_PTR_SCM_SENDTOUDI);
    }

    if (SOCP_CODER_DST_OM_CNF == enChanID)
    {
        pstDebugInfo = &g_stAcpuDebugInfo.stCnfDebugInfo;
        enLogicPort  = CPM_OM_CFG_COMM;
    }
    else if (SOCP_CODER_DST_OM_IND == enChanID)
    {
        pstDebugInfo = &g_stAcpuDebugInfo.stIndDebugInfo;
        enLogicPort  = CPM_OM_IND_COMM;
    }
    else
    {
        g_stAcpuDebugInfo.ulInvaldChannel++;

        return;
    }

    /*参数检查*/
    SOCP_SEND_DATA_PARA_CHECK(pstDebugInfo, ulDataLen, pucVirData);

    PPM_GetSendDataLen(enChanID, ulDataLen, &ulSendDataLen, &enPhyport);

    if(SOCP_CODER_DST_OM_CNF == enChanID)
    {
        OM_ACPU_DEBUG_CHANNEL_TRACE(enChanID, pucVirData, ulSendDataLen, OM_ACPU_SEND_USB, OM_ACPU_DATA);
    }
    else if(SOCP_CODER_DST_OM_IND == enChanID)
    {
        OM_ACPU_DEBUG_CHANNEL_TRACE(enChanID, pucVirData, ulSendDataLen, OM_ACPU_SEND_USB_IND, OM_ACPU_DATA);
    }

    /* 统计数据通道的吞吐率 */
    if(CPM_OM_IND_COMM == enLogicPort)
    {
        diag_ThroughputSave(EN_DIAG_THRPUT_DATA_CHN_PHY, ulSendDataLen);
    }
    ulResult = CPM_ComSend(enLogicPort, pucVirData, pucPHYData, ulSendDataLen);

    if(CPM_SEND_ERR == ulResult)  /*当前通道已经发送失败，调用SOCP通道无数据搬运*/
    {
        pstDebugInfo->ulUSBSendErrNum++;
        pstDebugInfo->ulUSBSendErrLen += ulSendDataLen;

        if (SOCP_CODER_DST_OM_IND == enChanID)
        {
            diag_ReportLost(EN_DIAG_LOST_CPMWR);
        }
    }
    else if(CPM_SEND_FUNC_NULL == ulResult)   /*当前通道异常，扔掉所有数据*/
    {
        pstDebugInfo->ulOmDiscardNum++;
        pstDebugInfo->ulOmDiscardLen += ulDataLen;

        if (SOCP_CODER_DST_OM_IND == enChanID)
        {
            diag_ReportLost(EN_DIAG_LOST_CPMWR);
        }
    }
    else if(CPM_SEND_PARA_ERR == ulResult)   /* 发送数据获取实地址异常 */
    {
        pstDebugInfo->ulOmGetVirtErr++;
        pstDebugInfo->ulOmGetVirtSendLen += ulDataLen;

        if (SOCP_CODER_DST_OM_IND == enChanID)
        {
            diag_ReportLost(EN_DIAG_LOST_CPMWR);
        }
    }
    else if(CPM_SEND_AYNC == ulResult) //增加cpm错误码
    {
        bUsbSendSucFlag = VOS_TRUE;
        bUsbSendFlag    = VOS_TRUE;
        ulRet           = VOS_OK;
    }
    else if(CPM_SEND_OK == ulResult)
    {
        SCM_SocpSendDataToUDISucc(enChanID, enPhyport, pstDebugInfo, &ulSendDataLen);

        bUsbSendSucFlag = VOS_TRUE;
    }
    else
    {
        vos_printf("PPM_SocpSendDataToUDI: CPM_ComSend return Error %d", (VOS_INT)ulResult);
    }

    if(bUsbSendFlag != VOS_TRUE)
    {
        ulRet = SCM_RlsDestBuf(enChanID, ulSendDataLen);

        if(VOS_OK != ulRet)
        {
            pstDebugInfo->ulSocpReadDoneErrNum++;
            pstDebugInfo->ulSocpReadDoneErrLen += ulSendDataLen;

            vos_printf("PPM_SocpSendDataToUDI: SCM_RlsDestBuf return Error %d", (VOS_INT)ulRet);
        }
    }

    if ((VOS_OK == ulRet) && (VOS_TRUE == bUsbSendSucFlag))
    {
        pstDebugInfo->ulUSBSendNum++;
        pstDebugInfo->ulUSBSendRealLen += ulSendDataLen;
    }

    return;
}


VOS_VOID SCM_RegCoderDestIndChan(VOS_VOID)
{
    vos_printf("SCM_RegCoderDestIndChan.\n");

    /* 将OM SOCP目的通道处理函数注册给SCM */
    if (VOS_OK != SCM_RegCoderDestProc(SOCP_CODER_DST_OM_IND, (SCM_CODERDESTFUCN)SCM_SocpSendDataToUDI))
    {
        vos_printf("SCM_RegCoderDestIndChan Reg OM IND Fail.\n");

        return ;
    }
}

VOS_VOID SCM_unRegCoderDestIndChan(VOS_VOID)
{
    vos_printf("SCM_unRegCoderDestIndChan.\n");

    /* 将OM SOCP目的通道处理函数注册给SCM */
    if (VOS_OK != SCM_RegCoderDestProc(SOCP_CODER_DST_OM_IND, (SCM_CODERDESTFUCN)VOS_NULL))
    {
        vos_printf("SCM_unRegCoderDestIndChan Reg OM IND Fail.\n");

        return ;
    }
}


VOS_UINT32 SCM_Init(VOS_VOID)
{
   if (VOS_OK != SCM_ChannelMemInit())/* 通道内存初始化 */
   {
       return VOS_ERR;
   }

   if(VOS_OK != SCM_ChannelInit())
   {
        return VOS_ERR;
   }
   /* 将OM SOCP目的通道处理函数注册给SCM */
   if (VOS_OK != SCM_RegCoderDestProc(SOCP_CODER_DST_OM_CNF, (SCM_CODERDESTFUCN)SCM_SocpSendDataToUDI))
   {
        vos_printf("SCM_Init:SCM_RegCoderDestProc Reg OM CNF Fail.\n");

        return VOS_ERR;
    }

    CPM_LogicRcvReg(CPM_OM_CFG_COMM, SCM_SoftDecodeCfgDataRcv);

    g_stScmSoftDecodeInfo.ulCpmRegLogicRcvSuc++;

    return VOS_OK;
}


VOS_UINT32 COMM_Init(VOS_VOID)
{
    if (VOS_OK != SCM_Init())
    {
        return VOS_ERR;
    }

    /* 初始化物理通道 */
    if (VOS_OK != PPM_InitPhyPort())
    {
        return VOS_ERR;
    }

    return VOS_OK;
}


unsigned long long g_scm_dma_mask = (unsigned long long) (-1);


VOS_VOID SCM_FlushCpuCache( VOS_VOID *pAddress, VOS_INT lSize )
{
    struct device dev = {0};

    dev.dma_mask = (unsigned long long *)(&g_scm_dma_mask);
    dma_map_single(&dev, pAddress, lSize, DMA_TO_DEVICE);
}



VOS_VOID SCM_InvalidCpuCache( VOS_VOID *pAddress, VOS_INT lSize )
{
    struct device dev = {0};

    dev.dma_mask = (unsigned long long *)(&g_scm_dma_mask);
    dma_map_single(&dev, pAddress, lSize, DMA_FROM_DEVICE);
}





