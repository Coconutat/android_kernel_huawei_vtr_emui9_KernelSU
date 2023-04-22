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


#ifndef _NV_COMM_H_
#define _NV_COMM_H_


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif


#include <asm/io.h>
#include <linux/gfp.h>
#include <linux/mm.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/wakelock.h>
#include <linux/dma-mapping.h>
#include <product_config.h>
#include <osl_thread.h>
#include <osl_common.h>
#include <osl_sem.h>
#include <osl_spinlock.h>
#include <osl_list.h>
#include <osl_bio.h>
#include <osl_barrier.h>
#include <bsp_nvim.h>
#include <bsp_shared_ddr.h>
#include <bsp_pm_om.h>
#include <bsp_slice.h>
#include <bsp_ipc.h>
#include <bsp_icc.h>
#include <bsp_trace.h>
#include "drv_nv_id.h"
#include "msp_nv_id.h"
#include "drv_nv_def.h"
#include "nv_debug.h"
#include "nv_partition_upgrade.h"
#include "mdrv_rfile_common.h"

extern struct nv_global_ctrl_info_stru  g_nv_ctrl;
extern struct nv_global_ctrl_stru * g_flash_emmc_info_ptr;

#define nv_malloc(a)     kmalloc(a,GFP_KERNEL)
#define nv_free(p)       kfree(p)

#define nv_taskdelay(n)             msleep(n)
#define nv_flush_cache(ptr, size)   mb()

#define nv_debug_trace(pdata,len)\
    if(g_nv_ctrl.debug_sw == true)\
    {\
        u32 marcro_i;\
        nv_printf("<%s> len :0x%x\n",__FUNCTION__,len);\
        for(marcro_i=0; marcro_i<len; marcro_i++)\
        {\
            nv_printf("%02x ",*((u8*)pdata+marcro_i));\
        }\
        nv_printf("\n");\
    };

#define nv_pm_trace(icc_req)\
    if(g_nv_ctrl.pmSw == true)\
    {\
        printk(KERN_ERR"[C SR]NV slice0x%x,ID:0x%x\n",icc_req.send_slice,icc_req.data.itemid);\
        bsp_pm_log_type(PM_OM_NVA, 0, (u32)sizeof(icc_req), (void *)&icc_req);\
        g_nv_ctrl.pmSw = false;\
    };


#define NV_ID_SYS_MAX_ID          0xd1ff
#define NV_ID_SYS_MIN_ID          0xd100


#define nv_spin_lock(nvflag,lock) \
do{\
    spin_lock_irqsave(&g_nv_ctrl.spinlock, nvflag);\
    bsp_ipc_spin_lock(lock);\
}while(0)

#define nv_spin_unlock(nvflag,lock) \
do{\
    bsp_ipc_spin_unlock(lock);\
    spin_unlock_irqrestore(&g_nv_ctrl.spinlock, nvflag);\
}while(0)

#define IPC_SME_TIME_OUT   (1000)


/*睡眠唤醒状态*/
enum
{
    NV_WAKEUP_STATE         =0,
    NV_SLEEP_STATE          =1
};

/*操作进行与空闲状态*/
enum
{
    NV_IDLE_STATE           =0,
    NV_OPS_STATE            =1
};

#define NV_MAX_WAIT_TICK             (50000)


#define NV_ICC_BUF_LEN               64
#define NV_ICC_SEND_COUNT            5

enum
{
    NV_MODE_FACTORY     = 1,
    NV_MODE_USER        = 2,
    NV_MODE_BUTT
};

enum
{
    NV_PRODUCT_PHONE    = 0,
    NV_PRODUCT_MBB      = 1,
    NV_PRODUCT_BUTT
};
enum
{
    NV_IMG,
    NV_SC_CK_FILE,
	NV_SC_DK_FILE,
	NV_SC_AK_FILE,
	NV_SC_PI_FILE,
	NV_SC_IMEI_I0_BIN,
	NV_SC_IMEI_I1_BIN,
	NV_SC_IMEI_I2_BIN,
	NV_SC_CK_SIGN,
	NV_SC_DK_SIGN,
	NV_SC_AK_SIGN,
	NV_SC_PI_SIGN,
	NV_SC_IMEI_I0_HASH,
	NV_SC_IMEI_I1_HASH,
	NV_SC_IMEI_I2_HASH,
    NV_MAX
};
typedef struct nv_path_info_stru
{
    s8* root_dir;
    s8  file_path[NV_MAX][100];
}nv_path_info;


typedef struct nv_flush_list_node_stru
{
    u16  itemid;     /* NV ID */
    u16  modemid;     /* NV Length */
    struct list_head stList;
}nv_flush_node_s;

typedef struct _nv_wr_req
{
    u32  itemid;
    u32  modemid;
    u32  offset;
    u8 * pdata;
    u32  size;
}nv_wr_req;


typedef struct _nv_rd_req
{
    u32  itemid;
    u32  modemid;
    u32  offset;
    u8 * pdata;
    u32  size;
}nv_rd_req;

typedef struct _nv_revert_file_s
{
    FILE*   fp;
    u8*     global_info;
    u8*     ctrl_data;
}nv_revert_file_s;


/*global info type define*/
struct nv_global_ctrl_info_stru
{
    u8 nv_icc_buf[NV_ICC_BUF_LEN];
    osl_sem_id rw_sem;      /*与IPC锁同时使用保证本核访问的互斥*/
    osl_sem_id cc_sem;      /*lock the core comm*/
    osl_sem_id task_sem;
    osl_sem_id suspend_sem;
    u32 mid_prio;    /*nv priority*/
    NV_SELF_CTRL_STRU nv_self_ctrl;
    u32 debug_sw;
    u32 revert_count;
    u32 revert_search_err;
    u32 revert_len_err;
    spinlock_t spinlock;
    struct task_struct *task_id;
    nv_global_info_s* shared_addr;
    u32 icc_cb_count;
    u32 task_proc_count;
    struct list_head stList;
    bool statis_sw;
    struct wake_lock wake_lock;
    u32 opState;
    u32 pmState;
    bool pmSw;
    u32 initStatus;
    u32 acore_flush_req;
    u32 acore_flush_count;

    osl_sem_id nv_list_sem;         /*用于保护nv_list的信号量*/
    struct list_head  nv_list;      /*存储待写入的list的buf*/

};

u32 nv_ipc_sem_take(u32 sem, u32 timeout);
void nv_ipc_sem_give(u32 sem);
u32 nv_write_to_mem(nv_wr_req *wreq, nv_item_info_s *item_info);
u32 nv_read_from_mem(nv_rd_req *rreq, nv_item_info_s *item_info);
u32 nv_icc_chan_init(u32 fun_id);
s32 nv_icc_msg_proc(u32 chanid, u32 len, void * pdata);
u32 nv_icc_send(u32 chanid,u8* pdata,u32 len);
u32 nv_get_file_len(FILE * fp);
u32 nv_read_file(s8* path, u32 offset, u8* ptr, u32* len);
u32 nv_write_to_file(nv_wr_req *wreq, nv_item_info_s* item_info);
u32 nv_read_from_file(FILE * fp, u8* ptr, u32 offset, u32 file_len);
u32 nv_read_ctrl_from_file(const s8 * path);
u32 nv_read_ctrl_from_upfile(void);
u32 nv_revertEx(const s8* path);
u32 nv_revert_data(s8*path,const u16 * revert_data, u32 len, u32 crc_mark);
u32 nv_get_key_data(const s8 * path, u32 itemid, void * buffer, u32 len);
u32 nv_imei_data_comp(const s8 * path);
void nv_create_flag_file(const s8* path);
void nv_delete_flag_file(const s8* path);
bool nv_flag_file_isExist(const s8* path);
void nv_file_flag_check(void);
u32  nv_data_writeback(void);
u32  nv_load_err_proc(void);
u32  nv_upgrade_revert_proc(void);
bool nv_check_file_validity(s8 * filePath, s8 *flagPath);
bool nv_read_right(void);
bool nv_write_right(void);
u32  nv_init_file_info(nv_ctrl_info_s * major_info, nv_global_info_s * base_info);
bool nv_dload_file_check(void);
void nv_modify_print_sw(u32 arg);
bool nv_isSysNv(u16 itemid);
s32  nv_modify_pm_sw(s32 arg);
u32  nv_resume_item(nv_item_info_s *item_info, u32 itemid, u32 modemid);
u32  nv_resume_ddr_from_img(void);
u32  nv_write_to_mem_revert(u8* pdata,u32 size,u32 file_id,u32 offset);
void nv_flushListInit(void);
u32  bsp_nvm_reload(void);
u32  bsp_nvm_buf_init(void);
u32  nv_check_item_len(u32 start, u32 end);
u32  nv_test_for_local_list(u32 min_priority, u32 max_priority, u32 max_num);
u32  nv_flushItem(nv_flush_item_s *flush_item);
u32  nv_copy_img2backup(void);
u32  nv_upgrade_revert_proc(void);
u32  nv_set_resume_mode(u32 mode);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif



#endif /*_NV_COMM_H_*/

