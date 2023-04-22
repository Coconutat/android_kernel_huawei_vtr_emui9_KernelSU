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
#ifndef __APR_MODEM_H__
#define __APR_MODEM_H__

#include "osl_types.h"
#include "bsp_dump.h"
#include "bsp_dump_mem.h"

typedef struct module_info
{
    u32     moudle_sum;               /*组件总数*/
    struct
    {
    	u32 module;	                  /*责任组件*/
        char module_name[DUMP_MODULE_NAME_LEN];         /*责任组件名称*/
    }module_info[1];
}module_info_t;

typedef struct modid_info
{
    u32     moudid_sum;             /*范围总数*/
    struct
    {
    	u32  modid_start;		    /*组件主动复位modid起始值*/
    	u32  modid_end;		        /*组件主动复位modid结束值*/
    	char module[DUMP_MODULE_NAME_LEN];	        /*责任组件*/
    }modid_info[1];
}modid_info_t;

typedef struct task_table
{
    u32 task_sum;                /*任务总数*/
    struct
    {
    	char  task_name[16]; 	/*任务名*/
    	char  module[DUMP_MODULE_NAME_LEN]; 		/*责任组件*/
    }task_info[1];
}task_table_t;

typedef struct interupt_table
{
    u32 intrupt_sum;            /*中断总数*/
    struct
    {
    	u32 interrpt_id; 		/*中断号*/
    	char  module[DUMP_MODULE_NAME_LEN]; 		/*责任组件*/
    }interupt_info[1];
}interupt_table_t;

typedef struct
{
    u32 except_core;
    u32 except_reason;
    u32 voice;
    u32 modId;
    u32 reboot_context;
    u32 reboot_task;
    u32 reboot_int;
}dump_except_info_t;

typedef struct
{
    u8  brieftype[16];       /*module+voice*/
    u8  module_name[12];      /*根因组件名*/
    u8  voice[4];            /*是否在语音下异常, yes,no*/
    u8  task_name[16];       /*复位任务名,如果为中断则为中断号,如果临终为任务则为任务名*/
    u32 reboot_int;          /*复位中断号*/
    u32 modid;               /*复位modid,system_error的第一个入参*/
    u8  reboot_core[8];      /*取值AP CP*/
    u8  reboot_reson[16];    /*取值Normal/DataAbort/Wdt*/
}dump_reset_log_t;

s32 dump_apr_init(void);
void dump_save_apr_data(char* dir_name);

void dump_apr_parse_reset_info( dump_reset_log_t *dump_reset_info, dump_except_info_t dump_except_info);
void dump_apr_get_reset_module(dump_except_info_t dump_except_info, u8* task_name,u8 * module);
void dump_apr_get_reset_task(dump_except_info_t dump_except_info,  u8 * task_name, u32 * reboot_int);
s32  dump_apr_get_reset_taskid(dump_queue_t *Q);
void dump_apr_get_reset_context_and_id(u32 *reboot_context, u32 *reboot_task,u32 *reboot_int, struct dump_global_struct_s* dump_head);
void dump_apr_get_reset_voice(u32 voice, u8 * reboot_voice);
void dump_apr_get_reset_voice(u32 voice, u8 * reboot_voice);
void dump_apr_get_reset_reason(u32 reason, u8 * reboot_reason);
void dump_apr_get_reset_ccore(u32 core, char *reboot_core);
void dump_apr_get_reset_ccore(u32 core, char *reboot_core);
void dump_apr_get_reset_modid(u32 reason, u32 reboot_modid, u32 * modId);
#endif
