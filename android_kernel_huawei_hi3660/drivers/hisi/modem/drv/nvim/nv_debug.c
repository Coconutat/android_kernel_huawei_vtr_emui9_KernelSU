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


#include <linux/string.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/rtc.h>
#include "../../config/product/hi3660/config/product_config.h"
#include <bsp_dump.h>
#include <bsp_slice.h>
#include <bsp_nvim.h>
#include "../../adrv/adrv.h"
#include <bsp_rfile.h>
#include "NVIM_ResumeId.h"
#include "nv_partition_img.h"
#include "nv_comm.h"
#include "nv_ctrl.h"
#include "nv_debug.h"
#include "nv_index.h"
#include "nv_crc.h"
#include "../../include/nv/tl/drv/drv_nv_id.h"
#include "../../include/nv/tl/drv/drv_nv_def.h"
#include "../../include/drv/acore/mdrv_rfile.h"

NV_DEBUG_CFG_STRU              g_nv_debug_cfg;
extern struct nv_global_ctrl_info_stru  g_nv_ctrl;
struct nv_global_debug_stru g_nv_debug[NV_FUN_MAX_ID];
struct nv_debug_stru        g_nv_debug_info = {0};
debug_table_t g_nv_write_debug_table[NV_DEBUG_BUTT - 1] = {
    {NV_DEBUG_WRITEEX_START,            "write nv start"},
    {NV_DEBUG_WRITEEX_GET_IPC_START,    "for check crc,start to get ipc sem"},
    {NV_DEBUG_WRITEEX_GET_IPC_END,      "get ipc sem end"},
    {NV_DEBUG_WRITEEX_GIVE_IPC,         "check crc end, release ipc sem"},
    {NV_DEBUG_WRITEEX_MEM_START,        "write to mem start"},
    {NV_DEBUG_WRITEEX_FILE_START,       "write to file start"},
    {NV_DEBUG_FLUSH_START,              "flush nv list start"},
    {NV_DEBUG_FLUSH_END,                "flush nv list end"},
    {NV_DEBUG_REQ_FLUSH_START,          "req flush nv list start"},
    {NV_DEBUG_REQ_FLUSH_END,            "req flush nv list end"},
    {NV_DEBUG_FLUSHEX_START,            "flush nv to file start"},
    {NV_DEBUG_FLUSHEX_OPEN_START,       "open nv file start"},
    {NV_DEBUG_FLUSHEX_OPEN_END,          "open nv file end"},
    {NV_DEBUG_FLUSHEX_GET_PROTECT_SEM_START,"before write to file get ipc and sem start"},
    {NV_DEBUG_FLUSHEX_GET_PROTECT_SEM_END,"before write to file get ipc and sem end"},
    {NV_DEBUG_FLUSHEX_GIVE_IPC,         "write to file release ipc"},
    {NV_DEBUG_FLUSHEX_GIVE_SEM,         "release sem"},
    {NV_DEBUG_FLUSHEX_WRITE_FILE_START, "write to nv.bin start"},
    {NV_DEBUG_FLUSHEX_WRITE_FILE_END,   "write to nv.bin end"},
    {NV_DEBUG_WRITEEX_END,              "write nv end"},
    {NV_DEBUG_RECEIVE_ICC,              "recive icc from ccore"},
    {NV_DEBUG_SEND_ICC,                 "cnf to ccore"},
    {NV_DEBUG_READ_ICC,                 "read icc from ccore"},
    {NV_DEBUG_REQ_ASYNC_WRITE,          "send async write nv msg"},
    {NV_DEBUG_REQ_REMOTE_WRITE,         "read icc from ccore"}
};

s8 g_debug_check_file[255] = {0};
u32 g_debug_check_flag = 0;
extern struct nv_path_info_stru g_nv_path;
extern unsigned int bbox_check_edition(void);
void nv_debug_QueueInit(nv_queue_t *Q, u32 elementNum);


void nv_debug_QueueInit(nv_queue_t *Q, u32 elementNum)
{
    Q->maxNum = elementNum;
    Q->front = 0;
    Q->rear = 0;
    Q->num = 0;

    /* coverity[secure_coding] */
    (void)memset((void *)Q->data, 0, (size_t)(elementNum*sizeof(nv_queue_elemnt)));/* [false alarm]:fortify  */
}

static __inline__ s32 nv_debug_QueueIn(nv_queue_t *Q, nv_queue_elemnt element)
{
    if (Q->num == Q->maxNum)
    {
        return -1;
    }

    Q->data[Q->rear].state = element.state;
    Q->data[Q->rear].slice = element.slice;
    Q->rear = (Q->rear+1) % Q->maxNum;
    Q->num++;

    return 0;
}

static __inline__ s32 nv_debug_QueueLoopIn(nv_queue_t *Q, nv_queue_elemnt element)
{
    if (Q->num < Q->maxNum)
    {
        return nv_debug_QueueIn(Q, element);
    }
    else
    {
        Q->data[Q->rear].state = element.state;
        Q->data[Q->rear].slice = element.slice;
        Q->rear = (Q->rear+1) % Q->maxNum;
        Q->front = (Q->front+1) % Q->maxNum;
    }

    return 0;
}

u32 nv_debug_init(void)
{
    u32 ret;
    s32 fd = 0;

    ret = bsp_nvm_read(NV_ID_DRV_NV_DEBUG_CFG,(u8*)&(g_nv_debug_cfg),(u32)sizeof(NV_DEBUG_CFG_STRU));
    if(ret)
    {
        g_nv_debug_cfg.resume_bakup = 1;
        g_nv_debug_cfg.resume_img   = 1;
        g_nv_debug_cfg.save_ddr     = 0;
        g_nv_debug_cfg.save_image   = 0;
        g_nv_debug_cfg.save_bakup   = 0;
        g_nv_debug_cfg.reset        = 0;
        g_nv_debug_cfg.product      = NV_PRODUCT_PHONE;
        g_nv_debug_cfg.reserved     = 0;
        nv_printf("read nv 0x%x fail,use default value! ret :0x%x\n",NV_ID_DRV_NV_DEBUG_CFG,ret);
    }

    /*init nv dump queue*/
    g_nv_debug_info.write_debug_table = g_nv_write_debug_table;

    g_nv_debug_info.nv_dump_queue = (nv_queue_t *)(unsigned long)bsp_dump_register_field(DUMP_MODEMAP_NV, "NVA", 0, 0, NV_DUMP_SIZE, 0x0000);
    if(g_nv_debug_info.nv_dump_queue)
    {
        nv_debug_QueueInit(g_nv_debug_info.nv_dump_queue, (u32)((NV_DUMP_SIZE - 4*sizeof(u32))/sizeof(nv_queue_elemnt)));
    }
    else
    {
        nv_printf("alloc dump buffer fail, field id = 0x%x\n", DUMP_MODEMAP_NV);
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    /*if or not print info when write nv*/
    /* coverity[secure_coding] */
    memset(&NV_DEBUG_CTRL_VALUE, 0, sizeof(debug_ctrl_union_t));
    ret = (u32)bsp_access(NV_DEBUG_SWICH_PATH, 0);
    if(ret)
    {
        nv_printf("[debug info]debug file %s not exist, not a error\n", NV_DEBUG_SWICH_PATH);
    }
    else
    {
        fd = bsp_open(NV_DEBUG_SWICH_PATH, RFILE_RDONLY, 0660);
        if(fd < 0)
        {
            nv_printf("open %s fail\n", NV_DEBUG_SWICH_PATH);
        }
        else
        {
            ret = (u32)bsp_read((u32)fd, (s8 *)&NV_DEBUG_CTRL_VALUE, (u32)sizeof(debug_ctrl_union_t));
            if(bsp_close((u32)fd))
            {
                nv_printf("close debug file %s fail\n", NV_DEBUG_SWICH_PATH);
                return NV_OK;
            }
            if(sizeof(debug_ctrl_union_t) != ret)
            {
                nv_printf("read file fail, ret = 0x%x\n", ret);
                return NV_OK;
            }
        }
    }

    return NV_OK;
}

bool nv_debug_is_reset(void)
{
    /* MBB产品默认不复位 */
    if(NV_PRODUCT_MBB == nv_debug_product())
    {
        nv_printf("mbb product don't reset system! %d\n",g_nv_debug_cfg.reset);
        return (bool)g_nv_debug_cfg.reset;
    }
    else
    {
        if(g_nv_debug_cfg.reset)
        {
            nv_printf("phone product need reset system! %d\n",g_nv_debug_cfg.reset);
            return true;
        }

        /* 手机产品产线上不复位 */
        if(NV_MODE_FACTORY == g_nv_ctrl.nv_self_ctrl.ulResumeMode)
        {
            nv_printf("phone product don't reset system in factory! %d\n",g_nv_ctrl.nv_self_ctrl.ulResumeMode);
            return false;
        }
        else
        {
            /* 手机beta阶段复位，商用阶段不复位 */
            if(EDITION_USER == bbox_check_edition())
            {
                nv_printf("phone product don't reset system in user! \n");
                return false;
            }
            else
            {
                nv_printf("phone product need reset system in beta! \n");
                return true;
            }
        }
    }
}

void nv_debug_set_reset(u32 reset)
{
    if(reset)
    {
        g_nv_debug_cfg.reset = 1;
    }
    else
    {
        g_nv_debug_cfg.reset = 0;
        g_nv_ctrl.nv_self_ctrl.ulResumeMode = NV_MODE_FACTORY;
    }
}


#define BACK_DDR_DATA_PATH "/modem_log/drv/nv/ddr_nv.bin"
u32 nv_debug_store_ddr_data(void)
{
    u32 ret = 0;
    void * buf = NULL;
    FILE* dst_fp;
    u32 size = 0;

    if(!nv_debug_is_save_ddr())
    {
        return NV_OK;
    }

    dst_fp = mdrv_file_open(BACK_DDR_DATA_PATH, NV_FILE_WRITE);
    if(!dst_fp)
    {
        nv_printf("open %s fail\n", BACK_DDR_DATA_PATH);
        ret = BSP_ERR_NV_OPEN_FILE_FAIL;
        goto out;
    }
    buf = (u8 *)NV_GLOBAL_CTRL_INFO_ADDR;
    size = SHM_MEM_NV_SIZE - NV_GLOBAL_INFO_SIZE;
    ret = (u32)mdrv_file_write(buf, 1, size, dst_fp);
    if(ret != size)
    {
        nv_printf("nv wite file error, ret = 0x%x, size = 0x%x\n", ret, size);
        ret = BSP_ERR_NV_WRITE_DATA_FAIL;
        goto out;
    }
    ret = NV_OK;
out:
    nv_printf("ret = 0x%x\n", ret);
    if(dst_fp){mdrv_file_close(dst_fp);}
    return ret;
}

u32 nv_debug_store_file(char * src)
{
    FILE* src_fp;
    FILE* dst_fp;
    void * buf;
    char * dst = "/modem_log/drv/nv/nv_file.bin";
    u32 ret;
    u32 len;

    if(0 == strncmp(src, g_nv_path.file_path[NV_IMG], sizeof(g_nv_path.file_path[NV_IMG])))
    {
        if(!nv_debug_is_save_image())
        {
            return NV_OK;
        }
    }
    else if(0 == strncmp(src, NV_BACK_PATH, sizeof(NV_BACK_PATH)))
    {
        if(!nv_debug_is_save_bakup())
        {
            return NV_OK;
        }
    }
    else
    {
        return NV_OK;
    }

    /*open src file*/
    src_fp = nv_file_open(src, NV_FILE_READ);
    if(!src_fp)
    {
        nv_printf("open %s fail\n", src);
        return BSP_ERR_NV_OPEN_FILE_FAIL;
    }
    /*get src file len*/
    len = nv_get_file_len(src_fp);
    if(NV_ERROR == len)
    {
        nv_file_close(src_fp);
        nv_printf("get nv len error\n");
        return len;
    }
    /*open dst file*/
    dst_fp = mdrv_file_open(dst, NV_FILE_WRITE);
    if(!dst_fp)
    {
        nv_file_close(src_fp);
        nv_printf("open %s fail\n", dst);
        return BSP_ERR_NV_OPEN_FILE_FAIL;
    }
    /*write src file info*/
    ret = (u32)mdrv_file_write((void *)src, 1, (u32)strlen(src), dst_fp);
    if(ret != strlen(src))
    {
        nv_printf("write file info fail\n");
    }

    /*write src file data to dst file*/
    /* coverity[negative_returns] */
    buf = vmalloc((unsigned long)len);
    if(!buf)
    {
        nv_file_close(src_fp);
        mdrv_file_close(dst_fp);
        nv_printf("alloc buff fail ,len = 0x%x\n", len);
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    ret = (u32)nv_file_seek(src_fp,0,SEEK_SET);
    if(ret)
    {
        nv_printf("seek set file fail\n");
    }

    ret = (u32)nv_file_read(buf, 1,len, src_fp);
    if(ret != len)
    {
        nv_file_close(src_fp);
        mdrv_file_close(dst_fp);
        vfree(buf);
        nv_printf("read file len err, ret = 0x%x len = 0x%x\n", ret, len);
        return BSP_ERR_NV_READ_FILE_FAIL;
    }
    ret = (u32)mdrv_file_write(buf, 1, len, dst_fp);
    if(ret != len)
    {
        nv_file_close(src_fp);
        mdrv_file_close(dst_fp);
        vfree(buf);
        nv_printf("write file len err, ret = 0x%x len = 0x%x\n", ret, len);
        return BSP_ERR_NV_WRITE_DATA_FAIL;
    }
    nv_file_close(src_fp);
    mdrv_file_close(dst_fp);
    vfree(buf);
    return NV_OK;
}
/*****************************************************************************
 函 数 名  : nv_debug_switch
 功能描述  : nv debug功能开关
 输入参数  : void
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
u32 nv_debug_switch(debug_ctrl_union_t debug_switch, u32 type)
{
    s32 fd = 0;
    u32 ret = 0;

    switch(type)
    {
        case NV_DEBUG_SWITCH_TYPE_FOREVER:
            fd = bsp_open(NV_DEBUG_SWICH_PATH, RFILE_RDWR|RFILE_CREAT, 0660);
            if(fd < 0)
            {
                nv_printf("create %s fail\n", NV_DEBUG_SWICH_PATH);
                return NV_ERROR;
            }
            ret = (u32)bsp_write_sync((u32)fd, (const s8*)&debug_switch, (u32)sizeof(debug_ctrl_union_t));
            if(bsp_close((u32)fd))
            {
                nv_printf("close %s fail\n", NV_DEBUG_SWICH_PATH);
                return NV_ERROR;
            }
            if(sizeof(debug_ctrl_union_t) != ret)
            {
                nv_printf("write debug switch fail\n");
                return BSP_ERR_NV_WRITE_DATA_FAIL;
            }
            break;
        case NV_DEBUG_SWITCH_TYPE_TEMPORARY:
            NV_DEBUG_CTRL_VALUE = debug_switch;
            break;

        default:
            nv_printf("para error, type:0x%x\n", type);
    }

    return NV_OK;
}
/*****************************************************************************
 函 数 名  : nv_debug_record
 功能描述  : 对读写操作过程记录打点时间
 输入参数  : void
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void nv_debug_record(u32 current_state)
{
    nv_queue_elemnt new_record = {0};
    u32 table_index = 0;
    nv_global_info_s* ddr_info= (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    bool need_print = ((NV_INIT_OK != ddr_info->acore_init_state)
                        &&((NV_DEBUG_WRITEEX_GET_IPC_START == current_state)
                        ||(NV_DEBUG_WRITEEX_GET_IPC_END == current_state)
                        ||(NV_DEBUG_WRITEEX_GIVE_IPC == current_state)))?false:true;

    if((nv_debug_is_print_in_write())&&(need_print))
    {
        for(table_index = 0; table_index < NV_DEBUG_BUTT; table_index++)
        {
            if(g_nv_debug_info.write_debug_table[table_index].state == (current_state&0xFFFF))
            {
                nv_printf_info("%s\n", g_nv_debug_info.write_debug_table[table_index].info);
                break;
            }
        }
        switch (current_state&0xFFFF)
        {
            case NV_DEBUG_WRITEEX_START:
            case NV_DEBUG_WRITEEX_END:
                nv_printf_info("nv id: 0x%x\n", current_state>>16);
                break;
            case NV_DEBUG_SEND_ICC:
            case NV_DEBUG_READ_ICC:
                nv_printf_info("msg type: 0x%x\n", current_state>>16);
                break;
            default:
                break;
        }
    }

    if(!g_nv_debug_info.nv_dump_queue)
    {
        return;
    }
    new_record.slice = bsp_get_slice_value();
    new_record.state = current_state;

    nv_debug_QueueLoopIn(g_nv_debug_info.nv_dump_queue, new_record);

    return;
}
/*****************************************************************************
 函 数 名  : nv_debug_print_dump_queue
 功能描述  : 打印dump队列中的信息
 输入参数  : void
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void nv_debug_print_dump_queue(void)
{
    u32 queue_index;
    u32 queue_num;
    u32 state_str_index = 0;
    u32 current_state = 0;
    char * state_str_info = NULL;

    if(!g_nv_debug_info.nv_dump_queue)
    {
        nv_printf("dump queue is NULL\n");
        return;
    }

    queue_num = g_nv_debug_info.nv_dump_queue->num > g_nv_debug_info.nv_dump_queue->maxNum?\
                    g_nv_debug_info.nv_dump_queue->maxNum:g_nv_debug_info.nv_dump_queue->num;

    for(queue_index = g_nv_debug_info.nv_dump_queue->front; queue_index < queue_num; queue_index++)
    {
        current_state = g_nv_debug_info.nv_dump_queue->data[queue_index].state;

        for(state_str_index = 0; state_str_index < NV_DEBUG_BUTT; state_str_index++)
        {
            if(g_nv_debug_info.write_debug_table[state_str_index].state == (current_state&0xFFFF))
            {
                state_str_info = g_nv_debug_info.write_debug_table[state_str_index].info;
                break;
            }
        }
        nv_printf_info("slice:0x%x state:0x%x %s\n", g_nv_debug_info.nv_dump_queue->data[queue_index].slice, \
                g_nv_debug_info.nv_dump_queue->data[queue_index].state, state_str_info);

        switch (current_state&0xFFFF)
        {
            case NV_DEBUG_WRITEEX_START:
            case NV_DEBUG_WRITEEX_END:
                nv_printf_info("nv id: 0x%x\n", current_state>>16);
                break;
            case NV_DEBUG_SEND_ICC:
            case NV_DEBUG_READ_ICC:
                nv_printf_info("msg type: 0x%x\n", current_state>>16);
                break;
            default:
                break;
        }

    }
    return;
}
/*****************************************************************************
 函 数 名  : nv_debug_record_delta_time
 功能描述  : 记录操作最大时间
 输入参数  : void
 输出参数  : 无
 返 回 值  : 无
*****************************************************************************/
void nv_debug_record_delta_time(u32 type, u32 start, u32 end)
{
    u32 delta;

    delta = get_timer_slice_delta(start, end);
    if(delta > g_nv_debug_info.delta_time[type])
    {
        g_nv_debug_info.delta_time[type] = delta;
    }
    return;
}
void nv_debug_print_delta_time(void)
{
    u32 type;
    for(type = 0;type < NV_DEBUG_DELTA_BUTT; type++)
    {
        nv_printf_info("type 0x%x max delta time 0x%x\n", type, g_nv_debug_info.delta_time[type]);
    }
    return;
}


void nv_show_ddr_info(void)
{
    u32 i;
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    nv_file_info_s* file_info = (nv_file_info_s*)(NV_GLOBAL_CTRL_INFO_ADDR+NV_GLOBAL_CTRL_INFO_SIZE);
    nv_item_info_s* ref_info   = (nv_item_info_s*)(NV_GLOBAL_CTRL_INFO_ADDR+NV_GLOBAL_CTRL_INFO_SIZE\
        +NV_GLOBAL_FILE_ELEMENT_SIZE*ctrl_info->file_num);
    struct rtc_time tm = {0};


    nv_get_current_sys_time(&tm);
    nv_printf("current slice:0x%x sys time:%d-%02d-%02d %02d:%02d:%02d\n",\
                    bsp_get_slice_value(),\
                    tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,\
                    tm.tm_hour, tm.tm_min, tm.tm_sec);

    nv_printf("global start ddr        :0x%p\n",NV_GLOBAL_INFO_ADDR);
    nv_printf("global ctrl file ddr    :0x%p\n",NV_GLOBAL_CTRL_INFO_ADDR);
    nv_printf("global file list ddr    :0x%p\n",file_info);
    nv_printf("global ref info  ddr    :0x%p\n",ref_info);
    nv_printf("*******************ddr global ctrl************************\n");
    nv_printf("acore init state    : 0x%x\n",ddr_info->acore_init_state);
    nv_printf("ccore init state    : 0x%x\n",ddr_info->ccore_init_state);
    nv_printf("ddr read case       : 0x%x\n",ddr_info->ddr_read);
    nv_printf("crc status          : 0x%x\n",ctrl_info->crc_mark);
    nv_printf("mid priority 		: 0x%x\n",ddr_info->priority);
    nv_printf("mem file type   : 0x%x\n",ddr_info->mem_file_type);
    nv_printf("total revert count      :%d\n",g_nv_ctrl.revert_count);
    nv_printf("revert search err count :%d\n",g_nv_ctrl.revert_search_err);
    nv_printf("revert len err count    :%d\n",g_nv_ctrl.revert_len_err);
    nv_printf("file total len  : 0x%x\n",ddr_info->file_len);
    nv_printf("comm file num   : 0x%x\n",ddr_info->file_num);
    nv_printf("nv resume mode  : 0x%x\n", g_nv_ctrl.nv_self_ctrl.ulResumeMode);

    if(ddr_info->acore_init_state <= NV_INIT_FAIL)
    return;
    for(i = 0;i<ddr_info->file_num;i++)
    {
        nv_printf("##############################\n");
        nv_printf("** file id   0x%x\n",ddr_info->file_info[i].file_id);
        nv_printf("** file size 0x%x\n",ddr_info->file_info[i].size);
        nv_printf("** file off  0x%x\n",ddr_info->file_info[i].offset);
    }

    nv_printf("*******************global ctrl file***********************\n");
    nv_printf("ctrl file size    : 0x%x\n",ctrl_info->ctrl_size);
    nv_printf("file num          : 0x%x\n",ctrl_info->file_num);
    nv_printf("file list off     : 0x%x\n",ctrl_info->file_offset);
    nv_printf("file list size    : 0x%x\n",ctrl_info->file_size);
    nv_printf("ctrl file magic   : 0x%x\n",ctrl_info->magicnum);
    nv_printf("modem num         : 0x%x\n",ctrl_info->modem_num);
    nv_printf("nv count          : 0x%x\n",ctrl_info->ref_count);
    nv_printf("nv ref data off   : 0x%x\n",ctrl_info->ref_offset);
    nv_printf("nv ref data size  : 0x%x\n",ctrl_info->ref_size);
    nv_printf("*******************global file list***********************\n");
    for(i = 0;i<ctrl_info->file_num;i++)
    {
        nv_printf("file_info     : 0x%x\n",file_info);
        nv_printf("file id       : 0x%x\n",file_info->file_id);
        nv_printf("file name     : %s\n",file_info->file_name);
        nv_printf("file size     : 0x%x\n",file_info->file_size);
        file_info ++;
        nv_printf("\n");
    }
}


void nv_show_ref_info(u32 arg1,u32 arg2)
{
    u32 i;
    u32 _max;
    u32 _min;
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;
    nv_item_info_s* ref_info   = (nv_item_info_s*)(NV_GLOBAL_CTRL_INFO_ADDR+NV_GLOBAL_CTRL_INFO_SIZE\
        +NV_GLOBAL_FILE_ELEMENT_SIZE*ctrl_info->file_num);

    _max = arg2 > ctrl_info->ref_count ? ctrl_info->ref_count:arg2;
    _min = arg1 > _max ? 0: arg1;

    _max = (arg2 ==0)?ctrl_info->ref_count: _max;

    ref_info = ref_info+_min;

    for(i = _min;i<_max;i++)
    {
        nv_printf("第%d项 :\n",i);
        nv_printf("nvid   :0x%-8x, file id : 0x%-8x\n",ref_info->itemid,ref_info->file_id);
        nv_printf("nvlen  :0x%-8x, nv_off  : 0x%-8x, nv_pri 0x%-8x\n",ref_info->nv_len,ref_info->nv_off,ref_info->priority);
        nv_printf("dsda   :0x%-8x\n",ref_info->modem_num);
        ref_info++;
    }
}

void nv_show_fastboot_err(void)
{
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;

    nv_printf("**************************************\n");
    nv_printf("line       :%d\n",  ddr_info->fb_debug.line);
    nv_printf("result     :0x%x\n",ddr_info->fb_debug.ret);
    nv_printf("reseved1   :0x%x\n",ddr_info->fb_debug.reseverd1);
    nv_printf("reseved2   :0x%x\n",ddr_info->fb_debug.reseverd2);
    nv_printf("**************************************\n");
}


/* [false alarm]:i is in using */
void nv_help(u32 type)
{
    /*[false alarm]:i is in using*/
    u32 i = type;
    if(type == 63/*'?'*/)
    {
        nv_printf("1   -------  read\n");
        nv_printf("4   -------  auth read\n");
        nv_printf("5   -------  write\n");
        nv_printf("6   -------  auth write\n");
        nv_printf("8   -------  get len\n");
        nv_printf("9   -------  auth get len\n");
        nv_printf("10  -------  flush\n");
        nv_printf("12  -------  backup\n");
        nv_printf("15  -------  import\n");
        nv_printf("16  -------  export\n");
        nv_printf("19  -------  kernel init\n");
        nv_printf("20  -------  remain init\n");
        nv_printf("21  -------  nvm init\n");
        nv_printf("22  -------  xml decode\n");
        nv_printf("24  -------  revert\n");
        nv_printf("25  -------  update default\n");
        return;

    }
    if(type == NV_FUN_MAX_ID)
    {
        for(i = 0;i< NV_FUN_MAX_ID;i++)
        {
            nv_printf("************fun id %d******************\n",i);
            nv_printf("call num             : 0x%x\n",g_nv_debug[i].callnum);
            nv_printf("out branch (reseved1): 0x%x\n",g_nv_debug[i].reseved1);
            nv_printf("reseved2             : 0x%x\n",g_nv_debug[i].reseved2);
            nv_printf("reseved3             : 0x%x\n",g_nv_debug[i].reseved3);
            nv_printf("reseved4             : 0x%x\n",g_nv_debug[i].reseved4);
            nv_printf("***************************************\n");
        }
        return ;
    }

    i = type;
    nv_printf("************fun id %d******************\n",i);
    nv_printf("call num             : 0x%x\n",g_nv_debug[i].callnum);
    nv_printf("out branch (reseved1): 0x%x\n",g_nv_debug[i].reseved1);
    nv_printf("reseved2             : 0x%x\n",g_nv_debug[i].reseved2);
    nv_printf("reseved3             : 0x%x\n",g_nv_debug[i].reseved3);
    nv_printf("reseved4             : 0x%x\n",g_nv_debug[i].reseved4);
    nv_printf("***************************************\n");
}


u32 nv_show_item_info(u16 itemid)
{
    nv_global_info_s* ddr_info = (nv_global_info_s*)NV_GLOBAL_INFO_ADDR;
    nv_item_info_s ref_info = {0};
    nv_file_info_s file_info = {0};
    u32 nv_phy_addr;
    void* nv_virt_addr;
    u32 ret;

    ret = nv_search_byid((u32)itemid, (u8*)NV_GLOBAL_CTRL_INFO_ADDR, &ref_info, &file_info);
    if(ret)
    {
        return NV_ERROR;
    }
    nv_printf("itemid = 0x%x\n", ref_info.itemid);
    nv_printf("nv_len = 0x%x\n", ref_info.nv_len);
    nv_printf("nv_off = 0x%x\n", ref_info.nv_off);
    nv_printf("file_id = 0x%x\n", ref_info.file_id);
    nv_printf("priority = 0x%x\n", ref_info.priority);
    nv_printf("modem_num = 0x%x\n", ref_info.modem_num);

    nv_printf("file_id = 0x%x\n", file_info.file_id);
    nv_printf("file_name = %s\n", file_info.file_name);
    nv_printf("file_size = 0x%x\n", file_info.file_size);

    nv_virt_addr = (void *)(NV_GLOBAL_CTRL_INFO_ADDR + ddr_info->file_info[ref_info.file_id - 1].offset + ref_info.nv_off);

    nv_phy_addr = (u32)(unsigned long)SHD_DDR_V2P(nv_virt_addr);

    nv_printf("nv_virt_addr = 0x%x\n", nv_virt_addr);
    nv_printf("nv_phy_addr = 0x%x\n", nv_phy_addr);

    return NV_OK;
}
void nv_show_crc_status(void)
{
    nv_ctrl_info_s* ctrl_info = (nv_ctrl_info_s*)NV_GLOBAL_CTRL_INFO_ADDR;

    nv_printf("crc status          : 0x%x\n",ctrl_info->crc_mark);
}

/*nv debug func,reseverd1 used to reg branch*/
void nv_debug(u32 type,u32 reseverd1,u32 reserved2,u32 reserved3,u32 reserved4)
{
    if(0 == reseverd1)
    {
        g_nv_debug[type].callnum++;
    }
    g_nv_debug[type].reseved1 = reseverd1;
    g_nv_debug[type].reseved2 = reserved2;
    g_nv_debug[type].reseved3 = reserved3;
    g_nv_debug[type].reseved4 = reserved4;
}


/*系统启动log记录接口，保存到 NV_LOG_PATH 中，大小限定在 NV_LOG_MAX_SIZE*/
void nv_record(char* fmt,...)
{
    char   buffer[256];
    va_list arglist;
    FILE* fp = NULL;
    u32 ret;
    u32 file_len;
    u32 buffer_size = 0;

    va_start(arglist, fmt);
    /* coverity[secure_coding] */
    vsnprintf(buffer,(size_t)256, fmt, arglist);/* [false alarm]:format string */
    va_end(arglist);

    nv_printf_info("%s",buffer);

    if(mdrv_file_access((char*)NV_LOG_PATH,0))
    {
        fp = mdrv_file_open((char*)NV_LOG_PATH,"w+");
        if(!fp)
            return;
    }
    else
    {
        fp = mdrv_file_open((char*)NV_LOG_PATH,"r+");
        if(!fp)
            return;
        if(mdrv_file_seek(fp,(long)0,SEEK_END))
        {
            mdrv_file_close(fp);
            return;
        }
        file_len = (u32)mdrv_file_tell(fp);
        if((file_len + strlen(buffer))>= NV_LOG_MAX_SIZE)
        {
            mdrv_file_close(fp);
            fp = mdrv_file_open((char*)NV_LOG_PATH,"w+");
            if(!fp)
                return;
        }
    }
    ret = (u32)mdrv_file_write(buffer,1,(unsigned int)strlen(buffer),fp);
    if(ret != strlen(buffer))
    {
        buffer_size = (u32)strlen(buffer);
        nv_printf("mdrv_file_write   nv   log err!  ret :0x%x buffer len :0x%x\n",ret,buffer_size);
    }
    mdrv_file_close(fp);
}

void nv_get_current_sys_time(struct rtc_time *tm)
{
    u32 ret;
    struct rtc_device *rtc = rtc_class_open(CONFIG_RTC_HCTOSYS_DEVICE);

    if (rtc == NULL){
        nv_error_printf("%s: unable to open rtc device (%s)\n",
            __FILE__, CONFIG_RTC_HCTOSYS_DEVICE);
        return;
    }

    ret = (u32)rtc_read_time(rtc, tm);
    if (ret) {
        nv_error_printf("unable to read the hardware clock, ret=0x%x\n", ret);
        return;
    }

    ret = (u32)rtc_valid_tm(tm);
    if (ret) {
        nv_error_printf("invalid date/time\n");
        return;
    }
    /*获取到的时间为格林威治时间，转换为北京时间*/
    tm->tm_hour+=8;
    return;
}
/*****************************************************************************
 Prototype       : nv_debug_is_bak_resume_nv
 Description     : 检测nv是否为备份恢复nv
 Return Value    : NONE
*****************************************************************************/
void nv_debug_is_bak_resume_nv(u16 itemid)
{
    u32 resume_num;
    u32 nv_index;

    resume_num = (u32)bsp_nvm_getRevertNum((unsigned long)NV_MANUFACTURE_ITEM);
    for(nv_index = 0; nv_index < resume_num; nv_index++)
    {
        if(itemid == g_ausNvResumeManufactureIdList[nv_index])
        {
            nv_printf("nv 0x%x is bak resume nv, is in resume table  g_ausNvResumeManufactureIdList\n", itemid);
            return;
        }
    }
    resume_num = (u32)bsp_nvm_getRevertNum((unsigned long)NV_USER_ITEM);
    for(nv_index = 0; nv_index < resume_num; nv_index++)
    {
        if(itemid == g_ausNvResumeUserIdList[nv_index])
        {
            nv_printf("nv 0x%x is bak resume nv, is in resume table  g_ausNvResumeUserIdList\n", itemid);
            return;
        }
    }

    resume_num = (u32)bsp_nvm_getRevertNum((unsigned long)NV_SECURE_ITEM);
    for(nv_index = 0; nv_index < resume_num; nv_index++)
    {
        if(itemid == g_ausNvResumeSecureIdList[nv_index])
        {
            nv_printf("nv 0x%x is bak resume nv, is in resume table  g_ausNvResumeSecureIdList\n", itemid);
            return;
        }
    }

    nv_printf("nv 0x%x is not bak resume nv\n", itemid);
    return;
}

/*****************************************************************************
 Prototype       : nv_debug_chk_invalid_type
 Description     : 检查文件是否是测试桩定义的文件
 Return Value    : NONE
*****************************************************************************/
u32 nv_debug_chk_invalid_type(const s8 * path, u32 invalid_type)
{
    if(0 != g_debug_check_flag)
    {
        if((0 == strncmp((const s8 *)path, (const s8 *)g_debug_check_file,
                            strlen((s8 *)g_debug_check_file) + 1)) && (g_debug_check_flag == invalid_type))
        {
            return g_debug_check_flag;
        }
    }

    return 0;
}

/*****************************************************************************
 Prototype       : nv_debug_set_invalid_type
 Description     : 设置测试桩定义的文件的名字和属性
 Return Value    : NONE
*************************************************************************/
void nv_debug_set_invalid_type(const s8 * path, u32 invalid_type)
{
    memcpy((s8 *)g_debug_check_file, (s8 *)path, sizeof(g_debug_check_file));
    g_debug_check_flag = invalid_type;
    return;
}

/*****************************************************************************
 Prototype       : nv_debug_set_invalid_type
 Description     : 清除测试桩定义的文件的名字和属性
 Return Value    : NONE
*************************************************************************/
void nv_debug_clear_invalid_type(void)
{
    memset((s8 *)g_debug_check_file, 0, sizeof(g_debug_check_file));
    g_debug_check_flag = 0;

    nv_img_clear_check_result();
    return;
}

u32 nvm_read_rand(u32 nvid)
{
    u32 ret;
    u8* tempdata;
    u32 i;
    nv_item_info_s ref_info = {0};
    nv_file_info_s file_info = {0};

    ret = nv_search_byid(nvid, (u8*)NV_GLOBAL_CTRL_INFO_ADDR,&ref_info,&file_info);
    if(NV_OK != ret)
    {
        return ret;
    }
    bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MODU_NV,"[0x%x]:len 0x%x,off 0x%x,file id %d\n",nvid,ref_info.nv_len,ref_info.nv_off,ref_info.file_id);

    tempdata = (u8*)nv_malloc((unsigned long)(ref_info.nv_len) +1);
    if(NULL == tempdata)
    {
        return BSP_ERR_NV_MALLOC_FAIL;
    }

    ret = bsp_nvm_read(nvid,tempdata,ref_info.nv_len);
    if(NV_OK != ret)
    {
        nv_free(tempdata);
        return BSP_ERR_NV_READ_DATA_FAIL;
    }
    for(i=0;i<ref_info.nv_len;i++)
    {
        if((i%32) == 0)
        {
            bsp_trace(BSP_LOG_LEVEL_ERROR,BSP_MODU_NV, "\n");
        }
        bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MODU_NV, "%02x ",(u8)(*(tempdata+i)));
    }
    nv_free(tempdata);
    bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MODU_NV,"\n\n");
    return 0;
}


u32 nvm_read_randex(u32 nvid,u32 modem_id)
{
    u32 ret;    
    u8* tempdata;    
    u32 i;
    nv_item_info_s ref_info = {0};
    nv_file_info_s file_info = {0};

    ret = nv_search_byid(nvid, (u8*)NV_GLOBAL_CTRL_INFO_ADDR,&ref_info,&file_info);
    if(NV_OK != ret)
    {
        return ret;
    }
    if(ref_info.nv_len == 0)
    {
        return NV_ERROR;
    }

    nv_printf("[0x%x]:len 0x%x,off 0x%x,file id %d\n",nvid,ref_info.nv_len,ref_info.nv_off,ref_info.file_id);
    nv_printf("[0x%x]:dsda 0x%x\n",nvid,ref_info.modem_num);

    tempdata = (u8*)nv_malloc((unsigned long)(ref_info.nv_len) +1);
    if(NULL == tempdata)
    {
        return BSP_ERR_NV_MALLOC_FAIL;
    }
    ret = bsp_nvm_dcread(modem_id,nvid,tempdata,ref_info.nv_len);
    if(NV_OK != ret)
    {
        nv_free(tempdata);
        return BSP_ERR_NV_READ_DATA_FAIL;
    }
    for(i=0;i<ref_info.nv_len;i++)
    {
        if((i%32) == 0)
        {
            nv_printf("\n");
        }
        nv_printf("%02x ",(u8)(*(tempdata+i)));
    }

    nv_printf("\n\n");
    nv_free(tempdata);

    return 0;

}

EXPORT_SYMBOL(nv_debug_print_dump_queue);
EXPORT_SYMBOL(nv_debug_print_delta_time);
EXPORT_SYMBOL(nv_debug_store_file);
EXPORT_SYMBOL(nv_debug_switch);
EXPORT_SYMBOL(nv_show_crc_status);
EXPORT_SYMBOL(nv_show_ref_info);
EXPORT_SYMBOL(nv_help);
EXPORT_SYMBOL(nv_show_fastboot_err);
EXPORT_SYMBOL(nv_show_item_info);
EXPORT_SYMBOL(nv_show_ddr_info);
EXPORT_SYMBOL(nv_debug_set_reset);
EXPORT_SYMBOL(nv_debug_is_bak_resume_nv);
EXPORT_SYMBOL(nv_debug_set_invalid_type);
EXPORT_SYMBOL(nv_debug_chk_invalid_type);
EXPORT_SYMBOL(nv_debug_clear_invalid_type);
