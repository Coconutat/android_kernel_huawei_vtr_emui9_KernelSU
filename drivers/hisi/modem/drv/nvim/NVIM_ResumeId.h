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

#ifndef _NVIM_RESUMEID_H
#define _NVIM_RESUMEID_H

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "product_config.h"

#if defined(DMT)
#include "comm_adapter.h"
#endif

extern unsigned short   g_ausNvResumeManufactureIdList[];

extern unsigned short   g_ausNvResumeUserIdList[];

extern unsigned short   g_ausNvResumeSecureIdList[];

#if defined(DMT)
/*lint -e553 */
#if (FEATURE_ON == FEATURE_S_SKU_M_OPERATORS)
extern unsigned short   g_ausNvResumeSkuSpecIdList[];
#endif
/*lint +e553 */

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/* NV恢复表类型 */
enum NV_RESUME_ITEM
{
    NV_MANUFACTURE_ITEM = 0,
    NV_USER_ITEM,
    NV_SECURE_ITEM,
#if (FEATURE_ON == FEATURE_S_SKU_M_OPERATORS)
    NV_SKU_ITEM,
#endif
    NV_ITEM_BUTT
};
typedef unsigned int NV_RESUME_ITEM_ENUM_UINT32;

extern unsigned int NV_GetResumeNvIdNum(NV_RESUME_ITEM_ENUM_UINT32 enNvItem);

#else
extern unsigned long bsp_nvm_getRevertNum(unsigned long enNvItem);
#endif

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif



#endif

