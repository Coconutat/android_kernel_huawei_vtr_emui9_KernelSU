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

#ifndef __BSP_OM_LOG_H__
#define __BSP_OM_LOG_H__

#include "bsp_trace.h"
#include "drv_comm.h"
#include <linux/miscdevice.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <bsp_modem_log.h>

#define  BSP_DIAG_IND_DATA_MAX_LEN         (4096)
#define  BSP_PRINT_BUF_LEN                 (256)

/* log缓冲区满阈值，buffer空闲空间小于200字符，认为buffer已满 */
#define LOG_BUFFER_FULL_THRESHOLD   200
/* log task调度间隔，2s */
#define LOG_TASK_SCHED_TIME         200
/* log buffer满，输出提示 */
#define LOG_DROPPED_MESSAGE         "log buffer full, data dropped\n"
/* ccore log设备名 */
#define CCORE_LOG_DEV_NAME          "ccorelog"
/* mcore log设备名 */
#define MCORE_LOG_DEV_NAME          "mcorelog"

/*错误码定义*/
#define BSP_ERR_LOG_BASE                (int)(0x80000000 | (BSP_DEF_ERR(BSP_MODU_LOG, 0)))
#define BSP_ERR_LOG_INVALID_MODULE      (BSP_ERR_LOG_BASE + 0x1)
#define BSP_ERR_LOG_INVALID_LEVEL       (BSP_ERR_LOG_BASE + 0x2)
#define BSP_ERR_LOG_INVALID_PARAM       (BSP_ERR_LOG_BASE + 0x3)
#define BSP_ERR_LOG_NO_BUF              (BSP_ERR_LOG_BASE + 0x4)
#define BSP_ERR_LOG_SOCP_ERR            (BSP_ERR_LOG_BASE + 0x5)

#ifdef ENABLE_BUILD_OM
#define modem_om_log_err(fmt, ...)        pr_err("[om log]: " fmt, ##__VA_ARGS__)
#define modem_om_log_debug(fmt, ...)      //pr_err("[om log]: " fmt, ##__VA_ARGS__)
#endif

typedef struct
{
    void*                     addr;
    u16                     is_edle;
    u16                     is_valible;
}bsp_log_node_ctrl_s;

typedef struct
{
    u32 print_level;
} bsp_log_swt_cfg_s;

typedef struct
{
    struct log_mem log_info;
    u32     logged_chars;   /* number of chars in log buffer */
    u32     w_mark;         /* water mark, if logged chars exceed w_mark, ccore send IPC to acore */
    u32     reserved;       /* reserved for 32-byte align */
    u8 *    log_buf;        /* data area */
}log_mem_stru;

typedef struct
{
    struct miscdevice	misc;       /* misc device */
    wait_queue_head_t	wq;         /* wait queue */
    u32                 in_use;     /* whether device is in use */
    u32                 sleep_flag; /* whether app is sleep */
    spinlock_t          lock;
  	struct list_head	logs;
    log_mem_stru      * log_mem;
}logger_info_stru;

#ifdef ENABLE_BUILD_OM
struct new_om_log
{
	struct log_usr_info  ccore_log_info;
	struct log_usr_info  mcore_log_info;
};
struct new_om_log g_om_log;
#endif

typedef int (*print_report_hook)(u32 module_id, u32 level, char* print_buff);
extern print_report_hook g_bsp_print_hook;
u32 bsp_log_module_cfg_get(bsp_module_e mod_id);

#endif

