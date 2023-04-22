/*
 * Copyright 2018 HUAWEI Tech. Co., Ltd.
 */
#ifndef __INA231_H
#define __INA231_H

/* common register definitions */
#define INA231_CONFIG			  0x00
#define INA231_SHUNT_VOLTAGE	  0x01 /* readonly */
#define INA231_BUS_VOLTAGE	      0x02 /* readonly */
#define INA231_POWER			  0x03 /* readonly */
#define INA231_CURRENT			  0x04 /* readonly */
#define INA231_CALIBRATION		  0x05
#define INA231_MASK_ENABLE	      0x06
#define INA231_ALERT_LIMIT		  0x07

/* register count */
#define INA231_MAX_REGS		8

#define LCD_ON            (1 << 0)
#define ALREADY_READ      (1 << 1)
#define LCD_RESUME        (1 << 2)
#define INA231_OK         0
#define INA231_FALSE      -1
#define INA231_SAMPLE_TIME  3    //every 3s sampling value from ina231
#define INA231_POWER_UNIT_CONVERSION    1000

struct ina231_config {
	u16 config_sleep_in;
	u16 config_reset;
	u16 config_work;
	u16 calibrate_content;
	u16 mask_enable_content;
	u16 alert_limit_content;

	int shunt_lsb;
	int shunt_max;
	int bus_voltage_lsb;	/* uV */
	int bus_voltage_max;

	int current_lsb;	/* uA */
	int current_max;
	int power_lsb;		/* uW */
	int power_max;
};

struct ina231_data {
	struct device *dev;
	struct i2c_client *client;
	const struct ina231_config *config;
	int type;
	u16 regs[INA231_MAX_REGS];
	unsigned int flag;
	int num;
	u64 acc_power;
	u64 acc_current;
	struct mutex mutex_lock;
	struct work_struct wq;
	struct timer_list ina321_timer;
};

#define INA231_EMERG_LEVEL       0
#define INA231_ALERT_LEVEL       1
#define INA231_CRIT_LEVEL        2
#define INA231_ERR_LEVEL         3
#define INA231_WARNING_LEVEL     4
#define INA231_NOTICE_LEVEL      5
#define INA231_INFO_LEVEL        6
#define INA231_DEBUG_LEVEL       7


#define INA231_EMERG(msg, ...)    \
	do { if (ina231_msg_level > INA231_EMERG_LEVEL)  \
		printk(KERN_EMERG "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define INA231_ALERT(msg, ...)    \
	do { if (ina231_msg_level > INA231_ALERT_LEVEL)  \
		printk(KERN_ALERT "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define INA231_CRIT(msg, ...)    \
	do { if (ina231_msg_level > INA231_CRIT_LEVEL)  \
		printk(KERN_CRIT "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define INA231_ERR(msg, ...)    \
	do { if (ina231_msg_level > INA231_ERR_LEVEL)  \
		printk(KERN_ERR "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define INA231_WARNING(msg, ...)    \
	do { if (ina231_msg_level > INA231_WARNING_LEVEL)  \
		printk(KERN_WARNING "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define INA231_NOTICE(msg, ...)    \
	do { if (ina231_msg_level > INA231_NOTICE_LEVEL)  \
		printk(KERN_NOTICE "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define INA231_INFO(msg, ...)    \
	do { if (ina231_msg_level > INA231_INFO_LEVEL)  \
		printk(KERN_INFO "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)
#define INA231_DEBUG(msg, ...)    \
	do { if (ina231_msg_level > INA231_DEBUG_LEVEL)  \
		printk(KERN_DEBUG "[ina231]%s: "msg, __func__, ## __VA_ARGS__); } while (0)


int ina231_power_monitor_on(void);
int ina231_power_monitor_off(void);


#endif
