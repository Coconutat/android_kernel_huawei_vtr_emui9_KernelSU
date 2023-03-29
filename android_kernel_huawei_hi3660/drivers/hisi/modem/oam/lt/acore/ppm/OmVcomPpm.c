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
#include "OmVcomPpm.h"
#include "cpm.h"
#include "dms.h"



#define    THIS_FILE_ID        PS_FILE_ID_OM_VCOM_PPM_C

/* ****************************************************************************
  2 全局变量定义
**************************************************************************** */
/* 用于记录 VCOM 通道发送的统计信息 */
OM_VCOM_DEBUG_INFO                      g_stVComDebugInfo[3];

/*****************************************************************************
  3 外部引用声明
*****************************************************************************/


/*****************************************************************************
  4 函数实现
*****************************************************************************/




VOS_UINT32 PPM_VComCfgSendData(VOS_UINT8 *pucVirAddr, VOS_UINT8 *pucPhyAddr, VOS_UINT32 ulDataLen)
{
    g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendNum++;
    g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendLen += ulDataLen;

    if (VOS_OK != DMS_WriteOmData(DMS_VCOM_OM_CHAN_CTRL, pucVirAddr, ulDataLen))
    {
        g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendErrNum++;
        g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendErrLen += ulDataLen;
        
        (void)vos_printf("vcom cnf cmd failed, ind sum leng 0x%x, ind err len 0x%x.\n", \
            g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendLen, \
            g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendErrLen);

        return CPM_SEND_ERR;
    }

    /* 与手机软件连接时，启动延时上报，且打印到缓存中，可以输出打印 */
    (void)vos_printf("vcom cnf cmd success, ind sum leng 0x%x, ind err len 0x%x.\n", \
            g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendLen, \
            g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendErrLen);

    return CPM_SEND_OK;
}


VOS_VOID PPM_VComEvtCB(VOS_UINT32 ulChan, VOS_UINT32 ulEvent)
{
    OM_LOGIC_CHANNEL_ENUM_UINT32        enChannel;
    VOS_BOOL                            ulSndMsg;

    (void)vos_printf("PPM_VComEvtCB Chan:%s Event:%s.\n", 
                     (ulChan  == DMS_VCOM_OM_CHAN_CTRL) ? "cnf"  : "ind",
                     (ulEvent == DMS_CHAN_EVT_OPEN)     ? "open" : "close");

    if(ulChan == DMS_VCOM_OM_CHAN_CTRL)
    {
        enChannel = OM_LOGIC_CHANNEL_CNF;
    }
    else if(ulChan == DMS_VCOM_OM_CHAN_DATA)
    {
        enChannel = OM_LOGIC_CHANNEL_IND;
    }
    else
    {
        (void)vos_printf("[%s] Error channel NO %d\n",__FUNCTION__,ulChan);
        return;
    }

    /*打开操作直接返回*/
    if(ulEvent == DMS_CHAN_EVT_OPEN)
    {
        (void)vos_printf("PPM_VComEvtCB open, do nothing.\n");
        return;
    }
    else if(ulEvent == DMS_CHAN_EVT_CLOSE)
    {
        ulSndMsg  = VOS_FALSE;

        if((CPM_VCOM_CFG_PORT == CPM_QueryPhyPort(CPM_OM_CFG_COMM)) &&
           (CPM_VCOM_IND_PORT == CPM_QueryPhyPort(CPM_OM_IND_COMM)))
        {
            ulSndMsg = TRUE;
        }
        if(ulSndMsg == TRUE)
        {
            (void)vos_printf("PPM_VComEvtCB close, disconnect all ports.\n");
            PPM_DisconnectAllPort(enChannel);
        }
    }
    else
    {
        (void)vos_printf("[%s] Error Event State %d\n",__FUNCTION__,ulEvent);
    }

    return;
}


VOS_UINT32 PPM_VComCfgReadData(VOS_UINT32 ulDevIndex, VOS_UINT8 *pData, VOS_UINT32 uslength)
{
    if (ulDevIndex != DMS_VCOM_OM_CHAN_CTRL)
    {
        (void)vos_printf("[%s]:PhyPort port is error: %d\n", __FUNCTION__, ulDevIndex);

        return VOS_ERR;
    }

    g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvNum++;
    g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvLen += uslength;

    if ((VOS_NULL_PTR == pData) || (0 == uslength))
    {
        (void)vos_printf("PPM_VComCfgReadData:Send data is NULL\n");

        return VOS_ERR;
    }

    /* 与手机软件连接时，下发命令有限，且打印到缓存中，可以输出打印 */
    (void)vos_printf("vcom receive cmd, length : 0x%x, sum length : 0x%x.\n", \
        uslength, g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvLen);


    if(VOS_OK != CPM_ComRcv(CPM_VCOM_CFG_PORT, pData, uslength))
    {
        g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvErrNum++;
        g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvErrLen += uslength;
    }

    return VOS_OK;
}


VOS_UINT32 PPM_VComIndSendData(VOS_UINT8 *pucVirAddr, VOS_UINT8 *pucPhyAddr, VOS_UINT32 ulDataLen)
{
    VOS_UINT32          ulInSlice;
    VOS_UINT32          ulOutSlice;
    VOS_UINT32          ulWriteSlice;
    VOS_UINT32          ret;
    
    g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendNum++;
    g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendLen += ulDataLen;

    ulInSlice = mdrv_timer_get_normal_timestamp();

    ret = DMS_WriteOmData(DMS_VCOM_OM_CHAN_DATA, pucVirAddr, ulDataLen);

    ulOutSlice = mdrv_timer_get_normal_timestamp();

    ulWriteSlice = (ulInSlice > ulOutSlice) ? (0xffffffff - ulInSlice + ulOutSlice) : (ulOutSlice - ulInSlice);

    if(ulWriteSlice > g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulMaxTimeLen)
    {
        g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulMaxTimeLen = ulWriteSlice;
    }
    
    if (VOS_OK != ret)
    {
        g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendErrNum++;
        g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendErrLen += ulDataLen;

        return CPM_SEND_ERR;
    }

    return CPM_SEND_OK;
}


OM_VCOM_DEBUG_INFO *PPM_VComGetIndInfo(VOS_VOID)
{
    return &g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND];
}



VOS_VOID PPM_VComCfgPortInit(VOS_VOID)
{
    /* 配置数据走VCOM28，会有数据下发 */
    DMS_RegOmChanDataReadCB(DMS_VCOM_OM_CHAN_CTRL, PPM_VComCfgReadData);
    /*CTRL口事件回调*/
    DMS_RegOmChanEventCB(DMS_VCOM_OM_CHAN_CTRL, PPM_VComEvtCB);

    CPM_PhySendReg(CPM_VCOM_CFG_PORT, PPM_VComCfgSendData);

    return;
}


VOS_VOID PPM_VComIndPortInit(VOS_VOID)
{
    /* 可维可测数据数据上报走VCOM31，不会有数据下发 */
    DMS_RegOmChanDataReadCB(DMS_VCOM_OM_CHAN_DATA, VOS_NULL_PTR);
    /*DATA口事件回调*/
    DMS_RegOmChanEventCB(DMS_VCOM_OM_CHAN_DATA, PPM_VComEvtCB);

    CPM_PhySendReg(CPM_VCOM_IND_PORT, PPM_VComIndSendData);

    return;
}


VOS_VOID PPM_VComPortInit(VOS_VOID)
{
    (VOS_VOID)VOS_MemSet_s(&g_stVComDebugInfo[0], sizeof(g_stVComDebugInfo), 0, 3*sizeof(OM_VCOM_DEBUG_INFO));

    /* Vcom 口OM IND通道的初始化 */
    PPM_VComIndPortInit();

    /* Vcom 口OM CNF通道的初始化 */
    PPM_VComCfgPortInit();

    /* Vcom 口errorlog通道的初始化 */
    GU_OamErrLogVComPortInit();

    return;
}


VOS_VOID PPM_VComInfoShow(VOS_VOID)
{
    (void)vos_printf(" VCom30 Send num is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMSendNum);
    (void)vos_printf(" VCom30 Send Len is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMSendLen);

    (void)vos_printf(" VCom30 Send Error num is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMSendErrNum);
    (void)vos_printf(" VCom30 Send Error Len is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMSendErrLen);

    (void)vos_printf(" VCom30 receive num is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMRcvNum);
    (void)vos_printf(" VCom30 receive Len is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMRcvLen);

    (void)vos_printf(" VCom30 receive Error num is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMRcvErrNum);
    (void)vos_printf(" VCom30 receive Error Len is         %d.\n\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CBT].ulVCOMRcvErrLen);


    (void)vos_printf(" VCom28 Send num is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendNum);
    (void)vos_printf(" VCom28 Send Len is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendLen);

    (void)vos_printf(" VCom28 Send Error num is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendErrNum);
    (void)vos_printf(" VCom28 Send Error Len is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMSendErrLen);

    (void)vos_printf(" VCom28 receive num is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvNum);
    (void)vos_printf(" VCom28 receive Len is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvLen);

    (void)vos_printf(" VCom28 receive Error num is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvErrNum);
    (void)vos_printf(" VCom28 receive Error Len is         %d.\n\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_CNF].ulVCOMRcvErrLen);


    (void)vos_printf(" VCom31 Send num is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendNum);
    (void)vos_printf(" VCom31 Send Len is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendLen);

    (void)vos_printf(" VCom31 Send Error num is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendErrNum);
    (void)vos_printf(" VCom31 Send Error Len is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMSendErrLen);

    (void)vos_printf(" VCom31 receive num is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMRcvNum);
    (void)vos_printf(" VCom31 receive Len is           %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMRcvLen);

    (void)vos_printf(" VCom31 receive Error num is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMRcvErrNum);
    (void)vos_printf(" VCom31 receive Error Len is         %d.\n", g_stVComDebugInfo[OM_LOGIC_CHANNEL_IND].ulVCOMRcvErrLen);

    return;
}





