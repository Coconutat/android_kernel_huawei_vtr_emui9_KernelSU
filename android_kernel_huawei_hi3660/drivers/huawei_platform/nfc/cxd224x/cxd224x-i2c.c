/*
 *	cxd224x-i2c.c - cxd224x NFC i2c driver
 *
 * Copyright (C) 2013- Sony Corporation.
 * Copyright (C) 2012 Broadcom Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/i2c.h>
#include <linux/irq.h>
#include <linux/jiffies.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/io.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/miscdevice.h>
#include <linux/spinlock.h>
#include <linux/reboot.h>
#include <linux/poll.h>
#include <linux/version.h>
#include <linux/of_gpio.h>
#include <linux/mfd/hisi_pmic.h>
#include <huawei_platform/log/hw_log.h>
#define HWLOG_TAG nfc
HWLOG_REGIST();

#include "cxd224x.h"
#include "huawei_nfc.h"

#ifdef CONFIG_HUAWEI_HW_DEV_DCT
#include <huawei_platform/devdetect/hw_dev_dec.h>
#endif

#ifdef CONFIG_WAKELOCK
#include <linux/wakelock.h>

#define CXD224X_WAKE_LOCK_TIMEOUT	2		/* wake lock timeout for HOSTINT (sec) */
#define CXD224X_WAKE_LOCK_NAME	"cxd224x-i2c"		/* wake lock for HOSTINT */
#define CXD224X_WAKE_LOCK_TIMEOUT_LP	2		/* wake lock timeout for low-power-mode (sec) */
#define CXD224X_WAKE_LOCK_NAME_LP "cxd224x-i2c-lp"	/* wake lock for low-power-mode */
#endif

#ifndef DEBUG_NCI_KERNEL
#define DEBUG_NCI_KERNEL
#endif

/* do not change below */
#define MAX_BUFFER_SIZE		780

/* Read data */
#define PACKET_HEADER_SIZE_NCI	(3)
#define PACKET_HEADER_SIZE_HCI	(3)
#define PACKET_TYPE_NCI		(16)
#define PACKET_TYPE_HCIEV	(4)
#define MAX_PACKET_SIZE		(PACKET_HEADER_SIZE_NCI + 255)

/* RESET */
#define RESET_ASSERT_MS			(1)

static struct huawei_nfc_data *g_p_nfc_data;

struct cxd224x_dev {
	wait_queue_head_t read_wq;
	struct mutex read_mutex;
	struct i2c_client *client;
	struct miscdevice cxd224x_device;
	struct cxd224x_platform_data *gpio;
	bool irq_enabled;
	struct mutex lock;
	spinlock_t irq_enabled_lock;
	unsigned int users;
	unsigned int count_irq;
#ifdef CONFIG_WAKELOCK
	struct wake_lock wakelock;	/* wake lock for HOSTINT */
	struct wake_lock wakelock_lp;	/* wake lock for low-power-mode */
#endif
	/* Driver message queue */
	struct workqueue_struct *wqueue;
	struct work_struct qmsg;
};

#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
static void cxd224x_workqueue(struct work_struct *work)
{
	struct cxd224x_dev *cxd224x_dev = container_of(work, struct cxd224x_dev, qmsg);
	unsigned long flags;

	hwlog_info("%s, xrst assert\n", __func__);
	spin_lock_irqsave(&cxd224x_dev->irq_enabled_lock, flags);
	gpio_set_value(cxd224x_dev->gpio->rst_gpio, CXDNFC_RST_ACTIVE);
	cxd224x_dev->count_irq = 0; /* clear irq */
	spin_unlock_irqrestore(&cxd224x_dev->irq_enabled_lock, flags);

	msleep(RESET_ASSERT_MS);
	hwlog_info("%s, xrst deassert\n", __func__);
	gpio_set_value(cxd224x_dev->gpio->rst_gpio, ~CXDNFC_RST_ACTIVE & 0x1);
}

static int __init init_wqueue(struct cxd224x_dev *cxd224x_dev)
{
	INIT_WORK(&cxd224x_dev->qmsg, cxd224x_workqueue);
	cxd224x_dev->wqueue = create_workqueue("cxd224x-i2c_wrokq");
	if (cxd224x_dev->wqueue == NULL)
		return -EBUSY;
	return 0;
}
#endif /* CONFIG_NFC_CXD224X_RST */

static int cxd224x_pon_enable(struct huawei_nfc_data *pdev)
{
	int ret = 0;
	if (pdev == NULL) {
		hwlog_err("%s: pdev is null !\n", __func__);
		return -1;
	}
	switch (pdev->ven_on_gpio_type) {
	case NFCC_ON_BY_GPIO:
		hwlog_info("%s: Nfc on by GPIO !\n", __func__);
		//gpio_set_value(pdev->ven_gpio, 1);
		break;
	case NFCC_ON_BY_HISI_PMIC:
	case NFCC_ON_BY_HI6421V600_PMIC:
	case NFCC_ON_BY_HI6555V110_PMIC:
	case NFCC_ON_BY_HI6421V700_PMIC:
		hwlog_info("%s: Nfc on by HISI PMIC !\n", __func__);
		ret = set_hisi_pmic_onoroff(PMIC_ON);
		break;

	default:
		hwlog_err("%s: Unknown nfc on type !\n", __func__);
		break;
	}

	return ret;
}

static int cxd224x_pon_disable(struct huawei_nfc_data *pdev)
{
	int ret = 0;

	if (pdev == NULL) {
		hwlog_err("%s: pdev is null !\n", __func__);
		return -1;
	}

	switch (pdev->ven_on_gpio_type) {
	case NFCC_ON_BY_GPIO:
		hwlog_info("%s: Nfc off by GPIO !\n", __func__);
		//gpio_set_value(pdev->ven_gpio, 0);
		break;
	case NFCC_ON_BY_HISI_PMIC:
	case NFCC_ON_BY_HI6421V600_PMIC:
	case NFCC_ON_BY_HI6555V110_PMIC:
	case NFCC_ON_BY_HI6421V700_PMIC:
		hwlog_info("%s: Nfc off by HISI PMIC !\n", __func__);
		ret = set_hisi_pmic_onoroff(PMIC_OFF);
		break;

	default:
		hwlog_err("%s: Unknown nfc off type !\n", __func__);
		break;
	}

	return ret;
}

static void cxd224x_init_stat(struct cxd224x_dev *cxd224x_dev)
{
	cxd224x_dev->count_irq = 0;
}

static void cxd224x_disable_irq(struct cxd224x_dev *cxd224x_dev)
{
	unsigned long flags;
	spin_lock_irqsave(&cxd224x_dev->irq_enabled_lock, flags);
	if (cxd224x_dev->irq_enabled) {
		disable_irq_nosync(cxd224x_dev->client->irq);
		cxd224x_dev->irq_enabled = false;
	}
	spin_unlock_irqrestore(&cxd224x_dev->irq_enabled_lock, flags);
}

static void cxd224x_enable_irq(struct cxd224x_dev *cxd224x_dev)
{
	unsigned long flags;
	spin_lock_irqsave(&cxd224x_dev->irq_enabled_lock, flags);
	if (!cxd224x_dev->irq_enabled) {
		cxd224x_dev->irq_enabled = true;
		enable_irq(cxd224x_dev->client->irq);
	}
	spin_unlock_irqrestore(&cxd224x_dev->irq_enabled_lock, flags);
}

static irqreturn_t cxd224x_dev_irq_handler(int irq, void *dev_id)
{
	struct cxd224x_dev *cxd224x_dev = dev_id;
	unsigned long flags;

	spin_lock_irqsave(&cxd224x_dev->irq_enabled_lock, flags);
	cxd224x_dev->count_irq++;
	spin_unlock_irqrestore(&cxd224x_dev->irq_enabled_lock, flags);
	wake_up(&cxd224x_dev->read_wq);

	return IRQ_HANDLED;
}

static unsigned int cxd224x_dev_poll(struct file *filp, poll_table *wait)
{
	struct cxd224x_dev *cxd224x_dev = filp->private_data;
	unsigned int mask = 0;
	unsigned long flags;

	poll_wait(filp, &cxd224x_dev->read_wq, wait);

	spin_lock_irqsave(&cxd224x_dev->irq_enabled_lock, flags);
	if (cxd224x_dev->count_irq > 0)
	{
		cxd224x_dev->count_irq--;
		mask |= POLLIN | POLLRDNORM;
	}
	spin_unlock_irqrestore(&cxd224x_dev->irq_enabled_lock, flags);

#ifdef CONFIG_WAKELOCK
	if(mask)
		wake_lock_timeout(&cxd224x_dev->wakelock, CXD224X_WAKE_LOCK_TIMEOUT*HZ);
#endif

	return mask;
}

static ssize_t cxd224x_dev_read(struct file *filp, char __user *buf,
				  size_t count, loff_t *offset)
{
	struct cxd224x_dev *cxd224x_dev = filp->private_data;
	unsigned char tmp[MAX_BUFFER_SIZE];
	int total, len, ret;

	total = 0;
	len = 0;

	if (count > MAX_BUFFER_SIZE)
		count = MAX_BUFFER_SIZE;

	mutex_lock(&cxd224x_dev->read_mutex);

	ret = i2c_master_recv(cxd224x_dev->client, tmp, 3);
	if (ret == 3 && (tmp[0] != 0xff)) {
		total = ret;

		len = tmp[PACKET_HEADER_SIZE_NCI-1];

		/** make sure full packet fits in the buffer
		**/
		if (len > 0 && (len + total) <= count) {
			/** read the remainder of the packet.
			**/
			ret = i2c_master_recv(cxd224x_dev->client, tmp+total, len);
			if (ret == len)
				total += len;
		}
	}

#ifdef DEBUG_NCI_KERNEL
{
	char *tmpStr = NULL;
	int i = 0;
	tmpStr = (char *)kzalloc(sizeof(tmp)*2 + 1, GFP_KERNEL);
	if (!tmpStr) {
		hwlog_err("%s:Cannot allocate memory for write tmpStr.\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < total; i++) {
		snprintf(&tmpStr[i * 2], 3, "%02X", tmp[i]);
	}
	hwlog_info("%s : ret = %d, count = %3d > %s\n", __func__, ret, (int)total, tmpStr);

	kfree(tmpStr);
}
#endif

	mutex_unlock(&cxd224x_dev->read_mutex);

	if (total > count || copy_to_user(buf, tmp, total)) {
		hwlog_err("failed to copy to user space, total = %d\n", total);
		total = -EFAULT;
	}

	return total;
}

static ssize_t cxd224x_dev_write(struct file *filp, const char __user *buf,
				   size_t count, loff_t *offset)
{
	struct cxd224x_dev *cxd224x_dev = filp->private_data;
	char tmp[MAX_BUFFER_SIZE];
	int ret;

	if (count > MAX_BUFFER_SIZE) {
		hwlog_err("out of memory\n");
		return -ENOMEM;
	}

	if (copy_from_user(tmp, buf, count)) {
		hwlog_err("failed to copy from user space\n");
		return -EFAULT;
	}

	mutex_lock(&cxd224x_dev->read_mutex);
	/* Write data */

	ret = i2c_master_send(cxd224x_dev->client, tmp, count);
	if (ret != count) {
		hwlog_err("failed to write %d\n", ret);
		ret = -EIO;
	}
#ifdef DEBUG_NCI_KERNEL
{
	char *tmpStr = NULL;
	int i = 0;
	tmpStr = (char *)kzalloc(sizeof(tmp)*2 + 1, GFP_KERNEL);
	if (!tmpStr) {
		hwlog_err("%s:Cannot allocate memory for write tmpStr.\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < count; i++) {
		snprintf(&tmpStr[i * 2], 3, "%02X", tmp[i]);
	}
	hwlog_info("%s : ret = %d, count = %3d > %s\n", __func__, ret, (int)count, tmpStr);

	kfree(tmpStr);
}
#endif
	mutex_unlock(&cxd224x_dev->read_mutex);

	return ret;
}

static int cxd224x_dev_open(struct inode *inode, struct file *filp)
{
	int ret = 0;
	int call_enable = 0;
	struct cxd224x_dev *cxd224x_dev = container_of(filp->private_data,
			   struct cxd224x_dev,
			   cxd224x_device);
	filp->private_data = cxd224x_dev;
	mutex_lock(&cxd224x_dev->lock);
	if (!cxd224x_dev->users)
	{
		cxd224x_init_stat(cxd224x_dev);
		call_enable = 1;
	}
	cxd224x_dev->users++;
	mutex_unlock(&cxd224x_dev->lock);
	if (call_enable)
		cxd224x_enable_irq(cxd224x_dev);

	hwlog_info("open %d,%d users=%d\n", imajor(inode), iminor(inode), cxd224x_dev->users);

	return ret;
}

static int cxd224x_dev_release(struct inode *inode, struct file *filp)
{
	int ret = 0;
	int call_disable = 0;
	struct cxd224x_dev *cxd224x_dev = filp->private_data;

	mutex_lock(&cxd224x_dev->lock);
	cxd224x_dev->users--;
	if (!cxd224x_dev->users)
	{
		call_disable = 1;
	}
	mutex_unlock(&cxd224x_dev->lock);
	if (call_disable)
		cxd224x_disable_irq(cxd224x_dev);

	hwlog_info("release %d,%d users=%d\n", imajor(inode), iminor(inode), cxd224x_dev->users);

	return ret;
}

static long cxd224x_dev_unlocked_ioctl(struct file *filp,
					 unsigned int cmd, unsigned long arg)
{
	struct cxd224x_dev *cxd224x_dev = filp->private_data;
	switch (cmd) {
	case CXDNFC_RST_CTL:
#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
		return (queue_work(cxd224x_dev->wqueue, &cxd224x_dev->qmsg) ? 0 : 1);
#endif
		break;
	case CXDNFC_POWER_CTL:
#if defined(CONFIG_NFC_CXD224X_VEN) || defined(CONFIG_NFC_CXD224X_VEN_MODULE)
		if (arg == 0) {
			gpio_set_value(cxd224x_dev->en_gpio, 1);
		} else if (arg == 1) {
			gpio_set_value(cxd224x_dev->en_gpio, 0);
		} else {
			/* do nothing */
		}
#else
		return 1; /* not support */
#endif
		break;
	case CXDNFC_WAKE_CTL:
		if (arg == 0) {
#ifdef CONFIG_WAKELOCK
			wake_lock_timeout(&cxd224x_dev->wakelock_lp, CXD224X_WAKE_LOCK_TIMEOUT_LP*HZ);
#endif
			/* PON HIGH (normal power mode)*/
			//gpio_set_value(cxd224x_dev->gpio->wake_gpio, 1);
			//hisi_pmic_reg_write(0x240, 0x01);
			cxd224x_pon_enable(g_p_nfc_data);
		} else if (arg == 1) {
			/* PON LOW (low power mode) */
			//gpio_set_value(cxd224x_dev->gpio->wake_gpio, 0);
			//hisi_pmic_reg_write(0x240, 0x00);
			cxd224x_pon_disable(g_p_nfc_data);
#ifdef CONFIG_WAKELOCK
			wake_unlock(&cxd224x_dev->wakelock_lp);
#endif
		} else {
			/* do nothing */
		}
		break;
	default:
		hwlog_err("%s, unknown cmd (%x, %lx)\n", __func__, cmd, arg);
		return 0;
	}

	return 0;
}


static int cxd224x_pon_low_beforepwd(struct notifier_block *this, unsigned long code, void *unused)
{
	int retval = 0;

	hwlog_info("[%s]: enter!\n", __func__);
	retval = cxd224x_pon_disable(g_p_nfc_data);
	if (retval < 0) {
	hwlog_err("[%s,%d]:cxd224x_pon_disable failed; ret:%d\n", __func__, __LINE__, retval);
	}
	return retval;
}

static struct notifier_block cxd224x_pon_low_notifier = {
	.notifier_call = cxd224x_pon_low_beforepwd,
};

static const struct file_operations cxd224x_dev_fops = {
	.owner = THIS_MODULE,
	.llseek = no_llseek,
	.poll = cxd224x_dev_poll,
	.read = cxd224x_dev_read,
	.write = cxd224x_dev_write,
	.open = cxd224x_dev_open,
	.release = cxd224x_dev_release,
	.unlocked_ioctl = cxd224x_dev_unlocked_ioctl
};

#if defined(CONFIG_OF)
static int cxd224x_parse_dt(struct device *dev,
		struct cxd224x_platform_data *pdata)
{
	int ret=0;

	init_nfc_cfg_data_from_dts(g_p_nfc_data);

	/*nfc_int*/
	pdata->irq_gpio =  of_get_named_gpio_flags(dev->of_node, "sony,nfc_int", 0,NULL);
	if (pdata->irq_gpio < 0) {
		hwlog_err( "failed to get \"nfc_int\"\n");
		goto dt_err;
	}

#if defined(CONFIG_NFC_CXD224X_VEN) || defined(CONFIG_NFC_CXD224X_VEN_MODULE)
	pdata->en_gpio = of_get_named_gpio_flags(dev->of_node, "sony,nfc_ven", 0,NULL);
	if (pdata->en_gpio< 0) {
		hwlog_err( "failed to get \"nfc_ven\"\n");
		goto dt_err;
	}
#endif

#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
	pdata->rst_gpio = of_get_named_gpio_flags(dev->of_node, "sony,nfc_rst", 0,NULL);
	if (pdata->rst_gpio< 0) {
		hwlog_err( "failed to get \"nfc_rst\"\n");
		goto dt_err;
	}
#endif
	if (g_p_nfc_data->ven_on_gpio_type == NFCC_ON_BY_GPIO) {
		pdata->wake_gpio = of_get_named_gpio_flags(dev->of_node, "sony,nfc_wake", 0,NULL);
		if (pdata->wake_gpio< 0) {
			hwlog_err( "failed to get \"nfc_wake\"\n");
			goto dt_err;
		}
	} else {
		hwlog_info("nfc wakeup gpio by hisi pmic!\n");
	}

	return 0;

dt_err:
	return ret;
}
#endif

static int cxd224x_probe(struct i2c_client *client,
			   const struct i2c_device_id *id)
{
	int ret;
	struct cxd224x_platform_data *platform_data=NULL;
	struct cxd224x_dev *cxd224x_dev;
	int irq_gpio_ok  = 0;
#if defined(CONFIG_NFC_CXD224X_VEN) || defined(CONFIG_NFC_CXD224X_VEN_MODULE)
	int en_gpio_ok	 = 0;
#endif
#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
	int rst_gpio_ok = 0;
#endif
	int wake_gpio_ok = 0;

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		hwlog_err("need I2C_FUNC_I2C\n");
		return -ENODEV;
	}

	register_nfc_dmd_client();

	g_p_nfc_data = kzalloc(sizeof(struct huawei_nfc_data),
		GFP_KERNEL);
	if (g_p_nfc_data == NULL) {
		hwlog_err("g_nfc_data failed to allocate memory\n");
		return -ENOMEM;
	}
	init_nfc_info_data(g_p_nfc_data);

#if defined(CONFIG_OF)
	platform_data = kzalloc(sizeof(struct cxd224x_platform_data),
		GFP_KERNEL);
	if (platform_data == NULL) {
		hwlog_err("failed to allocate memory\n");
		return -ENOMEM;
	}
	ret = cxd224x_parse_dt(&client->dev, platform_data);
	if (ret) {
			hwlog_err("failed to parse device tree\n");
			kfree(platform_data);
			return -ENODEV;
	}
#else
	platform_data = client->dev.platform_data;

	hwlog_info("%s, probing cxd224x driver flags = %x\n", __func__, client->flags);
	if (platform_data == NULL) {
		hwlog_err("nfc probe fail\n");
		return -ENODEV;
	}
#endif
	hwlog_info("%s, rst_gpio(%d)\n", __func__, platform_data->rst_gpio);
	hwlog_info("%s, ven_gpio(%d)\n", __func__, platform_data->en_gpio);
	hwlog_info("%s, irq_gpio(%d)\n", __func__, platform_data->irq_gpio);
	hwlog_info("%s, wake_gpio(%d)\n", __func__, platform_data->wake_gpio);

	irq_gpio_ok=1;
	client->irq = gpio_to_irq(platform_data->irq_gpio);
	if (client->irq<0)
	{
		hwlog_err("%s, failed to allocate irq=%d\n", __func__, client->irq);
		return -ENODEV;
	}
	hwlog_info("%s, irq(%d)\n", __func__, client->irq);

	ret = create_sysfs_interfaces(&client->dev);
	if (ret < 0) {
		hwlog_err("[%s,%d]:Failed to create_sysfs_interfaces\n", __func__, __LINE__);
		return -ENODEV;
	}
	ret = sysfs_create_link(NULL, &client->dev.kobj, "nfc");
	if (ret < 0) {
		hwlog_err("[%s,%d]:Failed to sysfs_create_link\n", __func__, __LINE__);
		return -ENODEV;
	}

#if defined(CONFIG_NFC_CXD224X_VEN) || defined(CONFIG_NFC_CXD224X_VEN_MODULE)
	ret = gpio_request_one(platform_data->en_gpio, GPIOF_OUT_INIT_LOW, "nfc_cen");
	if (ret)
		goto err_exit;
	en_gpio_ok=1;
	ret = gpio_direction_output(platform_data->en_gpio, 0);
	if (ret)
		return -ENODEV;
#endif

#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
	ret = gpio_request_one(platform_data->rst_gpio, GPIOF_OUT_INIT_HIGH, "nfc_rst");
	if (ret)
		goto err_exit;
	rst_gpio_ok=1;
	ret = gpio_direction_output(platform_data->rst_gpio, ~CXDNFC_RST_ACTIVE & 0x1);
	if (ret)
		return -ENODEV;
	hwlog_info("%s, xrst deassert\n", __func__);
#endif

	if (g_p_nfc_data->ven_on_gpio_type == NFCC_ON_BY_GPIO) {
		ret = gpio_request_one(platform_data->wake_gpio, GPIOF_OUT_INIT_LOW, "nfc_wake");
		if (ret)
			goto err_exit;
		wake_gpio_ok=1;
		ret = gpio_direction_output(platform_data->wake_gpio,0);
	}

	cxd224x_dev = kzalloc(sizeof(*cxd224x_dev), GFP_KERNEL);
	if (cxd224x_dev == NULL) {
		hwlog_err("failed to allocate memory for module data\n");
		ret = -ENOMEM;
		goto err_exit;
	}

	cxd224x_dev->client = client;
	cxd224x_dev->gpio = platform_data;
#ifdef CONFIG_WAKELOCK
	wake_lock_init(&cxd224x_dev->wakelock, WAKE_LOCK_SUSPEND, CXD224X_WAKE_LOCK_NAME);
	wake_lock_init(&cxd224x_dev->wakelock_lp, WAKE_LOCK_SUSPEND, CXD224X_WAKE_LOCK_NAME_LP);
#endif
	cxd224x_dev->users =0;

	/* init mutex and queues */
	init_waitqueue_head(&cxd224x_dev->read_wq);
	mutex_init(&cxd224x_dev->read_mutex);
	mutex_init(&cxd224x_dev->lock);
	spin_lock_init(&cxd224x_dev->irq_enabled_lock);

#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
	if (init_wqueue(cxd224x_dev) != 0) {
		hwlog_err("init workqueue failed\n");
		goto err_exit;
	}
#endif

	cxd224x_dev->cxd224x_device.minor = MISC_DYNAMIC_MINOR;
	cxd224x_dev->cxd224x_device.name = "cxd224x-i2c";
	cxd224x_dev->cxd224x_device.fops = &cxd224x_dev_fops;

	ret = misc_register(&cxd224x_dev->cxd224x_device);
	if (ret) {
		hwlog_err("misc_register failed\n");
		goto err_misc_register;
	}

	/* request irq.  the irq is set whenever the chip has data available
	 * for reading.  it is cleared when all data has been read.
	 */
	hwlog_info("requesting IRQ %d\n", client->irq);
	cxd224x_dev->irq_enabled = true;
	ret = request_irq(client->irq, cxd224x_dev_irq_handler,
			IRQF_TRIGGER_FALLING, client->name, cxd224x_dev);
	if (ret) {
		hwlog_err("request_irq failed\n");
		goto err_request_irq_failed;
	}
	cxd224x_disable_irq(cxd224x_dev);
	i2c_set_clientdata(client, cxd224x_dev);
	/*pull PON down, when turn on or off*/
	cxd224x_pon_disable(g_p_nfc_data);
	/*notifier for supply shutdown*/
	register_reboot_notifier(&cxd224x_pon_low_notifier);
#ifdef CONFIG_HUAWEI_HW_DEV_DCT
	/* detect current device successful, set the flag as present */
	set_hw_dev_flag(DEV_I2C_NFC);
#endif
	g_p_nfc_data->fw_update = 1;
	hwlog_info("%s, probing cxd224x driver exited successfully\n", __func__);
	return 0;

err_request_irq_failed:
	misc_deregister(&cxd224x_dev->cxd224x_device);
err_misc_register:
	mutex_destroy(&cxd224x_dev->read_mutex);
	kfree(cxd224x_dev);
err_exit:
	if(irq_gpio_ok)
		gpio_free(platform_data->irq_gpio);
#if defined(CONFIG_NFC_CXD224X_VEN) || defined(CONFIG_NFC_CXD224X_VEN_MODULE)
	if(en_gpio_ok)
		gpio_free(platform_data->en_gpio);
#endif
#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
	if(rst_gpio_ok)
		gpio_free(platform_data->rst_gpio);
#endif
	if(wake_gpio_ok)
		gpio_free(platform_data->wake_gpio);

#if defined(CONFIG_OF)
		if(platform_data)
			kfree(platform_data);
#endif
	remove_sysfs_interfaces(&client->dev);
	return ret;
}

static int cxd224x_remove(struct i2c_client *client)
{
	struct cxd224x_dev *cxd224x_dev;

	unregister_reboot_notifier(&cxd224x_pon_low_notifier);
	cxd224x_dev = i2c_get_clientdata(client);
#ifdef CONFIG_WAKELOCK
	wake_lock_destroy(&cxd224x_dev->wakelock);
	wake_lock_destroy(&cxd224x_dev->wakelock_lp);
#endif
	free_irq(client->irq, cxd224x_dev);
	misc_deregister(&cxd224x_dev->cxd224x_device);
	mutex_destroy(&cxd224x_dev->read_mutex);
		if(cxd224x_dev->gpio)
		{
			gpio_free(cxd224x_dev->gpio->irq_gpio);
			if (g_p_nfc_data->ven_on_gpio_type == NFCC_ON_BY_GPIO) {
				gpio_free(cxd224x_dev->gpio->wake_gpio);
			}

#if defined(CONFIG_NFC_CXD224X_VEN) || defined(CONFIG_NFC_CXD224X_VEN_MODULE)
			gpio_free(cxd224x_dev->gpio->en_gpio);
#endif
#if defined(CONFIG_NFC_CXD224X_RST) || defined(CONFIG_NFC_CXD224X_RST_MODULE)
			gpio_free(cxd224x_dev->gpio->rst_gpio);
#endif

#if defined(CONFIG_OF)

			kfree(cxd224x_dev->gpio);
#endif
		}
	remove_sysfs_interfaces(&client->dev);
	kfree(g_p_nfc_data);
	kfree(cxd224x_dev);

	return 0;
}

#ifdef CONFIG_PM_SLEEP
static int cxd224x_suspend(struct device *dev)
{
		struct i2c_client *client = to_i2c_client(dev);
		struct cxd224x_dev *cxd224x_dev = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev)) {
		int irq = gpio_to_irq(cxd224x_dev->gpio->irq_gpio);
		enable_irq_wake(irq);
	}
	return 0;
}

static int cxd224x_resume(struct device *dev)
{
		struct i2c_client *client = to_i2c_client(dev);
		struct cxd224x_dev *cxd224x_dev = i2c_get_clientdata(client);

	if (device_may_wakeup(&client->dev)) {
		int irq = gpio_to_irq(cxd224x_dev->gpio->irq_gpio);
		disable_irq_wake(irq);
	}
	return 0;
}

static const struct dev_pm_ops cxd224x_pm_ops = {
	.suspend	= cxd224x_suspend,
	.resume		= cxd224x_resume,
};
#endif

static struct of_device_id cxd224x_i2c_dt_ids[] = {
	{.compatible = "hisilicon,cxd224x_nfc" },
	{}
};

static const struct i2c_device_id cxd224x_id[] = {
	{"cxd224x-i2c", 0},
	{}
};
MODULE_DEVICE_TABLE(of, cxd224x_i2c_dt_ids);

static struct i2c_driver cxd224x_driver = {
	.id_table = cxd224x_id,
	.probe = cxd224x_probe,
	.remove = cxd224x_remove,
	.driver = {
		.owner = THIS_MODULE,
		.name = "cxd224x-i2c",
//#ifdef CONFIG_PM
//		.pm = &cxd224x_pm_ops,
//#endif
		.of_match_table = of_match_ptr(cxd224x_i2c_dt_ids),
	},
};

/*
 * module load/unload record keeping
 */

static int __init cxd224x_dev_init(void)
{
	return i2c_add_driver(&cxd224x_driver);
}
module_init(cxd224x_dev_init);

static void __exit cxd224x_dev_exit(void)
{
	i2c_del_driver(&cxd224x_driver);
}
module_exit(cxd224x_dev_exit);

MODULE_AUTHOR("Sony");
MODULE_DESCRIPTION("NFC cxd224x driver");
MODULE_LICENSE("GPL");
