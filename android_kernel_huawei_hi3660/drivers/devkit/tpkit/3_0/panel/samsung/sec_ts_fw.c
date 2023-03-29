/* drivers/input/touchscreen/sec_ts_fw.c
 *
 * Copyright (C) 2015 Samsung Electronics Co., Ltd.
 * http://www.samsungsemi.com/
 *
 * Core file for Samsung TSC driver
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "sec_ts.h"

#define SEC_TS_FW_BLK_SIZE		256

typedef struct {
	u32 signature;			/* signature */
	u32 version;			/* version */
	u32 totalsize;			/* total size */
	u32 checksum;			/* checksum */
	u32 img_ver;			/* image file version */
	u32 img_date;			/* image file date */
	u32 img_description;		/* image file description */
	u32 fw_ver;			/* firmware version */
	u32 fw_date;			/* firmware date */
	u32 fw_description;		/* firmware description */
	u32 para_ver;			/* parameter version */
	u32 para_date;			/* parameter date */
	u32 para_description;		/* parameter description */
	u32 num_chunk;			/* number of chunk */
	u32 reserved1;
	u32 reserved2;
} fw_header;

typedef struct {
	u32 signature;
	u32 addr;
	u32 size;
	u32 reserved;
} fw_chunk;

static int sec_ts_sw_reset(struct sec_ts_data *ts)
{
	int ret = NO_ERR;
	u8 tBuff[1] = { 0 };

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_PROTOCOL_ID, tBuff, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: protocol id read fail\n", __func__);
		return 0;
	}

	if (tBuff[0] == 0x1)
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_APP_SW_RESET, NULL, 0);
	else
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_BOOT_SW_RESET, NULL, 0);
	if (ret < 0) {
		TS_LOG_ERR("%s: write fail, sw_reset\n", __func__);
		return 0;
	}

	sec_ts_delay(100); /* ic do reset need 100 ms */

	ret = sec_ts_wait_for_ready(ts, SEC_TS_ACK_BOOT_COMPLETE);
	if (ret < 0) {
		TS_LOG_ERR("%s: time out\n", __func__);
		return 0;
	}

	TS_LOG_INFO("%s: sw_reset\n", __func__);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0) {
		TS_LOG_ERR("%s: write fail, Sense_on\n", __func__);
		return 0;
	}

	return ret;
}


static int sec_ts_enter_fw_mode(struct sec_ts_data *ts)
{
	int ret = NO_ERR;
	u8 fw_update_mode_passwd[] = {0x55, 0xAC}; /* update cmd code */
	u8 fw_status = 0;
	u8 id[3] = { 0 }; /* id length is 3 */

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_ENTER_FW_MODE, fw_update_mode_passwd, 2);
	sec_ts_delay(100); /* delay time for wait IC finish enter fw mode*/
	if (ret < 0) {
		TS_LOG_ERR("%s: write fail, enter_fw_mode\n", __func__);
		return 0;
	}

	TS_LOG_INFO("%s: write ok, enter_fw_mode - 0x%x 0x%x 0x%x\n",
		__func__, SEC_TS_CMD_ENTER_FW_MODE, fw_update_mode_passwd[0], fw_update_mode_passwd[1]);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &fw_status, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: read fail, read_boot_status\n", __func__);
		return 0;
	}
	if (fw_status != SEC_TS_STATUS_BOOT_MODE) {
		TS_LOG_ERR("%s: enter fail! read_boot_status = 0x%x\n", __func__, fw_status);
		return 0;
	}

	TS_LOG_INFO("%s: Success! read_boot_status = 0x%x\n", __func__, fw_status);

	sec_ts_delay(10);


	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_ID, id, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: read id fail\n", __func__);
		return 0;
	}

	ts->boot_ver[0] = id[0];
	ts->boot_ver[1] = id[1];
	ts->boot_ver[2] = id[2];

	ts->flash_page_size = SEC_TS_FW_BLK_SIZE_DEFAULT;
	if ((ts->boot_ver[1] == 0x37) && (ts->boot_ver[2] == 0x61))
		ts->flash_page_size = 512;

	TS_LOG_INFO("%s: read_boot_id = %02X%02X%02X\n", __func__, id[0], id[1], id[2]);

	return 1;
}


static void sec_ts_save_version_of_bin(struct sec_ts_data *ts, const fw_header *fw_hd)
{
	ts->plat_data->img_version_of_bin[3] = ((fw_hd->img_ver >> 24) & 0xff);
	ts->plat_data->img_version_of_bin[2] = ((fw_hd->img_ver >> 16) & 0xff);
	ts->plat_data->img_version_of_bin[1] = ((fw_hd->img_ver >> 8) & 0xff);
	ts->plat_data->img_version_of_bin[0] = ((fw_hd->img_ver >> 0) & 0xff);

	ts->plat_data->core_version_of_bin[3] = ((fw_hd->fw_ver >> 24) & 0xff);
	ts->plat_data->core_version_of_bin[2] = ((fw_hd->fw_ver >> 16) & 0xff);
	ts->plat_data->core_version_of_bin[1] = ((fw_hd->fw_ver >> 8) & 0xff);
	ts->plat_data->core_version_of_bin[0] = ((fw_hd->fw_ver >> 0) & 0xff);

	ts->plat_data->config_version_of_bin[3] = ((fw_hd->para_ver >> 24) & 0xff);
	ts->plat_data->config_version_of_bin[2] = ((fw_hd->para_ver >> 16) & 0xff);
	ts->plat_data->config_version_of_bin[1] = ((fw_hd->para_ver >> 8) & 0xff);
	ts->plat_data->config_version_of_bin[0] = ((fw_hd->para_ver >> 0) & 0xff);

	TS_LOG_INFO("%s: img_ver of bin = %x.%x.%x.%x\n", __func__,
			ts->plat_data->img_version_of_bin[0],
			ts->plat_data->img_version_of_bin[1],
			ts->plat_data->img_version_of_bin[2],
			ts->plat_data->img_version_of_bin[3]);

	TS_LOG_INFO("%s: core_ver of bin = %x.%x.%x.%x\n", __func__,
			ts->plat_data->core_version_of_bin[0],
			ts->plat_data->core_version_of_bin[1],
			ts->plat_data->core_version_of_bin[2],
			ts->plat_data->core_version_of_bin[3]);

	TS_LOG_INFO("%s: config_ver of bin = %x.%x.%x.%x\n", __func__,
			ts->plat_data->config_version_of_bin[0],
			ts->plat_data->config_version_of_bin[1],
			ts->plat_data->config_version_of_bin[2],
			ts->plat_data->config_version_of_bin[3]);
}

static int sec_ts_save_version_of_ic(struct sec_ts_data *ts)
{
	struct sec_ts_plat_data *pdata = ts->plat_data;
	u8 img_ver[4] = {0,};
	u8 core_ver[4] = {0,};
	u8 config_ver[4] = {0,};
	int ret = 0;

	/* Image ver */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_IMG_VERSION, img_ver, 4);
	if (ret < 0) {
		TS_LOG_ERR("%s: Image version read error\n", __func__);
		return -EIO;
	}
	TS_LOG_INFO("%s: IC Image version info : %x.%x.%x.%x\n", __func__,
			img_ver[0], img_ver[1], img_ver[2], img_ver[3]);

	pdata->img_version_of_ic[0] = img_ver[0];
	pdata->img_version_of_ic[1] = img_ver[1];
	pdata->img_version_of_ic[2] = img_ver[2];
	pdata->img_version_of_ic[3] = img_ver[3];

	if(S6SY761X == ts->ic_name) {
		snprintf(ts->chip_data->version_name, MAX_STR_LEN - 1, "%02X.%02X.%02X.%02x",
				img_ver[0], img_ver[1], img_ver[2], img_ver[3]);
	}

	/* Core ver */
	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_FW_VERSION, core_ver, 4);
	if (ret < 0) {
		TS_LOG_ERR("%s: core version read error\n", __func__);
		return -EIO;
	}
	TS_LOG_INFO("%s: IC Core version info : %x.%x.%x.%x,\n", __func__,
			core_ver[0], core_ver[1], core_ver[2], core_ver[3]);

	pdata->core_version_of_ic[0] = core_ver[0];
	pdata->core_version_of_ic[1] = core_ver[1];
	pdata->core_version_of_ic[2] = core_ver[2];
	pdata->core_version_of_ic[3] = core_ver[3];

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_PARA_VERSION, config_ver, 4);
	if (ret < 0) {
		TS_LOG_ERR("%s: config version read error\n", __func__);
		return -EIO;
	}
	TS_LOG_INFO("%s: IC config version info : %x.%x.%x.%x\n", __func__,
		config_ver[0], config_ver[1], config_ver[2], config_ver[3]);

	pdata->config_version_of_ic[0] = config_ver[0];
	pdata->config_version_of_ic[1] = config_ver[1];
	pdata->config_version_of_ic[2] = config_ver[2];
	pdata->config_version_of_ic[3] = config_ver[3];

	return 0;
}

static int sec_ts_check_firmware_version(struct sec_ts_data *ts, const u8 *fw_info)
{
	struct sec_ts_plat_data *pdata = ts->plat_data;
	fw_header *fw_hd = NULL;
	u8 buff[1] = { 0 };
	int i = 0;
	int ret = NO_ERR;

	fw_hd = (fw_header *)fw_info;

	sec_ts_save_version_of_bin(ts, fw_hd);

	ret = ts->sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, buff, 1);
	if (ret < 0) {
		TS_LOG_ERR("%s: fail to read BootStatus\n", __func__);
		return -EIO;
	}

	if (buff[0] == SEC_TS_STATUS_BOOT_MODE) {
		TS_LOG_ERR("%s: ReadBootStatus = 0x%x, F/W download Start!\n",
					__func__, buff[0]);
		return DO_FWUP;
	}

	ret = sec_ts_save_version_of_ic(ts);
	if (ret < 0) {
		TS_LOG_ERR("%s: fail to read ic version\n", __func__);
		return -EIO;
	}

	for (i = 0; i < 2; i++) {
		if (pdata->img_version_of_ic[i] != pdata->img_version_of_bin[i]) {
			TS_LOG_ERR("%s: don't matched version info\n", __func__);
			return NO_FWUP;
		}
	}

	if ((pdata->img_version_of_ic[2] >= 1) && (pdata->img_version_of_ic[3] > 0)) {
		ts->event_length = SEC_TS_EVENT_BUFF_LONGSIZE;
		if (!ts->event_buff)
			kfree(ts->event_buff);
		ts->event_buff = kzalloc(MAX_EVENT_COUNT * ts->event_length, GFP_KERNEL);
		if (!ts->event_buff) {
			TS_LOG_ERR("%s: event_buffer memory alloc error\n", __func__);
			return -EIO;
		}
	}

	for (i = 2; i < 4; i++) {
		if (pdata->img_version_of_ic[i] != pdata->img_version_of_bin[i]) {
			TS_LOG_INFO("%s: i:%d, img ver:%d, bin ver:%d", __func__, i,
				pdata->img_version_of_ic[i],
				pdata->img_version_of_bin[i]);
			return DO_FWUP;
		}
	}
	return 0;
}

static u8 sec_ts_checksum(u8 *data, int offset, int size)
{
	int i = 0;
	u8 checksum = 0;

	for (i = 0; i < size; i++)
		checksum += data[i + offset];

	return checksum;
}

static int sec_ts_flashpageerase(struct sec_ts_data *ts, u32 page_idx, u32 page_num)
{
	int ret = NO_ERR;
	u8 tCmd[6] = { 0 };

	tCmd[0] = SEC_TS_CMD_FLASH_ERASE;
	tCmd[1] = (u8)((page_idx >> 8) & 0xFF);
	tCmd[2] = (u8)((page_idx >> 0) & 0xFF);
	tCmd[3] = (u8)((page_num >> 8) & 0xFF);
	tCmd[4] = (u8)((page_num >> 0) & 0xFF);
	tCmd[5] = sec_ts_checksum(tCmd, 1, 4);

	ret = ts->sec_ts_i2c_write_burst(ts, tCmd, 6);

	return ret;
}

static int sec_ts_flashpagewrite(struct sec_ts_data *ts, u32 page_idx, u8 *page_data)
{
	int ret = NO_ERR;
	u8 tCmd[1 + 2 + SEC_TS_FW_BLK_SIZE_MAX + 1] = { 0 };
	int flash_page_size = (int)ts->flash_page_size;

	tCmd[0] = 0xD9;
	tCmd[1] = (u8)((page_idx >> 8) & 0xFF);
	tCmd[2] = (u8)((page_idx >> 0) & 0xFF);

	memcpy(&tCmd[3], page_data, flash_page_size);
	tCmd[1 + 2 + flash_page_size] = sec_ts_checksum(tCmd, 1, 2 + flash_page_size);

	ret = ts->sec_ts_i2c_write_burst(ts, tCmd, 1 + 2 + flash_page_size + 1);
	return ret;
}

static bool sec_ts_limited_flashpagewrite(struct sec_ts_data *ts, u32 page_idx, u8 *page_data)
{
	int ret = 0;
	u8 *tCmd = NULL;
	u8 copy_data[3 + SEC_TS_FW_BLK_SIZE_MAX];
	int copy_left = (int)ts->flash_page_size + 3;
	int copy_size = 0;
	int copy_max = ts->i2c_burstmax - 1;
	int flash_page_size = (int)ts->flash_page_size;

	copy_data[0] = (u8)((page_idx >> 8) & 0xFF);
	copy_data[1] = (u8)((page_idx >> 0) & 0xFF);

	memcpy(&copy_data[2], page_data, flash_page_size);
	copy_data[2 + flash_page_size] = sec_ts_checksum(copy_data, 0, 2 + flash_page_size);

	while (copy_left > 0) {
		int copy_cur = (copy_left > copy_max) ? copy_max : copy_left;

		tCmd = kzalloc(copy_cur + 1, GFP_KERNEL);
		if (!tCmd)
			goto err_write;

		if (copy_size == 0)
			tCmd[0] = SEC_TS_CMD_FLASH_WRITE;
		else
			tCmd[0] = SEC_TS_CMD_FLASH_PADDING;

		memcpy(&tCmd[1], &copy_data[copy_size], copy_cur);

		ret = ts->sec_ts_i2c_write_burst(ts, tCmd, 1 + copy_cur);
		if (ret < 0)
			TS_LOG_ERR("%s: failed, ret:%d\n", __func__, ret);

		copy_size += copy_cur;
		copy_left -= copy_cur;
		kfree(tCmd);
		sec_ts_delay(2);
	}
	return ret;

err_write:
	TS_LOG_ERR("%s: failed to alloc.\n", __func__);
	return -ENOMEM;

}

static int sec_ts_flashwrite(struct sec_ts_data *ts, u32 mem_addr, u8 *mem_data, u32 mem_size, int retry)
{
	int ret = NO_ERR;
	u32 page_idx = 0;
	u32 size_copy = 0;
	u32 flash_page_size = 0;
	u32 page_idx_start = 0;
	u32 page_idx_end = 0;
	u32 page_num = 0;
	u8 page_buf[SEC_TS_FW_BLK_SIZE_MAX] = { 0 };

	if (mem_size == 0)
		return 0;

	flash_page_size = ts->flash_page_size;
	page_idx_start = mem_addr / flash_page_size;
	page_idx_end = (mem_addr + mem_size - 1) / flash_page_size;
	page_num = page_idx_end - page_idx_start + 1;

	TS_LOG_DEBUG("%s: flash page erase, page start=%08X, page_num = %d\n",
				__func__, page_idx_start, page_num);

	ret = sec_ts_flashpageerase(ts, page_idx_start, page_num);
	if (ret < 0) {
		TS_LOG_ERR(
					"%s: fw erase failed, mem_addr= %08X, pagenum = %d\n",
					__func__, mem_addr, page_num);
		return -EIO;
	}

	sec_ts_delay(page_num + 10);

	size_copy = mem_size % flash_page_size;
	if (size_copy == 0)
		size_copy = flash_page_size;

	memset(page_buf, 0, flash_page_size);

	for (page_idx = page_num - 1;; page_idx--) {
		memcpy(page_buf, mem_data + (page_idx * flash_page_size), size_copy);
		if (ts->boot_ver[0] == SEC_TS_BOOT_OLDVER) {
			ret = sec_ts_flashpagewrite(ts, (page_idx + page_idx_start), page_buf);
			if (ret < 0) {
				TS_LOG_ERR("%s: fw write failed, page_idx = %u\n", __func__, page_idx);
				goto err;
			}
		} else {
			TS_LOG_DEBUG("%s: fw write, page_idx = %u\n", __func__, page_idx);

			ret = sec_ts_limited_flashpagewrite(ts, (page_idx + page_idx_start), page_buf);
			if (ret < 0) {
				TS_LOG_ERR("%s: fw write failed, page_idx = %u\n", __func__, page_idx);
				goto err;
			}
		}

		size_copy = flash_page_size;
		sec_ts_delay(5);

		if (page_idx == 0)
			break;
	}

	return mem_size;
err:
	return -EIO;
}

static int sec_ts_memoryblockread(struct sec_ts_data *ts, u32 mem_addr, int mem_size, u8 *buf)
{
	int ret = NO_ERR;
	u8 cmd[5] = { 0 };
	u8 *data = NULL;

	if (mem_size >= 64 * 1024) {
		TS_LOG_ERR("%s: mem size over 64K\n", __func__);
		return -EIO;
	}

	cmd[0] = (u8)SEC_TS_CMD_FLASH_READ_ADDR;
	cmd[1] = (u8)((mem_addr >> 24) & 0xff);
	cmd[2] = (u8)((mem_addr >> 16) & 0xff);
	cmd[3] = (u8)((mem_addr >> 8) & 0xff);
	cmd[4] = (u8)((mem_addr >> 0) & 0xff);

	ret = ts->sec_ts_i2c_write_burst(ts, cmd, 5);
	if (ret < 0) {
		TS_LOG_ERR("%s: send command failed, %02X\n", __func__, cmd[0]);
		return -EIO;
	}

	udelay(10);
	cmd[0] = (u8)SEC_TS_CMD_FLASH_READ_SIZE;
	cmd[1] = (u8)((mem_size >> 8) & 0xff);
	cmd[2] = (u8)((mem_size >> 0) & 0xff);

	ret = ts->sec_ts_i2c_write_burst(ts, cmd, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: send command failed, %02X\n", __func__, cmd[0]);
		return -EIO;
	}

	udelay(10);
	cmd[0] = (u8)SEC_TS_CMD_FLASH_READ_DATA;

	data = buf;


	ret = ts->sec_ts_i2c_read(ts, cmd[0], data, mem_size);
	if (ret < 0) {
		TS_LOG_ERR("%s: memory read failed\n", __func__);
		return -EIO;
	}

	return 0;
}

static int sec_ts_memoryread(struct sec_ts_data *ts, u32 mem_addr, u8 *mem_data, u32 mem_size)
{
	int ret = NO_ERR;
	int retry = 3;
	int read_size = 0;
	int unit_size = 0;
	int max_size = ts->i2c_burstmax;
	int read_left = (int)mem_size;
	u8 *tmp_data = NULL;

	tmp_data = kmalloc(max_size, GFP_KERNEL);
	if (!tmp_data) {
		TS_LOG_ERR(
			"%s: failed to kmalloc\n", __func__);
		return -ENOMEM;
	}

	while (read_left > 0) {
		unit_size = (read_left > max_size) ? max_size : read_left;
		retry = 3;
		do {
			TS_LOG_DEBUG("%s: fw read mem_addr=%08X,unit_size=%d\n",
					__func__, mem_addr, unit_size);

			ret = sec_ts_memoryblockread(ts, mem_addr, unit_size, tmp_data);
			if (retry-- == 0) {
				TS_LOG_ERR("%s: fw read fail mem_addr=%08X,unit_size=%d\n",
						__func__, mem_addr, unit_size);
				kfree(tmp_data);
				return -ENOMEM;
			}

			memcpy(mem_data + read_size, tmp_data, unit_size);
		} while (ret < 0);

		mem_addr += unit_size;
		read_size += unit_size;
		read_left -= unit_size;
	}

	kfree(tmp_data);

	return read_size;
}

int set_pid_data(struct sec_ts_data *ts, u8 *data)
{
	char buff[SEC_TS_PROJECTID_MAX+2] = { 0 };
	int ret = 0;

	if (ts->power_status == SEC_TS_STATE_POWER_OFF) {
		TS_LOG_ERR("%s: TSP_truned off", __func__);
		return -1;
	}

	buff[0] = 0;
	buff[1] = SEC_TS_PROJECTID_MAX-1;
	memcpy(&buff[2], data, sizeof(u8) * SEC_TS_PROJECTID_MAX);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, buff, 2+SEC_TS_PROJECTID_MAX);
	if (ret < 0)
		TS_LOG_ERR("%s: nvm write failed. ret: %d\n", __func__, ret);

	sec_ts_delay(40);

	return ret;
}

int get_pid_data(struct sec_ts_data *ts, u8 *data)
{
	int ret = 0;

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_OFF, NULL, 0);
	if (ret < 0) {
		TS_LOG_ERR("%s: fail to write Sense_off\n", __func__);
		goto out_nvm;
	}

	TS_LOG_INFO("%s: SENSE OFF\n", __func__);

	sec_ts_delay(100);

	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_CLEAR_EVENT_STACK, NULL, 0);
	if (ret < 0) {
		TS_LOG_ERR("%s: i2c write clear event failed\n", __func__);
		goto out_nvm;
	}

	TS_LOG_INFO("%s: CLEAR EVENT STACK\n", __func__);

	sec_ts_delay(100);

	memset(data, 0x00, 10);
	data[0] = 0;
	data[1] = 9;
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_NVM, data, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

	sec_ts_delay(20);

	ret = ts->sec_ts_i2c_read_bulk(ts, data, 10);
	if (ret < 0) {
		TS_LOG_ERR("%s nvm send command failed. ret: %d\n", __func__, ret);
		goto out_nvm;
	}

out_nvm:
	ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
	if (ret < 0)
		TS_LOG_ERR(" fail to write Sense_on\n");

	TS_LOG_INFO("%s: SENSE ON\n", __func__);

	return ret;
}


int sec_ts_read_pid(struct sec_ts_data *ts)
{
	int ret = NO_ERR;
	u8 data[SEC_TS_PROJECTID_MAX] = {0};
	u8 retry = 0;
	u32 addr = SEC_TS_PID_ADDR;

	if(ts->ic_name == S6SY761X){
		addr = SEC_TS_Y761_PID_ADDR;
	}

	ret = sec_ts_memoryread(ts, addr, data, SEC_TS_PROJECTID_MAX);
	if (ret < 0) {
		TS_LOG_ERR("%s: read pid fail\n", __func__);
		return -EIO;
	}
	TS_LOG_INFO("%s: data = %02X %02X %02X %02X\n", __func__,
				data[0], data[1], data[2], data[3]);

	if(ts->is_need_set_reseved_bit){
		data[SEC_TS_PROJECTID_MAX-2] = 0x30;//set the reserved bit 9(11-2) to 0x30.
	}

	data[SEC_TS_PROJECTID_MAX-1] = 0;
	/* 0xff, 0x00 -> read project id error : */
	if ((data[0] == 0xff) || (data[0] == 0x00)) {
		TS_LOG_INFO("%s: pid read: data[0] = %d \n", __func__, data[0]);
		snprintf(ts->project_id, sizeof(ts->project_id), "%s", ts->default_projectid);

		for (retry = 0; retry < SEC_TS_I2C_RETRY_CNT; retry++) {
			set_pid_data(ts, ts->project_id);
			get_pid_data(ts, data);
			data[SEC_TS_PROJECTID_MAX-1] = '\0';
			if (memcmp(ts->project_id, data, SEC_TS_PROJECTID_MAX) == 0) {
				TS_LOG_ERR("%s: pid write and read: data[0] = %s \n", __func__, data);
				break;
			} else
				TS_LOG_ERR("%s: pid write error: data[0] = %d \n", __func__, data[0]);
		}
	} else {
		snprintf(ts->project_id, SEC_TS_PROJECTID_MAX, "%s", data);
	}
	TS_LOG_INFO("%s: %s\n", __func__, ts->project_id);
	return ret;
}

static int sec_ts_chunk_update(struct sec_ts_data *ts, u32 addr, u32 size, u8 *data, int retry)
{
	u32 fw_size = 0;
	u32 write_size = 0;
	u8 *mem_rb = NULL;
	int ret = 0;

	fw_size = size;

	write_size = sec_ts_flashwrite(ts, addr, data, fw_size, retry);
	if (write_size != fw_size) {
		TS_LOG_ERR("%s: fw write failed\n", __func__);
		ret = -1;
		goto err_write_fail;
	}

	mem_rb = vzalloc(fw_size);
	if (!mem_rb) {
		TS_LOG_ERR("%s: vzalloc failed\n", __func__);
		ret = -1;
		goto err_write_fail;
	}

	if (sec_ts_memoryread(ts, addr, mem_rb, fw_size) >= 0) {
		u32 ii;

		for (ii = 0; ii < fw_size; ii++) {
			if (data[ii] != mem_rb[ii])
				break;
		}

		if (fw_size != ii) {
			TS_LOG_ERR("%s: fw verify fail\n", __func__);
			ret = -1;
			goto out;
		}
	} else {
		ret = -1;
		goto out;
	}

	TS_LOG_INFO("%s: verify done(%d)\n", __func__, ret);

out:
	vfree(mem_rb);
err_write_fail:
	sec_ts_delay(10);

	return ret;
}

int sec_ts_execute_force_calibration(struct sec_ts_data *ts, int cal_mode)
{
	int rc = -1;
	u8 cmd = 0;

	TS_LOG_INFO("%s: %d\n", __func__, cal_mode);

	if (cal_mode == OFFSET_CAL_SEC)
		cmd = SEC_TS_CMD_FACTORY_PANELCALIBRATION;
	else if (cal_mode == AMBIENT_CAL)
		cmd = SEC_TS_CMD_CALIBRATION_AMBIENT;
	else
		return rc;

	if (ts->sec_ts_i2c_write(ts, cmd, NULL, 0) < 0) {
		TS_LOG_ERR("%s: Write Cal commend failed!\n", __func__);
		return rc;
	}

	sec_ts_delay(1000);

	rc = sec_ts_wait_for_ready(ts, SEC_TS_VENDOR_ACK_OFFSET_CAL_DONE);

	return rc;
}

static int sec_ts_firmware_valid(const u8 *data, u32 size)
{
	int i = 0;
	u32 check_data = 0;
	u32 size_4 = ((size + 3) & ~3);
	u32 *p_temp = NULL;
	u8 *temp_data = NULL;

	temp_data = kzalloc(size_4, GFP_KERNEL);
	if (!temp_data) {
		TS_LOG_ERR("%s: failed to kmalloc\n", __func__);
		return -ENOMEM;
	}
	memcpy(temp_data, data, size);
	p_temp = (u32 *)temp_data;
	for (i = 0; i < (size_4 / 4); i++) {
		check_data += p_temp[i];
	}
	kfree(temp_data);
	if (check_data != 0) {
		TS_LOG_ERR("%s: Invalid Image file: %d\n", __func__, check_data);
		return -ENODATA;
	}
	return NO_ERR;
}

static int sec_ts_firmware_update(struct sec_ts_data *ts, const u8 *data, size_t size, int bl_update, int restore_cal, int retry)
{
	int i = 0;
	int ret = NO_ERR;
	fw_header *fw_hd = NULL;
	fw_chunk *fw_ch = NULL;
	u8 fw_status = 0;
	u8 *fd = (u8 *)data;
	u8 tBuff[3] = {0};

	fw_hd = (fw_header *)fd;
	fd += sizeof(fw_header);

	if (fw_hd->signature != SEC_TS_FW_HEADER_SIGN) {
		TS_LOG_ERR("%s: firmware header error = %08X\n", __func__, fw_hd->signature);
		return -ENODATA;
	}

	ret = sec_ts_firmware_valid(data, fw_hd->totalsize);
	if (ret < 0) {
		return -ENODATA;
	}

	if (!sec_ts_enter_fw_mode(ts)) {
		TS_LOG_ERR("%s: firmware mode failed\n", __func__);
		return -ENODATA;
	}

	if (bl_update && (ts->boot_ver[0] == SEC_TS_BOOT_VER)) {
		TS_LOG_INFO("%s: bootloader is up to date\n", __func__);
		return NO_ERR;
	}

	TS_LOG_INFO("%s: firmware update retry :%d\n", __func__, retry);


	TS_LOG_ERR("%s: num_chunk : %d\n", __func__, fw_hd->num_chunk);
	for (i = 0; i < fw_hd->num_chunk; i++) {
		fw_ch = (fw_chunk *)fd;

		TS_LOG_INFO("%s: [%d] 0x%08X, 0x%08X, 0x%08X, 0x%08X\n", __func__, i,
				fw_ch->signature, fw_ch->addr, fw_ch->size, fw_ch->reserved);

		if (fw_ch->signature != SEC_TS_FW_CHUNK_SIGN) {
			TS_LOG_ERR("%s: firmware chunk error = %08X\n", __func__, fw_ch->signature);
			return -ENODATA;
		}
		TS_LOG_ERR("%s: firmware chunk count %d, addr=%08X, size = %d\n", __func__,
					i, fw_ch->addr, fw_ch->size);
		fd += sizeof(fw_chunk);
		ret = sec_ts_chunk_update(ts, fw_ch->addr, fw_ch->size, fd, retry);
		if (ret < 0) {
			TS_LOG_ERR("%s: firmware chunk write failed, addr=%08X, size = %d\n", __func__, fw_ch->addr, fw_ch->size);
			return -ENODATA;
		}
		fd += fw_ch->size;
	}

	sec_ts_sw_reset(ts);


	if (!bl_update) {
		ret = ts->sec_ts_i2c_write(ts, SEC_TS_CMD_SENSE_ON, NULL, 0);
		if (ret < 0) {
			TS_LOG_ERR("%s: write fail, Sense_on\n", __func__);
			return -ENODATA;
		}

		if (ts->sec_ts_i2c_read(ts, SEC_TS_READ_BOOT_STATUS, &fw_status, 1) < 0) {
			TS_LOG_ERR("%s: read fail, read_boot_status = 0x%x\n", __func__, fw_status);
			return -ENODATA;
		}

		if (fw_status != SEC_TS_STATUS_APP_MODE) {
			TS_LOG_ERR("%s: fw update sequence done, BUT read_boot_status = 0x%x\n", __func__, fw_status);
			return -ENODATA;
		}

		TS_LOG_INFO("%s: fw update Success! read_boot_status = 0x%x\n", __func__, fw_status);

		return 1;
	} else {

		if (ts->sec_ts_i2c_read(ts, SEC_TS_READ_ID, tBuff, 3) < 0) {
			TS_LOG_ERR("%s: read device id fail after bl fw download\n", __func__);
			return -ENODATA;
		}

		if (tBuff[0] == SEC_TS_APP_VER) {
			TS_LOG_INFO("%s: bl fw download success - device id = %02X\n", __func__, tBuff[0]);
			return -ENODATA;
		} else {
			TS_LOG_ERR("%s: bl fw id does not match - device id = %02X\n", __func__, tBuff[0]);
			return -ENODATA;
		}
	}

}

int sec_ts_firmware_update_on_probe(struct sec_ts_data *ts, bool force_update)
{
	const struct firmware *fw_entry = NULL;
	const char *fw_path = ts->plat_data->firmware_name;
	int result = 0;
	int restore_cal = 0;
	int ii = 0;
	int ret = 0;

	disable_irq(ts->client->irq);
	ts->cal_status = sec_ts_read_calibration_report(ts);

	TS_LOG_INFO("%s: initial firmware update %s, cal:%X\n",
					__func__, fw_path, ts->cal_status);

	/* If firmware not found, should return NO_ERR */
	if (request_firmware(&fw_entry, fw_path, &ts->sec_ts_pdev->dev) !=  0) {
		TS_LOG_ERR("%s: firmware is not available\n", __func__);
		goto err_request_fw;
	}
	TS_LOG_INFO("%s: request firmware done! size = %d\n", __func__, (int)fw_entry->size);

	result = sec_ts_check_firmware_version(ts, fw_entry->data);

	if ((result <= NO_FWUP) && (!force_update)) {
		TS_LOG_ERR("%s: skip fw update\n", __func__);
		goto err_request_fw;
	}
	if(ts->ic_name == S6SY761X) {
		wake_lock(&ts->wakelock);//add wakelock,avoid i2c suspend
	}
	for (ii = 0; ii < 3; ii++) {
		ret = sec_ts_firmware_update(ts, fw_entry->data, fw_entry->size, 0, restore_cal, ii);
		if (ret >= 0)
			break;
	}

	if (ret < 0) {
		result = -EIO;;
	} else {
		result = NO_ERR;
	}

	sec_ts_save_version_of_ic(ts);

	if(ts->is_need_calibrate_after_update_fw && result == NO_ERR) {
		TS_LOG_INFO("%s, do calibrate after update firmware\n", __func__);
		sec_ts_do_once_calibrate();
	}
	if(ts->ic_name == S6SY761X) {
		wake_unlock(&ts->wakelock);//add wakelock,avoid i2c suspend
	}

err_request_fw:
	release_firmware(fw_entry);
	enable_irq(ts->client->irq);
	return result;
}

int sec_ts_load_fw_request(struct sec_ts_data *ts, u8 sd_enable)
{
	const struct firmware *fw_entry = NULL;
	char fw_path[SEC_TS_MAX_FW_PATH] = { 0 };
	int error = 0;

	if (ts->client->irq)
		disable_irq(ts->client->irq);

	if (sd_enable)
		snprintf(fw_path, SEC_TS_MAX_FW_PATH, "%s", SEC_TS_DEFAULT_HW_FW);
	else
		snprintf(fw_path, SEC_TS_MAX_FW_PATH, "%s", ts->plat_data->firmware_name);

	TS_LOG_INFO("%s: initial firmware update  %s\n", __func__, fw_path);

	if (request_firmware(&fw_entry, fw_path, &ts->sec_ts_pdev->dev) !=  0) {
		TS_LOG_ERR("%s: firmware is not available\n", __func__);
		error = -ENODATA;
		goto err_request_fw;
	}
	TS_LOG_INFO("%s: request firmware done! size = %d\n", __func__, (int)fw_entry->size);

	if (sec_ts_firmware_update(ts, fw_entry->data, fw_entry->size, 0, 1, 0) < 0)
		error = -ENODATA;
	else
		error = NO_ERR;

	sec_ts_save_version_of_ic(ts);

err_request_fw:
	release_firmware(fw_entry);
	if (ts->client->irq)
		enable_irq(ts->client->irq);

	return error;
}
