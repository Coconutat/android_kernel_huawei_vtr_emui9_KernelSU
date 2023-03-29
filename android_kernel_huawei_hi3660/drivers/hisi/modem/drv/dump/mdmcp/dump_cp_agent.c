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
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <asm/string.h>
#include <linux/kthread.h>
#include <linux/timer.h>
#include <linux/timex.h>
#include <linux/rtc.h>
#include <linux/sched.h>
#include "osl_types.h"
#include "bsp_sysctrl.h"
#include "bsp_slice.h"
#include "bsp_wdt.h"
#include "bsp_ipc.h"
#include "bsp_fiq.h"
#include "bsp_coresight.h"
#include "bsp_dump.h"
#include "bsp_adump.h"
#include "bsp_ddr.h"
#include "bsp_slice.h"
#include "bsp_om.h"
#include "dump_modem_field.h"
#include "dump_print.h"
#include "dump_cp_agent.h"
#include "dump_cp_wdt.h"
#include "dump_config.h"
#include "dump_modem_baseinfo.h"
#include "dump_modem_rdr.h"
#include "dump_lphy_tcm.h"
#include "dump_cphy_tcm.h"
#include "dump_modem_rdr.h"
#include "dump_exc_ctrl.h"
#include "dump_file.h"

u8*      g_modem_ddr_map_addr = NULL;
struct semaphore g_cp_agent_sem;
u32 g_rdr_mod_id = 0;

/*****************************************************************************
* 函 数 名  : dump_memcpy
* 功能描述  : 拷贝寄存器函数
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_memcpy(u32 * dst, const u32 * src, u32 len)
{
    while(len-- > 0)
    {
        *dst++ = *src++;
    }
}
/*****************************************************************************
* 函 数 名  : dump_save_modem_sysctrl
* 功能描述  : 保存modem的系统控制寄存器
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_modem_sysctrl(void)
{

}

/*****************************************************************************
* 函 数 名  : dump_memmap_modem_ddr
* 功能描述  : 映射modem ddr的内存，只在手机版本上使用，mbb平台上在fastboot导出
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_map_mdm_ddr(void)
{

}

/*****************************************************************************
* 函 数 名  : dump_save_mdm_ddr_file
* 功能描述  : 保存modem的ddr
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_mdm_ddr_file(char* dir_name)
{

}

/*****************************************************************************
* 函 数 名  : dump_save_mdm_ddr_file
* 功能描述  : 保存modem的ddr
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_mdm_dts_file(char* dir_name)
{

}
/*****************************************************************************
* 函 数 名  : dump_cp_wdt_hook
* 功能描述  : cp 看门狗回调函数
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_cp_wdt_hook(void)
{
    system_error(DRV_ERRNO_DUMP_CP_WDT, DUMP_REASON_WDT, 0, 0, 0);
}


/*****************************************************************************
* 函 数 名  : dump_get_cp_task_name_by_id
* 功能描述  : 通过任务id查找任务名
*
* 输入参数  :task_id
* 输出参数  :task_name

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_get_cp_task_name_by_id(u32 task_id, char* task_name)
{


}

/*****************************************************************************
* 函 数 名  : dump_save_cp_base_info
* 功能描述  : 保存modem ap的基本信息
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
/*看门狗异常要重新考虑怎么处理*/
void dump_save_cp_base_info(u32 mod_id, u32 arg1, u32 arg2, char *data, u32 length)
{

}


/*****************************************************************************
* 函 数 名  : dump_wait_cp_save_done
* 功能描述  : 保存modem cp的区域
*
* 输入参数  : u32 ms  等待的毫秒数
              bool wait 是否循环等待
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 dump_wait_cp_save_done(u32 ms,bool wait)
{
    return BSP_OK;
}

/*****************************************************************************
* 函 数 名  : dump_int_handle
* 功能描述  : 处理modem cp发送过来的中断
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_cp_agent_handle(s32 param)
{
    g_rdr_mod_id = RDR_MODEM_CP_MOD_ID;

    up(&g_cp_agent_sem);

    return;
}



/*****************************************************************************
* 函 数 名  : dump_notify_cp
* 功能描述  : 通知modem cp
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_notify_cp(u32 mod_id)
{

}

/*****************************************************************************
* 函 数 名  : dump_cp_wdt_proc
* 功能描述  : 看门狗异常处理
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_cp_wdt_proc(void)
{
}
/*****************************************************************************
* 函 数 名  : dump_cp_wdt_proc
* 功能描述  : 看门狗异常处理
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_cp_dlock_proc(void)
{
}

/*****************************************************************************
* 函 数 名  : dump_cp_task
* 功能描述  : 保存modem log的入口函数
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
int dump_cp_task(void *data)
{
    (void)data;
    /* coverity[no_escape] */
    while(1)
    {
        down(&g_cp_agent_sem);
        dump_fetal("down g_cp_agent_sem ,g_rdr_mod_id = 0x%x\n",g_rdr_mod_id);
        rdr_system_error(g_rdr_mod_id, 0, 0);
    }
    /*lint -save -e527*/
    return BSP_OK;
    /*lint -restore +e527*/
}

/*****************************************************************************
* 函 数 名  : dump_save_task_init
* 功能描述  : 创建modem ap 保存log 的任务函数
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 dump_cp_task_init(void)
{
    struct task_struct * pid;
    struct sched_param   param = {0,};

    sema_init(&g_cp_agent_sem, 0);

    pid = (struct task_struct *)kthread_run(dump_cp_task, 0, "dump_cp_task");
    if (IS_ERR((void*)pid))
    {
        return BSP_ERROR;
    }
    param.sched_priority = 97;
    if (BSP_OK != sched_setscheduler(pid, SCHED_FIFO, &param))
    {
        dump_error("dump_cp_task_init[%d]: sched_setscheduler failed!\n", __LINE__);
        return BSP_ERROR;
    }

    dump_fetal("dump_cp_task_init finish\n");

    return BSP_OK;
}

/*****************************************************************************
* 函 数 名  : dump_cp_wdt_proc
* 功能描述  : 看门狗异常处理
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 dump_cp_agent_init(void)
{
    int ret ;

    dump_cp_task_init();

    dump_map_mdm_ddr();

    ret = bsp_ipc_int_connect(IPC_ACPU_SRC_CCPU_DUMP, (voidfuncptr)dump_cp_agent_handle, 0);
    if(BSP_OK != ret)
    {
        dump_error("bsp_ipc_int_connect fail\n");
        return BSP_ERROR;
    }

    ret = bsp_ipc_int_enable(IPC_ACPU_SRC_CCPU_DUMP);
    if(BSP_OK != ret)
    {
        dump_error("bsp_ipc_int_enable fail\n");
        return BSP_ERROR;
    }

    ret = bsp_wdt_register_hook(WDT_CCORE_ID,dump_cp_wdt_hook);

    if(ret == BSP_ERROR)
    {
        dump_fetal("dump_register_hook fail\n");
    }

    return BSP_OK;

}
/*****************************************************************************
* 函 数 名  : dump_cp_save_logs
* 功能描述  : 保存c核的log
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_cp_logs(char* dir_name)
{

}
/*****************************************************************************
* 函 数 名  : dump_cp_timeout_proc
* 功能描述  : IPC中断超时的处理
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_cp_timeout_proc(void)
{

}

