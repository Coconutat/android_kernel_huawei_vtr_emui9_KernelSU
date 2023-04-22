#ifndef _OASES_PATCH_FILE_H_
#define _OASES_PATCH_FILE_H_

#include <linux/types.h>

#define PATCH_FILE_MAGIC "OASES16\0"
#define PATCH_MAGIC_SIZE 8

#define PATCH_ID_LEN 64
#define PATCH_NAME_LEN 20

#define OASES_SIG_STRING "~OASES signature appended~\n"

#define RELOC_FUNC_NAME_SIZE	64
#define PATCH_MAX_SIZE (PAGE_SIZE * 4)
#define PATCH_MAX_MEM_SIZE (PAGE_SIZE * 16)

struct oases_patch_info;

struct reloc_function_info {
	unsigned int offset;
	char name[RELOC_FUNC_NAME_SIZE];
};

#if OASES_ENABLE_PRECISE_PATCH
struct reloc_symbol_info {
	char name[RELOC_FUNC_NAME_SIZE];
	unsigned int count;
	unsigned int offsets[]; /* array size == count */
};
#endif

enum section_type {
	SECTION_RX = 0,
	SECTION_RW,
	SECTION_RO,
	SECTION_NR,
};

struct section_info {
	unsigned int type;
	unsigned int offset;
	unsigned int size;
	unsigned int page;
};

#define PATCH_ATTR_ADAPTIVE 0
#define PATCH_ATTR_PRECISE  1
#define PATCH_ATTR_ARM32 0
#define PATCH_ATTR_ARM64 1

struct oases_patch_header {
	char magic[PATCH_MAGIC_SIZE];
	unsigned int checksum;

	/* uniq vendor id */
	char id[PATCH_ID_LEN];
	/* name of the vuln */
	char vulnname[PATCH_NAME_LEN];

	unsigned int oases_version;
	unsigned int patch_version;

	unsigned int header_size;
	unsigned int patch_size;
	unsigned int reset_data_count; /* the count of reset datas with (base addres + &offset) */
	unsigned int reset_data_offset;
	unsigned int reloc_func_count; /* relocation api function count */
	unsigned int reloc_func_offset;
	unsigned int section_info_offset;
	unsigned int code_size;
	unsigned int code_offset;
	unsigned int code_entry_offset; /* code entry offset, base is code_body*/

	/* attr, PATCH_ATTR_XXX
	 +------------------------------+-------------------+
	 |	  precise_patch(4 bits) 	|	arch(4 bits)	|
	 +------------------------------+-------------------+
	 | 0000(adaptive) 0001(precise) | 0000(32) 0001(64) |
	 +------------------------------+-------------------+
	 */
	unsigned int attr;
#if OASES_ENABLE_PRECISE_PATCH
	unsigned int reloc_symbol_count;
	unsigned int reloc_symbol_offset;
#else
	unsigned int padding2;
	unsigned int padding3;
#endif
	unsigned int padding4;
};

/*
 * Layout of patch file:
 * +--------------------------------------------------------------+
 * | struct oases_patch_header                                    |
 * +--------------------------------------------------------------+
 * | reset_data_count * sizeof(int)                               |
 * +--------------------------------------------------------------+
 * | reloc_func_count * sizeof(reloc_function_info)               |
 * +--------------------------------------------------------------+
 * | SECTION_NR * sizeof(section_info)                            |
 * +--------------------------------------------------------------+
 * | OPTIONAL reloc_symbol_count * sizeof(reloc_symbol_info)      |
 * +--------------------------------------------------------------+
 * | code RX in bytes, size indicated by section_info             |
 * | data RW                                                      |
 * | data RO                                                      |
 * +--------------------------------------------------------------+
 */

struct oases_patch_file {
	struct oases_patch_header *pheader;
	unsigned int *redatas; /* unsinged int[reset_data_count] */
	struct reloc_function_info *relfuncs;
	struct section_info *sections;
#if OASES_ENABLE_PRECISE_PATCH
	struct reloc_symbol_info *relsymbols;
#endif
	char *codes;
	unsigned long len; /* patch length */
	unsigned int code_size;
};

int oases_init_patch_file(struct oases_patch_file *pfile, void *data);

int oases_layout_patch_file(struct oases_patch_file *pfile, char *data);

int oases_build_code(struct oases_patch_info *info, struct oases_patch_file *pfile);

#endif/* _OASES_PATCH_H */
