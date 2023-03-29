				/*
* tfa98xx-mono.c -- ALSA SoC Mono TFA98XX driver
*
* Copyright 2013-2014 NXP Integrated Products
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/
/*lint -e528 -e529 -e578 -e629 -e533 -e613 -e10*/
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/of.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif
#include <huawei_platform/log/hw_log.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/regmap.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/delay.h>

#define SUPPORT_DEVICE_TREE
#ifdef SUPPORT_DEVICE_TREE
#include <linux/regulator/consumer.h>
#endif

#include "huawei_platform/audio/tfa98xx.h"
/*lint -e30 -e715 -e785 -e826 -e845 -e846 -e838 -e528 -e753*/
/*lint -e64 -e529 -e530 -e1058 -e1564 -e774 -e550 -e438 -e142 -e50*/

#define HWLOG_TAG tfa98xx
HWLOG_REGIST();


static struct tfa98xx_ioctl_ops *tfa_io_ops;

//static struct list_head tfa98xx_list;
LIST_HEAD(tfa98xx_list);


struct tfa98xx_priv * tfa98xx_priv_data = NULL;
enum rcv_switch_status {
	RCV_SMARTPA_CONNECTED = 0,
	RCV_CODEC_CONNECTED,
	RCV_DISCONNECTED,
};


static inline void rcv_switch_gpio_set_value(enum rcv_switch_status rcv_status)
{

	switch (rcv_status) {
		case RCV_SMARTPA_CONNECTED:
			gpio_set_value((unsigned int)tfa98xx_priv_data->rcv_en2_gpio, 1);
			gpio_set_value((unsigned int)tfa98xx_priv_data->rcv_en1_gpio, 0);
			break;

		case RCV_CODEC_CONNECTED:
			gpio_set_value((unsigned int)tfa98xx_priv_data->rcv_en2_gpio, 0);
			gpio_set_value((unsigned int)tfa98xx_priv_data->rcv_en1_gpio, 1);
			break;

		case RCV_DISCONNECTED:
			gpio_set_value((unsigned int)tfa98xx_priv_data->rcv_en1_gpio, 0);
			gpio_set_value((unsigned int)tfa98xx_priv_data->rcv_en2_gpio, 0);
			break;

		default:
			break;
		}

}

//ioctl ops regist function
int tfa98xx_ioctl_isregist(void)
{
	return tfa_io_ops == NULL;
}

int tfa98xx_ioctl_regist(struct tfa98xx_ioctl_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		tfa_io_ops = ops;
	} else {
		hwlog_err("tfain ioctrl interface ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}


//tfa98xx list_head operation function
void tfa98xx_list_add(struct tfa98xx_priv *tfa98xx)
{
	list_add(&tfa98xx->list, &tfa98xx_list);
}

void tfa98xx_list_del(struct tfa98xx_priv *tfa98xx)
{
	if (tfa98xx->regmap) {
		regmap_exit(tfa98xx->regmap);
	}

	list_del(&tfa98xx->list);
	kfree(tfa98xx);
}

/*lint -save -e* */
void tfa98xx_list_del_all(void)
{
	struct list_head *pos = NULL;
	struct tfa98xx_priv *p = NULL;
	struct list_head *head = get_tfa98xx_list_head();

	list_for_each(pos, head) {
		p = list_entry(pos, struct tfa98xx_priv, list);
		tfa98xx_list_del(p);
	}
}
/*lint -restore*/

/*lint -save -e* */
struct tfa98xx_priv *find_tfa98xx_by_type(struct list_head *tfa98xx, unsigned int type)
{
	struct list_head *pos = NULL;
	struct tfa98xx_priv *p = NULL;

	list_for_each(pos, tfa98xx) {
		p = list_entry(pos, struct tfa98xx_priv, list);
		if (type == p->type) {
			break;
		} else {
			p = NULL;
		}
	}

	return p;
}
/*lint -restore*/

struct tfa98xx_priv *find_tfa98xx_by_dev(struct device *dev)
{
	int ret;
	unsigned int type = 0;
	const char *type_dts_name = "smartpa_type";


	ret = of_property_read_u32(dev->of_node, type_dts_name, &type);
	if (ret) {
		hwlog_info("%s: get smartPA type from dts failed!!!\n", __func__);
		return NULL;
	}

	return find_tfa98xx_by_type(&tfa98xx_list, type);
}

/*lint -save -e* */
int get_tfa98xx_num(void)
{
	int i = 0;
	struct list_head *pos = NULL;

	list_for_each(pos, &tfa98xx_list) {
		i++;
	}

	return i;
}
/*lint -restore*/

struct list_head *get_tfa98xx_list_head(void)
{
	return &tfa98xx_list;
}

/*lint -save -e* */
static int tfa98xx_open(struct inode *inode, struct file *filp)
{
    hwlog_err("%s: get smartPA type from dts failed!!!\n", __func__);
	if (NULL == tfa_io_ops || NULL == tfa_io_ops->tfa98xx_open) {
		return -1;
	}
	return tfa_io_ops->tfa98xx_open(inode, filp);
}
/*lint -restore*/

/*lint -save -e* */
static int tfa98xx_release(struct inode *inode, struct file *filp)
{
	if (NULL == tfa_io_ops || NULL == tfa_io_ops->tfa98xx_release) {
		return -1;
	}
	return tfa_io_ops->tfa98xx_release(inode, filp);
}
/*lint -restore*/

/*lint -save -e* */
static int tfa98xx_do_ioctl(struct file *file, unsigned int cmd, void __user *p, int compat_mode)
{
	int ret = 0;
	unsigned int value = 0;
	//struct tfa98xx_reg_ops reg_val;
	unsigned int __user *pUser = (unsigned int __user *) p;
	struct list_head *tfa98xx = NULL;
	hwlog_info("%s: enter, cmd:%x, tfa98xx_num:%d\n", __func__, cmd, get_tfa98xx_num());

	if (NULL == file) {
		return -EFAULT;
	}

	tfa98xx = (struct list_head *)file->private_data;

	switch (cmd) {
		case TFA98XX_GET_VERSION:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_get_version);
			ret = tfa_io_ops->tfa98xx_get_version(tfa98xx, TFA98XX_L, &value);
			ret |= put_user(value, pUser);
			break;

		case TFA98XX_R_GET_VERSION:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_get_version);
			ret = tfa_io_ops->tfa98xx_get_version(tfa98xx, TFA98XX_R, &value);
			ret |= put_user(value, pUser);
			break;

		case TFA98XX_POWER_ON:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_digital_mute);
			if (tfa98xx_priv_data -> rcv_switch_support == true) {
				rcv_switch_gpio_set_value(RCV_SMARTPA_CONNECTED);
			}
			ret = tfa_io_ops->tfa98xx_digital_mute(tfa98xx, MUTE_ON);
			break;

		case TFA98XX_POWER_OFF:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_digital_mute);
			ret = tfa_io_ops->tfa98xx_digital_mute(tfa98xx, MUTE_OFF);
			if (tfa98xx_priv_data -> rcv_switch_support == true) {
				msleep(20);
				rcv_switch_gpio_set_value(RCV_DISCONNECTED);
			}
			break;

		case TFA98XX_POWER_SPK_ON:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_single_digital_mute);
			ret = tfa_io_ops->tfa98xx_single_digital_mute(tfa98xx, TFA98XX_L, MUTE_ON);
			break;

		case TFA98XX_POWER_SPK_OFF:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_single_digital_mute);
			ret = tfa_io_ops->tfa98xx_single_digital_mute(tfa98xx, TFA98XX_L, MUTE_OFF);
			break;

		case TFA98XX_POWER_REC_ON:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_single_digital_mute);
			if (tfa98xx_priv_data -> rcv_switch_support == true) {
				rcv_switch_gpio_set_value(RCV_CODEC_CONNECTED);
			} else {
				ret = tfa_io_ops->tfa98xx_single_digital_mute(tfa98xx, TFA98XX_R, MUTE_ON);
			}
			break;

		case TFA98XX_POWER_REC_OFF:
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_single_digital_mute);
			ret = tfa_io_ops->tfa98xx_single_digital_mute(tfa98xx, TFA98XX_R, MUTE_OFF);
			if (tfa98xx_priv_data -> rcv_switch_support == true) {
				rcv_switch_gpio_set_value(RCV_DISCONNECTED);
			}
			break;

		case TFA98XX_SET_PARAM:
			if(NULL == pUser)
			{
				return -EFAULT;
			}
			CHECK_IOCTL_OPS(tfa_io_ops, tfa98xx_set_param);
			ret = tfa_io_ops->tfa98xx_set_param(tfa98xx, pUser);
			break;

		default:
			hwlog_err("%s: cmd input is not support\n", __func__);
			return -EFAULT;
	}
#ifdef CONFIG_HUAWEI_DSM
	if (ret) {
		audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR, "%s: ioctl error %d\n", __func__, ret);
	}
#endif
	return ret;
}
/*lint -restore*/

/*lint -save -e* */
static long tfa98xx_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	return (long)tfa98xx_do_ioctl(file, command, (void __user *)arg, 0);
}
/*lint -restore*/

#ifdef CONFIG_COMPAT
static long tfa98xx_ioctl_compat(struct file *file, unsigned int command, unsigned long arg)
{
	return tfa98xx_do_ioctl(file, command, compat_ptr((unsigned int)arg), 1);
}
#else
#define tfa98xx_ioctl_compat NULL
#endif


static const struct file_operations tfa98xx_ctrl_fops = {
	.owner            = THIS_MODULE,
	.open            = tfa98xx_open,
	.release        = tfa98xx_release,
	.unlocked_ioctl = tfa98xx_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl    = tfa98xx_ioctl_compat,
#endif
};

static struct miscdevice tfa98xx_ctrl_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "nxp_smartpa_dev",
	.fops =     &tfa98xx_ctrl_fops,
};

/*lint -save -e* */
static int tfa98xx_ioctl_probe(struct platform_device *pdev)
{
	int ret = FAILED;
	int val = 0;
	const char *rcv_switch_str = "rcv_switch_support";
	const char *rcv_en1_str = "gpio_rcv_en1";
	const char *rcv_en2_str = "gpio_rcv_en2";
	struct device *dev = &pdev->dev;

	tfa98xx_priv_data = kzalloc(sizeof(struct tfa98xx_priv), GFP_KERNEL);
	if(NULL == tfa98xx_priv_data){
		hwlog_err("%s: tfa98xx kzalloc tfa98xx_priv failed!!!\n", __func__);
		goto err_exit;
	}

	ret = of_property_read_u32(dev->of_node, rcv_switch_str, &val);
	if (ret !=0) {
		hwlog_err("%s: can't get rcv_switch_support from dts, set defaut false value!!!\n", __func__);
		tfa98xx_priv_data->rcv_switch_support = false;
	} else {
		if (val) {
			tfa98xx_priv_data->rcv_switch_support = true;
			hwlog_info("%s: rcv_switch_support is true!\n", __func__);
		} else {
			tfa98xx_priv_data->rcv_switch_support = false;
			hwlog_info("%s: rcv_switch_support is false!\n", __func__);
		}
	}

	if (tfa98xx_priv_data->rcv_switch_support == true)  {
		/* get receiver switch gpio */
		tfa98xx_priv_data->rcv_en1_gpio = of_get_named_gpio(dev->of_node, rcv_en1_str, 0);
		if (tfa98xx_priv_data->rcv_en1_gpio < 0) {
			hwlog_err("rcv_en1_gpio is invalid!\n");
			ret = FAILED;
			goto err_exit;
		}
		ret = gpio_request((unsigned int)tfa98xx_priv_data->rcv_en1_gpio, "rcv_switch_gpio_en1");
		if (ret != 0) {
			hwlog_err("error request GPIO for rcv_en1_gpio fail %d\n", ret);
			goto err_exit;
		}

		tfa98xx_priv_data->rcv_en2_gpio = of_get_named_gpio(dev->of_node, rcv_en2_str, 0);
		if (tfa98xx_priv_data->rcv_en2_gpio < 0) {
			hwlog_err("rcv_en2_gpio is invalid!\n");
			ret = FAILED;
			goto err_rcv_en1_gpio;
		}
		ret = gpio_request((unsigned int)tfa98xx_priv_data->rcv_en2_gpio, "rcv_switch_gpio_en2");
		if (ret != 0) {
			hwlog_err("error request GPIO for rcv_en2_gpio fail %d\n", ret);
			goto err_rcv_en1_gpio;
		}

		rcv_switch_gpio_set_value(RCV_DISCONNECTED);
	}

	ret = misc_register(&tfa98xx_ctrl_miscdev);
	if (0 != ret) {
		hwlog_err("%s: can't register tfa98xx miascdev, ret:%d.\n", __func__, ret);
		goto err_rcv_en2_gpio;
	}

	return 0;

err_rcv_en2_gpio:
	if (tfa98xx_priv_data->rcv_switch_support == true) {
		gpio_free ((unsigned int)tfa98xx_priv_data->rcv_en2_gpio);
	}
err_rcv_en1_gpio:
	if (tfa98xx_priv_data->rcv_switch_support == true) {
		gpio_free ((unsigned int)tfa98xx_priv_data->rcv_en1_gpio);
	}
err_exit:
	if (ret <0) {
		if (tfa98xx_priv_data) {
			kfree(tfa98xx_priv_data);
			tfa98xx_priv_data = NULL;
		}
	}
    return ret;
}
/*lint -restore*/

/*lint -save -e* */
static int tfa98xx_ioctl_remove(struct platform_device *pdev)
{
	int ret = 0;

	if (tfa98xx_priv_data) {
		if (tfa98xx_priv_data->rcv_switch_support == true) {
			gpio_free ((unsigned int)tfa98xx_priv_data->rcv_en2_gpio);
		}
		if (tfa98xx_priv_data->rcv_switch_support == true) {
			gpio_free ((unsigned int)tfa98xx_priv_data->rcv_en1_gpio);
		}
		kfree(tfa98xx_priv_data);
		tfa98xx_priv_data = NULL;
	}

	 misc_deregister(&tfa98xx_ctrl_miscdev);

	return ret;
}
/*lint -restore*/

static const struct of_device_id tfa98xx_match[] = {
	{ .compatible = "huawei,tfa98xx_ioctl", },
	{},
};
/*lint -save -e* */
MODULE_DEVICE_TABLE(of, tfa98xx_match);
/*lint -restore*/

static struct platform_driver tfa98xx_driver = {
	.driver = {
		.name = "tfa98xx_ioctl",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(tfa98xx_match),
	},
	.probe  = tfa98xx_ioctl_probe,
	.remove = tfa98xx_ioctl_remove,
};

/*lint -save -e* */
static int __init tfa98xx_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&tfa98xx_driver);
	if (ret) {
		hwlog_err("%s: platform_driver_register(tfa98xx_driver) failed, ret:%d.\n", __func__, ret);
	}

	return ret;
}
/*lint -restore*/

/*lint -save -e* */
static void __exit tfa98xx_exit(void)
{
	platform_driver_unregister(&tfa98xx_driver);

}
/*lint -restore*/

/*lint -save -e* */
device_initcall_sync(tfa98xx_init);
module_exit(tfa98xx_exit);
/*lint -restore*/

MODULE_DESCRIPTION("TFA98XX misc device driver");
MODULE_AUTHOR("zhujiaxin<zhujiaxin@huawei.com>");
MODULE_LICENSE("GPL");
#ifdef CONFIG_LLT_TEST
#include "tfa98xx_static_llt.h"
#include "tfa98xx_static_llt.c"
#endif
