/*
 * /ion/ion_secsg_heap.c
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

#define pr_fmt(fmt) "secsg: " fmt

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mm.h>
#include <linux/dma-mapping.h>
#include <linux/dma-contiguous.h>
#include <linux/genalloc.h>
#include <linux/mutex.h>
#include <linux/of_fdt.h>
#include <linux/of_reserved_mem.h>
#include <linux/of_address.h>
#include <linux/platform_device.h>
#include <linux/scatterlist.h>
#include <linux/slab.h>
#include <linux/cma.h>
#include <linux/sizes.h>
#include <linux/memblock.h>
#include <linux/sched.h>
#include <linux/list.h>
#include <linux/workqueue.h>
#include <linux/version.h>
#include <linux/hisi/hisi_cma.h>
#include <linux/hisi/hisi_ion.h>
#include <linux/hisi/hisi_mm.h>
#include <teek_client_api.h>
#include <teek_client_id.h>
#include <teek_client_constants.h>

#include <asm/cacheflush.h>
#include <asm/tlbflush.h>

#include "ion.h"
#include "ion_priv.h"
#include "hisi/ion_sec_priv.h"

static struct ion_sec_cma hisi_secsg_cmas[SEC_SG_CMA_NUM];
static unsigned int secsg_cma_num;

int hisi_sec_cma_set_up(struct reserved_mem *rmem)
{
	phys_addr_t align = PAGE_SIZE << max(MAX_ORDER - 1, pageblock_order);
	phys_addr_t mask = align - 1;
	unsigned long node = rmem->fdt_node;
	struct cma *cma;
	const char *cma_name;
	int err;

	if (secsg_cma_num == 0)
		memset(hisi_secsg_cmas, 0,
		       sizeof(hisi_secsg_cmas));

	if (secsg_cma_num >= SEC_SG_CMA_NUM) {
		pr_err("Sec cma num overflow!secsg_cma_num:%u\n",
		       secsg_cma_num);
		return -EINVAL;
	}

	cma_name = (const char *)of_get_flat_dt_prop(node, "compatible", NULL);

	if (!cma_name) {
		pr_err("can't get compatible, id:%u\n", secsg_cma_num);
		return -EINVAL;
	}
	if (!of_get_flat_dt_prop(node, "reusable", NULL) ||
	    of_get_flat_dt_prop(node, "no-map", NULL)) {
		pr_err("%s err node\n", __func__);
		return -EINVAL;
	}

	if ((rmem->base & mask) || (rmem->size & mask)) {
		pr_err("Reserved memory: incorrect alignment of CMA region\n");
		return -EINVAL;
	}

	if(!memblock_is_memory(rmem->base)){
		memblock_free(rmem->base, rmem->size);
		pr_err("memory is invalid(0x%llx), size(0x%llx)\n",
			rmem->base, rmem->size);
		return -EINVAL;
	}
	err = cma_init_reserved_mem(rmem->base, rmem->size, 0, &cma);
	if (err) {
		pr_err("Reserved memory: unable to setup CMA region\n");
		return err;
	}

	hisi_secsg_cmas[secsg_cma_num].cma_region = cma;
	hisi_secsg_cmas[secsg_cma_num].cma_name = cma_name;

	pr_err("%s init cma:rmem->base:0x%llx,size:0x%llx,cmaname:%s, secsg_cma_num:%u\n",
	       __func__, rmem->base, rmem->size, cma_name, secsg_cma_num);

	secsg_cma_num++;

	if (!strcmp(cma_name, "hisi-cma-pool")) {
		pr_err("%s,cma reg to dma_camera_cma\n", __func__);
		ion_register_dma_camera_cma((void *)cma);
	}

	return 0;
}
/*lint -e528 -esym(528,RESERVEDMEM_OF_DECLARE)*/
RESERVEDMEM_OF_DECLARE(hisi_dynamic_cma,
		       "hisi-cma-pool",
		       hisi_sec_cma_set_up);//lint !e611
RESERVEDMEM_OF_DECLARE(hisi_iris_static_cma,
		       "hisi-iris-sta-cma-pool",
		       hisi_sec_cma_set_up);
RESERVEDMEM_OF_DECLARE(hisi_algo_static_cma,
		       "hisi-algo-sta-cma-pool",
		       hisi_sec_cma_set_up);
/*lint -e528 +esym(528,RESERVEDMEM_OF_DECLARE)*/

static int __secsg_pgtable_init(struct ion_secsg_heap *secsg_heap)
{
	int ret = 0;
	struct mem_chunk_list mcl;
	struct tz_pageinfo pageinfo;

	if (!secsg_heap->TA_init ||
	    secsg_heap->heap_attr != HEAP_PROTECT) {
		pr_err("normal/sec heap can't init secure iommu!\n");
		return -EINVAL;
	}

	mcl.protect_id = SEC_TASK_DRM;
	pageinfo.addr = secsg_heap->pgtable_phys;
	pageinfo.nr_pages = secsg_heap->pgtable_size / PAGE_SIZE;
	mcl.phys_addr = (void *)&pageinfo;

	ret = secmem_tee_exec_cmd(secsg_heap->session,
				  &mcl, ION_SEC_CMD_PGATBLE_INIT);
	if (ret)
		pr_err("init secure iommu fail!\n");

	return ret;
}

static int __secsg_heap_input_check(struct ion_secsg_heap *secsg_heap,
				    unsigned long size, unsigned long flag)
{
	if (secsg_heap->alloc_size + size <= secsg_heap->alloc_size) {
		pr_err("size overflow! alloc_size = 0x%lx, size = 0x%lx,\n",
			secsg_heap->alloc_size, size);
		return -EINVAL;
	}

	if ((secsg_heap->alloc_size + size) > secsg_heap->heap_size) {
		pr_err("alloc size = 0x%lx, size = 0x%lx, heap size = 0x%lx\n",
			secsg_heap->alloc_size, size, secsg_heap->heap_size);
		return -EINVAL;
	}

	if (size > secsg_heap->per_alloc_sz) {
		pr_err("size too large! size 0x%lx, per_alloc_sz 0x%llx",
			size, secsg_heap->per_alloc_sz);
		return -EINVAL;
	}

	if (((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) &&
	    !(flag & ION_FLAG_SECURE_BUFFER)) {
		pr_err("alloc mem w/o sec flag in sec heap(%u) flag(%lx)\n",
		       secsg_heap->heap_attr, flag);
		return -EINVAL;
	}

	if ((secsg_heap->heap_attr == HEAP_NORMAL) &&
	    (flag & ION_FLAG_SECURE_BUFFER)) {
		pr_err("invalid allocate sec in sec heap(%u) flag(%lx)\n",
				secsg_heap->heap_attr, flag);
		return -EINVAL;
	}

	secsg_heap->flag = flag;
	return 0;
}

static int __secsg_create_static_cma_region(struct ion_secsg_heap *secsg_heap,
					    u64 cma_base, u64 cma_size)
{
	struct page *page = NULL;

	page = cma_alloc(secsg_heap->cma, cma_size >> PAGE_SHIFT, 0);
	if (!page) {
		pr_err("%s cma_alloc failed!cma_base 0x%llx cma_size 0x%llx\n",
		       __func__, cma_base, cma_size);
		return -ENOMEM;
	}

	pr_err("%s cma_alloc success!cma_base 0x%llx cma_size 0x%llx\n",
	       __func__, cma_base, cma_size);

	secsg_heap->static_cma_region =
		gen_pool_create(secsg_heap->pool_shift, -1);

	if (!secsg_heap->static_cma_region) {
		pr_err("static_cma_region create failed\n");
		cma_release(secsg_heap->cma, page, cma_size >> PAGE_SHIFT);
		return -ENOMEM;
	}
	gen_pool_set_algo(secsg_heap->static_cma_region,
			  gen_pool_best_fit, NULL);

	if (gen_pool_add(secsg_heap->static_cma_region,
			 page_to_phys(page), cma_size, -1)) {
		pr_err("static_cma_region add base 0x%llx cma_size 0x%llx failed!\n",
		       cma_base, cma_size);
		cma_release(secsg_heap->cma, page, cma_size >> PAGE_SHIFT);
		gen_pool_destroy(secsg_heap->static_cma_region);
		secsg_heap->static_cma_region = NULL;
		return -ENOMEM;
	}

	return 0;
}

static int __secsg_create_pool(struct ion_secsg_heap *secsg_heap)
{
	u64 cma_base;
	u64 cma_size;
	int ret = 0;

	secsg_debug("into %s\n", __func__);
	/* Allocate on 4KB boundaries (1 << ION_PBL_SHIFT)*/
	secsg_heap->pool = gen_pool_create(secsg_heap->pool_shift, -1);

	if (!secsg_heap->pool) {
		pr_err("in __secsg_create_pool create failed\n");
		return -ENOMEM;
	}
	gen_pool_set_algo(secsg_heap->pool, gen_pool_best_fit, NULL);

	/* Add all memory to genpool first，one chunk only*/
	cma_base = cma_get_base(secsg_heap->cma);
	cma_size = cma_get_size(secsg_heap->cma);

	if (gen_pool_add(secsg_heap->pool, cma_base, cma_size, -1)) {
		pr_err("genpool add base 0x%llx cma_size 0x%llx\n",
		       cma_base, cma_size);
		ret = -ENOMEM;
		goto err_add;
	}

	if (!gen_pool_alloc(secsg_heap->pool, cma_size)) {
		pr_err("in __secsg_create_pool alloc failed\n");
		ret = -ENOMEM;
		goto err_alloc;
	}

	if (secsg_heap->cma_type == CMA_STATIC) {
		ret = __secsg_create_static_cma_region(secsg_heap,
						       cma_base,
						       cma_size);
		if (ret)
			goto err_cma_alloc;
	}

	return 0;
err_cma_alloc:
	gen_pool_free(secsg_heap->pool, cma_base, cma_size);
err_alloc:
	gen_pool_destroy(secsg_heap->pool);
err_add:
	secsg_heap->pool = NULL;
	return ret;
}

static int secsg_alloc(struct ion_secsg_heap *secsg_heap,
			struct ion_buffer *buffer,
			unsigned long size)
{
	int ret = 0;

	if (secsg_heap->heap_attr == HEAP_PROTECT)
		ret = __secsg_alloc_scatter(secsg_heap, buffer, size);
	else
		ret = __secsg_alloc_contig(secsg_heap, buffer, size);

	return ret;
}

#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4, 9, 0))
int ion_secmem_heap_phys(struct ion_heap *heap, struct ion_buffer *buffer,
			 ion_phys_addr_t *addr, size_t *len)
{
	struct sg_table *table = buffer->sg_table;
	struct page *page = sg_page(table->sgl);
	struct ion_secsg_heap *secsg_heap;

	if (heap->type != ION_HEAP_TYPE_SECSG) {
		pr_err("%s: not secsg mem!\n", __func__);
		return -EINVAL;
	}

	secsg_heap = container_of(heap, struct ion_secsg_heap, heap);

	if(secsg_heap->heap_attr == HEAP_PROTECT) {
		*addr = buffer->id;
		*len = buffer->size;
	} else {
		ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));
		*addr = paddr;
		*len = buffer->size;
	}

	return 0;
}
#endif

static void secsg_free(struct ion_secsg_heap *secsg_heap,
			struct ion_buffer *buffer)
{
	secsg_debug("%s: enter, free size 0x%zx\n", __func__, buffer->size);
	if (secsg_heap->heap_attr == HEAP_PROTECT)
		__secsg_free_scatter(secsg_heap, buffer);
	else
		__secsg_free_contig(secsg_heap, buffer);
	secsg_debug("%s: exit\n", __func__);
}

static int ion_secsg_heap_allocate(struct ion_heap *heap,
				   struct ion_buffer *buffer,
				   unsigned long size, unsigned long align,
				   unsigned long flags)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	int ret = 0;

	secsg_debug("enter %s  size 0x%lx heap id %u\n",
		    __func__, size, heap->id);
	mutex_lock(&secsg_heap->mutex);

	if (__secsg_heap_input_check(secsg_heap, size, flags)){
		pr_err("input params failed\n");
		ret = -EINVAL;
		goto out;
	}

	/*init the TA conversion here*/
	if(!secsg_heap->TA_init &&
	   (flags & ION_FLAG_SECURE_BUFFER)) {
		ret = secmem_tee_init(secsg_heap->context, secsg_heap->session);
		if (ret) {
			pr_err("[%s] TA init failed\n", __func__);
			goto out;
		}
		secsg_heap->TA_init = 1;
	}

	/* SMMU pgtable init */
	if (!secsg_heap->iova_init && secsg_heap->pgtable_phys) {
		ret = __secsg_pgtable_init(secsg_heap);
		if (ret)
			goto out;
		secsg_heap->iova_init = 1;
	}

	if (secsg_alloc(secsg_heap, buffer, size)) {/*lint !e838*/
		ret = -ENOMEM;
		goto out;
	}
	secsg_heap->alloc_size += size;
	secsg_debug("secsg heap alloc succ, heap all alloc_size 0x%lx\n",
		    secsg_heap->alloc_size);
	mutex_unlock(&secsg_heap->mutex);
	return 0;
out:
	pr_err("heap[%d] alloc fail, size 0x%lx, heap all alloc_size 0x%lx\n",
	       heap->id, size, secsg_heap->alloc_size);
	mutex_unlock(&secsg_heap->mutex);

	if (ret == -ENOMEM)
		hisi_ion_memory_info(true);

	return ret;
}/*lint !e715*/

static void ion_secsg_heap_free(struct ion_buffer *buffer)
{
	struct ion_heap *heap = buffer->heap;
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/

	secsg_debug("%s:enter, heap %d, free size:0x%zx,\n",
		    __func__, heap->id, buffer->size);
	mutex_lock(&secsg_heap->mutex);
	secsg_free(secsg_heap, buffer);
	secsg_debug("out %s:heap remaining allocate %lx\n",
		    __func__, secsg_heap->alloc_size);
	mutex_unlock(&secsg_heap->mutex);
}/*lint !e715*/

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
static int ion_secsg_heap_phys(struct ion_heap *heap,
				struct ion_buffer *buffer,
				ion_phys_addr_t *addr, size_t *len)
{
	/* keep the input parames for compatible with other heaps*/
	/* TZ driver can call "ion_phys" with ion_handle input*/
	struct sg_table *table = buffer->priv_virt;
	struct page *page = sg_page(table->sgl);
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/

	if(secsg_heap->heap_attr == HEAP_PROTECT) {
		*addr = buffer->id;
		*len = buffer->size;
	} else {
		ion_phys_addr_t paddr = PFN_PHYS(page_to_pfn(page));
		*addr = paddr;
		*len = buffer->size;
	}

	return 0;
}/*lint !e715*/
#endif

static int ion_secsg_heap_map_user(struct ion_heap *heap,
				   struct ion_buffer *buffer,
				   struct vm_area_struct *vma)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("secure buffer, can not call %s\n", __func__);
		return -EINVAL;
	}
	return ion_heap_map_user(heap, buffer, vma);
}

static void *ion_secsg_heap_map_kernel(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("secure buffer, can not call %s\n", __func__);
		return NULL;
	}
	return ion_heap_map_kernel(heap, buffer);
}

static void ion_secsg_heap_unmap_kernel(struct ion_heap *heap,
					struct ion_buffer *buffer)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		pr_err("secure buffer, can not call %s\n", __func__);
		return;
	}
	ion_heap_unmap_kernel(heap, buffer);
}

static int ion_secsg_heap_map_iommu(struct ion_buffer *buffer,
				    struct ion_iommu_map *map_data)
{
	struct ion_heap *heap = buffer->heap;
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/
	int ret = 0;

	if(secsg_heap->heap_attr == HEAP_PROTECT) {
		ret = secsg_map_iommu(secsg_heap, buffer, map_data);
		if (ret)
			pr_err("%s:protect map iommu fail\n", __func__);
		return ret;
	} else if (secsg_heap->heap_attr == HEAP_SECURE) {
		pr_err("%s:sec or protect buffer can't map iommu\n", __func__);
		return -EINVAL;
	} else {
		return ion_heap_map_iommu(buffer, map_data);
	}
}

static void ion_secsg_heap_unmap_iommu(struct ion_iommu_map *map_data)
{
	struct ion_buffer *buffer = map_data->buffer;
	struct ion_heap *heap = buffer->heap;
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/

	if(secsg_heap->heap_attr == HEAP_PROTECT)
		secsg_unmap_iommu(secsg_heap, buffer, map_data);
	else if (secsg_heap->heap_attr == HEAP_SECURE)
		pr_err("[%s]secure buffer, do nothing.\n", __func__);
	else
		ion_heap_unmap_iommu(map_data);
}

#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
/*lint -save -e715 */
static struct sg_table *ion_secsg_heap_map_dma(struct ion_heap *heap,
						struct ion_buffer *buffer)
{
	return buffer->priv_virt;
}

static void ion_secsg_heap_unmap_dma(struct ion_heap *heap,
				     struct ion_buffer *buffer)
{
}
/*lint -restore*/
#endif

static struct ion_heap_ops secsg_heap_ops = {
	.allocate = ion_secsg_heap_allocate,
	.free = ion_secsg_heap_free,
#if (LINUX_VERSION_CODE < KERNEL_VERSION(4, 9, 0))
	.phys = ion_secsg_heap_phys,
	.map_dma = ion_secsg_heap_map_dma,
	.unmap_dma = ion_secsg_heap_unmap_dma,
#endif
	.map_user = ion_secsg_heap_map_user,
	.map_kernel = ion_secsg_heap_map_kernel,
	.unmap_kernel = ion_secsg_heap_unmap_kernel,
	.map_iommu = ion_secsg_heap_map_iommu,
	.unmap_iommu = ion_secsg_heap_unmap_iommu,
};/*lint !e785*/

static void __secsg_parse_dt_cma_para(struct device_node *nd,
				      struct ion_secsg_heap *secsg_heap)
{
	const char *compatible;
	struct device_node *phandle_node;
	u32 cma_type = 0;
	int ret = 0;

	phandle_node = of_parse_phandle(nd, "cma-region", 0);
	if (phandle_node) {
		ret = of_property_read_string(phandle_node,
					      "compatible",
					      &compatible);
		if (ret) {
			pr_err("no compatible\n");
			return;
		}
		secsg_heap->cma_name = compatible;
	} else {
		secsg_heap->cma_name = "hisi-cma-pool";
		pr_err("no cma-region phandle\n");
	}
	ret = of_property_read_u32(nd, "cma-type", &cma_type);
	if (ret < 0) {
		secsg_heap->cma_type = CMA_DYNAMIC;
		pr_err("can't find prop:cma-type\n");
	} else {
		secsg_heap->cma_type = cma_type;
	}
}
static int __secsg_parse_dt(struct device *dev,
			    struct ion_platform_heap *heap_data,
			    struct ion_secsg_heap *secsg_heap)
{
	struct device_node *nd;
	struct device_node *mem_region;
	struct resource res;
	u64 per_bit_sz = 0;
	u64 per_alloc_sz = 0;
	u64 water_mark = 0;
	u32 heap_attr = 0;
	u32 pool_shift = ION_PBL_SHIFT;
	int ret = 0;

	nd = of_get_child_by_name(dev->of_node, heap_data->name);
	if (!nd) {
		pr_err("can't of_get_child_by_name %s\n", heap_data->name);
		ret = -EINVAL;
		goto out;
	}

	mem_region = of_parse_phandle(nd, "memory-region", 0);
	if (mem_region) {
		ret = of_address_to_resource(mem_region, 0, &res);
		of_node_put(mem_region);
		if (ret) {
			pr_err("failed to parse memory-region: %d\n", ret);
			goto out;
		}
		secsg_heap->pgtable_phys = res.start;
		secsg_heap->pgtable_size = resource_size(&res);
	} else {
		secsg_heap->pgtable_phys = 0;
		secsg_heap->pgtable_size = 0;
		pr_err("no memory-region phandle\n");
	}

	__secsg_parse_dt_cma_para(nd, secsg_heap);

	ret = of_property_read_u64(nd, "per-alloc-size", &per_alloc_sz);
	if (ret < 0) {
		pr_err("can't find prop:per-alloc-size\n");
		goto out;
	}
	secsg_heap->per_alloc_sz = PAGE_ALIGN(per_alloc_sz);

	ret = of_property_read_u64(nd, "per-bit-size", &per_bit_sz);
	if (ret < 0) {
		pr_err("can't find prop:per-bit-size\n");
		goto out;
	}
	secsg_heap->per_bit_sz = PAGE_ALIGN(per_bit_sz);

	ret = of_property_read_u64(nd, "water-mark", &water_mark);
	if (ret < 0) {
		pr_err("can't find prop:water-mark\n");
		water_mark = 0;
	}
	secsg_heap->water_mark = PAGE_ALIGN(water_mark);

	ret = of_property_read_u32(nd, "pool-shift", &pool_shift);
	if (ret < 0) {
		pr_err("can not find pool-shift.\n");
		pool_shift = ION_PBL_SHIFT;
	}
	secsg_heap->pool_shift = pool_shift;

	ret = of_property_read_u32(nd, "heap-attr", &heap_attr);
	if (ret < 0) {
		pr_err("can not find heap-arrt.\n");
		heap_attr = HEAP_NORMAL;
	}
	if (heap_attr >= HEAP_MAX)
		heap_attr = HEAP_NORMAL;
	secsg_heap->heap_attr = heap_attr;

out:
	return ret;
}

static struct cma *__secsg_heap_getcma(struct ion_secsg_heap *secsg_heap)
{
	int i = 0;

	for (i = 0; i < SEC_SG_CMA_NUM; i++) {
		if (secsg_heap->cma_name &&
		    hisi_secsg_cmas[i].cma_name &&
		    (!strcmp(secsg_heap->cma_name,
			     hisi_secsg_cmas[i].cma_name)))
			return hisi_secsg_cmas[i].cma_region;
	}

	return NULL;
}

struct ion_heap *ion_secsg_heap_create(struct ion_platform_heap *heap_data)
{
	int ret;
	struct device *dev;
	struct ion_secsg_heap *secsg_heap;

	secsg_heap = kzalloc(sizeof(*secsg_heap), GFP_KERNEL);
	if (!secsg_heap)
		return ERR_PTR(-ENOMEM);/*lint !e747*/

	mutex_init(&secsg_heap->mutex);

	secsg_heap->pool = NULL;
	secsg_heap->heap.ops = &secsg_heap_ops;
	secsg_heap->heap.type = ION_HEAP_TYPE_SECSG;
	secsg_heap->heap_size = heap_data->size;
	secsg_heap->alloc_size = 0;
	dev = heap_data->priv;
	INIT_LIST_HEAD(&secsg_heap->allocate_head);

	ret = __secsg_parse_dt(dev, heap_data, secsg_heap);
	if (ret)
		goto free_heap;

	secsg_heap->cma = __secsg_heap_getcma(secsg_heap);

	if (!secsg_heap->cma) {
		pr_err("%s, can't get cma! cma_type:%u, cma_name:%s\n",
		       __func__, secsg_heap->cma_type, secsg_heap->cma_name);
		goto free_heap;
	}

	if ((secsg_heap->heap_attr == HEAP_SECURE) ||
	    (secsg_heap->heap_attr == HEAP_PROTECT)) {
		secsg_heap->context = kzalloc(sizeof(TEEC_Context), GFP_KERNEL);
		if (!secsg_heap->context)
			goto free_heap;
		secsg_heap->session = kzalloc(sizeof(TEEC_Session), GFP_KERNEL);
		if (!secsg_heap->session)
			goto free_context;
	} else {
		secsg_heap->context = NULL;
		secsg_heap->session = NULL;
	}
	secsg_heap->TA_init = 0;
	secsg_heap->iova_init = 0;

	ret = __secsg_create_pool(secsg_heap);
	if (ret) {
		pr_err("[%s] pool create failed.\n", __func__);
		goto free_session;
	}

	if (secsg_heap->water_mark &&
	    __secsg_fill_watermark(secsg_heap))
		pr_err("__secsg_fill_watermark failed!\n");

	pr_err("secsg heap info %s:\n"
		  "\t\t\t\t heap id : %u\n"
		  "\t\t\t\t heap cmatype : %u\n"
		  "\t\t\t\t heap cmaname : %s\n"
		  "\t\t\t\t heap attr : %u\n"
		  "\t\t\t\t pool shift : %u\n"
		  "\t\t\t\t heap size : %lu MB\n"
		  "\t\t\t\t per alloc size :  %llu MB\n"
		  "\t\t\t\t per bit size : %llu KB\n"
		  "\t\t\t\t water_mark size : %llu MB\n"
		  "\t\t\t\t cma base : 0x%llx\n"
		  "\t\t\t\t cma size : 0x%lx\n",
		  heap_data->name,
		  heap_data->id,
		  secsg_heap->cma_type,
		  secsg_heap->cma_name,
		  secsg_heap->heap_attr,
		  secsg_heap->pool_shift,
		  secsg_heap->heap_size / SZ_1M,
		  secsg_heap->per_alloc_sz / SZ_1M,
		  secsg_heap->per_bit_sz / SZ_1K,
		  secsg_heap->water_mark / SZ_1M,
		  cma_get_base(secsg_heap->cma),
		  cma_get_size(secsg_heap->cma));

	return &secsg_heap->heap;/*lint !e429*/
free_session:
	if (secsg_heap->session)
		kfree(secsg_heap->session);
free_context:
	if (secsg_heap->context)
		kfree(secsg_heap->context);
free_heap:
	kfree(secsg_heap);
	return ERR_PTR(-ENOMEM);/*lint !e747*/
}

void ion_secsg_heap_destroy(struct ion_heap *heap)
{
	struct ion_secsg_heap *secsg_heap =
		container_of(heap, struct ion_secsg_heap, heap);/*lint !e826*/

	if (secsg_heap->TA_init) {
		secmem_tee_destroy(secsg_heap->context, secsg_heap->session);
		secsg_heap->TA_init = 0;
	}

	if (secsg_heap->context)
		kfree(secsg_heap->context);
	if (secsg_heap->session)
		kfree(secsg_heap->session);
	kfree(secsg_heap);
}
