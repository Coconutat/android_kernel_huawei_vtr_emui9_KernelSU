#ifndef _DRIVER_REGISTER.H
#define _DRIVER_REGISTER.H

extern int dhd_msg_level;
extern bool g_abs_enabled;
extern int wlanfty_status_value;

struct dev_wifi_handle {
	int (*wl_get_wrong_action_flag_handle)(void);
	int (*wl_trigger_disable_nmode_handle)(void);
};
void register_dev_wifi_handle (struct dev_wifi_handle org);
void unregister_dev_wifi_handle(void);

#endif
