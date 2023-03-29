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

#ifndef __BSP_HDS_SERVICE_H__
#define __BSP_HDS_SERVICE_H__

#include <osl_types.h>
#include "bsp_trace.h"
#include "bsp_diag_frame.h"
#include "mdrv_hds_common.h"

/*错误码定义*/
#define HDS_CMD_ERROR          (0x00abcd00 + 0x1)  /*cmdid未注册错误码*/
#define HDS_PRINT_SW_ERR       (0x00abcd00 + 0x2)  /*工具未连接或print开关关闭或打印级别错误*/
#define HDS_PRINT_RE_SUCC      (0x00abcd00 + 0x3)  /*print数据上报成功标志*/
#define HDS_TRANS_SW_ERR       (0x00abcd00 + 0x4)  /*工具未连接或trans开关关闭错误码*/
#define HDS_TRANS_RE_SUCC      (0x00abcd00 + 0x5)  /*trans数据上报成功标志*/
#define HDS_TRANS_CFG_OFF      (0x00abcd00 + 0x6)  /*trans数据上报功能关闭*/
#define HDS_PRINT_RECURSION    (0x00abcd00 + 0x7)
#define BSP_HDS_CMD_NUM_MAX    (50)
#define HDS_GET_MSG_DATA_CFG(pstReq, pData, pReqData) \
do {    \
    pData = (diag_frame_head_stru *)pstReq;  \
    pReqData = (MSP_DIAG_DATA_REQ_STRU *)((void*)(pData->aucData));  \
}while(0)



typedef struct
{
    struct list_head    list;
    u32                 ulCmdId;
    bsp_hds_func       pFunc;
    u32                 ulReserve;
}bsp_hds_cmd_stru;

typedef struct
{
    u32         ulSSId;         /* 数据产生的CPU ID */
    u32         ulMsgType;      /* 所属组件 */
    u32         ulMode;         /* 模式 */
    u32         ulSubType;      /* 子类型，DIAG_MSG_SUB_TYPE_U32 */
    u32         ulDirection;    /* 上报消息的方向 */
    u32         ulModemid;
    u32         ulMsgId;        /* 低16位有效 */
    u32         ulTransId;      /* TransId */
}hds_cnf_stru;

extern void bsp_hds_log_init(void);
void bsp_hds_cmd_register(u32 cmdid, bsp_hds_func fn);
void bsp_hds_get_cmdlist(u32 *cmdlist, u32 *num);
int  bsp_hds_msg_proc(diag_frame_head_stru *pData);
void bsp_hds_cnf_register(hds_cnf_func fn);
void bsp_hds_confirm(hds_cnf_stru *cnf, void *data, u32 len);
void bsp_hds_cnf_common_fill(hds_cnf_stru *cnf, diag_frame_head_stru *req);
void bsp_hds_service_init(void);

#endif
