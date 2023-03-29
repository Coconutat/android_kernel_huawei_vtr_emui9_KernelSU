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
#include "osl_types.h"
#include "osl_io.h"
#include "osl_bio.h"
#include "bsp_dump.h"
#include "bsp_ipc.h"
#include "bsp_memmap.h"
#include "bsp_wdt.h"
#include "bsp_icc.h"
#include "bsp_onoff.h"
#include "bsp_nvim.h"
#include "bsp_softtimer.h"
#include "bsp_version.h"
#include "of.h"
#include "dump_print.h"

u32 g_wdt_int_no = 0;

wdt_timeout_cb g_wdt_reboot_func = NULL;

static irqreturn_t watchdog_mdm_int(int irq, void *dev_id)
{
    disable_irq_nosync(g_wdt_int_no);

    dump_fetal("wdt irq,irq:0x%x\n",irq);
    if(g_wdt_reboot_func)
    {
        g_wdt_reboot_func();
    }
    return IRQ_HANDLED;
}

void bsp_wdt_irq_disable(WDT_CORE_ID core_id)
{
    if(g_wdt_int_no)
    {
        disable_irq_nosync(g_wdt_int_no);
    }
}

void bsp_wdt_irq_enable(WDT_CORE_ID core_id)
{
    if(g_wdt_int_no)
    {
        enable_irq(g_wdt_int_no);
    }
}

int bsp_wdt_register_hook(WDT_CORE_ID core_id, void *func)
{
    unsigned int flag = 0;

    struct device_node *node = NULL;

    /*austin 版本上CP的看门狗中断注册在AP,由dump模块负责去存储*/
    node = of_find_compatible_node(NULL, NULL, "hisilicon,watchdog_app");
    if(node)
    {
        (void)of_property_read_u32(node, "flag", &flag);
        if(flag)
        {
            g_wdt_int_no = (u32)irq_of_parse_and_map(node, 0);
            if (BSP_OK != request_irq(g_wdt_int_no, watchdog_mdm_int, 0, "watchdog mdm", NULL))
            {
                dump_error("watchdog mdm int err\n");
            }
        }
    }
    else
    {
        return BSP_ERROR;
    }
    g_wdt_reboot_func = func;
    return BSP_OK;
}


