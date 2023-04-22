/*
 * tfa98xx.h -- TFA98XX ALSA SoC Audio driver
 *
 * Copyright 2011-2012 NXP Integrated Products
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef _TFA98XX_H
#define _TFA98XX_H

#include <linux/list.h>

typedef struct list_head tfa98xx_list_t;

#define CHECK_IOCTL_OPS(ops, func) \
do{\
    if(NULL == ops || NULL == ops->func)\
        return -1;\
}while(0)

#define FAILED -1

/*IO controls for user*/
#define TFA98XX_POWER_ON					_IO('M', 0x01)
#define TFA98XX_POWER_OFF					_IO('M', 0x02)
#define TFA98XX_POWER_SPK_ON				_IO('M', 0x03)
#define TFA98XX_POWER_SPK_OFF				_IO('M', 0x04)
#define TFA98XX_POWER_REC_ON				_IO('M', 0x05)
#define TFA98XX_POWER_REC_OFF				_IO('M', 0x06)

#define TFA98XX_GET_VERSION				_IOR('M', 0xFF, __u32)
#define TFA98XX_R_GET_VERSION				_IOR('M', 0xFE, __u32)

/*Get M98925 related settings, nr from 0x10 to 0x3f*/
#define TFA98XX_GET_VOLUME				_IOR('M', 0x10, __u32)
#define TFA98XX_GET_DAIFORMAT				_IOR('M', 0x11, __u32)
#define TFA98XX_GET_DAICLOCK				_IOR('M', 0x12, __u32)
#define TFA98XX_GET_BOOSTVOLT				_IOR('M', 0x13, __u32)
#define TFA98XX_GET_ALCTHRESHOLD			_IOR('M', 0x14, __u32)
#define TFA98XX_GET_FILTERS				_IOR('M', 0x15, __u32)
#define TFA98XX_GET_GAINRAMP				_IOR('M', 0x16, __u32)
#define TFA98XX_GET_REG_VAL				_IOR('M', 0x17, struct tfa98xx_reg_ops)

/*Get M98925_r related settings, nr from 0x10 to 0x3f*/
#define TFA98XX_R_GET_VOLUME				_IOR('M', 0x20, __u32)
#define TFA98XX_R_GET_DAIFORMAT			_IOR('M', 0x21, __u32)
#define TFA98XX_R_GET_DAICLOCK			_IOR('M', 0x22, __u32)
#define TFA98XX_R_GET_BOOSTVOLT			_IOR('M', 0x23, __u32)
#define TFA98XX_R_GET_ALCTHRESHOLD		_IOR('M', 0x24, __u32)
#define TFA98XX_R_GET_FILTERS				_IOR('M', 0x25, __u32)
#define TFA98XX_R_GET_GAINRAMP			_IOR('M', 0x26, __u32)
#define TFA98XX_R_GET_REG_VAL				_IOR('M', 0x27, struct tfa98xx_reg_ops)

/*Change M98925 related settings, nr from 0x40 to 0x7f*/
#define TFA98XX_SET_VOLUME				_IOW('M', 0x40, __u32)
#define TFA98XX_SET_DAIFORMAT				_IOW('M', 0x41, __u32)
#define TFA98XX_SET_DAICLOCK				_IOW('M', 0x42, __u32)
#define TFA98XX_SET_BOOSTVOLT				_IOW('M', 0x43, __u32)
#define TFA98XX_SET_ALCTHRESHOLD			_IOW('M', 0x44, __u32)
#define TFA98XX_SET_FILTERS				_IOW('M', 0x45, __u32)
#define TFA98XX_SET_GAINRAMP				_IOW('M', 0x46, __u32)
#define TFA98XX_SET_REG_VAL				_IOW('M', 0x47, struct tfa98xx_reg_ops)
#define TFA98XX_SET_PARAM				_IOW('M', 0x48, struct tfa98xx_param)

#define TFA98XX_PARAM_MAX_NUM			(100)
struct tfa98xx_priv {
	unsigned int gain;
	unsigned int gain_incall;
	unsigned int sysclk;
	unsigned int type; //0:left, 1:right
	unsigned int pa_elec_limit;//0:unlimited, 1:limit
	int gpio_irq;
	int gpio_reset;
	int rcv_en1_gpio;
	int rcv_en2_gpio;
	u8 reg;
	bool rcv_switch_support;
    bool iv_slot_change;
    bool dcie_cfg;
    struct regmap *regmap;
    struct mutex  lock;
    struct list_head list;
};

struct tfa98xx_reg_ops{
	unsigned int reg_addr;
	unsigned int reg_val;
};

struct tfa98xx_gain_def {
	unsigned int gain;
	unsigned int gain_incall;
};

struct tfa98xx_param{
	unsigned int l_num;
	unsigned int r_num;
	unsigned char aucData[TFA98XX_PARAM_MAX_NUM];
};

struct tfa98xx_ioctl_ops{
	int  (*tfa98xx_set_slave)(struct list_head *tfa98xx);
	void (*tfa98xx_set_master)(struct list_head *tfa98xx);
	int  (*tfa98xx_set_clock)(struct list_head *tfa98xx, unsigned int rate);
	int  (*tfa98xx_open)(struct inode *inode, struct file *filp);
	int  (*tfa98xx_release)(struct inode *inode, struct file *filp);
	int  (*tfa98xx_get_version)(struct list_head *tfa98xx, unsigned int type, unsigned int *value);
	int  (*tfa98xx_get_reg_val)(struct list_head *tfa98xx, unsigned int type, struct tfa98xx_reg_ops *reg_val, unsigned int __user *pUser);
	int  (*tfa98xx_set_reg_val)(struct list_head *tfa98xx, struct tfa98xx_reg_ops *reg_val, unsigned int __user *pUser);
	int  (*tfa98xx_digital_mute)(struct list_head *tfa98xx, int mute);
	int  (*tfa98xx_single_digital_mute)(struct list_head *tfa98xx, unsigned int type, int mute);
	int  (*tfa98xx_get_volume)(struct list_head *tfa98xx, unsigned int type, unsigned int *value);
	int  (*tfa98xx_set_volume)(struct list_head *tfa98xx, unsigned int value);
	int  (*tfa98xx_get_daiclock)(struct list_head *tfa98xx, unsigned int type, unsigned int *value);
	int  (*tfa98xx_set_daiclock)(struct list_head *tfa98xx, unsigned int value);
	int  (*tfa98xx_get_boostvolt)(struct list_head *tfa98xx, unsigned int type, unsigned int *value);
	int  (*tfa98xx_set_boostvolt)(struct list_head *tfa98xx, unsigned int value);
	int  (*tfa98xx_get_alcthreshold)(struct list_head *tfa98xx, unsigned int type, unsigned int *value);
	int  (*tfa98xx_set_alcthreshold)(struct list_head *tfa98xx, unsigned int value);
	int  (*tfa98xx_get_filters)(struct list_head *tfa98xx, unsigned int type, unsigned int *value);
	int  (*tfa98xx_set_filters)(struct list_head *tfa98xx, unsigned int value);
	int  (*tfa98xx_get_gainramp)(struct list_head *tfa98xx, unsigned int type, unsigned int *value);
	int  (*tfa98xx_set_gainramp)(struct list_head *tfa98xx, unsigned int value);
	int  (*tfa98xx_set_param)(struct list_head *tfa98xx, unsigned int __user *pUser);
};

enum tfa98xx_type{
	TFA98XX_L = 0,
	TFA98XX_R,
};

enum mute_val{
	MUTE_ON = 0,
	MUTE_OFF,
};

extern int tfa98xx_ioctl_regist(struct tfa98xx_ioctl_ops *ops);
extern int tfa98xx_ioctl_isregist(void);
extern void tfa98xx_list_add(struct tfa98xx_priv *tfa98xx);
extern void tfa98xx_list_del_all(void);
extern struct tfa98xx_priv *find_tfa98xx_by_dev(struct device *dev);
extern int get_tfa98xx_num(void);
extern struct list_head *get_tfa98xx_list_head(void);
extern struct tfa98xx_priv *find_tfa98xx_by_type(struct list_head *tfa98xx, unsigned int type);

#endif
