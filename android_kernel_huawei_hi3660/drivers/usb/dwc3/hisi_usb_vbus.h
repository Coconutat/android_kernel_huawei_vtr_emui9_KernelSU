#ifndef __HISI_USB_VBUS_H
#define __HISI_USB_VBUS_H
int hisi_usb_vbus_value(void);

struct usb_vbus_ops {
	int (*get_irq_byname)(void *device, const char *name);
};

int hisi_usb_vbus_request_irq(void *pdev, const struct usb_vbus_ops *vbus_ops);
int hisi_usb_vbus_free_irq(void *pdev);
#endif /* __HISI_USB_VBUS_H */
