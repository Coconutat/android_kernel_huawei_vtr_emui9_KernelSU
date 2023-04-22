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
#include <linux/module.h>
#include <linux/mm.h>
#include <linux/io.h>
#include <soc_memmap.h>
#include <bsp_trace.h>
#include <bsp_s_memory.h>
#include <bsp_shared_ddr.h>
#include <bsp_ddr.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_fdt.h>
#include <linux/of.h>
#include <linux/vmalloc.h>

/*lint -e528*/

#define  s_mem_pr_err(fmt, ...) \
	(printk(KERN_ERR "[s_mem]: <%s> "fmt, __FUNCTION__, ##__VA_ARGS__))
struct mem_ctrl g_mem_ctrl;

static int __init bsp_shared_mem_init(void)
{
	g_mem_ctrl.sram_phy_addr = (void*)HI_SRAM_MEM_ADDR;
	g_mem_ctrl.sram_mem_size  = HI_SRAM_SIZE;
	g_mem_ctrl.sram_virt_addr = (void*)ioremap_wc((unsigned long)g_mem_ctrl.sram_phy_addr, g_mem_ctrl.sram_mem_size);
    s_mem_pr_err("sram phy=%10pK, virt=%10lx, size=%10x\n", g_mem_ctrl.sram_phy_addr, (unsigned long)g_mem_ctrl.sram_virt_addr, HI_SRAM_SIZE);
	g_mem_ctrl.sddr_phy_addr  = (void*)HI_SHARED_DDR_BASE_ADDR;
	g_mem_ctrl.sddr_mem_size  = HI_SHARED_DDR_SIZE;
	g_mem_ctrl.sddr_virt_addr = (void*)ioremap_wc((unsigned long)g_mem_ctrl.sddr_phy_addr, g_mem_ctrl.sddr_mem_size);
    s_mem_pr_err("shared_ddr phy=%10pK, virt=%10lx, size=%10x\n", g_mem_ctrl.sddr_phy_addr, (unsigned long)g_mem_ctrl.sddr_virt_addr, HI_SHARED_DDR_SIZE);
	s_mem_pr_err("ok!\n");
	return 0;
}



 #define MAX_RESERVED_REGIONS    16
 static struct reserve_mem_node reserved_mems[MAX_RESERVED_REGIONS];
 static int reserved_mem_count = 0;





static int modem_mperf_reserve_area(struct reserved_mem *rmem)
{
	char *status = NULL;
      s_mem_pr_err("enter modem_mperf_reserve_area 0x%llx, size is 0x%llx\n",
                 rmem->base, rmem->size );
	  
	status = (char *)of_get_flat_dt_prop(rmem->fdt_node, "status", NULL);/*lint !e838*/
	if (status && (strncmp(status, "ok", strlen("ok")) != 0))
		return 0;
	
	/* coverity[secure_coding] */
	(void)strncpy(reserved_mems[reserved_mem_count].name, rmem->name
		, min(sizeof(reserved_mems[reserved_mem_count].name)
		, strnlen(rmem->name,sizeof(reserved_mems[reserved_mem_count].name))));/*lint !e666*/

	reserved_mems[reserved_mem_count].base = rmem->base;
	reserved_mems[reserved_mem_count].size = rmem->size;
	reserved_mem_count++;

      s_mem_pr_err("kernel reserved buffer name %s\n",
                  rmem->name);	

      s_mem_pr_err("kernel reserved buffer is useful, base 0x%llx, size is 0x%llx\n",
                  rmem->base, rmem->size );

	return 0;
}

/*lint -save -e611*/
RESERVEDMEM_OF_DECLARE(hisi_mperf,"hisi_mperf",modem_mperf_reserve_area);/*lint !e528*/
/*lint -restore +e611*/

struct reserve_mem_node*  bsp_reserve_mem_mperf_get(const char* name)
{
	int i;
	for (i=0; i<reserved_mem_count; i++)
	{
		if (strncmp(reserved_mems[i].name, name,strlen(name)) == 0)
			return &reserved_mems[i];
	}
	return NULL;
}

EXPORT_SYMBOL(bsp_reserve_mem_mperf_get);

void * bsp_mem_remap_uncached(unsigned long phys_addr, unsigned int size)
{
    unsigned long i;
    u8* vaddr;
    unsigned long npages = (PAGE_ALIGN((phys_addr & (PAGE_SIZE - 1)) + size) >> PAGE_SHIFT);
    unsigned long offset = phys_addr & (PAGE_SIZE - 1);
    struct page **pages;

    pages = vmalloc(sizeof(struct page *) * npages);
    if (!pages)
    {
        return NULL;
    }

    pages[0] = phys_to_page(phys_addr);

    for (i = 0; i < npages - 1 ; i++)
    {
        pages[i + 1] = pages[i] + 1;
    }

    vaddr = (u8*)vmap(pages, (unsigned int)npages, (unsigned long)VM_MAP, pgprot_writecombine(PAGE_KERNEL));

    if ( NULL != vaddr )

    {
        vaddr += offset;
    }

    vfree(pages);

    return (void *)vaddr;
}

EXPORT_SYMBOL(bsp_mem_remap_uncached);


void  bsp_mem_unmap_uncached(void* virt_addr)
{
    vunmap(virt_addr);
}

EXPORT_SYMBOL(bsp_mem_unmap_uncached);





static struct mperf_info  g_mperf_info = {0};

static int __init  bsp_mem_init_mperf_info(void)
{
	s32 ret = 0;
	struct device_node *np = NULL;
		
	
	np = of_find_compatible_node(NULL, NULL, "hisilicon,mperf_balong");/*lint !e838*/
	if (!np) 
	{
		s_mem_pr_err("can not match hisilicon,mperf_balong in gator_events_mperf_init. defaultcpu_nr=%d\n", g_mperf_info.nr_cpus);
		return -1;
	}	

	ret = of_property_read_u32(np,"nr_cpus", &g_mperf_info.nr_cpus);/*lint !e838*/
	if (!ret)
	{
		g_mperf_info.inited = 1;
		s_mem_pr_err("mperf cntl.cpu_nr=%d\n", g_mperf_info.nr_cpus);
	}
	return 0;
}

struct mperf_info*  bsp_mem_get_mperf_info(void)
{
	if (g_mperf_info.inited)
		return &g_mperf_info;
	else
		return NULL;
}
EXPORT_SYMBOL(bsp_mem_get_mperf_info);

module_init(bsp_mem_init_mperf_info); /*lint !e528*/

core_initcall(bsp_shared_mem_init); /*lint !e528*/
EXPORT_SYMBOL_GPL(g_mem_ctrl);

