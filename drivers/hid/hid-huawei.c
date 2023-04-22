/*
 *  HID driver for some huawei "special" devices
 *
 */

#include <linux/device.h>
#include <linux/usb.h>
#include <linux/hid.h>
#include <linux/module.h>

#include "hid-ids.h"

#define huawei_typec_headset_map_key_clear(c) \
	hid_map_usage_clear(hi, usage, bit, max, EV_KEY, (c))

static int huawei_typec_headset_input_mapping(struct hid_device *hdev,
	struct hid_input *hi, struct hid_field *field, struct hid_usage *usage,
	unsigned long **bit, int *max)
{
	if (HID_UP_CONSUMER != (usage->hid & HID_USAGE_PAGE))
		return 0;

	hid_dbg(hdev, "huawei typeC headset input mapping event [0x%x]\n",
		usage->hid & HID_USAGE);

	switch (usage->hid & HID_USAGE) {
	case 0x0cd: huawei_typec_headset_map_key_clear(KEY_MEDIA);	break;
	default:
		return 0;
	}

	return 1;
}

static int huawei_input_mapping(struct hid_device *hdev, struct hid_input *hi,
	struct hid_field *field, struct hid_usage *usage,
	unsigned long **bit, int *max)
{
	int ret = 0;

	if (USB_DEVICE_ID_HUAWEI_HEADSET == hdev->product)
		ret = huawei_typec_headset_input_mapping(hdev,
			hi, field, usage, bit, max);

	return ret;
}

static int huawei_probe(struct hid_device *hdev,
		const struct hid_device_id *id)
{
	int ret;

	ret = hid_parse(hdev);
	if (ret) {
		hid_err(hdev, "parse failed\n");
		goto err;
	}

	ret = hid_hw_start(hdev, HID_CONNECT_DEFAULT);
	if (ret) {
		hid_err(hdev, "hw start failed\n");
		goto err;
	}

	hid_info(hdev, "huawei typeC headset product 0x%x probe\n",
			hdev->product);
	return 0;
err:
	return ret;
}


static const struct hid_device_id huawei_devices[] = {
	{ HID_USB_DEVICE(USB_VENDOR_ID_HUAWEI, USB_DEVICE_ID_HUAWEI_HEADSET) },
	{ }
};
MODULE_DEVICE_TABLE(hid, samsung_devices);

static struct hid_driver huawei_driver = {
	.name = "huawei",
	.id_table = huawei_devices,
	.input_mapping = huawei_input_mapping,
	.probe = huawei_probe,
};
module_hid_driver(huawei_driver);

MODULE_LICENSE("GPL");
