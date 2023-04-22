				/*
* maxim-mono.c -- ALSA SoC Mono MAXIM driver
*
* Copyright 2013-2014 Maxim Integrated Products
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*/

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

#include "huawei_platform/audio/maxim.h"

#define HWLOG_TAG maxim
HWLOG_REGIST();


static struct maxim_ioctl_ops *max_io_ops;

//static struct list_head maxim_list;
LIST_HEAD(maxim_list);


struct maxim_priv * maxim_priv_data = NULL;
enum rcv_switch_status {
	RCV_SMARTPA_CONNECTED = 0,
	RCV_CODEC_CONNECTED,
	RCV_DISCONNECTED,
};


static inline void rcv_switch_gpio_set_value(enum rcv_switch_status rcv_status)
{

	switch (rcv_status) {
		case RCV_SMARTPA_CONNECTED:
			gpio_set_value(maxim_priv_data->rcv_en2_gpio, 1);
			gpio_set_value(maxim_priv_data->rcv_en1_gpio, 0);
			break;

		case RCV_CODEC_CONNECTED:
			gpio_set_value(maxim_priv_data->rcv_en2_gpio, 0);
			gpio_set_value(maxim_priv_data->rcv_en1_gpio, 1);
			break;

		case RCV_DISCONNECTED:
			gpio_set_value(maxim_priv_data->rcv_en1_gpio, 0);
			gpio_set_value(maxim_priv_data->rcv_en2_gpio, 0);
			break;

		default:
			break;
		}

}

//ioctl ops regist function
int maxim_ioctl_isregist(void)
{
	return max_io_ops == NULL;
}

int maxim_ioctl_regist(struct maxim_ioctl_ops *ops)
{
	int ret = 0;

	if (ops != NULL) {
		max_io_ops = ops;
	} else {
		hwlog_err("maxin ioctrl interface ops register fail!\n");
		ret = -EPERM;
	}
	return ret;
}


//maxim list_head operation function
void maxim_list_add(struct maxim_priv *maxim)
{
	list_add(&maxim->list, &maxim_list);
}

void maxim_list_del(struct maxim_priv *maxim)
{
	if (maxim->regmap) {
		regmap_exit(maxim->regmap);
	}

	list_del(&maxim->list);
	kfree(maxim);
}

void maxim_list_del_all(void)
{
	struct list_head *pos = NULL;
	struct maxim_priv *p = NULL;
	struct list_head *head = get_maxim_list_head();

	list_for_each(pos, head) {
		p = list_entry(pos, struct maxim_priv, list);
		maxim_list_del(p);
	}
}

struct maxim_priv *find_maxim_by_type(struct list_head *maxim, unsigned int type)
{
	struct list_head *pos = NULL;
	struct maxim_priv *p = NULL;

	list_for_each(pos, maxim) {
		p = list_entry(pos, struct maxim_priv, list);
		if (type == p->type) {
			break;
		} else {
			p = NULL;
		}
	}

	return p;
}


struct maxim_priv *find_maxim_by_dev(struct device *dev)
{
	int ret;
	unsigned int type = 0;
	const char *type_dts_name = "smartpa_type";


	ret = of_property_read_u32(dev->of_node, type_dts_name, &type);
	if (ret) {
		hwlog_info("%s: get smartPA type from dts failed!!!\n", __func__);
		return NULL;
	}

	return find_maxim_by_type(&maxim_list, type);
}


int get_maxim_num(void)
{
	int i = 0;
	struct list_head *pos = NULL;

	list_for_each(pos, &maxim_list) {
		i++;
	}

	return i;
}

struct list_head *get_maxim_list_head(void)
{
	return &maxim_list;
}


static int maxim_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	if (NULL == max_io_ops || NULL == max_io_ops->maxim_open) {
		return -1;
	}

	ret = max_io_ops->maxim_open(inode, filp);

#ifdef CONFIG_HUAWEI_DSM
	if (ret) {
		audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR, "%s: maxim open error %d\n", __func__, ret);
	}
#endif

	return ret;
}


static int maxim_release(struct inode *inode, struct file *filp)
{
	if (NULL == max_io_ops || NULL == max_io_ops->maxim_release) {
		return -1;
	}
	return max_io_ops->maxim_release(inode, filp);
}


static int maxim_do_ioctl(struct file *file, unsigned int cmd, void __user *p, int compat_mode)
{
	int ret = 0;
	unsigned int value = 0;
	//struct maxim_reg_ops reg_val;
	unsigned int __user *pUser = (unsigned int __user *) p;
	struct list_head *maxim = NULL;
	hwlog_info("%s: enter, cmd:%x, maxim_num:%d\n", __func__, cmd, get_maxim_num());

	if (NULL == file) {
		return -EFAULT;
	}

	maxim = (struct list_head *)file->private_data;

	switch (cmd) {
		case MAXIM_GET_VERSION:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_version);
			ret = max_io_ops->maxim_get_version(maxim, MAXIM_L, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_R_GET_VERSION:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_version);
			ret = max_io_ops->maxim_get_version(maxim, MAXIM_R, &value);
			ret |= put_user(value, pUser);
			break;

/*		case MAXIM_GET_REG_VAL:
			if(NULL == pUser)
			{
				return -EFAULT;
			}
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_reg_val);
			ret = max_io_ops->maxim_get_reg_val(maxim, MAXIM_L, &reg_val, pUser);
			break;

		case MAXIM_R_GET_REG_VAL:
			if(NULL == pUser)
			{
				return -EFAULT;
			}
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_reg_val);
			ret = max_io_ops->maxim_get_reg_val(maxim, MAXIM_R, &reg_val, pUser);
			break;

		case MAXIM_SET_REG_VAL:
			if(NULL == pUser)
			{
				return -EFAULT;
			}
			CHECK_IOCTL_OPS(max_io_ops, maxim_set_reg_val);
			ret = max_io_ops->maxim_set_reg_val(maxim, &reg_val, pUser);
			break;
*/
		case MAXIM_POWER_ON:
			CHECK_IOCTL_OPS(max_io_ops, maxim_digital_mute);
			if (maxim_priv_data -> rcv_switch_support == true) {
				rcv_switch_gpio_set_value(RCV_SMARTPA_CONNECTED);
			}
			ret = max_io_ops->maxim_digital_mute(maxim, MUTE_ON);
			break;

		case MAXIM_POWER_OFF:
			CHECK_IOCTL_OPS(max_io_ops, maxim_digital_mute);
			ret = max_io_ops->maxim_digital_mute(maxim, MUTE_OFF);
			if (maxim_priv_data -> rcv_switch_support == true) {
				msleep(20);
				rcv_switch_gpio_set_value(RCV_DISCONNECTED);
			}
			break;

		case MAXIM_POWER_SPK_ON:
			CHECK_IOCTL_OPS(max_io_ops, maxim_single_digital_mute);
			ret = max_io_ops->maxim_single_digital_mute(maxim, MAXIM_L, MUTE_ON);
			break;

		case MAXIM_POWER_SPK_OFF:
			CHECK_IOCTL_OPS(max_io_ops, maxim_single_digital_mute);
			ret = max_io_ops->maxim_single_digital_mute(maxim, MAXIM_L, MUTE_OFF);
			break;

		case MAXIM_POWER_REC_ON:
			CHECK_IOCTL_OPS(max_io_ops, maxim_single_digital_mute);
			if (maxim_priv_data -> rcv_switch_support == true) {
				rcv_switch_gpio_set_value(RCV_CODEC_CONNECTED);
			} else {
				ret = max_io_ops->maxim_single_digital_mute(maxim, MAXIM_R, MUTE_ON);
			}
			break;

		case MAXIM_POWER_REC_OFF:
			CHECK_IOCTL_OPS(max_io_ops, maxim_single_digital_mute);
			ret = max_io_ops->maxim_single_digital_mute(maxim, MAXIM_R, MUTE_OFF);
			if (maxim_priv_data -> rcv_switch_support == true) {
				rcv_switch_gpio_set_value(RCV_DISCONNECTED);
			}
			break;

/*		case MAXIM_GET_VOLUME:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_volume);
			ret = max_io_ops->maxim_get_volume(maxim, MAXIM_L, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_R_GET_VOLUME:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_volume);
			ret = max_io_ops->maxim_get_volume(maxim, MAXIM_R, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_SET_VOLUME:
			CHECK_IOCTL_OPS(max_io_ops, maxim_set_volume);
			ret = get_user(value, pUser);
			ret |= max_io_ops->maxim_set_volume(maxim, value);
			hwlog_info("%s:  maxim smartpa set volume: 0x%x\n", __func__, value);
			break;
*/
		case MAXIM_GET_DAICLOCK:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_daiclock);
			ret = max_io_ops->maxim_get_daiclock(maxim, MAXIM_L, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_R_GET_DAICLOCK:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_daiclock);
			ret = max_io_ops->maxim_get_daiclock(maxim, MAXIM_R, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_SET_DAICLOCK:
			CHECK_IOCTL_OPS(max_io_ops, maxim_set_daiclock);
			ret = get_user(value, pUser);
			hwlog_info("%s: maxim smartpa set daiclock: %d\n", __func__, value);
			ret |= max_io_ops->maxim_set_daiclock(maxim, value);
			break;
/*		case MAXIM_GET_BOOSTVOLT:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_boostvolt);
			ret = max_io_ops->maxim_get_boostvolt(maxim, MAXIM_L, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_R_GET_BOOSTVOLT:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_boostvolt);
			ret = max_io_ops->maxim_get_boostvolt(maxim, MAXIM_R, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_SET_BOOSTVOLT:
			CHECK_IOCTL_OPS(max_io_ops, maxim_set_boostvolt);
			ret = get_user(value, pUser);
			ret |= max_io_ops->maxim_set_boostvolt(maxim, value);
			break;

		case MAXIM_GET_ALCTHRESHOLD:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_alcthreshold);
			ret = max_io_ops->maxim_get_alcthreshold(maxim, MAXIM_L, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_R_GET_ALCTHRESHOLD:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_alcthreshold);
			ret = max_io_ops->maxim_get_alcthreshold(maxim, MAXIM_R, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_SET_ALCTHRESHOLD:
			CHECK_IOCTL_OPS(max_io_ops, maxim_set_alcthreshold);
			ret = get_user(value, pUser);
			hwlog_info("%s: maxim smartpa set alc threshold: 0x%x\n", __func__, value);
			ret |= max_io_ops->maxim_set_alcthreshold(maxim, value);
			break;

		case MAXIM_GET_FILTERS:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_filters);
			ret = max_io_ops->maxim_get_filters(maxim, MAXIM_L, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_R_GET_FILTERS:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_filters);
			ret = max_io_ops->maxim_get_filters(maxim, MAXIM_R, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_SET_FILTERS:
			CHECK_IOCTL_OPS(max_io_ops, maxim_set_filters);
			ret = get_user(value, pUser);
			hwlog_info("%s: maxim smartpa set fliters: 0x%x\n", __func__, value);
			ret |= max_io_ops->maxim_set_filters(maxim, value);
			break;

		case MAXIM_GET_GAINRAMP:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_gainramp);
			ret = max_io_ops->maxim_get_gainramp(maxim, MAXIM_L, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_R_GET_GAINRAMP:
			CHECK_IOCTL_OPS(max_io_ops, maxim_get_gainramp);
			ret = max_io_ops->maxim_get_gainramp(maxim, MAXIM_R, &value);
			ret |= put_user(value, pUser);
			break;

		case MAXIM_SET_GAINRAMP:
			CHECK_IOCTL_OPS(max_io_ops, maxim_set_gainramp);
			ret = get_user(value, pUser);
			hwlog_info("%s: maxim smartpa set gainramp: 0x%x\n", __func__, value);
			ret |= max_io_ops->maxim_set_gainramp(maxim, value);
			break;
		default:
			hwlog_err("%s: cmd input is not support\n", __func__);
			return -EFAULT;
*/
	}
#ifdef CONFIG_HUAWEI_DSM
	if (ret) {
		audio_dsm_report_info(AUDIO_SMARTPA, DSM_SMARTPA_I2C_ERR, "%s: ioctl error %d\n", __func__, ret);
	}
#endif
	return ret;
}


static long maxim_ioctl(struct file *file, unsigned int command, unsigned long arg)
{
	return maxim_do_ioctl(file, command, (void __user *)arg, 0);
}

#ifdef CONFIG_COMPAT
static long maxim_ioctl_compat(struct file *file, unsigned int command, unsigned long arg)
{
	return maxim_do_ioctl(file, command, compat_ptr(arg), 1);
}
#else
#define maxim_ioctl_compat NULL
#endif


static const struct file_operations maxim_ctrl_fops = {
	.owner            = THIS_MODULE,
	.open            = maxim_open,
	.release        = maxim_release,
	.unlocked_ioctl = maxim_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl    = maxim_ioctl_compat,
#endif
};

static struct miscdevice maxim_ctrl_miscdev = {
	.minor =    MISC_DYNAMIC_MINOR,
	.name =     "maxim_smartpa_dev",
	.fops =     &maxim_ctrl_fops,
};


static int maxim_ioctl_probe(struct platform_device *pdev)
{
	int ret = FAILED;
	int val = 0;
	const char *rcv_switch_str = "rcv_switch_support";
	const char *rcv_en1_str = "gpio_rcv_en1";
	const char *rcv_en2_str = "gpio_rcv_en2";
	struct device *dev = &pdev->dev;

	maxim_priv_data = kzalloc(sizeof(struct maxim_priv), GFP_KERNEL);
	if(NULL == maxim_priv_data){
		hwlog_err("%s: maxim kzalloc maxim_priv failed!!!\n", __func__);
		goto err_exit;
	}

	ret = of_property_read_u32(dev->of_node, rcv_switch_str, &val);
	if (ret !=0) {
		hwlog_err("%s: can't get rcv_switch_support from dts, set defaut false value!!!\n", __func__);
		maxim_priv_data->rcv_switch_support = false;
	} else {
		if (val) {
			maxim_priv_data->rcv_switch_support = true;
			hwlog_info("%s: rcv_switch_support is true!\n", __func__);
		} else {
			maxim_priv_data->rcv_switch_support = false;
			hwlog_info("%s: rcv_switch_support is false!\n", __func__);
		}
	}

	if (maxim_priv_data->rcv_switch_support == true)  {
		/* get receiver switch gpio */
		maxim_priv_data->rcv_en1_gpio = of_get_named_gpio(dev->of_node, rcv_en1_str, 0);
		if (maxim_priv_data->rcv_en1_gpio < 0) {
			hwlog_err("rcv_en1_gpio is invalid!\n");
			ret = FAILED;
			goto err_exit;
		}
		ret = gpio_request(maxim_priv_data->rcv_en1_gpio, "rcv_switch_gpio_en1");
		if (ret != 0) {
			hwlog_err("error request GPIO for rcv_en1_gpio fail %d\n", ret);
			goto err_exit;
		}

		maxim_priv_data->rcv_en2_gpio = of_get_named_gpio(dev->of_node, rcv_en2_str, 0);
		if (maxim_priv_data->rcv_en2_gpio < 0) {
			hwlog_err("rcv_en2_gpio is invalid!\n");
			ret = FAILED;
			goto err_rcv_en1_gpio;
		}
		ret = gpio_request(maxim_priv_data->rcv_en2_gpio, "rcv_switch_gpio_en2");
		if (ret != 0) {
			hwlog_err("error request GPIO for rcv_en2_gpio fail %d\n", ret);
			goto err_rcv_en1_gpio;
		}

		rcv_switch_gpio_set_value(RCV_DISCONNECTED);
	}

	ret = misc_register(&maxim_ctrl_miscdev);
	if (0 != ret) {
		hwlog_err("%s: can't register maxim miascdev, ret:%d.\n", __func__, ret);
		goto err_rcv_en2_gpio;
	}

	return 0;

err_rcv_en2_gpio:
	if (maxim_priv_data->rcv_switch_support == true) {
		gpio_free (maxim_priv_data->rcv_en2_gpio);
	}
err_rcv_en1_gpio:
	if (maxim_priv_data->rcv_switch_support == true) {
		gpio_free (maxim_priv_data->rcv_en1_gpio);
	}
err_exit:
	if (ret <0) {
		if (maxim_priv_data) {
			kfree(maxim_priv_data);
			maxim_priv_data = NULL;
		}
	}
    return ret;
}

static int maxim_ioctl_remove(struct platform_device *pdev)
{
	int ret = 0;

	if (maxim_priv_data) {
		if (maxim_priv_data->rcv_switch_support == true) {
			gpio_free (maxim_priv_data->rcv_en2_gpio);
		}
		if (maxim_priv_data->rcv_switch_support == true) {
			gpio_free (maxim_priv_data->rcv_en1_gpio);
		}
		kfree(maxim_priv_data);
		maxim_priv_data = NULL;
	}

	misc_deregister(&maxim_ctrl_miscdev);

	return ret;
}


static const struct of_device_id maxim_match[] = {
	{ .compatible = "huawei,maxim_ioctl", },
	{},
};
MODULE_DEVICE_TABLE(of, maxim_match);


static struct platform_driver maxim_driver = {
	.driver = {
		.name = "maxim_ioctl",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(maxim_match),
	},
	.probe  = maxim_ioctl_probe,
	.remove = maxim_ioctl_remove,
};


static int __init maxim_init(void)
{
	int ret = 0;

	ret = platform_driver_register(&maxim_driver);
	if (ret) {
		hwlog_err("%s: platform_driver_register(maxim_driver) failed, ret:%d.\n", __func__, ret);
	}

	return ret;
}


static void __exit maxim_exit(void)
{
	platform_driver_unregister(&maxim_driver);

}


device_initcall_sync(maxim_init);
module_exit(maxim_exit);

MODULE_DESCRIPTION("MAXIM misc device driver");
MODULE_AUTHOR("zhujiaxin<zhujiaxin@huawei.com>");
MODULE_LICENSE("GPL");
#ifdef CONFIG_LLT_TEST
#include "maxim_static_llt.h"
#include "maxim_static_llt.c"
#endif
