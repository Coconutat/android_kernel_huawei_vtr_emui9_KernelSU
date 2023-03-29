/*
 * Copyright (C) 2010 - 2016 Novatek, Inc.
 *
 * $Revision: 6032 $
 * $Date: 2016-08-22 11:26:22 +0800 (週一, 22 八月 2016) $
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

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/vmalloc.h>
//#include <linux/fs.h>
#include <asm/uaccess.h>
#include <../../huawei_ts_kit.h>

#include "NVTtouch_207.h"

#define NVT_HYBRID_FW_BIN_SIZE_124KB 126976
#define NVT_HYBRID_FW_BIN_SIZE NVT_HYBRID_FW_BIN_SIZE_124KB
#define NVT_HYBRID_FW_BIN_VER_OFFSET 0x1E000
#define NVT_HYBRID_FW_BIN_VER_BAR_OFFSET 0x1E001
#define NVT_HYBRID_FW_BIN_CHIP_ID_OFFSET 0x1A00E
#define NVT_HYBRID_FW_BIN_CHIP_ID 8
#define NVT_HYBRID_FLASH_SECTOR_SIZE 4096
#define NVT_HYBRID_SIZE_64KB 65536
#define NVT_HYBRID_BLOCK_64KB_NUM 2

struct nvt_hybrid_ts_firmware {
	size_t size;
	u8 *data;
};
static struct nvt_hybrid_ts_firmware nvt_ts_fw_entry;
static struct nvt_hybrid_ts_firmware *fw_entry = &nvt_ts_fw_entry;

const struct firmware *nvt_hybrid_fw_entry_boot = NULL;


extern struct nvt_hybrid_ts_data *nvt_hybrid_ts;
extern int32_t nvt_hybrid_ts_i2c_dummy_read(struct i2c_client *client, uint16_t i2c_addr);
extern int32_t nvt_hybrid_ts_i2c_read(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern int32_t nvt_hybrid_ts_i2c_write(struct i2c_client *client, uint16_t address, uint8_t *buf, uint16_t len);
extern void nvt_hybrid_hw_reset(void);
extern void nvt_hybrid_bootloader_reset(void);
extern int32_t nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RST_COMPLETE_STATE check_reset_state);
extern void nvt_hybrid_sw_reset_idle(void);

/*******************************************************
Description:
	Novatek touchscreen request update firmware function.

return:
	Executive outcomes. 0---succeed. -1,-22---failed.
*******************************************************/
int32_t nvt_hybrid_update_firmware_request(char *filename)
{
	int32_t ret = 0;

	if (NULL == filename) {
		return -1;
	}

	TS_LOG_INFO("%s: filename is %s\n", __func__, filename);

	ret = request_firmware(&nvt_hybrid_fw_entry_boot, filename, &nvt_hybrid_ts->ts_dev->dev);
	if (ret) {
		TS_LOG_ERR("%s: firmware load failed, ret=%d\n", __func__, ret);
		return ret;
	}
	fw_entry->size = nvt_hybrid_fw_entry_boot->size;
	fw_entry->data = (u8 *)nvt_hybrid_fw_entry_boot->data;
	TS_LOG_INFO("%s: fw_entry->size=%d\n", __func__, fw_entry->size);

	// check bin file size (124kb)
	if (fw_entry->size > NVT_HYBRID_FW_BIN_SIZE) {
		TS_LOG_ERR("%s: bin file size not match. (%zu)\n", __func__, fw_entry->size);
		return -EINVAL;
	}

	// check if FW version add FW version bar equals 0xFF
	if (*(fw_entry->data + NVT_HYBRID_FW_BIN_VER_OFFSET) + *(fw_entry->data + NVT_HYBRID_FW_BIN_VER_BAR_OFFSET) != 0xFF) {
		TS_LOG_ERR("%s: bin file FW_VER + FW_VER_BAR should be 0xFF!\n", __func__);
		TS_LOG_ERR("%s: FW_VER=0x%02X, FW_VER_BAR=0x%02X\n", __func__, *(fw_entry->data+NVT_HYBRID_FW_BIN_VER_OFFSET), *(fw_entry->data+NVT_HYBRID_FW_BIN_VER_BAR_OFFSET));
		return -EINVAL;
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen release update firmware function.

return:
	n.a.
*******************************************************/
void nvt_hybrid_update_firmware_release(void)
{
	if (nvt_hybrid_fw_entry_boot) {
		release_firmware(nvt_hybrid_fw_entry_boot);
	}
	nvt_hybrid_fw_entry_boot=NULL;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware version function.

return:
	Executive outcomes. 0---need update. 1---need not
	update.
*******************************************************/
int32_t NVT_Hybrid_Check_FW_Ver(void)
{
	uint8_t buf[16] = {0};
	int32_t ret = 0;

	//---dummy read to resume TP before writing command---
	nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);

	//write i2c index to 0x14700
	buf[0] = 0xFF;
	buf[1] = 0x01;
	buf[2] = 0x47;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: i2c write error!(%d)\n", __func__, ret);
		return ret;
	}

	//read Firmware Version
	buf[0] = 0x78;
	buf[1] = 0x00;
	buf[2] = 0x00;
	ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: i2c read error!(%d)\n", __func__, ret);
		return ret;
	}

	TS_LOG_INFO("IC FW Ver = 0x%02X, FW Ver Bar = 0x%02X\n", buf[1], buf[2]);
	TS_LOG_INFO("Bin FW Ver = 0x%02X, FW ver Bar = 0x%02X\n",
			fw_entry->data[NVT_HYBRID_FW_BIN_VER_OFFSET], fw_entry->data[NVT_HYBRID_FW_BIN_VER_BAR_OFFSET]);
	snprintf(nvt_hybrid_ts->chip_data->version_name,MAX_STR_LEN-1,"%02x ",buf[1]);

	// check IC FW_VER + FW_VER_BAR equals 0xFF or not, need to update if not
	if ((buf[1] + buf[2]) != 0xFF) {
		TS_LOG_ERR("%s: IC FW_VER + FW_VER_BAR not equals to 0xFF!\n", __func__);
		return 0;
	}

	// compare IC and binary FW version

	/* update fw, if *.bin firmware verison is different between ic firmware verison */
	if (buf[1] == fw_entry->data[NVT_HYBRID_FW_BIN_VER_OFFSET])
		return 1;
	else
		return 0;
}

/*******************************************************
Description:
	Novatek touchscreen check firmware checksum function.

return:
	Executive outcomes. 0---checksum not match.
	1---checksum match. -1--- checksum read failed.
*******************************************************/
int32_t NVT_Hybrid_Check_CheckSum(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14000;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[NVT_HYBRID_BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[NVT_HYBRID_BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	fw_bin_size = fw_entry->size;

	for (i = 0; i < NVT_HYBRID_BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * NVT_HYBRID_SIZE_64KB)) {
			// Calculate WR_Filechksum of each 64KB block
			len_in_blk = min(fw_bin_size - i * NVT_HYBRID_SIZE_64KB, (size_t)NVT_HYBRID_SIZE_64KB);
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] += fw_entry->data[k + i * NVT_HYBRID_SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			if (i == 0) {
				//---dummy read to resume TP before writing command---
				nvt_hybrid_ts_i2c_dummy_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address);
			}

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 7);
			if (ret < 0) {
				TS_LOG_ERR("%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
				if (ret < 0) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}
			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				TS_LOG_INFO("RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", i, RD_Filechksum[i], i, WR_Filechksum[i]);
				TS_LOG_INFO("%s: firmware checksum not match!!\n", __func__);
				return 0;
			}
		}
	}

	TS_LOG_INFO("%s: firmware checksum match\n", __func__);
	return 1;
}

/*******************************************************
Description:
	Novatek touchscreen initial bootloader and flash
	block function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t NVT_Hybrid_Init_BootLoader(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;

	// SW Reset & Idle
	nvt_hybrid_sw_reset_idle();

	// Initiate Flash Block
	buf[0] = 0x00;
	buf[1] = 0x00;
	buf[2] = NVT_HYBRID_I2C_FW_Address;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Inittial Flash Block error!!(%d)\n", __func__, ret);
		return ret;
	}
	msleep(5);

	// Check 0xAA (Initiate Flash Block)
	buf[0] = 0x00;
	buf[1] = 0x00;
	ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
	if (ret < 0) {
		TS_LOG_ERR("%s: Check 0xAA (Inittial Flash Block) error!!(%d)\n", __func__, ret);
		return ret;
	}
	if (buf[1] != 0xAA) {
		TS_LOG_ERR("%s: Check 0xAA (Inittial Flash Block) error!! status=0x%02X\n", __func__, buf[1]);
		return -1;
	}

	TS_LOG_INFO("%s: Init OK \n", __func__);
	msleep(20);

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen erase flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t NVT_Hybrid_Erase_Flash(void)
{
	uint8_t buf[64] = {0};
	int32_t ret = 0;
	int32_t count = 0;
	int32_t i = 0;
	int32_t Flash_Address = 0;
	int32_t retry = 0;

	if (fw_entry->size % NVT_HYBRID_FLASH_SECTOR_SIZE)
		count = fw_entry->size / NVT_HYBRID_FLASH_SECTOR_SIZE + 1;
	else
		count = fw_entry->size / NVT_HYBRID_FLASH_SECTOR_SIZE;

	for(i = 0; i < count; i++) {
		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
		if (ret < 0) {
			TS_LOG_ERR("%s: Write Enable error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(10);

		// Check 0xAA (Write Enable)
		buf[0] = 0x00;
		buf[1] = 0x00;
		ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
		if (ret < 0) {
			TS_LOG_ERR("%s: Check 0xAA (Write Enable) error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		if (buf[1] != 0xAA) {
			TS_LOG_ERR("%s: Check 0xAA (Write Enable) error!! status=0x%02X\n", __func__, buf[1]);
			return -1;
		}
		msleep(10);

		Flash_Address = i * NVT_HYBRID_FLASH_SECTOR_SIZE;

		// Sector Erase
		buf[0] = 0x00;
		buf[1] = 0x20;    // Command : Sector Erase
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 5);
		if (ret < 0) {
			TS_LOG_ERR("%s: Sector Erase error!!(%d,%d)\n", __func__, ret, i);
			return ret;
		}
		msleep(20);

		retry = 0;
		while (1) {
			// Check 0xAA (Sector Erase)
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Check 0xAA (Sector Erase) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if (buf[1] == 0xAA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s: Check 0xAA (Sector Erase) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}

		// Read Status
		retry = 0;
		while (1) {
			msleep(30);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Status error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Check 0xAA (Read Status) error!!(%d,%d)\n", __func__, ret, i);
				return ret;
			}
			if ((buf[1] == 0xAA) && (buf[2] == 0x00)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s:Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
	}

	TS_LOG_INFO("Erase OK \n");
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen write flash sectors function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t NVT_Hybrid_Write_Flash(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14002;
	uint32_t Flash_Address = 0;
	int32_t i = 0, j = 0, k = 0;
	uint8_t tmpvalue = 0;
	int32_t count = 0;
	int32_t ret = 0;
	int32_t retry = 0;

	// change I2C buffer index
	buf[0] = 0xFF;
	buf[1] = XDATA_Addr >> 16;
	buf[2] = (XDATA_Addr >> 8) & 0xFF;
	ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
	if (ret < 0) {
		TS_LOG_ERR("%s: change I2C buffer index error!!(%d)\n", __func__, ret);
		return ret;
	}

	if (fw_entry->size % 256)
		count = fw_entry->size / 256 + 1;
	else
		count = fw_entry->size / 256;

	for (i = 0; i < count; i++) {
		Flash_Address = i * 256;

		// Write Enable
		buf[0] = 0x00;
		buf[1] = 0x06;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
		if (ret < 0) {
			TS_LOG_ERR("%s: Write Enable error!!(%d)\n", __func__, ret);
			return ret;
		}
		udelay(100);

		// Write Page : 256 bytes
		for (j = 0; j < min(fw_entry->size - i * 256, (size_t)256); j += 32) {
			buf[0] = (XDATA_Addr + j) & 0xFF;
			for (k = 0; k < 32; k++) {
				buf[1 + k] = fw_entry->data[Flash_Address + j + k];
			}
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 33);
			if (ret < 0) {
				TS_LOG_ERR("%s: Write Page error!!(%d), j=%d\n", __func__, ret, j);
				return ret;
			}
		}
		if (fw_entry->size - Flash_Address >= 256)
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (255);
		else
			tmpvalue=(Flash_Address >> 16) + ((Flash_Address >> 8) & 0xFF) + (Flash_Address & 0xFF) + 0x00 + (fw_entry->size - Flash_Address - 1);

		for (k = 0;k < min(fw_entry->size - Flash_Address,(size_t)256); k++)
			tmpvalue += fw_entry->data[Flash_Address + k];

		tmpvalue = 255 - tmpvalue + 1;

		// Page Program
		buf[0] = 0x00;
		buf[1] = 0x02;
		buf[2] = ((Flash_Address >> 16) & 0xFF);
		buf[3] = ((Flash_Address >> 8) & 0xFF);
		buf[4] = (Flash_Address & 0xFF);
		buf[5] = 0x00;
		buf[6] = min(fw_entry->size - Flash_Address,(size_t)256) - 1;
		buf[7] = tmpvalue;
		ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 8);
		if (ret < 0) {
			TS_LOG_ERR("%s: Page Program error!!(%d), i=%d\n", __func__, ret, i);
			return ret;
		}

		// Check 0xAA (Page Program)
		retry = 0;
		while (1) {
			mdelay(3);
			buf[0] = 0x00;
			buf[1] = 0x00;
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Page Program error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (buf[1] == 0xAA || buf[1] == 0xEA) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s: Check 0xAA (Page Program) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			TS_LOG_ERR("%s: Page Program error!! i=%d\n", __func__, i);
			return -3;
		}

		// Read Status
		retry = 0;
		while (1) {
			mdelay(2);
			buf[0] = 0x00;
			buf[1] = 0x05;
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Status error!!(%d)\n", __func__, ret);
				return ret;
			}

			// Check 0xAA (Read Status)
			buf[0] = 0x00;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Check 0xAA (Read Status) error!!(%d)\n", __func__, ret);
				return ret;
			}
			if (((buf[1] == 0xAA) && (buf[2] == 0x00)) || (buf[1] == 0xEA)) {
				break;
			}
			retry++;
			if (unlikely(retry > 5)) {
				TS_LOG_ERR("%s: Check 0xAA (Read Status) failed, buf[1]=0x%02X, buf[2]=0x%02X, retry=%d\n", __func__, buf[1], buf[2], retry);
				return -1;
			}
		}
		if (buf[1] == 0xEA) {
			TS_LOG_ERR("%s: Page Program error!! i=%d\n", __func__, i);
			return -4;
		}

		TS_LOG_INFO("%s: Programming...%2d%%\r", __func__, ((i * 100) / count));
	}

	TS_LOG_INFO("%s: Programming...%2d%%\r", __func__, 100);
	TS_LOG_INFO("%s: Program OK\n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen verify checksum of written
	flash function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t NVT_Hybrid_Verify_Flash(void)
{
	uint8_t buf[64] = {0};
	uint32_t XDATA_Addr = 0x14000;
	int32_t ret = 0;
	int32_t i = 0;
	int32_t k = 0;
	uint16_t WR_Filechksum[NVT_HYBRID_BLOCK_64KB_NUM] = {0};
	uint16_t RD_Filechksum[NVT_HYBRID_BLOCK_64KB_NUM] = {0};
	size_t fw_bin_size = 0;
	size_t len_in_blk = 0;
	int32_t retry = 0;

	fw_bin_size = fw_entry->size;

	for (i = 0; i < NVT_HYBRID_BLOCK_64KB_NUM; i++) {
		if (fw_bin_size > (i * NVT_HYBRID_SIZE_64KB)) {
			// Calculate WR_Filechksum of each 64KB block
			len_in_blk = min(fw_bin_size - i * NVT_HYBRID_SIZE_64KB, (size_t)NVT_HYBRID_SIZE_64KB);
			WR_Filechksum[i] = i + 0x00 + 0x00 + (((len_in_blk - 1) >> 8) & 0xFF) + ((len_in_blk - 1) & 0xFF);
			for (k = 0; k < len_in_blk; k++) {
				WR_Filechksum[i] += fw_entry->data[k + i * NVT_HYBRID_SIZE_64KB];
			}
			WR_Filechksum[i] = 65535 - WR_Filechksum[i] + 1;

			// Fast Read Command
			buf[0] = 0x00;
			buf[1] = 0x07;
			buf[2] = i;
			buf[3] = 0x00;
			buf[4] = 0x00;
			buf[5] = ((len_in_blk - 1) >> 8) & 0xFF;
			buf[6] = (len_in_blk - 1) & 0xFF;
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 7);
			if (ret < 0) {
				TS_LOG_ERR("%s: Fast Read Command error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Check 0xAA (Fast Read Command)
			retry = 0;
			while (1) {
				msleep(80);
				buf[0] = 0x00;
				buf[1] = 0x00;
				ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_HW_Address, buf, 2);
				if (ret < 0) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) error!!(%d)\n", __func__, ret);
					return ret;
				}
				if (buf[1] == 0xAA) {
					break;
				}
				retry++;
				if (unlikely(retry > 5)) {
					TS_LOG_ERR("%s: Check 0xAA (Fast Read Command) failed, buf[1]=0x%02X, retry=%d\n", __func__, buf[1], retry);
					return -1;
				}
			}
			// Read Checksum (write addr high byte & middle byte)
			buf[0] = 0xFF;
			buf[1] = XDATA_Addr >> 16;
			buf[2] = (XDATA_Addr >> 8) & 0xFF;
			ret = nvt_hybrid_ts_i2c_write(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum (write addr high byte & middle byte) error!!(%d)\n", __func__, ret);
				return ret;
			}
			// Read Checksum
			buf[0] = (XDATA_Addr) & 0xFF;
			buf[1] = 0x00;
			buf[2] = 0x00;
			ret = nvt_hybrid_ts_i2c_read(nvt_hybrid_ts->client, NVT_HYBRID_I2C_FW_Address, buf, 3);
			if (ret < 0) {
				TS_LOG_ERR("%s: Read Checksum error!!(%d)\n", __func__, ret);
				return ret;
			}

			RD_Filechksum[i] = (uint16_t)((buf[2] << 8) | buf[1]);
			if (WR_Filechksum[i] != RD_Filechksum[i]) {
				TS_LOG_ERR("%s: Verify Fail%d!!\n", __func__, i);
				TS_LOG_ERR("%s: RD_Filechksum[%d]=0x%04X, WR_Filechksum[%d]=0x%04X\n", __func__, i, RD_Filechksum[i], i, WR_Filechksum[i]);
				return -1;
			}
		}
	}

	TS_LOG_INFO("%s: Verify OK \n", __func__);
	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware function.

return:
	Executive outcomes. 0---succeed. negative---failed.
*******************************************************/
int32_t NVT_Hybrid_Update_Firmware(void)
{
	int32_t ret = 0;

	TS_LOG_INFO("%s: firmware Upgrade Start!\n", __func__);
	// Step 1 : initial bootloader
	ret = NVT_Hybrid_Init_BootLoader();
	if (ret) {
		return ret;
	}

	// Step 2 : Erase
	ret = NVT_Hybrid_Erase_Flash();
	if (ret) {
		return ret;
	}

	// Step 3 : Program
	ret = NVT_Hybrid_Write_Flash();
	if (ret) {
		return ret;
	}

	// Step 4 : Verify
	ret = NVT_Hybrid_Verify_Flash();
	if (ret) {
		return ret;
	}

	//Step 5 : Bootloader Reset
	nvt_hybrid_bootloader_reset();
	nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT);
	snprintf(nvt_hybrid_ts->chip_data->version_name,MAX_STR_LEN-1,"%02x",fw_entry->data[NVT_HYBRID_FW_BIN_VER_OFFSET]);
	TS_LOG_INFO("%s: firmware Upgrade Done!\n", __func__);
	return ret;
}

int g_nvt_hybrid_sd_force_update = 0;
int32_t nvt_bybrid_fw_update_boot(char *file_name)
{
	int32_t ret = 0;
	u8 firmware[64];

	nvt_hybrid_ts->firmware_updating = true;
	
	TS_LOG_INFO("%s: file_name=%s\n", __func__, file_name);
	snprintf(firmware, PAGE_SIZE, "ts/%s", file_name);

	ret = nvt_hybrid_update_firmware_request(firmware);
	if (ret) {
		TS_LOG_ERR("%s: nvt_hybrid_update_firmware_request failed. (%d)\n", __func__, ret);
		ret = 1; /* If firmware not exist, return not need update fw*/
		goto err_nvt_hybrid_update_firmware_request;
	}

	//---Start firmware update procedure---
	nvt_hybrid_sw_reset_idle();
	if (!g_nvt_hybrid_sd_force_update){
		//---prevent last time write step was discontinued by unpredictable power off, Taylor 20160715---
		ret = NVT_Hybrid_Check_CheckSum();
		//--------------------------------------------------------------------------------

		if (ret < 0) {	// read firmware checksum failed
			TS_LOG_ERR("%s: read firmware checksum failed\n", __func__);
		//	nvt_hybrid_ts->firmware_updating=true;
			ret = NVT_Hybrid_Update_Firmware();
		} else if ((ret == 0) && (NVT_Hybrid_Check_FW_Ver() == 0)) {	// (ic fw ver check failed) && (bin fw version > ic fw version)
			TS_LOG_INFO("%s: firmware version not match\n", __func__);
		//	nvt_hybrid_ts->firmware_updating=true;
			ret = NVT_Hybrid_Update_Firmware();
		} else {
			// Bootloader Reset
		//	nvt_hybrid_ts->firmware_updating=false;
			ret = 0;
			nvt_hybrid_bootloader_reset();
			nvt_hybrid_check_fw_reset_state(NVT_HYBRID_RESET_STATE_INIT);
		}
	}else{
		g_nvt_hybrid_sd_force_update = 0;

		TS_LOG_INFO("%s: fw force update\n", __func__);
		ret = NVT_Hybrid_Update_Firmware();
	}

	nvt_hybrid_update_firmware_release();

err_nvt_hybrid_update_firmware_request:
	nvt_hybrid_ts->firmware_updating = false;
	return ret;
}

int32_t nvt_hybrid_fw_update_sd(void)
{
	TS_LOG_INFO("%s enter\n", __func__);
	int32_t ret = 0;

	g_nvt_hybrid_sd_force_update = 1;

	nvt_bybrid_fw_update_boot("touch_screen_firmware.img");

	return ret;
}
