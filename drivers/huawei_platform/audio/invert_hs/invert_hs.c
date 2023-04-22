/*
 * invert_hs.c -- invert headset driver
 *
 * Copyright (c) 2015 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*lint -e528 -e529 */
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/wakelock.h>
/*lint -save -e* */
#include <linux/workqueue.h>
/*lint -restore*/
#include <linux/regulator/consumer.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/pinctrl/consumer.h>
#include <huawei_platform/log/hw_log.h>

#include "huawei_platform/audio/invert_hs.h"

#define HWLOG_TAG invert_hs
HWLOG_REGIST();

enum invert_hs_gpio_type {
	INVERT_HS_GPIO_SOC           = 0,
	INVERT_HS_GPIO_CODEC         = 1,
};

struct invert_hs_data {
	int gpio_mic_gnd;         /* switch chip control gpio*/
	int gpio_type;
	struct mutex invert_hs_lock;
	struct wake_lock wake_lock;
	struct workqueue_struct* anc_hs_invert_ctl_delay_wq;
	struct delayed_work anc_hs_invert_ctl_delay_work;
};

static struct invert_hs_data *pdata;

/*lint -save -e* */
static inline int invert_hs_gpio_get_value(int gpio)
{
	if (pdata->gpio_type == INVERT_HS_GPIO_CODEC) {
		return gpio_get_value_cansleep(gpio);
	} else {
		return gpio_get_value(gpio);
	}
}
/*lint -restore*/

static inline void invert_hs_gpio_set_value(int gpio, int value)
{
	if (pdata->gpio_type == INVERT_HS_GPIO_CODEC) {
		gpio_set_value_cansleep(gpio, value);
	} else {
		gpio_set_value(gpio, value);
	}
}

/*lint -save -e* */
static void anc_hs_invert_ctl_work(struct work_struct* work)
{
	wake_lock(&pdata->wake_lock);
	mutex_lock(&pdata->invert_hs_lock);

	invert_hs_gpio_set_value(pdata->gpio_mic_gnd, INVERT_HS_MIC_GND_CONNECT);

	mutex_unlock(&pdata->invert_hs_lock);
	wake_unlock(&pdata->wake_lock);
}
/*lint -restore*/

/**
 * invert_hs_control - call this function to connect mic and gnd pin
 *                       for invert headset
 *
 **/
/*lint -save -e* */
int invert_hs_control(int connect)
{
	/* invert_hs driver not be probed, just return */
	if (pdata == NULL) {
		return -ENODEV;
	}

	wake_lock_timeout(&pdata->wake_lock, msecs_to_jiffies(1000));

	switch(connect) {
		case INVERT_HS_MIC_GND_DISCONNECT:
			cancel_delayed_work(&pdata->anc_hs_invert_ctl_delay_work);
			flush_workqueue(pdata->anc_hs_invert_ctl_delay_wq);

			wake_lock(&pdata->wake_lock);
			mutex_lock(&pdata->invert_hs_lock);

			invert_hs_gpio_set_value(pdata->gpio_mic_gnd, INVERT_HS_MIC_GND_DISCONNECT);

			mutex_unlock(&pdata->invert_hs_lock);
			wake_unlock(&pdata->wake_lock);

			hwlog_info("invert_hs_control: disconnect MIC and GND.");
			break;
		case INVERT_HS_MIC_GND_CONNECT:
			queue_delayed_work(pdata->anc_hs_invert_ctl_delay_wq,
					&pdata->anc_hs_invert_ctl_delay_work,
					msecs_to_jiffies(3000));
			hwlog_info("invert_hs_control: queue delay work.");
			break;
		default:
			hwlog_info("invert_hs_control: unknown connect type.");
			break;
	}

	return 0;
}
/*lint -restore*/

static const struct of_device_id invert_hs_of_match[] = {
	{
		.compatible = "huawei,invert_hs",
	},
	{ },
};
/*lint -save -e* */
MODULE_DEVICE_TABLE(of, invert_hs_of_match);
/*lint -restore*/

/* load dts config for board difference */
static void load_invert_hs_config(struct device_node *node)
{
	int temp = 0;
	/*lint -save -e* */
	if (!of_property_read_u32(node, "gpio_type", &temp)) {
	/*lint -restore*/
		pdata->gpio_type = temp;
	} else {
		pdata->gpio_type = INVERT_HS_GPIO_SOC;
	}

}

/*lint -save -e* */
static int invert_hs_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node =  dev->of_node;
	int ret = 0;
	struct pinctrl *p;
	struct pinctrl_state *pinctrl_def;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (NULL == pdata) {
		hwlog_err("cannot allocate anc hs dev data\n");
		return -ENOMEM;
	}

	mutex_init(&pdata->invert_hs_lock);
	wake_lock_init(&pdata->wake_lock, WAKE_LOCK_SUSPEND, "invert_hs_wakelock");

	pdata->anc_hs_invert_ctl_delay_wq =
		create_singlethread_workqueue("anc_hs_invert_ctl_delay_wq");
	if (!(pdata->anc_hs_invert_ctl_delay_wq)) {
		hwlog_err("%s : invert_ctl wq create failed\n", __FUNCTION__);
		ret = -ENOMEM;
		goto err_out;
	}

	INIT_DELAYED_WORK(&pdata->anc_hs_invert_ctl_delay_work, anc_hs_invert_ctl_work);

	p = devm_pinctrl_get(dev);
	hwlog_info("invert_hs:node name is %s\n", node->name);
	if (IS_ERR(p)) {
		hwlog_err("could not get pinctrl dev.\n");
		ret = -1;
		goto err_invert_ctl_delay_wq;
	}
	pinctrl_def = pinctrl_lookup_state(p, "default");
	if (IS_ERR(pinctrl_def))
		hwlog_err("could not get defstate.\n");

	ret = pinctrl_select_state(p, pinctrl_def);
	if (ret)
		hwlog_err("could not set pins to default state.\n");

	/* get mic->gnd control gpio */
	pdata->gpio_mic_gnd =  of_get_named_gpio(node, "gpios", 0);
	if (pdata->gpio_mic_gnd < 0) {
		hwlog_err("gpio_mic_gnd is unvalid!\n");
		/*lint -save -e* */
		ret = -ENOENT;
		/*lint -restore*/
		goto err_invert_ctl_delay_wq;
	}

	if (!gpio_is_valid(pdata->gpio_mic_gnd)) {
		hwlog_err("gpio is unvalid!\n");
		/*lint -save -e* */
		ret = -ENOENT;
		/*lint -restore*/
		goto gpio_mic_gnd_err;
	}

	/* applay for mic->gnd gpio */
	ret = gpio_request(pdata->gpio_mic_gnd, "gpio_mic_gnd");
	if (ret) {
		hwlog_err("error request GPIO for mic_gnd fail %d\n", ret);
		goto gpio_mic_gnd_err;
	}
	gpio_direction_output(pdata->gpio_mic_gnd, INVERT_HS_MIC_GND_CONNECT);

	/* load dts config for board difference */
	load_invert_hs_config(node);

	hwlog_info("invert_hs probe success!\n");

	return 0;


gpio_mic_gnd_err:
	gpio_free(pdata->gpio_mic_gnd);
err_invert_ctl_delay_wq:
	if (pdata->anc_hs_invert_ctl_delay_wq) {
		cancel_delayed_work(&pdata->anc_hs_invert_ctl_delay_work);
		flush_workqueue(pdata->anc_hs_invert_ctl_delay_wq);
		destroy_workqueue(pdata->anc_hs_invert_ctl_delay_wq);
	}
err_out:
	kfree(pdata);
	pdata = NULL;

	return ret;

}
/*lint -restore*/

static int invert_hs_remove(struct platform_device *pdev)
{
	if (pdata == NULL) {
		return 0;
	}

	if (pdata->anc_hs_invert_ctl_delay_wq) {
		cancel_delayed_work(&pdata->anc_hs_invert_ctl_delay_work);
		flush_workqueue(pdata->anc_hs_invert_ctl_delay_wq);
		destroy_workqueue(pdata->anc_hs_invert_ctl_delay_wq);
	}

	gpio_free(pdata->gpio_mic_gnd);

	return 0;
}

static struct platform_driver invert_hs_driver = {
	.driver = {
		.name   = "invert_hs",
		.owner  = THIS_MODULE,
		.of_match_table = invert_hs_of_match,
	},
	.probe  = invert_hs_probe,
	.remove = invert_hs_remove,
};

static int __init invert_hs_init(void)
{
	return platform_driver_register(&invert_hs_driver);
}

static void __exit invert_hs_exit(void)
{
	platform_driver_unregister(&invert_hs_driver);
}

/*lint -save -e* */
device_initcall(invert_hs_init);
module_exit(invert_hs_exit);
/*lint -restore*/

MODULE_DESCRIPTION("invert headset driver");
MODULE_LICENSE("GPL");
