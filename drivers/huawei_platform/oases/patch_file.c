/*
 * patch_file.c - transform patch to code
 *
 * Copyright (C) 2016 Baidu, Inc. All Rights Reserved.
 *
 * You should have received a copy of license along with this program;
 * if not, ask for it from Baidu, Inc.
 *
 */

#if IS_ENABLED(CONFIG_MODULES)
#include <linux/moduleloader.h>
#endif

#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <linux/crc32.h>
#include <asm/cacheflush.h>

#include "util.h"
#include "patch_api.h"
#include "patch_file.h"
#include "patch_info.h"
#include "patch_mgr.h"
#include "oases_signing.h"
#include "plts.h"
#include "vmarea.h"
#include "kallsyms.h"

struct func_relocation_item {
	char* name;		 /* relocation function name */
	unsigned long addr; /* relocation function addr */
};

#define funcptr_2_ul(func) ((unsigned long)(&(func)))
#define FUNC_RELOCATION_ITEM(s) {#s, funcptr_2_ul(s)}

/* api function from patch_api.h */
static const struct func_relocation_item api_fri_list[] = {
	FUNC_RELOCATION_ITEM(oases_lookup_name),
	FUNC_RELOCATION_ITEM(oases_register_patch),
	FUNC_RELOCATION_ITEM(oases_printk),
	FUNC_RELOCATION_ITEM(oases_memset),
	FUNC_RELOCATION_ITEM(oases_memcpy),
	FUNC_RELOCATION_ITEM(oases_memcmp),
	FUNC_RELOCATION_ITEM(oases_copy_from_user),
	FUNC_RELOCATION_ITEM(oases_copy_to_user),
	FUNC_RELOCATION_ITEM(oases_get_user_u8),
	FUNC_RELOCATION_ITEM(oases_put_user_u8),
	FUNC_RELOCATION_ITEM(oases_get_user_u16),
	FUNC_RELOCATION_ITEM(oases_put_user_u16),
	FUNC_RELOCATION_ITEM(oases_get_user_u32),
	FUNC_RELOCATION_ITEM(oases_put_user_u32),
	FUNC_RELOCATION_ITEM(oases_get_user_u64),
	FUNC_RELOCATION_ITEM(oases_put_user_u64),
	FUNC_RELOCATION_ITEM(oases_get_current),
	FUNC_RELOCATION_ITEM(oases_linux_version_code),
	FUNC_RELOCATION_ITEM(oases_version),
	FUNC_RELOCATION_ITEM(oases_query_patch_info),
	FUNC_RELOCATION_ITEM(oases_setup_callbacks),
};

static void fix_oases_plt(unsigned long symbol, void* plt_entry)
{
#ifdef __aarch64__
	struct oases_plt_entry entry = {
		.ldr = 0x58000050U, /* LDR X16, loc_addr */
		.br = 0xd61f0200U, /* BR X16 */
		.addr = symbol /* lable: loc_addr */
	};
#else
	struct oases_plt_entry entry = {
		.ldr = 0xE51FF004U, /* LDR PC, [PC, #-4] */
		.addr = symbol /* DCD: loc_addr */
	};
#endif
	memcpy(plt_entry, &entry, sizeof(entry));
}

static unsigned long get_symbol_addr(const char *name)
{
	unsigned int i;
	void *addr = NULL;
	size_t listsize = ARRAY_SIZE(api_fri_list);

	for (i = 0; i < listsize; i++) {
		if (!strncmp(name, api_fri_list[i].name, RELOC_FUNC_NAME_SIZE)) {
			return api_fri_list[i].addr;
		}
	}

#if OASES_ENABLE_PRECISE_PATCH
	addr = oases_lookup_name(VMLINUX, name, NULL);
#endif

	return (unsigned long)addr;
}

static int reloc_api_function(void *code_base, const char *name, unsigned int offset)
{
	unsigned long symbol;

	symbol = get_symbol_addr(name);
	if (!symbol) {
		oases_debug("symbol:%s, offset: %#x find addr fail\n", name, offset);
		return -EINVAL;
	}

	fix_oases_plt(symbol, code_base + offset);
	return 0;
}

#if OASES_ENABLE_PRECISE_PATCH
static int reloc_symbol_data(void *code_base, void *symbols, unsigned int count)
{
	unsigned long addr = 0;
	unsigned size = 0;
	int i = 0;
	int j = 0;
	struct reloc_symbol_info *symbol;

	for (i = 0; i < count; i++) {
		symbol = (struct reloc_symbol_info *)(symbols + size);
		addr = (unsigned long)oases_lookup_name(VMLINUX, symbol->name, NULL);
		if (!addr) {
			oases_debug("symbol:%s find addr fail\n", symbol->name);
			return -EINVAL;
		}

		for (j = 0; j < symbol->count; j++)
			memcpy(code_base+symbol->offsets[j], &addr, sizeof(unsigned long));

		size = sizeof(struct reloc_function_info) + sizeof(unsigned int) * symbol->count;
	}

	return 0;
}
#endif

static void reset_data(void *data, unsigned long value)
{
	unsigned long tmp;
	tmp = *((unsigned long *)data);
	tmp += value;
	memcpy(data, &tmp, sizeof(unsigned long));
}

static int oases_patch_sig_check(struct oases_patch_file *patch,  char *data)
{
	const unsigned long markerlen = sizeof(OASES_SIG_STRING) - 1;
	int ret;

	if (patch->len < markerlen
		|| memcmp(data + patch->len - markerlen, OASES_SIG_STRING, markerlen) != 0) {
		oases_error("system sig marker check fail\n");
		return -ENOKEY;
	}

	patch->len -= markerlen;
	ret = oases_verify_sig(data, &patch->len, SIG_TYPE_VENDOR);
	if (ret < 0) {
		oases_error("vendor sig verify fail, ret=%d\n", ret);
		return ret;
	}

	if (patch->len > markerlen
		&& !memcmp(data + patch->len - markerlen, OASES_SIG_STRING, markerlen)) {
		patch->len -= markerlen;
		ret = oases_verify_sig(data, &patch->len, SIG_TYPE_OASES);
		if (ret < 0) {
			oases_error("oases sig verify fail, ret=%d\n", ret);
			return ret;
		}
	}

	return ret;
}

static int verify_patch_checksum(struct oases_patch_header *pheader, char *data, unsigned long size)
{
	unsigned int checksum;
	unsigned int offset = PATCH_MAGIC_SIZE + sizeof(pheader->checksum);

	checksum = crc32(0, data + offset, size - offset);
	if (checksum != pheader->checksum)
		return -ENOEXEC;

	return 0;
}

static int verify_patch_header(struct oases_patch_header *pheader, void *data, unsigned long size)
{
	unsigned long offset;
#if OASES_ENABLE_PRECISE_PATCH
	struct reloc_symbol_info *relsymbols;
	unsigned int i;
#endif

	if (pheader->header_size != sizeof(struct oases_patch_header))
		return -ENOEXEC;
	if (pheader->patch_size != size)
		return -ENOEXEC;

	offset = pheader->header_size;

	/* reset_data_offset and reset_data_count */
	if (pheader->reset_data_offset != offset
		|| (offset + pheader->reset_data_count * sizeof(unsigned int) >= size)) {
		oases_debug("reset_data error\n");
		return -ENOEXEC;
	}
	offset += pheader->reset_data_count * sizeof(unsigned int);

	/* reloc_func_offset and reloc_func_count */
	if (pheader->reloc_func_offset != offset
		|| (offset + pheader->reloc_func_count * sizeof(struct reloc_function_info) >= size)) {
		oases_debug("reloc_func error\n");
		return -ENOEXEC;
	}
	offset += pheader->reloc_func_count * sizeof(struct reloc_function_info);

	/* section_info_offset */
	if (pheader->section_info_offset != offset
		|| (offset + SECTION_NR * sizeof(struct section_info) >= size)) {
		oases_debug("section_info error\n");
		return -ENOEXEC;
	}
	offset += SECTION_NR * sizeof(struct section_info);

#if OASES_ENABLE_PRECISE_PATCH
	/* reloc_symbol */
	if (pheader->reloc_symbol_count) {
		if (pheader->reloc_symbol_offset != offset) {
			oases_debug("reloc_symbol error\n");
			return -ENOEXEC;
		}
		for (i = 0; i < pheader->reloc_symbol_count; i++) {
			if (offset + sizeof(struct reloc_symbol_info) >= size) {
				oases_debug("reloc_symbol error\n");
				return -ENOEXEC;
			}
			relsymbols = data + offset;
			offset += sizeof(*relsymbols);
			offset += relsymbols->count * sizeof(unsigned int);
		}
		if (offset >= size) {
			oases_debug("reloc_symbol error\n");
			return -ENOEXEC;
		}
	}
#endif

	/* code_offset and code_entry_offset */
	if (pheader->code_offset != offset
		|| offset + pheader->code_size != size
		|| pheader->code_entry_offset > (pheader->code_size - sizeof(unsigned int))) {
		oases_debug("code_offset error\n");
		return -ENOEXEC;
	}
	offset += pheader->code_size;

	/* all parsed */
	if (offset != size)
		return -ENOEXEC;

	return 0;
}

#if 0 // Huawei disabled
static int is_valid_type(unsigned int attr)
{
#if !OASES_ENABLE_PRECISE_PATCH
	unsigned int type = (attr >> 4) & 0x0F;
	if (type == PATCH_ATTR_PRECISE)
		return 0;
#endif
	return 1;
}

static int is_valid_arch(unsigned int attr)
{
	unsigned int arch = attr & 0x0F;
#ifdef __aarch64__
	return (arch == PATCH_ATTR_ARM64) ? 1: 0;
#else
	return (arch == PATCH_ATTR_ARM32) ? 1: 0;
#endif
}
#endif

static int is_valid_version(unsigned int version)
{
	if (version > OASES_VERSION)
		return 0;
	return 1;
}

static int is_valid_section_info(struct section_info *sections, unsigned int code_size)
{
	unsigned long size = 0, section_end, page_size;
	int i;
	struct section_info *s;

	for (i = 0; i < SECTION_NR; i++) {
		s = sections + i;
		/* type must be SECTION_* */
		if (s->type != i)
			return 0;
		/* section data must be within code */
		section_end = s->offset + s->size;
		if (section_end > code_size)
			return 0;
		/* size must within page */
		page_size = s->page * PAGE_SIZE;
		if (PAGE_ALIGN(s->size) != page_size)
			return 0;
		size += s->size;
	}
	/* sum of per section must match code size */
	if (size != code_size)
		return 0;

	return 1;
}

static int is_valid_reset_data(unsigned int *data, unsigned int count,
	unsigned int size)
{
	int i;

	for (i = 0; i < count; i++)
		if (data[i] > size)
			return 0;
	return 1;
}

static int is_valid_reloc_function_info(struct reloc_function_info *relfuncs,
	unsigned int count, unsigned int size)
{
	int i;

	for (i = 0; i < count; i++) {
		if (relfuncs[i].offset > size
			|| relfuncs[i].name[RELOC_FUNC_NAME_SIZE - 1] != 0
			|| relfuncs[i].name[0] == 0)
			return 0;
	}
	return 1;
}

#if OASES_ENABLE_PRECISE_PATCH
static int is_valid_reloc_symbol_info(struct reloc_symbol_info *relsymbols,
	unsigned int count, unsigned int size)
{
	int i, n;
	void *data = relsymbols;
	struct reloc_symbol_info *p;

	for (i = 0; i < count; i++) {
		p = data;
		for (n = 0; n < p->count; n++) {
			if (p->offsets[n] > size)
				return 0;
		}
		data += sizeof(*p);
		data += p->count * sizeof(unsigned int);
	}
	return 1;
}
#endif

static int oases_layout_patch(struct oases_patch_file *pfile, void *data)
{
	int ret = 0, i;
	struct oases_patch_header *pheader = data;
	unsigned long code_size = 0;
	unsigned int *redatas;
	struct reloc_function_info *relfuncs;
	struct section_info *sections;
#if OASES_ENABLE_PRECISE_PATCH
	struct reloc_symbol_info *relsymbols;
#endif

	if (strncmp(pheader->magic, PATCH_FILE_MAGIC, PATCH_MAGIC_SIZE)) {
		oases_debug("invalid PATCH_FILE_MAGIC\n");
		return -EINVAL;
	}

	ret = verify_patch_checksum(pheader, data, pfile->len);
	if (ret < 0) {
		oases_debug("invalid patch checksum\n");
		return ret;
	}

#if 0 // Huawei disabled
	ret = is_valid_arch(pheader->attr);
	if (!ret) {
		oases_debug("invalid patch arch\n");
		return -ENOEXEC;
	}

	ret = is_valid_type(pheader->attr);
	if (!ret) {
		oases_debug("invalid patch type\n");
		return -ENOEXEC;
	}

	ret = is_valid_version(pheader->oases_version);
	if (!ret) {
		oases_debug("invalid patch version\n");
		return -ENOEXEC;
	}
#endif

	ret = oases_valid_name(pheader->id, PATCH_ID_LEN - 1);
	if (ret < 0) {
		oases_debug("invalid patch id\n");
		return -EINVAL;
	}

	/* ensures that all fields are within data */
	ret = verify_patch_header(pheader, data, pfile->len);
	if (ret < 0) {
		oases_debug("invalid patch header\n");
		return ret;
	}

	/* check section info */
	ret = is_valid_section_info(data + pheader->section_info_offset, pheader->code_size);
	if (!ret) {
		oases_debug("invalid patch header, section info\n");
		return -EINVAL;
	}
	sections = (struct section_info *)(data + pheader->section_info_offset);

	for (i= 0; i < SECTION_NR; i++)
		code_size += sections[i].page * PAGE_SIZE;
	pfile->code_size = code_size;
	code_size -= sizeof(unsigned long);
	if (code_size > PATCH_MAX_MEM_SIZE) {
		oases_error("code_size(%lu) exceeds threshold\n", code_size);
		return -EINVAL;
	}
	oases_debug("code_size(%lu) check pass\n", code_size);

	/* check reset data offset */
	ret = is_valid_reset_data(data + pheader->reset_data_offset,
		pheader->reset_data_count, code_size);
	if (!ret) {
		oases_debug("invalid patch header, reset data\n");
		return -EINVAL;
	}
	redatas = data + pheader->reset_data_offset;

	/* check reloc function info */
	ret = is_valid_reloc_function_info(data + pheader->reloc_func_offset,
		pheader->reloc_func_count, code_size);
	if (!ret) {
		oases_debug("invalid patch header, reloc function info\n");
		return -EINVAL;
	}
	relfuncs = data + pheader->reloc_func_offset;

#if OASES_ENABLE_PRECISE_PATCH
	/* check reloc symbol info */
	if (pheader->reloc_symbol_count > 0) {
		ret = is_valid_reloc_symbol_info(data + pheader->reloc_symbol_offset,
			pheader->reloc_symbol_count, code_size);
		if (!ret) {
			oases_debug("invalid patch header, reloc symbol info\n");
			return -EINVAL;
		}
		relsymbols = data + pheader->reloc_symbol_offset;
	} else {
		relsymbols = NULL;
	}
#endif
	pfile->pheader = pheader;
	pfile->redatas = redatas;
	pfile->relfuncs = relfuncs;
	pfile->sections = sections;
#if OASES_ENABLE_PRECISE_PATCH
	pfile->relsymbols = relsymbols;
#endif
	pfile->codes = data + pheader->code_offset;
	return 0;
}

int oases_build_code(struct oases_patch_info *info, struct oases_patch_file *pfile)
{
	int i, ret;
	unsigned int cnt;
	unsigned long code_size = 0;
	struct oases_patch_header *pheader = pfile->pheader;
	struct section_info *sections = pfile->sections;
	struct reloc_function_info *relocinfo;
	void *code_base, *tmp;

	code_size = pfile->code_size;
	if (code_size > PATCH_MAX_MEM_SIZE) {
		oases_error("code_size(%lu) error\n", code_size);
		return -ENOMEM;
	}
	oases_debug("code_size(%lu) ok\n", code_size);
	code_base = vmalloc_exec(code_size);
	if (!code_base) {
		oases_debug("vmalloc_exec %u failed\n", code_size);
		return -ENOMEM;
	}
	memset(code_base, 0, code_size);
	/* copy sections */
	tmp = code_base;
	for (i = 0; i < SECTION_NR; i++) {
		if (sections[i].size > 0) {
			memcpy(tmp, pfile->codes + sections[i].offset, sections[i].size);
			tmp += sections[i].page * PAGE_SIZE;
		}
	}

	cnt = pheader->reset_data_count;
	for (i = 0; i < cnt; i++) {
		reset_data(code_base + pfile->redatas[i], (unsigned long)code_base);
	}

	cnt = pheader->reloc_func_count;
	for (i = 0; i < cnt; i++) {
		relocinfo = &pfile->relfuncs[i];
		ret = reloc_api_function(code_base, relocinfo->name, relocinfo->offset);
		if (ret < 0)
			goto fail;
	}

#if OASES_ENABLE_PRECISE_PATCH
	if (pfile->relsymbols) {
		ret = reloc_symbol_data(code_base, pfile->relsymbols, pheader->reloc_symbol_count);
		if (ret < 0)
			goto fail;
	}
#endif
	/* set page attr , section layout
		+++++++++++++++
		+  RX  | RW  | RO +
		+++++++++++++++
	*/
	tmp = code_base;
	for (i = 0; i < SECTION_NR; i++) {
		if (sections[i].size == 0) {
			continue;
		}
		/* already RWX */
		if (sections[i].type == SECTION_RX) {
			/* kill W for RX */
			ret = oases_set_vmarea_ro((unsigned long)tmp, sections[i].page);
			oases_debug("setting rx, ret=%d\n", ret);
#ifdef CONFIG_HISI_HHEE
			if (is_hkip_enabled()) {
				oases_debug("hkip_enabled:tmp=%pK, sections[%d].page=%lu\n", tmp, i, sections[i].page);
				aarch64_insn_disable_pxn_hkip(tmp, sections[i].page * PAGE_SIZE);
			}
#endif
		} else if (sections[i].type == SECTION_RW) {
			/* kill X for RW */
			oases_set_vmarea_nx((unsigned long)tmp, sections[i].page);
		} else {
			/* kill WX for RO */
			oases_set_vmarea_ro((unsigned long)tmp, sections[i].page);
			oases_set_vmarea_nx((unsigned long)tmp, sections[i].page);
		}
		tmp += sections[i].page * PAGE_SIZE;
	}

	flush_icache_range((unsigned long) code_base, (unsigned long)(code_base + code_size));
	info->code_base = code_base;
	info->code_entry = code_base + pheader->code_entry_offset;
	info->code_size = code_size;
	return 0;
fail:
	vfree(code_base);
	return ret;
}

int oases_init_patch_file(struct oases_patch_file *pfile, void *data)
{
	int ret;

	ret = oases_patch_sig_check(pfile, data);
	if (ret < 0)
		return ret;

	ret = oases_layout_patch(pfile, data);
	if (ret < 0)
		return ret;

	ret = oases_check_patch(pfile->pheader->id);
	if (ret < 0)
		return ret;

	return 0;
}
