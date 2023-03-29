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
#ifndef __RNIC_INTERFACE_H__
#define __RNIC_INTERFACE_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"

#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
/*================================================*/
/* 数值宏定义 */
/*================================================*/

/*================================================*/
/* 功能函数宏定义 */
/*================================================*/

/*******************************************************************************
  3 枚举定义
*******************************************************************************/

enum RNIC_RMNET_ID_ENUM
{
    RNIC_RMNET_ID_0,                                                            /* 网卡0 */
    RNIC_RMNET_ID_1,                                                            /* 网卡1 */
    RNIC_RMNET_ID_2,                                                            /* 网卡2 */
#if (FEATURE_ON == FEATURE_MULTI_MODEM)
    RNIC_RMNET_ID_3,                                                            /* 网卡3 */
    RNIC_RMNET_ID_4,                                                            /* 网卡4 */
#if (MULTI_MODEM_NUMBER == 3)
    RNIC_RMNET_ID_5,                                                            /* 网卡5 */
    RNIC_RMNET_ID_6,                                                            /* 网卡6 */
#endif
#endif

#ifdef CONFIG_VOWIFI_NEW_FRW
    RNIC_RMNET_ID_IMS00,                                                        /* 主卡的VT网卡 */
    RNIC_RMNET_ID_R_IMS00,                                                      /* 主卡VoWiFi信令转发网卡 */

#if (FEATURE_ON == FEATURE_MULTI_MODEM)
    RNIC_RMNET_ID_IMS10,                                                        /* 副卡的VT网卡 */
    RNIC_RMNET_ID_R_IMS10,                                                      /* 副卡VoWiFi信令转发网卡 */
#endif

    /* 当前只有mbb产品使用spe */
#if (defined(CONFIG_BALONG_SPE))

#else
    RNIC_RMNET_ID_TUN00,                                                        /* modem0的隧道网卡，仅用来挂载IP地址 */
    RNIC_RMNET_ID_TUN01,
    RNIC_RMNET_ID_TUN02,
    RNIC_RMNET_ID_TUN03,
    RNIC_RMNET_ID_TUN04,

#if (FEATURE_ON == FEATURE_MULTI_MODEM)
    RNIC_RMNET_ID_TUN10,                                                        /* modem1的隧道网卡，仅用来挂载IP地址 */
    RNIC_RMNET_ID_TUN11,
    RNIC_RMNET_ID_TUN12,
    RNIC_RMNET_ID_TUN13,
    RNIC_RMNET_ID_TUN14,
#endif
#endif

#else
    RNIC_RMNET_ID_VT,                                                           /* VT绑定的网卡 */

    RNIC_RMNET_ID_VOWIFI,

    RNIC_RMNET_ID_IMS1,
#endif

    RNIC_RMNET_ID_EMC0,

    RNIC_RMNET_ID_BUTT
};
typedef VOS_UINT8 RNIC_RMNET_ID_ENUM_UINT8;

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/

/*****************************************************************************
  5 消息头定义
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
  H2ASN顶级消息结构定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/



#if (VOS_OS_VER == VOS_WIN32)
#pragma pack()
#else
#pragma pack(0)
#endif

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* RnicInterface.h */

