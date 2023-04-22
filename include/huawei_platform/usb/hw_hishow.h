#ifndef _HW_HISHOW_H_
#define _HW_HISHOW_H_

#define HISHOW_DEVICE_OFFLINE      (0x1)
#define HISHOW_DEVICE_ONLINE       (0x10)

#define HISHOW_DEV_DATA_MAX        (32)
#define HISHOW_STATE_MAX           (3)

enum hw_hishow_devno {
	HISHOW_DEVICE_BEGIN = 0,

	HISHOW_UNKNOWN_DEVICE = HISHOW_DEVICE_BEGIN, /* for unkown hishow device */
	HISHOW_USB_DEVICE, /* for usb hishow device */
	HISHOW_HALL_DEVICE, /* for hall hishow device */

	HISHOW_DEVICE_END
};

enum hw_hishow_state {
	HISHOW_UNKNOWN,
	HISHOW_DISCONNECTED,
	HISHOW_CONNECTED
};

struct hw_hishow_device {
	struct platform_device *pdev;
	struct device *dev;
	int dev_state;
	int dev_no;
};

#ifdef CONFIG_HUAWEI_HISHOW
extern void hishow_notify_android_uevent(int disconnedornot, int hishow_devno);
#else
static inline void hishow_notify_android_uevent(int disconnedornot, int hishow_devno) { }
#endif /* end of CONFIG_HUAWEI_HISHOW */

#endif /* end of _HW_HISHOW_H_ */
