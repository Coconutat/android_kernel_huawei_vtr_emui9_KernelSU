/*
 * pmalloc.h: Header for Protectable Memory Allocator
 *
 * (C) Copyright 2017 Huawei Technologies Co. Ltd.
 * Author: Igor Stoppa <igor.stoppa@huawei.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; version 2
 * of the License.
 */

#ifndef _PMALLOC_H
#define _PMALLOC_H
#ifdef CONFIG_HISI_PMALLOC


#include <linux/genalloc.h>
#include <linux/string.h>

#define PMALLOC_DEFAULT_ALLOC_ORDER (-1)

/*
 * Library for dynamic allocation of pools of memory that can be,
 * after initialization, marked as read-only.
 *
 * This is intended to complement __read_only_after_init, for those cases
 * where either it is not possible to know the initialization value before
 * init is completed, or the amount of data is variable and can be
 * determined only at run-time.
 *
 * ***WARNING***
 * The user of the API is expected to synchronize:
 * 1) allocation,
 * 2) writes to the allocated memory,
 * 3) write protection of the pool,
 * 4) freeing of the allocated memory, and
 * 5) destruction of the pool.
 *
 * For a non-threaded scenario, this type of locking is not even required.
 *
 * Even if the library were to provide support for locking, point 2)
 * would still depend on the user taking the lock.
 */


/**
 * pmalloc_create_pool - create a new protectable memory pool -
 * @name: the name of the pool, must be unique
 * @min_alloc_order: log2 of the minimum allocation size obtainable
 *                   from the pool
 *
 * Creates a new (empty) memory pool for allocation of protectable
 * memory. Memory will be allocated upon request (through pmalloc).
 *
 * Returns a pointer to the new pool upon success, otherwise a NULL.
 */
struct gen_pool *pmalloc_create_pool(const char *name,
					 int min_alloc_order);

/**
 * pmalloc_prealloc - tries to allocate a memory chunk of the requested size
 * @pool: handler to the pool to be used for memory allocation
 * @size: amount of memory (in bytes) requested
 *
 * Prepares a chunk of the requested size.
 * This is intended to both minimize latency in later memory requests and
 * avoid sleping during allocation.
 * Memory allocated with prealloc is stored in one single chunk, as
 * opposite to what is allocated on-demand when pmalloc runs out of free
 * space already existing in the pool and has to invoke vmalloc.
 *
 * Returns true if the vmalloc call was successful, false otherwise.
 */
bool pmalloc_prealloc(struct gen_pool *pool, size_t size);

/**
 * pmalloc - allocate protectable memory from a pool
 * @pool: handler to the pool to be used for memory allocation
 * @size: amount of memory (in bytes) requested
 * @gfp: flags for page allocation
 *
 * Allocates memory from an unprotected pool. If the pool doesn't have
 * enough memory, and the request did not include GFP_ATOMIC, an attempt
 * is made to add a new chunk of memory to the pool
 * (a multiple of PAGE_SIZE), in order to fit the new request.
 * Otherwise, NULL is returned.
 *
 * Returns the pointer to the memory requested upon success,
 * NULL otherwise (either no memory available or pool already read-only).
 */
void *pmalloc(struct gen_pool *pool, size_t size, gfp_t gfp);


/**
 * pzalloc - zero-initialized version of pmalloc
 * @pool: handler to the pool to be used for memory allocation
 * @size: amount of memory (in bytes) requested
 * @gfp: flags for page allocation
 *
 * Executes pmalloc, initializing the memory requested to 0,
 * before returning the pointer to it.
 *
 * Returns the pointer to the zeroed memory requested, upon success,
 * NULL otherwise (either no memory available or pool already read-only).
 */
static inline void *pzalloc(struct gen_pool *pool, size_t size, gfp_t gfp)
{
	return pmalloc(pool, size, gfp | __GFP_ZERO);
}

/**
 * pmalloc_array - allocates an array according to the parameters
 * @pool: handler to the pool to be used for memory allocation
 * @size: amount of memory (in bytes) requested
 * @gfp: flags for page allocation
 *
 * Executes pmalloc, if it has a chance to succeed.
 *
 * Returns either NULL or the pmalloc result.
 */
static inline void *pmalloc_array(struct gen_pool *pool, size_t n,
				  size_t size, gfp_t flags)
{
	size_t total_size = n * size;

	if (unlikely(!(pool && n && size && (total_size / n == size))))
	    return NULL;
	return pmalloc(pool, n * size, flags);
}

/**
 * pcalloc - allocates a 0-initialized array according to the parameters
 * @pool: handler to the pool to be used for memory allocation
 * @size: amount of memory (in bytes) requested
 * @gfp: flags for page allocation
 *
 * Executes pmalloc, if it has a chance to succeed.
 *
 * Returns either NULL or the pmalloc result.
 */
static inline void *pcalloc(struct gen_pool *pool, size_t n,
			    size_t size, gfp_t flags)
{
	return pmalloc_array(pool, n, size, flags | __GFP_ZERO);
}

/**
 * pstrdup - duplicate a string, using pmalloc as allocator
 * @pool: handler to the pool to be used for memory allocation
 * @s: string to duplicate
 * @gfp: flags for page allocation
 *
 * Generates a copy of the given string, allocating sufficient memory
 * from the given pmalloc pool.
 *
 * Returns a pointer to the replica, NULL in case of recoverable error.
 */
static inline char *pstrdup(struct gen_pool *pool, const char *s, gfp_t gfp)
{
	size_t len;
	char *buf;

	if (unlikely(pool == NULL || s == NULL))
		return NULL;

	len = strlen(s) + 1;
	buf = pmalloc(pool, len, gfp);
	if (likely(buf))
		strncpy(buf, s, len);/* unsafe_function_ignore: strncpy*/
	return buf;
}

/**
 * pmalloc_protect_pool - turn a read/write pool read-only
 * @pool: the pool to protect
 *
 * Write-protects all the memory chunks assigned to the pool.
 * This prevents any further allocation.
 *
 * Returns 0 upon success, -EINVAL in abnormal cases.
 */
int pmalloc_protect_pool(struct gen_pool *pool);

/**
 * pfree - mark as unused memory that was previously in use
 * @pool: handler to the pool to be used for memory allocation
 * @addr: the beginning of the memory area to be freed
 *
 * The behavior of pfree is different, depending on the state of the
 * protection.
 * If the pool is not yet protected, the memory is marked as unused and
 * will be availabel for further allocations.
 * If the pool is already protected, the memory is marked as unused, but
 * it will still be impossible to perform further allocation, because of
 * the existing protection.
 * The freed memory, in this case, will be truly released only when the
 * pool is destroyed.
 */
void pfree(struct gen_pool *pool, const void *addr);

/**
 * pmalloc_destroy_pool - destroys a pool and all the associated memory
 * @pool: the pool to destroy
 *
 * All the memory that was allocated through pmalloc in the pool will be freed.
 *
 * Returns 0 upon success, -EINVAL in abnormal cases.
 */
int pmalloc_destroy_pool(struct gen_pool *pool);

#else
#include <linux/slab.h>
#include <linux/err.h>

/**
 * These are an alternate version of the pmalloc API, meant mostly for
 * debugging.
 */

#define ERR_PMALLOC_OFF 1
static inline
struct gen_pool *pmalloc_create_pool(const char *name,
				     int min_alloc_order)
{
	return ERR_PTR(ERR_PMALLOC_OFF);
}

static inline
bool pmalloc_prealloc(struct gen_pool *pool, size_t size)
{
	return true;
}

static inline
void *pmalloc(struct gen_pool *pool, size_t size, gfp_t gfp)
{
	return kmalloc(size, gfp);
}

static inline
void *pzalloc(struct gen_pool *pool, size_t size, gfp_t gfp)
{
	return kzalloc(size, gfp);
}

static inline
void *pmalloc_array(struct gen_pool *pool, size_t n,
		    size_t size, gfp_t flags)
{
	return kmalloc_array(n, size, flags);
}

static inline
void *pcalloc(struct gen_pool *pool, size_t n,
		    size_t size, gfp_t flags)
{
	return kcalloc(n, size, flags);
}

static inline
char *pstrdup(struct gen_pool *pool, const char *s, gfp_t gfp)
{
	return kstrdup(s, gfp);
}

static inline
int pmalloc_protect_pool(struct gen_pool *pool)
{
	return 0;
}

static inline
void pfree(struct gen_pool *pool, const void *addr)
{
	kfree(addr);
}

static inline
int pmalloc_destroy_pool(struct gen_pool *pool)
{
	return 0;
}

#endif
#endif
