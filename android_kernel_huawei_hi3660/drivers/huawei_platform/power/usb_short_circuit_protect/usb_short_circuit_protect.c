/*
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/of.h>
#include <linux/io.h>
#include <linux/jiffies.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/notifier.h>
#include <linux/wakelock.h>
#include <linux/hisi/usb/hisi_usb.h>
#include <linux/timer.h>
#include <linux/hrtimer.h>
#include <linux/hisi/hisi_adc.h>
#include <linux/delay.h>
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif
#include <huawei_platform/power/huawei_charger.h>
#include <huawei_platform/power/usb_short_circuit_protect.h>
#include <linux/thermal.h>

#define HWLOG_TAG usb_short_circuit_protect
HWLOG_REGIST();

static bool uscp_enable = true;

module_param(uscp_enable, bool, USCP_ENABLE_PAR);//uscp enable parameter 0644

static int usb_temp = USB_TEMP_NUM;//USB temperature 25 centigrade
module_param(usb_temp, int, USB_TEMP_PAR);//USB temperature parameter 0444

static bool uscp_enforce = false;
module_param(uscp_enforce, bool, USCP_ENFORCE_PAR);//uscp enforce parameter 0644

struct uscp_device_info
{
    struct device   *dev;
    struct workqueue_struct *uscp_wq;
    struct work_struct uscp_check_wk;
    struct notifier_block   usb_nb;
    struct notifier_block tcpc_nb;
    struct hrtimer timer;
    int gpio_uscp;
    int uscp_threshold_tusb;
    int open_mosfet_temp;
    int open_hiz_temp;
    int close_mosfet_temp;
    int interval_switch_temp;
    int check_interval;
    int keep_check_cnt;
    int dmd_hiz_enable;
};

static int protect_enable = 0;
static int protect_dmd_notify_enable = 1;
static int protect_dmd_notify_enable_hiz = DMD_NOTIFY_HIZ_ENABLE;
static int is_uscp_mode = 0;
static int is_hiz_mode = NOT_HIZ_MODE;
static int is_rt_uscp_mode = 0;
static unsigned int first_in = 1;
#ifdef CONFIG_DIRECT_CHARGER
static int is_scp_charger = 0;
#endif

static struct uscp_device_info* g_di = NULL;
static struct wake_lock uscp_wakelock;

#ifdef CONFIG_HUAWEI_POWER_DEBUG
static ssize_t uscp_dbg_show(void *dev_data, char *buf, size_t size)
{
	struct uscp_device_info *dev_p = (struct uscp_device_info *)dev_data;

	if (!dev_p) {
		hwlog_err("error: platform_get_drvdata return null!\n");
		return scnprintf(buf, size, "platform_get_drvdata return null!\n");
	}

	return scnprintf(buf, size,
		"uscp_threshold_tusb=%d\n" "open_mosfet_temp=%d\n" "close_mosfet_temp=%d\n" "interval_switch_temp=%d\n",
		dev_p->uscp_threshold_tusb,
		dev_p->open_mosfet_temp,
		dev_p->close_mosfet_temp,
		dev_p->interval_switch_temp);
}

static ssize_t uscp_dbg_store(void *dev_data, const char *buf, size_t size)
{
	struct uscp_device_info *dev_p = (struct uscp_device_info *)dev_data;
	unsigned int uscp_tusb = 0;
	unsigned int open_temp = 0;
	unsigned int close_temp = 0;
	unsigned int switch_temp = 0;

	if (!dev_p) {
		hwlog_err("error: platform_get_drvdata return null!\n");
		return -EINVAL;
	}

	if (sscanf(buf, "%d %d %d %d", &uscp_tusb, &open_temp, &close_temp, &switch_temp) != 4) {
		hwlog_err("error: unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	dev_p->uscp_threshold_tusb = uscp_tusb;
	dev_p->open_mosfet_temp = open_temp;
	dev_p->close_mosfet_temp = close_temp;
	dev_p->interval_switch_temp = switch_temp;

	hwlog_info("uscp_threshold_tusb=%d, open_mosfet_temp=%d, close_mosfet_temp=%d, interval_switch_temp=%d\n",
		dev_p->uscp_threshold_tusb,
		dev_p->open_mosfet_temp,
		dev_p->close_mosfet_temp,
		dev_p->interval_switch_temp);

	return size;
}
#endif

static void uscp_wake_lock(void)
{
    if(!wake_lock_active(&uscp_wakelock))
    {
        hwlog_info("wake lock\n");
        wake_lock(&uscp_wakelock);
    }
}

static void uscp_wake_unlock(void)
{
    if(wake_lock_active(&uscp_wakelock))
    {
        hwlog_info("wake unlock\n");
        wake_unlock(&uscp_wakelock);
    }
}

static void charge_type_handler(struct uscp_device_info* di, enum hisi_charger_type type)
{
    int interval = 0;

    if (!protect_enable)
        return;
    if ((CHARGER_TYPE_DCP == type) || (CHARGER_TYPE_UNKNOWN == type)
        ||(CHARGER_TYPE_SDP == type) ||(CHARGER_TYPE_CDP == type))
    {
        if (hrtimer_active(&(di->timer)))
        {
            hwlog_info("timer already armed , do nothing\n");
        }
        else
        {
            hwlog_info("start uscp check\n");
            interval = 0;
            first_in = 1;
            /*record 30 seconds after the charger just insert; 30s = (1100 - 1001 + 1)*300ms */
            di->keep_check_cnt = CHECK_CNT_INIT;//keep check count init number 1100
            hrtimer_start(&di->timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);
        }
    }
    else
    {
        hwlog_info("charger type = %d, do nothing\n", type);
    }
}

static int uscp_notifier_call(struct notifier_block *usb_nb, unsigned long event, void *data)
{
    struct uscp_device_info *di = container_of(usb_nb, struct uscp_device_info, usb_nb);
    enum hisi_charger_type type = ((enum hisi_charger_type)event);

    charge_type_handler(di, type);
    return NOTIFY_OK;
}
#ifdef CONFIG_TCPC_CLASS
static int pd_notifier_call(struct notifier_block *usb_nb, unsigned long event, void *data)
{
    return uscp_notifier_call(usb_nb, event, data);
}
#endif
static int usb_notifier_call(struct notifier_block *usb_nb, unsigned long event, void *data)
{
    return uscp_notifier_call(usb_nb, event, data);
}

#define USB_SHORT_NTC_SAMPLES  (3)
#define USB_SHORT_NTC_INVALID_TEMP_THRE  (10)

static int get_temperature_value(void)
{
    int i = 0;
    int j = 0;
    int ret = 0;
    struct thermal_zone_device *tz;

    int temp_array[USB_SHORT_NTC_SAMPLES] = {0};

    int temp_samples = 3;
    int temp_invalid_flag = 0;

    int sum = 0;

    tz = thermal_zone_get_zone_by_name("usb_port");
    if (IS_ERR(tz)) {
        hwlog_err("get uscp thermal zone fail\n");
        return 0;
    }

    while (temp_samples--)
    {
        temp_invalid_flag = 0;

        for (i = 0; i < USB_SHORT_NTC_SAMPLES; ++i)
        {
            ret = thermal_zone_get_temp(tz, &temp_array[i]);
            temp_array[i] = temp_array[i] / 1000;
            if (ret) {
                hwlog_err("error: get uscp temp fail!\n");
                temp_invalid_flag = 1;
            }

            hwlog_info("tusb adc value [%d]=%d\n", i, temp_array[i]);
        }

        if (temp_invalid_flag == 1) {
            continue;
        }

        /* check temperature value is valid */
        for (i = 0; i < (USB_SHORT_NTC_SAMPLES - 1); ++i)
        {
            for (j = (i + 1); j < USB_SHORT_NTC_SAMPLES; ++j)
            {
                if (abs(temp_array[i] - temp_array[j]) > USB_SHORT_NTC_INVALID_TEMP_THRE)
                {
                    hwlog_err("invalid temperature temp[%d]=%d temp[%d]=%d!\n", i, temp_array[i], j, temp_array[j]);
                    temp_invalid_flag = 1;
                    break;
                }
            }
        }

        if (temp_invalid_flag == 0) {
            break;
        }
    }

    if (temp_invalid_flag == 0)
    {
        /* get average temperature */
        for (i = 0; i < USB_SHORT_NTC_SAMPLES; ++i)
        {
            sum += temp_array[i];
        }
        return (sum/USB_SHORT_NTC_SAMPLES);
    }
    else
    {
        hwlog_err("use 0 as default temperature!\n");
        return 0;
    }
}
static void set_interval(struct uscp_device_info* di, int temp)
{
    if(NULL == di)
    {
        hwlog_err("di is NULL\n");
        return;
    }
    if (temp > di->interval_switch_temp) {
        di->check_interval = CHECK_INTERVAL_300;//set the check interval 300
        di->keep_check_cnt = 0;
        hwlog_info("cnt = %d!\n", di->keep_check_cnt);
    } else {
        if (di->keep_check_cnt > CHECK_CNT_LIMIT) {
            /*check the temperature per 0.3 second for 100 times ,when the charger just insert.*/
            hwlog_info("cnt = %d!\n", di->keep_check_cnt);
            di->keep_check_cnt -= 1;
            di->check_interval = CHECK_INTERVAL_300;
            is_uscp_mode = 0;
        } else if (di->keep_check_cnt == CHECK_CNT_LIMIT) {
            /* reset the flag when the temperature status is stable*/
            hwlog_info("cnt = %d!\n", di->keep_check_cnt);
            di->keep_check_cnt = -1;
            di->check_interval = CHECK_INTERVAL_10000;//set the check interval 10000
            is_uscp_mode = 0;
            uscp_wake_unlock();
        } else if (di->keep_check_cnt >= 0) {
            hwlog_info("cnt = %d!\n", di->keep_check_cnt);
            di->keep_check_cnt = di->keep_check_cnt + 1;
            di->check_interval = CHECK_INTERVAL_300;
        } else {
            di->check_interval = CHECK_INTERVAL_10000;
            is_uscp_mode = 0;
        }
    }
}
static void protection_process(struct uscp_device_info* di, int tbatt, int tusb)
{
    int ret = 0;
    int state = 0;
    int tdiff = 0;

    if(NULL == di)
    {
        hwlog_err("di is NULL\n");
        return;
    }
    if (!uscp_enable) {
        hwlog_info("uscp_enable=%d, usb current protect is disabled!\n", uscp_enable);
        return;
    }
    tdiff = tusb - tbatt;

    if ((tusb >= di->uscp_threshold_tusb) && (tdiff >= di->open_hiz_temp)){
        is_hiz_mode = HIZ_MODE;
        hwlog_err("enable charge hiz!\n");
        charge_set_hiz_enable(HIZ_MODE_ENABLE);
    }

    if (((tusb >= di->uscp_threshold_tusb) && (tdiff >= di->open_mosfet_temp)) || (uscp_enforce)) {
        uscp_wake_lock();
        is_uscp_mode = 1;
        is_rt_uscp_mode = 1;
        if (uscp_enforce)
            hwlog_err("uscp_enforce=%d, force usb circuit protect work!\n", uscp_enforce);
#ifdef CONFIG_DIRECT_CHARGER
        scp_set_stop_charging_flag(1);
        state = scp_get_stage_status();
        while(1) {
            state = scp_get_stage_status();
            if (is_scp_stop_charging_complete() &&
                ((SCP_STAGE_DEFAULT == state) ||(SCP_STAGE_CHARGE_DONE == state)))
                    break;
            }
        scp_set_stop_charging_flag(0);
        if (SCP_STAGE_DEFAULT == state) {
            if (first_in) {
                if (SCP_ADAPTOR_DETECT_SUCC == scp_adaptor_detect()) {
                    is_scp_charger = 1;
                } else {
                    is_scp_charger = 0;
                }
                first_in = 0;
            }
        } else if(SCP_STAGE_CHARGE_DONE == state) {
            is_scp_charger = 1;
        }else {
            /*do nothing*/
        }

        if(is_scp_charger) {
            ret = scp_adaptor_set_output_enable(0);
            if (!ret) {
                hwlog_err("disable scp adaptor output success!\n");
                msleep(SLEEP_200MS);
            } else {
                hwlog_err("disable scp adaptor output fail!\n");
            }
        }
#endif
        msleep(SLEEP_10MS);
        gpio_set_value(di->gpio_uscp, 1);/*open mosfet*/
        hwlog_err("pull up gpio_uscp!\n");
    } else if (tdiff <= di->close_mosfet_temp) {
#ifdef CONFIG_DIRECT_CHARGER
        if (is_scp_charger) {
            ret = scp_adaptor_set_output_enable(1);
            if (!ret) {
                hwlog_err("enable scp adaptor output success!\n");
            } else {
                    hwlog_err("enable scp adaptor output fail!\n");
            }
        }
#endif
        if (is_uscp_mode) {
            gpio_set_value(di->gpio_uscp, 0);/*close mosfet*/
            is_rt_uscp_mode = 0;
            msleep(SLEEP_10MS);
            charge_set_hiz_enable(HIZ_MODE_DISABLE);
            is_hiz_mode = NOT_HIZ_MODE;
            hwlog_info("pull down gpio_uscp!\n");
        }
        if(is_hiz_mode){
            charge_set_hiz_enable(HIZ_MODE_DISABLE);
            is_hiz_mode = NOT_HIZ_MODE;
            hwlog_info("disable charge hiz!\n");
        }
    } else {
        /*do nothing*/
    }
}
static void check_temperature(struct uscp_device_info* di)
{
    int tusb = 0;
    int tbatt = 0;
    int tdiff = 0;
    int batt_id = 0;

    if(NULL == di)
    {
        hwlog_err("di is NULL\n");
        return;
    }
    tusb = get_temperature_value();
    usb_temp = tusb;
    tbatt = hisi_battery_temperature();
    hwlog_info("tusb = %d, tbatt = %d\n", tusb, tbatt);
    tdiff = tusb - tbatt;

    if (di->dmd_hiz_enable) {
        if ((tusb >= di->uscp_threshold_tusb) && (tdiff >= di->open_hiz_temp)) {
            if (protect_dmd_notify_enable_hiz) {
                if (!dsm_client_ocuppy(power_dsm_get_dclient(POWER_DSM_USCP))) {
                    hwlog_info("record and notify open hiz temp\n");
                    dsm_client_record(power_dsm_get_dclient(POWER_DSM_USCP), "usb short happened,open hiz!\n");
                    dsm_client_notify(power_dsm_get_dclient(POWER_DSM_USCP), ERROR_NO_USB_SHORT_PROTECT_HIZ);
                    protect_dmd_notify_enable_hiz = DMD_NOTIFY_HIZ_DISABLE;
                }
            }
        }
    }

    if ((tusb >= di->uscp_threshold_tusb) && (tdiff >= di->open_mosfet_temp)) {
        is_rt_uscp_mode = 1;
        if (protect_dmd_notify_enable) {
            if (!dsm_client_ocuppy(power_dsm_get_dclient(POWER_DSM_USCP))) {
                batt_id = hisi_battery_id_voltage();
                hwlog_info("record and notify\n");
                dsm_client_record(power_dsm_get_dclient(POWER_DSM_USCP), "usb short happened,tusb = %d,tbatt = %d,batt_id = %d\n",
                    tusb,tbatt,batt_id);
                dsm_client_notify(power_dsm_get_dclient(POWER_DSM_USCP), ERROR_NO_USB_SHORT_PROTECT);
                protect_dmd_notify_enable = 0;
            }
        }
    }

    set_interval(di, tdiff);
    protection_process(di, tbatt, tusb);
}
int is_in_uscp_mode(void)
{
    return is_uscp_mode;
}
int is_uscp_hiz_mode(void)
{
    return is_hiz_mode;
}
int is_in_rt_uscp_mode(void)
{
    return is_rt_uscp_mode;
}
int get_usb_ntc_temp(void)
{
    return get_temperature_value();
}
static void uscp_check_work(struct work_struct *work)
{
    struct uscp_device_info *di = container_of(work,struct uscp_device_info, uscp_check_wk);
    int interval = 0;
    enum hisi_charger_type type = hisi_get_charger_type();
#ifdef CONFIG_DIRECT_CHARGER
    if ((-1 == di->keep_check_cnt) && (CHARGER_TYPE_NONE == type) &&
        SCP_STAGE_DEFAULT == scp_get_stage_status())
#else
    if ((-1 == di->keep_check_cnt) && (CHARGER_TYPE_NONE == type))
#endif
    {
        protect_dmd_notify_enable = 1;
        gpio_set_value(di->gpio_uscp, 0);/*close mosfet*/
        di->keep_check_cnt = -1;
        di->check_interval = CHECK_INTERVAL_10000;
        is_uscp_mode = 0;
        di->keep_check_cnt = CHECK_CNT_INIT;/*check count init number 1100*/
        first_in = 1;
#ifdef CONFIG_DIRECT_CHARGER
        is_scp_charger = 0;
#endif
        hwlog_info("chargertype is %d,stop checking\n", type);
        return;
    }

    check_temperature(di);
    interval = di->check_interval;
    hrtimer_start(&di->timer, ktime_set(interval/MSEC_PER_SEC, (interval % MSEC_PER_SEC) * USEC_PER_SEC), HRTIMER_MODE_REL);

}

static enum hrtimer_restart uscp_timer_func(struct hrtimer *timer)
{
    struct uscp_device_info *di;

    di = container_of(timer, struct uscp_device_info, timer);
    queue_work(di->uscp_wq, &di->uscp_check_wk);
    return HRTIMER_NORESTART;
}

static void check_ntc_error(void)
{
    int temp = 0;
    int sum = 0;
    int i = 0;
    int tbatt = INVALID_TEMP_VAL;
    int batt_id = 0;

    for (i = 0; i < GET_TEMP_VAL_NUM; ++i)
    {
        sum += get_temperature_value();
    }
    temp = sum / GET_TEMP_VAL_NUM;
    hwlog_info("check ntc error, temp = %d\n", temp);
    if (temp > CHECK_NTC_TEMP_MAX || temp < CHECK_NTC_TEMP_MIN)
    {
        if (!dsm_client_ocuppy(power_dsm_get_dclient(POWER_DSM_USCP)))
        {
            tbatt = hisi_battery_temperature();
            batt_id = hisi_battery_id_voltage();
            hwlog_info("ntc error notify\n");
            dsm_client_record(power_dsm_get_dclient(POWER_DSM_USCP), "ntc error happened,tusb = %d,tbatt = %d,batt_id = %d\n",
                temp,tbatt,batt_id);
            dsm_client_notify(power_dsm_get_dclient(POWER_DSM_USCP), ERROR_NO_USB_SHORT_PROTECT_NTC);
        }
        protect_enable = 0;
    }
    else
    {
        hwlog_info("enable usb short protect\n");
        protect_enable = 1;
    }
}
static int uscp_parse_dts(struct device_node* np, struct uscp_device_info* di)
{
    int ret;

    ret = of_property_read_u32(np, "uscp_threshold_tusb", &(di->uscp_threshold_tusb));
    if (ret)
    {
        di->uscp_threshold_tusb = DEFAULT_TUSB_THRESHOLD;
        hwlog_err("get uscp_threshold_tusb info fail!use default threshold = %d\n",di->uscp_threshold_tusb);
    }
    hwlog_info("uscp_threshold_tusb = %d\n", di->uscp_threshold_tusb);

    ret = of_property_read_u32(np, "open_mosfet_temp", &(di->open_mosfet_temp));
    if (ret)
    {
        hwlog_err("get open_mosfet_temp info fail!\n");
        return -EINVAL;
    }
    hwlog_info("open_mosfet_temp = %d\n", di->open_mosfet_temp);
    ret = of_property_read_u32(np, "open_hiz_temp", &(di->open_hiz_temp));
    if (ret)
    {
        di->open_hiz_temp = di->open_mosfet_temp;
        hwlog_err("get open_hiz_temp info fail,use default open_mosfet_temp!\n");
    }
    hwlog_info("open_hiz_temp = %d\n", di->open_hiz_temp);
    ret = of_property_read_u32(np, "dmd_hiz_enable", &(di->dmd_hiz_enable));
    if (ret)
    {
        di->dmd_hiz_enable = DMD_HIZ_DISABLE;
        hwlog_err("get dmd_hiz_enable info fail,use value zero!\n");
    }
    hwlog_info("dmd_hiz_enable = %d\n", di->dmd_hiz_enable);
    ret = of_property_read_u32(np, "close_mosfet_temp", &(di->close_mosfet_temp));
    if (ret)
    {
        hwlog_err("get close_mosfet_temp info fail!\n");
        return -EINVAL;
    }
    hwlog_info("close_mosfet_temp = %d\n", di->close_mosfet_temp);
    ret = of_property_read_u32(np, "interval_switch_temp", &(di->interval_switch_temp));
    if (ret)
    {
        hwlog_err("get interval_switch_temp info fail!\n");
        return -EINVAL;
    }
    hwlog_info("interval_switch_temp = %d\n", di->interval_switch_temp);
    return 0;
}
static int uscp_probe(struct platform_device *pdev)
{
    struct device_node* np;
    struct uscp_device_info* di;
    enum hisi_charger_type type = hisi_get_charger_type();
    int ret = 0;

    np = pdev->dev.of_node;
    if(NULL == np)
    {
        hwlog_err("np is NULL\n");
        return -1;
    }
    di = kzalloc(sizeof(*di), GFP_KERNEL);
    if (!di)
    {
        hwlog_err("di is NULL\n");
        return -ENOMEM;

    }
    g_di = di;
    platform_set_drvdata(pdev, di);
    ret = uscp_parse_dts(np, di);
    if (ret)
    {
        hwlog_err("could not parse dts gpio_uscp\n");
        goto free_mem;
    }

    is_uscp_mode = 0;
    is_rt_uscp_mode = 0;
    di->keep_check_cnt = CHECK_CNT_INIT;

    di->gpio_uscp = of_get_named_gpio(np, "gpio_usb_short_circuit_protect",0);
    if (!gpio_is_valid(di->gpio_uscp))
    {
        hwlog_err("gpio_uscp is not valid\n");
        ret = -EINVAL;
        goto free_mem;
    }
    hwlog_info("gpio_uscp = %d\n", di->gpio_uscp);

    ret = gpio_request(di->gpio_uscp, "usb_short_circuit_protect");
    if (ret)
    {
        hwlog_err("could not request gpio_uscp\n");
        ret = -EINVAL;
        goto free_mem;
    }
    gpio_direction_output(di->gpio_uscp, 0);

    check_ntc_error();
    if (!is_hisi_battery_exist()) {
        hwlog_err("battery is not exist, disable usb short protect!\n");
        protect_enable = 0;
    }
    if (!protect_enable)
    {
        goto free_gpio;
    }
    wake_lock_init(&uscp_wakelock, WAKE_LOCK_SUSPEND, "usb_short_circuit_protect_wakelock");
    di->uscp_wq = create_singlethread_workqueue("usb_short_circuit_protect_wq");
    INIT_WORK(&di->uscp_check_wk, uscp_check_work);
    hrtimer_init(&di->timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
    di->timer.function = uscp_timer_func;

#ifdef CONFIG_TCPC_CLASS
    if(is_pd_supported()) {
        di->tcpc_nb.notifier_call = pd_notifier_call;
        ret = register_pd_dpm_notifier(&di->tcpc_nb);
    } else {
#endif
        di->usb_nb.notifier_call = usb_notifier_call;
        ret = hisi_charger_type_notifier_register(&di->usb_nb);
#ifdef CONFIG_TCPC_CLASS
    }
#endif
    if (ret < 0)
    {
        hwlog_err("charger_type_notifier_register failed\n");
        ret = -EINVAL;
        goto fail_free_wakelock;
    }
    charge_type_handler(di, type);

#ifdef CONFIG_HUAWEI_POWER_DEBUG
	power_dbg_ops_register("uscp_para", platform_get_drvdata(pdev),
		(power_dgb_show)uscp_dbg_show, (power_dgb_store)uscp_dbg_store);
#endif

    hwlog_info("uscp probe ok!\n");
    return 0;

fail_free_wakelock:
	wake_lock_destroy(&uscp_wakelock);
free_gpio:
    gpio_free(di->gpio_uscp);
free_mem:
    platform_set_drvdata(pdev, NULL);
    kfree(di);
    g_di = NULL;
    return ret;
}

#ifdef CONFIG_PM
static int usb_short_circuit_protect_suspend(struct platform_device *pdev, pm_message_t state)
{
    struct uscp_device_info* di = platform_get_drvdata(pdev);
    if(NULL == di)
    {
        hwlog_err("%s:di is NULL\n", __func__);
        return 0;
    }
    hwlog_info("%s:+\n", __func__);
    cancel_work_sync(&di->uscp_check_wk);
    hrtimer_cancel(&di->timer);
    hwlog_info("%s:-\n", __func__);
    return 0;
}
static int usb_short_circuit_protect_resume(struct platform_device *pdev)
{
    enum hisi_charger_type type = hisi_get_charger_type();
    struct uscp_device_info* di = platform_get_drvdata(pdev);
    if(NULL == di)
    {
        hwlog_err("%s:di is NULL\n", __func__);
        return 0;
    }
    if( CHARGER_TYPE_NONE == type )
    {
        hwlog_info("%s:charger type = %d\n", __func__,type);
        return 0;
    }
    hwlog_info("%s:+ charger type = %d\n", __func__,type);
    queue_work(di->uscp_wq, &di->uscp_check_wk);
    hwlog_info("%s:-\n", __func__);
    return 0;
}
#endif

static struct of_device_id uscp_match_table[] =
{
    {
        .compatible = "huawei,usb_short_circuit_protect",
        .data = NULL,
    },
    {
    },
};
static struct platform_driver uscp_driver = {
    .probe = uscp_probe,
#ifdef CONFIG_PM
    /*depend on IPC driver,so we set SR suspend/resume and IPC is suspend_late/early_resume*/
    .suspend = usb_short_circuit_protect_suspend,
    .resume = usb_short_circuit_protect_resume,
#endif
    .driver = {
        .name = "huawei,usb_short_circuit_protect",
        .owner = THIS_MODULE,
        .of_match_table = of_match_ptr(uscp_match_table),
    },
};

static int __init uscp_init(void)
{
    return platform_driver_register(&uscp_driver);
}

device_initcall_sync(uscp_init);

static void __exit uscp_exit(void)
{
    platform_driver_unregister(&uscp_driver);
}

module_exit(uscp_exit);

MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:uscp");
MODULE_AUTHOR("HUAWEI Inc");
