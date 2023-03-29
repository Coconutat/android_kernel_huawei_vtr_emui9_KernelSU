/*
 * Copyright (c) 2016, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef __HISI_CORE_CTL_H
#define __HISI_CORE_CTL_H

#ifdef CONFIG_HISI_CORE_CTRL
void core_ctl_check(void);
void core_ctl_set_boost(void);
void core_ctl_spread_affinity(cpumask_t *allowed_mask);
#else
static inline void core_ctl_check(void) {}
static inline void core_ctl_set_boost(void) {}
static inline void core_ctl_spread_affinity(cpumask_t *allowed_mask) {}
#endif
#endif
