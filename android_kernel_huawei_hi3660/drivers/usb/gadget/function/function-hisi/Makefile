obj-$(CONFIG_USB_F_BALONG_ACM)	+= usb_f_balong_acm.o
usb_f_balong_acm-y		:= f_balong_acm.o balong_acm.o usb_vendor.o
usb_f_balong_acm-y		+= hisi_adp_acm.o hisi_adp_usb.o hisi_adp_acm_test.o

usb_f_balong_acm-$(CONFIG_HISI_DEBUG_FS)	+= hiusb_debug.o

obj-$(CONFIG_USB_HW_F_ACM)	+= usb_hw_f_acm.o
usb_hw_f_acm-y			:= hw_f_acm.o hw_u_serial.o

obj-y += hisi_usb_interface_fake.o
