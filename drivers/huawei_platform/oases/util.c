/*
 * util.c - util functions for OASES framework
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */
#include <linux/device.h>
#include <linux/stop_machine.h>
#include <linux/sched.h>
#include <linux/slab.h>
#ifdef CONFIG_MODULES
#include <linux/module.h>
#endif
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <asm/uaccess.h>
#include <asm/cacheflush.h>
#include <asm/stacktrace.h>
#include <asm/irq.h>

#ifdef __arm__
#include <asm/mach/map.h>
#include <asm/memory.h>
#include <linux/io.h>
#include <linux/vmalloc.h>
/* for struct mem_type */
#include <asm/tlb.h>
#include "../../../arch/arm/mm/mm.h"
#else
#include <asm/insn.h>
#endif

#include "util.h"
#include "patch_info.h"
#include "patch_base.h"
#include "plts.h"

#if PAGE_SIZE != 4096
#error "Only support PAGE_SIZE=4096"
#endif

/* MT_MEMORY changed to MT_MEMORY_RWX */
#ifndef MT_MEMORY
#define MT_MEMORY MT_MEMORY_RWX
#endif

int oases_is_null(const void *data, int size)
{
	int i;
	const long *pl;
	const char *pc;

	for (i = 0; i < size; i += sizeof(*pl)) {
		pl = (data + i);
		if (*pl)
			return 0;
	}
	if (i < size) {
		while (i < size) {
			pc = data + i;
			if (*pc)
				return 0;
			i++;
		}
	}
	return 1;
}

/* a-zA-Z0-9-_ */
int oases_valid_name(const char *id, int max_len)
{
	int i;
	int len;
	char c;

	len = strlen(id);
	/* max_len = PATCH_ID_LEN - 1 */
	if (len > max_len)
		return -EINVAL;

	for(i = 0; i < len; i++) {
		c = id[i];
		if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z')
			|| (c >= '0' && c <= '9') || c == '-' || c == '_'))
			return -EINVAL;
	}

	return 0;
}

void oases_module_lock(void)
{
#ifdef CONFIG_MODULES
	mutex_lock(&module_mutex);
#endif
}

void oases_module_unlock(void)
{
#ifdef CONFIG_MODULES
	mutex_unlock(&module_mutex);
#endif
}

void *oases_ref_module(const char *name)
{
#if IS_ENABLED(CONFIG_MODULES)
	struct module *mod;

	mod = find_module(name);
	if (mod == NULL) {
		return NULL;
	}
	if (!try_module_get(mod)) {
		return NULL;
	}
	return mod;
#else
	return NULL;
#endif
}

int oases_ref_module_ptr(void *module)
{
#if IS_ENABLED(CONFIG_MODULES)
	return try_module_get(module);
#else
	return 1;
#endif
}

void oases_unref_module(void *module)
{
#if IS_ENABLED(CONFIG_MODULES)
	if (module)
		module_put(module);
#endif
}

/* check for old kernel */
#ifndef IRQ_STACK_PTR
#define OASES_QUIRK_INCOMPLETE_STACK
#define OASES_STACK_LOOKBACK_COUNT 64
#endif

struct oases_stack_verify_info {
	void *args;
	int ret;
};

static int patch_address_check(unsigned long addr, struct oases_patch_info *ctx)
{
	struct oases_patch_entry *patch, *p;
	unsigned long off;

	off = addr - (unsigned long)ctx->code_base;
	if (off <= ctx->code_size) {
		oases_debug("pc:%pK in patch\n", (void *)addr);
		return 1;
	}
	list_for_each_entry_safe(patch, p, &ctx->patches, list) {
		if (oases_patch_is_busy(patch, addr)) {
			oases_debug("pc:%pK in patch\n", (void *)addr);
			return 1;
		}
	}
	return 0;
}

static int remove_stack_verify(struct stackframe *frame, void *verify)
{
	struct oases_stack_verify_info *info = verify;
	struct oases_patch_info *patch = info->args;

	if (info->ret)
		return 1;

	if (patch_address_check(frame->pc, patch)) {
		info->ret = 1;
		return 1;
	}

	return 0;
}

/*
 * This function's prototype of aarch64 version changed from [4.5,
 * and msm/hisi backported it from [4.4, but not mtk. as for arm,
 * nothing changed.
 */
static void oases_walk_stackframe(struct task_struct *t,
	struct stackframe *frame, int (*remove_stack_verify)(struct stackframe *frame, void *verify),
	struct oases_stack_verify_info *info)
{
#ifdef __arm__
	walk_stackframe(frame, remove_stack_verify, info);
#else
#if ((LINUX_VERSION_CODE < KERNEL_VERSION(4, 4, 0)) \
		|| ((LINUX_VERSION_CODE < KERNEL_VERSION(4, 5, 0)) \
				&& IS_ENABLED(CONFIG_MEDIATEK_SOLUTION)))
	walk_stackframe(frame, remove_stack_verify, info);
#else
	walk_stackframe(t, frame, remove_stack_verify, info);
#endif
#endif
}

static int oases_remove_safe(void *verify)
{
	struct task_struct *g, *t;
	struct stackframe frame;
	struct oases_stack_verify_info *info = verify;
#ifdef OASES_QUIRK_INCOMPLETE_STACK
	struct oases_patch_info *patch = info->args;
	unsigned long sp_lo, sp_hi, sp_ul, *sp, pc;
	int i;
#endif

	do_each_thread(g, t) {
#ifdef OASES_QUIRK_INCOMPLETE_STACK
		/*
		 * for old kernel we have to check el1_lr
		 * which is some where near thread sp
		 */
		sp_lo = (unsigned long)task_stack_page(t);
		sp_hi = sp_lo + THREAD_SIZE;
		sp = (unsigned long *)thread_saved_sp(t);
		if (sp) {
			for (i = 0; i < OASES_STACK_LOOKBACK_COUNT; i++) {
				sp_ul = (unsigned long) &sp[i];
				if (sp_ul < sp_lo || sp_ul >= sp_hi)
					break;
				pc = sp[i];
				if (patch_address_check(pc, patch)) {
					oases_debug("check stack fail\n");
					info->ret = 1;
					return -EBUSY;
				}
			}
		}
#endif
		frame.fp = thread_saved_fp(t);
		frame.sp = thread_saved_sp(t);
		frame.pc = thread_saved_pc(t);
		oases_walk_stackframe(t, &frame, remove_stack_verify, info);
		if (info->ret) {
			oases_debug("find patch address in stack\n");
			return -EBUSY;
		}
	} while_each_thread(g, t);

	oases_debug("oases_remove_safe\n");
	return 0;
}

int oases_remove_patch(struct oases_patch_info *info)
{
	int ret;
	struct oases_stack_verify_info verify = {
		.args = info,
		.ret = 0
	};

	ret = stop_machine(oases_remove_safe, &verify, NULL);
	if (ret)
		oases_error("stop_machine fail: %d\n", ret);

	return ret;
}

/*
 * bypass kernel memory write protection in this function.
 */
#ifdef CONFIG_HISI_HHEE
static inline int oases_insn_patch_nosync(void* addr, u32 insn)
#else
static int oases_insn_patch_nosync(void* addr, u32 insn)
#endif
{
#if defined(__aarch64__)
	oases_debug("addr:0x%pK, insn:%x\n", addr, insn);
#ifdef CONFIG_HISI_HHEE
	if (is_hkip_enabled()) {
		oases_debug("calling hkip patch text\n");
		aarch64_insn_patch_text_hkip(addr, insn);
		return 0;
	}
#endif
	return aarch64_insn_patch_text_nosync(addr, insn);
#else
	int err;
	unsigned long virt = ((unsigned long)addr) & PAGE_MASK;
	unsigned long offset = ((unsigned long)addr) & (PAGE_SIZE - 1);
	unsigned long phys;
	const struct mem_type *mt;
	struct vm_struct *vm;

	oases_debug("addr:0x%pK, insn:%x\n", addr, insn);
	/* align */
	if (((unsigned long)addr) & ((1UL < 2) - 1))
		return -EINVAL;
	/* cross page */
	if (offset > (PAGE_SIZE - sizeof(int)))
		return -EINVAL;
	mt = get_mem_type(MT_MEMORY);
	if (mt == NULL)
		return -EIO;
	vm = get_vm_area(PAGE_SIZE, VM_IOREMAP);
	if (vm == NULL)
		return -ENOMEM;
	if (core_kernel_text((unsigned long)addr)) {
		phys = __virt_to_phys((unsigned long)virt);
	} else {
		phys = __pfn_to_phys(vmalloc_to_pfn((void *)virt));
	}
	oases_debug("virt %pK => phys %pK\n", (void *)virt, (void *)phys);
	virt = (unsigned long) vm->addr;
	err = ioremap_page_range(virt, virt + PAGE_SIZE, phys, __pgprot(mt->prot_pte));
	oases_debug("phys %pK => virt %pK, %d\n", (void *)phys, (void *)virt, err);
	if (err) {
		vunmap((void *)virt);
		return err;
	}
	flush_cache_vmap(virt, virt + PAGE_SIZE);
	*((volatile u32 *)(virt + offset)) = insn;
	flush_icache_range(virt + offset, virt + offset + sizeof(insn));
	vunmap((void *)virt);
	flush_icache_range((unsigned long)addr, (unsigned long)addr + sizeof(insn));
	return 0;
#endif
}

static int oases_unpatch_safe(void *info)
{
	struct oases_patch_info *patch = info;
	struct oases_patch_addr *addr = &patch->addresses;
	void (*disable)(void) = NULL;
	int ret = 0, i;

	for (i = 0; i < addr->i_key && !ret; i++) {
		ret = oases_insn_patch_nosync(addr->addrs_key[i], addr->old_insns_key[i]);
	}
	if (ret != 0)
		oases_error("oases_insn_patch_nosync index:%d fail ret: %d\n", --i, ret);

	disable = patch->cbs.disable;
	if (disable)
		disable();

	return ret;
}

int oases_insn_unpatch(struct oases_patch_info *info)
{
	int ret;

	ret = stop_machine(oases_unpatch_safe, info, NULL);
	if (ret)
		oases_error("stop_machine fail: %d\n", ret);

	return ret;
}

static int oases_poke_plts(void *addrs[], u32 insns[], int count)
{
	int i, ret;

	for (i = 0; i < count; i++) {
		oases_debug("addr:0x%pK, insn: %x\n", addrs[i], insns[i]);
		ret = oases_insn_patch_nosync(addrs[i], insns[i]);
		if (ret) {
			return ret;
		}
		flush_icache_range((unsigned long)addrs[i],
				(unsigned long)addrs[i] + sizeof(insns[i]));
	}
	return 0;
}

static int oases_patch_safe(void *info)
{
	int ret = 0, i;
	struct oases_patch_info *patch = info;
	struct oases_patch_addr *addr = &patch->addresses;
	void (*enable)(void) = NULL;

	ret = oases_poke_plts(addr->addrs_plt, addr->new_insns_plt, addr->i_plt);
	if (ret) {
		oases_debug("oases_poke_plts fail\n");
		return ret;
	}
	for (i = 0; i < addr->i_key && !ret; i++) {
		ret = oases_insn_patch_nosync(addr->addrs_key[i],
					addr->new_insns_key[i]);
	}

	enable = patch->cbs.enable;
	if (enable)
		enable();

	return ret;
}

int oases_insn_patch(struct oases_patch_info *info)
{
	int ret;

	ret = stop_machine(oases_patch_safe, info, NULL);
	if (ret != 0)
		oases_error("write insn text fail\n");
	return ret;
}
