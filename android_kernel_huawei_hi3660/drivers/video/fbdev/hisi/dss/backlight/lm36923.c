/*
* Simple driver for Texas Instruments LM3639 Backlight + Flash LED driver chip
* Copyright (C) 2012 Texas Instruments
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/leds.h>
#include <linux/backlight.h>
#include <linux/err.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/interrupt.h>
#include <linux/regmap.h>
#include <linux/semaphore.h>
#include "lm36923.h"
#include "../hisi_fb.h"
#include <dsm/dsm_pub.h>
extern struct dsm_client *lcd_dclient;

struct class *lm36923_class;
struct lm36923_chip_data *g_pchip;
static int lcd_fake_panel_enable_reg = 0x3;
static bool lm36923_init_status = false;
static unsigned int g_reg_val[LM36923_RW_REG_MAX] = {0,0,0xf,0x85,0xa3,0x79,0,0,0,0,0,0,4,0};
static int lm36923_hw_en_gpio = 0;
#define GPIO_LM36923_EN_NAME "lm36923_hw_en"
#define ENABLE_REG_DEFAULT	0x0
#define LSB_REG_DEFAULT	0x0
#define MSB_REG_DEFAULT	0x0
static struct gpio_desc lm36923_hw_en_on_cmds[] = {
	{DTYPE_GPIO_REQUEST, WAIT_TYPE_US, 0,
		GPIO_LM36923_EN_NAME, &lm36923_hw_en_gpio, 0},
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 10,
		GPIO_LM36923_EN_NAME, &lm36923_hw_en_gpio, 1},
};

static struct gpio_desc lm36923_hw_en_disable_cmds[] =
{
	{DTYPE_GPIO_OUTPUT, WAIT_TYPE_US, 0,
		GPIO_LM36923_EN_NAME, &lm36923_hw_en_gpio, 0},
	{DTYPE_GPIO_INPUT, WAIT_TYPE_US, 10,
		GPIO_LM36923_EN_NAME, &lm36923_hw_en_gpio, 0},
};

static struct gpio_desc lm36923_hw_en_free_cmds[] =
{
	{DTYPE_GPIO_FREE, WAIT_TYPE_US, 50,
		GPIO_LM36923_EN_NAME, &lm36923_hw_en_gpio, 0},
};

/*lint -save -e502 -e568 -e570 -e650 -e685*/
/*
** for debug, S_IRUGO
** /sys/module/hisifb/parameters
*/
unsigned lm36923_msg_level = 7;
module_param_named(debug_lm36923_msg_level, lm36923_msg_level, int, 0644);
MODULE_PARM_DESC(debug_lm36923_msg_level, "backlight lm36923 msg level");
uint32_t last_brightness = -1;
static int lm36923_led_num = 0;

/* initialize chip */
static int lm36923_chip_init(struct lm36923_chip_data *pchip)
{
	int ret = -1;
	int reten = -1;
	struct device_node *np = NULL;
	uint32_t lm36923_ctrl_mode = 0;
	int enable_reg = 0xF;
	int protect_disable_flag = 0;
	int disable_ovp = 0;
	int lm36923_boost_ctrl_value = 0;
	int lm36923_boost_ctrl_dbc = 0;
	int fault_ctrl = 0;

	LM36923_INFO("in!\n");

	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return ret;
	}

	np = of_find_compatible_node(NULL, NULL, DTS_COMP_TI_LM36923);
	if (!np) {
		LM36923_ERR("NOT FOUND device node %s!\n", DTS_COMP_TI_LM36923);
		goto out;
	}

	ret = of_property_read_u32(np, LM36923_CTRL_MODE, &lm36923_ctrl_mode);
	if (ret) {
		LM36923_ERR("get lm36923_ctrl_mode failed!\n");
		lm36923_ctrl_mode = BRT_RAMP_MUL_MODE;
	}

	LM36923_INFO("lm36923_ctrl_mode = %d\n", lm36923_ctrl_mode);

	ret = of_property_read_u32(np, "fake_panel_enable_reg", &lcd_fake_panel_enable_reg);
	if (ret) {
		lcd_fake_panel_enable_reg = 0x3;
	}
	LM36923_INFO("fake_panel_enable_reg = %d\n", lcd_fake_panel_enable_reg);

	/* set Brightness Control register*/
	if (BRT_REG_ONLY_MODE == lm36923_ctrl_mode) {
		ret = regmap_update_bits(pchip->regmap, REG_BRT_CTR, MASK_CHANGE_MODE,
			LM36923_BLED_MODE_EXPONETIAL|BRT_REG_ONLY|LM36923_RAMP_DIS|RAMP_RATE_05|BL_ADJ_HIGHT);
	} else if (BRT_RAMP_MUL_MODE == lm36923_ctrl_mode) {
		ret = regmap_update_bits(pchip->regmap, REG_BRT_CTR, MASK_CHANGE_MODE,
			LM36923_BLED_MODE_LINEAR|BRT_RAMP_MUL|LM36923_RAMP_DIS|RAMP_RATE_05|BL_ADJ_HIGHT);
	}

	ret = of_property_read_u32(np, TI_LM36923_LED_NUM, &lm36923_led_num);
	if (ret < 0) {
		LM36923_ERR("get lm36923 led num not config, use default 3 leds\n");
		enable_reg = 0xF;
	}else{
		switch(lm36923_led_num){
			case TI_LM36923_LED_TWO:
				enable_reg = 0x7;
				break;
			case TI_LM36923_LED_THREE:
			default:
				enable_reg = 0xF;
				break;
		}
		g_reg_val[2] = enable_reg;
		LM36923_INFO("lm36923 led enable  =  0x%x\n", enable_reg);
	}

	if (g_fake_lcd_flag) {
		LM36923_INFO("is unknown lcd\n");
		enable_reg = 0;
	}

	ret = of_property_read_u32(np, TI_LM36923_BOOST_CTRL_1, &lm36923_boost_ctrl_value);
	if (ret < 0) {
		LM36923_INFO("can not get lm36923 REG_BOOST_CTR1, use default value\n");
	}else{
		if (g_fake_lcd_flag){
			ret = of_property_read_u32(np, "lm36923-boost-ctrl-dbc", &lm36923_boost_ctrl_dbc);
			if (ret) {
				LM36923_INFO("can not get lm36923-boost-ctrl-dbc!\n");
			}else {
				lm36923_boost_ctrl_value = lm36923_boost_ctrl_dbc;
			}
		}
		LM36923_INFO("get lm36923 REG_BOOST_CTR1 = 0x%x\n", lm36923_boost_ctrl_value);
		g_reg_val[5] = lm36923_boost_ctrl_value;
		ret = regmap_write(pchip->regmap, REG_BOOST_CTR1, lm36923_boost_ctrl_value);
		if (ret < 0)
			goto out;
	}
	LM36923_INFO("lm36923_enable_reg = %d\n", enable_reg);
	ret = regmap_write(pchip->regmap, REG_ENABLE, enable_reg);
	if (ret < 0)
		goto out;

	/* set PWM Hysteresis to 0 */
	ret = regmap_update_bits(pchip->regmap, REG_PWM_CTR, MASK_PWM_HYS, LM36923_PWM_HYS_NONE);
	if (ret < 0)
		goto out;

	ret = of_property_read_u32(np, TI_LM36923_PROTECT_DISABLE, &protect_disable_flag);
	LM36923_INFO("lm36923 protect flag = %d\n", protect_disable_flag);
	if (ret < 0) {
		/* Enable OCP/OVP Detect,IC will shutdowm when OCP/OVP occur */
		ret = regmap_write(pchip->regmap, REG_FAULT_CTR, OVP_OCP_SHUTDOWN_ENABLE);
		if (ret < 0)
			goto out;
	}

	ret = of_property_read_u32(np, TI_LM36923_DISABLE_OVP, &disable_ovp);
	LM36923_INFO("lm36923 disable ovp = %d\n", disable_ovp);
	if (ret < 0) {
		LM36923_ERR("lm36923 disable ovp flag not config, use default config\n");
		/* get disable_ovp flag failed is not error ,so ret = 0 */
		ret = 0;
	} else {
		ret = regmap_write(pchip->regmap, REG_FAULT_CTR, OCP_SHUTDOWN_OVP_DISABLE);
		if (ret < 0) {
			goto out;
		}
	}

	ret = of_property_read_u32(np, TI_LM36923_FAULT_CTRL, &fault_ctrl);
	g_reg_val[12] = fault_ctrl;
	LM36923_INFO("lm36923 fault ctrl = 0x%x, ret = %d.\n", fault_ctrl, ret);
	if (ret == 0) {
		ret = regmap_write(pchip->regmap, REG_FAULT_CTR, fault_ctrl);
		if (ret < 0) {
			goto out;
		}
	} else {
		LM36923_INFO("lm36923 use default fault config\n");
		ret = 0;
	}
	reten = of_property_read_u32(np, "lm36923_hw_en_gpio", &lm36923_hw_en_gpio);
	if (reten < 0) {
		LM36923_INFO("get lm36923 hw en false\n");
	}
	LM36923_INFO("ok!\n");

	return ret;
out:
	dev_err(pchip->dev, "i2c failed to access register\n");
	return ret;
}

/**
 * lm36923_set_backlight_mode(): Set Backlight working mode
 *
 * @bl_mode: BRT_PWM_ONLY
 *		BRT_REG_ONLY
 *		BRT_MUL_RAMP
 * 		BRT_RAMP_MUL
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36923_set_backlight_mode(uint32_t bl_mode)
{
	uint32_t mode = 0;
	ssize_t ret = -1;
	unsigned int ctrl_mode[4] = {0x00,0x20,0x40,0x60};

	if (!lm36923_init_status) {
		LM36923_ERR("init fail, return.\n");
		return ret;
	}

	if(g_pchip == NULL){
		LM36923_ERR("g_chip is null pointer\n");
		return ret;
	}

	mode = bl_mode;

	if (mode > BRT_RAMP_MUL_MODE) {
		dev_err(g_pchip->dev, "%s:input unvalid\n", __func__);
		return -EINVAL;
	}

	/* Update IC control mode */
	ret = regmap_update_bits(g_pchip->regmap, REG_BRT_CTR, MASK_BRT_MODE,ctrl_mode[mode]);
	if (ret < 0) {
		dev_err(g_pchip->dev, "%s:regmap update BRT_REG_ONLY_MODE failure\n", __func__);
		goto i2c_error;
	}

	return ret;

i2c_error:
	dev_err(g_pchip->dev, "i2c failed to access register\n");
	return ret;
}
EXPORT_SYMBOL(lm36923_set_backlight_mode);

ssize_t lm36923_resume_init(struct lm36923_chip_data *pchip)
{
	int ret = 0;

	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return -1;
	}

	ret = regmap_write(pchip->regmap, REG_ENABLE,ENABLE_REG_DEFAULT);
	if (ret < 0)
		goto out;

	ret = regmap_write(pchip->regmap, REG_BRT_CTR, g_reg_val[3]);
	if (ret < 0)
		goto out;

	ret = regmap_write(pchip->regmap, REG_BOOST_CTR1, g_reg_val[5]);
	if (ret < 0)
		goto out;

	ret = regmap_update_bits(pchip->regmap, REG_BRT_VAL_L, MASK_BL_LSB,LSB_REG_DEFAULT);
	if (ret < 0) {
		goto out;
	}

	ret = regmap_write(pchip->regmap, REG_BRT_VAL_M, MSB_REG_DEFAULT);
	if (ret < 0) {
		goto out;
	}

	ret = regmap_write(pchip->regmap, REG_ENABLE, g_reg_val[2]);
	if (ret < 0)
		goto out;

	ret = regmap_write(pchip->regmap, REG_PWM_CTR, g_reg_val[4]);
	if (ret < 0)
		goto out;

	ret = regmap_write(pchip->regmap, REG_FAULT_CTR, g_reg_val[12]);
	if (ret < 0)
		goto out;

	return ret;

out:
	dev_err(pchip->dev, "i2c failed to access register\n");
	return ret;
}

/**
 * lm36923_set_backlight_init(): initial ic working mode
 *
 * @bl_level: value for backlight ,range from 0 to ~
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */

ssize_t lm36923_set_backlight_init(uint32_t bl_level)
{
	int ret = 0;

	if (g_fake_lcd_flag) {
		LM36923_INFO("is fake lcd!\n");
		return ret;
	}

	if(g_pchip == NULL){
		LM36923_ERR("g_chip is null pointer\n");
		return -1;
	}

	if (down_trylock(&(g_pchip->test_sem))) {
		LM36923_INFO("Now in test mode\n");
		return 0;
	}
	if (false == lm36923_init_status && bl_level > 0) {
		LM36923_INFO("get lm36923 hw en %d, enable hw en\n", lm36923_hw_en_gpio);
		gpio_cmds_tx(lm36923_hw_en_on_cmds, \
		ARRAY_SIZE(lm36923_hw_en_on_cmds));
		/* chip resume initialize */
		LM36923_INFO("resume in!\n");
		ret = lm36923_resume_init(g_pchip);
		if (ret < 0) {
			LM36923_INFO( "fail : chip init\n");
			goto out;
		}
		LM36923_INFO("resume ok!\n");
		lm36923_init_status = true;
	} else if (true == lm36923_init_status && 0 == bl_level) {
		gpio_cmds_tx(lm36923_hw_en_disable_cmds, ARRAY_SIZE(lm36923_hw_en_disable_cmds));
		gpio_cmds_tx(lm36923_hw_en_free_cmds, ARRAY_SIZE(lm36923_hw_en_free_cmds));

		lm36923_init_status = false;
	} else {
		LM36923_DEBUG("lm36923_chip_init %u, 0: already off; else : already init!\n", bl_level);
	}

out:
	up(&(g_pchip->test_sem));
	return ret;
}
/**
 * lm36923_set_backlight_reg(): Set Backlight working mode
 *
 * @bl_level: value for backlight ,range from 0 to 2047
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36923_set_backlight_reg(uint32_t bl_level)
{
	ssize_t ret = -1;
	uint32_t level = 0;
	int bl_msb = 0;
	int bl_lsb = 0;
	static int last_level = -1;
	static int enable_flag = 0;
	static int disable_flag = 0;

	if (!lm36923_init_status) {
		LM36923_ERR("init fail, return.\n");
		return ret;
	}

	if(g_pchip == NULL){
		LM36923_ERR("g_chip is null pointer\n");
		return ret;
	}

	if (down_trylock(&(g_pchip->test_sem))) {
		LM36923_INFO("Now in test mode\n");
		return 0;
	}

	level = bl_level;

	if (level > BL_MAX) {
		level = BL_MAX;
	}

	if (g_fake_lcd_flag) {
		if (level > 0) {
			if (!enable_flag) {
				ret = regmap_write(g_pchip->regmap, REG_ENABLE, lcd_fake_panel_enable_reg);
				LM36923_INFO("REG_ENABLE = %d\n", lcd_fake_panel_enable_reg);
				mdelay(16);
			}
			enable_flag = 1;
			disable_flag = 0;
		} else {
			if (!disable_flag) {
				ret = regmap_write(g_pchip->regmap, REG_ENABLE, 0x0);
				LM36923_INFO("REG_ENABLE = 0x0\n");
				mdelay(16);
			}
			disable_flag = 1;
			enable_flag = 0;
		}
	}

	/* 11-bit brightness code */
	bl_msb = level >> 3;
	bl_lsb = level & 0x07;

	if ((BL_MIN == last_level && LOG_LEVEL_INFO == lm36923_msg_level)
		|| (BL_MIN == level && LOG_LEVEL_INFO == lm36923_msg_level)
		|| (-1 == last_level)) {
		LM36923_INFO("level = %d, bl_msb = %d, bl_lsb = %d\n", level, bl_msb, bl_lsb);
	}

	LM36923_DEBUG("level = %d, bl_msb = %d, bl_lsb = %d\n", level, bl_msb, bl_lsb);

	ret = regmap_update_bits(g_pchip->regmap, REG_BRT_VAL_L, MASK_BL_LSB,bl_lsb);
	if (ret < 0) {
		goto i2c_error;
	}

	ret = regmap_write(g_pchip->regmap, REG_BRT_VAL_M, bl_msb);
	if (ret < 0) {
		goto i2c_error;
	}

	last_level = level;
	up(&(g_pchip->test_sem));
	return ret;

i2c_error:
	up(&(g_pchip->test_sem));
	dev_err(g_pchip->dev, "%s:i2c access fail to register\n", __func__);
	return ret;
}
EXPORT_SYMBOL(lm36923_set_backlight_reg);

/**
 * lm36923_set_reg(): Set lm36923 reg
 *
 * @bl_reg: which reg want to write
 * @bl_mask: which bits of reg want to change
 * @bl_val: what value want to write to the reg
 *
 * A value of zero will be returned on success, a negative errno will
 * be returned in error cases.
 */
ssize_t lm36923_set_reg(u8 bl_reg,u8 bl_mask,u8 bl_val)
{
	ssize_t ret = -1;
	u8 reg = bl_reg;
	u8 mask = bl_mask;
	u8 val = bl_val;

	if (!lm36923_init_status) {
		LM36923_ERR("init fail, return.\n");
		return ret;
	}

	if (REG_MAX < reg) {
		LM36923_ERR("Invalid argument!!!\n");
		return ret;
	}

	LM36923_INFO("%s:reg=0x%x,mask=0x%x,val=0x%x\n", __func__, reg, mask, val);

	ret = regmap_update_bits(g_pchip->regmap, reg, mask, val);
	if (ret < 0) {
		LM36923_ERR("i2c access fail to register\n");
		return ret;
	}

	return ret;
}
EXPORT_SYMBOL(lm36923_set_reg);

/**
 * lm36923_ramp_brightness(): Use RampRate func
 *
 * @cur_brightness: current brightness
 *
 */
void lm36923_ramp_brightness(uint32_t cur_brightness)
{
    int i = 0;
    uint32_t amount_brightness = 0;
    uint32_t ramp_mode[MAX_RATE_NUM] = {1,2,4,8,16,32,64,128,299};
    u8 ramp_rate[MAX_RATE_NUM] = {RAMP_RATE_16,RAMP_RATE_8,
		RAMP_RATE_4,RAMP_RATE_2,RAMP_RATE_1,
		RAMP_RATE_05,RAMP_RATE_025,RAMP_RATE_0125,RAMP_RATE_0125};

	if (!lm36923_init_status) {
		LM36923_ERR("init fail, return.\n");
		return;
	}

    if (last_brightness != -1){
        amount_brightness = abs(cur_brightness - last_brightness);
        for(i=0;i<MAX_RATE_NUM;i++){
            if (ramp_mode[i] >= amount_brightness ){
                lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_EN,LM36923_RAMP_EN);
                lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_RATE,ramp_rate[i]);
                last_brightness = cur_brightness;
                LM36923_INFO("amount_brightness=%d,ramp_rate=0x%x\n",
					amount_brightness,ramp_rate[i]);
                break;
            } else {
                last_brightness = cur_brightness;
                lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_EN,LM36923_RAMP_DIS);
            }
        }
    }else{
        last_brightness = cur_brightness;
        lm36923_set_reg(REG_BRT_CTR,MASK_RAMP_EN,LM36923_RAMP_DIS);
    }

    return;
}
EXPORT_SYMBOL(lm36923_ramp_brightness);

static ssize_t lm36923_bled_mode_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36923_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	uint32_t mode = 0;
	ssize_t ret = -1;

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_read(pchip->regmap, REG_BRT_CTR, &mode);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36923 I2C read error\n");

	return snprintf(buf, PAGE_SIZE, "LM36923 bl_mode:0x%x\n",mode);
}

static ssize_t lm36923_bled_mode_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36923_chip_data *pchip = NULL;
	uint32_t mode = 0;

	if(dev == NULL || g_pchip == NULL){
		LM36923_ERR("dev or g_chip is null pointer\n");
		return ret;
	}

	pchip = dev_get_drvdata(dev);
	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return ret;
	}

	ret = kstrtouint(buf, 16, &mode);
	if (ret)
		goto out_input;

	ret = regmap_update_bits(g_pchip->regmap, REG_BRT_CTR, MASK_CHANGE_MODE, mode);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}

static DEVICE_ATTR(bled_mode, S_IRUGO|S_IWUSR, lm36923_bled_mode_show, lm36923_bled_mode_store);

static ssize_t lm36923_reg_bl_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36923_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret = -1;
	int bl_lsb = 0;
	int bl_msb = 0;
	int bl_level = 0;

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");

	ret = regmap_read(pchip->regmap, REG_BRT_VAL_M, &bl_msb);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36923 I2C read error\n");

	ret = regmap_read(pchip->regmap, REG_BRT_VAL_L, &bl_lsb);
	if(ret < 0)
		return snprintf(buf, PAGE_SIZE, "LM36923 I2C read error\n");

	bl_level = (bl_msb << 3) | bl_lsb;

	return snprintf(buf, PAGE_SIZE, "LM36923 bl_level:%d\n", bl_level);
}

static ssize_t lm36923_reg_bl_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36923_chip_data *pchip = NULL;
	unsigned int bl_level = 0;
	unsigned int bl_msb = 0;
	unsigned int bl_lsb = 0;

	if(dev == NULL){
		LM36923_ERR("dev is null pointer\n");
		return ret;
	}

	pchip = dev_get_drvdata(dev);
	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return ret;
	}

	ret = kstrtouint(buf, 10, &bl_level);
	if (ret)
		goto out_input;

	LM36923_INFO("%s:buf=%s,state=%d\n", __func__, buf, bl_level);

	if (bl_level < BL_MIN)
		bl_level = BL_MIN;

	if (bl_level > BL_MAX)
		bl_level = BL_MAX;

	/* 11-bit brightness code */
	bl_msb = bl_level >> 3;
	bl_lsb = bl_level & 0x07;

	LM36923_INFO("bl_level = %d, bl_msb = %d, bl_lsb = %d\n", bl_level, bl_msb, bl_lsb);

	ret = regmap_update_bits(pchip->regmap, REG_BRT_VAL_L, MASK_BL_LSB, bl_lsb);
	if (ret < 0)
		goto i2c_error;

	ret = regmap_write(pchip->regmap, REG_BRT_VAL_M, bl_msb);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}

static DEVICE_ATTR(reg_bl, S_IRUGO|S_IWUSR, lm36923_reg_bl_show, lm36923_reg_bl_store);

static ssize_t lm36923_reg_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36923_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	ssize_t ret = -1;
	unsigned int val[14] = {0};

	if (!dev)
		return snprintf(buf, PAGE_SIZE, "dev is null\n");

	pchip = dev_get_drvdata(dev);
	if (!pchip)
		return snprintf(buf, PAGE_SIZE, "data is null\n");

	client = pchip->client;
	if(!client)
		return snprintf(buf, PAGE_SIZE, "client is null\n");
	ret = regmap_read(pchip->regmap, 0x00, &val[0]);
	ret = regmap_read(pchip->regmap, 0x01, &val[1]);
	ret = regmap_read(pchip->regmap, 0x10, &val[2]);
	ret = regmap_read(pchip->regmap, 0x11, &val[3]);
	ret = regmap_read(pchip->regmap, 0x12, &val[4]);
	ret = regmap_read(pchip->regmap, 0x13, &val[5]);
	ret = regmap_read(pchip->regmap, 0x14, &val[6]);
	ret = regmap_read(pchip->regmap, 0x15, &val[7]);
	ret = regmap_read(pchip->regmap, 0x16, &val[8]);
	ret = regmap_read(pchip->regmap, 0x17, &val[9]);
	ret = regmap_read(pchip->regmap, 0x18, &val[10]);
	ret = regmap_read(pchip->regmap, 0x19, &val[11]);
	ret = regmap_read(pchip->regmap, 0x1E, &val[12]);
	ret = regmap_read(pchip->regmap, 0x1F, &val[13]);

	return snprintf(buf, PAGE_SIZE, "Revision(0x00)= 0x%x\nSoftware Reset(0x01)= 0x%x\n \
			\rEnable(0x10) = 0x%x\nBrightness Control(0x11) = 0x%x\n \
			\rPWM Control(0x12) = 0x%x\nBoost Control 1(0x13) = 0x%x\n \
			\rBoost Control 2(0x14) = 0x%x\nAuto Frequency High Threshold(0x15) = 0x%x\n \
			\rAuto Frequency High Threshold(0x16)  = 0x%x\nBack Light Adjust Threshold(0x17)  = 0x%x\n \
			\rBrightness Register LSB's(0x18) = 0x%x\nBrightness Register MSB's(0x19) = 0x%x\n \
			\rFault Control(0x1E) = 0x%x\nFault Flags(0x1F) = 0x%x\n",
			val[0],val[1],val[2],val[3],val[4],val[5],val[6],val[7],
			val[8],val[9],val[10],val[11],val[12],val[13]);

}

static ssize_t lm36923_reg_store(struct device *dev,
					struct device_attribute *devAttr,
					const char *buf, size_t size)
{
	ssize_t ret = -1;
	struct lm36923_chip_data *pchip = NULL;
	unsigned int reg = 0;
	unsigned int mask = 0;
	unsigned int val = 0;

	if(dev == NULL){
		LM36923_ERR("dev is null pointer\n");
		return ret;
	}

	pchip = dev_get_drvdata(dev);
	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return ret;
	}

	ret = sscanf(buf, "reg=0x%x, mask=0x%x, val=0x%x",&reg,&mask,&val);
	if (ret < 0) {
		printk("check your input!!!\n");
		goto out_input;
	}

	if (reg > REG_MAX) {
		printk("Invalid argument!!!\n");
		goto out_input;
	}

	LM36923_INFO("%s:reg=0x%x,mask=0x%x,val=0x%x\n", __func__, reg, mask, val);

	ret = regmap_update_bits(pchip->regmap, reg, mask, val);
	if (ret < 0)
		goto i2c_error;

	return size;

i2c_error:
	dev_err(pchip->dev, "%s:i2c access fail to register\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: i2c access fail to register\n", __func__);

out_input:
	dev_err(pchip->dev, "%s:input conversion fail\n", __func__);
	return snprintf((char *)buf, PAGE_SIZE, "%s: input conversion fail\n", __func__);
}
static DEVICE_ATTR(reg, S_IRUGO|S_IWUSR, lm36923_reg_show, lm36923_reg_store);

static int lm36923_test_set_brightness(struct lm36923_chip_data *pchip, int level)
{
	int result = TEST_OK;
	int ret;
	int bl_lsb = 0;
	int bl_msb = 0;

	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return -1;
	}

	/* 11-bit brightness code */
	bl_msb = level >> 3;
	bl_lsb = level & 0x07;

	ret = regmap_update_bits(pchip->regmap, REG_BRT_VAL_L, MASK_BL_LSB, bl_lsb);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	ret = regmap_write(pchip->regmap, REG_BRT_VAL_M, bl_msb);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}
	return result;
}

static int lm36923_test_led_open(struct lm36923_chip_data *pchip, int led)
{
	int ret;
	int result = TEST_OK;
	unsigned int val = 0;
	int enable_reg = 0xF;

	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return -1;
	}

	switch(lm36923_led_num){
		case TI_LM36923_LED_TWO:
			enable_reg = 0x07;
			break;
		case TI_LM36923_LED_THREE:
		default:
			enable_reg = 0x0F;
			break;
	}

	/* Apply power the the LM36923H. */
	/* Enable all LED strings (Register 0x10 = 0x0F). */
	ret = regmap_write(pchip->regmap, REG_ENABLE, enable_reg);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* Set maximum brightness (Register 0x18 = 0x07 and Register 0x19 = 0xFF). */
	ret |= lm36923_test_set_brightness(pchip, BL_MAX);

	/* Set the brightness control (Register 0x11 = 0x00). */
	ret = regmap_write(pchip->regmap, REG_BRT_CTR, 0x00);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* Open LEDx string. */
	ret = regmap_write(pchip->regmap, REG_ENABLE, (~(1<<(1 + led))) & enable_reg);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* Wait 4 msec. */
	msleep(4);

	/* Read LED open fault (Register 0x1F). */
	ret = regmap_read(pchip->regmap, REG_FAULT_FLAG, &val);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* If bit[4] = 1, then a LED open fault condition has been detected. */
	if (val & (1 << 4)) {
		result |= (1<<(4 + led));
	}

	/* Connect LEDx string. */
	ret = regmap_write(pchip->regmap, REG_ENABLE, enable_reg);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return result|TEST_ERROR_I2C;
	}

	/* Repeat the procedure for the other LED strings. */
	msleep(1000);
	return result;
}
/*lint -restore*/
static int lm36923_test_led_short(struct lm36923_chip_data *pchip, int led)
{
	unsigned int val = 0;
	int ret;
	int result = TEST_OK;
	int enable_reg = 0xF;

	if(pchip == NULL){
		LM36923_ERR("pchip is null pointer\n");
		return -1;
	}

	switch(lm36923_led_num){
		case TI_LM36923_LED_TWO:
			enable_reg = 0x07;
			break;
		case TI_LM36923_LED_THREE:
		default:
			enable_reg = 0x0F;
			break;
	}

	/* Apply power the LM36923H. */
	/* Enable only LEDx string (example:LED1 Register 0x10 = 0x03). */
	ret = regmap_write(pchip->regmap, REG_ENABLE, (1<<(1 + led)) | 0x01);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* Enable short fault (Register 0x1E = 0x01. */
	ret = regmap_write(pchip->regmap, REG_FAULT_CTR, enable_reg);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* Set maximum brightness (Register 0x18 = 0x07 and Register 0x19 = 0xFF). */
	ret |= lm36923_test_set_brightness(pchip, BL_MAX);

	/* Set the brightness control (Register 0x11 = 0x00). */
	ret = regmap_write(pchip->regmap, REG_BRT_CTR, 0x00);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* Wait 4 msec. */
	msleep(4);

	/* Read LED short fault (Register 0x1F). */
	ret = regmap_read(pchip->regmap, REG_FAULT_FLAG, &val);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	/* If bit[3] = 1, then a LED short fault condition has been detected. */
	if (val & (1 << 3)) {
		result |= (1<<(7 + led));
	}

	/* Set chip enable and LED string enable low (Register 0x10 = 0x00). */
	ret = regmap_write(pchip->regmap, REG_ENABLE, 0x00);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return result|TEST_ERROR_I2C;
	}

	/* Repeat the procedure for the other LED Strings */
	msleep(1000);
	return result;
}

static int lm36923_test_regs_store(struct lm36923_chip_data *pchip, unsigned int reg_val[])
{
	int ret;

	if((pchip == NULL) || (reg_val == NULL)){
		LM36923_ERR("pchip or reg_val is null pointer\n");
		return -1;
	}

	ret = regmap_read(pchip->regmap, REG_BRT_CTR, &reg_val[0]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}
	ret = regmap_read(pchip->regmap, REG_ENABLE, &reg_val[1]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}
	ret = regmap_read(pchip->regmap, REG_PWM_CTR, &reg_val[2]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	ret = regmap_read(pchip->regmap, REG_FAULT_CTR, &reg_val[3]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	ret = regmap_read(pchip->regmap, REG_BRT_VAL_M, &reg_val[4]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	ret = regmap_read(pchip->regmap, REG_BRT_VAL_L, &reg_val[5]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	LM36923_INFO("REG_BRT_CTR=%d, REG_ENABLE=%d, REG_PWM_CTR=%d, \
				REG_FAULT_CTR=%d, REG_BRT_VAL_M=%d, REG_BRT_VAL_L=%d\n",
				reg_val[0], reg_val[1], reg_val[2], reg_val[3], reg_val[4], reg_val[5]);
	return TEST_OK;
}

static int lm36923_test_regs_restore(struct lm36923_chip_data *pchip, unsigned int reg_val[])
{
	int ret;

	if((pchip == NULL) || (reg_val == NULL)){
		LM36923_ERR("pchip or reg_val is null pointer\n");
		return -1;
	}

	ret = regmap_write(pchip->regmap, REG_FAULT_CTR, reg_val[3]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}
	msleep(10);
	ret = regmap_write(pchip->regmap, REG_BRT_VAL_M, reg_val[4]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}
	ret = regmap_write(pchip->regmap, REG_BRT_VAL_L, reg_val[5]);
	if (ret < 0) {
		LM36923_ERR("TEST_ERROR_I2C\n");
		return TEST_ERROR_I2C;
	}

	return TEST_OK;
}

static ssize_t lm36923_self_test_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	struct lm36923_chip_data *pchip = NULL;
	struct i2c_client *client = NULL;
	int result = TEST_OK;
	int i;
	unsigned int reg_val[14] = {0};
	int ret;
	int led_num = 3;

	LM36923_INFO("self test in\n");

	if (!lm36923_init_status) {
		LM36923_ERR("init fail, return.\n");
		result |= TEST_ERROR_CHIP_INIT;
		return snprintf(buf, PAGE_SIZE, "%d\n", result);
	}

	if (!dev) {
		result = TEST_ERROR_DEV_NULL;
		LM36923_ERR("TEST_ERROR_DEV_NULL\n");
		goto lm36923_test_out;
	}

	pchip = dev_get_drvdata(dev);
	if (!pchip) {
		result = TEST_ERROR_DATA_NULL;
		LM36923_ERR("TEST_ERROR_DATA_NULL\n");
		goto lm36923_test_out;
	}

	client = pchip->client;
	if(!client) {
		result = TEST_ERROR_CLIENT_NULL;
		LM36923_ERR("TEST_ERROR_CLIENT_NULL\n");
		goto lm36923_test_out;
	}

	/* protect i2c access*/
	down(&(pchip->test_sem));

	result = lm36923_test_regs_store(pchip, reg_val);
	if (result & TEST_ERROR_I2C) {
		up(&(pchip->test_sem));
		goto lm36923_test_out;
	}

	switch(lm36923_led_num){
		case TI_LM36923_LED_TWO:
			led_num = TI_LM36923_LED_TWO;
			break;
		case TI_LM36923_LED_THREE:
		default:
			led_num = TI_LM36923_LED_THREE;
			break;
	}

	for (i = 0; i < led_num; i++) {
		result |= lm36923_test_led_open(pchip, i);
	}

	for (i = 0; i < led_num; i++) {
		result |= lm36923_test_led_short(pchip, i);
	}

	ret = lm36923_chip_init(pchip);
	if (ret < 0) {
		result |= TEST_ERROR_CHIP_INIT;
	}

	result |= lm36923_test_regs_restore(pchip, reg_val);

	up(&(pchip->test_sem));

lm36923_test_out:
	LM36923_INFO("self test out:%d\n", result);
	return snprintf(buf, PAGE_SIZE, "%d\n", result);
}

static DEVICE_ATTR(self_test, S_IRUGO|S_IWUSR, lm36923_self_test_show, NULL);

static const struct regmap_config lm36923_regmap = {
	.reg_bits = 8,
	.val_bits = 8,
	.reg_stride = 1,
};

/* pointers to created device attributes */
static struct attribute *lm36923_attributes[] = {
	&dev_attr_bled_mode.attr,
	&dev_attr_reg_bl.attr,
	&dev_attr_reg.attr,
	&dev_attr_self_test.attr,
	NULL,
};

static const struct attribute_group lm36923_group = {
	.attrs = lm36923_attributes,
};

static int lm36923_probe(struct i2c_client *client,
				const struct i2c_device_id *id)
{
	struct i2c_adapter *adapter = NULL;
	struct lm36923_chip_data *pchip;
	int ret = -1;
	unsigned int val = 0;

	LM36923_INFO("in!\n");

	if(client == NULL){
		LM36923_ERR("client is NULL pointer\n");
		return -1;
	}

	adapter = client->adapter;
	if(adapter == NULL){
		LM36923_ERR("adapter is NULL pointer\n");
		return -1;
	}

	if (!i2c_check_functionality(adapter, I2C_FUNC_I2C)) {
		dev_err(&client->dev, "i2c functionality check fail.\n");
		return -EOPNOTSUPP;
	}

	pchip = devm_kzalloc(&client->dev,
				sizeof(struct lm36923_chip_data), GFP_KERNEL);
	if (!pchip)
		return -ENOMEM;

	pchip->regmap = devm_regmap_init_i2c(client, &lm36923_regmap);
	if (IS_ERR(pchip->regmap)) {
		ret = PTR_ERR(pchip->regmap);
		dev_err(&client->dev, "fail : allocate register map: %d\n", ret);
		goto err_out;
	}
	pchip->client = client;
	i2c_set_clientdata(client, pchip);

	sema_init(&(pchip->test_sem), 1);

	/* chip initialize */
	ret = lm36923_chip_init(pchip);
	if (ret < 0) {
		dev_err(&client->dev, "fail : chip init\n");
		goto err_out;
	}

	ret = regmap_read(pchip->regmap, REG_FAULT_FLAG, &val);
	if (ret < 0) {
		dev_err(&client->dev, "fail : read chip reg REG_FAULT_FLAG error!\n");
		goto err_out;
	}

	if (0 != val) {
		ret = dsm_client_ocuppy(lcd_dclient);
		if (!ret) {
			dev_err(&client->dev, "fail : REG_FAULT_FLAG statues error 0X1F=%d!\n", val);
			dsm_client_record(lcd_dclient, "REG_FAULT_FLAG statues error 0X1F=%d!\n", val);
			dsm_client_notify(lcd_dclient, DSM_LCD_OVP_ERROR_NO);
        } else {
			dev_err(&client->dev, "dsm_client_ocuppy fail:  ret=%d!\n", ret);
		}
	}

	pchip->dev = device_create(lm36923_class, NULL, 0, "%s", client->name);
	if (IS_ERR(pchip->dev)) {
		/* Not fatal */
		LM36923_ERR("Unable to create device; errno = %ld\n", PTR_ERR(pchip->dev));
		pchip->dev = NULL;
	} else {
		dev_set_drvdata(pchip->dev, pchip);
		ret = sysfs_create_group(&pchip->dev->kobj, &lm36923_group);
		if (ret)
			goto err_sysfs;
	}

	g_pchip = pchip;

	LM36923_INFO("name: %s, address: (0x%x) ok!\n", client->name, client->addr);
	lm36923_init_status = true;

	return ret;

err_sysfs:
	device_destroy(lm36923_class, 0);
err_out:
	devm_kfree(&client->dev, pchip);
	return ret;
}

static int lm36923_remove(struct i2c_client *client)
{
	struct lm36923_chip_data *pchip = NULL;

	if(client == NULL){
		LM36923_ERR("client is NULL pointer\n");
		return -1;
	}

	pchip = i2c_get_clientdata(client);
	if(pchip == NULL){
		LM36923_ERR("pchip is NULL pointer\n");
		return -1;
	}

	regmap_write(pchip->regmap, REG_ENABLE, 0x00);

	sysfs_remove_group(&client->dev.kobj, &lm36923_group);

	return 0;
}

static const struct i2c_device_id lm36923_id[] = {
	{LM36923_NAME, 0},
	{}
};

static const struct of_device_id lm36923_of_id_table[] = {
	{.compatible = "ti,lm36923"},
	{ },
};

MODULE_DEVICE_TABLE(i2c, lm36923_id);
static struct i2c_driver lm36923_i2c_driver = {
		.driver = {
			.name = "lm36923",
			.owner = THIS_MODULE,
			.of_match_table = lm36923_of_id_table,
		},
		.probe = lm36923_probe,
		.remove = lm36923_remove,
		.id_table = lm36923_id,
};

static int __init lm36923_module_init(void)
{
	int ret = -1;

	LM36923_INFO("in!\n");

	lm36923_class = class_create(THIS_MODULE, "lm36923");
	if (IS_ERR(lm36923_class)) {
		LM36923_ERR("Unable to create lm36923 class; errno = %ld\n", PTR_ERR(lm36923_class));
		lm36923_class = NULL;
	}

	ret = i2c_add_driver(&lm36923_i2c_driver);
	if (ret)
		LM36923_ERR("Unable to register lm36923 driver\n");

	LM36923_INFO("ok!\n");

	return ret;
}
late_initcall(lm36923_module_init);

MODULE_DESCRIPTION("Texas Instruments Backlight driver for LM36923");
MODULE_AUTHOR("Daniel Jeong <daniel.jeong@ti.com>");
MODULE_AUTHOR("G.Shark Jeong <gshark.jeong@gmail.com>");
MODULE_LICENSE("GPL v2");
