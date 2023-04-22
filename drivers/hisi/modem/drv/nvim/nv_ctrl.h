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

#ifndef _NV_CTRL_H_
#define _NV_CTRL_H_


#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif

#include "nv_comm.h"
#include "nv_file.h"

extern s32 bsp_open(const s8 *path, s32 flags, s32 mode);
extern s32 bsp_read(u32 fd, s8 *ptr, u32 size);
extern s32 bsp_write_sync(u32 fd, const s8 *ptr, u32 size);
extern s32 bsp_lseek(u32 fd, long offset, s32 whence);
extern long bsp_tell(u32 fd);
extern s32 bsp_remove(const s8 *pathname);
extern s32 bsp_access(s8 *path, s32 mode);
extern s32 bsp_close(u32 fp);

#ifdef CONFIG_MULTI_CARRIER
#define MTCA_ROUTINE_MAX_LENGTH   (64)

/* added by yangzhi for muti-carrier, Begin:*/
#define NV_MTCARRIER_USR_BIN_MAGIC  (0xa55af00f)

struct nv_mtcarrier_userlist_bin_stru
{
    u32 magic_head;
    u32 nvid_num;
};

/* added by yangzhi for muti-carrier, End! */

#endif


typedef enum
{
    NV_FILE_STOR_FS     = 0,
    NV_FILE_STOR_NON_FS,
    NV_FILE_STOR_BUTT
}NV_FILE_STOR_TYPE_ENUM;

struct nv_file_p
{
    NV_FILE_STOR_TYPE_ENUM stor_type;
    void* fd;
};


typedef FILE* (*file_open)(const s8 * path,const s8* mode);
typedef u32  (*file_close)(FILE* fp);
typedef u32  (*file_read)(u8* ptr,u32 size,u32 count,FILE* fp);
typedef u32  (*file_write)(u8* ptr,u32 size,u32 count,FILE* fp);
typedef u32  (*file_remove)(const s8* path);
typedef u32  (*file_seek)(FILE* fp,u32 offset,s32 whence);
typedef u32  (*file_ftell)(FILE* fp);
typedef u32  (*file_access)(const s8* path,s32 mode);
typedef u32  (*file_mkdir)(const s8* path);
typedef u32  (*file_update)(const s8* path);
struct file_ops_table_stru
{
    file_open   fo;
    file_read   fr;
    file_write  fw;
    file_close  fc;
    file_remove frm;
    file_seek   fs;
    file_ftell  ff;
    file_access fa;
    file_update fu;
};

int nv_file_getmode(const char *mode, int *flag);
u32 nv_file_init(void);
FILE* nv_file_open(const s8 * path,const s8* mode);
u32 nv_file_read(u8 * ptr, u32 size, u32 count, FILE * fp);
u32 nv_file_write(u8 * ptr, u32 size, u32 count, FILE * fp);
u32 nv_file_close(FILE * fp);
u32 nv_file_seek(FILE * fp, u32 offset, s32 whence);
u32 nv_file_remove(const s8 * path);
u32 nv_file_ftell(FILE * fp);
u32 nv_file_access(const s8* path,s32 mode);
u32 nv_get_bin_file_len(nv_ctrl_info_s* ctrl_info,nv_file_info_s* file_info,u32 * file_len);
u32 nv_get_file_len(FILE* fp);
u32 nv_file_update(const s8* path);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif

#endif /*_NV_CTRL_H_*/


