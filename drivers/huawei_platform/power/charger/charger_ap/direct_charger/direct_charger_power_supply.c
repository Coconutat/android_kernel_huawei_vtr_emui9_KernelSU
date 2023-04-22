#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/direct_charger_power_supply.h>
#include <huawei_platform/power/direct_charger.h>
#include <huawei_platform/power/huawei_charger.h>
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif
#ifdef CONFIG_USB_AUDIO_POWER
#include <huawei_platform/audio/usb_audio_power.h>
#endif

#define HWLOG_TAG direct_charge_ps
HWLOG_REGIST();

static struct scp_power_supply_ops* g_scp_ps_ops;
static int boost_5v_support_scp_power = 0;
static int huawei_charger_support_scp_power = 0;
static int is_need_bst_ctrl = 0;
static int bst_ctrl = 0;
static int bst_ctrl_use_common_gpio = 0;


#ifdef CONFIG_SYSFS
static ssize_t direct_charge_ps_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	return NULL;
}

static ssize_t direct_charge_ps_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	return count;
}

static DEVICE_ATTR(direct_charge_ps_enable, S_IWUSR | S_IRUGO, direct_charge_ps_sysfs_show, direct_charge_ps_sysfs_store);

static struct attribute *direct_charge_ps_attributes[] = {
	&dev_attr_direct_charge_ps_enable.attr,
	NULL,
};

static const struct attribute_group direct_charge_ps_attr_group = {
    .attrs = direct_charge_ps_attributes,
};

static int direct_charge_ps_sysfs_create_group(struct platform_device *pdev)
{
	return sysfs_create_group(&pdev->dev.kobj, &direct_charge_ps_attr_group);
}

static void direct_charge_ps_sysfs_remove_group(struct platform_device *pdev)
{
	sysfs_remove_group(&pdev->dev.kobj, &direct_charge_ps_attr_group);
}
#else
static int direct_charge_ps_sysfs_create_group(struct platform_device *pdev)
{
	return 0;
}

static void direct_charge_ps_sysfs_remove_group(struct platform_device *pdev)
{
}
#endif

int direct_charge_set_bst_ctrl(int enable)
{
	int ret = 0;
	if (is_need_bst_ctrl)
	{
		if (!bst_ctrl_use_common_gpio)
		{
			ret |= gpio_direction_output(bst_ctrl, enable);
		}else
		{
#ifdef CONFIG_USB_AUDIO_POWER
			ret |= bst_ctrl_enable(enable, VBOOST_CONTROL_PM);
#endif
		}
	}

	return ret;
}

static int set_scp_power_enable_by_5vboost(int enable)
{
	int ret = 0;
	hwlog_info("%s ++\n", __func__);

	if(boost_5v_support_scp_power)
	{
		hwlog_info("%s boost_5v_support_scp_power = %d, %d, %d\n", __func__, enable, is_need_bst_ctrl, bst_ctrl_use_common_gpio);
#ifdef CONFIG_BOOST_5V
		ret |= boost_5v_enable(enable, BOOST_CTRL_DC);
#endif

		ret |= direct_charge_set_bst_ctrl(enable);

		if (ret)
			ret = -1;
		else
			ret = 0;
	}
	hwlog_info("%s --\n", __func__);
	return ret;
}

static int set_scp_power_enable_by_hwcharger(int enable)
{
	int ret = 0;
	hwlog_info("%s ++\n", __func__);

	if (huawei_charger_support_scp_power)
	{
#ifdef CONFIG_HUAWEI_CHARGER_AP
		hwlog_info("%s huawei_charger_support_scp_power = %d\n", __func__, enable);
		ret |= charge_otg_mode_enable(enable, OTG_CTRL_DC);
#endif
	}

	hwlog_info("%s --\n", __func__);
	return ret;
}

static int set_scp_power_enable_dummy(int enable)
{
	return 0;
}

static struct scp_power_supply_ops scp_ps_dummy_ops = {
	.scp_power_enable = set_scp_power_enable_dummy,
};


static struct scp_power_supply_ops scp_ps_5vboost_ops = {
	.scp_power_enable = set_scp_power_enable_by_5vboost,
};
static struct scp_power_supply_ops scp_ps_hwcharger_ops = {
	.scp_power_enable = set_scp_power_enable_by_hwcharger,
};

int direct_charge_ps_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct device_node *np = (&pdev->dev)->of_node;

	if (of_property_read_u32(np, "boost_5v_support_scp_power", &boost_5v_support_scp_power)) {
		hwlog_info("%s unsupport_scp_power(boost_5v)\n", __func__);
		boost_5v_support_scp_power = 0;
	}
	hwlog_info("boost_5v_support_scp_power = %d\n", boost_5v_support_scp_power);

	if (of_property_read_u32(np, "huawei_charger_support_scp_power", &huawei_charger_support_scp_power)) {
		hwlog_info("%s unsupport_scp_power(huawei_charger)\n", __func__);
		huawei_charger_support_scp_power = 0;
	}
	hwlog_info("huawei_charger_support_scp_power = %d\n", huawei_charger_support_scp_power);

	if (of_property_read_u32(np, "is_need_bst_ctrl", &is_need_bst_ctrl)) {
		hwlog_info("%s unsupport is_need_bst_ctrl\n", __func__);
		is_need_bst_ctrl = 0;
	}
	hwlog_info("is_need_bst_ctrl = %d\n", is_need_bst_ctrl);

	if (of_property_read_u32(np, "bst_ctrl_use_common_gpio", &bst_ctrl_use_common_gpio)) {
		hwlog_info("%s unsupport bst_ctrl_use_common_gpio\n", __func__);
		bst_ctrl_use_common_gpio = 0;
	}
	hwlog_info("bst_ctrl_use_common_gpio = %d\n", bst_ctrl_use_common_gpio);

	if ((is_need_bst_ctrl) && (!bst_ctrl_use_common_gpio))
	{
		bst_ctrl = of_get_named_gpio(np, "bst_ctrl", 0);
		hwlog_info("bst_ctrl = %d\n", bst_ctrl);
		if (!gpio_is_valid(bst_ctrl))
		{
			hwlog_err("%s: get bst_ctrl fail\n", __func__);
			return -1;
		}

		ret = gpio_request(bst_ctrl,"bst_ctrl");
		if (ret)
		{
			hwlog_err("could not request bst_ctrl\n");
			gpio_free(bst_ctrl);
			return -1;
		}
	}

        scp_power_supply_ops_register(&scp_ps_dummy_ops);

	if (boost_5v_support_scp_power)
	{
		ret = scp_power_supply_ops_register(&scp_ps_5vboost_ops);
		if (ret)
		{
			hwlog_err("register scp power ops failed!\n");
			goto dc_ps_fail;
		}
		else
		{
			hwlog_info(" scp power ops register success!\n");
		}
	}

	if (huawei_charger_support_scp_power)
	{
		ret = scp_power_supply_ops_register(&scp_ps_hwcharger_ops);
		if (ret)
		{
			hwlog_err("register scp power ops failed!\n");
			goto dc_ps_fail;
		}
		else
		{
			hwlog_info(" scp power ops register success!\n");
		}
	}

	ret = direct_charge_ps_sysfs_create_group(pdev);
	if (ret) {
		hwlog_err("can't create direct_charge_ps sysfs entries\n");
		goto dc_ps_fail;
	}

	hwlog_info("direct_charge_ps probe ok.\n");
	return 0;

dc_ps_fail:
	if((is_need_bst_ctrl) && (!bst_ctrl_use_common_gpio))
	{
		gpio_free(bst_ctrl);
	}

	return -1;
}
static int direct_charge_ps_remove(struct platform_device *pdev)
{
	direct_charge_ps_sysfs_remove_group(pdev);
	hwlog_info("%s --\n", __func__);
	return 0;
}
static struct of_device_id direct_charge_ps_match_table[] = {
	{
	 .compatible = "huawei,direct_charge_ps",
	 .data = NULL,
	},
	{ },
};

static struct platform_driver direct_charge_ps_driver = {
	.probe = direct_charge_ps_probe,
	.remove = direct_charge_ps_remove,
	.driver = {
		.name = "huawei,direct_charge_ps",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(direct_charge_ps_match_table),
	},
};
static int __init direct_charge_ps_init(void)
{
	hwlog_info("direct_charge_ps init ok.\n");
	return platform_driver_register(&direct_charge_ps_driver);
}

static void __exit direct_charge_ps_exit(void)
{
	platform_driver_unregister(&direct_charge_ps_driver);
}

device_initcall_sync(direct_charge_ps_init);
module_exit(direct_charge_ps_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("direct_charge_ps module driver");
MODULE_AUTHOR("HUAWEI Inc");
