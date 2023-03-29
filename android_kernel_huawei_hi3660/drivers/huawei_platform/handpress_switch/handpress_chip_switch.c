#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>


#define HP_SWITCH_BUFF_SIZE			32
#define HP_SWITCH_MAX_COUNTS			8
#define HP_SWITCH_EXTERN_GPIO_BASE		224

#define HUAEI_HANDPRESS_SWITCH		"huawei,handpress_switch"

#define HP_SWITCH_DEFAULT_STATUS		"huawei,switch_default"
#define HP_SWITCH_COUNTS			"huawei,switch_counts"
#define HP_SWITCH_GPIOS			"huawei,switch_gpios"

#define HP_SWITCH_NAME				"hp_switch"

#define hp_bit(i)				(1<<i)
#define hp_get_bit(flag, i)			(flag & (1<<i))
#define hp_set_bit(flag, i)			(flag |= (1<<i))
#define hp_clear_bit(flag, i)			(flag &= (~(1<<i)))

#ifdef HWLOG_TAG
#undef HWLOG_TAG
#endif
#define HWLOG_TAG hp_switch
HWLOG_REGIST();

struct hp_switch_data {
	struct gpio gpio_reset[8];
};

struct hp_switch_dev {
	int status;
	int switch_counts;
	/*bit set 0: output L 1:output H*/
	int switch_default_status;
	struct platform_device *pdev;
	struct device *dev;
	struct class *switch_class;
	struct hp_switch_data pdata;
};

static int handpress_switch_cfg(struct hp_switch_dev *switch_dev,
				int enable_flag)
{
	int i = 0;
	int enable = 0;
	int chip_nums = 0;
	int input_status = 0;
	int current_status = 0;
	int default_status = 0;
	unsigned gpio = 0;

	if (!switch_dev) {
		hwlog_err("switch cfg input null\n");
		return -1;
	}

	hwlog_info("input flag:%x\n", enable_flag);
	chip_nums = switch_dev->switch_counts;
	current_status = switch_dev->status;
	default_status = switch_dev->switch_default_status;
	if (chip_nums > HP_SWITCH_MAX_COUNTS) {
		hwlog_warn("switch count overflow. %d\n", chip_nums);
		chip_nums = HP_SWITCH_MAX_COUNTS;
	}

	hwlog_info("chip nums:%d; current:%x; default:%x\n",
				chip_nums, current_status, default_status);
	for (i = 0; i < chip_nums; i++) {
		input_status = hp_get_bit(enable_flag, i);
		if (input_status == hp_get_bit(current_status, i)) {
			hwlog_info("no need to set:%d %x\n", i, input_status);
			continue;
		}

		if (hp_get_bit(input_status, i)) {
			enable = !(hp_get_bit(default_status, i));
			hp_set_bit(current_status, i);
			hwlog_info("enable enter;cus:%x\n", current_status);
			hwlog_info("gpio:%d;def:%x\n", enable, default_status);
		} else {
			enable = !!(hp_get_bit(default_status, i));
			hp_clear_bit(current_status, i);
			hwlog_info("reset enter;cus:%x\n", current_status);
			hwlog_info("gpio:%d;def:%x\n", enable, default_status);
		}

		gpio = switch_dev->pdata.gpio_reset[i].gpio;
		gpio_direction_output(gpio, enable);
	}

	switch_dev->status = current_status;
	hwlog_info("current_status:%x\n", current_status);
	return 0;
}

static int handpress_switch_state(struct hp_switch_dev *switch_dev)
{
	int i = 0;
	int chip_nums = 0;
	int current_status = 0;
	unsigned gpio = 0;

	if (!switch_dev) {
		hwlog_err("switch cfg input null\n");
		return -1;
	}

	chip_nums = switch_dev->switch_counts;
	if (chip_nums > HP_SWITCH_MAX_COUNTS) {
		hwlog_warn("switch count overflow. %d\n", chip_nums);
		chip_nums = HP_SWITCH_MAX_COUNTS;
	}

	for (i = 0; i < chip_nums; i++) {
		gpio = switch_dev->pdata.gpio_reset[i].gpio;
		if (gpio > HP_SWITCH_EXTERN_GPIO_BASE)
			current_status |= gpio_get_value_cansleep(gpio);
		else
			current_status |= gpio_get_value(gpio);
		hwlog_info("current status:%x\n", current_status);
	}

	hwlog_info("total status:%x\n", current_status);
	return 0;
}

static ssize_t handpress_switch_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int state = 0;
	struct hp_switch_dev *switch_dev = dev_get_drvdata(dev);

	state = handpress_switch_state(switch_dev);
	hwlog_info("status: input-%x; true-%x\n", switch_dev->status, state);

	return snprintf(buf, HP_SWITCH_BUFF_SIZE, "%d\n", state);
}

static ssize_t handpress_switch_set(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret = -1;
	unsigned long state = 0;
	struct hp_switch_dev *switch_dev = dev_get_drvdata(dev);

	if (!kstrtol(buf, 10, &state)) {
		ret = handpress_switch_cfg(switch_dev, (int)state);
		if (!ret)
			ret = count;
	}
	return ret;
}
static DEVICE_ATTR(hp_switch_cfg, 0660,
			  handpress_switch_get,
			  handpress_switch_set);

static int handpress_switch_gpio_init(struct device_node *np,
			struct hp_switch_dev *switch_dev)
{
	int i = 0;
	int ret = -1;
	int counts = 0;
	int gpio_t = 0;
	unsigned int gpio = 0;

	if (!switch_dev) {
		hwlog_err("[%s] input null\n", __func__);
		goto out;
	}
	counts = switch_dev->switch_counts;

	for (i = 0; i < counts; i++) {
		gpio_t = of_get_named_gpio(np, HP_SWITCH_GPIOS, i);
		if (!gpio_is_valid(gpio_t)) {
			hwlog_err("gpio is invalid; gpio:%d\n", gpio_t);
			return -EINVAL;
		}
		hwlog_info("[%s] gpio:%d\n", __func__, gpio_t);
		gpio = (unsigned int)gpio_t;
		ret = gpio_request(gpio, HP_SWITCH_NAME);
		if (ret < 0) {
			hwlog_err("gpio request err; %d\n", gpio_t);
			return -1;
		}
		switch_dev->pdata.gpio_reset[i].gpio = gpio;
	}

	ret = 0;
out:
	return ret;
}

static void handpress_switch_gpio_exit(struct hp_switch_dev *switch_dev)
{
	int i = 0;
	int counts = 0;
	unsigned gpio = 0;

	if (!switch_dev)
		return;

	counts = switch_dev->switch_counts;
	if (counts > HP_SWITCH_MAX_COUNTS) {
		hwlog_warn("switch count overflow. %d\n", counts);
		counts = HP_SWITCH_MAX_COUNTS;
	}
	for (i = 0; i < counts; i++) {
		gpio = switch_dev->pdata.gpio_reset[i].gpio;
		gpio_free(gpio);
	}
}

static int handpress_switch_probe(struct platform_device *pdev)
{
	int ret = 0;
	int switch_counts = 0;
	int switch_default = 0;
	struct device_node *np = NULL;
	struct hp_switch_dev *switch_dev = NULL;

	np = pdev->dev.of_node;
	if (np == NULL) {
		hwlog_err("none device\n");
		ret = -ENODEV;
		goto error;
	}

	switch_dev = (struct hp_switch_dev *)devm_kzalloc(&pdev->dev,
					       sizeof(struct hp_switch_dev),
					       GFP_KERNEL);
	if (NULL == switch_dev) {
		hwlog_err("Failed to allocate hp_switch_dev\n");
		ret = -ENOMEM;
		goto error;
	}

	ret = of_property_read_u32(np, HP_SWITCH_COUNTS, &switch_counts);
	if (ret) {
		hwlog_err("Failed to get power type; ret:%d\n", ret);
		goto error;
	}
	switch_dev->switch_counts = switch_counts;

	ret = of_property_read_u32(np, HP_SWITCH_DEFAULT_STATUS,
					&switch_default);
	if (ret) {
		hwlog_err("Failed to get power type; ret:%d\n", ret);
		goto error;
	}
	switch_dev->switch_default_status = switch_default;

	ret = handpress_switch_gpio_init(np, switch_dev);
	if (ret) {
		hwlog_err("Failed to init handpress_switch power.\n");
		goto error;
	}

	switch_dev->switch_class = class_create(THIS_MODULE, "hp_switch");
	if (IS_ERR(switch_dev->switch_class)) {
		ret = PTR_ERR(switch_dev->switch_class);
		hwlog_err("Failed to create  class; ret:%d\n", ret);
		goto gpio_exit;
	}

	switch_dev->dev = device_create(switch_dev->switch_class,
						NULL, MKDEV(0, 0),
						NULL, "hp_switch");
	if (IS_ERR(switch_dev->dev)) {
		ret = PTR_ERR(switch_dev->dev);
		hwlog_err("Failed to create dev; ret:%d\n", ret);
		goto free_class;
	}

	ret = device_create_file(switch_dev->dev, &dev_attr_hp_switch_cfg);
	if (ret) {
		hwlog_err("Failed to create file; ret:%d\n", ret);
		goto free_dev;
	}

	dev_set_drvdata(switch_dev->dev, switch_dev);
	platform_set_drvdata(pdev, switch_dev);

	hwlog_info("platform device probe success\n");
	return 0;

free_dev:
	device_destroy(switch_dev->switch_class, switch_dev->dev->devt);
free_class:
	class_destroy(switch_dev->switch_class);
gpio_exit:
	handpress_switch_gpio_exit(switch_dev);
error:
	return ret;
}

static int handpress_switch_remove(struct platform_device *pdev)
{
	struct hp_switch_dev *switch_dev = platform_get_drvdata(pdev);

	device_remove_file(switch_dev->dev, &dev_attr_hp_switch_cfg);
	device_destroy(switch_dev->switch_class, switch_dev->dev->devt);
	class_destroy(switch_dev->switch_class);
	handpress_switch_gpio_exit(switch_dev);
	kfree(switch_dev);

	return 0;
}

static const struct of_device_id hp_switch_match_table[] = {
	{.compatible = HUAEI_HANDPRESS_SWITCH,},
	{},
};
MODULE_DEVICE_TABLE(of, hp_switch_match_table);

static struct platform_driver handpress_switch_driver = {
	.probe = handpress_switch_probe,
	.remove = handpress_switch_remove,
	.driver = {
		   .name = "handpress_switch",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(hp_switch_match_table),
		   },
};

static int handpress_switch_remote_init(void)
{
	return platform_driver_register(&handpress_switch_driver);
}

static void handpress_switch_remote_exit(void)
{
	platform_driver_unregister(&handpress_switch_driver);
}

module_init(handpress_switch_remote_init);
module_exit(handpress_switch_remote_exit);

MODULE_AUTHOR("Huawei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("handpress switch control driver");
