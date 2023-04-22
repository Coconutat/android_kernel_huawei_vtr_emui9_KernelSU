/*
 * drivers/power/bq25892_charger_sh.c
 *
 * BQ25892/1/2/3/4 charging sensorhub driver
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
#include <linux/hisi/usb/hisi_usb.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/raid/pq.h>
#include <huawei_platform/power/huawei_charger_sh.h>
#ifdef CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif
#include <huawei_platform/power/charger/charger_ap/bq25892/bq25892_charger.h>
#include <linux/hisi/hisi_adc.h>
#include <inputhub_route.h>
#include <protocol.h>
#include <sensor_info.h>

static bool first_charging_done = FALSE;       /* has ever reach charge done state after connect charger. */
static bool charging_again_after_charging_done = FALSE;
extern struct charger_platform_data charger_dts_data;

#define BQ25892_CHARGER_SLAVADDR  0x6b

/**********************************************************
*  Function:       bq25892_read_block
*  Discription:    register read block interface
*  Parameters:   reg:register name
*                      value:register value
*			len: bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_read_block(u8 reg, u8*value, unsigned len)
{
	int ret = mcu_i2c_rw(3, BQ25892_CHARGER_SLAVADDR, &reg, 1, value, len);
	if (ret < 0) {
		hwlog_err("bq25892 read register %d fail!\n", reg);
	} else {
		hwlog_debug("bq25892 read value %d from register %d success!\n", *value, reg);
	}
	return ret;
}

/**********************************************************
*  Function:       bq25892_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_write_byte(uint8_t reg, uint8_t value)
{
	uint8_t tx_buf[2];
	int ret;
	tx_buf[0] = reg;
	tx_buf[1] = value;
	ret = mcu_i2c_rw(3, BQ25892_CHARGER_SLAVADDR, tx_buf, 2, NULL, 0);
	if (ret < 0) {
		hwlog_err("bq25892 write value %d to register %d fail!\n", value, reg);
	} else {
		hwlog_debug("bq25892 write value %d to register %d success!\n", value, reg);
	}
	return ret;
}

/**********************************************************
*  Function:       bq25892_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_read_byte(uint8_t reg, uint8_t*value)
{
	return bq25892_read_block(reg, value, 1);
}


/**********************************************************
*  Function:       bq25892_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = bq25892_read_byte(reg, &val);
	if (ret < 0)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = bq25892_write_byte(reg, val);

	return ret;
}

/**********************************************************
*  Function:       bq25892_read_mask
*  Discription:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_read_mask(u8 reg, u8 MASK, u8 SHIFT, u8 *value)
{
	int ret = 0;
	u8 val = 0;

	ret = bq25892_read_byte(reg, &val);
	if (ret < 0)
		return ret;
	val &= MASK;
	val >>= SHIFT;
	*value = val;

	return 0;
}

#define CONFIG_SYSFS_BQ
#ifdef CONFIG_SYSFS_BQ
/*
 * There are a numerous options that are configurable on the bq25892
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */

#define BQ25892_SYSFS_FIELD(_name, r, f, m, store)                  \
{                                                   \
    .attr = __ATTR(_name, m, bq25892_sysfs_show, store),    \
    .reg = BQ25892_REG_##r,                      \
    .mask = BQ25892_REG_##r##_##f##_MASK,                       \
    .shift = BQ25892_REG_##r##_##f##_SHIFT,                     \
}

#define BQ25892_SYSFS_FIELD_RW(_name, r, f)                     \
	BQ25892_SYSFS_FIELD(_name, r, f, S_IWUSR | S_IRUGO, bq25892_sysfs_store)

#define BQ25892_SYSFS_FIELD_RO(_name, r, f)                         \
	BQ25892_SYSFS_FIELD(_name, r, f, S_IRUGO, NULL)

static ssize_t bq25892_sysfs_show(struct device *dev,
				  struct device_attribute *attr, char *buf);
static ssize_t bq25892_sysfs_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count);

static struct bq25892_sysfs_field_info {
	struct device_attribute attr;
	u8 reg;
	u8 mask;
	u8 shift;
};

static struct bq25892_sysfs_field_info bq25892_sysfs_field_tbl[] = {
	/* sysfs name reg field in reg */
	BQ25892_SYSFS_FIELD_RW(en_hiz, 00, EN_HIZ),
	BQ25892_SYSFS_FIELD_RW(en_ilim, 00, EN_ILIM),
	BQ25892_SYSFS_FIELD_RW(iinlim, 00, IINLIM),
	BQ25892_SYSFS_FIELD_RW(dpm_os, 01, VINDPM_OS),
	BQ25892_SYSFS_FIELD_RW(conv_start, 02, CONV_START),
	BQ25892_SYSFS_FIELD_RW(ico_en, 02, ICO_EN),
	BQ25892_SYSFS_FIELD_RW(force_dpdm, 02, FORCE_DPDM),
	BQ25892_SYSFS_FIELD_RW(auto_dpdm_en, 02, AUTO_DPDM_EN),
	BQ25892_SYSFS_FIELD_RW(chg_config, 03, CHG_CONFIG),
	BQ25892_SYSFS_FIELD_RW(otg_config, 03, OTG_CONFIG),
	BQ25892_SYSFS_FIELD_RW(sys_min, 03, SYS_MIN),
	BQ25892_SYSFS_FIELD_RW(ichg, 04, ICHG),
	BQ25892_SYSFS_FIELD_RW(iprechg, 05, IPRECHG),
	BQ25892_SYSFS_FIELD_RW(iterm, 05, ITERM),
	BQ25892_SYSFS_FIELD_RW(vreg, 06, VREG),
	BQ25892_SYSFS_FIELD_RW(batlowv, 06, BATLOWV),
	BQ25892_SYSFS_FIELD_RW(vrechg, 06, VRECHG),
	BQ25892_SYSFS_FIELD_RW(en_term, 07, EN_TERM),
	BQ25892_SYSFS_FIELD_RW(watchdog, 07, WATCHDOG),
	BQ25892_SYSFS_FIELD_RW(en_timer, 07, EN_TIMER),
	BQ25892_SYSFS_FIELD_RW(chg_timer, 07, CHG_TIMER),
	BQ25892_SYSFS_FIELD_RW(jeta_iset, 07, JEITA_ISET),
	BQ25892_SYSFS_FIELD_RW(bat_comp, 08, BAT_COMP),
	BQ25892_SYSFS_FIELD_RW(vclamp, 08, VCLAMP),
	BQ25892_SYSFS_FIELD_RW(treg, 08, TREG),
	BQ25892_SYSFS_FIELD_RW(force_ico, 09, FORCE_ICO),
	BQ25892_SYSFS_FIELD_RW(batfet_disable, 09, BATFET_DISABLE),
	BQ25892_SYSFS_FIELD_RW(jeita_vset, 09, JEITA_VSET),
	BQ25892_SYSFS_FIELD_RW(boost_vol, 0A, BOOSTV),
	BQ25892_SYSFS_FIELD_RW(boost_lim, 0A, BOOST_LIM),
	BQ25892_SYSFS_FIELD_RW(vindpm, 0D, VINDPM),
	BQ25892_SYSFS_FIELD_RW(force_vindpm, 0D, FORCE_VINDPM),
	BQ25892_SYSFS_FIELD_RW(reg_rst, 14, REG_RST),
	BQ25892_SYSFS_FIELD_RO(vbus_stat, 0B, VBUS_STAT),
	BQ25892_SYSFS_FIELD_RO(chrg_stat, 0B, CHRG_STAT),
	BQ25892_SYSFS_FIELD_RO(pg_stat, 0B, PG_STAT),
	BQ25892_SYSFS_FIELD_RO(sdp_stat, 0B, SDP_STAT),
	BQ25892_SYSFS_FIELD_RO(vsys_stat, 0B, VSYS_STAT),
	BQ25892_SYSFS_FIELD_RO(watchdog_fault, 0C, WATCHDOG_FAULT),
	BQ25892_SYSFS_FIELD_RO(boost_fault, 0C, BOOST_FAULT),
	BQ25892_SYSFS_FIELD_RO(chrg_fault, 0C, CHRG_FAULT),
	BQ25892_SYSFS_FIELD_RO(bat_fault, 0C, BAT_FAULT),
	BQ25892_SYSFS_FIELD_RO(ntc_fault, 0C, NTC_FAULT),
	BQ25892_SYSFS_FIELD_RO(therm_stat, 0E, THERM_STAT),
	BQ25892_SYSFS_FIELD_RO(bat_vol, 0E, BATV),
	BQ25892_SYSFS_FIELD_RO(sys_volt, 0F, SYSV),
	BQ25892_SYSFS_FIELD_RO(vbus_volt, 11, VBUSV),
	BQ25892_SYSFS_FIELD_RO(ichg_adc, 12, ICHGR),
	BQ25892_SYSFS_FIELD_RO(vdpm_stat, 13, VDPM_STAT),
	BQ25892_SYSFS_FIELD_RO(idpm_stat, 13, IDPM_STAT),
	BQ25892_SYSFS_FIELD_RO(idpm_lim, 13, IDPM_LIM),
	BQ25892_SYSFS_FIELD_RO(ico_optimized, 14, ICO_OPTIMIZED),
	BQ25892_SYSFS_FIELD_RW(reg_addr, NONE, NONE),
	BQ25892_SYSFS_FIELD_RW(reg_value, NONE, NONE),
};

static struct attribute *bq25892_sysfs_attrs[ARRAY_SIZE(bq25892_sysfs_field_tbl) + 1];

static const struct attribute_group bq25892_sysfs_attr_group = {
	.attrs = bq25892_sysfs_attrs,
};

/**********************************************************
*  Function:       bq25892_sysfs_init_attrs
*  Discription:    initialize bq25892_sysfs_attrs[] for bq25892 attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void bq25892_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(bq25892_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		bq25892_sysfs_attrs[i] = &bq25892_sysfs_field_tbl[i].attr.attr;

	bq25892_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

/**********************************************************
*  Function:       bq25892_sysfs_field_lookup
*  Discription:    get the current device_attribute from bq25892_sysfs_field_tbl by attr's name
*  Parameters:   name:evice attribute name
*  return value:  bq25892_sysfs_field_tbl[]
**********************************************************/
static struct bq25892_sysfs_field_info *bq25892_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(bq25892_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		if (!strcmp(name, bq25892_sysfs_field_tbl[i].attr.attr.name))
			break;

	if (i >= limit)
		return NULL;

	return &bq25892_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       bq25892_sysfs_show
*  Discription:    show the value for all bq25892 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t bq25892_sysfs_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	struct bq25892_sysfs_field_info *info;
	struct bq25892_sysfs_field_info *info2;
	int ret;
	u8 v;

	info = bq25892_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	if (!strncmp("reg_addr", attr->attr.name, strlen("reg_addr"))) {
		return scnprintf(buf, PAGE_SIZE, "0x%hhx\n", info->reg);
	}

	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = bq25892_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}

	ret = bq25892_read_mask(info->reg, info->mask, info->shift, &v);
	if (ret)
		return ret;

	return scnprintf(buf, PAGE_SIZE, "%hhx\n", v);
}

/**********************************************************
*  Function:       bq25892_sysfs_store
*  Discription:    set the value for all bq25892 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t bq25892_sysfs_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	struct bq25892_sysfs_field_info *info;
	struct bq25892_sysfs_field_info *info2;
	int ret;
	u8 v;

	info = bq25892_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	ret = kstrtou8(buf, 0, &v);
	if (ret < 0)
		return ret;
	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = bq25892_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}
	if (!strncmp(("reg_addr"), attr->attr.name, strlen("reg_addr"))) {
		if (v < (u8) BQ25892_REG_TOTAL_NUM) {
			info->reg = v;
			return count;
		} else {
			return -EINVAL;
		}
	}

	ret = bq25892_write_mask(info->reg, info->mask, info->shift, v);
	if (ret)
		return ret;

	return count;
}

/**********************************************************
*  Function:       bq25892_sysfs_create_group
*  Discription:    create the bq25892 device sysfs group
*  Parameters:   di:bq25892_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_sysfs_create_group(struct device *dev)
{
	bq25892_sysfs_init_attrs();

    return sysfs_create_group(&dev->kobj,&bq25892_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Discription:    remove the bq25892 device sysfs group
*  Parameters:   di:bq25892_device_info
*  return value:  NULL
**********************************************************/
static void bq25892_sysfs_remove_group(struct device *dev)
{
    sysfs_remove_group(&dev->kobj, &bq25892_sysfs_attr_group);
}
#else
static int bq25892_sysfs_create_group(struct bq25892_device_info *di)
{
	return 0;
}

static inline void bq25892_sysfs_remove_group(struct bq25892_device_info *di)
{
}
#endif

/**********************************************************
*  Function:       bq25892_device_check
*  Discription:    check chip i2c communication
*  Parameters:   null
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_device_check(void)
{
	int ret = 0;
	u8 reg = 0xff;
	ret |= bq25892_read_byte(BQ25892_REG_14, &reg);
	if (ret) {
		hwlog_err("read bq25892 version error.\n");
		return CHARGE_IC_BAD;
	}

	if ((BQ25892 == (reg & CHIP_VERSION_MASK))
	    && (CHIP_REVISION == (reg & CHIP_REVISION_MASK))) {
		hwlog_info("bq25892 is good.\n");
		return CHARGE_IC_GOOD;
	} else {
		hwlog_err("bq25892 is bad.\n");
		return CHARGE_IC_BAD;
	}
}

/**********************************************************
*  Function:       bq25892_set_adc_conv_rate
*  Discription:    set adc conversion rate
*  Parameters:   mode:0-One shot ADC conversion
*                              1-Start 1s Continuous Conversion
*  return value:  0-sucess or others-fail
**********************************************************/

static int bq25892_set_adc_conv_rate(int mode)
{
	if (mode) {
		mode = 1;
	}

	hwlog_debug("adc conversion rate mode is set to %d\n", mode);

	return bq25892_write_mask(BQ25892_REG_02, BQ25892_REG_02_CONV_RATE_MASK,
				  BQ25892_REG_02_CONV_RATE_SHIFT, mode);
}

/**********************************************************
*  Function:       bq25892_check_input_current_exit_PFM_when_CD
*  Discription:    ONLY TI bq25892 charge chip has a bug.
*                      When charger is in charging done state and under PFM mode,
*                      there is risk to slowly drive Q4 to open when unplug charger.
*                      The result is VSYS drops to 2.0V and reach milliseconds interval.
*                      The VSYS drop can be captured by hi6421 that result in SMPL.
*                      The solution that TI suggest: when charger chip is bq25892, and under charging done state,
*                      set IINLIM(reg00 bit0-bit5) to 400mA or below to force chargeIC to exit PFM
*  Parameters:   value:input current value that upper layer suggests
*  return:          current that upper layer suggests  or  current that force bq25892 to exit PFM
**********************************************************/
static int bq25892_check_input_current_exit_PFM_when_CD(unsigned int limit_default)
{
	u8 reg0B = 0;
	u8 reg14 = 0;
	int ret = 0;

	ret |= bq25892_read_byte(BQ25892_REG_0B, &reg0B);
	ret |= bq25892_read_byte(BQ25892_REG_14, &reg14);
	if (ret != 0) {
		hwlog_err
		    ("read bq25892 reg OB or 14 fail ret:%d, then return default current:%d\n",
		     ret, limit_default);
		return limit_default;
	}

	if (((reg14 & BQ25892_REG_14_REG_PN_MASK) == BQ25892_REG_14_REG_PN_IS_25892) &&	/* bq25892 */
	    (limit_default > IINLIM_FOR_BQ25892_EXIT_PFM)) {
		if ((reg0B & BQ25892_REG_0B_CHRG_STAT_MASK) == BQ25892_CHGR_STAT_CHARGE_DONE) {	/* charging done */
			if (FALSE == first_charging_done) {	/* first time charging done */
				first_charging_done = TRUE;
				limit_default = IINLIM_FOR_BQ25892_EXIT_PFM;
			} else if (FALSE == charging_again_after_charging_done) {	/* charging done and there is NOT second charging */
				limit_default = IINLIM_FOR_BQ25892_EXIT_PFM;
			} else {
				/* charging done but thers is second charging, keep iinlim as upper layer suggests */
			}

			if (IINLIM_FOR_BQ25892_EXIT_PFM == limit_default) {
				hwlog_err
				    ("this is bq25892 and CD, 1stCD(%d), 2nd FC(%d), limit input current to %d to force exit PFM\n",
				     first_charging_done,
				     charging_again_after_charging_done,
				     limit_default);
			}
		} else if (TRUE == first_charging_done) {	/* fast charging,  is it ever charging done? */
			charging_again_after_charging_done = TRUE;	/* keep iinlim as upper layer suggests */
		} else {
			/* keep iinlim as upper layer suggests, as normal charging before first charging done */
		}
	}

	return limit_default;
}

/**********************************************************
*  Function:       bq25892_set_input_current
*  Discription:    set the input current in charging process
*  Parameters:   value:input current value
*  return value:  0-sucess or others-fail
**********************************************************/

static int bq25892_set_input_current(int value)
{
	unsigned int limit_current = 0;
	u8 Iin_limit = 0;
	limit_current = value;
	if (limit_current <= IINLIM_MIN_100) {
		hwlog_info("input current %dmA is out of range:%dmA!!", value,
			   IINLIM_MIN_100);
		limit_current = IINLIM_MIN_100;
	} else if (limit_current > IINLIM_MAX_3250) {
		hwlog_info("input current %dmA is out of range:%dmA!!", value,
			   IINLIM_MAX_3250);
		limit_current = IINLIM_MAX_3250;
	} else {
		/*do nothing*/
	}

	hwlog_debug("input current is set %dmA\n", limit_current);

	/* in order to avoid smpl because bq25892 bug,
	   set iinlim to 400mA if under charging done and chip ic is bq25892, OTHERWISE keep it change */
	limit_current =
	    bq25892_check_input_current_exit_PFM_when_CD(limit_current);

	hwlog_debug("input current is set %dmA\n", limit_current);
	Iin_limit = (limit_current - IINLIM_MIN_100) / IINLIM_STEP_50;
	return bq25892_write_mask(BQ25892_REG_00, BQ25892_REG_00_IINLIM_MASK,
				  BQ25892_REG_00_IINLIM_SHIFT, Iin_limit);
}

/**********************************************************
*  Function:       bq25892_set_charge_current
*  Discription:    set the charge current in charging process
*  Parameters:   value:charge current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_set_charge_current(int value)
{
	unsigned int currentmA = 0;
	u8 ichg = 0;
	currentmA = value;
	if (currentmA < 0) {
		currentmA = 0;
	} else if (currentmA > ICHG_MAX_5056) {
		hwlog_info("set charge current %dmA is out of range:%dmA!!",
			   value, ICHG_MAX_5056);
		currentmA = ICHG_MAX_5056;
	} else {
		/*do nothing*/
	}
	hwlog_debug("charge current is set %dmA\n", currentmA);
	ichg = currentmA / ICHG_STEP_64;
	return bq25892_write_mask(BQ25892_REG_04, BQ25892_REG_04_ICHG_MASK,
				  BQ25892_REG_04_ICHG_SHIFT, ichg);
}

/**********************************************************
*  Function:       bq25892_set_terminal_voltage
*  Discription:    set the terminal voltage in charging process
*  Parameters:   value:terminal voltage value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_set_terminal_voltage(int value)
{
	unsigned int voltagemV = 0;
	u8 Voreg = 0;
	voltagemV = value;
	if (voltagemV < VCHARGE_MIN_3840) {
		hwlog_info("set terminal voltage %dmV is out of range:%dmV!!",
			   value, VCHARGE_MIN_3840);
		voltagemV = VCHARGE_MIN_3840;
	} else if (voltagemV > VCHARGE_MAX_4496) {
		hwlog_info("set terminal voltage %dmV is out of range:%dmV!!",
			   value, VCHARGE_MAX_4496);
		voltagemV = VCHARGE_MAX_4496;
	}
	hwlog_debug("terminal voltage is set %dmV\n", voltagemV);
	Voreg = (voltagemV - VCHARGE_MIN_3840) / VCHARGE_STEP_16;
	return bq25892_write_mask(BQ25892_REG_06, BQ25892_REG_06_VREG_MASK,
				  BQ25892_REG_06_VREG_SHIFT, Voreg);
}

/**********************************************************
*  Function:       bq25892_set_charge_enable
*  Discription:    set the charge enable in charging process
*  Parameters:   enable:charge enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_set_charge_enable(int enable)
{
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	uint8_t sub_cmd[2] = {0,};
	int ret = 0;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	sub_cmd[0] = CHARGE_SET_CHARGE_ENABLE;
	sub_cmd[1] = enable;
	pkg_ap.tag = TAG_CHARGER;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = sub_cmd;
	pkg_ap.wr_len = sizeof(sub_cmd);
	ret=write_customize_cmd(&pkg_ap,  &pkg_mcu);
	if(ret) {
		hwlog_err("send set charge enable to sensorhub fail, ret is %d\n", ret);
		return -1;
	} else {
		if(pkg_mcu.errno!=0) {
			hwlog_err("send set charge enable to sensorhub fail, errno is %d\n", pkg_mcu.errno);
			return -1;
		} else {
			hwlog_info( "send set charge enable to sensorhub success\n");
			return 0;
		}
	}
}

/**********************************************************
*  Function:       bq25892_set_otg_enable
*  Discription:    set the otg mode enable in charging process
*  Parameters:   enable:otg mode  enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_set_otg_enable(int enable)
{
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	uint8_t sub_cmd[2] = {0,};
	int ret = 0;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	sub_cmd[0] = CHARGE_SET_OTG_ENABLE;
	sub_cmd[1] = enable;
	pkg_ap.tag = TAG_CHARGER;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = sub_cmd;
	pkg_ap.wr_len = sizeof(sub_cmd);
	ret=write_customize_cmd(&pkg_ap,  &pkg_mcu);
	if(ret) {
		hwlog_err("send set otg enable to sensorhub fail, ret is %d\n", ret);
		return -1;
	} else {
		if(pkg_mcu.errno!=0) {
			hwlog_err("send set otg enable to sensorhub fail, errno is %d\n", pkg_mcu.errno);
			return -1;
		} else {
			hwlog_info( "send set otg enable to sensorhub success\n");
			return 0;
		}
	}
}

/**********************************************************
*  Function:       bq25892_get_vbus_mv
*  Discription:    get voltage of vbus
*  Parameters:   vbus_mv:voltage of vbus
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_get_vbus_mv(unsigned int *vbus_mv)
{
	u8 reg = 0;
	int ret = 0;
	ret = bq25892_read_byte(BQ25892_REG_11, &reg);
	reg = reg & BQ25892_REG_11_VBUSV_MASK;
	*vbus_mv =
	    (unsigned int)reg * BQ25892_REG_11_VBUSV_STEP_MV +
	    BQ25892_REG_11_VBUSV_OFFSET_MV;
	hwlog_debug(" vbus mv  is  %dmV\n", *vbus_mv);
	return ret;
}

/**********************************************************
*  Function:       bq25892_get_ilim
*  Discription:    get average value for ilim
*  Parameters:     NULL
*  return value:   average value for ilim
**********************************************************/
static int bq25892_get_ilim(void)
{
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	uint8_t sub_cmd = 0;
	int ret = 0, ilim;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	sub_cmd = CHARGE_GET_ILIM;
	pkg_ap.tag = TAG_CHARGER;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = &sub_cmd;
	pkg_ap.wr_len = sizeof(sub_cmd);
	ret=write_customize_cmd(&pkg_ap,  &pkg_mcu);
	if(ret) {
		hwlog_err("send get ilim enable to sensorhub fail, ret is %d\n", ret);
		return -1;
	} else {
		if(pkg_mcu.errno!=0) {
			hwlog_err("send get ilim enable to sensorhub fail, errno is %d\n", pkg_mcu.errno);
			return -1;
		} else {
			ilim = *((int *)pkg_mcu.data);
			hwlog_info( "send get ilim enable to sensorhub success\n");
			return ilim;
		}
	}
}

static int bq25892_vbat_sys(void)
{
	int i = 0;
	int retry_times = 3;
	int V_sample = -1;

	for (i = 0; i < retry_times; ++i) {
		V_sample = hisi_adc_get_value(charger_dts_data.adc_channel_vbat_sys);
		if (V_sample < 0) {
			hwlog_err("adc read channel 15 fail!\n");
		} else {
			break;
		}
	}
	return V_sample;
}

static int bq25892_get_vbat_sys(void)
{
	int i;
	int cnt = 0;
	int V_temp;
	int delay_times = 100;
	int sample_num = 5;	/* use 5 samples to get an average value*/
	int sum = 0;

	for (i = 0; i < sample_num; ++i) {
		V_temp = bq25892_vbat_sys();
		if (V_temp >= 0) {
			sum += V_temp;
			++cnt;
		} else {
			hwlog_err("bq25892 get V_temp fail!\n");
		}
		msleep(delay_times);
	}
	if (cnt > 0) {
		return (3 * 1000 * sum / cnt);
	} else {
		hwlog_err("use 0 as default Vvlim!\n");
		return 0;
	}
}

/**********************************************************
*  Function:       bq25892_dump_register
*  Discription:    print the register value in charging process
*  Parameters:   reg_value:string for save register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_dump_register(char *reg_value)
{
	u8 reg[BQ25892_REG_TOTAL_NUM] = { 0 };
	char buff[26] = { 0 };
	int i = 0;

	memset(reg_value, 0, CHARGELOG_SIZE);
	snprintf(buff, 26, "%-8.2d", bq25892_get_ilim());
	strncat(reg_value, buff, strlen(buff));
	bq25892_read_block(0, &reg[0], BQ25892_REG_TOTAL_NUM);
	bq25892_read_block(0, &reg[0], BQ25892_REG_TOTAL_NUM);
	for (i = 0; i < BQ25892_REG_TOTAL_NUM; i++) {
		snprintf(buff, 26, "0x%-8.2x", reg[i]);
		strncat(reg_value, buff, strlen(buff));
	}
	return 0;
}

/**********************************************************
*  Function:       bq25892_get_register_head
*  Discription:    print the register head in charging process
*  Parameters:   reg_head:string for save register head
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_get_register_head(char *reg_head)
{
	char buff[26] = { 0 };
	int i = 0;

	memset(reg_head, 0, CHARGELOG_SIZE);
	snprintf(buff, 26, "Ibus    ");
	strncat(reg_head, buff, strlen(buff));
	for (i = 0; i < BQ25892_REG_TOTAL_NUM; i++) {
		snprintf(buff, 26, "Reg[%d] ", i);
		strncat(reg_head, buff, strlen(buff));
	}
	return 0;
}

/**********************************************************
*  Function:       bq25892_set_batfet_disable
*  Discription:    set the batfet disable in charging process
*  Parameters:   disable:batfet disable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_set_batfet_disable(int disable)
{
	return bq25892_write_mask(BQ25892_REG_09,
				  BQ25892_REG_09_BATFET_DISABLE_MASK,
				  BQ25892_REG_09_BATFET_DISABLE_SHIFT, disable);
}

/**********************************************************
*  Function:       bq25892_set_watchdog_timer
*  Discription:    set the watchdog timer in charging process
*  Parameters:   value:watchdog timer value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_set_watchdog_timer(int value)
{
	u8 val = 0;
	u8 dog_time = value;
	if (dog_time >= WATCHDOG_TIMER_160_S) {
		val = BQ25892_REG_07_WATCHDOG_160;
	} else if (dog_time >= WATCHDOG_TIMER_80_S) {
		val = BQ25892_REG_07_WATCHDOG_80;
	} else if (dog_time >= WATCHDOG_TIMER_40_S) {
		val = BQ25892_REG_07_WATCHDOG_40;
	} else {
		val = BQ25892_REG_07_WATCHDOG_DIS;
	}
	hwlog_debug(" watch dog timer is %d ,the register value is set %d \n",
		    dog_time, val);
	return bq25892_write_mask(BQ25892_REG_07, BQ25892_REG_07_WATCHDOG_MASK,
				  BQ25892_REG_07_WATCHDOG_SHIFT, val);
}

/**********************************************************
*  Function:       bq25892_set_charger_hiz
*  Discription:    set the charger hiz close watchdog
*  Parameters:   enable:charger in hiz or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_set_charger_hiz(int enable)
{
	int ret = 0;

	if (enable > 0) {
		ret |=
		    bq25892_write_mask(BQ25892_REG_00,
				       BQ25892_REG_00_EN_HIZ_MASK,
				       BQ25892_REG_00_EN_HIZ_SHIFT, TRUE);
	} else {
		ret |=
		    bq25892_write_mask(BQ25892_REG_00,
				       BQ25892_REG_00_EN_HIZ_MASK,
				       BQ25892_REG_00_EN_HIZ_SHIFT, FALSE);
	}
	return ret;
}

/**********************************************************
*  Function:       bq25892_check_input_dpm_state
*  Discription:    check whether VINDPM or IINDPM
*  Parameters:     NULL
*  return value:   TRUE means VINDPM or IINDPM
*                  FALSE means NoT DPM
**********************************************************/
static int bq25892_check_input_dpm_state(void)
{
	u8 reg = 0;
	int ret = -1;

	ret = bq25892_read_byte(BQ25892_REG_13, &reg);
	if (ret < 0) {
		hwlog_err("bq25892_check_input_dpm_state err\n");
		return ret;
	}

	if ((reg & BQ25892_REG_13_VDPM_STAT_MASK)
	    || (reg & BQ25892_REG_13_IDPM_STAT_MASK)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static struct charge_device_ops bq25892_ops = {
	.dev_check = bq25892_device_check,
	.set_adc_conv_rate = bq25892_set_adc_conv_rate,
	.set_input_current = bq25892_set_input_current,
	.set_charge_current = bq25892_set_charge_current,
	.set_terminal_voltage = bq25892_set_terminal_voltage,
	.set_charge_enable = bq25892_set_charge_enable,
	.set_otg_enable = bq25892_set_otg_enable,
	.dump_register = bq25892_dump_register,
	.get_register_head = bq25892_get_register_head,
	.set_watchdog_timer = bq25892_set_watchdog_timer,
	.set_batfet_disable = bq25892_set_batfet_disable,
	.get_ibus = bq25892_get_ilim,
	.get_vbus = bq25892_get_vbus_mv,
	.get_vbat_sys = bq25892_get_vbat_sys,
	.set_charger_hiz = bq25892_set_charger_hiz,
	.check_input_dpm_state = bq25892_check_input_dpm_state,
};

/**********************************************************
*  Function:       bq25892_probe
*  Discription:    bq25892 module probe
*  Parameters:     NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_probe(struct platform_device *pdev)
{
    int ret = 0;
    struct charge_device_ops *ops = NULL;
    struct class *power_class = NULL;

    ops = &bq25892_ops;
    ret = charge_ops_register(ops);
    if (ret) {
    	hwlog_err("register charge ops failed!\n");
    	goto bq25892_fail_0;
    }

    ret = bq25892_sysfs_create_group(&pdev->dev);
    if(ret)
    {
        hwlog_err("create sysfs entries failed!\n");
        goto bq25892_fail_0;
    }

    power_class = hw_power_get_class();
    if(power_class)
    {
        if(charge_dev == NULL)
            charge_dev = device_create(power_class, NULL, 0, NULL,"charger");
        ret = sysfs_create_link(&charge_dev->kobj, &pdev->dev.kobj, "bq25892");
        if(ret) {
            hwlog_err("create link to bq25892 fail.\n");
	    goto bq25892_fail_1;
        }
    }

    hwlog_info("bq25892 sensorhub init ok!\n");
    return 0;

bq25892_fail_1:
    bq25892_sysfs_remove_group(&pdev->dev);
bq25892_fail_0:
    return ret;
}

/**********************************************************
*  Function:       bq25892_exit
*  Discription:    bq25892 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25892_remove(struct platform_device *pdev)
{
    bq25892_sysfs_remove_group(&pdev->dev);
    return 0;
}

static struct of_device_id bq25892_of_match[] = {
	{
	 .compatible = "huawei,bq25892_charger_sensorhub",
	 .data = NULL,
	 },
	{
	 },
};

static struct platform_driver bq25892_driver = {
	.probe = bq25892_probe,
	.remove = bq25892_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "bq25892_charger_sensorhub",
		   .of_match_table = of_match_ptr(bq25892_of_match),
	},
};

/**********************************************************
*  Function:       charge_core_init
*  Discription:    charge module initialization
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init bq25892_init(void)
{
	return platform_driver_register(&bq25892_driver);
}

/**********************************************************
*  Function:       charge_core_exit
*  Discription:    charge module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit bq25892_exit(void)
{
	platform_driver_unregister(&bq25892_driver);
}

module_init(bq25892_init);
module_exit(bq25892_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("bq25892 sensorhub charger module driver");
MODULE_AUTHOR("HW Inc");

