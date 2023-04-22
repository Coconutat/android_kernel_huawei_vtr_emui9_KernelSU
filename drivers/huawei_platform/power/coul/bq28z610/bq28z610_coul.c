
/*
 * drivers/power/coul/bq28z610_coul.c
 * bqGauge battery driver
 * Copyright (C) 2016 HUAWEI, Inc.
 *
 * This package is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/module.h>
#include <linux/param.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/idr.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <asm/unaligned.h>
#include <linux/of.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HISI_COUL
#include <linux/power/hisi/coul/hisi_coul_drv.h>
#endif
#ifdef CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif

#include <bq28z610_coul.h>

#define HWLOG_TAG bq28z610_coul
HWLOG_REGIST();

static struct bq28z610_device_info *g_bq28z610_dev;
static struct mutex bq28z610_mutex;
static struct chrg_para_lut *p_batt_data;
static struct device *coul_dev;


/**********************************************************
*  Function:       bq28z610_read_block
*  Discription:    register read block interface
*  Parameters:   reg:register addr
*                      data:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_read_block(struct bq28z610_device_info *di, u8 reg, u8 *data, u8 len)
{
	struct i2c_msg msg[2];
	int ret = 0;
	int i = 0;

	if (!di->client->adapter)
		return -ENODEV;

	msg[0].addr = di->client->addr;
	msg[0].flags = 0;
	msg[0].buf = &reg;
	msg[0].len = 1;

	msg[1].addr = di->client->addr;
	msg[1].flags = I2C_M_RD;
	msg[1].buf = data;
	msg[1].len = len;

	mutex_lock(&bq28z610_mutex);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(di->client->adapter, msg, ARRAY_SIZE(msg));
		if(ret >= 0)
			break;
		msleep(5);
	}
	mutex_unlock(&bq28z610_mutex);

	if (ret < 0) {
		hwlog_err("bq28z610 read block fail\n");
		return -1;
	}

	return 0;
}

/**********************************************************
*  Function:       bq28z610_read_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register addr
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_read_byte(u8 reg, u8* data)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;

	return bq28z610_read_block(di, reg, data, 1);
}

/**********************************************************
*  Function:       bq28z610_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register addr
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_read_word(u8 reg, u16* data)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u8 buff[2] = { 0 };
	int ret = 0;

	ret = bq28z610_read_block(di, reg, &buff, 2);
	if (ret)
		return -1;

	*data = get_unaligned_le16(buff);
	return 0;
}

/**********************************************************
*  Function:       bq28z610_wite_block
*  Discription:    register wite block interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_write_block(struct bq28z610_device_info *di, u8 reg, u8 *buff, u8 len)
{
	struct i2c_msg msg;
	int ret;
	int i = 0;

	if (!di->client->adapter)
		return -ENODEV;

	buff[0] = reg;

	msg.buf = buff;
	msg.addr = di->client->addr;
	msg.flags = 0;
	msg.len = len + 1;

	mutex_lock(&bq28z610_mutex);
	for (i = 0; i < I2C_RETRY_CNT; i++) {
		ret = i2c_transfer(di->client->adapter, &msg, 1);
		if (ret >= 0)
			break;
		msleep(5);
	}
	mutex_unlock(&bq28z610_mutex);

	if (ret < 0) {
		hwlog_err("bq28z610 write block fail\n");
		return -1;
	}

	return 0;
}

/**********************************************************
*  Function:       bq28z610_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_write_byte(u8 reg, u8 data)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u8 buff[2];
	buff[1] = data;

	return bq28z610_write_block(di, reg, buff, 1);
}

/**********************************************************
*  Function:       bq28z610_write_word
*  Discription:    register write word interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_write_word(u8 reg, u16 data)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u8 buff[4];
	put_unaligned_le16(data, &buff[1]);

	return bq28z610_write_block(di, reg,buff, 2);
}

/**********************************************************
*  Function:       checksum
*  Discription:    get checksum
*  Parameters:   u8 *data:MAC data
*                      len:data length
*  return value:  checksum number
**********************************************************/
static u8 checksum(u8 *data, u8 len)
{
	u16 sum = 0;
	int i;

	for (i = 0; i < len; i++)
		sum += data[i];

	sum &= 0xFF;

	return 0xFF - sum;
}

/**********************************************************
*  Function:       bq28z610_read_mac_data
*  Discription:    readedata in the Manufacturer Access System (MAC)
*  Parameters:   u16 cmd:AltManufacturerAccess
*                      u8 *dat:flash value
*  return value:  >0-sucess or others-fail
**********************************************************/
static int bq28z610_read_mac_data(u16 cmd, u8 *dat)
{
	u8 buf[36];
	u8 cksum_calc,cksum;
	u8 len;
	int ret;
	int i;

	struct bq28z610_device_info *di = g_bq28z610_dev;

    	/* read length to be read */
	ret  = bq28z610_read_byte(BQ28Z610_MAC_DATA_LEN, &len);
	if (ret) {
		hwlog_err("Failed to read BQ28Z610_MAC_DATA_LEN register:%d\n", len);
		return 0;
	}

	if (len >36)
		len = 36;
	len = len - 2;	/* here length includes checksum byte and length byte itself */
	if (len < 0) {
		hwlog_err("length is not correct %04x\n", len);
		return 0;
	}

	mdelay(2);
	/* read data */
	ret = bq28z610_read_block(di, BQ28Z610_MAC_CMD, &buf, len);
	if (ret)
		hwlog_err("bq28z610 read mac data fail.\n");

	mdelay(2);
	/* calculate checksum */
	cksum_calc = checksum(buf, len);

	/*read gauge calculated checksum */
	ret  = bq28z610_read_byte(BQ28Z610_MAC_DATA_CHECKSUM, &cksum);
	if (ret) {
		hwlog_err("Failed to read BQ28Z610_MAC_DATA_CHECKSUM register:%d\n", cksum);
		return 0;
	}

	/* compare checksum */
	if (cksum != cksum_calc) {
		hwlog_err("checksum error\n");
		return 0;
	}

	if (cmd != get_unaligned_le16(buf)) { // command code not match
		hwlog_err("command code not match, input: %04x output: %04x\n",
			cmd, get_unaligned_le16(buf));
		return 0;
	}

	/*ignore command code, return data field*/
	len -= 2;
	for (i = 0; i < len; i++)
		dat[i] = buf[i+2];

	return (int)len;
}

/*******************************************************
* Function:		bq28z610_is_ready
*  Description:	check wheather coul is ready
*  Parameters:	void
*  return value:	1: ready     0:not ready
********************************************************/
static int bq28z610_is_ready(void)
{
	if (g_bq28z610_dev)
		return 1;
	else
		return 0;
}

/*******************************************************
* Function:		bq28z610_get_battery_id_vol
*  Description:	get voltage on ID pin by HKADC
*  Parameters:	void
*  return value:	batt id vol
********************************************************/
static int bq28z610_get_battery_id_vol(void)
{
	return 0;
}

/*******************************************************
*  Function:		bq28z610_get_battery_temp
*  Description:	get the temperature of battery
*  Parameters:	void
*  return value:	the temperature of battery in degrees centigrade
********************************************************/
static int bq28z610_get_battery_temp(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	int temp_c = ABNORMAL_BATT_TEMPERATURE_LOW -1;
	u16 temp_k = temp_c + 273;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_TEMP, &temp_k) ;
	if (ret) {
		hwlog_err("Failed to read Temperature register:%d\n", temp_k);
		temp_c = di->cache.temp;
	} else {
		temp_c = (int)temp_k/ 10 - 273;
	}

	di->cache.temp = temp_c;

	return temp_c;
}

/*******************************************************
* Function:		bq28z610_is_battery_exist
*  Description:	check whether battery exist
*  Parameters:	void
*  return value:	0:battery isn't exist, 1: exist
********************************************************/
static int bq28z610_is_battery_exist(void)
{
	int temp;

#ifdef CONFIG_HLTHERM_RUNTEST
    	return 0;
#endif

	temp = bq28z610_get_battery_temp();

	if ((temp <= ABNORMAL_BATT_TEMPERATURE_LOW)
		|| (temp >= ABNORMAL_BATT_TEMPERATURE_HIGH))
		return 0;
	else
		return 1;
}

/*******************************************************
*  Function:        bq28z610_get_battery_soc
*  Description:    get the soc of battery
*  Parameters:    void
*  return value:   the soc of battery
********************************************************/
static int bq28z610_get_battery_soc(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	int ret = 0;
	u16 soc = 0;

	if(CHARGE_STATE_CHRG_DONE == di->charge_status) {
		hwlog_info("charge done, force soc to full.\n");
		return BATTERY_FULL_CAPACITY;
	}

	ret = bq28z610_read_word(BQ28Z610_REG_SOC, &soc);
	if (ret) {
		hwlog_err("Failed to read RelativeStateOfCharge register:%d\n", soc);
		soc = di->cache.soc;
	}
	di->cache.soc = soc;
	hwlog_info("soc = %d\n", (int)soc);
	return (int)soc;
}

/*******************************************************
* Function:		bq28z610_is_battery_reach_threshold
*  Description:	check whether battery uah reach threshold
*  Parameters:	void
*  return value:	0:not, 4: lower than warning_lev, 8: lower than Low_Lev
********************************************************/
int bq28z610_is_battery_reach_threshold(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	int soc = 0;

	if (!bq28z610_is_battery_exist())
		return 0;

	soc = bq28z610_get_battery_soc();

	if (soc > BATTERY_CC_WARNING_LEV)
		return 0;
	else if (soc > BATTERY_CC_LOW_LEV)
		return BQ_FLAG_SOC1;
	else
		return BQ_FLAG_LOCK;
}

/*******************************************************
* Function:		bq28z610_get_battery_brand
*  Description:	get battery brand in string.
*  Parameters:	void
*  return value:	battery band string
********************************************************/
char* bq28z610_get_battery_brand(void)
{
	u8 data[40];
	static char brand[BQ28Z610_BATT_BRAND_LEN];
	int ret;
	int len;
	int i;

	ret = bq28z610_write_word(BQ28Z610_MAC_CMD, BQ28Z610_MANUINFO);
	if (ret) {
		hwlog_err("Failed to write AltManufacturerAccess register:%d\n");
		return("error");
	}
	mdelay(5);
	len = bq28z610_read_mac_data(BQ28Z610_MANUINFO, data);
	if (BQ28Z610_MANUINFO_LEN != len) {
		hwlog_err("Failed to read manufacture info\n");
		return("error");
	}

	memset(brand, 0, BQ28Z610_BATT_BRAND_LEN);
	switch (data[BQ28Z610_PACK_NAME]) {
	case 'D':
		strncat(brand, "desay", strlen("desay"));
		break;
	case 'I':
		strncat(brand, "sunwoda", strlen("sunwoda"));
		break;
	case 'C':
		strncat(brand, "coslight", strlen("coslight"));
		break;
	default:
		strncat(brand, "error", strlen("error"));
		break;
	}
	switch (data[BQ28Z610_CELL_NAME]) {
	case 'A':
		strncat(brand, "atl", strlen("atl"));
		break;
	case 'L':
		strncat(brand, "lg", strlen("lg"));
		break;
	case 'C':
		strncat(brand, "coslight", strlen("coslight"));
		break;
	default:
		strncat(brand, "error", strlen("error"));
		break;
	}

	len = strlen(brand);
	for (i = 0; i < BQ28Z610_PACK_DATE_LEN; i++)
		if (len + i < BQ28Z610_BATT_BRAND_LEN -1)
			brand[len + i] = data[BQ28Z610_PACK_DATE + i] + 0x30;

	return brand;
}

/*******************************************************
*  Function:        bq28z610_get_battery_vol
*  Description:    get the voltage of battery
*  Parameters:    void
*  return value:   the voltage of battery in mV
********************************************************/
static int bq28z610_get_battery_vol(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 vol = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_VOLT, &vol);
	if (ret) {
		hwlog_err("Failed to read Voltage register:%d\n", vol);
		vol = di->cache.vol;
	}
	di->cache.vol = vol;

	return (int)vol;
}

/*******************************************************
*  Function:        bq28z610_get_battery_vol_uv
*  Description:    get the voltage of battery
*  Parameters:    void
*  return value:   the voltage of battery in uV
********************************************************/
static int bq28z610_get_battery_vol_uv(void)
{
	return 1000 * bq28z610_get_battery_vol();
}

/*******************************************************
*  Function:        bq28z610_get_battery_curr
*  Description:    get the current of battery
*  Parameters:    void
*  return value:   the current of battery in mA
********************************************************/
static int bq28z610_get_battery_curr(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 curr = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_CURR, &curr);
	if (ret) {
		hwlog_err("Failed to read Current register:%d\n", curr);
		curr = di->cache.curr;
	}

	di->cache.curr= curr;

	return -(int)((s16)curr);
}

/*******************************************************
*  Function:        bq28z610_get_battery_avgcurr
*  Description:    get the average current of battery
*  Parameters:    void
*  return value:   the average current of battery in mA
********************************************************/
static int bq28z610_get_battery_avgcurr(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 avg_curr = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_AVRGCURR, &avg_curr);
	if (ret) {
		hwlog_err("Failed to read AverageCurrent register:%d\n", avg_curr);
		avg_curr = di->cache.avg_curr;
	}

	di->cache.avg_curr= avg_curr;

	return (int)((s16)avg_curr);
}

/*******************************************************
*  Function:        bq28z610_get_battery_rm
*  Description:    get the remaining capacity of battery
*  Parameters:    void
*  return value:   the remaining capacity of battery in mAh
********************************************************/
static int bq28z610_get_battery_rm(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 rm = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_RM, &rm);
	if (ret) {
		hwlog_err("Failed to read RemainingCapacity register:%d\n", rm);
		rm = di->cache.rm;
	}
	di->cache.rm = rm;

	return (int)rm;
}

/*******************************************************
*  Function:        bq28z610_get_battery_dc
*  Description:    get the design capacity of battery
*  Parameters:    void
*  return value:   the design capacity of battery in mAh
********************************************************/
static int bq28z610_get_battery_dc(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 dc = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_DC, &dc);
	if (ret) {
		hwlog_err("Failed to read DesignCapacity register:%d\n", dc);
		dc = di->cache.dc;
	}
	di->cache.dc = dc;

	return (int)dc;
}

/*******************************************************
*  Function:        bq28z610_get_battery_fcc
*  Description:    get the full charge capacity of battery
*  Parameters:    void
*  return value:   the full charge capacity of battery in mAh
********************************************************/
static int bq28z610_get_battery_fcc(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 fcc = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_FCC, &fcc);
	if (ret) {
		hwlog_err("Failed to read FullChargeCapacity register:%d\n", fcc);
		fcc = di->cache.fcc;
	}
	di->cache.fcc = fcc;

	return (int)fcc;
}

/*******************************************************
*  Function:        bq28z610_get_battery_soh
*  Description:    get the state of health of battery, SOH = FCC/DC
*  Parameters:    void
*  return value:   the  the state of health of battery
********************************************************/
static int bq28z610_get_battery_soh(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u8 soh = 0;
	u8 status = 0;
	int ret = 0;

	ret  = bq28z610_read_byte(BQ28Z610_REG_SOH + 1, &status);
	if (ret) {
		hwlog_err("Failed to read BQ28Z610_REG_SOH+1 register:%d\n", status);
	}
	if (BQ28Z610_SOH_READY != status) {
		hwlog_err("bq28z610 soh is not ready");
	}
	ret = bq28z610_read_byte(BQ28Z610_REG_SOH, &soh);
	if (ret) {
		hwlog_err("Failed to read BQ28Z610_REG_SOH register:%d\n", soh);
		soh = di->cache.soh;
	}
	di->cache.soh = soh;

	return (int)soh;
}

/*******************************************************
*  Function:        bq28z610_get_battery_tte
*  Description:    get the average time to empty of battery
*  Parameters:    void
*  return value:   the average time to empty of battery in minutes
********************************************************/
static int bq28z610_get_battery_tte(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 tte = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_TTE, &tte);
	if (ret) {
		hwlog_err("Failed to read AverageTimeToEmpty register:%d\n", tte);
		tte = di->cache.tte;
	}
	di->cache.tte = tte;

	if (tte == 65535) {
		hwlog_info("the battery is not being discharged\n");
		return -1;
	}

	return (int)tte;
}

/*******************************************************
*  Function:        bq28z610_get_battery_tte
*  Description:    get the average time to full of battery
*  Parameters:    void
*  return value:   the average time to full of battery in minutes
********************************************************/
static int bq28z610_get_battery_ttf(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 ttf = 0;
	int ret = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_TTF, &ttf);
	if (ret) {
		hwlog_err("Failed to read AverageTimeToFull register:%d\n", ttf);
		ttf = di->cache.ttf;
	}
	di->cache.ttf = ttf;

	if (ttf == 65535) {
		hwlog_info("the battery is not being charged\n");
		return -1;
	}

	return (int)ttf;
}

/*******************************************************
*  Function:        bq28z610_get_battery_cycle
*  Description:    get the cycle count of battery
*  Parameters:    void
*  return value:   the cycle count of battery
********************************************************/
static int bq28z610_get_battery_cycle(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	int cycle = 0;
	int ret  = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_CYCLE, &cycle);
	if (ret) {
		hwlog_err("Failed to read BQ28Z610_REG_CYCLE register:%d\n", cycle);
		cycle = di->cache.ttf;
	}
	di->cache.cycle = cycle;

	return cycle;
}

/*******************************************************
*  Function:        bq28z610_battery_unfiltered_soc
*  Description:    get the unfiltered soc of battery
*  Parameters:    void
*  return value:   the unfiltered soc of battery
********************************************************/
static int bq28z610_battery_unfiltered_soc(void)
{
	return bq28z610_get_battery_soc();
}

/*******************************************************
*  Function:        bq28z610_get_battery_health
*  Description:    get battery health
*  Parameters:    void
*  return value:   battery health status
********************************************************/
static int bq28z610_get_battery_health(void)
{
	int status = POWER_SUPPLY_HEALTH_GOOD;
	int temp = 0;

	if (!bq28z610_is_battery_exist())
		return 0;

	temp = bq28z610_get_battery_temp();
	if (temp < TEMP_TOO_COLD)
		status = POWER_SUPPLY_HEALTH_COLD;
	else if (temp > TEMP_TOO_HOT)
		status = POWER_SUPPLY_HEALTH_OVERHEAT;

	return status;
}

/*******************************************************
*  Function:        bq28z610_get_battery_capacity_level
*  Description:    get battery capacity level
*  Parameters:    void
*  return value:   0: Unknown, 1:CRITICAL, 2:LOW, 3:NORMAL, 4:HIGH,5:FULL
********************************************************/
static int bq28z610_get_battery_capacity_level (void)
{
	int capacity = 0;
	int status = 0;

	if (!bq28z610_is_battery_exist())
		return 0;

	capacity = bq28z610_get_battery_soc();

	if (capacity > 100 || capacity < 0)
		status = POWER_SUPPLY_CAPACITY_LEVEL_UNKNOWN;
	else if ((capacity >= 0) && (capacity <= 5))
		status = POWER_SUPPLY_CAPACITY_LEVEL_CRITICAL;
	else if ((capacity > 5) && (capacity <= 15))
		status = POWER_SUPPLY_CAPACITY_LEVEL_LOW;
	else if ((capacity >= 95) && (capacity < 100))
		status = POWER_SUPPLY_CAPACITY_LEVEL_HIGH;
	else if (100 == capacity)
		status = POWER_SUPPLY_CAPACITY_LEVEL_FULL;
	else
		status = POWER_SUPPLY_CAPACITY_LEVEL_NORMAL;

	return status;
}

/*******************************************************
*  Function:        bq28z610_get_battery_status
*  Description:    get the battery status of battery
*  Parameters:    void
*  return value:   the battery status of battery
********************************************************/
static int bq28z610_get_battery_status(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;
	u16 flags = 0;
	int ret  = 0;

	ret = bq28z610_read_word(BQ28Z610_REG_FLAGS, &flags);
	if (ret) {
		hwlog_err("Failed to read BQ28Z610_REG_FLAGS register:%d\n", flags);
		flags = di->cache.flags;
	}
	di->cache.flags = flags;

	return (int)flags;
}

/*******************************************************
*  Function:        bq28z610_get_battery_technology
*  Description:    get battery_technology
*  Parameters:    void
*  return value:   "Li-poly"
********************************************************/
static int bq28z610_get_battery_technology(void)
{
	/*Default technology is "Li-poly"*/
	return POWER_SUPPLY_TECHNOLOGY_LIPO;
}

/*******************************************************
*  Function:        bq28z610_get_battery_vbat_max
*  Description:    get battery vbat max vol
*  Parameters:    void
*  return value:   max vbatt vol
********************************************************/
static int bq28z610_get_battery_vbat_max(void)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;

	return di->vbat_max;
}

/*******************************************************
*  Function:        bq28z610_is_fcc_debounce
*  Description:    check whether fcc is debounce
*  Parameters:    void
*  return value:    0: no  1: is debounce
********************************************************/
static int bq28z610_is_fcc_debounce(void)
{
	return 0;
}

/*******************************************************
*  Function:        bq28z610_device_check
*  Description:     check bq28z610 is ok
*  Parameters:      void
*  return value:    0: success  1: fail
********************************************************/
static int bq28z610_device_check(void)
{
	u8 version;
	u8 manu_info[40];
	int ret;
	int len;

	ret = bq28z610_write_word(BQ28Z610_MAC_CMD, BQ28Z610_MANUINFO);
	if (ret) {
		hwlog_err("Failed to write AltManufacturerAccess register\n");
		return -1;
	}
	mdelay(5);
	len = bq28z610_read_mac_data(BQ28Z610_MANUINFO, manu_info);
	if (BQ28Z610_MANUINFO_LEN != len) {
		hwlog_err("Failed to read manufacture info\n");
		return -1;
	}
	version = manu_info[BQ28Z610_VERSION_INDEX];
	if(version == BQ28Z610_DEFAULT_VERSION){
		hwlog_err("Firmware version is wrong\n");
		return -1;
	}

	return 0;
}

/*******************************************************
* Function:		charger_event_process
* Description:		charge event distribution function
* Parameters:		di -- coul device; event -- charge event
* return value:	NULL
********************************************************/
static void charger_event_process(struct bq28z610_device_info *di, unsigned int event)
{
	if( NULL == di ) {
		hwlog_err("NULL point in [%s]\n", __func__);
		return;
	}
	switch (event) {
	case VCHRG_START_USB_CHARGING_EVENT:
	case VCHRG_START_AC_CHARGING_EVENT:
	case VCHRG_START_CHARGING_EVENT:
		hwlog_info("receive charge start event = 0x%x\n",(int)event);
		di->charge_status = CHARGE_STATE_START_CHARGING;
		break;
	case VCHRG_STOP_CHARGING_EVENT:
		hwlog_info("receive charge stop event = 0x%x\n",(int)event);
		di->charge_status = CHARGE_STATE_STOP_CHARGING;
		break;

	case VCHRG_CHARGE_DONE_EVENT:
		hwlog_info("receive charge done event = 0x%x\n",(int)event);
		di->charge_status = CHARGE_STATE_CHRG_DONE;
		break;

	case VCHRG_NOT_CHARGING_EVENT:
		di->charge_status = CHARGE_STATE_NOT_CHARGING;
		hwlog_err("charging is stop by fault\n");
		break;

	case VCHRG_POWER_SUPPLY_OVERVOLTAGE:
		di->charge_status = CHARGE_STATE_NOT_CHARGING;
		hwlog_err("charging is stop by overvoltage\n");
		break;
	case VCHRG_POWER_SUPPLY_WEAKSOURCE:
		di->charge_status = CHARGE_STATE_NOT_CHARGING;
		hwlog_err("charging is stop by weaksource\n");
		break;

	default:
		di->charge_status = CHARGE_STATE_NOT_CHARGING;
		hwlog_err("unknow event %d\n",(int)event);
		break;
    }
}

/*******************************************************
*  Function:		bq28z610_battery_charger_event_rcv
*  Description:	package charger_event_process, and be registered in scharger Model
				to get charge event
*  Parameters:	unsigned int event         ---- charge event
*  return value:	0
********************************************************/
static int bq28z610_battery_charger_event_rcv(unsigned int evt)
{
	struct bq28z610_device_info *di = g_bq28z610_dev;

	if (!di || !bq28z610_is_battery_exist())
		return 0;

	charger_event_process(di, evt);
	return 0;
}

/*******************************************************
*  Function:		get_batt_para
*  Description:	get battery para, save in struct p_batt_data
*  Parameters:	void
*  return value:	0-success; others-fail
********************************************************/
static int get_batt_para(void)
{
	int ret = 0;
	int array_len = 0;
	int i = 0;
	const char *chrg_data_string = NULL;
	struct bq28z610_device_info *di = g_bq28z610_dev;
	struct device_node *np = of_find_compatible_node(NULL, NULL, "Watt_2900_4400_battery");

	/*vbat_max*/
	ret = of_property_read_u32(np, "vbat_max", &di->vbat_max);
	if (ret) {
		hwlog_err("get vbat_max failed, use default value 4400mV.\n");
		di->vbat_max = BATTERY_DEFAULT_MAX_VOLTAGE;
	}

	/* temp_para */
	array_len = of_property_count_strings(np, "temp_para");

	if ((array_len <= 0) || (array_len % TEMP_PARA_TOTAL != 0)
	    || (array_len > TEMP_PARA_LEVEL * TEMP_PARA_TOTAL)) {
		hwlog_err("temp_para is invaild,please check temp_para number!!\n");
		return -EINVAL;
	}
	p_batt_data->temp_len = array_len;
	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "temp_para", i, &chrg_data_string);
		if (ret) {
			hwlog_err("get temp_para failed\n");
			return -EINVAL;
		}
		p_batt_data->temp_data[i / TEMP_PARA_TOTAL][i % TEMP_PARA_TOTAL]
			= simple_strtol(chrg_data_string, NULL, 10);
		hwlog_debug("p_batt_data->temp_data[%d][%d] = %d\n",
			i / TEMP_PARA_TOTAL,
			i % TEMP_PARA_TOTAL,
			p_batt_data->temp_data[i / TEMP_PARA_TOTAL][i % TEMP_PARA_TOTAL]);
	}

	/* vbat_para */
	array_len = of_property_count_strings(np, "vbat_para");
	if ((array_len <= 0) || (array_len % VOLT_PARA_TOTAL != 0)
	    || (array_len > VOLT_PARA_LEVEL * VOLT_PARA_TOTAL)) {
		hwlog_err("vbat_para is invaild,please check vbat_para number!!\n");
		return -EINVAL;
	}
	p_batt_data->volt_len = array_len;
	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "vbat_para", i, &chrg_data_string);
		if (ret) {
			hwlog_err("get vbat_para failed\n");
			return -EINVAL;
		}
		p_batt_data->volt_data[i / VOLT_PARA_TOTAL][i % VOLT_PARA_TOTAL]
			= simple_strtol(chrg_data_string, NULL, 10);
		hwlog_debug("chrg_para->volt_data[%d][%d] = %d\n",
			i / VOLT_PARA_TOTAL,
			i % VOLT_PARA_TOTAL,
			p_batt_data->volt_data[i / VOLT_PARA_TOTAL][i % VOLT_PARA_TOTAL]);
	}

	/* segment_para */
	array_len = of_property_count_strings(np, "segment_para");
	if ((array_len <= 0) || (array_len % SEGMENT_PARA_TOTAL != 0)
	    || (array_len > SEGMENT_PARA_LEVEL * SEGMENT_PARA_TOTAL)) {
		hwlog_err("segment_para is invaild,please check segment_para number!!\n");
		return -EINVAL;
	}
	p_batt_data->segment_len = array_len;
	for (i = 0; i < array_len; i++) {
		ret = of_property_read_string_index(np, "segment_para", i, &chrg_data_string);
		if (ret) {
			hwlog_err("get segment_para failed\n");
			return -EINVAL;
		}
		p_batt_data->segment_data[i / SEGMENT_PARA_TOTAL][i % SEGMENT_PARA_TOTAL]
			= simple_strtol(chrg_data_string, NULL, 10);
		hwlog_debug("chrg_para->segment_data[%d][%d] = %d\n",
			i / SEGMENT_PARA_TOTAL,
			i % SEGMENT_PARA_TOTAL,
			p_batt_data->segment_data[i / SEGMENT_PARA_TOTAL][i % SEGMENT_PARA_TOTAL]);
	}

	return 0;
}

/*******************************************************
  Function:        bq28z610_get_battery_charge_params
  Description:     battery data params
  Input:           NULL
  Output:          NULL
  Return:          0: invalid battery, 1: successed
********************************************************/
struct chrg_para_lut *bq28z610_get_battery_charge_params(void)
{
	return p_batt_data;
}

struct hisi_coul_ops bq28z610_ops = {
	.battery_id_voltage		= bq28z610_get_battery_id_vol,
	.is_coul_ready				= bq28z610_is_ready,
	.is_battery_exist			= bq28z610_is_battery_exist,
	.is_battery_reach_threshold	= bq28z610_is_battery_reach_threshold,
	.battery_brand			= bq28z610_get_battery_brand,
	.battery_voltage			= bq28z610_get_battery_vol,
	.battery_voltage_uv		= bq28z610_get_battery_vol_uv,
	.battery_current			= bq28z610_get_battery_curr,
	.battery_current_avg		= bq28z610_get_battery_avgcurr,
	.battery_unfiltered_capacity	= bq28z610_battery_unfiltered_soc,
	.battery_capacity			= bq28z610_get_battery_soc,
	.battery_temperature		= bq28z610_get_battery_temp,
	.battery_rm				= bq28z610_get_battery_rm,
	.battery_fcc				= bq28z610_get_battery_fcc,
	.battery_tte				= bq28z610_get_battery_tte,
	.battery_ttf				= bq28z610_get_battery_ttf,
	.battery_health			= bq28z610_get_battery_health,
	.battery_capacity_level		= bq28z610_get_battery_capacity_level,
	.battery_technology		= bq28z610_get_battery_technology,
	.battery_charge_params		= bq28z610_get_battery_charge_params,
	.battery_vbat_max			= bq28z610_get_battery_vbat_max,
	.charger_event_rcv			= bq28z610_battery_charger_event_rcv,
	.coul_is_fcc_debounce		= bq28z610_is_fcc_debounce,
	.battery_cycle_count		= bq28z610_get_battery_cycle,
	.battery_fcc_design		= bq28z610_get_battery_dc,
	.dev_check				= bq28z610_device_check,
};

#if CONFIG_SYSFS
#define BQ28Z610_COUL_SYSFS_FIELD(_name, n, m, store)                \
{                                                   \
	.attr = __ATTR(_name, m, bq28z610_coul_sysfs_show, store),    \
	.name = BQ28Z610_COUL_SYSFS_##n,       \
}

#define BQ28Z610_COUL_SYSFS_FIELD_RW(_name, n)               \
	BQ28Z610_COUL_SYSFS_FIELD(_name, n, S_IWUSR | S_IRUGO,       \
		bq28z610_coul_sysfs_store)

#define BQ28Z610_COUL_SYSFS_FIELD_RO(_name, n)               \
	BQ28Z610_COUL_SYSFS_FIELD(_name, n, S_IRUGO, NULL)

static ssize_t bq28z610_coul_sysfs_show(struct device *dev,
				       struct device_attribute *attr,
				       char *buf);
static ssize_t bq28z610_coul_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count);

struct bq28z610_coul_sysfs_field_info {
	struct device_attribute attr;
	u8 name;
};

static struct bq28z610_coul_sysfs_field_info bq28z610_coul_sysfs_field_tbl[] = {
	BQ28Z610_COUL_SYSFS_FIELD_RO(gaugelog,		GAUGELOG),
	BQ28Z610_COUL_SYSFS_FIELD_RO(gaugelog_head,	GAUGELOG_HEAD),
};

static struct attribute *bq28z610_coul_sysfs_attrs[ARRAY_SIZE(bq28z610_coul_sysfs_field_tbl) + 1];

static const struct attribute_group bq28z610_coul_sysfs_attr_group = {
	.attrs = bq28z610_coul_sysfs_attrs,
};

static void bq28z610_coul_sysfs_init_attrs(void)
{
	int i, limit = ARRAY_SIZE(bq28z610_coul_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		bq28z610_coul_sysfs_attrs[i] =
		    &bq28z610_coul_sysfs_field_tbl[i].attr.attr;
	}
	bq28z610_coul_sysfs_attrs[limit] = NULL;
}

static struct bq28z610_coul_sysfs_field_info
*bq28z610_coul_sysfs_field_lookup(const char *name)
{
	int i, limit = ARRAY_SIZE(bq28z610_coul_sysfs_field_tbl);

	for (i = 0; i < limit; i++) {
		if (!strncmp
		    (name, bq28z610_coul_sysfs_field_tbl[i].attr.attr.name,
		     strlen(name)))
			break;
	}
	if (i >= limit)
		return NULL;

	return &bq28z610_coul_sysfs_field_tbl[i];
}

static ssize_t bq28z610_coul_sysfs_show(struct device *dev,
				       struct device_attribute *attr, char *buf)
{
	struct bq28z610_coul_sysfs_field_info *info = NULL;
	int ret;
	int temp = 0, voltage = 0, curr = 0, avg_curr = 0, soc = 100, afsoc = 0, rm = 0, fcc = 0;

	info = bq28z610_coul_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	case BQ28Z610_COUL_SYSFS_GAUGELOG_HEAD:
		return snprintf(buf, PAGE_SIZE, "ss_VOL  ss_CUR  ss_AVGCUR  ss_SOC  SOC  ss_RM  ss_FCC  Temp  ");
	case BQ28Z610_COUL_SYSFS_GAUGELOG:
		temp	= bq28z610_get_battery_temp();
		voltage	= bq28z610_get_battery_vol();
		curr		= -bq28z610_get_battery_curr();
		avg_curr	= bq28z610_get_battery_avgcurr();
		soc		= bq28z610_get_battery_soc();
		afsoc	= hisi_bci_show_capacity();
		rm		= bq28z610_get_battery_rm();
		fcc		= bq28z610_get_battery_fcc();
		snprintf(buf, PAGE_SIZE, "%-6d  %-6d  %-9d  %-6d  %-3d  %-5d  %-6d  %-4d  ",
			voltage,  curr, avg_curr, soc, afsoc, rm, fcc, temp);
		return strlen(buf);
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n", __func__, info->name);
		break;
	}
	return 0;
}

static ssize_t bq28z610_coul_sysfs_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct bq28z610_coul_sysfs_field_info *info = NULL;
	int ret;
	long val = 0;

	info = bq28z610_coul_sysfs_field_lookup(attr->attr.name);
	if (!info)
		return -EINVAL;

	switch (info->name) {
	default:
		hwlog_err("(%s)NODE ERR!!HAVE NO THIS NODE:(%d)\n",__func__,info->name);
		break;
	}

	return count;
}

static int bq28z610_coul_sysfs_create_group(struct bq28z610_device_info *di)
{
	bq28z610_coul_sysfs_init_attrs();
	return sysfs_create_group(&di->dev->kobj,
				  &bq28z610_coul_sysfs_attr_group);
}

static inline void bq28z610_coul_sysfs_remove_group(struct bq28z610_device_info *di)
{
	sysfs_remove_group(&di->dev->kobj, &bq28z610_coul_sysfs_attr_group);
}
#else
static int bq28z610_coul_sysfs_create_group(struct bq28z610_device_info *di)
{
	return 0;
}

static inline void bq28z610_coul_sysfs_remove_group(struct bq28z610_device_info *di)
{
}
#endif


/**********************************************************
*  Function:       bq28z610_probe
*  Discription:    bq28z610 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret = 0;
	struct bq28z610_device_info *di = NULL;
	struct hisi_coul_ops *ops = NULL;
	struct device_node *np = NULL;
	struct class *power_class = NULL;

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("bq28z610_device_info is NULL!\n");
		return  -ENOMEM;
	}

	di->cache.vol = BATTERY_DEFAULT_VOLTAGE;
	di->cache.temp = ABNORMAL_BATT_TEMPERATURE_LOW -1;
	di->cache.soc = BATTERY_DEFAULT_CAPACITY;
	di->charge_status = CHARGE_STATE_NOT_CHARGING;
	g_bq28z610_dev = di;

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);

	mutex_init(&bq28z610_mutex);

	p_batt_data = (struct chrg_para_lut *)kzalloc(sizeof(struct chrg_para_lut), GFP_KERNEL);
	if (!p_batt_data) {
		hwlog_err("p_batt_data is NULL!\n");
		ret = -ENOMEM;
		goto bq28z610_fail_0;
	}
	ret = get_batt_para();
	if (ret) {
		hwlog_err("register bq28z610 coul ops failed!\n");
		goto bq28z610_fail_1;
	}

	ops = &bq28z610_ops;
	ret = hisi_coul_ops_register(ops, COUL_BQ28Z610);
	if (ret) {
		hwlog_err("register bq28z610 coul ops failed!\n");
		goto bq28z610_fail_2;
	}

	ret = bq28z610_coul_sysfs_create_group(di);
	if (ret)
		hwlog_err("can't create coul sysfs entries\n");
	power_class = hw_power_get_class();
	if (power_class) {
		if (coul_dev == NULL)
			coul_dev = device_create(power_class, NULL, 0, NULL, "coul");
		ret = sysfs_create_link(&coul_dev->kobj, &di->dev->kobj, "coul_data");
		if (ret) {
			hwlog_err("create link to charge_data fail.\n");
			goto bq28z610_fail_3;
		}
	}

	hwlog_info("bq28z610 probe ok!\n");
	return 0;

bq28z610_fail_3:
	bq28z610_coul_sysfs_remove_group(di);
bq28z610_fail_2:
	ops = NULL;
bq28z610_fail_1:
	kfree(p_batt_data);
	p_batt_data = NULL;
bq28z610_fail_0:
	kfree(di);
	di = NULL;
	return ret;
}

/**********************************************************
*  Function:       bq28z610_remove
*  Discription:    bq28z610 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_remove(struct i2c_client *client)
{
	struct bq25892_device_info *di = i2c_get_clientdata(client);
	bq28z610_coul_sysfs_remove_group(di);

	return 0;
}
/**********************************************************
*  Function:       bq28z610_suspend
*  Discription:    bq28z610 module suspend
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_suspend(struct i2c_client *client)
{
	hwlog_info("bq28z610 suspend ++");
	hwlog_info("bq28z610 suspend --");
	return 0;
}
/**********************************************************
*  Function:       bq28z610_resume
*  Discription:    bq28z610 module resume
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int bq28z610_resume(struct i2c_client *client)
{
	hwlog_info("bq28z610 resume ++");
	hwlog_info("bq28z610 resume --");
	return 0;
}


MODULE_DEVICE_TABLE(i2c, bq28z610);
static struct of_device_id bq28z610_of_match[] = {
	{
	 .compatible = "huawei,bq28z610_coul",
	 .data = NULL,
	 },
	{
	 },
};

static const struct i2c_device_id bq28z610_i2c_id[] = {
	{"bq28z610_coul", 0}, {}
};

static struct i2c_driver bq28z610_driver = {
	.probe = bq28z610_probe,
	.remove = bq28z610_remove,
	.suspend = bq28z610_suspend,
	.resume = bq28z610_resume,
	.id_table = bq28z610_i2c_id,
	.driver = {
		   .owner = THIS_MODULE,
		   .name = "bq28z610_coul",
		   .of_match_table = of_match_ptr(bq28z610_of_match),
		   },
};

/**********************************************************
*  Function:       bq28z610_init
*  Discription:    bq28z610 module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init bq28z610_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&bq28z610_driver);
	if (ret)
		hwlog_err("%s: i2c_add_driver error!!!\n", __func__);

	return ret;
}

/**********************************************************
*  Function:       bq28z610_exit
*  Discription:    bq28z610 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit bq28z610_exit(void)
{
	i2c_del_driver(&bq28z610_driver);
}

rootfs_initcall(bq28z610_init);
module_exit(bq28z610_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("bq28z610 coul module driver");
MODULE_AUTHOR("HW Inc");
