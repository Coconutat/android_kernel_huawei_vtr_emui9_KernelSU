/*
 * hisi_vibrator.c - Hisilicon PMIC vibrator driver
 *
 * Copyright (C) 2013 Hisilicon Ltd.
 * Copyright (C) 2013 Linaro Ltd.
 *
 * This file is subject to the terms and conditions of the GNU General
 * Public License. See the file "COPYING" in the main directory of this
 * archive for more details.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of_device.h>
#include <linux/mutex.h>
#include <linux/leds.h>
#include "../../hisi/tzdriver/libhwsecurec/securec.h"
#include <linux/regulator/consumer.h>
#include <linux/mfd/hisi_pmic.h>
#include <linux/hisi/hisi_vibrator.h>

volatile int vibrator_shake = 0;
EXPORT_SYMBOL(vibrator_shake);

#define HISI_VIBRATOR_NOT_USE_DR 0
#define HISI_VIBRATOR_TIMEOUT_MIN 30

/**
 * struct hisi_vibrator_data
 * @dev: device struct
 * @cdev: LED class device for this vibrator
 * @lock: resource lock
 * @ldo: power supply
 * @min_voltage: min voltage for ldo
 * @max_voltage: max voltage for ldo
 * @vibrator_reg_on: enabled register of ldo
 * @vibrator_reg_off: disabled register of ldo
 * @vibrator_bit_on: mask bit
 */
struct hisi_vibrator_data {
	struct device *dev;
	struct led_classdev cdev;
	/* struct mutex lock; */
	struct regulator *ldo;
	unsigned min_voltage;
	unsigned max_voltage;
	unsigned vibrator_reg_on;
	unsigned vibrator_reg_off;
	unsigned vibrator_bit_on;
	unsigned vibrator_use_dr;
	unsigned vibrator_reg_current;
	unsigned vibrator_bit_current;
};

/*
static void  hisi_vibrator_ldo_ctrl(struct led_classdev *cdev, int state)
{
	struct hisi_vibrator_data *vdata
		= container_of(cdev, struct hisi_vibrator_data, cdev);
	int ret = 0;

	if (vdata == NULL) {
		pr_err("%s: vibrator data is NULL", __FUNCTION__);
		return;
	}
	if (vdata->ldo == NULL) {
		dev_err(vdata->dev, "no pwero ldo");
		return;
	}

	mutex_lock(&vdata->lock);
	if (state) {
		vibrator_shake = 1;
		ret = regulator_enable(vdata->ldo);
	} else {
		vibrator_shake = 0;
		ret = regulator_disable(vdata->ldo);
	}
	mutex_unlock(&vdata->lock);

	if (ret)
		dev_err(vdata->dev, "failed to ctrl power ldo\n");
	return;
}
*/

static void hisi_vibrator_ldo_ctrl(struct led_classdev *cdev, enum led_brightness state)
{
	struct hisi_vibrator_data *vdata
		= container_of(cdev, struct hisi_vibrator_data, cdev);

	if (vdata == NULL) {
		pr_err("%s: vibrator data is NULL", __FUNCTION__);
		return;
	}
	/* no blocking, the lock should not be used */
	/* mutex_lock(&vdata->lock); */
	if (state) {
		vibrator_shake = 1;
		hisi_pmic_reg_write(vdata->vibrator_reg_on,
				vdata->vibrator_bit_on);
		dev_info(vdata->dev, "hisi_vibrator open\n");
	} else {
		vibrator_shake = 0;
		hisi_pmic_reg_write(vdata->vibrator_reg_off,VIBRATOR_OFF);
		dev_info(vdata->dev, "hisi_vibrator close\n");
	}
	/* mutex_unlock(&vdata->lock); */

	return;
}

static int hisi_vibrator_get_dr_vout(struct hisi_vibrator_data *vdata)
{
	struct device_node *dn;
	int ret;

	dn = vdata->dev->of_node;
	ret = of_property_read_u32(dn, "vibrator-reg-current", &vdata->vibrator_reg_current);
	if (ret) {
		dev_err(vdata->dev, "get vibrator-reg-current failed\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "vibrator-bit-current", &vdata->vibrator_bit_current);
	if (ret) {
		dev_err(vdata->dev, "get vibrator-bit-current failed\n");
		return ret;
	}

	hisi_pmic_reg_write(vdata->vibrator_reg_current, vdata->vibrator_bit_current);

	return 0;
}

static int hisi_vibrator_get_vout(struct hisi_vibrator_data *vdata)
{
	struct device_node *dn;
	int ret;

	vdata->ldo = devm_regulator_get(vdata->dev, "vibrator-vdd");
	if (IS_ERR(vdata->ldo)){
		dev_err(vdata->dev, "get vdd failed\n");
		return -EPERM;
	}

	dn = vdata->dev->of_node;
	ret = of_property_read_u32(dn, "vibrator_vout_min_voltage", &vdata->min_voltage);
	if (ret) {
		dev_err(vdata->dev, "min_voltage read failed\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "vibrator_vout_max_voltage", &vdata->max_voltage);
	if (ret) {
		dev_err(vdata->dev, "max_voltage read failed\n");
		return ret;
	}

	ret = regulator_set_voltage(vdata->ldo,
			vdata->min_voltage,
			vdata->max_voltage);
	if (ret){
		dev_err(vdata->dev, "vibrator set voltage error\n");
		return ret;
	}

	return 0;
}

static int hisi_vibrator_get_vout_reg(struct hisi_vibrator_data *vdata)
{
	struct device_node *dn;
	int ret = 0;

	dn = vdata->dev->of_node;

	ret = of_property_read_u32(dn, "vibrator-use-dr",
				&vdata->vibrator_use_dr);
	if (ret) {
		dev_info(vdata->dev, "failed to get vibrator-bit-on\n");
		vdata->vibrator_use_dr = HISI_VIBRATOR_NOT_USE_DR;
	}

	ret = of_property_read_u32(dn, "vibrator-reg-on",
				&vdata->vibrator_reg_on);
	if (ret) {
		dev_err(vdata->dev, "failed to get vibrator-reg-on\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "vibrator-reg-off",
				&vdata->vibrator_reg_off);
	if (ret) {
		dev_err(vdata->dev, "failed to get vibrator-reg-off\n");
		return ret;
	}

	ret = of_property_read_u32(dn, "vibrator-bit-on",
				&vdata->vibrator_bit_on);
	if (ret) {
		dev_err(vdata->dev, "failed to get vibrator-bit-on\n");
	}

	return ret;
}

static ssize_t hisi_vibrator_min_timeout_show(struct device *dev,
				     struct device_attribute *attr, char *buf)
{
	u32 val = HISI_VIBRATOR_TIMEOUT_MIN;

	return snprintf_s(buf, PAGE_SIZE,PAGE_SIZE - 1, "%d\n", val);
}

static DEVICE_ATTR(vibrator_min_timeout, 0664, hisi_vibrator_min_timeout_show,
		   NULL);

static struct attribute *hisi_vb_attributes[] = {
	&dev_attr_vibrator_min_timeout.attr,
	NULL
};
static const struct attribute_group hisi_vb_attr_group = {
	.attrs = hisi_vb_attributes,
};

static int hisi_vibrator_register_led_classdev(
			struct hisi_vibrator_data *vdata)
{
	struct led_classdev *cdev = &vdata->cdev;

	cdev->name = "vibrator";
	cdev->flags = LED_CORE_SUSPENDRESUME;
	cdev->brightness_set = hisi_vibrator_ldo_ctrl;
	cdev->default_trigger = "transient";

	return devm_led_classdev_register(vdata->dev, cdev);
}

static const struct of_device_id hisi_vibrator_match[] = {
	{.compatible = "hisilicon,vibrator"},
	{},
};

MODULE_DEVICE_TABLE(of, hisi_vibrator_match);

static int hisi_vibrator_probe(struct platform_device *pdev)
{
	struct hisi_vibrator_data *vdata;
	int ret = 0;

	if (!of_match_node(hisi_vibrator_match, pdev->dev.of_node)) {
		dev_err(&pdev->dev, "dev node is not match. exiting.\n");
		return -ENODEV;
	}

	vdata = devm_kzalloc(&pdev->dev,
		sizeof(struct hisi_vibrator_data),GFP_KERNEL);
	if (vdata == NULL) {
		dev_err(&pdev->dev, "failed to allocate vibrator device\n");
		return -ENOMEM;
	}

	vdata->dev = &pdev->dev;

	ret = hisi_vibrator_get_vout_reg(vdata);
	if (ret)
		return ret;

	if(vdata->vibrator_use_dr) {
		ret = hisi_vibrator_get_dr_vout(vdata);
		if (ret) {
			dev_err(&pdev->dev, "failed to get vib vout\n");
			return ret;
		}
	}else {
		ret = hisi_vibrator_get_vout(vdata);
		if (ret) {
			dev_err(&pdev->dev, "failed to get vib vout\n");
			return ret;
		}
	}
	/* init lock */
	/* mutex_init(&vdata->lock); */

	/* register led classdev, use "transient" as default trigger */
	ret = hisi_vibrator_register_led_classdev(vdata);
	if (ret) {
		dev_err(&pdev->dev, "failed to register led classdev\n");
		return ret;
	}

	ret = sysfs_create_group(&vdata->cdev.dev->kobj, &hisi_vb_attr_group);
	if (ret)
		dev_err(&pdev->dev,"unable create vibrator's min_timeout\n");

	platform_set_drvdata(pdev, vdata);

	dev_info(&pdev->dev, "init ok\n");

	return 0;
}

static int hisi_vibrator_remove(struct platform_device *pdev)
{
	struct hisi_vibrator_data *vdata;

	vdata = dev_get_drvdata(&pdev->dev);
	if (!vdata) {
		pr_err("%s:failed to get drvdata\n", __func__);
		return -ENODEV;
	}

	sysfs_remove_group(&vdata->cdev.dev->kobj, &hisi_vb_attr_group);

	return 0;
}

static struct platform_driver hisi_vibrator_driver = {
	.probe  = hisi_vibrator_probe,
	.remove = hisi_vibrator_remove,
	.driver = {
		.name   = "vibrator",
		.owner  = THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_vibrator_match),
	},
};

static int __init hisi_vibrator_init(void)
{
	return platform_driver_register(&hisi_vibrator_driver);
}

static void __exit hisi_vibrator_exit(void)
{
        platform_driver_unregister(&hisi_vibrator_driver);
}

module_init(hisi_vibrator_init);
module_exit(hisi_vibrator_exit);

MODULE_AUTHOR("Wang Xiaoyin <hw.wangxiaoyin@hisilicon.com>");
MODULE_DESCRIPTION("Hisi vibrator driver");
MODULE_LICENSE("GPL");
