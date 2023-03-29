/*
 * anc_ncx8293.c -- anc ncx8293 headset driver
 *
 * Copyright (c) 2014 Huawei Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
/*lint -e528 -e529 */
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
#include <media/huawei/hw_extern_pmic.h>

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#include <linux/hisi/hisi_adc.h>
#ifdef CONFIG_HUAWEI_DSM
#include <dsm_audio/dsm_audio.h>
#endif
#include "huawei_platform/audio/anc_ncx8293.h"

#define HWLOG_TAG anc_ncx8293

HWLOG_REGIST();
#ifdef CONFIG_HIFI_DSP_ONE_TRACK
extern int hifi_send_msg(unsigned int mailcode, void *data, unsigned int length);
#endif

enum anc_hs_mode {
    ANC_HS_CHARGE_OFF        = 0,
    ANC_HS_CHARGE_ON         = 1,
};

enum anc_ncx8293_irq_type {
    ANC_NCX8293_JACK_PLUG_IN  = 0,
    ANC_NCX8293_JACK_PLUG_OUT,
    ANC_NCX8293_BUTTON_PRESS,
    ANC_NCX8293_IQR_MAX,
};

#define  ANC_HS_LIMIT_MIN                  20
#define  ANC_HS_LIMIT_MAX                  200
#define  ANC_CHIP_STARTUP_TIME             30
#define  ADC_CALIBRATION_TIMES             10
#define  ADC_READ_COUNT                    3
#define  ADC_NORMAL_LIMIT_MIN              -500
#define  ADC_NORMAL_LIMIT_MAX              500
#define  ANC_HS_HOOK_MIN                   160
#define  ANC_HS_HOOK_MAX                   185
#define  ANC_HS_VOLUME_UP_MIN              205
#define  ANC_HS_VOLUME_UP_MAX              230
#define  ANC_HS_VOLUME_DOWN_MIN            240
#define  ANC_HS_VOLUME_DOWN_MAX            265

#define NO_BUTTON_PRESS                    (-1)

#define CODEC_GPIO_BASE                    (224)
#define I2C_REG_FAIL_MAX_TIMES             (10)
#define I2C_FAIL_REPORT_MAX_TIMES          (20)

/*#define ANC_BTN_MASK (SND_JACK_BTN_0)*/
#define ANC_BTN_MASK (SND_JACK_BTN_0 | SND_JACK_BTN_1 | SND_JACK_BTN_2)

struct anc_ncx8293_priv {
    struct i2c_client *client;
    struct device *dev;
    void *control_data;

    struct regmap *regmapL;
    struct regmap *regmapR;

    int anc_hs_mode; /* charge status */
    int anc_pwr_en_gpio; /* VBST_5V EN */

    int channel_pwl_h; /* adc channel for high voltage*/
    int channel_pwl_l; /* adc channel for low voltage*/
    int anc_hs_limit_min;
    int anc_hs_limit_max;

    int anc_hs_btn_hook_min_voltage;
    int anc_hs_btn_hook_max_voltage;
    int anc_hs_btn_volume_up_min_voltage;
    int anc_hs_btn_volume_up_max_voltage;
    int anc_hs_btn_volume_down_min_voltage;
    int anc_hs_btn_volume_down_max_voltage;

    bool irq_flag;
    bool boost_flag;

    int sleep_time; /* charge chip pre-charge time */
    bool mic_used; /* flag to show mic status */
    bool detect_again;
    int force_charge_ctl; /* force charge control for userspace*/
    int hs_micbias_ctl; /* hs micbias control*/

    int adc_calibration_base; /* calibration value*/
    int button_pressed;
    int headset_type;

    struct mutex btn_mutex;
    struct mutex charge_lock; /* charge status protect lock */
    struct mutex invert_hs_lock;
    struct wake_lock wake_lock;

    /*lint -save -e* */
    spinlock_t irq_lock;
    /*lint -restore*/

    int registered; /* anc hs regester flag */
    struct anc_hs_dev *anc_dev; /* anc hs dev */
    void *private_data; /* store codec decription data*/

    int gpio_int;
    int anc_ncx8293_irq;

    bool ldo_supply_used;
    bool cam_ldo_used;
    int cam_ldo_num;
    int cam_ldo_value;
    struct regulator *anc_hs_vdd;

    /* chip irq workqueue */
    struct workqueue_struct *anc_hs_plugin_delay_wq;
    struct delayed_work anc_hs_plugin_delay_work;

    struct workqueue_struct *anc_hs_plugout_delay_wq;
    struct delayed_work anc_hs_plugout_delay_work;

    struct workqueue_struct *anc_hs_btn_delay_wq;
    struct delayed_work anc_hs_btn_delay_work;

    struct workqueue_struct* anc_hs_invert_ctl_delay_wq;
    struct delayed_work anc_hs_invert_ctl_delay_work;
};

struct anc_ncx8293_priv *g_anc_ncx8293_priv;
static unsigned int i2c_reg_fail_times = 0;
static unsigned int i2c_fail_report_times = I2C_FAIL_REPORT_MAX_TIMES;

/*lint -save -e* */
static struct reg_default anc_ncx8293_reg[] = {
    { 0x00, 0x17 }, /* Device ID */
    { 0x01, 0x41 }, /* Device Setup */
    { 0x02, 0x00 }, /* Interrupt */
    { 0x03, 0x00 }, /* MIC Path Select */
    { 0x04, 0x00 }, /* DET Trigger */
    { 0x05, 0x00 }, /* Accessory Status */
    { 0x06, 0x03 }, /* Switch Status 1 */
    { 0x07, 0x00 }, /* Switch Status 2 */
    { 0x08, 0x03 }, /* Manual Switch Control 1 */
    { 0x09, 0x00 }, /* Manual Switch Control 2 */
    { 0x0A, 0x00 }, /* Key Press */
    { 0x0B, 0x03 }, /* Double Click Time */
    { 0x0C, 0x2e }, /* Threshold 1 */
    { 0x0D, 0x55 }, /* Threshold 2 */
    { 0x0E, 0x89 }, /* Threshold 3 */
    { 0x0F, 0x57 }, /* Threshold 4 */
    { 0x10, 0x7c }, /* ANC Upper Threshold */
    { 0x32, 0x11 }, /* ANC Key Debounce Threshold */
};
/*lint -restore*/

/*lint -save -e* */
static bool anc_ncx8293_volatile_register(struct device *dev, unsigned int reg)
{
    return true;
}
/*lint -restore*/

/*lint -save -e* */
static bool anc_ncx8293_readable_register(struct device *dev, unsigned int reg)
{
    return true;
}
/*lint -restore*/

/*lint -save -e* */
static inline int anc_ncx8293_get_value(int gpio)
{
    if (gpio >= CODEC_GPIO_BASE)
        return gpio_get_value_cansleep(gpio);
    else
        return gpio_get_value(gpio);
}
/*lint -restore*/

static inline void anc_ncx8293_gpio_set_value(int gpio, int value)
{
    if (gpio >= CODEC_GPIO_BASE)
        gpio_set_value_cansleep(gpio, value);
    else
        gpio_set_value(gpio, value);
}

/*lint -save -e* */
static inline void anc_hs_enable_irq(int irq)
{
    if (!g_anc_ncx8293_priv->irq_flag) {
        enable_irq(irq);
        g_anc_ncx8293_priv->irq_flag = true;
    }
}
/*lint -restore*/

/*lint -save -e* */
static inline void anc_hs_disable_irq(int irq)
{
    if (g_anc_ncx8293_priv->irq_flag) {
        disable_irq_nosync(irq);
        g_anc_ncx8293_priv->irq_flag = false;
    }
}
/*lint -restore*/

static void anc_dsm_i2c_reg_fail_report(void)
{
    i2c_reg_fail_times ++;
    if ((i2c_reg_fail_times > I2C_REG_FAIL_MAX_TIMES)
        && (i2c_fail_report_times > 0)) {
        i2c_reg_fail_times = 0;
        i2c_fail_report_times --;
#ifdef CONFIG_HUAWEI_DSM
	audio_dsm_report_info(AUDIO_ANC_HS, ANC_HS_I2C_ERR, "anc regmap i2c error.\n");
#endif
    }
    return;
}

static int anc_ncx8293_regmap_read(int reg, int *value)
{
    int ret = 0;

    /*lint -save -e* */
    ret = regmap_read(g_anc_ncx8293_priv->regmapL, reg, value);
    /*lint -restore*/
    if (ret < 0) {
        anc_dsm_i2c_reg_fail_report();
        hwlog_err("anc_ncx8293 regmap read error,%d\n", ret);
    }

    return ret;
}

static int anc_ncx8293_regmap_write(int reg, int value)
{
    int ret = 0;

    ret = regmap_write(g_anc_ncx8293_priv->regmapL, reg, value);
    if (ret < 0) {
        anc_dsm_i2c_reg_fail_report();
        hwlog_err("anc_ncx8293 regmap write error,%d\n", ret);
    }

    return ret;
}

static int anc_ncx8293_regmap_update_bits(int reg, int mask, int value)
{
    int ret = 0;

    ret = regmap_update_bits(g_anc_ncx8293_priv->regmapL, reg, mask, value);
    if (ret < 0) {
        anc_dsm_i2c_reg_fail_report();
        hwlog_err("anc_ncx8293 regmap update bits error,%d\n", ret);
    }

    return ret;
}

bool anc_ncx8293_check_headset_pluged_in(void);

static int force_clear_irq(void)
{
    int value = 0;
    int ret = 0;

    ret = anc_ncx8293_regmap_read(ANC_NCX8293_R002_INTERRUPT, &value);
    if (ret < 0) {
        hwlog_err("anc_ncx8293 force_clear_irq, read irq reg fail.\n");
        return ret;
    }
    hwlog_info("anc_ncx8293 force_clear_irq, irq reg : 0x%x.\n", value);

    if ((value & ANC_NCX8293_REMOVAL_IRQ_BIT) && !anc_ncx8293_check_headset_pluged_in()) {
        queue_delayed_work(g_anc_ncx8293_priv->anc_hs_plugout_delay_wq,
                           &g_anc_ncx8293_priv->anc_hs_plugout_delay_work,
                           0);
        hwlog_info("anc_ncx8293 force_clear_irq, plugout event.\n");
    }

    return ret;
}

static void anc_ncx8293_unmask_btn_irq(void)
{
    hwlog_info("anc_ncx8293 unmask btn irq.\n");

    force_clear_irq();

    anc_ncx8293_regmap_update_bits(ANC_NCX8293_R001_DEVICE_SETUP,
        ANC_NCX8293_KEY_PRESS_DET_DISABLE_BIT, 0x00);
}

static void anc_ncx8293_mask_btn_irq(void)
{
    hwlog_info("anc_ncx8293 mask btn irq.\n");

    anc_ncx8293_regmap_update_bits(ANC_NCX8293_R001_DEVICE_SETUP,
        ANC_NCX8293_KEY_PRESS_DET_DISABLE_BIT, ANC_NCX8293_KEY_PRESS_DET_DISABLE_BIT);
}

static void mic_bias_mode(void)
{
    hwlog_info("anc_ncx8293 go into mic_bias_mode.\n");
    if (NULL != g_anc_ncx8293_priv) {
        anc_ncx8293_regmap_write(ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1, ANC_NCX8293_MIC_BIAS_MODE_BIT);
        /* disconnect the hs mic & gnd. */
        anc_ncx8293_regmap_write(ANC_NCX8293_R001_DEVICE_SETUP, ANC_NCX8293_CTRL_OUTPUT_LOW_BIT);
    }
}

static void power_mode(void)
{
    hwlog_info("anc_ncx8293 go into power_mode.\n");
    if (NULL != g_anc_ncx8293_priv) {
        /* close the mic/pwr <--> mic_out path, to prevent high pulse on mbhc pin, it spend 1ms. */
        anc_ncx8293_regmap_update_bits(ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1, ANC_NCX8293_MIC_BIAS_MODE_BIT, 0x00);
        /*lint -save -e* */
        mdelay(CLOSE_MIC_OUT_PATH_TIME);
        /*lint -restore*/

        anc_ncx8293_regmap_write(ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1, ANC_NCX8293_POWER_MODE_BIT);
        /* disconnect the hs mic & gnd. */
        anc_ncx8293_regmap_write(ANC_NCX8293_R001_DEVICE_SETUP, ANC_NCX8293_CTRL_OUTPUT_LOW_BIT);
    }
}

static void idle_mode(void)
{
    hwlog_info("anc_ncx8293 go into idle_mode.\n");
    if (NULL != g_anc_ncx8293_priv) {
        /* close the mic/pwr <--> mic_out path,
         * close the mic/pwr <--> 5VCharge path */
        anc_ncx8293_regmap_update_bits(ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1, ANC_NCX8293_MIC_BIAS_MODE_BIT, 0x00);
        anc_ncx8293_regmap_update_bits(ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1, ANC_NCX8293_POWER_MODE_BIT, 0x00);
        /* connect the hs mic & gnd, to prevent pop when plug in. */
        anc_ncx8293_regmap_write(ANC_NCX8293_R001_DEVICE_SETUP, ANC_NCX8293_CTRL_OUTPUT_HIGH_BIT);
    }
}

static void anc_ncx8293_discharge(void)
{
    hwlog_info("ncx8293 chip begin discharge.\n");
    /* when change power_mode to mic_bias_mode,
     * mic/pwr pin switch to GND, to release the residual charge,
     * to prevent damage the Codec chip. */
    anc_ncx8293_regmap_write(ANC_NCX8293_R008_MANUAL_SWITCH_CONTROL1, ANC_NCX8293_GND_SL_SWITCH_BIT);
    /* when change power_mode to mic_bias_mode,
     * ANC hs has 30ms to supply power outside.
     * In this period of time, the hs mic is not work. */
    /*lint -save -e* */
    mdelay(ANC_HS_DISCHARGE_TIME);
    /*lint -restore*/
}

void anc_ncx8293_refresh_headset_type(int headset_type)
{
    if (NULL != g_anc_ncx8293_priv) {
        g_anc_ncx8293_priv->headset_type = headset_type;
        hwlog_info("ncx8293: refresh headset_type %d.", g_anc_ncx8293_priv->headset_type);
    }
}

static void anc_hs_invert_ctl_work(struct work_struct* work)
{
    wake_lock(&g_anc_ncx8293_priv->wake_lock);
    mutex_lock(&g_anc_ncx8293_priv->invert_hs_lock);

    if (ANC_HS_REVERT_4POLE == g_anc_ncx8293_priv->headset_type) {
        anc_ncx8293_regmap_write(ANC_NCX8293_R001_DEVICE_SETUP, ANC_NCX8293_CTRL_OUTPUT_HIGH_BIT);
        hwlog_info("anc_ncx8293: invert headset plugin, connect MIC and GND.");
    }

    mutex_unlock(&g_anc_ncx8293_priv->invert_hs_lock);
    wake_unlock(&g_anc_ncx8293_priv->wake_lock);
}

void anc_ncx8293_invert_headset_control(int connect)
{
    switch(connect) {
        case ANC_HS_MIC_GND_DISCONNECT:
            cancel_delayed_work(&g_anc_ncx8293_priv->anc_hs_invert_ctl_delay_work);
            flush_workqueue(g_anc_ncx8293_priv->anc_hs_invert_ctl_delay_wq);

            mutex_lock(&g_anc_ncx8293_priv->invert_hs_lock);
            anc_ncx8293_regmap_write(ANC_NCX8293_R001_DEVICE_SETUP, ANC_NCX8293_CTRL_OUTPUT_LOW_BIT);
            mutex_unlock(&g_anc_ncx8293_priv->invert_hs_lock);

            hwlog_info("anc_ncx8293: disconnect MIC and GND.\n");
            break;

        case ANC_HS_MIC_GND_CONNECT:
            queue_delayed_work(g_anc_ncx8293_priv->anc_hs_invert_ctl_delay_wq,
                               &g_anc_ncx8293_priv->anc_hs_invert_ctl_delay_work,
                               msecs_to_jiffies(3000));
            hwlog_info("anc_ncx8293_invert_headset_control: queue delay work.");
            break;

        default:
            hwlog_info("anc_ncx8293_invert_headset_control: unknown connect type.");
            break;
    }
}

/**
 * anc_hs_get_adc_delta
 *
 * get 3 times adc value with 1ms delay and use average value(delta) of it,
 * charge for it when delta is between anc_hs_limit_min and anc_hs_limit_max
 **/
/*lint -save -e* */
static int anc_ncx8293_get_adc_delta(void)
{
    int ear_pwr_h = 0, ear_pwr_l = 0;
    int delta = 0, count, fail_count = 0;
    int loop = ADC_READ_COUNT;
    int temp = 0;
    bool need_report = true;

    while (loop) {
        loop--;
        mdelay(1);
        ear_pwr_h = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_h);
        if (ear_pwr_h < 0) {
            hwlog_err("%s:get hkadc(h) fail, err:%d\n", __func__, ear_pwr_h);
            fail_count++;
            continue;
        }

        ear_pwr_l = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_l);
        if (ear_pwr_l < 0) {
            hwlog_err("%s:get hkadc(l) fail, err:%d\n", __func__, ear_pwr_l);
            fail_count++;
            continue;
        }
        hwlog_info("%s:adc_h:%d,adc_l:%d\n", __func__, ear_pwr_h, ear_pwr_l);

        temp = ear_pwr_h - ear_pwr_l
            - g_anc_ncx8293_priv->adc_calibration_base;

        /* if the adc value far away from normal value,
         * just abandon it
         */
        if ((temp > ADC_NORMAL_LIMIT_MAX)
            || (temp < ADC_NORMAL_LIMIT_MIN)) {
            fail_count++;
            need_report = false;
            continue;
        }

        delta += temp;
    }

    /* if adc value is out of rage, we make a dmd report */
    /*
        if (ear_pwr_h >= ADC_OUT_OF_RANGE || ear_pwr_l >= ADC_OUT_OF_RANGE) {
            anc_dsm_report(ANC_HS_ADC_FULL_ERR, 0);
        }
    */
    count = ADC_READ_COUNT - loop - fail_count;
    if (0 == count) {
        hwlog_err("%s:get anc_hs hkadc failed\n", __func__);
        /*
        if (need_report) {
            anc_dsm_report(ANC_HS_ADCH_READ_ERR, 0);
        }
        */
        return false;
    }
    /* compute an average value */
    delta /= count;
    hwlog_info("%s:final adc value= %d  count=%d\n", __func__, delta, count);

    return delta;
}
/*lint -restore*/

/**
 * anc_hs_get_btn_value - judge which button is pressed
 *
 *
 **/
static int anc_ncx8293_get_btn_value(void)
{
    int delta = 0;
    delta = anc_ncx8293_get_adc_delta();
    if ((delta >= g_anc_ncx8293_priv->anc_hs_btn_hook_min_voltage)
        && (delta <= g_anc_ncx8293_priv->anc_hs_btn_hook_max_voltage)) {
        return SND_JACK_BTN_0;
    } else if ((delta >= g_anc_ncx8293_priv->anc_hs_btn_volume_up_min_voltage)
               && (delta <= g_anc_ncx8293_priv->anc_hs_btn_volume_up_max_voltage)) {
        return SND_JACK_BTN_1;
    } else if ((delta >= g_anc_ncx8293_priv->anc_hs_btn_volume_down_min_voltage)
               && (delta <= g_anc_ncx8293_priv->anc_hs_btn_volume_down_max_voltage)) {
        return SND_JACK_BTN_2;
    } else {
        hwlog_err("[anc_ncx8293]btn delta not in range delta:%d\n", delta);
        /*anc_dsm_report(ANC_HS_BTN_NOT_IN_RANGE, 0);*/
        return NO_BUTTON_PRESS;
    }
}

static bool anc_ncx8293_btn_press(void)
{
    int value = 0;

    if (NULL == g_anc_ncx8293_priv)
        return false;

    anc_ncx8293_regmap_read(ANC_NCX8293_R00A_KEY_PRESS, &value);
    if ((value & ANC_NCX8293_KEY1_PRESS)
        || (value & ANC_NCX8293_KEY2_PRESS)
        || (value & ANC_NCX8293_KEY3_PRESS)) {
        return true;
    } else {
        return false;
    }
}

/**
 * anc_hs_btn_judge - delay work for anc headset irq
 *
 * @work: work struct
 *
 * should sync with codec power control, codec visit should
 * after codec_resume_lock(pdata->private_data, false).
 **/
static void anc_ncx8293_btn_judge(void)
{
    struct anc_hs_dev *pdev = g_anc_ncx8293_priv->anc_dev;
    struct anc_hs_codec_ops *fops = &pdev->ops;
    int btn_report = 0;

    if (!g_anc_ncx8293_priv->registered)
        return;

    hwlog_info("%s(%u):deal with button irq event!\n", __func__, __LINE__);

    /* should get wake lock before codec power lock which may be blocked*/
    mutex_lock(&g_anc_ncx8293_priv->btn_mutex);

    if (anc_ncx8293_btn_press()
        && (0 == g_anc_ncx8293_priv->button_pressed)) {
        /*button down event*/
        hwlog_info("%s(%u) : button down event !\n", __func__, __LINE__);
        /*lint -save -e* */
        mdelay(50);
        /*lint -restore*/
        btn_report = anc_ncx8293_get_btn_value();
        if (NO_BUTTON_PRESS != btn_report) {
            g_anc_ncx8293_priv->button_pressed = 1;
            /*lint -save -e* */
            fops->btn_report(btn_report, ANC_BTN_MASK);
            /*lint -restore*/
        } else {
            hwlog_warn("%s: it is not a button press.\n", __func__);
        }
    } else if (1 == g_anc_ncx8293_priv->button_pressed) {
        /*button up event*/
        hwlog_info("%s(%u) : button up event !\n", __func__, __LINE__);

        btn_report = 0;
        g_anc_ncx8293_priv->button_pressed = 0;

        /* we permit button up event report to userspace,
         * make sure down and up in pair
         */
        /*lint -save -e* */
        fops->btn_report(btn_report, ANC_BTN_MASK);
        /*lint -restore*/
    }

    mutex_unlock(&g_anc_ncx8293_priv->btn_mutex);

    return;
}

/**
 * anc_hs_send_hifi_msg - send hifi dsp message to set 3A parameter whether it is anc headset
 *
 *
 **/
static int anc_hs_send_hifi_msg(int anc_status)
{
    int ret = OK_RET;

    struct MLIBSetParaInfo *pMLIBSetParaInfo = (struct MLIBSetParaInfo *)kzalloc(sizeof(struct MLIBSetParaInfo)
                                             + MLIB_PARA_LENGTH_MAX, 1);
    struct MlibParameterVoice  *pPara_ANC_HS = NULL;

    if (NULL == pMLIBSetParaInfo) {
        hwlog_err("%s: kzalloc failed\n", __FUNCTION__);
        ret = ERROR_RET;// error return;
        return ret;
    }

    pMLIBSetParaInfo->msgID = ID_AP_AUDIO_MLIB_SET_PARA_IND;
    pMLIBSetParaInfo->uwPathID = MLIB_PATH_CS_VOICE_CALL_MICIN;
    pMLIBSetParaInfo->uwSize = MLIB_PARA_LENGTH_MAX;
    pMLIBSetParaInfo->reserve = 0;
    pMLIBSetParaInfo->uwModuleID = MLIB_MODULE_3A_VOICE;

    pPara_ANC_HS = (struct MlibParameterVoice *)pMLIBSetParaInfo->aucData;
    pPara_ANC_HS->key = MLIB_ANC_HS_PARA_ENABLE;
    pPara_ANC_HS->value = anc_status;
#ifdef CONFIG_HIFI_DSP_ONE_TRACK
    ret = hifi_send_msg(MAILBOX_MAILCODE_ACPU_TO_HIFI_MISC,
                        pMLIBSetParaInfo,
                        sizeof(struct MLIBSetParaInfo) + MLIB_PARA_LENGTH_MAX);
#endif
    kfree(pMLIBSetParaInfo);

    return ret;
}

/**
 * anc_hs_need_charge - judge whether it is anc headset
 *
 **/
static bool anc_ncx8293_need_charge(void)
{
    int delta = 0;
    /*lint -save -e* */
    mdelay(30);
    /*lint -restore*/
    delta = anc_ncx8293_get_adc_delta();
    if ((delta >= g_anc_ncx8293_priv->anc_hs_limit_min) &&
        (delta <= g_anc_ncx8293_priv->anc_hs_limit_max)) {
        hwlog_info("[%s][%d] anc headset = true\n", __func__, __LINE__);
        return true;
    } else {
        hwlog_info("[%s][%d] anc headset = false\n", __func__, __LINE__);
        return false;
    }
}

/**
 * anc_hs_adc_calibration - calibrate anc headset charge-circut
 *
 * make sure 5vboost is on and it is in float status before
 * call this function, if calibrate failed, set it as zero by default.
 **/
static void anc_ncx8293_adc_calibration(void)
{
    int loop = ADC_CALIBRATION_TIMES;
    int count, fail_count = 0;
    g_anc_ncx8293_priv->adc_calibration_base = 0;

    while (loop) {
        int adc_h, adc_l;
        loop--;
        usleep_range(1000, 1100);
        adc_h = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_h);
        if (adc_h < 0) {
            hwlog_err("[anc_hs]get adc fail,adc_h:%d\n", adc_h);
            fail_count++;
            continue;
        }
        adc_l = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_l);
        if (adc_l < 0) {
            hwlog_err("[anc_hs]get adc fail,adc_l:%d\n", adc_l);
            fail_count++;
            continue;
        }

        /* one calibrate value completely unnormal, abandon it*/
        if ((adc_h - adc_l < -100) || (adc_h - adc_l > 100)) {
            hwlog_err("[anc_hs]adc value is not expect, %d\n",
                adc_h - adc_l);
            fail_count++;
            continue;
        }
        g_anc_ncx8293_priv->adc_calibration_base += (adc_h - adc_l);
    }

    count = ADC_CALIBRATION_TIMES - loop - fail_count;
    if (0 == count) {
        /* if all adc read fail, set 0 to it as default*/
        g_anc_ncx8293_priv->adc_calibration_base = 0;
        hwlog_err("[anc_hs] calibration whole failed\n");
    } else {
        g_anc_ncx8293_priv->adc_calibration_base /= count;
        hwlog_info("anc_hs:calibration_base = %d with %d times\n",
            g_anc_ncx8293_priv->adc_calibration_base, count);

        if (g_anc_ncx8293_priv->adc_calibration_base > 50 ||
            g_anc_ncx8293_priv->adc_calibration_base < -50) {
            g_anc_ncx8293_priv->adc_calibration_base = 0;
            hwlog_err("[anc_hs] calibration value is not illegal, error occured\n");
        }
    }

    return;

}

/**
 * anc_hs_charge_judge - judge whether need charge for it
 *
 * get 3 times adc value with 1ms delay and use average value(delta) of it,
 * charge for it when delta is between anc_hs_limit_min and anc_hs_limit_max
 **/
static bool anc_ncx8293_charge_judge(void)
{
    struct anc_hs_dev *pdev = g_anc_ncx8293_priv->anc_dev;
    struct anc_hs_codec_ops *fops = &pdev->ops;

    /* userspace prohibit charging with using hs-mic pin */
    if (ANC_HS_DISABLE_CHARGE == g_anc_ncx8293_priv->force_charge_ctl) {
        hwlog_info("%s(%u) : charge is occupied by app level\n",
                   __func__, __LINE__);
        /* need second detect for charge*/
        g_anc_ncx8293_priv->detect_again = true;
        return false;
    }

    /* hs mic is using record, not take it */
    if (ANC_HS_DISABLE_CHARGE == g_anc_ncx8293_priv->hs_micbias_ctl) {
        /* need second detect for charge*/
        hwlog_info("%s(%u) :hs mic is in using!\n", __func__, __LINE__);
        g_anc_ncx8293_priv->detect_again = true;
        return false;
    }

    g_anc_ncx8293_priv->detect_again = false;
    hwlog_debug("%s(%u) : anc hs charge !\n", __func__, __LINE__);

    /* headset may have pluged out, just return */
    if (!fops->check_headset_in(g_anc_ncx8293_priv->private_data)) {
        hwlog_info("%s(%u) :headset has plug out!\n", __func__, __LINE__);
        return false;
    }

    mutex_lock(&g_anc_ncx8293_priv->charge_lock);
    /* connect 5vboost with hs_mic pin*/
    power_mode();
    mutex_unlock(&g_anc_ncx8293_priv->charge_lock);

    /* waiting for anc chip start up*/
    hwlog_info("%s: delay %d ms to wait anc chip up!\n",
               __func__, g_anc_ncx8293_priv->sleep_time);
    /*lint -save -e* */
    mdelay(g_anc_ncx8293_priv->sleep_time);
    /*lint -restore*/

    mutex_lock(&g_anc_ncx8293_priv->charge_lock);

    if ((ANC_HS_ENABLE_CHARGE == g_anc_ncx8293_priv->hs_micbias_ctl) &&
        anc_ncx8293_need_charge()) {
        /* start to charge for anc headset
         * and respond charging btn event
         */
        if (ANC_HS_CHARGE_OFF == g_anc_ncx8293_priv->anc_hs_mode) {
            hwlog_info("%s(%u) : anc_hs enable irq !\n", __func__, __LINE__);
            g_anc_ncx8293_priv->anc_hs_mode = ANC_HS_CHARGE_ON;
            anc_ncx8293_unmask_btn_irq();
        }

        if (ERROR_RET == anc_hs_send_hifi_msg(ANC_HS_CHARGE_ON)) {
            hwlog_err("%s(%u) : anc_hs_send_hifi_msg TURN ON ANC_HS return ERROR !\n", __func__, __LINE__);
        }
    } else {
        if (ANC_HS_CHARGE_ON == g_anc_ncx8293_priv->anc_hs_mode) {
            hwlog_info("%s(%u) : anc_hs disable irq !\n", __func__, __LINE__);
        }
        /* stop charge and change status to CHARGE_OFF*/
        anc_ncx8293_mask_btn_irq();
        anc_ncx8293_discharge();
        mic_bias_mode();
        /*lint -save -e* */
        udelay(500);
        /*lint -restore*/
        g_anc_ncx8293_priv->anc_hs_mode = ANC_HS_CHARGE_OFF;
        if (ERROR_RET == anc_hs_send_hifi_msg(ANC_HS_CHARGE_OFF)) {
            hwlog_err("%s(%u) : anc_hs_send_hifi_msg TURN OFF ANC_HS return ERROR !\n", __func__, __LINE__);
        }
    }

    mutex_unlock(&g_anc_ncx8293_priv->charge_lock);

    if (g_anc_ncx8293_priv->anc_hs_mode == ANC_HS_CHARGE_ON)
        return true;

    return false;
}

/**
 * update_charge_status - according to external control info to update
 * charge function
 *
 * get 3 times adc value with 1ms delay and use average value(delta) of it,
 * charge for it when delta is between anc_hs_limit_min and anc_hs_limit_max
 **/
static void update_charge_status(void)
{
    struct anc_hs_dev *pdev = g_anc_ncx8293_priv->anc_dev;
    struct anc_hs_codec_ops *fops = &pdev->ops;

    if (ANC_HS_DISABLE_CHARGE == g_anc_ncx8293_priv->hs_micbias_ctl ||
        ANC_HS_DISABLE_CHARGE == g_anc_ncx8293_priv->force_charge_ctl) {
        /* force stop charge function */
        mutex_lock(&g_anc_ncx8293_priv->charge_lock);

        if (ANC_HS_CHARGE_ON == g_anc_ncx8293_priv->anc_hs_mode) {
            anc_ncx8293_mask_btn_irq();
            anc_ncx8293_discharge();
            mic_bias_mode();
            /*lint -save -e* */
            udelay(500);
            /*lint -restore*/

            hwlog_info("%s(%u) : stop charging for anc hs !\n",
                __func__, __LINE__);
            g_anc_ncx8293_priv->anc_hs_mode = ANC_HS_CHARGE_OFF;
            g_anc_ncx8293_priv->mic_used = true;
        }

        mutex_unlock(&g_anc_ncx8293_priv->charge_lock);
    } else if (ANC_HS_ENABLE_CHARGE == g_anc_ncx8293_priv->hs_micbias_ctl &&
               ANC_HS_ENABLE_CHARGE == g_anc_ncx8293_priv->force_charge_ctl) {
        if (g_anc_ncx8293_priv->mic_used) {
            g_anc_ncx8293_priv->mic_used = false;
            /* headset maybe have plug out here */
            if (!fops->check_headset_in(g_anc_ncx8293_priv->private_data)) {
                hwlog_info("%s(%u) :headset has plug out!\n",
                    __func__, __LINE__);
            } else {
                /* force resume charge for anc headset */
                mutex_lock(&g_anc_ncx8293_priv->charge_lock);

                if (ANC_HS_CHARGE_OFF == g_anc_ncx8293_priv->anc_hs_mode) {
                    g_anc_ncx8293_priv->anc_hs_mode = ANC_HS_CHARGE_ON;
                    power_mode();
                    /*lint -save -e* */
                    udelay(500);
                    /*lint -restore*/
                    anc_ncx8293_unmask_btn_irq();

                    hwlog_info("%s(%u) : resume charging for anc hs!\n",
                        __func__, __LINE__);
                }

                mutex_unlock(&g_anc_ncx8293_priv->charge_lock);
            }
        }else if (g_anc_ncx8293_priv->detect_again) {
            /* need detect charge again due to interrupted before */
            anc_ncx8293_charge_judge();
        }
    }
}

/**
 * anc_hs_start_charge - call this to enbale 5vboost if support anc charge function
 *
 * make sure call this before headset sradc, the voltage stable time
 * should consider here, now 50ms need at least which is dependent
 * on hardware feature.
 **/
void anc_ncx8293_start_charge(void)
{
    if ((NULL == g_anc_ncx8293_priv)
        || !g_anc_ncx8293_priv->registered)
        return;

    hwlog_info("%s(%u) :enable 5vboost\n", __func__, __LINE__);

    /* default let hsbias connect to hs-mic pin*/
    mic_bias_mode();
    /* enable 5vboost first, this need time to be stable */
    anc_ncx8293_gpio_set_value(g_anc_ncx8293_priv->anc_pwr_en_gpio, 1);
    g_anc_ncx8293_priv->anc_hs_mode = ANC_HS_CHARGE_OFF;

    return;
}

/**
 * anc_hs_force_charge - when need hs-mic to record in charging status,
 * you must call this function to close charge
 *
 * @disable: charge control value, only enable and disable
 *
 **/
void anc_ncx8293_force_charge(int disable)
{
    if ((NULL == g_anc_ncx8293_priv)
        || !g_anc_ncx8293_priv->registered)
        return;

    /* don't make repeated switch with charge status*/
    if (disable == g_anc_ncx8293_priv->hs_micbias_ctl)
        return;

    g_anc_ncx8293_priv->hs_micbias_ctl = disable;

    /* update charge status here*/
    update_charge_status();

    return;
}

/**
 * anc_hs_charge_detect - detect whether plug-in headset is anc headset, if
 * it is anc headset, we will use mic-pin to charge with it
 *
 * @saradc_value: voltage number on mic-pin when headset plug in
 * @headset_type: headset type, only support 4-pole and 3-pole
 *
 * due to resistor difference , we should calibrate it first, then use
 * for 3-pole headset, don't make further judge.
 * (attention): revert 4-pole headset still need
 * 5vboost on to support second recognition
 **/
bool anc_ncx8293_charge_detect(int saradc_value, int headset_type)
{
    if ((g_anc_ncx8293_priv == NULL)
        || !g_anc_ncx8293_priv->registered)
        return false;

    /* calibration adc resistance which can
        make charge detect more accuracy*/
    anc_ncx8293_adc_calibration();

    g_anc_ncx8293_priv->headset_type = headset_type;

    /* revert 4-pole headset still need 5vboost on
        to support second recognition*/
    if (headset_type == ANC_HS_NORMAL_4POLE) {
        /* 4-pole headset maybe an anc headset*/
        hwlog_debug("%s : start anc hs charge judge\n", __func__);
        return anc_ncx8293_charge_judge();
    } else if (headset_type == ANC_HS_NORMAL_3POLE) {
        hwlog_info("%s : no disable 5vboost for 3-pole headset\n", __func__);
        /* 3-pole also support second-detect */
        /*
         *enable_boost(false);
         */
        return false;
    } else {
        return false;
    }
}

/**
 * anc_hs_stop_charge - call this function when headset plug out
 *
 **/
void anc_ncx8293_stop_charge(void)
{
    if ((g_anc_ncx8293_priv == NULL)
        || !g_anc_ncx8293_priv->registered)
        return;

    hwlog_info("%s : stop anc hs charge\n", __func__);

    anc_ncx8293_mask_btn_irq();
    idle_mode();
    anc_ncx8293_gpio_set_value(g_anc_ncx8293_priv->anc_pwr_en_gpio, 0);
    g_anc_ncx8293_priv->anc_hs_mode = ANC_HS_CHARGE_OFF;
    g_anc_ncx8293_priv->headset_type = ANC_HS_NONE;
    g_anc_ncx8293_priv->button_pressed = 0;

    if (ERROR_RET == anc_hs_send_hifi_msg(ANC_HS_CHARGE_OFF)) {
        hwlog_err("%s(%u) : anc_hs_send_hifi_msg TURN OFF ANC_HS return ERROR !\n", __func__, __LINE__);
    }

    return;
}

/**
 * anc_ncx8293_dev_register - call this function to support ncx8293,
 * this need hardware support
 *
 * @dev: anc_hs_dev function and data description
 * @codec_data: codec description which is need by callback function
 *
 * only support one codec to be registered, and all the callback
 * functions must be realized.
 **/
int anc_ncx8293_dev_register(struct anc_hs_dev *dev, void *codec_data)
{
    /* anc_hs driver not be probed, just return */
    /*lint -save -e* */
    if (NULL == g_anc_ncx8293_priv)
        return -ENODEV;
    /*lint -restore*/

    /* only support one codec to be registered */
    if (g_anc_ncx8293_priv->registered) {
        hwlog_err("one codec has registered, no more permit\n");
        /*lint -save -e* */
        return -EEXIST;
        /*lint -restore*/
    }

    if (!dev->ops.check_headset_in ||
        !dev->ops.btn_report ||
        !dev->ops.codec_resume_lock ||
        !dev->ops.plug_in_detect ||
        !dev->ops.plug_out_detect) {
        hwlog_err("codec ops funtion must be all registed\n");
        /*lint -save -e* */
        return -EINVAL;
        /*lint -restore*/
    }

    g_anc_ncx8293_priv->anc_dev = dev;
    g_anc_ncx8293_priv->private_data = codec_data;
    g_anc_ncx8293_priv->registered = true;

    force_clear_irq();
    hwlog_info("%s(%u) : anc hs has been register sucessful!\n", __func__, __LINE__);

    return 0;
}

bool check_anc_ncx8293_support(void)
{
    if ((NULL == g_anc_ncx8293_priv)
        || !g_anc_ncx8293_priv->registered)
        return false;
    else
        return true;
}

bool anc_ncx8293_plug_enable(void)
{
    if ((NULL == g_anc_ncx8293_priv)
        || !g_anc_ncx8293_priv->registered)
        return false;
    else
        return true;
}

bool anc_ncx8293_check_headset_pluged_in(void)
{
    int value = 0;
    if (NULL == g_anc_ncx8293_priv)
        return false;

    anc_ncx8293_regmap_read(ANC_NCX8293_R005_ACCESSORY_STATUS, &value);
    if (value & ANC_NCX8293_ATTACH_BIT)
        return true;
    else
        return false;
}

static int get_irq_type(void)
{
    int irq_type = ANC_NCX8293_IQR_MAX;
    int value = 0;

    anc_ncx8293_regmap_read(ANC_NCX8293_R002_INTERRUPT, &value);

    if ((value & ANC_NCX8293_INSERTION_IRQ_BIT)
        && anc_ncx8293_check_headset_pluged_in()) {
        irq_type = ANC_NCX8293_JACK_PLUG_IN;
    } else if ((value & ANC_NCX8293_REMOVAL_IRQ_BIT)
        && !anc_ncx8293_check_headset_pluged_in()) {
        irq_type = ANC_NCX8293_JACK_PLUG_OUT;
    } else if (value & ANC_NCX8293_KEY_PRESS_IRQ_BIT) {
        irq_type = ANC_NCX8293_BUTTON_PRESS;
    }

    hwlog_info("anc_ncx8293 irq value is 0x%x, type is %d.\n",
            value, irq_type);

    return irq_type;
}

static void anc_hs_plugin_work(struct work_struct *work)
{
    struct anc_hs_codec_ops *fops = &g_anc_ncx8293_priv->anc_dev->ops;

    wake_lock(&g_anc_ncx8293_priv->wake_lock);

    if (NULL != g_anc_ncx8293_priv->private_data)
        fops->plug_in_detect(g_anc_ncx8293_priv->private_data);

    wake_unlock(&g_anc_ncx8293_priv->wake_lock);
}

static void anc_hs_plugout_work(struct work_struct *work)
{
    struct anc_hs_codec_ops *fops = &g_anc_ncx8293_priv->anc_dev->ops;

    wake_lock(&g_anc_ncx8293_priv->wake_lock);

    if (NULL != g_anc_ncx8293_priv->private_data)
        fops->plug_out_detect(g_anc_ncx8293_priv->private_data);

    wake_unlock(&g_anc_ncx8293_priv->wake_lock);
}

static void anc_hs_btn_work(struct work_struct *work)
{
    wake_lock(&g_anc_ncx8293_priv->wake_lock);
    anc_ncx8293_btn_judge();
    wake_unlock(&g_anc_ncx8293_priv->wake_lock);
}

/**
 * anc_ncx8293_irq_handler - respond button irq while charging
 * for anc headset
 * @irq: irq number
 * @data: irq data, not used now
 *
 * disable irq will be better until delay_work to be scheduled.
 **/
static irqreturn_t anc_ncx8293_irq_handler(int irq, void *data)
{
    int irq_type = ANC_NCX8293_IQR_MAX;
    int value = 0;
    int loop = 5;

    if ((NULL == g_anc_ncx8293_priv)
        || !g_anc_ncx8293_priv->registered) {
        pr_info("anc_ncx8293 dev has not been regestered\n");
        anc_ncx8293_regmap_read(ANC_NCX8293_R002_INTERRUPT, &value);
        return IRQ_HANDLED;
    }

    wake_lock_timeout(&g_anc_ncx8293_priv->wake_lock,
                      msecs_to_jiffies(1000));
    irq_type = get_irq_type();

    switch (irq_type) {
        case ANC_NCX8293_JACK_PLUG_IN:
            queue_delayed_work(g_anc_ncx8293_priv->anc_hs_plugin_delay_wq,
                               &g_anc_ncx8293_priv->anc_hs_plugin_delay_work,
                               msecs_to_jiffies(800));
            break;
        case ANC_NCX8293_JACK_PLUG_OUT:
            queue_delayed_work(g_anc_ncx8293_priv->anc_hs_plugout_delay_wq,
                               &g_anc_ncx8293_priv->anc_hs_plugout_delay_work,
                               0);
            break;
        case ANC_NCX8293_BUTTON_PRESS:
            queue_delayed_work(g_anc_ncx8293_priv->anc_hs_btn_delay_wq,
                               &g_anc_ncx8293_priv->anc_hs_btn_delay_work,
                               msecs_to_jiffies(20));
            break;
        default:
            break;
    }

    while (0 < loop) {
        irq_type = get_irq_type();
        if (ANC_NCX8293_IQR_MAX == irq_type) {
            break;
        }

        if (ANC_NCX8293_JACK_PLUG_OUT == irq_type) {
            queue_delayed_work(g_anc_ncx8293_priv->anc_hs_plugout_delay_wq,
                               &g_anc_ncx8293_priv->anc_hs_plugout_delay_work,
                               0);
        }
        loop = loop - 1;
    }

    if (loop <= 0) {
#ifdef CONFIG_HUAWEI_DSM
	audio_dsm_report_info(AUDIO_ANC_HS, ANC_HS_UNHANDLED_IRQ, "there is irq unhandled in anc_max14744_irq_handler.\n");
#endif
    }

    return IRQ_HANDLED;
}

static int compute_final_voltage(void)
{
    int R1 = 51;
    int R2 = 100;
    int voltage1 = 0, voltage2 = 0;

    msleep(70);
    voltage1 = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_l);
    voltage2 = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_h);

    hwlog_info("adc_l value: %d adc_h value: %d\n", voltage1, voltage2);

    return (voltage1 * (R1 + R2)) / R1;
}

/**
 * anc_ncx8293_ioctl - ioctl interface for userspeace
 *
 * @file: file description
 * @cmd: control commond
 * @arg: arguments
 *
 * userspeace can get charge status and force control
 * charge status.
 **/
/*lint -save -e* */
static long anc_ncx8293_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int ret = 0;
    int charge_mode = ANC_HS_CHARGE_OFF;
    int voltage = 0;
    unsigned int __user *p_user = (unsigned int __user *)arg;
    int resistance_type = 0;

    if (!g_anc_ncx8293_priv->registered)
        return -EBUSY;

    switch (cmd) {
        case IOCTL_ANC_HS_CHARGE_ENABLE_CMD:
            if (ANC_HS_ENABLE_CHARGE == g_anc_ncx8293_priv->force_charge_ctl)
                break;

            /* resume anc headset charge*/
            g_anc_ncx8293_priv->force_charge_ctl = ANC_HS_ENABLE_CHARGE;
            hwlog_info("app level contrl set charge status with %d.\n",
                        g_anc_ncx8293_priv->force_charge_ctl);
            update_charge_status();
            break;
        case IOCTL_ANC_HS_CHARGE_DISABLE_CMD:
            if (ANC_HS_DISABLE_CHARGE == g_anc_ncx8293_priv->force_charge_ctl)
                break;

            /* force stop anc headset charge*/
            g_anc_ncx8293_priv->force_charge_ctl = ANC_HS_DISABLE_CHARGE;
            hwlog_info("app level contrl set charge status with %d.\n",
                        g_anc_ncx8293_priv->force_charge_ctl);
            update_charge_status();
            break;
        case IOCTL_ANC_HS_GET_HEADSET_CMD:
            charge_mode = g_anc_ncx8293_priv->anc_hs_mode;
            if (ANC_HS_CHARGE_ON == charge_mode) {
                if (!anc_ncx8293_need_charge()) {
                    charge_mode = ANC_HS_CHARGE_OFF;
                } else {
                    msleep(10);
                    if (!anc_ncx8293_need_charge())
                        charge_mode = ANC_HS_CHARGE_OFF;
                }
            }
            if (ANC_HS_CHARGE_ON == charge_mode)
                g_anc_ncx8293_priv->headset_type = ANC_HS_HEADSET;

            ret = put_user((__u32)(g_anc_ncx8293_priv->headset_type), p_user);
            break;
        case IOCTL_ANC_HS_GET_CHARGE_STATUS_CMD:
            ret = put_user((__u32)(g_anc_ncx8293_priv->anc_hs_mode), p_user);
            break;
        case IOCTL_ANC_HS_GET_VBST_5VOLTAGE_CMD:
            mic_bias_mode();
            anc_ncx8293_gpio_set_value(g_anc_ncx8293_priv->anc_pwr_en_gpio, 1);
            voltage = compute_final_voltage();
            anc_ncx8293_gpio_set_value(g_anc_ncx8293_priv->anc_pwr_en_gpio, 0);
            ret = put_user((__u32)voltage, p_user);
            break;
        case IOCTL_ANC_HS_GET_VDD_BUCK_VOLTAGE_CMD:
            mic_bias_mode();
            anc_ncx8293_gpio_set_value(g_anc_ncx8293_priv->anc_pwr_en_gpio, 0);
            voltage = compute_final_voltage();
            ret = put_user((__u32)voltage, p_user);
            break;
        case IOCTL_ANC_HS_GET_HEADSET_RESISTANCE_CMD:
            ret = put_user((__u32)resistance_type, p_user);
            break;
        default:
            hwlog_err("unsupport cmd\n");
            ret = -EINVAL;
            break;
    }

    return (long)ret;
}
#ifdef ANC_NCX8293_DEBUG
/*lint -restore*/

static ssize_t anc_ncx8293_reg_list_show(struct device *dev,
                                         struct device_attribute *attr,
                                         char *buf)
{
    int value = 0;
    int reg;
    char val_str[20];

    buf[0] = '\0';
    for (reg = 0; reg <= 0x0f; reg++) {
        anc_ncx8293_regmap_read(reg, &value);
        snprintf(val_str, sizeof(val_str), "0x%02x = 0x%02x\n", reg, value);

        strcat(buf, val_str);
    }
    return strlen(buf);
}

static ssize_t anc_ncx8293_adc_show(struct device *dev,
                                    struct device_attribute *attr,
                                    char *buf)
{
    char val_str[20] = {0};
    int ear_pwr_h = 0, ear_pwr_l = 0;

    if (NULL == buf) {
        hwlog_err("buf is null.\n");
        return -EINVAL;
    }
    buf[0] = '\0';

    ear_pwr_h = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_h);
    /*lint -save -e* */
    mdelay(50);
    /*lint -restore*/
    ear_pwr_l = hisi_adc_get_value(g_anc_ncx8293_priv->channel_pwl_l);
    /*lint -save -e* */
    mdelay(50);
    /*lint -restore*/
    snprintf(val_str, sizeof(val_str), "h = %d, l = %d\n", ear_pwr_h, ear_pwr_l);
    strcat(buf, val_str);

    return strlen(buf);
}

/*lint -save -e* */
static DEVICE_ATTR(reg_list, 0664, anc_ncx8293_reg_list_show, NULL);
static DEVICE_ATTR(adc, 0664, anc_ncx8293_adc_show, NULL);
/*lint -restore*/

static struct attribute *anc_ncx8293_attributes[] = {
    &dev_attr_reg_list.attr,
    &dev_attr_adc.attr,
    NULL
};

static const struct attribute_group anc_ncx8293_attr_group = {
    .attrs = anc_ncx8293_attributes,
};
#endif

/*lint -save -e* */
static const struct regmap_config anc_ncx8293_regmap = {
    .reg_bits            = 8,
    .val_bits            = 8,
    .max_register        = ANC_NCX8293_R032_ANC_KEY_DEBOUNCE_THRESHOLD,
    .reg_defaults        = anc_ncx8293_reg,
    .num_reg_defaults    = ARRAY_SIZE(anc_ncx8293_reg),
    .volatile_reg        = anc_ncx8293_volatile_register,
    .readable_reg        = anc_ncx8293_readable_register,
    .cache_type          = REGCACHE_RBTREE,
};
/*lint -restore*/

/*lint -save -e* */
static const struct file_operations anc_ncx8293_fops = {
    .owner            = THIS_MODULE,
    .open             = simple_open,
    .unlocked_ioctl   = anc_ncx8293_ioctl,
#ifdef CONFIG_COMPAT
    .compat_ioctl     = anc_ncx8293_ioctl,
#endif
};
/*lint -restore*/

/*lint -save -e* */
static struct miscdevice anc_ncx8293_device = {
    .minor  = MISC_DYNAMIC_MINOR,
    .name   = "anc_hs",
    .fops   = &anc_ncx8293_fops,
};
/*lint -restore*/

/* load dts config for board difference */
/*lint -save -e* */
static void load_anc_hs_config(struct device_node *node)
{
    int temp = 0;

    /* read adc channel number */
    if (!of_property_read_u32(node, "adc_channel_h", &temp))
        g_anc_ncx8293_priv->channel_pwl_h = temp;
    else
        g_anc_ncx8293_priv->channel_pwl_h = 15;
    if (!of_property_read_u32(node, "adc_channel_l", &temp))
        g_anc_ncx8293_priv->channel_pwl_l = temp;
    else
        g_anc_ncx8293_priv->channel_pwl_l = 14;
    /* read charge limit */
    if (!of_property_read_u32(node, "anc_hs_limit_min", &temp))
        g_anc_ncx8293_priv->anc_hs_limit_min = temp;
    else
        g_anc_ncx8293_priv->anc_hs_limit_min = ANC_HS_LIMIT_MIN;
    if (!of_property_read_u32(node, "anc_hs_limit_max", &temp))
        g_anc_ncx8293_priv->anc_hs_limit_max = temp;
    else
        g_anc_ncx8293_priv->anc_hs_limit_max = ANC_HS_LIMIT_MAX;
    /* read hook limit */
    if (!of_property_read_u32(node, "anc_hs_btn_hook_min_voltage", &temp))
        g_anc_ncx8293_priv->anc_hs_btn_hook_min_voltage = temp;
    else
        g_anc_ncx8293_priv->anc_hs_btn_hook_min_voltage = ANC_HS_HOOK_MIN;
    if (!of_property_read_u32(node, "anc_hs_btn_hook_max_voltage", &temp))
        g_anc_ncx8293_priv->anc_hs_btn_hook_max_voltage = temp;
    else
        g_anc_ncx8293_priv->anc_hs_btn_hook_max_voltage = ANC_HS_HOOK_MAX;
    /* read volume up limit */
    if (!of_property_read_u32(node, "anc_hs_btn_volume_up_min_voltage", &temp))
        g_anc_ncx8293_priv->anc_hs_btn_volume_up_min_voltage = temp;
    else
        g_anc_ncx8293_priv->anc_hs_btn_volume_up_min_voltage = ANC_HS_VOLUME_UP_MIN;
    if (!of_property_read_u32(node, "anc_hs_btn_volume_up_max_voltage", &temp))
        g_anc_ncx8293_priv->anc_hs_btn_volume_up_max_voltage = temp;
    else
        g_anc_ncx8293_priv->anc_hs_btn_volume_up_max_voltage = ANC_HS_VOLUME_UP_MAX;
    /* read volume down limit */
    if (!of_property_read_u32(node, "anc_hs_btn_volume_down_min_voltage", &temp))
        g_anc_ncx8293_priv->anc_hs_btn_volume_down_min_voltage = temp;
    else
        g_anc_ncx8293_priv->anc_hs_btn_volume_down_min_voltage = ANC_HS_VOLUME_DOWN_MIN;
    if (!of_property_read_u32(node, "anc_hs_btn_volume_down_max_voltage", &temp))
        g_anc_ncx8293_priv->anc_hs_btn_volume_down_max_voltage = temp;
    else
        g_anc_ncx8293_priv->anc_hs_btn_volume_down_max_voltage = ANC_HS_VOLUME_DOWN_MAX;
}
/*lint -restore*/

struct anc_hs_ops anc_ncx8293_ops = {
    .anc_hs_dev_register = anc_ncx8293_dev_register,
    .anc_hs_check_headset_pluged_in = anc_ncx8293_check_headset_pluged_in,
    .anc_hs_start_charge = anc_ncx8293_start_charge,
    .anc_hs_charge_detect = anc_ncx8293_charge_detect,
    .anc_hs_stop_charge = anc_ncx8293_stop_charge,
    .anc_hs_force_charge = anc_ncx8293_force_charge,
    .check_anc_hs_support = check_anc_ncx8293_support,
    .anc_hs_plug_enable = anc_ncx8293_plug_enable,
    .anc_hs_5v_control = NULL,
    .anc_hs_invert_hs_control = anc_ncx8293_invert_headset_control,
    .anc_hs_refresh_headset_type = anc_ncx8293_refresh_headset_type,
};

/*lint -save -e838*/
static int work_voltage_supply(struct anc_ncx8293_priv *di, struct device_node *np)
{
    int ret = 0;
    int val = 0;
    const char *ldo_supply_used_name = "ldo_supply_used";
    const char *anc_hs_vdd_name = "anc_hs_vdd";
    const char *cam_ldo_used_name = "cam_ldo_used";

    ret = of_property_read_u32(np, ldo_supply_used_name, &val);
    if (ret != 0) {
        hwlog_info("%s: can't get ldo_supply_used from dts, set defaut false value!!!\n", __func__);
        di->ldo_supply_used = false;
    } else {
        if (val) {
            di->ldo_supply_used = true;
            hwlog_info("%s: ldo_supply_used is true!\n", __func__);
        } else {
            di->ldo_supply_used = false;
            hwlog_info("%s: ldo_supply_used is false!\n", __func__);
        }
    }

    ret = of_property_read_u32(np, cam_ldo_used_name, &val);
    if (ret != 0) {
        hwlog_info("%s: can't get cam_ldo_used from dts, set defaut false value!!!\n", __func__);
        di->cam_ldo_used = false;
    } else {
        if (val) {
            di->cam_ldo_used = true;
            hwlog_info("%s: cam_ldo_used is true!\n", __func__);
        } else {
            di->cam_ldo_used = false;
            hwlog_info("%s: cam_ldo_used is false!\n", __func__);
        }
    }

    if (di->ldo_supply_used == true) {
        di->anc_hs_vdd = devm_regulator_get(di->dev, anc_hs_vdd_name);
        if (IS_ERR(di->anc_hs_vdd)) {
            hwlog_err("%s: Couldn't get anc_hs_vdd regulator ip %pK\n", __func__, di->anc_hs_vdd);
            di->anc_hs_vdd = NULL;
            goto err_out;
        }

        ret = regulator_set_voltage(di->anc_hs_vdd, 3300000, 3300000);
        if (ret) {
            hwlog_err("%s: Couldn't set anc_hs_vdd 3.3 voltage.\n", __func__);
            goto err_out;
        }

        ret = regulator_enable(di->anc_hs_vdd);
        if (ret) {
            hwlog_err("%s: Couldn't enable anc_hs_vdd ldo.\n", __func__);
            goto err_out;
        }
    } else if (di->cam_ldo_used == true) {
        ret = of_property_read_u32(np, "cam_ldo_num", &di->cam_ldo_num);
        if (ret) {
            di->cam_ldo_num = -EINVAL;
            hwlog_err("%s failed to get cam_ldo_num from device tree.\n", __func__);
            goto err_out;
        }

        ret = of_property_read_u32(np, "cam_ldo_value", &di->cam_ldo_value);
        if (ret) {
            di->cam_ldo_value = -EINVAL;

            hwlog_err("%s failed to get cam_ldo_value from device tree.\n", __func__);
            goto err_out;
        }

        ret = hw_extern_pmic_config(di->cam_ldo_num, di->cam_ldo_value, 1);
        if (ret) {
            hwlog_err("%s camera pmic LDO_%d power on fail.\n", __func__, di->cam_ldo_num+1);
            goto err_out;
        }
    } else {
        hwlog_info("%s: ldo_supply_used and cam_ldo_used are both false!\n", __func__);
    }

    return 0;

err_out:
    return -1;
}
/*lint -save +e838*/

static int create_irq_workqueue(struct i2c_client *client, struct anc_ncx8293_priv *di, struct device_node *np)
{
    int ret = 0;
#ifdef ANC_NCX8293_DEBUG
    ret = sysfs_create_group(&client->dev.kobj, &anc_ncx8293_attr_group);
    if (ret < 0)
        hwlog_err("failed to register sysfs\n");
#endif

    di->anc_hs_plugin_delay_wq =
        create_singlethread_workqueue("anc_hs_plugin_delay_wq");
    if (!(di->anc_hs_plugin_delay_wq)) {
        hwlog_err("%s : plugin wq create failed\n", __func__);
        /*lint -save -e* */
        ret = -ENOMEM;
        /*lint -restore*/
        goto err_out_sysfs;
    }
    INIT_DELAYED_WORK(&di->anc_hs_plugin_delay_work, anc_hs_plugin_work);

    di->anc_hs_plugout_delay_wq =
        create_singlethread_workqueue("anc_hs_plugout_delay_wq");
    if (!(di->anc_hs_plugout_delay_wq)) {
        hwlog_err("%s : plugout wq create failed\n", __func__);
        /*lint -save -e* */
        ret = -ENOMEM;
        /*lint -restore*/
        goto err_plugin_delay_wq;
    }
    INIT_DELAYED_WORK(&di->anc_hs_plugout_delay_work, anc_hs_plugout_work);

    di->anc_hs_btn_delay_wq =
        create_singlethread_workqueue("anc_hs_btn_delay_wq");
    if (!(di->anc_hs_btn_delay_wq)) {
        hwlog_err("%s : btn wq create failed\n", __func__);
        /*lint -save -e* */
        ret = -ENOMEM;
        /*lint -restore*/
        goto err_plugout_delay_wq;
    }
    INIT_DELAYED_WORK(&di->anc_hs_btn_delay_work, anc_hs_btn_work);

    di->anc_hs_invert_ctl_delay_wq =
        create_singlethread_workqueue("anc_hs_invert_ctl_delay_wq");
    if (!(di->anc_hs_invert_ctl_delay_wq)) {
        hwlog_err("%s : invert_ctl wq create failed\n", __FUNCTION__);
        ret = -ENOMEM;
        goto err_btn_delay_wq;
    }
    INIT_DELAYED_WORK(&di->anc_hs_invert_ctl_delay_work, anc_hs_invert_ctl_work);

    return ret;

err_btn_delay_wq:
    if (di->anc_hs_btn_delay_wq) {
        cancel_delayed_work(&di->anc_hs_btn_delay_work);
        flush_workqueue(di->anc_hs_btn_delay_wq);
        destroy_workqueue(di->anc_hs_btn_delay_wq);
    }

err_plugout_delay_wq:
    if (di->anc_hs_plugout_delay_wq) {
        cancel_delayed_work(&di->anc_hs_plugout_delay_work);
        flush_workqueue(di->anc_hs_plugout_delay_wq);
        destroy_workqueue(di->anc_hs_plugout_delay_wq);
    }

err_plugin_delay_wq:
    if (di->anc_hs_plugin_delay_wq) {
        cancel_delayed_work(&di->anc_hs_plugin_delay_work);
        flush_workqueue(di->anc_hs_plugin_delay_wq);
        destroy_workqueue(di->anc_hs_plugin_delay_wq);
    }

err_out_sysfs:
#ifdef ANC_NCX8293_DEBUG
    sysfs_remove_group(&client->dev.kobj, &anc_ncx8293_attr_group);
#endif
    return ret;
}

static int chip_init(struct i2c_client *client, struct anc_ncx8293_priv *di, struct device_node *np)
{
    int ret = 0;
    unsigned long flag = IRQF_ONESHOT | IRQF_NO_SUSPEND;

    di->anc_pwr_en_gpio = of_get_named_gpio(np, "gpio_pwr_en", 0);
    if (di->anc_pwr_en_gpio < 0) {
        hwlog_err("anc_pwr_en_gpio is invalid!\n");
        goto err_invert_ctl_delay_wq;
    }

    ret = gpio_request(di->anc_pwr_en_gpio, "anc_ncx8293_pwr_en");
    if (ret) {
        hwlog_err("error request GPIO for anc_pwr_en_gpio fail %d\n", ret);
        goto err_invert_ctl_delay_wq;
    }

    gpio_direction_output(di->anc_pwr_en_gpio, 0);

    di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
    if (di->gpio_int < 0) {
        hwlog_err("gpio_int is invalid!\n");
        goto err_gpio_pwr_en;
    }

    ret = gpio_request(di->gpio_int, "anc_ncx8293_int");
    if (ret) {
        hwlog_err("error request GPIO for gpio_int fail %d\n", ret);
        goto err_gpio_pwr_en;
    }

    /* set irq gpio to input status */
    gpio_direction_input(di->gpio_int);
    di->anc_ncx8293_irq = gpio_to_irq(di->gpio_int);

    /*anc ncx8293 irq request */
    flag |= IRQF_TRIGGER_FALLING;
    ret = request_threaded_irq(di->anc_ncx8293_irq, NULL,
                               anc_ncx8293_irq_handler,
                               flag, "anc_ncx8293_irq", NULL);

    if (ret < 0) {
        hwlog_err("anc_ncx8293_irq request fail: ret = %d\n", ret);
        goto err_out_gpio;
    }

    pr_info("anc_ncx8293 gpio_int:%d, gpio_pwr_en:%d\n", di->gpio_int, di->anc_pwr_en_gpio);

    ret = anc_hs_ops_register(&anc_ncx8293_ops);
    if (ret) {
        hwlog_err("register anc_hs_interface ops failed!\n");
        goto err_out_irq;
    }

    /* change the key debounce time from 60ms(default) to 30ms, to ensure the MMT-equipment test pass.
     * The MMT-equipment test ANC key, from key release to next key press, has 55ms latency time. */
    ret = anc_ncx8293_regmap_write(ANC_NCX8293_R032_ANC_KEY_DEBOUNCE_THRESHOLD, ANC_NCX8293_30ms_KEY_DEBOUNCE_TIME);
    if (ret < 0) {
        hwlog_err("%s: modify the key debounce time failed, the MMT-equipment may test fail.\n", __func__);
    }

    idle_mode();

    /* register misc device for userspace */
    ret = misc_register(&anc_ncx8293_device);
    if (ret) {
        hwlog_err("%s: anc_ncx8293 misc device register failed", __func__);
        goto err_out_irq;
    }
    return ret;

err_out_irq:
    free_irq(di->anc_ncx8293_irq, NULL);

err_out_gpio:
    gpio_free(di->gpio_int);

err_gpio_pwr_en:
    gpio_free(di->anc_pwr_en_gpio);

err_invert_ctl_delay_wq:
    if (di->anc_hs_invert_ctl_delay_wq) {
        cancel_delayed_work(&di->anc_hs_invert_ctl_delay_work);
        flush_workqueue(di->anc_hs_invert_ctl_delay_wq);
        destroy_workqueue(di->anc_hs_invert_ctl_delay_wq);
    }

    if (di->anc_hs_btn_delay_wq) {
        cancel_delayed_work(&di->anc_hs_btn_delay_work);
        flush_workqueue(di->anc_hs_btn_delay_wq);
        destroy_workqueue(di->anc_hs_btn_delay_wq);
    }

    if (di->anc_hs_plugout_delay_wq) {
        cancel_delayed_work(&di->anc_hs_plugout_delay_work);
        flush_workqueue(di->anc_hs_plugout_delay_wq);
        destroy_workqueue(di->anc_hs_plugout_delay_wq);
    }

    if (di->anc_hs_plugin_delay_wq) {
        cancel_delayed_work(&di->anc_hs_plugin_delay_work);
        flush_workqueue(di->anc_hs_plugin_delay_wq);
        destroy_workqueue(di->anc_hs_plugin_delay_wq);
    }

#ifdef ANC_NCX8293_DEBUG
    sysfs_remove_group(&client->dev.kobj, &anc_ncx8293_attr_group);
#endif

    return ret;

}

/**********************************************************
 *  Function:        anc_ncx8293_probe
 *  Description:     anc_ncx8293 module probe
 *  Parameters:      client:i2c_client
 *  id:              i2c_device_id
 *  return value:    0-sucess or others-fail
**********************************************************/
/*lint -save -e* */
static int anc_ncx8293_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret = -1;
    int val = 0;
    const char *chip_powered_on_time = "chip_powered_on_time";
    struct anc_ncx8293_priv *di = NULL;
    struct device_node *np = NULL;
    struct pinctrl *p;
    struct pinctrl_state *pinctrl_def;

    hwlog_info("anc_ncx8293_probe++\n");
    di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
    if (!di) {
        hwlog_err("anc_ncx8293_priv is NULL!\n");
        return -ENOMEM;
    }

    g_anc_ncx8293_priv = di;
    di->dev = &client->dev;
    np = di->dev->of_node;
    di->client = client;

    ret = work_voltage_supply(di, np);
    if (ret < 0) {
        goto err_out;
    }

    ret = of_property_read_u32(np, chip_powered_on_time, &val);
    if (ret) {
        hwlog_err("%s: fail to get chip_powered_on_time.\n", __func__);
        val = CHIP_DEFUALT_POWERED_TIME;
    }
    mdelay(val);

    i2c_set_clientdata(client, di);
    di->regmapL = regmap_init_i2c(client, &anc_ncx8293_regmap);
    if (IS_ERR(di->regmapL)) {
        ret = PTR_ERR(di->regmapL);
        hwlog_err("Failed to allocate regmapL: %d\n", ret);
        goto err_out;
    }

    ret = anc_ncx8293_regmap_read(ANC_NCX8293_R000_DEVICE_ID, &val);
    if (ret < 0) {
        hwlog_err("ncx8293 chip is not exist, stop the chip init.\n");
        ret = -ENXIO;
        goto err_out;
    }

    mutex_init(&di->charge_lock);
    mutex_init(&di->btn_mutex);
    mutex_init(&di->invert_hs_lock);
    spin_lock_init(&di->irq_lock);
    wake_lock_init(&di->wake_lock, WAKE_LOCK_SUSPEND, "anc_ncx8293");

    /* init all values */
    di->anc_hs_mode = ANC_HS_CHARGE_OFF;
    di->irq_flag = true;
    di->mic_used = false;
    di->sleep_time = ANC_CHIP_STARTUP_TIME;
    di->adc_calibration_base = 0;
    di->hs_micbias_ctl = ANC_HS_ENABLE_CHARGE;
    di->force_charge_ctl = ANC_HS_ENABLE_CHARGE;
    di->boost_flag = false;
    di->registered = false;
    di->headset_type = ANC_HS_NONE;
    load_anc_hs_config(np);

    p = devm_pinctrl_get(di->dev);
    hwlog_info("ncx8293:node name is %s\n", np->name);
    if (IS_ERR(p)) {
        hwlog_err("could not get pinctrl dev.\n");
        goto err_out;
    }
    pinctrl_def = pinctrl_lookup_state(p, "default");
    if (IS_ERR(pinctrl_def))
        hwlog_err("could not get defstate.\n");

    ret = pinctrl_select_state(p, pinctrl_def);
    if (ret)
        hwlog_err("could not set pins to default state.\n");

    ret = create_irq_workqueue(client, di, np);
    if (ret < 0)
        goto err_out;

    ret = chip_init(client, di, np);
    if(ret < 0)
        goto err_out;


#ifdef CONFIG_HUAWEI_HW_DEV_DCT
    /* detect current device successful, set the flag as present */
    set_hw_dev_flag(DEV_I2C_ANC_MAX14744);
#endif

    hwlog_info("anc_ncx8293_probe--\n");
    return 0;

err_out:
    if (ret < 0) {
        if (di->regmapL)
            regmap_exit(di->regmapL);
    }
    g_anc_ncx8293_priv = NULL;

    return ret;

}
/*lint -restore*/

/**********************************************************
*  Function:        anc_ncx8293_remove
*  Description:     anc_ncx8293 module remove
*  Parameters:      client:i2c_client
*  return value:    0-sucess or others-fail
**********************************************************/
/*lint -save -e* */
static int anc_ncx8293_remove(struct i2c_client *client)
{
    struct anc_ncx8293_priv *di = i2c_get_clientdata(client);
    int ret = -1;

    if (NULL == di)
        return 0;

    if (di && di->regmapL)
        regmap_exit(di->regmapL);

    if (di->anc_hs_plugin_delay_wq) {
        cancel_delayed_work(&di->anc_hs_plugin_delay_work);
        flush_workqueue(di->anc_hs_plugin_delay_wq);
        destroy_workqueue(di->anc_hs_plugin_delay_wq);
    }

    if (di->anc_hs_plugout_delay_wq) {
        cancel_delayed_work(&di->anc_hs_plugout_delay_work);
        flush_workqueue(di->anc_hs_plugout_delay_wq);
        destroy_workqueue(di->anc_hs_plugout_delay_wq);
    }

    if (di->anc_hs_btn_delay_wq) {
        cancel_delayed_work(&di->anc_hs_btn_delay_work);
        flush_workqueue(di->anc_hs_btn_delay_wq);
        destroy_workqueue(di->anc_hs_btn_delay_wq);
    }

    if (di->anc_hs_invert_ctl_delay_wq) {
        cancel_delayed_work(&di->anc_hs_invert_ctl_delay_work);
        flush_workqueue(di->anc_hs_invert_ctl_delay_wq);
        destroy_workqueue(di->anc_hs_invert_ctl_delay_wq);
    }

    if (di->ldo_supply_used) {
        ret = regulator_disable(di->anc_hs_vdd);
        if (ret)
            hwlog_err("%s: disable anc hs ldo failed.\n", __func__);
    }

    misc_deregister(&anc_ncx8293_device);
    hwlog_info("%s: exit\n", __func__);

    return 0;
}
/*lint -restore*/

/*lint -save -e* */
static struct of_device_id anc_ncx8293_of_match[] = {
    {
        .compatible = "huawei,anc_ncx8293",
        .data = NULL,
    },
    {
    },
};
/*lint -restore*/

/*lint -save -e* */
MODULE_DEVICE_TABLE(of, anc_ncx8293_of_match);
/*lint -restore*/

static const struct i2c_device_id anc_ncx8293_i2c_id[] = {
    {"anc_ncx8293", 0}, {},
};

/*lint -save -e* */
MODULE_DEVICE_TABLE(i2c, anc_ncx8293_i2c_id);
/*lint -restore*/

static struct i2c_driver anc_ncx8293_driver = {
    .probe                = anc_ncx8293_probe,
    .remove               = anc_ncx8293_remove,
    .id_table             = anc_ncx8293_i2c_id,
    .driver = {
        .owner            = THIS_MODULE,
        .name             = "anc_ncx8293",
        .of_match_table   = of_match_ptr(anc_ncx8293_of_match),
    },
};

/**********************************************************
*  Function:        anc_ncx8293_init
*  Description:     anc_ncx8293 module initialization
*  Parameters:      NULL
*  return value:    0-sucess or others-fail
**********************************************************/
static int __init anc_ncx8293_init(void)
{
    int ret = 0;

    ret =  i2c_add_driver(&anc_ncx8293_driver);
    if (ret)
        hwlog_err("%s: i2c_add_driver error!!!\n", __func__);

    return ret;
}
/**********************************************************
*  Function:        anc_ncx8293_exit
*  Description:     anc_ncx8293 module exit
*  Parameters:      NULL
*  return value:    NULL
**********************************************************/
static void __exit anc_ncx8293_exit(void)
{
    i2c_del_driver(&anc_ncx8293_driver);
}
/*lint -save -e* */
module_init(anc_ncx8293_init);
module_exit(anc_ncx8293_exit);
/*lint -restore*/

MODULE_DESCRIPTION("anc ncx8293 headset driver");
MODULE_LICENSE("GPL");
