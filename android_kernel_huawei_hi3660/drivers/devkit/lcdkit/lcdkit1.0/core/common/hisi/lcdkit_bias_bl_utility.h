#ifndef _LCDKIT_BIAS_BL_IC_UTILITY_H_
#define _LCDKIT_BIAS_BL_IC_UTILITY_H_

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
#include "lcdkit_disp.h"
#include "lcdkit_panel.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
extern struct dsm_client *lcd_dclient;
#endif


#define LCD_BIAS_IC_NAME_LEN 20 
#define LCD_BACKLIGHT_IC_NAME_LEN 20
int lcdkit_get_backlight_ic_name(char* buf, int len);
int lcdkit_get_bias_ic_name(char* buf, int len);
uint32_t lcdkit_check_lcd_plugin(void);
struct device_node* lcdkit_get_lcd_node(void);
void lcdkit_set_lcd_node(struct device_node* pnode);
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
void set_lcd_bias_dev_flag(void);
#endif
#endif
