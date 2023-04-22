/*
 * hw_ptrace_log.h
 *
 * Huawei Kernel Harden
 *
 * Copyright (c) 2017-, Huawei Tech. Co., Ltd. All rights reserved.
 *
 *	yinyouzhan<yinyouzhan@huawei.com>
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

void record_ptrace_info_before_return(long request, struct task_struct *child);
