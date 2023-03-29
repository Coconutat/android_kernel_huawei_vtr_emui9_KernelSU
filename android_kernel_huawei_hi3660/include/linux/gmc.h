/*
 * Copyright 2016 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Authors:
 *   Alexander Yashchenko <a.yashchenko@samsung.com>
 *   Sergei Rogachev <s.rogachev@samsung.com>
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
#ifndef __GMC_H__
#define __GMC_H__

#include <linux/types.h>
#include <linux/gmc_storage.h>

#define GMC_FS_MAX_DENTRIES 4

enum _gmc_op{
	GMC_COMPRESS,
	GMC_DECOMPRESS
};
typedef enum _gmc_op gmc_op;

struct gmc_fs {
	struct proc_dir_entry *dentries[GMC_FS_MAX_DENTRIES];
};

struct gmc_device {
	struct list_head list;

	struct gmc_ops     *ops;
	struct gmc_storage *storage;

	struct gmc_fs      fs;
};

struct gmc_ops {
	int (*compress_kctx) (pid_t, struct gmc_device *,long);
	int (*decompress_kctx) (pid_t, struct gmc_device *);
	int (*meminfo_open)(struct inode *in, struct file *file);
};

int gmc_register_device(struct gmc_ops *gmc_operations,
		struct gmc_device *device);

#endif
