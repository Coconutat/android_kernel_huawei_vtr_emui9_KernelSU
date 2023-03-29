/*
 * inlinehook.c - inlinehook for functions
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/gfp.h>
#include <linux/vmalloc.h>
#include <linux/types.h>
#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <asm/sections.h>
#include <asm/cacheflush.h>
#include "inlinehook.h"
#include "hook_insn.h"
#include "util.h"

#if defined(__aarch64__)

/* any insn with bit[28:27] = 00 kprobes use 0x07F001F8U */
#define OASES_ARM_UNDEF_INSN 0x07F02016U

/*
 * make_jump_insn() - generate a B instruction to jump from @addr to @dst
 *
 * Return: 0 for success, also set insn, otherwise return -1 when failed
 */
int oases_make_jump_insn(u32 *addr, u32 *dst, u32 *insn)
{
	u32 jump_insn;
	unsigned long offset = 0;
	unsigned long forward_max = 0;
	unsigned long backward_min = 0;

	offset = (unsigned long) dst - (unsigned long) addr;
	/*
	 * Forward offset: 0 - 0x7FFFFFCUL
	 * Backward offset: 0xFFFFFFFFF8000000UL - 0xFFFFFFFFFFFFFFFCUL
	 */
	forward_max = 0x7FFFFFCUL;
	backward_min = 0xFFFFFFFFF8000000UL;
	if (offset > forward_max && offset < backward_min) {
		return -1;
	}

	/* offset = SignExtend(imm26:'00', 64) */
	offset = (offset & 0xFFFFFFFUL) >> 2;
	jump_insn = (u32) 0x14000000UL + (u32) offset;
	*insn = jump_insn;
	return 0;
}

/* location dependent instructions */

/* ADR  <Xd>, <label> */
#define ARM64_ADR       1
/* ADRP <Xd>, <label> */
#define ARM64_ADRP      2
/* B.cond <label> */
#define ARM64_BCOND     3
/* B <label> */
#define ARM64_B         4
/* BL <label> */
#define ARM64_BL        5
/* BLR <Xn> */
#define ARM64_BLR       6
/*CB(N)Z <label>*/
#define ARM64_CBNZ      7
#define ARM64_CBZ       8
/* LDR <Xt>, <label>*/
#define ARM64_LDR_32    9
/* LDR <Wt>, <label>*/
#define ARM64_LDR_64    10
/* LDRSW <Xt>, <label>*/
#define ARM64_LDRSW     11
/* TB(N)Z <R><t>, #<imm>, <label> */
#define ARM64_TBNZ      12
#define ARM64_TBZ       13
/* PRFM (prfop|#<imm5>), <label> */
#define ARM64_PRFM		14
/* BR <Xn> */
#define ARM64_BR        15
/* TODO: */
#define ARM64_UNDEF     99

static int get_insn_type(u32 instruction)
{
	if ((instruction & (u32) 0x9F000000UL) == (u32) 0x10000000UL)
		return ARM64_ADR;

	if ((instruction & (u32) 0x9F000000UL) == (u32) 0x90000000UL)
		return ARM64_ADRP;

	if ((instruction & (u32) 0xFF000010UL) == (u32) 0x54000000UL)
		return ARM64_BCOND;

	if ((instruction & (u32) 0xFC000000UL) == (u32) 0x14000000UL)
		return ARM64_B;

	if ((instruction & (u32) 0xFC000000UL) == (u32) 0x94000000UL)
		return ARM64_BL;

	if ((instruction & (u32) 0xFFFFFC1FUL) == (u32) 0xD63F0000UL)
		return ARM64_BLR;

	if ((instruction & (u32) 0xFF000000UL) == (u32) 0xB5000000UL)
		return ARM64_CBNZ;

	if ((instruction & (u32) 0xFF000000UL) == (u32) 0xB4000000UL)
		return ARM64_CBZ;

	if ((instruction & (u32) 0x7F000000UL) == (u32) 0x37000000UL)
		return ARM64_TBNZ;

	if ((instruction & (u32) 0x7F000000UL) == (u32) 0x36000000UL)
		return ARM64_TBZ;

	if ((instruction & (u32) 0xFF000000UL) == (u32) 0x58000000UL)
		return ARM64_LDR_64;

	if ((instruction & (u32) 0xFF000000UL) == (u32) 0x18000000UL)
		return ARM64_LDR_32;

	if ((instruction & (u32) 0xFF000000UL) == (u32) 0x98000000UL)
		return ARM64_LDRSW;

	if ((instruction & (u32) 0xFF000000UL) == (u32) 0xD8000000UL)
		return ARM64_PRFM;

	if ((instruction & (u32) 0xFFFFFC1FUL) == (u32) 0xD61F0000UL)
		return ARM64_BR;

	return ARM64_UNDEF;
}

/*
 * LDR X16, <label>
 * BLR X16
 * label: DCQ
 */
static void trampoline_setup_thunk_lr(u32 *tramp_insn, u64 *tramp_label, u64 target)
{
	u32 ldr_off = (u32 *)tramp_label - tramp_insn;

	*tramp_insn++ = (u32)0x58000000UL + (ldr_off << 5) + 16;
	*tramp_insn++ = (u32)0xD63F0000UL + (16 << 5);
	*tramp_label++ = target;
}

/*
 * LDR X16, <label>
 * BR X16
 * label: DCQ
 */
static void trampoline_setup_thunk(u32 *from, u64 *label, u64 target)
{
	u32 ldr_off = (u32 *)label - from;

	*from++ = (u32)0x58000000UL + (ldr_off << 5) + 16;
	*from++ = (u32)0xD61F0000UL + (16 << 5);
	*label++ = target;
}

/*
 * oases_relocate_insn() - relocate instruction to trampoline
 *
 * Return: 0 if relocate operation succeed, otherwise -1 and cause inlinehook fail
 */
int oases_relocate_insn(struct oases_insn *info, int off)
{
	int ret = 0;
	u32 insn;
	u32 ldr_off;
	u64 offset;
	u32 rd;
	u32 opc;
	u64 adr_immhi;
	u64 adr_immlo;

	u32 *tramp_insn = info->trampoline + off;
	u32 *tramp_thunk = info->trampoline + TRAMPOLINE_THUNK_OFF;
	u32 *tramp_data = info->trampoline + TRAMPOLINE_DATA_OFF;
	u64 *tramp_label = info->trampoline + TRAMPOLINE_LABEL_OFF;

	insn = *((u32 *)info->address);
	switch (get_insn_type(insn)) {
		case ARM64_ADR:
			/*
			 * ADR <Xd>, <label> (+/-1MB)
			 *
			 * Caculate the Xd value of ADR and store it in label of LDR
			 *
			 * LDR <Xd>, <label>
			 * label: PC[] + imm
			*/
			adr_immhi = insn >> 5 & 0x7FFFFUL; /* 19 bits */
			adr_immlo = insn >> 29 & 0x3UL; /* 2 bits */

			/* imm = SignExtend(immhi:immlo, 64) */
			offset = (adr_immhi << 2) + adr_immlo;
			if (offset & 0x100000UL) {
				offset += 0xFFFFFFFFFFE00000UL;
			}

			rd = insn & 0x1FUL;
			ldr_off = (u32 *)tramp_label - tramp_insn;
			*tramp_insn++ = (u32)0x58000000UL + (ldr_off << 5) + rd;
			*tramp_label++ = (u64)info->address + offset;
			break;
		case ARM64_ADRP:
			/*
			 * ADRP <Xd>, <label> (+/-4GB)
			 *
			 * Caculate the Xd value of ADRP and store it in label of LDR
			 *
			 * LDR <Xd>, <label>
			 * label: PC[](PC[11:0] = ZERO(12)) + imm
			*/
			adr_immhi = insn >> 5 & 0x7FFFFUL; /* 19 bits */
			adr_immlo = insn >> 29 & 0x3UL; /* 2 bits */

			/* imm = SignExtend(immhi:immlow:Zeros(12), 64) */
			offset = ((adr_immhi << 2) + adr_immlo) << 12;
			if (offset & 0x100000000UL) {
				offset += 0xFFFFFFFE00000000UL;
			}

			rd = insn & 0x1FUL;
			ldr_off = (u32 *)tramp_label - tramp_insn;
			*tramp_insn++ = (u32)0x58000000UL + (ldr_off << 5) + rd;
			*tramp_label++ = ((u64)info->address & 0xFFFFFFFFFFFFF000UL) + offset;
			break;
		case ARM64_BCOND:
			/*
			 * B.<cond> <label> (+/-1MB)
			 *
			 * copy and modify the insn to branch to thunk which jump to original target
			 */
			ldr_off = tramp_thunk - tramp_insn;
			/* B.<cond> thunk */
			*tramp_insn++ = (insn & (u32) 0xFF00001FUL) + (ldr_off << 5);

			/* imm = SignExtend(imm19:'00',64) */
			offset = (insn >> 5 & 0x7FFFFUL) << 2;
			if (offset & 0x100000UL) {
				offset += 0xFFFFFFFFFFE00000UL;
			}
			trampoline_setup_thunk(tramp_thunk, tramp_label, offset + (u64)info->address);
			break;
		case ARM64_B:
			/*
			 * B <label> (+/-128MB)
			 *
			 * Calculate the target address of B instruction, and store it in LDR label.
			 *
			 * LDR Xd, <label>; BR/RET Xd;
			 */
			offset = (insn & (u32) 0x3FFFFFFUL) << 2; /* 28 bits */
			if (offset & 0x8000000UL) {
				offset += 0xFFFFFFFFF0000000UL;
			}
			trampoline_setup_thunk(tramp_insn, tramp_label, offset + (u64)info->address);
			break;
		case ARM64_BL:
			/* BL <label> */
			offset = (insn & (u32) 0x3FFFFFFUL) << 2; /* 28 bits */
			if (offset & 0x8000000UL) {
				offset += 0xFFFFFFFFF0000000UL;
			}
			trampoline_setup_thunk_lr(tramp_insn, tramp_label, offset + (u64)info->address);
			break;
		case ARM64_BR:
		case ARM64_BLR:
			/*
			 * B(L)R <Xn>
			 *
			 * for kernel with CFI enabled, we can't hook funcs with BLR as the first insn
			 */
#if OASES_ENABLE_CFI
			ret = -1;
#else
			rd = (insn >> INSN_REGN_BITS) & 0x1FUL;
			if (rd < ARG_REGS_MAX) {
				*tramp_insn++ = insn;
			} else {
				ret = -1;
			}
#endif
			break;
		case ARM64_CBNZ:
		case ARM64_CBZ:
			/*
			 * CB(N)Z Rt, <label> (+/-1MB)
			 *
			 * CB(N)Z THUNK
			 */
			ldr_off = tramp_thunk - tramp_insn;
			*tramp_insn++ = (insn & (u32) 0xFF00001FUL) + (ldr_off << 5);
			offset = ((insn >> 5) & (u32) 0x7FFFFUL) << 2; /* imm19 */
			if (offset & 0x100000UL) {
				offset += 0xFFFFFFFFFFE00000UL;
			}
			trampoline_setup_thunk(tramp_thunk, tramp_label, offset + (u64)info->address);
			break;
		case ARM64_LDR_32:
		case ARM64_LDR_64:
		case ARM64_LDRSW:
			/*
			 * LDR <W|X>t, <label> +/-1MB
			 * LDRSW Xt, <label> +/-1MB
			 */
			offset = ((insn >> 5) & (u32) 0x7FFFFUL) << 2;
			if (offset & 0x100000UL) {
				offset += 0xFFFFFFFFFFE00000UL;
			}

			/*
			 * opc=00, LDR, size=4;
			 * opc=01, LDR, size=8;
			 * opc=10, LDRSW, size=4, signed=true;
			 * opc=11, prefetch, size=8???
			 */
			opc = insn >> 30;
			if (opc == 1 || opc == 3) {
				ldr_off = (u32 *)tramp_label - tramp_insn;
				*tramp_insn++ = (insn & (u32) 0xFF00001FUL) + (ldr_off << 5);
				*tramp_label++ = *((u64 *)(offset + (u64)info->address));
			} else {
				ldr_off = tramp_data - tramp_insn;
				*tramp_insn++ = (insn & (u32) 0xFF00001FUL) + (ldr_off << 5);
				*tramp_data++ = *(u32 *)(offset + (u64)info->address);
			}
			/* another way: LDR Xd, <label>; LDR Xd, <Xd>; label save the target address */
			break;
		case ARM64_TBNZ:
		case ARM64_TBZ:
			/*
			 * TB(N)Z Rt, #<imm>, <label> (+/-32KB)
			 *
			 * TB(N)Z THUNK
			 */
			ldr_off = tramp_thunk - tramp_insn;
			*tramp_insn++ = (insn & (u32) 0xFFF8001FUL) + (ldr_off << 5);
			offset = ((insn >> 5) & (u32) 0x3FFFUL) << 2; /* imm 14 bits */
			if (offset & 0x8000UL) {
				offset += 0xFFFFFFFFFFFF0000UL;
			}
			trampoline_setup_thunk(tramp_thunk, tramp_label, offset + (u64)info->address);
			break;
		case ARM64_PRFM:
			/*
			 * PRFM #<imm5>, <label>
			 *
			 * LDR X16, <label>; PRFM #<imm5>, [X16]
			 */
			offset = (insn >> 5 & 0x7FFFFUL) << 2;
			if (offset & 0x100000UL) {
				offset += 0xFFFFFFFFFFE00000UL;
			}

			ldr_off = (u32 *)tramp_label - tramp_insn;
			*tramp_insn++ = (u32) 0x58000000UL + (ldr_off << 5) + 16;
			*tramp_label++ = offset + (u64)info->address;
			/* PRFM(immediate) */
			*tramp_insn++ = (u32) 0xF9800000UL + (16 << 5) + (insn & 0x1FUL);
			break;
		default:
			*tramp_insn++ = insn;
			break;
	}

	return ret;
}

#else
#error "__arm__ platform not supported"
#endif
