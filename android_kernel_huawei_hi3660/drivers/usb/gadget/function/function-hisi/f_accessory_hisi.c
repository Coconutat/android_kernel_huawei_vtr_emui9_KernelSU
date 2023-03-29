

#ifdef CONFIG_HISI_USB_FUNC_ADD_SS_DESC
static struct usb_endpoint_descriptor acc_superspeed_in_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_IN,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor acc_superspeed_in_comp_desc = {
	.bLength =		sizeof(acc_superspeed_in_comp_desc),
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	/* the following 2 values can be tweaked if necessary */
	/* .bMaxBurst =		0, */
	/* .bmAttributes =	0, */
};

static struct usb_endpoint_descriptor acc_superspeed_out_desc = {
	.bLength                = USB_DT_ENDPOINT_SIZE,
	.bDescriptorType        = USB_DT_ENDPOINT,
	.bEndpointAddress       = USB_DIR_OUT,
	.bmAttributes           = USB_ENDPOINT_XFER_BULK,
	.wMaxPacketSize         = __constant_cpu_to_le16(1024),
};

static struct usb_ss_ep_comp_descriptor acc_superspeed_out_comp_desc = {
	.bLength =		sizeof(acc_superspeed_out_comp_desc),
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,

	/* the following 2 values can be tweaked if necessary */
	/* .bMaxBurst =		0, */
	/* .bmAttributes =	0, */
};

static struct usb_descriptor_header *ss_acc_descs[] = {
	(struct usb_descriptor_header *) &acc_interface_desc,
	(struct usb_descriptor_header *) &acc_superspeed_in_desc,
	(struct usb_descriptor_header *) &acc_superspeed_in_comp_desc,
	(struct usb_descriptor_header *) &acc_superspeed_out_desc,
	(struct usb_descriptor_header *) &acc_superspeed_out_comp_desc,
	NULL,
};
#endif
