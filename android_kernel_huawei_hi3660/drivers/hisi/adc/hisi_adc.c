/*
 * hisi_adc.c for the hkadc driver.
 *
 * Copyright (c) 2013 Hisilicon Technologies CO.Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/notifier.h>
#include <linux/delay.h>
#include <linux/completion.h>
#include <linux/mutex.h>
#include <linux/debugfs.h>
#include <linux/hisi/hisi_mailbox.h>
#include <linux/hisi/hisi_mailbox_dev.h>
#include <linux/hisi/hisi_adc.h>
#include <linux/hisi/hisi_rproc.h>

#define	MODULE_NAME		"hisi_adc"
/*adc maybe IPC timeout*/
#define ADC_IPC_TIMEOUT		1500
#define	ADC_IPC_MAX_CNT		3

#define HISI_AP_ID		0x00
#define	ADC_OBJ_ID		0x0b
#define ADC_IPC_CMD_TYPE0	0x02
#define	ADC_IPC_CMD_TYPE1	0x0c
#define	ADC_IPC_DATA		((HISI_AP_ID << 24) | (ADC_OBJ_ID << 16) | (ADC_IPC_CMD_TYPE0 << 8) | ADC_IPC_CMD_TYPE1)
#define	HADC_IPC_RECV_HEADER	((0x08 << 24) | (ADC_OBJ_ID << 16) | (ADC_IPC_CMD_TYPE0 << 8) | ADC_IPC_CMD_TYPE1)
#define	ADC_DATA_BIT_R(d, n)	((d) >> (n))
#define	ADC_DATA_MASK		0xff
#define	ADC_RESULT_ERR		(-EINVAL)
#define	ADC_CHN_MAX		18

#define	ADC_CHANNEL_XOADC	(15)
#define	VREF_VOLT		(1800)
#define	AD_RANGE				(32767)

#define	ADC_IPC_CMD_TYPE_CURRENT	(0x14)
#define	ADC_IPC_CURRENT		((HISI_AP_ID << 24) | (ADC_OBJ_ID << 16) | (ADC_IPC_CMD_TYPE0 << 8) | ADC_IPC_CMD_TYPE_CURRENT)
#define	HADC_IPC_RECV_CURRENT_HEADER	((0x08 << 24) | (ADC_OBJ_ID << 16) | (ADC_IPC_CMD_TYPE0 << 8) | ADC_IPC_CMD_TYPE_CURRENT)

enum {
	ADC_IPC_CMD_TYPE = 0,
	ADC_IPC_CMD_CHANNEL,
	ADC_IPC_CMD_LEN
};

struct adc_info {
	int channel;
	int value;
};

struct hisi_adc_device {
	struct adc_info		info;
	mbox_msg_t		tx_msg[ADC_IPC_CMD_LEN];
	struct notifier_block	*nb;
	struct mutex		mutex;
	struct completion	completion;
};
static struct hisi_adc_device	*hisi_adc_dev = NULL;


#define HKADC_VREF  (1800)
#define HKADC_ACCURACY  (0xFFF)
#define ADC_RPROC_SEND_ID	HISI_RPROC_LPM3_MBX16
#define ADC_RPROC_RECV_ID	HISI_RPROC_LPM3_MBX0

int g_hkadc_debug = 0;

void hkadc_debug(int onoff)
{
	g_hkadc_debug = onoff;
}

/* notifiers AP when LPM3 sends msgs*/
static int adc_dev_notifier(struct notifier_block *nb, unsigned long len, void *msg)
{
	u32 *_msg = (u32 *)msg;
	int is_complete = ADC_RESULT_ERR;
	unsigned long i;

	if (1 == g_hkadc_debug) {
		for (i = 0; i < len; i++)
			pr_info("%s_debug:[notifier] msg[%lu] = 0x%x\n", MODULE_NAME, i, _msg[i]);
	}

	if (_msg[0] == HADC_IPC_RECV_HEADER || _msg[0] == HADC_IPC_RECV_CURRENT_HEADER) {
		if (!(ADC_DATA_BIT_R(_msg[1], 8) & ADC_DATA_MASK)) {
			if ((_msg[1] & ADC_DATA_MASK) == hisi_adc_dev->info.channel) {
				hisi_adc_dev->info.value = ADC_DATA_BIT_R(_msg[1], 16);
				is_complete = 0;
				pr_debug("%s: value msg[1][0x%x]\n", MODULE_NAME, _msg[1]);
				goto exit;
			}
		} else {
			is_complete = 0;
			hisi_adc_dev->info.value = ADC_RESULT_ERR;
		}
		pr_err("%s: error value msg[1][0x%x]\n", MODULE_NAME, _msg[1]);
	}

exit:
	if (!is_complete)
		complete(&hisi_adc_dev->completion);

	return 0;
}


/* AP sends msgs to LPM3 for adc sampling&converting. */
static int adc_send_ipc_to_lpm3(int channel, int ipc_header)
{
	int loop = ADC_IPC_MAX_CNT;
	int ret = 0;

	if (channel > ADC_CHN_MAX) {
		pr_err("%s: invalid channel!\n", MODULE_NAME);
		ret = -EINVAL;
		goto err_adc_channel;
	}

	if (!hisi_adc_dev) {
		pr_err("%s: adc dev is not initialized yet!\n", MODULE_NAME);
		ret = -ENODEV;
		goto err_adc_dev;
	}

	hisi_adc_dev->tx_msg[ADC_IPC_CMD_TYPE] = ipc_header;

	hisi_adc_dev->info.channel = channel;
	hisi_adc_dev->tx_msg[ADC_IPC_CMD_CHANNEL] = channel;

	do {
		ret = RPROC_ASYNC_SEND(ADC_RPROC_SEND_ID, (mbox_msg_t *)hisi_adc_dev->tx_msg, ADC_IPC_CMD_LEN);
		loop--;
	} while (ret == -ENOMEM && loop > 0);
	if (ret) {
		pr_err("%s: fail to send mbox msg, ret = %d!\n", MODULE_NAME, ret);
		goto err_msg_async;
	}

	return ret;

err_msg_async:
	hisi_adc_dev->info.channel = ADC_RESULT_ERR;
err_adc_dev:
err_adc_channel:
	return ret;
}

/*
 * Function name:adc_to_volt.
 * Discription:calculate volt from hkadc.
 * Parameters:
 *      @ adc
 * return value:
 *      @ volt(mv): negative-->failed, other-->succeed.
 */
int adc_to_volt(int adc)
{
	int volt;
	if (adc < 0)
		return -1;

	volt = ((adc * HKADC_VREF) / HKADC_ACCURACY);

	return volt;
}

int xoadc_to_volt(int adc)
{
	int volt;

	if (adc < 0)
		return -1;

	volt = adc * VREF_VOLT / AD_RANGE;

	return volt;
}
/*
 * Function name:hisi_adc_get_value.
 * Discription:get volt from hkadc.
 * Parameters:
 *      @ adc_channel
 * return value:
 *      @ channel volt(mv): negative-->failed, other-->succeed.
 */
int hisi_adc_get_value(int adc_channel)
{
	int ret;
	int volt;

	ret = hisi_adc_get_adc(adc_channel);

	if (ret < 0)
		return ret;

	if (ADC_CHANNEL_XOADC == adc_channel)
		volt = xoadc_to_volt(ret);
	else
		volt = adc_to_volt(ret);

	return volt;
}
EXPORT_SYMBOL(hisi_adc_get_value);

static int hisi_adc_send_wait(int adc_channel, int ipc_header)
{
	int ret = 0;

#if LINUX_VERSION_CODE >= KERNEL_VERSION(3,13,0)
	reinit_completion(&hisi_adc_dev->completion);
#else
	INIT_COMPLETION(hisi_adc_dev->completion);
#endif
	ret = adc_send_ipc_to_lpm3(adc_channel, ipc_header);
	if (ret)
		goto exit;

	ret = wait_for_completion_timeout(&hisi_adc_dev->completion, msecs_to_jiffies(ADC_IPC_TIMEOUT));
	if (!ret) {
		pr_err("%s: channel-%d timeout!\n", MODULE_NAME, adc_channel);
		ret =  -ETIMEOUT;
	} else
		ret = 0;

exit:
	return ret;
}
/*
 * Function name:hisi_adc_get_adc.
 * Discription:get adc value from hkadc.
 * Parameters:
 *      @ adc_channel
 * return value:
 *      @ adc value(12 bits): negative-->failed, other-->succeed.
 */
int hisi_adc_get_adc(int adc_channel)
{
	int ret = 0, value = 0;

	mutex_lock(&hisi_adc_dev->mutex);

	ret = hisi_adc_send_wait(adc_channel, ADC_IPC_DATA);
	value = hisi_adc_dev->info.value;
	hisi_adc_dev->info.channel = ADC_RESULT_ERR;

	mutex_unlock(&hisi_adc_dev->mutex);
	if (1 == g_hkadc_debug)
		pr_info("%s value %d;ret %d\n", __func__, value, ret);

	return ret ? ret : value;
}
EXPORT_SYMBOL(hisi_adc_get_adc);

int hisi_adc_get_current(int adc_channel)
{
	int ret = 0, value = 0;

	mutex_lock(&hisi_adc_dev->mutex);

	ret = hisi_adc_send_wait(adc_channel, ADC_IPC_CURRENT);
	value = hisi_adc_dev->info.value;
	hisi_adc_dev->info.channel = ADC_RESULT_ERR;

	mutex_unlock(&hisi_adc_dev->mutex);

	return ret ? ret : value;
}
EXPORT_SYMBOL(hisi_adc_get_current);


static void adc_init_device_debugfs(void)
{
}

static void adc_remove_device_debugfs(void)
{
}


/* hisi adc init function */
static int __init hisi_adc_driver_init(void)
{
	int ret = 0;

	hisi_adc_dev = kzalloc(sizeof(*hisi_adc_dev), GFP_KERNEL);
	if (!hisi_adc_dev) {
		pr_err("%s: fail to alloc adc dev!\n", MODULE_NAME);
		ret = -ENOMEM;
		goto err_adc_dev;
	}

	hisi_adc_dev->nb = kzalloc(sizeof(struct notifier_block), GFP_KERNEL);
	if (!hisi_adc_dev->nb) {
		pr_err("%s: fail to alloc notifier_block!\n", MODULE_NAME);
		ret =  -ENOMEM;
		goto err_adc_nb;
	}

	hisi_adc_dev->nb->next = NULL;
	hisi_adc_dev->nb->notifier_call = adc_dev_notifier;

	/* register the rx notify callback */
	ret = RPROC_MONITOR_REGISTER(ADC_RPROC_RECV_ID,  hisi_adc_dev->nb);
	if (ret) {
		pr_info("%s:RPROC_MONITOR_REGISTER failed", __func__);
		goto err_get_mbox;
	}

	hisi_adc_dev->tx_msg[ADC_IPC_CMD_TYPE] = ADC_IPC_DATA;
	hisi_adc_dev->tx_msg[ADC_IPC_CMD_CHANNEL] = (mbox_msg_t)ADC_RESULT_ERR;

	mutex_init(&hisi_adc_dev->mutex);
	init_completion(&hisi_adc_dev->completion);
	adc_init_device_debugfs();

	pr_info("%s: init ok!\n", MODULE_NAME);
	return ret;

err_get_mbox:
	kfree(hisi_adc_dev->nb);
	hisi_adc_dev->nb = NULL;
err_adc_nb:
	kfree(hisi_adc_dev);
	hisi_adc_dev = NULL;
err_adc_dev:
	return ret;
}

/* hisi adc exit function */
static void __exit hisi_adc_driver_exit(void)
{
	if (hisi_adc_dev) {
		kfree(hisi_adc_dev->nb);
		hisi_adc_dev->nb = NULL;

		kfree(hisi_adc_dev);
		hisi_adc_dev = NULL;
	}

	adc_remove_device_debugfs();
}

subsys_initcall(hisi_adc_driver_init);
module_exit(hisi_adc_driver_exit);
