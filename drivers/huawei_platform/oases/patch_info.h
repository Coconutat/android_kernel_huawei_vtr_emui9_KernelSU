#ifndef _OASES_PATCH_INFO_H_
#define _OASES_PATCH_INFO_H_

#include <linux/types.h>
#include <linux/kobject.h>

#include "patch_base.h"
#include "patch_file.h"
#include "patch_addr.h"

enum {
	STATUS_DISABLED = 0,
	STATUS_ENABLED = 1,
};

#define OASES_LOG_NODE_MAX 20

struct oases_attack_log {
	long uid;
	unsigned long count;
	unsigned long start_time;
	unsigned long end_time;
};

struct patch_callbacks {
	void (*init)(void); /* not used */
	void (*exit)(void);
	void (*enable)(void);
	void (*disable)(void);
};

struct oases_patch_info {
	char id[PATCH_ID_LEN];
	char vulnname[PATCH_NAME_LEN];
	int entries;

	unsigned int version;
	unsigned int status;

	/* allocated space for code */
	void *code_base;
	void *code_entry;
	unsigned long code_size;

	/* list of struct oases_patch_entry */
	struct list_head patches;

	/* for enable/disable/install/remove patch */
	int address_num;
	struct oases_patch_addr addresses;

	/* attack records */
	struct oases_attack_log *plog;
	unsigned int log_index;
	spinlock_t log_lock;

	struct kobject kobj;
	struct list_head list;
	int attached;

	struct patch_callbacks cbs;
};

static inline int valid_patch_pointer(void *info, void *p)
{
	struct oases_patch_info *ctx = info;
	return (p >= ctx->code_base) && (p < ctx->code_base + ctx->code_size);
}

#endif
