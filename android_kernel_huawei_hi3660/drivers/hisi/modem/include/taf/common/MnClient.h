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

#ifndef __MNCLIENT_H__
#define __MNCLIENT_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include  "vos.h"
#include  "TafTypeDef.h"
#include  "product_config.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define AT_MAX_CLIENT_NUM               (AT_CLIENT_ID_BUTT)

/*广播Client Id，供MODEM侧使用 */
#define MN_CLIENT_ID_BROADCAST          (0xFFFF)

/* Modified by l60609 for DSDA Phase II, 2012-12-15, Begin */
/* 广播ClientId说明，最高的2个bit位表示ModemId。
   Modem0对应的广播Client为0011111111111111(0x3FFF)，
   Modem1对应的广播Client为0111111111111111(0x7FFF)
   Modem1对应的广播Client为1011111111111111(0xBFFF) */

/* Modem 0的广播Client Id */
#define AT_BROADCAST_CLIENT_ID_MODEM_0  (0x3FFF)

/* Modem 1的广播Client Id */
#define AT_BROADCAST_CLIENT_ID_MODEM_1  (0x7FFF)

/* Modem 2的广播Client Id */
#define AT_BROADCAST_CLIENT_ID_MODEM_2  (0xBFFF)

/* Modified by l60609 for DSDA Phase II, 2012-12-15, End */

#if (FEATURE_ON == FEATURE_VCOM_EXT)
#define AT_MAX_APP_CLIENT_ID            (AT_CLIENT_ID_APP52)
#else
#define AT_MAX_APP_CLIENT_ID            (AT_CLIENT_ID_APP1)
#endif

#define AT_MIN_APP_CLIENT_ID            (AT_CLIENT_ID_APP)

/*****************************************************************************
  3 枚举定义
*****************************************************************************/


enum AT_CLIENT_ID_ENUM
{
    AT_CLIENT_ID_PCUI                 = 0,
    AT_CLIENT_ID_CTRL,
    AT_CLIENT_ID_PCUI2,
    AT_CLIENT_ID_MODEM,
    AT_CLIENT_ID_NDIS,
    AT_CLIENT_ID_UART,
#if (FEATURE_ON == FEATURE_AT_HSUART)
    AT_CLIENT_ID_HSUART,
#endif
    AT_CLIENT_ID_SOCK,
    AT_CLIENT_ID_APPSOCK,
#if (FEATURE_ON == FEATURE_AT_HSIC)
    AT_CLIENT_ID_HSIC1,
    AT_CLIENT_ID_HSIC2,
    AT_CLIENT_ID_HSIC3,
    AT_CLIENT_ID_HSIC4,
    AT_CLIENT_ID_HSIC_MODEM,
    AT_CLIENT_ID_MUX1,
    AT_CLIENT_ID_MUX2,
    AT_CLIENT_ID_MUX3,
    AT_CLIENT_ID_MUX4,
    AT_CLIENT_ID_MUX5,
    AT_CLIENT_ID_MUX6,
    AT_CLIENT_ID_MUX7,
    AT_CLIENT_ID_MUX8,
#endif
    AT_CLIENT_ID_APP,
    AT_CLIENT_ID_APP1,
#if (FEATURE_ON == FEATURE_VCOM_EXT)
    AT_CLIENT_ID_APP2,
    AT_CLIENT_ID_APP3,
    AT_CLIENT_ID_APP4,
    AT_CLIENT_ID_APP5,
    AT_CLIENT_ID_APP6,
    AT_CLIENT_ID_APP7,
    AT_CLIENT_ID_APP8,
    AT_CLIENT_ID_APP9,
    AT_CLIENT_ID_APP10,
    AT_CLIENT_ID_APP11,
    AT_CLIENT_ID_APP12,
    AT_CLIENT_ID_APP13,
    AT_CLIENT_ID_APP14,
    AT_CLIENT_ID_APP15,
    AT_CLIENT_ID_APP16,
    AT_CLIENT_ID_APP17,
    AT_CLIENT_ID_APP18,
    AT_CLIENT_ID_APP19,
    AT_CLIENT_ID_APP20,
    AT_CLIENT_ID_APP21,
    AT_CLIENT_ID_APP22,
    AT_CLIENT_ID_APP23,
    AT_CLIENT_ID_APP24,
    AT_CLIENT_ID_APP25,
    AT_CLIENT_ID_APP26,
    AT_CLIENT_ID_APP27,
    AT_CLIENT_ID_APP28,
    AT_CLIENT_ID_APP29,
    AT_CLIENT_ID_APP30,
    AT_CLIENT_ID_APP31,
    AT_CLIENT_ID_APP32,
    AT_CLIENT_ID_APP33,
    AT_CLIENT_ID_APP34,
    AT_CLIENT_ID_APP35,
    AT_CLIENT_ID_APP36,
    AT_CLIENT_ID_APP37,
    AT_CLIENT_ID_APP38,
    AT_CLIENT_ID_APP39,
    AT_CLIENT_ID_APP40,
    AT_CLIENT_ID_APP41,
    AT_CLIENT_ID_APP42,
    AT_CLIENT_ID_APP43,
    AT_CLIENT_ID_APP44,
    AT_CLIENT_ID_APP45,
    AT_CLIENT_ID_APP46,
    AT_CLIENT_ID_APP47,
    AT_CLIENT_ID_APP48,
    AT_CLIENT_ID_APP49,
    AT_CLIENT_ID_APP50,
    AT_CLIENT_ID_APP51,
    AT_CLIENT_ID_APP52,
#endif

    AT_CLIENT_ID_BUTT
};
typedef VOS_UINT16 AT_CLIENT_ID_ENUM_UINT16;


enum MN_OAM_CLIENT_ID_ENUM
{
    OAM_CLIENT_ID_OM                 = AT_CLIENT_ID_BUTT,
    OAM_CLIENT_ID_SPY,
    OAM_CLIENT_ID_DIAG,
    OAM_CLIENT_ID_STK,
    OAM_CLIENT_ID_PIH,
    OAM_CLIENT_ID_PB,
    OAM_CLIENT_ID_CBT,
    OMA_CLIENT_ID_BUTT
};
typedef VOS_UINT16 MN_OAM_CLIENT_ID_ENUM_UINT16;


enum MN_INTERNAL_CLIENT_ID_ENUM
{
    INTERNAL_CLIENT_ID_SMMA             = OMA_CLIENT_ID_BUTT,
    INTERNAL_CLIENT_ID_SPM,
    INTERNAL_CLIENT_ID_BUTT
};
typedef VOS_UINT16 MN_INTERNAL_CLIENT_ID_ENUM_UINT16;


enum AT_CLIENT_TAB_INDEX_ENUM
{
    AT_CLIENT_TAB_PCUI_INDEX              = 0,
    AT_CLIENT_TAB_CTRL_INDEX,
    AT_CLIENT_TAB_PCUI2_INDEX,
    AT_CLIENT_TAB_MODEM_INDEX,
    AT_CLIENT_TAB_NDIS_INDEX,
    AT_CLIENT_TAB_UART_INDEX,
#if (FEATURE_ON == FEATURE_AT_HSUART)
    AT_CLIENT_TAB_HSUART_INDEX,
#endif
    AT_CLIENT_TAB_SOCK_INDEX,
    AT_CLIENT_TAB_APPSOCK_INDEX,
#if (FEATURE_ON == FEATURE_AT_HSIC)
    AT_CLIENT_TAB_HSIC1_INDEX,
    AT_CLIENT_TAB_HSIC2_INDEX,
    AT_CLIENT_TAB_HSIC3_INDEX,
    AT_CLIENT_TAB_HSIC4_INDEX,
    AT_CLIENT_TAB_HSIC_MODEM_INDEX,
    AT_CLIENT_TAB_MUX1_INDEX,
    AT_CLIENT_TAB_MUX2_INDEX,
    AT_CLIENT_TAB_MUX3_INDEX,
    AT_CLIENT_TAB_MUX4_INDEX,
    AT_CLIENT_TAB_MUX5_INDEX,
    AT_CLIENT_TAB_MUX6_INDEX,
    AT_CLIENT_TAB_MUX7_INDEX,
    AT_CLIENT_TAB_MUX8_INDEX,
#endif
    AT_CLIENT_TAB_APP_INDEX,
    AT_CLIENT_TAB_APP1_INDEX,
#if (FEATURE_ON == FEATURE_VCOM_EXT)
    AT_CLIENT_TAB_APP2_INDEX,
    AT_CLIENT_TAB_APP3_INDEX,
    AT_CLIENT_TAB_APP4_INDEX,
    AT_CLIENT_TAB_APP5_INDEX,
    AT_CLIENT_TAB_APP6_INDEX,
    AT_CLIENT_TAB_APP7_INDEX,
    AT_CLIENT_TAB_APP8_INDEX,
    AT_CLIENT_TAB_APP9_INDEX,
    AT_CLIENT_TAB_APP10_INDEX,
    AT_CLIENT_TAB_APP11_INDEX,
    AT_CLIENT_TAB_APP12_INDEX,
    AT_CLIENT_TAB_APP13_INDEX,
    AT_CLIENT_TAB_APP14_INDEX,
    AT_CLIENT_TAB_APP15_INDEX,
    AT_CLIENT_TAB_APP16_INDEX,
    AT_CLIENT_TAB_APP17_INDEX,
    AT_CLIENT_TAB_APP18_INDEX,
    AT_CLIENT_TAB_APP19_INDEX,
    AT_CLIENT_TAB_APP20_INDEX,
    AT_CLIENT_TAB_APP21_INDEX,
    AT_CLIENT_TAB_APP22_INDEX,
    AT_CLIENT_TAB_APP23_INDEX,
    AT_CLIENT_TAB_APP24_INDEX,
    AT_CLIENT_TAB_APP25_INDEX,
    AT_CLIENT_TAB_APP26_INDEX,

    AT_CLIENT_TAB_APP27_INDEX,
    AT_CLIENT_TAB_APP28_INDEX,
    AT_CLIENT_TAB_APP29_INDEX,
    AT_CLIENT_TAB_APP30_INDEX,
    AT_CLIENT_TAB_APP31_INDEX,
    AT_CLIENT_TAB_APP32_INDEX,
    AT_CLIENT_TAB_APP33_INDEX,
    AT_CLIENT_TAB_APP34_INDEX,
    AT_CLIENT_TAB_APP35_INDEX,
    AT_CLIENT_TAB_APP36_INDEX,
    AT_CLIENT_TAB_APP37_INDEX,
    AT_CLIENT_TAB_APP38_INDEX,
    AT_CLIENT_TAB_APP39_INDEX,
    AT_CLIENT_TAB_APP40_INDEX,
    AT_CLIENT_TAB_APP41_INDEX,
    AT_CLIENT_TAB_APP42_INDEX,
    AT_CLIENT_TAB_APP43_INDEX,
    AT_CLIENT_TAB_APP44_INDEX,
    AT_CLIENT_TAB_APP45_INDEX,
    AT_CLIENT_TAB_APP46_INDEX,
    AT_CLIENT_TAB_APP47_INDEX,
    AT_CLIENT_TAB_APP48_INDEX,
    AT_CLIENT_TAB_APP49_INDEX,
    AT_CLIENT_TAB_APP50_INDEX,
    AT_CLIENT_TAB_APP51_INDEX,
    AT_CLIENT_TAB_APP52_INDEX,
#endif

    AT_CLIENT_BUTT
};
typedef VOS_UINT8 AT_CLIENT_TAB_INDEX_UINT8;



enum MN_CLIENT_ID_TYPE
{
    MN_CLIENT_ID_TYPE_AT,
    MN_CLIENT_ID_TYPE_OAM,
    MN_CLIENT_ID_TYPE_INTERNAL,
    MN_CLIENT_ID_TYPE_BUTT
};
typedef VOS_UINT16 MN_CLIENT_ID_TYPE_UINT16;

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
  10 函数声明
*****************************************************************************/
extern VOS_UINT32 AT_GetModemIdFromClient(
    VOS_UINT16                          usClientId,
    MODEM_ID_ENUM_UINT16               *pModemId
);

extern MN_CLIENT_ID_T AT_GetRealClientId(
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT32                          ulPid
);

extern MN_CLIENT_ID_T MN_GetRealClientId(
    MN_CLIENT_ID_T                      usClientId,
    VOS_UINT32                          ulPid
);


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

#endif /* end of MnClient.h */
