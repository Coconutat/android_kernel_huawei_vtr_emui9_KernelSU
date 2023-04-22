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
#ifndef __BSP_OM_H__
#define __BSP_OM_H__

#ifdef __cplusplus
extern "C" {
#endif

#if defined(__KERNEL__)
#include <linux/kernel.h>
#include <asm/string.h>
#endif

#include "product_config.h"
#include "osl_common.h"
#include <stdarg.h>
#include "drv_comm.h"

#if defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
#include "sre_shell.h"
#endif

/*****************************************************************************
  2 宏定义
*****************************************************************************/
#define BSP_OM_MODU_ID                     THIS_MODU_ID


/*SOCP头SID:用于HiStudio检测与Modem的连接状态*/
#define BSP_SOCP_SID_DIAG_SERVER          (0x2) /* DIAG Service，配置诊断消息输出*/

/*SOCP头 usSSId:低四位保留,高四位;SSID指明App CPU*/
#define BSP_SOCP_HIGH4BIT_SSID_ACPU       (0x1)
#define BSP_SOCP_HIGH4BIT_SSID_MCPU       (0x2)      /*Modem*/
#define BSP_SOCP_HIGH4BIT_SSID_M3         (0xA)      /* M3 */

/*SOCP头 ucServiceSessionId:标识Service与Client之间的连接*/
#define BSP_SOCP_SERVER_SESSION_ID        (0x1) /* Service Session ID固定为1*/

/*SOCP头 ucMsgType: 表示消息类型，高6比特保留，设为0，低2比特取值如下*/
#define BSP_SOCP_MSG_TYPE_REQ             (0x1) /* 请求*/
#define BSP_SOCP_MSG_TYPE_CNF             (0x2) /* 响应*/
#define BSP_SOCP_MSG_TYPE_IND             (0x3) /* 上报*/


/* 结构化ID里面的一级（高四位(28~31)）字段*/
#define BSP_STRU_ID_28_31_GROUP_MSP  (0x1)
#define BSP_STRU_ID_28_31_GROUP_BSP    (0x9)

/* 结构化ID里面的二级（高四位(16_23)）字段,对应的一级字段是BSP_STRU_ID_28_31_GROUP_BSP*/
#define BSP_STRU_ID_16_23_BSP_CMD         (0x1) /* BSP内部相关命令*/
#define BSP_STRU_ID_16_23_BSP_CPU         (0x2) /* CPU占用率数据上报*/
#define BSP_STRU_ID_16_23_BSP_PRINT       (0x3) /* BSP 打印信息*/
#define BSP_STRU_ID_16_23_BSP_MEM         (0x4) /* BSP内存TRACEA上报*/
#define BSP_STRU_ID_16_23_BSP_TASK        (0x5) /* 系统任务(包括中断)调度信息上报*/
#define BSP_STRU_ID_16_23_BSP_CMD_IND   (0x5) /* 底软二进制上报，为了适配HSO，id定义为5*/
#define BSP_STRU_ID_16_23_BSP_INT_LOCK    (0x6) /* 系统锁中断调度信息上报*/
#define BSP_STRU_ID_16_23_BSP_AMON        (0x7) /* AXI Monitor时间窗监控结束上报 */

#define CMD_BSP_SYSVIEW_IND_ACORE           (0x5310)
#define CMD_BSP_SYSVIEW_IND_CCORE           (0x5311)

#ifdef  BSP_CONFIG_PHONE_TYPE
#define DUMP_DMESG_SIZE                     (0x10000)
#else
#define DUMP_DMESG_SIZE                     (0x4000)
#endif
/*****************************************************************************
  2 枚举定义
*****************************************************************************/

/* 模块定义 */

typedef enum
{
    BSP_SYSVIEW_MEM_TRACE       = 1,
    BSP_SYSVIEW_CPU_INFO        = 2,
    BSP_SYSVIEW_TASK_INFO       = 3,
    BSP_SYSVIEW_INT_LOCK_INFO   = 4,
    BSP_SYSVIEW_SWT_ALL         = 0xf,
    BSP_SYSVIEW_SWT_MAX         /* 边界值 */
} bsp_sysview_type_e;


/*****************************************************************************
  2 函数声明
*****************************************************************************/

/* 打印接口，输出trace到HSO*/
#ifdef ENABLE_BUILD_OM

/* 记录异常到文件*/
void error_log(char *fmt ,...);
int dmesg_write(const char* buffer, const unsigned len);

typedef int (*print_hook)(char *out_char);
void balongv7r2_uart_register_hook(print_hook hook);

void uart_register_hook(print_hook hook);

void bsp_socp_chan_enable(void);

s32 bsp_om_server_init(void);

u32 om_timer_get(void);

u32 om_timer_tick_get(void);

void bsp_om_set_hso_conn_flag(u32 flag);

u32 sysview_get_all_task_name(void *p_task_stru,u32 param_len);

u32  bsp_sysview_swt_set(bsp_sysview_type_e set_type,u32 set_swt,u32 period);

int dmesg_init(void);

/* drx mntn callback */
void bsp_log_bin_ind(s32 str_id, void * ind_data, u32 ind_data_size);

void bsp_print2dmsg(char *fmt, ...);

#else
static void inline error_log(char *fmt ,...)
{
    return;
}


static int inline dmesg_write(const char* buffer, const unsigned len)
{
    return 0;
}

static void inline bsp_socp_chan_enable(void)
{
    return;
}

typedef int (*print_hook)(char *out_char);

static s32 inline bsp_om_server_init(void)
{
    return 0;
}

static u32 inline om_timer_get(void)
{
    return 0;
}
static u32 inline om_timer_tick_get(void)
{
    return 0;
}
static void inline bsp_log_bin_ind(s32 str_id, void * ind_data, u32 ind_data_size)
{
    return;
}
static inline int dmesg_init(void)
{
    return 0;
}

static inline void bsp_print2dmsg(char* fmt, ...)
{
    return;
}


#endif

#ifdef __cplusplus
}
#endif


#endif




