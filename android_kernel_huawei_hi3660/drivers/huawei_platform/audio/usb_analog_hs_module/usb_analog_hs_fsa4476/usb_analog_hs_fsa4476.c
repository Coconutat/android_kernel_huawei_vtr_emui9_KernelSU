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
#include "huawei_platform/audio/usb_analog_hs_fsa4476.h"
#include <linux/hisi/hi64xx/hi64xx_mbhc.h>
#ifdef CONFIG_SUPERSWITCH_FSC
#include "../../../usb/superswitch/fsc/Platform_Linux/fusb3601_global.h"
#endif

#define LOG_TAG "usb_analog_hs_fsa4476"
#define SD_GPIO_SCTRL_REG    0xfff0a314

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
#define DEFLAUT_SLEEP_TIME_20MS   (20)
#define DEFAULT_MIC_SWITCH_DELAY_TIME   (0)

struct usb_ana_hs_fsa4476_data {
    int gpio_type;
    int gpio_en1;
    int gpio_en2;
    int gpio_enn;
    int mic_gnd_np; /* H: sbu1/sbu2 hung up; L: sbu1/sbu2 connected to mic/gnd */
    int mic_gnd_switch; /* H: sbu1-->mic, sbu2-->gnd; L: sbu1-->gnd, sbu2-->mic */
    int mic_switch_delay_time;
    int registed;  //usb analog headset dev register flag
    int sd_gpio_used;
    struct wake_lock wake_lock;
    struct mutex mutex;
    struct workqueue_struct *analog_hs_plugin_delay_wq;
    struct delayed_work analog_hs_plugin_delay_work;

    struct workqueue_struct *analog_hs_plugout_delay_wq;
    struct delayed_work analog_hs_plugout_delay_work;

    struct usb_analog_hs_dev *codec_ops_dev;
    void *private_data;  //store codec description data
    int usb_analog_hs_in;
    bool using_superswitch;
    bool support_cc;
    bool connect_linein_r;
    bool switch_antenna; //HUAWEI USB-C TO 3.5MM AUDIO ADAPTER has TDD noise, when usb analog hs plug in, need switch to upper antenna.
};

enum {
    FSA4776_ENABLE     = 0,
    FSA4776_DISABLE    = 1,
};

static struct usb_ana_hs_fsa4476_data *g_pdata_fsa4476 = NULL;

static inline int usb_analog_hs_gpio_get_value(int gpio)
{
    if (g_pdata_fsa4476->gpio_type == USB_ANALOG_HS_GPIO_CODEC) {
        return gpio_get_value_cansleep(gpio);
    } else {
        return gpio_get_value(gpio);
    }
}

static inline void usb_analog_hs_gpio_set_value(int gpio, int value)
{
    if (g_pdata_fsa4476->gpio_type == USB_ANALOG_HS_GPIO_CODEC) {
        gpio_set_value_cansleep(gpio, value);
    } else {
        gpio_set_value(gpio, value);
    }
}

void usb_analog_hs_fsa4476_set_gpio_state(int enn, int en1, int en2)
{
    if (g_pdata_fsa4476->using_superswitch) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_enn, enn);
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en1, en1);
        if(g_pdata_fsa4476->mic_gnd_np> 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_np, (en1==0) ? 1:0);
        }
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, en2);
        if(g_pdata_fsa4476->mic_gnd_switch> 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_switch, en2);
        }
    } else if (g_pdata_fsa4476->support_cc) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en1, en1);
        if(g_pdata_fsa4476->mic_gnd_np> 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_np, (en1==0) ? 1:0);
        }
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, en2);
        if(g_pdata_fsa4476->mic_gnd_switch> 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_switch, en2);
        }
    }
    return;
}

static void usb_analog_hs_fsa4476_enable(int enable)
{
    if (g_pdata_fsa4476->using_superswitch) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_enn, enable);
    }
    return;
}

#ifdef CONFIG_SUPERSWITCH_FSC
static int set_superswitch_sbu_switch(SbuSwitch_t SbuSwitch)
{
    if (g_pdata_fsa4476->using_superswitch) {
        struct fusb3601_chip *chip = fusb3601_GetChip();
        if (!chip) {
            loge("fusb3601_chip is NULL!\n");
            return -1;
        }
        FUSB3601_set_sbu_switch(&chip->port, SbuSwitch);
    }
    return 0;
}
#endif

static void usb_analog_hs_plugin_work(struct work_struct *work)
{
    IN_FUNCTION;

    wake_lock(&g_pdata_fsa4476->wake_lock);
    //change codec hs resistence from 70ohm to 3Kohm, to reduce the pop sound in hs when usb analog hs plug in.
    g_pdata_fsa4476->codec_ops_dev->ops.hs_high_resistence_enable(g_pdata_fsa4476->private_data, true);
    #ifdef CONFIG_SUPERSWITCH_FSC
    if (set_superswitch_sbu_switch(Sbu_Cross_Close_Aux) < 0) { //SBU2 connect to HS_FB
        wake_unlock(&g_pdata_fsa4476->wake_lock);
        return;
    }
    #endif

    if (g_pdata_fsa4476->switch_antenna) {
        pd_dpm_send_event(ANA_AUDIO_IN_EVENT); //notify the phone: usb analog hs plug in, switch to upper antenna, to avoid TDD-noise
    }

    usb_analog_hs_fsa4476_enable(FSA4776_ENABLE);
    usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en1, 1);
    if(g_pdata_fsa4476->mic_gnd_np > 0) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_np, 0);
    }
    msleep(DEFLAUT_SLEEP_TIME_20MS);
    g_pdata_fsa4476->codec_ops_dev->ops.plug_in_detect(g_pdata_fsa4476->private_data);
    mutex_lock(&g_pdata_fsa4476->mutex);
    g_pdata_fsa4476->usb_analog_hs_in = USB_ANA_HS_PLUG_IN;

    mutex_unlock(&g_pdata_fsa4476->mutex);
    //recovery codec hs resistence to 70ohm, to avoid affecting other hs businesses.
    g_pdata_fsa4476->codec_ops_dev->ops.hs_high_resistence_enable(g_pdata_fsa4476->private_data, false);
    wake_unlock(&g_pdata_fsa4476->wake_lock);

    OUT_FUNCTION;
}

static void usb_analog_hs_plugout_work(struct work_struct *work)
{
    IN_FUNCTION;

    wake_lock(&g_pdata_fsa4476->wake_lock);
    if (g_pdata_fsa4476->analog_hs_plugin_delay_wq) {
         logi("remove plugin work in plugin_delay_workqueue if exist to prevent GPIO-EN1 remaining high caused by inserting-removing headset too fast!\n");
         cancel_delayed_work(&g_pdata_fsa4476->analog_hs_plugin_delay_work);
         flush_workqueue(g_pdata_fsa4476->analog_hs_plugin_delay_wq);
    }
    if (g_pdata_fsa4476->usb_analog_hs_in == USB_ANA_HS_PLUG_IN) {
        logi("usb analog hs plug out act!\n");
        g_pdata_fsa4476->codec_ops_dev->ops.plug_out_detect(g_pdata_fsa4476->private_data);
        mutex_lock(&g_pdata_fsa4476->mutex);
        g_pdata_fsa4476->usb_analog_hs_in = USB_ANA_HS_PLUG_OUT;
        mutex_unlock(&g_pdata_fsa4476->mutex);
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en1, 0);
        if(g_pdata_fsa4476->mic_gnd_np > 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_np, 1);
        }
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, 0);
        if(g_pdata_fsa4476->mic_gnd_switch > 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_switch, 0);
        }
        if (g_pdata_fsa4476->switch_antenna) {
            pd_dpm_send_event(ANA_AUDIO_OUT_EVENT); //notify the phone: usb analog hs plug out.
        }

        #ifdef CONFIG_SUPERSWITCH_FSC
        if (set_superswitch_sbu_switch(Sbu_None) < 0) { //HS_FB disconnect
            wake_unlock(&g_pdata_fsa4476->wake_lock);
            return;
        }
        #endif
        usb_analog_hs_fsa4476_enable(FSA4776_DISABLE);
    }
    wake_unlock(&g_pdata_fsa4476->wake_lock);

    OUT_FUNCTION;
}

int usb_ana_hs_fsa4476_dev_register(struct usb_analog_hs_dev *dev, void *codec_data)
{
    /* usb analog headset driver not be probed, just return */
    if (g_pdata_fsa4476 == NULL) {
        loge("pdata is NULL\n");
        return -ENODEV;
    }

    /* only support one codec to be registed */
    if (g_pdata_fsa4476->registed == USB_ANALOG_HS_ALREADY_REGISTER) {
        loge("one codec has registed, no more permit\n");
        return -EEXIST;
    }

    if (!dev->ops.plug_in_detect ||
        !dev->ops.plug_out_detect) {
        loge("codec ops funtion must be all registed\n");
        return -EINVAL;
    }

    mutex_lock(&g_pdata_fsa4476->mutex);
    g_pdata_fsa4476->codec_ops_dev = dev;
    g_pdata_fsa4476->private_data = codec_data;
    g_pdata_fsa4476->registed = USB_ANALOG_HS_ALREADY_REGISTER;
    mutex_unlock(&g_pdata_fsa4476->mutex);

    logi("usb analog hs has been register sucessful!\n");

    return 0;
}

int usb_ana_hs_fsa4476_check_hs_pluged_in(void)
{
    int analog_hs_state = pd_dpm_get_analog_hs_state();

    logi("analog_hs_state =%d\n",analog_hs_state);

    if(analog_hs_state)
        return USB_ANA_HS_PLUG_IN;
    else
        return USB_ANA_HS_PLUG_OUT;
}

void usb_ana_hs_fsa4476_mic_swtich_change_state(void)
{
    int gpio_mic_sel_val = 0;
    int gpio_mic_gnd_switch = 0;
    /* usb analog headset driver not be probed, just return */
    if (g_pdata_fsa4476 == NULL) {
        loge("pdata is NULL\n");
        return;
    }

    if(g_pdata_fsa4476->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("codec_ops_dev is not registed\n");
        return;
    }

    IN_FUNCTION;

    gpio_mic_sel_val = usb_analog_hs_gpio_get_value(g_pdata_fsa4476->gpio_en2);
    logi("gpio mic sel is %d!\n", gpio_mic_sel_val);
    if(g_pdata_fsa4476->mic_gnd_switch > 0) {
        gpio_mic_gnd_switch = usb_analog_hs_gpio_get_value(g_pdata_fsa4476->mic_gnd_switch);
        logi("gpio mic gnd switch is %d!\n", gpio_mic_gnd_switch);
    }

    if (gpio_mic_sel_val == 0) {
        gpio_mic_sel_val = 1;
        #ifdef CONFIG_SUPERSWITCH_FSC
        if (set_superswitch_sbu_switch(Sbu_Close_Aux) < 0) { //SBU1 connect to HS_FB
            return;
        }
        #endif
    } else {
        gpio_mic_sel_val = 0;
        #ifdef CONFIG_SUPERSWITCH_FSC
        if (set_superswitch_sbu_switch(Sbu_Cross_Close_Aux) < 0) { //SBU2 connect to HS_FB
            return;
        }
        #endif
    }
    logi("gpio mic sel will set to %d!\n", gpio_mic_sel_val);

    usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, gpio_mic_sel_val);
    if(g_pdata_fsa4476->mic_gnd_switch > 0) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_switch, gpio_mic_sel_val);
    }
    if(g_pdata_fsa4476->mic_switch_delay_time > 0)
        msleep(g_pdata_fsa4476->mic_switch_delay_time);
    gpio_mic_sel_val = usb_analog_hs_gpio_get_value(g_pdata_fsa4476->gpio_en2);
    logi("gpio mic sel change to %d!\n",gpio_mic_sel_val);
    if(g_pdata_fsa4476->mic_gnd_switch > 0) {
        gpio_mic_gnd_switch = usb_analog_hs_gpio_get_value(g_pdata_fsa4476->mic_gnd_switch);
        logi("gpio mic gnd switch change to %d!\n",gpio_mic_gnd_switch);
    }

    OUT_FUNCTION;

}

void usb_ana_hs_fsa4476_plug_in_out_handle(int hs_state)
{
    /* usb analog headset driver not be probed, just return */
    if (g_pdata_fsa4476 == NULL) {
        loge("pdata is NULL\n");
        return;
    }

    if(g_pdata_fsa4476->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("codec_ops_dev is not registed\n");
        return;
    }
    IN_FUNCTION;

    wake_lock_timeout(&g_pdata_fsa4476->wake_lock, msecs_to_jiffies(1000));

    switch (hs_state) {
        case USB_ANA_HS_PLUG_IN:
            queue_delayed_work(g_pdata_fsa4476->analog_hs_plugin_delay_wq,
                    &g_pdata_fsa4476->analog_hs_plugin_delay_work,
                    msecs_to_jiffies(800));
            break;
        case USB_ANA_HS_PLUG_OUT:
            queue_delayed_work(g_pdata_fsa4476->analog_hs_plugout_delay_wq,
                    &g_pdata_fsa4476->analog_hs_plugout_delay_work, 0);
            break;
        case DP_PLUG_IN:
            usb_analog_hs_fsa4476_set_gpio_state(FSA4776_ENABLE, 0, 0);
            break;
        case DP_PLUG_IN_CROSS:
            usb_analog_hs_fsa4476_set_gpio_state(FSA4776_ENABLE, 0, 1);
            break;
        case DP_PLUG_OUT:
            usb_analog_hs_fsa4476_set_gpio_state(FSA4776_DISABLE, 0, 0);
            break;
        case DIRECT_CHARGE_IN:
            usb_analog_hs_fsa4476_set_gpio_state(FSA4776_ENABLE, 0, 0);
            break;
        case DIRECT_CHARGE_OUT:
            usb_analog_hs_fsa4476_set_gpio_state(FSA4776_DISABLE, 0, 0);
            break;
        default:
            break;
    }
    logi("hs_state is %d\n", hs_state);
    OUT_FUNCTION;
}

bool check_usb_analog_hs_fsa4476_support(void)
{
    if (g_pdata_fsa4476 == NULL)
        return false;

    if(g_pdata_fsa4476->registed == USB_ANALOG_HS_NOT_REGISTER)
        return false;
    else
        return true;
}

static const struct of_device_id usb_ana_hs_fsa4476_of_match[] = {
    {
        .compatible = "huawei,usb_ana_hs_fsa4476",
    },
    { },
};
MODULE_DEVICE_TABLE(of, usb_ana_hs_fsa4476_of_match);

/* load dts config for board difference */
static void load_gpio_type_config(struct device_node *node)
{
    unsigned int temp = USB_ANALOG_HS_GPIO_SOC;

    if (!of_property_read_u32(node, "gpio_type", &temp)) {
        g_pdata_fsa4476->gpio_type = temp;
    } else {
        g_pdata_fsa4476->gpio_type = USB_ANALOG_HS_GPIO_SOC;
    }

}

static int usb_ana_hs_fsa4476_load_gpio_pdata(struct device *dev,
    struct usb_ana_hs_fsa4476_data *data)
{
    int ret = 0;

    /* get swtich enn control gpio */
    data->gpio_enn = of_get_named_gpio(dev->of_node, "swtich_enn", 0);
    if (data->gpio_enn < 0) {
        loge(":Looking up %s property in node swtich_enn failed %d\n"
            , dev->of_node->full_name, data->gpio_enn);
        ret = -ENOENT;
        goto fsa4476_get_gpio_err;
    }

    if (!gpio_is_valid(data->gpio_enn)) {
        loge("gpio swtich_enn is unvalid!\n");
        ret = -ENOENT;
        goto fsa4476_get_gpio_err;
    }

    /* applay for swtich enn gpio */
    ret = gpio_request(data->gpio_enn, "swtich_fsa4476_enn");
    if (ret < 0) {
        loge("error request GPIO for swtich_enn fail %d\n", ret);
        goto fsa4476_get_gpio_err;
    }
    gpio_direction_output(data->gpio_enn, 0);

    /* get usb hs swith en1 gpio */
    data->gpio_en1 = of_get_named_gpio(dev->of_node, "swtich_en1", 0);
    if (data->gpio_en1 < 0) {
        loge(":Looking up %s property in node swtich_en1 failed %d\n"
            , dev->of_node->full_name, data->gpio_en1);
        ret = -ENOENT;
        goto fsa4476_get_gpio_switch_enn_err;
    }

    if (!gpio_is_valid(data->gpio_en1)) {
        loge("gpio en1 is unvalid!\n");
        ret = -ENOENT;
        goto fsa4476_get_gpio_switch_enn_err;
    }

    /* applay for usb hs swith en1 gpio */
    ret = gpio_request(data->gpio_en1, "swtich_fsa4476_en1");
    if (ret < 0) {
        loge("error request gpio_en1 fail %d\n", ret);
        goto fsa4476_get_gpio_switch_enn_err;
    }
    gpio_direction_output(data->gpio_en1, 0);

    /* get usb hs swith en2 gpio */
    data->gpio_en2 = of_get_named_gpio(dev->of_node, "swtich_en2", 0);
    if (data->gpio_en2 < 0) {
        loge(":Looking up %s property in node swtich_en2 failed %d\n"
            , dev->of_node->full_name, data->gpio_en2);
        ret = -ENOENT;
        goto fsa4476_get_gpio_en2_switch_err;
    }

    if (!gpio_is_valid(data->gpio_en2)) {
        loge("gpio_en2 is unvalid!\n");
        ret = -ENOENT;
        goto fsa4476_get_gpio_en2_switch_err;
    }

    /* applay for usb hs swith en2 gpio */
    ret = gpio_request(data->gpio_en2, "swtich_fsa4476_en2");
    if (ret < 0) {
        loge("error request GPIO for swtich_fsa4476_en2 fail %d\n", ret);
        goto fsa4476_get_gpio_en2_switch_err;
    }
    gpio_direction_output(data->gpio_en2, 0);

    /* get mic and gnd hung up gpio */
    data->mic_gnd_np= of_get_named_gpio(dev->of_node, "mic_gnd_np", 0);
    if (data->mic_gnd_np < 0) {
        logi(":Looking up %s property in node mic_gnd_np failed %d\n"
            , dev->of_node->full_name, data->mic_gnd_np);
        //if not use this gpio, set a invalid value -1
        data->mic_gnd_np = -1;
    } else {
        if (!gpio_is_valid(data->mic_gnd_np)) {
            loge("mic_gnd_np is unvalid!\n");
            ret = -ENOENT;
            goto mic_gnd_np_gpio_invalid_err;
        }

        /* applay for mic and gnd hung up gpio */
        ret = gpio_request(data->mic_gnd_np, "swtich_hs_mic_gnd_np");
        if (ret < 0) {
            loge("error request GPIO for swtich_hs_mic_gnd_np fail %d\n", ret);
            goto mic_gnd_np_gpio_invalid_err;
        }
        gpio_direction_output(data->mic_gnd_np, 1);
    }
    /* get mic gnd switch gpio */
    data->mic_gnd_switch= of_get_named_gpio(dev->of_node, "mic_gnd_switch", 0);
    if (data->mic_gnd_switch < 0) {
        logi(":Looking up %s property in node mic_gnd_switch failed %d\n"
            , dev->of_node->full_name, data->mic_gnd_switch);
        //if not use this gpio, set a invalid value -1
        data->mic_gnd_switch = -1;
    } else {
        if (!gpio_is_valid(data->mic_gnd_switch)) {
            loge("mic_gnd_switch is unvalid!\n");
            ret = -ENOENT;
            goto mic_gnd_switch_gpio_invalid_err;
        }

        /* applay for mic and gnd switch gpio */
        ret = gpio_request(data->mic_gnd_switch, "swtich_hs_mic_gnd_switch");
        if (ret < 0) {
            loge("error request GPIO for swtich_hs_mic_gnd_switch fail %d\n", ret);
            goto mic_gnd_switch_gpio_invalid_err;
        }
        gpio_direction_output(data->mic_gnd_switch, 1);
    }

    /*get sd gpio use config*/
    data->sd_gpio_used = of_get_named_gpio(dev->of_node, "sd_gpio_used", 0);
    if (data->sd_gpio_used < 0) {
        loge(":do not get sd_gpio_used config from dts, use default\n");
        data->sd_gpio_used = 0;
    }

    return 0;

mic_gnd_switch_gpio_invalid_err:
    if(data->mic_gnd_np > 0) {
        gpio_free(data->mic_gnd_np);
    }
mic_gnd_np_gpio_invalid_err:
    gpio_free(data->gpio_en2);
fsa4476_get_gpio_en2_switch_err:
    gpio_free(data->gpio_en1);
fsa4476_get_gpio_switch_enn_err:
    gpio_free(data->gpio_enn);
fsa4476_get_gpio_err:
    return ret;
}

static void usb_analog_hs_free_gpio(struct usb_ana_hs_fsa4476_data *data)
{
    IN_FUNCTION;

    if(data->gpio_enn > 0)
        gpio_free(data->gpio_enn);

    if(data->gpio_en1 > 0)
        gpio_free(data->gpio_en1);

    if(data->gpio_en2 > 0)
        gpio_free(data->gpio_en2);

    if(data->mic_gnd_np> 0)
        gpio_free(data->mic_gnd_np);

    if(data->mic_gnd_switch> 0)
        gpio_free(data->mic_gnd_switch);
}

#ifdef USB_ANALOG_HEADSET_DEBUG
static ssize_t usb_ana_hs_fsa4476_mic_switch_show(struct device *dev,
                      struct device_attribute *attr, char *buf)
{
    int value = 0;
    int count = 0;

    if (g_pdata_fsa4476 == NULL) {
        loge("pdata is NULL!\n");
        return count;
    }

    if (g_pdata_fsa4476->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("pdata is not registed!\n");
        return count;
    }

    value = usb_analog_hs_gpio_get_value(g_pdata_fsa4476->gpio_en2);

    return scnprintf(buf, MAX_BUF_SIZE, "%d\n", value);
}

static ssize_t usb_ana_hs_fsa4476_mic_switch_store(struct device *dev,
                    struct device_attribute *attr,
                    const char *buf, size_t count)
{


    int ret = 0;
    long val;

    if (g_pdata_fsa4476 == NULL) {
        loge("pdata is NULL!\n");
        return count;
    }

    if (g_pdata_fsa4476->registed == USB_ANALOG_HS_NOT_REGISTER) {
        loge("pdata is not registed!\n");
        return count;
    }

    ret = kstrtol(buf, 10, &val);
    if (ret < 0) {
        loge("input error!\n");
        return count;
    }

    if(val) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, 1);
        if(g_pdata_fsa4476->mic_gnd_switch> 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_switch, 1);
        }
    } else {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, 0);
        if(g_pdata_fsa4476->mic_gnd_switch> 0) {
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_switch, 0);
        }
    }

    return count;
}

static DEVICE_ATTR(mic_switch, 0660, usb_ana_hs_fsa4476_mic_switch_show,
                   usb_ana_hs_fsa4476_mic_switch_store);

static struct attribute *usb_ana_hs_fsa4476_attributes[] = {
    &dev_attr_mic_switch.attr,
    NULL
};

static const struct attribute_group usb_ana_hs_fsa4476_attr_group = {
    .attrs = usb_ana_hs_fsa4476_attributes,
};
#endif

static long usb_ana_hs_fsa4476_ioctl(struct file *file, unsigned int cmd,
                               unsigned long arg)
{
    int ret = 0;
    int gpio_mic_sel_val = 0;

    unsigned int __user *p_user = (unsigned int __user *) arg;

    if (g_pdata_fsa4476 == NULL)
        return -EBUSY;

    if (g_pdata_fsa4476->registed == USB_ANALOG_HS_NOT_REGISTER)
        return -EBUSY;

    switch (cmd) {
        case IOCTL_USB_ANA_HS_GET_MIC_SWITCH_STATE:
            gpio_mic_sel_val = usb_analog_hs_gpio_get_value(g_pdata_fsa4476->gpio_en2);
            logi("gpio_mic_sel_val = %d!\n", gpio_mic_sel_val);
            ret = put_user((__u32)(gpio_mic_sel_val),p_user);
            break;
        case IOCTL_USB_ANA_HS_GET_CONNECT_LINEIN_R_STATE:
            logi("connect_linein_r = %d!\n", g_pdata_fsa4476->connect_linein_r);
            ret = put_user((__u32)(g_pdata_fsa4476->connect_linein_r),p_user);
            break;
        case IOCTL_USB_ANA_HS_GND_FB_CONNECT:
            logi("usb analog hs fsa4476 ioctl gnd fb connect\n");
            #ifdef CONFIG_SUPERSWITCH_FSC
            if (set_superswitch_sbu_switch(Sbu_Cross_Close_Aux) < 0) { //SBU2 connect to HS_FB
                return -1;
            }
            #endif
            usb_analog_hs_fsa4476_enable(FSA4776_ENABLE);
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en1, 1);
            if(g_pdata_fsa4476->mic_gnd_np> 0) {
                usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_np, 0);
            }
            //usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, 1);
            break;
        case IOCTL_USB_ANA_HS_GND_FB_DISCONNECT:
            logi("usb analog hs fsa4476 ioctl gnd fb disconnect\n");
            #ifdef CONFIG_SUPERSWITCH_FSC
            if (set_superswitch_sbu_switch(Sbu_None) < 0) { //HS_FB disconnect
                return -1;
            }
            #endif
            usb_analog_hs_fsa4476_enable(FSA4776_DISABLE);
            usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en1, 0);
            if(g_pdata_fsa4476->mic_gnd_np> 0) {
                usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_np, 1);
            }
            //usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, 0);
            break;
        default:
            loge("unsupport cmd\n");
            ret = -EINVAL;
            break;
    }

    return (long)ret;
}

static const struct file_operations usb_ana_hs_fsa4476_fops = {
    .owner               = THIS_MODULE,
    .open                = simple_open,
    .unlocked_ioctl      = usb_ana_hs_fsa4476_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl        = usb_ana_hs_fsa4476_ioctl,
#endif
};

static struct miscdevice usb_ana_hs_fsa4476_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "usb_analog_hs",
    .fops   = &usb_ana_hs_fsa4476_fops,
};

struct usb_analog_hs_ops usb_ana_hs_fsa4476_ops = {
    .usb_ana_hs_check_headset_pluged_in = usb_ana_hs_fsa4476_check_hs_pluged_in,
    .usb_ana_hs_dev_register = usb_ana_hs_fsa4476_dev_register,
    .check_usb_ana_hs_support = check_usb_analog_hs_fsa4476_support,
    .usb_ana_hs_plug_in_out_handle = usb_ana_hs_fsa4476_plug_in_out_handle,
    .usb_ana_hs_mic_swtich_change_state = usb_ana_hs_fsa4476_mic_swtich_change_state,
};

int sd_gpio_config(void)
{
    unsigned int tmp_reg_value = 0;
    unsigned long *virtual_addr = (unsigned long *)ioremap(SD_GPIO_SCTRL_REG, sizeof(unsigned long));
    if (NULL == virtual_addr) {
        loge("sd gpio config fail\n");
        return -1;
    }
    tmp_reg_value = *(unsigned long *)virtual_addr | 0x4;
    *(unsigned long *)virtual_addr = tmp_reg_value;

    iounmap(virtual_addr);

    return 0;
}

static int usb_ana_hs_fsa4476_probe(struct platform_device *pdev)
{
    struct device *dev = &pdev->dev;
    struct device_node *node =  dev->of_node;
    const char *mic_switch_delay = "mic_switch_delay";
    const char *connect_linein_r = "connect_linein_r";
    int ret = 0;
    int val = 0;

    IN_FUNCTION;

    g_pdata_fsa4476 = kzalloc(sizeof(struct usb_ana_hs_fsa4476_data), GFP_KERNEL);
    if (NULL == g_pdata_fsa4476) {
        loge("cannot allocate anc hs dev data\n");
        return -ENOMEM;
    }

    ret = usb_ana_hs_fsa4476_load_gpio_pdata(dev, g_pdata_fsa4476);
    if (ret < 0){
        loge("get gpios failed, ret =%d\n",ret);
        goto fsa4476_err_out;
    }

    /* get mic swtich delay time */
    ret = of_property_read_u32(dev->of_node, mic_switch_delay, &g_pdata_fsa4476->mic_switch_delay_time);
    if (ret){
        logi("%s(%d): missing %s in dt node and set dafault value\n", __FUNCTION__,__LINE__, mic_switch_delay);
        g_pdata_fsa4476->mic_switch_delay_time = DEFAULT_MIC_SWITCH_DELAY_TIME;
    }
    logi("mic_switch_delay_time =%d\n", g_pdata_fsa4476->mic_switch_delay_time);

    if (g_pdata_fsa4476->sd_gpio_used == 1) {
        ret = sd_gpio_config();//sd gpio config
        if (ret < 0) {
            goto fsa4476_err_out;
        }
    }


    if (!of_property_read_u32(dev->of_node, connect_linein_r, &val)){
        if(0 == val) {
            g_pdata_fsa4476->connect_linein_r = false;
        } else {
            g_pdata_fsa4476->connect_linein_r = true;
        }
    } else {
        logi("%s(%d): connect_linein_r isn't in dt node and set dafault value: true\n", __FUNCTION__,__LINE__);
        g_pdata_fsa4476->connect_linein_r = true;
    }

    if (!of_property_read_u32(dev->of_node, "support_cc", &val)) {
        if (val) {
            g_pdata_fsa4476->support_cc = true;
        } else {
            g_pdata_fsa4476->support_cc = false;
        }
    } else {
        logi("%s(%d): support_cc isn't in dt node and set dafault value: false\n", __FUNCTION__,__LINE__);
        g_pdata_fsa4476->support_cc = false;
    }

    if (!of_property_read_u32(node, "using_superswitch", &val)) {
        if (val) {
            g_pdata_fsa4476->using_superswitch = true;
        } else {
            g_pdata_fsa4476->using_superswitch = false;
        }
    } else {
        g_pdata_fsa4476->using_superswitch = false;
    }

    if (!of_property_read_u32(node, "switch_antenna", &val)) {
        if (val) {
            g_pdata_fsa4476->switch_antenna = true;
        } else {
            g_pdata_fsa4476->switch_antenna = false;
        }
    } else {
        logi("%s(%d): switch_antenna isn't in dt node and set dafault value: false\n", __FUNCTION__,__LINE__);
        g_pdata_fsa4476->switch_antenna = false;
    }

    wake_lock_init(&g_pdata_fsa4476->wake_lock, WAKE_LOCK_SUSPEND, "usb_analog_hs");
    mutex_init(&g_pdata_fsa4476->mutex);

    /* load dts config for board difference */
    load_gpio_type_config(node);
    g_pdata_fsa4476->registed = USB_ANALOG_HS_NOT_REGISTER;
    g_pdata_fsa4476->usb_analog_hs_in = USB_ANA_HS_PLUG_OUT;

    if (g_pdata_fsa4476->using_superswitch) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_enn, FSA4776_DISABLE);
    } else {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_enn, FSA4776_ENABLE);
    }
    usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en1, 0);
    if(g_pdata_fsa4476->mic_gnd_np> 0) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_np, 1);
    }
    usb_analog_hs_gpio_set_value(g_pdata_fsa4476->gpio_en2, 0);
    if(g_pdata_fsa4476->mic_gnd_switch> 0) {
        usb_analog_hs_gpio_set_value(g_pdata_fsa4476->mic_gnd_switch, 0);
    }

    /* create workqueue */
    g_pdata_fsa4476->analog_hs_plugin_delay_wq =
        create_singlethread_workqueue("usb_analog_hs_plugin_delay_wq");
    if (!(g_pdata_fsa4476->analog_hs_plugin_delay_wq)) {
        loge("plugin wq create failed\n");
        ret = -ENOMEM;
        goto fsa4476_err_out;
    }
    INIT_DELAYED_WORK(&g_pdata_fsa4476->analog_hs_plugin_delay_work, usb_analog_hs_plugin_work);

    g_pdata_fsa4476->analog_hs_plugout_delay_wq =
        create_singlethread_workqueue("usb_analog_hs_plugout_delay_wq");
    if (!(g_pdata_fsa4476->analog_hs_plugout_delay_wq)) {
        loge("plugout wq create failed\n");
        ret = -ENOMEM;
        goto fsa4476_err_plugin_delay_wq;
    }
    INIT_DELAYED_WORK(&g_pdata_fsa4476->analog_hs_plugout_delay_work, usb_analog_hs_plugout_work);

    ret = usb_analog_hs_ops_register(&usb_ana_hs_fsa4476_ops);
    if (ret) {
        loge("register usb_ana_hs_fsa4476_ops ops failed!\n");
        goto fsa4476_err_plugin_delay_wq;
    }

    /* register misc device for userspace */
    ret = misc_register(&usb_ana_hs_fsa4476_device);
    if (ret) {
        loge("usb_analog_hs_device misc device register failed\n");
        goto fsa4476_err_misc_register;
    }

#ifdef USB_ANALOG_HEADSET_DEBUG
    ret = sysfs_create_group(&dev->kobj, &usb_ana_hs_fsa4476_attr_group);
    if (ret < 0)
        loge("failed to register sysfs\n");
#endif

    logi("usb_analog_hs probe success!\n");
    return 0;

fsa4476_err_misc_register:
    if (g_pdata_fsa4476->analog_hs_plugout_delay_wq) {
        cancel_delayed_work(&g_pdata_fsa4476->analog_hs_plugout_delay_work);
        flush_workqueue(g_pdata_fsa4476->analog_hs_plugout_delay_wq);
        destroy_workqueue(g_pdata_fsa4476->analog_hs_plugout_delay_wq);
    }
fsa4476_err_plugin_delay_wq:
    if (g_pdata_fsa4476->analog_hs_plugin_delay_wq) {
        cancel_delayed_work(&g_pdata_fsa4476->analog_hs_plugin_delay_work);
        flush_workqueue(g_pdata_fsa4476->analog_hs_plugin_delay_wq);
        destroy_workqueue(g_pdata_fsa4476->analog_hs_plugin_delay_wq);
    }
fsa4476_err_out:
    kfree(g_pdata_fsa4476);
    g_pdata_fsa4476 = NULL;

    return ret;

}

static int usb_ana_hs_fsa4476_remove(struct platform_device *pdev)
{
    logi("in remove\n");

    if (g_pdata_fsa4476 == NULL) {
        return 0;
    }

    if (g_pdata_fsa4476->analog_hs_plugin_delay_wq) {
        cancel_delayed_work(&g_pdata_fsa4476->analog_hs_plugin_delay_work);
        flush_workqueue(g_pdata_fsa4476->analog_hs_plugin_delay_wq);
        destroy_workqueue(g_pdata_fsa4476->analog_hs_plugin_delay_wq);
    }

    if (g_pdata_fsa4476->analog_hs_plugout_delay_wq) {
        cancel_delayed_work(&g_pdata_fsa4476->analog_hs_plugout_delay_work);
        flush_workqueue(g_pdata_fsa4476->analog_hs_plugout_delay_wq);
        destroy_workqueue(g_pdata_fsa4476->analog_hs_plugout_delay_wq);
    }

    usb_analog_hs_free_gpio(g_pdata_fsa4476);
    misc_deregister(&usb_ana_hs_fsa4476_device);

#ifdef USB_ANALOG_HEADSET_DEBUG
    sysfs_remove_group(&pdev->dev.kobj, &usb_ana_hs_fsa4476_attr_group);
#endif

    kfree(g_pdata_fsa4476);
    g_pdata_fsa4476 = NULL;

    return 0;
}

static struct platform_driver usb_ana_hs_fsa4476_driver = {
    .driver = {
        .name   = "usb_ana_hs_fsa4476",
        .owner  = THIS_MODULE,
        .of_match_table = usb_ana_hs_fsa4476_of_match,
    },
    .probe  = usb_ana_hs_fsa4476_probe,
    .remove = usb_ana_hs_fsa4476_remove,
};

static int __init usb_ana_hs_fsa4476_init(void)
{
    return platform_driver_register(&usb_ana_hs_fsa4476_driver);
}

static void __exit usb_ana_hs_fsa4476_exit(void)
{
    platform_driver_unregister(&usb_ana_hs_fsa4476_driver);
}

subsys_initcall_sync(usb_ana_hs_fsa4476_init);
module_exit(usb_ana_hs_fsa4476_exit);

MODULE_DESCRIPTION("usb analog headset swtich fsa4476 driver");
MODULE_LICENSE("GPL");
