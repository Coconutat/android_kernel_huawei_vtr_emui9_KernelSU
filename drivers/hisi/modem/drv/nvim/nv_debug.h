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

#ifndef _NV_DEBUG_H_
#define _NV_DEBUG_H_


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/rtc.h>
#ifdef BSP_CONFIG_PHONE_TYPE
#include "../../adrv/adrv.h"
#endif
#include <bsp_nvim.h>
#include "nv_comm.h"

#define nv_printf(fmt, ...) \
    bsp_trace(BSP_LOG_LEVEL_ERROR,BSP_MODU_NV,"[nv]:<%s %d> "fmt, __FUNCTION__,__LINE__, ##__VA_ARGS__)
#define nv_warn_printf(fmt, ...) \
    bsp_trace(BSP_LOG_LEVEL_WARNING,BSP_MODU_NV,"[nv]:<%s %d> "fmt, __FUNCTION__,__LINE__, ##__VA_ARGS__)
#define nv_info_printf(fmt, ...) \
        bsp_trace(BSP_LOG_LEVEL_INFO,BSP_MODU_NV,"[nv]:<%s %d> "fmt, __FUNCTION__,__LINE__, ##__VA_ARGS__)
#define nv_debug_printf(fmt, ...) \
    bsp_trace(BSP_LOG_LEVEL_DEBUG,BSP_MODU_NV,"[nv]:<%s %d> "fmt, __FUNCTION__,__LINE__, ##__VA_ARGS__)
#define nv_error_printf(fmt, ...) \
    bsp_trace(BSP_LOG_LEVEL_ERROR,BSP_MODU_NV,"[nv]:<%s %d> "fmt, __FUNCTION__,__LINE__, ##__VA_ARGS__)
#define nv_printf_info(fmt, ...) \
        bsp_trace(BSP_LOG_LEVEL_ERROR,BSP_MODU_NV, fmt, ##__VA_ARGS__)

#define NV_DUMP_SIZE     (4*1024)



enum nv_debug_delta_e
{
    NV_DEBUG_DELTA_WRITE_FILE   = 0,    /*写文件系统最大时长*/
    NV_DEBUG_DELTA_GET_IPC      = 1,    /*获取IPC锁最大时长*/
    NV_DEBUG_DELTA_BUTT
};
enum nv_debug_state{
    NV_DEBUG_WRITEEX_START = 1,
    NV_DEBUG_WRITEEX_GET_IPC_START = 2,
    NV_DEBUG_WRITEEX_GET_IPC_END = 3,
    NV_DEBUG_WRITEEX_GIVE_IPC = 4,
    NV_DEBUG_WRITEEX_MEM_START = 5,
    NV_DEBUG_WRITEEX_FILE_START = 6,
    NV_DEBUG_FLUSH_START = 7,
    NV_DEBUG_FLUSH_END = 8,
    NV_DEBUG_REQ_FLUSH_START = 9,
    NV_DEBUG_REQ_FLUSH_END = 10,
    NV_DEBUG_FLUSHEX_START = 11,
    NV_DEBUG_FLUSHEX_OPEN_START = 12,
    NV_DEBUG_FLUSHEX_OPEN_END = 13,
    NV_DEBUG_FLUSHEX_GET_PROTECT_SEM_START = 14,
    NV_DEBUG_FLUSHEX_GET_PROTECT_SEM_END = 15,
    NV_DEBUG_FLUSHEX_GIVE_IPC = 16,
    NV_DEBUG_FLUSHEX_GIVE_SEM = 17,
    NV_DEBUG_FLUSHEX_WRITE_FILE_START = 18,
    NV_DEBUG_FLUSHEX_WRITE_FILE_END = 19,
    NV_DEBUG_WRITEEX_END = 20,
    NV_DEBUG_RECEIVE_ICC = 21,
    NV_DEBUG_SEND_ICC = 22,
    NV_DEBUG_READ_ICC = 23,
    NV_DEBUG_REQ_ASYNC_WRITE = 24,
    NV_DEBUG_REQ_REMOTE_WRITE = 25,
    NV_DEBUG_BUTT
};

enum
{
    NV_DEBUG_SWITCH_TYPE_FOREVER = 0,       /*debug开关设置后从现在开始一直生效*/
    NV_DEBUG_SWITCH_TYPE_TEMPORARY = 1,     /*debug开关设置后仅本次生效，重启失效*/
    NV_DEBUG_SEITCH_TYPE_BUTT
};

/*nv debug info*/
struct nv_global_debug_stru
{
    u32 callnum;
    u32 reseved1;
    u32 reseved2;
    u32 reseved3;
    u32 reseved4;
};

typedef struct nv_queue_elemnt_str
{
    u32 state;
    u32 slice;
}nv_queue_elemnt;

typedef struct
{
    u32 maxNum;
    u32 front;
    u32 rear;
    u32 num;
    nv_queue_elemnt data[1];
} nv_queue_t;

typedef struct
{
    u32 state;                  /*取值取自enum nv_debug_state*/
    char *info;                 /*对应state的状态*/
}debug_table_t;


struct nv_debug_stru
{
    nv_queue_t *            nv_dump_queue;
    debug_table_t *         write_debug_table;
    union debug_ctrl_union  debug_ctrl;
    u32                     delta_time[NV_DEBUG_DELTA_BUTT];
};

enum nv_debug_invalid_file_sta{
    INVALID_FILE_NO_REMOVE = 1,     /* 表示无法删除的非白名单文件 */
    INVALID_FILE_NO_STAT,       /* 表示无法获取文件信息文件 */
    INVALID_FILE_NO_ODIR,       /* 表示无法打开的目录文件 */
    INVALID_FILE_NO_RDIR        /* 表示无法读取的目录文件 */
};

/*only phone and beta can store nv data*/
#define nv_support_debug_info       (DUMP_PHONE == dump_get_product_type())
/*nv debug 开关文件路径*/
#define NV_DEBUG_SWICH_PATH        "modem_log/drv/nv/debug_switch.txt"
#define NV_DEBUG_CTRL_VALUE        ((((nv_global_info_s*)NV_GLOBAL_INFO_ADDR))->debug_ctrl)

extern NV_DEBUG_CFG_STRU              g_nv_debug_cfg;
extern u32 g_debug_check_flag;

static inline u32 nv_debug_is_save_image(void)
{
    return (u32)g_nv_debug_cfg.save_image;
}

static inline u32 nv_debug_is_save_ddr(void)
{
    return (u32)g_nv_debug_cfg.save_ddr;
}

static inline u32 nv_debug_is_save_bakup(void)
{
    return (u32)g_nv_debug_cfg.save_bakup;
}

static inline u32 nv_debug_is_resume_img(void)
{
    return (u32)g_nv_debug_cfg.resume_img;
}

static inline u32 nv_debug_is_resume_bakup(void)
{
    return (u32)g_nv_debug_cfg.resume_bakup;
}

static inline u32 nv_debug_product(void)
{
    return (u32)g_nv_debug_cfg.product;
}
static inline u32 nv_debug_is_print_in_write(void)
{
    return (u32)NV_DEBUG_CTRL_VALUE.ctrl_bits.write_print_ctrl;
}

u32  nv_debug_init(void);
u32  nv_debug_is_save_image(void);
u32  nv_debug_is_save_ddr(void);
u32  nv_debug_is_save_bakup(void);
u32  nv_debug_is_resume_img(void);
u32  nv_debug_is_resume_bakup(void);
u32  nv_debug_product(void);
u32  nv_debug_is_print_in_write(void);
bool nv_debug_is_reset(void);
u32  nv_debug_store_ddr_data(void);
u32  nv_debug_store_file(char * src);
void nv_debug_record(u32 current_state);
u32  nv_debug_switch(debug_ctrl_union_t debug_switch, u32 type);
void nv_debug_record_delta_time(u32 type, u32 start, u32 end);
void nv_debug_print_dump_queue(void);
void nv_debug_print_delta_time(void);
void nv_debug(u32 type, u32 reseverd1, u32 reserved2, u32 reserved3, u32 reserved4);
void nv_debug_set_reset(u32 reset);
void nv_help(u32 type);
void nv_show_fastboot_err(void);
void nv_show_crc_status(void);
u32  nv_show_item_info(u16 itemid);
void nv_show_ref_info(u32 min, u32 max);
void nv_show_ddr_info(void);
void nv_record(char* fmt,...);
void nv_get_current_sys_time(struct rtc_time *tm);
void nv_debug_is_bak_resume_nv(u16 itemid);
u32 nv_debug_chk_invalid_type(const s8 * path, u32 invalid_type);
void nv_debug_set_invalid_type(const s8 * path, u32 invalid_type);
void nv_debug_clear_invalid_type(void);
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /*_NV_DEBUG_H_*/



