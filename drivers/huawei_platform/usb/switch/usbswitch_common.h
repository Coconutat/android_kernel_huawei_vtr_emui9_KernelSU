#ifndef _USBSWITCH_COMMON_H_
#define _USBSWITCH_COMMON_H_

#include <linux/types.h>

struct usbswitch_common_ops {
	int (*manual_switch)(int input_select);
	int (*dcd_timeout_enable)(bool enable_flag);
	int (*dcd_timeout_status)(void);
	int (*manual_detach)(void);
};

int usbswitch_common_ops_register(struct usbswitch_common_ops *ops);

int usbswitch_common_manual_sw(int input_select);
int usbswitch_common_manual_detach(void);
int usbswitch_common_dcd_timeout_enable(bool enable_flag);
int usbswitch_common_dcd_timeout_status(void);

#endif /* end of _USBSWITCH_COMMON_H_ */
