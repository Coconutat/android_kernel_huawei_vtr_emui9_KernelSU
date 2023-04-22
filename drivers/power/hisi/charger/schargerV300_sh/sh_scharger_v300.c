

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
#include <linux/power/hisi/hisi_bci_battery.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include "../schargerV300/hisi_scharger_v300.h"
#include <linux/raid/pq.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/hisi/hisi_adc.h>
#include <huawei_platform/power/huawei_charger_sh.h>
#include <inputhub_route.h>
#include <protocol.h>
#include <sensor_info.h>
static unsigned int is_board_type = 2;	/*0:sft 1:udp 2:asic */
extern struct charger_platform_data charger_dts_data;
#define HI6523_FCP_REG_TOTAL_NUM 88
/**********************************************************
*  Function:       hi6523_read_block
*  Discription:    register read block interface
*  Parameters:   reg:register name
*                      value:register value
*			len: bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_read_block(u8 reg, u8*value, unsigned len)
{
	int ret = mcu_i2c_rw(3, HI6523_I2C_SLAVE_ADDRESS, &reg, 1, value, len);
	if (ret < 0) {
		hwlog_err("hi6523 read register %d fail!\n", reg);
	} else {
		hwlog_info("hi6523 read value %d from register %d success!\n", *value, reg);
	}
	return ret;
}

/**********************************************************
*  Function:       hi6523_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_write_byte(uint8_t reg, uint8_t value)
{
	uint8_t tx_buf[2];
	int ret;
	tx_buf[0] = reg;
	tx_buf[1] = value;
	ret = mcu_i2c_rw(3, HI6523_I2C_SLAVE_ADDRESS, tx_buf, 2, NULL, 0);
	if (ret < 0) {
		hwlog_err("write value %d to register %d fail!\n", value, reg);
	} else {
		hwlog_info("hi6523 write value %d to register %d success!\n", value, reg);
	}
	return ret;
}

/**********************************************************
*  Function:       hi6523_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_read_byte(uint8_t reg, uint8_t * value)
{
	return hi6523_read_block(reg, value, 1);
}

/**********************************************************
*  Function:       hi6523_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = hi6523_read_byte(reg, &val);
	if (ret < 0)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = hi6523_write_byte(reg, val);

	return ret;
}

/**********************************************************
*  Function:       hi6523_read_mask
*  Discription:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_read_mask(u8 reg, u8 MASK, u8 SHIFT, u8 * value)
{
	int ret = 0;
	u8 val = 0;

	ret = hi6523_read_byte(reg, &val);
	if (ret < 0)
		return ret;
	val &= MASK;
	val >>= SHIFT;
	*value = val;

	return 0;
}

#define CONFIG_SYSFS_SCHG
#ifdef CONFIG_SYSFS_SCHG
/*
 * There are a numerous options that are configurable on the HI6523
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */

#define HI6523_SYSFS_FIELD(_name, r, f, m, store)                  \
{                                                   \
    .attr = __ATTR(_name, m, hi6523_sysfs_show, store),    \
    .reg = CHG_##r##_REG,                      \
    .mask = CHG_##f##_MSK,                       \
    .shift = CHG_##f##_SHIFT,                     \
}

#define HI6523_SYSFS_FIELD_RW(_name, r, f)                     \
        HI6523_SYSFS_FIELD(_name, r, f, S_IWUSR | S_IRUGO, \
                hi6523_sysfs_store)

static ssize_t hi6523_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t hi6523_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

struct hi6523_sysfs_field_info {
	struct device_attribute attr;
	u8 reg;
	u8 mask;
	u8 shift;
};

static struct hi6523_sysfs_field_info hi6523_sysfs_field_tbl[] = {
	/* sysfs name reg field in reg */
	HI6523_SYSFS_FIELD_RW(en_hiz, HIZ_CTRL, HIZ_ENABLE),
	HI6523_SYSFS_FIELD_RW(iinlim, INPUT_SOURCE, ILIMIT),
	HI6523_SYSFS_FIELD_RW(reg_addr, NONE, NONE),
	HI6523_SYSFS_FIELD_RW(reg_value, NONE, NONE),
};

static struct attribute *hi6523_sysfs_attrs[ARRAY_SIZE(hi6523_sysfs_field_tbl) +
					    1];

static const struct attribute_group hi6523_sysfs_attr_group = {
	.attrs = hi6523_sysfs_attrs,
};

/**********************************************************
*  Function:       hi6523_sysfs_init_attrs
*  Discription:    initialize hi6523_sysfs_attrs[] for HI6523 attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void hi6523_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(hi6523_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		hi6523_sysfs_attrs[i] = &hi6523_sysfs_field_tbl[i].attr.attr;

	hi6523_sysfs_attrs[limit] = NULL;	/* Has additional entry for this */
}

/**********************************************************
*  Function:       hi6523_sysfs_field_lookup
*  Discription:    get the current device_attribute from hi6523_sysfs_field_tbl by attr's name
*  Parameters:   name:evice attribute name
*  return value:  hi6523_sysfs_field_tbl[]
**********************************************************/
static struct hi6523_sysfs_field_info *hi6523_sysfs_field_lookup(const char
								 *name)
{
	int i, limit = ARRAY_SIZE(hi6523_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		if (!strncmp(name, hi6523_sysfs_field_tbl[i].attr.attr.name, strlen(name)))
			break;

	if (i >= limit)
		return NULL;

	return &hi6523_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       hi6523_sysfs_show
*  Discription:    show the value for all HI6523 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t hi6523_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct hi6523_sysfs_field_info *info;
	struct hi6523_sysfs_field_info *info2;
#ifdef CONFIG_HISI_DEBUG_FS
	int ret;
#endif
	u8 v;

	info = hi6523_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	if (!strncmp("reg_addr", attr->attr.name, strlen("reg_addr"))) {
		return scnprintf(buf, PAGE_SIZE, "0x%hhx\n", info->reg);
	}

	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = hi6523_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}
#ifdef CONFIG_HISI_DEBUG_FS
	ret = hi6523_read_mask(info->reg, info->mask, info->shift, &v);
	if (ret)
		return ret;
#endif

	return scnprintf(buf, PAGE_SIZE, "0x%hhx\n", v);
}

/**********************************************************
*  Function:       hi6523_sysfs_store
*  Discription:    set the value for all HI6523 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t hi6523_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct hi6523_sysfs_field_info *info;
	struct hi6523_sysfs_field_info *info2;
	int ret;
	u8 v;

	info = hi6523_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	ret = kstrtou8(buf, 0, &v);
	if (ret < 0)
		return ret;
	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = hi6523_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}
	if (!strncmp(("reg_addr"), attr->attr.name, strlen("reg_addr"))) {
		if (v < (u8) HI6523_REG_TOTAL_NUM) {
			info->reg = v;
			return count;
		} else {
			return -EINVAL;
		}
	}
#ifdef CONFIG_HISI_DEBUG_FS
	ret = hi6523_write_mask(info->reg, info->mask, info->shift, v);
	if (ret)
		return ret;
#endif

	return count;
}

/**********************************************************
*  Function:       hi6523_sysfs_create_group
*  Discription:    create the HI6523 device sysfs group
*  Parameters:   di:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_sysfs_create_group(struct device *dev)
{
	hi6523_sysfs_init_attrs();

	return sysfs_create_group(&dev->kobj, &hi6523_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Discription:    remove the HI6523 device sysfs group
*  Parameters:   di:platform_device
*  return value:  NULL
**********************************************************/
static void hi6523_sysfs_remove_group(struct device *dev)
{
	sysfs_remove_group(&dev->kobj, &hi6523_sysfs_attr_group);
}
#else
static int hi6523_sysfs_create_group(struct device *dev)
{
	return 0;
}

static inline void hi6523_sysfs_remove_group(struct device *dev)
{
}
#endif

/**********************************************************
*  Function:       hi6523_device_check
*  Discription:    check chip i2c communication
*  Parameters:   null
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_device_check(void)
{
	int ret = 0;
	u8 reg_chip_id0 = 0xff;

	ret = hi6523_read_byte(CHIP_VERSION_4, &reg_chip_id0);
	if (ret) {
		hwlog_err("[%s]:read chip_id0 fail\n", __FUNCTION__);
		return CHARGE_IC_BAD;
	}

	if (CHIP_ID0 == reg_chip_id0 || CHIP_ID1 == reg_chip_id0) {
		hwlog_info("hi6523 is good.\n");
		return CHARGE_IC_GOOD;
	}
	hwlog_err("hi6523 is bad.\n");
	return CHARGE_IC_BAD;
}

/**********************************************************
*  Function:     hi6523_set_batfet_ctrl()
*  Discription:  config batfet status 1:enable, 0: disable
*  Parameters:   status
*  return value:
*                 0-sucess or others-fail
**********************************************************/
static int hi6523_set_batfet_ctrl(u32 status)
{
	return hi6523_write_mask(BATFET_CTRL_CFG_REG, BATFET_CTRL_CFG_MSK,
				 BATFET_CTRL_CFG_SHIFT, status);
}

/**********************************************************
*  Function:       hi6523_get_vbus_mv
*  Description:    get voltage of vbus
*  Parameters:   vbus_mv:voltage of vbus
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_get_vbus_mv(unsigned int *vbus_mv)
{
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	uint8_t sub_cmd[2] = {0,};
	int ret = 0;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	sub_cmd[0] = CHARGE_GET_VBUS;
	pkg_ap.tag = TAG_CHARGER;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = sub_cmd;
	pkg_ap.wr_len = sizeof(sub_cmd);
	ret = write_customize_cmd(&pkg_ap,  &pkg_mcu);
	if(ret) {
		hwlog_err("get vbus from sensorhub fail, ret is %d\n", ret);
		return -1;
	} else {
		if(pkg_mcu.errno!=0) {
			hwlog_err("get vbus from sensorhub fail, errno is %d\n", pkg_mcu.errno);
			return -1;
		} else {
			memcpy(vbus_mv, pkg_mcu.data, sizeof(unsigned int));
			hwlog_info( "get vbus from sensorhub success\n");
			return 0;
		}
	}
}

/**********************************************************
*  Function:       hi6523_set_input_current
*  Discription:    set the input current in charging process
*  Parameters:   value:input current value
*  return value:  0-sucess or others-fail
**********************************************************/

static int hi6523_set_input_current(int cin_limit)
{
	u8 Iin_limit;

	if (cin_limit <= CHG_ILIMIT_100)
		Iin_limit = 0;
	else if (cin_limit <= CHG_ILIMIT_130)
		Iin_limit = 1;
	else if (cin_limit <= CHG_ILIMIT_470)
		Iin_limit = 2;
	else if (cin_limit <= CHG_ILIMIT_900)
		Iin_limit = 3;
	else if (cin_limit <= CHG_ILIMIT_1000)
		Iin_limit = 4;
	else if (cin_limit <= CHG_ILIMIT_1080)
		Iin_limit = 5;
	else if (cin_limit <= CHG_ILIMIT_1200)
		Iin_limit = 6;
	else if (cin_limit <= CHG_ILIMIT_1300)
		Iin_limit = 7;
	else if (cin_limit <= CHG_ILIMIT_1400)
		Iin_limit = 8;
	else if (cin_limit <= CHG_ILIMIT_1500)
		Iin_limit = 9;
	else if (cin_limit <= CHG_ILIMIT_1600)
		Iin_limit = 10;
	else if (cin_limit <= CHG_ILIMIT_1700)
		Iin_limit = 11;
	else if (cin_limit <= CHG_ILIMIT_1800)
		Iin_limit = 12;
	else if (cin_limit <= CHG_ILIMIT_1900)
		Iin_limit = 13;
	else if (cin_limit <= CHG_ILIMIT_2000)
		Iin_limit = 14;
	else if (cin_limit <= CHG_ILIMIT_2100)
		Iin_limit = 15;
	else if (cin_limit <= CHG_ILIMIT_2200)
		Iin_limit = 16;
	else if (cin_limit <= CHG_ILIMIT_2280)
		Iin_limit = 17;
	else if (cin_limit <= CHG_ILIMIT_2400)
		Iin_limit = 18;
	else if (cin_limit <= CHG_ILIMIT_2480)
		Iin_limit = 19;
	else if (cin_limit <= CHG_ILIMIT_2600)
		Iin_limit = 20;
	else if (cin_limit <= CHG_ILIMIT_2680)
		Iin_limit = 21;
	else if (cin_limit <= CHG_ILIMIT_2800)
		Iin_limit = 22;
	else if (cin_limit <= CHG_ILIMIT_2880)
		Iin_limit = 23;
	else if (cin_limit <= CHG_ILIMIT_3000)
		Iin_limit = 24;
	else if (cin_limit <= CHG_ILIMIT_3080)
		Iin_limit = 25;
	else if (cin_limit <= CHG_ILIMIT_3200)
		Iin_limit = 26;
	else
		Iin_limit = 26;

	hwlog_info("input current reg is set 0x%x\n", Iin_limit);
	return hi6523_write_mask(CHG_INPUT_SOURCE_REG, CHG_ILIMIT_MSK,
				 CHG_ILIMIT_SHIFT, Iin_limit);
}

/**********************************************************
*  Function:       hi6523_set_charge_current
*  Discription:    set the charge current in charging process
*  Parameters:   value:charge current value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_set_charge_current(int charge_current)
{
	u8 Ichg_limit;
	if (charge_current <= CHG_FAST_ICHG_100MA) {
		Ichg_limit = 0;
	} else if (charge_current > CHG_FAST_ICHG_100MA
		   && charge_current <= CHG_FAST_ICHG_1000MA) {
		Ichg_limit =
		    (charge_current -
		     CHG_FAST_ICHG_100MA) / CHG_FAST_ICHG_STEP_100;
	} else if (charge_current > CHG_FAST_ICHG_1000MA
		   && charge_current <= CHG_FAST_ICHG_2000MA) {
		Ichg_limit =
		    (charge_current -
		     CHG_FAST_ICHG_1000MA) / CHG_FAST_ICHG_STEP_200 + 9;
	} else if (charge_current > CHG_FAST_ICHG_2000MA
		   && charge_current <= CHG_FAST_ICHG_3000MA) {
		Ichg_limit =
		    (charge_current -
		     CHG_FAST_ICHG_2000MA) / CHG_FAST_ICHG_STEP_100 + 14;
	} else if (charge_current > CHG_FAST_ICHG_3000MA
		   && charge_current <= CHG_FAST_ICHG_4200MA) {
		Ichg_limit =
		    (charge_current -
		     CHG_FAST_ICHG_3000MA) / CHG_FAST_ICHG_STEP_200 + 24;
	} else if (charge_current > CHG_FAST_ICHG_4200MA
		   && charge_current < CHG_FAST_ICHG_4500MA) {
		Ichg_limit = 30;
	} else {
		Ichg_limit = 31;
	}
	hwlog_info("charge current reg is set 0x%x\n", Ichg_limit);
	return hi6523_write_mask(CHG_FAST_CURRENT_REG, CHG_FAST_ICHG_MSK,
				 CHG_FAST_ICHG_SHIFT, Ichg_limit);
}

/**********************************************************
*  Function:       hi6523_set_terminal_voltage
*  Discription:    set the terminal voltage in charging process
*  Parameters:   value:terminal voltage value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_set_terminal_voltage(int charge_voltage)
{
	u8 data;
	if (charge_voltage < CHG_FAST_VCHG_MIN) {
		charge_voltage = CHG_FAST_VCHG_MIN;
	} else if (charge_voltage > CHG_FAST_VCHG_MAX) {
		charge_voltage = CHG_FAST_VCHG_MAX;
	} else {
		//do nothing
	}
	data =
	    (u8) ((charge_voltage - CHG_FAST_VCHG_MIN) / CHG_FAST_VCHG_STEP_50);
	return hi6523_write_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
				 CHG_FAST_VCHG_SHIFT, data);
}

/**********************************************************
*  Function:       hi6523_set_charge_enable
*  Discription:    set the charge enable in charging process
*  Parameters:     enable:charge enable or not
*  return value:   0-sucess or others-fail
**********************************************************/
static int hi6523_set_charge_enable(int enable)
{
	/*invalidate charge enable on udp board */
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	uint8_t sub_cmd[2] = {0,};
	int ret = 0;

	if ((BAT_BOARD_UDP == is_board_type) && (CHG_ENABLE == enable))
		return 0;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	sub_cmd[0] = CHARGE_SET_CHARGE_ENABLE;
	sub_cmd[1] = enable;
	pkg_ap.tag = TAG_CHARGER;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = sub_cmd;
	pkg_ap.wr_len = sizeof(sub_cmd);
	ret = write_customize_cmd(&pkg_ap,  &pkg_mcu);
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
*  Function:       hi6523_set_otg_enable
*  Discription:    set the otg mode enable in charging process
*  Parameters:   enable:otg mode  enable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_set_otg_enable(int enable)
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
	ret = write_customize_cmd(&pkg_ap,  &pkg_mcu);
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
*  Function:       hi6523_get_ibus_ma
*  Discription:    get average value for ilim
*  Parameters:     NULL
*  return value:   average value for ilim
**********************************************************/
static int hi6523_get_ibus_ma(void)
{
	write_info_t	pkg_ap;
	read_info_t	pkg_mcu;
	uint8_t sub_cmd[2] = {0,};
	int ret = 0;

	memset(&pkg_ap, 0, sizeof(pkg_ap));
	memset(&pkg_mcu, 0, sizeof(pkg_mcu));

	sub_cmd[0] = CHARGE_GET_ILIM;
	pkg_ap.tag = TAG_CHARGER;
	pkg_ap.cmd = CMD_CMN_CONFIG_REQ;
	pkg_ap.wr_buf = sub_cmd;
	pkg_ap.wr_len = sizeof(sub_cmd);
	ret = write_customize_cmd(&pkg_ap,  &pkg_mcu);
	if(ret) {
		hwlog_err("get average value from sensorhub fail, ret is %d\n", ret);
		return -1;
	} else {
		if(pkg_mcu.errno!=0) {
			hwlog_err("get average value from sensorhub fail, errno is %d\n", pkg_mcu.errno);
			return -1;
		} else {
			memcpy(&ret, pkg_mcu.data, sizeof(int));
			hwlog_info( "get average value from sensorhub success\n");
			return ret;
		}
	}
}

/**********************************************************
*  Function:       hi6523_vbat_sys
*  Discription:    get vsys sample
*  Parameters:     NULL
*  return value:   vsys sample
**********************************************************/
static int hi6523_vbat_sys(void)
{
	int i = 0;
	int retry_times = 3;
	int V_sample = -1;

	if (charger_dts_data.adc_channel_vbat_sys < 0)
		return V_sample;

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

/**********************************************************
*  Function:       hi6523_get_vbat_sys
*  Discription:    get vsys voltage
*  Parameters:     NULL
*  return value:   vsys voltage
**********************************************************/
static int hi6523_get_vbat_sys(void)
{
	int i;
	int cnt = 0;
	int V_temp;
	int delay_times = 100;
	int sample_num = 5;	// use 5 samples to get an average value
	int sum = 0;

	for (i = 0; i < sample_num; ++i) {
		V_temp = hi6523_vbat_sys();
		if (V_temp >= 0) {
			sum += V_temp;
			++cnt;
		} else {
			hwlog_err("hi6523 get V_temp fail!\n");
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
*  Function:       hi6523_dump_register
*  Discription:    print the register value in charging process
*  Parameters:   reg_value:string for save register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_dump_register(char *reg_value)
{
	u8 reg[HI6523_REG_TOTAL_NUM] = { 0 };
	char buff[26] = { 0 };
	int i = 0;

	memset(reg_value, 0, CHARGELOG_SIZE);
	hi6523_read_block(0, &reg[0], 88);
	hi6523_read_block(88, &reg[88], 88);
	hi6523_read_block(176, &reg[176], 59);
	snprintf(buff, 26, "%-8.2d", hi6523_get_ibus_ma());
	strncat(reg_value, buff, strlen(buff));
	for (i = 0; i < HI6523_REG_TOTAL_NUM; i++) {
		snprintf(buff, 26, "0x%-8x", reg[i]);
		strncat(reg_value, buff, strlen(buff));
	}
	return 0;
}

/**********************************************************
*  Function:       hi6523_fcp_reg_dump
*  Discription:    print the register value about fcp
*  Parameters:   pbuffer:string for save register value
*  return value:  void
**********************************************************/
static void hi6523_fcp_reg_dump(char *pbuffer)
{
	u8 reg[HI6523_FCP_REG_TOTAL_NUM] = { 0 };
	char buff[14] = { 0 };
	u8 i =0;

	hi6523_read_block(5, &reg[0], 11);
	hi6523_read_block(32, &reg[11], 9);
	hi6523_read_block(61, &reg[20], 51);
	hi6523_read_block(144, &reg[71], 17);
	for (i = 0; i < 11; i++) {
		snprintf(buff, sizeof(buff), "r[%d]0x%-3x", i+5, reg[i]);
		strncat(pbuffer, buff, strlen(buff));
	}
	snprintf(buff, sizeof(buff), "\n");
	strncat(pbuffer, buff, strlen(buff));
	for (i = 11; i < 20; i++) {
		snprintf(buff, sizeof(buff), "r[%d]0x%-3x", i+21, reg[i]);
		strncat(pbuffer, buff, strlen(buff));
	}
	snprintf(buff, sizeof(buff), "\n");
	strncat(pbuffer, buff, strlen(buff));
	for (i = 20; i < 71; i++) {
		if(!((i+41)%10)) {
			snprintf(buff, sizeof(buff), "\n");
			strncat(pbuffer, buff, strlen(buff));
		}
		snprintf(buff, sizeof(buff), "r[%d]0x%-3x", i+41, reg[i]);
		strncat(pbuffer, buff, strlen(buff));
	}
	snprintf(buff, sizeof(buff), "\n");
	strncat(pbuffer, buff, strlen(buff));
	for (i = 71; i < 88; i++) {
		snprintf(buff, sizeof(buff), "r[%d]0x%-3x", i+73, reg[i]);
		strncat(pbuffer, buff, strlen(buff));
	}
	return;
}

/**********************************************************
*  Function:       hi6523_get_register_head
*  Discription:    print the register head in charging process
*  Parameters:   reg_head:string for save register head
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_get_register_head(char *reg_head)
{
	char buff[26] = { 0 };
	int i = 0;

	memset(reg_head, 0, CHARGELOG_SIZE);
	snprintf(buff, 26, "Ibus    ");
	strncat(reg_head, buff, strlen(buff));
	for (i = 0; i < HI6523_REG_TOTAL_NUM; i++) {
		snprintf(buff, 26, "Reg[0x%2x] ", i);
		strncat(reg_head, buff, strlen(buff));
	}
	return 0;
}

/**********************************************************
*  Function:       hi6523_set_batfet_disable
*  Discription:    set the batfet disable in charging process
*  Parameters:   disable:batfet disable or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_set_batfet_disable(int disable)
{
	return hi6523_set_batfet_ctrl(!disable);
}

/**********************************************************
*  Function:       hi6523_set_watchdog_timer
*  Discription:    set the watchdog timer in charging process
*  Parameters:   value:watchdog timer value
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_set_watchdog_timer(int value)
{
	u8 val = 0;
	u8 dog_time = value;
	if (dog_time >= WATCHDOG_TIMER_40_S) {
		val = 3;
	} else if (dog_time >= WATCHDOG_TIMER_20_S) {
		val = 2;
	} else if (dog_time >= WATCHDOG_TIMER_10_S) {
		val = 1;
	} else {
		val = 0;
	}
	hwlog_info(" watch dog timer is %d ,the register value is set %u \n",
		     dog_time, val);
	return hi6523_write_mask(WATCHDOG_CTRL_REG, WATCHDOG_TIMER_MSK,
				 WATCHDOG_TIMER_SHIFT, val);
}

/**********************************************************
*  Function:       hi6523_set_charger_hiz
*  Discription:    set the charger hiz close watchdog
*  Parameters:   enable:charger in hiz or not
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_set_charger_hiz(int enable)
{
	return hi6523_write_mask(CHG_HIZ_CTRL_REG, CHG_HIZ_ENABLE_MSK,
				 CHG_HIZ_ENABLE_SHIFT, enable);
}

struct fcp_adapter_device_ops sh_fcp_hi6523_ops = {
	.reg_dump = hi6523_fcp_reg_dump,
};

static struct charge_device_ops hi6523_ops = {
	.dev_check = hi6523_device_check,
	.set_input_current = hi6523_set_input_current,
	.set_charge_current = hi6523_set_charge_current,
	.set_terminal_voltage = hi6523_set_terminal_voltage,
	.set_charge_enable = hi6523_set_charge_enable,
	.set_otg_enable = hi6523_set_otg_enable,
	.dump_register = hi6523_dump_register,
	.get_register_head = hi6523_get_register_head,
	.set_watchdog_timer = hi6523_set_watchdog_timer,
	.set_batfet_disable = hi6523_set_batfet_disable,
	.get_ibus = hi6523_get_ibus_ma,
	.get_vbus = hi6523_get_vbus_mv,
	.get_vbat_sys = hi6523_get_vbat_sys,
	.set_charger_hiz = hi6523_set_charger_hiz,
};

/**********************************************************
*  Function:       hi6523_probe
*  Discription:    HI6523 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct class *power_class = NULL;

	ret = charge_ops_register(&hi6523_ops);
	if (ret) {
		hwlog_err("register charge ops failed!\n");
		goto hi6523_fail_0;
	}

	ret = hi6523_sysfs_create_group(&pdev->dev);
	if (ret) {
		hwlog_err("create sysfs entries failed!\n");
		goto hi6523_fail_0;
	}
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL) {
			charge_dev =
			    device_create(power_class, NULL, 0, NULL,
					  "charger");
			if (IS_ERR(charge_dev)) {
				charge_dev = NULL;
				hwlog_err("create charge_dev failed!\n");
				goto hi6523_fail_1;
			}
		}
		ret = sysfs_create_link(&charge_dev->kobj, &pdev->dev.kobj,"HI6523");
		if (ret)
			hwlog_err("create link to HI6523 fail.\n");
	}
	hwlog_info("HI6523 sensorhub charge probe ok!\n");
	return 0;

hi6523_fail_1:
	hi6523_sysfs_remove_group(&pdev->dev);
hi6523_fail_0:
	return ret;
}

/**********************************************************
*  Function:       hi6523_remove
*  Discription:    hi6523 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int hi6523_remove(struct platform_device *pdev)
{
    hi6523_sysfs_remove_group(&pdev->dev);
    return 0;
}

static struct of_device_id hi6523_of_match[] = {
	{
	 .compatible = "huawei,hi6523_charger_sensorhub",
	 .data = NULL,
	 },
	{
	 },
};

static struct platform_driver hi6523_driver = {
	.probe = hi6523_probe,
	.remove = hi6523_remove,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "hi6523_charger_sensorhub",
		   .of_match_table = of_match_ptr(hi6523_of_match),
	},
};

/**********************************************************
*  Function:       charge_core_init
*  Discription:    charge module initialization
*  Parameters:   pdev:platform_device
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init hi6523_init(void)
{
	return platform_driver_register(&hi6523_driver);
}

/**********************************************************
*  Function:       charge_core_exit
*  Discription:    charge module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit hi6523_exit(void)
{
	platform_driver_unregister(&hi6523_driver);
}

module_init(hi6523_init);
module_exit(hi6523_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("bq25892 sensorhub charger module driver");
MODULE_AUTHOR("HW Inc");
