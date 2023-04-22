#ifndef _HISILICON_DDR_H_
#define _HISILICON_DDR_H_

#ifdef CONFIG_HISI_DDRC_SEC
extern void dmss_ipi_handler(void);
extern void dmss_fiq_handler(void);
#else
static inline void dmss_ipi_handler(void){}
static inline void dmss_fiq_handler(void){}
#endif

#endif
