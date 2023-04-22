#ifndef __MNTN_BC_PANIC_H
#define __MNTN_BC_PANIC_H

#ifdef CONFIG_HISI_RECOVERY_BIGCORE_PANIC
extern void record_bc_panic(void);
#else
void record_bc_panic(void){return};
#endif
#endif
