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

#include "msp_errno.h"
#include <dms.h>
#include "dms_core.h"
#include "vos.h"
#include "TafTypeDef.h"




/*****************************************************************************
    协议栈打印打点方式下的.C文件宏定义
*****************************************************************************/

#define THIS_FILE_ID                    PS_FILE_ID_DMS_ACM_AT_TX_C


/*****************************************************************************
  2 全局变量定义
*****************************************************************************/

DMS_STATIC_BUF_STRU                     stDmsStaticBufInfo ;

VOS_UINT8                              *g_aucStaticBuf = NULL;


/*****************************************************************************
  3 函数实现
*****************************************************************************/


VOS_INT32 DMS_WriteData(DMS_PHY_BEAR_ENUM enPhyBear, VOS_UINT8 *pucData, VOS_UINT16 usLen)
{
    VOS_UINT8                          *pucSenBuf = NULL;
    VOS_INT32                           lRet      = VOS_ERROR;

    if ((NULL == pucData) || (0 == usLen))
    {
        return VOS_ERROR;
    }

    DMS_LOG_INFO("DMS_WriteData: PortNo = %d, len = %d, buf = %s\n", enPhyBear, usLen, pucData);

    if ( (DMS_PHY_BEAR_USB_PCUI == enPhyBear)
      || (DMS_PHY_BEAR_USB_CTRL == enPhyBear)
      || (DMS_PHY_BEAR_USB_PCUI2 == enPhyBear) )
    {
        pucSenBuf = Dms_GetStaticBuf(usLen);

        if (NULL == pucSenBuf)
        {
            return VOS_ERROR;
        }

        TAF_MEM_CPY_S(pucSenBuf, usLen, pucData, usLen);

        lRet = (VOS_INT32)DMS_VcomWriteAsync(enPhyBear, pucSenBuf, usLen);

        if (ERR_MSP_SUCCESS != lRet)
        {
            Dms_FreeStaticBuf(pucSenBuf);
        }

    }
    else if (DMS_PHY_BEAR_USB_NCM == enPhyBear)
    {
        pucSenBuf = Dms_GetStaticBuf(usLen);

        if (NULL == pucSenBuf)
        {
            return VOS_ERROR;
        }

        TAF_MEM_CPY_S(pucSenBuf, usLen, pucData, usLen);

        lRet = (VOS_INT32)DMS_NcmSendData(pucSenBuf, usLen);

        if (ERR_MSP_SUCCESS != lRet)
        {
            Dms_FreeStaticBuf(pucSenBuf);
        }
    }
    else
    {
        lRet = VOS_ERROR;
    }

    return lRet;
}


VOS_VOID Dms_StaticBufInit(VOS_VOID)
{
    VOS_UINT32 i = 0;
    VOS_UINT8 * pTemp = NULL;

    VOS_UINT32 ulBufSize;

    ulBufSize = (DMS_LOG_STATIC_ONE_BUF_SIZE*DMS_LOG_STATIC_BUF_NUM + 32);
    g_aucStaticBuf = kmalloc(ulBufSize, GFP_KERNEL|__GFP_DMA);

    if(g_aucStaticBuf == VOS_NULL)
    {
        return ;
    }


    /*取32字节对齐的地址*/
    pTemp = g_aucStaticBuf + (32 - ((VOS_ULONG )g_aucStaticBuf%32));

    stDmsStaticBufInfo.enBufType      = DMS_BUF_TYP_DYMIC;

    /* 初始化缓冲信息*/
    for (i = 0; i < DMS_LOG_STATIC_BUF_NUM; i++)
    {
        stDmsStaticBufInfo.stBufSta[i].pcuBuf = (VOS_UINT8 *)((VOS_ULONG)i * DMS_LOG_STATIC_ONE_BUF_SIZE + pTemp);
        stDmsStaticBufInfo.stBufSta[i].enBusy = STATIC_BUF_STA_IDLE;
    }

    return ;

}

VOS_UINT8* Dms_GetStaticBuf(VOS_UINT32 ulLen)
{
    VOS_UINT32 i = 0;
    VOS_UINT32 *buf = NULL;

    /*数据长度超过静态buf最大size，动态申请buf*/
    if(ulLen >DMS_LOG_STATIC_ONE_BUF_SIZE )
    {

        buf = kmalloc(ulLen, GFP_KERNEL|__GFP_DMA);
        return (VOS_UINT8* )buf;
    }

    for (i = 0; i < DMS_LOG_STATIC_BUF_NUM; i++)
    {
        if (stDmsStaticBufInfo.stBufSta[i].enBusy == STATIC_BUF_STA_IDLE)
        {
            stDmsStaticBufInfo.stBufSta[i].enBusy = STATIC_BUF_STA_BUSY;

            return  stDmsStaticBufInfo.stBufSta[i].pcuBuf;
        }
    }

    /*极限场景下 如果静态buf用完，申请动态内存使用*/
    buf = kmalloc(ulLen, GFP_KERNEL|__GFP_DMA);

    return (VOS_UINT8* )buf;

}



 VOS_BOOL Dms_IsStaticBuf(VOS_UINT8 *buf)
 {

    if(( buf >= g_aucStaticBuf )
        &&(buf < g_aucStaticBuf +DMS_LOG_STATIC_ONE_BUF_SIZE * DMS_LOG_STATIC_BUF_NUM  +32))
    {
        return VOS_TRUE;
    }
    else
    {
        return VOS_FALSE;
    }

}


VOS_VOID Dms_FreeStaticBuf( VOS_UINT8 * buf)
{
    VOS_UINT32 i = 0;

    if (NULL == buf)
    {
        return ;
    }

    /*静态buf释放*/
    for (i = 0; i < DMS_LOG_STATIC_BUF_NUM; i++)
    {
        if (stDmsStaticBufInfo.stBufSta[i].pcuBuf == buf)
        {
            stDmsStaticBufInfo.stBufSta[i].enBusy = STATIC_BUF_STA_IDLE;
            return ;
        }
    }

    /*动态buf释放*/
    if(i == DMS_LOG_STATIC_BUF_NUM)
    {
        kfree(buf );
    }


    return ;
}

VOS_UINT32 DMS_VcomWriteAsync(
    DMS_PHY_BEAR_ENUM                   enPhyBear,
    VOS_UINT8                          *pucDataBuf,
    VOS_UINT32                          ulLen
)
{
    DMS_PHY_BEAR_PROPERTY_STRU         *pstPhyBearProp = NULL;
    ACM_WR_ASYNC_INFO                   stAcmInfo = {0};
    UDI_HANDLE                          lHandle = UDI_INVALID_HANDLE;
    VOS_INT32                           lRet = ERR_MSP_SUCCESS;

    stAcmInfo.pVirAddr = (VOS_CHAR *)pucDataBuf;
    stAcmInfo.pPhyAddr = NULL;
    stAcmInfo.u32Size  = ulLen;

    pstPhyBearProp = DMS_GetPhyBearProperty(enPhyBear);

    lHandle = pstPhyBearProp->lPortHandle;
    if (UDI_INVALID_HANDLE == lHandle)
    {
        DMS_LOG_INFO("DMS_VcomWriteAsync[%d]: INVALID HANDLE.\n", enPhyBear);
        return ERR_MSP_FAILURE;
    }

    if (ACM_EVT_DEV_SUSPEND == pstPhyBearProp->ucChanStat)
    {
        DMS_LOG_INFO("DMS_VcomWriteAsync[%d]: DEV SUSPEND.\n", enPhyBear);
        return ERR_MSP_FAILURE;
    }

    DMS_DBG_SDM_FUN((DMS_SDM_MSG_ID_ENUM)(DMS_SDM_VCOM_WRT_BEGIN + (VOS_UINT32)enPhyBear),\
                    ulLen, 0, 0);

    lRet = mdrv_udi_ioctl(lHandle, ACM_IOCTL_WRITE_ASYNC, &stAcmInfo);
    if (ERR_MSP_SUCCESS == lRet)
    {
        DMS_DBG_SDM_FUN((DMS_SDM_MSG_ID_ENUM)(DMS_SDM_VCOM_WRT_SUSS_BEGIN + (VOS_UINT32)enPhyBear),\
                        ulLen, 0, 0);
    }

    return (VOS_UINT32)lRet;
}



