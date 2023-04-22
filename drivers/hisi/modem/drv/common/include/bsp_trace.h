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
#ifndef __BSP_TRACE_H__
#define __BSP_TRACE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <product_config.h>
#include "drv_comm.h"

/*****************************************************************************
  2 枚举定义
*****************************************************************************/

/* 模块定义 */
typedef enum _bsp_module_e
{
    BSP_MODU_ICC = 1,
    BSP_MODU_IPF,
    BSP_MODU_UDI,
    BSP_MODU_DLOAD,
    BSP_MODU_SYNC,
    BSP_MODU_OM,
    BSP_MODU_DUMP,
    BSP_MODU_SOCP,
    BSP_MODU_OMS,
    BSP_MODU_LOG,
    BSP_MODU_SYSVIEW,
    BSP_MODU_RFILE,
    BSP_MODU_MBX,
    BSP_MODU_OTHER_CORE,
    BSP_MODU_SCI,
    BSP_MODU_NV,
    BSP_MODU_SOFTTIMER,
    BSP_MODU_HARDTIMER,
    BSP_MODU_MIPI,
    BSP_MODU_BBP,
    BSP_MODU_DPM,
    BSP_MODU_IPC,
    BSP_MODU_DFC,
    BSP_MODU_I2C,
    BSP_MODU_RTC,
    BSP_MODU_HI6551_RTC,
    BSP_MODU_MEM,
    BSP_MODU_EARLYSUSPEND,
    BSP_MODU_AMON,
    BSP_MODU_TEST,
    BSP_MODU_LCD,
    BSP_MODU_EMI,
    BSP_MODU_VERSION,
    BSP_MODU_SPI,
    BSP_MODU_PMU,
    BSP_MODU_CLK,
    BSP_MODU_REGULATOR,
    BSP_MUDU_CPUFREQ,
    BSP_MUDU_WDT,
    BSP_MODU_WAKELOCK,
    BSP_MODU_VIC,
    BSP_MODU_CSHELL,
    BSP_MODU_CIPHER,
    BSP_MODU_ACC,
    BSP_MODU_SECURITY,
    BSP_MODU_AT_UART,
    BSP_MODU_EDMA,
    BSP_MODU_UTRACE,
    BSP_MODU_ABB,
    BSP_MODU_DSP,
    BSP_MODU_HKADC,
    BSP_MODU_GPIO,
    BSP_MODU_IOS,
    BSP_MODU_EFUSE,
    BSP_MODU_DDM,
    BSP_MODU_KEY,
    BSP_MODU_ANTEN,
    BSP_MODU_RSE,
    BSP_MODU_TSENSOR,
    BSP_MODU_TEMPERATURE,
    BSP_MODU_PM,
    BSP_MODU_COUL,
    BSP_MODU_HWADP,
    BSP_MODU_ADP_DPM,
    BSP_MODU_M3_CPUFREQ,
    BSP_MODU_LED,
    BSP_MODU_NAND,
    BSP_MODU_HIFIMBX,
    BSP_MODU_ONOFF,
    BSP_MODU_RESET,
    BSP_MODU_TUNER,
    BSP_MODU_HW_SPINLOCK,
    BSP_MODU_MEMREPAIR,
    BSP_MODU_DUAL_MODEM,
    BSP_MODU_PASTAR,
    BSP_MODU_HI6559_RTC,
    BSP_MODU_HSUART,
    BSP_MODU_PARF,
    BSP_MODU_SC,
    BSP_MODU_OF,
    BSP_MODU_PSAM,
    BSP_MODU_SYSCTRL,
    BSP_MODU_STRESS,
    BSP_MODU_L2CACHE,
    BSP_MODU_PM_OM,
    BSP_MODU_PAN_RPC,
    BSP_MODU_CROSS_MIPI,
    BSP_MODU_WATCHPOINT,
    BSP_MODU_PDLOCK,
    BSP_MODU_XMBX,
    BSP_MODU_GCOV,
    BSP_MODU_SYS_BUS,
    BSP_MODU_LOADPS,
    BSP_MODU_CDSP,
    BSP_MODU_TLDSP,
    BSP_MODU_DSPDVS,
    BSP_MODU_RSRACC,
    BSP_MODU_THERMAL_UP,
	BSP_MODU_SYS_PMU,
	BSP_MODU_MPERF,
    BSP_MODU_DSPDFS,
    BSP_MODU_OCBC,
    BSP_MODU_DLOCK,
	BSP_MODU_AVS,
	BSP_MODU_NOC,
	BSP_MODU_HOTPLUG,
	BSP_MODU_CPM,
    BSP_MODU_ALL,   /* 代表所有的模块 */
    BSP_MODU_MAX = 128    /* 边界值 */
} bsp_module_e;


typedef enum _bsp_log_level_e
{
    BSP_LOG_LEVEL_DEBUG = 0,  /* 0x0:debug-level                                  */
    BSP_LOG_LEVEL_INFO,      /* 0x1:informational                                */
    BSP_LOG_LEVEL_NOTICE,     /* 0x2:normal but significant condition             */
    BSP_LOG_LEVEL_WARNING,    /* 0x3:warning conditions                           */
    BSP_LOG_LEVEL_ERROR,      /* 0x4:error conditions                             */
    BSP_LOG_LEVEL_CRIT,       /* 0x5:critical conditions                          */
    BSP_LOG_LEVEL_ALERT,      /* 0x6:action must be taken immediately             */
    BSP_LOG_LEVEL_FATAL,      /* 0x7:just for compatibility with previous version */
    BSP_LOG_LEVEL_MAX         /* 边界值 */
} bsp_log_level_e;


/*****************************************************************************
  2 函数声明
*****************************************************************************/
#ifdef ENABLE_BUILD_OM
void bsp_trace(bsp_log_level_e log_level, bsp_module_e mod_id, char *fmt,...);

unsigned int bsp_mod_level_set(bsp_module_e  mod_id ,unsigned int print_level);

unsigned int bsp_log_module_cfg_get(bsp_module_e mod_id);

#else
static inline void bsp_trace(bsp_log_level_e log_level, bsp_module_e mod_id, char *fmt,...){return ;}

static inline unsigned int bsp_mod_level_set(bsp_module_e  mod_id ,unsigned int print_level){return 0;}

static inline unsigned int bsp_log_module_cfg_get(bsp_module_e mod_id){return 0;}

#endif


#ifdef __cplusplus
}
#endif

#endif




