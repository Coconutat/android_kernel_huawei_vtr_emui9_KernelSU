/*
 * livepatch.h - arm64-specific Kernel Live Patching Core
 *
 * Copyright (C) 2014 Li Bin <huawei.libin@huawei.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ASM_ARM64_LIVEPATCH_H
#define _ASM_ARM64_LIVEPATCH_H

#include <linux/module.h>
#include <linux/livepatch.h>

struct klp_patch;

#ifdef CONFIG_LIVEPATCH
static inline int klp_check_compiler_support(void)
{
	return 0;
}
extern int klp_write_module_reloc(struct module *mod, unsigned long type,
				  unsigned long loc, unsigned long value);
extern int klp_check_calltrace(struct klp_patch *patch, int enable);
#else
#error Live patching support is disabled; check CONFIG_LIVEPATCH
#endif

#endif /* _ASM_ARM64_LIVEPATCH_H */
