#ifndef _HISI_POWERKEY_EVENT_H
#define _HISI_POWERKEY_EVENT_H
#include  <linux/notifier.h>


typedef  enum {
	HISI_PRESS_KEY_DOWN = 0,
	HISI_PRESS_KEY_UP,
	HISI_PRESS_KEY_1S,
	HISI_PRESS_KEY_6S,
	HISI_PRESS_KEY_8S,
	HISI_PRESS_KEY_10S,
	HISI_PRESS_KEY_MAX
}HISI_POWERKEY_EVENT_T;

#ifdef  CONFIG_HISI_POWERKEY_SPMI
int hisi_powerkey_register_notifier(struct notifier_block *nb);
int hisi_powerkey_unregister_notifier(struct notifier_block *nb);
int hisi_call_powerkey_notifiers(int val,void *v);
#else
static inline int hisi_powerkey_register_notifier(struct notifier_block *nb){return 0;};
static inline int hisi_powerkey_unregister_notifier(struct notifier_block *nb){return 0;};
static inline int hisi_call_powerkey_notifiers(int val,void *v){return 0;};
#endif

#endif
