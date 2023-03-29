/*
 *
 * (C) COPYRIGHT 2017 ARM Limited. All rights reserved.
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
#ifndef _MALI_KBASE_HISILICON_H_
#define _MALI_KBASE_HISILICON_H_

#include <mali_kbase_mgm.h>
#include "mali_kbase_hisi_callback.h"
#ifdef CONFIG_HISI_LAST_BUFFER
#include <last_buffer/mali_kbase_hisi_lb_mem_pools.h>
#endif

/**
 * struct kbase_hisi_device_data - all hisi platform data in device level.
 *
 * @callbacks: The callbacks hisi implements.
 * @mgm_dev: The memory_group_manager_device used to alloc/free memory, etc.
 *           We can use this dev to alloc normal memory or last buffer memory.
 * @mgm_ops: The operation interfaces of @mgm_dev.
 * @lb_pools: The info related with device's last_buffer memory pools.
 * @cache_policy_info: The list of cache policy info object.
 * @nr_cache_policy: Number of entries in the list of cache_policy_info.
 */
struct kbase_hisi_device_data {
	struct kbase_hisi_callbacks *callbacks;
	struct memory_group_manager_device *mgm_dev;
	struct memory_group_manager_ops *mgm_ops;

	/* Add other device data here */

	/* The data related with last buffer. */
#ifdef CONFIG_HISI_LAST_BUFFER
	struct kbase_hisi_lb_pools lb_pools;
	struct kbase_mem_cache_policy_info *cache_policy_info;
	u8 nr_cache_policy;
#endif
};

/**
 * struct kbase_hisi_ctx_data - all hisi platform data in context level.
 *
 * @lb_pools: The info related with context's last_buffer memory pools.
 *            Contains the last buffer's memory pools with different policy id
 *            and corresponding lock.
 */
struct kbase_hisi_ctx_data {

/* Add other context data here */

#ifdef CONFIG_HISI_LAST_BUFFER
	struct kbase_hisi_lb_pools lb_pools;
#endif
};

#endif /* _MALI_KBASE_HISILICON_H_ */
