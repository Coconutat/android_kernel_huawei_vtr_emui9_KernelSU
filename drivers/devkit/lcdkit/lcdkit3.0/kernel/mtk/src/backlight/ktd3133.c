/*
 * drivers/display/hisi/backlight/ktd3133.c
 *
 * ktd3133 driver reffer to lcd
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
#include "ktd3133.h"
#include "lcm_drv.h"

static char *ktd3133_dts_string[KTD3133_RW_REG_MAX] = {
	"ktd3133_reg_control",
	"ktd3133_reg_lsb",
	"ktd3133_reg_msb",
	"ktd3133_reg_pwm",
	"ktd3133_reg_ramp_on",
	"ktd3133_reg_transit_ramp",
	"ktd3133_reg_mode",
};

static unsigned int ktd3133_reg_addr[KTD3133_RW_REG_MAX] = {
	KTD3133_REG_CONTROL,
	KTD3133_REG_RATIO_LSB,
	KTD3133_REG_RATIO_MSB,
	KTD3133_REG_PWM,
	KTD3133_REG_RAMP_ON,
	KTD3133_REG_TRANS_RAMP,
	KTD3133_REG_MODE,
};

static struct ktd3133_backlight_information ktd3133_bl_info;
struct class *ktd3133_class = NULL;
struct ktd3133_chip_data *ktd3133_g_chip = NULL;
static bool ktd3133_init_status = true;
extern struct LCM_DRIVER lcdkit_mtk_common_panel;

static int ktd3133_parse_dts(struct device_node *np)
{
	int ret = 0;
	int i = 0;

	if(np == NULL){
		LCD_KIT_ERR("np is null pointer\n");
		return -1;
	}

	for (i = 0;i < KTD3133_RW_REG_MAX;i++ ) {
		ret = of_property_read_u32(np, ktd3133_dts_string[i], &ktd3133_bl_info.ktd3133_reg[i]);
		if (ret < 0) {
			ktd3133_bl_info.ktd3133_reg[i] = 0xffff;//init to invalid data
			LCD_KIT_INFO("can not find config:%s\n", ktd3133_dts_string[i]);
		}
	}

	ret = of_property_read_u32(np, KTD3133_HW_ENABLE, &ktd3133_bl_info.ktd3133_hw_en);
	if (ret < 0) {
		LCD_KIT_ERR("get ktd3133_hw_enable dts config failed\n");
		return ret;
	}

	ret = of_property_read_u32(np, KTD3133_HW_EN_GPIO, &ktd3133_bl_info.ktd3133_hw_en_gpio);
	if (ret < 0) {
		LCD_KIT_ERR("get ktd3133_hw_en_gpio dts config failed\n");
		return ret;
	}

	/*gpio number offset*/
	ktd3133_bl_info.ktd3133_hw_en_gpio += ((struct mtk_panel_info *)(lcdkit_mtk_common_panel.panel_info))->gpio_offset;
	ret = of_property_read_u32(np, KTD3133_HW_EN_DELAY, &ktd3133_bl_info.bl_on_lk_mdelay);
	if (ret < 0) {
		LCD_KIT_ERR("get bl_on_kernel_mdelay dts config failed\n");
		return ret;
	}

	return ret;
}

static int ktd3133_config_write(struct ktd3133_chip_data *pchip,
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
				LCD_KIT_ERR("write ktd3133 backlight config register 0x%x failed\n",reg[i]);
				goto exit;
			}
		}
	}

exit:
	return ret;
}

static int ktd3133_config_read(struct ktd3133_chip_data *pchip,
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
			LCD_KIT_ERR("read ktd3133 backlight config register 0x%x failed",reg[i]);
			goto exit;
		} else {
			LCD_KIT_INFO("read 0x%x value = 0x%x\n", reg[i], val[i]);
		}
	}

exit:
	return ret;
}

/* initialize chip */
static int ktd3133_chip_init(struct ktd3133_chip_data *pchip)
{
	int ret = -1;

	LCD_KIT_INFO("%s in!\n",__func__);

	if(pchip == NULL){
		LCD_KIT_ERR("pchip is null pointer\n");
		return -1;
	}

	ret = ktd3133_config_write(pchip, ktd3133_reg_addr, ktd3133_bl_info.ktd3133_reg, KTD3133_RW_REG_MAX);
	if (ret < 0) {
		LCD_KIT_ERR("ktd3133 config register failed");
		goto out;
	}
	LCD_KIT_INFO("%s ok!\n",__func__);
	return ret;

out:
	dev_err(pchip->dev, "i2c failed to access register\n");
	return ret;
}

static ssize_t ktd3133_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct ktd3133_chip_data *pchip = NULL;
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

	ret = ktd3133_config_read(pchip, ktd3133_reg_addr, ktd3133_bl_info.ktd3133_reg, KTD3133_RW_REG_MAX);
	if (ret < 0) {
		LCD_KIT_ERR("ktd3133 config read failed");
		goto i2c_error;
	}

	ret = snprintf(buf, PAGE_SIZE, "Device control(0x02)= 0x%x\nEprom Configuration0(0x03) = 0x%x\n \
			\rEprom Configuration1(0x04) = 0x%x\nEprom Configuration2(0x05) = 0x%x\n \
			\rEprom Configuration3(0x06) = 0x%x\nEprom Configuration4(0x07) = 0x%x\n \
			\rEprom Configuration5(0x08) = 0x%x\n",
			ktd3133_bl_info.ktd3133_reg[6], ktd3133_bl_info.ktd3133_reg[0], ktd3133_bl_info.ktd3133_reg[1], ktd3133_bl_info.ktd3133_reg[2], ktd3133_bl_info.ktd3133_reg[3], ktd3133_bl_info.ktd3133_reg[4],ktd3133_bl_info.ktd3133_reg[5]);
	return ret;

i2c_error:
	ret = snprintf(buf, PAGE_SIZE,"%s: i2c access fail to register\n", __func__);
	return ret;
}

static ssize_t ktd3133_reg_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct ktd3133_chip_data *pchip = NULL;
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

static DEVICE_ATTR(reg, (S_IRUGO|S_IWUSR), ktd3133_reg_show, ktd3133_reg_store);

/* pointers to created device attributes */
static struct attribute *ktd3133_attributes[] = {
	&dev_attr_reg.attr,
	NULL,
};

static const struct attribute_group ktd3133_group = {
	.attrs = ktd3133_attributes,
};

static const struct regmap_config ktd3133_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
};

static void ktd3133_enable(void)
{
	int ret = 0;

	if(ktd3133_bl_info.ktd3133_hw_en)
	{
		ret = gpio_request(ktd3133_bl_info.ktd3133_hw_en_gpio, NULL);
		if (ret) {
			LCD_KIT_ERR("ktd3133 Could not request ktd3133 hw_en_gpio\n");
		}
		ret = gpio_direction_output(ktd3133_bl_info.ktd3133_hw_en_gpio, GPIO_DIR_OUT);
		if (ret) {
			LCD_KIT_ERR("ktd3133 Could not set ktd3133 gpio output not success.\n");
		}
		gpio_set_value(ktd3133_bl_info.ktd3133_hw_en_gpio, GPIO_OUT_ONE);
		if(ktd3133_bl_info.bl_on_lk_mdelay)
		{
			mdelay(ktd3133_bl_info.bl_on_lk_mdelay);
		}
	}
	/* chip initialize */
	ret = ktd3133_chip_init(ktd3133_g_chip);
	if (ret < 0) {
		LCD_KIT_ERR("ktd3133_chip_init fail!\n");
		return ;
	}
	ktd3133_init_status = true;
}

static void ktd3133_disable(void)
{
	if(ktd3133_bl_info.ktd3133_hw_en)
	{
		gpio_set_value(ktd3133_bl_info.ktd3133_hw_en_gpio, GPIO_OUT_ZERO);
		gpio_free(ktd3133_bl_info.ktd3133_hw_en_gpio);
	}
	ktd3133_init_status = false;
}

int ktd3133_set_backlight(uint32_t bl_level)
{
	static int last_bl_level = 0;
	int ret = 0;

	if (!ktd3133_g_chip) {
		LCD_KIT_ERR("ktd3133_g_chip is null\n");
		return -1;
	}
	if (down_trylock(&(ktd3133_g_chip->test_sem))) {
		LCD_KIT_INFO("Now in test mode\n");
		return 0;
	}
	/*first set backlight, enable ktd3133*/
	if (false == ktd3133_init_status && bl_level > 0) {
		ktd3133_enable();
	}
	/*set backlight level*/
	ret = regmap_write(ktd3133_g_chip->regmap, KTD3133_REG_RATIO_LSB, KTD3133_REG_RATIO_LSB_BRIGHTNESS[bl_level]);
	if (ret < 0) {
		LCD_KIT_ERR("write ktd3133 backlight level lsb:0x%x failed\n", KTD3133_REG_RATIO_LSB_BRIGHTNESS[bl_level]);
	}
	ret = regmap_write(ktd3133_g_chip->regmap, KTD3133_REG_RATIO_MSB, KTD3133_REG_RATIO_MSB_BRIGHTNESS[bl_level]);
	if (ret < 0) {
		LCD_KIT_ERR("write ktd3133 backlight level msb:0x%x failed\n", KTD3133_REG_RATIO_MSB_BRIGHTNESS[bl_level]);
	}
	/*if set backlight level 0, disable ktd3133*/
	if (true == ktd3133_init_status && 0 == bl_level) {
		ktd3133_disable();
	}
	up(&(ktd3133_g_chip->test_sem));

	last_bl_level = bl_level;
	return ret;
}

static struct lcd_kit_bl_ops bl_ops = {
	.set_backlight = ktd3133_set_backlight,
};

static int ktd3133_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = NULL;
	struct ktd3133_chip_data *pchip = NULL;
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

	pchip = devm_kzalloc(&client->dev, sizeof(struct ktd3133_chip_data), GFP_KERNEL);
	if (!pchip)
		return -ENOMEM;

#ifdef CONFIG_REGMAP_I2C
	pchip->regmap = devm_regmap_init_i2c(client, &ktd3133_regmap);
	if (IS_ERR(pchip->regmap)) {
		ret = PTR_ERR(pchip->regmap);
		dev_err(&client->dev, "fail : allocate register map: %d\n", ret);
		goto err_out;
	}
#endif

	ktd3133_g_chip = pchip;
	pchip->client = client;
	i2c_set_clientdata(client, pchip);

	sema_init(&(pchip->test_sem), 1);

	pchip->dev = device_create(ktd3133_class, NULL, 0, "%s", client->name);
	if (IS_ERR(pchip->dev)) {
		/* Not fatal */
		LCD_KIT_ERR("Unable to create device; errno = %ld\n", PTR_ERR(pchip->dev));
		pchip->dev = NULL;
	} else {
		dev_set_drvdata(pchip->dev, pchip);
		ret = sysfs_create_group(&pchip->dev->kobj, &ktd3133_group);
		if (ret)
			goto err_sysfs;
	}

	memset(&ktd3133_bl_info, 0, sizeof(struct ktd3133_backlight_information));

	ret = ktd3133_parse_dts(np);
	if (ret < 0) {
		LCD_KIT_ERR("parse ktd3133 dts failed");
		goto err_sysfs;
	}

	if (ktd3133_bl_info.ktd3133_hw_en) {
		ret = gpio_request(ktd3133_bl_info.ktd3133_hw_en_gpio, NULL);
		if (ret) {
			LCD_KIT_ERR("ktd3133 Could not request ktd3133 hw_en_gpio\n");
			goto err_sysfs;
		}
	}
	lcd_kit_bl_register(&bl_ops);

	return ret;

err_sysfs:
	LCD_KIT_ERR("sysfs error!\n");
	device_destroy(ktd3133_class, 0);
err_out:
	devm_kfree(&client->dev, pchip);
	return ret;
}

static int ktd3133_remove(struct i2c_client *client)
{
	if(!client){
		LCD_KIT_ERR("client is null pointer\n");
		return -1;
	}

	sysfs_remove_group(&client->dev.kobj, &ktd3133_group);

	return 0;
}


static const struct i2c_device_id ktd3133_id[] = {
	{KTD3133_NAME, 0},
	{},
};

static const struct of_device_id ktd3133_of_id_table[] = {
	{.compatible = "ktd,ktd3133"},
	{},
};

MODULE_DEVICE_TABLE(i2c, ktd3133_id);
static struct i2c_driver ktd3133_i2c_driver = {
		.driver = {
			.name = "ktd3133",
			.owner = THIS_MODULE,
			.of_match_table = ktd3133_of_id_table,
		},
		.probe = ktd3133_probe,
		.remove = ktd3133_remove,
		.id_table = ktd3133_id,
};

static int __init ktd3133_init(void)
{
	int ret = 0;

	LCD_KIT_INFO("%s in!\n",__func__);
	ktd3133_class = class_create(THIS_MODULE, "ktd3133");
	if (IS_ERR(ktd3133_class)) {
		LCD_KIT_ERR("Unable to create ktd3133 class; errno = %ld\n", PTR_ERR(ktd3133_class));
		ktd3133_class = NULL;
	}

	ret = i2c_add_driver(&ktd3133_i2c_driver);
	if (ret) {
		LCD_KIT_ERR("Unable to register ktd3133 driver\n");
	}
	LCD_KIT_INFO("%s ok!\n",__func__);
	return ret;
}

static void __exit ktd3133_exit(void)
{
	i2c_del_driver(&ktd3133_i2c_driver);
}

module_init(ktd3133_init);
module_exit(ktd3133_exit);

MODULE_AUTHOR("kinet-ic.com");
MODULE_LICENSE("GPL");
