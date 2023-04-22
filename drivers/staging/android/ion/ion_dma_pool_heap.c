/*
 * drivers/staging/android/ion/ion_dma_pool_heap.c
 *
 * Copyright (C) 2011 Google, Inc.
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
#define pr_fmt(fmt) "[dma_pool: ]" fmt

#include <linux/spinlock.h>
#include <linux/dma-mapping.h>
#include <linux/err.h>
#include <linux/genalloc.h>
#include <linux/io.h>
#include <linux/ion.h>
#include <linux/workqueue.h>
#include <linux/mm.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/time.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/dma-contiguous.h>
#include <linux/cma.h>
#include <linux/atomic.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/version.h>
#include <linux/platform_device.h>

#include "ion.h"
#include "ion_priv.h"

/**
 * Why pre-allocation size is 64MB?
 * 1.The time of 64MB memory cma releasing is short
 * 2.64MB memory is larger then iris's memory size
 */

/* align must be modify with count */
#define PREALLOC_ALIGN   (14U)
#define PREALLOC_CNT     (64UL * SZ_1M / PAGE_SIZE)
#define PREALLOC_NWK     (PREALLOC_CNT * 4U)

#define DMA_CAMERA_WATER_MARK (512 * SZ_1M)

static void pre_alloc_wk_func(struct work_struct *work);

struct ion_dma_pool_heap {
	struct ion_heap heap;
	struct gen_pool *pool;
	struct device	*dev;
	ion_phys_addr_t base;
	size_t size;
	atomic64_t alloc_size;
	atomic64_t prealloc_cnt;
};
struct cma *ion_dma_camera_cma;
static struct ion_dma_pool_heap *ion_dma_camera_heap;
static DECLARE_WORK(ion_pre_alloc_wk, pre_alloc_wk_func);

struct cma *hisi_camera_pool;

static int  hisi_camera_pool_set_up(struct reserved_mem *rmem)
{
	phys_addr_t align = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);
	phys_addr_t mask = align - 1;
	unsigned long node = rmem->fdt_node;
	struct cma *cma;
	int err;

	if (!of_get_flat_dt_prop(node, "reusable", NULL) ||
	    of_get_flat_dt_prop(node, "no-map", NULL))
		return -EINVAL;

	if ((rmem->base & mask) || (rmem->size & mask)) {
		pr_err("Reserved memory: incorrect alignment of CMA region\n");
		return -EINVAL;
	}
	err = cma_init_reserved_mem(rmem->base, rmem->size, 0, &cma);
	if (err) {
		pr_err("Reserved memory: unable to setup CMA region\n");
		return err;
	}

	hisi_camera_pool = cma;

	return 0;
}

RESERVEDMEM_OF_DECLARE(hisi_camera_pool, "hisi-camera-pool", hisi_camera_pool_set_up);/*lint !e611*/

void ion_register_dma_camera_cma(void *p)
{
	struct cma *cma = (struct cma *)p;
	if (cma
	    && cma_get_size(cma) >= DMA_CAMERA_WATER_MARK){
		ion_dma_camera_cma = cma;
		pr_info("register_dma_camera_cma is ok\n");
	}
}

void ion_clean_dma_camera_cma(void)
{
	ion_phys_addr_t addr;
	unsigned long size_remain = 0;
#ifdef ION_DMA_POOL_DEBUG
	int free_count = 0;
	unsigned long t_ns = 0;
	ktime_t t1 = ktime_get();
#endif
	if (!ion_dma_camera_heap)
		return;

	while (!!(addr = gen_pool_alloc(ion_dma_camera_heap->pool, PAGE_SIZE))) {/*lint !e820*/
		if (ion_dma_camera_cma) {
			if (cma_release(ion_dma_camera_cma,  phys_to_page(addr), 1)) {
				atomic64_sub(1, &ion_dma_camera_heap->prealloc_cnt);
#ifdef ION_DMA_POOL_DEBUG
				free_count++;
#endif
			}
			else
				pr_err("cma release failed\n");
		}

		/* here is mean the camera is start again */
		if (!atomic64_sub_and_test(0L, &ion_dma_camera_heap->alloc_size)) {
			pr_err("@free memory camera is start again, alloc sz %llx\n",
					atomic64_read(&ion_dma_camera_heap->alloc_size));
			break;
		}
	}

	size_remain = gen_pool_avail(ion_dma_camera_heap->pool);
	if (!!size_remain)
		pr_err("out %s, size_remain = 0x%lx\n", __func__, size_remain);

	pr_info("quit %s,prealloc_cnt now:%ld\n",
		__func__, atomic64_read(&ion_dma_camera_heap->prealloc_cnt));

#ifdef ION_DMA_POOL_DEBUG
	t_ns = ktime_to_ns(ktime_sub(ktime_get(), t1));
	pr_err("cma free size %lu B time is %lu us\n",
		free_count * PAGE_SIZE, t_ns / 1000UL);
#endif
}

static void pre_alloc_wk_func(struct work_struct *work)
{
	struct page *page = NULL;
	struct page *t_page = NULL;
	ion_phys_addr_t addr;
	int k;
#ifdef ION_DMA_POOL_DEBUG
	unsigned long t_ns = 0;
	ktime_t t1 = ktime_get();
#endif

	if (!ion_dma_camera_heap)
		return;

	if ((unsigned long)atomic64_read(&ion_dma_camera_heap->prealloc_cnt) > PREALLOC_NWK)
		return;

	page = cma_alloc(ion_dma_camera_cma, PREALLOC_CNT, PREALLOC_ALIGN);/*lint !e838*/
	if (page) {
		addr = page_to_phys(page);
		memset(phys_to_virt(addr), 0x0, PREALLOC_CNT * PAGE_SIZE); /* unsafe_function_ignore: memset  */
#ifdef CONFIG_HISI_KERNELDUMP
		t_page = page;
		for (k = 0; k < (int)PREALLOC_CNT; k++) {
			SetPageMemDump(t_page);
			t_page++;
		}
#endif
		atomic64_add(PREALLOC_CNT, &ion_dma_camera_heap->prealloc_cnt);
		gen_pool_free(ion_dma_camera_heap->pool,
			      page_to_phys(page),
			      PREALLOC_CNT * PAGE_SIZE);

#ifdef ION_DMA_POOL_DEBUG
		t_ns = ktime_to_ns(ktime_sub(ktime_get(), t1));
		pr_err("cma alloc size %lu B time is %lu us\n",
			PREALLOC_CNT * PAGE_SIZE, t_ns / 1000);
#endif
		pr_info("enter %s,prealloc_cnt now:%ld\n",
			__func__, atomic64_read(&ion_dma_camera_heap->prealloc_cnt));

		return;
	}
}/*lint !e715*/

static ion_phys_addr_t ion_dma_pool_allocate(struct ion_heap *heap,
				      unsigned long size,
				      unsigned long align)
{
	unsigned long offset = 0;
	struct ion_dma_pool_heap *dma_pool_heap =
		container_of(heap, struct ion_dma_pool_heap, heap);/*lint !e826*/

	if (!dma_pool_heap)
		return (ion_phys_addr_t)-1L;

	offset = gen_pool_alloc(dma_pool_heap->pool, size);
	if (!offset) {
		if ((heap->id == ION_CAMERA_DAEMON_HEAP_ID)
			/*
			 * When the camera can only use 7/16 of all CMA size,
			 * the watermark its looks ok.
			 * v    wm    used   maxchun
			 * NA   NA    400M
			 * 1/2  304M  360M   160M
			 * 1/3  200M  260M   300M
			 * 3/8  228M  300M   240M
			 * 7/16 266M  340M   256M
			 */
			&& (atomic64_read(&dma_pool_heap->alloc_size) < dma_pool_heap->size / 16 * 7))/*lint !e574*/
			schedule_work(&ion_pre_alloc_wk);
		return (ion_phys_addr_t)-1L;
	}
	atomic64_add(size, &dma_pool_heap->alloc_size);
	return offset;
}/*lint !e715*/

static void ion_dma_pool_free(struct ion_heap *heap, ion_phys_addr_t addr,
		       unsigned long size)
{
	struct ion_dma_pool_heap *dma_pool_heap =
		container_of(heap, struct ion_dma_pool_heap, heap);/*lint !e826*/

	if (addr == (ion_phys_addr_t)-1L)
		return;

	memset(phys_to_virt(addr), 0x0, size); /* unsafe_function_ignore: memset  */
	gen_pool_free(dma_pool_heap->pool, addr, size);
	atomic64_sub(size, &dma_pool_heap->alloc_size);/*lint !e713*/

	if (heap->id == ION_CAMERA_DAEMON_HEAP_ID) {
		if (atomic64_sub_and_test(0L, &dma_pool_heap->alloc_size)) {
			ion_clean_dma_camera_cma();
		}
	}
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
static int ion_dma_pool_heap_phys(struct ion_heap *heap,
				  struct ion_buffer *buffer,
				  ion_phys_addr_t *addr, size_t *len)
{
	struct sg_table *table = buffer->priv_virt;
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	*addr = paddr;
	*len = buffer->size;
	return 0;
}/*lint !e715*/
#endif

static int ion_dma_pool_heap_allocate(struct ion_heap *heap,
				      struct ion_buffer *buffer,
				      unsigned long size, unsigned long align,
				      unsigned long flags)
{
	struct sg_table *table;
	ion_phys_addr_t paddr;
	int ret;

	if (align > PAGE_SIZE)
		return -EINVAL;

	table = kmalloc(sizeof(struct sg_table), GFP_KERNEL);
	if (!table)
		return -ENOMEM;
	ret = sg_alloc_table(table, 1, GFP_KERNEL);
	if (ret)
		goto err_free;

	paddr = ion_dma_pool_allocate(heap, size, align);
	if (paddr == (ion_phys_addr_t)-1L) {
		ret = -ENOMEM;
		goto err_free_table;
	}

	sg_set_page(table->sgl, pfn_to_page(PFN_DOWN(paddr)), (unsigned int)size, (unsigned int)0);
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	buffer->priv_virt = table;
#else
	buffer->sg_table = table;
#endif

	return 0;

err_free_table:
	sg_free_table(table);
err_free:
	kfree(table);
	return ret;
}/*lint !e715*/

static void ion_dma_pool_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct platform_device *hisi_ion_dev = get_hisi_ion_platform_device();
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	struct sg_table *table = buffer->priv_virt;
#else
	struct sg_table *table = buffer->sg_table;
#endif
	struct page *page = sg_page(table->sgl);
	ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));

	/*the sync has done by caller, we don't need to do any more */
	ion_heap_buffer_zero(buffer);

	if (ion_buffer_cached(buffer))
		dma_sync_sg_for_device(&hisi_ion_dev->dev, table->sgl, (int)table->nents,
				       DMA_BIDIRECTIONAL);

	ion_dma_pool_free(heap, paddr, buffer->size);
	sg_free_table(table);
	kfree(table);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
static struct sg_table *ion_dma_pool_heap_map_dma(struct ion_heap *heap,
						  struct ion_buffer *buffer)
{
	return buffer->priv_virt;
}/*lint !e715*/

static void ion_dma_pool_heap_unmap_dma(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
}/*lint !e715*/
#endif

static void ion_dma_pool_heap_buffer_zero(struct ion_buffer *buffer)
{
	ion_heap_buffer_zero(buffer);
}

static struct ion_heap_ops dma_pool_heap_ops = {
	.allocate = ion_dma_pool_heap_allocate,
	.free = ion_dma_pool_heap_free,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	.phys = ion_dma_pool_heap_phys,
	.map_dma = ion_dma_pool_heap_map_dma,
	.unmap_dma = ion_dma_pool_heap_unmap_dma,
#endif
	.map_user = ion_heap_map_user,
	.map_kernel = ion_heap_map_kernel,
	.unmap_kernel = ion_heap_unmap_kernel,
	.map_iommu = ion_heap_map_iommu,
	.unmap_iommu = ion_heap_unmap_iommu,
	.buffer_zero = ion_dma_pool_heap_buffer_zero,
};/*lint !e785*/

static struct ion_heap *ion_dynamic_dma_pool_heap_create(struct ion_platform_heap *heap_data)
{
	struct ion_dma_pool_heap *dma_pool_heap =
				kzalloc(sizeof(struct ion_dma_pool_heap), GFP_KERNEL);
	if(!dma_pool_heap)
		return ERR_PTR(-ENOMEM);/*lint !e747*/

	dma_pool_heap->heap.ops = &dma_pool_heap_ops;
	dma_pool_heap->heap.type = ION_HEAP_TYPE_DMA_POOL;
	dma_pool_heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;
	if (!ion_dma_camera_cma)
		goto alloc_err;
	dma_pool_heap->base = cma_get_base(ion_dma_camera_cma);
	dma_pool_heap->size = cma_get_size(ion_dma_camera_cma);
	dma_pool_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!dma_pool_heap->pool)
		goto alloc_err;
	if (gen_pool_add(dma_pool_heap->pool,
			 dma_pool_heap->base,
			 dma_pool_heap->size,
			 -1))
		goto pool_create_err;
	if (!gen_pool_alloc(dma_pool_heap->pool, dma_pool_heap->size))
		goto pool_create_err;

	atomic64_set(&dma_pool_heap->alloc_size, 0);
	atomic64_set(&dma_pool_heap->prealloc_cnt, 0);

	ion_dma_camera_heap = dma_pool_heap;

	heap_data->size = dma_pool_heap->size;
	heap_data->base = dma_pool_heap->base;

	return &dma_pool_heap->heap;

pool_create_err:
	gen_pool_destroy(dma_pool_heap->pool);
alloc_err:
	kfree(dma_pool_heap);

	return ERR_PTR(-ENOMEM);/*lint !e747*/
}/*lint !e715*/

struct ion_heap *ion_static_dma_pool_heap_create(struct ion_platform_heap *heap_data)/*lint !e715*/
{
	struct ion_dma_pool_heap *dma_pool_heap;
	int ret, k;
	struct page *page, *t_page;
	size_t size;

	if (!hisi_camera_pool)
		return ERR_PTR(-ENOMEM);/*lint !e747*/

	size = cma_get_size(hisi_camera_pool);
	page = cma_alloc(hisi_camera_pool, size >> PAGE_SHIFT, 0);
	if (!page)
		return ERR_PTR(-ENOMEM);/*lint !e747*/

#ifdef CONFIG_HISI_KERNELDUMP
		t_page = page;
		for (k = 0; k < (int)(size >> PAGE_SHIFT); k++) {
			SetPageMemDump(t_page);
			t_page++;
		}
#endif

	ion_pages_sync_for_device(NULL, page, size, DMA_BIDIRECTIONAL);

	ret = ion_heap_pages_zero(page, size, pgprot_writecombine(PAGE_KERNEL));
	if (ret)
		return ERR_PTR(ret);/*lint !e747*/

	dma_pool_heap = kzalloc(sizeof(struct ion_dma_pool_heap), GFP_KERNEL);
	if (!dma_pool_heap)
		return ERR_PTR(-ENOMEM);/*lint !e747*/

	dma_pool_heap->pool = gen_pool_create(PAGE_SHIFT, -1);
	if (!dma_pool_heap->pool) {
		kfree(dma_pool_heap);
		return ERR_PTR(-ENOMEM);/*lint !e747*/
	}

	dma_pool_heap->base = page_to_phys(page);
	dma_pool_heap->size = size;
	gen_pool_add(dma_pool_heap->pool, dma_pool_heap->base, dma_pool_heap->size,
		     -1);
	dma_pool_heap->heap.ops = &dma_pool_heap_ops;
	dma_pool_heap->heap.type = ION_HEAP_TYPE_DMA_POOL;
	dma_pool_heap->heap.flags = ION_HEAP_FLAG_DEFER_FREE;
	atomic64_set(&dma_pool_heap->alloc_size, 0);
	atomic64_set(&dma_pool_heap->prealloc_cnt, 0);

	heap_data->size = size;
	heap_data->base = page_to_phys(page);

	return &dma_pool_heap->heap;/*lint !e429*/
}

struct ion_heap *ion_dma_pool_heap_create(struct ion_platform_heap *heap_data)/*lint !e715*/
{
	if (heap_data->id == ION_CAMERA_DAEMON_HEAP_ID)
		return ion_dynamic_dma_pool_heap_create(heap_data);
	else
		return ion_static_dma_pool_heap_create(heap_data);

}

static void ion_dynamic_dma_pool_heap_destroy(struct ion_heap *heap)
{
	struct ion_dma_pool_heap *dma_pool_heap =
	     container_of(heap, struct  ion_dma_pool_heap, heap);/*lint !e826*/
	kfree(dma_pool_heap);
	dma_pool_heap = NULL;
}/*lint !e438*/

void ion_static_dma_pool_heap_destroy(struct ion_heap *heap)/*lint !e438*/
{
	struct ion_dma_pool_heap *dma_pool_heap =
	     container_of(heap, struct  ion_dma_pool_heap, heap);/*lint !e826*/
	size_t size = dma_pool_heap->size;
	struct page *page = phys_to_page(dma_pool_heap->base);

	gen_pool_destroy(dma_pool_heap->pool);
	if (hisi_camera_pool)
		if (!cma_release(hisi_camera_pool, page, (int)(size >> PAGE_SHIFT)))
			pr_err("cma release failed\n");
	kfree(dma_pool_heap);
	dma_pool_heap = NULL;
}/*lint !e438*/

void ion_dma_pool_heap_destroy(struct ion_heap *heap)/*lint !e438*/
{
	if (heap->id == ION_CAMERA_DAEMON_HEAP_ID)
		ion_dynamic_dma_pool_heap_destroy(heap);
	else
		ion_static_dma_pool_heap_destroy(heap);
}
