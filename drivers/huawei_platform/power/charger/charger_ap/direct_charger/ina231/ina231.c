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
#include "ina231.h"
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_direct_charger.h>
#endif

#define HWLOG_TAG ina231_for_charge
HWLOG_REGIST();

struct ina231_device_info *g_ina231_dev;
static int ina231_init_finish_flag = INA231_NOT_INIT;
static int ina231_interrupt_notify_enable_flag = INA231_DISABLE_INTERRUPT_NOTIFY;

/*
	current_lsb = (maximum expected current) / 2^15
	callibration = 0.00512 / (current_lsb * Rshunt)
	current = (shunt_voltage * calibration) / 2048
	power = (current * bus_voltage) / 20000
*/
static struct ina231_config_data ina231_config = {
	 /* INA231_CONFIG_REG [0x4124]: number of averages:1, Vbus ct:1.1ms, Vsh ct:1.1ms, powerdown mode */
	.config_sleep_in =
		((INA231_CONFIG_MODE_PD1 << INA231_CONFIG_MODE_SHIFT) |
		(INA231_CONFIG_CT_1100US << INA231_CONFIG_VSHUNTCT_SHIFT) |
		(INA231_CONFIG_CT_1100US << INA231_CONFIG_VBUSCT_SHIFT) |
		(INA231_CONFIG_AVG_NUM_1 << INA231_CONFIG_AVG_SHIFT)),

	/* INA231_CONFIG_REG [0x8000]: reset the whole chip */
	.config_reset = (INA231_CONFIG_RST_ENABLE << INA231_CONFIG_RST_SHIFT),

	/* INA231_CONFIG_REG [0x4377]: number of averages:4, Vbus ct:2.116ms, Vsh ct:4.156ms, shunt and bus continous mode */
	.config_work =
		 ((INA231_CONFIG_MODE_SB_CONS << INA231_CONFIG_MODE_SHIFT) |
		 (INA231_CONFIG_CT_4156US << INA231_CONFIG_VSHUNTCT_SHIFT) |
		 (INA231_CONFIG_CT_2116US << INA231_CONFIG_VBUSCT_SHIFT) |
		 (INA231_CONFIG_AVG_NUM_4 << INA231_CONFIG_AVG_SHIFT)),

	/* INA231_CALIBRATION_REG [0x1000] */
	.calibrate_content = 0x400,

	/* INA231_MASK_ENABLE_REG [0x01] */
	.mask_enable_content =
		(1 << INA231_MASK_ENABLE_LEN_SHIFT),

	/* ALERT_LIMIT [0x0] */
	.alert_limit_content = 0x0,

	.shunt_voltage_lsb = 2500, /* 2500 nV/bit */
	.bus_voltage_lsb = 1250, /* 1250 uV/bit */
	.current_lsb = 500, /* 500 uA/bit */
};

#ifdef CONFIG_HUAWEI_POWER_DEBUG
static ssize_t ina231_dbg_show(void *dev_data, char *buf, size_t size)
{
	struct ina231_device_info *dev_p = (struct ina231_device_info *)dev_data;

	if (!dev_p) {
		hwlog_err("error: i2c_get_clientdata return null!\n");
		return scnprintf(buf, size, "i2c_get_clientdata return null!\n");
	}

	return scnprintf(buf, size,
		"ina231_calibrate_content=0x%x\n",
		dev_p->config->calibrate_content);
}

static ssize_t ina231_dbg_store(void *dev_data, const char *buf, size_t size)
{
	struct ina231_device_info *dev_p = (struct ina231_device_info *)dev_data;
	unsigned int calibrate_content = 0;

	if (!dev_p) {
		hwlog_err("error: i2c_get_clientdata return null!\n");
		return -EINVAL;
	}

	if (sscanf(buf, "%d", &calibrate_content) != 1) {
		hwlog_err("error: unable to parse input:%s\n", buf);
		return -EINVAL;
	}

	dev_p->config->calibrate_content = calibrate_content;

	hwlog_info("calibrate_content=0x%x\n",
		dev_p->config->calibrate_content);

	return size;
}
#endif

static int ina231_get_shunt_voltage_mv(int *val)
{
	struct ina231_device_info *di = g_ina231_dev;
	struct i2c_client *client = NULL;
	s16 reg_value = 0;
	int lsb_value = 0;
	int ret = 0;

	if (NULL == di || NULL == di->client) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	client = di->client;
	lsb_value = di->config->shunt_voltage_lsb;

	ret = i2c_smbus_read_word_swapped(client, INA231_SHUNT_VOLTAGE_REG);
	if (ret < 0) {
		hwlog_err("error: shunt_voltage_reg read fail!\n");
		return -1;
	}

	reg_value = (s16)ret;
	*val = (int)(reg_value * lsb_value) / INA231_NV_TO_MV;

	hwlog_info("shunt_voltage=%d, VOLTAGE_REG=0x%x\n", *val, reg_value);
	return 0;
}

static int ina231_get_bus_voltage_mv(int *val)
{
	struct ina231_device_info *di = g_ina231_dev;
	struct i2c_client *client = NULL;
	s16 reg_value = 0;
	int lsb_value = 0;
	int ret = 0;

	if (NULL == di || NULL == di->client) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	client = di->client;
	lsb_value = di->config->bus_voltage_lsb;

	ret = i2c_smbus_read_word_swapped(client, INA231_BUS_VOLTAGE_REG);
	if (ret < 0) {
		hwlog_err("error: bus_voltage_reg read fail!\n");
		return -1;
	}

	reg_value = (s16)ret;
	*val = (int)(reg_value * lsb_value) / INA231_UV_TO_MV;

	hwlog_info("voltage=%d, VOLTAGE_REG=0x%x\n", *val, reg_value);
	return 0;
}

static int ina231_get_current_ma(int* val)
{
	struct ina231_device_info *di = g_ina231_dev;
	struct i2c_client *client = NULL;
	s16 reg_value = 0;
	int lsb_value = 0;
	int ret = 0;

	if (NULL == di || NULL == di->client) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	client = di->client;
	lsb_value = di->config->current_lsb;

	ret= i2c_smbus_read_word_swapped(client, INA231_CURRENT_REG);
	if (ret < 0) {
		hwlog_err("error: current_reg read fail!\n");
		return -1;
	}

	reg_value = (s16)ret;
	*val = (int)(reg_value * lsb_value) / INA231_UA_TO_MA;

	hwlog_info("current=%d, CURRENT_REG=0x%x\n", *val, reg_value);
	return 0;
}

static int ina231_get_vbat_mv(void)
{
	/* to be doing */
	return 0;
}

static int ina231_get_ibat_ma(int* val)
{
	/* to be doing */
	*val = 0;

	return 0;
}

static int ina231_get_device_temp(int *temp)
{
	*temp = INA231_DEVICE_DEFAULT_TEMP;

	return 0;
}

static int ina231_dump_register(void)
{
	struct ina231_device_info *di = g_ina231_dev;
	struct i2c_client *client = NULL;
	int i = 0;
	int shunt_volt = 0;
	int bus_volt = 0;
	int cur = 0;

	if (NULL == di || NULL == di->client) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	client = di->client;

	for (i = 0; i < INA231_MAX_REGS; ++i) {
		hwlog_info("reg [%d]=0x%x\n", i, i2c_smbus_read_word_swapped(client, i));
	}

	ina231_get_shunt_voltage_mv(&shunt_volt);
	ina231_get_bus_voltage_mv(&bus_volt);
	ina231_get_current_ma(&cur);

	hwlog_info("shunt_voltage=%d, bus_voltage=%d, current=%d\n", shunt_volt, bus_volt, cur);
	return 0;
}

static int ina231_device_reset(void)
{
	struct ina231_device_info *di = g_ina231_dev;
	struct i2c_client *client = NULL;
	int ret = 0;

	if (NULL == di || NULL == di->client) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	client = di->client;

	/* communication check and reset device */
	ret = i2c_smbus_write_word_swapped(client, INA231_CONFIG_REG, di->config->config_reset);
	if (ret < 0) {
		hwlog_err("error: device_reset fail!\n");
		return -1;
	}

	mdelay(20);

	/* device goto sleep */
	i2c_smbus_write_word_swapped(client, INA231_CONFIG_REG, di->config->config_sleep_in);
	if (ret < 0) {
		hwlog_err("error: device_reset fail!\n");
		return -1;
	}

	return 0;
}

static int ina231_reg_init(void)
{
	struct ina231_device_info *di = g_ina231_dev;
	struct i2c_client *client = NULL;
	int ret = 0;

	if (NULL == di || NULL == di->client) {
		hwlog_err("error: di or client is null!\n");
		return -1;
	}

	client = di->client;

	ret = i2c_smbus_write_word_swapped(client, INA231_CONFIG_REG, di->config->config_work);
	ret |= i2c_smbus_write_word_swapped(client, INA231_CALIBRATION_REG, di->config->calibrate_content);
	ret |= i2c_smbus_write_word_swapped(client, INA231_MASK_ENABLE_REG, di->config->mask_enable_content);
	ret |= i2c_smbus_write_word_swapped(client, INA231_ALERT_LIMIT_REG, di->config->alert_limit_content);
	if (ret) {
		hwlog_err("error: reg_init fail!\n");
		return -1;
	}

	msleep(100);

	ina231_init_finish_flag = INA231_INIT_FINISH;

	return 0;
}

static int ina231_batinfo_exit(void)
{
	return 0;
}

static void ina231_interrupt_work(struct work_struct *work)
{
	struct ina231_device_info *di = container_of(work, struct ina231_device_info, irq_work);
	u16 mask_enable;
	u16 alert_limit;

	mask_enable = i2c_smbus_read_word_swapped(di->client, INA231_MASK_ENABLE_REG);
	alert_limit = i2c_smbus_read_word_swapped(di->client, INA231_ALERT_LIMIT_REG);

	if (INA231_ENABLE_INTERRUPT_NOTIFY == ina231_interrupt_notify_enable_flag) {
		ina231_dump_register();
	}

	hwlog_err("mask_enable_reg [%x]=0x%x\n", INA231_MASK_ENABLE_REG, mask_enable);
	hwlog_err("alert_limit_reg [%x]=0x%x\n", INA231_ALERT_LIMIT_REG, alert_limit);

	/* clear irq */
	enable_irq(di->irq_int);
}

static irqreturn_t ina231_interrupt(int irq, void *_di)
{
	struct ina231_device_info *di = _di;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	if (0 == di->chip_already_init) {
		hwlog_err("error: chip not init!\n");
	}

	if (INA231_INIT_FINISH == ina231_init_finish_flag) {
		ina231_interrupt_notify_enable_flag = INA231_ENABLE_INTERRUPT_NOTIFY;
	}

	hwlog_info("ina231 interrupt happened(%d)!\n", ina231_init_finish_flag);

	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}

static struct batinfo_ops ina231_batinfo_ops = {
	.init = ina231_reg_init,
	.exit = ina231_batinfo_exit,
	.get_bat_btb_voltage = ina231_get_vbat_mv,
	.get_bat_package_voltage = ina231_get_vbat_mv,
	.get_vbus_voltage = ina231_get_bus_voltage_mv,
	.get_bat_current = ina231_get_ibat_ma,
	.get_ls_ibus = ina231_get_current_ma,
	.get_ls_temp = ina231_get_device_temp,
};

static int ina231_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct ina231_device_info *di = NULL;
	struct device_node *np = NULL;
	unsigned int calibrate_content;

	hwlog_info("probe begin\n");

	if (NULL == client || NULL == id) {
		hwlog_err("error: client is null or id is null!\n");
		return -ENOMEM;
	}

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_WORD_DATA)) {
		hwlog_err("error: i2c_check failed!\n");
		return -ENODEV;
	}

	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("error: kzalloc failed!\n");
		return -ENOMEM;
	}
	g_ina231_dev = di;

	di->config = &ina231_config;
	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	INIT_WORK(&di->irq_work, ina231_interrupt_work);

	ret = of_property_read_u32(np, "calibrate_content", &calibrate_content);
	if (ret == 0) {
		ina231_config.calibrate_content = (u16)calibrate_content;
		hwlog_info("calibrate_content=0x%x (use dts value)\n", ina231_config.calibrate_content);
	}
	else {
		hwlog_info("calibrate_content=0x%x (use default value)\n", ina231_config.calibrate_content);
	}

	di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
	hwlog_info("gpio_int=%d\n", di->gpio_int);

	if (!gpio_is_valid(di->gpio_int)) {
		hwlog_err("error: gpio(gpio_int) is not valid!\n");
		ret = -EINVAL;
		goto ina231_fail_0;
	}

	ret = gpio_request(di->gpio_int, "ina231_gpio_int");
	if (ret) {
		hwlog_err("error: gpio(gpio_int) request fail!\n");
		ret = -EINVAL;
		goto ina231_fail_0;
	}

	ret = gpio_direction_input(di->gpio_int);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) set input fail!\n");
		goto ina231_fail_1;
	}

	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0) {
		hwlog_err("error: gpio(gpio_int) map to irq fail!\n");
		ret = -EINVAL;
		goto ina231_fail_1;
	}

	ret = request_irq(di->irq_int, ina231_interrupt, IRQF_TRIGGER_FALLING, "ina231_int_irq", di);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) irq request fail!\n");
		ret = -EINVAL;
		di->irq_int = -1;
		goto ina231_fail_1;
	}

	ret = batinfo_lvc_ops_register(&ina231_batinfo_ops);
	if (ret) {
		hwlog_err("error: ina231 batinfo ops register fail!\n");
		goto ina231_fail_2;
	}

	ret = batinfo_sc_ops_register(&ina231_batinfo_ops);
	if (ret) {
		hwlog_err("error: ina231 batinfo ops register fail!\n");
		goto ina231_fail_2;
	}

#ifdef CONFIG_WIRELESS_CHARGER
	ret = wireless_sc_batinfo_ops_register(&ina231_batinfo_ops);
	if (ret) {
		hwlog_err("error: ina231 wireless_sc batinfo ops register fail!\n");
		goto ina231_fail_2;
	}
#endif

#ifdef CONFIG_HUAWEI_POWER_DEBUG
	power_dbg_ops_register("ina231_para", i2c_get_clientdata(client),
		(power_dgb_show)ina231_dbg_show, (power_dgb_store)ina231_dbg_store);
#endif

	di->chip_already_init = 1;

	ret = ina231_device_reset();
	if (ret) {
		hwlog_err("error: ina231 reg reset fail!\n");
		di->chip_already_init = 0;
		goto ina231_fail_2;
	}

	ret = ina231_reg_init();
	if (ret) {
		hwlog_err("error: ina231 reg init fail!\n");
		di->chip_already_init = 0;
		goto ina231_fail_2;
	}

	ina231_dump_register();

	hwlog_info("probe end\n");
	return 0;

ina231_fail_2:
	free_irq(di->irq_int, di);
ina231_fail_1:
	gpio_free(di->gpio_int);
ina231_fail_0:
	g_ina231_dev = NULL;
	devm_kfree(&client->dev, di);
	return ret;
}

static int ina231_remove(struct i2c_client *client)
{
	struct ina231_device_info *di = i2c_get_clientdata(client);

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

static void ina231_shutdown(struct i2c_client *client)
{
	ina231_device_reset();
}

MODULE_DEVICE_TABLE(i2c, ina231);

static struct of_device_id ina231_of_match[] = {
	{
		.compatible = "huawei,ina231_for_charge",
		.data = NULL,
	},
	{ },
};

static const struct i2c_device_id ina231_i2c_id[] = {
	{"ina231_for_charge", 0}, { }
};

static struct i2c_driver ina231_driver = {
	.probe = ina231_probe,
	.remove = ina231_remove,
	.shutdown = ina231_shutdown,
	.id_table = ina231_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "huawei_ina231_for_charge",
		.of_match_table = of_match_ptr(ina231_of_match),
	},
};

static int __init ina231_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&ina231_driver);
	if (ret) {
		hwlog_err("error: ina231 i2c_add_driver error!\n");
	}

	return ret;
}

static void __exit ina231_exit(void)
{
	i2c_del_driver(&ina231_driver);
}

module_init(ina231_init);
module_exit(ina231_exit);

MODULE_DESCRIPTION("ina231 module driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("HW Inc");