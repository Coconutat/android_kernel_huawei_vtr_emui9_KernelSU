#ifndef _DWC3_HIFI_USB_H_
#define _DWC3_HIFI_USB_H_

#if !IS_ENABLED(CFG_CONFIG_HISI_FAMA) && IS_ENABLED(CONFIG_USB_DWC3_NYET_ABNORMAL)
int ap_start_use_hifiusb(void);
void ap_stop_use_hifiusb(void);
void ap_use_hifi_usb_msg_receiver(void *msg_buf);
#else
static inline int ap_start_use_hifiusb(void) {return -ENODEV;}
static inline void ap_stop_use_hifiusb(void) {}
static inline void ap_use_hifi_usb_msg_receiver(void *msg_buf) {}
#endif

#endif /* _DWC3_HIFI_USB_H_ */

