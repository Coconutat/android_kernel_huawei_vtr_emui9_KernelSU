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

/*lint --e{750} */
#include <linux/slab.h>
#include <linux/err.h>
#include "../../adrv/adrv.h"
#include "mdrv.h"
#include <bsp_icc.h>
#include "udi_balong.h"



static int udiAdpIccInit(void);
static int udiAdpAcmInit(void);

/* 各模块特性值定义 */
#define UDI_USB_ACM_CAPA        (UDI_CAPA_BLOCK_READ | UDI_CAPA_BLOCK_WRITE | UDI_CAPA_READ_CB | UDI_CAPA_WRITE_CB)
#define UDI_USB_NCM_NDIS_CAPA   (UDI_CAPA_READ_CB | UDI_CAPA_BUFFER_LIST)
#define UDI_USB_NCM_CTRL_CAPA   (UDI_CAPA_READ_CB | UDI_CAPA_CTRL_OPT)
#define UDI_ICC_CAPA            (UDI_CAPA_BLOCK_READ | UDI_CAPA_BLOCK_WRITE | UDI_CAPA_READ_CB | UDI_CAPA_WRITE_CB)
#define UDI_UART_CAPA           (UDI_CAPA_BLOCK_READ | UDI_CAPA_BLOCK_WRITE)

/* 各模块初始化函数定义 */
void* g_udiInitFuncTable[(unsigned int)UDI_DEV_MAX+1] =
{

    /* ACM Init */
    udiAdpAcmInit,
    
    NULL,

    /* ICC Init */
    udiAdpIccInit,

    /* UART Init */
    NULL,

    /* HSUART Init */
    NULL,

    NULL,

    /* Must Be END */
    NULL
};

/*** eric **/
static int udiAcmAdpOpen(UDI_OPEN_PARAM_S *param, UDI_HANDLE handle)
{
	//BSP_U8* pstrName;
	unsigned int u32Type;
	void* filep;

	UDI_UNUSED_PARAM(handle);

	u32Type = UDI_GET_DEV_TYPE(param->devid);
	filep = bsp_acm_open(u32Type);

	if (!filep)
	{
		return (-1);
	}

	(void)BSP_UDI_SetPrivate(param->devid, filep);
	return 0;
}
/*lint -e429 -esym(429,*)*/
static int udiAdpAcmInit(void)
{
	UDI_DRV_INTEFACE_TABLE* pDrvInterface;
	unsigned int u32Cnt;

	/* 构造回调函数指针列表 */
	pDrvInterface = kmalloc(sizeof(UDI_DRV_INTEFACE_TABLE), GFP_KERNEL);
	if (NULL == pDrvInterface)
	{
		printk(KERN_ERR "BSP_MODU_UDI NO Mem, line:%d\n", __LINE__);
		return (-1);
	}
	memset((void*)pDrvInterface, 0, sizeof(UDI_DRV_INTEFACE_TABLE));

	/* 只设置需要实现的几个接口 */
	pDrvInterface->udi_open_cb = udiAcmAdpOpen;
	pDrvInterface->udi_close_cb = (UDI_CLOSE_CB_T)bsp_acm_close;
	pDrvInterface->udi_write_cb = (UDI_WRITE_CB_T)bsp_acm_write;
	pDrvInterface->udi_read_cb = (UDI_READ_CB_T)bsp_acm_read;
	pDrvInterface->udi_ioctl_cb = (UDI_IOCTL_CB_T)bsp_acm_ioctl;

	/*  ACM 都使用同一套驱动函数指针 */
	for (u32Cnt = UDI_USB_ACM_CTRL; u32Cnt < UDI_USB_ACM_MAX; u32Cnt++)
	{
		(void)BSP_UDI_SetCapability((UDI_DEVICE_ID_E)UDI_BUILD_DEV_ID(UDI_DEV_USB_ACM, u32Cnt), UDI_USB_ACM_CAPA);
		(void)BSP_UDI_SetInterfaceTable((UDI_DEVICE_ID_E)UDI_BUILD_DEV_ID(UDI_DEV_USB_ACM, u32Cnt), pDrvInterface);
	}

	return 0;
}
/*lint -e429 +esym(429,*)*/

/**************************************************************************
  ICC 适配实现
**************************************************************************/
static UDI_HANDLE sg_chnHandleTbl[ICC_CHAN_NUM_MAX] = {0};

UDI_HANDLE BSP_UDI_ICC_ChnToHandle(unsigned int u32Chn)
{
	return sg_chnHandleTbl[u32Chn];
}

static int udiIccAdpOpen(UDI_OPEN_PARAM_S *param, UDI_HANDLE handle)
{
	unsigned long u32ChanId;
	ICC_CHAN_ATTR_S *pstOpenParam;

	u32ChanId = UDI_GET_DEV_TYPE(param->devid);
	(void)BSP_UDI_SetPrivate(param->devid, (void*)u32ChanId);

	/* 从param 中解析出各个参数 */
	pstOpenParam = (ICC_CHAN_ATTR_S *)param->pPrivate;
	sg_chnHandleTbl[u32ChanId] = handle;     /*lint !e732*/

	return BSP_ICC_Open((unsigned int)u32ChanId, pstOpenParam);
}

/*lint -e429 -esym(429,*)*/
static int udiAdpIccInit(void)
{
	UDI_DRV_INTEFACE_TABLE* pDrvInterface;
	unsigned int u32Cnt;

	/* 构造回调函数指针列表 */
	pDrvInterface = kmalloc(sizeof(UDI_DRV_INTEFACE_TABLE), GFP_KERNEL);
	if (NULL == pDrvInterface)
	{
		printk(KERN_ERR "BSP_MODU_UDI NO Mem, line:%d\n", __LINE__);
		return (-1);
	}
	memset((void*)pDrvInterface, 0, sizeof(UDI_DRV_INTEFACE_TABLE));

	/* 只设置需要实现的几个接口 */
	pDrvInterface->udi_open_cb = (UDI_OPEN_CB_T)udiIccAdpOpen;
	pDrvInterface->udi_close_cb = (UDI_CLOSE_CB_T)BSP_ICC_Close;
	pDrvInterface->udi_write_cb = (UDI_WRITE_CB_T)BSP_ICC_Write;
	pDrvInterface->udi_read_cb = (UDI_READ_CB_T)BSP_ICC_Read;
	pDrvInterface->udi_ioctl_cb = (UDI_IOCTL_CB_T)BSP_ICC_Ioctl;

	/* 几个 ICC 都使用同一套驱动函数指针 */
	for (u32Cnt = 0; u32Cnt < ICC_CHAN_NUM_MAX; u32Cnt++)
	{
		(void)BSP_UDI_SetCapability((UDI_DEVICE_ID_E)UDI_BUILD_DEV_ID(UDI_DEV_ICC, u32Cnt), UDI_ICC_CAPA);
		(void)BSP_UDI_SetInterfaceTable((UDI_DEVICE_ID_E)UDI_BUILD_DEV_ID(UDI_DEV_ICC, u32Cnt), pDrvInterface);
	}

	return 0;
}
/*lint -e429 +esym(429,*)*/






