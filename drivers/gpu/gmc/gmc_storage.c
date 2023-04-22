/*
 * Copyright 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Authors:
 *   Sergei Rogachev <s.rogachev@samsung.com>
 *   Alexander Yashchenko <a.yashchenko@samsung.com>
 *
 * Some ideas were got from a prototype made by Krzysztof Kozlowski.
 *
 * This file is part of GMC (graphical memory compression) framework.
 *
 * GMC is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Midgard GMC is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with GMC. If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * gmc_storage.c - implementation of generic interface for compressed objects
 * storage that can be used by GPU kernel drivers to implement 'native' memory
 * compression facilities.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/gmc_storage.h>
#include "securec.h"

#define GMC_FLAG_ZERO (1 << 0)

/*
 * A threshold for determining badly-compressible pages.
 * Refer to ZRAM source code for a similar formulae.
 */
#define GMC_COMPR_BAD_THR (PAGE_SIZE / 16 * 15)

/**
 * struct gmc_storage_handle - generic handle of compressed page.
 *
 * @handle: a handle of compressed object (zpool handle);
 * @size: a size of compressed object (we can use short integer because a
 * maximum size is just a PAGE_SIZE or 4096 bytes);
 * @flags: flags associated with compressed object.
 */
struct gmc_storage_handle {
	unsigned long  handle;
	unsigned short size;
	unsigned short flags;
};

/* Another possible value: lz4. */
static char *gmc_storage_algorithm_str = "lz4";
/* Another possible value: zbud. */
static char *gmc_storage_allocator_str = "zsmalloc";

static DEFINE_PER_CPU(struct crypto_comp *, gmc_storage_cc);
static DEFINE_PER_CPU(u8 *, gmc_storage_buff);

static struct kmem_cache *gmc_storage_handle_cache;

static struct gmc_storage_handle *gmc_storage_handle_create(void)
{
	struct gmc_storage_handle *handle = NULL;
	int ret = 0;

	handle = kmem_cache_alloc(gmc_storage_handle_cache, GFP_KERNEL);
	if (handle == NULL) {
		pr_err("Unable to allocate storage handle.\n");
		return ERR_PTR(-ENOMEM);
	}

	ret = memset_s(handle, sizeof(*handle), 0, sizeof(*handle));
	if (ret != 0) {
		pr_err("memset_s failed\n");
	}
	handle->size = PAGE_SIZE;

	return handle;
}

static void gmc_storage_handle_destroy(struct gmc_storage_handle *handle)
{
	if (handle == NULL)
		return;

	kmem_cache_free(gmc_storage_handle_cache, handle);
}

static void gmc_storage_handle_set_zeroed(struct gmc_storage_handle *handle)
{
	handle->flags |= GMC_FLAG_ZERO;
}

static int gmc_storage_handle_is_zeroed(struct gmc_storage_handle *handle)
{
	return !!(handle->flags & GMC_FLAG_ZERO);
}

/**
 * is_region_zero_filled() - check if the virtual memory region is filled with
 * zeroes.
 *
 * @p: pointer to a first byte of the region.
 *
 * Returns 1 if the region is fully filled with zeroes and 0 otherwise.
 */
static int is_region_zero_filled(void *p)
{
	unsigned int pos, top_pos;
	unsigned long *page;

	page = (unsigned long *)p;
	top_pos = PAGE_SIZE / sizeof(*page);

	for (pos = 0; pos != top_pos; pos++) {
		if (page[pos])
			return 0;
	}

	return 1;
}

/**
 * is_page_zero_filled() - check if the page is filled with zeroes.
 * @page: pointer to page struct.
 *
 * The function maps data of the page to some virtual address and checks the
 * data on zeroes. Page is additionally locked during this operation.
 *
 * Returns 1 if the page is fully filled with zeroes and 0 otherwise.
 */
static int is_page_zero_filled(struct page *page)
{
	void *p;
	int ret;

	/* It is safe, because we don't do any sleepy stuff here. */
	p = kmap_atomic(page);
	ret = is_region_zero_filled(p);
	kunmap_atomic(p);

	return ret;
}

/**
 * compress_page() - compress a page with selected crypto engine.
 *
 * @page: pointer to page struct;
 * @buff: pointer to first byte of the destination buffer;
 * @dsizep: pointer to variable with compressed data size.
 *
 * Returns 0 on success, error code, otherwise.
 * Modifies the variable pointed by dsizep;
 */
static int compress_page(struct page *page, u8 *buff, unsigned int *dsizep)
{
	struct crypto_comp *ccp;
	void *p;
	int ret;

	ccp = get_cpu_var(gmc_storage_cc);

	p = kmap_atomic(page);
	ret = crypto_comp_compress(ccp, p, PAGE_SIZE, buff, dsizep);
	kunmap_atomic(p);

	put_cpu_var(gmc_storage_cc);

	return ret;
}

/**
 * store_data() - store compressed data to zpool memory area.
 *
 * @storage: pointer to storage object;
 * @buff: pointer to buffer with compressed data;
 * @size: size of compressed data;
 * @handlep: pointer to variable for placing zpool handle.
 *
 * Returns 0 on success, error code otherwise.
 * Modifies a variable pointed by handlep pointer.
 */
static int store_data(struct gmc_storage *storage, u8 *buff, unsigned int size,
		unsigned long *handlep)
{
	void *zpagep = NULL;
	int ret;

	spin_lock(&storage->zpool_lock);
	ret = zpool_malloc(storage->zpool, size, __GFP_NORETRY | __GFP_NOWARN,
			handlep);
	spin_unlock(&storage->zpool_lock);

	if (ret)
		return ret;

	zpagep = zpool_map_handle(storage->zpool, *handlep, ZPOOL_MM_RW);
	memcpy_s(zpagep, size, buff, size);
	zpool_unmap_handle(storage->zpool, *handlep);

	return 0;
}

/**
 * is_well_compressed() - check if the data is well compressed.
 *
 * @size: size of compressed data.
 *
 * Returns 1 if the data is well-compressed and 0 otherwise.
 */
static int is_well_compressed(unsigned int size)
{
	if (size >= GMC_COMPR_BAD_THR)
		return 0;

	return 1;
}

/**
 * gmc_storage_put_page() - put the page to compressed storage.
 * @storage: pointer to gmc_storage object;
 * @page:    pointer to some page to be stored;
 *
 * The function allocates place in compressed data storage, compresses page's
 * data and store the result. Handle of compressed data storage allocation is
 * passed to the user wrapped in opaque container.
 *
 * Zeroed pages are detected and marked the special way.
 *
 * Returns a pointer to newly allocated handle, error otherwise (use PTR_ERR).
 */
struct gmc_storage_handle *gmc_storage_put_page(struct gmc_storage *storage,
		struct page *page)
{
	struct gmc_storage_handle *storage_handle = NULL;
	unsigned long handle = 0;
	unsigned int dsize = 0;
	u8 *buff = NULL;

	int err = 0;

	storage_handle = gmc_storage_handle_create();
	if (IS_ERR(storage_handle))
		return storage_handle;

	if (is_page_zero_filled(page)) {
		gmc_storage_handle_set_zeroed(storage_handle);
		atomic64_inc(&storage->stat.nr_zero_pages);
		goto out;
	}

	buff = get_cpu_var(gmc_storage_buff);

	if (compress_page(page, buff, &dsize)) {
		pr_err("Unable to compress the page.\n");
		err = -EINVAL;
		goto error_put_and_destroy;
	}

	if (!is_well_compressed(dsize)) {
		pr_debug("Badly compressed page, size: %u.\n", dsize);
		err = -EFBIG;
		goto error_put_and_destroy;
	}

	if (store_data(storage, buff, dsize, &handle)) {
		pr_err("Unable to allocate a storage.\n");
		err = -ENOMEM;
		goto error_put_and_destroy;
	}

	put_cpu_var(gmc_storage_buff);

	storage_handle->handle = handle;
	storage_handle->size = dsize;
	atomic64_inc(&storage->stat.nr_pages);
	atomic64_add((long) dsize, &storage->stat.compr_data_size);

out:
	return storage_handle;

error_put_and_destroy:
	put_cpu_var(gmc_storage_buff);
	gmc_storage_handle_destroy(storage_handle);

	return ERR_PTR(err);
}
EXPORT_SYMBOL(gmc_storage_put_page);

/**
 * fill_page_with_zeroes() - fill the page with zeroes.
 *
 * @page: pointer to page struct.
 */
static void fill_page_with_zeroes(struct page *page)
{
	void *p;

	p = kmap_atomic(page);
	memset_s(p, PAGE_SIZE, 0, PAGE_SIZE);
	kunmap_atomic(p);
}

/**
 * decompress_page() - decompress data and put them to page.
 *
 * @page: pointer to page struct;
 * @src: pointer to source data
 * @size: size of data for decompression.
 *
 * Returns 0 on success, error code otherwise.
 */
static int decompress_page(struct page *page, u8 *src, unsigned int size)
{
	unsigned int dsize = PAGE_SIZE;
	struct crypto_comp *cc;
	u8 *dst;
	int ret;

	cc = get_cpu_var(gmc_storage_cc);

	dst = kmap_atomic(page);
	ret = crypto_comp_decompress(cc, src, size, dst, &dsize);
	kunmap_atomic(dst);

	put_cpu_var(gmc_storage_cc);

	return ret;
}

/**
 * gmc_storage_get_page() - get the page from compressed storage.
 * @storage: pointer to gmc_storage object;
 * @page:    pointer to poiner to struct page;
 * @handle:  compressed data handle (gmc_storage_handle opaque struct).
 *
 * Returns 0 on success, error otherwise.
 */
int gmc_storage_get_page(struct gmc_storage *storage,
		struct page *page, struct gmc_storage_handle *handle)
{
	u8 *src = NULL;
	int ret;

	if (gmc_storage_handle_is_zeroed(handle)) {
		fill_page_with_zeroes(page);
		gmc_storage_handle_destroy(handle);
		atomic64_dec(&storage->stat.nr_zero_pages);

		return 0;
	}

	src = zpool_map_handle(storage->zpool, handle->handle, ZPOOL_MM_RO);
	ret = decompress_page(page, src, handle->size);
	zpool_unmap_handle(storage->zpool, handle->handle);

	if (ret) {
		pr_err("Unable to decompress page.\n");
		return -EINVAL;
	}

	spin_lock(&storage->zpool_lock);
	zpool_free(storage->zpool, handle->handle);
	spin_unlock(&storage->zpool_lock);

	atomic64_dec(&storage->stat.nr_pages);
	atomic64_sub((long) handle->size, &storage->stat.compr_data_size);
	gmc_storage_handle_destroy(handle);

	return 0;
}
EXPORT_SYMBOL(gmc_storage_get_page);

/**
 * gmc_storage_invalidate_page() - invalidate the page in compressed storage.
 * @storage: pointer to gmc_storage object;
 * @handle:  pointer to compressed data handle (gmc_storage_handle struct).
 *
 * The function is used to invalidate an entry in compressed storage when it is
 * no longer needed.
 */
void gmc_storage_invalidate_page(struct gmc_storage *storage,
		struct gmc_storage_handle *handle)
{
	if (!gmc_storage_handle_is_zeroed(handle)) {
		spin_lock(&storage->zpool_lock);
		zpool_free(storage->zpool, handle->handle);
		spin_unlock(&storage->zpool_lock);

		atomic64_dec(&storage->stat.nr_pages);
		atomic64_sub((long) handle->size,
				&storage->stat.compr_data_size);
	} else {
		atomic64_dec(&storage->stat.nr_zero_pages);
	}

	gmc_storage_handle_destroy(handle);
}
EXPORT_SYMBOL(gmc_storage_invalidate_page);

static int gmc_storage_zpool_evict(struct zpool *pool, unsigned long handle)
{
	WARN_ONCE(1, "Unsupported try to evict zpool object %lu.\n", handle);
	return -EINVAL;
}

static struct zpool_ops gmc_storage_zpool_ops = {
	.evict = gmc_storage_zpool_evict
};

/**
 * gmc_storage_create() - create a new storage object.
 *
 * Returns a pointer to struct gmc_storage or NULL.
 */
struct gmc_storage *gmc_storage_create(void)
{
	struct gmc_storage *storage = NULL;
	struct zpool *zpool = NULL;

	storage = kmalloc(sizeof *storage, GFP_KERNEL);
	if (storage == NULL)
		goto error;

	zpool = zpool_create_pool(gmc_storage_allocator_str,
			"gmc_storage_zpool", __GFP_NORETRY | __GFP_NOWARN,
			&gmc_storage_zpool_ops);
	if (zpool == NULL) {
		pr_err("Unable to create zpool.\n");
		goto error_free_storage;
	}

	storage->zpool = zpool;
	spin_lock_init(&storage->zpool_lock);

	memset_s(&storage->stat, sizeof(struct gmc_storage_stat), 0, sizeof(struct gmc_storage_stat));

	return storage;

error_free_storage:
	kfree(storage);
error:
	return NULL;
}
EXPORT_SYMBOL(gmc_storage_create);

/**
 * gmc_storage_destroy() - destroy a storage object.
 *
 * @storage: pointer to some storage object.
 */
void gmc_storage_destroy(struct gmc_storage *storage)
{
	if (storage == NULL)
		return;

	zpool_destroy_pool(storage->zpool);
	kfree(storage);
}
EXPORT_SYMBOL(gmc_storage_destroy);

static int gmc_storage_cpu_notifier(struct notifier_block *nb,
		unsigned long notification, void *hcpu)
{
	long cpu = (long)(uintptr_t)hcpu;
	struct crypto_comp *cc = NULL;
	u8 *buff = NULL;

	switch (notification) {
	case CPU_UP_PREPARE:
		cc = crypto_alloc_comp(gmc_storage_algorithm_str, 0, 0);
		if (IS_ERR(cc)) {
			pr_err("Unable to allocate crypto context: %ld\n",
					PTR_ERR(cc));
			return NOTIFY_BAD;
		}
		per_cpu(gmc_storage_cc, cpu) = cc;
		buff = kmalloc_node(PAGE_SIZE << 1, GFP_KERNEL,
				cpu_to_node(cpu));
		if (buff == NULL) {
			pr_err("Unable to allocate compression buffer.\n");
			crypto_free_comp(cc);
			per_cpu(gmc_storage_cc, cpu) = NULL;
			return NOTIFY_BAD;
		}
		per_cpu(gmc_storage_buff, cpu) = buff;
		break;
	case CPU_DEAD:
	case CPU_UP_CANCELED:
		cc = per_cpu(gmc_storage_cc, cpu);
		if (cc != NULL) {
			crypto_free_comp(cc);
			per_cpu(gmc_storage_cc, cpu) = NULL;
		}
		buff = per_cpu(gmc_storage_buff, cpu);
		kfree(buff);
		per_cpu(gmc_storage_buff, cpu) = NULL;
		break;
	default:
		break;
	}
	return NOTIFY_OK;
}

static struct notifier_block gmc_storage_cpu_notifier_block = {
	.notifier_call = gmc_storage_cpu_notifier
};

static __init int gmc_storage_init(void)
{
	long cpu = 0;

	cpu_notifier_register_begin();
	for_each_online_cpu(cpu) {
		if (gmc_storage_cpu_notifier(NULL, CPU_UP_PREPARE, (void *)cpu)
				!= NOTIFY_OK)
			goto backtrack;
	}
	__register_cpu_notifier(&gmc_storage_cpu_notifier_block);
	cpu_notifier_register_done();

	gmc_storage_handle_cache = kmem_cache_create("gmc_storage_handle_cache",
			sizeof (struct gmc_storage_handle),
			__alignof__(struct gmc_storage_handle), 0, NULL);
	if (gmc_storage_handle_cache == NULL) {
		pr_err("Unable to create a cache for GMC storage handles.\n");
		goto backtrack;
	}

	return 0;

backtrack:
	for_each_online_cpu(cpu)
		gmc_storage_cpu_notifier(NULL, CPU_UP_CANCELED, (void *)(uintptr_t)cpu);
	cpu_notifier_register_done();

	return -ENOMEM;
}


module_init(gmc_storage_init);


