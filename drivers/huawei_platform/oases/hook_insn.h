#ifndef _OASES_HOOK_INSN_H_
#define _OASES_HOOK_INSN_H_

/* no trampoline needed */
#define OASES_INSN_FLAG_NO_IC 1
#define OSAES_PLT_SIZE 16

struct oases_insn {
	void *address;
	void *origin_to;
	void *plt;
	void *trampoline;
	void *handler;
};

int oases_insn_alloc(struct oases_insn *insn, void *mod, int flags);
int oases_insn_alloc_nr(struct oases_insn **insn, void *mod, int nr, int flags);
void oases_insn_free(struct oases_insn *insn, void *mod);

int oases_insn_is_busy(struct oases_insn *insn, unsigned long addr);

#endif
