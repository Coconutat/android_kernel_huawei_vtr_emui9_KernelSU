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

 

#ifndef __OM_HDLC_H__
#define __OM_HDLC_H__

/******************************************************************************
  1 其他头文件包含
******************************************************************************/
#include "vos.h"

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#pragma pack(4)
/******************************************************************************
  2 宏定义
******************************************************************************/
#define OM_HDLC_FRAME_FLAG  0x7e        /* HDLC帧标志位，固定为0x7e，区分不同的HDLC帧 */
#define OM_HDLC_ESC         0x7d        /* 转义字符，紧接其后的字符需要转义 */
#define OM_HDLC_ESC_MASK    0x20        /* 转义字符掩字 */

#define OM_HDLC_INIT_FCS    0xffff      /* 计算FCS的初始FCS值 */
#define OM_HDLC_GOOD_FCS    0xf0b8      /* 计算结果为该FCS值时，说明FCS校验正确 */

#define OM_HDLC_MODE_HUNT   0x1         /* 解封装模式中bit0为1，表明未找到帧标志，没有开始解封装 */
#define OM_HDLC_MODE_ESC    0x2         /* 解封装模式中bit1为1，表明上个字符为转义字符，当前字符为真正数据的掩字 */

#define OM_HDLC_FCS_LEN     2           /* HDLC帧中FCS未转义时的长度(单位: Byte) */


/******************************************************************************
  3 枚举定义
******************************************************************************/
enum OM_HDLC_RESULT_ENUM
{
    HDLC_SUCC           = 0,                /* 0    HDLC处理成功           */
    HDLC_NOT_HDLC_FRAME = 1,                /* 1    没有完整的帧             */
    HDLC_FCS_ERROR,                         /* 2    发生了校验错误        */
    HDLC_FRAME_DISCARD,                     /* 3    发生异常，丢弃当前HDLC帧 */
    HDLC_BUFF_FULL,                         /* 4    目的缓存满           */
    HDLC_PARA_ERROR,                        /* 5    参数检查异常           */
    HDLC_BUTT
};
typedef VOS_UINT32 OM_HDLC_RESULT_ENUM_UINT32;


/******************************************************************************
  4 全局变量声明
******************************************************************************/

/******************************************************************************
  5 消息头定义
******************************************************************************/


/******************************************************************************
  6 消息定义
******************************************************************************/


/******************************************************************************
  7 STRUCT定义
******************************************************************************/


typedef struct
{
    VOS_UINT32  ulMode;             /* HDLC解析使用的内部参数，HDLC负责维护 */
    VOS_UINT32  ulLength;           /* HDLC解析时内部维护的中间变量 */
    VOS_UINT32  ulDecapBuffSize;    /* HDLC解析内容存放BUFFER的长度，调用者负责指定 */
    VOS_UINT8  *pucDecapBuff;       /* HDLC解析内容存放地址，由调用者负责指定空间 */
    VOS_UINT32  ulInfoLen;          /* HDLC解析成功的数据长度，HDLC负责维护 */
} OM_HDLC_STRU;


/******************************************************************************
  8 UNION定义
******************************************************************************/


/******************************************************************************
  9 函数声明
******************************************************************************/
extern VOS_UINT32 Om_HdlcEncap(
                const VOS_UINT8  *pucSrc,
                VOS_UINT16  usSrcLen,
                VOS_UINT8  *pucDest,
                VOS_UINT16  usDestBuffLen,
                VOS_UINT16 *pusDestLen );

extern VOS_VOID Om_HdlcInit( OM_HDLC_STRU *pstHdlc );

extern OM_HDLC_RESULT_ENUM_UINT32 Om_HdlcDecap( OM_HDLC_STRU *pstHdlc, VOS_UINT8 ucChar );



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

#endif /* OmHdlcInterface.h */




