

#ifdef CONFIG_HISI_USB_FUNC_ADD_SS_DESC
/* Standard ISO IN Endpoint Descriptor for superspeed */
static struct usb_endpoint_descriptor ss_as_in_ep_desc  = {
	.bLength =		USB_DT_ENDPOINT_AUDIO_SIZE,
	.bDescriptorType =	USB_DT_ENDPOINT,
	.bEndpointAddress =	USB_DIR_IN,
	.bmAttributes =		USB_ENDPOINT_SYNC_SYNC
				| USB_ENDPOINT_XFER_ISOC,
	.wMaxPacketSize =	__constant_cpu_to_le16(IN_EP_MAX_PACKET_SIZE),
	.bInterval =		4, /* poll 1 per millisecond */
};

static struct usb_ss_ep_comp_descriptor ss_as_in_ep_comp_desc = {
	.bLength =		USB_DT_SS_EP_COMP_SIZE,
	.bDescriptorType =	USB_DT_SS_ENDPOINT_COMP,
};

static struct usb_descriptor_header *ss_audio_desc[] = {
	(struct usb_descriptor_header *)&ac_interface_desc,
	(struct usb_descriptor_header *)&ac_header_desc,

	(struct usb_descriptor_header *)&input_terminal_desc,
	(struct usb_descriptor_header *)&output_terminal_desc,
	(struct usb_descriptor_header *)&feature_unit_desc,

	(struct usb_descriptor_header *)&as_interface_alt_0_desc,
	(struct usb_descriptor_header *)&as_interface_alt_1_desc,
	(struct usb_descriptor_header *)&as_header_desc,

	(struct usb_descriptor_header *)&as_type_i_desc,

	(struct usb_descriptor_header *)&ss_as_in_ep_desc,
	(struct usb_descriptor_header *)&ss_as_in_ep_comp_desc,
	(struct usb_descriptor_header *)&as_iso_in_desc,
	NULL,
};
#endif
