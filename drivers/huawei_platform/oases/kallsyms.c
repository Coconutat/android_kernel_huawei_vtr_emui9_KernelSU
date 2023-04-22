/*
 * search symbol in kernel
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#include <linux/slab.h>
#include <linux/kallsyms.h>
#include <linux/module.h>
#include <asm/sections.h>
#include "kallsyms.h"
#include "util.h"

extern const unsigned long kallsyms_addresses[] __attribute__((weak));
extern const u8 kallsyms_names[] __attribute__((weak));
extern const unsigned long kallsyms_num_syms __attribute__((weak, section(".rodata")));
extern const u8 kallsyms_token_table[] __attribute__((weak));
extern const u16 kallsyms_token_index[] __attribute__((weak));
extern const unsigned long kallsyms_markers[] __attribute__((weak));

#if IS_ENABLED(CONFIG_KALLSYMS_BASE_RELATIVE)

extern const int kallsyms_offsets[] __weak;
extern const unsigned long kallsyms_relative_base
__attribute__((weak, section(".rodata")));

static unsigned long kallsyms_sym_address(int idx)
{
	if (!IS_ENABLED(CONFIG_KALLSYMS_BASE_RELATIVE))
		return kallsyms_addresses[idx];

	/* values are unsigned offsets if --absolute-percpu is not in effect */
	if (!IS_ENABLED(CONFIG_KALLSYMS_ABSOLUTE_PERCPU))
		return kallsyms_relative_base + (u32)kallsyms_offsets[idx];

	/* ...otherwise, positive offsets are absolute values */
	if (kallsyms_offsets[idx] >= 0)
		return kallsyms_offsets[idx];

	/* ...and negative offsets are relative to kallsyms_relative_base - 1 */
	return kallsyms_relative_base - 1 - kallsyms_offsets[idx];
}

#endif

/*
 * This is taken from kallsyms_expand_symbol mostly
 * return 0 means match
 */
static int kallsyms_compare_symbol(unsigned int *off, const char *name, int nlen)
{
	int diff = -1;
	int len, skipped_first = 0;
	const u8 *tptr, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[*off];
	len = *data;
	data++;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	*off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				diff = *tptr - *name;
				if (diff)
					return diff;
				name++;
				nlen--;
				if (!nlen)
					break;
			} else
				skipped_first = 1;
			tptr++;
		}
		if (!nlen)
			break;
	}

	return diff;
}

static unsigned int kallsyms_expand_symbol(unsigned int off,
					   char *result, size_t maxlen)
{
	int len, skipped_first = 0;
	const u8 *tptr, *data;

	/* Get the compressed symbol length from the first symbol byte. */
	data = &kallsyms_names[off];
	len = *data;
	data++;

	/*
	 * Update the offset to return the offset for the next symbol on
	 * the compressed stream.
	 */
	off += len + 1;

	/*
	 * For every byte on the compressed symbol data, copy the table
	 * entry for that byte.
	 */
	while (len) {
		tptr = &kallsyms_token_table[kallsyms_token_index[*data]];
		data++;
		len--;

		while (*tptr) {
			if (skipped_first) {
				if (maxlen <= 1)
					goto tail;
				*result = *tptr;
				result++;
				maxlen--;
			} else
				skipped_first = 1;
			tptr++;
		}
	}

tail:
	if (maxlen)
		*result = '\0';

	/* Return to offset to the next symbol. */
	return off;
}

static int module_lookup_name_callback(void *data, const char *name,
	struct module *mod, unsigned long addr)
{
	struct oases_find_symbol *s = data;

	if (strcmp(s->name, name))
		return 0;

#if IS_ENABLED(CONFIG_MODULES)
	if ((s->type == LOOKUP_MODULE) && strcmp(s->mod, mod->name))
		return 0;
#endif

	if (s->callback && !(*s->callback)((void *)addr)) {
		return 0;
	}

	s->addr = (void *)addr;
	s->count++;
	s->module = mod;

	if (s->count > 1) {
		return 1;
	}

	return 0;

}

/*
 * Return 1 if found a match symbol, otherwise 0 to continue search
 * Note we search until all symbols checked, trying to solve duplicate
 * symbol problem.
 */
static int vmlinux_lookup_name_callback(void *data, const char *name,
			struct module *mod, unsigned long addr)
{
	struct oases_find_symbol *s = data;

	if (strcmp(s->name, name))
		return 0;

	if (s->callback && !(*s->callback)((void *)addr)) {
		return 0;
	}

	s->addr = (void *)addr;
	s->count++;
	s->module = mod;

	if (s->count > 1) {
		return 1;
	}

	return 0;
}

/*
 * search a symbol address throughout vmlinux or modules or both,
 * return early if found duplicate symbols.
 */
static int oases_vmlinux_kallsyms_on_each_symbol(
	struct oases_find_symbol *data)
{
	int ret = 0, i;
	char namebuf[KSYM_NAME_LEN];
	int nlen;
	unsigned int off = 0, tmp;

	if (!data->name)
		return 0;
	nlen = strlen(data->name);
	if (!nlen)
		return 0;

	for (i = 0; i < kallsyms_num_syms; i++) {
		tmp = off;
		ret = kallsyms_compare_symbol(&off, data->name, nlen);
		/* only expand when prefix is matched */
		if (ret == 0) {
			kallsyms_expand_symbol(tmp, namebuf, ARRAY_SIZE(namebuf));
			ret = vmlinux_lookup_name_callback(data, namebuf, NULL,
#if IS_ENABLED(CONFIG_KALLSYMS_BASE_RELATIVE)
	kallsyms_sym_address(i));
#else
	kallsyms_addresses[i]);
#endif
			if (ret != 0)
				return ret;
		}
	}

	return ret;
}

static int oases_module_kallsyms_on_each_symbol(
	struct oases_find_symbol *data)
{
	return module_kallsyms_on_each_symbol(module_lookup_name_callback, data);
}

static int get_lookup_type(const char *mod)
{
	if (!mod)
		return LOOKUP_BOTH;

	if (strcmp(mod, VMLINUX) == 0)
		return LOOKUP_VMLINUX;

	return LOOKUP_MODULE;
}

/*
 * Return 0 if success otherwise error code.
 *
 * further check on symbol size, attributes, T/t/W/w/D/d/A...
 * e.g. 000000000000a2a0 A mc_buffer
 */
int oases_lookup_name_internal(struct oases_find_symbol *args)
{
	int ret = 0;

	args->type = get_lookup_type(args->mod);
	if (args->type & LOOKUP_VMLINUX) {
		oases_vmlinux_kallsyms_on_each_symbol(args);
	}

	if (args->type & LOOKUP_MODULE) {
		oases_module_lock();
		oases_module_kallsyms_on_each_symbol(args);
	}

	if (!args->addr || args->count != 1) {
		oases_error("invalid args->addr or args->count\n");
		ret = -EINVAL;
		goto bail;
	}

	/* oases_lookup_name api shouldn't increase module ref */
	if (!args->api_use) {
		if (args->module && !oases_ref_module_ptr(args->module)) {
			oases_error("oases_ref_module_ptr failed\n");
			ret = -EINVAL;
			goto bail;
		}
	}

bail:
	if (args->type & LOOKUP_MODULE) {
		oases_module_unlock();
	}
	return ret;
}
