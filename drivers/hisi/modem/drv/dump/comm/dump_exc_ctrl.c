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
#include "osl_types.h"
#include "bsp_dump.h"
#include "dump_print.h"
#include "dump_exc_ctrl.h"
dump_reboot_contex_s            g_dump_reboot_contex = {DUMP_CPU_BUTTON,DUMP_REASON_UNDEF};
volatile u32                    g_exc_flag   = false;
u32                             g_init_flag = DUMP_INIT_FLAG_CONFIG;

/*****************************************************************************
* 函 数 名  : dump_get_init_phase
* 功能描述  : 获取当前的初始化阶段
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
u32 dump_get_init_phase(void)
{
    return g_init_flag;
}

/*****************************************************************************
* 函 数 名  : dump_set_init_phase
* 功能描述  : 设置当前的初始化阶段
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_set_init_phase(u32 phase)
{
    g_init_flag = phase;
}

/*****************************************************************************
* 函 数 名  : bsp_dump_get_reboot_contex
* 功能描述  : 为apr提供的接口用于获取当前的异常核以及异常原因
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_get_reboot_contex(u32* core ,u32* reason)
{
    if(core != NULL)
    {
        *core = (u32)g_dump_reboot_contex.reboot_core;
    }
    if(reason != NULL)
    {
        *reason = (u32)g_dump_reboot_contex.reboot_reason;
    }
}

/*****************************************************************************
* 函 数 名  : dump_set_reboot_contex
* 功能描述  : 设定异常原因和异常核
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_set_reboot_contex(dump_reboot_cpu_t core ,dump_reboot_reason_t reason)
{
    g_dump_reboot_contex.reboot_core = core;
    g_dump_reboot_contex.reboot_reason = reason;
}
/*****************************************************************************
* 函 数 名  : dump_check_has_error
* 功能描述  : 判断是否发生过一场
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
u32 dump_check_has_error(void)
{
    if(g_exc_flag)
    {
        dump_fetal("exc has happen,do not deal new exc\n");
        return true;
    }
    else
    {
        g_exc_flag = true;
    }
    return false;

}
/*****************************************************************************
* 函 数 名  : dump_config_init
* 功能描述  : dump 配置初始化
*
* 输入参数  :
* 输出参数  :

* 返 回 值  :

*
* 修改记录  : 2016年1月4日17:05:33   lixiaofan  creat
*
*****************************************************************************/
void dump_set_exc_flag(u32 flag)
{
    g_exc_flag = flag;
}

