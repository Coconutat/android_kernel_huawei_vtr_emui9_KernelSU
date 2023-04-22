#ifndef _LCDKIT_BIAS_BL_UTILITY_H_
#define _LCDKIT_BIAS_BL_UTILITY_H_

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/init.h>
#include <linux/leds.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>
#include <linux/kernel.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <linux/hw_dev_dec.h>
#endif

#define LCD_BIAS_IC_NAME_LEN 20 
#define LCD_BACKLIGHT_IC_NAME_LEN 20

int lcdkit_get_backlight_ic_name(char* buf, int len);
int lcdkit_get_bias_ic_name(char* buf, int len);
struct device_node* lcdkit_get_lcd_node(void);
void lcdkit_set_lcd_node(struct device_node* pnode);
bool lcdkit_check_lcd_plugin(void);
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
void set_lcd_bias_dev_flag(void);
#endif

#endif