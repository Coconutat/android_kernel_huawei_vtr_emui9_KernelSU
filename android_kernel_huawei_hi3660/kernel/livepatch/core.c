/*
 * core.c - Kernel Live Patching Core
 *
 * Copyright (C) 2014 Seth Jennings <sjenning@redhat.com>
 * Copyright (C) 2014 SUSE
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

#define pr_fmt(fmt) KBUILD_MODNAME ": " fmt

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/list.h>
#include <linux/kallsyms.h>
#include <linux/livepatch.h>
#include <asm/stacktrace.h>
#include <linux/stop_machine.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <asm/memory.h>
#include <huawei_platform/log/imonitor.h>
#include <asm/livepatch-hhee.h>

unsigned long hkip_token;

struct patch_data {
	struct klp_patch	*patch;
	atomic_t		cpu_count;
};

/*
 * The klp_mutex protects the global lists and state transitions of any
 * structure reachable from them.  References to any structure must be obtained
 * under mutex protection (except in klp_ftrace_handler(), which uses RCU to
 * ensure it gets consistent data).
 */
extern struct mutex klp_mutex;

static LIST_HEAD(klp_patches);

static void klp_free_patch(struct klp_patch *patch);
static struct kobject *klp_root_kobj;

static bool klp_is_module(struct klp_object *obj)
{
	return obj->name;
}

static bool klp_is_object_loaded(struct klp_object *obj)
{
	return !obj->name || obj->mod;
}

/* sets obj->mod if object is not vmlinux and module is found */
static int klp_find_object_module(struct klp_object *obj)
{
	struct module *mod;

	if (!klp_is_module(obj))
		return 0;

	mutex_lock(&module_mutex);
	/*
	 * We do not want to block removal of patched modules and therefore
	 * we do not take a reference here. The patches are removed by
	 * a going module handler instead.
	 */
	mod = find_module(obj->name);

	if (mod) {
		obj->mod = mod;
		try_module_get(mod);
	} else {
		pr_err("module '%s' not loaded\n", obj->name);
		mutex_unlock(&module_mutex);
		return -EINVAL;
	}

	mutex_unlock(&module_mutex);
	return 0;
}

/* klp_mutex must be held by caller */
static bool klp_is_patch_registered(struct klp_patch *patch)
{
	struct klp_patch *mypatch;

	list_for_each_entry(mypatch, &klp_patches, list)
		if (mypatch == patch)
			return true;

	return false;
}

static bool klp_initialized(void)
{
	return !!klp_root_kobj;
}

struct klp_find_arg {
	const char *objname;
	const char *name;
	unsigned long addr;
	/*
	 * If count == 0, the symbol was not found. If count == 1, a unique
	 * match was found and addr is set.  If count > 1, there is
	 * unresolvable ambiguity among "count" number of symbols with the same
	 * name in the same object.
	 */
	unsigned long count;
};

static int klp_find_callback(void *data, const char *name,
			     struct module *mod, unsigned long addr)
{
	struct klp_find_arg *args = data;

	if ((mod && !args->objname) || (!mod && args->objname))
		return 0;

	if (strcmp(args->name, name))
		return 0;

	if (args->objname && strcmp(args->objname, mod->name))
		return 0;

	/*
	 * args->addr might be overwritten if another match is found
	 * but klp_find_object_symbol() handles this and only returns the
	 * addr if count == 1.
	 */
	args->addr = addr;
	args->count++;

	return 0;
}

static int klp_find_object_symbol(const char *objname, const char *name,
				  unsigned long *addr)
{
	struct klp_find_arg args = {
		.objname = objname,
		.name = name,
		.addr = 0,
		.count = 0
	};

	mutex_lock(&module_mutex);
	kallsyms_on_each_symbol(klp_find_callback, &args);
	mutex_unlock(&module_mutex);

	if (args.count == 1) {
		*addr = args.addr;
		return 0;
	}

	if (args.count == 0)
		pr_err("symbol '%s' not found in symbol table\n", name);

	*addr = 0;
	return -EINVAL;
}

struct klp_verify_args {
	const char *name;
	const unsigned long addr;
};

static int klp_verify_callback(void *data, const char *name,
			       struct module *mod, unsigned long addr)
{
	struct klp_verify_args *args = data;

	if (!mod &&
	    !strcmp(args->name, name) &&
	    args->addr == addr)
		return 1;

	return 0;
}

static int klp_verify_vmlinux_symbol(const char *name, unsigned long addr)
{
	struct klp_verify_args args = {
		.name = name,
		.addr = addr,
	};
	int ret;

	mutex_lock(&module_mutex);
	ret = kallsyms_on_each_symbol(klp_verify_callback, &args);
	mutex_unlock(&module_mutex);

	if (!ret) {
		pr_err("symbol '%s' not found at specified address 0x%016lx, kernel mismatch?\n",
			name, addr);
		return -EINVAL;
	}

	return 0;
}

static int klp_find_verify_func_addr(struct klp_object *obj,
				     struct klp_func *func)
{
	int ret;

#if defined(CONFIG_RANDOMIZE_BASE)
	/* If KASLR has been enabled, adjust old_addr accordingly */
	if (func->old_addr)
		func->old_addr += (kimage_vaddr - KIMAGE_VADDR);
#endif

	if (!func->old_addr || klp_is_module(obj)) {
		ret = klp_find_object_symbol(obj->name, func->old_name,
					     &func->old_addr);
		if (ret && func->ref_name) {
			unsigned long ref_addr;
			ret = klp_find_object_symbol(obj->name, func->ref_name,
					&ref_addr);
			if (!ret)
				func->old_addr = (unsigned long)((long)ref_addr +
						func->ref_offset);
		}
	}
	else
		ret = klp_verify_vmlinux_symbol(func->old_name,
						func->old_addr);

	return ret;
}

/*
 * external symbols are located outside the parent object (where the parent
 * object is either vmlinux or the kmod being patched).
 */
static int klp_find_external_symbol(struct module *pmod, const char *name,
				    unsigned long *addr)
{
	const struct kernel_symbol *sym;

	/* first, check if it's an exported symbol */
	preempt_disable();
	sym = find_symbol(name, NULL, NULL, true, true);
	if (sym) {
		*addr = sym->value;
		preempt_enable();
		return 0;
	}
	preempt_enable();

	/* otherwise check if it's in another .o within the patch module */
	return klp_find_object_symbol(pmod->name, name, addr);
}

static int klp_write_object_relocations(struct module *pmod,
					struct klp_object *obj)
{
	int ret;
	struct klp_reloc *reloc;

	if (WARN_ON(!klp_is_object_loaded(obj)))
		return -EINVAL;

	if (WARN_ON(!obj->relocs))
		return -EINVAL;

	for (reloc = obj->relocs; reloc->name; reloc++) {
		if (!klp_is_module(obj)) {

#if defined(CONFIG_RANDOMIZE_BASE)
			/* If KASLR has been enabled, adjust old value accordingly */
			reloc->val += (kimage_vaddr - KIMAGE_VADDR);
#endif
			ret = klp_verify_vmlinux_symbol(reloc->name,
							reloc->val);
			if (ret)
				return ret;
		} else {
			/* module, reloc->val needs to be discovered */
			if (reloc->external)
				ret = klp_find_external_symbol(pmod,
						reloc->name,
						&reloc->val);
			else {
				ret = klp_find_object_symbol(obj->mod->name,
						reloc->name,
						&reloc->val);
				if (ret && reloc->ref_name) {
					unsigned long ref_addr;
					ret = klp_find_object_symbol(obj->mod->name,
							reloc->ref_name,
							&ref_addr);
					if (!ret)
						reloc->val = (unsigned long)((long)ref_addr +
								reloc->ref_offset);
				}
			}

			if (ret)
				return ret;
		}
		ret = klp_write_module_reloc(pmod, reloc->type, reloc->loc,
					     reloc->val + reloc->addend);
		if (ret) {
			pr_err("relocation failed for symbol '%s' at 0x%016lx (%d)\n",
			       reloc->name, reloc->val, ret);
			return ret;
		}
	}

	return 0;
}

int __weak arch_klp_disable_func(struct klp_func *func)
{
	return 0;
}

static int klp_disable_func(struct klp_func *func)
{
	int ret;
	WARN_ON(func->state != KLP_ENABLED);
	WARN_ON(!func->old_addr);

	ret = arch_klp_disable_func(func);
	if (!ret) {
        	func->state = KLP_DISABLED;
	}
	return ret;
}
#define KLP_PATCH_STATE_IMONITOR_ID     (940000002)
#define UPLOADTYPE_INIT_FAIL    (1)
#define UPLOADTYPE_ENABLE_FAIL  (2)
#define UPLOADTYPE_DISBALE_FAIL (3)

static int do_upload_log(long type,const char * desc)
{
	struct imonitor_eventobj *obj=NULL;
	int ret = 0;

	obj = imonitor_create_eventobj(KLP_PATCH_STATE_IMONITOR_ID);
	if (obj) {
		ret += imonitor_set_param(obj, E940000002_ERRTYPE_INT,type);
		ret += imonitor_set_param(obj, E940000002_ERRDESC_VARCHAR,(long)desc);
		if(ret){
			imonitor_destroy_eventobj(obj);
			return ret;
		}
		ret = imonitor_send_event(obj);
		imonitor_destroy_eventobj(obj);
	}
	else {
		ret = -1;
	}
	return ret;
}
int __weak arch_klp_enable_func(struct klp_func *func)
{
	return 0;
}

static int klp_enable_func(struct klp_func *func)
{
	int ret;

	if (WARN_ON(!func->old_addr))
		return -EINVAL;

	if (WARN_ON(func->state != KLP_DISABLED))
		return -EINVAL;

	ret = arch_klp_enable_func(func);
	if (!ret) {
		func->state = KLP_ENABLED;
	}

	return ret;
}

static inline int klp_unload_hook(struct klp_object *obj)
{
	struct klp_hook *hook;

	for (hook = obj->hooks_unload; hook->hook; hook++)
		(*hook->hook)();

	return 0;
}

static int klp_disable_object(struct klp_object *obj)
{
	struct klp_func *func;
	int ret = 0;
	for (func = obj->funcs; func->old_name; func++) {
		if (func->state == KLP_ENABLED) {
			ret += klp_disable_func(func);
		}
	}
	if (!ret) {
		obj->state = KLP_DISABLED;
	}
	return ret;
}

static inline int klp_load_hook(struct klp_object *obj)
{
	struct klp_hook *hook;

	for (hook = obj->hooks_load; hook->hook; hook++)
		(*hook->hook)();

	return 0;
}

static int klp_enable_object(struct klp_object *obj)
{
	struct klp_func *func;
	int ret;

	if (WARN_ON(obj->state != KLP_DISABLED))
		return -EINVAL;

	if (WARN_ON(!klp_is_object_loaded(obj)))
		return -EINVAL;

	for (func = obj->funcs; func->old_name; func++) {
		ret = klp_enable_func(func);
		if (ret) {
			klp_disable_object(obj);
			return ret;
		}
	}
	obj->state = KLP_ENABLED;

	return 0;
}

void __weak arch_klp_code_modify_prepare(void)
{

}

void __weak arch_klp_code_modify_post_process(void)
{

}

/*
 * This function is called from stop_machine() context.
 */
int disable_patch(struct klp_patch *patch)
{
	int ret = 0;
	struct klp_object *obj;

	pr_notice("disabling patch '%s'\n", patch->mod->name);

	for (obj = patch->objs; obj->funcs; obj++) {
		if (obj->state == KLP_ENABLED) {
			ret += klp_disable_object(obj);
		}
	}
	if (!ret) {
		patch->state = KLP_DISABLED;
	}
	module_put(patch->mod);
	return ret;
}

int klp_try_disable_patch(void *data)
{
	struct klp_patch *patch = data;
	int ret = 0;

	ret = klp_check_calltrace(patch, 0);
	if (ret) {
		return ret;
	}
	ret = disable_patch(patch);
	return ret;
}

static int __klp_disable_patch(struct klp_patch *patch)
{
	int ret;

#ifdef CONFIG_LIVEPATCH_STACK
	/* enforce stacking: only the last enabled patch can be disabled */
	if (!list_is_last(&patch->list, &klp_patches) &&
	    list_next_entry(patch, list)->state == KLP_ENABLED)
		return -EBUSY;
#endif

	arch_klp_code_modify_prepare();
	ret = stop_machine(klp_try_disable_patch, patch, NULL);
	arch_klp_code_modify_post_process();

	return ret;
}

/*
 * This function is called from stop_machine() context.
 */
int enable_patch(struct klp_patch *patch)
{
	struct klp_object *obj;
	int ret;

	pr_notice_once("tainting kernel with TAINT_LIVEPATCH\n");
	add_taint(TAINT_LIVEPATCH, LOCKDEP_STILL_OK);

	pr_notice("enabling patch '%s'\n", patch->mod->name);

	for (obj = patch->objs; obj->funcs; obj++) {
		ret = klp_enable_object(obj);
		if (ret)
			goto disable;
	}

	patch->state = KLP_ENABLED;
	try_module_get(patch->mod);
	return 0;

disable:
	disable_patch(patch);
	pr_err("livepatch: enable_patch failed,ret = %d.\n",ret);
	return ret;
}

int klp_try_enable_patch(void *data)
{
	int ret = 0;
	int flag = 0;
	struct patch_data *pd = data;

	if (atomic_inc_return(&pd->cpu_count) == 1) {
		struct klp_patch *patch = pd->patch;

		ret = klp_check_calltrace(patch, 1);
		if (ret) {
			flag = 1;
			atomic_inc(&pd->cpu_count);
			return ret;
		}
		ret = enable_patch(patch);
		if (ret) {
			flag = 1;
			atomic_inc(&pd->cpu_count);
			return ret;
		}
		atomic_inc(&pd->cpu_count);
	} else {
		while (atomic_read(&pd->cpu_count) <= num_online_cpus())
			cpu_relax();

		if (!flag)
			isb();
	}

	return ret;
}

static int __klp_enable_patch(struct klp_patch *patch)
{
	int ret;
	struct patch_data patch_data = {
		.patch = patch,
		.cpu_count = ATOMIC_INIT(0),
	};

	if (WARN_ON(patch->state != KLP_DISABLED))
		return -EINVAL;

#ifdef CONFIG_LIVEPATCH_STACK
	/* enforce stacking: only the first disabled patch can be enabled */
	if (patch->list.prev != &klp_patches &&
			(list_prev_entry(patch, list)->state == KLP_DISABLED)) {
		pr_err("only the first disabled patch can be enabled\n");
		return -EBUSY;
	}
#endif

	arch_klp_code_modify_prepare();
	ret = stop_machine(klp_try_enable_patch, &patch_data, cpu_online_mask);
	arch_klp_code_modify_post_process();
	if (ret)
		return ret;

#ifndef CONFIG_LIVEPATCH_STACK
	/* move the enabled patch to the list tail */
	list_del(&patch->list);
	list_add_tail(&patch->list, &klp_patches);
#endif

	return 0;
}

/*
 * Sysfs Interface
 *
 * /sys/kernel/livepatch
 * /sys/kernel/livepatch/<patch>
 * /sys/kernel/livepatch/<patch>/enabled
 * /sys/kernel/livepatch/<patch>/<object>
 * /sys/kernel/livepatch/<patch>/<object>/<func> or
 * /sys/kernel/livepatch/<patch>/<object>/<func-number>
 */

/*
 * Procfs Interface
 *
 * /proc/livepatch/state
 */

static ssize_t enabled_store(struct kobject *kobj, struct kobj_attribute *attr,
			     const char *buf, size_t count)
{
	struct klp_patch *patch;
	int ret;
	unsigned long val;

	ret = kstrtoul(buf, 10, &val);
	if (ret)
		return -EINVAL;

	if (val != KLP_DISABLED && val != KLP_ENABLED)
		return -EINVAL;

	patch = container_of(kobj, struct klp_patch, kobj);

	mutex_lock(&klp_mutex);

	if (val == patch->state) {
		/* already in requested state */
		ret = -EINVAL;
		goto err;
	}

	if (val == KLP_ENABLED) {
		ret = __klp_enable_patch(patch);
		if (ret)
			goto err;
	} else {
		ret = __klp_disable_patch(patch);
		if (ret)
			goto err;
	}

	mutex_unlock(&klp_mutex);

	return count;

err:
	mutex_unlock(&klp_mutex);
	if(val == KLP_ENABLED) {
		(void)do_upload_log(UPLOADTYPE_ENABLE_FAIL,"enable patch failed.");
	}
	else{
		(void)do_upload_log(UPLOADTYPE_DISBALE_FAIL,"disable patch failed.");
	}
	return ret;
}

static ssize_t enabled_show(struct kobject *kobj,
			    struct kobj_attribute *attr, char *buf)
{
	struct klp_patch *patch;

	patch = container_of(kobj, struct klp_patch, kobj);
	return snprintf(buf, PAGE_SIZE-1, "%d\n", patch->state);
}

static struct kobj_attribute enabled_kobj_attr =
	__ATTR(enabled, S_IWUSR | S_IWGRP | S_IRUGO, enabled_show, enabled_store);

static struct attribute *klp_patch_attrs[] = {
	&enabled_kobj_attr.attr,
	NULL
};

static void klp_kobj_release_patch(struct kobject *kobj)
{
	/*
	 * Once we have a consistency model we'll need to module_put() the
	 * patch module here.  See klp_register_patch() for more details.
	 */
}

static struct kobj_type klp_ktype_patch = {
	.release = klp_kobj_release_patch,
	.sysfs_ops = &kobj_sysfs_ops,
	.default_attrs = klp_patch_attrs,
};

static void klp_kobj_release_func(struct kobject *kobj)
{
}

static struct kobj_type klp_ktype_func = {
	.release = klp_kobj_release_func,
	.sysfs_ops = &kobj_sysfs_ops,
};

void __weak arch_klp_free_func(struct klp_object *obj, struct klp_func *func)
{
}
/*
 * Free all functions' kobjects in the array up to some limit. When limit is
 * NULL, all kobjects are freed.
 */
static void klp_free_funcs_limited(struct klp_object *obj,
				   struct klp_func *limit)
{
	struct klp_func *func;

	for (func = obj->funcs; func->old_name && func != limit; func++) {
		arch_klp_free_func(obj, func);
		kobject_put(&func->kobj);
	}
}

/*
 * Free all objects' kobjects in the array up to some limit. When limit is
 * NULL, all kobjects are freed.
 */
static void klp_free_objects_limited(struct klp_patch *patch,
				     struct klp_object *limit)
{
	struct klp_object *obj;

	for (obj = patch->objs; obj->funcs && obj != limit; obj++) {
		if (klp_is_module(obj))
			module_put(obj->mod);

		klp_free_funcs_limited(obj, NULL);
		kobject_put(obj->kobj);
	}
}

static void klp_free_patch(struct klp_patch *patch)
{
	klp_free_objects_limited(patch, NULL);
	if (!list_empty(&patch->list))
		list_del(&patch->list);
	kobject_put(&patch->kobj);
}

static int klp_count_sysfs_funcs(struct klp_object *obj, const char *name)
{
	struct klp_func *func;
	int n = 0;
	/* count the times a function name occurs and is initialized */
	for (func = obj->funcs; func->old_name; func++) {
		if ((!strcmp(func->old_name, name) &&
			func->kobj.state_initialized))
				n++;
	}

	return n;
}

int __weak arch_klp_init_func(struct klp_object *obj, struct klp_func *func)
{
	return 0;
}

static int klp_init_func(struct klp_object *obj, struct klp_func *func)
{
	int ret;
	unsigned int count;

	ret = arch_klp_init_func(obj, func);
	if (ret)
		return ret;

	func->state = KLP_DISABLED;

	count = klp_count_sysfs_funcs(obj, func->old_name);
	if (count)
		return kobject_init_and_add(&func->kobj, &klp_ktype_func,
				obj->kobj, "%s-%d", func->old_name, count);
	else
		return kobject_init_and_add(&func->kobj, &klp_ktype_func,
				obj->kobj, "%s", func->old_name);
}

/* parts of the initialization that is done only when the object is loaded */
static int klp_init_object_loaded(struct klp_patch *patch,
				  struct klp_object *obj)
{
	struct klp_func *func;
	int ret;

	if (obj->relocs) {
		ret = klp_write_object_relocations(patch->mod, obj);
		if (ret)
			return ret;
	}

	for (func = obj->funcs; func->old_name; func++) {
		ret = klp_find_verify_func_addr(obj, func);
		if (ret)
			return ret;
	}

	return 0;
}

static int klp_init_object(struct klp_patch *patch, struct klp_object *obj)
{
	struct klp_func *func;
	int ret;
	const char *name;

	if (!obj->funcs)
		return -EINVAL;

	obj->state = KLP_DISABLED;
	obj->mod = NULL;

	ret = klp_find_object_module(obj);
	if (ret)
		return ret;

	name = klp_is_module(obj) ? obj->name : "vmlinux";
	obj->kobj = kobject_create_and_add(name, &patch->kobj);
	if (!obj->kobj)
		return -ENOMEM;

	if (klp_is_object_loaded(obj)) {
		ret = klp_init_object_loaded(patch, obj);
		if (ret)
			goto out;
	}

	for (func = obj->funcs; func->old_name; func++) {
		ret = klp_init_func(obj, func);
		if (ret)
			goto free;
	}

	return 0;

free:
	klp_free_funcs_limited(obj, func);
out:
	kobject_put(obj->kobj);
	pr_err("livepatch: klp initialize object of patch failed,ret = %d.\n",ret);
	return ret;
}

static int klp_init_patch(struct klp_patch *patch)
{
	struct klp_object *obj;
	int ret;

	if (!patch->objs)
		return -EINVAL;

	mutex_lock(&klp_mutex);

	patch->state = KLP_DISABLED;

	ret = kobject_init_and_add(&patch->kobj, &klp_ktype_patch,
				   klp_root_kobj, "%s", patch->mod->name);
	if (ret)
		goto unlock;

	for (obj = patch->objs; obj->funcs; obj++) {
		ret = klp_init_object(patch, obj);
		if (ret)
			goto free;
	}

	jump_label_register(patch->mod);

	for (obj = patch->objs; obj->funcs; obj++)
		klp_load_hook(obj);

	list_add_tail(&patch->list, &klp_patches);

	mutex_unlock(&klp_mutex);

	return 0;

free:
	klp_free_objects_limited(patch, obj);
	kobject_put(&patch->kobj);
unlock:
	mutex_unlock(&klp_mutex);
	(void)do_upload_log(UPLOADTYPE_INIT_FAIL,"init patch failed.");
	pr_err("livepatch: klp initialize patch failed,ret = %d.\n",ret);
	return ret;
}

/**
 * klp_unregister_patch() - unregisters a patch
 * @patch:	Disabled patch to be unregistered
 *
 * Frees the data structures and removes the sysfs interface.
 *
 * Return: 0 on success, otherwise error
 */
int klp_unregister_patch(struct klp_patch *patch)
{
	int ret = 0;
	struct klp_object *obj;

	mutex_lock(&klp_mutex);

	if (!klp_is_patch_registered(patch)) {
		ret = -EINVAL;
		goto out;
	}

	if (patch->state == KLP_ENABLED) {
		ret = -EBUSY;
		goto out;
	}

	klp_free_patch(patch);

	for (obj = patch->objs; obj->funcs; obj++)
		klp_unload_hook(obj);

out:
	mutex_unlock(&klp_mutex);
	(void)do_upload_log(UPLOADTYPE_INIT_FAIL,"unregister patch failed.");
	return ret;
}
EXPORT_SYMBOL_GPL(klp_unregister_patch);

/**
 * klp_register_patch() - registers a patch
 * @patch:	Patch to be registered
 *
 * Initializes the data structure associated with the patch and
 * creates the sysfs interface.
 *
 * Return: 0 on success, otherwise error
 */
int klp_register_patch(struct klp_patch *patch)
{
	int ret;

	if (!klp_initialized())
		return -ENODEV;

	if (!patch || !patch->mod)
		return -EINVAL;

	ret = klp_init_patch(patch);

	return ret;
}
EXPORT_SYMBOL_GPL(klp_register_patch);

static int state_show(struct seq_file *m, void *v)
{
	struct klp_patch *patch;
	char *state;
	int index = 0;
	seq_printf(m, "%-5s\t%-26s\t%-8s\n", "Index", "Patch", "State");
	seq_printf(m, "-----------------------------------------------\n");
	mutex_lock(&klp_mutex);
	list_for_each_entry(patch, &klp_patches, list) {
		if (patch->state == KLP_ENABLED)
			state = "enabled";
		else if (patch->state == KLP_DISABLED)
			state = "disabled";
		else
			state = "UNDEF";

		seq_printf(m, "%-5d\t%-26s\t%-8s\n", ++index, patch->mod->name, state);
	}
	mutex_unlock(&klp_mutex);
	seq_printf(m, "-----------------------------------------------\n");

	return 0;
}

static int klp_state_open(struct inode *inode, struct file *filp)
{
	return single_open(filp, state_show, NULL);
}

static const struct file_operations proc_klpstate_operations = {
	.open		= klp_state_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static int __init klp_init(void)
{
	int ret;
	struct proc_dir_entry *root_klp_dir, *res;

	if (is_hkip_enabled())
		hkip_token = get_hkip_token();
	ret = klp_check_compiler_support();
	if (ret) {
		pr_info("Your compiler is too old; turning off.\n");
		return -EINVAL;
	}

	root_klp_dir = proc_mkdir("livepatch", NULL);
	if (!root_klp_dir)
		goto error_out;

	res = proc_create("livepatch/state", 0, NULL, &proc_klpstate_operations);
	if (!res)
		goto error_remove;

	klp_root_kobj = kobject_create_and_add("livepatch", kernel_kobj);
	if (!klp_root_kobj)
		goto error_remove;

	return 0;

error_remove:
	remove_proc_entry("livepatch", NULL);
	(void)do_upload_log(UPLOADTYPE_INIT_FAIL,"kernel patch module initialize failed.");
error_out:
	return -ENOMEM;
}

module_init(klp_init);
