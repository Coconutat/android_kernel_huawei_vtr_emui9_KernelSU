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
#ifndef __ATMUXINTERFACE_H__
#define __ATMUXINTERFACE_H__


/*****************************************************************************
      1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "product_config.h"

#ifdef __cplusplus
#if __cplusplus
    extern "C" {
#endif
#endif

#pragma pack(4)


/*****************************************************************************
  1 消息头定义
*****************************************************************************/


/*****************************************************************************
  2 宏定义
*****************************************************************************/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/

enum AT_MUX_DLCI_TYPE_ENUM
{
    AT_MUX_DLCI1_ID                 = 1, /* 链路号1，对应HSIC端口14, 音频切换端口 */
    AT_MUX_DLCI2_ID                 = 2, /* 链路号2，对应HSIC端口15, AGPS端口 */
    AT_MUX_DLCI3_ID                 = 3, /* 链路号3，对应HSIC端口16, FELICA/NFC端口 */
    AT_MUX_DLCI4_ID                 = 4, /* 链路号4，对应HSIC端口17, ISDB端口 */
    AT_MUX_DLCI5_ID                 = 5, /* 链路号5 */
    AT_MUX_DLCI6_ID                 = 6, /* 链路号6 */
    AT_MUX_DLCI7_ID                 = 7, /* 链路号7 */
    AT_MUX_DLCI8_ID                 = 8, /* 链路号8 */
    AT_MUX_DLCI9_ID                 = 9, /* 链路号9 */
    AT_MUX_DLCI10_ID                = 10, /* 链路号10 */
    AT_MUX_DLCI11_ID                = 11, /* 链路号11 */
    AT_MUX_DLCI12_ID                = 12, /* 链路号12 */
    AT_MUX_DLCI13_ID                = 13, /* 链路号13 */
    AT_MUX_DLCI14_ID                = 14, /* 链路号14 */
    AT_MUX_DLCI15_ID                = 15, /* 链路号15 */
    AT_MUX_DLCI16_ID                = 16, /* 链路号16 */

    AT_MUX_DLCI_TYPE_BUTT           = 65  /*链路号最多到64号 */
};
typedef VOS_UINT8 AT_MUX_DLCI_TYPE_ENUM_UINT8;

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/

/*****************************************************************************
  6 消息定义
*****************************************************************************/

/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/

/*****************************************************************************
  8 UNION定义
*****************************************************************************/

/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/


/*****************************************************************************
  10 函数声明
*****************************************************************************/
VOS_UINT32 MUX_DlciDlDataSend (AT_MUX_DLCI_TYPE_ENUM_UINT8 enDlci, VOS_UINT8* pData, VOS_UINT16 usDataLen);

typedef VOS_UINT32 (*RCV_UL_DLCI_DATA_FUNC)(AT_MUX_DLCI_TYPE_ENUM_UINT8 enDlci, VOS_UINT8* pData, VOS_UINT16 usDataLen);

VOS_UINT32 MUX_AtRgstUlPortCallBack (AT_MUX_DLCI_TYPE_ENUM_UINT8 enDlci, RCV_UL_DLCI_DATA_FUNC pFunc);

#if ((VOS_OS_VER == VOS_WIN32) || (VOS_OS_VER == VOS_NUCLEUS))
#pragma pack()
#else
#pragma pack(0)
#endif


#ifdef __cplusplus
    #if __cplusplus
            }
    #endif
#endif

#endif


