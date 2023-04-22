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
#include "../dual_loadswitch.h"
#include "fpf2283.h"
#ifdef CONFIG_WIRELESS_CHARGER
#include <huawei_platform/power/wireless_direct_charger.h>
#endif

#define HWLOG_TAG fpf2283_aux
HWLOG_REGIST();

static struct fpf2283_device_info *g_fpf2283_dev;
static int fpf2283_init_finish_flag = FPF2283_NOT_INIT;
static int fpf2283_interrupt_notify_enable_flag = FPF2283_DISABLE_INTERRUPT_NOTIFY;

#define MSG_LEN                      (2)

/**********************************************************
*  Function:       fpf2283_write_block
*  Discription:    register write block interface
*  Parameters:   di:fpf2283_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int fpf2283_write_block(struct fpf2283_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
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
*  Function:       fpf2283_read_block
*  Discription:    register read block interface
*  Parameters:   di:fpf2283_device_info
*                      value:register value
*                      reg:register name
*                      num_bytes:bytes number
*  return value:  0-sucess or others-fail
**********************************************************/
static int fpf2283_read_block(struct fpf2283_device_info *di, u8 *value, u8 reg, unsigned num_bytes)
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
*  Function:       fpf2283_write_byte
*  Discription:    register write byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fpf2283_write_byte(u8 reg, u8 value)
{
	struct fpf2283_device_info *di = g_fpf2283_dev;
	u8 temp_buffer[MSG_LEN] = { 0 }; /* 2 bytes offset 1 contains the data offset 0 is used by i2c_write */

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	/* offset 1 contains the data */
	temp_buffer[1] = value;
	return fpf2283_write_block(di, temp_buffer, reg, 1);
}

/**********************************************************
*  Function:       fpf2283_read_byte
*  Discription:    register read byte interface
*  Parameters:   reg:register name
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fpf2283_read_byte(u8 reg, u8 *value)
{
	struct fpf2283_device_info *di = g_fpf2283_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -ENOMEM;
	}

	return fpf2283_read_block(di, value, reg, 1);
}

/**********************************************************
*  Function:       fpf2283_write_mask
*  Discription:    register write mask interface
*  Parameters:   reg:register name
*                      MASK:mask value of the function
*                      SHIFT:shift number of the function
*                      value:register value
*  return value:  0-sucess or others-fail
**********************************************************/
static int fpf2283_write_mask(u8 reg, u8 MASK, u8 SHIFT, u8 value)
{
	int ret = 0;
	u8 val = 0;

	ret = fpf2283_read_byte(reg, &val);
	if (ret < 0)
		return ret;

	val &= ~MASK;
	val |= ((value << SHIFT) & MASK);

	ret = fpf2283_write_byte(reg, val);

	return ret;
}

static void fpf2283_dump_register(void)
{
	u8 i = 0;
	int ret = 0;
	u8 val = 0;

	for (i = 0; i < FPF2283_MAX_REGS; ++i) {
		ret = fpf2283_read_byte(i, &val);
		if (ret) {
			hwlog_err("error: dump_register read fail!\n");
		}
		hwlog_info("reg [%x]=0x%x\n", i, val);
	}
}

/**********************************************************
*  Function:       fpf2283_reg_init
*  Discription:
*  Parameters:
*  return value:   0-sucess or others-fail
**********************************************************/
static int fpf2283_reg_init(void)
{
	int ret;

	ret = fpf2283_write_byte(FPF2283_ENABLE_REG, FPF2283_ENABLE_INIT);
	ret |= fpf2283_write_byte(FPF2283_INTERRUPT_MASK_REG, FPF2283_INTERRUPT_MASK_INIT);
	ret |= fpf2283_write_mask(FPF2283_OVP_REG, FPF2283_OVP_OFFSET_MASK, FPF2283_OVP_OFFSET_SHIFT, FPF2283_OVP_OFFSET_0MV);
	ret |= fpf2283_write_mask(FPF2283_OVP_REG, FPF2283_OVP_CENTER_VALUE_MASK, FPF2283_OVP_CENTER_VALUE_SHIFT, FPF2283_OVP_CENTER_VALUE_11500MV);
	ret |= fpf2283_write_mask(FPF2283_ISRC_AMPLITUDE_REG, FPF2283_ISRC_AMPLITUDE_MASK, FPF2283_ISRC_AMPLITUDE_SHIFT, FPF2283_ISRC_AMPLITUDE_0UA);
	ret |= fpf2283_write_mask(FPF2283_ISRC_PULSE_REG, FPF2283_ISRC_PULSE_TDET_MASK, FPF2283_ISRC_PULSE_TDET_SHIFT, FPF2283_ISRC_PULSE_TDET_200US);
	ret |= fpf2283_write_mask(FPF2283_ISRC_PULSE_REG, FPF2283_ISRC_PULSE_TBLK_MASK, FPF2283_ISRC_PULSE_TBLK_SHIFT, FPF2283_ISRC_PULSE_TBLK_SINGLE_PULSE);

	if (ret) {
		hwlog_err("error: reg_init fail!\n");
		return -1;
	}

	return 0;
}

/**********************************************************
*  Function:        fpf2283_software_charge_enable
*  Discription:    Software bit for charge enable
*  Parameters:     enable: 1   disable: 0
*  return value:   0-sucess or others-fail
**********************************************************/
static int fpf2283_charge_enable(int enable)
{
	u8 reg = 0;
	int ret;
	u8 value = enable ? 0x0 : 0x1;

	ret = fpf2283_write_mask(FPF2283_ENABLE_REG, FPF2283_ENABLE_SW_ENB_MASK, FPF2283_ENABLE_SW_ENB_SHIFT, value);
	if (ret) {
		hwlog_err("error: charge_enable write fail!\n");
		return -1;
	}

	ret = fpf2283_read_byte(FPF2283_ENABLE_REG, &reg);
	if (ret) {
		hwlog_err("error: charge_enable read fail!\n");
		return -1;
	}

	hwlog_info("charge_enable [%x]=0x%x\n", FPF2283_ENABLE_REG, reg);
	return 0;
}

static int fpf2283_charge_init(void)
{
	int ret = 0;
	struct fpf2283_device_info *di = g_fpf2283_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	di->chip_already_init = 1;

	ret = fpf2283_reg_init();
	if (ret) {
		return -1;
	}

	fpf2283_init_finish_flag = FPF2283_INIT_FINISH;
	return 0;
}

static int fpf2283_charge_exit(void)
{
	int ret = 0;
	struct fpf2283_device_info *di = g_fpf2283_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	ret = fpf2283_charge_enable(FPF2283_ENABLE_SW_ENB_DISABLED);

	/* pull down reset pin to reset fpf2283 */
	ret = gpio_direction_output(di->gpio_en, FPF2283_CHIP_DISABLE);
	if (ret) {
		hwlog_err("error: gpio(gpio_en) disable fail!\n");
		return -1;
	}

	di->chip_already_init = 0;
	fpf2283_init_finish_flag = FPF2283_NOT_INIT;
	fpf2283_interrupt_notify_enable_flag = FPF2283_DISABLE_INTERRUPT_NOTIFY;

	msleep(10);

	return ret;
}

static int fpf2283_discharge(int enable)
{
	return 0;
}

static int fpf2283_config_watchdog_ms(int time)
{
	return 0;
}

/**********************************************************
*  Function:        fpf2283_is_device_close
*  Discription:     whether fpf2283 is close
*  Parameters:    void
*  return value:   0: not close ,1: close
**********************************************************/
static int fpf2283_is_ls_close(void)
{
	u8 reg = 0;
	int ret = 0;

	ret = fpf2283_read_byte(FPF2283_ENABLE_REG, &reg);
	if (ret) {
		hwlog_err("error: is_ls_close read fail!\n");
		return 1;
	}

	if (reg & FPF2283_ENABLE_SW_ENB_MASK) {
		return 1;
	}

	return 0;
}

static int fpf2283_ls_status(void)
{
	struct fpf2283_device_info *di = g_fpf2283_dev;

	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	hwlog_info("ls_status=%d\n", di->chip_already_init);

	if (di->chip_already_init == 1) {
		return 0;
	}

	return -1;
}

#if 0
/**********************************************************
*  Function:       fpf2283_irq_work
*  Discription:    handler for loadswitch fault irq in charging process
*  Parameters:   work:loadswitch fault interrupt workqueue
*  return value:  NULL
**********************************************************/
static void fpf2283_irq_work(struct work_struct *work)
{
	struct fpf2283_device_info *di = container_of(work, struct fpf2283_device_info, irq_work);
	u8 detection_status;
	u8 power_switch_flag;
	u8 interrupt_mask;
	struct nty_data * data = &(di->nty_data);
	struct atomic_notifier_head *direct_charge_fault_notifier_list;

	direct_charge_lvc_get_fault_notifier(&direct_charge_fault_notifier_list);

	fpf2283_read_byte(FPF2283_DETECTION_STATUS_REG, &detection_status);
	fpf2283_read_byte(FPF2283_POWER_SWITCH_FLAG_REG, &power_switch_flag);
	fpf2283_read_byte(FPF2283_INTERRUPT_MASK_REG, &interrupt_mask);

	data->event1 = detection_status;
	data->event2 = power_switch_flag;
	data->addr = di->client->addr;

	if (FPF2283_ENABLE_INTERRUPT_NOTIFY == fpf2283_interrupt_notify_enable_flag) {
		if (power_switch_flag & FPF2283_POWER_SWITCH_FLAG_OV_FLG_MASK) {
			hwlog_err("ovp happened\n");
			atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_VBUS_OVP, data);
		}
		else if (power_switch_flag & FPF2283_POWER_SWITCH_FLAG_OC_FLG_MASK) {
			hwlog_err("ocp happened\n");
			atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_INPUT_OCP, data);
		}
		else if (power_switch_flag & FPF2283_POWER_SWITCH_FLAG_OT_FLG_MASK) {
			hwlog_err("otp happened\n");
			atomic_notifier_call_chain(direct_charge_fault_notifier_list, DIRECT_CHARGE_FAULT_OTP, data);
		}
		else {
			/*do nothing*/
		}

		fpf2283_dump_register();
	}

	hwlog_err("detection_status_reg [%x]=0x%x\n", FPF2283_DETECTION_STATUS_REG, detection_status);
	hwlog_err("power_switch_flag_reg [%x]=0x%x\n", FPF2283_POWER_SWITCH_FLAG_REG, power_switch_flag);
	hwlog_err("interrupt_mask_reg [%x]=0x%x\n", FPF2283_INTERRUPT_MASK_REG, interrupt_mask);

	/* clear irq */
	enable_irq(di->irq_int);
}

/**********************************************************
*  Function:       fpf2283_interrupt
*  Discription:    callback function for loadswitch fault irq in charging process
*  Parameters:   irq:loadswitch fault interrupt
*                      _di:fpf2283_device_info
*  return value:  IRQ_HANDLED-sucess or others
**********************************************************/
static irqreturn_t fpf2283_interrupt(int irq, void *_di)
{
	struct fpf2283_device_info *di = _di;
	if (NULL == di) {
		hwlog_err("error: di is null!\n");
		return -1;
	}

	if (0 == di->chip_already_init) {
		hwlog_err("error: chip not init!\n");
	}

	if (FPF2283_INIT_FINISH == fpf2283_init_finish_flag) {
		fpf2283_interrupt_notify_enable_flag = FPF2283_ENABLE_INTERRUPT_NOTIFY;
	}

	hwlog_info("fpf2283 interrupt happened(%d)!\n", fpf2283_init_finish_flag);

	disable_irq_nosync(di->irq_int);
	schedule_work(&di->irq_work);

	return IRQ_HANDLED;
}
#endif

static struct loadswitch_ops fpf2283_sysinfo_ops ={
	.ls_init = fpf2283_charge_init,
	.ls_exit = fpf2283_charge_exit,
	.ls_enable = fpf2283_charge_enable,
	.ls_discharge = fpf2283_discharge,
	.is_ls_close = fpf2283_is_ls_close,
	.watchdog_config_ms = fpf2283_config_watchdog_ms,
	.ls_status = fpf2283_ls_status,
};

/**********************************************************
*  Function:       fpf2283_probe
*  Discription:    fpf2283 module probe
*  Parameters:   client:i2c_client
*                      id:i2c_device_id
*  return value:  0-sucess or others-fail
**********************************************************/
static int fpf2283_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;
	struct fpf2283_device_info *di = NULL;
	struct device_node *np = NULL;

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
	g_fpf2283_dev = di;

	di->dev = &client->dev;
	np = di->dev->of_node;
	di->client = client;
	i2c_set_clientdata(client, di);

#if 0
	INIT_WORK(&di->irq_work, fpf2283_irq_work);

	di->gpio_int = of_get_named_gpio(np, "gpio_int", 0);
	hwlog_info("gpio_int=%d\n", di->gpio_int);

	if (!gpio_is_valid(di->gpio_int)) {
		hwlog_err("error: gpio(gpio_int) is not valid!\n");
		ret = -EINVAL;
		goto fpf2283_fail_1;
	}

	ret = gpio_request(di->gpio_int, "fpf2283_gpio_int");
	if (ret) {
		hwlog_err("error: gpio(gpio_int) request fail!\n");
		ret = -EINVAL;
		goto fpf2283_fail_1;
	}

	ret = gpio_direction_input(di->gpio_int);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) set input fail!\n");
		goto fpf2283_fail_2;
	}

	di->irq_int = gpio_to_irq(di->gpio_int);
	if (di->irq_int < 0) {
		hwlog_err("error: gpio(gpio_int) map to irq fail!\n");
		ret = -EINVAL;
		goto fpf2283_fail_2;
	}

	ret = request_irq(di->irq_int, fpf2283_interrupt, IRQF_TRIGGER_FALLING, "fpf2283_int_irq", di);
	if (ret) {
		hwlog_err("error: gpio(gpio_int) irq request fail!\n");
		ret = -EINVAL;
		di->irq_int = -1;
		goto fpf2283_fail_2;
	}
#endif

	ret = loadswitch_aux_ops_register(&fpf2283_sysinfo_ops);
	if (ret) {
		hwlog_err("register loadswitch ops failed!\n");
		goto fpf2283_fail_3;
	}

	hwlog_info("probe end\n");
	return 0;

fpf2283_fail_3:
	//free_irq(di->irq_int, di);
fpf2283_fail_2:
	//gpio_free(di->gpio_int);
fpf2283_fail_1:
	devm_kfree(&client->dev, di);
	g_fpf2283_dev = NULL;

	return ret;
}

/**********************************************************
*  Function:       fpf2283_remove
*  Discription:    fpf2283 module remove
*  Parameters:   client:i2c_client
*  return value:  0-sucess or others-fail
**********************************************************/
static int fpf2283_remove(struct i2c_client *client)
{
	struct fpf2283_device_info *di = i2c_get_clientdata(client);

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

MODULE_DEVICE_TABLE(i2c, fpf2283);

static struct of_device_id fpf2283_of_match[] = {
	{
		.compatible = "huawei,fpf2283_aux",
		.data = NULL,
	},
	{ },
};

static const struct i2c_device_id fpf2283_i2c_id[] = {
	{"fpf2283_aux", 0}, {}
};

static struct i2c_driver fpf2283_driver = {
	.probe = fpf2283_probe,
	.remove = fpf2283_remove,
	.id_table = fpf2283_i2c_id,
	.driver = {
		.owner = THIS_MODULE,
		.name = "huawei,fpf2283_aux",
		.of_match_table = of_match_ptr(fpf2283_of_match),
	},
};

/**********************************************************
*  Function:       fpf2283_init
*  Discription:    fpf2283 module initialization
*  Parameters:   NULL
*  return value:  0-sucess or others-fail
**********************************************************/
static int __init fpf2283_init(void)
{
	int ret = 0;

	ret = i2c_add_driver(&fpf2283_driver);
	if (ret) {
		hwlog_err("error: fpf2283 i2c_add_driver error!\n");
	}

	return ret;
}

/**********************************************************
*  Function:       fpf2283_exit
*  Discription:    fpf2283 module exit
*  Parameters:   NULL
*  return value:  NULL
**********************************************************/
static void __exit fpf2283_exit(void)
{
	i2c_del_driver(&fpf2283_driver);
}

module_init(fpf2283_init);
module_exit(fpf2283_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("fpf2283 module driver");
MODULE_AUTHOR("HW Inc");
