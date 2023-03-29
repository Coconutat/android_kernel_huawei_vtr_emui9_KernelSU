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
#ifndef __AP_DUMP_H__
#define __AP_DUMP_H__


#include <mdrv_om_common.h>
#include "bsp_om_enum.h"



typedef enum _dump_arm_vec_e
{
    DUMP_ARM_VEC_RESET          = 0x0,
    DUMP_ARM_VEC_UNDEF          = 0x4,
    DUMP_ARM_VEC_SWI            = 0x8,
    DUMP_ARM_VEC_PREFETCH       = 0xc,
    DUMP_ARM_VEC_DATA           = 0x10,
    DUMP_ARM_VEC_IRQ            = 0x18,
    DUMP_ARM_VEC_FIQ            = 0x1c,
    DUMP_ARM_VEC_UNKNOW         = 0xff,
}dump_arm_vec_t;

typedef enum _adump_reboot_reason_e
{
    AP_DUMP_REASON_NORMAL      = 0x0,
    AP_DUMP_REASON_ARM         = 0x1,
    AP_DUMP_REASON_STACKFLOW   = 0x2,
    AP_DUMP_REASON_UNDEF       = 0xff
}adump_reboot_reason_e;



/**************************************************************************
  STRUCT定义
**************************************************************************/


/**************************************************************************
  宏定义
**************************************************************************/
/*0x00~0xff linux kernel*/
#define RDR_AP_DUMP_ARM_MOD_ID_START             (0x80000000)
#define RDR_AP_DUMP_ARM_RESET_MOD_ID             (0x80000000)
#define RDR_AP_DUMP_ARM_UNDEF_MOD_ID             (0x80000004)
#define RDR_AP_DUMP_ARM_SWI_MOD_ID               (0x80000008)
#define RDR_AP_DUMP_ARM_PREFETCH_MOD_ID          (0x8000000c)
#define RDR_AP_DUMP_ARM_DATA_MOD_ID              (0x80000010)
#define RDR_AP_DUMP_ARM_IRQ_MOD_ID               (0x80000018)
#define RDR_AP_DUMP_ARM_FIQ_MOD_ID               (0x8000001c)
#define RDR_AP_DUMP_PANIC_MOD_ID                 (0x8000001d)

/*0xff~..*/
#define RDR_AP_DUMP_ARM_UNKNOW_MOD_ID            (0x80000100)
#define RDR_AP_DUMP_NORMAL_EXC_MOD_ID            (0x80000101)
#define RDR_AP_DUMP_AP_WDT_MOD_ID                (0x80000102)



#define RDR_AP_DUMP_ARM_MOD_ID_END                   (0x81FFFFFF)








/********************************************/
void adump_set_exc_vec(u32 vec);
/********************************************/
#ifdef CONFIG_HISI_DUMP
dump_handle bsp_adump_register_hook(char * name, dump_hook func);
u8 * bsp_adump_register_field(u32 field_id, char * name, void * virt_addr, void * phy_addr, u32 length, u16 version);
u8 * bsp_adump_get_field_addr(u32 field_id);
s32 bsp_adump_unregister_hook(dump_handle handle);
void bsp_adump_save_exc_scene(u32 mod_id, u32 arg1, u32 arg2);
u8 * bsp_adump_get_field_phy_addr(u32 field_id);
void ap_system_error(u32 mod_id, u32 arg1, u32 arg2, char *data, u32 length);
#else
static inline dump_handle bsp_adump_register_hook(char * name, dump_hook func){return -1;}
static inline u8 * bsp_adump_register_field(u32 field_id, char * name, void * virt_addr, void * phy_addr, u32 length, u16 version){return NULL;}
static inline u8 * bsp_adump_get_field_addr(u32 field_id){return NULL;}
static inline s32 bsp_adump_unregister_hook(dump_handle handle){return -1;}
static inline void bsp_adump_save_exc_scene(u32 mod_id, u32 arg1, u32 arg2){return;}
static inline u8 * bsp_adump_get_field_phy_addr(u32 field_id){return NULL;}
static inline void ap_system_error(u32 mod_id, u32 arg1, u32 arg2, char *data, u32 length){return;}
#endif

#endif

