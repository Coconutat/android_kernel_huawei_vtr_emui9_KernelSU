/*
 * drivers/display/hisi/backlight/lm3697.c
 *
 * lm3697 driver reffer to lcd
 *
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/module.h>
#include <linux/regmap.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include "lcd_kit_common.h"
#include "lcd_kit_utils.h"
#include "lm3697.h"
#include "lcm_drv.h"

static char *lm3697_dts_string[LM3697_RW_REG_MAX] = {
	"lm3697_reg_sink_output_config",
	"lm3697_reg_ramp_on_off_l",
	"lm3697_reg_ramp_on_off_r",
	"lm3697_reg_ramp_time",
	"lm3697_reg_ramp_time_config",
	"lm3697_reg_brightness_config",
	"lm3697_reg_full_scale_current_setting_a",
	"lm3697_reg_full_scale_current_setting_b",
	"lm3697_reg_current_sink_feedback_enable",
	"lm3697_reg_boost_control",
	"lm3697_reg_auto_frequency_threshold",
	"lm3697_reg_pwm_config",
	"lm3697_reg_brightness_lsb_a",
	"lm3697_reg_brightness_msb_a",
	"lm3697_reg_brightness_lsb_b",
	"lm3697_reg_brightness_msb_b",
	"lm3697_reg_bank_enable",
	"lm3697_reg_open_faults",
	"lm3697_reg_short_faults",
	"lm3697_reg_fault_enable",
};

static unsigned int lm3697_reg_addr[LM3697_RW_REG_MAX] = {
	LM3697_REG_SINK_OUTPUT_CONFIG,
	LM3697_REG_RAMP_ON_OFF_L,
	LM3697_REG_RAMP_ON_OFF_R,
	LM3697_REG_RAMP_TIME,
	LM3697_REG_RAMP_TIME_CONFIG,
	LM3697_REG_BRIGHTNESS_CONFIG,
	LM3697_REG_FULL_SCALE_CURRENT_SETTING_A,
	LM3697_REG_FULL_SCALE_CURRENT_SETTING_B,
	LM3697_REG_CURRENT_SINK_FEEDBACK_ENABLE,
	LM3697_REG_BOOST_CONTROL,
	LM3697_REG_AUTO_FREQUENCY_THRESHOLD,
	LM3697_REG_PWM_CONFIG,
	LM3697_REG_BRIGHTNESS_LSB_A,
	LM3697_REG_BRIGHTNESS_MSB_A,
	LM3697_REG_BRIGHTNESS_LSB_B,
	LM3697_REG_BRIGHTNESS_MSB_B,
	LM3697_REG_BANK_ENABLE,
	LM3697_REG_OPEN_FAULTS,
	LM3697_REG_SHORT_FAULTS,
	LM3697_REG_FAULT_ENABLE,
};

static struct lm3697_backlight_information lm3697_bl_info;
struct class *lm3697_class = NULL;
struct lm3697_chip_data *lm3697_g_chip = NULL;
static bool lm3697_init_status = true;
extern struct LCM_DRIVER lcdkit_mtk_common_panel;

static int lm3697_parse_dts(struct device_node *np)
{
	int ret = 0;
	int i = 0;

	if(np == NULL){
		LCD_KIT_ERR("np is null pointer\n");
		return -1;
	}

	for (i = 0;i < LM3697_RW_REG_MAX;i++ ) {
		ret = of_property_read_u32(np, lm3697_dts_string[i], &lm3697_bl_info.lm3697_reg[i]);
		if (ret < 0) {
			lm3697_bl_info.lm3697_reg[i] = 0xffff;//init to invalid data
			LCD_KIT_INFO("can not find config:%s\n", lm3697_dts_string[i]);
		}
	}
	ret = of_property_read_u32(np, LM3697_HW_ENABLE, &lm3697_bl_info.lm3697_hw_en);
	if (ret < 0) {
		LCD_KIT_ERR("get lm3697_hw_enable dts config failed\n");
		return ret;
	}
	ret = of_property_read_u32(np, LM3697_HW_EN_GPIO, &lm3697_bl_info.lm3697_hw_en_gpio);
	if (ret < 0) {
		LCD_KIT_ERR("get lm3697_hw_en_gpio dts config failed\n");
		return ret;
	}

	/*gpio number offset*/
	lm3697_bl_info.lm3697_hw_en_gpio += ((struct mtk_panel_info *)(lcdkit_mtk_common_panel.panel_info))->gpio_offset;
	ret = of_property_read_u32(np, LM3697_HW_EN_DELAY, &lm3697_bl_info.bl_on_lk_mdelay);
	if (ret < 0) {
		LCD_KIT_ERR("get bl_on_kernel_mdelay dts config failed\n");
		return ret;
	}

	ret = of_property_read_u32(np, LM3697_BACKLIGHT_CONFIG_LSB_REG_ADDR, &lm3697_bl_info.lm3697_level_lsb_reg);
	if (ret < 0) {
		LCD_KIT_ERR("get lm3697_backlight_config_lsb_reg_addr dts config failed\n");
		lm3697_bl_info.lm3697_level_lsb_reg = LM3697_REG_BRIGHTNESS_LSB_B;
	}

	ret = of_property_read_u32(np, LM3697_BACKLIGHT_CONFIG_MSB_REG_ADDR, &lm3697_bl_info.lm3697_level_msb_reg);
	if (ret < 0) {
		LCD_KIT_ERR("get lm3697_backlight_config_msb_reg_addr dts config failed\n");
		lm3697_bl_info.lm3697_level_msb_reg = LM3697_REG_BRIGHTNESS_MSB_B;
	}
	return ret;
}

static int lm3697_config_write(struct lm3697_chip_data *pchip,
			unsigned int reg[],unsigned int val[],unsigned int size)
{
	int ret = 0;
	unsigned int i = 0;

	if((pchip == NULL) || (reg == NULL) || (val == NULL)){
		LCD_KIT_ERR("pchip or reg or val is null pointer\n");
		return -1;
	}

	for(i = 0;i < size;i++) {
		/*judge reg is valid*/
		if (val[i] != 0xffff) {
			ret = regmap_write(pchip->regmap, reg[i], val[i]);
			if (ret < 0) {
				LCD_KIT_ERR("write lm3697 backlight config register 0x%x failed\n",reg[i]);
				goto exit;
			}
		}
	}

exit:
	return ret;
}

static int lm3697_config_read(struct lm3697_chip_data *pchip,
			unsigned int reg[],unsigned int val[],unsigned int size)
{
	int ret = 0;
	unsigned int i = 0;

	if((pchip == NULL) || (reg == NULL) || (val == NULL)){
		LCD_KIT_ERR("pchip or reg or val is null pointer\n");
		return -1;
	}

	for(i = 0;i < size;i++) {
		ret = regmap_read(pchip->regmap, reg[i],&val[i]);
		if (ret < 0) {
			LCD_KIT_ERR("read lm3697 backlight config register 0x%x failed",reg[i]);
			goto exit;
		} else {
			LCD_KIT_INFO("read 0x%x value = 0x%x\n", reg[i], val[i]);
		}
	}

exit:
	return ret;
}

/* initialize chip */
static int lm3697_chip_init(struct lm3697_chip_data *pchip)
{
	int ret = -1;

	LCD_KIT_INFO("%s in!\n",__func__);

	if(pchip == NULL){
		LCD_KIT_ERR("pchip is null pointer\n");
		return -1;
	}

	ret = lm3697_config_write(pchip, lm3697_reg_addr, lm3697_bl_info.lm3697_reg, LM3697_RW_REG_MAX);
	if (ret < 0) {
		LCD_KIT_ERR("lm3697 config register failed");
		goto out;
	}
	LCD_KIT_INFO("%s ok!\n",__func__);
	return ret;

out:
	dev_err(pchip->dev, "i2c failed to access register\n");
	return ret;
}

static ssize_t lm3697_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm3697_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret = -1;

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return ret;
	}

	if (!dev) {
		ret =  snprintf(buf, PAGE_SIZE, "dev is null\n");
		return ret;
	}

	pchip = dev_get_drvdata(dev);
	if (!pchip) {
		ret = snprintf(buf, PAGE_SIZE, "data is null\n");
		return ret;
	}

	client = pchip->client;
	if (!client) {
		ret = snprintf(buf, PAGE_SIZE, "client is null\n");
		return ret;
	}

	ret = lm3697_config_read(pchip, lm3697_reg_addr, lm3697_bl_info.lm3697_reg, LM3697_RW_REG_MAX);
	if (ret < 0) {
		LCD_KIT_ERR("lm3697 config read failed");
		goto i2c_error;
	}

	ret = snprintf(buf, PAGE_SIZE, "Device Sink Config(0x10)= 0x%x\nEprom Configuration0(0x11) = 0x%x\n \
			\rEprom Configuration1(0x12) = 0x%x\nEprom Configuration2(0x13) = 0x%x\n \
			\rEprom Configuration3(0x14) = 0x%x\nEprom Configuration4(0x16) = 0x%x\n \
			\rEprom Configuration5(0x17) = 0x%x\nEprom Configuration4(0x18) = 0x%x\n \
			\rEprom Configuration4(0x19) = 0x%x\nEprom Configuration4(0x1A) = 0x%x\n \
			\rEprom Configuration4(0x1B) = 0x%x\nEprom Configuration4(0x1C) = 0x%x\n \
			\rEprom Configuration4(0x20) = 0x%x\nEprom Configuration4(0x21) = 0x%x\n \
			\rEprom Configuration4(0x22) = 0x%x\nEprom Configuration4(0x23) = 0x%x\n \
			\rEprom Configuration4(0x24) = 0x%x\nEprom Configuration4(0xB0) = 0x%x\n \
			\rEprom Configuration4(0xB2) = 0x%x\nEprom Configuration4(0xB4) = 0x%x\n ",
			lm3697_bl_info.lm3697_reg[0], lm3697_bl_info.lm3697_reg[1], lm3697_bl_info.lm3697_reg[2], lm3697_bl_info.lm3697_reg[3], lm3697_bl_info.lm3697_reg[4], lm3697_bl_info.lm3697_reg[5],lm3697_bl_info.lm3697_reg[6],lm3697_bl_info.lm3697_reg[7],lm3697_bl_info.lm3697_reg[8],lm3697_bl_info.lm3697_reg[9],lm3697_bl_info.lm3697_reg[10],lm3697_bl_info.lm3697_reg[11],lm3697_bl_info.lm3697_reg[12],lm3697_bl_info.lm3697_reg[13],lm3697_bl_info.lm3697_reg[14],lm3697_bl_info.lm3697_reg[15],lm3697_bl_info.lm3697_reg[16],lm3697_bl_info.lm3697_reg[17],lm3697_bl_info.lm3697_reg[18],lm3697_bl_info.lm3697_reg[19]);
	return ret;

i2c_error:
	ret = snprintf(buf, PAGE_SIZE,"%s: i2c access fail to register\n", __func__);
	return ret;
}

static ssize_t lm3697_reg_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm3697_chip_data *pchip = NULL;
	unsigned int reg = 0;
	unsigned int mask = 0;
	unsigned int val = 0;

	if (!buf) {
		LCD_KIT_ERR("buf is null\n");
		return ret;
	}

	if (!dev) {
		LCD_KIT_ERR("dev is null\n");
		return ret;
	}

	pchip = dev_get_drvdata(dev);
	if(!pchip){
		LCD_KIT_ERR("pchip is null\n");
		return ret;
	}

	ret = sscanf(buf, "reg=0x%x, mask=0x%x, val=0x%x", &reg, &mask, &val);
	if (ret < 0) {
		LCD_KIT_INFO("check your input!!!\n");
		goto out_input;
	}

	LCD_KIT_INFO("%s: reg=0x%x,mask=0x%x,val=0x%x\n", __func__, reg, mask, val);

	ret = regmap_update_bits(pchip->regmap, reg, mask, val);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	ret = snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);
	return ret;

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	ret = snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
	return ret;
}

static DEVICE_ATTR(reg, (S_IRUGO|S_IWUSR), lm3697_reg_show, lm3697_reg_store);

/* pointers to created device attributes */
static struct attribute *lm3697_attributes[] = {
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group lm3697_group = {
	.attrs = lm3697_attributes,
};

static const struct regmap_config lm3697_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
};

static void lm3697_enable(void)
{
	int ret = 0;

	if(lm3697_bl_info.lm3697_hw_en)
	{
		ret = gpio_request(lm3697_bl_info.lm3697_hw_en_gpio, NULL);
		if (ret) {
			LCD_KIT_ERR("lm3697 Could not request lm3697 hw_en_gpio\n");
		}
		ret = gpio_direction_output(lm3697_bl_info.lm3697_hw_en_gpio, GPIO_DIR_OUT);
		if (ret) {
			LCD_KIT_ERR("lm3697 Could not set lm3697 gpio output not success.\n");
		}
		gpio_set_value(lm3697_bl_info.lm3697_hw_en_gpio, GPIO_OUT_ONE);
		if(lm3697_bl_info.bl_on_lk_mdelay)
		{
			mdelay(lm3697_bl_info.bl_on_lk_mdelay);
		}
	}
	/* chip initialize */
	ret = lm3697_chip_init(lm3697_g_chip);
	if (ret < 0) {
		LCD_KIT_ERR("lm3697_chip_init fail!\n");
		return ;
	}
	lm3697_init_status = true;
}

static void lm3697_disable(void)
{
	if(lm3697_bl_info.lm3697_hw_en)
	{
		gpio_set_value(lm3697_bl_info.lm3697_hw_en_gpio, GPIO_OUT_ZERO);
		gpio_free(lm3697_bl_info.lm3697_hw_en_gpio);
	}
	lm3697_init_status = false;
}

int lm3697_set_backlight(uint32_t bl_level)
{
	static int last_bl_level = 0;
	int ret = 0;

	if (!lm3697_g_chip) {
		LCD_KIT_ERR("lm3697_g_chip is null\n");
		return -1;
	}
	if (down_trylock(&(lm3697_g_chip->test_sem))) {
		LCD_KIT_INFO("Now in test mode\n");
		return 0;
	}
	/*first set backlight, enable lm3697*/
	if (false == lm3697_init_status && bl_level > 0) {
		lm3697_enable();
	}
	/*set backlight level*/
	ret = regmap_write(lm3697_g_chip->regmap, lm3697_bl_info.lm3697_level_lsb_reg, LM3697_REG_BRIGHTNESS_LSB_B_BRIGHTNESS[bl_level]);
	if (ret < 0) {
		LCD_KIT_ERR("write lm3697 backlight level lsb:0x%x failed\n", LM3697_REG_BRIGHTNESS_LSB_B_BRIGHTNESS[bl_level]);
	}
	ret = regmap_write(lm3697_g_chip->regmap, lm3697_bl_info.lm3697_level_msb_reg, LM3697_REG_BRIGHTNESS_MSB_B_BRIGHTNESS[bl_level]);
	if (ret < 0) {
		LCD_KIT_ERR("write lm3697 backlight level msb:0x%x failed\n", LM3697_REG_BRIGHTNESS_MSB_B_BRIGHTNESS[bl_level]);
	}
	/*if set backlight level 0, disable lm3697*/
	if (true == lm3697_init_status && 0 == bl_level) {
		lm3697_disable();
	}
	up(&(lm3697_g_chip->test_sem));

	last_bl_level = bl_level;
	return ret;
}

static struct lcd_kit_bl_ops bl_ops = {
	.set_backlight = lm3697_set_backlight,
};

static int lm3697_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = NULL;
	struct lm3697_chip_data *pchip = NULL;
	int ret = -1;
	struct device_node *np = NULL;

	LCD_KIT_INFO("in %s!\n",__func__);

	if(!client){
		LCD_KIT_ERR("client is null pointer\n");
		return -1;
	}
	adapter = client->adapter;

	np = client->dev.of_node;
	if(!np){
		LCD_KIT_ERR("np is null pointer\n");
		return -1;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c functionality check fail.\n");
		return -EOPNOTSUPP;
	}

	pchip = devm_kzalloc(&client->dev, sizeof(struct lm3697_chip_data), GFP_KERNEL);
	if (!pchip)
		return -ENOMEM;

#ifdef CONFIG_REGMAP_I2C
	pchip->regmap = devm_regmap_init_i2c(client, &lm3697_regmap);
	if (IS_ERR(pchip->regmap)) {
		ret = PTR_ERR(pchip->regmap);
		dev_err(&client->dev, "fail : allocate register map: %d\n", ret);
		goto err_out;
	}
#endif

	lm3697_g_chip = pchip;
	pchip->client = client;
	i2c_set_clientdata(client, pchip);

	sema_init(&(pchip->test_sem), 1);

	pchip->dev = device_create(lm3697_class, NULL, 0, "%s", client->name);
	if (IS_ERR(pchip->dev)) {
		/* Not fatal */
		LCD_KIT_ERR("Unable to create device; errno = %ld\n", PTR_ERR(pchip->dev));
		pchip->dev = NULL;
	} else {
		dev_set_drvdata(pchip->dev, pchip);
		ret = sysfs_create_group(&pchip->dev->kobj, &lm3697_group);
		if (ret)
			goto err_sysfs;
	}

	memset(&lm3697_bl_info, 0, sizeof(struct lm3697_backlight_information));

	ret = lm3697_parse_dts(np);
	if (ret < 0) {
		LCD_KIT_ERR("parse lm3697 dts failed");
		goto err_sysfs;
	}

	if (lm3697_bl_info.lm3697_hw_en) {
		ret = gpio_request(lm3697_bl_info.lm3697_hw_en_gpio, NULL);
		if (ret) {
			LCD_KIT_ERR("lm3697 Could not request lm3697 hw_en_gpio\n");
			goto err_sysfs;
		}
	}
	lcd_kit_bl_register(&bl_ops);

	return ret;

err_sysfs:
	LCD_KIT_ERR("sysfs error!\n");
	device_destroy(lm3697_class, 0);
err_out:
	devm_kfree(&client->dev, pchip);
	return ret;
}

static int lm3697_remove(struct i2c_client *client)
{
	if(!client){
		LCD_KIT_ERR("client is null pointer\n");
		return -1;
	}

	sysfs_remove_group(&client->dev.kobj, &lm3697_group);

	return 0;
}


static const struct i2c_device_id lm3697_id[] = {
	{LM3697_NAME, 0},
	{},
};

static const struct of_device_id lm3697_of_id_table[] = {
	{.compatible = "ti,lm3697"},
	{},
};

MODULE_DEVICE_TABLE(i2c, lm3697_id);
static struct i2c_driver lm3697_i2c_driver = {
		.driver = {
			.name = "lm3697",
			.owner = THIS_MODULE,
			.of_match_table = lm3697_of_id_table,
		},
		.probe = lm3697_probe,
		.remove = lm3697_remove,
		.id_table = lm3697_id,
};

static int __init lm3697_init(void)
{
	int ret = 0;

	LCD_KIT_INFO("%s in!\n",__func__);
	lm3697_class = class_create(THIS_MODULE, "lm3697");
	if (IS_ERR(lm3697_class)) {
		LCD_KIT_ERR("Unable to create lm3697 class; errno = %ld\n", PTR_ERR(lm3697_class));
		lm3697_class = NULL;
	}

	ret = i2c_add_driver(&lm3697_i2c_driver);
	if (ret) {
		LCD_KIT_ERR("Unable to register lm3697 driver\n");
	}
	LCD_KIT_INFO("%s ok!\n",__func__);
	return ret;
}

static void __exit lm3697_exit(void)
{
	i2c_del_driver(&lm3697_i2c_driver);
}

module_init(lm3697_init);
module_exit(lm3697_exit);

MODULE_AUTHOR("kinet-ic.com");
MODULE_LICENSE("GPL");
