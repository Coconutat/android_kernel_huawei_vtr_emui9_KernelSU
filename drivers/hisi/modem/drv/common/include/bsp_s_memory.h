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
#ifndef __BSP_S_MEMORY_H__
#define __BSP_S_MEMORY_H__
#ifdef __cplusplus
extern "C" {
#endif

#ifndef __ASSEMBLY__
#include <osl_types.h>
struct mem_ctrl{
	void* sram_virt_addr;
	void* sram_phy_addr;
	unsigned int sram_mem_size;
	void* sddr_virt_addr;
	void* sddr_phy_addr;
	unsigned int sddr_mem_size;
};

extern struct mem_ctrl g_mem_ctrl;

#if 0
static inline void* bsp_sram_addr_get(void* phy_addr){
	return (void*)(g_mem_ctrl.sram_virt_addr + (phy_addr - g_mem_ctrl.sram_phy_addr));
}
static inline void* bsp_sddr_addr_get(void* phy_addr){
	return (void*)(g_mem_ctrl.sddr_virt_addr + (phy_addr - g_mem_ctrl.sddr_phy_addr));
}
#endif

 
struct reserve_mem_node {
	char 				name[64];
	unsigned long			base;
	unsigned long			size;
};

#ifdef CONFIG_OF_RESERVED_MEM
struct reserve_mem_node*  bsp_reserve_mem_mperf_get(const char* name);

void * bsp_mem_remap_uncached(unsigned long phys_addr, unsigned int size);

void  bsp_mem_unmap_uncached(void* virt_addr);

#else
static inline struct reserve_mem_node*  bsp_reserve_mem_mperf_get(const char* name) {return NULL;}

static inline void * bsp_mem_remap_uncached(unsigned long phys_addr, unsigned int size) {return NULL;}

static inline void  bsp_mem_unmap_uncached(void* virt_addr){return ;}

#endif

struct mperf_info {
	u32 inited;
	u32 nr_cpus;
};

struct mperf_info*  bsp_mem_get_mperf_info(void);

#endif /*__ASSEMBLY__*/

#ifdef __cplusplus
}
#endif

#endif /* __BSP_S_MEMORY_H__ */

