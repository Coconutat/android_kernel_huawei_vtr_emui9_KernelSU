#ifndef _HISI_COUL_EVENT_H
#define _HISI_COUL_EVENT_H
#include  <linux/notifier.h>


enum hisi_coul_event {
    /* event below just processed in the orign flow */
    HISI_SOH_ACR = 0,
    HISI_SOH_DCR = 1,
    HISI_EEPROM_CYC = 2,

   /* event below should be processed as atom in the orign flow
    * and event name should be end with _ATOM
    */
};

#ifdef CONFIG_HISI_COUL
int hisi_coul_register_atomic_notifier(struct notifier_block *nb);
int hisi_coul_unregister_atomic_notifier(struct notifier_block *nb);
int hisi_call_coul_atomic_notifiers(int val,void *v);
int hisi_coul_register_blocking_notifier(struct notifier_block *nb);
int hisi_coul_unregister_blocking_notifier(struct notifier_block *nb);
int hisi_call_coul_blocking_notifiers(int val,void *v);
#else
int hisi_coul_register_atomic_notifier(struct notifier_block *nb){return 0;}
int hisi_coul_unregister_atomic_notifier(struct notifier_block *nb){return 0;}
int hisi_call_coul_atomic_notifiers(int val,void *v){return 0;}
int hisi_coul_register_blocking_notifier(struct notifier_block *nb){return 0;}
int hisi_coul_unregister_blocking_notifier(struct notifier_block *nb){return 0;}
int hisi_call_coul_blocking_notifiers(int val,void *v){return 0;}
#endif
#endif
