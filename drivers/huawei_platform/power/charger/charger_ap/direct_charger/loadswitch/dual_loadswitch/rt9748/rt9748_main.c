#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/notifier.h>
#include <linux/mutex.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include <linux/raid/pq.h>
#include <huawei_platform/power/direct_charger.h>
#include "../dual_loadswitch.h"
#include "rt9748.h"

#define HWLOG_TAG rt9748_main

HWLOG_REGIST();

static struct rt9748_device_info *g_rt9748_dev;
static int g_get_id_time = 0;
/**********************************************************
*  Function:       rt9748_write_block
*  Discription:    register write block interface
*  Parameters:   di:rt9748_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int rt9748_write_block(struct rt9748_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

	if (!di->chip_already_init)
	{
		hwlog_err("chip not init\n");
		return -EIO;
	}
	*value = reg;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = value;
	msg[0].len = num_bytes + 1;

	ret = i2c_transfer(di->client->adapter, msg, 1);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 1)
	{
		hwlog_err("i2c_write failed to transfer all messages\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	else
	{
		return 0;
	}
}

/**********************************************************
*  Function:       rt9748_read_block
*  Discription:    register read block interface
*  Parameters:   di:rt9748_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int rt9748_read_block(struct rt9748_device_info *di,u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[2];
	u8 buf = 0;
	int ret = 0;

	if (!di->chip_already_init)
	{
		hwlog_err("chip not init\n");
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

	ret = i2c_transfer(di->client->adapter, msg, 2);

	/* i2c_transfer returns number of messages transferred */
	if (ret != 2)
	{
		hwlog_err("i2c_write failed to transfer all messages\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	}
	else
	{
		return 0;
	}
}

/**********************************************************
*  Function:       rt9748_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int rt9748_write_byte(u8 reg, u8 value)
{
	struct rt9748_device_info *di = g_rt9748_dev;
	/* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
	u8 temp_buffer[2] = { 0 };

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return rt9748_write_block(di, temp_buffer, reg, 1);
}

/**********************************************************
*  Function:       rt9748_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int rt9748_read_byte(u8 reg, u8 *value)
{
	struct rt9748_device_info *di = g_rt9748_dev;
	return rt9748_read_block(di, value, reg, 1);
}

/*lint -save -e* */
static ssize_t rt9748_sysfs_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	return 0;
}

/**********************************************************
*  Function:       fan54151_sysfs_store
*  Discription:    set the value for all fan54151 device's node
*  Parameters:   dev:device
*                      attr:device_attribute
*                      buf:string of node value
*                      count:unused
*  return value:  0-sucess or others-fail
**********************************************************/
static ssize_t rt9748_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	return count;
}
/*lint -restore*/

/**********************************************************
*  Function:       fan54151_sysfs_create_group
*  Discription:    create the fan54151 device sysfs group
*  Parameters:   di:fan54151_device_info
*  return value:  0-sucess or others-fail
**********************************************************/
static int rt9748_sysfs_create_group(struct rt9748_device_info *di)
{
	return 0;
}

/**********************************************************
*  Function:       charge_sysfs_remove_group
*  Discription:    remove the fan54151 device sysfs group
*  Parameters:   di:fan54151_device_info
*  return value:  NULL
**********************************************************/
static void rt9748_sysfs_remove_group(struct rt9748_device_info *di)
{
}
/**********************************************************
*  Function:       rt9748_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
/*lint -save -e* */
static int rt9748_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = rt9748_read_byte(reg, &val);
	if (ret < 0)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = rt9748_write_byte(reg, val);

	return ret;
}
/*lint -restore*/

/**********************************************************
*  Function:       rt9748_read_mask
*  Discription:    register read mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int rt9748_read_mask(u8 reg, u8 MASK, u8 SHIFT, u8 *value)
{
	int ret = 0;
	u8 val = 0;

	ret = rt9748_read_byte(reg, &val);
	if (ret < 0)
		return ret;
	val &= MASK;
	val >>= SHIFT;
	*value = val;

	return 0;
}


/**********************************************************
*  Function:       rt9748_reg_init
*  Discription:
*  Parameters:
*  return value:   0-sucess or others-fail
**********************************************************/
static int rt9748_reg_init(void)
{
	int ret;
	u8 val;
	switch (g_rt9748_dev->device_id)
	{
		case loadswitch_rt9748:
			ret = rt9748_write_byte(RT9748_EVENT_1_MASK,RT9748_EVENT_1_MASK_INIT);
			ret |= rt9748_write_byte(RT9748_EVENT_2_MASK,RT9748_EVENT_2_MASK_INIT);
			ret |= rt9748_write_byte(RT9748_EVENT_1_EN,RT9748_EVENT_1_EN_INIT);
			ret |= rt9748_write_byte(RT9748_CONTROL,RT9748_CONTROL_INIT);
			ret |= rt9748_write_byte(RT9748_ADC_CTRL,RT9748_ADC_CTRL_INIT);
			ret |= rt9748_write_byte(RT9748_SAMPLE_EN,RT9748_SAMPLE_EN_INIT);
			ret |= rt9748_write_mask(RT9748_PROT_DLY_OCP,RT9748_REG_INIT_MASK,RT9748_REG_INIT_SHIFT,RT9748_PROT_DLY_OCP_INIT);
			if (ret)
			{
				hwlog_err("%s: rt9748_reg_init fail\n", __func__);
				return -1;
			}
			rt9748_write_byte(RT9748_REV_CURRENT_SELECT,RT9748_REV_CURRENT_SELECT_INIT);
			rt9748_read_byte(RT9748_REV_CURRENT_SELECT, &val);
			hwlog_info("%s: rt9748 reg0x26 = 0x%x\n", __func__,val);
			if (RT9748_REV_CURRENT_SELECT_INIT != val)
			{
				hwlog_err("%s: rt9748  write 0x26 fail\n", __func__);
			}
			break;
		case loadswitch_bq25870:
			ret = rt9748_write_byte(BQ25870_EVENT_1_MASK,BQ25870_EVENT_1_MASK_INIT);
			ret |= rt9748_write_byte(BQ25870_EVENT_2_MASK,BQ25870_EVENT_2_MASK_INIT);
			ret |= rt9748_write_byte(BQ25870_EVENT_1_EN,BQ25870_EVENT_1_EN_INIT);
			ret |= rt9748_write_byte(BQ25870_CONTROL,BQ25870_CONTROL_INIT);
			ret |= rt9748_write_byte(BQ25870_ADC_CTRL,BQ25870_ADC_CTRL_INIT);
			ret |= rt9748_write_byte(BQ25870_SAMPLE_EN,BQ25870_SAMPLE_EN_INIT);
			ret |= rt9748_write_mask(BQ25870_PROT_DLY_OCP,BQ25870_REG_INIT_MASK,BQ25870_REG_INIT_SHIFT,BQ25870_PROT_DLY_OCP_INIT);
			if (ret)
			{
				hwlog_err("%s: ba25870_reg_init fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return 0;
}

/**********************************************************
*  Function:       rt9748_adc _enable
*  Discription:    Enable/disable ADC
*  Parameters:     enable: 1   disable: 0
*  return value:   0-sucess or others-fail
**********************************************************/
static int rt9748_adc_enable(int enable)
{
	int ret;
	u8 value = enable ? 0x1 : 0x0;
	ret = rt9748_write_mask(RT9748_ADC_CTRL ,RT9748_ADC_EN_MASK, RT9748_ADC_EN_SHIFT, value);
	if (ret)
	{
		hwlog_err("%s: rt9748 ADC enable fail, val = %d\n", __func__, enable);
		return -1;
	}
	return 0;
}

/**********************************************************
*  Function:        rt9748_software_charge_enable
*  Discription:    Software bit for charge enable
*  Parameters:     enable: 1   disable: 0
*  return value:   0-sucess or others-fail
**********************************************************/
static int rt9748_charge_enable(int enable)
{
	u8 reg = 0;
	int ret;
	u8 value = enable ? 0x1 : 0x0;

	ret = rt9748_write_mask(RT9748_CONTROL ,RT9748_CHARGE_EN_MASK, RT9748_CHARGE_EN_SHIFT, value);
	if (ret)
	{
		hwlog_err("%s: write fail, val = %d\n", __func__, enable);
		return -1;
	}
	ret = rt9748_read_byte(RT9748_CONTROL, &reg);
	if (ret)
	{
		hwlog_err("%s: read fail\n", __func__);
		return -1;
	}
	hwlog_info("%s:control reg = 0x%x\n", __func__, reg);
	return 0;
}
/**********************************************************
*  Function:        rt9748_discharge
*  Discription:    rt9748 discharge
*  Parameters:     enable: 1   disable: 0
*  return value:   0-sucess or others-fail
**********************************************************/
static int rt9748_discharge(int enable)
{
	u8 reg = 0;
	int ret;
	u8 value = enable ? 0x1 : 0x0;

	ret = rt9748_write_mask(RT9748_EVENT_1_EN ,RT9748_PD_EN_MASK, RT9748_PD_EN_SHIFT, value);
	if (ret)
	{
		hwlog_err("%s: write fail, val = %d\n", __func__, enable);
		return -1;
	}
	ret = rt9748_read_byte(RT9748_EVENT_1_EN, &reg);
	if (ret)
	{
		hwlog_err("%s: read fail\n", __func__);
		return -1;
	}
	hwlog_info("%s:event1_en reg = 0x%x\n", __func__, reg);
	return 0;
}
/**********************************************************
*  Function:        rt9748_config_ocp_threshold
*  Discription:
*  Parameters:
*  return value:   0-sucess or others-fail
**********************************************************/


static int rt9748_config_ioc_ocp_threshold_ma(int ocp_threshold)
{
	u8 value;
	int ret = 0;
	switch (g_rt9748_dev->device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_IOC_OCP_MIN_0_MA  > ocp_threshold)
			{
				ocp_threshold = RT9748_IOC_OCP_MIN_0_MA;
			}
			if (RT9748_IOC_OCP_MAX_6500_MA < ocp_threshold)
			{
				ocp_threshold = RT9748_IOC_OCP_MAX_6500_MA;
			}
			value = (u8) ((ocp_threshold - RT9748_IOC_OCP_OFFSET_0_MA) / RT9748_IOC_OCP_STEP );
			hwlog_info("rt9748 ocp_threshold = %d, value = 0x%x\n", ocp_threshold, value);
			ret = rt9748_write_mask(RT9748_PROT_DLY_OCP , RT9748_IOC_OCP_MASK , RT9748_IOC_OCP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config ioc_ocp threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_IOC_OCP_MIN_0_MA  > ocp_threshold)
			{
				ocp_threshold = BQ25870_IOC_OCP_MIN_0_MA;
			}
			if (BQ25870_IOC_OCP_MAX_7500_MA < ocp_threshold)
			{
				ocp_threshold = BQ25870_IOC_OCP_MAX_7500_MA;
			}
			value = (u8) ((ocp_threshold - BQ25870_IOC_OCP_OFFSET_0_MA) / BQ25870_IOC_OCP_STEP );
			hwlog_info("bq25870 ocp_threshold = %d, value = 0x%x\n", ocp_threshold, value);
			ret = rt9748_write_mask(BQ25870_PROT_DLY_OCP , BQ25870_IOC_OCP_MASK , BQ25870_IOC_OCP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config ioc_ocp threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return 0;
}




static int rt9748_config_vbus_ovp_threshold_mv(int ovp_threshold)
{
	u8 value;
	int ret = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_VBUS_OVP_MIN_4200_MV  > ovp_threshold)
			{
				ovp_threshold = RT9748_VBUS_OVP_MIN_4200_MV ;
			}
			if (RT9748_VBUS_OVP_MAX_6500_MV < ovp_threshold)
			{
				ovp_threshold = RT9748_VBUS_OVP_MAX_6500_MV;
			}
			value = (u8) ((ovp_threshold - RT9748_VBUS_OVP_OFFSET_4200_MV)/RT9748_VBUS_OVP_STEP );
			hwlog_info("rt9748 ovp_threshold = %d, value = 0x%x\n", ovp_threshold, value);
			ret = rt9748_write_mask(RT9748_VBUS_OVP, RT9748_VBUS_OVP_MASK , RT9748_VBUS_OVP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vbus ovp threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_VBUS_OVP_MIN_4200_MV  > ovp_threshold)
			{
				ovp_threshold = BQ25870_VBUS_OVP_MIN_4200_MV;
			}
			if (BQ25870_VBUS_OVP_MAX_6510_MV < ovp_threshold)
			{
				ovp_threshold = BQ25870_VBUS_OVP_MAX_6510_MV;
			}
			value = (u8) ((ovp_threshold - BQ25870_VBUS_OVP_OFFSET_4200_MV)/BQ25870_VBUS_OVP_STEP );
			hwlog_info("bq25870 ovp_threshold = %d, value = 0x%x\n", ovp_threshold, value);
			ret = rt9748_write_mask(BQ25870_VBUS_OVP, BQ25870_VBUS_OVP_MASK , BQ25870_VBUS_OVP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vbus ovp threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return 0;
}



static int rt9748_config_vout_reg_threshold_mv(int vout_reg_threshold)
{
	u8 value;
	int ret = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_VOUT_REG_MIN_4200_MV  > vout_reg_threshold)
			{
				vout_reg_threshold = RT9748_VOUT_REG_MIN_4200_MV ;
			}
			if (RT9748_VOUT_REG_MAX_5000_MV < vout_reg_threshold)
			{
				vout_reg_threshold = RT9748_VOUT_REG_MAX_5000_MV;
			}
			value = (u8) ((vout_reg_threshold - RT9748_VOUT_REG_OFFSET_4200_MV)/RT9748_VOUT_REG_STEP );
			hwlog_info("rt9748 vout_reg_threshold = %d, value = 0x%x\n", vout_reg_threshold, value);
			ret = rt9748_write_mask(RT9748_VOUT_REG, RT9748_VOUT_REG_MASK , RT9748_VOUT_REG_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vout reg threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_VOUT_REG_MIN_4200_MV  > vout_reg_threshold)
			{
				vout_reg_threshold = BQ25870_VOUT_REG_MIN_4200_MV ;
			}
			if (BQ25870_VOUT_REG_MAX_4975_MV < vout_reg_threshold)
			{
				vout_reg_threshold = BQ25870_VOUT_REG_MAX_4975_MV;
			}
			value = (u8) ((vout_reg_threshold - BQ25870_VOUT_REG_OFFSET_4200_MV)/BQ25870_VOUT_REG_STEP );
			hwlog_info("bq25870 vout_reg_threshold = %d, value = 0x%x\n", vout_reg_threshold, value);
			ret = rt9748_write_mask(BQ25870_VOUT_REG, BQ25870_VOUT_REG_MASK , BQ25870_VOUT_REG_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vout reg threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return 0;
}



static int rt9748_config_vdrop_ovp_reg_threshold_mv(int vdrop_ovp_threshold)
{
	u8 value;
	int ret = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_VDROP_OVP_MIN_0_MV  > vdrop_ovp_threshold)
			{
				vdrop_ovp_threshold = RT9748_VDROP_OVP_MIN_0_MV ;
			}
			if (RT9748_VDROP_OVP_MAX_1000_MV < vdrop_ovp_threshold)
			{
				vdrop_ovp_threshold = RT9748_VDROP_OVP_MAX_1000_MV;
			}
			value = (u8) ((vdrop_ovp_threshold - RT9748_VDROP_OVP_OFFSET_0_MV)/RT9748_VDROP_OVP_STEP );
			hwlog_info("vdrop_ovp_threshold = %d, value = 0x%x\n", vdrop_ovp_threshold, value);
			ret = rt9748_write_mask(RT9748_VDROP_OVP, RT9748_VDROP_OVP_MASK , RT9748_VDROP_OVP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vdrop ovp threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_VDROP_OVP_MIN_0_MV  > vdrop_ovp_threshold)
			{
				vdrop_ovp_threshold = BQ25870_VDROP_OVP_MIN_0_MV ;
			}
			if (BQ25870_VDROP_OVP_MAX_1000_MV < vdrop_ovp_threshold)
			{
				vdrop_ovp_threshold = BQ25870_VDROP_OVP_MAX_1000_MV;
			}
			value = (u8) ((vdrop_ovp_threshold - BQ25870_VDROP_OVP_OFFSET_0_MV)/BQ25870_VDROP_OVP_STEP );
			hwlog_info("vdrop_ovp_threshold = %d, value = 0x%x\n", vdrop_ovp_threshold, value);
			ret = rt9748_write_mask(BQ25870_VDROP_OVP, BQ25870_VDROP_OVP_MASK , BQ25870_VDROP_OVP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vdrop ovp threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return 0;
}


static int rt9748_config_vdrop_alm_reg_threshold_mv(int vdrop_alm_threshold)
{
	u8 value;
	int ret = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_VDROP_ALM_MIN_0_MV  > vdrop_alm_threshold)
			{
				vdrop_alm_threshold = RT9748_VDROP_ALM_MIN_0_MV ;
			}
			if (RT9748_VDROP_ALM_MAX_1000_MV < vdrop_alm_threshold)
			{
				vdrop_alm_threshold = RT9748_VDROP_ALM_MAX_1000_MV;
			}
			value = (u8) ((vdrop_alm_threshold - RT9748_VDROP_ALM_OFFSET_0_MV)/RT9748_VDROP_ALM_STEP );
			hwlog_info("vdrop_alm_threshold = %d, value = 0x%x\n", vdrop_alm_threshold, value);
			ret = rt9748_write_mask(RT9748_VDROP_ALM, RT9748_VDROP_ALM_MASK , RT9748_VDROP_ALM_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vdrop alm threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_VDROP_ALM_MIN_0_MV  > vdrop_alm_threshold)
			{
				vdrop_alm_threshold = BQ25870_VDROP_ALM_MIN_0_MV ;
			}
			if (BQ25870_VDROP_ALM_MAX_1000_MV < vdrop_alm_threshold)
			{
				vdrop_alm_threshold = BQ25870_VDROP_ALM_MAX_1000_MV;
			}
			value = (u8) ((vdrop_alm_threshold - BQ25870_VDROP_ALM_OFFSET_0_MV)/BQ25870_VDROP_ALM_STEP );
			hwlog_info("vdrop_alm_threshold = %d, value = 0x%x\n", vdrop_alm_threshold, value);
			ret = rt9748_write_mask(BQ25870_VDROP_ALM, BQ25870_VDROP_ALM_MASK , BQ25870_VDROP_ALM_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vdrop alm threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return 0;
}


static int rt9748_config_vbat_reg_threshold_mv(int vbat_reg_threshold)
{
	u8 value;
	int ret = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_VBAT_REG_MIN_4200_MV  > vbat_reg_threshold)
			{
				vbat_reg_threshold = RT9748_VBAT_REG_MIN_4200_MV ;
			}
			if (RT9748_VBAT_REG_MAX_5000_MV < vbat_reg_threshold)
			{
				vbat_reg_threshold = RT9748_VBAT_REG_MAX_5000_MV;
			}
			value = (u8) ((vbat_reg_threshold - RT9748_VBAT_REG_OFFSET_4200_MV)/RT9748_VBAT_REG_STEP );
			hwlog_info("vbat_reg_threshold = %d, value = 0x%x\n", vbat_reg_threshold, value);
			ret = rt9748_write_mask(RT9748_VBAT_REG, RT9748_VBAT_REG_MASK , RT9748_VBAT_REG_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vbat reg threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_VBAT_REG_MIN_4200_MV  > vbat_reg_threshold)
			{
				vbat_reg_threshold = BQ25870_VBAT_REG_MIN_4200_MV ;
			}
			if (BQ25870_VBAT_REG_MAX_4975_MV < vbat_reg_threshold)
			{
				vbat_reg_threshold = BQ25870_VBAT_REG_MAX_4975_MV;
			}
			value = (u8) ((vbat_reg_threshold - BQ25870_VBAT_REG_OFFSET_4200_MV)/BQ25870_VBAT_REG_STEP );
			hwlog_info("vbat_reg_threshold = %d, value = 0x%x\n", vbat_reg_threshold, value);
			ret = rt9748_write_mask(BQ25870_VBAT_REG, BQ25870_VBAT_REG_MASK , BQ25870_VBAT_REG_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config vbat reg threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return ret;
}


static int rt9748_config_ibat_ocp_threshold_ma(int ocp_threshold)
{
	u8 value;
	int ret = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_IBAT_OCP_MIN_400_MA  > ocp_threshold)
			{
				ocp_threshold = RT9748_IBAT_OCP_MIN_400_MA;
			}
			if (RT9748_IBAT_OCP_MAX_6350_MA < ocp_threshold)
			{
				ocp_threshold = RT9748_IBAT_OCP_MAX_6350_MA;
			}
			value = (u8) ((ocp_threshold - RT9748_IBAT_OCP_OFFSET_0_MA)/RT9748_IBAT_OCP_STEP );
			hwlog_info("ocp_threshold = %d, value = 0x%x\n", ocp_threshold, value);
			ret = rt9748_write_mask(RT9748_IBAT_OCP , RT9748_IBAT_OCP_MASK , RT9748_IBAT_OCP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config ibat_ocp threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_IBAT_OCP_MIN_0_MA  > ocp_threshold)
			{
				ocp_threshold = BQ25870_IBAT_OCP_MIN_0_MA;
			}
			if (BQ25870_IBAT_OCP_MAX_6350_MA < ocp_threshold)
			{
				ocp_threshold = BQ25870_IBAT_OCP_MAX_6350_MA;
			}
			value = (u8) ((ocp_threshold - BQ25870_IBAT_OCP_OFFSET_0_MA)/BQ25870_IBAT_OCP_STEP );
			hwlog_info("ocp_threshold = %d, value = 0x%x\n", ocp_threshold, value);
			ret = rt9748_write_mask(BQ25870_IBAT_OCP , BQ25870_IBAT_OCP_MASK , BQ25870_IBAT_OCP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config ibat_ocp threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return ret;
}

static int rt9748_config_ibus_ocp_threshold_ma(int ocp_threshold)
{
	u8 value;
	int ret = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			if (RT9748_IBUS_OCP_MIN_400_MA  > ocp_threshold)
			{
				ocp_threshold = RT9748_IBUS_OCP_MIN_400_MA;
			}
			if (RT9748_IBUS_OCP_MAX_6350_MA < ocp_threshold)
			{
				ocp_threshold = RT9748_IBUS_OCP_MAX_6350_MA;
			}
			value = (u8) ((ocp_threshold - RT9748_IBUS_OCP_OFFSET_0_MA)/RT9748_IBUS_OCP_STEP );
			hwlog_info("ocp_threshold = %d, value = 0x%x\n", ocp_threshold, value);
			ret = rt9748_write_mask(RT9748_IBUS_OCP , RT9748_IBUS_OCP_MASK , RT9748_IBUS_OCP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config ibus_ocp threshold fail\n", __func__);
				return -1;
			}
			break;
		case loadswitch_bq25870:
			if (BQ25870_IBUS_OCP_MIN_0_MA  > ocp_threshold)
			{
				ocp_threshold = BQ25870_IBUS_OCP_MIN_0_MA;
			}
			if (BQ25870_IBUS_OCP_MAX_6300_MA < ocp_threshold)
			{
				ocp_threshold = BQ25870_IBUS_OCP_MAX_6300_MA;
			}
			value = (u8) ((ocp_threshold - BQ25870_IBUS_OCP_OFFSET_0_MA)/BQ25870_IBUS_OCP_STEP );
			hwlog_info("ocp_threshold = %d, value = 0x%x\n", ocp_threshold, value);
			ret = rt9748_write_mask(BQ25870_IBUS_OCP , BQ25870_IBUS_OCP_MASK , BQ25870_IBUS_OCP_SHIFT, value);
			if (ret)
			{
				hwlog_err("%s: config ibus_ocp threshold fail\n", __func__);
				return -1;
			}
			break;
		default:
			return -1;
	}
	return ret;
}

static int rt9748_get_vbus_voltage_mv(int *vbus)
{
	int ret = 0;
	u8 reg_high = 0;
	u8 reg_low = 0;
	int polarity = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			ret = rt9748_read_byte(RT9748_VBUS_ADC2, &reg_high);
			ret |= rt9748_read_byte(RT9748_VBUS_ADC1,&reg_low);
			if (ret)
			{
				hwlog_err("%s: vbus reg read fail\n", __func__);
				return -1;
			}
			polarity = (reg_high & RT9748_VBUS_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			*vbus = (reg_high & RT9748_VBUS_ADC_MASK) * RT9748_VBUS_HIGH_LSB;
			*vbus += reg_low * RT9748_VBUS_LOW_LSB;
			if (1 == polarity)
			*vbus *= -1;
			break;
		case loadswitch_bq25870:
			ret = rt9748_read_byte(BQ25870_VBUS_ADC2, &reg_high);
			ret |= rt9748_read_byte(BQ25870_VBUS_ADC1,&reg_low);
			if (ret)
			{
				hwlog_err("%s: vbus reg read fail\n", __func__);
				return -1;
			}
			polarity = (reg_high & BQ25870_VBUS_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			*vbus = (reg_high & BQ25870_VBUS_ADC_MASK) * BQ25870_VBUS_HIGH_LSB;
			*vbus += reg_low * BQ25870_VBUS_LOW_LSB;
			if (1 == polarity)
			*vbus *= -1;
			break;
		default:
			return -1;
	}
	return 0;
}
static int rt9748_get_bat_voltage_mv(void)
{
	int ret = 0;
	int polarity = 0;
	u8 reg_high = 0;
	u8 reg_low = 0;
	int vbat = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			ret = rt9748_read_byte(RT9748_VBAT_ADC2, &reg_high);
			ret |= rt9748_read_byte(RT9748_VBAT_ADC1,&reg_low);
			polarity= (reg_high & RT9748_VBAT_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			if (ret)
			{
				hwlog_err("%s: vbat reg read fail\n", __func__);
				return -1;
			}
			vbat = (reg_high & RT9748_VBAT_ADC_MASK) * RT9748_VBAT_HIGH_LSB;
			vbat += reg_low * RT9748_VBAT_LOW_LSB;
			if (1 == polarity)
			vbat *= -1;
			break;
		case loadswitch_bq25870:
			ret = rt9748_read_byte(BQ25870_VBAT_ADC2, &reg_high);
			ret |= rt9748_read_byte(BQ25870_VBAT_ADC1,&reg_low);
			polarity= (reg_high & BQ25870_VBAT_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			if (ret)
			{
				hwlog_err("%s: vbat reg read fail\n", __func__);
				return -1;
			}
			vbat = (reg_high & BQ25870_VBAT_ADC_MASK) * BQ25870_VBAT_HIGH_LSB;
			vbat += reg_low * BQ25870_VBAT_LOW_LSB;
			if (1 == polarity)
			vbat *= -1;
			break;
		default:
			return -1;
	}
	return vbat;
}



static int rt9748_get_bat_current_ma(int * ibat)
{
	int ret = 0;
	u8 reg_high = 0;
	u8 reg_low = 0;
	int polarity = 0;
	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			ret = rt9748_read_byte(RT9748_IBAT_ADC2, &reg_high);
			hwlog_info("%s: IBAT_ADC2 = 0x%x\n", __func__, reg_high);
			ret |= rt9748_read_byte(RT9748_IBAT_ADC1,&reg_low);
			hwlog_info("%s: IBAT_ADC1 = 0x%x\n", __func__, reg_low);
			if (ret)
			{
				hwlog_err("%s: ibat reg read fail\n", __func__);
				return -1;
			}
			polarity = (reg_high & RT9748_IBAT_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			*ibat = (reg_high & RT9748_IBAT_ADC_MASK) * RT9748_IBAT_HIGH_LSB;
			*ibat += reg_low * RT9748_IBAT_LOW_LSB;
			*ibat *= 2;     /*for dual loadswitch the sens R is 2.5*/
			if (1 == polarity)
				*ibat *= -1;
			break;
		case loadswitch_bq25870:
			ret = rt9748_read_byte(BQ25870_IBAT_ADC2, &reg_high);
			hwlog_info("%s: IBAT_ADC2 = 0x%x\n", __func__, reg_high);
			ret |= rt9748_read_byte(BQ25870_IBAT_ADC1,&reg_low);
			hwlog_info("%s: IBAT_ADC1 = 0x%x\n", __func__, reg_low);
			if (ret)
			{
				hwlog_err("%s: ibat reg read fail\n", __func__);
				return -1;
			}
			polarity = (reg_high &BQ25870_IBAT_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			*ibat = (reg_high & BQ25870_IBAT_ADC_MASK) *BQ25870_IBAT_HIGH_LSB;
			*ibat += reg_low * BQ25870_IBAT_LOW_LSB;
			*ibat *= 2;     /*for dual loadswitch the sens R is 2.5*/
			if (1 == polarity)
				*ibat *= -1;
			break;
		default:
			return -1;
	}
	return 0;
}

static int rt9748_get_ls_temp(int * temp)
{
	u8 reg;
	int ret;

	ret = rt9748_read_byte(RT9748_TDIE_ADC1, &reg);
	if (ret)
	{
		hwlog_err("%s: fail\n", __func__);
		return -1;
	}
	*temp = (int)reg;
	return 0;
}

static int rt9748_get_ls_ibus(int * ibus)
{
	u8 reg_high = 0;
	u8 reg_low = 0;
	int polarity = 0;
	int ret;

	switch(g_rt9748_dev -> device_id)
	{
		case loadswitch_rt9748:
			ret = rt9748_read_byte(RT9748_IBUS_ADC2, &reg_high);
			ret |= rt9748_read_byte(RT9748_IBUS_ADC1,&reg_low);
			hwlog_info("%s: IBUS_ADC2 = 0x%x\n", __func__, reg_high);
			hwlog_info("%s: IBUS_ADC1 = 0x%x\n", __func__, reg_low);
			if (ret)
			{
				hwlog_err("%s: ibus reg read fail\n", __func__);
				return -1;
			}
			polarity = (reg_high & RT9748_IBUS_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			*ibus = (reg_high & RT9748_IBUS_ADC_MASK) * RT9748_IBUS_HIGH_LSB;
			*ibus += reg_low * RT9748_IBUS_LOW_LSB;
			if (1 == polarity)
				*ibus *= -1;
			break;
		case loadswitch_bq25870:
			ret = rt9748_read_byte(BQ25870_IBUS_ADC2, &reg_high);
			ret |= rt9748_read_byte(BQ25870_IBUS_ADC1,&reg_low);
			hwlog_info("%s: IBUS_ADC2 = 0x%x\n", __func__, reg_high);
			hwlog_info("%s: IBUS_ADC1 = 0x%x\n", __func__, reg_low);
			if (ret)
			{
				hwlog_err("%s: ibus reg read fail\n", __func__);
				return -1;
			}
			polarity = (reg_high & BQ25870_IBUS_POLARITY_MASK) >> (LENTH_OF_BYTE - 1);
			*ibus = (reg_high & BQ25870_IBUS_ADC_MASK) * BQ25870_IBUS_HIGH_LSB;
			*ibus += reg_low * BQ25870_IBUS_LOW_LSB;
			if (1 == polarity)
				*ibus *= -1;
			break;
		default:
			return -1;
	}
	return 0;
}

static int loadswitch_get_device_id(void)
{
	u8 reg = 0;
	int ret = 0;
	int bit3;
	int dev_id = -1;
	struct rt9748_device_info *di = g_rt9748_dev;
	if (0 == g_get_id_time)
	{
		g_get_id_time = 1;
		ret = rt9748_read_byte(RT9748_ADC_CTRL, &reg);
		if (ret)
		{
			hwlog_err("%s: dev_id reg07 read fail\n", __func__);
			return -1;
		}
		hwlog_info("%s: reg07 = %x\n", __func__,reg);
		bit3 = (reg >> LS_GET_DEV_ID_SHIFT) & 1;
		if (1 == bit3)
		{
			dev_id = loadswitch_rt9748;
			hwlog_info("%s: dev_id = %d\n", __func__,dev_id);
			return dev_id;
		}
		ret = rt9748_read_byte(LOADSWITCH_DEV_INFO_REG0, &reg);
		if (ret)
		{
			hwlog_err("%s: dev_id reg00 read fail\n", __func__);
			return -1;
		}
		hwlog_info("%s: reg00 = %x\n", __func__,reg);
		switch(reg)
		{
			case 0:
				dev_id = loadswitch_rt9748;
				break;
			case 1:
				dev_id = loadswitch_bq25870;
				break;
			case 2:
				dev_id = loadswitch_fair_child;
				break;
			case 3:
				dev_id = loadswitch_nxp;
				break;
			default:
				dev_id = -1;
				hwlog_err("%s: ls get id ERR!\n", __func__);
				break;
		}
		hwlog_info("%s: dev_id = %d\n", __func__,dev_id);
		return dev_id;
	}
	hwlog_info("%s: dev_id = %d\n", __func__,di->device_id);
	return di->device_id;
}
static int chip_init(void)
{
	int ret = 0;
	struct rt9748_device_info *di = g_rt9748_dev;

	if (di->chip_already_init)
		return 0;
	ret = gpio_direction_output(di->gpio_en,1);
	if (ret)
	{
		hwlog_err("%s:gpio_en fail\n", __func__);
		return -1;
	}
	msleep(200);
	di->chip_already_init = 1;
	return 0;
}

/**********************************************************
*  Function:        rt9748_charge_status
*  Discription:     return the status of cur module
*  Parameters:    void
*  return value:   0-sucess or others-fail
**********************************************************/
static int rt9748_charge_status(void)
{
	struct rt9748_device_info *di = g_rt9748_dev;
	if (NULL == di)
	{
		hwlog_err("%s: di is NULL\n", __func__);
		return -1;
	}

	if (di->chip_already_init == 1)
		return 0;

	hwlog_err("%s = %d\n", __func__, di->chip_already_init);
	return -1;
}

static int rt9748_charge_init(void )
{
	int ret = 0;
	struct rt9748_device_info *di = g_rt9748_dev;

	ret = chip_init();
	di->device_id = loadswitch_get_device_id();
	if (-1 == di->device_id)
	{
		hwlog_err("%s: get ls dev id ERR!\n", __func__);
		return -1;
	}
	hwlog_info("%s:loadswitch device id is %d\n",__func__,di->device_id);
	ret |= rt9748_reg_init( );
	ret |= rt9748_config_ioc_ocp_threshold_ma(6000);
	ret |= rt9748_config_vbus_ovp_threshold_mv(5800);
	ret |= rt9748_config_vout_reg_threshold_mv(4740);
	ret |= rt9748_config_vdrop_alm_reg_threshold_mv(1000);
	ret |= rt9748_config_vdrop_ovp_reg_threshold_mv(1000);
	ret |= rt9748_config_vbat_reg_threshold_mv(4450);
	ret |= rt9748_config_ibat_ocp_threshold_ma(5800);
	ret |= rt9748_config_ibus_ocp_threshold_ma(5800);
	if(ret)
	{
		hwlog_err("%s: config  init parameters error\n", __func__);
		return -1;
	}

	return 0;
}
static int batinfo_init(void)
{
	int ret = 0;

	ret = chip_init();
	ret |= rt9748_adc_enable(1);
	if(ret)
	{
		hwlog_err("%s:rt9748_adc_enable\n", __func__);
		return -1;
	}
	return 0;
}

static int rt9748_charge_exit(void)
{
	int ret = 0;
	struct rt9748_device_info *di = g_rt9748_dev;

	if (NULL == di)
	{
		hwlog_err("%s: di is NULL\n", __func__);
		return -1;
	}
	di->chip_already_init = 0;
	ret = rt9748_charge_enable(0);
	if (ret)
	{
		hwlog_err("%s: close fail\n", __func__);
		/*here do not return, cause reset pin can also close the switch*/
	}
	/*pull down reset pin to reset fan54151*/
	ret = gpio_direction_output(di->gpio_en, 0);
	if (ret < 0)
	{
		hwlog_err("%s: reset pull down fail!\n", __func__);
		return -1;
	}
	msleep(10);
	return ret;
}
static int rt9748_batinfo_exit(void)
{
	return 0;
}



static int rt9748_is_ls_close(void)
{
	u8 reg = 0;
	int ret = 0;

	ret = rt9748_read_byte(RT9748_CONTROL, &reg);
	if (ret)
	{
		hwlog_err("%s: ibat reg read fail\n", __func__);
		return 1;
	}
	if (reg & RT9748_CHARGE_EN_MASK)
	{
		return 0;
	}
	return 1;
}
static struct loadswitch_ops  rt9748_sysinfo_ops ={
	.ls_init = rt9748_charge_init,
	.ls_exit= rt9748_charge_exit,
	.ls_enable = rt9748_charge_enable,
	.ls_discharge = rt9748_discharge,
	.is_ls_close = rt9748_is_ls_close,
	.get_ls_id = loadswitch_get_device_id,
	.ls_status = rt9748_charge_status,
};
static struct batinfo_ops rt9748_batinfo_ops = {
	.init = batinfo_init,
	.exit = rt9748_batinfo_exit,
	.get_bat_btb_voltage =  rt9748_get_bat_voltage_mv,
	.get_bat_package_voltage =  rt9748_get_bat_voltage_mv,
	.get_vbus_voltage =  rt9748_get_vbus_voltage_mv,
	.get_bat_current = rt9748_get_bat_current_ma,
	.get_ls_ibus = rt9748_get_ls_ibus,
	.get_ls_temp = rt9748_get_ls_temp,
};

/**********************************************************
*  Function:       rt9748_irq_work
*  Discription:    handler for loadswitch fault irq in charging process
*  Parameters:   work:loadswitch fault interrupt workqueue
*  return value:  NULL
**********************************************************/

/*lint -save -e* */
static void rt9748_irq_work(struct work_struct *work)
{
	struct rt9748_device_info *di = container_of(work, struct rt9748_device_info, irq_work);
	u8 event1;
	u8 event2;
	u8 status;
	struct nty_data * data = &(di->nty_data);
	struct atomic_notifier_head *direct_charge_fault_notifier_list;

	direct_charge_lvc_get_fault_notifier(&direct_charge_fault_notifier_list);

	rt9748_read_byte(RT9748_EVENT_1, &event1);
	rt9748_read_byte(RT9748_EVENT_2, &event2);
	rt9748_read_byte(RT9748_EVENT_STATUS, &status);
	hwlog_info("event1 = 0x%x\n", event1);
	hwlog_info("event2 = 0x%x\n", event2);
	hwlog_info("status = 0x%x\n", status);
	data->event1 = event1;
	data->event2 = event2;
	data->addr = di->client->addr;
	if (event1 & RT9748_VBUS_OVP_FLT)
	{
		hwlog_err("vbus ovp happened\n");
		atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_VBUS_OVP, data);
	}
	else if (event1 & RT9748_IBUS_REVERSE_OCP_FLT)
	{
		hwlog_err("ibus reverse ocp happened\n");
		atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_REVERSE_OCP, data);
	}
	else if (event2 & RT9748_OTP_FLT)
	{
		hwlog_err("otp happened\n");
		atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_OTP, data);
	}
	else if (event2 & RT9748_INPUT_OCP_FLT)
	{
		hwlog_err("input ocp happened\n");
		atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_INPUT_OCP, data);
	}
	else if (event2 & RT9748_VDROP_OVP_FLT)
	{
		hwlog_err("vdrop ovp happened\n");
		atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_VDROP_OVP, data);
	}
	else
	{
		/*do nothing*/
	}


	//*clear irq*/
	enable_irq(di->irq_int);
}
/*lint -restore*/

/**********************************************************
*  Function:       rt9748_interrupt
*  Discription:    callback function for loadswitch fault irq in charging process
*  Parameters:   irq:loadswitch fault interrupt
*                      _di:rt9748_device_info
*  return value:  IRQ_HANDLED-sucess or others
**********************************************************/

/*lint -save -e* */
static irqreturn_t rt9748_interrupt(int irq, void *_di)
{
	struct rt9748_device_info *di = _di;
	hwlog_info("rt9748 interrupt\n");
	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);
	return IRQ_HANDLED;
}

static void parse_dts(struct device_node *np, struct rt9748_device_info *di)
{
	return;
}
/*lint -restore*/

/**********************************************************
*  Function:       rt9748_probe
*  Discription:    rt9748 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int rt9748_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct rt9748_device_info *di = NULL;
	struct device_node *np = NULL;
	//struct class *power_class = NULL;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di)
	{
		hwlog_err("rt9748_device_info is NULL!\n");
		return -ENOMEM;
	}
	g_rt9748_dev = di;
	di->chip_already_init = 0;
	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	parse_dts(np, di);
	INIT_WORK(&di->irq_work, rt9748_irq_work);

	di->gpio_int = of_get_named_gpio(np, "loadswitch_int", 0);
	hwlog_err("loadswitch_int = %d\n", di->gpio_int);
	if (!gpio_is_valid(di->gpio_int))
	{
		hwlog_err("loadswitch_int is not valid\n");
		ret = -EINVAL;
		goto rt9748_fail_0;
	}
	di->gpio_en= of_get_named_gpio(np, "loadswitch_en", 0);
	hwlog_err("loadswitch_en = %d\n", di->gpio_en);
	if (!gpio_is_valid(di->gpio_en))
	{
		hwlog_err("loadswitch_en is not valid\n");
		ret = -EINVAL;
		goto rt9748_fail_0;
	}
	ret = gpio_request(di->gpio_int, "loadswitch_int");
	if (ret)
	{
		hwlog_err("can not request gpio_int\n");
		goto rt9748_fail_0;
	}
	ret = gpio_request(di->gpio_en, "loadswitch_en");
	if (ret)
	{
		hwlog_err("could not request gpio_en\n");
		ret = -ENOMEM;
		goto rt9748_fail_1;
	}
	gpio_direction_output(di->gpio_en,0);
	gpio_direction_input(di->gpio_int);
	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0)
	{
		hwlog_err("could not map gpio_int to irq\n");
		goto rt9748_fail_2;
	}
	ret = request_irq(di->irq_int, rt9748_interrupt, IRQF_TRIGGER_FALLING, "loadswitch_int_irq", di);
	if (ret)
	{
		hwlog_err("could not request irq_int\n");
		di->irq_int = -1;
		goto rt9748_fail_2;
	}

	ret = loadswitch_main_ops_register(&rt9748_sysinfo_ops);
	if (ret)
	{
		hwlog_err("register loadswitch ops failed!\n");
		goto rt9748_fail_3;
	}
	ret = batinfo_lvc_ops_register(&rt9748_batinfo_ops);
	if (ret)
	{
		hwlog_err("register ina231 ops failed!\n");
		goto rt9748_fail_3;
	}
	ret = rt9748_sysfs_create_group(di);
	if (ret)
	{
		hwlog_err("create sysfs entries failed!\n");
		goto rt9748_fail_3;
	}
	hwlog_info("rt9748 probe ok!\n");
	return 0;

rt9748_fail_3:
	free_irq(di->irq_int, di);
rt9748_fail_2:
	gpio_free(di->gpio_en);
rt9748_fail_1:
	gpio_free(di->gpio_int);
rt9748_fail_0:
	devm_kfree(&client->dev, di);
	g_rt9748_dev = NULL;
	np = NULL;

	return ret;
}
/*lint -restore*/

/**********************************************************
*  Function:       rt9748_remove
*  Discription:    rt9748 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int rt9748_remove(struct i2c_client *client)
{
	struct rt9748_device_info *di = i2c_get_clientdata(client);

	rt9748_sysfs_remove_group(di);

	/*reset rt9748*/
	gpio_set_value(di->gpio_en, 0);
	if (di->gpio_en)
	{
		gpio_free(di->gpio_en);
	}
	if (di->irq_int)
	{
		free_irq(di->irq_int, di);
	}
	if (di->gpio_int)
	{
		gpio_free(di->gpio_int);
	}
	return 0;
}
/*lint -restore*/

MODULE_DEVICE_TABLE(i2c, rt9748);
static struct of_device_id rt9748_of_match[] = {
	{
	 .compatible = "rt9748_main",
	 .data = NULL,
	 },
	{
	 },
};

static const struct i2c_device_id rt9748_i2c_id[] = {
	{"rt9748_main", 0}, {}
};

static struct i2c_driver rt9748_driver = {
	.probe = rt9748_probe,
	.remove = rt9748_remove,
	.id_table = rt9748_i2c_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "rt9748_main",
		   .of_match_table = of_match_ptr(rt9748_of_match),
		   },
};

/**********************************************************
*  Function:       rt9748_init
*  Discription:    rt9748 module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/

/*lint -save -e* */
static int __init rt9748_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&rt9748_driver);
	if (ret)
		hwlog_err("%s: i2c_add_driver error!!!\n", __func__);

	return ret;
}

/**********************************************************
*  Function:       rt9748_exit
*  Discription:    rt9748 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit rt9748_exit(void)
{
	i2c_del_driver(&rt9748_driver);
}
/*lint -restore*/

/*lint -save -e* */
module_init(rt9748_init);
module_exit(rt9748_exit);
/*lint -restore*/

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("rt9748 module driver");
MODULE_AUTHOR("HW Inc");
