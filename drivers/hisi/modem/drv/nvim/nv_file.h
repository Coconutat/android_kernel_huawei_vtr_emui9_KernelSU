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

#ifndef _NV_FILE_H_
#define _NV_FILE_H_


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include <linux/kernel.h>
#include <product_config.h>
#include <mdrv.h>
#include <osl_types.h>
#include "nv_comm.h"
#include "bsp_nvim.h"

#ifndef NAND_OK
#define NAND_OK    0
#endif


enum
{
    NV_FILE_DLOAD = 0,
    NV_FILE_DLOAD_CUST,
    NV_FILE_BACKUP ,
    NV_FILE_XNV_CARD_1 ,
    NV_FILE_XNV_CARD_2,
    NV_FILE_XNV_CARD_3,
    NV_FILE_CUST_CARD_1,
    NV_FILE_CUST_CARD_2,
    NV_FILE_CUST_CARD_3,
    NV_FILE_SYS_NV,
    NV_FILE_DEFAULT,
    NV_FILE_XNV_MAP_CARD_1,
    NV_FILE_XNV_MAP_CARD_2,
    NV_FILE_XNV_MAP_CARD_3,
    NV_FILE_BUTT
};

#define   NV_FILE_EXIST              NV_FILE_USING   /*nv*/
#define   NV_DLOAD_INVALID_FLAG      0xe601cdba



/*API ENUM*/
enum
{
    NV_FILE_INIT_API = 0,
    NV_FILE_OPEN_API = 1,
    NV_FILE_CLOSE_API= 2,
    NV_FILE_READ_API = 3,
    NV_FILE_WRITE_API= 4,
    NV_FILE_WRITE_API_IMG = 5,
    NV_FILE_WRITE_API_BAK = 6,
    NV_FILE_WRITE_API_DEF = 7,
    NV_FILE_SEEK_API = 8,
    NV_FILE_FTELL_API= 9,
    NV_FILE_REMOVE_API = 10,
    NV_FILE_OPS_MAX_API
};

enum
{
    NV_GOOD_SECTION,
    NV_BAD_SECTION
};



#define NV_FLASH_NULL                        (0xffffffff)
#define NV_FLASH_VALID_DATA                  (0x0)
#define NV_FLASH_FILL                        (0xff)    /*mtd write one page,need to fill invality data*/

/*8bit nand,blocksize = 128k*/
#define NV_BIN_FILE_BLOCK_NUM         32           /*分区最大有4M,32块*/
#define NV_DLOAD_FILE_BLOCK_NUM       24           /*dload 分区长度3M,块数最多24块*/

struct nv_sec_file_block_info
{
    u32 nv_dload[NV_DLOAD_FILE_BLOCK_NUM];
    u32 sys_nv[NV_DLOAD_FILE_BLOCK_NUM];
    u32 nv_bak[NV_BIN_FILE_BLOCK_NUM];
    u32 nv_default[NV_BIN_FILE_BLOCK_NUM];
};


struct nv_global_ctrl_stru
{
    nv_dload_packet_head_s nv_dload;  /*data info in dload flag*/
    xnv_map_file_s *other_card_info;  /*除卡1 2外的其他卡的NV信息*/
    nv_file_map_s  dload_nv;
    nv_file_map_s  dload_nvcust;
    nv_file_map_s  sys_nv;
    nv_file_map_s  bak_sec;
    nv_file_map_s  def_sec;
};

#ifdef BSP_CONFIG_PHONE_TYPE

#include "adrv.h"

#define NV_BACK_SEC_NAME               PARTITION_MODEM_NVBACKUP_NAME
#define NV_DLOAD_SEC_NAME              PARTITION_MODEM_NVDLOAD_NAME
#define NV_DLOAD_CUST_SEC_NAME         PARTITION_MODEM_NVDLOAD_CUST_NAME
#define NV_DEF_SEC_NAME                PARTITION_MODEM_NVDEFAULT_NAME
#define NV_SYS_SEC_NAME                PARTITION_MODEM_NVSYS_NAME
#else

#include "ptable_com.h"

#define NV_BACK_SEC_NAME               PTABLE_NVBACK_NM
#define NV_DLOAD_SEC_NAME              PTABLE_NVDLOAD_NM
#define NV_DLOAD_CUST_SEC_NAME         "nvdload_cust"
#define NV_DEF_SEC_NAME                PTABLE_NVDEFAULT_NM
#define NV_SYS_SEC_NAME                PTABLE_NVDLOAD_NM
#endif

#if defined(FEATURE_NV_FLASH_ON)

struct nv_nand_info_stru
{
    u32 page_size;
    u32 block_size;
    u32 total_size;
};

struct nv_flash_file_header_stru
{
    void* fp;          /*file head pointer*/
    u32   flash_type;  /*flash type*/
    u32   off;         /*offset in this flash*/
    u32   seek;        /*flash seek*/
    u32   ops;         /*ops count*/
    u32   length;      /*file length,not section length*/
    u32   image;       /*nv img id :ptable_com.h*/
    s8    path[64];    /*file path*/
    s8    name[16];    /*patrition name*/
    struct mtd_info *mtd;  /*mtd device*/
    osl_sem_id file_sem;
};


u32   nv_flash_init(void);
FILE* nv_flash_open(const s8 * path,const s8* mode);
u32   nv_flash_close(FILE * fp);
u32   nv_flash_ftell(FILE * fp);
u32   nv_flash_read(u8 * ptr, u32 size, u32 count, FILE * fp);
u32   nv_flash_write(u8 * ptr, u32 size, u32 count, FILE * fp);
u32   nv_flash_remove(const s8 * path);
u32   nv_flash_seek(FILE * fp, u32 offset, s32 whence);
u32   nv_flash_access(const s8 * path, s32 mode);
u32   nv_flash_update_info(const s8* path);
void  nv_flash_help(u32 type);
void  show_flash_info(void);

#elif defined(FEATURE_NV_LFILE_ON)

FILE* nv_lfile_open(const s8 * path,const s8* mode);
s32   nv_lfile_close(FILE * fp);
s32   nv_lfile_ftell(FILE * fp);
s32   nv_lfile_read(u8 * ptr, u32 size, u32 count, FILE * fp);
s32   nv_lfile_write(u8 * ptr, u32 size, u32 count, FILE * fp);
s32   nv_lfile_remove(const s8 * path);
s32   nv_lfile_seek(FILE * fp, s32 offset, s32 whence);

#elif  defined(FEATURE_NV_RFILE_ON)

FILE* nv_rfile_open(const s8 * path,const s8* mode);
s32   nv_rfile_close(FILE * fp);
s32   nv_rfile_ftell(FILE * fp);
s32   nv_rfile_read(u8 * ptr, u32 size, u32 count, FILE * fp);
s32   nv_rfile_write(u8 * ptr, u32 size, u32 count, FILE * fp);
s32   nv_rfile_remove(const s8 * path);
s32   nv_rfile_seek(FILE * fp, s32 offset, s32 whence);

#elif  defined(FEATURE_NV_EMMC_ON)

#define FSZ                             long unsigned int

struct nv_emmc_info_stru
{
    u32 page_size;
    u32 block_size;
    u32 total_size;
};

struct nv_emmc_file_header_stru
{
    void* fp;          /*file head pointer*/
    u32   emmc_type;   /*emmc type*/
    u32   off;         /*offset in this flash*/
    u32   seek;        /*flash seek*/
    u32   ops;         /*ops count*/
    u32   length;      /*file length,not section length*/
    s8    path[64];    /*file path*/
    s8    name[64];    /*patrition name*/
    struct mtd_info *mtd;  /*mtd device*/
    osl_sem_id file_sem;
};

u32   nv_emmc_init(void);

FILE* nv_emmc_open(const s8 * path,const s8* mode);
u32   nv_emmc_close(FILE * fp);
u32   nv_emmc_ftell(FILE * fp);
u32   nv_emmc_read(u8 * ptr, u32 size, u32 count, FILE * fp);
u32   nv_emmc_write(u8 * ptr, u32 size, u32 count, FILE * fp);
u32   nv_emmc_remove(const s8 * path);
u32   nv_emmc_seek(FILE * fp, u32 offset, s32 whence);
u32   nv_emmc_access(const s8 * path, s32 mode);
u32   nv_emmc_update_info(const s8* path);
void  nv_emmc_help(u32 type);
void  show_emmc_info(void);
#endif
u32 nv_support_multi_card(void);
u32 nv_get_dload_file_len(void);
#define NV_SUPPORT_MULTI_CARD nv_support_multi_card()
#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif


#endif /*_NV_FILE_H_*/

