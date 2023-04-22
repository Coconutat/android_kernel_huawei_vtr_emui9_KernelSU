/*
 * fts.c
 *
 * FTS Capacitive touch screen controller (FingerTipS)
 *
 * Copyright (C) 2012, 2013 STMicroelectronics Limited.
 * Authors: AMS(Analog Mems Sensor)
 *        : Victor Phay <victor.phay@st.com>
 *        : Li Wu <li.wu@st.com>
 *        : Giuseppe Di Giore <giuseppe.di-giore@st.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/device.h>

#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/input.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/hrtimer.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/completion.h>
#include <linux/wakelock.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/regulator/consumer.h>
#ifdef CONFIG_HAS_EARLYSUSPEND //hank modified
#include <linux/earlysuspend.h>
#endif
#include "fts.h"
#include <linux/notifier.h>
#include <linux/fb.h>
#ifdef KERNEL_ABOVE_2_6_38
#include <linux/input/mt.h>
#endif

static char *fts_fw_filename[] = {
	"../../../sdcard/update/lon_st_firmware.bin",
};

static char fts_firmware_name[128] = {0};

static unsigned int le_to_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[0] + (unsigned int)ptr[1] * 0x100;
}
static unsigned int be_to_uint(const unsigned char *ptr)
{
	return (unsigned int)ptr[1] + (unsigned int)ptr[0] * 0x100;
}

int cx_crc_check(struct fts_ts_info *info)
{
	unsigned char regAdd1[3] = {0xB6, 0x00, 0x86};
	unsigned char val[4] = {0};
	unsigned char crc_status;
	unsigned int error;
	
	error = st_i2c_read(info, regAdd1, sizeof(regAdd1), val, sizeof(val));
	if (error) {
		TS_LOG_ERR("Cannot read crc status\n");
		return -1;
	}

	crc_status = val[1] & 0x02;
	if(crc_status != 0) // CRC error
	{
		TS_LOG_INFO("%s:fts CRC status = %d \n",__func__,crc_status);
		return -2;
	}

	return crc_status;
}	

static int st_init_flash_reload(struct fts_ts_info *info)
{
	unsigned char data[FTS_EVENT_SIZE] = {0};
	int retry, error = 0;
	unsigned char event_id = 0;
	unsigned char tune_flag = 0;
	int init_error = -1;
	unsigned char regAdd =0;

	st_interrupt_set(info, INT_DISABLE);
	st_command(info, FLUSHBUFFER);
	st_command(info, INIT_CMD);

	for (retry = 0; retry <= READ_CNT_INIT; retry++){  // poll with timeout
		regAdd = READ_ONE_EVENT;
		error = st_i2c_read(info, &regAdd,sizeof(regAdd), data, FTS_EVENT_SIZE);
		if (error) {
			TS_LOG_ERR("Cannot read device info\n");
			return -2;
		}
		TS_LOG_DEBUG("FTS fts status event(init check): %02X %02X %02X %02X %02X %02X %02X %02X\n",
				data[0], data[1], data[2], data[3],
				data[4], data[5], data[6], data[7]);

		event_id = data[0];
		tune_flag = data[1];

		if((event_id == 0x16) && (tune_flag == 0x07)){
			if(data[2] == 0x00){
				init_error = 0;
				TS_LOG_INFO("fts initialization passed \n");
			} else {
				init_error = -1;
				TS_LOG_INFO("fts initialization failed \n");
			}
			break;
		} else if(retry == READ_CNT_INIT) {
			init_error = -2;
		} else {
			msleep(50);
		}
	}

	error += st_command(info, SENSEON);
	mdelay(5);
#ifdef PHONE_KEY
	error += st_command(info, KEYON);
#endif
	error += st_command(info, FORCECALIBRATION);
	mdelay(5);
	error += st_command(info, FLUSHBUFFER);
	mdelay(5);
	st_interrupt_set(info, INT_ENABLE);

	if (error != 0){
		TS_LOG_ERR("Init (2) error (#errors = %d)\n", error);
		return -ENODEV ;
	}else{
		return init_error;
	}
}

int st_chip_initialization(struct fts_ts_info *info)
{
	int ret = 0;
	int retry;
	int error;
	int initretrycnt = 0;

	TS_LOG_INFO("chip calibrate and initialization\n");

	st_systemreset(info);
	st_wait_controller_ready(info);
	ret = st_init_flash_reload(info);
	if (!ret)
		return ret;

	//initialization error, retry initialization
	for(retry = 0; retry <= INIT_FLAG_CNT; retry++){
		st_chip_powercycle(info);
		st_systemreset(info);
		st_wait_controller_ready(info);	
		ret = st_init_flash_reload(info);
		if(ret == 0)
			break;
		initretrycnt++;
		TS_LOG_INFO("initialization cycle count = %04d\n", initretrycnt);
	}
	if(ret != 0){ //initialization error
		TS_LOG_INFO("fts initialization 3 times error\n");
		error = st_systemreset(info);
		st_wait_controller_ready(info);
		error += st_command(info, SENSEON);
		mdelay(5);
		error += st_command(info, FORCECALIBRATION);
		mdelay(5);
#ifdef PHONE_KEY
		st_command(info, KEYON);
#endif
		error += st_command(info, FLUSHBUFFER);
		mdelay(5);
		st_interrupt_set(info, INT_ENABLE);
		if (error){
			TS_LOG_INFO("%s: Cannot reset the device----------\n", __func__);
		}
	}

	return ret;
}

static int st_flash_status(struct fts_ts_info *info,
				unsigned int timeout, unsigned int steps)
{
	int ret, status;
	unsigned char data = 0;
	unsigned char regAdd[2];

	do {
		regAdd[0] = FLASH_READ_STATUS;
		regAdd[1] = 0;

		msleep(20);

		ret = st_i2c_read(info, regAdd, sizeof(regAdd), &data, sizeof(data));
		if (ret)
			status = FLASH_STATUS_UNKNOWN;
		else
			status = (data & 0x01) ? FLASH_STATUS_BUSY : FLASH_STATUS_READY;

		if (status == FLASH_STATUS_BUSY) {
			timeout -= steps;
			msleep(steps);
		}

	} while ((status == FLASH_STATUS_BUSY) && (timeout));

	return status;
}

static int st_flash_unlock(struct fts_ts_info *info)
{
	int ret;
	unsigned char regAdd[4] = { FLASH_UNLOCK,
				FLASH_UNLOCK_CODE_0,
				FLASH_UNLOCK_CODE_1,
				0x00 };

	ret = st_i2c_write(info, regAdd, sizeof(regAdd));

	if (ret)
		TS_LOG_ERR("Cannot unlock flash\n");
	else
		TS_LOG_INFO("Flash unlocked\n");

	return ret;
}

static int st_flash_load(struct fts_ts_info *info,
			int cmd, int address, const char *data, int size)
{
	int ret;
	unsigned char *cmd_buf;
	unsigned int loaded;

	cmd_buf = kmalloc(FLASH_LOAD_COMMAND_SIZE, GFP_KERNEL);
	if (cmd_buf == NULL) {
		TS_LOG_ERR("Out of memory when programming flash\n");
		return -ENOMEM;
	}

	loaded = 0;
	while (loaded < size) {
		cmd_buf[0] = cmd;
		cmd_buf[1] = (address >> 8) & 0xFF;
		cmd_buf[2] = (address) & 0xFF;

		memcpy(&cmd_buf[3], data, FLASH_LOAD_CHUNK_SIZE);
		ret = st_i2c_write(info, cmd_buf, FLASH_LOAD_COMMAND_SIZE);
		if (ret) {
			TS_LOG_ERR("Cannot load firmware in RAM\n");
			break;
		}

		data += FLASH_LOAD_CHUNK_SIZE;
		loaded += FLASH_LOAD_CHUNK_SIZE;
		address += FLASH_LOAD_CHUNK_SIZE;
	}

	kfree(cmd_buf);

	return (loaded == size) ? 0 : -1;
}


static int st_flash_erase(struct fts_ts_info *info, int cmd)
{
	int ret;
	unsigned char regAdd = cmd;

	ret = st_i2c_write(info, &regAdd, sizeof(regAdd));
	return ret;
}

static int st_flash_program(struct fts_ts_info *info, int cmd)
{
	int ret;
	unsigned char regAdd = cmd;

	ret = st_i2c_write(info, &regAdd, sizeof(regAdd));
	return ret;
}

int st_fw_upgrade(struct fts_ts_info *info, int mode,int fw_forceupdate,int crc_err, char *file_name)
{
	int ret;
	const struct firmware *fw = NULL;
	unsigned char *data = NULL;
	unsigned int size;
	int updata_loop = 0;
	int status, fw_ver = 0, config_ver = 0;
	int program_command, erase_command, load_command, load_address = 0;

	if (file_name) {
		strncat(file_name, "firmware", strlen("firmware"));
		snprintf(fts_firmware_name, sizeof(fts_firmware_name), "ts/st/%s.bin", file_name);
	}

	info->fwupdate_stat = 1;
	
	if(!info->fw_force){
		TS_LOG_INFO("Firmware upgrade from bootupdate... firmware name is %s\n", fts_firmware_name);
		ret = request_firmware(&fw, fts_firmware_name, info->dev);
	}else{
		TS_LOG_INFO("Firmware upgrade from sdupdate... firmware name is %s\n", fts_fw_filename[0]);
		ret = request_firmware(&fw, fts_fw_filename[0], info->dev);
		mode = MODE_RELEASE_AND_CONFIG_128;
	}
	if (ret) {
		if (!info->fw_force)
			TS_LOG_INFO("Unable to open firmware file[For boot update] '%s'\n",fts_firmware_name);
		else
			TS_LOG_INFO("Unable to open firmware file[For sd update] '%s'\n",fts_fw_filename[0]);
		return 0;
	}

	if ((fw->size == 0) /*|| (fw->size != fts_fw_size[mode])*/) {
		if (!info->fw_force)
			TS_LOG_ERR("Wrong firmware file[For boot update] '%s'\n", fts_firmware_name);
		else
			TS_LOG_ERR("Wrong firmware file[For sd update] '%s'\n", fts_fw_filename[0]);
		goto fw_done;
	}
	
	data = (unsigned char *)fw->data;
	size = fw->size;
	fw_ver = le_to_uint(&data[FILE_FW_VER_OFFSET]); 
	config_ver = le_to_uint(&data[FILE_CONFIG_VER_OFFSET]);

	if(!info->fw_force) {
		TS_LOG_INFO("%s: fw update probe begin!\n", __func__);
		ret = st_get_fw_version(info);
		if(ret) {
			TS_LOG_ERR("%s: can not get fw version!\n", __func__);
		}
		TS_LOG_INFO("%s: tp:fw_version = %x, config_id = %x. bin: fw_ver = %x, config_ver = %x\n", __func__, 
			info->fw_version, info->config_id, fw_ver, config_ver);

		if(fw_ver != info->fw_version || config_ver != info->config_id || fw_forceupdate == 1){
			mode = MODE_RELEASE_AND_CONFIG_128;
			TS_LOG_INFO("%s: different fw version mode = %d, need update fw\n",__func__, mode);
		}else{
			info->fwupdate_stat = 0;
			TS_LOG_INFO("%s: no need to update, release firmware file\n",__func__);
			ret = 0;
			goto fw_done;
		}
	}

fts_updat:
	TS_LOG_INFO("Flash programming...\n");
	ret = st_systemreset(info);
	if (ret) {
		TS_LOG_ERR("Cannot reset the device 00\n");
		goto fw_done;
	}
	st_wait_controller_ready(info);
	switch (mode) {
	case MODE_CONFIG_ONLY:
		program_command = FLASH_PROGRAM;
		erase_command = FLASH_ERASE;
		load_command = FLASH_LOAD_FIRMWARE_UPPER_64K;
		load_address = FLASH_LOAD_INFO_BLOCK_OFFSET;
		break;
	case MODE_RELEASE_AND_CONFIG_128:
		/* skip 32 bytes header */
		data += 32;
		size = size - 32;
		/* fall throug */
	case MODE_RELEASE_ONLY:
		program_command = FLASH_PROGRAM;
		erase_command = FLASH_ERASE;
		load_command = FLASH_LOAD_FIRMWARE_LOWER_64K;
		load_address = FLASH_LOAD_FIRMWARE_OFFSET;
		break;
	default:
		/* should never be here, already checked mode value before */
		break;
	}

	TS_LOG_INFO("1) checking for status.\n");
	status = st_flash_status(info, 1000, 100);
	if ((status == FLASH_STATUS_UNKNOWN) || (status == FLASH_STATUS_BUSY)) {
		TS_LOG_ERR("Wrong flash status 1\n");
		goto fw_done;
	}

	TS_LOG_INFO("2) unlock the flash.\n");
	ret = st_flash_unlock(info);
	if (ret) {
		TS_LOG_ERR("Cannot unlock the flash device\n");
		goto fw_done;
	}

	/* wait for a while */
	status = st_flash_status(info, 3000, 100);
	if ((status == FLASH_STATUS_UNKNOWN) || (status == FLASH_STATUS_BUSY)) {
		TS_LOG_ERR("Wrong flash status 2\n");
		goto fw_done;
	}

	TS_LOG_INFO("3) load the program.\n");
	if(load_command == FLASH_LOAD_FIRMWARE_LOWER_64K){
		ret = st_flash_load(info, load_command, load_address, data, FLASH_SIZE_F0_CMD);
		load_command = FLASH_LOAD_FIRMWARE_UPPER_64K;
		if((crc_err == 0)&&(size == (FLASH_SIZE_FW_CONFIG + FLASH_SIZE_CXMEM))){ //only for D2 chip
			//if size is 128 K, then adjust the size to include only fw and config(124 K)
			size = size - FLASH_SIZE_CXMEM;
		}
		
		ret = st_flash_load(info, load_command, load_address, (data+FLASH_SIZE_F0_CMD), (size-FLASH_SIZE_F0_CMD));
	}else{
		ret = st_flash_load(info, load_command, load_address, data, size);
	}
	if (ret) {
		TS_LOG_ERR("Cannot load program to for the flash device\n");
		goto fw_done;
	}

	/* wait for a while */
	status = st_flash_status(info, 3000, 100);
	if ((status == FLASH_STATUS_UNKNOWN) || (status == FLASH_STATUS_BUSY)) {
		TS_LOG_ERR("Wrong flash status 3\n");
		goto fw_done;
	}

	TS_LOG_INFO("4) erase the flash.\n");
	ret = st_flash_erase(info, erase_command);
	if (ret) {
		TS_LOG_ERR("Cannot erase the flash device\n");
		goto fw_done;
	}

	/* wait for a while */
	TS_LOG_INFO("5) checking for status.\n");
	status = st_flash_status(info, 3000, 100);
	if ((status == FLASH_STATUS_UNKNOWN) || (status == FLASH_STATUS_BUSY)) {
		TS_LOG_ERR("Wrong flash status 4\n");
		goto fw_done;
	}

	TS_LOG_INFO("6) program the flash.\n");
	ret = st_flash_program(info, program_command);
	if (ret) {
		TS_LOG_ERR("Cannot program the flash device\n");
		goto fw_done;
	}

	/* wait for a while */
	status = st_flash_status(info, 3000, 100);
	if ((status == FLASH_STATUS_UNKNOWN) || (status == FLASH_STATUS_BUSY)) 
	{
		TS_LOG_ERR("Wrong flash status 5\n");
		goto fw_done;
	}

	TS_LOG_INFO("Flash programming: done.\n");

	TS_LOG_INFO("Perform a system reset\n");
	ret = st_systemreset(info);
	if (ret){
		TS_LOG_ERR("Cannot reset the device\n");
		goto fw_done;
	}
	st_wait_controller_ready(info);
	st_interrupt_set(info, INT_ENABLE);
	mdelay(5);
	ret = st_get_fw_version(info);
	if (ret){
		TS_LOG_ERR("Cannot retrieve firmware version\n");
		goto fw_done;
	}

	TS_LOG_INFO("%s: tp:fw_version = %x, config_id = %x. bin: fw_ver = %x, config_ver = %x\n", __func__, 
			info->fw_version, info->config_id, fw_ver, config_ver);

	if(fw_ver == info->fw_version && config_ver == info->config_id){
		info->fwupdate_stat = 2;
		TS_LOG_INFO("%s: firmware update OK!\n", __func__);
	}else {
		if (updata_loop < 2){
			updata_loop++;
			mode = MODE_RELEASE_ONLY;
			TS_LOG_ERR("%s: firmware updata failed, update again %d******************\n", __func__, updata_loop);
			goto fts_updat;
		}
		TS_LOG_ERR("%s: firmware update failed!\n", __func__);
		ret = -1;//fw upgrade error
	}

	TS_LOG_INFO("New firmware version 0x%04x installed\n",
		info->fw_version);

fw_done:
	release_firmware(fw);
	return ret;
}
