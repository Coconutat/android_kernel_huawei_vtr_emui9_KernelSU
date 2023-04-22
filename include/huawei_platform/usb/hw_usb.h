#ifndef _HW_USB_H_
#define _HW_USB_H_

#define HW_USB_STR_MAX_LEN          (16)

/* event types notify user-space host abnormal event  */
enum hw_usb_host_abnormal_event_type {
	USB_HOST_EVENT_NORMAL,
	USB_HOST_EVENT_POWER_INSUFFICIENT,
	USB_HOST_EVENT_HUB_TOO_DEEP,
	USB_HOST_EVENT_UNKNOW_DEVICE
};

typedef enum hw_usb_ldo_ctrl_type {
	HW_USB_LDO_CTRL_BEGIN = 0,
	HW_USB_LDO_CTRL_USB = HW_USB_LDO_CTRL_BEGIN,
	HW_USB_LDO_CTRL_COMBOPHY,
	HW_USB_LDO_CTRL_DIRECT_CHARGE,
	HW_USB_LDO_CTRL_HIFIUSB,
	HW_USB_LDO_CTRL_TYPECPD,

	HW_USB_LDO_CTRL_MAX,
} hw_usb_ldo_ctrl_type_t;

struct hw_usb_device {
	struct platform_device *pdev;
	struct device *dev;
	struct regulator *usb_phy_ldo; /* usb phy 3.3v ldo */
	char usb_speed[HW_USB_STR_MAX_LEN];
};


#ifdef CONFIG_HUAWEI_USB
extern void hw_usb_host_abnormal_event_notify(unsigned int event);

extern void hw_usb_set_usb_speed(unsigned int usb_speed);

extern int hw_usb_ldo_supply_enable(hw_usb_ldo_ctrl_type_t type);
extern int hw_usb_ldo_supply_disable(hw_usb_ldo_ctrl_type_t type);
#else
static inline void hw_usb_host_abnormal_event_notify(unsigned int event) {}

static inline void hw_usb_set_usb_speed(unsigned int usb_speed) {}

static inline int hw_usb_ldo_supply_enable(hw_usb_ldo_ctrl_type_t type) { return 0; }
static inline int hw_usb_ldo_supply_disable(hw_usb_ldo_ctrl_type_t type) { return 0; }
#endif /* end of CONFIG_HUAWEI_USB */

#endif /* end of _HW_USB_H_ */