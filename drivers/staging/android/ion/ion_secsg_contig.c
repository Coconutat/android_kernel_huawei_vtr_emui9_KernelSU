/*
 * ion_secsg_contig.c
 *
 * Copyright (C) 2018 Hisilicon, Inc.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#define pr_fmt(fmt) "secsg: " fmt

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/genalloc.h>
#include <linux/mutex.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/cma.h>
#include <linux/sizes.h>
#include <linux/list.h>
#include <linux/version.h>
#include <linux/hisi/hisi_cma.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/hisi/hisi_mm.h>

#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include "ion.h"
#include "ion_priv.h"
#include "hisi/ion_sec_priv.h"

static inline void free_alloc_list(struct list_head *head)
{
	struct alloc_list *pos = NULL;

	while (!list_empty(head)) {
		pos = list_first_entry(head, struct alloc_list, list);
		if (pos->addr || pos->size)
			pr_err("in %s %pK %x failed\n", __func__,
			       (void *)pos->addr, pos->size);

		list_del(&pos->list);
		kfree(pos);
	}
}

static u32 count_list_nr(struct ion_secsg_heap *secsg_heap)
{
	u32 nr = 0;
	struct list_head *head = &secsg_heap->allocate_head;
	struct list_head *pos;

	list_for_each(pos, head)
		nr++;
	return nr;
}

static struct page *__secsg_cma_alloc(struct ion_secsg_heap *secsg_heap,
				      size_t count, u64 size,
				      unsigned int align)
{
	unsigned long offset = 0;

	if (!secsg_heap->static_cma_region)
		return cma_alloc(secsg_heap->cma, count, align);

	offset = gen_pool_alloc(secsg_heap->static_cma_region,
				size);
	if (!offset) {
		pr_err("__cma_alloc failed!size:0x%llx\n", size);
		return NULL;
	}

	return pfn_to_page(PFN_DOWN(offset));
}

static bool __secsg_cma_release(struct ion_secsg_heap *secsg_heap,
				const struct page *pages,
				unsigned int count,
				u64 size)
{
	if (!secsg_heap->static_cma_region)
		return cma_release(secsg_heap->cma, pages, count);

	gen_pool_free(secsg_heap->static_cma_region, page_to_phys(pages),
		      size);

	return 0;
}

static int cons_phys_struct(struct ion_secsg_heap *secsg_heap, u32 nents,
			    struct list_head *head, u32 cmd_to_ta)
{
	u32 i = 0;
	int ret = 0;
	u32 protect_id = SEC_TASK_MAX;
	struct tz_pageinfo *pageinfo;
	struct mem_chunk_list mcl;
	struct list_head *pos = head->prev;
	struct alloc_list *tmp_list = NULL;
	unsigned long size = nents * sizeof(*pageinfo);

	if (!secsg_heap->TA_init) {
		pr_err("[%s] TA not inited.\n", __func__);
		return -EINVAL;
	}

	pageinfo = kzalloc(size, GFP_KERNEL);
	if (!pageinfo)
		return -ENOMEM;

	for (i = 0; (i < nents) && (pos != (head)); i++) {
		tmp_list = list_entry(pos, struct alloc_list, list);
		pageinfo[i].addr = tmp_list->addr;
		pageinfo[i].nr_pages = tmp_list->size / PAGE_SIZE;
		pos = pos->prev;
	}

	if (i < nents) {
		pr_err("[%s], invalid nents(%d) or head!\n", __func__, nents);
		ret = -EINVAL;
		goto out;
	}

	if (secsg_heap->heap_attr == HEAP_SECURE)
		protect_id = SEC_TASK_SEC;
	else {
		pr_err("not sec heap, return.!\n");
		ret = -EINVAL;
		goto out;
	}
	/*Call TZ Driver here*/
	mcl.nents = nents;
	mcl.phys_addr = (void *)pageinfo;
	mcl.protect_id = protect_id;
	mcl.size = nents * sizeof(struct tz_pageinfo);
	ret = secmem_tee_exec_cmd(secsg_heap->session, &mcl, cmd_to_ta);
out:
	kfree(pageinfo);
	return ret;
}

static int secsg_cma_alloc(struct ion_secsg_heap *secsg_heap,
			   unsigned long user_alloc_size)
{
	int ret = 0;
	unsigned long size_remain;
	unsigned long allocated_size;
	unsigned long cma_remain;
	u64 size = secsg_heap->per_alloc_sz;
	u64 per_bit_sz = secsg_heap->per_bit_sz;
	u64 cma_size;
	/* Add for TEEOS ++, per_alloc_size = 64M */
	unsigned long virt;
	struct page *pg;
	struct alloc_list *alloc = NULL;
	unsigned int count = 0;
	/* Add for TEEOS -- */
#ifdef CONFIG_HISI_KERNELDUMP
	unsigned int k;
	struct page *tmp_page = NULL;
#endif

	secsg_debug("into %s\n", __func__);
	/* add 64M for every times
	 * per_alloc_sz = 64M, per_bit_sz = 16M(the original min_size)
	 */
	allocated_size = secsg_heap->alloc_size;
	size_remain = gen_pool_avail(secsg_heap->pool);
	cma_size = cma_get_size(secsg_heap->cma);
	cma_remain = cma_size - (allocated_size + size_remain);
	if (secsg_heap->heap_size <= (allocated_size + size_remain)) {
		pr_err("heap full! allocated_size(0x%lx), remain_size(0x%lx),"
		       " heap_size(0x%lx), cma_remain(0x%lx)\n", allocated_size,
		       size_remain, secsg_heap->heap_size, cma_remain);
		return -ENOMEM;
	}

	/* we allocated more than 1M for SMMU page table before.
	 * then, for the last cma alloc , there is no 64M in
	 * cma pool. So, we allocate as much contiguous memory
	 * as we can.
	 */
	count = size >> PAGE_SHIFT;

	pg = __secsg_cma_alloc(secsg_heap, (size_t)count, size,
			       get_order(per_bit_sz));

	if (!pg) {
		size = ALIGN(user_alloc_size, per_bit_sz);
		count = size >> PAGE_SHIFT;
		pg = __secsg_cma_alloc(secsg_heap, (size_t)count, size,
				       get_order(per_bit_sz));
		if (!pg) {
			pr_err("out of memory,cma:0x%llx,type:%u,usesize:0x%lx\n",
			       cma_size, secsg_heap->cma_type, allocated_size);
			return -ENOMEM;
		}
	}

#ifdef CONFIG_HISI_KERNELDUMP
	tmp_page = pg;
	for (k = 0; k < count; k++) {
		SetPageMemDump(tmp_page);
		tmp_page++;
	}
#endif

	alloc = kzalloc(sizeof(*alloc), GFP_KERNEL);
	if (!alloc) {
		ret = -ENOMEM;
		goto err_out1;
	}
	alloc->addr = page_to_phys(pg);
	alloc->size = size;
	list_add_tail(&alloc->list, &secsg_heap->allocate_head);

	if (secsg_heap->flag & ION_FLAG_SECURE_BUFFER) {
		ion_flush_all_cpus_caches();
		virt = (unsigned long)__va(alloc->addr);/*lint !e648*/
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
		create_mapping_late(alloc->addr, virt, size,
				    __pgprot(PROT_DEVICE_nGnRE));
#else
		change_secpage_range(alloc->addr, virt, size,
				     __pgprot(PROT_DEVICE_nGnRE));
#endif
		flush_tlb_all();
		if (cons_phys_struct(secsg_heap, 1,
				     &secsg_heap->allocate_head,
				     ION_SEC_CMD_TABLE_SET)) {
			pr_err("cons_phys_struct failed\n");
			ret = -EINVAL;
			goto err_out2;
		}
	} else {
		memset(page_address(pg), 0x0, size);
		ion_flush_all_cpus_caches();
	}
	gen_pool_free(secsg_heap->pool, page_to_phys(pg), size);
	secsg_debug("out %s %llu MB memory(ret = %d).\n",
		    __func__, size / SZ_1M, ret);
	return 0;/*lint !e429*/
err_out2:
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	create_mapping_late(alloc->addr, virt, size,
			    PAGE_KERNEL);
#else
	change_secpage_range(alloc->addr, virt, size,
			     PAGE_KERNEL);
#endif
	flush_tlb_all();
	list_del(&alloc->list);
	kfree(alloc);
err_out1:
	__secsg_cma_release(secsg_heap, pg, count, size);
	return ret;
}

static bool gen_pool_bulk_free(struct gen_pool *pool, u32 size)
{
	u32 i;
	unsigned long offset = 0;

	for (i = 0; i < (size / PAGE_SIZE); i++) {
		offset = gen_pool_alloc(pool, PAGE_SIZE);
		if (!offset) {
			pr_err("%s:%d:gen_pool_alloc failed!\n",
			       __func__, __LINE__);
			return false;
		}
	}
	return true;
}

static void __secsg_pool_release(struct ion_secsg_heap *secsg_heap)
{
	u32 nents;
	u64 addr;
	u32 size;
	unsigned long virt;
	unsigned long size_remain = 0;
	unsigned long offset = 0;
	struct alloc_list *pos;

	if (secsg_heap->flag & ION_FLAG_SECURE_BUFFER) {
		nents = count_list_nr(secsg_heap);
		if (nents &&
		    cons_phys_struct(secsg_heap, nents,
				     &secsg_heap->allocate_head,
				     ION_SEC_CMD_TABLE_CLEAN)) {
			pr_err("heap_type:(%u)unconfig failed!!!\n",
			       secsg_heap->heap_attr);
			goto out;
		}
	}

	if (!list_empty(&secsg_heap->allocate_head)) {
		list_for_each_entry(pos, &secsg_heap->allocate_head, list) {
			addr = pos->addr;
			size = pos->size;
			offset = gen_pool_alloc(secsg_heap->pool, size);
			if (!offset) {
				pr_err("%s:%d:gen_pool_alloc failed! %llx %x\n",
				       __func__, __LINE__, addr, size);
				continue;
			}
			virt = (unsigned long)__va(addr);/*lint !e648*/
			if (secsg_heap->flag & ION_FLAG_SECURE_BUFFER) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
				create_mapping_late(addr, virt, size,
						    PAGE_KERNEL);
#else
				change_secpage_range(addr, virt, size,
						     PAGE_KERNEL);
				flush_tlb_all();
#endif
			}

			__secsg_cma_release(secsg_heap, phys_to_page(addr),
					    size >> PAGE_SHIFT, size);
			pos->addr = 0;
			pos->size = 0;
		}
	}

	if (!list_empty(&secsg_heap->allocate_head)) {
		list_for_each_entry(pos, &secsg_heap->allocate_head, list) {
			addr = pos->addr;
			size = pos->size;
			if (!addr || !size)
				continue;

			if (unlikely(!gen_pool_bulk_free(secsg_heap->pool,
							 size))) {
				pr_err("%s:%d:bulk_free failed! %llx %x\n",
				       __func__, __LINE__, addr, size);
				continue;
			}

			virt = (unsigned long)__va(addr);/*lint !e648*/
			if (secsg_heap->flag & ION_FLAG_SECURE_BUFFER) {
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
				create_mapping_late(addr, virt, size,
						    PAGE_KERNEL);
#else
				change_secpage_range(addr, virt, size,
						     PAGE_KERNEL);
				flush_tlb_all();
#endif
			}
			__secsg_cma_release(secsg_heap, phys_to_page(addr),
					    size >> PAGE_SHIFT, size);
			pos->addr = 0;
			pos->size = 0;
		}
		free_alloc_list(&secsg_heap->allocate_head);
	}
	flush_tlb_all();
out:
	size_remain = gen_pool_avail(secsg_heap->pool);
	if (size_remain)
		pr_err("out %s, size_remain = 0x%lx(0x%lx)\n",
		       __func__, size_remain, offset);
}

int __secsg_alloc_contig(struct ion_secsg_heap *secsg_heap,
			 struct ion_buffer *buffer, unsigned long size)
{
	int ret = 0;
	unsigned long offset = 0;
	struct sg_table *table;

	table = kzalloc(sizeof(*table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;

	if (sg_alloc_table(table, 1, GFP_KERNEL)) {
		pr_err("[%s] sg_alloc_table failed .\n", __func__);
		ret = -ENOMEM;
		goto err_out1;
	}
	/*align size*/
	offset = gen_pool_alloc(secsg_heap->pool, size);
	if (!offset) {
		ret = secsg_cma_alloc(secsg_heap, size);
		if (ret)
			goto err_out2;
		offset = gen_pool_alloc(secsg_heap->pool, size);
		if (!offset) {
			ret = -ENOMEM;
			pr_err("line %d, gen_pool_alloc failed!\n", __LINE__);
			goto err_out2;
		}
	}
	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(offset)), size, 0);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	buffer->priv_virt = table;
#else
	buffer->sg_table = table;
#endif
	if (secsg_heap->heap_attr == HEAP_SECURE)
		pr_info("__secsg_alloc sec buffer phys %lx, size %lx\n",
			offset, size);

	secsg_debug(" out [%s].\n", __func__);
	return ret;
err_out2:
	sg_free_table(table);
err_out1:
	kfree(table);
	return ret;
}

static void __secsg_free_pool(struct ion_secsg_heap *secsg_heap,
			      struct sg_table *table,
			      struct ion_buffer *buffer)
{
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));
	struct platform_device *hisi_ion_dev = get_hisi_ion_platform_device();

	if (!(buffer->flags & ION_FLAG_SECURE_BUFFER)) {
		(void)ion_heap_buffer_zero(buffer);
		if (buffer->flags & ION_FLAG_CACHED)
			dma_sync_sg_for_device(&hisi_ion_dev->dev, table->sgl,
					       table->nents, DMA_BIDIRECTIONAL);
	}
	gen_pool_free(secsg_heap->pool, paddr, buffer->size);
	if (secsg_heap->heap_attr == HEAP_SECURE)
		pr_info("__secsg_free sec buffer phys %lx, size %zx\n",
			paddr, buffer->size);

	sg_free_table(table);
	kfree(table);

	secsg_debug("out %s\n", __func__);
}

int __secsg_fill_watermark(struct ion_secsg_heap *secsg_heap)
{
	struct page *pg;
	u64 size = secsg_heap->water_mark;
	u64 per_bit_sz = secsg_heap->per_bit_sz;
	struct alloc_list *alloc;
	unsigned int count = size >> PAGE_SHIFT;
	unsigned int align = get_order(per_bit_sz);
#ifdef CONFIG_HISI_KERNELDUMP
	unsigned int k;
	struct page *tmp_page = NULL;
#endif

	if (!size || size > secsg_heap->per_alloc_sz)
		return -EINVAL;

	pg = __secsg_cma_alloc(secsg_heap, (size_t)count, size, align);
	if (!pg) {
		pr_err("%s:alloc cma fail\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_HISI_KERNELDUMP
	tmp_page = pg;
	for (k = 0; k < count; k++) {
		SetPageMemDump(tmp_page);
		tmp_page++;
	}
#endif

	alloc = kzalloc(sizeof(*alloc), GFP_KERNEL);
	if (!alloc)
		goto err;

	alloc->addr = page_to_phys(pg);
	alloc->size = size;
	list_add_tail(&alloc->list, &secsg_heap->allocate_head);

	memset(page_address(pg), 0x0, size);
	gen_pool_free(secsg_heap->pool, page_to_phys(pg), size);
	secsg_debug("out %s %llu MB memory.\n",
		    __func__, (size) / SZ_1M);
	return 0;/*lint !e429*/
err:
	__secsg_cma_release(secsg_heap, pg, count, size);
	return -ENOMEM;
}

void __secsg_free_contig(struct ion_secsg_heap *secsg_heap,
			 struct ion_buffer *buffer)
{
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	struct sg_table *table = buffer->priv_virt;
#else
	struct sg_table *table = buffer->sg_table;
#endif

	__secsg_free_pool(secsg_heap, table, buffer);
	WARN_ON(secsg_heap->alloc_size < buffer->size);
	secsg_heap->alloc_size -= buffer->size;
	if (!secsg_heap->alloc_size) {
		__secsg_pool_release(secsg_heap);
		if (secsg_heap->water_mark &&
		    __secsg_fill_watermark(secsg_heap))
			pr_err("__secsg_fill_watermark failed!\n");
	}
}
