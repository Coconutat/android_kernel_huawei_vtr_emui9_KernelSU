/*
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 4017 $
 * $Date: 2016-04-01 09:41:08 +0800 (星期五, 01 四月 2016) $
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
 */

#include <linux/proc_fs.h>
#include <linux/unistd.h>

#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <asm/uaccess.h>
#include <linux/slab.h>

#include <asm/uaccess.h>
#include <linux/delay.h>

#include <linux/regulator/consumer.h>
#include <huawei_platform/log/log_jank.h>
#include "../../huawei_ts_kit_algo.h"
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>
#endif

#include "nt36xxx.h"


#define NVT_FW_VERSION "nvt_fw_version"
#define NVT_BASELINE "nvt_baseline"
#define NVT_RAW "nvt_raw"
#define NVT_DIFF "nvt_diff"

#define I2C_TRANSFER_LENGTH  64
#define SPI_TRANSFER_LENGTH  256

#define NORMAL_MODE 0x00
#define TEST_MODE_1 0x21
#define TEST_MODE_2 0x22
#define HANDSHAKING_HOST_READY 0xBB

#define XDATA_SECTOR_SIZE   256

static uint8_t xdata_tmp[2048] = {0};
static int32_t xdata[2048] = {0};
uint8_t nvt_fw_ver = 0;
static uint8_t x_num = 0;
static uint8_t y_num = 0;
static uint8_t button_num = 0;
static uint32_t NVT_TRANSFER_LENGTH = I2C_TRANSFER_LENGTH;	//Default suppot I2C

static struct proc_dir_entry *NVT_proc_fw_version_entry;
static struct proc_dir_entry *NVT_proc_baseline_entry;
static struct proc_dir_entry *NVT_proc_raw_entry;
static struct proc_dir_entry *NVT_proc_diff_entry;

extern struct nvt_ts_data *nvt_ts;
extern int32_t novatek_ts_kit_read(uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern int32_t novatek_ts_kit_dummy_read(uint16_t i2c_addr);
extern int32_t novatek_ts_kit_write(uint16_t i2c_addr, uint8_t *buf, uint16_t len);
extern int32_t nvt_kit_clear_fw_status(void);
extern int32_t nvt_kit_check_fw_status(void);
//extern struct ts_data g_ts_data;



/*******************************************************
  Create Device Node (Proc Entry)
*******************************************************/
#if NVT_TOUCH_PROC
static struct proc_dir_entry *NVT_proc_entry;
#define DEVICE_I2C_NAME	"NVTflash"

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash read function.

return:
	Executive outcomes. 2---succeed. -5,-14---failed.
*******************************************************/
static ssize_t nvt_flash_i2c_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
	uint8_t str[68] = {0};
	int32_t ret = -1;
	int32_t retries = 0;
	int8_t i2c_wr = 0;
	u16 tmp_addr = 0;

	if (count > sizeof(str)) {
		TS_LOG_ERR("error count=%zu\n", count);
		return -EFAULT;
	}

	if (copy_from_user(str, buff, count)) {
		TS_LOG_ERR("copy from user error\n");
		return -EFAULT;
	}

	i2c_wr = str[0] >> 7;

	if (i2c_wr == 0) {	//I2C write
		TS_LOG_DEBUG("%s: i2c_addr=0x%02X, len=%d\n", __func__, (str[0] & 0x7F), str[1]);

		if (!nvt_ts->chip_data->ts_platform_data->bops->bus_read) {
			TS_LOG_ERR("%s: error, invalid bus_write\n", __func__);
			return -EIO;
		}

		mutex_lock(&nvt_ts->bus_mutex);
		tmp_addr = nvt_ts->chip_data->ts_platform_data->client->addr;
		nvt_ts->chip_data->ts_platform_data->client->addr = (str[0] & 0x7F);

		ret = nvt_ts->chip_data->ts_platform_data->bops->bus_write(&str[2], str[1]);
		if (ret < 0)
			TS_LOG_ERR("%s: error, bus_write fail, ret=%d\n", __func__, ret);

		nvt_ts->chip_data->ts_platform_data->client->addr = tmp_addr;
		mutex_unlock(&nvt_ts->bus_mutex);

		return ret;
	} else if (i2c_wr == 1) {	//I2C read
		TS_LOG_DEBUG("%s: i2c_addr=0x%02X, len=%d\n", __func__, (str[0] & 0x7F), str[1]);

		if (!nvt_ts->chip_data->ts_platform_data->bops->bus_read) {
			TS_LOG_ERR("%s: error, invalid bus_read\n", __func__);
			return -EIO;
		}

		mutex_lock(&nvt_ts->bus_mutex);
		tmp_addr = nvt_ts->chip_data->ts_platform_data->client->addr;
		nvt_ts->chip_data->ts_platform_data->client->addr = (str[0] & 0x7F);

		ret = nvt_ts->chip_data->ts_platform_data->bops->bus_read(&str[2], 1, &str[3], (str[1] - 1));
		if (ret < 0)
			TS_LOG_ERR("%s: error, bus_read fail, ret=%d\n", __func__, ret);

		nvt_ts->chip_data->ts_platform_data->client->addr = tmp_addr;
		mutex_unlock(&nvt_ts->bus_mutex);

		// copy buff to user if i2c transfer
		if (retries < 20) {
			if (copy_to_user(buff, str, count))
				return -EFAULT;
		}

		return ret;
	} else {
		TS_LOG_ERR("%s: Call error, str[0]=%d\n", __func__, str[0]);
		return -EFAULT;
	}
}

static struct proc_dir_entry *NVT_proc_entry;
#define DEVICE_SPI_NAME	"NVTSPI"

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTSPI read function.

return:
	Executive outcomes. 2---succeed. -5,-14---failed.
*******************************************************/
static ssize_t nvt_flash_spi_read(struct file *file, char __user *buff, size_t count, loff_t *offp)
{
	uint8_t *str = NULL;
	int32_t ret = NO_ERR;
	int32_t retries = 0;
	int8_t spi_wr = 0;

	if (count > NVT_TANSFER_LEN) {
		TS_LOG_ERR("invalid transfer len!\n");
		return -EFAULT;
	}

	/* allocate buffer for spi transfer */
	str = (uint8_t *)kzalloc((count), GFP_KERNEL);
	if(str == NULL) {
		TS_LOG_ERR("kzalloc for buf failed!\n");
		ret = -ENOMEM;
		goto kzalloc_failed;
	}

	if (copy_from_user(str, buff, count)) {
		TS_LOG_ERR("copy from user error\n");
		ret = -EFAULT;
		goto out;
	}

#if defined(NVT_QEEXO_EARSENSE)
	cancel_delayed_work_sync(&nvt_proximity_work);
#endif

	spi_wr = str[0] >> 7;

	if (spi_wr == NVTWRITE) {	//SPI write
		if (!nvt_ts->chip_data->ts_platform_data->bops->bus_write) {
			TS_LOG_ERR("%s: error, invalid bus_write\n", __func__);
			ret = -EIO;
			goto out;
		}

		ret = novatek_ts_kit_write(I2C_FW_Address, &str[2], ((str[0]&0x7F) << 8) | str[1]);
		if (ret < 0) {
			TS_LOG_ERR("%s: error, bus_write fail, ret=%d\n", __func__, ret);
		}

		goto out;
	} else if (spi_wr == NVTREAD) {	//SPI read
		if (!nvt_ts->chip_data->ts_platform_data->bops->bus_read) {
			TS_LOG_ERR("%s: error, invalid bus_read\n", __func__);
			ret = -EIO;
			goto out;
		}

		ret = novatek_ts_kit_read(I2C_FW_Address, &str[2], (((str[0] & 0x7F) << 8) | str[1]));
		if (ret < 0) {
			TS_LOG_ERR("%s: error, bus_read fail, ret=%d\n", __func__, ret);
		}

		// copy buff to user if spi transfer
		if (retries < 20) {
			if (copy_to_user(buff, str, count)) {
				ret = -EFAULT;
				goto out;
			}
		}

		goto out;
	} else {
		TS_LOG_ERR("%s: Call error, str[0]=%d\n", __func__, str[0]);
		ret = -EFAULT;
		goto out;
	}

out:
	if (str) {
		kfree(str);
	}
kzalloc_failed:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash open function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
static int32_t nvt_flash_open(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev;

	dev = kmalloc(sizeof(struct nvt_flash_data), GFP_KERNEL);
	if (dev == NULL) {
		TS_LOG_ERR("%s: Failed to allocate memory for nvt flash data\n", __func__);
		return -ENOMEM;
	}

	rwlock_init(&dev->lock);
	file->private_data = dev;

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash close function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_flash_close(struct inode *inode, struct file *file)
{
	struct nvt_flash_data *dev = file->private_data;

	if (dev)
		kfree(dev);

	return 0;
}

static const struct file_operations nvt_flash_i2c_fops = {
	.owner = THIS_MODULE,
	.open = nvt_flash_open,
	.release = nvt_flash_close,
	.read = nvt_flash_i2c_read,
};

static const struct file_operations nvt_flash_spi_fops = {
	.owner = THIS_MODULE,
	.open = nvt_flash_open,
	.release = nvt_flash_close,
	.read = nvt_flash_spi_read,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/NVTflash initial function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
int32_t nvt_kit_flash_proc_init(void)
{
	if (nvt_ts->btype == TS_BUS_I2C) {
		NVT_proc_entry = proc_create(DEVICE_I2C_NAME, 0444, NULL,&nvt_flash_i2c_fops);
	} else if (nvt_ts->btype == TS_BUS_SPI) {
		NVT_proc_entry = proc_create(DEVICE_SPI_NAME, 0444, NULL,&nvt_flash_spi_fops);
	}
	if (NVT_proc_entry == NULL) {
		TS_LOG_ERR("%s: Failed!\n", __func__);
		return -ENOMEM;
	} else {
		TS_LOG_INFO("%s: Succeeded!\n", __func__);
	}

	TS_LOG_INFO("============================================================\n");
	if (nvt_ts->btype == TS_BUS_I2C) {
		NVT_TRANSFER_LENGTH = I2C_TRANSFER_LENGTH;
		TS_LOG_INFO("Create /proc/NVTflash\n");
	} else if (nvt_ts->btype == TS_BUS_SPI) {
		NVT_TRANSFER_LENGTH = SPI_TRANSFER_LENGTH;	//Support SPI transfer length
		TS_LOG_INFO("Create /proc/NVTSPI\n");
	}
	TS_LOG_INFO("============================================================\n");

	return 0;
}
#endif



/*******************************************************
  Create Device Node (Ext Proc Entry)
*******************************************************/
#if NVT_TOUCH_EXT_PROC
/*******************************************************
Description:
	Novatek touchscreen change mode function.

return:
	n.a.
*******************************************************/
void nvt_kit_change_mode(uint8_t mode)
{
	uint8_t buf[8] = {0};
	int32_t ret = 0;

	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HOST_CMD);

	//---set mode---
	buf[0] = EVENT_MAP_HOST_CMD;
	buf[1] = mode;
	ret = novatek_ts_kit_write(I2C_FW_Address, buf, 2);
	if (ret) {
		TS_LOG_INFO("%s: set mode FAIL\n", __func__);
	}

	if (mode == NORMAL_MODE) {
		buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
		buf[1] = HANDSHAKING_HOST_READY;
		ret = novatek_ts_kit_write(I2C_FW_Address, buf, 2);
		if (ret) {
			TS_LOG_INFO("%s: in NORMAL_MODE set mode FAIL\n", __func__);
		}
		msleep(20);
	}
}
/*******************************************************
Description:
	Novatek touchscreen get firmware related information
	function.

return:
	Executive outcomes. 0---success. -1---fail.
*******************************************************/
int8_t nvt_kit_get_fw_info(void)
{
	uint8_t buf[64] = {0};
	uint32_t retry_count = 0;
	int8_t ret = 0;
	int32_t ret_32 = 0;

info_retry:
	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_FWINFO);

	//---read fw info---
	buf[0] = EVENT_MAP_FWINFO;
	ret_32 = novatek_ts_kit_read(I2C_FW_Address, buf, 17);
	if (ret_32) {
		TS_LOG_INFO("%s:read fw info FAIL\n", __func__);
	}

	nvt_fw_ver = buf[1];
	x_num = buf[3];
	y_num = buf[4];
	nvt_ts->x_num = x_num;
	nvt_ts->y_num = y_num;
	button_num = buf[11];

	TS_LOG_INFO("%s: nvt_fw_ver=0x%02X\n", __func__, nvt_fw_ver);
	//---clear x_num, y_num if fw info is broken---
	if ((buf[1] + buf[2]) != 0xFF) {
		TS_LOG_ERR("%s: FW info is broken! nvt_fw_ver=0x%02X, ~nvt_fw_ver=0x%02X\n", __func__, buf[1], buf[2]);
		x_num = 0;
		y_num = 0;
		button_num = 0;

		if(retry_count < 3) {
			retry_count++;
			TS_LOG_ERR("%s: retry_count=%d\n", __func__, retry_count);
			goto info_retry;
		} else {
			ret = -1;
		}
	} else {
		ret = 0;
	}

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen get firmware pipe function.

return:
	Executive outcomes. 0---pipe 0. 1---pipe 1.
*******************************************************/
uint8_t nvt_kit_get_fw_pipe(void)
{
	uint8_t buf[8]= {0};
	int32_t ret = 0;

	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR | EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE);

	//---read fw status---
	buf[0] = EVENT_MAP_HANDSHAKING_or_SUB_CMD_BYTE;
	buf[1] = 0x00;
	ret = novatek_ts_kit_read(I2C_FW_Address, buf, 2);
	if (ret) {
		TS_LOG_INFO("%s: read fw status FAIL\n", __func__);
	}
	//TS_LOG_INFO("FW pipe=%d, buf[1]=0x%02X\n", (buf[1]&0x01), buf[1]);

	return (buf[1] & 0x01);
}

/*******************************************************
Description:
	Novatek touchscreen read meta data function.

return:
	n.a.
*******************************************************/
void nvt_kit_read_mdata(uint32_t xdata_addr)
{
	int32_t i = 0;
	int32_t j = 0;
	int32_t k = 0;
	uint8_t buf[SPI_TRANSFER_LENGTH + 1] = {0};
	uint32_t head_addr = 0;
	int32_t dummy_len = 0;
	int32_t data_len = 0;
	int32_t residual_len = 0;
	int32_t ret = 0;

	//---set xdata sector address & length---
	head_addr = xdata_addr - (xdata_addr % XDATA_SECTOR_SIZE);
	dummy_len = xdata_addr - head_addr;
	data_len = x_num * y_num * 2;
	residual_len = (head_addr + dummy_len + data_len) % XDATA_SECTOR_SIZE;

	//printk("head_addr=0x%05X, dummy_len=0x%05X, data_len=0x%05X, residual_len=0x%05X\n", head_addr, dummy_len, data_len, residual_len);

	//read xdata : step 1
	for (i = 0; i < ((dummy_len + data_len) / XDATA_SECTOR_SIZE); i++) {
		if (nvt_ts->btype == TS_BUS_I2C) {
			//---change xdata index---
			nvt_set_page(head_addr + (XDATA_SECTOR_SIZE * i));
		}

		//---read xdata by NVT_TRANSFER_LENGTH
		for (j = 0; j < (XDATA_SECTOR_SIZE / NVT_TRANSFER_LENGTH); j++) {
			if (nvt_ts->btype == TS_BUS_SPI) {
				//---change xdata index---
				nvt_set_page(head_addr + (XDATA_SECTOR_SIZE * i) + (NVT_TRANSFER_LENGTH * j));
			}

			//---read data---
			buf[0] = NVT_TRANSFER_LENGTH * j;
			ret = novatek_ts_kit_read(I2C_FW_Address, buf, NVT_TRANSFER_LENGTH + 1);
			if (ret) {
				TS_LOG_INFO("%s: read data 1st FAIL\n", __func__);
			}

			//---copy buf to xdata_tmp---
			for (k = 0; k < NVT_TRANSFER_LENGTH; k++) {
				xdata_tmp[XDATA_SECTOR_SIZE * i + NVT_TRANSFER_LENGTH * j + k] = buf[k + 1];
				//printk("0x%02X, 0x%04X\n", buf[k+1], (XDATA_SECTOR_SIZE*i + NVT_TRANSFER_LENGTH*j + k));
			}
		}
		//printk("addr=0x%05X\n", (head_addr+XDATA_SECTOR_SIZE*i));
	}

	//read xdata : step2
	if (residual_len != 0) {
		if (nvt_ts->btype == TS_BUS_I2C) {
			//---change xdata index---
			nvt_set_page(xdata_addr + data_len - residual_len);
		}

		//---read xdata by NVT_TRANSFER_LENGTH
		for (j = 0; j < (residual_len / NVT_TRANSFER_LENGTH + 1); j++) {
			if (nvt_ts->btype == TS_BUS_SPI) {
				//---change xdata index---
				nvt_set_page(xdata_addr + data_len - residual_len + (NVT_TRANSFER_LENGTH * j));
			}

			//---read data---
			buf[0] = NVT_TRANSFER_LENGTH * j;
			ret = novatek_ts_kit_read(I2C_FW_Address, buf, NVT_TRANSFER_LENGTH + 1);
			if (ret) {
				TS_LOG_INFO("%s: read data 2rd FAIL\n", __func__);
			}

			//---copy buf to xdata_tmp---
			for (k = 0; k < NVT_TRANSFER_LENGTH; k++) {
				xdata_tmp[(dummy_len + data_len - residual_len) + NVT_TRANSFER_LENGTH * j + k] = buf[k + 1];
				//printk("0x%02X, 0x%04x\n", buf[k+1], ((dummy_len+data_len-residual_len) + NVT_TRANSFER_LENGTH*j + k));
			}
		}
		//printk("addr=0x%05X\n", (xdata_addr+data_len-residual_len));
	}

	//---remove dummy data and 2bytes-to-1data---
	for (i = 0; i < (data_len / 2); i++) {
		xdata[i] = (xdata_tmp[dummy_len + i * 2] + 256 * xdata_tmp[dummy_len + i * 2 + 1]);
	}

#if TOUCH_KEY_NUM > 0
		//read button xdata : step3
		//---change xdata index---
		nvt_set_page(xdata_btn_addr);

		//---read data---
		buf[0] = (xdata_btn_addr & 0xFF);
		ret = novatek_ts_kit_read(I2C_FW_Address, buf, (TOUCH_KEY_NUM * 2 + 1));
		if (ret) {
			TS_LOG_INFO("%s: read data 3rd FAIL\n", __func__);
		}

		//---2bytes-to-1data---
		for (i = 0; i < TOUCH_KEY_NUM; i++) {
			xdata[x_num * y_num + i] = (buf[1 + i * 2] + 256 * buf[1 + i * 2 + 1]);
		}
#endif

	//---set xdata index to EVENT BUF ADDR---
	nvt_set_page(nvt_ts->mmap->EVENT_BUF_ADDR);
}

/*******************************************************
Description:
    Novatek touchscreen get meta data function.

return:
    n.a.
*******************************************************/
void nvt_kit_get_mdata(int32_t *buf, uint8_t *m_x_num, uint8_t *m_y_num)
{
    *m_x_num = x_num;
    *m_y_num = y_num;
    memcpy(buf, xdata, (x_num * y_num * 4));
}

/*******************************************************
Description:
	Novatek touchscreen firmware version show function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t c_fw_version_show(struct seq_file *m, void *v)
{
	seq_printf(m, "nvt_fw_ver=%d, x_num=%d, y_num=%d, button_num=%d\n", nvt_fw_ver, x_num, y_num, button_num);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print show
	function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t c_show(struct seq_file *m, void *v)
{
	int32_t i = 0;
	int32_t j = 0;

	for (i = 0; i < y_num; i++) {
		for (j = 0; j < x_num; j++) {
			seq_printf(m, "%5d, ", (int16_t)xdata[i * x_num + j]);
		}
		seq_puts(m, "\n");
	}
	seq_printf(m, "\n\n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print start
	function.

return:
	Executive outcomes. 1---call next function.
	NULL---not call next function and sequence loop
	stop.
*******************************************************/
static void *c_start(struct seq_file *m, loff_t *pos)
{
	return *pos < 1 ? (void *)1 : NULL;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print next
	function.

return:
	Executive outcomes. NULL---no next and call sequence
	stop function.
*******************************************************/
static void *c_next(struct seq_file *m, void *v, loff_t *pos)
{
	++*pos;
	return NULL;
}

/*******************************************************
Description:
	Novatek touchscreen xdata sequence print stop
	function.

return:
	n.a.
*******************************************************/
static void c_stop(struct seq_file *m, void *v)
{
	return;
}

static const struct seq_operations nvt_fw_version_seq_ops = {
	.start  = c_start,
	.next   = c_next,
	.stop   = c_stop,
	.show   = c_fw_version_show
};

static const struct seq_operations nvt_seq_ops = {
	.start  = c_start,
	.next   = c_next,
	.stop   = c_stop,
	.show   = c_show
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_fw_version open
	function.

return:
	n.a.
*******************************************************/
static int32_t nvt_fw_version_open(struct inode *inode, struct file *file)
{

	if (nvt_kit_get_fw_info())
		return -EAGAIN;

	return seq_open(file, &nvt_fw_version_seq_ops);
}

static const struct file_operations nvt_fw_version_fops = {
	.owner = THIS_MODULE,
	.open = nvt_fw_version_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_baseline open function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_baseline_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&nvt_ts->lock)) {
		return -ERESTARTSYS;
	}

	if (nvt_kit_clear_fw_status()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	nvt_kit_change_mode(TEST_MODE_2);

	if (nvt_kit_check_fw_status()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_info()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	nvt_kit_read_mdata(nvt_ts->mmap->BASELINE_ADDR);

	nvt_kit_change_mode(NORMAL_MODE);

	mutex_unlock(&nvt_ts->lock);

	return seq_open(file, &nvt_seq_ops);
}

static const struct file_operations nvt_baseline_fops = {
	.owner = THIS_MODULE,
	.open = nvt_baseline_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_raw open function.

return:
	Executive outcomes. 0---succeed.
*******************************************************/
static int32_t nvt_raw_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&nvt_ts->lock)) {
		return -ERESTARTSYS;
	}

	if (nvt_kit_clear_fw_status()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	nvt_kit_change_mode(TEST_MODE_2);

	if (nvt_kit_check_fw_status()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_info()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	if (nvt_kit_get_fw_pipe() == 0)
		nvt_kit_read_mdata(nvt_ts->mmap->RAW_PIPE0_ADDR);
	else
		nvt_kit_read_mdata(nvt_ts->mmap->RAW_PIPE1_ADDR);

	nvt_kit_change_mode(NORMAL_MODE);

	mutex_unlock(&nvt_ts->lock);

	return seq_open(file, &nvt_seq_ops);
}

static const struct file_operations nvt_raw_fops = {
	.owner = THIS_MODULE,
	.open = nvt_raw_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen /proc/nvt_diff open function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
static int32_t nvt_diff_open(struct inode *inode, struct file *file)
{
	if (mutex_lock_interruptible(&nvt_ts->lock)) {
		return -ERESTARTSYS;
	}

	if (nvt_kit_clear_fw_status()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	nvt_kit_change_mode(TEST_MODE_2);

	if (nvt_kit_check_fw_status()) {
		mutex_unlock(&nvt_ts->lock);
		return -EAGAIN;
	}

	if(nvt_kit_get_fw_info())
	{
		TS_LOG_INFO("get_fw_info fail\n");
	}

	if (nvt_kit_get_fw_pipe() == 0)
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE0_ADDR);
	else
		nvt_kit_read_mdata(nvt_ts->mmap->DIFF_PIPE1_ADDR);

	nvt_kit_change_mode(NORMAL_MODE);

	mutex_unlock(&nvt_ts->lock);

	return seq_open(file, &nvt_seq_ops);
}

static const struct file_operations nvt_diff_fops = {
	.owner = THIS_MODULE,
	.open = nvt_diff_open,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = seq_release,
};

/*******************************************************
Description:
	Novatek touchscreen extra function proc. file node
	initial function.

return:
	Executive outcomes. 0---succeed. -12---failed.
*******************************************************/
int32_t nvt_kit_extra_proc_init(void)
{
	NVT_proc_fw_version_entry = proc_create(NVT_FW_VERSION, 0444, NULL,&nvt_fw_version_fops);
	if (NVT_proc_fw_version_entry == NULL) {
		TS_LOG_ERR("%s: create proc/nvt_fw_version Failed!\n", __func__);
		return -ENOMEM;
	} else {
		TS_LOG_INFO("%s: create proc/nvt_fw_version Succeeded!\n", __func__);
	}

	NVT_proc_baseline_entry = proc_create(NVT_BASELINE, 0444, NULL,&nvt_baseline_fops);
	if (NVT_proc_baseline_entry == NULL) {
		TS_LOG_ERR("%s: create proc/nvt_baseline Failed!\n", __func__);
		return -ENOMEM;
	} else {
		TS_LOG_INFO("%s: create proc/nvt_baseline Succeeded!\n", __func__);
	}

	NVT_proc_raw_entry = proc_create(NVT_RAW, 0444, NULL,&nvt_raw_fops);
	if (NVT_proc_raw_entry == NULL) {
		TS_LOG_ERR("%s: create proc/nvt_raw Failed!\n", __func__);
		return -ENOMEM;
	} else {
		TS_LOG_INFO("%s: create proc/nvt_raw Succeeded!\n", __func__);
	}

	NVT_proc_diff_entry = proc_create(NVT_DIFF, 0444, NULL,&nvt_diff_fops);
	if (NVT_proc_diff_entry == NULL) {
		TS_LOG_ERR("%s: create proc/nvt_diff Failed!\n", __func__);
		return -ENOMEM;
	} else {
		TS_LOG_INFO("%s: create proc/nvt_diff Succeeded!\n", __func__);
	}

	return 0;
}
#endif
