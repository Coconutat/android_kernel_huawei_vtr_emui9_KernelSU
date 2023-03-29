/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2013 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/ctype.h>
#include <linux/delay.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/debugfs.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/regulator/consumer.h>
#include <linux/string.h>
#include <linux/of_gpio.h>
#include <linux/kthread.h>
#include <linux/uaccess.h>
#include <linux/sched/rt.h>
#include <linux/fb.h>
#include <linux/workqueue.h>
#if defined(CONFIG_FB)
#include <linux/notifier.h>
#include <linux/fb.h>
#elif defined(CONFIG_HAS_EARLYSUSPEND)
#include <linux/earlysuspend.h>
#endif
#include "huawei_touchscreen_chips.h"
#include "huawei_touchscreen_algo.h"
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif
#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>

#define LDO17_PHYS_ADDR		(0X93)
#define LSW50_PHYS_ADDR	(0xAC)
#endif

/*#define DATA_SIZE     (4)*/
#define MAX_LOTUS_NUM  6
static struct mutex easy_wake_guesure_lock;

#define	EDGE_WIDTH_DEFAULT	10
#define BUFFER1_LEN  6
#define BUFFER2_LEN  50

#if defined (CONFIG_HUAWEI_DSM)
static struct dsm_dev dsm_tp = {
	.name = "dsm_tp",
	.device_name = "TP",
	.ic_name = "syn",	/*just for testing, waiting for the module owner revised*/
	.module_name = "NNN",
	.fops = NULL,
	.buff_size = 1024,
};

struct dsm_client *tp_dclient;

static unsigned char LDO17_status;
static unsigned char LSW50_status;
bool chip_detfail_dsm;
#endif

struct ts_data g_ts_data;
atomic_t g_data_report_over = ATOMIC_INIT(1);
u8 g_ts_log_cfg;
volatile bool gesture_func;
volatile bool g_lcd_control_tp_power;
volatile int g_tp_power_ctrl;
volatile int g_lcd_brightness_info = 0;
static struct ts_cmd_node ping_cmd_buff;
static struct ts_cmd_node pang_cmd_buff;
static struct work_struct check_tp_calibration_info_work;
static struct work_struct tp_init_work;
int tp_gpio_num;
/*external variable declare*/

/*global variable declare*/
static void rawdata_timeout_proc_fn(struct work_struct *work);
static void tp_init_work_fn(struct work_struct *work);
static void ts_work_after_input(void);
static int ts_get_brightness_info_cmd(void);
static int ts_proc_command_directly(struct ts_cmd_node *cmd);

struct rawdata_timeout_info {
	atomic_t idle_flag;
	struct ts_rawdata_info *info;
};

static struct delayed_work g_rawdata_proc_work;

static struct rawdata_timeout_info g_rawdata_timeout_info = {
	.idle_flag = ATOMIC_INIT(0),
	.info = NULL,
};

static DECLARE_DELAYED_WORK(g_rawdata_proc_work, rawdata_timeout_proc_fn);

/*external function declare*/
extern int i2c_check_addr_busy(struct i2c_adapter *adapter, int addr);
extern unsigned int get_boot_into_recovery_flag(void);
extern unsigned int get_pd_charge_flag(void);
#if defined (CONFIG_TEE_TUI)
extern int i2c_init_secos(struct i2c_adapter *adap);
extern int i2c_exit_secos(struct i2c_adapter *adap);
#endif
static void ts_stop_wd_timer(struct ts_data *cd);
void ts_thread_stop_notify(void);
static int ts_get_chip_info(struct ts_cmd_node *in_cmd,
			    struct ts_cmd_node *out_cmd);


static struct ts_device_data g_ts_device_map[] = {
#ifdef CONFIG_SYNAPTICS_TS
	[0] = {
	       .chip_name = "synaptics",
	       .irq_gpio = TS_IO_UNDEFINE,	/*must be defined in dts/boardid*/
	       .irq_config = TS_IRQ_CFG_UNDEFINE,	/*must be defined in dts/boardid*/
	       .reset_gpio = TS_IO_UNDEFINE,	/*must be defined in dts/boardid*/
	       .ops = &ts_synaptics_ops,
	       },
#endif
#ifdef CONFIG_ATMEL_TS
	[1] = {
	       .chip_name = "atmel",
	       .irq_gpio = TS_IO_UNDEFINE,	/*must be defined in dts/boardid*/
	       .irq_config = TS_IRQ_CFG_UNDEFINE,	/*must be defined in dts/boardid*/
	       .reset_gpio = TS_IO_UNDEFINE,	/*must be defined in dts/boardid*/
	       .ops = &ts_atmel_ops,
	       },
#endif
#ifdef CONFIG_CYPRESS_TS
/*	[2] = {
*	       .chip_name = "cypress",
*	       .irq_gpio = TS_IO_UNDEFINE,
*	       .irq_config = TS_IRQ_CFG_UNDEFINE,
*	       .reset_gpio = TS_IO_UNDEFINE,
*	       .ops = &ts_cypress_ops,
*	       },
*/
#endif
#ifdef CONFIG_HIDEEP_TS
	[3] = {
	       .chip_name = "hideep",
	       .irq_gpio = TS_IO_UNDEFINE,
	       .irq_config = TS_IRQ_CFG_UNDEFINE,
	       .reset_gpio = TS_IO_UNDEFINE,
	       .ops = &ts_hideep_ops,
	       },
#endif
#ifdef CONFIG_ST_TS
	[4]	=	{
		.chip_name = "st",
		.irq_gpio = TS_IO_UNDEFINE,
		.irq_config = TS_IRQ_CFG_UNDEFINE,
		.reset_gpio = TS_IO_UNDEFINE,
		.ops = &ts_st_ops,
	},
#endif
#ifdef CONFIG_HW_NOVATEK_TS
	[5]	=	{
		.chip_name = "novatek",
		.irq_gpio = TS_IO_UNDEFINE,
		.irq_config = TS_IRQ_CFG_UNDEFINE,
		.reset_gpio = TS_IO_UNDEFINE,
		.ops = &ts_novatek_ops,
	},
#endif
#ifdef CONFIG_GOODIX_TS
	[6]	=	{
		.chip_name = "goodix",
		.irq_gpio = TS_IO_UNDEFINE,
		.irq_config = TS_IRQ_CFG_UNDEFINE,
		.reset_gpio = TS_IO_UNDEFINE,
		.ops = &ts_goodix_ops,
	},
#endif
};

#if defined (CONFIG_TEE_TUI)
static void ts_tui_secos_init(void)
{
	unsigned char ts_state = 0;
	int times = 0;

	while (times < TS_FB_LOOP_COUNTS) {
		ts_state = atomic_read(&g_ts_data.state);
		if ((TS_SLEEP == ts_state) || (TS_WORK_IN_SLEEP == ts_state)) {
			mdelay(TS_FB_WAIT_TIME);
			times++;
		} else {
			break;
		}
	}

	if (!g_ts_data.chip_data->report_tui_enable) {
		disable_irq(g_ts_data.irq_id);
		times = 0;
		while (times < TS_FB_LOOP_COUNTS) {
			if (!atomic_read(&g_data_report_over)) {
				mdelay(TS_FB_WAIT_TIME);
				times++;
			} else {
				break;
			}
		}
		i2c_init_secos(g_ts_data.client->adapter);
		g_ts_data.chip_data->report_tui_enable = true;
		TS_LOG_INFO("[tui] ts_tui_secos_init: report_tui_enable is %d\n",
				g_ts_data.chip_data->report_tui_enable);
	}
}

static void ts_tui_secos_exit(void)
{
	struct ts_device_data *dev = g_ts_data.chip_data;

	if (g_ts_data.chip_data->report_tui_enable) {
		i2c_exit_secos(g_ts_data.client->adapter);
		if (dev->ops->chip_reset)
			dev->ops->chip_reset();

		enable_irq(g_ts_data.irq_id);
		g_ts_data.chip_data->report_tui_enable = false;
		TS_LOG_INFO("ts_tui_secos_exit: tui_set_flag is %d\n",
			    g_ts_data.chip_data->tui_set_flag);

		if (g_ts_data.chip_data->tui_set_flag & 0x1) {
			TS_LOG_INFO("TUI exit, do before suspend\n");
			ts_power_control_notify(TS_BEFORE_SUSPEND,
						SHORT_SYNC_TIMEOUT);
		}

		if (g_ts_data.chip_data->tui_set_flag & 0x2) {
			TS_LOG_INFO("TUI exit, do suspend\n");
			ts_power_control_notify(TS_SUSPEND_DEVICE,
						NO_SYNC_TIMEOUT);
		}

		g_ts_data.chip_data->tui_set_flag = 0;
		TS_LOG_INFO("ts_tui_secos_exit: report_tui_enable is %d\n",
			    g_ts_data.chip_data->report_tui_enable);
	}
}

static int tui_tp_init(void *data, int secure)
{
	if (secure) {
		ts_tui_secos_init();
	} else
		ts_tui_secos_exit();
	return 0;
}

int ts_tui_report_input(void *finger_data)
{
	int error = NO_ERR;
	struct ts_fingers *finger = (struct ts_fingers *)finger_data;
	struct input_dev *input_dev = g_ts_data.input_dev;
	int id;

	int finger_num = 0;
	TS_LOG_DEBUG("ts_tui_report_input\n");

	for (id = 0; id < TS_MAX_FINGER; id++) {

		if (finger->fingers[id].status == 0) {
			TS_LOG_DEBUG("never touch before: id is %d\n", id);
			continue;
		}

		if (finger->fingers[id].status == TS_FINGER_PRESS) {
			TS_LOG_DEBUG
			    ("down: id is %d, finger->fingers[id].pressure = %d, finger->fingers[id].x = %d, finger->fingers[id].y = %d\n",
			     id, finger->fingers[id].pressure,
			     finger->fingers[id].x, finger->fingers[id].y);
			finger_num++;

			input_report_abs(input_dev, ABS_MT_PRESSURE,
					 finger->fingers[id].pressure);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					 finger->fingers[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					 finger->fingers[id].y);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
			input_mt_sync(input_dev);

		} else if (finger->fingers[id].status == TS_FINGER_RELEASE) {
			TS_LOG_DEBUG("up: id is %d, status = %d\n", id,
				     finger->fingers[id].status);
			input_mt_sync(input_dev);
		}
	}

	input_report_key(input_dev, BTN_TOUCH, finger_num);
	input_sync(input_dev);

	return error;
}
#endif

static int ts_i2c_write(u8 *buf, u16 length)
{
	int count = 0;
	int ret;
#if defined (CONFIG_TEE_TUI)
	if (g_ts_data.chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif
	do {
		ret = i2c_master_send(g_ts_data.client, buf, length);
		if (ret == length) {
			return NO_ERR;
		}
#if defined (CONFIG_HUAWEI_DSM)
		else
			g_ts_data.dsm_info.constraints_I2C_status = ret;
#endif

		msleep(I2C_WAIT_TIME);
	} while (++count < I2C_RW_TRIES);

#if defined (CONFIG_HUAWEI_DSM)
	if (!chip_detfail_dsm) {
		TS_LOG_ERR("chip write init no need dsm \n");
		return -EIO;
	}
	LDO17_status = 0;	/*hi6xxx_pmic_reg_read(LDO17_PHYS_ADDR);*/
	LSW50_status = 0;	/*hi6xxx_pmic_reg_read(LSW50_PHYS_ADDR);*/
	if (!dsm_client_ocuppy(tp_dclient)) {
		dsm_client_record(tp_dclient, "irq_gpio:%d\tvalue:%d.\n\
reset_gpio:%d\t value:%d.\nLDO17_status is 0x%x.\nLSW50_status is 0x%x\nI2C_status:%d.\n", g_ts_data.chip_data->irq_gpio, gpio_get_value(g_ts_data.chip_data->irq_gpio), g_ts_data.chip_data->reset_gpio, gpio_get_value(g_ts_data.chip_data->reset_gpio), LDO17_status, LSW50_status, g_ts_data.dsm_info.constraints_I2C_status);
		dsm_client_notify(tp_dclient, DSM_TP_I2C_RW_ERROR_NO);
	}
#endif
	TS_LOG_ERR("ts_i2c_write failed\n");
	return -EIO;
}

static int ts_spi_write(u8 *buf, u16 length)
{
	return NO_ERR;
}

static int ts_i2c_read(u8 *reg_addr, u16 reg_len, u8 *buf, u16 len)
{
	int count = 0;
	int ret;
	struct i2c_msg xfer[2];
#if defined (CONFIG_TEE_TUI)
	if (g_ts_data.chip_data->report_tui_enable) {
		return NO_ERR;
	}
#endif
	if (g_ts_data.chip_data->is_i2c_one_byte) {
		/* Read data */
		xfer[0].addr = g_ts_data.client->addr;
		xfer[0].flags = I2C_M_RD;
		xfer[0].len = len;
		xfer[0].buf = buf;
		do {
			ret = i2c_transfer(g_ts_data.client->adapter, xfer, 1);
			if (ret == 1) {
				return NO_ERR;
			}
#if defined (CONFIG_HUAWEI_DSM)
			else
				g_ts_data.dsm_info.constraints_I2C_status = ret;
#endif

			msleep(I2C_WAIT_TIME);
		} while (++count < I2C_RW_TRIES);
	} else {
		/*register addr */
		xfer[0].addr = g_ts_data.client->addr;
		xfer[0].flags = 0;
		xfer[0].len = reg_len;
		xfer[0].buf = reg_addr;

		/* Read data */
		xfer[1].addr = g_ts_data.client->addr;
		xfer[1].flags = I2C_M_RD;
		xfer[1].len = len;
		xfer[1].buf = buf;

		do {
			ret = i2c_transfer(g_ts_data.client->adapter, xfer, 2);
			if (ret == 2) {
				return NO_ERR;
			}
#if defined (CONFIG_HUAWEI_DSM)
			else
				g_ts_data.dsm_info.constraints_I2C_status = ret;
#endif

			msleep(I2C_WAIT_TIME);
		} while (++count < I2C_RW_TRIES);
	}

#if defined (CONFIG_HUAWEI_DSM)
	if (!chip_detfail_dsm) {
		TS_LOG_ERR("chip read init no need dsm \n");
		return -EIO;
	}
	LDO17_status = 0;	/*hi6xxx_pmic_reg_read(LDO17_PHYS_ADDR);*/
	LSW50_status = 0;	/*hi6xxx_pmic_reg_read(LSW50_PHYS_ADDR);*/
	if (!dsm_client_ocuppy(tp_dclient)) {
		dsm_client_record(tp_dclient, "irq_gpio:%d\tvalue:%d.\n\
reset_gpio:%d\t value:%d.\nLDO17_status is 0x%x.\nLSW50_status is 0x%x\nI2C_status:%d.\n", g_ts_data.chip_data->irq_gpio, gpio_get_value(g_ts_data.chip_data->irq_gpio), g_ts_data.chip_data->reset_gpio, gpio_get_value(g_ts_data.chip_data->reset_gpio), LDO17_status, LSW50_status, g_ts_data.dsm_info.constraints_I2C_status);
		dsm_client_notify(tp_dclient, DSM_TP_I2C_RW_ERROR_NO);
	}
#endif
	TS_LOG_ERR("ts_i2c_read failed\n");
	return -EIO;
}

static int ts_spi_read(u8 *reg_addr, u16 reg_len, u8 *buf, u16 len)
{
	return NO_ERR;
}

static struct ts_bus_info ts_bus_i2c_info = {
	.btype = TS_BUS_I2C,
	.bus_write = ts_i2c_write,
	.bus_read = ts_i2c_read,
};

static struct ts_bus_info ts_bus_spi_info = {
	.btype = TS_BUS_SPI,
	.bus_write = ts_spi_write,
	.bus_read = ts_spi_read,
};

int register_algo_func(struct ts_device_data *chip_data,
		       struct ts_algo_func *fn)
{
	int error = -EIO;

	if (!chip_data || !fn)
		goto out;

	fn->algo_index = chip_data->algo_size;
	list_add_tail(&fn->node, &chip_data->algo_head);
	chip_data->algo_size++;
	smp_mb();
	error = NO_ERR;

out:
	return error;
}

int put_one_cmd_direct_sync(struct ts_cmd_node *cmd, int timeout)
{
	int error = NO_ERR;
	TS_LOG_INFO("%s Enter\n",__func__);
	if(g_ts_data.chip_data->is_parade_solution == 0){
		return put_one_cmd(cmd,timeout);
	}

	if((atomic_read(&g_ts_data.state) == TS_UNINIT)){
		error = -EIO;
		return error;
	}
	if((atomic_read(&g_ts_data.state) == TS_SLEEP)
	     || (atomic_read(&g_ts_data.state) == TS_WORK_IN_SLEEP)){
	     TS_LOG_INFO("%s In Sleep State\n",__func__);
		 error = -EIO;
	     return error;
	}
out:
	return error;
}

int put_one_cmd(struct ts_cmd_node *cmd, int timeout)
{
	int error = -EIO;
	unsigned long flags;
	struct ts_cmd_queue *q;
	struct ts_cmd_sync *sync = NULL;

	if (!cmd) {
		TS_LOG_ERR("find null pointer\n");
		goto out;
	}

	if (TS_UNINIT == atomic_read(&g_ts_data.state)) {
		TS_LOG_ERR("ts module not initialize\n");
		goto out;
	}

	if (timeout) {
		sync =
		    (struct ts_cmd_sync *)kzalloc(sizeof(struct ts_cmd_sync),
						  GFP_KERNEL);
		if (NULL == sync) {
			TS_LOG_ERR("failed to kzalloc completion\n");
			error = -ENOMEM;
			goto out;
		}
		init_completion(&sync->done);
		atomic_set(&sync->timeout_flag, TS_NOT_TIMEOUT);
		cmd->sync = sync;
	} else {
		cmd->sync = NULL;
	}

	if((g_ts_data.chip_data->is_direct_proc_cmd) &&
		(g_ts_data.chip_data->is_can_device_use_int==false)){
		if(cmd->command == TS_INT_PROCESS)
			goto out; //Not use INT in the init process
		q = &g_ts_data.no_int_queue;
	} else {
		q = &g_ts_data.queue;
	}

	spin_lock_irqsave(&q->spin_lock, flags);
	smp_wmb();
	if (q->cmd_count == q->queue_size) {
		spin_unlock_irqrestore(&q->spin_lock, flags);
		TS_LOG_ERR("queue is full\n");
		WARN_ON(1);
		error = -EIO;
		goto free_sync;
	}
	memcpy(&q->ring_buff[q->wr_index], cmd, sizeof(struct ts_cmd_node));
	q->cmd_count++;
	q->wr_index++;
	q->wr_index %= q->queue_size;
	smp_mb();
	spin_unlock_irqrestore(&q->spin_lock, flags);
	TS_LOG_DEBUG("put one cmd :%d in ring buff\n", cmd->command);
	error = NO_ERR;
	wake_up_process(g_ts_data.ts_task);	/*wakeup process*/

	if (timeout
	    && !(wait_for_completion_timeout(&sync->done, abs(timeout) * HZ))) {
		atomic_set(&sync->timeout_flag, TS_TIMEOUT);
		TS_LOG_ERR("wait for cmd respone timeout\n");
		error = -EBUSY;
		goto out;
	}
	smp_wmb();

free_sync:
	if (sync) {
		kfree(sync);
	}
out:
	return error;
}

static int get_one_cmd(struct ts_cmd_node *cmd)
{
	unsigned long flags;
	int error = -EIO;
	struct ts_cmd_queue *q;

	if (unlikely(!cmd)) {
		TS_LOG_ERR("find null pointer\n");
		goto out;
	}

	q = &g_ts_data.queue;

	spin_lock_irqsave(&q->spin_lock, flags);
	smp_wmb();
	if (!q->cmd_count) {
		TS_LOG_DEBUG("queue is empty\n");
		spin_unlock_irqrestore(&q->spin_lock, flags);
		goto out;
	}
	memcpy(cmd, &q->ring_buff[q->rd_index], sizeof(struct ts_cmd_node));
	q->cmd_count--;
	q->rd_index++;
	q->rd_index %= q->queue_size;
	smp_mb();
	spin_unlock_irqrestore(&q->spin_lock, flags);
	TS_LOG_DEBUG("get one cmd :%d from ring buff\n", cmd->command);
	error = NO_ERR;

out:
	return error;
}

int ts_get_esd_status(void)
{
	int ret = 0;

	ret = atomic_read(&g_ts_data.ts_esd_state);

	return ret;
}
EXPORT_SYMBOL(ts_get_esd_status);

void ts_clear_esd_status(void)
{
	atomic_set(&g_ts_data.ts_esd_state, TS_NO_ESD);
}
EXPORT_SYMBOL(ts_clear_esd_status);

ssize_t ts_calibration_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_calibration_info_param *info = NULL;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	if (!g_ts_data.chip_data->should_check_tp_calibration_info) {
		TS_LOG_ERR("No calibration info.\n");
		error = NO_ERR;
		goto out;
	}

	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node), GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info = (struct ts_calibration_info_param *)kzalloc(sizeof(struct ts_calibration_info_param), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	cmd->command = TS_GET_CALIBRATION_INFO;
	cmd->cmd_param.prv_params = (void *)info;
	if(g_ts_data.chip_data->is_direct_proc_cmd)
		error = ts_proc_command_directly(cmd);
	else
		error = put_one_cmd(cmd, LONG_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}
	error = snprintf(buf, PAGE_SIZE, "%d\n", info->calibration_crc);
out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}
	TS_LOG_INFO("%s done\n", __FUNCTION__);

	return error;
}

static ssize_t ts_oem_info_show(struct device *dev, struct device_attribute *attr, char *buf)
{
		int error = NO_ERR;
		struct ts_cmd_node *cmd = NULL;
		struct ts_oem_info_param *info = NULL;
		unsigned char str_oem[TS_CHIP_TYPE_MAX_SIZE];
		unsigned char str_tmp[TS_CHIP_TYPE_MAX_SIZE];
		int i;
		int parade_flag = 0;
		int count = 30;
		TS_LOG_INFO("%s: called\n", __func__);

		if (dev == NULL) {
			TS_LOG_ERR("%s: dev is null\n", __func__);
			error = -EINVAL;
			goto out;
		}

		cmd =
			(struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
						  GFP_KERNEL);
		if (!cmd) {
			TS_LOG_ERR("%s: malloc failed\n", __func__);
			error = -ENOMEM;
			goto out;
		}

		info =
			(struct ts_oem_info_param *)
			kzalloc(sizeof(struct ts_oem_info_param), GFP_KERNEL);
		if (!info) {
			TS_LOG_ERR("%s: malloc failed\n", __func__);
			error = snprintf(buf, TS_CHIP_TYPE_MAX_SIZE, "%d,\n", TS_CHIP_READ_OEM_INFO_ERROR);
			goto out;
		}

		info->op_action = TS_ACTION_READ;
		cmd->command = TS_OEM_INFO_SWITCH;
		cmd->cmd_param.prv_params = (void *)info;
		if( g_ts_data.chip_data->is_direct_proc_cmd) {
			TS_LOG_INFO("%s:use put_one_cmd_thread func\n", __func__);
			error = ts_proc_command_directly(cmd);
		} else {
			error = put_one_cmd(cmd, LONG_SYNC_TIMEOUT);
		}
		if (error) {
			TS_LOG_ERR("%s: put cmd error :%d\n", __func__, error);
			error = snprintf(buf, TS_CHIP_TYPE_MAX_SIZE, "%d,\n", TS_CHIP_READ_OEM_INFO_ERROR);
			goto out;
		}

		if (info->status != TS_ACTION_SUCCESS) {
			TS_LOG_ERR("%s: read action failed\n", __func__);
			error = snprintf(buf, TS_CHIP_TYPE_MAX_SIZE, "%d,\n", TS_CHIP_READ_OEM_INFO_ERROR);
			TS_LOG_INFO("%s: return to sys str:%s\n", __func__,buf);
			goto out;
		}

		if( g_ts_data.chip_data->is_new_oem_structure) {
			TS_LOG_INFO("%s: use new oem structure\n", __func__);
			memset(str_oem, 0, sizeof(str_oem));
			if( info->data[1] != 0 ) {
				for(i = 0; i < info->length; ++i) {
					memset(str_tmp, 0, sizeof(str_tmp));
					snprintf(str_tmp, sizeof(str_tmp), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
					info->data[0+i*16],info->data[1+i*16],info->data[2+i*16],info->data[3+i*16],
					info->data[4+i*16],info->data[5+i*16],info->data[6+i*16],info->data[7+i*16],
					info->data[8+i*16],info->data[9+i*16],info->data[10+i*16],info->data[11+i*16],
					info->data[12+i*16],info->data[13+i*16],info->data[14+i*16],info->data[15+i*16]);
					strncat(str_oem, str_tmp,strlen(str_tmp));
				}
				TS_LOG_INFO("%s:  str_oem string:%s \n", __func__,str_oem);
			}
		} else {
			memset(str_oem, 0, sizeof(str_oem));
			if( info->data[1] != 0 ) {
				for(i = 0; i < info->data[1]; ++i) {
					memset(str_tmp, 0, sizeof(str_tmp));
					snprintf(str_tmp, sizeof(str_tmp), "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,",
					info->data[0+i*16],info->data[1+i*16],info->data[2+i*16],info->data[3+i*16],
					info->data[4+i*16],info->data[5+i*16],info->data[6+i*16],info->data[7+i*16],
					info->data[8+i*16],info->data[9+i*16],info->data[10+i*16],info->data[11+i*16],
					info->data[12+i*16],info->data[13+i*16],info->data[14+i*16],info->data[15+i*16]);
					strncat(str_oem, str_tmp,strlen(str_tmp));
				}
			TS_LOG_INFO("%s:  str_oem string:%s \n", __func__,str_oem);
			}
		}
		if( strlen(info->data) == 1) {
			error = snprintf(buf, TS_CHIP_TYPE_MAX_SIZE, "%d,\n", info->data[0]);
			TS_LOG_INFO("%s: Return read result string:%s to sys file\n", __func__,buf);
		} else {
			error = snprintf(buf, TS_CHIP_TYPE_MAX_SIZE, "%s\n", str_oem);
			TS_LOG_INFO("%s: Return read data string:%s to sys file\n", __func__,buf);
		}

	out:
		if (cmd){
			kfree(cmd);
			cmd =NULL;
		}
		if (info){
			kfree(info);
			info =NULL;
		}
		TS_LOG_DEBUG("%s done\n", __func__);
		return error;
}

ssize_t ts_ome_info_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned int value;
	int error= NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_oem_info_param *info = NULL;
	char *cur;
	char *token;
	int i = 0;
	TS_LOG_INFO("%s: called\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("%s: dev is null\n", __func__);
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("%s: malloc failed\n", __func__);
		error = -ENOMEM;
		goto out;
	}

	info =
		(struct ts_oem_info_param *)
		kzalloc(sizeof(struct ts_oem_info_param), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("%s: malloc failed\n", __func__);
		error = -ENOMEM;
		goto out;
	}

	if (strlen(buf) > TS_CHIP_TYPE_MAX_SIZE+1) {
		TS_LOG_ERR("%s: Store TPIC type data size= %d larger than MAX input size=%d \n",
			__func__, strlen(buf), TS_CHIP_TYPE_MAX_SIZE);
		error = -EINVAL;
		goto out;
	}

	cur = (char*)buf;
	token = strsep(&cur, ",");
	while (token) {
		info->data[i++] = (unsigned char)simple_strtol(token, NULL, 0);
		token = strsep(&cur, ",");
	}

	info->op_action = TS_ACTION_WRITE;
	cmd->command = TS_OEM_INFO_SWITCH;
	cmd->cmd_param.prv_params = (void *)info;
	if( g_ts_data.chip_data->is_direct_proc_cmd) {
		TS_LOG_INFO("%s:use put_one_cmd_thread func\n", __func__);
		error = ts_proc_command_directly(cmd);
	}else{
		error = put_one_cmd(cmd, LONG_LONG_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("%s: put cmd error :%d\n", __func__, error);
		error = -EBUSY;
		goto out;
	}

	error = count;
out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}
	TS_LOG_DEBUG("%s: done\n",  __func__);
	return error;
}

#ifdef CONFIG_HUAWEI_DSM
static void ts_get_projectid_for_dsm(struct ts_chip_info_param *info)
{
	char *tmp = NULL;
	if(NULL != info->ic_vendor)
	{
		tmp =  info->ic_vendor;
		while(strchr(tmp,'-') != NULL)
		{
			tmp = strrchr(tmp,'-')+1;
		}

		if (tmp && DSM_MAX_MODULE_NAME_LEN > strlen(tmp)) {
			dsm_tp.module_name = tmp;
			if (dsm_update_client_vendor_info(&dsm_tp)) {
				TS_LOG_ERR("dsm update client_vendor_info is failed\n");
			}
		}else {
			TS_LOG_ERR("project id is invalid\n");
		}
	}
}
#endif

ssize_t ts_chip_info_show(struct device *dev, struct device_attribute *attr,
			  char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_chip_info_param *info = &g_ts_data.chip_info;

	TS_LOG_INFO("ts_chip_info_show called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	cmd->command = TS_GET_CHIP_INFO;
	cmd->cmd_param.prv_params = (void *)info;

	if (g_ts_data.chip_data->is_direct_proc_cmd){
		error = ts_proc_command_directly(cmd);
	} else {
		error = put_one_cmd(cmd, LONG_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}


	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}
	if (g_ts_data.get_info_flag) {
		error = snprintf(buf, CHIP_INFO_LENGTH, "%s", info->mod_vendor);
		g_ts_data.get_info_flag = false;
	} else {
		error =
		    snprintf(buf,
			     CHIP_INFO_LENGTH * 2 + CHIP_INFO_LENGTH * 2 + 1,
			     "%s-%s-%s\n", info->ic_vendor, info->mod_vendor,
			     info->fw_vendor);
	}

#ifdef CONFIG_HUAWEI_DSM
        ts_get_projectid_for_dsm(info);
#endif
out:
	if (cmd){
		kfree(cmd);
		cmd = NULL;
	}
	TS_LOG_DEBUG("ts_chip_info_show done\n");
	return error;
}

ssize_t ts_chip_info_store(struct device *dev, struct device_attribute *attr,
			   const char *buf, size_t count)
{
	unsigned int value;
	int error;
	struct ts_cmd_node *cmd = NULL;
	struct ts_data *info = NULL;

	TS_LOG_INFO("ts_chip_info_store called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_DEBUG("sscanf value is %u\n", value);

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	info = (struct ts_data *)kzalloc(sizeof(struct ts_data), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	info->get_info_flag = value;
	cmd->command = TS_SET_INFO_FLAG;
	cmd->cmd_param.prv_params = (void *)info;
	if(g_ts_data.chip_data->is_direct_proc_cmd) {
		error = ts_proc_command_directly(cmd);
	} else {
		error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}
	error = count;
out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}
	TS_LOG_DEBUG("ts_chip_info_store done\n");
	return error;
}

static ssize_t ts_dsm_debug_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_dsm_debug_info info;

	TS_LOG_INFO("ts_dsm_debug_show called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	cmd->command = TS_DSM_DEBUG;
	cmd->cmd_param.prv_params = (void *)&info;
	error = put_one_cmd(cmd, LONG_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		goto out;
	}
	if (info.status == TS_ACTION_SUCCESS)
		error = snprintf(buf, MAX_STR_LEN, "%s\n", "success");
	else
		error = snprintf(buf, MAX_STR_LEN, "%s\n", "failed");

out:
	if (cmd){
		kfree(cmd);
		cmd = NULL;
	}
	TS_LOG_DEBUG("ts_debug_show done\n");
	return error;
}

static ssize_t ts_calibrate_wakeup_gesture_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_calibrate_info info;

	TS_LOG_INFO("%s called\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	cmd->command = TS_CALIBRATE_DEVICE_LPWG;
	cmd->cmd_param.prv_params = (void *)&info;
	error = put_one_cmd(cmd, LONG_LONG_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info.status == TS_ACTION_SUCCESS)
		error = snprintf(buf, MAX_STR_LEN, "%s\n", "success");
	else
		error = snprintf(buf, MAX_STR_LEN, "%s\n", "failed");

out:
	if (cmd){
		kfree(cmd);
		cmd = NULL;
	}
	TS_LOG_DEBUG("%s done\n", __func__);
	return error;
}

static ssize_t ts_calibrate_show(struct device *dev,
				 struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_calibrate_info info;

	TS_LOG_INFO("ts_calibrate_show called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	cmd->command = TS_CALIBRATE_DEVICE;
	cmd->cmd_param.prv_params = (void *)&info;
	if( g_ts_data.chip_data->is_direct_proc_cmd) {
		error = ts_proc_command_directly(cmd);
	}else{
		error = put_one_cmd(cmd, LONG_LONG_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info.status == TS_ACTION_SUCCESS)
		error = snprintf(buf, MAX_STR_LEN, "%s\n", "success");
	else
		error = snprintf(buf, MAX_STR_LEN, "%s\n", "failed");

out:
	if (cmd){
		kfree(cmd);
		cmd = NULL;
	}
	TS_LOG_DEBUG("ts_calibrate_show done\n");
	return error;
}

static ssize_t ts_reset_store(struct device *dev, struct device_attribute *attr,
			      const char *buf, size_t count)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;

	TS_LOG_INFO("%s called\n", __func__);

	memset(&cmd, 0, sizeof(struct ts_cmd_node));
	cmd.command = TS_FORCE_RESET;
	error = put_one_cmd(&cmd, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("%s failed:%d\n", __func__, error);
	}
	TS_LOG_INFO("%s done\n", __func__);
	return count;
}

static ssize_t ts_glove_mode_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_glove_info *info = NULL;

	TS_LOG_INFO("ts_glove_mode_show called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	info =
	    (struct ts_glove_info *)kzalloc(sizeof(struct ts_glove_info),
					    GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info->op_action = TS_ACTION_READ;
	cmd->command = TS_GLOVE_SWITCH;
	cmd->cmd_param.prv_params = (void *)info;
	if( g_ts_data.chip_data->is_direct_proc_cmd) {
		error = ts_proc_command_directly(cmd);
	}else{
		error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info->status == TS_ACTION_SUCCESS)
		error = snprintf(buf, MAX_STR_LEN, "%d\n", info->glove_switch);
	else
		error = -EFAULT;

out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}
	TS_LOG_DEBUG("ts_glove_mode_show done\n");
	return error;
}

static ssize_t ts_glove_mode_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned int value;
	int error;
	struct ts_cmd_node *cmd = NULL;
	struct ts_glove_info *info = NULL;

	TS_LOG_INFO("ts_glove_mode_store called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_DEBUG("sscanf value is %u\n", value);

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info = &g_ts_data.feature_info.glove_info;
	info->op_action = TS_ACTION_WRITE;
	info->glove_switch = value;
	cmd->command = TS_GLOVE_SWITCH;
	cmd->cmd_param.prv_params = (void *)info;
	if( g_ts_data.chip_data->is_direct_proc_cmd) {
		error = ts_proc_command_directly(cmd);
	}else{
		error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("action failed\n");
		error = -EIO;
		goto out;
	}

	error = count;

out:
	if (cmd){
		kfree(cmd);
		cmd = NULL;
	}
	TS_LOG_DEBUG("ts_glove_mode_store done\n");
	return error;
}

static int ts_send_holster_cmd(void)
{
	int error;
	struct ts_cmd_node cmd;

	TS_LOG_DEBUG("set holster\n");
	cmd.command = TS_HOLSTER_SWITCH;
	cmd.cmd_param.prv_params = (void *)&g_ts_data.feature_info.holster_info;
	if( g_ts_data.chip_data->is_direct_proc_cmd) {
		error = ts_proc_command_directly(&cmd);
	}else{
		error = put_one_cmd(&cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}
	if (g_ts_data.feature_info.holster_info.status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("action failed\n");
		error = -EIO;
		goto out;
	}

out:
	return error;
}

static int ts_wakeup_gesture_enable_cmd(u8 switch_value)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;
	struct ts_wakeup_gesture_enable_info *info =
	    &g_ts_data.feature_info.wakeup_gesture_enable_info;

	info->op_action = TS_ACTION_WRITE;
	info->switch_value = switch_value;
	cmd.command = TS_WAKEUP_GESTURE_ENABLE;
	cmd.cmd_param.prv_params = (void *)info;

	if (TS_WORK == atomic_read(&g_ts_data.state)) {
		TS_LOG_ERR
		    ("can not enable/disable wakeup_gesture when tp is working in normal mode\n");
		error = -EINVAL;
		goto out;
	}

	error = put_one_cmd(&cmd, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("%s: put cmd error :%d\n", __func__, error);
		error = -EBUSY;
		goto out;
	}
	if (g_ts_data.feature_info.wakeup_gesture_enable_info.status !=
	    TS_ACTION_SUCCESS) {
		TS_LOG_ERR("%s action failed\n", __func__);
		error = -EIO;
		goto out;
	}

out:

	return error;
}

static int ts_send_init_cmd(void)
{
	int error = NO_ERR;
	TS_LOG_INFO("%s Enter\n", __func__);
	if(g_ts_data.chip_data->is_direct_proc_cmd){
		g_ts_data.chip_data->is_can_device_use_int = true;
		struct ts_cmd_node cmd;
		cmd.command = TS_TP_INIT;
		error = put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
		if (error) {
			TS_LOG_ERR("put cmd error :%d\n", error);
			error = -EBUSY;
		}
	} else {
		TS_LOG_ERR("%s, nothing to do\n", __func__);
	}
	return error;
}

static void proc_init_cmd(void){
	schedule_work(&tp_init_work);
	return;
}

static void tp_init_work_fn(struct work_struct *work){
	struct ts_cmd_node use_cmd;
	int i = TS_CMD_QUEUE_SIZE;
	struct ts_cmd_queue *q;
	unsigned long flags;
	struct ts_cmd_node *cmd = &use_cmd;
	struct ts_device_data *dev = g_ts_data.chip_data;
	q = &g_ts_data.no_int_queue;
	int error = NO_ERR;
	//Call chip init
	mutex_lock(&g_ts_data.chip_data->device_call_lock);
	if (dev->ops->chip_init) {
		TS_LOG_INFO("%s, call chip init\n",__func__);
		error = dev->ops->chip_init();
	}
	mutex_unlock(&g_ts_data.chip_data->device_call_lock);
	if(error != NO_ERR){
		TS_LOG_ERR("%s,chip init fail with error:%d\n",__func__,error);
		return;
	}
	TS_LOG_INFO("%s, chip init done\n",__func__);
	while(i-->0){
		spin_lock_irqsave(&q->spin_lock, flags);
		smp_wmb();
		if (!q->cmd_count) {
			TS_LOG_DEBUG("queue is empty\n");
			spin_unlock_irqrestore(&q->spin_lock, flags);
			break;
		}
		memcpy(cmd, &q->ring_buff[q->rd_index], sizeof(struct ts_cmd_node));
		q->cmd_count--;
		q->rd_index++;
		q->rd_index %= q->queue_size;
		smp_mb();
		spin_unlock_irqrestore(&q->spin_lock, flags);
		error = ts_proc_command_directly(cmd);
		if(error != NO_ERR){
			TS_LOG_INFO("%s process init cmd %d error",__func__, cmd->command);
		}
	}
}

static int ts_send_roi_cmd(enum ts_action_status read_write_type, int timeout)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;

	TS_LOG_INFO("ts_send_roi_cmd, read_write_type=%d\n", read_write_type);
	if (g_ts_data.feature_info.roi_info.roi_supported) {
		g_ts_data.feature_info.roi_info.op_action = read_write_type;

		cmd.command = TS_ROI_SWITCH;
		cmd.cmd_param.prv_params =
		    (void *)&g_ts_data.feature_info.roi_info;
		if( (g_ts_data.chip_data->is_direct_proc_cmd) && (g_ts_data.chip_data->is_can_device_use_int)) { //Set this macro check to make sure bootup use the put one cmd
			error = ts_proc_command_directly(&cmd);
		}else{
			error = put_one_cmd(&cmd, timeout);
		}

		if (error) {
			TS_LOG_ERR("put cmd error :%d\n", error);
			error = -EBUSY;
		}
	}
	return error;
}

static ssize_t ts_touch_window_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int window_enable;
	int x0 = 0;
	int y0 = 0;
	int x1 = 0;
	int y1 = 0;
	int error;
	struct ts_window_info info = { 0 };
	struct ts_cmd_node cmd;

	error =
	    sscanf(buf, "%d %d %d %d %d", &window_enable, &x0, &y0, &x1, &y1);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_INFO("sscanf value is %d (%d,%d), (%d,%d)\n", window_enable, x0,
		    y0, x1, y1);
	if (window_enable && ((x0 < 0) || (y0 < 0) || (x1 <= x0) || (y1 <= y0))) {
		TS_LOG_ERR("value is %d (%d,%d), (%d,%d)\n", window_enable, x0,
			   y0, x1, y1);
		error = -EINVAL;
		goto out;
	}

	info.top_left_x0 = x0;
	info.top_left_y0 = y0;
	info.bottom_right_x1 = x1;
	info.bottom_right_y1 = y1;
	info.window_enable = window_enable;

	cmd.command = TS_TOUCH_WINDOW;
	cmd.cmd_param.prv_params = (void *)&info;
	error = put_one_cmd(&cmd, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}
	if (info.status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("action failed\n");
		error = -EIO;
		goto out;
	}

	error = count;

out:
	TS_LOG_DEBUG("ts_touch_window_store done\n");
	return error;
}

static void ts_check_touch_window(struct ts_fingers *finger)
{
	int id;
	int flag = 0;
	int x0, y0, x1, y1;
	int window_enable;

	window_enable = g_ts_data.feature_info.window_info.window_enable;
	x0 = g_ts_data.feature_info.window_info.top_left_x0;
	y0 = g_ts_data.feature_info.window_info.top_left_y0;
	x1 = g_ts_data.feature_info.window_info.bottom_right_x1;
	y1 = g_ts_data.feature_info.window_info.bottom_right_y1;

	if (0 == window_enable) {
		TS_LOG_DEBUG("no need to part report\n");
		return;
	}

	if (finger->fingers[0].status != TS_FINGER_RELEASE) {
		for (id = 0; id < TS_MAX_FINGER; id++) {
			if (finger->fingers[id].status != 0) {
				if ((finger->fingers[id].x >= x0)
				    && (finger->fingers[id].x <= x1)
				    && (finger->fingers[id].y >= y0)
				    && (finger->fingers[id].y <= y1)) {
					flag = 1;
				} else {
					finger->fingers[id].status = 0;
				}
			}
		}
		if (!flag)
			finger->fingers[0].status = TS_FINGER_RELEASE;
	}
}

static ssize_t ts_sensitivity_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct ts_holster_info *info = &g_ts_data.feature_info.holster_info;
	ssize_t ret;

	TS_LOG_INFO("%s\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		ret = -EINVAL;
		goto out;
	}

	ret = snprintf(buf, MAX_STR_LEN, "%d\n", info->holster_switch);
out:
	return ret;
}

static ssize_t ts_sensitivity_store(struct device *dev,
				    struct device_attribute *attr,
				    const char *buf, size_t count)
{
	unsigned int value;
	int error;
	struct ts_holster_info *info = &g_ts_data.feature_info.holster_info;

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_INFO("%s, sscanf value is %u\n", __func__, value);

	info->op_action = TS_ACTION_WRITE;
	info->holster_switch = value;

	error = ts_send_holster_cmd();
	if (error) {
		TS_LOG_ERR("ts_send_holster_cmd failed\n");
		error = -ENOMEM;
		goto out;
	}

	error = count;

out:
	TS_LOG_DEBUG("ts_sensitivity_store done\n");
	return error;
}

static ssize_t ts_hand_detect_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_hand_info *info = NULL;

	TS_LOG_INFO("ts_hand_detect_show called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	info =
	    (struct ts_hand_info *)kzalloc(sizeof(struct ts_hand_info),
					   GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info->op_action = TS_ACTION_READ;
	cmd->command = TS_HAND_DETECT;
	cmd->cmd_param.prv_params = (void *)info;
	error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info->status == TS_ACTION_SUCCESS)
		error = snprintf(buf, MAX_STR_LEN, "%d\n", info->hand_value);
	else
		error = -EFAULT;

out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}
	TS_LOG_DEBUG("done\n");
	return error;
}
/*http://3ms.huawei.com/hi/group/2034707/wiki_3999893.html?for_statistic_from=all_group_wiki
 *Name : supported_func_indicater
 *Description: The node each bit represents a functional markers . Currently defined tags have six .
 *Bit0: Indicates whether to support bright screen calibration , 1 expressed support for the use of LGD module .
 *Bit1: Indicates whether to support off -screen calibration , 1 expressed support for gesture support black self-capacitance modules (LGD) using
 *Bit2: Indicates whether a consonance support means 1 not supported .
 *Bit3: knuckles
 *Bit4: Indicates whether pressure support function , expressed support for 1 , 0 indicates no support .
 *Bit5: Indicates whether to support TP calibration data CRC check, and TP calibration data is stored .
 */
static ssize_t ts_supported_func_indicater_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int error = NO_ERR;

	TS_LOG_INFO("%s called\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}
	/*supported_func_indicater, bit order: right -> left. bit0: calibrate,
	 * bit1: calibrate_wakeup_gesture, bit2: calibration data crc check. */
	error =
	    snprintf(buf, MAX_STR_LEN, "%d\n",
		     g_ts_data.chip_data->supported_func_indicater);

out:
	TS_LOG_INFO("%s done\n", __func__);
	return error;
}

static ssize_t ts_loglevel_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;

	TS_LOG_INFO("ts_loglevel_show called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = snprintf(buf, MAX_STR_LEN, "%d\n", g_ts_log_cfg);

out:
	TS_LOG_INFO("ts_loglevel_show done\n");
	return error;
}

static ssize_t ts_touch_window_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;
	struct ts_window_info *info = &g_ts_data.feature_info.window_info;

	error =
	    snprintf(buf, MAX_STR_LEN, "%d %d %d %d %d\n", info->window_enable,
		     info->top_left_x0, info->top_left_y0,
		     info->bottom_right_x1, info->bottom_right_y1);

	return error;
}

static ssize_t ts_loglevel_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	unsigned int value;
	int error;
	struct ts_device_data *dev_data = g_ts_data.chip_data;

	TS_LOG_INFO("ts_loglevel_store called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = sscanf(buf, "%u", &value);
	if (!error) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_DEBUG("sscanf value is %u\n", value);
	g_ts_log_cfg = value;
	error = count;

	if (dev_data->ops->chip_debug_switch)
		dev_data->ops->chip_debug_switch(g_ts_log_cfg);

out:
	TS_LOG_INFO("ts_loglevel_store done\n");
	return error;
}

static ssize_t ts_fw_update_sd_store(struct device *dev,
				     struct device_attribute *attr,
				     const char *buf, size_t count)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;

	TS_LOG_INFO("ts_fw_update_sd_store called\n");

	memset(&cmd, 0, sizeof(struct ts_cmd_node));
	cmd.command = TS_FW_UPDATE_SD;
	if (g_ts_data.chip_data->is_direct_proc_cmd){
		error = ts_proc_command_directly(&cmd);
	}else{
		error = put_one_cmd(&cmd, LONG_LONG_SYNC_TIMEOUT);
	}

	if (error) {
		TS_LOG_ERR("ts_fw_update_sd_store failed:%d\n", error);
	}
	TS_LOG_INFO("ts_fw_update_sd_store done\n");

	return count;
}

static ssize_t ts_register_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	int error = NO_ERR;
	int count = 0;
	char *buf_value = NULL;
	int i = 0;
	int ret = 0;

	TS_LOG_INFO("ts_register_show is called\n");
	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		return error;
	}
	buf_value = buf;
	for (i = 0; i < g_ts_data.chip_data->reg_num; i++) {
		ret =
		    snprintf(buf_value, TS_MAX_REG_VALUE_NUM, "0x%02x",
			     g_ts_data.chip_data->reg_values[i]);
		buf_value += ret;
		count += ret;	/*0x%2x,one num need four char to show */
	}
	TS_LOG_DEBUG
	    ("show reg value error,maybe no store first,count = %d, buf = %s\n",
	     count, buf);
	if (count <= 0) {
		TS_LOG_ERR
		    ("show reg value error,maybe no store first,count = %d, buf = %s\n",
		     count, buf);
		error = -EINVAL;
		return error;
	}
	TS_LOG_INFO("ts_register_show done\n");
	return count;
}

static int ts_strsep_and_parse_int_value(char **stringp, const char *delim, const char *format, int *ret_value)
{
	char *token;
	int error = NO_ERR;

	token = strsep(stringp, delim);
	if (!token) {
		TS_LOG_ERR("parse parameter error1\n");
		return -EINVAL;
	}
	error = sscanf(token, format, ret_value);
	if (error <= 0) {
		TS_LOG_ERR("parse parameter error2\n");
		return -EINVAL;
	}
	return NO_ERR;
}

static int ts_register_store_parse_param(char *buf, int *flag, unsigned int *addr, char *value, int *num, int *bit)
{
	char *cur;
	char *token;
	cur = buf;
	int error = NO_ERR;

	if (ts_strsep_and_parse_int_value(&cur, " ", "%d", flag) != NO_ERR) {
		TS_LOG_ERR("parse parameter flag error\n");
		return -EINVAL;
	}

	token = strsep(&cur, " ");
	if (!token) {
		TS_LOG_ERR("parse parameter addr error1\n");
		return -EINVAL;
	}
	error = sscanf(token, "0x%4x", addr);
	if (error <= 0) {
		TS_LOG_ERR("parse parameter addr error2\n");
		return -EINVAL;
	}

	token = strsep(&cur, " ");
	if (!token || strlen(token) > TS_MAX_REG_VALUE_NUM-1) { /*%s avoid stack overflow*/
		TS_LOG_ERR("parse parameter value error1\n");
		return -EINVAL;
	}
	error = sscanf(token, "%s", value);
	if (error <= 0) {
		TS_LOG_ERR("parse parameter value error2\n");
		return -EINVAL;
	}

	if (ts_strsep_and_parse_int_value(&cur, " ", "%d", num) != NO_ERR) {
		TS_LOG_ERR("parse parameter num error\n");
		return -EINVAL;
	}

	if (ts_strsep_and_parse_int_value(&cur, " ", "%d", bit) != NO_ERR) {
		TS_LOG_ERR("parse parameter bit error\n");
		return -EINVAL;
	}
	return NO_ERR;
}

static ssize_t ts_register_store(struct device *dev,
				 struct device_attribute *attr, const char *buf,
				 size_t count)
{
	unsigned int addr = 0;
	int bit = 0;
	char *value = NULL;
	u8 value_u8[TS_MAX_REG_VALUE_NUM] = { 0 };
	int num = 0;
	int flag = 0;
	int error = NO_ERR;
	int i = 0;
	struct ts_cmd_node *cmd = NULL;
	struct ts_regs_info *info = NULL;

	TS_LOG_INFO("ts_register_store called\n");
	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	if(strlen(buf) >= TS_MAX_REG_VALUE_NUM){
		TS_LOG_ERR("Input too long\n");
		return count;
	}

	value = (char *)kzalloc(TS_MAX_REG_VALUE_NUM, GFP_KERNEL);
	if (!value) {
		TS_LOG_ERR("value kzalloc error\n");
		error = -ENOMEM;
		goto out;
	}

	if (ts_register_store_parse_param((char *)buf, &flag, &addr, value, &num, &bit) != NO_ERR) {
		TS_LOG_ERR("parse parameter error\n");
		error = -EINVAL;
		goto free_memory;
	}

	if (num > TS_MAX_REG_VALUE_NUM/4 - 1) {
		TS_LOG_ERR("input num is larger than the max!\n");
		num = TS_MAX_REG_VALUE_NUM/4 - 1;
	}

	TS_LOG_INFO
	    ("sscanf return data is flag:%d, addr:%d, value:%s, num:%d, bit:%d\n",
	     flag, addr, value, num, bit);

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto free_memory;
	}
	info =
	    (struct ts_regs_info *)kzalloc(sizeof(struct ts_regs_info),
					   GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto free_memory;
	}

	if (0 == flag) {
		info->op_action = TS_ACTION_READ;
	} else {
		info->op_action = TS_ACTION_WRITE;
	}
	info->addr = addr;
	info->bit = bit;
	info->num = num;
	g_ts_data.chip_data->reg_num = num;

	/*convert char to u8 because sscanf char from buf */
	for (i = 0; i < num; i++) {
		if ('0' <= value[4 * i + 2] && value[4 * i + 2] <= '9') {
			value_u8[2 * i] = value[4 * i + 2] - '0';
		} else if ('a' <= _tolower(value[4 * i + 2])
			   && _tolower(value[4 * i + 2]) <= 'f') {
			value_u8[2 * i] = _tolower(value[4 * i + 2]) - 'a' + 10;
		}
		if ('0' <= value[4 * i + 3] && value[4 * i + 3] <= '9') {
			value_u8[2 * i + 1] = value[4 * i + 3] - '0';
		} else if ('a' <= _tolower(value[4 * i + 3])
			   && _tolower(value[4 * i + 3]) <= 'f') {
			value_u8[2 * i + 1] =
			    _tolower(value[4 * i + 3]) - 'a' + 10;
		}
	}

	/*convert input value to reg_values */
	for (i = 0; i < num; i++) {
		info->values[i] =
		    (value_u8[2 * i] << 4) | (value_u8[2 * i + 1]);
	}

	cmd->command = TS_REGS_STORE;
	cmd->cmd_param.prv_params = (void *)info;
	error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto free_memory;
	}
	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("action failed\n");
		error = -EIO;
		goto free_memory;
	}
	error = count;

free_memory:
	if (info){
		kfree(info);
		info = NULL;}
	if (cmd){
		kfree(cmd);
		cmd = NULL;
	}
	if (value){
		kfree(value);
		value = NULL;}
out:
	TS_LOG_INFO("ts_reg_operate_store done\n");
	return error;
}

static ssize_t ts_wakeup_gesture_enable_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	unsigned int value;
	int error;

	TS_LOG_INFO("%s called\n", __func__);

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}

	error = ts_wakeup_gesture_enable_cmd(value);
	if (error) {
		TS_LOG_ERR("ts_wakeup_gesture_enable_cmd failed\n");
		error = -ENOMEM;
		goto out;
	}

	error = count;

out:
	return error;
}

static ssize_t ts_wakeup_gesture_enable_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	struct ts_wakeup_gesture_enable_info *info =
	    &g_ts_data.feature_info.wakeup_gesture_enable_info;
	ssize_t ret;

	TS_LOG_INFO("%s called\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		ret = -EINVAL;
		goto out;
	}

	ret = snprintf(buf, MAX_STR_LEN, "%d\n", info->switch_value);
out:
	return ret;
}

static ssize_t ts_easy_wakeup_gesture_show(struct device *dev,
					   struct device_attribute *attr,
					   char *buf)
{
	struct ts_easy_wakeup_info *info =
	    &g_ts_data.chip_data->easy_wakeup_info;
	ssize_t ret;

	TS_LOG_INFO("%s\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		ret = -EINVAL;
		goto out;
	}

	ret =
	    snprintf(buf, MAX_STR_LEN, "0x%04X\n",
		     info->easy_wakeup_gesture +
		     info->palm_cover_flag * TS_GET_CALCULATE_NUM);
out:
	return ret;
}

static ssize_t ts_easy_wakeup_gesture_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t size)
{

	struct ts_easy_wakeup_info *info =
	    &g_ts_data.chip_data->easy_wakeup_info;
	unsigned long value;
	int ret;
	struct ts_cmd_node *cmd = NULL;
	struct ts_palm_info *palm_info = NULL;

	TS_LOG_INFO("ts_easy_wakeup_gesture_store_called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		ret = -EINVAL;
		goto out;
	}

	if ((TS_SLEEP == atomic_read(&g_ts_data.state))
	    || (TS_WORK_IN_SLEEP == atomic_read(&g_ts_data.state))) {
		TS_LOG_ERR
		    ("do not echo this node when tp work in sleep or tp is sleep\n");
		ret = -EINVAL;
		goto out;
	}

	ret = kstrtoul(buf, 10, &value);
	if (ret < 0)
		return ret;
	if (value > TS_GESTURE_INVALID_COMMAND)
		return -1;
	info->easy_wakeup_gesture = (u16) value & TS_GESTURE_COMMAND;
	info->palm_cover_flag =
	    (u16) (value & TS_GESTURE_PALM_BIT) / TS_GET_CALCULATE_NUM;
	if (true == info->palm_cover_flag)
		info->palm_cover_control = true;
	else
		info->palm_cover_control = false;
	TS_LOG_INFO("easy_wakeup_gesture=0x%x,palm_cover_flag=0x%x\n",
		    info->easy_wakeup_gesture, info->palm_cover_flag);
	if (false == info->easy_wakeup_gesture) {
		info->sleep_mode = TS_POWER_OFF_MODE;
		gesture_func = false;
		TS_LOG_INFO("poweroff mode\n");
	} else {
		info->sleep_mode = TS_GESTURE_MODE;
		gesture_func = true;
		TS_LOG_INFO("gesture mode\n");
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		ret = -ENOMEM;
		goto out;
	}
	palm_info =
	    (struct ts_palm_info *)kzalloc(sizeof(struct ts_palm_info),
					   GFP_KERNEL);
	if (!palm_info) {
		TS_LOG_ERR("malloc failed\n");
		ret = -ENOMEM;
		goto out;
	}
	palm_info->op_action = TS_ACTION_WRITE;
	palm_info->palm_switch = info->palm_cover_control;
	cmd->command = TS_PALM_SWITCH;
	cmd->cmd_param.prv_params = (void *)palm_info;
	ret = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	if (ret) {
		TS_LOG_ERR("put cmd error :%d\n", ret);
		ret = -EBUSY;
		goto out;
	}
	if (palm_info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("action failed\n");
		ret = -EIO;
		goto out;
	}
	ret = size;
out:
	if (palm_info){
		kfree(palm_info);
		palm_info =NULL;
	}
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	TS_LOG_DEBUG("ts gesture wakeup no done\n");

	return ret;
}

static ssize_t ts_easy_wakeup_control_store(struct device *dev,
					    struct device_attribute *attr,
					    const char *buf, size_t size)
{

	unsigned long value;
	int ret;
	int error = NO_ERR;

	TS_LOG_INFO("called\n");

	ret = kstrtoul(buf, 10, &value);
	if (ret < 0)
		return ret;

	if (value > TS_GESTURE_INVALID_CONTROL_NO )
		return -1;

	value = (u8) value & TS_GESTURE_COMMAND;
	if (1 == value) {
		if (NULL != g_ts_data.chip_data->ops->chip_wrong_touch) {
			error = g_ts_data.chip_data->ops->chip_wrong_touch();
			if (error < 0) {
				TS_LOG_INFO("chip_wrong_touch error\n");
			}
		} else {
			TS_LOG_INFO("chip_wrong_touch not init\n");
		}
		value = 0;
	}
	TS_LOG_INFO("done\n");
	return size;
}

static ssize_t ts_easy_wakeup_position_show(struct device *dev,
					    struct device_attribute *attr,
					    char *buf)
{
	int ret = 0;
	char temp[9] = { 0 };
	int i = 0;
	TS_LOG_INFO("ts_position_show\n");
	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		ret = -EINVAL;
		goto out;
	}
	mutex_lock(&easy_wake_guesure_lock);
	for (i = 0; i < MAX_LOTUS_NUM; i++) {
		ret =
		    snprintf(temp, (sizeof(u32) * 2 + 1), "%08x",
			     g_ts_data.chip_data->easy_wakeup_info.
			     easywake_position[i]);
		if(ret < 0){
			TS_LOG_ERR("snprintf failed \n");
		}
		strncat(buf, temp, (sizeof(u32) * 2 + 1));
	}
	strncat(buf, "\n", 1);
	strncat(buf, "\0", 1);
	mutex_unlock(&easy_wake_guesure_lock);
	ret = (strlen(buf) + 1);
out:
	return ret;
}

#if defined (CONFIG_TEE_TUI)
static ssize_t ts_tui_report_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	int ret = 0;

	ret = snprintf(buf, 10, "%d", g_ts_data.chip_data->report_tui_enable);

	return ret;
}

static ssize_t ts_tui_report_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	int ret = 0;
	int value = 0;

	ret = sscanf(buf, "%d", &value);

	g_ts_data.chip_data->report_tui_enable = value;

	if (g_ts_data.chip_data->report_tui_enable == true) {
		ts_tui_secos_init();
	} else {
		ts_tui_secos_exit();
	}

	TS_LOG_INFO("tui_report enable is %d\n",
		    g_ts_data.chip_data->report_tui_enable);

	return count;
}
#endif

static ssize_t ts_roi_enable_show(struct device *dev,
				  struct device_attribute *attr, char *buf)
{
	unsigned int value;
	int error;

	error = ts_send_roi_cmd(TS_ACTION_READ, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("ts_send_roi_show_cmd failed\n");
		error = -ENOMEM;
		goto out;
	}
	value = g_ts_data.feature_info.roi_info.roi_switch;
	error = snprintf(buf, MAX_STR_LEN, "%d\n", value);
out:
	TS_LOG_INFO("roi_enable_show done\n");
	return error;
}

static ssize_t ts_roi_enable_store(struct device *dev,
				   struct device_attribute *attr,
				   const char *buf, size_t count)
{
	unsigned int value;
	int error;
	struct ts_roi_info *info = &g_ts_data.feature_info.roi_info;

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_DEBUG("sscanf value is %u\n", value);

	if (info->roi_switch == value) {
		TS_LOG_INFO
		    ("%s, there is no need to send same cmd twice. roi_switch valie is %u",
		     __func__, value);
		error = count;
		goto out;
	}

	info->roi_switch = value;
	error = ts_send_roi_cmd(TS_ACTION_WRITE, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("ts_send_roi_store_cmd failed\n");
		error = -ENOMEM;
		goto out;
	}
	error = count;

out:
	TS_LOG_INFO("roi_enable_store done\n");
	return error;
}

static ssize_t ts_roi_data_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	unsigned char *roi_data_p = NULL;
	struct ts_device_data *dev_data = g_ts_data.chip_data;

	if (dev_data->ops->chip_roi_rawdata)
		roi_data_p = dev_data->ops->chip_roi_rawdata();
	if (NULL == roi_data_p) {
		TS_LOG_ERR("not define ROI for roi_data_show \n");
		return -ENOMEM;
	}
	/*roi_data_temp <-- This is the buffer that has the ROI data you want to send to Qeexo*/
	memcpy(buf, roi_data_p + ROI_HEAD_DATA_LENGTH, ROI_DATA_SEND_LENGTH);
	return ROI_DATA_SEND_LENGTH;
}

static ssize_t ts_roi_data_debug_show(struct device *dev,
				      struct device_attribute *attr, char *buf)
{
	int cnt;
	int count = 0;
	int i, j = 0;
	short roi_data_16[ROI_DATA_SEND_LENGTH / 2];
	unsigned char *roi_data_p = NULL;
	struct ts_device_data *dev_data = g_ts_data.chip_data;

	if (dev_data->ops->chip_roi_rawdata)
		roi_data_p = dev_data->ops->chip_roi_rawdata();
	if (NULL == roi_data_p) {
		TS_LOG_ERR("not define ROI for roi_data_show \n");
		return -ENOMEM;
	}
	TS_LOG_DEBUG("ts_roi_data_debug_show CALLED \n");

	for (i = 0; i < ROI_HEAD_DATA_LENGTH; i++) {
		cnt =
		    snprintf(buf, PAGE_SIZE - count, "%2d\n",
			     (char)roi_data_p[i]);
		buf += cnt;
		count += cnt;
	}

	for (i = ROI_HEAD_DATA_LENGTH; i < ROI_DATA_READ_LENGTH; i += 2, j++) {
		roi_data_16[j] = roi_data_p[i] | (roi_data_p[i + 1] << 8);
		cnt = snprintf(buf, PAGE_SIZE - count -1, "%4d\t", roi_data_16[j]);
		buf += cnt;
		count += cnt;

		if ((j + 1) % 7 == 0) {
			cnt = snprintf(buf, PAGE_SIZE - count - 1, "\n");
			buf += cnt;
			count += cnt;
		}
	}
	snprintf(buf, PAGE_SIZE - count - 1, "\n");
	count++;
	return count;
}

static ssize_t ts_capacitance_test_type_show(struct device *dev,
					     struct device_attribute *attr,
					     char *buf)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_test_type_info *info = NULL;

	TS_LOG_INFO("ts_touch_test_mode_show called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	info =
	    (struct ts_test_type_info *)
	    kzalloc(sizeof(struct ts_test_type_info), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info->op_action = TS_ACTION_READ;
	cmd->command = TS_TEST_TYPE;
	cmd->cmd_param.prv_params = (void *)info;
	error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info->status == TS_ACTION_SUCCESS)
		error =
		    snprintf(buf, TS_CAP_TEST_TYPE_LEN, "%s\n",
			     info->tp_test_type);
	else
		error = -EFAULT;

out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}
	TS_LOG_DEBUG("ts_touch_test_mode_show done\n");
	return error;
}

static ssize_t ts_capacitance_test_type_store(struct device *dev,
					      struct device_attribute *attr,
					      const char *buf, size_t count)
{
	unsigned int value;
	int error;

	TS_LOG_INFO("ts_capacitance_test_type_store called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_DEBUG("sscanf value is %u\n", value);
	error = count;

out:
	TS_LOG_DEBUG("ts_capacitance_test_type_store done\n");
	return error;
}

/*
 *Name : ts_capacitance_test_config_show
 *Description: The node each bit represents a functional markers.
 *Bit0: Ignore fgets fail by reason of timeout in runningtest.
 */
static ssize_t ts_capacitance_test_config_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	int error = NO_ERR;

	TS_LOG_INFO("%s called\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}
	error =
	    snprintf(buf, MAX_STR_LEN, "%d\n",
		     g_ts_data.chip_data->capacitance_test_config);

out:
	TS_LOG_INFO("%s done\n", __func__);
	return error;
}

static ssize_t ts_rawdata_debug_test_show(struct device *dev,
					  struct device_attribute *attr,
					  char *buf)
{
	int index = 0;
	int index1 = 0;
	int count = 0;
	short row_size = 0;
	int range_size = 0;
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_diff_data_info *info = NULL;
	char buffer1[BUFFER1_LEN] = { 0 };
	char buffer2[BUFFER2_LEN] = { 0 };

	TS_LOG_INFO("ts_rawdata_debug_test_show called\n");

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	info =
	    (struct ts_diff_data_info *)
	    kzalloc(sizeof(struct ts_diff_data_info), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info->used_size = 0;
	info->debug_type = g_ts_data.chip_data->rawdata_debug_type;
	cmd->command = TS_DEBUG_DATA;
	cmd->cmd_param.prv_params = (void *)info;

	error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}

	sprintf(buf, "*************touch debug data*************\n");
	count = count + 42;

	row_size = info->buff[0];
	if ((row_size <= 0) || (info->used_size) <= 0){
		TS_LOG_ERR("%s data error! DO NOT surport this mode!", __func__);
		goto out;
	}
	range_size = info->buff[1];
	sprintf(buffer2, "rx: %d, tx : %d\n ", row_size, range_size);
	strncat(buf, buffer2, strlen(buffer2));
	count = count + strlen(buffer2);

	TS_LOG_INFO("info->used+size = %d\n", info->used_size);

	for (index = 0; row_size * index + 2 < info->used_size; index++) {
		for (index1 = 0; index1 < row_size; index1++) {
			sprintf(buffer1, "%d,",
				info->buff[2 + row_size * index + index1]);
			strncat(buf, buffer1, strlen(buffer1));
			count = count + strlen(buffer1);
		}
		sprintf(buffer1, "\n ");
		strncat(buf, buffer1, strlen(buffer1));
		count = count + strlen(buffer1);
	}

	strcat(buf, "noisedata end\n");
	count = count + sizeof("noisedata end\n");
	error = count;

out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}

	TS_LOG_INFO("ts_rawdata_debug_test_show done\n");
	return error;
}

static ssize_t ts_rawdata_debug_test_store(struct device *dev,
					   struct device_attribute *attr,
					   const char *buf, size_t count)
{
	unsigned int value;
	int error;

	TS_LOG_INFO("%s called\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_INFO("sscanf value is %u\n", value);

	g_ts_data.chip_data->rawdata_debug_type = value;

	error = count;

out:
	TS_LOG_DEBUG("%s done\n", __func__);
	return error;
}

static ssize_t touch_special_hardware_test_show(struct device *dev,
						struct device_attribute *attr,
						char *buf)
{
	struct ts_device_data *dev_data = g_ts_data.chip_data;
	ssize_t ret = 0;

	TS_LOG_INFO("%s\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		ret = -EINVAL;
		goto out;
	}

	if (dev_data->ops->chip_special_hardware_test_result) {
		ret = dev_data->ops->chip_special_hardware_test_result(buf);
	} else {
		ret = snprintf(buf, MAX_STR_LEN, "pass\n");
	}

out:
	return ret;
}

static void ts_special_hardware_test_switch(struct ts_cmd_node *in_cmd,
					    struct ts_cmd_node *out_cmd)
{
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_special_hardware_test_info *info =
	    (struct ts_special_hardware_test_info *)in_cmd->cmd_param.
	    prv_params;

	TS_LOG_INFO("%s, action :%d, value:%d\n", __func__, info->op_action,
		    info->switch_value);
	if (dev->ops->chip_special_hardware_test_swtich) {
		dev->ops->chip_special_hardware_test_swtich(info->switch_value);
	}
}

static ssize_t touch_special_hardware_test_store(struct device *dev,
						 struct device_attribute *attr,
						 const char *buf, size_t count)
{
	unsigned int value;
	int error;
	struct ts_cmd_node *cmd = NULL;
	struct ts_special_hardware_test_info *info = NULL;

	TS_LOG_INFO("%s called\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_DEBUG("sscanf value is %u\n", value);

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info = &g_ts_data.feature_info.hardware_test_info;
	info->op_action = TS_ACTION_WRITE;
	info->switch_value = value;
	cmd->command = TS_HARDWARE_TEST;
	cmd->cmd_param.prv_params = (void *)info;
	error = put_one_cmd(cmd, NO_SYNC_TIMEOUT);
	if (error < 0) {
		TS_LOG_ERR("put_one_cmd failed\n");
		error = -EINVAL;
		goto out;
	}

	error = count;
out:
	if (cmd) {
		kfree(cmd);
		cmd = NULL;
	}
	TS_LOG_DEBUG("%s done\n", __func__);
	return error;
}

static ssize_t ts_touch_wideth_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	TS_LOG_INFO("ts_touch_wideth_show done\n");
	return snprintf(buf, MAX_STR_LEN, "%d\n", g_ts_data.edge_wideth);
}

static ssize_t ts_touch_wideth_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	unsigned int value;
	int error;

	TS_LOG_INFO("ts_touch_wideth_store called\n");

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		error = -EINVAL;
		goto out;
	}

	error = sscanf(buf, "%u", &value);
	if (error <= 0) {
		TS_LOG_ERR("sscanf return invaild :%d\n", error);
		error = -EINVAL;
		goto out;
	}
	TS_LOG_INFO("sscanf value is %u\n", value);

	g_ts_data.edge_wideth = value;

	error = count;

out:
	return error;
}

//struct anti_false_touch_param *g_anti_false_touch_param = NULL;
void ts_anti_false_touch_param_achieve(struct ts_device_data *chip_data){
	int retval  = NO_ERR;
	unsigned int value = 0;
	struct anti_false_touch_param *local_param = NULL;
	struct device_node *root = g_ts_data.node;
	struct device_node *device = NULL;
	bool found = false;

	TS_LOG_INFO("%s +\n", __func__);
	if (NULL == chip_data){
		TS_LOG_ERR("%s anti false touch get chip data NULL\n", __func__);
		return ;
	}
	local_param = &(chip_data->anti_false_touch_param_data);
	//g_anti_false_touch_param = local_param;
	memset(local_param, 0, sizeof(struct anti_false_touch_param));

	TS_LOG_INFO("%s chip_name:%s\n", __func__, chip_data->chip_name);
	for_each_child_of_node(root, device) {	/*find the chip node*/
		if (of_device_is_compatible(device, chip_data->chip_name)) {
			found = true;
			break;
		}
	}

	if (false == found){
		TS_LOG_ERR("%s anti false touch find dts node fail\n", __func__);
		return ;
	}

	//feature_all
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_ALL, &value);
	if (retval) {
		local_param->feature_all = 0;
		TS_LOG_ERR("get device feature_all failed\n");
	}else{
		local_param->feature_all = value;
	}

	if (!local_param->feature_all){
		TS_LOG_INFO("product do not suppurt avert misoper, set all param to 0\n");
		return ;
	}

	//feature_resend_point
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT, &value);
	if (retval) {
		local_param->feature_resend_point = 0;
		TS_LOG_ERR("get device feature_resend_point failed\n");
	}else{
		local_param->feature_resend_point = value;
	}

	//feature_orit_support
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT, &value);
	if (retval) {
		local_param->feature_orit_support = 0;
		TS_LOG_ERR("get device feature_orit_support failed\n");
	}else{
		local_param->feature_orit_support = value;
	}

	//feature_reback_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_REBACK_BT, &value);
	if (retval) {
		local_param->feature_reback_bt = 0;
		TS_LOG_ERR("get device feature_reback_bt failed\n");
	}else{
		local_param->feature_reback_bt = value;
	}

	//lcd_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_LCD_WIDTH, &value);
	if (retval) {
		local_param->lcd_width = 0;
		TS_LOG_ERR("get device lcd_width failed\n");
	}else{
		local_param->lcd_width = value;
	}

	//lcd_height
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_LCD_HEIGHT, &value);
	if (retval) {
		local_param->lcd_height = 0;
		TS_LOG_ERR("get device lcd_height failed\n");
	}else{
		local_param->lcd_height = value;
	}

	//click_time_limit
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT, &value);
	if (retval) {
		local_param->click_time_limit = 0;
		TS_LOG_ERR("get device click_time_limit failed\n");
	}else{
		local_param->click_time_limit = value;
	}

	//click_time_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_CLICK_TIME_BT, &value);
	if (retval) {
		local_param->click_time_bt = 0;
		TS_LOG_ERR("get device click_time_bt failed\n");
	}else{
		local_param->click_time_bt = value;
	}

	//edge_position
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_POISION, &value);
	if (retval) {
		local_param->edge_position = 0;
		TS_LOG_ERR("get device edge_position failed\n");
	}else{
		local_param->edge_position = value;
	}

	//edge_postion_secondline
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE, &value);
	if (retval) {
		local_param->edge_postion_secondline = 0;
		TS_LOG_ERR("get device edge_postion_secondline failed\n");
	}else{
		local_param->edge_postion_secondline = value;
	}

	//bt_edge_x
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_BT_EDGE_X, &value);
	if (retval) {
		local_param->bt_edge_x = 0;
		TS_LOG_ERR("get device bt_edge_x failed\n");
	}else{
		local_param->bt_edge_x = value;
	}

	//bt_edge_y
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_BT_EDGE_Y, &value);
	if (retval) {
		local_param->bt_edge_y = 0;
		TS_LOG_ERR("get device bt_edge_y failed\n");
	}else{
		local_param->bt_edge_y = value;
	}

	//move_limit_x
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_X, &value);
	if (retval) {
		local_param->move_limit_x = 0;
		TS_LOG_ERR("get device move_limit_x failed\n");
	}else{
		local_param->move_limit_x = value;
	}

	//move_limit_y
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y, &value);
	if (retval) {
		local_param->move_limit_y = 0;
		TS_LOG_ERR("get device move_limit_y failed\n");
	}else{
		local_param->move_limit_y = value;
	}

	//move_limit_x_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T, &value);
	if (retval) {
		local_param->move_limit_x_t = 0;
		TS_LOG_ERR("get device move_limit_x_t failed\n");
	}else{
		local_param->move_limit_x_t = value;
	}

	//move_limit_y_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T, &value);
	if (retval) {
		local_param->move_limit_y_t = 0;
		TS_LOG_ERR("get device move_limit_y_t failed\n");
	}else{
		local_param->move_limit_y_t = value;
	}

	//move_limit_x_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT, &value);
	if (retval) {
		local_param->move_limit_x_bt = 0;
		TS_LOG_ERR("get device move_limit_x_bt failed\n");
	}else{
		local_param->move_limit_x_bt = value;
	}

	//move_limit_y_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT, &value);
	if (retval) {
		local_param->move_limit_y_bt = 0;
		TS_LOG_ERR("get device move_limit_y_bt failed\n");
	}else{
		local_param->move_limit_y_bt = value;
	}

	//edge_y_confirm_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T, &value);
	if (retval) {
		local_param->edge_y_confirm_t = 0;
		TS_LOG_ERR("get device edge_y_confirm_t failed\n");
	}else{
		local_param->edge_y_confirm_t = value;
	}

	//edge_y_dubious_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T, &value);
	if (retval) {
		local_param->edge_y_dubious_t = 0;
		TS_LOG_ERR("get device edge_y_dubious_t failed\n");
	}else{
		local_param->edge_y_dubious_t = value;
	}

	//edge_y_avg_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT, &value);
	if (retval) {
		local_param->edge_y_avg_bt = 0;
		TS_LOG_ERR("get device edge_y_avg_bt failed\n");
	}else{
		local_param->edge_y_avg_bt = value;
	}

	//edge_xy_down_bt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT, &value);
	if (retval) {
		local_param->edge_xy_down_bt = 0;
		TS_LOG_ERR("get device edge_xy_down_bt failed\n");
	}else{
		local_param->edge_xy_down_bt = value;
	}

	//edge_xy_confirm_t
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T, &value);
	if (retval) {
		local_param->edge_xy_confirm_t = 0;
		TS_LOG_ERR("get device edge_xy_confirm_t failed\n");
	}else{
		local_param->edge_xy_confirm_t = value;
	}

	//max_points_bak_num
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM, &value);
	if (retval) {
		local_param->max_points_bak_num = 0;
		TS_LOG_ERR("get device max_points_bak_num failed\n");
	}else{
		local_param->max_points_bak_num = value;
	}

	//drv_stop_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_DRV_STOP_WIDTH, &value);
	if (retval) {
		local_param->drv_stop_width = 0;
		TS_LOG_ERR("get device drv_stop_width failed\n");
	}else{
		local_param->drv_stop_width = value;
	}

	//sensor_x_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_SENSOR_X_WIDTH, &value);
	if (retval) {
		local_param->sensor_x_width = 0;
		TS_LOG_ERR("get device sensor_x_width failed\n");
	}else{
		local_param->sensor_x_width = value;
	}

	//sensor_y_width
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH, &value);
	if (retval) {
		local_param->sensor_y_width = 0;
		TS_LOG_ERR("get device sensor_y_width failed\n");
	}else{
		local_param->sensor_y_width = value;
	}

	/* emui5.1 support */
	//feature_sg
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_SG, &value);
	if (retval) {
		local_param->feature_sg = 0;
		TS_LOG_ERR("get device feature_sg failed\n");
	}else{
		local_param->feature_sg = value;
	}

	//sg_min_value
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_SG_MIN_VALUE, &value);
	if (retval) {
		local_param->sg_min_value = 0;
		TS_LOG_ERR("get device sg_min_value failed\n");
	}else{
		local_param->sg_min_value = value;
	}

	//feature_support_list
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST, &value);
	if (retval) {
		local_param->feature_support_list = 0;
		TS_LOG_ERR("get device feature_support_list failed\n");
	}else{
		local_param->feature_support_list = value;
	}

	//max_distance_dt
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MAX_DISTANCE_DT, &value);
	if (retval) {
		local_param->max_distance_dt = 0;
		TS_LOG_ERR("get device max_distance_dt failed\n");
	}else{
		local_param->max_distance_dt = value;
	}

	//feature_big_data
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_BIG_DATA, &value);
	if (retval) {
		local_param->feature_big_data = 0;
		TS_LOG_ERR("get device feature_big_data failed\n");
	}else{
		local_param->feature_big_data = value;
	}

	//feature_click_inhibition
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION, &value);
	if (retval) {
		local_param->feature_click_inhibition = 0;
		TS_LOG_ERR("get device feature_click_inhibition failed\n");
	}else{
		local_param->feature_click_inhibition = value;
	}

	//min_click_time
	retval = of_property_read_u32(device, ANTI_FALSE_TOUCH_MIN_CLICK_TIME, &value);
	if (retval) {
		local_param->min_click_time = 0;
		TS_LOG_ERR("get device min_click_time failed\n");
	}else{
		local_param->min_click_time = value;
	}

	TS_LOG_INFO("%s:"
		" feature_all:%d, feature_resend_point:%d, feature_orit_support:%d, feature_reback_bt:%d, lcd_width:%d, lcd_height:%d,"
		" click_time_limit:%d, click_time_bt:%d, edge_position:%d, edge_postion_secondline:%d, bt_edge_x:%d, bt_edge_y:%d,"
		" move_limit_x:%d, move_limit_y:%d, move_limit_x_t:%d, move_limit_y_t:%d, move_limit_x_bt:%d,"
		" move_limit_y_bt:%d, edge_y_confirm_t:%d, edge_y_dubious_t:%d, edge_y_avg_bt:%d, edge_xy_down_bt:%d, edge_xy_confirm_t:%d, max_points_bak_num:%d,"
		" drv_stop_width:%d sensor_x_width:%d, sensor_y_width:%d,"
		" feature_sg:%d, sg_min_value:%d, feature_support_list:%d, max_distance_dt:%d, feature_big_data:%d, feature_click_inhibition:%d, min_click_time:%d\n",
		__func__,
		local_param->feature_all,
		local_param->feature_resend_point,
		local_param->feature_orit_support,
		local_param->feature_reback_bt,
		local_param->lcd_width,
		local_param->lcd_height,

		local_param->click_time_limit,
		local_param->click_time_bt,
		local_param->edge_position,
		local_param->edge_postion_secondline,
		local_param->bt_edge_x,
		local_param->bt_edge_y,

		local_param->move_limit_x,
		local_param->move_limit_y,
		local_param->move_limit_x_t,
		local_param->move_limit_y_t,
		local_param->move_limit_x_bt,

		local_param->move_limit_y_bt,
		local_param->edge_y_confirm_t,
		local_param->edge_y_dubious_t,
		local_param->edge_y_avg_bt,
		local_param->edge_xy_down_bt,
		local_param->edge_xy_confirm_t,
		local_param->max_points_bak_num,

		local_param->drv_stop_width,
		local_param->sensor_x_width,
		local_param->sensor_y_width,

		local_param->feature_sg,
		local_param->sg_min_value,
		local_param->feature_support_list,
		local_param->max_distance_dt,
		local_param->feature_big_data,
		local_param->feature_click_inhibition,
		local_param->min_click_time);
}

static ssize_t ts_anti_false_touch_param_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct anti_false_touch_param *local_param = NULL;
	struct ts_device_data *chip_data = g_ts_data.chip_data;
	int feature_all = 0;

	TS_LOG_INFO("%s +\n", __func__);
	if (!chip_data){
		TS_LOG_ERR("%s chip_data is null\n", __func__);
		return snprintf(buf, PAGE_SIZE, "error1\n");
	}
	local_param = &(chip_data->anti_false_touch_param_data);
	if(g_ts_data.aft_param.aft_enable_flag)
	{
		feature_all = 0;
	}
	else
	{
		feature_all = local_param->feature_all;
	}

	return snprintf(buf, PAGE_SIZE, "feature_all=%d,"
		"feature_resend_point=%d,"
		"feature_orit_support=%d,"
		"feature_reback_bt=%d,"
		"lcd_width=%d,"
		"lcd_height=%d,"
		"click_time_limit=%d,"
		"click_time_bt=%d,"
		"edge_position=%d,"
		"edge_postion_secondline=%d,"
		"bt_edge_x=%d,"
		"bt_edge_y=%d,"
		"move_limit_x=%d,"
		"move_limit_y=%d,"
		"move_limit_x_t=%d,"
		"move_limit_y_t=%d,"
		"move_limit_x_bt=%d,"
		"move_limit_y_bt=%d,"
		"edge_y_confirm_t=%d,"
		"edge_y_dubious_t=%d,"
		"edge_y_avg_bt=%d,"
		"edge_xy_down_bt=%d,"
		"edge_xy_confirm_t=%d,"
		"max_points_bak_num=%d,"
		"drv_stop_width=%d,"
		"sensor_x_width=%d,"
		"sensor_y_width=%d,"

		/* emui5.1 new support */
		"feature_sg=%d,"
		"sg_min_value=%d,"
		"feature_support_list=%d,"
		"max_distance_dt=%d,"
		"feature_big_data=%d,"
		"feature_click_inhibition=%d,"
		"min_click_time=%d,\n",
		feature_all,
		local_param->feature_resend_point,
		local_param->feature_orit_support,
		local_param->feature_reback_bt,
		local_param->lcd_width,
		local_param->lcd_height,
		local_param->click_time_limit,
		local_param->click_time_bt,
		local_param->edge_position,
		local_param->edge_postion_secondline,
		local_param->bt_edge_x,
		local_param->bt_edge_y,
		local_param->move_limit_x,
		local_param->move_limit_y,
		local_param->move_limit_x_t,
		local_param->move_limit_y_t,
		local_param->move_limit_x_bt,
		local_param->move_limit_y_bt,
		local_param->edge_y_confirm_t,
		local_param->edge_y_dubious_t,
		local_param->edge_y_avg_bt,
		local_param->edge_xy_down_bt,
		local_param->edge_xy_confirm_t,
		local_param->max_points_bak_num,
		local_param->drv_stop_width,
		local_param->sensor_x_width,
		local_param->sensor_y_width,

		/* emui5.1 new support */
		local_param->feature_sg,
		local_param->sg_min_value,
		local_param->feature_support_list,
		local_param->max_distance_dt,
		local_param->feature_big_data,
		local_param->feature_click_inhibition,
		local_param->min_click_time
	);
}

static char *g_anti_false_touch_string[]={
	ANTI_FALSE_TOUCH_FEATURE_ALL,
	ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT,
	ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT,
	ANTI_FALSE_TOUCH_FEATURE_REBACK_BT,
	ANTI_FALSE_TOUCH_LCD_WIDTH,
	ANTI_FALSE_TOUCH_LCD_HEIGHT,
	ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT,
	ANTI_FALSE_TOUCH_CLICK_TIME_BT,
	ANTI_FALSE_TOUCH_EDGE_POISION,
	ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE,
	ANTI_FALSE_TOUCH_BT_EDGE_X,
	ANTI_FALSE_TOUCH_BT_EDGE_Y,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_X,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_Y,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT,
	ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT,
	ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T,
	ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T,
	ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT,
	ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT,
	ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T,
	ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM,
	//for driver
	ANTI_FALSE_TOUCH_DRV_STOP_WIDTH,
	ANTI_FALSE_TOUCH_SENSOR_X_WIDTH,
	ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH,

	/* emui5.1 new support */
	ANTI_FALSE_TOUCH_FEATURE_SG,
	ANTI_FALSE_TOUCH_SG_MIN_VALUE,
	ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST,
	ANTI_FALSE_TOUCH_MAX_DISTANCE_DT,
	ANTI_FALSE_TOUCH_FEATURE_BIG_DATA,
	ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION,
	ANTI_FALSE_TOUCH_MIN_CLICK_TIME,
};

static void set_value_to_param(char *tag, int value){
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct anti_false_touch_param *local_param = NULL;

	if (!tag || !dev){
		TS_LOG_ERR("%s aft get tag/dev null\n", __func__);
		return ;
	}

	local_param = &(dev->anti_false_touch_param_data);

	if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_ALL)){
		local_param->feature_all = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_RESEND_POINT)){
		local_param->feature_resend_point = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_ORIT_SUPPORT)){
		local_param->feature_orit_support = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_REBACK_BT)){
		local_param->feature_reback_bt = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_LCD_WIDTH)){
		local_param->lcd_width = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_LCD_HEIGHT)){
		local_param->lcd_height = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_CLICK_TIME_LIMIT)){
		local_param->click_time_limit = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_CLICK_TIME_BT)){
		local_param->click_time_bt = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_POISION)){
		local_param->edge_position = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_POISION_SECONDLINE)){
		local_param->edge_postion_secondline = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_BT_EDGE_X)){
		local_param->bt_edge_x = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_BT_EDGE_Y)){
		local_param->bt_edge_y = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_X)){
		local_param->move_limit_x = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y)){
		local_param->move_limit_y = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_T)){
		local_param->move_limit_x_t = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_T)){
		local_param->move_limit_y_t = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_X_BT)){
		local_param->move_limit_x_bt = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MOVE_LIMIT_Y_BT)){
		local_param->move_limit_y_bt = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_Y_CONFIRM_T)){
		local_param->edge_y_confirm_t = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_Y_DUBIOUS_T)){
		local_param->edge_y_dubious_t = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_Y_AVG_BT)){
		local_param->edge_y_avg_bt = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_XY_DOWN_BT)){
		local_param->edge_xy_down_bt = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_EDGE_XY_CONFIRM_T)){
		local_param->edge_xy_confirm_t = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MAX_POINTS_BAK_NUM)){
		local_param->max_points_bak_num = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_DRV_STOP_WIDTH)){
		local_param->drv_stop_width = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_SENSOR_X_WIDTH)){
		local_param->sensor_x_width = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_SENSOR_Y_WIDTH)){
		local_param->sensor_y_width = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_SG)){
		local_param->feature_sg = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_SG_MIN_VALUE)){
		local_param->sg_min_value = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_SUPPORT_LIST)){
		local_param->feature_support_list = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MAX_DISTANCE_DT)){
		local_param->max_distance_dt = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_BIG_DATA)){
		local_param->feature_big_data = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_FEATURE_CLICK_INHIBITION)){
		local_param->feature_click_inhibition = value;
	}else if (!strcmp(tag, ANTI_FALSE_TOUCH_MIN_CLICK_TIME)){
		local_param->min_click_time = value;
	}
	TS_LOG_INFO("%s: set %s to %d\n", __func__, tag, value);
}

static int anti_false_touch_get_param(const char *buf, char *tag)
{
	char *ptr_begin = NULL, *ptr_end = NULL;
	char tmp_str[128] = {0};
	char value_str[32] = {0};
	int len;
	int value = 0;
	int error;

	if (!buf || !tag){
		TS_LOG_ERR("misoper get error string\n");
		return -EINVAL;
	}
	TS_LOG_DEBUG("%s:%s\n", buf, tag);
	snprintf(tmp_str, sizeof(tmp_str), "%s=", tag);
	if (NULL != (ptr_begin = strstr(buf, tmp_str))){
		ptr_begin += strlen(tmp_str);
		if (ptr_begin){
			ptr_end = strstr(ptr_begin, ",");
			if (ptr_end){
				len = ptr_end - ptr_begin;
				if (len > 0 && len < (int)sizeof(value_str)){
					TS_LOG_DEBUG("%s: get %s string %s\n", __func__, tag, value_str);
					strncpy(value_str, ptr_begin, len);
					error = sscanf(value_str, "%u", &value);
					if (error <= 0) {
						TS_LOG_ERR("sscanf return invaild :%d\n", error);
						return -EINVAL;
					}
					set_value_to_param(tag, value);
				}
			}
		}
	}
	return 0;
}

static ssize_t ts_anti_false_touch_param_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t count)
{
	int i;
	int str_num = 0;

	TS_LOG_INFO("%s +\n", __func__);

	if (dev == NULL) {
		TS_LOG_ERR("dev is null\n");
		return -EINVAL;
	}
	str_num = sizeof(g_anti_false_touch_string) / sizeof(char *);
	TS_LOG_DEBUG("str_num:%d, input buf is [%s]\n", str_num, buf);
	for(i = 0; i < str_num; i++){
		anti_false_touch_get_param(buf, g_anti_false_touch_string[i]);
	}
	TS_LOG_INFO("%s -\n", __func__);
	return count;
}

static DEVICE_ATTR(touch_oem_info, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
		   ts_oem_info_show, ts_ome_info_store);
static DEVICE_ATTR(touch_chip_info, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
		   ts_chip_info_show, ts_chip_info_store);
static DEVICE_ATTR(calibration_info, (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH),
		ts_calibration_info_show, NULL);
static DEVICE_ATTR(calibrate, (S_IRUSR|S_IRGRP), ts_calibrate_show, NULL);
static DEVICE_ATTR(calibrate_wakeup_gesture, S_IRUSR,
		   ts_calibrate_wakeup_gesture_show, NULL);
static DEVICE_ATTR(touch_glove, (S_IRUSR | S_IWUSR), ts_glove_mode_show,
		   ts_glove_mode_store);
static DEVICE_ATTR(touch_sensitivity, (S_IRUSR | S_IWUSR), ts_sensitivity_show,
		   ts_sensitivity_store);
static DEVICE_ATTR(hand_detect, S_IRUSR, ts_hand_detect_show, NULL);
static DEVICE_ATTR(loglevel, (S_IRUSR | S_IWUSR), ts_loglevel_show,
		   ts_loglevel_store);
static DEVICE_ATTR(supported_func_indicater, (S_IRUSR|S_IRGRP),
		   ts_supported_func_indicater_show, NULL);
static DEVICE_ATTR(touch_window, (S_IRUSR | S_IWUSR), ts_touch_window_show,
		   ts_touch_window_store);
static DEVICE_ATTR(fw_update_sd, S_IWUSR, NULL, ts_fw_update_sd_store);
static DEVICE_ATTR(reset, S_IWUSR, NULL, ts_reset_store);
static DEVICE_ATTR(easy_wakeup_gesture, (S_IRUSR | S_IWUSR),
		   ts_easy_wakeup_gesture_show, ts_easy_wakeup_gesture_store);
static DEVICE_ATTR(wakeup_gesture_enable, (S_IRUSR | S_IWUSR),
		   ts_wakeup_gesture_enable_show,
		   ts_wakeup_gesture_enable_store);
static DEVICE_ATTR(touch_dsm_debug, S_IRUSR | S_IRGRP | S_IROTH,
		   ts_dsm_debug_show, NULL);
static DEVICE_ATTR(easy_wakeup_control, S_IWUSR, NULL,
		   ts_easy_wakeup_control_store);
static DEVICE_ATTR(easy_wakeup_position, S_IRUSR, ts_easy_wakeup_position_show,
		   NULL);
static DEVICE_ATTR(touch_register_operation,
		   S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP, ts_register_show,
		   ts_register_store);
static DEVICE_ATTR(roi_enable, (S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP),
		   ts_roi_enable_show, ts_roi_enable_store);
static DEVICE_ATTR(roi_data, (S_IRUSR | S_IRGRP), ts_roi_data_show, NULL);
static DEVICE_ATTR(roi_data_debug, (S_IRUSR | S_IRGRP), ts_roi_data_debug_show,
		   NULL);
static DEVICE_ATTR(tp_capacitance_test_type, (S_IRUSR | S_IWUSR),
		   ts_capacitance_test_type_show,
		   ts_capacitance_test_type_store);
static DEVICE_ATTR(tp_capacitance_test_config, (S_IRUSR),
		   ts_capacitance_test_config_show, NULL);
#if defined (CONFIG_TEE_TUI)
static DEVICE_ATTR(touch_tui_enable, S_IRUSR | S_IWUSR, ts_tui_report_show,
		   ts_tui_report_store);
#endif
static DEVICE_ATTR(touch_rawdata_debug, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP,
		   ts_rawdata_debug_test_show, ts_rawdata_debug_test_store);
static DEVICE_ATTR(touch_special_hardware_test,
		   S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP,
		   touch_special_hardware_test_show,
		   touch_special_hardware_test_store);
static DEVICE_ATTR(touch_wideth, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP, ts_touch_wideth_show, ts_touch_wideth_store);
static DEVICE_ATTR(anti_false_touch_param, S_IRUSR | S_IRGRP | S_IWUSR | S_IWGRP, ts_anti_false_touch_param_show, ts_anti_false_touch_param_store);

static struct attribute *ts_attributes[] = {
	&dev_attr_touch_oem_info.attr,
	&dev_attr_touch_chip_info.attr,
	&dev_attr_calibration_info.attr,
	&dev_attr_calibrate.attr,
	&dev_attr_calibrate_wakeup_gesture.attr,
	&dev_attr_touch_glove.attr,
	&dev_attr_touch_sensitivity.attr,
	&dev_attr_hand_detect.attr,
	&dev_attr_loglevel.attr,
	&dev_attr_supported_func_indicater.attr,
	&dev_attr_touch_window.attr,
	&dev_attr_fw_update_sd.attr,
	&dev_attr_reset.attr,
	&dev_attr_easy_wakeup_gesture.attr,
	&dev_attr_wakeup_gesture_enable.attr,
	&dev_attr_touch_dsm_debug.attr,
	&dev_attr_easy_wakeup_control.attr,
	&dev_attr_easy_wakeup_position.attr,
	&dev_attr_touch_register_operation.attr,
	&dev_attr_roi_enable.attr,
	&dev_attr_roi_data.attr,
	&dev_attr_roi_data_debug.attr,
	&dev_attr_tp_capacitance_test_type.attr,
	&dev_attr_tp_capacitance_test_config.attr,
#if defined (CONFIG_TEE_TUI)
	&dev_attr_touch_tui_enable.attr,
#endif
	&dev_attr_touch_rawdata_debug.attr,
	&dev_attr_touch_special_hardware_test.attr,
	&dev_attr_touch_wideth.attr,
	&dev_attr_anti_false_touch_param.attr,
	NULL
};

static const struct attribute_group ts_attr_group = {
	.attrs = ts_attributes,
};

static void rawdata_timeout_proc_fn(struct work_struct *work)
{
	if (g_rawdata_timeout_info.info) {
		if ((g_rawdata_timeout_info.info)->status != TS_ACTION_UNDEF) {
			kfree(g_rawdata_timeout_info.info);
			g_rawdata_timeout_info.info = NULL;
			atomic_set(&(g_rawdata_timeout_info.idle_flag),
				   TS_RAWDATA_IDLE);
			TS_LOG_INFO("Rawdata reading is ready\n");
		} else {
			TS_LOG_INFO("Rawdata reading is not ready\n");
			schedule_delayed_work(&g_rawdata_proc_work,
					      msecs_to_jiffies(200));
		}
	}
}

void rotate_rawdata(int row, int column, int *data_start, int rotate_type)
{
	int *rawdatabuf_temp = NULL;
	int row_index, column_index;
	int row_size = 0;
	int column_size = 0;
	int i = 0;

	TS_LOG_INFO("\n");
	rawdatabuf_temp =
	    (int *)kzalloc(row * column * sizeof(int), GFP_KERNEL);
	if (!rawdatabuf_temp) {
		TS_LOG_ERR("Failed to alloc buffer for rawdatabuf_temp\n");
		return;
	}

	memcpy(rawdatabuf_temp, data_start, row * column * sizeof(int));
	switch (rotate_type) {
		case TS_RAWDATA_TRANS_NONE:
			break;
		case TS_RAWDATA_TRANS_ABCD2CBAD:
			/* src column to dst row*/
			row_size = column;
			column_size = row;
			for (column_index = column_size - 1; column_index >= 0; column_index--) {
				for (row_index = row_size - 1; row_index >= 0; row_index--) {
					data_start[i++] =
					    rawdatabuf_temp[row_index * column_size + column_index];
				}
			}
			break;
		case TS_RAWDATA_TRANS_ABCD2ADCB:
			/* src column to dst row*/
			row_size = column;
			column_size = row;
			for (column_index = 0; column_index < column_size; column_index++) {
				for (row_index = 0; row_index < row_size; row_index++) {
					data_start[i++] =
					    rawdatabuf_temp[row_index * column_size +
							    column_index];
				}
			}
			break;
		default:
			break;
	}
	if (rawdatabuf_temp) {
		kfree(rawdatabuf_temp);
		rawdatabuf_temp = NULL;
	}
	return;
}

static int rawdata_proc_show(struct seq_file *m, void *v)
{
	int index;
	int index1;
	int rdIndex;
	short row_size = 0;
	int range_size = 0;
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_rawdata_info *info = NULL;

	TS_LOG_INFO("rawdata_proc_show, buffer size = %ld\n", m->size);
	if (m->size <= RAW_DATA_SIZE) {
		m->count = m->size;
		return 0;
	}

	if (atomic_read(&(g_rawdata_timeout_info.idle_flag)) == TS_RAWDATA_WORK) {
		TS_LOG_INFO("rawdata work busy\n");
		return -EBUSY;
	}
	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info =
	    (struct ts_rawdata_info *)kzalloc(sizeof(struct ts_rawdata_info),
					      GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info->used_size = 0;
	info->used_sharp_selcap_single_ended_delta_size = 0;
	info->used_sharp_selcap_touch_delta_size = 0;
	info->used_synaptics_short_size = 0;
	info->used_synaptics_resistance_size = 0;
	info->used_synaptics_self_cap_size = 0;
	info->status = TS_ACTION_UNDEF;
	cmd->command = TS_READ_RAW_DATA;
	cmd->cmd_param.prv_params = (void *)info;

	if(g_ts_data.chip_data->is_direct_proc_cmd){
		error = ts_proc_command_directly(cmd);
	}
	else{
		if (g_ts_data.chip_data->rawdata_get_timeout)
			error = put_one_cmd(cmd, g_ts_data.chip_data->rawdata_get_timeout);
		else
			error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}

	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		if (error == -EBUSY) {
			g_rawdata_timeout_info.info = info;
			atomic_set(&(g_rawdata_timeout_info.idle_flag),
				   TS_RAWDATA_WORK);
			schedule_delayed_work(&g_rawdata_proc_work,
					      msecs_to_jiffies(200));
		}
		error = -EBUSY;
		goto out;
	}

	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}
	seq_printf(m, "%s\n", info->result);
	seq_printf(m, "*************touch data*************\n");

	if (g_ts_data.chip_data->rawdata_arrange_swap) {
		row_size = info->buff[1];
		range_size = info->buff[0];
	} else {
		row_size = info->buff[0];
		range_size = info->buff[1];
	}

	if (g_ts_data.chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2CBAD
		|| g_ts_data.chip_data->rawdata_arrange_type == TS_RAWDATA_TRANS_ABCD2ADCB) {
		rotate_rawdata(row_size, range_size, info->buff + 2,
				g_ts_data.chip_data->rawdata_arrange_type);
		rotate_rawdata(row_size, range_size,
				info->buff + 2 + row_size * range_size, g_ts_data.chip_data->rawdata_arrange_type);
		row_size = info->buff[1];
		range_size = info->buff[0];
	}

	seq_printf(m, "rx: %d, tx : %d\n", row_size, range_size);
	if(g_ts_data.chip_data->is_parade_solution == 0){//Not Parade Solution, use default
		for (index = 0; row_size * index + 2 < info->used_size; index++) {
			if (0 == index) {
				seq_printf(m, "rawdata begin\n");	/*print the title */
			}
			for (index1 = 0; index1 < row_size; index1++) {
				seq_printf(m, "%d,", info->buff[2 + row_size * index + index1]);	/*print oneline */
			}
			/*index1 = 0;*/
			seq_printf(m, "\n ");

			if ((range_size - 1) == index) {
				seq_printf(m, "rawdata end\n");
				seq_printf(m, "noisedata begin\n");
			}
		}
		seq_printf(m, "noisedata end\n");
		if(info->used_sharp_selcap_single_ended_delta_size) {
			seq_printf(m, "selfcap touchdelta begin\n");
			if((info->used_sharp_selcap_single_ended_delta_size + info->used_sharp_selcap_touch_delta_size +
					info->used_size) >= TS_RAWDATA_BUFF_MAX) {

				TS_LOG_ERR("the info->buff_size is %d was out of memory\n",
					info->used_size + info->used_sharp_selcap_touch_delta_size +
					info->used_sharp_selcap_single_ended_delta_size);

				error = -ENOMEM;
				goto out;
			}
			for (index = 0; index < info->used_sharp_selcap_touch_delta_size; index++) {
				seq_printf(m, "%d,", info->buff[info->used_size + index]);
			}
			seq_printf(m, "\n ");
			seq_printf(m, "selfcap touchdelta end\n");
			seq_printf(m, "selfcap singleenddelta begin\n");

			for (index = 0; index < info->used_sharp_selcap_single_ended_delta_size; index++) {
				seq_printf(m, "%d,", info->buff[info->used_size +
						info->used_sharp_selcap_touch_delta_size + index]);
			}
			seq_printf(m, "\n ");
			seq_printf(m, "selfcap singleenddelta end\n");
		}
		if((info->used_synaptics_resistance_size + info->used_synaptics_short_size +
				info->used_size) >= TS_RAWDATA_BUFF_MAX) {

			TS_LOG_ERR("the info->buff_size is %d was out of memory\n",
				info->used_size + info->used_sharp_selcap_touch_delta_size
				+ info->used_sharp_selcap_single_ended_delta_size);

			error = -ENOMEM;
			goto out;
		}

		if(info->used_synaptics_short_size) {
			seq_printf(m, "short_test_begin\n");
			for(index = 0; index < info->used_synaptics_short_size; index++) {
				seq_printf(m, "%d,", info->buff[info->used_size + index]);
			}
			seq_printf(m, "\n");
			seq_printf(m, "short_test_end\n");
		}
		if(info->used_synaptics_resistance_size) {
			seq_printf(m, "resistance_test_begin\n");
			for(index = 0; index < info->used_synaptics_resistance_size; index++) {
				seq_printf(m, "%d,", info->buff[info->used_size + info->used_synaptics_short_size + index]);
			}
			seq_printf(m, "\n");
			seq_printf(m, "resistance_test_end\n");
		}

		if(info->used_synaptics_self_cap_size >= TS_HYBRID_BUFF_MAX) {
			TS_LOG_ERR("the info->hybrid_buff_size is %d was out of memory\n", info->used_synaptics_self_cap_size);
			error = -ENOMEM;
			goto out;
		}

		if(info->used_synaptics_self_cap_size) {
			seq_printf(m, "self_delta_test_begin\n");
			for(index = 0; index < (info->used_synaptics_self_cap_size / 2); index++) {
				seq_printf(m, "%d,", info->hybrid_buff[index]);
			}
			seq_printf(m, "\n");
			seq_printf(m, "self_delta_test_end\n");
			seq_printf(m, "self_cap_test_begin\n");
			for(index = info->used_synaptics_self_cap_size/2; index < info->used_synaptics_self_cap_size; index++) {
				seq_printf(m, "%d,", info->hybrid_buff[index]);
			}
			seq_printf(m, "\n");
			seq_printf(m, "self_cap_test_end\n");
		}
	} else {
		rdIndex = 2;
		seq_printf(m, "cm data begin\n");	/*print the title */
		for(index = 0; index < range_size; index++){
			for (index1 = 0; index1 < row_size; index1++){
				if(rdIndex < info->used_size)
					seq_printf(m, "%d,", info->buff[rdIndex++]);
				else{
					seq_printf(m, "\n ");
					goto out;
				}
			}
			seq_printf(m, "\n ");
		}
		seq_printf(m, "cm data end\n");	/*print the title */
		seq_printf(m, "mutual noise data begin\n");	/*print the title */
		for(index = 0; index < range_size; index++){
			for (index1 = 0; index1 < row_size; index1++){
				if(rdIndex < info->used_size)
					seq_printf(m, "%d,", info->buff[rdIndex++]);
				else{
					seq_printf(m, "\n ");
					goto out;
				}
			}
			seq_printf(m, "\n ");
		}
		seq_printf(m, "mutual noise data end\n");	/*print the title */
		seq_printf(m, "self noise data begin\n");	/*print the title */
		seq_printf(m, "-rx:,");
		for (index1 = 0; index1 < row_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "-tx:,");
		for (index1 = 0; index1 < range_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "self noise data end\n");	/*print the title */
		seq_printf(m, "cm gradient(10x real value) begin\n");	/*print the title */
		seq_printf(m, "-rx:,");
		for (index1 = 0; index1 < row_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "-tx:,");
		for (index1 = 0; index1 < range_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "cm gradient end\n");	/*print the title */
		seq_printf(m, "cp begin\n");	/*print the title */
		seq_printf(m, "-rx:,");
		for (index1 = 0; index1 < row_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "-tx:,");
		for (index1 = 0; index1 < range_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "cp end\n");	/*print the title */
		seq_printf(m, "cp delta begin\n");	/*print the title */
		seq_printf(m, "-rx:,");
		for (index1 = 0; index1 < row_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "-tx:,");
		for (index1 = 0; index1 < range_size; index1++){
			if(rdIndex < info->used_size)
				seq_printf(m, "%d,", info->buff[rdIndex++]);
			else{
				seq_printf(m, "\n ");
				goto out;
			}
		}
		seq_printf(m, "\n ");
		seq_printf(m, "cp detlat end\n");	/*print the title */
OUT:
		seq_printf(m, "*************end data*************\n");
	}

	if (g_ts_data.chip_data->support_3d_func) {
		TS_LOG_INFO("print 3d data\n");
		row_size = info->buff_3d[0];
		range_size = info->buff_3d[1];
		seq_printf(m, "rx: %d, tx : %d(3d)\n", row_size, range_size);

		for (index = 0; row_size * index + 2 < info->used_size_3d;
		     index++) {
			if (0 == index) {
				seq_printf(m, "rawdata begin(3d)\n");	/*print the title */
			}
			for (index1 = 0; index1 < row_size; index1++) {
				seq_printf(m, "%d,", info->buff_3d[2 + row_size * index + index1]);	/*print oneline */
			}
			/*index1 = 0;*/
			seq_printf(m, "\n ");

			if ((range_size - 1) == index) {
				seq_printf(m, "rawdata end(3d)\n");
				seq_printf(m, "noisedata begin(3d)\n");
			}
		}
		seq_printf(m, "noisedata end(3d)\n");
	}
	error = NO_ERR;

out:

	if (!
	    (atomic_read(&(g_rawdata_timeout_info.idle_flag)) ==
	     TS_RAWDATA_WORK)) {
		if(info){
			TS_LOG_INFO("rawdata_proc_show done:status=%d, result: %s\n",
			    error, info->result);
			kfree(info);
			info = NULL;
			g_rawdata_timeout_info.info = NULL;
		}
	} else {
		TS_LOG_ERR("rawdata_proc_show done:status=%d, timeout error!\n",
			   error);
	}
	if (cmd){
		kfree(cmd);
		cmd = NULL;
	}

	return error;
}

static int rawdata_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, rawdata_proc_show, NULL);
}

static const struct file_operations rawdata_proc_fops = {
	.open = rawdata_proc_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

static int seq_print_freq(struct seq_file *m, char *buf, int tx_num, int rx_num)
{
	char ii, jj;
	unsigned char *head;
	unsigned char *report_data_8;

	head = (unsigned char*)buf;

	seq_printf(m, "Calibration Image - Coarse & Fine\n");
	report_data_8 = (unsigned char*)buf;
	report_data_8++;	//point to second byte of F54 report data
	for (ii = 0; ii < rx_num; ii++)
	{
		for (jj = 0; jj < tx_num; jj++)
		{
			seq_printf(m, "%02x ", *report_data_8);

			report_data_8 +=2;

		}
		seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Image - Detail\n");
	report_data_8 = head;
	for (ii = 0; ii < rx_num; ii++)
	{
		for (jj = 0; jj < tx_num; jj++)
		{
			seq_printf(m, "%02x ", *report_data_8);

			report_data_8 +=2;

		}
		seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Noise - Coarse & Fine\n");
	report_data_8 = (unsigned char*)buf;	//point to first byte of data
	report_data_8 += (tx_num * rx_num *2 + 1);
	for (ii = 0; ii < rx_num * 2 ; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;

		if ((ii+1) % tx_num == 0 )
			seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration Noise - Detail\n");
	report_data_8 = buf;
	report_data_8 += (tx_num * rx_num *2);
	for (ii = 0; ii < rx_num * 2; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;

		if ((ii+1) % tx_num == 0 )
			seq_printf(m, "\n");
	}

	seq_printf(m, "\nCalibration button - Coarse & Fine\n");
	report_data_8 = (unsigned char*)buf;
	report_data_8 += (tx_num * rx_num *2 + rx_num *4 + 1);
	for (ii = 0; ii < 4; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;
	}

	seq_printf(m, "\nCalibration button - Detail\n");
	report_data_8 = (unsigned char*)buf;
	report_data_8 += (tx_num * rx_num *2 + rx_num *4);
	for (ii = 0; ii < 4; ii++)
	{
		seq_printf(m, "%02x ", *report_data_8);
		report_data_8 +=2;
	}

	return 0;
}

static int calibration_proc_show(struct seq_file *m, void *v)
{
	struct ts_calibration_data_info *info = NULL;
	struct ts_cmd_node *cmd = NULL;
	int error = NO_ERR;

	if (!g_ts_data.chip_data->should_check_tp_calibration_info) {
		TS_LOG_ERR("No calibration data.\n");
		error = NO_ERR;
		goto out;
	}

	info = (struct ts_calibration_data_info *)kzalloc(sizeof(struct
				ts_calibration_data_info), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node), GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out_free_info;
	}

	cmd->command = TS_READ_CALIBRATION_DATA;
	cmd->cmd_param.prv_params = (void *)info;

	if( g_ts_data.chip_data->is_direct_proc_cmd) {
		error = ts_proc_command_directly(cmd);
	}else{
		error = put_one_cmd(cmd, SHORT_SYNC_TIMEOUT);
	}
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out_free_cmd;
	}

	seq_write(m, info->data, info->used_size);

	seq_printf(m, "\n\nCollect data for freq: 0\n\n");
	seq_print_freq(m, info->data, info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for freq: 1\n\n");
	seq_print_freq(m,
			info->data
			+ info->tx_num * info->rx_num * 2	/* shift 2D */
			+ info->rx_num * 2 * 2	/* shift noise */
			+ 4 * 2,		/* shift 0D */
			info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for freq: 2\n\n");
	seq_print_freq(m,
			info->data
			+ (info->tx_num * info->rx_num * 2	/* shift 2D */
			+ info->rx_num * 2 * 2	/* shift noise */
			+ 4 * 2) * 2,		/* shift 0D */
			info->tx_num, info->rx_num);

	seq_printf(m, "\n\nCollect data for interval scan\n\n");
	seq_print_freq(m, info->data+2048, info->tx_num, info->rx_num);

out_free_cmd:
	kfree(cmd);
	cmd = NULL;
out_free_info:
	kfree(info);
out:
	return 0;
}

static int calibration_proc_open(struct inode *inode, struct file *file)
{
	return single_open(file, calibration_proc_show, NULL);
}

static const struct file_operations calibration_proc_fops = {
	.open		= calibration_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

static void procfs_create(void)
{
	if (!proc_mkdir("touchscreen", NULL))
		return;
	proc_create("touchscreen/tp_capacitance_data",
		    S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH, NULL,
		    &rawdata_proc_fops);
	proc_create("touchscreen/tp_calibration_data", S_IRUSR | S_IWUSR |
			S_IRGRP | S_IROTH, NULL, &calibration_proc_fops);
	return;
}

int ts_power_control_notify(enum ts_pm_type pm_type, int timeout)
{
	int error;
	struct ts_cmd_node cmd;

	if (TS_UNINIT == atomic_read(&g_ts_data.state)) {
		TS_LOG_INFO("ts is not init");
		return -EINVAL;
	}

#if defined (CONFIG_TEE_TUI)
	if (g_ts_data.chip_data->report_tui_enable
	    && TS_BEFORE_SUSPEND == pm_type) {
		g_ts_data.chip_data->tui_set_flag |= 0x1;
		TS_LOG_INFO("TUI is working, later do before suspend\n");
		return NO_ERR;
	}

	if (g_ts_data.chip_data->report_tui_enable
	    && TS_SUSPEND_DEVICE == pm_type) {
		g_ts_data.chip_data->tui_set_flag |= 0x2;
		TS_LOG_INFO("TUI is working, later do suspend\n");
		return NO_ERR;
	}
#endif

	cmd.command = TS_POWER_CONTROL;
	cmd.cmd_param.pub_params.pm_type = pm_type;
	error = put_one_cmd(&cmd, timeout);
	if (error) {
		TS_LOG_ERR("ts_power_control_notify, put cmd error :%d\n",
			   error);
		error = -EBUSY;
	}
	if (TS_AFTER_RESUME == pm_type) {
		TS_LOG_INFO("ts_resume_send_roi_cmd\n");
		if(g_ts_data.chip_data->is_direct_proc_cmd == 0)
			ts_send_roi_cmd(TS_ACTION_WRITE, NO_SYNC_TIMEOUT);	/*force to write the roi function */
		if (error) {
			TS_LOG_ERR("ts_resume_send_roi_cmd failed\n");
		}
	}
	return error;
}

void ts_thread_stop_notify(void)
{
	struct ts_device_data *dev = g_ts_data.chip_data;

	TS_LOG_INFO("ts thread stop called by lcd only shutdown\n");
	if (TS_UNINIT == atomic_read(&g_ts_data.state)) {
		TS_LOG_INFO("ts is not init");
		return;
	}

	atomic_set(&g_ts_data.state, TS_UNINIT);
	disable_irq(g_ts_data.irq_id);
	ts_stop_wd_timer(&g_ts_data);
	if (dev->ops->chip_shutdown)
		dev->ops->chip_shutdown();
	/*there is something wrong about system, now abandon the kthread_stop to avoid unkown bug */
	/*kthread_stop(g_ts_data.ts_task);*/
}

#if defined(HUAWEI_CHARGER_FB)
void ts_charger_switch(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_charger_info *info =
	    (struct ts_charger_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_DEBUG("%s, action :%d, value:%d\n", __func__, info->op_action,
		     info->charger_switch);

	if (dev->ops->chip_charger_switch)
		error = dev->ops->chip_charger_switch(info);

	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("%s process result: %d\n", __func__, error);
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_DEBUG("%s process result: %d\n", __func__, error);

	return;
}

static int ts_charger_detect_cmd(enum hisi_charger_type charger_type)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_charger_info *info = NULL;

	TS_LOG_INFO
	    ("%s called, charger type: %d, [0 in, other out], supported: %d\n",
	     __func__, charger_type,
	     g_ts_data.feature_info.charger_info.charger_supported);

	if (g_ts_data.feature_info.charger_info.charger_supported == 0) {
		TS_LOG_DEBUG("%s, do nothing cause charger_supported is zero\n",
			     __func__);
		goto out;
	}

	info = &g_ts_data.feature_info.charger_info;
	info->op_action = TS_ACTION_WRITE;
	if (CHARGER_TYPE_NONE == charger_type) {	/*usb plug out*/
		if (info->charger_switch == 0) {
			TS_LOG_ERR
			    ("%s, there is no need to send cmd repeated\n",
			     __func__);
			error = -EINVAL;
			goto out;
		}
		info->charger_switch = 0;
	} else {		/*usb plug in*/
		if (info->charger_switch == 1) {
			TS_LOG_ERR("%s, there is no need to send repeated\n",
				   __func__);
			error = -EINVAL;
			goto out;
		}
		info->charger_switch = 1;
	}

	if (TS_WORK != atomic_read(&g_ts_data.state)) {
		TS_LOG_ERR
		    ("%s, can not send cmd when TP is not working in normal mode\n",
		     __func__);
		error = -EINVAL;
		goto out;
	}

	cmd =
	    (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node),
					  GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}
	cmd->command = TS_CHARGER_SWITCH;
	cmd->cmd_param.prv_params = (void *)info;
	if (NO_ERR != put_one_cmd(cmd, NO_SYNC_TIMEOUT)) {
		TS_LOG_ERR("%s, put_one_cmd failed\n", __func__);
		error = -EINVAL;
		goto out;
	}

out:
	if (cmd != NULL)
		kfree(cmd);
	return error;
}

static int charger_detect_notifier_callback(struct notifier_block *self,
					    unsigned long event, void *data)
{
	TS_LOG_INFO
	    ("%s, charger type:%ld, [0 in, other out]. charger switch supported: %d\n",
	     __func__, event,
	     g_ts_data.feature_info.charger_info.charger_supported);

	if (g_ts_data.feature_info.charger_info.charger_supported != 0) {
		ts_charger_detect_cmd(event);
	}
	return NO_ERR;
}
#endif

#if defined(CONFIG_FB)
static int fb_notifier_callback(struct notifier_block *self,
				unsigned long event, void *data)
{
	int error = NO_ERR;
	int i;
	struct fb_event *fb_event = data;
	int *blank = fb_event->data;
	unsigned char ts_state = 0;
	int times = 0;

	if (g_lcd_control_tp_power)
		return NO_ERR;

	/* only need process event  FB_EARLY_EVENT_BLANK\FB_EVENT_BLANK  */
	if (!(event == FB_EARLY_EVENT_BLANK || event == FB_EVENT_BLANK)) {
		TS_LOG_DEBUG("event(%d) do not need process\n", event);
		return NO_ERR;
	}

	for (i = 0; i < FB_MAX; i++) {
		if (registered_fb[i] == fb_event->info) {
			if (i == 0) {
				TS_LOG_DEBUG("Ts-index:%d, go on !\n", i);
				break;
			} else {
				TS_LOG_INFO("Ts-index:%d, exit !\n", i);
				return error;
			}
		}
	}
	TS_LOG_INFO("fb_notifier_callback, blank: %d, event:%lu, state: %d\n",
		    *blank, event, atomic_read(&g_ts_data.state));
	switch (*blank) {
	case FB_BLANK_UNBLANK:
		/*resume device */
		switch (event) {

		case FB_EARLY_EVENT_BLANK:
			TS_LOG_DEBUG("resume: event = %lu, not care\n", event);
			break;
		case FB_EVENT_BLANK:
			while (1) {
				ts_state = atomic_read(&g_ts_data.state);
				if ((TS_SLEEP == ts_state)
				    || (TS_WORK_IN_SLEEP == ts_state)) {
					error = ts_power_control_notify(TS_RESUME_DEVICE, SHORT_SYNC_TIMEOUT);	/*touch power on */
					if (error)
						TS_LOG_ERR
						    ("ts resume device err\n");
					error = ts_power_control_notify(TS_AFTER_RESUME, NO_SYNC_TIMEOUT);	/*enable irq */
					if (error)
						TS_LOG_ERR
						    ("ts after resume err\n");
					break;
				} else {
					msleep(TS_FB_WAIT_TIME);
					if (times++ > TS_FB_LOOP_COUNTS) {
						times = 0;
						TS_LOG_ERR
						    ("no resume, blank: %d, event:%lu, state: %d\n",
						     *blank, event, ts_state);
						break;
					}
				}
			}
			break;
		}
		break;
	case FB_BLANK_VSYNC_SUSPEND:
	case FB_BLANK_HSYNC_SUSPEND:
	case FB_BLANK_NORMAL:
	case FB_BLANK_POWERDOWN:
	default:
		/*suspend device */
		switch (event) {
		case FB_EARLY_EVENT_BLANK:
			while (1) {
				ts_state = atomic_read(&g_ts_data.state);
				if ((TS_WORK == ts_state)
				    || (TS_WORK_IN_SLEEP == ts_state)) {
					error = ts_power_control_notify(TS_BEFORE_SUSPEND, SHORT_SYNC_TIMEOUT);	/*disable irq */
					if (error)
						TS_LOG_ERR
						    ("ts suspend device err\n");
					break;
				} else {
					msleep(TS_FB_WAIT_TIME);
					if (times++ > TS_FB_LOOP_COUNTS) {
						times = 0;
						TS_LOG_ERR
						    ("no early suspend, blank: %d, event:%lu, state: %d\n",
						     *blank, event, ts_state);
						break;
					}
				}
			}
			break;
		case FB_EVENT_BLANK:
			while (1) {
				ts_state = atomic_read(&g_ts_data.state);
				if ((TS_WORK == ts_state)
				    || (TS_WORK_IN_SLEEP == ts_state)) {
					error = ts_power_control_notify(TS_SUSPEND_DEVICE, NO_SYNC_TIMEOUT);	/*touch power off */
					if (error)
						TS_LOG_ERR
						    ("ts before suspend err\n");
					break;
				} else {
					msleep(TS_FB_WAIT_TIME);
					if (times++ > TS_FB_LOOP_COUNTS) {
						times = 0;
						TS_LOG_ERR
						    ("no suspend, blank: %d, event:%lu, state: %d\n",
						     *blank, event, ts_state);
						break;
					}
				}
			}
			break;
		default:
			TS_LOG_DEBUG("suspend: event = %lu, not care\n", event);
			break;
		}
		break;
	}

	return NO_ERR;
}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
static void ts_early_suspend(struct early_suspend *h)
{
	bool is_in_cell = g_ts_data.chip_data->is_in_cell;
	int error = NO_ERR;

	TS_LOG_INFO("ts early suspend, %s\n",
		    (is_in_cell == false) ? "need suspend" : "no need suspend");

	/*for the in-cell, ts_suspend_notify called by lcd, not here */
	if (false == is_in_cell) {
		error =
		    ts_power_control_notify(TS_BEFORE_SUSPEND,
					    SHORT_SYNC_TIMEOUT);
		if (error)
			TS_LOG_ERR("ts before suspend err\n");
		error =
		    ts_power_control_notify(TS_SUSPEND_DEVICE,
					    SHORT_SYNC_TIMEOUT);
		if (error)
			TS_LOG_ERR("ts suspend device err\n");
	}

	TS_LOG_INFO("ts_early_suspend done\n");
}

static void ts_late_resume(struct early_suspend *h)
{
	bool is_in_cell = g_ts_data.chip_data->is_in_cell;
	int error = NO_ERR;
	TS_LOG_INFO("ts late resume, %s\n",
		    (is_in_cell == false) ? "need resume" : "no need resume");

	/*for the in-cell, ts_resume_notify called by lcd, not here */
	if (false == is_in_cell) {
		error =
		    ts_power_control_notify(TS_RESUME_DEVICE,
					    SHORT_SYNC_TIMEOUT);
		if (error)
			TS_LOG_ERR("ts resume device err\n");
		error =
		    ts_power_control_notify(TS_AFTER_RESUME,
					    SHORT_SYNC_TIMEOUT);
		if (error)
			TS_LOG_ERR("ts after resume err\n");
	}
	if(g_ts_data.chip_data->is_direct_proc_cmd == 0)
		ts_send_holster_cmd();

	TS_LOG_INFO("ts_late_resume done\n");
}
#endif

static irqreturn_t ts_irq_handler(int irq, void *dev_id)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;

	wake_lock_timeout(&g_ts_data.ts_wake_lock, HZ);

	if (g_ts_data.chip_data->ops->chip_irq_top_half)
		error = g_ts_data.chip_data->ops->chip_irq_top_half(&cmd);

	if (error)		/*unexpected error happen, put err cmd to ts thread*/
		cmd.command = TS_INT_ERR_OCCUR;
	else
		cmd.command = TS_INT_PROCESS;

	disable_irq_nosync(g_ts_data.irq_id);

	if (put_one_cmd(&cmd, NO_SYNC_TIMEOUT)
	    && (TS_UNINIT != atomic_read(&g_ts_data.state)))
		enable_irq(g_ts_data.irq_id);

	return IRQ_HANDLED;
}

static inline void ts_proc_bottom_half(struct ts_cmd_node *in_cmd,
				       struct ts_cmd_node *out_cmd)
{
	struct ts_device_data *dev = g_ts_data.chip_data;

	TS_LOG_DEBUG("bottom half called\n");

	atomic_set(&g_data_report_over, 0);
	/*related event need process, use out cmd to notify*/
	if (dev->ops->chip_irq_bottom_half)
		dev->ops->chip_irq_bottom_half(in_cmd, out_cmd);
}

static void ts_watchdog_work(struct work_struct *work)
{
	int error = NO_ERR;
	struct ts_cmd_node cmd;

	TS_LOG_DEBUG("ts_watchdog_work\n");
	cmd.command = TS_CHECK_STATUS;

	error = put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put TS_CHECK_STATUS cmd error :%d\n", error);
	}
	return;
}

static void ts_watchdog_timer(unsigned long data)
{
	struct ts_data *cd = (struct ts_data *)data;

	TS_LOG_DEBUG("Timer triggered\n");

	if (!cd)
		return;

	if (!work_pending(&cd->watchdog_work))
		schedule_work(&cd->watchdog_work);

	return;
}

static void ts_start_wd_timer(struct ts_data *cd)
{
	if (!cd->chip_data->need_wd_check_status)
		return;

	if (!TS_WATCHDOG_TIMEOUT)
		return;

	TS_LOG_DEBUG("start wd\n");
	if (cd->chip_data->check_status_watchdog_timeout){
		mod_timer(&cd->watchdog_timer, jiffies +
			  msecs_to_jiffies(cd->chip_data->check_status_watchdog_timeout));
	}else{
		mod_timer(&cd->watchdog_timer, jiffies +
			  msecs_to_jiffies(TS_WATCHDOG_TIMEOUT));
	}
	return;
}

static void ts_stop_wd_timer(struct ts_data *cd)
{
	if (!cd->chip_data->need_wd_check_status)
		return;

	if (!TS_WATCHDOG_TIMEOUT)
		return;

	TS_LOG_DEBUG("stop wd\n");
	del_timer(&cd->watchdog_timer);
	cancel_work_sync(&cd->watchdog_work);
	del_timer(&cd->watchdog_timer);
	return;
}

static inline void ts_algo_calibrate(struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd)
{
	int id;
	int algo_size = g_ts_data.chip_data->algo_size;
	u32 order = in_cmd->cmd_param.pub_params.algo_param.algo_order;
	struct ts_fingers *in_finger =
	    &in_cmd->cmd_param.pub_params.algo_param.info;
	struct ts_fingers *out_finger =
	    &out_cmd->cmd_param.pub_params.algo_param.info;
	struct ts_algo_func *algo;

	if (!algo_size) {
		TS_LOG_INFO("no algo handler, direct report\n");
		goto out;
	}

	TS_LOG_DEBUG("algo order: %d, algo_size :%d\n", order, algo_size);

	for (id = 0; id < algo_size; id++) {
		if (order & BIT_MASK(id)) {
			TS_LOG_DEBUG("algo id:%d is setted\n", id);
			list_for_each_entry(algo,
					    &g_ts_data.chip_data->algo_head,
					    node) {
				if (algo->algo_index == id) {	/*found the right algo func*/
					TS_LOG_DEBUG("algo :%s called\n",
						     algo->algo_name);
					algo->chip_algo_func(g_ts_data.
							     chip_data,
							     in_finger,
							     out_finger);
					memcpy(in_finger, out_finger,
					       sizeof(struct ts_fingers));
					break;
				}
			}
		}
	}

out:
	memcpy(&out_cmd->cmd_param.pub_params.report_info, in_finger,
	       sizeof(struct ts_fingers));
	out_cmd->command = TS_REPORT_INPUT;
	if(g_ts_data.aft_param.aft_enable_flag)
	{
		if(atomic_read(&g_ts_data.fingers_waitq_flag) == AFT_WAITQ_WAIT)
		{
			memcpy(&g_ts_data.fingers_send_aft_info, &out_cmd->cmd_param.pub_params.report_info, sizeof(struct ts_fingers));
			atomic_set(&g_ts_data.fingers_waitq_flag, AFT_WAITQ_WAKEUP);
			up(&g_ts_data.fingers_aft_send);
			//TS_LOG_ERR("[MUTI_AFT] wake_up_interruptible  fingers_waitq!\n");
			ts_work_after_input();  /* do some delayed works */
			out_cmd->command = TS_INVAILD_CMD;
		}
		else if(atomic_read(&g_ts_data.fingers_waitq_flag)  != AFT_WAITQ_IDLE)
		{
			//atomic_set(&g_ts_data.fingers_waitq_flag, AFT_WAITQ_IGNORE);
			up(&g_ts_data.fingers_aft_send);
			TS_LOG_ERR("[MUTI_AFT] ts_algo_calibrate hal aglo process too slow \n");
		}
		else
		{
	            TS_LOG_ERR("[MUTI_AFT] ts_algo_calibrate no wakeup \n");
		}
	}
	return;
}

/**
 * ts_film_touchplus()
 *
 * Called by ts_report_input()
 *
 * touchplus(LingXiYiZhi) report key in this function
 */
static void ts_film_touchplus(struct ts_fingers *finger, int finger_num,
			      struct input_dev *input_dev)
{
	static int pre_special_button_key = TS_TOUCHPLUS_INVALID;
	int key_max = TS_TOUCHPLUS_KEY2;
	int key_min = TS_TOUCHPLUS_KEY3;
	unsigned char ts_state = 0;

	TS_LOG_DEBUG("ts_film_touchplus called\n");

	/*discard touchplus report in gesture wakeup mode */
	ts_state = atomic_read(&g_ts_data.state);
	if ((TS_SLEEP == ts_state) || (TS_WORK_IN_SLEEP == ts_state)) {
		return;
	}

	/*touchplus(LingXiYiZhi) report ,  The difference between ABS_report and touchpls key_report
	 *when ABS_report is running, touchpls key will not report
	 *when touchpls key is not in range of touchpls keys, will not report key
	 */
	if ((finger_num != 0) || (finger->special_button_key > key_max)
	    || (finger->special_button_key < key_min)) {
		if (finger->special_button_flag != 0) {
			input_report_key(input_dev, finger->special_button_key,
					 0);
			input_sync(input_dev);
		}
		return;
	}

	/*touchplus(LingXiYiZhi) report ,  store touchpls key data(finger->special_button_key)
	 *when special_button_flag report touchpls key DOWN , store current touchpls key
	 *till the key report UP, then other keys will not report
	 */
	if (finger->special_button_flag == 1) {
		input_report_key(input_dev, finger->special_button_key,
				 finger->special_button_flag);
		input_sync(input_dev);
	} else if ((finger->special_button_flag == 0)
		   && (pre_special_button_key == finger->special_button_key)) {
		input_report_key(input_dev, finger->special_button_key,
				 finger->special_button_flag);
		input_sync(input_dev);
	} else if ((finger->special_button_flag == 0)
		   && (pre_special_button_key != finger->special_button_key)) {
		input_report_key(input_dev, pre_special_button_key, 0);
		input_sync(input_dev);
	}
	pre_special_button_key = finger->special_button_key;

	return;
}

static void ts_work_after_input(void)
{
	struct ts_device_data *dev = g_ts_data.chip_data;

	if (dev->ops->chip_work_after_input)
		dev->ops->chip_work_after_input();
}

static inline void ts_report_input(struct ts_cmd_node *in_cmd,
				   struct ts_cmd_node *out_cmd)
{
	struct ts_fingers *finger = &in_cmd->cmd_param.pub_params.report_info;
	struct input_dev *input_dev = g_ts_data.input_dev;
	struct anti_false_touch_param *local_param = NULL;
	int finger_num = 0;
	int id;
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	struct aft_abs_param_major aft_abs_major;
	int major = 0;
	int minor = 0;
#else
	int x_y_distance = 0;
	short tmp_distance = 0;
	char *p;
#endif

	if (g_ts_data.chip_data){
		local_param = &(g_ts_data.chip_data->anti_false_touch_param_data);
	}else{
		local_param = NULL;
	}

	TS_LOG_DEBUG("ts_report_input\n");
	ts_check_touch_window(finger);

	for (id = 0; id < TS_MAX_FINGER; id++) {
		if (finger->fingers[id].status == 0) {
			TS_LOG_DEBUG("never touch before: id is %d\n", id);
			continue;
		}
		if (finger->fingers[id].status == TS_FINGER_PRESS) {
			TS_LOG_DEBUG
			    ("down: id is %d, finger->fingers[id].pressure = %d, finger->fingers[id].x = %d, finger->fingers[id].y = %d\n",
			     id, finger->fingers[id].pressure,
			     finger->fingers[id].x, finger->fingers[id].y);
			finger_num++;
			input_report_abs(input_dev, ABS_MT_PRESSURE,
					 finger->fingers[id].pressure);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					 finger->fingers[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					 finger->fingers[id].y);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
			if (local_param && local_param->feature_all){
				if (local_param->sensor_x_width && local_param->sensor_y_width){
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
					if ((finger->fingers[id].major || finger->fingers[id].minor)
						&& (!g_ts_data.feature_info.holster_info.holster_switch)){
						major = 0; minor = 1;
						memset(&aft_abs_major, 0, sizeof(struct aft_abs_param_major));
						aft_abs_major.edgex = finger->fingers[id].major * local_param->sensor_x_width;
						aft_abs_major.edgey = finger->fingers[id].minor * local_param->sensor_y_width;
						aft_abs_major.orientation = finger->fingers[id].orientation;
						aft_abs_major.version = 0x01;
						memcpy(&major, &aft_abs_major, sizeof(int));
						input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, major);
						input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, minor);
					}else{
						major = 0; minor = 0;
						input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, major);
						input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, minor);
					}
#else
					x_y_distance = 0;
					p = (char *)&x_y_distance;
					tmp_distance = finger->fingers[id].major * local_param->sensor_x_width;
					memcpy(p, (char *)&tmp_distance, sizeof(short));
					tmp_distance = finger->fingers[id].minor * local_param->sensor_y_width;
					memcpy(p+sizeof(short), (char *)&tmp_distance, sizeof(short));
					input_report_abs(input_dev, ABS_MT_DISTANCE, x_y_distance);
#endif
				}else{
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
					input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, 0);
					input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, 0);
#else
					input_report_abs(input_dev, ABS_MT_DISTANCE, 0);
#endif
				}
			}
			input_mt_sync(input_dev);       /*modfiy by mengkun*/
		} else if (finger->fingers[id].status == TS_FINGER_RELEASE) {
			TS_LOG_DEBUG("up: id is %d, status = %d\n", id,
				     finger->fingers[id].status);
			input_mt_sync(input_dev);	/*modfiy by mengkun*/
		}
	}

	input_report_key(input_dev, BTN_TOUCH, finger_num);
	input_sync(input_dev);

	ts_film_touchplus(finger, finger_num, input_dev);
	if (((g_ts_data.chip_data->easy_wakeup_info.sleep_mode ==
	      TS_GESTURE_MODE)
	     || (g_ts_data.chip_data->easy_wakeup_info.palm_cover_flag == true))
	    && (g_ts_data.feature_info.holster_info.holster_switch == 0)) {
		input_report_key(input_dev, finger->gesture_wakeup_value, 1);
		input_sync(input_dev);
		input_report_key(input_dev, finger->gesture_wakeup_value, 0);
		input_sync(input_dev);
	}
	TS_LOG_DEBUG("ts_report_input done, finger_num = %d\n", finger_num);

	ts_work_after_input();  /* do some delayed works */

	atomic_set(&g_data_report_over, 1);
	return;
}

void send_up_msg_in_resume(void)
{
	struct input_dev *input_dev = g_ts_data.input_dev;

	input_report_key(input_dev, BTN_TOUCH, 0);
	input_mt_sync(input_dev);
	input_sync(input_dev);
	TS_LOG_DEBUG("send_up_msg_in_resume\n");
	return;
}

static int ts_power_control(int irq_id,
			    struct ts_cmd_node *in_cmd,
			    struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	enum ts_pm_type pm_type = in_cmd->cmd_param.pub_params.pm_type;
	struct ts_device_data *dev = g_ts_data.chip_data;

	if (g_ts_data.chip_data->easy_wakeup_info.sleep_mode ==
	    TS_POWER_OFF_MODE) {
		switch (pm_type) {
		case TS_BEFORE_SUSPEND:	/*do something before suspend */
			ts_stop_wd_timer(&g_ts_data);
			disable_irq(irq_id);
			if (dev->ops->chip_before_suspend)
				error = dev->ops->chip_before_suspend();
			break;
		case TS_SUSPEND_DEVICE:	/*device power off or sleep */
			atomic_set(&g_ts_data.state, TS_SLEEP);
			if(g_ts_data.aft_param.aft_enable_flag)
			{
				TS_LOG_INFO("ts_kit aft suspend\n");
				kobject_uevent(&g_ts_data.input_dev->dev.kobj,
					KOBJ_OFFLINE);
			}
			if (dev->ops->chip_suspend)
				error = dev->ops->chip_suspend();
			break;
		case TS_IC_SHUT_DOWN:
			disable_irq(irq_id);
			if (dev->ops->chip_shutdown)
				dev->ops->chip_shutdown();
			break;
		case TS_RESUME_DEVICE:	/*device power on or wakeup */
			if (dev->ops->chip_resume)
				error = dev->ops->chip_resume();
			break;
		case TS_AFTER_RESUME:	/*do something after resume */
			if (dev->ops->chip_after_resume)
				error =
				    dev->ops->
				    chip_after_resume((void *)&g_ts_data.
						      feature_info);
			send_up_msg_in_resume();
			if(g_ts_data.aft_param.aft_enable_flag)
			{
				TS_LOG_INFO("ts_kit aft resume\n");
				kobject_uevent(&g_ts_data.input_dev->dev.kobj,
					KOBJ_ONLINE);
			}
			atomic_set(&g_ts_data.state, TS_WORK);
			enable_irq(irq_id);
			ts_start_wd_timer(&g_ts_data);
			break;
		default:
			TS_LOG_ERR("pm_type = %d\n", pm_type);
			error = -EINVAL;
			break;
		}
	} else if (g_ts_data.chip_data->easy_wakeup_info.sleep_mode ==
		   TS_GESTURE_MODE) {
		switch (pm_type) {
		case TS_BEFORE_SUSPEND:	/*do something before suspend */
			ts_stop_wd_timer(&g_ts_data);
			disable_irq(irq_id);
			if (dev->ops->chip_before_suspend)
				error = dev->ops->chip_before_suspend();
			break;
		case TS_SUSPEND_DEVICE:	/*switch to easy-wakeup mode, and enable interrupts */
			atomic_set(&g_ts_data.state, TS_WORK_IN_SLEEP);
			if (dev->ops->chip_suspend)
				error = dev->ops->chip_suspend();
			enable_irq(irq_id);
			out_cmd->command = TS_WAKEUP_GESTURE_ENABLE;
			out_cmd->cmd_param.prv_params =
			    (void *)&g_ts_data.feature_info.
			    wakeup_gesture_enable_info;
			break;
		case TS_IC_SHUT_DOWN:
			disable_irq(irq_id);
			if (dev->ops->chip_shutdown)
				dev->ops->chip_shutdown();
			break;
		case TS_RESUME_DEVICE:	/*exit easy-wakeup mode and restore sth */
			disable_irq(irq_id);
			if (dev->ops->chip_resume)
				error = dev->ops->chip_resume();
			break;
		case TS_AFTER_RESUME:	/*do nothing */
			if (dev->ops->chip_after_resume)
				error =
				    dev->ops->
				    chip_after_resume((void *)&g_ts_data.
						      feature_info);
			send_up_msg_in_resume();
			atomic_set(&g_ts_data.state, TS_WORK);
			enable_irq(irq_id);
			ts_start_wd_timer(&g_ts_data);
			break;
		default:
			TS_LOG_ERR("pm_type = %d\n", pm_type);
			error = -EINVAL;
			break;
		}

	} else {
		TS_LOG_ERR("no such mode\n");
		error = -EINVAL;
	}
	return error;
}

static inline int ts_fw_update_boot(struct ts_cmd_node *in_cmd,
				    struct ts_cmd_node *out_cmd)
{
	char *fw_name = in_cmd->cmd_param.pub_params.firmware_info.fw_name;
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	if (dev->ops->chip_fw_update_boot)
		error = dev->ops->chip_fw_update_boot(fw_name);
	TS_LOG_INFO("process firmware update boot, return value:%d\n", error);

	return error;
}

static inline int ts_fw_update_sd(struct ts_cmd_node *in_cmd,
				  struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	TS_LOG_INFO("process firmware update sd\n");
	if (dev->ops->chip_fw_update_sd)
		error = dev->ops->chip_fw_update_sd();

	return error;
}

static inline int ts_calibrate_wakeup_gesture(struct ts_cmd_node *in_cmd,
					      struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_calibrate_info *info =
	    (struct ts_calibrate_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_DEBUG("process firmware calibrate\n");

	if (dev->ops->chip_calibrate_wakeup_gesture)
		error = dev->ops->chip_calibrate_wakeup_gesture();

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static inline int ts_calibrate(struct ts_cmd_node *in_cmd,
			       struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_calibrate_info *info =
	    (struct ts_calibrate_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_DEBUG("process firmware calibrate\n");

	if (dev->ops->chip_calibrate)
		error = dev->ops->chip_calibrate();

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static inline int ts_dsm_debug(struct ts_cmd_node *in_cmd,
			       struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_dsm_debug_info *info =
	    (struct ts_dsm_debug_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("ts dsm debug is called\n");

	if (dev->ops->chip_dsm_debug)
		error = dev->ops->chip_dsm_debug();

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static int ts_get_calibration_info(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_calibration_info_param *info = (struct ts_calibration_info_param *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	if (dev->ops->chip_get_calibration_info)
		error = dev->ops->chip_get_calibration_info(info, out_cmd);

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static int ts_oem_info_switch(struct ts_cmd_node *in_cmd,
			    struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_oem_info_param *info =
	    (struct ts_oem_info_param *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("ts chip data switch called\n");
	if (dev->ops->oem_info_switch)
		error = dev->ops->oem_info_switch(info);

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static int ts_get_chip_info(struct ts_cmd_node *in_cmd,
			    struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_chip_info_param *info =
	    (struct ts_chip_info_param *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("get chip info called\n");

	if (dev->ops->chip_get_info)
		error = dev->ops->chip_get_info(info);

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static int ts_get_capacitance_test_type(struct ts_cmd_node *in_cmd,
					struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_test_type_info *info =
	    (struct ts_test_type_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("get_mmi_test_mode called\n");

	if (dev->ops->chip_get_capacitance_test_type)
		error = dev->ops->chip_get_capacitance_test_type(info);

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static int ts_set_info_flag(struct ts_cmd_node *in_cmd,
			    struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_data *info = (struct ts_data *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("ts_set_info_flag called\n");

	if (dev->ops->chip_set_info_flag)
		error = dev->ops->chip_set_info_flag(info);
	return error;
}

static inline int ts_force_reset(struct ts_cmd_node *in_cmd,
				 struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	TS_LOG_INFO("ts force reset called\n");

	if (dev->ops->chip_reset)
		error = dev->ops->chip_reset();

	if (error) {
		out_cmd->command = TS_ERR_OCCUR;
		goto out;
	}

out:
	return error;
}

static inline int ts_read_rawdata(struct ts_cmd_node *in_cmd,
				  struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_rawdata_info *info =
	    (struct ts_rawdata_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("ts read rawdata called\n");


	if (dev->ops->chip_get_rawdata)
		error = dev->ops->chip_get_rawdata(info, out_cmd);

	if (!error) {
		TS_LOG_INFO("read rawdata success\n");
		info->status = TS_ACTION_SUCCESS;
		info->time_stamp = ktime_get();
		goto out;
	}

	info->status = TS_ACTION_FAILED;
	TS_LOG_ERR("read rawdata failed :%d\n", error);

out:
	return error;
}

static inline int ts_read_calibration_data(struct ts_cmd_node *in_cmd, struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_calibration_data_info *info = (struct ts_calibration_data_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	if (dev->ops->chip_get_calibration_data)
		error = dev->ops->chip_get_calibration_data(info, out_cmd);

	if (!error) {
		TS_LOG_INFO("read calibration data success\n");
		info->status = TS_ACTION_SUCCESS;
		info->time_stamp = ktime_get();
		goto out;
	}

	info->status = TS_ACTION_FAILED;
	TS_LOG_ERR("read calibration data failed :%d\n", error);

out:
	return error;
}

static inline int ts_read_debug_data(struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_diff_data_info *info =
	    (struct ts_diff_data_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_INFO("read diff data called\n");

	if (dev->ops->chip_get_debug_data)
		error = dev->ops->chip_get_debug_data(info, out_cmd);

	if (!error) {
		TS_LOG_INFO("read diff data success\n");
		info->status = TS_ACTION_SUCCESS;
		info->time_stamp = ktime_get();
		goto out;
	}

	info->status = TS_ACTION_FAILED;
	TS_LOG_INFO("read diff data failed :%d\n", error);

out:
	return error;
}

static inline int ts_palm_switch(struct ts_cmd_node *in_cmd,
				 struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_palm_info *info =
	    (struct ts_palm_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_DEBUG("palm action :%d, value:%d", info->op_action,
		     info->palm_switch);

	if (dev->ops->chip_palm_switch)
		error = dev->ops->chip_palm_switch(info);

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	TS_LOG_DEBUG("palm switch process result: %d\n", error);

	return error;
}

static inline int ts_glove_switch(struct ts_cmd_node *in_cmd,
				  struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_glove_info *info =
	    (struct ts_glove_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_DEBUG("glove action :%d, value:%d", info->op_action,
		     info->glove_switch);

	if (dev->ops->chip_glove_switch)
		error = dev->ops->chip_glove_switch(info);

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	TS_LOG_DEBUG("glove switch process result: %d\n", error);

	return error;
}

static inline int ts_wakeup_gesture_enable_switch(struct ts_cmd_node *in_cmd,
						  struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_wakeup_gesture_enable_info *info =
	    (struct ts_wakeup_gesture_enable_info *)in_cmd->cmd_param.
	    prv_params;

	TS_LOG_INFO("%s: write value: %d", __func__, info->switch_value);

	if (atomic_read(&g_ts_data.state) == TS_WORK_IN_SLEEP
	    && dev->ops->chip_wakeup_gesture_enable_switch) {
		error = dev->ops->chip_wakeup_gesture_enable_switch(info);
	}

	info->op_action = TS_ACTION_UNDEF;
	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("%s, process error: %d\n", __func__, error);
	} else {
		info->status = TS_ACTION_SUCCESS;
	}

	TS_LOG_DEBUG("%s, process result: %d\n", __func__, error);

	return error;
}

static inline int ts_holster_switch(struct ts_cmd_node *in_cmd,
				    struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_holster_info *info =
	    (struct ts_holster_info *)in_cmd->cmd_param.prv_params;

	TS_LOG_DEBUG("Holster action :%d, value:%d", info->op_action,
		     info->holster_switch);

	if (dev->ops->chip_holster_switch)
		error = dev->ops->chip_holster_switch(info);

	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("holster switch process error: %d\n", error);
	} else
		info->status = TS_ACTION_SUCCESS;

	TS_LOG_DEBUG("holster switch process result: %d\n", error);

	return error;
}

static inline int ts_roi_switch(struct ts_cmd_node *in_cmd,
				struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_roi_info *info =
	    (struct ts_roi_info *)in_cmd->cmd_param.prv_params;

	if (dev->ops->chip_roi_switch)
		error = dev->ops->chip_roi_switch(info);

	if (error) {
		info->status = TS_ACTION_FAILED;
		TS_LOG_ERR("roi switch process error: %d\n", error);
	} else
		info->status = TS_ACTION_SUCCESS;

	TS_LOG_INFO("roi action :%d, value:%d, process result: %d\n",
		    info->op_action, info->roi_switch, error);

	return error;
}

static inline int ts_touch_window(struct ts_cmd_node *in_cmd,
				  struct ts_cmd_node *out_cmd)
{
	struct ts_window_info *info =
	    (struct ts_window_info *)in_cmd->cmd_param.prv_params;

	g_ts_data.feature_info.window_info.window_enable = info->window_enable;
	g_ts_data.feature_info.window_info.top_left_x0 = info->top_left_x0;
	g_ts_data.feature_info.window_info.top_left_y0 = info->top_left_y0;
	g_ts_data.feature_info.window_info.bottom_right_x1 =
	    info->bottom_right_x1;
	g_ts_data.feature_info.window_info.bottom_right_y1 =
	    info->bottom_right_y1;

	info->status = TS_ACTION_SUCCESS;

	return NO_ERR;
}

static inline int ts_hand_detect(struct ts_cmd_node *in_cmd,
				 struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_hand_info *info =
	    (struct ts_hand_info *)in_cmd->cmd_param.prv_params;

	if (dev->ops->chip_hand_detect)
		error = dev->ops->chip_hand_detect(info);

	if (error)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static inline int ts_chip_regs_operate(struct ts_cmd_node *in_cmd,
				       struct ts_cmd_node *out_cmd)
{
	int error = -EIO;
	struct ts_device_data *dev = g_ts_data.chip_data;
	struct ts_regs_info *info =
	    (struct ts_regs_info *)in_cmd->cmd_param.prv_params;

	if (dev->ops->chip_regs_operate)
		error = dev->ops->chip_regs_operate(info);

	if (error < 0)
		info->status = TS_ACTION_FAILED;
	else
		info->status = TS_ACTION_SUCCESS;

	return error;
}

static inline int ts_err_process(struct ts_cmd_node *in_cmd,
				 struct ts_cmd_node *out_cmd)
{
	static int error_count;
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	TS_LOG_INFO("error process\n");

	if (dev->ops->chip_reset)
		error = dev->ops->chip_reset();
	if (error) {		/*error nest occurred, we define nest level*/
		out_cmd->command = TS_ERR_OCCUR;
		/*BUG_ON(unlikely(++error_count == TS_ERR_NEST_LEVEL));*/
		goto out;
	}

	error_count = 0;
out:
	return error;
}

static inline int ts_int_err_process(struct ts_cmd_node *in_cmd,
				     struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	if (dev->ops->chip_reset)
		error = dev->ops->chip_reset();

	if (error) {		/*error nest occurred, we define nest level*/
		out_cmd->command = TS_ERR_OCCUR;
		goto out;
	}

out:
	return error;
}

static inline int ts_test_cmd(struct ts_cmd_node *in_cmd,
			      struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	if (dev->ops->chip_test)
		error = dev->ops->chip_test(in_cmd, out_cmd);

	if (error) {
		out_cmd->command = TS_ERR_OCCUR;
		goto out;
	}

out:
	return error;
}

static inline int ts_check_status(struct ts_cmd_node *in_cmd,
				  struct ts_cmd_node *out_cmd)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	if (dev->ops->chip_check_status)
		error = dev->ops->chip_check_status();
	if (error) {
		out_cmd->command = TS_ERR_OCCUR;
	}

	ts_start_wd_timer(&g_ts_data);
	return error;
}

static inline bool ts_cmd_need_process(struct ts_cmd_node *cmd)
{
	bool is_need_process = true;
	struct ts_cmd_sync *sync;
	enum ts_pm_type pm_type = cmd->cmd_param.pub_params.pm_type;
	sync = cmd->sync;
	if (unlikely
	    ((atomic_read(&g_ts_data.state) == TS_SLEEP)
	     || (atomic_read(&g_ts_data.state) == TS_WORK_IN_SLEEP))) {
		if (atomic_read(&g_ts_data.state) == TS_SLEEP) {
			switch (cmd->command) {
			case TS_POWER_CONTROL:
				if ((pm_type != TS_RESUME_DEVICE)
				    && (pm_type != TS_AFTER_RESUME))
					is_need_process = false;
				break;
			case TS_TOUCH_WINDOW:
				is_need_process = true;
				break;
			case TS_INT_PROCESS:
			case TS_INT_ERR_OCCUR:
				//enable_irq(g_ts_data.irq_id);
				if (g_ts_data.chip_data->is_parade_solution){
					if(g_ts_data.chip_data->isbootupdate_finish==false)
						is_need_process = true;
					else
						is_need_process = false;
				}else{
					is_need_process = false;
				}
				break;
			case TS_GET_CHIP_INFO:
				is_need_process = true;
				break;
			default:
				is_need_process = false;
				break;
			}
		} else {
			switch (cmd->command) {
			case TS_POWER_CONTROL:
				if ((pm_type != TS_RESUME_DEVICE)
				    && (pm_type != TS_AFTER_RESUME))
					is_need_process = false;
				break;
			case TS_TOUCH_WINDOW:
				is_need_process = true;
				break;
			case TS_OEM_INFO_SWITCH:
				is_need_process = true;
				break;
			case TS_GET_CHIP_INFO:
				is_need_process = true;
				break;
			default:
				is_need_process = true;
				break;
			}
		}
	}

	if (!is_need_process && sync) {
		if (atomic_read(&sync->timeout_flag) == TS_TIMEOUT) {
			kfree(sync);
		} else {
			complete(&sync->done);
		}
	}

	return is_need_process;
}

static int ts_proc_command(struct ts_cmd_node *cmd)
{
	int error = NO_ERR;
	struct ts_cmd_sync *sync = cmd->sync;
	struct ts_cmd_node *proc_cmd = cmd;
	struct ts_cmd_node *out_cmd = &pang_cmd_buff;

	/*discard timeout cmd to fix panic*/
	if (sync && atomic_read(&sync->timeout_flag) == TS_TIMEOUT) {
		kfree(sync);
		goto out;
	}

	if (!ts_cmd_need_process(proc_cmd)) {
		TS_LOG_INFO("no need to process cmd:%d", proc_cmd->command);
		goto out;
	}

related_proc:
	out_cmd->command = TS_INVAILD_CMD;

	switch (proc_cmd->command) {
	case TS_INT_PROCESS:
		ts_proc_bottom_half(proc_cmd, out_cmd);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_INPUT_ALGO:
		ts_algo_calibrate(proc_cmd, out_cmd);
		break;
	case TS_REPORT_INPUT:
		ts_report_input(proc_cmd, out_cmd);
		break;
	case TS_POWER_CONTROL:
		ts_power_control(g_ts_data.irq_id, proc_cmd, out_cmd);
		break;
	case TS_FW_UPDATE_BOOT:
		disable_irq(g_ts_data.irq_id);
		ts_fw_update_boot(proc_cmd, out_cmd);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_FW_UPDATE_SD:
		disable_irq(g_ts_data.irq_id);
		ts_fw_update_sd(proc_cmd, out_cmd);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_DEBUG_DATA:
		disable_irq(g_ts_data.irq_id);
		ts_read_debug_data(proc_cmd, out_cmd);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_READ_RAW_DATA:
		disable_irq(g_ts_data.irq_id);
		ts_read_rawdata(proc_cmd, out_cmd);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_READ_CALIBRATION_DATA:
		disable_irq(g_ts_data.irq_id);
		ts_read_calibration_data(proc_cmd, out_cmd);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_GET_CALIBRATION_INFO:
		ts_get_calibration_info(proc_cmd, out_cmd);
		break;
	case TS_OEM_INFO_SWITCH:
		ts_oem_info_switch(proc_cmd, out_cmd);
		break;
	case TS_GET_CHIP_INFO:
		ts_get_chip_info(proc_cmd, out_cmd);
		break;
	case TS_SET_INFO_FLAG:
		ts_set_info_flag(proc_cmd, out_cmd);
		break;
	case TS_CALIBRATE_DEVICE:
		ts_calibrate(proc_cmd, out_cmd);
		break;
	case TS_CALIBRATE_DEVICE_LPWG:
		ts_calibrate_wakeup_gesture(proc_cmd, out_cmd);
		break;
	case TS_DSM_DEBUG:
		ts_dsm_debug(proc_cmd, out_cmd);
		break;
	case TS_GLOVE_SWITCH:
		ts_glove_switch(proc_cmd, out_cmd);
		break;
	case TS_TEST_TYPE:
		ts_get_capacitance_test_type(proc_cmd, out_cmd);
		break;
	case TS_PALM_SWITCH:
		ts_palm_switch(proc_cmd, out_cmd);
		break;
	case TS_HAND_DETECT:
		ts_hand_detect(proc_cmd, out_cmd);
		break;
	case TS_FORCE_RESET:
		disable_irq(g_ts_data.irq_id);
		ts_stop_wd_timer(&g_ts_data);
		ts_force_reset(proc_cmd, out_cmd);
		ts_start_wd_timer(&g_ts_data);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_INT_ERR_OCCUR:
		ts_stop_wd_timer(&g_ts_data);
		ts_int_err_process(proc_cmd, out_cmd);
		enable_irq(g_ts_data.irq_id);
		ts_start_wd_timer(&g_ts_data);
		break;
	case TS_ERR_OCCUR:
		disable_irq(g_ts_data.irq_id);
		ts_stop_wd_timer(&g_ts_data);
		ts_err_process(proc_cmd, out_cmd);
		ts_start_wd_timer(&g_ts_data);
		enable_irq(g_ts_data.irq_id);
		break;
	case TS_CHECK_STATUS:
		ts_check_status(proc_cmd, out_cmd);
		break;
	case TS_WAKEUP_GESTURE_ENABLE:
		ts_wakeup_gesture_enable_switch(proc_cmd, out_cmd);
		break;
	case TS_HOLSTER_SWITCH:
		ts_holster_switch(proc_cmd, out_cmd);
		break;
	case TS_ROI_SWITCH:
		ts_roi_switch(proc_cmd, out_cmd);
		break;
	case TS_TOUCH_WINDOW:
		ts_touch_window(proc_cmd, out_cmd);
		break;
#if defined(HUAWEI_CHARGER_FB)
	case TS_CHARGER_SWITCH:
		ts_charger_switch(proc_cmd, out_cmd);
		break;
#endif
	case TS_REGS_STORE:
		ts_chip_regs_operate(proc_cmd, out_cmd);
		break;
	case TS_TEST_CMD:
		ts_test_cmd(proc_cmd, out_cmd);
		break;
	case TS_HARDWARE_TEST:
		ts_special_hardware_test_switch(proc_cmd, out_cmd);
		break;
	case TS_READ_BRIGHTNESS_INFO:
		ts_get_brightness_info_cmd();
		break;
	case TS_TP_INIT:
		proc_init_cmd();
		break;
	default:
		break;
	}

	TS_LOG_DEBUG("command :%d process result:%d \n", proc_cmd->command,
		     error);

	if (out_cmd->command != TS_INVAILD_CMD) {
		TS_LOG_DEBUG("related command :%d  need process\n",
			     out_cmd->command);
		swap(proc_cmd, out_cmd);	/*ping - pang*/
		goto related_proc;
	}

	if (sync) {		/*notify wait threads by completion*/
		smp_mb();
		TS_LOG_DEBUG("wakeup threads in waitqueue\n");
		if (atomic_read(&sync->timeout_flag) == TS_TIMEOUT) {
			kfree(sync);
		} else {
			complete(&sync->done);
		}
	}

out:
	return error;
}

static int ts_proc_command_directly(struct ts_cmd_node *cmd)
{
	int error = NO_ERR;
	TS_LOG_INFO("%s Enter\n",__func__);
	/*Do not use cmd->sync in this func, setting it as null*/
	cmd->sync = NULL;
	if (!ts_cmd_need_process(cmd)) {
		TS_LOG_INFO("%s, no need to process cmd:%d",__func__, cmd->command);
		error = -EIO;
		goto out;
	}
	struct ts_cmd_node outcmd;
	mutex_lock(&g_ts_data.chip_data->device_call_lock);
	switch (cmd->command) {
		case TS_INT_PROCESS:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_INPUT_ALGO:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_REPORT_INPUT:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_POWER_CONTROL:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_FW_UPDATE_BOOT:
			error = ts_fw_update_boot(cmd, &outcmd);
			break;
		case TS_FW_UPDATE_SD:
			error = ts_fw_update_sd(cmd, &outcmd);
			break;
		case TS_DEBUG_DATA:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_READ_RAW_DATA:
			error = ts_read_rawdata(cmd, &outcmd);
			break;
		case TS_READ_CALIBRATION_DATA:
			error = ts_read_calibration_data(cmd, &outcmd);
			break;
		case TS_GET_CALIBRATION_INFO:
			error = ts_get_calibration_info(cmd, &outcmd);
			break;
		case TS_OEM_INFO_SWITCH:
			error = ts_oem_info_switch(cmd, &outcmd);
			break;
		case TS_GET_CHIP_INFO:
			error = ts_get_chip_info(cmd, &outcmd);
			break;
		case TS_SET_INFO_FLAG:
			error = ts_set_info_flag(cmd, &outcmd);
			break;
		case TS_CALIBRATE_DEVICE:
			error = ts_calibrate(cmd, &outcmd);
			break;
		case TS_CALIBRATE_DEVICE_LPWG:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_DSM_DEBUG:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_GLOVE_SWITCH:
			error = ts_glove_switch(cmd, &outcmd);
			break;
		case TS_TEST_TYPE:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_PALM_SWITCH:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_HAND_DETECT:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_FORCE_RESET:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_INT_ERR_OCCUR:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_ERR_OCCUR:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_CHECK_STATUS:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_WAKEUP_GESTURE_ENABLE:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_HOLSTER_SWITCH:
			error = ts_holster_switch(cmd, &outcmd);
			break;
		case TS_ROI_SWITCH:
			error = ts_roi_switch(cmd, &outcmd);
			break;
		case TS_TOUCH_WINDOW:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
#if defined(HUAWEI_CHARGER_FB)
		case TS_CHARGER_SWITCH:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
#endif
		case TS_REGS_STORE:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_TEST_CMD:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_HARDWARE_TEST:
			TS_LOG_ERR("%s, command %d does not support direct call!",__func__, cmd->command);
			break;
		case TS_READ_BRIGHTNESS_INFO:
			error = ts_get_brightness_info_cmd();
			break;
		default:
			TS_LOG_ERR("%s, command %d unknown!",__func__, cmd->command);
			break;
		}
		mutex_unlock(&g_ts_data.chip_data->device_call_lock);
		TS_LOG_DEBUG("%s, command :%d process result:%d \n",__func__, cmd->command,
				 error);
out:
	return error;
}

static bool ts_task_continue(void)
{
	bool task_continue = true;
	unsigned long flags;
	TS_LOG_DEBUG("prepare enter idle\n");

repeat:
	if (unlikely(kthread_should_stop())) {
		task_continue = false;
		goto out;
	}
	spin_lock_irqsave(&g_ts_data.queue.spin_lock, flags);
	smp_wmb();
	if (g_ts_data.queue.cmd_count) {
		set_current_state(TASK_RUNNING);
		TS_LOG_DEBUG("ts task state to  TASK_RUNNING\n");
		goto out_unlock;
	} else {
		set_current_state(TASK_INTERRUPTIBLE);
		TS_LOG_DEBUG("ts task state to  TASK_INTERRUPTIBLE\n");
		spin_unlock_irqrestore(&g_ts_data.queue.spin_lock, flags);
		schedule();
		goto repeat;
	}

out_unlock:
	spin_unlock_irqrestore(&g_ts_data.queue.spin_lock, flags);
out:
	return task_continue;
}

static long ts_ioctl_get_fingers_info(unsigned long arg)
{
	int ret = 0;
	void __user* argp = (void __user*)arg;
	struct ts_fingers data;
	u32 frame_size;

	//TS_LOG_ERR("[MUTI_AFT] ts_ioctl_get_fingers_info enter \n");
	if (arg == 0)
	{
		TS_LOG_ERR("arg == 0.\n");
		return -EINVAL;
	}
	/* wait event */
	atomic_set(&g_ts_data.fingers_waitq_flag, AFT_WAITQ_WAIT);
	ret = down_interruptible(&g_ts_data.fingers_aft_send);
	if(ret){
		TS_LOG_ERR("ret of down_interruptible: %d\n",ret);
	}
	ret = 0;
	if(atomic_read(&g_ts_data.fingers_waitq_flag) == AFT_WAITQ_WAIT)
	{
	      atomic_set(&g_ts_data.fingers_waitq_flag, AFT_WAITQ_IGNORE);
		return -EINVAL;
	}
	//TS_LOG_ERR("[MUTI_AFT] get_fingers_info status:%d, x:%d, y:%d,major:%d, minor:%d\n",g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].status,
		//g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].x,g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].y,
		//g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].major,g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].minor);
	if(atomic_read(&g_ts_data.fingers_waitq_flag) == AFT_WAITQ_WAKEUP)
	{
		if (copy_to_user(argp, &g_ts_data.fingers_send_aft_info,
				sizeof(struct ts_fingers)))
		{
			TS_LOG_ERR("ts_ioctl_get_fingers_info Failed to copy_from_user().\n");
			return -EFAULT;
		}
	}
	/*if (wait_event_interruptible(g_ts_kit_platform_data.fingers_waitq, (g_ts_kit_platform_data.fingers_waitq_flag == AFT_WAITQ_WAKEUP)))
	{
		TS_LOG_ERR("ts_ioctl_get_fingers_info -ERESTARTSYS\n");
		return -ERESTARTSYS;
	}
	g_ts_kit_platform_data.fingers_waitq_flag = AFT_WAITQ_WAIT;
	TS_LOG_ERR("get_fingers_info status:%d, x:%d, y:%d,major:%d, minor:%d \n",g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].status,
	g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].x,g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].y,
	g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].major,g_ts_kit_platform_data.fingers_send_aft_info.fingers[0].minor);
	if (copy_to_user(argp, &g_ts_kit_platform_data.fingers_send_aft_info,
			sizeof(struct ts_fingers)))
	{
		TS_LOG_ERR("ts_ioctl_get_fingers_info Failed to copy_from_user().\n");
		return -EFAULT;
	}*/
	return ret;
}
static long ts_ioctl_get_aft_param_info(unsigned long arg)
{
	if (arg == 0)
	{
		TS_LOG_ERR("arg == 0.\n");
		return -EINVAL;
	}
	if (copy_to_user(arg, &g_ts_data.aft_param,
			sizeof(struct ts_aft_algo_param)))
	{
		TS_LOG_ERR("ts_ioctl_get_aft_param_info Failed to copy_to_user().\n");
		return -EFAULT;
	}
	return 0;
}
static long ts_ioctl_set_coordinates(unsigned long arg)
{
	struct ts_fingers data;
	void __user* argp = (void __user*)arg;
	struct ts_fingers* finger = NULL;
	struct input_dev* input_dev = g_ts_data.input_dev;
	struct anti_false_touch_param *local_param = NULL;
	int finger_num = 0;
	int id = 0;
	unsigned long flags;
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
	struct aft_abs_param_major aft_abs_major;
	int major = 0;
	int minor = 0;
#else
	int x_y_distance = 0;
	short tmp_distance = 0;
	char *p;
#endif

	if(!input_dev){
		TS_LOG_ERR("The command node or input device is not exist!\n");
		return -EINVAL;
	}

	//TS_LOG_ERR("[MUTI_AFT] ts_ioctl_set_coordinates enter\n");
	if (arg == 0)
	{
		TS_LOG_ERR("arg == 0.\n");
		return -EINVAL;
	}
	if (copy_from_user(&data, argp,
			sizeof(struct ts_fingers)))
	{
		TS_LOG_ERR("ts_ioctl_set_coordinates Failed to copy_from_user().\n");
		return -EFAULT;
	}
	//memcpy(&g_ts_kit_platform_data.fingers_recv_aft_info,&data,sizeof(struct ts_fingers));
	finger = &data;
	if (g_ts_data.chip_data){
		local_param = &(g_ts_data.chip_data->anti_false_touch_param_data);
	}else{
		local_param = NULL;
	}

	TS_LOG_DEBUG("ts_report_input\n");
	ts_check_touch_window(finger);

	for (id = 0; id < TS_MAX_FINGER; id++) {
		if (finger->fingers[id].status == 0) {
			TS_LOG_DEBUG("never touch before: id is %d\n", id);
			continue;
		}
		if (finger->fingers[id].status == TS_FINGER_PRESS) {
			TS_LOG_DEBUG
				("down: id is %d, finger->fingers[id].pressure = %d, finger->fingers[id].x = %d, finger->fingers[id].y = %d\n",
				id, finger->fingers[id].pressure,
				finger->fingers[id].x, finger->fingers[id].y);
			finger_num++;
			input_report_abs(input_dev, ABS_MT_PRESSURE,
					finger->fingers[id].pressure);
			input_report_abs(input_dev, ABS_MT_POSITION_X,
					finger->fingers[id].x);
			input_report_abs(input_dev, ABS_MT_POSITION_Y,
					finger->fingers[id].y);
			input_report_abs(input_dev, ABS_MT_TRACKING_ID, id);
			if (local_param && local_param->feature_all){
				if (local_param->sensor_x_width && local_param->sensor_y_width){
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
					if ((finger->fingers[id].major || finger->fingers[id].minor)
						&& (!g_ts_data.feature_info.holster_info.holster_switch)){
						major = 0; minor = 1;
						memset(&aft_abs_major, 0, sizeof(struct aft_abs_param_major));
						aft_abs_major.edgex = finger->fingers[id].major * local_param->sensor_x_width;
						aft_abs_major.edgey = finger->fingers[id].minor * local_param->sensor_y_width;
						aft_abs_major.orientation = finger->fingers[id].orientation;
						aft_abs_major.version = 0x01;
						memcpy(&major, &aft_abs_major, sizeof(int));
						input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, major);
						input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, minor);
					}else{
						major = 0; minor = 0;
						input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, major);
						input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, minor);
					}
#else
					x_y_distance = 0;
					p = (char *)&x_y_distance;
					tmp_distance = finger->fingers[id].major * local_param->sensor_x_width;
					memcpy(p, (char *)&tmp_distance, sizeof(short));
					tmp_distance = finger->fingers[id].minor * local_param->sensor_y_width;
					memcpy(p+sizeof(short), (char *)&tmp_distance, sizeof(short));
					input_report_abs(input_dev, ABS_MT_DISTANCE, x_y_distance);
#endif
				}else{
#if ANTI_FALSE_TOUCH_USE_PARAM_MAJOR_MINOR
					input_report_abs(input_dev, ABS_MT_WIDTH_MAJOR, 0);
					input_report_abs(input_dev, ABS_MT_WIDTH_MINOR, 0);
#else
					input_report_abs(input_dev, ABS_MT_DISTANCE, 0);
#endif
				}
			}
			input_mt_sync(input_dev);       /*modfiy by mengkun*/
		} else if (finger->fingers[id].status == TS_FINGER_RELEASE) {
			TS_LOG_DEBUG("up: id is %d, status = %d\n", id,
				finger->fingers[id].status);
			input_mt_sync(input_dev);	/*modfiy by mengkun*/
		}
	}

	input_report_key(input_dev, BTN_TOUCH, finger_num);
	input_sync(input_dev);

	ts_film_touchplus(finger, finger_num, input_dev);
	if (((g_ts_data.chip_data->easy_wakeup_info.sleep_mode ==
		TS_GESTURE_MODE)
		|| (g_ts_data.chip_data->easy_wakeup_info.palm_cover_flag == true))
		&& (g_ts_data.feature_info.holster_info.holster_switch == 0)) {
		input_report_key(input_dev, finger->gesture_wakeup_value, 1);
		input_sync(input_dev);
		input_report_key(input_dev, finger->gesture_wakeup_value, 0);
		input_sync(input_dev);
	}
	TS_LOG_DEBUG("ts_report_input done, finger_num = %d\n", finger_num);
	if ((g_ts_data.aft_param.aft_enable_flag) && (finger->add_release_flag))
	{
		finger->add_release_flag = 0;
		input_report_key(input_dev, BTN_TOUCH, 0);
		input_mt_sync(input_dev);
		input_sync(input_dev);
		//TS_LOG_ERR("[MUTI_AFT] report the added release event\n");
	}
	//TS_LOG_ERR("[MUTI_AFT] ts_report_input done, finger_num = %d\n", finger_num);


	atomic_set(&g_data_report_over, 1);
	/*TS_LOG_ERR("[MUTI_AFT] set_coordinates status:%d, x:%d, y:%d,major:%d, minor:%d \n",g_ts_kit_platform_data.fingers_recv_aft_info.fingers[0].status,
		g_ts_kit_platform_data.fingers_recv_aft_info.fingers[0].x,g_ts_kit_platform_data.fingers_recv_aft_info.fingers[0].y,
		g_ts_kit_platform_data.fingers_recv_aft_info.fingers[0].major,g_ts_kit_platform_data.fingers_recv_aft_info.fingers[0].minor);
	up(&g_ts_kit_platform_data.fingers_aft_done); */
	return 0;
}
static int aft_get_info_misc_open(struct inode* inode, struct file* filp)
{
	return 0;
}

static int aft_get_info_misc_release(struct inode* inode, struct file* filp)
{
	/*if(g_ts_kit_platform_data.aft_param.aft_enable_flag)
	{
		g_ts_kit_platform_data.fingers_waitq_flag = AFT_WAITQ_WAKEUP;
		wake_up_interruptible(&(g_ts_kit_platform_data.fingers_waitq));
	}*/
	return 0;
}

static long aft_get_info_misc_ioctl(struct file* filp, unsigned int cmd,
                                   unsigned long arg)
{
	long ret;
	//TS_LOG_ERR("[MUTI_AFT] aft_get_info_misc_ioctl enter cmd:%d\n",cmd);
	switch (cmd)
	{
	case INPUT_AFT_IOCTL_CMD_GET_TS_FINGERS_INFO:
		ret = ts_ioctl_get_fingers_info(arg);
		break;
	case INPUT_AFT_IOCTL_CMD_GET_ALGO_PARAM_INFO:
		ret = ts_ioctl_get_aft_param_info(arg);
		break;
	default:
		TS_LOG_ERR("cmd unkown.\n");
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations g_aft_get_info_misc_fops =
{
	.owner = THIS_MODULE,
	.open = aft_get_info_misc_open,
	.release = aft_get_info_misc_release,
	.unlocked_ioctl = aft_get_info_misc_ioctl,
};
static struct miscdevice g_aft_get_info_misc_device =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_AFT_GET_INFO,
	.fops = &g_aft_get_info_misc_fops,
};
static int aft_set_info_misc_open(struct inode* inode, struct file* filp)
{
	return 0;
}

static int aft_set_info_misc_release(struct inode* inode, struct file* filp)
{
	/*if(g_ts_kit_platform_data.aft_param.aft_enable_flag)
	{
		g_ts_kit_platform_data.fingers_waitq_flag = AFT_WAITQ_WAKEUP;
		wake_up_interruptible(&(g_ts_kit_platform_data.fingers_waitq));
	}*/
	return 0;
}

static long aft_set_info_misc_ioctl(struct file* filp, unsigned int cmd,
                                   unsigned long arg)
{
	long ret;
	//TS_LOG_ERR("[MUTI_AFT] aft_Set_info_misc_ioctl enter cmd:%d\n",cmd);
	switch (cmd)
	{
	case INPUT_AFT_IOCTL_CMD_SET_COORDINATES:
		ret = ts_ioctl_set_coordinates(arg);
		break;
	default:
		TS_LOG_ERR("cmd unkown.\n");
		ret = -EINVAL;
	}

	return ret;
}

static const struct file_operations g_aft_set_info_misc_fops =
{
	.owner = THIS_MODULE,
	.open = aft_set_info_misc_open,
	.release = aft_set_info_misc_release,
	.unlocked_ioctl = aft_set_info_misc_ioctl,
};
static struct miscdevice g_aft_set_info_misc_device =
{
	.minor = MISC_DYNAMIC_MINOR,
	.name = DEVICE_AFT_SET_INFO,
	.fops = &g_aft_set_info_misc_fops,
};
static int ts_input_open(struct input_dev *dev)
{
	TS_LOG_DEBUG("input_open called:%d\n", dev->users);
	return NO_ERR;
}

static void ts_input_close(struct input_dev *dev)
{
	TS_LOG_DEBUG("input_close called:%d\n", dev->users);
}

static int chip_detect(const char *chip_name)
{
	bool found = false;
	int index;
	int error = -EIO;
	int len;
	const __be32 *addr;
	struct device_node *child = NULL;
	struct device_node *root = g_ts_data.node;
	struct ts_device_data *ptr = &g_ts_device_map[0];

	for_each_child_of_node(root, child) {	/*find the chip node*/
		if (of_device_is_compatible(child, chip_name)) {
			found = true;
			break;
		}
	}

	if (!found) {
		TS_LOG_ERR("not find chip :%s's node\n", chip_name);
		goto out;
	}
	g_ts_data.dev_node = child;
	if (g_ts_data.bops->btype == TS_BUS_I2C) {	/*i2c ts need get chip addr*/
		addr = of_get_property(child, "slave_address", &len);
		if (!addr || (len < sizeof(int))) {
			TS_LOG_ERR("invalid slave_address on %s, len:%d\n",
				   chip_name, len);
			goto out;
		}
		if (g_ts_data.client->addr != be32_to_cpup(addr)) {
			error =
			    i2c_check_addr_busy(g_ts_data.client->adapter,
						be32_to_cpup(addr));
			if (error) {
				TS_LOG_ERR("0x%x slave addr conflict\n", *addr);
				goto out;
			}
			TS_LOG_DEBUG("slave addr :0x%x not occupied\n", *addr);
			g_ts_data.client->addr = be32_to_cpup(addr);
		}
	}

	for (index = 0; ptr != NULL && index < ARRAY_SIZE(g_ts_device_map);
	     ptr++, index++) {
		if (!strcmp(chip_name, ptr->chip_name)) {
			if (g_ts_data.bops->btype == TS_BUS_I2C)
				ptr->slave_addr = g_ts_data.client->addr;

			ptr->bops = g_ts_data.bops;
			INIT_LIST_HEAD(&ptr->algo_head);
			g_ts_data.chip_data = ptr;
			/*setting the default data*/
			g_ts_data.chip_data->is_i2c_one_byte = 0;
			g_ts_data.chip_data->is_new_oem_structure= 0;
			g_ts_data.chip_data->is_parade_solution= 0;

			/*have parse dt func , execute it now*/
			if (ptr->ops->chip_parse_config
			    && ptr->ops->chip_parse_config(child, ptr)) {
				TS_LOG_ERR
				    ("call %s's chip self parse config failed\n",
				     chip_name);
				error = -EINVAL;
				goto out;
			}

			if (ptr->ops->chip_detect) {
				TS_LOG_DEBUG("call chip self init handle\n");
				error =
				    ptr->ops->chip_detect(child, ptr,
							  g_ts_data.ts_dev);
			}
			goto out;
		}
	}
	if (ARRAY_SIZE(g_ts_device_map) == index) {
		TS_LOG_ERR("we can not find %s's chip data in device map\n",
			   chip_name);
		error = -EINVAL;
	}
out:
	return error;
}

static int get_support_chip(void)
{
	struct device_node *np = g_ts_data.node;
	const char *support_chip_name = NULL;
	/*const char *bus_type;*/
	int rc = 0;
	int index = 0;
	int found = -ENXIO;
	int array_len = 0;

	if (np) {		/*support dts*/
		array_len = of_property_count_strings(np, "support_chip_name");
		if (array_len <= 0) {
			TS_LOG_ERR("chip name length invaild:%d\n", array_len);
			return -EINVAL;
		}

		for (index = 0; index < array_len; index++) {	/*try to detect active ts ic*/
			rc = of_property_read_string_index(np,
							   "support_chip_name",
							   index,
							   &support_chip_name);
			if (rc) {
				TS_LOG_ERR("read %d - chip name :%s, err:%d\n",
					   index, support_chip_name, rc);
				continue;
			}

			if (!chip_detect(support_chip_name)) {
				found = NO_ERR;
				TS_LOG_INFO("chip: %s found success\n",
					    support_chip_name);
				break;
			}
		}
	} else {		/*not support dts*/
		TS_LOG_ERR("dts feature not support: %d\n", rc);
	}
	return found;
}

static int ts_parse_config(void)
{
	int error = NO_ERR;
	int rc;
	int index;
	char *tmp_buff = NULL;

	if (g_ts_data.node) {
		rc = of_property_read_string(g_ts_data.node, "product",
					     (const char **)&tmp_buff);
		if (rc) {
			TS_LOG_ERR("product read failed:%d\n", rc);
			error = -EINVAL;
			goto out;
		}

		rc = of_property_read_u32(g_ts_data.node, "fpga_flag",
					  &g_ts_data.fpga_flag);
		if (rc) {
			TS_LOG_ERR("fpga flag read failed:%d, set to 0\n", rc);
			g_ts_data.fpga_flag = 0;
		}
	}

	if (!tmp_buff) {
		TS_LOG_ERR("tmp_buff failed\n");
		error = -EINVAL;
		goto out;
	}

	for (index = 0; tmp_buff[index] && index < strlen(tmp_buff); index++)	/*exchange name to lower*/
		g_ts_data.product_name[index] = tolower(tmp_buff[index]);

	if (index == MAX_STR_LEN)
		g_ts_data.product_name[MAX_STR_LEN - 1] = '\0';

	TS_LOG_INFO("parse product name :%s\n", g_ts_data.product_name);

out:
	return error;
}

static ssize_t ts_get_virtual_keys_range(void)
{
	const __be32 *values = NULL;
	int size = 0;
	int len = 0;
	u16 *val_array = NULL;
	int ret = 0;
	int count = 0;
	values = of_get_property(g_ts_data.node, "virtual_keys", &len);
	if (NULL == values) {
		TS_LOG_ERR("%s:of_get_property read fail, line=%d\n", __func__,
			   __LINE__);
		ret = -EINVAL;
		goto fail;
	}
	size = len / sizeof(u32);
	val_array = kzalloc(size * sizeof(u16), GFP_KERNEL);
	if (NULL == val_array) {
		TS_LOG_ERR("%s:memory not enough,fail line=%d\n", __func__,
			   __LINE__);
		ret = -EINVAL;
		goto fail;
	}

	for (count = 0; count < size; count++) {
		val_array[count] = (u16) be32_to_cpup(values++);
	}

	if (size % VIRTUAL_KEY_ELEMENT_SIZE) {
		ret = -ENOMEM;
		TS_LOG_ERR("%s:fail line=%d virtual keys size error\n",
			   __func__, __LINE__);
		goto fail_del_values;
	}
	g_ts_data.virtual_keys_size = size;
	g_ts_data.virtual_keys_values = (int *)val_array;
	return ret;

fail_del_values:
	kfree(val_array);
fail:
	return ret;
}

static int ts_parse_virtualkey(void)
{
	int error = NO_ERR;

	if (g_ts_data.node) {
		error =
		    of_property_read_u32(g_ts_data.node, "has_virtualkey",
					 &g_ts_data.chip_data->has_virtualkey);
		if (error) {
			TS_LOG_ERR("%s:get has_virtualkey failed\n", __func__);
			return -EINVAL;
		}
	}
	if (g_ts_data.chip_data->has_virtualkey) {
		error =
		    of_property_read_u32(g_ts_data.node, "lcd_full",
					 &g_ts_data.chip_data->lcd_full);
		if (error) {
			TS_LOG_ERR("%s:get device lcd_full failed\n", __func__);
			error = -EINVAL;
		}
		error = ts_get_virtual_keys_range();
		if (error) {
			TS_LOG_ERR
			    ("%s:fail to get virtual_keys_range form dts\n",
			     __func__);
			error = -EINVAL;
		}
	}
	TS_LOG_INFO("ts_data.chip_data->has_virtualkey = %d\n",
		    g_ts_data.chip_data->has_virtualkey);
	return error;
}

static int get_ts_bus_info(void)
{
	const char *bus_type;

	int rc;
	int error = NO_ERR;
	u32 bus_id = 0;

	g_ts_data.node = NULL;

	g_ts_data.node = of_find_compatible_node(NULL, NULL, TS_DEV_NAME);
	if (!g_ts_data.node) {
		TS_LOG_ERR("can't find ts module node\n");
		error = -EINVAL;
		goto out;
	}

	rc = of_property_read_string(g_ts_data.node, "bus_type", &bus_type);
	if (rc) {
		TS_LOG_ERR("bus type read failed:%d\n", rc);
		error = -EINVAL;
		goto out;
	}
	if (!strcmp(bus_type, "i2c")) {
		g_ts_data.bops = &ts_bus_i2c_info;
	} else if (!strcmp(bus_type, "spi")) {
		g_ts_data.bops = &ts_bus_spi_info;
	} else {
		TS_LOG_ERR("bus type invaild:%s\n", bus_type);
		error = -EINVAL;
	}

	rc = of_property_read_u32(g_ts_data.node, "bus_id", &bus_id);
	if (rc) {
		TS_LOG_ERR("bus id read failed\n");
		error = -EINVAL;
		goto out;
	}
	g_ts_data.bops->bus_id = bus_id;
	TS_LOG_DEBUG("bus id :%d\n", bus_id);
    rc = of_property_read_u32(g_ts_data.node, "aft_enable", &g_ts_data.aft_param.aft_enable_flag);
    if (g_ts_data.aft_param.aft_enable_flag)
    {
	  of_property_read_u32(g_ts_data.node, "drv_stop_width", &g_ts_data.aft_param.drv_stop_width);
	  of_property_read_u32(g_ts_data.node, "lcd_width", &g_ts_data.aft_param.lcd_width);
	  of_property_read_u32(g_ts_data.node, "lcd_height", &g_ts_data.aft_param.lcd_height);
        TS_LOG_INFO("aft enable,drv_stop_width is %d,lcd_width is %d, lcd_height is %d\n",
			g_ts_data.aft_param.drv_stop_width,g_ts_data.aft_param.lcd_width,g_ts_data.aft_param.lcd_height);
    }
    else
    {
        TS_LOG_INFO("aft disable\n");
    }
out:
	return error;
}

static int ts_chip_init(void)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	TS_LOG_INFO("ts_chip_init called\n");
	mutex_init(&easy_wake_guesure_lock);
	if(g_ts_data.chip_data->is_direct_proc_cmd == 0){
		if (dev->ops->chip_init) {
			error = dev->ops->chip_init();
		}
	}
	if (error) {
		TS_LOG_ERR("chip init failed\n");
	}
#ifdef CONFIG_HUAWEI_DSM
	else {
		if (dev->chip_name && DSM_MAX_IC_NAME_LEN > strlen(dev->chip_name)) {
			dsm_tp.ic_name = dev->chip_name;
			if (dsm_update_client_vendor_info(&dsm_tp)) {
				TS_LOG_ERR("dsm update client_vendor_info is failed\n");
			}
		} else {
			TS_LOG_ERR("ic_name, module_name is invalid\n");
		}
}
#endif

	return error;
}

static void ts_get_brightness_info(void)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;
	g_lcd_brightness_info = 0;

	TS_LOG_INFO("ts_get_brightness_info called\n");
	if(g_ts_data.chip_data->is_direct_proc_cmd){
		struct ts_cmd_node cmd;
		memset(&cmd, 0, sizeof(struct ts_cmd_node));
		cmd.command = TS_READ_BRIGHTNESS_INFO;
		error = put_one_cmd(&cmd, NO_SYNC_TIMEOUT);
	} else {
		if (dev->ops->chip_get_brightness_info) {
			g_lcd_brightness_info = dev->ops->chip_get_brightness_info();
			TS_LOG_INFO("ts_get_brightness_info  brightness data:%d\n", g_lcd_brightness_info);
		}
	}
	return;
}

static int ts_get_brightness_info_cmd(void)
{
	struct ts_device_data *dev = g_ts_data.chip_data;
	int rc = NO_ERR;
	g_lcd_brightness_info = 0;
	TS_LOG_INFO("ts_get_brightness_info_cmd called\n");

	if (dev->ops->chip_get_brightness_info) {
		if(g_ts_data.chip_data->is_direct_proc_cmd){
			rc = dev->ops->chip_get_brightness_info();
			if(rc > 0){
				g_lcd_brightness_info = rc;
				rc = NO_ERR;
			}
		} else {
			g_lcd_brightness_info = dev->ops->chip_get_brightness_info();
		}
		TS_LOG_INFO("ts_get_brightness_info_cmd brightness data:%d\n", g_lcd_brightness_info);
	}
	return rc;
}

static int ts_register_algo(void)
{
	int error = NO_ERR;
	struct ts_device_data *dev = g_ts_data.chip_data;

	TS_LOG_INFO("register algo called\n");

	error = ts_register_algo_func(dev);

	return error;
}

static void ts_ic_shutdown(void)
{
	struct ts_device_data *dev = g_ts_data.chip_data;
	if (dev->ops->chip_shutdown)
		dev->ops->chip_shutdown();
	return;
}

static ssize_t ts_virtual_keys_show(struct kobject *kobj,
				    struct kobj_attribute *attr, char *buf)
{
	u16 *data = (u16 *)g_ts_data.virtual_keys_values;
	int size = g_ts_data.virtual_keys_size;
	int index = 0;
	int i = 0;
	int count = 0;

	TS_LOG_ERR("%s:keys_show begin\n", __func__);

	for (i = 0; i < size; i += VIRTUAL_KEY_ELEMENT_SIZE) {
		count = scnprintf(buf + index, MAX_PRBUF_SIZE - index,
				  "0x01:%d:%d:%d:%d:%d\n",
				  data[i], data[i + 1], data[i + 2],
				  data[i + 3], data[i + 4]);

		if (count > 0) {
			index += count;
		} else {
			TS_LOG_ERR("%s:print to buff error,err num=%d\n",
				   __func__, count);
			return count;
		}
	}

	return index;
}

static struct kobj_attribute virtual_keys_attr = {
	.attr = {
		 .name = "virtualkeys.huawei,touchscreen",
		 .mode = S_IRUGO,
		 },
	.show = &ts_virtual_keys_show,
};

static struct attribute *properties_attrs[] = {
	&virtual_keys_attr.attr,
	NULL
};

static struct attribute_group properties_attr_group = {
	.attrs = properties_attrs,
};

static void __init ts_virtual_keys_init(void)
{
	struct kobject *properties_kobj;
	int ret = 0;

	properties_kobj = kobject_create_and_add("board_properties", NULL);
	if (properties_kobj)
		ret = sysfs_create_group(properties_kobj,
					 &properties_attr_group);
	if (!properties_kobj || ret)
		pr_err("%s: failed to create board_properties!\n", __func__);
}

static int try_update_firmware(void)
{
	char joint_chr = '_';
	int error = NO_ERR;
	char *fw_name;
	struct ts_cmd_node cmd;

	memset(&cmd, 0, sizeof(struct ts_cmd_node));
	cmd.command = TS_FW_UPDATE_BOOT;
	fw_name = cmd.cmd_param.pub_params.firmware_info.fw_name;

	/*firmware name [product_name][ic_name][module][vendor] */
	strncat(fw_name, g_ts_data.product_name, MAX_STR_LEN);
	strncat(fw_name, &joint_chr, 1);
	strncat(fw_name, g_ts_data.chip_data->chip_name, MAX_STR_LEN);
	strncat(fw_name, &joint_chr, 1);
#if 0
	strncat(fw_name, &joint_chr, 1);
	strncat(fw_name, g_ts_data.chip_data->module_name, MAX_STR_LEN);
	strncat(fw_name, &joint_chr, 1);
	strncat(fw_name, g_ts_data.chip_data->version_name, MAX_STR_LEN);
#endif

	error = put_one_cmd(&cmd, NO_SYNC_TIMEOUT);

	return error;
}

void check_tp_calibration_info(void)
{
	int error = NO_ERR;
	struct ts_cmd_node *cmd = NULL;
	struct ts_calibration_info_param *info = NULL;

	TS_LOG_INFO("%s called\n", __FUNCTION__);

	cmd = (struct ts_cmd_node *)kzalloc(sizeof(struct ts_cmd_node), GFP_KERNEL);
	if (!cmd) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	info = (struct ts_calibration_info_param *)kzalloc(sizeof(struct ts_calibration_info_param), GFP_KERNEL);
	if (!info) {
		TS_LOG_ERR("malloc failed\n");
		error = -ENOMEM;
		goto out;
	}

	cmd->command = TS_GET_CALIBRATION_INFO;
	cmd->cmd_param.prv_params = (void *)info;

	error = put_one_cmd(cmd, LONG_SYNC_TIMEOUT);
	if (error) {
		TS_LOG_ERR("put cmd error :%d\n", error);
		error = -EBUSY;
		goto out;
	}

	if (info->status != TS_ACTION_SUCCESS) {
		TS_LOG_ERR("read action failed\n");
		error = -EIO;
		goto out;
	}
out:
	if (cmd){
		kfree(cmd);
		cmd =NULL;
	}
	if (info){
		kfree(info);
		info =NULL;
	}
	TS_LOG_INFO("%s done\n", __FUNCTION__);

	return error;
}

static int ts_init(void)
{
	int error = NO_ERR;
	unsigned int touch_recovery_mode = 0;
	unsigned int charge_flag = 0;
	unsigned int irq_flags;
	struct input_dev *input_dev;

	atomic_set(&g_ts_data.state, TS_UNINIT);
	atomic_set(&g_ts_data.ts_esd_state, TS_NO_ESD);

	g_ts_data.edge_wideth = EDGE_WIDTH_DEFAULT;

	g_ts_data.queue.rd_index = 0;
	g_ts_data.queue.wr_index = 0;
	g_ts_data.queue.cmd_count = 0;
	g_ts_data.queue.queue_size = TS_CMD_QUEUE_SIZE;

	spin_lock_init(&g_ts_data.queue.spin_lock);
	TS_LOG_DEBUG("ts init: cmd queue size : %d\n", TS_CMD_QUEUE_SIZE);

	wake_lock_init(&g_ts_data.ts_wake_lock, WAKE_LOCK_SUSPEND,
		       "ts_wake_lock");

	error = ts_parse_config();
	if (error) {
		TS_LOG_ERR("ts parse config failed : %d\n", error);
		goto err_out;
	}

	g_ts_data.ts_dev = platform_device_alloc("huawei_touch", -1);
	if (!g_ts_data.ts_dev) {
		TS_LOG_ERR("platform device malloc failed\n");
		error = -ENOMEM;
		goto err_out;
	}

	error = platform_device_add(g_ts_data.ts_dev);
	if (error) {
		TS_LOG_ERR("platform device add failed :%d\n", error);
		goto err_put_platform_dev;
	}

	error = sysfs_create_group(&g_ts_data.ts_dev->dev.kobj, &ts_attr_group);
	if (error) {
		TS_LOG_ERR("can't create ts's sysfs\n");
		goto err_del_platform_dev;
	}

	procfs_create();

	error =
	    sysfs_create_link(NULL, &g_ts_data.ts_dev->dev.kobj, "touchscreen");
	if (error) {
		TS_LOG_ERR("%s: Fail create link error = %d\n", __func__,
			   error);
		goto err_free_sysfs;
	}

	error = get_support_chip();
	if (error) {
		TS_LOG_ERR("get support chip failed : %d\n", error);
		goto err_remove_sysfs_link;
	}

	error = ts_parse_virtualkey();
	if (error) {
		TS_LOG_ERR("ts parse vitualkey config failed : %d\n", error);
		goto err_remove_sysfs_link;
	}

	error = ts_chip_init();
	if (error) {
		TS_LOG_ERR("chip init failed : %d,  try fw update again\n",
			   error);
	}

	if(g_ts_data.chip_data->is_direct_proc_cmd){
		g_ts_data.chip_data->is_can_device_use_int = false;
		g_ts_data.no_int_queue.rd_index = 0;
		g_ts_data.no_int_queue.wr_index = 0;
		g_ts_data.no_int_queue.cmd_count = 0;
		g_ts_data.no_int_queue.queue_size = TS_CMD_QUEUE_SIZE;
		spin_lock_init(&g_ts_data.no_int_queue.spin_lock);
		INIT_WORK(&tp_init_work, tp_init_work_fn);
	}

#if defined (CONFIG_HUAWEI_DSM)
	chip_detfail_dsm = true;	/*true means after chip init to dsm*/
#endif
	error = ts_register_algo();
	if (error) {
		TS_LOG_ERR("ts register algo failed : %d\n", error);
		goto err_remove_sysfs_link;
	}
    if(g_ts_data.aft_param.aft_enable_flag)
    {
	    error = misc_register(&g_aft_get_info_misc_device);
	    if (error)
	    {
	        TS_LOG_ERR("Failed to register misc device\n");
	        goto err_remove_sysfs_link;
	    }
	    error = misc_register(&g_aft_set_info_misc_device);
	    if (error)
	    {
	        TS_LOG_ERR("Failed to register misc device\n");
	        goto err_remove_sysfs_link;
	    }
	    sema_init(&g_ts_data.fingers_aft_send,0);
        atomic_set(&g_ts_data.fingers_waitq_flag, AFT_WAITQ_IDLE);
    }
	if (1 == g_ts_data.chip_data->has_virtualkey) {
		TS_LOG_INFO("g_ts_data.chip_data->has_virtualkey = %d\n",
			    g_ts_data.chip_data->has_virtualkey);
		ts_virtual_keys_init();
	}

	input_dev = input_allocate_device();
	if (!input_dev) {
		TS_LOG_ERR("failed to allocate memory for input dev\n");
		error = -ENOMEM;
		goto err_remove_sysfs_link;
	}

	input_dev->name = TS_DEV_NAME;
	if (g_ts_data.bops->btype == TS_BUS_I2C)
		input_dev->id.bustype = BUS_I2C;
	else if (g_ts_data.bops->btype == TS_BUS_SPI)
		input_dev->id.bustype = BUS_SPI;
	input_dev->dev.parent = &g_ts_data.ts_dev->dev;
	input_dev->open = ts_input_open;
	input_dev->close = ts_input_close;
	g_ts_data.input_dev = input_dev;

	if (g_ts_data.chip_data->ops->chip_input_config)	/*config input for diff chip*/
		error =
		    g_ts_data.chip_data->ops->chip_input_config(g_ts_data.
								input_dev);
	if (error)
		goto err_free_dev;

	input_set_drvdata(input_dev, &g_ts_data);

	error = input_register_device(input_dev);
	if (error) {
		TS_LOG_ERR("input dev register failed : %d\n", error);
		goto err_free_dev;
	}
#if defined(CONFIG_FB)
	g_ts_data.fb_notify.notifier_call = fb_notifier_callback;
	error = fb_register_client(&g_ts_data.fb_notify);
	if (error) {
		TS_LOG_ERR("unable to register fb_notifier: %d\n", error);
		goto err_free_input_dev;
	}
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	g_ts_data.early_suspend.level =
	    EARLY_SUSPEND_LEVEL_BLANK_SCREEN + TS_SUSPEND_LEVEL;
	g_ts_data.early_suspend.suspend = ts_early_suspend;
	g_ts_data.early_suspend.resume = ts_late_resume;
	register_early_suspend(&g_ts_data.early_suspend);
#endif

#if defined (CONFIG_TEE_TUI)
	register_tui_driver(tui_tp_init, "tp", g_ts_data.chip_data->tui_data);
#endif
	g_ts_data.irq_id = g_ts_data.client->irq =
	    gpio_to_irq(g_ts_data.chip_data->irq_gpio);
	tp_gpio_num = g_ts_data.chip_data->irq_gpio;
	switch (g_ts_data.chip_data->irq_config) {
	case TS_IRQ_LOW_LEVEL:
		irq_flags = IRQF_TRIGGER_LOW;
		break;
	case TS_IRQ_HIGH_LEVEL:
		irq_flags = IRQF_TRIGGER_HIGH;
		break;
	case TS_IRQ_RAISE_EDGE:
		irq_flags = IRQF_TRIGGER_RISING;
		break;
	case TS_IRQ_FALL_EDGE:
		irq_flags = IRQF_TRIGGER_FALLING;
		break;
	default:
		TS_LOG_ERR("ts irq_config invaild\n");
		goto err_unregister_suspend;
	}

	atomic_set(&g_ts_data.state, TS_WORK);	/*avoid 1st irq unable to handler*/
	error =
	    request_irq(g_ts_data.irq_id, ts_irq_handler,
			irq_flags | IRQF_NO_SUSPEND, "ts", &g_ts_data);
	if (error) {
		TS_LOG_ERR("ts request_irq failed\n");
		goto err_unregister_suspend;
	}
	/*get_boot_into_recovery_flag need to be added later*/
	touch_recovery_mode = get_boot_into_recovery_flag();
	charge_flag = get_pd_charge_flag();

	/*get brightness info*/
	ts_get_brightness_info();

	/*do not do boot fw update on recovery mode*/
	TS_LOG_INFO("touch_recovery_mode is %d, charge_flag:%u\n", touch_recovery_mode, charge_flag);
	if (!touch_recovery_mode && !charge_flag) {
		error = try_update_firmware();
		if (error) {
			TS_LOG_ERR("return fail : %d\n", error);
			goto err_firmware_update;
		}
	}
	ts_send_roi_cmd(TS_ACTION_READ, NO_SYNC_TIMEOUT);	/*roi function set as default by TP firmware */
	ts_send_init_cmd();/*Send this cmd to make sure all the cmd in the init is called*/
#if defined(HUAWEI_CHARGER_FB)
	g_ts_data.charger_detect_notify.notifier_call =
	    charger_detect_notifier_callback;
	error =
	    hisi_charger_type_notifier_register(&g_ts_data.
						charger_detect_notify);
	if (error < 0) {
		TS_LOG_ERR("%s, hisi_charger_type_notifier_register failed\n",
			   __func__);
		g_ts_data.charger_detect_notify.notifier_call = NULL;
	} else {
		TS_LOG_INFO
		    ("%s, already registe charger_detect_notifier_callback\n",
		     __func__);
		ts_charger_detect_cmd(hisi_get_charger_type());
	}
#endif

	if (g_ts_data.chip_data->need_wd_check_status) {
		TS_LOG_INFO("This chip need watch dog to check status\n");
		INIT_WORK(&(g_ts_data.watchdog_work), ts_watchdog_work);
		setup_timer(&(g_ts_data.watchdog_timer), ts_watchdog_timer,
			    (unsigned long)(&g_ts_data));
		ts_start_wd_timer(&g_ts_data);
	}
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */
	set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
#endif
	error = NO_ERR;
	TS_LOG_INFO("ts_init called out\n");
	goto out;

err_firmware_update:
	free_irq(g_ts_data.irq_id, &g_ts_data);
err_unregister_suspend:
#if defined(CONFIG_FB)
	if (fb_unregister_client(&g_ts_data.fb_notify))
		TS_LOG_ERR("error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&g_ts_data.early_suspend);
#endif
#if defined(CONFIG_FB)
err_free_input_dev:
#endif
	input_unregister_device(input_dev);
err_free_dev:
	input_free_device(input_dev);
	misc_deregister(&g_aft_get_info_misc_device);
	misc_deregister(&g_aft_set_info_misc_device);
err_remove_sysfs_link:
	sysfs_remove_link(NULL, "touchscreen");
err_free_sysfs:
	sysfs_remove_group(&g_ts_data.ts_dev->dev.kobj, &ts_attr_group);
err_del_platform_dev:
	platform_device_del(g_ts_data.ts_dev);
err_put_platform_dev:
	platform_device_put(g_ts_data.ts_dev);
err_out:
	atomic_set(&g_ts_data.state, TS_UNINIT);
	wake_lock_destroy(&g_ts_data.ts_wake_lock);
out:
	TS_LOG_INFO("ts_init, g_ts_data.state : %d\n",
		    atomic_read(&g_ts_data.state));
	return error;
}

static int ts_creat_i2c_client(void)
{
	struct i2c_adapter *adapter = NULL;
	struct i2c_client *client = NULL;
	struct i2c_board_info board_info;

	memset(&board_info, 0, sizeof(struct i2c_board_info));
	strncpy(board_info.type, TS_DEV_NAME, I2C_NAME_SIZE);
	board_info.addr = I2C_DEFAULT_ADDR;
	board_info.flags = true;

	adapter = i2c_get_adapter(g_ts_data.bops->bus_id);
	if (!adapter) {
		TS_LOG_ERR("i2c_get_adapter failed\n");
		return -EIO;
	}

	client = i2c_new_device(adapter, &board_info);
	if (!client) {
		TS_LOG_ERR("i2c_new_device failed\n");
		return -EIO;
	}
	g_ts_data.client = client;
	i2c_set_clientdata(client, &g_ts_data);

#if defined (CONFIG_HUAWEI_DSM)
	if (!tp_dclient) {
		tp_dclient = dsm_register_client(&dsm_tp);
	}
#endif

	return NO_ERR;
}

static int ts_destory_i2c_client(void)
{
	TS_LOG_ERR("destory i2c device\n");
	i2c_unregister_device(g_ts_data.client);
	return NO_ERR;
}

static int ts_create_client(void)
{
	int error = -EINVAL;

	switch (g_ts_data.bops->btype) {
	case TS_BUS_I2C:
		TS_LOG_DEBUG("create ts's i2c device\n");
		error = ts_creat_i2c_client();
		break;
	case TS_BUS_SPI:
		TS_LOG_DEBUG("create ts's spi device\n");
		break;
	default:
		TS_LOG_ERR("unknown ts's device\n");
		break;
	}

	return error;
}

static int ts_destory_client(void)
{
	TS_LOG_ERR("destory touchscreen device\n");

	switch (g_ts_data.bops->btype) {
	case TS_BUS_I2C:
		TS_LOG_DEBUG("destory ts's i2c device\n");
		ts_destory_i2c_client();
		break;
	case TS_BUS_SPI:
		TS_LOG_DEBUG("destory ts's spi device\n");
		break;
	default:
		TS_LOG_ERR("unknown ts's device\n");
		break;
	}
	return NO_ERR;
}

/******************************************************************************
 Prototype       : lon ts_thread_bindtocpu
 Description     : °ó¶¨ts_thread  µ½CPU1~3 ÉÏÃæ
******************************************************************************/
void ts_thread_bindtocpu(void)
{
	long   ret;
	pid_t  target_pid;
	int     cpu;
	struct cpumask  orig_mask;
	struct cpumask  curr_mask;

	/* »ñÈ¡µ±Ç°Ïß³ÌµÄPid */
	target_pid = current->pid;

	/* »ñÈ¡µ±Ç°Ïß³ÌµÄaffinity */
	ret = sched_getaffinity(target_pid, &(orig_mask));
	if (ret < 0)
	{
		TS_LOG_ERR("warning: unable to get cpu affinity\n");
		return;
	}

	memset(&(curr_mask), 0, cpumask_size());

	/* ÉèÖÃµ±Ç°Ïß³ÌµÄaffinity */
	for_each_cpu(cpu, &(orig_mask))
	{
        /* È¥°ó¶¨CPU0 */
		if ((0 < cpu) && (cpumask_test_cpu(cpu, &(orig_mask))))
		{
			cpumask_set_cpu((unsigned int)cpu, &(curr_mask));
		}
	}

	if (0 == cpumask_weight(&(curr_mask)))
	{
		cpumask_set_cpu(0, &(curr_mask));
		return;
	}

	ret = sched_setaffinity(target_pid, &(curr_mask));
	if (ret < 0)
	{
		TS_LOG_ERR("warning: unable to set cpu affinity\n");
		 return;
	}

	 return;
}

static int ts_thread(void *p)
{
	static const struct sched_param param = {
		//.sched_priority = MAX_USER_RT_PRIO / 2,
		.sched_priority = 99,
	};
	smp_wmb();
	if (ts_init()) {
		TS_LOG_ERR("ts_init  failed\n");
		goto out;
	}
	memset(&ping_cmd_buff, 0, sizeof(struct ts_cmd_node));
	memset(&pang_cmd_buff, 0, sizeof(struct ts_cmd_node));
	smp_mb();
	sched_setscheduler(current, SCHED_RR, &param);

	if (strncmp(g_ts_data.product_name, "lon", 3)==0){
		ts_thread_bindtocpu();
	} //lon ts_thread run in CPU1~3 modified by fingerprint module

	if (g_ts_data.chip_data->should_check_tp_calibration_info) {
	    INIT_WORK(&check_tp_calibration_info_work, check_tp_calibration_info);
	    schedule_work(&check_tp_calibration_info_work);
	}

	while (ts_task_continue()) {
		while (!get_one_cmd(&ping_cmd_buff)) {	/*get one command*/
			ts_proc_command(&ping_cmd_buff);
			memset(&ping_cmd_buff, 0, sizeof(struct ts_cmd_node));
			memset(&pang_cmd_buff, 0, sizeof(struct ts_cmd_node));
		}
	}

	TS_LOG_ERR("ts thread stop\n");
	atomic_set(&g_ts_data.state, TS_UNINIT);
	disable_irq(g_ts_data.irq_id);
	ts_ic_shutdown();
	free_irq(g_ts_data.irq_id, &g_ts_data);
	sysfs_remove_group(&g_ts_data.ts_dev->dev.kobj, &ts_attr_group);
#if defined(CONFIG_FB)
	if (fb_unregister_client(&g_ts_data.fb_notify))
		TS_LOG_ERR("error occurred while unregistering fb_notifier.\n");
#elif defined(CONFIG_HAS_EARLYSUSPEND)
	unregister_early_suspend(&g_ts_data.early_suspend);
#endif
	input_unregister_device(g_ts_data.input_dev);
	input_free_device(g_ts_data.input_dev);
	misc_deregister(&g_aft_get_info_misc_device);
	misc_deregister(&g_aft_set_info_misc_device);
	platform_device_unregister(g_ts_data.ts_dev);
out:
	ts_destory_client();
#if defined(HUAWEI_CHARGER_FB)
	if (NULL != g_ts_data.charger_detect_notify.notifier_call) {
		hisi_charger_type_notifier_unregister(&g_ts_data.
						      charger_detect_notify);
		TS_LOG_INFO("hisi_charger_type_notifier_unregister called\n");
	}
#endif
	memset(&g_ts_data, 0, sizeof(struct ts_data));
	atomic_set(&g_ts_data.state, TS_UNINIT);
	TS_LOG_ERR("ts_thread exited\n");
	return NO_ERR;
}

static int __init ts_module_init(void)
{
	int error = NO_ERR;

	memset(&g_ts_data, 0, sizeof(struct ts_data));

	g_ts_data.dev_id = 0;

	error = get_ts_bus_info();
	if (error) {
		TS_LOG_ERR("get bus info failed :%d\n", error);
		memset(&g_ts_data, 0, sizeof(struct ts_data));
		goto out;
	}

	error = ts_create_client();
	if (error) {
		TS_LOG_ERR("create device failed :%d\n", error);
		goto out;
	}

	g_ts_data.ts_task =
	    kthread_create(ts_thread, &g_ts_data, "ts_thread:%d",
			   g_ts_data.dev_id);
	if (IS_ERR(g_ts_data.ts_task)) {
		TS_LOG_ERR("create ts_thread failed\n");
		ts_destory_client();
		memset(&g_ts_data, 0, sizeof(struct ts_data));
		error = -EINVAL;
		goto out;
	}
	/* Attention about smp_mb/rmb/wmb
	   Add these driver to avoid  data consistency problem
	   ts_thread/ts_probe/irq_handler/put_one_cmd/get_one_cmd
	   may run in different cpus and L1/L2 cache data consistency need
	   to conside. We use barrier to make sure data consistently
	 */
	smp_mb();
	wake_up_process(g_ts_data.ts_task);

out:
	return error;
}

static void __exit ts_module_exit(void)
{
	TS_LOG_INFO("ts_module_exit called here\n");
	if (g_ts_data.ts_task)
		kthread_stop(g_ts_data.ts_task);
#if defined (CONFIG_TEE_TUI)
	unregister_tui_driver("tp");
#endif
	return;
}

late_initcall(ts_module_init);
module_exit(ts_module_exit);
EXPORT_SYMBOL(tp_gpio_num);
MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei TouchScreen Driver");
MODULE_LICENSE("GPL");

#ifdef CONFIG_LLT_TEST
#include "huawei_touchscreen_static_llt.h"
#endif
#ifdef CONFIG_LLT_TEST
#include "huawei_touchscreen_static_llt.c"
#endif
