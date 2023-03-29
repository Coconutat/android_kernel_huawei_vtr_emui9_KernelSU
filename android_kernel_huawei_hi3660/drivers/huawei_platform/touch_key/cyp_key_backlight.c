#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/leds.h>
#include <linux/of_platform.h>
#include <linux/clk.h>
#include <linux/mfd/hisi_pmic.h>
#include <huawei_platform/log/hw_log.h>
#include <linux/of_device.h>
#include <linux/sched.h>
#include <linux/timer.h>
#include "huawei_platform/sensor/huawei_key.h"
#ifdef CONFIG_FB
#include <linux/notifier.h>
#include <linux/fb.h>
#endif

#define KEY_BACKLIGHT_AP_COMPATIBLE_ID	"huawei,key_backlight_ap_reg"
#define HWLOG_TAG key_backlight_ap

#define DR_ENABLE_KEYPAD1	0x01	/* dr1 enable */
#define DR_ENABLE_KEYPAD2	0x10	/* dr2 enable */
#define DR_ENABLE_PWM_KEYPAD1	0x03	/* dr1 pwm enable */
#define DR_ENABLE_PWM_KEYPAD2	0x30	/* dr2 pwm enable */
#define DR_ENABLE_PWM_KEYPAD	0x33	/* dr12 pwm enable */
#define DR_DISABLE_KEYPAD	0x00	/* dr1,2 disable */
#define KEYPAD_ALWAYS_ON	0x70
#define KEYPAD_PWM_1MS_ON	0x00

#define KEYPAD_DAFAULT_DELAY_OFF	(500)	/* default delay 5 sec */

#define KEYPAD_BRIGHTNESS_FULL	0x01
#define KEYPAD_BRIGHTNESS_HALF	0x00

#define KEYPAD_BRIGHTNESS_3MA	0x00
#define KEYPAD_BRIGHTNESS_6MA	0x01
#define KEYPAD_BRIGHTNESS_9MA	0x02
#define KEYPAD_BRIGHTNESS_12MA	0x03
#define KEYPAD_BRIGHTNESS_15MA	0x04
#define KEYPAD_BRIGHTNESS_18MA	0x05
#define KEYPAD_BRIGHTNESS_21MA	0x06
#define KEYPAD_BRIGHTNESS_24MA	0x07

#define ENABLE_NUM	1
#define DISABLE_NUM	0
#define MAX_BYTE	0xff

#define BRIGHTNESS_FULL		255
#define BRIGHTNESS_HALF		127
#define BRIGHTNESS_OFF		0

#define BRIGHTNESS_3MA		0x01
#define BRIGHTNESS_6MA		0x20
#define BRIGHTNESS_9MA		0x40
#define BRIGHTNESS_12MA		0x60
#define BRIGHTNESS_15MA		0x80
#define BRIGHTNESS_18MA		0xA0
#define BRIGHTNESS_21MA		0xC0
#define BRIGHTNESS_24MA		0xE0

#define TP_COLOR_WHITE		0xE1
#define TP_COLOR_GOLD		0x87

#define DR12_TIM_CONF1		0xB5
#define DR12_TIM_CONF1_PWM	0x00

HWLOG_REGIST();

struct huawei_led {
	const char *name;
	unsigned int dr_iset0;
	unsigned int dr_iset1;
	unsigned long dr_led_ctrl;
	unsigned long dr_time_config0;
	unsigned long dr_time_config1;
	unsigned long delay_off;
	char *default_trigger;
	struct mutex data_lock;
	struct mutex dr_lock;
#ifdef CONFIG_FB
	struct notifier_block fb_notif;
#endif
};

static struct key_param_t last_brightness_on = {
		.brightness1 = BRIGHTNESS_OFF,
		.brightness2 = BRIGHTNESS_OFF,
};

static struct huawei_led keypad_leds = {0};
static struct timer_list keypad_backlight_timer = {0};
static int debug_mode = DISABLE_NUM;

#define BRIGHTNESS_THRESHOLD0 0
#define BRIGHTNESS_THRESHOLD1 40
#define BRIGHTNESS_THRESHOLD2 130
#define BRIGHTNESS_THRESHOLD3 255

struct huawei_keypad_brightness {
	unsigned int threshold0;
	unsigned int threshold1;
	unsigned int threshold2;
	unsigned int threshold3;
	unsigned int current0;
	unsigned int current1;
	unsigned int current2;
};

static struct huawei_keypad_brightness keypad_brightness = {0};

/* tp color*/
#define TP_COLOR_BUF_SIZE   20
static int keypad_tp_color = 0;
static char tp_color_buf[TP_COLOR_BUF_SIZE] = {0};

int huawei_set_dr_key_backlight(void *param_t);

/* write register  */
static void hisi_led_reg_write(u8 led_set, u32 led_address)
{
	hisi_pmic_reg_write(led_address, led_set);
}

static void hisi_led_keypad_set_brightness(u8 brightness_set1,
		u8 brightness_set2, u8 dr_enable)
{
	mutex_lock(&keypad_leds.dr_lock);
	hwlog_info("keypad set reg to %d, %d\n", brightness_set1, brightness_set2);

	if((keypad_tp_color == TP_COLOR_WHITE) && (brightness_set1 == KEYPAD_BRIGHTNESS_9MA)) {
		brightness_set1 = KEYPAD_BRIGHTNESS_12MA;
		brightness_set2 = KEYPAD_BRIGHTNESS_12MA;
	}
	/* config current */
	hisi_led_reg_write(brightness_set1, keypad_leds.dr_iset0);
	hisi_led_reg_write(brightness_set2, keypad_leds.dr_iset1);

	if (keypad_tp_color == TP_COLOR_WHITE) {
		/* set 1ms pwm on */
		hisi_led_reg_write(KEYPAD_PWM_1MS_ON, keypad_leds.dr_time_config0);
		hisi_led_reg_write(DR12_TIM_CONF1_PWM, keypad_leds.dr_time_config1);
		hwlog_info("keypad set 1ms pwm mode\n");
	} else {
		/* set_on */
		hisi_led_reg_write(KEYPAD_ALWAYS_ON, keypad_leds.dr_time_config0);
		hwlog_info("keypad set always on mode\n");
	}
	/* enable dr1 dr2*/
	hisi_led_reg_write(dr_enable, keypad_leds.dr_led_ctrl);
	mutex_unlock(&keypad_leds.dr_lock);
}

static void hisi_led_keypad_set_dr_disable(void)
{
	if (debug_mode == ENABLE_NUM) {
		hwlog_info("debug mode, keypad led always on.\n");
		return;
	}
	hwlog_info("set to 0, 0\n");
	mutex_lock(&keypad_leds.dr_lock);
	hisi_led_reg_write(DR_DISABLE_KEYPAD, keypad_leds.dr_led_ctrl);
	mutex_unlock(&keypad_leds.dr_lock);
}

int huawei_set_dr_key_backlight(void *param_t)
{
	u8 bl_level1 = 0, bl_level2 = 0;
	u8 brightness1, brightness2, test_mode;
	u8 brightness = 0, last_brightness1 = 0, last_brightness2 = 0;
	u8 dr_enable = DR_DISABLE_KEYPAD;
	int ret = 0;
	struct key_param_t *param = (struct key_param_t *)param_t;

	last_brightness1 = param->brightness1;
	last_brightness2 = param->brightness2;
	brightness = param->brightness1;
	if(brightness == keypad_brightness.threshold0) {
		param->brightness1 = BRIGHTNESS_OFF;
		param->brightness2 = BRIGHTNESS_OFF;
	}
	if((brightness > keypad_brightness.threshold0) && (brightness <= keypad_brightness.threshold1)) {
		param->brightness1 = keypad_brightness.current0;
		param->brightness2 = keypad_brightness.current0;
	}
	if((brightness > keypad_brightness.threshold1) && (brightness <= keypad_brightness.threshold2)) {
		if(keypad_tp_color == TP_COLOR_GOLD) {
			param->brightness1 = keypad_brightness.current2;
			param->brightness2 = keypad_brightness.current2;
		} else {
			param->brightness1 = keypad_brightness.current1;
			param->brightness2 = keypad_brightness.current1;
		}
	}
	if((brightness > keypad_brightness.threshold2) && (brightness <= keypad_brightness.threshold3)) {
		param->brightness1 = keypad_brightness.current2;
		param->brightness2 = keypad_brightness.current2;
	}

	mutex_lock(&keypad_leds.data_lock);
	brightness1 = param->brightness1;
	brightness2 = param->brightness2;
	test_mode = param->test_mode;
	mutex_unlock(&keypad_leds.data_lock);

	if (debug_mode == ENABLE_NUM && test_mode != ENABLE_NUM) {
		hwlog_info("debug mode.\n");
		return ret;
	}

	switch (brightness1) {
	case BRIGHTNESS_OFF:
		break;
	case BRIGHTNESS_3MA:
		bl_level1 = KEYPAD_BRIGHTNESS_3MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	case BRIGHTNESS_6MA:
		bl_level1 = KEYPAD_BRIGHTNESS_6MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	case BRIGHTNESS_9MA:
		bl_level1 = KEYPAD_BRIGHTNESS_9MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	case BRIGHTNESS_12MA:
		bl_level1 = KEYPAD_BRIGHTNESS_12MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	case BRIGHTNESS_15MA:
		bl_level1 = KEYPAD_BRIGHTNESS_15MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	case BRIGHTNESS_18MA:
		bl_level1 = KEYPAD_BRIGHTNESS_18MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	case BRIGHTNESS_21MA:
		bl_level1 = KEYPAD_BRIGHTNESS_21MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	case BRIGHTNESS_24MA:
		bl_level1 = KEYPAD_BRIGHTNESS_24MA;
		dr_enable |= DR_ENABLE_KEYPAD1;
		break;
	default:
		hwlog_info("keypad1 set not support brightness\n");
		return ret;
	}
	switch (brightness2) {
	case BRIGHTNESS_OFF:
		break;
	case BRIGHTNESS_3MA:
		bl_level2 = KEYPAD_BRIGHTNESS_3MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	case BRIGHTNESS_6MA:
		bl_level2 = KEYPAD_BRIGHTNESS_6MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	case BRIGHTNESS_9MA:
		bl_level2 = KEYPAD_BRIGHTNESS_9MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	case BRIGHTNESS_12MA:
		bl_level2 = KEYPAD_BRIGHTNESS_12MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	case BRIGHTNESS_15MA:
		bl_level2 = KEYPAD_BRIGHTNESS_15MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	case BRIGHTNESS_18MA:
		bl_level2 = KEYPAD_BRIGHTNESS_18MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	case BRIGHTNESS_21MA:
		bl_level2 = KEYPAD_BRIGHTNESS_21MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	case BRIGHTNESS_24MA:
		bl_level2 = KEYPAD_BRIGHTNESS_24MA;
		dr_enable |= DR_ENABLE_KEYPAD2;
		break;
	default:
		hwlog_info("keypad2 set not support brightness\n");
		return ret;
	}

	if ((keypad_tp_color == TP_COLOR_WHITE) && (dr_enable != DR_DISABLE_KEYPAD)) {
		dr_enable |= DR_ENABLE_PWM_KEYPAD;
	}

	hisi_led_keypad_set_brightness(bl_level1, bl_level2, dr_enable);

	if (dr_enable == DR_DISABLE_KEYPAD) {
		hwlog_info("set keypad disable.\n");
		hisi_led_keypad_set_dr_disable();
		last_brightness_on.brightness1 = BRIGHTNESS_OFF;
		last_brightness_on.brightness2 = BRIGHTNESS_OFF;
		return ret;
	}
#ifdef CONFIG_FB
	else {
		last_brightness_on.brightness1 = last_brightness1;
		last_brightness_on.brightness2 = last_brightness2;
	}
#endif

	hwlog_info("[%s]  id is keypad, bl_level:%d, %d\n",
					__FUNCTION__, bl_level1, bl_level2);

	return ret;
}

int key_backlight_power_on(void) {
	huawei_set_dr_key_backlight(&last_brightness_on);
	return 0;
}

#ifdef CONFIG_FB
static int fb_notifier_callback(struct notifier_block *self,
		unsigned long event, void *data)
{
	int *blank;
	int ret = 0;
	struct fb_event *evdata = data;

	struct huawei_led *led_data =
		container_of(self, struct huawei_led, fb_notif);
	if (evdata && evdata->data &&
			(FB_EVENT_BLANK == event) && led_data) {
		blank = evdata->data;
		if (FB_BLANK_UNBLANK == *blank) {
			hwlog_info("%s: unblank +.\n", __func__);
			huawei_set_dr_key_backlight(&last_brightness_on);
		} else if (FB_BLANK_POWERDOWN == *blank) {
			hwlog_info("%s: blank -.\n", __func__);
			hisi_led_keypad_set_dr_disable();
		}
	}
	return 0;
}
#endif

static void huawei_led_set_brightness(struct led_classdev *led_ldev,
				      u32 brightness)
{
	int ret = 0;
	struct key_param_t param;

	param.brightness1 = (brightness >> sizeof(char))
						& MAX_BYTE;
	param.brightness2 = brightness & MAX_BYTE;
	param.test_mode = ENABLE_NUM;
	if (brightness == 0) {
		debug_mode = DISABLE_NUM;
	} else {
		debug_mode = ENABLE_NUM;
	}
	ret = huawei_set_dr_key_backlight(&param);
	if (ret < 0) {
		hwlog_info("set key backlight err. ret:%d\n", ret);
	}
	return;
}

static struct led_classdev huawei_led_ap_ldev=
{
	.name = "keyboard-backlight-ap",
	.max_brightness = LED_FULL,
	.brightness_set = huawei_led_set_brightness,
};

/* Get tp_color parm from cmdline */
static int read_tp_color(void)
{
	int tp_color;
	hwlog_info("keypad tp color is %s\n", tp_color_buf);

	tp_color = (int)simple_strtol(tp_color_buf, NULL, 0);
	return tp_color;
}

static int __init early_parse_tp_color_cmdline(char *arg)
{
	unsigned int len = 0;
	memset(tp_color_buf, 0, sizeof(tp_color_buf));
	if (arg) {
		len = strlen(arg);

		if (len > sizeof(tp_color_buf)) {
			len = sizeof(tp_color_buf);
		}
		memcpy(tp_color_buf, arg, len);
	} else {
		hwlog_info("%s : arg is NULL\n", __func__);
	}

	return 0;
}
/*lint -save -e* */
early_param("TP_COLOR", early_parse_tp_color_cmdline);

static int key_backlight_probe(struct platform_device *pdev)
{
	int ret = 0;
	hwlog_info("key_backlight_ap device probe in\n");

	keypad_tp_color = read_tp_color();
	hwlog_info("keypad tp_color = 0x%x\n", keypad_tp_color);

	ret = led_classdev_register(&pdev->dev, &huawei_led_ap_ldev);
	if (ret < 0) {
		hwlog_info("couldn't register LED %s\n", huawei_led_ap_ldev.name);
		goto errout;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "dr_led_ctrl", &keypad_leds.dr_led_ctrl);
	if (ret < 0) {
		hwlog_info("couldn't get dr_led_ctrl\n");
		goto error;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "dr_iset0", &keypad_leds.dr_iset0);
	if (ret < 0) {
		hwlog_info("couldn't get dr_iset0\n");
		goto error;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "dr_iset1", &keypad_leds.dr_iset1);
	if (ret < 0) {
		hwlog_info("couldn't get dr_iset1\n");
		goto error;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "dr_time_config0", &keypad_leds.dr_time_config0);
	if (ret < 0) {
		hwlog_info("couldn't get dr_time_config0\n");
		goto error;
	}
	ret = of_property_read_u32(pdev->dev.of_node, "dr_time_config1", &keypad_leds.dr_time_config1);
	if (ret < 0) {
		keypad_leds.dr_time_config1 = DR12_TIM_CONF1;
		hwlog_info("couldn't get dr_time_config1, use the default reg = 0x%x\n", keypad_leds.dr_time_config1);
	}
	ret = of_property_read_u32(pdev->dev.of_node, "dr_delay_off", &keypad_leds.delay_off);
	if (ret < 0) {
		hwlog_info("couldn't get dr_delay_off\n");
		keypad_leds.delay_off = KEYPAD_DAFAULT_DELAY_OFF;
	}

	ret = of_property_read_u32(pdev->dev.of_node, "keypad_threshold0", &keypad_brightness.threshold0);
	if (ret < 0) {
		keypad_brightness.threshold0 = BRIGHTNESS_THRESHOLD0;
		hwlog_info("couldn't get keypad_threshold0, set to default = %d\n", keypad_brightness.threshold0);
	}

	ret = of_property_read_u32(pdev->dev.of_node, "keypad_threshold1", &keypad_brightness.threshold1);
	if (ret < 0) {
		keypad_brightness.threshold1 = BRIGHTNESS_THRESHOLD1;
		hwlog_info("couldn't get keypad_threshold1, set to default = %d\n", keypad_brightness.threshold1);
	}

	ret = of_property_read_u32(pdev->dev.of_node, "keypad_threshold2", &keypad_brightness.threshold2);
	if (ret < 0) {
		keypad_brightness.threshold2 = BRIGHTNESS_THRESHOLD2;
		hwlog_info("couldn't get keypad_threshold2, set to default = %d\n", keypad_brightness.threshold2);
	}

	ret = of_property_read_u32(pdev->dev.of_node, "keypad_threshold3", &keypad_brightness.threshold3);
	if (ret < 0) {
		keypad_brightness.threshold3 = BRIGHTNESS_THRESHOLD3;
		hwlog_info("couldn't get keypad_threshold3, set to default = %d\n", keypad_brightness.threshold3);
	}

	ret = of_property_read_u32(pdev->dev.of_node, "keypad_current0", &keypad_brightness.current0);
	if (ret < 0) {
		keypad_brightness.current0 = BRIGHTNESS_3MA;
		hwlog_info("couldn't get keypad_current0, set to default = 0x%x\n", keypad_brightness.current0);
	}

	ret = of_property_read_u32(pdev->dev.of_node, "keypad_current1", &keypad_brightness.current1);
	if (ret < 0) {
		keypad_brightness.current1 = BRIGHTNESS_6MA;
		hwlog_info("couldn't get keypad_current1, set to default = 0x%x\n", keypad_brightness.current1);
	}

	ret = of_property_read_u32(pdev->dev.of_node, "keypad_current2", &keypad_brightness.current2);
	if (ret < 0) {
		keypad_brightness.current2 = BRIGHTNESS_9MA;
		hwlog_info("couldn't get keypad_current2, set to default = 0x%x\n", keypad_brightness.current2);
	}

#ifdef CONFIG_FB
	keypad_leds.fb_notif.notifier_call = fb_notifier_callback;
	ret = fb_register_client(&keypad_leds.fb_notif);
	if (ret) {
		hwlog_err("%s: Unable to register fb_notifier: %d", __func__, ret);
		goto err_register_fb;
	}
#endif

	mutex_init(&keypad_leds.data_lock);
	mutex_init(&keypad_leds.dr_lock);

	hwlog_info("key_backlight_ap device probe success\n");
	return 0;

#ifdef CONFIG_FB
err_register_fb:
	fb_unregister_client(&keypad_leds.fb_notif);
#endif
error:
	led_classdev_unregister(&huawei_led_ap_ldev);
errout:
	return ret;
}

static int key_backlight_remove(struct platform_device *pdev)
{
#ifdef CONFIG_FB
	fb_unregister_client(&keypad_leds.fb_notif);
#endif

	return 0;
}

static const struct of_device_id key_backlight_match_table[] = {
	{
	 .compatible = KEY_BACKLIGHT_AP_COMPATIBLE_ID,
	 .data = NULL,
	},
	{},
};

MODULE_DEVICE_TABLE(of, key_backlight_match_table);

static struct platform_driver key_backlight_driver = {
	.probe = key_backlight_probe,
	.remove = key_backlight_remove,
	.driver = {
		   .name = "key_backlight_ap",
		   .owner = THIS_MODULE,
		   .of_match_table = of_match_ptr(key_backlight_match_table),
		   },
};

static int key_backlight_init(void)
{
	hwlog_info("key_backlight_ap device init \n");
	return platform_driver_register(&key_backlight_driver);
}

static void key_backlight_exit(void)
{
	platform_driver_unregister(&key_backlight_driver);
}

module_init(key_backlight_init);
module_exit(key_backlight_exit);

MODULE_AUTHOR("Huawei");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("huawei touch key backlight ap driver");
