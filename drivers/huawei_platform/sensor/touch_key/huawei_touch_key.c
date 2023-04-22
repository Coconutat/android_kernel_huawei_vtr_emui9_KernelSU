#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/kernel.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/delay.h>

#define TOUCH_KEY_COMPATIBLE_ID		"huawei,touch_key"
#define HWLOG_TAG touch_key

HWLOG_REGIST();



struct touch_key_device {
	struct platform_device *pdev;
	struct device *dev;
	struct input_dev *input_dev;
};

struct touch_key_device *touch_key_dev;


int touch_key_input_register(struct touch_key_device *touch_key_dev)
{
	int ret = 0;
	if (!touch_key_dev)
		return -ENOMEM;

	touch_key_dev->input_dev = input_allocate_device();
	if (IS_ERR_OR_NULL(touch_key_dev->input_dev)) {
		hwlog_err("input dev alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	touch_key_dev->input_dev->name = "touch_key";
	set_bit(EV_KEY, touch_key_dev->input_dev->evbit);
	set_bit(KEY_APPSELECT, touch_key_dev->input_dev->keybit);
	set_bit(KEY_BACK, touch_key_dev->input_dev->keybit);

	ret = input_register_device(touch_key_dev->input_dev);
	if (ret) {
		hwlog_err("touch_key register error %d\n", ret);
		input_free_device(touch_key_dev->input_dev);
	}
	return ret;
}

static int key_flag = 0;
int touch_key_report_from_sensorhub(int key, int value)
{
	if (key&1 && !(key_flag&1)) {
		input_report_key(touch_key_dev->input_dev, KEY_BACK, 1);
		input_sync(touch_key_dev->input_dev);
		hwlog_info("%s report back key! 1\n", __func__);
		key_flag |= 1;
	} else if (!(key&1) && (key_flag&1)){
		input_report_key(touch_key_dev->input_dev, KEY_BACK, 0);
		input_sync(touch_key_dev->input_dev);
		hwlog_info("%s report back key! 0\n", __func__);
		key_flag &= ~1;
	}
	if (key&2 && !(key_flag&2)) {
		input_report_key(touch_key_dev->input_dev, KEY_APPSELECT, 1);
		input_sync(touch_key_dev->input_dev);
		hwlog_info("%s report app switch key 1!\n", __func__);
		key_flag |= 2;
	} else if (!(key&2) && (key_flag&2)) {
		input_report_key(touch_key_dev->input_dev, KEY_APPSELECT, 0);
		input_sync(touch_key_dev->input_dev);
		hwlog_info("%s report app switch key 0!\n", __func__);
		key_flag &= ~2;
	}
	hwlog_info("key_flag:%d\n", key_flag);
	return 0;
}
static ssize_t show_key_attr(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	return snprintf(buf, 5, "%d\n", 1);
}

static ssize_t store_key_attr(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t size)
{
	int value = 0;
	value = simple_strtol(buf, NULL, 10);

	if(value < 0 || value >KEY_MAX)
	{
		hwlog_err("%s get unknow key value!\n", __func__);
		return -1;
	}
	input_report_key(touch_key_dev->input_dev, value, 1);
	input_sync(touch_key_dev->input_dev);
	msleep(100);
	input_report_key(touch_key_dev->input_dev, value, 0);
	input_sync(touch_key_dev->input_dev);
	hwlog_err("input key value %d!\n", value);

	return size;
}

static DEVICE_ATTR(key_attr, 0664, show_key_attr, store_key_attr);
static struct attribute *key_attributes[] = {
	&dev_attr_key_attr.attr,
	NULL
};

static const struct attribute_group key_node = {
	.attrs = key_attributes,
};

static int touch_key_probe(struct platform_device *pdev)
{
	int ret = 0;
	hwlog_info("touch_key device probe in\n");
	touch_key_dev =
	    (struct touch_key_device *)kzalloc(sizeof(struct touch_key_device),
					  GFP_KERNEL);
	if (NULL == touch_key_dev) {
		hwlog_err("Failed to allocate touch_key_dev\n");
		ret = -ENOMEM;
		goto error;
	}
	ret = touch_key_input_register(touch_key_dev);
	if(ret)
	{
		hwlog_err("touch_key_input_register fail ret =%d", ret);
		goto free_touch_key_dev;
	}
	ret = sysfs_create_group(&pdev->dev.kobj, &key_node);
	if (ret) 
	{
		hwlog_err("touch key sysfs_create_group error ret =%d", ret);
		goto unregister_input_dev;
	}
	platform_set_drvdata(pdev, pdev);
	hwlog_info("touch_key device probe success\n");
	return 0;
unregister_input_dev:
	input_unregister_device(&touch_key_dev->input_dev);
	input_free_device(&touch_key_dev->input_dev);
free_touch_key_dev:
	kfree(touch_key_dev);
error:
	return ret;
}

static int touch_key_remove(struct platform_device *pdev)
{
	kfree(touch_key_dev);
	return 0;
}

static const struct of_device_id touch_key_match_table[] = {
	{
	 .compatible = TOUCH_KEY_COMPATIBLE_ID,
	 .data = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, touch_key_match_table);

static struct platform_driver touch_key_driver = {
	.probe = touch_key_probe,
	.remove = touch_key_remove,
	.driver = {
		   .name = "touch_key",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(touch_key_match_table),
		   },
};

static int touch_key_init(void)
{
	hwlog_info("key driver  init \n");
	return platform_driver_register(&touch_key_driver);
}

static void touch_key_exit(void)
{
	platform_driver_unregister(&touch_key_driver);
}

module_init(touch_key_init);
module_exit(touch_key_exit);

MODULE_AUTHOR("Huawei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei touch key driver");
