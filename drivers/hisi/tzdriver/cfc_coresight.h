#ifndef CFC_CORESIGHT_H
#define CFC_CORESIGHT_H

extern spinlock_t cfc_coresight_spinlock;
extern unsigned int *cfc_seqlock;
extern void cfc_enable_coresight(void);
extern void cfc_disable_coresight(void);

#endif
