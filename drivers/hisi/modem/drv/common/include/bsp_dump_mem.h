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

#ifndef _BSP_DUMP_MEM_H
#define _BSP_DUMP_MEM_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLY__
#include "bsp_dump.h"
#include "osl_types.h"
#include "osl_list.h"
#include "osl_spinlock.h"
#endif
#include "bsp_memmap.h"
#include "bsp_s_memory.h"
#include "mntn_interface.h"
#if defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
#include <string.h>
#endif

/*  dump content

    ---------------
   | head          |
    ---------------
   | area map [8]  |
    ---------------
   | area 1        |
    ---------------
   | area 2        |
    ---------------
   | area 3        |
    ---------------
   | area 4        |
    ---------------
   | area 5        |
    ---------------
   | area 6        |
    ---------------
   | area 7        |
    ---------------
   | area 8        |
    ---------------
*/

/* area content

    ---------------
   | head          |
    ---------------
   | field map[64] |
    ---------------
   | field 1       |
    ---------------
   | field 2       |
    ---------------
   | ...           |
    ---------------
   | field N(N<=64)|
    ---------------
*/
#if defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
#ifdef printk
#undef printk
#endif
#define printk  SRE_Printf
#endif



/* area number supported by dump */
#define DUMP_AREA_MAX_NUM   8

/* field number supported by area */
#define DUMP_FIELD_MAX_NUM  64

#ifndef __ASSEMBLY__
/* field status */
enum
{
    DUMP_FIELD_UNUSED = 0,
    DUMP_FIELD_USED   = 1,
};

/* areas, max number is 32 */
#if !defined(BSP_CONFIG_PHONE_TYPE)
typedef enum
{
	DUMP_AREA_AP,
	DUMP_AREA_CP,
	DUMP_AREA_TEEOS,
	DUMP_AREA_HIFI,
	DUMP_AREA_LPM3,
	DUMP_AREA_IOM3,
	DUMP_AREA_ISP,
	DUMP_AREA_IVP,
	DUMP_AREA_EMMC,
	DUMP_AREA_MDMAP,
	DUMP_AREA_BUTT
}DUMP_AREA_ID;
#else
typedef enum
{
	DUMP_AREA_CP,
    DUMP_AREA_MDMAP,
	DUMP_AREA_BUTT
}DUMP_AREA_ID;
#endif

#endif


/* field magic num */
#define DUMP_FIELD_MAGIC_NUM    (0x6C7D9F8E)

#ifndef __ASSEMBLY__

/*头部接口要与rdr_area.h中定义格式相同*/
#define DUMP_GLOBALE_TOP_HEAD_MAGIC          (0x44656164)
struct dump_global_top_head_s {
	u32 magic;
	u32 version;
	u32 area_number;
    u32 reserve;
    u8 build_time[32];
    u8 product_name[32];
    u8 product_version[32];
};

struct dump_global_area_s {
	u32 offset;
	u32 length;
};

struct dump_global_base_info_s {
	u32 modid;
	u32 arg1;
	u32 arg2;
	u32 e_core;
    u32 e_type;
    u32 start_flag;
    u32 savefile_flag;
    u32 reboot_flag;
	u8	e_module[16];
	u8	e_desc[48];

    u8 datetime[24];
};

struct dump_global_struct_s {
	struct dump_global_top_head_s top_head;
    struct dump_global_base_info_s base_info;
	struct dump_global_area_s area_info[DUMP_AREA_BUTT];
	u8 padding2[MNTN_BASEINFO_SIZE
                - sizeof(struct dump_global_top_head_s)
                - sizeof(struct dump_global_area_s)*DUMP_AREA_BUTT
                - sizeof(struct dump_global_base_info_s)];
};
struct dump_area_mntn_addr_info_s
{
    void*       vaddr;
    void*       paddr;
    u32         len;
};
/* area head  */
typedef struct _dump_area_head_s
{
    u32 magic_num;
    u32 field_num;
    u8  name[8];
    u8  version[16]; /* cp 区域的0-3字节用例表示cp log是否保存完成，12-16字节用来表示当前是否在通话 */
}dump_area_head_t;

/* field map */
typedef struct _dump_field_map_s
{
    u32 field_id;
    u32 offset_addr;
    u32 length;
    u16 version;
    u16 status;
    u8  field_name[16];
}dump_field_map_t;

/* area */
typedef struct _dump_area_s
{
    dump_area_head_t  area_head;
    dump_field_map_t  fields[DUMP_FIELD_MAX_NUM];
    u8                data[4];
}dump_area_t;

#define DUMP_FIELD_SELF_MAGICNUM        (0x73656c66)
struct dump_field_self_info_s
{
    u32         magic_num;
    u32         reserved;
    u32         phy_addr;
    void*       virt_addr;
};

#if defined(__OS_RTOSCK__) || defined(__OS_RTOSCK_SMP__)
#define DUMP_FIXED_FIELD(p, id, name, offset, size) \
{ \
    ((dump_field_map_t*)(p))->field_id    = (id); \
    ((dump_field_map_t*)(p))->length      = (size); \
    ((dump_field_map_t*)(p))->offset_addr      = (u32)(offset); \
    ((dump_field_map_t*)(p))->version     = 0; \
    ((dump_field_map_t*)(p))->status      = DUMP_FIELD_USED; \
    (void)memcpy_s((char *)(((dump_field_map_t*)(p))->field_name), 16, (char *)(name),strlen((char *)(name)) < 16 ? strlen((char *)(name)): 16); \
}
#else
#define DUMP_FIXED_FIELD(p, id, name, offset, size) \
{ \
    ((dump_field_map_t*)(p))->field_id    = (id); \
    ((dump_field_map_t*)(p))->length      = (size); \
    ((dump_field_map_t*)(p))->offset_addr = (u32)(offset); \
    ((dump_field_map_t*)(p))->version     = 0; \
    ((dump_field_map_t*)(p))->status      = DUMP_FIELD_USED; \
    memcpy((char *)(((dump_field_map_t*)(p))->field_name), (char *)(name), strlen((char *)(name)) <16 ? strlen((char *)(name)): 16); \
}
#endif

#ifdef ENABLE_BUILD_OM
s32 bsp_dump_mem_init(void);
u8* bsp_dump_get_field_map(u32 field_id);
u8* bsp_dump_get_area_addr(u32 field_id);
#else
static s32 inline bsp_dump_mem_init(void)
{
    return 0;
}
static inline u8 * bsp_dump_get_field_map(u32 field_id)
{
    return 0;
}
static inline u8 * bsp_dump_get_area_addr(u32 field_id)
{
    return 0;
}
static u32 inline bsp_dump_mem_map(void)
{
    return BSP_OK;
}
#endif

#endif /*__ASSEMBLY__*/

#ifdef __cplusplus
}
#endif

#endif /* _BSP_DUMP_MEM_H */

