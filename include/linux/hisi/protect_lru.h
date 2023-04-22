/*
 * protect_lru.h
 *
 * Provide protect_lru external call interface
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * Xishi Qiu <qiuxishi@hauwei.com>
 * Yisheng Xie <xieyisheng1@hauwei.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef PROTECT_LRU_H
#define PROTECT_LRU_H

#ifdef CONFIG_TASK_PROTECT_LRU
#include <linux/sysctl.h>

extern const struct file_operations proc_protect_lru_operations;
extern struct ctl_table protect_lru_table[];

extern int ioctl_protect_lru_set(struct file *filp, unsigned long arg);
extern int ioctl_protect_lru_get(struct file *filp);

extern struct page *protect_lru_move_and_shrink(struct page *page);

extern void add_page_to_protect_lru_list(struct page *page,
					struct lruvec *lruvec, bool lru_head);
extern void del_page_from_protect_lru_list(struct page *page,
					struct lruvec *lruvec);

extern void protect_lru_set_from_process(struct page *page);
extern void protect_lru_set_from_file(struct page *page);
extern void protect_lruvec_init(struct lruvec *lruvec);
extern void shrink_protect_file(struct lruvec *lruvec, bool force);
extern bool protect_file_is_full(struct lruvec *lruvec);
extern unsigned long protect_lru_enable __read_mostly;
extern unsigned long protect_reclaim_ratio;

static inline bool check_file_page(struct page *page)
{
	/*
	 * When do page migration, the mapping of file page maybe
	 * set to null, however it still need to be charged.
	 */

	/* Skip anon page */
	if (PageSwapBacked(page))
		return false;

	/* Skip mlock page */
	if (PageUnevictable(page))
		return false;

	return true;
}

#endif

#endif
