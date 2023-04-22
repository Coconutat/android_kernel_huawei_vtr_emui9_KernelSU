

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
#include "hisi_scharger_v300.h"
#include <linux/raid/pq.h>
#include <linux/mfd/hisi_pmic.h>
#include <pmic_interface.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#ifdef CONFIG_DIRECT_CHARGER
#include <huawei_platform/power/direct_charger.h>
#endif
#ifdef CONFIG_USB_AUDIO_POWER
#include <huawei_platform/audio/usb_audio_power.h>
#endif
#ifdef CONFIG_HUAWEI_CHARGER
#include <huawei_platform/power/huawei_charger.h>
#else
#include <linux/power/hisi/charger/hisi_charger.h>
#endif
#include <linux/hisi/hisi_adc.h>
#ifdef CONFIG_SWITCH_FSA9685
#include <huawei_platform/usb/switch/switch_fsa9685.h>
#endif
#ifdef CONFIG_BOOST_5V
#include <huawei_platform/power/boost_5v.h>
#endif
#include "securec.h"

#define MAX_RBOOST_CNT	(300)
#define ILIMIT_RBOOST_CNT	(15)
struct hi6523_device_info *g_hi6523_dev = NULL;
struct mutex hi6523_fcp_detect_lock;
struct mutex hi6523_adc_conv_lock;
struct mutex hi6523_accp_adapter_reg_lock;
struct mutex hi6523_ibias_calc_lock;
static int adc_channel_vbat_sys = -1;
static unsigned int is_board_type = 2;	/*0:sft 1:udp 2:asic */
unsigned short hi6523_version;
static int plugout_check_ibus_ma = 150;
static int plugout_check_delay_ms = 1000;
static unsigned int switch_id_flag = 0;/*switch id status no need changed*/
static int hi6523_force_set_term_flag = CHG_STAT_DISABLE;
static char g_batt_ovp_cnt_30s;
static char g_vbat_check_cycle;
static char g_batt_ovp_cnt_1s;
static int g_rboost_cnt;
static int I_bias_all = 0;
static int dm_array_len = 0;
static int dp_array_len = 0;
#ifdef CONFIG_DIRECT_CHARGER
static u32 scp_error_flag = 0;
#endif
static unsigned int single_phase_buck;

static u8 scaf_randnum_remote[SCP_ADAPTOR_RANDOM_NUM_LO_LEN];
static u8 scaf_digest_remote_hi[SCP_ADAPTOR_DIGEST_LEN];

static int hi6523_get_vbus_mv(unsigned int *vbus_mv);
static int hi6523_get_charge_state(unsigned int *state);
static int hi6523_set_charge_enable(int enable);
static int hi6523_fcp_adapter_reg_read(u8 * val, u8 reg);
static int hi6523_fcp_adapter_reg_write(u8 val, u8 reg);

#ifdef CONFIG_DIRECT_CHARGER
static int hi6523_is_support_scp(void);
#endif

/*init vterm-vdpm table*/
static struct charge_cv_vdpm_data vterm_vdpm[] = {
	{4150,4460,3},
	{4400,4675,4},
};
static int charger_is_fcp = FCP_FALSE;
static int first_insert_flag = FIRST_INSERT_TRUE;
static int is_weaksource = WEAKSOURCE_FALSE;
static int Gain_cal = 0;
static int iin_set = CHG_ILIMIT_100;

#ifdef CONFIG_DIRECT_CHARGER
static int g_direct_charge_mode = 0;
static void scp_set_direct_charge_mode(int mode)
{
	g_direct_charge_mode = mode;
	SCHARGER_INF("%s : g_direct_charge_mode = %d \n",__func__, g_direct_charge_mode);
}
static int scp_get_direct_charge_mode(void)
{
	SCHARGER_INF("%s : g_direct_charge_mode = %d \n",__func__, g_direct_charge_mode);
	return g_direct_charge_mode;
}
#endif

static void set_boot_weaksource_flag(void)
{
#ifdef CONFIG_HISI_COUL_HI6421V700
    unsigned int reg_val = 0;
    reg_val = hisi_pmic_reg_read(WEAKSOURCE_FLAG_REG);
    reg_val |= WAEKSOURCE_FLAG;
    hisi_pmic_reg_write(WEAKSOURCE_FLAG_REG, reg_val);
#endif
    return;
}

static void clr_boot_weaksource_flag(void)
{
#ifdef CONFIG_HISI_COUL_HI6421V700
    unsigned int reg_val = 0;
    reg_val = hisi_pmic_reg_read(WEAKSOURCE_FLAG_REG);
    reg_val &= (~WAEKSOURCE_FLAG);
    hisi_pmic_reg_write(WEAKSOURCE_FLAG_REG, reg_val);
#endif
    return;
}
/**********************************************************
*  Function:       is_hi6523_cv_limit
*  Description:    juege if do cv limit
*  Parameters:   NULL
*  return value:  TRUE or FALSE
**********************************************************/
bool is_hi6523_cv_limit(void)
{
	if ((CHG_VERSION_V300 == hi6523_version)
		|| (CHG_VERSION_V310 == hi6523_version))
		return TRUE;
	else
		return FALSE;
}
EXPORT_SYMBOL(is_hi6523_cv_limit);

/**********************************************************
*  Function:        scharger_i2c_err_monitor
*  Description:    record SchargerV300 i2c trans error, and when need, notify it
*  Parameters:    NA
*  return value:   NA
**********************************************************/
static void scharger_i2c_err_monitor(void)
{
	static int scharger_i2c_err_cnt = RESET_VAL_ZERO;

	scharger_i2c_err_cnt ++;
	if(I2C_ERR_MAX_COUNT <= scharger_i2c_err_cnt){
		scharger_i2c_err_cnt = RESET_VAL_ZERO;
		atomic_notifier_call_chain(&fault_notifier_list,
			CHARGE_FAULT_I2C_ERR,
			NULL);
	}
}

/**********************************************************
*  Function:       hi6523_write_block
*  Description:    register write block interface
*  Parameters:   di:hi6523_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_write_block(struct hi6523_device_info *di, u8 * value,
			      u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

	*value = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = value;
	msg[0].len = num_bytes + 1;

	ret = i2c_transfer(di->client->adapter, msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 1) {
		SCHARGER_ERR("i2c_write failed to transfer all messages\n");
		scharger_i2c_err_monitor();
		if (ret < 0)
			return ret;
		else
			return -EIO;
	} else {
		return 0;
	}
}

/**********************************************************
*  Function:       hi6523_read_block
*  Description:    register read block interface
*  Parameters:   di:hi6523_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_read_block(struct hi6523_device_info *di, u8 * value,
			     u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[2];
	u8 buf = 0;
	int ret = 0;

	buf = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = &buf;
	msg[0].len = 1;

	msg[1].addr = di->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = value;
	msg[1].len = num_bytes;

	ret = i2c_transfer(di->client->adapter, msg, 2);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 2) {
		SCHARGER_ERR("i2c_write failed to transfer all messages\n");
		scharger_i2c_err_monitor();
		if (ret < 0)
			return ret;
		else
			return -EIO;
	} else {
		return 0;
	}
}

/**********************************************************
*  Function:       hi6523_write_byte
*  Description:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_write_byte(u8 reg, u8 value)
{
	struct hi6523_device_info *di = g_hi6523_dev;
	/* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
	u8 temp_buffer[2] = { 0 };
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}
	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return hi6523_write_block(di, temp_buffer, reg, 1);
}

/**********************************************************
*  Function:       hi6523_read_byte
*  Description:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_read_byte(u8 reg, u8 * value)
{
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}

	return hi6523_read_block(di, value, reg, 1);
}

/**********************************************************
*  Function:       hi6523_write_mask
*  Description:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-success or others-fail
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
*  Description:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-success or others-fail
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
	HI6523_SYSFS_FIELD_RW(adapter_reg, NONE, NONE),
	HI6523_SYSFS_FIELD_RW(adapter_val, NONE, NONE),
};

static struct attribute *hi6523_sysfs_attrs[ARRAY_SIZE(hi6523_sysfs_field_tbl) +
					    1];

static const struct attribute_group hi6523_sysfs_attr_group = {
	.attrs = hi6523_sysfs_attrs,
};

/**********************************************************
*  Function:       hi6523_sysfs_init_attrs
*  Description:    initialize hi6523_sysfs_attrs[] for HI6523 attribute
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
*  Description:    get the current device_attribute from hi6523_sysfs_field_tbl by attr's name
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
*  Description:    show the value for all HI6523 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-success or others-fail
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
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}

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

	if (!strncmp("adapter_reg", attr->attr.name, strlen("adapter_reg"))) {
		return scnprintf(buf, PAGE_SIZE, "0x%x\n", di->sysfs_fcp_reg_addr);
	}
	if (!strncmp("adapter_val", attr->attr.name, strlen("adapter_val"))) {
#ifdef CONFIG_HISI_DEBUG_FS
                ret = hi6523_fcp_adapter_reg_read(&v, di->sysfs_fcp_reg_addr);
                SCHARGER_INF(" sys read fcp adapter reg 0x%x , v 0x%x \n", di->sysfs_fcp_reg_addr, v);
                if (ret)
                        return ret;
#endif
                return scnprintf(buf, PAGE_SIZE, "0x%x\n", v);
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
*  Description:    set the value for all HI6523 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-success or others-fail
**********************************************************/
static ssize_t hi6523_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct hi6523_sysfs_field_info *info;
	struct hi6523_sysfs_field_info *info2;
	int ret;
	u8 v;
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}

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

	if (!strncmp(("adapter_reg"), attr->attr.name, strlen("adapter_reg"))) {
                        di->sysfs_fcp_reg_addr = (u8)v;
                        return count;
	}
	if (!strncmp(("adapter_val"), attr->attr.name, strlen("adapter_val"))) {
                        di->sysfs_fcp_reg_val = (u8)v;
#ifdef CONFIG_HISI_DEBUG_FS
                        ret = hi6523_fcp_adapter_reg_write(di->sysfs_fcp_reg_val,
                                                                di->sysfs_fcp_reg_addr);
                        SCHARGER_INF(" sys write fcp adapter reg 0x%x , v 0x%x \n",
                                                      di->sysfs_fcp_reg_addr, di->sysfs_fcp_reg_val);

                        if (ret)
                                return ret;
#endif
                        return count;
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
*  Description:    create the HI6523 device sysfs group
*  Parameters:   di:hi6523_device_info
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_sysfs_create_group(struct hi6523_device_info *di)
{
	hi6523_sysfs_init_attrs();

	return sysfs_create_group(&di->dev->kobj, &hi6523_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Description:    remove the HI6523 device sysfs group
*  Parameters:   di:hi6523_device_info
*  return value:  NULL
**********************************************************/
static void hi6523_sysfs_remove_group(struct hi6523_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &hi6523_sysfs_attr_group);
}
#else
static int hi6523_sysfs_create_group(struct hi6523_device_info *di)
{
	return 0;
}

static inline void hi6523_sysfs_remove_group(struct hi6523_device_info *di)
{
}
#endif

/**********************************************************
*  Function:       hi6523_device_check
*  Description:    check chip i2c communication
*  Parameters:   null
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_device_check(void)
{
	int ret = 0;
	u8 reg_chip_id0 = 0xff;

	ret = hi6523_read_byte(CHIP_VERSION_4, &reg_chip_id0);
	if (ret) {
		SCHARGER_ERR("[%s]:read chip_id0 fail\n", __func__);
		return CHARGE_IC_BAD;
	}

	if (CHIP_ID0 == reg_chip_id0 || CHIP_ID1 == reg_chip_id0
		|| CHIP_ID2 == reg_chip_id0) {
		SCHARGER_INF("hi6523 is good.\n");
		return CHARGE_IC_GOOD;
	}
	SCHARGER_ERR("hi6523 is bad.\n");
	return CHARGE_IC_BAD;
}

/**********************************************************
*  Function:       hi6523_set_bat_comp
*  Description:    set the bat comp
				schargerv100 can't set ir comp due to lx bug
*  Parameters:   value:bat_comp mohm
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_bat_comp(int value)
{
	u8 reg;

	if ((CHG_VERSION_V300 == hi6523_version)
		|| (CHG_VERSION_V310 == hi6523_version))
		return 0;

	if (value < CHG_IR_COMP_MIN)
		reg = 0;
	else if (value >= CHG_IR_COMP_0mohm && value < CHG_IR_COMP_15mohm)
		reg = 0;
	else if (value >= CHG_IR_COMP_15mohm && value < CHG_IR_COMP_30mohm)
		reg = 1;
	else if (value >= CHG_IR_COMP_30mohm && value < CHG_IR_COMP_45mohm)
		reg = 2;
	else if (value >= CHG_IR_COMP_45mohm && value < CHG_IR_COMP_60mohm)
		reg = 3;
	else if (value >= CHG_IR_COMP_60mohm && value < CHG_IR_COMP_75mohm)
		reg = 4;
	else if (value >= CHG_IR_COMP_75mohm && value < CHG_IR_COMP_95mohm)
		reg = 5;
	else if (value >= CHG_IR_COMP_95mohm && value < CHG_IR_COMP_110mohm)
		reg = 6;
	else
		reg = 7;
	return hi6523_write_mask(CHG_IR_COMP_REG, CHG_IR_COMP_MSK,
				 CHG_IR_COMP_SHIFT, reg);
}

/**********************************************************
*  Function:       hi6523_set_vclamp
*  Description:    set the vclamp
*  Parameters:   value:vclamp mv
				schargerv100 can't set vclamp due to lx bug
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_vclamp(int value)
{
	u8 reg;

	if ((CHG_VERSION_V300 == hi6523_version)
		|| (CHG_VERSION_V310 == hi6523_version))
		return 0;
	if (value < CHG_IR_VCLAMP_MIN)
		value = CHG_IR_VCLAMP_MIN;
	else if (value > CHG_IR_VCLAMP_MAX)
		value = CHG_IR_VCLAMP_MAX;
	else {
		//do nothing
	}
	reg = value / CHG_IR_VCLAMP_STEP;
	return hi6523_write_mask(CHG_IR_VCLAMP_REG, CHG_IR_VCLAMP_MSK,
				 CHG_IR_VCLAMP_SHIFT, reg);
}

/**********************************************************
*  Function:       hi6523_set_adc_channel
*  Description:    select adc channel
*  Parameters:     channel
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_adc_channel(u32 chan)
{
	return hi6523_write_mask(CHG_ADC_CTRL_REG, CHG_ADC_CH_MSK,
				 CHG_ADC_CH_SHIFT, (u8) chan);
}

/**********************************************************
*  Function:       hi6523_adc_enable
*  Description:    enable hi6523 adc
*  Parameters:   value: 1(enable) or 0(disable)
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_adc_enable(u32 enable)
{
	return hi6523_write_mask(CHG_ADC_CTRL_REG, CHG_ADC_EN_MSK,
				 CHG_ADC_EN_SHIFT, (u8) enable);
}

/**********************************************************
*  Function:       hi6523_adc_conv_status
*  Description:    get hi6523 adc conv_status
*  Parameters:     null
*  return value:  0-in conv or others-fail
**********************************************************/
static int hi6523_get_adc_conv_status(u8 * value)
{
	return hi6523_read_mask(CHG_ADC_CONV_STATUS_REG,
				CHG_ADC_CONV_STATUS_MSK,
				CHG_ADC_CONV_STATUS_SHIFT, value);
}

/**********************************************************
*  Function:       hi6523_set_conv_start
*  Description:    set covn start
*  Parameters:     chan:adc channel ,data :adc value
*  return value:   0-success or others-fail
**********************************************************/
static int hi6523_get_adc_value(u32 chan, u32 * data)
{
	int ret = 0;
	u8 reg = 0;
	int i = 0;
	u8 adc_data[2] = { 0 };
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}

	mutex_lock(&hi6523_adc_conv_lock);
	ret |= hi6523_set_adc_channel(chan);
	ret |= hi6523_adc_enable(CHG_ADC_EN);
	ret |=
	    hi6523_write_mask(CHG_ADC_START_REG, CHG_ADC_START_MSK,
			      CHG_ADC_START_SHIFT, TRUE);
	if (ret) {
		SCHARGER_ERR("set covn fail! ret =%d \n", ret);
		hi6523_adc_enable(CHG_ADC_DIS);
		mutex_unlock(&hi6523_adc_conv_lock);
		return -1;
	}
	/*The conversion result is ready after tCONV, max 10ms */
	for (i = 0; i < 10; i++) {
		ret = hi6523_get_adc_conv_status(&reg);
		if (ret) {
			SCHARGER_ERR(" HI6523 read ADC CONV STAT fail!.\n");
			continue;
		}
		/* if ADC Conversion finished, hkadc_valid bit will be 1 */
		if (reg == 1) {
			break;
		}
		msleep(1);
	}

	if (10 == i) {
		SCHARGER_ERR("Wait for ADC CONV timeout! \n");
		hi6523_adc_enable(CHG_ADC_DIS);
		mutex_unlock(&hi6523_adc_conv_lock);
		return -1;
	}
	ret |= hi6523_read_block(di, adc_data, CHG_ADC_DATA_REG, 2);
	ret |= hi6523_adc_enable(CHG_ADC_DIS);
	if (ret) {
		SCHARGER_ERR("[%s]get ibus_ref_data fail,ret:%d\n", __func__,
			     ret);
		hi6523_adc_enable(CHG_ADC_DIS);
		mutex_unlock(&hi6523_adc_conv_lock);
		return -1;
	}
	*data = (u32) adc_data[0] * 256 + adc_data[1];
	mutex_unlock(&hi6523_adc_conv_lock);
	return 0;
}

/**********************************************************
*  Function:     hi6523_set_fast_safe_timer()
*  Description:  set fast safe timer
*  Parameters:   safe timer value
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6523_set_fast_safe_timer(u32 chg_fastchg_safe_timer)
{
	return hi6523_write_mask(CHG_FASTCHG_TIMER_REG, CHG_FASTCHG_TIMER_MSK,
				 CHG_FASTCHG_TIMER_SHIFT,
				 (u8) chg_fastchg_safe_timer);
}

/**********************************************************
*  Function:     hi6523_set_precharge_current()
*  Description:  config precharge current limit
*  Parameters:   precharge current
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6523_set_precharge_current(int precharge_current)
{
	u8 prechg_limit;

	if (precharge_current < CHG_PRG_ICHG_MIN) {
		precharge_current = CHG_PRG_ICHG_MIN;
	} else if (precharge_current > CHG_PRG_ICHG_MAX) {
		precharge_current = CHG_PRG_ICHG_MAX;
	} else {
		//do nothing
	}

	prechg_limit =
	    (u8) ((precharge_current - CHG_PRG_ICHG_MIN) / CHG_PRG_ICHG_STEP);

	return hi6523_write_mask(CHG_PRE_ICHG_REG, CHG_PRE_ICHG_MSK,
				 CHG_PRE_ICHG_SHIFT, prechg_limit);
}

/**********************************************************
*  Function:     hi6523_set_precharge_voltage()
*  Description:  config precharge voltage
*  Parameters:   precharge voltage
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6523_set_precharge_voltage(u32 pre_vchg)
{
	u8 vprechg;
	if (pre_vchg <= CHG_PRG_VCHG_2800)
		vprechg = 0;
	else if (pre_vchg > CHG_PRG_VCHG_2800 && pre_vchg <= CHG_PRG_VCHG_3000)
		vprechg = 1;
	else if (pre_vchg > CHG_PRG_VCHG_3000 && pre_vchg <= CHG_PRG_VCHG_3100)
		vprechg = 2;
	else if (pre_vchg > CHG_PRG_VCHG_3100 && pre_vchg <= CHG_PRG_VCHG_3200)
		vprechg = 3;
	else
		vprechg = 0;	/*default 2.8V */
	return hi6523_write_mask(CHG_PRE_VCHG_REG, CHG_PRE_VCHG_MSK,
				 CHG_PRE_VCHG_SHIFT, vprechg);
}

/**********************************************************
*  Function:     hi6523_set_batfet_ctrl()
*  Description:  config batfet status 1:enable, 0: disable
*  Parameters:   status
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6523_set_batfet_ctrl(u32 status)
{
	return hi6523_write_mask(BATFET_CTRL_CFG_REG, BATFET_CTRL_CFG_MSK,
				 BATFET_CTRL_CFG_SHIFT, status);
}

/**********************************************************
*  Function:       hi6523_get_charge_enable
*  Description:    get the charge enable in charging process
*  Parameters:     void
*  return value:   TRUE or FALSE
**********************************************************/
static bool hi6523_get_charge_enable(void)
{
	u8 charge_state = 0;

	hi6523_read_mask(CHG_ENABLE_REG, CHG_EN_MSK, CHG_EN_SHIFT, &charge_state);

	if (charge_state) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/**********************************************************
*  Function:     hi6523_vbus_init_set()
*  Description:  set vbus voltage init para
*  Parameters:   vbus_vol:VBUS_VSET_5V&VBUS_VSET_9V&VBUS_VSET_12V
*				charge_stop_flag:1 stop charge; 0 charge
*  return value:
*                 void
**********************************************************/
static void hi6523_vbus_init_set(int vbus_vol)
{
	if (VBUS_VSET_5V == vbus_vol) {
		hi6523_write_mask(0xa0, VBUS_VSET_MSK, VBUS_VSET_SHIFT, 0x00);
		hi6523_write_byte(0x78, 0x0);
		if ((CHG_VERSION_V300 == hi6523_version)
			|| (CHG_VERSION_V310 == hi6523_version)) {
			hi6523_write_byte(0x8e, 0xc6);
			hi6523_write_byte(0x62, 0x2c);
		} else {
			hi6523_write_byte(0x8e, 0xc8);
			hi6523_write_byte(0x62, 0x24);
		}
		hi6523_write_byte(0x64, 0x28);
		hi6523_write_byte(0x71, 0x54);
		if ((CHG_VERSION_V300 == hi6523_version)
			|| (CHG_VERSION_V310 == hi6523_version))
			hi6523_write_byte(0x95, 0x07);
		else
			hi6523_write_mask(0x95, 0x07, 0x00, 0x00);
	} else if (VBUS_VSET_9V == vbus_vol) {
		hi6523_write_mask(0xa0, VBUS_VSET_MSK, VBUS_VSET_SHIFT, 0x01);
		hi6523_write_byte(0x62, 0x2C);
		hi6523_write_byte(0x64, 0x40);
		hi6523_write_byte(0x71, 0x55);
		if ((CHG_VERSION_V300 == hi6523_version)
			|| (CHG_VERSION_V310 == hi6523_version))
			hi6523_write_byte(0x95, 0x07);
		else
			hi6523_write_mask(0x95, 0x07, 0x00, 0x03);
	} else if (VBUS_VSET_12V == vbus_vol) {
		hi6523_write_mask(0xa0, VBUS_VSET_MSK, VBUS_VSET_SHIFT, 0x10);
		hi6523_write_byte(0x62, 0x2C);
		hi6523_write_byte(0x64, 0x40);
		hi6523_write_byte(0x71, 0x55);
		if ((CHG_VERSION_V300 == hi6523_version)
			|| (CHG_VERSION_V310 == hi6523_version))
			hi6523_write_byte(0x95, 0x07);
		else
			hi6523_write_mask(0x95, 0x07, 0x00, 0x03);
	}

	/* for single-phase synchronous BUCK */
	if(1 == single_phase_buck){
		hi6523_write_mask(CHG_OTG_RESERVE2, CHG_BUCK_OCP_HALVED_MASK, CHG_BUCK_OCP_HALVED_SHIFT, 1);
		SCHARGER_INF("%s: buck ocp is halved for single-phase synchronous BUCK!\n", __func__);
	}
}

/**********************************************************
*  Function:     hi6523_set_vbus_vset()
*  Description:  set vbus_vset voltage
*  Parameters:   vbus_set voltage: 5V/9V/12V
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6523_set_vbus_vset(u32 value)
{
	u8 data = 0;
	u32 charger_flag = 0;

	/*check charge state, if open, close charge.*/
	if (TRUE == hi6523_get_charge_enable()) {
		hi6523_set_charge_enable(CHG_DISABLE);
		charger_flag = 1;
	}
	if (value < VBUS_VSET_9V) {
		hi6523_vbus_init_set(VBUS_VSET_5V);
		charger_is_fcp = FCP_FALSE;
		data = 0;
	} else if (value < VBUS_VSET_12V) {
		hi6523_vbus_init_set(VBUS_VSET_9V);
		charger_is_fcp = FCP_TRUE;
		data = 1;
	} else {
		data = 2;
	}

	/*resume charge state*/
	if (1 == charger_flag) {
		hi6523_set_charge_enable(CHG_ENABLE);
	}
	return hi6523_write_mask(CHG_VBUS_VSET_REG, VBUS_VSET_MSK,
				 VBUS_VSET_SHIFT, data);
}

/**********************************************************
*  Function:     hi6523_config_opt_param()
*  Description:  config opt parameters for hi6523
*  Parameters:   vbus_vol:5V/9V/12V
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6523_config_opt_param(int vbus_vol)
{
	if (CHG_VERSION_V300 == hi6523_version) {
		hi6523_write_byte(0x78, 0x0);
		hi6523_write_byte(0x8e, 0x46);
		hi6523_write_byte(0x62, 0x2c);
		hi6523_write_byte(0x64, 0x28);
		hi6523_write_byte(0x66, 0xf1);
		hi6523_write_mask(0xE1, 0x07 << 0x04, 0x04, 0x03);
		hi6523_write_mask(0xA0, 0x01 << 0x02, 0x02, 0x01);
		hi6523_write_byte(0xE0, 0x87);
		hi6523_write_byte(0x65, 0x80);
		hi6523_write_byte(0x69, 0xd4);
		hi6523_write_byte(0x6a, 0x1f);
		hi6523_write_byte(0x6b, 0x1c);
		hi6523_write_byte(0x72, 0x5d);
		hi6523_write_byte(0x73, 0x18);
		hi6523_write_byte(0x74, 0x01);
		hi6523_write_byte(0x75, 0xd1);
		hi6523_write_byte(0x71, 0x54);
		hi6523_write_byte(0x76, 0x01);
		hi6523_write_byte(0x79, 0x90);
		hi6523_write_byte(0x84, 0x25);
		hi6523_write_byte(0x87, 0x65);
		hi6523_write_byte(0x95, 0x07);
		hi6523_write_byte(0xA3, 0x04);
	} else if (CHG_VERSION_V310 == hi6523_version) {
		hi6523_write_byte(0x78, 0x0);
		hi6523_write_byte(0x8e, 0x46);
		hi6523_write_byte(0x62, 0x2c);
		hi6523_write_byte(0x64, 0x28);
		hi6523_write_byte(0x66, 0xf1);
		hi6523_write_mask(0xE1, 0x07 << 0x04, 0x04, 0x03);
		hi6523_write_mask(0xA0, 0x01 << 0x02, 0x02, 0x01);
		hi6523_write_byte(0xE0, 0x87);
		hi6523_write_byte(0x65, 0x80);
		hi6523_write_byte(0x69, 0xd4);
		hi6523_write_byte(0x6a, 0x1f);
		hi6523_write_byte(0x6b, 0x1c);
		hi6523_write_byte(0x72, 0x5d);
		hi6523_write_byte(0x73, 0x18);
		hi6523_write_byte(0x74, 0x01);
		hi6523_write_byte(0x75, 0xd1);
		hi6523_write_byte(0x71, 0x54);
		hi6523_write_byte(0x76, 0x01);
		hi6523_write_byte(0x79, 0x90);
		hi6523_write_byte(0x84, 0x25);
		hi6523_write_byte(0x87, 0x65);
		hi6523_write_byte(0x95, 0x07);
		hi6523_write_byte(0xA3, 0x04);
	}
	else if (CHG_VERSION_V320 == hi6523_version || CHG_VERSION_V200 <= hi6523_version) {
		hi6523_write_byte(0x78, 0x0);
		hi6523_write_byte(0x8e, 0x48);
		hi6523_write_byte(0x62, 0x24);
		hi6523_write_byte(0x64, 0x28);
		hi6523_write_byte(0x66, 0xf1);
		hi6523_write_mask(0xE1, 0x03 << 0x05, 0x05, 0x01);
		hi6523_write_mask(0xA0, 0x01 << 0x02, 0x02, 0x01);
		hi6523_write_byte(0xE0, 0x87);
		if(1 == single_phase_buck){
			/* for single-phase synchronous BUCK */
			hi6523_write_byte(CHG_BUCK_REG5_REG, 0x00);
			SCHARGER_INF("%s: single-phase synchronous BUCK !\n", __func__);
		}else{
			/* for dual-phase synchronous BUCK */
			hi6523_write_byte(CHG_BUCK_REG5_REG, 0x80);
		}
		hi6523_write_byte(0x69, 0xd4);
		hi6523_write_byte(0x6a, 0x1f);
		hi6523_write_byte(0x6b, 0x1c);
		hi6523_write_byte(0x72, 0x5d);
		hi6523_write_byte(0x73, 0x18);
		hi6523_write_byte(0x74, 0x01);
		hi6523_write_byte(0x75, 0xd1);
		hi6523_write_byte(0x71, 0x54);
		hi6523_write_byte(0x76, 0x02);
		hi6523_write_byte(0x79, 0x90);
		hi6523_write_byte(0x84, 0x25);
		hi6523_write_byte(0x87, 0x65);
		hi6523_write_byte(0x96, 0x04);
		hi6523_write_byte(0x97, 0xd4);
		hi6523_write_byte(0x9e, 0x8a);
		hi6523_write_byte(0xA3, 0x04);
	}

	hi6523_vbus_init_set(vbus_vol);
	return 0;
}

/**********************************************************
*  Function:       hi6523_set_input_current
*  Description:    set the input current in charging process
*  Parameters:   value:input current value
*  return value:  0-success or others-fail
**********************************************************/

static int hi6523_set_input_current(int cin_limit)
{
	u8 Iin_limit;

	if (cin_limit <= CHG_ILIMIT_100)
		Iin_limit = 0;
	else if (cin_limit <= CHG_ILIMIT_130)
		Iin_limit = 1;
	else if (cin_limit <= CHG_ILIMIT_699)
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

	iin_set = cin_limit;

	SCHARGER_INF("input current reg is set 0x%x\n", Iin_limit);
	return hi6523_write_mask(CHG_INPUT_SOURCE_REG, CHG_ILIMIT_MSK,
				 CHG_ILIMIT_SHIFT, Iin_limit);
}

/**********************************************************
*  Function:       hi6523_get_input_current
*  Description:    get the input current limit
*  Parameters:  NULL
*  return value:  ilimit mA
**********************************************************/

static int hi6523_get_input_current(void)
{
	int ret = 0;
	u8 reg = 0;
	hi6523_read_mask(CHG_INPUT_SOURCE_REG, CHG_ILIMIT_MSK, CHG_ILIMIT_SHIFT,
			 &reg);
	switch (reg) {
	case 0:
		ret = 100;
		break;
	case 1:
		ret = 130;
		break;
	case 2:
		ret = 470;
		break;
	case 3:
		ret = 900;
		break;
	case 4:
		ret = 1000;
		break;
	case 5:
		ret = 1080;
		break;
	case 6:
		ret = 1200;
		break;
	case 7:
		ret = 1300;
		break;
	case 8:
		ret = 1400;
		break;
	case 9:
		ret = 1500;
		break;
	case 10:
		ret = 1600;
		break;
	case 11:
		ret = 1700;
		break;
	case 12:
		ret = 1800;
		break;
	case 13:
		ret = 1900;
		break;
	case 14:
		ret = 2000;
		break;
	case 15:
		ret = 2100;
		break;
	case 16:
		ret = 2200;
		break;
	case 17:
		ret = 2280;
		break;
	case 18:
		ret = 2400;
		break;
	case 19:
		ret = 2480;
		break;
	case 20:
		ret = 2600;
		break;
	case 21:
		ret = 2680;
		break;
	case 22:
		ret = 2800;
		break;
	case 23:
		ret = 2880;
		break;
	case 24:
		ret = 3000;
		break;
	case 25:
		ret = 3080;
		break;
	case 26:
		ret = 3200;
		break;
	default:
		break;
	}
	return ret;
}

static int hi6523_get_input_current_set(void)
{
	return iin_set;
}

/**********************************************************
*  Function:       hi6523_get_charge_current
*  Description:    get the charge current in charging process
*  Parameters:   NULL
*  return value:  the charge current
**********************************************************/
static int hi6523_get_charge_current(void)
{
	int ret = 0;
	u8 reg = 0;
	int charge_current = 0;
	ret = hi6523_read_mask(CHG_FAST_CURRENT_REG, CHG_FAST_ICHG_MSK, CHG_FAST_ICHG_SHIFT,
		&reg);
	if(ret)
	{
		SCHARGER_INF("HI6523 read mask fail\n");
		return CHG_FAST_ICHG_00MA;
	}
	if(reg == CURRENT_STEP_0)
		charge_current = CHG_FAST_ICHG_100MA;
	else if(reg<=CURRENT_STEP_9)
		charge_current = CHG_FAST_ICHG_STEP_100 * reg + CHG_FAST_ICHG_100MA;
	else if(reg <= CURRENT_STEP_14)
		charge_current = (reg - CURRENT_STEP_9)*CHG_FAST_ICHG_STEP_200 + CHG_FAST_ICHG_1000MA;
	else if(reg <=CURRENT_STEP_24)
		charge_current = (reg - CURRENT_STEP_14)*CHG_FAST_ICHG_STEP_100 + CHG_FAST_ICHG_2000MA;
	else if(reg <=CURRENT_STEP_30)
		charge_current = (reg - CURRENT_STEP_24)*CHG_FAST_ICHG_STEP_200 + CHG_FAST_ICHG_3000MA;
	else if(reg == CURRENT_STEP_31)
		charge_current = CHG_FAST_ICHG_4500MA;
	else
	{
		charge_current = CHG_FAST_ICHG_00MA;
	}

	SCHARGER_INF("charge current is set %d %d\n", charge_current,reg);
	return charge_current;

}

/**********************************************************
*  Function:       hi6523_get_charge_current_bias
*  Description:    calc the charge current bias in charging process
*  Parameters:   value:charge current value
*  return value:  charge currernt with bias
**********************************************************/
static int hi6523_calc_charge_current_bias(int charge_current)
{
	int I_coul_ma = 0;
	int I_bias = 0;
	int I_delta_ma = 0;
	static int last_ichg = 0;

	mutex_lock(&hi6523_ibias_calc_lock);
    /*if target is less than 1A,need not to calc ibias,need not to minus bias calculated before*/
	if (CHG_FAST_ICHG_1000MA > charge_current){
		last_ichg = charge_current;
		mutex_unlock(&hi6523_ibias_calc_lock);
		return charge_current;
	}
    /*if target charge current changed, no need to calc ibias,just use bias calculated before*/
	if (last_ichg != charge_current) {
		last_ichg = charge_current;
		charge_current -= I_bias_all;
		mutex_unlock(&hi6523_ibias_calc_lock);
		return charge_current;
	}

    /*calculate bias with difference between I_coul and last target charge current*/
	I_coul_ma = hisi_battery_current();
    /*current from hisi_battery_current is negative while charging,change to positive to calc with charge_current*/
    I_coul_ma = -I_coul_ma;
	I_delta_ma = I_coul_ma - charge_current;
    /*if I_coul is less than last target charge current for more than 100ma, bias should minus 100ma */
	if (-CHG_FAST_ICHG_100MA > I_delta_ma)
		I_bias = -CHG_FAST_ICHG_100MA;
    /*if difference between I_coul and last target charge current is less than 100ma, no need to add bias*/
	else if (CHG_FAST_ICHG_100MA > I_delta_ma)
		I_bias = 0;
	else
    /*if difference between I_coul and last target charge current is more than 100ma, calc bias with 100 rounding down*/
		I_bias = (I_delta_ma/100)*100;

    /*update i_bias_all within [0,400] ma*/
	I_bias_all += I_bias;
	if (I_bias_all <= 0)
		I_bias_all = 0;
	if (I_bias_all >= CHG_FAST_ICHG_400MA)
		I_bias_all = CHG_FAST_ICHG_400MA;

    /*update charge current*/
	charge_current -= I_bias_all;
	SCHARGER_INF("%s:Ichg:%d, I_coul_ma:%d,Ibias:%d,I_bias_all:%d\n", __func__, charge_current, I_coul_ma, I_bias, I_bias_all);
	mutex_unlock(&hi6523_ibias_calc_lock);
	return charge_current;
}
/**********************************************************
*  Function:       hi6523_set_charge_current
*  Description:    set the charge current in charging process
*  Parameters:   value:charge current value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_charge_current(int charge_current)
{
	u8 Ichg_limit;

	charge_current = hi6523_calc_charge_current_bias(charge_current);
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
	SCHARGER_INF("charge current reg is set 0x%x\n", Ichg_limit);
	return hi6523_write_mask(CHG_FAST_CURRENT_REG, CHG_FAST_ICHG_MSK,
				 CHG_FAST_ICHG_SHIFT, Ichg_limit);
}

/**********************************************************
*  Function:       hi6523_set_terminal_voltage
*  Description:    set the terminal voltage in charging process
		(v300&v310 scharger's max cv is 4.25V due to lx bug)
*  Parameters:   value:terminal voltage value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_terminal_voltage(int charge_voltage)
{
	u8 data;
	struct hi6523_device_info *di = g_hi6523_dev;

	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}

	if (charge_voltage < CHG_FAST_VCHG_MIN) {
		charge_voltage = CHG_FAST_VCHG_MIN;
	} else if (charge_voltage > CHG_FAST_VCHG_MAX) {
		charge_voltage = CHG_FAST_VCHG_MAX;
	} else {
		//do nothing
	}
	if (TRUE == is_hi6523_cv_limit()) {
		if (charge_voltage > CHG_FAST_VCHG_4250)
			charge_voltage = CHG_FAST_VCHG_4250;
	}
	di->term_vol_mv = charge_voltage;

	//need not do dpm, set vterm directly
	if(DPM_ENABLE == di->param_dts.dpm_en){
		if(WEAKSOURCE_TRUE == is_weaksource){
			SCHARGER_ERR("weaksource true, do not set CV.\n");
			return 0;
		}
		SCHARGER_INF("no need do vdpm, just set cv!\n");
	}

	data =
	    (u8) ((charge_voltage - CHG_FAST_VCHG_MIN) / CHG_FAST_VCHG_STEP_50);
	return hi6523_write_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
				 CHG_FAST_VCHG_SHIFT, data);
}

/**********************************************************
*  Function:       hi6523_get_terminal_voltage
*  Description:    get the terminal voltage in charging process
*  Parameters:   value:terminal voltage value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_get_terminal_voltage(void)
{
	u8 data = 0;

	hi6523_read_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
				 CHG_FAST_VCHG_SHIFT, &data);
	return (int)(data * CHG_FAST_VCHG_STEP_50 + CHG_FAST_VCHG_MIN);
}
/**********************************************************
*  Function:       hi6523_check_input_dpm_state
*  Description:    check whether VINDPM or IINDPM
*  Parameters:     NULL
*  return value:   TRUE means VINDPM or IINDPM
*                  FALSE means NoT DPM
**********************************************************/
static int hi6523_check_input_dpm_state(void)
{
	u8 reg = 0;
	int ret;

	ret = hi6523_read_byte(CHG_STATUS0_REG, &reg);
	if (ret < 0) {
		SCHARGER_ERR("hi6523_check_input_dpm_state err\n");
		return ret;
	}

	if (CHG_IN_DPM_STATE == (reg & CHG_IN_DPM_STATE)){
		SCHARGER_INF("CHG_STATUS0_REG:0x%2x in dpm state.\n",reg);
		return TRUE;
	}
	return FALSE;
}

/**********************************************************
*  Function:       hi6523_check_input_iacl_state
*  Description:    check whether ACL
*  Parameters:     NULL
*  return value:   TRUE means ACL
*                  FALSE means NoT ACL
**********************************************************/
static int hi6523_check_input_acl_state(void)
{
	u8 reg = 0;
	int ret;

	ret = hi6523_read_byte(CHG_STATUS0_REG, &reg);
	if (ret < 0) {
		SCHARGER_ERR("hi6523_check_input_acl_state err\n");
		return ret;
	}
	if (CHG_IN_ACL_STATE == (reg & CHG_IN_ACL_STATE)){
		SCHARGER_INF("CHG_STATUS0_REG:0x%2x in acl state.\n",reg);
		return TRUE;
	}
	return FALSE;
}

/**********************************************************
*  Function:       hi6523_get_charge_state
*  Description:    get the charge states in charging process
*  Parameters:   state:charge states
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_get_charge_state(unsigned int *state)
{
	u8 reg1 = 0, reg2 = 0;
	int ret = 0;

	ret |= hi6523_read_byte(CHG_STATUS0_REG, &reg1);
	ret |= hi6523_read_byte(CHG_STATUS1_REG, &reg2);
	if (ret) {
		SCHARGER_ERR("[%s]read charge status reg fail,ret:%d\n",
			     __func__, ret);
		return -1;
	}

	if (HI6523_CHG_BUCK_OK != (reg1 & HI6523_CHG_BUCK_OK))
		*state |= CHAGRE_STATE_NOT_PG;
	if (HI6523_CHG_STAT_CHARGE_DONE == (reg1 & HI6523_CHG_STAT_CHARGE_DONE))
		*state |= CHAGRE_STATE_CHRG_DONE;
	if (HI6523_WATCHDOG_OK != (reg2 & HI6523_WATCHDOG_OK))
		*state |= CHAGRE_STATE_WDT_FAULT;
	return ret;
}

/**********************************************************
*  Function:       hi6523_get_anti_reverbst_state
*  Description:    check if the anti_reverbst is enabled in charging process
*  Parameters:    NULL
*  return value:  TRUE or FALSE
**********************************************************/
static bool hi6523_get_anti_reverbst_enabled(void)
{
	u8 reg_val = 0;

	hi6523_read_mask(CHG_ANTI_REVERBST_REG, CHG_ANTI_REVERBST_EN_MSK, CHG_ANTI_REVERBST_EN_SHIFT, &reg_val);

	if (CHG_ANTI_REVERBST_EN != reg_val) {
		return FALSE;
	}
	return TRUE;
}

/**********************************************************
*  Function:       check_weaksource
*  Description:    check the charger and the line is weak source
*  Parameters:     NULL
*  return value:   TRUE: weak source
*                  FALSE: not weak_source
**********************************************************/
static int check_weaksource(void)
{
	/*init return val, test result, and temp val to 0*/
	int ret = 0;
	int result = 0;
	int orig_iin = 0;
	int current_iin = 0;

	orig_iin = hi6523_get_input_current();
	ret = hi6523_set_input_current(WEAK_IIN_THRESHOLD);
	if(ret){
		SCHARGER_ERR("%s set inputcurrent fail!\n", __func__);
		return WEAK_TEMP_FALSE;
	}
	msleep(WEAK_CHECK_WAIT_100);
	current_iin = hi6523_get_input_current();
	if(WEAK_IIN_THRESHOLD != current_iin){
		SCHARGER_ERR("during weak checking, iin changed, return false!\n");
		return WEAK_TEMP_FALSE;
	}
	if(hi6523_check_input_dpm_state()){
		result = WEAK_TEMP_TRUE;
		SCHARGER_ERR("%s weak source.\n", __func__);
	}else{
		result = WEAK_TEMP_FALSE;
		SCHARGER_ERR("%s not weak source.\n", __func__);
	}
	ret = hi6523_set_input_current(orig_iin);
	if(ret){
		SCHARGER_ERR("%s set inputcurrent fail!\n", __func__);
	}

	return result;
}

/**********************************************************
*  Function:       hi6523_apply_dpm_policy
*  Description:    apply Vdpm-Vterm modifing policy
*  Parameters:     vindpm: current Vdpm selected by Vbat trend
*  return value:   0: not need do dpm or Vdpm-Vterm set ok
*                  1: Vdpm-Vterm not set ok
**********************************************************/
static int hi6523_apply_dpm_policy(int vindpm)
{
	/*init return val and temp val to 0*/
	int i = 0;
	int vterm = 0;
	int Vdpm_set_val = 0;
	u8 Vterm_set = 0;
	int ret_vterm = 0;
	int ret_vdpm = 0;
	int vterm_vdpm_tbl_len = 0;
	static int last_vdpm = VINDPM_4700;//init last_vdpm to max
	static int weak_check_cnt = 0;
	static int weak_sum = 0;
	struct hi6523_device_info *di = g_hi6523_dev;

	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return 0;//not do dpm
	}

	if(di->charger_type != CHARGER_TYPE_DCP || FCP_TRUE == charger_is_fcp)
	{
		SCHARGER_ERR("charger_type is %d, do not modify dpm.\n",di->charger_type);
		return 0;//not do dpm
	}

	//charger type is DCP, first insert, do nothing
	if(FIRST_INSERT_TRUE == first_insert_flag){
		first_insert_flag = FIRST_INSERT_FALSE;
		is_weaksource = WEAKSOURCE_FALSE;
		weak_check_cnt = 0;
		weak_sum = 0;
		last_vdpm = VINDPM_4700;
		return 0;//not do dpm
	}

	//start check weaksource, check two times
	if(WEAKSOURCE_CHECK_MAX_CNT > weak_check_cnt){
		if(WEAK_TEMP_TRUE == check_weaksource()){
			weak_sum++;
		}
		weak_check_cnt++;
		if(WEAKSOURCE_CHECK_MAX_CNT == weak_check_cnt){
			if(WEAKSOURCE_CONFIRM_THRES <= weak_sum){
				is_weaksource = WEAKSOURCE_TRUE;
				atomic_notifier_call_chain(&fault_notifier_list,
					   CHARGE_FAULT_WEAKSOURCE,
					   NULL);
			}else{
				is_weaksource = WEAKSOURCE_FALSE;
			}
		}else{
			return 0;//check times is less than 3,just return
		}
	}

	if(WEAKSOURCE_FALSE == is_weaksource){
		SCHARGER_INF("weaksource false, not do dpm.\n");
		return 0;//not do dpm
	}else{
		SCHARGER_INF("weaksource true, do dpm and cv.\n");
	}

	//select vterm according to Vdpm_set
	vterm_vdpm_tbl_len = sizeof(vterm_vdpm)/(sizeof(vterm_vdpm[0]));
	SCHARGER_INF("vterm_vdpm_tbl_len is %d.\n",vterm_vdpm_tbl_len);
	for(i = 0;i < vterm_vdpm_tbl_len;i++){
		if(vindpm==vterm_vdpm[i].vdpm){
			vterm = vterm_vdpm[i].cv;
			Vdpm_set_val = vterm_vdpm[i].vdpm_set_val;
			break;
		}
	}

	if(i == vterm_vdpm_tbl_len){
		SCHARGER_ERR("Can not find Vdpm-Vterm, apply original setting.\n");
		//set Vdpm to 4.675V; CV to 4.4V
		vterm = vterm_vdpm[vterm_vdpm_tbl_len-1].cv;
		Vdpm_set_val = vterm_vdpm[vterm_vdpm_tbl_len-1].vdpm_set_val;
		vterm = di->term_vol_mv < vterm ? di->term_vol_mv:vterm;
		Vterm_set = (u8)((vterm - CHG_FAST_VCHG_MIN) / CHG_FAST_VCHG_STEP_50);
		ret_vdpm = hi6523_write_mask(CHG_OTG_RESERVE2,CHG_5V_DPM_SHIFT_MSK,CHG_5V_DPM_SHIFT,Vdpm_set_val);
		if(ret_vdpm == 0){
			ret_vterm = hi6523_write_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
				 CHG_FAST_VCHG_SHIFT, Vterm_set);
		}
		last_vdpm = VINDPM_4700;

		return (ret_vdpm || ret_vterm);
	}

	vterm = di->term_vol_mv < vterm ? di->term_vol_mv:vterm;
	Vterm_set = (u8) ((vterm - CHG_FAST_VCHG_MIN) / CHG_FAST_VCHG_STEP_50);
	SCHARGER_INF("Do dpm_set, Vdpm: %d, Vterm: %d\n",vindpm,vterm);
	if(vindpm >= last_vdpm){
		//first set Vdpm. then set Vterm
		ret_vdpm = hi6523_write_mask(CHG_OTG_RESERVE2,CHG_5V_DPM_SHIFT_MSK,CHG_5V_DPM_SHIFT,Vdpm_set_val);
		if(ret_vdpm == 0){
			last_vdpm = vindpm;
			ret_vterm = hi6523_write_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
				 CHG_FAST_VCHG_SHIFT, Vterm_set);
		}
	}else{
		//first set Vterm, then set Vdpm
		ret_vterm = hi6523_write_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
				 CHG_FAST_VCHG_SHIFT, Vterm_set);
		if(ret_vterm == 0){
			ret_vdpm = hi6523_write_mask(CHG_OTG_RESERVE2,CHG_5V_DPM_SHIFT_MSK,CHG_5V_DPM_SHIFT,Vdpm_set_val);
			if(ret_vdpm == 0){
				last_vdpm = vindpm;
			}
		}
	}

	return (ret_vdpm || ret_vterm);
}

/**********************************************************
*  Function:       hi6523_set_dpm_voltage
*  Description:    set the dpm voltage in charging process
*  Parameters:   value:dpm voltage value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_dpm_voltage(int vindpm)
{
	int ret = 0;
	struct hi6523_device_info *di = g_hi6523_dev;

	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return ret;
	}

	if(DPM_ENABLE == di->param_dts.dpm_en){
		ret = hi6523_apply_dpm_policy(vindpm);
	}

	return ret;
}

/**********************************************************
*  Function:       hi6523_set_terminal_current
*  Description:    set the terminal current in charging process
*                   (min value is 400ma for scharger ic bug)
*  Parameters:   value:terminal current value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_terminal_current(int term_current)
{
	u8 Iterm;
	if (term_current <= CHG_TERM_ICHG_400MA)
		Iterm = 5;
	else if (term_current > CHG_TERM_ICHG_400MA
		 && term_current <= CHG_TERM_ICHG_500MA)
		Iterm = 6;
	else if (term_current > CHG_TERM_ICHG_500MA
		 && term_current <= CHG_TERM_ICHG_600MA)
		Iterm = 7;
	else
		Iterm = 5;	/*default 400mA */

	SCHARGER_INF(" term current reg is set 0x%x\n", Iterm);
	return hi6523_write_mask(CHG_TERM_ICHG_REG, CHG_TERM_ICHG_MSK,
				 CHG_TERM_ICHG_SHIFT, Iterm);
}

/**********************************************************
*  Function:       hi6523_set_charge_enable
*  Description:    set the charge enable in charging process
*  Parameters:     enable:charge enable or not
*  return value:   0-success or others-fail
**********************************************************/
static int hi6523_set_charge_enable(int enable)
{
	/*invalidate charge enable on udp board */
	if ((BAT_BOARD_UDP == is_board_type) && (CHG_ENABLE == enable))
		return 0;
	return hi6523_write_mask(CHG_ENABLE_REG, CHG_EN_MSK, CHG_EN_SHIFT,
				 enable);
}

/**********************************************************
*  Function:       hi6523_config_otg_opt_param
*  Description:    config hi6523 opt params for otg
*  Parameters:     NULL
*  return value:   0-success or others-fail
**********************************************************/
static int hi6523_config_otg_opt_param(void)
{
	int ret = 0;
	u8 otg_lim_set = 0;
	hi6523_read_mask(CHG_OTG_REG0, CHG_OTG_LIM_MSK, CHG_OTG_LIM_SHIFT,
			 &otg_lim_set);
	ret |= hi6523_write_byte(0x6a, 0x3f);
	ret |= hi6523_write_byte(0x6b, 0x5c);
	ret |= hi6523_write_byte(0x81, 0x5c);
	ret |= hi6523_write_byte(0x86, 0x39);
	/*need to config regs according to otg current */
	if (3 != otg_lim_set) {
		ret |= hi6523_write_byte(0x8a, 0x10);
		ret |= hi6523_write_byte(0x88, 0xbf);
	} else {
		ret |= hi6523_write_byte(0x8a, 0x18);
		ret |= hi6523_write_byte(0x88, 0x9f);
	}
	return ret;
}

/**********************************************************
*  Function:       hi6523_set_otg_enable
*  Description:    set the otg mode enable in charging process
*  Parameters:   enable:otg mode  enable or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_otg_enable(int enable)
{
	if (enable) {
		hi6523_set_charge_enable(CHG_DISABLE);
		hi6523_config_otg_opt_param();
	}
	return hi6523_write_mask(CHG_OTG_REG0, CHG_OTG_EN_MSK, CHG_OTG_EN_SHIFT,
				 enable);
}

/**********************************************************
*  Function:       hi6523_set_otg_current
*  Description:    set the otg mdoe current
*  Parameters:     value :current value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_otg_current(int value)
{
	unsigned int temp_currentmA = 0;
	u8 reg = 0;
	temp_currentmA = value;

	if (temp_currentmA < BOOST_LIM_MIN || temp_currentmA > BOOST_LIM_MAX)
		SCHARGER_INF("set otg current %dmA is out of range!\n", value);
	if (temp_currentmA < BOOST_LIM_500) {
		reg = 3;
	} else if (temp_currentmA >= BOOST_LIM_500
		   && temp_currentmA < BOOST_LIM_1000) {
		reg = 3;
	} else if (temp_currentmA >= BOOST_LIM_1000
		   && temp_currentmA < BOOST_LIM_1500) {
		reg = 1;
	} else if (temp_currentmA >= BOOST_LIM_1500
		   && temp_currentmA < BOOST_LIM_2000) {
		reg = 0;
	} else {
		reg = 0;
	}

	SCHARGER_INF(" otg current reg is set 0x%x\n", reg);
	return hi6523_write_mask(CHG_OTG_REG0, CHG_OTG_LIM_MSK,
				 CHG_OTG_LIM_SHIFT, reg);
}

/**********************************************************
*  Function:       hi6523_vbus_in_dpm
*  Description:    juege if in dpm by vbus vol
*  Parameters:   vbus mv
*  return value:  TRUE or FALSE
**********************************************************/
static bool hi6523_vbus_in_dpm(int vbus)
{
	if ((CHG_VERSION_V300 == hi6523_version)
		|| (CHG_VERSION_V310 == hi6523_version)) {
		if (vbus <= CHG_DPM_VOL_4855_MV)
			return TRUE;
		else
			return FALSE;
	} else {
		if (vbus <= CHG_DPM_VOL_4835_MV)
			return TRUE;
		else
			return FALSE;
	}
}

/**********************************************************
*  Function:       hi6523_set_term_enable
*  Description:    set the terminal enable in charging process
*  Parameters:   enable:terminal enable or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_term_enable(int enable)
{
	int vbatt_mv;
	int term_mv;
	u8 state = 0;
	int vbus = 0;
	int ret;

	if (CHG_STAT_ENABLE == hi6523_force_set_term_flag) {
		SCHARGER_INF("Charger is in the production line testing phase!\n");
		return 0;
	}

	if (0 == enable)
		return hi6523_write_mask(CHG_EN_TERM_REG, CHG_EN_TERM_MSK,
					 CHG_EN_TERM_SHIFT, (u8)enable);
	hi6523_read_byte(CHG_STATUS0_REG, &state);
	vbatt_mv = hisi_battery_voltage();
	term_mv = hi6523_get_terminal_voltage();
	ret = hi6523_get_vbus_mv((unsigned int *)&vbus);
	if (ret)
		return ret;
	if ((0 == (CHG_TERM_ABLE_STATE & state)) &&
			((FALSE == hi6523_vbus_in_dpm(vbus)) ||
			(0 == (CHG_IN_DPM_STATE & state))) &&
			(vbatt_mv > (term_mv - 100))) {
		return hi6523_write_mask(CHG_EN_TERM_REG, CHG_EN_TERM_MSK,
					 CHG_EN_TERM_SHIFT, (u8)enable);
	} else {
		SCHARGER_INF("cancel term_en state:0x%x,vbatt_mv:%d,term_mv:%d\n",
			state, vbatt_mv, term_mv);
		/*close EOC*/
		hi6523_write_mask(CHG_EN_TERM_REG, CHG_EN_TERM_MSK,
					 CHG_EN_TERM_SHIFT, (u8)CHG_TERM_DIS);
		return -1;
	}
}

/**********************************************************
*  Function:       hi6523_force_set_term_enable
*  Description:    set the terminal enable in charging process
*  Parameters:   enable:terminal enable or not
*                0&1:dbc control. 2:original charger procedure
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_force_set_term_enable(int enable)
{
	if ((0 == enable) || (1 == enable)) {
		hi6523_force_set_term_flag = CHG_STAT_ENABLE;
	} else {
		hi6523_force_set_term_flag = CHG_STAT_DISABLE;
		return 0;
	}
	return hi6523_write_mask(CHG_EN_TERM_REG, CHG_EN_TERM_MSK,
					 CHG_EN_TERM_SHIFT, (u8)enable);
}

/**********************************************************
*  Function:     hi6523_get_charger_state()
*  Description:  get charger state
*  Parameters:   NULL
*  return value:
*       charger state:
*        1:Charge Termination Done
*        0:Not Charging and buck is closed;Pre-charge;Fast-charg;
*       -1:error
**********************************************************/
static int hi6523_get_charger_state(void)
{
	u8 data = 0;
	int ret = -1;

	ret = hi6523_read_byte(CHG_STATUS0_REG, &data);
	if (ret) {
		SCHARGER_ERR("[%s]:read charge status reg fail.\n", __func__);
		return -1;
	}

	/*Charge buck was not work*/
	if (0 == (HI6523_CHG_BUCK_OK & data)) {
		return 0;
	}

	data &= HI6523_CHG_STAT_MASK;
	data = data >> HI6523_CHG_STAT_SHIFT;
	switch (data) {
	case 0:
	case 1:
	case 2:
		ret = 0;
		break;
	case 3:
		ret = 1;
		break;
	default:
		SCHARGER_ERR("get charger state fail\n");
		break;
	}

	return ret;
}

/**********************************************************
*  Function:       hi6523_is_vbus_adc_ready
*  Description:    check if vbus adc ready to sample
*  Parameters:   NULL
*  return value:  TRUE OR FALSE
**********************************************************/
static bool hi6523_is_vbus_adc_ready(void)
{
	u8 reg_val = 0;

	hi6523_read_mask(CHG_ADC_VBUS_RDY_REG, CHG_ADC_VBUS_RDY_MSK, CHG_ADC_VBUS_RDY_SHIFT, &reg_val);
	if (CHG_ADC_VBUS_RDY == reg_val)
		return TRUE;

	return FALSE;
}

/**********************************************************
*  Function:       hi6523_get_Gain_cal_value
*  Description:    get Gain_cal_value
*  Parameters:   Gain_cal:from efuse value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_get_vbus_gain_cal(int *vol)
{
	int ret = 0;
	int reg = 0;
	int value = 0;
	u8 reg_val = 0;

	ret = hi6523_read_byte(CHG_EFUSE_WE7_REG,&reg_val);
	if(ret){
		SCHARGER_ERR("[%s]get efuse fail,ret:%d\n", __func__, ret);
		return ret;
	}

	value = (reg_val & CHG_GAIN_CAL_VALUE_MASK) >> 1;
	reg = (reg_val & CHG_GAIN_CAL_REG_MASK) >> 4;
	if(0x1 == reg) {
		*vol = value * (-1);
	} else {
		*vol = value;
	}

	return 0;
}


/**********************************************************
*  Function:       hi6523_get_vbus_mv
*  Description:    get voltage of vbus
*  Parameters:   vbus_mv:voltage of vbus
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_get_vbus_mv(unsigned int *vbus_mv)
{
	int ret;
	u32 result = 0;
	u32 vref = 2000;	//2000mV
	u8 data = 0;

	if (FALSE == hi6523_is_vbus_adc_ready()) {
		hi6523_read_byte(CHG_ADC_VBUS_RDY_REG, &data);
		/*for debug log*/
		/*SCHARGER_INF("[%s]adc is not ready[0x7c]:0x%x\n", __func__, data);*/
		*vbus_mv = 0;
		return 0;
	}
	ret = hi6523_get_adc_value(CHG_ADC_CH_VBUS, &result);
	if (ret) {
		SCHARGER_ERR("[%s]get vbus_mv fail,ret:%d\n", __func__, ret);
		return -1;
	}

	*vbus_mv = result * vref * 7/ 4096 * (100000-125*Gain_cal)/100000;
	return ret;
}

/**********************************************************
*  Function:       hi6523_reset_watchdog_timer
*  Description:    reset watchdog timer in charging process
*  Parameters:   NULL
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_reset_watchdog_timer(void)
{
	return hi6523_write_mask(WATCHDOG_SOFT_RST_REG, WD_RST_N_MSK,
				 WATCHDOG_TIMER_SHIFT, WATCHDOG_TIMER_RST);
}

/**********************************************************
*  Function:       hi6523_get_ibus_ma
*  Description:    get average value for ilim
*  Parameters:     NULL
*  return value:   average value for ilim
**********************************************************/
static int hi6523_get_ibus_ma(void)
{
	int ret = 0;
	int ilimit = 0;
	u32 ibus_ref_data = 0;
	u32 ibus_data = 0;
	u32 ibus = 0;
	u32 state = 0;

	ret |= hi6523_get_charge_state(&state);
	if (ret) {
		SCHARGER_ERR("[%s] get_charge_state fail,ret:%d\n", __func__,
			     ret);
		return -1;
	}
	if (CHAGRE_STATE_NOT_PG & state) {
		/*for debug log*/
		/*SCHARGER_INF("[%s] CHAGRE_STATE_NOT_PG ,state:%d\n", __func__,
			     state);*/
		return 0;
	}
	ret |= hi6523_get_adc_value(CHG_ADC_CH_IBUS_REF, &ibus_ref_data);
	if (ret) {
		SCHARGER_ERR("[%s]get ibus_ref_data fail,ret:%d\n", __func__,
			     ret);
		return -1;
	}

	ret |= hi6523_get_adc_value(CHG_ADC_CH_IBUS, &ibus_data);
	if (ret) {
		SCHARGER_ERR("[%s]get ibus_data fail,ret:%d\n", __func__, ret);
		return -1;
	}

	ilimit = hi6523_get_input_current();
	if (0 == ibus_ref_data)
		return -1;
	ibus = ibus_data * ilimit / ibus_ref_data;
	return ibus;
}

/**********************************************************
*  Function:       hi6523_vbat_sys
*  Description:    get vsys sample
*  Parameters:     NULL
*  return value:   vsys sample
**********************************************************/
static int hi6523_vbat_sys(void)
{
	int i = 0;
	int retry_times = 3;
	int V_sample = -1;

	if (adc_channel_vbat_sys < 0)
		return V_sample;

	for (i = 0; i < retry_times; ++i) {
		V_sample = hisi_adc_get_value(adc_channel_vbat_sys);
		if (V_sample < 0) {
			SCHARGER_ERR("adc read channel 15 fail!\n");
		} else {
			break;
		}
	}
	return V_sample;
}

/**********************************************************
*  Function:       hi6523_get_vbat_sys
*  Description:    get vsys voltage
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
			SCHARGER_ERR("hi6523 get V_temp fail!\n");
		}
		msleep(delay_times);
	}
	if (cnt > 0) {
		return (3 * 1000 * sum / cnt);
	} else {
		SCHARGER_ERR("use 0 as default Vvlim!\n");
		return 0;
	}
}

/**********************************************************
*  Function:       hi6523_dump_register
*  Description:    print the register value in charging process
*  Parameters:   reg_value:string for save register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_dump_register(char *reg_value)
{
	u8 reg[HI6523_REG_TOTAL_NUM] = { 0 };
	char buff[26] = { 0 };
	int i = 0;
	int vbus = 0;
	int ret = 0;
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}
	memset(reg_value, 0, CHARGELOG_SIZE);
	hi6523_read_block(di, &reg[0], 0, HI6523_REG_TOTAL_NUM);
	ret = hi6523_get_vbus_mv((unsigned int *)&vbus);
	if (ret){
		SCHARGER_ERR("%s hi6523_get_vbus_mv failed!\n", __func__);
	}
	snprintf(buff, 26, "%-8.2d", hi6523_get_ibus_ma());
	strncat(reg_value, buff, strlen(buff));
	snprintf(buff, (unsigned long)26, "%-8.2d", vbus);
	strncat(reg_value, buff, strlen(buff));
	snprintf(buff, (unsigned long)26, "%-8.2d", I_bias_all);
	strncat(reg_value, buff, strlen(buff));
	for (i = 0; i < HI6523_REG_TOTAL_NUM; i++) {
		snprintf(buff, 26, "0x%-8x", reg[i]);
		strncat(reg_value, buff, strlen(buff));
	}
	return 0;
}

/**********************************************************
*  Function:       hi6523_get_register_head
*  Description:    print the register head in charging process
*  Parameters:   reg_head:string for save register head
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_get_register_head(char *reg_head)
{
	char buff[26] = { 0 };
	int i = 0;

	memset(reg_head, 0, CHARGELOG_SIZE);
	snprintf(buff, 26, "Ibus    ");
	strncat(reg_head, buff, strlen(buff));
	snprintf(buff, (unsigned long)26, "Vbus    ");
	strncat(reg_head, buff, strlen(buff));
	snprintf(buff, (unsigned long)26, "Ibias   ");
	strncat(reg_head, buff, strlen(buff));
	for (i = 0; i < HI6523_REG_TOTAL_NUM; i++) {
		snprintf(buff, 26, "Reg[0x%2x] ", i);
		strncat(reg_head, buff, strlen(buff));
	}
	return 0;
}

/**********************************************************
*  Function:       hi6523_set_batfet_disable
*  Description:    set the batfet disable in charging process
*  Parameters:   disable:batfet disable or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_batfet_disable(int disable)
{
	return hi6523_set_batfet_ctrl(!disable);
}

/**********************************************************
*  Function:       hi6523_set_watchdog_timer
*  Description:    set the watchdog timer in charging process
*  Parameters:   value:watchdog timer value
*  return value:  0-success or others-fail
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
	SCHARGER_INF(" watch dog timer is %d ,the register value is set %u \n",
		     dog_time, val);
	return hi6523_write_mask(WATCHDOG_CTRL_REG, WATCHDOG_TIMER_MSK,
				 WATCHDOG_TIMER_SHIFT, val);
}

/**********************************************************
*  Function:       hi6523_set_charger_hiz
*  Description:    set the charger hiz close watchdog
*  Parameters:   enable:charger in hiz or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_set_charger_hiz(int enable)
{
	return hi6523_write_mask(CHG_HIZ_CTRL_REG, CHG_HIZ_ENABLE_MSK,
				 CHG_HIZ_ENABLE_SHIFT, enable);
}

/**********************************************************
*  Function:       hi6523_is_charger_hiz
*  Description:   is charger in hiz or not
*  Parameters:   NULL
*  return value:  TRUE or FALSE
**********************************************************/
static bool hi6523_is_charger_hiz(void)
{
	u8 hiz_state = 0;
	hi6523_read_mask(CHG_HIZ_CTRL_REG, CHG_HIZ_ENABLE_MSK,/*lint !e835*/
		CHG_HIZ_ENABLE_SHIFT, &hiz_state);
	if (hiz_state)
		return TRUE;
	return FALSE;
}
/****************************************************************************
  Function:     hi6523_disable_adapter_detect
  Description:  enable scharger apple adapter detect
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: other fail
***************************************************************************/
static int hi6523_apple_adapter_detect(int enable)
{
	return hi6523_write_mask(CHG_ADC_APPDET_REG, APPLE_DETECT_MASK, APPLE_DETECT_SHIFT,
		enable);
}

/**********************************************************
*  Function:       is_dm_water_intrused
*  Description:   check voltage of DM
*  Parameters:   NULL
*  return value:  TRUE/False
**********************************************************/
static bool is_dm_water_intrused(u32 dm_vol)
{
    int i = 0;
    struct hi6523_device_info *di = g_hi6523_dev;
    if (NULL == di) {
        return FALSE;
    }

    if(0 == dm_array_len){
        if (dm_vol > HI6523_DPDM_WATER_THRESH_1460MV && dm_vol < HI6523_DPDM_WATER_THRESH_1490MV)
            return TRUE;
        else if (dm_vol > HI6523_DPDM_WATER_THRESH_1560MV && dm_vol < HI6523_DPDM_WATER_THRESH_1590MV)
            return TRUE;
        else
            return FALSE;
    }else{
        for(i = 0; i < dm_array_len/WATER_VOLT_PARA; i++){
            if(dm_vol > di->param_dts.scharger_check_vol.dm_vol_data[i].vol_min&&
                dm_vol < di->param_dts.scharger_check_vol.dm_vol_data[i].vol_max)
                return TRUE;
        }
            return FALSE;
    }
}

/**********************************************************
*  Function:       is_DP_water_intrused
*  Description:   check voltage of DM
*  Parameters:   NULL
*  return value:  TRUE/False
**********************************************************/
static bool is_dp_water_intrused(u32 dp_vol)
{
    int i = 0;
    struct hi6523_device_info *di = g_hi6523_dev;
    if (NULL == di) {
        return FALSE;
    }

    if(0 == dp_array_len){
        if (dp_vol > HI6523_DPDM_WATER_THRESH_1460MV && dp_vol < HI6523_DPDM_WATER_THRESH_1490MV)
            return TRUE;
        else if (dp_vol > HI6523_DPDM_WATER_THRESH_1560MV && dp_vol < HI6523_DPDM_WATER_THRESH_1590MV)
            return TRUE;
        else
            return FALSE;
    }else{
        for(i = 0; i < dp_array_len/WATER_VOLT_PARA; i++){
            if(dp_vol > di->param_dts.scharger_check_vol.dp_vol_data[i].vol_min&&
                dp_vol < di->param_dts.scharger_check_vol.dp_vol_data[i].vol_max)
                return TRUE;
        }
        return FALSE;
    }
}

/**********************************************************
*  Function:       hi6523_is_water_intrused
*  Description:   check voltage of DN/DP
*  Parameters:   NULL
*  return value:  1:water intrused/ 0:water not intrused/ -ENOMEM:hi6523 is not initializied
**********************************************************/
int hi6523_is_water_intrused(void)
{
	u32 dm_data = 0;
	u32 dp_data	= 0;
	u32 dm_vol = 0;
	u32 dp_vol = 0;
	int i = 0;

    if (NULL == g_hi6523_dev) {
		return -ENOMEM;
	}
	hi6523_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK, CHG_FCP_EN_SHIFT, FALSE);
	hi6523_apple_adapter_detect(CHG_ADC_APPDET_EN);
    usleep_range(9000,10000);
	hi6523_write_mask(CHG_ADC_APPDET_REG, CHG_ADC_APPDET_CHSEL_MSK, CHG_ADC_APPDET_CHSEL_SHIFT,
		CHG_ADC_APPDET_DMINUS);
	for (i = 0; i < HI6523_WATER_CHECKDPDN_NUM; i++) {
		hi6523_get_adc_value(CHG_ADC_CH_DET, &dm_data);
		dm_vol = HI6523_DPDM_CALC_MV(dm_data);
		SCHARGER_INF("%s:DM_VOL:%umv, data:0x%x\n", __func__,dm_vol, dm_data);
        if(FALSE == is_dm_water_intrused(dm_vol))
                break;
	}
    if (HI6523_WATER_CHECKDPDN_NUM == i) {
        SCHARGER_INF("D- water intrused\n");
        return 1;
    }
	hi6523_write_mask(CHG_ADC_APPDET_REG, CHG_ADC_APPDET_CHSEL_MSK, CHG_ADC_APPDET_CHSEL_SHIFT,
		CHG_ADC_APPDET_DPLUS);
	for (i = 0; i < HI6523_WATER_CHECKDPDN_NUM; i++) {
		hi6523_get_adc_value(CHG_ADC_CH_DET, &dp_data);
		dp_vol = HI6523_DPDM_CALC_MV(dp_data);
		SCHARGER_INF("%s:DP_VOL:%u mv, data:0x%x\n", __func__, dp_vol, dp_data);
        if(FALSE == is_dp_water_intrused(dp_vol))
                return 0;
	}
    SCHARGER_INF("D+ water intrused\n");
	return 1;
}

/****************************************************************************
  Function:     hi6523_soft_vbatt_ovp_protect
  Description:  vbatt soft ovp check
  Input:        NA
  Output:       NA
  Return:        0: success;-1: other fail
***************************************************************************/
static int hi6523_soft_vbatt_ovp_protect(void)
{
	struct hi6523_device_info *di = g_hi6523_dev;
	int vbatt_mv, vbatt_max;

	if (NULL == di)
		return -1;
	vbatt_mv = hisi_battery_voltage();
	vbatt_max = hisi_battery_vbat_max();
	if (vbatt_mv >= MIN(CHG_VBATT_SOFT_OVP_MAX, CHG_VBATT_CV_103(vbatt_max))) {
		g_batt_ovp_cnt_30s++;
		if (CHG_VBATT_SOFT_OVP_CNT == g_batt_ovp_cnt_30s) {
			hi6523_set_charger_hiz(TRUE);
			SCHARGER_ERR("%s:vbat:%d,cv_mv:%d,ovp_cnt:%d,shutdown buck.\n",
				__func__, vbatt_mv, di->term_vol_mv, g_batt_ovp_cnt_30s);
			g_batt_ovp_cnt_30s = 0;
			}
	} else
	g_batt_ovp_cnt_30s = 0;
	return 0;
}

/****************************************************************************
  Function:     hi6523_rboost_buck_limit
  Description:  limit buck current to 470ma according to rboost count
  Input:        NA
  Output:       NA
  Return:       0: do nothing; 1:limit buck current 470ma
***************************************************************************/
static int hi6523_rboost_buck_limit(void)
{
	if (ILIMIT_RBOOST_CNT < g_rboost_cnt) {
		SCHARGER_INF("%s:rboost cnt:%d\n", __func__, g_rboost_cnt);
        set_boot_weaksource_flag();
		return 1;
	}
	else {
		g_rboost_cnt = 0;
	}
	return 0;
}
/**********************************************************
*  Function:     hi6523_chip_init()
*  Description:  chip init for hi6523
*  Parameters:   chip_init_crit
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6523_chip_init(struct chip_init_crit* init_crit)
{
	int ret = 0;
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di || NULL == init_crit) {
		SCHARGER_ERR("%s hi6523_device_info or init_crit is NULL!\n", __func__);
		return -ENOMEM;
	}

	switch(init_crit->vbus) {
		case ADAPTER_5V:
			ret |= hi6523_config_opt_param(VBUS_VSET_5V);
			ret |= hi6523_set_vbus_vset(VBUS_VSET_5V);
			charger_is_fcp = FCP_FALSE;
			first_insert_flag = FIRST_INSERT_TRUE;
			break;
		case ADAPTER_9V:
			ret |= hi6523_config_opt_param(VBUS_VSET_9V);
			ret |= hi6523_set_vclamp(di->param_dts.vclamp);
			break;
		case ADAPTER_12V:
			ret |= hi6523_config_opt_param(VBUS_VSET_12V);
			ret |= hi6523_set_vclamp(di->param_dts.vclamp);
			break;
		default:
			SCHARGER_ERR("%s: init mode err\n", __func__);
			return -EINVAL;
	}

	ret |= hi6523_set_charge_enable(CHG_DISABLE);
	ret |= hi6523_set_fast_safe_timer(CHG_FASTCHG_TIMER_20H);
	ret |= hi6523_set_term_enable(CHG_TERM_DIS);
	ret |= hi6523_set_input_current(CHG_ILIMIT_470);
	ret |= hi6523_set_charge_current(CHG_FAST_ICHG_500MA);
	ret |= hi6523_set_terminal_voltage(CHG_FAST_VCHG_4400);
	ret |= hi6523_set_terminal_current(CHG_TERM_ICHG_150MA);
	ret |= hi6523_set_watchdog_timer(WATCHDOG_TIMER_40_S);
	ret |= hi6523_set_precharge_current(CHG_PRG_ICHG_200MA);
	ret |= hi6523_set_precharge_voltage(CHG_PRG_VCHG_2800);
	ret |= hi6523_set_batfet_ctrl(CHG_BATFET_EN);
	ret |= hi6523_set_bat_comp(di->param_dts.bat_comp);
	ret |= hi6523_set_otg_current(BOOST_LIM_1000);
	ret |= hi6523_set_otg_enable(OTG_DISABLE);

	return ret;
}

/**********************************************************
*  Function:       hi6523_fcp_get_adapter_output_current
*  Description:    fcp get the output current from adapter max power and output vol
*  Parameters:     NA
*  return value:  input_current(MA)
**********************************************************/
static int hi6523_fcp_get_adapter_output_current(void)
{

	return 0;
}

/****************************************************************************
  Function:     hi6523_fcp_cmd_transfer_check
  Description:  check cmd transfer success or fail
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/
static int hi6523_fcp_cmd_transfer_check(void)
{
	u8 reg_val1 = 0, reg_val2 = 0;
	int i = 0;
	int ret = 0;
	u8 reg_val3 = 0;
	u8 reg_val4 = 0;
	/*read accp interrupt registers until value is not zero */
	do {
		usleep_range(12000, 13000);
		ret |= hi6523_read_byte(CHG_FCP_ISR1_REG, &reg_val1);
		ret |= hi6523_read_byte(CHG_FCP_ISR2_REG, &reg_val2);
		ret |= hi6523_read_byte(CHG_FCP_IRQ3_REG, &reg_val3);
		ret |= hi6523_read_byte(CHG_FCP_IRQ4_REG, &reg_val4);
		if (ret) {
			SCHARGER_ERR("%s : reg read failed!\n", __func__);
			break;
		}
		if (reg_val1 || reg_val2) {
			if ((reg_val1 & CHG_FCP_ACK)
			    && (reg_val1 & CHG_FCP_CMDCPL)
			    && !(reg_val2 & (CHG_FCP_CRCRX | CHG_FCP_PARRX))) {
				return 0;
			} else if (((reg_val1 & CHG_FCP_CRCPAR) || (reg_val3 & CHG_FCP_INIT_HAND_FAIL) || (reg_val4 & CHG_FCP_ENABLE_HAND_FAIL))
			    && (reg_val2 & CHG_FCP_PROTSTAT)){
				SCHARGER_INF
				    ("%s :  FCP_TRANSFER_FAIL,slave status changed: ISR1=0x%x,ISR2=0x%x,ISR3=0x%x,ISR4=0x%x\n",
				     __func__, reg_val1, reg_val2, reg_val3, reg_val4);
				return -1;
			}else if (reg_val1 & CHG_FCP_NACK) {
				SCHARGER_INF
				    ("%s :  FCP_TRANSFER_FAIL,slave nack: ISR1=0x%x,ISR2=0x%x\n",
				     __func__, reg_val1, reg_val2);
				return -1;
			}else if ((reg_val2 & CHG_FCP_CRCRX) || (reg_val2 & CHG_FCP_PARRX)
				|| (reg_val3 & CHG_FCP_TAIL_HAND_FAIL)) {
				SCHARGER_INF
				    ("%s : FCP_TRANSFER_FAIL, CRCRX_PARRX_ERROR:ISR1=0x%x,ISR2=0x%x,ISR3=0x%x\n",
				     __func__, reg_val1, reg_val2, reg_val3);
				return -1;
			}else
				SCHARGER_INF
				    ("%s : FCP_TRANSFER_FAIL, ISR1=0x%x,ISR2=0x%x,ISR3=0x%x,total time = %dms\n",
				     __func__, reg_val1, reg_val2, reg_val3, i*10);
		}
		i++;
	} while (i < FCP_ACK_RETRY_CYCLE);

	SCHARGER_INF("%s : fcp adapter transfer time out,total time %d ms\n",
		     __func__, i * 10);
	return -1;
}

/****************************************************************************
  Function:     hi6523_fcp_protocol_restart
  Description:  disable accp protocol and enable again
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/
static void hi6523_fcp_protocol_restart(void)
{
	u8 reg_val = 0;
	int ret = 0;
	int i;
	/* disable accp protocol */
	mutex_lock(&hi6523_fcp_detect_lock);
	hi6523_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK, CHG_FCP_EN_SHIFT,
			  FALSE);
	usleep_range(9000, 10000);
	hi6523_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK, CHG_FCP_EN_SHIFT,
			  TRUE);
	/*detect hisi fcp charger, wait for ping succ */
	for (i = 0; i < HI6523_RESTART_TIME; i++) {
		usleep_range(9000, 10000);
		ret = hi6523_read_byte(CHG_FCP_STATUS_REG, &reg_val);
		if (ret) {
			SCHARGER_ERR("%s:read det attach err,ret:%d.\n",
				     __func__, ret);
			continue;
		}

		if ((CHG_FCP_SLAVE_GOOD ==
		     (reg_val & (CHG_FCP_DVC_MSK | CHG_FCP_ATTATCH_MSK)))) {
			break;
		}
	}

	if (HI6523_RESTART_TIME == i) {
		SCHARGER_ERR("%s:wait for slave fail\n", __func__);
		mutex_unlock(&hi6523_fcp_detect_lock);
		return;
	}
	mutex_unlock(&hi6523_fcp_detect_lock);
	SCHARGER_ERR("%s :disable and enable fcp protocol accp status  is 0x%x \n",__func__,reg_val);
}

/****************************************************************************
  Function:     hi6523_fcp_adapter_reg_read
  Description:  read adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:       0: success
                others: fail
***************************************************************************/
static int hi6523_fcp_adapter_reg_read(u8 * val, u8 reg)
{
	int ret = 0;
	int i = 0;
	u8 reg_val1 = 0, reg_val2 = 0;

	mutex_lock(&hi6523_accp_adapter_reg_lock);
	for(i = 0; i < FCP_RETRY_TIME; i++) {
		/*before send cmd, clear accp interrupt registers */
		ret |= hi6523_read_byte(CHG_FCP_ISR1_REG, &reg_val1);
		ret |= hi6523_read_byte(CHG_FCP_ISR2_REG, &reg_val2);
		if (reg_val1 != 0) {
			ret |= hi6523_write_byte(CHG_FCP_ISR1_REG, reg_val1);
		}
		if (reg_val2 != 0) {
			ret |= hi6523_write_byte(CHG_FCP_ISR2_REG, reg_val2);
		}

		ret |= hi6523_write_byte(CHG_FCP_CMD_REG, CHG_FCP_CMD_SBRRD);
		ret |= hi6523_write_byte(CHG_FCP_ADDR_REG, reg);
		ret |=
		    hi6523_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_SNDCMD_MSK,
				      CHG_FCP_SNDCMD_SHIFT, CHG_FCP_EN);

		if (ret) {
			SCHARGER_ERR("%s: write error ret is %d \n", __func__, ret);
			mutex_unlock(&hi6523_accp_adapter_reg_lock);
			return HI6523_FAIL;
		}

		/* check cmd transfer success or fail */
		if (0 == hi6523_fcp_cmd_transfer_check()) {
			/* recived data from adapter */
			ret |= hi6523_read_byte(CHG_FCP_RDATA_REG, val);
			break;
		}
		hi6523_fcp_protocol_restart();
	}
	if(FCP_RETRY_TIME == i)
	{
		SCHARGER_ERR("%s : ack error,retry %d times \n",__func__,i);
		ret = HI6523_FAIL;
	}
	mutex_unlock(&hi6523_accp_adapter_reg_lock);
	return ret;
}

/****************************************************************************
  Function:     hi6523_fcp_adapter_reg_write
  Description:  write value into the adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6523_fcp_adapter_reg_write(u8 val, u8 reg)
{
	int ret = 0;
	int i = 0;
	u8 reg_val1 = 0, reg_val2 = 0;

	mutex_lock(&hi6523_accp_adapter_reg_lock);
	for(i = 0; i < FCP_RETRY_TIME; i++) {
		/*before send cmd, clear accp interrupt registers */
		ret |= hi6523_read_byte(CHG_FCP_ISR1_REG, &reg_val1);
		ret |= hi6523_read_byte(CHG_FCP_ISR2_REG, &reg_val2);
		if (reg_val1 != 0) {
			ret |= hi6523_write_byte(CHG_FCP_ISR1_REG, reg_val1);
		}
		if (reg_val2 != 0) {
			ret |= hi6523_write_byte(CHG_FCP_ISR2_REG, reg_val2);
		}
		ret |= hi6523_write_byte(CHG_FCP_CMD_REG, CHG_FCP_CMD_SBRWR);
		ret |= hi6523_write_byte(CHG_FCP_ADDR_REG, reg);
		ret |= hi6523_write_byte(CHG_FCP_WDATA_REG, val);
		ret |=
		    hi6523_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_SNDCMD_MSK,
				      CHG_FCP_SNDCMD_SHIFT, CHG_FCP_EN);

		if (ret) {
			SCHARGER_ERR("%s: write error ret is %d \n", __func__, ret);
			mutex_unlock(&hi6523_accp_adapter_reg_lock);
			return HI6523_FAIL;
		}

		/* check cmd transfer success or fail */
		if (0 == hi6523_fcp_cmd_transfer_check()) {
			break;
		}
		hi6523_fcp_protocol_restart();
	}
	if(FCP_RETRY_TIME == i)
	{
		SCHARGER_ERR("%s : ack error,retry %d times \n",__func__,i);
		ret = HI6523_FAIL;
	}
	mutex_unlock(&hi6523_accp_adapter_reg_lock);
	return ret;
}

/****************************************************************************
  Function:     hi6523_fcp_get_adapter_output_vol
  Description:  get fcp output vol
  Input:        NA.
  Output:       fcp output vol(5V/9V/12V)*10
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6523_fcp_get_adapter_output_vol(u8 * vol)
{
	u8 num = 0;
	u8 output_vol = 0;
	int ret = 0;

	/*get adapter vol list number,exclude 5V */
	ret |=
	    hi6523_fcp_adapter_reg_read(&num,
					CHG_FCP_SLAVE_DISCRETE_CAPABILITIES);
	/*currently,fcp only support three out vol config(5v/9v/12v) */
	if (ret || num > 2) {
		SCHARGER_ERR("%s: vout list support err, reg[0x21] = %u.\n",
			     __func__, num);
		return -1;
	}

	/*get max out vol value */
	ret |=
	    hi6523_fcp_adapter_reg_read(&output_vol,
					CHG_FCP_SLAVE_REG_DISCRETE_OUT_V(num));
	if (ret) {
		SCHARGER_ERR
		    ("%s: get max out vol value failed ,ouputvol=%u,num=%u.\n",
		     __func__, output_vol, num);
		return -1;
	}
	*vol = output_vol;
	SCHARGER_INF("%s: get adapter max out vol = %u,num= %u.\n", __func__,
		     output_vol, num);
	return 0;
}

/****************************************************************************
  Function:     hi6523_fcp_adapter_vol_check
  Description:  check adapter voltage is around expected voltage
  Input:        adapter_vol_mv : expected adapter vol mv
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6523_fcp_adapter_vol_check(int adapter_vol_mv)
{
	int i = 0, ret = 0;
	int adc_vol = 0;
	if ((adapter_vol_mv < FCP_ADAPTER_MIN_VOL)
	    || (adapter_vol_mv > FCP_ADAPTER_MAX_VOL)) {
		SCHARGER_ERR("%s: check vol out of range, input vol = %dmV\n",
			     __func__, adapter_vol_mv);
		return -1;
	}

	for (i = 0; i < FCP_ADAPTER_VOL_CHECK_TIMEOUT; i++) {
		ret = hi6523_get_vbus_mv((unsigned int *)&adc_vol);
		if (ret) {
			continue;
		}
		if ((adc_vol > (adapter_vol_mv - FCP_ADAPTER_VOL_CHECK_ERROR))
		    && (adc_vol <
			(adapter_vol_mv + FCP_ADAPTER_VOL_CHECK_ERROR))) {
			break;
		}
		msleep(FCP_ADAPTER_VOL_CHECK_POLLTIME);
	}

	if (i == FCP_ADAPTER_VOL_CHECK_TIMEOUT) {
		SCHARGER_ERR("%s: check vol timeout, input vol = %dmV\n",
			     __func__, adapter_vol_mv);
		return -1;
	}
	SCHARGER_INF("%s: check vol success, input vol = %dmV, spent %dms\n",
		     __func__, adapter_vol_mv,
		     i * FCP_ADAPTER_VOL_CHECK_POLLTIME);
	return 0;
}

/****************************************************************************
  Function:     hi6523_fcp_set_adapter_output_vol
  Description:  set fcp adapter output vol
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6523_fcp_set_adapter_output_vol(int *output_vol)
{
	u8 val = 0;
	u8 vol = 0;
	int ret = 0;

	/*read ID OUTI , for identify huawei adapter */
	ret = hi6523_fcp_adapter_reg_read(&val, CHG_FCP_SLAVE_ID_OUT0);
	if (ret != 0) {
		SCHARGER_ERR("%s: adapter ID OUTI read failed, ret is %d \n",
			     __func__, ret);/*lint !e64*/
		return -1;
	}
	SCHARGER_INF("%s: id out reg[0x4] = %u.\n", __func__, val);

	/*get adapter max output vol value */
	ret = hi6523_fcp_get_adapter_output_vol(&vol);
	if (ret) {
		SCHARGER_ERR("%s: fcp get adapter output vol err.\n", __func__);
		return -1;
	}

	if (vol > CHG_FCP_OUTPUT_VOL_9V * CHG_FCP_VOL_STEP) {
		vol = CHG_FCP_OUTPUT_VOL_9V * CHG_FCP_VOL_STEP;
		SCHARGER_INF("fcp limit adapter vol to 9V, while adapter support 12V.\n");
	}
	*output_vol = vol / CHG_FCP_VOL_STEP;

	/*retry if write fail */
	ret |= hi6523_fcp_adapter_reg_write(vol, CHG_FCP_SLAVE_VOUT_CONFIG);
	ret |= hi6523_fcp_adapter_reg_read(&val, CHG_FCP_SLAVE_VOUT_CONFIG);
	SCHARGER_INF("%s: vout config reg[0x2c] = %u.\n", __func__, val);
	if (ret || val != vol) {
		SCHARGER_ERR("%s:out vol config err, reg[0x2c] = %u,vol :%d.\n",
			     __func__, val, vol);
		return -1;
	}

	ret |=
	    hi6523_fcp_adapter_reg_write(CHG_FCP_SLAVE_SET_VOUT,
					 CHG_FCP_SLAVE_OUTPUT_CONTROL);
	if (ret) {
		SCHARGER_ERR("%s : enable adapter output voltage failed \n ",
			     __func__);
		return -1;
	}

	ret |= hi6523_fcp_adapter_vol_check(vol / CHG_FCP_VOL_STEP * 1000);
	if (ret) {
		SCHARGER_ERR("%s : adc check adapter output voltage failed \n ",
			     __func__);
		return -1;
	}

	SCHARGER_INF("fcp adapter output vol set ok.\n");
	return 0;
}

/****************************************************************************
  Function:     hi6523_config_uvp_fvp
  Description:  hi6523 config uvp fvp
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6523_set_uvp_ovp(void)
{
	int ret = 0;

	ret |=
	    hi6523_write_mask(CHG_UVP_OVP_VOLTAGE_REG, CHG_UVP_OVP_VOLTAGE_MSK,
			      CHG_UVP_OVP_VOLTAGE_SHIFT,
			      CHG_UVP_OVP_VOLTAGE_MAX);

	if (ret) {
		SCHARGER_ERR("%s:uvp&ovp voltage set failed, ret = %d.\n",
			     __func__, ret);
		return -1;
	}

	return ret;
}

/****************************************************************************
  Function:     hi6523_fcp_switch_to_soc
  Description:  switch_to_master
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6523_fcp_switch_to_soc(void)
{
	SCHARGER_INF("%s\n", __func__);
#ifdef CONFIG_SWITCH_FSA9685
	usbswitch_common_manual_sw(FSA9685_USB1_ID_TO_IDBYPASS);
#endif
	return 0;
}

/****************************************************************************
  Function:     hi6523_fcp_switch_to_master
  Description:  switch_to_master
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6523_fcp_switch_to_master(void)
{
	SCHARGER_INF("%s\n", __func__);
#ifdef CONFIG_SWITCH_FSA9685
	usbswitch_common_manual_sw(FSA9685_USB2_ID_TO_IDBYPASS);
#endif
	return 0;
}

/****************************************************************************
  Function:     hi6523_fcp_adapter_detect
  Description:  detect fcp adapter
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: other fail
                1:fcp adapter but detect fail
***************************************************************************/
static int hi6523_fcp_adapter_detect(void)
{
	u8 reg_val1 = 0;
	u8 reg_val2 = 0;
	int i = 0;
	int ret = 0;

	mutex_lock(&hi6523_fcp_detect_lock);
	ret |= hi6523_read_byte(CHG_FCP_STATUS_REG, &reg_val2);
	if (ret) {
		SCHARGER_ERR("%s:read det attach err,ret:%d.\n", __func__, ret);
		mutex_unlock(&hi6523_fcp_detect_lock);
		return -1;
	}

	if (CHG_FCP_SLAVE_GOOD ==
		(reg_val2 & (CHG_FCP_DVC_MSK | CHG_FCP_ATTATCH_MSK))) {
		mutex_unlock(&hi6523_fcp_detect_lock);
		SCHARGER_INF("fcp adapter detect ok.\n");
		return CHG_FCP_ADAPTER_DETECT_SUCC;
	}
	ret |=
	    hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
			      CHG_FCP_DET_EN_SHIFT, TRUE);
	ret |= hi6523_apple_adapter_detect(APPLE_DETECT_DISABLE);
	ret |= hi6523_fcp_switch_to_master();
	ret |=
	    hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
			      CHG_FCP_CMP_EN_SHIFT, TRUE);
	if (ret) {
		SCHARGER_ERR("%s:FCP enable detect fail,ret:%d.\n", __func__,
			     ret);
		hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
				  CHG_FCP_CMP_EN_SHIFT, FALSE);
		hi6523_fcp_switch_to_soc();
		hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
				  CHG_FCP_DET_EN_SHIFT, FALSE);
		mutex_unlock(&hi6523_fcp_detect_lock);
		return -1;
	}
	/* wait for fcp_set */
	for (i = 0; i < CHG_FCP_DETECT_MAX_COUT; i++) {
		ret = hi6523_read_byte(CHG_FCP_SET_STATUS_REG, &reg_val1);
		if (ret) {
			SCHARGER_ERR("%s:read det attach err,ret:%d.\n",
				     __func__, ret);
			continue;
		}
		if (reg_val1 & CHG_FCP_SET_STATUS_MSK) {
			break;
		}
		msleep(CHG_FCP_POLL_TIME);
	}
	if (CHG_FCP_DETECT_MAX_COUT == i) {
		hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
				  CHG_FCP_CMP_EN_SHIFT, FALSE);
		hi6523_fcp_switch_to_soc();
		hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
				  CHG_FCP_DET_EN_SHIFT, FALSE);
		mutex_unlock(&hi6523_fcp_detect_lock);
		return CHG_FCP_ADAPTER_DETECT_OTHER;
	}

	/* enable fcp_en */
	hi6523_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK, CHG_FCP_EN_SHIFT,
			  TRUE);

	/*detect hisi fcp charger, wait for ping succ */
	for (i = 0; i < CHG_FCP_DETECT_MAX_COUT; i++) {
		ret = hi6523_read_byte(CHG_FCP_STATUS_REG, &reg_val2);
		if (ret) {
			SCHARGER_ERR("%s:read det attach err,ret:%d.\n",
				     __func__, ret);
			continue;
		}

		if ((CHG_FCP_SLAVE_GOOD ==
		     (reg_val2 & (CHG_FCP_DVC_MSK | CHG_FCP_ATTATCH_MSK)))) {
			break;
		}
		msleep(CHG_FCP_POLL_TIME);
	}

	if (CHG_FCP_DETECT_MAX_COUT == i) {
		hi6523_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK,
				  CHG_FCP_EN_SHIFT, FALSE);
		hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
				  CHG_FCP_CMP_EN_SHIFT, FALSE);
		hi6523_fcp_switch_to_soc();
		hi6523_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
				  CHG_FCP_DET_EN_SHIFT, FALSE);
		SCHARGER_ERR("fcp adapter detect failed,reg[0x%x]=0x%x\n",
			     CHG_FCP_STATUS_REG, reg_val2);
		mutex_unlock(&hi6523_fcp_detect_lock);
		return CHG_FCP_ADAPTER_DETECT_FAIL;	/*not fcp adapter */

	}
	SCHARGER_INF("fcp adapter detect ok\n");
	mutex_unlock(&hi6523_fcp_detect_lock);
	return CHG_FCP_ADAPTER_DETECT_SUCC;

}
static int fcp_adapter_detect(void)
{
	int ret;
#ifdef CONFIG_DIRECT_CHARGER
	u8 val;
#endif
	ret = hi6523_fcp_adapter_detect();
	if (CHG_FCP_ADAPTER_DETECT_OTHER == ret)
	{
		SCHARGER_INF("fcp adapter other detect\n");
		return FCP_ADAPTER_DETECT_OTHER;
	}
	if (CHG_FCP_ADAPTER_DETECT_FAIL == ret)
	{
		SCHARGER_INF("fcp adapter detect fail\n");
		return FCP_ADAPTER_DETECT_FAIL;
	}
#ifdef CONFIG_DIRECT_CHARGER
	if (hi6523_is_support_scp())
	{
		return FCP_ADAPTER_DETECT_SUCC;
	}
	ret = hi6523_fcp_adapter_reg_read(&val, SCP_ADP_TYPE);
	if(ret)
	{
		SCHARGER_ERR("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
		return FCP_ADAPTER_DETECT_SUCC;
	}
	return FCP_ADAPTER_DETECT_OTHER;
#else
	return FCP_ADAPTER_DETECT_SUCC;
#endif
}
/****************************************************************************
  Function:     hi6523_is_support_fcp
  Description:  check_if_support_fcp
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6523_is_support_fcp(void)
{
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}
	if (0 != di->param_dts.fcp_support) {
		SCHARGER_INF("support fcp charge \n");
		return 0;
	} else {
		return 1;
	}
}

/****************************************************************************
  Function:     hi6523_fcp_master_reset
  Description:  reset master
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6523_fcp_master_reset(void)
{
	int ret = 0;
	ret |= hi6523_write_byte(CHG_FCP_SOFT_RST_REG, CHG_FCP_SOFT_RST_VAL);
	ret |= hi6523_write_byte(CHG_FCP_CTRL_REG, 0);	//clear fcp_en and fcp_master_rst
	return ret;
}

/****************************************************************************
  Function:     hi6523_fcp_adapter_reset
  Description:  reset adapter
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6523_fcp_adapter_reset(void)
{
	u8 val = 0;
	int ret = 0, i = 0;
	u8 output_vol = 0;

	ret |= hi6523_set_vbus_vset(VBUS_VSET_5V);
	ret |= hi6523_fcp_adapter_reg_read((u8*)&output_vol, CHG_FCP_SLAVE_REG_DISCRETE_OUT_V(0));
	if (ret){
		SCHARGER_ERR("%s get output_vol error.\n", __func__);
		return ret;
	}


	/*retry if reset fail*/
	for (i = 0; i < FCP_RESET_RETRY_TIME; i++) {
		ret |= hi6523_fcp_adapter_reg_write(output_vol, CHG_FCP_SLAVE_VOUT_CONFIG);
		ret |= hi6523_fcp_adapter_reg_read(&val, CHG_FCP_SLAVE_VOUT_CONFIG);
		SCHARGER_INF("%s: vout config reg[0x2c] = %u.\n", __func__, val);
		if (ret || val != output_vol) {
			SCHARGER_ERR("%s: set vout config err, reg[0x2c] = %u.\n", __func__, val);
			continue;
		}

		ret |= hi6523_fcp_adapter_reg_write(CHG_FCP_SLAVE_SET_VOUT, CHG_FCP_SLAVE_OUTPUT_CONTROL);
		if (ret) {
			SCHARGER_ERR("%s: enable adapter output voltage failed.\n", __func__);
			continue;
		}
		SCHARGER_INF("%s: enable adapter output voltage succ, i = %d.\n", __func__, i);
		break;
	}

	ret |= hi6523_write_byte(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK | CHG_FCP_MSTR_RST_MSK);
	if (ret) {
		SCHARGER_ERR("%s: send rst cmd failed.\n ", __func__);
		return ret;
	}
	/*after reset, v300 need 17ms*/
	msleep(25);
	ret |= hi6523_fcp_adapter_vol_check(FCP_ADAPTER_RST_VOL);
	if (ret) {
		ret |= hi6523_write_byte(CHG_FCP_CTRL_REG, 0);	//clear fcp_en and fcp_master_rst
		SCHARGER_ERR("%s: adc check adapter output voltage failed.\n ", __func__);
		return ret;
	}

	ret |= hi6523_write_byte(CHG_FCP_CTRL_REG, 0);	//clear fcp_en and fcp_master_rst
	ret |= hi6523_config_opt_param(VBUS_VSET_5V);
	SCHARGER_INF("%s: fcp adapter output voltage reset succ.\n", __func__);
	return ret;
}

/**********************************************************
*  Function:       hi6523_stop_charge_config
*  Description:    config chip after stop charging
*  Parameters:     NULL
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_stop_charge_config(void)
{
	int ret = 0;
	ret |= hi6523_set_vbus_vset(VBUS_VSET_5V);

	is_weaksource = WEAKSOURCE_FALSE;

	return ret;
}

/**********************************************************
*  Function:       hi6523_fcp_stop_charge_config
*  Description:    fcp config chip after stop charging
*  Parameters:     NULL
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_fcp_stop_charge_config(void)
{
	SCHARGER_INF("hi6523_fcp_master_reset");
	hi6523_fcp_master_reset();
	hi6523_apple_adapter_detect(APPLE_DETECT_ENABLE);
	if(!switch_id_flag)/*when charge stop it not need change*/
	{
		hi6523_fcp_switch_to_soc();
	}
	return 0;
}
static int is_fcp_charger_type(void)
{
	u8 reg_val = 0;
	int ret = 0;

	if (hi6523_is_support_fcp()) {
		SCHARGER_ERR("%s:NOT SUPPORT FCP!\n", __func__);
		return FALSE;
	}
	ret |= hi6523_read_byte(FCP_ADAPTER_CNTL_REG, &reg_val);
	if (ret) {
		SCHARGER_ERR("%s reg read fail!\n", __func__);
		return FALSE;
	}
	if (HI6523_ACCP_CHARGER_DET == (reg_val & HI6523_ACCP_CHARGER_DET))
		return TRUE;
	return FALSE;
}
static int fcp_read_adapter_status (void)
{
    u8 val = 0;
    int ret = 0;
    ret = hi6523_fcp_adapter_reg_read(&val, FCP_ADAPTER_STATUS);
    if(ret != 0)
    {
        SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
        return 0;
    }
    SCHARGER_INF("val is %d \n",val);

    if( FCP_ADAPTER_OVLT == (val & FCP_ADAPTER_OVLT))
    {
       return FCP_ADAPTER_OVLT;
    }

    if( FCP_ADAPTER_OCURRENT == (val & FCP_ADAPTER_OCURRENT))
    {
        return FCP_ADAPTER_OCURRENT;
    }

    if( FCP_ADAPTER_OTEMP == (val & FCP_ADAPTER_OTEMP))
    {
        return FCP_ADAPTER_OTEMP;
    }
    return 0;
}

static int fcp_read_switch_status(void)
{
	return 0;
}
static void hi6523_reg_dump(char* ptr)
{
	return;
}

#ifdef CONFIG_DIRECT_CHARGER
static int hi6523_is_support_scp(void)
{
	struct hi6523_device_info *di = g_hi6523_dev;


	if(!di || !di->param_dts.scp_support)
	{
		return HI6523_FAIL;
	}
	return 0;
}
static int scp_adapter_reg_read(u8* val, u8 reg)
{
	int ret;

	if (scp_error_flag)
	{
		SCHARGER_ERR("%s : scp timeout happened ,do not read reg = %d \n",__func__,reg);
		return HI6523_FAIL;
	}
	hi6523_apple_adapter_detect(APPLE_DETECT_DISABLE);
	ret = hi6523_fcp_adapter_reg_read(val, reg);
	if (ret)
	{
		SCHARGER_ERR("%s : error reg = %d \n",__func__,reg);
		scp_error_flag = SCP_IS_ERR;
		return HI6523_FAIL;
	}
	return 0;
}
static int scp_adapter_reg_write(u8 val, u8 reg)
{
	int ret;

	if (scp_error_flag)
	{
		SCHARGER_ERR("%s : scp timeout happened ,do not write reg = %d \n",__func__,reg);
			return HI6523_FAIL;
	}
	hi6523_apple_adapter_detect(APPLE_DETECT_DISABLE);
	ret = hi6523_fcp_adapter_reg_write(val, reg);
	if (ret)
	{
		SCHARGER_ERR("%s : error reg = %d \n",__func__,reg);
		scp_error_flag = SCP_IS_ERR;
	return HI6523_FAIL;
	}
	return 0;
}

static int hi6523_scp_adaptor_vout_regval_convert(u8 reg_val)
{
	u8 iout_exp = 0;
	u8 B = 0;
	int A = 0;
	int rs = 0;

	iout_exp = (SCP_MAX_IOUT_A_MASK & reg_val) >> SCP_MAX_IOUT_A_SHIFT;
	B = SCP_MAX_IOUT_B_MASK & reg_val;
	switch (iout_exp){
	case MAX_IOUT_EXP_0:
		A = TEN_EXP_0;
		break;
	case MAX_IOUT_EXP_1:
		A = TEN_EXP_1;
		break;
	case MAX_IOUT_EXP_2:
		A = TEN_EXP_2;
		break;
	case MAX_IOUT_EXP_3:
		A = TEN_EXP_3;
		break;
	default:
		return HI6523_FAIL;
	}
	rs = B*A;
	return rs;
}

static int hi6523_scp_get_adaptor_max_voltage(void)
{
	u8 reg_val = 0;
	int ret = 0;
	int rs = 0;

	ret = scp_adapter_reg_read(&reg_val, SCP_MAX_VOUT);
	if(ret)
	{
		SCHARGER_ERR("%s : read MAX_VOUT failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}

	rs = hi6523_scp_adaptor_vout_regval_convert(reg_val);
	SCHARGER_INF("[%s]max_vout reg is 0x%x, val is %d \n", __func__, reg_val,rs);

	return rs;
}
static int hi6523_scp_get_adaptor_min_voltage(void)
{
	u8 reg_val = 0;
	int ret = 0;
	int rs = 0;

	ret = scp_adapter_reg_read(&reg_val, SCP_MIN_VOUT);
	if(ret)
	{
		SCHARGER_ERR("%s : read MIN_VOUT failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}

	rs = hi6523_scp_adaptor_vout_regval_convert(reg_val);
	SCHARGER_INF("[%s]min_vout reg is 0x%x, val is %d \n", __func__, reg_val, rs);

	return rs;
}
static int hi6523_scp_adaptor_detect(void)
{
	int ret = 0;
	u8 val = 0;
	int max_voltage = 0, min_voltage = 0;
	struct hi6523_device_info *di = g_hi6523_dev;
	scp_error_flag = SCP_NO_ERR;

	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}

	di->adaptor_support = 0;

	ret = hi6523_fcp_adapter_detect();

	if (CHG_FCP_ADAPTER_DETECT_OTHER == ret)
	{
		SCHARGER_INF("scp adapter other detect\n");
		return SCP_ADAPTOR_DETECT_OTHER;
	}
	if (CHG_FCP_ADAPTER_DETECT_FAIL == ret)
	{
		SCHARGER_INF("scp adapter detect fail\n");
		return SCP_ADAPTOR_DETECT_FAIL;
	}

	ret = scp_adapter_reg_read(&val, SCP_ADP_TYPE0);
	if(ret || !(val & SCP_ADP_TYPE0_B_SC_MASK))
	{
		if (ret)
			SCHARGER_ERR("%s : read SCP_ADP_TYPE_0 fail ,ret = %d \n",__func__,ret);

		/*in case the LVC charger does not have 0x7e reg*/
		scp_error_flag = SCP_NO_ERR;
		ret = scp_adapter_reg_read(&val, SCP_ADP_TYPE);
		if(ret)
		{
			SCHARGER_ERR("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
			return SCP_ADAPTOR_DETECT_OTHER;
		}
		SCHARGER_INF("%s : read SCP_ADP_TYPE val = %d \n",__func__,val);
		if ((val & SCP_ADP_TYPE_B_MASK) == SCP_ADP_TYPE_B)
		{
			SCHARGER_INF("scp type B adapter detect\n ");
			ret = scp_adapter_reg_read(&val, SCP_B_ADP_TYPE);
			if (ret)
			{
				SCHARGER_ERR("%s : read SCP_B_ADP_TYPE fail ,ret = %d \n",__func__,ret);
				return SCP_ADAPTOR_DETECT_OTHER;/*not scp adapter*/
			}
			SCHARGER_INF("%s : read SCP_B_ADP_TYPE val = %d \n",__func__,val);
			if (SCP_B_DIRECT_ADP == val)
			{
				SCHARGER_INF("scp type B direct charge adapter detect\n ");

				max_voltage = hi6523_scp_get_adaptor_max_voltage();
				min_voltage = hi6523_scp_get_adaptor_min_voltage();

				if(min_voltage < 3700 && max_voltage > 4800) {
					di->adaptor_support |= LVC_MODE;
				}

				SCHARGER_INF("scp type B, max vol = %d, min vol = %d, support mode: 0x%x\n " ,\
					max_voltage, min_voltage, di->adaptor_support);
				return SCP_ADAPTOR_DETECT_SUCC;
			}
		}
		return SCP_ADAPTOR_DETECT_OTHER;
	}
	SCHARGER_INF("%s : read SCP_ADP_TYPE_0 val = %d \n",__func__,val);
	if (val)
	{
		if(val & SCP_ADP_TYPE0_B_SC_MASK)
		{
			SCHARGER_INF("scp type B SC adapter detect\n ");
			di->adaptor_support |= SC_MODE;
		}

		if(!(val & SCP_ADP_TYPE0_B_LVC_MASK))
		{
			SCHARGER_INF("scp type B lvc adapter detect\n ");
			di->adaptor_support |= LVC_MODE;
		}

		SCHARGER_INF("%s : adaptor_support = %d \n",__func__, di->adaptor_support);
		ret = scp_adapter_reg_read(&val, SCP_B_ADP_TYPE);
		if (ret)
		{
			SCHARGER_ERR("%s : read SCP_B_ADP_TYPE fail ,ret = %d \n",__func__,ret);
			return SCP_ADAPTOR_DETECT_OTHER;/*not scp adapter*/
		}
		SCHARGER_INF("%s : read SCP_B_ADP_TYPE val = %d \n",__func__,val);
		if (SCP_B_DIRECT_ADP == val)
		{
			SCHARGER_INF("scp type B direct charge adapter detect\n ");
			return SCP_ADAPTOR_DETECT_SUCC;
		}
	}

	return SCP_ADAPTOR_DETECT_OTHER;
}

static int hi6523_scp_get_adaptor_type(void)
{
	struct hi6523_device_info *di = g_hi6523_dev;
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}

	SCHARGER_INF("%s : adaptor_support = 0x%x \n ",__func__, di->adaptor_support);
	return (int)di->adaptor_support;
}

static int hi6523_scp_output_mode_enable(int enable)
{
	u8 val;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
	val &= ~(SCP_OUTPUT_MODE_MASK);
	val |= enable ? SCP_OUTPUT_MODE_ENABLE:SCP_OUTPUT_MODE_DISABLE;
	SCHARGER_INF("[%s]val after is %d \n", __func__, val);
	ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
	if(ret < 0)
	{
		SCHARGER_ERR("%s : failed \n ",__func__);
		return HI6523_FAIL;
	}
	return 0;
}

static int hi6523_scp_adaptor_output_enable(int enable)
{
	u8 val;
	int ret;

	ret = hi6523_scp_output_mode_enable(OUTPUT_MODE_ENABLE);
	if(ret)
	{
		SCHARGER_ERR("%s : scp output mode enable failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}

	ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
	val &= ~(SCP_OUTPUT_MASK);
	val |= enable ? SCP_OUTPUT_ENABLE:SCP_OUTPUT_DISABLE;
	SCHARGER_INF("[%s]val after is %d \n", __func__, val);
	ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
	if(ret < 0)
	{
		SCHARGER_ERR("%s : failed \n ",__func__);
		return HI6523_FAIL;
	}
	return 0;
}
static int hi6523_adaptor_reset(int enable)
{
	u8 val;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
	val &= ~(SCP_ADAPTOR_RESET_MASK);
	val |= enable ? SCP_ADAPTOR_RESET_ENABLE:SCP_ADAPTOR_RESET_DISABLE;
	SCHARGER_INF("[%s]val after is %d \n", __func__, val);
	ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
	if(ret < 0)
	{
		SCHARGER_ERR("%s : failed \n ",__func__);
		return HI6523_FAIL;
	}
	return 0;
}
static int hi6523_scp_config_iset_boundary(int iboundary)
{
	u8 val;
	int ret;

	/*high byte store in low address*/
	val = (iboundary >> ONE_BYTE_LEN) & ONE_BYTE_MASK;
	ret = scp_adapter_reg_write(val, SCP_ISET_BOUNDARY_L);
	if (ret)
		return ret;
	/*low byte store in high address*/
	val = iboundary & ONE_BYTE_MASK;
	ret |= scp_adapter_reg_write(val, SCP_ISET_BOUNDARY_H);
	if (ret < 0)
	{
		SCHARGER_ERR("%s : failed \n ",__func__);
	}
	return ret;
}
static int hi6523_scp_config_vset_boundary(int vboundary)
{
	u8 val;
	int ret;

	/*high byte store in low address*/
	val = (vboundary >> ONE_BYTE_LEN) & ONE_BYTE_MASK;
	ret = scp_adapter_reg_write(val, SCP_VSET_BOUNDARY_L);
	if (ret)
		return ret;
	/*low byte store in high address*/
	val = vboundary & ONE_BYTE_MASK;
	ret |= scp_adapter_reg_write(val, SCP_VSET_BOUNDARY_H);
	if(ret < 0)
	{
		SCHARGER_ERR("%s : failed \n ",__func__);
	}
	return ret;
}
static int hi6523_scp_set_adaptor_voltage(int vol)
{
	int val = 0;
	int ret = 0;
	int dc_mode = 0;
	u8 reg = 0;

	dc_mode= scp_get_direct_charge_mode();

	if (dc_mode == LVC_MODE)
	{
		val = vol - VSSET_OFFSET;
		val = val / VSSET_STEP;
		ret = scp_adapter_reg_write((u8)val, SCP_VSSET);
		if(ret < 0)
		{
			SCHARGER_ERR("%s : failed \n ",__func__);
			return HI6523_FAIL;
		}
	}
	else if (dc_mode == SC_MODE)
	{
		/*high byte store in low address*/
		reg = (vol >> ONE_BYTE_LEN) & ONE_BYTE_MASK;
		ret = scp_adapter_reg_write(reg, SCP_VSET_L);
		/*low byte store in high address*/
		reg = vol & ONE_BYTE_MASK;
		ret |= scp_adapter_reg_write(reg, SCP_VSET_H);
		if(ret < 0)
		{
			SCHARGER_ERR("%s : failed \n ",__func__);
			return HI6523_FAIL;
		}
	}

	return 0;
}
static int hi6523_scp_set_watchdog_timer(int second)
{
	u8 val;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
	val &= ~(SCP_WATCHDOG_MASK);
	val |= (second * ONE_BIT_EQUAL_TWO_SECONDS) & SCP_WATCHDOG_MASK; /*1 bit means 0.5 second*//*lint !e647 */
	SCHARGER_INF("[%s]val after is %d \n", __func__, val);
	ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE1);
	if(ret < 0)
	{
		SCHARGER_ERR("%s : failed \n ",__func__);
		return HI6523_FAIL;
	}
	return 0;
}
static int hi6523_scp_init(struct scp_init_data * sid)
{
	/*open 5v boost*/
	int ret;
	u8 val;

	scp_error_flag = SCP_NO_ERR;
	ret = hi6523_scp_output_mode_enable(sid->scp_mode_enable);
	if(ret)
		return ret;
	ret = hi6523_scp_config_vset_boundary(sid->vset_boundary);
	if(ret)
		return ret;
	ret = hi6523_scp_config_iset_boundary(sid->iset_boundary);
	if(ret)
		return ret;
	ret = hi6523_scp_set_adaptor_voltage(sid->init_adaptor_voltage);
	if(ret)
		return ret;
	ret = hi6523_scp_set_watchdog_timer(sid->watchdog_timer);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
	if(ret)
		return ret;
	SCHARGER_INF("%s : CTRL_BYTE0 = 0x%x \n ",__func__, val);
	ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
	if(ret)
		return ret;
	SCHARGER_INF("%s : CTRL_BYTE1 = 0x%x \n ",__func__, val);
	ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
	if(ret)
		return ret;
	SCHARGER_INF("%s : STATUS_BYTE0 = 0x%x \n ",__func__, val);
	ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE1);
	if(ret)
		return ret;
	SCHARGER_INF("%s : STATUS_BYTE1 = 0x%x \n ",__func__, val);
	ret = scp_adapter_reg_read(&val, SCP_VSET_BOUNDARY_H);
	if(ret)
		return ret;
	SCHARGER_INF("%s : VSET_BOUNDARY_H = 0x%x \n ",__func__, val);
	ret = scp_adapter_reg_read(&val, SCP_VSET_BOUNDARY_L);
	if(ret)
		return ret;
	SCHARGER_INF("%s : VSET_BOUNDARY_L = 0x%x \n ",__func__, val);
	ret = scp_adapter_reg_read(&val, SCP_ISET_BOUNDARY_H);
	if(ret)
		return ret;
	SCHARGER_INF("%s : ISET_BOUNDARY_H = 0x%x \n ",__func__, val);
	ret = scp_adapter_reg_read(&val, SCP_ISET_BOUNDARY_L);
	if(ret)
		return ret;
	SCHARGER_INF("%s : ISET_BOUNDARY_L = 0x%x \n ",__func__, val);
	return ret;
}
static int hi6523_scp_chip_reset(void)
{
	int ret;

	ret = hi6523_fcp_master_reset();
	if(ret)
	{
		SCHARGER_INF("%s:hi6523_fcp_master_reset fail!\n ",__func__);
		return HI6523_FAIL;
	}
	return 0;
}
static int hi6523_scp_exit(struct direct_charge_device* di)
{
	int ret;

	ret = hi6523_scp_output_mode_enable(OUTPUT_MODE_DISABLE);
	switch(di->adaptor_vendor_id)
	{
		case IWATT_ADAPTER:
			ret  |= hi6523_adaptor_reset(ADAPTOR_RESET);
			break;
		default:
			SCHARGER_INF("%s:not iWatt\n",__func__);
	}
	msleep(WAIT_FOR_ADAPTOR_RESET);
	SCHARGER_ERR("%s\n",__func__);
	scp_error_flag = SCP_NO_ERR;
	return ret;
}
static int hi6523_scp_get_adaptor_voltage(void)
{
	u8  reg_val = 0;
	u8  reg_low = 0;
	u8  reg_high = 0;
	int val = 0;
	int ret= 0;
	int dc_mode = 0;

	dc_mode= scp_get_direct_charge_mode();
	if (dc_mode == LVC_MODE)
	{
		ret = scp_adapter_reg_read(&reg_val, SCP_SREAD_VOUT);
		if(ret)
		{
			SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
			return HI6523_FAIL;
		}
		val = reg_val*SCP_SREAD_VOUT_STEP + SCP_SREAD_VOUT_OFFSET;
		SCHARGER_INF("[%s]val is %d \n", __func__, val);
		return val;
	}
	else if (dc_mode == SC_MODE)
	{
		ret = scp_adapter_reg_read(&reg_low, SCP_READ_VOLT_L);
		ret |= scp_adapter_reg_read(&reg_high, SCP_READ_VOLT_H);
		if(ret)
		{
			SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
			return HI6523_FAIL;
		}
		val = reg_low << ONE_BYTE_LEN;
		val += reg_high;

		SCHARGER_INF("[%s]val is %d \n", __func__, val);
		return val;
	}
        return HI6523_FAIL;
}
static int hi6523_scp_set_adaptor_current(int cur)
{
	u8 val;
	int ret;

	val = cur / ISSET_STEP;
	ret = scp_adapter_reg_write(val, SCP_ISSET);
	if(ret < 0)
	{
		SCHARGER_ERR("%s : failed \n ",__func__);
		return HI6523_FAIL;
	}
	return 0;
}
static int hi6523_scp_get_adaptor_current(void)
{
	int val;
	u8 reg_val;
	int ret;

	ret = scp_adapter_reg_read(&reg_val, SCP_SREAD_IOUT);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	val = reg_val*SCP_SREAD_IOUT_STEP;
	SCHARGER_INF("[%s]val is %d \n", __func__, val);
	return val;
}
static int hi6523_scp_get_adaptor_current_set(void)
{
	int val;
	u8 reg_val;
	int ret;

	ret = scp_adapter_reg_read(&reg_val, SCP_ISSET);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	val = reg_val*ISSET_STEP;
	SCHARGER_INF("[%s]val is %d \n", __func__, val);
	return val;
}
static int hi6523_scp_get_adaptor_max_current(void)
{
	u8 reg_val;
	int ret;
	int A;
	int B;
	int rs;

	ret = scp_adapter_reg_read(&reg_val, SCP_MAX_IOUT);
	if(ret)
	{
		SCHARGER_ERR("%s : read MAX_IOUT failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]max_iout reg is %d \n", __func__, reg_val);
	A = (SCP_MAX_IOUT_A_MASK & reg_val) >> SCP_MAX_IOUT_A_SHIFT;
	B = SCP_MAX_IOUT_B_MASK & reg_val;
	switch (A){
	case MAX_IOUT_EXP_0:
		A = TEN_EXP_0;
		break;
	case MAX_IOUT_EXP_1:
		A = TEN_EXP_1;
		break;
	case MAX_IOUT_EXP_2:
		A = TEN_EXP_2;
		break;
	case MAX_IOUT_EXP_3:
		A = TEN_EXP_3;
		break;
	default:
		return HI6523_FAIL;
	}
	rs = B*A;
	SCHARGER_INF("[%s]MAX IOUT initial is %d \n", __func__, rs);
	ret = scp_adapter_reg_read(&reg_val, SCP_SSTS);
	if(ret)
	{
		SCHARGER_ERR("%s : read SSTS failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]ssts reg is %d \n", __func__, reg_val);
	B = (SCP_SSTS_B_MASK & reg_val) >> SCP_SSTS_B_SHIFT;
	A = SCP_SSTS_A_MASK & reg_val;
	if (DROP_POWER_FLAG == B)
	{
		rs = rs * A / DROP_POWER_FACTOR;
	}
	SCHARGER_INF("[%s]MAX IOUT final is %d \n", __func__, rs);
	return rs;
}

static int hi6523_scp_get_adaptor_temp(int* temp)
{
	u8 val = 0;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_INSIDE_TMP);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]val is %d \n", __func__, val);
	*temp = val;

	return 0;
}
static int hi6523_scp_cable_detect(void)
{
	u8 val;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return SCP_CABLE_DETECT_ERROR;
	}
	SCHARGER_INF("[%s]val is %d \n", __func__, val);
	if (val & SCP_CABLE_STS_MASK)
	{
		return SCP_CABLE_DETECT_SUCC;
	}
	return SCP_CABLE_DETECT_FAIL;
}
static int hi6523_scp_adaptor_reset(void)
{
	return hi6523_fcp_adapter_reset();
}
static int hi6523_scp_stop_charge_config(void)
{
	return 0;
}
static int hi6523_is_scp_charger_type(void)
{
	return is_fcp_charger_type();
}
static int hi6523_scp_get_adaptor_status(void)
{
	return 0;
}
static int hi6523_scp_get_adaptor_info(void* info)
{
	int ret;
	struct adaptor_info* p = (struct adaptor_info*)info;

	ret = scp_adapter_reg_read((u8 *)(&(p->b_adp_type)), SCP_B_ADP_TYPE);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->vendor_id_h)), SCP_VENDOR_ID_H);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->vendor_id_l)), SCP_VENDOR_ID_L);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->module_id_h)), SCP_MODULE_ID_H);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->module_id_l)), SCP_MODULE_ID_L);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->serrial_no_h)), SCP_SERRIAL_NO_H);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->serrial_no_l)), SCP_SERRIAL_NO_L);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->pchip_id)), SCP_PCHIP_ID);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->hwver)), SCP_HWVER);
	if(ret)
		return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->fwver_h)), SCP_FWVER_H);
	if(ret)
	return ret;
	ret = scp_adapter_reg_read((u8 *)(&(p->fwver_l)), SCP_FWVER_L);

	return ret;
}
static int hi6523_get_adapter_vendor_id(void)
{
	u8 val = 0;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_PCHIP_ID);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]val is 0x%x \n", __func__, val);
	switch (val)
	{
		case VENDOR_ID_RICHTEK:
			SCHARGER_INF("[%s]adapter is richtek \n", __func__);
			return RICHTEK_ADAPTER;
		case VENDOR_ID_IWATT:
			SCHARGER_INF("[%s]adapter is iwatt \n", __func__);
			return IWATT_ADAPTER;
		default:
			SCHARGER_INF("[%s]this adaptor vendor id is not found!\n", __func__);
			return val;
	}
}
static int hi6523_get_usb_port_leakage_current_info(void)
{
	u8 val = 0;
	int ret;

	ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
	if(ret)
	{
		SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
		return HI6523_FAIL;
	}
	SCHARGER_INF("[%s]val is 0x%x \n", __func__, val);
	val &= SCP_PORT_LEAKAGE_INFO;
	val = val>>SCP_PORT_LEAKAGE_SHIFT;
	SCHARGER_INF("[%s]val is 0x%x \n", __func__, val);
	return val;
}
static int hi6523_scp_get_chip_status(void)
{
	return 0;
}

static int hi6523_scp_set_adaptor_encrypt_enable(int type)
{
	int ret = 0;

	ret = scp_adapter_reg_write(type, SCP_ADAPTOR_KEY_INDEX_REG);
	if (ret) {
		SCHARGER_ERR("%s : scp_adapter_reg_write  SCP_ADAPTOR_KEY_INDEX_REG failed, value = %02X\n", __func__, type);
		return HI6523_FAIL;
	}

	return 0;
}

static int hi6523_scp_get_adaptor_encrypt_enable(void)
{
	int ret = 0;
	u8 val = 0;

	ret = scp_adapter_reg_read(&val, SCP_ADAPTOR_ENCRYPT_INFO_REG);
	if (ret) {
		SCHARGER_ERR("%s :scp_adapter_reg_read SCP_ADAPTOR_ENCRYPT_INFO_REG failed\n", __func__);
		return HI6523_FAIL;
	}

	if (!(val & SCP_ADAPTOR_ENCRYPT_ENABLE)) {
		SCHARGER_ERR("%s : SCP_ADAPTOR_ENCRYPT_ENABLE val = %d\n", __func__, val);
		return HI6523_FAIL;
	}

	return 0;
}

static int hi6523_scp_set_adaptor_random_num(char *random_local)
{
	int ret = 0;
	u8 j = 0;

	for (j = 0; j < SCP_ADAPTOR_RANDOM_NUM_HI_LEN; j++)
	{
		ret = scp_adapter_reg_write(random_local[j], SCP_ADAPTOR_RANDOM_NUM_HI_BASE_REG + j);
		if (ret) {
			SCHARGER_ERR("%s : random_local[%d] = %d\n", __func__, j, random_local[j]);
			return HI6523_FAIL;
		}
	}

	return 0;
}

static int hi6523_scp_get_adaptor_encrypt_completed(void)
{
	int ret = 0;
	u8 val = 0;

	ret = scp_adapter_reg_read(&val, SCP_ADAPTOR_ENCRYPT_INFO_REG);
	if (ret) {
		SCHARGER_ERR("%s : scp_adapter_reg_read  SCP_ADAPTOR_ENCRYPT_INFO_REG failed\n", __func__);
		return HI6523_FAIL;
	}

	if( !(val & SCP_ADAPTOR_ENCRYPT_COMPLETE)) {
		SCHARGER_ERR("%s : SCP_ADAPTOR_ENCRYPT_COMPLETE val = %d\n", __func__, val);
		return HI6523_FAIL;
	}

	return 0;
}

static int hi6523_scp_get_adaptor_random_num(char *num)
{
	int ret = 0;
	u8 j = 0;
	u8 val = 0;

	(void)memset_s(scaf_randnum_remote, SCP_ADAPTOR_RANDOM_NUM_HI_LEN, 0, SCP_ADAPTOR_RANDOM_NUM_HI_LEN);
	for (j = 0; j < SCP_ADAPTOR_RANDOM_NUM_HI_LEN; j++)
	{
		ret = scp_adapter_reg_read(&val, SCP_ADAPTOR_RANDOM_NUM_LO_BASE_REG + j);
		if (ret) {
			SCHARGER_ERR("%s :scp_adapter_reg_read SCP_ADAPTOR_RANDOM_NUM_LO_BASE_REG  %d failed\n", __func__, j);
			return HI6523_FAIL;
		}
		else {
			scaf_randnum_remote[j] = val;
		}
	}

	memcpy_s(num, SCP_ADAPTOR_RANDOM_NUM_HI_LEN, scaf_randnum_remote, SCP_ADAPTOR_RANDOM_NUM_HI_LEN);
	return 0;
}

static int hi6523_scp_get_adaptor_encrypted_value(char *hash)
{
	int ret = 0;
	u8 j = 0;
	u8 val = 0;

	(void)memset_s(scaf_digest_remote_hi, sizeof(scaf_digest_remote_hi), 0, sizeof(scaf_digest_remote_hi));
	for (j = 0; j < SCP_ADAPTOR_DIGEST_LEN; j++)
	{
		ret = scp_adapter_reg_read(&val, SCP_ADAPTOR_DIGEST_BASE_REG + j);
		if (ret) {
			SCHARGER_ERR("%s :scp_adapter_reg_read SCP_ADAPTOR_DIGEST_BASE_REG %d failed\n", __func__, j);
			return HI6523_FAIL;
		}
		else {
			scaf_digest_remote_hi[j] = val;
		}
	}

	memcpy_s(hash, SCP_ADAPTOR_DIGEST_LEN, scaf_digest_remote_hi, SCP_ADAPTOR_DIGEST_LEN);
	return 0;
}

struct smart_charge_ops scp_hi6523_ops = {
	.is_support_scp = hi6523_is_support_scp,
	.scp_init = hi6523_scp_init,
	.scp_exit = hi6523_scp_exit,
	.scp_adaptor_detect = hi6523_scp_adaptor_detect,
	.scp_set_adaptor_voltage = hi6523_scp_set_adaptor_voltage,
	.scp_get_adaptor_voltage = hi6523_scp_get_adaptor_voltage,
	.scp_set_adaptor_current = hi6523_scp_set_adaptor_current,
	.scp_get_adaptor_current = hi6523_scp_get_adaptor_current,
	.scp_get_adaptor_current_set = hi6523_scp_get_adaptor_current_set,
	.scp_get_adaptor_max_current = hi6523_scp_get_adaptor_max_current,
	.scp_adaptor_reset = hi6523_scp_adaptor_reset,
	.scp_adaptor_output_enable = hi6523_scp_adaptor_output_enable,
	.scp_chip_reset = hi6523_scp_chip_reset,
	.scp_stop_charge_config = hi6523_scp_stop_charge_config,
	.is_scp_charger_type = hi6523_is_scp_charger_type,
	.scp_get_adaptor_status = hi6523_scp_get_adaptor_status,
	.scp_get_adaptor_info = hi6523_scp_get_adaptor_info,
	.scp_get_chip_status = hi6523_scp_get_chip_status,
	.scp_cable_detect = hi6523_scp_cable_detect,
	.scp_get_adaptor_temp = hi6523_scp_get_adaptor_temp,
	.scp_get_adapter_vendor_id = hi6523_get_adapter_vendor_id,
	.scp_get_usb_port_leakage_current_info = hi6523_get_usb_port_leakage_current_info,
	.scp_set_direct_charge_mode = scp_set_direct_charge_mode,
	.scp_get_adaptor_type = hi6523_scp_get_adaptor_type,
	.scp_set_adaptor_encrypt_enable = hi6523_scp_set_adaptor_encrypt_enable,
	.scp_get_adaptor_encrypt_enable = hi6523_scp_get_adaptor_encrypt_enable,
	.scp_set_adaptor_random_num = hi6523_scp_set_adaptor_random_num,
	.scp_get_adaptor_encrypt_completed = hi6523_scp_get_adaptor_encrypt_completed,
	.scp_get_adaptor_random_num = hi6523_scp_get_adaptor_random_num,
	.scp_get_adaptor_encrypted_value = hi6523_scp_get_adaptor_encrypted_value,
};
#endif

struct fcp_adapter_device_ops fcp_hi6523_ops = {
	.get_adapter_output_current = hi6523_fcp_get_adapter_output_current,
	.set_adapter_output_vol = hi6523_fcp_set_adapter_output_vol,
	.detect_adapter = fcp_adapter_detect,
	.is_support_fcp = hi6523_is_support_fcp,
	.switch_chip_reset = hi6523_fcp_master_reset,
	.fcp_adapter_reset = hi6523_fcp_adapter_reset,
	.stop_charge_config = hi6523_fcp_stop_charge_config,
	.is_fcp_charger_type    = is_fcp_charger_type,
	.fcp_read_adapter_status = fcp_read_adapter_status,
	.fcp_read_switch_status = fcp_read_switch_status,
	.reg_dump = hi6523_reg_dump,
};

struct charge_device_ops hi6523_ops = {
	.chip_init = hi6523_chip_init,
	.dev_check = hi6523_device_check,
	.set_input_current = hi6523_set_input_current,
	.set_charge_current = hi6523_set_charge_current,
	.set_terminal_voltage = hi6523_set_terminal_voltage,
	.set_dpm_voltage = hi6523_set_dpm_voltage,
	.set_terminal_current = hi6523_set_terminal_current,
	.set_charge_enable = hi6523_set_charge_enable,
	.set_otg_enable = hi6523_set_otg_enable,
	.set_term_enable = hi6523_set_term_enable,
	.get_charge_state = hi6523_get_charge_state,
	.reset_watchdog_timer = hi6523_reset_watchdog_timer,
	.dump_register = hi6523_dump_register,
	.get_register_head = hi6523_get_register_head,
	.set_watchdog_timer = hi6523_set_watchdog_timer,
	.set_batfet_disable = hi6523_set_batfet_disable,
	.get_ibus = hi6523_get_ibus_ma,
	.get_vbus = hi6523_get_vbus_mv,
	.get_vbat_sys = hi6523_get_vbat_sys,
	.set_charger_hiz = hi6523_set_charger_hiz,
	.check_input_dpm_state = hi6523_check_input_dpm_state,
	.check_input_vdpm_state = hi6523_check_input_dpm_state,
	.check_input_idpm_state = hi6523_check_input_acl_state,
	.set_otg_current = hi6523_set_otg_current,
	.stop_charge_config = hi6523_stop_charge_config,
	.set_vbus_vset = hi6523_set_vbus_vset,
	.set_uvp_ovp = hi6523_set_uvp_ovp,
	.set_force_term_enable = hi6523_force_set_term_enable,
	.get_charger_state = hi6523_get_charger_state,
	.soft_vbatt_ovp_protect = hi6523_soft_vbatt_ovp_protect,
	.rboost_buck_limit = hi6523_rboost_buck_limit,
	.get_charge_current = hi6523_get_charge_current,
	.get_iin_set = hi6523_get_input_current_set,
};

struct  water_detect_ops hi6523_water_detect_ops = {
	.is_water_intrused = hi6523_is_water_intrused,
};
/**********************************************************
*  Function:       hi6523_plugout_check_process
*  Description:    schedule or cancel check work based on charger type
*                  USB/NON_STD/BC_USB: schedule work
*                  REMOVED: cancel work
*  Parameters:     type: charger type
*  return value:   NULL
**********************************************************/
static void hi6523_plugout_check_process(enum hisi_charger_type type)
{
	struct hi6523_device_info *di = g_hi6523_dev;

	if (NULL == di)
		return;
	switch (type) {
	case CHARGER_TYPE_SDP:
	case CHARGER_TYPE_DCP:
	case CHARGER_TYPE_CDP:
	case CHARGER_TYPE_UNKNOWN:
		queue_delayed_work(system_power_efficient_wq, &di->plugout_check_work,
				      msecs_to_jiffies(0));
		break;
	case CHARGER_TYPE_NONE:
		cancel_delayed_work_sync(&di->plugout_check_work);
		g_vbat_check_cycle = 0;
		g_batt_ovp_cnt_1s = 0;
		g_batt_ovp_cnt_30s = 0;
		g_rboost_cnt = 0;
	    mutex_lock(&hi6523_ibias_calc_lock);
		I_bias_all = 0;
	    mutex_unlock(&hi6523_ibias_calc_lock);
        clr_boot_weaksource_flag();
		break;
	default:
		break;
	}
}

/**********************************************************
*  Function:       hi6523_usb_notifier_call
*  Description:    respond the charger_type events from USB PHY
*  Parameters:   usb_nb:usb notifier_block
*                      event:charger type event name
*                      data:unused
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int hi6523_usb_notifier_call(struct notifier_block *usb_nb,
				    unsigned long event, void *data)
{
	struct hi6523_device_info *di = g_hi6523_dev;

	if(NULL == di){
		SCHARGER_ERR("%s : di is NULL!\n",__func__);
		return NOTIFY_OK;
	}

	di->charger_type = (enum hisi_charger_type)event;
	hi6523_plugout_check_process(di->charger_type);
	return NOTIFY_OK;
}

/**********************************************************
*  Function:       hi6523_plugout_check_work
*  Description:    handler for chargerIC plugout detect failure check
*  Parameters:     work: hi6523 plugout check workqueue
*  return value:   NULL
**********************************************************/
static void hi6523_plugout_check_work(struct work_struct *work)
{
	int ibus;
	int vbatt_mv, vbatt_max;
	int vbus = 0;
	int ret;
	unsigned char rboost_irq_reg = 0;
	struct hi6523_device_info *di = g_hi6523_dev;

	if (NULL == di)
		return;
	hi6523_read_mask(CHG_IRQ2, CHG_RBOOST_IRQ_MSK, CHG_RBOOST_IRQ_SHIFT, &rboost_irq_reg);
	if (CHG_RBOOST_IRQ == rboost_irq_reg) {
		g_rboost_cnt++;
		hi6523_write_mask(CHG_IRQ2, CHG_RBOOST_IRQ_MSK, CHG_RBOOST_IRQ_SHIFT, CHG_RBOOST_IRQ);
	}
	vbatt_mv = hisi_battery_voltage();
	vbatt_max = hisi_battery_vbat_max();
	g_vbat_check_cycle++;
	if (0 == (g_vbat_check_cycle % CHG_VBATT_CHK_CNT)) {
		g_vbat_check_cycle = 0;
		if (vbatt_mv >= MIN(CHG_VBATT_SOFT_OVP_MAX, CHG_VBATT_CV_103(vbatt_max))) {
			g_batt_ovp_cnt_1s++;
			if (CHG_VBATT_SOFT_OVP_CNT <= g_batt_ovp_cnt_1s) {
				hi6523_set_charger_hiz(TRUE);
				SCHARGER_ERR("%s:vbat:%d,cv_mv:%d,ovp_cnt:%d,shutdown buck.\n",
					__func__, vbatt_mv, di->term_vol_mv, g_batt_ovp_cnt_1s);
				g_batt_ovp_cnt_1s = 0;
				}
		} else
			g_batt_ovp_cnt_1s = 0;
	}
	if (!((TRUE == hi6523_get_anti_reverbst_enabled()) && (vbatt_mv < CHG_DPM_VOL_4000_MV))) {
		ret = hi6523_get_vbus_mv((unsigned int *)&vbus);
		if (0 == ret) {
			if (TRUE == hi6523_vbus_in_dpm(vbus)) {
				ibus = hi6523_get_ibus_ma();
				if ((ibus < plugout_check_ibus_ma) && FALSE == hi6523_is_charger_hiz()) {
					if (MAX_RBOOST_CNT > g_rboost_cnt) {
						hi6523_set_charger_hiz(TRUE);
						msleep(50);
						hi6523_set_charger_hiz(FALSE);
						g_rboost_cnt++;
						SCHARGER_INF("%s:ibus=%dma, vbus=%dmv, vbat=%dmv, r-boost cnt:%d\n",
							__func__, ibus, vbus, vbatt_mv, g_rboost_cnt);
					} else {
						SCHARGER_ERR("%s:ibus=%dma, vbus=%dmv, vbat=%dmv, r-boost cnt:%d, shutdown buck.\n",
							__func__, ibus, vbus, vbatt_mv, g_rboost_cnt);
						hi6523_set_charger_hiz(TRUE);
                        set_boot_weaksource_flag();
					}
				}
			}
		}
	}
	queue_delayed_work(system_power_efficient_wq, &di->plugout_check_work,
			      msecs_to_jiffies(plugout_check_delay_ms));
}

/**********************************************************
*  Function:       hi6523_mask_irq
*  Description:    mask chargerIC fault irq in irq bottom process
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void hi6523_mask_irq(void)
{
	hi6523_write_byte(CHG_IRQ0_MASK_ADDR, CHG_IRQ_MASK_ALL);
	hi6523_write_byte(CHG_IRQ1_MASK_ADDR, CHG_IRQ_MASK_ALL);
	hi6523_write_byte(CHG_IRQ2_MASK_ADDR, CHG_IRQ_MASK_ALL);
	hi6523_write_byte(CHG_WDT_IRQ_MASK_ADDR, CHG_IRQ_MASK_ALL);
}

/**********************************************************
*  Function:       hi6523_unmask_irq
*  Description:    unmask chargerIC irq in irq bottom process
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void hi6523_unmask_irq(void)
{
	hi6523_write_byte(CHG_IRQ0_MASK_ADDR, CHG_IRQ0_MASK_DEFAULT);
	hi6523_write_byte(CHG_IRQ1_MASK_ADDR, CHG_IRQ1_MASK_DEFAULT);
	hi6523_write_byte(CHG_IRQ2_MASK_ADDR, CHG_IRQ2_MASK_DEFAULT);
	hi6523_write_byte(CHG_WDT_IRQ_MASK_ADDR, CHG_WDT_IRQ_DEFAULT);
}

/**********************************************************
*  Function:       hi6523_init_irq_status
*  Description:    inti irq status and configuration
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void hi6523_init_irq_status(void)
{
	u8 reg0 = 0;
	u8 reg1 = 0;
	u8 reg2 = 0;
	u8 reg_wdt = 0;
    /*FCP_IRQ3 FCP_IRQ4 are default to be disabled*/
	hi6523_write_byte(CHG_FCP_ISR1_MASK_ADDR, CHG_IRQ_MASK_ALL);
	hi6523_write_byte(CHG_FCP_ISR2_MASK_ADDR, CHG_IRQ_MASK_ALL);
	hi6523_write_byte(CHG_FCP_IRQ5_MASK_ADDR, CHG_IRQ_MASK_ALL);
	/*get irq status from boot*/
	hi6523_read_byte(CHG_IRQ0, &reg0);
	hi6523_read_byte(CHG_IRQ1, &reg1);
	hi6523_read_byte(CHG_IRQ2, &reg2);
	hi6523_read_byte(CHG_WDT_IRQ, &reg_wdt);
	SCHARGER_INF("%s : CHG_IRQ0:0x%x,CHG_IRQ1:0x%x,CHG_IRQ2:0x%x,CHG_WDT_IRQ:0x%x\n", __func__,
	                reg0, reg1, reg2, reg_wdt);
	/*clear irqs on boot after we got them*/
	hi6523_write_byte(CHG_IRQ0, 0xff);
	hi6523_write_byte(CHG_IRQ1, 0xff);
	hi6523_write_byte(CHG_IRQ2, 0xff);
	hi6523_write_byte(CHG_WDT_IRQ, 0xff);
}
/**********************************************************
*  Function:       hi6523_irq_work
*  Description:    handler for chargerIC fault irq in charging process
*  Parameters:   work:chargerIC fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void hi6523_irq_work(struct work_struct *work)
{
	struct hi6523_device_info *di =
	    container_of(work, struct hi6523_device_info, irq_work);
	u8 reg0 = 0, reg1 = 0, reg2 = 0,reg_s1 = 0;
	u8 reg_wdt = 0;
	u8 reg_read = 0;
	static u8 otg_scp_cnt;
	static u8 otg_uvp_cnt;
	u8 vbat_ovp_cnt = 0;/*lint !e64*/
	int i = 0;

	hi6523_mask_irq();
	hi6523_read_byte(CHG_IRQ0, &reg0);
	hi6523_read_byte(CHG_IRQ1, &reg1);
	hi6523_read_byte(CHG_IRQ2, &reg2);
	hi6523_read_byte(CHG_WDT_IRQ, &reg_wdt);

	if (reg0) {
		SCHARGER_INF("%s : CHG_IRQ0, 0x%x\n", __func__, reg0);
		if (CHG_OTG_OCP == (reg0 & CHG_OTG_OCP))
			SCHARGER_ERR("%s : CHARGE_FAULT_OTG_OCP\n", __func__);
		if (CHG_OTG_SCP == (reg0 & CHG_OTG_SCP)) {
			otg_scp_cnt++;
			hi6523_write_byte(CHG_IRQ0, CHG_OTG_SCP);
			msleep(600);
			hi6523_read_byte(CHG_STATUS0_REG, &reg_read);
			if ((reg_read & HI6523_CHG_OTG_ON) == HI6523_CHG_OTG_ON)
				otg_scp_cnt = 0;
			if (otg_scp_cnt > 3) {
				otg_scp_cnt = 0;
				SCHARGER_ERR("%s : CHARGE_FAULT_OTG_SCP\n",
					     __func__);
				atomic_notifier_call_chain(&fault_notifier_list,
					   CHARGE_FAULT_BOOST_OCP,
					   NULL);
			}
		}
		if (CHG_OTG_UVP == (reg0 & CHG_OTG_UVP)) {
			otg_uvp_cnt++;
			hi6523_write_byte(CHG_IRQ0, CHG_OTG_UVP);
			msleep(600);
			hi6523_read_byte(CHG_STATUS0_REG, &reg_read);
			if ((reg_read & HI6523_CHG_OTG_ON) == HI6523_CHG_OTG_ON)
				otg_uvp_cnt = 0;
			if (otg_uvp_cnt > 3) {
				otg_uvp_cnt = 0;
				SCHARGER_ERR("%s : CHARGE_FAULT_OTG_UVP\n",
					     __func__);
				atomic_notifier_call_chain(&fault_notifier_list,
					   CHARGE_FAULT_BOOST_OCP,
					   NULL);
			}
		}
	}

	if (reg1) {
		SCHARGER_INF("%s : CHG_IRQ1, 0x%x\n", __func__, reg1);
		if (CHG_VBAT_OVP == (reg1 & CHG_VBAT_OVP)) {
			for (i = 0; i < 5; i++) {
				hi6523_read_byte(CHG_IRQ_STATUS1, &reg_s1);
				if (CHG_VBAT_OVP == (reg_s1 & CHG_VBAT_OVP)) {
					vbat_ovp_cnt++;
					msleep(5);
				} else {
					vbat_ovp_cnt = 0;
					break;
				}
			}
			hi6523_write_byte(CHG_IRQ1, CHG_VBAT_OVP);
			if (vbat_ovp_cnt >= 5) {
				SCHARGER_ERR("%s : CHARGE_FAULT_VBAT_OVP\n",
					     __func__);
				atomic_notifier_call_chain(&fault_notifier_list,
					   CHARGE_FAULT_VBAT_OVP,
					   NULL);
			}
		}
	}

	if (reg2) {
		SCHARGER_INF("%s : CHG_IRQ2, 0x%x\n", __func__, reg2);
	}

	if (reg_wdt) {
		SCHARGER_INF("%s : CHG_WDT_IRQ, 0x%x\n", __func__, reg_wdt);
	}

	hi6523_write_byte(CHG_IRQ0, reg0);
	hi6523_write_byte(CHG_IRQ1, reg1);
	hi6523_write_byte(CHG_IRQ2, reg2);
	hi6523_write_byte(CHG_WDT_IRQ, reg_wdt);
	hi6523_unmask_irq();
	enable_irq(di->irq_int);
}

/**********************************************************
*  Function:       hi6523_interrupt
*  Description:    callback function for chargerIC fault irq in charging process
*  Parameters:   irq:chargerIC fault interrupt
*                      _di:hi6523_device_info
*  return value:  IRQ_HANDLED-success or others
**********************************************************/
static irqreturn_t hi6523_interrupt(int irq, void *_di)
{
	struct hi6523_device_info *di = _di;
	disable_irq_nosync(di->irq_int);
	queue_work(system_power_efficient_wq, &di->irq_work);
	return IRQ_HANDLED;
}

/**********************************************************
*  Function:       parse_dts
*  Description:    parse_dts
*  Parameters:   device_node:hi6523 device_node
*                      _di:hi6523_device_info
*  return value:  NULL
**********************************************************/
static void parse_dts(struct device_node *np, struct hi6523_device_info *di)
{
	int ret = 0;
	int i;
	const char *chrg_vol_string = NULL;
	struct device_node *batt_node;
	di->param_dts.bat_comp = 80;
	di->param_dts.vclamp = 224;
	ret = of_property_read_u32(np, "single_phase_buck", (u32 *)&single_phase_buck );
	if (ret) {
		SCHARGER_INF("get single_phase_buck failed\n");
		single_phase_buck = 0;
	}
	SCHARGER_INF("prase_dts single_phase_buck = %d\n", single_phase_buck);

	//parse Vdpm_en
	ret = of_property_read_u32(np, "dpm_en", (u32 *)&(di->param_dts.dpm_en));
	if (ret) {
		SCHARGER_INF("dpm_en config not found,No need do dpm.\n");
		di->param_dts.dpm_en = DPM_DISABLE;
	}
	SCHARGER_INF("prase_dts dpm_en = %d\n", di->param_dts.dpm_en);

	//scharger water check
	dm_array_len = of_property_count_strings(np, "dm_water_vol");
	if ((dm_array_len <= 0) || (dm_array_len % WATER_VOLT_PARA != 0)
		|| (dm_array_len > WATER_VOLT_PARA_LEVEL * WATER_VOLT_PARA)) {
		dm_array_len = 0;
		SCHARGER_INF("dm_water_vol is invaild\n");
	}else{
		for (i = 0; i < dm_array_len; i++) {
			ret = of_property_read_string_index(np, "dm_water_vol", i, &chrg_vol_string);
			if (ret) {
				SCHARGER_ERR("get dm_water_vol failed\n");
				dm_array_len = 0;
				break;
			}

			if(i%WATER_VOLT_PARA){
				di->param_dts.scharger_check_vol.dm_vol_data[i/WATER_VOLT_PARA].vol_max
				= simple_strtol(chrg_vol_string, NULL, 10);
				SCHARGER_INF("prase_dts dm_volt_data[%d] vol_max = %d\n",
				i/WATER_VOLT_PARA, di->param_dts.scharger_check_vol.dm_vol_data[i/WATER_VOLT_PARA].vol_max);
			}else{
				di->param_dts.scharger_check_vol.dm_vol_data[i/WATER_VOLT_PARA].vol_min
				= simple_strtol(chrg_vol_string, NULL, 10);
				SCHARGER_INF("prase_dts dm_volt_data[%d] vol_min = %d\n",
				i/WATER_VOLT_PARA, di->param_dts.scharger_check_vol.dm_vol_data[i/WATER_VOLT_PARA].vol_min);
			}
		}
	}

	dp_array_len = of_property_count_strings(np, "dp_water_vol");
	if ((dp_array_len <= 0) || (dp_array_len % WATER_VOLT_PARA != 0)
		|| (dp_array_len > WATER_VOLT_PARA_LEVEL * WATER_VOLT_PARA)) {
		dp_array_len = 0;
		SCHARGER_INF("dp_water_vol is invaild\n");
	}else{
		for (i = 0; i < dp_array_len; i++) {
			ret = of_property_read_string_index(np, "dp_water_vol", i, &chrg_vol_string);
			if (ret) {
				SCHARGER_ERR("get dp_arrary_len failed\n");
				dp_array_len = 0;
				break;
			}

			if(i%WATER_VOLT_PARA){
				di->param_dts.scharger_check_vol.dp_vol_data[i/WATER_VOLT_PARA].vol_max
				= simple_strtol(chrg_vol_string, NULL, 10);
				SCHARGER_INF("prase_dts dp_volt_data[%d] vol_max = %d\n",
				i/WATER_VOLT_PARA, di->param_dts.scharger_check_vol.dp_vol_data[i/WATER_VOLT_PARA].vol_max);
			}else{
				di->param_dts.scharger_check_vol.dp_vol_data[i/WATER_VOLT_PARA].vol_min
				= simple_strtol(chrg_vol_string, NULL, 10);
				SCHARGER_INF("prase_dts dp_volt_data[%d] vol_min = %d\n",
				i/WATER_VOLT_PARA, di->param_dts.scharger_check_vol.dp_vol_data[i/WATER_VOLT_PARA].vol_min);
			}
		}
	}

	ret = of_property_read_u32(np, "bat_comp", (u32 *)&(di->param_dts.bat_comp));
	if (ret) {
		SCHARGER_ERR("get bat_comp failed\n");
		return;
	}
	SCHARGER_INF("prase_dts bat_comp = %d\n", di->param_dts.bat_comp);

	ret = of_property_read_u32(np, "vclamp", (u32 *)&(di->param_dts.vclamp));
	if (ret) {
		SCHARGER_ERR("get vclamp failed\n");
		return;
	}
	SCHARGER_INF("prase_dts vclamp = %d\n", di->param_dts.vclamp);

	ret =
	    of_property_read_u32(np, "adc_channel_vbat_sys",
				 (u32 *)&adc_channel_vbat_sys);
	if (ret) {
		SCHARGER_INF("get adc_channel_vbat_sys failed!\n");
		adc_channel_vbat_sys = -1;
	}

	ret =
	    of_property_read_u32(np, "fcp_support",
				 (u32 *)&(di->param_dts.fcp_support));
	if (ret) {
		SCHARGER_ERR("get fcp_support failed\n");
		return;
	}
	SCHARGER_INF("prase_dts fcp_support = %d\n", di->param_dts.fcp_support);

	ret =
	    of_property_read_u32(np, "scp_support",
				 (u32 *)&(di->param_dts.scp_support));
	if (ret) {
		SCHARGER_ERR("get scp_support failed\n");
		return;
	}
	SCHARGER_INF("prase_dts scp_support = %d\n", di->param_dts.scp_support);
	batt_node =
	    of_find_compatible_node(NULL, NULL, "huawei,hisi_bci_battery");
	if (batt_node) {
		if (of_property_read_u32
		    (batt_node, "battery_board_type", &is_board_type)) {
			SCHARGER_ERR("get battery_board_type fail!\n");
			is_board_type = BAT_BOARD_ASIC;
		}
	} else {
		SCHARGER_ERR("get hisi_bci_battery fail!\n");
		is_board_type = BAT_BOARD_ASIC;
	}

	ret = of_property_read_u32(np, "switch_id_change",/*lint !e64*/
				 &switch_id_flag);
	if (ret) {
		SCHARGER_ERR("get switch id change fail!\n");
		return;
	}
	return;
}/*lint !e64*/

static void hi6523_fcp_scp_ops_register(void)
{
	int ret = 0;
	/* if support fcp ,register fcp adapter ops */
	if (0 == hi6523_is_support_fcp()) {
		ret = fcp_adapter_ops_register(&fcp_hi6523_ops);
		if (ret)
			SCHARGER_ERR("register fcp adapter ops failed!\n");
		else
			SCHARGER_INF(" fcp adapter ops register success!\n");
	}
#ifdef CONFIG_DIRECT_CHARGER
	/* if chip support scp ,register scp adapter ops */
	if( 0 == hi6523_is_support_scp())
	{
		ret = scp_ops_register(&scp_hi6523_ops);
		if (ret)
		{
			SCHARGER_ERR("register scp adapter ops failed!\n");
		}
		else
		{
			SCHARGER_INF(" scp adapter ops register success!\n");
		}
	}
#endif
}

/**********************************************************
*  Function:       hi6523_probe
*  Description:    HI6523 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_probe(struct i2c_client *client,/*lint !e64*/
			const struct i2c_device_id *id)
{
	int ret = 0;
	struct hi6523_device_info *di = NULL;
	struct device_node *np = NULL;
	struct class *power_class = NULL;
	unsigned char hi6523_version_high = 0;
	unsigned char hi6523_version_low = 0;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);/*lint !e64*/
	if (NULL == di) {
		SCHARGER_ERR("%s hi6523_device_info is NULL!\n", __func__);
		return -ENOMEM;
	}
	g_hi6523_dev = di;
	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	parse_dts(np, di);
	INIT_WORK(&di->irq_work, hi6523_irq_work);
	INIT_DELAYED_WORK(&di->plugout_check_work, hi6523_plugout_check_work);
	hi6523_init_irq_status();
    di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
	if (!gpio_is_valid(di->gpio_int)) {
		SCHARGER_ERR("gpio_int is not valid\n");
		ret = -EINVAL;
		goto hi6523_fail_0;
	}

	ret = gpio_request(di->gpio_int, "charger_int");
	if (ret) {
		SCHARGER_ERR("could not request gpio_int\n");
		goto hi6523_fail_1;
	}
	ret = gpio_direction_input(di->gpio_int);
	if (ret < 0) {
		SCHARGER_ERR("Could not set gpio direction.\n");
		goto hi6523_fail_2;
	}
	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0) {
		SCHARGER_ERR("could not map gpio_int to irq\n");
		goto hi6523_fail_2;
	}
	ret = request_irq(di->irq_int, hi6523_interrupt,
			IRQF_TRIGGER_FALLING, "charger_int_irq", di);
	if (ret) {
		SCHARGER_ERR("could not request irq_int\n");
		di->irq_int = -1;
		goto hi6523_fail_3;
	}

	di->charger_type = hisi_get_charger_type();
	di->usb_nb.notifier_call = hi6523_usb_notifier_call;
	ret = hisi_charger_type_notifier_register(&di->usb_nb);
	if (ret < 0) {
		SCHARGER_ERR("hisi_charger_type_notifier_register failed\n");
		goto hi6523_fail_3;
	}
	di->term_vol_mv = hi6523_get_terminal_voltage();
	hi6523_apple_adapter_detect(APPLE_DETECT_ENABLE);/*enable scharger apple detect*/
	mutex_init(&hi6523_fcp_detect_lock);
	mutex_init(&hi6523_adc_conv_lock);
	mutex_init(&hi6523_accp_adapter_reg_lock);
	mutex_init(&hi6523_ibias_calc_lock);
	ret = water_detect_ops_register(&hi6523_water_detect_ops);
	if (ret) {
		SCHARGER_ERR("register water detect ops failed!\n");
	}
	ret = charge_ops_register(&hi6523_ops);
	if (ret) {
		SCHARGER_ERR("register charge ops failed!\n");
		goto hi6523_fail_3;
	}
	hi6523_fcp_scp_ops_register();
	ret = hi6523_sysfs_create_group(di);
	if (ret) {
		SCHARGER_ERR("create sysfs entries failed!\n");
		goto hi6523_fail_6;
	}
	power_class = hw_power_get_class();
	if (power_class) {
		if (charge_dev == NULL) {
			charge_dev =
			    device_create(power_class, NULL, 0, NULL,
					  "charger");
			if (IS_ERR(charge_dev)) {
				charge_dev = NULL;
				SCHARGER_ERR("create charge_dev failed!\n");
				goto hi6523_fail_6;
			}
		}
		ret =
		    sysfs_create_link(&charge_dev->kobj, &di->dev->kobj,
				      "HI6523");
		if (ret)
			SCHARGER_ERR("create link to HI6523 fail.\n");
	}
	hi6523_read_byte(CHIP_VERSION_4, &hi6523_version_high);
	hi6523_read_byte(CHIP_VERSION_5, &hi6523_version_low);
	hi6523_version = hi6523_version_high;
	hi6523_version = ((hi6523_version << 8) | hi6523_version_low);
	SCHARGER_INF("%s : hi6523_version is 0x%x\n", __func__,
		     hi6523_version);
	hi6523_plugout_check_process(di->charger_type);

	ret = hi6523_get_vbus_gain_cal(&Gain_cal);
	if(ret)
		SCHARGER_ERR("[%s]get Gain fail,ret:%d\n", __func__, ret);

	SCHARGER_INF("HI6523 probe ok!\n");
	return 0;

hi6523_fail_6:
	hi6523_sysfs_remove_group(di);
hi6523_fail_3:
	free_irq(di->irq_int, di);
hi6523_fail_2:
	gpio_free(di->gpio_int);
hi6523_fail_1:
hi6523_fail_0:
	g_hi6523_dev = NULL;
	np = NULL;

	return ret;
}

/**********************************************************
*  Function:       hi6523_remove
*  Description:    HI6523 module remove
*  Parameters:   client:i2c_client
*  return value:  0-success or others-fail
**********************************************************/
static int hi6523_remove(struct i2c_client *client)
{
	struct hi6523_device_info *di = i2c_get_clientdata(client);
	if (NULL == di)
		return -1;

	hi6523_sysfs_remove_group(di);

	if (di->irq_int) {
		free_irq(di->irq_int, di);
	}
	if (di->gpio_int) {
		gpio_free(di->gpio_int);
	}
	g_hi6523_dev = NULL;
	return 0;
}

MODULE_DEVICE_TABLE(i2c, HI6523);
static struct of_device_id hi6523_of_match[] = {
	{
	 .compatible = "huawei,hi6523_charger",
	 .data = NULL,
	 },
	{
	 },
};

static const struct i2c_device_id hi6523_i2c_id[] =
    { {"hi6523_charger", 0}, {} };

static struct i2c_driver hi6523_driver = {
	.probe = hi6523_probe,
	.remove = hi6523_remove,
	.id_table = hi6523_i2c_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "hi6523_charger",
		   .of_match_table = of_match_ptr(hi6523_of_match),
		   },
};

/**********************************************************
*  Function:       hi6523_init
*  Description:    HI6523 module initialization
*  Parameters:   NULL
*  return value:  0-success or others-fail
**********************************************************/
static int __init hi6523_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&hi6523_driver);
	if (ret)
		SCHARGER_ERR("%s: i2c_add_driver error!!!\n", __func__);

	return ret;
}

/**********************************************************
*  Function:       hi6523_exit
*  Description:    HI6523 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit hi6523_exit(void)
{
	i2c_del_driver(&hi6523_driver);
}

module_init(hi6523_init);
module_exit(hi6523_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HI6523 charger module driver");
MODULE_AUTHOR("HW Inc");
