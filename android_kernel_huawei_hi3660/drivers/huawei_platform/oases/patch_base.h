#ifndef _OASES_PATCH_BASE_H_
#define _OASES_PATCH_BASE_H_

#include <linux/types.h>
#include <linux/kobject.h>

#include "hook_insn.h"

struct oases_patch_entry {
	struct list_head list;
	int type;
	void *data;
};

struct oases_patch_desc {
	int type;
	int size;
	int usize;
	/* allocate resources */
	int (*create)(struct oases_patch_entry *patch, void *data);
	/* free resources */
	void (*destroy)(struct oases_patch_entry *patch);
	/* visit each insn */
	int (*with_each_insn)(struct oases_patch_entry *patch,
		int (*callback)(struct oases_patch_entry *patch, struct oases_insn *insn, void *data),
		void *data);

	/* return name of the patch */
	const char *(*get_name)(struct oases_patch_entry *patch);
	/* return number of targeted insns */
	int (*get_count)(struct oases_patch_entry *patch);
	/* sysfs interface */
	int (*show_info)(struct oases_patch_entry *patch, struct kobject *kobj,
		struct kobj_attribute *attr, char *buf, int off);
	/* how to jump to other place */
	u32 (*setup_jump)(struct oases_patch_entry *patch, struct oases_insn *insn);
	/* how to setup trampoline */
	int (*setup_trampoline)(struct oases_patch_entry *patch, struct oases_insn *insn);
};

struct oases_patch_base {
	const struct oases_patch_desc *vtab;
	/* backward link to struct oases_patch_info */
	void *owner;
};

/* pb_xxx requres arg0 struct oases_patch_base */
static inline const struct oases_patch_desc *pb_vtab(struct oases_patch_base *patch)
{
	return patch->vtab;
}
#define pb_vtab(x) pb_vtab((struct oases_patch_base *)(x))

static inline void *pb_owner(struct oases_patch_base *patch)
{
	return patch->owner;
}
#define pb_owner(x) pb_owner((struct oases_patch_base *)(x))

/* kp_xxx requres args0 struct oases_patch_entry */
static inline struct oases_patch_base *kp_base(struct oases_patch_entry *patch)
{
	return (struct oases_patch_base *) patch->data;
}
#define kp_base(x) kp_base((struct oases_patch_entry *)(x))

static inline const struct oases_patch_desc *kp_vtab(struct oases_patch_entry *patch)
{
	return pb_vtab((struct oases_patch_base *) patch->data);
}
#define kp_vtab(x) kp_vtab((struct oases_patch_entry *)(x))

static inline void *kp_owner(struct oases_patch_entry *patch)
{
	return pb_owner((struct oases_patch_base *) patch->data);
}
#define kp_owner(x) kp_owner((struct oases_patch_entry *)(x))

const struct oases_patch_desc *oases_patch_desc_by_type(int type);
static inline int valid_patch_type(int type)
{
	return oases_patch_desc_by_type(type) != NULL;
}
int oases_patch_is_busy(struct oases_patch_entry *patch, unsigned long addr);

#endif
