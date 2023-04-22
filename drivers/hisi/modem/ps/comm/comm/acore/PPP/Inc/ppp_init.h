/*
 *
 * All rights reserved.
 *
 * This software is available to you under a choice of one of two
 * licenses. You may choose this file to be licensed under the terms
 * of the GNU General Public License (GPL) Version 2 or the 2-clause
 * BSD license listed below:
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 */

#ifndef __PPP_INIT_H__
#define __PPP_INIT_H__

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "PPP/Inc/ppp_public.h"
#include "LinuxInterface.h"

#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/


/*****************************************************************************
  3 枚举定义
*****************************************************************************/

/*****************************************************************************
  4 STRUCT定义
*****************************************************************************/
typedef struct
{
    VOS_UINT32                  ulUplinkCnt;                /* 上行数据包总个数 */
    VOS_UINT32                  ulUplinkDropCnt;            /* 上行丢包数 */
    VOS_UINT32                  ulUplinkSndDataCnt;         /* 上行发包数 */

    VOS_UINT32                  ulDownlinkCnt;              /* 下行数据包总个数 */
    VOS_UINT32                  ulDownlinkDropCnt;          /* 下行丢包数 */
    VOS_UINT32                  ulDownlinkSndDataCnt;       /* 下行发包数 */

    VOS_UINT32                  ulMemAllocDownlinkCnt;      /* 下行内存申请次数 */
    VOS_UINT32                  ulMemAllocDownlinkFailCnt;  /* 下行内存申请失败次数 */
    VOS_UINT32                  ulMemAllocUplinkCnt;        /* 上行内存申请次数 */
    VOS_UINT32                  ulMemAllocUplinkFailCnt;    /* 上行内存申请失败次数 */
    VOS_UINT32                  ulMemFreeCnt;               /* 其他内存释放次数 */

    VOS_UINT32                  ulDropCnt;                  /* 队列满丢包数 */

    VOS_UINT32                  ulQMaxCnt;                  /* 队列中出现过的最大结点个数 */
    VOS_UINT32                  ulSndMsgCnt;                /* DataNotify消息发送数 */
    VOS_UINT32                  ulProcMsgCnt;               /* DataNotify消息处理数 */
} PPP_DATA_Q_STAT_ST;

typedef struct
{
    PPP_ZC_QUEUE_STRU           stDataQ;                    /* PPP数据队列，上下行数据都在其中 */
    PPP_DATA_Q_STAT_ST          stStat;                     /* PPP数据队列的统计信息 */
    volatile VOS_UINT32         ulNotifyMsg;                /* 通知PPP处理数据 */
}PPP_DATA_Q_CTRL_ST;

typedef struct
{
    struct cpumask              orig_mask;
    struct cpumask              curr_mask;

    VOS_UINT32                  ulPppTaskId;
    VOS_UINT32                  ulPppInitFlag;

    TTF_BOOL_ENUM_UINT8         enChapEnable;           /* 是否使能Chap鉴权 */
    TTF_BOOL_ENUM_UINT8         enPapEnable;            /* 是否使能Pap鉴权 */
    VOS_UINT16                  usLcpEchoMaxLostCnt;    /* 发送LcpEchoRequest允许丢弃的最大个数 */

    VOS_UINT16                  usQueneMaxCnt;          /* 队列最大允许个数 */
    VOS_UINT8                   aucRsv[2];
} PPP_ENTITY_INFO_STRU;

/*****************************************************************************
  5 全局变量声明
*****************************************************************************/
extern PPP_ENTITY_INFO_STRU             g_stPppEntInfo;
extern PPP_DATA_Q_CTRL_ST               g_PppDataQCtrl;


#pragma pack()

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif

#endif  /*end of __PPP_INIT_H__*/

