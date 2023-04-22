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

#include "bsp_hds_service.h"
#include "bsp_hds_cmd.h"
#include "bsp_diag_frame.h"
#include "bsp_module.h"
#include <linux/list.h>
#include <linux/kernel.h>
#include "osl_malloc.h"

/*lint -save -e826 */
bsp_hds_cmd_stru g_hds_cmd_list_head;
hds_cnf_func     g_hds_cnf_fn = NULL;

/*lint -save -e429 */
void bsp_hds_cmd_register(u32 cmdid, bsp_hds_func fn)
{
    bsp_hds_cmd_stru *hds_list;
    struct list_head *me;
    bsp_hds_cmd_stru *pstDiag = NULL;

    /*判断cmdid是否重复注册*/
    list_for_each(me,&g_hds_cmd_list_head.list)
    {
        pstDiag = list_entry(me, bsp_hds_cmd_stru, list);

        if(cmdid == pstDiag->ulCmdId)
        {
            printk(KERN_ERR"cmdid:0x%x have registered!\n",cmdid);
            return;
        }
    }
    hds_list = osl_malloc((unsigned int)sizeof(bsp_hds_cmd_stru));
    if (NULL == hds_list)
    {
        printk(KERN_ERR"malloc hds_list memory failed!\n");
        return;
    }
    hds_list->ulCmdId = cmdid;
    hds_list->pFunc = fn;

    /*将注册的cmdid和回调函数添加到链表中*/
    list_add(&hds_list->list, &g_hds_cmd_list_head.list);
    //printk(KERN_ERR"registered cmdid:0x%x\n",hds_list->ulCmdId);
}
/*lint -restore*/


int bsp_hds_msg_proc(diag_frame_head_stru *pData)
{
    s32 ret = 0;
    struct list_head *me;
    bsp_hds_cmd_stru *pstDiag = NULL;

    if(DIAG_FRAME_MSG_TYPE_BSP != pData->stID.pri4b)
    {
        printk(KERN_ERR"Rcv Error Msg Id 0x%x\n",pData->u32CmdId);
        return BSP_ERROR;
    }

    /*遍历链表，根据注册的cmdid，跳到对应的回调中处理*/
    list_for_each(me,&g_hds_cmd_list_head.list)
    {
        pstDiag = list_entry(me, bsp_hds_cmd_stru, list);

        if(pstDiag->ulCmdId == pData->u32CmdId)
        {
            pstDiag->ulReserve ++;
            ret = pstDiag->pFunc((u8*)pData);
            return ret;
        }
    }

    /*未注册此cmdid，返回对应的错误码*/
    printk(KERN_ERR"cmdid is not register:%d\n",pData->u32CmdId);

    return HDS_CMD_ERROR;
}

void bsp_hds_get_cmdlist(u32 *cmdlist, u32 *num)
{
    u32 i = 0;
    struct list_head *me;
    bsp_hds_cmd_stru *pstDiag = NULL;
  
    if((NULL == cmdlist) || (NULL == num))
        return; 
    list_for_each(me,&g_hds_cmd_list_head.list)
    {
        if(i >= *num)
        {
            break;
        }

        pstDiag = list_entry(me, bsp_hds_cmd_stru, list);
        cmdlist[i++] = pstDiag->ulCmdId;
    }

    *num = i;

    return;
}

void bsp_hds_cnf_register(hds_cnf_func fn)
{
    g_hds_cnf_fn = fn;
}

void bsp_hds_confirm(hds_cnf_stru *cnf, void *data, u32 len)
{
    if(g_hds_cnf_fn)
    {
        g_hds_cnf_fn((void *)cnf, data, len);
    }
    else
    {
        printk(KERN_ERR"cnf fn isn't register, confirm failed!\n");
    }

    return;
}

void bsp_hds_cnf_common_fill(hds_cnf_stru *cnf, diag_frame_head_stru *req)
{
    cnf->ulSSId       = DIAG_FRAME_SSID_APP_CPU;
    cnf->ulMsgType    = DIAG_FRAME_MSG_TYPE_BSP;
    cnf->ulMode       = req->stID.mode4b;
    cnf->ulSubType    = req->stID.sec5b;
    cnf->ulDirection  = DIAG_FRAME_MT_CNF;
    cnf->ulModemid    = 0;
    cnf->ulMsgId      = req->u32CmdId;
    cnf->ulTransId    = req->stService.MsgTransId;
}


void bsp_ShowDebugInfo(void)
{
    struct list_head *me;
    bsp_hds_cmd_stru *pstDiag;

    list_for_each(me,&g_hds_cmd_list_head.list)
    {
        pstDiag = list_entry(me, bsp_hds_cmd_stru, list);

        printk(KERN_ERR"cmdid:0x%x\n",pstDiag->ulCmdId);
    }
}

void bsp_hds_service_init(void)
{
    INIT_LIST_HEAD(&g_hds_cmd_list_head.list);
    bsp_hds_log_init();
}

/*lint -restore +e826 */




