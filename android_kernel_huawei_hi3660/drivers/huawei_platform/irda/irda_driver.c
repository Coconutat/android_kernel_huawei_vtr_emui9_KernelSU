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
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/mutex.h>
#include "irda_driver.h"

#define HWLOG_TAG irda
HWLOG_REGIST();

struct class *irda_class;

static int irda_remote_init(void)
{
	int ret=0;

	irda_class = class_create(THIS_MODULE, "irda");
	if (IS_ERR(irda_class)) {
			ret = PTR_ERR(irda_class);
			hwlog_err("Failed to create irda class; ret:%d\n", ret);
			return ret;
		}

	ret = irda_maxim_power_config_regist();
	if (ret < 0) {
		goto error;
	}

	ret = irda_chip_type_regist();
	if (ret < 0) {
		goto error;
	}
	return 0;

error:
	hwlog_err("Failed to init irda driver");
	class_destroy(irda_class);
	return ret;
}

static void irda_remote_exit(void)
{
	irda_maxim_power_config_unregist();
	irda_chip_type_unregist();
}

module_init(irda_remote_init);
module_exit(irda_remote_exit);

MODULE_AUTHOR("Huawei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("irda power control driver");
