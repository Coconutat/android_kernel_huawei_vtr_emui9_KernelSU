


#ifndef _HUAWEI_CHARGER_COMMON
#define _HUAWEI_CHARGER_COMMON

#include <linux/types.h>

struct charge_extra_ops {
	bool (*check_ts)(void);
	bool (*check_otg_state)(void);
	enum fcp_check_stage_type (*get_stage)(void);
	enum usb_charger_type (*get_charger_type)(void);
	int (*set_state)(int state);
	int (*get_charge_current)(void);
};

int charge_extra_ops_register(struct charge_extra_ops *ops);
bool charge_check_charger_ts(void);
bool charge_check_charger_otg_state(void);
enum fcp_check_stage_type fcp_get_stage_status(void);
enum usb_charger_type charge_get_charger_type(void);
int charge_set_charge_state(int state);
int charge_check_charger_plugged(void);
int charge_check_input_dpm_state(void);
int get_charger_vbus_vol(void);
int get_charge_current_max(void);
int get_charger_ibus_curr(void);

#endif
