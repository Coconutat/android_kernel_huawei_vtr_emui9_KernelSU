#ifndef _OASES_VMAREA_H_
#define _OASES_VMAREA_H_

#include <asm/pgtable.h>

int oases_set_vmarea_ro(unsigned long addr, int numpages);
int oases_set_vmarea_rw(unsigned long addr, int numpages);
int oases_set_vmarea_nx(unsigned long addr, int numpages);
int oases_set_vmarea_x(unsigned long addr, int numpages);

#endif
