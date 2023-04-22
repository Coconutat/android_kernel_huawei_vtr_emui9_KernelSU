

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
#include <hisi_scharger_v600.h>
#include <linux/raid/pq.h>
#include <huawei_platform/power/wired_channel_switch.h>
#include <huawei_platform/power/direct_charger.h>
#include <huawei_platform/power/huawei_charger.h>
#include <linux/hisi/hisi_adc.h>
#include <huawei_platform/usb/hw_pd_dev.h>
#include "securec.h"


#define ILIMIT_RBOOST_CNT        (15)
struct hi6526_device_info *g_hi6526_dev = NULL;

static int hi6526_force_set_term_flag = CHG_STAT_DISABLE;
static int g_rboost_cnt;
static int I_bias_all = 0;
static u32 scp_error_flag = 0;

static u8 scaf_randnum_remote[SCP_ADAPTOR_RANDOM_NUM_LO_LEN];
static u8 scaf_digest_remote_hi[SCP_ADAPTOR_DIGEST_LEN];

static int hi6526_set_charge_enable(int enable);
static int hi6526_is_support_scp(void);
static int hi6526_fcp_adapter_reg_read(u8 * val, u8 reg);
static int hi6526_fcp_adapter_reg_write(u8 val, u8 reg);
static int hi6526_reset_watchdog_timer(void);
#ifndef CONFIG_DIRECT_CHARGER
static int dummy_ops_register1(struct smart_charge_ops* ops) {return 0;}
static int dummy_ops_register2(struct loadswitch_ops* ops) {return 0;}
static int dummy_ops_register3(struct batinfo_ops* ops) {return 0;}
#endif

static int g_direct_charge_mode = 0;
static void scp_set_direct_charge_mode(int mode)
{
	g_direct_charge_mode = mode;
	SCHARGER_INF("%s : g_direct_charge_mode = %d \n",__func__, g_direct_charge_mode);
}

static int charger_is_fcp = FCP_FALSE;
static int first_insert_flag = FIRST_INSERT_TRUE;
static int is_weaksource = WEAKSOURCE_FALSE;


struct opt_regs common_opt_regs[] = {
        /*reg,  mask, shift, val, before,after*/
        REG_CFG(0xab  , 0xff,   0, 0x63, 0,    0),
        REG_CFG(0x2e2 , 0xff,   0, 0x95, 0,    0),
        REG_CFG(0x2e5 , 0xff,   0, 0x58, 0,    0),
        REG_CFG(0x293 , 0xff,   0, 0xA0, 0,    0),
        REG_CFG(0x284 , 0xff,   0, 0x08, 0,    0),
        REG_CFG(0x287 , 0xff,   0, 0x1C, 0,    0),
        REG_CFG(0x280 , 0xff,   0, 0x0D, 0,    0),
        REG_CFG(0x2ac , 0xff,   0, 0x1e, 0,    0),
        REG_CFG(0x298 , 0xff,   0, 0x01, 0,    0),
};

struct opt_regs buck_opt_regs[] = {
        /*reg,  mask, shift, val, before,after*/
        REG_CFG(0x2d3 , 0xff,   0, 0x15, 0,    0),
        REG_CFG(0x2d4 , 0xff,   0, 0x97, 0,    0),
        REG_CFG(0x2e1 , 0xff,   0, 0x7D, 0,    0),
        REG_CFG(0x2e3 , 0xff,   0, 0x22, 0,    0),
        REG_CFG(0x2e4 , 0xff,   0, 0x3D, 0,    0),
        REG_CFG(0x2da , 0xff,   0, 0x8C, 0,    0),
        REG_CFG(0x2db , 0xff,   0, 0x94, 0,    0),
        REG_CFG(0x2d6 , 0xff,   0, 0x0F, 0,    0),
        REG_CFG(0x2d9 , 0xff,   0, 0x11, 0,    0),
        REG_CFG(0x2e7 , 0xff,   0, 0xBF, 0,    0),
        REG_CFG(0x2de , 0xff,   0, 0x1E, 0,    0),
        REG_CFG(0x286 , 0xff,   0, 0x06, 0,    0),
};

struct opt_regs lvc_opt_regs[] = {
        /*reg,  mask, shift, val, before,after*/
        REG_CFG(0x2d3 , 0xff,   0, 0x15, 0,    0),
        REG_CFG(0x2d4 , 0xff,   0, 0x97, 0,    0),
        REG_CFG(0x2e1 , 0xff,   0, 0x7D, 0,    0),
        REG_CFG(0x2e3 , 0xff,   0, 0x22, 0,    0),
        REG_CFG(0x2e4 , 0xff,   0, 0x3D, 0,    0),
        REG_CFG(0x2da , 0xff,   0, 0x8C, 0,    0),
        REG_CFG(0x2db , 0xff,   0, 0x94, 0,    0),
        REG_CFG(0x2d6 , 0xff,   0, 0x0F, 0,    0),
        REG_CFG(0x2d9 , 0xff,   0, 0x11, 0,    0),
        REG_CFG(0x2e7 , 0xff,   0, 0xBF, 0,    0),
        REG_CFG(0x2de , 0xff,   0, 0x1F, 0,    0),
        REG_CFG(0x286 , 0xff,   0, 0x06, 0,    0),
        REG_CFG(0x2b0 , 0xff,   0, 0xE2, 0,    0),
        REG_CFG(0x2b1 , 0xff,   0, 0x16, 0,    0),
        REG_CFG(0x2b7 , 0xff,   0, 0x4D, 0,    0),
        REG_CFG(0x2b8 , 0xff,   0, 0x67, 0,    0),
        REG_CFG(0x2b3 , 0xff,   0, 0x1F, 0,    0),
        REG_CFG(0x2b6 , 0xff,   0, 0x10, 0,    0),
        REG_CFG(0x2c6 , 0xff,   0, 0x13, 0,    0),
        REG_CFG(0x2b5 , 0xff,   0, 0xF8, 0,    0),
        REG_CFG(0x2bb , 0xff,   0, 0xE3, 0,    0),
        REG_CFG(0x2c4 , 0xff,   0, 0x4E, 0,    0),
        REG_CFG(0x2c5 , 0xff,   0, 0x65, 0,    0),
        REG_CFG(0xed ,  0xff,   0, 0xFF, 0,    0),
        REG_CFG(0xee ,  0xff,   0, 0x16, 0,    0),
        REG_CFG(0xef ,  0xff,   0, 0x23, 0,    0),
        REG_CFG(0x2b2 , 0xff,   0, 0xC1, 0,    0),
        REG_CFG(0x2bc , 0xff,   0, 0x57, 0,    0),
};

struct opt_regs lvc_opt_regs_after_enabled[] = {
        REG_CFG(0x2BB,  0xff,   0, 0xE7, 20,   0),
};

struct opt_regs sc_opt_regs[] = {
        /*reg,  mask, shift, val, before,after*/
        REG_CFG(0x2d3 , 0xff,   0, 0x15, 0,    0),
        REG_CFG(0x2d4 , 0xff,   0, 0x97, 0,    0),
        REG_CFG(0x2e1 , 0xff,   0, 0x7D, 0,    0),
        REG_CFG(0x2e3 , 0xff,   0, 0x22, 0,    0),
        REG_CFG(0x2e4 , 0xff,   0, 0x3D, 0,    0),
        REG_CFG(0x2da , 0xff,   0, 0x8C, 0,    0),
        REG_CFG(0x2db , 0xff,   0, 0x94, 0,    0),
        REG_CFG(0x2d6 , 0xff,   0, 0x0F, 0,    0),
        REG_CFG(0x2d9 , 0xff,   0, 0x11, 0,    0),
        REG_CFG(0x2e7 , 0xff,   0, 0xBF, 0,    0),
        REG_CFG(0x2de , 0xff,   0, 0X1F, 0,    0),
        REG_CFG(0x286 , 0xff,   0, 0x06, 0,    0),
        REG_CFG(0x2bd , 0xff,   0, 0xD9, 0,    0),
        REG_CFG(0x2be , 0xff,   0, 0x09, 0,    0),
        REG_CFG(0x2bf , 0xff,   0, 0x09, 0,    0),
        REG_CFG(0x2c0 , 0xff,   0, 0x09, 0,    0),
        REG_CFG(0x2c1 , 0xff,   0, 0x10, 0,    0),
        REG_CFG(0x2b0 , 0xff,   0, 0XE2, 0,    0),
        REG_CFG(0x2b1 , 0xff,   0, 0X16, 0,    0),
        REG_CFG(0x2b7 , 0xff,   0, 0x7D, 0,    0),
        REG_CFG(0x2b8 , 0xff,   0, 0X67, 0,    0),
        REG_CFG(0x2b3 , 0xff,   0, 0X1F, 0,    0),
        REG_CFG(0x2b6 , 0xff,   0, 0x14, 0,    0),
        REG_CFG(0x2c6,  0xff,   0, 0xF3, 0,    0),
        REG_CFG(0x2b5,  0xff,   0, 0xF8, 0,    0),
        REG_CFG(0x2c5,  0xff,   0, 0x53, 0,    0),
        REG_CFG(0x2a3 , 0xff,   0, 0x65, 0,    0),
        REG_CFG(0x2a4 , 0xff,   0, 0x00, 0,    0),
        REG_CFG(0x2bc , 0xff,   0, 0x57, 0,    0),
        REG_CFG(0xed ,  0xff,   0, 0xFF, 0,    0),
        REG_CFG(0xee ,  0xff,   0, 0x16, 0,    0),
        REG_CFG(0xef ,  0xff,   0, 0x23, 0,    0),
        REG_CFG(0x2b2 , 0xff,   0, 0xC1, 0,    0),
        REG_CFG(0x2bb , 0xff,   0, 0XD3, 0,    0),
};

struct opt_regs sc_opt_regs_after_enabled[] = {
        REG_CFG(0x2BB,  0xff,   0, 0xD7, 20,   0),
};

struct opt_regs otg_opt_regs[] = {
        /*reg,  mask, shift, val, before,after*/
        REG_CFG(0x2d3 , 0xff,   0, 0x08, 0,    0),
        REG_CFG(0x2d4 , 0xff,   0, 0x16, 0,    0),
        REG_CFG(0x2e1 , 0xff,   0, 0x5A, 0,    0),
        REG_CFG(0x2e3 , 0xff,   0, 0x20, 0,    0),
        REG_CFG(0x2e4 , 0xff,   0, 0x25, 0,    0),
        REG_CFG(0x2da , 0xff,   0, 0x91, 0,    0),
        REG_CFG(0x2db , 0xff,   0, 0x92, 0,    0),
        REG_CFG(0x2e9 , 0xff,   0, 0x1C, 0,    0),
        REG_CFG(0x294 , 0xff,   0, 0x01, 0,    0),
        REG_CFG(0x2ee , 0xff,   0, 0x09, 0,    0),
        REG_CFG(0xf0  , 0x0f,   0, 0x06, 0,    0),
        REG_CFG(0x2d9 , 0xff,   0, 0x10, 0,    0),
        REG_CFG(0x2de , 0xff,   0, 0X1E, 0,    0),
};

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
*  Function:       hi6526_write_block
*  Description:    register write block interface
*  Parameters:   di:hi6526_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_write_block(u16 reg, u8 * value, unsigned num_bytes)
{
        struct i2c_msg msg[2];
        u8 page[2];
        int ret = 0;
        u8 addr = 0;

        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        mutex_lock(&di->i2c_lock);

        /* page select*/
        page[0] = REG_PAGE_SELECT;
        page[1] = (reg >> 8) & 0xff;

        msg[0].addr = di->client->addr;
        msg[0].flags = 0;
        msg[0].buf = page;
        msg[0].len = 2;

        ret = i2c_transfer(di->client->adapter, msg, 1);
        if (ret != 1) {
                SCHARGER_ERR("i2c_write page fail \n");
                scharger_i2c_err_monitor();
                mutex_unlock(&di->i2c_lock);
                return -EIO;
        }


        /* set reg addr*/
        addr = (reg & 0xff);
        msg[0].addr = di->client->addr;
        msg[0].flags = 0;
        msg[0].buf = &addr;
        msg[0].len = 1;

        /* set value */
        msg[1].addr = di->client->addr;
        msg[1].flags = 0;
        msg[1].buf = value;
        msg[1].len = num_bytes;

        ret = i2c_transfer(di->client->adapter, msg, 2);

        /* i2c_transfer returns number of messages transferred */
        if (ret != 2) {
                SCHARGER_ERR("i2c_write failed to transfer all messages\n");
                scharger_i2c_err_monitor();
                if (ret < 0) {
                        // do nothing return ret
                } else {
                        ret = (-EIO);
                 }
        } else {
                 ret = 0;
        }

        mutex_unlock(&di->i2c_lock);

        return 0;
}


/**********************************************************
*  Function:       hi6526_read_block
*  Description:    register read block interface
*  Parameters:   di:hi6526_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_read_block(u16 reg, u8 * value,  unsigned num_bytes)
{
        struct i2c_msg msg[2];
        u8 page[2];
        u8 addr = 0;
        int ret = 0;

        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        mutex_lock(&di->i2c_lock);

        /* page select*/
        page[0] = REG_PAGE_SELECT;
        page[1] = (reg >> 8) & 0xff;

        msg[0].addr = di->client->addr;
        msg[0].flags = 0;
        msg[0].buf = page;
        msg[0].len = 2;

        ret = i2c_transfer(di->client->adapter, msg, 1);
        if (ret != 1) {
                SCHARGER_ERR("i2c_write page fail \n");
                scharger_i2c_err_monitor();
                mutex_unlock(&di->i2c_lock);
                return -EIO;
        }

        /* set reg addr */
        addr = (reg & 0xff);
        msg[0].addr = di->client->addr;
        msg[0].flags = 0;
        msg[0].buf = &addr;
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
                if (ret < 0) {
                        // do nothing return ret
                } else {
                        ret = (-EIO);
                 }
        } else {
                 ret = 0;
        }

        mutex_unlock(&di->i2c_lock);

        return ret;
}


/**********************************************************
*  Function:       hi6526_write
*  Description:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_write(u16 reg, u8 value)
{
        return hi6526_write_block(reg, &value, 1);
}

/**********************************************************
*  Function:       hi6526_read
*  Description:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_read(u16 reg, u8 *value)
{
        return hi6526_read_block(reg, value, 1);
}

/**********************************************************
*  Function:       hi6526_write_mask
*  Description:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_write_mask(u16 reg, u8 mask, u8 shift, u8 value)
{
        int ret = 0;
        u8 val = 0;

        ret = hi6526_read(reg, &val);
        if (ret < 0)
                return ret;

        val &= ~mask;
        val |= ((value << shift) & mask);

        ret = hi6526_write(reg, val);

        return ret;
}

/**********************************************************
*  Function:       hi6526_read_mask
*  Description:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_read_mask(u16 reg, u8 mask, u8 shift, u8 * value)
{
        int ret = 0;
        u8 val = 0;

        ret = hi6526_read(reg, &val);
        if (ret < 0)
                return ret;
        val &= mask;
        val >>= shift;
        *value = val;

        return 0;
}

/**********************************************************
*  Function:       hi6526_efuse_read
*  Description:    efuse read interface
*  Parameters:   efuse_id: efuse1/efuse2/efuse3
*                      offset: offset byte of efuse
*                      value: efuse value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_efuse_read(int efuse_id, u8 offset, u8 *value)
{
        int ret = 0;
        u16 set_reg, get_reg;
        u8 set_shift, set_mask;
        if(EFUSE1 == efuse_id) {
                set_reg = EFUSE1_SEL_REG;
                set_shift =EFUSE1_SEL_SHIFT;
                set_mask = EFUSE1_SEL_MASK;
                get_reg = EFUSE1_RDATA_REG;
        } else if (EFUSE2 == efuse_id) {
                set_reg = EFUSE2_SEL_REG;
                set_shift =EFUSE2_SEL_SHIFT;
                set_mask = EFUSE2_SEL_MASK;
                get_reg = EFUSE2_RDATA_REG;
        } else if (EFUSE3 == efuse_id){
                set_reg = EFUSE3_SEL_REG;
                set_shift =EFUSE3_SEL_SHIFT;
                set_mask = EFUSE3_SEL_MASK;
                get_reg = EFUSE3_RDATA_REG;
        } else {
                SCHARGER_ERR("%s efuse_id %d error!\n", __func__, efuse_id);
                return -1;
        }
        ret |= hi6526_write_mask(EFUSE_SEL_REG, EFUSE_SEL_MASK, EFUSE_SEL_SHIFT, efuse_id);
        ret |= hi6526_write_mask(set_reg, set_mask, set_shift, offset);
        ret |= hi6526_write_mask(EFUSE_EN_REG, EFUSE_EN_MASK, EFUSE_EN_SHIFT, EFUSE_EN);
        mdelay(1);
        ret |= hi6526_read(get_reg, value);
        ret |= hi6526_write_mask(EFUSE_EN_REG, EFUSE_EN_MASK, EFUSE_EN_SHIFT, EFUSE_DIS);

        if(ret) {
                *value = 0;
                SCHARGER_ERR("%s error  ret = %d !\n", __func__, ret);
        }

        return ret;
}


#define CONFIG_SYSFS_SCHG
#ifdef CONFIG_SYSFS_SCHG
/*
 * There are a numerous options that are configurable on the HI6526
 * that go well beyond what the power_supply properties provide access to.
 * Provide sysfs access to them so they can be examined and possibly modified
 * on the fly.
 */

#define HI6526_SYSFS_FIELD(_name, r, f, m, store)                  \
{                                                   \
    .attr = __ATTR(_name, m, hi6526_sysfs_show, store),    \
    .reg = CHG_##r##_REG,                      \
    .mask = CHG_##f##_MSK,                       \
    .shift = CHG_##f##_SHIFT,                     \
}

#define HI6526_SYSFS_FIELD_RW(_name, r, f)                     \
        HI6526_SYSFS_FIELD(_name, r, f, S_IWUSR | S_IRUGO, \
                hi6526_sysfs_store)

static ssize_t hi6526_sysfs_show(struct device *dev,
                                 struct device_attribute *attr, char *buf);
static ssize_t hi6526_sysfs_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count);

struct hi6526_sysfs_field_info {
        struct device_attribute attr;
        u16 reg;
        u8 mask;
        u8 shift;
};

static struct hi6526_sysfs_field_info hi6526_sysfs_field_tbl[] = {
        /* sysfs name reg field in reg */
        HI6526_SYSFS_FIELD_RW(en_hiz, HIZ_CTRL, HIZ_ENABLE),
        HI6526_SYSFS_FIELD_RW(iinlim, INPUT_SOURCE, ILIMIT),
        HI6526_SYSFS_FIELD_RW(reg_addr, NONE, NONE),
        HI6526_SYSFS_FIELD_RW(reg_value, NONE, NONE),
        HI6526_SYSFS_FIELD_RW(adapter_reg, NONE, NONE),
        HI6526_SYSFS_FIELD_RW(adapter_val, NONE, NONE),
};

static struct attribute *hi6526_sysfs_attrs[ARRAY_SIZE(hi6526_sysfs_field_tbl) +
                                            1];

static const struct attribute_group hi6526_sysfs_attr_group = {
        .attrs = hi6526_sysfs_attrs,
};

/**********************************************************
*  Function:       hi6526_sysfs_init_attrs
*  Description:    initialize hi6526_sysfs_attrs[] for HI6526 attribute
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void hi6526_sysfs_init_attrs(void)
{
        int i, limit = ARRAY_SIZE(hi6526_sysfs_field_tbl);

        for (i = 0; i < limit; i++)
                hi6526_sysfs_attrs[i] = &hi6526_sysfs_field_tbl[i].attr.attr;

        hi6526_sysfs_attrs[limit] = NULL;        /* Has additional entry for this */
}

/**********************************************************
*  Function:       hi6526_sysfs_field_lookup
*  Description:    get the current device_attribute from hi6526_sysfs_field_tbl by attr's name
*  Parameters:   name:evice attribute name
*  return value:  hi6526_sysfs_field_tbl[]
**********************************************************/
static struct hi6526_sysfs_field_info *hi6526_sysfs_field_lookup(const char
                                                                 *name)
{
        int i, limit = ARRAY_SIZE(hi6526_sysfs_field_tbl);

        for (i = 0; i < limit; i++)
                if (!strncmp(name, hi6526_sysfs_field_tbl[i].attr.attr.name, strlen(name)))
                        break;

        if (i >= limit)
                return NULL;

        return &hi6526_sysfs_field_tbl[i];
}

/**********************************************************
*  Function:       hi6526_sysfs_show
*  Description:    show the value for all HI6526 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*  return value:  0-success or others-fail
**********************************************************/
static ssize_t hi6526_sysfs_show(struct device *dev,
                                 struct device_attribute *attr, char *buf)
{
        struct hi6526_sysfs_field_info *info;
        struct hi6526_sysfs_field_info *info2;
#ifdef CONFIG_HISI_DEBUG_FS
        int ret = 0;
#endif
        u8 v = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        info = hi6526_sysfs_field_lookup(attr->attr.name);
        if (!info)
                return -EINVAL;

        if (!strncmp("reg_addr", attr->attr.name, strlen("reg_addr"))) {
                return scnprintf(buf, PAGE_SIZE, "0x%x\n", info->reg);
        }
        if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
                info2 = hi6526_sysfs_field_lookup("reg_addr");
                if (!info2)
                        return -EINVAL;
                info->reg = info2->reg;
        }

        if (!strncmp("adapter_reg", attr->attr.name, strlen("adapter_reg"))) {
                return scnprintf(buf, PAGE_SIZE, "0x%x\n", di->sysfs_fcp_reg_addr);
        }
        if (!strncmp("adapter_val", attr->attr.name, strlen("adapter_val"))) {
#ifdef CONFIG_HISI_DEBUG_FS
                ret = hi6526_fcp_adapter_reg_read(&v, di->sysfs_fcp_reg_addr);
                SCHARGER_INF(" sys read fcp adapter reg 0x%x , v 0x%x \n", di->sysfs_fcp_reg_addr, v);
                if (ret)
                        return ret;
#endif
                return scnprintf(buf, PAGE_SIZE, "0x%x\n", v);
        }

#ifdef CONFIG_HISI_DEBUG_FS
        ret = hi6526_read_mask(info->reg, info->mask, info->shift, &v);
        SCHARGER_INF(" sys read reg 0x%x , v 0x%x \n", info->reg, v);
        if (ret)
                return ret;
#endif

        return scnprintf(buf, PAGE_SIZE, "0x%hhx\n", v);
}

/**********************************************************
*  Function:       hi6526_sysfs_store
*  Description:    set the value for all HI6526 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-success or others-fail
**********************************************************/
static ssize_t hi6526_sysfs_store(struct device *dev,
                                  struct device_attribute *attr,
                                  const char *buf, size_t count)
{
        struct hi6526_sysfs_field_info *info;
        struct hi6526_sysfs_field_info *info2;
        int ret;
        u16 v;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        info = hi6526_sysfs_field_lookup(attr->attr.name);
        if (!info)
                return -EINVAL;

        ret = kstrtou16(buf, 0, &v);
        if (ret < 0)
                return ret;

        if (!strncmp(("reg_value"), attr->attr.name, strlen("reg_value"))) {
                info2 = hi6526_sysfs_field_lookup("reg_addr");
                if (!info2)
                        return -EINVAL;
                info->reg = info2->reg;
        }
        if (!strncmp(("reg_addr"), attr->attr.name, strlen("reg_addr"))) {
                if (v < HI6526_REG_MAX) {
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
                        ret = hi6526_fcp_adapter_reg_write(di->sysfs_fcp_reg_val,
                                                                di->sysfs_fcp_reg_addr);
                        SCHARGER_INF(" sys write fcp adapter reg 0x%x , v 0x%x \n",
                                                      di->sysfs_fcp_reg_addr, di->sysfs_fcp_reg_val);

                        if (ret)
                                return ret;
#endif
                        return count;
        }

#ifdef CONFIG_HISI_DEBUG_FS
        ret = hi6526_write_mask(info->reg, info->mask, info->shift, v);
        if (ret)
                return ret;
#endif

        return count;
}

/**********************************************************
*  Function:       hi6526_sysfs_create_group
*  Description:    create the HI6526 device sysfs group
*  Parameters:   di:hi6526_device_info
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_sysfs_create_group(struct hi6526_device_info *di)
{
        hi6526_sysfs_init_attrs();

        return sysfs_create_group(&di->dev->kobj, &hi6526_sysfs_attr_group);
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Description:    remove the HI6526 device sysfs group
*  Parameters:   di:hi6526_device_info
*  return value:  NULL
**********************************************************/
static void hi6526_sysfs_remove_group(struct hi6526_device_info *di)
{
        sysfs_remove_group(&di->dev->kobj, &hi6526_sysfs_attr_group);
}
#else
static int hi6526_sysfs_create_group(struct hi6526_device_info *di)
{
        return 0;
}

static inline void hi6526_sysfs_remove_group(struct hi6526_device_info *di)
{
}
#endif

static void hi6526_opt_regs_set(struct opt_regs *opt, unsigned int len)
{
        unsigned int i;
        if(NULL == opt) {
                SCHARGER_ERR("%s is NULL!\n", __func__);
                return ;
        }
        for(i = 0; i < len; i++) {
                mdelay(opt[i].before);
                hi6526_write_mask(opt[i].reg, opt[i].mask, opt[i].shift, opt[i].val);
                mdelay(opt[i].after);
        }
}

/**********************************************************
*  Function:       hi6526_device_check
*  Description:    check chip i2c communication
*  Parameters:   null
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_device_check(void)
{
        int ret = 0;
        u32 chip_id = 0;

        ret |= hi6526_read_block(CHIP_VERSION_2, (u8 *) (&chip_id), 4);
        if (ret) {
                SCHARGER_ERR("[%s]:read chip_id fail\n", __func__);
                return CHARGE_IC_BAD;
        }

        if(CHIP_ID_6526 == chip_id) {
                SCHARGER_INF("%s, chip id is hi6526\n", __func__);
                return CHARGE_IC_GOOD;
        }

        SCHARGER_ERR("%s, failed, chip id is 0x%x \n", __func__, chip_id);
        return CHARGE_IC_BAD;
}
static int hi6526_get_device_version(void)
{
        unsigned int hi6526_version = 0;
        hi6526_read_block(CHIP_VERSION_0, (u8 *) (&hi6526_version), 2);
        SCHARGER_INF("%s, version is 0x%x \n", __func__, hi6526_version);

        return hi6526_version;
}

/**********************************************************
*  Function:       hi6526_set_anti_reverbst_reset
*  Description:    hi6526_set_anti_reverbst_reset
*  Parameters:    NULL
*  return value:  TRUE or FALSE
**********************************************************/
static void hi6526_set_anti_reverbst_reset(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return;

        hi6526_write_mask(CHG_ANTI_REVERBST_REG, CHG_ANTI_REVERBST_EN_MSK, CHG_ANTI_REVERBST_EN_SHIFT, CHG_ANTI_REVERBST_DIS);
        queue_delayed_work(system_power_efficient_wq, &di->reverbst_work,
        		      msecs_to_jiffies(0));

        return ;
}

static int set_buck_mode_enable(int enable)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        SCHARGER_INF("%s  :%d\n", __func__, enable);
        if(enable)
                hi6526_set_anti_reverbst_reset();

        hi6526_write_mask(CHG_BUCK_MODE_CFG, CHG_BUCK_MODE_CFG_MASK, CHG_BUCK_MODE_CFG_SHIFT, !!enable);
        if(enable) {
                di->batt_ovp_cnt_30s = 0;
                di->chg_mode = BUCK;
        } else if(BUCK == di->chg_mode){
                di->chg_mode = NONE;
        }
        hi6526_write_mask(CHG_HIZ_CTRL_REG, CHG_HIZ_ENABLE_MSK, CHG_HIZ_ENABLE_SHIFT, !enable);

        return 0;
}

/**********************************************************
*  Function:       hi6526_set_bat_comp
*  Description:    set the bat comp
                                schargerv100 can't set ir comp due to lx bug
*  Parameters:   value:bat_comp mohm
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_bat_comp(int value)
{
        u8 reg;

        if (value < CHG_IR_COMP_MIN)
                value = CHG_IR_COMP_MIN;
        else if(value > CHG_IR_COMP_MAX)
                value = CHG_IR_COMP_MAX;

        reg = value / CHG_IR_COMP_STEP_15;

        return hi6526_write_mask(CHG_IR_COMP_REG, CHG_IR_COMP_MSK,
                                 CHG_IR_COMP_SHIFT, reg);
}

/**********************************************************
*  Function:       hi6526_set_vclamp
*  Description:    set the vclamp
*  Parameters:   value:vclamp mv
                                schargerv100 can't set vclamp due to lx bug
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_vclamp(int value)
{
        u8 reg;

        if (value < CHG_IR_VCLAMP_MIN)
                value = CHG_IR_VCLAMP_MIN;
        else if (value > CHG_IR_VCLAMP_MAX)
                value = CHG_IR_VCLAMP_MAX;
        else {
                //do nothing
        }
        reg = value / CHG_IR_VCLAMP_STEP;
        return hi6526_write_mask(CHG_IR_VCLAMP_REG, CHG_IR_VCLAMP_MSK,
                                 CHG_IR_VCLAMP_SHIFT, reg);
}

/**********************************************************
*  Function:       hi6526_set_adc_channel
*  Description:    select adc channel
*  Parameters:     channel
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_adc_channel(u32 chan)
{
        u8 val_l = 0x00;
        u8 val_h = 0x00;

        if(chan < CHG_ADC_CH_L_MAX) {
                if(chan == CHG_ADC_CH_IBUS) {
                        val_h |= (1 << (CHG_ADC_CH_IBUS_REF - CHG_ADC_CH_L_MAX));
                }
                val_l = (1 << chan);
        } else {
                val_h = (1 << (chan - CHG_ADC_CH_L_MAX));
        }
        hi6526_write(CHG_ADC_CH_SEL_L, val_l);
        hi6526_write(CHG_ADC_CH_SEL_H, val_h);

        return 0;
}

/**********************************************************
*  Function:       hi6526_adc_enable
*  Description:    enable hi6526 adc
*  Parameters:   value: 1(enable) or 0(disable)
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_adc_enable(u32 enable)
{

        if(enable) {
                hi6526_write_mask(CHG_ADC_START_REG, CHG_ADC_START_MSK,
		                        	      CHG_ADC_START_SHIFT, FALSE);
                hi6526_write(CHG_ADC_CTRL1, CHG_ADC_CTRL1_DEFAULT_VAL);
                hi6526_write(CHG_ADC_CTRL_REG, 0x90);
        } else
                hi6526_write(CHG_ADC_CTRL_REG, 0x00);

        return 0;
}

/**********************************************************
*  Function:       hi6526_adc_conv_status
*  Description:    get hi6526 adc conv_status
*  Parameters:     null
*  return value:  0-in conv or others-fail
**********************************************************/
static int hi6526_get_adc_conv_status(u8 * value)
{

        hi6526_read(CHG_ADC_CONV_STATUS_REG, value);

        if(*value & CHG_PULSE_NO_CHG_FLAG_MSK) {
                *value = 0;
                return -1;
        }

        *value = !!(*value & CHG_ADC_CONV_STATUS_MSK);
        return 0;
}

static int hi6526_adc_loop_enable(int enable)
{
        static int flag = 0;

        if(enable && !flag) {
                hi6526_write(CHG_ADC_CTRL1, CHG_ADC_CTRL1_DEFAULT_VAL);  // 0x2B = 0x67

                hi6526_write(CHG_ADC_CH_SEL_H, 0x39);  // sel ibus_ref/TSBAT/TSBUS/TSCHIP  // 0x2c = 0x39
                hi6526_write(CHG_ADC_CH_SEL_L, 0x3F);  // sel vusb/ibas/vbas/vout/vbat/ibat // 0x2d = 0x3f

                hi6526_write(CHG_ADC_CTRL_REG, 0x30); // loop       // 0x29 = 0x30
                hi6526_write(CHG_ADC_CTRL_REG, 0xB0); // enable adc and mult channels loop
                hi6526_write(CHG_ADC_START_REG, 0x01); // start conver   // 0x2A = 0x01
                flag = 1;
                mdelay(2);
        } else if(!enable) {
                hi6526_write(CHG_ADC_CTRL_REG, 0x0); // disable adc and mult channels loop   // 0x29 = 0
                flag = 0;
        }

        return 0;
}

/**********************************************************
*  Function:       hi6526_get_adc_value
*  Description:    set covn start
*  Parameters:     chan:adc channel ,data :adc value
*  return value:   0-success or others-fail
**********************************************************/
static int hi6526_get_adc_value(u32 chan, u32 * data)
{
        int ret = 0;
        u8 reg = 0;
        u8 lvc_mode = 0, sc_mode = 0;
        int i = 0;
        u8 adc_data[2] = { 0 };
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        mutex_lock(&di->adc_conv_lock);

        hi6526_read_mask(LVC_CHG_MODE_REG, LVC_CHG_MODE_MASK,LVC_CHG_MODE_SHIFT, &lvc_mode);
        hi6526_read_mask(SC_CHG_MODE_REG, SC_CHG_MODE_MASK,SC_CHG_MODE_SHIFT, &sc_mode);

        /* multi-channel & loop mode */
        if(lvc_mode || sc_mode) {

                hi6526_adc_loop_enable(CHG_ADC_EN);

                /*request data*/
                hi6526_write(CHG_ADC_RD_SEQ,0x01);

                ret |= hi6526_read(CHG_ADC_DATA_L_REG + (chan * 2), &adc_data[0]);
                ret |= hi6526_read(CHG_ADC_DATA_H_REG + (chan * 2), &adc_data[1]);

                *data = (u32) adc_data[0] | ((u32)(adc_data[1] & 0x3f) << 8);

                if((chan == CHG_ADC_CH_VBUS || chan == CHG_ADC_CH_VUSB) && lvc_mode) {
                        *data = *data * 4 /10 + 1500;
                }

                mutex_unlock(&di->adc_conv_lock);
                return 0;
        }

        /* signel-channel & signel mode */
        ret |= hi6526_adc_loop_enable(CHG_ADC_DIS);
        ret |= hi6526_set_adc_channel(chan);
        ret |= hi6526_adc_enable(CHG_ADC_EN);

        ret |=
            hi6526_write_mask(CHG_ADC_START_REG, CHG_ADC_START_MSK,
                                CHG_ADC_START_SHIFT, TRUE);
        if (ret) {
                SCHARGER_ERR("set covn fail! ret =%d \n", ret);
                hi6526_adc_enable(CHG_ADC_DIS);
                mutex_unlock(&di->adc_conv_lock);
                return -1;
        }
        /*The conversion result is ready after tCONV, max 10ms */
        for (i = 0; i < 10; i++) {
                ret = hi6526_get_adc_conv_status(&reg);
                if (ret) {
                        SCHARGER_ERR(" HI6526 read ADC CONV STAT fail!.\n");
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
                hi6526_adc_enable(CHG_ADC_DIS);
                mutex_unlock(&di->adc_conv_lock);
                return -1;
        }

        ret |= hi6526_read(CHG_ADC_DATA_L_REG + (chan * 2), &adc_data[0]);
        ret |= hi6526_read(CHG_ADC_DATA_H_REG + (chan * 2), &adc_data[1]);

        *data = (u32) adc_data[0] | ((u32)(adc_data[1] & 0x3f) << 8);

        ret |= hi6526_adc_enable(CHG_ADC_DIS);
        if (ret) {
                SCHARGER_ERR("[%s]get ibus_ref_data fail,ret:%d\n", __func__,
                	     ret);
                hi6526_adc_enable(CHG_ADC_DIS);
                mutex_unlock(&di->adc_conv_lock);
                return -1;
        }

        mutex_unlock(&di->adc_conv_lock);
        return 0;
}

/**********************************************************
*  Function:     hi6526_set_fast_safe_timer()
*  Description:  set fast safe timer
*  Parameters:   safe timer value
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6526_set_fast_safe_timer(u32 chg_fastchg_safe_timer)
{
        return hi6526_write_mask(CHG_FASTCHG_TIMER_REG, CHG_FASTCHG_TIMER_MSK,
                                 CHG_FASTCHG_TIMER_SHIFT,
                                 (u8) chg_fastchg_safe_timer);
}

/**********************************************************
*  Function:     hi6526_set_recharge_vol()
*  Description:  set rechg vol
*  Parameters:  set rechg vol
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6526_set_recharge_vol(u8 rechg)
{
        return hi6526_write_mask(CHG_RECHG_REG, CHG_RECHG_MSK, CHG_RECHG_SHIFT, rechg);
}

/**********************************************************
*  Function:     hi6526_set_precharge_current()
*  Description:  config precharge current limit
*  Parameters:   precharge current
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6526_set_precharge_current(int precharge_current)
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

        return hi6526_write_mask(CHG_PRE_ICHG_REG, CHG_PRE_ICHG_MSK,
                                 CHG_PRE_ICHG_SHIFT, prechg_limit);
}

/**********************************************************
*  Function:     hi6526_set_precharge_voltage()
*  Description:  config precharge voltage
*  Parameters:   precharge voltage
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6526_set_precharge_voltage(u32 pre_vchg)
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
                vprechg = 0;        /*default 2.8V */
        return hi6526_write_mask(CHG_PRE_VCHG_REG, CHG_PRE_VCHG_MSK,
                                 CHG_PRE_VCHG_SHIFT, vprechg);
}

/**********************************************************
*  Function:     hi6526_set_batfet_ctrl()
*  Description:  config batfet status 1:enable, 0: disable
*  Parameters:   status
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6526_set_batfet_ctrl(u32 status)
{
        return hi6526_write_mask(BATFET_CTRL_CFG_REG, BATFET_CTRL_CFG_MSK,
                                 BATFET_CTRL_CFG_SHIFT, status);
}

/**********************************************************
*  Function:       hi6526_get_charge_enable
*  Description:    get the charge enable in charging process
*  Parameters:     void
*  return value:   TRUE or FALSE
**********************************************************/
static bool hi6526_get_charge_enable(void)
{
        u8 charge_state = 0;

        hi6526_read_mask(CHG_ENABLE_REG, CHG_EN_MSK, CHG_EN_SHIFT, &charge_state);

        if (charge_state) {
                return TRUE;
        } else {
                return FALSE;
        }
}

/****************************************************************************
  Function:     hi6526_set_vbus_uvp_ovp
  Description:  hi6526 config vbus uvp fvp
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6526_set_vbus_uvp_ovp(int vbus)
{
        int ret = 0;
        u8 ov_vol, uv_vol;

        if(vbus < VBUS_VSET_9V)
                ov_vol = 0 ;
        else if (vbus < VBUS_VSET_12V)
                ov_vol = 1;
        else ov_vol = 2;

        uv_vol = 0; // 3.8V

        ret |=
                hi6526_write_mask(CHG_OVP_VOLTAGE_REG, CHG_BUCK_OVP_VOLTAGE_MSK,
                                  CHG_BUCK_OVP_VOLTAGE_SHIFT, ov_vol);
        ret |=
                hi6526_write_mask(CHG_UVP_VOLTAGE_REG, CHG_BUCK_UVP_VOLTAGE_MSK,
                                  CHG_BUCK_UVP_VOLTAGE_SHIFT, uv_vol);
        if (ret) {
                SCHARGER_ERR("%s:uvp&ovp voltage set failed, ret = %d.\n",
                             __func__, ret);
                return -1;
        }

        return ret;
}


/**********************************************************
*  Function:     hi6526_set_vbus_vset()
*  Description:  set vbus_vset voltage
*  Parameters:   vbus_set voltage: 5V/9V/12V
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6526_set_vbus_vset(u32 value)
{
        u8 data = 0;
        u32 charger_flag = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
         if (NULL == di) {
                 SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                 return -ENOMEM;
         }

        SCHARGER_INF("%s : %d v\n", __func__, value);

        /*check charge state, if open, close charge.*/
        if (TRUE == hi6526_get_charge_enable()) {
                hi6526_set_charge_enable(CHG_DISABLE);
                charger_flag = 1;
        }
        if (value < VBUS_VSET_9V) {
                charger_is_fcp = FCP_FALSE;
                data = 0;
        } else if (value < VBUS_VSET_12V) {
                charger_is_fcp = FCP_TRUE;
                data = 1;
        } else {
                data = 2;
        }
        di->buck_vbus_set = value;

        /*resume charge state*/
        if (1 == charger_flag) {
                hi6526_set_charge_enable(CHG_ENABLE);
        }

        hi6526_set_vbus_uvp_ovp(value);

        return hi6526_write_mask(CHG_VBUS_VSET_REG, VBUS_VSET_MSK,
                                 VBUS_VSET_SHIFT, data);
}

/**********************************************************
*  Function:     hi6526_config_opt_param()
*  Description:  config opt parameters for hi6526
*  Parameters:   vbus_vol:5V/9V/12V
*  return value:
*                 0-success or others-fail
**********************************************************/
static void hi6526_buck_opt_param(void)
{
        hi6526_opt_regs_set(buck_opt_regs, ARRAY_SIZE(buck_opt_regs));
}

static int hi6526_config_opt_param(int vbus_vol)
{
        hi6526_buck_opt_param();
        hi6526_set_vbus_vset(vbus_vol);
        return 0;
}

/**********************************************************
*  Function:       hi6526_set_input_current
*  Description:    set the input current in charging process
*  Parameters:   value:input current value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_input_current(int cin_limit)
{
        u8 Iin_limit;
        int max;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        if(di->buck_vbus_set < VBUS_VSET_9V) {
                max = CHG_ILIMIT_2500;
        } else if (di->buck_vbus_set < VBUS_VSET_12V) {
                max = CHG_ILIMIT_1400;
        } else {
                max = CHG_ILIMIT_1100;
        }

        if(di->input_limit_flag) {
		if(di->buck_vbus_set < VBUS_VSET_9V) {
			max = CHG_ILIMIT_1100;
                } else if (di->buck_vbus_set < VBUS_VSET_12V) {
			max = CHG_ILIMIT_600;
                } else {
			max = CHG_ILIMIT_475;
                }
        }

	di->input_current = cin_limit;

	if(cin_limit > max) {
                SCHARGER_DBG("%s cin_limit %d, max %d, vbus set is %d \n", __func__, cin_limit, max, di->buck_vbus_set);
                cin_limit = max;
        }

        if (cin_limit <= CHG_ILIMIT_85)
                Iin_limit = 0;
        else if (cin_limit > CHG_ILIMIT_85 && cin_limit <= CHG_ILIMIT_130)
                Iin_limit = 1;
        else if (cin_limit > CHG_ILIMIT_130 && cin_limit <= CHG_ILIMIT_200)
                Iin_limit = 2;
        else if (cin_limit > CHG_ILIMIT_200 && cin_limit <= CHG_ILIMIT_300)
                Iin_limit = 3;
        else if (cin_limit > CHG_ILIMIT_300 && cin_limit <= CHG_ILIMIT_400)
                Iin_limit = 4;
        else if (cin_limit > CHG_ILIMIT_400 && cin_limit <= CHG_ILIMIT_475)
                Iin_limit = 5;
        else if (cin_limit > CHG_ILIMIT_475 && cin_limit <= CHG_ILIMIT_600)
                Iin_limit = 6;
        else if (cin_limit > CHG_ILIMIT_600 && cin_limit <= CHG_ILIMIT_700)
                Iin_limit = 7;
        else if (cin_limit > CHG_ILIMIT_700 && cin_limit <= CHG_ILIMIT_800)
                Iin_limit = 8;
        else if (cin_limit > CHG_ILIMIT_800 && cin_limit <= CHG_ILIMIT_825)
                Iin_limit = 9;
        else if (cin_limit > CHG_ILIMIT_825 && cin_limit <= CHG_ILIMIT_1000)
                Iin_limit = 10;
        else {
                Iin_limit = cin_limit / CHG_ILIMIT_STEP_100;
       }
        SCHARGER_DBG("%s : cin_limit %d ma, reg is set 0x%x\n", __func__, cin_limit, Iin_limit);
        SCHARGER_DBG("%s : flag %d, buck_vbus_set %d\n", __func__, di->input_limit_flag, di->buck_vbus_set);

        return hi6526_write_mask(CHG_INPUT_SOURCE_REG, CHG_ILIMIT_MSK,
                                 CHG_ILIMIT_SHIFT, Iin_limit);
}
/**********************************************************
*  Function:       hi6526_set_input_current_limit
*  Description:    set the input current in charging process
*  Parameters:   value:input current value
*  return value:  0-success or others-fail
**********************************************************/
static void hi6526_set_input_current_limit(int enable)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return;
        }
        SCHARGER_INF("%s , flag %d, input current %d, vbus vset %d, enable %d\n", __func__ ,
                di->input_limit_flag, di->input_current, di->buck_vbus_set, enable);

        di->input_limit_flag = enable;

        if(enable) {
                if(di->buck_vbus_set < VBUS_VSET_9V) {
                        if(di->input_current > CHG_ILIMIT_1100)
                                hi6526_set_input_current(CHG_ILIMIT_1100);
                } else if(di->buck_vbus_set < VBUS_VSET_12V) {
                        if(di->input_current > CHG_ILIMIT_600)
                                hi6526_set_input_current(CHG_ILIMIT_600);
                } else {
                        if(di->input_current > CHG_ILIMIT_475)
                                hi6526_set_input_current(CHG_ILIMIT_475);
                }
        } else {
                hi6526_set_input_current(di->input_current);
        }
}

/**********************************************************
*  Function:       hi6526_get_charge_current
*  Description:    get the charge current in charging process
*  Parameters:   NULL
*  return value:  the charge current
**********************************************************/
static int hi6526_get_charge_current(void)
{
        int ret = 0;
        u8 reg = 0;
        int charge_current = 0;

        ret = hi6526_read_mask(CHG_FAST_CURRENT_REG, CHG_FAST_ICHG_MSK, CHG_FAST_ICHG_SHIFT,
                &reg);
        if(ret)
        {
                SCHARGER_ERR("HI6526 read mask fail\n");
                return CHG_FAST_ICHG_00MA;
        }

        charge_current = (reg + 1) * CHG_FAST_ICHG_STEP_100;

        SCHARGER_DBG("charge current is set %d %d\n", charge_current,reg);
        return charge_current;

}

/**********************************************************
*  Function:       hi6526_get_charge_current_bias
*  Description:    calc the charge current bias in charging process
*  Parameters:   value:charge current value
*  return value:  charge currernt with bias
**********************************************************/
static int hi6526_calc_charge_current_bias(int charge_current)
{
        int I_coul_ma = 0;
        int I_bias = 0;
        int I_delta_ma = 0;
        static int last_ichg = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        mutex_lock(&di->ibias_calc_lock);
    /*if target is less than 1A,need not to calc ibias,need not to minus bias calculated before*/
        if (CHG_FAST_ICHG_1000MA > charge_current){
                last_ichg = charge_current;
                mutex_unlock(&di->ibias_calc_lock);
                return charge_current;
        }
    /*if target charge current changed, no need to calc ibias,just use bias calculated before*/
        if (last_ichg != charge_current) {
                last_ichg = charge_current;
                charge_current -= I_bias_all;
                mutex_unlock(&di->ibias_calc_lock);
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
        mutex_unlock(&di->ibias_calc_lock);
        return charge_current;
}
/**********************************************************
*  Function:       hi6526_set_charge_current
*  Description:    set the charge current in charging process
*  Parameters:   value:charge current value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_charge_current(int charge_current)
{
        u8 Ichg_limit;

        charge_current = hi6526_calc_charge_current_bias(charge_current);

        /* Chip limit */
        if(charge_current > CHG_FAST_ICHG_2500MA)
                charge_current = CHG_FAST_ICHG_2500MA;

        Ichg_limit = (charge_current / CHG_FAST_ICHG_STEP_100) - 1;

        SCHARGER_DBG("%s :%d,  reg is set 0x%x\n" , __func__, charge_current, Ichg_limit);
        return hi6526_write_mask(CHG_FAST_CURRENT_REG, CHG_FAST_ICHG_MSK,
                                 CHG_FAST_ICHG_SHIFT, Ichg_limit);
}

/**********************************************************
*  Function:       hi6526_set_terminal_voltage
*  Description:    set the terminal voltage in charging process
                (v300&v310 scharger's max cv is 4.25V due to lx bug)
*  Parameters:   value:terminal voltage value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_terminal_voltage(int charge_voltage)
{
        u8 data;
        struct hi6526_device_info *di = g_hi6526_dev;
        SCHARGER_DBG("%s charge_voltage : %d \n", __func__, charge_voltage);

        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        if (charge_voltage < CHG_FAST_VCHG_MIN) {
                charge_voltage = CHG_FAST_VCHG_MIN;
        } else if (charge_voltage > CHG_FAST_VCHG_MAX) {
                charge_voltage = CHG_FAST_VCHG_MAX;
        } else {
                //do nothing
        }

        di->term_vol_mv = charge_voltage;

        data =
            (u8) ((charge_voltage - CHG_FAST_VCHG_MIN)  * 1000 / CHG_FAST_VCHG_STEP_16600UV);
        return hi6526_write_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
                                 CHG_FAST_VCHG_SHIFT, data);
}

/**********************************************************
*  Function:       hi6526_get_terminal_voltage
*  Description:    get the terminal voltage in charging process
*  Parameters:   value:terminal voltage value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_get_terminal_voltage(void)
{
        u8 data = 0;

        hi6526_read_mask(CHG_FAST_VCHG_REG, CHG_FAST_VCHG_MSK,
                                 CHG_FAST_VCHG_SHIFT, &data);
        return (int)((data * CHG_FAST_VCHG_STEP_16600UV + CHG_FAST_VCHG_BASE_UV) / 1000);
}
/**********************************************************
*  Function:       hi6526_check_input_dpm_state
*  Description:    check whether VINDPM or IINDPM
*  Parameters:     NULL
*  return value:   TRUE means VINDPM or IINDPM
*                  FALSE means NoT DPM
**********************************************************/
static int hi6526_check_input_dpm_state(void)
{
        u8 reg = 0;
        int ret;

        ret = hi6526_read(CHG_R0_REG_STATUE, &reg);
        if (ret < 0) {
                SCHARGER_ERR("hi6526_check_input_dpm_state err\n");
                return ret;
        }

        if (CHG_IN_DPM_STATE == (reg & CHG_IN_DPM_STATE)) {
                SCHARGER_INF("CHG_STATUS0_REG:%x in vdpm state.\n",reg);
                return TRUE;
        }

        return FALSE;
}

/**********************************************************
*  Function:       hi6526_check_input_acl_state
*  Description:    check whether acl
*  Parameters:     NULL
*  return value:   TRUE means acl
*                  FALSE means NoT acl
**********************************************************/
static int hi6526_check_therm_state(void)
{
        u8 reg = 0;
        int ret = 0;

        ret = hi6526_read(CHG_R0_REG2_STATUE, &reg);

        if (ret < 0) {
                SCHARGER_ERR("hi6526_check_input_dpm_state err\n");
                return ret;
        }

        if (CHG_IN_THERM_STATE == (reg & CHG_IN_THERM_STATE)) {
                return TRUE;
        }

        return FALSE;
}

/**********************************************************
*  Function:       hi6526_check_input_acl_state
*  Description:    check whether acl
*  Parameters:     NULL
*  return value:   TRUE means acl
*                  FALSE means NoT acl
**********************************************************/
static int hi6526_check_input_acl_state(void)
{
        u8 reg = 0;
        int ret;

        ret = hi6526_write_mask(CHG_ACL_RPT_EN_REG, CHG_ACL_PRT_EN_MASK,
                                        CHG_ACL_RPT_EN_SHIFT, true);
        ret |= hi6526_read(CHG_R0_REG_STATUE, &reg);
        ret |= hi6526_write_mask(CHG_ACL_RPT_EN_REG, CHG_ACL_PRT_EN_MASK,
                                        CHG_ACL_RPT_EN_SHIFT, true);

        if (ret < 0) {
                SCHARGER_ERR("hi6526_check_input_acl_state err\n");
                return ret;
        }

        if (CHG_IN_ACL_STATE == (reg & CHG_IN_ACL_STATE)) {
                SCHARGER_INF("CHG_STATUS0_REG:%x in acl state.\n",reg);
                return TRUE;
        }

        return FALSE;
}


/**********************************************************
*  Function:       hi6526_get_charge_state
*  Description:    get the charge states in charging process
*  Parameters:   state:charge states
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_get_charge_state(unsigned int *state)
{
        u8 reg0 = 0, reg1 = 0, reg2 = 0;
        int ret = 0;
        *state = 0;

        ret |= hi6526_read(CHG_BUCK_STATUS_REG, &reg0);
        ret |= hi6526_read(CHG_STATUS_REG, &reg1);
        ret |= hi6526_read(WATCHDOG_STATUS_REG, &reg2);
        if (ret) {
                SCHARGER_ERR("[%s]read charge status reg fail,ret:%d\n",
                             __func__, ret);
                return -1;
        }

        if (HI6526_CHG_BUCK_OK != (reg0 & HI6526_CHG_BUCK_OK))
                *state |= CHAGRE_STATE_NOT_PG;
        if (HI6526_CHG_STAT_CHARGE_DONE == (reg1 & HI6526_CHG_STAT_CHARGE_DONE))
                *state |= CHAGRE_STATE_CHRG_DONE;
        if (HI6526_WATCHDOG_OK != (reg2 & HI6526_WATCHDOG_OK))
                *state |= CHAGRE_STATE_WDT_FAULT;

        SCHARGER_INF("%s >>> reg0:0x%x, reg1 0x%x, reg2 0x%x, state 0x%x \n", __func__, reg0, reg1, reg2, *state);

        return ret;
}

static void hi6526_reverbst_delay_work(struct work_struct *work)
{
        u8 val = 0;
        int count = 10;
        struct hi6526_device_info *di = g_hi6526_dev;

        if (NULL == di)
                return;

        msleep(400);
        do {
                hi6526_read(CHG_BUCK_STATUS_REG, &val);
                mdelay(5);
        } while ((HI6526_CHG_BUCK_OK != (val & HI6526_CHG_BUCK_OK)) && count--);

        if(HI6526_CHG_BUCK_OK == (val & HI6526_CHG_BUCK_OK)) {
                mdelay(50);
                hi6526_write_mask(CHG_ANTI_REVERBST_REG, CHG_ANTI_REVERBST_EN_MSK, CHG_ANTI_REVERBST_EN_SHIFT, CHG_ANTI_REVERBST_EN);
        }
}

/**********************************************************
*  Function:       hi6526_set_terminal_current
*  Description:    set the terminal current in charging process
*                   (min value is 400ma for scharger ic bug)
*  Parameters:   value:terminal current value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_terminal_current(int term_current)
{
        u8 Iterm;

        if (term_current < CHG_TERM_ICHG_200MA)
                Iterm = 0;
        else if (term_current >= CHG_TERM_ICHG_200MA
                 && term_current < CHG_TERM_ICHG_300MA)
                Iterm = 1;
        else if (term_current >= CHG_TERM_ICHG_300MA
                 && term_current < CHG_TERM_ICHG_400MA)
                Iterm = 2;
        else
                Iterm = 3;        /*default 400mA */

        /* for chip bug */
        Iterm = 3;        /*default 400mA */

        SCHARGER_DBG(" term_current: %d, term current reg is set 0x%x\n",term_current, Iterm);
        return hi6526_write_mask(CHG_TERM_ICHG_REG, CHG_TERM_ICHG_MSK,
                                 CHG_TERM_ICHG_SHIFT, Iterm);
}

/**********************************************************
*  Function:       hi6526_set_charge_enable
*  Description:    set the charge enable in charging process
*  Parameters:     enable:charge enable or not
*  return value:   0-success or others-fail
**********************************************************/
static int hi6526_set_charge_enable(int enable)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        static int last_enable = 0;
        if (NULL == di)
                return -ENOMEM;
        /*invalidate charge enable on udp board */
        if ((BAT_BOARD_UDP == di->is_board_type) && (CHG_ENABLE == enable))
                return 0;

        if(enable && !last_enable) {
                hi6526_set_input_current_limit(0);
        } else if(!enable && last_enable){
                hi6526_set_input_current_limit(1);
        }

        last_enable = enable;
        return hi6526_write_mask(CHG_ENABLE_REG, CHG_EN_MSK, CHG_EN_SHIFT,
                                 enable);
}

/**********************************************************
*  Function:       hi6526_set_otg_current
*  Description:    set the otg mdoe current
*  Parameters:     value :current value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_otg_current(int value)
{
        unsigned int temp_currentmA = 0;
        u8 reg = 3;
        temp_currentmA = value;

        if (temp_currentmA < BOOST_LIM_MIN || temp_currentmA > BOOST_LIM_MAX)
                SCHARGER_INF("set otg current %dmA is out of range!\n", value);
        if (temp_currentmA < BOOST_LIM_500) {
                reg = 0;
        } else if (temp_currentmA >= BOOST_LIM_500
                   && temp_currentmA < BOOST_LIM_1000) {
                reg = 0;
        } else if (temp_currentmA >= BOOST_LIM_1000
                   && temp_currentmA < BOOST_LIM_1500) {
                reg = 1;
        } else if (temp_currentmA >= BOOST_LIM_1500
                   && temp_currentmA < BOOST_LIM_2000) {
                reg = 3;
        } else {
                reg = 3;
        }

        SCHARGER_DBG(" otg current reg is set 0x%x\n", reg);
        return hi6526_write_mask(CHG_OTG_REG0, CHG_OTG_LIM_MSK,
                                 CHG_OTG_LIM_SHIFT, reg);
}

static int hi6526_otg_switch_mode(int enable)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }
        SCHARGER_INF("%s enable %d \n", __func__, enable);

        hi6526_write_mask(CHG_OTG_SWITCH_CFG_REG, CHG_OTG_SWITCH_MASK, CHG_OTG_SWITCH_SHIFT, !!enable);
        return 0;
}


/**********************************************************
*  Function:       hi6526_set_otg_enable
*  Description:    set the otg mode enable in charging process
*  Parameters:   enable:otg mode  enable or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_otg_enable(int enable)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        if (enable) {
                hi6526_opt_regs_set(otg_opt_regs, ARRAY_SIZE(otg_opt_regs));
                hi6526_set_charge_enable(CHG_DISABLE);
        } else {
                hi6526_otg_switch_mode(0);
        }

        hi6526_write_mask(CHG_OTG_CFG_REG, CHG_OTG_EN_MSK, CHG_OTG_EN_SHIFT,
                                         enable);
        if(!enable) {
                mdelay(50);
        }
        hi6526_write_mask(CHG_OTG_CFG_REG_0, CHG_OTG_MODE_MSK, CHG_OTG_MODE_SHIFT,
                                         enable);

        return 0;
}

/**********************************************************
*  Function:       hi6526_set_term_enable
*  Description:    set the terminal enable in charging process
*  Parameters:   enable:terminal enable or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_term_enable(int enable)
{
        int chg_state = 0;
        int vbatt_mv;
        int term_mv;

        if (CHG_STAT_ENABLE == hi6526_force_set_term_flag) {
                SCHARGER_INF("Charger is in the production line testing phase!\n");
                return 0;
        }

        if(enable) {
                chg_state = hi6526_check_input_dpm_state();
                chg_state |= hi6526_check_input_acl_state();
                chg_state |= hi6526_check_therm_state();
                vbatt_mv = hisi_battery_voltage();
                term_mv = hi6526_get_terminal_voltage();
                if(chg_state || (vbatt_mv < (term_mv - 100))) {
                        SCHARGER_INF("%s enable:%d, %d, but in dpm or acl or thermal state\n", __func__, enable, chg_state);
                        enable = 0;
                } else {
                        SCHARGER_INF("%s enable:%d\n", __func__, enable);
                }
        }

        return hi6526_write_mask(CHG_EN_TERM_REG, CHG_EN_TERM_MSK,
					 CHG_EN_TERM_SHIFT, !!enable);
}

/**********************************************************
*  Function:       hi6526_force_set_term_enable
*  Description:    set the terminal enable in charging process
*  Parameters:   enable:terminal enable or not
*                0&1:dbc control. 2:original charger procedure
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_force_set_term_enable(int enable)
{
        SCHARGER_INF("%s enable:%d\n", __func__, enable);

        if ((0 == enable) || (1 == enable)) {
                hi6526_force_set_term_flag = CHG_STAT_ENABLE;
        } else {
                hi6526_force_set_term_flag = CHG_STAT_DISABLE;
                return 0;
        }
        return hi6526_write_mask(CHG_EN_TERM_REG, CHG_EN_TERM_MSK,
                                         CHG_EN_TERM_SHIFT, (u8)enable);
}

/**********************************************************
*  Function:     hi6526_get_charger_state()
*  Description:  get charger state
*  Parameters:   NULL
*  return value:
*       charger state:
*        1:Charge Termination Done
*        0:Not Charging and buck is closed;Pre-charge;Fast-charg;
*       -1:error
**********************************************************/
static int hi6526_get_charger_state(void)
{
        u8 data = 0;
        int ret = -1;

        ret = hi6526_read(CHG_STATUS_REG, &data);
        if (ret) {
                SCHARGER_ERR("[%s]:read charge status reg fail.\n", __func__);
                return -1;
        }

        data &= HI6526_CHG_STAT_MASK;
        data = data >> HI6526_CHG_STAT_SHIFT;
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
*  Function:       hi6526_get_vbus_mv
*  Description:    get voltage of vbus
*  Parameters:   vbus_mv:voltage of vbus
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_get_vbus_mv(unsigned int *vbus_mv)
{
        int ret;
        u32 result = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return -1;

        ret = hi6526_get_adc_value(CHG_ADC_CH_VBUS, &result);
        if (ret) {
                SCHARGER_ERR("[%s]get vbus_mv fail,ret:%d\n", __func__, ret);
                return -1;
        }

        if(CHARGER_TYPE_NONE == di->charger_type && result < VBUS_2600_MV)
                result = 0;

        *vbus_mv = result;
        return ret;
}
static int hi6526_get_vbus_mv2(int *vbus_mv)
{
        int ret;
        u32 result = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return -1;

        ret = hi6526_get_adc_value(CHG_ADC_CH_VBUS, &result);
        if (ret) {
                SCHARGER_ERR("[%s]get vbus_mv fail,ret:%d\n", __func__, ret);
                return -1;
        }

        if(CHARGER_TYPE_NONE == di->charger_type && result < VBUS_2600_MV)
                result = 0;

        *vbus_mv = (int)result;
        return ret;
}

/**********************************************************
*  Function:       hi6526_get_vout
*  Description:    get voltage of vout
*  Parameters:   vout_mv:voltage of vout
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_get_vout(int *vout_mv)
{
        int ret;
        u32 result = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return -1;

        ret = hi6526_get_adc_value(CHG_ADC_CH_VOUT, &result);
        if (ret) {
                SCHARGER_ERR("[%s]get vout_mv fail,ret:%d\n", __func__, ret);
                return -1;
        }

        *vout_mv = (int)result;
        return ret;
}

/**********************************************************
*  Function:       hi6526_get_vusb
*  Description:    get voltage of vusb
*  Parameters:   vusb_mv:voltage of vout
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_get_vusb(int *vusb_mv)
{
        int ret;
        u32 result = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return -1;

        ret = hi6526_get_adc_value(CHG_ADC_CH_VUSB, &result);
        if (ret) {
                SCHARGER_ERR("[%s]get vusb_mv fail,ret:%d\n", __func__, ret);
                return -1;
        }

        *vusb_mv = (int)result;
        return ret;
}

static int hi6526_get_dp_dm_volt(int ch)
{
        u32 vol_mv = 0;

        hi6526_write_mask(DPDM_PMID_SEL_REG, DPDM_PMID_SEL_MASK, DPDM_PMID_SEL_SHIFT, SEL_DPDM);
        hi6526_write_mask(CHG_ADC_APPDET_REG, CHG_ADC_APPDET_CHSEL_MSK, CHG_ADC_APPDET_CHSEL_SHIFT, ch);
        hi6526_write_mask(CHG_ADC_APPDET_REG, APPLE_DETECT_MASK, APPLE_DETECT_SHIFT, APPLE_DETECT_ENABLE);

        hi6526_get_adc_value(CHG_ADC_CH_DPDM, &vol_mv);

        hi6526_write_mask(CHG_ADC_APPDET_REG, APPLE_DETECT_MASK, APPLE_DETECT_SHIFT, APPLE_DETECT_DISABLE);

        vol_mv = vol_mv * 3750 / 4096;

        SCHARGER_DBG("[%s] %d\n", __func__, vol_mv);

        return vol_mv;
}

static void hi6526_dp_res_det_iqsel(int det_current)
{
        u8 code;

        if(det_current == IQ_SEL_1UA) {
                code = 0x00;
        } else if(det_current == IQ_SEL_10UA){
                code = 0x01;
        } else if(det_current == IQ_SEL_100UA) {
                code = 0x03;
        } else {
                code = 0x03;
                SCHARGER_INF("[%s]  det_current is error %d\n", __func__, det_current);
        }

        hi6526_write_mask(DP_RES_IQSEL_REG, DP_RES_IQSEL_MASK, DP_RES_IQSEL_SHIFT, det_current);
}

static int hi6526_get_dp_res(void)
{
        u32 code = 0;
        int res;
        int irsel = IQ_SEL_1UA;
        int dp_volt = 0;

        do{
                dp_volt = hi6526_get_dp_dm_volt(DP_DET);
                irsel = irsel * 10;
                hi6526_dp_res_det_iqsel(irsel);
        }  while(dp_volt < DP_VOLT_200MV&& irsel <= IQ_SEL_100UA );

        hi6526_write_mask(DPDM_PMID_SEL_REG, DPDM_PMID_SEL_MASK, DPDM_PMID_SEL_SHIFT, SEL_DPDM);
        hi6526_write_mask(CHG_ADC_APPDET_REG, CHG_ADC_APPDET_CHSEL_MSK, CHG_ADC_APPDET_CHSEL_SHIFT, DP_DET);
        hi6526_write_mask(CHG_ADC_APPDET_REG, APPLE_DETECT_MASK, APPLE_DETECT_SHIFT, APPLE_DETECT_DISABLE);

        hi6526_write_mask(CHG_ADC_APPDET_REG, CHG_ADC_DP_RES_DET_MSK, CHG_ADC_DP_RES_DET_SHIFT, DP_RES_DET_EN);

        hi6526_get_adc_value(CHG_ADC_CH_DPDM, &code);
        hi6526_write_mask(CHG_ADC_APPDET_REG, CHG_ADC_DP_RES_DET_MSK, CHG_ADC_DP_RES_DET_SHIFT, DP_RES_DET_DIS);
        hi6526_dp_res_det_iqsel(IQ_SEL_1UA);

        res = (long)code * 2500000 / 4096 / irsel;

        SCHARGER_DBG("[%s] %d\n", __func__, res);

        return 0;
}

static int hi6526_get_chip_temp(int *temp)
{
        int ret = 0;
        u32 val = 0;

        ret |= hi6526_get_adc_value(CHG_ADC_CH_TSCHIP, &val);
        if (ret) {
                SCHARGER_ERR("[%s]get vbat_data fail,ret:%d\n", __func__, ret);
                return -1;
        }
        /* adc loop mode cannot get chip temp */
        *temp = 25; // (int)val;

        return 0;
}

static int hi6526_get_vbat(void)
{
        int ret = 0;
        u32 val = 0;

        ret |= hi6526_get_adc_value(CHG_ADC_CH_VBAT, &val);
        if (ret) {
                SCHARGER_ERR("[%s]get vbat_data fail,ret:%d\n", __func__, ret);
                return -1;
        }

    return val;
}

static int hi6526_get_ibat(int *ibat_ma)
{
        int ret = 0;
        u32 val = 0;

        ret |= hi6526_get_adc_value(CHG_ADC_CH_IBAT, &val);
        if (ret) {
                SCHARGER_ERR("[%s]get Ibat_data fail,ret:%d\n", __func__, ret);
                return -1;
        }

        *ibat_ma = val;
    return 0;
}

static int hi6526_record_chip_track(void)
{
        int ret = 0;
        u8 ilimit = 0;
        unsigned int index;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return -1;
        index = di->dbg_index++;
        if(di->dbg_index == INFO_LEN)
                di->dbg_index = 0;

#ifdef CONFIG_HISI_TIME
        di->dbg_info[index].ts_nsec = hisi_getcurtime();
#else
        di->dbg_info[index].ts_nsec = local_clock();
#endif

        ret |= hi6526_get_adc_value(CHG_ADC_CH_IBUS_REF, &(di->dbg_info[index].ibus_ref));
        if (ret) {
                SCHARGER_ERR("[%s]get ibus_ref fail,ret:%d\n", __func__, ret);
                return -1;
        }

        ret |= hi6526_get_adc_value(CHG_ADC_CH_IBUS, &(di->dbg_info[index].ibus));
        if (ret) {
                SCHARGER_ERR("[%s]get ibus_data fail,ret:%d\n", __func__, ret);
                return -1;
        }

        ret |= hi6526_get_adc_value(CHG_ADC_CH_VBUS, &(di->dbg_info[index].vbus));
        if (ret) {
                SCHARGER_ERR("[%s]get CHG_ADC_CH_VBUS fail,ret:%d\n", __func__, ret);
                return -1;
        }

        ret |= hi6526_get_adc_value(CHG_ADC_CH_VUSB, &(di->dbg_info[index].vusb));
        if (ret) {
                SCHARGER_ERR("[%s]get CHG_ADC_CH_VUSB fail,ret:%d\n", __func__, ret);
                return -1;
        }

        ret |= hi6526_get_adc_value(CHG_ADC_CH_IBAT, &(di->dbg_info[index].ibat));
        if (ret) {
                SCHARGER_ERR("[%s]get CHG_ADC_CH_IBAT fail,ret:%d\n", __func__, ret);
                return -1;
        }
        ret |= hi6526_get_adc_value(CHG_ADC_CH_VBAT, &(di->dbg_info[index].vbat));
        if (ret) {
                SCHARGER_ERR("[%s]get CHG_ADC_CH_VBAT fail,ret:%d\n", __func__, ret);
                return -1;
        }

        ret |= hi6526_get_adc_value(CHG_ADC_CH_VOUT, &(di->dbg_info[index].vout));
        if (ret) {
                SCHARGER_ERR("[%s]get CHG_ADC_CH_VOUT fail,ret:%d\n", __func__, ret);
                return -1;
        }

        if(di->dbg_info[index].ibus == IBUS_INVALID_VAL || CHARGER_TYPE_NONE == di->charger_type) {
                 di->dbg_info[index].ibus = 0;
        }

        if(di->chg_mode != LVC && di->chg_mode != SC) {
                hi6526_read_mask(CHG_INPUT_SOURCE_REG, CHG_ILIMIT_MSK, CHG_ILIMIT_SHIFT, &ilimit);
                if(ilimit < CHG_ILIMIT_600MA)
                        di->dbg_info[index].ibus = di->dbg_info[index].ibus / CHG_IBUS_DIV;
        }

        return ret;
}

static size_t printk_time(u64 ts, char *buf)
{
        int temp = 0;
        unsigned long rem_nsec;

        rem_nsec = do_div(ts, 1000000000);

        temp = snprintf_s(buf,80, 80, "[%5lu.%06lus]",
        		(unsigned long)ts, rem_nsec/1000);
        if (temp >= 0) {
        	return (unsigned int)temp;
        } else {
        	return 0;
        }
}

static void hi6526_dbg_printk(struct hi6526_device_info *di )
{
        int count = INFO_LEN;
        char time_log[80] = "";
        size_t tlen = 0;
        char *ptime_log;
        int index = 0;

        if (NULL == di)
                return ;

        index = di->dbg_index;

        while(count--) {
                ptime_log = time_log;
                tlen = printk_time(di->dbg_info[index].ts_nsec, ptime_log);

                SCHARGER_ERR("%s <%s> vusb:%d, vbus:%d, ibus:%d, ibat:%d, vout:%d, vbat:%d, ibus_ref:%d\n",
                        __func__, time_log,di->dbg_info[index].vusb, di->dbg_info[index].vbus,
                        di->dbg_info[index].ibus, di->dbg_info[index].ibat, di->dbg_info[index].vout,
                        di->dbg_info[index].vbat, di->dbg_info[index].ibus_ref);
                index++;
                if(index == INFO_LEN)
                        index = 0;
        }
}
static void hi6526_dbg_work(struct work_struct *work)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return ;

        hi6526_record_chip_track();
        if(di->dbg_work_stop) {
                di->delay_cunt++;
        } else {
                di->delay_cunt = 0;
        }

        if(di->delay_cunt > DELAY_TIMES) {
                if(di->abnormal_happened)
                        hi6526_dbg_printk(di);
                di->abnormal_happened = 0;
                di->dbg_work_stop = 0;
        } else {
                queue_delayed_work(system_power_efficient_wq, &di->dbg_work,
        		      msecs_to_jiffies(DBG_WORK_TIME));
        }

}

/**********************************************************
*  Function:       hi6526_get_ibus_ma
*  Description:    get average value for ilim
*  Parameters:     NULL
*  return value:   average value for ilim
**********************************************************/
static int hi6526_get_ibus_ma(void)
{
        int ret = 0;
        u32 ibus = 0;
        u8 ilimit = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di)
                return -1;

        ret |= hi6526_get_adc_value(CHG_ADC_CH_IBUS, &ibus);
        if (ret) {
                SCHARGER_ERR("[%s]get ibus_data fail,ret:%d\n", __func__, ret);
                return -1;
        }

        if(ibus == IBUS_INVALID_VAL || CHARGER_TYPE_NONE == di->charger_type) {
                 ibus = 0;
        }

        if(di->chg_mode != LVC && di->chg_mode != SC) {
                hi6526_read_mask(CHG_INPUT_SOURCE_REG, CHG_ILIMIT_MSK, CHG_ILIMIT_SHIFT, &ilimit);
                if(ilimit < CHG_ILIMIT_600MA)
                        ibus = ibus / CHG_IBUS_DIV;
        }

        return ibus;
}

/**********************************************************
*  Function:       hi6526_dump_register
*  Description:    print the register value in charging process
*  Parameters:   reg_value:string for save register value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_dump_register(char *reg_value)
{
        u8 reg_val = 0;
        char buff[BUF_LEN] = { 0 };
        int i = 0;
        int vbus = 0, ibat = 0;
        int vusb = 0, vout = 0, vbat = 0;
        int ret = 0;
        struct hi6526_device_info *di = g_hi6526_dev;

        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }
        memset_s(reg_value, CHARGELOG_SIZE, 0, CHARGELOG_SIZE);

        ret =  hi6526_get_vbus_mv((unsigned int *)&vbus);
        ret |= hi6526_get_ibat(&ibat);
        ret |= hi6526_get_vusb(&vusb);
        ret |= hi6526_get_vout(&vout);
        vbat = hi6526_get_vbat();
        if (ret){
                SCHARGER_ERR("%s hi6526_get_vbus_mv failed!\n", __func__);
        }
        if(LVC == di->chg_mode)
                snprintf_s(buff, BUF_LEN, 26, "LVC    ");
        else if(SC == di->chg_mode)
                snprintf_s(buff, BUF_LEN, 26, "SC     ");
        else
                snprintf_s(buff, BUF_LEN, 26, "BUCK   ");

        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, BUF_LEN, 26, "%-8.2d", hi6526_get_ibus_ma());
        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26,"%-8.2d", vbus);
        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, BUF_LEN, 26,"%-8.2d", ibat);
        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "%-8.2d", vusb);
        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "%-8.2d", vout);
        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "%-8.2d", vbat);
        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "%-8.2d", I_bias_all);
        strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));

        for (i = 0; i < (PAGE0_NUM); i++) {
                hi6526_read(PAGE0_BASE + i , &reg_val);
                snprintf_s(buff, BUF_LEN, 26,"0x%-9x", reg_val);
                strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        }
        for (i = 0; i < (PAGE1_NUM); i++) {
                hi6526_read(PAGE1_BASE + i , &reg_val);
                snprintf_s(buff, BUF_LEN, 26,"0x%-9x", reg_val);
                strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        }
        for (i = 0; i < (PAGE2_NUM); i++) {
                hi6526_read(PAGE2_BASE + i , &reg_val);
                snprintf_s(buff, BUF_LEN, 26,"0x%-9x", reg_val);
                strncat_s(reg_value, CHARGELOG_SIZE, buff, strlen(buff));
        }
        return 0;
}

/**********************************************************
*  Function:       hi6526_get_register_head
*  Description:    print the register head in charging process
*  Parameters:   reg_head:string for save register head
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_get_register_head(char *reg_head)
{
        char buff[BUF_LEN] = { 0 };
        int i = 0;

        memset_s(reg_head,CHARGELOG_SIZE, 0, CHARGELOG_SIZE);
        snprintf_s(buff, BUF_LEN, 26, "mode    ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, BUF_LEN, 26, "Ibus    ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "Vbus    ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "Ibat    ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "Vusb    ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "Vout    ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "Vbat    ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));

        snprintf_s(buff, (unsigned long)BUF_LEN, 26, "Ibias   ");
        strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        for (i = 0; i < (PAGE0_NUM); i++) {
                snprintf_s(buff, BUF_LEN, 26, "Reg[0x%3x] ", PAGE0_BASE + i);
                strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        }
        for (i = 0; i < (PAGE1_NUM); i++) {
                snprintf_s(buff, BUF_LEN, 26, "Reg[0x%3x] ", PAGE1_BASE + i);
                strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        }
        for (i = 0; i < (PAGE2_NUM); i++) {
                snprintf_s(buff, BUF_LEN, 26, "Reg[0x%3x] ", PAGE2_BASE + i);
                strncat_s(reg_head, CHARGELOG_SIZE, buff, strlen(buff));
        }

        return 0;
}

/**********************************************************
*  Function:       hi6526_set_batfet_disable
*  Description:    set the batfet disable in charging process
*  Parameters:   disable:batfet disable or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_batfet_disable(int disable)
{
        SCHARGER_DBG("%s >>> disable:%d\n", __func__, disable);

        return hi6526_set_batfet_ctrl(!disable);
}

/**********************************************************
*  Function:       hi6526_set_watchdog_timer
*  Description:    set the watchdog timer in charging process
*  Parameters:   value:watchdog timer value
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_watchdog_timer(int value)
{
        u8 val = 0;
        u8 dog_time = value;

        if (dog_time >= WATCHDOG_TIMER_80_S) {
                val = 7;
        } else if (dog_time >= WATCHDOG_TIMER_40_S) {
                val = 6;
        } else if (dog_time >= WATCHDOG_TIMER_20_S) {
                val = 5;
        } else if (dog_time >= WATCHDOG_TIMER_10_S) {
                val = 4;
        } else if (dog_time >= WATCHDOG_TIMER_02_S) {
                val = 3;
        } else if (dog_time >= WATCHDOG_TIMER_01_S) {
                val = 2;
        } else {
                val = 0;
        }
        SCHARGER_DBG(" watch dog timer is %d ,the register value is set %u \n",
                     dog_time, val);
        hi6526_reset_watchdog_timer();
        return hi6526_write_mask(WATCHDOG_CTRL_REG, WATCHDOG_TIMER_MSK,
                                 WATCHDOG_TIMER_SHIFT, val);
}

static int hi6526_set_watchdog_timer_ms(int ms)
{
       return hi6526_set_watchdog_timer(ms / 1000);
}


/**********************************************************
*  Function:       hi6526_set_charger_hiz
*  Description:    set the charger hiz close watchdog
*  Parameters:   enable:charger in hiz or not
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_set_charger_hiz(int enable)
{
        return set_buck_mode_enable(!enable);
}

/**********************************************************
*  Function:       hi6526_is_water_intrused
*  Description:   check voltage of DN/DP
*  Parameters:   NULL
*  return value:  1:water intrused/ 0:water not intrused/ -ENOMEM:hi6526 is not initializied
**********************************************************/
int hi6526_is_water_intrused(void)
{
        int dp_res = 0;
        struct hi6526_device_info *di = g_hi6526_dev;

        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        if(!di->param_dts.dp_th_res)
                return 0;

        dp_res = hi6526_get_dp_res();
        if(dp_res > di->param_dts.dp_th_res) {
                SCHARGER_INF("D+ water intrused\n");
                return 1;
        }

        return 0;
}

/****************************************************************************
  Function:     hi6526_soft_vbatt_ovp_protect
  Description:  vbatt soft ovp check
  Input:        NA
  Output:       NA
  Return:        0: success;-1: other fail
***************************************************************************/
static int hi6526_soft_vbatt_ovp_protect(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        int vbatt_mv, vbatt_max;

        if (NULL == di)
                return -1;
        vbatt_mv = hisi_battery_voltage();
        vbatt_max = hisi_battery_vbat_max();
        if (vbatt_mv >= MIN(CHG_VBATT_SOFT_OVP_MAX, CHG_VBATT_CV_103(vbatt_max))) {
                di->batt_ovp_cnt_30s++;
                if (CHG_VBATT_SOFT_OVP_CNT == di->batt_ovp_cnt_30s) {
                        hi6526_set_charger_hiz(TRUE);
                        SCHARGER_ERR("%s:vbat:%d,cv_mv:%d,ovp_cnt:%d,shutdown buck.\n",
                                __func__, vbatt_mv, di->term_vol_mv, di->batt_ovp_cnt_30s);
                        di->batt_ovp_cnt_30s = 0;
                }
        } else
                di->batt_ovp_cnt_30s = 0;
        return 0;
}

/****************************************************************************
  Function:     hi6526_rboost_buck_limit
  Description:  limit buck current to 470ma according to rboost count
  Input:        NA
  Output:       NA
  Return:       0: do nothing; 1:limit buck current 470ma
***************************************************************************/
static int hi6526_rboost_buck_limit(void)
{
        if (ILIMIT_RBOOST_CNT < g_rboost_cnt) {
                SCHARGER_INF("%s:rboost cnt:%d\n", __func__, g_rboost_cnt);
                return 1;
        }
        else {
                g_rboost_cnt = 0;
        }
        return 0;
}

static int hi6526_dpm_init(void)
{
        int ret = 0;
        /* set dpm mode auto */
        ret |= hi6526_write_mask(CHG_DPM_MODE_REG, CHG_DPM_MODE_MSK,\
                                CHG_DPM_MODE_SHIFT, CHG_DPM_MODE_AUTO);

        /* set dpm voltage sel 90% vbus*/
        ret |= hi6526_write_mask(CHG_DPM_SEL_REG, CHG_DPM_SEL_MSK, \
                CHG_DPM_SEL_SHIFT, CHG_DPM_SEL_DEFAULT);

        return ret;
}

/****************************************************************************
  Function:     hi6526_ibat_res_sel
  Description:  ibat resisitance val for ADC
  Input:        resisitance
  Output:       NA
  Return:        0: success;-1: other fail
***************************************************************************/
static int hi6526_ibat_res_sel(int resisitance)
{
        u8 val = 0;
        int ret;

        if(R_MOHM_2 == resisitance)
                val = 0;
        else if(R_MOHM_5 == resisitance)
                val = 1;
        else
                SCHARGER_ERR("%s: %d mohm , not expected\n", __func__, resisitance);

        ret = hi6526_write_mask(IBAT_RES_SEL_REG, IBAT_RES_SEL_MASK, \
                IBAT_RES_SEL_SHIFT, val);

        return ret;
}

static int hi6526_get_dieid(char *dieid, unsigned int len)
{
        u8 val[3];
        int ret = 0;

        if(NULL == dieid) {
                SCHARGER_ERR("%s: dieid is null\n", __func__);
                return -1;
        }

        ret |= hi6526_efuse_read(EFUSE3, EFUSE_BYTE5, &val[0]);
        ret |= hi6526_efuse_read(EFUSE3, EFUSE_BYTE6, &val[1]);
        ret |= hi6526_efuse_read(EFUSE3, EFUSE_BYTE7, &val[2]);

        snprintf_s(dieid, len, len, "\r\nHi6526:0x%02x%02x%02x\r\n", val[2],val[1],val[0]);
        return ret;
}
/**********************************************************
*  Function:     hi6526_chip_init()
*  Description:  chip init for hi6526
*  Parameters:   chip_init_crit
*  return value:
*                 0-success or others-fail
**********************************************************/
static int hi6526_chip_init(struct chip_init_crit* init_crit)
{
        int ret = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di ||  NULL == init_crit) {
                SCHARGER_ERR("%s hi6526_device_info or chip_init_crit NULL!\n", __func__);
                return -ENOMEM;
        }
        switch(init_crit->vbus) {
                case ADAPTER_5V:
                        ret |= hi6526_ibat_res_sel(di->param_dts.r_coul_mohm);
                        ret |= hi6526_config_opt_param(VBUS_VSET_5V);
                        ret |= hi6526_dpm_init();
                        ret |= hi6526_set_vbus_vset(VBUS_VSET_5V);
                        charger_is_fcp = FCP_FALSE;
                        first_insert_flag = FIRST_INSERT_TRUE;
                        break;
                case ADAPTER_9V:
                        ret |= hi6526_config_opt_param(VBUS_VSET_5V);
                        ret |= hi6526_set_vbus_uvp_ovp(VBUS_VSET_9V);
                        break;
                default:
                        SCHARGER_ERR("%s: init mode err\n", __func__);
                        return -EINVAL;
        }

        ret |= set_buck_mode_enable(CHG_ENABLE);
        ret |= hi6526_set_charge_enable(CHG_DISABLE);
        ret |= hi6526_set_recharge_vol(CHG_RECHG_150);
        ret |= hi6526_set_fast_safe_timer(CHG_FASTCHG_TIMER_20H);
        ret |= hi6526_set_term_enable(CHG_TERM_DIS);
        ret |= hi6526_set_input_current(CHG_ILIMIT_475);
        ret |= hi6526_set_charge_current(CHG_FAST_ICHG_500MA);
        ret |= hi6526_set_terminal_voltage(CHG_FAST_VCHG_4400);
        ret |= hi6526_set_terminal_current(CHG_TERM_ICHG_150MA);
        ret |= hi6526_set_watchdog_timer(WATCHDOG_TIMER_40_S);
        ret |= hi6526_set_precharge_current(CHG_PRG_ICHG_200MA);
        ret |= hi6526_set_precharge_voltage(CHG_PRG_VCHG_2800);
        ret |= hi6526_set_batfet_ctrl(CHG_BATFET_EN);
        /*IR compensation voatge clamp ,IR compensation resistor setting */
        ret |= hi6526_set_bat_comp(di->param_dts.bat_comp);
        ret |= hi6526_set_vclamp(di->param_dts.vclamp);
        ret |= hi6526_set_otg_current(BOOST_LIM_1000);
        ret |= hi6526_set_otg_enable(OTG_DISABLE);

        return ret;
}
/**********************************************************
*  Function:       hi6526_fcp_get_adapter_output_current
*  Description:    fcp get the output current from adapter max power and output vol
*  Parameters:     NA
*  return value:  input_current(MA)
**********************************************************/
static int hi6526_fcp_get_adapter_output_current(void)
{

        return 0;
}

/****************************************************************************
  Function:     hi6526_fcp_cmd_transfer_check
  Description:  check cmd transfer success or fail
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/
static int hi6526_fcp_cmd_transfer_check(void)
{
        u8 reg_val1 = 0, reg_val2 = 0;
        int i = 0;
        int ret = 0;
        u8 reg_val3 = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }
        /*read accp interrupt registers until value is not zero */
        do {
                usleep_range(12000, 13000);
                ret |= hi6526_read(CHG_FCP_ISR1_REG, &reg_val1);
                ret |= hi6526_read(CHG_FCP_ISR2_REG, &reg_val2);
                ret |= hi6526_read(CHG_FCP_IRQ3_REG, &reg_val3);
                if (ret) {
                        SCHARGER_ERR("%s : reg read failed!\n", __func__);
                        break;
                }
                if (reg_val1 || reg_val2) {
                        if ((reg_val1 & CHG_FCP_ACK)
                            && (reg_val1 & CHG_FCP_CMDCPL)
                            && !(reg_val2 & (CHG_FCP_CRCRX | CHG_FCP_PARRX))) {
                                return 0;
                        } else if ((reg_val1 & CHG_FCP_CRCPAR) && (reg_val2 & CHG_FCP_PROTSTAT)){
                                SCHARGER_INF
                                    ("%s :  FCP_TRANSFER_FAIL,slave status changed: ISR1=0x%x,ISR2=0x%x\n",
                                     __func__, reg_val1, reg_val2);
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
                if(di->dc_ibus_ucp_happened)
                        i = FCP_ACK_RETRY_CYCLE;
        } while (i < FCP_ACK_RETRY_CYCLE);

        SCHARGER_INF("%s : fcp adapter transfer time out,total time %d ms\n",
                     __func__, i * 10);
        return -1;
}

/****************************************************************************
  Function:     hi6526_fcp_protocol_restart
  Description:  disable accp protocol and enable again
  Input:         NA
  Output:       NA
  Return:        0: success
                   -1: fail
***************************************************************************/
static void hi6526_fcp_protocol_restart(void)
{
        u8 reg_val = 0;
        int ret = 0;
        int i;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return ;
        }

        /* disable accp protocol */
        mutex_lock(&di->fcp_detect_lock);
        hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK, CHG_FCP_EN_SHIFT,
                          FALSE);
        usleep_range(9000, 10000);
        hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK, CHG_FCP_EN_SHIFT,
                          TRUE);
        /*detect hisi fcp charger, wait for ping succ */
        for (i = 0; i < HI6526_RESTART_TIME; i++) {
                usleep_range(9000, 10000);
                ret = hi6526_read(CHG_FCP_STATUS_REG, &reg_val);
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

        if (HI6526_RESTART_TIME == i) {
                SCHARGER_ERR("%s:wait for slave fail\n", __func__);
                mutex_unlock(&di->fcp_detect_lock);
                return;
        }
        mutex_unlock(&di->fcp_detect_lock);
        SCHARGER_ERR("%s :disable and enable fcp protocol accp status  is 0x%x \n",__func__,reg_val);
}

int hi6526_fcp_adapter_reg_read_block( u8 reg,u8 * val, u8 num)
{
        int ret = 0;
        int i = 0;
        u8 reg_val1 = 0, reg_val2 = 0;
        u8 data_len = 0;
        u8 *p = NULL;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        mutex_lock(&di->accp_adapter_reg_lock);

        data_len = num < FCP_DATA_LEN ? num:FCP_DATA_LEN;
        p = val;

        for(i = 0; i < FCP_RETRY_TIME; i++) {
                /*before send cmd, clear accp interrupt registers */
                ret |= hi6526_read(CHG_FCP_ISR1_REG, &reg_val1);
                ret |= hi6526_read(CHG_FCP_ISR2_REG, &reg_val2);
                if (reg_val1 != 0) {
                        ret |= hi6526_write(CHG_FCP_ISR1_REG, reg_val1);
                }
                if (reg_val2 != 0) {
                        ret |= hi6526_write(CHG_FCP_ISR2_REG, reg_val2);
                }

                ret |= hi6526_write(CHG_FCP_CMD_REG, CHG_FCP_CMD_MBRRD);
                ret |= hi6526_write(CHG_FCP_ADDR_REG, reg);
                ret |= hi6526_write(CHG_FCP_LEN_REG, data_len);

                ret |=
                    hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_SNDCMD_MSK,
                                      CHG_FCP_SNDCMD_SHIFT, CHG_FCP_EN);

                if (ret) {
                        SCHARGER_ERR("%s: read error ret is %d \n", __func__, ret);
                        mutex_unlock(&di->accp_adapter_reg_lock);
                        return HI6526_FAIL;
                }

                /* check cmd transfer success or fail */
                if (0 == hi6526_fcp_cmd_transfer_check()) {
                        /* recived data from adapter */
                        ret |= hi6526_read_block(CHG_FCP_RDATA_REG, p, data_len);
                        break;
                }
                hi6526_fcp_protocol_restart();
                if(di->dc_ibus_ucp_happened)
                        i = FCP_RETRY_TIME;
        }
        if(FCP_RETRY_TIME <= i)
        {
                SCHARGER_ERR("%s : ack error,retry %d times \n",__func__,i);
                ret = HI6526_FAIL;
        }
        mutex_unlock(&di->accp_adapter_reg_lock);

        if(ret)
                return ret;

        num -=data_len;
        if(num) {
                p += data_len;
                reg += data_len;
                ret = hi6526_fcp_adapter_reg_read_block(reg, p, num);
                 if (ret) {
                        SCHARGER_ERR("%s: read error ret is %d \n", __func__, ret);
                        //mutex_unlock(&di->accp_adapter_reg_lock);
                        return HI6526_FAIL;
                }
        }

        return 0;
}

/****************************************************************************
  Function:     hi6526_fcp_adapter_reg_read
  Description:  read adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:       0: success
                others: fail
***************************************************************************/
static int hi6526_fcp_adapter_reg_read(u8 * val, u8 reg)
{
        int ret = 0;
        int i = 0;
        u8 reg_val1 = 0, reg_val2 = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        mutex_lock(&di->accp_adapter_reg_lock);
        for(i = 0; i < FCP_RETRY_TIME; i++) {
                /*before send cmd, clear accp interrupt registers */
                ret |= hi6526_read(CHG_FCP_ISR1_REG, &reg_val1);
                ret |= hi6526_read(CHG_FCP_ISR2_REG, &reg_val2);
                if (reg_val1 != 0) {
                        ret |= hi6526_write(CHG_FCP_ISR1_REG, reg_val1);
                }
                if (reg_val2 != 0) {
                        ret |= hi6526_write(CHG_FCP_ISR2_REG, reg_val2);
                }

                ret |= hi6526_write(CHG_FCP_CMD_REG, CHG_FCP_CMD_SBRRD);
                ret |= hi6526_write(CHG_FCP_ADDR_REG, reg);
                ret |=
                    hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_SNDCMD_MSK,
                                      CHG_FCP_SNDCMD_SHIFT, CHG_FCP_EN);

                if (ret) {
                        SCHARGER_ERR("%s: write error ret is %d \n", __func__, ret);
                        mutex_unlock(&di->accp_adapter_reg_lock);
                        return HI6526_FAIL;
                }

                /* check cmd transfer success or fail */
                if (0 == hi6526_fcp_cmd_transfer_check()) {
                        /* recived data from adapter */
                        ret |= hi6526_read(CHG_FCP_RDATA_REG, val);
                        break;
                }
                hi6526_fcp_protocol_restart();
                if(di->dc_ibus_ucp_happened)
                        i = FCP_RETRY_TIME;
        }
        if(FCP_RETRY_TIME <= i)
        {
                SCHARGER_ERR("%s : ack error,retry %d times \n",__func__,i);
                ret = HI6526_FAIL;
        }
        mutex_unlock(&di->accp_adapter_reg_lock);

        return ret;
}

/****************************************************************************
  Function:     hi6526_fcp_adapter_reg_write
  Description:  write value into the adapter register
  Input:        reg:register's num
                val:the value of register
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6526_fcp_adapter_reg_write(u8 val, u8 reg)
{
        int ret = 0;
        int i = 0;
        u8 reg_val1 = 0, reg_val2 = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        mutex_lock(&di->accp_adapter_reg_lock);
        for(i = 0; i < FCP_RETRY_TIME; i++) {
                /*before send cmd, clear accp interrupt registers */
                ret |= hi6526_read(CHG_FCP_ISR1_REG, &reg_val1);
                ret |= hi6526_read(CHG_FCP_ISR2_REG, &reg_val2);
                if (reg_val1 != 0) {
                        ret |= hi6526_write(CHG_FCP_ISR1_REG, reg_val1);
                }
                if (reg_val2 != 0) {
                        ret |= hi6526_write(CHG_FCP_ISR2_REG, reg_val2);
                }
                ret |= hi6526_write(CHG_FCP_CMD_REG, CHG_FCP_CMD_SBRWR);
                ret |= hi6526_write(CHG_FCP_ADDR_REG, reg);
                ret |= hi6526_write(CHG_FCP_WDATA_REG, val);
                ret |=
                    hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_SNDCMD_MSK,
                                      CHG_FCP_SNDCMD_SHIFT, CHG_FCP_EN);

                if (ret) {
                        SCHARGER_ERR("%s: write error ret is %d \n", __func__, ret);
                        mutex_unlock(&di->accp_adapter_reg_lock);
                        return HI6526_FAIL;
                }

                /* check cmd transfer success or fail */
                if (0 == hi6526_fcp_cmd_transfer_check()) {
                        break;
                }
                hi6526_fcp_protocol_restart();
                if(di->dc_ibus_ucp_happened)
                        i = FCP_RETRY_TIME;
        }
        if(FCP_RETRY_TIME <= i)
        {
                SCHARGER_ERR("%s : ack error,retry %d times \n",__func__,i);
                ret = HI6526_FAIL;
        }

        mutex_unlock(&di->accp_adapter_reg_lock);
        return ret;
}
static int hi6526_fcp_adapter_reg_write_block(u8 reg, u8 *val, u8 num_bytes)
{
        int ret = 0;
        int i = 0;
        u16 reg_val = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        if(num_bytes > FCP_DATA_LEN) {
                SCHARGER_ERR("%s: max num is 8, num_bytes is %d,  \n", __func__, num_bytes);
                return HI6526_FAIL;
        }

        mutex_lock(&di->accp_adapter_reg_lock);
        for(i = 0; i < FCP_RETRY_TIME; i++) {
                /*before send cmd, clear accp interrupt registers */
                ret |= hi6526_read_block(CHG_FCP_ISR1_REG, (u8 *)&reg_val, 2);
                if(reg_val) {
                        ret |= hi6526_write_block(CHG_FCP_ISR1_REG, (u8 *)&reg_val, 2);
                }
                ret |= hi6526_read_block(CHG_FCP_ISR1_REG, (u8 *)&reg_val, 2);
                if(reg_val) {
                        SCHARGER_ERR("%s: reg_val 0x%x,  \n", __func__, reg_val);
                }

                ret |= hi6526_write(CHG_FCP_CMD_REG, CHG_FCP_CMD_MBRWR);
                ret |= hi6526_write(CHG_FCP_ADDR_REG, reg);
                ret |= hi6526_write(CHG_FCP_LEN_REG, num_bytes);
                ret |= hi6526_write_block(CHG_FCP_WDATA_REG, val, num_bytes);
                ret |=
                    hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_SNDCMD_MSK,
                                      CHG_FCP_SNDCMD_SHIFT, CHG_FCP_EN);

                if (ret) {
                        SCHARGER_ERR("%s: write error ret is %d \n", __func__, ret);
                        mutex_unlock(&di->accp_adapter_reg_lock);
                        return HI6526_FAIL;
                }

                /* check cmd transfer success or fail */
                if (0 == hi6526_fcp_cmd_transfer_check()) {
                        break;
                }
                hi6526_fcp_protocol_restart();
                if(di->dc_ibus_ucp_happened)
                        i = FCP_RETRY_TIME;
        }
        if(FCP_RETRY_TIME <= i)
        {
                SCHARGER_ERR("%s : ack error,retry %d times \n",__func__,i);
                ret = HI6526_FAIL;
        }

        mutex_unlock(&di->accp_adapter_reg_lock);
        return ret;
}

/****************************************************************************
  Function:     hi6526_fcp_get_adapter_output_vol
  Description:  get fcp output vol
  Input:        NA.
  Output:       fcp output vol(5V/9V/12V)*10
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6526_fcp_get_adapter_output_vol(u8 * vol)
{
        u8 num = 0;
        u8 output_vol = 0;
        int ret = 0;

        /*get adapter vol list number,exclude 5V */
        ret |=
            hi6526_fcp_adapter_reg_read(&num,
                                        CHG_FCP_SLAVE_DISCRETE_CAPABILITIES);
        /*currently,fcp only support three out vol config(5v/9v/12v) */
        if (ret || num > 2) {
                SCHARGER_ERR("%s: vout list support err, reg[0x21] = %u.\n",
                             __func__, num);
                return -1;
        }

        /*get max out vol value */
        ret |=
            hi6526_fcp_adapter_reg_read(&output_vol,
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
  Function:     hi6526_fcp_adapter_vol_check
  Description:  check adapter voltage is around expected voltage
  Input:        adapter_vol_mv : expected adapter vol mv
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6526_fcp_adapter_vol_check(int adapter_vol_mv)
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
                ret = hi6526_get_vbus_mv((unsigned int *)&adc_vol);
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
  Function:     hi6526_fcp_set_adapter_output_vol
  Description:  set fcp adapter output vol
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: fail
***************************************************************************/
static int hi6526_fcp_set_adapter_output_vol(int *output_vol)
{
        u8 val = 0;
        u8 vol = 0;
        int ret = 0;

        /*read ID OUTI , for identify huawei adapter */
        ret = hi6526_fcp_adapter_reg_read(&val, CHG_FCP_SLAVE_ID_OUT0);
        if (ret != 0) {
                SCHARGER_ERR("%s: adapter ID OUTI read failed, ret is %d \n",
                             __func__, ret);/*lint !e64*/
                return -1;
        }
        SCHARGER_INF("%s: id out reg[0x4] = %u.\n", __func__, val);

        /*get adapter max output vol value */
        ret = hi6526_fcp_get_adapter_output_vol(&vol);
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
        ret |= hi6526_fcp_adapter_reg_write(vol, CHG_FCP_SLAVE_VOUT_CONFIG);
        ret |= hi6526_fcp_adapter_reg_read(&val, CHG_FCP_SLAVE_VOUT_CONFIG);
        SCHARGER_INF("%s: vout config reg[0x2c] = %u.\n", __func__, val);
        if (ret || val != vol) {
                SCHARGER_ERR("%s:out vol config err, reg[0x2c] = %u,vol :%d.\n",
                             __func__, val, vol);
                return -1;
        }

        ret |=
            hi6526_fcp_adapter_reg_write(CHG_FCP_SLAVE_SET_VOUT,
                                         CHG_FCP_SLAVE_OUTPUT_CONTROL);
        if (ret) {
                SCHARGER_ERR("%s : enable adapter output voltage failed \n ",
                             __func__);
                return -1;
        }

        ret |= hi6526_fcp_adapter_vol_check(vol / CHG_FCP_VOL_STEP * 1000);
        if (ret) {
                SCHARGER_ERR("%s : adc check adapter output voltage failed \n ",
                             __func__);
                return -1;
        }

        SCHARGER_INF("fcp adapter output vol set ok.\n");
        return 0;
}

/****************************************************************************
  Function:     hi6526_fcp_adapter_detect
  Description:  detect fcp adapter
  Input:        NA
  Output:       NA
  Return:        0: success
                -1: other fail
                1:fcp adapter but detect fail
***************************************************************************/
static int hi6526_fcp_adapter_detect(struct hi6526_device_info *di)
{
        u8 reg_val1 = 0;
        u8 reg_val2 = 0;
        int i = 0;
        int ret = 0;

        mutex_lock(&di->fcp_detect_lock);
        ret |= hi6526_read(CHG_FCP_STATUS_REG, &reg_val2);

        SCHARGER_ERR("%s:CHG_FCP_STATUS_REG2:0x%x\n", __func__, reg_val2);
        if (ret) {
                SCHARGER_ERR("%s:read det attach err,ret:%d.\n", __func__, ret);
                mutex_unlock(&di->fcp_detect_lock);
                return -1;
        }

        if (CHG_FCP_SLAVE_GOOD ==
                (reg_val2 & (CHG_FCP_DVC_MSK | CHG_FCP_ATTATCH_MSK))) {
                mutex_unlock(&di->fcp_detect_lock);
                SCHARGER_INF("fcp adapter detect ok.\n");
                return CHG_FCP_ADAPTER_DETECT_SUCC;
        }
        ret |=
            hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
                              CHG_FCP_DET_EN_SHIFT, TRUE);
        ret |=
            hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
                              CHG_FCP_CMP_EN_SHIFT, TRUE);
        if (ret) {
                SCHARGER_ERR("%s:FCP enable detect fail,ret:%d.\n", __func__,
                             ret);
                hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
                                  CHG_FCP_CMP_EN_SHIFT, FALSE);
                hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
                                  CHG_FCP_DET_EN_SHIFT, FALSE);
                mutex_unlock(&di->fcp_detect_lock);
                return -1;
        }
        /* wait for fcp_set */
        for (i = 0; i < CHG_FCP_DETECT_MAX_COUT; i++) {
                ret = hi6526_read(CHG_FCP_SET_STATUS_REG, &reg_val1);
                SCHARGER_ERR("%s:CHG_FCP_SET_STATUS_REG1 0x%d.\n", __func__, reg_val1);
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
                hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
                                  CHG_FCP_CMP_EN_SHIFT, FALSE);
                hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
                                  CHG_FCP_DET_EN_SHIFT, FALSE);
                mutex_unlock(&di->fcp_detect_lock);
                SCHARGER_ERR("%s:CHG_FCP_ADAPTER_DETECT_OTHER return \n", __func__);
                return CHG_FCP_ADAPTER_DETECT_OTHER;
        }

        /* enable fcp_en */
        hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK, CHG_FCP_EN_SHIFT,
                          TRUE);

        /*detect hisi fcp charger, wait for ping succ */
        for (i = 0; i < CHG_FCP_DETECT_MAX_COUT; i++) {
                ret = hi6526_read(CHG_FCP_STATUS_REG, &reg_val2);

                SCHARGER_ERR("%s:wait for ping succ :0x%x\n", __func__, reg_val2);

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
                hi6526_write_mask(CHG_FCP_CTRL_REG, CHG_FCP_EN_MSK,
                                  CHG_FCP_EN_SHIFT, FALSE);
                hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_CMP_EN_MSK,
                                  CHG_FCP_CMP_EN_SHIFT, FALSE);
                hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
                                  CHG_FCP_DET_EN_SHIFT, FALSE);
                SCHARGER_ERR("fcp adapter detect failed,reg[0x%x]=0x%x\n",
                             CHG_FCP_STATUS_REG, reg_val2);
                mutex_unlock(&di->fcp_detect_lock);
                return CHG_FCP_ADAPTER_DETECT_FAIL;        /*not fcp adapter */

        }
        SCHARGER_INF("fcp adapter detect ok\n");
        mutex_unlock(&di->fcp_detect_lock);
        return CHG_FCP_ADAPTER_DETECT_SUCC;

}
static int fcp_adapter_detect(void)
{
        int ret;
        struct hi6526_device_info *di = g_hi6526_dev;
        u8 val;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        ret = hi6526_fcp_adapter_detect(di);
        if (CHG_FCP_ADAPTER_DETECT_OTHER == ret)
        {
                SCHARGER_INF("%s fcp adapter other detect\n", __func__);
                return FCP_ADAPTER_DETECT_OTHER;
        }
        if (CHG_FCP_ADAPTER_DETECT_FAIL == ret)
        {
                SCHARGER_INF("%s fcp adapter detect fail\n", __func__);
                return FCP_ADAPTER_DETECT_FAIL;
        }

        if (hi6526_is_support_scp())
        {
                return FCP_ADAPTER_DETECT_SUCC;
        }
        ret = hi6526_fcp_adapter_reg_read(&val, SCP_ADP_TYPE);
        if(ret)
        {
                SCHARGER_ERR("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
                return FCP_ADAPTER_DETECT_SUCC;
        }
        return FCP_ADAPTER_DETECT_OTHER;

}
/****************************************************************************
  Function:     hi6526_is_support_fcp
  Description:  check_if_support_fcp
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6526_is_support_fcp(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
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
  Function:     hi6526_fcp_master_reset
  Description:  reset master
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6526_fcp_master_reset(void)
{
        int ret = 0;
        ret |= hi6526_write(CHG_FCP_SOFT_RST_REG, CHG_FCP_SOFT_RST_VAL);
        msleep(10);
        ret |= hi6526_write(CHG_FCP_SOFT_RST_REG, CHG_FCP_SOFT_RST_DEFULT);
        ret |= hi6526_write(CHG_FCP_CTRL_REG, 0);        //clear fcp_en and fcp_master_rst
        return ret;
}

/****************************************************************************
  Function:     hi6526_fcp_adapter_reset
  Description:  reset adapter
  Input:        NA
  Output:       NA
  Return:        0: success
             other: fail
***************************************************************************/
static int hi6526_fcp_adapter_reset(void)
{
        int ret = 0;
        ret |= hi6526_set_vbus_vset(VBUS_VSET_5V);
        ret |=
            hi6526_write(CHG_FCP_CTRL_REG,
                              CHG_FCP_EN_MSK | CHG_FCP_MSTR_RST_MSK);
        if (ret) {
                SCHARGER_ERR("%s : send rst cmd failed \n ", __func__);
                return ret;
        }

        ret |= hi6526_fcp_adapter_vol_check(FCP_ADAPTER_RST_VOL);
        if (ret) {
                ret |= hi6526_write(CHG_FCP_CTRL_REG, 0);        //clear fcp_en and fcp_master_rst
                SCHARGER_ERR("%s : adc check adapter output voltage failed \n ",
                             __func__);
                return ret;
        }

        ret |= hi6526_write(CHG_FCP_CTRL_REG, 0);        //clear fcp_en and fcp_master_rst
        ret |= hi6526_config_opt_param(VBUS_VSET_5V);
        return ret;
}

/**********************************************************
*  Function:       hi6526_stop_charge_config
*  Description:    config chip after stop charging
*  Parameters:     NULL
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_stop_charge_config(void)
{
        int ret = 0;
        ret |= hi6526_set_vbus_vset(VBUS_VSET_5V);

        is_weaksource = WEAKSOURCE_FALSE;

        return ret;
}

/**********************************************************
*  Function:       hi6526_fcp_stop_charge_config
*  Description:    fcp config chip after stop charging
*  Parameters:     NULL
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_fcp_stop_charge_config(void)
{
        SCHARGER_DBG("hi6526_fcp_master_reset");
        hi6526_fcp_master_reset();

        hi6526_write_mask(CHG_FCP_DET_CTRL_REG, CHG_FCP_DET_EN_MSK,
                                  CHG_FCP_DET_EN_SHIFT, FALSE);
        return 0;
}
static int is_fcp_charger_type(void)
{
        u8 reg_val = 0;
        int ret = 0;

        if (hi6526_is_support_fcp()) {
                SCHARGER_ERR("%s:NOT SUPPORT FCP!\n", __func__);
                return FALSE;
        }
        ret |= hi6526_read(FCP_ADAPTER_CNTL_REG, &reg_val);
        if (ret) {
                SCHARGER_ERR("%s reg read fail!\n", __func__);
                return FALSE;
        }
        if (HI6526_ACCP_CHARGER_DET == (reg_val & HI6526_ACCP_CHARGER_DET))
                return TRUE;
        return FALSE;
}
static int fcp_read_adapter_status (void)
{
        u8 val = 0;
        int ret = 0;
        ret = hi6526_fcp_adapter_reg_read(&val, FCP_ADAPTER_STATUS);
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
static void hi6526_reg_dump(char* ptr)
{
        return;
}

static void hi6526_lvc_opt_regs(void)
{
        hi6526_opt_regs_set(lvc_opt_regs, ARRAY_SIZE(lvc_opt_regs));
}

static void hi6526_sc_opt_regs(void)
{
        hi6526_opt_regs_set(sc_opt_regs, ARRAY_SIZE(sc_opt_regs));
}

static int hi6526_is_support_scp(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;

        if(!di || !di->param_dts.scp_support)
        {
                return HI6526_FAIL;
        }
        return 0;
}
static int scp_adapter_reg_read(u8* val, u8 reg)
{
        int ret;

        if (scp_error_flag)
        {
                SCHARGER_ERR("%s : scp timeout happened ,do not read reg = %d \n",__func__,reg);
                return HI6526_FAIL;
        }

        ret = hi6526_fcp_adapter_reg_read(val, reg);
        if (ret)
        {
                SCHARGER_ERR("%s : error reg = %d \n",__func__,reg);
                if(reg != SCP_ADP_TYPE0)
                        scp_error_flag = SCP_IS_ERR;

                return HI6526_FAIL;
        }

        return 0;
}
static int scp_adapter_reg_write(u8 val, u8 reg)
{
        int ret;

        if (scp_error_flag)
        {
                SCHARGER_ERR("%s : scp timeout happened ,do not write reg = %d \n",__func__,reg);
                        return HI6526_FAIL;
        }

        ret = hi6526_fcp_adapter_reg_write(val, reg);
        if (ret)
        {
                SCHARGER_ERR("%s : error reg = %d \n",__func__,reg);
                scp_error_flag = SCP_IS_ERR;

                return HI6526_FAIL;
        }

        return 0;
}

static int scp_adapter_reg_read_block(u8 reg,u8* val, u8 num)
{
        int ret;
        int i = 0;
        u8 data = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        if (scp_error_flag)
        {
                SCHARGER_ERR("%s : scp timeout happened ,do not read reg = %d \n",__func__,reg);
                return HI6526_FAIL;
        }

        if(di->adaptor_support & SC_MODE) {
                ret = hi6526_fcp_adapter_reg_read_block(reg, val, num);
                if (ret)
                {
                        SCHARGER_ERR("%s : error reg = 0x%x \n",__func__,reg);
                        scp_error_flag = SCP_IS_ERR;

                        return HI6526_FAIL;
                }
        } else {
                for(i = 0; i < num; i++){
                        ret = scp_adapter_reg_read(&data,reg+i);
                        if (ret)
                        {
                                SCHARGER_ERR("%s : error reg = %d \n",__func__,reg);
                                scp_error_flag = SCP_IS_ERR;
                                return HI6526_FAIL;
                        }
                        val[i] = data;
                }
        }

        return 0;
}
static int scp_adapter_reg_write_block(u8 reg,u8* val, u8 num)
{
        int ret;
        int i = 0;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        if (scp_error_flag)
        {
                SCHARGER_ERR("%s : scp timeout happened ,do not read reg = %d \n",__func__,reg);
                return HI6526_FAIL;
        }

        if(di->adaptor_support & SC_MODE) {
                ret = hi6526_fcp_adapter_reg_write_block(reg, val, num);
                if (ret)
                {
                        SCHARGER_ERR("%s : error reg = %d \n",__func__,reg);
                        scp_error_flag = SCP_IS_ERR;
                        return HI6526_FAIL;
                }
        } else {
                for(i = 0; i < num; i++) {
                        ret = scp_adapter_reg_write(val[i], reg + i);
                        if (ret)
                        {
                                SCHARGER_ERR("%s : error reg = %d \n",__func__,reg);
                                scp_error_flag = SCP_IS_ERR;
                                return HI6526_FAIL;
                        }
                }
        }

        return 0;
}

static int hi6526_scp_adaptor_vout_regval_convert(u8 reg_val)
{
	u8 exponent;
	u8 B;
	int A;
	int rs;

	exponent = (SCP_MAX_IOUT_A_MASK & reg_val) >> SCP_MAX_IOUT_A_SHIFT;
	B = SCP_MAX_IOUT_B_MASK & reg_val;
	switch (exponent){
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
		return HI6526_FAIL;
	}
	rs = B*A;
	return rs;
}
static int hi6526_scp_get_adaptor_max_voltage(void)
{
	u8 reg_val;
	int ret;
	int rs;

	ret = scp_adapter_reg_read(&reg_val, SCP_MAX_VOUT);
	if(ret)
	{
		SCHARGER_ERR("%s : read MAX_VOUT failed ,ret = %d \n",__func__,ret);
		return HI6526_FAIL;
	}

	rs = hi6526_scp_adaptor_vout_regval_convert(reg_val);
	SCHARGER_INF("[%s]max_vout reg is 0x%x, val is %d \n", __func__, reg_val,rs);
	return rs;
}
static int hi6526_scp_get_adaptor_min_voltage(void)
{
	u8 reg_val;
	int ret;
	int rs;

	ret = scp_adapter_reg_read(&reg_val, SCP_MIN_VOUT);
	if(ret)
	{
		SCHARGER_ERR("%s : read MIN_VOUT failed ,ret = %d \n",__func__,ret);
		return HI6526_FAIL;
	}

	rs = hi6526_scp_adaptor_vout_regval_convert(reg_val);
	SCHARGER_INF("[%s]min_vout reg is 0x%x, val is %d \n", __func__, reg_val, rs);
	return rs;
}

static int hi6526_scp_type_detect(struct hi6526_device_info *di)
{
        u8 val;
        int ret = SCP_ADAPTOR_DETECT_FAIL;

        /*read 0x7E, judge adaptor type*/
        ret = scp_adapter_reg_read(&val, SCP_ADP_TYPE0);
        if(ret)
        {
                SCHARGER_ERR("%s : read SCP_ADP_TYPE0 fail ,ret = %d \n",__func__,ret);
                return ret;
        }
        SCHARGER_INF("%s : read SCP_ADP_TYPE0 val = 0x%x \n",__func__,val);
        if(val & 0xB0) {
                ret = SCP_ADAPTOR_DETECT_FAIL;
                if (val & SCP_ADP_TYPE0_B_SC_MASK)
                {
                        di->adaptor_support |= SC_MODE;
                        ret = SCP_ADAPTOR_DETECT_SUCC;
                }
                if (!(val & SCP_ADP_TYPE0_B_LVC_MASK))
                {
                        di->adaptor_support |= LVC_MODE;
                        ret = SCP_ADAPTOR_DETECT_SUCC;
                }

              SCHARGER_INF("scp type B, support mode: 0x%x\n ", di->adaptor_support);
              return ret;
        }

        return SCP_ADAPTOR_DETECT_FAIL;
}

static int hi6526_scp_type_detect_old(struct hi6526_device_info *di)
{
        u8 val;
        int ret = SCP_ADAPTOR_DETECT_FAIL;
        int max_voltage = 0, min_voltage = 0;

        /*read 0x80, judge adaptor type*/
        ret = scp_adapter_reg_read(&val, SCP_ADP_TYPE);
        if(ret)
        {
                SCHARGER_ERR("%s : read SCP_ADP_TYPE fail ,ret = %d \n",__func__,ret);
                return SCP_ADAPTOR_DETECT_OTHER;
        }
        SCHARGER_INF("%s : read SCP_ADP_TYPE val = %d \n",__func__,val);
        if (val & SCP_ADP_TYPE_B)
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

                        max_voltage = hi6526_scp_get_adaptor_max_voltage();
                        min_voltage = hi6526_scp_get_adaptor_min_voltage();

                        if(min_voltage < 3700 && max_voltage > 4800) {
                                di->adaptor_support |= LVC_MODE;
                                ret = SCP_ADAPTOR_DETECT_SUCC;
                        }
                        if(min_voltage < 6000 && max_voltage > 7000) {
                                di->adaptor_support |= SC_MODE;
                                ret = SCP_ADAPTOR_DETECT_SUCC;
                        }
                        SCHARGER_INF("scp type B, max vol = %d, min vol = %d, support mode: 0x%x\n ", \
                                                max_voltage, min_voltage, di->adaptor_support);
                        return ret;
                }
        }

        return SCP_ADAPTOR_DETECT_OTHER;
}

static int hi6526_scp_adaptor_detect(void)
{
        int ret;
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }
        scp_error_flag = SCP_NO_ERR;
        di->adaptor_support = 0;

        /* detect FCP adaptor */
        ret = hi6526_fcp_adapter_detect(di);
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

        ret = hi6526_scp_type_detect(di);
        if(SCP_ADAPTOR_DETECT_SUCC == ret)
        {
                SCHARGER_INF("%s sucess, judge by 0x7E\n",__func__);
                return SCP_ADAPTOR_DETECT_SUCC;
        }

        /*read 0x80, judge adaptor type*/
        ret = hi6526_scp_type_detect_old(di);
        if(SCP_ADAPTOR_DETECT_SUCC == ret)
        {
                SCHARGER_INF("%s sucess, judge by 0x80\n",__func__);
                return SCP_ADAPTOR_DETECT_SUCC;
        }

        SCHARGER_INF("%s fail !\n",__func__);

        return SCP_ADAPTOR_DETECT_OTHER;
}
static int hi6526_scp_get_adaptor_type(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        SCHARGER_INF("%s : adaptor_support = 0x%x \n ",__func__, di->adaptor_support);

        return di->adaptor_support;
}

static int hi6526_scp_output_mode_enable(int enable)
{
        u8 val;
        int ret;

        ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
        val &= ~(SCP_OUTPUT_MODE_MASK);
        val |= enable ? SCP_OUTPUT_MODE_ENABLE:SCP_OUTPUT_MODE_DISABLE;
        SCHARGER_INF("[%s]val after is %d \n", __func__, val);
        ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
        if(ret < 0)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
                return HI6526_FAIL;
        }
        return 0;
}
static int hi6526_scp_set_watchdog_timer(int second)
{
        u8 val;
        int ret;

        ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE1);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
        val &= ~(SCP_WATCHDOG_MASK);
        val |= (u8)(((u8)second * ONE_BIT_EQUAL_TWO_SECONDS) & SCP_WATCHDOG_MASK); /*1 bit means 0.5 second*/

        SCHARGER_INF("[%s]val after is %d \n", __func__, val);
        ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE1);
        if(ret < 0)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
                return HI6526_FAIL;
        }
        return 0;
}

static int hi6526_scp_adaptor_output_enable(int enable)
{
        u8 val;
        int ret;
        SCHARGER_INF("%s :%d \n ",__func__, enable);

        ret = hi6526_scp_output_mode_enable(OUTPUT_MODE_ENABLE);
        if(ret)
        {
                SCHARGER_ERR("%s : scp output mode enable failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }

        ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
        val &= ~(SCP_OUTPUT_MASK);
        val |= enable ? SCP_OUTPUT_ENABLE:SCP_OUTPUT_DISABLE;
        SCHARGER_INF("[%s]val after is %d \n", __func__, val);
        ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
        if(ret < 0)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
                return HI6526_FAIL;
        }

        return 0;
}
static int hi6526_adaptor_reset(int enable)
{
        u8 val;
        int ret;

        ret = scp_adapter_reg_read(&val, SCP_CTRL_BYTE0);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        SCHARGER_INF("[%s]val befor is %d \n", __func__, val);
        val &= ~(SCP_ADAPTOR_RESET_MASK);
        val |= enable ? SCP_ADAPTOR_RESET_ENABLE:SCP_ADAPTOR_RESET_DISABLE;
        SCHARGER_INF("[%s]val after is %d \n", __func__, val);
        ret = scp_adapter_reg_write(val, SCP_CTRL_BYTE0);
        if(ret < 0)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
                return HI6526_FAIL;
        }
        return 0;
}
static int hi6526_scp_config_vset_iset_boundary(int vboundary, int iboundary)
{
        int ret;
        u8 val[4];

        val[0] = (vboundary >> ONE_BYTE_LEN) & ONE_BYTE_MASK;  //0xb0
        val[1] = vboundary & ONE_BYTE_MASK;  // 0xb1
        val[2] = (iboundary >> ONE_BYTE_LEN) & ONE_BYTE_MASK; // 0xb2
        val[3] = iboundary & ONE_BYTE_MASK;  // 0xb3

        ret = scp_adapter_reg_write_block(SCP_VSET_BOUNDARY_L, &val[0], 4);
        if (ret < 0)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
        }
        return ret;
}

static int hi6526_scp_set_adaptor_voltage(int vol)
{
        int ret = 0;
        u8 val[2];

        /*high byte store in low address*/
        val[0] = (vol >> ONE_BYTE_LEN) & ONE_BYTE_MASK;
        /*low byte store in high address*/
        val[1] = vol & ONE_BYTE_MASK;
     //   SCHARGER_ERR("%s : %d, val0: 0x%x, val1: 0x%x \n ",__func__, vol, val[0], val[1]);

        ret = scp_adapter_reg_write_block(SCP_VSET_L, &val[0], 2);
        if(ret)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
        }

        return ret;
}

static int hi6526_scp_master_init(void)
{
        int ret = 0;

        ret |= hi6526_set_charge_enable(CHG_DISABLE);
        ret |= set_buck_mode_enable(CHG_DISABLE);
        ret |= hi6526_set_vbus_vset(VBUS_VSET_12V);
        ret |= hi6526_set_fast_safe_timer(CHG_FASTCHG_TIMER_5H);
        ret |= hi6526_set_watchdog_timer(WATCHDOG_TIMER_40_S);
        ret |= hi6526_set_batfet_ctrl(CHG_BATFET_EN);
        ret |= hi6526_set_otg_current(BOOST_LIM_1000);
        ret |= hi6526_set_otg_enable(OTG_DISABLE);

        return ret;

}

static int hi6526_self_check(void)
{
        int ret = 0; // 0:SUCESS, others fail

        return ret;
}
static int hi6526_scp_init(struct scp_init_data * sid)
{
        int ret = 0, i = 0;
        u8 val[4] = {0};

        scp_error_flag = SCP_NO_ERR;

        ret = hi6526_self_check();
        if(ret) {
                SCHARGER_ERR("%s : hi6526_self_check fail \n ",__func__);
                return ret;
        }
        hi6526_scp_master_init();

        ret = hi6526_scp_output_mode_enable(sid->scp_mode_enable);
        if(ret)
                return ret;

        ret = hi6526_scp_config_vset_iset_boundary(sid->vset_boundary, sid->iset_boundary);
        if(ret)
                return ret;
        ret = hi6526_scp_set_adaptor_voltage(sid->init_adaptor_voltage);
        if(ret)
                return ret;

        ret = hi6526_scp_set_watchdog_timer(sid->watchdog_timer);
        if(ret)
                return ret;

        ret = scp_adapter_reg_read_block(SCP_CTRL_BYTE0, &val[0], 4);
        if(ret)
                return ret;
        for(i = 0; i < 4; i++) {
                SCHARGER_INF("%s : adapter reg 0x%x = 0x%x \n ",__func__, SCP_CTRL_BYTE0 + i, val[i]);
        }

        ret = scp_adapter_reg_read_block(SCP_VSET_BOUNDARY_L, &val[0], 4);
        if(ret)
                return ret;
        for(i = 0; i < 4; i++) {
                SCHARGER_INF("%s : adapter reg 0x%x = 0x%x \n ",__func__, SCP_VSET_BOUNDARY_L + i, val[i]);
        }

        return ret;
}
static int hi6526_scp_chip_reset(void)
{
        int ret;

        ret = hi6526_fcp_master_reset();
        if(ret)
        {
                SCHARGER_INF("%s:hi6526_fcp_master_reset fail!\n ",__func__);
                return HI6526_FAIL;
        }

        return 0;
}
static int hi6526_scp_exit(struct direct_charge_device* di)
{
        int ret;

        ret = hi6526_scp_output_mode_enable(OUTPUT_MODE_DISABLE);
        switch(di->adaptor_vendor_id)
        {
                case IWATT_ADAPTER:
                        ret  |= hi6526_adaptor_reset(ADAPTOR_RESET);
                        break;
                default:
                        SCHARGER_INF("%s:not iWatt\n",__func__);
        }
        msleep(WAIT_FOR_ADAPTOR_RESET);

        SCHARGER_INF("%s\n",__func__);
        scp_error_flag = SCP_NO_ERR;
        return ret;
}
static int hi6526_scp_get_adaptor_voltage(void)
{
        int vol;
        u8 val[2] = {0};
        int ret = 0;

        //get adapter voltage from 0xA8 and 0xA9
        ret = scp_adapter_reg_read_block(SCP_READ_VOLT_L, &val[0], 2);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        vol = val[0] << ONE_BYTE_LEN;
        vol += val[1];

        return vol;
}
static int hi6526_scp_set_adaptor_current(int cur)
{
        int ret;
        u8 val[2];

        /*high byte store in low address*/
        val[0] = (cur >> ONE_BYTE_LEN) & ONE_BYTE_MASK;
        /*low byte store in high address*/
        val[1] = cur & ONE_BYTE_MASK;

        ret = scp_adapter_reg_write_block(SCP_ISET_L, &val[0], 2);
        if(ret)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
        }
        return ret;
}
static int hi6526_scp_get_adaptor_current(void)
{
        int ret, curr;
        u8 val[2];

        ret = scp_adapter_reg_read_block(SCP_READ_IOLT_L, &val[0], 2);
        if(ret)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
        }
        curr = (val[0] << ONE_BYTE_LEN) | val[1] ;

        return curr;
}
static int hi6526_scp_get_adaptor_current_set(void)
{
        int curr;
        int ret;
        u8 val[2];

        ret = scp_adapter_reg_read_block(SCP_ISET_L, &val[0], 2);
        if(ret)
        {
                SCHARGER_ERR("%s : failed \n ",__func__);
        }

        curr = (val[0] << ONE_BYTE_LEN) | val[1];
        return curr;
}
static int hi6526_scp_get_adaptor_max_current(void)
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
                return HI6526_FAIL;
        }
        SCHARGER_DBG("[%s]max_iout reg is %d \n", __func__, reg_val);
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
                return HI6526_FAIL;
        }
        rs = B*A;
        SCHARGER_DBG("[%s]MAX IOUT initial is %d \n", __func__, rs);
        ret = scp_adapter_reg_read(&reg_val, SCP_SSTS);
        if(ret)
        {
                SCHARGER_ERR("%s : read SSTS failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        SCHARGER_DBG("[%s]ssts reg is %d \n", __func__, reg_val);
        B = (SCP_SSTS_B_MASK & reg_val) >> SCP_SSTS_B_SHIFT;
        A = SCP_SSTS_A_MASK & reg_val;
        if (DROP_POWER_FLAG == B)
        {
                rs = rs * A / DROP_POWER_FACTOR;
        }
        SCHARGER_DBG("[%s]MAX IOUT final is %d \n", __func__, rs);
        return rs;
}

static int hi6526_scp_get_adaptor_temp(int* temp)
{
        u8 val = 0;
        int ret;

        ret = scp_adapter_reg_read(&val, SCP_INSIDE_TMP);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        SCHARGER_DBG("[%s]val is %d \n", __func__, val);
        *temp = val;

        return 0;
}

static int hi6526_scp_cable_detect(void)
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
static int hi6526_scp_adaptor_reset(void)
{
        return hi6526_fcp_adapter_reset();
}
static int hi6526_scp_stop_charge_config(void)
{
        return 0;
}
static int hi6526_is_scp_charger_type(void)
{
        return is_fcp_charger_type();
}
static int hi6526_scp_get_adaptor_status(void)
{
        return 0;
}
static int hi6526_scp_get_adaptor_info(void* info)
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

static int hi6526_get_adapter_vendor_id(void)
{
        u8 val = 0;
        int ret;

        ret = scp_adapter_reg_read(&val, SCP_PCHIP_ID);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }
        switch (val)
        {
                case VENDOR_ID_RICHTEK:
                        SCHARGER_INF("[%s] 0x%x, adapter is richtek \n", __func__, val);
                        return RICHTEK_ADAPTER;
                case VENDOR_ID_IWATT:
                        SCHARGER_INF("[%s]0x%x, adapter is iwatt \n", __func__, val);
                        return IWATT_ADAPTER;
                default:
                        SCHARGER_INF("[%s]0x%x,  this adaptor vendor id is not found!\n", __func__, val);
                        return val;
        }
}
static int hi6526_get_usb_port_leakage_current_info(void)
{
        u8 val = 0;
        int ret;

        ret = scp_adapter_reg_read(&val, SCP_STATUS_BYTE0);
        if(ret)
        {
                SCHARGER_ERR("%s : read failed ,ret = %d \n",__func__,ret);
                return HI6526_FAIL;
        }

        SCHARGER_INF("[%s]val is 0x%x \n", __func__, val);
        val &= SCP_PORT_LEAKAGE_INFO;
        val = val>>SCP_PORT_LEAKAGE_SHIFT;
        return val;
}
static int hi6526_scp_get_chip_status(void)
{
        return 0;
}

/* for loadswitch ops */
static int hi6526_dummy_fun_1(void )
{
        return 0;
}
static int hi6526_dummy_fun_2(int val )
{
        return 0;
}

static void hi6526_after_direct_charger(struct hi6526_device_info *di, int enable)
{

        if(enable) {
                if(di->chg_mode == LVC)
                        hi6526_opt_regs_set(lvc_opt_regs_after_enabled,  \
                                                ARRAY_SIZE(lvc_opt_regs_after_enabled));
                else if (di->chg_mode == SC)
                        hi6526_opt_regs_set(sc_opt_regs_after_enabled,  \
                                                ARRAY_SIZE(sc_opt_regs_after_enabled));
                else {
                        SCHARGER_ERR("%s chg mode %d  error !\n", __func__, di->chg_mode);
                        return;
                }
                di->dbg_work_stop = 0;
                di->abnormal_happened = 0;
                di->ucp_work_first_run = 1;
                queue_delayed_work(system_power_efficient_wq, &di->dc_ucp_work,
                                msecs_to_jiffies(IBUS_ABNORMAL_TIME));
                queue_delayed_work(system_power_efficient_wq, &di->dbg_work,
                                msecs_to_jiffies(DBG_WORK_TIME));
        } else {
                di->chg_mode = NONE;
                di->ucp_work_first_run = 0;
                di->dbg_work_stop = 1;

        }
}

static int hi6526_lvc_enable(int enable )
{
        struct hi6526_device_info *di = g_hi6526_dev;
        u8 lvc_mode = 0;
         if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }
         hi6526_read_mask(LVC_CHG_MODE_REG, LVC_CHG_MODE_MASK,LVC_CHG_MODE_SHIFT, &lvc_mode);

        if(!enable && LVC != di->chg_mode && (!lvc_mode) )
                return 0;

        SCHARGER_INF("[%s] %d \n", __func__, enable);

        if(enable) {
                di->chg_mode = LVC;
                hi6526_lvc_opt_regs();
        }
        hi6526_write_mask(LVC_CHG_MODE_FLAG_REG,\
                LVC_CHG_MODE_FLAG_MASK, LVC_CHG_MODE_FLAG_SHIFT, !!enable);
        hi6526_write_mask(LVC_CHG_MODE_REG, \
                LVC_CHG_MODE_MASK, LVC_CHG_MODE_SHIFT, !!enable);
        hi6526_write_mask(LVC_CHG_EN_REG,  \
                LVC_CHG_EN_MASK, LVC_CHG_EN_SHIFT, !!enable);

        hi6526_after_direct_charger(di, enable);

        return 0;

}

static int hi6526_sc_enable(int enable )
{
        struct hi6526_device_info *di = g_hi6526_dev;
        u8 sc_mode = 0;
         if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }
        hi6526_read_mask(SC_CHG_MODE_REG, SC_CHG_MODE_MASK,SC_CHG_MODE_SHIFT, &sc_mode);

        if(!enable && SC != di->chg_mode && (!sc_mode))
                return 0;

        SCHARGER_INF("[%s] %d \n", __func__, enable);

        if(enable) {
                di->chg_mode = SC;
                hi6526_sc_opt_regs();
        }
        hi6526_write_mask(SC_CHG_MODE_FLAG_REG,\
                SC_CHG_MODE_FLAG_MASK, SC_CHG_MODE_FLAG_SHIFT, !!enable);
        hi6526_write_mask(SC_CHG_MODE_REG, \
                SC_CHG_MODE_MASK, SC_CHG_MODE_SHIFT, !!enable);
        hi6526_write_mask(SC_CHG_EN_REG,  \
                SC_CHG_EN_MASK, SC_CHG_EN_SHIFT, !!enable);

        hi6526_after_direct_charger(di, enable);

        return 0;
}

static int hi6526_batinfo_get_ibus_ma(int *vbus_mv)
{
        *vbus_mv = hi6526_get_ibus_ma();
        return 0;
}
static int hi6526_get_loadswitch_id(void)
{
        return LOADSWITCH_SCHARGERV600;
}
static int hi6526_get_switchcap_id(void)
{
        return SWITCHCAP_SCHARGERV600;
}
#ifdef CONFIG_WIRED_CHANNEL_SWITCH
static int hi6526_set_wired_channel(int flag)
{
        return 0;
}
static struct wired_chsw_device_ops hi6526_chsw_ops = {
	.set_wired_channel = hi6526_set_wired_channel,
};
#endif
static int hi6526_switch_to_buck_mode(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;
         if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }

        di->dc_ibus_ucp_happened = 0;

        /* switch to BUCK */
        set_buck_mode_enable(CHG_ENABLE);
        hi6526_set_charge_enable(CHG_DISABLE);
        hi6526_config_opt_param(VBUS_VSET_5V);
        hi6526_set_fast_safe_timer(CHG_FASTCHG_TIMER_20H);
        hi6526_dpm_init();
        hi6526_set_term_enable(CHG_TERM_DIS);
        hi6526_set_input_current(CHG_ILIMIT_475);
        hi6526_set_charge_current(CHG_FAST_ICHG_500MA);
        hi6526_set_terminal_voltage(CHG_FAST_VCHG_4400);
        hi6526_set_terminal_current(CHG_TERM_ICHG_150MA);
        hi6526_set_watchdog_timer(WATCHDOG_TIMER_40_S);

        return 0;
}
static int hi6526_scp_set_adaptor_encrypt_enable(int type)
{
        int ret = 0;

        ret = scp_adapter_reg_write(type, SCP_ADAPTOR_KEY_INDEX_REG);
        if (ret) {
                SCHARGER_ERR("%s: scp_adapter_reg_write  SCP_ADAPTOR_KEY_INDEX_REG failed, value = %02X\n", __func__, type);
                return HI6526_FAIL;
        }

        return 0;
}

static int hi6526_scp_get_adaptor_encrypt_enable(void)
{
        int ret = 0;
        u8 val = 0;

        ret = scp_adapter_reg_read(&val, SCP_ADAPTOR_ENCRYPT_INFO_REG);
        if (ret) {
                SCHARGER_ERR("%s: scp_adapter_reg_read SCP_ADAPTOR_ENCRYPT_INFO_REG failed\n", __func__);
                return HI6526_FAIL;
        }

        if (!(val & SCP_ADAPTOR_ENCRYPT_ENABLE)) {
                SCHARGER_ERR("%s: SCP_ADAPTOR_ENCRYPT_ENABLE val = %d\n", __func__, val);
                return HI6526_FAIL;
        }

        return 0;
}
static int hi6526_scp_set_adaptor_random_num(char *random_local)
{
        int ret = 0;

        ret = hi6526_fcp_adapter_reg_write_block(SCP_ADAPTOR_RANDOM_NUM_HI_BASE_REG, random_local, SCP_ADAPTOR_RANDOM_NUM_HI_LEN);
        if (ret) {
                SCHARGER_ERR("%s: hi6526_fcp_adapter_reg_write_block SCP_ADAPTOR_RANDOM_NUM_REG failed\n", __func__);
                return HI6526_FAIL;
        }
        return 0;
}

static int hi6526_scp_get_adaptor_encrypt_completed(void)
{
        int ret = 0;
        u8 val = 0;

        ret = scp_adapter_reg_read(&val, SCP_ADAPTOR_ENCRYPT_INFO_REG);
        if (ret) {
                SCHARGER_ERR("%s : scp_adapter_reg_read  SCP_ADAPTOR_ENCRYPT_INFO_REG failed\n", __func__);
                return HI6526_FAIL;
        }

        if( !(val & SCP_ADAPTOR_ENCRYPT_COMPLETE)) {
                SCHARGER_ERR("%s: SCP_ADAPTOR_ENCRYPT_COMPLETE val = %d\n", __func__, val);
                return HI6526_FAIL;
        }

        return 0;
}
static int hi6526_scp_get_adaptor_random_num(char *num)
{
        int ret = 0;

        (void)memset_s(scaf_randnum_remote, SCP_ADAPTOR_RANDOM_NUM_HI_LEN, 0, SCP_ADAPTOR_RANDOM_NUM_HI_LEN);

        ret = hi6526_fcp_adapter_reg_read_block(SCP_ADAPTOR_RANDOM_NUM_LO_BASE_REG, scaf_randnum_remote, SCP_ADAPTOR_RANDOM_NUM_HI_LEN);
        if (ret) {
                SCHARGER_ERR("%s: hi6526_fcp_adapter_reg_read_block SCP_ADAPTOR_RANDOM_NUM_LO_BASE_REG failed\n", __func__);
                return HI6526_FAIL;
        }

        memcpy_s(num, SCP_ADAPTOR_RANDOM_NUM_HI_LEN, scaf_randnum_remote, SCP_ADAPTOR_RANDOM_NUM_HI_LEN);
        return 0;
}
static int hi6526_scp_get_adaptor_encrypted_value(char *hash)
{
        int ret = 0;

        (void)memset_s(scaf_digest_remote_hi, sizeof(scaf_digest_remote_hi), 0, sizeof(scaf_digest_remote_hi));

        ret = hi6526_fcp_adapter_reg_read_block(SCP_ADAPTOR_DIGEST_BASE_REG, scaf_digest_remote_hi, SCP_ADAPTOR_DIGEST_LEN);
        if (ret) {
                SCHARGER_ERR("%s: hi6526_fcp_adapter_reg_read_block SCP_ADAPTOR_DIGEST_BASE_REG failed\n", __func__);
                return HI6526_FAIL;
        }

        memcpy_s(hash, SCP_ADAPTOR_DIGEST_LEN, scaf_digest_remote_hi, SCP_ADAPTOR_DIGEST_LEN);
        return 0;
}

static struct loadswitch_ops  hi6526_lvc_ops ={
        .ls_init = hi6526_dummy_fun_1,
        .ls_exit= hi6526_switch_to_buck_mode,
        .ls_enable = hi6526_lvc_enable,
        .ls_discharge = hi6526_dummy_fun_2,
        .is_ls_close = hi6526_dummy_fun_1,
        .get_ls_id = hi6526_get_loadswitch_id,
        .watchdog_config_ms = hi6526_set_watchdog_timer_ms,
        .kick_watchdog = hi6526_reset_watchdog_timer,
};
static struct loadswitch_ops  hi6526_sc_ops ={
        .ls_init = hi6526_dummy_fun_1,
        .ls_exit= hi6526_switch_to_buck_mode,
        .ls_enable = hi6526_sc_enable,
        .ls_discharge = hi6526_dummy_fun_2,
        .is_ls_close = hi6526_dummy_fun_1,
        .get_ls_id = hi6526_get_switchcap_id,
        .watchdog_config_ms = hi6526_set_watchdog_timer_ms,
        .kick_watchdog = hi6526_reset_watchdog_timer,
};
static struct batinfo_ops hi6526_batinfo_ops = {
        .init = hi6526_dummy_fun_1,
        .exit = hi6526_dummy_fun_1,
        .get_bat_btb_voltage =  hi6526_get_vbat,
        .get_bat_package_voltage =  hi6526_get_vbat,
        .get_vbus_voltage =  hi6526_get_vbus_mv2,
        .get_bat_current = hi6526_get_ibat,
        .get_ls_ibus = hi6526_batinfo_get_ibus_ma,
        .get_ls_temp = hi6526_get_chip_temp,
};

struct smart_charge_ops scp_hi6526_ops = {
        .is_support_scp = hi6526_is_support_scp,
        .scp_init = hi6526_scp_init,
        .scp_exit = hi6526_scp_exit,
        .scp_adaptor_detect = hi6526_scp_adaptor_detect,
        .scp_get_adaptor_type = hi6526_scp_get_adaptor_type,
        .scp_set_adaptor_voltage = hi6526_scp_set_adaptor_voltage,
        .scp_get_adaptor_voltage = hi6526_scp_get_adaptor_voltage,
        .scp_set_adaptor_current = hi6526_scp_set_adaptor_current,
        .scp_get_adaptor_current = hi6526_scp_get_adaptor_current,
        .scp_get_adaptor_current_set = hi6526_scp_get_adaptor_current_set,
        .scp_get_adaptor_max_current = hi6526_scp_get_adaptor_max_current,
        .scp_adaptor_reset = hi6526_scp_adaptor_reset,
        .scp_adaptor_output_enable = hi6526_scp_adaptor_output_enable,
        .scp_chip_reset = hi6526_scp_chip_reset,
        .scp_stop_charge_config = hi6526_scp_stop_charge_config,
        .is_scp_charger_type = hi6526_is_scp_charger_type,
        .scp_get_adaptor_status = hi6526_scp_get_adaptor_status,
        .scp_get_adaptor_info = hi6526_scp_get_adaptor_info,
        .scp_get_chip_status = hi6526_scp_get_chip_status,
        .scp_cable_detect = hi6526_scp_cable_detect,
        .scp_get_adaptor_temp = hi6526_scp_get_adaptor_temp,
        .scp_get_adapter_vendor_id = hi6526_get_adapter_vendor_id,
        .scp_get_usb_port_leakage_current_info = hi6526_get_usb_port_leakage_current_info,
        .scp_set_direct_charge_mode = scp_set_direct_charge_mode,
        .scp_set_adaptor_encrypt_enable = hi6526_scp_set_adaptor_encrypt_enable,
        .scp_get_adaptor_encrypt_enable = hi6526_scp_get_adaptor_encrypt_enable,
        .scp_set_adaptor_random_num = hi6526_scp_set_adaptor_random_num,
        .scp_get_adaptor_encrypt_completed = hi6526_scp_get_adaptor_encrypt_completed,
        .scp_get_adaptor_random_num = hi6526_scp_get_adaptor_random_num,
        .scp_get_adaptor_encrypted_value = hi6526_scp_get_adaptor_encrypted_value,
};
struct fcp_adapter_device_ops fcp_hi6526_ops = {
        .get_adapter_output_current = hi6526_fcp_get_adapter_output_current,
        .set_adapter_output_vol = hi6526_fcp_set_adapter_output_vol,
        .detect_adapter = fcp_adapter_detect,
        .is_support_fcp = hi6526_is_support_fcp,
        .switch_chip_reset = hi6526_fcp_master_reset,
        .fcp_adapter_reset = hi6526_fcp_adapter_reset,
        .stop_charge_config = hi6526_fcp_stop_charge_config,
        .is_fcp_charger_type    = is_fcp_charger_type,
        .fcp_read_adapter_status = fcp_read_adapter_status,
        .fcp_read_switch_status = fcp_read_switch_status,
        .reg_dump = hi6526_reg_dump,
};

struct charge_device_ops hi6526_ops = {
        .chip_init = hi6526_chip_init,
        .get_dieid = hi6526_get_dieid,
        .dev_check = hi6526_device_check,
        .set_input_current = hi6526_set_input_current,
        .set_charge_current = hi6526_set_charge_current,
        .get_charge_current = hi6526_get_charge_current,
        .set_terminal_voltage = hi6526_set_terminal_voltage,
        .set_terminal_current = hi6526_set_terminal_current,
        .set_term_enable = hi6526_set_term_enable,
        .set_force_term_enable = hi6526_force_set_term_enable,
        .set_charge_enable = hi6526_set_charge_enable,
        .get_charge_state = hi6526_get_charge_state,
        .get_charger_state = hi6526_get_charger_state,
        .set_watchdog_timer = hi6526_set_watchdog_timer,
        .reset_watchdog_timer = hi6526_reset_watchdog_timer,
        .set_batfet_disable = hi6526_set_batfet_disable,
        .set_charger_hiz = hi6526_set_charger_hiz,
        .get_ibus = hi6526_get_ibus_ma,
        .get_vbus = hi6526_get_vbus_mv,
        .get_vbat_sys = NULL,
        .check_input_dpm_state = hi6526_check_input_dpm_state,
        .check_input_vdpm_state = hi6526_check_input_dpm_state,
        .check_input_idpm_state = hi6526_check_input_acl_state,
        .set_dpm_voltage = hi6526_dummy_fun_2,
        .set_vbus_vset = hi6526_set_vbus_vset,
        .set_uvp_ovp = hi6526_dummy_fun_1,
        .soft_vbatt_ovp_protect = hi6526_soft_vbatt_ovp_protect,
        .rboost_buck_limit = hi6526_rboost_buck_limit,
        .stop_charge_config = hi6526_stop_charge_config,
        .set_otg_enable = hi6526_set_otg_enable,
        .set_otg_current = hi6526_set_otg_current,
        .set_otg_switch_mode_enable = hi6526_otg_switch_mode,
        .get_register_head = hi6526_get_register_head,
        .dump_register = hi6526_dump_register,
};

struct  water_detect_ops hi6526_water_detect_ops = {
        .is_water_intrused = hi6526_is_water_intrused,
};

/**********************************************************
*  Function:       hi6526_mask_all_irq
*  Description:    inti irq status and configuration
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void hi6526_mask_all_irq(void)
{
        /* mask all irq output  */
        hi6526_write_mask(CHG_IRQ_MASK_ALL_ADDR, CHG_IRQ_MASK_ALL_MSK,
                                        CHG_IRQ_MASK_ALL_SHIFT, CHG_IRQ_MASK_ALL_MSK);
}


/**********************************************************
*  Function:       hi6526_unmask_all_irq
*  Description:    inti irq status and configuration
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void hi6526_unmask_all_irq(void)
{
        /* buck irq unmask */
        hi6526_write(CHG_IRQ_MASK_0, CHG_IRQ_MASK_0_VAL);
        hi6526_write(CHG_IRQ_MASK_1, CHG_IRQ_MASK_1_VAL);
        hi6526_write(CHG_IRQ_MASK_2, CHG_IRQ_MASK_2_VAL);
        hi6526_write(CHG_IRQ_MASK_3, CHG_IRQ_MASK_3_VAL);
        hi6526_write(CHG_IRQ_MASK_4, CHG_IRQ_MASK_4_VAL);
        hi6526_write(CHG_IRQ_MASK_5, CHG_IRQ_MASK_5_VAL);
        hi6526_write(CHG_IRQ_MASK_6, CHG_IRQ_MASK_6_VAL);
        hi6526_write(CHG_IRQ_MASK_7, CHG_IRQ_MASK_7_VAL);

        // FCP MASK
        hi6526_write(CHG_FCP_IRQ5_MASK_ADDR, 0x03);
        hi6526_write(0x9b, 0xff);
        hi6526_write(0x9c, 0xff);

        /* mask all irq output  */
        hi6526_write_mask(CHG_IRQ_MASK_ALL_ADDR, CHG_IRQ_MASK_ALL_MSK,
                                        CHG_IRQ_MASK_ALL_SHIFT, CHG_IRQ_UNMASK_DEFAULT);
}

static void hi6526_lvc_sc_irq2str(u32 lvc_sc_irq_state)
{
        int index = 0;
        char buf[512] = { 0 };

        lvc_sc_irq_state &= LVC_SC_IRQ_MASK;
        snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, "hi6526_irq_work :lvc_sc_irq_state: 0x%x : ", lvc_sc_irq_state);
        for(index = 24; index >= 0; index--) {
                if(lvc_sc_irq_state & (1 << index)) {
                        if(strlen(lvc_sc_irq_str[index]) < 512)
                                strncat_s(buf, strlen(lvc_sc_irq_str[index]), lvc_sc_irq_str[index], strlen(lvc_sc_irq_str[index]));
                }
        }

        SCHARGER_ERR("%s \n", buf);
}

static void hi6526_direct_dmd_report(u32 fault_type, struct nty_data * data)
{
#ifdef CONFIG_DIRECT_CHARGER
        struct hi6526_device_info *di = g_hi6526_dev;
        struct atomic_notifier_head *direct_charge_fault_notifier_list;
        if((NULL == di) || !(SC == di->chg_mode || LVC == di->chg_mode))
                return;

        if(SC == di->chg_mode)
                direct_charge_sc_get_fault_notifier(&direct_charge_fault_notifier_list);
        else
                direct_charge_lvc_get_fault_notifier(&direct_charge_fault_notifier_list);

        atomic_notifier_call_chain(direct_charge_fault_notifier_list, fault_type, data);
#endif
}

static void hi6526_direct_charge_fault_handle(u32 lvc_sc_irq_state)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        u32 fault_type = DIRECT_CHARGE_FAULT_NON;
        struct nty_data * data = &(di->dc_nty_data);

        if((NULL == di) || !(SC == di->chg_mode || LVC == di->chg_mode))
                return;

        di->abnormal_happened = 1;
        hi6526_lvc_sc_irq2str(lvc_sc_irq_state);

        if(lvc_sc_irq_state & (LVC_REGULATOR_IRQ_MASK | IRQ_IBUS_DC_UCP_MASK))
                return;

        data->addr = di->client->addr;
        data->event1 = (lvc_sc_irq_state & 0xff0000) >> 16;
        data->event2 = (lvc_sc_irq_state & 0x00ff00) >> 8;
        data->event3 = (lvc_sc_irq_state & 0xff);

        if (lvc_sc_irq_state & FAULT_VBUS_OVP)
        {
                SCHARGER_ERR("hi6526_irq_work: chg_mode %s, vbus ovp happened\n", (SC == di->chg_mode)? "SC":"LVC" );
                fault_type = DIRECT_CHARGE_FAULT_VBUS_OVP;
        }
        if (lvc_sc_irq_state & FAULT_IBUS_OCP)
        {
                SCHARGER_ERR("hi6526_irq_work: chg_mode %s, ibus ocp happened\n", (SC == di->chg_mode)? "SC":"LVC" );
                fault_type = DIRECT_CHARGE_FAULT_IBUS_OCP;
        }
        if (lvc_sc_irq_state & FAULT_REVERSE_OCP)
        {
                SCHARGER_ERR("hi6526_irq_work: chg_mode %s, reverse ocp happened\n", (SC == di->chg_mode)? "SC":"LVC" );
                fault_type = DIRECT_CHARGE_FAULT_REVERSE_OCP;
        }
        if (lvc_sc_irq_state & FAULT_VDROP_OVP)
        {
                SCHARGER_ERR("hi6526_irq_work: chg_mode %s, vdrop ovp happened\n", (SC == di->chg_mode)? "SC":"LVC" );
                fault_type = DIRECT_CHARGE_FAULT_VDROP_OVP;
        }
        if (lvc_sc_irq_state & FAULT_VBAT_OVP)
        {
                SCHARGER_ERR("hi6526_irq_work: chg_mode %s, vbat ovp happened\n", (SC == di->chg_mode)? "SC":"LVC" );
                fault_type = DIRECT_CHARGE_FAULT_VBAT_OVP;
        }
        if (lvc_sc_irq_state & FAULT_IBAT_OCP)
        {
                SCHARGER_ERR("hi6526_irq_work: chg_mode %s, vbat ovp happened\n", (SC == di->chg_mode)? "SC":"LVC" );
                fault_type = DIRECT_CHARGE_FAULT_IBAT_OCP;
        }
        hi6526_direct_dmd_report(fault_type, data);
        hi6526_lvc_enable(0);
        hi6526_sc_enable(0);
}
static void hi6526_chip_overtemp_handle(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        struct nty_data * data = &(di->dc_nty_data);

        if((NULL == di) || !(SC == di->chg_mode || LVC == di->chg_mode))
                return;

        hi6526_direct_dmd_report( DIRECT_CHARGE_FAULT_TDIE_OTP,data);
}

static void hi6526_buck_vbat_ovp_handle(void)
{
        int i = 0;
        u8 vbat_ovp_cnt = 0, irq_st0 = 0;

        SCHARGER_ERR("%s : irq_vbus_ovp \n", __func__);
        for (i = 0; i < 5; i++) {
                hi6526_read(CHG_IRQ_STATUS0, &irq_st0);
                if (CHG_VBAT_OVP == (irq_st0 & CHG_VBAT_OVP)) {
                        vbat_ovp_cnt++;
                        mdelay(2);
                } else {
                        vbat_ovp_cnt = 0;
                        break;
                }
        }
        if (vbat_ovp_cnt >= 5) {
                SCHARGER_ERR("%s : CHARGE_FAULT_VBAT_OVP\n", __func__);
                hi6526_set_input_current_limit(1);
                atomic_notifier_call_chain(&fault_notifier_list, CHARGE_FAULT_VBAT_OVP, NULL);
        }

}

static void hi6526_buck_fault_handle(u32 buck_irq_state)
{
        unsigned long jiffies_cur = jiffies;
        struct hi6526_device_info *di = g_hi6526_dev;

        if((NULL == di) || SC == di->chg_mode || LVC == di->chg_mode)
                return;

        if(buck_irq_state & FAULT_BUCK_VBAT_OVP) {
                hi6526_buck_vbat_ovp_handle();
        }
        if(buck_irq_state & FAULT_OTG_OCP) {
                SCHARGER_ERR("%s : CHARGE_FAULT_BOOST_OCP\n", __func__);
                atomic_notifier_call_chain(&fault_notifier_list, CHARGE_FAULT_BOOST_OCP, NULL);
        }
        if(buck_irq_state & FAULT_REVERSBST) {
                SCHARGER_ERR("%s : irq_reversbst , cnt:%d\n", __func__, di->reverbst_cnt);
                if(time_after(jiffies_cur, di->reverbst_begin + HZ * 30)) {
                        di->reverbst_begin = jiffies_cur;
                        di->reverbst_cnt = 0;
                } else {
                        di->reverbst_cnt++;
                }

                if(di->reverbst_cnt < REVERBST_RETRY)
                        hi6526_set_anti_reverbst_reset();
               else {
                        SCHARGER_ERR("%s : CHARGE_FAULT_WEAKSOURCE\n", __func__);
                        atomic_notifier_call_chain(&fault_notifier_list, CHARGE_FAULT_WEAKSOURCE, NULL);
                }
        }
        if(buck_irq_state & (FAULT_CHG_DONE | FAULT_CHG_FAULT)) {
                hi6526_set_input_current_limit(1);
        }
        if(buck_irq_state & FAULT_RECHG) {
                hi6526_set_input_current_limit(0);
        }
}

/**********************************************************
*  Function:       hi6526_irq_work
*  Description:    handler for chargerIC fault irq in charging process
*  Parameters:   work:chargerIC fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void hi6526_irq_work(struct work_struct *work)
{
        u8 irq_state = 0;
        u32 buck_irq_state = 0 , lvc_sc_irq_state = 0;
        u16 fcp_irq_state1 = 0,fcp_irq_state2 = 0;
        u8 pd_irq_state = 0, fcp_irq_state3 = 0;
        u32 others_irq_state = 0;
        struct hi6526_device_info *di =
                container_of(work, struct hi6526_device_info, irq_work);

        hi6526_mask_all_irq();
        hi6526_read(CHG_IRQ_ADDR, &irq_state);
        SCHARGER_INF("%s :irq_state: 0x%x \n", __func__, irq_state);

        if(irq_state & CHG_BUCK_IRQ) {
                hi6526_read_block(CHG_BUCK_IRQ_ADDR, (u8*) &buck_irq_state, 3);
                SCHARGER_INF("%s : CHG_BUCK_IRQ, irq_state: 0x%x, buck_irq_state: 0x%x \n",
                                                        __func__, irq_state, buck_irq_state);
                buck_irq_state &= BUCK_IRQ_MASK;
                hi6526_buck_fault_handle(buck_irq_state);
                hi6526_write_block(CHG_BUCK_IRQ_ADDR, (u8*) &buck_irq_state, 3);
        }
        if(irq_state & CHG_LVC_SC_IRQ) {
                hi6526_read_block(CHG_LVC_SC_IRQ_ADDR, (u8*) &lvc_sc_irq_state, 3);
                SCHARGER_ERR("%s : CHG_LVC_SC_IRQ, irq_state: 0x%x, lvc_sc_irq_state: 0x%x \n",
                                                __func__, irq_state, lvc_sc_irq_state);
                lvc_sc_irq_state &= LVC_SC_IRQ_MASK;
                hi6526_direct_charge_fault_handle(lvc_sc_irq_state);
                hi6526_write_block(CHG_LVC_SC_IRQ_ADDR, (u8*) &lvc_sc_irq_state, 3);
        }

        if(irq_state & CHG_PD_IRQ) {
                hi6526_read(CHG_PD_IRQ_ADDR, &pd_irq_state);
                SCHARGER_INF("%s : CHG_PD_IRQ, irq_state: 0x%x, pd_irq_state: 0x%x \n",
                                                __func__, irq_state, pd_irq_state);
                pd_irq_state &= PD_IRQ_MASK;
                hi6526_write(CHG_PD_IRQ_ADDR, pd_irq_state);
        }

        if(irq_state & CHG_OTHERS_IRQ) {
                hi6526_read_block(CHG_OTHERS_IRQ_ADDR, (u8*) &others_irq_state, 3);
                SCHARGER_INF("%s : CHG_OTHERS_IRQ, irq_state: 0x%x, others_irq_state: 0x%x \n",
                                                __func__, irq_state, others_irq_state);

                if(others_irq_state & OTHERS_VDROP_IRQ_MASK) {
                        if(others_irq_state & IRQ_VDROP_MIN_MASK)
                                SCHARGER_ERR("hi6526_irq_work : irq_vdrop_min \n ");
                        if(others_irq_state & IRQ_VDROP_MIN_MASK)
                                SCHARGER_ERR("hi6526_irq_work : irq_vdrop_ovp \n ");
                }
                if(others_irq_state & OTHERS_OTP_IRQ_MASK)
                        hi6526_chip_overtemp_handle();

                others_irq_state &= OTHERS_IRQ_MASK;
                hi6526_write_block(CHG_OTHERS_IRQ_ADDR, (u8*) &others_irq_state, 3);
        }

        if(irq_state & CHG_FCP_IRQ) {
                hi6526_read_block(CHG_FCP_IRQ_ADDR1, (u8*) &fcp_irq_state1, 2);
                hi6526_read_block(CHG_FCP_IRQ_ADDR2, (u8*) &fcp_irq_state2, 2);
                hi6526_read(CHG_FCP_IRQ_ADDR3, &fcp_irq_state3);

                SCHARGER_INF("%s : CHG_FCP_IRQ, irq_state: 0x%x, fcp_irq_state1: 0x%x,  fcp_irq_state2: 0x%x, fcp_irq_state3: 0x%x \n",
                    __func__, irq_state, fcp_irq_state1, fcp_irq_state2, fcp_irq_state3);

                fcp_irq_state1 &= OTHERS_IRQ_MASK1;
                fcp_irq_state2 &= OTHERS_IRQ_MASK2;
                fcp_irq_state3 &= OTHERS_IRQ_MASK3;
                hi6526_write_block(CHG_FCP_IRQ_ADDR1, (u8*) &fcp_irq_state1, 2);
                hi6526_write_block(CHG_FCP_IRQ_ADDR2, (u8*) &fcp_irq_state2, 2);
                hi6526_write(CHG_FCP_IRQ_ADDR3, fcp_irq_state3);
        }

        hi6526_unmask_all_irq();
        enable_irq(di->irq_int);
}

/**********************************************************
*  Function:       hi6526_interrupt
*  Description:    callback function for chargerIC fault irq in charging process
*  Parameters:   irq:chargerIC fault interrupt
*                      _di:hi6526_device_info
*  return value:  IRQ_HANDLED-success or others
**********************************************************/
static irqreturn_t hi6526_interrupt(int irq, void *_di)
{
        struct hi6526_device_info *di = _di;
        disable_irq_nosync(di->irq_int);
        queue_work(system_power_efficient_wq, &di->irq_work);
        return IRQ_HANDLED;
}

/**********************************************************
*  Function:       parse_dts
*  Description:    parse_dts
*  Parameters:   device_node:hi6526 device_node
*                      _di:hi6526_device_info
*  return value:  NULL
**********************************************************/
static void parse_dts(struct device_node *np, struct hi6526_device_info *di)
{
        int ret = 0;
        struct device_node *batt_node;
        di->param_dts.bat_comp = 80;
        di->param_dts.vclamp = 224;
        di->is_board_type = BAT_BOARD_ASIC;

        ret = of_property_read_u32(of_find_compatible_node(NULL, NULL, "hisi,coul_core"),
                            "r_coul_mohm", (u32 *)&(di->param_dts.r_coul_mohm));
        if (ret) {
                di->param_dts.r_coul_mohm = R_MOHM_DEFULT;
                SCHARGER_INF("get r_coul_mohm fail, use default value 2 mohm!\n");
        }
        SCHARGER_INF("prase_dts r_coul_mohm = %d\n", di->param_dts.r_coul_mohm);

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

        ret = of_property_read_u32(np, "fcp_support", (u32 *)&(di->param_dts.fcp_support));
        if (ret) {
                SCHARGER_ERR("get fcp_support failed\n");
                return;
        }
        SCHARGER_INF("prase_dts fcp_support = %d\n", di->param_dts.fcp_support);

        ret = of_property_read_u32(np, "scp_support", (u32 *)&(di->param_dts.scp_support));
        if (ret) {
                SCHARGER_ERR("get scp_support failed\n");
                return;
        }
        SCHARGER_INF("prase_dts scp_support = %d\n", di->param_dts.scp_support);
        batt_node = of_find_compatible_node(NULL, NULL, "huawei,hisi_bci_battery");
        if (batt_node) {
                if (of_property_read_u32(batt_node, "battery_board_type", &di->is_board_type)) {
                        SCHARGER_ERR("get battery_board_type fail!\n");
                        di->is_board_type = BAT_BOARD_ASIC;
                }
        } else {
                SCHARGER_ERR("get hisi_bci_battery fail!\n");
                di->is_board_type = BAT_BOARD_ASIC;
        }

        ret = of_property_read_u32(np, "dp_th_res", (u32 *)&(di->param_dts.dp_th_res));
        if (ret) {
                SCHARGER_ERR("get dp_th_res failed\n");
        }
        SCHARGER_INF("prase_dts dp_th_res = %d\n", di->param_dts.dp_th_res);

        return;
}/*lint !e64*/

static void hi6526_fcp_scp_ops_register(void)
{
        int ret = 0;
        /* if support fcp ,register fcp adapter ops */
        if (0 == hi6526_is_support_fcp()) {
                ret = fcp_adapter_ops_register(&fcp_hi6526_ops);
                if (ret)
                        SCHARGER_ERR("register fcp adapter ops failed!\n");
                else
                        SCHARGER_INF(" fcp adapter ops register success!\n");
        }
        /* if chip support scp ,register scp adapter ops */
        if( 0 == hi6526_is_support_scp())
        {
                #ifdef CONFIG_DIRECT_CHARGER
                ret = scp_ops_register(&scp_hi6526_ops);
                ret |= loadswitch_ops_register(&hi6526_lvc_ops);
                ret |= sc_ops_register(&hi6526_sc_ops);
                ret |= batinfo_sc_ops_register(&hi6526_batinfo_ops);
                ret |= batinfo_lvc_ops_register(&hi6526_batinfo_ops);
                #else
                ret = dummy_ops_register1(&scp_hi6526_ops);
                ret |= dummy_ops_register2(&hi6526_lvc_ops);
                ret |= dummy_ops_register2(&hi6526_sc_ops);
                ret |= dummy_ops_register3(&hi6526_batinfo_ops);
                #endif
                #ifdef CONFIG_WIRED_CHANNEL_SWITCH
                ret |= wired_chsw_ops_register(&hi6526_chsw_ops);
                #endif
                if (ret)
                {
                        SCHARGER_ERR("register scp adapter ops failed!\n");
                }
                else
                {
                        SCHARGER_INF(" scp adapter ops register success!\n");
                }
        }
}

/**********************************************************
*  Function:       hi6526_plugout_check_process
*  Description:    schedule or cancel check work based on charger type
*                  USB/NON_STD/BC_USB: schedule work
*                  REMOVED: cancel work
*  Parameters:     type: charger type
*  return value:   NULL
**********************************************************/
static void hi6526_plugout_check_process(enum hisi_charger_type type)
{
        struct hi6526_device_info *di = g_hi6526_dev;

        if (NULL == di)
        	return;
        switch (type) {
        case CHARGER_TYPE_SDP:
        case CHARGER_TYPE_DCP:
        case CHARGER_TYPE_CDP:
        case CHARGER_TYPE_UNKNOWN:
                hi6526_write_mask(CHG_IRQ_MASK_0, CHG_IRQ_VBUS_UVP_MSK,
                                        CHG_IRQ_VBUS_UVP_SHIFT, IRQ_VBUS_UVP_UNMASK);
        break;

        case CHARGER_TYPE_NONE:
                di->reverbst_cnt = 0;
                di->reverbst_begin = 0;
                hi6526_write_mask(CHG_IRQ_MASK_0, CHG_IRQ_VBUS_UVP_MSK,
                                CHG_IRQ_VBUS_UVP_SHIFT, IRQ_VBUS_UVP_MASK);
                break;
        default:
                break;
        }
}

/**********************************************************
*  Function:       hi6526_usb_notifier_call
*  Description:    respond the charger_type events from USB PHY
*  Parameters:   usb_nb:usb notifier_block
*                      event:charger type event name
*                      data:unused
*  return value:  NOTIFY_OK-success or others
**********************************************************/
static int hi6526_usb_notifier_call(struct notifier_block *usb_nb,
				    unsigned long event, void *data)
{
        struct hi6526_device_info *di = g_hi6526_dev;

        if(NULL == di){
                SCHARGER_ERR("%s : di is NULL!\n",__func__);
                return NOTIFY_OK;
        }

        di->charger_type = (enum hisi_charger_type)event;

        SCHARGER_INF("%s : di->charger_type %d\n",__func__, di->charger_type);
        hi6526_plugout_check_process(di->charger_type);
        return NOTIFY_OK;
}
/**********************************************************
*  Function:       hi6526_reset_watchdog_timer
*  Description:    reset watchdog timer in charging process
*  Parameters:   NULL
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_reset_watchdog_timer(void)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        int ibus = 0;
        static int ibus_abnormal_cnt = 0;

        if (NULL == di || (SC != di->chg_mode && LVC != di->chg_mode)) {
                ibus_abnormal_cnt = 0;
                /* kick watchdog */
                hi6526_write_mask(WATCHDOG_SOFT_RST_REG, WD_RST_N_MSK,
                                             WATCHDOG_TIMER_SHIFT, WATCHDOG_TIMER_RST);
                return 0;
        }

        ibus = hi6526_get_ibus_ma();

        if(di->ucp_work_first_run && ibus > IBUS_OCP_START_VAL) {
                di->ucp_work_first_run = 0;
        }

        if (ibus < IBUS_ABNORMAL_VAL && !di->ucp_work_first_run)
                ibus_abnormal_cnt++;
        else
                ibus_abnormal_cnt = 0;

        if (ibus_abnormal_cnt >= IBUS_ABNORMAL_CNT) {

                SCHARGER_INF("%s : cnt %d, ibus %d, chg_mode %d\n",__func__, ibus_abnormal_cnt, ibus, di->chg_mode);
                ibus_abnormal_cnt = 0;
                di->dc_ibus_ucp_happened = 1;
                hi6526_direct_charge_fault_handle(IRQ_IBUS_DC_UCP_MASK);
                hi6526_lvc_enable(0);
                hi6526_sc_enable(0);
                return 0;
        }

        return hi6526_write_mask(WATCHDOG_SOFT_RST_REG, WD_RST_N_MSK,
                             WATCHDOG_TIMER_SHIFT, WATCHDOG_TIMER_RST);
}

static void hi6526_dc_ucp_delay_work(struct work_struct *work)
{
        struct hi6526_device_info *di = g_hi6526_dev;
        int ibus = 0;
        static int ibus_abnormal_cnt = 0;

        if (NULL == di || (SC != di->chg_mode && LVC != di->chg_mode)) {
        	ibus_abnormal_cnt = 0;
                return;
        }

        ibus = hi6526_get_ibus_ma();

        if(di->ucp_work_first_run && ibus > IBUS_OCP_START_VAL) {
                di->ucp_work_first_run = 0;
        }

        if (ibus < IBUS_ABNORMAL_VAL && !di->ucp_work_first_run)
        	ibus_abnormal_cnt++;
        else
        	ibus_abnormal_cnt = 0;

        if (ibus_abnormal_cnt >= IBUS_ABNORMAL_CNT) {

                SCHARGER_INF("%s : cnt %d, ibus %d, chg_mode %d\n",__func__, ibus_abnormal_cnt, ibus, di->chg_mode);
                ibus_abnormal_cnt = 0;
                di->dc_ibus_ucp_happened = 1;
                hi6526_direct_charge_fault_handle(IRQ_IBUS_DC_UCP_MASK);
                hi6526_lvc_enable(0);
                hi6526_sc_enable(0);
                return;
        }

        di->dc_ibus_ucp_happened = 0;
        queue_delayed_work(system_power_efficient_wq, &di->dc_ucp_work,
        		      msecs_to_jiffies(IBUS_ABNORMAL_TIME));
}

static int hi6526_lock_mutex_init(struct hi6526_device_info *di)
{
        if (NULL == di)
                return -ENOMEM;

        mutex_init(&di->i2c_lock);
        mutex_init(&di->fcp_detect_lock);
        mutex_init(&di->adc_conv_lock);
        mutex_init(&di->accp_adapter_reg_lock);
        mutex_init(&di->ibias_calc_lock);
        return 0;
}

static void hi6526_irq_clear(void)
{
        int i = 0;
        u8 irq_state = 0;
        u8 val = 0;

        hi6526_read(CHG_IRQ_ADDR, &irq_state);
        for(i = 0; i< 8; i++){
                hi6526_read(CHG_BUCK_IRQ_ADDR + i, &val);
                if(val) {
                        SCHARGER_ERR("[%s]:irq_state = 0x%x, irq[%d] = 0x%x\n", __func__, irq_state, i, val);
                        hi6526_write(CHG_BUCK_IRQ_ADDR + i, val);
                }
        }

}

static int hi6526_irq_init(struct hi6526_device_info *di, struct device_node *np)
{
        int ret = 0;
        if (NULL == di)
                return -ENOMEM;

        hi6526_mask_all_irq();
        hi6526_irq_clear();

        di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
        if (!gpio_is_valid(di->gpio_int)) {
                SCHARGER_ERR(" %s, gpio_int is not valid\n", __func__);
                ret = -EINVAL;
                goto fail_0;
        }

        ret = gpio_request(di->gpio_int, "charger_int");
        if (ret) {
                SCHARGER_ERR(" %s, could not request gpio_int\n", __func__);
                goto fail_0;
        }
        ret = gpio_direction_input(di->gpio_int);
        if (ret < 0) {
                SCHARGER_ERR(" %s, Could not set gpio direction.\n", __func__);
                goto fail_1;
        }
        di->irq_int = gpio_to_irq(di->gpio_int);
        if (di->irq_int < 0) {
                SCHARGER_ERR(" %s, could not map gpio_int to irq\n", __func__);
                goto fail_1;
        }
        ret = request_irq(di->irq_int, hi6526_interrupt,
                        IRQF_TRIGGER_FALLING, "charger_int_irq", di);
        if (ret) {
                SCHARGER_ERR(" %s, could not request irq_int\n", __func__);
                di->irq_int = -1;
                goto fail_2;
        }

        return ret;
fail_2:
        free_irq(di->irq_int, di);
fail_1:
        gpio_free(di->gpio_int);
fail_0:
        return ret;
}

/**********************************************************
*  Function:       hi6526_probe
*  Description:    HI6526 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_probe(struct i2c_client *client,/*lint !e64*/
                        const struct i2c_device_id *id)
{
        int ret = 0;
        struct hi6526_device_info *di = NULL;
        struct device_node *np = NULL;
        struct class *power_class = NULL;

        di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);/*lint !e64*/
        if (NULL == di) {
                SCHARGER_ERR("%s hi6526_device_info is NULL!\n", __func__);
                return -ENOMEM;
        }
        if(hi6526_lock_mutex_init(di)) {
                ret = -EINVAL;
                goto hi6526_fail_0;
        }
        di->dev = &client->dev;
        np = di->dev->of_node;
        di->client = client;
        i2c_set_clientdata(client, di);


        parse_dts(np, di);
        INIT_WORK(&di->irq_work, hi6526_irq_work);
        INIT_DELAYED_WORK(&di->reverbst_work, hi6526_reverbst_delay_work);
        INIT_DELAYED_WORK(&di->dc_ucp_work, hi6526_dc_ucp_delay_work);
        INIT_DELAYED_WORK(&di->dbg_work, hi6526_dbg_work);

        ret = hi6526_irq_init(di, np);
        if(ret) {
                SCHARGER_ERR("%s, hi6526_irq_init failed\n", __func__);
                goto hi6526_fail_1;
        }
        di->usb_nb.notifier_call = hi6526_usb_notifier_call;
        ret = hisi_charger_type_notifier_register(&di->usb_nb);
        if (ret < 0) {
                SCHARGER_ERR("hisi_charger_type_notifier_register failed\n");
                goto hi6526_fail_1;
        }

        g_hi6526_dev = di;

        ret = hi6526_device_check();
        if(CHARGE_IC_BAD == ret)
                goto hi6526_fail_1;
        di->hi6526_version = hi6526_get_device_version();

        di->term_vol_mv = hi6526_get_terminal_voltage();
        ret = charge_ops_register(&hi6526_ops);
        if (ret) {
                SCHARGER_ERR("register charge ops failed!\n");
                goto hi6526_fail_1;
        }
        water_detect_ops_register(&hi6526_water_detect_ops);
        hi6526_fcp_scp_ops_register();
        ret = hi6526_sysfs_create_group(di);
        if (ret) {
                SCHARGER_ERR("create sysfs entries failed!\n");
                goto hi6526_fail_2;
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
                                goto hi6526_fail_2;
                        }
                }
                ret =
                    sysfs_create_link(&charge_dev->kobj, &di->dev->kobj,
                                      "HI6526");
                if (ret)
                        SCHARGER_ERR("create link to HI6526 fail.\n");
        }

        hi6526_opt_regs_set(common_opt_regs, ARRAY_SIZE(common_opt_regs));
        hi6526_buck_opt_param();
        hi6526_unmask_all_irq();

        SCHARGER_INF("%s  success!\n", __func__);
        return 0;

hi6526_fail_2:
        hi6526_sysfs_remove_group(di);
hi6526_fail_1:
        free_irq(di->irq_int, di);
        gpio_free(di->gpio_int);
hi6526_fail_0:
        g_hi6526_dev = NULL;
        np = NULL;

        return ret;
}

/**********************************************************
*  Function:       hi6526_remove
*  Description:    HI6526 module remove
*  Parameters:   client:i2c_client
*  return value:  0-success or others-fail
**********************************************************/
static int hi6526_remove(struct i2c_client *client)
{
        struct hi6526_device_info *di = i2c_get_clientdata(client);
        if (NULL == di)
                return -1;

        hi6526_sysfs_remove_group(di);

        if (di->irq_int) {
                free_irq(di->irq_int, di);
        }
        if (di->gpio_int) {
                gpio_free(di->gpio_int);
        }
        g_hi6526_dev = NULL;
        return 0;
}

MODULE_DEVICE_TABLE(i2c, HI6526);
static struct of_device_id hi6526_of_match[] = {
        {
         .compatible = "huawei,hi6526_charger",
         .data = NULL,
         },
        {
        },
};

static const struct i2c_device_id hi6526_i2c_id[] =
    { {"hi6526_charger", 0}, {} };

static struct i2c_driver hi6526_driver = {
        .probe = hi6526_probe,
        .remove = hi6526_remove,
        .id_table = hi6526_i2c_id,
        .driver = {
                   .owner = THIS_MODULE,
                   .name = "hi6526_charger",
                   .of_match_table = of_match_ptr(hi6526_of_match),
                   },
};

/**********************************************************
*  Function:       hi6526_init
*  Description:    HI6526 module initialization
*  Parameters:   NULL
*  return value:  0-success or others-fail
**********************************************************/
static int __init hi6526_init(void)
{
        int ret = 0;

        ret = i2c_add_driver(&hi6526_driver);
        if (ret)
                SCHARGER_ERR("%s: i2c_add_driver error!!!\n", __func__);
        SCHARGER_ERR("%s: !\n", __func__);

        return ret;
}

/**********************************************************
*  Function:       hi6526_exit
*  Description:    HI6526 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit hi6526_exit(void)
{
        i2c_del_driver(&hi6526_driver);
}

module_init(hi6526_init);
module_exit(hi6526_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HI6526 charger module driver");
MODULE_AUTHOR("HW Inc");
