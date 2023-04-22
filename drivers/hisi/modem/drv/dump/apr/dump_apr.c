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


#include <linux/sched.h>
#include <linux/timer.h>
#include <linux/thread_info.h>
#include <linux/syslog.h>
#include <linux/errno.h>
#include <linux/kthread.h>
#include <linux/semaphore.h>
#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/spinlock.h>
#include <linux/notifier.h>
#include <linux/kdebug.h>
#include <linux/reboot.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/delay.h>
#include <linux/wakelock.h>
#include <asm/string.h>
#include <asm/traps.h>
#include "product_config.h"
#include <linux/syscalls.h>
#include "osl_types.h"
#include "osl_io.h"
#include "osl_bio.h"
#include "osl_malloc.h"
#include "bsp_dump.h"
#include "bsp_nvim.h"
#include "bsp_dump_mem.h"
#include "drv_nv_def.h"
#include "mdrv_om.h"
#include "drv_comm.h"
#include <gunas_errno.h>
#include "dump_modem_area.h"
#include "dump_apr.h"
#include "dump_print.h"
#include "dump_file.h"
#include "dump_config.h"
#include "dump_exc_ctrl.h"



/*存储cp中断信息 中断号 责任组件*/
interupt_table_t *g_cp_interupt_table = NULL;

/*存储cp任务 责任组件信息*/
task_table_t *g_cp_task_table = NULL;
/*存储modid范围 责任组件信息*/
modid_info_t *g_p_modid_table = NULL;

/*****************************************************************************
* 函 数 名  : bsp_dump_parse_apr_dts_info
*
* 功能描述  : 读取dts初始化任务信息和中断信息 组件信息
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
s32 dump_apr_init(void)
{

    return BSP_OK;
}
/*****************************************************************************
* 函 数 名  : bsp_dump_parse_reset_info
*
* 功能描述  : 将异常信息解析
*
* 输入参数  :  reset_info:存储解析后的异常信息的数据流
               size
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_parse_reset_info( dump_reset_log_t *dump_reset_info, dump_except_info_t dump_except_info)
{

}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_module
*
* 功能描述  : 解析复位的责任组件
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_module(dump_except_info_t dump_except_info, u8* task_name,u8 * module)
{

}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_cp_reset_reason
*
* 功能描述  : 获取CP复位的原因
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_task(dump_except_info_t dump_except_info,  u8 * task_name, u32 * reboot_int)
{

}

/*****************************************************************************
* 函 数 名  : bsp_dump_search_taskid
*
* 功能描述  : 查找复位的task id
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
s32 dump_apr_get_reset_taskid(dump_queue_t *Q)
{

    return BSP_ERROR;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_context_and_taskid
*
* 功能描述  :获取复位为中断复位还是任务复位,如果为任务复位,解析复位任务id，如果复位为中断，解析复位中断号
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_context_and_id(u32 *reboot_context, u32 *reboot_task,u32 *reboot_int, struct dump_global_struct_s* dump_head)
{


}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_voice
*
* 功能描述  : 解析复位时是否在语音下
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_voice(u32 voice, u8 * reboot_voice)
{

}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_modid
*
* 功能描述  : 获取复位的modid
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_modid(u32 reason, u32 reboot_modid, u32 * modId)
{

}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_cp_reset_reason
*
* 功能描述  : 获取复位的原因
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_reason(u32 reason, u8 * reboot_reason)
{

}
/*****************************************************************************
* 函 数 名  : bsp_dump_get_reset_ccore
*
* 功能描述  : 获取CP复位的原因
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_ccore(u32 core, char *reboot_core)
{

}


/*****************************************************************************
* 函 数 名  : bsp_dump_show_apr_cfg
*
* 功能描述  : 显示apr的配置
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
s32  dump_apr_show_cfg(u32 type)
{

    return BSP_OK;
}
/*****************************************************************************
* 函 数 名  : om_get_reset_info
*
* 功能描述  : 获取复位信息
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_apr_get_reset_info(char * reset_info, u32 size)
{

}
/*****************************************************************************
* 函 数 名  : bsp_apr_save_data
*
* 功能描述  : 保存apr数据
*
* 输入参数  :
*
*
* 输出参数  :无
*
* 返 回 值  : 无
*****************************************************************************/
void dump_save_apr_data(char* dir_name)
{

}

