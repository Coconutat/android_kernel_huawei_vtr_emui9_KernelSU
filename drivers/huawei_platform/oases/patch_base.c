/*
 * patch desc
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include "patch_base.h"
#include "patch_api.h"
#include "hook_insn.h"
#include "hook_func.h"
#include "hook_subfunc.h"

const struct oases_patch_desc *oases_patch_desc_by_type(int type)
{
	switch (type) {
	case OASES_FUNC_PRE:
		return &oases_func_pre_ops;
	case OASES_FUNC_POST:
		return &oases_func_post_ops;
#if OASES_ENABLE_REPLACEMENT_HANDLER
	case OASES_FUNC_REP:
		return &oases_func_rep_ops;
#endif
	case OASES_FUNC_PRE_POST:
		return &oases_func_pre_post_ops;
	case OASES_SUBFUNC_PRE:
		return &oases_subfunc_pre_ops;
	case OASES_SUBFUNC_POST:
		return &oases_subfunc_post_ops;
#if OASES_ENABLE_REPLACEMENT_HANDLER
	case OASES_SUBFUNC_REP:
		return &oases_subfunc_rep_ops;
#endif
	case OASES_SUBFUNC_PRE_POST:
		return &oases_subfunc_pre_post_ops;
	}
	return NULL;
}

static int insn_is_busy(struct oases_patch_entry *patch, struct oases_insn *insn, void *data)
{
    return oases_insn_is_busy(insn, (unsigned long) data);
}

int oases_patch_is_busy(struct oases_patch_entry *patch, unsigned long addr)
{
    return kp_vtab(patch)->with_each_insn(patch, insn_is_busy, (void *) addr);
}
