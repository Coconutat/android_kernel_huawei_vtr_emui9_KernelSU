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



#ifndef __HDLC_INTERFACE_H__
#define __HDLC_INTERFACE_H__

/*****************************************************************************
  1 其他头文件包含
*****************************************************************************/
#include "product_config.h"
#include "vos.h"

#include "ImmInterface.h"
#include "PPP/Inc/throughput.h"
#include "PPP/Inc/ppp_mbuf.h"
#include "PPP/Inc/lcp.h"
#include "PPP/Inc/ipcp.h"
#include "PPP/Inc/async.h"
#include "PPP/Inc/chap.h"
#include "PPP/Inc/pap.h"
#include "PPP/Inc/layer.h"
#include "PPP/Inc/proto.h"

#if (FEATURE_ON == FEATURE_HARDWARE_HDLC_FUNC)
#include "hdlc_hardware_service.h"
#endif

#ifdef __cplusplus
    #if __cplusplus
        extern "C" {
    #endif
#endif

#pragma pack(4)

/*****************************************************************************
  2 宏定义
*****************************************************************************/
/* 定义PPP_ST_TEST时，把PppStTest.c编进来，在单板上运行ST用例，正式版本不定义此宏 */
/* #define PPP_ST_TEST */

#define NPROTOSTAT                  (13)

extern struct link*                 pgPppLink;

#define PPP_LINK(PppId)             ((pgPppLink + PppId) - 1)
#define PPP_LINK_TO_ID(pLink)       ((pLink - pgPppLink) + 1)

#define LINK_QUEUES(link) (sizeof (link)->Queue / sizeof (link)->Queue[0])
#define LINK_HIGHQ(link) ((link)->Queue + LINK_QUEUES(link) - 1)

#define PPP_HDLC_PROC_AS_FRM_PACKET_IND         (100)

#define PPP_ONCE_DEAL_MAX_CNT       (200)

/*******************************************************************************
  3 枚举定义
*******************************************************************************/
/*****************************************************************************
 软、硬件HDLC处理返回结果
*****************************************************************************/
enum PPP_HDLC_RESULT_TYPE_ENUM
{
    PPP_HDLC_RESULT_COMM_FINISH           = 0,      /* 本次处理正常完成 */
    PPP_HDLC_RESULT_COMM_CONTINUE         = 1,      /* 本次处理正常，但还有数据在队列中待下次继续处理，用于限制单次最大处理个数 */
    PPP_HDLC_RESULT_COMM_ERROR            = 2,      /* 本次处理出错 */

    PPP_HDLC_RESULT_BUTT
};
typedef VOS_UINT32   PPP_HDLC_RESULT_TYPE_ENUM_UINT32;

/*****************************************************************************
 PPP数据类型
*****************************************************************************/
enum PPP_DATA_TYPE_ENUM
{
    PPP_PULL_PACKET_TYPE = 1,                       /* IP类型上行数据 */
    PPP_PUSH_PACKET_TYPE,                           /* IP类型下行数据 */
    PPP_PULL_RAW_DATA_TYPE,                         /* PPP类型上行数据 */
    PPP_PUSH_RAW_DATA_TYPE                          /* PPP类型下行数据 */
};
typedef VOS_UINT8   PPP_DATA_TYPE_ENUM_UINT8;

/*****************************************************************************
  4 全局变量声明
*****************************************************************************/


/*****************************************************************************
  5 消息头和消息类型定义
*****************************************************************************/


/*****************************************************************************
  7 STRUCT定义
*****************************************************************************/
/*****************************************************************************
 软、硬件HDLC处理函数原型，在创建PPP实体的时候，根据需要可以灵活地将
 软件或硬件的实现与该实体关联
*****************************************************************************/
typedef PPP_HDLC_RESULT_TYPE_ENUM_UINT32 (* PPP_HDLC_PROC_DATA_FUNC )
(
    PPP_ID usPppId,
    struct link *pstLink,
    PPP_ZC_QUEUE_STRU *pstDataQ
);

/*****************************************************************************
 处理PPP协议栈输出的协议包，需要封装后发给PC，主要在PPP协商期间调用
*****************************************************************************/
typedef VOS_VOID (* PPP_HDLC_PROC_PROTOCOL_PACKET_FUNC )
(
    struct link *pstLink,
    struct ppp_mbuf *pstMbuf,
    VOS_INT32 ulPri,
    VOS_UINT16 usProtocol
);

/*****************************************************************************
 处理PPP协议栈输出的协议包，进行封装的处理函数，主要在PPP协商期间调用
*****************************************************************************/
typedef VOS_VOID (* PPP_HDLC_PROC_AS_FRM_PACKET_FUNC )
(
    VOS_UINT16       usPppId,
    VOS_UINT16       usProtocol,
    PPP_ZC_STRU     *pstMem
);

/*****************************************************************************
 去使能HDLC函数原型，硬件HDLC需要实现此接口，软件不需要
*****************************************************************************/
typedef VOS_VOID (* PPP_HDLC_DISABLE_FUNC )(VOS_VOID);

struct link {
  VOS_INT32 type;                           /* _LINK type */
  VOS_INT32 len;                            /* full size of parent struct */
  const VOS_CHAR *name;                     /* Points to datalink::name */
  struct {
    unsigned gather : 1;                    /* Gather statistics ourself ? */
    struct pppThroughput total;             /* Link throughput statistics */
    struct pppThroughput *parent;           /* MP link throughput statistics */
  } stats;

  struct ppp_mqueue Queue[2];               /* Our output queue of mbufs */

  VOS_UINT32 proto_in[NPROTOSTAT];          /* outgoing protocol stats */
  VOS_UINT32 proto_out[NPROTOSTAT];         /* incoming protocol stats */

  struct lcp lcp;                           /* Our line control FSM */

  VOS_UINT32 phase;                         /* Curent phase */
  VOS_INT32  timer_state;

  struct ipcp ipcp;

  struct {
        VOS_CHAR name[AUTHLEN];        /* PAP/CHAP system name */
        VOS_CHAR key[AUTHLEN];         /* PAP/CHAP key */
      } auth;

  struct pap  pap;
  struct chap chap;                         /* Authentication using chap */

  struct async async;
  struct hdlc hdlc;

  struct layer const *layer[LAYER_MAX];     /* i/o layers */

  VOS_INT32  nlayers;

  VOS_UINT32 DropedPacketFromGgsn;
};

/*****************************************************************************
 软硬件HDLC配置结构体
*****************************************************************************/
typedef struct _PPP_HDLC_CONFIG_STRU
{
    PPP_HDLC_PROC_DATA_FUNC                 pFunProcData;           /* 指向软件或硬件处理队列数据函数指针 */
    PPP_HDLC_PROC_PROTOCOL_PACKET_FUNC      pFunProcProtocolPacket; /* 指向软件或硬件处理协议栈输出数据函数指针 */
    PPP_HDLC_DISABLE_FUNC                   pFunDisable;            /* 指向软件或硬件功能去使能函数指针 */
    PPP_HDLC_PROC_AS_FRM_PACKET_FUNC        pFunProcAsFrmData;      /* 指向软件或硬件功能以封装方式处理函数指针 */
}PPP_HDLC_CONFIG_STRU;
extern PPP_HDLC_CONFIG_STRU        *g_pstHdlcConfig;

/*****************************************************************************
 协议栈输出协商包消息结构体
*****************************************************************************/
typedef struct
{
    MSG_HEADER_STRU                     MsgHeader;
    VOS_UINT16                          usPppId;
    VOS_UINT16                          usProtocol;
    VOS_UINT8                           aucReserve[4];
    PPP_ZC_STRU                        *pstMem;
}HDLC_PROC_AS_FRM_PACKET_IND_MSG_STRU;

#if (FEATURE_ON == FEATURE_HARDWARE_HDLC_FUNC)
/* 一个页表最小长度 */
#define     PPP_HDLC_MEM_MAP_SIZE                               (4096)

/* 构造参数链表结果信息 */
typedef struct _HDLC_PARA_LINK_BUILD_PARA_STRU
{
    PPP_DATA_TYPE_ENUM_UINT8            ucDataType;             /* 数据类型 */
    PPP_SERVICE_HDLC_HARD_MODE_ENUM_UINT8       enPppMode;      /* PPP工作模式 */
    VOS_UINT8                           aucReserve1[2];
    VOS_UINT16                          usPppId;                /* PPP ID */
    VOS_UINT16                          usProtocol;             /* 协议类型，封装的时候使用 */
    PPP_ZC_QUEUE_STRU                  *pstDataQ;               /* 数据队列 */
    struct link                        *pstLink;                /* PPP链路信息 */
} HDLC_PARA_LINK_BUILD_PARA_STRU;

/* 构造参数链表结果信息 */
typedef struct _HDLC_PARA_LINK_BUILD_INFO_STRU
{
    VOS_UINT32                          ulDealCnt;              /* 本次构造链表过程从数据队列中总共处理的数据包个数 */
    VOS_UINT32                          ulInputLinkNodeCnt;     /* 输入参数链表节点个数 */
    PPP_ZC_STRU *                       apstInputLinkNode[HDLC_INPUT_PARA_LINK_MAX_SIZE];   /* 输入参数链表节点对应的零拷贝内存 */
    VOS_UINT32                          ulOutputLinkNodeCnt;                                /* 输出参数链表节点个数，只在封装有效 */
    PPP_ZC_STRU *                       apstOutputLinkNode[HDLC_OUTPUT_PARA_LINK_MAX_SIZE]; /* 输出参数链表节点对应的零拷贝内存，只在封装有效 */
} HDLC_PARA_LINK_BUILD_INFO_STRU;
#endif

/*****************************************************************************
  8 UNION定义
*****************************************************************************/


/*****************************************************************************
  9 OTHERS定义
*****************************************************************************/

/*****************************************************************************
  10 函数声明
*****************************************************************************/


VOS_VOID PPP_HDLC_ProcIpModeUlData
(
    struct link *pstLink,
    PPP_ZC_STRU *pstMem,
    VOS_UINT16  usProto
);


VOS_VOID PPP_HDLC_ProcPppModeUlData
(
    PPP_ID      usPppId,
    PPP_ZC_STRU *pstMem
);



VOS_VOID PPP_HDLC_ProcDlData(VOS_UINT16 usPppId, PPP_ZC_STRU *pstMem);

#if (FEATURE_ON == FEATURE_HARDWARE_HDLC_FUNC)

extern HTIMER                                      g_hHdlcFrmTimerHandle;

extern VOS_UINT32 PPP_HDLC_HARD_Init(VOS_VOID);
extern PPP_HDLC_RESULT_TYPE_ENUM_UINT32 PPP_HDLC_HARD_ProcData
(
    PPP_ID              usPppId,
    struct link        *pstLink,
    PPP_ZC_QUEUE_STRU  *pstDataQ
);
extern VOS_VOID PPP_HDLC_HARD_ProcAsFrmPacket
(
    VOS_UINT16       usPppId,
    VOS_UINT16       usProtocol,
    PPP_ZC_STRU     *pstMem
);
extern VOS_VOID PPP_HDLC_HARD_ProcProtocolPacket
(
    struct link     *pstLink,
    struct ppp_mbuf *pstMbuf,
    VOS_INT32        ulPri,
    VOS_UINT16       usProtocol
);
#endif


#pragma pack()

#ifdef __cplusplus
    #if __cplusplus
        }
    #endif
#endif


#endif /* end of hdlc_interface.h */

