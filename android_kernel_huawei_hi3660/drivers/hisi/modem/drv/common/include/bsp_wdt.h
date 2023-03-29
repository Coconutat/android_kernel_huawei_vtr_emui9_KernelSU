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
#ifndef __BSP_WDT_H__
#define __BSP_WDT_H__

#include <soc_interrupts.h>
#include <soc_clk.h>
#include <bsp_memmap.h>
#include <bsp_trace.h>
#include <bsp_hardtimer.h>
#include <drv_nv_id.h>
#include <drv_nv_def.h>
#include <bsp_nvim.h>
#include <bsp_softtimer.h>



#define  wdt_err(fmt,...)		(bsp_trace(BSP_LOG_LEVEL_ERROR, BSP_MUDU_WDT, "[wdt]:"fmt, ##__VA_ARGS__))
#define  wdt_pinfo(fmt,...)		(bsp_trace(BSP_LOG_LEVEL_INFO, BSP_MUDU_WDT, "[wdt]:"fmt, ##__VA_ARGS__))
#define  wdt_debug(fmt,...)		(bsp_trace(BSP_LOG_LEVEL_DEBUG, BSP_MUDU_WDT, "[wdt]:"fmt,##__VA_ARGS__))

/* 整理后 */
#define WDT_NUM              			(2)
#define WDT_INDEX                		(0)
#define TIMER_INDEX              		(1)
#define WDT_RESET_TIMEOUT				(60)
#define WDT_OK              			(0)
#define WDT_ERROR              			(-1)
#define WDT_NULL            			(void*)0
#define WDT_RES_NUM              		(4)

/*硬狗使用*/
#define WDT_UNLOCK               		(0x1ACCE551)
#define WDT_LOCK                 		(0x0)
#define WDT_COUNT_DEFAULT       		(0xf0000)
#ifdef CONFIG_HI3630_CCORE_WDT
#define WDT_RST_INT_EN 					(0x1)
#else
#define WDT_RST_INT_EN 					(0x3)
#endif
#define WDT_DEF_CLK_FREQ         		(32768)                  /* 32khz */
#define WDT_KEEPALIVE_TIME				(15)

#define HARD_WATCHDOG					(0xaaaaaaaa)

#ifdef __KERNEL__
#define WATCHDOG_TIMEOUT_SEC			(32768 * 30)
#define WDT_HI_TIMER_CLK				(32768)
//4fe1fe00
#define STOP_WDT_TRACR_RUN_FLAG			(((SRAM_SMALL_SECTIONS * )((unsigned long)SRAM_BASE_ADDR + SRAM_OFFSET_SMALL_SECTIONS))->SRAM_WDT_AM_FLAG)
#elif defined( __OS_VXWORKS__)||defined( __OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
#define WATCHDOG_TIMEOUT_SEC				(32768 * 30)
#define WDT_HI_TIMER_CLK									(32768)
#define STOP_WDT_TRACR_RUN_FLAG			(((SRAM_SMALL_SECTIONS * )((unsigned long)SRAM_BASE_ADDR + SRAM_OFFSET_SMALL_SECTIONS))->SRAM_WDT_CM_FLAG)
#elif defined(__CMSIS_RTOS)
#endif /* end of __KERNEL__ */


#ifndef BSP_WDT_SUSPEND_TIMEROUT
#define BSP_WDT_SUSPEND_TIMEROUT		(30)
#endif
#ifndef BSP_WDT_SOFTTIMEROUT
#define	 BSP_WDT_SOFTTIMEROUT			(1000)
#endif


#define BSP_SW_WDT_PERIOD       		(1500)


struct wdt_info{
		s32  lowtaskid;
		s32  init;
		s32  enable;
		u32  task_delay_value;
		u32  wdt_timeout;
		u32  wdt_suspend_timeout;
#if defined(__KERNEL__) || defined(__OS_VXWORKS__) || defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
		struct bsp_hardtimer_control my_timer;
#endif
		struct softtimer_list      wdt_timer_list;
		u32 periphid;
		u32 mdm_wdt_num;
};

typedef enum _WDT_CORE_ID{
	WDT_CCORE_ID,
	WDT_ACORE_ID,
	WDT_SRE_CORE_ID,
	WDT_MCORE_ID,
	MAX_WDT_CORE_ID,
}WDT_CORE_ID;

typedef void(*wdt_timeout_cb)(void);/*A、C核共用*/

s32 bsp_wdt_register_hook(WDT_CORE_ID core_id, void *func);
s32 bsp_wdt_unregister_hook(WDT_CORE_ID core_id);
s32 bsp_wdt_reboot_register_hook(void *hook);
s32  bsp_wdt_reboot_unregister_hook(void);
void bsp_wdt_irq_disable(WDT_CORE_ID core_id);
void bsp_wdt_irq_enable(WDT_CORE_ID core_id);
s32 bsp_watchdog_init(void);
#if defined( __OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
int bsp_wdt_start(u32 core_id);
int bsp_wdt_stop(unsigned int cpu_id);
u32 bsp_wdt_task_suspend(u32 cpu_id);
u32 bsp_wdt_task_resume(u32 cpu_id);
#endif
#if defined(__KERNEL__)
s32 bsp_wdt_start(void);

s32 bsp_wdt_stop(void);
s32 bsp_wdt_keepalive(void);

s32 bsp_wdt_get_timeleft(u32 *ptimeleft);

s32 bsp_wdt_set_timeout(u32 timeout_s);
#endif
#endif /*__BSP_WDT_H__*/
