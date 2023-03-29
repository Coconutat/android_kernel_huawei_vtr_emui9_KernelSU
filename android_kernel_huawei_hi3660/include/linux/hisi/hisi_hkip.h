/*
 * HKIP hooks
 *
 * Copyright (C) 2017 Huawei Technologies
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#ifndef LINUX_HKIP_H
#define LINUX_HKIP_H

#include <asm/alternative.h>
#include <asm/cpufeature.h>
#include <linux/sched.h>
#include <linux/threads.h>
#include <linux/types.h>
#include <linux/hisi/hisi_hhee.h>

bool hhee_is_present(void);

u32 hkip_hvc3(u32, unsigned long, unsigned long);
u32 hkip_hvc4(u32, unsigned long, unsigned long, unsigned long);
u32 hkip_hvc_dispatch(u32, unsigned long, unsigned long, unsigned long,
		unsigned long, unsigned long, unsigned long);

static inline bool hkip_get_bit(const u8 *bits, size_t pos, size_t max)
{
	if (unlikely(pos >= max))
		return false;
	return (READ_ONCE(bits[pos / 8]) >> (pos % 8)) & 1;
}

static inline void hkip_set_bit(u8 *bits, size_t pos, size_t max, bool value)
{
	if (hkip_get_bit(bits, pos, max) == value)
		return;

	if (unlikely(hkip_hvc4(HKIP_HVC_ROWM_SET_BIT, (unsigned long)bits,
				pos, value)))
		BUG();
}

static inline bool __hkip_get_task_bit(const u8 *bits,
					struct task_struct *task)
{
	return hkip_get_bit(bits, task_pid_nr(task), PID_MAX_DEFAULT);
}

static inline bool hkip_get_task_bit(const u8 *bits, struct task_struct *task,
					bool def_value)
{
	pid_t pid = task_pid_nr(task);
	if (pid != 0)
		return hkip_get_bit(bits, pid, PID_MAX_DEFAULT);
	return def_value;
}

static inline void hkip_set_task_bit(u8 *bits, struct task_struct *task,
					bool value)
{
	pid_t pid = task_pid_nr(task);
	if (pid != 0)
		hkip_set_bit(bits, pid, PID_MAX_DEFAULT, value);
}

static inline bool __hkip_get_current_bit(const u8 *bits)
{
	return __hkip_get_task_bit(bits, current);
}

static inline bool hkip_get_current_bit(const u8 *bits, bool def_value)
{
	return hkip_get_task_bit(bits, current, def_value);
}

static inline void hkip_set_current_bit(u8 *bits, bool value)
{
	hkip_set_task_bit(bits, current, value);
}

struct cred;

#ifdef CONFIG_HISI_HHEE
int hkip_register_ro(const void *base, size_t size);
int hkip_register_ro_mod(const void *base, size_t size);
int hkip_unregister_ro_mod(const void *base, size_t size);

extern u8 hkip_addr_limit_bits[];

#define hkip_is_kernel_fs() \
	((current_thread_info()->addr_limit == KERNEL_DS) \
	&& hkip_get_current_bit(hkip_addr_limit_bits, true))
#define hkip_get_fs() \
	(hkip_is_kernel_fs() ? KERNEL_DS : USER_DS)
#define hkip_set_fs(fs) \
	hkip_set_current_bit(hkip_addr_limit_bits, (fs) == KERNEL_DS)

int hkip_check_uid_root(void);
int hkip_check_gid_root(void);
int hkip_check_xid_root(void);

void hkip_init_task(struct task_struct *task);
void hkip_update_xid_root(const struct cred *creds);

#else

#define hkip_set_fs(fs) ((void)(fs))

static inline int hkip_register_ro(const void *base, size_t size)
{
	return 0;
}

static inline int hkip_register_ro_mod(const void *base, size_t size)
{
	return 0;
}

static inline int hkip_unregister_ro_mod(const void *base, size_t size)
{
	return 0;
}

static inline int hkip_check_uid_root(void)
{
	return 0;
}
static inline int hkip_check_gid_root(void)
{
	return 0;
}
static inline int hkip_check_xid_root(void)
{
	return 0;
}

static inline void hkip_init_task(struct task_struct *task) { }
static inline void hkip_update_xid_root(const struct cred *creds) { }

#endif

#endif /* LINUX_HKIP_H */
