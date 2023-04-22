/*
 * ion_sec_priv.h
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

#ifndef _ION_SEC_PRIV_H
#define _ION_SEC_PRIV_H

#include <linux/err.h>
#include <teek_client_api.h>
#include <teek_client_id.h>
#include <teek_client_constants.h>

#include "ion.h"
#include "ion_priv.h"

enum SECSG_HEAP_TYPE {
	HEAP_NORMAL = 0,
	HEAP_PROTECT = 1,
	HEAP_SECURE = 2,
	HEAP_MAX,
};

enum SEC_Task {
	SEC_TASK_DRM = 0x0,
	SEC_TASK_SEC,
	SEC_TASK_MAX,
};

enum SECSG_CMA_TYPE {
	CMA_STATIC = 0,
	CMA_DYNAMIC = 1,
	CMA_TYPE_MAX,
};

#define ION_PBL_SHIFT 12
#define ION_NORMAL_SHIFT 16

#define SECBOOT_CMD_ID_MEM_ALLOCATE 0x1

#ifdef CONFIG_HISI_ION_SECSG_DEBUG
#define secsg_debug(fmt, ...) \
	pr_info(fmt, ##__VA_ARGS__)
#else
#define secsg_debug(fmt, ...)
#endif

#define SEC_SG_CMA_NUM 4

struct ion_secsg_heap {
	struct ion_heap heap;
	struct cma *cma;
	struct gen_pool *pool;
	/* heap mutex */
	struct mutex mutex;
	/* heap attr: secure, protect, non_sec */
	u32 heap_attr;
	u32 pool_shift;
	/* heap total size*/
	size_t heap_size;
	/* heap allocated size*/
	unsigned long alloc_size;
	/* heap flag */
	u64 flag;
	u64 per_alloc_sz;
	/* align size = 64K*/
	u64 per_bit_sz;
	u64 water_mark;
	phys_addr_t pgtable_phys;
	u32 pgtable_size;
	struct list_head allocate_head;
	TEEC_Context *context;
	TEEC_Session *session;
	int iova_init;
	int TA_init;
	u32 cma_type;
	const char *cma_name;
	struct gen_pool *static_cma_region;
};

struct ion_sec_cma {
	struct cma *cma_region;
	const char *cma_name;
};

struct mem_chunk_list {
	u32 protect_id;
	union {
		u32 nents;
		u32 buff_id;
	};
	u32 va;
	void *phys_addr;  /* Must be the start addr of struct tz_pageinfo */
	u32 size;
};

struct tz_pageinfo {
	u64 addr;
	u32 nr_pages;
} __aligned(8);

struct alloc_list {
	u64 addr;
	u32 size;
	struct list_head list;
};

int secmem_tee_init(TEEC_Context *context, TEEC_Session *session);

int secmem_tee_exec_cmd(TEEC_Session *session,
		       struct mem_chunk_list *mcl, u32 cmd);

void secmem_tee_destroy(TEEC_Context *context, TEEC_Session *session);

int __secsg_alloc_contig(struct ion_secsg_heap *secsg_heap,
			 struct ion_buffer *buffer, unsigned long size);
void __secsg_free_contig(struct ion_secsg_heap *secsg_heap,
			 struct ion_buffer *buffer);
int __secsg_fill_watermark(struct ion_secsg_heap *secsg_heap);

int __secsg_alloc_scatter(struct ion_secsg_heap *secsg_heap,
			  struct ion_buffer *buffer, unsigned long size);
void __secsg_free_scatter(struct ion_secsg_heap *secsg_heap,
			  struct ion_buffer *buffer);
int secsg_map_iommu(struct ion_secsg_heap *secsg_heap,
		    struct ion_buffer *buffer,
		    struct ion_iommu_map *map_data);
void secsg_unmap_iommu(struct ion_secsg_heap *secsg_heap,
		       struct ion_buffer *buffer,
		       struct ion_iommu_map *map_data);
#endif
