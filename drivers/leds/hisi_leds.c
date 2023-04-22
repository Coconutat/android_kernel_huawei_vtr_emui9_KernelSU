/*
 * LEDs driver for hisi
 *
 * Copyright (c) 2011-2013 Hisilicon Technologies CO., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/ioport.h>
#include <linux/gpio.h>
#include <linux/hisi/hisi_leds.h>
#include <linux/io.h>
#include <linux/clk.h>
#include <linux/notifier.h>
#include <linux/mfd/hisi_pmic.h>

#include <linux/of_device.h>
#include <linux/hisi/hisi_log.h>

#define HISI_LOG_TAG HISI_LED_TAG

#ifdef CONFIG_HW_LED_CONFIG
extern void led_config_get_current_setting(struct hisi_led_platform_data* hisi_leds);
#else
void led_config_get_current_setting(struct hisi_led_platform_data* hisi_leds)
{	pr_info("%s enter,not need to set irset for tp color, using default!\n", __func__); }
#endif

extern struct atomic_notifier_head panic_notifier_list;

#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
extern int tp_color_provider(void);
#endif

static struct hisi_led_drv_data *hisi_led_pdata;

static struct hisi_led_platform_data hisi_leds = {
	.leds_size = HISI_LEDS_MAX,
	.leds = {
		[0] = {
			.name = "red",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
			.each_maxdr_iset = 0,
		},
		[1] = {
			.name = "green",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
			.each_maxdr_iset = 0,
		},
		[2] {
			.name = "blue",
			.brightness = LED_OFF,
			.delay_on = 0,
			.delay_off = 0,
			.default_trigger = "timer",
			.each_maxdr_iset = 0,
		},
	},

};

#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
static struct hisi_led_flash_data hisi_leds_flash = {
	.dr_en_mode_345_address = 0,
	.flash_period_dr345_address = 0,
	.flash_on_dr345_address = 0,
	.dr_mode_sel_address = 0,
	.dr_en_mode_345_conf = 0,
	.flash_period_dr345_conf = 0,
	.flash_on_dr345_conf = 0,
	.dr_mode_sel_dr4_conf = 0,
};

int flash_mode_config = 0;
bool  blue_tp_flash_flag = false;
#endif

/* read register  */
static unsigned char hisi_led_reg_read(u32 led_address)
{
    /* return readl(hisi_led_pdata->hisi_led_base + led_address);*/

    return hisi_pmic_reg_read(led_address);
}
/* write register  */
static void hisi_led_reg_write(u8 led_set, u32 led_address)
{
    /*writel(led_set, hisi_led_pdata->hisi_led_base + led_address); */

    hisi_pmic_reg_write(led_address,led_set);
}

static void hisi_led_set_disable(u8 id)
{
	u32 hisi_led_dr_ctl;

	hisi_led_dr_ctl = hisi_led_reg_read(hisi_leds.dr_led_ctrl);
	hisi_led_dr_ctl &= ~(0x1 << id);/*lint !e502*/
	hisi_led_reg_write(hisi_led_dr_ctl, hisi_leds.dr_led_ctrl);

#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
	if((HISI_LED1 == id) && blue_tp_flash_flag)
	{
		hisi_led_reg_write(DR_MODE_SEL_RESET, hisi_leds_flash.dr_mode_sel_address);
		hisi_led_reg_write(DR_EN_MODE_345_RESET, hisi_leds_flash.dr_en_mode_345_address);
	}
#endif
}

static int hisi_led_panic_handler(struct notifier_block *nb,
			unsigned long action, void *data)
{
    int index;

    pr_info("Apanic: %s\n", __func__);
    for(index = 0; index < HISI_LEDS_MAX; index++)
    {
        hisi_led_set_disable(index);
	}
    return 0;
}

static struct notifier_block panic_led = {
	.notifier_call	= hisi_led_panic_handler,
};
static void hisi_led_set_reg_write(struct led_set_config *brightness_config)
{
	/* config current */
	hisi_led_reg_write(brightness_config->brightness_set, brightness_config->hisi_led_iset_address);
	/* start_delay  */
#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
	if( blue_tp_flash_flag)
	{
		hisi_led_reg_write(DR_START_DEL_512_OFF, brightness_config->hisi_led_start_address);//set dr_start_del to 0, otherwise the green light will be lighter earlier than the red light
	}
	else
#endif
	{
		hisi_led_reg_write(DR_START_DEL_512, brightness_config->hisi_led_start_address);
	}
	/* set_on  */
	hisi_led_reg_write(DR_DELAY_ON, brightness_config->hisi_led_tim_address);
	/* enable */
	hisi_led_reg_write(brightness_config->hisi_led_dr_ctl, hisi_leds.dr_led_ctrl);
	/* output enable */
	hisi_led_reg_write(brightness_config->hisi_led_dr_out_ctl, hisi_leds.dr_out_ctrl);
}

#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
static void hisi_led_set_reg_write_flash(u8 id)
{
	hisi_led_reg_write(DR4_ISET_1MA, hisi_leds.leds[id].dr_iset);
	hisi_led_reg_write(hisi_leds_flash.flash_period_dr345_conf, hisi_leds_flash.flash_period_dr345_address);
	hisi_led_reg_write(hisi_leds_flash.flash_on_dr345_conf, hisi_leds_flash.flash_on_dr345_address);
	hisi_led_reg_write(hisi_leds_flash.dr_mode_sel_dr4_conf, hisi_leds_flash.dr_mode_sel_address);
	hisi_led_reg_write(hisi_leds_flash.dr_en_mode_345_conf, hisi_leds_flash.dr_en_mode_345_address);
}
#endif

/* set led half brightness or full brightness  */
static void hisi_led_set_enable(u8 brightness_set, u8 id)
{
    unsigned char hisi_led_dr_ctl;
    unsigned char hisi_led_dr_out_ctl;
    struct led_set_config brightness_config;
#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
    int tp_color = 0;
#endif

    hisi_led_dr_ctl = hisi_led_reg_read(hisi_leds.dr_led_ctrl);
    hisi_led_dr_out_ctl = hisi_led_reg_read(hisi_leds.dr_out_ctrl);

    brightness_config.brightness_set = brightness_set;
    brightness_config.hisi_led_iset_address = hisi_leds.leds[id].dr_iset;
    brightness_config.hisi_led_start_address = hisi_leds.leds[id].dr_start_del;
    brightness_config.hisi_led_tim_address = hisi_leds.leds[id].dr_time_config0;
    brightness_config.hisi_led_tim1_address = hisi_leds.leds[id].dr_time_config1;
    brightness_config.hisi_led_dr_ctl = hisi_led_dr_ctl | (0x1 << id);/*lint !e647*/
    brightness_config.hisi_led_dr_out_ctl = hisi_led_dr_out_ctl & LED_OUT_CTRL_VAL(id);/*lint !e502*/

    pr_info("hisi_led_set_select_led, Led_id=[%d], brightness_set=%d,\n", id, brightness_set);

#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
    pr_info("hisi_led_set_enable, flash_mode_config=%d,blue_tp_flash_flag=%d\n", flash_mode_config,blue_tp_flash_flag);
    /* if need flash,then read tp color  */
    if((HISI_LED1 == id)&&(FLASH_MODE_HAVE_CONFIGERED == flash_mode_config))
    {
        tp_color = tp_color_provider();
        pr_info("read tp color is tp_color:0x%x\n", tp_color);

        /* blue tp */
        if(BLUE == tp_color)
        {
            blue_tp_flash_flag = true;
        }
    }

    if ((HISI_LED1 == id) && blue_tp_flash_flag)
    {
        hisi_led_reg_write(DR_TIME_CONFIG1_OFF, hisi_leds.leds[HISI_LED0].dr_time_config1);//set DR345_TIM_CONF1 to 0, otherwise the green light will be lighter earlier than the red light
        hisi_led_set_reg_write_flash(id);
    }
    else
    {
        hisi_led_reg_write(DR_TIME_CONFIG1_ON, hisi_leds.leds[HISI_LED0].dr_time_config1);//restore DR345_TIM_CONF1
        hisi_led_set_reg_write(&brightness_config);
    }
#else
    hisi_led_set_reg_write(&brightness_config);
#endif

}

#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
/* set led half brightness or full brightness  */
static void hisi_led_set_enable_blink(u8 brightness_set, u8 id)
{
    unsigned char hisi_led_dr_ctl;
    unsigned char hisi_led_dr_out_ctl;
    struct led_set_config brightness_config;

    hisi_led_dr_ctl = hisi_led_reg_read(hisi_leds.dr_led_ctrl);
    hisi_led_dr_out_ctl = hisi_led_reg_read(hisi_leds.dr_out_ctrl);

    brightness_config.brightness_set = brightness_set;
    brightness_config.hisi_led_iset_address = hisi_leds.leds[id].dr_iset;
    brightness_config.hisi_led_start_address = hisi_leds.leds[id].dr_start_del;
    brightness_config.hisi_led_tim_address = hisi_leds.leds[id].dr_time_config0;
    brightness_config.hisi_led_tim1_address = hisi_leds.leds[id].dr_time_config1;
    brightness_config.hisi_led_dr_ctl = hisi_led_dr_ctl | (0x1 << id);
    brightness_config.hisi_led_dr_out_ctl = hisi_led_dr_out_ctl & LED_OUT_CTRL_VAL(id);

    pr_info("hisi_led_set_enable_blink, Led_id=[%d], brightness_set=%d\n", id, brightness_set);

    hisi_led_set_reg_write(&brightness_config);
}
#endif

/* set enable or disable led of dr3,4,5 */
static int hisi_led_set(struct hisi_led_data *led, u8 brightness)
{
	int ret = 0;
	u8 id = led->id;
	u8 iset;
	struct hisi_led_drv_data *data = hisi_led_pdata;

	mutex_lock(&data->lock);

	led_config_get_current_setting(&hisi_leds);

	switch (id) {

	case HISI_LED0:
	case HISI_LED1:
	case HISI_LED2:
	if (brightness == LED_OFF) {
		/* set led off */
		hisi_led_set_disable(id);
		pr_info("[%s] off id is %d\n", __FUNCTION__, id);
	} else if (brightness == LED_FULL) {
		/* set led brightness */
		iset = hisi_leds.leds[id].each_maxdr_iset;
		hisi_led_set_enable(iset, id);
		pr_info("[%s] full id is %d, iset:%d\n", __FUNCTION__, id, iset);
	} else {
		pr_info("[%s] half id is %d\n", __FUNCTION__, id);
		/* set led half brightness */
		hisi_led_set_enable(DR_BRIGHTNESS_HALF, id);
		pr_info("[%s] half id is %d\n", __FUNCTION__, id);
	}
        break;
	default:
		pr_err("hisi_led_set id:%d is error\n", id);
		ret = -EINVAL;
		break;
	}

	mutex_unlock(&data->lock);
	return ret;
}
static void hisi_led_set_blink_reg_write(u8 id, u8 set_time)
{
	hisi_led_reg_write(set_time, hisi_leds.leds[id].dr_time_config0);
}
/* get the set time in area */
static u8 hisi_led_get_time(unsigned long delay, u8 flag)
{
	u8 set_time = 0;

	if (delay == DEL_0)
		set_time = DR_DEL00;
	else if (delay <= DEL_500)
		set_time = DR_DEL01;
	else if (delay <= DEL_1000)
		set_time = DR_DEL02;
	else if (delay <= DEL_2000)
		set_time = DR_DEL03;
	else if (delay <= DEL_4000)
		set_time = DR_DEL04;
	else if (delay <= DEL_6000)
		set_time = DR_DEL05;
	else if (delay <= DEL_8000)
		set_time = DR_DEL06;
	else if (delay <= DEL_12000)
		set_time = DR_DEL07;
	else if (delay <= DEL_14000)
		set_time = DR_DEL08;
	else
		set_time = DR_DEL09;

	if (flag)
		return set_time << 4;
	else
		return set_time;
}

/* config of dr delay_on and delay_off registers */
static int hisi_led_set_blink(struct led_classdev *led_ldev,
	unsigned long *delay_on, unsigned long *delay_off)
{
	struct hisi_led_data *led_dat =
		container_of(led_ldev, struct hisi_led_data, ldev);

	struct hisi_led_drv_data *data = hisi_led_pdata;

	u8 id = 0;
	int ret = 0;
	u8 set_time_on = 0;
	u8 set_time_off = 0;
#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
	u8 iset = 0;
#endif

	if (!led_dat) {
		pr_err("led set blink error\n");
		return -EINVAL;
	}
	id = led_dat->id;
	if ((*delay_on == 0) && (*delay_off == 0))
		return ret;

	mutex_lock(&data->lock);

	if ((id == HISI_LED0) || (id == HISI_LED1) || (id == HISI_LED2)) {
#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
		if((HISI_LED1 == id) && blue_tp_flash_flag && (led_dat->status.brightness != 0))
		{
			 /* first turn off flash mode */
			 hisi_led_set_disable(HISI_LED1);
			 /* set led brightness */
			 iset = DR4_ISET_1MA;
			 hisi_led_set_enable_blink(iset, HISI_LED1);
		}
#endif
		led_ldev->blink_delay_on =  *delay_on;
		led_ldev->blink_delay_off = *delay_off;

		set_time_on = hisi_led_get_time(*delay_on, DELAY_ON);
		set_time_off = hisi_led_get_time(*delay_off, DELAY_OFF);
		hisi_led_set_blink_reg_write(id, set_time_on | set_time_off);
		pr_info("[%s] id is %d, delay-on:%ld, delay-off:%ld\n",
				__FUNCTION__, id, *delay_on, *delay_off);
	} else {
		pr_err("hisi_led_set_blink id:%d is error\n", id);
		ret = -EINVAL;
	}

	mutex_unlock(&data->lock);
	return ret;
}

/* set brightness of dr3,4,5 lights*/
static void hisi_led_set_brightness(struct led_classdev *led_ldev,
				      enum led_brightness brightness)
{
	struct hisi_led_data *led =
		container_of(led_ldev, struct hisi_led_data, ldev);
	if (!led) {
		pr_err("led set btrightnss error!\n");
		return;
	}
	led->status.brightness = brightness;/*lint !e613*/
	hisi_led_set(led, led->status.brightness);/*lint !e613*/
}

/* config lights */
static int hisi_led_configure(struct platform_device *pdev,
				struct hisi_led_drv_data *data,
				struct hisi_led_platform_data *pdata)
{
	int i;
	int ret = 0;

	for (i = 0; i < pdata->leds_size; i++) {
		struct hisi_led *pled = &pdata->leds[i];
		struct hisi_led_data *led = &data->leds[i];
		led->id = i;
		led->status.brightness = pled->brightness;
		led->status.delay_on = pled->delay_on;
		led->status.delay_off = pled->delay_off;
		led->ldev.name = pled->name;
		led->ldev.default_trigger = pled->default_trigger;
		led->ldev.max_brightness = LED_FULL;
		led->ldev.blink_set = hisi_led_set_blink;
		led->ldev.brightness_set = hisi_led_set_brightness;
		//led->ldev.flags = LED_CORE_SUSPENDRESUME;

		ret = led_classdev_register(&pdev->dev, &led->ldev);
		if (ret < 0) {
			dev_err(&pdev->dev,
					"couldn't register LED %s\n",
					led->ldev.name);
			goto exit;
		}
	}
	return 0;

exit:
	if (i > 0) {
		for (i = i - 1; i >= 0; i--) {
			led_classdev_unregister(&data->leds[i].ldev);
		}
	}
	return ret;
}

#ifdef CONFIG_OF
static const struct of_device_id hisi_led_match[] = {
	{ .compatible = "hisilicon,hisi-pmic-led",
		.data = &hisi_leds,},
	{},
};
MODULE_DEVICE_TABLE(of, hisi_led_match);
#endif
static int hisi_led_probe(struct platform_device *pdev)
{
	struct hisi_led_platform_data *pdata;
	struct hisi_led_drv_data *data;
	const struct of_device_id *match;
    struct device_node *root = NULL;
    char compatible_string[LED_DTS_ATTR_LEN] = {0};
    int index;
	int ret = 0 ;

	if ((match = of_match_node(hisi_led_match, pdev->dev.of_node)) == NULL) {
		dev_err(&pdev->dev, "dev_node is not match. exiting.\n");
		return -ENODEV;
	}

	pdata = (struct hisi_led_platform_data *)match->data;
	if (pdata == NULL) {
		dev_err(&pdev->dev, "no platform data\n");
		return -EINVAL;
	}

	data = devm_kzalloc(&pdev->dev, sizeof(struct hisi_led_drv_data), GFP_KERNEL);
	if (!data) {
		dev_err(&pdev->dev, "failed to allocate led_device\n");
		return -ENOMEM;
	}

	mutex_init(&data->lock);

	platform_set_drvdata(pdev, data);
	hisi_led_pdata = data;

    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,dr_led_ctrl", &hisi_leds.dr_led_ctrl);
    if (ret < 0) {
        dev_err(&pdev->dev, "config dr_led_ctrl failure!\n");
        goto err;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,dr_out_ctrl", &hisi_leds.dr_out_ctrl);
    if (ret < 0) {
        dev_err(&pdev->dev, "config dr_out_ctrl failure!\n");
        goto err;
    }

    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,max_iset", &hisi_leds.max_iset);
    if (ret < 0) {
        dev_info(&pdev->dev, "config max_iset failure! use default value\n");
        hisi_leds.max_iset = DR_BRIGHTNESS_FULL;
        //goto err;
    }

#ifdef CONFIG_HISI_LEDS_BLUE_TP_COLOR_SWITCH
    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,hisi_blue_tp_color_switch", &flash_mode_config);
    dev_err(&pdev->dev, "flash_mode_config is %d\n",flash_mode_config);
    if (ret < 0) {
        dev_info(&pdev->dev, "flash_mode_config is not configed!\n");
        flash_mode_config = 0;
    }

    if(FLASH_MODE_HAVE_CONFIGERED == flash_mode_config)
    {
	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,dr_en_mode_345", &hisi_leds_flash.dr_en_mode_345_address);
	    dev_err(&pdev->dev, "dr_en_mode_345 is %x\n",hisi_leds_flash.dr_en_mode_345_address);
	    if (ret < 0) {
	        dev_info(&pdev->dev, "dr_en_mode_345 is not configed!\n");
	        hisi_leds_flash.dr_en_mode_345_address = 0;
		    flash_mode_config = 0;
	    }

	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,flash_period_dr345", &hisi_leds_flash.flash_period_dr345_address);
	    dev_err(&pdev->dev, "flash_period_dr345 is %x\n",hisi_leds_flash.flash_period_dr345_address);
	    if (ret < 0) {
		    dev_info(&pdev->dev, "flash_period_dr345 is not configed!\n");
		    hisi_leds_flash.flash_period_dr345_address = 0;
		    flash_mode_config = 0;
	    }

	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,flash_on_dr345", &hisi_leds_flash.flash_on_dr345_address);
	    dev_err(&pdev->dev, "flash_on_dr345 is %x\n",hisi_leds_flash.flash_on_dr345_address);
	    if (ret < 0) {
		    dev_info(&pdev->dev, "flash_on_dr345 is not configed!\n");
		    hisi_leds_flash.flash_on_dr345_address = 0;
		    flash_mode_config = 0;
	    }

	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,dr_mode_sel", &hisi_leds_flash.dr_mode_sel_address);
	    dev_err(&pdev->dev, "dr_mode_sel is %x\n",hisi_leds_flash.dr_mode_sel_address);
	    if (ret < 0) {
		    dev_info(&pdev->dev, "dr_mode_sel is not configed!\n");
		    hisi_leds_flash.dr_mode_sel_address = 0;
		    flash_mode_config = 0;
	    }

	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,flash_period_dr345_conf", &hisi_leds_flash.flash_period_dr345_conf);
	    dev_err(&pdev->dev, "flash_period_dr345_conf is %x\n",hisi_leds_flash.flash_period_dr345_conf);
	    if (ret < 0) {
		    dev_info(&pdev->dev, "flash_period_dr345_conf is not configed!\n");
		    hisi_leds_flash.flash_period_dr345_conf = 0;
		    flash_mode_config = 0;
	    }

	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,flash_on_dr345_conf", &hisi_leds_flash.flash_on_dr345_conf);
	    dev_err(&pdev->dev, "flash_on_dr345_conf is %x\n",hisi_leds_flash.flash_on_dr345_conf);
	    if (ret < 0) {
		    dev_info(&pdev->dev, "flash_on_dr345_conf is not configed!\n");
		    hisi_leds_flash.flash_on_dr345_conf = 0;
		    flash_mode_config = 0;
	    }

	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,dr_mode_sel_dr4_conf", &hisi_leds_flash.dr_mode_sel_dr4_conf);
	    dev_err(&pdev->dev, "dr_mode_sel_dr4_conf is %x\n",hisi_leds_flash.dr_mode_sel_dr4_conf);
	    if (ret < 0) {
		    dev_info(&pdev->dev, "dr_mode_sel_dr4_conf is not configed!\n");
		    hisi_leds_flash.dr_mode_sel_dr4_conf = 0;
		    flash_mode_config = 0;
	    }

	    ret = of_property_read_u32(pdev->dev.of_node, "hisilicon,dr_en_mode_345_conf", &hisi_leds_flash.dr_en_mode_345_conf);
	    dev_err(&pdev->dev, "dr_en_mode_345_conf is %x\n",hisi_leds_flash.dr_en_mode_345_conf);
	    if (ret < 0) {
		    dev_info(&pdev->dev, "dr_en_mode_345_conf is not configed!\n");
		    hisi_leds_flash.dr_en_mode_345_conf = 0;
		    flash_mode_config = 0;
	    }
    }
#endif

    for (index = 0; index < HISI_LEDS_MAX; index++) {
        snprintf(compatible_string, LED_DTS_ATTR_LEN, "hisilicon,hisi-led%d", index);
        root = of_find_compatible_node(pdev->dev.of_node, NULL, compatible_string);
        if (!root) {
            dev_err(&pdev->dev, "hisilicon,hisi-led%d root error!\n", index);
            goto err;
        }

        ret = of_property_read_string(root, "hisilicon,dr_ctrl", &hisi_leds.leds[index].name);
        if (ret < 0) {
            dev_err(&pdev->dev, "config led%d's colour failure!\n", index);
            goto err;
        }

        ret = of_property_read_u32(root, "hisilicon,each_max_iset", &hisi_leds.leds[index].each_maxdr_iset);
        if (ret < 0) {
            dev_info(&pdev->dev, "config led%d's each_maxdr_iset failure! use default value.\n", index);
            hisi_leds.leds[index].each_maxdr_iset = hisi_leds.max_iset;
            //goto err;
        }

        ret = of_property_read_u32(root, "hisilicon,dr_start_del", &hisi_leds.leds[index].dr_start_del);
        if (ret < 0) {
            dev_err(&pdev->dev, "config led%d's dr_start_del failure!\n", index);
            goto err;
        }

        ret = of_property_read_u32(root, "hisilicon,dr_iset", &hisi_leds.leds[index].dr_iset);
        if (ret < 0) {
            dev_err(&pdev->dev, "config led%d's dr_iset failure!\n", index);
            goto err;
        }

        ret = of_property_read_u32(root, "hisilicon,dr_time_config0", &hisi_leds.leds[index].dr_time_config0);
        if (ret < 0) {
            dev_err(&pdev->dev, "config led%d's dr_time_config0 failure!\n", index);
            goto err;
        }

        ret = of_property_read_u32(root, "hisilicon,dr_time_config1", &hisi_leds.leds[index].dr_time_config1);
        if (ret < 0) {
            dev_err(&pdev->dev, "config led%d's dr_time_config1 failure!\n", index);
            goto err;
        }
    }

	ret = hisi_led_configure(pdev, data, pdata);
	if (ret < 0)
		goto err;

	atomic_notifier_chain_register(&panic_notifier_list, &panic_led);
    dev_err(&pdev->dev, "hisi leds init success.\n");

	return 0;

err:
	return ret;
}

static int hisi_led_remove(struct platform_device *pdev)
{
	int i;
	struct hisi_led_platform_data *pdata;
	const struct of_device_id *match = NULL;
	struct hisi_led_drv_data *data = platform_get_drvdata(pdev);

	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return -ENODEV;
	}

	match = of_match_node(hisi_led_match, pdev->dev.of_node);
	if (match == NULL) {
		dev_err(&pdev->dev, "%s:Device id is NULL\n", __func__);
		return -ENODEV;
	}
	pdata = (struct hisi_led_platform_data *)match->data;
	for (i = 0; i < pdata->leds_size; i++) {
		led_classdev_unregister(&data->leds[i].ldev);
	}

	platform_set_drvdata(pdev, NULL);
	return 0;
}

static void hisi_led_shutdown(struct platform_device *pdev)
{
    int index;

	struct hisi_led_drv_data *data = platform_get_drvdata(pdev);
	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return;
	}

    for (index = 0; index < HISI_LEDS_MAX; index++) {
        hisi_led_set_disable(index);
    }

	return ;
}

#ifdef CONFIG_PM
static int hisi_led_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct hisi_led_drv_data *data = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s: suspend +\n", __func__);
	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return -ENODEV;
	}

	if (!mutex_trylock(&data->lock)) {
		dev_err(&pdev->dev, "%s: mutex_trylock.\n", __func__);
		return -EAGAIN;
	}

	dev_info(&pdev->dev, "%s: suspend -\n", __func__);
	return 0;
}
static int hisi_led_resume(struct platform_device *pdev)
{
	struct hisi_led_drv_data *data = platform_get_drvdata(pdev);

	dev_info(&pdev->dev, "%s: resume +\n", __func__);
	if (data == NULL) {
		dev_err(&pdev->dev, "%s:data is NULL\n", __func__);
		return -ENODEV;
	}

	mutex_unlock(&data->lock);/*lint !e455*/

	dev_info(&pdev->dev, "%s: resume -\n", __func__);
	return 0;
}
#endif

static struct platform_driver hisi_led_driver = {
	.probe		= hisi_led_probe,
	.remove		= hisi_led_remove,
	.shutdown	= hisi_led_shutdown,
#ifdef CONFIG_PM
	.suspend	= hisi_led_suspend,
	.resume		= hisi_led_resume,
#endif
	.driver		= {
		.name	= HISI_LEDS,
		.owner	= THIS_MODULE,
		.of_match_table = of_match_ptr(hisi_led_match),
	},
};

module_platform_driver(hisi_led_driver);

MODULE_ALIAS("platform:hisi-leds");
MODULE_DESCRIPTION("hisi LED driver");
MODULE_LICENSE("GPL");
