/*

 **************************************************************************
 **                        STMicroelectronics                            **
 **************************************************************************
 **                        marco.cali@st.com**
 **************************************************************************
 *                                                                        *
 *                  FTS API for Flashing the IC                           *
 *                                                                        *
 **************************************************************************
 **************************************************************************

 */

#include "ftsCrossCompile.h"
#include "ftsCompensation.h"
#include "ftsError.h"
#include "ftsFlash.h"
#include "ftsFrame.h"
#include "ftsIO.h"
#include "ftsSoftware.h"
#include "ftsTest.h"
#include "ftsTime.h"
#include "ftsTool.h"
#include "../fts.h"

#include <linux/init.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <linux/string.h>
#include <stdarg.h>
#include <linux/input.h>
#include <linux/interrupt.h>
#include <linux/serio.h>
#include <linux/time.h>
#include <linux/pm.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/power_supply.h>
#include <linux/firmware.h>
#include <linux/regulator/consumer.h>
#include <linux/of_gpio.h>
//#include <linux/sec_sysfs.h>



#define LOAD_FW_FROM 0

extern chipInfo ftsInfo;

int getFirmwareVersion(u16* fw_vers, u16* config_id)
{
	u8 fwvers[DCHIP_FW_VER_BYTE];
	u8 confid[CONFIG_ID_BYTE];
	int res;

	res = readCmdU16(FTS_CMD_HW_REG_R, DCHIP_FW_VER_ADDR, fwvers, DCHIP_FW_VER_BYTE, DUMMY_HW_REG);
	if (res < OK) {
		TS_LOG_ERR("%s getFirmwareVersion: unable to read fw_version ERROR %02X\n", __func__, ERROR_FW_VER_READ);
		return (res | ERROR_FW_VER_READ);
	}

	u8ToU16(fwvers, fw_vers); //fw version use big endian
	if (*fw_vers != 0) { // if fw_version is 00 00 means that there is no firmware running in the chip therefore will be impossible find the config_id
		res = readB2(CONFIG_ID_ADDR, confid, CONFIG_ID_BYTE);
		if (res < OK) {
			TS_LOG_ERR("%s getFirmwareVersion: unable to read config_id ERROR %02X\n", __func__, ERROR_FW_VER_READ);
			return (res | ERROR_FW_VER_READ);
		}
		u8ToU16(confid, config_id); //config id use little endian
	} else {
		*config_id = 0x0000;
	}

	TS_LOG_INFO("%s FW VERS = %04X\n", __func__, *fw_vers);
	TS_LOG_INFO("%s CONFIG ID = %04X\n", __func__, *config_id);
	return OK;
}

int getFWdata(const char* pathToFile, u8** data, int *size,int from)
{
	const struct firmware *fw = NULL;
	int res;
	struct fts_ts_info *info = fts_get_info();

	TS_LOG_INFO("%s getFWdata starting ...\n", __func__);
	switch(from){
	default:
		TS_LOG_INFO("%s Read FW from BIN file!\n", __func__);
		res = request_firmware(&fw, pathToFile, info->i2c_cmd_dev);
		if(res == 0){
			*size = fw->size;
			*data = (u8*) kmalloc((*size) * sizeof (u8), GFP_KERNEL);
			if(*data==NULL){
				TS_LOG_ERR("%s getFWdata: Impossible to allocate memory! ERROR %08X\n", __func__, ERROR_ALLOC);
				release_firmware(fw);
				return ERROR_ALLOC;
			}
			memcpy(*data, (u8 *) fw->data , (*size));
			release_firmware(fw);
		}else{
			TS_LOG_ERR("%s getFWdata: No File found! ERROR %08X\n", __func__, ERROR_FILE_NOT_FOUND);
			return ERROR_FILE_NOT_FOUND;
		}
	}

	TS_LOG_INFO("%s getFWdata Finshed!\n", __func__);
	return OK;
}

int readFwFile(const char* path, Firmware *fw, int keep_cx)
{
	int res;
	int orig_size;
	u8* orig_data = NULL;

	res = getFWdata(path, &orig_data, &orig_size, LOAD_FW_FROM);
	if (res < OK) {
		TS_LOG_ERR("%s readFwFile: impossible retrieve FW... ERROR %08X\n", __func__, ERROR_MEMH_READ);
		return (res | ERROR_MEMH_READ);
	}
	res = parseBinFile(orig_data, orig_size, fw, keep_cx);
	if (res < OK) {
		TS_LOG_ERR("%s readFwFile: impossible parse ERROR %08X\n", __func__, ERROR_MEMH_READ);
		return (res | ERROR_MEMH_READ);
	}

	return OK;

}

int flashProcedure(const char* path, int force, int keep_cx)
{
	Firmware fw;
	int res;

	fw.data = NULL;
	TS_LOG_INFO("%s Reading Fw file...\n", __func__);
	res = readFwFile(path, &fw, keep_cx);
	if (res < OK) {
		TS_LOG_ERR("%s flashProcedure: ERROR %02X\n", __func__, (res | ERROR_FLASH_PROCEDURE));
		kfree(fw.data);
		return (res | ERROR_FLASH_PROCEDURE);
	}
	TS_LOG_INFO("%s Fw file read COMPLETED!\n", __func__);

	TS_LOG_INFO("%s Starting flashing procedure...\n", __func__);
	res = flash_burn(fw, force, keep_cx);
	if (res < OK && res != (ERROR_FW_NO_UPDATE | ERROR_FLASH_BURN_FAILED)) {
		TS_LOG_ERR("%s flashProcedure: ERROR %02X\n", __func__, ERROR_FLASH_PROCEDURE);
	kfree(fw.data);
		return (res | ERROR_FLASH_PROCEDURE);
	}
	TS_LOG_INFO("%s flashing procedure Finished!\n", __func__);
	kfree(fw.data);

	return res;
}

int wait_for_flash_ready(u8 type)
{
	u8 cmd[2] = {FLASH_CMD_READ_REGISTER, type};
	u8 readData;
	int i, res = -1;

	TS_LOG_INFO("%s Waiting for flash ready ...\n", __func__);
	for (i = 0; i < FLASH_RETRY_COUNT && res != 0; i++) {
		if (fts_readCmd(cmd, sizeof (cmd), &readData, 1) < 0) {
			TS_LOG_ERR("%s wait_for_flash_ready: ERROR %02X\n", __func__, ERROR_I2C_R);
		} else{
			res = readData & 0x80;
		}
		mdelay(FLASH_WAIT_BEFORE_RETRY);
	}

	if (i == FLASH_RETRY_COUNT && res != 0) {
		TS_LOG_ERR("%s Wait for flash TIMEOUT! ERROR %02X\n", __func__, ERROR_TIMEOUT);
		return ERROR_TIMEOUT;
	}

	TS_LOG_INFO("%s Flash READY!\n", __func__);
	return OK;
}

int fts_warm_boot()
{
	u8 cmd[4] = {FTS_CMD_HW_REG_W, 0x00, 0x00, WARM_BOOT_VALUE}; //write the command to perform the warm boot
	u16ToU8_be(ADDR_WARM_BOOT, &cmd[1]);

	TS_LOG_INFO("%s Command warm boot ...\n", __func__);
	if (fts_writeCmd(cmd, sizeof (cmd)) < 0) {
		TS_LOG_ERR("%s flash_unlock: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}

	TS_LOG_INFO("%s Warm boot DONE!\n", __func__);

	return OK;
}

int parseBinFile(u8* data, int fw_size, Firmware *fwData, int keep_cx) {

	int dimension, index = 0;
	u32 temp;
	int res,i;

			//the file should contain at least the header plus the content_crc
	if (fw_size < FW_HEADER_SIZE+FW_BYTES_ALLIGN || data == NULL) {
		TS_LOG_ERR("%s parseBinFile: Read only %d instead of %d... ERROR %02X\n", __func__, fw_size, FW_HEADER_SIZE+FW_BYTES_ALLIGN, ERROR_FILE_PARSE);
		res = ERROR_FILE_PARSE;
		goto END;
}
	//start parsing of bytes
	u8ToU32(&data[index],&temp);
	if(temp!=FW_HEADER_SIGNATURE){
		TS_LOG_ERR("%s parseBinFile: Wrong Signature %08X ... ERROR %02X\n", __func__, temp, ERROR_FILE_PARSE);
		res = ERROR_FILE_PARSE;
		goto END;
	}
	TS_LOG_INFO("%s parseBinFile: Fw Signature OK!\n", __func__);
	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	if(temp!=FW_FTB_VER){
		TS_LOG_ERR("%s parseBinFile: Wrong ftb_version %08X ... ERROR %02X\n", __func__, temp, ERROR_FILE_PARSE);
		res = ERROR_FILE_PARSE;
		goto END;
	}
	TS_LOG_INFO("%s parseBinFile: ftb_version OK!\n", __func__);
	index+=FW_BYTES_ALLIGN;
	if(data[index]!=DCHIP_ID_0 || data[index+1]!=DCHIP_ID_1){
		TS_LOG_ERR("%s parseBinFile: Wrong target %02X != %02X  %02X != %02X ... ERROR %08X\n", __func__, data[index], DCHIP_ID_0, data[index+1], DCHIP_ID_1, ERROR_FILE_PARSE);
		res = ERROR_FILE_PARSE;
		goto END;
	}
	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	TS_LOG_INFO("%s parseBinFile: Fw ID = %08X\n", __func__, temp);

	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	fwData->fw_ver = temp;
	TS_LOG_INFO("%s parseBinFile: FILE Fw Version = %04X\n", __func__, fwData->fw_ver);

	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	fwData->config_id = temp;
	TS_LOG_INFO("%s parseBinFile: FILE Config ID = %04X\n", __func__, fwData->config_id);

	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	TS_LOG_INFO("%s parseBinFile: Config Version = %08X\n", __func__, temp);

	index+=FW_BYTES_ALLIGN*2;			//skip reserved data

	index+=FW_BYTES_ALLIGN;
	TS_LOG_INFO("%s parseBinFile: File External Release =  ", __func__);
	for(i=0;i<EXTERNAL_RELEASE_INFO_SIZE;i++){
		fwData->externalRelease[i] = data[index++];
		TS_LOG_INFO("%02X ", fwData->externalRelease[i]);
	}
	TS_LOG_INFO("\n");

	u8ToU32(&data[index],&temp);
	fwData->sec0_size = temp;
	TS_LOG_INFO("%s parseBinFile:  sec0_size = %08X (%d bytes)\n", __func__, fwData->sec0_size, fwData->sec0_size);

	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	fwData->sec1_size = temp;
	TS_LOG_INFO("%s parseBinFile:  sec1_size = %08X (%d bytes)\n", __func__, fwData->sec1_size, fwData->sec1_size);

	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	fwData->sec2_size = temp;
	TS_LOG_INFO("%s parseBinFile:  sec2_size = %08X (%d bytes)\n", __func__, fwData->sec2_size, fwData->sec2_size);

	index+=FW_BYTES_ALLIGN;
	u8ToU32(&data[index],&temp);
	fwData->sec3_size = temp;
	TS_LOG_INFO("%s parseBinFile:  sec3_size = %08X (%d bytes)\n", __func__, fwData->sec3_size, fwData->sec3_size);

	index+=FW_BYTES_ALLIGN;		//skip header crc

	if(!keep_cx){
		dimension = fwData->sec0_size + fwData->sec1_size + fwData->sec2_size + fwData->sec3_size;
		temp = fw_size;
	}else{
		dimension = fwData->sec0_size + fwData->sec1_size;					//sec2 may contain cx data (future implementation) sec3 atm not used
		temp = fw_size - fwData->sec2_size - fwData->sec3_size;
		fwData->sec2_size = 0;
		fwData->sec3_size = 0;
	}

	if(dimension+FW_HEADER_SIZE+FW_BYTES_ALLIGN != temp){
		TS_LOG_ERR("%s parseBinFile: Read only %d instead of %d... ERROR %02X\n", __func__, fw_size, dimension+FW_HEADER_SIZE+FW_BYTES_ALLIGN, ERROR_FILE_PARSE);
		res = ERROR_FILE_PARSE;
		goto END;
	}

	fwData->data = (u8*) kmalloc(dimension * sizeof (u8), GFP_KERNEL);
	if (fwData->data == NULL) {
		TS_LOG_ERR("%s parseBinFile: ERROR %02X\n", __func__, ERROR_ALLOC);
		res = ERROR_ALLOC;
		goto END;
	}

	index+=FW_BYTES_ALLIGN;
	memcpy(fwData->data, &data[index], dimension);
	fwData->data_size = dimension;

	TS_LOG_INFO("%s READ FW DONE %d bytes!\n", __func__, fwData->data_size);
	res = OK;

END:
	kfree(data);
	return res;
}

int flash_unlock()
{
	u8 cmd[3] = {FLASH_CMD_UNLOCK, FLASH_UNLOCK_CODE0, FLASH_UNLOCK_CODE1}; //write the command to perform the unlock

	TS_LOG_INFO("%s Command unlock ...\n", __func__);
	if (fts_writeCmd(cmd, sizeof (cmd)) < 0) {
		TS_LOG_ERR("%s flash_unlock: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}

	TS_LOG_INFO("%s Unlock flash DONE!\n", __func__);

	return OK;

}

int flash_erase_unlock()
{
	u8 cmd[3] = {FLASH_CMD_WRITE_REGISTER, FLASH_ERASE_UNLOCK_CODE0, FLASH_ERASE_UNLOCK_CODE1}; //write the command to perform the unlock for erasing the flash

	TS_LOG_INFO("%s:Try to erase unlock flash...\n", __func__);

	TS_LOG_INFO("%s:Command erase unlock ...\n", __func__);
	if (fts_writeCmd(cmd, sizeof (cmd)) < 0) {
		TS_LOG_ERR("%s flash_erase_unlock: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}

	TS_LOG_INFO("%s:Erase Unlock flash DONE!\n", __func__);

	return OK;

}

int flash_full_erase()
{
	int status;
	u8 cmd[3] = {FLASH_CMD_WRITE_REGISTER, FLASH_ERASE_CODE0, FLASH_ERASE_CODE1}; //write the command to erase the flash

	TS_LOG_INFO("%s Command full erase sent ...\n", __func__);
	if (fts_writeCmd(cmd, sizeof (cmd)) < 0) {
		TS_LOG_ERR("%s flash_full_erase: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}

	status = wait_for_flash_ready(FLASH_ERASE_CODE0);

	if (status != OK) {
		TS_LOG_ERR("%s flash_full_erase: ERROR %02X\n", __func__, ERROR_FLASH_NOT_READY);
		return (status | ERROR_FLASH_NOT_READY); //Flash not ready within the chosen time, better exit!
	}

	TS_LOG_INFO("%s Full Erase flash DONE!\n", __func__);

	return OK;

}

int flash_erase_page_by_page(int keep_cx)
{
	u8 status,i=0;
	u8 cmd[4] = {FLASH_CMD_WRITE_REGISTER, FLASH_ERASE_CODE0, 0x00, 0x00}; //write the command to erase the flash

	for(i=0; i<FLASH_NUM_PAGE; i++) {
		if(i>=FLASH_CX_PAGE_START && i<=FLASH_CX_PAGE_END && keep_cx == 1) {
			TS_LOG_INFO("%s Skipping erase page %d\n", __func__, i);
			continue;
		}

		cmd[2] = (0x3F&i)|FLASH_ERASE_START;
		TS_LOG_INFO("%s Command erase page %d sent...%02X %02X %02X %02X\n", __func__,i, cmd[0],cmd[1],cmd[2],cmd[3]);
		if (fts_writeCmd(cmd, sizeof (cmd)) < 0) {
			TS_LOG_ERR("%s flash_erase_page_by_page: ERROR %08X\n", __func__, ERROR_I2C_W);
			return ERROR_I2C_W;
		}

		status = wait_for_flash_ready(FLASH_ERASE_CODE0);
		if (status != OK) {
			TS_LOG_ERR("%s flash_erase_page_by_page: ERROR %08X\n", __func__, ERROR_FLASH_NOT_READY);
			return (status | ERROR_FLASH_NOT_READY); //Flash not ready within the chosen time, better exit!
		}
	}

	TS_LOG_INFO("%s Erase flash page by page DONE!\n", __func__);
	return OK;
}

int start_flash_dma(void)
{
	int status;
	u8 cmd[3] = {FLASH_CMD_WRITE_REGISTER, FLASH_DMA_CODE0, FLASH_DMA_CODE1}; //write the command to erase the flash

	TS_LOG_INFO("%s Command flash DMA\n", __func__);
	if (fts_writeCmd(cmd, sizeof (cmd)) < 0) {
		TS_LOG_ERR("%s start_flash_dma: ERROR %02X\n", __func__, ERROR_I2C_W);
		return ERROR_I2C_W;
	}

	status = wait_for_flash_ready(FLASH_DMA_CODE0);

	if (status != OK) {
		TS_LOG_ERR("%s start_flash_dma: ERROR %02X\n", __func__, ERROR_FLASH_NOT_READY);
		return (status | ERROR_FLASH_NOT_READY); //Flash not ready within the chosen time, better exit!
	}

	TS_LOG_INFO("%s flash DMA DONE!\n", __func__);

	return OK;
}

int fillFlash(u32 address, u8 *data, int size)
{
	int remaining = size;
	int toWrite = 0;
	int byteBlock = 0;
	int wheel = 0;
	u32 addr = 0;
	int res;
	int delta;
	u8* buff = NULL;
	u8 buff2[9] = {0};


	buff = (u8*) kmalloc((DMA_CHUNK + 3) * sizeof (u8), GFP_KERNEL);
	if (buff == NULL) {
		TS_LOG_ERR("%s fillFlash: ERROR %02X\n", __func__, ERROR_ALLOC);
		return ERROR_ALLOC;
	}

	while (remaining > 0) {
		byteBlock = 0;
		addr =0;
		while (byteBlock < FLASH_CHUNK && remaining > 0) {
			buff[0] = FLASH_CMD_WRITE_64K;
			if (remaining >= DMA_CHUNK) {
				if ((byteBlock + DMA_CHUNK) <= FLASH_CHUNK) {
					toWrite = DMA_CHUNK;
					remaining -= DMA_CHUNK;
					byteBlock += DMA_CHUNK;
				} else {
					delta = FLASH_CHUNK - byteBlock;
					toWrite = delta;
					remaining -= delta;
					byteBlock += delta;
				}
			} else {
				if ((byteBlock + remaining) <= FLASH_CHUNK) {
					toWrite = remaining;
					byteBlock += remaining;
					remaining = 0;
				} else {
					delta = FLASH_CHUNK - byteBlock;
					toWrite = delta;
					remaining -= delta;
					byteBlock += delta;
				}
			}
			buff[1] = (u8) ((addr & 0x0000FF00) >> 8);
			buff[2] = (u8) (addr & 0x000000FF);
			memcpy(&buff[3], data, toWrite);
			if (fts_writeCmd(buff, 3 + toWrite) < 0) {
				TS_LOG_ERR("%s fillFlash: ERROR %02X\n", __func__, ERROR_I2C_W);
				kfree(buff);
				return ERROR_I2C_W;
			}

			addr += toWrite;
			data += toWrite;
		}

		//configuring the DMA
		byteBlock = byteBlock / 4 - 1;

		buff2[0] = FLASH_CMD_WRITE_REGISTER;
		buff2[1] = FLASH_DMA_CONFIG;
		buff2[2] = 0x00;
		buff2[3] = 0x00;

		addr = address + ((wheel * FLASH_CHUNK)/4);
		buff2[4] = (u8) ((addr & 0x000000FF));
		buff2[5] = (u8) ((addr & 0x0000FF00) >> 8);
		buff2[6] = (u8) (byteBlock & 0x000000FF);
		buff2[7] = (u8) ((byteBlock & 0x0000FF00)>> 8);
		buff2[8] = 0x00;

		TS_LOG_INFO("%s Command = %02X , address = %02X %02X, words =  %02X %02X\n",
			__func__, buff2[0], buff2[5], buff2[4], buff2[7], buff2[6]);
		if (fts_writeCmd(buff2, 9) < OK) {
			TS_LOG_ERR("%s   Error during filling Flash! ERROR %02X\n", __func__, ERROR_I2C_W);
			kfree(buff);
			return (ERROR_I2C_W);
		}

		res = start_flash_dma();
		if (res < OK) {
			TS_LOG_ERR("%s   Error during flashing DMA! ERROR %02X\n", __func__, res);
			kfree(buff);
			return res;
		}
		wheel++;
	}
	kfree(buff);
	return OK;
}

static bool fts_check_fw_version(Firmware *fw)
{
	TS_LOG_INFO("%s:ic: fw_ver=%x conifg_id=%x fw image:fw_ver=%x config_id=%x\n",
		__func__, ftsInfo.u16_fwVer, ftsInfo.u16_cfgId, 
		fw->fw_ver, fw->config_id);

	if (ftsInfo.u16_fwVer != fw->fw_ver || ftsInfo.u16_cfgId != fw->config_id)
		return true;
	else
		return false;
}

int flash_burn(Firmware fw, int force_burn,int keep_cx)
{
	int res;

	/*check firmware version here*/
	if (force_burn || fts_check_fw_version(&fw)) {
		goto start;
	} else {
		TS_LOG_INFO("%s: do not need fw update\n", __func__);
		return (ERROR_FW_NO_UPDATE);
	}

	/* programming procedure start */
start:
	TS_LOG_INFO("%s Programming Procedure for flashing started:\n", __func__);

	TS_LOG_INFO("%s 1) SYSTEM RESET:\n", __func__);
	res = fts_system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s    system reset FAILED!\n", __func__);
		if (res != (ERROR_SYSTEM_RESET_FAIL | ERROR_TIMEOUT)) //if there is no firmware i will not get the controller ready event and there will be a timeout but i can keep going, but if there is an I2C error i have to exit
			return (res | ERROR_FLASH_BURN_FAILED);
	} else {
		TS_LOG_INFO("%s   system reset COMPLETED!\n", __func__);
	}

	TS_LOG_INFO("%s 2) WARM BOOT:\n", __func__);
	res = fts_warm_boot();
	if (res < OK) {
		TS_LOG_ERR("%s    warm boot FAILED!\n", __func__);
		return (res | ERROR_FLASH_BURN_FAILED);
	} else {
		TS_LOG_INFO("%s    warm boot COMPLETED!\n", __func__);
	}

	TS_LOG_INFO("%s 3) FLASH UNLOCK:\n", __func__);
	res = flash_unlock();
	if (res < OK) {
		TS_LOG_ERR("%s   flash unlock FAILED! ERROR %02X\n", __func__, ERROR_FLASH_BURN_FAILED);
		return (res | ERROR_FLASH_BURN_FAILED);
	} else {
		TS_LOG_INFO("%s   flash unlock COMPLETED!\n", __func__);
	}

	TS_LOG_INFO("%s 4) FLASH ERASE UNLOCK:\n", __func__);
	res = flash_erase_unlock();
	if (res < 0) {
		TS_LOG_ERR("%s   flash unlock FAILED! ERROR %02X\n", __func__, ERROR_FLASH_BURN_FAILED);
		return (res | ERROR_FLASH_BURN_FAILED);
	} else {
		TS_LOG_INFO("%s   flash unlock COMPLETED!\n", __func__);
	}


	TS_LOG_INFO("%s 5) FLASH ERASE:\n", __func__);
	if(keep_cx==1)
	res = flash_erase_page_by_page(keep_cx);
	else
		res = flash_full_erase();
	if (res < 0) {
		TS_LOG_ERR("%s   flash erase FAILED! ERROR %02X\n", __func__, ERROR_FLASH_BURN_FAILED);
		return (res | ERROR_FLASH_BURN_FAILED);
	} else {
		TS_LOG_INFO("%s   flash erase COMPLETED!\n", __func__);
	}

	TS_LOG_INFO("%s 6) LOAD PROGRAM:\n", __func__);
	res = fillFlash(FLASH_ADDR_CODE, &fw.data[0], fw.sec0_size);
	if (res < OK) {
		TS_LOG_ERR("%s   load program ERROR %02X\n", __func__, ERROR_FLASH_BURN_FAILED);
		return (res | ERROR_FLASH_BURN_FAILED);
	}
	TS_LOG_INFO("%s   load program DONE!\n", __func__);

	TS_LOG_INFO("%s 7) LOAD CONFIG:\n", __func__);
	res = fillFlash(FLASH_ADDR_CONFIG, &(fw.data[fw.sec0_size]), fw.sec1_size);
	if (res < OK) {
		TS_LOG_ERR("%s   load config ERROR %02X\n", __func__, ERROR_FLASH_BURN_FAILED);
		return (res | ERROR_FLASH_BURN_FAILED);
	}
	TS_LOG_INFO("%s   load config DONE!\n", __func__);

	TS_LOG_INFO("%s   Flash burn COMPLETED!\n", __func__);

	TS_LOG_INFO("%s 8) SYSTEM RESET:\n", __func__);
	res = fts_system_reset();
	if (res < 0) {
		TS_LOG_ERR("%s    system reset FAILED! ERROR %02X\n", __func__, ERROR_FLASH_BURN_FAILED);
		return (res | ERROR_FLASH_BURN_FAILED);
	}
	TS_LOG_INFO("%s   system reset COMPLETED!\n", __func__);

	TS_LOG_INFO("%s 9) FINAL CHECK:\n", __func__);
	res = readChipInfo(0);
	if (res < 0) {
		TS_LOG_ERR("%s flash_burn: Unable to retrieve Chip INFO! ERROR %02X\n", __func__, ERROR_FLASH_BURN_FAILED);
		return (res | ERROR_FLASH_BURN_FAILED);
	}

	for(res=0;res<EXTERNAL_RELEASE_INFO_SIZE;res++){
		if(fw.externalRelease[res]!=ftsInfo.u8_extReleaseInfo[res]){ //external release is prined during readChipInfo
			TS_LOG_ERR("%s  Firmware in the chip different from the one that was burn! fw: %x != %x , conf: %x != %x\n", __func__, ftsInfo.u16_fwVer, fw.fw_ver, ftsInfo.u16_cfgId, fw.config_id);
			return ERROR_FLASH_BURN_FAILED;
		}
	}

	TS_LOG_INFO("%s Final check OK! fw: %02X , conf: %02X\n", __func__, ftsInfo.u16_fwVer, ftsInfo.u16_cfgId);

	return OK;
}

