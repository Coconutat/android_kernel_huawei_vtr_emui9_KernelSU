/*
 * Copyright (C) 2010 - 2017 Novatek, Inc.
 *
 * $Revision$
 * $Date$
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
#include <linux/seq_file.h>
#include <linux/delay.h>
#include <linux/vmalloc.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/gpio.h>
#if defined (CONFIG_HUAWEI_DSM)
#include <dsm/dsm_pub.h>

extern struct dsm_client *ts_dclient;
#endif
extern struct ts_kit_platform_data g_ts_kit_platform_data;


#include "nt36xxx.h"


#define FW_BIN_SIZE_116KB		(118784)
#define FW_BIN_SIZE FW_BIN_SIZE_116KB
#define FW_BIN_VER_OFFSET		(0x1A000)
#define FW_BIN_VER_BAR_OFFSET	(0x1A001)
#if defined (CONFIG_HUAWEI_DSM)
enum FW_uptate_state
{
	Nova_Init_BootLoader_fail = 0,
	Nova_Resume_PD_fail,
	Erase_Flash_fail,
	Write_Flash_fail,
	Verify_Flash_fail,
	TS_UPDATE_STATE_UNDEFINE = 255,
};

enum SPI_COM_MODE {
	INTERRUPT_MODE = 0,
	POLLING_MODE,
	DMA_MODE,
};
#endif

#define NVT_DUMP_PARTITION      (0)
#define NVT_DUMP_PARTITION_LEN  (1024)
#define NVT_DUMP_PARTITION_PATH "/data/local/tmp"

struct timeval start, end;
const struct firmware *fw_entry = NULL;
static uint8_t *fwbuf = NULL;

struct nvt_ts_bin_map {
	char name[12];
	uint32_t BIN_addr;
	uint32_t SRAM_addr;
	uint32_t size;
	uint32_t crc;
};

static struct nvt_ts_bin_map *bin_map;

extern struct nvt_ts_data *nvt_ts;

/*******************************************************
Description:
	Novatek touchscreen init variable and allocate buffer
for download firmware function.

return:
	n.a.
*******************************************************/
static int32_t nvt_download_init(void)
{
	/* allocate buffer for transfer firmware */
	//TS_LOG_INFO("NVT_TANSFER_LEN = %ld\n", NVT_TANSFER_LEN);

	if (fwbuf == NULL) {
		fwbuf = (uint8_t *)kzalloc((NVT_TANSFER_LEN+1), GFP_KERNEL);
		if(fwbuf == NULL) {
			TS_LOG_ERR("kzalloc for fwbuf failed!\n");
			return -ENOMEM;
		}
	}

	return 0;
}

static uint32_t byte_to_word(const uint8_t *data)
{
	return data[0] + (data[1] << 8) + (data[2] << 16) + (data[3] << 24);
}

/*******************************************************
Description:
	Novatek touchscreen parsing bin header function.

return:
	n.a.
*******************************************************/
static uint32_t partition = 0;
static uint8_t ilm_dlm_num = 2;
static int32_t nvt_bin_header_parser(const u8 *fwdata, size_t fwsize)
{
	uint32_t list = 0;
	uint32_t pos = 0x00;
	uint32_t end = 0x00;
	uint8_t info_sec_num = 0;
	uint8_t ovly_sec_num = 0;
	uint8_t ovly_info = 0;

	/* Find the header size */
	end = fwdata[0] + (fwdata[1] << 8) + (fwdata[2] << 16) + (fwdata[3] << 24);
	pos = 0x30;	// info section start at 0x30 offset
	while (pos < end) {
		info_sec_num ++;
		pos += 0x10;	/* each header info is 16 bytes */
	}

	/*
	 * Find the DLM OVLY section
	 * [0:3] Overlay Section Number
	 * [4]   Overlay Info
	 */
	ovly_info = (fwdata[0x28] & 0x10) >> 4;
	ovly_sec_num = (ovly_info) ? (fwdata[0x28] & 0x0F) : 0;

	/*
	 * calculate all partition number
	 * ilm_dlm_num (ILM & DLM) + ovly_sec_num + info_sec_num
	 */
	partition = ilm_dlm_num + ovly_sec_num + info_sec_num;
	TS_LOG_INFO("ovly_info = %d, ilm_dlm_num = %d, ovly_sec_num = %d, info_sec_num = %d, partition = %d\n",
			ovly_info, ilm_dlm_num, ovly_sec_num, info_sec_num, partition);

	/* allocated memory for header info */
	bin_map = (struct nvt_ts_bin_map *)kzalloc((partition+1) * sizeof(struct nvt_ts_bin_map), GFP_KERNEL);
	if(bin_map == NULL) {
		TS_LOG_ERR("kzalloc for bin_map failed!\n");
		return -ENOMEM;
	}

	for (list = 0; list < partition; list++) {
		/*
		 * [1] parsing ILM & DLM header info
		 * BIN_addr : SRAM_addr : size (12-bytes)
		 * crc located at 0x18 & 0x1C
		 */
		if (list < ilm_dlm_num) {
			bin_map[list].BIN_addr = byte_to_word(&fwdata[0 + list*12]);
			bin_map[list].SRAM_addr = byte_to_word(&fwdata[4 + list*12]);
			bin_map[list].size = byte_to_word(&fwdata[8 + list*12]);
			bin_map[list].crc = byte_to_word(&fwdata[0x18 + list*4]);
			if (list == 0)
				sprintf(bin_map[list].name, "ILM");
			else if (list == 1)
				sprintf(bin_map[list].name, "DLM");
		}

		/*
		 * [2] parsing others header info
		 * SRAM_addr : size : BIN_addr : crc (16-bytes)
		 */
		if ((list >= ilm_dlm_num) && (list < (ilm_dlm_num + info_sec_num))) {
			/* others partition located at 0x30 offset */
			pos = 0x30 + (0x10 * (list - ilm_dlm_num));

			bin_map[list].SRAM_addr = byte_to_word(&fwdata[pos]);
			bin_map[list].size = byte_to_word(&fwdata[pos+4]);
			bin_map[list].BIN_addr = byte_to_word(&fwdata[pos+8]);
			bin_map[list].crc = byte_to_word(&fwdata[pos+12]);
			/* detect header end to protect parser function */
			if ((bin_map[list].BIN_addr == 0) && (bin_map[list].size != 0)) {
				sprintf(bin_map[list].name, "Header");
			} else {
				sprintf(bin_map[list].name, "Info-%d", (list - ilm_dlm_num));
			}
		}

		/*
		 * [3] parsing overlay section header info
		 * SRAM_addr : size : BIN_addr : crc (16-bytes)
		 */
		if (list >= (ilm_dlm_num + info_sec_num)) {
			/* overlay info located at DLM (list = 1) start addr */
			pos = bin_map[1].BIN_addr + (0x10 * (list- ilm_dlm_num - info_sec_num));

			bin_map[list].SRAM_addr = byte_to_word(&fwdata[pos]);
			bin_map[list].size = byte_to_word(&fwdata[pos+4]);
			bin_map[list].BIN_addr = byte_to_word(&fwdata[pos+8]);
			bin_map[list].crc = byte_to_word(&fwdata[pos+12]);
			sprintf(bin_map[list].name, "Overlay-%d", (list- ilm_dlm_num - info_sec_num));
		}

		/* BIN size error detect */
		if ((bin_map[list].BIN_addr + bin_map[list].size) > fwsize) {
			TS_LOG_ERR("access range (0x%08X to 0x%08X) is larger than bin size!\n",
					bin_map[list].BIN_addr, bin_map[list].BIN_addr + bin_map[list].size);
			return -EINVAL;
		}

//		TS_LOG_INFO("[%d][%s] SRAM (0x%08X), SIZE (0x%08X), BIN (0x%08X), CRC (0x%08X)\n",
//				list, bin_map[list].name,
//				bin_map[list].SRAM_addr, bin_map[list].size,  bin_map[list].BIN_addr, bin_map[list].crc);
	}

	return 0;
}

/*******************************************************
Description:
	Novatek touchscreen release update firmware function.

return:
	n.a.
*******************************************************/
static void update_firmware_release(void)
{
	if (fw_entry) {
		release_firmware(fw_entry);
	}

	fw_entry = NULL;
}

/*******************************************************
Description:
	Novatek touchscreen request update firmware function.

return:
	Executive outcomes. 0---succeed. -1,-22---failed.
*******************************************************/
static int32_t update_firmware_request(char *filename)
{
	uint8_t retry = 0;
	int32_t ret = 0;

	if (NULL == filename) {
		return -ENOENT;
	}

	while (1) {
		TS_LOG_INFO("filename is %s\n", filename);

		ret = request_firmware(&fw_entry, filename, &nvt_ts->ts_dev->dev);
		if (ret) {
			TS_LOG_ERR("firmware load failed, ret=%d\n", ret);
			goto request_fail;
		}

		// check bin file size (116kb)
		if (fw_entry->size != FW_BIN_SIZE) {
			TS_LOG_ERR("bin file size not match. (%zu)\n", fw_entry->size);
			ret = -ENOEXEC;
			goto invalid;
		}

		// check if FW version add FW version bar equals 0xFF
		if (*(fw_entry->data + FW_BIN_VER_OFFSET) + *(fw_entry->data + FW_BIN_VER_BAR_OFFSET) != 0xFF) {
			TS_LOG_ERR("bin file FW_VER + FW_VER_BAR should be 0xFF!\n");
			TS_LOG_ERR("FW_VER=0x%02X, FW_VER_BAR=0x%02X\n", *(fw_entry->data+FW_BIN_VER_OFFSET), *(fw_entry->data+FW_BIN_VER_BAR_OFFSET));
			ret = -ENOEXEC;
			goto invalid;
		}

		/* BIN Header Parser */
		ret = nvt_bin_header_parser(fw_entry->data, fw_entry->size);
		if (ret) {
			TS_LOG_ERR("bin header parser failed\n");
			goto invalid;
		} else {
			break;
		}

invalid:
		update_firmware_release();
		if (!IS_ERR_OR_NULL(bin_map)) {
			kfree(bin_map);
			bin_map = NULL;
		}

request_fail:
		retry++;
		if(unlikely(retry > 2)) {
			TS_LOG_ERR("error, retry=%d\n", retry);
			break;
		}
	}

	return ret;
}

#if NVT_DUMP_PARTITION
/*******************************************************
Description:
	Novatek touchscreen dump flash partition function.

return:
	n.a.
*******************************************************/
loff_t file_offset = 0;
static int32_t nvt_read_ram_and_save_file(uint32_t addr, uint16_t len, char *name)
{
	char file[256] = "";
	uint8_t *fbufp = NULL;
	int32_t ret = 0;
	struct file *fp = NULL;
	mm_segment_t org_fs;

	sprintf(file, "%s/dump_%s.bin", NVT_DUMP_PARTITION_PATH, name);
	TS_LOG_INFO("Dump [%s] from 0x%08X to 0x%08X\n", file, addr, addr+len);

	fbufp = (uint8_t *)kzalloc(len+1, GFP_KERNEL);
	if(fbufp == NULL) {
		TS_LOG_ERR("kzalloc for fbufp failed!\n");
		ret = -ENOMEM;
		goto alloc_buf_fail;
	}

	org_fs = get_fs();
	set_fs(KERNEL_DS);
	fp = filp_open(file, O_RDWR | O_CREAT, 0644);
	if (fp == NULL || IS_ERR(fp)) {
		ret = -ENOMEM;
		TS_LOG_ERR("open file failed\n");
		goto open_file_fail;
	}

	/* SPI read */
	//---set xdata index to addr---
	nvt_set_page(addr);

	fbufp[0] = addr & 0x7F;	//offset
	ret = novatek_ts_kit_read(I2C_FW_Address, fbufp, len+1);
	if (ret) {
		TS_LOG_INFO("%s: read data FAIL\n", __func__);
	}

	/* Write to file */
	ret = vfs_write(fp, (char __user *)fbufp+1, len, &file_offset);
	if (ret != len) {
		TS_LOG_ERR("write file failed\n");
		goto open_file_fail;
	} else {
		ret = 0;
	}

open_file_fail:
	set_fs(org_fs);
	if (!IS_ERR_OR_NULL(fp)) {
		filp_close(fp, NULL);
		fp = NULL;
	}

	if (!IS_ERR_OR_NULL(fbufp)) {
		kfree(fbufp);
		fbufp = NULL;
	}
alloc_buf_fail:

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen nvt_dump_partition function to dump
 each partition for debug.

return:
	n.a.
*******************************************************/
static int32_t nvt_dump_partition(void)
{
	uint32_t list = 0;
	char *name;
	uint32_t SRAM_addr, size;
	uint32_t i = 0;
	uint16_t len = 0;
	int32_t count = 0;
	int32_t ret = 0;

	if (NVT_DUMP_PARTITION_LEN >= sizeof(nvt_ts->rbuf)) {
		TS_LOG_ERR("dump len %d is larger than buffer size %ld\n",
				NVT_DUMP_PARTITION_LEN, sizeof(nvt_ts->rbuf));
		return -EINVAL;
	} else if (NVT_DUMP_PARTITION_LEN >= NVT_TANSFER_LEN) {
		TS_LOG_ERR("dump len %d is larger than NVT_TANSFER_LEN\n", NVT_DUMP_PARTITION_LEN);
		return -EINVAL;
	}

	if (bin_map == NULL) {
		TS_LOG_ERR("bin_map is NULL\n");
		return -ENOMEM;
	}

	memset(fwbuf, 0, (NVT_DUMP_PARTITION_LEN+1));

	for (list = 0; list < partition; list++) {
		/* initialize variable */
		SRAM_addr = bin_map[list].SRAM_addr;
		size = bin_map[list].size;
		name = bin_map[list].name;

		/* ignore reserved partition (Reserved Partition size is zero) */
		if (!size)
			continue;
		else
			size = size +1;

		/* write data to SRAM */
		if (size % NVT_DUMP_PARTITION_LEN)
			count = (size / NVT_DUMP_PARTITION_LEN) + 1;
		else
			count = (size / NVT_DUMP_PARTITION_LEN);

		for (i = 0 ; i < count ; i++) {
			len = (size < NVT_DUMP_PARTITION_LEN) ? size : NVT_DUMP_PARTITION_LEN;

			/* dump for debug download firmware */
			ret = nvt_read_ram_and_save_file(SRAM_addr, len, name);
			if (ret < 0) {
				TS_LOG_ERR("nvt_read_ram_and_save_file failed, ret = %d\n", ret);
				goto out;
			}

			SRAM_addr += NVT_DUMP_PARTITION_LEN;
			size -= NVT_DUMP_PARTITION_LEN;
		}

		file_offset = 0;
	}

out:
	return ret;
}
#endif /* NVT_DUMP_PARTITION */

/*******************************************************
Description:
	Novatek touchscreen nvt_write_firmware function to write
firmware into each partition.

return:
	n.a.
*******************************************************/
static int32_t nvt_write_firmware(const u8 *fwdata, size_t fwsize)
{
	uint32_t list = 0;
	char *name;
	uint32_t BIN_addr, SRAM_addr, size;
	uint32_t i = 0;
	int32_t len = 0;
	int32_t count = 0;
	int32_t ret = 0;

	memset(fwbuf, 0, (NVT_TANSFER_LEN+1));

	for (list = 0; list < partition; list++) {
		/* initialize variable */
		SRAM_addr = bin_map[list].SRAM_addr;
		size = bin_map[list].size;
		BIN_addr = bin_map[list].BIN_addr;
		name = bin_map[list].name;

//		TS_LOG_INFO("[%d][%s] SRAM (0x%08X), SIZE (0x%08X), BIN (0x%08X)\n",
//				list, name, SRAM_addr, size, BIN_addr);

		/* Check data size */
		if ((BIN_addr + size) > fwsize) {
			TS_LOG_ERR("access range (0x%08X to 0x%08X) is larger than bin size!\n",
					BIN_addr, BIN_addr + size);
			ret = -EINVAL;
			goto out;
		}

		/* ignore reserved partition (Reserved Partition size is zero) */
		if (!size)
			continue;
		else
			size = size +1;

		/* write data to SRAM */
		if (size % NVT_TANSFER_LEN)
			count = (size / NVT_TANSFER_LEN) + 1;
		else
			count = (size / NVT_TANSFER_LEN);

		for (i = 0 ; i < count ; i++) {
			len = (size < NVT_TANSFER_LEN) ? size : NVT_TANSFER_LEN;

			//---set xdata index to start address of SRAM---
			nvt_set_page(SRAM_addr);

			//---write data into SRAM---
			fwbuf[0] = SRAM_addr & 0x7F;	//offset
			memcpy(fwbuf+1, &fwdata[BIN_addr], len);	//payload
			ret = novatek_ts_kit_write(I2C_FW_Address, fwbuf, len+1);
			if (ret) {
				TS_LOG_INFO("%s: write data into SRAM FAIL\n", __func__);
			}

			SRAM_addr += NVT_TANSFER_LEN;
			BIN_addr += NVT_TANSFER_LEN;
			size -= NVT_TANSFER_LEN;
		}
	}

out:
	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen set bootload crc reg bank function.
This function will set hw crc reg before enable crc function.

return:
	n.a.
*******************************************************/
static void nvt_set_bld_crc_bank(uint32_t DES_ADDR, uint32_t SRAM_ADDR,
		uint32_t LENGTH_ADDR, uint32_t size,
		uint32_t G_CHECKSUM_ADDR, uint32_t crc)
{
	int32_t ret = 0;

	/* write destination address */
	nvt_set_page(DES_ADDR);
	fwbuf[0] = DES_ADDR & 0x7F;
	fwbuf[1] = (SRAM_ADDR) & 0xFF;
	fwbuf[2] = (SRAM_ADDR >> 8) & 0xFF;
	fwbuf[3] = (SRAM_ADDR >> 16) & 0xFF;
	ret = novatek_ts_kit_write(I2C_FW_Address, fwbuf, 4);
	if (ret) {
		TS_LOG_INFO("%s: write destination address FAIL\n", __func__);
	}

	/* write length */
	//nvt_set_page(LENGTH_ADDR);
	fwbuf[0] = LENGTH_ADDR & 0x7F;
	fwbuf[1] = (size) & 0xFF;
	fwbuf[2] = (size >> 8) & 0xFF;
	ret = novatek_ts_kit_write(I2C_FW_Address, fwbuf, 3);
	if (ret) {
		TS_LOG_INFO("%s: write length FAIL\n", __func__);
	}

	/* write golden dlm checksum */
	//nvt_set_page(G_CHECKSUM_ADDR);
	fwbuf[0] = G_CHECKSUM_ADDR & 0x7F;
	fwbuf[1] = (crc) & 0xFF;
	fwbuf[2] = (crc >> 8) & 0xFF;
	fwbuf[3] = (crc >> 16) & 0xFF;
	fwbuf[4] = (crc >> 24) & 0xFF;
	ret = novatek_ts_kit_write(I2C_FW_Address, fwbuf, 5);
	if (ret) {
		TS_LOG_INFO("%s: write golden dlm checksum FAIL\n", __func__);
	}

	return;
}

/*******************************************************
Description:
	Novatek touchscreen check DMA hw crc function.
This function will check hw crc result is pass or not.

return:
	n.a.
*******************************************************/
static void nvt_set_bld_hw_crc(void)
{
	/* [0] ILM */
	/* write register bank */
	nvt_set_bld_crc_bank(nvt_ts->mmap->ILM_DES_ADDR, bin_map[0].SRAM_addr,
			nvt_ts->mmap->ILM_LENGTH_ADDR, bin_map[0].size,
			nvt_ts->mmap->G_ILM_CHECKSUM_ADDR, bin_map[0].crc);

	/* [1] DLM */
	/* write register bank */
	nvt_set_bld_crc_bank(nvt_ts->mmap->DLM_DES_ADDR, bin_map[1].SRAM_addr,
			nvt_ts->mmap->DLM_LENGTH_ADDR, bin_map[1].size,
			nvt_ts->mmap->G_DLM_CHECKSUM_ADDR, bin_map[1].crc);
}

/*******************************************************
Description:
	Novatek touchscreen Download_Firmware with HW CRC
function. It's complete download firmware flow.

return:
	n.a.
*******************************************************/
static int32_t nvt_download_firmware_hw_crc(void)
{
	uint8_t retry = 0;
	int32_t ret = 0;

	do_gettimeofday(&start);

	while (1) {
		/* bootloader reset to reset MCU */
		nvt_kit_bootloader_reset();

		/* Start to write firmware process */
		ret = nvt_write_firmware(fw_entry->data, fw_entry->size);
		if (ret) {
			TS_LOG_ERR("Write_Firmware failed. (%d)\n", ret);
			goto fail;
		}

#if NVT_DUMP_PARTITION
		ret = nvt_dump_partition();
		if (ret) {
			TS_LOG_ERR("nvt_dump_partition failed, ret = %d\n", ret);
		}
#endif

		/* set ilm & dlm reg bank */
		nvt_set_bld_hw_crc();

		/* enable hw bld crc function */
		nvt_bld_crc_enable();

		/* clear fw reset status & enable fw crc check */
		nvt_fw_crc_enable();

		/* Set Boot Ready Bit */
		nvt_boot_ready();

		ret = nvt_kit_check_fw_reset_state(RESET_STATE_INIT);
		if (ret) {
			TS_LOG_ERR("nvt_kit_check_fw_reset_state failed. (%d)\n", ret);
			goto fail;
		} else {
			break;
		}

fail:
		retry++;
		if(unlikely(retry > 2)) {
			TS_LOG_ERR("error, retry=%d\n", retry);
			break;
		}
	}

	do_gettimeofday(&end);

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware main function.

return:
	n.a.
*******************************************************/
int32_t nvt_kit_fw_update_boot_spi(char *file_name)
{
	int32_t ret = 0;
	if(nvt_ts->use_dma_download_firmware){
		g_ts_kit_platform_data.spidev0_chip_info.com_mode = (unsigned int) DMA_MODE;
	}

	ret = update_firmware_request(file_name);
	//TODO: project id not ready. force update ts/touch_screen_firmware.bin
	//ret = update_firmware_request(file_name);
	if (ret) {
		TS_LOG_ERR("update_firmware_request failed. (%d)\n", ret);
		goto request_firmware_fail;
	}

	/* initial buffer and variable */
	ret = nvt_download_init();
	if (ret) {
		TS_LOG_ERR("Download Init failed. (%d)\n", ret);
		goto download_fail;
	}

	/* download firmware process */
	ret = nvt_download_firmware_hw_crc();
	if (ret) {
		TS_LOG_ERR("Download Firmware failed. (%d)\n", ret);
		goto download_fail;
	}

	TS_LOG_INFO("Update firmware success! <%ld us>\n",
			(end.tv_sec - start.tv_sec)*1000000L + (end.tv_usec - start.tv_usec));

	/* Get FW Info */
	ret = nvt_kit_get_fw_info();
	if (ret) {
		TS_LOG_ERR("nvt_kit_get_fw_info failed. (%d)\n", ret);
		goto download_fail;
	}

download_fail:
	if (!IS_ERR_OR_NULL(bin_map)) {
		kfree(bin_map);
		bin_map = NULL;
	}
	update_firmware_release();
request_firmware_fail:

	if(nvt_ts->use_dma_download_firmware){
		g_ts_kit_platform_data.spidev0_chip_info.com_mode = (unsigned int) POLLING_MODE;
	}

	return ret;
}

/*******************************************************
Description:
	Novatek touchscreen update firmware when booting
	function.

return:
	n.a.
*******************************************************/
void Boot_Update_Firmware(struct work_struct *work)
{
	mutex_lock(&nvt_ts->lock);
	nvt_kit_fw_update_boot_spi(NOVATEK_FW_MANUAL_UPDATE_FILE_NAME);
	mutex_unlock(&nvt_ts->lock);
}
