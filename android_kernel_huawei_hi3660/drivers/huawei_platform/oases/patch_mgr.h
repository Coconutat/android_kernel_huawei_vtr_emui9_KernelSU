#ifndef _OASES_PATCH_MGR_H
#define _OASES_PATCH_MGR_H

#include "patch_file.h"

enum {
	OASES_PATCH_SUCCESS = 0,
	OASES_PATCH_FAILURE
};

struct oases_patch_info;

int oases_op_patch(struct oases_patch_file *pfile);

struct oases_unpatch {
	char id[PATCH_ID_LEN];
};

int oases_op_unpatch(struct oases_unpatch *unpatch);

int oases_check_patch(const char *name);

int oases_check_patch_func(const char *name);

void oases_free_patch(struct oases_patch_info *ctx);

void oases_attack_logger(struct oases_patch_info *ctx);

#endif/* _OASES_PATCH_MGR_H */
