#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/usb/hw_hishow.h>

#define HWLOG_TAG hw_hishow
HWLOG_REGIST();

static struct class *hishow_class = NULL;
static struct device *hishow_device = NULL;

struct hw_hishow_device *g_hw_hishow_di = NULL;

static const char *hishow_device_table[] = {
	[HISHOW_UNKNOWN_DEVICE] = "unknown_hishow",
	[HISHOW_USB_DEVICE] = "usb_hishow",
	[HISHOW_HALL_DEVICE] = "hall_hishow",
};

static const char *hishow_get_device_name(unsigned int devno)
{
	if ((devno > HISHOW_UNKNOWN_DEVICE) && (devno < HISHOW_DEVICE_END)) {
		return hishow_device_table[devno];
	}

	return "unknown_hishow";
}

static ssize_t hishow_dev_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	static const char *const dev_state[] = {
		[HISHOW_UNKNOWN] = "UNKNOWN",
		[HISHOW_DISCONNECTED] = "DISCONNECTED",
		[HISHOW_CONNECTED] = "CONNECTED",
	};
	char* cur_state = dev_state[HISHOW_UNKNOWN];

	if (!g_hw_hishow_di) {
		hwlog_err("error: g_hw_hishow_di is null!\n");
		return scnprintf(buf, PAGE_SIZE, "%s\n", cur_state);
	}

	hwlog_info("Hishow Dev State : %s : %x\n",(g_hw_hishow_di->dev_state == HISHOW_DEVICE_OFFLINE ? "OFFLINE" : "ONLINE"),
		g_hw_hishow_di->dev_no);

	switch (g_hw_hishow_di->dev_state) {
		case HISHOW_DEVICE_OFFLINE:
			cur_state = dev_state[HISHOW_DISCONNECTED];
		break;

		case HISHOW_DEVICE_ONLINE:
			cur_state = dev_state[HISHOW_CONNECTED];
		break;

		default:
			cur_state = dev_state[HISHOW_UNKNOWN];
		break;
	}

	return scnprintf(buf, PAGE_SIZE, "%s:%d\n", cur_state, g_hw_hishow_di->dev_no);
}

static DEVICE_ATTR(dev, S_IRUGO, hishow_dev_info_show, NULL);

static struct attribute *hishow_ctrl_attributes[] = {
	&dev_attr_dev.attr,
	NULL,
};
static const struct attribute_group hishow_attr_group = {
	.attrs = hishow_ctrl_attributes,
};

void hishow_notify_android_uevent(int disconnedornot, int hishow_devno)
{
	char *disconnected[HISHOW_STATE_MAX] = { "HISHOWDEV_STATE=DISCONNECTED", NULL, NULL };
	char *connected[HISHOW_STATE_MAX] = { "HISHOWDEV_STATE=CONNECTED", NULL, NULL };
	char *unknown[HISHOW_STATE_MAX] = { "HISHOWDEV_STATE=UNKNOWN", NULL, NULL };

	char device_data[HISHOW_DEV_DATA_MAX] = {0};

	if (IS_ERR(hishow_device) || (!g_hw_hishow_di)) {
		hwlog_err("error: hishow_device or g_hw_hishow_di is null!\n");
		return;
	}

	if (hishow_devno <= HISHOW_UNKNOWN_DEVICE || hishow_devno > HISHOW_DEVICE_END) {
		hwlog_err("error: invalid hishow_devno(%d)!\n", hishow_devno);
		return;
	}

	g_hw_hishow_di->dev_state = disconnedornot;
	g_hw_hishow_di->dev_no = hishow_devno;

	snprintf(device_data, HISHOW_DEV_DATA_MAX, "DEVICENO=%d", hishow_devno);

	switch (disconnedornot) {
		case HISHOW_DEVICE_ONLINE:
			connected[1] = device_data;
			kobject_uevent_env(&hishow_device->kobj, KOBJ_CHANGE, connected);
			hwlog_info("hishow_notify_android_uevent kobject_uevent_env connected\n");
		break;

		case HISHOW_DEVICE_OFFLINE:
			disconnected[1] = device_data;
			kobject_uevent_env(&hishow_device->kobj, KOBJ_CHANGE, disconnected);
			hwlog_info("hishow_notify_android_uevent kobject_uevent_env disconnected\n");
		break;

		default:
			unknown[1] = device_data;
			kobject_uevent_env(&hishow_device->kobj, KOBJ_CHANGE, unknown);
			hwlog_info("hishow_notify_android_uevent kobject_uevent_env unknown\n");
		break;
	}
}
EXPORT_SYMBOL_GPL(hishow_notify_android_uevent);

static void hishow_destroy_monitor_device(struct platform_device *pdev)
{
	if (!pdev) {
		return;
	}

	if (!IS_ERR(hishow_device)) {
		sysfs_remove_group(&hishow_device->kobj, &hishow_attr_group);
		device_destroy(hishow_device->class, hishow_device->devt);
	}

	if (!IS_ERR(hishow_class)) {
		class_destroy(hishow_class);
	}

	hishow_device = NULL;
	hishow_class = NULL;
}

static int hishow_init_monitor_device(struct platform_device *pdev)
{
	int ret = -1;

	if (hishow_device || hishow_class) {
		hishow_destroy_monitor_device(pdev);
	}

	hishow_class = class_create(THIS_MODULE, "hishow");
	if (IS_ERR(hishow_class)) {
		hwlog_err("error: cannot create class!\n");
		ret = PTR_ERR(hishow_class);
		goto err_init;
	}

	if (hishow_class) {
		hishow_device = device_create(hishow_class, NULL, 0, NULL, "monitor");
		if (IS_ERR(hishow_device)) {
			hwlog_err("error: sysfs device create failed!\n");
			ret = PTR_ERR(hishow_device);
			goto err_init;
		}
	}

	ret = sysfs_create_group(&hishow_device->kobj, &hishow_attr_group);
	if (ret) {
		hwlog_err("error: sysfs group create failed!\n");
		goto err_init;
	}

	return ret;

err_init:
	hishow_destroy_monitor_device(pdev);
	return ret;
}

/**********************************************************
*  Function:       hishow_probe
*  Description:    hishow module probe
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/

static int hishow_probe(struct platform_device *pdev)
{
	struct hw_hishow_device *di = NULL;
	int ret = -1;

	hwlog_info("probe begin\n");

	di = devm_kzalloc(&pdev->dev, sizeof(struct hw_hishow_device), GFP_KERNEL);
	if (!di) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_hw_hishow_di = di;
	g_hw_hishow_di->dev_state = HISHOW_DEVICE_OFFLINE;
	g_hw_hishow_di->dev_no = HISHOW_UNKNOWN_DEVICE;

	di->pdev = pdev;
	di->dev = &pdev->dev;
	if (NULL == di->pdev || NULL == di->dev) {
		hwlog_err("error: device_node is null!\n");
		goto free_mem;
	}

	ret = hishow_init_monitor_device(pdev);
	if (ret) {
		goto free_mem;
	}

	platform_set_drvdata(pdev, di);

	hwlog_info("probe end\n");
	return 0;

free_mem:
	devm_kfree(&pdev->dev, di);
	g_hw_hishow_di = NULL;
	return ret;
}

/**********************************************************
*  Function:       hishow_remove
*  Description:    hishow module remove
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int hishow_remove(struct platform_device *pdev)
{
	struct hw_hishow_device *di = platform_get_drvdata(pdev);

	hishow_destroy_monitor_device(pdev);

	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, di);

	g_hw_hishow_di = NULL;

	hwlog_info("remove end\n");

	return 0;
}


static struct of_device_id hishow_match_table[] = {
	{
		.compatible = "huawei,hishow",
		.data = NULL,
	},
	{},
};

static struct platform_driver hishow_driver = {
	.probe = hishow_probe,
	.remove = hishow_remove,
	.driver = {
		.name = "huawei,hishow",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hishow_match_table),
	},
};

/**********************************************************
*  Function:       hishow_init
*  Description:    hishow module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/

static int __init hishow_init(void)
{
	return  platform_driver_register(&hishow_driver);
}

/**********************************************************
*  Function:       hishow_exit
*  Description:    hishow module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit hishow_exit(void)
{
	platform_driver_unregister(&hishow_driver);
	return;
}

late_initcall(hishow_init);
module_exit(hishow_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei hishow event module driver");
MODULE_AUTHOR("HUAWEI Inc");
