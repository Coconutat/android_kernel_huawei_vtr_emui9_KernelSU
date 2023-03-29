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
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/
#define    THIS_FILE_ID        PS_FILE_ID_CPM_C


/*****************************************************************************
  1 头文件包含
*****************************************************************************/
#include "cpm.h"
#include "NVIM_Interface.h"
#include "mdrv.h"
#include "diag_api.h"

/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

CPM_PHY_PORT_CFG_STRU                   g_astCPMPhyPortCfg[CPM_PORT_BUTT - CPM_IND_PORT];
CPM_LOGIC_PORT_CFG_STRU                 g_astCPMLogicPortCfg[CPM_COMM_BUTT];

/* 端口配置全局变量 */
OM_CHANNLE_PORT_CFG_STRU                g_stPortCfg;

/* 逻辑端口发送错误统计 */
CPM_COM_PORT_SND_ERR_INFO_STRU          g_stCPMSndErrInfo = {0};

/* 物理端口发送错误统计 */
CPM_COM_PORT_RCV_ERR_INFO_STRU          g_stCPMRcvErrInfo = {0};

/*****************************************************************************
  3 函数体申明
*****************************************************************************/

/*****************************************************************************
  4 函数体定义
*****************************************************************************/

/*****************************************************************************
 函 数 名  : CPM_PhySendReg
 功能描述  : 提供给外部的注册函数，用来物理通道接收到数据的处理
 输入参数  : enPhyPort：  注册的物理通道号
             pRecvFunc：  数据接收函数
 输出参数  : 无
 返 回 值  : 无

*****************************************************************************/
VOS_VOID CPM_PhySendReg(CPM_PHY_PORT_ENUM_UINT32 enPhyPort, CPM_SEND_FUNC pSendFunc)
{
    if (CPM_PORT_BUTT > enPhyPort)
    {
        CPM_PHY_SEND_FUNC(enPhyPort - CPM_IND_PORT) = pSendFunc;
    }

    return;
}

/*****************************************************************************
 函 数 名  : CPM_LogicRcvReg
 功能描述  : 给逻辑通道注册接收函数
 输入参数  : enLogicPort： 注册的逻辑通道号
             pRecvFunc：   数据接收函数
 输出参数  : 无
 返 回 值  : 无

*****************************************************************************/
VOS_VOID CPM_LogicRcvReg(CPM_LOGIC_PORT_ENUM_UINT32 enLogicPort, CPM_RCV_FUNC pRcvFunc)
{
    if (CPM_COMM_BUTT > enLogicPort)
    {
        CPM_LOGIC_RCV_FUNC(enLogicPort) = pRcvFunc;
    }

    return;
}

/*****************************************************************************
 函 数 名  : CPM_QueryPhyPort
 功能描述  : 查询当前逻辑通道使用的物理端口
 输入参数  : enLogicPort：  逻辑通道号
 输出参数  : 无
 返 回 值  : 物理通道号

*****************************************************************************/
CPM_PHY_PORT_ENUM_UINT32 CPM_QueryPhyPort(CPM_LOGIC_PORT_ENUM_UINT32 enLogicPort)
{
    return CPM_LOGIC_PHY_PORT(enLogicPort);
}

/*****************************************************************************
 函 数 名  : CPM_ConnectPorts
 功能描述  : 将物理通道和逻辑通道连接上
 输入参数  : enPhyPort：    物理通道号
             enLogicPort：  逻辑通道号
 输出参数  : 无
 返 回 值  : 无

*****************************************************************************/
VOS_VOID CPM_ConnectPorts(CPM_PHY_PORT_ENUM_UINT32 enPhyPort, CPM_LOGIC_PORT_ENUM_UINT32 enLogicPort)
{
    if ((CPM_PORT_BUTT <= enPhyPort) || (CPM_COMM_BUTT <= enLogicPort))
    {
        return;
    }

    /* 连接发送通道 */
    CPM_LOGIC_SEND_FUNC(enLogicPort)= CPM_PHY_SEND_FUNC(enPhyPort - CPM_IND_PORT);

    /* 连接接收通道 */
    CPM_PHY_RCV_FUNC(enPhyPort - CPM_IND_PORT) = CPM_LOGIC_RCV_FUNC(enLogicPort);

    /* 将物理发送函数注册给逻辑通道 */
    CPM_LOGIC_PHY_PORT(enLogicPort) = enPhyPort;

    return;
}

/*****************************************************************************
 函 数 名  : CPM_DisconnectPorts
 功能描述  : 断开物理通道和逻辑通道连接
 输入参数  : enPhyPort：    物理通道号
             enLogicPort：  逻辑通道号
 输出参数  : 无
 返 回 值  : 无

*****************************************************************************/
VOS_VOID CPM_DisconnectPorts(CPM_PHY_PORT_ENUM_UINT32 enPhyPort, CPM_LOGIC_PORT_ENUM_UINT32 enLogicPort)
{
    if ((CPM_PORT_BUTT <= enPhyPort) || (CPM_COMM_BUTT <= enLogicPort))
    {
        return;
    }

    /* 假如当前逻辑通道并没有使用此物理通道，则不用处理 */
    if (enPhyPort != CPM_LOGIC_PHY_PORT(enLogicPort))
    {
        return;
    }

    /* 断开接收通道 */
    CPM_PHY_RCV_FUNC(enPhyPort - CPM_IND_PORT) = VOS_NULL_PTR;

    /* 断开发送通道 */
    CPM_LOGIC_SEND_FUNC(enLogicPort)= VOS_NULL_PTR;
    CPM_LOGIC_PHY_PORT(enLogicPort) = CPM_PORT_BUTT;

    return;
}


/*****************************************************************************
 函 数 名  : CPM_PortAssociateInit
 功能描述  : 根据端口类型关联物理端口和逻辑端口
 输入参数  : VOS_VOID
 输出参数  : 无
 返 回 值  : VOS_OK:成功，其他为失败
*****************************************************************************/
VOS_UINT32 CPM_PortAssociateInit(VOS_VOID)
{
    VOS_UINT32                          i;

    for (i = 0; i < CPM_COMM_BUTT; i++)
    {
        g_astCPMLogicPortCfg[i].enPhyPort = CPM_PORT_BUTT;
    }

    /* Modified by h59254 for AP-Modem Personalisation Project, 2012/04/12, begin */
    /* 产品支持HSIC特性，直接返回成功，不做端口关联 */
    if (BSP_MODULE_SUPPORT == mdrv_misc_support_check(BSP_MODULE_TYPE_HSIC))
    {
        return VOS_OK;
    }
    /* Modified by h59254 for AP-Modem Personalisation Project, 2012/04/12, end */

    /* 读取OM的物理输出通道 */
    if (NV_OK != NV_Read(en_NV_Item_Om_Port_Type, &g_stPortCfg, sizeof(OM_CHANNLE_PORT_CFG_STRU)))
    {
        return VOS_ERR;
    }

    /* 检测参数*/
    if (CPM_OM_PORT_TYPE_USB == g_stPortCfg.enPortNum)
    {
        CPM_ConnectPorts(CPM_CFG_PORT, CPM_OM_CFG_COMM);
        CPM_ConnectPorts(CPM_IND_PORT, CPM_OM_IND_COMM);
    }
    else if (CPM_OM_PORT_TYPE_VCOM == g_stPortCfg.enPortNum)
    {
        CPM_ConnectPorts(CPM_VCOM_CFG_PORT, CPM_OM_CFG_COMM);
        CPM_ConnectPorts(CPM_VCOM_IND_PORT, CPM_OM_IND_COMM);
    }
    else if (CPM_OM_PORT_TYPE_WIFI == g_stPortCfg.enPortNum)
    {
        CPM_ConnectPorts(CPM_WIFI_AT_PORT,     CPM_AT_COMM);
        CPM_ConnectPorts(CPM_WIFI_OM_IND_PORT, CPM_OM_IND_COMM);
        CPM_ConnectPorts(CPM_WIFI_OM_CFG_PORT, CPM_OM_CFG_COMM);
    }
    /* NV项不正确时按USB输出处理 */
    else
    {
        CPM_ConnectPorts(CPM_CFG_PORT, CPM_OM_CFG_COMM);
        CPM_ConnectPorts(CPM_IND_PORT, CPM_OM_IND_COMM);

        g_stPortCfg.enPortNum = CPM_OM_PORT_TYPE_USB;
    }

    /*如果当前连接为USB输出，需要设置SOCP默认超时*/
    if(g_stPortCfg.enPortNum == CPM_OM_PORT_TYPE_USB)
    {
        mdrv_socp_set_ind_mode(SOCP_IND_MODE_DIRECT);
    }

    return VOS_OK;
}


/*****************************************************************************
 函 数 名  : CPM_ComSend
 功能描述  : 发送数据函数，提供给逻辑通道使用
 输入参数  : enLogicPort：逻辑通道号
             pucVirData:  数据虚拟地址
             pucPHYData:  数据物理地址
             pucData：    发送数据的指针
             ulLen:       发送数据的长度

 输出参数  : 无
 返 回 值  : VOS_OK:成功，其他为失败

*****************************************************************************/
VOS_UINT32 CPM_ComSend(CPM_LOGIC_PORT_ENUM_UINT32 enLogicPort, VOS_UINT8 *pucVirData, VOS_UINT8 *pucPHYData, VOS_UINT32 ulLen)
{
    if(CPM_OM_CFG_COMM == enLogicPort)
    {
        diag_PTR(EN_DIAG_PTR_CPM_COMSEND);
    }

    /* 参数检测 */
    if (CPM_COMM_BUTT <= enLogicPort)
    {
        g_stCPMSndErrInfo.ulPortErr++;

        return CPM_SEND_PARA_ERR;
    }

    if ((VOS_NULL_PTR == pucVirData) || (0 == ulLen))
    {
        g_stCPMSndErrInfo.astCPMSndErrInfo[enLogicPort].ulParaErr++;

        return CPM_SEND_PARA_ERR;
    }

    if (VOS_NULL_PTR == CPM_LOGIC_SEND_FUNC(enLogicPort))
    {
        g_stCPMSndErrInfo.astCPMSndErrInfo[enLogicPort].ulNullPtr++;

        return CPM_SEND_FUNC_NULL;
    }

    return CPM_LOGIC_SEND_FUNC(enLogicPort)(pucVirData, pucPHYData, ulLen);
}

/*****************************************************************************
 函 数 名  : CPM_ComRcv
 功能描述  : 接收数据函数，提供给物理通道使用
 输入参数  : enPhyPort：  物理通道号
             pucData：    接收数据的指针
             ulLen:       接收数据的长度
 输出参数  : 无
 返 回 值  : VOS_OK:成功，其他为失败

*****************************************************************************/
VOS_UINT32 CPM_ComRcv(CPM_PHY_PORT_ENUM_UINT32 enPhyPort, VOS_UINT8 *pucData, VOS_UINT32 ulLen)
{
    /* 参数检测 */
    if (CPM_PORT_BUTT <= enPhyPort)
    {
        g_stCPMRcvErrInfo.ulPortErr++;
        diag_PTR(EN_DIAG_PTR_CPM_ERR1);

        return VOS_ERR;
    }

    if ((VOS_NULL_PTR == pucData) || (0 == ulLen))
    {
        g_stCPMRcvErrInfo.astCPMRcvErrInfo[enPhyPort - CPM_IND_PORT].ulParaErr++;
        diag_PTR(EN_DIAG_PTR_CPM_ERR2);

        return VOS_ERR;
    }

    if (VOS_NULL_PTR == CPM_PHY_RCV_FUNC(enPhyPort - CPM_IND_PORT))
    {
        (void)vos_printf("\r\nCPM_ComRcv The Phy Port %d Rec Func is NULL", (VOS_INT)enPhyPort);

        g_stCPMRcvErrInfo.astCPMRcvErrInfo[enPhyPort - CPM_IND_PORT].ulNullPtr++;
        diag_PTR(EN_DIAG_PTR_CPM_ERR3);

        return VOS_ERR;
    }

    diag_PTR(EN_DIAG_PTR_CPM_COMRCV);

    return CPM_PHY_RCV_FUNC(enPhyPort - CPM_IND_PORT)(pucData, ulLen);
}

/*****************************************************************************
 函 数 名  : CPM_Show
 功能描述  : 显示当前的逻辑和物理端口对应关系
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无

*****************************************************************************/
VOS_VOID CPM_Show(VOS_VOID)
{
    CPM_PHY_PORT_ENUM_UINT32    enPhyPort;
    CPM_LOGIC_PORT_ENUM_UINT32  enLogicPort;

    (void)vos_printf("\r\nCPM_Show The Logic and Phy Relation is :");

    for(enLogicPort=CPM_AT_COMM; enLogicPort<CPM_COMM_BUTT; enLogicPort++)
    {
        enPhyPort = CPM_QueryPhyPort(enLogicPort);

        if(CPM_OM_IND_COMM == enLogicPort) 
        {
            (void)vos_printf("\r\nThe Logic Port %d OM IND is connnect PHY ", enLogicPort);
        }
        else if(CPM_OM_CFG_COMM == enLogicPort) 
        {
            (void)vos_printf("\r\nThe Logic Port %d OM CFG is connnect PHY ", enLogicPort);
        }
        else
        {
            (void)vos_printf("\r\nThe Logic Port %d        is connnect PHY ", enLogicPort);
        }
        
        if((CPM_IND_PORT == enPhyPort) || (CPM_CFG_PORT == enPhyPort))
        {
            (void)vos_printf("Port %d(USB Port).", enPhyPort);
        }
        else if((CPM_WIFI_OM_IND_PORT == enPhyPort) || (CPM_WIFI_OM_CFG_PORT == enPhyPort))
        {
            (void)vos_printf("Port %d(socket).", enPhyPort);
        }
        else if((CPM_VCOM_IND_PORT == enPhyPort) || (CPM_VCOM_CFG_PORT == enPhyPort))
        {
            (void)vos_printf("Port %d(netlink).", enPhyPort);
        }
        else
        {
            (void)vos_printf("Port %d.", enPhyPort);
        }
    }

    (void)vos_printf("\r\nCPM_Show The Phy Info is :\n");

    for(enPhyPort=0; enPhyPort<(CPM_PORT_BUTT - CPM_IND_PORT); enPhyPort++)
    {
        /* 为打印出函数名称，使用printk */
        (void)printk("The Phy %d Port's Rec Func is %pS.\n \
                Send Func is %pS.\n", \
                        enPhyPort, \
                        g_astCPMPhyPortCfg[enPhyPort].pRcvFunc, \
                        g_astCPMPhyPortCfg[enPhyPort].pSendFunc);
    }

    for(enLogicPort=0; enLogicPort<CPM_COMM_BUTT; enLogicPort++)
    {
        /* 为打印出函数名称，使用printk */
        (void)printk("The Logic %d Port's Rec Func is %pS.\n \
                  Send Func is %pS.\n", \
                        enLogicPort, \
                        g_astCPMLogicPortCfg[enLogicPort].pRcvFunc, \
                        g_astCPMLogicPortCfg[enLogicPort].pSendFunc);
    }

    return;
}

/*****************************************************************************
 函 数 名  : CPM_ComErrShow
 功能描述  : 显示当前的逻辑和物理端口错误显示
 输入参数  : 无
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
VOS_VOID CPM_ComErrShow(VOS_VOID)
{
    CPM_LOGIC_PORT_ENUM_UINT32  enLogicPort;
    CPM_PHY_PORT_ENUM_UINT32    enPhyPort;

    (void)vos_printf("\r\nCPM_ComErrShow:");

    (void)vos_printf("\r\nLogic Port Err Times: %d", g_stCPMSndErrInfo.ulPortErr);

    for (enLogicPort = 0; enLogicPort < CPM_COMM_BUTT; enLogicPort++)
    {
        (void)vos_printf("\r\nLogic %d Port Para Err Times: %d", enLogicPort, g_stCPMSndErrInfo.astCPMSndErrInfo[enLogicPort].ulParaErr);
        (void)vos_printf("\r\nLogic %d Port Null Ptr Times: %d", enLogicPort, g_stCPMSndErrInfo.astCPMSndErrInfo[enLogicPort].ulNullPtr);
    }

    (void)vos_printf("\r\nPhy Port Err Times: %d", g_stCPMRcvErrInfo.ulPortErr);

    for (enPhyPort = 0; enPhyPort < (CPM_PORT_BUTT - CPM_IND_PORT); enPhyPort++)
    {
        (void)vos_printf("\r\nPhy %d Port Para Err Times: %d", enPhyPort, g_stCPMRcvErrInfo.astCPMRcvErrInfo[enPhyPort].ulParaErr);
        (void)vos_printf("\r\nPhy %d Port Null Ptr Times: %d", enPhyPort, g_stCPMRcvErrInfo.astCPMRcvErrInfo[enPhyPort].ulNullPtr);
    }

    return;
}

