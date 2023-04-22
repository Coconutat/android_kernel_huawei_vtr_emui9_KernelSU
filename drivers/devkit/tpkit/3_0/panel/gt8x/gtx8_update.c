#include "gtx8.h"

struct fw_update_ctrl update_ctrl = {
	.status = UPSTA_NOTWORK,
	.progress = FW_UPDATE_0_PERCENT,
	.force_update = false,
};

/**
 * gtx8_reg_write_confirm - write register and confirm the value
 *  in the register.
 * @addr: register address
 * @data: pointer to data buffer
 * @len: data length
 * return: 0 write success and confirm ok
 *		   < 0 failed
 */
static int gtx8_reg_write_confirm(u16 addr, u8 *data, u32 len)
{
	u8 *cfm = NULL;
	u8 cfm_buf[32] = {0};
	int i = 0;
	int ret = NO_ERR;

	if (len > sizeof(cfm_buf)) {
		cfm = kzalloc(len, GFP_KERNEL);
		if (!cfm) {
			TS_LOG_ERR("Mem alloc failed\n");
			return -ENOMEM;
		}
	} else {
		cfm = &cfm_buf[0];
	}

	for (i = 0; i < GTX8_RETRY_NUM_3; i++) {
		ret = gtx8_ts->ops.write_trans(addr, data, len);
		if (ret < 0)
			goto exit;

		ret = gtx8_ts->ops.read_trans(addr, cfm, len);
		if (ret < 0)
			goto exit;

		if (memcmp(data, cfm, len)) {
			ret = -EMEMCMP;
			continue;
		} else {
			ret = 0;
			break;
		}
	}

exit:
	if (cfm != &cfm_buf[0]){
		kfree(cfm);
		cfm = NULL;
	}
	return ret;
}

static int gtx8_reg_write(u16 addr, u8 *data, u32 len)
{
	return gtx8_ts->ops.write_trans(addr, data, len);
}

static int gtx8_reg_read(u16 addr, u8 *data, u32 len)
{
	return gtx8_ts->ops.read_trans(addr, data, len);
}


/**
 * gtx8_request_firmware - request firmware data from user space
 *
 * @fw_data: firmware struct, contains firmware header info
 *	and firmware data pointer.
 * return: 0 - OK, < 0 - error
 */
static int gtx8_request_firmware(struct firmware_data *fw_data,
				const char *name)
{
	int ret = NO_ERR;
	struct device *dev = &gtx8_ts->pdev->dev;

	TS_LOG_INFO("Request firmware image [%s]\n", name);
	ret = request_firmware(&fw_data->firmware, name, dev);
	if (ret < 0)
		TS_LOG_ERR("Firmware image [%s] not available,errno:%d\n",
					name, ret);
	else
		TS_LOG_INFO("Firmware image [%s] is ready\n", name);

	return ret;
}

/**
 * relase firmware resources
 *
 */
static void gtx8_release_firmware(struct firmware_data *fw_data)
{
	if (fw_data->firmware) {
		release_firmware(fw_data->firmware);
		fw_data->firmware = NULL;
	}
}

/**
 * gtx8_parse_firmware - parse firmware header infomation
 *	and subsystem infomation from firmware data buffer
 *
 * @fw_data: firmware struct, contains firmware header info
 *	and firmware data.
 * return: 0 - OK, < 0 - error
 */
static int gtx8_parse_firmware(struct firmware_data *fw_data)
{
	const struct firmware *firmware;
	struct firmware_info *fw_info = NULL;
	unsigned int i = 0, fw_offset = 0, info_offset = 0;
	u16 checksum = 0;
	int ret = NO_ERR;

	if (!fw_data || !fw_data->firmware) {
		TS_LOG_ERR("Invalid firmware data\n");
		return -EINVAL;
	}
	fw_info = &fw_data->fw_info;

	/* copy firmware head info */
	firmware = fw_data->firmware;
	if (firmware->size < FW_SUBSYS_INFO_OFFSET) {
		TS_LOG_ERR("Invalid firmware size:%zu\n", firmware->size);
		ret = -EINVAL;
		goto err_size;
	}
	memcpy(fw_info, firmware->data, 
		sizeof(struct firmware_info) > FW_SUBSYS_INFO_OFFSET ? FW_SUBSYS_INFO_OFFSET : sizeof(struct firmware_info));

	/* check firmware size */
	fw_info->size = be32_to_cpu(fw_info->size);
	if (firmware->size != fw_info->size + 6) {
		TS_LOG_ERR("Bad firmware, size not match\n");
		ret = -EINVAL;
		goto err_size;
	}

	/* calculate checksum, note: sum of bytes, but check
	 * by u16 checksum */
	for (i = 6, checksum = 0; i < firmware->size; i++)
		checksum += firmware->data[i];

	/* byte order change, and check */
	fw_info->checksum = be16_to_cpu(fw_info->checksum);
	if (checksum != fw_info->checksum) {
		TS_LOG_ERR("Bad firmware, cheksum error\n");
		ret = -EINVAL;
		goto err_size;
	}

	if (fw_info->subsys_num > FW_SUBSYS_MAX_NUM) {
		TS_LOG_ERR("Bad firmware, invalid subsys num: %d\n",
		       fw_info->subsys_num);
		ret = -EINVAL;
		goto err_size;
	}

	/* parse subsystem info */
	fw_offset = FW_HEADER_SIZE;
	for (i = 0; i < fw_info->subsys_num; i++) {
		info_offset = FW_SUBSYS_INFO_OFFSET +
					i * FW_SUBSYS_INFO_SIZE;

		fw_info->subsys[i].type = firmware->data[info_offset];
		fw_info->subsys[i].size =
				be32_to_cpup((__be32 *)&firmware->data[info_offset + 1]);
		fw_info->subsys[i].flash_addr =
				be16_to_cpup((__be16 *)&firmware->data[info_offset + 5]);

		if (fw_offset > firmware->size) {
			TS_LOG_ERR("Sybsys offset exceed Firmware size\n");
			goto err_size;
		}

		fw_info->subsys[i].data = firmware->data + fw_offset;
		fw_offset += fw_info->subsys[i].size;
	}

	TS_LOG_INFO("Firmware package protocol: V%u\n", fw_info->protocol_ver);
	TS_LOG_INFO("Fimware PID:GT%s\n", fw_info->fw_pid);
	TS_LOG_INFO("Fimware VID:%02X%02X%02X%02X\n", fw_info->fw_vid[0],
				fw_info->fw_vid[1], fw_info->fw_vid[2],
				fw_info->fw_vid[3]);
	TS_LOG_INFO("Firmware chip type:%02X\n", fw_info->chip_type);
	TS_LOG_INFO("Firmware size:%u\n", fw_info->size);
	TS_LOG_INFO("Firmware subsystem num:%u\n", fw_info->subsys_num);

	for (i = 0; i < fw_info->subsys_num; i++) {
		TS_LOG_DEBUG("------------------------------------------\n");
		TS_LOG_DEBUG("Index:%d\n", i);
		TS_LOG_DEBUG("Subsystem type:%02X\n", fw_info->subsys[i].type);
		TS_LOG_DEBUG("Subsystem size:%u\n", fw_info->subsys[i].size);
		TS_LOG_DEBUG("Subsystem flash_addr:%08X\n", fw_info->subsys[i].flash_addr);
		TS_LOG_DEBUG("Subsystem Ptr:%p\n", fw_info->subsys[i].data);
	}
	TS_LOG_DEBUG("------------------------------------------\n");

err_size:
	return ret;
}

/**
 * gtx8_check_update - compare the version of firmware running in
 *  touch device with the version getting from the firmware file.
 * @fw_info: firmware infomation to be compared
 * return: 0 firmware in the touch device needs to be updated
 *			< 0 no need to update firmware
 */
static int gtx8_check_update(const struct firmware_info *fw_info)
{
	struct gtx8_ts_version fw_ver;
	/*u8 fwimg_cid;*/
	int r = NO_ERR;
	int res = NO_ERR;

	/* read version from chip, if we got invalid
	 * firmware version, maybe fimware in flash is
	 * incorrect, so we need to update firmware */
	r = gtx8_ts->ops.read_version(&fw_ver);
	if (fw_ver.valid) {
		if (memcmp(fw_ver.pid, fw_info->fw_pid, gtx8_ts->reg.pid_len)) {
			TS_LOG_ERR("Product ID is not match\n");
			return -EPERM;
		}

		/*fwimg_cid = fw_info->fw_vid[0];*/
		res = memcmp(fw_ver.vid, fw_info->fw_vid, gtx8_ts->reg.vid_len);
		if (res == 0) {
			TS_LOG_ERR("FW version is equal to the IC's\n");
			return -EPERM;
		} else if (res > 0) {
			TS_LOG_INFO("Warning: fw version is lower the IC's\n");
		}
	} /* else invalid firmware, update firmware */

	TS_LOG_INFO("Firmware needs to be updated\n");
	return 0;
}

/**
 * gtx8_load_isp - load ISP program to deivce ram
 * @fw_data: firmware data
 * return 0 ok, <0 error
 */
static int gtx8_load_isp(struct firmware_data *fw_data)
{
	struct fw_subsys_info *fw_isp = NULL;
	u8 reg_val[8] = {0x00};
	int i = 0;
	int ret = NO_ERR;

	fw_isp = &fw_data->fw_info.subsys[0];

	TS_LOG_INFO("Loading ISP start\n");
	/* select bank0 */
	reg_val[0] = 0x00;
	ret = gtx8_reg_write(HW_REG_BANK_SELECT,
				     reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed to select bank0\n");
		return ret;
	}
	TS_LOG_DEBUG("Success select bank0, Set 0x%x -->0x00\n",
					HW_REG_BANK_SELECT);

	/* enable bank0 access */
	reg_val[0] = 0x01;
	ret = gtx8_reg_write(HW_REG_ACCESS_PATCH0,
				     reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed to enable patch0 access\n");
		return ret;
	}
	TS_LOG_DEBUG("Success select bank0, Set 0x%x -->0x01",
					HW_REG_ACCESS_PATCH0);

	ret = gtx8_reg_write_confirm(HW_REG_ISP_ADDR,
				     (u8 *)fw_isp->data, fw_isp->size);
	if (ret < 0) {
		TS_LOG_ERR("Loading ISP error\n");
		return ret;
	}

	TS_LOG_DEBUG("Success send ISP data to IC\n");

	/* forbid patch access */
	reg_val[0] = 0x00;
	ret = gtx8_reg_write_confirm(HW_REG_ACCESS_PATCH0,
				     reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed to disable patch0 access\n");
		return ret;
	}
	TS_LOG_DEBUG("Success forbit bank0 accedd, set 0x%x -->0x00\n",
					HW_REG_ACCESS_PATCH0);

	/*clear 0x6006 0x6007*/
	reg_val[0] = 0x00;
	reg_val[1] = 0x00;
	ret = gtx8_reg_write(HW_REG_ISP_RUN_FLAG, reg_val, 2);
	if (ret < 0) {
		TS_LOG_ERR("Failed to clear 0x%x\n", HW_REG_ISP_RUN_FLAG);
		return ret;
	}
	TS_LOG_DEBUG("Success clear 0x%x\n", HW_REG_ISP_RUN_FLAG);

	/* TODO: change address 0xBDE6 set backdoor flag HW_REG_CPU_RUN_FROM */
	memset(reg_val, 0x55, sizeof(reg_val));
	ret = gtx8_reg_write(HW_REG_CPU_RUN_FROM, reg_val, 8);
	if (ret < 0) {
		TS_LOG_ERR("Failed set backdoor flag\n");
		return ret;
	}
	TS_LOG_DEBUG("Success write [8]0x55 to 0x%x\n",
					HW_REG_CPU_RUN_FROM);

	/* TODO: change reg_val 0x08---> 0x00 release ss51 */
	reg_val[0] = 0x00;
	ret = gtx8_reg_write(HW_REG_CPU_CTRL,
				     reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed to run isp\n");
		return ret;
	}
	TS_LOG_DEBUG("Success run isp, set 0x%x-->0x00\n",
					HW_REG_CPU_CTRL);

	/* check isp work state */
	for (i = 0; i < 200; i++) {
		ret = gtx8_reg_read(HW_REG_ISP_RUN_FLAG,
				    reg_val, 2);
		if (ret < 0 || (reg_val[0] == 0xAA && reg_val[1] == 0xBB))
			break;
		usleep_range(5000, 5100);
	}
	if (reg_val[0] == 0xAA && reg_val[1] == 0xBB) {
		TS_LOG_INFO("ISP working OK\n");
		return NO_ERR;
	} else {
		TS_LOG_ERR("ISP not work,0x%x=0x%x, 0x%x=0x%x\n",
			HW_REG_ISP_RUN_FLAG, reg_val[0],
			HW_REG_ISP_RUN_FLAG + 1, reg_val[1]);
		return -EFAULT;
	}
}

/**
 * gtx8_update_prepare - update prepare, loading ISP program
 *  and make sure the ISP is running.
 * return: 0 ok, <0 error
 */
static int gtx8_update_prepare(void)
{
	u8 reg_val[4] = { 0x00 };
	u8 temp_buf[8] = { 0x00};
	int reset_gpio = gtx8_ts->dev_data->ts_platform_data->reset_gpio;
	int retry = 20;
	int ret = NO_ERR;

	gtx8_ts->ops.i2c_write(0x4506, temp_buf, 8);

	TS_LOG_INFO("fwupdate chip reset\n");
	gpio_direction_output(reset_gpio, 0);
	udelay(2000);
	gpio_direction_output(reset_gpio, 1);
	udelay(10000);

	retry = 20;
	do {
		reg_val[0] = 0x24;
		ret = gtx8_reg_write_confirm(HW_REG_CPU_CTRL, reg_val, 1);
		if (ret < 0) {
			TS_LOG_INFO("Failed to hold ss51, retry\n");
			msleep(20);
		} else {
			break;
		}
	} while (--retry);
	if (!retry) {
		TS_LOG_ERR("Failed hold ss51,return =%d\n", ret);
		return -EINVAL;
	}
	TS_LOG_DEBUG("Success hold ss51");

	/* enable DSP & MCU power */
	reg_val[0] = 0x00;
	ret = gtx8_reg_write_confirm(HW_REG_DSP_MCU_POWER, reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed enable DSP&MCU power\n");
		return ret;
	}
	TS_LOG_DEBUG("Success enabled DSP&MCU power,set 0x%x-->0x00\n",
		 HW_REG_DSP_MCU_POWER);

	/* disable watchdog timer */
	reg_val[0] = 0x00;
	ret = gtx8_reg_write(0x204B, reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed to clear cache\n");
		return ret;
	}
	TS_LOG_DEBUG("Success clear cache\n");

	reg_val[0] = 0x95;
	ret = gtx8_reg_write(0x2318, reg_val, 1);
	reg_val[0] = 0x00;
	ret |= gtx8_reg_write(0x20B0, reg_val, 1);

	reg_val[0] = 0x27;
	ret |= gtx8_reg_write(0x2318, reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed to disable watchdog\n");
		return ret;
	}
	TS_LOG_DEBUG("Success disable watchdog\n");

	/* set scramble */
	reg_val[0] = 0x00;
	ret = gtx8_reg_write(HW_REG_SCRAMBLE, reg_val, 1);
	if (ret < 0) {
		TS_LOG_ERR("Failed to set scramble\n");
		return ret;
	}
	TS_LOG_DEBUG("Succcess set scramble");

	/* load ISP code and run form isp */
	ret = gtx8_load_isp(&update_ctrl.fw_data);
	if (ret < 0)
		TS_LOG_ERR("Failed lode and run isp\n");

	return ret;
}

/**
 * gtx8_format_fw_packet - formate one flash packet
 * @pkt: target firmware packet
 * @flash_addr: flash address
 * @size: packet size
 * @data: packet data
 */
static int gtx8_format_fw_packet(u8 *pkt, u32 flash_addr,
				   u16 len, const u8 *data)
{
	u16 checksum = 0;
	if (!pkt || !data)
		return -EINVAL;

	/*
	 * checksum rule:sum of data in one format is equal to zero
	 * data format: byte/le16/be16/le32/be32/le64/be64
	 */
	pkt[0] = (len >> 8) & 0xff;
	pkt[1] = len & 0xff;
	pkt[2] = (flash_addr >> 16) & 0xff; /* u16 >> 16bit seems nosense but really important */
	pkt[3] = (flash_addr >> 8) & 0xff;
	memcpy(&pkt[4], data, len);
	checksum = checksum_be16(pkt, len + 4);
	checksum = 0 - checksum;
	pkt[len + 4] = (checksum >> 8) & 0xff;
	pkt[len + 5] = checksum & 0xff;
	return NO_ERR;
}

/**
 * gtx8_send_fw_packet - send one firmware packet to ISP
 * @dev: target touch device
 * @pkt: firmware packet
 * return: 0 ok, < 0 error
 */
static int gtx8_send_fw_packet(u8 type, u8 *pkt, u32 len)
{
	u8 reg_val[4] = {0};
	int ret = NO_ERR, i = 0;

	if (!pkt)
		return -EINVAL;

	ret = gtx8_reg_write_confirm(HW_REG_ISP_BUFFER, pkt, len);
	if (ret < 0) {
		TS_LOG_ERR("Failed to write firmware packet\n");
		return ret;
	}

	reg_val[0] = 0;
	reg_val[1] = 0;
	/* clear flash flag 0X6022 */
	ret = gtx8_reg_write_confirm(HW_REG_FLASH_FLAG, reg_val, 2);
	if (ret < 0) {
		TS_LOG_ERR("Faile to clear flash flag\n");
		return ret;
	}

	/* write subsystem type 0X8020*/
	reg_val[0] = type;
	reg_val[1] = type;
	ret = gtx8_reg_write_confirm(HW_REG_SUBSYS_TYPE, reg_val, 2);
	if (ret < 0) {
		TS_LOG_ERR("Failed write subsystem type to IC\n");
		return ret;
	}

	for (i = 0; i < 200; i++) {
		ret = gtx8_reg_read(HW_REG_FLASH_FLAG, reg_val, 2);
		if (ret < 0) {
			TS_LOG_ERR("Failed read flash state\n");
			return ret;
		}

		/* flash haven't end */
		if (reg_val[0] == 0xAA && reg_val[1] == 0xAA) {
			TS_LOG_DEBUG("Flash not ending...\n");
			usleep_range(55000, 56000);
			continue;
		}
		if (reg_val[0] == 0xBB && reg_val[1] == 0xBB) {
			/* read twice to confirm the result */
			ret = gtx8_reg_read(HW_REG_FLASH_FLAG, reg_val, 2);
			if (!ret && reg_val[0] == 0xBB && reg_val[1] == 0xBB) {
				TS_LOG_INFO("Flash subsystem ok\n");
				return NO_ERR;
			}
		}
		if (reg_val[0] == 0xCC && reg_val[1] == 0xCC) {
			TS_LOG_ERR(" Flash subsystem failed\n");
			return -EAGAIN;
		}
		if (reg_val[0] == 0xDD) {
			TS_LOG_ERR("Subsystem checksum err\n");
			return -EAGAIN;
		}
		udelay(250);
	}

	TS_LOG_ERR("Wait for flash end timeout, 0x6022= %x %x\n",
			reg_val[0], reg_val[1]);
	return -EAGAIN;
}

/**
 * gtx8_flash_subsystem - flash subsystem firmware,
 *  Main flow of flashing firmware.
 *	Each firmware subsystem is divided into several
 *	packets, the max size of packet is limited to
 *	@{ISP_MAX_BUFFERSIZE}
 * @subsys: subsystem infomation
 * return: 0 ok, < 0 error
 */
static int gtx8_flash_subsystem(struct fw_subsys_info *subsys)
{
	u16 data_size = 0, offset = 0;
	u32 total_size = 0;
	u32 subsys_base_addr = subsys->flash_addr << 8;
	u8 *fw_packet = NULL;
	int i = 0;
	int ret = NO_ERR;

	/*
	 * if bus(i2c/spi) error occued, then exit, we will do
	 * hardware reset and re-prepare ISP and then retry
	 * flashing
	 */
	total_size = subsys->size;
	fw_packet = kzalloc(ISP_MAX_BUFFERSIZE + 6, GFP_KERNEL);
	if (!fw_packet) {
		TS_LOG_ERR("Failed alloc memory\n");
		return -EINVAL;
	}

	offset = 0;
	while (total_size > 0) {
		data_size = total_size > ISP_MAX_BUFFERSIZE ?
				ISP_MAX_BUFFERSIZE : total_size;
		TS_LOG_INFO("Flash firmware to %08x,size:%u bytes\n",
			subsys_base_addr + offset, data_size);

		/* format one firmware packet */
		ret = gtx8_format_fw_packet(fw_packet, subsys_base_addr + offset,
				data_size, &subsys->data[offset]);
		if (ret < 0) {
			TS_LOG_ERR("Invalid packet params\n");
			goto exit;
		}

		/* send one firmware packet, retry 3 time if send failed */
		for (i = 0; i < 3; i++) {
			ret = gtx8_send_fw_packet(subsys->type,
						  fw_packet, data_size + 6);
			if (!ret)
				break;
		}
		if (ret) {
			TS_LOG_ERR("Failed flash subsystem\n");
			goto exit;
		}
		offset += data_size;
		total_size -= data_size;
	} /* end while */

exit:
	if(fw_packet) {
		kfree(fw_packet);
		fw_packet = NULL;
	}
	return ret;
}

/**
 * gtx8_flash_firmware - flash firmware
 * @fw_data: firmware data
 * return: 0 ok, < 0 error
 */
static int gtx8_flash_firmware(struct firmware_data *fw_data)
{
	struct fw_update_ctrl *fw_ctrl = NULL;
	struct firmware_info  *fw_info = NULL;
	struct fw_subsys_info *fw_x = NULL;
	int retry = GTX8_RETRY_NUM_3;
	int i = 0, fw_num = 0;
	int ret = NO_ERR;

	/* start from subsystem 1,
	 * subsystem 0 is the ISP program */
	fw_ctrl = container_of(fw_data, struct fw_update_ctrl, fw_data);
	fw_info = &fw_data->fw_info;
	fw_num = fw_info->subsys_num;


	for (i = 1; i < fw_num && retry;) {
		TS_LOG_INFO("--- Start to flash subsystem[%d] ---\n", i);
		fw_x = &fw_info->subsys[i];
		ret = gtx8_flash_subsystem(fw_x);
		if (ret == 0) {
			TS_LOG_INFO("--- End flash subsystem[%d]: OK ---\n", i);
			i++;
		} else if (ret == -EAGAIN) {
			retry--;
			TS_LOG_ERR("--- End flash subsystem%d: Fail, errno:%d, retry:%d ---\n",
				i, ret, GTX8_RETRY_NUM_3 - retry);
		} else if (ret < 0) { /* bus error */
			TS_LOG_ERR("--- End flash subsystem%d: Fatal error:%d exit ---\n",
				i, ret);
			goto exit_flash;
		}
	}
exit_flash:
	return ret;
}

/**
 * gtx8_update_finish - update finished, free resource
 *  and reset flags---
 * return: 0 ok, < 0 error
 */
static int gtx8_update_finish(void)
{
	u8 reg_val[8] = {0};
	int ret = NO_ERR;

	/* hold ss51 */
	reg_val[0] = 0x24;
	ret = gtx8_reg_write(HW_REG_CPU_CTRL, reg_val, 1);
	if (ret < 0)
		TS_LOG_ERR("Failed to hold ss51\n");

	/* clear back door flag */
	memset(reg_val, 0, sizeof(reg_val));
	ret = gtx8_reg_write(HW_REG_CPU_RUN_FROM, reg_val, 8);
	if (ret) {
		TS_LOG_ERR("Failed set CPU run from normal firmware\n");
		return ret;
	}

	/* release ss51 */
	reg_val[0] = 0x00;
	ret = gtx8_reg_write(HW_REG_CPU_CTRL, reg_val, 1);
	if (ret < 0)
		TS_LOG_ERR("Failed to run ss51\n");

	/*reset*/
	gtx8_ts->ops.chip_reset();
	if (gtx8_ts->ops.read_version(&gtx8_ts->hw_info))
		TS_LOG_ERR("Failed read version after fwupdate\n");

	return ret;
}

int gtx8_fw_update_proc(void)
{
	int ret = NO_ERR;
	int retry0 = FW_UPDATE_RETRY;
	int retry1 = FW_UPDATE_RETRY;

	ret = gtx8_parse_firmware(&update_ctrl.fw_data);
	if (ret < 0) {
		goto err_parse_fw;
	}

	/*update_ctrl.force_update = true;*/

	if (update_ctrl.force_update == false) {
		ret = gtx8_check_update(&update_ctrl.fw_data.fw_info);
		if (ret < 0) {
			/*when tp ic don't need to update fw,it's should rerurn 0 to aviod repord DMD.
			  the function ts_fw_update_boot  will check return value to decide report dmd or not.
			*/
			ret  = 0;
			goto err_check_update;
		}
	}

start_update:

	ret = gtx8_update_prepare();
	if (ret == -EAGAIN && --retry0 > 0) {
		TS_LOG_ERR("Bus error, retry prepare ISP:%d\n",
				FW_UPDATE_RETRY - retry0);
		goto start_update;
	} else if (ret < 0) {
		TS_LOG_ERR("Failed to prepare ISP, exit update:%d\n", ret);
		goto err_fw_prepare;
	}

	ret = gtx8_flash_firmware(&update_ctrl.fw_data);
	if (ret == -ETIMEOUT && --retry1 > 0) {
		/* we will retry[twice] if returns bus error[i2c/spi]
		 * we will do hardware reset and re-prepare ISP and then retry
		 * flashing
		 */
		TS_LOG_ERR("Bus error, retry firmware update:%d\n",
				FW_UPDATE_RETRY - retry1);
		goto start_update;
	} else if (ret < 0) {
		TS_LOG_ERR("Fatal error, exit update:%d\n", ret);
		goto err_fw_flash;
	}


err_fw_flash:
err_fw_prepare:
	gtx8_update_finish();
err_check_update:
err_parse_fw:
	gtx8_ts->ops.feature_resume(gtx8_ts);
	msleep(200);
	return ret;
}

int gtx8_update_firmware(void)
{
	int ret = NO_ERR;

	static DEFINE_MUTEX(fwu_lock);

	mutex_lock(&fwu_lock);

	ret = gtx8_request_firmware(&update_ctrl.fw_data,
			update_ctrl.fw_name);
	if (ret < 0) {
		TS_LOG_ERR("%s:request_firmware error.\n",__func__);
		/*when fw is not exist,it's should rerurn 0 to aviod repord DMD.
		  the function ts_fw_update_boot  will check return value to decide report dmd or not.
		*/
		ret = 0;
		goto out;
	}

	/* ready to update */
	ret = gtx8_fw_update_proc();

	/* clean */
	gtx8_release_firmware(&update_ctrl.fw_data);

out:
	mutex_unlock(&fwu_lock);
	return ret;
}
