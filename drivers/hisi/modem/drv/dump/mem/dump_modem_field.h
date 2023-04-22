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

#ifndef __DUMP_FIELD_H__
#define __DUMP_FIELD_H__

#include <product_config.h>
#include "osl_types.h"
#include "osl_list.h"
#include "osl_spinlock.h"
#include "bsp_memmap.h"
#include "bsp_s_memory.h"
#include "bsp_dump.h"
#include "bsp_dump_mem.h"


/* field number supported by area */
#define DUMP_FIELD_MAX_NUM  64
#define DUMP_AREA_MAGICNUM  0x4e656464


#if defined(__KERNEL__)

#define CURRENT_AREA                (DUMP_AREA_MDMAP)
#define CURRENT_AREA_NAME           "MDMAP"
#define CURRENT_FIELD_ID_START      (0x01000000)
#define CURRENT_FIELD_ID_END        (0x01ffffff)

#endif
#define DUMP_SAVE_SUCCESS                   0xA4A4A4A4

#define DUMP_MODEMAP_BASE_INFO_SIZE                 (0x180)
#define DUMP_MODEMAP_USER_DATA_SIZE                 (0x1000)
 

struct dump_field_ctrl_info_s
{
    dump_area_t*        virt_area_addr;
    void*               phy_area_addr;
    u32                 total_length;
    u32                 free_offset;
    u32                 free_length;
    u32                 ulInitflag;
    u32                 field_num;
    spinlock_t          lock;
};

void bsp_dump_save_self_addr(void);
void dump_fill_save_done(void);
s32 dump_get_cp_save_done(void);
void dump_mdmap_field_init(void);



#endif /*__DUMP_FIELD_H__*/



