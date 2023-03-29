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

#ifndef __CSDDEBUG_H__
#define __CSDDEBUG_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include  "vos.h"
/* Added by l60609 for AP适配项目 ，2012-08-30 Begin */
#include "product_config.h"
/* Added by l60609 for AP适配项目 ，2012-08-30 End */

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#if( FEATURE_ON == FEATURE_CSD )

/*****************************************************************************
  3 枚举定义
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
    /*CSD上行统计信息*/
    VOS_UINT32                          ulULRecvPktNum;                         /*CSD上行收到的包数目*/
    VOS_UINT32                          ulULSaveBuffPktNum;                     /*CSD上行缓存包的数目*/
    VOS_UINT32                          ulULEnQueFail;                          /*CSD入队失败的次数*/
    VOS_UINT32                          ulULSendPktNum;                         /*CSD发送上行缓存包的数目*/
    VOS_UINT32                          ulULQueNullNum;                         /*CSD发送上行数据时从队列中获取到空指针包数目*/
    VOS_UINT32                          ulULZcToImmFailNum;                     /*CSD发送上行数据sk_buffer头转换到IMM头失败的包数目*/
    VOS_UINT32                          ulULInsertDiccFailNum;                  /*CSD发送上行数据插入DICC通道失败的包数目*/

    /*CSD下行统计信息*/
    VOS_UINT32                          ulDLRecvPktNum;                         /*CSD下行收到的包数目*/
    VOS_UINT32                          ulDLSendPktNum;                         /*CSD下行发送到驱动的包数目*/
    VOS_UINT32                          ulDLSendFailNum;                        /*CSD下行发送失败包的数目*/

}CSD_UL_STATUS_INFO_STRU;

/*****************************************************************************
  8 全局变量声明
*****************************************************************************/
extern CSD_UL_STATUS_INFO_STRU          g_stCsdStatusInfo;

/*****************************************************************************
  10 宏定义
*****************************************************************************/

/*CSD上行统计信息*/
#define CSD_DBG_UL_RECV_PKT_NUM(n)      (g_stCsdStatusInfo.ulULRecvPktNum       += (n))
#define CSD_DBG_UL_SAVE_BUFF_PKT_NUM(n) (g_stCsdStatusInfo.ulULSaveBuffPktNum   += (n))
#define CSD_DBG_UL_ENQUE_FAIL_NUM(n)    (g_stCsdStatusInfo.ulULEnQueFail        += (n))
#define CSD_DBG_UL_SEND_PKT_NUM(n)      (g_stCsdStatusInfo.ulULSendPktNum       += (n))
#define CSD_DBG_UL_QUENULL_NUM(n)       (g_stCsdStatusInfo.ulULQueNullNum       += (n))
#define CSD_DBG_UL_ZCTOIMM_FAIL_NUM(n)  (g_stCsdStatusInfo.ulULZcToImmFailNum   += (n))
#define CSD_DBG_UL_INSERT_FAIL_NUM(n)   (g_stCsdStatusInfo.ulULInsertDiccFailNum += (n))

/*CSD下行统计信息*/
#define CSD_DBG_DL_RECV_PKT_NUM(n)      (g_stCsdStatusInfo.ulDLRecvPktNum       += (n))
#define CSD_DBG_DL_SEND_PKT_NUM(n)      (g_stCsdStatusInfo.ulDLSendPktNum       += (n))
#define CSD_DBG_DL_SEND_FAIL_NUM(n)     (g_stCsdStatusInfo.ulDLSendFailNum      += (n))



#endif

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

#endif /* end of CsdDebug.h */
