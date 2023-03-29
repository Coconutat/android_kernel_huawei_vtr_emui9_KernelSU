/*
 * gt1x_update.c - Firmware update
 *
 * 2010 - 2016 gt1x Technology.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be a reference
 * to you, when you are integrating the gt1x's CTP IC into your system,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 */
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/sched.h>
#include <linux/kthread.h>
#include <linux/wait.h>
#include <linux/time.h>
#include <linux/delay.h>
#include <linux/namei.h>
#include <linux/mount.h>
#include <asm/uaccess.h>
#include <linux/firmware.h>
#include <linux/slab.h>
#include "gt1x.h"

// hardware register define
#define _bRW_MISCTL__SRAM_BANK       0x4048
#define _bRW_MISCTL__MEM_CD_EN       0x4049
#define _bRW_MISCTL__CACHE_EN        0x404B
#define _bRW_MISCTL__TMR0_EN         0x40B0
#define _rRW_MISCTL__SWRST_B0_       0x4180
#define _bWO_MISCTL__CPU_SWRST_PULSE 0x4184
#define _rRW_MISCTL__BOOTCTL_B0_     0x4190
#define _rRW_MISCTL__BOOT_OPT_B0_    0x4218
#define _rRW_MISCTL__BOOT_CTL_       0x5094
#define _bRW_MISCTL__DSP_MCU_PWR_    0x4010
#define _bRW_MISCTL__PATCH_AREA_EN_  0x404D

#define PACK_SIZE	256
#define FW_HEAD_SIZE	128
#define FW_START_LOCAL	6
#define FW_INFO_LOCAL	32



#define  PID_RESERVE_BIT	4
#define  CHECKSUM_IN_HEADER	4
#define  PATCH_MAIN_VERSION	5
#define FW_HEAD_SUBSYSTEM_INFO_SIZE          8
#define FW_HEAD_OFFSET_SUBSYSTEM_INFO_BASE   32

#define FW_SECTION_TYPE_SS51_ISP               0x01
#define FW_SECTION_TYPE_SS51_PATCH             0x02
#define FW_SECTION_TYPE_SS51_PATCH_OVERLAY     0x03
#define FW_SECTION_TYPE_DSP                    0x04



#define UPDATE_STATUS_IDLE     0
#define UPDATE_STATUS_RUNNING  1
#define UPDATE_STATUS_ABORT    2

#define _ERROR(e)      ((0x01 << e) | (0x01 << (sizeof(s32) * 8 - 1)))
#define  ERROR          _ERROR(1)	//for common use
//system relevant
#define ERROR_IIC      _ERROR(2)	//IIC communication error.
#define ERROR_MEM      _ERROR(3)	//memory error.

//system irrelevant
#define ERROR_HN_VER   _ERROR(10)		//HotKnot version error.
#define ERROR_CHECK    _ERROR(11)		//Compare src and dst error.
#define ERROR_RETRY    _ERROR(12)		//Too many retries.
#define ERROR_PATH     _ERROR(13)		//Mount path error
#define ERROR_FW       _ERROR(14)
#define ERROR_FILE     _ERROR(15)
#define ERROR_VALUE    _ERROR(16)		//Illegal value of variables

struct fw_subsystem_info {
	int type;
	int length;
	u32 address;
	int offset;
};
#pragma pack(1)
struct fw_info {
	u32 length;
	u16 checksum;
	u8 target_mask[6];
	u8 target_mask_version[3];
	u8 pid[6];
	u8 version[3];
	u8 subsystem_count;
	u8 chip_type;
	u8 reserved[6];
	struct fw_subsystem_info subsystem[12];
};
#pragma pack()

struct fw_update_info update_info = {
	.status = UPDATE_STATUS_IDLE,
	.progress = 0,
	.max_progress = 9,
	.force_update = 0
};
static int gt1x_update_prepare(void);
static int gt1x_check_firmware(void);
static int gt1x_update_judge(void);
static int gt1x_run_ss51_isp(u8 * ss51_isp, int length);
static int gt1x_burn_subsystem(struct fw_subsystem_info *subsystem);
static u16 gt1x_calc_checksum(u8 * fw, u32 length);
static int gt1x_recall_check(u8 * chk_src, u16 start_rd_addr, u16 chk_length);
static void gt1x_update_cleanup(void);
static int gt1x_check_subsystem_in_flash(struct fw_subsystem_info *subsystem);
static int gt1x_read_flash(u32 addr, int length);
static int gt1x_error_erase(void);


int gt1x_update_firmware(void);

static int gt1x_hold_ss51_dsp(void);

void gt1x_leave_update_mode(void);


static int gt1x_i2c_write_with_readback(u16 addr, u8 * buffer, int length)
{
	u8 buf[100] = {0};
	int ret;
	if (buffer == NULL || length > sizeof(buf)) {
	   return  ERROR_CHECK;
	}
	ret = gt1x_i2c_write(addr, buffer, length);
	if (ret) {
		return ret;
	}
	ret = gt1x_i2c_read(addr, buf, length);
	if (ret) {
		return ret;
	}
	if (memcmp(buf, buffer, length)) {
		return ERROR_CHECK;
	}
	return 0;
}

u32 getUint(u8 * buffer, int len)
{
	u32 num = 0;
	int i;
	for (i = 0; i < len; i++) {
		num <<= 8;
		num += buffer[i];
	}
	return num;
}

// temp solution
static void gt1x_select_addr(void)
{
	int reset_gpio = gt1x_ts->dev_data->ts_platform_data->reset_gpio;
	/* reset */
	gpio_direction_output(reset_gpio, 0);
	udelay(150);
	gpio_direction_output(reset_gpio, 1);
	msleep(80);
}


int gt1x_update_firmware(void)
{
	int i = 0;
	int ret = 0;
	u8 *subsystem_info=NULL;
	TS_LOG_INFO("%s:Start auto update thread...\n", __func__);
	if (update_info.status != UPDATE_STATUS_IDLE) {
		TS_LOG_ERR("%s:Update process is running!\n", __func__);
		return ERROR;
	}
	update_info.status = UPDATE_STATUS_RUNNING;
	update_info.progress = 0;

	ret = gt1x_update_prepare();
	if(!ret){
		TS_LOG_INFO("%s : update preapre ok\n", __func__);
	}else if(ret == ERROR_FW) {
		TS_LOG_ERR("%s:fw image not exit, not need to update\n", __func__);
		update_info.status = UPDATE_STATUS_IDLE;
		return 0;
	} else {
		TS_LOG_ERR("%s:update_prepare fail\n", __func__);
		update_info.status = UPDATE_STATUS_IDLE;
		return ret;
	}

	ret = gt1x_check_firmware();
	if (ret) {
		update_info.status = UPDATE_STATUS_ABORT;
		goto update_exit;
	}
#if GTP_FW_UPDATE_VERIFY
	update_info.max_progress = 
		6 + update_info.firmware_info->subsystem_count;
#else
	update_info.max_progress = 
		3 + update_info.firmware_info->subsystem_count;
#endif
	update_info.progress++; // 1

	ret = gt1x_update_judge();
	if (ret) {
		update_info.status = UPDATE_STATUS_ABORT;
		goto  update_exit;
	}
	update_info.progress++; // 2

	subsystem_info = (u8 *)(&update_info.fw->data[update_info.firmware_info->subsystem[0].offset]);	
	if( subsystem_info==NULL ) {
		TS_LOG_ERR("%s:get isp fail\n", __func__);
		ret = ERROR_FW;
		update_info.status = UPDATE_STATUS_ABORT;
		goto  update_exit;
	}
	update_info.progress++; 			// 3

	ret = gt1x_run_ss51_isp( subsystem_info , update_info.firmware_info->subsystem[0].length);
	if (ret) {
		TS_LOG_ERR("%s:run isp fail\n", __func__);
		goto  update_exit;
	}
	update_info.progress++; 			// 4
	msleep(100);

	for (i = 1; i < update_info.firmware_info->subsystem_count; i++) {
		TS_LOG_INFO("subsystem: %d\n", update_info.firmware_info->subsystem[i].type);
		TS_LOG_INFO("Length: %d\n", update_info.firmware_info->subsystem[i].length);
		TS_LOG_INFO("Address: 0x%x\n", update_info.firmware_info->subsystem[i].address);

		ret = gt1x_burn_subsystem(&(update_info.firmware_info->subsystem[i]));
		if (ret) {
			TS_LOG_ERR("%s:burn subsystem fail!\n", __func__);
			goto  update_exit;
		}
		update_info.progress++;
	}

#if GTP_FW_UPDATE_VERIFY
	ret = gt1x_chip_reset();
	if(ret < 0){
		TS_LOG_ERR("%s, failed to chip_reset, ret = %d\n", __func__, ret);
	}

	subsystem_info =&update_info.fw->data[update_info.firmware_info->subsystem[0].offset];
	if(subsystem_info==NULL){
		TS_LOG_ERR("%s:get isp fail\n", __func__);
		ret = ERROR_FW;
		goto  update_exit;
	}
	update_info.progress++;

	ret = gt1x_run_ss51_isp(subsystem_info, update_info.firmware_info->subsystem[0].length);
	if (ret) {
		TS_LOG_ERR("%s:run isp fail\n", __func__);
		goto  update_exit;
	}
	update_info.progress++;

	TS_LOG_INFO("Reset guitar & check firmware in flash.\n");
	for (i = 1; i < update_info.firmware_info->subsystem_count; i++) {
		TS_LOG_INFO("subsystem: %d\n", update_info.firmware_info->subsystem[i].type);
		TS_LOG_INFO("Length: %d\n", update_info.firmware_info->subsystem[i].length);
		TS_LOG_INFO("Address: %d\n", update_info.firmware_info->subsystem[i].address);

		ret = gt1x_check_subsystem_in_flash(&(update_info.firmware_info->subsystem[i]));
		if (ret) {
			gt1x_error_erase();
			break;
		}
	}
	update_info.progress++;
#endif

update_exit:
	gt1x_update_cleanup();
	gt1x_leave_update_mode();
	gt1x_read_version(&gt1x_ts->hw_info);
	if (ret) {
		update_info.progress = 2 * update_info.max_progress;
		gt1x_ts->fw_update_ok = false;
		if (update_info.status == UPDATE_STATUS_ABORT) {
			TS_LOG_ERR("%s:No need to update firmware!\n", __func__);
			ret = NO_ERR;
		}else{
			TS_LOG_ERR("%s:Update firmware failed!\n", __func__);
		}
		goto exit;
	}
	gt1x_ts->fw_update_ok = true;

	TS_LOG_INFO("%s:Update firmware succeefully!\n", __func__);
exit:
	update_info.status = UPDATE_STATUS_IDLE;
	return ret;
}

static int gt1x_update_prepare(void)
{
	int ret = 0;
	int retry = GT1X_RETRY_NUM;
	update_info.fw = NULL;

	ret = request_firmware(&update_info.fw, gt1x_ts->firmware_name,
				&gt1x_ts->pdev->dev);
	if (ret < 0) {
		TS_LOG_ERR("%s:Request firmware failed - %s (%d)\n",
				__func__, gt1x_ts->firmware_name, ret);
		return ERROR_FW;
	}
	TS_LOG_INFO("Firmware size: %zd\n", update_info.fw->size);

	for (retry = 0; retry < GT1X_RETRY_NUM; retry++) {
		update_info.firmware_info = (struct fw_info *)kzalloc(sizeof(struct fw_info), GFP_KERNEL);
		if (update_info.firmware_info == NULL) {
			TS_LOG_ERR("%s:Alloc %zu bytes memory fail\n", __func__, sizeof(struct fw_info));
			continue;
		} else {
			break;
		}
	}
	if (retry >= GT1X_RETRY_NUM) {
		ret = ERROR_RETRY;
		goto no_memory_err;
	}

	for (retry = 0; retry < GT1X_RETRY_NUM; retry++) {
		update_info.buffer = (u8 *) kzalloc(1024 * 4, GFP_KERNEL);/* every burn 4k*/
		if (update_info.buffer == NULL) {
			TS_LOG_ERR("%s: Alloc %d bytes memory fail\n", __func__, 1024 * 4);
			continue;
		} else {
			break;
		}
	}
	if (retry < GT1X_RETRY_NUM)
		return NO_ERR;

	ret = ERROR_RETRY;
	if (update_info.firmware_info != NULL){
		kfree(update_info.firmware_info);
		update_info.firmware_info = NULL;
	}

no_memory_err:
	if (update_info.fw != NULL) {
		release_firmware(update_info.fw);
		update_info.fw = NULL;
	}
	return ret;
}

static void gt1x_update_cleanup(void)
{
	if (update_info.fw != NULL) {
		release_firmware(update_info.fw);
		update_info.fw = NULL;
	}
	if (update_info.buffer != NULL) {
		kfree(update_info.buffer);
		update_info.buffer = NULL;
	}
	if (update_info.firmware_info != NULL) {
		kfree(update_info.firmware_info);
		update_info.firmware_info = NULL;
	}
}

static int gt1x_check_firmware(void)
{
	u16 checksum;
	u16 checksum_in_header;
	u8 *subsystem_info = NULL;
	struct fw_info *firmware_tmpinfo = NULL;
	int i;
	int offset;

	// compare file length with the length field in the firmware header
	if (update_info.fw->size < FW_HEAD_SIZE) {
		TS_LOG_ERR("%s:Bad firmware!(file_size: %d)\n", __func__, update_info.fw->size);
		return ERROR_CHECK;
	}

	if (update_info.fw->data== NULL) {
		TS_LOG_ERR("%s: read fw data fail\n", __func__);
		return ERROR_FW;
	}

	if (getU32(update_info.fw->data) + FW_START_LOCAL != update_info.fw->size) {
		TS_LOG_ERR("%s:Bad firmware!(file length: %d, header define: %d)\n", 
				__func__, update_info.fw->size, getU32(update_info.fw->data));
		return ERROR_CHECK;
	}
	// check firmware's checksum
	checksum_in_header = getU16(&update_info.fw->data[ CHECKSUM_IN_HEADER ]);
	checksum = 0;
	for (i = FW_START_LOCAL; i < update_info.fw->size; i++) 
		checksum += update_info.fw->data[i];
	if (checksum != checksum_in_header) {
		TS_LOG_ERR("%s:Bad firmware!(checksum: 0x%04X, header define: 0x%04X)\n", 
				__func__, checksum, checksum_in_header);
		return ERROR_CHECK;
	}
	// parse firmware
	memcpy((u8 *) update_info.firmware_info, &update_info.fw->data[0],  FW_INFO_LOCAL);
	update_info.firmware_info->pid[PATCH_MAIN_VERSION] = 0;

	subsystem_info =(u8 *)(&update_info.fw->data[FW_HEAD_OFFSET_SUBSYSTEM_INFO_BASE]);
	firmware_tmpinfo = update_info.firmware_info;
	offset = FW_HEAD_SIZE;
	for (i = 0; i < firmware_tmpinfo->subsystem_count; i++) {
		firmware_tmpinfo->subsystem[i].type = 
				subsystem_info[i * FW_HEAD_SUBSYSTEM_INFO_SIZE];
		firmware_tmpinfo->subsystem[i].length = 
				getU16(&subsystem_info[i * FW_HEAD_SUBSYSTEM_INFO_SIZE + 1]);
		firmware_tmpinfo->subsystem[i].address = 
				getU16(&subsystem_info[i * FW_HEAD_SUBSYSTEM_INFO_SIZE + 3]) * 256;
		firmware_tmpinfo->subsystem[i].offset = offset;
		offset += firmware_tmpinfo->subsystem[i].length;
	}

	// print update information
	TS_LOG_INFO("Firmware length: %d\n", update_info.fw->size);
	TS_LOG_INFO("Firmware product: GT%s\n", update_info.firmware_info->pid);
	TS_LOG_INFO("Firmware patch: %02X%02X%02X\n", update_info.firmware_info->version[0], update_info.firmware_info->version[1], update_info.firmware_info->version[2]);
	TS_LOG_INFO("Firmware chip: 0x%02X\n", update_info.firmware_info->chip_type);
	TS_LOG_INFO("Subsystem count: %d\n", update_info.firmware_info->subsystem_count);
	for (i = 0; i < update_info.firmware_info->subsystem_count; i++) {
		TS_LOG_DEBUG("------------------------------------------\n");
		TS_LOG_DEBUG("Subsystem: %d\n", i);
		TS_LOG_DEBUG("Type: %d\n", update_info.firmware_info->subsystem[i].type);
		TS_LOG_DEBUG("Length: %d\n", update_info.firmware_info->subsystem[i].length);
		TS_LOG_DEBUG("Address: 0x%08X\n", update_info.firmware_info->subsystem[i].address);
		TS_LOG_DEBUG("Offset: %d\n", update_info.firmware_info->subsystem[i].offset);
	}
	return NO_ERR;
}

/**
 * @return: return a pointer pointed at the content of firmware
 *          if success, otherwise return NULL.
 */
static int gt1x_update_judge(void)
{
	int ret =0;
	u8 reg_val[2] = {0};
	u8 retry = GT1X_RETRY_NUM;
	struct gt1x_hw_info ver_info;
	struct gt1x_hw_info fw_ver_info;

	fw_ver_info.mask_id = (update_info.firmware_info->target_mask_version[0] << 16)
		| (update_info.firmware_info->target_mask_version[1] << 8)
		| (update_info.firmware_info->target_mask_version[2]);
	fw_ver_info.patch_id = (update_info.firmware_info->version[0] << 16)
		| (update_info.firmware_info->version[1] << 8)
		| (update_info.firmware_info->version[2]);
	memcpy(fw_ver_info.product_id, update_info.firmware_info->pid, GT1X_PRODUCT_ID_LEN);
	fw_ver_info.product_id[ PID_RESERVE_BIT ] = 0;

	/* check fw status reg */
	do {
		ret = gt1x_i2c_read_dbl_check(GTP_REG_FW_CHK_MAINSYS, reg_val, 1);
		if (ret < 0) {	/* read reg failed */
			goto _reset;
		} else if (ret > 0) {
			continue;
		}

		ret = gt1x_i2c_read_dbl_check(GTP_REG_FW_CHK_SUBSYS, &reg_val[1], 1);
		if (ret < 0) {
			goto _reset;
		} else if (ret > 0) {
			continue;
		}
		break;
_reset:
		ret = gt1x_chip_reset();
		if(ret < 0){
			TS_LOG_ERR("%s, failed to chip_reset, ret = %d\n", __func__, ret);
		}
	} while (--retry);

	if (retry <= 0) {
		TS_LOG_INFO("%s:Update abort due to i2c error.\n", __func__);
		return ERROR_CHECK;
	}

	if (reg_val[0] != FW_IS_OK || reg_val[1] == FW_IS_RUNING) {
		TS_LOG_INFO("Check fw status reg not pass,reg[0x814E]=0x%2X,reg[0x5095]=0x%2X!\n",
			reg_val[0], reg_val[1]);
		return NO_ERR;
	}

	if (update_info.force_update || gt1x_ts->fw_damage_flag) {
		TS_LOG_INFO("%s:Force update firmware\n", __func__);
		return NO_ERR;
	}

	ret = gt1x_read_version(&ver_info);
	if (ret < 0) {
		TS_LOG_INFO("%s:Get IC's version info failed, force update!\n", __func__);
		return NO_ERR;
	}
	if (memcmp(fw_ver_info.product_id, ver_info.product_id, GT1X_PRODUCT_ID_LEN)) {
		TS_LOG_INFO("%s:Product id is not match!\n", __func__);
		return ERROR_CHECK;
	}
	if ((fw_ver_info.mask_id & 0xFFFFFF00) != (ver_info.mask_id & 0xFFFFFF00)) {
		TS_LOG_INFO("%s:Mask id is not match!\n", __func__);
		return ERROR_CHECK;
	}
	if ((fw_ver_info.patch_id & 0xFF0000) != (ver_info.patch_id & 0xFF0000)){
		TS_LOG_INFO("%s:CID is not equal, need update!\n", __func__);
		return NO_ERR;
	}

	if ((fw_ver_info.patch_id & 0xFFFF) == (ver_info.patch_id & 0xFFFF)) {
		TS_LOG_INFO("%s:The version of the fw is same with IC's!\n", __func__);
		return ERROR_CHECK;
	}
	return NO_ERR;
}

static int __gt1x_hold_ss51_dsp_20(void)
{
	int ret = -1;
	int retry = 0;
	u8 buf[1];
	int hold_times = 0;

	while (retry++ < 30) {
		// Hold ss51 & dsp
		buf[0] = 0x0C;
		ret = gt1x_i2c_write(_rRW_MISCTL__SWRST_B0_, buf, 1);
		if (ret) {
			TS_LOG_ERR("Hold ss51 & dsp I2C error,retry:%d\n", retry);
			continue;
		}
		// Confirm hold
		buf[0] = 0x00;
		ret = gt1x_i2c_read(_rRW_MISCTL__SWRST_B0_, buf, 1);
		if (ret) {
			TS_LOG_ERR("Hold ss51 & dsp I2C error,retry:%d\n", retry);
			continue;
		}
		if (0x0C == buf[0]) {
			if (hold_times++ < 20) {
				continue;
			} else {
				break;
			}
		}
		TS_LOG_ERR("Hold ss51 & dsp confirm 0x4180 failed,value:%d\n", buf[0]);
	}
	if (retry >= 30) {
		TS_LOG_ERR("Hold ss51&dsp failed!\n");
		return ERROR_RETRY;
	}

	TS_LOG_INFO("Hold ss51&dsp successfully.\n");
	return NO_ERR;
}

static int gt1x_hold_ss51_dsp(void)
{
	int ret = ERROR, retry = GT1X_RETRY_FIVE;
	u8 buffer[2] = {0};

	do {
		ret = gt1x_chip_reset();
		if(ret){
			TS_LOG_ERR("%s: chip_reset fail\n", __func__);
		}
		ret = gt1x_i2c_read(GTP_REG_HW_INFO, buffer, 1);
	} while (retry-- && ret < 0);

	if (ret < 0)
		return ERROR;

	//hold ss51_dsp
	ret = __gt1x_hold_ss51_dsp_20();
	if (ret) {
		return ret;
	}
	// enable dsp & mcu power
	buffer[0] = 0x00;
	ret = gt1x_i2c_write_with_readback(_bRW_MISCTL__DSP_MCU_PWR_, buffer, 1);
	if (ret) {
		TS_LOG_ERR("enabel dsp & mcu power fail!\n");
		return ret;
	}
	// disable watchdog
	buffer[0] = 0x00;
	ret = gt1x_i2c_write_with_readback(_bRW_MISCTL__TMR0_EN, buffer, 1);
	if (ret) {
		TS_LOG_ERR("disable wdt fail!\n");
		return ret;
	}
	// clear cache
	buffer[0] = 0x00;
	ret = gt1x_i2c_write_with_readback(_bRW_MISCTL__CACHE_EN, buffer, 1);
	if (ret) {
		TS_LOG_ERR("clear cache fail!\n");
		return ret;
	}
	// soft reset
	buffer[0] = 0x01;
	ret = gt1x_i2c_write(_bWO_MISCTL__CPU_SWRST_PULSE, buffer, 1);
	if (ret) {
		TS_LOG_ERR("software reset fail!\n");
		return ret;
	}
	// set scramble
	buffer[0] = 0x00;
	ret = gt1x_i2c_write_with_readback(_rRW_MISCTL__BOOT_OPT_B0_, buffer, 1);
	if (ret) {
		TS_LOG_ERR("set scramble fail!\n");
		return ret;
	}

	return NO_ERR;
}

static int gt1x_run_ss51_isp(u8 * ss51_isp, int length)
{
	int ret;
	u8 buffer[10];

	ret = gt1x_hold_ss51_dsp();
	if (ret) {
		return ret;
	}
	// select bank4
	buffer[0] = 0x04;
	ret = gt1x_i2c_write_with_readback(_bRW_MISCTL__SRAM_BANK, buffer, 1);
	if (ret) {
		TS_LOG_ERR("select bank4 fail.\n");
		return ret;
	}
	// enable patch area access
	buffer[0] = 0x01;
	ret = gt1x_i2c_write_with_readback(_bRW_MISCTL__PATCH_AREA_EN_, buffer, 1);
	if (ret) {
		TS_LOG_ERR("enable patch area access fail!\n");
		return ret;
	}

	TS_LOG_INFO("ss51_isp length: %d, checksum: 0x%04X\n", length, gt1x_calc_checksum(ss51_isp, length));
	// load ss51 isp
	ret = gt1x_i2c_write(0xC000, ss51_isp, length);
	if (ret) {
		TS_LOG_ERR("load ss51 isp fail!\n");
		return ret;
	}
	// recall compare
	ret = gt1x_recall_check(ss51_isp, 0xC000, length);
	if (ret) {
		TS_LOG_ERR("recall check ss51 isp fail!\n");
		return ret;
	}
	//clear version
	memset(buffer, 0xAA, 10);
	ret = gt1x_i2c_write_with_readback(GTP_REG_VERSION, buffer, 10);
	if (ret) {
		TS_LOG_ERR("write readback fail!\n");

	}
	// disable patch area access
	buffer[0] = 0x00;
	ret = gt1x_i2c_write_with_readback(_bRW_MISCTL__PATCH_AREA_EN_, buffer, 1);
	if (ret) {
		TS_LOG_ERR("disable patch area access fail!\n");
		return ret;
	}
	// set 0x8006
	memset(buffer, 0x55, 8);
	ret = gt1x_i2c_write_with_readback(0x8006, buffer, 8);
	if (ret) {
		TS_LOG_ERR("set 0x8006[0~7] 0x55 fail!\n");
		return ret;
	}
	// release ss51
	buffer[0] = 0x08;
	ret = gt1x_i2c_write_with_readback(_rRW_MISCTL__SWRST_B0_, buffer, 1);
	if (ret) {
		TS_LOG_ERR("release ss51 fail!\n");
		return ret;
	}

	msleep(100);
	// check run state
	ret = gt1x_i2c_read(0x8006, buffer, 2);
	if (ret) {
		TS_LOG_ERR("read 0x8006 fail!");
		return ret;
	}
	if (!(buffer[0] == 0xAA && buffer[1] == 0xBB)) {
		TS_LOG_ERR("ERROR: isp is not running! 0x8006: %02X %02X\n", buffer[0], buffer[1]);
		return ERROR_CHECK;
	}

	return NO_ERR;
}

static u16 gt1x_calc_checksum(u8 * fw, u32 length)
{
	u32 i = 0;
	u32 checksum = 0;

	for (i = 0; i < length; i += 2) {
		checksum += (((int)fw[i]) << 8);
		checksum += fw[i + 1];
	}
	return (checksum & 0xFFFF);
}

static int gt1x_recall_check(u8 * chk_src, u16 start_addr, u16 chk_length)
{
	u8 rd_buf[PACK_SIZE] = {0};
	s32 ret = 0;
	u16 len = 0;
	u32 compared_length = 0;

	while (chk_length > 0) {
		len = (chk_length > PACK_SIZE ? PACK_SIZE : chk_length);

		ret = gt1x_i2c_read(start_addr + compared_length, rd_buf, len);
		if (ret) {
			TS_LOG_ERR("recall i2c error,exit!\n");
			return ret;
		}

		if (memcmp(rd_buf, &chk_src[compared_length], len)) {
			TS_LOG_ERR("Recall frame not equal(addr: 0x%04X)\n",
					start_addr + compared_length);
			return ERROR_CHECK;
		}

		chk_length -= len;
		compared_length += len;
	}

	TS_LOG_DEBUG("Recall check %d bytes(address: 0x%04X) success.\n", compared_length, start_addr);
	return NO_ERR;
}
#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0
#define MTK_IIC_MAX_TRANSFER_SIZE 8
static int gt1x_write_block_data(u32 addr, u8 * buffer, int len)
{
	int ret =0;
	int package_count = 0;
	int i;
	u32 start_write_addr = addr;
	u32 data_size = 0;
	u32 copy_offset = 0;

	/* computer package count*/
	package_count = len / MTK_IIC_MAX_TRANSFER_SIZE;
	if (len % MTK_IIC_MAX_TRANSFER_SIZE != 0)
		package_count += 1;

	for(i =0 ; i < package_count ; i++){
		/* the last package to write size*/
		if ((i + 1 == package_count) && (len % MTK_IIC_MAX_TRANSFER_SIZE != 0)) {
			data_size = len % MTK_IIC_MAX_TRANSFER_SIZE;
		} else {
			data_size = MTK_IIC_MAX_TRANSFER_SIZE;
		}

		copy_offset = start_write_addr - addr;
		ret = gt1x_i2c_write(start_write_addr, &buffer[copy_offset], data_size);
		if (ret) {
			TS_LOG_ERR("%s : write fw data fail !\n", __func__);
			goto out;
		}

		start_write_addr += data_size;	
	}
	return NO_ERR;
out :
	return ret;
}
#endif
static int gt1x_burn_subsystem(struct fw_subsystem_info *subsystem)
{
	int block_len;
	u16 checksum;
	int burn_len = 0;
	u16 cur_addr;
	u32 length = subsystem->length;
	u8 buffer[10];
	int ret;
	int wait_time;
	int burn_state;
	int retry = 5;
	u8 *fw = NULL;

	TS_LOG_INFO("Subsystem: %d\n", subsystem->type);
	TS_LOG_INFO("Length: %d\n", subsystem->length);
	TS_LOG_INFO("Address: 0x%08X\n", subsystem->address);

	while (length > 0 && retry > 0) {
		retry--;
		// if block_len >4K   block_len =4k;
		block_len = length > 1024 * 4 ? 1024 * 4 : length;

		TS_LOG_INFO("Burn block ==> length: %d, address: 0x%08X\n", block_len, subsystem->address + burn_len);
		fw =(u8 *)(&update_info.fw->data[subsystem->offset + burn_len]);

		cur_addr = ((subsystem->address + burn_len) >> 8);

		checksum = 0;
		checksum += block_len;
		checksum += cur_addr;
		checksum += gt1x_calc_checksum(fw, block_len);
		checksum = (0 - checksum);

		buffer[0] = ((block_len >> 8) & 0xFF);
		buffer[1] = (block_len & 0xFF);
		buffer[2] = ((cur_addr >> 8) & 0xFF);
		buffer[3] = (cur_addr & 0xFF);
		//write  block_len and cur addr
		ret = gt1x_i2c_write_with_readback(0x8100, buffer, 4);
		if (ret) {
			TS_LOG_ERR("write length & address fail!\n");
			continue;
		}
		//write block data
#ifdef CONFIG_HUAWEI_DEVKIT_MTK_3_0

		ret = gt1x_write_block_data(0x8100 + 4, fw, block_len);
		if (ret) {
			TS_LOG_ERR("write fw data fail!\n");
			continue;
		}
#else
		ret = gt1x_i2c_write(0x8100 + 4, fw, block_len);
		if (ret) {
			TS_LOG_ERR("write fw data fail!\n");
			continue;
		}
#endif
		//write block checksum
		buffer[0] = ((checksum >> 8) & 0xFF);
		buffer[1] = (checksum & 0xFF);
		ret = gt1x_i2c_write_with_readback(0x8100 + 4 + block_len, buffer, 2);
		if (ret) {
			TS_LOG_ERR("write checksum fail!\n");
			continue;
		}
		//clear clear control flag
		buffer[0] = 0;
		ret = gt1x_i2c_write_with_readback(0x8022, buffer, 1);
		if (ret) {
			TS_LOG_ERR("clear control flag fail!\n");
			continue;
		}
		//write subsystem type
		buffer[0] = subsystem->type;
		buffer[1] = subsystem->type;
		ret = gt1x_i2c_write_with_readback(0x8020, buffer, 2);
		if (ret) {
			TS_LOG_ERR("write subsystem type fail!\n");
			continue;
		}
		burn_state = ERROR;
		wait_time = 200;
		msleep(5);

		while (wait_time-- > 0) {
			u8 confirm = 0x55;
			 //read burn state from 0x8022  for two times
			ret = gt1x_i2c_read(0x8022, buffer, 1);
			if (ret < 0) {
				continue;
			}
			msleep(5);
			ret = gt1x_i2c_read(0x8022, &confirm, 1);
			if (ret < 0) {
				continue;
			}
			if (buffer[0] != confirm) {
				continue;
			}
			//check burn staus
			if (buffer[0] == 0xAA) {
				TS_LOG_DEBUG("burning.....\n");
				continue;
			} else if (buffer[0] == 0xDD) {
				TS_LOG_ERR("checksum error!\n");
				break;
			} else if (buffer[0] == 0xBB) {
				TS_LOG_INFO("burning success.\n");
				burn_state = 0;
				break;
			} else if (buffer[0] == 0xCC) {
				TS_LOG_ERR("burning failed!");
				break;
			} else {
				TS_LOG_DEBUG("unknown state!(0x8022: 0x%02X)\n", buffer[0]);
			}
		}

		if (!burn_state) {
			length -= block_len;
			burn_len += block_len;
			retry = 5;
		}
	}
	if (length == 0) {
		return NO_ERR;
	} else {
		return ERROR_RETRY;
	}
}

static int gt1x_check_subsystem_in_flash(struct fw_subsystem_info *subsystem)
{
	int block_len;
	int checked_len = 0;
	u32 length = subsystem->length;
	int ret;
	int check_state = 0;
	u8 *fw;

	TS_LOG_INFO("Subsystem: %d, Length: %d, Address: 0x%08X\n", 
			subsystem->type, subsystem->length, subsystem->address);

	while (length > 0) {
		block_len = length > 1024 * 4 ? 1024 * 4 : length;

		TS_LOG_INFO("Check block ==> length: %d, address: 0x%08X\n", block_len, subsystem->address + checked_len);
		fw =(u8 *)(&update_info.fw->data[subsystem->offset + checked_len]);

		ret = gt1x_read_flash(subsystem->address + checked_len, block_len);
		if (ret) {
			check_state |= (u32)ret;
		}

		ret = gt1x_recall_check(fw, 0x8100, block_len);
		if (ret) {
			TS_LOG_ERR("Block in flash is broken!\n");
			check_state |= (u32)ret;
		}

		length -= block_len;
		checked_len += block_len;

	}
	if (check_state) {
		TS_LOG_ERR("Subsystem in flash is broken!\n");
	} else {
		TS_LOG_INFO("Subsystem in flash is correct!\n");
	}
	return check_state;
}

static int gt1x_read_flash(u32 addr, int length)
{
	int wait_time;
	int ret = 0;
	u8 buffer[4];
	u16 read_addr = (addr >> 8);

	TS_LOG_INFO("Read flash: 0x%04X, length: %d\n", addr, length);

	buffer[0] = 0;
	ret = gt1x_i2c_write_with_readback(0x8022, buffer, 1);

	buffer[0] = ((length >> 8) & 0xFF);
	buffer[1] = (length & 0xFF);
	buffer[2] = ((read_addr >> 8) & 0xFF);
	buffer[3] = (read_addr & 0xFF);
	ret |= gt1x_i2c_write_with_readback(0x8100, buffer, 4);

	buffer[0] = 0xAA;
	buffer[1] = 0xAA;
	ret |= gt1x_i2c_write(0x8020, buffer, 2);
	if (ret) {
		TS_LOG_ERR("Error occured.\n");	//comment
		return ret;
	}

	wait_time = 200;
	while (wait_time > 0) {
		wait_time--;
		msleep(5);
		ret = gt1x_i2c_read_dbl_check(0x8022, buffer, 1);
		if (ret) {
			continue;
		}
		if (buffer[0] == 0xBB) {
			TS_LOG_INFO("Read success(addr: 0x%04X, length: %d)\n", addr, length);
			break;
		}
	}
	if (wait_time == 0) {
		TS_LOG_ERR("Read Flash FAIL!\n");
		return ERROR_RETRY;
	}
	return NO_ERR;
}

static int gt1x_error_erase(void)
{
	int block_len = 0;
	u16 checksum = 0;
	u16 erase_addr = 0;
	u8 buffer[10] = {0};
	int ret = 0;
	int wait_time = 0;
	int burn_state = ERROR;
	int retry = 5;
	u8 *fw = NULL;

	TS_LOG_INFO("Erase flash area of ss51.\n");
	ret = gt1x_chip_reset();
	if(ret < 0){
		TS_LOG_ERR("%s, failed to chip_reset, ret = %d\n", __func__, ret);
	}
	fw = (u8 *)(&update_info.fw->data[update_info.firmware_info->subsystem[0].offset]);
	if (fw == NULL) {
		TS_LOG_ERR("get isp fail\n");
		return ERROR_FW;
	}
	ret = gt1x_run_ss51_isp(fw, update_info.firmware_info->subsystem[0].length);
	if (ret) {
		TS_LOG_ERR("run isp fail\n");
		return ERROR_PATH;
	}

	fw = kzalloc(1024 * 4, GFP_KERNEL);
	if (!fw) {
		TS_LOG_ERR("error when alloc mem.\n");
		return ERROR_MEM;
	}
	//set flash data 0XFF
	memset(fw, 0xFF, 1024 * 4);
	erase_addr = 0x00;
	block_len = 1024 * 4;

	while (retry-- > 0) {
		checksum = 0;
		checksum += block_len;
		checksum += erase_addr;
		checksum += gt1x_calc_checksum(fw, block_len);
		checksum = (0 - checksum);

		buffer[0] = ((block_len >> 8) & 0xFF);
		buffer[1] = (block_len & 0xFF);
		buffer[2] = ((erase_addr >> 8) & 0xFF);
		buffer[3] = (erase_addr & 0xFF);

		ret = gt1x_i2c_write_with_readback(0x8100, buffer, 4);
		if (ret) {
			TS_LOG_ERR("write length & address fail!\n");
			continue;
		}

		ret = gt1x_i2c_write(0x8100 + 4, fw, block_len);
		if (ret) {
			TS_LOG_ERR("write fw data fail!\n");
			continue;
		}

		ret = gt1x_recall_check(fw, 0x8100 + 4, block_len);
		if (ret) {
			continue;
		}

		buffer[0] = ((checksum >> 8) & 0xFF);
		buffer[1] = (checksum & 0xFF);
		ret = gt1x_i2c_write_with_readback(0x8100 + 4 + block_len, buffer, 2);
		if (ret) {
			TS_LOG_ERR("write checksum fail!\n");
			continue;
		}

		buffer[0] = 0;
		ret = gt1x_i2c_write_with_readback(0x8022, buffer, 1);
		if (ret) {
			TS_LOG_ERR("clear control flag fail!\n");
			continue;
		}

		buffer[0] = FW_SECTION_TYPE_SS51_PATCH;
		buffer[1] = FW_SECTION_TYPE_SS51_PATCH;
		ret = gt1x_i2c_write_with_readback(0x8020, buffer, 2);
		if (ret) {
			TS_LOG_ERR("write subsystem type fail!\n");
			continue;
		}
		burn_state = ERROR;
		wait_time = 200;
		while (wait_time > 0) {
			wait_time--;
			msleep(5);
			ret = gt1x_i2c_read_dbl_check(0x8022, buffer, 1);
			if (ret) {
				continue;
			}
			//check clear status
			if (buffer[0] == 0xAA) {
				TS_LOG_DEBUG("burning.....\n");
				continue;
			} else if (buffer[0] == 0xDD) {
				TS_LOG_ERR("checksum error!");
				break;
			} else if (buffer[0] == 0xBB) {
				TS_LOG_INFO("burning success.\n");
				burn_state = 0;
				break;
			} else if (buffer[0] == 0xCC) {
				TS_LOG_ERR("burning failed!\n");
				break;
			} else {
				TS_LOG_DEBUG("unknown state!(0x8022: 0x%02X)\n", buffer[0]);
			}
		}
	}

	if(fw){
		kfree(fw);
		fw = NULL;
	}
	if (burn_state == 0) {
		return NO_ERR;
	} else {
		return ERROR_RETRY;
	}
}

void gt1x_leave_update_mode(void)
{
	TS_LOG_DEBUG("Leave FW update mode.\n");
	if (update_info.status != UPDATE_STATUS_ABORT){
		if (gt1x_ts){
			gt1x_ts->ops.chip_reset();
			gt1x_ts->noise_env = false;
			gt1x_ts->ops.feature_resume(gt1x_ts);
		}
	}
}

