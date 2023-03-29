/*
 * livepatch-hhee.h - hhee interface for arm64-specific Kernel Live Patching 
 *
 * Copyright (c) 2001-2021, Huawei Tech. Co., Ltd. All rights reserved.
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
#ifndef __ASM_LIVEPATCH_HHEE_H
#define __ASM_LIVEPATCH_HHEE_H

#include <linux/arm-smccc.h>
#include <asm/cacheflush.h>
#include <linux/of.h>
#include <asm/insn.h>
#ifdef CONFIG_HISI_HHEE_TOKEN
#include <linux/hisi/hisi_hhee.h>
#endif

extern unsigned long hkip_token;

enum aarch64_reloc_stage {
	RELOC_MODULE_LOADING,
	RELOC_MODULE_LOADED,
};

static inline bool is_hkip_enabled(void)
{
       bool ret = false;
#ifdef CONFIG_HISI_HHEE_TOKEN
       if (HHEE_ENABLE == hhee_check_enable())
               ret = true;
#endif
       return ret;
}

static inline int aarch64_insn_patch_text_hkip(void *place, u32 insn, unsigned long token)
{
#ifdef CONFIG_HISI_HHEE_TOKEN
	struct arm_smccc_res res;
	arm_smccc_hvc(HHEE_HVC_LIVEPATCH, (unsigned long)place,
			AARCH64_INSN_SIZE, insn, 0, token, 0, 0, &res);
	if(res.a0)
		return -EFAULT;
	flush_icache_range((uintptr_t)place,
			(uintptr_t)place + AARCH64_INSN_SIZE);
#endif
	return 0;
}

static inline int apply_reloc_hkip(void *place, int len, unsigned long val, unsigned long token)
{
#ifdef CONFIG_HISI_HHEE_TOKEN
	struct arm_smccc_res res;
	arm_smccc_hvc(HHEE_HVC_LIVEPATCH, (unsigned long)place, len, val, 0, token,
             0, 0, &res);
	if(res.a0)
		return -EFAULT;
#endif
	return 0;
}

static inline unsigned long get_hkip_token(void)
{
	unsigned long  token = 0;
#ifdef CONFIG_HISI_HHEE_TOKEN
	struct arm_smccc_res res;
	arm_smccc_hvc(HHEE_HVC_TOKEN, 0, 0,
			0, 0, 0, 0, 0, &res);
	if(res.a0)
		return token;
	token = res.a1;
#endif
	return token;
}
#endif

