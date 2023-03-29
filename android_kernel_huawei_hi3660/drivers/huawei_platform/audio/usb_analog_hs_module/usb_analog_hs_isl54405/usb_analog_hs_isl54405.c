/*
 *
 * Copyright (c) 2015 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/string.h>
#include <linux/irq.h>
#include <linux/io.h>
#include <linux/of.h>
#include <linux/ioctl.h>
#include <linux/of_device.h>
#include <linux/of_platform.h>
#include <linux/wakelock.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/workqueue.h>
#include <linux/uaccess.h>
#include <linux/fs.h>
#include "huawei_platform/audio/usb_analog_hs_isl54405.h"
#include <linux/hisi/hi64xx/hi64xx_mbhc.h>

#define LOG_TAG "usb_analog_hs_isl54405"

#define PRINT_INFO  1
#define PRINT_WARN  1
#define PRINT_DEBUG 0
#define PRINT_ERR   1

#if PRINT_INFO
#define logi(fmt, ...) printk(LOG_TAG"[I]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define logi(fmt, ...)
#endif

#if PRINT_WARN
#define logw(fmt, ...) printk(LOG_TAG"[W]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define logw(fmt, ...)
#endif

#if PRINT_DEBUG
#define logd(fmt, ...) printk(LOG_TAG"[D]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__)
#else
#define logd(fmt, ...)
#endif

#if PRINT_ERR
#define loge(fmt, ...) printk(LOG_TAG"[E]:%s:%d: "fmt, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#else
#define loge(fmt, ...)
#endif

#define IN_FUNCTION   logi("function comein\n");
#define OUT_FUNCTION  logi("function comeout\n");

#define MAX_BUF_SIZE   (32)

struct usb_ana_hs_isl54405_data {
    int gpio_type;
    int gpio_switch_1v8_en;
    int gpio_mic_switch;
    int gpio_usb_hs_switch;
    int registed;  //usb analog headset dev register flag
    struct wake_lock wake_lock;
    struct mutex mutex;
    struct workqueue_struct *analog_hs_plugin_delay_wq;
    struct delayed_work analog_hs_plugin_delay_work;

    struct workqueue_struct *analog_hs_plugout_delay_wq;
    struct delayed_work analog_hs_plugout_delay_work;

    struct usb_analog_hs_dev *codec_ops_dev;
    void *private_data;  //store codec description data
    int usb_analog_hs_in;
};

static struct usb_ana_hs_isl54405_data *g_pdata_isl54405 = NULL;


static inline int usb_analog_hs_gpio_get_value(int gpio)
{
    if (g_pdata_isl54405->gpio_type == USB_ANALOG_HS_GPIO_CODEC) {
        return gpio_get_value_cansleep(gpio);
    } else {
        return gpio_get_value(gpio);
    }
}

static inline void usb_analog_hs_gpio_set_value(int gpio, int value)
{
    if (g_pdata_isl54405->gpio_type == USB_ANALOG_HS_GPIO_CODEC) {
        gpio_set_value_cansleep(gpio, value);
    } else {
        gpio_set_value(gpio, value);
    }
}

static void usb_analog_hs_plugin_work(struct work_struct *work)
{
    enum hisi_jack_states hs_type = HISI_JACK_NONE;

    IN_FUNCTION;

    wake_lock(&g_pdata_isl54405->wake_lock);
    g_pdata_isl54405->codec_ops_dev->ops.plug_in_detect(g_pdata_isl54405->private_data);
    mutex_lock(&g_pdata_isl54405->mutex);
    g_pdata_isl54405->usb_analog_hs_in = USB_ANA_HS_PLUG_IN;
    hs_type = g_pdata_isl54405->codec_ops_dev->ops.get_headset_type(g_pdata_isl54405->private_data);

    logi("hs_type =%d!\n",hs_type);
    if(hs_type == HISI_JACK_HEADSET || hs_type == HISI_JACK_HEADPHONE ||
        hs_type == HISI_JACK_INVERT )
        usb_analog_hs_gpio_set_value(g_pdata_isl54405->gpio_usb_hs_switch, 0);

    mutex_unlock(&g_pdata_isl54405->mutex);
    wake_unlock(&g_pdata_isl54405->wake_lock);

    OUT_FUNCTION;
}

static void usb_analog_hs_plugout_work(struct work_struct *work)
{
    IN_FUNCTION;

    wake_lock(&g_pdata_isl54405->wake_lock);
    if (g_pdata_isl54405->usb_analog_hs_in == USB_ANA_HS_PLUG_IN) {
        logi("usb analog hs plug out act!\n");
        g_pdata_isl54405->codec_ops_dev->ops.plug_out_detect(g_pdata_isl54405->private_data);
        mutex_lock(&g_pdata_isl54405->mutex);
        g_pdata_isl54405->usb_analog_hs_in = USB_ANA_HS_PLUG_OUT;
        usb_analog_hs_gpio_set_value(g_pdata_isl54405->gpio_usb_hs_switch, 1);
        mutex_unlock(&g_pdata_isl54405->mutex);
    }
    wake_unlock(&g_pdata_isl54405->wake_lock);

    OUT_FUNCTION;
}

int usb_ana_hs_isl54405_dev_register(struct usb_analog_hs_dev *dev, void *codec_data)
{
    /* usb analog headset driver not be probed, just return */
    if (g_pdata_isl54405 == NULL) {
        loge("pdata is NULL\n");
        return -ENODEV;
    }

    /* only support one codec to be registed */
    if (g_pdata_isl54405->registed == USB_ANALOG_HS_ALREADY_REGISTER) {
        loge("one codec has registed, no more permit\n");
        return -EEXIST;
    }

    if (!dev->ops.plug_in_detect ||
        !dev->ops.plug_out_detect ||
        !dev->ops.get_headset_type) {
        loge("codec ops funtion must be all registed\n");
        return -EINVAL;
    }

    mutex_lock(&g_pdata_isl54405->mutex);
    g_pdata_isl54405->codec_ops_dev = dev;
    g_pdata_isl54405->private_data = codec_data;
    g_pdata_isl54405->registed = USB_ANALOG_HS_ALREADY_REGISTER;
    mutex_unlock(&g_pdata_isl54405->mutex);

    logi("usb analog hs has been register sucessful!\n");

    return 0;
}

int usb_ana_hs_isl54405_check_hs_pluged_in(void)
{
    int typec_detach = PD_DPM_TYPEC_UNATTACHED;

    pd_dpm_get_typec_state(&typec_detach);

    logi("typec_detach =%d\n",typec_detach);

    if(typec_detach == PD_DPM_USB_TYPEC_AUDIO_ATTACHED)
        return USB_ANA_HS_PLUG_IN;
    else
        return USB_ANA_HS_PLUG_OUT;
}

void usb_ana_hs_isl54405_mic_swtich_change_state(void)
{
    int gpio_mic_sel_val = 0;

    /* usb analog headset driver not be probed, just return */
    if (g_pdata_isl54405 == NULL) {
        loge("pdata is NULL\n");
        return;
    }

    if(g_pdata_isl54405->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("codec_ops_dev is not registed\n");
        return;
    }

    IN_FUNCTION;

    gpio_mic_sel_val = usb_analog_hs_gpio_get_value(g_pdata_isl54405->gpio_mic_switch);
    logi("gpio mic sel is %d!\n", gpio_mic_sel_val);

    if (gpio_mic_sel_val == 0) {
        gpio_mic_sel_val = 1;
    } else {
        gpio_mic_sel_val = 0;
    }
    logi("gpio mic sel will set to %d!\n", gpio_mic_sel_val);

    usb_analog_hs_gpio_set_value(g_pdata_isl54405->gpio_mic_switch, gpio_mic_sel_val);
    gpio_mic_sel_val = usb_analog_hs_gpio_get_value(g_pdata_isl54405->gpio_mic_switch);
    logi("gpio mic sel change to %d!\n",gpio_mic_sel_val);

    OUT_FUNCTION;

}

void usb_ana_hs_isl54405_plug_in_out_handle(int hs_state)
{
    /* usb analog headset driver not be probed, just return */
    if (g_pdata_isl54405 == NULL) {
        loge("pdata is NULL\n");
        return;
    }

    if(g_pdata_isl54405->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("codec_ops_dev is not registed\n");
        return;
    }
    IN_FUNCTION;

    logi("hs_state is %d[%s]\n",hs_state,
        (hs_state == USB_ANA_HS_PLUG_IN)?"PLUG_IN":"PLUG_OUT");

    wake_lock_timeout(&g_pdata_isl54405->wake_lock, msecs_to_jiffies(1000));

    if(hs_state == USB_ANA_HS_PLUG_IN) {
        queue_delayed_work(g_pdata_isl54405->analog_hs_plugin_delay_wq,
                           &g_pdata_isl54405->analog_hs_plugin_delay_work,
                           msecs_to_jiffies(800));
    } else {
        queue_delayed_work(g_pdata_isl54405->analog_hs_plugout_delay_wq,
                           &g_pdata_isl54405->analog_hs_plugout_delay_work,
                           0);
    }
    OUT_FUNCTION;
}

bool check_usb_analog_hs_isl54405_support(void)
{
    if(g_pdata_isl54405 == NULL)
        return false;

    if(g_pdata_isl54405->registed == USB_ANALOG_HS_NOT_REGISTER)
        return false;
    else
        return true;
}

static const struct of_device_id usb_ana_hs_isl54405_of_match[] = {
    {
        .compatible = "huawei,usb_ana_hs_isl54405",
    },
    { },
};
MODULE_DEVICE_TABLE(of, usb_ana_hs_isl54405_of_match);

/* load dts config for board difference */
static void load_gpio_type_config(struct device_node *node)
{
    unsigned int temp = USB_ANALOG_HS_GPIO_SOC;

    if (!of_property_read_u32(node, "gpio_type", &temp)) {
        g_pdata_isl54405->gpio_type = temp;
    } else {
        g_pdata_isl54405->gpio_type = USB_ANALOG_HS_GPIO_SOC;
    }

}

static int usb_ana_hs_isl54405_load_gpio_pdata(struct device *dev,
    struct usb_ana_hs_isl54405_data *data)
{
    int ret = 0;

    /* get swtich 1v8 control gpio */
    data->gpio_switch_1v8_en =  of_get_named_gpio(dev->of_node, "swtich_1v8_en", 0);
    if (data->gpio_switch_1v8_en < 0) {
        loge(":Looking up %s property in node swtich_1v8_en failed %d\n"
            , dev->of_node->full_name, data->gpio_switch_1v8_en);
        ret = -ENOENT;
        goto isl54405_get_gpio_err;
    }

    if (!gpio_is_valid(data->gpio_switch_1v8_en)) {
        loge("gpio audio 1v8 en is unvalid!\n");
        ret = -ENOENT;
        goto isl54405_get_gpio_err;
    }

    /* applay for audio 1v8 en gpio */
    ret = gpio_request(data->gpio_switch_1v8_en, "swtich_1v8_en");
    if (ret < 0) {
        loge("error request GPIO for audio 3v3 en fail %d\n", ret);
        goto isl54405_get_gpio_err;
    }
    gpio_direction_output(data->gpio_switch_1v8_en, 1);

    /* get mic sel control gpio */
    data->gpio_mic_switch =  of_get_named_gpio(dev->of_node, "gpio_mic_switch", 0);
    if (data->gpio_mic_switch < 0) {
        loge(":Looking up %s property in node gpio_mic_sel failed %d\n"
            , dev->of_node->full_name, data->gpio_mic_switch);
        ret = -ENOENT;
        goto isl54405_get_gpio_switch_1v8_en_err;
    }

    if (!gpio_is_valid(data->gpio_mic_switch)) {
        loge("gpio mic sel is unvalid!\n");
        ret = -ENOENT;
        goto isl54405_get_gpio_switch_1v8_en_err;
    }

    /* applay for mic sel  gpio */
    ret = gpio_request(data->gpio_mic_switch, "gpio_mic_switch");
    if (ret < 0) {
        loge("error request GPIO for mic sel fail %d\n", ret);
        goto isl54405_get_gpio_switch_1v8_en_err;
    }
    gpio_direction_output(data->gpio_mic_switch, 1);

    /* get usb hs switch control gpio */
    data->gpio_usb_hs_switch =  of_get_named_gpio(dev->of_node, "gpio_usb_hs_switch", 0);
    if (data->gpio_usb_hs_switch < 0) {
        loge(":Looking up %s property in node gpio_usb_hs_switch failed %d\n"
            , dev->of_node->full_name, data->gpio_usb_hs_switch);
        ret = -ENOENT;
        goto isl54405_get_gpio_mic_switch_err;
    }

    if (!gpio_is_valid(data->gpio_usb_hs_switch)) {
        loge("gpio usb hs switch is unvalid!\n");
        ret = -ENOENT;
        goto isl54405_get_gpio_mic_switch_err;
    }

    /* applay for usb hs switch  gpio */
    ret = gpio_request(data->gpio_usb_hs_switch, "gpio_usb_hs_switch");
    if (ret < 0) {
        loge("error request GPIO for usb hs switch fail %d\n", ret);
        goto isl54405_get_gpio_mic_switch_err;
    }
    gpio_direction_output(data->gpio_usb_hs_switch, 1);

    return 0;
isl54405_get_gpio_mic_switch_err:
    gpio_free(data->gpio_mic_switch);
isl54405_get_gpio_switch_1v8_en_err:
    gpio_free(data->gpio_switch_1v8_en);
isl54405_get_gpio_err:
    return ret;
}

static void usb_analog_hs_free_gpio(struct usb_ana_hs_isl54405_data *data)
{
    IN_FUNCTION;

    if(data->gpio_switch_1v8_en > 0)
        gpio_free(data->gpio_switch_1v8_en);

    if(data->gpio_mic_switch > 0)
        gpio_free(data->gpio_mic_switch);

    if(data->gpio_usb_hs_switch > 0)
        gpio_free(data->gpio_usb_hs_switch);
}

#ifdef USB_ANALOG_HS_ISL54405_DEBUG
static ssize_t usb_ana_hs_isl54405_mic_switch_show(struct device *dev,
                      struct device_attribute *attr, char *buf)
{
    int value = 0;
    int count = 0;

    if (g_pdata_isl54405 == NULL) {
        loge("pdata is NULL or not registed!\n");
        return count;
    }

    if (g_pdata_isl54405->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("pdata is NULL or not registed!\n");
        return count;
    }

    value = usb_analog_hs_gpio_get_value(g_pdata_isl54405->gpio_mic_switch);

    return scnprintf(buf, MAX_BUF_SIZE, "%d\n", value);
}

static ssize_t usb_ana_hs_isl54405_mic_switch_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{
    int ret = 0;
    long val;

    if (g_pdata_isl54405 == NULL) {
        loge("pdata is NULL or not registed!\n");
        return count;
    }

    if (g_pdata_isl54405->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("pdata is NULL or not registed!\n");
        return count;
    }

    ret = kstrtol(buf, 10, &val);
    if (ret < 0) {
        loge("input error!\n");
        return count;
    }

    if(val)
        usb_analog_hs_gpio_set_value(g_pdata_isl54405->gpio_mic_switch, 1);
    else
        usb_analog_hs_gpio_set_value(g_pdata_isl54405->gpio_mic_switch, 0);

    return count;
}

static DEVICE_ATTR(mic_switch, 0660, usb_ana_hs_isl54405_mic_switch_show,
                   usb_ana_hs_isl54405_mic_switch_store);

static struct attribute *usb_ana_hs_isl54405_attributes[] = {
    &dev_attr_mic_switch.attr,
    NULL
};

static const struct attribute_group usb_ana_hs_isl54405_attr_group = {
    .attrs = usb_ana_hs_isl54405_attributes,
};
#endif

static long usb_ana_hs_isl54405_ioctl(struct file *file, unsigned int cmd,
                               unsigned long arg)
{
    int ret = 0;
    int gpio_mic_sel_val = 0;

    unsigned int __user *p_user = (unsigned int __user *) arg;

    if (g_pdata_isl54405 == NULL)
        return -EBUSY;

    if (g_pdata_isl54405->registed == USB_ANALOG_HS_NOT_REGISTER)
        return -EBUSY;

    switch (cmd) {
        case IOCTL_USB_ANA_HS_GET_MIC_SWITCH_STATE:
            gpio_mic_sel_val = usb_analog_hs_gpio_get_value(g_pdata_isl54405->gpio_mic_switch);
            logi("gpio_mic_sel_val = %d!\n", gpio_mic_sel_val);
            ret = put_user((__u32)(gpio_mic_sel_val),p_user);
            break;
        default:
            loge("unsupport cmd\n");
            ret = -EINVAL;
            break;
    }

    return (long)ret;
}

static const struct file_operations usb_ana_hs_isl54405_fops = {
    .owner               = THIS_MODULE,
    .open                = simple_open,
    .unlocked_ioctl      = usb_ana_hs_isl54405_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl        = usb_ana_hs_isl54405_ioctl,
#endif
};

static struct miscdevice usb_ana_hs_isl54405_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "usb_analog_hs",
    .fops   = &usb_ana_hs_isl54405_fops,
};

struct usb_analog_hs_ops usb_ana_hs_isl54405_ops = {
	.usb_ana_hs_check_headset_pluged_in = usb_ana_hs_isl54405_check_hs_pluged_in,
	.usb_ana_hs_dev_register = usb_ana_hs_isl54405_dev_register,
	.check_usb_ana_hs_support = check_usb_analog_hs_isl54405_support,
	.usb_ana_hs_plug_in_out_handle = usb_ana_hs_isl54405_plug_in_out_handle,
	.usb_ana_hs_mic_swtich_change_state = usb_ana_hs_isl54405_mic_swtich_change_state,
};

static int usb_ana_hs_isl54405_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *node =  dev->of_node;
    int ret = 0;

    IN_FUNCTION;

    g_pdata_isl54405 = kzalloc(sizeof(struct usb_ana_hs_isl54405_data), GFP_KERNEL);
    if (NULL == g_pdata_isl54405) {
        loge("cannot allocate anc hs dev data\n");
        return -ENOMEM;
    }

    ret = usb_ana_hs_isl54405_load_gpio_pdata(dev, g_pdata_isl54405);
    if (ret < 0){
        loge("get gpios failed, ret =%d\n",ret);
        goto isl54405_err_out;
    }

    wake_lock_init(&g_pdata_isl54405->wake_lock, WAKE_LOCK_SUSPEND, "usb_analog_hs");
    mutex_init(&g_pdata_isl54405->mutex);

    /* load dts config for board difference */
    load_gpio_type_config(node);
    g_pdata_isl54405->registed = USB_ANALOG_HS_NOT_REGISTER;
    g_pdata_isl54405->usb_analog_hs_in = USB_ANA_HS_PLUG_OUT;
    usb_analog_hs_gpio_set_value(g_pdata_isl54405->gpio_switch_1v8_en, 1);
    usb_analog_hs_gpio_set_value(g_pdata_isl54405->gpio_usb_hs_switch, 1);

    /* create workqueue */
    g_pdata_isl54405->analog_hs_plugin_delay_wq =
        create_singlethread_workqueue("usb_analog_hs_plugin_delay_wq");
    if (!(g_pdata_isl54405->analog_hs_plugin_delay_wq)) {
        loge("plugin wq create failed\n");
        ret = -ENOMEM;
        goto isl54405_err_out;
    }
    INIT_DELAYED_WORK(&g_pdata_isl54405->analog_hs_plugin_delay_work, usb_analog_hs_plugin_work);

    g_pdata_isl54405->analog_hs_plugout_delay_wq =
        create_singlethread_workqueue("usb_analog_hs_plugout_delay_wq");
    if (!(g_pdata_isl54405->analog_hs_plugout_delay_wq)) {
        loge("plugout wq create failed\n");
        ret = -ENOMEM;
        goto isl54405_err_plugin_delay_wq;
    }
    INIT_DELAYED_WORK(&g_pdata_isl54405->analog_hs_plugout_delay_work, usb_analog_hs_plugout_work);

    ret = usb_analog_hs_ops_register(&usb_ana_hs_isl54405_ops);
    if (ret) {
        loge("register usb_ana_hs_isl54405_ops ops failed!\n");
        goto isl54405_err_plugin_delay_wq;
    }

    /* register misc device for userspace */
    ret = misc_register(&usb_ana_hs_isl54405_device);
    if (ret) {
        loge("usb_analog_hs_device misc device register failed\n");
        goto isl54405_err_misc_register;
    }

#ifdef USB_ANALOG_HS_ISL54405_DEBUG
    ret = sysfs_create_group(&dev->kobj, &usb_ana_hs_isl54405_attr_group);
    if (ret < 0)
        loge("failed to register sysfs\n");
#endif

    logi("usb_analog_hs probe success!\n");
    return 0;

isl54405_err_misc_register:
    if (g_pdata_isl54405->analog_hs_plugout_delay_wq) {
        cancel_delayed_work(&g_pdata_isl54405->analog_hs_plugout_delay_work);
        flush_workqueue(g_pdata_isl54405->analog_hs_plugout_delay_wq);
        destroy_workqueue(g_pdata_isl54405->analog_hs_plugout_delay_wq);
    }
isl54405_err_plugin_delay_wq:
    if (g_pdata_isl54405->analog_hs_plugin_delay_wq) {
        cancel_delayed_work(&g_pdata_isl54405->analog_hs_plugin_delay_work);
        flush_workqueue(g_pdata_isl54405->analog_hs_plugin_delay_wq);
        destroy_workqueue(g_pdata_isl54405->analog_hs_plugin_delay_wq);
    }
isl54405_err_out:
    kfree(g_pdata_isl54405);
    g_pdata_isl54405 = NULL;

    return ret;

}

static int usb_ana_hs_isl54405_remove(struct platform_device *pdev)
{
    logi("in remove\n");

    if (g_pdata_isl54405 == NULL) {
        return 0;
    }

    if (g_pdata_isl54405->analog_hs_plugin_delay_wq) {
        cancel_delayed_work(&g_pdata_isl54405->analog_hs_plugin_delay_work);
        flush_workqueue(g_pdata_isl54405->analog_hs_plugin_delay_wq);
        destroy_workqueue(g_pdata_isl54405->analog_hs_plugin_delay_wq);
    }

    if (g_pdata_isl54405->analog_hs_plugout_delay_wq) {
        cancel_delayed_work(&g_pdata_isl54405->analog_hs_plugout_delay_work);
        flush_workqueue(g_pdata_isl54405->analog_hs_plugout_delay_wq);
        destroy_workqueue(g_pdata_isl54405->analog_hs_plugout_delay_wq);
    }

    usb_analog_hs_free_gpio(g_pdata_isl54405);
    misc_deregister(&usb_ana_hs_isl54405_device);

#ifdef USB_ANALOG_HS_ISL54405_DEBUG
    sysfs_remove_group(&pdev->dev.kobj, &usb_ana_hs_isl54405_attr_group);
#endif

    kfree(g_pdata_isl54405);
    g_pdata_isl54405 = NULL;

    return 0;
}

static struct platform_driver usb_ana_hs_isl54405_driver = {
    .driver = {
        .name   = "usb_ana_hs_isl54405",
        .owner  = THIS_MODULE,
        .of_match_table = usb_ana_hs_isl54405_of_match,
    },
    .probe  = usb_ana_hs_isl54405_probe,
    .remove = usb_ana_hs_isl54405_remove,
};

static int __init usb_ana_hs_isl5405_init(void)
{
    return platform_driver_register(&usb_ana_hs_isl54405_driver);
}

static void __exit usb_ana_hs_isl5405_exit(void)
{
    platform_driver_unregister(&usb_ana_hs_isl54405_driver);
}

subsys_initcall_sync(usb_ana_hs_isl5405_init);
module_exit(usb_ana_hs_isl5405_exit);

MODULE_DESCRIPTION("usb analog headset swtich isl5405 driver");
MODULE_LICENSE("GPL");
