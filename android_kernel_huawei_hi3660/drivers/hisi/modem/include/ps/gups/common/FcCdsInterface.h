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

#ifndef __FCCDSINTERFACE_H__
#define __FCCDSINTERFACE_H__


/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include  "vos.h"
#include  "OmApi.h"


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)

/*****************************************************************************
  1 消息头定义
*****************************************************************************/
enum CDS_FC_MSG_TYPE_ENUM
{
    ID_CDS_FC_START_CHANNEL_IND       = 0x0001,     /* _H2ASN_MsgChoice CDS_FC_START_CHANNEL_IND_STRU */
    ID_CDS_FC_STOP_CHANNEL_IND        = 0x0002,     /* _H2ASN_MsgChoice CDS_FC_STOP_CHANNEL_IND_STRU */
    ID_FC_CDS_DL_THRES_CHG_IND        = 0x0003,     /* _H2ASN_MsgChoice FC_CDS_THRES_CHG_IND_STRU */
    ID_CDS_FC_MSG_TYPE_BUTT           = 0xFFFF
};
typedef VOS_UINT32 CDS_FC_MSG_TYPE_ENUM_UINT32;


/*****************************************************************************
  2 宏定义
*****************************************************************************/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/


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
    VOS_MSG_HEADER                                          /* _H2ASN_Skip */
    CDS_FC_MSG_TYPE_ENUM_UINT32         enMsgName;          /* _H2ASN_Skip */
    MODEM_ID_ENUM_UINT16                enModemId;          /* _H2ASN_Replace VOS_UINT16  enModemId; */
    VOS_UINT8                           ucRabId;
    VOS_UINT8                           aucReserved[1];
} CDS_FC_START_CHANNEL_IND_STRU;

typedef struct
{
    VOS_MSG_HEADER                                          /* _H2ASN_Skip */
    CDS_FC_MSG_TYPE_ENUM_UINT32         enMsgName;          /* _H2ASN_Skip */
    MODEM_ID_ENUM_UINT16                enModemId;          /* _H2ASN_Replace VOS_UINT16  enModemId; */
    VOS_UINT8                           ucRabId;
    VOS_UINT8                           aucReserved[1];
} CDS_FC_STOP_CHANNEL_IND_STRU;

typedef struct
{
    VOS_MSG_HEADER                                          /* _H2ASN_Skip */
    CDS_FC_MSG_TYPE_ENUM_UINT32         enMsgName;          /* _H2ASN_Skip */
    VOS_UINT8                           ucThres;
    VOS_UINT8                           aucReserved[3];
}FC_CDS_THRES_CHG_IND_STRU;
/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/
/*****************************************************************************
  H2ASN顶级消息结构定义
*****************************************************************************/
typedef struct
{
    CDS_FC_MSG_TYPE_ENUM_UINT32         enMsgID;    /*_H2ASN_MsgChoice_Export CDS_FC_MSG_TYPE_ENUM_UINT32*/

    VOS_UINT8                           aucMsgBlock[4];
    /***************************************************************************
        _H2ASN_MsgChoice_When_Comment          CDS_FC_MSG_TYPE_ENUM_UINT32
    ****************************************************************************/
}FC_CDS_MSG_DATA;
/*_H2ASN_Length UINT32*/

typedef struct
{
    VOS_MSG_HEADER
    FC_CDS_MSG_DATA                     stMsgData;
}FcCdsInterface_MSG;


/*****************************************************************************
  10 函数声明
*****************************************************************************/
extern SPY_DATA_DOWNGRADE_RESULT_ENUM_UINT32 FC_DownUlGradeProcess(VOS_VOID);
extern SPY_DATA_UPGRADE_RESULT_ENUM_UINT32 FC_UpUlGradeProcess(VOS_VOID);
extern VOS_VOID FC_RecoverUlGradeProcess(VOS_VOID);
extern unsigned int FC_CPU_Process( unsigned int ulCpuLoad );


#pragma pack()


#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif /* end of FcCdsInterface.h */

