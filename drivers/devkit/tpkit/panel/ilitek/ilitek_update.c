#include "ilitek_ts.h"
#include "ilitek_dts.h"
#include <linux/firmware.h>
#include <linux/vmalloc.h>

#define ILITEK_FW_FILENAME_SD	"ts/touch_screen_firmware.bin"

extern struct i2c_data *ilitek_data;

int inwrite(unsigned int address)
{
	uint8_t outbuff[64];
	int data, ret;
	outbuff[0] = REG_START_DATA;
	outbuff[1] = (char)((address & DATA_SHIFT_0) >> 0);
	outbuff[2] = (char)((address & DATA_SHIFT_8) >> 8);
	outbuff[3] = (char)((address & DATA_SHIFT_16) >> 16);
	ret = ilitek_i2c_write( outbuff, 4);
	if (ret ) {
		TS_LOG_ERR("%s:fai", __func__);
		return TRANSMIT_ERROR;
	}
	udelay(10);
	ret=ilitek_i2c_read(outbuff, 0, outbuff, 4);
	if (ret ) {
		TS_LOG_ERR("%s:fai", __func__);
		return TRANSMIT_ERROR;
	}
	data = (outbuff[0] + outbuff[1] * 256 + outbuff[2] * 256 * 256 + outbuff[3] * 256 * 256 * 256);
	TS_LOG_DEBUG("%s, data=0x%x, outbuff[0]=%x, outbuff[1]=%x, outbuff[2]=%x, outbuff[3]=%x\n", __func__, data, outbuff[0], outbuff[1], outbuff[2], outbuff[3]);
	return data;
}

int outwrite(unsigned int address, unsigned int data, int size)
{
	int ret, i;
	char outbuff[64];
	outbuff[0] = REG_START_DATA;
	outbuff[1] = (char)((address & DATA_SHIFT_0) >> 0);
	outbuff[2] = (char)((address & DATA_SHIFT_8) >> 8);
	outbuff[3] = (char)((address & DATA_SHIFT_16) >> 16);
	for(i = 0; i < size; i++)
	{
		outbuff[i + 4] = (char)(data >> (8 * i));
	}
	ret = ilitek_i2c_write( outbuff, size + 4);
	if (ret ) {
		TS_LOG_ERR("%s:fai", __func__);
		return TRANSMIT_ERROR;
	}
	return ret;
}

static int ilitek_set_firmware_ver(unsigned char * firmware_ver)
{
	int ret = 0;
	ilitek_data->firmware_ver[0] = firmware_ver[0];
	ilitek_data->firmware_ver[1] = firmware_ver[1];
	ilitek_data->firmware_ver[2] = firmware_ver[2];
	snprintf(ilitek_data->ilitek_chip_data->version_name,MAX_STR_LEN-1,"%02x.%02x.%02x",firmware_ver[0], firmware_ver[1], firmware_ver[2]);
	if (ret < 0) {
		TS_LOG_ERR("%s(line %d): error,ret=%d\n",__func__,__LINE__,ret);
	}
	return ret;
}
int ilitek_ready_to_update_fw(void) {
	int ret = 0;
	ret = outwrite(ENTER_ICE_MODE, ENTER_ICE_MODE_NO_DATA, 0);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(WDTRLDT, WDTRLDT_CLOSE, 2);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(WDTCNT1, WDTCNT1_OPEN, 1);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(WDTCNT1, WDTCNT1_CLOSE, 1);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(CLOSE_10K_WDT1, CLOSE_10K_WDT1_VALUE, 4);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(CLOSE_10K_WDT2, CLOSE_10K_WDT2_VALUE, 1);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(CLOSE_10K_WDT3, CLOSE_10K_WDT3_VALUE, 4);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	TS_LOG_INFO("%s, release Power Down Release mode\n", __func__);
	ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_RELEASE_FROM_POWER_DOWN, 1);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(1);
	ret = outwrite(REG_TIMING_SET, REG_TIMING_SET_10MS, 1);
	if (ret ==  (TRANSMIT_ERROR)) {
		TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
		return ret;
	}
	msleep(5);
	return NO_ERR;
}

int ilitek_erase_flash_data(void) {
	int i = 0;
	int j = 0;
	int temp = 0;
	int ret = NO_ERR;
	unsigned char buf[4] = {0};
	for (i = 0; i <= SECTOR_ENDADDR; i += SECTOR_SIZE) {
		TS_LOG_DEBUG("%s, i = %X\n", __func__, i);
		ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_WRITE_ENABLE, 1);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		msleep(1);
		ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		msleep(1);
		temp = (i << 8) + REG_FLASH_CMD_DATA_ERASE;
		ret = outwrite(REG_FLASH_CMD, temp, 4);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		msleep(1);
		ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		msleep(15);
		for (j = 0; j < 50; j++) {
			ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_READ_FLASH_STATUS, 1);
			if (ret ==  (TRANSMIT_ERROR)) {
				TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
				return ret;
			}
			ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
			if (ret ==  (TRANSMIT_ERROR)) {
				TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
				return ret;
			}
			msleep(1);
			buf[0] = inwrite(FLASH_STATUS);
			if (ret ==  (TRANSMIT_ERROR)) {
				TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
				return ret;
			}
			TS_LOG_DEBUG("%s, buf[0] = %X\n", __func__, buf[0]);
			if (buf[0] == 0) {
				break;
			}
			else {
				msleep(2);
			};
		}
		if (j >= 50) {
			TS_LOG_INFO("FLASH_STATUS ERROR j = %d, buf[0] = 0x%X\n", j, buf[0]);
		}
	}
	msleep(100);
	return NO_ERR;
}

int ilitek_update_flash_data(int ap_startaddr, int ap_endaddr, const struct firmware *fw) {
	int i = 0;
	int k = 0;
	int ret = NO_ERR;
	int temp = 0;
	unsigned char buf[300] = {0};
	for(i = ap_startaddr; i < ap_endaddr; i += UPGRADE_TRANSMIT_LEN) {
		TS_LOG_DEBUG("%s, i = %X\n", __func__, i);
		ret = outwrite(REG_FLASH_CMD, REG_FLASH_CMD_WRITE_ENABLE, 1);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_TRIGGER_KEY, 4);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		temp = (i << 8) + REG_FLASH_CMD_DATA_PROGRAMME;
		ret = outwrite(REG_FLASH_CMD, temp, 4);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		ret = outwrite(REG_PGM_NUM, REG_PGM_NUM_256, 4);
		if (ret ==  (TRANSMIT_ERROR)) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			return ret;
		}
		buf[0] = REG_START_DATA;
		buf[3] = (char)((REG_PGM_DATA  & DATA_SHIFT_16) >> 16);
		buf[2] = (char)((REG_PGM_DATA  & DATA_SHIFT_8) >> 8);
		buf[1] = (char)((REG_PGM_DATA  & DATA_SHIFT_0));
		for(k = 0; k < UPGRADE_TRANSMIT_LEN; k++)
		{
			buf[4 + k] = fw->data[i  + k];
		}

		if(ilitek_i2c_write( buf, UPGRADE_TRANSMIT_LEN + REG_LEN) < 0) {
			TS_LOG_ERR("%s, write data error, address = 0x%X, start_addr = 0x%X, end_addr = 0x%X\n", __func__, (int)i, (int)ap_startaddr, (int)ap_endaddr);
			return ret;
		}
		mdelay(3);
	}
	return NO_ERR;
}

int ilitek_fw_update(char *file_name)
{
	int ret = 0, upgrade_status = 0, i = 0, j = 0, k = 0;
	unsigned char buf[3] = {0};
	unsigned long ap_startaddr = 0, ap_endaddr = 0, temp = 0, ap_len = 0;
	unsigned char firmware_ver[4];
	int retry = 0;
	const struct firmware *fw;
	int set_appinfo_ret;
	ap_startaddr = AP_STARTADDR;
	ap_endaddr = AP_ENDADDR;
	ap_len = ap_endaddr - ap_startaddr;
	TS_LOG_INFO("ap_startaddr=0x%lX, ap_endaddr=0x%lX,\n", ap_startaddr, ap_endaddr);
	ret = request_firmware(&fw, file_name, &ilitek_data->ilitek_dev->dev);
	if (ret) {
		TS_LOG_INFO("[ILITEK] failed to request firmware %s: %d\n",
			file_name, ret);
		return ret;
	}
	TS_LOG_INFO("ilitek fw->size = %d\n", (int)fw->size);
	if (((int)fw->size) != FW_UPGRADE_LEN) {
		TS_LOG_INFO("(((int)fw->size) != FW_UPGRADE_LEN) failed RETURN\n");
		return FW_REQUEST_ERR;
	}
	firmware_ver[0] = fw->data[FW_VERSION1];
	firmware_ver[1] = fw->data[FW_VERSION2];
	firmware_ver[2] = fw->data[FW_VERSION3];

	TS_LOG_INFO("firmware_ver[0] = %d, firmware_ver[1] = %d firmware_ver[2]=%d\n",firmware_ver[0], firmware_ver[1], firmware_ver[2]);
	if (!(ilitek_data->force_upgrade)) {
		for (i = 0; i < 3; i++) {
			TS_LOG_INFO("ilitek_data->firmware_ver[%d] = %d, firmware_ver[%d] = %d\n", i, ilitek_data->firmware_ver[i], i, firmware_ver[i ]);
			if (ilitek_data->firmware_ver[i] != firmware_ver[i]) {
				TS_LOG_INFO("%s firmware version is not same so upgrade\n", __func__);
				break;
			}
			if (i == 2) {
				TS_LOG_INFO("%s firmware version is  same so not need upgrade\n", __func__);
				set_appinfo_ret = ilitek_set_firmware_ver(ilitek_data->firmware_ver);
				if (set_appinfo_ret) {
					TS_LOG_INFO("%s:%d set app info err\n",__FUNCTION__,__LINE__);
				}
				return NOT_NEED_UPGRADE;
			}
		}
	}
	ilitek_data->firmware_updating = true;
	if (ic2120) {
Retry:
		if (retry < 2) {
			retry++;
		}
		else {
			release_firmware(fw);
			ilitek_data->firmware_updating = false;
			TS_LOG_INFO("%s,upgrade fail\n", __func__);
			return UPGRADE_FAIL;
		}
		ret = ilitek_ready_to_update_fw();
		if (ret) {
			goto Retry;
		}
		ret = ilitek_erase_flash_data();
		if (ret) {
			goto Retry;
		}
		ilitek_update_flash_data(ap_startaddr, ap_endaddr, fw);
		if (ret) {
			goto Retry;
		}
		buf[0] = (unsigned char)(EXIT_ICE_MODE & DATA_SHIFT_0);
		buf[1] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_8) >> 8);
		buf[2] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_16) >> 16);
		buf[3] = (unsigned char)((EXIT_ICE_MODE & DATA_SHIFT_24) >> 24);
		ret = ilitek_i2c_write( buf, 4);
		if (ret) {
			TS_LOG_ERR("%s, line = %d TRANSMIT_ERROR\n", __func__, __LINE__);
			goto Retry;
		}
		ilitek_reset(ILITEK_RESET_MODEL_CHECKFW_DELAY);
		ret = ilitek_check_int_low(INT_POLL_SHORT_RETRY);
		if (ret != ILITEK_INT_STATUS_LOW) {
			TS_LOG_ERR("ilitek reset but int not pull low so retry\n");
			goto Retry;
		}
		else {
			TS_LOG_INFO("ilitek reset  int  pull low  write 0x10 cmd\n");
		}
		buf[0] = ILITEK_TP_CMD_READ_DATA;
		ret = ilitek_i2c_write_and_read( buf, 1, 0, buf, 3);
		if (ret) {
			TS_LOG_ERR("%s, line = %d ilitek_i2c_write_and_read\n", __func__, __LINE__);
			goto Retry;
		}
		TS_LOG_INFO("%s, buf = %X, %X, %X\n", __func__, buf[0], buf[1], buf[2]);
		if (buf[1] >= FW_OK) {
			ilitek_data->force_upgrade = false;
			TS_LOG_INFO("%s,upgrade ok ok \n", __func__);
			set_appinfo_ret = ilitek_set_firmware_ver(firmware_ver);
			if (set_appinfo_ret) {
				TS_LOG_INFO("%s:%d set app info err\n",__FUNCTION__,__LINE__);
			}
		}else {
			goto Retry;
		}
	}
	release_firmware(fw);
	ilitek_data->firmware_updating = false;
	return UPGRADE_OK;
}

int ilitek_fw_update_boot(char *file_name) {
	int ret = 0;
	char comp_name[ILITEK_VENDOR_COMP_NAME_LEN] = {0};
	const char *producer = NULL;
	char * project_id = ilitek_data->product_id;
	struct device_node *np = NULL;
	ret = ilitek_get_vendor_compatible_name(project_id, comp_name,
		ILITEK_VENDOR_COMP_NAME_LEN);
	if (ret) {
		TS_LOG_ERR("%s:get compatible name fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	TS_LOG_INFO("ilitek comp_name = %s\n", comp_name);
	np = of_find_compatible_node(NULL, NULL, comp_name);
	if (!np) {
		TS_LOG_ERR("%s:find dev node faile, compatible name:%s\n",
			__func__, comp_name);
		return -ENODEV;
	}
	ret = of_property_read_string(np, "producer", &producer);
	if (ret){
		TS_LOG_ERR("%s:find producer in dts fail, ret=%d\n",
			__func__, ret);
		return ret;
	}
	TS_LOG_INFO("ilitek producer = %s\n", producer);
	strncpy(ilitek_data->ilitek_chip_data->module_name,producer,MAX_STR_LEN-1);
	snprintf(file_name, ILITEK_FW_NAME_LEN, "ts/%s_%s_%s_%s.bin",
		ilitek_data->ilitek_chip_data->ts_platform_data->product_name, ilitek_data->ilitek_chip_data->chip_name,ilitek_data->product_id, producer);
	TS_LOG_INFO("%s:update firmware file name =%s\n", __func__, file_name);
	ret = ilitek_fw_update(file_name);
	if (ret){
		TS_LOG_ERR("%s:ilitek_fw_update_boot fail, ret=%d\n",__func__, ret);
	}
	return ret ;
}

int ilitek_fw_update_sd(char *file_name) {
	int ret = 0;
	ilitek_data->force_upgrade = true;
	ret =  ilitek_fw_update(ILITEK_FW_FILENAME_SD);
	if (ret){
		TS_LOG_ERR("%s:ilitek_fw_update_sd fail, ret=%d\n",__func__, ret);
	}
	return ret ;
}
