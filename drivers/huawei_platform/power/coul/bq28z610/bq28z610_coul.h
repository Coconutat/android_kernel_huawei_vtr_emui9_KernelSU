#ifndef _BQ28Z610_COUL
#define _BQ28Z610_COUL

#define ABNORMAL_BATT_TEMPERATURE_LOW		(-40)
#define ABNORMAL_BATT_TEMPERATURE_HIGH		(80)
#define DEFAULT_TEMP		(25)
#define TEMP_TOO_HOT		(60)
#define TEMP_TOO_COLD		(-20)


#define TEMP_PARA_LEVEL		(10)
#define VOLT_PARA_LEVEL		(4)
#define SEGMENT_PARA_LEVEL	(2)

#define BATTERY_DEFAULT_CAPACITY		50
#define BATTERY_FULL_CAPACITY			100
#define BATTERY_DEFAULT_MAX_VOLTAGE	4400
#define BATTERY_DEFAULT_VOLTAGE		3800

#define BATTERY_CC_LOW_LEV			3
#define BATTERY_CC_WARNING_LEV		10
#define BATTERY_SWITCH_ON_VOLTAGE	3250
#define BATTERY_VOL_LOW_ERR			2900
#define BATTERY_NORMAL_CUTOFF_VOL	3150

#define CHARGE_STATE_UNKNOW			0
#define CHARGE_STATE_START_CHARGING	1
#define CHARGE_STATE_CHRG_DONE		2
#define CHARGE_STATE_STOP_CHARGING	3
#define CHARGE_STATE_RECHARGE		4
#define CHARGE_STATE_NOT_CHARGING	5

#define I2C_RETRY_CNT			3

#define BQ28Z610_REG_AR			0x02  /*AtRate()*/
#define BQ28Z610_REG_TEMP			0x06  /*Temperature()*/
#define BQ28Z610_REG_VOLT			0x08  /*Voltage()*/
#define BQ28Z610_REG_CURR			0x0C  /*Curent()*/
#define BQ28Z610_REG_RM			0x10 /*RemainingCapacity()*/
#define BQ28Z610_REG_FCC			0x12 /*FullChargeCapacity*/
#define BQ28Z610_REG_AVRGCURR	0x14 /*AverageCurrent*/
#define BQ28Z610_REG_TTE			0x16 /*AverageTimeToEmpty*/
#define BQ28Z610_REG_TTF			0x18 /*AverageTimeToFull*/
#define BQ28Z610_REG_CYCLE			0x2A /*CycleCount*/
#define BQ28Z610_REG_SOC			0x2C /*RelativeStateOfCharge*/
#define BQ28Z610_REG_DC			0x3C /*DesignCapacity*/

#define BQ28Z610_MAC_CMD				0x3E /*AltManufacturerAccess*/
#define BQ28Z610_MAC_DATA				0x40 /*MACData*/
#define BQ28Z610_MAC_DATA_CHECKSUM   0x60 /*MACDataSum*/
#define BQ28Z610_MAC_DATA_LEN        	0x61 /*MACDataLen*/

#define BQ28Z610_MANUINFO			0x0070 /*ManufacturerData*/
#define BQ28Z610_MANUINFO_LEN		32
#define BQ28Z610_DEFAULT_VERSION	0x35 /*Max value of last byte in Manuinformation*/
#define BQ28Z610_VERSION_INDEX		31 /*version index*/
#define BQ28Z610_CELL_NAME			18
#define BQ28Z610_PACK_NAME			19
#define BQ28Z610_PACK_DATE			20 /*pack date of assembly*/
#define BQ28Z610_PACK_DATE_LEN		6
#define BQ28Z610_BATT_BRAND_LEN	30


#define BQ28Z610_REG_SOH				0x2E //StateOfHealth
#define BQ28Z610_SOH_NOT_VALID		0x00
#define BQ28Z610_INSTANT_SOH_READY	0x01
#define BQ28Z610_INITIAL_SOH_READY	0x02
#define BQ28Z610_SOH_READY			0x03


#define BQ28Z610_REG_FLAGS				0x0A //BatteryStatus()

struct bq28z610_reg_cache {
	s16 temp;	//Temperature
	u16 vol;		//Voltage
	s16 curr;		//Curent
	s16 avg_curr;//AverageCurrent
	u16 rm;		//RemainingCapacity
	u16 tte;		//AverageTimeToEmpty
	u16 ttf;		//AverageTimeToFull
	u16 fcc;		//FullChargeCapacity
	u16 dc;		//DesignCapacity
	u16 cycle;	//CycleCount
	u16 soc;		//RelativeStateOfCharge
	u8 soh;		//StateOfHealth
	u16 flags;	//BatteryStatus
};

struct bq28z610_device_info {
	struct i2c_client	*client;
	struct  device		*dev;

	struct  bq28z610_reg_cache cache;
	int charge_status;
	int vbat_max;
};

enum temp_para_info {
	TEMP_PARA_TEMP_MIN = 0,
	TEMP_PARA_TEMP_MAX,
	TEMP_PARA_IIN,
	TEMP_PARA_ICHG,
	TEMP_PARA_VTERM,
	TEMP_PARA_TEMP_BACK,
	TEMP_PARA_TOTAL,
};

enum volt_para_info {
	VOLT_PARA_VOLT_MIN = 0,
	VOLT_PARA_VOLT_MAX,
	VOLT_PARA_IIN,
	VOLT_PARA_ICHG,
	VOLT_PARA_VOLT_BACK,
	VOLT_PARA_TOTAL,
};
enum segment_para_info {
	SEGMENT_PARA_VOLT_MIN = 0,
	SEGMENT_PARA_VOLT_MAX,
	SEGMENT_PARA_ICHG,
	SEGMENT_PARA_VTERM,
	SEGMENT_PARA_VOLT_BACK,
	SEGMENT_PARA_TOTAL,
};

struct chrg_para_lut {
	int temp_len;
	int temp_data[TEMP_PARA_LEVEL][TEMP_PARA_TOTAL];
	int volt_len;
	int volt_data[VOLT_PARA_LEVEL][VOLT_PARA_TOTAL] ;
	int segment_len;
	int segment_data[SEGMENT_PARA_LEVEL][SEGMENT_PARA_TOTAL];
};


enum bq28z610_coul_sysfs_type {
	BQ28Z610_COUL_SYSFS_GAUGELOG_HEAD = 0,
	BQ28Z610_COUL_SYSFS_GAUGELOG,
};

#endif
