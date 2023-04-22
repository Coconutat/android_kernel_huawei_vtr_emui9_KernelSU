#ifndef _H_FSA7830_H_
#define _H_FSA7830_H_

#include <linux/uaccess.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/sched.h>
#include <linux/spinlock_types.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/jiffies.h>
#include <linux/workqueue.h>
#include <linux/err.h>
#include <linux/mutex.h>
#include <linux/clk.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#define FSA7830_CONFIG_COUNT			8
#define FSA7830_CHIP_TYPES			4
#define FSA7830_CHIP_INFO_MAXLEN		24
#define FSA7830_DEVNAME_MAXLEN		(FSA7830_CHIP_INFO_MAXLEN+8)

#define HUAWEI_VOLTAGE_COMPATIBLE		"huawei,fsa7830"

#define FSA7830_HUAWEI_ADC			"huawei,adc_channel"
#define FSA7830_HUAWEI_ID			"huawei,id"
#define FSA7830_HUAWEI_ID_REG			"huawei,chip_id_reg"
#define FSA7830_HUAWEI_ID_VAL			"huawei,chip_id_val"
#define FSA7830_HUAWEI_INFO			"huawei,chip_info"
#define FSA7830_HUAWEI_CHIP_FLAG		"huawei,chip_total_flag"
#define FSA7830_HUAWEI_INPUT_CFG		"huawei,input_cfg"
#define FSA7830_HUAWEI_IRQ_CFG		"huawei,irq_support"
#define FSA7830_HUAWEI_IRQ			"huawei,gpio_irq"
#define FSA7830_HUAWEI_MAIN_CHANNEL		"huawei,main_channel_count"
#define FSA7830_HUAWEI_SEC_CHANNEL		"huawei,sec_channel_count"

#define FSA7830_HUAWEI_MAX_THRESHOLD		"huawei,max_threshold"
#define FSA7830_HUAWEI_MIN_THRESHOLD		"huawei,min_threshold"
#define FSA7830_HUAWEI_OVP_THRESHOLD		"huawei,ovp_threshold"
#define FSA7830_HUAWEI_OVP_ACTION		"huawei,ovp_action"
#define FSA7830_HUAWEI_SWITCH_CFG		"huawei,switch_cfg"

enum fsa8730_action {
	FSA8730_ACTION_ENABLE = 0,
	FSA8730_ACTION_SWCFG,
	FSA8730_ACTION_IRQ_ENABLE,
	FSA8730_ACTION_OVP_VALUE,
	FSA8730_ACTION_UNDEFINE
};

struct fsa830_chip_data {
	int adc_channel;
	int chip_flag;
	int chip_type_counts;
	int id;
	int id_reg;
	int irq_support;
	int input_cfg;
	int main_channel_count;
	int sec_channel_count;

	unsigned int irq_gpio;
	char chip_info[FSA7830_CHIP_INFO_MAXLEN];
	int id_val[FSA7830_CHIP_TYPES];
	int max_threshold[FSA7830_CONFIG_COUNT];
	int min_threshold[FSA7830_CONFIG_COUNT];
	int ovp_threshold[FSA7830_CONFIG_COUNT];
	int ovp_action[FSA7830_CONFIG_COUNT];
	int sw_cfg[FSA7830_CONFIG_COUNT];
};

struct fsa7830_data {
	char reg_buf;	/* debug reg buf */
	int chip_enable;	/* enable disable */
	int current_adc_channel;
	int current_input_channel;
	int volt;
	int irq;
	char dev_name[FSA7830_DEVNAME_MAXLEN];
	struct hw_voltage_data *vol_data;
	struct i2c_client *client;
	struct device *dev;
	struct miscdevice fsa7830_dev;
	spinlock_t fsa7830_lock;
	struct list_head list;
	struct work_struct work;
	struct fsa830_chip_data chip_data;
	struct pinctrl *pctrl;
	struct pinctrl_state *pins_default;
	struct pinctrl_state *pins_idle;
};
#endif
