#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/interrupt.h>
#include <linux/ioctl.h>
#include <linux/io.h>
#include <linux/irq.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/wakelock.h>
#include <linux/miscdevice.h>
#include <linux/regulator/consumer.h>
#include <linux/workqueue.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <sound/jack.h>
#include <linux/fs.h>
#include <linux/regmap.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/pinctrl/consumer.h>
#include <linux/regulator/consumer.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/machine.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#ifdef CONFIG_SUPERSWITCH_FSC
#include <huawei_platform/usb/superswitch/fsc/core/hw_scp.h>
#endif
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wired_channel_switch.h>
#endif
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif
#include "huawei_platform/audio/usb_audio_power.h"

#define HWLOG_TAG usb_audio_power
HWLOG_REGIST();

#ifdef CONFIG_SUPERSWITCH_FSC
#define USB_DISCONNECT_WAIT_TIME 4000   // DBC station: wait for usb disconnect when accept the AT command.
#endif

enum usb_audio_power_gpio_type {
	USB_AUDIO_POWER_GPIO_SOC           = 0,
	USB_AUDIO_POWER_GPIO_CODEC         = 1,
};

#ifdef CONFIG_SUPERSWITCH_FSC
enum {
	NOT_USING_SUPWRSWITCH              = 0,
	USING_SUPWRSWITCH                  = 1,
};

enum {
	SUPERSWITCH_VOUT_SWITCH_OPEN       = 0,
	SUPERSWITCH_VOUT_SWITCH_CLOSE      = 1,
};
#endif

#ifdef CONFIG_WIRELESS_CHARGER
enum {
	NOT_USING_WIRELESS_CHARGER         = 0,
	USING_WIRELESS_CHARGER             = 1,
};
#endif

#ifdef CONFIG_BOOST_5V
enum {
	NOT_USING_BOOST_5V                 = 0,
	USING_BOOST_5V                     = 1,
};
#endif

struct usb_audio_power_data {
#ifdef CONFIG_SUPERSWITCH_FSC
	int using_superswitch;
#endif
#ifdef CONFIG_WIRELESS_CHARGER
	int using_wireless_charger;
#endif
#ifdef CONFIG_BOOST_5V
	int using_boost_5v;
#endif
	int gpio_chg_vbst_ctrl;
	int gpio_ear_power_en;
	int gpio_type;
	bool pm_ref; /*used to static PM vboost ctrl*/
	bool audio_ref; /*used to static audio vboost ctrl*/
	bool audio_buckboost_enable;

	struct mutex vboost_ctrl_lock;
	struct wake_lock wake_lock;
#ifdef CONFIG_SUPERSWITCH_FSC
	struct workqueue_struct* superswitch_vout_switch_delay_wq;
	struct delayed_work superswitch_vout_switch_delay_work;
#endif
};

static struct usb_audio_power_data *pdata;

/*lint -save -e* */
static inline int usb_audio_power_gpio_get_value(int gpio)
{
	if (pdata->gpio_type == USB_AUDIO_POWER_GPIO_CODEC) {
		return gpio_get_value_cansleep(gpio);
	} else {
		return gpio_get_value(gpio);
	}
}
/*lint -restore*/

/*lint -save -e* */
static inline void usb_audio_power_gpio_set_value(int gpio, int value)
{
	hwlog_info("%s: gpio %d, value %d\n", __func__, gpio, value);
	if (pdata->gpio_type == USB_AUDIO_POWER_GPIO_CODEC) {
		gpio_set_value_cansleep(gpio, value);
	} else {
		gpio_set_value(gpio, value);
	}
}
/*lint -restore*/

int bst_ctrl_enable(bool enable, enum VBOOST_CONTROL_SOURCE_TYPE type)
{
	if (NULL == pdata) {
		hwlog_warn("pdata is NULL!\n");
		return -ENOMEM;
	}

	mutex_lock(&pdata->vboost_ctrl_lock);
	if ((pdata->pm_ref == enable && type == VBOOST_CONTROL_PM) || (pdata->audio_ref == enable && type == VBOOST_CONTROL_AUDIO)) {
		mutex_unlock(&pdata->vboost_ctrl_lock);
		return 0;
	}
	hwlog_info("%s: enable %d, type %d\n", __func__, enable, type);
	if (enable) {
		if (type == VBOOST_CONTROL_PM) {
			pdata->pm_ref = true;
			if (pdata->audio_ref == false) {
				usb_audio_power_gpio_set_value(pdata->gpio_chg_vbst_ctrl, AUDIO_POWER_GPIO_SET);
			}
		} else if (type == VBOOST_CONTROL_AUDIO) {
			pdata->audio_ref = true;
			if (pdata->pm_ref == false) {
				usb_audio_power_gpio_set_value(pdata->gpio_chg_vbst_ctrl, AUDIO_POWER_GPIO_SET);
			}
		}
	} else {
		if (type == VBOOST_CONTROL_PM) {
			pdata->pm_ref = false;
			if (pdata->audio_ref == false) {
				usb_audio_power_gpio_set_value(pdata->gpio_chg_vbst_ctrl, AUDIO_POWER_GPIO_RESET);
			}
		} else if (type == VBOOST_CONTROL_AUDIO) {
			pdata->audio_ref = false;
			if (pdata->pm_ref == false) {
				usb_audio_power_gpio_set_value(pdata->gpio_chg_vbst_ctrl, AUDIO_POWER_GPIO_RESET);
			}
		}
	}
	mutex_unlock(&pdata->vboost_ctrl_lock);
	return 0;
}

static int buckboost_voltage_control(void)
{
#ifdef CONFIG_BOOST_5V
	if (USING_BOOST_5V == pdata->using_boost_5v) {
		boost_5v_enable(true, BOOST_CTRL_AUDIO);
	}
#endif
	bst_ctrl_enable(true, VBOOST_CONTROL_AUDIO);
	usb_audio_power_gpio_set_value(pdata->gpio_ear_power_en, AUDIO_POWER_GPIO_SET);

	pd_dpm_vbus_ctrl(CHARGER_TYPE_NONE);
	pdata->audio_buckboost_enable = true;
	hwlog_info("%s", __func__);
	return 0;
}

int usb_audio_power_buckboost()
{
	if (NULL == pdata) {
		hwlog_warn("pdata is NULL!\n");
		return -ENOMEM;
	}

	if (pdata->audio_buckboost_enable == false && pd_dpm_get_pd_source_vbus()) {
		buckboost_voltage_control();
	}

	return 0;
}

static int usb_audio_power_buckboost_no_headset(void)
{
	if (NULL == pdata) {
		hwlog_warn("pdata is NULL!\n");
		return -ENOMEM;
	}

	if (pdata->audio_buckboost_enable == false) {
		buckboost_voltage_control();
	}

	return 0;
}


int usb_audio_power_scharger()
{
	if (NULL == pdata) {
		hwlog_warn("pdata is NULL!\n");
		return -ENOMEM;
	}

	//pd_dpm_vbus_ctrl(PLEASE_PROVIDE_POWER);
	if (pdata->audio_buckboost_enable == true) {
		bst_ctrl_enable(false, VBOOST_CONTROL_AUDIO);
		usb_audio_power_gpio_set_value(pdata->gpio_ear_power_en, AUDIO_POWER_GPIO_RESET);
#ifdef	CONFIG_BOOST_5V
		if (USING_BOOST_5V == pdata->using_boost_5v) {
			boost_5v_enable(false, BOOST_CTRL_AUDIO);
		}
#endif
		pdata->audio_buckboost_enable = false;
		hwlog_info("%s", __func__);
	}
	return 0;
}

#ifdef CONFIG_SUPERSWITCH_FSC
static void superswitch_vout_switch_work(struct work_struct* work)
{
	usb_audio_power_buckboost();
	/* close the vout switch, to avoid the voltage drop when pass through super switch.*/
	FUSB3601_vout_enable(SUPERSWITCH_VOUT_SWITCH_CLOSE);
}
#endif

/**
 * usb_audio_power_ioctl - ioctl interface for userspeace
 *
 * @file: file description
 * @cmd: control commond
 * @arg: arguments
 *
 * userspeace can get charge status and force control
 * charge status.
 **/
/*lint -save -e* */
static long usb_audio_power_ioctl(struct file *file, unsigned int cmd,
							   unsigned long arg)
{
	int ret = 0;

	switch (cmd) {
		case IOCTL_USB_AUDIO_POWER_BUCKBOOST_NO_HEADSET_CMD:
#ifdef CONFIG_WIRELESS_CHARGER
			if (USING_WIRELESS_CHARGER == pdata->using_wireless_charger) {
				wired_chsw_set_wired_channel(WIRED_CHANNEL_CUTOFF);
			}
#endif
#ifdef CONFIG_SUPERSWITCH_FSC
			if (USING_SUPWRSWITCH == pdata->using_superswitch) {
				queue_delayed_work(pdata->superswitch_vout_switch_delay_wq,
							   &pdata->superswitch_vout_switch_delay_work,
							   msecs_to_jiffies(USB_DISCONNECT_WAIT_TIME));
				ret = 0;
			} else {
#endif
				ret = usb_audio_power_buckboost_no_headset();
#ifdef CONFIG_SUPERSWITCH_FSC
			}
#endif
			break;
		case IOCTL_USB_AUDIO_POWER_SCHARGER_CMD:
#ifdef CONFIG_WIRELESS_CHARGER
			if (USING_WIRELESS_CHARGER == pdata->using_wireless_charger) {
				wired_chsw_set_wired_channel(WIRED_CHANNEL_RESTORE);
			}
#endif
			ret = usb_audio_power_scharger();
			break;
		default:
			hwlog_err("unsupport cmd\n");
			ret = -EINVAL;
			break;
	}

	return (long)ret;
}

static const struct of_device_id usb_audio_power_of_match[] = {
	{
		.compatible = "huawei,usb_audio_power",
	},
	{ },
};
/*lint -save -e* */
MODULE_DEVICE_TABLE(of, usb_audio_power_of_match);
/*lint -restore*/

/* load dts config for board difference */
static void load_usb_audio_power_config(struct device_node *node)
{
	int temp = 0;
	/*lint -save -e* */
	if (!of_property_read_u32(node, "gpio_type", &temp)) {
	/*lint -restore*/
		pdata->gpio_type = temp;
	} else {
		pdata->gpio_type = USB_AUDIO_POWER_GPIO_SOC;
	}
#ifdef CONFIG_SUPERSWITCH_FSC
	/*lint -save -e* */
	if (!of_property_read_u32(node, "using_superswitch", &temp)) {
	/*lint -restore*/
		pdata->using_superswitch = temp;
	} else {
		pdata->using_superswitch = NOT_USING_SUPWRSWITCH;
	}
#endif
#ifdef CONFIG_WIRELESS_CHARGER
	/*lint -save -e* */
	if (!of_property_read_u32(node, "using_wireless_charger", &temp)) {
	/*lint -restore*/
		pdata->using_wireless_charger = temp;
	} else {
		pdata->using_wireless_charger = NOT_USING_WIRELESS_CHARGER;
	}
#endif
#ifdef CONFIG_BOOST_5V
	/*lint -save -e* */
	if (!of_property_read_u32(node, "using_boost_5v", &temp)) {
	/*lint -restore*/
		pdata->using_boost_5v = temp;
	} else {
		pdata->using_boost_5v = NOT_USING_BOOST_5V;
	}
#endif
}
static const struct file_operations usb_audio_power_fops = {
	.owner            = THIS_MODULE,
	.open            = simple_open,
	.unlocked_ioctl = usb_audio_power_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl    = usb_audio_power_ioctl,
#endif
};

static struct miscdevice usb_audio_power_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "usb_audio_power",
	.fops =     &usb_audio_power_fops,
};

/*lint -save -e* */
static int usb_audio_power_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	struct device_node *node =  dev->of_node;
	int ret = 0;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (NULL == pdata) {
		hwlog_err("cannot allocate usb_audio_power data\n");
		return -ENOMEM;
	}
	pdata->pm_ref = false;
	pdata->audio_ref = false;
	pdata->audio_buckboost_enable = false;
	mutex_init(&pdata->vboost_ctrl_lock);

	/* get gpio_chg_vbst_ctrl gpio */
	pdata->gpio_chg_vbst_ctrl =  of_get_named_gpio(node, "gpio_chg_vbst_ctrl", 0);
	if (pdata->gpio_chg_vbst_ctrl < 0) {
		hwlog_err("gpio_chg_vbst_ctrl is unvalid!\n");
		/*lint -save -e* */
		ret = -ENOENT;
		/*lint -restore*/
		goto err_out;
	}

	if (!gpio_is_valid(pdata->gpio_chg_vbst_ctrl)) {
		hwlog_err("gpio is unvalid!\n");
		/*lint -save -e* */
		ret = -ENOENT;
		/*lint -restore*/
		goto gpio_chg_vbst_ctrl_err;
	}

	/* applay for mic->gnd gpio */
	ret = gpio_request(pdata->gpio_chg_vbst_ctrl, "gpio_chg_vbst_ctrl");
	if (ret) {
		hwlog_err("error request GPIO for vbst ctrl fail %d\n", ret);
		goto gpio_chg_vbst_ctrl_err;
	}
	gpio_direction_output(pdata->gpio_chg_vbst_ctrl, 0);

	/* get gpio_ear_power_en gpio */
	pdata->gpio_ear_power_en =  of_get_named_gpio(node, "gpio_ear_power_en", 0);
	if (pdata->gpio_ear_power_en < 0) {
		hwlog_err("gpio_ear_power_en is unvalid!\n");
		/*lint -save -e* */
		ret = -ENOENT;
		/*lint -restore*/
		goto gpio_chg_vbst_ctrl_err;
	}

	if (!gpio_is_valid(pdata->gpio_ear_power_en)) {
		hwlog_err("gpio is unvalid!\n");
		/*lint -save -e* */
		ret = -ENOENT;
		/*lint -restore*/
		goto gpio_ear_power_en_err;
	}

	/* applay for mic->gnd gpio */
	ret = gpio_request(pdata->gpio_ear_power_en, "gpio_ear_power_en");
	if (ret) {
		hwlog_err("error request GPIO for mic_gnd fail %d\n", ret);
		goto gpio_ear_power_en_err;
	}
	gpio_direction_output(pdata->gpio_ear_power_en, 0);
#ifdef CONFIG_SUPERSWITCH_FSC
	pdata->superswitch_vout_switch_delay_wq = create_singlethread_workqueue("superswitch_vout_switch_delay_wq");
	if (!(pdata->superswitch_vout_switch_delay_wq)) {
		hwlog_err("%s : vout switch create failed\n", __func__);
		/*lint -save -e* */
		ret = -ENOMEM;
		/*lint -restore*/
		goto gpio_ear_power_en_err;
	}
	INIT_DELAYED_WORK(&pdata->superswitch_vout_switch_delay_work, superswitch_vout_switch_work);
#endif
	load_usb_audio_power_config(node);

	ret = misc_register(&usb_audio_power_miscdev);
	if (0 != ret) {
		hwlog_err("%s: can't register usb audio power miscdev, ret:%d.\n", __func__, ret);
#ifdef CONFIG_SUPERSWITCH_FSC
		goto superswitch_vout_switch_delay_wq_err;
#else
		goto gpio_ear_power_en_err;
#endif
	}

	hwlog_info("usb_audio_power probe success!\n");

	return 0;
#ifdef CONFIG_SUPERSWITCH_FSC
superswitch_vout_switch_delay_wq_err:
	if (pdata->superswitch_vout_switch_delay_wq) {
		cancel_delayed_work(&pdata->superswitch_vout_switch_delay_work);
		flush_workqueue(pdata->superswitch_vout_switch_delay_wq);
		destroy_workqueue(pdata->superswitch_vout_switch_delay_wq);
	}
#endif
gpio_ear_power_en_err:
	gpio_free(pdata->gpio_ear_power_en);
gpio_chg_vbst_ctrl_err:
	gpio_free(pdata->gpio_chg_vbst_ctrl);
err_out:
	devm_kfree(dev, pdata);
	pdata = NULL;

	return ret;
}
/*lint -restore*/

static int usb_audio_power_remove(struct platform_device *pdev)
{
	if (pdata) {
		if(pdata->gpio_chg_vbst_ctrl > 0) {
			gpio_free(pdata->gpio_chg_vbst_ctrl);
		}
		if(pdata->gpio_ear_power_en > 0) {
			gpio_free(pdata->gpio_ear_power_en);
		}
#ifdef CONFIG_SUPERSWITCH_FSC
		if (pdata->superswitch_vout_switch_delay_wq) {
			cancel_delayed_work(&pdata->superswitch_vout_switch_delay_work);
			flush_workqueue(pdata->superswitch_vout_switch_delay_wq);
			destroy_workqueue(pdata->superswitch_vout_switch_delay_wq);
		}
#endif
		devm_kfree(&pdev->dev, pdata);
		pdata = NULL;
	}

	misc_deregister(&usb_audio_power_miscdev);
	hwlog_info("%s: exit\n", __func__);

	return 0;
}

static struct platform_driver usb_audio_power_driver = {
	.driver = {
		.name   = "usb_audio_power",
		.owner  = THIS_MODULE,
		.of_match_table = usb_audio_power_of_match,
	},
	.probe  = usb_audio_power_probe,
	.remove = usb_audio_power_remove,
};

static int __init usb_audio_power_init(void)
{
	return platform_driver_register(&usb_audio_power_driver);
}

static void __exit usb_audio_power_exit(void)
{
	platform_driver_unregister(&usb_audio_power_driver);
}

/*lint -save -e* */
fs_initcall(usb_audio_power_init);
module_exit(usb_audio_power_exit);
/*lint -restore*/

MODULE_DESCRIPTION("usb audio power control driver");
MODULE_LICENSE("GPL");
