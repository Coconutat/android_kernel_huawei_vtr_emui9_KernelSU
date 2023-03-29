/*
 * patch_api.c - exporting functions for patch
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/slab.h>
#include <linux/uaccess.h>
#include <linux/module.h>
#include <linux/version.h>
#include <linux/vmalloc.h>
#include <asm/cacheflush.h>
#include "patch_api.h"
#include "patch_base.h"
#include "patch_info.h"
#include "util.h"
#include "inlinehook.h"
#include "vmarea.h"
#include "kallsyms.h"

/*
 * Return 0 if success otherwise error code.
 *
 * further check on symbol size, attributes, T/t/W/w/D/d/A...
 * e.g. 000000000000a2a0 A mc_buffer
 */
void * __oases_api oases_lookup_name(const char *mod,
			const char *name, int (*cb)(void *addr))
{
	struct oases_find_symbol args;
	int ret;

	if (!name)
		return NULL;

	memset(&args, 0, sizeof(struct oases_find_symbol));
	args.mod = mod;
	args.name = name;
	args.callback = cb;
	args.api_use = 1;
	ret = oases_lookup_name_internal(&args);
	if (ret < 0) {
		oases_error("could not find %s, %lu\n", args.name, args.count);
		return NULL;
	}

	return args.addr;
}

static int oases_init_patches(struct oases_patch_info *ctx, struct patch_entry *entry)
{
	int ret, i;
	int entry_num = 0, patch_num = 0, address_num = 0;
	struct patch_entry *pe;
	void *user;
	const struct oases_patch_desc *impl;
	struct oases_patch_entry *node;

	for (pe = entry; ; pe++) {
		if (!valid_patch_pointer(ctx, pe))
			return -EINVAL;
		if (!pe->type && !pe->head) /* end {} */
			break;
		if (!valid_patch_type(pe->type) || !valid_patch_pointer(ctx, pe->head))
			return -EINVAL;
		entry_num++;
	}
	oases_debug("total entry count %d\n", entry_num);

	pe = entry;
	for (i = 0; i < entry_num; i++) {
		oases_debug("entry type %d\n", pe[i].type);
		impl = oases_patch_desc_by_type(pe[i].type);
		if (impl == NULL) {
			return -EINVAL;
		}
		for (user = pe[i].head;
			!oases_is_null(user, impl->usize); user += impl->usize) {
			node = kzalloc(sizeof(*node), GFP_KERNEL);
			if (node == NULL) {
				return -ENOMEM;
			}
			node->type = impl->type;
			node->data = kzalloc(impl->size, GFP_KERNEL);
			if (node->data == NULL) {
				kfree(node);
				return -ENOMEM;
			}
			kp_base(node)->vtab = impl;
			kp_base(node)->owner = ctx;
			oases_debug("patch->create()\n");
			ret = impl->create(node, user);
			if (ret) {
				oases_debug("patch->create() failed with %d\n", ret);
				kfree(node->data);
				kfree(node);
				return ret;
			}
			patch_num++;
			address_num += impl->get_count(node);
			list_add_tail(&node->list, &ctx->patches);
		}
	}

	ctx->address_num = address_num;
	oases_debug("total patch count %d\n", patch_num);
	oases_debug("total insn count %d\n", address_num);

	return 0;
}

static int patch_insn_setup(struct oases_patch_entry *patch,
	struct oases_insn *insn, void *data)
{
	int ret;
	u32 jump;
	struct oases_patch_addr *addr = data;

	jump = kp_vtab(patch)->setup_jump(patch, insn);
	if (!jump) {
		oases_debug("oases_make_jump_insn() failed: %pK => %pK\n",
			insn->address, insn->plt);
		return -EINVAL;
	}
	oases_patch_addr_add_key(addr, insn->address, jump);
	switch (patch->type) {
		case OASES_FUNC_PRE:
		case OASES_FUNC_POST:
		case OASES_SUBFUNC_PRE:
		case OASES_SUBFUNC_POST:
			oases_patch_addr_add_plt(addr, insn->plt, insn->trampoline);
			ret = kp_vtab(patch)->setup_trampoline(patch, insn);
			if (ret) {
				oases_debug("setup_trampoline() failed\n");
				return ret;
			}
			break;
#if OASES_ENABLE_REPLACEMENT_HANDLER
		case OASES_FUNC_REP:
		case OASES_SUBFUNC_REP:
			oases_patch_addr_add_plt(addr, insn->plt, insn->handler);
			break;
#endif
		case OASES_FUNC_PRE_POST:
		case OASES_SUBFUNC_PRE_POST:
			oases_patch_addr_add_plt(addr, insn->plt, insn->handler);
			ret = kp_vtab(patch)->setup_trampoline(patch, insn);
			if (ret) {
				oases_debug("setup_trampoline() failed\n");
				return ret;
			}
			break;
		default:
			oases_debug("unknown type %d\n", patch->type);
			return -EINVAL;
	}
	if (insn->trampoline) {
		oases_set_vmarea_ro((unsigned long) insn->trampoline,
			PAGE_ALIGN(TRAMPOLINE_BUF_SIZE) >> PAGE_SHIFT);
#ifdef CONFIG_HISI_HHEE
		if (is_hkip_enabled()) {
			oases_debug("disable pxn for trampoline, trampoline=%pK, size=%d, aligned size=%d\n",
				insn->trampoline, TRAMPOLINE_BUF_SIZE, PAGE_ALIGN(TRAMPOLINE_BUF_SIZE));
			aarch64_insn_disable_pxn_hkip(insn->trampoline, PAGE_ALIGN(TRAMPOLINE_BUF_SIZE));
	    }
#endif
		flush_icache_range((unsigned long) insn->trampoline,
			(unsigned long) insn->trampoline + TRAMPOLINE_BUF_SIZE);
	}
	return 0;
}

static int oases_setup_patches(struct oases_patch_info *ctx)
{
	int ret = 0;
	struct oases_patch_entry *node, *p;
	struct oases_patch_addr *addr = &ctx->addresses;

	ret = oases_patch_addr_init(addr, ctx->address_num);
	if (ret)
		return ret;
	list_for_each_entry_safe(node, p, &ctx->patches, list) {
		oases_debug("patch->with_each_insn()\n");
		ret = kp_vtab(node)->with_each_insn(node, patch_insn_setup,
			&ctx->addresses);
		if (ret) {
			oases_debug("patch->with_each_insn() failed with %d\n", ret);
			goto fail;
		}
	}
	return 0;
fail:
	oases_patch_addr_free(addr);
	return ret;
}

static void oases_free_patches(struct oases_patch_info *ctx)
{
	struct oases_patch_entry *patch, *p;

	list_for_each_entry_safe(patch, p, &ctx->patches, list) {
		kp_vtab(patch)->destroy(patch);
		list_del(&patch->list);
		kfree(patch);
	}
}

/*
 * Register a patch to OASES
 *
 * !!!NOTE!!!
 * All resources will be released in oases_op_patch through oases_patch_info
 * if patch failed, never release any resource in patch itself
 *
 * A patch can only be used to patch kernel or only one module, neither both nor multiple modules.
 *
 * Return: 0 for success otherwise error code
 */
int __oases_api oases_register_patch(struct oases_patch_info *ctx, struct oases_patch *patch)
{
	int ret;

	if (!patch) {
		oases_error("invalid patch\n");
		return -EINVAL;
	}

	if (!valid_patch_pointer(ctx, patch)) {
		oases_error("invalid patch pointer\n");
		return -EINVAL;
	}

	if (!patch->name || !patch->pe) {
		oases_error("invalid patch->name/patch->pe\n");
		return -EINVAL;
	}

	strncpy(ctx->vulnname, patch->name, PATCH_NAME_LEN - 1);

	ret = oases_init_patches(ctx, patch->pe);
	if (ret < 0) {
		oases_error("oases_init_patch failed\n");
		goto fail;
	}

	ret = oases_setup_patches(ctx);
	if (ret < 0) {
		oases_error("oases_setup_patch failed\n");
		goto fail;
	}

	return 0;
fail:
	oases_free_patches(ctx);
	return ret;
}

int __oases_api oases_printk(const char *fmt, ...)
{
	va_list args;
	int r;

	va_start(args, fmt);
#if LINUX_VERSION_CODE >= KERNEL_VERSION(3, 10, 0)
	r = vprintk_emit(0, -1, NULL, 0, fmt, args);
#else
	r = vprintk(fmt, args);
#endif
	va_end(args);
	return r;
}

void * __oases_api oases_memset(void *s, int c, size_t count)
{
	char *xs = s;

	while (count--)
		*xs++ = c;
	return s;
}

void * __oases_api oases_memcpy(void *dest, const void *src, size_t count)
{
	char *tmp = dest;
	const char *s = src;

	while (count--)
		*tmp++ = *s++;
	return dest;
}

int __oases_api oases_memcmp(const void *cs, const void *ct, size_t count)
{
	const unsigned char *su1, *su2;
	int res = 0;

	for (su1 = cs, su2 = ct; 0 < count; ++su1, ++su2, count--)
		if ((res = *su1 - *su2) != 0)
			break;
	return res;
}

int __oases_api oases_copy_from_user(void *to, const void __user *from,
				 unsigned long n)
{
	return copy_from_user(to, from, n);
}

int __oases_api oases_copy_to_user(void __user *to, const void *from,
				unsigned long n)
{
	return copy_to_user(to, from, n);
}

int __oases_api oases_get_user_u8(u8 *value, u8 *ptr)
{
	return get_user(*value, ptr);
}

int __oases_api oases_put_user_u8(u8 value, u8 *ptr)
{
	return put_user(value, ptr);
}

int __oases_api oases_get_user_u16(u16 *value, u16 *ptr)
{
	return get_user(*value, ptr);
}

int __oases_api oases_put_user_u16(u16 value, u16 *ptr)
{
	return put_user(value, ptr);
}

int __oases_api oases_get_user_u32(u32 *value, u32 *ptr)
{
	return get_user(*value, ptr);
}

int __oases_api oases_put_user_u32(u32 value, u32 *ptr)
{
	return put_user(value, ptr);
}

int __oases_api oases_get_user_u64(u64 *value, u64 *ptr)
{
	return copy_from_user(value, ptr, sizeof(*value));
}

int __oases_api oases_put_user_u64(u64 value, u64 *ptr)
{
	return copy_to_user(ptr, &value, sizeof(value));
}

/* For CONFIG_CC_STACKPROTECTOR which makes current(task_struct) randomized(kti_offset)  */
void * __oases_api oases_get_current(void)
{
	return current;
}

unsigned long __oases_api oases_linux_version_code(void)
{
	return LINUX_VERSION_CODE;
}

unsigned long __oases_api oases_version(void)
{
	return OASES_VERSION;
}

struct query_patch_info_args {
	struct patch_info *out;
	int ret;
	int index;
	int nth;
};

static int query_patch_info(struct oases_patch_entry *patch,
	struct oases_insn *insn, void *data)
{
	struct query_patch_info_args *args = data;
	struct patch_info *out = args->out;

	if (args->nth == args->index) {
		args->ret = 0;
		out->address = insn->address;
		out->subfunc = insn->origin_to;
		out->plt = insn->plt;
		out->trampoline = insn->trampoline;
		out->handler = insn->handler;
		return 1;
	}
	args->index++;
	return 0;
}

int __oases_api oases_query_patch_info( struct oases_patch_info *ctx,
	const char *name, int nth, struct patch_info *info)
{
	struct oases_patch_entry *node, *p;
	struct query_patch_info_args args;

	if (!info || !ctx || !name || nth < 0)
		return -EINVAL;
	list_for_each_entry_safe(node, p, &ctx->patches, list) {
		/* for subfunc we use parent name */
		if (!strcmp(kp_vtab(node)->get_name(node), name)) {
			args.out = info;
			args.ret = -EINVAL;
			args.index = 0;
			args.nth = nth;
			kp_vtab(node)->with_each_insn(node, query_patch_info, &args);
			return args.ret;
		}
	}
	return -ENOENT;
}

int __oases_api oases_setup_callbacks(struct oases_patch_info *ctx,
		struct patch_callbacks *ucbs)
{
	struct patch_callbacks *kcbs = &ctx->cbs;

	if (ucbs->init && !valid_patch_pointer(ctx, ucbs->init))
		return -EINVAL;

	if (ucbs->exit && !valid_patch_pointer(ctx, ucbs->exit))
		return -EINVAL;

	if (ucbs->enable && !valid_patch_pointer(ctx, ucbs->enable))
		return -EINVAL;

	if (ucbs->disable && !valid_patch_pointer(ctx, ucbs->disable))
		return -EINVAL;

	kcbs->init= ucbs->init;
	kcbs->exit= ucbs->exit;
	kcbs->enable = ucbs->enable;
	kcbs->disable = ucbs->disable;

	return 0;
}
