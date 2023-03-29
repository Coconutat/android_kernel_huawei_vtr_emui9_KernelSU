#ifndef __LCDKIT_EXT_H_
#define __LCDKIT_EXT_H_
/*TS Event*/
enum lcdkit_pm_type
{
	LCDKIT_TS_BEFORE_SUSPEND = 0,
	LCDKIT_TS_SUSPEND_DEVICE,
	LCDKIT_TS_RESUME_DEVICE,
	LCDKIT_TS_AFTER_RESUME,
	LCDKIT_TS_IC_SHUT_DOWN,
	LCDKIT_TS_EARLY_SUSPEND,
};

/*TS sync*/
#define LCDKIT_NO_SYNC_TIMEOUT		0
#define LCDKIT_SHORT_SYNC_TIMEOUT		5

/*Function declare*/
int lcdkit_register_notifier(struct notifier_block* nb);
int lcdkit_unregister_notifier(struct notifier_block* nb);
void lcdkit_notifier_call_chain(unsigned long event, void* data);
#endif
