#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/hisi/usb/hisi_usb.h>
#include "hisi_usb_vbus.h"

static int hisi_usb_platform_get_irq_byname(void *device, const char *name)
{
	struct platform_device *pdev = (struct platform_device *)device;

	if(NULL == name) {
		return 0;
	}

	return platform_get_irq_byname(pdev, name);
}

static const struct usb_vbus_ops platform_vbus_ops = {
	.get_irq_byname = hisi_usb_platform_get_irq_byname
};

static int hisi_usb_platform_vbus_probe(struct platform_device *pdev)
{
	int ret;
	pr_info("[%s]+\n", __func__);

	ret = hisi_usb_vbus_request_irq((void *)pdev, &platform_vbus_ops);
	if (ret) {
		pr_err("usb platform request irq failed: [%d] \n", ret);
	}

	pr_info("[%s]-\n", __func__);

	return ret;
}

static int hisi_usb_platform_vbus_remove(struct platform_device *pdev)
{
	hisi_usb_vbus_free_irq((void *)pdev);
	return 0;
}

static struct of_device_id hisi_usb_vbus_of_match[] = {
	{ .compatible = "hisilicon,usbvbus", },
	{ },
};

static struct platform_driver hisi_usb_vbus_drv = {
	.probe		= hisi_usb_platform_vbus_probe,
	.remove		= hisi_usb_platform_vbus_remove,
	.driver		= {
		.owner		= THIS_MODULE,
		.name		= "hisi_usb_platform_vbus",
		.of_match_table	= hisi_usb_vbus_of_match,
	},
};
module_platform_driver(hisi_usb_vbus_drv);

MODULE_AUTHOR("hisilicon");
MODULE_DESCRIPTION("This module detect USB VBUS connection/disconnection");
MODULE_LICENSE("GPL v2");
