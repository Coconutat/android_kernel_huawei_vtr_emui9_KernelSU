/*
 * hook ops for function
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/types.h>
#include <linux/kobject.h>
#include <linux/slab.h>

#if IS_ENABLED(CONFIG_MODULES)
#include <linux/module.h>
#endif

#include "hook_func.h"
#include "util.h"
#include "hook_insn.h"
#include "patch_api.h"
#include "inlinehook.h"
#include "inlinehook_offset.h"
#include "patch_info.h"
#include "patch_mgr.h"
#include "kallsyms.h"

/* common functions */
static const char *func_get_name(struct oases_patch_entry *patch)
{
	return kp_func(patch)->name;
}

static int func_get_count(struct oases_patch_entry *patch)
{
	return 1;
}

/* VMLINUX for vmlinux */
static const char *get_module_name(struct oases_patch_entry *patch)
{
#if IS_ENABLED(CONFIG_MODULES)
	struct module *m = kp_func(patch)->module;
	if (m)
		return m->name;
#endif
	return kp_func(patch)->mod ?: VMLINUX;
}

static int func_show_info(struct oases_patch_entry *patch,
	struct kobject *kobj, struct kobj_attribute *attr, char *buf, int off)
{
	return scnprintf(buf + off, PAGE_SIZE - off - 1,
		"%s %s\n", get_module_name(patch), func_get_name(patch));
}

static int func_with_each_insn(struct oases_patch_entry *patch,
	int (*callback)(struct oases_patch_entry *patch, struct oases_insn *insn, void *data), void *data)
{
	return callback(patch, &kp_func(patch)->insn, data);
}

static int func_check_user(struct oases_func *kp,
	struct patch_func *up)
{
	if (!valid_patch_pointer(pb_owner(kp), up))
		return -EINVAL;
	if (!valid_patch_pointer(pb_owner(kp), up->name))
		return -EINVAL;
	if (!valid_patch_pointer(pb_owner(kp), up->filter))
		return -EINVAL;
	if (up->mod && !valid_patch_pointer(pb_owner(kp), up->mod))
		return -EINVAL;
	return oases_check_patch_func(up->name);
}

static int func_copy_from_user_l(struct oases_func *kp,
	struct patch_func *up, int flags)
{
	int ret;
	struct oases_find_symbol args;
	struct oases_insn *insn = &kp->insn;

	memset(&args, 0, sizeof(struct oases_find_symbol));
	args.name = up->name;
	args.mod = up->mod;
	args.callback = NULL;
	args.api_use = 0;
	ret = oases_lookup_name_internal(&args);
	if (ret < 0) {
		oases_error("could not find %s, %lu\n", args.name, args.count);
		return -EINVAL;
	}

	kp->name = up->name;
	kp->mod = up->mod;
	kp->module = args.module;
	insn->address = args.addr;
	insn->handler = up->filter;
	return 0;
}

static int func_copy_from_user_alloc(struct oases_func *kp,
	struct patch_func *up, int flags)
{
	struct oases_insn *insn = &kp->insn;

	/* in case patched already by kpatch alike ... */
	if (is_insn_b(*((u32 *)(insn->address)))) {
		oases_error("%pK already hooked\n", insn->address);
		return -EEXIST;
	}

	return oases_insn_alloc(insn, kp->module, flags);
}

static int func_copy_from_user(struct oases_func *kp,
	struct patch_func *up,int flags)
{
	int ret = 0;
	ret = func_copy_from_user_l(kp, up, flags);
	if (ret < 0)
		goto fail;
	ret = func_copy_from_user_alloc(kp, up, flags);
fail:
#if IS_ENABLED(CONFIG_MODULES)
	if (ret < 0 && kp->module) {
		oases_unref_module(kp->module);
		kp->module = NULL;
	}
#endif
	return ret;
}

static int func_create(struct oases_patch_entry *patch,
	void *user, int flags)
{
	int ret;
	struct oases_func *kc = kp_func(patch);
	struct patch_func *uc = user;

	/* check user params */
	ret = func_check_user(kc, uc);
	if (ret)
		return ret;
	/* validate modules and function names */
	ret = func_copy_from_user(kc, uc, flags);
	if (ret)
		return ret;
	return 0;
}

static void func_destroy(struct oases_patch_entry *patch)
{
	struct oases_func *kc = kp_func(patch);
	oases_insn_free(&kc->insn, kc->module);
#if IS_ENABLED(CONFIG_MODULES)
	if (kc->module) {
		oases_unref_module(kc->module);
		kc->module = NULL;
	}
#endif
}

static u32 func_setup_jump(struct oases_patch_entry *patch,
	struct oases_insn *insn)
{
	int ret;
	u32 x;
	ret = oases_make_jump_insn(insn->address, insn->plt, &x);
	return ret ? 0 : x;
}

/* func_pre functions */
static int func_pre_create(struct oases_patch_entry *patch, void *user)
{
	return func_create(patch, user, 0);
}

extern char oases_handler_func_pre_start[];
extern char oases_handler_func_pre_end[];

static int func_pre_setup_trampoline(struct oases_patch_entry *patch,
	struct oases_insn *insn)
{
	void *trampoline = insn->trampoline;

	memcpy(trampoline, &oases_handler_func_pre_start[0],
		&oases_handler_func_pre_end[0] - &oases_handler_func_pre_start[0]);
	/* NOP */
	*((u32 *)(trampoline + FUNC_PRE_PLACE_HOLDER_OFFSET)) = (u32) INSN_NOOP;
	*((void **)(trampoline + FUNC_PRE_FILTER_ADDR_OFFSET)) = (void *)(insn->handler);
	*((void **)(trampoline + FUNC_PRE_ORIGIN_ADDR_OFFSET)) = (void *)(insn->address + 4);
	*((void **)(trampoline + FUNC_PRE_PATCH_INFO_CTX_OFFSET)) = (void *)(kp_owner(patch));
	*((void **)(trampoline + FUNC_PRE_ATTACK_LOGGER_OFFSET)) = (void *)(oases_attack_logger);
	return oases_relocate_insn(insn, FUNC_PRE_PLACE_HOLDER_OFFSET);
}

const struct oases_patch_desc oases_func_pre_ops = {
	.type = OASES_FUNC_PRE,
	.size = sizeof(struct oases_func_pre),
	.usize = sizeof(struct patch_func_pre),
	.get_name = func_get_name,
	.get_count = func_get_count,
	.show_info = func_show_info,
	.with_each_insn = func_with_each_insn,
	.create = func_pre_create,
	.destroy = func_destroy,
	.setup_jump = func_setup_jump,
	.setup_trampoline = func_pre_setup_trampoline,
};

/* func_post functions */
static int func_post_create(struct oases_patch_entry *patch, void *user)
{
	return func_create(patch, user, 0);
}

extern char oases_handler_func_post_start[];
extern char oases_handler_func_post_end[];

static int func_post_setup_trampoline(struct oases_patch_entry *patch,
	struct oases_insn *insn)
{
	void *trampoline = insn->trampoline;

	memcpy(trampoline, &oases_handler_func_post_start[0],
		&oases_handler_func_post_end[0] - &oases_handler_func_post_start[0]);
	/* NOP */
	*((u32 *)(trampoline + FUNC_POST_PLACE_HOLDER_OFFSET)) = (u32) INSN_NOOP;
	*((void **)(trampoline + FUNC_POST_FILTER_ADDR_OFFSET)) = (void *)(insn->handler);
	*((void **)(trampoline + FUNC_POST_ORIGIN_ADDR_OFFSET)) = (void *)(insn->address + 4);
	*((void **)(trampoline + FUNC_POST_PATCH_INFO_CTX_OFFSET)) = (void *)(kp_owner(patch));
	*((void **)(trampoline + FUNC_POST_ATTACK_LOGGER_OFFSET)) = (void *)(oases_attack_logger);
	return oases_relocate_insn(insn, FUNC_POST_PLACE_HOLDER_OFFSET);
}

const struct oases_patch_desc oases_func_post_ops = {
	.type = OASES_FUNC_POST,
	.size = sizeof(struct oases_func_post),
	.usize = sizeof(struct patch_func_post),
	.get_name = func_get_name,
	.get_count = func_get_count,
	.show_info = func_show_info,
	.with_each_insn = func_with_each_insn,
	.create = func_post_create,
	.destroy = func_destroy,
	.setup_jump = func_setup_jump,
	.setup_trampoline = func_post_setup_trampoline,
};

#if OASES_ENABLE_REPLACEMENT_HANDLER
static int func_rep_create(struct oases_patch_entry *patch, void *user)
{
	return func_create(patch, user, OASES_INSN_FLAG_NO_IC);
}

static int func_rep_setup_trampoline(struct oases_patch_entry *patch,
	struct oases_insn *insn)
{
	return 0;
}

const struct oases_patch_desc oases_func_rep_ops = {
	.type = OASES_FUNC_REP,
	.size = sizeof(struct oases_func_rep),
	.usize = sizeof(struct patch_func_rep),
	.get_name = func_get_name,
	.get_count = func_get_count,
	.show_info = func_show_info,
	.with_each_insn = func_with_each_insn,
	.create = func_rep_create,
	.destroy = func_destroy,
	.setup_jump = func_setup_jump,
	.setup_trampoline = func_rep_setup_trampoline,
};
#endif

/* func_pre_post functions */
static int func_pre_post_create(struct oases_patch_entry *patch, void *user)
{
	return func_create(patch, user, 0);
}

extern char oases_handler_func_pre_post_start[];
extern char oases_handler_func_pre_post_end[];

static int func_pre_post_setup_trampoline(struct oases_patch_entry *patch,
	struct oases_insn *insn)
{
	void *trampoline = insn->trampoline;

	memcpy(trampoline, &oases_handler_func_pre_post_start[0],
		&oases_handler_func_pre_post_end[0] - &oases_handler_func_pre_post_start[0]);
	*((u32 *)(trampoline + FUNC_PRE_POST_PLACE_HOLDER_OFFSET)) = (u32) INSN_NOOP;
	*((void **)(trampoline + FUNC_PRE_POST_ORIGIN_ADDR_OFFSET)) = insn->address + 4;
	return oases_relocate_insn(insn, FUNC_PRE_POST_PLACE_HOLDER_OFFSET);
}

const struct oases_patch_desc oases_func_pre_post_ops = {
	.type = OASES_FUNC_PRE_POST,
	.size = sizeof(struct oases_func_pre_post),
	.usize = sizeof(struct patch_func_pre_post),
	.get_name = func_get_name,
	.get_count = func_get_count,
	.show_info = func_show_info,
	.with_each_insn = func_with_each_insn,
	.create = func_pre_post_create,
	.destroy = func_destroy,
	.setup_jump = func_setup_jump,
	.setup_trampoline = func_pre_post_setup_trampoline,
};
