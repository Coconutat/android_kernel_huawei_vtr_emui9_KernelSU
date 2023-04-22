#include <linux/device.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/module.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/usb.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>

#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#include <huawei_platform/usb/hw_usb.h>

#define HWLOG_TAG hw_usb
HWLOG_REGIST();

struct hw_usb_device *g_hw_usb_di = NULL;

static struct class *hw_usb_class = NULL;
static struct device *hw_usb_dev  = NULL;

static unsigned int g_hw_usb_ldo_status = 0;
static DEFINE_MUTEX(g_hw_usb_ldo_op_mutex);

static unsigned int g_hw_usb_speed = USB_SPEED_UNKNOWN;
static unsigned int g_hw_usb_abnormal_event = USB_HOST_EVENT_NORMAL;

extern void pd_dpm_send_event(enum pd_dpm_cable_event_type event);

static const char *hw_usb_ldo_ctrl_strings(hw_usb_ldo_ctrl_type_t type)
{
	static const char *const string_table[] = {
		[HW_USB_LDO_CTRL_USB] = "USB",
		[HW_USB_LDO_CTRL_COMBOPHY] = "COMBOPHY",
		[HW_USB_LDO_CTRL_DIRECT_CHARGE] = "DC",
		[HW_USB_LDO_CTRL_HIFIUSB] = "HIFIUSB",
		[HW_USB_LDO_CTRL_TYPECPD] = "TYPECPD",
	};

	if (type < HW_USB_LDO_CTRL_BEGIN || type >= HW_USB_LDO_CTRL_MAX) {
		return "illegal type";
	}

	return string_table[type];
}
int hw_usb_ldo_supply_enable(hw_usb_ldo_ctrl_type_t type)
{
	int ret = 0;

	if (type >= HW_USB_LDO_CTRL_MAX) {
		hwlog_err("error: type(%d) is invalid!\n", type);
		return -EINVAL;
	}
	hwlog_info("count(%d), type(%s)\n", g_hw_usb_ldo_status, hw_usb_ldo_ctrl_strings(type));

	if (g_hw_usb_di == NULL || g_hw_usb_di->usb_phy_ldo == NULL) {
		hwlog_err("error: g_hw_usb_di is null or usb_phy_ldo is null!\n");
		return -EINVAL;
	}

	mutex_lock(&g_hw_usb_ldo_op_mutex);

	if (g_hw_usb_ldo_status == 0) {
		ret = regulator_enable(g_hw_usb_di->usb_phy_ldo);
		if (ret) {
			hwlog_err("error: regulator enable failed(%d)!\n", ret);
			mutex_unlock(&g_hw_usb_ldo_op_mutex);
			return -EPERM;
		}
	}
	g_hw_usb_ldo_status =  g_hw_usb_ldo_status | (1 << type);

	mutex_unlock(&g_hw_usb_ldo_op_mutex);

	hwlog_info("regulator enable(%s) success\n", hw_usb_ldo_ctrl_strings(type));
	return 0;
}
EXPORT_SYMBOL_GPL(hw_usb_ldo_supply_enable);

int hw_usb_ldo_supply_disable(hw_usb_ldo_ctrl_type_t type)
{
	int ret = 0;

	if (type >= HW_USB_LDO_CTRL_MAX) {
		hwlog_err("error: type(%d) is invalid!\n", type);
		return -EINVAL;
	}
	hwlog_info("count(%d), type(%s)\n", g_hw_usb_ldo_status, hw_usb_ldo_ctrl_strings(type));

	if (g_hw_usb_di == NULL || g_hw_usb_di->usb_phy_ldo == NULL) {
		hwlog_err("error: g_hw_usb_di is null or usb_phy_ldo is null!\n");
		return -EINVAL;
	}

	mutex_lock(&g_hw_usb_ldo_op_mutex);

	if (g_hw_usb_ldo_status != 0) {
		g_hw_usb_ldo_status = g_hw_usb_ldo_status & (~(1 << type));
		if (g_hw_usb_ldo_status == 0) {
			ret = regulator_disable(g_hw_usb_di->usb_phy_ldo);
			if (ret) {
				hwlog_err("error: regulator disable failed(%d)!\n", ret);
				mutex_unlock(&g_hw_usb_ldo_op_mutex);
				return -EPERM;
			}
		}
	}

	mutex_unlock(&g_hw_usb_ldo_op_mutex);

	hwlog_info("regulator disable(%s) success\n", hw_usb_ldo_ctrl_strings(type));
	return 0;
}
EXPORT_SYMBOL_GPL(hw_usb_ldo_supply_disable);


void hw_usb_set_usb_speed(unsigned int usb_speed)
{
	g_hw_usb_speed = usb_speed;

	hwlog_info("usb_speed=%d\n", usb_speed);

	if (usb_speed == USB_SPEED_UNKNOWN) {
		pd_dpm_send_event(USB31_CABLE_OUT_EVENT);
	}

	if ((usb_speed == USB_SPEED_SUPER) || \
		(usb_speed == USB_SPEED_SUPER_PLUS)
	) {
		pd_dpm_send_event(USB31_CABLE_IN_EVENT);
	}
}
EXPORT_SYMBOL_GPL(hw_usb_set_usb_speed);

static unsigned int hw_usb_get_usb_speed(void)
{
	hwlog_info("g_hw_usb_speed=%d\n", g_hw_usb_speed);

	return g_hw_usb_speed;
}

static ssize_t hw_usb_speed_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch (hw_usb_get_usb_speed()) {
		case USB_SPEED_UNKNOWN:
			return scnprintf(buf, PAGE_SIZE, "%s", "unknown");
		break;

		case USB_SPEED_LOW:
		case USB_SPEED_FULL:
			return scnprintf(buf, PAGE_SIZE, "%s", "full-speed");
		break;

		case USB_SPEED_HIGH:
			return scnprintf(buf, PAGE_SIZE, "%s", "high-speed");
		break;

		case USB_SPEED_WIRELESS:
			return scnprintf(buf, PAGE_SIZE, "%s", "wireless-speed");
		break;

		case USB_SPEED_SUPER:
			return scnprintf(buf, PAGE_SIZE, "%s", "super-speed");
		break;

		case USB_SPEED_SUPER_PLUS:
			return scnprintf(buf, PAGE_SIZE, "%s", "super-speed-plus");
		break;

		default :
			return scnprintf(buf, PAGE_SIZE, "%s", "unknown");
		break;
	}
}

void hw_usb_host_abnormal_event_notify(unsigned int event)
{
	hwlog_info("event=%d\n", event);

	if ((USB_HOST_EVENT_HUB_TOO_DEEP == g_hw_usb_abnormal_event) && (USB_HOST_EVENT_UNKNOW_DEVICE == event)) {
		g_hw_usb_abnormal_event = USB_HOST_EVENT_HUB_TOO_DEEP;
	}
	else {
		g_hw_usb_abnormal_event = event;
	}
}
EXPORT_SYMBOL_GPL(hw_usb_host_abnormal_event_notify);

static unsigned int usb_host_get_abnormal_event(void)
{
	hwlog_info("g_hw_usb_abnormal_event=%d\n", g_hw_usb_abnormal_event);

	return g_hw_usb_abnormal_event;
}

static ssize_t hw_usb_host_abnormal_event_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	switch (usb_host_get_abnormal_event()) {
		case USB_HOST_EVENT_NORMAL:
			return scnprintf(buf, PAGE_SIZE, "%s", "normal");
		break;

		case USB_HOST_EVENT_POWER_INSUFFICIENT:
			return scnprintf(buf, PAGE_SIZE, "%s", "power_insufficient");
		break;

		case USB_HOST_EVENT_HUB_TOO_DEEP:
			return scnprintf(buf, PAGE_SIZE, "%s", "hub_too_deep");
		break;

		case USB_HOST_EVENT_UNKNOW_DEVICE:
			return scnprintf(buf, PAGE_SIZE, "%s", "unknown_device");
		break;

		default :
			return scnprintf(buf, PAGE_SIZE, "%s", "invalid");
		break;
	}
}

static DEVICE_ATTR(usb_speed, S_IRUGO, hw_usb_speed_show, NULL);
static DEVICE_ATTR(usb_event, S_IRUGO, hw_usb_host_abnormal_event_show, NULL);

static struct attribute *hw_usb_ctrl_attributes[] = {
	&dev_attr_usb_speed.attr,
	&dev_attr_usb_event.attr,
	NULL,
};

static const struct attribute_group hw_usb_attr_group = {
	.attrs = hw_usb_ctrl_attributes,
};

static int hw_usb_parse_dts(struct hw_usb_device *di)
{
	int ret = 0;
	const char *speed = NULL;

	if (di == NULL) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	ret = of_property_read_string(di->dev->of_node, "maximum-speed", &speed);
	if (ret) {
		hwlog_err("error: maximum-speed dts read failed!\n");
		return -1;
	}
	strncpy(di->usb_speed, speed, (HW_USB_STR_MAX_LEN - 1));

	hwlog_info("maximum-speed=%s\n", di->usb_speed);

	di->usb_phy_ldo = devm_regulator_get(di->dev, "usb_phy_ldo_33v");
	if (IS_ERR(di->usb_phy_ldo)) {
		hwlog_err("error: usb_phy_ldo_33v regulator dts read failed!\n");
		return -1;
	}

	ret = regulator_get_voltage(di->usb_phy_ldo);
	hwlog_info("usb_phy_ldo_33v=%d\n", ret);

	return 0;
}

static int hw_usb_probe(struct platform_device *pdev)
{
	struct hw_usb_device *di = NULL;
	int ret = -1;

	hwlog_info("probe begin\n");

	di = devm_kzalloc(&pdev->dev, sizeof(struct hw_usb_device), GFP_KERNEL);
	if (!di) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_hw_usb_di = di;

	di->pdev = pdev;
	di->dev = &pdev->dev;
	if (NULL == di->pdev || NULL == di->dev || NULL == di->dev->of_node) {
		hwlog_err("error: device_node is null!\n");
		goto free_mem;
	}

	ret = hw_usb_parse_dts(di);
	if (ret) {
		hwlog_err("error: parse dts failed!\n");
	}

	hw_usb_class = class_create(THIS_MODULE, "hw_usb");
	if (IS_ERR(hw_usb_class)) {
		hwlog_err("error: cannot create class!\n");
		goto free_dts;
	}

	if (hw_usb_class) {
		hw_usb_dev = device_create(hw_usb_class, NULL, 0, NULL, "usb");
		ret = sysfs_create_group(&hw_usb_dev->kobj, &hw_usb_attr_group);
		if (ret) {
			hwlog_err("error: sysfs group create failed!\n");
			goto free_dts;
		}
	}

	platform_set_drvdata(pdev, di);

	hwlog_info("probe end\n");
	return 0;

free_dts:
	if (!IS_ERR(di->usb_phy_ldo)) {
		regulator_put(di->usb_phy_ldo);
	}

free_mem:
	devm_kfree(&pdev->dev, di);
	g_hw_usb_di = NULL;

	return ret;
}

static int hw_usb_remove(struct platform_device *pdev)
{
	struct hw_usb_device *di = platform_get_drvdata(pdev);

	hwlog_info("remove begin\n");

	if (!IS_ERR(di->usb_phy_ldo)) {
		regulator_put(di->usb_phy_ldo);
	}

	sysfs_remove_group(&hw_usb_dev->kobj, &hw_usb_attr_group);
	platform_set_drvdata(pdev, NULL);
	devm_kfree(&pdev->dev, di);

	hw_usb_dev = NULL;
	g_hw_usb_di = NULL;

	hwlog_info("remove end\n");

	return 0;
}

static struct of_device_id hw_usb_match_table[] = {
	{
		.compatible = "huawei,huawei_usb",
		.data = NULL,
	},
	{ },
};

static struct platform_driver hw_usb_driver = {
	.probe = hw_usb_probe,
	.remove = hw_usb_remove,
	.driver = {
		.name = "huawei_usb",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hw_usb_match_table),
	},
};

static int __init hw_usb_init(void)
{
	return platform_driver_register(&hw_usb_driver);
}

static void __exit hw_usb_exit(void)
{
	platform_driver_unregister(&hw_usb_driver);
}

fs_initcall_sync(hw_usb_init);
module_exit(hw_usb_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("hw usb module driver");
MODULE_AUTHOR("HUAWEI Inc");
