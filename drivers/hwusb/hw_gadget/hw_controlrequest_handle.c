static int hw_ep0_handler(struct usb_composite_dev *cdev,
			const struct usb_ctrlrequest *ctrl);

static struct usb_ctrlrequest android_ctrl_request;

static inline int hw_ep0_config_handler(struct usb_configuration *c,
			const struct usb_ctrlrequest *ctrl)
{
	return hw_ep0_handler(c->cdev, ctrl);
}

static int hw_ep0_handler(struct usb_composite_dev *cdev,
			const struct usb_ctrlrequest *ctrl)
{
	struct usb_request *req = cdev->req;
	u16     w_index  = le16_to_cpu(ctrl->wIndex);
	u16     w_value  = le16_to_cpu(ctrl->wValue);
	u16     w_length = le16_to_cpu(ctrl->wLength);
	int     value    = -EOPNOTSUPP;

	/*
	 * for MBB spec command such like "40 A2",
	 * to support any other commands, add here.
	 */
	switch (ctrl->bRequestType) {
	case (USB_DIR_OUT | USB_TYPE_VENDOR):
		switch (ctrl->bRequest) {
		case (USB_REQ_SEND_HOST_TIME):
			/*
			 * "40 A1"-The host sends sys-time to device
			 */
			memcpy((void *)&android_ctrl_request, (void *)ctrl,
					sizeof(struct usb_ctrlrequest));
			req->context = &android_ctrl_request;
			req->complete = hw_usb_handle_host_time;
			cdev->gadget->ep0->driver_data = cdev;
			value = w_length;
			break;

		default:
			pr_warn("invalid control req%02x.%02x v%04x i%04x l%d\n",
				ctrl->bRequestType, ctrl->bRequest,    w_value, w_index, w_length);
			break;
		}
		break;

	case (USB_DIR_IN|USB_TYPE_VENDOR|USB_RECIP_DEVICE):
		switch (ctrl->bRequest) {
		case USB_REQ_VENDOR_SWITCH_MODE: {
				int mode = 0, state = 0;
				if ((ctrl->bRequestType != (USB_DIR_IN | USB_TYPE_VENDOR | USB_RECIP_DEVICE))
					|| (w_index != 0))
					goto unknown;

				/* Handle vendor customized request */
				pr_info("vendor request: %d index: %d value: %d length: %d\n",
						ctrl->bRequest, w_index, w_value, w_length);

				mode = hw_usb_port_mode_get();
				/* INDEX_FACTORY_REWORK stands for manufacture,adb */
				if (INDEX_FACTORY_REWORK == w_value) {
					state = hw_usb_port_switch_request(w_value);
				}
				value = min(w_length, (u16)(sizeof(mode) + sizeof(state)));
				memcpy(req->buf, &state, value / 2);
				memcpy(req->buf + value / 2, &mode, value / 2);
			}
			break;

		default:
			break;
		}

		break;

	default:
unknown:
		pr_warn("invalid control req%02x.%02x v%04x i%04x l%d\n",
			ctrl->bRequestType, ctrl->bRequest, w_value, w_index, w_length);
		break;
	}

	/* respond with data transfer or status phase? */
	if (value >= 0) {
		req->zero = 0;
		req->length = value;
		value = usb_ep_queue(cdev->gadget->ep0, req, GFP_ATOMIC);
		if (value < 0) {
			pr_warn("android_setup_config response err 0x%x\n", value);
		}
	}

	return value;
}
