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
#ifdef CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif
#include <huawei_platform/power/battery_voltage.h>
#include <bq25882_charger.h>

#define HWLOG_TAG bq25882_charger
HWLOG_REGIST();

struct bq25882_device_info *g_bq25882_dev;

static int iin_set = IINLIM_MIN;

#define MSG_LEN                      (2)
#define BUF_LEN                      (26)
#define CONV_RETRY                   (8)

/**********************************************************
*  Function:       bq25882_write_block
*  Discription:    register write block interface
*  Parameters:   di:bq25882_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_write_block(struct bq25882_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
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
*  Function:       bq25882_read_block
*  Discription:    register read block interface
*  Parameters:   di:bq25882_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_read_block(struct bq25882_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
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
*  Function:       bq25882_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_write_byte(u8 reg, u8 value)
{
	struct bq25882_device_info *di = g_bq25882_dev;
	u8 temp_buffer[MSG_LEN] = { 0 }; /* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return bq25882_write_block(di, temp_buffer, reg, 1);
}

/**********************************************************
*  Function:       bq25882_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_read_byte(u8 reg, u8 *value)
{
	struct bq25882_device_info *di = g_bq25882_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	return bq25882_read_block(di, value, reg, 1);
}

/**********************************************************
*  Function:       bq25882_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = bq25882_read_byte(reg, &val);
	if (ret < 0) {
		return ret;
	}

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = bq25882_write_byte(reg, val);

	return ret;
}

/**********************************************************
*  Function:       bq25882_read_mask
*  Discription:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_read_mask(u8 reg, u8 MASK, u8 SHIFT, u8 *value)
{
	int ret = 0;
	u8 val = 0;

	ret = bq25882_read_byte(reg, &val);
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
 * There are a numerous options that are configurable on the bq25882
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */

#define BQ25882_SYSFS_FIELD(_name, r, f, m, store)                  \
{                                                   \
	.attr = __ATTR(_name, m, bq25882_sysfs_show, store),    \
	.reg = BQ25882_REG_##r,                      \
	.mask = BQ25882_REG_##r##_##f##_MASK,                       \
	.shift = BQ25882_REG_##r##_##f##_SHIFT,                     \
}

#define BQ25882_SYSFS_FIELD_RW(_name, r, f)                     \
	BQ25882_SYSFS_FIELD(_name, r, f, S_IWUSR | S_IRUGO, bq25882_sysfs_store)

#define BQ25882_SYSFS_FIELD_RO(_name, r, f)                         \
	BQ25882_SYSFS_FIELD(_name, r, f, S_IRUGO, NULL)

static ssize_t bq25882_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf);
static ssize_t bq25882_sysfs_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count);

struct bq25882_sysfs_field_info {
	struct device_attribute attr;
	u8 reg;
	u8 mask;
	u8 shift;
};

static struct bq25882_sysfs_field_info bq25882_sysfs_field_tbl[] = {
	/* sysfs name reg field in reg */
	BQ25882_SYSFS_FIELD_RW(vreg,          BVRL,  VREG),
	BQ25882_SYSFS_FIELD_RW(en_hiz,        CCL,   EN_HIZ),
	BQ25882_SYSFS_FIELD_RW(en_ilim,       CCL,   EN_ILIM),
	BQ25882_SYSFS_FIELD_RW(ichg,          CCL,   ICHG),
	BQ25882_SYSFS_FIELD_RW(vindpm,        IVL,   VINDPM),
	BQ25882_SYSFS_FIELD_RW(en_bat_dischg, IVL,   EN_BAT_DISCHG),
	BQ25882_SYSFS_FIELD_RW(force_ico,     ICL,   FORCE_ICO),
	BQ25882_SYSFS_FIELD_RW(force_indet,   ICL,   FORCE_INDET),
	BQ25882_SYSFS_FIELD_RW(en_ico,        ICL,   EN_ICO),
	BQ25882_SYSFS_FIELD_RW(iindpm,        ICL,   IINDPM),
	BQ25882_SYSFS_FIELD_RW(iprechg,       PTCL,  IPRECHG),
	BQ25882_SYSFS_FIELD_RW(iterm,         PTCL,  ITERM),
	BQ25882_SYSFS_FIELD_RW(en_term,       CC1,   EN_TERM),
	BQ25882_SYSFS_FIELD_RW(watchdog,      CC1,   WATCHDOG),
	BQ25882_SYSFS_FIELD_RW(en_timer,      CC1,   EN_TIMER),
	BQ25882_SYSFS_FIELD_RW(chg_timer,     CC1,   CHG_TIMER),
	BQ25882_SYSFS_FIELD_RW(tmr2x_en,      CC1,   TMR2X_EN),
	BQ25882_SYSFS_FIELD_RW(en_otg,        CC2,   EN_OTG),
	BQ25882_SYSFS_FIELD_RW(auto_indet_en, CC2,   AUTO_INDET_EN),
	BQ25882_SYSFS_FIELD_RW(treg,          CC2,   TREG),
	BQ25882_SYSFS_FIELD_RW(en_chg,        CC2,   EN_CHG),
	BQ25882_SYSFS_FIELD_RW(batlowv,       CC2,   BATLOWV),
	BQ25882_SYSFS_FIELD_RW(vrechg,        CC2,   VRECHG),
	BQ25882_SYSFS_FIELD_RW(pfm_dis,       CC3,   PFM_DIS),
	BQ25882_SYSFS_FIELD_RW(wd_rst,        CC3,   WD_RST),
	BQ25882_SYSFS_FIELD_RW(sys_min,       CC3,   SYS_MIN),
	BQ25882_SYSFS_FIELD_RW(jeta_vset,     CC4,   JEITA_VSET),
	BQ25882_SYSFS_FIELD_RW(jeta_iseth,    CC4,   JEITA_ISETH),
	BQ25882_SYSFS_FIELD_RW(jeta_isetc,    CC4,   JEITA_ISETC),
	BQ25882_SYSFS_FIELD_RW(otg_ilim,      OC,    OTG_ILIM),
	BQ25882_SYSFS_FIELD_RW(otg_vlim,      OC,    OTG_VLIM),
	BQ25882_SYSFS_FIELD_RO(ico_ilim,      ICOCL, ICO_ILIM),
	BQ25882_SYSFS_FIELD_RO(iindpm_stat,   CS1,   IINDPM_STAT),
	BQ25882_SYSFS_FIELD_RO(vindpm_stat,   CS1,   VINDPM_STAT),
	BQ25882_SYSFS_FIELD_RO(treg_stat,     CS1,   TREG_STAT),
	BQ25882_SYSFS_FIELD_RO(wd_stat,       CS1,   WD_STAT),
	BQ25882_SYSFS_FIELD_RO(chrg_stat,     CS1,   CHRG_STAT),
	BQ25882_SYSFS_FIELD_RO(pg_stat,       CS2,   PG_STAT),
	BQ25882_SYSFS_FIELD_RO(vbus_stat,     CS2,   VBUS_STAT),
	BQ25882_SYSFS_FIELD_RO(ico_stat,      CS2,   ICO_STAT),
	BQ25882_SYSFS_FIELD_RO(vsys_stat,     CS2,   VSYS_STAT),
	BQ25882_SYSFS_FIELD_RO(ts_stat,       NS,    TS_STAT),
	BQ25882_SYSFS_FIELD_RO(vbus_ovp_stat, FS,    VBUS_OVP_STAT),
	BQ25882_SYSFS_FIELD_RO(tshut_stat,    FS,    TSHUT_STAT),
	BQ25882_SYSFS_FIELD_RO(batovp_stat,   FS,    BATOVP_STAT),
	BQ25882_SYSFS_FIELD_RO(tmr_stat,      FS,    TMR_STAT),
	BQ25882_SYSFS_FIELD_RO(otg_flag,      FF,    OTG_FLAG),
	BQ25882_SYSFS_FIELD_RW(reg_rst,       PI,    REG_RST),
};

static struct attribute *bq25882_sysfs_attrs[ARRAY_SIZE(bq25882_sysfs_field_tbl) + 1];

static const struct attribute_group bq25882_sysfs_attr_group = {
	.attrs = bq25882_sysfs_attrs,
};

/**********************************************************
*  Function:       bq25882_sysfs_init_attrs
*  Discription:    initialize bq25882_sysfs_attrs[] for bq25882 attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void bq25882_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(bq25882_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		bq25882_sysfs_attrs[i] = &bq25882_sysfs_field_tbl[i].attr.attr;
	}

	bq25882_sysfs_attrs[limit] = NULL; /* Has additional entry for this */
}

/**********************************************************
*  Function:       bq25882_sysfs_field_lookup
*  Discription:    get the current device_attribute from bq25882_sysfs_field_tbl by attr's name
*  Parameters:   name:evice attribute name
*  return value:  bq25882_sysfs_field_tbl[]
**********************************************************/
static struct bq25882_sysfs_field_info *bq25882_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(bq25882_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strcmp(name, bq25882_sysfs_field_tbl[i].attr.attr.name)) {
			break;
		}
	}

	if (i >= limit) {
		return NULL;
	}

	return &bq25882_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       bq25882_sysfs_show
*  Discription:    show the value for all bq25882 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t bq25882_sysfs_show(struct device *dev,
					struct device_attribute *attr, char *buf)
{
	struct bq25882_sysfs_field_info *info;
	int ret;
	u8 v;

	info = bq25882_sysfs_field_lookup(attr->attr.name);
	if (!info) {
		hwlog_err("error: get sysfs entries failed!\n");
		return -EINVAL;
	}

	ret = bq25882_read_mask(info->reg, info->mask, info->shift, &v);
	if (ret) {
		return ret;
	}

	return scnprintf(buf, PAGE_SIZE, "%hhx\n", v);
}

/**********************************************************
*  Function:       bq25882_sysfs_store
*  Discription:    set the value for all bq25882 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t bq25882_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct bq25882_sysfs_field_info *info;
	int ret;
	u8 v;

	info = bq25882_sysfs_field_lookup(attr->attr.name);
	if (!info) {
		hwlog_err("error: get sysfs entries failed!\n");
		return -EINVAL;
	}

	ret = kstrtou8(buf, 0, &v);
	if (ret < 0) {
		hwlog_err("error: get kstrtou8 failed!\n");
		return ret;
	}

	ret = bq25882_write_mask(info->reg, info->mask, info->shift, v);
	if (ret) {
		return ret;
	}

	return count;
}

/**********************************************************
*  Function:       bq25882_sysfs_create_group
*  Discription:    create the bq25882 device sysfs group
*  Parameters:   di:bq25882_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_sysfs_create_group(struct bq25882_device_info *di)
{
	bq25882_sysfs_init_attrs();

	return sysfs_create_group(&di->dev->kobj, &bq25882_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Discription:    remove the bq25882 device sysfs group
*  Parameters:   di:bq25882_device_info
*  return value:  NULL
**********************************************************/
static void bq25882_sysfs_remove_group(struct bq25882_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &bq25882_sysfs_attr_group);
}

#else

static int bq25882_sysfs_create_group(struct bq25882_device_info *di)
{
	return 0;
}

static inline void bq25882_sysfs_remove_group(struct bq25882_device_info *di)
{
}

#endif

static int bq25882_5v_chip_init(struct bq25882_device_info *di)
{
	int ret = 0;

	/* Charge Voltage Limit 7.8V */
	ret |= bq25882_write_byte(BQ25882_REG_BVRL, 0x64);

	/* Fast Charge Current Limit 1000mA */
	ret |= bq25882_write_byte(BQ25882_REG_CCL, 0x54);

	/* iprechg = 150ma,iterm current = 150ma */
	ret |= bq25882_write_byte(BQ25882_REG_PTCL, 0x22);

	/* I2C watchdog timer setting = 80s, fast charge timer setting = 12h, EN_TERM 0, EN_TIMER 1 */
	ret |= bq25882_write_byte(BQ25882_REG_CC1, 0x2d);

	/* Battery Precharge to Fast Charge Threshold 6, Battery Recharge 200mV */
	ret |= bq25882_write_byte(BQ25882_REG_CC2, 0x7d);

	/* WD_RST 1, SYS_MIN 7.0V */
	ret |= bq25882_write_byte(BQ25882_REG_CC3, 0x4a);

	/* JEITA Low Temperature Current Setting 20% of ICHG */
	ret |= bq25882_write_byte(BQ25882_REG_CC4, 0x7d);

	/* buck mode current limit = 2A, OTG_VLIM 5.1V */
	ret |= bq25882_write_byte(BQ25882_REG_OC, 0xf6);

	/* ADC_RATE: one-shot conversion, ADC_SAMPLE: 14-bit effective resolution */
	ret |= bq25882_write_byte(BQ25882_REG_AC, 0x50);

	/* enable charging */
	gpio_set_value(di->gpio_cd, 0);

	return ret;

}

/**********************************************************
*  Function:       bq25882_chip_init
*  Discription:    bq25882 chipIC initialization
*  Parameters:   struct chip_init_crit* init_crit
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_chip_init(struct chip_init_crit* init_crit)
{
	int ret = -1;
	struct bq25882_device_info *di = g_bq25882_dev;

	if (NULL == di || NULL == init_crit) {
		hwlog_err("error: di or init_crit is null!\n");
		return -ENOMEM;
	}

	switch(init_crit->vbus) {
		case ADAPTER_5V:
			ret = bq25882_5v_chip_init(di);
		break;

		default:
			hwlog_err("error: invaid init_crit vbus mode!\n");
		break;
	}

	return ret;
}

/**********************************************************
*  Function:       bq25882_device_check
*  Discription:    check chip i2c communication
*  Parameters:   null
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_device_check(void)
{
	int ret = 0;
	u8 reg = 0xff;

	ret |= bq25882_read_byte(BQ25882_REG_PI, &reg);
	if (ret) {
		hwlog_err("error: device_check read fail!\n");
		return CHARGE_IC_BAD;
	}

	if ((BQ25882 == (reg & BQ25882_REG_PI_PN_MASK))
		&& (CHIP_REVISION == (reg & BQ25882_REG_PI_DEV_REV_MASK))) {
		hwlog_info("bq25882 is good\n");
		return CHARGE_IC_GOOD;
	}
	else {
		hwlog_err("error: bq25882 is bad!\n");
		return CHARGE_IC_BAD;
	}
}

/**********************************************************
*  Function:       bq25882_set_adc_conv_rate
*  Discription:    set adc conversion rate
*  Parameters:   mode:0-One shot ADC conversion
*                              1-Start 1s Continuous Conversion
*  return value:  0-sucess or others-fail
**********************************************************/

static int bq25882_set_adc_conv_rate(int mode)
{
	/* mode: 0-continuous conversion, 1-one-shot conversion */
	mode = !mode;

	hwlog_info("set_adc_conv_rate [%x]=0x%x\n", BQ25882_REG_AC, mode);

	return bq25882_write_mask(BQ25882_REG_AC,
				BQ25882_REG_AC_ADC_RATE_MASK,
				BQ25882_REG_AC_ADC_RATE_SHIFT, mode);
}

/**********************************************************
*  Function:       bq25882_set_covn_start
*  Discription:    set covn start
*  Parameters:   value: 1(enable) or 0(disable)
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_covn_start(int enable)
{
	int ret = 0;
	u8 reg = 0;
	int i = 0;

	ret = bq25882_read_byte(BQ25882_REG_CS2, &reg);
	if (ret) {
		hwlog_err("error: set_covn_start read fail!\n");
		return -1;
	}

	if (!(reg & BQ25882_REG_CS2_PG_STAT_MASK)) {
		hwlog_err("error: bq25882 PG NOT GOOD, can not set covn,reg:%x!\n", reg);
		return -1;
	}

	ret = bq25882_write_mask(BQ25882_REG_AC,
				BQ25882_REG_AC_ADC_EN_MASK,
				BQ25882_REG_AC_ADC_EN_SHIFT, enable);
	if (ret) {
		hwlog_err("error: set_covn_start write fail!\n");
		return -1;
	}

	/* The conversion result is ready after tCONV, max (3*8)ms */
	for (i = 0; i < CONV_RETRY; i++) {
		ret = bq25882_read_byte(BQ25882_REG_CS1, &reg);
		if (ret) {
			hwlog_err("error: set_covn_start read fail!\n");
			return -1;
		}

		/* if ADC Conversion finished, ADC_DONE_STAT bit will be 1 */
		if (reg & BQ25882_REG_CS1_ADC_DONE_STAT_MASK) {
			break;
		}

		msleep(3); /* sleep 3ms */
	}

	hwlog_info("set_covn_start one-shot covn start is set %d\n", enable);

	return 0;
}

/**********************************************************
*  Function:       bq25882_set_input_current
*  Discription:    set the input current in charging process
*  Parameters:   value:input current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_input_current(int value)
{
	unsigned int limit_current = 0;
	u8 Iin_limit = 0;

	limit_current = value;

	if (limit_current < IINLIM_MIN) {
		limit_current = IINLIM_MIN;
	}
	else if (limit_current > IINLIM_MAX) {
		limit_current = IINLIM_MAX;
	}
	else {
		/* do nothing */
	}

	Iin_limit = (limit_current - IINLIM_OFFSET) / IINLIM_STEP;
	iin_set = Iin_limit * IINLIM_STEP + IINLIM_OFFSET;

	hwlog_info("set_input_current [%x]=0x%x\n", BQ25882_REG_ICL, Iin_limit);

	return bq25882_write_mask(BQ25882_REG_ICL,
				BQ25882_REG_ICL_IINDPM_MASK,
				BQ25882_REG_ICL_IINDPM_SHIFT, Iin_limit);
}

static int bq25882_get_input_current_set(void)
{
	return iin_set;
}

/**********************************************************
*  Function:       bq25882_set_charge_current
*  Discription:    set the charge current in charging process
*  Parameters:   value:charge current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_charge_current(int value)
{
	int currentmA = 0;
	u8 ichg = 0;

	currentmA = value;

	if (currentmA < ICHG_MIN) {
		currentmA = ICHG_MIN;
	}
	else if (currentmA > ICHG_MAX) {
		currentmA = ICHG_MAX;
	}
	else {
		/* do nothing */
	}

	ichg = currentmA / ICHG_STEP;

	hwlog_info("set_charge_current [%x]=0x%x\n", BQ25882_REG_CCL, ichg);

	return bq25882_write_mask(BQ25882_REG_CCL,
				BQ25882_REG_CCL_ICHG_MASK,
				BQ25882_REG_CCL_ICHG_SHIFT, ichg);
}

/**********************************************************
*  Function:       bq25882_set_terminal_voltage
*  Discription:    set the terminal voltage in charging process
*  Parameters:   value:terminal voltage value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_terminal_voltage(int value)
{
	unsigned int voltagemV = 0;
	u8 Voreg = 0;

	voltagemV = value * 2;

	if (voltagemV < VCHARGE_MIN) {
		voltagemV = VCHARGE_MIN;
	}
	else if (voltagemV > VCHARGE_MAX) {
		voltagemV = VCHARGE_MAX;
	}
	else {
		/* do nothing */
	}

	Voreg = (voltagemV - VCHARGE_MIN) / VCHARGE_STEP;

	hwlog_info("set_terminal_voltage [%x]=0x%x\n", BQ25882_REG_BVRL, Voreg);

	return bq25882_write_byte(BQ25882_REG_BVRL, Voreg);
}

/**********************************************************
*  Function:       bq25882_set_dpm_voltage
*  Discription:    set the dpm voltage in charging process
*  Parameters:   value:dpm voltage value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_dpm_voltage(int value)
{
	int vindpm_voltage;
	u8 vindpm;

	vindpm_voltage = value;

	if (vindpm_voltage > VINDPM_MAX) {
		vindpm_voltage = VINDPM_MAX;
	}
	else if (vindpm_voltage < VINDPM_MIN) {
		vindpm_voltage = VINDPM_MIN;
	}
	else {
		/* do nothing */
	}

	vindpm = (vindpm_voltage - VINDPM_OFFSET) / VINDPM_STEP;

	hwlog_info("set_dpm_voltage [%x]=0x%x\n", BQ25882_REG_IVL, vindpm);

	return bq25882_write_mask(BQ25882_REG_IVL,
				BQ25882_REG_IVL_VINDPM_MASK,
				BQ25882_REG_IVL_VINDPM_SHIFT, vindpm);
}

/**********************************************************
*  Function:       bq25882_set_terminal_current
*  Discription:    set the terminal current in charging process
*  Parameters:   value:terminal current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_terminal_current(int value)
{
	unsigned int term_currentmA = 0;
	u8 iterm_reg = 0;

	term_currentmA = value;

	if (term_currentmA < ITERM_MIN) {
		term_currentmA = ITERM_MIN;
	}
	else if (term_currentmA > ITERM_MAX) {
		term_currentmA = ITERM_MAX;
	}
	else {
		/* do nothing */
	}

	iterm_reg = (term_currentmA - ITERM_OFFSET) / ITERM_STEP;

	hwlog_info("set_terminal_current [%x]=0x%x\n", BQ25882_REG_PTCL, iterm_reg);

	return bq25882_write_mask(BQ25882_REG_PTCL,
				BQ25882_REG_PTCL_ITERM_MASK,
				BQ25882_REG_PTCL_ITERM_SHIFT, iterm_reg);
}

/**********************************************************
*  Function:       bq25882_set_charge_enable
*  Discription:    set the charge enable in charging process
*  Parameters:   enable:charge enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_charge_enable(int enable)
{
	struct bq25882_device_info *di = g_bq25882_dev;

	gpio_set_value(di->gpio_cd, !enable);

	return bq25882_write_mask(BQ25882_REG_CC2,
				BQ25882_REG_CC2_EN_CHG_MASK,
				BQ25882_REG_CC2_EN_CHG_SHIFT, enable);
}

/**********************************************************
*  Function:       bq25882_set_otg_enable
*  Discription:    set the otg mode enable in charging process
*  Parameters:   enable:otg mode  enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_otg_enable(int enable)
{
	struct bq25882_device_info *di = g_bq25882_dev;

	gpio_set_value(di->gpio_cd, !enable);

	/* NOTICE:
	   why enable irq when entry to OTG mode only?
	   because we care VBUS overloaded OCP or OVP's interrupt in buck mode
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

	return bq25882_write_mask(BQ25882_REG_CC2,
				BQ25882_REG_CC2_EN_OTG_MASK,
				BQ25882_REG_CC2_EN_OTG_SHIFT, enable);
}

/**********************************************************
*  Function:       bq25882_set_term_enable
*  Discription:    set the terminal enable in charging process
*  Parameters:   enable:terminal enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_term_enable(int enable)
{
	return bq25882_write_mask(BQ25882_REG_CC1,
				BQ25882_REG_CC1_EN_TERM_MASK,
				BQ25882_REG_CC1_EN_TERM_SHIFT, enable);
}

/**********************************************************
*  Function:       bq25882_get_charge_state
*  Discription:    get the charge states in charging process
*  Parameters:   state:charge states
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_get_charge_state(unsigned int *state)
{
	u8 reg = 0;
	int ret = 0;

	ret = bq25882_read_byte(BQ25882_REG_CS2, &reg);

	hwlog_info("get_charge_state [%x]=0x%x\n", BQ25882_REG_CS2, reg);

	if (!(reg & BQ25882_NOT_PG_STAT)) {
		*state |= CHAGRE_STATE_NOT_PG;
	}

	ret |= bq25882_read_byte(BQ25882_REG_CS1, &reg);

	hwlog_info("get_charge_state [%x]=0x%x\n", BQ25882_REG_CS1, reg);

	if ((reg & BQ25882_REG_CS1_CHRG_STAT_MASK) == BQ25882_CHGR_STAT_CHARGE_DONE) {
		*state |= CHAGRE_STATE_CHRG_DONE;
	}

	if (reg & BQ25882_WATCHDOG_FAULT) {
		*state |= CHAGRE_STATE_WDT_FAULT;
	}

	ret |= bq25882_read_byte(BQ25882_REG_NS, &reg);

	hwlog_info("get_charge_state [%x]=0x%x\n", BQ25882_REG_NS, reg);

	if (reg & BQ25882_REG_NS_TS_STAT_MASK) {
		*state |= CHAGRE_STATE_NTC_FAULT;
	}

	ret |= bq25882_read_byte(BQ25882_REG_FS, &reg);

	hwlog_info("get_charge_state [%x]=0x%x\n", BQ25882_REG_FS, reg);

	if (reg & BQ25882_REG_FS_VBUS_OVP_STAT_MASK) {
		*state |= CHAGRE_STATE_VBUS_OVP;
	}

	if (reg & BQ25882_REG_FS_BATOVP_STAT_MASK) {
		*state |= CHAGRE_STATE_BATT_OVP;
	}

	return ret;
}

/**********************************************************
*  Function:       bq25882_reset_watchdog_timer
*  Discription:    reset watchdog timer in charging process
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_reset_watchdog_timer(void)
{
	return bq25882_write_mask(BQ25882_REG_CC3,
				BQ25882_REG_CC3_WD_RST_MASK,
				BQ25882_REG_CC3_WD_RST_SHIFT, TRUE);
}

/**********************************************************
*  Function:       bq25882_get_ibus_ma
*  Discription:    get ibus
*  Parameters:   NULL
*  return value:  ibus
**********************************************************/
static int bq25882_get_ibus_ma(void)
{
	u16 reg_adc = 0;
	u8 reg_adc1 = 0;
	u8 reg_adc0 = 0;
	int ret = 0;
	int ibus_ma = 0;

	ret = bq25882_read_byte(BQ25882_REG_IBUS_ADC1, &reg_adc1);
	ret |= bq25882_read_byte(BQ25882_REG_IBUS_ADC0, &reg_adc0);

	reg_adc = (reg_adc1 << 8) | reg_adc0;
	ibus_ma = (int)reg_adc * BQ25882_REG_IBUS_ADC_STEP_MA;

	hwlog_info("get_ibus_ma ibus is %dmA\n", ibus_ma);

	return ibus_ma;
}

/**********************************************************
*  Function:       bq25882_get_vbus_mv
*  Discription:    get voltage of vbus
*  Parameters:   vbus_mv:voltage of vbus
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_get_vbus_mv(unsigned int *vbus_mv)
{
	u16 reg_adc = 0;
	u8 reg_adc1 = 0;
	u8 reg_adc0 = 0;
	int ret = 0;

	ret = bq25882_read_byte(BQ25882_REG_VBUS_ADC1, &reg_adc1);
	ret |= bq25882_read_byte(BQ25882_REG_VBUS_ADC0, &reg_adc0);

	reg_adc = (reg_adc1 << 8) | reg_adc0;
	*vbus_mv = (int)reg_adc * BQ25882_REG_VBUS_ADC_STEP_MV;

	hwlog_info("get_vbus_mv vbus is %dmV\n", *vbus_mv);

	return ret;
}

/**********************************************************
*  Function:       bq25882_get_vbat_mv
*  Discription:    get voltage of vbat
*  Parameters:   vbat_mv:voltage of vbat
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_get_vbat_mv(void)
{
	u16 reg_adc = 0;
	u8 reg_adc1 = 0;
	u8 reg_adc0 = 0;
	int ret = 0;
	int vbat_mv;

	ret = bq25882_read_byte(BQ25882_REG_VBAT_ADC1, &reg_adc1);
	ret |= bq25882_read_byte(BQ25882_REG_VBAT_ADC0, &reg_adc0);

	if (ret < 0) {
		hwlog_err("error: get_vbat_mv read fail!\n");
		return -1;
	}

	reg_adc = (reg_adc1 << 8) | reg_adc0;
	vbat_mv = (int)reg_adc * BQ25882_REG_VBAT_ADC_STEP_MV;

	hwlog_info("get_vbat_mv vbat is %dmV\n", vbat_mv);

	return vbat_mv;
}

/**********************************************************
*  Function:       bq25882_check_input_dpm_state
*  Discription:    check whether VINDPM or IINDPM
*  Parameters:     NULL
*  return value:   TRUE means VINDPM or IINDPM
*                  FALSE means NoT DPM
**********************************************************/
static int bq25882_check_input_dpm_state(void)
{
	u8 reg = 0;
	int ret = -1;

	ret = bq25882_read_byte(BQ25882_REG_CS1, &reg);
	if (ret < 0) {
		hwlog_err("error: check_input_dpm_state read fail!\n");
		return ret;
	}

	hwlog_info("check_input_dpm_state [%x]=0x%x\n", BQ25882_REG_CS1, reg);

	if ((reg & BQ25882_REG_CS1_IINDPM_STAT_MASK)
		||(reg & BQ25882_REG_CS1_VINDPM_STAT_MASK)) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

/**********************************************************
*  Function:       bq25882_set_otg_current
*  Discription:    set the otg mdoe current
*  Parameters:     value :current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_otg_current(int value)
{
	unsigned int otg_currentmA = 0;
	u8 iotg_reg = 0;
	otg_currentmA = value;

	if (otg_currentmA < OTG_ILIM_MIN) {
		otg_currentmA = OTG_ILIM_MIN;
	}
	else if (otg_currentmA > OTG_ILIM_MAX) {
		otg_currentmA = OTG_ILIM_MAX;
	}
	else {
		/* do nothing */
	}

	iotg_reg = (otg_currentmA - ITERM_OFFSET) / ITERM_STEP;

	hwlog_info("check_input_dpm_state [%x]=0x%x\n", BQ25882_REG_OC, iotg_reg);

	return bq25882_write_mask(BQ25882_REG_OC,
				BQ25882_REG_OC_OTG_ILIM_MASK,
				BQ25882_REG_OC_OTG_ILIM_SHIFT, iotg_reg);
}

/**********************************************************
*  Function:       bq25882_dump_register
*  Discription:    print the register value in charging process
*  Parameters:   reg_value:string for save register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_dump_register(char *reg_value)
{
	u8 reg[BQ25882_REG_NUM] = { 0 };
	char buff[BUF_LEN] = { 0 };
	int i = 0;

	memset(reg_value, 0, CHARGELOG_SIZE);

	snprintf(buff, BUF_LEN, "%-8.2d", bq25882_get_ibus_ma());
	strncat(reg_value, buff, strlen(buff));

	for (i = 0; i < BQ25882_REG_NUM; i++) {
		bq25882_read_byte(i, &reg[i]);
		snprintf(buff, BUF_LEN, "0x%-8.2x", reg[i]);
		strncat(reg_value, buff, strlen(buff));
	}

	return 0;
}

/**********************************************************
*  Function:       bq25882_dump_register
*  Discription:    print the register head in charging process
*  Parameters:   reg_head:string for save register head
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_get_register_head(char *reg_head)
{
	char buff[BUF_LEN] = { 0 };
	int i = 0;

	memset(reg_head, 0, CHARGELOG_SIZE);

	snprintf(buff, BUF_LEN, "Ibus    ");
	strncat(reg_head, buff, strlen(buff));

	for (i = 0; i < BQ25882_REG_NUM; i++) {
		snprintf(buff, BUF_LEN, "Reg[%d]    ", i);
		strncat(reg_head, buff, strlen(buff));
	}

	return 0;
}

/**********************************************************
*  Function:       bq25882_set_batfet_disable
*  Discription:    set the batfet disable in charging process
*  Parameters:   disable:batfet disable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_batfet_disable(int disable)
{
	return bq25882_set_charge_enable(!disable);
}

/**********************************************************
*  Function:       bq25882_set_watchdog_timer
*  Discription:    set the watchdog timer in charging process
*  Parameters:   value:watchdog timer value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_watchdog_timer(int value)
{
	u8 val = 0;
	u8 dog_time = value;

	if (dog_time >= WATCHDOG_TIMER_160_S) {
		val = BQ25882_REG_CC1_WATCHDOG_160;
	}
	else if (dog_time >= WATCHDOG_TIMER_80_S) {
		val = BQ25882_REG_CC1_WATCHDOG_80;
	}
	else if (dog_time >= WATCHDOG_TIMER_40_S) {
		val = BQ25882_REG_CC1_WATCHDOG_40;
	}
	else {
		val = BQ25882_REG_CC1_WATCHDOG_DIS;
	}

	hwlog_info("set_watchdog_timer [%x]=0x%x\n", BQ25882_REG_CC1, val);

	return bq25882_write_mask(BQ25882_REG_CC1,
				BQ25882_REG_CC1_WATCHDOG_MASK,
				BQ25882_REG_CC1_WATCHDOG_SHIFT, val);
}

/**********************************************************
*  Function:       bq25882_set_charger_hiz
*  Discription:    set the charger hiz close watchdog
*  Parameters:   enable:charger in hiz or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_set_charger_hiz(int enable)
{
	int ret = 0;

	if (enable > 0) {
		ret |= bq25882_write_mask(BQ25882_REG_CCL,
				BQ25882_REG_CCL_EN_HIZ_MASK,
				BQ25882_REG_CCL_EN_HIZ_SHIFT, TRUE);
	}
	else {
		ret |= bq25882_write_mask(BQ25882_REG_CCL,
				BQ25882_REG_CCL_EN_HIZ_MASK,
				BQ25882_REG_CCL_EN_HIZ_SHIFT, FALSE);
	}

	return ret;
}

static int bq25882_force_set_term_enable(int enable)
{
	return 0;
}

struct charge_device_ops bq25882_ops = {
	.chip_init                = bq25882_chip_init,
	.dev_check                = bq25882_device_check,
	.set_adc_conv_rate        = bq25882_set_adc_conv_rate,
	.set_input_current        = bq25882_set_input_current,
	.set_charge_current       = bq25882_set_charge_current,
	.set_terminal_voltage     = bq25882_set_terminal_voltage,
	.set_dpm_voltage          = bq25882_set_dpm_voltage,
	.set_terminal_current     = bq25882_set_terminal_current,
	.set_charge_enable        = bq25882_set_charge_enable,
	.set_otg_enable           = bq25882_set_otg_enable,
	.set_term_enable          = bq25882_set_term_enable,
	.get_charge_state         = bq25882_get_charge_state,
	.reset_watchdog_timer     = bq25882_reset_watchdog_timer,
	.dump_register            = bq25882_dump_register,
	.get_register_head        = bq25882_get_register_head,
	.set_watchdog_timer       = bq25882_set_watchdog_timer,
	.set_batfet_disable       = bq25882_set_batfet_disable,
	.get_ibus                 = bq25882_get_ibus_ma,
	.get_vbus                 = bq25882_get_vbus_mv,
	.set_covn_start           = bq25882_set_covn_start,
	.check_input_dpm_state    = bq25882_check_input_dpm_state,
	.set_otg_current          = bq25882_set_otg_current,
	.set_force_term_enable    = bq25882_force_set_term_enable,
	.set_charger_hiz          = bq25882_set_charger_hiz,
	.get_iin_set              = bq25882_get_input_current_set,
	.soft_vbatt_ovp_protect   = NULL,
	.rboost_buck_limit        = NULL,
	.get_charge_current       = NULL,
	.turn_on_ico              = NULL,
};

struct hw_batt_vol_ops bq25882_batt_vol_ops = {
	.get_batt_vol        = bq25882_get_vbat_mv,
};

/**********************************************************
*  Function:       bq25882_irq_work
*  Discription:    handler for chargerIC fault irq in charging process
*  Parameters:   work:chargerIC fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void bq25882_irq_work(struct work_struct *work)
{
	struct bq25882_device_info *di = container_of(work, struct bq25882_device_info, irq_work);
	u8 reg = 0, reg1 = 0;

	msleep(100); /* sleep 100ms */

	bq25882_read_byte(BQ25882_REG_CS1, &reg1);
	bq25882_read_byte(BQ25882_REG_FF, &reg);

	hwlog_info("reg[%x]:0x%x, reg[%x]:0x%0x\n", BQ25882_REG_CS1, reg1, BQ25882_REG_FF, reg);

	if (reg & BQ25882_REG_FF_OTG_FLAG_MASK) {
		hwlog_info("CHARGE_FAULT_BOOST_OCP happened\n");
		atomic_notifier_call_chain(&fault_notifier_list, CHARGE_FAULT_BOOST_OCP, NULL);
	}

	if (di->irq_active == 0) {
		di->irq_active = 1;
		enable_irq(di->irq_int);
	}
}

/**********************************************************
*  Function:       bq25882_interrupt
*  Discription:    callback function for chargerIC fault irq in charging process
*  Parameters:   irq:chargerIC fault interrupt
*                      _di:bq25882_device_info
*  return value:  IRQ_HANDLED-sucess or others
**********************************************************/
static irqreturn_t bq25882_interrupt(int irq, void *_di)
{
	struct bq25882_device_info *di = _di;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	hwlog_info("bq25882 interrupt happened (%d)!\n", di->irq_active);

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
*  Function:       bq25882_probe
*  Discription:    bq25882 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret = 0;
	struct bq25882_device_info *di = NULL;
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
	g_bq25882_dev = di;

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);

	INIT_WORK(&di->irq_work, bq25882_irq_work);

	di->gpio_cd = of_get_named_gpio(np, "gpio_cd", 0);
	hwlog_info("gpio_cd=%d\n", di->gpio_cd);

	if (!gpio_is_valid(di->gpio_cd)) {
		hwlog_err("error: gpio(gpio_cd) is not valid!\n");
		ret = -EINVAL;
		goto bq25882_fail_0;
	}

	ret = gpio_request(di->gpio_cd, "charger_cd");
	if (ret) {
		hwlog_err("error: gpio(gpio_cd) request fail!\n");
		goto bq25882_fail_0;
	}

	/* set gpio to control CD pin to disable/enable bq2560x IC */
	ret = gpio_direction_output(di->gpio_cd, 0);
	if (ret) {
		hwlog_err("error: gpio(gpio_cd) set output fail!\n");
		goto bq25882_fail_0;
	}

	di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
	hwlog_info("gpio_int=%d\n", di->gpio_int);

	if (!gpio_is_valid(di->gpio_int)) {
		hwlog_err("error: gpio(gpio_int) is not valid!\n");
		ret = -EINVAL;
		goto bq25882_fail_1;
	}

	ret = gpio_request(di->gpio_int, "charger_int");
	if (ret) {
		hwlog_err("error: gpio(gpio_int) request fail!\n");
		goto bq25882_fail_1;
	}

	ret = gpio_direction_input(di->gpio_int);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) set input fail!\n");
		goto bq25882_fail_1;
	}

	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0) {
		hwlog_err("error: gpio(gpio_int) map to irq fail!\n");
		ret = -EINVAL;
		goto bq25882_fail_2;
	}

	ret = request_irq(di->irq_int, bq25882_interrupt, IRQF_TRIGGER_FALLING, "charger_int_irq", di);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) irq request fail!\n");
		di->irq_int = -1;
		goto bq25882_fail_2;
	}

	disable_irq(di->irq_int);
	di->irq_active = 0;

	ret = charge_ops_register(&bq25882_ops);
	if (ret) {
		hwlog_err("error: bq25882 charge ops register fail!\n");
		goto bq25882_fail_3;
	}

	ret = hw_battery_voltage_ops_register(&bq25882_batt_vol_ops, "bq25882");
	if (ret) {
		hwlog_err("error: bq25882 hw_battery_voltage ops register failed!\n");
	}

	ret = bq25882_sysfs_create_group(di);
	if (ret) {
		hwlog_err("error: sysfs group create failed!\n");
	}

	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL) {
			charge_dev = device_create(power_class, NULL, 0, NULL, "charger");
		}

		ret = sysfs_create_link(&charge_dev->kobj, &di->dev->kobj, "bq25882");
		if (ret) {
			hwlog_err("error: sysfs link create failed!\n");
			goto bq25882_fail_4;
		}
	}

	hwlog_info("probe end!\n");
	return 0;

bq25882_fail_4:
	bq25882_sysfs_remove_group(di);
bq25882_fail_3:
	free_irq(di->irq_int, di);
bq25882_fail_2:
	gpio_free(di->gpio_int);
bq25882_fail_1:
	gpio_free(di->gpio_cd);
bq25882_fail_0:
	kfree(di);
	g_bq25882_dev = NULL;
	np = NULL;

	return ret;
}

/**********************************************************
*  Function:       bq25882_remove
*  Discription:    bq25882 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25882_remove(struct i2c_client *client)
{
	struct bq25882_device_info *di = i2c_get_clientdata(client);

	hwlog_info("remove begin\n");

	bq25882_sysfs_remove_group(di);

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

MODULE_DEVICE_TABLE(i2c, bq25882);
static struct of_device_id bq25882_of_match[] = {
	{
		.compatible = "huawei,bq25882_charger",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id bq25882_i2c_id[] = {
	{"bq25882_charger", 0}, {}
};

static struct i2c_driver bq25882_driver = {
	.probe = bq25882_probe,
	.remove = bq25882_remove,
	.id_table = bq25882_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "bq25882_charger",
		.of_match_table = of_match_ptr(bq25882_of_match),
	},
};

/**********************************************************
*  Function:       bq25882_init
*  Discription:    bq25882 module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init bq25882_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&bq25882_driver);
	if (ret) {
		hwlog_err("error: bq25882 i2c_add_driver error!\n");
	}

	return ret;
}

/**********************************************************
*  Function:       bq25882_exit
*  Discription:    bq25882 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit bq25882_exit(void)
{
	i2c_del_driver(&bq25882_driver);
}

module_init(bq25882_init);
module_exit(bq25882_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("bq25882 charger module driver");
MODULE_AUTHOR("HW Inc");
