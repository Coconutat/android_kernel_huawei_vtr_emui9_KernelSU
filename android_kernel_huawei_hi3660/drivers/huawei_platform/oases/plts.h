#ifndef _OASES_PLTS_H_
#define _OASES_PLTS_H_

#include <linux/types.h>

/*
 * arm64 plt:
 * LDR X16, <lable>
 * BR  X16
 * <label>
 * u64 target
 *
 * arm plt:
 * LDR PC, [PC, #-4]
 * DCD target
 */
struct oases_plt_entry {
	unsigned int ldr;
#ifdef __aarch64__
	unsigned int br;
#endif
	unsigned long addr;
};

void plts_lock(void *mod);
void plts_unlock(void *mod);
/* plts_*() must be called with plt lock held */
int plts_empty(void *mod);
void *plts_reserve(void *mod);
void *plts_free(void *mod, void *plt);
int plts_purge(void *mod);

int oases_plts_init(void);
void oases_plts_free(void);

#endif
