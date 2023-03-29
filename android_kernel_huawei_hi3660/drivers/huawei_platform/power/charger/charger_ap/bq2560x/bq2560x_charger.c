/*
 * drivers/power/bq2560x_charger.c
 *
 * BQ2560x/1/2/3/4 charging driver
 *
 * Copyright (C) 2012-2015 HUAWEI, Inc.
 * Author: HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/wakelock.h>
#include <linux/usb/otg.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/power_supply.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <linux/raid/pq.h>

#include <linux/hisi/usb/hisi_usb.h>
#include <linux/hisi/hisi_adc.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <huawei_platform/power/huawei_charger.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <bq2560x_charger.h>
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
#include <huawei_platform/power/usb_short_circuit_protect.h>
#endif

#define HWLOG_TAG bq2560x_charger
HWLOG_REGIST();

struct bq2560x_device_info *g_bq2560x_dev;
static unsigned int rilim = BQ2560X_RILIM_220_OHM; /* this should be configured in dts file based on the real value of the Iin limit resistance */
static unsigned int adc_channel_iin = BQ2560X_ADC_CHANNEL_IIN_10; /* this should be configured in dts file based on the real adc channel number */

static bool g_hiz_mode = FALSE;
static int hiz_iin_limit_flag = HIZ_IIN_FLAG_FALSE;
static int g_cv_flag = 0;
static int g_cv_policy = 0;

#define TERM_CURR                    (840)
#define LIMIT_CURR                   (840)

#define MSG_LEN                      (2)
#define BUF_LEN                      (26)

/**********************************************************
*  Function:       params_to_reg
*  Discription:    turn the setting parameter to register value
*  Parameters:   const int tbl[], int tbl_size, int val
*  return value:  register value
**********************************************************/
static int params_to_reg(const int tbl[], int tbl_size, int val)
{
	int i;

	for (i = 1; i < tbl_size; i++) {
		if (val < tbl[i]) {
			return (i - 1);
		}
	}

	return (tbl_size - 1);
}

/**********************************************************
*  Function:       bq2560x_write_block
*  Discription:    register write block interface
*  Parameters:   di:bq2560x_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_write_block(struct bq2560x_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

	if (NULL == di || NULL == value) {
		hwlog_err("error: di is null or value is null!\n");
		return -EIO;
	}

	*value = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = value;
	msg[0].len = num_bytes + 1;

	ret = i2c_transfer(di->client->adapter, msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 1) {
		hwlog_err("error: i2c_write failed to transfer all messages!\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	else {
		return 0;
	}
}

/**********************************************************
*  Function:       bq2560x_read_block
*  Discription:    register read block interface
*  Parameters:   di:bq2560x_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_read_block(struct bq2560x_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[MSG_LEN];
	u8 buf = 0;
	int ret = 0;

	if (NULL == di || NULL == value) {
		hwlog_err("error: di is null or value is null!\n");
		return -EIO;
	}

	buf = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = &buf;
	msg[0].len = 1;

	msg[1].addr = di->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = value;
	msg[1].len = num_bytes;

	ret = i2c_transfer(di->client->adapter, msg, MSG_LEN);

	/* i2c_transfer returns number of messages transferred */
	if (ret != MSG_LEN) {
		hwlog_err("error: i2c_read failed to transfer all messages!\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	else {
		return 0;
	}
}

/**********************************************************
*  Function:       bq2560x_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_write_byte(u8 reg, u8 value)
{
	struct bq2560x_device_info *di = g_bq2560x_dev;
	u8 temp_buffer[MSG_LEN] = { 0 }; /* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return bq2560x_write_block(di, temp_buffer, reg, 1);
}

/**********************************************************
*  Function:       bq2560x_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_read_byte(u8 reg, u8 *value)
{
	struct bq2560x_device_info *di = g_bq2560x_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	return bq2560x_read_block(di, value, reg, 1);
}

/**********************************************************
*  Function:       bq2560x_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = bq2560x_read_byte(reg, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = bq2560x_write_byte(reg, val);

	return ret;
}

/**********************************************************
*  Function:       bq2560x_read_mask
*  Discription:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_read_mask(u8 reg, u8 MASK, u8 SHIFT, u8 *value)
{
	int ret = 0;
	u8 val = 0;

	ret = bq2560x_read_byte(reg, &val);
	if (ret < 0) {
		return ret;
	}

	val &= MASK;
	val >>= SHIFT;
	*value = val;

	return 0;
}

#ifdef CONFIG_SYSFS
/*
 * There are a numerous options that are configurable on the bq2560x
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */

#define BQ2560x_SYSFS_FIELD(_name, r, f, m, store)                  \
{                                                   \
	.attr = __ATTR(_name, m, bq2560x_sysfs_show, store),    \
	.reg = BQ2560X_REG_##r,                      \
	.mask = BQ2560X_REG_##f##_MASK,                       \
	.shift = BQ2560X_REG_##f##_SHIFT,                     \
}

#define BQ2560x_SYSFS_FIELD_RW(_name, r, f)                     \
	BQ2560x_SYSFS_FIELD(_name, r, f, S_IWUSR | S_IRUGO, bq2560x_sysfs_store)

#define BQ2560x_SYSFS_FIELD_RO(_name, r, f)                         \
	BQ2560x_SYSFS_FIELD(_name, r, f, S_IRUGO, NULL)

static ssize_t bq2560x_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t bq2560x_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

struct bq2560x_sysfs_field_info {
	struct device_attribute attr;
	u8 reg;
	u8 mask;
	u8 shift;
};

/* On i386 ptrace-abi.h defines SS that breaks the macro calls below. */
//#undef SS

static struct bq2560x_sysfs_field_info bq2560x_sysfs_field_tbl[] = {
	/* sysfs name reg field in reg */
	BQ2560x_SYSFS_FIELD_RW(reg_addr, NONE, NONE),
	BQ2560x_SYSFS_FIELD_RW(reg_value, NONE, NONE),
};

static struct attribute *bq2560x_sysfs_attrs[ARRAY_SIZE(bq2560x_sysfs_field_tbl) + 1];

static const struct attribute_group bq2560x_sysfs_attr_group = {
	.attrs = bq2560x_sysfs_attrs,
};

/**********************************************************
*  Function:       bq2560x_sysfs_init_attrs
*  Discription:    initialize bq2560x_sysfs_attrs[] for bq2560x attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void bq2560x_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(bq2560x_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		bq2560x_sysfs_attrs[i] = &bq2560x_sysfs_field_tbl[i].attr.attr;
	}

	bq2560x_sysfs_attrs[limit] = NULL; /* Has additional entry for this */
}

/**********************************************************
*  Function:       bq2560x_sysfs_field_lookup
*  Discription:    get the current device_attribute from bq2560x_sysfs_field_tbl by attr's name
*  Parameters:   name:evice attribute name
*  return value:  bq2560x_sysfs_field_tbl[]
**********************************************************/
static struct bq2560x_sysfs_field_info *bq2560x_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(bq2560x_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strcmp(name, bq2560x_sysfs_field_tbl[i].attr.attr.name)) {
			break;
		}
	}

	if (i >= limit) {
		return NULL;
	}

	return &bq2560x_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       bq2560x_sysfs_show
*  Discription:    show the value for all bq2560x device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t bq2560x_sysfs_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{

	struct bq2560x_sysfs_field_info *info;
	struct bq2560x_sysfs_field_info *info2;
	int ret;
	u8 v;

	info = bq2560x_sysfs_field_lookup(attr->attr.name);
	if (!info) {
		hwlog_err("error: get sysfs entries failed!\n");
		return -EINVAL;
	}

	if (!strncmp("reg_addr", attr->attr.name, strlen("reg_addr"))) {
		return scnprintf(buf, PAGE_SIZE, "0x%hhx\n", info->reg);
	}

	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = bq2560x_sysfs_field_lookup("reg_addr");
		if (!info2) {
			hwlog_err("error: get sysfs entries failed!\n");
			return -EINVAL;
		}

		info->reg = info2->reg;
	}

	ret = bq2560x_read_mask(info->reg, info->mask, info->shift, &v);
	if (ret) {
		return ret;
	}

	return scnprintf(buf, PAGE_SIZE, "%hhx\n", v);
}

/**********************************************************
*  Function:       bq2560x_sysfs_store
*  Discription:    set the value for all bq2560x device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t bq2560x_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct bq2560x_sysfs_field_info *info;
	struct bq2560x_sysfs_field_info *info2;
	int ret;
	u8 v;

	info = bq2560x_sysfs_field_lookup(attr->attr.name);
	if (!info) {
		hwlog_err("error: get sysfs entries failed!\n");
		return -EINVAL;
	}

	ret = kstrtou8(buf, 0, &v);
	if (ret < 0) {
		hwlog_err("error: get kstrtou8 failed!\n");
		return ret;
	}

	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = bq2560x_sysfs_field_lookup("reg_addr");
		if (!info2) {
			hwlog_err("error: get sysfs entries failed!\n");
			return -EINVAL;
		}

		info->reg = info2->reg;
	}

	if (!strncmp(("reg_addr"), attr->attr.name, strlen("reg_addr"))) {
		if (v < (u8) BQ2560X_REG_NUM) {
			info->reg = v;
			return count;
		}
		else {
			return -EINVAL;
		}
	}

	ret = bq2560x_write_mask(info->reg, info->mask, info->shift, v);
	if (ret) {
		return ret;
	}

	return count;
}

/**********************************************************
*  Function:       bq2560x_sysfs_create_group
*  Discription:    create the bq2560x device sysfs group
*  Parameters:   di:bq2560x_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_sysfs_create_group(struct bq2560x_device_info *di)
{
	bq2560x_sysfs_init_attrs();

	return sysfs_create_group(&di->dev->kobj, &bq2560x_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Discription:    remove the bq2560x device sysfs group
*  Parameters:   di:bq2560x_device_info
*  return value:  NULL
**********************************************************/
static void bq2560x_sysfs_remove_group(struct bq2560x_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &bq2560x_sysfs_attr_group);
}

#else

static int bq2560x_sysfs_create_group(struct bq2560x_device_info *di)
{
	return 0;
}

static inline void bq2560x_sysfs_remove_group(struct bq2560x_device_info *di)
{
}

#endif

static int bq2560x_device_check(void)
{
	u8 reg = 0;
	int ret = 0;

	ret = bq2560x_read_byte(BQ2560X_REG_VPRS, &reg);
	if (ret) {
		hwlog_err("error: device_check read fail!\n");
		return CHARGE_IC_BAD;
	}

	hwlog_info("device_check [%x]=0x%x\n", BQ2560X_REG_VPRS, reg);

	if (((reg & BQ2560X_REG_VPRS_PN_MASK) >> BQ2560X_REG_VPRS_PN_SHIFT) == VENDOR_ID) {
		return CHARGE_IC_GOOD;
	}

	return CHARGE_IC_BAD;
}

/**********************************************************
*  Function:       bq2560x_5v_chip_init
*  Discription:    bq2560x chipIC initialization
*  Parameters:   struct bq2560x_device_info *di
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_5v_chip_init(struct bq2560x_device_info *di)
{
	int ret = 0;
	g_hiz_mode = FALSE;

	/* boost mode current limit = 1000mA */
	ret = bq2560x_write_byte(BQ2560X_REG_CCC, 0xa1);

	/* I2C watchdog timer setting = 80s */
	/* fast charge timer setting = 12h */
	ret |= bq2560x_write_byte(BQ2560X_REG_CTTC, 0xAF);

	/* iprechg = 256ma,iterm current = 128ma */
	ret |= bq2560x_write_byte(BQ2560X_REG_PCTCC, 0x42);
	ret |= bq2560x_write_mask(BQ2560X_REG_MOC,
				BQ2560X_REG_MOC_VDPM_BAT_TRACK_MASK,
				BQ2560X_REG_MOC_VDPM_BAT_TRACK_SHIFT, REG07_VDPM_BAT_TRACK_DISABLE);

	hiz_iin_limit_flag = HIZ_IIN_FLAG_FALSE;

	/* enable charging */
	gpio_set_value(di->gpio_cd, 0);

	return ret;
}
static int bq2560x_chip_init(struct chip_init_crit* init_crit)
{
	int ret = -1;
	struct bq2560x_device_info *di = g_bq2560x_dev;

	if (NULL == di || NULL == init_crit) {
		hwlog_err("error: di or init_crit is null!\n");
		return -ENOMEM;
	}

	switch (init_crit->vbus) {
		case ADAPTER_5V:
			ret = bq2560x_5v_chip_init(di);
		break;

		default:
			hwlog_err("error: invaid init_crit vbus mode!\n");
		break;
	}

	return ret;
}

/**********************************************************
*  Function:       bq2560x_set_input_current
*  Discription:    set the input current in charging process
*  Parameters:   value:input current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_input_current(int value)
{
	int val = 0;

	if (value >= AC_IIN_MAX_CURRENT){
		value = EX_AC_IIN_MAX_CURRENT;
	}

	val = (value - REG00_IINLIM_BASE) / REG00_IINLIM_LSB;

	hwlog_info("set_input_current [%x]=0x%x\n", BQ2560X_REG_ISC, val);

	return bq2560x_write_mask(BQ2560X_REG_ISC,
				BQ2560X_REG_ISC_IINLIM_MASK,
				BQ2560X_REG_ISC_IINLIM_SHIFT, val);
}

/**********************************************************
*  Function:       bq2560x_set_charge_current
*  Discription:    set the charge current in charging process
*  Parameters:   value:charge current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_charge_current(int value)
{
	int val = 0;

	val = (value - REG02_ICHG_BASE) / REG02_ICHG_LSB;

	hwlog_info("set_charge_current [%x]=0x%x\n", BQ2560X_REG_CCC, val);

	return bq2560x_write_mask(BQ2560X_REG_CCC,
				BQ2560X_REG_CCC_ICHG_MASK,
				BQ2560X_REG_CCC_ICHG_SHIFT, val);
}


/**********************************************************
*  Function:       bq2560x_set_boost_current
*  Discription:    set the OTG current in charging process
*  Parameters:   value:otg boost current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_boost_current(int curr)
{
	int val = 0;

	if (curr == BOOST_LIM_0P5A) {
		val = REG02_BOOST_LIM_0P5A;
	}
	else {
		val = REG02_BOOST_LIM_1P2A;
	}

	hwlog_info("set_boost_current [%x]=0x%x\n", BQ2560X_REG_REG_BVTRC, val);

	return bq2560x_write_mask(BQ2560X_REG_REG_BVTRC,
				BQ2560X_REG_REG_BVTRC_BOOSTV_MASK,
				BQ2560X_REG_REG_BVTRC_BOOSTV_SHIFT, val);
}

static int bq2560x_reused_set_cv(int value)
{
	int val = 0;

	val = (value - REG04_VREG_BASE) / REG04_VREG_LSB;

	hwlog_info("reused_set_cv [%x]=0x%x\n", BQ2560X_REG_CVC, val);

	return bq2560x_write_mask(BQ2560X_REG_CVC,
				BQ2560X_REG_CVC_VREG_MASK,
				BQ2560X_REG_CVC_VREG_SHIFT, val);
}

/**********************************************************
*  Function:       bq2560x_set_terminal_voltage
*  Discription:    set the terminal voltage in charging process
*  Parameters:   value:terminal voltage value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_terminal_voltage(int value)
{
	g_cv_policy = value;

	if (g_cv_flag == 0) {
		return bq2560x_reused_set_cv(value);
	}
	else {
		return 0;
	}
}

/**********************************************************
*  Function:       bq2560x_set_dpm_voltage
*  Discription:    set the dpm voltage in charging process
*  Parameters:   value:dpm voltage value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_dpm_voltage(int value)
{
	int val = 0;

	val = (value - REG06_VINDPM_BASE) / REG06_VINDPM_LSB;

	hwlog_info("set_dpm_voltage [%x]=0x%x\n", BQ2560X_REG_REG_BVTRC, val);

	return bq2560x_write_mask(BQ2560X_REG_REG_BVTRC,
				BQ2560X_REG_REG_BVTRC_VINDPM_MASK,
				BQ2560X_REG_REG_BVTRC_VINDPM_SHIFT, val);
}

static int bq2560x_reused_set_iterm(int value)
{
	int val = 0;

	val = (value - REG03_ITERM_BASE) / REG03_ITERM_LSB;

	hwlog_info("reused_set_iterm [%x]=0x%x\n", BQ2560X_REG_PCTCC, val);

	return bq2560x_write_mask(BQ2560X_REG_PCTCC,
				BQ2560X_REG_PCTCC_ITERM_MASK,
				BQ2560X_REG_PCTCC_ITERM_SHIFT, val);
}

/**********************************************************
*  Function:       bq2560x_set_terminal_current
*  Discription:    set the terminal current in charging process
*  Parameters:   value:terminal current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_terminal_current(int value)
{
	int ret = 0;
	int ichg = -hisi_battery_current();

	if (ichg > LIMIT_CURR) {
		ret = bq2560x_reused_set_cv(g_cv_policy + REG04_VREG_LSB);
		if (ret) {
			hwlog_err("error: set the high v_term fail, reset to g_cv_policy!\n");

			ret = bq2560x_reused_set_cv(g_cv_policy);
			if (ret) {
				hwlog_err("error: set g_cv_policy error!");
			}

			return bq2560x_reused_set_iterm(value);
		}
		else {
			g_cv_flag = 1;
			return bq2560x_reused_set_iterm(TERM_CURR);
		}
	}
	else {
		ret = bq2560x_reused_set_iterm(value);
		if (ret) {
			hwlog_err("error: set the iterm fail!\n");
			return ret;
		}
		else {
			g_cv_flag = 0;
			return bq2560x_reused_set_cv(g_cv_policy);
		}
	}
}

/**********************************************************
*  Function:       bq2560x_set_charge_enable
*  Discription:    set the charge enable in charging process
*  Parameters:   enable:charge enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_charge_enable(int enable)
{
	return bq2560x_write_mask(BQ2560X_REG_POC,
				BQ2560X_REG_POC_CHG_CONFIG_MASK,
				BQ2560X_REG_POC_CHG_CONFIG_SHIFT, enable);
}

/**********************************************************
*  Function:       bq2560x_set_otg_vboost
*  Discription:    set the otg vboost voltage range
*  Parameters:   voltage
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_boost_voltage(int voltage)
{
	int val = 0;
	struct bq2560x_device_info *di = g_bq2560x_dev;

	if (voltage== BOOSTV_4850) {
		val = REG06_BOOSTV_4P85V;
	}
	else if (voltage == BOOSTV_5150) {
		val = REG06_BOOSTV_5P15V;
	}
	else if (voltage == BOOSTV_5300) {
		val = REG06_BOOSTV_5P3V;
	}
	else {
		val = REG06_BOOSTV_5V;
	}

	hwlog_info("set_boost_voltage [%x]=0x%x\n", BQ2560X_REG_REG_BVTRC, val);

	return bq2560x_write_mask(BQ2560X_REG_REG_BVTRC,
				BQ2560X_REG_REG_BVTRC_BOOSTV_MASK,
				BQ2560X_REG_REG_BVTRC_BOOSTV_SHIFT, val);
}

/**********************************************************
*  Function:       bq2560x_set_otg_enable
*  Discription:    set the otg mode enable in charging process
*  Parameters:   enable:otg mode  enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_otg_enable(int enable)
{
	int val = 0;
	struct bq2560x_device_info *di = g_bq2560x_dev;

	/*NOTICE:
	   why enable irq when entry to OTG mode only?
	   because we care VBUS overloaded OCP or OVP's interrupt in boost mode
	*/
	if ((!di->irq_active) && (enable)) {
		di->irq_active = 1; /* ACTIVE */
		enable_irq(di->irq_int);
	}
	else if ((di->irq_active) && (!enable)) {
		di->irq_active = 0; /* INACTIVE */
		disable_irq(di->irq_int);
	}
	else {
		/* do nothing */
	}

	return bq2560x_write_mask(BQ2560X_REG_POC,
				BQ2560X_REG_POC_OTG_CONFIG_MASK,
				BQ2560X_REG_POC_OTG_CONFIG_SHIFT, enable);
}

/**********************************************************
*  Function:       bq2560x_set_term_enable
*  Discription:    set the terminal enable in charging process
*  Parameters:   enable:terminal enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_term_enable(int enable)
{
	return bq2560x_write_mask(BQ2560X_REG_CTTC,
				BQ2560X_REG_CTTC_EN_TERM_MASK,
				BQ2560X_REG_CTTC_EN_TERM_SHIFT, enable);
}

/**********************************************************
*  Function:       bq2560x_get_charge_state
*  Discription:    get the charge states in charging process
*  Parameters:   state:charge states
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_get_charge_state(unsigned int *state)
{
	u8 reg = 0;
	int ret = 0;

	ret = bq2560x_read_byte(BQ2560X_REG_SS, &reg);

	hwlog_info("get_charge_state [%x]=0x%x\n", BQ2560X_REG_SS, reg);

	if (!(reg & BQ2560X_REG_SS_PG_STAT_MASK)) {
		*state |= CHAGRE_STATE_NOT_PG;
	}

	if ((reg & BQ2560X_REG_SS_CHRG_STAT_MASK) == BQ2560X_REG_SS_CHRG_STAT_MASK) {
		*state |= CHAGRE_STATE_CHRG_DONE;
	}

	ret |= bq2560x_read_byte(BQ2560X_REG_F, &reg);

	hwlog_info("get_charge_state [%x]=0x%x\n", BQ2560X_REG_F, reg);

	if ((reg & BQ2560X_REG_F_FAULT_WDT_MASK) == BQ2560X_REG_F_FAULT_WDT_MASK) {
		*state |= CHAGRE_STATE_WDT_FAULT;
	}

	if ((reg & BQ2560X_REG_F_FAULT_BOOST_MASK) == BQ2560X_REG_F_FAULT_BOOST_MASK) {
		*state |= CHAGRE_STATE_VBUS_OVP;
	}

	if ((reg & BQ2560X_REG_F_FAULT_BAT_MASK) == BQ2560X_REG_F_FAULT_BAT_MASK) {
		*state |= CHAGRE_STATE_BATT_OVP;
	}

	return ret;
}

/**********************************************************
*  Function:       bq2560x_reset_watchdog_timer
*  Discription:    reset watchdog timer in charging process
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_reset_watchdog_timer(void)
{
	return bq2560x_write_mask(BQ2560X_REG_POC,
				BQ2560X_REG_POC_WDT_RESET_MASK,
				BQ2560X_REG_POC_WDT_RESET_SHIFT, 0x01);
}


/**********************************************************
*  Function:       bq2560x_get_ilim
*  Discription:    get average value for ilim
*  Parameters:     NULL
*  return value:   average value for ilim
**********************************************************/
static int bq2560x_get_ilim(void)
{
	return 0;
}

/**********************************************************
*  Function:       bq2560x_check_charger_plugged
*  Discription:    check whether USB or adaptor is plugged
*  Parameters:     NULL
*  return value:   1 means USB or adaptor plugged
*                  0 means USB or adaptor not plugged
**********************************************************/
static int bq2560x_check_charger_plugged(void)
{
	u8 reg = 0;
	int ret = 0;

	ret = bq2560x_read_mask(BQ2560X_REG_SS,
				BQ2560X_REG_SS_PG_STAT_MASK,
				BQ2560X_REG_SS_PG_STAT_SHIFT, &reg);

	hwlog_info("check_charger_plugged [%x]=0x%x\n", BQ2560X_REG_SS, reg);

	if (reg == BQ2560x_REG_SS_VBUS_PLUGGED) {
		return REG08_POWER_GOOD;
	}

	return 0;
}

/**********************************************************
*  Function:       bq2560x_check_input_dpm_state
*  Discription:    check whether VINDPM or IINDPM
*  Parameters:     NULL
*  return value:   TRUE means VINDPM or IINDPM
*                  FALSE means NoT DPM
**********************************************************/
static int bq2560x_check_input_dpm_state(void)
{
	u8 reg = 0;
	int ret = -1;

	ret = bq2560x_read_byte(BQ2560X_REG_VINS, &reg);
	if (ret < 0) {
		hwlog_err("error: check_input_dpm_state read fail!\n");
		return ret;
	}

	hwlog_info("check_input_dpm_state [%x]=0x%x\n", BQ2560X_REG_VINS, reg);

	if (reg & BQ2560X_REG_VINS_VINDPM_STAT_MASK) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/**********************************************************
*  Function:       bq2560x_check_input_vdpm_state
*  Discription:    check whether VINDPM
*  Parameters:     NULL
*  return value:   TRUE means VINDPM
*                  FALSE means NoT VINDPM
**********************************************************/
static int bq2560x_check_input_vdpm_state(void)
{
	u8 reg = 0;
	int ret = -1;

	ret = bq2560x_read_byte(BQ2560X_REG_VINS, &reg);
	if (ret < 0) {
		hwlog_err("error: check_input_vdpm_state read fail!\n");
		return ret;
	}

	hwlog_info("check_input_vdpm_state [%x]=0x%x\n", BQ2560X_REG_VINS, reg);

	if (reg & BQ2560X_REG_VINS_VINDPM_STAT_MASK) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/**********************************************************
*  Function:       bq2560x_check_input_idpm_state
*  Discription:    check whether IINDPM
*  Parameters:     NULL
*  return value:   TRUE means  IINDPM
*                  FALSE means NoT IINDPM
**********************************************************/
static int bq2560x_check_input_idpm_state(void)
{
	u8 reg = 0;
	int ret = -1;

	ret = bq2560x_read_byte(BQ2560X_REG_VINS, &reg);
	if (ret < 0) {
		hwlog_err("error: check_input_idpm_state read fail!\n");
		return ret;
	}

	hwlog_info("check_input_idpm_state [%x]=0x%x\n", BQ2560X_REG_VINS, reg);

	if (reg & BQ2560X_REG_VINS_IINDPM_STAT_MASK) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/**********************************************************
*  Function:       bq2560x_dump_register
*  Discription:    print the register value in charging process
*  Parameters:   reg_value:string for save register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_dump_register(char *reg_value)
{
	u8 reg[BQ2560X_REG_NUM] = { 0 };
	char buff[BUF_LEN] = { 0 };
	int i = 0;

	memset(reg_value, 0, CHARGELOG_SIZE);

	for (i = 0; i < BQ2560X_REG_NUM; i++) {
		bq2560x_read_byte(i, &reg[i]);
		bq2560x_read_byte(i, &reg[i]);
		snprintf(buff, BUF_LEN, "0x%-8.2x", reg[i]);
		strncat(reg_value, buff, strlen(buff));
	}

	return 0;
}

/**********************************************************
*  Function:       bq2560x_dump_register
*  Discription:    print the register head in charging process
*  Parameters:   reg_head:string for save register head
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_get_register_head(char *reg_head)
{
	char buff[BUF_LEN] = { 0 };
	int i = 0;

	memset(reg_head, 0, CHARGELOG_SIZE);

	for (i = 0; i < BQ2560X_REG_NUM; i++) {
		snprintf(buff, BUF_LEN, "Reg[0x%x]  ", i);
		strncat(reg_head, buff, strlen(buff));
	}

	return 0;
}

/**********************************************************
*  Function:       bq2560x_set_batfet_disable
*  Discription:    set the batfet disable in charging process
*  Parameters:   disable:batfet disable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_batfet_disable(int disable)
{
	return bq2560x_write_mask(BQ2560X_REG_MOC,
				BQ2560X_REG_MOC_BATFET_DISABLE_MASK,
				BQ2560X_REG_MOC_BATFET_DISABLE_SHIFT, disable);
}

/**********************************************************
*  Function:       bq2560x_set_watchdog_timer
*  Discription:    set the watchdog timer in charging process
*  Parameters:   value:watchdog timer value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_watchdog_timer(int value)
{
	int ret = 0;
	int val = 0;

	if (value == WDT_BASE || TRUE == g_hiz_mode) {
		val = REG05_WDT_DISABLE;
	}
	else if (value <= WDT_40S) {
		val = REG05_WDT_40S;
	}
	else if (value <= WDT_80S) {
		val = REG05_WDT_80S;
	}
	else {
		val = REG05_WDT_160S;
	}

	ret = bq2560x_write_mask(BQ2560X_REG_CTTC,
				BQ2560X_REG_CTTC_WDT_MASK,
				BQ2560X_REG_CTTC_WDT_SHIFT, val);
	if (ret) {
		hwlog_err("error: set_watchdog_timer write fail!\n");
	}

	hwlog_info("set_watchdog_timer [%x]=0x%x\n", BQ2560X_REG_CTTC, val);

	if (value > 0) {
		ret = bq2560x_reset_watchdog_timer();
	}

	return ret;
}

/**********************************************************
*  Function:       bq2560x_set_charger_hiz
*  Discription:    set the charger hiz close watchdog
*  Parameters:   enable:charger in hiz or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_set_charger_hiz(int enable)
{
	int ret = 0;
	int ret2 = 0;
	static int first_in = 1;
	struct bq2560x_device_info *di = g_bq2560x_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return 0;
	}

	if (enable > 0) {
#ifdef CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
		if(1 == di->hiz_iin_limit && is_uscp_hiz_mode() && !is_in_rt_uscp_mode()){
			hiz_iin_limit_flag = HIZ_IIN_FLAG_TRUE;

			if (first_in) {
				hwlog_info("is_uscp_hiz_mode HIZ, enable:%d, set 100mA\n", enable);
				first_in = 0;
				return bq2560x_set_input_current(IINLIM_100); /* set inputcurrent to 100mA */
			}
			else {
				return 0;
			}
		}
		else {
#endif
			ret |= bq2560x_write_mask(BQ2560X_REG_ISC,
					BQ2560X_REG_ISC_EN_HIZ_MASK,
					BQ2560X_REG_ISC_EN_HIZ_SHIFT, TRUE);

			g_hiz_mode = TRUE;

			ret2 = bq2560x_set_watchdog_timer(WATCHDOG_TIMER_DISABLE);
			if(ret2){
				hwlog_err("error: set_charger_hiz write fail!\n");
			}
#ifdef CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
		}
#endif
	}
	else {
		hiz_iin_limit_flag = HIZ_IIN_FLAG_FALSE;
		first_in = 1;

		ret |= bq2560x_write_mask(BQ2560X_REG_ISC,
					BQ2560X_REG_ISC_EN_HIZ_MASK,
					BQ2560X_REG_ISC_EN_HIZ_SHIFT, FALSE);
		g_hiz_mode = FALSE;
	}

	return ret;
}

struct charge_device_ops bq2560x_ops = {
	.chip_init = bq2560x_chip_init,
	.dev_check = bq2560x_device_check,
	.set_input_current = bq2560x_set_input_current,
	.set_charge_current = bq2560x_set_charge_current,
	.set_terminal_voltage = bq2560x_set_terminal_voltage,
	.set_dpm_voltage = bq2560x_set_dpm_voltage,
	.set_terminal_current = bq2560x_set_terminal_current,
	.set_charge_enable = bq2560x_set_charge_enable,
	.set_otg_enable = bq2560x_set_otg_enable,
	.set_otg_current = bq2560x_set_boost_current,
	.set_term_enable = bq2560x_set_term_enable,
	.get_charge_state = bq2560x_get_charge_state,
	.reset_watchdog_timer = bq2560x_reset_watchdog_timer,
	.dump_register = bq2560x_dump_register,
	.get_register_head = bq2560x_get_register_head,
	.set_watchdog_timer = bq2560x_set_watchdog_timer,
	.set_batfet_disable = bq2560x_set_batfet_disable,
	.get_ibus = bq2560x_get_ilim,
	.check_charger_plugged = bq2560x_check_charger_plugged,
	.check_input_dpm_state = bq2560x_check_input_dpm_state,
	.check_input_vdpm_state = bq2560x_check_input_vdpm_state,
	.check_input_idpm_state = bq2560x_check_input_idpm_state,
	.set_charger_hiz = bq2560x_set_charger_hiz,
	.get_charge_current = NULL,
	.set_boost_voltage = bq2560x_set_boost_voltage,
};

/**********************************************************
*  Function:       bq2560x_irq_work
*  Discription:    handler for chargerIC fault irq in charging process
*  Parameters:   work:chargerIC fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void bq2560x_irq_work(struct work_struct *work)
{
	struct bq2560x_device_info *di = container_of(work, struct bq2560x_device_info, irq_work);
	u8 reg = 0;

	msleep(100); /* sleep 100ms */

	bq2560x_read_byte(BQ2560X_REG_F, &reg);
	bq2560x_read_byte(BQ2560X_REG_F, &reg);

	hwlog_err("boost_ovp_reg [%x]=0x%x\n", BQ2560X_REG_F, reg);

	if (reg & BQ2560X_REG_F_FAULT_BOOST_MASK) {
		hwlog_info("CHARGE_FAULT_BOOST_OCP happened\n");
		atomic_notifier_call_chain(&fault_notifier_list, CHARGE_FAULT_BOOST_OCP, NULL);
	}

	if (di->irq_active == 0) {
		di->irq_active = 1;
		enable_irq(di->irq_int);
	}
}

/**********************************************************
*  Function:       bq2560x_interrupt
*  Discription:    callback function for chargerIC fault irq in charging process
*  Parameters:   irq:chargerIC fault interrupt
*                      _di:bq2560x_device_info
*  return value:  IRQ_HANDLED-sucess or others
**********************************************************/
static irqreturn_t bq2560x_interrupt(int irq, void *_di)
{
	struct bq2560x_device_info *di = _di;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	hwlog_info("bq2560x interrupt happened (%d)!\n", di->irq_active);

	if (di->irq_active == 1) {
		di->irq_active = 0;
		disable_irq_nosync(di->irq_int);
		schedule_work(&di->irq_work);
	}
	else {
		hwlog_info("The irq is not enable,do nothing!\n");
	}

	return IRQ_HANDLED;
}

/**********************************************************
*  Function:       bq2560x_probe
*  Discription:    bq2560x module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret = 0;
	struct bq2560x_device_info *di = NULL;
	struct device_node *np = NULL;
	struct class *power_class = NULL;

	hwlog_info("probe begin\n");

	if (NULL == client || NULL == id) {
		hwlog_err("error: client is null or id is null!\n");
		return -ENOMEM;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_bq2560x_dev = di;

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);

	INIT_WORK(&di->irq_work, bq2560x_irq_work);

	/* check if bq2560x exist */
	if (CHARGE_IC_BAD == bq2560x_device_check()) {
		hwlog_err("error: bq2560x not exists!\n");
		ret = -EINVAL;
		goto bq2560x_fail_0;
	}

	ret = of_property_read_u32(np, "hiz_iin_limit", &(di->hiz_iin_limit));
	if (ret) {
		di->hiz_iin_limit = 0;
	}
	hwlog_info("hiz_iin_limit=%d\n", di->hiz_iin_limit);

	di->gpio_cd = of_get_named_gpio(np, "gpio_cd", 0);
	hwlog_info("gpio_cd=%d\n", di->gpio_cd);

	if (!gpio_is_valid(di->gpio_cd)) {
		hwlog_err("error: gpio(gpio_cd) is not valid!\n");
		ret = -EINVAL;
		goto bq2560x_fail_0;
	}

	ret = gpio_request(di->gpio_cd, "charger_cd");
	if (ret) {
		hwlog_err("error: gpio(gpio_cd) request fail!\n");
		goto bq2560x_fail_0;
	}

	/* set gpio to control CD pin to disable/enable bq2560x IC */
	ret = gpio_direction_output(di->gpio_cd, 0);
	if (ret) {
		hwlog_err("error: gpio(gpio_cd) set output fail!\n");
		goto bq2560x_fail_1;
	}

	di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
	hwlog_info("gpio_int=%d\n", di->gpio_int);

	if (!gpio_is_valid(di->gpio_int)) {
		hwlog_err("error: gpio(gpio_int) is not valid!\n");
		ret = -EINVAL;
		goto bq2560x_fail_1;
	}

	ret = gpio_request(di->gpio_int, "charger_int");
	if (ret) {
		hwlog_err("error: gpio(gpio_int) request fail!\n");
		goto bq2560x_fail_1;
	}

	ret = gpio_direction_input(di->gpio_int);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) set input fail!\n");
		goto bq2560x_fail_2;
	}

	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0) {
		hwlog_err("error: gpio(gpio_int) map to irq fail!\n");
		ret = -EINVAL;
		goto bq2560x_fail_2;
	}

	ret = request_irq(di->irq_int, bq2560x_interrupt, IRQF_TRIGGER_FALLING, "charger_int_irq", di);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) irq request fail!\n");
		di->irq_int = -1;
		goto bq2560x_fail_2;
	}

	disable_irq(di->irq_int);
	di->irq_active = 0;

	ret = charge_ops_register(&bq2560x_ops);
	if (ret) {
		hwlog_err("error: bq2560x charge ops register fail!\n");
		goto bq2560x_fail_3;
	}

	ret = bq2560x_sysfs_create_group(di);
	if (ret) {
		hwlog_err("error: sysfs group create failed!\n");
	}

	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL) {
			charge_dev = device_create(power_class, NULL, 0, NULL, "charger");
		}

		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "bq2560x");
		if (ret) {
			hwlog_err("error: sysfs link create failed!\n");
			goto bq2560x_fail_4;
		}
	}

	/* set bq2560x boost voltage */
	ret = bq2560x_set_boost_voltage(BOOSTV_5000);
	if (ret < 0) {
		hwlog_err("error: set bq2560x boost voltage fail!\n");
	}

	hwlog_info("probe end!\n");
	return 0;

bq2560x_fail_4:
	bq2560x_sysfs_remove_group(di);
bq2560x_fail_3:
	free_irq(di->irq_int, di);
bq2560x_fail_2:
	gpio_free(di->gpio_int);
bq2560x_fail_1:
	gpio_free(di->gpio_cd);
bq2560x_fail_0:
	kfree(di);
	g_bq2560x_dev = NULL;
	np = NULL;

	return ret;
}

/**********************************************************
*  Function:       bq2560x_remove
*  Discription:    bq2560x module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq2560x_remove(struct i2c_client *client)
{
	struct bq2560x_device_info *di = i2c_get_clientdata(client);

	hwlog_info("remove begin\n");

	bq2560x_sysfs_remove_group(di);

	gpio_set_value(di->gpio_cd, 1);

	if (di->gpio_cd) {
		gpio_free(di->gpio_cd);
	}

	if (di->irq_int) {
		free_irq(di->irq_int, di);
	}

	if (di->gpio_int) {
		gpio_free(di->gpio_int);
	}

	kfree(di);

	hwlog_info("remove end\n");
	return 0;
}

MODULE_DEVICE_TABLE(i2c, bq25601);
static struct of_device_id bq2560x_of_match[] = {
	{
		.compatible = "huawei,bq2560x_charger",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id bq2560x_i2c_id[] = {
	{"bq2560x_charger", 0}, {}
};

static struct i2c_driver bq2560x_driver = {
	.probe = bq2560x_probe,
	.remove = bq2560x_remove,
	.id_table = bq2560x_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "bq2560x_charger",
		.of_match_table = of_match_ptr(bq2560x_of_match),
	},
};

/**********************************************************
*  Function:       bq2560x_init
*  Discription:    bq2560x module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init bq2560x_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&bq2560x_driver);
	if (ret) {
		hwlog_err("error: bq2560x i2c_add_driver error!\n");
	}

	return ret;
}

/**********************************************************
*  Function:       bq2560x_exit
*  Discription:    bq2560x module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit bq2560x_exit(void)
{
	i2c_del_driver(&bq2560x_driver);
}

module_init(bq2560x_init);
module_exit(bq2560x_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("bq2560x charger module driver");
MODULE_AUTHOR("HW Inc");
