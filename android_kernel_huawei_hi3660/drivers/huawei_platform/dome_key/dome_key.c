#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>
#include <linux/kernel.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/slab.h>
#include <linux/kdev_t.h>
#include <linux/input.h>
#include <linux/irq.h>
#include <linux/workqueue.h>
#include <linux/wakelock.h>
#include <linux/time.h>
#include <linux/list.h>
#include <linux/interrupt.h>


#define DOME_KEY_COMPATIBLE_ID		"huawei,dome_key"
#define DOME_KEY_GPIO_IRQ			"huawei,dome_key_gpio"

#define HWLOG_TAG dome_key
HWLOG_REGIST();

struct dome_key_data {
	struct gpio gpio_reset;
	struct input_dev *input_dev;
};

struct dome_key_device {
	struct platform_device *pdev;
	struct device *dev;
	struct class *dome_key_class;
	struct dome_key_data pdata;
	struct delayed_work dome_delay_work;
};

struct dome_key_device *dome_key_dev;


int dome_key_input_register(struct dome_key_data *data)
{
	int ret = 0;

	if (!data)
		return -ENOMEM;

	data->input_dev = input_allocate_device();
	if (IS_ERR(data->input_dev)) {
		hwlog_err("input dev alloc failed\n");
		ret = -ENOMEM;
		return ret;
	}

	data->input_dev->name = "dome_key";

	set_bit(EV_MSC, data->input_dev->evbit);
	set_bit(MSC_SCAN, data->input_dev->mscbit);
	set_bit(EV_KEY, data->input_dev->evbit);
	set_bit(KEY_HOME, data->input_dev->keybit);
	set_bit(KEY_HOMEPAGE, data->input_dev->keybit);
	set_bit(EV_SYN, data->input_dev->evbit);

	set_bit(EV_ABS, data->input_dev->evbit);


	ret = input_register_device(data->input_dev);
	if (ret) {
		hwlog_err("dome_key regiset error %d\n", ret);
		input_free_device(data->input_dev);
	}
	return ret;
}

static void dome_key_report_delayed_work(struct work_struct *work)
{
	if(gpio_get_value_cansleep(dome_key_dev->pdata.gpio_reset.gpio)) {
		input_report_key(dome_key_dev->pdata.input_dev, KEY_HOMEPAGE, 0);
		input_sync(dome_key_dev->pdata.input_dev);
	} else {
		input_report_key(dome_key_dev->pdata.input_dev, KEY_HOMEPAGE, 1);
		input_sync(dome_key_dev->pdata.input_dev);
	}
}

static irqreturn_t dome_key_event_handler(int irq, void *demo_key_dev)
{
	schedule_delayed_work(&dome_key_dev->dome_delay_work, HZ / 50);

	return IRQ_HANDLED;
}

static int dome_key_probe(struct platform_device *pdev)
{
	int ret;
	int gpio;
	int irq;
	struct device_node *np;
	
	np = pdev->dev.of_node;
	if (np == NULL) {
		hwlog_err("%s none device\n",__func__);
		ret = -ENODEV;
		goto error;
	}

	dome_key_dev =
	    (struct dome_key_device *)kzalloc(sizeof(struct dome_key_device),
					  GFP_KERNEL);
	if (NULL == dome_key_dev) {
		hwlog_err("Failed to allocate dome_key_dev\n");
		ret = -ENOMEM;
		goto error;
	}

	gpio = of_get_named_gpio(np, DOME_KEY_GPIO_IRQ, 0);
	if (!gpio_is_valid(gpio)) {
		hwlog_err("gpio is not valid\n");
		ret = -EINVAL;
		goto free_dome_key_dev;
	}

	dome_key_input_register(&dome_key_dev->pdata);

	dome_key_dev->pdata.gpio_reset.gpio = (unsigned int)gpio;
	ret = gpio_request(dome_key_dev->pdata.gpio_reset.gpio, "dome_key_gpio");
	if (ret) {
		hwlog_err("Failed to request gpio[%ud]; ret:%d",
			  dome_key_dev->pdata.gpio_reset.gpio, ret);
		goto free_dome_key_dev;
	}

	INIT_DELAYED_WORK(&dome_key_dev->dome_delay_work,
					  dome_key_report_delayed_work);

	irq = gpio_to_irq(gpio);
	ret = request_irq(irq, dome_key_event_handler,
				IRQF_TRIGGER_FALLING | IRQF_TRIGGER_RISING | IRQF_NO_SUSPEND,
				"dome_key", dome_key_dev);
	if (ret) {
		hwlog_err("Failed to request irq; ret:%d", ret);
		goto free_gpio;
	}
//	dev_set_drvdata(dome_key_dev->dev, dome_key_dev);
	platform_set_drvdata(pdev, dome_key_dev);

	hwlog_info("dome_key device probe success\n");
	return 0;

free_gpio:
	gpio_free(dome_key_dev->pdata.gpio_reset.gpio);
free_dome_key_dev:
	kfree(dome_key_dev);
error:
	return ret;
}

static int dome_key_remove(struct platform_device *pdev)
{
#if 0
	struct dome_key_device *dome_key_dev = platform_get_drvdata(pdev);

	input_unregister_device(data->input_dev);
	device_remove_file(dome_key_dev->dev, &dev_attr_power_cfg);
	device_destroy(dome_key_dev->dome_key_class, dome_key_dev->dev->devt);
	class_destroy(dome_key_dev->dome_key_class);
	gpio_free(dome_key_dev->pdata.gpio_reset.gpio);
	kfree(dome_key_dev);
#endif
	return 0;
}

static const struct of_device_id dome_key_match_table[] = {
	{
	 .compatible = DOME_KEY_COMPATIBLE_ID,
	 .data = NULL,
	 },
	
};

MODULE_DEVICE_TABLE(of, dome_key_match_table);

static struct platform_driver dome_key_driver = {
	.probe = dome_key_probe,
	.remove = dome_key_remove,
	.driver = {
		   .name = "dome_key",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(dome_key_match_table),
		   },
};

static int dome_key_init(void)
{
	return platform_driver_register(&dome_key_driver);
}

static void dome_key_exit(void)
{
	platform_driver_unregister(&dome_key_driver);
}

module_init(dome_key_init);
module_exit(dome_key_exit);

MODULE_AUTHOR("Huawei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("dome key driver");
