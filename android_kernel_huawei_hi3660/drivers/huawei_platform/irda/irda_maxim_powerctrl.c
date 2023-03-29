#include "irda_driver.h"
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

/*
* CONFIG_USE_CAMERA3_ARCH : the camera module build config
* du to the irda power suplly by camera power chip
*/
#ifdef CONFIG_USE_CAMERA3_ARCH
#define IRDA_EXTERN_POWER
#endif

#ifdef IRDA_EXTERN_POWER
#include <media/huawei/hw_extern_pmic.h>
#endif
#define IRDA_COMPATIBLE_ID		"huawei,irda_maxim"
#define IRDA_POWER_CONTROL_GPIO	"gpio_power_control"
#define IRDA_POWER_TYPE		"irda,power_type"

#define IRDA_BUFF_SIZE			50
#define IRDA_SUCCESS			0
#define IRDA_ERROR			-1
#define IRDA_POWER_VOLTAGE		1800000
#define IRDA_EXTERN_POWER_INDEX	1

#define HWLOG_TAG irda_powercfg

HWLOG_REGIST();

/**
*struct for the value of uart, power, and irda work state
*/
enum irda_work_state {
	/**UNKNOWN when set LDO success but set uart fail*/
	IRDA_STATE_UNKNOWN = -1,
	IRDA_IDLE = 0,
	IRDA_DEFAULT = 1,
};

/*
* default type set to gpio to adapt old dts config
*/
enum irda_power_type {
	IRDA_POWER_TYPE_GPIO = 0,
	IRDA_POWER_TYPE_INTERNAL_LDO,
	IRDA_POWER_TYPE_EXTERNAL_LDO,
	IRDA_POWER_TYPE_OTHER,
	IRDA_POWER_TYPE_UNDEFINE,
};

/**
*struct for mark uart, power
*/
struct irda_power_state {
	int work_state;  /**irda work state*/
	int power_state;  /**irda power state*/
	int uart_state;  /**irda uart pin state*/
};

struct irda_private_data {
	int power_type;
	struct gpio gpio_reset;
	struct irda_power_state power_state;
	struct mutex write_lock;
	struct regulator *ldo_reset;
	struct pinctrl *uart_reset;
};

struct irda_device {
	struct platform_device *pdev;
	struct device *dev;
	struct irda_private_data pdata;
};

extern struct class *irda_class;
#ifdef IRDA_EXTERN_POWER
extern int hw_extern_pmic_config(int index, int voltage, int enable);
#else
int hw_extern_pmic_config(int index, int voltage, int enable)
{
	hwlog_err("the camera power cfg donot define\n");
	return 1;
}
#endif
static int uartpin_set(struct irda_device *irda_dev, int enable)
{
	int ret;
	struct pinctrl_state *pinctrl_set;

	pinctrl_set = pinctrl_lookup_state(irda_dev->pdata.uart_reset,
			enable ? PINCTRL_STATE_DEFAULT : PINCTRL_STATE_IDLE);
	if (pinctrl_set == NULL || IS_ERR(pinctrl_set)) {
		hwlog_err("could not get pin state\n");
		return IRDA_ERROR;
	}

	hwlog_err("set uart %s state\n", enable ? "default" : "idle");

	ret = pinctrl_select_state(irda_dev->pdata.uart_reset, pinctrl_set);
	if (ret < 0) {
		hwlog_err("set  uart state failed\n");
		return IRDA_ERROR;
	}

	return IRDA_SUCCESS;
}

static int irda_get_power_status(struct irda_device *irda_dev)
{
	int ret = 0;
	int power_state = 0;
	int type = irda_dev->pdata.power_type;
	unsigned gpio = 0;

	switch (type) {
	case IRDA_POWER_TYPE_GPIO:
		gpio = irda_dev->pdata.gpio_reset.gpio;
		power_state = gpio_get_value(gpio);
		break;
	case IRDA_POWER_TYPE_INTERNAL_LDO:
		power_state = irda_dev->pdata.power_state.work_state;
		break;
	case IRDA_POWER_TYPE_EXTERNAL_LDO:
/*
		ret = extern_get_power_status(&power_state);
*/
		if (ret)
			hwlog_err("get power status err.\n");
		break;
	case IRDA_POWER_TYPE_OTHER:
		power_state = irda_dev->pdata.power_state.work_state;
		break;
	default:
		hwlog_err("[%s] err type:%d\n", __func__, type);
		ret = IRDA_ERROR;
		break;
	}

	return power_state;
}

static int irda_power_config(struct irda_device *irda_dev, int enable)
{
	int ret = IRDA_ERROR;
	int type = irda_dev->pdata.power_type;
	int power_state;
	unsigned gpio = irda_dev->pdata.gpio_reset.gpio;

	/**if power_state is same to expect, then ignore*/
	if (enable == irda_dev->pdata.power_state.work_state) {
		ret = IRDA_SUCCESS;
		return ret;
	}

	switch (type) {
	case IRDA_POWER_TYPE_GPIO:
		ret = gpio_direction_output(gpio, enable);
		if (0 == ret) {
			ret = IRDA_SUCCESS;
		} else {
			ret = IRDA_ERROR;
		}
		break;
	case IRDA_POWER_TYPE_INTERNAL_LDO:
		power_state = regulator_is_enabled(irda_dev->pdata.ldo_reset);
		if (enable != irda_dev->pdata.power_state.power_state) {
			if (enable && (power_state <= 0)) {
				hwlog_err("regulator_enable\n");
				ret = regulator_enable(irda_dev->pdata.ldo_reset);
			}
			if (!enable && (power_state > 0)) {
				hwlog_err("regulator_disable\n");
				ret = regulator_disable(irda_dev->pdata.ldo_reset);
			}
		}
		if (ret < 0) {
			ret = IRDA_ERROR;
			hwlog_err("set ldo failed %d\n", ret);
			break;
		}

		irda_dev->pdata.power_state.power_state = enable;

		if (enable != irda_dev->pdata.power_state.uart_state) {
			ret = uartpin_set(irda_dev, enable);
		}

		/*if set ldo success, but uart fail, set work state error*/
		if (IRDA_ERROR == ret) {
			irda_dev->pdata.power_state.work_state = IRDA_STATE_UNKNOWN;
			break;
		}
		irda_dev->pdata.power_state.uart_state = enable;

		ret = IRDA_SUCCESS;

		break;
	case IRDA_POWER_TYPE_EXTERNAL_LDO:
		ret = hw_extern_pmic_config(1, IRDA_POWER_VOLTAGE, enable);
		if (!ret)
			ret = IRDA_SUCCESS;
		break;
	case IRDA_POWER_TYPE_OTHER:
		irda_dev->pdata.power_state.power_state = enable;
		hwlog_info("just change the state. enable:%d\n", enable);
		ret = IRDA_SUCCESS;
		break;
	default:
		hwlog_err("err type:%d\n", type);
		ret = IRDA_ERROR;
		break;
	}

	return ret;
}

static ssize_t power_config_get(struct device *dev,
			struct device_attribute *attr, char *buf)
{
	int status = 0;
	struct irda_device *irda_dev = dev_get_drvdata(dev);

	/*
	* return the user setting status to user, and print the real status.
	*/
	irda_dev->pdata.power_state.work_state = irda_get_power_status(irda_dev);
	hwlog_info("true status:%d\n", status);

	return snprintf(buf, IRDA_BUFF_SIZE, "%d\n", irda_dev->pdata.power_state.work_state);
}

static ssize_t power_config_set(struct device *dev,
			struct device_attribute *attr,
			const char *buf, size_t count)
{
	int ret = -1;
	unsigned long state = 0;
	struct irda_device *irda_dev = dev_get_drvdata(dev);

	if (!kstrtol(buf, 10, &state)) {
		mutex_lock(&irda_dev->pdata.write_lock);
		ret = irda_power_config(irda_dev, !!state);
		mutex_unlock(&irda_dev->pdata.write_lock);
		if (IRDA_SUCCESS == ret) {
			irda_dev->pdata.power_state.work_state = !!state;
			hwlog_info("current state:%d\n", !!state);
			ret = count;
		}
	}
	return ret;
}
static DEVICE_ATTR(power_cfg, 0660, power_config_get, power_config_set);

static int irda_power_gpio_init(struct device_node *np,
			struct irda_device *irda_dev)
{
	int gpio;
	int ret;

	gpio = of_get_named_gpio(np, IRDA_POWER_CONTROL_GPIO, 0);
	if (!gpio_is_valid(gpio)) {
		hwlog_err("gpio is not valid\n");
		ret = -EINVAL;
	} else {
		irda_dev->pdata.gpio_reset.gpio = (unsigned int)gpio;
		ret = gpio_request((unsigned int)gpio, "irda_gpio");
	}
	return ret;
}

static int irda_power_internal_ldo_init(struct device *dev,
			struct irda_device *irda_dev)
{
	/*get ldo settings for irda*/
	irda_dev->pdata.ldo_reset = devm_regulator_get(dev, "ldo_power");
	if (IS_ERR(irda_dev->pdata.ldo_reset)) {
		hwlog_err("ldo is not valid\n");
		return IRDA_ERROR;
	}
	irda_dev->pdata.power_state.power_state = IRDA_STATE_UNKNOWN;

	/*get pinctrl settings of uart*/
	irda_dev->pdata.uart_reset = devm_pinctrl_get(dev);
	if (IS_ERR(irda_dev->pdata.uart_reset)) {
		hwlog_err("get pin error\n");
		return IRDA_ERROR;
	}

	return IRDA_SUCCESS;
}

static int irda_power_init(struct device *dev,
			struct irda_device *irda_dev)
{
	int ret = IRDA_SUCCESS;
	int type = irda_dev->pdata.power_type;

	switch (type) {
	case IRDA_POWER_TYPE_GPIO:
		ret = irda_power_gpio_init(dev->of_node, irda_dev);
		break;
	case IRDA_POWER_TYPE_EXTERNAL_LDO:
/*
		ret = irda_external_power_register();
*/
		hwlog_info("no need to init, direct control.\n");
		break;
	case IRDA_POWER_TYPE_INTERNAL_LDO:
		ret = irda_power_internal_ldo_init(dev, irda_dev);
		break;
	case IRDA_POWER_TYPE_OTHER:
		hwlog_info("power control by other. init\n");
		break;
	default:
		hwlog_err("[%s] err type:%d", __func__, type);
		ret = IRDA_ERROR;
		break;
	}

	return ret;
}

static void irda_power_exit(struct irda_device *irda_dev)
{
	int type = irda_dev->pdata.power_type;

	switch (type) {
	case IRDA_POWER_TYPE_GPIO:
		gpio_free(irda_dev->pdata.gpio_reset.gpio);
		break;
	case IRDA_POWER_TYPE_INTERNAL_LDO:
		devm_pinctrl_put(irda_dev->pdata.uart_reset);
		break;
	case IRDA_POWER_TYPE_EXTERNAL_LDO:
		hwlog_info("no need to exit, direct control.\n");
		break;
	case IRDA_POWER_TYPE_OTHER:
		hwlog_info("power control by other. exit\n");
		break;
	default:
		break;
	}
}

static int irda_probe(struct platform_device *pdev)
{
	int ret;
	int power_type;
	struct device_node *np;
	struct irda_device *irda_dev;

	np = pdev->dev.of_node;
	if (np == NULL) {
		hwlog_err("none device\n");
		ret = -ENODEV;
		goto error;
	}

	irda_dev =
	    (struct irda_device *)kzalloc(sizeof(struct irda_device),
					  GFP_KERNEL);
	if (NULL == irda_dev) {
		hwlog_err("Failed to allocate irda_dev\n");
		ret = -ENOMEM;
		goto error;
	}

	ret = of_property_read_u32(np, IRDA_POWER_TYPE, &power_type);
	if (ret) {
		hwlog_warn("Failed to get power type; ret:%d\n", ret);
		/*
		* set default power type gpio control;
		*/
		irda_dev->pdata.power_type = IRDA_POWER_TYPE_GPIO;
	} else
		irda_dev->pdata.power_type = power_type;

	ret = irda_power_init(&(pdev->dev), irda_dev);
	if (ret) {
		hwlog_err("Failed to init irda power.\n");
		goto free_irda_dev;
	}


	irda_dev->dev =
	    device_create(irda_class, NULL, MKDEV(0, 0), NULL, "%s",
			  "irda_maxim");
	if (IS_ERR(irda_dev->dev)) {
		ret = PTR_ERR(irda_dev->dev);
		hwlog_err("Failed to create dev; ret:%d\n", ret);
		goto power_exit;
	}

	ret = device_create_file(irda_dev->dev, &dev_attr_power_cfg);
	if (ret) {
		hwlog_err("Failed to create file; ret:%d\n", ret);
		goto free_dev;
	}

	dev_set_drvdata(irda_dev->dev, irda_dev);
	platform_set_drvdata(pdev, irda_dev);

	/**deault status is IDLE, for all of who use this file will set 1 when boot*/
	irda_dev->pdata.power_state.uart_state = IRDA_STATE_UNKNOWN;
	irda_dev->pdata.power_state.work_state = IRDA_STATE_UNKNOWN;
	mutex_init(&(irda_dev->pdata.write_lock));

	hwlog_info("platform device probe success\n");
	return 0;

free_dev:
	device_destroy(irda_class, irda_dev->dev->devt);
power_exit:
	irda_power_exit(irda_dev);
free_irda_dev:
	kfree(irda_dev);
error:
	return ret;
}

static int irda_remove(struct platform_device *pdev)
{
	struct irda_device *irda_dev = platform_get_drvdata(pdev);

	device_remove_file(irda_dev->dev, &dev_attr_power_cfg);
	device_destroy(irda_class, irda_dev->dev->devt);
	class_destroy(irda_class);
	irda_power_exit(irda_dev);
	kfree(irda_dev);

	return 0;
}

static const struct of_device_id irda_match_table[] = {
		{
			.compatible = IRDA_COMPATIBLE_ID,
			.data = NULL,
		},
		{
		},
};

MODULE_DEVICE_TABLE(of, irda_match_table);

static struct platform_driver irda_driver = {
	.probe = irda_probe,
	.remove = irda_remove,
	.driver = {
		.name = "irda_maxq616",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(irda_match_table),
	},
};

int irda_maxim_power_config_regist(void)
{
	return platform_driver_register(&irda_driver);
}

void irda_maxim_power_config_unregist(void)
{
	platform_driver_unregister(&irda_driver);
}
