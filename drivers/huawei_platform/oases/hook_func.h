#ifndef _OASES_HOOK_FUNC_H_
#define _OASES_HOOK_FUNC_H_

#include "patch_base.h"

struct oases_func {
	/* base */
	struct oases_patch_base base;
	/* external */
	char *name;
	char *mod;
	void *filter;
	/* internal */
	void *module; /* struct module * */
	struct oases_insn insn;
};
#define kp_func(x) ((struct oases_func *)((x)->data))

struct oases_func_pre {
	struct oases_func common;
};
#define kp_func_pre(x) ((struct oases_func_pre *)((x)->data))

struct oases_func_post {
	struct oases_func common;
};
#define kp_func_post(x) ((struct oases_func_post *)((x)->data))

#if OASES_ENABLE_REPLACEMENT_HANDLER
struct oases_func_rep {
	struct oases_func common;
};
#define kp_func_rep(x) ((struct oases_func_rep *)((x)->data))
#endif

struct oases_func_pre_post {
	struct oases_func common;
};
#define kp_func_pre_post(x) ((struct oases_func_pre_post *)((x)->data))

extern const struct oases_patch_desc oases_func_pre_ops;
extern const struct oases_patch_desc oases_func_post_ops;
#if OASES_ENABLE_REPLACEMENT_HANDLER
extern const struct oases_patch_desc oases_func_rep_ops;
#endif
extern const struct oases_patch_desc oases_func_pre_post_ops;

#endif
