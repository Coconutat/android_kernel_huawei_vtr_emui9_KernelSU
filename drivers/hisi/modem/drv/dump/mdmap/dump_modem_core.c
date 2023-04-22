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
#include <linux/rtc.h>
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
#include "bsp_ipc.h"
#include "bsp_memmap.h"
#include "bsp_wdt.h"
#include "bsp_icc.h"
#include "bsp_onoff.h"
#include "bsp_nvim.h"
#include "bsp_softtimer.h"
#include "bsp_version.h"
#include "bsp_sram.h"
#include "bsp_dump_mem.h"
#include "bsp_coresight.h"
#include "bsp_reset.h"
#include "drv_nv_def.h"
#include "mdrv_om.h"
#include <gunas_errno.h>
#include "bsp_adump.h"
#include "bsp_wdt.h"
#include "dump_modem.h"
#include "dump_modem_rdr.h"
#include "dump_config.h"
#include "dump_print.h"
#include "dump_modem_field.h"
#include "dump_modem_rdr.h"
#include "dump_modem_baseinfo.h"
#include "dump_modem_save.h"
#include "dump_cp_agent.h"
#include "dump_apr.h"
#include "dump_config.h"
#include "dump_exc_ctrl.h"



/*****************************************************************************
* 函 数 名  : dump_show_stack
* 功能描述  : 打印调用栈，用于非arm异常
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/

void dump_show_stack(u32 modid,u32 reason)
{
    if (!(AP_DUMP_REASON_ARM == reason))
    {
        dump_fetal("###########show mem and current task stack start##############!\n");

        show_mem(0);

        if(DUMP_T_TASK_ERROR(modid))
        {
            dump_fetal("not current task exc\n");
            show_stack(find_task_by_vpid(reason),NULL);
        }
        else
        {
            show_stack(current, NULL);
        }

        dump_fetal("###########show mem and current task stack end################!\n");
    }
}

/*****************************************************************************
* 函 数 名  : dump_save_usr_data
* 功能描述  : 保存用户数据区
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_save_usr_data(char *data, u32 length)
{
    u32 len =0;
    void* addr = NULL;
    dump_field_map_t* pfield = NULL;

    if ((NULL != data) && (length))
    {
        pfield = (dump_field_map_t*)bsp_dump_get_field_map(DUMP_MODEMAP_USER_DATA);
        addr = (void*)bsp_dump_get_field_addr(DUMP_MODEMAP_USER_DATA);
        len = (length > DUMP_MODEMAP_USER_DATA_SIZE) ? DUMP_MODEMAP_USER_DATA_SIZE : length;

        if(addr != NULL)
        {
            /*coverity[secure_coding]*/
            memcpy((void *)addr, (const void * )(uintptr_t)data, (size_t)len);
        }

        if(pfield!= NULL)
        {
            pfield->length = len;
        }
    }
    dump_fetal("dump save usr data finish\n");
    return;
}
/*****************************************************************************
* 函 数 名  : system_error
* 功能描述  : modem 异常函数入口
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void system_error(u32 mod_id, u32 arg1, u32 arg2, char *data, u32 length)
{
    u32 rdr_mod_id;

    rdr_mod_id = dump_match_rdr_mod_id(mod_id);

    dump_fetal("rdr mod id is 0x%x\n", rdr_mod_id);

    if(rdr_mod_id == RDR_MODEM_CP_RESET_SIM_SWITCH_MOD_ID ||rdr_mod_id ==  RDR_MODEM_CP_RESET_USER_RESET_MOD_ID)
    {
        dump_fetal("rdr mod id no need to save log,enter rdr_system_error directly\n");
        rdr_system_error(rdr_mod_id, arg1, arg2);
        return;
    }

    rdr_system_error(rdr_mod_id, arg1, arg2);

    return;
}


/*****************************************************************************
* 函 数 名  : bsp_dump_init
* 功能描述  : modem dump 初始化函数
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
s32 __init bsp_dump_init(void)
{
    s32 ret ;

    dump_config_init();

    dump_mdmap_field_init();

    dump_set_init_phase(DUMP_INIT_FLAG_CONFIG);

    ret = dump_base_info_init();
    if(ret == BSP_ERROR)
    {
        dump_fetal("dump_base_info_init fail\n");
    }
    dump_set_init_phase(DUMP_INIT_FLAG_BASEINFO);

    ret = dump_save_task_init();
    if(ret == BSP_ERROR)
    {
        dump_fetal("dump_save_task_init fail\n");
    }
    dump_set_init_phase(DUMP_INIT_FLAG_SAVETASK);

    ret = dump_register_rdr_exc();
    if(ret == BSP_ERROR)
    {
        dump_fetal("dump_register_rdr_exc fail\n");
    }
    dump_set_init_phase(DUMP_INIT_FLAG_RDR_REG);

    ret = dump_cp_agent_init();
    if(BSP_OK != ret)
    {
        dump_fetal("dump_cp_agent_init fail\n");

    }
    dump_set_init_phase(DUMP_INIT_FLAG_CP_AGENT);

    ret = dump_apr_init();
    if(BSP_OK != ret)
    {
        dump_fetal("bsp_apr_init fail\n");

    }
    dump_set_init_phase(DUMP_INIT_FLAG_APR);

    dump_set_exc_flag(false);

    dump_set_init_phase(DUMP_INIT_FLAG_DONE);

    return BSP_OK;
}

EXPORT_SYMBOL_GPL(system_error);
module_init(bsp_dump_init);


