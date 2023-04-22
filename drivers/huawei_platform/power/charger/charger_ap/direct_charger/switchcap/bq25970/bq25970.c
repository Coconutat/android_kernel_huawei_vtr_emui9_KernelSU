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
#include <linux/raid/pq.h>

#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/direct_charger.h>
#include "../switchcap.h"
#include "bq25970.h"
#include <huawei_platform/power/power_thermalzone.h>
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_direct_charger.h>
#endif

#define HWLOG_TAG bq25970
HWLOG_REGIST();

static struct bq25970_device_info *g_bq25970_dev;
static int g_get_id_time = 0;
static int tsbus_high_r_kohm = BQ2597X_RESISTORS_100KOHM;
static int tsbus_low_r_kohm = BQ2597X_RESISTORS_100KOHM;
static int switching_frequency = BQ2597X_SWITCHING_FREQ_550KHZ;
static int bq25970_init_finish_flag = BQ2597X_NOT_INIT;
static int bq25970_interrupt_notify_enable_flag = BQ2597X_DISABLE_INTERRUPT_NOTIFY;

#define MSG_LEN                      (2)

/**********************************************************
*  Function:       bq25970_write_block
*  Discription:    register write block interface
*  Parameters:   di:bq25970_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25970_write_block(struct bq25970_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

	if (NULL == di || NULL == value) {
		hwlog_err("error: di is null or value is null!\n");
		return -EIO;
	}

	if (!di->chip_already_init) {
		hwlog_err("error: chip not init!\n");
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
*  Function:       bq25970_read_block
*  Discription:    register read block interface
*  Parameters:   di:bq25970_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25970_read_block(struct bq25970_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[MSG_LEN];
	u8 buf = 0;
	int ret = 0;

	if (NULL == di || NULL == value) {
		hwlog_err("error: di is null or value is null!\n");
		return -EIO;
	}

	if (!di->chip_already_init) {
		hwlog_err("error: chip not init!\n");
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
*  Function:       bq25970_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25970_write_byte(u8 reg, u8 value)
{
	struct bq25970_device_info *di = g_bq25970_dev;
	u8 temp_buffer[MSG_LEN] = { 0 }; /* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return bq25970_write_block(di, temp_buffer, reg, 1);
}

/**********************************************************
*  Function:       bq25970_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25970_read_byte(u8 reg, u8 *value)
{
	struct bq25970_device_info *di = g_bq25970_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	return bq25970_read_block(di, value, reg, 1);
}

/**********************************************************
*  Function:       bq25970_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25970_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = bq25970_read_byte(reg, &val);
	if (ret < 0)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = bq25970_write_byte(reg, val);

	return ret;
}

static void bq25970_dump_register(void)
{
	u8 i = 0;
	int ret = 0;
	u8 val = 0;

	for (i = 0; i < BQ2597X_DEGLITCH_REG; ++i) {
		ret = bq25970_read_byte(i, &val);
		if (ret) {
			hwlog_err("error: dump_register read fail!\n");
		}
		hwlog_info("reg [%x]=0x%x\n", i, val);
	}
}

static int bq25970_reg_reset(void)
{
	int ret;
	u8 reg = 0;

	ret = bq25970_write_mask(BQ2597X_CONTROL_REG, BQ2597X_REG_RST_MASK, BQ2597X_REG_RST_SHIFT, BQ2597X_REG_RST_ENABLE);
	if (ret) {
		hwlog_err("error: reg_reset write fail!\n");
		return -1;
	}

	ret = bq25970_read_byte(BQ2597X_CONTROL_REG, &reg);
	if (ret) {
		hwlog_err("error: reg_reset read fail!\n");
		return -1;
	}

	hwlog_info("reg_reset [%x]=0x%x\n", BQ2597X_CONTROL_REG, reg);
	return 0;
}
/**********************************************************
*  Function:        bq25970_charge_enable
*  Discription:     Software bit for charge enable
*  Parameters:    enable: 1   disable: 0
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_charge_enable(int enable)
{
	int ret;
	u8 reg = 0;
	u8 value = enable ? 0x1 : 0x0;

	ret = bq25970_write_mask(BQ2597X_CHRG_CTL_REG, BQ2597X_CHARGE_EN_MASK, BQ2597X_CHARGE_EN_SHIFT, value);
	if (ret) {
		hwlog_err("error: charge_enable write fail!\n");
		return -1;
	}

	ret = bq25970_read_byte(BQ2597X_CHRG_CTL_REG, &reg);
	if (ret) {
		hwlog_err("error: charge_enable read fail!\n");
		return -1;
	}

	hwlog_info("charge_enable [%x]=0x%x\n", BQ2597X_CHRG_CTL_REG, reg);
	return 0;
}

/**********************************************************
*  Function:        bq25970_discharge
*  Discription:     bq25970 discharge
*  Parameters:    enable: 1   disable: 0
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_discharge(int enable)
{
	int ret;
	u8 reg = 0;
	u8 value = enable ? 0x1 : 0x0;

	ret = bq25970_write_mask(BQ2597X_CONTROL_REG, BQ2597X_VBUS_PD_EN_MASK, BQ2597X_VBUS_PD_EN_SHIFT, value);
	if (ret) {
		hwlog_err("error: discharge write fail!\n");
		return -1;
	}

	ret = bq25970_read_byte(BQ2597X_CONTROL_REG, &reg);
	if (ret) {
		hwlog_err("error: discharge read fail!\n");
		return -1;
	}

	hwlog_info("discharge [%x]=0x%x\n", BQ2597X_CONTROL_REG, reg);
	return 0;
}

/**********************************************************
*  Function:        bq25970_is_device_close
*  Discription:     whether bq25970 is close
*  Parameters:    void
*  return value:   0: not close ,1: close
**********************************************************/
static int bq25970_is_device_close(void)
{
	u8 reg = 0;
	int ret = 0;

	ret = bq25970_read_byte(BQ2597X_CHRG_CTL_REG, &reg);
	if (ret) {
		hwlog_err("error: is_device_close read fail!\n");
		return 1;
	}

	if (reg & BQ2597X_CHARGE_EN_MASK) {
		return 0;
	}

	return 1;
}

/**********************************************************
*  Function:        bq25970_get_device_id
*  Discription:     get divice id
*  Parameters:    void
*  return value:   divice_id
**********************************************************/
static int bq25970_get_device_id(void)
{
	u8 part_info = 0;
	int ret = 0;
	struct bq25970_device_info *di = g_bq25970_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	if (BQ2597X_USED == g_get_id_time) {
		return di->device_id;
	}
	else {
		g_get_id_time = BQ2597X_USED;
		ret = bq25970_read_byte(BQ2597X_PART_INFO_REG, &part_info);
		if (ret) {
			g_get_id_time = BQ2597X_NOT_USED;
			hwlog_err("error: get_device_id read fail!\n");
			return -1;
		}
		hwlog_info("get_device_id [%x]=0x%x\n", BQ2597X_PART_INFO_REG, part_info);

		part_info = part_info & BQ2597X_DEVICE_ID_MASK;
		switch (part_info) {
			case BQ2597X_DEVICE_ID_BQ25970:
				di->device_id = SWITCHCAP_TI_BQ25970;
			break;

			default:
				di->device_id = -1;
				hwlog_err("error: switchcap get dev_id fail!\n");
			break;
		}

		return di->device_id;
	}
}

/**********************************************************
*  Function:        bq25970_get_vbat_mv
*  Discription:     bq25970 get battery voltage (mv)
*  Parameters:    void
*  return value:   battery voltage
**********************************************************/
static int bq25970_get_vbat_mv(void)
{
	u8 reg_high = 0;
	u8 reg_low = 0;
	s16 voltage = 0;
	int ret = 0;
	int vbat = 0;

	ret = bq25970_read_byte(BQ2597X_VBAT_ADC1_REG, &reg_high);
	ret |= bq25970_read_byte(BQ2597X_VBAT_ADC0_REG,&reg_low);
	if (ret) {
		hwlog_err("error: get_vbat_mv read fail!\n");
		return -1;
	}
	hwlog_info("VBAT_ADC1=0x%x, VBAT_ADC0=0x%x\n", reg_high, reg_low);

	voltage = (reg_high << BQ2597X_LENTH_OF_BYTE) + reg_low;
	vbat = (int)(voltage);

	return vbat;
}

/**********************************************************
*  Function:        bq25970_get_ibat_ma
*  Discription:     bq25970 get battery current (ma)
*  Parameters:    ibat
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_get_ibat_ma(int *ibat)
{
	int ret = 0;
	u8 reg_high = 0;
	u8 reg_low = 0;
	s16 curr = 0;

	ret = bq25970_read_byte(BQ2597X_IBAT_ADC1_REG, &reg_high);
	ret |= bq25970_read_byte(BQ2597X_IBAT_ADC0_REG, &reg_low);
	if (ret) {
		hwlog_err("error: get_ibat_ma read fail!\n");
		return -1;
	}
	hwlog_info("IBAT_ADC1=0x%x, IBAT_ADC0=0x%x\n", reg_high, reg_low);

	curr = (reg_high << BQ2597X_LENTH_OF_BYTE) + reg_low;
	*ibat = (int)(curr);

	return 0;
}
/**********************************************************
*  Function:        bq25970_get_ibus_ma
*  Discription:     bq25970 get the bus current (ma)
*  Parameters:    ibus
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_get_ibus_ma(int * ibus)
{
	u8 reg_high = 0;
	u8 reg_low = 0;
	int ret;
	s16 curr;

	ret = bq25970_read_byte(BQ2597X_IBUS_ADC1_REG, &reg_high);
	ret |= bq25970_read_byte(BQ2597X_IBUS_ADC0_REG,&reg_low);
	if (ret) {
		hwlog_err("error: get_ibus_ma read fail!\n");
		return -1;
	}
	hwlog_info("IBUS_ADC1=0x%x, IBUS_ADC0=0x%x\n", reg_high, reg_low);

	curr = (reg_high << BQ2597X_LENTH_OF_BYTE) + reg_low;
	*ibus = (int)(curr);

	return 0;
}

/**********************************************************
*  Function:        bq25970_get_vbus_mv
*  Discription:     bq25970 get the bus voltage (mV)
*  Parameters:    vbus
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_get_vbus_mv(int *vbus)
{
	int ret = 0;
	u8 reg_high = 0;
	u8 reg_low = 0;
	s16 voltage = 0;

	ret = bq25970_read_byte(BQ2597X_VBUS_ADC1_REG, &reg_high);
	ret |= bq25970_read_byte(BQ2597X_VBUS_ADC0_REG,&reg_low);
	if (ret) {
		hwlog_err("error: get_vbus_mv read fail!\n");
		return -1;
	}
	hwlog_info("VBUS_ADC1=0x%x, VBUS_ADC0=0x%x\n", reg_high, reg_low);

	voltage = (reg_high << BQ2597X_LENTH_OF_BYTE) + reg_low;
	*vbus = (int)(voltage);

	return 0;
}

/**********************************************************
*  Function:        bq25970_get_vac_mv
*  Discription:     get ac voltage(mv)
*  Parameters:    vac
*  return value:   0-sucess or others-fail
**********************************************************/
#if 0
static int bq25970_get_vac_mv(int *vac)
{
	int ret = 0;
	u8 reg_high = 0;
	u8 reg_low = 0;
	s16 voltage;

	ret = bq25970_read_byte(BQ2597X_VAC_ADC1_REG, &reg_high);
	ret |= bq25970_read_byte(BQ2597X_VAC_ADC0_REG, &reg_low);
	if (ret) {
		hwlog_err("error: get_vac_mv read fail!\n");
		return -1;
	}
	hwlog_info("VAC_ADC1=0x%x, VAC_ADC0=0x%x\n", reg_high, reg_low);

	voltage = (reg_high << BQ2597X_LENTH_OF_BYTE) + reg_low;
	*vac = (int)(voltage);

	return 0;
}
#endif

/**********************************************************
*  Function:        bq25970_get_tsbus_percentage
*  Discription:     get tsbus pin voltage as a percentage of vout
*  Parameters:      percentage
*  return value:    0-sucess or others-fail
**********************************************************/
static int bq25970_get_tsbus_percentage(long *tsbus_per)
{
	int ret = 0;
	u8 reg_high = 0;
	u8 reg_low = 0;
	s16 adc_value = 0;

	ret = bq25970_read_byte(BQ2597X_TSBUS_ADC1_REG, &reg_high);
	ret |= bq25970_read_byte(BQ2597X_TSBUS_ADC0_REG, &reg_low);
	if (ret) {
		hwlog_err("error: get_tsbus_percentage read fail!\n");
		return -1;
	}
	hwlog_info("TSBUS_ADC1=0x%x, TSBUS_ADC0=0x%x\n", reg_high, reg_low);

	adc_value = ((reg_high & BQ2597X_TSBUS_ADC1_MASK) << BQ2597X_LENTH_OF_BYTE) + reg_low;
	*tsbus_per = (long)(adc_value * BQ2597X_TSBUS_ADC_STEP);

	return 0;
}

static int bq25970_get_tsbus_ntc_resistor(int adc_channel, long *data)
{
	int ret = 0;
	long tsbus_per = 0;
	long r_temp = 0;

	ret = bq25970_get_tsbus_percentage(&tsbus_per);
	if (ret) {
		hwlog_err("error: get_tsbus_ntc_resistor read fail!\n");
		return -1;
	}

	/*
		Rt = 1/((1 / Rntc) + (1 / Rlow))
		Vtsbus/Vout =  Rt/(Rhigh + Rt)
		r_temp = (tsbus_per * tsbus_high_r_kohm) / (BQ2597X_TSBUS_PER_MAX - tsbus_per);
		data = (r_temp * tsbus_low_r_kohm) / (tsbus_low_r_kohm - r_temp);
	*/
	r_temp = (BQ2597X_TSBUS_PER_MAX * tsbus_low_r_kohm) - tsbus_per * (tsbus_low_r_kohm + tsbus_high_r_kohm);
	if (r_temp <= 0) {
		hwlog_err("error: get tsbus ntc resistor failed!\n");
		return -1;
	}

	*data = (tsbus_high_r_kohm * tsbus_low_r_kohm * tsbus_per)/r_temp * BQ2597X_RESISTORS_KILO;

	return 0;
}

/**********************************************************
*  Function:        bq25970_is_tsbat_disabled
*  Discription:     get the status of bq25970. the value of BQ2597X_TSBAT_DIS_MASK bit show the status of bq25970. 1 means working , 0 means protected.
*  Parameters:    void
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_is_tsbat_disabled(void)
{
	u8 reg = 0;
	int ret = 0;

	ret = bq25970_read_byte(BQ2597X_CHRG_CTL_REG, &reg);
	if (ret) {
		hwlog_err("error: is_tsbat_disabled read fail!\n");
		return -1;
	}
	hwlog_info("is_tsbat_disabled [%x]=0x%x\n", BQ2597X_CHRG_CTL_REG, reg);

	if (reg & BQ2597X_TSBAT_DIS_MASK) {
		return 0;
	}

	return -1;
}

/**********************************************************
*  Function:        bq25970_get_device_temp
*  Discription:     get the temperature of switchcap
*  Parameters:    temperature
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_get_device_temp(int * temp)
{
	u8 reg_high = 0;
	u8 reg_low = 0;
	s16 temperature;
	int ret;

	ret = bq25970_read_byte(BQ2597X_TDIE_ADC1_REG, &reg_high);
	ret |= bq25970_read_byte(BQ2597X_TDIE_ADC0_REG, &reg_low);
	if (ret) {
		hwlog_err("error: get_device_temp read fail!\n");
		return -1;
	}
	hwlog_info("TDIE_ADC1=0x%x, TDIE_ADC0=0x%x\n", reg_high, reg_low);

	temperature = ((reg_high & BQ2597X_TDIE_ADC1_MASK) << BQ2597X_LENTH_OF_BYTE) + reg_low;
	/*bq25970 tdie adc is not working correctly , return 0*/
	//*temp = (int)(temperature / BQ2597X_TDIE_SCALE);
	*temp = 0;

	return 0;
}

/**********************************************************
*  Function:        bq25970_watchdog_config
*  Discription:     bq25970  watchdog time config
*  Parameters:    watchdog time
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_config_watchdog_ms(int time)
{
	u8 val;
	u8 reg;
	int ret;

	if (time >= BQ2597X_WATCHDOG_CONFIG_TIMING_30000MS) {
		val = 3;
	}
	else if (time >= BQ2597X_WATCHDOG_CONFIG_TIMING_5000MS) {
		val = 2;
	}
	else if (time >= BQ2597X_WATCHDOG_CONFIG_TIMING_1000MS) {
		val = 1;
	}
	else {
		val = 0;
	}

	ret = bq25970_write_mask(BQ2597X_CONTROL_REG, BQ2597X_WATCHDOG_CONFIG_MASK, BQ2597X_WATCHDOG_CONFIG_SHIFT, val);
	if (ret) {
		hwlog_err("error: config_watchdog_ms write fail!\n");
		return -1;
	}

	ret = bq25970_read_byte(BQ2597X_CONTROL_REG, &reg);
	if (ret) {
		hwlog_err("error: config_watchdog_ms read fail!\n");
		return -1;
	}

	hwlog_info("config_watchdog_ms [%x]=0x%x\n", BQ2597X_CONTROL_REG, reg);
	return 0;
}

/**********************************************************
*  Function:        bq25970_config_vbat_ovp_threshold_mv
*  Discription:     config the battery ovp value (mV)
*  Parameters:    ovp_threshold
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_config_vbat_ovp_threshold_mv(int ovp_threshold)
{
	u8 value;
	int ret = 0;

	if (BQ2597X_BAT_OVP_BASE_3500MV > ovp_threshold) {
		ovp_threshold = BQ2597X_BAT_OVP_BASE_3500MV;
	}

	if (BQ2597X_BAT_OVP_MAX_5075MV < ovp_threshold) {
		ovp_threshold = BQ2597X_BAT_OVP_MAX_5075MV;
	}

	value = (u8)((ovp_threshold - BQ2597X_BAT_OVP_BASE_3500MV) / BQ2597X_BAT_OVP_STEP);
	ret = bq25970_write_mask(BQ2597X_BAT_OVP_REG, BQ2597X_BAT_OVP_MASK, BQ2597X_BAT_OVP_SHIFT, value);
	if (ret) {
		hwlog_err("error: config_vbat_ovp_threshold_mv write fail!\n");
		return -1;
	}

	hwlog_info("config_vbat_ovp_threshold_mv [%x]=0x%x\n", BQ2597X_BAT_OVP_REG, value);
	return 0;
}

/**********************************************************
*  Function:        bq25970_config_ibat_ocp_threshold_ma
*  Discription:     set the battery ocp value (mA)
*  Parameters:    ocp_threshold
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_config_ibat_ocp_threshold_ma(int ocp_threshold)
{
	u8 value;
	int ret = 0;

	if (BQ2597X_BAT_OCP_BASE_2000MA > ocp_threshold) {
		ocp_threshold = BQ2597X_BAT_OCP_BASE_2000MA;
	}

	if (BQ2597X_BAT_OCP_MAX_14700MA < ocp_threshold) {
		ocp_threshold = BQ2597X_BAT_OCP_MAX_14700MA;
	}

	value = (u8)((ocp_threshold - BQ2597X_BAT_OCP_BASE_2000MA) / BQ2597X_BAT_OCP_STEP);
	ret = bq25970_write_mask(BQ2597X_BAT_OCP_REG, BQ2597X_BAT_OCP_MASK, BQ2597X_BAT_OCP_SHIFT, value);
	if (ret) {
		hwlog_err("error: config_ibat_ocp_threshold_ma write fail!\n");
		return -1;
	}

	hwlog_info("config_ibat_ocp_threshold_ma [%x]=0x%x\n", BQ2597X_BAT_OCP_REG, value);
	return 0;
}


/**********************************************************
*  Function:        bq25970_config_ac_ovp_threshold_mv
*  Discription:     set ac ovp value (mV)
*  Parameters:    ovp_threshold
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_config_ac_ovp_threshold_mv(int ovp_threshold)
{
	u8 value;
	int ret = 0;

	if (BQ2597X_AC_OVP_BASE_11000MV > ovp_threshold) {
		ovp_threshold = BQ2597X_AC_OVP_BASE_11000MV;
	}

	if (BQ2597X_AC_OVP_MAX_18000MV < ovp_threshold) {
		ovp_threshold = BQ2597X_AC_OVP_MAX_18000MV;
	}

	value = (u8)(ovp_threshold - BQ2597X_AC_OVP_BASE_11000MV) / BQ2597X_AC_OVP_STEP;
	ret = bq25970_write_mask(BQ2597X_AC_OVP_REG, BQ2597X_AC_OVP_MASK, BQ2597X_AC_OVP_SHIFT, value);
	if(ret)
	{
		hwlog_err("error: config_ac_ovp_threshold_mv write fail!\n");
		return -1;
	}

	hwlog_info("config_ac_ovp_threshold_mv [%x]=0x%x\n", BQ2597X_AC_OVP_REG, value);
	return 0;
}
/**********************************************************
*  Function:        bq25970_config_vbus_ovp_threshold_mv
*  Discription:     set the bus ovp value (mV)
*  Parameters:    ovp_threshold
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_config_vbus_ovp_threshold_mv(int ovp_threshold)
{
	u8 value;
	int ret = 0;

	if (BQ2597X_BUS_OVP_BASE_6000MV > ovp_threshold) {
		ovp_threshold = BQ2597X_BUS_OVP_BASE_6000MV;
	}

	if (BQ2597X_BUS_OVP_MAX_12350MV < ovp_threshold) {
		ovp_threshold = BQ2597X_BUS_OVP_MAX_12350MV;
	}

	value = (u8)((ovp_threshold - BQ2597X_BUS_OVP_BASE_6000MV) / BQ2597X_BUS_OVP_STEP);
	ret = bq25970_write_mask(BQ2597X_BUS_OVP_REG, BQ2597X_BUS_OVP_MASK, BQ2597X_BUS_OVP_SHIFT, value);
	if (ret) {
		hwlog_err("error: config_vbus_ovp_threshold_mv write fail!\n");
		return -1;
	}

	hwlog_info("config_vbus_ovp_threshold_mv [%x]=0x%x\n", BQ2597X_BUS_OVP_REG, value);
	return 0;
}

/**********************************************************
*  Function:        bq25970_config_ibus_ocp_threshold_ma
*  Discription:     set the bus ocp value (mA)
*  Parameters:    ocp threshold
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_config_ibus_ocp_threshold_ma(int ocp_threshold)
{
	u8 value;
	int ret = 0;

	if (BQ2597X_BUS_OCP_BASE_1000MA > ocp_threshold) {
		ocp_threshold = BQ2597X_BUS_OCP_BASE_1000MA;
	}

	if (BQ2597X_BUS_OCP_MAX_4750MA < ocp_threshold) {
		ocp_threshold = BQ2597X_BUS_OCP_MAX_4750MA;
	}

	value = (u8)((ocp_threshold - BQ2597X_BUS_OCP_BASE_1000MA) / BQ2597X_BUS_OCP_STEP);
	ret = bq25970_write_mask(BQ2597X_BUS_OCP_UCP_REG, BQ2597X_BUS_OCP_MASK, BQ2597X_BUS_OCP_SHIFT, value);
	if (ret) {
		hwlog_err("error: config_ibus_ocp_threshold_ma write fail!\n");
		return -1;
	}

	hwlog_info("config_ibus_ocp_threshold_ma [%x]=0x%x\n", BQ2597X_BUS_OCP_UCP_REG, value);
	return 0;
}


/**********************************************************
*  Function:        bq25970_config_switching_frequency
*  Discription:     set the switching frequency
*  Parameters:    freq
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_config_switching_frequency(int data)
{
	int freq = 0;
	int freq_shift = 0;
	int ret = 0;

	switch (data) {
		case BQ2597X_SWITCHING_FREQ_450KHZ:
			freq = BQ2597X_FSW_SET_SWITCHING_FREQ_500KHZ;
			freq_shift = BQ2597X_SWITCHING_FREQ_SHIFT_M_P10;
		break;

		case BQ2597X_SWITCHING_FREQ_500KHZ:
			freq = BQ2597X_FSW_SET_SWITCHING_FREQ_500KHZ;
			freq_shift = BQ2597X_SWITCHING_FREQ_SHIFT_NORMAL;
		break;

		case BQ2597X_SWITCHING_FREQ_550KHZ:
			freq = BQ2597X_FSW_SET_SWITCHING_FREQ_500KHZ;
			freq_shift = BQ2597X_SWITCHING_FREQ_SHIFT_P_P10;
		break;

		case BQ2597X_SWITCHING_FREQ_675KHZ:
			freq = BQ2597X_FSW_SET_SWITCHING_FREQ_750KHZ;
			freq_shift = BQ2597X_SWITCHING_FREQ_SHIFT_M_P10;
		break;

		case BQ2597X_SWITCHING_FREQ_750KHZ:
			freq = BQ2597X_FSW_SET_SWITCHING_FREQ_750KHZ;
			freq_shift = BQ2597X_SWITCHING_FREQ_SHIFT_NORMAL;
		break;

		case BQ2597X_SWITCHING_FREQ_825KHZ:
			freq = BQ2597X_FSW_SET_SWITCHING_FREQ_750KHZ;
			freq_shift = BQ2597X_SWITCHING_FREQ_SHIFT_P_P10;
		break;

		default:
			freq = BQ2597X_FSW_SET_SWITCHING_FREQ_500KHZ;
			freq_shift = BQ2597X_SWITCHING_FREQ_SHIFT_P_P10;
		break;
	}

	ret = bq25970_write_mask(BQ2597X_CONTROL_REG, BQ2597X_FSW_SET_MASK, BQ2597X_FSW_SET_SHIFT, freq);
	if (ret) {
		hwlog_err("error: config_switching_frequency write fail!\n");
		return -1;
	}

	ret = bq25970_write_mask(BQ2597X_CHRG_CTL_REG, BQ2597X_FREQ_SHIFT_MASK, BQ2597X_FREQ_SHIFT_SHIFT, freq_shift);
	if (ret) {
		hwlog_err("error: config_switching_frequency write fail!\n");
		return -1;
	}

	hwlog_info("config_switching_frequency [%x]=0x%x\n", BQ2597X_CONTROL_REG, freq);
	hwlog_info("config_switching_frequency [%x]=0x%x\n", BQ2597X_CHRG_CTL_REG, freq_shift);
	return 0;
}

static int bq25970_chip_init(void)
{
	return 0;
}

static int bq25970_reg_init(void)
{
	int ret = 0;

	ret = bq25970_write_byte(BQ2597X_CONTROL_REG, BQ2597X_CONTROL_REG_INIT);
	ret |= bq25970_write_byte(BQ2597X_CHRG_CTL_REG, BQ2597X_CHRG_CTL_REG_INIT);
	ret |= bq25970_write_byte(BQ2597X_INT_MASK_REG, BQ2597X_INT_MASK_REG_INIT);
	ret |= bq25970_write_byte(BQ2597X_FLT_MASK_REG, BQ2597X_FLT_MASK_REG_INIT);
	ret |= bq25970_write_byte(BQ2597X_ADC_CTRL_REG, BQ2597X_ADC_CTRL_REG_INIT);
	ret |= bq25970_write_byte(BQ2597X_ADC_FN_DIS_REG, BQ2597X_ADC_FN_DIS_REG_INIT);
	ret |= bq25970_write_mask(BQ2597X_BAT_OVP_ALM_REG, BQ2597X_BAT_OVP_ALM_DIS_MASK, BQ2597X_BAT_OVP_ALM_DIS_SHIFT, BQ2597X_ALM_DISABLE);
	ret |= bq25970_write_mask(BQ2597X_BAT_OCP_ALM_REG, BQ2597X_BAT_OCP_ALM_DIS_MASK, BQ2597X_BAT_OCP_ALM_DIS_SHIFT, BQ2597X_ALM_DISABLE);
	ret |= bq25970_write_mask(BQ2597X_BAT_UCP_ALM_REG, BQ2597X_BAT_UCP_ALM_DIS_MASK, BQ2597X_BAT_UCP_ALM_DIS_SHIFT, BQ2597X_ALM_DISABLE);
	ret |= bq25970_write_mask(BQ2597X_BUS_OVP_ALM_REG, BQ2597X_BUS_OVP_ALM_DIS_MASK, BQ2597X_BUS_OVP_ALM_DIS_SHIFT, BQ2597X_ALM_DISABLE);
	ret |= bq25970_write_mask(BQ2597X_BUS_OCP_ALM_REG, BQ2597X_BUS_OCP_ALM_DIS_MASK, BQ2597X_BUS_OCP_ALM_DIS_SHIFT, BQ2597X_ALM_DISABLE);
	ret |= bq25970_config_vbat_ovp_threshold_mv(BQ2597X_VBAT_OVP_THRESHOLD_INIT);
	ret |= bq25970_config_ibat_ocp_threshold_ma(BQ2597X_IBAT_OCP_THRESHOLD_INIT);
	ret |= bq25970_config_ac_ovp_threshold_mv(BQ2597X_AC_OVP_THRESHOLD_INIT);
	ret |= bq25970_config_vbus_ovp_threshold_mv(BQ2597X_VBUS_OVP_THRESHOLD_INIT);
	ret |= bq25970_config_ibus_ocp_threshold_ma(BQ2597X_IBUS_OCP_THRESHOLD_INIT);
	ret |= bq25970_config_switching_frequency(switching_frequency);

	if (ret) {
		hwlog_err("error: reg_init fail!\n");
		return -1;
	}

	return 0;
}
/**********************************************************
*  Function:        bq25970_charge_init
*  Discription:     bq25970 charging parameters initial
*  Parameters:    void
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_charge_init(void)
{
	struct bq25970_device_info *di = g_bq25970_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	if (bq25970_reg_init()) {
		return -1;
	}

	di->device_id = bq25970_get_device_id();
	if (-1 == di->device_id) {
		return -1;
	}

	hwlog_info("switchcap bq25970 device id is %d\n", di->device_id);

	bq25970_init_finish_flag = BQ2597X_INIT_FINISH;
	return 0;
}

/**********************************************************
*  Function:        bq25970_charge_exit
*  Discription:     bq25970 charging exit
*  Parameters:    void
*  return value:   0-sucess or others-fail
**********************************************************/
static int bq25970_charge_exit(void)
{
	int ret = 0;
	struct bq25970_device_info *di = g_bq25970_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	ret = bq25970_charge_enable(BQ2597X_SWITCHCAP_DISABLE);

	bq25970_init_finish_flag = BQ2597X_NOT_INIT;
	bq25970_interrupt_notify_enable_flag = BQ2597X_DISABLE_INTERRUPT_NOTIFY;

	msleep(10);

	return ret;
}
/**********************************************************
*  Function:       bq25970_adc_enable_enable
*  Discription:    Enable/disable ADC
*  Parameters:     enable: 1   disable: 0
*  return value:   0-sucess or others-fail
**********************************************************/
#if 0
static int bq25970_adc_enable(u8 enable)
{
	int ret;
	u8 value = enable ? 0x1 : 0x0;

	ret = bq25970_write_mask(BQ2597X_ADC_CTRL_REG, BQ2597X_ADC_CTRL_EN_MASK, BQ2597X_ADC_CTRL_EN_SHIFT, value);
	if (ret) {
		hwlog_err("error: adc_enable write fail!\n");
		return -1;
	}

	hwlog_info("adc_enable [%x]=0x%x\n", BQ2597X_ADC_CTRL_REG, value);
	return 0;
}
#endif

static int bq25970_batinfo_exit(void)
{
	return 0;
}

static int bq25970_batinfo_init(void)
{
	int ret = 0;

	ret = bq25970_chip_init();
	if (ret) {
		hwlog_err("error: batinfo init fail!\n");
		return -1;
	}

	return ret;
}

static void bq25970_interrupt_work(struct work_struct *work)
{
	struct bq25970_device_info *di = container_of(work, struct bq25970_device_info, irq_work);
	struct nty_data * data = &(di->nty_data);
	struct atomic_notifier_head *direct_charge_fault_notifier_list;
	u8 converter_state;
	u8 fault_flag;
	u8 ac_protection;
	u8 ibus_ucp;
	int val = 0;

	direct_charge_sc_get_fault_notifier(&direct_charge_fault_notifier_list);

	bq25970_read_byte(BQ2597X_AC_OVP_REG, &ac_protection);
	bq25970_read_byte(BQ2597X_BUS_OCP_UCP_REG, &ibus_ucp);
	bq25970_read_byte(BQ2597X_FLT_FLAG_REG, &fault_flag);
	bq25970_read_byte(BQ2597X_CONVERTER_STATE_REG, &converter_state);

	data->event1 = fault_flag;
	data->event2 = ac_protection;
	data->addr = di->client->addr;

	if (BQ2597X_ENABLE_INTERRUPT_NOTIFY == bq25970_interrupt_notify_enable_flag) {
		if (ac_protection & BQ2597X_AC_OVP_FLAG_MASK) {
			hwlog_info("AC OVP happened\n");

			atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_AC_OVP, data);
		}
		else if (fault_flag & BQ2597X_BAT_OVP_FLT_FLAG_MASK) {
			hwlog_info("BAT OVP happened\n");

			val = bq25970_get_vbat_mv();
			if (BQ2597X_VBAT_OVP_THRESHOLD_INIT <= val){
				hwlog_info("BAT OVP happened [%d]\n", val);

				atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_VBAT_OVP, data);
			}
		}
		else if (fault_flag & BQ2597X_BAT_OCP_FLT_FLAG_MASK) {
			hwlog_info("BAT OCP happened\n");

			bq25970_get_ibat_ma(&val);
			if (BQ2597X_IBAT_OCP_THRESHOLD_INIT <= val) {
				hwlog_info("BAT OCP happened [%d]\n", val);

				atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_IBAT_OCP, data);
			}
		}
		else if (fault_flag & BQ2597X_BUS_OVP_FLT_FLAG_MASK) {
			hwlog_info("BUS OVP happened\n");

			bq25970_get_vbus_mv(&val);
			if (BQ2597X_VBUS_OVP_THRESHOLD_INIT <= val) {
				hwlog_info("BUS OVP happened [%d]\n", val);

				atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_VBUS_OVP, data);
			}
		}
		else if (fault_flag & BQ2597X_BUS_OCP_FLT_FLAG_MASK) {
			hwlog_info("BUS OCP happened\n");

			bq25970_get_ibus_ma(&val);
			if (BQ2597X_IBUS_OCP_THRESHOLD_INIT <= val) {
				hwlog_info("BUS OCP happened [%d]\n", val);

				atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_IBUS_OCP, data);
			}
		}
		else if (fault_flag & BQ2597X_TSBAT_FLT_FLAG_MASK) {
			hwlog_info("BAT TEMP OTP happened\n");

			atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_TSBAT_OTP, data);
		}
		else if (fault_flag & BQ2597X_TSBUS_FLT_FLAG_MASK) {
			hwlog_info("BUS TEMP OTP happened\n");

			atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_TSBUS_OTP, data);
		}
		else if (fault_flag & BQ2597X_TDIE_ALM_FLAG_MASK) {
			hwlog_info("DIE TEMP OTP happened\n");

			//atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_TDIE_OTP, data);
		}
		else {
			/* do nothing */
		}

		if (converter_state & BQ2597X_CONV_OCP_FLAG_MASK) {
			hwlog_info("CONV OCP happened\n");
			atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_CONV_OCP, data);
		}

		bq25970_dump_register();
	}

	hwlog_err("ac_ovp_reg [%x]=0x%x\n", BQ2597X_AC_OVP_REG, ac_protection);
	hwlog_err("bus_ocp_ucp_reg [%x]=0x%x\n", BQ2597X_BUS_OCP_UCP_REG, ibus_ucp);
	hwlog_err("flt_flag_reg [%x]=0x%x\n", BQ2597X_FLT_FLAG_REG, fault_flag);
	hwlog_err("converter_state_reg [%x]=0x%x\n", BQ2597X_CONVERTER_STATE_REG, converter_state);

	/* clear irq */
	enable_irq(di->irq_int);
}

static irqreturn_t bq25970_interrupt(int irq, void *_di)
{
	struct bq25970_device_info *di = _di;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	if (0 == di->chip_already_init) {
		hwlog_err("error: chip not init!\n");
	}

	if (BQ2597X_INIT_FINISH == bq25970_init_finish_flag) {
		bq25970_interrupt_notify_enable_flag = BQ2597X_ENABLE_INTERRUPT_NOTIFY;
	}

	hwlog_info("bq25970 interrupt happened (%d)!\n", bq25970_init_finish_flag);

	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static void bq25970_parse_dts(struct device_node *np, struct bq25970_device_info *di)
{
	int ret = 0;

	ret = of_property_read_u32(np, "tsbus_high_r_kohm", &tsbus_high_r_kohm);
	if (ret) {
		tsbus_high_r_kohm = BQ2597X_RESISTORS_100KOHM;
	}
	hwlog_info("tsbus_high_r_kohm=%d\n", tsbus_high_r_kohm);

	ret = of_property_read_u32(np, "tsbus_low_r_kohm", &tsbus_low_r_kohm);
	if (ret) {
		tsbus_low_r_kohm = BQ2597X_RESISTORS_100KOHM;
	}
	hwlog_info("tsbus_low_r_kohm=%d\n", tsbus_low_r_kohm);

	ret = of_property_read_u32(np, "switching_frequency", &switching_frequency);
	if (ret) {
		switching_frequency = BQ2597X_SWITCHING_FREQ_550KHZ;
	}
	hwlog_info("switching_frequency=%d\n", switching_frequency);
}

static struct loadswitch_ops bq25970_sysinfo_ops = {
	.ls_init = bq25970_charge_init,
	.ls_exit = bq25970_charge_exit,
	.ls_enable = bq25970_charge_enable,
	.ls_discharge = bq25970_discharge,
	.is_ls_close = bq25970_is_device_close,
	.get_ls_id = bq25970_get_device_id,
	.watchdog_config_ms = bq25970_config_watchdog_ms,
	.ls_status = bq25970_is_tsbat_disabled,
};

static struct batinfo_ops bq25970_batinfo_ops = {
	.init = bq25970_batinfo_init,
	.exit = bq25970_batinfo_exit,
	.get_bat_btb_voltage = bq25970_get_vbat_mv,
	.get_bat_package_voltage = bq25970_get_vbat_mv,
	.get_vbus_voltage = bq25970_get_vbus_mv,
	.get_bat_current = bq25970_get_ibat_ma,
	.get_ls_ibus = bq25970_get_ibus_ma,
	.get_ls_temp = bq25970_get_device_temp,
};

static struct power_thermalzone_ops bq25970_temp_sensing_ops = {
	.get_raw_data = bq25970_get_tsbus_ntc_resistor,
};

/**********************************************************
*  Function:       bq2597x_probe
*  Discription:    bq2597x module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25970_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct bq25970_device_info *di = NULL;
	struct device_node *np = NULL;

	hwlog_info("probe begin\n");

	if (NULL == client || NULL == id) {
		hwlog_err("error: client is null or id is null!\n");
		return -ENOMEM;
	}

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_bq25970_dev = di;

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	INIT_WORK(&di->irq_work, bq25970_interrupt_work);

	bq25970_parse_dts(np, di);

	di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
	hwlog_info("gpio_int=%d\n", di->gpio_int);

	if (!gpio_is_valid(di->gpio_int)) {
		hwlog_err("error: gpio(gpio_int) is not valid!\n");
		ret = -EINVAL;
		goto bq25970_fail_0;
	}

	ret = gpio_request(di->gpio_int, "bq25970_gpio_int");
	if (ret < 0) {
		hwlog_err("error: gpio(gpio_int) request fail!\n");
		ret = -EINVAL;
		goto bq25970_fail_0;
	}

	ret = gpio_direction_input(di->gpio_int);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) set input fail!\n");
		goto bq25970_fail_1;
	}

	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0) {
		hwlog_err("error: gpio(gpio_int) map to irq fail!\n");
		ret = -EINVAL;
		goto bq25970_fail_1;
	}

	ret = request_irq(di->irq_int, bq25970_interrupt, IRQF_TRIGGER_FALLING, "bq25970_int_irq", di);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) irq request fail!\n");
		ret = -EINVAL;
		di->irq_int = -1;
		goto bq25970_fail_1;
	}

	ret = sc_ops_register(&bq25970_sysinfo_ops);
	if (ret) {
		hwlog_err("error: bq25970 sysinfo ops register fail!\n");
		goto bq25970_fail_2;
	}

	ret = batinfo_sc_ops_register(&bq25970_batinfo_ops);
	if (ret) {
		hwlog_err("error: bq25970 batinfo ops register fail!\n");
		goto bq25970_fail_2;
	}

#ifdef CONFIG_WIRELESS_CHARGER
	ret = wireless_sc_ops_register(&bq25970_sysinfo_ops);
	if (ret) {
		hwlog_err("error: bq25970 wireless_sc sysinfo ops register fail!\n");
		goto bq25970_fail_2;
	}

	ret = wireless_sc_batinfo_ops_register(&bq25970_batinfo_ops);
	if (ret) {
		hwlog_err("error: bq25970 wireless_sc batinfo ops register fail!\n");
		goto bq25970_fail_2;
	}
#endif

	ret = power_thermalzone_ops_register(&bq25970_temp_sensing_ops, "bq25970");
	if (ret) {
		hwlog_err("error: bq25970 thermalzone ops register fail!\n");
	}

	di->chip_already_init = 1;

	ret = bq25970_reg_reset();
	if (ret) {
		hwlog_err("error: bq25970 reg reset fail!\n");
		di->chip_already_init = 0;
		goto bq25970_fail_2;
	}

	ret = bq25970_reg_init();
	if (ret) {
		hwlog_err("error: bq25970 reg init fail!\n");
		di->chip_already_init = 0;
		goto bq25970_fail_2;
	}

	hwlog_info("probe end\n");
	return 0;

bq25970_fail_2:
	free_irq(di->irq_int, di);
bq25970_fail_1:
	gpio_free(di->gpio_int);
bq25970_fail_0:
	g_bq25970_dev = NULL;
	devm_kfree(&client->dev, di);
	np = NULL;
	return ret;
}

/**********************************************************
*  Function:       bq2597x_remove
*  Discription:    bq2597x module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq25970_remove(struct i2c_client *client)
{
	struct bq25970_device_info *di = i2c_get_clientdata(client);

	hwlog_info("remove begin\n");

	if (di->irq_int) {
		free_irq(di->irq_int, di);
	}

	if (di->gpio_int) {
		gpio_free(di->gpio_int);
	}

	hwlog_info("remove end\n");
	return 0;
}

static void bq25970_shutdown(struct i2c_client *client)
{
	bq25970_reg_reset();
}

MODULE_DEVICE_TABLE(i2c, bq25970);

static struct of_device_id bq25970_of_match[] = {
	{
		.compatible = "bq25970",
		.data = NULL,
	},
	{},
};

static const struct i2c_device_id bq25970_i2c_id[] = {
	{"bq25970", 0}, {}
};

static struct i2c_driver bq25970_driver = {
	.probe = bq25970_probe,
	.remove = bq25970_remove,
	.shutdown = bq25970_shutdown,
	.id_table = bq25970_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "bq25970",
		.of_match_table = of_match_ptr(bq25970_of_match),
	},
};

/**********************************************************
*  Function:       bq2597x_init
*  Discription:    bq2597x module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init bq25970_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&bq25970_driver);
	if (ret) {
		hwlog_err("error: bq25970 i2c_add_driver error!\n");
	}

	return ret;
}

/**********************************************************
*  Function:       bq2597x_exit
*  Discription:    bq2597x module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit bq25970_exit(void)
{
	i2c_del_driver(&bq25970_driver);
}

module_init(bq25970_init);
module_exit(bq25970_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("bq25970 module driver");
MODULE_AUTHOR("HW Inc");
