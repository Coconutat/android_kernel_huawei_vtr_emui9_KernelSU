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
#ifndef _TAF_API_H_
#define _TAF_API_H_


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "vos.h"
#include "TafTypeDef.h"
#include "PsTypeDef.h"
#include "PsCommonDef.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)


/*****************************************************************************
  2 宏定义
*****************************************************************************/
typedef VOS_UINT8 MN_CALLBACK_TYPE_T;   /* Type of callback functions for event reporting*/
#define MN_CALLBACK_CS_CALL             0 /* CS call */
#define MN_CALLBACK_PS_CALL             1 /* PS call */
#define MN_CALLBACK_MSG                 2 /* Short message service */
#define MN_CALLBACK_SS                  3 /* Call independent supplemental service */
#define MN_CALLBACK_PHONE               4 /* Phone management and network register service */
#define MN_CALLBACK_DATA_IND            5 /* User plane data indication */
#define MN_CALLBACK_DATA_STATUS         6 /* User plane data status */
#define MN_CALLBACK_DATA_FLOW           7 /* User plane data flow */
#define MN_CALLBACK_SET                 8
#define MN_CALLBACK_QRY                 9
#define MN_CALLBACK_PHONE_BOOK          10 /* Phone book */
#define MN_CALLBACK_STK                 11
#define MN_CALLBACK_CMD_CNF             12 /* Cmd Cnf*/
#define MN_CALLBACK_CHANNEL_STATUS      13
#define MN_CALLBACK_PIH                 14
#define MN_CALLBACK_VOICE_CONTROL       15
#define MN_CALLBACK_LOG_PRINT           16
#define MN_CALLBACK_MAX                 17



#define  TAF_MAX_CLIENT_OF_ONE_PROC       OMA_CLIENT_ID_BUTT

#define TAF_FREE                        0   /*未使用*/
#define TAF_USED                        1   /*使用*/



#define TAF_CALL_APP_MSG_BASE           (0x0000)                                /* AT与CALL模块间消息起始 */
#define TAF_MSG_APP_MSG_BASE            (0x1000)                                /* AT与MSG模块间消息起始 */
#define TAF_SSA_APP_MSG_BASE            (0x2000)                                /* AT与SSA模块间消息起始 */
#define TAF_MMA_APP_MSG_BASE            (0x3000)                                /* AT与MMA模块间消息起始 */
#define TAF_APP_SET_MSG_BASE            (0x5000)                                /* AT与MN模块间设置消息起始 */
#define TAF_APP_QRY_MSG_BASE            (0x6000)                                /* AT与MN模块间查询消息起始 */
#define TAF_APS_MSG_BASE                (0x7000)                                /* AT/IMSA与PS适配模块间消息起始 */
#define TAF_SPM_IMSA_MSG_BASE           (0x8000)                                /* IMSA与SPM模块间消息起始 */
#define TAF_MSG_IMSA_MSG_BASE           (0x9000)                                /* IMSA与MSG模块间消息起始 */
#define TAF_MMA_IMSA_MSG_BASE           (0xa000)                                /* IMSA与MMA模块间消息起始 */
#define TAF_CALL_IMSA_MSG_BASE          (0xb000)                                /* CALL与IMSA公共消息起始 */
#define TAF_IMSA_COMM_MSG_BASE          (0xc000)                                /* TAF与IMSA公共消息起始 */
#define TAF_SSA_IMSA_MSG_BASE           (0xd000)                                /* IMSA与SSA模块间消息起始 */
#define TAF_IMSA_MSG_MASK               (0xf000)                                /* IMSA与TAF间的MASK */




/* 控制头填写 */
#define TAF_API_CTRL_HEADER(pstCtrlHeader, ModuleId, ClientId, OpId) \
                            ((TAF_CTRL_STRU *)(pstCtrlHeader))->ulModuleId = ModuleId; \
                            ((TAF_CTRL_STRU *)(pstCtrlHeader))->usClientId = ClientId;\
                            ((TAF_CTRL_STRU *)(pstCtrlHeader))->ucOpId     = OpId

/*****************************************************************************
  3 枚举定义
*****************************************************************************/

enum TAF_LMM_MULTIMODE_CARD_TYPE_ENUM
{
    TAF_LMM_CTCC_CDMA_SINGLE_MODE_CARD,                         /* 中国电信CDMA单模卡 */
    TAF_LMM_CTCC_CDMA_DUAL_MODE_CARD,                           /* 中国电信CDMA双模卡 */
    TAF_LMM_UNKONWN_CDMA_SINGLE_MODE_CARD,                      /* 未知CDMA单模卡 */
    TAF_LMM_UNKONWN_CDMA_DUAL_MODE_CARD,                        /* 未知CDMA双模卡 */
    TAF_LMM_SINGLE_MODE_USIM_CARD,                              /* (U)SIM单模卡 */
    TAF_LMM_NO_CARD,                                            /* 无卡 */
    TAF_LMM_MULTIMODE_CARD_TYPE_BUTT
};
typedef VOS_UINT8 TAF_LMM_MULTIMODE_CARD_TYPE_ENUM_UINT8;


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

typedef struct
{
    VOS_UINT32                          ulModuleId;         /* 填入PID */
    VOS_UINT16                          usClientId;
    VOS_UINT8                           ucOpId;
    VOS_UINT8                           aucReserved[1];
} TAF_CTRL_STRU;


typedef struct
{
    MSG_HEADER_STRU                     stHeader;
    VOS_UINT8                           aucContent[4];
} TAF_PS_MSG_STRU;


typedef TAF_PS_MSG_STRU     TAF_SSA_MSG_STRU;


/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/



/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern TAF_LMM_MULTIMODE_CARD_TYPE_ENUM_UINT8 TAF_LMM_GetCtccRoamCtrlMultiModeCardType(VOS_VOID);



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

#endif

