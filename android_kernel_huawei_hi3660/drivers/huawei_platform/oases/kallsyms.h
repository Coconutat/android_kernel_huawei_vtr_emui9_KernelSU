#ifndef _OASES_KALLSYMS_H_
#define _OASES_KALLSYMS_H_

#define VMLINUX "vmlinux"

#define LOOKUP_VMLINUX 1
#define LOOKUP_MODULE 2
#define LOOKUP_BOTH 3

struct module;

struct oases_find_symbol {
	/* args */
	const char *mod;
	const char *name;
	int (*callback)(void *addr);

	/* ret */
	void *addr;
	unsigned long count;
	void *module;
	int type; /* lookup type */
	int api_use;
};

int oases_lookup_name_internal(struct oases_find_symbol *args);

#endif
