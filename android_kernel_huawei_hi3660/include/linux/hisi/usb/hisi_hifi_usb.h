#ifndef _HISI_HIFI_USB_H_
#define _HISI_HIFI_USB_H_
#include <linux/usb.h>

/* parameters passed to hisi_usb_check_hifi_usb_status */
enum hifi_usb_status_trigger {
	HIFI_USB_AUDIO = 1,
	HIFI_USB_TCPC,
	HIFI_USB_FB,
	HIFI_USB_PROXY,
	HIFI_USB_URB_ENQUEUE,
};

#ifdef CONFIG_USB_PROXY_HCD
/**
 * switch to hifi usb.
 * return 0 means the request was accepted, others means rejected.
 */
int hisi_usb_start_hifi_usb(void);

/**
 * Similarly to the hisi_usb_start_hifi_usb, with vbus power-off and power-on.
 */
int hisi_usb_start_hifi_usb_reset_power(void);

/**
 * switch to AP usb host from hifi usb.
 */
void hisi_usb_stop_hifi_usb(void);
void hisi_usb_stop_hifi_usb_reset_power(void);

/**
 * Wether a usb_device using hifi usb.
 * return true means the usb device is using hifi usb, others means the usb
 * device is not using hifi usb.
 */
bool hisi_usb_using_hifi_usb(struct usb_device *udev);

/**
 * Start hifi usb, but will not configure the usb phy.
 * This function should used only by hisi usb.
 */
int start_hifi_usb(void);

/**
 * Stop the hifi usb, but will not configure the usb phy.
 * This function should used only by hisi usb.
 */
void stop_hifi_usb(void);

/**
 * Rest hifi usb, including reset hifi usb states and freeing its resources.
 * This function should used only by hisi usb when switch to poweroff mode.
 */
void reset_hifi_usb(void);

int get_never_hifi_usb_value(void);

int get_always_hifi_usb_value(void);

int get_hifi_usb_retry_count(void);

int hifi_usb_hibernate(void);

int hifi_usb_revive(void);

/* This function is used to make sure hifiusb alive. If hifiusb hibernated,
 * revive it. */
int hisi_usb_check_hifi_usb_status(enum hifi_usb_status_trigger trigger);
void hisi_usb_check_huawei_earphone_device(struct usb_device *dev);
void export_usbhid_key_pressed(struct usb_device *udev, bool key_pressed);
void hifi_usb_hifi_reset_inform(void);
#else
static inline int hisi_usb_start_hifi_usb(void){return -1;}
static inline int hisi_usb_start_hifi_usb_reset_power(void){return -1;}
static inline void hisi_usb_stop_hifi_usb(void){}
static inline bool hisi_usb_using_hifi_usb(struct usb_device *udev){return false;}
static inline int start_hifi_usb(void){return -1;}
static inline void stop_hifi_usb(void){}
static inline void reset_hifi_usb(void){}
static inline int get_never_hifi_usb_value(void){return 0;}
static inline int get_always_hifi_usb_value(void){return 0;}
static inline int get_hifi_usb_retry_count(void){return 0;}
static inline int hifi_usb_hibernate(void){return -1;}
static inline int hifi_usb_revive(void){return -1;}
static inline int hisi_usb_check_hifi_usb_status(enum hifi_usb_status_trigger trigger){return -1;}
static inline void hisi_usb_check_huawei_earphone_device(struct usb_device *dev){}
static inline void export_usbhid_key_pressed(struct usb_device *udev, bool key_pressed){}
static inline void hifi_usb_hifi_reset_inform(void){}
#endif /* CONFIG_USB_PROXY_HCD */


#endif /* _HISI_HIFI_USB_H_ */
