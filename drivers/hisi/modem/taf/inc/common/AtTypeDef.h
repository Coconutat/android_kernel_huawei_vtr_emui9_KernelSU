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

#ifndef __ATTPYEDEF_H__
#define __ATTPYEDEF_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#ifndef __AT_DISABLE_OM__
#define AT_INFO_LOG(String)                     TAF_LOG(WUEPS_PID_AT, 0, PS_LOG_LEVEL_INFO, String)
#define AT_NORM_LOG(String)                     TAF_LOG(WUEPS_PID_AT, 0, PS_LOG_LEVEL_NORMAL, String)
#define AT_NORM_LOG1(String,Para1)              TAF_LOG1(WUEPS_PID_AT, 0, PS_LOG_LEVEL_NORMAL, String, (TAF_INT32)Para1)
#define AT_WARN_LOG(String)                     TAF_LOG(WUEPS_PID_AT, 0, PS_LOG_LEVEL_WARNING, String)
#define AT_WARN_LOG1(String,Para1)              TAF_LOG1(WUEPS_PID_AT, 0, PS_LOG_LEVEL_WARNING, String, (TAF_INT32)Para1)
#define AT_WARN_LOG2(String,Para1, Para2)       TAF_LOG2(WUEPS_PID_AT, 0, PS_LOG_LEVEL_WARNING, String, (TAF_INT32)Para1, (TAF_INT32)Para2)
#define AT_ERR_LOG(String)                      TAF_LOG(WUEPS_PID_AT, 0, PS_LOG_LEVEL_ERROR, String)
#define AT_ERR_LOG1(String, Para1)              TAF_LOG1(WUEPS_PID_AT, 0, PS_LOG_LEVEL_ERROR, String, (VOS_INT32)Para1)
#define AT_LOG1(String, Para1)                  TAF_LOG1(WUEPS_PID_AT, 0, PS_LOG_LEVEL_INFO, String, (TAF_INT32)Para1)
#else
#define AT_INFO_LOG(String)
#define AT_NORM_LOG(String)
#define AT_WARN_LOG(String)
#define AT_ERR_LOG(String)
#define AT_LOG1(String, Para1)
#endif

/*****************************************************************************
  3 枚举定义
*****************************************************************************/
/*================================USER TYPE Begin================================*/

enum AT_USER_TYPE_ENUM
{
    AT_USBCOM_USER                      = 0,
    AT_PCUI2_USER                       = 2,
    AT_COM_USER                         = 3,
    AT_MODEM_USER                       = 4,
    AT_IPCCOM_USER                      = 6,
    AT_NDIS_USER                        = 7,
    AT_CTR_USER                         = 8,
    AT_APP_USER                         = 9,
    AT_SOCK_USER                        = 10,
    AT_APP_SOCK_USER                    = 11,
    AT_UART_USER                        = 12,
    AT_HSIC1_USER                       = 13,
    AT_HSIC2_USER                       = 14,
    AT_HSIC3_USER                       = 15,

    /* Added by L47619 for V7R1C50 A-GPS Project, 2012/06/28, begin */
    AT_HSIC4_USER                       = 16,
    /* Added by L47619 for V7R1C50 A-GPS Project, 2012/06/28, end */

    /* Added by L60609 for MUX，2012-08-03,  Begin */
    AT_MUX1_USER                        = 17,
    AT_MUX2_USER                        = 18,
    AT_MUX3_USER                        = 19,
    AT_MUX4_USER                        = 20,
    AT_MUX5_USER                        = 21,
    AT_MUX6_USER                        = 22,
    AT_MUX7_USER                        = 23,
    AT_MUX8_USER                        = 24,
    /* Added by L60609 for MUX，2012-08-03,  End */

    AT_HSUART_USER                      = 25,

    AT_BUTT_USER                        = 0xff
};
typedef VOS_UINT8 AT_USER_TYPE;

/*================================USER TYPE End================================*/

typedef VOS_UINT8 AT_MODE_TYPE;
#define AT_CMD_MODE                     (0)
#define AT_DATA_MODE                    (1)
#define AT_ONLINE_CMD_MODE              (2)

typedef VOS_UINT8 AT_IND_MODE_TYPE;
#define AT_IND_MODE                     (0)
#define AT_NO_IND_MODE                  (1)

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

#endif /* end of AtTypeDef.h */
