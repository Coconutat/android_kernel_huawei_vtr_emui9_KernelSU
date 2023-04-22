#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/input.h>
#include <linux/leds.h>
#include "huawei_platform/sensor/huawei_key.h"

#define KEY_BACKLIGHT_COMPATIBLE_ID		"huawei,key_backlight"
#define HWLOG_TAG key_backlight

#define CONFIG_BY_SENSOR	0
#define CONFIG_BY_DR		1

HWLOG_REGIST();

#if defined(CONFIG_INPUTHUB) || defined(CONFIG_INPUTHUB_20)
extern int huawei_set_key_backlight(void *param_t);
#else
int huawei_set_key_backlight(void *param_t)
{
	return 0;
}
#endif

#ifdef CONFIG_HW_RL_TOUCH_KEY
extern int huawei_set_dr_key_backlight(void *param_t);
#else
int huawei_set_dr_key_backlight(void *param_t)
{
	return 0;
}
#endif

int send_key_event_to_inputhub(struct key_param_t *param_t);
static void huawei_led_set_brightness(struct led_classdev *led_ldev,
				      enum led_brightness brightness);
static int ctrl_mode = 0;

struct huawei_led {
	const char *name;
	enum led_brightness brightness;
	unsigned long delay_on;
	unsigned long delay_off;
	char *default_trigger;
};

static struct led_classdev huawei_led_ldev=
{
	.name = "keyboard-backlight",
	.max_brightness = LED_FULL,
	.brightness_set = huawei_led_set_brightness,
};

static int set_brightness(void *param_t) {
	int ret = 0;
	switch(ctrl_mode){
	case CONFIG_BY_SENSOR:
		ret = huawei_set_key_backlight(param_t);
		break;
	case CONFIG_BY_DR:
		ret = huawei_set_dr_key_backlight(param_t);
		break;
	default:
		hwlog_err("not support control mode\n");
		return -EINVAL;
	}
	return ret;
}

static void huawei_led_set_brightness(struct led_classdev *led_ldev,
				      enum led_brightness brightness)
{
	int ret = 0;
	struct key_param_t param;

	hwlog_info("keypad set brightness = %d\n", brightness);

	if(brightness < 0 || brightness > 255)
	{
		hwlog_err("key_backlight value abnormal(%d)\n", brightness);
		return;
	}
	param.brightness1 = brightness;
	param.brightness2 = brightness;

	hwlog_info("keypad set key backlight1 = 0x%x, backlight2 = 0x%x\n", param.brightness1, param.brightness2);
	param.test_mode = 0;
	ret = set_brightness(&param);
	if (ret < 0) {
		hwlog_err("set key backlight err. ret:%d\n", ret);
	}
	return;
}
static ssize_t show_keybacklight_attr(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return snprintf(buf, 5, "%d\n", 1);
}

static ssize_t store_keybacklight_attr(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{	
	int ret = 0;
	int brightness = 0;
	unsigned long state = 0L;
	struct key_param_t param;

	memset(&param, 0, sizeof(param));
	if (!strict_strtol(buf, 10, &state)) {
		hwlog_info("brightness:%lu\n", state);
		brightness = (int)state;
		param.brightness1 = (uint8_t)(brightness & 0xff);
		param.brightness2 = (uint8_t)((brightness>>8) & 0xff);
		param.test_mode = 1;
		ret = set_brightness(&param);
		if (ret < 0) {
			hwlog_info("set backlight failed\n", brightness);
		}
	}
	return size;
}

static DEVICE_ATTR(keybacklight, 0664, show_keybacklight_attr,
		   store_keybacklight_attr);
static struct attribute *key_attributes[] = {
	&dev_attr_keybacklight.attr,
	NULL
};

static const struct attribute_group key_backlight_node = {
	.attrs = key_attributes,
};
static int key_backlight_probe(struct platform_device *pdev)
{
	int ret = 0;
	int val = 0;
	hwlog_info("key_backlight device probe in\n");

	ret = led_classdev_register(&pdev->dev, &huawei_led_ldev);
	if (ret < 0) {
		hwlog_err("couldn't register LED %s\n", huawei_led_ldev.name);
		goto error;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "ctrl_mode", &val);
    if (ret < 0) {
		hwlog_info("couldn't get ctrl_mode\n");
		val = 0;
	}
	ctrl_mode = val;

	ret = sysfs_create_group(&huawei_led_ldev.dev->kobj, &key_backlight_node);
	if (ret)
	{
		hwlog_err("touch key backlight sysfs_create_group error ret =%d", ret);
		goto unregister;
	}
	hwlog_info("key_backlight device probe success\n");
	return 0;

unregister:
	led_classdev_unregister(&huawei_led_ldev);
error:
	return ret;
}

static int key_backlight_remove(struct platform_device *pdev)
{
	return 0;
}

static const struct of_device_id key_backlight_match_table[] = {
	{
	 .compatible = KEY_BACKLIGHT_COMPATIBLE_ID,
	 .data = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, key_backlight_match_table);

static struct platform_driver key_backlight_driver = {
	.probe = key_backlight_probe,
	.remove = key_backlight_remove,
	.driver = {
		   .name = "key_backlight",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(key_backlight_match_table),
		   },
};

static int key_backlight_init(void)
{
	hwlog_info("key_backlight device init \n");
	return platform_driver_register(&key_backlight_driver);
}

static void key_backlight_exit(void)
{
	platform_driver_unregister(&key_backlight_driver);
}

module_init(key_backlight_init);
module_exit(key_backlight_exit);

MODULE_AUTHOR("Huawei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei touch key backlight driver");
