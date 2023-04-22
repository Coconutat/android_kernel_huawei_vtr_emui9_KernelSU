#ifndef _OASES_UTIL_H
#define _OASES_UTIL_H

#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/printk.h>

/* general printk */
#define oases_debug(fmt, ...)				\
do {							\
	pr_debug("[oases][%s:%d] " fmt,			\
		__func__, __LINE__, ## __VA_ARGS__);	\
} while(0)

#define oases_info(fmt, ...)				\
do {							\
	pr_info("[oases][%s:%d] " fmt,			\
		__func__, __LINE__, ## __VA_ARGS__);	\
} while(0)

#define oases_error(fmt, ...)				\
do {							\
	pr_err("[oases][%s:%d] " fmt, 			\
		__func__, __LINE__, ## __VA_ARGS__);	\
} while(0)

struct oases_patch_info;

int oases_is_null(const void *, int);
int oases_valid_name(const char *id, int maxlen);

void oases_module_lock(void);
void oases_module_unlock(void);
void *oases_ref_module(const char *name);
int oases_ref_module_ptr(void *module);
void oases_unref_module(void *module);

static inline int oases_insn_patch_nosync(void* addr, u32 insn);
int oases_insn_patch(struct oases_patch_info *info);
int oases_insn_unpatch(struct oases_patch_info *info);
int oases_remove_patch(struct oases_patch_info *info);

#ifdef CONFIG_HISI_HHEE

#include <linux/arm-smccc.h>
#include <linux/of.h>
#include <asm/insn.h>
#include <asm/cacheflush.h>
#include <linux/hisi/hisi_hhee.h>

static inline bool is_hkip_enabled(void) {
	bool ret = false;
	if (HHEE_ENABLE == hhee_check_enable())
		ret = true;
	return ret;
}

static inline void aarch64_insn_patch_text_hkip(void *place, u32 insn)
{
	extern unsigned long oases_hkip_token;
	struct arm_smccc_res res;

	arm_smccc_hvc(HHEE_HVC_LIVEPATCH, (unsigned long)place, AARCH64_INSN_SIZE, insn, 0, oases_hkip_token, 0, 0, &res);
	flush_icache_range((uintptr_t)place, (uintptr_t)place + AARCH64_INSN_SIZE);
}

static inline void aarch64_insn_disable_pxn_hkip(void *place, u32 text_size)
{
	extern unsigned long oases_hkip_token;
	struct arm_smccc_res res;

	arm_smccc_hvc(HHEE_LKM_UPDATE, (unsigned long)place, text_size, oases_hkip_token, 0, 0, 0, 0, &res);
	oases_debug("%s, res=%d, %d, %d, %d\n", __FUNCTION__, res.a0, res.a1, res.a2, res.a3);

	flush_icache_range((uintptr_t)place, (uintptr_t)place + text_size);
}

static inline int get_oases_hkip_token(unsigned long *token)
{
	struct arm_smccc_res res;

	arm_smccc_hvc(HHEE_HVC_TOKEN, 0, 0, 0, 0, 0, 0, 0, &res);
	oases_debug("%s, res.a0=%d,\n", __FUNCTION__, res.a0);

	if (res.a0 != 0) {
		return -1;
	}

	*token = res.a1;

	return 0 ;
}
#endif

#endif /* OASES_UTIL_H */
