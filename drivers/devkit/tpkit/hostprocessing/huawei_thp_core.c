/*
 * Huawei Touchscreen Driver
 *
 * Copyright (C) 2017 Huawei Device Co.Ltd
 * License terms: GNU General Public License (GPL) version 2
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/spi/spi.h>
#include <linux/gpio.h>
#include <linux/delay.h>
#include <linux/notifier.h>
#include <linux/fb.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/uaccess.h>
#include <linux/of_gpio.h>
#include <linux/of.h>
#include <linux/regulator/consumer.h>
#include <linux/hwspinlock.h>
#include "hwspinlock_internal.h"
#include "../../lcdkit/lcdkit1.0/include/lcdkit_ext.h"
#include "huawei_thp.h"
#include "huawei_thp_mt_wrapper.h"
#include "huawei_thp_attr.h"
#if defined (CONFIG_LCD_KIT_DRIVER)
#include <lcd_kit_core.h>
#endif

#ifdef CONFIG_INPUTHUB_20
#include "contexthub_recovery.h"
#endif

#if CONFIG_HISI_BCI_BATTERY
#include <linux/power/hisi/hisi_bci_battery.h>
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#if defined(CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#if defined (CONFIG_TEE_TUI)
#include "tui.h"
#endif

#if defined(CONFIG_HUAWEI_TS_KIT_3_0)
#include "../3_0/trace-events-touch.h"
#else
#define trace_touch(x...)
#endif

struct thp_core_data *g_thp_core;
static u8 *spi_sync_tx_buf = NULL;
static u8 *spi_sync_rx_buf = NULL;

static bool g_thp_prox_enable;

#if defined (CONFIG_TEE_TUI)
struct thp_tui_data thp_tui_info;
EXPORT_SYMBOL(thp_tui_info);
#endif
#if defined(CONFIG_HUAWEI_DSM)
static struct dsm_dev dsm_thp = {
	.name = "dsm_tphostprocessing",
	.device_name = "TPHOSTPROCESSING",
	.ic_name = "syn",
	.module_name = "NNN",
	.fops = NULL,
	.buff_size = 1024,
};

struct dsm_client *dsm_thp_dclient;

#endif

#define THP_DEVICE_NAME	"huawei_thp"
#define THP_MISC_DEVICE_NAME "thp"
#if defined (CONFIG_LCD_KIT_DRIVER)
int thp_power_control_notify(enum lcd_kit_ts_pm_type pm_type, int timeout);

int ts_kit_ops_register(struct ts_kit_ops * ops);
struct ts_kit_ops thp_ops = {
	.ts_power_notify = thp_power_control_notify,
};
#endif
static int thp_spi_transfer_one_byte_bootloader(struct thp_core_data *const cd,
										   const char *const tx_buf,
										   char *const rx_buf,
										   const unsigned int len);

int thp_spi_sync(struct spi_device *spi, struct spi_message *message)
{
	int ret = 0;

	trace_touch(TOUCH_TRACE_SPI, TOUCH_TRACE_FUNC_IN, "thp");
	ret = spi_sync(spi, message);
	trace_touch(TOUCH_TRACE_SPI, TOUCH_TRACE_FUNC_OUT, "thp");

	return ret;
}

int thp_project_id_provider(char* project_id)
{
	return 0;
}
EXPORT_SYMBOL(thp_project_id_provider);

struct thp_core_data *thp_get_core_data(void)
{
	return g_thp_core;
}
EXPORT_SYMBOL(thp_get_core_data);
static void thp_wake_up_frame_waitq(struct thp_core_data *cd)
{
	cd->frame_waitq_flag = WAITQ_WAKEUP;
	wake_up_interruptible(&(cd->frame_waitq));
}
static int thp_pinctrl_get_init(struct thp_device *tdev);
static int thp_pinctrl_select_normal(struct thp_device *tdev);
static int thp_pinctrl_select_lowpower(struct thp_device *tdev);
int thp_parse_pinctrl_config(struct device_node *spi_cfg_node,struct thp_core_data *cd);

#define IS_INVAILD_POWER_ID(x) (x >= THP_POWER_ID_MAX)

static char *thp_power_name[THP_POWER_ID_MAX] = {
	"thp-iovdd",
	"thp-vcc",
};

static const char* thp_power_id2name(enum thp_power_id id)
{
	return !IS_INVAILD_POWER_ID(id) ? thp_power_name[id] : 0;
}

int thp_power_supply_get(enum thp_power_id power_id)
{
	struct thp_core_data *cd = thp_get_core_data();
	struct thp_power_supply *power;
	int ret = 0;

	if (IS_INVAILD_POWER_ID(power_id)) {
		THP_LOG_ERR("%s: invalid power id %d", __func__, power_id);
		return -EINVAL;
	}

	power = &cd->thp_powers[power_id];
	if (power->type == THP_POWER_UNUSED) {
		return 0;
	}
	if (power->use_count) {
		power->use_count++;
		return 0;
	}
	switch (power->type) {
	case THP_POWER_LDO:
		power->regulator = regulator_get(&cd->sdev->dev, thp_power_id2name(power_id));
		if (IS_ERR_OR_NULL(power->regulator)) {
			THP_LOG_ERR("%s:fail to get %s\n", __func__, thp_power_id2name(power_id));
			return -ENODEV;
		}

		ret = regulator_set_voltage(power->regulator, power->ldo_value, power->ldo_value);
		if (ret) {
			regulator_put(power->regulator);
			THP_LOG_ERR("%s:fail to set %s valude %d\n", __func__,
					thp_power_id2name(power_id), power->ldo_value);
			return ret;
		}
		break;
	case THP_POWER_GPIO:
		ret = gpio_request(power->gpio, thp_power_id2name(power_id));
		if (ret) {
			THP_LOG_ERR("%s:request gpio %d for %s failed\n", __func__,
					power->gpio, thp_power_id2name(power_id));
			return ret;
		}
		break;
	default:
		THP_LOG_ERR("%s: invalid power type %d\n", __func__, power->type);
		break;
	}
	power->use_count++;
	return 0;
}

int thp_power_supply_put(enum thp_power_id power_id)
{
	struct thp_core_data *cd = thp_get_core_data();
	struct thp_power_supply *power;

	if (IS_INVAILD_POWER_ID(power_id)) {
		THP_LOG_ERR("%s: invalid power id %d", __func__, power_id);
		return -EINVAL;
	}

	power = &cd->thp_powers[power_id];
	if (power->type == THP_POWER_UNUSED) {
		return 0;
	}
	if ((--power->use_count) > 0)
		return 0;

	switch (power->type) {
	case THP_POWER_LDO:
		// regulator_disable(power->regulator);
		regulator_put(power->regulator);
		break;
	case THP_POWER_GPIO:
		gpio_direction_output(power->gpio, 0);
		gpio_free(power->gpio);
		break;
	default:
		THP_LOG_ERR("%s: invalid power type %d\n", __func__, power->type);
		break;
	}
	return 0;
}

int thp_power_supply_ctrl(enum thp_power_id power_id, int status, unsigned int delay_ms)
{
	struct thp_core_data *cd = thp_get_core_data();
	struct thp_power_supply *power;
	int rc = 0;
	if (IS_INVAILD_POWER_ID(power_id)) {
		THP_LOG_ERR("%s: invalid power id %d", __func__, power_id);
		return -EINVAL;
	}

	power = &cd->thp_powers[power_id];
	if (power->type == THP_POWER_UNUSED) {
		return 0;
	}

	THP_LOG_INFO("%s:power %s %s\n", __func__, thp_power_id2name(power_id), status ? "on" : "off");

	if (!power->use_count) {
		THP_LOG_ERR("%s:regulator %s not gotten yet\n", __func__,
				thp_power_id2name(power_id));
		return -ENODEV;
	}
	switch (power->type) {
	case THP_POWER_LDO:
		rc =  status ? regulator_enable(power->regulator) :
			regulator_disable(power->regulator);
		break;
	case THP_POWER_GPIO:
		gpio_direction_output(power->gpio, status ? 1 : 0);
		break;
	default:
		THP_LOG_ERR("%s: invalid power type %d\n", __func__, power->type);
		break;
	}
	if (delay_ms)
		mdelay(delay_ms);
	return rc;
}
#define POWER_CONFIG_NAME_MAX 20
static int thp_parse_one_power(struct device_node *thp_node,
			struct thp_core_data *cd,
			int power_id)
{
	const char *power_name;
	char config_name[POWER_CONFIG_NAME_MAX] = {0};
	struct thp_power_supply *power;
	int rc;

	power_name = thp_power_id2name(power_id);
	power = &cd->thp_powers[power_id];

	rc = snprintf(config_name, POWER_CONFIG_NAME_MAX - 1, "%s-type", power_name);

	THP_LOG_ERR("%s:parse power: %s\n", __func__, config_name);

	rc = of_property_read_u32(thp_node, config_name, &power->type);
	if (rc || power->type == THP_POWER_UNUSED) {
		THP_LOG_INFO("%s: power %s type not config or 0, unused\n", __func__, config_name);
		return 0;
	}

	switch (power->type) {
	case THP_POWER_GPIO:
		snprintf(config_name, POWER_CONFIG_NAME_MAX - 1, "%s-gpio", power_name);
		power->gpio = of_get_named_gpio(thp_node, config_name, 0);
		if (!gpio_is_valid(power->gpio)) {
			THP_LOG_ERR("%s:failed to get %s\n", __func__, config_name);
			return -ENODEV;
		}
		break;
	case THP_POWER_LDO:
		snprintf(config_name, POWER_CONFIG_NAME_MAX - 1, "%s-value", power_name);
		rc = of_property_read_u32(thp_node, config_name, &power->ldo_value);
		if (rc) {
			THP_LOG_ERR("%s:failed to get %s\n", __func__, config_name);
			return rc;
		}
		break;
	default:
		THP_LOG_ERR("%s: invaild power type %d", __func__, power->type);
		break;
	}

	return 0;
}

static int thp_parse_power_config(struct device_node *thp_node,
			struct thp_core_data *cd)
{
	int rc;
	int i;

	for (i = 0; i < THP_POWER_ID_MAX; i++) {
		rc = thp_parse_one_power(thp_node, cd, i);
		if (rc)
			return rc;
	}

	return 0;
}

int is_valid_project_id(char *id)
{
	while(*id != '\0') {
		if(*id & BIT_MASK(7) || !isalnum(*id))
			return false;
		id++;
	}

	return true;
}


#define GET_HWLOCK_FAIL   0
int thp_bus_lock() {
	int ret = 0;
	unsigned long time = 0;
	unsigned long timeout = 0;
	struct thp_core_data *cd = thp_get_core_data();
	struct hwspinlock *hwlock = cd->hwspin_lock;

	mutex_lock(&cd->spi_mutex);
	if(!cd->use_hwlock) {
		return 0;
	}

	timeout = jiffies + msecs_to_jiffies(THP_GET_HARDWARE_TIMEOUT);

	do {
		ret = hwlock->bank->ops->trylock(hwlock);
		if (GET_HWLOCK_FAIL == ret) {
			time = jiffies;
			if (time_after(time, timeout)) {
				THP_LOG_ERR("%s:get hardware_mutex for completion timeout\n", __func__);
				return -ETIME;
			}
		}
	} while (GET_HWLOCK_FAIL == ret);

	return 0;
}

void thp_bus_unlock() {
	struct thp_core_data *cd = thp_get_core_data();
	struct hwspinlock *hwlock = cd->hwspin_lock;

	mutex_unlock(&cd->spi_mutex);
	if(cd->use_hwlock) {
		hwlock->bank->ops->unlock(hwlock);
	}

}

static int thp_setup_spi(struct thp_core_data *cd);
int thp_set_spi_max_speed(unsigned int speed)
{
	struct thp_core_data *cd = thp_get_core_data();
	int rc = 0;

	cd->sdev->max_speed_hz = speed;
	THP_LOG_INFO("%s:set max_speed_hz %d\n", __func__, speed);

	thp_bus_lock();
	if (thp_setup_spi(cd)) {
		THP_LOG_ERR("%s: set spi speed fail\n", __func__);
		rc = -EIO;
	}
	thp_bus_unlock();

	return rc;
}

static int thp_wait_frame_waitq(struct thp_core_data *cd)
{
	int t;

	cd->frame_waitq_flag = WAITQ_WAIT;

	/* if not use timeout*/
	if (!cd->timeout) {
		t = wait_event_interruptible(cd->frame_waitq,
				(cd->frame_waitq_flag == WAITQ_WAKEUP));
		return 0;
	}

	/* if use timeout*/
	t = wait_event_interruptible_timeout(cd->frame_waitq,
			(cd->frame_waitq_flag == WAITQ_WAKEUP),
			msecs_to_jiffies(cd->timeout));
	if (!IS_TMO(t))
		return 0;

	THP_LOG_ERR("%s: wait frame timed out, dmd code:%d\n",
			__func__, DSM_TPHOSTPROCESSING_DEV_STATUS_ERROR_NO);

#if defined(CONFIG_HUAWEI_DSM)
#ifdef THP_TIMEOUT_DMD
	if (!dsm_client_ocuppy(dsm_thp_dclient)) {
		dsm_client_record(dsm_thp_dclient,
			"irq_gpio:%d\tvalue:%d.\n\
			reset_gpio:%d\tvalue:%d.\n\
			THP_status:%d.\n",
			cd->gpios.irq_gpio,
			gpio_get_value(cd->gpios.irq_gpio),
			cd->gpios.rst_gpio,
			gpio_get_value(cd->gpios.rst_gpio),
			ETIMEDOUT);
		dsm_client_notify(dsm_thp_dclient,
			DSM_TPHOSTPROCESSING_DEV_STATUS_ERROR_NO);
	}
#endif
#endif

	return -ETIMEDOUT;
}

int thp_set_status(int type, int status)
{
	struct thp_core_data *cd = thp_get_core_data();

	mutex_lock(&cd->status_mutex);
	status ? __set_bit(type, (volatile unsigned long *)&cd->status) :
		__clear_bit(type, (volatile unsigned long *)&cd->status);
	mutex_unlock(&cd->status_mutex);

	thp_mt_wrapper_wakeup_poll();

	THP_LOG_INFO("%s:type=%d value=%d\n",
			__func__, type, status);
	return 0;
}

int thp_get_status(int type)
{
	struct thp_core_data *cd = thp_get_core_data();

	return test_bit(type, (unsigned long*)&cd->status);
}

u32 thp_get_status_all(void)
{
	struct thp_core_data *cd = thp_get_core_data();

	return cd->status;
}

static void thp_clear_frame_buffer(struct thp_core_data *cd)
{
	struct thp_frame *temp;
	struct list_head *pos, *n;

	if (list_empty(&cd->frame_list.list))
		return;

	list_for_each_safe(pos, n, &cd->frame_list.list) {
		temp = list_entry(pos, struct thp_frame, list);
		list_del(pos);
		kfree(temp);
	}

	cd->frame_count = 0;
}

static int thp_spi_transfer(struct thp_core_data *cd,
			char *tx_buf, char *rx_buf, unsigned int len)
{
	struct spi_message msg;
	struct spi_device *sdev = cd->sdev;

	struct spi_transfer xfer = {
		.tx_buf = tx_buf,
		.rx_buf = rx_buf,
		.len    = len,
	};
	int rc = 0;

	if (cd->suspended && (!cd->need_work_in_suspend)) {
		THP_LOG_ERR("%s - suspended\n", __func__);
		return 0;
	}

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	thp_bus_lock();
	thp_spi_cs_set(GPIO_HIGH);
	rc = thp_spi_sync(sdev, &msg);
	thp_bus_unlock();

	return rc;
}

/*
 * If irq is disabled/enabled, can not disable/enable again
 * disable - status 0; enable - status not 0
 */
static void thp_set_irq_status(struct thp_core_data *cd, int status)
{
	mutex_lock(&cd->irq_mutex);
	if (cd->irq_enabled != !!status) {
		status ? enable_irq(cd->irq) : disable_irq(cd->irq);
		cd->irq_enabled = !!status;
		THP_LOG_INFO("%s: %s irq\n", __func__,
				status ? "enable" : "disable");
	}
	mutex_unlock(&cd->irq_mutex);
};

static int thp_suspend(struct thp_core_data *cd)
{
	if (cd->suspended) {
		THP_LOG_INFO("%s: already suspended, return\n", __func__);
		return 0;
	}

	cd->suspended = true;
	if (cd->need_work_in_suspend) {
		THP_LOG_INFO("[Proximity_feature] %s: Enter prximity mode, no need suspend!\n",
			__func__);
		return 0;
	}
	if(1 == cd->support_pinctrl)
		thp_pinctrl_select_lowpower(cd->thp_dev);
	if (cd->open_count)
		thp_set_irq_status(cd, THP_IRQ_DISABLE);

	cd->thp_dev->ops->suspend(cd->thp_dev);

	return 0;
}

static int thp_resume(struct thp_core_data *cd)
{
	if (!cd->suspended) {
		THP_LOG_INFO("%s: already resumed, return\n", __func__);
		return 0;
	}

	cd->thp_dev->ops->resume(cd->thp_dev);

	/*
	 * clear rawdata frame buffer list
	 */
	mutex_lock(&cd->mutex_frame);
	thp_clear_frame_buffer(cd);
	mutex_unlock(&cd->mutex_frame);
	if(1 == cd->support_pinctrl)
		thp_pinctrl_select_normal(cd->thp_dev);

	if (cd->proximity_support == PROX_SUPPORT) {
		g_thp_prox_enable = cd->prox_cache_enable;
		THP_LOG_INFO("[Proximity_feature]%s:update g_thp_prox_enable to %d!\n",
				__func__, g_thp_prox_enable);
	}
	cd->suspended = false;

	return 0;
}
static int thp_lcdkit_notifier_callback(struct notifier_block* self,
			unsigned long event, void* data)
{
	struct thp_core_data *cd = thp_get_core_data();
	unsigned long pm_type = event;

	THP_LOG_DEBUG("%s: called by lcdkit, pm_type=%d\n", __func__, pm_type);

	switch (pm_type) {
	case LCDKIT_TS_EARLY_SUSPEND:
		if (cd->proximity_support != PROX_SUPPORT) {
			THP_LOG_INFO("%s: early suspend\n", __func__);
			thp_set_status(THP_STATUS_POWER, THP_SUSPEND);
		} else {
			THP_LOG_INFO("%s: early suspend! g_thp_prox_enable = %d\n",
				__func__, g_thp_prox_enable);
			cd->need_work_in_suspend = g_thp_prox_enable;
			cd->prox_cache_enable = g_thp_prox_enable;
			if (cd->need_work_in_suspend)
				thp_set_status(THP_STATUS_AFE_PROXIMITY,
						THP_PROX_ENTER);
			else
				thp_set_status(THP_STATUS_POWER, THP_SUSPEND);
		}
		break;

	case LCDKIT_TS_SUSPEND_DEVICE :
		THP_LOG_DEBUG("%s: suspend\n", __func__);
		thp_clean_fingers();
		break;

	case LCDKIT_TS_BEFORE_SUSPEND :
		THP_LOG_INFO("%s: before suspend\n", __func__);
		thp_suspend(cd);
		break;

	case LCDKIT_TS_RESUME_DEVICE :
		THP_LOG_INFO("%s: resume\n", __func__);
		thp_resume(cd);
		break;

	case LCDKIT_TS_AFTER_RESUME:
		if (cd->proximity_support != PROX_SUPPORT) {
			THP_LOG_INFO("%s: after resume\n", __func__);
			thp_set_status(THP_STATUS_POWER, THP_RESUME);
		} else {
			THP_LOG_INFO("%s: after resume! need_work_in_suspend = %d\n",
				__func__, cd->need_work_in_suspend);
			if (cd->need_work_in_suspend)
				thp_set_status(THP_STATUS_AFE_PROXIMITY,
						THP_PROX_EXIT);
			else
				thp_set_status(THP_STATUS_POWER, THP_RESUME);
		}
		break;

	default :
		break;
	}

	return 0;
}

#if defined (CONFIG_LCD_KIT_DRIVER)
int thp_power_control_notify(enum lcd_kit_ts_pm_type pm_type, int timeout)
{
	struct thp_core_data *cd = thp_get_core_data();
	THP_LOG_DEBUG("%s: called by lcdkit, pm_type=%d\n", __func__, pm_type);

	switch (pm_type) {
	case TS_EARLY_SUSPEND:
		THP_LOG_INFO("%s: early suspend\n", __func__);
		thp_set_status(THP_STATUS_POWER, THP_SUSPEND);
		break;

	case TS_SUSPEND_DEVICE :
		THP_LOG_INFO("%s: suspend\n", __func__);
		thp_clean_fingers();
		break;

	case TS_BEFORE_SUSPEND :
		THP_LOG_INFO("%s: before suspend\n", __func__);
		thp_suspend(cd);
		break;

	case TS_RESUME_DEVICE :
		THP_LOG_INFO("%s: resume\n", __func__);
		thp_resume(cd);
		break;

	case TS_AFTER_RESUME:
		THP_LOG_INFO("%s: after resume\n", __func__);
		thp_set_status(THP_STATUS_POWER, THP_RESUME);
		break;

	default :
		break;
	}

	return 0;
}
#endif

static int thp_open(struct inode *inode, struct file *filp)
{
	struct thp_core_data *cd = thp_get_core_data();

	THP_LOG_INFO("%s: called\n", __func__);

	mutex_lock(&cd->thp_mutex);
	if (cd->open_count) {
		THP_LOG_ERR("%s: dev have be opened\n", __func__);
		mutex_unlock(&cd->thp_mutex);
		return -EBUSY;
	}

	cd->open_count++;
	mutex_unlock(&cd->thp_mutex);
	cd->reset_flag = 0;//current isn't in reset status
	cd->get_frame_block_flag = THP_GET_FRAME_BLOCK;

	cd->frame_size = THP_MAX_FRAME_SIZE;
#ifdef THP_NOVA_ONLY
	cd->frame_size = NT_MAX_FRAME_SIZE;
#endif
	cd->timeout = THP_DEFATULT_TIMEOUT_MS;

	/*Daemon default is  0, setting  to 1 will trigger daemon to init or restore the status.*/
	__set_bit(THP_STAUTS_WINDOW_UPDATE, (volatile unsigned long *)&cd->status);
	__set_bit(THP_STAUTS_TOUCH_SCENE, (volatile unsigned long *)&cd->status);

	THP_LOG_INFO("%s: cd->status = 0x%x\n", __func__,cd->status);

	thp_clear_frame_buffer(cd);

	/* restore spi config */
	thp_set_spi_max_speed(cd->spi_config.max_speed_hz);

	return 0;
}

static int thp_release(struct inode *inode, struct file *filp)
{
	struct thp_core_data *cd = thp_get_core_data();

	THP_LOG_INFO("%s: called\n", __func__);

	mutex_lock(&cd->thp_mutex);
	cd->open_count--;
	if (cd->open_count < 0) {
		THP_LOG_ERR("%s: abnormal release\n", __func__);
		cd->open_count = 0;
	}
	mutex_unlock(&cd->thp_mutex);

	thp_wake_up_frame_waitq(cd);
	thp_set_irq_status(cd, THP_IRQ_DISABLE);

	return 0;
}

static int  thp_spi_sync_alloc_mem(void)
{
	if(!spi_sync_rx_buf && !spi_sync_tx_buf ){
		spi_sync_rx_buf = kzalloc(THP_SYNC_DATA_MAX, GFP_KERNEL);
		if(!spi_sync_rx_buf){
			THP_LOG_ERR("%s:spi_sync_rx_buf request memory fail,sync_data.size = %d\n", __func__);
			goto exit;
		}
		spi_sync_tx_buf = kzalloc(THP_SYNC_DATA_MAX, GFP_KERNEL);
		if(!spi_sync_tx_buf){
			THP_LOG_ERR("%s:spi_sync_tx_buf request memory fail,sync_data.size = %d\n", __func__);
			kfree(spi_sync_rx_buf);
			spi_sync_rx_buf = NULL;
			goto exit;
		}
	}
	return  0;

exit :
	return  -ENOMEM;
}

static long thp_ioctl_spi_sync(void __user *data)
{
	struct thp_core_data *cd = thp_get_core_data();
	int rc = 0;
	u8 *tx_buf = NULL;
	u8 *rx_buf = NULL;
	struct thp_ioctl_spi_sync_data sync_data;

	THP_LOG_DEBUG("%s: called\n", __func__);

	if (cd->suspended && (!cd->need_work_in_suspend)) {
		THP_LOG_INFO("[%s] suspended return!\n", __func__);
		return 0;
	}

#if defined (CONFIG_TEE_TUI)
	if(thp_tui_info.enable)
		return 0;
#endif

	if (copy_from_user(&sync_data, data,
				sizeof(struct thp_ioctl_spi_sync_data))) {
		THP_LOG_ERR("Failed to copy_from_user().\n");
		return -EFAULT;
	}

	if (sync_data.size > THP_SYNC_DATA_MAX) {
		THP_LOG_ERR("sync_data.size out of range.\n");
		return -EINVAL;
	}

	if(cd->need_huge_memory_in_spi){
		rc = thp_spi_sync_alloc_mem();
		if(!rc ){
			rx_buf = spi_sync_rx_buf;
			tx_buf = spi_sync_tx_buf;
		}
		else{
			THP_LOG_ERR("%s:buf request memory fail.\n", __func__);
			goto exit;
		}
	}
	else{
		rx_buf = kzalloc(sync_data.size, GFP_KERNEL);
		tx_buf = kzalloc(sync_data.size, GFP_KERNEL);
		if (!rx_buf || !tx_buf) {
			THP_LOG_ERR("%s:buf request memory fail,sync_data.size = %d\n", __func__,sync_data.size);
			goto exit;
		}
	}
	rc = copy_from_user(tx_buf, sync_data.tx, sync_data.size);
	if (rc) {
		THP_LOG_ERR("%s:copy in buff fail\n", __func__);
		goto exit;
	}

	rc =  thp_spi_transfer(cd, tx_buf, rx_buf, sync_data.size);
	if (rc) {
		THP_LOG_ERR("%s: transfer error, ret = %d\n", __func__, rc);
		goto exit;
	}

	if (sync_data.rx) {
		rc = copy_to_user(sync_data.rx, rx_buf, sync_data.size);
		if (rc) {
			THP_LOG_ERR("%s:copy out buff fail\n", __func__);
			goto exit;
		}
	}

exit:
	if(!cd->need_huge_memory_in_spi){
		if(rx_buf){
			kfree(rx_buf);
			rx_buf = NULL;
		}
		if(tx_buf){
			kfree(tx_buf);
			tx_buf = NULL;
		}
	}
	return rc;
}

static long thp_ioctl_finish_notify(unsigned long arg)
{
	THP_LOG_INFO("%s: called\n", __func__);
	struct thp_core_data *cd = thp_get_core_data();
	unsigned long event_type = arg;
	int rc = 0;

	switch(event_type) {
		case THP_AFE_NOTIFY_FW_UPDATE :
			rc = cd->thp_dev->ops->afe_notify ?
				cd->thp_dev->ops->afe_notify(cd->thp_dev, event_type) : 0;
			break;
		default :
			THP_LOG_ERR("%s: illegal event type\n", __func__);
			rc = -EINVAL;
	}
	return rc;
}

static long thp_ioctl_get_frame_count(unsigned long arg)
{
	struct thp_core_data *cd = thp_get_core_data();
	u32 __user *frame_cnt = (u32 *)arg;

	if (cd->frame_count) {
		THP_LOG_INFO("%s:frame_cnt=%d\n", __func__, cd->frame_count);
	}

	if (frame_cnt == NULL) {
		THP_LOG_ERR("%s: input parameter null\n", __func__);
		return -EINVAL;
	}

	if(copy_to_user(frame_cnt, &cd->frame_count, sizeof(u32))) {
		THP_LOG_ERR("%s:copy frame_cnt failed\n", __func__);
		return -EFAULT;
	}

	return 0;
}

static long thp_ioctl_clear_frame_buffer(void)
{
	struct thp_core_data *cd = thp_get_core_data();

	if (cd->frame_count == 0) {
		return 0;
	}

	THP_LOG_INFO("%s called\n", __func__);
	mutex_lock(&cd->mutex_frame);
	thp_clear_frame_buffer(cd);
	mutex_unlock(&cd->mutex_frame);
	return 0;
}

static long thp_ioctl_get_frame(unsigned long arg, unsigned int f_flag)
{
	long rc = 0;
	struct thp_core_data *cd = thp_get_core_data();
	void __user *argp = (void __user *)arg;
	struct thp_ioctl_get_frame_data data;

	trace_touch(TOUCH_TRACE_GET_FRAME, TOUCH_TRACE_FUNC_IN, "thp");

	if (!arg) {
		THP_LOG_ERR("%s: input parameter null\n", __func__);
		return -EINVAL;
	}

	if (cd->suspended && (!cd->need_work_in_suspend)) {
		THP_LOG_INFO("%s: drv suspended\n", __func__);
		return -ETIMEDOUT;
	}

	if (copy_from_user(&data, argp,
			sizeof(struct thp_ioctl_get_frame_data))) {
		THP_LOG_ERR("Failed to copy_from_user .\n");
		return -EFAULT;
	}

	if (data.buf == 0 || data.size == 0 ||
		data.size > THP_MAX_FRAME_SIZE || data.tv == 0) {
		THP_LOG_ERR("%s:input buf invalid\n", __func__);
		return -EINVAL;
	}
	cd->frame_size = data.size;

	thp_set_irq_status(cd, THP_IRQ_ENABLE);
	if (list_empty(&cd->frame_list.list) && cd->get_frame_block_flag) {
		if (thp_wait_frame_waitq(cd))
			rc = -ETIMEDOUT;
	}

	mutex_lock(&cd->mutex_frame);
	if (!list_empty(&cd->frame_list.list)) {
		struct thp_frame *temp;

		temp = list_first_entry(&cd->frame_list.list,
				struct thp_frame, list);

		if (copy_to_user(data.buf, temp->frame, cd->frame_size)) {
			THP_LOG_ERR("Failed to copy_to_user().\n");
			rc = -EFAULT;
			goto out;
		}

		if (copy_to_user(data.tv, &(temp->tv),
					sizeof(struct timeval))) {
			THP_LOG_ERR("Failed to copy_to_user().\n");
			rc = -EFAULT;
			goto out;
		}

		list_del(&temp->list);
		kfree(temp);
		cd->frame_count--;
		rc = 0;
	} else {
		THP_LOG_ERR("%s:no frame\n", __func__);
		/*
		 * When wait timeout, try to get data.
		 * If timeout and no data, return -ETIMEDOUT
		 */
		if (rc != -ETIMEDOUT)
			rc = -ENODATA ;
	}
out:
	mutex_unlock(&cd->mutex_frame);
	trace_touch(TOUCH_TRACE_GET_FRAME, TOUCH_TRACE_FUNC_OUT, rc ? "no frame" : "with frame");
	return rc;
}

static long thp_ioctl_reset(unsigned long reset)
{
	struct thp_core_data *cd = thp_get_core_data();

	THP_LOG_INFO("%s:set reset status %d\n", __func__, reset);

	gpio_set_value(cd->gpios.rst_gpio, !!reset);

	cd->frame_waitq_flag = WAITQ_WAIT;
	cd->reset_flag = !reset;

	return 0;
}

static long thp_ioctl_set_timeout(unsigned long arg)
{
	struct thp_core_data *ts = thp_get_core_data();
	unsigned int timeout_ms = min((unsigned int)arg, THP_WAIT_MAX_TIME);

	THP_LOG_INFO("set wait time %d ms.(current %dms)\n", timeout_ms, ts->timeout);

	if (timeout_ms != ts->timeout) {
		ts->timeout = timeout_ms;
		thp_wake_up_frame_waitq(ts);
	}

	return 0;
}

static long thp_ioctl_set_block(unsigned long arg)
{
	struct thp_core_data *ts = thp_get_core_data();
	unsigned int block_flag = arg;

	if (block_flag)
		ts->get_frame_block_flag = THP_GET_FRAME_BLOCK;
	else
		ts->get_frame_block_flag = THP_GET_FRAME_NONBLOCK;

	THP_LOG_INFO("%s:set block %d\n", __func__, block_flag);

	thp_wake_up_frame_waitq(ts);
	return 0;
}


static long thp_ioctl_set_irq(unsigned long arg)
{
	struct thp_core_data *ts = thp_get_core_data();
	unsigned int irq_en = (unsigned int)arg;
	thp_set_irq_status(ts, irq_en);
	return 0;
}

static long thp_ioctl_get_irq_gpio_value(unsigned long arg)
{
	struct thp_core_data *cd = thp_get_core_data();
	u32 __user *out_value = (u32 *)arg;
	u32 value = 0;

	value = gpio_get_value(cd->gpios.irq_gpio);
	if (copy_to_user(out_value, &value, sizeof(u32))) {
		THP_LOG_ERR("%s:copy value fail\n", __func__);
		return -EFAULT;
	}
	return 0;
}

static long thp_ioctl_set_spi_speed(unsigned long arg)
{
	struct thp_core_data *cd = thp_get_core_data();
	unsigned long max_speed_hz = arg;

	if (max_speed_hz == cd->sdev->max_speed_hz)
		return 0;

	return thp_set_spi_max_speed(max_speed_hz);
}

static long thp_ioctl_spi_sync_ssl_bl(void __user *data)
{
	int rc = 0;
	u8 *tx_buf = NULL;
	u8 *rx_buf = NULL;
	struct thp_ioctl_spi_sync_data sync_data;
	struct thp_core_data *cd = thp_get_core_data();

	THP_LOG_DEBUG("%s: called\n", __func__);

	if (!data) {
		THP_LOG_ERR("%s: data null\n", __func__);
		return -EINVAL;
	}
	if (cd->suspended)
		return 0;

#if defined (CONFIG_TEE_TUI)
	if(thp_tui_info.enable)
		return 0;
#endif

	if (copy_from_user(&sync_data, data,
				sizeof(struct thp_ioctl_spi_sync_data))) {
		THP_LOG_ERR("Failed to copy_from_user().\n");
		return -EFAULT;
	}

	if (sync_data.size > THP_SYNC_DATA_MAX || 0 == sync_data.size) {
		THP_LOG_ERR("sync_data.size out of range.\n");
		return -EINVAL;
	}

	rx_buf = kzalloc(sync_data.size, GFP_KERNEL);
	tx_buf = kzalloc(sync_data.size, GFP_KERNEL);
	if (!rx_buf || !tx_buf) {
		THP_LOG_ERR("%s:buf request memory fail,sync_data.size = %d\n", __func__,sync_data.size);
		goto exit;
	}

	rc = copy_from_user(tx_buf, sync_data.tx, sync_data.size);
	if (rc) {
		THP_LOG_ERR("%s:copy in buff fail\n", __func__);
		goto exit;
	}

	rc =  thp_spi_transfer_one_byte_bootloader(cd, tx_buf, rx_buf, sync_data.size);

	if (rc) {
		THP_LOG_ERR("%s: transfer error, ret = %d\n", __func__, rc);
		goto exit;
	}

	if (sync_data.rx) {
		rc = copy_to_user(sync_data.rx, rx_buf, sync_data.size);
		if (rc) {
			THP_LOG_ERR("%s:copy out buff fail\n", __func__);
			goto exit;
		}
	}

exit:
	if(rx_buf){
		kfree(rx_buf);
		rx_buf = NULL;
	}
	if(tx_buf){
		kfree(tx_buf);
		tx_buf = NULL;
	}
	return rc;
}

static int thp_spi_transfer_one_byte_bootloader(struct thp_core_data *const cd,
						const char *const tx_buf,
						char *const rx_buf,
						const unsigned int buf_len)
{
	unsigned int idx = 0;
	struct spi_message msg;
	char tcb = 0, rcb = 0;
	int rc = 0;
	if (!cd || !tx_buf || !rx_buf) {
		THP_LOG_ERR("%s: point null\n", __func__);
		return -EINVAL;
	}
	struct spi_device *const sdev = cd->sdev;
	struct spi_transfer xfer = {
		.tx_buf = &tcb,
		.rx_buf = &rcb,
		.len    = 1,
	};

	if (cd->suspended){
		THP_LOG_ERR("%s - suspended\n", __func__);
		return 0;
	}

	spi_message_init(&msg);
	spi_message_add_tail(&xfer, &msg);

	mutex_lock(&cd->spi_mutex);
	for( idx = 0; idx < buf_len; ++idx) {
		tcb = tx_buf[idx];
		rc = spi_sync(sdev, &msg);
		if (rc != 0){
			THP_LOG_ERR("%s - Doing individual byte transfer FAILED!\n", __func__);
			break;
		}
		rx_buf[idx] = rcb;
	}
	mutex_unlock(&cd->spi_mutex);

	return rc;
}

static long thp_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	long ret;

	switch (cmd) {
	case THP_IOCTL_CMD_GET_FRAME:
		ret = thp_ioctl_get_frame(arg, filp->f_flags);
		break;

	case THP_IOCTL_CMD_RESET:
		ret = thp_ioctl_reset(arg);
		break;

	case THP_IOCTL_CMD_SET_TIMEOUT:
		ret = thp_ioctl_set_timeout(arg);
		break;

	case THP_IOCTL_CMD_SPI_SYNC:
		ret = thp_ioctl_spi_sync((void __user *)arg);
		break;

	case THP_IOCTL_CMD_FINISH_NOTIFY:
		ret = thp_ioctl_finish_notify(arg);
		break;
	case THP_IOCTL_CMD_SET_BLOCK:
		ret = thp_ioctl_set_block(arg);
		break;

	case THP_IOCTL_CMD_SET_IRQ:
		ret = thp_ioctl_set_irq(arg);
		break;

	case THP_IOCTL_CMD_GET_FRAME_COUNT:
		ret = thp_ioctl_get_frame_count(arg);
		break;
	case THP_IOCTL_CMD_CLEAR_FRAME_BUFFER:
		ret = thp_ioctl_clear_frame_buffer();
		break;

	case THP_IOCTL_CMD_GET_IRQ_GPIO_VALUE:
		ret = thp_ioctl_get_irq_gpio_value(arg);
		break;

	case THP_IOCTL_CMD_SET_SPI_SPEED:
		ret = thp_ioctl_set_spi_speed(arg);
		break;
	case THP_IOCTL_CMD_SPI_SYNC_SSL_BL:
		ret = thp_ioctl_spi_sync_ssl_bl((void __user *) arg);
		break;
	default:
		THP_LOG_ERR("cmd unknown.\n");
		ret = 0;
	}

	return ret;
}

static const struct file_operations g_thp_fops = {
	.owner = THIS_MODULE,
	.open = thp_open,
	.release = thp_release,
	.unlocked_ioctl = thp_ioctl,
};

static struct miscdevice g_thp_misc_device = {
	.minor = MISC_DYNAMIC_MINOR,
	.name = THP_MISC_DEVICE_NAME,
	.fops = &g_thp_fops,
};

static void thp_copy_frame(struct thp_core_data *cd)
{
	struct thp_frame *temp;
	static int pre_frame_count = -1;

	mutex_lock(&(cd->mutex_frame));

	/* check for max limit */
	if (cd->frame_count >= THP_LIST_MAX_FRAMES) {
		if (cd->frame_count != pre_frame_count)
			THP_LOG_ERR("ts frame buferr full start,frame_count:%d\n",cd->frame_count);

		temp = list_first_entry(&cd->frame_list.list,
						struct thp_frame, list);
		list_del(&temp->list);
		kfree(temp);
		pre_frame_count = cd->frame_count;
		cd->frame_count--;
	} else if (pre_frame_count >= THP_LIST_MAX_FRAMES) {
		THP_LOG_ERR("%s:ts frame buf full exception restored,frame_count:%d\n",__func__,cd->frame_count);
		pre_frame_count = cd->frame_count;
	}

	temp = kzalloc(sizeof(struct thp_frame), GFP_KERNEL);
	if (!temp) {
		THP_LOG_ERR("%s:memory out\n", __func__);
		mutex_unlock(&(cd->mutex_frame));
		return;
	}

	memcpy(temp->frame, cd->frame_read_buf, cd->frame_size);
	do_gettimeofday(&(temp->tv));
	list_add_tail(&(temp->list), &(cd->frame_list.list));
	cd->frame_count++;
	mutex_unlock(&(cd->mutex_frame));
}

static irqreturn_t thp_irq_thread(int irq, void *dev_id)
{
	struct thp_core_data *cd = dev_id;
	u8 *read_buf = (u8 *)cd->frame_read_buf;
	int rc;
	trace_touch(TOUCH_TRACE_IRQ_BOTTOM, TOUCH_TRACE_FUNC_IN, "thp");
	if ((cd->reset_flag || cd->suspended) && (!cd->need_work_in_suspend)) {
		THP_LOG_ERR("%s: ignore this irq.\n", __func__);
		return IRQ_HANDLED;
	}
	disable_irq_nosync(cd->irq);
	/* get frame */
	rc = cd->thp_dev->ops->get_frame(cd->thp_dev, read_buf, cd->frame_size);
	if (rc) {
		THP_LOG_ERR("%s: failed to read frame (%d)\n", __func__, rc);
		goto exit;
	}

	trace_touch(TOUCH_TRACE_DATA2ALGO, TOUCH_TRACE_FUNC_IN, "thp");
	thp_copy_frame(cd);
	thp_wake_up_frame_waitq(cd);
	trace_touch(TOUCH_TRACE_DATA2ALGO, TOUCH_TRACE_FUNC_OUT, "thp");

exit:
	enable_irq(cd->irq);
	trace_touch(TOUCH_TRACE_IRQ_BOTTOM, TOUCH_TRACE_FUNC_OUT, "thp");
	return IRQ_HANDLED;
}

void thp_spi_cs_set(u32 control)
{
	int rc = 0;
	struct thp_core_data *cd = thp_get_core_data();

	if (!cd) {
		THP_LOG_ERR("%s:no driver data", __func__);
		return;
	}

	if (control == SSP_CHIP_SELECT) {
		rc = gpio_direction_output(cd->gpios.cs_gpio, control);
		ndelay(cd->thp_dev->timing_config.spi_sync_cs_low_delay_ns);
	} else {
		rc = gpio_direction_output(cd->gpios.cs_gpio, control);
		ndelay(cd->thp_dev->timing_config.spi_sync_cs_hi_delay_ns);
	}

	if (rc < 0)
		THP_LOG_ERR("%s:fail to set gpio cs", __func__);

}
EXPORT_SYMBOL(thp_spi_cs_set);
#if CONFIG_HISI_BCI_BATTERY
static int thp_charger_detect_notifier_callback(struct notifier_block *self,
					unsigned long event, void *data)
{
	int charger_stat_before = thp_get_status(THP_STATUS_CHARGER);
	int charger_state_new = charger_stat_before;
	THP_LOG_DEBUG("%s called, charger type: %d\n", __func__, event);

	switch (event) {
	case VCHRG_START_USB_CHARGING_EVENT :
	case VCHRG_START_AC_CHARGING_EVENT:
	case VCHRG_START_CHARGING_EVENT:
		charger_state_new = 1;
		break;
	case VCHRG_STOP_CHARGING_EVENT:
		charger_state_new = 0;
		break;
	default:
		break;
	}

	if(charger_stat_before != charger_state_new){
		THP_LOG_INFO("%s, set new status: %d\n", __func__, charger_state_new);
		thp_set_status(THP_STATUS_CHARGER, charger_state_new);
	}

	return 0;
}
#endif

#define THP_PROJECTID_LEN 9
#define THP_PROJECTID_PRODUCT_NAME_LEN 4
#define THP_PROJECTID_IC_NAME_LEN 2
#define THP_PROJECTID_VENDOR_NAME_LEN 3

struct thp_vendor {
	char *vendor_id;
	char *vendor_name;
};

struct thp_ic_name {
	char *ic_id;
	char *ic_name;
};

static struct thp_vendor thp_vendor_table[] = {
	{"000", "ofilm"},
	{"030", "mutto"},
	{"080", "jdi"},
	{"100", "lg"},
	{"110", "tianma"},
	{"120", "cmi"},
	{"130", "boe"},
	{"140", "ctc"},
	{"160", "sharp"},
	{"170", "auo"},
	{"270", "tcl"},
};

static struct thp_ic_name thp_ic_table[] = {
	{"32", "rohm"},
	{"47", "rohm"},
	{"49", "novatech"},
	{"59", "himax"},
	{"60", "himax"},
	{"61", "himax"},
	{"62", "synaptics"},
	{"65", "novatech"},
	{"66", "himax"},
	{"69", "synaptics"},
	{"71", "novatech"},
	{"77", "novatech"},
	{"86", "synaptics"},
	{"88", "novatech"},
};

static int thp_projectid_to_vender_name(char *project_id,
				char **vendor_name)
{
	char temp_buf[THP_PROJECTID_LEN + 1] = {'0'};
	int i;

	strncpy(temp_buf, project_id + THP_PROJECTID_PRODUCT_NAME_LEN +
		THP_PROJECTID_IC_NAME_LEN, THP_PROJECTID_VENDOR_NAME_LEN);

	for (i = 0; i < ARRAY_SIZE(thp_vendor_table); i++) {
		if (!strncmp(thp_vendor_table[i].vendor_id, temp_buf,
			strlen(thp_vendor_table[i].vendor_id))) {
			*vendor_name = thp_vendor_table[i].vendor_name;
			return 0;
		}
	}

	return -ENODATA;
}

static int thp_projectid_to_ic_name(char *project_id,
				char **ic)
{
	char temp_buf[THP_PROJECTID_LEN + 1] = {'0'};
	int i;

	strncpy(temp_buf, project_id + THP_PROJECTID_PRODUCT_NAME_LEN,
			THP_PROJECTID_IC_NAME_LEN);

	for (i = 0; i < ARRAY_SIZE(thp_ic_table); i++) {
		if (!strncmp(thp_ic_table[i].ic_id, temp_buf,
			strlen(thp_ic_table[i].ic_id))) {
			*ic = thp_ic_table[i].ic_name;
			return 0;
		}
	}

	return -ENODATA;
}

static int thp_init_chip_info(struct thp_core_data *cd)
{
	int rc;
#if defined (CONFIG_LCD_KIT_DRIVER)
	struct lcd_kit_ops *tp_ops = lcd_kit_get_ops();
#endif

	if (cd->is_udp) {
		rc = hostprocessing_get_project_id_for_udp(cd->project_id);
	}else{
#ifndef CONFIG_LCD_KIT_DRIVER
		rc = hostprocessing_get_project_id(cd->project_id);
#else
		if(tp_ops && tp_ops->get_project_id) {
			rc = tp_ops->get_project_id(cd->project_id);
		}else{
			rc = -EINVAL;
			THP_LOG_ERR("%s:get lcd_kit_get_ops fail\n", __func__);
		}
#endif
	}
	if (rc)
		THP_LOG_ERR("%s:get project id form LCD fail\n", __func__);
	else
		THP_LOG_INFO("%s:project id :%s\n", __func__, cd->project_id);

	cd->project_id[THP_PROJECT_ID_LEN] = '\0';

	rc = thp_projectid_to_vender_name(cd->project_id, (char **)&cd->vendor_name);
	if (rc)
		THP_LOG_INFO("%s:vendor name parse fail\n", __func__);

	rc = thp_projectid_to_ic_name(cd->project_id, (char **)&cd->ic_name);
	if (rc)
		THP_LOG_INFO("%s:ic name parse fail\n", __func__);
	return rc;
}

static int thp_setup_irq(struct thp_core_data *cd)
{
	int rc = 0;
	int irq = 0;
	unsigned long irq_flag_type;
	u32 current_trigger_mode = 0;
	if(!cd ){
		THP_LOG_ERR("%s: thp_core_data is  null\n", __func__);
		return -EINVAL;
	}

	irq = gpio_to_irq(cd->gpios.irq_gpio);

	/*
	 * IRQF_TRIGGER_RISING  0x00000001
	 * IRQF_TRIGGER_FALLING 0x00000002
	 * IRQF_TRIGGER_HIGH    0x00000004
	 * IRQF_TRIGGER_LOW 0x00000008
	 * IRQF_NO_SUSPEND  0x00004000
	 */
	current_trigger_mode =  cd->irq_flag;
	THP_LOG_INFO("[%s] current_trigger_mode->0x%x\n",
		__func__, current_trigger_mode);
	irq_flag_type = IRQF_ONESHOT | current_trigger_mode;
	rc = request_threaded_irq(irq, NULL,
				thp_irq_thread, irq_flag_type,
				"thp", cd);
	if (rc) {
		THP_LOG_ERR("%s: request irq fail\n", __func__);
		return rc;
	}
	mutex_lock(&cd->irq_mutex);
	disable_irq(irq);
	cd->irq_enabled = false;
	mutex_unlock(&cd->irq_mutex);
	THP_LOG_INFO("%s: disable irq\n", __func__);
	cd->irq = irq;

	return 0;
}

static int thp_setup_gpio(struct thp_core_data *cd)
{
	int rc;

	THP_LOG_INFO("%s: called\n", __func__);

	rc = gpio_request(cd->gpios.rst_gpio, "thp_reset");
	if (rc) {
		THP_LOG_ERR("%s:gpio_request(%d) failed\n", __func__,
				cd->gpios.rst_gpio);
		return rc;
	}

	rc = gpio_request(cd->gpios.cs_gpio, "thp_cs");
	if (rc) {
		THP_LOG_ERR("%s:gpio_request(%d) failed\n", __func__,
				cd->gpios.cs_gpio);
		gpio_free(cd->gpios.rst_gpio);
		return rc;
	}
	gpio_direction_output(cd->gpios.cs_gpio, GPIO_HIGH);
	THP_LOG_INFO("%s:set cs gpio(%d) deault hi\n", __func__,
				cd->gpios.cs_gpio);

	rc = gpio_request(cd->gpios.irq_gpio, "thp_int");
	if (rc) {
		THP_LOG_ERR("%s: irq gpio(%d) request failed\n", __func__,
				cd->gpios.irq_gpio);
		gpio_free(cd->gpios.rst_gpio);
		gpio_free(cd->gpios.cs_gpio);
		return rc;
	}

	gpio_direction_input(cd->gpios.irq_gpio);

	return 0;
}

static void thp_free_gpio(struct thp_core_data *ts)
{
	gpio_free(ts->gpios.irq_gpio);
	gpio_free(ts->gpios.cs_gpio);
	gpio_free(ts->gpios.rst_gpio);
}

static int thp_setup_spi(struct thp_core_data *cd)
{
	int rc;

	rc = spi_setup(cd->sdev);
	if (rc) {
		THP_LOG_ERR("%s: spi setup fail\n", __func__);
		return rc;
	}

	return 0;
}

#if defined (CONFIG_TEE_TUI)
extern int spi_exit_secos(unsigned int spi_bus_id);
extern int spi_init_secos(unsigned int spi_bus_id);
void thp_tui_secos_init(void)
{
	struct thp_core_data *cd = thp_get_core_data();
	int t = 0;
	if (!cd) {
		THP_LOG_ERR("%s: core not inited\n", __func__);
		return;
	}

	/*NOTICE: should not change this path unless ack daemon*/
	thp_set_status(THP_STATUS_TUI, 1);
	cd->thp_ta_waitq_flag = WAITQ_WAIT;

	THP_LOG_INFO("%s: busid=%d. diable irq=%d\n", __func__, cd->spi_config.bus_id,cd->irq);
	t = wait_event_interruptible_timeout(cd->thp_ta_waitq,
				(cd->thp_ta_waitq_flag == WAITQ_WAKEUP),HZ);
	THP_LOG_INFO("%s: wake up finish \n",__func__);
	disable_irq(cd->irq);
	spi_init_secos(cd->spi_config.bus_id);
	thp_tui_info.enable = 1;
	return;
}

void thp_tui_secos_exit(void)
{
	struct thp_core_data *cd = thp_get_core_data();
	if (!cd) {
		THP_LOG_ERR("%s: core not inited\n", __func__);
		return;
	}
	THP_LOG_INFO("%s: busid=%d\n", __func__, cd->spi_config.bus_id);
	thp_tui_info.enable = 0;
	spi_exit_secos(cd->spi_config.bus_id);
	enable_irq(cd->irq);
	thp_set_status(THP_STATUS_TUI, 0);
	return;
}

static int thp_tui_switch(void *data, int secure)
{
	if (secure)
		thp_tui_secos_init();
	else
		thp_tui_secos_exit();
	return 0;
}

static void thp_tui_init(struct thp_core_data *cd)
{
	int rc;

	if (!cd) {
		THP_LOG_ERR("%s: core not inited\n", __func__);
		return;
	}

	thp_tui_info.enable = 0;
	strncpy(thp_tui_info.project_id, cd->project_id, THP_PROJECT_ID_LEN);
	thp_tui_info.project_id[THP_PROJECT_ID_LEN] = '\0';
	rc = register_tui_driver(thp_tui_switch, "tp", &thp_tui_info);
	if(rc != 0)
	{
		THP_LOG_ERR("%s reg thp_tui_switch fail: %d\n", __func__, rc);
		return;
	}

	THP_LOG_INFO("%s reg thp_tui_switch success addr %d\n", __func__, &thp_tui_info);
	return;
}
#endif

static int thp_pinctrl_get_init(struct thp_device *tdev)
{
	int ret = 0;
	tdev->thp_core->pctrl = devm_pinctrl_get(&tdev->sdev->dev);
	if (IS_ERR(tdev->thp_core->pctrl)) {
		THP_LOG_ERR("failed to devm pinctrl get\n");
		ret = -EINVAL;
		return ret;
	}

	tdev->thp_core->pins_default =
		pinctrl_lookup_state(tdev->thp_core->pctrl, "default");
	if (IS_ERR(tdev->thp_core->pins_default)) {
		THP_LOG_ERR("failed to pinctrl lookup state default\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}

	tdev->thp_core->pins_idle = pinctrl_lookup_state(tdev->thp_core->pctrl, "idle");
	if (IS_ERR(tdev->thp_core->pins_idle)) {
		THP_LOG_ERR("failed to pinctrl lookup state idle\n");
		ret = -EINVAL;
		goto err_pinctrl_put;
	}
	return 0;

err_pinctrl_put:
	devm_pinctrl_put(tdev->thp_core->pctrl);
	return ret;
}

static int thp_pinctrl_select_normal(struct thp_device *tdev)
{
	int retval = 0;

	retval =
	    pinctrl_select_state(tdev->thp_core->pctrl, tdev->thp_core->pins_default);
	if (retval < 0) {
		THP_LOG_ERR("set iomux normal error, %d\n", retval);
	}
	return retval;
}

static int thp_pinctrl_select_lowpower(struct thp_device *tdev)
{
	int retval = 0;

	retval = pinctrl_select_state(tdev->thp_core->pctrl, tdev->thp_core->pins_idle);
	if (retval < 0) {
		THP_LOG_ERR("set iomux lowpower error, %d\n", retval);
	}
	return retval;
}

const char *thp_get_vendor_name(void)
{
	struct thp_core_data *cd = thp_get_core_data();

	return  (cd && cd->thp_dev) ? cd->thp_dev->ic_name : 0;
}
EXPORT_SYMBOL(thp_get_vendor_name);

static int thp_project_init(struct thp_core_data *cd)
{
	int ret = 0;

	if (cd->project_in_tp && cd->thp_dev->ops->get_project_id)
		ret = cd->thp_dev->ops->get_project_id(cd->thp_dev, cd->project_id, THP_PROJECT_ID_LEN);

	if (ret) {
		strncpy(cd->project_id, cd->project_id_dummy, THP_PROJECT_ID_LEN);
		THP_LOG_INFO("%s:get projectfail ,use dummy id:%s\n", __func__, cd->project_id);
	}

	THP_LOG_INFO("%s:project id:%s\n", __func__, cd->project_id);
	return ret;
}

static int thp_core_init(struct thp_core_data *cd)
{
	int rc;

	/*step 1 : init mutex */
	mutex_init(&cd->mutex_frame);
	mutex_init(&cd->irq_mutex);
	mutex_init(&cd->thp_mutex);
	mutex_init(&cd->status_mutex);

	dev_set_drvdata(&cd->sdev->dev, cd);
	cd->ic_name = cd->thp_dev->ic_name;
	cd->prox_cache_enable = false;
	cd->need_work_in_suspend = false;
	g_thp_prox_enable = false;

#if defined(CONFIG_HUAWEI_DSM)
	if (cd->ic_name)
		dsm_thp.ic_name = cd->ic_name;
	if (strlen(cd->project_id))
		dsm_thp.module_name = cd->project_id;
	dsm_thp_dclient = dsm_register_client(&dsm_thp);
#endif

	rc = thp_project_init(cd);
	if (rc) {
		THP_LOG_ERR("%s: failed to get project id form tp ic\n", __func__);
	}

	rc = misc_register(&g_thp_misc_device);
	if (rc)	{
		THP_LOG_ERR("%s: failed to register misc device\n", __func__);
		goto err_register_misc;
	}

	rc = thp_mt_wrapper_init();
	if (rc) {
		THP_LOG_ERR("%s: failed to init input_mt_wrapper\n", __func__);
		goto err_init_wrapper;
	}

	rc = thp_init_sysfs(cd);
	if (rc) {
		THP_LOG_ERR("%s: failed to create sysfs\n", __func__);
		goto err_init_sysfs;
	}

	rc = thp_setup_irq(cd);
	if (rc) {
		THP_LOG_ERR("%s: failed to set up irq\n", __func__);
		goto err_register_misc;
	}
#ifndef CONFIG_LCD_KIT_DRIVER
	cd->lcd_notify.notifier_call = thp_lcdkit_notifier_callback;
	rc = lcdkit_register_notifier(&cd->lcd_notify);
	if (rc)	{
		THP_LOG_ERR("%s: failed to register fb_notifier: %d\n",__func__,rc);
		goto err_register_fb_notify;
	}
#endif

#if defined (CONFIG_LCD_KIT_DRIVER)
	rc = ts_kit_ops_register(&thp_ops);
	if (rc)
		THP_LOG_INFO("%s:ts_kit_ops_register fail\n", __func__);
#endif

#if CONFIG_HISI_BCI_BATTERY
	cd->charger_detect_notify.notifier_call =
			thp_charger_detect_notifier_callback;
	rc = hisi_register_notifier(&cd->charger_detect_notify, 1);
	if (rc < 0) {
		THP_LOG_ERR("%s:charger notifier register failed\n", __func__);
		cd->charger_detect_notify.notifier_call = NULL;
	} else {
		THP_LOG_INFO("%s:charger notifier register succ\n", __func__);
	}
#endif

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	set_hw_dev_flag(DEV_I2C_TOUCH_PANEL);
#endif

#if defined (CONFIG_TEE_TUI)
	thp_tui_init(cd);
#endif

	atomic_set(&cd->register_flag, 1);
	thp_set_status(THP_STATUS_POWER, 1);
	return 0;
#if 0
err_setip_irq:
	thp_sysfs_release(cd);
#endif

err_init_sysfs:
	thp_mt_wrapper_exit();
err_init_wrapper:
	misc_deregister(&g_thp_misc_device);
err_register_misc:
#ifndef CONFIG_LCD_KIT_DRIVER
	lcdkit_unregister_notifier(&cd->lcd_notify);
err_register_fb_notify:
#endif

#if defined (CONFIG_LCD_KIT_DRIVER)
	rc = ts_kit_ops_unregister(&thp_ops);
	if (rc)
		THP_LOG_INFO("%s:ts_kit_ops_register fail\n", __func__);
#endif

	mutex_destroy(&cd->mutex_frame);
	mutex_destroy(&cd->irq_mutex);
	mutex_destroy(&cd->thp_mutex);
	return rc;
}

static int thp_parse_test_config(struct device_node *test_node,
				struct thp_test_config *config)
{
	int rc;
	unsigned int value = 0;

	if (!test_node || !config) {
		THP_LOG_INFO("%s: input dev null\n", __func__);
		return -ENODEV;
	}

	rc = of_property_read_u32(test_node,
			"pt_station_test", &value);
	if (!rc) {
		config->pt_station_test = value;
		THP_LOG_INFO("%s:pt_test_flag %d\n",
			__func__, value);
	}

	return 0;
}

static struct device_node *thp_get_dev_node(struct thp_core_data *cd, struct thp_device *dev)
{
	struct device_node *dev_node = of_get_child_by_name(cd->thp_node, dev->ic_name);
	if (!dev_node && dev->dev_node_name)
		return of_get_child_by_name(cd->thp_node, dev->dev_node_name);

	return dev_node;
}

int thp_register_dev(struct thp_device *dev)
{
	int rc;
	struct thp_core_data *cd = thp_get_core_data();
	if (!dev) {
		THP_LOG_ERR("%s: input dev null\n", __func__);
		return -EINVAL;
	}
	THP_LOG_INFO("%s: called, ic name:%s\n", __func__, dev->ic_name);

	if (!cd) {
		THP_LOG_ERR("%s: core not inited\n", __func__);
		return -EINVAL;
	}
	/* check device configed ot not */
	if (!thp_get_dev_node(cd, dev)) {
		THP_LOG_INFO("%s:%s not config in dts\n", __func__, dev->ic_name);
		return -ENODEV;
	}

	if (atomic_read(&cd->register_flag)) {
		THP_LOG_ERR("%s: thp have registerd\n", __func__);
		return -ENODEV;
	}

	if ( !cd->project_in_tp && cd->ic_name && dev->ic_name &&
			strcmp(cd->ic_name, dev->ic_name)) {
		THP_LOG_ERR("%s:driver support ic mismatch connected device\n",
					__func__);
		return -ENODEV;
	}

	dev->thp_core = cd;
	dev->gpios = &cd->gpios;
	dev->sdev = cd->sdev;
	cd->thp_dev = dev;

	rc = thp_parse_timing_config(cd->thp_node, &dev->timing_config);
	if (rc) {
		THP_LOG_ERR("%s: timing config parse fail\n", __func__);
		return rc;
	}

	rc = thp_parse_test_config(cd->thp_node, &dev->test_config);
	if (rc) {
		THP_LOG_ERR("%s: special scene config parse fail\n", __func__);
		return rc;
	}

	rc = dev->ops->init(dev);
	if (rc) {
		THP_LOG_ERR("%s: dev init fail\n", __func__);
		goto dev_init_err;
	}

	rc = thp_setup_gpio(cd);
	if (rc) {
		THP_LOG_ERR("%s: spi dev init fail\n", __func__);
		goto dev_init_err;
	}

	rc = thp_setup_spi(cd);
	if (rc) {
		THP_LOG_ERR("%s: spi dev init fail\n", __func__);
		goto err;
	}

	rc = dev->ops->detect(dev);
	if (rc) {
		THP_LOG_ERR("%s: chip detect fail\n", __func__);
		goto err;
	}
	if(1 == cd->support_pinctrl){
		rc = thp_pinctrl_get_init(dev);
		if (rc) {
				THP_LOG_ERR("%s:pinctrl get init fail\n", __func__);
				goto err;
		}
	}
	rc = thp_core_init(cd);
	if (rc) {
		THP_LOG_ERR("%s: core init\n", __func__);
		goto err;
	}

	return 0;
err:
	thp_free_gpio(cd);
dev_init_err:
	cd->thp_dev = 0;
	return rc;
}
EXPORT_SYMBOL(thp_register_dev);
#define THP_SUPPORT_PINCTRL "support_pinctrl"
int thp_parse_pinctrl_config(struct device_node *spi_cfg_node,
			struct thp_core_data *cd)
{
	int rc = 0;
	unsigned int value = 0;

	if (NULL == spi_cfg_node || NULL == cd) {
		THP_LOG_INFO("%s: input null\n", __func__);
		return -ENODEV;
	}
	rc = of_property_read_u32(spi_cfg_node, THP_SUPPORT_PINCTRL, &value);
	if (0 == rc) {
		cd->support_pinctrl = value;
	}else
		cd->support_pinctrl = 0;
	THP_LOG_INFO("%s:support_pinctrl %d\n",__func__, value);
	return 0;

}

int thp_parse_spi_config(struct device_node *spi_cfg_node,
			struct thp_core_data *cd)
{
	int rc;
	unsigned int value;
	struct thp_spi_config *spi_config;
	struct pl022_config_chip *pl022_spi_config;

	if (!spi_cfg_node || !cd) {
		THP_LOG_INFO("%s: input null\n", __func__);
		return -ENODEV;
	}

	spi_config = &cd->spi_config;
	pl022_spi_config = &cd->spi_config.pl022_spi_config;

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "spi-max-frequency", &value);
	if (!rc) {
		spi_config->max_speed_hz = value;
		THP_LOG_INFO("%s:spi-max-frequency configed %d\n",
				__func__, value);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "spi-bus-id", &value);
	if (!rc) {
		spi_config->bus_id = (u8)value;
		THP_LOG_INFO("%s:spi-bus-id configed %d\n",__func__, value);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "spi-mode", &value);
	if (!rc) {
		spi_config->mode = value;
		THP_LOG_INFO("%s:spi-mode configed %d\n", __func__, value);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "bits-per-word", &value);
	if (!rc) {
		spi_config->bits_per_word = value;
		THP_LOG_INFO("%s:bits-per-word configed %d\n", __func__, value);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "pl022,interface", &value);
	if (!rc) {
		pl022_spi_config->iface = value;
		THP_LOG_INFO("%s: pl022,interface parsed\n", __func__);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "pl022,com-mode", &value);
	if (!rc) {
		pl022_spi_config->com_mode = value;
		THP_LOG_INFO("%s:com_mode parsed\n", __func__);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "pl022,rx-level-trig", &value);
	if (!rc) {
		pl022_spi_config->rx_lev_trig = value;
		THP_LOG_INFO("%s:rx-level-trig parsed\n", __func__);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "pl022,tx-level-trig", &value);
	if (!rc) {
		pl022_spi_config->tx_lev_trig = value;
		THP_LOG_INFO("%s:tx-level-trig parsed\n", __func__);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "pl022,ctrl-len", &value);
	if (!rc) {
		pl022_spi_config->ctrl_len = value;
		THP_LOG_INFO("%s:ctrl-len parsed\n", __func__);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "pl022,wait-state", &value);
	if (!rc) {
		pl022_spi_config->wait_state = value;
		THP_LOG_INFO("%s:wait-state parsed\n", __func__);
	}

	value = 0;
	rc = of_property_read_u32(spi_cfg_node, "pl022,duplex", &value);
	if (!rc) {
		pl022_spi_config->duplex = value;
		THP_LOG_INFO("%s:duplex parsed\n", __func__);
	}

	cd->spi_config.pl022_spi_config.cs_control = thp_spi_cs_set;
	cd->spi_config.pl022_spi_config.hierarchy = SSP_MASTER;

	if (!cd->spi_config.max_speed_hz)
		cd->spi_config.max_speed_hz = THP_SPI_SPEED_DEFAULT;
	if (!cd->spi_config.mode)
		cd->spi_config.mode = SPI_MODE_0;
	if (!cd->spi_config.bits_per_word)
		/*spi_config.bits_per_word default value*/
		cd->spi_config.bits_per_word = 8;

	cd->sdev->mode = spi_config->mode;
	cd->sdev->max_speed_hz = spi_config->max_speed_hz;
	cd->sdev->bits_per_word = spi_config->bits_per_word;
	cd->sdev->controller_data = &spi_config->pl022_spi_config;

	return 0;
}
EXPORT_SYMBOL(thp_parse_spi_config);

int thp_parse_timing_config(struct device_node *timing_cfg_node,
			struct thp_timing_config *timing)
{
	int rc;
	unsigned int value;

	if (!timing_cfg_node || !timing) {
		THP_LOG_INFO("%s: input null\n", __func__);
		return -ENODEV;
	}

	rc = of_property_read_u32(timing_cfg_node,
					"boot_reset_hi_delay_ms", &value);
	if (!rc) {
		timing->boot_reset_hi_delay_ms = value;
		THP_LOG_INFO("%s:boot_reset_hi_delay_ms configed %d\n",
				__func__, value);
	}

	rc = of_property_read_u32(timing_cfg_node,
					"boot_reset_low_delay_ms", &value);
	if (!rc) {
		timing->boot_reset_low_delay_ms = value;
		THP_LOG_INFO("%s:boot_reset_low_delay_ms configed %d\n",
				__func__, value);
	}

	rc = of_property_read_u32(timing_cfg_node,
					"boot_reset_after_delay_ms", &value);
	if (!rc) {
		timing->boot_reset_after_delay_ms = value;
		THP_LOG_INFO("%s:boot_reset_after_delay_ms configed %d\n",
				__func__, value);
	}

	rc = of_property_read_u32(timing_cfg_node,
					"resume_reset_after_delay_ms", &value);
	if (!rc) {
		timing->resume_reset_after_delay_ms = value;
		THP_LOG_INFO("%s:resume_reset_after_delay_ms configed %d\n",
				__func__, value);
	}

	rc = of_property_read_u32(timing_cfg_node,
					"suspend_reset_after_delay_ms", &value);
	if (!rc) {
		timing->suspend_reset_after_delay_ms = value;
		THP_LOG_INFO("%s:suspend_reset_after_delay configed_ms %d\n",
				__func__, value);
	}

	rc = of_property_read_u32(timing_cfg_node,
					"spi_sync_cs_hi_delay_ns", &value);
	if (!rc) {
		timing->spi_sync_cs_hi_delay_ns = value;
		THP_LOG_INFO("%s:spi_sync_cs_hi_delay_ns configed_ms %d\n",
				__func__, value);
	}

	rc = of_property_read_u32(timing_cfg_node,
					"spi_sync_cs_low_delay_ns", &value);
	if (!rc) {
		timing->spi_sync_cs_low_delay_ns = value;
		THP_LOG_INFO("%s:spi_sync_cs_low_delay_ns configed_ms %d\n",
				__func__, value);
	}

	return 0;
}
EXPORT_SYMBOL(thp_parse_timing_config);
int thp_parse_trigger_config(struct device_node *thp_node,
			struct thp_core_data *cd)
{
	int rc = 0;
	unsigned int value = 0;
	THP_LOG_DEBUG("%s:Enter!\n",__func__);

	rc = of_property_read_u32(thp_node,"irq_flag", &value);
	if (!rc) {
		cd->irq_flag = value;
		THP_LOG_INFO("%s:cd->irq_flag %d\n",__func__, value);
	}else{
		cd->irq_flag = IRQF_TRIGGER_FALLING;
		THP_LOG_INFO("%s:cd->irq_flag defaule =>  %d\n",__func__, cd->irq_flag);
	}
	return	0;
}
EXPORT_SYMBOL(thp_parse_trigger_config);


 int thp_parse_feature_config(struct device_node *thp_node,
			struct thp_core_data *cd)
 {
	int rc = 0;
	unsigned int value = 0;
	THP_LOG_DEBUG("%s:Enter!\n",__func__);

	rc = of_property_read_u32(thp_node,"need_huge_memory_in_spi", &value);
	if (!rc) {
		cd->need_huge_memory_in_spi = value;
		THP_LOG_INFO("%s:need_huge_memory_in_spi configed %d\n",__func__, value);
	}

	rc = of_property_read_u32(thp_node,"project_in_tp", &value);
	if (!rc) {
		cd->project_in_tp = value;
		THP_LOG_INFO("%s:project_in_tp configed %d\n",__func__, value);
	}

	cd->project_id_dummy = "dummy";
	rc = of_property_read_string(thp_node,"project_id_dummy", (const char **)&cd->project_id_dummy);
	if (!rc) {
		THP_LOG_INFO("%s:project_id_dummy configed %s\n",__func__, cd->project_id_dummy);
	}

	rc = of_property_read_u32(thp_node,"supported_func_indicater", &value);
	if (!rc) {
		cd->supported_func_indicater = value;
		THP_LOG_INFO("%s:supported_func_indicater configed %d\n",__func__, value);
	}

	rc = of_property_read_u32(thp_node,"use_hwlock", &value);
	if (!rc) {
		cd->use_hwlock = value;
		THP_LOG_INFO("%s:use_hwlock configed %d\n",__func__, value);
	}

	return  0;
}
EXPORT_SYMBOL(thp_parse_feature_config);

int is_pt_test_mode(struct thp_device *tdev)
{
	int ret = 0;
	int thp_pt_station_flag = 0;

#if defined (CONFIG_LCD_KIT_DRIVER)
	struct lcd_kit_ops *lcd_ops = lcd_kit_get_ops();
	if((lcd_ops)&&(lcd_ops->get_status_by_type)) {
		ret = lcd_ops->get_status_by_type(PT_STATION_TYPE, &thp_pt_station_flag);
		if(ret < 0) {
			THP_LOG_INFO("%s: get thp_pt_station_flag fail\n", __func__);
			return ret;
		}
	}
#else
	thp_pt_station_flag = (g_tskit_pt_station_flag && tdev->test_config.pt_station_test);
#endif

	THP_LOG_INFO("%s thp_pt_station_flag = %d\n", __func__,thp_pt_station_flag);

	return thp_pt_station_flag;
}
EXPORT_SYMBOL(is_pt_test_mode);

int is_tp_detected(void)
{
	int ret = TP_DETECT_SUCC;
	struct thp_core_data *cd = thp_get_core_data();

	if (!atomic_read(&cd->register_flag))
		ret = TP_DETECT_FAIL;

	THP_LOG_INFO("[Proximity_feature] %s : Check if tp is in place, ret = %d\n",
		__func__, ret);
	return ret;
}

int thp_set_prox_switch_status(bool enable)
{
#ifdef CONFIG_INPUTHUB_20
	int ret;
	int report_value[PROX_VALUE_LEN] = {0};
#endif
	struct thp_core_data *cd = thp_get_core_data();

	if (!atomic_read(&cd->register_flag))
		return 0;

	if (cd->proximity_support == PROX_SUPPORT) {
#ifdef CONFIG_INPUTHUB_20
		report_value[0] = AWAY_EVENT_VALUE;
		ret = thp_prox_event_report(report_value, PROX_EVENT_LEN);
		if (ret < 0)
			THP_LOG_INFO("%s: report event fail\n", __func__);
		THP_LOG_INFO("[Proximity_feature] %s: default report [far] event!\n",
			__func__);
#endif
		thp_set_status(THP_STATUS_TOUCH_APPROACH, !!enable);
		if (cd->suspended == false) {
			g_thp_prox_enable = enable;
			THP_LOG_INFO("[Proximity_feature] %s :(1) Update g_thp_prox_enable to %d in screen on!\n",
				__func__, g_thp_prox_enable);
		} else {
			cd->prox_cache_enable = enable;
			THP_LOG_INFO("[Proximity_feature] %s :(2) Update prox_cache_enable to %d in screen off!\n",
				__func__, cd->prox_cache_enable);
		}
		return 0;
	}
	THP_LOG_INFO("[Proximity_feature]%s: Not support proximity feature!\n",
		__func__);
	return 0;
}

bool thp_get_prox_switch_status(void)
{
	struct thp_core_data *cd = thp_get_core_data();

	if (cd->proximity_support == PROX_SUPPORT) {
		THP_LOG_INFO("[Proximity_feature] %s: need_work_in_suspend = %d!\n",
			__func__, cd->need_work_in_suspend);
		return cd->need_work_in_suspend;
	}
	THP_LOG_INFO("[Proximity_feature]%s:Not support proximity feature!\n",
		__func__);
	return 0;
}

static int thp_parse_config(struct thp_core_data *cd,
					struct device_node *thp_node)
{
	int rc;
	unsigned int value;

	if (!thp_node) {
		THP_LOG_ERR("%s:thp not config in dts, exit\n", __func__);
		return -ENODEV;
	}

	rc = thp_parse_spi_config(thp_node, cd);
	if (rc) {
		THP_LOG_ERR("%s: spi config parse fail\n", __func__);
		return rc;
	}

	rc = thp_parse_power_config(thp_node, cd);
	if (rc) {
		THP_LOG_ERR("%s: power config parse fail\n", __func__);
		return rc;
	}

	rc = thp_parse_pinctrl_config(thp_node, cd);
	if (rc) {
		THP_LOG_ERR("%s:pinctrl parse fail\n", __func__);
		return rc;
	}

	cd->irq_flag = IRQF_TRIGGER_FALLING;
	rc = of_property_read_u32(thp_node, "irq_flag", &value);
	if (!rc) {
		cd->irq_flag = value;
		THP_LOG_INFO("%s:irq_flag parsed\n", __func__);
	}

	cd->proximity_support = PROX_NOT_SUPPORT;
	rc = of_property_read_u32(thp_node, "proximity_support", &value);
	if (!rc) {
		cd->proximity_support = value;
		THP_LOG_INFO("%s:parsed success, proximity_support = %u\n",
			__func__, cd->proximity_support);
	} else {
		THP_LOG_INFO("%s:parsed failed, proximity_support = %u\n",
			__func__, cd->proximity_support);
	}

	value = of_get_named_gpio(thp_node, "irq_gpio", 0);
	THP_LOG_INFO("irq gpio_ = %d\n", value);
	if (!gpio_is_valid(value)) {
		THP_LOG_ERR("%s: get irq_gpio failed\n", __func__);
		return rc;
	}
	cd->gpios.irq_gpio = value;

	value = of_get_named_gpio(thp_node, "rst_gpio", 0);
	THP_LOG_ERR("rst_gpio = %d\n", value);
	if (!gpio_is_valid(value)) {
		THP_LOG_ERR("%s: get rst_gpio failed\n", __func__);
		return rc;
	}
	cd->gpios.rst_gpio = value;

	value = of_get_named_gpio(thp_node, "cs_gpio", 0);
	THP_LOG_ERR("cs_gpio = %d\n", value);
	if (!gpio_is_valid(value)) {
		THP_LOG_ERR("%s: get cs_gpio failed\n", __func__);
		return rc;
	}
	thp_parse_feature_config(thp_node , cd);

	cd->gpios.cs_gpio = value;

	cd->thp_node = thp_node;

	if (of_find_property(thp_node, "kirin-udp", NULL))
		cd->is_udp = true;
	else
		cd->is_udp = false;

	return 0;
}

static int thp_probe(struct spi_device *sdev)
{
	struct thp_core_data *thp_core;
	int rc;

	THP_LOG_INFO("%s: in\n", __func__);

	thp_core = kzalloc(sizeof(struct thp_core_data), GFP_KERNEL);
	if (!thp_core) {
		THP_LOG_ERR("%s: out of memory\n", __func__);
		return -ENOMEM;
	}

	thp_core->sdev = sdev;
	rc = thp_parse_config(thp_core, sdev->dev.of_node);
	if (rc) {
		THP_LOG_ERR("%s: parse dts fail\n", __func__);
		kfree(thp_core);
		return -ENODEV;
	}

	rc = thp_init_chip_info(thp_core);
	if (rc)
		THP_LOG_ERR("%s: chip info init fail\n", __func__);

	mutex_init(&thp_core->spi_mutex);
	THP_LOG_INFO("%s:use_hwlock = %d\n", __func__, thp_core->use_hwlock);
	if (thp_core->use_hwlock) {
		thp_core->hwspin_lock = hwspin_lock_request_specific(TP_HWSPIN_LOCK_CODE);
		if (!thp_core->hwspin_lock)
			THP_LOG_ERR("%s: hwspin_lock request failed\n", __func__);
	}

	atomic_set(&thp_core->register_flag, 0);
	INIT_LIST_HEAD(&thp_core->frame_list.list);
	init_waitqueue_head(&(thp_core->frame_waitq));
	init_waitqueue_head(&(thp_core->thp_ta_waitq));
	init_waitqueue_head(&(thp_core->thp_event_waitq));
	thp_core->event_flag = false;
	spi_set_drvdata(sdev, thp_core);

	g_thp_core = thp_core;

	return 0;
}

static int thp_remove(struct spi_device *sdev)
{
	struct thp_core_data *cd = spi_get_drvdata(sdev);
	int rc;

	THP_LOG_INFO("%s: in\n", __func__);

	if (atomic_read(&cd->register_flag)) {
		thp_sysfs_release(cd);

#if defined(THP_CHARGER_FB)
		if (cd->charger_detect_notify.notifier_call)
			hisi_charger_type_notifier_unregister(
					&cd->charger_detect_notify);
#endif
#ifndef CONFIG_LCD_KIT_DRIVER
		lcdkit_unregister_notifier(&cd->lcd_notify);
#endif

#if defined (CONFIG_LCD_KIT_DRIVER)
	rc = ts_kit_ops_unregister(&thp_ops);
	if (rc)
		THP_LOG_INFO("%s:ts_kit_ops_register fail\n", __func__);
#endif

		misc_deregister(&g_thp_misc_device);
		mutex_destroy(&cd->mutex_frame);
		thp_mt_wrapper_exit();
	}

	if(spi_sync_rx_buf){
		kfree(spi_sync_rx_buf);
		spi_sync_rx_buf = NULL;
	}
	if(spi_sync_tx_buf){
		kfree(spi_sync_tx_buf);
		spi_sync_tx_buf = NULL;
	}
	if(cd){
		kfree(cd);
		cd = NULL;
	}


#if defined (CONFIG_TEE_TUI)
	unregister_tui_driver("tp");
#endif
	return 0;
}


static const struct of_device_id g_thp_psoc_match_table[] = {
	{.compatible = "huawei,thp",},
	{ },
};


static const struct spi_device_id g_thp_device_id[] = {
	{ THP_DEVICE_NAME, 0 },
	{ }
};
MODULE_DEVICE_TABLE(spi, g_thp_device_id);

static struct spi_driver g_thp_spi_driver = {
	.probe = thp_probe,
	.remove = thp_remove,
	.id_table = g_thp_device_id,
	.driver = {
		.name = THP_DEVICE_NAME,
		.owner = THIS_MODULE,
		.bus = &spi_bus_type,
		.of_match_table = g_thp_psoc_match_table,
	},
};

module_spi_driver(g_thp_spi_driver);

MODULE_AUTHOR("Huawei Device Company");
MODULE_DESCRIPTION("Huawei THP Driver");
MODULE_LICENSE("GPL");

