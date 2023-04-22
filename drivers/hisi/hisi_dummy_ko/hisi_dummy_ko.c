/*
 * hisi_dummy_ko.c for vts ProcModulesTest, which parsing /proc/modules,
 * require at least one entry.
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
 *
 * Author: chenjun <chenjun14@huawei.com>
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
#include <linux/init.h>
#include <linux/module.h>

MODULE_LICENSE("GPL");

static int hisi_dummy_ko_init(void)
{
	return 0;
}

static void hisi_dummy_ko_exit(void)
{
}

module_init(hisi_dummy_ko_init);
module_exit(hisi_dummy_ko_exit);
