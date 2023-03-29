/*
 *
 * Copyright (c) 2011-2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 *
 * Discription:
 *     3630 using gpio_203 realizing volume-up-key and gpio_204
 * realizing volume-down-key instead of KPC in kernel, only support simple
 * key-press at currunt version, not support combo-keys.
 *     6620 using gpio_12 realizing volume-up-key and gpio_13
 * realizing volume-down-key.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/input.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/pinctrl/consumer.h>
#include <linux/wakelock.h>
#include <asm/irq.h>
#include <linux/uaccess.h>
#include <huawei_platform/log/hw_log.h>

#define TRUE					(1)

#define GPIO_KEY_PRESS      	(1)
#define GPIO_KEY_RELEASE     (0)

#define GPIO_HIGH_VOLTAGE   	 	(1)
#define GPIO_LOW_VOLTAGE    	 	(0)
#define TIMER_DEBOUNCE				(15)

#define HWLOG_TAG huawei_play_key
HWLOG_REGIST();
static int support_play_key = 0;

struct huawei_gpio_key {
	struct input_dev		*input_dev;
	struct delayed_work	gpio_keyplay_work;
	struct timer_list		key_play_timer;
	int					gpio_play;
	int					key_play_irq;

	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
};

static struct wake_lock play_key_lock;

static int of_get_key_gpio(struct device_node *np, const char *propname,
			   int prop_index, int gpio_index, enum of_gpio_flags *flags)
{
	int ret = 0;

#ifdef CONFIG_GPIO_LOGIC
	ret = of_get_gpio_by_prop(np, propname, prop_index, gpio_index, flags);
#else
	ret = of_get_named_gpio(np, propname, prop_index);
#endif

	return ret;
}

static int huawei_gpio_key_open(struct input_dev *dev)
{
	return 0;
}

static void huawei_gpio_key_close(struct input_dev *dev)
{
	return;
}

static void huawei_gpio_keyplay_work(struct work_struct *work)
{
	struct huawei_gpio_key *gpio_key = container_of(work,
		struct huawei_gpio_key, gpio_keyplay_work.work);

	unsigned int keyplay_value = 0;
	unsigned int report_action = GPIO_KEY_RELEASE;

	keyplay_value = gpio_get_value((unsigned int)gpio_key->gpio_play);
	/*judge key is pressed or released.*/
	if (keyplay_value == GPIO_LOW_VOLTAGE)
		report_action = GPIO_KEY_PRESS;
	else if (keyplay_value == GPIO_HIGH_VOLTAGE)
		report_action = GPIO_KEY_RELEASE;
	else {
		hwlog_err("%s-%d: invalid gpio key_value.\n", __func__, __LINE__);
		return;
	}

	hwlog_info("%s-%d: play key %u action %u\n", __func__, __LINE__, KEY_PLAY, report_action);
	input_report_key(gpio_key->input_dev, KEY_F25, report_action);
	input_sync(gpio_key->input_dev);

	if (keyplay_value == GPIO_HIGH_VOLTAGE)
		wake_unlock(&play_key_lock);

	return;
}

static void gpio_keyplay_timer(unsigned long data)
{
	int keyplay_value;
	struct huawei_gpio_key *gpio_key = (struct huawei_gpio_key *)data;

	keyplay_value = gpio_get_value((unsigned int)gpio_key->gpio_play);
        /*judge key is pressed or released.*/
        if (keyplay_value == GPIO_LOW_VOLTAGE)
                wake_lock(&play_key_lock);

	schedule_delayed_work(&(gpio_key->gpio_keyplay_work), 0);

	return;
}

static irqreturn_t huawei_gpio_key_irq_handler(int irq, void *dev_id)
{
	struct huawei_gpio_key *gpio_key = (struct huawei_gpio_key *)dev_id;
	int key_event = 0;

	/* handle gpio key volume up & gpio key volume down event at here */
	if (support_play_key && irq == gpio_key->key_play_irq) {
		mod_timer(&(gpio_key->key_play_timer), jiffies + msecs_to_jiffies(TIMER_DEBOUNCE));
		wake_lock_timeout(&play_key_lock, 50);
	} else {
		hwlog_err("%s-%d: invalid irq %d!\n", __func__, __LINE__);
	}
	return IRQ_HANDLED;
}

#ifdef CONFIG_OF
static const struct of_device_id hs_gpio_key_match[] = {
	{ .compatible = "huawei,gpio-key" },
	{},
};
MODULE_DEVICE_TABLE(of, hs_gpio_key_match);
#endif

static int huawei_gpio_key_probe(struct platform_device* pdev)
{
	struct huawei_gpio_key *gpio_key = NULL;
	struct input_dev *input_dev = NULL;
	enum of_gpio_flags flags;
	int err =0;

	if (NULL == pdev) {
		hwlog_err("%s-%d: parameter error!\n", __func__, __LINE__);
		return -EINVAL;
	}

	dev_info(&pdev->dev, "huawei gpio key driver probes start!\n");
#ifdef CONFIG_OF
	if (!of_match_node(hs_gpio_key_match, pdev->dev.of_node)) {
		dev_err(&pdev->dev, "dev node is not match. exiting.\n");
		return -ENODEV;
	}
#endif

	err = of_property_read_u32(pdev->dev.of_node, "support_play_key",(u32 *)&support_play_key);
	if (err) {
		support_play_key = 0;
		hwlog_info("%s-%d: Not support play_key\n", __func__, __LINE__);
	} else {
		hwlog_info("%s-%d: Support play_key: %d\n", __func__, __LINE__, support_play_key);
	}

	gpio_key = devm_kzalloc(&pdev->dev, sizeof(struct huawei_gpio_key), GFP_KERNEL);
	if (!gpio_key) {
		dev_err(&pdev->dev, "Failed to allocate struct huawei_gpio_key!\n");
		return -ENOMEM;
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		dev_err(&pdev->dev, "Failed to allocate struct input_dev!\n");
		return -ENOMEM;/*lint !e429*/
	}

	input_dev->name = pdev->name;
	input_dev->id.bustype = BUS_HOST;
	input_dev->dev.parent = &pdev->dev;
	input_set_drvdata(input_dev, gpio_key);
	set_bit(EV_KEY, input_dev->evbit);
	set_bit(EV_SYN, input_dev->evbit);
	if (support_play_key) {
		set_bit(KEY_F25, input_dev->keybit);
	}
	input_dev->open = huawei_gpio_key_open;
	input_dev->close = huawei_gpio_key_close;

	gpio_key->input_dev = input_dev;

	/*initial work before we use it.*/
	if (support_play_key) {
		INIT_DELAYED_WORK(&(gpio_key->gpio_keyplay_work), huawei_gpio_keyplay_work);
		wake_lock_init(&play_key_lock, WAKE_LOCK_SUSPEND, "key_play_wake_lock");
	}

	if (support_play_key) {
		gpio_key->gpio_play = of_get_key_gpio(pdev->dev.of_node, "gpio-keyplay,gpio-irq", 0, 0, &flags);
		if (!gpio_is_valid(gpio_key->gpio_play)) {
			hwlog_info("%s-%d: gpio of play key is not valid, check DTS\n", __func__, __LINE__);
		}
		hwlog_info("%s-%d: Support play_key: %d\n", __func__, __LINE__, support_play_key);
	}

	if (support_play_key && gpio_is_valid(gpio_key->gpio_play)) {
		err = gpio_request((unsigned int)gpio_key->gpio_play, "gpio_play");
		if (err) {
			dev_err(&pdev->dev, "Fail request gpio:%d\n", gpio_key->gpio_play);
			goto err_gpio_play_req;
		}

			dev_err(&pdev->dev, " play_key request gpio:%d\n", gpio_key->gpio_play);
		hwlog_info("%s-%d: Support play_key: %d\n", __func__, __LINE__, support_play_key);
		gpio_direction_input((unsigned int)gpio_key->gpio_play);

		gpio_key->key_play_irq = gpio_to_irq((unsigned int)gpio_key->gpio_play);
		if (gpio_key->key_play_irq < 0) {
			dev_err(&pdev->dev, "Failed to get gpio key-play release irq!\n");
			err = gpio_key->key_play_irq;
			goto err_gpio_to_irq;
		}
		hwlog_info("%s-%d: Support play_key: irq %d\n", __func__, __LINE__, gpio_key->key_play_irq);
	}

	gpio_key->pctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(gpio_key->pctrl)) {
		dev_err(&pdev->dev, "failed to devm pinctrl get\n");
		err = -EINVAL;
		goto err_pinctrl;
	}
	gpio_key->pins_default = pinctrl_lookup_state(gpio_key->pctrl, PINCTRL_STATE_DEFAULT);
	if (IS_ERR(gpio_key->pins_default)) {
		dev_err(&pdev->dev, "failed to pinctrl lookup state default\n");
		err = -EINVAL;
		goto err_pinctrl_put;
	}
	gpio_key->pins_idle = pinctrl_lookup_state(gpio_key->pctrl, PINCTRL_STATE_IDLE);
	if (IS_ERR(gpio_key->pins_idle)) {
		dev_err(&pdev->dev, "failed to pinctrl lookup state idle\n");
		err = -EINVAL;
		goto err_pinctrl_put;
	}
	err = pinctrl_select_state(gpio_key->pctrl, gpio_key->pins_default);
	if (err < 0) {
		dev_err(&pdev->dev, "set iomux normal error, %d\n", err);
		goto err_pinctrl_put;
	}

	if (support_play_key) {
		setup_timer(&(gpio_key->key_play_timer), gpio_keyplay_timer, (unsigned long )gpio_key);
	}

		hwlog_info("%s-%d: Support play_key: %d\n", __func__, __LINE__, support_play_key);
	if (support_play_key && gpio_is_valid(gpio_key->gpio_play)) {
		err = request_irq(gpio_key->key_play_irq, huawei_gpio_key_irq_handler, IRQF_NO_SUSPEND | IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, pdev->name, gpio_key);
		if (err) {
			dev_err(&pdev->dev, "Failed to request release interupt handler!\n");
			goto err_play_irq_req;
		}
		hwlog_err("%s-%d: invalid irq is  play_key\n", __func__, __LINE__);
	}

	err = input_register_device(gpio_key->input_dev);
	if (err) {
		dev_err(&pdev->dev, "Failed to register input device!\n");
		goto err_register_dev;
	}

	device_init_wakeup(&pdev->dev, TRUE);
	platform_set_drvdata(pdev, gpio_key);


	dev_info(&pdev->dev, "hisi gpio key driver probes successfully!\n");
	return 0;

err_register_dev:
	if (support_play_key) {
		free_irq(gpio_key->key_play_irq, gpio_key);
	}
err_play_irq_req:
err_pinctrl_put:
	devm_pinctrl_put(gpio_key->pctrl);
err_pinctrl:
err_gpio_to_irq:
	if (support_play_key) {
		gpio_free((unsigned int)gpio_key->gpio_play);
	}
err_gpio_play_req:
	input_free_device(input_dev);
	if (support_play_key) {
		wake_lock_destroy(&play_key_lock);
	}
	pr_info(KERN_ERR "[gpiokey]K3v3 gpio key probe failed! ret = %d.\n", err);
	return err;/*lint !e593*/
}

static int huawei_gpio_key_remove(struct platform_device* pdev)
{
	struct huawei_gpio_key *gpio_key = platform_get_drvdata(pdev);

	if (gpio_key == NULL) {
		hwlog_err("%s-%d: get invalid gpio_key pointer\n", __func__, __LINE__);
		return -EINVAL;
	}

	devm_pinctrl_put(gpio_key->pctrl);

	if (support_play_key) {
		free_irq(gpio_key->key_play_irq, gpio_key);
		gpio_free((unsigned int)gpio_key->gpio_play);
		cancel_delayed_work(&(gpio_key->gpio_keyplay_work));
		wake_lock_destroy(&play_key_lock);
	}

	input_unregister_device(gpio_key->input_dev);
	platform_set_drvdata(pdev, NULL);
	kfree(gpio_key);
	gpio_key = NULL;
	return 0;
}

#ifdef CONFIG_PM
static int huawei_gpio_key_suspend(struct platform_device *pdev, pm_message_t state)
{
	int err;
	struct huawei_gpio_key *gpio_key = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s: suspend +\n", __func__);

	err = pinctrl_select_state(gpio_key->pctrl, gpio_key->pins_idle);
	if (err < 0) {
		dev_err(&pdev->dev, "set iomux normal error, %d\n", err);
	}

	dev_info(&pdev->dev, "%s: suspend -\n", __func__);
	return 0;
}

static int huawei_gpio_key_resume(struct platform_device *pdev)
{
	int err;
	struct huawei_gpio_key *gpio_key = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s: resume +\n", __func__);

	err = pinctrl_select_state(gpio_key->pctrl, gpio_key->pins_default);
	if (err < 0) {
		dev_err(&pdev->dev, "set iomux idle error, %d\n", err);
	}

	dev_info(&pdev->dev, "%s: resume -\n", __func__);
	return 0;
}
#endif

struct platform_driver huawei_gpio_key_driver = {
	.probe = huawei_gpio_key_probe,
	.remove = huawei_gpio_key_remove,
	.driver = {
		.name = "huawei_gpio_key",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(hs_gpio_key_match),
	},
#ifdef CONFIG_PM
	.suspend = huawei_gpio_key_suspend,
	.resume = huawei_gpio_key_resume,
#endif
};

module_platform_driver(huawei_gpio_key_driver);

MODULE_AUTHOR("Huawei Driver Group");
MODULE_DESCRIPTION("Huawei keypad platform driver");
MODULE_LICENSE("GPL");
