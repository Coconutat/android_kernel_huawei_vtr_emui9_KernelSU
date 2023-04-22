#ifndef _HIFI_USB_H_
#define _HIFI_USB_H_

#include <linux/mutex.h>
#include <linux/completion.h>
#include <linux/timer.h>
#include <linux/fs.h>
#include <linux/wakelock.h>
#include <linux/notifier.h>
#include <linux/usb/hifi-usb-mailbox.h>

#include "proxy-hcd.h"
#include "hifi-usb-urb-buf.h"
#include "hifi-usb-stat.h"

#define HIFI_USB_CONFIRM_UDEV_CONNECT_TIME (2 * HZ)
#define HIFI_USB_CONFIRM_UDEV_RECONNECT_TIME (5 * HZ)

#define MAX_QUIRK_DEVICES_ONE_GROUP 255

struct hifi_usb_msg_wrap {
	struct list_head node;
	struct hifi_usb_op_msg msg;
};

struct hifi_usb_phy_ldo_cfg {
	u32 addr;
	u32 bit;
	u32 always_on;
	u32 accessable; /* 0: not accessable, non-zero: accessable */
};

struct hifi_usb_proxy {
	struct proxy_hcd_client 	*client;
	struct dentry 			*debugfs_root;

	/* for message process */
	struct mutex 			msg_lock;
	struct completion 		msg_completion;
	struct work_struct		msg_work;
	struct list_head		msg_queue;
	struct hifi_usb_op_msg 		op_msg;
	struct hifi_usb_runstop_msg 	runstop_msg;
	unsigned int			runstop;

	struct urb_buffers 		urb_bufs;
	struct list_head		complete_urb_list;
	struct timer_list 		confirm_udev_timer;
	struct wake_lock 		hifi_usb_wake_lock;

	spinlock_t			lock; /* for complete_urb_list */

	struct hifi_usb_stats 		stat;

	/* for hibernation */
	unsigned int			hibernation_policy;
	unsigned int			hibernation_state:1;
	unsigned int			hibernation_support:1;
	unsigned int 			ignore_port_status_change_once:1;

	struct notifier_block		fb_notify;
	unsigned int			hibernation_ctrl; /* hibernation allowed when all bits cleared */

	__u32				port_status;
	unsigned int			hibernation_count;
	unsigned int			revive_time;
	unsigned int 			max_revive_time;

	bool				hifiusb_suspended;
	bool				hifiusb_hibernating;
	bool				hid_key_pressed;

	struct hifi_usb_phy_ldo_cfg	hifi_usb_phy_ldo_33v;
	struct hifi_usb_phy_ldo_cfg	hifi_usb_phy_ldo_18v;

#ifdef CONFIG_HIFI_USB_HAS_H2X
	/* for onetrack of apr es and cs
	 * es: using h2x
	 * cs: using x2x
	 */
	bool usb_not_using_h2x;
#endif
};

extern u32 hifi_usb_base_quirk_devices[MAX_QUIRK_DEVICES_ONE_GROUP + 1];
extern u32 hifi_usb_ext_quirk_devices[MAX_QUIRK_DEVICES_ONE_GROUP + 1];

void hifi_usb_msg_receiver(struct hifi_usb_op_msg *__msg);
void hifi_usb_announce_udev(struct usb_device *udev);
int always_use_hifi_usb(int val);
int never_use_hifi_usb(int val);

#define HUAWEI_USB_C_AUDIO_ADAPTER "HUAWEI USB-C TO 3.5MM AUDIO ADAPTER"

#endif /* _HIFI_USB_H_ */
