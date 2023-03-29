/*
* mntn_log.h   --  log print format of mntn
*/

#ifndef _MNTN_RECORD_SP_H
#define _MNTN_RECORD_SP_H
#ifdef CONFIG_HISI_RECORD_SP
extern void mntn_show_cpustack(int cpu);
extern void mntn_show_stack_cpustall(void);
extern void mntn_show_stack_othercpus(int cpu);
#else
static inline void mntn_show_cpustack(int cpu){};
static inline void mntn_show_stack_cpustall(void){};
static inline void mntn_show_stack_othercpus(int cpu){};
#endif
#endif /* _MNTN_H_ */
