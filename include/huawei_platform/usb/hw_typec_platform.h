

#ifndef __HW_TYPEC_PLATFORM_H__
#define __HW_TYPEC_PLATFORM_H__

/* detect type-c protocol current for external modules */
int typec_current_mode_detect(void);

/* detect type-c port mode for external modules */
int typec_detect_port_mode(void);

/* detect type-c inserted plug orientation for external modules */
int typec_detect_cc_orientation(void);

void typec_wake_lock(struct typec_device_info *di);

void typec_wake_unlock(struct typec_device_info *di);

int typec_current_notifier_register(struct notifier_block *nb);
#endif
