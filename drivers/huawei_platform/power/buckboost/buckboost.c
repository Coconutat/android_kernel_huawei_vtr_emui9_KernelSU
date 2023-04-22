#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_device.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/mutex.h>
#include <huawei_platform/log/hw_log.h>
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#include "buckboost.h"

#define HWLOG_TAG max77813

HWLOG_REGIST();
struct max77813_device_info *g_max77813_dev;

/**********************************************************
*  Function:       max77813_read_block
*  Discription:    register read block interface
*  Parameters:   di:max77813_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int max77813_read_block(struct max77813_device_info *di, u8 *value,
				u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[2];
	u8 buf = 0;
	int ret = 0;

	if(NULL == di){
		hwlog_err("%s:NULL POINTER!!\n",__func__);
		return -1;
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
	if (ret != 2) {
		hwlog_err("i2c_write failed to transfer all messages\n");
		if (ret < 0)
			return ret;
		else
			return -EIO;
	} else {
		return 0;
	}
}

/**********************************************************
*  Function:       max77813_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int max77813_read_byte(u8 reg, u8 *value)
{
	struct max77813_device_info *di = g_max77813_dev;
	return max77813_read_block(di, value, reg, 1);
}
static int max77813_write_block(struct max77813_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
{
	struct i2c_msg msg[1];
	int ret = 0;

	if(NULL == di){
		hwlog_err("%s:NULL POINTER!!\n",__func__);
		return -1;
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
static int max77813_write_byte(u8 reg, u8 value)
{
	struct max77813_device_info *di = g_max77813_dev;
	/* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */
	u8 temp_buffer[2] = { 0 };

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return max77813_write_block(di, temp_buffer, reg, 1);
}
static int max77813_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = max77813_read_byte(reg, &val);
	if (ret < 0)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = max77813_write_byte(reg, val);

	return ret;
}
static int max77813_sysfs_create_group(struct max77813_device_info *di)
{
	return 0;
}
static int max77813_sysfs_remove_group(struct max77813_device_info *di)
{
	return 0;
}
int max77813_forced_pwm_enable(int enable)
{
	int ret;
	u8 reg;
	u8 value = enable ? 0x1 : 0x0;

	ret = max77813_read_byte(MAX77813_CONFIG1, &reg);
	if (ret)
	{
		hwlog_err("%s: read fail\n", __func__);
		return -1;
	}
	hwlog_info("%s:before write config1 reg = 0x%x\n", __func__, reg);
	ret = max77813_write_mask(MAX77813_CONFIG1,MAX77813_FORCED_PWM_MASK, MAX77813_FORCED_PWM_SHIFT, value);
	if (ret)
	{
		hwlog_err("%s: max77813 pwm enable fail, val = %d\n", __func__, enable);
		return -1;
	}
	ret = max77813_read_byte(MAX77813_CONFIG1, &reg);
	if (ret)
	{
		hwlog_err("%s: read fail\n", __func__);
		return -1;
	}
	hwlog_info("%s:config1 reg = 0x%x\n", __func__, reg);
	return 0;
}

/**********************************************************
*  Function:       max77813_device_check
*  Discription:    check chip i2c communication
*  Parameters:   null
*  return value:  0-sucess or others-fail
**********************************************************/
static int max77813_device_check(void)
{
	int ret = 0;
	u8 reg = 0xff;
	ret |= max77813_read_byte(MAX77813_INFO_REG00, &reg);
	hwlog_info("value read from max77813 reg0 =  %x\n",reg);
	if (ret) {
		hwlog_err(" max77813 dev check error.\n");
		return -1;
	}
	return 0;
}
static void max77813_irq_work(struct work_struct *work)
{
	u8 reg_irq ;
	max77813_read_byte(MAX77813_STATUS_REG01, &reg_irq);
	hwlog_info("max77813_status_reg01 = 0x%x\n", reg_irq);
	return;
}

static irqreturn_t max77813_interrupt(int irq, void *_di)
{
	struct max77813_device_info *di = _di;
	hwlog_info("max77813 interrupt\n");
	disable_irq_nosync(di->irq_pok);
	schedule_work(&di->irq_work);
	return IRQ_HANDLED;
}

static void max77813_prase_dts(struct device_node *np, struct max77813_device_info *di)
{
	return;
}

/**********************************************************
*  Function:       max77813_probe
*  Discription:    max77813 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int max77813_probe(struct i2c_client *client,
			 const struct i2c_device_id *id)
{
	int ret = 0;
	struct max77813_device_info *di = NULL;
	struct device_node *np = NULL;
	di = devm_kzalloc(&client->dev, sizeof(*di), GFP_KERNEL);
	if (!di) {
		hwlog_err("max77813_device_info is NULL!\n");
		return -ENOMEM;
	}
	g_max77813_dev = di;
	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);
	max77813_prase_dts(np, di);
	INIT_WORK(&di->irq_work, max77813_irq_work);
	di->gpio_pok = of_get_named_gpio(np, "gpio_pok", 0);
	if (!gpio_is_valid(di->gpio_pok)) {
		hwlog_err("gpio_pok is not valid\n");
		goto max77813_fail_0;
	}
	ret = gpio_request(di->gpio_pok, "buckboost_max77813");
	if (ret) {
		hwlog_err("could not request gpio_pok\n");
		goto max77813_fail_0;
	}
	gpio_direction_input(di->gpio_pok);
	di->irq_pok = gpio_to_irq(di->gpio_pok);
	if (di->irq_pok < 0) {
		hwlog_err("could not map gpio_pok to irq\n");
		goto max77813_fail_1;
	}
	ret = request_irq(di->irq_pok, max77813_interrupt,IRQF_TRIGGER_FALLING, "max77813  buckboost interrupt", di);
	if (ret) {
		hwlog_err("could not request irq_int\n");
		di->irq_pok = -1;
		goto max77813_fail_1;
	}
	ret = max77813_sysfs_create_group(di);
	if (ret) {
		hwlog_err("create sysfs entries failed!\n");
		goto max77813_fail_2;
	}
	ret = max77813_device_check();
	if (ret) {
		hwlog_err("max77813 dev_check failed!\n");
		goto max77813_fail_3;
	}
	#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_BUCKBOOST_MAX77813);
	#endif
	hwlog_info("max77813 probe ok!\n");
	return 0;

max77813_fail_3:
	max77813_sysfs_remove_group(di);
max77813_fail_2:
	free_irq(di->irq_pok, di);
max77813_fail_1:
	gpio_free(di->gpio_pok);
max77813_fail_0:
	g_max77813_dev = NULL;
	np = NULL;
	return -1;
}

/**********************************************************
*  Function:       max77813_remove
*  Discription:    max77813 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int max77813_remove(struct i2c_client *client)
{
	struct max77813_device_info *di = i2c_get_clientdata(client);
	max77813_sysfs_remove_group(di);
	if (di->gpio_pok)
	{
		gpio_free(di->gpio_pok);
	}
	if (di->irq_pok)
	{
		free_irq(di->irq_pok, di);
	}
	return 0;
}

MODULE_DEVICE_TABLE(i2c, max77813);
static struct of_device_id max77813_of_match[] = {
	{
		.compatible = "huawei,max77813",
		.data = NULL,
	},
	{
	},
};

static const struct i2c_device_id max77813_i2c_id[] = {
	{"max77813", 0}, {}
};

static struct i2c_driver max77813_driver = {
	.probe = max77813_probe,
	.remove = max77813_remove,
	.id_table = max77813_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "max77813",
		.of_match_table = of_match_ptr(max77813_of_match),
	},
};

/**********************************************************
*  Function:       max77813_init
*  Discription:    max77813 module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init max77813_init(void)
{
	int ret = 0;
	ret = i2c_add_driver(&max77813_driver);
	if (ret)
		hwlog_err("%s: i2c_add_driver error!!!\n", __func__);
	return ret;
}

/**********************************************************
*  Function:       max77813_exit
*  Discription:    max77813 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit max77813_exit(void)
{
	i2c_del_driver(&max77813_driver);
}

module_init(max77813_init);
module_exit(max77813_exit);
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("buckboost module max77813 driver");
MODULE_AUTHOR("HW Inc");

