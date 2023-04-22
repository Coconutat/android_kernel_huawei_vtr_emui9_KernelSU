/*
 * Copyright (C) 2016 Hisiliocn Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_HISI_HW_BREAKPOINT_H
#define __ASM_HISI_HW_BREAKPOINT_H

#define AARCH64_DBG_REG_BVR	0

extern void arch_read_wvr(u64 *buf);
extern void arch_read_wcr(u64 *buf);

#endif	/* __ASM_HISI_HW_BREAKPOINT_H */
