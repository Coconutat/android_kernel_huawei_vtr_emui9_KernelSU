/*
 * allocate trampoline
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/slab.h>
#include <linux/vmalloc.h>
#include "hook_insn.h"
#include "plts.h"
#include "inlinehook.h"

int oases_insn_alloc(struct oases_insn *insn, void *mod, int flags)
{
	int ret = 0;

	plts_lock(mod);
	insn->plt = plts_reserve(mod);
	if (insn->plt == NULL) {
		ret = -ENOMEM;
		goto no_plt;
	}
	if (!(flags & OASES_INSN_FLAG_NO_IC)) {
		insn->trampoline = vmalloc_exec(TRAMPOLINE_BUF_SIZE);
		if (insn->trampoline == NULL) {
			ret = -ENOMEM;
			goto no_vm;
		}
	}
	plts_unlock(mod);
	return 0;
no_vm:
	plts_free(mod, insn->plt);
	insn->plt = NULL;
no_plt:
	plts_unlock(mod);
	return ret;
}

void oases_insn_free(struct oases_insn *insn, void *mod)
{
	if (insn->plt) {
		plts_lock(mod);
		plts_free(mod, insn->plt);
		if (plts_empty(mod))
			plts_purge(mod);
		plts_unlock(mod);
		insn->plt = NULL;
	}
	if (insn->trampoline) {
		vfree(insn->trampoline);
		insn->trampoline = NULL;
	}
}

int oases_insn_alloc_nr(struct oases_insn **insn, void *mod, int nr, int flags)
{
	struct oases_insn *pa, *pi;
	int ret, i, n;

	pa = kzalloc(sizeof(struct oases_insn) * nr, GFP_KERNEL);
	if (!pa)
		return -ENOMEM;
	for (i = 0; i < nr; i++) {
		pi = pa + i;
		ret = oases_insn_alloc(pi, mod, flags);
		if (ret)
			goto fail;
	}
	*insn = pa;
	return 0;
fail:
	for (n = 0; n < i; n++) {
		pi = pa + n;
		oases_insn_free(pi, mod);
	}
	kfree(pa);
	*insn = NULL;
	return ret;
}

int oases_insn_is_busy(struct oases_insn *insn, unsigned long addr)
{
	unsigned long off;

	if (addr == (unsigned long) insn->address) {
		return 1;
	}
	if (insn->plt) {
		off = addr - (unsigned long) insn->plt;
		if (off <= OSAES_PLT_SIZE) {
			return 1;
		}
	}
	if (insn->trampoline) {
		off = addr - (unsigned long) insn->trampoline;
		if (off <= TRAMPOLINE_BUF_SIZE) {
			return 1;
		}
	}
	return 0;
}
