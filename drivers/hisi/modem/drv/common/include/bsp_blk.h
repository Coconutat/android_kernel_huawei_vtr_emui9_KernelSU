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

#ifndef __BSP_BLK_H__
#define __BSP_BLK_H__

/* fmc自适应约束，注意事项:
  1.block0不支持擦除后直接读取，请写入后再读取;
  2.block0只支持整块写入，不支持部分写入。
*/

#ifdef __cplusplus /* __cplusplus */
extern "C"
{
#endif /* __cplusplus */

#ifdef __KERNEL__
#include<linux/types.h>

int bsp_blk_read(const char *part_name,loff_t part_offset, void *data_buf, size_t data_len);
int bsp_blk_write(const char *part_name,loff_t part_offset, void *data_buf, size_t data_len);
int bsp_blk_isbad(const char *part_name,loff_t partition_offset);
int bsp_blk_erase(const char *part_name);
int bsp_blk_size(const char *part_name,u32 *size);

int bsp_blk_get_cdromiso_blkname(char *blk_path, int len);

#else
#define FS_OOB_SIZE  16
int bsp_blk_get_page_size(void);
int bsp_blk_get_block_size(void);
int bsp_blk_get_spare_size(unsigned int *sparesize);

int bsp_blk_read(const char *part_name,unsigned int part_offset, void *data_buf, unsigned int data_len);
int bsp_blk_read_oob(const char *part_name, unsigned int part_offset, void *data_buf, unsigned int length, unsigned int spare);
int bsp_blk_read_ecc0(const char *part_name, unsigned int part_offset, void *data_buf, unsigned int length, unsigned int spare);
int bsp_blk_write(const char *part_name,unsigned int part_offset, void *data_buf, unsigned int data_len);
int bsp_blk_write_oob(const char *part_name,unsigned int part_offset, void *data_buf, unsigned int length, unsigned int spare);
int bsp_blk_write_ecc0(const char *part_name,unsigned int part_offset, void *data_buf, unsigned int length, unsigned int spare);
int bsp_blk_erase(char *part_name);
int bsp_blk_erase_all(void);

int bsp_blk_ptable_update(void *ptbl_buf);

int bsp_blk_get_total_blknr(void);

enum {
    READ_OPS = 1,
    READ_OOB,
    READ_ECC0,
    WRITE_OPS,
    WRITE_OOB,
    WRITE_ECC0,
    MAX_OPS,
};
int bsp_blk_read_write_dump(unsigned int ops_type, unsigned int offset, void *data, unsigned int length, unsigned int spare);
#endif

#ifdef __cplusplus /* __cplusplus */
}
#endif /* __cplusplus */

#endif    /*  __BSP_NANDC_H__ */
