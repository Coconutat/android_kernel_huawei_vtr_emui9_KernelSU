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
#ifndef _BSP_CORESIGHT_H_
#define _BSP_CORESIGHT_H_

#ifdef __cplusplus
extern "C"
{
#endif


#define CORESIGHT_MAGIC_NUM         (0x89ABCDEF)
#define CORESIGHT_HOTPLUG_MAGICNUM  (0xFEDCBA98)
#define DUMP_AP_UTRACE_SIZE  0x2400
#define DUMP_CP_UTRACE_SIZE  0x2400



/*etm4x*/
#define MDM_TRCPRGCTLR			0x004

/* etm3x Trace registers (0x000-0x2FC) */
#define MDM_ETMCR               (0x000)
#define MDM_ETMSR               (0x010)
#define MDM_ETMTEEVR            (0x020)
#define MDM_CORESIGHT_LAR       (0xFB0)
#define MDM_CORESIGHT_UNLOCK    (0xC5ACCE55)



struct coresight_etb_data_head_info
{
    unsigned int magic;
    unsigned int length;
};

#ifdef CONFIG_CORESIGHT
#if defined(BSP_CONFIG_PHONE_TYPE) && defined(__KERNEL__) /*linux“‘º∞PHONE–ŒÃ¨*/
static void inline bsp_coresight_enable(void) {  return;   }
static void inline bsp_coresight_disable(void){  return;   }
static void inline bsp_coresight_suspend(void){  return;   }
static void inline bsp_coresight_resume(void) {  return;   }
#else
void bsp_coresight_enable(void);
void bsp_coresight_disable(void);
void bsp_coresight_suspend(void);
void bsp_coresight_resume(void);
#endif
void bsp_coresight_save_cp_etb(char* dir_name);
int bsp_coresight_stop_cp(void);
#else
static unsigned int inline  bsp_coresight_init(void)   {  return 0; }
static void inline bsp_coresight_enable(void) {  return;   }
static void inline bsp_coresight_disable(void){  return;   }
static void inline bsp_coresight_suspend(void){  return;   }
static void inline bsp_coresight_resume(void) {  return;   }
static void inline bsp_coresight_save_cp_data(void){  return;   }
static void inline bsp_coresight_save_cp_etb(char* dir_name){  return;   }
static int inline bsp_coresight_stop_cp(void){ return 0;}
#endif

#ifdef __cplusplus
}
#endif

#endif /* _BSP_CORESIGHT_H_ */


