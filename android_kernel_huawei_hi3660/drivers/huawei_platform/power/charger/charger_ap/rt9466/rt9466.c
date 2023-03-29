/*
 *  Driver for Richtek RT9466 Charger
 *
 *  Copyright (C) 2017 Richtek Technology Corp.
 *  shufan_lee <shufan_lee@richtek.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/of_device.h>
#include <linux/of_irq.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/delay.h>
#include <linux/mutex.h>
#include <linux/version.h>
#include <linux/workqueue.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/pm_runtime.h>
#include <huawei_platform/log/hw_log.h>
#include <huawei_platform/power/huawei_charger.h>
#include <../charging_core.h>
#include "rt9466.h"
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
#include <huawei_platform/power/usb_short_circuit_protect.h>
#endif

#ifdef CONFIG_RT_REGMAP
#include <huawei_platform/usb/pd/richtek/rt-regmap.h>
#endif /* CONFIG_RT_REGMAP */

#define I2C_ACCESS_MAX_RETRY	5
#define RT9466_DRV_VERSION	"1.0.2_HUAWEI"
#define RT9466_TDEG_EOC_16MS	0x7
#define RT9466_SW_RESET_NO_HZ	0x10
#define RT9466_SW_RESET_HZ	0x14
#define VBUS_THRESHOLD          3800
#define VBUS_POST_THRESHOLD     1000
#define RT9466_BOOST_VOTGBST    5000
#define AICR_LOWER_BOUND        150

static int huawei_force_hz = 0; /* for huawei force enable hz */
static struct i2c_client *g_rt9466_i2c;
static int hiz_iin_limit_flag = HIZ_IIN_FLAG_FALSE;
/* ======================= */
/* RT9466 Parameter        */
/* ======================= */

static const u32 rt9466_boost_oc_threshold[] = {
	500, 700, 1100, 1300, 1800, 2100, 2400, 3000,
}; /* uA */

static const u32 rt9466_safety_timer[] = {
	4, 6, 8, 10, 12, 14, 16, 20,
}; /* hour */

static const u32 rt9466_wdt[] = {
	8, 40, 80, 160,
}; /* s */

/* Register 0x01 ~ 0x10 */
static u8 reset_reg_data[] = {
		0x10, 0x03, 0x23, 0x3C, 0x67, 0x0B, 0x4C, 0xA1,
		0x3C, 0x58, 0x2C, 0x82, 0x52, 0x05, 0x00, 0x10
};

enum rt9466_irq_idx {
	RT9466_IRQIDX_CHG_STATC = 0,
	RT9466_IRQIDX_CHG_FAULT,
	RT9466_IRQIDX_TS_STATC,
	RT9466_IRQSTAT_MAX,
	RT9466_IRQIDX_CHG_IRQ1 = RT9466_IRQSTAT_MAX,
	RT9466_IRQIDX_CHG_IRQ2,
	RT9466_IRQIDX_CHG_IRQ3,
	RT9466_IRQIDX_MAX,
};

static const u8 rt9466_irq_maskall[RT9466_IRQIDX_MAX] = {
	0xF0, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF
};

enum rt9466_charging_status {
	RT9466_CHG_STATUS_READY = 0,
	RT9466_CHG_STATUS_PROGRESS,
	RT9466_CHG_STATUS_DONE,
	RT9466_CHG_STATUS_FAULT,
	RT9466_CHG_STATUS_MAX,
};

static const char *rt9466_chg_status_name[RT9466_CHG_STATUS_MAX] = {
	"ready", "progress", "done", "fault",
};

static const u8 rt9466_val_en_hidden_mode[] = {
	0x49, 0x32, 0xB6, 0x27, 0x48, 0x18, 0x03, 0xE2,
};

enum rt9466_iin_limit_sel {
	RT9466_IIMLMTSEL_PSEL_OTG,
	RT9466_IINLMTSEL_AICR = 2,
	RT9466_IINLMTSEL_LOWER_LEVEL, /* lower of above two */
};

enum rt9466_adc_sel {
	RT9466_ADC_VBUS_DIV5 = 1,
	RT9466_ADC_VBUS_DIV2,
	RT9466_ADC_VSYS,
	RT9466_ADC_VBAT,
	RT9466_ADC_TS_BAT = 6,
	RT9466_ADC_IBUS = 8,
	RT9466_ADC_IBAT,
	RT9466_ADC_REGN = 11,
	RT9466_ADC_TEMP_JC,
	RT9466_ADC_MAX,
};

/*
 * Unit for each ADC parameter
 * 0 stands for reserved
 * For TS_BAT, the real unit is 0.25.
 * Here we use 25, please remember to divide 100 while showing the value
 */
static const int rt9466_adc_unit[RT9466_ADC_MAX] = {
	0,
	RT9466_ADC_UNIT_VBUS_DIV5,
	RT9466_ADC_UNIT_VBUS_DIV2,
	RT9466_ADC_UNIT_VSYS,
	RT9466_ADC_UNIT_VBAT,
	0,
	RT9466_ADC_UNIT_TS_BAT,
	0,
	RT9466_ADC_UNIT_IBUS,
	RT9466_ADC_UNIT_IBAT,
	0,
	RT9466_ADC_UNIT_REGN,
	RT9466_ADC_UNIT_TEMP_JC,
};

static const int rt9466_adc_offset[RT9466_ADC_MAX] = {
	0,
	RT9466_ADC_OFFSET_VBUS_DIV5,
	RT9466_ADC_OFFSET_VBUS_DIV2,
	RT9466_ADC_OFFSET_VSYS,
	RT9466_ADC_OFFSET_VBAT,
	0,
	RT9466_ADC_OFFSET_TS_BAT,
	0,
	RT9466_ADC_OFFSET_IBUS,
	RT9466_ADC_OFFSET_IBAT,
	0,
	RT9466_ADC_OFFSET_REGN,
	RT9466_ADC_OFFSET_TEMP_JC,
};

struct rt9466_desc {
	u32 ichg;	/* uA */
	u32 aicr;	/* uA */
	u32 mivr;	/* uV */
	u32 cv;		/* uV */
	u32 ieoc;	/* uA */
	u32 safety_timer;	/* hour */
	u32 ircmp_resistor;	/* uohm */
	u32 ircmp_vclamp;	/* uV */
	bool en_te;
	bool en_wdt;
	bool en_irq_pulse;
	bool en_jeita;
	int regmap_represent_slave_addr;
	const char *regmap_name;
	const char *chg_name;
	bool ceb_invert;
	int hiz_iin_limit;
};

/* These default values will be applied if there's no property in dts */
static struct rt9466_desc rt9466_default_desc = {
	.ichg = 2000,	/* mA */
	.aicr = 500,	/* mA */
	.mivr = 4400,	/* mV */
	.cv = 4350,	/* mA */
	.ieoc = 250,	/* mA */
	.safety_timer = 12,
	.ircmp_resistor = 25,	/* mohm */
	.ircmp_vclamp = 32,	/* mV */
	.en_te = true,
	.en_wdt = true,
	.en_irq_pulse = false,
	.en_jeita = false,
	.regmap_represent_slave_addr = RT9466_SLAVE_ADDR,
	.regmap_name = "rt9466",
	.chg_name = "primary_chg",
	.ceb_invert = false,
};

struct rt9466_info {
	struct i2c_client *client;
	struct mutex io_lock;
	struct mutex adc_lock;
	struct mutex irq_lock;
	struct mutex aicr_lock;
	struct mutex ichg_lock;
	struct mutex ieoc_lock;
	struct mutex hidden_mode_lock;
	struct rt9466_desc *desc;
	struct device *dev;
	wait_queue_head_t wait_queue;
	int irq;
	int aicr_limit;
	u32 intr_gpio;
	u32 ceb_gpio;
	u8 chip_rev;
	u8 irq_flag[RT9466_IRQIDX_MAX];
	u8 irq_stat[RT9466_IRQSTAT_MAX];
	u8 irq_mask[RT9466_IRQIDX_MAX];
	u32 hidden_mode_cnt;
	u32 ieoc;
	bool ieoc_wkard;
	struct work_struct init_work;
	struct work_struct irq_work;
	struct work_struct aicr_work;
	struct delayed_work mivr_unmask_dwork;
	atomic_t pwr_rdy;
#ifdef CONFIG_RT_REGMAP
	struct rt_regmap_device *regmap_dev;
	struct rt_regmap_properties *regmap_prop;
#endif /* CONFIG_RT_REGMAP */
	struct charge_core_data *core_data;
	bool irq_active;
};

/* ======================= */
/* Register Address        */
/* ======================= */

static const unsigned char rt9466_reg_addr[] = {
	RT9466_REG_CORE_CTRL0,
	RT9466_REG_CHG_CTRL1,
	RT9466_REG_CHG_CTRL2,
	RT9466_REG_CHG_CTRL3,
	RT9466_REG_CHG_CTRL4,
	RT9466_REG_CHG_CTRL5,
	RT9466_REG_CHG_CTRL6,
	RT9466_REG_CHG_CTRL7,
	RT9466_REG_CHG_CTRL8,
	RT9466_REG_CHG_CTRL9,
	RT9466_REG_CHG_CTRL10,
	RT9466_REG_CHG_CTRL11,
	RT9466_REG_CHG_CTRL12,
	RT9466_REG_CHG_CTRL13,
	RT9466_REG_CHG_CTRL14,
	RT9466_REG_CHG_CTRL15,
	RT9466_REG_CHG_CTRL16,
	RT9466_REG_CHG_ADC,
	RT9466_REG_CHG_CTRL19,
	RT9466_REG_CHG_CTRL17,
	RT9466_REG_CHG_CTRL18,
	RT9466_REG_DEVICE_ID,
	RT9466_REG_CHG_STAT,
	RT9466_REG_CHG_NTC,
	RT9466_REG_ADC_DATA_H,
	RT9466_REG_ADC_DATA_L,
	RT9466_REG_CHG_STATC,
	RT9466_REG_CHG_FAULT,
	RT9466_REG_TS_STATC,
	/* Skip IRQ evt to prevent reading clear while dumping registers */
	RT9466_REG_CHG_STATC_CTRL,
	RT9466_REG_CHG_FAULT_CTRL,
	RT9466_REG_TS_STATC_CTRL,
	RT9466_REG_CHG_IRQ1_CTRL,
	RT9466_REG_CHG_IRQ2_CTRL,
	RT9466_REG_CHG_IRQ3_CTRL,
};

/* ========= */
/* RT Regmap */
/* ========= */

#ifdef CONFIG_RT_REGMAP
RT_REG_DECL(RT9466_REG_CORE_CTRL0, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL4, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL5, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL7, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL8, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL9, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL10, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL11, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL12, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL13, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL14, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL15, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL16, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_ADC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL19, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL17, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_CTRL18, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL4, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL6, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL7, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL8, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL9, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_HIDDEN_CTRL15, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_DEVICE_ID, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_STAT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_NTC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_ADC_DATA_H, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_ADC_DATA_L, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_STATC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_FAULT, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_TS_STATC, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ1, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ2, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ3, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_STATC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_FAULT_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_TS_STATC_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ1_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ2_CTRL, 1, RT_VOLATILE, {});
RT_REG_DECL(RT9466_REG_CHG_IRQ3_CTRL, 1, RT_VOLATILE, {});

static const rt_register_map_t rt9466_regmap_map[] = {
	RT_REG(RT9466_REG_CORE_CTRL0),
	RT_REG(RT9466_REG_CHG_CTRL1),
	RT_REG(RT9466_REG_CHG_CTRL2),
	RT_REG(RT9466_REG_CHG_CTRL3),
	RT_REG(RT9466_REG_CHG_CTRL4),
	RT_REG(RT9466_REG_CHG_CTRL5),
	RT_REG(RT9466_REG_CHG_CTRL6),
	RT_REG(RT9466_REG_CHG_CTRL7),
	RT_REG(RT9466_REG_CHG_CTRL8),
	RT_REG(RT9466_REG_CHG_CTRL9),
	RT_REG(RT9466_REG_CHG_CTRL10),
	RT_REG(RT9466_REG_CHG_CTRL11),
	RT_REG(RT9466_REG_CHG_CTRL12),
	RT_REG(RT9466_REG_CHG_CTRL13),
	RT_REG(RT9466_REG_CHG_CTRL14),
	RT_REG(RT9466_REG_CHG_CTRL15),
	RT_REG(RT9466_REG_CHG_CTRL16),
	RT_REG(RT9466_REG_CHG_ADC),
	RT_REG(RT9466_REG_CHG_CTRL19),
	RT_REG(RT9466_REG_CHG_CTRL17),
	RT_REG(RT9466_REG_CHG_CTRL18),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL1),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL2),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL4),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL6),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL7),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL8),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL9),
	RT_REG(RT9466_REG_CHG_HIDDEN_CTRL15),
	RT_REG(RT9466_REG_DEVICE_ID),
	RT_REG(RT9466_REG_CHG_STAT),
	RT_REG(RT9466_REG_CHG_NTC),
	RT_REG(RT9466_REG_ADC_DATA_H),
	RT_REG(RT9466_REG_ADC_DATA_L),
	RT_REG(RT9466_REG_CHG_STATC),
	RT_REG(RT9466_REG_CHG_FAULT),
	RT_REG(RT9466_REG_TS_STATC),
	RT_REG(RT9466_REG_CHG_IRQ1),
	RT_REG(RT9466_REG_CHG_IRQ2),
	RT_REG(RT9466_REG_CHG_IRQ3),
	RT_REG(RT9466_REG_CHG_STATC_CTRL),
	RT_REG(RT9466_REG_CHG_FAULT_CTRL),
	RT_REG(RT9466_REG_TS_STATC_CTRL),
	RT_REG(RT9466_REG_CHG_IRQ1_CTRL),
	RT_REG(RT9466_REG_CHG_IRQ2_CTRL),
	RT_REG(RT9466_REG_CHG_IRQ3_CTRL),
};
#endif /* CONFIG_RT_REGMAP */

/*function declare here*/
static int rt9466_set_aicr(int aicr);

/* ========================= */
/* I2C operations            */
/* ========================= */
static int rt9466_device_read(void *client, u32 addr, int leng, void *dst)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_read_i2c_block_data(i2c, addr, leng, dst);
}

static int rt9466_device_write(void *client, u32 addr, int leng,
	const void *src)
{
	struct i2c_client *i2c = (struct i2c_client *)client;

	return i2c_smbus_write_i2c_block_data(i2c, addr, leng, src);
}

#ifdef CONFIG_RT_REGMAP
static struct rt_regmap_fops rt9466_regmap_fops = {
	.read_device = rt9466_device_read,
	.write_device = rt9466_device_write,
};

static int rt9466_register_rt_regmap(struct rt9466_info *info)
{
	int ret = 0;
	struct i2c_client *client = info->client;
	struct rt_regmap_properties *prop = NULL;

	dev_info(info->dev, "%s\n", __func__);

	prop = devm_kzalloc(&client->dev, sizeof(struct rt_regmap_properties),
		GFP_KERNEL);
	if (!prop)
		return -ENOMEM;

	prop->name = info->desc->regmap_name;
	prop->aliases = info->desc->regmap_name;
	prop->register_num = ARRAY_SIZE(rt9466_regmap_map);
	prop->rm = rt9466_regmap_map;
	prop->rt_regmap_mode = RT_SINGLE_BYTE | RT_CACHE_DISABLE |
		RT_IO_PASS_THROUGH;
	prop->io_log_en = 0;

	info->regmap_prop = prop;
	info->regmap_dev = rt_regmap_device_register_ex(info->regmap_prop,
		&rt9466_regmap_fops, &client->dev, client,
		info->desc->regmap_represent_slave_addr, info);
	if (!info->regmap_dev) {
		dev_err(info->dev, "%s: register regmap dev fail\n", __func__);
		return -EIO;
	}

	return ret;
}
#endif /* CONFIG_RT_REGMAP */

static inline int __rt9466_i2c_write_byte(struct rt9466_info *info, u8 cmd,
	u8 data)
{
	int ret = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_write(info->regmap_dev, cmd, 1, &data);
#else
		ret = rt9466_device_write(info->client, cmd, 1, &data);
#endif /* CONFIG_RT_REGMAP */
		retry++;
		if (ret < 0)
			udelay(10);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0)
		dev_err(info->dev, "%s: I2CW[0x%02X] = 0x%02X fail\n",
			__func__, cmd, data);
	else
		dev_dbg(info->dev, "%s: I2CW[0x%02X] = 0x%02X\n", __func__,
			cmd, data);

	return ret;
}

static int rt9466_i2c_write_byte(struct rt9466_info *info, u8 cmd, u8 data)
{
	int ret = 0;

	mutex_lock(&info->io_lock);
	ret = __rt9466_i2c_write_byte(info, cmd, data);
	mutex_unlock(&info->io_lock);

	return ret;
}

static inline int __rt9466_i2c_read_byte(struct rt9466_info *info, u8 cmd)
{
	int ret = 0, ret_val = 0, retry = 0;

	do {
#ifdef CONFIG_RT_REGMAP
		ret = rt_regmap_block_read(info->regmap_dev, cmd, 1, &ret_val);
#else
		ret = rt9466_device_read(info->client, cmd, 1, &ret_val);
#endif /* CONFIG_RT_REGMAP */
		retry++;
		if (ret < 0)
			udelay(10);
	} while (ret < 0 && retry < I2C_ACCESS_MAX_RETRY);

	if (ret < 0) {
		dev_err(info->dev, "%s: I2CR[0x%02X] fail\n", __func__, cmd);
		return ret;
	}

	ret_val = ret_val & 0xFF;

	dev_dbg(info->dev, "%s: I2CR[0x%02X] = 0x%02X\n", __func__, cmd,
		ret_val);

	return ret_val;
}

static int rt9466_i2c_read_byte(struct rt9466_info *info, u8 cmd)
{
	int ret = 0;

	mutex_lock(&info->io_lock);
	ret = __rt9466_i2c_read_byte(info, cmd);
	mutex_unlock(&info->io_lock);

	if (ret < 0)
		return ret;

	return (ret & 0xFF);
}

static inline int __rt9466_i2c_block_write(struct rt9466_info *info, u8 cmd,
	u32 leng, const u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_write(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9466_device_write(info->client, cmd, leng, data);
#endif /* CONFIG_RT_REGMAP */

	return ret;
}


static int rt9466_i2c_block_write(struct rt9466_info *info, u8 cmd, u32 leng,
	const u8 *data)
{
	int ret = 0;

	mutex_lock(&info->io_lock);
	ret = __rt9466_i2c_block_write(info, cmd, leng, data);
	mutex_unlock(&info->io_lock);

	return ret;
}

static inline int __rt9466_i2c_block_read(struct rt9466_info *info, u8 cmd,
	u32 leng, u8 *data)
{
	int ret = 0;

#ifdef CONFIG_RT_REGMAP
	ret = rt_regmap_block_read(info->regmap_dev, cmd, leng, data);
#else
	ret = rt9466_device_read(info->client, cmd, leng, data);
#endif /* CONFIG_RT_REGMAP */

	return ret;
}


static int rt9466_i2c_block_read(struct rt9466_info *info, u8 cmd, u32 leng,
	u8 *data)
{
	int ret = 0;

	mutex_lock(&info->io_lock);
	ret = __rt9466_i2c_block_read(info, cmd, leng, data);
	mutex_unlock(&info->io_lock);

	return ret;
}

static int rt9466_i2c_test_bit(struct rt9466_info *info, u8 cmd, u8 shift,
	bool *is_one)
{
	int ret = 0;
	u8 data = 0;

	ret = rt9466_i2c_read_byte(info, cmd);
	if (ret < 0) {
		*is_one = false;
		return ret;
	}

	data = ret & (1 << shift);
	*is_one = (data == 0 ? false : true);

	return ret;
}

static int rt9466_i2c_update_bits(struct rt9466_info *info, u8 cmd, u8 data,
	u8 mask)
{
	int ret = 0;
	u8 reg_data = 0;

	mutex_lock(&info->io_lock);
	ret = __rt9466_i2c_read_byte(info, cmd);
	if (ret < 0) {
		mutex_unlock(&info->io_lock);
		return ret;
	}

	reg_data = ret & 0xFF;
	reg_data &= ~mask;
	reg_data |= (data & mask);

	ret = __rt9466_i2c_write_byte(info, cmd, reg_data);
	mutex_unlock(&info->io_lock);

	return ret;
}

static inline int rt9466_set_bit(struct rt9466_info *info, u8 reg, u8 mask)
{
	return rt9466_i2c_update_bits(info, reg, mask, mask);
}

static inline int rt9466_clr_bit(struct rt9466_info *info, u8 reg, u8 mask)
{
	return rt9466_i2c_update_bits(info, reg, 0x00, mask);
}

/* ================== */
/* Internal Functions */
/* ================== */
static int rt9466_enable_hz(int en);
static int rt9466_kick_wdt(void);
static int rt9466_init_setting(struct rt9466_info *info);
static inline int rt9466_irq_init(struct rt9466_info *info);

static inline void rt9466_irq_set_flag(struct rt9466_info *info, u8 *irq,
	u8 mask)
{
	mutex_lock(&info->irq_lock);
	*irq |= mask;
	mutex_unlock(&info->irq_lock);
}

static inline void rt9466_irq_clr_flag(struct rt9466_info *info, u8 *irq,
	u8 mask)
{
	mutex_lock(&info->irq_lock);
	*irq &= ~mask;
	mutex_unlock(&info->irq_lock);
}

static inline u8 rt9466_closest_reg(u32 min, u32 max, u32 step, u32 target)
{
	if (target < min)
		return 0;

	if (target >= max)
		return (max - min) / step;

	return (target - min) / step;
}

static inline u8 rt9466_closest_reg_via_tbl(const u32 *tbl, u32 tbl_size,
	u32 target)
{
	u32 i = 0;

	if (target < tbl[0])
		return 0;

	for (i = 0; i < tbl_size - 1; i++) {
		if (target >= tbl[i] && target < tbl[i + 1])
			return i;
	}

	return tbl_size - 1;
}

static inline u32 rt9466_closest_value(u32 min, u32 max, u32 step, u8 reg_val)
{
	u32 ret_val = 0;

	ret_val = min + reg_val * step;
	if (ret_val > max)
		ret_val = max;

	return ret_val;
}

static int rt9466_get_aicr(struct rt9466_info *info, u32 *aicr)
{
	int ret = 0;
	u8 reg_aicr = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL3);
	if (ret < 0)
		return ret;

	reg_aicr = (ret & RT9466_MASK_AICR) >> RT9466_SHIFT_AICR;
	*aicr = rt9466_closest_value(RT9466_AICR_MIN, RT9466_AICR_MAX,
		RT9466_AICR_STEP, reg_aicr);

	return ret;
}

static int rt9466_get_ichg(struct rt9466_info *info, u32 *ichg)
{
	int ret = 0;
	u8 reg_ichg = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL7);
	if (ret < 0)
		return ret;

	reg_ichg = (ret & RT9466_MASK_ICHG) >> RT9466_SHIFT_ICHG;
	*ichg = rt9466_closest_value(RT9466_ICHG_MIN, RT9466_ICHG_MAX,
		RT9466_ICHG_STEP, reg_ichg);

	return ret;
}

static int rt9466_get_mivr(struct rt9466_info *info, u32 *mivr)
{
	int ret = 0;
	u8 reg_mivr = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL6);
	if (ret < 0)
		return ret;
	reg_mivr = ((ret & RT9466_MASK_MIVR) >> RT9466_SHIFT_MIVR) & 0xFF;

	*mivr = rt9466_closest_value(RT9466_MIVR_MIN, RT9466_MIVR_MAX,
		RT9466_MIVR_STEP, reg_mivr);

	return ret;
}

static int rt9466_get_ieoc(struct rt9466_info *info, u32 *ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL9);
	if (ret < 0)
		return ret;

	reg_ieoc = (ret & RT9466_MASK_IEOC) >> RT9466_SHIFT_IEOC;
	*ieoc = rt9466_closest_value(RT9466_IEOC_MIN, RT9466_IEOC_MAX,
		RT9466_IEOC_STEP, reg_ieoc);

	return ret;
}

static int rt9466_get_adc(struct rt9466_info *info,
	enum rt9466_adc_sel adc_sel, int *adc_val)
{
	int ret = 0, i = 0;
	const int max_wait_times = 6;
	u8 adc_data[2] = {0, 0};
	u32 aicr = 0, ichg = 0;
	bool adc_start = false;

	mutex_lock(&info->adc_lock);

	/* Select ADC to desired channel */
	ret = rt9466_i2c_update_bits(info, RT9466_REG_CHG_ADC,
		adc_sel << RT9466_SHIFT_ADC_IN_SEL, RT9466_MASK_ADC_IN_SEL);
	if (ret < 0) {
		dev_err(info->dev, "%s: select ch to %d fail(%d)\n", __func__,
			adc_sel, ret);
		goto out;
	}

	/* Coefficient for IBUS & IBAT */
	if (adc_sel == RT9466_ADC_IBUS) {
		mutex_lock(&info->aicr_lock);
		ret = rt9466_get_aicr(info, &aicr);
		if (ret < 0) {
			dev_err(info->dev, "%s: get aicr fail\n", __func__);
			goto out_unlock_all;
		}
	} else if (adc_sel == RT9466_ADC_IBAT) {
		mutex_lock(&info->ichg_lock);
		ret = rt9466_get_ichg(info, &ichg);
		if (ret < 0) {
			dev_err(info->dev, "%s: get ichg fail\n", __func__);
			goto out_unlock_all;
		}
	}

	/* Start ADC conversation */
	ret = rt9466_set_bit(info, RT9466_REG_CHG_ADC, RT9466_MASK_ADC_START);
	if (ret < 0) {
		dev_err(info->dev, "%s: start con fail(%d), sel = %d\n",
			__func__, ret, adc_sel);
		goto out_unlock_all;
	}

	for (i = 0; i < max_wait_times; i++) {
		msleep(35);
		ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_ADC,
			RT9466_SHIFT_ADC_START, &adc_start);
		if (ret >= 0 && !adc_start)
			break;
	}
	if (i == max_wait_times) {
		dev_err(info->dev, "%s: wait con fail(%d), sel = %d\n",
			__func__, ret, adc_sel);
		ret = -EINVAL;
		goto out_unlock_all;
	}

	msleep(1);

	/* Read ADC data high/low byte */
	ret = rt9466_i2c_block_read(info, RT9466_REG_ADC_DATA_H, 2, adc_data);
	if (ret < 0) {
		dev_err(info->dev, "%s: read ADC data fail\n", __func__);
		goto out_unlock_all;
	}

	/* Calculate ADC value */
	*adc_val = ((adc_data[0] << 8) + adc_data[1]) * rt9466_adc_unit[adc_sel]
		+ rt9466_adc_offset[adc_sel];

	dev_dbg(info->dev,
		"%s: adc_sel = %d, adc_h = 0x%02X, adc_l = 0x%02X, val = %d\n",
		__func__, adc_sel, adc_data[0], adc_data[1], *adc_val);

	ret = 0;

out_unlock_all:
	/* Coefficient of IBUS & IBAT */
	if (adc_sel == RT9466_ADC_IBUS) {
		if (aicr < 400) /* 400mA */
			*adc_val = *adc_val * 67 / 100;
		mutex_unlock(&info->aicr_lock);
	} else if (adc_sel == RT9466_ADC_IBAT) {
		if (ichg >= 100 && ichg <= 450) /* 100~450mA */
			*adc_val = *adc_val * 57 / 100;
		else if (ichg >= 500 && ichg <= 850) /* 500~850mA */
			*adc_val = *adc_val * 63 / 100;
		mutex_unlock(&info->ichg_lock);
	}

out:
	mutex_unlock(&info->adc_lock);
	return ret;
}

static int rt9466_is_charging_enable(struct rt9466_info *info, bool *en)
{
	return rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL2,
		RT9466_SHIFT_CHG_EN, en);
}

static inline int rt9466_enable_ilim(struct rt9466_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL3, RT9466_MASK_ILIM_EN);
}
/* dont use aicl
static int rt9466_set_aicl_vth(struct rt9466_info *info, u32 aicl_vth)
{
	u8 reg_aicl_vth = 0;

	reg_aicl_vth = rt9466_closest_reg(RT9466_AICL_VTH_MIN,
		RT9466_AICL_VTH_MAX, RT9466_AICL_VTH_STEP, aicl_vth);

	dev_info(info->dev, "%s: vth = %d(0x%02X)\n", __func__, aicl_vth,
		reg_aicl_vth);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL14,
		reg_aicl_vth << RT9466_SHIFT_AICL_VTH, RT9466_MASK_AICL_VTH);
}
*/
static int __rt9466_set_aicr(struct rt9466_info *info, u32 aicr)
{
	u8 reg_aicr = 0;

	reg_aicr = rt9466_closest_reg(RT9466_AICR_MIN, RT9466_AICR_MAX,
		RT9466_AICR_STEP, aicr);

	dev_info(info->dev, "%s: aicr = %d(0x%02X)\n", __func__, aicr,
		reg_aicr);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL3,
		reg_aicr << RT9466_SHIFT_AICR, RT9466_MASK_AICR);
}

static int __rt9466_run_aicr(struct rt9466_info *info)
{
	int ret = 0;
	u32 aicr = 0;
	bool mivr_act = false;

	/* Check whether MIVR loop is active */
	ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_STATC,
		RT9466_SHIFT_CHG_MIVR, &mivr_act);
	if (ret < 0) {
		dev_err(info->dev, "%s: read mivr stat fail\n", __func__);
		goto out;
	}

	if (!mivr_act) {
		dev_info(info->dev, "%s: mivr loop is not active\n", __func__);
		goto out;
	}

	mutex_lock(&info->aicr_lock);
	ret = rt9466_get_aicr(info, &aicr);
	if (ret < 0)
		goto unlock_out;

	aicr -= RT9466_AICR_STEP;
	if (aicr < AICR_LOWER_BOUND)
		aicr = AICR_LOWER_BOUND;
	ret = __rt9466_set_aicr(info, aicr);
	if (ret < 0)
		goto unlock_out;

	info->aicr_limit = aicr;
	dev_dbg(info->dev, "%s: OK, aicr upper bound = %dmA\n", __func__,
				info->aicr_limit);

unlock_out:
	mutex_unlock(&info->aicr_lock);
out:
	return ret;
}

static void rt9466_mivr_unmask_dwork_handler(struct work_struct *work)
{
	int ret = 0;
	struct delayed_work *dwork = to_delayed_work(work);
	struct rt9466_info *info = (struct rt9466_info *)container_of(dwork,
		struct rt9466_info, mivr_unmask_dwork);

	/* Enable MIVR IRQ */
	ret = rt9466_clr_bit(info, RT9466_REG_CHG_STATC_CTRL,
				     RT9466_MASK_CHG_MIVR);
	if (ret < 0)
		dev_err(info->dev, "%s: en MIVR IRQ failed\n", __func__);
}

static int rt9466_toggle_cfo(struct rt9466_info *info)
{
	int ret = 0;
	u8 data = 0;

	dev_info(info->dev, "%s\n", __func__);
	mutex_lock(&info->io_lock);
	ret = rt9466_device_read(info->client, RT9466_REG_CHG_CTRL2, 1, &data);
	if (ret < 0) {
		dev_err(info->dev, "%s read cfo fail(%d)\n", __func__, ret);
		goto out;
	}

	/* CFO off */
	data &= ~RT9466_MASK_CFO_EN;
	ret = rt9466_device_write(info->client, RT9466_REG_CHG_CTRL2, 1, &data);
	if (ret < 0) {
		dev_err(info->dev, "%s cfo off fail(%d)\n", __func__, ret);
		goto out;
	}

	/* CFO on */
	data |= RT9466_MASK_CFO_EN;
	ret = rt9466_device_write(info->client, RT9466_REG_CHG_CTRL2, 1, &data);
	if (ret < 0)
		dev_err(info->dev, "%s cfo on fail(%d)\n", __func__, ret);

out:
	mutex_unlock(&info->io_lock);
	return ret;
}

/* IRQ handlers */
static int rt9466_reserved_irq_handler(struct rt9466_info *info)
{
	return 0;
}

static int rt9466_pwr_rdy_irq_handler(struct rt9466_info *info)
{
	int ret = 0;
	bool pwr_rdy = false;

	dev_err(info->dev, "%s\n", __func__);
	ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_STATC,
		RT9466_SHIFT_PWR_RDY, &pwr_rdy);
	if (ret < 0)
		return ret;

	ret = rt9466_enable_hz(!pwr_rdy);
	if (ret < 0) {
		dev_err(info->dev, "%s enable_hz fail\n", __func__);
		return ret;
	}
	/* reset aicl_limit */
	if (pwr_rdy)
		info->aicr_limit = -1;

	atomic_set(&info->pwr_rdy, pwr_rdy);
	return 0;
}

static int rt9466_chg_mivr_irq_handler(struct rt9466_info *info)
{
	int ret = 0;
	bool mivr_act = false;
	int adc_ibus = 0;

	dev_err(info->dev, "%s\n", __func__);

	/* Disable MIVR IRQ */
	ret = rt9466_set_bit(info, RT9466_REG_CHG_STATC_CTRL,
				RT9466_MASK_CHG_MIVR);

	/* Check whether MIVR loop is active */
	ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_STATC,
		RT9466_SHIFT_CHG_MIVR, &mivr_act);
	if (ret < 0) {
		dev_err(info->dev, "%s: read mivr stat failed\n", __func__);
		goto out;
	}

	if (!mivr_act) {
		dev_info(info->dev, "%s: mivr loop is not active\n", __func__);
		goto out;
	}

	if (strcmp(info->desc->chg_name, "primary_chg") == 0) {
		/* Check IBUS ADC */
		ret = rt9466_get_adc(info, RT9466_ADC_IBUS, &adc_ibus);
		if (ret < 0) {
			dev_err(info->dev, "%s: get ibus fail\n", __func__);
			goto out;
		}
		if (adc_ibus < 100) { /* 100mA */
			ret = rt9466_toggle_cfo(info);
			goto out;
		}
	}
	schedule_work(&info->aicr_work);
	return ret;
out:
	/* Enable MIVR IRQ */
	schedule_delayed_work(&info->mivr_unmask_dwork, msecs_to_jiffies(200));
	return ret;
}

static int rt9466_chg_aicr_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_treg_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_vsysuv_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_vsysov_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_vbatov_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_vbusov_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_ts_batcold_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_ts_batcool_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_ts_batwarm_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_ts_bathot_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_ts_statci_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_faulti_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_statci_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_tmri_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_batabsi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_adpbadi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_rvpi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_otpi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_aiclmeasi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	rt9466_irq_set_flag(info, &info->irq_flag[RT9466_IRQIDX_CHG_IRQ2],
		RT9466_MASK_CHG_AICLMEASI);
	wake_up_interruptible(&info->wait_queue);
	return 0;
}

static int rt9466_chg_ichgmeasi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_wdtmri_irq_handler(struct rt9466_info *info)
{
	int ret = 0;

	dev_err(info->dev, "%s\n", __func__);
	ret = rt9466_kick_wdt();
	if (ret < 0)
		dev_err(info->dev, "%s: kick wdt fail\n", __func__);

	return ret;
}

static int rt9466_ssfinishi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_rechgi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_termi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_chg_ieoci_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_adc_donei_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_pumpx_donei_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_bst_batuvi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_bst_midovi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

static int rt9466_bst_olpi_irq_handler(struct rt9466_info *info)
{
	dev_err(info->dev, "%s\n", __func__);
	return 0;
}

struct irq_mapping_tbl {
	const char *name;
	int (*hdlr)(struct rt9466_info *info);
};

#define RT9466_IRQ_MAPPING(_name) \
	{.name = #_name, .hdlr = rt9466_ ## _name ## _irq_handler}

static const struct irq_mapping_tbl rt9466_irq_mapping_tbl[48] = {
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(chg_treg),
	RT9466_IRQ_MAPPING(chg_aicr),
	RT9466_IRQ_MAPPING(chg_mivr),
	RT9466_IRQ_MAPPING(pwr_rdy),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(chg_vsysuv),
	RT9466_IRQ_MAPPING(chg_vsysov),
	RT9466_IRQ_MAPPING(chg_vbatov),
	RT9466_IRQ_MAPPING(chg_vbusov),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(ts_batcold),
	RT9466_IRQ_MAPPING(ts_batcool),
	RT9466_IRQ_MAPPING(ts_batwarm),
	RT9466_IRQ_MAPPING(ts_bathot),
	RT9466_IRQ_MAPPING(ts_statci),
	RT9466_IRQ_MAPPING(chg_faulti),
	RT9466_IRQ_MAPPING(chg_statci),
	RT9466_IRQ_MAPPING(chg_tmri),
	RT9466_IRQ_MAPPING(chg_batabsi),
	RT9466_IRQ_MAPPING(chg_adpbadi),
	RT9466_IRQ_MAPPING(chg_rvpi),
	RT9466_IRQ_MAPPING(otpi),
	RT9466_IRQ_MAPPING(chg_aiclmeasi),
	RT9466_IRQ_MAPPING(chg_ichgmeasi),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(wdtmri),
	RT9466_IRQ_MAPPING(ssfinishi),
	RT9466_IRQ_MAPPING(chg_rechgi),
	RT9466_IRQ_MAPPING(chg_termi),
	RT9466_IRQ_MAPPING(chg_ieoci),
	RT9466_IRQ_MAPPING(adc_donei),
	RT9466_IRQ_MAPPING(pumpx_donei),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(reserved),
	RT9466_IRQ_MAPPING(bst_batuvi),
	RT9466_IRQ_MAPPING(bst_midovi),
	RT9466_IRQ_MAPPING(bst_olpi),
};

static inline int rt9466_enable_irqrez(struct rt9466_info *info, bool en)
{
	dev_dbg(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL13, RT9466_MASK_IRQ_REZ);
}

static int __rt9466_irq_handler(struct rt9466_info *info)
{
	int ret = 0, i = 0, j = 0;
	u8 evt[RT9466_IRQIDX_MAX] = {0};
	u8 mask[RT9466_IRQIDX_MAX] = {0};
	u8 stat[RT9466_IRQSTAT_MAX] = {0};

	dev_info(info->dev, "%s\n", __func__);

	/* Read event and skip CHG_IRQ3 */
	ret = rt9466_i2c_block_read(info, RT9466_REG_CHG_IRQ1, 2, &evt[3]);
	if (ret < 0) {
		dev_err(info->dev, "%s: read evt fail(%d)\n", __func__, ret);
		goto err;
	}

	ret = rt9466_i2c_block_read(info, RT9466_REG_CHG_STATC, 3, evt);
	if (ret < 0) {
		dev_err(info->dev, "%s: read stat fail(%d)\n", __func__, ret);
		goto err;
	}

	/* Read mask */
	ret = rt9466_i2c_block_read(info, RT9466_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(mask), mask);
	if (ret < 0) {
		dev_err(info->dev, "%s: read mask fail(%d)\n", __func__, ret);
		goto err;
	}

	/* Store/Update stat */
	memcpy(stat, info->irq_stat, RT9466_IRQSTAT_MAX);

	for (i = 0; i < RT9466_IRQIDX_MAX; i++) {
		evt[i] &= ~mask[i];
		if (i < RT9466_IRQSTAT_MAX) {
			info->irq_stat[i] = evt[i];
			evt[i] ^= stat[i];
		}
		for (j = 0; j < 8; j++) {
			if (!(evt[i] & (1 << j)))
				continue;
			if (rt9466_irq_mapping_tbl[i * 8 + j].hdlr)
				(rt9466_irq_mapping_tbl[i * 8 + j].hdlr)(info);
		}
	}

err:
	return ret;
}

/**********************************************************
*  Function:       rt9466_irq_work
*  Discription:    handler for chargerIC fault irq in charging process
*  Parameters:   work:chargerIC fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void rt9466_irq_work(struct work_struct *work)
{
	struct rt9466_info *info =
	    container_of(work, struct rt9466_info, irq_work);
	int ret = 0;

	msleep(100);//sleep 100ms
	ret = __rt9466_irq_handler(info);
	ret = rt9466_enable_irqrez(info, true);
	if (ret < 0)
		dev_err(info->dev, "%s: en irqrez fail\n", __func__);

	if (info->irq_active == 0) {
		info->irq_active = 1;
		enable_irq(info->irq);
	}
}

static irqreturn_t rt9466_irq_handler(int irq, void *data)
{
	struct rt9466_info *info = (struct rt9466_info *)data;

	dev_info(info->dev, "%s\n", __func__);
	if (info->irq_active == 1) {
		info->irq_active = 0;
		disable_irq_nosync(info->irq);
		schedule_work(&info->irq_work);
	} else {
		dev_info(info->dev, "The irq is not enable,do nothing!\n");
	}

	return IRQ_HANDLED;
}

static int rt9466_irq_register(struct rt9466_info *info)
{
	int ret = 0, len = 0;
	char *name = NULL;

	dev_info(info->dev, "%s\n", __func__);

	/* request gpio */
	len = strlen(info->desc->chg_name);
	name = devm_kzalloc(info->dev, len + 10, GFP_KERNEL);
	snprintf(name,  len + 10, "%s_irq_gpio", info->desc->chg_name);
	ret = devm_gpio_request_one(info->dev, info->intr_gpio, GPIOF_IN, name);
	if (ret < 0) {
		dev_err(info->dev, "%s: gpio request fail\n", __func__);
		return ret;
	}

	ret = gpio_to_irq(info->intr_gpio);
	if (ret < 0) {
		dev_err(info->dev, "%s: irq mapping fail\n", __func__);
		return ret;
	}
	info->irq = ret;
	dev_info(info->dev, "%s: irq = %d\n", __func__, info->irq);

	/* Request threaded IRQ */
	name = devm_kzalloc(info->dev, len + 5, GFP_KERNEL);
	snprintf(name, len + 5, "%s_irq", info->desc->chg_name);
	ret = devm_request_threaded_irq(info->dev, info->irq, NULL,
		rt9466_irq_handler, IRQF_TRIGGER_FALLING | IRQF_ONESHOT, name,
		info);
	if (ret < 0) {
		dev_err(info->dev, "%s: request thread irq fail\n", __func__);
		return ret;
	}
	device_init_wakeup(info->dev, true);

	return 0;
}

static inline int rt9466_irq_init(struct rt9466_info *info)
{
	dev_info(info->dev, "%s\n", __func__);
	return rt9466_i2c_block_write(info, RT9466_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(info->irq_mask), info->irq_mask);
}

static bool rt9466_is_hw_exist(struct rt9466_info *info)
{
	int ret = 0;
	u8 vendor_id = 0, chip_rev = 0;

	ret = i2c_smbus_read_byte_data(info->client, RT9466_REG_DEVICE_ID);
	if (ret < 0)
		return false;

	vendor_id = ret & 0xF0;
	chip_rev = ret & 0x0F;
	if (vendor_id != RT9466_VENDOR_ID) {
		dev_err(info->dev, "%s: vendor id is incorrect (0x%02X)\n",
			__func__, vendor_id);
		return false;
	}

	dev_info(info->dev, "%s: 0x%02X\n", __func__, chip_rev);
	info->chip_rev = chip_rev;

	return true;
}

static int rt9466_set_safety_timer(struct rt9466_info *info, u32 hr)
{
	u8 reg_st = 0;

	reg_st = rt9466_closest_reg_via_tbl(rt9466_safety_timer,
		ARRAY_SIZE(rt9466_safety_timer), hr);

	dev_info(info->dev, "%s: time = %d(0x%02X)\n", __func__, hr, reg_st);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL12,
		reg_st << RT9466_SHIFT_WT_FC, RT9466_MASK_WT_FC);
}

static int rt9466_enable_wdt(struct rt9466_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL13, RT9466_MASK_WDT_EN);
}

static int rt9466_select_input_current_limit(struct rt9466_info *info,
	enum rt9466_iin_limit_sel sel)
{
	dev_info(info->dev, "%s: sel = %d\n", __func__, sel);
	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL2,
		sel << RT9466_SHIFT_IINLMTSEL, RT9466_MASK_IINLMTSEL);
}

static int rt9466_enable_hidden_mode(struct rt9466_info *info, bool en)
{
	int ret = 0;

	mutex_lock(&info->hidden_mode_lock);

	if (en) {
		if (info->hidden_mode_cnt == 0) {
			ret = rt9466_i2c_block_write(info, 0x70,
				ARRAY_SIZE(rt9466_val_en_hidden_mode),
				rt9466_val_en_hidden_mode);
			if (ret < 0)
				goto err;
		}
		info->hidden_mode_cnt++;
	} else {
		if (info->hidden_mode_cnt == 1) /* last one */
			ret = rt9466_i2c_write_byte(info, 0x70, 0x00);
		info->hidden_mode_cnt--;
		if (ret < 0)
			goto err;
	}
	dev_dbg(info->dev, "%s: en = %d\n", __func__, en);
	goto out;

err:
	dev_err(info->dev, "%s: en = %d fail(%d)\n", __func__, en, ret);
out:
	mutex_unlock(&info->hidden_mode_lock);
	return ret;
}

static int rt9466_set_iprec(struct rt9466_info *info, u32 iprec)
{
	u8 reg_iprec = 0;

	reg_iprec = rt9466_closest_reg(RT9466_IPREC_MIN, RT9466_IPREC_MAX,
		RT9466_IPREC_STEP, iprec);

	dev_info(info->dev, "%s: iprec = %d(0x%02X)\n", __func__, iprec,
		reg_iprec);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL8,
		reg_iprec << RT9466_SHIFT_IPREC, RT9466_MASK_IPREC);
}

static int rt9466_post_init(struct rt9466_info *info)
{
	int ret = 0;

	dev_info(info->dev, "%s\n", __func__);

	rt9466_enable_hidden_mode(info, true);

	/* Disable PSK mode */
	ret = rt9466_clr_bit(info, RT9466_REG_CHG_HIDDEN_CTRL9, 0x80);
	if (ret < 0) {
		dev_err(info->dev, "%s: disable skip mode fail\n", __func__);
		goto out;
	}

#if 0 /* For no battery test only */
		ret = rt9466_set_bit(info, RT9466_REG_CHG_HIDDEN_CTRL2, 0x40);
	if (ret < 0)
		goto out;
#endif

	/* Disable TS auto sensing */
	ret = rt9466_clr_bit(info, RT9466_REG_CHG_HIDDEN_CTRL15, 0x01);
	if (ret < 0)
		goto out;

	/* Set precharge current to 850mA, only do this in normal boot */
	if (info->chip_rev <= RT9466_CHIP_REV_E3) {
		/* Worst case delay: wait auto sensing */
		msleep(200);

		ret = rt9466_set_iprec(info, 850);
		if (ret < 0)
			goto out;

		/* Increase Isys drop threshold to 2.5A */
		ret = rt9466_i2c_write_byte(info, RT9466_REG_CHG_HIDDEN_CTRL7,
			0x1c);
		if (ret < 0)
			goto out;
	}

	/* Only revision <= E1 needs the following workaround */
	if (info->chip_rev > RT9466_CHIP_REV_E1)
		goto out;

	/* ICC: modify sensing node, make it more accurate */
	ret = rt9466_i2c_write_byte(info, RT9466_REG_CHG_HIDDEN_CTRL8, 0x00);
	if (ret < 0)
		goto out;

	/* DIMIN level */
	ret = rt9466_i2c_write_byte(info, RT9466_REG_CHG_HIDDEN_CTRL9, 0x86);

out:
	rt9466_enable_hidden_mode(info, false);
	return ret;
}

/* Reset all registers' value to default
static int rt9466_reset_chip(struct rt9466_info *info)
{
	int ret = 0;

	dev_info(info->dev, "%s\n", __func__);

	// disable hz before reset chip
	ret = rt9466_enable_hz(false);
	if (ret < 0) {
		dev_err(info->dev, "%s: disable hz fail\n", __func__);
		return ret;
	}

	return rt9466_set_bit(info, RT9466_REG_CORE_CTRL0, RT9466_MASK_RST);
}
*/
static inline int rt9466_enable_safety_timer(struct rt9466_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL12, RT9466_MASK_TMR_EN);
}

static int __rt9466_set_ieoc(struct rt9466_info *info, u32 ieoc)
{
	int ret = 0;
	u8 reg_ieoc = 0;

	/* IEOC workaround */
	if (info->ieoc_wkard)
		ieoc += 50; /*ICHG < 900mA && ieoc set val < 200mA, here comp 50mA */

	reg_ieoc = rt9466_closest_reg(RT9466_IEOC_MIN, RT9466_IEOC_MAX,
		RT9466_IEOC_STEP, ieoc);

	dev_info(info->dev, "%s: ieoc = %d(0x%02X)\n", __func__, ieoc,
		reg_ieoc);

	ret = rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL9,
		reg_ieoc << RT9466_SHIFT_IEOC, RT9466_MASK_IEOC);

	/* Store IEOC */
	return rt9466_get_ieoc(info, &info->ieoc);
}

static int rt9466_enable_jeita(struct rt9466_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL16, RT9466_MASK_JEITA_EN);
}

static int rt9466_get_charging_status(struct rt9466_info *info,
	enum rt9466_charging_status *chg_stat)
{
	int ret = 0;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STAT);
	if (ret < 0)
		return ret;

	*chg_stat = (ret & RT9466_MASK_CHG_STAT) >> RT9466_SHIFT_CHG_STAT;

	return 0;
}

static int __rt9466_set_ichg(struct rt9466_info *info, u32 ichg)
{
	int ret = 0;
	u8 reg_ichg = 0;

	/* keep ichg >= 900mA
	if (strcmp(info->desc->chg_name, "primary_chg") == 0)
		ichg = (ichg < 900) ? 900 : ichg;
	*/

	reg_ichg = rt9466_closest_reg(RT9466_ICHG_MIN, RT9466_ICHG_MAX,
		RT9466_ICHG_STEP, ichg);

	dev_info(info->dev, "%s: ichg = %d(0x%02X)\n", __func__, ichg,
		reg_ichg);

	ret = rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL7,
		reg_ichg << RT9466_SHIFT_ICHG, RT9466_MASK_ICHG);

	/* Make IEOC accurate */
	if (ichg < 900 && !info->ieoc_wkard) { /* 900mA */
		ret = __rt9466_set_ieoc(info, info->ieoc + 50);
		info->ieoc_wkard = true;
	} else if (ichg >= 900 && info->ieoc_wkard) {
		info->ieoc_wkard = false;
		ret = __rt9466_set_ieoc(info, info->ieoc - 50);
	}

	return ret;
}

static int rt9466_set_ircmp_resistor(struct rt9466_info *info, u32 uohm)
{
	u8 reg_resistor = 0;

	reg_resistor = rt9466_closest_reg(RT9466_IRCMP_RES_MIN,
		RT9466_IRCMP_RES_MAX, RT9466_IRCMP_RES_STEP, uohm);

	dev_info(info->dev, "%s: resistor = %d(0x%02X)\n", __func__, uohm,
		reg_resistor);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL18,
		reg_resistor << RT9466_SHIFT_IRCMP_RES, RT9466_MASK_IRCMP_RES);
}

static int rt9466_set_ircmp_vclamp(struct rt9466_info *info, u32 uV)
{
	u8 reg_vclamp = 0;

	reg_vclamp = rt9466_closest_reg(RT9466_IRCMP_VCLAMP_MIN,
		RT9466_IRCMP_VCLAMP_MAX, RT9466_IRCMP_VCLAMP_STEP, uV);

	dev_info(info->dev, "%s: vclamp = %d(0x%02X)\n", __func__, uV,
		reg_vclamp);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL18,
		reg_vclamp << RT9466_SHIFT_IRCMP_VCLAMP,
		RT9466_MASK_IRCMP_VCLAMP);
}

static int rt9466_enable_irq_pulse(struct rt9466_info *info, bool en)
{
	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL1, RT9466_MASK_IRQ_PULSE);
}

static inline int rt9466_get_irq_number(struct rt9466_info *info,
	const char *name)
{
	int i = 0;

	if (!name) {
		dev_err(info->dev, "%s: null name\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < ARRAY_SIZE(rt9466_irq_mapping_tbl); i++) {
		if (!strcmp(name, rt9466_irq_mapping_tbl[i].name))
			return i;
	}

	return -EINVAL;
}

static inline const char *rt9466_get_irq_name(struct rt9466_info *info,
	int irqnum)
{
	if (irqnum >= 0 && irqnum < ARRAY_SIZE(rt9466_irq_mapping_tbl))
		return rt9466_irq_mapping_tbl[irqnum].name;

	return "not found";
}

static inline void rt9466_irq_mask(struct rt9466_info *info, int irqnum)
{
	dev_dbg(info->dev, "%s: irq = %d, %s\n", __func__, irqnum,
		rt9466_get_irq_name(info, irqnum));
	info->irq_mask[irqnum / 8] |= (1 << (irqnum % 8));
}

static inline void rt9466_irq_unmask(struct rt9466_info *info, int irqnum)
{
	dev_dbg(info->dev, "%s: irq = %d, %s\n", __func__, irqnum,
		rt9466_get_irq_name(info, irqnum));
	info->irq_mask[irqnum / 8] &= ~(1 << (irqnum % 8));
}


static int rt9466_parse_dt(struct rt9466_info *info, struct device *dev)
{
	int ret = 0, irq_cnt = 0;
	struct rt9466_desc *desc = NULL;
	struct device_node *np = dev->of_node;
	const char *name = NULL;
	char *ceb_name = NULL;
	int irqnum = 0, len = 0;

	dev_info(info->dev, "%s\n", __func__);

	if (!np) {
		dev_err(info->dev, "%s: no device node\n", __func__);
		return -EINVAL;
	}

	info->desc = &rt9466_default_desc;

	desc = devm_kzalloc(dev, sizeof(struct rt9466_desc), GFP_KERNEL);
	if (!desc)
		return -ENOMEM;
	memcpy(desc, &rt9466_default_desc, sizeof(struct rt9466_desc));

	if (of_property_read_string(np, "charger_name", &desc->chg_name) < 0)
		dev_err(info->dev, "%s: no charger name\n", __func__);

	ret = of_get_named_gpio(np, "rt,intr_gpio", 0);
	if (ret < 0)
		return ret;
	info->intr_gpio = ret;

	ret = of_get_named_gpio(np, "rt,ceb_gpio", 0);
	if (ret < 0)
		return ret;
	info->ceb_gpio = ret;

	dev_info(info->dev, "%s: intr/ceb gpio = %d, %d\n", __func__,
		info->intr_gpio, info->ceb_gpio);

	/* request gpio */
	len = strlen(info->desc->chg_name);
	ceb_name = devm_kzalloc(info->dev, len + 10, GFP_KERNEL);
	snprintf(ceb_name,  len + 10, "%s_ceb_gpio", info->desc->chg_name);
	ret = devm_gpio_request_one(info->dev, info->ceb_gpio,
		GPIOF_DIR_OUT, ceb_name);
	if (ret < 0) {
		dev_err(info->dev, "%s: ceb gpio request fail\n",
			__func__);
		return ret;
	}

	if (of_property_read_u32(np, "regmap_represent_slave_addr",
		&desc->regmap_represent_slave_addr) < 0)
		dev_err(info->dev, "%s: no regmap slave addr\n", __func__);

	if (of_property_read_string(np, "regmap_name",
		&(desc->regmap_name)) < 0)
		dev_err(info->dev, "%s: no regmap name\n", __func__);

	if (of_property_read_u32(np, "ichg", &desc->ichg) < 0)
		dev_err(info->dev, "%s: no ichg\n", __func__);

	if (of_property_read_u32(np, "aicr", &desc->aicr) < 0)
		dev_err(info->dev, "%s: no aicr\n", __func__);

	if (of_property_read_u32(np, "mivr", &desc->mivr) < 0)
		dev_err(info->dev, "%s: no mivr\n", __func__);

	if (of_property_read_u32(np, "cv", &desc->cv) < 0)
		dev_err(info->dev, "%s: no cv\n", __func__);

	if (of_property_read_u32(np, "ieoc", &desc->ieoc) < 0)
		dev_err(info->dev, "%s: no ieoc\n", __func__);

	if (of_property_read_u32(np, "safety_timer", &desc->safety_timer) < 0)
		dev_err(info->dev, "%s: no safety timer\n", __func__);

	if (of_property_read_u32(np, "ircmp_resistor",
		&desc->ircmp_resistor) < 0)
		dev_err(info->dev, "%s: no ircmp resistor\n", __func__);

	if (of_property_read_u32(np, "ircmp_vclamp", &desc->ircmp_vclamp) < 0)
		dev_err(info->dev, "%s: no ircmp vclamp\n", __func__);

	if (of_property_read_u32(np, "hiz_iin_limit", &(desc->hiz_iin_limit)) < 0){
		desc->hiz_iin_limit = 0;
		dev_err(info->dev, "%s: no hiz_iin_limit\n", __func__);
	}
	dev_info(info->dev, "%s: hiz_iin_limit = %d\n", __func__,desc->hiz_iin_limit);
	desc->en_te = of_property_read_bool(np, "en_te");
	desc->en_wdt = of_property_read_bool(np, "en_wdt");
	desc->en_irq_pulse = of_property_read_bool(np, "en_irq_pulse");
	desc->en_jeita = of_property_read_bool(np, "en_jeita");
	desc->ceb_invert = of_property_read_bool(np, "ceb_invert");

	while (true) {
		ret = of_property_read_string_index(np, "interrupt-names",
			irq_cnt, &name);
		if (ret < 0)
			break;
		irq_cnt++;
		irqnum = rt9466_get_irq_number(info, name);
		if (irqnum >= 0)
			rt9466_irq_unmask(info, irqnum);
	}

	info->desc = desc;

	return 0;
}

/* =========================================================== */
/* Released interfaces                                         */
/* =========================================================== */

static int rt9466_enable_hz(int en)
{
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	if (huawei_force_hz == 1)
		en = 1;

	dev_info(info->dev, "%s: caller = %pF, %pF en = %d\n", __func__,
		__builtin_return_address(0), __builtin_return_address(1), en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL1, RT9466_MASK_HZ_EN);
}

static int rt9466_huawei_force_enable_hz(int en)
{
	struct rt9466_info *info = NULL;
	struct rt9466_desc *desc = NULL;
	static int first_in = 1;
	int ret = 0;

	info = i2c_get_clientdata(g_rt9466_i2c);
	if(NULL == info){
		pr_err("%s: info is NULL\n", __func__);
		return 0;
	}
	desc = info->desc;
	if(NULL == desc){
		pr_err("%s: desc is NULL\n", __func__);
		return 0;
	}

	if (en > 0){
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
		if(1 == desc->hiz_iin_limit && is_uscp_hiz_mode() && !is_in_rt_uscp_mode()){
			hiz_iin_limit_flag = HIZ_IIN_FLAG_TRUE;
			if(first_in){
				dev_info(info->dev,"[%s] is_uscp_hiz_mode HIZ,enable:%d,set 100mA\n", __func__, en);
				first_in = 0;
				return rt9466_set_aicr(IINLIM_100);//set inputcurrent to 100mA
			}else{
				return 0;
			}
		}else{
#endif
			huawei_force_hz = en;
			ret = rt9466_enable_hz(en);
#ifdef  CONFIG_HUAWEI_USB_SHORT_CIRCUIT_PROTECT
		}
#endif
	}else{
		hiz_iin_limit_flag = HIZ_IIN_FLAG_FALSE;
		first_in = 1;
		huawei_force_hz = en;
		ret = rt9466_enable_hz(en);
	}
	return ret;
}

static int rt9466_enable_charging(int en)
{
	int ret = 0, adc_vbus = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	/* Check VBUS_ADC */
	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
	if (ret < 0)
		dev_err(info->dev, "%s, rt9466 get vbus fail\n", __func__);

	/* set hz/ceb pin for secondary charger */
	if (strcmp(info->desc->chg_name, "secondary_chg") == 0) {
		ret = rt9466_enable_hz(!en);
		if (ret < 0) {
			dev_err(info->dev, "%s: set hz of sec chg fail\n",
				__func__);
			return ret;
		}
		if (info->desc->ceb_invert)
			gpio_set_value(info->ceb_gpio, en);
		else
			gpio_set_value(info->ceb_gpio, !en);
	}
	if (adc_vbus >= VBUS_THRESHOLD) {
		ret = rt9466_enable_hz(false);
		if (ret < 0)
			dev_info(info->dev, "%s: rt9466_enable_hz fail\n", __func__);
	}
	if (en) {
		ret |= rt9466_set_bit(info, RT9466_REG_CHG_CTRL2, RT9466_MASK_CHG_EN);
	} else {
		ret = rt9466_clr_bit(info, RT9466_REG_CHG_CTRL2, RT9466_MASK_CHG_EN);
		if (adc_vbus < VBUS_THRESHOLD)
			ret |= rt9466_enable_hz(true);
	}
	dev_info(info->dev, "%s: adc_vbus = %d\n", __func__, adc_vbus);
	if (ret < 0)
		dev_err(info->dev, "%s: fail\n", __func__);
	return ret;
}

static int rt9466_set_aicr(int aicr)
{
	int ret = 0;
	int adc_vbus = 0, mivr = 0, aicl_vth = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	/* check vbus > v_th */
	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
	if (ret < 0) {
		dev_err(info->dev, "%s: get ibus fail\n", __func__);
		return ret;
	}
	ret = rt9466_get_mivr(info, &mivr);
	if (ret < 0)
		return ret;

	/* Check if there's a suitable AICL_VTH */
	aicl_vth = mivr + 200;
	if (adc_vbus > aicl_vth)
		info->aicr_limit = -1;

	/* limit aicr */
	if (info->aicr_limit > 0 && info->aicr_limit < aicr) {
		aicr = info->aicr_limit;
		dev_info(info->dev, "%s: aicr limit by %d\n", __func__, info->aicr_limit);
	}

	mutex_lock(&info->aicr_lock);
	ret = __rt9466_set_aicr(info, aicr);
	mutex_unlock(&info->aicr_lock);

	return ret;
}

static int rt9466_set_ichg(int ichg)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	mutex_lock(&info->ichg_lock);
	mutex_lock(&info->ieoc_lock);
	ret = __rt9466_set_ichg(info, ichg);
	mutex_unlock(&info->ieoc_lock);
	mutex_unlock(&info->ichg_lock);

	return (ret < 0 ? ret : 0);
}

static int rt9466_set_cv(int cv)
{
	u8 reg_cv = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	reg_cv = rt9466_closest_reg(RT9466_CV_MIN, RT9466_CV_MAX,
		RT9466_CV_STEP, cv);

	dev_info(info->dev, "%s: cv = %d(0x%02X)\n", __func__, cv, reg_cv);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL4,
		reg_cv << RT9466_SHIFT_CV, RT9466_MASK_CV);
}

static int rt9466_set_mivr(int mivr)
{
	u8 reg_mivr = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	reg_mivr = rt9466_closest_reg(RT9466_MIVR_MIN, RT9466_MIVR_MAX,
		RT9466_MIVR_STEP, mivr);

	dev_info(info->dev, "%s: mivr = %d(0x%02X)\n", __func__, mivr,
		reg_mivr);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL6,
		reg_mivr << RT9466_SHIFT_MIVR, RT9466_MASK_MIVR);
}

static int rt9466_set_ieoc(int ieoc)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	mutex_lock(&info->ichg_lock);
	mutex_lock(&info->ieoc_lock);
	ret = __rt9466_set_ieoc(info, ieoc);
	mutex_unlock(&info->ieoc_lock);
	mutex_unlock(&info->ichg_lock);

	return (ret < 0 ? ret : 0);
}

static int rt9466_set_boost_voltage(unsigned int voltage)
{
	u8 reg_voltage = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	if (voltage < RT9466_BOOST_VOREG_MIN)
		return -1;
	reg_voltage = (voltage - RT9466_BOOST_VOREG_MIN) / RT9466_BOOST_VOREG_STEP;
	dev_info(info->dev, "%s: boost voltage = %d(0x%02X)\n", __func__, reg_voltage);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL5,
		reg_voltage << RT9466_SHIFT_BOOST_VOREG, RT9466_MASK_BOOST_VOREG);
}

static int rt9466_set_boost_current_limit(int current_limit)
{
	u8 reg_ilimit = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	reg_ilimit = rt9466_closest_reg_via_tbl(rt9466_boost_oc_threshold,
		ARRAY_SIZE(rt9466_boost_oc_threshold), current_limit);

	dev_info(info->dev, "%s: boost ilimit = %d(0x%02X)\n", __func__,
		current_limit, reg_ilimit);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL10,
		reg_ilimit << RT9466_SHIFT_BOOST_OC, RT9466_MASK_BOOST_OC);
}

static int rt9466_enable_discharging(int en)
{
	int ret =0, i = 0, reg_value = 0, data = 0;
	const unsigned int max_retry_cnt = 3;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	if (!en)
		return ret;
/*
	ret = rt9466_enable_hidden_mode(info, true);
	if (ret < 0) {
		dev_err(info->dev, "%s: enable hidden mode fail\n", __func__);
		return ret;
	}
*/
	/* Set bit2 of reg[0x21] to 1 to enable discharging */
	ret = rt9466_set_bit(info, RT9466_REG_CHG_HIDDEN_CTRL2, 0x04);
	if (ret < 0) {
		dev_err(info->dev, "%s: enable discharging fail\n", __func__);
		goto out;
	}

	/* Wait for discharging to 0V */
	msleep(200);

	for (i = 0; i < max_retry_cnt; i++) {
		/* Disable discharging */
		ret = rt9466_clr_bit(info, RT9466_REG_CHG_HIDDEN_CTRL2, 0x04);
		if (ret < 0)
			continue;
		reg_value = rt9466_i2c_read_byte(info,
		RT9466_REG_CHG_HIDDEN_CTRL2);
		data = reg_value & 0x04;
		if (data == 0)
			break;
	}
	if (i == max_retry_cnt)
		dev_info(info->dev, "%s: disable discharging fail\n", __func__);
out:
	/* rt9466_enable_hidden_mode(info, false); */
	return ret;
}

/* Disable OTG workaround */
static int rt9466_check_vbus_hz_workaround(int disable)
{
	int ret = 0, adc_vbus = 0, vbus_last = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	dev_info(info->dev, "%s: en = %d\n", __func__, disable);
	while (1) {
		/* Check VBUS_ADC */
		ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
		if (ret < 0)
			dev_err(info->dev, "%s, rt9466 get vbus fail\n", __func__);
		vbus_last = adc_vbus;

		if (adc_vbus < VBUS_POST_THRESHOLD)
			ret = rt9466_enable_hz(true);
		else
			ret = rt9466_enable_hz(false);
		/* Wait for adc_vbus active */
		msleep(40);
		/* Check VBUS ADC */
		ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
		if (ret < 0)
			dev_err(info->dev, "%s, rt9466 get vbus fail\n", __func__);

		dev_info(info->dev, "%s: vbus_last = %d, vbus_now = %d\n",
					__func__,  vbus_last, adc_vbus);

		if (((vbus_last > VBUS_POST_THRESHOLD) && (adc_vbus > VBUS_POST_THRESHOLD)) ||
			((vbus_last < VBUS_POST_THRESHOLD) && (adc_vbus < VBUS_POST_THRESHOLD)))
			break;
	}
	return ret;
}

static int rt9466_enable_otg(int en)
{
	int ret = 0;
	bool en_otg = false;
	u8 hidden_val = en ? 0x00 : 0x0F;
	u8 lg_slew_rate = en ? 0x7c : 0x73;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	dev_info(info->dev, "%s: en = %d\n", __func__, en);

	if (en) {
		ret = rt9466_enable_hz(false);
		if (ret < 0) {
			dev_err(info->dev, "%s: rt9466_enable_hz: %d fail\n",
					__func__, en);
			goto out;
		}
	}

	rt9466_enable_hidden_mode(info, true);

	/* Set OTG_OC */
	ret = rt9466_set_boost_current_limit(info->core_data->otg_curr);
	if (ret < 0) {
		dev_err(info->dev, "%s: set current limit fail\n", __func__);
		return ret;
	}

	/*
	 * Slow Low side mos Gate driver slew rate
	 * for decline VBUS noise
	 * reg[0x23] = 0x7c after entering OTG mode
	 * reg[0x23] = 0x73 after leaving OTG mode
	 */
	ret = rt9466_i2c_write_byte(info, RT9466_REG_CHG_HIDDEN_CTRL4,
		lg_slew_rate);
	if (ret < 0) {
		dev_err(info->dev,
			"%s: set Low side mos Gate drive speed fail(%d)\n",
			__func__, ret);
		goto out;
	}

	/* Enable WDT */
	if (en && info->desc->en_wdt) {
		ret = rt9466_enable_wdt(info, true);
		if (ret < 0) {
			dev_err(info->dev, "%s: en wdt fail\n", __func__);
			goto err_en_otg;
		}
	}

	/* Switch OPA mode */
	ret = (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL1, RT9466_MASK_OPA_MODE);

	msleep(20);

	if (en) {
		ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL1,
			RT9466_SHIFT_OPA_MODE, &en_otg);
		if (ret < 0 || !en_otg) {
			dev_err(info->dev, "%s: otg fail(%d)\n", __func__, ret);
			goto err_en_otg;
		}
	}

	/*
	 * reg[0x25] = 0x00 after entering OTG mode
	 * reg[0x25] = 0x0F after leaving OTG mode
	 */
	ret = rt9466_i2c_write_byte(info, RT9466_REG_CHG_HIDDEN_CTRL6,
		hidden_val);
	if (ret < 0)
		dev_err(info->dev, "%s: workaroud fail(%d)\n", __func__, ret);

	/* discharge when disable_otg */
	if (!en) {
		ret = rt9466_enable_discharging(!en);
		if (ret < 0)
			dev_err(info->dev, "%s: discharging fail\n", __func__);
	}
	/* Disable WDT */
	if (!en) {
		ret = rt9466_enable_wdt(info, false);
		if (ret < 0)
			dev_err(info->dev, "%s: disable wdt fail\n", __func__);

		ret = rt9466_check_vbus_hz_workaround(en);
		if (ret < 0)
			dev_err(info->dev, "%s: check vbus and hz status fail\n", __func__);
	}
	goto en_hidden_mode;

err_en_otg:
	/* Disable WDT */
	ret = rt9466_enable_wdt(info, false);
	if (ret < 0)
		dev_err(info->dev, "%s: disable wdt fail\n", __func__);

	/* Recover Low side mos Gate slew rate */
	ret = rt9466_i2c_write_byte(info, RT9466_REG_CHG_HIDDEN_CTRL4, 0x73);
	if (ret < 0)
		dev_err(info->dev,
			"%s: recover Low side mos Gate drive speed fail(%d)\n",
			__func__, ret);
	ret = -EIO;
out:
	rt9466_enable_hz(true);
en_hidden_mode:
	rt9466_enable_hidden_mode(info, false);
	return ret;
}

static int rt9466_enable_te(int en)
{
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	dev_info(info->dev, "%s: en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL2, RT9466_MASK_TE_EN);
}

static int rt9466_set_wdt(int wdt)
{
	int ret = 0;
	u8 reg_wdt = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	if (wdt == WDT_STOP)
		return rt9466_enable_wdt(info, false);
	ret = rt9466_enable_wdt(info, true);
	if (ret < 0)
		return ret;

	reg_wdt = rt9466_closest_reg_via_tbl(rt9466_wdt,
		ARRAY_SIZE(rt9466_wdt), wdt);

	dev_info(info->dev, "%s: wdt = %d(0x%02X)\n", __func__, wdt, reg_wdt);

	return rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL13,
		reg_wdt << RT9466_SHIFT_WDT, RT9466_MASK_WDT);
}

static int rt9466_get_charge_state(unsigned int *state)
{
	int ret = 0;
	u8 chg_fault = 0;
	u8 chg_stat = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);
	enum rt9466_charging_status chg_status = RT9466_CHG_STATUS_READY;

	ret = rt9466_get_charging_status(info, &chg_status);
	if (ret < 0)
		return ret;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STATC);
	if (ret < 0)
		return ret;
	chg_stat = ret & 0xFF;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_FAULT);
	if (ret < 0)
		return ret;
	chg_fault = ret & 0xFF;

	if (chg_status == RT9466_CHG_STATUS_DONE)
		*state |= CHAGRE_STATE_CHRG_DONE;
	if (!(chg_stat & RT9466_MASK_PWR_RDY))
		*state |= CHAGRE_STATE_NOT_PG;
	if (chg_fault & RT9466_MASK_VBUSOV)
		*state |= CHAGRE_STATE_VBUS_OVP;
	if (chg_fault & RT9466_MASK_VBATOV)
		*state |= CHAGRE_STATE_BATT_OVP;

	return 0;
}

static int rt9466_get_vbus(unsigned int *vbus)
{
	int ret = 0, adc_vbus = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
	if (ret < 0) {
		*vbus = 0;
		return -1;
	}
	*vbus = adc_vbus;
	return 0;
}

static int rt9466_get_ibus(void)
{
	int ret = 0, adc_ibus = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	ret = rt9466_get_adc(info, RT9466_ADC_IBUS, &adc_ibus);
	if (ret < 0)
		return -1;
	return adc_ibus;
}

static int rt9466_kick_wdt(void)
{
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);
	enum rt9466_charging_status chg_status = RT9466_CHG_STATUS_READY;

	/* Any I2C communication can reset watchdog timer */
	return rt9466_get_charging_status(info, &chg_status);
}

static int rt9466_check_input_dpm_state(void)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STATC);
	if (ret < 0)
		return ret;

	if ((ret & RT9466_MASK_CHG_MIVRM) || (ret & RT9466_MASK_CHG_AICRM))
		return TRUE;
	return FALSE;
}

static int rt9466_check_input_vdpm_state(void)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STATC);
	if (ret < 0)
		return ret;

	if (ret & RT9466_MASK_CHG_MIVRM)
		return TRUE;
	return FALSE;
}

static int rt9466_check_input_idpm_state(void)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STATC);
	if (ret < 0)
		return ret;

	if (ret & RT9466_MASK_CHG_AICRM)
		return TRUE;
	return FALSE;
}
static int rt9466_get_register_head(char *reg_head)
{
	char buff[26] = {0};
	int i = 0;

	memset(reg_head, 0, CHARGELOG_SIZE);
	for (i = 0; i < ARRAY_SIZE(rt9466_reg_addr); i++) {
		snprintf(buff, 26, "Reg[0x%2x] ", rt9466_reg_addr[i]);
		strncat(reg_head, buff, strlen(buff));
	}
	return 0;
}

static int rt9466_dump_register(char *reg_value)
{
	int i = 0, ret = 0;
	u32 ichg = 0, aicr = 0, mivr = 0, ieoc = 0;
	bool chg_en = 0;
	int adc_vsys = 0, adc_vbat = 0, adc_ibat = 0, adc_ibus = 0, adc_vbus = 0;
	enum rt9466_charging_status chg_status = RT9466_CHG_STATUS_READY;
	u8 chg_stat = 0, chg_ctrl[2] = {0};
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);
	char buff[26] = {0};

	ret = rt9466_get_ichg(info, &ichg);
	ret = rt9466_get_aicr(info, &aicr);
	ret = rt9466_get_mivr(info, &mivr);
	ret = rt9466_is_charging_enable(info, &chg_en);
	ret = rt9466_get_ieoc(info, &ieoc);
	ret = rt9466_get_charging_status(info, &chg_status);
	ret = rt9466_get_adc(info, RT9466_ADC_VSYS, &adc_vsys);
	ret = rt9466_get_adc(info, RT9466_ADC_VBAT, &adc_vbat);
	ret = rt9466_get_adc(info, RT9466_ADC_IBAT, &adc_ibat);
	ret = rt9466_get_adc(info, RT9466_ADC_IBUS, &adc_ibus);
	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
	chg_stat = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STATC);
	ret = rt9466_i2c_block_read(info, RT9466_REG_CHG_CTRL1, 2, chg_ctrl);

	memset(reg_value, 0, CHARGELOG_SIZE);
	for (i = 0; i < ARRAY_SIZE(rt9466_reg_addr); i++) {
		ret = rt9466_i2c_read_byte(info, rt9466_reg_addr[i]);
		if (ret < 0)
			continue;
		snprintf(buff, 26, "0x%-8x", ret & 0xFF);
		strncat(reg_value, buff, strlen(buff));
	}

	dev_info(info->dev,
		"%s: ICHG = %dmA, AICR = %dmA, MIVR = %dmV, IEOC = %dmA\n",
		__func__, ichg, aicr, mivr, ieoc);

	dev_info(info->dev,
		"%s: VSYS = %dmV, VBAT = %dmV, IBAT = %dmA, IBUS = %dmA, VBUS = %dmV\n",
		__func__, adc_vsys, adc_vbat, adc_ibat, adc_ibus, adc_vbus);

	dev_info(info->dev,
		"%s: CHG_EN = %d, CHG_STATUS = %s, CHG_STAT = 0x%02X\n",
		__func__, chg_en, rt9466_chg_status_name[chg_status], chg_stat);

	dev_info(info->dev, "%s: CHG_CTRL1 = 0x%02X, CHG_CTRL2 = 0x%02X\n",
		__func__, chg_ctrl[0], chg_ctrl[1]);

	return ret;
}

static int rt9466_run_aicr(struct rt9466_info *info)
{
	int ret = 0;

	/* Disable MIVR IRQ */
	ret = rt9466_set_bit(info, RT9466_REG_CHG_STATC_CTRL, RT9466_MASK_CHG_MIVR);
	if (ret < 0)
		dev_err(info->dev, "%s: disable MIVR IRQ failed\n", __func__);

	ret = __rt9466_run_aicr(info);
	return ret;
}

static int rt9466_enable_shipping_mode(int en)
{
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	dev_info(info->dev, "%s en = %d\n", __func__, en);
	return (en ? rt9466_set_bit : rt9466_clr_bit)
		(info, RT9466_REG_CHG_CTRL2, RT9466_MASK_SHIPMODE);
}

static int rt9466_5v_chip_init(void)
{
	int ret = 0;
	u8 chg_ctrl2 = 0, statc_ctrl = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	dev_info(info->dev, "%s\n", __func__);
	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_STATC_CTRL);
	if (ret < 0)
		return ret;
	statc_ctrl = ret & 0xff;

	ret = rt9466_i2c_read_byte(info, RT9466_REG_CHG_CTRL1);
	if (ret < 0)
		return ret;
	chg_ctrl2 = ret & 0xff;
	chg_ctrl2 = (chg_ctrl2 & RT9466_MASK_IINLMTSEL)
		>> RT9466_SHIFT_IINLMTSEL;

	hiz_iin_limit_flag = HIZ_IIN_FLAG_FALSE;//init hiz_limit flag to false
	if (chg_ctrl2 != RT9466_IINLMTSEL_AICR &&
		statc_ctrl != info->irq_mask[RT9466_IRQIDX_CHG_STATC]) {
		dev_err(info->dev, "%s 0x%02X 0x%02X\n", __func__, chg_ctrl2,
			statc_ctrl);
		dev_err(info->dev, "%s [WARNING]: reset happened\n", __func__);
		rt9466_init_setting(info);
		rt9466_irq_init(info);
		schedule_work(&info->init_work);
	}

	return 0;
}
static int rt9466_chip_init(struct chip_init_crit* init_crit)
{
	int ret = -1;
	if (!init_crit) {
		return -ENOMEM;
	}
	switch(init_crit->vbus) {
		case ADAPTER_5V:
			ret = rt9466_5v_chip_init();
			break;
		default:
			break;
	}
	return ret;
}
static int rt9466_device_check(void)
{
	int ret = 0;
	u8 vendor_id = 0;
	struct rt9466_info *info = i2c_get_clientdata(g_rt9466_i2c);

	ret = rt9466_i2c_read_byte(info, RT9466_REG_DEVICE_ID);
	if (ret < 0)
		return CHARGE_IC_BAD;

	vendor_id = ret & 0xF0;
	if (vendor_id != RT9466_VENDOR_ID) {
		dev_err(info->dev, "%s: vendor id is incorrect (0x%02X)\n",
			__func__, vendor_id);
		return CHARGE_IC_BAD;
	}
	return CHARGE_IC_GOOD;
}

static struct charge_device_ops rt9466_ops = {
	.set_charger_hiz = rt9466_huawei_force_enable_hz,
	.set_charge_enable = rt9466_enable_charging,
	.set_input_current = rt9466_set_aicr,
	.set_charge_current = rt9466_set_ichg,
	.set_terminal_voltage = rt9466_set_cv,
	.set_dpm_voltage = rt9466_set_mivr,
	.set_terminal_current = rt9466_set_ieoc,
	.set_otg_current = rt9466_set_boost_current_limit,
	.set_otg_enable = rt9466_enable_otg,
	.set_term_enable = rt9466_enable_te,
	.set_watchdog_timer = rt9466_set_wdt,
	.get_charge_state = rt9466_get_charge_state,
	.get_ibus = rt9466_get_ibus,
	.get_vbus = rt9466_get_vbus,
	.reset_watchdog_timer = rt9466_kick_wdt,
	.check_input_dpm_state = rt9466_check_input_dpm_state,
	.check_input_vdpm_state = rt9466_check_input_vdpm_state,
	.check_input_idpm_state = rt9466_check_input_idpm_state,
	.get_register_head = rt9466_get_register_head,
	.dump_register = rt9466_dump_register,
	.set_batfet_disable = rt9466_enable_shipping_mode,
	.chip_init = rt9466_chip_init,
	.dev_check = rt9466_device_check,
	.turn_on_ico = NULL,
	.set_boost_voltage = rt9466_set_boost_voltage,
};

static int __rt9466_enable_auto_sensing(struct rt9466_info *info, bool en)
{
	int ret = 0;
	u8 auto_sense = 0;
	u8 exit_hid = 0x00;

	/* enter hidden mode */
	ret = rt9466_device_write(info->client, 0x70,
		ARRAY_SIZE(rt9466_val_en_hidden_mode),
		rt9466_val_en_hidden_mode);
	if (ret < 0)
		return ret;

	ret = rt9466_device_read(info->client, RT9466_REG_CHG_HIDDEN_CTRL15, 1,
		&auto_sense);
	if (ret < 0) {
		dev_err(info->dev, "%s: read auto sense fail\n", __func__);
		goto out;
	}

	if (en)
		auto_sense &= 0xFE; /* clear bit0 */
	else
		auto_sense |= 0x01; /* set bit0 */
	ret = rt9466_device_write(info->client, RT9466_REG_CHG_HIDDEN_CTRL15, 1,
		&auto_sense);
	if (ret < 0)
		dev_err(info->dev, "%s: en = %d fail\n", __func__, en);

out:
	return rt9466_device_write(info->client, 0x70, 1, &exit_hid);
}

/*
 * This function is used in shutdown function
 * Use i2c smbus directly
 */
static int rt9466_sw_reset(struct rt9466_info *info)
{
	int ret = 0, adc_vbus = 0;
	bool opa_mode = false;
	u8 evt[RT9466_IRQIDX_MAX] = {0};

	dev_info(info->dev, "%s\n", __func__);

	/* Disable auto sensing/Enable HZ,ship mode of secondary charger */
	if (strcmp(info->desc->chg_name, "secondary_chg") == 0) {
		mutex_lock(&info->hidden_mode_lock);
		mutex_lock(&info->io_lock);
		__rt9466_enable_auto_sensing(info, false);
		mutex_unlock(&info->io_lock);
		mutex_unlock(&info->hidden_mode_lock);

		reset_reg_data[0] = 0x14; /* HZ */
		reset_reg_data[1] = 0x83; /* Shipping mode */
	}

	/* Mask all irq */
	mutex_lock(&info->io_lock);
	ret = rt9466_device_write(info->client, RT9466_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(rt9466_irq_maskall), rt9466_irq_maskall);
	if (ret < 0)
		dev_err(info->dev, "%s: mask all irq fail\n", __func__);

	/* Read event and skip CHG_IRQ3 */
	ret = rt9466_device_read(info->client, RT9466_REG_CHG_IRQ1, 2, &evt[3]);
	if (ret < 0)
		dev_err(info->dev, "%s: read evt fail(%d)\n", __func__, ret);

	/* Reset necessary registers */
	ret = rt9466_device_write(info->client, RT9466_REG_CHG_CTRL1,
		ARRAY_SIZE(reset_reg_data), reset_reg_data);
	if (ret < 0)
		dev_err(info->dev, "%s: reset registers fail\n", __func__);
	mutex_unlock(&info->io_lock);

	/* Check pwr_rdy and opa_mode */
	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
	if (ret < 0)
		dev_err(info->dev, "%s, rt9466 get vbus fail\n", __func__);
	ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL1,
				RT9466_SHIFT_OPA_MODE, &opa_mode);
	if (ret < 0)
		dev_err(info->dev, "%s, rt9466 i2c test bit fail\n", __func__);
	if ((adc_vbus < VBUS_THRESHOLD) && !opa_mode)
		ret = rt9466_enable_hz(true);
	else
		ret = rt9466_enable_hz(false);
	if (ret < 0)
		dev_err(info->dev, "%s, operate hiz fail\n", __func__);

	return ret;
}

static int rt9466_set_tdeg_eoc(struct rt9466_info *info, u32 tdeg)
{
	int ret = 0;
	ret = rt9466_i2c_update_bits(info, RT9466_REG_CHG_CTRL9,
		tdeg, RT9466_MASK_CHG_TDEG_EOC);
	if (ret < 0)
		dev_err(info->dev, "%s: fail\n", __func__);

	return ret;
}

static int rt9466_init_setting(struct rt9466_info *info)
{
	int ret = 0;
	struct rt9466_desc *desc = info->desc;
	u8 evt[RT9466_IRQIDX_MAX] = {0};

	dev_info(info->dev, "%s\n", __func__);

	/* mask all irq */
	ret = rt9466_i2c_block_write(info, RT9466_REG_CHG_STATC_CTRL,
		ARRAY_SIZE(rt9466_irq_maskall), rt9466_irq_maskall);
	if (ret < 0) {
		dev_err(info->dev, "%s: mask all irq fail\n", __func__);
		goto err;
	}

	/* Read event and skip CHG_IRQ3 */
	ret = rt9466_i2c_block_read(info, RT9466_REG_CHG_IRQ1, 2, &evt[3]);
	if (ret < 0) {
		dev_err(info->dev, "%s: read evt fail(%d)\n", __func__, ret);
		goto err;
	}

	ret = __rt9466_set_ichg(info, desc->ichg);
	if (ret < 0)
		dev_err(info->dev, "%s: set ichg fail\n", __func__);

	ret = __rt9466_set_aicr(info, desc->aicr);
	if (ret < 0)
		dev_err(info->dev, "%s: set aicr fail\n", __func__);

	ret = rt9466_set_mivr(desc->mivr);
	if (ret < 0)
		dev_err(info->dev, "%s: set mivr fail\n", __func__);

	ret = rt9466_set_cv(desc->cv);
	if (ret < 0)
		dev_err(info->dev, "%s: set cv fail\n", __func__);

	ret = __rt9466_set_ieoc(info, desc->ieoc);
	if (ret < 0)
		dev_err(info->dev, "%s: set ieoc fail\n", __func__);

	ret = rt9466_enable_te(desc->en_te);
	if (ret < 0)
		dev_err(info->dev, "%s: set te fail\n", __func__);

	ret = rt9466_enable_safety_timer(info, false);
	if (ret < 0)
		dev_err(info->dev, "%s: disable chg timer fail\n", __func__);

	ret = rt9466_set_safety_timer(info, desc->safety_timer);
	if (ret < 0)
		dev_err(info->dev, "%s: set fast timer fail\n", __func__);

	ret = rt9466_enable_safety_timer(info, true);
	if (ret < 0)
		dev_err(info->dev, "%s: enable chg timer fail\n", __func__);

	ret = rt9466_enable_wdt(info, desc->en_wdt);
	if (ret < 0)
		dev_err(info->dev, "%s: set wdt fail\n", __func__);

	ret = rt9466_enable_jeita(info, desc->en_jeita);
	if (ret < 0)
		dev_err(info->dev, "%s: disable jeita fail\n", __func__);

	ret = rt9466_enable_irq_pulse(info, desc->en_irq_pulse);
	if (ret < 0)
		dev_err(info->dev, "%s: set irq pulse fail\n", __func__);

	/* Set ircomp according */
	ret = rt9466_set_ircmp_resistor(info, desc->ircmp_resistor);
	if (ret < 0)
		dev_err(info->dev, "%s: set ircmp resistor fail\n", __func__);

	ret = rt9466_set_ircmp_vclamp(info, desc->ircmp_vclamp);
	if (ret < 0)
		dev_err(info->dev, "%s: set ircmp clamp fail\n", __func__);

	ret = rt9466_set_tdeg_eoc(info, RT9466_TDEG_EOC_16MS);
	if (ret < 0)
		dev_err(info->dev, "%s: set tdeg eoc to 16ms fail\n", __func__);

	ret = rt9466_post_init(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: workaround fail\n", __func__);
		return ret;
	}

	/* Enable HZ mode of secondary charger */
	if (strcmp(info->desc->chg_name, "secondary_chg") == 0) {
		ret = rt9466_enable_hz(true);
		if (ret < 0)
			dev_err(info->dev, "%s: hz sec chg fail\n", __func__);
	}
err:
	return ret;
}

static void rt9466_init_setting_work_handler(struct work_struct *work)
{
	int ret = 0, retry_cnt = 0, adc_vbus = 0;
	bool opa_mode = false;
	struct rt9466_info *info = (struct rt9466_info *)container_of(work,
		struct rt9466_info, init_work);

	do {
		/* Select IINLMTSEL to use AICR */
		ret = rt9466_select_input_current_limit(info,
			RT9466_IINLMTSEL_AICR);
		if (ret < 0) {
			dev_err(info->dev, "%s: sel ilmtsel fail\n", __func__);
			retry_cnt++;
		}
	} while (retry_cnt < 5 && ret < 0);

	msleep(150);

	/* Check pwr_rdy and opa_mode */
	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
	if (ret < 0)
		dev_err(info->dev, "%s, rt9466 get vbus fail\n", __func__);
	ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL1,
				RT9466_SHIFT_OPA_MODE, &opa_mode);
	if (ret < 0)
		dev_err(info->dev, "%s, rt9466 i2c test bit fail\n", __func__);
	if ((adc_vbus < VBUS_THRESHOLD) && !opa_mode)
		ret = rt9466_enable_hz(true);
	else
		ret = rt9466_enable_hz(false);
	retry_cnt = 0;
	do {
		/* Disable hardware ILIM */
		ret = rt9466_enable_ilim(info, false);
		if (ret < 0) {
			dev_err(info->dev, "%s: disable ilim fail\n", __func__);
			retry_cnt++;
		}
	} while (retry_cnt < 5 && ret < 0);
}

static void rt9466_aicr_work_handler(struct work_struct *work)
{
	int ret = 0;
	struct rt9466_info *info = (struct rt9466_info *)container_of(work,
		struct rt9466_info, aicr_work);

	ret = rt9466_run_aicr(info);
	if (ret < 0)
		dev_err(info->dev, "%s: run aicr failed\n", __func__);

	/* Enable MIVR IRQ */
	schedule_delayed_work(&info->mivr_unmask_dwork, msecs_to_jiffies(200));
}

static int rt9466_disable_auto_pmid(struct rt9466_info *info)
{
	int ret;

	dev_info(info->dev, "%s\n", __func__);

	rt9466_enable_hidden_mode(info, true);
	/* disable auto pmid */
	ret = rt9466_clr_bit(info, RT9466_REG_CHG_HIDDEN_CTRL2, 0x10);
	if (ret < 0)
		dev_err(info->dev, "%s: disable auto pmid fail\n", __func__);
	rt9466_enable_hidden_mode(info, false);
	return ret;
}

/* ========================= */
/* I2C driver function       */
/* ========================= */

static int rt9466_probe(struct i2c_client *client,
	const struct i2c_device_id *dev_id)
{
	int ret = 0, adc_vbus = 0;
	struct rt9466_info *info = NULL;
	bool opa_mode = false;

	pr_err("%s(%s)\n", __func__, RT9466_DRV_VERSION);

	info = devm_kzalloc(&client->dev, sizeof(struct rt9466_info),
		GFP_KERNEL);
	if (!info)
		return -ENOMEM;

	mutex_init(&info->io_lock);
	mutex_init(&info->adc_lock);
	mutex_init(&info->irq_lock);
	mutex_init(&info->aicr_lock);
	mutex_init(&info->ichg_lock);
	mutex_init(&info->hidden_mode_lock);
	mutex_init(&info->ieoc_lock);
	info->client = client;
	info->dev = &client->dev;
	info->aicr_limit = -1;
	info->hidden_mode_cnt = 0;
	info->ieoc_wkard = false;
	info->ieoc = 250; /* register default value 250mA */
	memcpy(info->irq_mask, rt9466_irq_maskall, RT9466_IRQIDX_MAX);
	atomic_set(&info->pwr_rdy, 0);

	/* Init wait queue head */
	init_waitqueue_head(&info->wait_queue);

	INIT_WORK(&info->init_work, rt9466_init_setting_work_handler);
	INIT_WORK(&info->irq_work, rt9466_irq_work);
	INIT_WORK(&info->aicr_work, rt9466_aicr_work_handler);
	INIT_DELAYED_WORK(&info->mivr_unmask_dwork, rt9466_mivr_unmask_dwork_handler);

	/* Is HW exist */
	if (!rt9466_is_hw_exist(info)) {
		dev_err(info->dev, "%s: no rt9466 exists\n", __func__);
		ret = -ENODEV;
		goto rt_9466_err_no_dev;
	}
	i2c_set_clientdata(client, info);
	g_rt9466_i2c = client;

	ret = rt9466_disable_auto_pmid(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: disable auto pmid fail\n", __func__);
		goto rt_9466_err_no_dev;
	}

	ret = rt9466_parse_dt(info, &client->dev);
	if (ret < 0) {
		dev_err(info->dev, "%s: parse dt fail\n", __func__);
		goto rt_9466_err_parse_dt;
	}

#ifdef CONFIG_RT_REGMAP
	ret = rt9466_register_rt_regmap(info);
	if (ret < 0)
		goto rt_9466_err_register_regmap;
#endif /* CONFIG_RT_REGMAP */

	/* Check adc_vbus and opa_mode */
	ret = rt9466_get_adc(info, RT9466_ADC_VBUS_DIV5, &adc_vbus);
	if (ret < 0)
		dev_err(info->dev, "%s, rt9466 get vbus fail\n", __func__);
	ret = rt9466_i2c_test_bit(info, RT9466_REG_CHG_CTRL1,
				RT9466_SHIFT_OPA_MODE, &opa_mode);
	if (ret < 0)
		dev_err(info->dev, "%s, rt9466 i2c test bit fail\n", __func__);
	if ((adc_vbus < VBUS_THRESHOLD) && !opa_mode)
		reset_reg_data[0] = RT9466_SW_RESET_HZ;
	else
		reset_reg_data[0] = RT9466_SW_RESET_NO_HZ;

	ret = rt9466_sw_reset(info);
	if (ret < 0)
		pr_err("%s: sw reset fail\n", __func__);
	reset_reg_data[0] = RT9466_SW_RESET_NO_HZ;
/*
	ret = rt9466_reset_chip(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: reset chip fail\n", __func__);
		goto rt_9466_err_reset_chip;
	}
-
+*/

	ret = rt9466_init_setting(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: init setting fail\n", __func__);
		goto rt_9466_err_init_setting;
	}

	ret = charge_ops_register(&rt9466_ops);
	if (ret < 0) {
		dev_err(info->dev, "%s: register charge ops fail!\n", __func__);
		goto rt_9466_err_chgops;
	}
	info->core_data = charge_core_get_params();
	if (NULL == info->core_data){
		dev_err(info->dev, "info->core_data is NULL\n");
		ret = -EINVAL;
		goto rt_9466_err_core_data;
	}

	/*set OTG boost_voltage*/
	ret = rt9466_set_boost_voltage(RT9466_BOOST_VOTGBST);
	if (ret < 0) {
		dev_err(info->dev, "set RT9466 boost voltage fail\n");
	}

	info->irq_active = 1;
	ret = rt9466_irq_register(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: irq register fail\n", __func__);
		goto rt_9466_err_irq_register;
	}

	ret = rt9466_irq_init(info);
	if (ret < 0) {
		dev_err(info->dev, "%s: irq init fail\n", __func__);
		goto rt_9466_err_irq_init;
	}

	schedule_work(&info->init_work);
	dev_err(info->dev, "%s: successfully\n", __func__);
	return ret;

rt_9466_err_irq_init:
rt_9466_err_irq_register:
rt_9466_err_chgops:
rt_9466_err_init_setting:
rt_9466_err_core_data:
#ifdef CONFIG_RT_REGMAP
	rt_regmap_device_unregister(info->regmap_dev);
rt_9466_err_register_regmap:
#endif /* CONFIG_RT_REGMAP */
rt_9466_err_parse_dt:
rt_9466_err_no_dev:
	mutex_destroy(&info->io_lock);
	mutex_destroy(&info->adc_lock);
	mutex_destroy(&info->irq_lock);
	mutex_destroy(&info->aicr_lock);
	mutex_destroy(&info->ichg_lock);
	mutex_destroy(&info->hidden_mode_lock);
	mutex_destroy(&info->ieoc_lock);
	devm_kfree(&client->dev, info);
	info = NULL;
	return ret;
}


static int rt9466_remove(struct i2c_client *client)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(client);

	pr_info("%s\n", __func__);

	if (info) {
#ifdef CONFIG_RT_REGMAP
		rt_regmap_device_unregister(info->regmap_dev);
#endif /* CONFIG_RT_REGMAP */
		mutex_destroy(&info->io_lock);
		mutex_destroy(&info->adc_lock);
		mutex_destroy(&info->irq_lock);
		mutex_destroy(&info->aicr_lock);
		mutex_destroy(&info->ichg_lock);
		mutex_destroy(&info->hidden_mode_lock);
		mutex_destroy(&info->ieoc_lock);
	}

	return ret;
}

static void rt9466_shutdown(struct i2c_client *client)
{
	int ret = 0;
	struct rt9466_info *info = i2c_get_clientdata(client);

	pr_info("%s\n", __func__);
	if (info) {
		ret = rt9466_sw_reset(info);
		if (ret < 0)
			pr_err("%s: sw reset fail\n", __func__);
	}
}

static int rt9466_suspend(struct device *dev)
{
	struct rt9466_info *info = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);
	if (device_may_wakeup(dev))
		enable_irq_wake(info->irq);

	return 0;
}

static int rt9466_resume(struct device *dev)
{
	struct rt9466_info *info = dev_get_drvdata(dev);

	dev_info(dev, "%s\n", __func__);
	if (device_may_wakeup(dev))
		disable_irq_wake(info->irq);

	return 0;
}

static SIMPLE_DEV_PM_OPS(rt9466_pm_ops, rt9466_suspend, rt9466_resume);

static const struct i2c_device_id rt9466_i2c_id[] = {
	{"rt9466", 0},
	{}
};
MODULE_DEVICE_TABLE(i2c, rt9466_i2c_id);

static const struct of_device_id rt9466_of_match[] = {
	{ .compatible = "richtek,rt9466", },
	{},
};
MODULE_DEVICE_TABLE(of, rt9466_of_match);

#ifndef CONFIG_OF
#define RT9466_BUSNUM 1

static struct i2c_board_info rt9466_i2c_board_info __initdata = {
	I2C_BOARD_INFO("rt9466", RT9466_SALVE_ADDR)
};
#endif /* CONFIG_OF */


static struct i2c_driver rt9466_i2c_driver = {
	.driver = {
		.name = "rt9466",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt9466_of_match),
		.pm = &rt9466_pm_ops,
	},
	.probe = rt9466_probe,
	.remove = rt9466_remove,
	.shutdown = rt9466_shutdown,
	.id_table = rt9466_i2c_id,
};

static int __init rt9466_init(void)
{
	int ret = 0;

#ifdef CONFIG_OF
	pr_info("%s: with dts\n", __func__);
#else
	pr_info("%s: without dts\n", __func__);
	i2c_register_board_info(RT9466_BUSNUM, &rt9466_i2c_board_info, 1);
#endif

	ret = i2c_add_driver(&rt9466_i2c_driver);
	if (ret < 0)
		pr_err("%s: register i2c driver fail\n", __func__);

	return ret;
}
module_init(rt9466_init);


static void __exit rt9466_exit(void)
{
	i2c_del_driver(&rt9466_i2c_driver);
}
module_exit(rt9466_exit);


MODULE_LICENSE("GPL");
MODULE_AUTHOR("ShuFanLee <shufan_lee@richtek.com>");
MODULE_DESCRIPTION("RT9466 Charger Driver");
MODULE_VERSION(RT9466_DRV_VERSION);
