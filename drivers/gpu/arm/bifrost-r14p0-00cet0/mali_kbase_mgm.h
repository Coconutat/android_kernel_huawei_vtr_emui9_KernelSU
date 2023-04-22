/*
 *
 * (C) COPYRIGHT 2018 HISI Limited. All rights reserved.
 *
 * This program is free software and is provided to you under the terms of the
 * GNU General Public License version 2 as published by the Free Software
 * Foundation, and any use by you of this program is subject to the terms
 * of such GNU licence.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you can access it online at
 * http://www.gnu.org/licenses/gpl-2.0.html.
 *
 * SPDX-License-Identifier: GPL-2.0
 *
 */

#ifndef _HISI_MEMORY_GROUP_MANAGER_H_
#define _HISI_MEMORY_GROUP_MANAGER_H_

#include <linux/kernel.h>

enum memory_group_manager_memory_type {
	MEMORY_GROUP_MANAGER_MEMORY_TYPE_DMA_BUF
};

/* Import type and dma_buf of memory group manager */
struct memory_group_manager_import_data {
	enum memory_group_manager_memory_type type;
	struct dma_buf *dma_buf;
};

struct memory_group_manager_device;
/**
 * struct memory_group_manager_ops - Callbacks for memory manager operations
 *
 * @mgm_alloc_pages:        Callback to allocate a page of memory for device
 * @mgm_free_pages:         Callback to free a page of memory for device
 * @mgm_get_import_mem_id:  Callback to get the physical memory group ID for the imported memory
 * @mgm_gpu_map_page:       Callback to map a physical address to the GPU
 * @mgm_vm_insert_pfn_prot: Callback to map a range of physical address to the CPU
 */
struct memory_group_manager_ops {

	/**
	 * mgm_alloc_pages -- Allocate a page of memory
	 *
	 * @mgm_dev: The memory group manager the request is being made through.
	 * @id: The physical memory group ID for the page.
	 * @gfp_mask: Allocation flags.
	 * @order: The page order.
	 *
	 * Return: The struct page of the allocated page.
	 */
	struct page* (*mgm_alloc_pages)(struct memory_group_manager_device *mgm_dev,
								u8 id, gfp_t gfp_mask, unsigned int order);

	/**
	 * mgm_free_pages -- Free a page of memory
	 *
	 * @mgm_dev: The memory group manager the request is being made through.
	 * @id: The physical memory group ID the page was allocated with.
	 * @page: The page to free.
	 * @order: The page order.
	 */
	void (*mgm_free_pages)(struct memory_group_manager_device *mgm_dev,
					   u8 id, struct page *page, unsigned int order);

	/**
	 *get_import_memory_id -- Get the physical memory group ID for the imported memory
	 *
	 * @mgm_dev: The memory group manager the request is being made through.
	 * @data: Data which describes the imported memory.
	 *
	 * Note that provision of this call back is optional, where it is not provided
	 * this call back pointer must be set to NULL to indicate it is not in use.
	 *
	 * Return: The memory group ID to use when mapping pages from this imported memory.
	 */
	u8 (*get_import_memory_id)(struct memory_group_manager_device *mgm_dev,
	 						   struct memory_group_manager_import_data *data);


	/**
	 * gpu_map_page -- Map a physical address to the GPU
	 *
	 * This function can be used to encode extra information into the GPU's page table
	 * entry.
	 * @mmu_level: The level the page table is being built for.
	 * @pte: The prebuilt page table entry from KBase, either in lpae or aarch64
	 * depending on the drivers configuration. This should be decoded to determine the
	 * physical address and other properties of the mapping the manager requires.
	 *
	 * Return: A page table entry prot for KBase to store in the page tables.
	 */
	u64 (*gpu_map_page)(u8 mmu_level, u64 pte);

	/**
	 * vm_insert_pfn_prot -- Map a physical address to the CPU
	 *
	 * Note: Unlike gpu_map_page this function must do the work of writing the CPU's
	 * page table entry.
	 * @id: The physical memory group identifier.
	 * @page_nr: The Page Frame Number to map.
	 * @pgprot: pgprot flags for the inserted page.
	 */
	int (*vm_insert_pfn_prot)(struct page **pages, unsigned long page_nr, pgprot_t *prot);
};

/**
 * struct memory_group_manager_device - Device structure for memory group manager devices
 *
 * @ops  - Callbacks associated with this device
 * @data - Pointer to device private data
 *
 * This structure should be registered with the platform device using
 * platform_set_drvdata().
 */
struct memory_group_manager_device {
	struct memory_group_manager_ops ops;
	void *data;
};

#endif /* _HISI_MEMORY_GROUP_MANAGER_H_ */
