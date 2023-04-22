
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/i2c.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/syscalls.h>
#include <linux/module.h>
#include "hi6531_soh.h"
#include <./../hisi_soh_core.h>
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#include <linux/power/hisi/hisi_bci_battery.h>
#include "securec.h"

#ifndef STATIC
#define STATIC  static
#endif

/*acr mul and resistivity table for acr calculate, Magnified 100,000 times times*/
static long M_R_TABLE[ACR_MUL_MAX][ACR_DATA_FIFO_DEPTH] =
{
    {-9194, -22196, -22196, -9194, 9194, 22196, 22196, 9194},
    {-4597, -11098, -11098, -4597, 4597, 11098, 11098, 4597},
    {-2298, -5549,  -5549,  -2298, 2298, 5549,  5549,  2298},
    {-1149, -2774,  -2774,  -1149, 1149, 2774,  2774,  1149}
};
/*acr mul and range table
    ACR_MUL_35 -> [10moh,200moh]
	ACR_MUL_70 -> [10moh,100moh]
	ACR_MUL_140-> [10moh,50moh]
	ACR_MUL_280-> [10moh,25moh]
*/
static int M_RANGE_TABLE[ACR_MUL_MAX][2] =
{
   {10, 200},
   {10, 100},
   {10, 50},
   {10, 25}
};

static u8 acr_mul_sel = ACR_MUL_70;
static struct hi6531_device_info *g_hi6531_dev = NULL;

int hisi_soh_drv_register_atomic_notifier(struct notifier_block *nb)
{
	return 0;
}

int hisi_soh_drv_unregister_atomic_notifier(struct notifier_block *nb)
{
	return 0;
}

/**********************************************************
*  Function:       hi6531_write_block
*  Description:    register write block interface
*  Parameters:     di:hi6531_device_info
*                  value:register value
*                  reg:register name
*                  num_bytes:bytes number
*  return value:   0-success or others-fail
**********************************************************/
static int hi6531_write_block(struct hi6531_device_info *di, u8 * value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

    if (!di || !value)
        return -1;

	*value = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = value;
	msg[0].len = num_bytes + 1;

	ret = i2c_transfer(di->client->adapter, msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 1) {
		hi6531_soh_err("[%s] fail, ret = %d\n", __func__, ret);
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	return 0;
}

/**********************************************************
*  Function:       hi6531_read_block
*  Description:    register read block interface
*  Parameters:     di:hi6531_device_info
*                  value:register value
*                  reg:register name
*                  num_bytes:bytes number
*  return value:  0-success or others-fail
**********************************************************/
static int hi6531_read_block(struct hi6531_device_info *di, u8 * value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[2];
	u8 buf = 0;
	int ret = 0;

    if (!di || !value)
        return -1;

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
		hi6531_soh_err("[%s]fail,ret = %d\n", __func__, ret);
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	return 0;
}

/**********************************************************
*  Function:       hi6531_write_byte
*  Description:    register write byte interface
*  Parameters:     reg:register name
*                  value:register value
*  return value:   0-success or others-fail
**********************************************************/
STATIC void hi6531_write_byte(u8 reg, u8 value)
{
	/* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
	u8 temp_buffer[2] = {0};
    int ret;
    struct hi6531_device_info *di = g_hi6531_dev;

	if (!di) {
		hi6531_soh_err("[%s] di is NULL!\n", __func__);
		return;
	}
	/* offset 1 contains the data */
	temp_buffer[1] = value;
	ret = hi6531_write_block(di, temp_buffer, reg, 1);
    if (ret)
        hi6531_soh_err("[%s] w [0x%x]fail!\n", __func__, reg);
}

/**********************************************************
*  Function:       hi6531_read_byte
*  Description:    register read byte interface
*  Parameters:     reg:register name
*                  value:register value
*  return value:   0-success or others-fail
**********************************************************/
STATIC void hi6531_read_byte(u8 reg, u8 * value)
{
	struct hi6531_device_info *di = g_hi6531_dev;
    int ret;

	if (!di) {
		hi6531_soh_err("[%s] di is NULL!\n", __func__);
		return ;
	}

	ret = hi6531_read_block(di, value, reg, 1);
    if (ret)
        hi6531_soh_err("[%s] r[0x%x] fail!\n", __func__, reg);
}



/********************************************************
  Function:      soh_hi6531_enable_acr
  Description:   enable acr .
  Input:         0:disalbe ,other:enable
  Output:        NA
  Return:        NA
  Remart:        NA
********************************************************/
STATIC void __soh_hi6531_ldo1_en(int enable)
{
    hi6531_write_byte(LDO_LOCK_REG, LDO_WRITE_UNLOCK);
    if (enable)
        hi6531_write_byte(ACR_LDO1_EN_REG, ACR_LDO1_EN_BIT);
    else
        hi6531_write_byte(ACR_LDO1_EN_REG, (u8)(~ACR_LDO1_EN_BIT));

    hi6531_write_byte(LDO_LOCK_REG, LDO_WRITE_LOCK);
}
/********************************************************
  Function:      soh_hi6531_acr_enable
  Description:   enable acr .
  Input:         0:disalbe ,other:enable
  Output:        NA
  Return:        NA
  Remart:        NA
********************************************************/
STATIC void soh_hi6531_acr_enable(int enable)
{
    if (enable) {
        __soh_hi6531_ldo1_en(ACR_LDO1_EN);
        hi6531_write_byte(ACR_EN_ADDR, ACR_EN);
    } else {
        hi6531_write_byte(ACR_EN_ADDR, ACR_DIS);
        __soh_hi6531_ldo1_en(ACR_LDO1_DIS);
    }
}
/*******************************************************
  Function:      soh_hi6531_acr_get_acr_flag
  Description:   get acr flag .
  Input:         NA
  Output:        NA
  Return:        1:acr flag Produce 0:no
  Remart:        NA
********************************************************/
STATIC u32 soh_hi6531_acr_get_acr_flag(void)
{
    u8 reg_val = 0;

    hi6531_read_byte(ACR_INT_ADDR, &reg_val);
    reg_val &= ACR_FLAG_INT_BIT;
    if (reg_val)
        return 1;
    else
        return 0;
}
/*******************************************************
  Function:      soh_hi6531_acr_get_ocp_status
  Description:   get acr ocp status .
  Input:         NA
  Output:        NA
  Return:        1:acr ocp Produce 0:no
  Remart:        NA
********************************************************/
STATIC u32 soh_hi6531_acr_get_ocp_status(void)
{
    u8  reg_val = 0;

    hi6531_read_byte(ACR_INT_ADDR, &reg_val);
    reg_val &= ACR_OCP_INT_BIT;
    if (reg_val)
        return 1;
    else
        return 0;
}
/*******************************************************
  Function:      soh_hi6531_acr_get_fault_status
  Description:   get acr fault status .
  Input:         NA
  Output:        NA
  Return:        1:acr fault 0: normal
  Remart:        NA
********************************************************/
STATIC u32 soh_hi6531_acr_get_fault_status(void)
{
    u8  reg_val = 0;

    hi6531_read_byte(ACR_INT_ADDR, &reg_val);

    reg_val &= (ACR_UVP_ABS_INT_MASK_BIT | ACR_OTMP_140_INT_MASK_BIT | ACR_LDO1_OCP_INT_MASK_BIT | ACR_OCP_INT_MASK_BIT) ;
    if (reg_val) {
        hi6531_soh_err("[%s] acr fault,fault reg=[0x%x]!\n", __func__,reg_val);
        return 1;
    } else
        return 0;
}
/*******************************************************
  Function:      soh_hi6531_acr_clear_acr_flag
  Description:   clear acr flag .
  Input:         NA
  Output:        NA
  Return:        NA
  Remart:        NA
********************************************************/
STATIC void soh_hi6531_acr_clear_acr_flag(void)
{
    hi6531_write_byte(ACR_INT_ADDR, ACR_FLAG_INT_BIT);
}
/*******************************************************
  Function:      soh_hi6531_acr_clear_ocp
  Description:   clear acr ocp.
  Input:         NA
  Output:        NA
  Return:        NA
  Remart:        NA
********************************************************/
STATIC void soh_hi6531_acr_clear_ocp(void)
{
    hi6531_write_byte(ACR_INT_ADDR, ACR_OCP_INT_BIT);
}

/*******************************************************
  Function:      soh_hi6531_acr_clear_fault
  Description:   clear acr fault int status.
  Input:         NA
  Output:        NA
  Return:        NA
  Remart:        NA
********************************************************/
STATIC void soh_hi6531_acr_clear_fault(void)
{
    hi6531_write_byte(ACR_INT_ADDR, (ACR_UVP_ABS_INT_MASK_BIT | ACR_OTMP_140_INT_MASK_BIT | ACR_LDO1_OCP_INT_MASK_BIT | ACR_OCP_INT_MASK_BIT));
}

/*******************************************************
  Function:      soh_hi6531_acr_set_mul
  Description:   set acr cal Multiple .
  Input:         Multiple
  Output:        NA
  Return:        NA
  Remart:        NA
********************************************************/
STATIC void __soh_hi6531_acr_set_mul(enum acr_mul mul)
{
    u8 reg_val;

	switch (mul) {
	case ACR_MUL_35:
		acr_mul_sel = ACR_MUL_35;
		break;
	case ACR_MUL_70:
		acr_mul_sel = ACR_MUL_70;
		break;
    case ACR_MUL_140:
		acr_mul_sel = ACR_MUL_140;
		break;
	case ACR_MUL_280:
		acr_mul_sel = ACR_MUL_280;
		break;
    default:
        acr_mul_sel = ACR_MUL_35;
        hi6531_soh_err("[%s] mul is 0x%x,err!\n",__func__,mul);
        break;
	}
    hi6531_read_byte(ACR_MUL_SEL_ADDR, &reg_val);
    hi6531_write_byte(ACR_MUL_SEL_ADDR, ((reg_val & ACR_MUL_MASK) | (acr_mul_sel << ACR_MUL_SHIFT)));
    hi6531_soh_inf("[%s]:mul = 0x%x\n",__func__,((reg_val & ACR_MUL_MASK) | (acr_mul_sel << ACR_MUL_SHIFT)));
}

/********************************************************
  Function:      soh_hi6531_gpio_en
  Description:   enable acr .
  Input:         0:disalbe ,other:enable
  Output:        NA
  Return:        NA
  Remart:        1 When the gpio is pulled down, it takes 20 milliseconds to wait to pull gpio low.
                 2 When the gpio is pulled UP  , it takes 20 milliseconds to wait to pull gpio high.
********************************************************/
STATIC void soh_hi6531_gpio_en(int enable)
{
	struct hi6531_device_info *di = g_hi6531_dev;
    int ret;

	if (!di) {
		hi6531_soh_err("[%s] di is NULL!\n", __func__);
		return ;
    }

    ret = gpio_direction_output(di->acr_gpio_en, enable);
    if (ret < 0)
        hi6531_soh_err("[%s] output fail!\n", __func__);

    /*This delay is necessary for io en/disable */
    usleep_range(25000,30000);

    /*Registers are restored to default values when gpio is pulled down*/
    if (enable)
        __soh_hi6531_acr_set_mul(acr_mul_sel);

}

/*******************************************************
  Function:      __soh_hi6531_acr_get_fifo_data
  Description:   get acr fifo data .
  Input:         fifo_order :fifo Serial number
  Output:        NA
  Return:        NA
  Remart:        NA
********************************************************/
STATIC u16 __soh_hi6531_acr_get_fifo_data(int fifo_order)
{
    u16 reg_val_l = 0;
    u16 reg_val_h = 0;
    int tmp_addr_l = 0;
    int tmp_addr_h = 0;

    if (fifo_order > ACR_DATA_FIFO_DEPTH) {
        hi6531_soh_err("[%s]:fifo_order = %d,overflow!\n",__func__,fifo_order);
        return 0;
    }

    tmp_addr_l = (int)ACR_L_DATA_BASE + ACR_DATA_REG_NUM * fifo_order;
    tmp_addr_h = (int)ACR_H_DATA_BASE + ACR_DATA_REG_NUM * fifo_order;

    hi6531_read_byte(tmp_addr_l, (u8 *)&reg_val_l);
    hi6531_read_byte(tmp_addr_h, (u8 *)&reg_val_h);

    return (reg_val_h << ACR_DATA_H_SHIFT | reg_val_l );
}

/*******************************************************
  Function:      soh_hi6531_acr_calculate_acr
  Description:   cal acr value
  Input:         NA
  Output:        NA
  Return:        acr value (moh)
  Remart:        If the measurement exceeds the gear position, the lower one is lowered.
********************************************************/
STATIC int soh_hi6531_acr_calculate_acr(void)
{
    u16 fifo[ACR_DATA_FIFO_DEPTH];
    int i;
    long acr_value = 0;

    for (i = 0; i < ACR_DATA_FIFO_DEPTH; i++) {
        /*acr adc vol [0.1v,1.7v], acr data is greater than zero */
        fifo[i] = __soh_hi6531_acr_get_fifo_data(i);
        if (0 == fifo[i]) {
            hi6531_soh_err("%s: fifo[%d] =0,err!\n",__func__,i);
            return -1;
        }

        acr_value += fifo[i] * M_R_TABLE[acr_mul_sel][i];
        hi6531_soh_inf("[%s]fifo[%d] = %d,m_r_table[%d][%d] = %ld, acr_value = %ld \n",
                            __func__,i,fifo[i],acr_mul_sel,i,M_R_TABLE[acr_mul_sel][i],acr_value);
    }
    /*if exceeding the gear position, the gear position is lowered and acr is recalculated*/
    if (acr_value/ACR_CAL_MAGNIFICATION > M_RANGE_TABLE[acr_mul_sel][1]) {
        hi6531_soh_err("[%s]:acr_value = %ld,change [%d] range !\n",__func__,acr_value, acr_mul_sel);
        if (acr_mul_sel >= 1) {
            __soh_hi6531_acr_set_mul(acr_mul_sel -1);
            return -1;
        }

    }
    return (int)(acr_value*ACR_MOHM_TO_UOHM/ACR_CAL_MAGNIFICATION);
}

/*******************************************************
  Function:      soh_hi6531_acr_get_chip_temp
  Description:   get acr chip temperature
  Input:         NA
  Output:        NA
  Return:        temerature (¡ã)
  Remart:        VPTAT_ACR= 1800*code£¨Ê®½øÖÆ£©/4095 (mV)
                 VPTAT_ACR=386.38+1.3*T
********************************************************/
STATIC int soh_hi6531_acr_get_chip_temp(void)
{
    int retry = ACR_GET_TEMP_RETRY;
    int reg_value = 0;
    u16 reg_val_l = 0;
    u16 reg_val_h = 0;
    int v_acr;
    int t_acr = INVALID_TEMP;

    /*start arc temp adc*/
    hi6531_write_byte(ACR_CHIP_TEMP_CFG_ADDR, ACR_CHIP_TEMP_CFG);
    hi6531_write_byte(ACR_CHIP_TEMP_EN_ADDR, ACR_CHIP_TEMP_EN);
    udelay(ACR_ADC_WAIT_US);
    hi6531_write_byte(ACR_CHIP_TEMP_ADC_START_ADDR, ACR_CHIP_TEMP_ADC_START);

    do {
        if(0 == retry) {
            hi6531_soh_err("[%s] retry fail!\n", __func__);
            t_acr = INVALID_TEMP;
            goto reset_temp_adc;
        }
        udelay(ACR_ADC_WAIT_US);/*chip requires 12.5us*/
        hi6531_read_byte(ACR_CHIP_TEMP_ADC_STATUS_ADDR, (u8 *)&reg_value);
        hi6531_soh_err("[%s] retry = [%d]\n", __func__, ACR_GET_TEMP_RETRY - retry);
        retry--;
    } while (!(reg_value & ACR_CHIP_TEMP_ADC_READY));

    hi6531_read_byte(ACR_CHIP_TEMP_ADC_DATA0_ADDR, (u8 *)&reg_val_l);
    hi6531_read_byte(ACR_CHIP_TEMP_ADC_DATA1_ADDR, (u8 *)&reg_val_h);
    reg_value = (int)(reg_val_h << ACR_CHIP_TEMP_H_SHIFT | reg_val_l);
    v_acr     = (1800 * reg_value)/4095;
    t_acr     = (v_acr*10000 - 3863800)/13000;
    hi6531_soh_debug("[%s] reg_val_l = [0x%x], reg_val_h = [0x%x], reg_value = [0x%x]\n", __func__, reg_val_l, reg_val_h, reg_value);
    /*close chip adc channel*/
reset_temp_adc:
    hi6531_write_byte(ACR_CHIP_TEMP_CFG_ADDR, ACR_CHIP_TEMP_CFG_DEFAULT);
    hi6531_write_byte(ACR_CHIP_TEMP_EN_ADDR, ACR_CHIP_TEMP_EN_DEFAULT);

    return t_acr;
}

/*******************************************************
  Function:      __soh_hi6531_check_chip_version
  Description:   check chip version.
  Input:         NA
  Output:        NA
  Return:        0:check suc;other:check fail.
  Remart:        NA
********************************************************/
static int __soh_hi6531_check_chip_version(void)
{
    u8 reg_data;
    hi6531_read_byte(ACR_VERSION0_ADDR, &reg_data);
    if (ACR_VERSION0_DATA != reg_data)
        return -1;

    hi6531_read_byte(ACR_VERSION1_ADDR, &reg_data);
    if (ACR_VERSION1_DATA != reg_data)
        return -1;

    hi6531_read_byte(ACR_VERSION2_ADDR, &reg_data);
    if (ACR_VERSION2_DATA != reg_data)
        return -1;

    hi6531_read_byte(ACR_VERSION3_ADDR, &reg_data);
    if (ACR_VERSION3_DATA != reg_data)
        return -1;

    hi6531_soh_inf("[%s]:suc!\n", __func__);
    return 0;

}

/*******************************************************
  Function:      parse_hi6531_acr_dts
  Description:   parse dts.
  Input:         NA
  Output:        NA
  Return:        0:suc;other: fail.
  Remart:        NA
********************************************************/
static int parse_hi6531_acr_dts(struct hi6531_device_info *di)
{
    int ret = 0;
    struct device_node* np;

    if (!di)
        return -1;

    np = di->dev->of_node;
    ret = of_property_read_u32(np, "hi6531_acr_support", (u32 *)&di->acr_support);
    if (ret) {
      hi6531_soh_err("[%s]:acr support dts not exist!\n",__func__);
      di->acr_support = 0;
    }

    hi6531_soh_inf("[%s]:support =%d\n",__func__,di->acr_support);
    return 0;
}



static struct soh_acr_device_ops hi6531_acr_ops =
{
    .enable_acr             = soh_hi6531_acr_enable,
    .get_acr_flag           = soh_hi6531_acr_get_acr_flag,
    .get_acr_ocp            = soh_hi6531_acr_get_ocp_status,
    .clear_acr_flag         = soh_hi6531_acr_clear_acr_flag,
    .clear_acr_ocp          = soh_hi6531_acr_clear_ocp,
    .calculate_acr          = soh_hi6531_acr_calculate_acr,
    .get_acr_chip_temp      = soh_hi6531_acr_get_chip_temp,
    .get_acr_fault_status   = soh_hi6531_acr_get_fault_status,
    .clear_acr_fault_status = soh_hi6531_acr_clear_fault,
    .io_ctrl_acr_chip_en    = soh_hi6531_gpio_en,
};


#ifdef CONFIG_SYSFS
/*
 * There are a numerous options that are configurable on the HI6531
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */
#define HI6531_SYSFS_FIELD(_name, r, f, m, store)                  \
{                                                   \
    .attr = __ATTR(_name, m, hi6531_sysfs_show, store),    \
}

#define HI6531_SYSFS_FIELD_RW(_name, r, f)                     \
        HI6531_SYSFS_FIELD(_name, r, f, S_IWUSR | S_IRUGO, \
                hi6531_sysfs_store)

static ssize_t hi6531_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf);
static ssize_t hi6531_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count);

struct hi6531_sysfs_field_info {
	struct device_attribute attr;
	u8 reg;
};

static struct hi6531_sysfs_field_info hi6531_sysfs_field_tbl[] = {
	/* sysfs name reg field in reg */
	HI6531_SYSFS_FIELD_RW(reg_addr, NONE, NONE),
	HI6531_SYSFS_FIELD_RW(reg_value, NONE, NONE),
};

static struct attribute *hi6531_sysfs_attrs[ARRAY_SIZE(hi6531_sysfs_field_tbl) + 1];

static const struct attribute_group hi6531_sysfs_attr_group = {
	.attrs = hi6531_sysfs_attrs,
};

/**********************************************************
*  Function:       hi6531_sysfs_init_attrs
*  Description:    initialize hi6531_sysfs_attrs[] for HI6531 attribute
*  Parameters:     NULL
*  return value:   NULL
**********************************************************/
static void hi6531_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(hi6531_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		hi6531_sysfs_attrs[i] = &hi6531_sysfs_field_tbl[i].attr.attr;

	hi6531_sysfs_attrs[limit] = NULL;
}

/**********************************************************
*  Function:       hi6531_sysfs_field_lookup
*  Description:    get the current device_attribute from hi6531_sysfs_field_tbl by attr's name
*  Parameters:     name:evice attribute name
*  return value:   hi6531_sysfs_field_tbl[]
**********************************************************/
static struct hi6531_sysfs_field_info *hi6531_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(hi6531_sysfs_field_tbl);

	for (i = 0; i < limit; i++)
		if (!strncmp(name, hi6531_sysfs_field_tbl[i].attr.attr.name, strlen(name)))
			break;

	if (i >= limit)
		return NULL;

	return &hi6531_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       hi6531_sysfs_show
*  Description:    show the value for all HI6531 device's node
*  Parameters:     dev:device
*                  attr:device_attribute
*                  buf:string of node value
*  return value:   0-success or others-fail
**********************************************************/
static ssize_t hi6531_sysfs_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	struct hi6531_sysfs_field_info *info;
	struct hi6531_sysfs_field_info *info2;
	u8 v = 0;

	info = hi6531_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	if (!strncmp("reg_addr", attr->attr.name, strlen("reg_addr"))) {
		return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "0x%hhx\n", info->reg);
	}

	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = hi6531_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}

#ifdef CONFIG_HISI_DEBUG_FS
	hi6531_read_byte(info->reg, &v);
#endif

	return snprintf_s(buf, PAGE_SIZE, PAGE_SIZE, "0x%hhx\n", v);
}

/**********************************************************
*  Function:       hi6531_sysfs_store
*  Description:    set the value for all HI6531 device's node
*  Parameters:     dev:device
*                  attr:device_attribute
*                  buf:string of node value
*                  count:unused
*  return value:   0-success or others-fail
**********************************************************/
static ssize_t hi6531_sysfs_store(struct device *dev,
				  struct device_attribute *attr,
				  const char *buf, size_t count)
{
	struct hi6531_sysfs_field_info *info;
	struct hi6531_sysfs_field_info *info2;
	int ret;
	u8 v = 0;

	info = hi6531_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	ret = kstrtou8(buf, 0, &v);
	if (ret < 0)
		return ret;

	if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
		info2 = hi6531_sysfs_field_lookup("reg_addr");
		if (!info2)
			return -EINVAL;
		info->reg = info2->reg;
	}
	if (!strncmp(("reg_addr"), attr->attr.name, strlen("reg_addr"))) {
		if (v < (u8) HI6531_REG_TOTAL_NUM) {
			info->reg = v;
        #ifdef CONFIG_HISI_DEBUG_FS
            soh_hi6531_gpio_en(1);
        #endif
			return count;
		} else {
			return -EINVAL;
		}
	}
#ifdef CONFIG_HISI_DEBUG_FS
	hi6531_write_byte(info->reg, v);
#endif

	return count;
}

/**********************************************************
*  Function:       hi6531_sysfs_create_group
*  Description:    create the HI6531 device sysfs group
*  Parameters:    di:hi6531_device_info
*  return value:  0-success or others-fail
**********************************************************/
static int hi6531_sysfs_create_group(struct hi6531_device_info *di)
{
	hi6531_sysfs_init_attrs();

	return sysfs_create_group(&di->dev->kobj, &hi6531_sysfs_attr_group);
}


static int hi6531_create_sysfs_file(struct hi6531_device_info *di)
{
    int retval = 0;
    struct class *power_class;

    if (!di) {
        hisi_soh_err("%s input di is null.", __func__);
        return -1;
    }

    retval = hi6531_sysfs_create_group(di);
    if (retval) {
        hisi_soh_err("%s failed to create sysfs group!!!\n", __func__);
        return -1;
    }
    power_class = hw_power_get_class();
    if (power_class) {
        if (!soh_dev) {
            soh_dev = device_create(power_class, NULL, 0, "%s", "soh");
            if (IS_ERR(soh_dev)) {
                soh_dev = NULL;
            }
        }
        if (soh_dev) {
            retval = sysfs_create_link(&soh_dev->kobj, &di->dev->kobj, "hi6531");
            if (0 != retval)
                hisi_soh_err("[%s] failed to create sysfs link!!!\n", __func__);
        } else
            hisi_soh_err("[%s] failed to create new_dev!!!\n", __func__);
    }

    if (retval)
        sysfs_remove_group(&di->dev->kobj, &hi6531_sysfs_attr_group);
    return retval;
}


#else
static int hi6531_create_sysfs_file(struct hi6531_device_info *di)
{
	return 0;
}

#endif

/*******************************************************
  Function:      soh_hi6531_acr_init
  Description:   acr init.
  Input:         hi6531_device_info *di :soh device struct.
  Output:        NA
  Return:        0:success; other:fail.
  Remart:        NA
********************************************************/
STATIC int soh_hi6531_acr_init(struct hi6531_device_info *di)
{
    int ret;

    if (!di)
        return -1;

    /*get dts config*/
    ret = parse_hi6531_acr_dts(di);
    if (ret)
        return -1;

    /*acr starts according dts config*/
    if (!di->acr_support)
        return 0;

    /*get acr en io*/
	di->acr_gpio_en = of_get_named_gpio(di->dev->of_node, "acr_gpio_en", 0);
	if (!gpio_is_valid(di->acr_gpio_en)) {
		hi6531_soh_err("[%s]: get acr_gpio_en fail\n", __func__);
		return -1;
	}

    ret = gpio_request(di->acr_gpio_en, "acr_gpio_en");
    if (ret) {
        hi6531_soh_err("[%s]: acr_en invalid!\n", __func__);
        return -1;
    }

    ret = gpio_direction_output(di->acr_gpio_en, 1);
    if (ret) {
        hi6531_soh_err("[%s]: acr_en set input fail!\n", __func__);
        goto init_fail;
    }
    /*it is necessary for io en by chip*/
    usleep_range(25000,30000);
    hi6531_soh_inf("[%s]: acr_en is %d!\n", __func__, di->acr_gpio_en);

    /*check chip version*/
    ret = __soh_hi6531_check_chip_version();
    if (ret) {
        hi6531_soh_err("[%s]: chip check fail!\n", __func__);
        goto init_fail;
    }

    /*mask acr all interrupt*/
    hi6531_write_byte(ACR_INT_MASK_REG, ACR_MASK_ALL_BIT);

    /*choose acr mul*/
    __soh_hi6531_acr_set_mul(ACR_MUL_70);

    /*register acr ops to soh core*/
    ret = soh_core_drv_ops_register(&hi6531_acr_ops, ACR_DRV_OPS);
    if (ret)
        goto init_fail;

    hi6531_soh_inf("[%s] soh acr init success!\n", __func__);
    return 0;

init_fail:
    ret = gpio_direction_output(di->acr_gpio_en, 0);
    if (ret)
        hi6531_soh_err("[%s] init_fail set gpio fail.\n", __func__);
    usleep_range(25000,30000);
	if (di->acr_gpio_en)
		gpio_free(di->acr_gpio_en);

    return -1;

}

/*******************************************************
  Function:      soh_hi6531_acr_uninit
  Description:   acr init.
  Input:         NA.
  Output:        NA
  Return:        NA.
  Remart:        NA
********************************************************/
STATIC void soh_hi6531_acr_uninit(void)
{
    int ret;
    ret = soh_core_drv_ops_register(NULL, ACR_DRV_OPS);
    if (ret)
        hi6531_soh_inf("[%s] fail!\n", __func__);
}

/*******************************************************
  Function:        soh_hi6531_probe
  Description:     hisi soh hi6531 probe function
  Input:           struct i2c_client *client  ---- i2c device
  Output:          NULL
  Return:          NULL
********************************************************/
static int  soh_hi6531_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    struct hi6531_device_info *di = NULL;
    int ret = 0;

    di = (struct hi6531_device_info *)devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
    if (!di) {
        hi6531_soh_err("%s failed to alloc di struct\n",__func__);
        return -ENOMEM;
    }
    di->dev =&client->dev;
    di->client = client;

    i2c_set_clientdata(client, di);

    g_hi6531_dev = di;

    /*acr init*/
    ret = soh_hi6531_acr_init(di);
    if (ret) {
        hi6531_soh_err("[%s]acr init fail!\n", __func__);
        goto acr_int_fail;
    }
    /*creat sysfs file*/
    ret = hi6531_create_sysfs_file(di);
    if (ret) {
        hi6531_soh_err("[%s]sysfs fail!\n", __func__);
        goto sysfs_fail;
    }
    /*To conserve power, turn off ACR io en*/
    soh_hi6531_gpio_en(0);
    hi6531_soh_inf("[%s]suc!\n", __func__);
    return 0;

sysfs_fail:
    soh_hi6531_acr_uninit();
acr_int_fail:
    i2c_set_clientdata(client, NULL);
    g_hi6531_dev = NULL;
    return ret;
}

/*******************************************************
  Function:        soh_hi6531_remove
  Description:     remove function
  Input:           struct i2c_client *pdev   ---- i2c device
  Output:          NULL
  Return:          NULL
********************************************************/
static int  soh_hi6531_remove(struct i2c_client *client)
{
    struct hi6531_device_info *di = i2c_get_clientdata(client);

    if(!di)
    {
        hi6531_soh_err("[%s]di is null\n",__func__);
        return -1;
    }
    i2c_set_clientdata(client, NULL);
    soh_hi6531_acr_uninit();
	if (di->acr_gpio_en)
		gpio_free(di->acr_gpio_en);

    g_hi6531_dev = NULL;
    return 0;
}

MODULE_DEVICE_TABLE(i2c, hi6531);

static struct of_device_id soh_hi6531_match_table[] =
{
    {
          .compatible = "hisi,soh_hi6531",
          .data       = NULL,
    },
    {},
};

static const struct i2c_device_id soh_hi6531_i2c_id[] = {
    {"soh_hi6531", 0},
    {}
};

static struct i2c_driver hi6531_soh_driver = {
    .probe         = soh_hi6531_probe,
    .remove        = soh_hi6531_remove,
    .driver        = {
       .name           = "soh_hi6531",
       .owner          = THIS_MODULE,
       .of_match_table = of_match_ptr(soh_hi6531_match_table),
    },
    .id_table     = soh_hi6531_i2c_id,
};

int __init soh_hi6531_init(void)
{
    return i2c_add_driver(&hi6531_soh_driver);
}

void __exit soh_hi6531_exit(void)
{
    i2c_del_driver(&hi6531_soh_driver);
}

module_init(soh_hi6531_init);
module_exit(soh_hi6531_exit);


MODULE_AUTHOR("HISILICON");
MODULE_DESCRIPTION("hisi hi6531 soh driver");
MODULE_LICENSE("GPL");
