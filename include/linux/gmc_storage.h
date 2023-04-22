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
#ifndef __GMC_STORAGE_H__
#define __GMC_STORAGE_H__

#include <linux/cpu.h>
#include <linux/zpool.h>
#include <linux/crypto.h>
#include <linux/spinlock.h>
#include <linux/highmem.h>
#include <linux/pagemap.h>
#include <linux/gfp.h>
#include <linux/atomic.h>

/**
 * struct gmc_storage_stat - storage statistics.
 *
 * @compr_data_size: size of the whole compressed data in the storage;
 * @nr_pages:        number of pages in the storage;
 * @nr_zero_pages:   number of zeroed pages in the storage;
 */
struct gmc_storage_stat {
	atomic64_t compr_data_size;
	atomic64_t nr_pages;
	atomic64_t nr_zero_pages;
};

/**
 * struct gmc_storage - compressed pages storage abstraction.
 *
 * @zpool: a pointer to compressed objects pool;
 * @zpool_lock: spinlock used to protect pool allocation/deallocation;
 * @stat: an internal statistics of the storage.
 */
struct gmc_storage {
	struct zpool            *zpool;
	struct spinlock          zpool_lock;
	struct gmc_storage_stat  stat;
};

struct gmc_storage_handle;

struct gmc_storage_handle *gmc_storage_put_page(struct gmc_storage *storage,
		struct page *page);

int gmc_storage_get_page(struct gmc_storage *storage,
		struct page *page, struct gmc_storage_handle *handle);

void gmc_storage_invalidate_page(struct gmc_storage *storage,
		struct gmc_storage_handle *handle);

struct gmc_storage *gmc_storage_create(void);
void gmc_storage_destroy(struct gmc_storage *storage);

#endif
